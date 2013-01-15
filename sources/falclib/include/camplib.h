#ifndef CAMPLIB
#define CAMPLIB

#include "FalcLib.h"
#include "F4Vu.h"
#include "FalcList.h"

// =====================================
// Campaign defines and typedefs
// =====================================

typedef ulong CampaignTime;
#define CampaignSeconds   1000
#define CampaignMinutes  60000
#define CampaignHours  3600000
#define CampaignDay   86400000

#define INFINITE_TIME		    4294967295		// Max value of CampaignTime
#define VEHICLE_GROUPS_PER_UNIT 16
#define FEATURES_PER_OBJ        32
#define MAXIMUM_ROLES           16
#define MAXIMUM_OBJTYPES        32
#define MAXIMUM_WEAPTYPES       600
//#define MAXIMUM_WEAPTYPES       1200
#define MAX_UNIT_CHILDREN       5
#define MAX_FEAT_DEPEND         5

#define MAX_NUMBER_OF_OBJECTIVES		8000
#define MAX_NUMBER_OF_UNITS				4000	// Max # of NON volitile units only
#define MAX_NUMBER_OF_VOLATILE_UNITS	16000
#define MAX_CAMP_ENTITIES				(MAX_NUMBER_OF_OBJECTIVES+MAX_NUMBER_OF_UNITS+MAX_NUMBER_OF_VOLATILE_UNITS)

#define MONOMODE_OFF		0
#define MONOMODE_TEXT		1
#define MONOMODE_MAP		2

class UnitClass;
typedef UnitClass* Unit;

// =====================================
// Vu shortcuts
// =====================================

typedef FalconPrivateList*			F4PFList;
typedef FalconPrivateOrderedList*	F4POList;
typedef VuListIterator*				F4LIt;

// =====================================
// Campaign Library Functions
// =====================================

extern void Camp_Init (int processor);

extern void Camp_Exit (void);

extern void Camp_SetPlayerSquadron (Unit squadron);

extern Unit Camp_GetPlayerSquadron (void);

extern VuEntity* Camp_GetPlayerEntity (void);

extern CampaignTime Camp_GetCurrentTime (void);

extern void Camp_SetCurrentTime(double newTime);

extern void Camp_FreeMemory (void);
 
extern FILE* OpenCampFile (char *filename, char *ext, char *mode);

#endif
