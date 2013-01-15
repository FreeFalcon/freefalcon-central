/***************************************************************************\
	serverbrowser.cpp
	Oliver Weichhold
	August 19, 2000
\***************************************************************************/

// TODO
// - Stop beacon when in flight
// - Server context menu: Play, Refresh, Add to favorites (C_PopupList)
// - Password protected servers

#include <windows.h>
#include "falclib.h"
#include "f4vu.h"
#include "Mesg.h"
#include "msginc\sendchatmessage.h"
#include "msginc\requestlogbook.h"
#include "falcmesg.h"
#include "sim\include\otwdrive.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "uicomms.h"
#include "userids.h"
#include "textids.h"
#include "sim\include\commands.h"
#include "CmpClass.h"
#include "flight.h"
#include "queue.h"
#include "Dispcfg.h"
#include "FalcSnd\voicemanager.h"
#include "FalcSnd\voicefilter.h"
#include "remotelb.h"

#include "include\comsup.h"
extern CComModule _Module;
#include <atlbase.h>
#include <atlcom.h>
#include "..\..\..\gnet\include\core.h"       // main symbols

// Imports
extern C_Handler *gMainHandler;
extern C_Parser *gMainParser;
void CloseWindowCB(long ID,short hittype,C_Base *control);
void GetPlayerInfo(VU_ID ID);
extern void Phone_Connect_CB(long n,short hittype,C_Base *control);

#pragma warning(disable:4192)
#import "gnet\bin\core.tlb" 
#import "gnet\bin\shared.tlb" named_guids
#pragma warning(default:4192)

extern char g_strMasterServerName[0x40];
// M.N. EnableUplink UI switch
extern GNETCORELib::IUplinkPtr m_pUplink;
extern bool g_bEnableUplink;
extern int g_nMasterServerPort;
char strVersion[0x20];
extern char g_strServerLocation[0x40];
extern char g_strServerName[0x40];
struct __declspec(uuid("41C27D56-3A03-4E9D-BE01-3423126C3983")) GameSpyUplink;
extern int MajorVersion;
extern int MinorVersion;
extern int gLangIDNum;
extern int BuildNumber;

// Helper classes
class CGNetUpdater : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IGameEvents,
	public IRemoteMasterServerEvents
{
	public:
	CGNetUpdater();

	BEGIN_COM_MAP(CGNetUpdater)
		COM_INTERFACE_ENTRY(IGameEvents)
		COM_INTERFACE_ENTRY(IRemoteMasterServerEvents)
	END_COM_MAP()

	// Attributes
	_bstr_t m_bstrMasterServerName;
	_bstr_t m_bstrName;		// These are copied verbatim from the master server
	_bstr_t m_bstrLocation;
	_bstr_t m_bstrVersion;
	_bstr_t m_bstrMOTD;
	_bstr_t m_bstrAdminEmail;
	_bstr_t m_bstrHostAddress;
	int m_nServerCount;

	GNETCORELib::IRemoteMasterServerPtr m_pMasterServer;
	DWORD m_dwMasterServerCookie;
	DWORD m_dwMasterServerCookie2;
	bool m_bUpdating;

	// Implementation
	void Init();
	void Cleanup();
	void Update();
	void CancelUpdate();
	void UpdateGame(IGame *p, bool bSuccess);
	bool IsUpdating();

	// IGameEvents
	STDMETHOD(UpdateComplete)(IGame *Game);
	STDMETHOD(UpdateAborted)(IGame *Game);

	// IRemoteMasterServerEvents
	STDMETHOD(UpdateComplete)(BOOL bSuccess);
	STDMETHOD(ServerListReceived)(int nServers);
};

class GNetUpdater : public CComObject<CGNetUpdater>
{
	public:
	GNetUpdater()
	{
		AddRef();
	}
};

class C_ServerItem : public C_Control
{
	#ifdef USE_SH_POOLS
	public:
	// Overload new/delete to use a SmartHeap pool
	void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
	void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
	#endif

	public:
	C_ServerItem();
	C_ServerItem(char **stream);
	C_ServerItem(FILE *fp);
	~C_ServerItem();

	// Attributes
	private:
	long DefaultFlags_;
	long Font_;
	int m_nPing;
	int m_nPlayers;
	DWORD m_dwIP;
	COLORREF Color_[4]; //0-off,1=on,2=playerflt,3=disabled
	O_Output *m_arrOutput[5];
	TREELIST *Owner_;
	short State_;

	// Implementation
	protected:
	void Draw(SCREEN *surface,UI95_RECT *cliprect);

	public:
	long Size();
	void Setup(IGame *pGame, C_TreeList *pParent);
	void Cleanup(void);
	void SetFont(long);
	long CheckHotSpots(long relx,long rely);
	BOOL Process(long ID,short HitType);
	void Refresh();
	inline int CompareText(C_ServerItem *pOther, int nIndex);
	inline int ComparePing(C_ServerItem *pOther);
	inline int ComparePlayers(C_ServerItem *pOther);
	void Save(char **);
	void Save(FILE *);
	void SetState(short state);
	short GetState();
	void SetOwner(TREELIST *item);
	TREELIST *GetOwner();
	long GetFont();
	void SetDefaultFlags();
	long GetDefaultFlags();
	inline int TrimString(char *str, int nMaxPixelWidth);
	DWORD GetIP();
};

// Attributes
enum _FILTER_MODE
{
	FILTER_MODE_ALL,
	FILTER_MODE_CAMPAIGN,
	FILTER_MODE_TE,
	FILTER_MODE_DOGFIGHT,
	FILTER_MODE_POPULATED,
	FILTER_MODE_FAVORITES
} m_eFilterMode = FILTER_MODE_ALL;

typedef std::vector<IGame *> GAMEARRAY;
static GAMEARRAY m_arrGames;
static C_Window *m_pWnd;
static C_Text *m_pWndStatus;
static int m_nUpdatedServers = 0;
static C_TreeList *m_pListServers=NULL;
static C_ServerItem *m_pSelectedItem = NULL;
static GNetUpdater *m_pUpdater = NULL;
static HANDLE m_hEventShutdown = NULL;
static bool m_bConnectedToMaster = false;
static bool m_bShutdownPending = false;
static bool m_bCloseWindowPending = false;
static bool m_bConnectPending = false;

// Implementation
static C_ServerItem *MakeServerItem(C_TreeList *tree, IGame *p);
static void Update();
static void UpdateDisplay();
static void SetFilterMode(enum _FILTER_MODE eMode);
static void ClearServerList();
static void UpdateComplete();
static void UpdateStatus(char *strFmt, ...);
static void UpdateServerStatus();

BOOL ServerListSortCB_Name(TREELIST *_pItem1,TREELIST *_pItem2)
{
	C_ServerItem *pItem1 = (C_ServerItem *) _pItem1->Item_;
	C_ServerItem *pItem2 = (C_ServerItem *) _pItem2->Item_;

	return pItem1->CompareText(pItem2, 0) >= 0;
}

BOOL ServerListSortCB_Ping(TREELIST *_pItem1,TREELIST *_pItem2)
{
	C_ServerItem *pItem1 = (C_ServerItem *) _pItem1->Item_;
	C_ServerItem *pItem2 = (C_ServerItem *) _pItem2->Item_;

	return pItem1->ComparePing(pItem2);
}

BOOL ServerListSortCB_Mode(TREELIST *_pItem1,TREELIST *_pItem2)
{
	C_ServerItem *pItem1 = (C_ServerItem *) _pItem1->Item_;
	C_ServerItem *pItem2 = (C_ServerItem *) _pItem2->Item_;

	return pItem1->CompareText(pItem2, 2) >= 0;
}

BOOL ServerListSortCB_Players(TREELIST *_pItem1,TREELIST *_pItem2)
{
	C_ServerItem *pItem1 = (C_ServerItem *) _pItem1->Item_;
	C_ServerItem *pItem2 = (C_ServerItem *) _pItem2->Item_;

	return pItem1->ComparePlayers(pItem2);
}

BOOL ServerListSortCB_Location(TREELIST *_pItem1,TREELIST *_pItem2)
{
	C_ServerItem *pItem1 = (C_ServerItem *) _pItem1->Item_;
	C_ServerItem *pItem2 = (C_ServerItem *) _pItem2->Item_;

	return pItem1->CompareText(pItem2, 4) >= 0;
}

static void OnDeleteItemServerList(TREELIST *item)
{
	if(item->Item_ == m_pSelectedItem)
		m_pSelectedItem = NULL;
}

static void LocalCloseWindowCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	if(m_bCloseWindowPending)
		return;

	if(m_pUpdater && m_pUpdater->IsUpdating())
	{
		m_bCloseWindowPending = true;
		m_pUpdater->CancelUpdate();
		UpdateStatus("Aborting ...");
	}

	else
	{
		C_Base *wndClose = control->GetParent()->FindControl(CLOSE_WINDOW);
		if(wndClose)
			CloseWindowCB(wndClose->GetID(), hittype, wndClose);
	}
}

static void OnClickedBack(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	C_Base *wndClose = control->GetParent()->FindControl(CLOSE_WINDOW);
	if(wndClose)
		LocalCloseWindowCB(wndClose->GetID(), hittype, wndClose);
}
/* 	This crashes, removed from the UI
static void OnClickedSettings(long, short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	C_Window *win;
	C_Button *button;

	win=gMainHandler->FindWindow(JETNET_WIN);
	if(!win) return;

	button = (C_Button*)win->FindControl(SETUP_JETNET_ENABLEUPLINK);
	if (button)
	{
		g_bEnableUplink = !g_bEnableUplink;
		if (g_bEnableUplink) // now we switched from off to on, set up the uplink
		{
			// Make sure all objects are registered
			ComSup::RegisterServer("GNGameSpy.dll");
			ComSup::RegisterServer("GNCorePS.dll");
			ComSup::RegisterServer("GNShared.dll");

			// Create Uplink service object
			CheckHR(m_pUplink.CreateInstance(__uuidof(GameSpyUplink)));

			m_pUplink->PutMasterServerName(g_strMasterServerName);
			m_pUplink->PutMasterServerPort(g_nMasterServerPort);
			m_pUplink->PutQueryPort(7778);
			m_pUplink->PutHeartbeatInterval(60000);
			m_pUplink->PutServerVersion(strVersion);
			m_pUplink->PutServerVersionMin(strVersion);
			m_pUplink->PutServerLocation(g_strServerLocation);
			m_pUplink->PutServerName(g_strServerName);
			m_pUplink->PutGameName("Falcon4");
			m_pUplink->PutGameMode("openplaying");
		}
		else
			m_pUplink->Release();
	}
}*/


static void OnClickedRefresh(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	Update();
}

static void OnClickedPlay(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	if(!m_pSelectedItem || m_bConnectPending)
		return;

	C_Base *wndClose = control->GetParent()->FindControl(CLOSE_WINDOW);
	if(wndClose)
	LocalCloseWindowCB(wndClose->GetID(), hittype, wndClose);

	if(m_bCloseWindowPending)
		m_bConnectPending = true;

	else
		Phone_Connect_CB((long) m_pSelectedItem->GetIP(), hittype, control);
}

static void OnClickedFilter_All(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	SetFilterMode(FILTER_MODE_ALL);
}

static void OnClickedFilter_Campaign(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	SetFilterMode(FILTER_MODE_CAMPAIGN);
}

static void OnClickedFilter_TE(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	SetFilterMode(FILTER_MODE_TE);
}

static void OnClickedFilter_Dogfight(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	SetFilterMode(FILTER_MODE_DOGFIGHT);
}

static void OnClickedFilter_Favorites(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	SetFilterMode(FILTER_MODE_FAVORITES);
}

static void OnClickedFilter_Populated(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	SetFilterMode(FILTER_MODE_POPULATED);
}

static void OnClickedSort_ServerName(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	// F4CSECTIONHANDLE *Leave = UI_Enter(control->Parent_);
	m_pListServers->SetSortCallback(ServerListSortCB_Name);
	m_pListServers->ReorderBranch(m_pListServers->GetRoot());
	m_pListServers->RecalcSize();
	control->Parent_->RefreshClient(m_pListServers->GetClient());
}

static void OnClickedSort_Ping(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	// F4CSECTIONHANDLE *Leave = UI_Enter(control->Parent_);

	m_pListServers->SetSortCallback(ServerListSortCB_Ping);
	m_pListServers->ReorderBranch(m_pListServers->GetRoot());
	m_pListServers->RecalcSize();
	control->Parent_->RefreshClient(m_pListServers->GetClient());

	// UI_Leave(Leave);
}

static void OnClickedSort_Mode(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	// F4CSECTIONHANDLE *Leave = UI_Enter(control->Parent_);

	m_pListServers->SetSortCallback(ServerListSortCB_Mode);
	m_pListServers->ReorderBranch(m_pListServers->GetRoot());
	m_pListServers->RecalcSize();
	control->Parent_->RefreshClient(m_pListServers->GetClient());

	// UI_Leave(Leave);
}

static void OnClickedSort_Players(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	// F4CSECTIONHANDLE *Leave = UI_Enter(control->Parent_);

	m_pListServers->SetSortCallback(ServerListSortCB_Players);
	m_pListServers->ReorderBranch(m_pListServers->GetRoot());
	m_pListServers->RecalcSize();
	control->Parent_->RefreshClient(m_pListServers->GetClient());

	// UI_Leave(Leave);
}

static void OnClickedSort_Location(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	// F4CSECTIONHANDLE *Leave = UI_Enter(control->Parent_);

	m_pListServers->SetSortCallback(ServerListSortCB_Location);
	m_pListServers->ReorderBranch(m_pListServers->GetRoot());
	m_pListServers->RecalcSize();
	control->Parent_->RefreshClient(m_pListServers->GetClient());

	// UI_Leave(Leave);
}


static void OnSelchangeServerList(long n,short hittype,C_Base *control)
{
	switch(hittype)
	{
		case C_TYPE_LMOUSEDOWN:
		{
			// F4CSECTIONHANDLE *Leave = UI_Enter(control->Parent_);
			if(m_pSelectedItem)
			{
				m_pSelectedItem->SetState(static_cast<short>(control->GetState() & ~1));
				m_pSelectedItem->Refresh();
			}

			m_pSelectedItem = (C_ServerItem *) control;
			m_pSelectedItem->SetState(static_cast<short>(control->GetState()|1));
			m_pSelectedItem->Refresh();

			// UI_Leave(Leave);
			break;
		}

		case C_TYPE_LMOUSEDBLCLK:
		{
			OnClickedPlay(n, C_TYPE_LMOUSEUP, control);
			break;
		}

		case C_TYPE_RMOUSEDOWN:
		{
			// Context menu
			break;
		}
	}
}
/*
static void OnClickedSetup(long,short hittype,C_Base *control)
{
    if(hittype != C_TYPE_LMOUSEUP)
	return;
    C_Window *win;
    win=gMainHandler->FindWindow(SETUP_JETNET_OPTIONS_WIN);
    if(win == NULL)
	return;
    
    gMainHandler->ShowWindow(win);
    gMainHandler->WindowToFront(win);
}
*/
static BOOL MainKBCallback(unsigned char DKScanCode,unsigned char Ascii,unsigned char ShiftStates,long RepeatCount)
{
	switch(DKScanCode)
	{
		case DIK_RETURN:
		case DIK_NUMPADENTER:
		{
			if(RepeatCount == 1 && m_pSelectedItem)
			{
				OnClickedPlay(0, C_TYPE_LMOUSEUP, m_pWnd->FindControl(JETNET_BROWSER_PLAY));
				return TRUE;
			}

			break;
		}
	}

	return FALSE;
}

static void OnJNEnableUplink(long,short hittype,C_Base *control)
{



}

void HookupServerBrowserControls(long ID)
{
	C_Window *winme;
	C_Button *ctrl;
	C_TreeList *tree;

	char strVersion[0x20];
	sprintf(strVersion,"%1d.%02d.%1d.%5d",MajorVersion, MinorVersion, gLangIDNum, BuildNumber);

	
	winme=gMainHandler->FindWindow(JETNET_WIN);

	if(winme == NULL)
		return;

	m_pWnd = winme;
	m_pWnd->SetKBCallback(MainKBCallback);

	ctrl = (C_Button *) winme->FindControl(CLOSE_WINDOW);
	if(ctrl)
		ctrl->SetCallback(LocalCloseWindowCB);

	tree=(C_TreeList*)winme->FindControl(JETNET_SERVER_TREE);
	if(tree)
	{
		m_pListServers=tree;
		m_pListServers->SetDelCallback(OnDeleteItemServerList);
		m_pListServers->SetSortType(TREE_SORT_CALLBACK);
		m_pListServers->SetSortCallback(ServerListSortCB_Ping);
	}

	ctrl = (C_Button *) winme->FindControl(JETNET_BROWSER_BACK);
	if(ctrl)
		ctrl->SetCallback(OnClickedBack);

	ctrl = (C_Button *) winme->FindControl(JETNET_BROWSER_REFRESH);
	if(ctrl)
		ctrl->SetCallback(OnClickedRefresh);

	ctrl = (C_Button *) winme->FindControl(JETNET_BROWSER_PLAY);
	if(ctrl)
		ctrl->SetCallback(OnClickedPlay);

	ctrl = (C_Button *) winme->FindControl(JETNET_BROWSER_FILTER_ALL);
	if(ctrl)
		ctrl->SetCallback(OnClickedFilter_All);

	ctrl = (C_Button *) winme->FindControl(JETNET_BROWSER_FILTER_CAMPAIGN);
	if(ctrl)
		ctrl->SetCallback(OnClickedFilter_Campaign);

	ctrl = (C_Button *) winme->FindControl(JETNET_BROWSER_FILTER_TE);
	if(ctrl)
		ctrl->SetCallback(OnClickedFilter_TE);

	ctrl = (C_Button *) winme->FindControl(JETNET_BROWSER_FILTER_DOGFIGHT);
	if(ctrl)
		ctrl->SetCallback(OnClickedFilter_Dogfight);

	ctrl = (C_Button *) winme->FindControl(JETNET_BROWSER_FILTER_FAVORITES);
	if(ctrl)
		ctrl->SetCallback(OnClickedFilter_Favorites);

	ctrl = (C_Button *) winme->FindControl(JETNET_BROWSER_FILTER_POPULATED);
	if(ctrl)
		ctrl->SetCallback(OnClickedFilter_Populated);

	ctrl = (C_Button *) winme->FindControl(JETNET_SORT_SERVERNAME);
	if(ctrl)
		ctrl->SetCallback(OnClickedSort_ServerName);

	ctrl = (C_Button *) winme->FindControl(JETNET_SORT_PING);
	if(ctrl)
		ctrl->SetCallback(OnClickedSort_Ping);

	ctrl = (C_Button *) winme->FindControl(JETNET_SORT_MODE);
	if(ctrl)
		ctrl->SetCallback(OnClickedSort_Mode);

	ctrl = (C_Button *) winme->FindControl(JETNET_SORT_PLAYERS);
	if(ctrl)
		ctrl->SetCallback(OnClickedSort_Players);

	ctrl = (C_Button *) winme->FindControl(JETNET_SORT_LOCATION);
	if(ctrl)
		ctrl->SetCallback(OnClickedSort_Location);

/*	ctrl = (C_Button *) winme->FindControl(SETUP_JETNET_ENABLEUPLINK);
	if(ctrl)
		ctrl->SetCallback(OnClickedSettings);
*/
	m_pWndStatus = (C_Text *) winme->FindControl(JETNET_STATUS);

	m_hEventShutdown = CreateEvent(NULL, FALSE, FALSE, NULL);

	UpdateServerStatus();

//	SetJNSettings();	// setup window
}

C_ServerItem *MakeServerItem(C_TreeList *pTree, IGame *p)
{
	C_ServerItem *pServerItem;
	C_Window *pWindow;
	TREELIST *pTreeItem;

	if(!pTree || !p)
		return NULL;

	try
	{
		pServerItem=new C_ServerItem;
		if(!pServerItem)
			return(NULL);

		pWindow=pTree->GetParent();
		if(!pWindow)
			return(NULL);

		pServerItem->SetFont(pTree->GetFont());
		pServerItem->SetClient(pTree->GetClient());
		pServerItem->SetW(pWindow->ClientArea_[pTree->GetClient()].right - pWindow->ClientArea_[pTree->GetClient()].left);
		pServerItem->SetH(gFontList->GetHeight(pTree->GetFont()));
		pServerItem->Setup(p, pTree);
		pServerItem->SetCallback(OnSelchangeServerList);

		pTreeItem=pTree->CreateItem(-1, C_TYPE_ITEM, pServerItem);
		pServerItem->SetOwner(pTreeItem);
		
		if(pTree->AddItem(pTree->GetRoot(), pTreeItem))
			return pServerItem;
	}

	catch(_com_error e)
	{
		MonoPrint("MakeServerItem - _com_error 0x%X", e.Error());
	}

	pServerItem->Cleanup();
	delete pServerItem;
	delete pTreeItem;

	return NULL;
}

static void ClearServerList()
{
	GAMEARRAY::iterator it;
	for(it = m_arrGames.begin(); it != m_arrGames.end(); it++)
		if(*it)
			(*it)->Release();

	m_arrGames.clear();
}

static void Update()
{
	try
	{
		if(!m_pUpdater)
		{
			m_pUpdater = new GNetUpdater;
			if(!m_pUpdater)
				throw _com_error(E_OUTOFMEMORY);

			m_pUpdater->Init();
		}

		if(m_pUpdater->IsUpdating())
		{
			m_pUpdater->CancelUpdate();
			UpdateStatus("Aborting ...");
		}

		else
		{
			m_pListServers->DeleteBranch(m_pListServers->GetRoot());		// Delete all items
			ClearServerList();

#if 0
			// Adjust maximum number of concurent server quries according to phonebook bandwidth settings
			C_Window *pWin = gMainHandler->FindWindow(PB_WIN);
			if(pWin)
			{
				C_ListBox *pListBox = (C_ListBox *) pWin->FindControl(SET_JETNET_BANDWIDTH);
				if(pListBox)
				{
					static int Bandwidth2MaxConcurentQueries[] =
					{
						4, // INVALID
						4, // F4_BANDWIDTH_14
						4, // F4_BANDWIDTH_28
						8, // F4_BANDWIDTH_33
						15, // F4_BANDWIDTH_56Modem
						15, // F4_BANDWIDTH_56ISDN
						20, // F4_BANDWIDTH_112
						20, // F4_BANDWIDTH_256
						25 // F4_BANDWIDTH_T1
					};

					int nBandwidth = static_cast<uchar>(pListBox->GetTextID());

					if(nBandwidth < 0)
						nBandwidth = 1;
					else if(nBandwidth > F4_BANDWIDTH_T1)
						nBandwidth = 1;

					m_pUpdater->m_pMasterServer->PutMaxConcurrentHostQueries(Bandwidth2MaxConcurentQueries[nBandwidth]);
				}
			}
#else
			m_pUpdater->m_pMasterServer->PutMaxConcurrentHostQueries(8);
#endif

			m_nUpdatedServers = 0;
			m_bConnectedToMaster = false;

			m_pUpdater->Update();
		}
	}

	catch(_com_error e)
	{
		MonoPrint("Update - _com_error 0x%X", e.Error());
	}
}

static void SetFilterMode(enum _FILTER_MODE eMode)
{
	if(eMode == m_eFilterMode)
		return;

	m_eFilterMode = eMode;
	UpdateDisplay();
}

static inline bool FilterGame(IGame *p)
{
	switch(m_eFilterMode)
	{
		case FILTER_MODE_ALL:
			return true;

		case FILTER_MODE_CAMPAIGN:
		{
			static const _bstr_t bstrCA("CA");
			return GNETCORELib::IGamePtr(p)->GetType() == bstrCA;
		}

		case FILTER_MODE_TE:
		{
			static const _bstr_t bstrTE("TE");
			return GNETCORELib::IGamePtr(p)->GetType() == bstrTE;
		}

		case FILTER_MODE_DOGFIGHT:
		{
			static const _bstr_t bstrDF("DF");
			return GNETCORELib::IGamePtr(p)->GetType() == bstrDF;
		}

		case FILTER_MODE_POPULATED:
			return GNETCORELib::IGamePtr(p)->GetNumPlayers() > 0;

		case FILTER_MODE_FAVORITES:
		{
			// Currently not support
			ShiAssert(false);
			return true;
		}
	}

	return false;
}

static void UpdateDisplay()
{
	try
	{
		// Delete all items
		m_pListServers->DeleteBranch(m_pListServers->GetRoot());

		GAMEARRAY::iterator it;
		GNETCORELib::IGamePtr p;

		for(it = m_arrGames.begin(); it != m_arrGames.end(); it++)
			if(FilterGame(*it))
				MakeServerItem(m_pListServers, *it);

		m_pListServers->ReorderBranch(m_pListServers->GetRoot());
		m_pListServers->RecalcSize();
		m_pListServers->Parent_->RefreshClient(m_pListServers->GetClient());
	}

	catch(_com_error e)
	{
		MonoPrint("UpdateDisplay - _com_error 0x%X", e.Error());
	}
}

static void ServerListReceived(int nServers)
{
	m_bConnectedToMaster = false;

	if(nServers != 0)
		UpdateStatus("Connected .. getting server information");
}

static void UpdateComplete(BOOL bSuccess)
{
	if(m_bShutdownPending)
	{
		m_bShutdownPending = false;
		SetEvent(m_hEventShutdown);
	}

	else if(m_bCloseWindowPending)
	{
		m_bCloseWindowPending = false;

		C_Window *pWin = gMainHandler->FindWindow(JETNET_WIN);
		C_Base *wndClose = pWin->FindControl(CLOSE_WINDOW);

		if(wndClose)
			CloseWindowCB(wndClose->GetID(), C_TYPE_LMOUSEUP, wndClose);

		if(m_bConnectPending)
		{
			m_bConnectPending = false;

			ShiAssert(m_pSelectedItem);
			if(m_pSelectedItem)
				Phone_Connect_CB((long) m_pSelectedItem->GetIP(), C_TYPE_LMOUSEUP, pWin->FindControl(JETNET_BROWSER_PLAY));
		}
	}

	if(bSuccess)
		UpdateStatus("%d Servers", m_nUpdatedServers);

	else
	{
		if(!m_bConnectedToMaster)
			UpdateStatus("Failed to connect to master server");

		else
			UpdateStatus("Update failed");
	}
}

void ServerBrowserExit()
{
	if(m_pUpdater)
	{
		if(m_pUpdater->IsUpdating())
		{
			m_bShutdownPending = true;
			m_pUpdater->CancelUpdate();

			if(m_hEventShutdown)
			{
				AtlWaitWithMessageLoop(m_hEventShutdown);
				CloseHandle(m_hEventShutdown);
				m_hEventShutdown = NULL;
			}
		}

		m_pUpdater->Cleanup();
		delete m_pUpdater;
		m_pUpdater = NULL;

		ClearServerList();
	}
}

static void UpdateStatus(char *strFmt, ...)
{
	if(m_pWndStatus)
	{
		TCHAR strMsg[1024];
		va_list pArg;

		va_start(pArg, strFmt);
		vsprintf(strMsg, strFmt, pArg);
		va_end(pArg);

		m_pWndStatus->SetText(strMsg);
		m_pWndStatus->Refresh();
	}
}

static void UpdateServerStatus()
{
	int nServerCount;

	if(!m_pUpdater || m_pUpdater->m_nServerCount == 0)
		nServerCount = 1;
	else
		nServerCount = m_pUpdater->m_nServerCount;

	UpdateStatus("%.1f%% (%d of %d Servers)", ((float) m_nUpdatedServers / ((float) nServerCount / 100.0f)), m_nUpdatedServers, nServerCount);
}

/////////////////////////////////////////////////////////////////////////////
// C_ServerItem

C_ServerItem::C_ServerItem() : C_Control()
{
	_SetCType_(_CNTL_MISSION_);

	State_=0;

	Color_[0]=0xd0d0d0; // Not Selected
	Color_[1]=0xC8; // Selected
	Color_[2]=0xffff00; // Player is in this mission
	Color_[3]=0x00ff00; // Player is in this mission & current mission

	ZeroMemory(m_arrOutput, sizeof(m_arrOutput));
	Owner_=NULL;
	DefaultFlags_=C_BIT_ENABLED | C_BIT_REMOVE | C_BIT_MOUSEOVER;

	// cached properties
	m_nPing = 0;
	m_nPlayers = 0;
	m_dwIP = 0;
}

C_ServerItem::C_ServerItem(char **stream) : C_Control(stream)
{
}

C_ServerItem::C_ServerItem(FILE *fp) : C_Control(fp)
{
}

C_ServerItem::~C_ServerItem()
{
}

long C_ServerItem::Size()
{
	return(0);
}

void C_ServerItem::Setup(IGame *_pGame, C_TreeList *pParent)
{
	if(!_pGame || !pParent)
		return;

	GNETCORELib::IGamePtr pGame(_pGame);

	m_dwIP = GNETCORELib::IHostPtr(pGame)->GetIP();
	m_nPing = GNETCORELib::IHostPtr(pGame)->GetPing();	// remember for quick sorting
	m_nPlayers = pGame->GetNumPlayers();	// remember for quick sorting

	char buf[0x10];
	_bstr_t str;
	int x, n;

	SetID((long) pGame);
	SetType(0);
	SetDefaultFlags();
	SetReady(1);

	m_arrOutput[0] = new O_Output;
	if(!m_arrOutput[0]) throw _com_error(E_OUTOFMEMORY);
	m_arrOutput[0]->SetOwner(this);
	m_arrOutput[0]->SetFont(Font_);
	m_arrOutput[0]->SetXY(pParent->GetUserNumber(C_STATE_0), 0);
	str = pGame->GetServerName();
	n = TrimString(str, pParent->GetUserNumber(C_STATE_1) - pParent->GetUserNumber(C_STATE_0));
	m_arrOutput[0]->SetTextWidth(n + 1);
	m_arrOutput[0]->SetText(str);

	m_arrOutput[1] = new O_Output;
	if(!m_arrOutput[1]) throw _com_error(E_OUTOFMEMORY);
	m_arrOutput[1]->SetOwner(this);
	m_arrOutput[1]->SetFont(Font_);
	m_arrOutput[1]->SetTextWidth(sprintf(buf, "%4d", GNETCORELib::IHostPtr(pGame)->GetPing()) + 1);
	// Center
	//x = ((pParent->GetUserNumber(C_STATE_2) - pParent->GetUserNumber(C_STATE_1)) - gFontList->StrWidth(Font_, buf, strlen(buf))) / 2;
	x = pParent->GetUserNumber(C_STATE_1);
	m_arrOutput[1]->SetXY(x, 0);
	m_arrOutput[1]->SetText(buf);

	m_arrOutput[2] = new O_Output;
	if(!m_arrOutput[2]) throw _com_error(E_OUTOFMEMORY);
	m_arrOutput[2]->SetOwner(this);
	m_arrOutput[2]->SetFont(Font_);
	str = pGame->GetType();
	n = TrimString(str, pParent->GetUserNumber(C_STATE_3) - pParent->GetUserNumber(C_STATE_2));
	m_arrOutput[2]->SetTextWidth(n + 1);
	m_arrOutput[2]->SetXY(pParent->GetUserNumber(C_STATE_2), 0);
	m_arrOutput[2]->SetText(str);

	m_arrOutput[3] = new O_Output;
	if(!m_arrOutput[3]) throw _com_error(E_OUTOFMEMORY);
	m_arrOutput[3]->SetOwner(this);
	m_arrOutput[3]->SetFont(Font_);
	m_arrOutput[3]->SetTextWidth(sprintf(buf, "%2d/%2d", pGame->GetNumPlayers(), pGame->GetMaxPlayers()) + 1);
	// Center
	//x = ((pParent->GetUserNumber(C_STATE_4) - pParent->GetUserNumber(C_STATE_3)) - gFontList->StrWidth(Font_, (char *) str, str.length())) / 2;
	x = pParent->GetUserNumber(C_STATE_3);
	m_arrOutput[3]->SetXY(x, 0);
	m_arrOutput[3]->SetText(buf);

	m_arrOutput[4] = new O_Output;
	if(!m_arrOutput[4]) throw _com_error(E_OUTOFMEMORY);
	m_arrOutput[4]->SetOwner(this);
	m_arrOutput[4]->SetFont(Font_);
	m_arrOutput[4]->SetXY(pParent->GetUserNumber(C_STATE_4), 0);
	str = pGame->GetLocation();
	m_arrOutput[4]->SetTextWidth(str.length() + 1);
	m_arrOutput[4]->SetText(str);
}

int C_ServerItem::TrimString(char *str, int nMaxPixelWidth)
{
	if(str == NULL)
		return 0;

	int nLength = strlen(str);

	while(nLength && gFontList->StrWidth(Font_, str, nLength) > nMaxPixelWidth)
		nLength--;

	return nLength;
}

void C_ServerItem::Cleanup(void)
{
	for(int i=0;i<5;i++)
	{
		if(m_arrOutput[i])
		{
			m_arrOutput[i]->Cleanup();
			delete m_arrOutput[i];
			m_arrOutput[i]=NULL;
		}
	}
}

void C_ServerItem::SetFont(long id)
{
	Font_ = id;

	for(int i=0;i<5;i++)
	{
		if(m_arrOutput[i])
			m_arrOutput[i]->SetFont(Font_);
	}
}

long C_ServerItem::CheckHotSpots(long relx,long rely)
{
	if(GetFlags() & C_BIT_INVISIBLE || !(GetFlags() & C_BIT_ENABLED) || !Ready())
		return(0);

	if(relx >= GetX() && rely >= GetY() && relx <= (GetX()+GetW()) && rely <= (GetY()+GetH()))
		return(GetID());
	return(0);
}

BOOL C_ServerItem::Process(long ID,short HitType)
{
	gSoundMgr->PlaySound(GetSound(HitType));
	if(HitType == C_TYPE_LMOUSEUP)
	{
		SetState(static_cast<short>(GetState() | 1));
		Refresh();
	}

	if(Callback_)
		(*Callback_)(ID,HitType,this);

	return(FALSE);
}

void C_ServerItem::Refresh()
{
	if(GetFlags() & C_BIT_INVISIBLE || Parent_ == NULL)
		return;

	Parent_->SetUpdateRect(GetX(),GetY(),GetX()+GetW(),GetY()+GetH(),GetFlags(),GetClient());
}

void C_ServerItem::Draw(SCREEN *surface,UI95_RECT *cliprect)
{
	if(GetFlags() & C_BIT_INVISIBLE || Parent_ == NULL || !Ready())
		return;

	short x=GetState();

	for(int i=0;i<5;i++)
	{
		if(m_arrOutput[i])
		{
			if(m_arrOutput[i])
			{
				m_arrOutput[i]->SetFgColor(Color_[x]);
				m_arrOutput[i]->Draw(surface,cliprect);
			}
		}
	}

	if(MouseOver_ || (GetFlags() & C_BIT_FORCEMOUSEOVER))
		HighLite(surface,cliprect);
}

inline int C_ServerItem::CompareText(C_ServerItem *pOther, int nIndex)
{
	return strcmp(m_arrOutput[nIndex]->GetText(), pOther->m_arrOutput[nIndex]->GetText());
}

inline int C_ServerItem::ComparePing(C_ServerItem *pOther)
{
	return m_nPing > pOther->m_nPing;
}

inline int C_ServerItem::ComparePlayers(C_ServerItem *pOther)
{
	return m_nPlayers > pOther->m_nPlayers;
}

void C_ServerItem::Save(char **)
{
}

void C_ServerItem::Save(FILE *)
{
}

void C_ServerItem::SetState(short state)
{
	State_=static_cast<short>(state & 3);
}

short C_ServerItem::GetState()
{
	return(State_);
}

void C_ServerItem::SetOwner(TREELIST *item)
{
	Owner_=item;
}

TREELIST *C_ServerItem::GetOwner()
{
	return(Owner_);
}

long C_ServerItem::GetFont()
{
	return(Font_);
}

void C_ServerItem::SetDefaultFlags()
{
	SetFlags(DefaultFlags_);
}

long C_ServerItem::GetDefaultFlags()
{
	return(DefaultFlags_);
}

DWORD C_ServerItem::GetIP()
{
	return m_dwIP;
}

/////////////////////////////////////////////////////////////////////////////
// CGNetUpdater

CGNetUpdater::CGNetUpdater()
{
	m_dwMasterServerCookie = m_dwMasterServerCookie2 = 0;
	m_bUpdating = false;
}

void CGNetUpdater::Init()
{
	CheckHR(m_pMasterServer.CreateInstance(__uuidof(SHAREDLib::RemoteMasterServer)));

	CheckHR(AtlAdvise(m_pMasterServer, (IGameEvents *) this, IID_IRemoteMasterServerEvents, &m_dwMasterServerCookie));
	CheckHR(AtlAdvise(m_pMasterServer, (IGameEvents *) this, IID_IGameEvents, &m_dwMasterServerCookie2));
}

void CGNetUpdater::Cleanup()
{
	if(m_pMasterServer)
	{
		if(m_dwMasterServerCookie)
		{
			AtlUnadvise(m_pMasterServer, IID_IRemoteMasterServerEvents, m_dwMasterServerCookie);
			m_dwMasterServerCookie = NULL;
		}

		if(m_dwMasterServerCookie2)
		{
			AtlUnadvise(m_pMasterServer, IID_IGameEvents, m_dwMasterServerCookie2);
			m_dwMasterServerCookie2 = NULL;
		}

		m_pMasterServer.Release();
	}
}

void CGNetUpdater::CancelUpdate()
{
	// Abort now
	if(m_pMasterServer != NULL)
	{
		m_pMasterServer->raw_CancelUpdate();	
		return;
	}
}

void CGNetUpdater::Update()
{
	try
	{
		if(m_bUpdating)
		{
			CancelUpdate();
			return;
		}

		m_nServerCount = 0;

		m_bUpdating = true;

		UpdateStatus("Contacting master server ...");

#ifdef _DEBUG
#if 1
		m_pMasterServer->PutServerName(g_strMasterServerName);
		m_pMasterServer->PutGameFilter("Falcon4");
#else
		m_pMasterServer->PutServerName("master.gamespy.com");
#endif
#else
		m_pMasterServer->PutServerName(g_strMasterServerName);
		m_pMasterServer->PutGameFilter("Falcon4");
#endif

		m_pMasterServer->PutPort(28900);
		m_pMasterServer->Update();
	}	

	catch(_com_error e)
	{
		MonoPrint("CGNetUpdater::UpdateGame - _com_error 0x%X", e.Error());
	}
}

void CGNetUpdater::UpdateGame(IGame *p, bool bSuccess)
{
	try
	{
		if(bSuccess)
		{
#ifndef _DEBUG
			BSTR _bstr;
			if(SUCCEEDED(p->get_Name(&_bstr) || _bstr_t(_bstr) == _bstr_t("Falcon4")))
#endif
			{
				p->AddRef();
				m_arrGames.push_back(p);

				if(FilterGame(p))
				{
					MakeServerItem(m_pListServers, p);

					m_pListServers->ReorderBranch(m_pListServers->GetRoot());
					m_pListServers->RecalcSize();
					m_pListServers->Parent_->RefreshClient(m_pListServers->GetClient());
				}
			}
		}

		m_nUpdatedServers++;
		UpdateServerStatus();
	}

	catch(_com_error e)
	{
		MonoPrint("CGNetUpdater::Refresh - _com_error 0x%X", e.Error());
	}
}

bool CGNetUpdater::IsUpdating()
{
	return m_bUpdating;
}

// IGameEvents
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CGNetUpdater::UpdateComplete(IGame *Game)
{
	UpdateGame(Game, true);
	return S_OK;
}

STDMETHODIMP CGNetUpdater::UpdateAborted(IGame *Game)
{
	UpdateGame(Game, false);
	return S_OK;
}

// IRemoteMasterServerEvents
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CGNetUpdater::UpdateComplete(BOOL bSuccess)
{
	m_bUpdating = false;

	::UpdateComplete(bSuccess);

	return S_OK;
}

STDMETHODIMP CGNetUpdater::ServerListReceived(int nServers)
{
	try
	{
		// Get server properties
		m_bstrName = m_pMasterServer->GetName();
		m_bstrLocation = m_pMasterServer->GetLocation();
		m_bstrVersion = m_pMasterServer->GetVersion();
		m_bstrMOTD = m_pMasterServer->GetMOTD();
		m_bstrAdminEmail = m_pMasterServer->GetAdminEmail();

		// CWindow(GetDlgItem(IDC_MOTD)).SetWindowText(m_bstrMOTD);
		m_nServerCount = nServers;

		::ServerListReceived(nServers);

		return S_OK;
	}

	catch(_com_error e)
	{
		return e.Error();
	}
}

