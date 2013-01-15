#ifndef FALCENT_H
#define FALCENT_H

//#include "shi/shierror.h"
#include "dirtybits.h"
#include "../../mathlib/math.h"


// ================================
// Data types and defines
// ================================

typedef enum {	NoMove = 0,
				Foot,
				Wheeled,
				Tracked,
				LowAir,
				Air,
				Naval,
				Rail,
				MOVEMENT_TYPES } MoveType;

// Radar Modes
#define FEC_RADAR_OFF			0x00	   	// Radar always off
#define FEC_RADAR_SEARCH_100	0x01	   	// Search Radar - 100 % of the time (always on)
#define FEC_RADAR_SEARCH_1		0x02	   	// Search Sequence #1
#define FEC_RADAR_SEARCH_2		0x03	   	// Search Sequence #2
#define FEC_RADAR_SEARCH_3		0x04	   	// Search Sequence #3
#define FEC_RADAR_AQUIRE		0x05	   	// Aquire Mode (looking for a target)
#define FEC_RADAR_GUIDE			0x06	   	// Missile in flight. Death is imminent
#define FEC_RADAR_CHANGEMODE	0x07	   	// Missile in flight. Death is imminent
// Flags
#define FEC_HOLDSHORT			0x01		// Don't takeoff until a player attaches
#define FEC_PLAYERONLY			0x02		// This entity is only valid if under player control
#define FEC_HASPLAYERS			0x04		// One or more player is attached to this entity
#define FEC_REGENERATING		0x08		// This entity is undead.
#define FEC_PLAYER_ENTERING		0x10		// A player is soon to attach to this aircraft/flight
#define FEC_INVULNERABLE		0x20		// This thing can't be destroyed

// localFlags
#define FELF_ON_PLAYERS_GM_CONTACT_LIST      0x1 // This object is listed in the players Ground Radar target list // MLR 4/19/2004 - new!
#define FELF_ON_PLAYERS_GMT_CONTACT_LIST     0x2 // This object is listed in the players Ground Radar target list // MLR 4/19/2004 - new!
#define FELF_UPDATE_TARGET_LIST_TEMP         0x4 // temporary marker for UpdateTargetList()      
#define FELF_ADDED_DURING_SIMDRIVER_CYCLE	 0x8 // Object was created during SimDriver::Cycle() and did missed having EntityDriver() called (due to hash table)

// ================================
// FalcEntity class
// ================================

class FalconSessionEntity;

class FalconEntity : public VuEntity
{
private:
	int			dirty_falcent;

	int			dirty_classes;
	/** flags indicating unit dirtyness. Can be Ored. See enum Dirtyness for valid scores. */
	int			dirty_score;

	/** returns the dirty bucket where unit should be inserted based on its dirty_score.
	* If unit is not dirty, returns -1
	*/
	static int calc_dirty_bucket(int dirty_score);
	uchar		falconFlags;

protected:
	char		falconType;

public:

	enum {FalconCampaignEntity = 0x1, FalconSimEntity = 0x2, 
		FalconPersistantEntity = 0x8, FalconSimObjective = 0x20};
	FalconEntity(ushort type, VU_ID_NUMBER eid);
	FalconEntity(VU_BYTE** stream, long *rem);
	FalconEntity(FILE* filePtr);
	int Save(VU_BYTE** stream);
	int Save(FILE* filePtr);
	int SaveSize(void);
	virtual ~FalconEntity(void);
	/** inits entity data, calling base class initialization in sequence. */
	virtual void InitData();
	/** cleans up the whole hierarchy data, calling base class in sequence. */
	virtual void CleanupData();
	// sfr: fixes mem leak
#define NEW_REMOVAL_CALLBACK 1
#if NEW_REMOVAL_CALLBACK
	/** calls cleanup data. Called after entity is removed from database in thread safe spot. */
	virtual VU_ERRCODE RemovalCallback();
#endif
private:
	/** initializes only local data (not base class), so not virtual. */
	void InitLocalData();
	/** cleans up only the local data */
	void CleanupLocalData();
public:

	virtual bool IsSimBase() { return false; }
	virtual bool IsCampBase() { return false; }
	int IsCampaign (void)
		{return (falconType & FalconCampaignEntity) ? TRUE : FALSE;};
	int IsSim (void)
		{return (falconType & FalconSimEntity) ? TRUE : FALSE;};
	int IsSimObjective (void)
		{return (falconType & FalconSimObjective) ? TRUE : FALSE;};
	int IsPersistant (void)
		{return (falconType & FalconPersistantEntity) ? TRUE : FALSE;};
	void SetTypeFlag (int flag)					{ falconType |= flag; };
	void UnSetTypeFlag (int flag)				{ falconType &= ~flag; };
	void SetFalcFlag (int flag)					{ if (!(falconFlags & flag)) {falconFlags |= flag; MakeFlagsDirty();} };
	void UnSetFalcFlag (int flag)				{ if (falconFlags & flag) {falconFlags &= ~flag; MakeFlagsDirty ();} };
	int IsSetFalcFlag (int flag)				{ return falconFlags & flag; };
	int IsPlayer (void)							{ return IsSetFalcFlag(FEC_HASPLAYERS); };

	virtual int Wake (void) = 0;
	virtual int Sleep (void) = 0;
	virtual short GetCampID (void)=0;//				{ ShiWarning( "Illegal use of FalconEntity" ); return 0; };
	virtual uchar GetTeam (void)=0;	//			{ ShiWarning( "Illegal use of FalconEntity" ); return 0; };
	virtual uchar GetCountry (void)=0;//				{ ShiWarning( "Illegal use of FalconEntity" ); return 0; };
	virtual uchar GetDomain (void);
	virtual int GetRadarMode (void)				{ return FEC_RADAR_OFF; }
	// sfr: modified to uchar
	virtual void SetRadarMode (uchar)		{};
	virtual void ReturnToSearch (void)			{};
	// sfr: modified to uchar
	virtual void SetSearchMode (uchar)		{};
	virtual int CombatClass (void)				{ return 999; } // 2002-02-25 ADDED BY S.G. No combat class for non flight or non aircraft class
	virtual int OnGround (void)					{ return FALSE; }
	virtual int HasEntity(VuEntity *e) const    { return this == e; } // sfr: added for new driver
	virtual int IsMissile (void)				{ return FALSE; }
	virtual int IsLauncher (void)               { return FALSE; } // MLR 3/4/2004 - rocket pods
	virtual int IsBomb (void)					{ return FALSE; }
	virtual int IsGun (void)					{ return FALSE; }
	virtual int IsMover (void)					{ return FALSE; }
	virtual int IsVehicle (void)				{ return FALSE; }
	virtual int IsStatic (void)					{ return FALSE; }
	virtual int IsHelicopter (void)				{ return FALSE; }
	virtual int IsEject (void)					{ return FALSE; }
	virtual int IsAirplane (void)	   			{ return FALSE; }
	virtual int IsGroundVehicle (void) 			{ return FALSE; }
	virtual int IsShip (void) 			        { return FALSE; }
	virtual int IsWeapon (void)   				{ return FALSE; }
	virtual int IsExploding (void)				{ return FALSE; }
	virtual int IsDead (void)					{ return FALSE; }
	virtual int IsEmitting (void)				{ return FALSE; }
	// sfr: pure virtual now
	virtual float GetVt (void) const = 0;
	virtual float GetKias (void) const = 0;
	virtual MoveType GetMovementType (void)		{ return NoMove; }
	virtual int IsUnit (void)					{ return FALSE; }
	virtual int IsObjective (void)				{ return FALSE; }
	virtual int IsBattalion (void)				{ return FALSE; }
	virtual int IsBrigade (void)				{ return FALSE; }
	virtual int IsFlight() const				{ return FALSE; }
	virtual int IsSquadron (void)				{ return FALSE; }
	virtual int IsPackage (void)				{ return FALSE; }
	virtual int IsTeam (void)					{ return FALSE; }
	virtual int IsTaskForce (void)				{ return FALSE; }
	virtual int IsSPJamming (void)				{ return FALSE; }
	virtual int IsAreaJamming (void)            { return FALSE; }
	virtual int HasSPJamming (void)				{ return FALSE; }
	virtual int HasAreaJamming (void)			{ return FALSE; }
	virtual float GetRCSFactor (void)			{ return 0.0f; }
	virtual float GetIRFactor (void)			{ return 0.0f; }
	virtual int	GetRadarType (void);
	
	virtual uchar* GetDamageModifiers (void);
	void GetLocation(short* x, short* y) const;
	int GetAltitude() const						{ return FloatToInt32(ZPos() * -1.0F); }
	
	void SetOwner (FalconSessionEntity* session);
	void SetOwner (VU_ID sessionId);
	void DoFullUpdate (void);

	// Dirty Functions
	void ClearDirty (void);
	void MakeDirty (Dirty_Class bits, Dirtyness score);
	int GetDirty() const { return dirty_score; }
	int EncodeDirty (unsigned char **stream);
	void DecodeDirty (VU_BYTE **stream, long *size);

	// sfr: changed to receive realtime (so we know when to compute)
	// realTime is in ms
	static const VU_TIME SIMDIRTYDATA_INTERVAL = 200;
	static const VU_TIME CMPDIRTYDATA_INTERVAL = 200;
	static void DoSimDirtyData(VU_TIME realTime);
	static void DoCampaignDirtyData(VU_TIME realTime);

	void MakeFlagsDirty (void);
	void MakeFalconEntityDirty (Dirty_Falcon_Entity bits, Dirtyness score);

	// 2002-03-22 ADDED BY S.G. Needs them outside of battalion class
	virtual void SetAQUIREtimer(VU_TIME newTime)		{ };
	virtual void SetSEARCHtimer(VU_TIME newTime)		{ };
	// sfr: modified to uchar
	virtual void SetStepSearchMode(uchar)					{ };
	virtual VU_TIME GetAQUIREtimer(void)				{ return 0; };
	virtual VU_TIME GetSEARCHtimer(void)				{ return 0; };
	// END OF ADDED SECTION 2002-03-22

	int feLocalFlags;

	void SetFELocalFlag   (int flag)	{ feLocalFlags |= flag;  };
	void UnSetFELocalFlag (int flag)	{ feLocalFlags &= ~flag; };
	int IsSetFELocalFlag  (int flag)	{ return feLocalFlags & flag; };
};
// sfr: FalconEntity container
typedef VuBin<FalconEntity> FalconEntityBin;

/** sfr: this class was duplicated in the code in cpp files. 
* Moved it here but maybe it should have its own file.
* Also renamed from GroundSpotEntity and TargetSpotEntity to SpotEntity
*/
class SpotEntity : public FalconEntity {
public:
	SpotEntity (ushort type);
	SpotEntity (VU_BYTE **, long *);
	virtual float GetVt() const { return 0; }
	virtual float GetKias() const { return 0; }
	virtual int Sleep ()      { return 0; }
	virtual int Wake ()       { return 0; }
	short GetCampID() { return 0; }
	uchar GetTeam()   { return 0; }
	uchar GetCountry(){ return 0; }
};


#endif
