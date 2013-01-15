#ifndef STRATEGY
#define STRATEGY

#include "Team.h"

// ==================================================
// Strategic State data structure
// ==================================================

   #define DEPEND_MAX            5
   #define SDATA_MAX             20

   struct  CampStateIDList {	
   						uchar       Data[SDATA_MAX];
  							};
   typedef CampStateIDList* IDList;

   struct CampDependState {
                     uchar       Function;            // 0:none, 1:<, 2:=, 3:>
                     uchar       Who;
                     uchar       StateType;
                     uchar       StateValue;
                     };

   // This structure will contain information for movies, briefing,
   // Reinforcements, etc..
   struct CampStateType {
                     uchar             Type:4;        // State type
                     uchar             Final:1;       // Is it a final state
                     uchar             Change:1;      // Should it update? Or be set?
                     uchar             Used:1;        // For one-time states
                     char              Name[12];
                     char              Short[3];
                     uchar             StateClass;
                     uchar             StateValue;
                     uchar             DaysHere;      // Time spent in state
                     // Dependancies
                     uchar             AMinRatio;     // value:10 = ratio
                     uchar             GMinRatio;
                     uchar             Difficulty;    // Difficulty level min
                     uchar             Units;
                     uchar             WarDay;
                     uchar             MaxDays;       // Maximum time to spend here
                     uchar             Chance;
                     CampDependState   Condition[DEPEND_MAX];
                     IDList            Owned;
                     IDList            PriData;
                     IDList            SecData;
                     CampStateType*		NextState;
                     };
   typedef CampStateType* CampaignState;

// ==================================================
// Defines for states specific to a theater
// ==================================================

   #define SUPPLY_STATE          0
   #define POLITICAL_STATE       1
   #define AIR_DEF_STATE         2
   #define AIR_OFF_STATE         3
	#define AIR_SUP_STATE			4
   #define GROUND_DEF_STATE      5
   #define GROUND_OFF_STATE      6
   #define YS_ACTIVITY_STATE     7
   #define KS_ACTIVITY_STATE     8
   #define SOJ_ACTIVITY_STATE    9
   #define COMMANDO_STATE        10
   #define SMISSILE_STATE        11
   #define CHEMICAL_STATE        12
   #define NUCLEAR_STATE         13
   #define ABORT_STATE           14
   #define NOFLY_STATE           15
   #define FLYALT_STATE          16
   #define FLYNIGHT_STATE        17
   #define STRIKE_STATE          18
   #define DEFLIGHT_STATE        19
   #define REINFORCEMENT_STATE   20
   #define REPAIR_RATE_STATE     21
   #define GRO_DEF_STATE         22
   #define GRO_OFF_STATE         23
   #define C_AND_C_STATE         24
   #define PLAYER_STATE          25
   #define O_STRATEGY_STATE      26                   // Offensive plans
   #define D_STRATEGY_STATE      27                   // Defensive plans 
   #define LAST_STATE            28

   #define MAX_CLASS             LAST_STATE

// Defines for SUPPLY_STATE:
   #define SUP_NONE              0
   #define SUP_POOR              1
   #define SUP_MOD               2
   #define SUP_GOOD              3

// Defines for POLITICAL_STATE:
   #define POL_ATPEACE           0
   #define POL_LIMITEDAIR        1
   #define POL_LIMITEDGROUND     2
   #define POL_FULL              3

// Defines for AIR_DEF_STATE:
   #define ADE_FULL              0
   #define ADE_ON_DEMAND         1
   #define ADE_ALERT_ONLY        2
   #define ADE_NONE              3

// Defines for AIR_OFF_STATE:
   #define AOF_AIR               0
   #define AOF_RADAR             1
   #define AOF_SAM               2
   #define AOF_INTERDICT         3
   #define AOF_GROUND            4

// Defines for AIR_SUP_STATE:
	#define ASU_HSUP_LOST			0
	#define ASU_HSUP_CONTESTED		1
	#define ASU_HSUP_FULL			2
	#define ASU_ESUP_CONTESTED		3
	#define ASU_ESUP_FULL			4

// Defines for GROUND_DEF_STATE:
   #define GRD_FULLRETREAT       0
   #define GRD_HOLDPRIMARY       1
   #define GRD_HOLDPRIORITY      2
   #define GRD_HOLDFULLLINE      3

// Defines for GROUND_OFF_STATE:
   #define GRO_NONE              0
   #define GRO_FRONTALATTACK     1
   #define GRO_POINTATTACK       2
   #define GRO_PURSUIT           3

// Defines for YS_ACTIVITY_STATE:
   #define YS_NONE               0
   #define YS_CONTESTED          1
   #define YS_SUPERIORITY        2
   #define YS_COMPLETE           3 

// Defines for KS_ACTIVITY_STATE:
   #define KS_NONE               0
   #define KS_CONTESTED          1
   #define KS_SUPERIORITY        2
   #define KS_COMPLETE           3 

// Defines for SOJ_ACTIVITY_STATE:
   #define SOJ_NONE              0
   #define SOJ_CONTESTED         1
   #define SOJ_SUPERIORITY       2
   #define SOJ_COMPLETE          3 

// Defines for COMMANDO_STATE:
   #define COM_NONE              0
   #define COM_INCOMING          1
   #define COM_ACTIVE            2

// Defines for SMISSILE_STATE:
   #define MIS_NONE              0
   #define MIS_LIGHT             1
   #define MIS_HEAVY             2

// Defines for CHEMICAL_STATE:
   #define CHE_NONE              0
   #define CHE_MISSILE           1
   #define CHE_BOMB              2

// Defines for NUCLEAR_STATE:
   #define NUC_NONE              0
   #define NUC_TACTICAL          1
   #define NUC_STRATEGIC         2

// Defines for ABORT_STATE:
   #define ABO_ANYLOSS           0
   #define ABO_STRIKELOSS        1
   #define ABO_NEVER             2

// Defines for FOFLY_STATE:
   #define NOF_NOMISSIONS        0
   #define NOF_NOECM             1
   #define NOF_NOSEAD            2
   #define NOF_NOESCORT          3
   #define NOF_FLYALWAYS         4

// Defines for STRIKE_STATE:
// Defines for DFLIGHT_STATE:

// Defines for REINFORCEMENT_STATE:
   #define REF_NONE              0
   #define REF_SOME              1
   #define REF_MOD               2
   #define REF_MANY              3

// Defines for REPAIR_RATE_STATE:
   #define REP_NONE              0
   #define REP_ONE               1
   #define REP_TWO               2
   #define REP_THREE             3

// Defines for GRO_DEF_STATE:
   #define DEF_NOCOMBAT          0
   #define DEF_WITHDRAW          1
   #define DEF_FIGHTRETREAT      2
   #define DEF_HOLD              3

// Defines for GRO_OFF_STATE:
   #define OFF_NOCOMBAT          0
   #define OFF_HOLD              1
   #define OFF_DEPLOYATTACK      2
   #define OFF_HASTYATTACK       3

// Defines for C_AND_C_STATE:
   #define CAC_NONE              0
   #define CAC_BAD               1
   #define CAC_MOD               2
   #define CAC_INTACT            3

// State Types:
   #define STA_INFO              0                 // Informational only
   #define STA_REINFORCEMENT     1                 // Contains reinforcement list
   #define STA_STRATEGY          2                 // Contains objective data

// Objective Priorities:
   #define SPRI_NONE             0
   #define SPRI_NEEDED           1
   #define SPRI_SECONDARY        2
   #define SPRI_PRIMARY          3

// =====================================================
// Externals and prototypes
// =====================================================

   extern CampaignState States[NUM_TEAMS][MAX_CLASS];
   extern CampaignState CurrState[NUM_TEAMS][MAX_CLASS];
   extern char StateNames[MAX_CLASS][12];
   extern char StateIDs[MAX_CLASS][3];

   extern CampaignState AddState(void);

   extern void DeleteState(CampaignState s);
	  
	extern void DeleteStates (void);

   extern void InitStates(void);

   extern int LoadStates(char *Filename);

   extern void SaveStates(char *Filename);

   extern IDList NewIDList(void);

   extern void DeleteIDList(IDList list);

   extern void UpdateStates(Control who);

   extern void SetPriority(Team who, int c, int ID);

   extern int PriStatus(Team who, int ID);

   extern void SetNeeded(Team who, int ID);

   extern CampaignState GetState(Team who, int state);

   extern int GetStateValue(Team who, int state);

   extern void SetOwnedID(CampaignState s, int ID);

   extern void SetPriID(CampaignState s, int ID);

   extern void SetSecID(CampaignState s, int ID);

   extern int GetStateIDPriority(CampaignState s, int oid);

   extern int GetStrategicPriority(int oid, Team who);

	extern int PriorityTarget(int oid);
	  
#endif

