#ifndef _GTMOBJ
#define _GTMOBJ

#include "team.h"

// ====================================
// Flags
// ====================================

#define GTMOBJ_PLAYER_SET_PRIORITY	0x01					// Player has modified priorities for this objective
#define GTMOBJ_SCRIPTED_PRIORITY	0x02					// Priority set by script

// ====================================
// Primary and secondary objective data
// ====================================

struct PrimaryObjectiveData {
		VU_ID				objective;						// Id of the objective
		short				ground_priority[NUM_TEAMS];		// It's calculated priority (per team)
		short				ground_assigned[NUM_TEAMS];		// Combat factors assigned (per team)
		short				air_priority[NUM_TEAMS];		// Air tasking manager's assessment of priority
		short				player_priority[NUM_TEAMS];		// Player adjusted priorities (or ATM's if no modifications)
		uchar				flags;
		};
typedef PrimaryObjectiveData* POData;

// ====================================
// GndObjDataType:
//
// Simple sorted list of objective data
// with scores and unit assignment data
// ====================================

#define GODN_SORT_BY_PRIORITY		1
#define GODN_SORT_BY_OPTIONS		2

#define USN_SORT_BY_SCORE			1
#define USN_SORT_BY_DISTANCE		2

class UnitScoreNode
	{
	public:
		Unit				unit;
		int					score;
		int					distance;
		UnitScoreNode		*next;

	public:
		UnitScoreNode (void);

		UnitScoreNode* Insert (UnitScoreNode* to_insert, int sort_by);
		UnitScoreNode* Remove (UnitScoreNode* to_remove);
		UnitScoreNode* Remove (Unit u);
		UnitScoreNode* Purge (void);
		UnitScoreNode* Sort (int sort_by);
	};
typedef UnitScoreNode* USNode;

class GndObjDataType
	{
	public:
		Objective			obj;
		int					priority_score;
		int					unit_options;
		UnitScoreNode		*unit_list;
		GndObjDataType		*next;

	public:
		GndObjDataType (void);
		~GndObjDataType (void);

		GndObjDataType* Insert (GndObjDataType* to_insert, int sort_by);
		GndObjDataType* Remove (GndObjDataType* to_remove);
		GndObjDataType* Remove (Objective o);
		GndObjDataType* Purge (void);
		GndObjDataType* Sort (int sort_by);

		void InsertUnit (Unit u, int s, int d);
		UnitScoreNode* RemoveUnit (Unit u);
		void RemoveUnitFromAll(Unit u);
		void PurgeUnits (void);

//		GndObjDataType* FindWorstOption (Unit u);
//		int FindNewOptions (Unit u);
	};
typedef GndObjDataType* GODNode;

// ==========================================
// Global functions
// ==========================================

extern void CleanupObjList (void);

extern void DisposeObjList (void);

extern POData GetPOData (Objective po);

extern void AddPODataEntry (Objective po);

extern void ResetObjectiveAssignmentScores (void);

extern int GetOptions (int score);

#endif
