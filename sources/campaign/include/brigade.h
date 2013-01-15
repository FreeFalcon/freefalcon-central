#ifndef BRIGADE_H
#define BRIGADE_H

#include "gndunit.h"

// =========================
// Brigade Class
// =========================

class BrigadeClass : public GroundUnitClass {
#ifdef USE_SH_POOLS
public:
    // Overload new/delete to use a SmartHeap fixed size pool
    void *operator new(size_t size) { ShiAssert( size == sizeof(BrigadeClass) ); return MemAllocFS(pool);	};
    void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
    static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(BrigadeClass), 200, 0 ); };
    static void ReleaseStorage()	{ MemPoolFree( pool ); };
    static MEM_POOL	pool;
#endif

private:
	uchar			elements;							// Number of child units
	mutable uchar	c_element;							// Which one we're looking at
	VU_ID			element[MAX_UNIT_CHILDREN];			// VU_IDs of elements
	short			fullstrength;						// How many vehicles we were before we took losses
public:
	// constructors and serial functions
	BrigadeClass(ushort type);
	BrigadeClass(VU_BYTE **stream, long *rem);
	virtual ~BrigadeClass();
	virtual int SaveSize (void);
	virtual int Save (VU_BYTE **stream);

	// event Handlers
	virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);

	virtual int IsBrigade (void)								{ return TRUE; }

	// Access Functions
	void SetElements (uchar);
	void SetCElement (uchar);
	void SetElement (int i, VU_ID);
	void SetFullStrength (short);

	uchar GetElements (void) { return elements; }
	uchar GetCElement (void) { return c_element; }
	VU_ID GetElement (int i) { return element[i]; }
	short GetFullStrength (void) { return fullstrength; }

	// Required pure virtuals handled by Brigade class
	virtual int MoveUnit (CampaignTime time);
	virtual int Reaction (CampEntity what, int zone, float range);
	virtual int ChooseTactic (void);
	virtual int CheckTactic (int tid);
	virtual int Father() const									{ return 1; }
	virtual int Real (void)										{ return 0; }
	virtual void SetUnitOrders (int o, VU_ID oid);
	//virtual void SetUnitAction (void);
	virtual int GetUnitSpeed() const;
	virtual int GetUnitSupply (void);
	virtual int GetUnitMorale (void);
	virtual int GetUnitSupplyNeed (int total);
	virtual int GetUnitFuelNeed (int total);
	virtual void SupplyUnit (int supply, int fuel);

	// Core functions
	virtual void SetUnitDivision (int d);
	virtual int OrderElement(Unit e, F4PFList l);
	virtual Unit GetFirstUnitElement() const;
	virtual Unit GetNextUnitElement() const;
	virtual Unit GetUnitElement (int e);
	virtual Unit GetUnitElementByID (int eid);
	virtual Unit GetPrevUnitElement (Unit e);
	virtual void AddUnitChild (Unit e);
	virtual void DisposeChildren (void);
	virtual void RemoveChild (VU_ID eid);
	virtual void ReorganizeUnit (void);
	virtual int UpdateParentStatistics (void);
	virtual int RallyUnit (int minutes);
};

typedef BrigadeClass* Brigade;

BrigadeClass* NewBrigade (int type);

#endif
