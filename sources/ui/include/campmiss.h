#ifndef _CAMP_MISS_STRUCT_
#define _CAMP_MISS_STRUCT_

typedef enum{
	AWARD_MEDAL		= 0x001,
	MDL_AFCROSS 	= 0x002,
	MDL_SILVERSTAR	= 0x004,
	MDL_DIST_FLY	= 0x008,
	MDL_AIR_MDL		= 0x010,
	MDL_KOR_CAMP	= 0x020,
	MDL_LONGEVITY	= 0x040,
	COURT_MARTIAL	= 0x080,
	CM_FR_FIRE1		= 0x100,
	CM_FR_FIRE2		= 0x200,
	CM_FR_FIRE3		= 0x400,
	CM_CRASH		= 0x800,
	CM_EJECT		= 0x1000,
	PROMOTION		= 0x2000,			
}RESULT_FLAGS;


extern int MissionResult;

typedef enum{
	DESTROYED_PRIMARY	= 0x01,
	LANDED_AIRCRAFT		= 0x02,
	CRASH_UNDAMAGED		= 0x04,
	EJECT_UNDAMAGED		= 0x08,
	FR_HUMAN_KILLED		= 0x10,
	DONT_SCORE_MISSION	= 0x20,		// Incomplete mission, only record the basics
}CAMP_MISS_FLAGS;

typedef struct
{
		ushort	Flags;
		float	FlightHours;

		//for mission complexity
		int		WeaponsExpended;
		int		ShotsAtPlayer;
		int		AircraftInPackage;
		
		//mission score from Kevin
		int		Score;
		
		//Air-to-Air
		int		Kills;
		int		HumanKills;
		int		Killed;
		int		KilledByHuman;
		int		KilledBySelf;

		//Air-to-Ground
		int		GroundUnitsKilled;
		int		FeaturesDestroyed;
		int		NavalUnitsKilled;

		//other
		int		FriendlyFireKills;
		int		WingmenLost;

}CAMP_MISS_STRUCT;

#endif //_CAMP_MISS_STRUCT_