
#ifndef UPDATE
#define UPDATE

#include "Unit.h"

// ====================
// Update defines
// ====================

#define	ENEMY_DETECTED		0x01
#define ENEMY_IN_RANGE		0x02
#define FRIENDLY_DETECTED	0x04
#define FRIENDLY_IN_RANGE	0x08
#define ENEMY_SAME_HEX		0x10

#define ALL_DETECTION		0x1f

// 2001-03-22 TESTING BY S.G. EVEN IF THE ENEMY IS IN RANGE, DON'T REACT IF YOU CANNOT SEE IT!
//#define REACTION_MASK		0x13			// Required to react
#define REACTION_MASK		0x11			// Required to react

// ===================
// Unit Entity ADT
// ===================

class FlightClass;
typedef FlightClass*	Flight;

extern int UpdateUnit (Unit U, CampaignTime DeltaTime);

extern int DoWPAction (Flight u);

extern CampaignTime TimeToMove (Unit u, CampaignHeading h);

extern int EngageParent(Unit u, FalconEntity *e);

extern int Detected(Unit u, FalconEntity *e, float *range);

extern int DoCombat(CampEntity u, FalconEntity *e);

extern void UpdateLocation (GridIndex *x, GridIndex *y, Path path, int start, int end);

#endif
