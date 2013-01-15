#ifndef _TEAM_DATA_H_
#define _TEAM_DATA_H_

#define MAX_PILOT_PICTURES 110
#define MAX_FEMALE_PILOTS 31
#define MAX_MALE_PILOTS (MAX_PILOT_PICTURES-MAX_FEMALE_PILOTS)

enum
{
	FLAG_STATUS=0,
	BIG_VERT_DARK,
	BIG_VERT,
	BIG_HORIZ,
	SMALL_HORIZ,
	MAX_FLAG_TYPES,
	TOTAL_FLAGS=8,
	NUM_TEAM_COLORS=8, // DON'T allow 0 to be selectable by user in arrays
};

enum
{
	REDDOT1           =50200,
	REDDOT2           =50201,
	REDDOT3           =50202,
	REDDOT4           =50203,
	REDDOT5           =50204,
	REDDOT6           =50205,
	REDDOT7           =50206,
	REDDOT8           =50207,
	BLUEDOT1           =50208,
	BLUEDOT2           =50209,
	BLUEDOT3           =50210,
	BLUEDOT4           =50211,
	BLUEDOT5           =50212,
	BLUEDOT6           =50213,
	BLUEDOT7           =50214,
	BLUEDOT8           =50215,
	WHITEDOT1           =50216,
	WHITEDOT2           =50217,
	WHITEDOT3           =50218,
	WHITEDOT4           =50219,
	WHITEDOT5           =50220,
	WHITEDOT6           =50221,
	WHITEDOT7           =50222,
	WHITEDOT8           =50223,
	GREENDOT1           =50224,
	GREENDOT2           =50225,
	GREENDOT3           =50226,
	GREENDOT4           =50227,
	GREENDOT5           =50228,
	GREENDOT6           =50229,
	GREENDOT7           =50230,
	GREENDOT8           =50231,
	BROWNDOT1           =50232,
	BROWNDOT2           =50233,
	BROWNDOT3           =50234,
	BROWNDOT4           =50235,
	BROWNDOT5           =50236,
	BROWNDOT6           =50237,
	BROWNDOT7           =50238,
	BROWNDOT8           =50239,
	ORANGEDOT1          = 50240,
	ORANGEDOT2          = 50241,
	ORANGEDOT3          = 50242,
	ORANGEDOT4          = 50243,
	ORANGEDOT5          = 50244,
	ORANGEDOT6          = 50245,
	ORANGEDOT7          = 50246,
	ORANGEDOT8          = 50247,
	YELLOWDOT1          = 50248,
	YELLOWDOT2          = 50249,
	YELLOWDOT3          = 50250,
	YELLOWDOT4          = 50251,
	YELLOWDOT5          = 50252,
	YELLOWDOT6          = 50253,
	YELLOWDOT7          = 50254,
	YELLOWDOT8          = 50255,
	GREYDOT1           =50256,
	GREYDOT2           =50257,
	GREYDOT3           =50258,
	GREYDOT4           =50259,
	GREYDOT5           =50260,
	GREYDOT6           =50261,
	GREYDOT7           =50262,
	GREYDOT8           =50264,
};

extern long PilotImageIDs[MAX_PILOT_PICTURES];
extern COLORREF TeamColorList[NUM_TEAM_COLORS];
extern char TeamColorUse[NUM_TEAM_COLORS];
extern long TeamColorIconIDs[NUM_TEAM_COLORS][2];
extern long TeamFlightColorIconIDs[NUM_TEAM_COLORS][8][2];
extern long FlagImageID[TOTAL_FLAGS][MAX_FLAG_TYPES];
extern long SquadronMatchIDs[][2];
extern long BLIP_IDS[8][8];

// Assign Pilot Image based on voice id
uchar AssignUIImageID(uchar voice_id);
// Assign squadron Image based on the Squadron #
uchar AssignUISquadronID(short SquadronNo);
#endif