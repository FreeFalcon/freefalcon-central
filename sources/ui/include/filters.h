#ifndef _UI_FILTERS_H_
#define _UI_FILTERS_H_

#ifndef TEAM_H
#include "team.h"
#endif

#ifndef MISGROUP_H
#include "package.h"
#endif

#define _MAX_TEAMS_ (NUM_TEAMS) // (8) Kevin only has 7 defined
#define _MAX_DIRECTIONS_	  (8)
#define _MIN_ZOOM_LEVEL_      (1)
#define _MAX_ZOOM_LEVEL_     (32)
#define I_NEED_TO_DRAW     (0x01)
#define I_NEED_TO_DRAW_MAP (0x02)

#define _MAP_NUM_OBJ_TYPES_		(14)
#define _MAP_NUM_AIR_TYPES_		(5)
#define _MAP_NUM_GND_TYPES_		(4)
#define _MAP_NUM_GND_LEVELS_	(3)
#define _MAP_NUM_NAV_TYPES_		(2)
#define _MAP_NUM_THREAT_TYPES_	(4)

#define NUM_BE_LINES	(6)
#define NUM_BE_CIRCLES	(6)

enum // Objective Filter Types
{
	_OBTV_AIR_DEFENSE		= 0x00000001,
	_OBTV_AIR_FIELDS		= 0x00000002,
	_OBTV_ARMY				= 0x00000004,
	_OBTV_CCC				= 0x00000008,
	_OBTV_INFRASTRUCTURE	= 0x00000010,
	_OBTV_LOGISTICS			= 0x00000020,
	_OBTV_OTHER				= 0x00000040,
	_OBTV_NAVIGATION		= 0x00000080,
	_OBTV_POLITICAL			= 0x00000100,
	_OBTV_WAR_PRODUCTION	= 0x00000200,
	_OBTV_NAVAL				= 0x00000400,
	_UNIT_SQUADRON			= 0x00000800,
	_UNIT_PACKAGE			= 0x00001000,
	_VC_CONDITION_			= 0x00002000,
};

enum
{
	_UNIT_AIR_DEFENSE		= 0x00000001,
	_UNIT_COMBAT			= 0x00000002,
	_UNIT_SUPPORT			= 0x00000004,
	_UNIT_ARTILLERY			= 0x00000008,
	_UNIT_ATTACK			= 0x00000010,
	_UNIT_HELICOPTER		= 0x00000020,
	_UNIT_BOMBER			= 0x00000040,
	_UNIT_FIGHTER			= 0x00000080,
	_UNIT_BATTALION			= 0x01000000,
	_UNIT_BRIGADE			= 0x02000000,
	_UNIT_DIVISION			= 0x04000000,
	_UNIT_NAVAL				= 0x08000000,
	_UNIT_GROUND_MASK		= 0x07000000,
	_UNIT_NAVAL_MASK		= 0x08000000,
};

enum
{
	OOB_AIRFORCE	=0x00010000,
	OOB_ARMY		=0x00020000,
	OOB_NAVY		=0x00040000,
	OOB_OBJECTIVE	=0x00080000,
	OOB_TEAM_MASK   =0xff000000,
};

enum
{
	_THR_SAM_LOW		=0x01,
	_THR_SAM_HIGH		=0x02,
	_THR_RADAR_LOW		=0x04,
	_THR_RADAR_HIGH		=0x08,
};

enum
{
	_THREAT_SAM_LOW_=0,
	_THREAT_SAM_HIGH_,
	_THREAT_RADAR_LOW_,
	_THREAT_RADAR_HIGH_,
};

enum // Ship/Air Unit Icon IDs
{
	TGT_CUR				=10501,
	TGT_CUR_SEL			=10502,
	TGT_CUR_ERROR		=10503,
	IP_CUR				=10504,
	IP_CUR_SEL			=10505,
	IP_CUR_ERROR		=10506,
	STPT_CUR			=10507,
	STPT_CUR_SEL		=10508,
	STPT_CUR_ERROR		=10509,
	TGT_OTR				=10510,
	TGT_OTR_SEL			=10511,
	TGT_OTR_OTHER		=10512,
	IP_OTR				=10513,
	IP_OTR_SEL			=10514,
	IP_OTR_OTHER		=10515,
	STPT_OTR			=10516,
	STPT_OTR_SEL		=10517,
	STPT_OTR_OTHER		=10518,
	ASSIGNED_TGT_CUR 	=10519,
	HOME_BASE_CUR	 	=10520,
	ADDLINE_CUR			=10521,
	ADDLINE_CUR_SEL		=10522,
};

typedef struct
{
	char Domain;
	char Class;
	char Type;
	char SubType;
	long UIType;
	long OOBCategory;
} FILTER_TABLE;

typedef struct
{
	char SubType;
	long IconID[3]; // 0=Friendly,1=Enemy,2=Neutral
	long UIType;
} AIR_ICONS;

extern FILTER_TABLE ObjectiveFilters[];
extern FILTER_TABLE UnitFilters[];
extern AIR_ICONS AirIcons[];
extern long OBJ_TypeList[_MAP_NUM_OBJ_TYPES_];
extern long NAV_TypeList[_MAP_NUM_NAV_TYPES_];
extern long AIR_TypeList[_MAP_NUM_AIR_TYPES_];
extern long GND_TypeList[_MAP_NUM_GND_TYPES_];
extern long GND_LevelList[_MAP_NUM_GND_LEVELS_];
extern long THR_TypeList[_MAP_NUM_THREAT_TYPES_];
extern float BullsEyeLines[NUM_BE_LINES][4];
extern float BullsEyeRadius[NUM_BE_CIRCLES];

long FindTypeIndex(long type,long TypeList[],int size);
long FindObjectiveIndex(Objective Obj);
long GetObjectiveType(CampBaseClass *ent);
long GetObjectiveCategory(Objective Obj);
long GetAirIcon(uchar STYPE);
long FindDivisionType(uchar type);
long FindUnitType(Unit unit);
long FindUnitCategory(Unit unit);

#endif
