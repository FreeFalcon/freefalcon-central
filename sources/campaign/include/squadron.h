
#ifndef SQUADRON_H
#define SQUADRON_H

#include "find.h"
#include "airunit.h"
#include "pilot.h"

// Define to flag moving aircraft from reserve
#define UMSG_FROM_RESERVE   255

// Defines for types of stats update
#define ASTAT_AAKILL		0
#define ASTAT_AGKILL		1
#define ASTAT_ASKILL		2
#define ASTAT_ANKILL		3
#define ASTAT_MISSIONS		4
#define ASTAT_PKILL			5								// Player kill

#define SQUADRON_PT_FUEL				100					// How many lbs each point of fuel is worth
#define SQUADRON_PT_SUPPLY				20					// How many weapon shots each point of supply is worth
#define SQUADRON_MISSIONS_PER_HOUR		4					// How many missions we expect each plane to fly per hour

#define SQUADRON_SPECIALTY_AA			1					// Specialty values
#define SQUADRON_SPECIALTY_AG			2

// =========================
// Squadron Class
// =========================

class SquadronClass : public AirUnitClass {
#ifdef USE_SH_POOLS
public:
    // Overload new/delete to use a SmartHeap fixed size pool
    void *operator new(size_t size) { ShiAssert( size == sizeof(SquadronClass) ); return MemAllocFS(pool);	};
    void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
    static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(SquadronClass), 200, 0 ); };
    static void ReleaseStorage()	{ MemPoolFree( pool ); };
    static MEM_POOL	pool;
#endif

private:
	long				fuel;							// fuel avail, 100s of lbs
	uchar				specialty;    					// Squadron's specialty
	uchar				stores[MAXIMUM_WEAPTYPES];		// # of weapons available
	PilotClass			pilot_data[PILOTS_PER_SQUADRON];// Pilot info
	ulong				schedule[VEHICLE_GROUPS_PER_UNIT];	// Aircraft usage schedule.
	VU_ID				airbase_id;						// ID of this squadron's airbase/carrier
	VU_ID				hot_spot;						// ID of 'primary' Primary Objective
	uchar				rating[ARO_OTHER];				// Rating by mission roll
	short				aa_kills;						// Kill counts (air to air)
	short				ag_kills;						// (air to ground)
	short				as_kills;						// (air to static)
	short				an_kills;						// (air to naval)
	short				missions_flown;
	short				mission_score;
	uchar				total_losses;					// Total aircraft losses since start of war
	uchar				pilot_losses;					// Total pilot losses since start of war
	uchar				assigned;						// Assigned to current package
	uchar				squadron_patch;					// ID of this squadron's patch art
	int					dirty_squadron;
	CampaignTime		last_resupply_time;				// Last time we received supply/reinforcements
	uchar				last_resupply;					// Number of aircraft we received

public:

	// Access Functions
	uchar GetAvailableStores(int i);
	ulong GetSchedule(int i) { return schedule[i]; }
	VU_ID GetHotSpot(void) { return hot_spot; }
	uchar GetRating(int i) { return rating[i]; }
	short GetAAKills(void) { return aa_kills; }
	short GetAGKills(void) { return ag_kills; }
	short GetASKills(void) { return as_kills; }
	short GetANKills(void) { return an_kills; }
	short GetMissionsFlown(void) { return missions_flown; }
	short GetMissionScore(void) { return mission_score; }
	uchar GetTotalLosses(void) { return total_losses; }
	uchar GetPilotLosses(void) { return pilot_losses; }
	uchar GetAssigned (void) { return assigned; }
	uchar GetPatchID (void) { return squadron_patch; }
	
	void SetSchedule(int, ulong);	// OR ulong into schedule
	void ClearSchedule(int);		// set it to 0
	void ShiftSchedule(int);		// shift it to the right 1 bit
	void SetHotSpot(VU_ID);
	void SetRating(int, uchar);
	void SetAAKills(short);
	void SetAGKills(short);
	void SetASKills(short);
	void SetANKills(short);
	void SetMissionsFlown(short);
	void SetMissionScore(short);
	void SetTotalLosses(uchar);
	void SetPilotLosses(uchar);
	void SetAssigned (uchar);

	// Other Functions
	SquadronClass(ushort type);
	SquadronClass(VU_BYTE **stream, long *rem);
	virtual ~SquadronClass();
	virtual int SaveSize (void);
	virtual int Save (VU_BYTE **stream);

	// event Handlers
	virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);

	// Required pure virtuals
	virtual int Reaction (CampEntity, int, float)	{	return 0; }
	virtual int MoveUnit (CampaignTime);
	// RV - Biker - Same for choppers
	virtual int MoveChopperUnit (CampaignTime);
	// RV - Biker - Move this to appropriate function
	void Scramble (void);
	virtual int ChooseTactic (void)					{ return 0; }
	virtual int CheckTactic (int)				{ return 0; }
	virtual int Real (void)							{ return 0; }
	virtual int IsSquadron (void)					{ return TRUE; }
	virtual int GetUnitSupplyNeed (int total);
	virtual int GetUnitFuelNeed (int total);
	virtual void SupplyUnit (int supply, int fuel);

	// Dirty Data Stuff
	void MakeSquadronDirty (Dirty_Squadron bits, Dirtyness score);
	void WriteDirty (unsigned char **stream);
	//sfr: added rem
	void ReadDirty (unsigned char **stream, long *rem);

	// Core functions
	virtual void UseFuel (long f);
	virtual void SetSquadronFuel (long f);
	virtual void SetUnitSpecialty (int s)			{ specialty = (uchar)s; }
	virtual void SetUnitStores (int w, uchar v);
	virtual void SetUnitAirbase (VU_ID ID);
	virtual void SetLastResupply (int s);
	virtual void SetLastResupplyTime (CampaignTime t)	{ last_resupply_time = t; }
	virtual long GetSquadronFuel (void)				{ return fuel; }
	virtual int GetUnitSpecialty (void)				{ return (int)specialty; }
	virtual uchar GetUnitStores (int w)				{ return stores[w]; }
	virtual CampaignTime GetLastResupplyTime (void)	{ return last_resupply_time; }
	virtual int GetLastResupply (void)				{ return last_resupply; }
	virtual CampEntity GetUnitAirbase (void)		{ return FindEntity(airbase_id); }
	virtual VU_ID GetUnitAirbaseID (void)			{ return airbase_id; }
	virtual void DisposeChildren (void);
	int GetPilotID (int pilot)						{ return pilot_data[pilot].pilot_id; }
	PilotClass* GetPilotData (int pilot)			{ return &pilot_data[pilot]; }
	PilotInfoClass* GetPilotInfo (int pilot)		{ return &PilotInfo[pilot_data[pilot].pilot_id]; }
	int NumActivePilots (void);
	void InitPilots (void);
	void ReinforcePilots (int max_new_pilots);
	void SetPilotStatus (int pilot, int s)			{ pilot_data[pilot].pilot_status = (uchar)s; }
	int GetPilotStatus (int pilot)					{ return pilot_data[pilot].pilot_status; }
	void ScoreKill (int pilot, int killtype);
	void ScoreMission (short missions)				{ missions_flown = static_cast<short>(missions_flown + missions); } // this looks silly but gets rid of warning, since changing the type could invalidate save files
	void ShiftSchedule (void);
	int FindAvailableAircraft (MissionRequest mis);
	void ScheduleAircraft (Flight fl, MissionRequest mis);
	int AssignPilots (Flight fl);
	void UpdateSquadronStores (short weapon[HARDPOINT_MAX], uchar weapons[HARDPOINT_MAX], int lbsfuel, int planes);
// 2001-12-28 M.N.
	void ResupplySquadronStores (short weapon[HARDPOINT_MAX], uchar weapons[HARDPOINT_MAX], int lbsfuel, int planes);
// 2001-07-05 ADDED BY S.G. NEED SOMETHING TO HOLD THE NEW VARIABLE FOR squadron RETASKING ONCE REALLOCATED
	CampaignTime squadronRetaskAt;
};

typedef SquadronClass* Squadron;

// ============================================
// Supporting functions
// ============================================

class FlightClass;
typedef FlightClass* Flight;

SquadronClass* NewSquadron (int type);

#endif
