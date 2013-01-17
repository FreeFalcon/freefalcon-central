#ifndef ATM_H
#define ATM_H

// ==================================
// Air Tasking Manager class
// ==================================

#include "listadt.h"
#include "falcmesg.h"
#include "unit.h"
#include "objectiv.h"
#include "team.h" 
#include "CampList.h"
#include "Manager.h"
#include "AirUnit.h"

#define ATM_STEALTH_AVAIL	0x01							// We've got stealth aircraft to use
#define ATM_NEW_PLANES		0x04							// We've got more planes to play with
#define ATM_NEW_REQUESTS	0x08							// We've got one or more new mission requests

#define ATM_MAX_CYCLES		32								// Bit size of a long

// Our scheduling requires a bitwise array of 1 minute time blocks. We've got 8 bits, so full would = 0xFF
// However, if our cycle is < 8 minutes long, we need modify the meaning of 'full'
#define ATM_CYCLE_FULL		0x1F							//	What a full schedule looks like (5 bits)

class ATMAirbaseClass
	{
	public:
		VU_ID			id;
		uchar			schedule[ATM_MAX_CYCLES];
		uchar			usage;
		ATMAirbaseClass	*next;
	public:
		ATMAirbaseClass(void);
		ATMAirbaseClass(CampEntity ent);
		//sfr: changed proto
		ATMAirbaseClass(VU_BYTE **stream, long *size);
		ATMAirbaseClass(FILE *file);
		~ATMAirbaseClass();
		int Save(VU_BYTE **stream);
		int Save(FILE *file);
		int Size(void)					{ return sizeof(VU_ID) + ATM_MAX_CYCLES; }
	};

class AirTaskingManagerClass : public CampManagerClass {
	private:
	public:
		// Transmittable data
		short			flags;
		short			squadrons;							// Number of available friendly squadrons
		short			averageCAStrength;					// Rolling average CA strength of CA missions
		short			averageCAMissions;					// Average # of CA missions being flow per hour
		uchar			currentCAMissions;					// # of CA missions planned so far during current cycle
		uchar			sampleCycles;						// # of cycles we've averaged missions over
		// Anything below here doesn't get transmitted
		int				missionsToFill;						// Amount of missions above to actually task.
		int				missionsFilled;						// # actually filled to date
		List			awacsList;							// List of awacs/jstar locations
		List			tankerList;							// List of tanker track locations
		List			ecmList;							// List of standoff jammer locations
		List			requestList;						// List of mission requests yet to be processed.
		List			delayedList;						// List of mission requests already handled, but not filled
		F4PFList		squadronList;						// List of this team's squadrons
		F4PFList		packageList;						// List of all active packages
		ATMAirbaseClass	*airbaseList;						// List of active airbases
		uchar			supplyBase;
		uchar			cycle;								// which planning block we're in.
		CampaignTime	scheduleTime;						// Last time we updated our blocks
	public:
		// constructors & serial functions
		AirTaskingManagerClass(ushort type, Team t);
		//sfr: changed proto
		//AirTaskingManagerClass(VU_BYTE **stream);
		AirTaskingManagerClass(VU_BYTE **stream, long *rem);
		AirTaskingManagerClass(FILE *file);
		virtual ~AirTaskingManagerClass();
		virtual int SaveSize (void);
		virtual int Save (VU_BYTE **stream);
		virtual int Save (FILE *file);

		// Required pure virtuals
		virtual int Task();
		virtual void DoCalculations();
		virtual int Handle(VuFullUpdateEvent *event);

		// core functions
		int BuildPackage (Package *pc, MissionRequest mis);
		int BuildDivert(MissionRequest mis);
		int BuildSpecificDivert(Flight flight);
		void ProcessRequest(MissionRequest request);
		Squadron FindBestAir(MissionRequest mis, GridIndex bx, GridIndex by);
		Flight FindBestAirFlight(MissionRequest mis);
		void SendATMMessage(VU_ID from, Team to, short msg, short d1, short d2, void* d3, int flags);
		int FindTakeoffSlot(VU_ID abid, WayPoint w);
		void ScheduleAircraft(VU_ID abid, WayPoint wp, int aircraft);
		void ZapAirbase(VU_ID abid);
		void ZapSchedule(int rw, ATMAirbaseClass *airbase, int tilblock);
		ATMAirbaseClass* FindATMAirbase(VU_ID abid);
		ATMAirbaseClass* AddToAirbaseList (CampEntity airbase);
		int FindNearestActiveTanker(GridIndex *x, GridIndex *y, CampaignTime *time);
		int FindNearestActiveJammer(GridIndex *x, GridIndex *y, CampaignTime *time);
	};

typedef AirTaskingManagerClass *AirTaskingManager;
typedef AirTaskingManagerClass *ATM;

// ==========================================
// Global functions
// ==========================================

enum RequIntHint { // 2001-10-27 ADDED BY S.G. Tells the RequestIntercept function not to ignore anything
	RI_NORMAL, RI_HELP
};

extern void InitATM (void);

extern void EndATM (void);

extern int LoadMissionLists (char* scenario);

extern int SaveMissionLists (char* scenario);

extern int RequestSARMission (FlightClass* flight);

extern void RequestIntercept (FlightClass* enemy, int who, RequIntHint hint = RI_NORMAL);

extern int TargetAllSites (Objective po, int action, int team, CampaignTime startTime);

//extern void TargetAdditionalSites (void);

#endif

