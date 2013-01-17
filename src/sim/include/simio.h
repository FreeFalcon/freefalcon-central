/******************************************************************************/
/*                                                                            */
/*  Unit Name : simio.h                                                       */
/*                                                                            */
/*  Abstract  : Header file with class definition for SIMLIB_IO_CLASS and     */
/*              defines used in its implementation.                           */
/*                                                                            */
/*  Dependencies : Auto-Generated                                             */
/*                                                                            */
/*  Operating System : MS-DOS 6.2, Windows 3.1                                */
/*                                                                            */
/*  Compiler : MSVC V1.5                                                      */
/*                                                                            */
/*  Naming Conventions :                                                      */
/*                                                                            */
/*      Public Functions    : Mixed case with no underscores                  */
/*      Private Functions   : Mixed case with no underscores                  */
/*      Public Functions    : Mixed case with no underscores                  */
/*      Global Variables    : Mixed case with no underscores                  */
/*      Classless Functions : Mixed case with no underscores                  */
/*      Classes             : All upper case seperated by an underscore       */
/*      Defined Constants   : All upper case seperated by an underscore       */
/*      Macros              : All upper case seperated by an underscore       */
/*      Structs/Types       : All upper case seperated by an underscore       */
/*      Private Variables   : All lower case seperated by an underscore       */
/*      Public Variables    : All lower case seperated by an underscore       */
/*      Local Variables     : All lower case seperated by an underscore       */
/*                                                                            */
/*  Development History :                                                     */
/*  Date      Programer           Description                                 */
/*----------------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                               */
/*                                                                            */
/******************************************************************************/
#ifndef _SIMIO_H
#define _SIMIO_H

 // Retro 31Dec2003 - major rewrite
#include "sinput.h"

/*	all axis I could think of right now.. this defines the order in the 'analog' 
	array in which the values are stored */
typedef enum {
	AXIS_START = 0,
	AXIS_PITCH = AXIS_START,										//	bipolar
	AXIS_ROLL,														//	bipolar
	AXIS_YAW,														//	bipolar
	AXIS_THROTTLE,			// leftmost, or EXCLUSIVE, throttle			unipolar
	AXIS_THROTTLE2,													//	bipolar
	AXIS_TRIM_PITCH,												//	bipolar
	AXIS_TRIM_YAW,													//	bipolar
	AXIS_TRIM_ROLL,													//	bipolar
	AXIS_BRAKE_LEFT,		// left, or EXCLUSIVE, brake				unipolar
//	AXIS_BRAKE_RIGHT,												//	unipolar
	AXIS_FOV,				// Field of View							unipolar
	AXIS_ANT_ELEV,			// Radar Antenna Elevation					bipolar
	AXIS_CURSOR_X,			// Cursor/Enable-X							bipolar
	AXIS_CURSOR_Y,			// Cursor/Enable-X							bipolar
	AXIS_RANGE_KNOB,												//	bipolar
	AXIS_COMM_VOLUME_1,		// Comms Channel 1 Volume					unipolar
	AXIS_COMM_VOLUME_2,		// Comms Channel 2 Volume					unipolar
	AXIS_MSL_VOLUME,												//	unipolar
	AXIS_THREAT_VOLUME,												//	unipolar
	AXIS_HUD_BRIGHTNESS,	// HUD Symbology Intensity					unipolar
	AXIS_RET_DEPR,			// Manual Reticle Depression				unipolar
	AXIS_ZOOM,				// View Zoom								unipolar
	AXIS_INTERCOM_VOLUME,	// InterCom Volume, that´s kinda a 'master volume' for all comm channels  // unipolar
	AXIS_MAX			// Add any additional axis BEFORE that one !
}	GameAxis_t;

#define SIMLIB_MAX_ANALOG		AXIS_MAX	// max number of axis
#define SIMLIB_MAX_DIGITAL      32			// max number of dinput buttons per device (the rest would have to be emulated as keypresses)
#define SIMLIB_MAX_POV			4			// max number of POVs per device

struct DeviceAxis {
	int Device;
	int Axis;
	int Deadzone;
	int Saturation;
	DeviceAxis() { Device = Axis = Saturation = -1; Deadzone = 100; }
};

enum {
	DX_XAXIS = 0,
	DX_YAXIS,
	DX_ZAXIS,
	DX_RXAXIS,
	DX_RYAXIS,
	DX_RZAXIS,
	DX_SLIDER0,
	DX_SLIDER1,
	DX_MOUSEWHEEL = 0	// Retro 17Jan2004
};

void ResetDeviceAxis(DeviceAxis* t);

struct AxisMapping {
	int FlightControlDevice;

	// these 2 are sanity values, to find out if the user attach/detached additional
	// DXDevices since the last mapping
	GUID FlightControllerGUID;
	int totalDeviceCount;

	DeviceAxis Pitch;
	DeviceAxis Bank;
	DeviceAxis Yaw;

	DeviceAxis Throttle;
	DeviceAxis Throttle2;

	DeviceAxis BrakeLeft;
	DeviceAxis BrakeRight;

	DeviceAxis FOV;

	DeviceAxis PitchTrim;
	DeviceAxis YawTrim;
	DeviceAxis BankTrim;

	DeviceAxis AntElev;
	DeviceAxis RngKnob;
	DeviceAxis CursorX;
	DeviceAxis CursorY;

	DeviceAxis Comm1Vol;
	DeviceAxis Comm2Vol;
	DeviceAxis MSLVol;
	DeviceAxis ThreatVol;
	DeviceAxis InterComVol;

	DeviceAxis HudBrt;
	DeviceAxis RetDepr;

	DeviceAxis Zoom;

	AxisMapping()
	{
		FlightControlDevice = -1;
		memset(&FlightControllerGUID, 0,sizeof(GUID));
		totalDeviceCount = 0;
	}
};

/* representation in the UI (ControlTab) */
struct AxisIDStuff {
	char* DXAxisName;
	int DXAxisID;
	int DXDeviceID;
	bool isMapped;

	AxisIDStuff()
	{
		DXAxisName = 0;
		DXAxisID = -1;
		DXDeviceID = -1;
		isMapped = false;
	}
};

/*****************************************************************************/
//	Retro 23Jan2004
//	Custom axis shaping..
/*****************************************************************************/
struct AxisCalibration {
	bool active[AXIS_MAX];
	DIPROPCPOINTS CalibPoints[AXIS_MAX];

	AxisCalibration()
	{
		for (int i = 0; i < AXIS_MAX; i++)
		{
			active[i] = false;
			CalibPoints[i].dwCPointsNum = 0;
			for (int j = 0; j < MAXCPOINTSNUM; j++)
			{
				CalibPoints[i].cp[j].lP = 0;
				CalibPoints[i].cp[j].dwLog = 0;
			}
		}
	}
};
extern AxisCalibration AxisShapes;
//	Retro 23Jan2004 end

/* representation in the game (actually, this is used on polling) */
typedef struct {
	int* device;
	int* axis;
	int* deadzone;
	int* saturation;
	bool isUniPolar;
} GameAxisSetup_t;

// these structs define the in-game-axis properties
typedef struct
{
   SIM_LONG		center;		// used to recenter pitch/bank axis and as ABDetent for throttles
   SIM_LONG		cutoff;		// idle cutoff val for throttles
   SIM_LONG		ioVal;		// 'raw' read value ??
   bool			isUsed;		// well... waddaya think ?
   SIM_FLOAT	engrValue;	// dunno what the abbreviation means but this is the processed input (with input linearity applied etc..)
   bool			isReversed;	// well..
   SIM_LONG		smoothingFactor;		// Retro 19Feb2004 - has to be power of 2, 0 if deactivated
} SIMLIB_ANALOG_TYPE;

/***************************************************************************/
//	OK this class just holds the processed data retrived from the joysticks,
//	and some flags on what to do with em (min, max, center)
//
//	the actual polling, processing, mapping is done by other (global.. urgs)
//	functions that write into this class
//
//	sijoy.cpp siloop.cpp and controltab.cpp are the prime suspects for this
//
//	the mapping of in-game axis to real device axis is also done elsewhere
/***************************************************************************/
class SIMLIB_IO_CLASS
{
public:
	SIMLIB_ANALOG_TYPE analog[SIMLIB_MAX_ANALOG];			// array of all axis
	SIM_SHORT digital[SIMLIB_MAX_DIGITAL*SIM_NUMDEVICES];	// array of all buttons of all devices
	DWORD povHatAngle[SIMLIB_MAX_POV];						// array of all POVs

public:
	/* constructor, basically sets all to '0' or 'false' */
	SIMLIB_IO_CLASS();
	/* called on entering the 3d, use to init axisvalues etc */
	SIM_INT	Init (char *fname);

	/* data retrieval stuff */
	SIM_FLOAT	ReadAnalog  (GameAxis_t id);		// Retro 28Dec2003 - this return the (processed, normalised ?) engrValue, of MPS vintage. used for primary flight stuff (pitch/yaw/bank/throttle)
	SIM_INT		GetAxisValue(GameAxis_t id);		// Retro 28Dec2003 - this returns the ioVal, -10000 to 10000 (bipolar), 0-15000 (unipolar)
	SIM_INT		ReadDigital (SIM_INT id);

	/* axis status stuff */
	bool	AnalogIsUsed(GameAxis_t id) { return ( analog[id].isUsed); };
	void	SetAnalogIsUsed(GameAxis_t id, bool flag) { analog[id].isUsed = flag; };

	bool	AnalogIsReversed(GameAxis_t id) { return (analog[id].isReversed); }
	void	SetAnalogIsReversed(GameAxis_t id, bool flag) { analog[id].isReversed = flag; };

	long	GetAxisCutoffValue(GameAxis_t id) { return analog[id].cutoff; }
	bool	IsAxisCutOff(GameAxis_t id) { return (analog[id].ioVal > (analog[id].cutoff + idleCutoffPad)); } // MD -- 20040210

	long	GetAxisSmoothing(GameAxis_t id) { return (analog[id].smoothingFactor); }
	void	SetAxisSmoothing(GameAxis_t id, long factor) { analog[id].smoothingFactor = factor; }	// factor has to be power of 2 !!

	// reads/writes offset-values for bipolar axis
	int ReadFile (void);
	int SaveFile (void);

	// reads/writes mapping of real axis to ingame axis
	int ReadAxisMappingFile ();
	int WriteAxisMappingFile ();

	// loads the custom axis shaping data
	int LoadAxisCalibrationFile ();	//	Retro 23Jan2004

	void Reset();
	void ResetAllInputs();

	bool MouseWheelExists() { return mouseWheelPresent; }
	void SetMouseWheelExists(bool yesno) { mouseWheelPresent = yesno; }
private:
	void SaveGUIDAndCount();
	bool mouseWheelPresent;
	int	 idleCutoffPad;  // MD -- 20040210: add variable pad to make sure we return cutoff only when we mean it, not on jitter/repeatability boundaries for flaky joystick pots
};

extern SIMLIB_IO_CLASS   IO;

#endif	// #define _SIMIO_H