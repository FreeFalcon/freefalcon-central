/***************************************************************************\
UI_setup.cpp
Dave Power (x4373)

  setup screen stuff for falcon
\***************************************************************************/

#include "falclib.h"
#include "chandler.h"
#include "userids.h"
#include "PlayerOp.h"
#include "sim\include\stdhdr.h"
#include "uicomms.h"
#include "Graphics\Include\render3d.h"
#include "Graphics\Include\renderow.h"
#include "Graphics\Include\drawBSP.h"
#include "Graphics\Include\matrix.h"
#include "Graphics\Include\TexBank.h"
#include "Graphics\Include\TerrTex.h"
#include "Graphics\Include\FarTex.h"
#include "objectiv.h"
#include "cbsplist.h"
#include "c3dview.h"
#include "ui_setup.h"
#include "Graphics\Include\RViewPnt.h"
#include <tchar.h>
#include "f4find.h"
#include "sim\include\inpFunc.h"
#include "dispopts.h"
#include "logbook.h"
#include "sim\include\sinput.h"
#include "sim\include\simio.h"
#include "cmusic.h"
#include "dispcfg.h"
#include "Graphics\Include\draw2d.h"
#include "falcsess.h"
#include "Graphics\Include\tod.h"

//JAM 21Nov03
#include "Weather.h"
#include "Campaign\include\Cmpclass.h"
//JAM

extern int STPLoaded;
extern C_Handler *gMainHandler;
extern C_Parser *gMainParser;
extern char **KeyDescrips;  

extern C_3dViewer	*SetupViewer;
extern RViewPoint	*tmpVpoint;
extern ObjectPos	*Objects;
extern FeaturePos	*Features;
extern Drawable2D	*Smoke;

int GraphicSettingMult = 1;

//M.N.
//int			skycolortime;
//extern int	NumberOfSkyColors;
//extern SkyColorDataType* skycolor;

long Cluster = 8001;
int ready = FALSE;
//JOYCAPS S_joycaps;
MMRESULT S_joyret;
F4CSECTIONHANDLE* SetupCritSection = NULL;

float JoyScale;
float RudderScale;
float ThrottleScale;

RECT Rudder;
RECT Throttle;
int Calibrated = FALSE;

//defined in this file
static void HookupSetupControls(long ID);
void STPSetupControls(void);
void SetupRadioCB(long ID,short hittype,C_Base *control);


//defined in another file
void CloseWindowCB(long ID,short hittype,C_Base *control);
void UI_Help_Guide_CB(long ID,short hittype,C_Base *ctrl);
void GenericTimerCB(long ID,short hittype,C_Base *control);
void OpenLogBookCB(long ID,short hittype,C_Base *control);
BOOL AddWordWrapTextToWindow(C_Window *win,short *x,short *y,short startcol,short endcol,COLORREF color,_TCHAR *str,long Client=0);
void INFOSetupControls(void);
void CheckFlyButton(void);

//SimTab.cpp
int GetRealism(C_Window *win);
int SetRealism(C_Window *win);
void SimControlCB(long ID,short hittype,C_Base *control);
void SetSkillCB(long ID,short hittype,C_Base *control);
void TurbulenceCB(long ID,short hittype,C_Base *control); //JAM 06Nov03

//ControlsTab.cpp
SIM_INT CalibrateFile (void);
SIM_INT Calibrate ( void  );
void InitKeyDescrips(void);
void CleanupKeys(void);
void RefreshJoystickCB(long ID,short hittype,C_Base *control);
BOOL KeystrokeCB(unsigned char DKScanCode,unsigned char Ascii,unsigned char ShiftStates,long RepeatCount);
void CalibrateCB(long ID,short hittype,C_Base *control);
BOOL SaveKeyMapList( char *filename);
int UpdateKeyMapList( char *fname, int flag);
void StopCalibrating(C_Base *control);
void SetKeyDefaultCB(long ID,short hittype,C_Base *control);
int CreateKeyMapList(  char *filename);
void SaveKeyButtonCB(long ID,short hittype,C_Base *control);
void LoadKeyButtonCB(long ID,short hittype,C_Base *control);
void ControllerSelectCB(long ID,short hittype,C_Base *control);
void BuildControllerList(C_ListBox *lbox);
void HideKeyStatusLines(C_Window *win);
void RecenterJoystickCB(long ID,short hittype,C_Base *control);
void AdvancedControlCB(long ID,short hittype,C_Base *control);			// Retro 31Dec2003
void AdvancedControlApplyCB(long ID,short hittype,C_Base *control);		// Retro 31Dec2003
void AdvancedControlOKCB(long ID,short hittype,C_Base *control);		// Retro 31Dec2003
void AdvancedControlCancelCB(long ID,short hittype,C_Base *control);	// Retro 31Dec2003
void SetJoystickAndPOVSymbols(const bool, C_Base *control);
void SetThrottleAndRudderBars(C_Base *control);

void SetABDetentCB(long ID,short hittype,C_Base *control);

//GraphicsTab.cpp
void STPMoveRendererCB(C_Window *win);
void STPViewTimerCB(long ID,short hittype,C_Base *control);
void STPDisplayCB(long ID,short hittype,C_Base *control);
void SfxLevelCB(long ID,short hittype,C_Base *control);
void RenderViewCB(long ID,short hittype,C_Base *control);
void ChangeViewpointCB(long ID,short hittype,C_Base *control);
//void GouraudCB(long ID,short hittype,C_Base *control);
void HazingCB(long ID,short hittype,C_Base *control);
//void AlphaBlendCB(long ID,short hittype,C_Base *control);
void RealWeatherShadowsCB(long ID,short hittype,C_Base *control); //JAM 07Dec03
void BilinearFilterCB(long ID,short hittype,C_Base *control);
//void ObjectTextureCB(long ID,short hittype,C_Base *control);
void BuildingDetailCB(long ID,short hittype,C_Base *control);
void ObjectDetailCB(long ID,short hittype,C_Base *control);
void VehicleSizeCB(long ID,short hittype,C_Base *control);
void TerrainDetailCB(long ID,short hittype,C_Base *control);
//void TextureDistanceCB(long ID,short hittype,C_Base *control);
void VideoCardCB(long ID,short hittype,C_Base *control);
void VideoDriverCB(long ID,short hittype,C_Base *control);
void ResolutionCB(long ID,short hittype,C_Base *control);
void BuildVideoCardList(C_ListBox *lbox);
void DisableEnableDrivers(C_ListBox *lbox);
void DisableEnableResolutions(C_ListBox *lbox);
void BuildVideoDriverList(C_ListBox *lbox);
void GraphicsDefaultsCB(long ID,short hittype,C_Base *control);
void ScalingCB(long ID,short hittype,C_Base *control);
void BuildResolutionList(C_ListBox *lbox);
void PlayerBubbleCB(long ID,short hittype,C_Base *control);
void AdvancedCB(long ID,short hittype,C_Base *control);
void SetAdvanced();
void AdvancedGameCB(long ID,short hittype,C_Base *control);
void SubTitleCB(long ID,short hittype,C_Base *control);	// Retro 25Dec2003
void SeasonCB(long ID,short hittype,C_Base *control); //THW 2004-01-17

//JAM 21Nov03
void RealWeatherCB(long ID,short hittype,C_Base *control);


// M.N.
//void SkyColorCB(long ID,short hittype,C_Base *control);
//void SetSkyColor();
//void SelectSkyColorCB(long ID,short hittype,C_Base *control);
//void SkyColTimeCB(long ID,short hittype,C_Base *control);

// Player radio voice turn on/off via UI
void TogglePlayerVoiceCB(long ID,short hittype,C_Base *control);
void ToggleUICommsCB(long ID,short hittype, C_Base *control);

//SoundTab.cpp
void InitSoundSetup();
void TestButtonCB(long ID,short hittype,C_Base *control);
void SoundSliderCB(long ID,short hittype,C_Base *control);
void PlayVoicesCB(long ID,short hittype,C_Base *control);

RECT AxisValueBox = { 0 };
float AxisValueBoxHScale;
float AxisValueBoxWScale;

void LoadSetupWindows()
{
	long		ID=0;
	int			size=0;
	C_Button	*ctrl=NULL;
	C_Window	*win=NULL;
	C_TimerHook *tmr=NULL;
	C_Line		*line=NULL;
	C_Bitmap	*bmap=NULL;
	UI95_RECT	client;

	//if setup is already loaded, we only need to make sure all the control
	//settings are up to date
	if(STPLoaded) 
	{
		STPSetupControls();
		if(KeyVar.NeedUpdate)
		{
			UpdateKeyMapList(PlayerOptions.keyfile,1);
			KeyVar.NeedUpdate = FALSE;
		}
		return;
	}

	//Do basic UI setup of window
	if( _LOAD_ART_RESOURCES_)
		gMainParser->LoadImageList("st_res.lst");
	else
		gMainParser->LoadImageList("st_art.lst");
	gMainParser->LoadSoundList("st_snd.lst");
	gMainParser->LoadWindowList("st_scf.lst");	// Modified by M.N. - add art/art1024 by LoadWindowList
	
	ID=gMainParser->GetFirstWindowLoaded();
	while(ID)
	{
		//Hookup the callbacks
		HookupSetupControls(ID);
		ID=gMainParser->GetNextWindowLoaded();
	}
	
	
	win=gMainHandler->FindWindow(SETUP_WIN);
	if(win != NULL)
	{
		//timer to update joystick bmp on controls tab
		tmr=new C_TimerHook;
		tmr->Setup(C_DONT_CARE,C_TYPE_TIMER);
		tmr->SetUpdateCallback(GenericTimerCB);
		tmr->SetRefreshCallback(RefreshJoystickCB);
		tmr->SetUserNumber(_UI95_TIMER_DELAY_,1); // Timer activates every 80 mseconds (Only when this window is open)
		tmr->SetCluster(8004);
		win->AddControl(tmr);
		
		//timer to update the view position on graphics tab
		tmr=new C_TimerHook;
		tmr->Setup(C_DONT_CARE,C_TYPE_TIMER);
		tmr->SetUpdateCallback(GenericTimerCB);
		tmr->SetRefreshCallback(ChangeViewpointCB);
		tmr->SetUserNumber(_UI95_TIMER_DELAY_,1); // Timer activates every 80 mseconds (Only when this window is open)
		tmr->SetCluster(8002);
		win->AddControl(tmr);

		//timer to generate new radio message calls on sound tab
		tmr=new C_TimerHook;
		tmr->Setup(C_DONT_CARE,C_TYPE_TIMER);
		tmr->SetUpdateCallback(GenericTimerCB);
		tmr->SetRefreshCallback(PlayVoicesCB);
		tmr->SetUserNumber(_UI95_TIMER_DELAY_,100); // Timer activates every 8 seconds (Only when this window is open)
		tmr->SetCluster(8003);
		win->AddControl(tmr);
		
		//make sure sim tab is selected the first time in
		ctrl=(C_Button *)win->FindControl(SIM_TAB);
		SetupRadioCB(SETUP_WIN,C_TYPE_LMOUSEUP,ctrl);
		
		bmap=(C_Bitmap *)win->FindControl(JOY_INDICATOR);
		if(bmap != NULL)
		{
			size = bmap->GetH() + 1;
		}
		
		//use joystick.dat to calibrate joystick
		//Calibration.calibrated = CalibrateFile();
		
		client = win->GetClientArea(1);
		
		//setup scale for manipulating joystick control on controls tab
		//if(Calibration.calibrated)
			JoyScale = (float)(client.right - client.left - size)/2.0F;
		//else
			//JoyScale = (float)(client.right - client.left - size)/65536.0F;
		
#if 0	// Retro 17Jan2004
		//setup scale for manipulating rudder control on controls tab
		line=(C_Line *)win->FindControl(RUDDER);
		if(line != NULL)
		{
			if( !IO.AnalogIsUsed(AXIS_YAW) )	// Retro 31Dec2003
				line->SetColor(RGB(130,130,130)); //grey
			//if(Calibration.calibrated)
				RudderScale = (line->GetH() )/2.0F;
			//else
				//RudderScale = (line->GetH() )/65536.0F;
			
			Rudder.left = line->GetX();
			Rudder.right = line->GetX() + line->GetW();
			Rudder.top = line->GetY();
			Rudder.bottom = line->GetY() + line->GetH();
		}
		
		//setup scale for manipulating throttle control on controls tab
		line=(C_Line *)win->FindControl(THROTTLE);
		if(line != NULL)
		{
			if( !IO.AnalogIsUsed(AXIS_THROTTLE) )	// Retro 31Dec2003
				line->SetColor(RGB(130,130,130)); //grey

			//if(Calibration.calibrated)
				ThrottleScale = (float)line->GetH();
			//else
				//ThrottleScale = (line->GetH())/65536.0F;
			
			Throttle.left = line->GetX();
			Throttle.right = line->GetX() + line->GetW();
			Throttle.top = line->GetY();
			Throttle.bottom = line->GetY() + line->GetH();
			
		}
#else
		// Retro 17Jan2004 - now caters to both throttle axis
		//setup scale for manipulating rudder control on controls tab
		line=(C_Line *)win->FindControl(RUDDER);
		if(line != NULL)
		{
			RudderScale = (line->GetH() )/2.0F;
			Rudder.left = line->GetX();
			Rudder.right = line->GetX() + line->GetW();
			Rudder.top = line->GetY();
			Rudder.bottom = line->GetY() + line->GetH();
		}
		//setup scale for manipulating throttle control on controls tab
		line=(C_Line *)win->FindControl(THROTTLE);
		if(line != NULL)
		{
			ThrottleScale = (float)line->GetH();
			Throttle.left = line->GetX();
			Throttle.right = line->GetX() + line->GetW();
			Throttle.top = line->GetY();
			Throttle.bottom = line->GetY() + line->GetH();

			// looks strange but works (the function needs a control on the controller-sheet)
			// this function activates/deactivates the bars bases on availability
			SetThrottleAndRudderBars(line);
		}
#endif

		// Retro 27Mar2004 - a bar to show the value of an analogue axis
		// ..actually there are about 20+ of these, but I use the coords of one, the
		// others are (or rather: should be) aligned to this one
		C_Window* win2=gMainHandler->FindWindow(SETUP_CONTROL_ADVANCED_WIN);
		if(!win2) return;

		C_Line* line=(C_Line *)win2->FindControl(SETUP_ADVANCED_THROTTLE_VAL);
		if(line != NULL)
		{
			AxisValueBox.left = line->GetX();
			AxisValueBox.right = line->GetX() + line->GetW();
			AxisValueBox.top = line->GetY();
			AxisValueBox.bottom = line->GetY() + line->GetH();
			AxisValueBoxHScale = (float)line->GetH();
			AxisValueBoxWScale = (float)line->GetW();
		}
		else
		{
			ShiAssert(false);
		}
		// Retro end
				
	}
	
	InitKeyDescrips();
	//set all the controls to their correct positions according to the saved options file
	STPSetupControls();	
	
	CreateKeyMapList( PlayerOptions.keyfile);

	STPLoaded++;
}//LoadSetupWindows


void SetupOpenLogBookCB(long ID,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	InitSoundSetup();

	OpenLogBookCB(ID,hittype,control);
}

///this function sets up all the controls according to the values stored
///in the PlayerOptions structure
void STPSetupControls(void)
{
	C_Window	*win;
	C_Button	*button;
	C_Text		*text;
	C_ListBox	*lbox;
	C_Slider	*slider;
	C_EditBox	*ebox;
	
	win=gMainHandler->FindWindow(SETUP_WIN);
	
	if(win == NULL)
		return;
	
	lbox=(C_ListBox *)win->FindControl(SET_FLTMOD);
	if(lbox != NULL)
	{
		if(PlayerOptions.GetFlightModelType()==FMAccurate)
			lbox->SetValue(SET_FLTMOD_1);
		else
			lbox->SetValue(SET_FLTMOD_2);

		lbox->Refresh();
	}
	
	lbox=(C_ListBox *)win->FindControl(SET_RADAR);
	if(lbox != NULL)
	{
		switch(PlayerOptions.GetAvionicsType())
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
		lbox->Refresh();
	}
	
	lbox=(C_ListBox *)win->FindControl(SET_WEAPEFF);
	if(lbox != NULL)
	{
		switch(PlayerOptions.GetWeaponEffectiveness())
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
		lbox->Refresh();
	}
	
	lbox=(C_ListBox *)win->FindControl(SET_AUTOPILOT);
	if(lbox != NULL)
	{
		switch(PlayerOptions.GetAutopilotMode())
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
		lbox->Refresh();
	}

	lbox=(C_ListBox *)win->FindControl(SET_REFUELING);
	if(lbox != NULL)
	{
		switch(PlayerOptions.GetRefuelingMode())
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
		lbox->Refresh();
	}

	lbox=(C_ListBox *)win->FindControl(SET_PADLOCK);
	if(lbox != NULL)
	{
		switch(PlayerOptions.GetPadlockMode())
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
		lbox->Refresh();
	}

	lbox=(C_ListBox *)win->FindControl(SET_CANOPY_CUE);
	if(lbox != NULL)
	{
		switch(PlayerOptions.GetVisualCueMode())
		{
		case VCNone:
			lbox->SetValue(CUE_NONE);
			break;
		case VCLiftLine:
			lbox->SetValue(CUE_LIFT_LINE);
			break;
		case VCReflection:
			lbox->SetValue(CUE_REFLECTION_MAP);
			break;
		case VCBoth:
			lbox->SetValue(CUE_BOTH);
			break;
		}
		lbox->Refresh();
	}

	lbox=(C_ListBox *)win->FindControl(SET_VIDEO_DRIVER);
	if(lbox != NULL)
	{
		BuildVideoDriverList(lbox);

		DisableEnableDrivers(lbox);
		lbox->SetValue(DisplayOptions.DispVideoDriver + 1);
		lbox->Refresh();
	}

	lbox=(C_ListBox *)win->FindControl(SET_VIDEO_CARD);
	if(lbox != NULL)
	{
		BuildVideoCardList(lbox);

		lbox->SetValue(DisplayOptions.DispVideoCard + 1);
		lbox->Refresh();
	}
	
	lbox=(C_ListBox *)win->FindControl(SET_RESOLUTION);
	if(lbox != NULL)
	{
		BuildResolutionList(lbox);

		#if 1
		// OW
		// Handled in BuildResolutionList
		DeviceManager::DDDriverInfo *pDI = FalconDisplay.devmgr.GetDriver(DisplayOptions.DispVideoDriver);
		if(pDI)
		{
			int nIndex = pDI->FindDisplayMode(DisplayOptions.DispWidth, DisplayOptions.DispHeight, DisplayOptions.DispDepth);
			lbox->SetValue(nIndex != -1 ? nIndex : 0); 
		}
		#else
		DisableEnableResolutions(lbox);
		lbox->SetValue( DisplayOptions.DispWidth ); 
		lbox->Refresh();
		#endif
	}

	//JAM 20Nov03
	lbox = (C_ListBox *)win->FindControl(SETUP_REALWEATHER);
	if(lbox != NULL)
	{
		if( TheCampaign.InMainUI )
		{
			lbox->RemoveAllItems();
			lbox->AddItem(70208,C_TYPE_ITEM,"Sunny");
			lbox->AddItem(70209,C_TYPE_ITEM,"Fair");
			lbox->AddItem(70210,C_TYPE_ITEM,"Poor");
			lbox->AddItem(70211,C_TYPE_ITEM,"Inclement");
			lbox->SetValue(PlayerOptions.weatherCondition+70207);
			lbox->Refresh();
		}
		else if( ((WeatherClass *)realWeather)->lockedCondition )
		{
			lbox->RemoveAllItems();
			lbox->AddItem(70212,C_TYPE_ITEM,"Locked");
			lbox->AddItem(70213,C_TYPE_ITEM,"Unlock");
			lbox->Refresh();
		}
	}
	//JAM
/*
	//THW 2004-01-18
	lbox=(C_ListBox *)win->FindControl(SETUP_SEASON);
	if(lbox != NULL)
	{
		lbox->SetValue(PlayerOptions.Season+70313);
		lbox->Refresh();
	}
	//THW
*/
	button=(C_Button *)win->FindControl(SET_LOGBOOK);
	if(button != NULL)
	{
		button->SetText(0,UI_logbk.Callsign());
		button->Refresh();
	}
	
	button=(C_Button *)win->FindControl(SET_ORDNANCE);
	if(button != NULL)
	{
		if(PlayerOptions.UnlimitedAmmo())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}
	
	button=(C_Button *)win->FindControl(SET_FUEL);
	if(button != NULL)
	{
		if(PlayerOptions.UnlimitedFuel())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}
	
	button=(C_Button *)win->FindControl(SET_CHAFFLARES);
	if(button != NULL)
	{
		if(PlayerOptions.UnlimitedChaff())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
		
	}
	
	button=(C_Button *)win->FindControl(SET_COLLISIONS);
	if(button != NULL)
	{
		if(PlayerOptions.CollisionsOn())
			button->SetState(C_STATE_0);
		else
			button->SetState(C_STATE_1);
		button->Refresh();
	}
	
	button=(C_Button *)win->FindControl(SET_BLACKOUT);
	if(button != NULL)
	{
		if(PlayerOptions.BlackoutOn())
			button->SetState(C_STATE_0);
		else
			button->SetState(C_STATE_1);
		button->Refresh();
	}
	
	
	button=(C_Button *)win->FindControl(SET_IDTAGS);
	if(button != NULL)
	{
		if(PlayerOptions.NameTagsOn())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}
	
	button=(C_Button *)win->FindControl(SET_BULLSEYE_CALLS);
	if(button != NULL)
	{
		if(PlayerOptions.BullseyeOn())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}

	button=(C_Button *)win->FindControl(SET_INVULNERABILITY); //should be SET_INVULNERABLITY
	if(button != NULL)
	{
		if(PlayerOptions.InvulnerableOn())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}

	// Retro 25Dec2003
	button=(C_Button *)win->FindControl(SETUP_SIM_INFOBAR);
	if(button != NULL)
	{
		if(PlayerOptions.getInfoBar())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}

	button=(C_Button *)win->FindControl(SETUP_SIM_SUBTITLES);
	if(button != NULL)
	{
		if(PlayerOptions.getSubtitles())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}
	// ..ends
	
	ebox = (C_EditBox *)win->FindControl(ACMI_FILE_SIZE);
	if(ebox)
	{
		ebox->SetInteger( PlayerOptions.AcmiFileSize() );
		ebox->Refresh();
	}

	button=(C_Button *)win->FindControl(AUTO_SCALE);
	if(button != NULL)
	{
		if(PlayerOptions.ObjectDynScalingOn())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}

	button=(C_Button *)win->FindControl(GOUROUD);//GOUROUD
	if(button != NULL)
	{
		if(PlayerOptions.GouraudOn())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}
	
	button=(C_Button *)win->FindControl(HAZING);
	if(button != NULL)
	{
		if(PlayerOptions.HazingOn())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}
	
	//JAM 07Dec03
	button=(C_Button *)win->FindControl(SETUP_REALWEATHER_SHADOWS);
	if(button != NULL)
	{
		if(PlayerOptions.ShadowsOn())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}

	button=(C_Button *)win->FindControl(SETUP_SPECULAR_LIGHTING);
	if(button != NULL)
	{
		if(DisplayOptions.bSpecularLighting)
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}
	
/*	button=(C_Button *)win->FindControl(OBJECT_TEXTURES);
	if(button != NULL)
	{
		if(PlayerOptions.ObjectTexturesOn())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}
*/	
	// M.N.
	button=(C_Button *)win->FindControl(PLAYERVOICE);
	if(button != NULL)
	{
		if(PlayerOptions.PlayerRadioVoice)
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}

	button=(C_Button *)win->FindControl(UICOMMS);
	if(button != NULL)
	{
		if(PlayerOptions.UIComms)
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}


	text=(C_Text *)win->FindControl(CAL_TEXT);
	if(text != NULL)
		text->SetText("");

	InitSoundSetup();
	
	slider=(C_Slider *)win->FindControl(OBJECT_DETAIL);
	if(slider != NULL)
	{
		ebox = (C_EditBox *)win->FindControl(OBJECT_DETAIL_READOUT);
		if(ebox)
		{
			PlayerOptions.ObjDetailLevel = min(PlayerOptions.ObjDetailLevel, 2.0F*GraphicSettingMult);
			ebox->SetInteger( FloatToInt32((PlayerOptions.ObjDetailLevel- .5f)/.25f + 1.5f));
			ebox->Refresh();
			slider->SetSteps(static_cast<short>(6*GraphicSettingMult));
			slider->SetUserNumber(0,OBJECT_DETAIL_READOUT);
		}
		slider->SetSliderPos(FloatToInt32((slider->GetSliderMax()-slider->GetSliderMin())*(PlayerOptions.ObjDetailLevel-0.5f)/(1.5f*GraphicSettingMult)));
	}

	slider=(C_Slider *)win->FindControl(SFX_LEVEL);
	if(slider != NULL)
	{
		ebox = (C_EditBox *)win->FindControl(SFX_LEVEL_READOUT);
		if(ebox)
		{
			ebox->SetInteger( FloatToInt32(PlayerOptions.SfxLevel));
			ebox->Refresh();
			slider->SetUserNumber(0,SFX_LEVEL_READOUT);
		}
		slider->SetSliderPos(FloatToInt32((slider->GetSliderMax()-slider->GetSliderMin())*(PlayerOptions.SfxLevel - 1.0F)/4.0f));
	}

	slider=(C_Slider *)win->FindControl(DISAGG_LEVEL);
	if(slider != NULL)
	{
		ebox = (C_EditBox *)win->FindControl(DISAGG_LEVEL_READOUT);
		if(ebox)
		{
			ebox->SetInteger(PlayerOptions.BldDeaggLevel + 1);
			ebox->Refresh();
			slider->SetUserNumber(0,DISAGG_LEVEL_READOUT);
		}
		slider->SetSliderPos((slider->GetSliderMax()-slider->GetSliderMin())*PlayerOptions.ObjDeaggLevel/100);
	}
	
		
	slider=(C_Slider *)win->FindControl(VEHICLE_SIZE);
	if(slider != NULL)
	{
		slider->SetSliderPos(FloatToInt32((slider->GetSliderMax()-slider->GetSliderMin())*(PlayerOptions.ObjMagnification - 1.0F)/4.0F));
		ebox = (C_EditBox *)win->FindControl(VEHICLE_SIZE_READOUT);
		if(ebox)
		{
			ebox->SetInteger(FloatToInt32(PlayerOptions.ObjMagnification));
			ebox->Refresh();
			slider->SetUserNumber(0,VEHICLE_SIZE_READOUT);
		}
	}
	
/*	slider=(C_Slider *)win->FindControl(TEXTURE_DISTANCE);
	if(slider != NULL)
	{
		ebox = (C_EditBox *)win->FindControl(TEX_DISTANCE_READOUT);
		if(ebox)
		{
			ebox->SetInteger(PlayerOptions.DispTextureLevel + 1);
			ebox->Refresh();
			slider->SetUserNumber(0,TEX_DISTANCE_READOUT);
		}
		slider->SetSliderPos((slider->GetSliderMax()-slider->GetSliderMin())*(PlayerOptions.DispTextureLevel)/4);
	}
*/
	slider=(C_Slider *)win->FindControl(PLAYER_BUBBLE_SLIDER);
	if(slider != NULL)
	{
		ebox = (C_EditBox *)win->FindControl(PLAYER_BUBBLE_READOUT);
		if(ebox)
		{
			PlayerOptions.PlayerBubble = min(PlayerOptions.PlayerBubble, 2.0F*GraphicSettingMult);
			ebox->SetInteger( FloatToInt32((PlayerOptions.PlayerBubble - .5f)*4.0F + 1.5F));
			ebox->Refresh();
			slider->SetSteps(static_cast<short>(6*GraphicSettingMult));
			slider->SetUserNumber(0,PLAYER_BUBBLE_READOUT);
		}
		slider->SetSliderPos(FloatToInt32((slider->GetSliderMax()-slider->GetSliderMin())*(PlayerOptions.PlayerBubble-0.5f)/(1.5f*GraphicSettingMult)));
	}

	slider=(C_Slider *)win->FindControl(TERRAIN_DETAIL);
	if(slider != NULL)
	{
		int step;
		step = (slider->GetSliderMax()-slider->GetSliderMin())/(6*GraphicSettingMult);
	
		slider->SetSteps(static_cast<short>(6*GraphicSettingMult));

		if(PlayerOptions.DispTerrainDist > 40)
			slider->SetSliderPos(FloatToInt32(step*(2+(PlayerOptions.DispTerrainDist - 40.0F)/10.0F)));
		else 
			slider->SetSliderPos((2 - PlayerOptions.DispMaxTerrainLevel)*step);

		ebox = (C_EditBox *)win->FindControl(TEX_DETAIL_READOUT);
		if(ebox)
		{
			ebox->SetInteger( FloatToInt32(((float)slider->GetSliderPos()/(slider->GetSliderMax()-slider->GetSliderMin()))*6.0F*GraphicSettingMult + 1.5F));
			ebox->Refresh();
			slider->SetUserNumber(0,TEX_DETAIL_READOUT);
		}
	}


	//ControlsTab
	lbox = (C_ListBox *)win->FindControl(JOYSTICK_SELECT );//JOYSTICK_SELECT
	if(lbox)
	{
		BuildControllerList(lbox);

extern AxisMapping AxisMap;									// Retro 31Dec2003
		lbox->SetValue(AxisMap.FlightControlDevice+1);		// Retro 31Dec2003
		lbox->Refresh();
	}
	
	//if (S_joycaps.wCaps & JOYCAPS_HASZ)
	if (IO.AnalogIsUsed(AXIS_THROTTLE))	// Retro 31Dec2003
	{
		button=(C_Button *)win->FindControl(THROTTLE_CHECK);
		if(button != NULL)
		{
			button->SetFlagBitOn(C_BIT_INVISIBLE);
			button->SetState(C_STATE_1);
			button->Refresh();
		}
	}
	
	if (IO.AnalogIsUsed(AXIS_YAW))	// Retro 31Dec2003
	{
		button=(C_Button *)win->FindControl(RUDDER_CHECK);
		if(button != NULL)
		{
			button->SetFlagBitOn(C_BIT_INVISIBLE);
			button->SetState(C_STATE_1);
			button->Refresh();
		}
	}
	
	SetRealism(win);

/*	// M.N. Sky Color Stuff
#if 0
	win=gMainHandler->FindWindow(SETUP_SKY_WIN);
	if (!win)
		return;
	lbox = (C_ListBox *) win->FindControl(SETUP_SKY_COLOR);
	if (lbox) 
	{
		lbox->SetValue(PlayerOptions.skycol);
		lbox->Refresh();
	}
#else
	win=gMainHandler->FindWindow(SETUP_SKY_WIN);
	if(!win) return;
	lbox=(C_ListBox *)win->FindControl(SETUP_SKY_COLOR);
	if (lbox)
	{
		for (int i = 0; i < NumberOfSkyColors; i++)
		{
			lbox->AddItem(i+1,C_TYPE_ITEM,skycolor[i].name);
		}
		lbox->SetValue(PlayerOptions.skycol);
		lbox->Refresh();
	}
#endif
*/

	SetAdvanced();

//	SetSkyColor();
	
	if(!SetupCritSection)
		SetupCritSection = F4CreateCriticalSection("SetupCrit");

	KeyVar.EditKey = FALSE;

}//SetupControls

void SetupRadioCB(long,short hittype,C_Base *control)
{
	int i;
	C_Button *button;
	
	if(hittype != C_TYPE_LMOUSEUP)
		return;
/*
	if(Calibration.calibrating)
	{
		StopCalibrating(control);
	}*/
	
	ready = FALSE;
	
	
	if(SetupViewer)
	{
		F4EnterCriticalSection(SetupCritSection);
		RViewPoint	*viewpt;
		viewpt = SetupViewer->GetVP();
		viewpt->RemoveObject(Smoke);
		delete Smoke;
		Smoke = NULL;
		SetupViewer->Cleanup();
		delete SetupViewer;
		SetupViewer = NULL;
		F4LeaveCriticalSection(SetupCritSection);
		button=(C_Button *)control->Parent_->FindControl(RENDER);
		if(button != NULL)
		{
			button->SetState(C_STATE_0);
		}
	}
	
	InitSoundSetup();

	i=1;
	while(control->GetUserNumber(i))
	{
		control->Parent_->HideCluster(control->GetUserNumber(i));
		i++;
	}
	control->Parent_->UnHideCluster(control->GetUserNumber(0));
	
	control->Parent_->RefreshWindow();
	
	Cluster = control->GetUserNumber(0);

	// Retro 31Dec2003
	// haha.. ok almost no difference to the old one..
	if(control->GetUserNumber(0) == 8004)
	{
		int hasPOV= FALSE;
extern AxisMapping AxisMap;
		if ((gTotalJoy)&&(AxisMap.FlightControlDevice != -1))
		{
			DIDEVCAPS				devcaps;
			devcaps.dwSize = sizeof(DIDEVCAPS);

			ShiAssert(FALSE == F4IsBadReadPtr(gpDIDevice[AxisMap.FlightControlDevice], sizeof *gpDIDevice[AxisMap.FlightControlDevice])); // JPO CTD

			if (gpDIDevice[AxisMap.FlightControlDevice])
			{
				gpDIDevice[AxisMap.FlightControlDevice]->GetCapabilities(&devcaps);
				if(devcaps.dwPOVs > 0)
					hasPOV = TRUE;
			}
		}

		HideKeyStatusLines(control->Parent_);

		const int POVSymbols[] = { LEFT_HAT, RIGHT_HAT, CENTER_HAT, UP_HAT, DOWN_HAT };
		const int POVSymbolCount = sizeof(POVSymbols)/sizeof(int);

		for (int i = 0; i < POVSymbolCount; i++)
		{
			button=(C_Button *)control->Parent_->FindControl(POVSymbols[i]);
			if(button != NULL)
			{
				if (!hasPOV)
					button->SetFlagBitOn(C_BIT_INVISIBLE);
				else
					button->SetFlagBitOff(C_BIT_INVISIBLE);
				button->Refresh();
			}
		}
	}
}//SetupRadioCallback

//JAM 13Oct03
void RestartCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	PostMessage(gMainHandler->GetAppWnd(),FM_EXIT_GAME,0,0);
}
//JAM

static void SaveValues(void)
{
	C_Window	*win;
	C_Button	*button;
	C_Text		*text;
	C_ListBox	*lbox;
	C_Slider	*slider;
	C_EditBox	*ebox;
	
	
	win=gMainHandler->FindWindow(SETUP_WIN);
	
	if(win == NULL)
		return;
	
	lbox=(C_ListBox *)win->FindControl(SET_FLTMOD);
	if(lbox != NULL)
	{
		if((lbox->GetTextID())==SET_FLTMOD_1)
			PlayerOptions.SimFlightModel = FMAccurate;
		else
			PlayerOptions.SimFlightModel = FMSimplified;

		lbox->Refresh();
	}
	
	lbox=(C_ListBox *)win->FindControl(SET_RADAR);
	if(lbox != NULL)
	{
		switch(lbox->GetTextID())
		{
			// M.N. full realism mode added
		case SET_RADAR_0:
			PlayerOptions.SimAvionicsType = ATRealisticAV;
			break;
		case SET_RADAR_1:
			PlayerOptions.SimAvionicsType = ATRealistic;
			break;
		case SET_RADAR_2:
			PlayerOptions.SimAvionicsType = ATSimplified;
			break;
		case SET_RADAR_3:
			PlayerOptions.SimAvionicsType = ATEasy;
			break;
		}
		lbox->Refresh();
	}
	
	lbox=(C_ListBox *)win->FindControl(SET_WEAPEFF);
	if(lbox != NULL)
	{
		switch(lbox->GetTextID())
		{
		case SET_WEAPEFF_1:
			PlayerOptions.SimWeaponEffect = WEAccurate;
			break;
		case SET_WEAPEFF_2:
			PlayerOptions.SimWeaponEffect = WEEnhanced;
			break;
		case SET_WEAPEFF_3:
			PlayerOptions.SimWeaponEffect = WEExaggerated;
			break;
		}
		lbox->Refresh();
	}
	
	lbox=(C_ListBox *)win->FindControl(SET_AUTOPILOT);
	if(lbox != NULL)
	{
		switch(lbox->GetTextID())
		{
		case SET_AUTO_1:
			PlayerOptions.SimAutopilotType = APNormal;
			break;
		case SET_AUTO_2:
			PlayerOptions.SimAutopilotType = APEnhanced;
			break;
		case SET_AUTO_3:
			PlayerOptions.SimAutopilotType = APIntelligent;
			break;
		}
		lbox->Refresh();
	}

	lbox=(C_ListBox *)win->FindControl(SET_PADLOCK);
	if(lbox != NULL)
	{
		switch(lbox->GetTextID())
		{
		case SET_PADLOCK_4:
			PlayerOptions.SimPadlockMode = PDDisabled;
			break;
		case SET_PADLOCK_1:
			PlayerOptions.SimPadlockMode = PDRealistic;
			break;
		case SET_PADLOCK_2:
			PlayerOptions.SimPadlockMode = PDEnhanced;
			break;
		//case SET_PADLOCK_3:
		//	PlayerOptions.SimPadlockMode = PDSuper;
		//	break;
		}
		lbox->Refresh();
	}
	
	lbox=(C_ListBox *)win->FindControl(SET_REFUELING);
	if(lbox != NULL)
	{
		switch(lbox->GetTextID())
		{
		case SET_REFUEL_1:
			PlayerOptions.SimAirRefuelingMode = ARRealistic;
			break;
		case SET_REFUEL_2:
			PlayerOptions.SimAirRefuelingMode = ARModerated;
			break;
		case SET_REFUEL_3:
			PlayerOptions.SimAirRefuelingMode = ARSimplistic;
			break;
		}
		lbox->Refresh();
	}

	lbox=(C_ListBox *)win->FindControl(SET_CANOPY_CUE);
	if(lbox != NULL)
	{
		switch(lbox->GetTextID())
		{
		case CUE_NONE:
			PlayerOptions.SimVisualCueMode = VCNone;
			break;
		case CUE_LIFT_LINE:
			PlayerOptions.SimVisualCueMode = VCLiftLine;
			break;
		case CUE_REFLECTION_MAP:
			PlayerOptions.SimVisualCueMode = VCReflection;
			break;
		case CUE_BOTH:
			PlayerOptions.SimVisualCueMode = VCBoth;
			break;
		}
		lbox->Refresh();
	}
	
	lbox=(C_ListBox *)win->FindControl(SET_VIDEO_CARD);
	if(lbox != NULL)
	{
		DisplayOptions.DispVideoCard = static_cast<uchar>(lbox->GetTextID() - 1);
		lbox->Refresh();
	}
	
	lbox=(C_ListBox *)win->FindControl(SET_VIDEO_DRIVER);
	if(lbox != NULL)
	{
		DisplayOptions.DispVideoDriver = static_cast<uchar>(lbox->GetTextID() - 1);
		lbox->Refresh();
	}

	lbox=(C_ListBox *)win->FindControl(SET_RESOLUTION);
	if(lbox != NULL)
	{
		// OW
		#if 1
		UINT nWidth, nHeight, nDepth;
		FalconDisplay.devmgr.GetMode(DisplayOptions.DispVideoDriver, DisplayOptions.DispVideoCard, lbox->GetTextID(),
			&nWidth, &nHeight, &nDepth);

		DisplayOptions.DispWidth = nWidth;
		DisplayOptions.DispHeight = nHeight;
		DisplayOptions.DispDepth = nDepth;

		ShiAssert(DisplayOptions.DispWidth <= 1600);
		FalconDisplay.SetSimMode(DisplayOptions.DispWidth, DisplayOptions.DispHeight, DisplayOptions.DispDepth);	// OW
		#else
		DisplayOptions.DispWidth = static_cast<short>(lbox->GetTextID());
		DisplayOptions.DispHeight = static_cast<ushort>(FloatToInt32(lbox->GetTextID() * 0.75F));

		ShiAssert(DisplayOptions.DispWidth <= 1600);
		FalconDisplay.SetSimMode(DisplayOptions.DispWidth, DisplayOptions.DispHeight, DisplayOptions.DispDepth);	// OW
		#endif

		lbox->Refresh();
	}

	//JAM 20Nov03
	if( TheCampaign.InMainUI )
	{
		lbox=(C_ListBox *)win->FindControl(SETUP_REALWEATHER);
		if(lbox != NULL)
		{
			PlayerOptions.weatherCondition = lbox->GetTextID()-70207;
			((WeatherClass *)realWeather)->UpdateCondition(PlayerOptions.weatherCondition,true);
			((WeatherClass *)realWeather)->Init(true);
		}
	}
	//JAM

	//THW 2004-01-17
/*	lbox=(C_ListBox *)win->FindControl(SETUP_SEASON);
	if(lbox != NULL)
	{
		//PlayerOptions.Season = lbox->GetTextID()-70312;
		lbox->Refresh();
	}
	//THW
*/
	button=(C_Button *)win->FindControl(SET_ORDNANCE);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1)
			PlayerOptions.SetSimFlag (SIM_UNLIMITED_AMMO);
		else
			PlayerOptions.ClearSimFlag (SIM_UNLIMITED_AMMO);
	}
	
	button=(C_Button *)win->FindControl(SET_FUEL);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1)
			PlayerOptions.SetSimFlag (SIM_UNLIMITED_FUEL);
		else
			PlayerOptions.ClearSimFlag (SIM_UNLIMITED_FUEL);
	}
	
	button=(C_Button *)win->FindControl(SET_CHAFFLARES);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1)
			PlayerOptions.SetSimFlag (SIM_UNLIMITED_CHAFF);
		else
			PlayerOptions.ClearSimFlag (SIM_UNLIMITED_CHAFF);
		
	}
	
	button=(C_Button *)win->FindControl(SET_COLLISIONS);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1)
			PlayerOptions.SetSimFlag (SIM_NO_COLLISIONS);
		else
			PlayerOptions.ClearSimFlag (SIM_NO_COLLISIONS);
	}
	
	button=(C_Button *)win->FindControl(SET_BLACKOUT);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1)
			PlayerOptions.SetSimFlag (SIM_NO_BLACKOUT);
		else
			PlayerOptions.ClearSimFlag (SIM_NO_BLACKOUT);
	}
	
	
	button=(C_Button *)win->FindControl(SET_IDTAGS);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1)
			PlayerOptions.SetSimFlag (SIM_NAMETAGS);
		else
			PlayerOptions.ClearSimFlag (SIM_NAMETAGS);			
	}
	
	button=(C_Button *)win->FindControl(SET_BULLSEYE_CALLS);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1)
			PlayerOptions.SetSimFlag(SIM_BULLSEYE_CALLS);
		else
			PlayerOptions.ClearSimFlag(SIM_BULLSEYE_CALLS);
	}

	button=(C_Button *)win->FindControl(SET_INVULNERABILITY); //should be SET_INVULNERABLITY
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1)
			PlayerOptions.SetSimFlag(SIM_INVULNERABLE);
		else
			PlayerOptions.ClearSimFlag(SIM_INVULNERABLE);
	}

	// Retro 25Dec2003
	button=(C_Button *)win->FindControl(SETUP_SIM_INFOBAR);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1)
			PlayerOptions.SetInfoBar(true);
		else
			PlayerOptions.SetInfoBar(false);
	}
	// ..ends

	ebox = (C_EditBox *)win->FindControl(ACMI_FILE_SIZE);
	if(ebox)
	{
		PlayerOptions.ACMIFileSize = ebox->GetInteger();
	}
	
	button=(C_Button *)win->FindControl(AUTO_SCALE);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1)
			PlayerOptions.SetObjFlag(DISP_OBJ_DYN_SCALING);
		else
			PlayerOptions.ClearObjFlag(DISP_OBJ_DYN_SCALING);
	}

	button=(C_Button *)win->FindControl(GOUROUD);//GOUROUD
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1)
			PlayerOptions.SetDispFlag(DISP_GOURAUD);
		else
			PlayerOptions.ClearDispFlag(DISP_GOURAUD);
	}
	
	button=(C_Button *)win->FindControl(HAZING);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1)
			PlayerOptions.SetDispFlag(DISP_HAZING);
		else
			PlayerOptions.ClearDispFlag(DISP_HAZING);
	}
	
	//JAM 07Dec03
	button=(C_Button *)win->FindControl(SETUP_REALWEATHER_SHADOWS);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1)
			PlayerOptions.SetDispFlag(DISP_SHADOWS);
		else
			PlayerOptions.ClearDispFlag(DISP_SHADOWS);
	}

	button=(C_Button *)win->FindControl(SETUP_SPECULAR_LIGHTING);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1)
			DisplayOptions.bSpecularLighting = TRUE;
		else
			DisplayOptions.bSpecularLighting = FALSE;
	}
	
/*	button=(C_Button *)win->FindControl(BILINEAR_FILTERING);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1)
			PlayerOptions.SetDispFlag(DISP_BILINEAR);
		else
			PlayerOptions.ClearDispFlag(DISP_BILINEAR);
	}
*/	
/*	button=(C_Button *)win->FindControl(OBJECT_TEXTURES);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1)
			PlayerOptions.ObjFlags |= DISP_OBJ_TEXTURES;
		else
			PlayerOptions.ObjFlags &= ~DISP_OBJ_TEXTURES;
	}
*/	
	slider=(C_Slider *)win->FindControl(OBJECT_DETAIL);
	if(slider != NULL)
	{
		PlayerOptions.ObjDetailLevel = ((float)slider->GetSliderPos()/(slider->GetSliderMax()-slider->GetSliderMin())*1.5f*GraphicSettingMult + 0.5f);
	}

	slider=(C_Slider *)win->FindControl(SFX_LEVEL);
	if(slider != NULL)
	{
		PlayerOptions.SfxLevel = ((float)slider->GetSliderPos()/(slider->GetSliderMax()-slider->GetSliderMin()) * 4.0f + 1.0F);
	}

	slider=(C_Slider *)win->FindControl(PLAYER_BUBBLE_SLIDER);
	if(slider != NULL)
	{
		PlayerOptions.PlayerBubble = ((float)slider->GetSliderPos()/(slider->GetSliderMax()-slider->GetSliderMin())*1.5f*GraphicSettingMult + 0.5f);
		FalconLocalSession->SetBubbleRatio( PlayerOptions.PlayerBubble );
	}

	slider=(C_Slider *)win->FindControl(DISAGG_LEVEL);
	if(slider != NULL)
	{
		PlayerOptions.BldDeaggLevel = FloatToInt32((float)slider->GetSliderPos()/(slider->GetSliderMax()-slider->GetSliderMin()) * 5 + 0.5F);
		PlayerOptions.ObjDeaggLevel = FloatToInt32((float)slider->GetSliderPos()/(slider->GetSliderMax()-slider->GetSliderMin()) * 100 + 0.5F);
	}
	
	slider=(C_Slider *)win->FindControl(VEHICLE_SIZE);
	if(slider != NULL)
	{
		PlayerOptions.ObjMagnification = static_cast<float>(FloatToInt32( (float)slider->GetSliderPos()/(float)( slider->GetSliderMax() - slider->GetSliderMin() ) * 4.0F + 1.0F ));
	}
	
/*	slider=(C_Slider *)win->FindControl(TEXTURE_DISTANCE);
	if(slider != NULL)
	{
		PlayerOptions.DispTextureLevel = FloatToInt32((float)slider->GetSliderPos()/(slider->GetSliderMax()-slider->GetSliderMin()) * 4.0F + 0.5F);
	}
*/
	slider=(C_Slider *)win->FindControl(TERRAIN_DETAIL);
	if(slider != NULL)
	{
		int step;
		step = (slider->GetSliderMax()-slider->GetSliderMin())/(6*GraphicSettingMult);
	
		if(slider->GetSliderPos() > 2*step)
		{
			PlayerOptions.DispTerrainDist = (40.0f + ( ((float)slider->GetSliderPos())/step - 2 ) * 10.0f);
			PlayerOptions.DispMaxTerrainLevel = 0;
		}
		else 
		{
			PlayerOptions.DispTerrainDist = 40.0f;
			PlayerOptions.DispMaxTerrainLevel = FloatToInt32( max(0.0F, 2.0F - slider->GetSliderPos()/step) + 0.5F);
		}
	}

	text=(C_Text *)win->FindControl(CAL_TEXT);
	if(text != NULL)
		text->SetText("");
	
	button = (C_Button *) win->FindControl(PLAYERVOICE);
	if(button) PlayerOptions.PlayerRadioVoice = (button->GetState() == C_STATE_1);

	button = (C_Button *) win->FindControl(UICOMMS);
	if(button) PlayerOptions.UIComms = (button->GetState() == C_STATE_1);

	double pos,range;
	int volume, i;
	
	for(i=0;i<17;i+=2)
	{
		slider=(C_Slider *)win->FindControl(ENGINE_VOLUME + i);
		if ( slider )
		{
			pos = slider->GetSliderPos();
			range = slider->GetSliderMax()-slider->GetSliderMin();
			
			pos =   (1.0F - pos / range);
			volume = (int)(pos * pos * (SND_RNG));
			
			PlayerOptions.GroupVol[i/2] = volume;
			F4SetGroupVolume(i/2,volume);
		}
	}

	PlayerOptions.Realism = GetRealism(win)/100.0f;

	win=gMainHandler->FindWindow(SETUP_ADVANCED_WIN);
	if(!win) return;

	//JAM 28Oct03
	button = (C_Button *)win->FindControl(SETUP_ADVANCED_ANISOTROPIC_FILTERING);
	if(button) DisplayOptions.bAnisotropicFiltering = button->GetState() == C_STATE_1;

	button = (C_Button *)win->FindControl(SETUP_ADVANCED_RENDER_2DCOCKPIT);
	if(button) DisplayOptions.bRender2DCockpit = button->GetState() == C_STATE_1;

	button = (C_Button *)win->FindControl(SETUP_ADVANCED_SCREEN_COORD_BIAS_FIX);
	if(button) DisplayOptions.bScreenCoordinateBiasFix = button->GetState() == C_STATE_1;		//Wombat778 4-01-04

//	button = (C_Button *)win->FindControl(SETUP_ADVANCED_SPECULAR_LIGHTING);
//	if(button) DisplayOptions.bSpecularLighting = button->GetState() == C_STATE_1;

	button = (C_Button *)win->FindControl(SETUP_ADVANCED_LINEAR_MIPMAP_FILTERING);
	if(button) DisplayOptions.bLinearMipFiltering = button->GetState() == C_STATE_1;

	button = (C_Button *)win->FindControl(SETUP_ADVANCED_MIPMAPPING);
	if(button) DisplayOptions.bMipmapping = button->GetState() == C_STATE_1;

	button = (C_Button *) win->FindControl(SETUP_ADVANCED_RENDER_TO_TEXTURE);
	if(button) DisplayOptions.bRender2Texture = button->GetState() == C_STATE_1;

	//========================================
	// FRB - Force Z-Buffering
     DisplayOptions.bZBuffering = TRUE;
	// FRB - Force Specular Lighting
//     DisplayOptions.bSpecularLighting = TRUE;
  // DDS textures only
//		 DisplayOptions.m_texMode = TEX_MODE_DDS;
	//========================================

	
	PlayerOptions.SaveOptions();
	DisplayOptions.SaveOptions();

	SaveKeyMapList(PlayerOptions.keyfile);

	win = gMainHandler->FindWindow(INFO_WIN);
	if(win)
	{
		INFOSetupControls();
	}

}//SaveValues

void ShutdownSetup()
{
	if(Objects)
	{
		delete [] Objects;
		Objects = NULL;
	}
	
	if(Features)
	{
		delete [] Features;
		Features = NULL;
	}
	
	if(SetupViewer)
	{
		ready = FALSE;
		F4EnterCriticalSection(SetupCritSection);

		RViewPoint	*viewpt;
		viewpt = SetupViewer->GetVP();
		viewpt->RemoveObject(Smoke);
		delete Smoke;
		Smoke = NULL;

		SetupViewer->Cleanup();
		delete SetupViewer;
		SetupViewer = NULL;
		F4LeaveCriticalSection(SetupCritSection);
	}

	if(tmpVpoint)
	{
		tmpVpoint->Cleanup();
		tmpVpoint = NULL;
	}
	
	F4EnterCriticalSection(SetupCritSection);
	F4LeaveCriticalSection(SetupCritSection);
	if(SetupCritSection)
	{
		F4DestroyCriticalSection(SetupCritSection);
		SetupCritSection = NULL;
	}
}


void CloseSetupWindowCB(long ID,short hittype,C_Base *control)
{
	C_Button *button;
	
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	ShutdownSetup();
	PlayerOptions.LoadOptions();
	InitSoundSetup();

/*
	if(Calibration.calibrating)
		StopCalibrating(control);*/
	
	button=(C_Button *)control->Parent_->FindControl(RENDER);
	if(button != NULL)
	{
		button->SetState(C_STATE_0);
	}
	
	UpdateKeyMapList( PlayerOptions.keyfile,USE_FILENAME);

	CloseWindowCB(ID,hittype,control);
}//CloseSetupWindowCB


//JAM 27Oct03
void DoSyncWindowCB(long ID,short hittype,C_Base *control)
{
//	C_Window *win;
/*
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	if( DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS )
	{
		TheTextureBank.FlushHandles();
		TheTextureBank.RestoreTexturePool();
		TheTerrTextures.FlushHandles();
		TheFarTextures.FlushHandles();


		win = gMainHandler->FindWindow(SYNC_WIN);
		if( win )
		{
			gMainHandler->ShowWindow(win);
			gMainHandler->WindowToFront(win);
		}
	
		TheTextureBank.SyncDDSTextures();
		TheTerrTextures.SyncDDSTextures();
		TheFarTextures.SyncDDSTextures();
	
		TheTerrTextures.FlushHandles();
		TheFarTextures.FlushHandles();
//			gMainHandler->HideWindow(win);
	}
*/
}
//JAM


void SetupOkCB(long ID,short hittype,C_Base *control)
{
	C_Button *button;
	
	if(hittype != C_TYPE_LMOUSEUP)
		return;
	
	
	SaveValues();
	InitSoundSetup();

	CheckFlyButton();
	ShutdownSetup();

/*
	if(Calibration.calibrating)
		StopCalibrating(control);*/
	
	button=(C_Button *)control->Parent_->FindControl(RENDER);
	if(button != NULL)
	{
		button->SetState(C_STATE_0);
	}
	
	CloseWindowCB(ID,hittype,control);
}//SetupOkCB


//JAM 28Oct03
void AApplySetupCB(long ID,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;
	
	SaveValues();
	CloseWindowCB(ID,hittype,control);
}
//JAM


void ApplySetupCB(long,short hittype,C_Base *)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;
	
	SaveValues();
	CheckFlyButton();
	
}//ApplySetupCB



void CancelSetupCB(long ID,short hittype,C_Base *control)
{
	C_Button *button;
	
	if(hittype != C_TYPE_LMOUSEUP)
		return;
	
	ShutdownSetup();
	PlayerOptions.LoadOptions();
	InitSoundSetup();

	/*
	if(Calibration.calibrating)
		StopCalibrating(control);*/
	
	button=(C_Button *)control->Parent_->FindControl(RENDER);
	if(button != NULL)
	{
		button->SetState(C_STATE_0);
	}


	UpdateKeyMapList( PlayerOptions.keyfile, USE_FILENAME);

	CloseWindowCB(ID,hittype,control);
} //CancelSetupCB


static void HookupSetupControls(long ID)
{
	C_Window	*win;
	C_Button	*button;
	C_Slider	*slider;
	C_ListBox	*listbox;
	
	win=gMainHandler->FindWindow(ID);
	
	if(win == NULL)
		return;
	
// Help GUIDE thing
	button=(C_Button*)win->FindControl(UI_HELP_GUIDE);
	if(button)
		button->SetCallback(UI_Help_Guide_CB);

	// Hook up IDs here
	// Hook up Close Button
	button=(C_Button *)win->FindControl(CLOSE_WINDOW);
	if(button != NULL)
		button->SetCallback(CloseSetupWindowCB);

	//JAM 24Oct03
//	button=(C_Button *)win->FindControl(DO_SYNC);
//	if(button != NULL)
//		button->SetCallback(DoSyncWindowCB);

	button=(C_Button *)win->FindControl(OK);
	if(button != NULL)
		button->SetCallback(SetupOkCB);
	
	button=(C_Button *)win->FindControl(APPLY);
	if(button != NULL)
		button->SetCallback(ApplySetupCB);
	
	button=(C_Button *)win->FindControl(CANCEL);
	if(button != NULL)
		button->SetCallback(CancelSetupCB);
	
	button=(C_Button *)win->FindControl(SET_LOGBOOK);
	if(button != NULL)
		button->SetCallback(SetupOpenLogBookCB);
	
	button=(C_Button *)win->FindControl(SIM_TAB);
	if(button != NULL)
		button->SetCallback(SetupRadioCB);
	
	button=(C_Button *)win->FindControl(GRAPHICS_TAB);
	if(button != NULL)
		button->SetCallback(SetupRadioCB);
	
	button=(C_Button *)win->FindControl(SOUND_TAB);
	if(button != NULL)
		button->SetCallback(SetupRadioCB);
	
	button=(C_Button *)win->FindControl(CONTROLLERS_TAB);
	if(button != NULL)
		button->SetCallback(SetupRadioCB);

	
	//Sim Tab
	listbox = (C_ListBox *)win->FindControl(SET_SKILL);
	if(listbox != NULL)
		listbox->SetCallback(SetSkillCB);

	listbox = (C_ListBox *)win->FindControl(SET_FLTMOD);
	if(listbox != NULL)
		listbox->SetCallback(SimControlCB);

	listbox = (C_ListBox *)win->FindControl(SET_RADAR);
	if(listbox != NULL)
		listbox->SetCallback(SimControlCB);

	listbox = (C_ListBox *)win->FindControl(SET_WEAPEFF);
	if(listbox != NULL)
		listbox->SetCallback(SimControlCB);

	listbox = (C_ListBox *)win->FindControl(SET_AUTOPILOT);
	if(listbox != NULL)
		listbox->SetCallback(SimControlCB);

	listbox = (C_ListBox *)win->FindControl(SET_REFUELING);
	if(listbox != NULL)
		listbox->SetCallback(SimControlCB);

	listbox = (C_ListBox *)win->FindControl(SET_PADLOCK);
	if(listbox != NULL)
		listbox->SetCallback(SimControlCB);

	button=(C_Button *)win->FindControl(SET_INVULNERABILITY);
	if(button != NULL)
		button->SetCallback(SimControlCB);

	button=(C_Button *)win->FindControl(SET_FUEL);
	if(button != NULL)
		button->SetCallback(SimControlCB);

	button=(C_Button *)win->FindControl(SET_CHAFFLARES);
	if(button != NULL)
		button->SetCallback(SimControlCB);

	button=(C_Button *)win->FindControl(SET_COLLISIONS);
	if(button != NULL)
		button->SetCallback(SimControlCB);

	button=(C_Button *)win->FindControl(SET_BLACKOUT);
	if(button != NULL)
		button->SetCallback(SimControlCB);

	button=(C_Button *)win->FindControl(SET_IDTAGS);
	if(button != NULL)
		button->SetCallback(SimControlCB);


	//Controllers Tab
	
	//button=(C_Button *)win->FindControl(SET_DEFAULTS);
	//if(button != NULL)
	//	button->SetCallback(SetKeyDefaultCB);

	button=(C_Button *)win->FindControl(CALIBRATE);
	if(button != NULL)
		button->SetCallback(RecenterJoystickCB);

	button=(C_Button *)win->FindControl(SET_AB_DETENT);
	if(button != NULL)
		button->SetCallback(SetABDetentCB);
	 

	button=(C_Button *)win->FindControl(KEYMAP_SAVE);
	if(button != NULL)
		button->SetCallback(SaveKeyButtonCB);

	button=(C_Button *)win->FindControl(KEYMAP_LOAD);
	if(button != NULL)
		button->SetCallback(LoadKeyButtonCB);

	win->SetKBCallback(KeystrokeCB);

	listbox = (C_ListBox *)win->FindControl(70137);//JOYSTICK_SELECT
	if(listbox != NULL)
		listbox->SetCallback(ControllerSelectCB);

	// Retro 31Dec2003 - callback function for the new advanced controls button
	button=(C_Button *)win->FindControl(SETUP_CONTROL_ADVANCED);
	if(button != NULL)
		button->SetCallback(AdvancedControlCB);

	//Sound Tab

	button=(C_Button*)win->FindControl(ENGINE_SND);
	if(button)
		button->SetCallback(TestButtonCB);
	
	button=(C_Button*)win->FindControl(SIDEWINDER_SND);
	if(button)
		button->SetCallback(TestButtonCB);
	
	button=(C_Button*)win->FindControl(RWR_SND);
	if(button)
		button->SetCallback(TestButtonCB);
	
	button=(C_Button*)win->FindControl(COCKPIT_SND);
	if(button)
		button->SetCallback(TestButtonCB);
	
	button=(C_Button*)win->FindControl(COM1_SND);
	if(button)
		button->SetCallback(TestButtonCB);
	
	button=(C_Button*)win->FindControl(COM2_SND);
	if(button)
		button->SetCallback(TestButtonCB);
	
	button=(C_Button*)win->FindControl(SOUNDFX_SND);
	if(button)
		button->SetCallback(TestButtonCB);
	
	button=(C_Button*)win->FindControl(UISOUNDFX_SND);
	if(button)
		button->SetCallback(TestButtonCB);

		// M.N.
	button=(C_Button *)win->FindControl(PLAYERVOICE);
	if(button != NULL)
		button->SetCallback(TogglePlayerVoiceCB);

	button=(C_Button *)win->FindControl(UICOMMS);
	if(button != NULL)
		button->SetCallback(ToggleUICommsCB);

	slider=(C_Slider*)win->FindControl(ENGINE_VOLUME);
	if(slider)
		slider->SetCallback(SoundSliderCB);

	slider=(C_Slider*)win->FindControl(SIDEWINDER_VOLUME);
	if(slider)
		slider->SetCallback(SoundSliderCB);

	slider=(C_Slider*)win->FindControl(RWR_VOLUME);
	if(slider)
		slider->SetCallback(SoundSliderCB);

	slider=(C_Slider*)win->FindControl(COCKPIT_VOLUME);
	if(slider)
		slider->SetCallback(SoundSliderCB);

	slider=(C_Slider*)win->FindControl(COM1_VOLUME);
	if(slider)
		slider->SetCallback(SoundSliderCB);

	slider=(C_Slider*)win->FindControl(COM2_VOLUME);
	if(slider)
		slider->SetCallback(SoundSliderCB);

	slider=(C_Slider*)win->FindControl(SOUNDFX_VOLUME);
	if(slider)
		slider->SetCallback(SoundSliderCB);

	slider=(C_Slider*)win->FindControl(UISOUNDFX_VOLUME);
	if(slider)
		slider->SetCallback(SoundSliderCB);

	slider=(C_Slider*)win->FindControl(MUSIC_VOLUME);
	if(slider)
		slider->SetCallback(SoundSliderCB);

	slider=(C_Slider*)win->FindControl(MASTER_VOLUME);
	if(slider)
		slider->SetCallback(SoundSliderCB);

	//Graphics Tab
	
	if(ID == SETUP_WIN)
	{
		C_TimerHook *tmr;
		
		tmr=new C_TimerHook;
		tmr->Setup(C_DONT_CARE,C_TYPE_NORMAL);
		tmr->SetClient(2);
		tmr->SetXY(win->ClientArea_[2].left,win->ClientArea_[2].top);
		tmr->SetW(win->ClientArea_[2].right - win->ClientArea_[2].left);
		tmr->SetH(win->ClientArea_[2].bottom - win->ClientArea_[2].top);
		tmr->SetUpdateCallback(STPViewTimerCB);
		tmr->SetDrawCallback(STPDisplayCB);
		tmr->SetFlagBitOff(C_BIT_TIMER);
		tmr->SetCluster(8002);
		//	tmr->SetUserNumber(1,2);
		tmr->SetReady(1);
		win->AddControl(tmr);
		win->SetDragCallback(STPMoveRendererCB);
	}

	listbox = (C_ListBox *)win->FindControl(SET_VIDEO_CARD);
	if(listbox != NULL)
		listbox->SetCallback(VideoCardCB);

	listbox = (C_ListBox *)win->FindControl(SET_VIDEO_DRIVER);
	if(listbox != NULL)
		listbox->SetCallback(VideoDriverCB);

	listbox = (C_ListBox *)win->FindControl(SET_RESOLUTION);
	if(listbox != NULL)
		listbox->SetCallback(ResolutionCB);
	
	//JAM 20Nov03
	listbox = (C_ListBox *)win->FindControl(SETUP_REALWEATHER);
	if(listbox != NULL)
		listbox->SetCallback(RealWeatherCB);
	//JAM
/*
	//THW 2004-01-17
	listbox = (C_ListBox *)win->FindControl(SETUP_SEASON);
	if(listbox != NULL)
		listbox->SetCallback(SeasonCB);
	//THW
*/
	button=(C_Button *)win->FindControl(SET_GRAPHICS_DEFAULTS);
	if(button != NULL)
	{
		button->SetCallback(GraphicsDefaultsCB);
	}

	// OW
	button=(C_Button *)win->FindControl(SET_GRAPHICS_ADVANCED);
	if(button != NULL)
	{
		button->SetCallback(AdvancedCB);
	}

	// JPO
	button = (C_Button *)win->FindControl(ADVANCED_GAME_OPTIONS);
	if (button != NULL) 
	{
	    button->SetCallback(AdvancedGameCB);
	}

	// M.N.
/*	button=(C_Button *)win->FindControl(SET_SKY_COLOR);
	if(button != NULL)
	{
		button->SetCallback(SkyColorCB);
	}
*/
	button=(C_Button *)win->FindControl(RENDER);
	if(button != NULL)
	{
		button->SetCallback(RenderViewCB);
	}
	
	button=(C_Button *)win->FindControl(AUTO_SCALE);
	if(button != NULL)
	{
		button->SetCallback(ScalingCB);
	}

	// Retro 25Dec2003
	button=(C_Button*)win->FindControl(SETUP_SIM_SUBTITLES);
	if (button != NULL)
	{
		button->SetCallback(SubTitleCB);
	}
	// ..ends

/*	button=(C_Button *)win->FindControl(70136);//GOUROUD
	if(button != NULL)
	{
		button->SetCallback(GouraudCB);
	}
*/	
	button=(C_Button *)win->FindControl(HAZING);
	if(button != NULL)
	{
		button->SetCallback(HazingCB);
	}
	
	button=(C_Button *)win->FindControl(SETUP_REALWEATHER_SHADOWS);
	if(button != NULL)
	{
		button->SetCallback(RealWeatherShadowsCB);
	}

/*	
	button=(C_Button *)win->FindControl(BILINEAR_FILTERING);
	if(button != NULL)
	{
		button->SetCallback(BilinearFilterCB);
	}
*/	
/*	button=(C_Button *)win->FindControl(OBJECT_TEXTURES);
	if(button != NULL)
	{
		button->SetCallback(ObjectTextureCB);
	}
*/	
	slider=(C_Slider *)win->FindControl(DISAGG_LEVEL);
	if(slider != NULL)
	{
		slider->SetCallback(BuildingDetailCB);
	}
	
	slider=(C_Slider *)win->FindControl(OBJECT_DETAIL);
	if(slider != NULL)
	{
		slider->SetCallback(ObjectDetailCB);
	}

	slider=(C_Slider *)win->FindControl(PLAYER_BUBBLE_SLIDER);
	if(slider != NULL)
	{
		slider->SetCallback(PlayerBubbleCB);
	}
	
	slider=(C_Slider *)win->FindControl(VEHICLE_SIZE);
	if(slider != NULL)
	{
		slider->SetCallback(VehicleSizeCB);
	}
	
	slider=(C_Slider *)win->FindControl(TERRAIN_DETAIL);
	if(slider != NULL)
	{
		slider->SetCallback(TerrainDetailCB);
	}

/*	slider=(C_Slider *)win->FindControl(TEXTURE_DISTANCE);
	if(slider != NULL)
	{
		slider->SetCallback(TextureDistanceCB);
	}
*/
	slider=(C_Slider *)win->FindControl(SFX_LEVEL);
	if(slider != NULL)
	{
		slider->SetCallback(SfxLevelCB);
	}

	// OW new stuff
	win=gMainHandler->FindWindow(SETUP_ADVANCED_WIN);
	if(!win) return;

	// disable parent notification for close and cancel button 
	button=(C_Button *)win->FindControl(AAPPLY);
	if(button) button->SetCallback(AApplySetupCB);

	button=(C_Button *)win->FindControl(CANCEL);
	if(button) button->SetCallback(CloseWindowCB);
	// OW end of new stuff


	// M.N. SkyColor stuff
/*	win = gMainHandler->FindWindow(SETUP_SKY_WIN);
	if(!win) return;

	// disable parent notification for close and cancel button
	listbox=(C_ListBox *)win->FindControl(SETUP_SKY_COLOR);
	if(listbox) listbox->SetCallback(SelectSkyColorCB);

	button=(C_Button *)win->FindControl(CANCEL);
	if(button) button->SetCallback(CloseWindowCB);

	button=(C_Button *)win->FindControl(SKY_COLOR_TIME_1);
	if(button) button->SetCallback(SkyColTimeCB);

	button=(C_Button *)win->FindControl(SKY_COLOR_TIME_2);
	if(button) button->SetCallback(SkyColTimeCB);

	button=(C_Button *)win->FindControl(SKY_COLOR_TIME_3);
	if(button) button->SetCallback(SkyColTimeCB);

	button=(C_Button *)win->FindControl(SKY_COLOR_TIME_4);
	if(button) button->SetCallback(SkyColTimeCB);
	// M.N. end SkyColor stuff
*/
	// disable parent notification for close and cancel button

	button=(C_Button *)win->FindControl(CANCEL);
	if(button) button->SetCallback(CloseWindowCB);
}
//HookupSetupControls


