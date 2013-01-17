/******************************************************************************
*
* VuList managing routines
*
*****************************************************************************/

#ifndef CAMPLIST_H
#define CAMPLIST_H

#include <list>
#include "CampBase.h"
#include "listadt.h"
#include "F4Vu.h"

// ==================================
// Unit specific filters
// ==================================

class UnitFilter : public VuFilter {
public:
	uchar		parent;					// Set if parents only
	uchar		real;					// Set if real only
	ushort		host;					// Set if this host only
	uchar		inactive;				// active or inactive units only

public:
	UnitFilter(uchar p, uchar r, ushort h, uchar a);
	virtual ~UnitFilter(void)			{}

	virtual VU_BOOL Test(VuEntity *ent);	
	virtual VU_BOOL RemoveTest(VuEntity *ent);
	virtual int Compare(VuEntity *ent1, VuEntity *ent2)	{ return (SimCompare (ent1, ent2)); }
	virtual VuFilter *Copy()			{ return new UnitFilter(parent,real,host,inactive); }
};

class AirUnitFilter : public VuFilter {
public:
	uchar		parent;					// Set if parents only
	uchar		real;					// Set if real only
	ushort		host;					// Set if this host only

public:
	AirUnitFilter(uchar p, uchar r, ushort h);
	virtual ~AirUnitFilter(void)		{}

	virtual VU_BOOL Test(VuEntity *ent);	
	virtual VU_BOOL RemoveTest(VuEntity *ent);
	virtual int Compare(VuEntity *ent1, VuEntity *ent2)	{ return (SimCompare (ent1, ent2)); }
	virtual VuFilter *Copy()			{ return new AirUnitFilter(parent,real,host); }
};

class GroundUnitFilter : public VuFilter 
{
public:
	uchar		parent;					// Set if parents only
	uchar		real;					// Set if real only
	ushort		host;					// Set if this host only

public:
	GroundUnitFilter(uchar p, uchar r, ushort h);
	virtual ~GroundUnitFilter(void)		{}

	virtual VU_BOOL Test(VuEntity *ent);	
	virtual VU_BOOL RemoveTest(VuEntity *ent);
	virtual int Compare(VuEntity *ent1, VuEntity *ent2)	{ return (SimCompare (ent1, ent2)); }
	virtual VuFilter *Copy()			{ return new GroundUnitFilter(parent,real,host); }
};

class NavalUnitFilter : public VuFilter 
{
public:
	uchar		parent;					// Set if parents only
	uchar		real;					// Set if real only
	ushort		host;					// Set if this host only

public:
	NavalUnitFilter(uchar p, uchar r, ushort h);
	virtual ~NavalUnitFilter(void)		{}

	virtual VU_BOOL Test(VuEntity *ent);	
	virtual VU_BOOL RemoveTest(VuEntity *ent);
	virtual int Compare(VuEntity *ent1, VuEntity *ent2)	{ return (SimCompare (ent1, ent2)); }
	virtual VuFilter *Copy()			{ return new NavalUnitFilter(parent,real,host); }
};

extern UnitFilter			AllUnitFilter;
extern AirUnitFilter		AllAirFilter;
extern GroundUnitFilter		AllGroundFilter;
extern NavalUnitFilter		AllNavalFilter;
extern UnitFilter			AllParentFilter;
extern UnitFilter			AllRealFilter;
extern UnitFilter			InactiveFilter;

/** corrects grid computations which are wrong. */
#define GRID_CORRECTION 1

// Unit proximity filters
class UnitProxFilter : public VuBiKeyFilter
{
public:
#if GRID_CORRECTION
	/** filter that converts from max to res. Used in grids. */
	UnitProxFilter(unsigned int res, BIG_SCALAR max) : VuBiKeyFilter(res, max){}
	UnitProxFilter(const UnitProxFilter &other) : VuBiKeyFilter(other){}
	virtual ~UnitProxFilter(){}
	virtual VuFilter *Copy(){ return new UnitProxFilter(*this); }
#else
	UnitProxFilter(int r);
	UnitProxFilter(const UnitProxFilter *other, int r);
	virtual VuFilter *Copy(){ return new UnitProxFilter(real); }
#endif

	virtual VU_BOOL Test(VuEntity *ent);
	virtual VU_BOOL RemoveTest(VuEntity *ent);
};

extern UnitProxFilter*	AllUnitProxFilter;
extern UnitProxFilter*	RealUnitProxFilter;

// ==============================
// Manager Filters
// ==============================

//extern UnitFilter  AllTMFilter;
//extern UnitFilter  MyTMFilter;

// ==============================
// Objective specific filters
// ==============================

// Standard Objective filter
class ObjFilter : public VuFilter 
{
public:
	ushort		host;					// Set if this host only

public:
	ObjFilter(ushort h);
	virtual ~ObjFilter(void)			{}

	virtual VU_BOOL Test(VuEntity *ent);	
	virtual VU_BOOL RemoveTest(VuEntity *ent);
	virtual int Compare(VuEntity *ent1, VuEntity *ent2)	{ return (SimCompare (ent1, ent2)); }
	virtual VuFilter *Copy()			{ return new ObjFilter(host); }
};

extern ObjFilter  AllObjFilter;

// Objective Proximity filter
class ObjProxFilter : public VuBiKeyFilter {
public:
#if GRID_CORRECTION
	/** creates a filter which converts from max to res. Used in grids. */
	ObjProxFilter(unsigned int res, BIG_SCALAR max) : VuBiKeyFilter(res, max){}
	ObjProxFilter(const ObjProxFilter &other) : VuBiKeyFilter(other){}
	virtual ~ObjProxFilter(){}
	virtual VuFilter *Copy(){ return new ObjProxFilter(*this); }
#else
	ObjProxFilter();
	ObjProxFilter(const ObjProxFilter *other);
	virtual VuFilter *Copy(){ return new ObjProxFilter(); }
#endif

	virtual VU_BOOL Test(VuEntity *ent);
	virtual VU_BOOL RemoveTest(VuEntity *ent);
};

extern ObjProxFilter* AllObjProxFilter;

// ==============================
// General Filters
// ==============================

class CampBaseFilter : public VuFilter 
{
public:

public:
	CampBaseFilter(void)				{}
	virtual ~CampBaseFilter(void)		{}

	virtual VU_BOOL Test(VuEntity *ent);
	virtual VU_BOOL RemoveTest(VuEntity *ent);
	virtual int Compare(VuEntity *ent1, VuEntity *ent2)	{ return (SimCompare (ent1, ent2)); }
	virtual VuFilter *Copy()			{ return new CampBaseFilter(); }
};

extern CampBaseFilter CampFilter;

// ==============================
// Registered Lists
// ==============================

#if VU_ALL_FILTERED
extern VuLinkedList* AllUnitList;		// All units
extern VuLinkedList* AllAirList;		// All air units
extern VuLinkedList* AllParentList;		// All parent units
extern VuLinkedList* AllRealList;		// All real units
extern VuLinkedList* AllObjList;		// All objectives
extern VuLinkedList* AllCampList;		// All campaign entities
extern VuLinkedList* InactiveList;		// Inactive units (reinforcements)
#else
extern VuFilteredList *AllUnitList;		// All units
extern VuFilteredList *AllAirList;		// All air units
extern VuFilteredList *AllRealList;		// All real units
extern VuFilteredList *AllParentList;	// All parent units
extern VuFilteredList *AllObjList;		// All objectives
extern VuFilteredList *AllCampList;		// All campaign entities
extern VuFilteredList *InactiveList;	// Inactive units (reinforcements)
#endif

//==============================
// Maintained Containers
//==============================

#define MAX_DIRTY_BUCKETS	9 //me123 from 8

extern F4PFList FrontList;				// Frontline objectives
extern F4POList POList;					// Primary objective list
extern F4PFList SOList;					// Secondary objective list
extern F4PFList AirDefenseList;			// All air defenses
extern F4PFList EmitterList;			// All emitters

//extern TailInsertList *DirtyBucket[MAX_DIRTY_BUCKETS];	// dirty data
#define USE_VU_COLL_FOR_CAMPAIGN 1
#define USE_VU_COLL_FOR_DIRTY 1


#if USE_VU_COLL_FOR_DIRTY
extern TailInsertList *campDirtyBuckets[MAX_DIRTY_BUCKETS];
extern TailInsertList *simDirtyBuckets[MAX_DIRTY_BUCKETS];
#else
typedef std::list<FalconEntityBin> FalconEntityList;
extern FalconEntityList *campDirtyBuckets[MAX_DIRTY_BUCKETS];
extern FalconEntityList *simDirtyBuckets[MAX_DIRTY_BUCKETS];
#endif

#if USE_VU_COLL_FOR_CAMPAIGN
	#if VU_ALL_FILTERED
extern VuHashTable *deaggregatedEntities;
	#else
extern VuFilteredHashTable *deaggregatedEntities;
	#endif
#else
/** associative container for CampBase */
typedef std::map<VU_ID, CampBaseBin> StdCampBaseMap;
class CampBaseMap : private StdCampBaseMap {
public:
	typedef StdCampBaseMap::iterator iterator;
	// constructor and destructors
	explicit CampBaseMap(const std::string &name);
	~CampBaseMap();
	void insert(CampBaseBin cb);
	void remove(const VU_ID &id);
	const iterator begin();
	const iterator end();
	F4CSECTIONHANDLE *getMutex(){ return mutex; }
private:
	F4CSECTIONHANDLE *mutex;
};
extern CampBaseMap *deaggregatedMap;     // All deaggregated entities
#endif

extern F4CSECTIONHANDLE *campDirtyMutexes[MAX_DIRTY_BUCKETS];
extern F4CSECTIONHANDLE *simDirtyMutexes[MAX_DIRTY_BUCKETS];


// ==============================
// Objective data lists
// ==============================

extern List PODataList;

// Front Line List
extern List FLOTList;

// ==============================
// Proximity Lists
// ==============================

extern VuGridTree* ObjProxList;			// Proximity list of all objectives
extern VuGridTree* RealUnitProxList;	// Proximity list of all real units

// ==============================
// Global Iterators
// ==============================

// ==============================
// List maintenance routines
// ==============================

extern void InitLists (void);

extern void InitProximityLists (void);

extern void InitIALists (void);

extern void DisposeLists (void);

extern void DisposeProxLists (void);

extern void DisposeIALists (void);

extern int RebuildFrontList (int do_barcaps, int incremental);

extern int RebuildObjectiveLists(void);

extern int RebuildParentsList (void);

extern int RebuildEmitterList (void);

extern void StandardRebuild (void);

void InactivateUnit(UnitClass *unit);

#endif
