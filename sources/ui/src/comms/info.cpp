/****************************************************
*
*	Restrictions Information Window
*	David Power (x4373)
*	2/2/98
*
*****************************************************/
#include <windows.h>
#include "f4version.h"
#include "targa.h"
#include "PlayerOp.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "falcsess.h"
#include "falclib\include\f4find.h"
#include "userids.h"
#include "iconids.h"
#include "textids.h"
#include "Dispcfg.h"
#include "logbook.h"
#include "rules.h"
#include "uicomms.h"

extern C_Handler *gMainHandler;

void CloseWindowCB(long ID,short hittype,C_Base *control);
static void INFOSaveValues(void);
static void INFOSaveRules(void);
void CheckCompliance(void);
void AreYouSure(long TitleID,long MessageID,void (*OkCB)(long,short,C_Base*),void (*CancelCB)(long,short,C_Base*));
void AreYouSure(long TitleID,_TCHAR *text,void (*OkCB)(long,short,C_Base*),void (*CancelCB)(long,short,C_Base*));

extern int INFOLoaded;
RulesClass CurrRules;
void (*OkCB)() = NULL;
void (*CancelCB)() = NULL;
int modify = 0;
_TCHAR GameName[19] = "Uninitialized";

void INFOSetupRulesControls(void)
{
	C_Window	*win;
	C_Button	*button;
	C_ListBox	*lbox;
	C_Slider	*slider;
	C_EditBox	*ebox;

	win=gMainHandler->FindWindow(INFO_WIN);
	if(win == NULL)
		return;

	int host = modify;
	//if(FalconLocalGame == vuPlayerPoolGroup)
	//if(vuLocalSessionEntity->Game())
		//host = TRUE;
	//if this is true, we are the host
	/*
	if( FalconLocalGameEntity && FalconLocalSession &&
		FalconLocalGameEntity->OwnerId() == FalconLocalSession->Id() )
			host = TRUE;
	else if(!gCommsMgr->GetTargetGame() && !FalconLocalGameEntity)
		host = TRUE;

	if(gCommsMgr->GetSettings()->Rules.GameStatus != GAME_WAITING)
		host = FALSE;*/

	ebox = (C_EditBox *)win->FindControl(INFO_GAMENAME);
	if(ebox)
	{
		if(host)
			ebox->SetFlagBitOn(C_BIT_ENABLED);
		else
			ebox->SetFlagBitOff(C_BIT_ENABLED);

//		if(FalconLocalGame != vuPlayerPoolGroup)
//			ebox->SetText(FalconLocalGameEntity->GameName());
//		else 
		if(gCommsMgr->GetTargetGame())
			ebox->SetText( gCommsMgr->GetTargetGame()->GameName() );
		else
		{
			ebox->SetText(GameName); 
		}
		
		ebox->Refresh();
	}

	ebox = (C_EditBox *)win->FindControl(MAX_PLAYERS);
	if(ebox)
	{
		if(host)
			ebox->SetFlagBitOn(C_BIT_ENABLED);
		else
			ebox->SetFlagBitOff(C_BIT_ENABLED);

		ebox->SetInteger(CurrRules.MaxPlayers);
		ebox->Refresh();
	}

	lbox=(C_ListBox *)win->FindControl(FLTMOD_REQ);
	if(lbox != NULL)
	{
		if(host)
			lbox->SetFlagBitOn(C_BIT_ENABLED);
		else
			lbox->SetFlagBitOff(C_BIT_ENABLED);

		if(CurrRules.GetFlightModelType()==FMAccurate)
			lbox->SetValue(FLTMOD_1);
		else
			lbox->SetValue(FLTMOD_2);

		lbox->Refresh();
	}

	lbox=(C_ListBox *)win->FindControl(RADAR_REQ);
	if(lbox != NULL)
	{
		if(host)
			lbox->SetFlagBitOn(C_BIT_ENABLED);
		else
			lbox->SetFlagBitOff(C_BIT_ENABLED);

		switch(CurrRules.GetAvionicsType())
		{
			// M.N. full realism mode added
		case ATRealisticAV:
			lbox->SetValue(RADAR_0);
			break;
		case ATRealistic:
			lbox->SetValue(RADAR_1);
			break;
		case ATSimplified:
			lbox->SetValue(RADAR_2);
			break;
		case ATEasy:
			lbox->SetValue(RADAR_3);
			break;
		}
		lbox->Refresh();
	}

	lbox=(C_ListBox *)win->FindControl(WEAPEFF_REQ);
	if(lbox != NULL)
	{
		if(host)
			lbox->SetFlagBitOn(C_BIT_ENABLED);
		else
			lbox->SetFlagBitOff(C_BIT_ENABLED);

		switch(CurrRules.GetWeaponEffectiveness())
		{
		case WEAccurate:
			lbox->SetValue(WEAPEFF_1);
			break;
		case WEEnhanced:
			lbox->SetValue(WEAPEFF_2);
			break;
		case WEExaggerated:
			lbox->SetValue(WEAPEFF_3);
			break;
		}
		lbox->Refresh();
	}

	lbox=(C_ListBox *)win->FindControl(AUTOPILOT_REQ);
	if(lbox != NULL)
	{
		if(host)
			lbox->SetFlagBitOn(C_BIT_ENABLED);
		else
			lbox->SetFlagBitOff(C_BIT_ENABLED);

		switch(CurrRules.GetAutopilotMode())
		{
		case APNormal:
			lbox->SetValue(AUTO_1);
			break;
		case APEnhanced:
			lbox->SetValue(AUTO_2);
			break;
		case APIntelligent:
			lbox->SetValue(AUTO_3);
			break;
		}
		lbox->Refresh();
	}

	lbox=(C_ListBox *)win->FindControl(REFUELING_REQ);
	if(lbox != NULL)
	{
		if(host)
			lbox->SetFlagBitOn(C_BIT_ENABLED);
		else
			lbox->SetFlagBitOff(C_BIT_ENABLED);

		switch(CurrRules.GetRefuelingMode())
		{
		case ARRealistic:
			lbox->SetValue(REFUEL_1);
			break;
		case ARModerated:
			lbox->SetValue(REFUEL_2);
			break;
		case ARSimplistic:
			lbox->SetValue(REFUEL_3);
			break;
		}
		lbox->Refresh();
	}

	lbox=(C_ListBox *)win->FindControl(PADLOCK_REQ);
	if(lbox != NULL)
	{
		if(host)
			lbox->SetFlagBitOn(C_BIT_ENABLED);
		else
			lbox->SetFlagBitOff(C_BIT_ENABLED);

		switch(CurrRules.GetPadlockMode())
		{
		case PDDisabled:
			lbox->SetValue(PADLOCK_4);
			break;
		case PDRealistic:
			lbox->SetValue(PADLOCK_1);
			break;
		case PDEnhanced:
			lbox->SetValue(PADLOCK_2);
			break;
		//case PDSuper:
		//	lbox->SetValue(PADLOCK_3);
		//	break;
		}
		lbox->Refresh();
	}

	button=(C_Button *)win->FindControl(FUEL_REQ);
	if(button != NULL)
	{
		if(CurrRules.UnlimitedFuel())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}

	button=(C_Button *)win->FindControl(CHAFFLARES_REQ);
	if(button != NULL)
	{
		if(CurrRules.UnlimitedChaff())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
		
	}

	button=(C_Button *)win->FindControl(COLLISIONS_REQ);
	if(button != NULL)
	{
		if(CurrRules.CollisionsOn())
			button->SetState(C_STATE_0);
		else
			button->SetState(C_STATE_1);
		button->Refresh();
	}

	button=(C_Button *)win->FindControl(BLACKOUT_REQ);
	if(button != NULL)
	{
		if(CurrRules.BlackoutOn())
			button->SetState(C_STATE_0);
		else
			button->SetState(C_STATE_1);
		button->Refresh();
	}


	button=(C_Button *)win->FindControl(IDTAGS_REQ);
	if(button != NULL)
	{
		if(CurrRules.NameTagsOn())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}

	button=(C_Button *)win->FindControl(WEATHER_REQ);
	if(button != NULL)
	{
		if(CurrRules.WeatherOn())
			button->SetState(C_STATE_0);
		else
			button->SetState(C_STATE_1);
		button->Refresh();
	}

	button=(C_Button *)win->FindControl(INVULNERABILITY_REQ); 
	if(button != NULL)
	{
		if(CurrRules.InvulnerableOn())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}

	button=(C_Button *)win->FindControl(EXT_VIEWS_REQ); 
	if(button != NULL)
	{			
		if(CurrRules.ExternalViewOn())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}

	slider=(C_Slider *)win->FindControl(VEHICLE_SIZE_REQ);
	if(slider != NULL)
	{	
		if(host)
			slider->SetFlagBitOn(C_BIT_ENABLED);
		else
			slider->SetFlagBitOff(C_BIT_ENABLED);

		slider->SetSliderPos(static_cast<long>((slider->GetSliderMax()-slider->GetSliderMin())*((CurrRules.ObjMagnification) - 1)/4.0f));
		ebox = (C_EditBox *)win->FindControl(VEHICLE_SIZE_READOUT_REQ);
		if(ebox)
		{
			ebox->SetInteger(static_cast<long>(CurrRules.ObjMagnification));
			ebox->Refresh();
			slider->SetUserNumber(0,VEHICLE_SIZE_READOUT_REQ);
		}
	}
	win->RefreshWindow();
}
///this function sets up all the controls according to the values stored
///in the PlayerOptions structure
void INFOSetupControls(void)
{
	C_Window	*win;
	C_Button	*button;
	C_ListBox	*lbox;
	C_Slider	*slider;
	C_EditBox	*ebox;
	
	
	win=gMainHandler->FindWindow(INFO_WIN);
	if(win == NULL)
		return;
	
	button=(C_Button *)win->FindControl(INFO_COMPLY);
	if(button != NULL)
	{
		if(PlayerOptions.InCompliance(CurrRules.GetRules()) )
		{
			button->SetLabel(0,TXT_OK);
		}
		else
		{
			button->SetLabel(0,TXT_COMPLY);
		}
		button->Refresh();
	}

	lbox=(C_ListBox *)win->FindControl(FLTMOD_CUR);
	if(lbox != NULL)
	{
		if(PlayerOptions.GetFlightModelType()==FMAccurate)
			lbox->SetValue(FLTMOD_1);
		else
			lbox->SetValue(FLTMOD_2);

		lbox->Refresh();
	}

	lbox=(C_ListBox *)win->FindControl(RADAR_CUR);
	if(lbox != NULL)
	{
		switch(PlayerOptions.GetAvionicsType())
		{
			// M.N. full realism mode added
		case ATRealisticAV:
			lbox->SetValue(RADAR_0);
			break;
		case ATRealistic:
			lbox->SetValue(RADAR_1);
			break;
		case ATSimplified:
			lbox->SetValue(RADAR_2);
			break;
		case ATEasy:
			lbox->SetValue(RADAR_3);
			break;
		}
		lbox->Refresh();
	}

	lbox=(C_ListBox *)win->FindControl(WEAPEFF_CUR);
	if(lbox != NULL)
	{
		switch(PlayerOptions.GetWeaponEffectiveness())
		{
		case WEAccurate:
			lbox->SetValue(WEAPEFF_1);
			break;
		case WEEnhanced:
			lbox->SetValue(WEAPEFF_2);
			break;
		case WEExaggerated:
			lbox->SetValue(WEAPEFF_3);
			break;
		}
		lbox->Refresh();
	}

	lbox=(C_ListBox *)win->FindControl(AUTOPILOT_CUR);
	if(lbox != NULL)
	{
		switch(PlayerOptions.GetAutopilotMode())
		{
		case APNormal:
			lbox->SetValue(AUTO_1);
			break;
		case APEnhanced:
			lbox->SetValue(AUTO_2);
			break;
		case APIntelligent:
			lbox->SetValue(AUTO_3);
			break;
		}
		lbox->Refresh();
	}

	lbox=(C_ListBox *)win->FindControl(REFUELING_CUR);
	if(lbox != NULL)
	{
		switch(PlayerOptions.GetRefuelingMode())
		{
		case ARRealistic:
			lbox->SetValue(REFUEL_1);
			break;
		case ARModerated:
			lbox->SetValue(REFUEL_2);
			break;
		case ARSimplistic:
			lbox->SetValue(REFUEL_3);
			break;
		}
		lbox->Refresh();
	}

	lbox=(C_ListBox *)win->FindControl(PADLOCK_CUR);
	if(lbox != NULL)
	{
		switch(PlayerOptions.GetPadlockMode())
		{
		case PDDisabled:
			lbox->SetValue(PADLOCK_4);
			break;
		case PDRealistic:
			lbox->SetValue(PADLOCK_1);
			break;
		case PDEnhanced:
			lbox->SetValue(PADLOCK_2);
			break;
		//case PDSuper:
		//	lbox->SetValue(PADLOCK_3);
		//	break;
		}
		lbox->Refresh();
	}
	
	button=(C_Button *)win->FindControl(FUEL_CUR);
	if(button != NULL)
	{
		if(PlayerOptions.UnlimitedFuel())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}

	button=(C_Button *)win->FindControl(CHAFFLARES_CUR);
	if(button != NULL)
	{
		if(PlayerOptions.UnlimitedChaff())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
		
	}

	button=(C_Button *)win->FindControl(COLLISIONS_CUR);
	if(button != NULL)
	{
		if(PlayerOptions.CollisionsOn())
			button->SetState(C_STATE_0);
		else
			button->SetState(C_STATE_1);
		button->Refresh();
	}

	button=(C_Button *)win->FindControl(BLACKOUT_CUR);
	if(button != NULL)
	{
		if(PlayerOptions.BlackoutOn())
			button->SetState(C_STATE_0);
		else
			button->SetState(C_STATE_1);
		button->Refresh();
	}
	
	button=(C_Button *)win->FindControl(IDTAGS_CUR);
	if(button != NULL)
	{
		if(PlayerOptions.NameTagsOn())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}

	button=(C_Button *)win->FindControl(WEATHER_CUR);
	if(button != NULL)
	{
		if(PlayerOptions.WeatherOn())
			button->SetState(C_STATE_0);
		else
			button->SetState(C_STATE_1);
		button->Refresh();
	}

	button=(C_Button *)win->FindControl(INVULNERABILITY_CUR); 
	if(button != NULL)
	{
		if(PlayerOptions.InvulnerableOn())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}

	slider=(C_Slider *)win->FindControl(VEHICLE_SIZE_CUR);
	if(slider != NULL)
	{
		slider->SetSliderPos(static_cast<long>((slider->GetSliderMax()-slider->GetSliderMin())*((PlayerOptions.ObjMagnification) - 1)/4));
		ebox = (C_EditBox *)win->FindControl(VEHICLE_SIZE_READOUT_CUR);
		if(ebox)
		{
			ebox->SetInteger(static_cast<short>(PlayerOptions.ObjMagnification));
			ebox->Refresh();
			slider->SetUserNumber(0,VEHICLE_SIZE_READOUT_CUR);
		}
	}

	INFOSetupRulesControls();
	CheckCompliance();
}

void UpdateRules(void)
{
	if (vuPlayerPoolGroup != vuLocalGame)
		CurrRules.LoadRules(FalconLocalGame->rules.GetRules());
	if (!gMainHandler)
		return;

	INFOSetupRulesControls();

	C_Window *win;
	C_Button *button;

	win=gMainHandler->FindWindow(INFO_WIN);
	if(win == NULL)
		return;
	
	button=(C_Button *)win->FindControl(INFO_COMPLY);
	if(button != NULL)
	{
		if(PlayerOptions.InCompliance(CurrRules.GetRules()) )
		{
			button->SetLabel(0,TXT_OK);
		}
		else
		{
			button->SetLabel(0,TXT_COMPLY);
		}
		button->Refresh();
	}
}

void ComplyCB(long ID,short hittype,C_Base *control)
{
	FalconGameEntity *game;
	C_EditBox *ebox;
	_TCHAR name[30];
	_TCHAR *head,*tail;

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	game=(FalconGameEntity*)gCommsMgr->GetTargetGame();

	ebox = (C_EditBox *)control->Parent_->FindControl(INFO_GAMENAME);
	if(ebox)
	{
		if(ebox->GetText())
			_tcscpy(name,ebox->GetText());
		else
			_tcscpy(name,GameName);

		head=name;

		while(*head && !_istalnum(*head))
			head++;

		tail=head;
		while(*tail)
			tail++;

		while(tail != head && !_istalnum(*tail))
			tail--;

		tail++;
		*tail=0;

		if((tail-head) < (3*sizeof(_TCHAR)) || !(*head))
		{
			AreYouSure(TXT_ERROR,TXT_INVALID_GAMENAME,CloseWindowCB,CloseWindowCB);
			return;
		}

		ebox->Refresh();
		ebox->SetText(head);
		ebox->Refresh();
	}

	INFOSaveValues();/*
	if( PlayerOptions.InCompliance(CurrRules.GetRules()) )
	{
		PlayerOptions.SaveOptions();
		CloseWindowCB(ID,hittype,control);
	}
	else
	{
		PlayerOptions.ComplyWRules(CurrRules.GetRules());
		PlayerOptions.SaveOptions();
		INFOSetupControls();
	}*/
	ebox=(C_EditBox*)control->Parent_->FindControl(INFO_PASSWORD);
	if(ebox)
	{
		game=(FalconGameEntity*)gCommsMgr->GetTargetGame();
		if(game && OkCB)
		{
			if(!game->CheckPassword(ebox->GetText()))
			{
				AreYouSure(TXT_ERROR,TXT_WRONGPASSWORD,NULL,CloseWindowCB);
				return;
			}
		}
	}

	PlayerOptions.ComplyWRules(CurrRules.GetRules());
	PlayerOptions.SaveOptions();

	CloseWindowCB(ID,hittype,control);
	if(OkCB)
		(*OkCB)();
}

int CheckButtonCompliance(C_Button *button,int test)
{
	if(button->GetState() == C_STATE_1 && test)
	{
		button->SetState(C_STATE_2);
		button->Refresh();
		return FALSE;
	}
	else if(button->GetState() == C_STATE_2 && !test)
	{
		button->SetState(C_STATE_1);
		button->Refresh();
		return TRUE;
	}
	else if(button->GetState() == C_STATE_2)
	{
		return FALSE;
	}
	return TRUE;
}

void CheckCompliance(void)
{
	C_Window *win = gMainHandler->FindWindow(INFO_WIN);
	if(!win)
		return;

	C_ListBox *lbox;
	C_Button *button;
	C_Slider *slider;
	C_EditBox *ebox;
	C_Line *line;

	int InCompliance = TRUE;
	int MakeRed = 0;

	lbox=(C_ListBox *)win->FindControl(FLTMOD_CUR);
	if(lbox != NULL)
	{
		if(lbox->GetTextID()==FLTMOD_2)
			if(CurrRules.SimFlightModel > FMSimplified)
				{
					InCompliance = FALSE;
					MakeRed++;
				}

		line = (C_Line *)win->FindControl(FLTMOD_LINE);
		if(MakeRed && line)
		{
			//lbox->SetNormColor(RGB(255,0,0));
			//lbox->Refresh();
			
			line->SetColor(RGB(230,0,0));
			line->Refresh();
			MakeRed--;
		}
		else if(line)
		{
			//lbox->SetNormColor(RGB(230,230,230));
			//lbox->Refresh();
			line->SetColor(RGB(65,128,173));
			line->Refresh();
		}
	}
	
	lbox=(C_ListBox *)win->FindControl(RADAR_CUR);
	if(lbox != NULL)
	{
		switch(lbox->GetTextID())
		{
		case RADAR_1:
			if(CurrRules.SimAvionicsType > ATRealistic)		// we've now a fourth setting
			{
				InCompliance = FALSE;
				MakeRed++;
			}
			break;
		case RADAR_2:
			if(CurrRules.SimAvionicsType > ATSimplified)
			{
				InCompliance = FALSE;
				MakeRed++;
			}
			break;
		case RADAR_3:
			if(CurrRules.SimAvionicsType > ATEasy)
			{
				InCompliance = FALSE;
				MakeRed++;
			}
			break;
		default:
			break;
		}

		line = (C_Line *)win->FindControl(RADAR_LINE);
		if(MakeRed && line)
		{
			//lbox->SetNormColor(RGB(255,0,0));
			//lbox->Refresh();
			
			line->SetColor(RGB(230,0,0));
			line->Refresh();
			MakeRed--;
		}
		else if(line)
		{
			//lbox->SetNormColor(RGB(230,230,230));
			//lbox->Refresh();
			line->SetColor(RGB(65,128,173));
			line->Refresh();
		}
	}
	
	lbox=(C_ListBox *)win->FindControl(WEAPEFF_CUR);
	if(lbox != NULL)
	{
		switch(lbox->GetTextID())
		{
		case WEAPEFF_2:
			if(CurrRules.SimWeaponEffect > WEEnhanced)
			{
				InCompliance = FALSE;
				MakeRed++;
			}
			break;
		case WEAPEFF_3:
			if(CurrRules.SimWeaponEffect > WEExaggerated)
			{
				InCompliance = FALSE;
				MakeRed++;
			}
			break;
		default:
			break;
		}

		line = (C_Line *)win->FindControl(WEAPEFF_LINE);
		if(MakeRed && line)
		{
			//lbox->SetNormColor(RGB(255,0,0));
			//lbox->Refresh();
			
			line->SetColor(RGB(230,0,0));
			line->Refresh();
			MakeRed--;
		}
		else if(line)
		{
			//lbox->SetNormColor(RGB(230,230,230));
			//lbox->Refresh();
			line->SetColor(RGB(65,128,173));
			line->Refresh();
		}
	}
	
	lbox=(C_ListBox *)win->FindControl(AUTOPILOT_CUR);
	if(lbox != NULL)
	{
		switch(lbox->GetTextID())
		{
		case AUTO_2:
			if(CurrRules.SimAutopilotType > APEnhanced)
			{
				InCompliance = FALSE;
				MakeRed++;
			}
			break;
		case AUTO_3:
			if(CurrRules.SimAutopilotType > APIntelligent)
			{
				InCompliance = FALSE;
				MakeRed++;
			}
			break;
		default:
			break;
		}

		line = (C_Line *)win->FindControl(AUTOPILOT_LINE);
		if(MakeRed && line)
		{
			//lbox->SetNormColor(RGB(255,0,0));
			//lbox->Refresh();
			
			line->SetColor(RGB(230,0,0));
			line->Refresh();
			MakeRed--;
		}
		else if(line)
		{
			//lbox->SetNormColor(RGB(230,230,230));
			//lbox->Refresh();
			line->SetColor(RGB(65,128,173));
			line->Refresh();
		}
	}

	lbox=(C_ListBox *)win->FindControl(PADLOCK_CUR);
	if(lbox != NULL)
	{
		switch(lbox->GetTextID())
		{
		case PADLOCK_1:
			if(CurrRules.SimPadlockMode > PDRealistic)
			{
				InCompliance = FALSE;
				MakeRed++;
			}
			break;
		case PADLOCK_2:
			if(CurrRules.SimPadlockMode > PDEnhanced)
			{
				InCompliance = FALSE;
				MakeRed++;
			}
			break;
		case PADLOCK_4:
			if(CurrRules.SimPadlockMode > PDDisabled)
			{
				InCompliance = FALSE;
				MakeRed++;
			}
			break;
	/*	case PADLOCK_3:
			if(CurrRules.SimPadlockMode > PDSuper)
			{
				InCompliance = FALSE;
				MakeRed++;
			}
			break;*/
		default:
			break;
		}

		line = (C_Line *)win->FindControl(PADLOCK_LINE);
		if(MakeRed && line)
		{
			//lbox->SetNormColor(RGB(255,0,0));
			//lbox->Refresh();
			
			line->SetColor(RGB(230,0,0));
			line->Refresh();
			MakeRed--;
		}
		else if(line)
		{
			//lbox->SetNormColor(RGB(230,230,230));
			//lbox->Refresh();
			line->SetColor(RGB(65,128,173));
			line->Refresh();
		}
	}
	
	lbox=(C_ListBox *)win->FindControl(REFUELING_CUR);
	if(lbox != NULL)
	{
		switch(lbox->GetTextID())
		{
		case REFUEL_2:
			if(CurrRules.SimAirRefuelingMode < ARModerated)
			{
				InCompliance = FALSE;
				MakeRed++;
			}
			break;
		case REFUEL_3:
			if(CurrRules.SimAirRefuelingMode < ARSimplistic)
			{
				InCompliance = FALSE;
				MakeRed++;
			}
			break;
		default:
			break;
		}

		line = (C_Line *)win->FindControl(REFUELING_LINE);
		if(MakeRed && line)
		{
			//lbox->SetNormColor(RGB(255,0,0));
			//lbox->Refresh();
			
			line->SetColor(RGB(230,0,0));
			line->Refresh();
			MakeRed--;
		}
		else if(line)
		{
			//lbox->SetNormColor(RGB(230,230,230));
			//lbox->Refresh();
			line->SetColor(RGB(65,128,173));
			line->Refresh();
		}
	}
	
	button=(C_Button *)win->FindControl(FUEL_CUR);
	if(button != NULL)
	{
		if(!CheckButtonCompliance(button,!CurrRules.UnlimitedFuel()) )
			InCompliance = FALSE;
	}
	
	button=(C_Button *)win->FindControl(CHAFFLARES_CUR);
	if(button != NULL)
	{
		if(!CheckButtonCompliance(button,!CurrRules.UnlimitedChaff()) )
			InCompliance = FALSE;
	}
	
	button=(C_Button *)win->FindControl(COLLISIONS_CUR);
	if(button != NULL)
	{
		if(!CheckButtonCompliance(button,!CurrRules.NoCollisions()) )
			InCompliance = FALSE;
	}
	
	button=(C_Button *)win->FindControl(BLACKOUT_CUR);
	if(button != NULL)
	{
		if(!CheckButtonCompliance(button,!CurrRules.NoBlackout()) )
			InCompliance = FALSE;
	}
	
	
	button=(C_Button *)win->FindControl(IDTAGS_CUR);
	if(button != NULL)
	{
		if(!CheckButtonCompliance(button,!CurrRules.NameTagsOn()) )
			InCompliance = FALSE;
	}
	
	button=(C_Button *)win->FindControl(WEATHER_CUR);
	if(button != NULL)
	{
		if(!CheckButtonCompliance(button,CurrRules.WeatherOn()) )
			InCompliance = FALSE;
	}
	

	button=(C_Button *)win->FindControl(INVULNERABILITY_CUR); 
	if(button != NULL)
	{
		if(!CheckButtonCompliance(button,!CurrRules.InvulnerableOn()) )
			InCompliance = FALSE;
	}

	/* May need to hit this if veh mag is 1
	button=(C_Button *)win->FindControl(AUTO_SCALE);
	if(button != NULL)
	{
		if(!CheckButtonCompliance(button,!CurrRules.InvulnerableOn()) )
			InCompliance = FALSE;

		if(button->GetState() == C_STATE_1)
			PlayerOptions.SetObjFlag(DISP_OBJ_DYN_SCALING);
		else
			PlayerOptions.ClearObjFlag(DISP_OBJ_DYN_SCALING);
	}*/

	
	
	slider=(C_Slider *)win->FindControl(VEHICLE_SIZE_CUR);
	if(slider != NULL)
	{
		if(CurrRules.ObjMagnification < (int)( (float)slider->GetSliderPos()/( slider->GetSliderMax() - slider->GetSliderMin() ) * 4 + 1.5f ) )
		{
			InCompliance = FALSE;
			MakeRed++;
		}
		ebox = (C_EditBox *)win->FindControl(slider->GetUserNumber(0));
		if(ebox)
		{
			if(MakeRed)
			{
				ebox->SetFgColor(RGB(255,0,0));
				MakeRed--;
			}
			else
			{
				ebox->SetFgColor(RGB(0,255,0));
			}
			ebox->Refresh();
		}
	}

	button=(C_Button *)win->FindControl(INFO_COMPLY);
	if(button != NULL)
	{
		if(InCompliance)
		{
			button->SetLabel(0,TXT_OK);
		}
		else
		{
			button->SetLabel(0,TXT_COMPLY);
		}
		button->Refresh();
	}
}

void ListBoxChangeCB(long,short hittype,C_Base *)
{
	if(hittype != C_TYPE_SELECT)
		return;
	
	CheckCompliance();
}

void SliderChangeCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_MOUSEMOVE)
		return;

	C_Slider	*slider;
	int			scale;

	slider=(C_Slider *)control;
	scale = FloatToInt32((float)slider->GetSliderPos()/(slider->GetSliderMax()-slider->GetSliderMin()) * 4 + 1.5F);

	if(scale != slider->GetUserNumber(2))
	{	
		C_EditBox *ebox;
		ebox = (C_EditBox *)control->Parent_->FindControl(slider->GetUserNumber(0));
		if(ebox)
		{
			ebox->SetInteger(scale);
			ebox->Refresh();
		}
		CheckCompliance();
	}
	slider->SetUserNumber(2,scale);
}

void ButtonChangeCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	C_Button *btn;
	btn = (C_Button *)control;

	if(btn->GetState() != C_STATE_0)
	{
		btn->SetState(C_STATE_0);
		btn->Refresh();
	}
	else
	{
		btn->SetState(C_STATE_1);
		btn->Refresh();		// MN was missed 
	}

	CheckCompliance();
}

static void INFOSaveRulesToFile(void)
{
	if(modify)
		CurrRules.SaveRules(LogBook.Callsign());
}

static void SliderRuleControlCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_MOUSEMOVE)
		return;

	C_Slider	*slider;
	int			scale;

	slider=(C_Slider *)control;
	scale = static_cast<int>(((float)slider->GetSliderPos()/(slider->GetSliderMax()-slider->GetSliderMin()) * 4 + 1.5F));

	if(scale != slider->GetUserNumber(2))
	{
		C_EditBox *ebox;
		ebox = (C_EditBox *)control->Parent_->FindControl(slider->GetUserNumber(0));
		if(ebox)
		{
			ebox->SetInteger(scale);
			ebox->Refresh();
		}

		INFOSaveRules();
		CheckCompliance();
	}
	slider->SetUserNumber(2,scale);
}

static void RuleControlCB(long,short hittype,C_Base *control)
{
	if(!control)
		return;

	if((hittype != C_TYPE_LMOUSEUP) && (hittype != C_TYPE_SELECT))
		return;

	if(modify && control->_GetCType_() == _CNTL_BUTTON_) // if host I assume
	{
		if(control->GetState())
			control->SetState(0);
		else
			control->SetState(1);
		control->Refresh();
	}
	else
		return;

	INFOSaveRules();
	CheckCompliance();
}

static void CloseInfoWindowCB(long ID,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	if(CancelCB)
		(*CancelCB)();

	CloseWindowCB(ID,hittype,control);
}

void INFOHookupControls()
{
	C_Window	*win;
	C_Button	*button;
	C_ListBox	*lbox;
	C_Slider	*slider;

	win=gMainHandler->FindWindow(INFO_WIN);
	if(!win)
		return;

	button=(C_Button *)win->FindControl(CLOSE_WINDOW);
	if(button != NULL)
		button->SetCallback(CloseInfoWindowCB);
	
	button=(C_Button *)win->FindControl(INFO_CANCEL);
	if(button != NULL)
		button->SetCallback(CloseInfoWindowCB);
	
	button=(C_Button *)win->FindControl(INFO_COMPLY);
	if(button != NULL)
		button->SetCallback(ComplyCB);
	/*
	INFO_COMPLY
	VEHICLE_SIZE_READOUT_REQ
	VEHICLE_SIZE_REQ
	VEHICLE_SIZE_CUR
	VEHICLE_SIZE_READOUT_CUR

  all of these need special callbacks to only allow the host to change them

	FUEL_REQ
	CHAFFLARES_REQ
	COLLISIONS_REQ
	BLACKOUT_REQ
	IDTAGS_REQ
	WEATHER_REQ
	INVULNERABILITY_REQ
	VEHICLE_SIZE_REQ
	FLTMOD_REQ
	RADAR_REQ
	WEAPEFF_REQ
	AUTOPILOT_REQ
	REFUELING_REQ
	PADLOCK_REQ
	*/
	lbox=(C_ListBox *)win->FindControl(FLTMOD_CUR);
	if(lbox != NULL)
		lbox->SetCallback(ListBoxChangeCB);

	
	lbox=(C_ListBox *)win->FindControl(RADAR_CUR);
	if(lbox != NULL)
		lbox->SetCallback(ListBoxChangeCB);
	
	
	lbox=(C_ListBox *)win->FindControl(WEAPEFF_CUR);
	if(lbox != NULL)
		lbox->SetCallback(ListBoxChangeCB);
	
	
	lbox=(C_ListBox *)win->FindControl(AUTOPILOT_CUR);
	if(lbox != NULL)
		lbox->SetCallback(ListBoxChangeCB);

	lbox=(C_ListBox *)win->FindControl(PADLOCK_CUR);
	if(lbox != NULL)
		lbox->SetCallback(ListBoxChangeCB);
	
	lbox=(C_ListBox *)win->FindControl(REFUELING_CUR);
	if(lbox != NULL)
		lbox->SetCallback(ListBoxChangeCB);
	
	button=(C_Button *)win->FindControl(FUEL_CUR);
	if(button != NULL)
		button->SetCallback(ButtonChangeCB);
	
	button=(C_Button *)win->FindControl(CHAFFLARES_CUR);
	if(button != NULL)
		button->SetCallback(ButtonChangeCB);
	
	button=(C_Button *)win->FindControl(COLLISIONS_CUR);
	if(button != NULL)
		button->SetCallback(ButtonChangeCB);
	
	button=(C_Button *)win->FindControl(BLACKOUT_CUR);
	if(button != NULL)
		button->SetCallback(ButtonChangeCB);
	
	
	button=(C_Button *)win->FindControl(IDTAGS_CUR);
	if(button != NULL)
		button->SetCallback(ButtonChangeCB);
	
	button=(C_Button *)win->FindControl(WEATHER_CUR);
	if(button != NULL)
		button->SetCallback(ButtonChangeCB);
	

	button=(C_Button *)win->FindControl(INVULNERABILITY_CUR); 
	if(button != NULL)
		button->SetCallback(ButtonChangeCB);

	/* May need to hit this if veh mag is 1
	button=(C_Button *)win->FindControl(AUTO_SCALE);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1)
			PlayerOptions.SetObjFlag(DISP_OBJ_DYN_SCALING);
		else
			PlayerOptions.ClearObjFlag(DISP_OBJ_DYN_SCALING);
	}*/

	slider=(C_Slider *)win->FindControl(VEHICLE_SIZE_CUR);
	if(slider != NULL)
		slider->SetCallback(SliderChangeCB);

	//required values

	lbox=(C_ListBox *)win->FindControl(FLTMOD_REQ);
	if(lbox != NULL)
		lbox->SetCallback(RuleControlCB);

	
	lbox=(C_ListBox *)win->FindControl(RADAR_REQ);
	if(lbox != NULL)
		lbox->SetCallback(RuleControlCB);
	
	
	lbox=(C_ListBox *)win->FindControl(WEAPEFF_REQ);
	if(lbox != NULL)
		lbox->SetCallback(RuleControlCB);
	
	
	lbox=(C_ListBox *)win->FindControl(AUTOPILOT_REQ);
	if(lbox != NULL)
		lbox->SetCallback(RuleControlCB);

	lbox=(C_ListBox *)win->FindControl(PADLOCK_REQ);
	if(lbox != NULL)
		lbox->SetCallback(RuleControlCB);
	
	lbox=(C_ListBox *)win->FindControl(REFUELING_REQ);
	if(lbox != NULL)
		lbox->SetCallback(RuleControlCB);
	
	button=(C_Button *)win->FindControl(FUEL_REQ);
	if(button != NULL)
		button->SetCallback(RuleControlCB);
	
	button=(C_Button *)win->FindControl(CHAFFLARES_REQ);
	if(button != NULL)
		button->SetCallback(RuleControlCB);
	
	button=(C_Button *)win->FindControl(COLLISIONS_REQ);
	if(button != NULL)
		button->SetCallback(RuleControlCB);
	
	button=(C_Button *)win->FindControl(BLACKOUT_REQ);
	if(button != NULL)
		button->SetCallback(RuleControlCB);
	
	
	button=(C_Button *)win->FindControl(IDTAGS_REQ);
	if(button != NULL)
		button->SetCallback(RuleControlCB);
	
	button=(C_Button *)win->FindControl(WEATHER_REQ);
	if(button != NULL)
		button->SetCallback(RuleControlCB);
	

	button=(C_Button *)win->FindControl(EXT_VIEWS_REQ);
	if(button != NULL)
		button->SetCallback(RuleControlCB);
	
	button=(C_Button *)win->FindControl(INVULNERABILITY_REQ); 
	if(button != NULL)
		button->SetCallback(RuleControlCB);

	/* May need to hit this if veh mag is 1
	button=(C_Button *)win->FindControl(AUTO_SCALE);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1)
			PlayerOptions.SetObjFlag(DISP_OBJ_DYN_SCALING);
		else
			PlayerOptions.ClearObjFlag(DISP_OBJ_DYN_SCALING);
	}*/

	
	
	slider=(C_Slider *)win->FindControl(VEHICLE_SIZE_REQ);
	if(slider != NULL)
		slider->SetCallback(SliderRuleControlCB);
}

void SetupInfoWindow(void (*tOkCB)(), void (*tCancelCB)())
{
	FalconGameEntity *game;
	C_Window *win;

	win=gMainHandler->FindWindow(INFO_WIN);
	if(!win)
	{
		if(tCancelCB)
			(*tCancelCB)();
		return;
	}
	/*if(FalconLocalGame != vuPlayerPoolGroup)
		CurrRules = &FalconLocalGameEntity->rules;
	else if(gCommsMgr->GetTargetGame())
		CurrRules = &((FalconGameEntity *)gCommsMgr->GetTargetGame())->rules;
	else
		CurrRules = &gRules[RuleMode];*/
	game = (FalconGameEntity *)gCommsMgr->GetTargetGame();
	if(game)
	{
		CurrRules.LoadRules(game->rules.GetRules());
		modify = FALSE;
	}
	else
	{
		modify = TRUE;
		CurrRules.LoadRules(gRules[RuleMode].GetRules());
	}
	
	OkCB = tOkCB;
	CancelCB = tCancelCB;

	
	if(!INFOLoaded)
	{
		switch(gLangIDNum)
		{
			case F4LANG_ENGLISH:
			case F4LANG_UK:
			case F4LANG_GERMAN:
				_stprintf(GameName,"%s%s",LogBook.Callsign(),gStringMgr->GetString(TXT_APPEND_GAME));
				break;
			case F4LANG_FRENCH:
			case F4LANG_SPANISH:
			case F4LANG_ITALIAN:
			case F4LANG_PORTUGESE:
				_stprintf(GameName,"%s %s",gStringMgr->GetString(TXT_APPEND_GAME),LogBook.Callsign());
				break;
		}

		INFOHookupControls();	
		INFOLoaded++;
	}

	INFOSetupControls();

	gMainHandler->EnableWindowGroup(win->GetGroup());
}


static void INFOSaveRules(void)
{
	C_Window	*win;
	C_Button	*button;
	C_ListBox	*lbox;
	C_Slider	*slider;
	C_EditBox	*ebox;
	RulesClass  tempRules;

	tempRules.Initialize();

	if (vuPlayerPoolGroup != vuLocalGame)
		return;

	win=gMainHandler->FindWindow(INFO_WIN);
	
	if(win == NULL)
		return;

	int host = FALSE;
/*
	if(FalconLocalGameEntity && FalconLocalSession && FalconLocalGameEntity->OwnerId() == FalconLocalSession->Id())
		host = TRUE;
	else if(!gCommsMgr->GetTargetGame() && !FalconLocalGameEntity)
	*/	host = modify;

	if(host)
	{
		ebox = (C_EditBox *)win->FindControl(INFO_GAMENAME);
		if(ebox)
		{
			//if(FalconLocalGame && _tcscmp( FalconLocalGame->GameName(),ebox->GetText() ) )
			//	FalconLocalGame->SetGameName(ebox->GetText());
			_tcscpy(GameName,ebox->GetText());
		}

		ebox = (C_EditBox *)win->FindControl(INFO_PASSWORD);
		if(ebox)
		{
			if(ebox->GetText())
				tempRules.SetPassword(ebox->GetText());
			//if(FalconLocalGameEntity)
				//FalconLocalGameEntity->EncipherPassword(CurrRules.Password,RUL_PW_LEN);
		}

		ebox = (C_EditBox *)win->FindControl(MAX_PLAYERS);
		if(ebox)
		{
			tempRules.SetMaxPlayers(ebox->GetInteger());
		}

		lbox=(C_ListBox *)win->FindControl(FLTMOD_REQ);
		if(lbox != NULL)
		{
			if((lbox->GetTextID())==FLTMOD_1)
				tempRules.SimFlightModel = FMAccurate;
			else
				tempRules.SimFlightModel = FMSimplified;
		}

		lbox=(C_ListBox *)win->FindControl(RADAR_REQ);
		if(lbox != NULL)
		{
			switch(lbox->GetTextID())
			{
				// M.N. full realism mode added
			case RADAR_0:
				tempRules.SimAvionicsType = ATRealisticAV;
				break;
			case RADAR_1:
				tempRules.SimAvionicsType = ATRealistic;
				break;
			case RADAR_2:
				tempRules.SimAvionicsType = ATSimplified;
				break;
			case RADAR_3:
				tempRules.SimAvionicsType = ATEasy;
				break;
			}
		}

		lbox=(C_ListBox *)win->FindControl(WEAPEFF_REQ);
		if(lbox != NULL)
		{
			switch(lbox->GetTextID())
			{
			case WEAPEFF_1:
				tempRules.SimWeaponEffect = WEAccurate;
				break;
			case WEAPEFF_2:
				tempRules.SimWeaponEffect = WEEnhanced;
				break;
			case WEAPEFF_3:
				tempRules.SimWeaponEffect = WEExaggerated;
				break;
			}
		}

		lbox=(C_ListBox *)win->FindControl(AUTOPILOT_REQ);
		if(lbox != NULL)
		{
			switch(lbox->GetTextID())
			{
			case AUTO_1:
				tempRules.SimAutopilotType = APNormal;
				break;
			case AUTO_2:
				tempRules.SimAutopilotType = APEnhanced;
				break;
			case AUTO_3:
				tempRules.SimAutopilotType = APIntelligent;
				break;
			}
		}

		lbox=(C_ListBox *)win->FindControl(PADLOCK_REQ);
		if(lbox != NULL)
		{
			switch(lbox->GetTextID())
			{
			case PADLOCK_4:
				tempRules.SimPadlockMode = PDDisabled;
				break;
			case PADLOCK_1:
				tempRules.SimPadlockMode = PDRealistic;
				break;
			case PADLOCK_2:
				tempRules.SimPadlockMode = PDEnhanced;
				break;
			//case PADLOCK_3:
			//	tempRules.SimPadlockMode = PDSuper;
			//	break;
			}
		}

		lbox=(C_ListBox *)win->FindControl(REFUELING_REQ);
		if(lbox != NULL)
		{
			switch(lbox->GetTextID())
			{
			case REFUEL_1:
				tempRules.SimAirRefuelingMode = ARRealistic;
				break;
			case REFUEL_2:
				tempRules.SimAirRefuelingMode = ARModerated;
				break;
			case REFUEL_3:
				tempRules.SimAirRefuelingMode = ARSimplistic;
				break;
			}
		}

		button=(C_Button *)win->FindControl(FUEL_REQ);
		if(button != NULL)
		{
			if(button->GetState() == C_STATE_1)
				tempRules.SetSimFlag (SIM_UNLIMITED_FUEL);
			else
				tempRules.ClearSimFlag (SIM_UNLIMITED_FUEL);
		}

		button=(C_Button *)win->FindControl(CHAFFLARES_REQ);
		if(button != NULL)
		{
			if(button->GetState() == C_STATE_1)
				tempRules.SetSimFlag (SIM_UNLIMITED_CHAFF);
			else
				tempRules.ClearSimFlag (SIM_UNLIMITED_CHAFF);
			
		}

		button=(C_Button *)win->FindControl(COLLISIONS_REQ);
		if(button != NULL)
		{
			if(button->GetState() == C_STATE_1)
				tempRules.SetSimFlag (SIM_NO_COLLISIONS);
			else
				tempRules.ClearSimFlag (SIM_NO_COLLISIONS);
		}

		button=(C_Button *)win->FindControl(BLACKOUT_REQ);
		if(button != NULL)
		{
			if(button->GetState() == C_STATE_1)
				tempRules.SetSimFlag (SIM_NO_BLACKOUT);
			else
				tempRules.ClearSimFlag (SIM_NO_BLACKOUT);
		}


		button=(C_Button *)win->FindControl(IDTAGS_REQ);
		if(button != NULL)
		{
			if(button->GetState() == C_STATE_1)
				tempRules.SetSimFlag (SIM_NAMETAGS);
			else
				tempRules.ClearSimFlag (SIM_NAMETAGS);			
		}

		button=(C_Button *)win->FindControl(WEATHER_REQ);
		if(button != NULL)
		{
			if(button->GetState() == C_STATE_1)
				tempRules.SetGenFlag(GEN_NO_WEATHER);
			else
				tempRules.ClearGenFlag(GEN_NO_WEATHER);
		}


		button=(C_Button *)win->FindControl(INVULNERABILITY_REQ); 
		if(button != NULL)
		{
			if(button->GetState() == C_STATE_1)
				tempRules.SetSimFlag(SIM_INVULNERABLE);
			else
				tempRules.ClearSimFlag(SIM_INVULNERABLE);
		}

		button=(C_Button *)win->FindControl(EXT_VIEWS_REQ); 
		if(button != NULL)
		{		
			if(button->GetState() == C_STATE_1)
				tempRules.SetGenFlag(GEN_EXTERNAL_VIEW);
			else
				tempRules.ClearGenFlag(GEN_EXTERNAL_VIEW);
		}

		/* May need to hit this if veh mag is 1
		button=(C_Button *)win->FindControl(AUTO_SCALE);
		if(button != NULL)
		{
			if(button->GetState() == C_STATE_1)
				PlayerOptions.SetObjFlag(DISP_OBJ_DYN_SCALING);
			else
				PlayerOptions.ClearObjFlag(DISP_OBJ_DYN_SCALING);
		}*/

		slider=(C_Slider *)win->FindControl(VEHICLE_SIZE_REQ);
		if(slider != NULL)
		{
			tempRules.ObjMagnification = static_cast<float>((int)( (float)slider->GetSliderPos()/( slider->GetSliderMax() - slider->GetSliderMin() ) * 4 + 1.5f ));
		}

//		if(FalconLocalGameEntity)
//			FalconLocalGameEntity->UpdateRules(tempRules.GetRules());
//		else
			gRules[RuleMode].LoadRules(tempRules.GetRules());
	}
}

static void INFOSaveValues(void)
{
	C_Window	*win;
	C_Button	*button;
	C_ListBox	*lbox;
	C_Slider	*slider;
	
	
	win=gMainHandler->FindWindow(INFO_WIN);
	
	if(win == NULL)
		return;

	int host = FALSE;

	if(FalconLocalGame && FalconLocalSession && FalconLocalGame->OwnerId() == FalconLocalSession->Id())
		host = TRUE;
	
	lbox=(C_ListBox *)win->FindControl(FLTMOD_CUR);
	if(lbox != NULL)
	{
		if((lbox->GetTextID())==FLTMOD_1)
			PlayerOptions.SimFlightModel = FMAccurate;
		else
			PlayerOptions.SimFlightModel = FMSimplified;

		lbox->Refresh();
	}
	
	lbox=(C_ListBox *)win->FindControl(RADAR_CUR);
	if(lbox != NULL)
	{
		switch(lbox->GetTextID())
		{
			// M.N. full realism mode added
		case RADAR_0:
			PlayerOptions.SimAvionicsType = ATRealisticAV;
			break;
		case RADAR_1:
			PlayerOptions.SimAvionicsType = ATRealistic;
			break;
		case RADAR_2:
			PlayerOptions.SimAvionicsType = ATSimplified;
			break;
		case RADAR_3:
			PlayerOptions.SimAvionicsType = ATEasy;
			break;
		}
		lbox->Refresh();
	}
	
	lbox=(C_ListBox *)win->FindControl(WEAPEFF_CUR);
	if(lbox != NULL)
	{
		switch(lbox->GetTextID())
		{
		case WEAPEFF_1:
			PlayerOptions.SimWeaponEffect = WEAccurate;
			break;
		case WEAPEFF_2:
			PlayerOptions.SimWeaponEffect = WEEnhanced;
			break;
		case WEAPEFF_3:
			PlayerOptions.SimWeaponEffect = WEExaggerated;
			break;
		}
		lbox->Refresh();
	}
	
	lbox=(C_ListBox *)win->FindControl(AUTOPILOT_CUR);
	if(lbox != NULL)
	{
		switch(lbox->GetTextID())
		{
		case AUTO_1:
			PlayerOptions.SimAutopilotType = APNormal;
			break;
		case AUTO_2:
			PlayerOptions.SimAutopilotType = APEnhanced;
			break;
		case AUTO_3:
			PlayerOptions.SimAutopilotType = APIntelligent;
			break;
		}
		lbox->Refresh();
	}

	lbox=(C_ListBox *)win->FindControl(PADLOCK_CUR);
	if(lbox != NULL)
	{
		switch(lbox->GetTextID())
		{
		case PADLOCK_4:
			PlayerOptions.SimPadlockMode = PDDisabled;
			break;
		case PADLOCK_1:
			PlayerOptions.SimPadlockMode = PDRealistic;
			break;
		case PADLOCK_2:
			PlayerOptions.SimPadlockMode = PDEnhanced;
			break;
		//case PADLOCK_3:
		//	PlayerOptions.SimPadlockMode = PDSuper;
		//	break;
		}
		lbox->Refresh();
	}
	
	lbox=(C_ListBox *)win->FindControl(REFUELING_CUR);
	if(lbox != NULL)
	{
		switch(lbox->GetTextID())
		{
		case REFUEL_1:
			PlayerOptions.SimAirRefuelingMode = ARRealistic;
			break;
		case REFUEL_2:
			PlayerOptions.SimAirRefuelingMode = ARModerated;
			break;
		case REFUEL_3:
			PlayerOptions.SimAirRefuelingMode = ARSimplistic;
			break;
		}
		lbox->Refresh();
	}
	
	button=(C_Button *)win->FindControl(FUEL_CUR);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1 || button->GetState() == C_STATE_2)
			PlayerOptions.SetSimFlag (SIM_UNLIMITED_FUEL);
		else
			PlayerOptions.ClearSimFlag (SIM_UNLIMITED_FUEL);
	}
	
	button=(C_Button *)win->FindControl(CHAFFLARES_CUR);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1 || button->GetState() == C_STATE_2)
			PlayerOptions.SetSimFlag (SIM_UNLIMITED_CHAFF);
		else
			PlayerOptions.ClearSimFlag (SIM_UNLIMITED_CHAFF);
		
	}
	
	button=(C_Button *)win->FindControl(COLLISIONS_CUR);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1 || button->GetState() == C_STATE_2)
			PlayerOptions.SetSimFlag (SIM_NO_COLLISIONS);
		else
			PlayerOptions.ClearSimFlag (SIM_NO_COLLISIONS);
	}
	
	button=(C_Button *)win->FindControl(BLACKOUT_CUR);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1 || button->GetState() == C_STATE_2)
			PlayerOptions.SetSimFlag (SIM_NO_BLACKOUT);
		else
			PlayerOptions.ClearSimFlag (SIM_NO_BLACKOUT);
	}
	
	
	button=(C_Button *)win->FindControl(IDTAGS_CUR);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1 || button->GetState() == C_STATE_2)
			PlayerOptions.SetSimFlag (SIM_NAMETAGS);
		else
			PlayerOptions.ClearSimFlag (SIM_NAMETAGS);			
	}
	
	button=(C_Button *)win->FindControl(WEATHER_CUR);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1 || button->GetState() == C_STATE_2)
			PlayerOptions.GeneralFlags |= GEN_NO_WEATHER;
		else
			PlayerOptions.GeneralFlags &= ~GEN_NO_WEATHER;
	}
	

	button=(C_Button *)win->FindControl(INVULNERABILITY_CUR); 
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1 || button->GetState() == C_STATE_2)
			PlayerOptions.SetSimFlag(SIM_INVULNERABLE);
		else
			PlayerOptions.ClearSimFlag(SIM_INVULNERABLE);
	}

	/* May need to hit this if veh mag is 1
	button=(C_Button *)win->FindControl(AUTO_SCALE);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1)
			PlayerOptions.SetObjFlag(DISP_OBJ_DYN_SCALING);
		else
			PlayerOptions.ClearObjFlag(DISP_OBJ_DYN_SCALING);
	}*/

	
	
	slider=(C_Slider *)win->FindControl(VEHICLE_SIZE_CUR);
	if(slider != NULL)
	{
		PlayerOptions.ObjMagnification = static_cast<float>((int)( (float)slider->GetSliderPos()/( slider->GetSliderMax() - slider->GetSliderMin() ) * 4 + 1.5f ));
	}
	
	PlayerOptions.SaveOptions();

	INFOSaveRules();
	INFOSaveRulesToFile();

}//SaveValues

void CopyRulesToGame(FalconGameEntity *)
{
	//game->UpdateRules(gRules[RuleMode].GetRules());
}
