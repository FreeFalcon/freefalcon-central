#ifndef GTM_H
#define GTM_H

// ==================================
// Ground Tasking Manager class
// ==================================

#include "falcmesg.h"
#include "unit.h"
#include "objectiv.h"
#include "team.h"
#include "CampList.h"
#include "Manager.h"
#include "Division.h"
#include "GtmObj.h"
#include "GndUnit.h"

struct GroundDoctrineType {
	uchar			stance[NUM_COUNS];					// Our air stance towards them: allied/friendly/neutral/alert/hostile/war
	uchar			loss_ratio;							// Acceptable loss ratio
	uchar			loss_score;							// Score loss for each friendly air loss
	};

class GroundTaskingManagerClass : public CampManagerClass
	{
	private:
	public:
		short			flags;
		// These don't need to be transmitted
		GODNode			objList[GORD_LAST];				// Sorted lists of objectives we want to assign to
		USNode			canidateList[GORD_LAST];		// List of all possible canidate units for each order
		short			topPriority;					// Highest PO priority (for scaling)
		short			priorityObj;					// CampID of highest priority objective
	public:
		// constructors
		GroundTaskingManagerClass(ushort type, Team t);
		//sfr: added rem
		GroundTaskingManagerClass(VU_BYTE **stream, long *rem);
		GroundTaskingManagerClass(FILE *file);
		virtual ~GroundTaskingManagerClass();
		virtual int SaveSize (void);
		virtual int Save (VU_BYTE **stream);
		virtual int Save (FILE *file);
		virtual int Handle(VuFullUpdateEvent *event);

		// Required pure virtuals
		virtual void DoCalculations();
		virtual int Task();

		// support functions
		void Setup (void);
		void Cleanup (void);
		int GetAddBits (Objective o, int to_collect);
		int BuildObjectiveLists (int to_collect);
   		int CollectGroundAssets (int to_collect);
		void AddToList (Unit u, int orders);
		void AddToLists(Unit u, int to_collect);
		int IsValidObjective (int orders, Objective o);

		int	AssignUnit (Unit u, int orders, Objective o, int score);
		int AssignUnits (int orders, int mode);
		int AssignObjective (GODNode curo, int orders, int mode);

		int ScoreUnit (USNode curu, GODNode curo, int orders, int mode);
		int ScoreUnitFast (USNode curu, GODNode curo, int orders, int mode);

		void FinalizeOrders(void);

		// core functions
		void SendGTMMessage (VU_ID from, short message, short data1, short data2, VU_ID data3);

		// Private message handling functions (Called by Process())
		void RequestSupport (VU_ID enemy, int division);
		void RequestEngineer (Objective o, int division);
		void RequestAirDefense (Objective o, int division);

	};

typedef GroundTaskingManagerClass *GroundTaskingManager;
typedef GroundTaskingManagerClass *GTM;

// ==========================================
// Global functions
// ==========================================

extern int MinAdjustLevel(Unit u);

extern short EncodePrimaryObjectiveList (uchar teammask, uchar **buffer);

extern void DecodePrimaryObjectiveList (uchar *datahead, FalconEntity *fe);

extern void SendPrimaryObjectiveList (uchar teammask);

extern void SavePrimaryObjectiveList (char* scenario);

extern int LoadPrimaryObjectiveList (char* scenario);

#endif
