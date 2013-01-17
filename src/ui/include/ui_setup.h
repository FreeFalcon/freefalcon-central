#ifndef UI_SETUP
#define UI_SETUP

#include "textids.h"

extern float JoyScale;
extern float RudderScale;
extern float ThrottleScale;

extern RECT Rudder;
extern RECT Throttle;

extern int Calibrated;
extern int ready;

extern F4CSECTIONHANDLE* SetupCritSection;

typedef struct
{
	float	Xpos;
	float	Ypos;
	float	Zpos;
	float	CamPitch;
	float	CamRoll;
	float	CamYaw;
	float	CamX;
	float	CamY;
	float	CamZ;
}ViewPos;

typedef struct
{
	float	VisID;
	float	Xpos;
	float	Ypos;
	float	Zpos;
	float	Pitch;
	float	Roll;
	float	Yaw;
}ObjectPos;

typedef struct
{
	float	VisID;
	float	Priority;
	float	Xpos;
	float	Ypos;
	float	Zpos;
	float	Facing;
}FeaturePos;

enum{
	PITCH_CHG =	10,
	YAW_CHG	=	10,
	MAX_ALT	=	-50000,
	MIN_ALT = -10,
	ALT_CHG = 500,
	SND_RNG	=	-4900,
	SQRT_SND_RNG =	70,
	MAX_TERR_DIST = 80,
	MIN_TERR_DIST = 40,
	NUM_LEVELS = 5,
	DESCRIP_LINE = 35,
	SND_BUTTON = 0,
	SND_VOICE = 1,
	SND_UI_SOUNDS = 2,
	SND_MUSIC = 3,
	SND_SFX = 4,
};

typedef struct
{
	int		FlightModel;
	int		RadarMode;
	int		WeapEffects;
	int		Autopilot;
	int		RefuelingMode;
	int		PadlockMode;
	int		flags;
}Preset;

typedef struct
{
	int EditKey;
	int CommandsKeyCombo;
	int CommandsKeyComboMod;
	long CurrControl;
	long OldControl;
	int Modified;
	int NeedUpdate;
}KeyVars;

extern KeyVars KeyVar;

typedef struct
{
	int calibrating;	//TRUE if currently calibrating
	int state;			//state = 1 if waiting for user to let go of button, 0 if waiting for press
	int step;			//current calibration step we're on
	int disp_text;		//TRUE if we need to display text prompts this time through
	int calibrated;
}CalibrateStruct;

extern CalibrateStruct Calibration;

enum{
	CTR_JOY	=	70105,
	MV_JOY =	70106,
	MV_RUD =	70107,
	MV_THR =	70108,
	USE_FILENAME = 1,
	USE_DEFAULT = 0,
};

#endif