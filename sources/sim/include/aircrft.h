#ifndef _AIRCRAFT_CLASS_H
#define _AIRCRAFT_CLASS_H


#include "simVeh.h"
#include "hardpnt.h"
#include "fsound.h"
#include "dofsnswitches.h"

#define FLARE_STATION	0
#define CHAFF_STATION	1
#define DEBRIS_STATION	2

#define NEW_VORTEX_TRAILS 1 //RV - I-Hawk - new vortex trails implementation

////////////////////////////////////////////
struct DamageF16PieceStructure {
	int	mask;
	int	index;
	int	damage;
	int sfxflag;
	int	sfxtype;
	float lifetime;
	float dx, dy, dz;
	float yd, pd, rd;
	float pitch, roll;	// resting angle
};
////////////////////////////////////////////

// fwd class pointers used
class GunClass;
class FireControlComputer;
class AirframeClass;
class SimObjectType;
class WeaponStation;
class DrawableTrail;
class CautionClass;
class FackClass;
class SMSClass;
class DigitalBrain;
class TankerBrain;
class DrawableGroundVehicle;
class ObjectiveClass;
class AircraftTurbulence;
class SimVehicleClass;


class AircraftClass : public SimVehicleClass {
friend class AirframeClass;	
#ifdef USE_SH_POOLS
public:
	// Overload new/delete to use a SmartHeap fixed size pool
	void *operator new(size_t size) { ShiAssert( size == sizeof(AircraftClass) ); return MemAllocFS(pool);	};
	void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
	static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(AircraftClass), 200, 0 ); };
	static void ReleaseStorage()	{ MemPoolFree( pool ); };
	static MEM_POOL	pool;
#endif

public:
	enum AutoPilotType	{ThreeAxisAP, WaypointAP, CombatAP, LantirnAP, APOff};
	// this is extremely stupid, since it allows a plane to be both an A10 and an F16.
	// wrong way to do things.
	enum ACFLAGS {
		isF16    = 0x01,     // is an F-16 - hopefully historic usage only soon
		hasSwing = 0x02,     // has swing wing
		isComplex = 0x04,    // has a complex model (lots of dofs and switches)
		InRecovery = 0x08,   // recovering from gloc
		isA10 = 0x10,        // TJL 01/11/04 In case we have to merge with OIR
		hasTwoEngines = 0x20 // TJL 01/11/04 
	};
	enum AvionicsPowerFlags {
		NoPower = 0,
		SMSPower = 0x1, FCCPower = 0x2, MFDPower = 0x4, UFCPower = 0x8, 
		GPSPower = 0x10, DLPower = 0x20, MAPPower = 0x40,
		LeftHptPower = 0x80, RightHptPower = 0x100,
		TISLPower = 0x200, FCRPower = 0x400, HUDPower = 0x800,
		//MI
		EWSRWRPower = 0x1000, EWSJammerPower = 0x2000, EWSChaffPower = 0x4000,
		EWSFlarePower = 0x8000,
		//MI
		RaltPower = 0x10000, 
		RwrPower  = 0x20000,
		APPower = 0x40000,
		PFDPower = 0x80000,
		ChaffFlareCount = 0x100000,
		//MI
		IFFPower = 0x200000,
		// systems that don't have power normally.
		SpotLightPower = 0x20000000,
		InstrumentLightPower = 0x40000000, 
		InteriorLightPower   = 0x80000000, // start from the top down
		AllPower = 0xffffff // all the systems have power normally.
	};
	enum MainPowerType { MainPowerOff, MainPowerBatt, MainPowerMain };
	// start of the power state matrix,
	// will get filled in more when I know what I'm talking about a little.
	enum PowerStates { PowerNone = 0, 
		PowerFlcs,
		PowerBattery,
		PowerEmergencyBus,
		PowerEssentialBus,
		PowerNonEssentialBus,
		PowerMaxState,
	};
	enum LightSwitch { LT_OFF, LT_LOW, LT_NORMAL };
	LightSwitch interiorLight, instrumentLight, spotLight;
	void SetInteriorLight(LightSwitch st) { interiorLight = st; if (st){ SetAcStatusBits(ACSTATUS_PITLIGHT); } };
	void SetInstrumentLight(LightSwitch st) { instrumentLight = st; };
	void SetSpotLight(LightSwitch st) { spotLight = st; };
	LightSwitch GetInteriorLight() const { return interiorLight; };
	LightSwitch GetInstrumentLight() const { return instrumentLight; };
	LightSwitch GetSpotLight() const { return spotLight; };

	//MI
	enum EWSPGMSwitch { Off, Stby, Man, Semi, Auto };
	//MI new OverG/Speed stuff
	void CheckForOverG(void);
	void CheckForOverSpeed(void);
	void DoOverGSpeedDamage(int station);
	void StoreToDamage(WeaponClass thing);
	unsigned int StationsFailed;
	enum StationFlags {
		Station1_Degr = 0x1,
		Station2_Degr = 0x2,
		Station3_Degr = 0x4,
		Station4_Degr = 0x8,
		Station5_Degr = 0x10,
		Station6_Degr = 0x20,
		Station7_Degr = 0x40,
		Station8_Degr = 0x80,
		Station9_Degr = 0x100,
		Station1_Fail = 0x200,
		Station2_Fail = 0x400,
		Station3_Fail = 0x800,
		Station4_Fail = 0x1000,
		Station5_Fail = 0x2000,
		Station6_Fail = 0x4000,
		Station7_Fail = 0x8000,
		Station8_Fail = 0x10000,
		Station9_Fail = 0x20000,
	};			
	void StationFailed(StationFlags fl) { StationsFailed |= fl; };
	int GetStationFailed(StationFlags fl) { return (StationsFailed & fl) == (unsigned int)fl ? 1 : 0; };

	/** status bits for the aircraft */
	enum AircraftStatusBit {
		//VIS_TYPE_MASK = 0x07,
		ACSTATUS_PILOT_EJECTED       = 0x00000001,
		ACSTATUS_GEAR_DOWN           = 0x00000002,
		ACSTATUS_EXT_LIGHTS          = 0x00000004,
		ACSTATUS_EXT_NAVLIGHTS       = 0x00000008,
		ACSTATUS_EXT_TAILSTROBE      = 0x00000010,
		ACSTATUS_EXT_LANDINGLIGHT    = 0x00000020,
		ACSTATUS_EXT_NAVLIGHTSFLASH  = 0x00000040,
		ACSTATUS_CANOPY              = 0x00000080,
		ACSTATUS_PITLIGHT            = 0x00000100,
	};
	/** aircraft dirtyness */
	enum DirtyAircraft {
		DIRTY_ACSTATUS_BITS = 1
	};
	// dirty functions
	void MakeAircraftDirty(DirtyAircraft bits, Dirtyness score);
	void WriteDirty(unsigned char **stream);
	void ReadDirty(unsigned char **stream, long *rem);
	// status bits functions, receiving a combination of AircraftStatusBit bitwised
	void SetAcStatusBits(int bits);
	void ClearAcStatusBits(int bits);
	bool IsAcStatusBitsSet(int bits) const { return (status_bits & bits) == bits; }
private:
	int status_bits;    ///< bitwise of DirtyAircraft
	char dirty_aircraft; ///< aircraft dirtyness, bitwise of DirtyAircraft
public:


	// sfr: removed. using base class status... avoid duplication
	//unsigned int ExteriorLights;
	enum ExtlLightFlags 
	{
		Extl_Main_Power		= ACSTATUS_EXT_LIGHTS,
		Extl_Anti_Coll		= ACSTATUS_EXT_TAILSTROBE,
		Extl_Wing_Tail		= ACSTATUS_EXT_NAVLIGHTS,
		Extl_Flash			= ACSTATUS_EXT_NAVLIGHTSFLASH,
	};			
	void ExtlOn(ExtlLightFlags fl) { SetAcStatusBits(fl); }
	void ExtlOff(ExtlLightFlags fl){ ClearAcStatusBits(fl); }
	int ExtlState(ExtlLightFlags fl) const { return (IsAcStatusBitsSet(fl) ? 1 : 0); }

	//MI
	unsigned int INSFlags;
	enum INSAlignFlags 
	{
		INS_PowerOff		= 0x1,
		INS_AlignNorm		= 0x2,
		INS_AlignHead		= 0x4,
		INS_Cal				= 0x8,
		INS_AlignFlight		= 0x10,
		INS_Nav				= 0x20,
		INS_Aligned			= 0x40,
		INS_ADI_OFF_IN		= 0x80,
		INS_ADI_AUX_IN		= 0x100,
		INS_HUD_FPM			= 0x200,
		INS_HUD_STUFF		= 0x400,
		INS_HSI_OFF_IN		= 0x800,
		INS_HSD_STUFF		= 0x1000,
		BUP_ADI_OFF_IN		= 0x2000,
	};			
	void INSOn(INSAlignFlags fl) { INSFlags |= fl; };
	void INSOff(INSAlignFlags fl) { INSFlags &= ~fl; };
	int INSState(INSAlignFlags fl) { return (INSFlags & fl) == (unsigned int)fl ? 1 : 0; };
	void RunINS(void);
	void DoINSAlign(void);
	void SwitchINSToAlign(void);
	void SwitchINSToNav(void);
	void SwitchINSToOff(void);
	void SwitchINSToInFLT(void);
	void CheckINSStatus(void);
	void CalcINSDrift(void);
	void CalcINSOffset(void);
	float GetINSLatDrift(void)	{return (INSLatDrift + INSLatOffset);};
	float GetINSLongDrift(void)	{return (INSLongDrift + INSLongOffset);};
	float GetINSAltOffset(void)	{return INSAltOffset;};
	float GetINSHDGOffset(void)	{return INSHDGOffset;};
	float INSAlignmentTimer;
	VU_TIME INSAlignmentStart;
	bool INSAlign, HasAligned;
	int INSStatus;
	float INSLatDrift;
	float INSLongDrift;
	float INSTimeDiff;
	bool INS60kts, CheckUFC;
	float INSLatOffset, INSLongOffset, INSAltOffset, INSHDGOffset;
	int INSDriftLatDirection, INSDriftLongDirection;
	float INSDriftDirectionTimer;
	float BUPADIEnergy;
	bool GSValid, LOCValid;

	//MI more realistic AVTR
	unsigned int AVTRFlags;
	float AVTRCountown;
	bool doAVTRCountdown;
	void AddAVTRSeconds(void);
	enum AVTRStateFlags	{
		AVTR_ON	= 0x1,
		AVTR_AUTO = 0x2,
		AVTR_OFF	= 0x4,
	};
	void AVTROn(AVTRStateFlags fl) { AVTRFlags |= fl; };
	void AVTROff(AVTRStateFlags fl) { AVTRFlags &= ~fl; };
	int AVTRState(AVTRStateFlags fl) { return (AVTRFlags & fl) == (unsigned int)fl ? 1 : 0; };


	//MI Mal and Ind Lights test button
	bool TestLights;
	//MI some other stuff
	bool TrimAPDisc;
	bool TEFExtend;
	bool CanopyDamaged;
	bool LEFLocked;
	float LTLEFAOA, RTLEFAOA;
	unsigned int LEFFlags;
	float leftLEFAngle;
	float rightLEFAngle;
	enum LEFStateFlags
	{
		LT_LEF_DAMAGED	= 0x1,
		LT_LEF_OUT		= 0x2,
		RT_LEF_DAMAGED	= 0x8,
		RT_LEF_OUT		= 0x10,
		LEFSASYNCH		= 0x20,
	};
	void LEFOn(LEFStateFlags fl) { LEFFlags |= fl; };
	void LEFOff(LEFStateFlags fl) { LEFFlags &= ~fl; };
	int LEFState(LEFStateFlags fl) { return (LEFFlags & fl) == (unsigned int)fl ? 1 : 0; };
	float CheckLEF(int side);

	int MissileVolume;
	int ThreatVolume;
	bool GunFire;
	//targeting pod cooling time
	float PodCooling;
	bool CockpitWingLight;
	bool CockpitWingLightFlash; // martinv
	bool CockpitStrobeLight;
	void SetCockpitWingLight(bool state);
	void SetCockpitWingLightFlash(bool state); // martinv
	void SetCockpitStrobeLight(bool state);

	// MLR 2003-10-12
	void CopyAnimationsToPit(DrawableBSP *Pit);
	//TJL 08/08/04 Hotpit Refueling //Cobra 10/30/04 TJL
	void HotPitRefuel(void);
	bool requestHotpitRefuel;
	int iffEnabled;//Cobra 11/20/04
	virtual int GetiffEnabled (void){ return iffEnabled; } //Cobra 11/20/04
	bool interrogating;//Cobra 11/21/04
	bool runIFFInt;//Cobra 11/21/04
	float iffModeTimer;//Cobra 11/21/04
	int iffModeChallenge;//Cobra 11/21/04

	//sfr: added rem
	AircraftClass		(int flag, VU_BYTE** stream, long *rem);
	AircraftClass		(int flag, FILE* filePtr);
	AircraftClass		(int flag, int type);
	virtual ~AircraftClass(void);
	virtual void InitData();
	virtual void CleanupData();
private:
	void InitLocalData(int);
	void CleanupLocalData();
public:

	float					glocFactor;
	int						fireGun, fireMissile, lastPickle;
	FackClass*				mFaults;
	AirframeClass*			af;
	FireControlComputer*	FCC;
	SMSClass*				Sms;
	GunClass*				Guns;

	virtual void	Init (SimInitDataClass* initData);
	virtual int	   Wake(void);
	//virtual int 	Sleep(void);
	virtual int		Exec (void);
	virtual void	JoinFlight (void);
	virtual int    CombatClass (void); // 2002-02-25 MODIFIED BY S.G. virtual added in front since FlightClass also have one now...
	void           SetAutopilot (AutoPilotType flag);
	AutoPilotType	AutopilotType (void) {return autopilotType;};
	VU_ID	         HomeAirbase(void);
	VU_ID	         TakeoffAirbase(void);
	VU_ID	         LandingAirbase(void);
	VU_ID	         DivertAirbase(void);
	void           DropProgramed (void);
	int            IsF16 (void) {return (acFlags & isF16 ? TRUE : FALSE);}
	int            IsComplex (void) {return ((acFlags & isComplex) ? TRUE : FALSE);}
	// 2000-11-17 ADDED BY S.G. SO AIRCRAFT CAN HAVE A ENGINE TEMPERATURE AS WELL AS 'POWER' (RPM) OUTPUT   
	void SetPowerOutput (float powerOutput);//me123 changed back
	// END OF ADDED SECTION	
	// sfr: commented out
	//virtual void	SetVt(float newvt);
	//virtual void	SetKias(float newkias);
	void			ResetFuel (void);
	virtual long	GetTotalFuel (void);
	virtual void	GetTransform(TransformMatrix vmat);
	virtual void	ApplyDamage (FalconDamageMessage *damageMessage);
	virtual void	SetLead (int flag);
	virtual void	ReceiveOrders (FalconEvent* newOrder);
	virtual float	GetP (void);
	virtual float	GetQ (void);
	virtual float	GetR (void);
	virtual float	GetAlpha (void);
	virtual float	GetBeta (void);
	virtual float	GetNx(void);
	virtual float	GetNy(void);
	virtual float	GetNz(void);
	virtual float	GetGamma(void);
	virtual float	GetSigma(void);
	virtual float	GetMu(void);
	virtual void	MakePlayerVehicle(void);
	virtual void	MakeNonPlayerVehicle(void);
	virtual void	MakeLocal (void);
	virtual void	MakeRemote (void);
	virtual void	ConfigurePlayerAvionics(void);
	virtual void	SetVuPosition (void);
	virtual void	Regenerate (float x, float y, float z, float yaw);
	virtual			FireControlComputer* GetFCC(void) { return FCC; };
	virtual SMSBaseClass* GetSMS(void) { return (SMSBaseClass*)Sms; };
	virtual int HasSPJamming (void);
	virtual int HasAreaJamming (void);

#ifdef _DEBUG
	virtual void	SetDead (int);
#endif // _DEBUG

private:
	// sfr: I wonder who is the genius who made these var public
	// ideally all should be private
	// im setting isDigital for now
	int					isDigital;

public:
	// sfr: adding public functions
	/** returns if aircraft is controlled by AI */
	int IsDigital() { return isDigital; }
	/** sets if a/c is controlled by AI. If so, turn AP on */
	void SetIsDigital(int isDigital) {
		this->isDigital = isDigital; 
		this->autopilotType = isDigital ? CombatAP : APOff; 
	}
	long				mCautionCheckTime;
	BOOL				MPOCmd;
	char				dropChaffCmd;
	char				dropFlareCmd;
	int            acFlags;
	AutoPilotType		autopilotType;
	AutoPilotType		lastapType;

	VU_TIME        dropProgrammedTimer;
	unsigned short		dropProgrammedStep;

	float				bingoFuel;
	//MI taking these functions for the ICP stuff, made some changes
	float GetBingoFuel (void){return bingoFuel;};//me123
	void SetBingoFuel (float newbingo){bingoFuel = newbingo;};//me123
	void DamageSounds(void);
	unsigned int SpeedSoundsWFuel;
	unsigned int SpeedSoundsNFuel;
	unsigned int GSoundsWFuel;
	unsigned int GSoundsNFuel;
	void WrongCAT(void);
	void CorrectCAT(void);
	//MI for RALT stuff
	enum RaltStatus { ROFF, RSTANDBY, RON } RALTStatus;
	float RALTCoolTime;	//Cooling is in progress
	int RaltReady() { return (RALTCoolTime < 0.0F && RALTStatus == RON) ? 1 : 0; };
	void RaltOn() {RALTStatus = RON;};
	void RaltStdby () {RALTStatus = RSTANDBY;};
	void RaltOff () { RALTStatus = ROFF; };
	//MI for EWS stuff
	void DropEWS(void);
	void EWSChaffBurst(void);
	void EWSFlareBurst(void);
	void ReleaseManualProgram(void);
	bool ManualECM;
	int FlareCount, ChaffCount, ChaffSalvoCount, FlareSalvoCount;
	VU_TIME ChaffBurstInterval, FlareBurstInterval, ChaffSalvoInterval, FlareSalvoInterval;
	//MI Autopilot
	enum APFlags	{AltHold = 0x1, //Right switch up
		AttHold = 0x2, //Right Switch down
		StrgSel = 0x4,	//Left switch down
		RollHold = 0x8, //Left switch middle
		HDGSel = 0x10,	//Left switch up
		Override = 0x20,	//Paddle switch
		StickStrng = 0x40
	};  // MD -- 20031115: AP fixes; this one is for when stick steering temporarily overrides AP pitch hold
	unsigned int APFlag;
	int IsOn (APFlags flag) {return APFlag & flag ? 1 : 0;};
	void SetAPFlag (APFlags flag) {APFlag |= flag;};
	void ClearAPFlag (APFlags flag) {APFlag &= ~flag;};
	void SetAPParameters(void);
	void SetNewRoll(void);
	void SetNewPitch(void);
	void SetNewAlt(void);
	//MI seatArm
	bool SeatArmed;
	void StepSeatArm(void);

	TransformMatrix		vmat;
	float				gLoadSeconds;
	long				lastStatus;
	BasicWeaponStation	counterMeasureStation[3];
	enum { // what trail is used for what
		TRAIL_DAMAGE = 0, // we've been hit
		TRAIL_ENGINE1,
		TRAIL_ENGINE2,
		TRAIL_MAX,
		MAXENGINES = 8,
	};
	//DrawableTrail*		smokeTrail[TRAIL_MAX];
	//DrawableTrail	    *conTrails[MAXENGINES];
	//DrawableTrail	    *engineTrails[MAXENGINES];
	//DrawableTrail	*rwingvortex, *lwingvortex;
	//DrawableTrail   *wingvapor;
	//DrawableTrail   *dustTrail; // MLR 1/3/2004 - for the dumbass dust/mist trail effect!
	// ********** NEW TRAIL STUFF *************
	DWORD       smokeTrail[TRAIL_MAX];
	DWORD       smokeTrail_trail[TRAIL_MAX];
	DWORD       conTrails[MAXENGINES];
	DWORD       conTrails_trail[MAXENGINES];
	DWORD       colorConTrails[MAXENGINES];
	DWORD       colorConTrails_trail[MAXENGINES];
	DWORD       engineTrails[MAXENGINES];
	DWORD       engineTrails_trail[MAXENGINES];
    DWORD		lwingvortex;
	DWORD		lwingvortex_trail;
	DWORD		rwingvortex;
	DWORD		rwingvortex_trail;
	DWORD       lvortex1;
	DWORD       lvortex1_trail;
	DWORD       rvortex1;
	DWORD       rvortex1_trail;
	DWORD       lvortex2;
	DWORD       lvortex2_trail;
	DWORD       rvortex2;
	DWORD       rvortex2_trail;
	DWORD       dustTrail;
	DWORD       dustTrail_trail;
    
	Tpoint damageTrailLocation0; 
    Tpoint damageTrailLocation1;
	bool   damageTrailLocationSet; //RV I-Hawk - added as flag for if damage trail locations are set
	bool burnEffectPosition;

	//RV I-Hawk - This variable decides the CTRL-S trail color, there are 5 different colors
	int colorContrail;
	void SetColorContrail (int color);
    // ****************************************
    BOOL             dustConnect;  // MLR 1/4/2004 - 
	BOOL				playerSmokeOn;
	DrawableGroundVehicle* pLandLitePool;
	BOOL				mInhibitLitePool;
	void				CleanupLitePool(void);
	void	AddEngineTrails(int ttype, DWORD *tlist, DWORD *tlist_trail);
	void	CancelEngineTrails(DWORD *tlist, DWORD *tlist_trail);

	// JPO Avionics power settings;
	unsigned int powerFlags;
	void PowerOn (AvionicsPowerFlags fl) { powerFlags |= fl; };
	int HasPower(AvionicsPowerFlags fl);
	void PowerOff (AvionicsPowerFlags fl) { powerFlags &= ~fl; };
	void PowerToggle (AvionicsPowerFlags fl) { powerFlags ^= fl; };
	int PowerSwitchOn(AvionicsPowerFlags fl) { return (powerFlags & fl) ? TRUE : FALSE; };

	void PreFlight (); // JPO - do preflight checks.

	// JPPO Main Power
	MainPowerType mainPower;
	MainPowerType MainPower() { return mainPower; };
	BOOL MainPowerOn() { return mainPower == MainPowerMain; };
	void SetMainPower (MainPowerType t) { mainPower = t; };
	void IncMainPower ();
	void DecMainPower ();
	PowerStates currentPower;
	enum ElectricLights {
	ElecNone = 0x0,
	ElecFlcsPmg = 0x1,
	ElecEpuGen = 0x2,
	ElecEpuPmg = 0x4,
	ElecToFlcs = 0x8,
	ElecFlcsRly = 0x10,
	ElecBatteryFail = 0x20,
	};
	unsigned int elecLights;
	bool ElecIsSet(ElectricLights lt) { return (elecLights & lt) ? true : false; };
	void ElecSet(ElectricLights lt) { elecLights |= lt; };
	void ElecClear(ElectricLights lt) { elecLights &= ~lt; };
	void DoElectrics();
	static const unsigned long systemStates[PowerMaxState];

	//MI EWS PGM Switch
	EWSPGMSwitch EWSPgm;
	EWSPGMSwitch EWSPGM() { return EWSPgm; };
	void SetPGM (EWSPGMSwitch t) { EWSPgm = t; };
	void IncEWSPGM();
	void DecEWSPGM();
	void DecEWSProg();
	void IncEWSProg();
	void SetEWSProg(int num) { if ((num >= 0) && (num <= 3)) EWSProgNum = num; }  // MD: 4 position knob
	//Prog select switch
	unsigned int EWSProgNum;
	//MI caution stuff
	bool NeedsToPlayCaution;
	bool NeedsToPlayWarning;
	VU_TIME WhenToPlayCaution;
	VU_TIME WhenToPlayWarning;
	void SetExternalData(void);
	//MI Inhibit VMS Switch
	bool playBetty;
	void ToggleBetty(void);
	//MI RF switch
	int RFState;	//0 = NORM; 1 = QUIET; 2 = SILENT
	void GSounds(void);
	void SSounds(void);
	int SpeedToleranceTanks;
	int SpeedToleranceBombs;
	float GToleranceTanks;
	float GToleranceBombs;
	int OverSpeedToleranceTanks[3];	//3 levels of OverSpeed for tanks
	int OverSpeedToleranceBombs[3]; //3 levels of OverSpeed for bombs
	int OverGToleranceTanks[3];	//3 levels of OverG for tanks
	int OverGToleranceBombs[3]; //3 levels of OverG for bombs
	void AdjustTankSpeed(int level);
	void AdjustBombSpeed(int level);
	void AdjustTankG(int level);
	void AdjustBombG(int level);

	void	DoWeapons (void);
	float	GlocPrediction (void);
	void	InitCountermeasures(void);
	void	CleanupCountermeasures(void);
	void	InitDamageStation(void);
	void	CleanupDamageStation (void);
	void    CleanupVortex (void); //RV - I-Hawk - new function to clean up vortex and dust trails
	void	DoCountermeasures(void);
	void	DropChaff(void);
	void	DropFlare(void);
	void	GatherInputs(void);
	void	RunSensors(void);
	BOOL	LandingCheck( float noseAngle, float impactAngle, int groundType );
	void	GroundFeatureCheck( float groundZ );
	void	RunExplosion(void);
	void	ShowDamage (void);
	void	CleanupDamage(void);
	void	MoveSurfaces(void);
	// sfr: control light dofs
	void	RunLightSurfaces(void); 
	// control gear dofs
	void	RunGearSurfaces(void);

	void	ToggleAutopilot (void);
	void	OnGroundInit (SimInitDataClass* initData);
	void	CheckObjectCollision( void );
	void	CheckPersistantCollision( void );
	void	CautionCheck (void);
	void	SetCursorCmdsByAnalog(void);   // MD -- 20040110: analog cursor control support
	void	SetSpeedBrake(void); //TJL 02/28/04
	int brakePos; //TJL 02/28/04
	float speedBrakeState; //TJL 02/28/04
	CampBaseClass* JDAMtarget;//Cobra
	SimBaseClass* JDAMsbc;//Cobra
	char JDAMtargetName[80];//Cobra
	char JDAMtargetName1[80];//Cobra
	int  JDAMStep;//Cobra
	int  JDAMtgtnum;//Cobra
	float JDAMtargetRange; //Cobra
	Tpoint JDAMtgtPos; // Cobra
	int GetJDAMPBTarget(AircraftClass* aircraft); // Cobra
	bool JDAMAllowAutoStep; // RV - I-Hawk

	int spawnpoint; //RAS-11Nov04-hold initial spawnpoint


	DigitalBrain *DBrain(void)			{return (DigitalBrain *)theBrain;}
	TankerBrain *TBrain(void)			{return (TankerBrain *)theBrain;}
	// so we can discover we have an aircraft at the falcentity level
	virtual int IsAirplane(void) {return TRUE;}
	virtual float	Mass(void);

	// Has the player triggered the ejection sequence?
	BOOL	ejectTriggered;
	float	ejectCountdown;
	BOOL	doEjectCountdown;

	//MI Emergency jettison
	bool EmerJettTriggered;
	float JettCountown;
	bool doJettCountdown; 

	//MI Cockpit nightlighting
	bool NightLight, WideView;


	void RemovePilot(void);
	void RunCautionChecks(void);

	// Run the ejection sequence
	void Eject(void);
	virtual int HasPilot (void) {return (IsAcStatusBitsSet(ACSTATUS_PILOT_EJECTED) ? FALSE : TRUE);};

	//virtual float GetKias();

	// Public for debug
	void AddFault(int failures, unsigned int failuresPossible, int numToBreak, int sourceOctant);


	float GetA2GJDAMAlt(void);
	float GetA2GJSOWAlt(void);
	float GetA2GHarmAlt(void);
	float GetA2GAGMAlt(void);
	float GetA2GGBUAlt(void);
	float GetA2GDumbHDAlt(void);
	float GetA2GClusterAlt(void);
	float GetA2GDumbLDAlt(void);
	float GetA2GGenericBombAlt(void);
	float GetA2GGunRocketAlt(void);
	float GetA2GCameraAlt(void);
	float GetA2GBombMissileAlt(void);

private:
	//used for safe deletion of sensor array when making a player vehicle
	SensorClass** tempSensorArray;
	int tempNumSensors;

	protected:
	int SetDamageF16PieceType (DamageF16PieceStructure *piece, int type, int flag, int mask, float speed);
	int CreateDamageF16Piece (DamageF16PieceStructure *piece, int *mask);
	int CreateDamageF16Effects ();
	void SetupDamageF16Effects (DamageF16PieceStructure *piece);

public:
	VuEntity *attachedEntity; // JB carrier
	bool AWACSsaidAbort;		// MN when target got occupied, AWACS says something useful
	//TJL 01/04/04 Moved it here from private to access it (per Jam)
	void CalculateSweepAndSpoiler(float &sweep, float &sl1, float &sr1 ,float &sl2, float &sr2);
	//TJL 01/04/04 added this variable to catch the wingsweep.
	float wingSweep;


private:
	void CalculateAileronAndFlap(float qf, float *al, float *ar, float *fl, float *fr);
	//void CalculateSweepAndSpoiler(float &sweep, float &sl1, float &sr1 ,float &sl2, float &sr2);
	void CalculateLef(float qfactor);
	void CalculateStab(float qfactor, float *sl, float *sr);
	float CalculateRudder(float qfactor);
	void MoveDof(int dof, float newvalue, float rate, int ssfx = -1, int lsfx = -1, int esfx = -1);
	void DeployDragChute(int n);
	int FindBestSpawnPoint(ObjectiveClass *obj, SimInitDataClass* initData);

private: // MLR's 2003-10
	VU_TIME animStrobeTimer;  // at this time, the strobe changes on/off
	VU_TIME animWingFlashTimer; // do for the flashing winglight martinv
	VU_TIME MPWingFlashTimer;
	// MLR 2003-11-15 Decoy Dispenser related
	int flareDispenser,chaffDispenser,flareUsed,chaffUsed;

	float acmiTimer;
	float acmiDOFValue[COMP_MAX_DOF];
	int   acmiSwitchValue[COMP_MAX_SWITCH];

	float swingWingAngle; // set in surface.cpp
	Tpoint swingWingTip; // MLR 3/5/2004 - 

	// RV - Biker - Flag for carrier takeoff position taken
	int carrierStartPosEngaged;
	float carrierInitTimer;
	int takeoffSlot;

	AircraftTurbulence	*turbulence, *lVortex, *rVortex;

public:		
	void	SetPulseTurbulence(float RateX, float RateY, float RateZ, float Time);
	void	SetStaticTurbulence(float RateX, float RateY, float RateZ){ 
		StaticTurbulence.x+=RateX; StaticTurbulence.y+=RateY; StaticTurbulence.z+=RateZ; 
	}
	Tpoint GetTurbulence();
	int Sleep(void);

private:
	// COBRA - RED - Adding turbulence to the Pit
	float				PulseTurbulenceTime;
	Tpoint				PulseTurbulence, StaticTurbulence, TotalTurbulence;
};

#endif

