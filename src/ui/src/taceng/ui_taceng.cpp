///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Tactical Engagement - Robin Heydon
//
// Implements the user interface for the tactical engagement section
// of falcon 4.0
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "falclib.h"
#include "vu2.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "uicomms.h"
#include "userids.h"
#include "textids.h"
#include "tac_class.h"
#include "te_defs.h"
#include <windows.h>
#include <ddraw.h>
#include "unit.h"
#include "team.h"
#include "CmpGlobl.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "Listadt.h"
#include "objectiv.h"
#include "Campaign.h"
#include "campmap.h"
#include "campwp.h"
#include "campstr.h"
#include "squadron.h"
#include "feature.h"
#include "pilot.h"
#include "team.h"
#include "find.h"
#include "misseval.h"
#include "cmpclass.h"
#include "ui95_dd.h"
#include "AirUnit.h"
#include "uicomms.h"
#include "userids.h"
#include "classtbl.h"
#include "chandler.h"
#include "uicomms.h"
#include "userids.h"
#include "division.h"
#include "cmap.h"
void GenericTimerCB(long ID,short hittype,C_Base *control);
void BlinkCommsButtonTimerCB(long ID,short hittype,C_Base *control);
void tactical_select_load (long ID, short hittype, C_Base *ctrl);
void tactical_select_training (long ID, short hittype, C_Base *ctrl);
void SetSingle_Comms_Ctrls();

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// This is called when "Tactical Engagement" is clicked the first time around
//

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void LoadCommonWindows (void);
void CleanupTacticalEngagementUI (void);

extern int PlannerLoaded,TACSelLoaded;
void LoadPlannerWindows (void);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern C_Map
	*gMapMgr;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void LoadTacEngSelectWindows()
{
	C_Window *win;
	C_Button *tac_train = NULL;
	long ID;

	if(!PlannerLoaded)
		LoadPlannerWindows();

	LoadCommonWindows();

	if(!TACSelLoaded)
	{
		if( _LOAD_ART_RESOURCES_)
			gMainParser->LoadImageList ("ts_res.lst");
		else
			gMainParser->LoadImageList ("ts_art.lst");

		gMainParser->LoadSoundList ("ts_snd.lst");

		gMainParser->LoadWindowList ("ts_scf.lst");

		ID=gMainParser->GetFirstWindowLoaded ();

		while(ID)
		{
			hookup_tactical_controls (ID);
			ID = gMainParser->GetNextWindowLoaded ();
		}
		TACSelLoaded++;
	}

	win = gMainHandler->FindWindow (TAC_MISSION_WIN);

	if (win) // JPO
	    tac_train=(C_Button *)win->FindControl(TAC_TRAIN_CTRL);
	if(tac_train)
		tactical_select_training(TAC_TRAIN_CTRL,C_TYPE_LMOUSEUP,tac_train);

	SetSingle_Comms_Ctrls();
}

void LoadTacticalWindows (void)
{
	long ID;
	C_Window *win;
	C_TimerHook *tmr;

	if (TACLoaded)
	{
		return;
	}

	if( _LOAD_ART_RESOURCES_)
		gMainParser->LoadImageList ("te_res.lst");
	else
		gMainParser->LoadImageList ("te_art.lst");

	gMainParser->LoadSoundList ("te_snd.lst");

	gMainParser->LoadWindowList ("te_scf.lst");

	ID=gMainParser->GetFirstWindowLoaded ();

	while(ID)
	{
		hookup_tactical_controls (ID);
		ID = gMainParser->GetNextWindowLoaded ();
	}

	win=gMainHandler->FindWindow(TAC_PLAY_SCREEN);
	if(win)
	{
		tmr=new C_TimerHook;
		tmr->Setup(C_DONT_CARE,C_TYPE_TIMER);
		tmr->SetUpdateCallback(GenericTimerCB);
		tmr->SetRefreshCallback(BlinkCommsButtonTimerCB);
		tmr->SetUserNumber(_UI95_TIMER_DELAY_,1*_UI95_TICKS_PER_SECOND_); // Timer activates every 2 seconds (Only when this window is open)

		win->AddControl(tmr);
	}
	TACLoaded = TRUE;
}
