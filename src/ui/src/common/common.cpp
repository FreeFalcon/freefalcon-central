/*

  UI Common... Common Files between Campaign & Tactical Engagement

*/

#include <windows.h>
#include "falclib.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "userids.h"
#include "textids.h"
#include "unit.h"
#include "Find.h"
#include "CmpClass.h"
#include "MissEval.h"
#include "Flight.h"
#include "campmiss.h"
#include "logbook.h"

extern C_Handler *gMainHandler;
extern C_Parser  *gMainParser;

C_SoundBite *gCampaignBites=NULL;

extern int CommonLoaded;
void CloseWindowCB(long,short,C_Base*);
void UI_Help_Guide_CB(long ID,short hittype,C_Base *ctrl);

void Cancel_Scramble_CB(long ID,short hittype,C_Base *control);
void Scramble_Intercept_CB(long ID,short hittype,C_Base *control);
void RedrawTreeWindowCB(long ID,short hittype,C_Base *control);
void Open_Flight_WindowCB(long ID,short hittype,C_Base *control);
void DeleteFlightFromPackage(long ID,short hittype,C_Base *control);
void tactical_cancel_package (long ID, short hittype, C_Base *ctrl);
void KeepPackage(long ID,short hittype,C_Base *control);
void EditFlightInPackage(long ID,short hittype,C_Base *control);

enum{
	CDECAFCE	= 400116,
	CDECAFLE	= 400117,
	CDECAME		= 400118,
	CDECDFCE	= 400119,
	CDECKFME	= 400120,
	CDECSSE		= 400121,
	CMCLE		= 400122,
	CMEE		= 400123,
	CMFF1E		= 400124,
	CMFF2E		= 400125,
	CMFF3E		= 400126,
	SND_GAVEL	= 500041,
	SND_MEDAL_FANFARE	= 500042,
	SND_PROMO_FANFARE	= 500043,
};
int lastSound = 0;
void AwardWindow(void);
void CourtMartialWindow(void);
void PromotionWindow(void);

void KIA_Abandon_CB(long,short hittype,C_Base *)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;
}

void Fly_Now_CB(long,short hittype,C_Base *)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;
}

void Dont_Fly_CB(long,short hittype,C_Base *)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;
}
void Truce_Accept_CB(long,short hittype,C_Base *)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;
}

void Truce_Fight_CB(long,short hittype,C_Base *)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;
}

void CloseResultsWindow(long ID,short hittype,C_Base *control)
{
	CloseWindowCB(ID,hittype,control);
	gSoundMgr->StopSound(SND_PROMO_FANFARE);
	gSoundMgr->StopSound(SND_GAVEL);
	gSoundMgr->StopSound(SND_MEDAL_FANFARE);
	gSoundMgr->StopSound(lastSound);
	F4HearVoices();
}

void CheckPromotion(long ID,short hittype,C_Base *control)
{
	CloseResultsWindow(ID,hittype,control);	

	PromotionWindow();
}

void AwardWindow(void)
{
	C_Window *win;
	C_Button *btn;

	if(!(MissionResult & AWARD_MEDAL))
	{
		PromotionWindow();
		return;
	}

	F4SilenceVoices();

	win=gMainHandler->FindWindow(AWARD_WIN);
	if(win)
	{
		gSoundMgr->PlaySound(SND_MEDAL_FANFARE);

		btn=(C_Button *)win->FindControl(MEDALS);
		if(btn)
		{
			if(MissionResult & MDL_AFCROSS)
			{
				btn->SetState(AIR_FORCE_CROSS);
				gSoundMgr->PlaySound(CDECAFCE);
				lastSound = CDECAFCE;
			}
			else if(MissionResult & MDL_SILVERSTAR)
			{
				btn->SetState(SILVER_STAR);
				gSoundMgr->PlaySound(CDECSSE);
				lastSound = CDECSSE;
			}
			else if(MissionResult & MDL_DIST_FLY)
			{
				btn->SetState(DIST_FLY_CROSS);
				gSoundMgr->PlaySound(CDECDFCE);
				lastSound = CDECDFCE;
			}
			else if(MissionResult & MDL_AIR_MDL)
			{
				btn->SetState(AIR_MEDAL);
				gSoundMgr->PlaySound(CDECAME);
				lastSound = CDECAME;
			}
			else if(MissionResult & MDL_KOR_CAMP)
			{
				btn->SetState(KOREA_CAMPAIGN);
				gSoundMgr->PlaySound(CDECKFME);
				lastSound = CDECKFME;
			}
			else if(MissionResult & MDL_LONGEVITY)
			{
				btn->SetState(LONGEVITY);
				gSoundMgr->PlaySound(CDECAFLE);
				lastSound = CDECAFLE;
			}
			btn->Refresh();

		}
		
		btn=(C_Button *)win->FindControl(CLOSE_WINDOW);
		if(btn)
		{
			btn->SetCallback(CheckPromotion);
		}
		gMainHandler->ShowWindow(win);
		gMainHandler->WindowToFront(win);
	}
}

void DoResultsWindows(void)
{
	if(MissionResult & COURT_MARTIAL)
		CourtMartialWindow();
	else
		AwardWindow();//this calls promotion window too	
}

void PromotionWindow(void)
{
	C_Window *win;
	C_Button *btn;

	if(!(MissionResult & PROMOTION))
	{
		MissionResult = 0;
		return;
	}

	F4SilenceVoices();

	win=gMainHandler->FindWindow(PROMO_WIN);
	if(win)
	{
		gSoundMgr->PlaySound(SND_PROMO_FANFARE);

		btn=(C_Button *)win->FindControl(RANKS);
		if(btn)
		{
			btn->SetState(static_cast<short>(LogBook.Rank()));
			btn->Refresh();
		}
		
		btn=(C_Button *)win->FindControl(CLOSE_WINDOW);
		if(btn)
		{
			btn->SetCallback(CloseResultsWindow);
		}
		gMainHandler->ShowWindow(win);
		gMainHandler->WindowToFront(win);
	}
	MissionResult = 0;
}

void CourtMartialWindow(void)
{
	C_Window *win;
	C_ListBox *lbox;
	C_Button *btn;

	F4SilenceVoices();

	win=gMainHandler->FindWindow(COURT_WIN);
	if(win)
	{
		gSoundMgr->PlaySound(SND_GAVEL);

		lbox=(C_ListBox *)win->FindControl(OFFENSE);
		if(lbox)
		{
			if(MissionResult & CM_FR_FIRE1)
			{
				lbox->SetValue(FRIENDLY_FIRE1);
				gSoundMgr->PlaySound(CMFF1E);
				lastSound = CMFF1E;
			}
			else if(MissionResult & CM_FR_FIRE2)
			{
				lbox->SetValue(FRIENDLY_FIRE2);
				gSoundMgr->PlaySound(CMFF2E);
				lastSound = CMFF2E;
			}
			else if(MissionResult & CM_FR_FIRE3)
			{
				lbox->SetValue(FRIENDLY_FIRE3);
				gSoundMgr->PlaySound(CMFF3E);
				lastSound = CMFF3E;
			}
			else if(MissionResult & CM_CRASH)
			{
				lbox->SetValue(CRASH_LANDING);
				gSoundMgr->PlaySound(CMCLE);
				lastSound = CMCLE;
			}
			else if(MissionResult & CM_EJECT)
			{
				lbox->SetValue(EJECT);
				gSoundMgr->PlaySound(CMEE);
				lastSound = CMEE;
			}

			lbox->Refresh();
		}
		
		btn=(C_Button *)win->FindControl(CLOSE_WINDOW);
		if(btn)
		{
			btn->SetCallback(CloseResultsWindow);
		}
		gMainHandler->ShowWindow(win);
		gMainHandler->WindowToFront(win);
	}
	MissionResult = 0;
}


void HookupCommonControls(long ID)
{
	C_Button *btn;
	C_Window *win;

	win=gMainHandler->FindWindow(ID);
	if(win)
	{
		// CloseWindow
		btn=(C_Button *)win->FindControl(CLOSE_WINDOW);
		if(btn)
			btn->SetCallback(CloseWindowCB);

		// KIA Abandon Campaign
		btn=(C_Button *)win->FindControl(ABANDON);
		if(btn)
			btn->SetCallback(KIA_Abandon_CB);

		// KIA Continue Campaign
		btn=(C_Button *)win->FindControl(CONTINUE);
		if(btn)
			btn->SetCallback(CloseWindowCB);

		// Scramble... Don't Intercept
		btn=(C_Button *)win->FindControl(CANCEL);
		if(btn)
			btn->SetCallback(Cancel_Scramble_CB);			// KCK: This callback is in campaign.cpp

		// Scramble... Intercept incoming bogeys
		btn=(C_Button *)win->FindControl(INTERCEPT);
		if(btn)
			btn->SetCallback(Scramble_Intercept_CB);		// KCK: This callback is in campaign.cpp

		// TOTIME... Wait until mission is ready
		btn=(C_Button *)win->FindControl(WAIT);
		if(btn)
			btn->SetCallback(CloseWindowCB);

		// TOTIME... Get in Aircraft & go
		btn=(C_Button *)win->FindControl(FLY);
		if(btn)
			btn->SetCallback(Fly_Now_CB);

		// TOWAIT... Don't Get in Aircraft
		btn=(C_Button *)win->FindControl(WAIT);
		if(btn)
			btn->SetCallback(Dont_Fly_CB);

		// TRUCE... Accept truce
		btn=(C_Button *)win->FindControl(ACCEPT_TRUCE);
		if(btn)
			btn->SetCallback(Truce_Accept_CB);

		// TRUCE... Fight on
		btn=(C_Button *)win->FindControl(FIGHT);
		if(btn)
			btn->SetCallback(Truce_Fight_CB);
	// Help GUIDE thing
		btn=(C_Button*)win->FindControl(UI_HELP_GUIDE);
		if(btn)
			btn->SetCallback(UI_Help_Guide_CB);

		btn = (C_Button *) win->FindControl (ADD_PACKAGE_FLIGHT);
		if (btn)
		{
			btn->SetCallback (Open_Flight_WindowCB);
		}
		btn = (C_Button *) win->FindControl (EDIT_PACKAGE_FLIGHT);
		if (btn)
		{
			btn->SetCallback (EditFlightInPackage);
		}
		btn = (C_Button *) win->FindControl (DELETE_PACKAGE_FLIGHT);
		if (btn)
		{
			btn->SetCallback (DeleteFlightFromPackage);
		}
		btn = (C_Button *) win->FindControl (CANCEL_PACK);
		if (btn)
		{
			btn->SetCallback (tactical_cancel_package);
		}
		btn = (C_Button *) win->FindControl (OK_PACK);
		if (btn)
		{
			btn->SetCallback (KeepPackage);
		}
	}
}

void LoadCommonWindows()
{
	long ID;
	if(CommonLoaded) return;

	if( _LOAD_ART_RESOURCES_)
		gMainParser->LoadImageList("cmn_res.lst");
	else
		gMainParser->LoadImageList("cmn_art.lst");
	gMainParser->LoadSoundList("cmn_snd.lst");

	if(!gCampaignBites)
		gCampaignBites=gMainParser->ParseSoundBite("art\\common\\uidcp.scf");

	gMainParser->LoadWindowList("cmn_scf.lst");	// Modified by M.N. - add art/art1024 by LoadWindowList

	ID=gMainParser->GetFirstWindowLoaded();
	while(ID)
	{
		HookupCommonControls(ID);
		ID=gMainParser->GetNextWindowLoaded();
	}
	CommonLoaded++;
}

