#include <windows.h>
#include "f4vu.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "cphoneb.h"
#include "uicomms.h"
#include "queue.h"
#include "userids.h"
#include "textids.h"
#include "router.h"

using namespace std;

extern C_Handler *gMainHandler;

PhoneBook *gPlayerBook=NULL;

// sfr: temp global while UI does not have ports 
extern "C" unsigned short force_port = 0;

// function prototypes
void SetSingle_Comms_Ctrls();
void LeaveDogfight();
void CheckFlyButton();
void DeleteGroupList(long ID);
void CloseWindowCB(long ID,short hittype,C_Base *control);

// copies stuff from params into window's controls
// local port, remote host ip, remote port
static ComDataClass localData = { 0, 0, 0};

// URL line
static _TCHAR localDescription[MAX_URL_SIZE+1] = "server address";
// id of phonebook entry used 
static long localID=0;

// sfr: new window code
void CopyDataToWindow(){
	// get pbook window handlers
	C_Window *win = gMainHandler->FindWindow(PB_WIN);
	C_EditBox *hostAddressControl = (C_EditBox*)win->FindControl(IP_ADDRESS_1);
	C_Button *servButtonControl = (C_Button*)win->FindControl(COMM_MODE_SERV);
	C_Button *clientButtonControl = (C_Button*)win->FindControl(COMM_MODE_CLIENT);

	if ((win==NULL) || (hostAddressControl == NULL) || (servButtonControl == NULL)){
		return;
	}

	// enter critical session
	F4CSECTIONHANDLE* uics = UI_Enter(win);

	if (localData.ip_address == 0){
		// set is server and erase address
		servButtonControl->SetState(C_STATE_1);
		clientButtonControl->SetState(C_STATE_2);
	}
	else {
		// set is client and get address
		// set is server and erase address
		servButtonControl->SetState(C_STATE_2);
		clientButtonControl->SetState(C_STATE_1);
		hostAddressControl->SetText(localDescription);
	}

	win->RefreshWindow();

	// leave critical section
	UI_Leave(uics);
}

void CopyDataFromWindow(){
	// get pbook window handlers
	C_Window *win = gMainHandler->FindWindow(PB_WIN);
	C_EditBox *hostAddressControl = (C_EditBox*)win->FindControl(IP_ADDRESS_1);
	C_Button *servButtonControl = (C_Button*)win->FindControl(COMM_MODE_SERV);
	if ((win==NULL) || (hostAddressControl == NULL) || (servButtonControl == NULL)){
		return;
	}

	// enter critical session
	F4CSECTIONHANDLE* uics = UI_Enter(win);

	if (servButtonControl->GetState() == C_STATE_1){
		localData.ip_address = 0;		
	}
	else {
		_tcscpy(localDescription, hostAddressControl->GetText());
		// parse address:port
		string uiText(hostAddressControl->GetText());
		// look for a :
		bool foundColon = false;
		string::iterator it=uiText.begin(); // outside for cause well need this below
		for (;it!=uiText.end();++it){
			if (*it == ':'){ foundColon = true; break; }
		}
		if (foundColon){
			string addressText(uiText.begin(), it);
			string portText(++it, uiText.end());
			localData.ip_address = ComAPIGetIP(addressText.c_str());
			localData.remotePort = atoi(portText.c_str());
		}
		else {
			localData.ip_address = ComAPIGetIP(uiText.c_str());
			localData.remotePort = CAPI_UDP_PORT;
		}
	}

	// TODO read UI values
	if (force_port){
		localData.localPort = force_port;	
	}
	else {
		localData.localPort = CAPI_UDP_PORT;
	}

	// leave critical section
	UI_Leave(uics);
}

// called when the URL text area is selected
void AddressInputCB(long ID, short hittype, C_Base *){
	// set connect as client
	if ((hittype != 1)){
		return;
	}
	C_Window *win = NULL;
	C_Button *servButtonControl = NULL;
	C_Button *clientButtonControl = NULL;
	
	if ( 
		((win = gMainHandler->FindWindow(PB_WIN)) == NULL) || 
		((clientButtonControl = (C_Button*)win->FindControl(COMM_MODE_CLIENT)) == NULL) ||
		((servButtonControl = (C_Button*)win->FindControl(COMM_MODE_SERV)) == NULL)
	){
		return;
	}
	servButtonControl->SetState(C_STATE_2);
	servButtonControl->Refresh();
	clientButtonControl->SetState(C_STATE_1);
	clientButtonControl->Refresh();
}

// called when user clicks a saved address
void Phone_Select_CB(long ID,short hittype,C_Base *)
{
	PHONEBOOK *data;
	if(hittype != C_TYPE_LMOUSEUP){
		return;
	}

	data=gPlayerBook->FindID(ID);
	if(data){
		// copy phonebook to local variables
		localID=ID;
		_tcscpy(localDescription,data->url);
		localData.ip_address = 1; // just to select connect as client
		localData.localPort = data->localPort;
		localData.remotePort = data->remotePort;		
		// localvariables to window
		CopyDataToWindow();
	}
}

// called to show list of address in window
// each address is a button
void CopyPBToWindow(long ID,long Client)
{
	C_Window *win;
	C_Button *btn=NULL;
	PHONEBOOK *entry;
	F4CSECTIONHANDLE* Leave;
	int y=4;

	win=gMainHandler->FindWindow(ID);
	if(win)
	{
		Leave=UI_Enter(win);
		DeleteGroupList(ID);

		gPlayerBook->GetFirst();
		entry=gPlayerBook->GetCurrent();
		while(entry)
		{
			btn=new C_Button;
			btn->Setup(entry->ID,C_TYPE_RADIO,0,0);
			btn->SetXY(5,y);
			btn->SetHotSpot(0,0,win->ClientArea_[Client].right-win->ClientArea_[Client].left-10,gFontList->GetHeight(win->Font_));
			btn->SetText(C_STATE_0,entry->url);
			btn->SetText(C_STATE_1,entry->url);
			btn->SetColor(C_STATE_0,0x00dddddd);
			btn->SetColor(C_STATE_1,0x0000ff00);
			btn->SetFont(win->Font_);
			btn->SetGroup(100);
			btn->SetClient(static_cast<short>(Client));
			btn->SetCallback(Phone_Select_CB);
			btn->SetUserNumber(_UI95_DELGROUP_SLOT_,_UI95_DELGROUP_ID_);
			win->AddControl(btn);
			y+=gFontList->GetHeight(win->Font_)+2;
			entry = gPlayerBook->GetNext();
		}
		win->RefreshClient(Client);
		win->ScanClientAreas();
		UI_Leave(Leave);
	}
	if((localID == 0) && (btn == NULL))
	{
		//_tcscpy(localDescription,gStringMgr->GetString(TXT_DEFAULT));
		localID=1;
		//CopyDataToWindow(localDescription,&localData);
		CopyDataToWindow();
	}
	else if(btn)
	{
		btn->SetState(1);
		Phone_Select_CB(btn->GetID(),C_TYPE_LMOUSEUP,btn);
	}
}

// new is hit
void Phone_New_CB(long,short hittype,C_Base *)
{
	if(hittype != C_TYPE_LMOUSEUP){
		return;
	}
	// TODO i think this should be gone
}

// save is hit
void Phone_Apply_CB(long,short hittype,C_Base *)
{
	if(hittype != C_TYPE_LMOUSEUP){
		return;
	}

	//CopyDataFromWindow(localDescription,&localData);
	CopyDataFromWindow();

	// add new entry
	gPlayerBook->Add(localDescription, localData.localPort, localData.remotePort);
	CopyPBToWindow(PB_WIN,0);
}

void Phone_Remove_CB(long,short hittype,C_Base *)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	PHONEBOOK *apply = gPlayerBook->FindID(localID);
	if (apply != NULL){
		gPlayerBook->Remove(localID);
		CopyPBToWindow(PB_WIN,0);
	}
}

void Phone_Connect_CB(long n,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP){
		return;
	}

	if(gCommsMgr->Online()){
		return;
	}

	//CopyDataFromWindow(localDescription,&localData);
	CopyDataFromWindow();
	if(!gUICommsQ){
		CommsQueue *nq=new CommsQueue;
		if(nq){
			nq->Setup(gCommsMgr->AppWnd_);
			gUICommsQ=nq;
		}
	}
	gCommsMgr->StartComms(&localData);
	SetSingle_Comms_Ctrls();
	LeaveDogfight();
	CheckFlyButton();
	if(gCommsMgr->Online() && control->Parent_){
		gMainHandler->DisableWindowGroup(control->Parent_->GetGroup());
	}
}

void Phone_ConnectType_CB(long,short hittype,C_Base *control)
{
	int i;

	if(hittype != C_TYPE_LMOUSEUP)
		return;


	control->Parent_->UnHideCluster(control->GetUserNumber(0));
	i=1;
	while(control->GetUserNumber(i))
	{
		control->Parent_->HideCluster(control->GetUserNumber(i));
		i++;
	}
	control->Parent_->RefreshWindow();
}
