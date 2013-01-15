// Campaign UI Hacks

#include <windows.h>
#include "falclib.h"
#include "vu2.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "userids.h"
#include "Find.h"
#include "Flight.h"
#include "FalcSess.h"
#include "MissEval.h"
#include "Resource.h"
#include "Dispcfg.h"
#include "division.h"
#include "Cmap.h"
#include "Gps.h"
#include "Brief.h"

extern C_Handler *gMainHandler;
extern VU_ID gCurrentFlight; // ID of current flight in mission list
extern BOOL WINAPI FistOfGod(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
extern BOOL WINAPI CheatTool(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
extern void BuildCampDebrief (C_Window *win);
extern void DeleteGroupList(long ID);

//extern int FistOfGodActive;
extern HINSTANCE hInst;
extern GlobalPositioningSystem *gGps;

void CampHackButton1CB(long,short hittype,C_Base *)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	// Button 1 is Fist Of God tool
	DialogBox(hInst,MAKEINTRESOURCE(IDD_FISTOFGOD),FalconDisplay.appWin,(DLGPROC)FistOfGod);
}

void CampHackButton2CB(long,short hittype,C_Base *)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	// Button 1 is Cheat tool
	DialogBox(hInst,MAKEINTRESOURCE(IDD_PLAYERCHEAT),FalconDisplay.appWin,(DLGPROC)CheatTool);
}

void CampHackButton3CB(long,short hittype,C_Base *)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	if (gGps->TeamNo_ >= 0)
		gGps->SetTeamNo(-1);
	else
		gGps->SetTeamNo(FalconLocalSession->GetTeam());
}

// These functions are intended to be called by the Sim for the kneeboard data
int GetBriefingData (int query, int data, _TCHAR *buffer, int len);

void CampHackButton4CB(long,short hittype,C_Base *)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	_TCHAR	buffer[80];
	int		i=0,done=0;

	GetBriefingData (GBD_PLAYER_ELEMENT, 0, buffer, 80);
	GetBriefingData (GBD_PLAYER_TASK, 0, buffer, 80);
	GetBriefingData (GBD_PACKAGE_LABEL, 0, buffer, 80);
	GetBriefingData (GBD_PACKAGE_MISSION, 0, buffer, 80);
	while (!done)
		{
		if (GetBriefingData (GBD_PACKAGE_ELEMENT_NAME, i, buffer, 80) < 0)
			done = 1;
		if (GetBriefingData (GBD_PACKAGE_ELEMENT_TASK, i, buffer, 80) < 0)
			done = 1;
		i++;
		}
}

void CampHackButton5CB(long,short hittype,C_Base *)
{
	C_Window *win;

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	win=gMainHandler->FindWindow(DEBRIEF_WIN);
	// KCK: Added the check for a pilot list so that we don't debrief after a
	// discarded mission
	if(win && TheCampaign.MissionEvaluator && TheCampaign.MissionEvaluator->flight_data)
	{
//		TheCampaign.MissionEvaluator->PostMissionEval();
		BuildCampDebrief(win);
		gMainHandler->EnableWindowGroup(win->GetGroup());
	}
}
