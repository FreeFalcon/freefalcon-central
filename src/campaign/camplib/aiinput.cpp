#include <windows.h>
#include "FalcLib.h"
#include "F4Find.h"
#include "ClassTbl.h"
#include "CampLib.h"

// ATM Inputs
short IMMEDIATE_MIN_TIME;
short IMMEDIATE_MAX_TIME;
short LONGRANGE_MIN_TIME;
short LONGRANGE_MAX_TIME;
short ATM_ASSIGN_RATIO;
short MAX_FLYMISSION_THREAT;
short MAX_FLYMISSION_HIGHTHREAT;
short MAX_FLYMISSION_NOTHREAT;
short MIN_SEADESCORT_THREAT;
short MIN_AVOID_THREAT;
short MIN_AP_DISTANCE;
short BREAKPOINT_DISTANCE;
short ATM_TARGET_CLEAR_RADIUS;
short LOITER_DIST;
short SWEEP_DISTANCE;
short MINIMUM_BDA_PRIORITY;
short MINIMUM_AWACS_DISTANCE;
short MAXIMUM_AWACS_DISTANCE;
short MINIMUM_JSTAR_DISTANCE;
short MAXIMUM_JSTAR_DISTANCE;
short MINIMUM_TANKER_DISTANCE;
short MAXIMUM_TANKER_DISTANCE;
short MINIMUM_ECM_DISTANCE;
short MAXIMUM_ECM_DISTANCE;
short FIRST_COLONEL;					// Indexes into our pilot name array
short FIRST_COMMANDER;
short FIRST_WINGMAN;
short LAST_WINGMAN;
short MAX_AA_STR_FOR_ESCORT;
short BARCAP_REQUEST_INTERVAL;			// Minimum time between BARCAP mission requests
short ONCALL_CAS_FLIGHTS_PER_REQUEST;	// How many CAS flights to plan per ONCALL request
short MIN_TASK_AIR;						// Retask time, in minutes
short MIN_PLAN_AIR;						// Replan time (KCK NOTE: PLAN_AIR max is 8)
short VICTORY_CHECK_TIME;				// Check the victory conditions in a Tactical Engagement
short FLIGHT_MOVE_CHECK_INTERVAL;		// How often to check a flight for movement (in seconds)
short FLIGHT_COMBAT_CHECK_INTERVAL;		// How often to check a flight for combat (in seconds)
short AIR_UPDATE_CHECK_INTERVAL;		// How often to check squadrons/packages for movement/update (in seconds)
short FLIGHT_COMBAT_RATE;				// Amount of time between weapon shots (in seconds)
short PACKAGE_CYCLES_TO_WAIT;			// How many planning cycles to wait for tankers/barcap/etc
short AIR_PATH_MAX;						// Max amount of nodes to search for air paths
float MIN_IGNORE_RANGE;					// Minimum range (km) at which we'll ignore an air target
short MAX_SAR_DIST;						// Max distance from front to fly sar choppers into. (km)
short MAX_BAI_DIST;						// Max distance from front to fly BAI missions into. (km)
long PILOT_ASSIGN_TIME;					// Time before takeoff to assign pilots
short AIRCRAFT_TURNAROUND_TIME_MINUTES;	// Time to wait between missions before an aircraft is available again

// GTM Inputs
short PRIMARY_OBJ_PRIORITY;				// Priorities for primary and secondary objectives
short SECONDARY_OBJ_PRIORITY;			// Only cities and towns can be POs and SOs
short MAX_OFFENSES;						// Maximum number of offensive POs
short MIN_OFFENSIVE_UNITS;				// Minimum number of units to constitute an offensive
short MINIMUM_ADJUSTED_LEVEL;			// Minimum % strength of unit assigned to offense.
short MINIMUM_VIABLE_LEVEL;				// Minimum % strength in order to be considered a viable unit
short MAX_ASSIGNMENT_DIST;				// Maximum distance from an object we'll assign a unit to
short MAXLINKS_FROM_SO_OFFENSIVE;		// Maximum objective links from our secondary objective we'll place units
short MAXLINKS_FROM_SO_DEFENSIVE;		// Same as above for defensive units
short BRIGADE_BREAK_BASE;				// Base moral a brigade breaks at (modified by objective priority)
short ROLE_SCORE_MODIFIER;				// Used to adjust role score for scoring purposes
short MIN_TASK_GROUND;					// Retask time, in minutes
short MIN_PLAN_GROUND;					// Replan time, in minutes
short MIN_REPAIR_OBJECTIVES;			// How often to repair objectives (in minutes);
short MIN_RESUPPLY;						// How often to resupply troops (in minutes)
short MORALE_REGAIN_RATE;				// How much morale per hour a typical unit will regain
short REGAIN_RATE_MULTIPLIER_FOR_TE;	// Morale/Fatigue regain rate multiplier for TE missions
short FOOT_MOVE_CHECK_INTERVAL;			// How often to check a foot battalion for movement (in seconds)
short TRACKED_MOVE_CHECK_INTERVAL;		// How often to check a tracked/wheeled battalion for combat (in seconds)
short GROUND_COMBAT_CHECK_INTERVAL;		// How often to check ground battalions for combat
short GROUND_UPDATE_CHECK_INTERVAL;		// How often to check brigades for update (in seconds)
short GROUND_COMBAT_RATE;				// Amount of time between weapon shots (in seconds)
short GROUND_PATH_MAX;					// Max amount of nodes to search for ground paths
short OBJ_GROUND_PATH_MAX_SEARCH;		// Max amount of nodes to search for objective paths
short OBJ_GROUND_PATH_MAX_COST;			// Max amount of nodes to search for objective paths
short MIN_FULL_OFFENSIVE_INITIATIVE;	// Minimum initiative required to launch a full offensive
short MIN_COUNTER_ATTACK_INITIATIVE;	// Minimum initiative required to launch a counter-attack
short MIN_SECURE_INITIATIVE;			// Minimum initiative required to secure defensive objectives
short MINIMUM_EXP_TO_FIRE_PREGUIDE;		// Minimum experience needed to fire SAMs before going to guide mode
long OFFENSIVE_REINFORCEMENT_TIME;		// How often we get reinforcements when on the offensive
long DEFENSIVE_REINFORCEMENT_TIME;		// How often we get reinforcements when on the defensive
long CONSOLIDATE_REINFORCEMENT_TIME;	// How often we get reinforcements when consolidating
long ACTION_PREP_TIME;					// How far in advance to plan any offensives

// NTM Inputs
short MIN_TASK_NAVAL;					// Retask time, in minutes	
short MIN_PLAN_NAVAL;					// Replan time, in minutes
short NAVAL_MOVE_CHECK_INTERVAL;		// How often to check a task force for movement (in seconds)
short NAVAL_COMBAT_CHECK_INTERVAL;		// How often to fire naval units (in seconds)
short NAVAL_COMBAT_RATE;				// Amount of time between weapon shots (in seconds)

// Other variables
short LOW_ALTITUDE_CUTOFF;
short MAX_GROUND_SEARCH;	          	// How far to look for surface units or objectives
short MAX_AIR_SEARCH;		           	// How far to look for air threats
short NEW_CLOUD_CHANCE;
short DEL_CLOUD_CHANCE;
short PLAYER_BUBBLE_MOVERS;				// The # of objects we try to keep in player bubble
short FALCON_PLAYER_TEAM;				// What team the player starts on
short CAMP_RESYNC_TIME;					// How often to resync campaigns (in seconds)
unsigned short BUBBLE_REBUILD_TIME;				// How often to rebuild the player bubble (in seconds)
short STANDARD_EVENT_LENGTH;			// Length of stored event queues
short PRIORITY_EVENT_LENGTH;
short MIN_RECALCULATE_STATISTICS;		// How often to do the unit statistics stuff
float LOWAIR_RANGE_MODIFIER;			// % of Air range to get LowAir range from
short MINIMUM_STRENGTH;					// Minimum strength points we'll bother appling with colateral damage
short MAX_DAMAGE_TRIES;					// Max # of times we'll attempt to find a random target vehicle/feature
float REAGREGATION_RATIO;				// Ratio over 1.0 which things will reaggregate at
short MIN_REINFORCE_TIME;				// Value, in minutes, of each reinforcement cycle
float SIM_BUBBLE_SIZE;					// Radius (in feet) of the Sim's campaign lists.
short INITIATIVE_LEAK_PER_HOUR;			// How much initiative is adjusted automatically per hour

int F4_GENERIC_US_TRUCK_TYPE_SMALL;		// Small US tractor vehicle
int F4_GENERIC_US_TRUCK_TYPE_LARGE;		// Large US tractor vehicle   
int F4_GENERIC_US_TRUCK_TYPE_TRAILER;	// Tractor US for trailers
int F4_GENERIC_OPFOR_TRUCK_TYPE_SMALL;	// Small OPFOR tractor vehicle
int F4_GENERIC_OPFOR_TRUCK_TYPE_LARGE;	// Large OPFOR tractor vehicle 
int F4_GENERIC_OPFOR_TRUCK_TYPE_TRAILER;// Tractor OPFOR for trailers
int SWITCH_TRACTORS_IN_TE;				// Because in standard Korea we have switched teams in TE

// A.S. Campaign variables -> MPS original values
int StartOffBonusRepl;					// Replacement bonus when a team goes offensive
int StartOffBonusSup;					// Supply Bonus when a team goes offensive
int StartOffBonusFuel;					// Fuel Bonus when a team goes offensive
int ActionRate;							// How often we can start a new action (in hours)
int ActionTimeOut;						// Maximum time an offensive action can last
float DataRateModRepl;					// Modification factor for production data rate for replacements
float DataRateModSup;					// Modification factor for production data rate for fuel and supply
float RelSquadBonus;					// Relative replacements of squadrons to ground units
// in fact bools
int NoActionBonusProd;					// No action bonus for production
int NoTypeBonusRepl;					// No type bonus for replacements, new supply system, bugfix in "supplies units" function
int NewInitiativePoints;				// New initative points setting;
int CampBugFixes;						// Smaller bugfixes
float HitChanceAir;						// 2D hitchance air target modulation
float HitChanceGround;					// 2D hitchance ground target modulation
float MIN_DEAD_PCT;						// Strength below which a reaggregating vehicle will be considered dead
float FLOTDrawDistance;					// Distance between objectives at which we won't draw a line between them (in km^2)
int FLOTSortDirection;					// sort FLOT list from north to south (1) or east to west (0)
int TheaterXPosition;					// central X position for theater - used for finding best bullseye position
int TheaterYPosition;					// central Y position for theater - used for finding best bullseye position
// A.S.

// Reader Function
void ReadCampAIInputs (char * name)
{
	char	fileName[256],tmpName[80];
	int		off,len;
	short	temp;

	sprintf(tmpName,"%s.AII",name);
	if (!F4FindFile(tmpName,fileName,256,&off,&len))
	   exit(0);

	/* ATM Inputs */
	IMMEDIATE_MIN_TIME = (short)GetPrivateProfileInt("ATM","ImmediatePlanMinTime", 0, fileName);
	IMMEDIATE_MAX_TIME = (short)GetPrivateProfileInt("ATM","ImmediatePlanMaxTime", 0, fileName);
	LONGRANGE_MIN_TIME = (short)GetPrivateProfileInt("ATM","LongrangePlanMinTime", 0, fileName);
	LONGRANGE_MAX_TIME = (short)GetPrivateProfileInt("ATM","LongrangePlanMaxTime", 0, fileName);
	ATM_ASSIGN_RATIO = (short)GetPrivateProfileInt("ATM","AircraftAssignmentRatio", 0, fileName);
	MAX_FLYMISSION_THREAT = (short)GetPrivateProfileInt("ATM","MaxFlymissionThreat", 0, fileName);
	MAX_FLYMISSION_HIGHTHREAT = (short)GetPrivateProfileInt("ATM","MaxFlymissionHighThreat", 0, fileName);
	MAX_FLYMISSION_NOTHREAT = (short)GetPrivateProfileInt("ATM","MaxFlymissionNoThreat", 0, fileName);
	MIN_SEADESCORT_THREAT = (short)GetPrivateProfileInt("ATM","MinSeadescortThreat", 0, fileName);
	MIN_AVOID_THREAT = (short)GetPrivateProfileInt("ATM","MinAvoidThreat", 0, fileName);
	MIN_AP_DISTANCE = (short)GetPrivateProfileInt("ATM","MinAssemblyPtDist", 0, fileName);
	BREAKPOINT_DISTANCE = (short)GetPrivateProfileInt("ATM","BreakpointDist", 0, fileName);
	ATM_TARGET_CLEAR_RADIUS = (short)GetPrivateProfileInt("ATM","TargetClearRadius", 0, fileName);
	LOITER_DIST = (short)GetPrivateProfileInt("ATM","LoiterTurnDistance", 0, fileName);
	SWEEP_DISTANCE = (short)GetPrivateProfileInt("ATM","SweepRadius", 0, fileName);
	MINIMUM_BDA_PRIORITY = (short)GetPrivateProfileInt("ATM","MinimumBDAPriority", 0, fileName);
	MINIMUM_AWACS_DISTANCE = (short)GetPrivateProfileInt("ATM","MinimumAWACSDistance", 0, fileName);
	MAXIMUM_AWACS_DISTANCE = (short)GetPrivateProfileInt("ATM","MaximumAWACSDistance", 0, fileName);
	MINIMUM_JSTAR_DISTANCE = (short)GetPrivateProfileInt("ATM","MinimumJSTARDistance", 0, fileName);
	MAXIMUM_JSTAR_DISTANCE = (short)GetPrivateProfileInt("ATM","MaximumJSTARDistance", 0, fileName);
	MINIMUM_TANKER_DISTANCE = (short)GetPrivateProfileInt("ATM","MinimumTankerDistance", 0, fileName);
	MAXIMUM_TANKER_DISTANCE = (short)GetPrivateProfileInt("ATM","MaximumTankerDistance", 0, fileName);
	MINIMUM_ECM_DISTANCE = (short)GetPrivateProfileInt("ATM","MinimumECMDistance", 0, fileName);
	MAXIMUM_ECM_DISTANCE = (short)GetPrivateProfileInt("ATM","MaximumECMDistance", 0, fileName);
	FIRST_COLONEL = (short)GetPrivateProfileInt("ATM","FirstColonel", 0, fileName);
	FIRST_COMMANDER = (short)GetPrivateProfileInt("ATM","FirstCommander", 0, fileName);
	FIRST_WINGMAN = (short)GetPrivateProfileInt("ATM","FirstWingman", 0, fileName);
	LAST_WINGMAN = (short)GetPrivateProfileInt("ATM","LastWingman", 0, fileName);
	MAX_AA_STR_FOR_ESCORT = (short)GetPrivateProfileInt("ATM","MaxEscortAAStrength", 0, fileName);
	BARCAP_REQUEST_INTERVAL = (short)GetPrivateProfileInt("ATM","BARCAPRequestInterval", 0, fileName);
	ONCALL_CAS_FLIGHTS_PER_REQUEST = (short)GetPrivateProfileInt("ATM","OnCallFlightsPerRequest", 0, fileName);
	MIN_TASK_AIR = (short)GetPrivateProfileInt("ATM","AirTaskTime", 0, fileName);
	MIN_PLAN_AIR = (short)GetPrivateProfileInt("ATM","AirPlanTime", 0, fileName);
	VICTORY_CHECK_TIME = (short)GetPrivateProfileInt("ATM","VictoryConditionTime", 0, fileName);
	FLIGHT_MOVE_CHECK_INTERVAL = (short)GetPrivateProfileInt("ATM","FlightMoveCheckInterval", 0, fileName);
	FLIGHT_COMBAT_CHECK_INTERVAL = (short)GetPrivateProfileInt("ATM","FlightCombatCheckInterval", 0, fileName);
	AIR_UPDATE_CHECK_INTERVAL = (short)GetPrivateProfileInt("ATM","AirUpdateCheckInterval", 0, fileName);
	FLIGHT_COMBAT_RATE = (short)GetPrivateProfileInt("ATM","FlightCombatRate", 0, fileName);
	PACKAGE_CYCLES_TO_WAIT = (short)GetPrivateProfileInt("ATM","PackageCyclesToWait", 0, fileName);
	AIR_PATH_MAX = (short)GetPrivateProfileInt("ATM","AirPathMax", 0, fileName);
	MIN_IGNORE_RANGE = (float)GetPrivateProfileInt("ATM","MinimumIgnoreRange", 0, fileName);
	MAX_SAR_DIST = (short)GetPrivateProfileInt("ATM","MaxSARDistance", 0, fileName);
	MAX_BAI_DIST = (short)GetPrivateProfileInt("ATM","MaxBAIDistance", 0, fileName);
	PILOT_ASSIGN_TIME = (long)GetPrivateProfileInt("ATM","PilotAssignTime", 0, fileName);
	AIRCRAFT_TURNAROUND_TIME_MINUTES = (short)GetPrivateProfileInt("ATM","AircraftTurnaroundMinutes", 0, fileName);

	/* GTM Inputs */
	PRIMARY_OBJ_PRIORITY = (short)GetPrivateProfileInt("GTM","PrimaryObjPriority", 0, fileName);
	SECONDARY_OBJ_PRIORITY = (short)GetPrivateProfileInt("GTM","SecondaryObjPriority", 0, fileName);
	MAX_OFFENSES = (short)GetPrivateProfileInt("GTM","MaximumOffenses", 0, fileName);
	MIN_OFFENSIVE_UNITS = (short)GetPrivateProfileInt("GTM","MinimumOffensiveUnits", 0, fileName);
	MINIMUM_ADJUSTED_LEVEL = (short)GetPrivateProfileInt("GTM","MinimumAdjustedLevel", 0, fileName);
	MINIMUM_VIABLE_LEVEL = (short)GetPrivateProfileInt("GTM","MinimumViableLevel", 0, fileName);
	MAX_ASSIGNMENT_DIST = (short)GetPrivateProfileInt("GTM","MaximumAssignmentDist", 0, fileName);
	MAXLINKS_FROM_SO_OFFENSIVE = (short)GetPrivateProfileInt("GTM","MaximumOffensiveLinks", 0, fileName);
	MAXLINKS_FROM_SO_DEFENSIVE = (short)GetPrivateProfileInt("GTM","MaximumDefensiveLinks", 0, fileName);
	BRIGADE_BREAK_BASE = (short)GetPrivateProfileInt("GTM","BrigadeBreakBase", 0, fileName);
	ROLE_SCORE_MODIFIER = (short)GetPrivateProfileInt("GTM","RoleScoreModifier", 0, fileName);
	MIN_TASK_GROUND = (short)GetPrivateProfileInt("GTM","TaskGroundTime", 0, fileName);
	MIN_PLAN_GROUND = (short)GetPrivateProfileInt("GTM","PlanGroundTime", 0, fileName);
	MIN_REPAIR_OBJECTIVES = (short)GetPrivateProfileInt("GTM","ObjectiveRepairInterval", 0, fileName);
	MIN_RESUPPLY = (short)GetPrivateProfileInt("GTM","ResupplyInterval", 0, fileName);
	MORALE_REGAIN_RATE = (short)GetPrivateProfileInt("GTM","MoraleRegainRate", 0, fileName);
	REGAIN_RATE_MULTIPLIER_FOR_TE = (short)GetPrivateProfileInt("GTM","TERegainRateMultiplier", 0, fileName);
	FOOT_MOVE_CHECK_INTERVAL = (short)GetPrivateProfileInt("GTM","FootMoveCheckInterval", 0, fileName);
	TRACKED_MOVE_CHECK_INTERVAL = (short)GetPrivateProfileInt("GTM","TrackedMoveCheckInterval", 0, fileName);
	GROUND_COMBAT_CHECK_INTERVAL = (short)GetPrivateProfileInt("GTM","GroundCombatCheckInterval", 0, fileName);
	GROUND_UPDATE_CHECK_INTERVAL = (short)GetPrivateProfileInt("GTM","GroundUpdateCheckInterval", 0, fileName);
	GROUND_COMBAT_RATE = (short)GetPrivateProfileInt("GTM","GroundCombatRate", 0, fileName);
	GROUND_PATH_MAX = (short)GetPrivateProfileInt("GTM","GroundPathMax", 0, fileName);
	OBJ_GROUND_PATH_MAX_SEARCH = (short)GetPrivateProfileInt("GTM","ObjGroundPathMaxSearch", 0, fileName);
	OBJ_GROUND_PATH_MAX_COST = (short)GetPrivateProfileInt("GTM","ObjGroundPathMaxCost", 0, fileName);
	MIN_FULL_OFFENSIVE_INITIATIVE = (short)GetPrivateProfileInt("GTM","MinFullOffensiveInitiative", 0, fileName);
	MIN_COUNTER_ATTACK_INITIATIVE = (short)GetPrivateProfileInt("GTM","MinCounterAttackInitiative", 0, fileName);
	MIN_SECURE_INITIATIVE = (short)GetPrivateProfileInt("GTM","MinSecureInitiative", 0, fileName);
	MINIMUM_EXP_TO_FIRE_PREGUIDE = (short)GetPrivateProfileInt("GTM","MinExpToFirePreGuide", 0, fileName);
	OFFENSIVE_REINFORCEMENT_TIME = (long)GetPrivateProfileInt("GTM","OffensiveReinforcementTime", 0, fileName) * CampaignMinutes;
	DEFENSIVE_REINFORCEMENT_TIME = (long)GetPrivateProfileInt("GTM","DefensiveReinforcementTime", 0, fileName) * CampaignMinutes;
	CONSOLIDATE_REINFORCEMENT_TIME = (long)GetPrivateProfileInt("GTM","ConsolidateReinforcementTime", 0, fileName) * CampaignMinutes;
	ACTION_PREP_TIME = (long)GetPrivateProfileInt("GTM","ActionPrepTime", 0, fileName) * CampaignMinutes;

	/* NTM Inputs */
	MIN_TASK_NAVAL = (short)GetPrivateProfileInt("NTM","TaskNavalTime", 0, fileName);	
	MIN_PLAN_NAVAL = (short)GetPrivateProfileInt("NTM","PlanNavalTime", 0, fileName);
	NAVAL_COMBAT_CHECK_INTERVAL = (short)GetPrivateProfileInt("NTM","NavalCombatCheckInterval", 0, fileName);
	NAVAL_MOVE_CHECK_INTERVAL = (short)GetPrivateProfileInt("NTM","NavalMoveCheckInterval", 0, fileName);
	NAVAL_COMBAT_RATE = (short)GetPrivateProfileInt("NTM","NavalCombatRate", 0, fileName);

	/* Other */
	LOW_ALTITUDE_CUTOFF = (short)GetPrivateProfileInt("Other","LowAltitudeCutoff", 0, fileName);
	MAX_GROUND_SEARCH = (short)GetPrivateProfileInt("Other","GroundSearchDistance", 0, fileName);
	MAX_AIR_SEARCH = (short)GetPrivateProfileInt("Other","AirSearchDistance", 0, fileName);
	NEW_CLOUD_CHANCE = (short)GetPrivateProfileInt("Other","NewCloudPercentage", 0, fileName);
	DEL_CLOUD_CHANCE = (short)GetPrivateProfileInt("Other","DeleteCloudPercentage", 0, fileName);
	PLAYER_BUBBLE_MOVERS = (short)GetPrivateProfileInt("Other","PlayerBubbleObjects", 0, fileName);
	FALCON_PLAYER_TEAM = (short)GetPrivateProfileInt("Other","PlayerTeam", 0, fileName);
	CAMP_RESYNC_TIME = (short)GetPrivateProfileInt("Other","CampaignResyncTime", 0, fileName);
	BUBBLE_REBUILD_TIME = (unsigned short)GetPrivateProfileInt("Other","BubbleRebuildTime", 0, fileName);
	STANDARD_EVENT_LENGTH = (short)GetPrivateProfileInt("Other","StandardEventqueueLength", 0, fileName);				// Length of stored event queues
	PRIORITY_EVENT_LENGTH = (short)GetPrivateProfileInt("Other","PriorityEventqueueLength", 0, fileName);
	MIN_RECALCULATE_STATISTICS = (short)GetPrivateProfileInt("Other","StatisticsRecalculationInterval", 0, fileName);
	temp = (short)GetPrivateProfileInt("Other","LowAirRangeModifier", 0, fileName);
	LOWAIR_RANGE_MODIFIER = (float)(temp / 100.0F);
	MINIMUM_STRENGTH = (short)GetPrivateProfileInt("Other","MinimumStrength", 0, fileName);
	MAX_DAMAGE_TRIES = (short)GetPrivateProfileInt("Other","MaximumDamageTries", 0, fileName);
	temp = (short)GetPrivateProfileInt("Other","ReaggregationRatio", 0, fileName);
	REAGREGATION_RATIO = (float)(temp / 100.0F);
	MIN_REINFORCE_TIME = (short)GetPrivateProfileInt("Other","ReinforcementTime", 0, fileName);
	SIM_BUBBLE_SIZE = (float)GetPrivateProfileInt("Other","SimBubbleSize", 0, fileName);
	INITIATIVE_LEAK_PER_HOUR = (short)GetPrivateProfileInt("Other","InitiativeLeakPerHour", 0, fileName);

	// RV - Biker - Read tractor vehicles from here
	F4_GENERIC_US_TRUCK_TYPE_SMALL   = (int)GetPrivateProfileInt("Campaign","US_TRUCK_TYPE_SMALL", 534, fileName); // HMMWV
	F4_GENERIC_US_TRUCK_TYPE_LARGE   = (int)GetPrivateProfileInt("Campaign","US_TRUCK_TYPE_LARGE", 548, fileName); // M997
	F4_GENERIC_US_TRUCK_TYPE_TRAILER = (int)GetPrivateProfileInt("Campaign","US_TRUCK_TYPE_TRAILER", 548, fileName); //TBD

	F4_GENERIC_OPFOR_TRUCK_TYPE_SMALL   = (int)GetPrivateProfileInt("Campaign","OPFOR_TRUCK_TYPE_SMALL", 707, fileName); //KrAz T 255B
	F4_GENERIC_OPFOR_TRUCK_TYPE_LARGE   = (int)GetPrivateProfileInt("Campaign","OPFOR_TRUCK_TYPE_LARGE", 707, fileName); //TBD
	F4_GENERIC_OPFOR_TRUCK_TYPE_TRAILER = (int)GetPrivateProfileInt("Campaign","OPFOR_TRUCK_TYPE_TRAILER", 835, fileName); //ZIL-135
	SWITCH_TRACTORS_IN_TE				= (int)GetPrivateProfileInt("Campaign","SWITCH_TRACTORS_IN_TE", 1, fileName); // Because in standard Korea we have switched teams in TE

	/* Campaign - MPS defaults */
	StartOffBonusRepl	= (int)GetPrivateProfileInt("Campaign","StartOffBonusRepl", 1000, fileName);
	StartOffBonusSup	= (int)GetPrivateProfileInt("Campaign","StartOffBonusSup", 5000, fileName);
	StartOffBonusFuel	= (int)GetPrivateProfileInt("Campaign","StartOffBonusFuel", 5000, fileName);
	ActionRate			= (int)GetPrivateProfileInt("Campaign","ActionRate", 8, fileName);
	ActionTimeOut		= (int)GetPrivateProfileInt("Campaign","ActionTimeOut", 24, fileName);

	GetPrivateProfileString("Campaign","DataRateModRepl", "1.0", tmpName, 40, fileName);
	DataRateModRepl		= (float)atof(tmpName);

	GetPrivateProfileString("Campaign","DataRateModSup", "1.0", tmpName, 40, fileName);
	DataRateModSup		= (float)atof(tmpName);

	GetPrivateProfileString("Campaign","RelSquadBonus", "4.0", tmpName, 40, fileName);
	RelSquadBonus		= (float)atof(tmpName);

	NoActionBonusProd	= (int)GetPrivateProfileInt("Campaign","NoActionBonusProd", 0, fileName);
	NoTypeBonusRepl		= (int)GetPrivateProfileInt("Campaign","NoTypeBonusRepl", 0, fileName);
	NewInitiativePoints = (int)GetPrivateProfileInt("Campaign","NewInitiativePoints", 0, fileName);
	CampBugFixes		= (int)GetPrivateProfileInt("Campaign","CampBugFixes", 0, fileName);
	
	GetPrivateProfileString("Campaign","2DHitChanceAir", "3.5", tmpName, 40, fileName);
	HitChanceAir		= (float)atof(tmpName);
	GetPrivateProfileString("Campaign","2DHitChanceGround", "1.5", tmpName, 40, fileName);
	HitChanceGround		= (float)atof(tmpName);
	GetPrivateProfileString("Campaign","MinDeadPct", "0.6", tmpName, 40, fileName);
	MIN_DEAD_PCT		= (float)atof(tmpName);
	// 2002-04-17 MN these are now read from the campaign trigger files to have them definable per campaign.
	//	GetPrivateProfileString("Campaign","FLOTDrawDistance", "2500.0", tmpName, 40, fileName); // 50 km
	//	FLOTDrawDistance	= (float)atof(tmpName);
	//	FLOTSortDirection	= (int)GetPrivateProfileInt("Campaign","SortNorthSouth", 0, fileName);
	//	TheaterXPosition	= (int)GetPrivateProfileInt("Campaign","TheaterXPosit", 512, fileName); 
	// default place in center of korea sized theaters
	//	TheaterYPosition	= (int)GetPrivateProfileInt("Campaign","TheaterYPosit", 512, fileName); 
	// default place in center of korea sized theaters
}
