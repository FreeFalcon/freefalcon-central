
#include <stdio.h>
#include <tchar.h>
#include "cmpglobl.h"
#include "PlayerOp.h"
#include "f4find.h"
//temporary until logbook is working
#include "ui/include/logbook.h"
#include "falcsess.h"
#include "dispopts.h"
#include "fsound.h"
#include "falcsnd/psound.h"
#include "ui95/chandler.h"
#include "ui/include/cmusic.h"
#include "soundgroups.h"
#include "falcsnd/VoiceManager.h"
#include "sim/include/sinput.h"
#include "Graphics/Include/matrix.h"
#include "Weather.h"
#include "Campaign/include/Cmpclass.h"

// OW
extern bool g_bHas3DNow;
extern bool g_bRealisticAvionics; // M.N.
//extern bool g_bClickablePitModeDefault; //Wombat778 

// ==================================
// Our global player options instance
// ==================================

PlayerOptionsClass PlayerOptions;

// =====================
// PlayerOptions Class
// =====================

PlayerOptionsClass::PlayerOptionsClass (void)
{
	Initialize();
}

void PlayerOptionsClass::Initialize(void)
{
// JB 011124 Start
/*
	DispFlags = DISP_HAZING|DISP_GOURAUD;		// Display Options
	DispTextureLevel = 4;			//0-4
	DispTerrainDist = 64.0f;		// sets ranges at which texture sets are switched
	DispMaxTerrainLevel = 0;		//should be 0-2 can be up to 4

	ObjFlags = DISP_OBJ_TEXTURES;	// Object Display Options
	SfxLevel = 4.0f;				// 0.0 to 5.0f
	ObjDetailLevel = 1;				// (.5 - 2) (modifies LOD switching distance)
	ObjMagnification = 1;			// 1-9
// 2001-11-09 M.N. Changed from 60 to 100 % = Slider setting 6 (Realism patch)
	ObjDeaggLevel = 100;			// 0-100 (Percentage of vehicles deaggregated per unit)
	BldDeaggLevel = 5;				// 0-5 (determines which buildings get deaggregated)
	PlayerBubble = 1.0F;			// .5 - 2 (ratio by which to multiply player bubble size)

	ACMIFileSize = 5;

	SimFlags =  SIM_NO_BLACKOUT | SIM_UNLIMITED_CHAFF | SIM_NAMETAGS | SIM_UNLIMITED_FUEL;		// Sim flags
	SimFlightModel = FMSimplified;			// Flight model type
	SimWeaponEffect = WEExaggerated;
	SimAvionicsType = ATEasy;
	SimAutopilotType = APEnhanced;
	GeneralFlags = GEN_RULES_FLAGS;			// General stuff
*/
	//JAM 07Dec03
	DispFlags = DISP_HAZING|DISP_GOURAUD|DISP_SHADOWS|DISP_BILINEAR;		// Display Options
//	DispTextureLevel = 4;			//0-4
	DispTerrainDist = 80.0f;		// sets ranges at which texture sets are switched
	DispMaxTerrainLevel = 0;		//should be 0-2 can be up to 4

	ObjFlags = DISP_OBJ_TEXTURES;	// Object Display Options
	SfxLevel = 5.0f;				// 0.0 to 5.0f
	ObjDetailLevel = 2;				// (.5 - 2) (modifies LOD switching distance)
	ObjMagnification = 1;			// 1-9
// 2001-11-09 M.N. Changed from 60 to 100 % = Slider setting 6 (Realism patch)
	ObjDeaggLevel = 100;			// 0-100 (Percentage of vehicles deaggregated per unit)
	BldDeaggLevel = 5;				// 0-5 (determines which buildings get deaggregated)
	PlayerBubble = 1.f;			// .5 - 2 (ratio by which to multiply player bubble size)

	//JAM 19Nov03
	weatherCondition = 1;
	//THW 2004-01-17
	Season = 1;	//Summer

	ACMIFileSize = 5;

	SimFlags =  0;		// Sim flags
	SimFlightModel = FMAccurate;			// Flight model type
	SimWeaponEffect = WEAccurate;
	SimAvionicsType = ATRealisticAV;
	SimAutopilotType = APNormal;
	SimPadlockMode = PDRealistic;

	SimVisualCueMode = VCReflection;
	GeneralFlags = GEN_RULES_FLAGS - GEN_NO_WEATHER;			// General stuff
// JB 011124 End

	SimStartFlags=START_RUNWAY;


	Realism = 0.25f;				//value from 0 to 1

	for(int i = 0; i < NUM_SOUND_GROUPS; i++)
	{
		GroupVol[i] = 0;
	}

	CampGroundRatio = 2;			// Default force ratio values
	CampAirRatio = 2;
	CampAirDefenseRatio = 2;
	CampNavalRatio = 2;

	CampEnemyAirExperience = 0;		// 0-4, Green - Ace
	CampEnemyGroundExperience = 0;	// 0-4, Green - Ace
	CampEnemyStockpile = 100;		// 0-100 % of max
	CampFriendlyStockpile = 100;	// 0-100 % of max

	// M.N. skyfix
	skycol = 1;
	PlayerRadioVoice = true;
	UIComms = true;

	// MLR 12/20/2003 - Whoops these need to be initted...
	SoundExtAttenuation=0;
	SoundFlags=0;

	infoBar = false;				// Retro 25Dec2003
	subTitles = false;				// Retro 25Dec2003
	TrackIR_2d = false;				// Retro 27Dec2003
	TrackIR_3d = false;				// Retro 27Dec2003
	enableFFB = false;				// Retro 27Dec2003
	enableMouseLook = true;			// Retro 28Dec2003
	//enableTouchBuddy = false;       // sfr: touch budy support 
	MouseLookSensitivity = 0.5f;	// Retro 15Jan2004
	MouseWheelSensitivity = 5;		// Retro 17Jan2004
	KeyboardPOVPanningSensitivity = 40;	// Retro 18Jan2004 - 40 degrees per second
	clickablePitMode = false;		// Wombat778 1-22-04
	enableAxisShaping = false;		// Retro 27Jan2004
								


	_tcscpy(keyfile, _T("keystrokes"));
//Retro_dead_15Jan2004	memset(&joystick, 0,sizeof(GUID));
	ApplyOptions();
}


//filename should be callsign of player
int PlayerOptionsClass::LoadOptions (_TCHAR* filename)
{
	size_t		success = 0;
	_TCHAR		path[_MAX_PATH];
	long		size;
	FILE *fp;

	_stprintf(path,_T("%s\\config\\%s.pop"),FalconDataDirectory,filename);
	
	fp = _tfopen(path,_T("rb"));
	if(!fp)
	{
		MonoPrint(_T("Couldn't open %s's player options\n"),filename);
		_stprintf(path,_T("%s\\Config\\default.pop"),FalconDataDirectory);
		fp = _tfopen(path,"rb");
		if(!fp)
		{
			MonoPrint(_T("Couldn't open default player options\n"),filename);
			Initialize();
			return FALSE;
		}
	}
	
	fseek(fp,0,SEEK_END);
	size = ftell(fp);
	fseek(fp,0,SEEK_SET);

// OW - dont break compatibility with 1.03 - 1.08 options

	success = fread(this, 1, size, fp);
	fclose(fp);
	if(success != size)
	{
		MonoPrint(_T("Failed to read %s's player options\n"),filename);
		Initialize();
		return FALSE;
	}

// M.N. If older version, set some new stuff to default values. Next save will update the saved file
	if(size != sizeof(class PlayerOptionsClass))
	{
		PlayerOptions.skycol = 1;
		PlayerOptions.PlayerRadioVoice = true;
	}

/*	if(size != sizeof(class PlayerOptionsClass))
	{
		MonoPrint(_T("%s's player options are in old file format\n"),filename);
		return FALSE;
	}


	success = fread(this, sizeof(class PlayerOptionsClass), 1, fp);
	fclose(fp);
	if(success != 1)
	{
		MonoPrint(_T("Failed to read %s's player options\n"),filename);
		Initialize();
		return FALSE;
	}
*/

	DisplayOptions.LoadOptions();
	FalconLocalSession->SetBubbleRatio(BubbleRatio());
	ApplyOptions();
	// M.N.
	if (SimAvionicsType == ATRealisticAV)
		g_bRealisticAvionics = true;
	else g_bRealisticAvionics = false;

	//JAM 18Nov03
	if(TheCampaign.InMainUI)
	{
		((WeatherClass *)realWeather)->UpdateCondition(weatherCondition,true);
		((WeatherClass *)realWeather)->Init(true);
	}

	return TRUE;
}

void PlayerOptionsClass::ApplyOptions (void)
{
	if(VM)
	{
		F4SetStreamVolume(VM->VoiceHandle(0),GroupVol[COM1_SOUND_GROUP]);
		F4SetStreamVolume(VM->VoiceHandle(1),GroupVol[COM2_SOUND_GROUP]);
	}
	if(gSoundMgr)
		gSoundMgr->SetVolume(GroupVol[UI_SOUND_GROUP]);
	if(gMusic)
		gMusic->SetVolume(GroupVol[MUSIC_SOUND_GROUP]);

	//Wombat778 Hack.  Fix later.
//	clickablePitMode = g_bClickablePitModeDefault;		

	SetMatrixCPUMode(0); //JAM 05Oct03 - Fixme
}

//filename should be callsign of player
int PlayerOptionsClass::SaveOptions (_TCHAR* filename)
{
	FILE		*fp;
	_TCHAR		path[_MAX_PATH];
	size_t		success = 0;
	
	_stprintf(path,_T("%s\\config\\%s.pop"),FalconDataDirectory,filename);
		
	if((fp = _tfopen(path,"wb")) == NULL)
	{
		MonoPrint(_T("Couldn't save player options"));
		return FALSE;
	}

	success = fwrite(this, sizeof(class PlayerOptionsClass), 1, fp);
	fclose(fp);
	if(success == 1)
	{
		LogBook.SetOptionsFile(filename);
		LogBook.SaveData();
	}
	DisplayOptions.SaveOptions();
	// M.N.
	if (SimAvionicsType == ATRealisticAV)
		g_bRealisticAvionics = true;
	else g_bRealisticAvionics = false;
	return TRUE;
}


// =====================
// Other functions
// =====================

// returns TRUE if in FULL compliance w/rules
int PlayerOptionsClass::InCompliance(RulesStruct *rules)
{
	if((SimFlags & SIM_RULES_FLAGS) & ~rules->SimFlags)
		return FALSE;

	if((GeneralFlags & GEN_RULES_FLAGS) & ~rules->GeneralFlags)
		return FALSE;

	if(ObjectMagnification() > rules->ObjMagnification)
		return FALSE;

	if(GetFlightModelType() < rules->SimFlightModel)
		return FALSE;

	if(GetWeaponEffectiveness() < rules->SimWeaponEffect)
		return FALSE;

	if(GetAvionicsType() < rules->SimAvionicsType)
		return FALSE;

	if(GetAutopilotMode() < rules->SimAutopilotType)
		return FALSE;

	if(GetRefuelingMode() > rules->SimAirRefuelingMode)
		return FALSE;

	if(GetPadlockMode() < rules->SimPadlockMode)
		return FALSE;

	return TRUE;
}
void PlayerOptionsClass::ComplyWRules(RulesStruct *rules)
{
	SimFlags &= ~(SIM_RULES_FLAGS & ~rules->SimFlags);

	GeneralFlags &= ~(GEN_RULES_FLAGS & ~rules->GeneralFlags);

	if(ObjectMagnification() > rules->ObjMagnification)
		ObjMagnification = rules->ObjMagnification;

	if(GetFlightModelType() < rules->SimFlightModel)
		SimFlightModel = rules->SimFlightModel;

	if(GetWeaponEffectiveness() < rules->SimWeaponEffect)
		SimWeaponEffect = rules->SimWeaponEffect;

	if(GetAvionicsType() < rules->SimAvionicsType)
		SimAvionicsType = rules->SimAvionicsType;

	if(GetAutopilotMode() < rules->SimAutopilotType)
		SimAutopilotType = rules->SimAutopilotType;

	if(GetRefuelingMode() > rules->SimAirRefuelingMode)
		SimAirRefuelingMode	= rules->SimAirRefuelingMode;

	if(GetPadlockMode() < rules->SimPadlockMode)
		SimPadlockMode	= rules->SimPadlockMode;

}

int CheckNumberPlayers (void)
{
	VuSessionsIterator
		sit(FalconLocalGame);

	FalconSessionEntity
		*session;

	int
		count;

	session = (FalconSessionEntity*) sit.GetFirst ();
	count = 0;

	while (session)
	{
		session = (FalconSessionEntity *) sit.GetNext ();
		count ++;
	}

	return FalconLocalGame->GetRules ()->MaxPlayers - count;
}

