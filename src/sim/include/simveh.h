#ifndef _SIM_VEHICLE_H
#define _SIM_VEHICLE_H

#include "simmover.h"

class BaseBrain;
class FireControlComputer;
class PilotInputs;
class RadioClass;
class WayPointClass;
class FalconDamageMessage;
class FalconDeathMessage;
class FalconEvent;
class SimWeaponClass;
class SMSBaseClass;

class SimVehicleClass : public SimMoverClass
{
protected:
	// Controls
	BaseBrain* theBrain;
	//      int numHardpoints;
	float ioPerturb;
	float irOutput;//me123
	// JB 000814
	float rBias;
	float pBias;
	float yBias; 
	// JB 000814

	// how does the thing die
	int dyingType;
	enum
	{
		DIE_SMOKE		= 0,
		DIE_FIREBALL,
		DIE_SHORT_FIREBALL,
		DIE_INTERMITTENT_FIRE,
		DIE_INTERMITTENT_SMOKE
	};

	// RV - I-Hawk - AC dying time is now randomized. maybe long,short, or sometiems immediate...
	// Value is intialized in Aircraft.cpp constructor, and used in simveh.cpp with the dying 
	// timer...  
   	int dyingTime;  

public:
	// rates and timers for targeting
	VU_TIME nextTargetUpdate;
	VU_TIME targetUpdateRate;
	VU_TIME nextGeomCalc;
	VU_TIME geomCalcRate;

public:
	void SendFireMessage(
		SimWeaponClass* curWeapon, int type, int startFlag, SimObjectType* targetPtr, VU_ID tgtId = FalconNullId
	);
	// Avionics
	PilotInputs* theInputs;
	void InitWeapons (ushort *type, ushort *num);
	enum SOI {SOI_HUD, SOI_RADAR, SOI_WEAPON, SOI_FCC}; //MI added SOI_FCC for HSD
	SOI curSOI;
	void SOIManager(SOI newSOI);
	SOI GetSOI(void)	{return curSOI;};	//MI
	void StepSOI(int dir);	//MI

	BaseBrain* Brain(void) {return theBrain;};

	//Steering Info
	WayPointClass* curWaypoint;
	WayPointClass* waypoint;
	Int32  numWaypoints;

	WayPointClass *GetWayPointNo(int n);
	virtual void ReceiveOrders (FalconEvent*) {};

	void ApplyProximityDamage (void);

	// for dying, we no longer send the death message as soon as
	// strength goes to 0, we delay until object explodes
	FalconDeathMessage *deathMessage;

	SimVehicleClass(int type);
	SimVehicleClass(VU_BYTE** stream, long *rem);
	SimVehicleClass(FILE* filePtr);
	virtual ~SimVehicleClass (void);
	virtual void InitData();
	virtual void CleanupData();
private:
	void InitLocalData();
	void CleanupLocalData();
public:

	virtual void Init (SimInitDataClass* initData);
	virtual int Wake (void);
	virtual int Sleep (void);
	virtual void MakeLocal (void);
	virtual void MakeRemote (void);
	virtual int Exec (void);
	virtual void SetDead (int);
	virtual void ApplyDamage (FalconDamageMessage *damageMessage);
	virtual FireControlComputer* GetFCC(void) { return NULL; };
	virtual SMSBaseClass* GetSMS(void) { return NULL; };
	virtual float GetRCSFactor (void);
	virtual float GetIRFactor (void);
	virtual int GetRadarType (void);
	virtual long	GetTotalFuel (void) { return -1; }; // KCK: -1 means "unfueled vehicle"

	//all ground vehicles and helicopters have pilots until they die
	//this determines whether they can talk on the radio or not
	virtual int HasPilot (void)		{ return TRUE;}
	virtual int IsVehicle (void)		{ return TRUE;}

	// this function can be called for entities which aren't necessarily
	// exec'd in a frame (ie ground units), but need to have their
	// gun tracers and (possibly other) weapons serviced
	virtual void WeaponKeepAlive( void ) { return; };

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
};

#endif
