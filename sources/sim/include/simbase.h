#ifndef _SIMBASE_H
#define _SIMBASE_H

#include "FalcLib/include/f4vu.h"
#include "Falclib/include/FalcEnt.h"
#include "geometry.h"
#include "Falclib/include/camp2sim.h"
#include "Falclib/include/fsound.h"
//#include "Simdrive.h"
#include "initdata.h"

// Flags used to convey special data
//NOTE top 16 bits are used for motion type
#define VIS_TYPE_MASK      0x7
#define RADAR_ON           0x1
#define ECM_ON             0x2
#define AIR_BRAKES_OUT     0x4	// Should really be a position value, not a bit, but for now...
#define CANOPY_OPEN        0x8
#define OBJ_EXPLODING      0x10
#define OBJ_DEAD           0x20
#define OBJ_FIRING_GUN     0x40
#define ON_GROUND          0x80
#define SHOW_EXPLOSION     0x100
#define IN_PERSISTANT_LIST 0x200
#define OBJ_DYING          0x400
//#define PILOT_EJECTED      0x800
#define I_AM_A_TANKER      0x1000
#define IS_LASED           0x2000
#define HAS_MISSILES       0x4000
//#define AVAILABLE        0x8000

// Local flags
#define OBJ_AWAKE          0x01
#define REMOVE_NEXT_FRAME  0x02
#define NOT_LABELED        0x04
#define IS_HIDDEN          0x08

class SimBaseSpecialData
{
public:
	SimBaseSpecialData();
	~SimBaseSpecialData();

	float			rdrAz, rdrEl, rdrNominalRng;
	float			rdrAzCenter, rdrElCenter;
	float			rdrCycleTime;
	int				flags;
	int				status;
	int				country;
	unsigned char	afterburner_stage;

	// These should really move into SimMover or SimVeh, since they're only relevant there...
	float			powerOutput;
	unsigned char	powerOutputNet;
	float			powerOutput2; // MLR 3/21/2004 - 
	unsigned char	powerOutputNet2;
	// 2000-11-17 ADDED BY S.G. SO ENGINE HAS HEAT TEMP
	float			engineHeatOutput;
	unsigned char	engineHeatOutputNet;
	// END OF ADDED SECTION
	VU_ID			ChaffID, FlareID;
};

// Forward declarations for class pointers
class CampBaseClass;
class SimInitDataClass;
class DrawableObject;
class FalconDamageMessage;
class FalconDeathMessage;
class DrawableTrail;
class FalconEntity;


// this class is used only for non-local entities
// for the most part, it will handle any special effects needed to
// run locally
class SimBaseNonLocalData {
public:
	int flags;
	#define		NONLOCAL_GUNS_FIRING		0x00000001
	float timer1;				// general purpose timer
	float timer2;				// general purpose timer
	float timer3;				// general purpose timer
	float dx;					// use as a vector
	float dy;
	float dz;
	DrawableTrail *smokeTrail;	// smoke when guns are firing

	SimBaseNonLocalData();
	~SimBaseNonLocalData();
};

/** base of all entities which can interact with 3D world. */
class SimBaseClass : public FalconEntity {
private:
	VuBin<CampBaseClass> campaignObject;

	// Special Transmit Data
	// 2000-11-17 MADE PUBLIC BY S.G. SO specialData.powerOutput IS VISIBLE OUTSIDE SimBaseClass.
	// sfr: @todo protect this special data
public:
	SimBaseSpecialData specialData;
private:
	int dirty_simbase;

protected:
	int callsignIdx;
	int slotNumber;
	// for damage, death and destruction....
	float strength;
	float maxStrength;
	float dyingTimer;
	float sfxTimer;
	long lastDamageTime;
	long explosionTimer;
	VU_ID lastShooter;				// KCK: replaces vu's lastShooter - Last person to hit this entity
	VU_TIME lastChaff, lastFlare;		// When will the most recently dropped counter-measures expire?
	long campaignFlags;
	uchar localFlags;					// Don't transmit these, or else..

public:
	long timeOfDeath;
	float pctStrength;
	TransformMatrix dmx;
	DrawableObject* drawPointer;
	char displayPriority;
	ObjectGeometry platformAngles;
	SimBaseNonLocalData *nonLocalData;

	// For regenerating entities, we keep a pointer to our init data
	SimInitDataClass *reinitData;

	// this function can be called for entities which aren't necessarily
	// exec'd in a frame (ie ground units), but need to have their
	// gun tracers and (possibly other) weapons serviced
	virtual void WeaponKeepAlive( void ) { return; };

	// Object grouping
	virtual void JoinFlight (void) {};

	//Functions
	SimBaseClass(ushort type);
	SimBaseClass(VU_BYTE** stream,  long *rem);
	SimBaseClass(FILE* filePtr);
	virtual ~SimBaseClass();
	virtual void InitData();
	virtual void CleanupData();
private:
	void InitLocalData();
	void CleanupLocalData();
public:
	virtual bool IsSimBase() { return true; }

	int GetCallsignIdx () { return callsignIdx; };
	int GetSlot () { return slotNumber;};
	virtual uchar GetTeam ();
	virtual uchar GetCountry() { return (uchar)specialData.country; };
	virtual short GetCampID();
	uchar GetDomain() const					{	return (EntityType())->classInfo_[VU_DOMAIN]; }
	uchar GetClass() const					{	return (EntityType())->classInfo_[VU_CLASS]; }
	uchar GetType() const					{	return (EntityType())->classInfo_[VU_TYPE]; }
	uchar GetSType() const					{	return (EntityType())->classInfo_[VU_STYPE]; }
	uchar GetSPType() const					{	return (EntityType())->classInfo_[VU_SPTYPE]; }

	// sfr: making virtual
	virtual void ChangeOwner (VU_ID new_owner);
	virtual int Wake ();
	virtual int Sleep ();
	virtual MoveType GetMovementType ();
	virtual void MakeLocal ();
	virtual void MakeRemote ();
	virtual int OnGround () { return (specialData.flags & ON_GROUND ? TRUE : FALSE); }
	virtual int IsExploding () { return (specialData.flags & OBJ_EXPLODING ? TRUE : FALSE); }
	virtual int IsDead() const { return (specialData.flags & OBJ_DEAD ? true : false); }
	int IsDying() const { return (specialData.flags & OBJ_DYING ? TRUE : FALSE); }
	int IsFiring() const { return (specialData.flags & OBJ_FIRING_GUN ? TRUE : FALSE); }
	int IsAwake() const { return localFlags & OBJ_AWAKE; }
	int  IsSetFlag(int flag) const { return ((specialData.flags & flag) ? TRUE : FALSE); }
	int  IsSetLocalFlag(int flag) const { return ((localFlags & flag) ? TRUE : FALSE); }
	void SetLocalFlag(int flag){ localFlags |= flag; }
	void UnSetLocalFlag(int flag){ localFlags &= ~(flag); }
	int  IsSetCampaignFlag(int flag) const { return ((campaignFlags & flag) ? TRUE : FALSE); }
	void SetCampaignFlag(int flag){ campaignFlags |= flag; }
	void UnSetCampaignFlag(int flag){campaignFlags &= ~(flag); }
	int IsSetRemoveFlag() const { return localFlags & REMOVE_NEXT_FRAME; }
	void SetRemoveFlag();
	void SetRemoveSilentFlag();
	void SetExploding(int);
	void SetFiring(int);
	void SetCallsign(int newCallsign) { callsignIdx = newCallsign; }
	void SetSlot(int newSlot) { slotNumber = newSlot; }
	int Status() const { return specialData.status; }

	// Seems like this stuff (and its support "sepecial data") could move into vehicle or aircraft???
	// LOCAL ONLY!
	void SetChaffExpireTime (VU_TIME t)	{ShiAssert(IsLocal()); lastChaff = t;};
	void SetFlareExpireTime (VU_TIME t)	{ShiAssert(IsLocal()); lastFlare = t;};
	VU_TIME ChaffExpireTime (void)			{ShiAssert(IsLocal()); return lastChaff;};
	VU_TIME FlareExpireTime (void)			{ShiAssert(IsLocal()); return lastFlare;};
	// REMOTE OR LOCAL
	void SetNewestChaffID(VU_ID id);
	void SetNewestFlareID(VU_ID id);

	VU_ID	NewestChaffID(void)			{return specialData.ChaffID;};
	VU_ID	NewestFlareID(void)			{return specialData.FlareID;};

	virtual int IsSPJamming (void);
	virtual int IsAreaJamming (void);
	virtual int HasSPJamming (void);
	virtual int HasAreaJamming (void);

	CampBaseClass *GetCampaignObject (void) { return campaignObject.get(); }
	void SetCampaignObject (CampBaseClass *ent);

	float PowerOutput (void) {return specialData.powerOutput;};
	// 2000-11-17 ADDED BY S.G. IT RETURNS THE ENGINE TEMP INSTEAD. 
	// FOR NON AIRPLANE VEHICLE, RPM IS COPIED INTO ENGINE TEMP TO BE CONSISTENT
	//	float EngineTempOutput (void) {return specialData.engineHeatOutput;};
	// END OF ADDED SECTION
	float RdrAz (void) {return specialData.rdrAz;};
	float RdrEl (void) {return specialData.rdrEl;};
	float RdrAzCenter (void) {return specialData.rdrAzCenter;};
	float RdrElCenter (void) {return specialData.rdrElCenter;};
	float RdrCycleTime (void) {return specialData.rdrCycleTime;};
	float RdrRng (void) {return specialData.rdrNominalRng;};
	float Strength (void) {return strength;};
	float MaxStrength (void) {return maxStrength;};

	int GetAfterburnerStage (void);
	void SetFlag (int flag);
	void UnSetFlag (int flag);
	void SetFlagSilent (int flag);
	void UnSetFlagSilent (int flag);
	void SetCountry(int newSide);
	void SetLastChaff (long a);
	void SetLastFlare (long a);
	void SetStatus (int status);
	void SetStatusBit (int status);
	void ClearStatusBit (int status);
	// sfr: removed these setters
	/*virtual void SetVt (float vt);
	virtual void SetKias (float GetKias);*/
	void SetPowerOutput (float powerOutput);
	void SetRdrAz (float az);
	void SetRdrEl (float el);
	void SetRdrAzCenter (float az);
	void SetRdrElCenter (float el);
	void SetRdrCycleTime (float cycle);
	void SetRdrRng (float rng);
	void SetAfterburnerStage (int s);
	virtual void Init(SimInitDataClass* initData);
	virtual int Exec(void) { return TRUE; };
	virtual void GetTransform(TransformMatrix) {};
	virtual void ApplyDamage(FalconDamageMessage *damageMessage);
	virtual void ApplyDeathMessage (FalconDeathMessage *deathMessage);
	virtual void SetDead(int);
	virtual void MakePlayerVehicle() {}
	virtual void MakeNonPlayerVehicle() {}
	virtual void ConfigurePlayerAvionics() {}
	virtual void SetVuPosition() {}
	virtual void Regenerate(float, float, float, float) {}
	VU_ID LastShooter (void) { return lastShooter; };
	void SetDying (int flag);// { if (flag) specialData.flags |= OBJ_DYING; else specialData.flags &= ~OBJ_DYING;};
	SimBaseSpecialData* SpecialData(void) {return &specialData;};

	//a nice big number for ground vehicles and features
	virtual float Mass(void)		{return 2500.0F;}
	// incoming missile notification
	// 2000-11-17 INSTEAD OF USING PREVIOUSLY UNUSED VARS IN AIRFRAME, I'LL CREATE MY OWN INSTEAD
	//	SimBaseClass *incomingMissile;
	//  PLUS SetIncomingMissile CAN CLEAR UP THE ARRAY IF TRUE IS PASSED AS A SECOND PARAMETER (DEFAULTS TO FALSE)
	//	void SetIncomingMissile ( SimBaseClass *missile );
	SimBaseClass *incomingMissile[2];
	void SetIncomingMissile ( SimBaseClass *missile, BOOL clearAll = FALSE);
	// 2000-11-17 ADDED BY S.G. INSTEAD OF USING oldrpm[5],
	// I'LL CREATE MY OWN VARIABLE TO KEEP THE MONITORED INCOMING MISSLE RANGE AND THE KEEP EVADING TIMER
	float		incomingMissileRange;
	VU_TIME	incomingMissileEvadeTimer;
	// END OF ADDED SECTION

	// threat
	VuBin<FalconEntity> threatObj;
	int threatType;
	#define	THREAT_NONE		0
	#define	THREAT_SPIKE	1
	#define	THREAT_MISSILE	2
	#define	THREAT_GUN		3
	void SetThreat ( FalconEntity *threat, int type );


	// virtual function interface
	// serialization functions
	virtual int SaveSize();
	virtual int Save(VU_BYTE **stream);	// returns bytes written
	virtual int Save(FILE *file);		// returns bytes written

	// event handlers
	virtual int Handle(VuFullUpdateEvent *event);
	virtual int Handle(VuPositionUpdateEvent *event);
	virtual int Handle(VuTransferEvent *event);
	virtual VU_ERRCODE InsertionCallback(void);
	virtual VU_ERRCODE RemovalCallback(void);

	// callsign and radio functions
	int GetPilotVoiceId(void);
	int GetFlightCallsign(void);

	/** where the camera should look at if its interested in this entity. */
	virtual void GetFocusPoint(BIG_SCALAR &x, BIG_SCALAR &y, BIG_SCALAR &z);  

	// Dirty Functions
	void MakeSimBaseDirty (Dirty_Sim_Base bits, Dirtyness score);
	void WriteDirty(unsigned char **stream);
	void ReadDirty(unsigned char **stream, long *rem);

	F4SoundPos SoundPos; // MLR 12/25/2003 - Needed in this class
	// this object needs to be Update()'ed in Exec() - HOWEVER, 
	// there's no  point in doing so for every object - instead, 
	// classes that really need it, will have to call Update() 
	// themselvse.
};

#include "simbsinl.cpp"

#endif
