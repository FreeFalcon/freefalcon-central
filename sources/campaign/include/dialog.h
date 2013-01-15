
#ifndef DIALOG_H
#define DIALOG_H

extern HINSTANCE hInst;

// constants

#define FILE_LEN            80

// Function prototypes

// procs
extern BOOL CALLBACK About(HWND, UINT, WPARAM, LPARAM);

extern BOOL CALLBACK EditState(HWND, UINT, WPARAM, LPARAM);

extern BOOL CALLBACK EditObjective(HWND, UINT, WPARAM, LPARAM);

extern BOOL CALLBACK MissionTriggerProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);

extern BOOL CALLBACK WeatherEditProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);

extern BOOL CALLBACK EditRelations(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);

extern BOOL CALLBACK EditTeams(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);

extern BOOL CALLBACK MapDialogProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);  

extern BOOL CALLBACK AdjustForceRatioProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);

extern BOOL CALLBACK CampClipperProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam) ;

extern BOOL CALLBACK EnterNew(HWND, UINT, WPARAM, LPARAM);

extern BOOL CALLBACK FileOpenHookProc(HWND, UINT, WPARAM, LPARAM);

extern BOOL CALLBACK FileSaveHookProc(HWND, UINT, WPARAM, LPARAM);

extern BOOL OpenCampFile( HWND );

extern BOOL OpenTheaterFile( HWND );

extern BOOL SaveCampFile( HWND, int mode );

extern BOOL SaveTheaterFile( HWND );

extern BOOL SaveAsCampFile( HWND, int mode );

extern BOOL SaveAsTheaterFile( HWND );

extern BOOL SaveAsScriptedUnitFile (HWND hWnd);

extern int inButton(RECT *but, WORD xPos, WORD yPos);

extern void ProcessCDError(DWORD, HWND);

#endif