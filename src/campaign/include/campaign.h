#ifndef CAMPAIGN
#define CAMPAIGN

#include "CmpGlobl.h"
#include "asearch.h"
#include "falcgame.h"
#include "Campweap.h"

//sfr:
//added this struct for a decent return type with size in it
typedef struct
{
    long dataSize;
    char *data;
} CampaignData;

// ============
// Map Deltas
// ============

extern GridIndex  dx[17];     // dx per direction
extern GridIndex dy[17];     // dy per direction

// ============
// Placeholders
// ============

class UnitClass;
class ObjectiveClass;
typedef UnitClass* Unit;
typedef ObjectiveClass* Objective;

// =====================
// Campaign wide globals
// =====================

struct F4CSECTIONHANDLE;
extern F4CSECTIONHANDLE* campCritical;
extern int VisualDetectionRange[OtherDam];
extern uchar DefaultDamageMods[OtherDam + 1];

// ================
// Defines bitand macros
// ================

#define InfiniteCost  32000

// sfr: start removing locks
#define NO_CAMP_LOCK 1

inline void CampEnterCriticalSection()
{
    F4EnterCriticalSection(campCritical);
}
inline void CampLeaveCriticalSection()
{
    F4LeaveCriticalSection(campCritical);
}

// ======================
// external functions
// ======================

extern Objective AddObjectiveToCampaign(GridIndex I, GridIndex J);

extern void RemoveObjective(Objective O);

extern int LoadTheater(char *filename);

extern int SaveTheater(char *filename);

extern int LinkCampaignObjectives(/*BasePathClass **/ Path p, Objective O1, Objective O2);

extern int UnLinkCampaignObjectives(Objective O1, Objective O2);

extern int RecalculateLinks(Objective o);

extern Unit GetUnitByXY(GridIndex I, GridIndex J);

extern Unit AddUnit(GridIndex I, GridIndex J, char Side);

extern Unit CreateUnit(Control who, int Domain, UnitType Type, uchar SType, uchar SPType, Unit Parent);

extern void RemoveUnit(Unit u);

extern int TimeOfDayGeneral(CampaignTime time);

extern int TimeOfDayGeneral(void);

extern CampaignTime TimeOfDay(void);

extern int CreateCampFile(char *filename, char* path);

extern FILE* OpenCampFile(char *filename, char *ext, char *mode);
extern void CloseCampFile(FILE *);

extern void StartReadCampFile(FalconGameType type, char *filename);
//sfr: changed return type
//extern char *ReadCampFile (char *filename, char *ext);
extern CampaignData ReadCampFile(char *filename, char *ext);
extern void EndReadCampFile(void);

extern void StartWriteCampFile(FalconGameType type, char *filename);
extern void WriteCampFile(char *filename, char *ext, char *data, int size);
extern void EndWriteCampFile(void);

// Bubble rebuilding stuff
extern void CampaignRequestSleep(void);
extern int CampaignAllAsleep(void);

#undef Unit
#undef Objective

#endif
