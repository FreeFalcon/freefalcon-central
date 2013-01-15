#ifndef _GROUND_H
#define _GROUND_H

#include "f4vu.h"
#include "simveh.h"
#include "simbase.h"
#include "gndai.h"
#include "msginc\trackmsg.h"
#include "fsound.h"


class WayPointClass;
class SimInitDataClass;
class SimObjectType;
class GunClass;
class DrawableTrail;
class DrawableObject;
class SMSBaseClass;
class DrawableGuys;
class DrawableGroundVehicle;

class GroundClass : public SimVehicleClass {
public:
	GroundClass (VU_BYTE** stream, long *rem);
	GroundClass (FILE* filePtr);
	GroundClass (int type);
	virtual ~GroundClass (void);
	virtual void InitData();
	virtual void CleanupData();
private:
	void InitLocalData();
	void CleanupLocalData();
public:

	void InitFromCampaignUnit(void);
	int MissileTrack(void);
	int GunTrack(void);

	GNDAIClass *gai;					// AI object
	int groupId;						// group id
	ulong lastProcess;
	ulong lastThought;
	ulong thoughtRate;
	ulong processRate;
	ulong nextSamFireTime;


	unsigned isFootSquad:		1;		// Since these are booleans, they can each be one bit
	unsigned isTowed:			1;
	unsigned isEmitter:		1;
	unsigned hasCrew:			1;
	unsigned needKeepAlive:	1;
	unsigned isAirCapable:	1;
	unsigned isGroundCapable:	1;
	unsigned isAirDefense:	1;
	unsigned allowSamFire:	1;
	unsigned isShip:		1;

	// RV - Biker
	unsigned radarDown: 1;

	// Other drawables associated with ground stuff
	//	  DrawableGuys *crewDrawable;					// Crew, if any		-> KCK: Done with a switch now
	DrawableGroundVehicle *truckDrawable;			// Tow vehicle (truck), if any

	// Weapon stuff
	SMSBaseClass	*Sms;
	GroundClass	*battalionFireControl;	// The active fire control radar vehicle in this battalion
	void FindBattalionFireControl( void );


	// setting, querying gun fire state functions
	BOOL IsGunFiring ( int i )	{ return (gunFireFlags & (1 << i) ); };
	void SetGunFiring ( int i )	{ gunFireFlags |= (1 << i);  };
	void UnSetGunFiring ( int i )	{ gunFireFlags &= ~(1 << i); };

	void RunSensors(void);
	void SelectWeapon(int gun_only);
	BOOL DoWeapons(void);
	void RotateTurret(void);
	void SetupGNDAI (SimInitDataClass *idata);
	void JoinFlight (void) {};
	virtual int Wake (void);
	virtual int Sleep (void);
	virtual void Init (SimInitDataClass* initData);
	virtual int Exec (void);
	virtual void ApplyDamage (FalconDamageMessage *damageMessage);
	virtual void SetDead (int flag);
	virtual float GetNz(void) {return (1.0F);};
	virtual SMSBaseClass* GetSMS(void) { return Sms; };
	virtual int IsGroundVehicle (void) {return TRUE;};
	virtual VU_ERRCODE InsertionCallback(void);
	virtual VU_ERRCODE RemovalCallback(void);

	// this function can be called for entities which aren't necessarily
	// exec'd in a frame (ie ground units), but need to have their
	// gun tracers and (possibly other) weapons serviced
	virtual void WeaponKeepAlive( void );

protected:
	// guns stuff
	ulong gunFireFlags;		// is gun firing?  One bit per hardpoint
	TransformMatrix gunDmx;

#ifdef USE_SH_POOLS
public:
	// Overload new/delete to use a SmartHeap fixed size pool
	void *operator new(size_t size) { ShiAssert( size == sizeof(GroundClass) ); return MemAllocFS(pool);	};
	void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
	static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(GroundClass), 200, 0 ); };
	static void ReleaseStorage()	{ MemPoolFree( pool ); };
	static MEM_POOL	pool;
#endif
};

#define TURRET_ROTATE_RATE		30.0F*DTR
#define TURRET_ELEVATE_RATE		15.0F*DTR

enum{
	TURRET_CALC_RATE = 500,
	// RV - Biker - IR SAMs (MANPADS) mostly do use this so make it faster
	//MAX_SAM_FIRE_RATE = 30000,
	MAX_SAM_FIRE_RATE = 10000,
};

#endif

