// EventReaderDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EventReader.h"
#include "EventReaderDlg.h"

#include "mesgrc.h"
#include "mesg.h"
#include "vuevent.h"

char curFileName[_MAX_PATH] = {0};
EventElement *RootEvent = NULL;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEventReaderDlg dialog

CEventReaderDlg::CEventReaderDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEventReaderDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEventReaderDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CEventReaderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEventReaderDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CEventReaderDlg, CDialog)
	//{{AFX_MSG_MAP(CEventReaderDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_FILE, OnFile)
	ON_LBN_DBLCLK(IDC_EVENTLIST, OnDblclkEventlist)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEventReaderDlg message handlers

BOOL CEventReaderDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	CString strAboutMenu;
	strAboutMenu.LoadString(IDS_ABOUTBOX);
	if (!strAboutMenu.IsEmpty())
	{
		pSysMenu->AppendMenu(MF_SEPARATOR);
		pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
   ((CListBox*) GetDlgItem (IDC_EVENTLIST))->SetTabStops(75);
   ((CListBox*) GetDlgItem (IDC_EVENTDATA))->SetTabStops(120);
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CEventReaderDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CEventReaderDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CEventReaderDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CEventReaderDlg::OnFile() 
{
OPENFILENAME openData;

   openData.lStructSize = sizeof (OPENFILENAME);
   openData.hwndOwner = m_hWnd;
   openData.lpstrFilter = "Event Files\0*.evt\0All Files\0*.*\0\0";
   openData.lpstrCustomFilter = NULL;
   openData.nFilterIndex = 1;
   openData.lpstrFile = curFileName;
   openData.nMaxFile = 1024;
   openData.lpstrFileTitle = NULL;
   openData.lpstrInitialDir = NULL;
   openData.lpstrTitle = NULL;
   openData.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
   if (GetOpenFileName (&openData))
      ReadFile ();
}

void CEventReaderDlg::ReadFile (void)
{
FILE* inFile;
EventElement* tmpEvent;
EventElement* curEvent;
CListBox *theList;
char newString[1024];

   if (RootEvent)
      DisposeFile();

   inFile = fopen (curFileName, "rb");

   if (inFile)
   {
      ((CListBox*) GetDlgItem (IDC_EVENTLIST))->ResetContent();
      ((CListBox*) GetDlgItem (IDC_EVENTDATA))->ResetContent();
      theList = (CListBox*) GetDlgItem (IDC_EVENTLIST);
      curEvent = NULL;
      tmpEvent = new EventElement;
      while (fread(&(tmpEvent->idData), sizeof (EventIdData), 1, inFile) > 0)
      {
         tmpEvent->next = NULL;
         tmpEvent->eventData = new char[tmpEvent->idData.size];
         fread(tmpEvent->eventData, tmpEvent->idData.size, 1, inFile);
         tmpEvent->idData.type -= VU_LAST_EVENT + 1;
         sprintf (newString, "%.2f\t%s", (float)(*((double *)(tmpEvent->eventData))),
            TheEventStrings[FalconMsgIdStr[tmpEvent->idData.type]]);
         theList->InsertString (-1, newString);

         if (RootEvent == NULL)
            RootEvent = tmpEvent;
         else
            curEvent->next = tmpEvent;

         curEvent = tmpEvent;
         tmpEvent = new EventElement;
      }

      delete tmpEvent;
   }
}

void CEventReaderDlg::DisposeFile (void)
{
EventElement* tmpEvent;

   while (RootEvent)
   {
      tmpEvent = RootEvent;
      RootEvent = RootEvent->next;
      delete (tmpEvent->eventData);
      delete (tmpEvent);
   }

   ((CListBox*) GetDlgItem (IDC_EVENTLIST))->ResetContent();
   ((CListBox*) GetDlgItem (IDC_EVENTDATA))->ResetContent();
   SetDlgItemText (IDC_EVENTTYPE, "");
   SetDlgItemText (IDC_EVENTTIME, "");
}


void CEventReaderDlg::ParseEvent (EventElement* theEvent)
{
CListBox* theList;
char tmpStr[1024];
int i, day, hour, min;
double time;
char *tmp;

   theList = (CListBox*) GetDlgItem (IDC_EVENTDATA);
   theList->ResetContent();
   SetDlgItemText (IDC_EVENTTYPE, TheEventStrings[FalconMsgIdStr[theEvent->idData.type]]);
   time = *((double *)(theEvent->eventData));
   day = (int)(time / (24.0F * 3600.0F));
   time -= day * 24.0F * 3600.0F;
   hour = (int)(time / 3600.0F);
   time -= hour * 3600.0F;
   min = (int)(time / 60.0F);
   time -= min * 60.0F;
   sprintf (tmpStr, "Day %2d  %02d:%02d.%02.0f", day, hour, min, time);
   SetDlgItemText (IDC_EVENTTIME, tmpStr);

   tmp = theEvent->eventData;
   for (i=0; i<MsgNumElements[theEvent->idData.type]; i++)
   {
      sprintf (tmpStr, "   %s:\t", TheEventStrings[FalconMsgElementStr[theEvent->idData.type][i]]);
      if (FalconMsgElementTypes[theEvent->idData.type][i] >= 1001)
      {
      int offset, array;

         offset = *((int *)tmp);
         array  = FalconMsgElementTypes[theEvent->idData.type][i] - 1001;
         sprintf (tmpStr, "%s%s", tmpStr, TheEventStrings[FalconMsgEnumStr[array][offset]]);
         tmp += sizeof (int);
      }
      else
      {
         switch (FalconMsgElementTypes[theEvent->idData.type][i])
         {
            case 1:
               sprintf (tmpStr, "%s%d",tmpStr,  *((int *)tmp));
               tmp += sizeof (int);
            break;

            case 2:
               sprintf (tmpStr, "%s%f",tmpStr,  *((float *)tmp));
               tmp += sizeof (float);
            break;

            case 3:
               sprintf (tmpStr, "%s%d",tmpStr,  *((ushort *)tmp));
               tmp += sizeof (ushort);
            break;

            case 4:
               sprintf (tmpStr, "%s id %d", tmpStr, *((int *)tmp));
               tmp += sizeof (VU_ID);
            break;

            case 5:
               sprintf (tmpStr, "%s%d",tmpStr,  (int)(*((uchar *)tmp)));
               tmp += sizeof (uchar);
            break;

            case 6:
               sprintf (tmpStr, "%s%g",tmpStr,  *((double *)tmp));
               tmp += sizeof (double);
            break;

            case 7:
               sprintf (tmpStr, "%s%p",tmpStr,  ((void *)tmp));
               tmp += sizeof (ushort);
            break;

            default:
               sprintf (tmpStr, "%sHow Do I Show This???", tmpStr);
            break;
         }
      }
      theList->InsertString(-1, tmpStr);
   }
} 

void CEventReaderDlg::OnDblclkEventlist() 
{
CListBox *theList;
EventElement* tmpEvent;
int i, selEvent;

   theList = (CListBox*) GetDlgItem (IDC_EVENTLIST);
   if ((selEvent = theList->GetCurSel()) != LB_ERR)
   {
      tmpEvent = RootEvent;
      for (i=0; i<selEvent; i++)
         tmpEvent = tmpEvent->next;

      ParseEvent (tmpEvent);
   }
}

void CEventReaderDlg::OnCancel() 
{
   DisposeFile();	
	CDialog::OnCancel();
}

void CEventReaderDlg::OnOK() 
{
   DisposeFile();	
	CDialog::OnOK();
}
