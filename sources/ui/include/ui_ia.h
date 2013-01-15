#ifndef _IA_Header_H_
#define _IA_Header_H_

#define MAX_NAME_LENGTH 32
#define MAX_SCORES 12

enum
{
	_MISSION_AIR_TO_AIR_=0,
	_MISSION_AIR_TO_GROUND_,
};

enum
{
	_PILOT_LEVEL_NOVICE_=0,
	_PILOT_LEVEL_CADET_,
	_PILOT_LEVEL_ROOKIE_,
	_PILOT_LEVEL_VETERAN_,
	_PILOT_LEVEL_ACE_,
};

enum
{
	_NO_SAM_SITES_=0,
	_SAM_SITES_,
};

enum
{
	_NO_AAA_SITES_=0,
	_AAA_SITES_,
};

typedef struct
{
	int MissionType;
	int PilotLevel;
	int SamSites;
	int AAASites;
} UI_IA;

extern UI_IA InstantActionSettings;

// UI Types.... lots of miscellaneous structs etc


struct kill_list
{
	long	id;
	int		num;
	long	points;
	kill_list *next;
};

#pragma pack(1)
typedef struct
{
	char Name[MAX_NAME_LENGTH];
	long Score;
} HighScore;

typedef struct
{
	HighScore Scores[MAX_SCORES];
	long CheckSum;
} HighScoreList;

#pragma pack()

#endif