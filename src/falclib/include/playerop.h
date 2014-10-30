#ifndef PLAYEROP_H
#define PLAYEROP_H

#include <tchar.h>
#include "soundgroups.h"
#include "falclib.h"
#include "ui/include/logbook.h"
#include "PlayerOpDef.h"
#include "rules.h"


// =====================
// PlayerOptions Class
// =====================

class PlayerOptionsClass
{
private:
public:
    int DispFlags; // Display Options
    // int DispTextureLevel; //0-4 (to what level do we display textures)
    float DispTerrainDist; //sets max range at which textures are displayed
    int DispMaxTerrainLevel; //should 0-2 can be up to 4

    int ObjFlags; // Object Display Options
    float ObjDetailLevel; // (.5 - 2) (modifies LOD switching distance)
    float ObjMagnification; // 1-5
    int ObjDeaggLevel; // 0-100 (Percentage of vehicles deaggregated per unit)
    int BldDeaggLevel; // 0-5 (determines which buildings get deaggregated)
    int ACMIFileSize; // 1-? MB's (determines the largest size a single acmi tape will be)
    float SfxLevel; // 1.0-5.0
    float PlayerBubble; // 0.5 - 2 multiplier for player bubble size

    int weatherCondition; //JAM 18Nov03
    int Season; //THW 2004-01-17

    int SimFlags; // Sim flags
    FlightModelType SimFlightModel; // FlightModelType
    WeaponEffectType SimWeaponEffect; // WeaponEffectType
    AvionicsType SimAvionicsType; // Avionics Difficulty
    AutopilotModeType   SimAutopilotType; // AutopilotModeType
    RefuelModeType SimAirRefuelingMode;// RefuelModeType
    PadlockModeType SimPadlockMode; // PadlockModeType
    VisualCueType SimVisualCueMode; // VisualCueType

    int GeneralFlags; // General stuff

    int CampGroundRatio; // Default force ratio values
    int CampAirRatio;
    int CampAirDefenseRatio;
    int CampNavalRatio;

    int CampEnemyAirExperience; // 0-4 Green - Ace
    int CampEnemyGroundExperience; // 0-4 Green - Ace
    int CampEnemyStockpile; // 0-100 % of max
    int CampFriendlyStockpile; // 0-100 % of max

    int   GroupVol[NUM_SOUND_GROUPS]; // Values are 0 to -3600 in dBs

    float Realism; // stores last realism value saved less the value
    // from UnlimitedAmmo (this is used to modify scores in
    // Instant Action.)
    _TCHAR  keyfile[PL_FNAME_LEN]; // name of keystrokes file to use
    // Retro_dead 15Jan2004 GUID joystick; // unique identifier for which joystick to use

    enum StartFlag
    {
        START_RUNWAY,
        START_TAXI,
        START_RAMP,
    } SimStartFlags; // Where to start the whole thing (taxi/runway etc)
    enum { RAMP_MINUTES = 20 }; // how long before take off MI increased from 8

    // M.N.
    char skycol; // ID of chosen skyfix (256 should be enough)
    bool PlayerRadioVoice; // Turn on/off all player radio voices
    bool UIComms; // Turn on/off random UI radio chatter

private: // Retro
    bool infoBar; // Retro 25Dec2003
    bool subTitles; // Retro 25Dec2003
    bool TrackIR_2d; // Retro 27Dec2003
    bool TrackIR_3d; // Retro 27Dec2003
    bool enableFFB; // Retro 27Dec2003
    bool enableMouseLook; // Retro 28Dec2003
    float MouseLookSensitivity; // Retro 15Jan2004
    int MouseWheelSensitivity; // Retro 17Jan2004
    int KeyboardPOVPanningSensitivity; // Retro 18Jan2004
    bool clickablePitMode; // Wombat778 1-22-04 moved here from simouse.cpp
    // Retro 22Jan2004 - flag that shows if clickable pit or mouselook is active
    // should also be used to decide if cursor is drawn. Shoul go into clickable pit class
    bool enableAxisShaping; // Retro 27Jan2004

    // sfr: todo make this private. I CARE
public: // Retro
    int SoundFlags; // MLR 12/13/2003 - I could give access functions, but really, who cares?
    int SoundExtAttenuation; // MLR 12/13/2003 -
private:
    // sfr moved here because this has no correct serialization stuff... cant change order
    bool enableTouchBuddy;      // sfr: added touch buddy support
    bool drawMirror;            // sfr: rearview mirror
public:

    // Important stuff
    PlayerOptionsClass(void);
    void Initialize(void);
    int LoadOptions(char* filename = LogBook.OptionsFile());
    int SaveOptions(char* filename = LogBook.Callsign());
    void ApplyOptions(void);

    int InCompliance(RulesStruct *rules); // returns TRUE if in FULL compliance w/rules
    void ComplyWRules(RulesStruct *rules); // forces all settings not in compliance to minimum settings

    // Nifty Access functions
    int GouraudOn(void)
    {
        return (DispFlags bitand DISP_GOURAUD) and TRUE;
    }
    int HazingOn(void)
    {
        return (DispFlags bitand DISP_HAZING) and TRUE;
    }
    int FilteringOn(void)
    {
        return (DispFlags bitand DISP_BILINEAR) and TRUE;
    }
    int PerspectiveOn(void)
    {
        return (DispFlags bitand DISP_PERSPECTIVE) and TRUE;
    }

    //JAM 08Dec03
    int ShadowsOn(void)
    {
        return (DispFlags bitand DISP_SHADOWS) and TRUE;
    }

    // void SetTextureLevel(int level) { DispTextureLevel = level;}
    // int TextureLevel (void) { return DispTextureLevel; }
    void SetTerrainDistance(float distance)
    {
        DispTerrainDist = distance;
    }
    float TerrainDistance(void)
    {
        return DispTerrainDist;
    }
    void SetMaxTerrainLevel(int level)
    {
        DispMaxTerrainLevel = level;
    }
    int MaxTerrainLevel(void)
    {
        return DispMaxTerrainLevel;
    }

    int ObjectDynScalingOn(void)
    {
        return (ObjFlags bitand DISP_OBJ_DYN_SCALING) and TRUE;
    }
    // int ObjectTexturesOn (void) { return (ObjFlags bitand DISP_OBJ_TEXTURES) and TRUE; }
    // int ObjectShadingOn (void) { return (ObjFlags bitand DISP_OBJ_SHADING) and TRUE; }
    float ObjectDetailLevel(void)
    {
        return ObjDetailLevel;
    }
    float ObjectMagnification(void)
    {
        return ObjMagnification;
    }
    float BubbleRatio(void)
    {
        return PlayerBubble;
    }
    int ObjectDeaggLevel(void)
    {
        return ObjDeaggLevel;
    }
    int BuildingDeaggLevel(void)
    {
        return BldDeaggLevel;
    }
    float SfxDetailLevel(void)
    {
        return SfxLevel;
    }
    int AcmiFileSize(void)
    {
        return ACMIFileSize;
    }

    int GetFlightModelType(void)
    {
        return SimFlightModel;
    }
    int GetWeaponEffectiveness(void)
    {
        return SimWeaponEffect;
    }
    int GetAvionicsType(void)
    {
        return SimAvionicsType;
    }
    int GetAutopilotMode(void)
    {
        return SimAutopilotType;
    }
    int GetRefuelingMode(void)
    {
        return SimAirRefuelingMode;
    };
    int GetPadlockMode(void)
    {
        return SimPadlockMode;
    };
    int GetVisualCueMode(void)
    {
        return SimVisualCueMode;
    };
    int AutoTargetingOn(void)
    {
        return (SimFlags bitand SIM_AUTO_TARGET) and TRUE;
    }
    int BlackoutOn(void)
    {
        return not (SimFlags bitand SIM_NO_BLACKOUT) and TRUE;
    }
    int NoBlackout(void)
    {
        return (SimFlags bitand SIM_NO_BLACKOUT) and TRUE;
    }
    int UnlimitedFuel(void)
    {
        return (SimFlags bitand SIM_UNLIMITED_FUEL) and TRUE;
    }
    int UnlimitedAmmo(void)
    {
        return (SimFlags bitand SIM_UNLIMITED_AMMO) and TRUE;
    }
    int UnlimitedChaff(void)
    {
        return (SimFlags bitand SIM_UNLIMITED_CHAFF) and TRUE;
    }
    int CollisionsOn(void)
    {
        return not (SimFlags bitand SIM_NO_COLLISIONS) and TRUE;
    }
    int NoCollisions(void)
    {
        return (SimFlags bitand SIM_NO_COLLISIONS) and TRUE;
    }
    int NameTagsOn(void)
    {
        return (SimFlags bitand SIM_NAMETAGS) and TRUE;
    }
    int LiftLineOn(void)
    {
        return (SimFlags bitand SIM_LIFTLINE_CUE) and TRUE;
    }
    int BullseyeOn(void)
    {
        return (SimFlags bitand SIM_BULLSEYE_CALLS) and TRUE;
    }
    int InvulnerableOn(void)
    {
        return (SimFlags bitand SIM_INVULNERABLE) and TRUE;
    }

    int WeatherOn(void)
    {
        return not (GeneralFlags bitand GEN_NO_WEATHER);
    }
    int MFDTerrainOn(void)
    {
        return (GeneralFlags bitand GEN_MFD_TERRAIN) and TRUE;
    }
    int HawkeyeTerrainOn(void)
    {
        return (GeneralFlags bitand GEN_HAWKEYE_TERRAIN) and TRUE;
    }
    int PadlockViewOn(void)
    {
        return (GeneralFlags bitand GEN_PADLOCK_VIEW) and TRUE;
    }
    int HawkeyeViewOn(void)
    {
        return (GeneralFlags bitand GEN_HAWKEYE_VIEW) and TRUE;
    }
    int ExternalViewOn(void)
    {
        return (GeneralFlags bitand GEN_EXTERNAL_VIEW) and TRUE;
    }

    int CampaignGroundRatio(void)
    {
        return CampGroundRatio;
    }
    int CampaignAirRatio(void)
    {
        return CampAirRatio;
    }
    int CampaignAirDefenseRatio(void)
    {
        return CampAirDefenseRatio;
    }
    int CampaignNavalRatio(void)
    {
        return CampNavalRatio;
    }

    int CampaignEnemyAirExperience(void)
    {
        return CampEnemyAirExperience;
    }
    int CampaignEnemyGroundExperience(void)
    {
        return CampEnemyGroundExperience;
    }
    int CampaignEnemyStockpile(void)
    {
        return CampEnemyStockpile;
    }
    int CampaignFriendlyStockpile(void)
    {
        return CampFriendlyStockpile;
    }

    bool getInfoBar(void) const
    {
        return infoBar;    // Retro 25Dec2003
    }
    bool getSubtitles(void) const
    {
        return subTitles;    // Retro 25Dec2003
    }
    bool Get2dTrackIR(void) const
    {
        return TrackIR_2d;    // Retro 27Dec2003
    }
    bool Get3dTrackIR(void) const
    {
        return TrackIR_3d;    // Retro 27Dec2003
    }
    bool GetFFB(void) const
    {
        return enableFFB;    // Retro 27Dec2003
    }
    bool GetMouseLook(void) const
    {
        return enableMouseLook;    // Retro 28Dec2003
    }
    bool GetTouchBuddy(void) const
    {
        return enableTouchBuddy;    // sfr: touch buddy support
    }
    bool GetDrawMirror(void) const
    {
        return drawMirror;    // sfr: rear mirror
    }

    // Setter functions
    void SetSimFlag(int flag)
    {
        SimFlags or_eq flag;
    };
    void ClearSimFlag(int flag)
    {
        SimFlags and_eq compl flag;
    };

    void SetDispFlag(int flag)
    {
        DispFlags or_eq flag;
    };
    void ClearDispFlag(int flag)
    {
        DispFlags and_eq compl flag;
    };

    void SetObjFlag(int flag)
    {
        ObjFlags or_eq flag;
    };
    void ClearObjFlag(int flag)
    {
        ObjFlags and_eq compl flag;
    };

    void SetGenFlag(int flag)
    {
        GeneralFlags or_eq flag;
    };
    void ClearGenFlag(int flag)
    {
        GeneralFlags and_eq compl flag;
    };

    void SetStartFlag(StartFlag flag)
    {
        SimStartFlags = flag;
    }
    StartFlag  GetStartFlag()
    {
        switch (SimStartFlags) // MLR 12/11/2003 - Sanity check
        {
            case START_RUNWAY:
            case START_TAXI:
            case START_RAMP:
                return(SimStartFlags);
        }

        return(START_RUNWAY);
    }

    void SetKeyFile(_TCHAR *fname)
    {
        _tcscpy(keyfile, fname);
    }
    _TCHAR *GetKeyfile(void)
    {
        return keyfile;
    }

    // Retro_dead 15Jan2004 void SetJoystick (GUID newID) { joystick=newID; }
    // Retro_dead 15Jan2004 GUID GetJoystick (void) { return joystick; }

    void SetCampEnemyAirExperience(int exp)
    {
        CampEnemyAirExperience = exp;
    }
    void SetCampEnemyGroundExperience(int exp)
    {
        CampEnemyGroundExperience = exp;
    }

    void SetCampGroundRatio(int ratio)
    {
        CampGroundRatio = ratio;
    }
    void SetCampAirRatio(int ratio)
    {
        CampAirRatio = ratio;
    }
    void SetCampAirDefenseRatio(int ratio)
    {
        CampAirDefenseRatio = ratio;
    }
    void SetCampNavalRatio(int ratio)
    {
        CampNavalRatio = ratio;
    }

    void SetInfoBar(bool onOff)
    {
        infoBar = onOff;    // Retro 25Dec2003
    }
    void SetSubtitles(bool onOff)
    {
        subTitles = onOff;    // Retro 25Dec2003
    }
    void SetTrackIR2d(bool onOff)
    {
        TrackIR_2d = onOff;    // Retro 27Dec2003
    }
    void SetTrackIR3d(bool onOff)
    {
        TrackIR_3d = onOff;    // Retro 27Dec2003
    }
    void SetFFB(bool onOff)
    {
        enableFFB = onOff;    // Retro 27Dec2003
    }
    void SetTouchBuddy(bool onOff)
    {
        enableTouchBuddy = onOff;    //sfr: touch buddy support
    }
    void SetDrawMirror(bool onOff)
    {
        drawMirror = onOff;    // sfr: rear view mirror
    }
    void SetMouseLook(bool onOff)
    {
        enableMouseLook = onOff;    // Retro 28Dec2003
    }

    void SetMouseLookSensitivity(float theVal)
    {
        MouseLookSensitivity = theVal;    // Retro 15Jan2004 - x/y axis
    }
    float GetMouseLookSensitivity(void)
    {
        return MouseLookSensitivity;    // Retro 15Jan2004
    }

    void SetMouseWheelSensitivity(int theVal)
    {
        MouseWheelSensitivity = theVal;    // Retro 17Jan2004 - z axis
    }
    int GetMouseWheelSensitivity(void)
    {
        return MouseWheelSensitivity;    // Retro 17Jan2004
    }

    void SetKeyboardPOVPanningSensitivity(int theVal)
    {
        KeyboardPOVPanningSensitivity = theVal;    // Retro 18Jan2004
    }
    int GetKeyboardPOVPanningSensitivity(void)
    {
        return KeyboardPOVPanningSensitivity;    // Retro 18Jan2004
    }

    bool GetClickablePitMode(void)
    {
        return clickablePitMode;
    }
    void SetClickablePitMode(bool onOff)
    {
        clickablePitMode = onOff;
    }

    bool GetAxisShaping(void)
    {
        return enableAxisShaping;    // Retro 27Jan2004
    }
    void SetAxisShaping(bool onOff)
    {
        enableAxisShaping = onOff;    // Retro 27Jan2004
    }
};

// ==================================
// Our global player options instance
// ==================================

extern PlayerOptionsClass PlayerOptions;

// =====================
// Other functions
// =====================

typedef struct
{
    _TCHAR name[50]; // To display in UI
    _TCHAR todname[MAX_PATH]; // Filename of tod file
    _TCHAR image1[MAX_PATH]; // screenshot 5:00
    _TCHAR image2[MAX_PATH];   // screenshot 10:00
    _TCHAR image3[MAX_PATH]; // screenshot 15:00
    _TCHAR image4[MAX_PATH];   // screenshot 20:00
} SkyColorDataType;

typedef struct
{
    _TCHAR name[50]; // To display in UI
    _TCHAR filename[MAX_PATH]; // accompagnied filename
    _TCHAR picfname[MAX_PATH]; // picture of weather distribution
} WeatherPatternDataType;


#endif
