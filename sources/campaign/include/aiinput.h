// ATM Inputs

extern short IMMEDIATE_MIN_TIME;
extern short IMMEDIATE_MAX_TIME;
extern short LONGRANGE_MIN_TIME;
extern short LONGRANGE_MAX_TIME;
extern short ATM_ASSIGN_RATIO;
extern short MAX_FLYMISSION_THREAT;
extern short MAX_FLYMISSION_HIGHTHREAT;
extern short MAX_FLYMISSION_NOTHREAT;
extern short MIN_SEADESCORT_THREAT;
extern short MIN_AVOID_THREAT;
extern short MIN_AP_DISTANCE;
extern short BREAKPOINT_DISTANCE;
extern short ATM_TARGET_CLEAR_RADIUS;
extern short LOITER_DIST;
extern short SWEEP_DISTANCE;
extern short MINIMUM_BDA_PRIORITY;
extern short MINIMUM_AWACS_DISTANCE;
extern short MAXIMUM_AWACS_DISTANCE;
extern short MINIMUM_JSTAR_DISTANCE;
extern short MAXIMUM_JSTAR_DISTANCE;
extern short MINIMUM_TANKER_DISTANCE;
extern short MAXIMUM_TANKER_DISTANCE;
extern short MINIMUM_ECM_DISTANCE;
extern short MAXIMUM_ECM_DISTANCE;
extern short FIRST_COLONEL;						// Indexes into our pilot name array
extern short FIRST_COMMANDER;
extern short FIRST_WINGMAN;
extern short LAST_WINGMAN;
extern short MAX_AA_STR_FOR_ESCORT;				// Max pack Air to Air str in order to add escorts
extern short BARCAP_REQUEST_INTERVAL;			// Minimum time between BARCAP mission requests
extern short ONCALL_CAS_FLIGHTS_PER_REQUEST;	// How many CAS flights to plan per ONCALL request
extern short MIN_TASK_AIR;						// Retask time, in minutes
extern short MIN_PLAN_AIR;						// Replan time (KCK NOTE: PLAN_AIR max is 8)
extern short VICTORY_CHECK_TIME;				// Check victory condition time in Tactical Engagement
extern short FLIGHT_MOVE_CHECK_INTERVAL;		// How often to check a flight for movement (in seconds)
extern short FLIGHT_COMBAT_CHECK_INTERVAL;		// How often to check a flight for combat (in seconds)
extern short AIR_UPDATE_CHECK_INTERVAL;			// How often to check squadrons/packages for movement/update (in seconds)
extern short FLIGHT_COMBAT_RATE;				// Amount of time between weapon shots (in seconds)
extern short PACKAGE_CYCLES_TO_WAIT;			// How many planning cycles to wait for tankers/barcap/etc
extern short AIR_PATH_MAX;						// Max amount of nodes to search for air paths
extern float MIN_IGNORE_RANGE;					// Minimum range (km) at which we'll ignore an air target
extern short MAX_SAR_DIST;						// Max distance from front to fly sar choppers into. (km)
extern short MAX_BAI_DIST;						// Max distance from front to fly BAI missions into. (km)
extern long PILOT_ASSIGN_TIME;					// Time before takeoff to assign pilots
extern short AIRCRAFT_TURNAROUND_TIME_MINUTES;	// Time to wait between missions before an aircraft is available again

// GTM Inputs
extern short PRIMARY_OBJ_PRIORITY;				// Priorities for primary and secondary objectives
extern short SECONDARY_OBJ_PRIORITY;			// Only cities and towns can be POs and SOs
extern short MAX_OFFENSES;						// Maximum number of offensive POs.
extern short MIN_OFFENSIVE_UNITS;				// Minimum number of units to constitute an offensive
extern short MINIMUM_ADJUSTED_LEVEL;			// Minimum % strength of unit assigned to offense.
extern short MINIMUM_VIABLE_LEVEL;				// Minimum % strength in order to be considered a viable unit
extern short MAX_ASSIGNMENT_DIST;				// Maximum distance from an object we'll assign a unit to
extern short MAXLINKS_FROM_SO_OFFENSIVE;		// Maximum objective links from our secondary objective we'll place units
extern short MAXLINKS_FROM_SO_DEFENSIVE;		// Same as above for defensive units
extern short BRIGADE_BREAK_BASE;				// Base moral a brigade breaks at (modified by objective priority)
extern short ROLE_SCORE_MODIFIER;				// Used to adjust role score for scoring purposes
extern short MIN_TASK_GROUND;					// Retask time, in minutes
extern short MIN_PLAN_GROUND;					// Replan time, in minutes
extern short MIN_REPAIR_OBJECTIVES;				// How often to repair objectives (in minutes);
extern short MIN_RESUPPLY;						// How often to resupply troops (in minutes)
extern short MORALE_REGAIN_RATE;				// How much morale per hour a typical unit will regain
extern short REGAIN_RATE_MULTIPLIER_FOR_TE;		// Morale/Fatigue regain rate multiplier for TE missions
extern short FOOT_MOVE_CHECK_INTERVAL;			// How often to check a foot battalion for movement (in seconds)
extern short TRACKED_MOVE_CHECK_INTERVAL;		// How often to check a tracked/wheeled battalion for combat (in seconds)
extern short GROUND_COMBAT_CHECK_INTERVAL;		// How often to check ground battalions for combat
extern short GROUND_UPDATE_CHECK_INTERVAL;		// How often to check brigades for update (in seconds)
extern short GROUND_COMBAT_RATE;				// Amount of time between weapon shots (in seconds)
extern short GROUND_PATH_MAX;					// Max amount of nodes to search for ground paths
extern short OBJ_GROUND_PATH_MAX_SEARCH;		// Max amount of nodes to search for objective paths
extern short OBJ_GROUND_PATH_MAX_COST;			// Max amount of nodes to search for objective paths
extern short MIN_FULL_OFFENSIVE_INITIATIVE;		// Minimum initiative required to launch a full offensive
extern short MIN_COUNTER_ATTACK_INITIATIVE;		// Minimum initiative required to launch a counter-attack
extern short MIN_SECURE_INITIATIVE;				// Minimum initiative required to secure defensive objectives
extern short MINIMUM_EXP_TO_FIRE_PREGUIDE;		// Minimum experience needed to fire SAMs before going to guide mode
extern long OFFENSIVE_REINFORCEMENT_TIME;		// How often we get reinforcements when on the offensive
extern long DEFENSIVE_REINFORCEMENT_TIME;		// How often we get reinforcements when on the defensive
extern long CONSOLIDATE_REINFORCEMENT_TIME;		// How often we get reinforcements when consolidating
extern long ACTION_PREP_TIME;					// How far in advance to plan any offensives

// NTM Inputs
extern short MIN_TASK_NAVAL;					// Retask time, in minutes	
extern short MIN_PLAN_NAVAL;					// Replan time, in minutes
extern short NAVAL_MOVE_CHECK_INTERVAL;			// How often to check a task force for movement (in seconds)
extern short NAVAL_COMBAT_CHECK_INTERVAL;		// How often to fire naval units (in seconds)
extern short NAVAL_COMBAT_RATE;					// Amount of time between weapon shots (in seconds)

// Other Inputs
extern short LOW_ALTITUDE_CUTOFF;
extern short MAX_GROUND_SEARCH;	          		// How far to look for surface units or objectives
extern short MAX_AIR_SEARCH;		           	// How far to look for air threats
extern short NEW_CLOUD_CHANCE;
extern short DEL_CLOUD_CHANCE;
extern short PLAYER_BUBBLE_MOVERS;				// The # of objects we try to keep in player bubble
extern short FALCON_PLAYER_TEAM;				// What team the player starts on
extern short CAMP_RESYNC_TIME;					// How often to resync campaigns (in seconds)
extern short CAMP_RESYNC_TIME;					// How often to resync campaigns (in seconds)
extern unsigned short BUBBLE_REBUILD_TIME;				// How often to rebuild the player bubble (in seconds)
extern short STANDARD_EVENT_LENGTH;				// Length of stored event queues
extern short PRIORITY_EVENT_LENGTH;
extern short MIN_RECALCULATE_STATISTICS;		// How often to do the unit statistics stuff
extern float LOWAIR_RANGE_MODIFIER;				// % of Air range to get LowAir range from
extern short MINIMUM_STRENGTH;					// Minimum strength points we'll bother appling with colateral damage
extern short MAX_DAMAGE_TRIES;					// Max # of times we'll attempt to find a random target vehicle/feature
extern float REAGREGATION_RATIO;				// Ratio over 1.0 which things will reaggregate at
extern short MIN_REINFORCE_TIME;				// Value, in minutes, of each reinforcement cycle
extern float SIM_BUBBLE_SIZE;					// Radius (in feet) of the Sim's campaign lists.
extern short INITIATIVE_LEAK_PER_HOUR;			// How much initiative is adjusted automatically per hour
extern int F4_GENERIC_US_TRUCK_TYPE_SMALL;		// Small US tractor vehicle
extern int F4_GENERIC_US_TRUCK_TYPE_LARGE;		// Large US tractor vehicle   
extern int F4_GENERIC_US_TRUCK_TYPE_TRAILER;	// Tractor US for trailers
extern int F4_GENERIC_OPFOR_TRUCK_TYPE_SMALL;	// Small OPFOR tractor vehicle
extern int F4_GENERIC_OPFOR_TRUCK_TYPE_LARGE;	// Large OPFOR tractor vehicle 
extern int F4_GENERIC_OPFOR_TRUCK_TYPE_TRAILER;	// Tractor OPFOR for trailers
extern int SWITCH_TRACTORS_IN_TE;				// Because in standard Korea we have switched teams in TE

// Campaign Inputs
extern int StartOffBonusRepl;					// Replacement bonus when a team goes offensive
extern int StartOffBonusSup;					// Supply Bonus when a team goes offensive
extern int StartOffBonusFuel;					// Fuel Bonus when a team goes offensive
extern int ActionRate;							// How often we can start a new action (in hours)
extern int ActionTimeOut;						// Maximum time an offensive action can last
extern float DataRateModRepl;					// Modification factor for production data rate for replacements
extern float DataRateModSup;					// Modification factor for production data rate for fuel and supply
extern float RelSquadBonus;						// Relative replacements of squadrons to ground units
// in fact bools
extern int NoActionBonusProd;					// No action bonus for production
extern int NoTypeBonusRepl;						// No type bonus for replacements, new supply system, bugfix in "supplies units" function
extern int NewInitiativePoints;					// New initiative point setting
extern int CampBugFixes;						// Smaller bugfixes
extern float HitChanceAir;						// 2D hitchance air target modulation
extern float HitChanceGround;					// 2D hitchance ground target modulation
// These are in fact read from the trigger files now - no need to mess with the campaign files !
extern float FLOTDrawDistance;					// Distance between FLOT objectives at which we won't draw a line anymore (2 front campaigns...)
extern int FLOTSortDirection;					// to sort FLOT by x or y objective coordinates
extern int TheaterXPosition;					// central theater x/y position for calculating new bullseye posit
extern int TheaterYPosition;					// central theater x/y position for calculating new bullseye posit

void ReadCampAIInputs (char * name);


