
#ifndef PLAYEROP_H
#define PLAYEROP_H

//#include "soundgroups.h"
//#include "falclib.h"
//#include "ui\include\logbook.h"
#include <tchar.h>
#include "PlayerOpDef.h"
//#include "rules.h"


// =====================
// PlayerOptions Class
// =====================
#define NUM_SOUND_GROUPS 10

class PlayerOptionsClass
{
	private:
	public:
	int DispFlags;							// Display Options
//	int DispTextureLevel;					//0-4 (to what level do we display textures)
	float DispTerrainDist;					//sets max range at which textures are displayed
	int DispMaxTerrainLevel;				//should 0-2 can be up to 4
											
	int		ObjFlags;						// Object Display Options
	float	ObjDetailLevel;					// (.5 - 2) (modifies LOD switching distance)
	float	ObjMagnification;				// 1-5
	int		ObjDeaggLevel;					// 0-100 (Percentage of vehicles deaggregated per unit)
	int		BldDeaggLevel;					// 0-5 (determines which buildings get deaggregated)
	int		ACMIFileSize;					// 1-? MB's (determines the largest size a single acmi tape will be)
	float	SfxLevel;						// 1.0-5.0
	float	PlayerBubble;					// 0.5 - 2 multiplier for player bubble size

	int					SimFlags;			// Sim flags
	FlightModelType		SimFlightModel;		// FlightModelType
	WeaponEffectType	SimWeaponEffect;	// WeaponEffectType
	AvionicsType		SimAvionicsType;	// Avionics Difficulty
	AutopilotModeType   SimAutopilotType;	// AutopilotModeType
	RefuelModeType		SimAirRefuelingMode;// RefuelModeType
	PadlockModeType		SimPadlockMode;		// PadlockModeType
	VisualCueType		SimVisualCueMode;	// VisualCueType

	int GeneralFlags;						// General stuff

	int CampGroundRatio;					// Default force ratio values
	int CampAirRatio;
	int CampAirDefenseRatio;
	int CampNavalRatio;

	int CampEnemyAirExperience;			// 0-4	Green - Ace
	int CampEnemyGroundExperience;		// 0-4	Green - Ace
	int CampEnemyStockpile;				// 0-100 % of max
	int CampFriendlyStockpile;			// 0-100 % of max

	int	  GroupVol[NUM_SOUND_GROUPS];		// Values are 0 to -3600 in dBs

	float Realism;							// stores last realism value saved less the value
											// from UnlimitedAmmo (this is used to modify scores in
											// Instant Action.)
	_TCHAR  keyfile[PL_FNAME_LEN];			// name of keystrokes file to use
	GUID	joystick;						// unique identifier for which joystick to use

	int				SimTaxiFirst;		// if true, start player taxiing instead of on the runway

	public:
	// Important stuff
	    PlayerOptionsClass (void) {};
	void Initialize(void);
	int LoadOptions (char* filename/* = LogBook.OptionsFile()*/);
	int SaveOptions (char* filename/* = LogBook.Callsign()*/);
	void ApplyOptions(void);

	int InCompliance(void *rules);	// returns TRUE if in FULL compliance w/rules
	void ComplyWRules(void *rules);	// forces all settings not in compliance to minimum settings

	// Nifty Access functions
	int GouraudOn (void)									{ return (DispFlags & DISP_GOURAUD) && TRUE; }
	int HazingOn (void)										{ return (DispFlags & DISP_HAZING) && TRUE; }
	int FilteringOn (void)									{ return (DispFlags & DISP_BILINEAR) && TRUE; }
	int PerspectiveOn (void)								{ return (DispFlags & DISP_PERSPECTIVE) && TRUE; }

//	void SetTextureLevel(int level)							{ DispTextureLevel = level;}
//	int TextureLevel (void)									{ return DispTextureLevel; }
	void SetTerrainDistance(float distance)					{ DispTerrainDist = distance;}
	float TerrainDistance (void)							{ return DispTerrainDist; }
	void SetMaxTerrainLevel(int level)						{ DispMaxTerrainLevel = level;}
	int MaxTerrainLevel (void)								{ return DispMaxTerrainLevel; }

	int ObjectDynScalingOn (void)							{ return (ObjFlags & DISP_OBJ_DYN_SCALING) && TRUE; }
//	int ObjectTexturesOn (void)								{ return (ObjFlags & DISP_OBJ_TEXTURES) && TRUE; }
//	int ObjectShadingOn (void)								{ return (ObjFlags & DISP_OBJ_SHADING) && TRUE; }
	float ObjectDetailLevel (void)							{ return ObjDetailLevel; }
	float ObjectMagnification (void)						{ return ObjMagnification; }
	float BubbleRatio(void)									{ return PlayerBubble;}
	int ObjectDeaggLevel (void)								{ return ObjDeaggLevel; }
	int BuildingDeaggLevel (void)							{ return BldDeaggLevel; }
	float SfxDetailLevel (void)								{ return SfxLevel; }
	int AcmiFileSize (void)									{ return ACMIFileSize; }

	int GetFlightModelType (void)							{ return SimFlightModel; }
	int GetWeaponEffectiveness (void)						{ return SimWeaponEffect; }
	int GetAvionicsType (void)								{ return SimAvionicsType; }
	int GetAutopilotMode (void)								{ return SimAutopilotType; }
	int GetRefuelingMode (void)								{ return SimAirRefuelingMode; };
	int GetPadlockMode (void)								{ return SimPadlockMode; };
	int GetVisualCueMode (void)								{ return SimVisualCueMode; };
	int AutoTargetingOn (void)								{ return (SimFlags & SIM_AUTO_TARGET) && TRUE; }
	int BlackoutOn (void)									{ return !(SimFlags & SIM_NO_BLACKOUT) && TRUE; }
	int NoBlackout (void)									{ return (SimFlags & SIM_NO_BLACKOUT) && TRUE; }
	int UnlimitedFuel (void)								{ return (SimFlags & SIM_UNLIMITED_FUEL) && TRUE; }
	int UnlimitedAmmo (void)								{ return (SimFlags & SIM_UNLIMITED_AMMO) && TRUE; }
	int UnlimitedChaff (void)								{ return (SimFlags & SIM_UNLIMITED_CHAFF) && TRUE; }
	int CollisionsOn (void)									{ return !(SimFlags & SIM_NO_COLLISIONS) && TRUE; }
	int NoCollisions (void)									{ return (SimFlags & SIM_NO_COLLISIONS) && TRUE; }
	int NameTagsOn (void)									{ return (SimFlags & SIM_NAMETAGS) && TRUE; }
	int LiftLineOn (void)									{ return (SimFlags & SIM_LIFTLINE_CUE) && TRUE; }
	int BullseyeOn (void)									{ return (SimFlags & SIM_BULLSEYE_CALLS) && TRUE; }
	int	InvulnerableOn(void)								{ return (SimFlags & SIM_INVULNERABLE) && TRUE; }

	int WeatherOn (void)									{ return !(GeneralFlags & GEN_NO_WEATHER); }
	int MFDTerrainOn (void)									{ return (GeneralFlags & GEN_MFD_TERRAIN) && TRUE; }
	int HawkeyeTerrainOn (void)								{ return (GeneralFlags & GEN_HAWKEYE_TERRAIN) && TRUE; }
	int PadlockViewOn (void)								{ return (GeneralFlags & GEN_PADLOCK_VIEW) && TRUE; }
	int HawkeyeViewOn (void)								{ return (GeneralFlags & GEN_HAWKEYE_VIEW) && TRUE; }
	int ExternalViewOn (void)								{ return (GeneralFlags & GEN_EXTERNAL_VIEW) && TRUE; }

	int CampaignGroundRatio (void)							{ return CampGroundRatio; }
	int CampaignAirRatio (void)								{ return CampAirRatio; }
	int CampaignAirDefenseRatio (void)						{ return CampAirDefenseRatio; }
	int CampaignNavalRatio (void)							{ return CampNavalRatio; }

	int CampaignEnemyAirExperience (void)					{ return CampEnemyAirExperience; }
	int CampaignEnemyGroundExperience (void)				{ return CampEnemyGroundExperience; }
	int CampaignEnemyStockpile (void)						{ return CampEnemyStockpile; }		
	int CampaignFriendlyStockpile (void)					{ return CampFriendlyStockpile; }

	// Setter functions
	void SetSimFlag (int flag)								{SimFlags |= flag;};
	void ClearSimFlag (int flag)							{SimFlags &= ~flag;};

	void SetDispFlag (int flag)								{DispFlags |= flag;};
	void ClearDispFlag (int flag)							{DispFlags &= ~flag;};

	void SetObjFlag (int flag)								{ObjFlags |= flag;};
	void ClearObjFlag (int flag)							{ObjFlags &= ~flag;};

	void SetGenFlag (int flag)								{GeneralFlags |= flag;};
	void ClearGenFlag (int flag)							{GeneralFlags &= ~flag;};

	void SetTaxiFlag(int flag)								{ SimTaxiFirst=flag; }
	int  GetTaxiFlag()										{ return(SimTaxiFirst); }

	void SetKeyFile(_TCHAR *fname)							{_tcscpy(keyfile,fname);}
	_TCHAR *GetKeyfile(void)								{return keyfile;}

	void SetJoystick (GUID newID)							{ joystick=newID; }
	GUID GetJoystick (void)									{ return joystick; }

	void SetCampEnemyAirExperience(int exp)					{ CampEnemyAirExperience=exp; }
	void SetCampEnemyGroundExperience(int exp)				{ CampEnemyGroundExperience=exp; }

	void SetCampGroundRatio (int ratio)						{ CampGroundRatio=ratio; }
	void SetCampAirRatio (int ratio)						{ CampAirRatio=ratio; }
	void SetCampAirDefenseRatio (int ratio)					{ CampAirDefenseRatio=ratio; }
	void SetCampNavalRatio (int ratio)						{ CampNavalRatio=ratio; }
};	

// ==================================
// Our global player options instance
// ==================================

extern PlayerOptionsClass PlayerOptions;

// =====================
// Other functions
// =====================

#endif
