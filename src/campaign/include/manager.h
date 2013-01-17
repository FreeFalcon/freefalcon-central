
#ifndef MANAGER_H
#define MANAGER_H

#include "Entity.h"
#include "F4Vu.h"

// ===================================
// Manager flags
// ===================================

#define	CTM_MUST_BE_OWNED		0x01				// Do this action only if tasker owner by this machine

// ===================================
// Manager class
// ===================================

class CampManagerClass;
typedef CampManagerClass* CampManager;

class CampManagerClass : public FalconEntity 
{
private:
public:
	short			managerFlags;				// Various user flags
	Team			owner;						// Controlling country/team

public:
	// Constructors
	CampManagerClass (ushort type, Team t);
	CampManagerClass (VU_BYTE **stream, long *rem);
	CampManagerClass (FILE *file);
	~CampManagerClass (void);
	virtual void InitData();
private:
	void InitLocalData(Team  t);
public:
	virtual int SaveSize (void);
	virtual int Save (VU_BYTE **stream);
	virtual int Save (FILE *file);

	// pure virtual interface
	virtual float GetVt() const { return 0; }
	virtual float GetKias() const { return 0; }

	// event handlers
	virtual int Handle(VuEvent *event);
	virtual int Handle(VuFullUpdateEvent *event);
	virtual int Handle(VuPositionUpdateEvent *event);
	virtual int Handle(VuEntityCollisionEvent *event);
	virtual int Handle(VuTransferEvent *event);
	virtual int Handle(VuSessionEvent *event);
	virtual VU_ERRCODE InsertionCallback(void);
	virtual VU_ERRCODE RemovalCallback(void);
	virtual int Wake (void) {return 0;};
	virtual int Sleep (void) {return 0;};

	// Required pure virtuals
	virtual int Task()            { return 0; }
	virtual void DoCalculations() {}
	virtual short GetCampID(void) { return 0; }
	virtual uchar GetTeam(void)   { return 0; }
	virtual uchar GetCountry(void){ return 0; }

	// Core functions
	int MyTasker (ushort)         { return IsLocal(); }
	int GetTaskTeam (void)        { return owner; }
	void SendMessage (VU_ID id, short msg, short d1, short d2, short d3);
};

#include "ATM.h"
#include "GTM.h"
#include "NTM.h"

// ===========================
// Global functions
// ===========================

extern VuEntity* NewManager (short tid, VU_BYTE **stream, long *rem);

#endif
