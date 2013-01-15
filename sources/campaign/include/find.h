#ifndef FIND
#define FIND

#include "Unit.h"
#include "Objectiv.h"
#include "F4Vu.h"

// =============================
// Path finding flags
// =============================

#define FIND_FINDFRIENDLY		0x01					// Find friendly stuff (default is enemy)
#define FIND_FINDCOUNT			0x02
#define FIND_NOAIR				0x04					// Ignore air units
#define FIND_NODETECT			0x08					// Ignore detection only units
#define FIND_NOMOVERS			0x10					// Ignore moving units
#define FIND_CAUTIOUS			0x20					// Assume slightly larger enemy ranges
#define FIND_AD_ONLY			0x40					// Find only dedicated AD assets
#define FIND_FINDUNSPOTTED		0x80					// Returns somewhat lesser score for unspotted stuff
#define	FIND_THISOBJONLY		0x100					// Find children within this secondary objective only
#define FIND_SECONDARYONLY		0x200					// Find secondary objectives only
#define FIND_STANDARDONLY		0x400					// Find only standard (non secondary) objectives

#define ALT_LEVELS 5

extern int MaxAltAtLevel[ALT_LEVELS];
extern int MinAltAtLevel[ALT_LEVELS];

// ==========================================
// Globals
// ==========================================

// extern uchar ThreatSearch[MAX_CAMP_ENTITIES];			// Search data

// =============================
// Global function headers
// =============================

extern int DistSqu (GridIndex ox, GridIndex oy, GridIndex dx, GridIndex dy);

extern float Distance (GridIndex ox, GridIndex oy, GridIndex dx, GridIndex dy);

extern float Distance (float ox, float oy, float dx, float dy);

extern float DistSqu (float ox, float oy, float dx, float dy);

extern float DistanceToFront (GridIndex x, GridIndex y);

extern float DirectionToFront (GridIndex x, GridIndex y);

extern float DirectionTowardFriendly (GridIndex x, GridIndex y, int team);

extern int GetBearingDeg (float x, float y, float tx, float ty);

extern int GetRangeFt (float x, float y, float tx, float ty);

extern void* PackXY (GridIndex x, GridIndex y);

extern void UnpackXY (void* n, GridIndex* x, GridIndex* y);

extern CampaignTime TimeToArrive (float distance, float speed);

extern void Trim (GridIndex *x, GridIndex *y);

extern float AngleTo (GridIndex ox, GridIndex oy, GridIndex tx, GridIndex ty);

extern CampaignHeading DirectionTo (GridIndex ox, GridIndex oy, GridIndex tx, GridIndex ty);

extern CampaignHeading DirectionTo (GridIndex ox, GridIndex oy, GridIndex tx, GridIndex ty,
									  GridIndex cx, GridIndex cy);

extern int OctantTo (GridIndex ox, GridIndex oy, GridIndex tx, GridIndex ty);

extern int OctantTo (float ox, float oy, float tx, float ty);

extern CampaignTime TimeBetween (GridIndex x, GridIndex y, GridIndex tx, GridIndex ty, int speed);

extern CampaignTime TimeBetweenO (Objective o1, Objective o2, int speed);

extern Objective FindObjective (VU_ID id);

extern Unit FindUnit (VU_ID id);

extern CampEntity FindEntity (VU_ID id);

extern CampEntity GetEntityByCampID (int id);

extern Objective FindNearestSupplySource(Objective o);

extern Unit FindNearestEnemyUnit (GridIndex X, GridIndex Y, GridIndex max);

extern Unit FindNearestRealUnit (GridIndex X, GridIndex Y, float *last, GridIndex max);

extern Unit FindNearestUnit (VuFilteredList* l, GridIndex X, GridIndex Y, float *last);

extern Unit FindNearestUnit (GridIndex X, GridIndex Y, float *last);

extern Unit FindUnitByXY (VuFilteredList* l, GridIndex X, GridIndex Y, int domain);

extern Unit GetUnitByXY (GridIndex X, GridIndex Y, int domain);

extern Unit GetUnitByXY (GridIndex X, GridIndex Y);

extern Objective FindNearestObjective (GridIndex X, GridIndex Y, float *last, GridIndex maxdist);

extern Objective FindNearestObjective (VuFilteredList* l, GridIndex X, GridIndex Y, float *last);

extern Objective FindNearestObjective (GridIndex X, GridIndex Y, float *last);

extern Objective FindNearestAirbase (GridIndex X, GridIndex Y);

extern Objective FindNearbyAirbase (GridIndex X, GridIndex Y);

extern Objective FindNearestFriendlyAirbase (Team who, GridIndex X, GridIndex Y);

extern Objective FindNearestFriendlyRunway (Team who, GridIndex X, GridIndex Y);

extern Objective FindNearestFriendlyObjective(VuFilteredList* l, Team who, GridIndex *x, GridIndex *y, int flags);

extern Objective FindNearestFriendlyObjective(Team who, GridIndex *x, GridIndex *y, int flags);

extern Objective FindNearestFriendlyPowerStation(VuFilteredList* l, Team who, GridIndex x, GridIndex y);

extern Objective GetObjectiveByXY (GridIndex X, GridIndex Y);

extern int AnalyseThreats (GridIndex X, GridIndex Y, MoveType mt, int alt, int roe_check, Team who, int flags);

extern int CollectThreats (GridIndex X, GridIndex Y, int Z, Team who, int flags, F4PFList foundlist);

extern int CollectThreatsFast (GridIndex X, GridIndex Y, int altlevel, Team who, int flags, F4PFList foundlist);

extern int ScoreThreat (GridIndex X, GridIndex Y, int Z, Team who, int flags);

extern int ScoreThreatFast(GridIndex X, GridIndex Y, int altlevel, Team who);

extern float GridToSim (GridIndex x);

extern GridIndex SimToGrid (float x);

extern void ConvertGridToSim(GridIndex x, GridIndex y, vector *pos);

extern void ConvertSimToGrid(vector *pos, GridIndex *x, GridIndex *y);

extern void ConvertSimToGrid (Tpoint *pos, GridIndex *x, GridIndex *y);

extern MoveType AltToMoveType (int alt);

extern int GetAltitudeLevel (int alt);

extern int GetAltitudeFromLevel (int level, int seed);

extern CampaignTime TimeTo (GridIndex x, GridIndex y, GridIndex tx, GridIndex ty, int speed);

extern F4PFList GetDistanceList (Team who, int  i, int j);

extern void FillDistanceList (List list, Team who, int  i, int j);

extern FalconSessionEntity* FindPlayer (Flight flight, uchar planeNum);

extern FalconSessionEntity* FindPlayer (VU_ID flightID, uchar planeNum);

extern FalconSessionEntity* FindPlayer (Flight flight, uchar planeNum, uchar pilotSlot);

extern FalconSessionEntity* FindPlayer (VU_ID flightID, uchar planeNum, uchar pilotSlot);

#endif // FIND
