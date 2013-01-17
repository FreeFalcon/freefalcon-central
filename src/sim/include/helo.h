#ifndef _HELICOPTER_CLASS_H
#define _HELICOPTER_CLASS_H

#include "simVeh.h"
#include "hardPnt.h"
#include "fsound.h"

#define FLARE_STATION	0
#define CHAFF_STATION	1
#define DEBRIS_STATION	2

// Class pointers used
class GunClass;
class FireControlComputer;
class SimObjectType;
class WeaponStation;
class DrawableTrail;
class HeliMMClass;
class SMSClass;
class HeliBrain;

class HelicopterClass : public SimVehicleClass
{
#ifdef USE_SH_POOLS
public:
	// Overload new/delete to use a SmartHeap fixed size pool
	void *operator new(size_t size) { ShiAssert( size == sizeof(HelicopterClass) ); return MemAllocFS(pool);	};
	void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
	static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(HelicopterClass), 20, 0 ); };
	static void ReleaseStorage()	{ MemPoolFree( pool ); };
	static MEM_POOL	pool;
#endif
public:
	HelicopterClass(VU_BYTE** stream, long *rem);
	HelicopterClass(FILE* filePtr);
	HelicopterClass(int type);
	virtual ~HelicopterClass();
	virtual void InitData();
	virtual void CleanupData();
private:
	void InitLocalData();
	void CleanupLocalData();
public:

	Int32 fireGun, fireMissile, lastPickle;
	FireControlComputer *FCC;
	SMSClass *Sms;
	GunClass* Guns;
	HeliMMClass *hf;
	TransformMatrix gunDmx;
	HeliBrain *hBrain;
	virtual void Init (SimInitDataClass* initData);
	virtual int Wake(void);
	virtual int Sleep(void);
public:
	virtual int Exec (void);
	virtual void GetTransform(TransformMatrix vmat);
	virtual void ApplyDamage (FalconDamageMessage *damageMessage);
	virtual void JoinFlight (void);
	virtual void SetLead (int flag);
	virtual void ReceiveOrders (FalconEvent* newOrder);
	virtual float GetP (void);
	virtual float GetQ (void);
	virtual float GetR (void);
	virtual float GetGamma(void);
	virtual float GetSigma(void);
	virtual float GetMu(void);
	virtual FireControlComputer* GetFCC(void) { return FCC; };
	virtual SMSBaseClass* GetSMS(void) { return (SMSBaseClass*)Sms; };
	virtual int IsHelicopter (void) {return TRUE;};

public:
	TransformMatrix vmat;
	void DoWeapons (void);
	Int32 numGuns;
	float wpAlt;
	float lastMoveTime;
	void SetWPalt(float Alt) { wpAlt = Alt; };
	float GetWPalt() { return wpAlt; };

	void InitDamageStation (void);
	void CleanupDamageStation (void);
	void GatherInputs(void);
	void RunSensors(void);
	void LandingCheck(void);
	void RunExplosion (void);
	void ShowDamage (void);
	void MoveSurfaces(void);
	void RunGear (void);
	void OnGroundInit (SimInitDataClass* initData);

	// use for LOD stuff
	HelicopterClass* flightLead;
	int flightIndex;
	float distLOD;
	BOOL useDistLOD;
	void SetDistLOD( void );
	void PromoteSubordinates( void );
	void GetFormationPos( float *x, float *y, float *z );

	virtual float Mass(void);

	// 2002-03-27 added by MN, helos also need to return their combat class
	virtual int CombatClass (void); // 2002-03-04 MODIFIED BY S.G. Moved inside Flight.cpp
	// RV - Biker - Offset value in z-axis from bounding box
public:
		float offsetZ;
};

#endif

