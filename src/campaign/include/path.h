#ifndef PATH_H
#define PATH_H

#include "ASearch.h"
#include "Objectiv.h"
#include "Unit.h"

// =============================
// Globals used by other modules
// =============================

extern float CostTable[COVER_TYPES][MOVEMENT_TYPES];
extern float ReliefCost[RELIEF_TYPES];
extern int QuickSearch;	
extern int moveAlt;

// =============================
// Path flags
// =============================
	
#define PATH_ROADOK		0x01			// Use road movement bonuses (private implimentation)
#define PATH_RAILOK		0x02			// Use rail movement bonuses (private implimentation)
#define PATH_ENEMYOK	0x04			// Ok to move through enemy territory (private implimentation)
#define PATH_ENEMYCOST	0x08			// Additional cost for moving through enemy territory
#define PATH_AIRBORNE	0x10			// Use best of lowair/foot movement
#define PATH_MARINE		0x20			// Use best of naval/tracked movement
#define PATH_BASIC		0x40			// Ignore any enemy/threat movement costs
#define PATH_ENGINEER	0x80			// Allows engineers to move into tiles with broken bridges

#define MAX_COST		50				// The highest cost we're willing to pay for any given move

// =============================
// Global functions
// =============================
	
extern int GetGridPath (Path p, GridIndex x, GridIndex y, GridIndex xx, GridIndex yy, int type, int who, int flags);

extern costtype GetPathCost (GridIndex x, GridIndex y, Path path, MoveType mt, int flags);

extern costtype GetPathCost (Objective o, Path path, MoveType mt, int flags);

extern int GetObjectivePath (Path p, Objective o, Objective t, int type, int who, int flags);

extern int GetObjectivePath (Path p, GridIndex x, GridIndex y, GridIndex xx, GridIndex yy, int type, int who, int flags);

extern int FindLinkPath (Path p, Objective O1, Objective O2, MoveType mt);

extern float GetMovementCost (GridIndex x, GridIndex y, MoveType move, int flags, CampaignHeading h);

extern float GetObjectiveMovementCost (Objective o, int neighbor, MoveType type, Team team, int flags);

#endif

