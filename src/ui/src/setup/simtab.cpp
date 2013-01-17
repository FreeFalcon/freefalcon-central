#include "falclib.h"
#include "chandler.h"
#include "userids.h"
#include "PlayerOp.h"
#include "sim\include\stdhdr.h"
#include "ui_setup.h"
#include "f4find.h"

//define presets for skill level list box (first set is ace, then veteran, etc...)
Preset Presets[] = {
{FMAccurate,ATRealisticAV,WEAccurate,APNormal,ARRealistic,PDRealistic,0},
{FMAccurate,ATRealistic,WEAccurate,APNormal,ARModerated,PDRealistic,SIM_UNLIMITED_CHAFF},
{FMAccurate,ATSimplified,WEEnhanced,APEnhanced,ARModerated,PDEnhanced,SIM_NO_BLACKOUT|SIM_UNLIMITED_CHAFF|SIM_NAMETAGS},
{FMSimplified,ATEasy,WEExaggerated,APEnhanced,ARSimplistic,PDEnhanced,SIM_NO_BLACKOUT|SIM_UNLIMITED_CHAFF|SIM_NAMETAGS|SIM_UNLIMITED_FUEL},			//need to put SIM_UNLIMITED_FUEL
{FMSimplified,ATEasy,WEExaggerated,APIntelligent,ARSimplistic, PDEnhanced,SIM_NO_BLACKOUT|SIM_UNLIMITED_AMMO|SIM_UNLIMITED_CHAFF|SIM_NAMETAGS|SIM_UNLIMITED_FUEL|SIM_NO_COLLISIONS},
//{FMSimplified,ATEasy,WEExaggerated,APIntelligent,ARSimplistic, PDSuper,SIM_NO_BLACKOUT|SIM_UNLIMITED_AMMO|SIM_UNLIMITED_CHAFF|SIM_NAMETAGS|SIM_UNLIMITED_FUEL|SIM_NO_COLLISIONS},	//back into these two after DEMO
};

extern C_Handler *gMainHandler;

/////////////
// Sim Tab
/////////////

int GetRealism(C_Window *win)
{
	C_ListBox	*listbox;
	C_Button	*button;

	int			realism = 100,maxrealism = 100;

	if(win == NULL)
		return 0;
	
	listbox=(C_ListBox *)win->FindControl(SET_FLTMOD);
	if(listbox != NULL)
	{
		button = (C_Button *)listbox->GetItem(listbox->GetTextID());
		if(button)
		{
			realism   -= button->GetUserNumber(0);
			if(maxrealism   > button->GetUserNumber(1))
				maxrealism   = button->GetUserNumber(1);
		}
	}
	
	listbox=(C_ListBox *)win->FindControl(SET_RADAR);
	if(listbox != NULL)
	{
	
		button = (C_Button *)listbox->GetItem(listbox->GetTextID());
		if(button)
		{
			realism   -= button->GetUserNumber(0);
			if(maxrealism   > button->GetUserNumber(1))
				maxrealism   = button->GetUserNumber(1);
		}
	}
	
	listbox=(C_ListBox *)win->FindControl(SET_WEAPEFF);
	if(listbox != NULL)
	{
		button = (C_Button *)listbox->GetItem(listbox->GetTextID());
		if(button)
		{
			realism   -= button->GetUserNumber(0);
			if(maxrealism   > button->GetUserNumber(1))
				maxrealism   = button->GetUserNumber(1);
		}
	}
	
	listbox=(C_ListBox *)win->FindControl(SET_AUTOPILOT);
	if(listbox != NULL)
	{
	
		button = (C_Button *)listbox->GetItem(listbox->GetTextID());
		if(button)
		{
			realism   -= button->GetUserNumber(0);
			if(maxrealism   > button->GetUserNumber(1))
				maxrealism   = button->GetUserNumber(1);
		}
	}

	listbox=(C_ListBox *)win->FindControl(SET_PADLOCK);
	if(listbox != NULL)
	{
	
		button = (C_Button *)listbox->GetItem(listbox->GetTextID());
		if(button)
		{
			realism   -= button->GetUserNumber(0);
			if(maxrealism   > button->GetUserNumber(1))
				maxrealism   = button->GetUserNumber(1);
		}
	}

	listbox=(C_ListBox *)win->FindControl(SET_REFUELING);
	if(listbox != NULL)
	{
	
		button = (C_Button *)listbox->GetItem(listbox->GetTextID());
		if(button)
		{
			realism   -= button->GetUserNumber(0);
			if(maxrealism   > button->GetUserNumber(1))
				maxrealism   = button->GetUserNumber(1);
		}
	}

	
	button=(C_Button *)win->FindControl(SET_FUEL);
	if(button != NULL)
	{
		if(button->GetState())
		{
			realism   -= button->GetUserNumber(0);
			if(maxrealism   > button->GetUserNumber(1))
					maxrealism   = button->GetUserNumber(1);
		}
	}
	
	button=(C_Button *)win->FindControl(SET_CHAFFLARES);
	if(button != NULL)
	{
		if(button->GetState())
		{
			realism   -= button->GetUserNumber(0);
			if(maxrealism   > button->GetUserNumber(1))
				maxrealism   = button->GetUserNumber(1);
		}
		
	}
	
	button=(C_Button *)win->FindControl(SET_COLLISIONS);
	if(button != NULL)
	{
		if(button->GetState())
		{
	// Cobra - 03-08-05 Temp disabled due to unknown cause mid-air collisions
	//		realism   -= button->GetUserNumber(0);
	//		if(maxrealism   > button->GetUserNumber(1))
	//			maxrealism   = button->GetUserNumber(1);
		}
	}
	
	button=(C_Button *)win->FindControl(SET_BLACKOUT);
	if(button != NULL)
	{
		if(button->GetState())
		{
			realism   -= button->GetUserNumber(0);
			if(maxrealism   > button->GetUserNumber(1))
				maxrealism   = button->GetUserNumber(1);
		}
	}
	
	
	button=(C_Button *)win->FindControl(SET_IDTAGS);
	if(button != NULL)
	{
		if(button->GetState())
		{
			realism   -= button->GetUserNumber(0);
			if(maxrealism   > button->GetUserNumber(1))
				maxrealism   = button->GetUserNumber(1);
		}
	}
	
	button=(C_Button *)win->FindControl(SET_WEATHER);
	if(button != NULL)
	{
		if(button->GetState())
		{
			realism   -= button->GetUserNumber(0);
			if(maxrealism   > button->GetUserNumber(1))
				maxrealism   = button->GetUserNumber(1);
		}
	}
	
	button=(C_Button *)win->FindControl(SET_BULLSEYE_CALLS);
	if(button != NULL)
	{
		if(button->GetState())
		{
			realism   -= button->GetUserNumber(0);
			if(maxrealism   > button->GetUserNumber(1))
				maxrealism   = button->GetUserNumber(1);
		}
	}

	button=(C_Button *)win->FindControl(SET_INVULNERABILITY); 
	if(button != NULL)
	{
		if(button->GetState())
		{
			//realism = 0;
			
			realism   -= button->GetUserNumber(0);
			if(maxrealism   > button->GetUserNumber(1))
				maxrealism   = button->GetUserNumber(1);
			//realism *= 0.5f;
			
		}
	}

	if(realism  > maxrealism )
			realism  = maxrealism ;

	if (realism < 0) 
		realism = 0;

	return realism;
}//GetRealism

int SetRealism(C_Window *win)
{
	C_Button	*button;
	C_EditBox	*ebox;
	C_ListBox	*listbox;
	int			realism = 100,maxrealism = 100;

	if(win == NULL)
		return 0;
	
	realism = GetRealism(win);
		
	ebox=(C_EditBox *)win->FindControl(REALISM_READOUT);
	if(ebox != NULL)
	{
		if(realism  > maxrealism )
			realism  = maxrealism ;

		if (realism < 0) 
			realism = 0;

		ebox->SetInteger(realism );
	}

	listbox = (C_ListBox *)win->FindControl(SET_SKILL);
	if(listbox)
	{
		listbox->SetValue(RECRUIT_LEVEL);

		for(int i = 0; i < NUM_LEVELS; i++)
		{
			button = (C_Button *)listbox->GetItem(ACE_LEVEL + i);
			if(button)
			{
				if(realism > button->GetUserNumber(0))
				{
					listbox->SetValue(ACE_LEVEL + i);
					break;
				}
			}
		}
	}
	 
	win->RefreshWindow();

	return realism;
}//SetRealism



void SimControlCB(long,short hittype, C_Base *control)
{
	if((hittype != C_TYPE_LMOUSEUP) && (hittype != C_TYPE_SELECT))
		return;

	SetRealism(control->Parent_);
}

void SetSkillCB(long,short hittype, C_Base *control)
{
	if(hittype != C_TYPE_SELECT)
		return;
	
	int			Index;
	C_Button	*button;
	C_Window	*win;
	C_ListBox	*lbox;

	win = (C_Window *)control->Parent_;

	Index = ((C_ListBox *)control)->GetTextID() - ACE_LEVEL;

	lbox=(C_ListBox *)win->FindControl(SET_FLTMOD);
	if(lbox != NULL)
	{
		if(Presets[Index].FlightModel==FMAccurate)
			lbox->SetValue(SET_FLTMOD_1);
		else
			lbox->SetValue(SET_FLTMOD_2);
	}
	
	lbox=(C_ListBox *)win->FindControl(SET_RADAR);
	if(lbox != NULL)
	{
		switch(Presets[Index].RadarMode)
		{
			// M.N. full realism mode added
		case ATRealisticAV:
			lbox->SetValue(SET_RADAR_0);
			break;
		case ATRealistic:
			lbox->SetValue(SET_RADAR_1);
			break;
		case ATSimplified:
			lbox->SetValue(SET_RADAR_2);
			break;
		case ATEasy:
			lbox->SetValue(SET_RADAR_3);
			break;
		}
	}
	
	lbox=(C_ListBox *)win->FindControl(SET_WEAPEFF);
	if(lbox != NULL)
	{
		switch(Presets[Index].WeapEffects)
		{
		case WEAccurate:
			lbox->SetValue(SET_WEAPEFF_1);
			break;
		case WEEnhanced:
			lbox->SetValue(SET_WEAPEFF_2);
			break;
		case WEExaggerated:
			lbox->SetValue(SET_WEAPEFF_3);
			break;
		}
	}
	
	lbox=(C_ListBox *)win->FindControl(SET_AUTOPILOT);
	if(lbox != NULL)
	{
		switch(Presets[Index].Autopilot)
		{
		case APNormal:
			lbox->SetValue(SET_AUTO_1);
			break;
		case APEnhanced:
			lbox->SetValue(SET_AUTO_2);
			break;
		case APIntelligent:
			lbox->SetValue(SET_AUTO_3);
			break;
		}
	}

	lbox=(C_ListBox *)win->FindControl(SET_REFUELING);
	if(lbox != NULL)
	{
		switch(Presets[Index].RefuelingMode)
		{
		case ARRealistic:
			lbox->SetValue(SET_REFUEL_1);
			break;
		case ARModerated:
			lbox->SetValue(SET_REFUEL_2);
			break;
		case ARSimplistic:
			lbox->SetValue(SET_REFUEL_3);
			break;
		}
	}

	lbox=(C_ListBox *)win->FindControl(SET_PADLOCK);
	if(lbox != NULL)
	{
		switch(Presets[Index].PadlockMode)
		{
		case PDDisabled:
			lbox->SetValue(SET_PADLOCK_4);
			break;
		case PDRealistic:
			lbox->SetValue(SET_PADLOCK_1);
			break;
		case PDEnhanced:
			lbox->SetValue(SET_PADLOCK_2);
			break;
		//case PDSuper:
		//	lbox->SetValue(SET_PADLOCK_3);
		//	break;
		}
	}

	button=(C_Button *)win->FindControl(SET_ORDNANCE);
	if(button != NULL)
	{
		if(Presets[Index].flags & SIM_UNLIMITED_AMMO)
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}

	button=(C_Button *)win->FindControl(SET_FUEL);
	if(button != NULL)
	{
		if(Presets[Index].flags & SIM_UNLIMITED_FUEL)
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}
	
	button=(C_Button *)win->FindControl(SET_CHAFFLARES);
	if(button != NULL)
	{
		if(Presets[Index].flags & SIM_UNLIMITED_CHAFF)
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
		
	}
	
	button=(C_Button *)win->FindControl(SET_COLLISIONS);
	if(button != NULL)
	{
		if(Presets[Index].flags & SIM_NO_COLLISIONS)
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}
	
	button=(C_Button *)win->FindControl(SET_BLACKOUT);
	if(button != NULL)
	{
		if(Presets[Index].flags & SIM_NO_BLACKOUT)
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}
	
	button=(C_Button *)win->FindControl(SET_IDTAGS);
	if(button != NULL)
	{
		if(Presets[Index].flags & SIM_NAMETAGS)
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}
	
	
	button=(C_Button *)win->FindControl(SET_INVULNERABILITY); //should be SET_INVULNERABLITY
	if(button != NULL)
	{
		button->SetState(C_STATE_0);
			
		button->Refresh();
	}

	SetRealism(win);
}//SetSkillCB


/*****************************************************************************/
// Retro 25Dec2003
//
// Callback for the radiosubtitle button. Constructs/Destroys the radiolabel
// object. This object is already constructed at startup when the corresponding
// playeroptions variable (subTitles) is set to true.
/*****************************************************************************/
#include "RadioSubTitle.h"
extern int g_nSubTitleTTL;
extern int g_nNumberOfSubTitles;
extern char g_strRadioflightCol[0x40];			// Retro 27Dec2003
extern char g_strRadiotoPackageCol[0x40];		// Retro 27Dec2003
extern char g_strRadioToFromPackageCol[0x40];	// Retro 27Dec2003
extern char g_strRadioTeamCol[0x40];			// Retro 27Dec2003
extern char g_strRadioProximityCol[0x40];		// Retro 27Dec2003
extern char g_strRadioWorldCol[0x40];			// Retro 27Dec2003
extern char g_strRadioTowerCol[0x40];			// Retro 27Dec2003
extern char g_strRadioStandardCol[0x40];		// Retro 27Dec2003

void SubTitleCB(long ID,short hittype,C_Base *control)
{
	if (hittype != C_TYPE_LMOUSEUP)
		return;

	if ((PlayerOptions.getSubtitles() == false)&&(!radioLabel))	// need to create a new object..
	{
		try
		{
			radioLabel = new RadioSubTitle(g_nNumberOfSubTitles,g_nSubTitleTTL);
			
			radioLabel->SetChannelColours(	g_strRadioflightCol, g_strRadiotoPackageCol, g_strRadioToFromPackageCol,
											g_strRadioTeamCol, g_strRadioProximityCol, g_strRadioWorldCol,
											g_strRadioTowerCol, g_strRadioStandardCol);

			PlayerOptions.SetSubtitles(true);
			control->SetState(C_STATE_1);
		}
		catch (RadioSubTitle::Init_Error)
		{
			delete(radioLabel);
			radioLabel = 0;
			PlayerOptions.SetSubtitles(false);
			control->SetState(C_STATE_0);
		};
	}
	else	// need to delete the object..
	{
		delete(radioLabel);
		radioLabel = 0;
		PlayerOptions.SetSubtitles(false);
		control->SetState(C_STATE_0);
	}
}
// Retro 25Dec2003 ends..
