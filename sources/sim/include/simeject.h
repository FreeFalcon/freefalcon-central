#ifndef _SIM_EJECT_H
#define _SIM_EJECT_H

#include "simmover.h"
#include "vector.h"

// Forward references.
class AircraftClass;
class FalconDeathMessage;
//	3d vector for motion stuff.
#define EP_VECTOR		Vector_3D<SIM_FLOAT>
// indices for 3d vector.
#define I_X				0
#define I_ROLL			0
#define I_Y				1
#define I_PITCH		1
#define I_Z				2
#define I_YAW			2

//	Ejection sequence physical data.
// Time from start of ejection sequence to switch to next stage of data.
#define SDPD_END_TIME_INDEX			0
// This is the combined constants that contribute to drag force:
// atmospheric density * cross-sectional area * drag coefficient.
#define SDPD_DRAG_FACTOR_INDEX		1
// This is the combined mass of the seat and the pilot, or just the seat.
// after the seat falls (in slugs).
#define SDPD_MASS_INDEX					2
// Number of stage physical data indices.
#define SDPD_NUM_INDICES				3

// Ejected pilot data that is dependent on stage.
typedef SIM_FLOAT EP_STAGE_PHYS_DATA[SDPD_NUM_INDICES];

// Stages of the ejection sequence.
#define PD_START									-1
#define PD_JETTISON_CANOPY						0
#define PD_EJECT_SEAT							1
#define PD_FREE_FALL_WITH_SEAT				2
#define PD_CHUTE_OPENING						3
#define PD_FREE_FALL_WITH_OPEN_CHUTE		4
#define PD_FREE_FALL_WITH_COLLAPSED_CHUTE	5
#define PD_SAFE_LANDING						6
#define PD_CRASH_LANDING					7
#define PD_NUM_STAGES							8

// Ejected pilot non stage-dependent physical data.
struct EP_PHYS_DATA
{
	EP_STAGE_PHYS_DATA	stageData[PD_NUM_STAGES];

	// Speed of pilot & seat at ejection time.
	SIM_FLOAT						ejectSpeed;

	// Thrust of seat in G's
	SIM_FLOAT						seatThrust;

	// Angle of ejection.  This defines the ejection unit vector .
	// This is using the airplane as a point of reference.
	// The angle is rotation about the axis that the plane's
	// wings lie along, with ejection in the direction that the 
	// plane is facing being 0 radians, and ejection straight up
	// out of the cockpit being pi/2 radians.
	SIM_FLOAT						ejectAngle;

	// Pitch of pilot in free fall with chute.
	// This value must be negative.
	SIM_FLOAT						startPitch;

	// Pitch decay of pilot in free fall with chute (radians/sec)
	// This value must be positive.
	SIM_FLOAT						pitchDecay;

	// Delta yaw of pilot in free fall with chute (radians/sec)
	SIM_FLOAT						yawSpeed;

	// Offset of seat from center of plane
	SIM_FLOAT						seatXOffset;
	SIM_FLOAT						seatYOffset;
	SIM_FLOAT						seatZOffset;

	// Player pilot end stage time adjustment.
	// This is added to the end stage time for the player pilot.
	SIM_FLOAT						humanPilotEndStageTimeAdjust;
};

// Model data for a stage or sequence of stages.
struct EP_MODEL_DATA
{
	int							bsp;
	int							creationStage;
	int							chaseMode;
	EP_VECTOR					focusOffset;
	
	EP_MODEL_DATA
	(
		int bsp_,
		int creationStage_,
		int chaseMode_,
		const EP_VECTOR &focusOffset_
	);
	~EP_MODEL_DATA();
};

// Model data stages.
#define MD_START									-1
#define MD_PILOT_AND_SEAT							0
#define MD_PILOT_AND_OPEN_CHUTE						1
#define MD_PILOT_AND_COLLAPSED_CHUTE				2
#define MD_SAFE_LANDING								3
#define MD_CRASH_LANDING							4
#define MD_NUM_MODELS								5

// Ejection modes
#define EM_F16_MODE1				0
#define EM_F16_MODE2				1
#define EM_NUM_MODES				2

// The ejected pilot is a sim object so he can be killed.
class EjectedPilotClass : public SimMoverClass {
public:
	// Constructors.
	EjectedPilotClass(AircraftClass *ac, int mode, int no);
	EjectedPilotClass (VU_BYTE** stream, long *rem);
	virtual ~EjectedPilotClass();
	virtual void InitData();
	virtual void CleanupData();
private:
	// Right now, EM_F16_MODE1 is the only valid mode.
	void InitLocalData(AircraftClass *ac, int mode, int no);
	void CleanupLocalData();
public:


	// Simulation methods.
	virtual void Init(SimInitDataClass *initData);
	virtual int Exec();
	//	void ExecFromAircraft();
	virtual void ApplyDamage(FalconDamageMessage *damageMsg);
	virtual void SetDead(int flag);
	virtual int Wake (void);
	virtual int Sleep (void);
	virtual int IsEject (void) {return TRUE;};
	virtual float Mass(void)		{return 20.0F;}

	// virtual function interface
	// serialization functions
	virtual int SaveSize();
	virtual int Save(VU_BYTE **stream);	// returns bytes written

	// Access methods.
	virtual void GetTransform(TransformMatrix vmat);

	// Where should the camera look if it is interested in me?
	virtual void GetFocusPoint
	(
		BIG_SCALAR &x,
		BIG_SCALAR &y,
		BIG_SCALAR &z
	);

	// Which aircraft did I eject from?
	AircraftClass *MyAircraft();

	// Get the class type.
	static int ClassType();

	// Get the parent aircraft
	AircraftClass* GetParentAircraft (void);
	//	// Tell me when my aircraft has died.
	//	void NotifyOfAircraftsDeath();

	// Am I the player pilot?
	BOOL IsPlayerPilot() const;
	// Am I a digi pilot?
	BOOL IsDigiPilot() const;

	//	// Can I collide with my own aircraft yet?
	//	BOOL CanCollideWithOwnAircraft() const;

private:
	// Stage time access functions
	SIM_FLOAT StageEndTime(int stage) const;
	SIM_FLOAT StageEndTime() const;

	// Physical data access functions.
	SIM_FLOAT DragFactor() const;
	SIM_FLOAT Mass() const;
	SIM_FLOAT EjectSpeed() const;
	SIM_FLOAT SeatThrust() const;
	SIM_FLOAT EjectAngle() const;
	SIM_FLOAT StartPitch() const;
	SIM_FLOAT PitchDecay() const;
	SIM_FLOAT YawSpeed() const;
	EP_VECTOR SeatOffset() const;

	// When was the current model created?
	SIM_FLOAT ModelCreateTime(int model) const;

	// Set the ejection sequence mode.
	// This will point us at the correct physical data.
	void SetMode(int mode);

	// Advance time.  This returns delta time.
	void AdvanceTime();

	// Set stage.  This returns the new stage.
	void SetStage(int stage);
	int AdvanceStage();

	// Set the current model to draw.
	void SetModel(int model);

	// Initialize each stage of the ejection sequence
	void InitJettisonCanopy();
	void InitEjectSeat();
	void InitFreeFallWithSeat();
	void InitChuteOpening();
	void InitFreeFallWithOpenChute();
	void InitFreeFallWithCollapsedChute();
	void InitSafeLanding();
	void InitCrashLanding();

	// Run each stage of the ejection sequence.
	void RunJettisonCanopy();
	void RunEjectSeat();
	void RunFreeFallWithOpenChute();
	void RunFreeFall();
	void RunCrashLanding();
	void RunSafeLanding();

	// Reference the aircraft I ejected from until I am
	// done with the aircraft's data, then dereference it.
	//	void ReferenceAircraft(AircraftClass *ac);
	//	void DereferenceAircraft();

	// Calculations used in the motion model.
	void CalculateAndSetPositionAndOrientationInCockpit();
	void CalculateThrustVector(EP_VECTOR &result) const;
	void CalculateDragVector(EP_VECTOR &result) const;
	void CalculateGravityVector(EP_VECTOR &result) const;
	void CalculateEjectionVector(EP_VECTOR &result) const;
	void FixOrientationRange();
	void ZeroPitchAndRoll();

	// Have I hit the ground?
	BOOL HasHitGround() const;

	// Handle ground collision.
	void HitGround();

	// Spew some debug info to the mono monitor.
	void SpewDebugData();

	// Aircraft I ejected from.
	//	AircraftClass				*_aircraft;
	VU_ID						_aircraftId;
	VU_ID						_flightId;

	// Physical data.
	EP_PHYS_DATA				*_pd;
	int							_stage;

	// Model data.
	EP_MODEL_DATA				*_md;
	int							_model;

	// Total time the ejected pilot simulation has been running. 
	SIM_FLOAT					_runTime;
	SIM_FLOAT					_deltaTime;
	SIM_FLOAT					_endStageTimeAdjust;
	SIM_FLOAT					_stageTimer;
	VU_TIME						_delayTime;

	// My position and rotation.
	EP_VECTOR					_pos;
	EP_VECTOR					_rot;

	// My velocity and angular velocity.
	EP_VECTOR					_vel;
	EP_VECTOR					_aVel;

	// Am I a digital pilot, or a human pilot?
	BOOL							_isDigital;
	// Am I the player or just something else
	BOOL							_isPlayer;

	// Have I hit the ground?
	BOOL							_hitGround;
	SIM_FLOAT						_hitGroundTime;

	// Has something caused my chute to collapse?
	BOOL							_collapseChute;
	SIM_FLOAT					_chuteCollapsedTime;

	// Current model space focus point.
	EP_VECTOR					_focusPoint;

	// These are here so that the class type only has to
	// be looked up once (apparently it's expensive).
	static int					_classType;
	static BOOL					_classTypeFound;

	// Hold on to the death message, I don't die when
	// I'm gunned down, I die when I hit the ground.
	FalconDeathMessage		*_deathMsg;

	// Hold on to the label, since there are multiple bsp's that
	// are used in the ejection sequence.
	char							_label[32];
	DWORD							_labelColor;

	// Do we want the exec function to be called from my aircraft?
	//	BOOL							_execFromAircraft;

	// Was exec called from my aircraft?
	BOOL							_execCalledFromAircraft;

	// This is used to sync the transfer of execution control
	SIM_UINT						_lastFrameCount;
	int							_execCount;
};

#include "simejinl.cpp"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#endif // _SIM_EJECT_H
