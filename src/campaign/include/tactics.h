// tactics.h

#ifndef TACTICS_H
#define TACTICS_H

// Tactics defines (standard tactics)
#define ATACTIC_ENGAGE_AIR				1
#define ATACTIC_SHOOT_AND_RUN			2
#define ATACTIC_BEAM					3
#define ATACTIC_ENGAGE_STRIKE			4
#define ATACTIC_ENGAGE_SURFACE			5
#define ATACTIC_ENGAGE_NAVAL			6
#define ATACTIC_RETROGRADE				7
#define ATACTIC_ENGAGE_DEF				8
#define ATACTIC_IGNORE					9
#define ATACTIC_AVOID					10
#define ATACTIC_REFUEL					11
#define ATACTIC_ABORT					12

#define GTACTIC_DEFEND					13
#define GTACTIC_DELAY_HOLD				14
#define GTACTIC_DELAY_FALLBACK			15
#define GTACTIC_MOVE_AIRBORNE			16
#define GTACTIC_MOVE_MARINE				17
#define GTACTIC_MOVE_BRIGADE_COLUMN		18
#define GTACTIC_REPAIR					19
#define GTACTIC_SUPPORT_UNIT			20
#define GTACTIC_SUPPORT					21
#define GTACTIC_MOVE_COLUMN				22
#define GTACTIC_MOVE_OVERWATCH1			23
#define GTACTIC_MOVE_OVERWATCH2			24
#define GTACTIC_MOVE_WEDGE				25
#define GTACTIC_MOVE_ECHELON			26
#define GTACTIC_RESERVE					27
#define GTACTIC_MOVE_HOLD				28
#define GTACTIC_WAIT_TIL_READY			29
#define GTACTIC_WITHDRAW				30

#define GTACTIC_BRIG_SECURE				31
#define GTACTIC_BRIG_DEFEND				32
#define GTACTIC_BRIG_MOVE				33
#define GTACTIC_BRIG_WITHDRAW			34

#define GILMANS_BEAM_TACTIC

// The bit field definition
struct ReactData {
	uchar		reaction[3];				// reaction (or 0 for ignore)
	};

// =========================
// Tactics Structure
// =========================
	 
struct TacticData;
typedef TacticData *Tactic;

struct TacticData {
	short		id;
	char		name[30];				// Tactic name
	uchar		team;					// Teams which are allowed to use this tactic
	uchar		domainType;				// Domain of units which can use this tactic
	uchar		unitSize;				// Size of units which can use this tactic
	short		minRangeToDest;			// How far from our target we gotta be (no target, mission dest)
	short		maxRangeToDest;			// How far from out target we can be
	short		distToFront;			// Minimum distance to the front
	uchar		actionList[10];			// Actions/orders this tactic's ok with (0 in slot 0 = any)
	uchar		broken;					// Can we do this while broken?
	uchar		engaged;				// Can we do this while engaged?
	uchar		combat;					// Can we do this while in combat?
	uchar		losses;					// Can we do this while taking losses?
	uchar		retreating;				// Can we do this while retreating?
	uchar		owned;					// Is our assigned objective owned by us?
	uchar		airborne;				// Is our unit air-mobile?
	uchar		marine;					// Is our unit marine-capibile?
	uchar		minOdds;				// Minimum odds we're willing to do this with (minOdds:10)
	uchar		role;					// Any special role we need to do this.
	uchar		fuel;					// How much extra fuel we gotta have
	uchar		weapons;				// 0 = none req, 1 = req for target, 2 = req for target and mission obj
	uchar		priority;				// Relative ranking of this tactic
	uchar		formation;				// Formation to do this in
	uchar		special;				// Any private stuff we want to check
	};

// =========================
// Tactics Lists
// =========================

extern short	AirTactics;
extern short	GroundTactics;
extern short	NavalTactics;
extern short	FirstAirTactic;
extern short	FirstGroundTactic;
extern short	FirstNavalTactic;
extern short	TotalTactics;

extern TacticData *TacticsTable;

// =========================
// Functions
// =========================

int LoadTactics (char* filename);

void FreeTactics (void);

int CheckTeam (int tid, int team);

int CheckUnitType(int tid, int domain, int type);

int CheckRange(int tid, int rng);

int CheckDistToFront(int tid, int dist);

int CheckAction(int tid, int act);

int CheckStatus(int tid, int status);

int CheckLosses(int tid, int losses);

int CheckEngaged(int tid, int engaged);

int CheckCombat(int tid, int combat);

int CheckRetreating(int tid, int retreating);

int CheckOwned(int tid, int o);

int CheckAirborne(int tid, int airborne);		// These two need some thought
int CheckMarine(int tid, int marine);			//

int CheckOdds(int tid, int odds);

int CheckRole(int tid, int role);

int CheckSpecial(int tid);

int CheckFuel(int tid, int fuel);

int CheckWeapons(int tid);

int GetTacticPriority(int tid);

int GetTacticFormation(int tid);

#endif