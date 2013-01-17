#ifndef _SIXDOF_H
#define _SIXDOF_H

#include "mathlib/math.h"
#include "mathlib/vector.h"
#include "mathlib/matrix.h"
#include "simlib.h"
#include "simmath.h"
#include "geometry.h"
#include "arfrmdat.h"
#include "campweap.h"
#include "fsound.h"
#include "lookuptable.h"
#include "sim/include/phyconst.h"


#ifndef _TYPES_H_
#include "graphics/include/grtypes.h"
#endif

#define OPTIMUM_ALPHA	7.0F//ME123 FROM 6
#define OPTIMUM_ALT_M1	-20.1225F
#define OPTIMUM_ALT_M2	-0.6774F
#define OPTIMUM_ALT_B	59784.5713F

class SimBaseClass;
class HeliMMClass;
class AircraftClass;

/*------------------------*/
/* Aerodynamics Data type */
/*------------------------*/
#ifdef USE_SH_POOLS
extern MEM_POOL	AirframeDataPool;
#endif

class AeroData
{
public:
	~AeroData (void) {delete mach; delete alpha; delete clift;
	delete cdrag; delete cy; };
	int numMach;
	int numAlpha;
	float *mach;
	float *alpha;
	float *clift;
	float *cdrag;
	float *cy;
	float clFactor;
	float cdFactor;
	float cyFactor;
#ifdef USE_SH_POOLS
public:
	// Overload new/delete to use a SmartHeap fixed size pool
	void *operator new(size_t size) { return MemAllocPtr(AirframeDataPool, size, 0);	};
	void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
};


struct DecoyDispenser
{
	Tpoint Pos[10],  // position of dispenser
		   Vec[10];  // vector (deltas) of fired decoy.
	int Decoys[10];   // decoys per dispenser
	// Current dispenser for next firing will need to be in the airframe object.
	int Count;       // number of dispensers 
	int Sequence;    // Firing sequence : 0 - alternate, 1 - sequential
};



// JB 010714 start
class AuxAeroData
{
public:
/*	AuxAeroData () {	JSBSimScript = 0; rollMomentum = 1; pitchMomentum = 1; yawMomentum = 1; pitchElasticity = 1; sinkRate = 15.0f; }
	~AuxAeroData() { free(JSBSimScript); }*/

	//engine
	float fuelFlowFactorNormal; // fuel flow rate for normal engine rates
	float fuelFlowFactorAb; // fuel flow rate for afterburner lit
	float minFuelFlow; // least possible fuel flow rate
	float normSpoolRate; // factor that the engines spool up/down by at normal operation
	float abSpoolRate; // factor the engines spool up/down by in the AB regime
	float jfsSpoolUpRate; // rate the engines spool up when JFS started
	float jfsSpoolUpLimit; // rpm percentage that JFS stops at
	float lightupSpoolRate; // spool up rate after engine is lit but before operating range
	float flameoutSpoolRate; // spool down rate when engine dies
	float jfsRechargeTime; // time jfs takes to recharge
	float jfsMinRechargeRpm; // min rpm required to recharge jfs
	float mainGenRpm; // rpm that main generator starts operating at
	float stbyGenRpm; // rpm that standby generator starts operating at.
	float epuBurnTime; // time the epu can burn fuel for in total
	float jfsSpinTime;	//MI how long the JFS runs with the hydraulic enegery
	int DeepStallEngineStall;
	int engineDamageStopThreshold; // 2002-04-11 ADDED BY S.G. Externalized the 13% chance of engine stop
	int engineDamageNoRestartThreshold; // 2002-04-11 ADDED BY S.G. Externalized the 50% chance of engine not able to restart
	int engineDamageHitThreshold; // 2002-04-11 ADDED BY S.G. Externalized the hitPoints 'theshold' being more than 0

#define AUX_LEFTEF_NONE	0
#define AUX_LEFTEF_MANUAL 1
#define AUX_LEFTEF_AOA	2
#define AUX_LEFTEF_MACH 3
#define AUX_LEFTEF_TEF 4
	//airframe
	float elevRate; // DPS of elevators
	int hasLef; // has LEF
	int hasTef; // has TEF
	int hasFlapperons; // has flapperons as opposed to separates.
	int hasSwingWing; // swing wing
	int isComplex; // complex model
	float tefMaxAngle; // divide angle of TEF by this to get amount of influence on CL and CD
	float lefMaxAngle; // divide angle of LEF by this to get amount of influence on CL and CD
	int tefNStages;
	int lefNStages; // number of stages of flaps
	float tefRate; // how fast the TEFs move.
	float tefTakeOff; // TEF angle for takeoff
	float lefRate; // how fast the TEFs move.
	float rudderMaxAngle;
	float aileronMaxAngle;
	float elevonMaxAngle;
	float airbrakeMaxAngle;
	float CLtefFactor; // how much the TEF affect the CL
	float CDtefFactor; // how much the TEF affect the CD
	float CDlefFactor; // how much the LEF affect the CD
	float CDSPDBFactor; // how much the speed brake affect drag
	float CDLDGFactor; // how much the landing gear affects drag
	float area2Span; // used to convert lift area into span
	float lefGround; // lef Position on the ground
	float lefMaxMach; // LEF maximum MACH
	int flapGearRelative; // if flaps only work with gear down
	float maxFlapVcas; // what Vcas flaps fully retract at
	float  flapVcasRange; // what range of Vcas flaps work over
	float rollMomentum;
	float pitchMomentum;
	float yawMomentum;
	float pitchElasticity;
	float sinkRate; // JPO - for landing
	float gearPitchFactor; // how much gear down affects AOA bias
	float rollGearGain; // how the gains change with landing gear down
	float yawGearGain; // how the gains change with landing gear down
	float pitchGearGain; // how the gains change with landing gear down
	float landingAOA; // JPO - what AOA required for landing
	float dragChuteCd; // extra CD for drag chute
	float dragChuteMaxSpeed; // how fast the drag chute can stand
	float rollCouple; // how much rudder affects roll
	int elevatorRolls; // elevator also responds to roll commands
	float canopyMaxAngle; // max angle of canopy when open
	float canopyRate; // deg/sec

	// fuel
	float fuelFwdRes; // forward resevoir size
	float fuelAftRes; // aft resevoir size (lbs)
	float fuelFwd1; // forward tank size
	float fuelAft1; // aft tank size
	float fuelWingAl; // left wing tank size
	float fuelWingFr; // right wing tank size
	float fuelFwdResRate; // transfer rate from fwd res
	float fuelAftResRate; // transfer rate from aft res
	float fuelFwd1Rate; // transfer rate from fwd
	float fuelAft1Rate; // transfer rate from aft
	float fuelWingAlRate; // transfer rate from left wing
	float fuelWingFrRate; // transfer rate from right wing
	float fuelClineRate; // transfer rate from center line
	float fuelWingExtRate; // transfer rate from ext wings
	float fuelMinFwd; // min fuel in the forward tanks to trigger warning
	float fuelMinAft; // min fuel in the forward tanks to trigger warning

	Tpoint gunLocation; // offset of the gun
	int gunTrailID;  // MLR 12/13/2003 - DrawableTrail ID
	float gunElevation;  // MLR 1/28/2004 - 
	float gunAzimuth;    // MLR 1/28/2004 - 
	int engineSmokes; // engine emits smoke in nonAB
	int nEngines; // count of engines
	Tpoint engineLocation[8]; // location of engines
	Tpoint wingTipLocation; // where the right wing tip is (left symmetric)
	Tpoint refuelLocation; // where the refuel point is.
	int nChaff; // number of chaff bundles
	int nFlare; // number of flares
	int hardpointrg[HARDPOINT_MAX];	

	//RV - I-Hawk - Added vortex positions for trails and PS vortex effects
	Tpoint vortex1Location; 
	Tpoint vortex2Location;
	Tpoint vortexPS1Location;
	Tpoint vortexPS2Location;
	Tpoint vortexPS3Location;
	int largeVortex; //being use as a bitwise variable for vortex effects size/type
	float vortexAOALimit; //AOA limit where AC stops generating vortex smoke

	// RV - I-Hawk - Added HTS ability for the advanced HTS code
	int HtsAble;
    	
	float MeanTimeBetweenFailures;	//Wombat778 2-24-04
	
	// sounds
    int sndInt; // MLR 2003-11-7 Internal engine sound - replaces Engine Noise in F4Browse if >0
	int sndAbInt; // SFX_BURNERI
	int sndInt2; // MLR 1/4/2004 - 
	int sndExt; // SFX_F16EXT
	int sndAbExt; // SFX_BURNERE
	int sndExt2; // MLR 1/4/2004 - 
	int sndWind; // SFX_ENGINEA

	LookupTable sndIntChart,
		        sndAbIntChart,
			    sndInt2Chart,
			    sndExtChart,
			    sndAbExtChart,
			    sndExt2Chart;

	LookupTable sndIntPitchChart,
		        sndAbIntPitchChart,
			    sndInt2PitchChart,
			    sndExtPitchChart,
			    sndAbExtPitchChart,
			    sndExt2PitchChart;


	float sndExternalVol; // MLR 2003-11 volume of exterior sounds when inside the pit

	int sndAero[4];   // MLR 1/7/2004 - Generic aerodynamic sounds (speed and AOA related)
	LookupTable sndAeroSpeedChart[4],
		        sndAeroAOAChart[4];

	int sndEject; // SFX_EJECT
	int sndSpdBrakeStart, sndSpdBrakeLoop, sndSpdBrakeEnd; // SFX_BRAKSTRT, SFX_BRAKLOOP, SFX_BRAKEND
	int sndSpdBrakeWind; // SFX_BRAKWIND
	int sndOverSpeed1, sndOverSpeed2; // SFX_OVERGSPEED1, SFX_OVERGSPEED2
	int sndGunStart, sndGunLoop, sndGunEnd; //SFX_VULSTART, SFX_VULLOOP, SFX_VULLOOPE
	int sndBBPullup; // SFX_BB_PULLUP
	int sndBBBingo; // SFX_BB_BINGO
	int sndBBWarning; // SFX_BB_WARNING
	int sndBBCaution; // SFX_BB_CAUTION
	int sndBBChaffFlareLow; //SFX_BB_CHFLLOW
	int sndBBFlare; // SFX_FLARE
	int sndBBChaffFlare; //SFX_BB_CHAFLARE
	int sndBBChaffFlareOut;//SFX_BB_CHFLOUT
	int sndBBAltitude; // SFX_BB_ALTITUDE
	int sndBBLock; // SFX_BB_LOCK
	int sndTouchDown; // SFX_TOUCHDOWN
	int sndWheelBrakes; // SFX_BIND
	int sndDragChute; // SFX_DRAGCHUTE
	int sndLowSpeed; // SFX_LOWSPDTONE
	int sndFlapStart, sndFlapLoop, sndFlapEnd; // SFX_FLAPSTRT, SFX_FLAPLOOP, SFX_FLAPEND
	int sndHookStart, sndHookLoop, sndHookEnd; // SFX_HOOKSTRT, SFX_HOOKLOOP, SFX_HOOKEND
	int sndGearCloseStart, sndGearCloseEnd; // SFX_GEARCST, SFX_GEARCEND
	int sndGearOpenStart, sndGearOpenEnd; // SFX_GEAROST, SFX_GEAROEND
	int sndGearLoop; // SFX_GEARLOOP

	// MLR 2/15/2004 
	int sndCanopyOpenStart;
	int sndCanopyOpenEnd;
	int sndCanopyCloseStart;
	int sndCanopyCloseEnd;
	int sndCanopyLoop;




	float rollLimitForAiInWP; // 2002-01-31 ADDED BY S.G. AI limition on roll when in waypoint (or similar) mode
	int flap2Nozzle; // for harrier - nozzles follow flaps HACK HACK HACK
	float startGroundAvoidCheck; // 2002-04-17 MN start ground avoid check only if closer than this distance to the ground
	int limitPstick; // 0 = use pStick 1.0f; 1 = use SetPstick function (old code - probably better for heavies)
	float refuelSpeed; // 2002-02-08 MN tanker speed for this aircraft when refueling
	float refuelAltitude; // 2002-02-08 MN tanker altitude for this aircraft when refueling
	int maxRippleCount; // 2002-02-23 MN maximum aircraft's ripple count (hardcoded 19 = F-16's max count)
	int largePark; // JPO - requires a large parking space
	float decelDistance; // 2002-03-05 MN different deceleration distances at refuel for each aircraft
	float followRate; // 2002-03-06 MN different follow rates for each aircraft
	float desiredClosureFactor; // 2002-03-08 MN another important factor for a smooth approach to the tanker
//	float IL78Factor; // 2002-03-09 MN for fixing "GivingGas" range factor
	float longLeg; // 2002-03-13 MN long leg for tanker track pattern
	float shortLeg; // 2002-03-13 MN short leg for tanker track pattern
	float refuelRate; // 2002-03-15 MN different aircraft have different refuel rates
	float AIBoomDistance; // 2002-03-28 MN hack to put the AI on the boom when in close range to it
	float BingoReturnDistance; // MN distance in nm to the closest friendly airbase at which AI is forced to go to RTB mode
	float jokerFactor; // 2002-03-12 MN default 2.0
	float bingoFactor; // 2002-03-12 MN default 5.0
	float fumesFactor; // 2002-03-12 MN default 15.0
	int	numBooms; // 18NOV03 - FRB  1 = has boom service
	int	numDrogues; // 18NOV03 - FRB >=1 = has drogue service	and how many stations
	int	activeDrogue; // 26NOV03 - FRB Slot number of drogue used for refueling
	float	DrogueExt; // 26NOV03 - FRB Drogue extension length (feet).
	float	BoomStoredAngle; // 12DEC03 - FRB Angle of boom in the stored position (deg. + = up)
	Tpoint DrogueRFPos; // 12DEC03 - FRB - a/c 0,0,0 refueling position relative to drogue pack (Slot)
	Tpoint BoomRFPos; // 12DEC03 - FRB - a/c 0,0,0 refueling position adjustments for different booms boom

	// RV - Biker - Catapult thrust multiplier
	float catapultThrustMultiplier;

	// RV - Biker - Data link capability level
	int dataLinkCapLevel;

	// RV - Biker - Enable lasing above 25k ft
	float maxLasingAlt;

	// RV - Biker - Check if AC can use programmable EWS
	int hasProgrammableEWS;

	// RV - Biker - Drogue and boom parents
	int boomParent;
	int drogueParent;

	// RV - Biker - Read switch and DOF for HPs from DAT files
	Tpoint hpSwitchDofRate[HARDPOINT_MAX];
	Tpoint gunSwitchDofRate;

	int refuelBoomSlot;
	int refuelDrogueSlot[3];

	int tankerType;

	//MI TFR stuff
	int Has_TFR;
	float PID_K;			//Proportianal gain in TFR PID pitch controler.
	float PID_KI;			//Intergral gain in TFR PID pitch controler
	float PID_KD;			//Differential gain in TFR PID pitch controler
	int TFR_LimitMX;	//Limit PID Integrator internal value so it doesn't get stuck in exteme.
	float TFR_Corner;	//Corner speed used in TFR calculations
	float TFR_Gain;		//Gain for calculating pitch error based on alt. difference
	float EVA_Gain;		//Pitch setpoint gain in EVA (evade) code
	float TFR_MaxRoll;	//Do not pull the stick in TFR if roll exceeds this value
	float TFR_SoftG;		//Max TFR G pull in soft mode
	float TFR_MedG;		//Max TFR G pull in medium mode
	float TFR_HardG;		//Max TFR G pull in hard mode
	float TFR_Clearance;	//Minimum clearance above the top of any obstacle [ft]
	float SlowPercent;		//Flash SLOW when airspeed is lower then this percentage of corner speed
	float TFR_lookAhead;	//Distance from ground directly under a/c used to measure ground inclination [ft]
	float EVA1_SoftFactor;	//Turnradius multiplier to get safe distance from ground for FLY_UP in SLOW
	float EVA2_SoftFactor;	//Turnradius multiplier to get safe distance from ground for OBSTACLE in SLOW
	float EVA1_MedFactor;	//Turnradius multiplier to get safe distance from ground for FLY_UP in MED
	float EVA2_MedFactor;	//Turnradius multiplier to get safe distance from ground for OBSTACLE in MED
	float EVA1_HardFactor;	//Turnradius multiplier to get safe distance from ground for FLY_UP in HARD
	float EVA2_HardFactor;	//Turnradius multiplier to get safe distance from ground for FLY_UP in MED
	float TFR_GammaCorrMult;	//Turnradius multiplier to get safe distance from ground for OBSTACLE in HARD
	float LantirnCameraX;	//Position of the camera
	float LantirnCameraY;
	float LantirnCameraZ;
	float minTGTMAR;		// 2002-03-22 ADDED BY S.G. Min TGTMAR for this type of aicraft
	float maxMARIdedStart;	// 2002-03-22 ADDED BY S.G. Max MAR for this type of aicraft when target is ID'ed and below 28K
	float addMARIded5k;		// 2002-03-22 ADDED BY S.G. Add MAR for this type of aicraft when target is ID'ed and below 5K
	float addMARIded18k;	// 2002-03-22 ADDED BY S.G. Add MAR for this type of aicraft when target is ID'ed and below 18K
	float addMARIded28k;	// 2002-03-22 ADDED BY S.G. Add MAR for this type of aicraft when target is ID'ed and below 28K
	float addMARIdedSpike;	// 2002-03-22 ADDED BY S.G. Add MAR for this type of aicraft when target is ID'ed and spiked

  // 2003-09-30 added by MLR to support Aircraft animations
  float animEngineRPMMult;  // Prop/Turbine multiplier. 
  float animSpoiler1Max;    // spoiler max angle extended
  float animSpoiler1Rate;   // spoiler motion rate
  float animSpoiler1OffAtWingSweep; // at what degree of wing sweep does this spoiler become inoperable
  int animSpoiler1AirBrake;         // spoiler funtions as airbrake aswell?
  float animSpoiler2Max;            // spoiler max angle extended
  float animSpoiler2Rate;           // spoiler motion rate
  float animSpoiler2OffAtWingSweep; // at what degree of wing sweep does this spoiler become inoperable
  int animSpoiler2AirBrake; //  spoiler funtions as airbrake aswell?
  float animExhNozIdle;     // Animation for exhaust nozzle DOF
  float animExhNozMil;      // 
  float animExhNozAB;       //
  float animExhNozRate;     //
  float animAileronRate;    // ailerons used TEFrate before which is to slow
  float animStrobeOnTime;   // Time for duration of the AntiCollision light(Tail top) ON sequence - Coment by MartinV
  float animStrobeOffTime;  // Time for duration of the AntiCollision light(Tail top) OFF sequence - Coment by MartinV
  float animWingFlashOnTime; // Time for duration of the Navigation light(wing/fuselage) ON sequence - MartinV
  float animWingFlashOffTime; // Time for duration of the Navigation light(wing/fuselage) OFF sequence - MartinV
  float animHookAngle;
  float animHookRate;
  	float animThrRevAngle; // FRB - Thrust reverser
  	float animThrRevRate;  // FRB - Thrust reverser
  int   animSwingWingStages; // new wing sweep animation, default values mimic MPS code.
  float animSwingWingRate;
  float animSwingWingMach[10];
  float animSwingWingAngle[10];
  float animWheelRadius[8]; // radius of wheels
  float animRefuelAngle; // 2003-10-12 MLR
  float animRefuelRate ;

  float fuelGaugeMultiplier; // MLR 2003-10-12 maybe this will make the fuel dial function properly? (def 10)

  float animGearMaxComp[8];  // 2003-10-14 maximum gear compression. ft
  float animGearMaxExt[8];   // 2003-10-14 maximum gear extention. ft

  struct DecoyDispenser Flare, Chaff; // MLR 2003-11-17

  Tpoint pilotEyePos;     // MLR 12/1/2003 - Exports pilots Eye view
  Tpoint swingWingHinge;  // MLR 3/5/2004 - 

  int typeAC; //TJL 02/08/04 Allows aircraft specific features based on number
  /*
  1 F16A/B
  2 F16C/D
  3 F15A/B
  4 F15C/D
  5	F15E
  6 F14A
  7 F14B/D
  8 F18A
  9 F18C/D
  10 F18E/F
  11 F4E
  12 A10
  */
  int typeEngine; //TJL 02/08/04 Allows engine specific features based on number
  /*
  1.F100-PW-100 2.F100-PW-220 3.F100-PW-229 4.F110-GE-100 5.F110-GE-129
  6.GE404-400 7.GE404-402 8.GE414-400 (F18E/F) 9. F110-GE-400 (F14B/D) 10. TF30-P-414A (F-14A/B)
  11. J79-GE-17 (F-4E) 12 TF34-GE-100 (A10)
  100. Generic Old Engine
  */

	float A2GJDAMAlt;// = 0.0f; //TJL 10/27/03 Sets AI JDAM attack altitude
	float A2GJSOWAlt;// = 0.0f; //TJL 10/27/03 Sets AI JSOW attack altitude
	float A2GHarmAlt;// = 0.0f; //TJL 10/27/03 Sets AI HARM attack altitude (all set to SP3 defaults)
	float A2GAGMAlt;// = 4000.0f; //TJL 10/27/03 Sets AI AGM attack altitude
	float A2GGBUAlt;// = 13000.0f; //TJL 10/27/03 Sets AI GBU attack altitude
	float A2GDumbHDAlt;// = 250.0f; //TJL 10/27/03 Sets AI Durandal attack altitude
	float A2GClusterAlt;// = 5000.0f; //TJL 10/27/03 Sets AI Cluster Bomb attack altitude
	float A2GDumbLDAlt;// = 11000.0f; //TJL 10/27/03 Sets AI Generic attack altitude
	float A2GGenericBombAlt;// = 11000.0f; //TJL 10/27/03 Sets AI Generic Bomb attack altitude
	float A2GGunRocketAlt;// = 1000.0f; //TJL 10/27/03 Sets AI Gun and Rocket altitude
	float A2GCameraAlt;// = 7000.0f; //TJL 10/27/03 Sets AI BDA/Recon altitude
	float A2GBombMissileAlt;// 
	//Cobra 10/30/04 TJL
    //    6,702.0	59,144.0	63,137.7	0.0			848.6		0.0  //f-16c slug/ft^2
	//float ixx,		iyy,		izz,		ixy,		ixz,		iyz;

	struct animIntakeData
    {
		TwoDimensionTable table;
    	float Rate;
    } animIntakeRamp[3];
 
 	float FTITStart;//TJL 09/11/04
 	float FTITIdle;
 	float FTITMax;
 	float criticalAOA;
	int hasIFF;//Cobra 11/20/04
	int hasThrRev;//Cobra

 
 
 	//char *JSBSimScript;

};
// JB 010714 end

/*------------------*/
/* Engine Data Type */
/*------------------*/
class EngineData
{
public:
	~EngineData();

	float thrustFactor;
	float fuelFlowFactor;
	int numMach;
	int numAlt;
	float *mach;
	float *alt;
	bool hasAB; // JB 010706
	float *thrust[3];
	float *fuelflow[3]; // MLR 5/16/2004 - 
	int hasFuelFlow;
#ifdef USE_SH_POOLS
public:
	// Overload new/delete to use a SmartHeap fixed size pool
	void *operator new(size_t size) { return MemAllocPtr(AirframeDataPool, size, 0);	};
	void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
};

/*---------------------*/
/* Roll Rate Data Type */
/*---------------------*/
class RollData
{
public:
	~RollData(void) {delete alpha; delete qbar; delete roll;};
	int numAlpha;
	int numQbar;
	float *alpha;
	float *qbar;
	float *roll;
#ifdef USE_SH_POOLS
public:
	// Overload new/delete to use a SmartHeap fixed size pool
	void *operator new(size_t size) { return MemAllocPtr(AirframeDataPool, size, 0);	};
	void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
};

class GearData
{
public:
	enum GearFlags{
		GearStuck	= 0x01,
		GearBroken	= 0x02,
		DoorStuck	= 0x04,
		DoorBroken	= 0x08,
		GearProblem = 0x0F,
	};
	GearData(void);
	~GearData(void) {}
	float	strength;	//how many hitpoints it has left
	float	vel;		//at what rate is it currently compressing/extending in ft/s
	float	obstacle;	//rock height/rut depth
	uint	flags;		//gear stuck/broken, door stuck/broken,
	float   WheelAngle; // the current angle the wheel is rotated. Used to position DOF
	float   WheelRPS;   // Wheel spins at Radians Per Sec. Used to spin wheel when not touching ground
	// sfr: not needed anymore
	//float   LastX;      // stored world position MLR 2003-10-04 - used to determin WheelRPS
	//float   LastY;
	float   StrutExtension; // visual extension to the strut.
#ifdef USE_SH_POOLS
public:
	// Overload new/delete to use a SmartHeap fixed size pool
	void *operator new(size_t size) { return MemAllocPtr(AirframeDataPool, size, 0);	};
	void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
};

class AirframeClass
{
	friend AircraftClass; // JB carrier

#ifdef USE_SH_POOLS
public:
	// Overload new/delete to use a SmartHeap fixed size pool
	void *operator new(size_t size) { ShiAssert( size == sizeof(AirframeClass) ); return MemAllocFS(pool);	};
	void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
	static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(AirframeClass), 200, 0 ); };
	static void ReleaseStorage()	{ MemPoolFree( pool ); };
	static MEM_POOL	pool;
#endif
	
private:
	// Airframe/Engine data
	AeroData          *aeroData;
	AuxAeroData		  *auxaeroData; // JB 010714
	EngineData        *engineData;
	RollData          *rollCmd;
	float area, fuel, fuelFlow, fuelFlow2, externalFuel, epuFuel;//TJL 01/11/04 Added fuelFlow2 Engine 2
	float mass, weight, dragIndex, emptyWeight;
	float gsAvail, maxGs, maxRoll, maxRollDelta, startRoll;
	float minVcas, maxVcas, cornerVcas;
	unsigned int flags;
	short	vehicleIndex;
	float fuelFlowSS, fuelFlowSS2; //TJL 04/25/04
	float ftitLeft;//TJL 09/11/04 //Cobra 10/30/04 TJL
 	float ftitRight;//TJL 09/11/04

	// A bit of a trick to get ground handling to be smooth at low speed
	float	groundAnchorX,	groundAnchorY;
	float	groundDeltaX,	groundDeltaY;

	// RV - Biker - Flag for carrier takeoff position taken
	int carrierStartPosEngaged;

	// Aerodynamics
	float clift0, clalph0;
	//float cdalpha;
	float cnalpha, clalpha;
	float aoabias;
	float cl,cd,cy, clalph;
	float aoamax, aoamin;
	float betmax, betmin;
	// Thrust Dynamics
	float thrtab; //Engine 1 Left
	float thrust; //TJL Default
	float thrust1;//TJL Engine 1
	float athrev, anozl, ethrst; //Engine 1
	// TJL 01/11/04 Multi-Engines
	float thrtab2; //Engine 2 Right
	float thrust2;// Engine 2 
	float athrev2, anoz2, ethrst2; //Engine 2
	DWORD engEventTimer;//TJL 02/23/04 Engine random timer eng 1
	DWORD engEventTimer2;//TJL 02/23/04 Engine random timer eng 2
	int engFlag1;//TJL 02/23/04
	int engFlag2;//TJL 02/23/04
	DWORD turbTimer;//TJL 03/14/04
	DWORD turbOn; //TJL 03/14/04 
	
	


	// Autopilot
	float forcedHeading, forcedSpeed;
	
	// Accelerometers
	float nxcgs, nycgs, nzcgs;
	float nxcgw, nycgw, nzcgw;
	
	// Filter Arrays
	//SAVE_ARRAY olde1, olde2, olde3, olde4;
	//SAVE_ARRAY oldvt, oldx,  oldy,  oldz;
public: // S.G. I NEED TO ACCES THEM FOR OTHSIDE AirframeClass FUNCTION
	SAVE_ARRAY oldp01, oldp02, oldp03, oldp04, oldp05;
	SAVE_ARRAY oldr01;
	//SAVE_ARRAY oldy02, oldy04;
	SAVE_ARRAY oldy01, oldy03, olda01;
	//SAVE_ARRAY ptrimArray, rtrimArray, ytrimArray;
	SAVE_ARRAY oldRpm;
	SAVE_ARRAY oldRpm2, oldp01Eng2, olda012; //TJL 01/11/04 Engine 2
	SAVE_ARRAY oldTurb1; //TJL 03/14/04
	SAVE_ARRAY oldRoll1; //TJL 03/23/04
	int GetHasAB() {return engineData->hasAB;};//Cobra
private:	
	// Normalization Params
	float qbar, qsom, qovt;
	
	// Control Inputs
	//float ptrim, rtrim, ytrim;
	float pshape, rshape, yshape;
	float pstab, rstab;
	float plsdamp, rlsdamp, ylsdamp;
	
	//control history
	//float rshape1, rshape2;
	//float pshape1, pshape2;
	//float yshape1, yshape2;
	float rshape1;
	float pshape1;
	float yshape1;
	float avgPdelta, avgRdelta , avgYdelta;
	
	//stalls
	float oscillationTimer;
	float oscillationSlope;
	
	float oldnzcgs;

	// Accelerations
	float xaero,  yaero,  zaero;
	float xsaero, ysaero, zsaero;
	float xwaero, ywaero, zwaero;
	float xprop,  yprop,  zprop;
	float xsprop, ysprop, zsprop;
	float xwprop, ywprop, zwprop;
	
	// Current interpolation breakpoints
	int curMachBreak, curAlphaBreak;
	int curRollAlphaBreak, curRollQbarBreak;
	int curEngMachBreak, curEngAltBreak;
	
	VU_TIME NextFailure;				//Wombat778 2-24-04

	// Simple flight model
	int simpleMode;	// 0 = no
	// Functions
	int ReadData(int idx);
	void SuperSimpleFCS (void);
	void SuperSimpleEngine (void);
	void Accelerometers (void);
	void Aerodynamics (void);
	void AeroRead(int);
	void AuxAeroRead(int); // JB 010714
	void Atmosphere(void);
	void Axial(float dt);
	void EngineRead(int);
	
	void CalcBodyRates(float dt);
	void EquationsOfMotion(float dt);
	void FcsRead(int);
	void FlightControlSystem(void);
	void Gains(void);
	void Initialize (void);
	void InitializeEOM (void);
	void ReInitialize(void);
	void OpenFiles(void);
	void Pitch(void);
	void ReadData(float *);
	void Roll(void);
	void Trigenometry(void);
	void TrimModel(void);
	void Yaw(void);
	float CalcMach (float GetKias, float pressRatio);
	float CalcPressureRatio(float alt, float* ttheta, float* rsigma);
	float Predictor(float x1, float x2, float y1, float y2);
	void SetStallConditions(void);
	void ResetIntegrators(void);
	float CalculateVt(float dt);
	void SetGroundPosition(float dt, float netAccel, float gndGmma, float relMu);
	void CalculateGroundPlane(float *gndGmma, float *relMu) const;
	void CalcGroundTurnRate(float dt);
	void RunLandingGear(void); // MLR 2003-10-15
	float RollInertia(float inertia); //TJL 12/11/03
	float Turbulence(float turb);//TJL 03/14/04

	
public:
	void TEFClose();
	void TEFMax();
	void TEFInc();
	void TEFDec();
	void TEFTakeoff();
	void TEFLEFStage1();
	void TEFLEFStage2();
	void TEFLEFStage3();
	void LEFClose();
	void LEFMax();
	void LEFInc();
	void LEFDec();
	void LEFTakeoff();
	void SetFlaps(bool islanding);
	void SetFlapsPlayer(void);//TJL 02/28/04
	void GetGunLocation(float *x, float *y, float *z) { *x = auxaeroData->gunLocation.x, *y = auxaeroData->gunLocation.y;*z = auxaeroData->gunLocation.z; };
	int EngineTrail();
	float EngineSmokeFactor();
	void GetRefuelPosition(Tpoint *pos) { *pos = auxaeroData->refuelLocation; };
	float GetRollLimitForAiInWP() { return auxaeroData->rollLimitForAiInWP; }; // 2002-01-01 ADDED BY S.G.
	float GetMinTGTMAR() { return auxaeroData->minTGTMAR; }; // 2002-03-22 ADDED BY S.G.
	float GetMaxMARIdedStart() { return auxaeroData->maxMARIdedStart; }; // 2002-03-22 ADDED BY S.G.
	float GetAddMARIded5k() { return auxaeroData->addMARIded5k; }; // 2002-03-22 ADDED BY S.G.
	float GetAddMARIded18k() { return auxaeroData->addMARIded18k; }; // 2002-03-22 ADDED BY S.G.
	float GetAddMARIded28k() { return auxaeroData->addMARIded28k; }; // 2002-03-22 ADDED BY S.G.
	float GetAddMARIdedSpike() { return auxaeroData->addMARIdedSpike; }; // 2002-03-22 ADDED BY S.G.

	int	GetEngineDamageStopThreshold() { return auxaeroData->engineDamageStopThreshold; }; // 2002-04-11 ADDED BY S.G.
	int	GetEngineDamageNoRestartThreshold() { return auxaeroData->engineDamageNoRestartThreshold; }; // 2002-04-11 ADDED BY S.G.
	int GetEngineDamageHitThreshold()  { return auxaeroData->engineDamageHitThreshold; }; // 2002-04-11 ADDED BY S.G.

	void GetDrogueRFPos(Tpoint *pos) { *pos = auxaeroData->DrogueRFPos; }; // 12DEC03 - FRB
	void GetBoomRFPos(Tpoint *pos) { *pos = auxaeroData->BoomRFPos; }; // 12DEC03 - FRB
	float GetBoomStoredAngle() { return auxaeroData->BoomStoredAngle; }; // 18NOV03 - FRB
	int GetnBooms() { return auxaeroData->numBooms; }; // 18NOV03 - FRB
	int GetnDrogues() { return auxaeroData->numDrogues; }; // 18NOV03 - FRB
	int GetActiveDrogue() { return auxaeroData->activeDrogue; }; // 26NOV03 - FRB
	float GetDrogueExt() { return auxaeroData->DrogueExt; }; // 29NOV03 - FRB
	float GetRefuelAngle() { return auxaeroData->animRefuelAngle; }; // 01DEC03 - FRB

	// RV - Biker - Boom and drogue parents
	int GetBoomParent() { return auxaeroData->boomParent; };
	int GetDrogueParent() { return auxaeroData->drogueParent; };

	// RV - Biker - Which slot boom and drogues should be placed at
	int GetRefuelBoomSlot() { return auxaeroData->refuelBoomSlot; };
	int GetRefuelDrogueSlot(int slot) { return auxaeroData->refuelDrogueSlot[slot]; };

	// RV - Biker - What tanker is it (does it have position lights)
	int GetTankerType() { return auxaeroData->tankerType; };

	// RV - Biker - Catapult thrust multiplier
	float GetCatapultThrustMultiplier() { return auxaeroData->catapultThrustMultiplier; };

	// RV - Biker - Data link capability level
	int GetDataLinkCapLevel() { return auxaeroData->dataLinkCapLevel; }

	// RV - Biker - Max lasing alt
	float GetMaxLasingAlt() { return auxaeroData->maxLasingAlt; };

	// RV - Biker - Check if AC can use programmable EWS
	int HasProgrammableEWS() { return auxaeroData->hasProgrammableEWS; };

	// RV - Biker - HP switch and DOF stuff
	int GetHpSwitchType(int numHP) { return int(auxaeroData->hpSwitchDofRate[numHP].x); };
	int GetHpDofType(int numHP) { return int(auxaeroData->hpSwitchDofRate[numHP].y); };
	float GetHpDofRate(int numHP) { return auxaeroData->hpSwitchDofRate[numHP].z; };

	// RV - Biker - Same for gun door
	int GetGunSwitchType() { return int(auxaeroData->gunSwitchDofRate.x); };
	int GetGunDofType() { return int(auxaeroData->gunSwitchDofRate.y); };
	float GetGunDofRate() { return auxaeroData->gunSwitchDofRate.z; };

	// RV-I-Hawk - The new HTS variable
	int GetIsHtsAble ( void ) { return auxaeroData->HtsAble; }

	//MI TFR
	bool HasTFR() { return auxaeroData->Has_TFR > 0;};
	float GetPID_K() {return auxaeroData->PID_K;};
	float GetPID_KI() {return auxaeroData->PID_KI;};
	float GetPID_KD() {return auxaeroData->PID_KD;};
	bool GetTFR_LimitMX() {return auxaeroData->TFR_LimitMX > 0;};
	float GetTFR_Corner() {return auxaeroData->TFR_Corner;};
	float GetTFR_Gain() {return auxaeroData->TFR_Gain;};
	float GetEVA_Gain() {return auxaeroData->EVA_Gain;};
	float GetTFR_MaxRoll() {return auxaeroData->TFR_MaxRoll;};
	float GetTFR_SoftG() {return auxaeroData->TFR_SoftG;};
	float GetTFR_MedG() {return auxaeroData->TFR_MedG;};
	float GetTFR_HardG() {return auxaeroData->TFR_HardG;};
	float GetTFR_Clearance() {return auxaeroData->TFR_Clearance;};
	float GetSlowPercent() {return auxaeroData->SlowPercent;};
	float GetTFR_lookAhead() {return auxaeroData->TFR_lookAhead;};
	float GetEVA1_SoftFactor() {return auxaeroData->EVA1_SoftFactor;};
	float GetEVA2_SoftFactor() {return auxaeroData->EVA2_SoftFactor;};
	float GetEVA1_MedFactor() {return auxaeroData->EVA1_MedFactor;};
	float GetEVA2_MedFactor() {return auxaeroData->EVA2_MedFactor;};
	float GetEVA1_HardFactor() {return auxaeroData->EVA1_HardFactor;};
	float GetEVA2_HardFactor() {return auxaeroData->EVA2_HardFactor;};
	float GetTFR_GammaCorrMult() {return auxaeroData->TFR_GammaCorrMult;};
	float GetLantirnCameraX() {return auxaeroData->LantirnCameraX;};
	float GetLantirnCameraY() {return auxaeroData->LantirnCameraY;};
	float GetLantirnCameraZ() {return auxaeroData->LantirnCameraZ;};

	float lastRStick;	//RAS 02Apr04 to improve grnd steering
	float lastYPedal;	//RAS 02Apr04
	
	void EngineModel(float dt);
	//TJL 01/11/04 Multi Engine Code
	void MultiEngineModel(float dt);
	int GetNumberEngines() {return auxaeroData->nEngines;};
	//TJL 02/21/04 Engine specific coding
	float EngineRpmMods (float rpmCmd);//For single engine code
	float Engine1RpmMods (float rpmCmd);//Engine 1
	float Engine2RpmMods (float rpmCmd2);//Engine 2
	int flapPos; //TJL 02/28/04
	float tefState;//TJL 02/28/04
	int GetTypeAC() {return auxaeroData->typeAC;};//TJL 02/28/04
	int GetTypeEngine() {return auxaeroData->typeEngine;};//TJL 02/28/04
	float GetFuelFlowLeft() {return fuelFlowSS;};//TJL 04/25/04
	float GetFuelFlowRight() {return fuelFlowSS2;};//TJL 04/25/04
	float GetFTITLeft() {return ftitLeft;};//TJL 09/11/04 //Cobra 10/30/04 TJL
	float GetFTITRight() {return ftitRight;};//TJL 09/11/04
	float CalcFtit(float tmpLeft, float tmpRight);//TJL 09/11/04
 	float FTITStart;//TJL 09/11/04 .dat file variables.
 	float FTITIDLE;
 	float FTITMax;
    

	void RandomFailureModel();	//Wombat778 2-24-04

	float Cd() { return cd; };
	float Cl() { return cl; };
	float Cy() { return cy; };
	float XAero() { return xaero; };
	float YAero() { return yaero; };
	float ZAero() { return zaero; };
	float XProp() { return xprop; };
	float YProp() { return yprop; };
	float ZProp() { return zprop; };
	float XSAero() { return xsaero; };
	float YSAero() { return ysaero; };
	float ZSAero() { return zsaero; };
	float XSProp() { return xsprop; };
	float YSProp() { return ysprop; };
	float ZSProp() { return zsprop; };
	float AOABias() { return aoabias; };

	float Thrust() { return thrust; };
	float Thrust2() { return thrust2; };//TJL 01/13/04 Engine 2

	float GetCanopyMaxAngle()	{ return auxaeroData->canopyMaxAngle; };  // MLR 11/x/2003 - 
	float GetSoundExternalVol() { return auxaeroData->sndExternalVol; };  // MLR 11/x/2003 - 
	Tpoint *GetPilotEyePos()    { return &auxaeroData->pilotEyePos;   };  // MLR 12/1/2003 - 

	int AnimPilotTime; // Cobra - time for next pilot animation act
	int AnimPilotAct; // Cobra - previous pilot animation action
	int AnimWSOTime; // Cobra - time for next copilot animation act
	int AnimWSOAct; // Cobra - previous copilot animation action
	int maxAnimPilotActs; // Cobra - max number of pilot animation actions
	int AnimPilotScenario; // Cobra - previous pilot animation routine
	int AnimWSOScenario; // Cobra - previous copilot animation routine
	int maxAnimPilotScenarios; // Cobra - max number of pilot animation routines
	int TheRoutine[5][10]; // Cobra - Array of pilot actions (PilotAction) (5 routines)
	enum PilotAction 
	{
		PA_None					= 0, 
		PA_Forward			= 1,
		PA_ForwardDown	=	2,
		PA_Left					= 3,
		PA_Right				=	4,
		PA_LeftBack			=	5, 
		PA_RightBack		=	6,
		PA_LeftBackUp		=	7,
		PA_RightBackUp	=	8,
		PA_ForwardUp		=	9,
		PA_BackUp				=	10,
		PA_End					=	11
	};
	

	/*------------------------*/
	/* Command mode constants */
	/*------------------------*/
	enum
	{
		IsDigital     = 0x1,
		InAir         = 0x2,
		Trimming      = 0x4,
		WheelBrakes   = 0x8,
		Refueling     = 0x10,
		AOACmdMode    = 0x20,
		AutoCommand   = 0x40,
		GCommand      = 0x80,
		ErrorCommand  = 0x100,
		GroundCommand = 0x200,
		AlphaCommand  = 0x400,
		GearBroken    = 0x800,
		Planted       = 0x1000,
		Simplified    = 0x2000,
		NoFuelBurn    = 0x4000,
		EngineOff     = 0x8000,
		ThrottleCheck = 0x10000,
		SuperSimple   = 0x20000,
		MPOverride	  = 0x40000,
		LowSpdHorn	  = 0x80000,
		HornSilenced  = 0x100000,
		CATLimiterIII = 0x200000,
		NoseSteerOn	  = 0x400000,
		OverRunway	  = 0x800000,
		HasComplexGear= 0x1000000,
		GearDamaged	  = 0x2000000,
		OverAirStrip  = 0x4000000,
		EngineStopped = 0x8000000,
		JfsStart = 0x10000000,
		// EpuRunning = 0x20000000, // no longer required
		EngineOff2 = 0x20000000, //TJL 01/11/04  Engine 2
		// JB carrier
		OnObject = 0x40000000,
		Hook = 0x80000000,
		// JB carrier
	};
		
	enum StallMode
	{
		None,
		Crashing,
		Recovering,
		EnteringDeepStall,
		DeepStall,
		Spinning,
		FlatSpin
	};
	// State data
	AircraftClass* platform;

	float e1, e2, e3, e4;
	BIG_SCALAR x, y, z;
	float vt, vcas, rho;
	float p, q, r;
	float mach, vRot;
	float alpha, beta;
	float theta, phi, psi;
	float gmma, sigma, mu;
	float nxcgb, nycgb, nzcgb;
	// sfr: same here
	SM_SCALAR xdot, ydot, zdot, vtDot;
	float rpm;
	float rpm2; //TJL 01/11/04 Engine 2
	//float limiterAssault;
	float stallMagnitude;
	float desiredMagnitude;
	float loadingFraction;
	float assymetry;
	float tefFactor;
	float lefFactor;
	float curMaxGs;
	StallMode	stallMode;
	float strFactor;//Cobra 10/30/04 TJL

	GearData	*gear;
   int groundType;
	float	grndphi, grndthe, groundZ;
	float	bumpphi, bumpthe, bumpyaw;
	Tpoint  gndNormal;
	
	// Geometry Stuff
	float alpdot;
	float betdot;
	float slice;
	float pitch;
	
	// Initialization Data
	float initialX;
	float initialY;
	float initialZ;
	float initialMach;
	float initialPsi;
	float initialFuel;
	
	// Flight Control System Gains
	float tp01, tp02, tp03, tp04;
	float zp01, zp02;
	float kp01, kp02, kp03, kp04, kp05, kp06;
	float wp01, wp02;
	int   jp01, jp02;
	float tr01;      
	float zr01;
	float kr01, kr02, kr03, kr04;
	float wr01;
	float ty01, ty02, ty03;
	float zy01, zy02;
	float ky01, ky02, ky03, ky04, ky05, ky06;
	float wy01, wy02;
	int   jy01, jy02;
	float zpdamp;
	int carrierLand;//Cobra
	
	// Pilot Commands
	float pstick, rstick, ypedal, throtl, pwrlev;
	float engine1Throttle, engine2Throttle; // TJL 01/11/04 Multi-Engine
	float pwrlevEngine1, pwrlevEngine2;
	float ptrmcmd, rtrmcmd, ytrmcmd;
	float dbrake, gearPos, gearHandle, speedBrake;
	float hookPos, hookHandle; // JB carrier
	bool altGearDeployed;
	float lefPos, tefPos; // JPO - for manual LEF/TEF
	enum DragChuteState {
	    DRAGC_STOWED = 0x00,
		DRAGC_DEPLOYED = 0x01,
		DRAGC_TRAILING = 0x02,
		DRAGC_JETTISONNED = 0x04,
		DRAGC_RIPPED = 0x08,
	};
	DragChuteState dragChute; // drag chute deployed or not.
	bool HasDragChute() { return auxaeroData->dragChuteCd > 0; };
	float DragChuteMaxSpeed() { return auxaeroData->dragChuteMaxSpeed; };
	bool HasManualFlaps() { return auxaeroData->hasTef == AUX_LEFTEF_MANUAL; };
	float TefDegrees() { return tefFactor * auxaeroData->tefMaxAngle; };
	float LefDegrees() { return lefFactor * auxaeroData->lefMaxAngle; };
	bool canopyState; // open or shut canopy
	void CanopyToggle();
	int thrustReverse;//Cobra
	int HasThrRev() { return auxaeroData->hasThrRev; };


	enum EpuState { // JPO state of the EPU switch
	    OFF, AUTO, ON
	} epuState;
	EpuState GetEpuSwitch () { return epuState; };
	void SetEpuSwitch (EpuState mode) { epuState = mode; };
	void StepEpuSwitch ();
	    // epu status and stuff
	enum EpuBurnState { EpuNone = 0x0, EpuHydrazine = 0x1, EpuAir = 0x2 };
	unsigned char epuBurnState;
	void EpuSetHydrazine() { epuBurnState |= EpuHydrazine; };
	void EpuSetAir() { epuBurnState |= EpuAir; };
	void EpuClear() { epuBurnState = EpuNone; };
	BOOL EpuIsAir() { return (epuBurnState & EpuAir) ? TRUE : FALSE; };
	BOOL EpuIsHydrazine() { return (epuBurnState & EpuHydrazine) ? TRUE : FALSE; };

	unsigned char hydrAB; // JPO - state of the hydraulics system
	enum {
	    HYDR_A_SYSTEM = 0x01, HYDR_B_SYSTEM = 0x02,
		HYDR_ALL = HYDR_A_SYSTEM|HYDR_B_SYSTEM,
		HYDR_A_BROKE = 0x04, HYDR_B_BROKE = 0x08, // whats permanently broke?
	};
	int HydraulicA() { return (hydrAB & HYDR_A_SYSTEM); };
	int HydraulicB() { return (hydrAB & HYDR_B_SYSTEM); };
	int HydraulicOK() { return hydrAB == HYDR_ALL ? 1 : 0; };
	void SetHydraulic(int type) { hydrAB = type; };
	void HydrBreak (int sys);
	void HydrDown (int sys) { hydrAB &= ~(sys & HYDR_ALL); };
	void HydrRestore (int sys);

	float jfsaccumulator;
	float JFSSpinTime;	//MI
	void JfsEngineStart (void);
	void QuickEngineStart();
	float curMaxStoreSpeed;//me123
	bool /*LLON,*/ PBON; //MI for LandingLight and Parkingbrake, sfr: removed LLON, aircraft has a flag for it
	bool BrakesToggle;	//MI for new Speedbrake
	void ToggleLL(void);
	void TogglePB(void);
	void ToggleHook(void); // JB carrier
	void HookUp(void); // MD
	void HookDown(void); // MD
	SAVE_ARRAY oldFtit, oldFtit2;//TJL 01/11/04 Engine 2
	float ftit; // Forward Turbine Inlet Temp (Degrees C) / 100 //TJL Engine 1
	float ftit2; // TJL 01/11/04 Multi-Engine Engine #2
	//Will add a flag here instead of making new ENUM list
	enum EngineFlags {
	    WingFirst = 0x1,
		MasterFuelOff = 0x2,
		FuelDoorOpen = 0x4,
		EngineStopped2 = 0x8,
	};
	unsigned int engineFlags;
	int IsEngineFlag(EngineFlags ef) { return (engineFlags & ef) ? 1 : 0; };
	void SetEngineFlag(EngineFlags ef) { engineFlags |= ef; };
	void ClearEngineFlag(EngineFlags ef) { engineFlags &= ~ ef; };
	void ToggleEngineFlag(EngineFlags ef) { engineFlags ^= ef; };
	enum FuelSwitch {
	    // ordered so that the first one is the defautl cockpit switch.
	    FS_NORM, FS_RESV, FS_WINGINT, FS_WINGEXT, FS_CENTEREXT, FS_TEST,
		FS_FIRST = FS_NORM, FS_LAST = FS_TEST, // for wrap around
	} fuelSwitch;
	void IncFuelSwitch();
	void DecFuelSwitch();
	void SetFuelSwitch(FuelSwitch fs) { fuelSwitch = fs; };
	FuelSwitch GetFuelSwitch() { return fuelSwitch; };
	enum FuelPump {
	    // ordered so that the first one is the defautl cockpit switch.
	     FP_OFF, FP_NORM, FP_AFT, FP_FWD,
		 FP_FIRST = FP_OFF, FP_LAST = FP_FWD,
	} fuelPump;
	void IncFuelPump();
	void DecFuelPump();
	void SetFuelPump(FuelPump fp) { fuelPump = fp; };
	FuelPump GetFuelPump() { return fuelPump; };
	enum { TANK_FWDRES, TANK_AFTRES, TANK_F1, TANK_A1, TANK_WINGFR, TANK_WINGAL,
	TANK_MAXINTERNAL = TANK_WINGAL, TANK_REXT, TANK_LEXT, TANK_CLINE, MAX_FUEL};
	float m_tanks[MAX_FUEL]; // tank current capacity
	float m_tankcap[MAX_FUEL]; // tank max capacity
	float m_trate[MAX_FUEL]; // tank transfer rate (the from tank).
	float AvailableFuel();
	float GetJoker();
	float GetBingo();
	float GetFumes();
	int CheckTrapped();
	int CheckHome();
	int HomeFuel;
	enum AirSource {
	    // ordered so that the first one is the defautl cockpit switch.
	    AS_OFF, AS_NORM, AS_DUMP, AS_RAM,
		AS_FIRST = AS_OFF, AS_LAST = AS_RAM
	} airSource;
	void IncAirSource();
	void DecAirSource();
	void SetAirSource(AirSource as) { airSource = as; };
	AirSource GetAirSource() { return airSource; };
	
	enum Generator { // come in pairs - first is active/inactive 2nd ok/broke
	    GenNone = 0x0,
	    GenFlcsPmg = 0x1,
	    GenFlcsPmgBroke = 0x2,
	    GenEpu = 0x4,
	    GenEpuBroke = 0x8,
	    GenEpuPmg = 0x10,
	    GenEpuPmgBroke = 0x20,
	    GenStdby = 0x40,
	    GenStdbyBroke = 0x80,
	    GenStdbyPmg = 0x100,
	    GenStdbyPmgBroke = 0x200,
	    GenMain = 0x400,   // MD -- 20031011: added missing zero to deconflict with GenStdby
	    GenMainBroke = 0x800,  // MD -- 20031011: added missing zero to deconflict with GenStdbyBroke
	};
	unsigned int generators;
	BOOL GeneratorRunning(Generator gen) { return (generators & gen) ? TRUE : FALSE; };
	BOOL GeneratorOK(Generator gen) { return (generators & (gen<<1)) ? FALSE : TRUE; };
	void GeneratorOn (Generator gen) { if(GeneratorOK(gen)) generators |= gen; };
	void GeneratorOff (Generator gen) { generators &= ~gen; };
	void GeneratorBreak(Generator gen) { generators |= (gen<<1); GeneratorOff(gen);  };

	float nozzlePos;

	// Functions
	AirframeClass (AircraftClass* self);
	~AirframeClass (void);
	void Init (int idx);
	void InitData (int idx);
	void Reinit (void);
	void Exec(void);
	void RemoteUpdate(void);
	void ResetOrientation(void);
	void CalcBodyOrientation(float dt);
	
	void YawIt(float betcmd, float dt);
	void PitchIt(float aoacmd, float dt);
	void RollIt(float pscmd, float dt);
	float CheckHeight(void) const;
	void CheckGroundImpact(float dt);
	
	float	GetOptimumCruise(void);
	float	GetOptimumAltitude(void);
	float	CalcThrotlPos(float speed);
	float	CalcMuFric(int groundType);
	float	CalcDesAlpha(float desGs);
	float	CalcDesSpeed(float desAlpha);
	void SetPStick (float newStick) {pstick = newStick;};
	void SetRStick (float newStick) {rstick = newStick;};
	void SetYPedal (float newPedal) {ypedal = newPedal;};
	void SetThrotl (float newThrot) {throtl = newThrot;};
	float PStick (void) {return pstick;};
	float RStick (void) {return rstick;};
	float YPedal (void) {return ypedal;};
	float Throtl (void) {return throtl;};
	float Mass (void)		{return mass;}
	float MaxRoll (void) {return maxRoll * RTD;};
	void  SetMaxRoll (float newRoll) {maxRoll = newRoll * DTR;};
	void  SetMaxRollDelta (float newRoll) {maxRollDelta = newRoll * DTR; startRoll = 0.0F;};
	void  ReSetMaxRoll (void);
	float MaxGs (void) {return maxGs;};
	float MinVcas(void) {return minVcas;}
	float MaxVcas(void) {return maxVcas;}
	float CalcTASfromCAS(float cas);
	float CornerVcas(void) {return cornerVcas;}
	float GsAvail (void) {return gsAvail;};
	float SustainedGs (int maxAB);
	float PsubS (int maxAB);
	float FuelBurn (int maxAB);
	float Fuel (void) {return fuel;};
	float ExternalFuel (void) {return externalFuel;};
	void  AddExternalFuel (float lbs);
	int  AddFuel (float lbs); //this checks whether we're full
	void ClearFuel (void);
	void AllocateFuel (float totalfuel);
	int BurnFuel (float bfuel);
	void RecalculateFuel(void);
	void FeedTank(int t1, int t2, float dt);
	float FuelFlow (void) {return fuelFlow;};
	void FuelTransfer (float dt);
	void DropTank (int n);
	void GetFuel(float *fwdp, float *aftp, float *total);
	void FindExternalTanks (void);
	float EPUFuel (void) {return epuFuel;};
	float VtDot(void) {return vtDot;};
	float WingArea (void) {return area;};
	int	VehicleIndex(void) {return vehicleIndex;}
	void  AddWeapon(float weight,float dragIndex, float offset);
	void  RemoveWeapon(float Weight, float DragIndex, float offset);
	void SetForcedConditions (float newSpeed, float newHeading) {forcedSpeed = newSpeed; forcedHeading = newHeading;};
	float Qsom(void)	{return qsom;}
	float Cnalpha(void) {return cnalpha;}
	int NumGear(void) const	{return FloatToInt32(aeroDataset[vehicleIndex].inputData[AeroDataSet::NumGear]);}
	float GetAeroData(int which) const { return aeroDataset[vehicleIndex].inputData[which]; }
	void DragBodypart(void);
	
	void SetDragIndex(float index) {dragIndex = index;};
	float GetDragIndex(void) {return dragIndex;};
	float GetMaxCurrentRollRate();
	void ResetAlpha(void);
	void SetFlag (int newFlag) {flags |= newFlag;};
	void ClearFlag (int newFlag) {flags &= ~newFlag;};
	int IsSet (int testFlag) const {return flags & testFlag ? 1 : 0;}
	// KCK added function - this is really only needed temporarily
	void SetPosition (float x, float y, float z);
	void ResetFuel (void);
	
	// simple flight mode stuff....
	void SimpleModel( void );
	void SetSimpleMode( int );
	int  GetSimpleMode(void) {return simpleMode;};

	//MI
	float GetOptKias(int mode);// climb, cruice end, cruice rng  0,1,2
	float GetLandingAoASpd() { return CalcDesSpeed(auxaeroData->landingAOA); };
	int GetRackGroup(int i) { return auxaeroData->hardpointrg[i]; };
	int GetBingoSnd() { return auxaeroData->sndBBBingo; };
	int GetAltitudeSnd() { return auxaeroData->sndBBAltitude; };
#define	SIMPLE_MODE_OFF	0
#define	SIMPLE_MODE_AF	1
#define	SIMPLE_MODE_HF	2
			
	// easter egg -- helicopter flight model.....
	void RunHeliModel(void);
	HeliMMClass *hf;
	float onObjectHeight; // JB carrier

	// MN
	float GetRefuelSpeed(void) {return auxaeroData->refuelSpeed; }
	float GetRefuelAltitude(void) {return auxaeroData->refuelAltitude; }
	int GetMaxRippleCount(void) {return auxaeroData->maxRippleCount; }
	int GetParkType();
	float GetDecelerateDistance(void) {return auxaeroData->decelDistance; }
	float GetRefuelFollowRate(void) {return auxaeroData->followRate; }
	float GetDesiredClosureFactor(void) {return auxaeroData->desiredClosureFactor; }
//	float GetIL78Factor(void) {return auxaeroData->IL78Factor; }
	float GetBingoReturnDistance(void) {return auxaeroData->BingoReturnDistance; }
	float GetTankerLongLeg(void) {return auxaeroData->longLeg; }
	float GetTankerShortLeg(void) {return auxaeroData->shortLeg; }
	float GetRefuelRate(void) {return auxaeroData->refuelRate; }
	float GetAIBoomDistance(void) {return auxaeroData->AIBoomDistance; }
	float GetStartGroundAvoidCheck(void) {return auxaeroData->startGroundAvoidCheck; }
	int LimitPstick(void) {return auxaeroData->limitPstick;}

	float gearExtension[8];  // MLR 2/22/2004 - this will be used instead of using the drawable's DOF values
};

#endif
