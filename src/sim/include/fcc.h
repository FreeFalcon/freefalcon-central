#ifndef _FCC_H
#define _FCC_H

#include "drawable.h"
#include "Entity.h"
#include "campwp.h"
#include "irst.h"//me123
#include "Hardpnt.h"//me123
#include "mfd.h"
#include "bomb.h"

// Forward declare of class pointers
class AirframeClass;
class AircraftClass;
class SMSClass;
class SimVehicleClass;
class PilotInputs; 
class SimObjectType;
class MissileClass;
class NavigationSystem;
class LaserPodClass;

class GroundListElement
{
public:
    GroundListElement(FalconEntity *newEntity);
    ~GroundListElement();
    
    FalconEntity*	BaseObject(void) {return baseObject;};
    GroundListElement *GetNext () { return next; };
    void		HandoffBaseObject(void);
    
    GroundListElement*	next;
    enum { DataLink = 0x1, RangeRing = 0x2,
	Radiate    = 0x4,
	Track      = 0x8,
	Launch     = 0x10,
	UnChecked  = 0x20,
    };
    int		flags;
    int		symbol;
    float		range;
    VU_TIME		lastHit;
    void SetFlag(int flag) { flags |= flag; };
    int IsSet (int flag) { return (flags & flag) ? 1 : 0; };
    void ClearFlag (int flag) { flags &= ~flag; } ;
	//MI
	void ToggleFlag(int flag) {flags ^= flag;};
private:
    FalconEntity*	baseObject;
};

class FireControlComputer : public MfdDrawable
//me123 todo class FireControlComputer : public IrstClass
// ASSOCIATOR 03/12/03: Added the combined SnapShot LCOS Gunmode SSLC 
{
  public:
	enum FCCMasterMode {AAGun, AGGun, Missile, ILS, Nav, AirGroundBomb, AirGroundMissile, AirGroundHARM, AirGroundLaser, Dogfight,
		MissileOverride, AirGroundCamera, ClearOveride, AirGroundRocket/* PrevMode, NextModeCmd */ };
	enum FCCSubMode {EEGS, SSLC, LCOS, Snapshot, SAM, Aim9, Aim120, CCIP, CCRP, LADD, DTOSS, OBSOLETERCKT, STRAF, PRE,
		BSGT, SLAVE, HTS, TargetingPod, TimeToGo, ETE, ETA, MAN, GPS, HARM/* PrevSub, NextSub */ };

	enum FCCStptMode {FCCWaypoint, FCCDLinkpoint, FCCMarkpoint, FCCGMPseudoPoint};  // MD -- 20040215: adding for GM SP ground stabilization
 	enum HsdStates { 
	    HSDNONE = 0x0,
	    HSDCEN =	0x1, //centered display
		HSDCPL =    0x2,  // coupled to FCR radar
		HSDEXP1 =    0x4, 
		HSDEXP2 =    0x8, 
		HSDCNTL =    0x10, // config mode
		HSDFRZ =     0x20, // freeze mode
		HSDNOFCR =   0x40, // no fcr symbols
		HSDNOPRE =   0x80, // no preplanned symbols
		HSDNOAIFF =  0x100, // no aiff symbols
		HSDNOLINE1 = 0x200, // no line1 (FLOT)
		HSDNOLINE2 = 0x400, // no line 2
		HSDNOLINE3 = 0x800,
		HSDNOLINE4 = 0x1000,
		HSDNORINGS = 0x2000, // no range rings
		HSDNONAV1 =  0x4000, // no nav path 1 - normal
		HSDNONAV2 =  0x8000, // no nav path 2
		HSDNONAV3 =  0x10000, // no nav path 3
		HSDNOADLNK=  0x20000, // no ad link daat
		HSDNOGNDLNK= 0x40000, // no gnd link data
	};
	// MLR - the "last" nomenclature is a bit of a misnomer.
	// these vars indicate the current state of the various modes
	int			    lastAirAirHp,
		            lastAirGroundHp,
					lastDogfightHp,
					lastMissileOverrideHp;		// MLR 2/1/2004 

	FCCSubMode		lastAirAirSubMode,
					//lastAirGroundMissileSubMode, // MLR 4/11/2004 - 
					//lastAirGroundHARMSubMode,    // MLR 4/11/2004 - 
					lastAirGroundLaserSubMode,   // MLR 4/11/2004 - 
					//lastAirGroundCameraSubMode,  // MLR 4/11/2004 - 
					lastAirAirGunSubMode,		// MLR 2/6/2004 - 
		            lastAirGroundGunSubMode,	// MLR 2/6/2004 - 
					lastMissileOverrideSubMode,
					lastDogfightGunSubMode;		// MLR 4/1/2004 - 					

	bool			inAAGunMode, inAGGunMode;	// MLR 3/14/2004 - 

	FCCMasterMode GetLastMasterMode()	{return lastMasterMode;};
	BombClass *GetTheBomb();
	
	// ASSOCIATOR
	/*
	void SetLastNavMasterMode( FCCMasterMode mm ) { lastNavMasterMode = mm; };
	void SetLastAaMasterMode( FCCMasterMode mm )  { lastAaMasterMode = mm; };
	void SetLastAgMasterMode( FCCMasterMode mm )  { lastAgMasterMode = mm; };
	*/
	FCCMasterMode GetLastNavMasterMode() { return lastNavMasterMode; };
	/*
	FCCMasterMode GetLastAaMasterMode()	 { return lastAaMasterMode; };
	FCCMasterMode GetLastAgMasterMode()	 { return lastAgMasterMode; };
	*/

  private:
      enum { HSDRANGESIZE = 5 };
	FCCMasterMode	lastMasterMode;
	// ASSOCIATOR
	FCCMasterMode	lastNavMasterMode;
	FCCMasterMode	lastAgMasterMode;
	FCCSubMode		lastSubMode;
	int				lastCage, playerFCC, lastDesignate;
	int bombReleaseOverride;
	static struct HsdCnfgStates {
	    char *label;
	    HsdStates mode;
	} hsdcntlcfg[20]; // config for buttons in hsd
	float frz_x, frz_y, frz_dir;

	// Air - Ground Mode
	void AirGroundMode (void);
   void DTOSMode(void);
	void AirGroundMissileMode (void);
	void MaverickMode (void);
   SimObjectType* MavCheckLock (MissileClass*);
	void HarmMode (void);
	void TargetingPodMode (void);
	void NavMode (void);
	void CalculateImpactPoint(void);
   void CalculateRocketImpactPoint(void);
	void FindRelativeImpactPoint(void);
	void DelayModePipperCorrection(void);
	void CheckForBombRelease(void);
	void DesignateGroundTarget(void);
	void SetDesignatedTarget (void);
	void CalculateReleaseRange (void);
	void FindTargetError(void);
	int  FindGroundIntersection (float el, float az, float* x, float* y, float* z);
	void CheckFeatures (MissileClass* theMissile);
	void CheckFeatures (LaserPodClass* targetingPod);
	void UpdateGroundObjectRelativeGeometry (void);
	SimObjectType* TargetStep (SimObjectType*, int);

	// Steerpoints
	WayPointClass*	mpSavedWaypoint;
	int				mSavedWayNumber;
	FCCStptMode		mStptMode;
	FCCStptMode		mNewStptMode;

	void StepPoint(void);
	void InitNewStptMode(void);
	void StepNextWayPoint(void);
	void StepPrevWayPoint(void);

	// Air - Air Mode
	void AirAirMode(void);

	SimObjectType* targetPtr;
	SimObjectType* targetList;
	SimVehicleClass *platform;
	FCCMasterMode masterMode;
	FCCSubMode subMode;
	FCCSubMode dgftSubMode; // for dogfight mode missiles
	FCCSubMode mrmSubMode; // ASSOCIATOR 04/12/03: for remembering MRM mode missiles

	void Display(VirtualDisplay*);
	void DisplayInit (ImageBuffer*);
	float HSDRange;
	int HsdRangeIndex;
	static int HsdRangeTbl[HSDRANGESIZE];
	unsigned int hsdstates;

	void NavDisplay(void);
	void DrawNavPoints(void);
	void DrawWayPoints(void);
	void DrawMarkPoints(void);
	void DrawLinkPoints(void);
	void MapWaypointToDisplay(WayPointClass*, float*, float*);
	void DrawPointPair(WayPointClass*, float, float, float, float);
	void DrawPointSymbol(WayPointClass*, float, float);
	void DrawTGTSymbol(float, float);
	void DrawIPSymbol(float, float);
	void DrawMarkSymbol(float x, float y, int type);
	void DrawFLOT (void);
	void DrawPPThreats(void);
	void DrawBullseye(void);
	void DrawGhostCursor(void);
	void DrawScanVolume(void);
	void DrawBuggedTarget ();
	void DrawWingmen ();
	void Draw1Wingman (AircraftClass *wing);
	void DrawAIFF(void);//Cobra 11/27/04
	void Draw1WingmanGnd (AircraftClass *wing);

  public:

	WayPointClass* SavedWaypoint() { return mpSavedWaypoint; }
	int  xBombAccuracy, yBombAccuracy; // 2001-09-06 ADDED BY S.G. I'LL USE THIS INSTEAD OF autoTarget WHICH DOES NOTHING USEFULL
	char autoTarget;
	char releaseConsent, preDesignate, postDrop;
	char designateCmd, dropTrackCmd;
	char groundPipperOnHud, missileCageCmd, missileSpotScanCmd, missileSlaveCmd, missileTDBPCmd;  // Marco Edit
	char bombPickle, missileTarget, noSolution, waypointStepCmd;
	char HSDRangeStepCmd;
	int cursorXCmd, cursorYCmd; // MD -- 20040110: make the cursor commands int values to help analog axis integration
	int HSDCursorXCmd, HSDCursorYCmd;	//MI

	FireControlComputer (SimVehicleClass*, int);
	~FireControlComputer (void);
	enum TossAnticipation {NoCue, EarlyPreToss, PreToss, PullUp, AwaitingRelease};
	TossAnticipation tossAnticipationCue;
	int inRange;
	float verticalSteering, predictedClimbAngle, predictedReleaseAltitude;
	float missileTOF;
	int missileLaunched;
	VU_TIME lastMissileShootTime; //me123 addet
	float targetspeed;//me123 addet
	float lastMissileShootRng;//me123 addet
	float Height;//me123 addet
	float lastMissileShootHeight;//me123 addet
	float lastMissileShootEnergy;//me123 addet
	float missileMaxTof;//me123
	float lastMissileImpactTime;
	float nextMissileImpactTime;
	float lastmissileActiveTime;//me123 addet
	float missileActiveTime, missileActiveRange;
	float missileRMax, missileRMin, missileRneMax, missileRneMin;
	float missileSeekerAz, missileSeekerEl;
	float missileWEZDisplayRange;

	// RV - Biker - This is for new AMRAAM DLZ
	//float missileRtr;

	// RV - Biker - WEZ max/min data from FMs in feet	
	float missileWEZmax;
	float missileWEZmin;

	bool Aim9AtGround;	// Marco Edit - for whether AIM9 diamond
						// pointing at the ground or not

	float airGroundDelayTime, airGroundRange, airGroundBearing, airGroundMinRange, airGroundMaxRange;
	float groundImpactX, groundImpactY, groundImpactZ, groundImpactTime;
	float groundDesignateAz, groundDesignateEl, groundDesignateDroll;
	float groundDesignateX, groundDesignateY, groundDesignateZ;
	float groundPipperAz, groundPipperEl;
	VU_TIME MissileImpactTimeFlash;
	int LastMissileWillMiss(float);
	char subModeString[8];

	SMSClass* Sms;
	void SetSms(SMSClass *SMS);

	void ClearOverrideMode (void);
	void NextSubMode(void);
	void SetMasterMode(FCCMasterMode);
	void SetSubMode(FCCSubMode);
	void SetDgftSubMode (FCCSubMode dsm) {dgftSubMode = dsm;};
	void SetMrmSubMode (FCCSubMode msm) { mrmSubMode = msm;}; // ASSOCIATOR 04/12/03: for remembering MRM mode missiles
	void WeaponStep(void);
	SimObjectType* Exec (SimObjectType* curTarget, SimObjectType* targetList, PilotInputs* theInputs);
	FCCMasterMode GetMasterMode (void) {return (masterMode);};
	MASTERMODES GetMainMasterMode();
	int IsAGMasterMode() { return GetMainMasterMode() == MM_AG; };
	int IsAAMasterMode() { return GetMainMasterMode() == MM_AA; };
	int IsNavMasterMode() { return GetMainMasterMode() == MM_NAV; };
	FCCSubMode GetSubMode (void) {return (subMode);};
	FCCSubMode GetDgftSubMode (void) {return (dgftSubMode);};
	FCCSubMode GetMrmSubMode (void) {return (mrmSubMode);};  // ASSOCIATOR 04/12/03: for remembering MRM mode missiles
	//MI
	FCCSubMode GetDgftGunSubMode(void) {return (lastDogfightGunSubMode);};
	int PlayerFCC (void) {return playerFCC;};
	void SetPlayerFCC (int flag);
	SimObjectType* TargetPtr(void) {return targetPtr; };
	void ClearCurrentTarget (void);
	void SetTarget (SimObjectType* newTarget);
	float Aim120ASECRadius (float range);
	void PushButton (int whichButton, int whichMFD = 0);
	void SetStptMode(FCCStptMode);
   void SetWaypointNum (int num);
	FCCStptMode GetStptMode(void) {return mStptMode;};
	BOOL InTransistion(void) {return mStptMode != mNewStptMode;};
   void SetBombReleaseOverride (int newVal) {bombReleaseOverride = newVal;};
   void SetHsdState (HsdStates st) { hsdstates |= st; };
   void ToggleHsdState (HsdStates st) { hsdstates ^= st; };
   BOOL IsHsdState (HsdStates st) { return (hsdstates & st) == (unsigned int) st ? TRUE : FALSE; };
   void MissileLaunch();

	// stuff for preplanning etc.
	GroundListElement *grndlist;
	GroundListElement *GetFirstGroundElement() { return grndlist; };
	void AddGroundElement(GroundListElement *add) { add->next = grndlist; grndlist = add; };
	VU_TIME nextDlUpdate;
	void BuildPrePlanned ();
	void UpdatePlanned ();
	void ClearPlanned ();
	void PruneList ();
	static const int DATALINK_CYCLE;
	static const float MAXJSTARRANGESQ;
	static const float EMITTERRANGE;
	//MI Laser stuff
	bool LaserArm;
	bool LaserWasFired;
	bool CheckForLaserFire;
	bool LaserFire, ManualFire;
	bool InhibitFire;
	void ToggleLaserArm(void);
	float Timer;
	float ImpactTime;
	float LaserRange;
	float pitch, roll, yaw;
	void SetLastDesignate(void)	{lastDesignate = TRUE;};
	void RecalcPos(void);
	int time;

	//MI LADD
	void LADDMode(void);
	void CalculateLADDReleaseRange(void);
	enum LADDAnticipation {NoLADDCue, EarlyPreLADD, PreLADD, LADDPullUp, LADDAwaitingRelease};
	LADDAnticipation laddAnticipationCue;
	float SafetyDistance;
	//MI OA stuff
	void DrawDESTOAPoints(void);
	void DrawVIPOAPoints(void);
	void DrawVRPOAPoints(void);
	void DrawInterogaterPoints(void);
	void DrawDESTOASymbol(float displayX, float displayY);
	void DrawVIPOASymbol(float displayX, float displayY);
	void DrawVRPOASymbol(float displayX, float displayY);
	//MI SOI management
	bool IsSOI, CouldBeSOI;
	void ClearSOIAll(void)	{IsSOI = FALSE; CouldBeSOI = FALSE; HSDZoom = 0;};
	void ClearSOI(void)		{IsSOI = FALSE; HSDZoom = 0;};
	//HSD stuff
	int HSDZoom;
	void ToggleHSDZoom(void);
	void HSDDisplay(void);
	float xPos;	//position of the curson on the scope
	float yPos;
	void MoveCursor(void);
	int HSDDesignate;
	float curCursorRate;
	static const float CursorRate;
	void ChangeSTPT(WayPointClass *tmpWp);
	void MapWaypointToXY(WayPointClass *tmpWp);
	float DispX, DispY;
	void CheckPP(void);


	float HSDXPos;		//Wombat778 11-10-2003
	float HSDYPos;		//Wombat778 11-10-2003

	// MLR 2/1/2004 - Weapon/MasterMode compatibility

	// check to see if the WeaponClass is valid for the current master mode
	int WeaponClassMatchesMaster(WeaponClass wc);

	// Check to see if a weapon can be stepped to while in the current master 
	// mode, even if we must change master modes
	int CanStepToWeaponClass(WeaponClass wc);

	// set the Master Mode to be compatible for the current weapon
	// we need 2 different functions because of the damn gun
	void SetAAMasterModeForCurrentWeapon(void);
	void SetAGMasterModeForCurrentWeapon(void);

	//mrivers - thinking
	int  IsInAAMasterMode(void);
	int  IsInAGMasterMode(void);

	void EnterAAMasterMode(void);
	void EnterAGMasterMode(void);

	void EnterMissileOverrideMode(void);
	void EnterDogfightMode(void);

	void ToggleAAGunMode(void);
	void ToggleAGGunMode(void);

	DWORD GetPickleTime(void)	{ return PickleTimeToRelease; }

	// RV - I-Hawk - Allow "Maddog" fire with ARH only if missile is in boresight mode
	bool AllowMaddog (void);

private:
	void UpdateWeaponPtr(void); // MLR 3/16/2004 - updates fccWeaponPtr/rocketPointer as needed
	void UpdateLastData(void);  // MLR 4/12/2004 - used to update the "last" data (ie when weapons/MM change)

	// MLR 3/16/2004 - used instead of the weapon ptr on the hardpoint
	// sfr: using smartpointer
	int             fccWeaponId;
	VuBin<SimWeaponClass> fccWeaponPtr;

	// MLR 3/5/2004 - used for finding rocket impact position
	VuBin<MissileClass> rocketPointer;
	int			  rocketWeaponId;

	// COBRA - RED - The Pickle Time Stuff
	DWORD	PickleTimeToRelease;
};

extern const float RANGE_POSITION;
extern const float RANGE_MIDPOINT;

#define	DEFAULT_PICKLE	0
#define	SEC_1_PICKLE	1000
#define	PICKLE(x)		PickleTimeToRelease=x


#endif
