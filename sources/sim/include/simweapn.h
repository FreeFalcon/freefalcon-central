#ifndef _SIM_WEAPON_H
#define _SIM_WEAPON_H

#include "simmover.h"

class SensorClass;
class SimWeaponClass;
struct SimWeaponDataType; // MLR 3/7/2004 - 
struct WeaponClassDataType; // MLR 3/7/2004 - 
#include "entity.h"


class SimWeaponClass : public SimMoverClass
{
public:
	SimWeaponClass(int type);
	SimWeaponClass(VU_BYTE** stream, long *rem);
	SimWeaponClass(FILE* filePtr);
	virtual ~SimWeaponClass (void);
	virtual void InitData();
	virtual void CleanupData();
private:
	void InitLocalData();
	void CleanupLocalData();

public:
	VuBin<SimWeaponClass> nextOnRail;
	virtual int Sleep (void);
	virtual int Wake (void);
	//char parentReferenced;
	int rackSlot;
	uchar shooterPilotSlot;				// The pilotSlot of the pilot who shot this weapon
	Float32 lethalRadiusSqrd;
	FalconEntityBin parent;

public:
	virtual void Init (void);
	virtual int Exec (void);
	virtual void SetDead (int);

	SimWeaponClass* GetNextOnRail(void) { return nextOnRail.get(); }
	void SetRackSlot (int slot) { rackSlot = slot; }
	int GetRackSlot (void) { return rackSlot; }
	void SetParent(FalconEntity* newParent) { parent.reset(newParent); }
	FalconEntity* Parent(void) { return parent.get(); }

	// virtual function interface
	// serialization functions
	virtual int SaveSize();
	virtual int Save(VU_BYTE **stream);	// returns bytes written
	virtual int Save(FILE *file);		// returns bytes written

	// event handlers
	virtual int Handle(VuFullUpdateEvent *event);
	virtual int Handle(VuPositionUpdateEvent *event);
	virtual int Handle(VuTransferEvent *event);

	// other stuff
	void SendDamageMessage(FalconEntity *testObject, float rangeSquare, int damageType);

	virtual int IsWeapon (void) {return TRUE;};
	virtual int GetRadarType (void);

	short GetWeaponId(void); // MLR 3/5/2004 - Tired of having to figure out the weapon Id by hand.

	SimWeaponDataType      *GetSWD(void);
	Falcon4EntityClassType *GetCT(void);
	WeaponClassDataType    *GetWCD(void);

	virtual int IsUseable (void) {return TRUE;}

	// MD -- 20040613: hold pickle until delay time passes or weapon won't launch
	// Currently only implemented in MissileClass but this would allow others as
	// well if that turns out to be useful at some point.
	virtual int LaunchDelayTime(void) { return 0; }  
};

#endif
