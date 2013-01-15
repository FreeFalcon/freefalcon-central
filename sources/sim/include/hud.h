#ifndef _HUD_H
#define _HUD_H

#include "drawable.h"
#include "f4vu.h"
#include "polylib.h"

// COBRA - RED - ********* MACRO DEFINITIONS FOR THE HUD STUFFS ***********

#define	HUD_MIN_BRIGHT_NIGHT	0.2f
#define	HUD_MIN_BRIGHT_DAY		0.3f
#define	HUD_MAX_BRIGHT_NIGHT	0.4f
#define	HUD_MAX_BRIGHT_DAY		0.8f

#define	DEFAULT_HUD_COLOR		0x4070ff80

// COBRA - RED - ******* END MACRO DEFINITIONS FOR THE HUD STUFFS *********


// These Windows are Zero Based
#define AIRSPEED_WINDOW            38
#define ALTITUDE_WINDOW            39
#define HEADING_WINDOW_HI          40
#define HEADING_WINDOW_LO          41
#define DLZ_WINDOW                 42
#define PITCH_LADDER_WINDOW        43
#define BORESIGHT_CROSS_WINDOW     44

#define NUM_VERTICAL_TICKS         11
#define NUM_HORIZONTAL_TICKS       10
//#define NUM_WIN 45
#define NUM_WIN 55 //TJL 03/07/04

#define MISSILE_RETICLE_OFFSET     (RadToHudUnits(-6.0F * DTR))
#define MISSILE_ASE_SIZE           0.03F

extern float hudWinX[NUM_WIN];
extern float hudWinY[NUM_WIN];
extern float hudWinWidth[NUM_WIN];
extern float hudWinHeight[NUM_WIN];
extern char *hudNumbers[101];

// Forward declarations for class pointers
class GunClass;
class FireControlComputer;
class SimBaseClass;
class AircraftClass;
class SimObjectType;
class SimObjectLocalData;
struct RunwayType;
class VuEntity;
class FalconEntity;

class HudDataType
{
  public:
	enum {
		RadarBoresight = 0x1,
		RadarSlew      = 0x2,
		RadarVertical  = 0x4,
      RadarNoRad     = 0x8,
	};

	HudDataType (void);

	float	tgtAz, tgtEl, tgtAta, tgtDroll;
	float	radarAz, radarEl;
	int		tgtId;
	int		flags;
	int		IsSet (int testFlag)	{ return (flags & testFlag ? TRUE : FALSE); };
	void	Set (int testFlag)		{ flags |= testFlag; };
	void	Clear (int testFlag)	{ flags &= ~testFlag; };
};

class HudClass : public DrawableClass
{
  public:
	enum	ScalesSwitch {VV_VAH, VAH, H, SS_OFF};
	enum	FPMSwitch {ATT_FPM, FPM, FPM_OFF, FPM_AUTO};
	enum	DEDSwitch {DED_DATA, DED_OFF, PFL_DATA};
	enum	VelocitySwitch {CAS, TAS, GND_SPD};
	enum	RadarSwitch {ALT_RADAR, BARO, RADAR_AUTO};
	enum	BrightnessSwitch {OFF, DAY, BRIGHT_AUTO, NIGHT};
// 2000-11-10 ADDED BY S.G. FOR THE drift C/O switch
	enum	DriftCOSwitch {DRIFT_CO_OFF, DRIFT_CO_ON};
// END OF ADDED SECTION

	enum	{NumEEGSSegments = 24};			// (Starts one segment away from ownship)
	enum	{EEGSTimePerSegment = 200};		// ms.  200 means each segment is 1/5 second long.
	enum	{EEGSTimeLength = (NumEEGSSegments) * EEGSTimePerSegment};	// milliseconds
	enum	{EEGSUpdateTime = 33};			// Limit how often we can grab new data (ms)
	enum	{NumEEGSFrames = EEGSTimeLength / EEGSUpdateTime + 1};
	//TJL 01/25/04
	SimObjectLocalData*	GetLocalTarget(void) {return targetData;};


  private:
	VirtualDisplay*      display;	// The renderer we are to draw upon
	AircraftClass*       ownship;	// Points to the AircraftClass we're riding upon
	FireControlComputer* FCC;		// Points to ownship->FCC.  Repeated here to save one dereference.
	SimObjectType*       targetPtr;
	SimObjectLocalData*  targetData;
	int						curRwy;
	VuEntity*            curAirbase;

	static int		flash;
	//MI
	static int		Warnflash;

	void	DrawFPM(void);
	void	DrawHorizonLine(void);
	void	DrawPitchLadder(void);
	void	DrawAirspeed(void);
	void	DrawAltitude(void);
	void	DrawHeading(void);
	void	DrawTDBox(void);
	//MI
	void	DrawDTOSSBox(void);
	void	DrawTDMarker(float az, float el, float dRoll, float size);
	void	DrawAATDBox(void);
	void	DrawTDCircle(void);
	void	DrawBoresightCross(void);
	void	DrawAlphaNumeric(void);
	void	DrawILS (void);
	void	DrawNav (void);
	void	DrawWaypoint(void);
	enum	DesignateShape {Circle, Square};
	void	DrawDesignateMarker (DesignateShape shape, float az, float el, float dRoll);
	void	TimeToSteerpoint(void);
	void	RangeToSteerpoint(void);
	void	DrawTadpole (void);
	void	DrawF18HUD (void);//TJL 03/07/04
	void	DrawF14HUD (void);//TJL 03/07/04
	void	DrawF15HUD (void);//TJL 03/10/04
	void	DrawA10HUD (void);//TJL 03/10/04

	// A-A Missile modes
	void	DrawAirMissile (void);
	void	DrawDogfight (void);
	void	DrawMissileOverride (void);
	void	DrawMissileReticle(float radius, int showRange, int showAspect);
	void	DrawAim9Diamond(void);
	void	DrawAim120Diamond(void);
	void	DrawAim9DLZ(void);
	void	DrawAim120DLZ(bool dgft);
	void	DrawAim120ASE(void);
	void	CheckBreakX(void);
	void	DrawDLZSymbol(float percentRange, char* tmpStr, float rMin, float rMax, float rNeMin, float rNeMax, BOOL aaMode,char* tmpStrpole);
	void	DrawAim9Reticle(float radius, int showRange, int showAspect);		//Wombat778 10-16-2003  

	// A-G Missile/Bomb Modes
	void	DrawAirGroundGravity(void);
	void    DrawAirGroundRocket(void);
	void	DrawTargetingPod(void);
	void	DrawGroundMissile (void);
	void	DrawHarm (void);
	void	DrawHarmFovBox( void ); // RV - I-Hawk
	void	DrawCCIP (void);
	void	DrawCCRP (void);
	void	DrawRCKT (void);
	void	DrawDTOSS (void);
	void	DrawLADD (void);
	void	DrawStrafe(void);
	void	DrawHTSDLZ(void);
	void	DrawAGMDLZ(void);
	void	DrawSteeringToRelease(void);
	void	DrawRollCue(void);
   void  DrawRPod(void);

	// A-A Gun modes
	void	DrawGuns(void);
	void	DrawEEGS (void); 
	void	DrawSSLC (void);	// ASSOCIATOR 03/12/03: Added the combined SnapShot LCOS Gunmode SSLC 
	void	DrawFunnel (void);
	void	DrawMRGS (void);
	void	DrawLCOS (void);
	void	DrawLCOSForSSLC (void);	// ASSOCIATOR 03/12/03: Helper function for DrawSSLC method
	void	DrawSnapshot (void);
	void	DrawTSymbol (void);
	void	FlyBullets(void);
	void	DrawBATR (void); //JPG 8 May 04 - Bullets at Target Range

	float	lastPipperX, lastPipperY;
	float	bulletH[NumEEGSSegments], bulletV[NumEEGSSegments], bulletRange[NumEEGSSegments];
	float	funnel1X[NumEEGSSegments], funnel1Y[NumEEGSSegments];
	float	funnel2X[NumEEGSSegments], funnel2Y[NumEEGSSegments];

	struct {
		SIM_LONG	time;
		float		x;
		float		y;
		float		z;
		float		vx;
		float		vy;
		float		vz;
	} eegsFrameArray[NumEEGSFrames];
	int			eegsFrameNum;
	SIM_LONG	lastEEGSstepTime;

	// Time interpolation from sampled EEGS data to the specified time ago
	float EEGShistory( SIM_LONG dt, int *beforeIndex, int *afterIndex );
	float HudClass::EEGSvalueX( float t, int before, int after )
		  { return eegsFrameArray[after].x + t * (eegsFrameArray[before].x-eegsFrameArray[after].x); };
	float HudClass::EEGSvalueY( float t, int before, int after )
		  { return eegsFrameArray[after].y + t * (eegsFrameArray[before].y-eegsFrameArray[after].y); };
	float HudClass::EEGSvalueZ( float t, int before, int after )
		  { return eegsFrameArray[after].z + t * (eegsFrameArray[before].z-eegsFrameArray[after].z); };
	float HudClass::EEGSvalueVX( float t, int before, int after )
		  { return eegsFrameArray[after].vx + t * (eegsFrameArray[before].vx-eegsFrameArray[after].vx); };
	float HudClass::EEGSvalueVY( float t, int before, int after )
		  { return eegsFrameArray[after].vy + t * (eegsFrameArray[before].vy-eegsFrameArray[after].vy); };
	float HudClass::EEGSvalueVZ( float t, int before, int after )
		  { return eegsFrameArray[after].vz + t * (eegsFrameArray[before].vz-eegsFrameArray[after].vz); };


	void	DrawWindowString (int, char *, int = 0);
	float	MRToHudUnits (float mr);
	float	RadToHudUnits (float mr);
	float	RadToHudUnitsX (float mr)	{ return RadToHudUnits(mr) * mHScale; };
	float	RadToHudUnitsY (float mr)	{ return RadToHudUnits(mr) * mVScale; };
	float	HudUnitsToRad (float mr);
	float	maxGs;
	float	halfAngle, degreesForScreen;
   float pixelXCenter, pixelYCenter, sightRadius;
   float alphaHudUnits, betaHudUnits;


	ScalesSwitch		scalesSwitch;
	FPMSwitch			fpmSwitch;
	DEDSwitch			dedSwitch;
	VelocitySwitch		velocitySwitch;
	RadarSwitch			radarSwitch;
	BrightnessSwitch	brightnessSwitch;
// 2000-11-10 ADDED BY S.G. FOR THE Drift C/O switch
	DriftCOSwitch		driftCOSwitch;
// END OF ADDED SECTION

	// For HUD coloring
//	static const int	NumHudColors;
//	int				curColorIdx;
	static Pcolor			hudColor;
	DWORD							curHudColor;
	float				HudBrightness,HudContrast,AutoHudCx;						// COBRA - RED - Hud Brightness & Contrast

	bool	CheckGhostHorizon(float radius, float xOffset, float yOffset, 
		                      float horizX1, float horizY1, float horizX2, float horizY2);

  public:
	HudClass(void);
	~HudClass(void);

	void			SetOwnship (AircraftClass* ownship);
	AircraftClass*	Ownship(void) {return ownship;};
	void			SetTarget (SimObjectType* newTarget);
	void			ClearTarget (void);


	void			GetBoresightPos (float*, float*);
	void			SetHalfAngle (float, float HScale = 1.0f, float VScale = 1.0f);
	VirtualDisplay*	GetDisplay (void) {return privateDisplay;};
	void			Display (VirtualDisplay*, bool Translucent=false);
	void			Display (VirtualDisplay* D) { Display( D, false); };
	void			DisplayInit (ImageBuffer*);

	VuEntity*   CanSeeTarget (int wid, VuEntity* entity, FalconEntity* platform); // Note: This is specific to the RECON pod

	void	PushButton (int whichButton, int whichMFD = 0);
	void	SetEEGSData (float x, float y, float z,
					 float gamma, float sigma,
					 float theta, float psi, float vt);

	DWORD	GetHudColor(void);
	void	SetHudColor(DWORD);
	void	HudColorStep (void);
	void	SetLightLevel(void);
	void	CalculateBrightness(float, DWORD*);
	void	SetContrastLevel(void);

	int		GetScalesSwitch(void);
	void	SetScalesSwitch(ScalesSwitch);
	void	CycleScalesSwitch(void);

	int		GetFPMSwitch(void);
	void	SetFPMSwitch(FPMSwitch);
	void	CycleFPMSwitch(void);

// 2000-11-10 ADDED BY S.G. FOR THE Drift C/O switch
	int		GetDriftCOSwitch(void);
	void	SetDriftCOSwitch(DriftCOSwitch);
	void	CycleDriftCOSwitch(void);
// END OF ADDED SECTION
	int		GetDEDSwitch(void);
	void	SetDEDSwitch(DEDSwitch);
	void	CycleDEDSwitch(void);

	int		GetVelocitySwitch(void);
	void	SetVelocitySwitch(VelocitySwitch);
	void	CycleVelocitySwitch(void);

	int		GetRadarSwitch(void);
	void	SetRadarSwitch(RadarSwitch);
	void	CycleRadarSwitch(void);

	int		GetBrightnessSwitch(void);
	void	SetBrightnessSwitch(BrightnessSwitch);
	void	CycleBrightnessSwitch(void);
	void	CycleBrightnessSwitchUp(void);	//MI
	void	CycleBrightnessSwitchDown(void);	//MI

	float	waypointX, waypointY, waypointZ;
//	float	waypointAz, waypointEl, waypointSpeed;
	float	waypointAz, waypointEl, waypointSpeed, waypointGNDSpeed;
	float	waypointArrival, waypointBearing, waypointRange;
	int		waypointNum;
	char	waypointValid;

	int							dedRepeat;
	float						lowAltWarning;
	enum	{ High, Low, Off }	headingPos;
	int							doPitchLadder;
	HudDataType					HudData;

	//MI
	int FindRollAngle(float);
	int FindPitchAngle(float);
	bool WasAboveMSLFloor;
	int MSLFloor, TFAdv;
	void CheckMSLFloor(void);
	void DrawMANReticle(void);
	int WhichMode;
	//Will be set from the ICP
	float DefaultTargetSpan;
	void DrawSteeringToReleaseLADD(void);
	float OA1Az, OA1Elev;
	float OA2Az, OA2Elev;
	bool OA1Valid, OA2Valid;
	float VIPAz,VIPElev;
	bool VIPValid;
	float VRPAz, VRPElev;
	bool VRPValid;
	void DrawOA(void);
	void DrawVIP(void);
	void DrawVRP(void);
	void DrawRALT(void);
	void DrawRALTBox(void);
	void DrawALString(void);
	char tmpStr[12];
	float hat;
	float YALText;
	float YRALTText;
	void ResetMaxG(void)	{maxGs = 1;};
	void DrawCMDSTRG(void);
	void DrawBankIndicator(void);
	bool CalcRoll, CalcBank;

	void DrawAirSpeedCarret(float Speed);
	void DrawAltCarret(float Alt);

	void FlyFEDSBullets(bool NewBullets);
	VU_TIME HideFunnelTimer, ShowFunnelTimer;
	bool HideFunnel, SetHideTimer, SetShowTimer;

	void DrawCruiseIndexes(void);
	char SpeedText[10];

	float RET_CENTER;
	int RetPos, ReticlePosition;
	void MoveRetCenter(void);
	float SlantRange;

	//Mav stuff
	void DrawBearing(void);

	//SYM Wheel
	float SymWheelPos,ContWheelPos;
	bool fpmConstrained;//TJL 03/11/04
	class SimObjectLocalData* lockedTargetData;
	long hudDelayTimer;
	long hudAltDelayTimer;
	long hudRAltDelayTimer;
	int aspeedHud;
	float altHud;
	float altHudn;
	float raltHud;
	int vvid;

	float	mVScale, mHScale;
};

extern HudClass *TheHud;

#endif
