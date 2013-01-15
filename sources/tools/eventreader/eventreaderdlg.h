// EventReaderDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEventReaderDlg dialog
#include "falcmesg.h"

typedef struct eventlisttag
{
   EventIdData idData;
   char* eventData;
   eventlisttag *next;
} EventElement;

extern EventElement* RootEvent;

extern char curFileName[_MAX_PATH];

class CEventReaderDlg : public CDialog
{
// Construction
public:
	CEventReaderDlg(CWnd* pParent = NULL);	// standard constructor
   void ReadFile (void);
   void DisposeFile (void);
   void ParseEvent (EventElement* theEvent);

// Dialog Data
	//{{AFX_DATA(CEventReaderDlg)
	enum { IDD = IDD_EVENTREADER_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEventReaderDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CEventReaderDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnFile();
	afx_msg void OnDblclkEventlist();
	virtual void OnCancel();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
