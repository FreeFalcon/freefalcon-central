#ifndef _LOG_BOOK_H
#define _LOG_BOOK_H

#include <tchar.h>
//#include "stdhdr.h"
#include "ui\include\campmiss.h"

//define value identifying medals for array index
typedef enum
{
	AIR_FORCE_CROSS,
	SILVER_STAR,
	DIST_FLY_CROSS,
	AIR_MEDAL,
	KOREA_CAMPAIGN,		
	LONGEVITY,
	NUM_MEDALS,
}LB_MEDAL;

//Ranks
typedef enum 
{
	 SEC_LT,			
	 LEIUTENANT,		
	 CAPTAIN,			
	 MAJOR,			
	 LT_COL,			
	 COLONEL,			
	 BRIG_GEN,
	 NUM_RANKS,
}LB_RANK;

enum{
  FILENAME_LEN = 32,
  PASSWORD_LEN	= 10,
  PERSONAL_TEXT_LEN = 120,
  COMM_LEN = 12,
  _NAME_LEN_ = 20,
  _CALLSIGN_LEN_ = 12,
};

enum{
	LB_INVALID_CALLSIGN	= 0x01,
	LB_EDITABLE			= 0x02,
	LB_OPPONENT			= 0x04,
	LB_CHECKED			= 0x08,
	LB_REFRESH_PILOT	= 0x10,
	LB_LOADED_ONCE		= 0x20,
};

enum{
	NOPATCH = 70050,
	NOFACE	= 60000,
	LOGBOOK_PICTURE_ID = 8649144,
	LOGBOOK_PICTURE_ID_2 = 8649145,
	LOGBOOK_SQUADRON_ID = 8649146,
	LOGBOOK_SQUADRON_ID_2 = 8649147,

	PATCHES_RESOURCE = 59998,
	PILOTS_RESOURCE = 59999,
};

typedef struct DogfightStats
{
	short	MatchesWon;
	short	MatchesLost;
	short	MatchesWonVHum;
	short	MatchesLostVHum;
	short	Kills;
	short	Killed;
	short	HumanKills;
	short	KilledByHuman;
}DF_STATS;

typedef struct CampaignStats
{
	short	GamesWon;
	short	GamesLost;
	short	GamesTied;
	short	Missions;
	long	TotalScore;
	long	TotalMissionScore;
	short	ConsecMissions;
	short	Kills;
	short	Killed;
	short	HumanKills;
	short	KilledByHuman;
	short	KilledBySelf;
	short	AirToGround;
	short	Static;
	short	Naval;
	short	FriendliesKilled;
	short	MissSinceLastFriendlyKill;
}CAMP_STATS;

typedef struct Pilot
{
	_TCHAR		Name[_NAME_LEN_+1];
	_TCHAR		Callsign[_CALLSIGN_LEN_+1];
	_TCHAR		Password[PASSWORD_LEN+1];
	_TCHAR		Commissioned[COMM_LEN+1];
	_TCHAR		OptionsFile[_CALLSIGN_LEN_+1];
	float		FlightHours;
	float		AceFactor;
	LB_RANK		Rank;
	DF_STATS	Dogfight;
	CAMP_STATS	Campaign;
	uchar		Medals[NUM_MEDALS];
	long		PictureResource;
	_TCHAR		Picture[FILENAME_LEN+1];
	long		PatchResource;
	_TCHAR		Patch[FILENAME_LEN+1];
	_TCHAR		Personal[PERSONAL_TEXT_LEN+1];
	_TCHAR		Squadron[_NAME_LEN_];
	short		voice;							// index from 0 - 11 indicating which voice they want
	long		CheckSum; // If this value is ever NON zero after Decrypting, the Data has been modified
}LB_PILOT;

class LogBookData
{
private:
	void EncryptPwd(void);
	void CalcRank(void);
	void AwardMedals(CAMP_MISS_STRUCT *MissStats);
	float MissionComplexity(CAMP_MISS_STRUCT *MissStats);
	float CampaignDifficulty(void);
public:
	LB_PILOT	Pilot;
	
	LogBookData(void);
	~LogBookData(void);
	void Initialize (void);
	void Cleanup (void);
	int Load(void);
	int LoadData (_TCHAR *PilotName);
	int LoadData (LB_PILOT *NewPilot);
	int SaveData(void);
	void Clear(void);
	void Encrypt(void);
	
	void UpdateDogfight(short MatchWonLost,float Hours, short VsHuman, short Kills, short Killed, short HumanKills, short KilledByHuman );
	void UpdateCampaign(CAMP_MISS_STRUCT *MissStats);
	void FinishCampaign(short WonLostTied);
	
	CAMP_STATS *GetCampaign(void)					{return &Pilot.Campaign;}
	DF_STATS *GetDogfight(void)						{return &Pilot.Dogfight;}
	LB_PILOT *GetPilot(void)						{return &Pilot;}
	// This is used for remote pilots...so I can get them in the class used for drawing the UI
	void	SetPilot(LB_PILOT *data)				{if(data) memcpy(&Pilot,data,sizeof(Pilot)); }

	uchar	GetMedal(LB_MEDAL MedalNo)				{if(MedalNo < NUM_MEDALS) return Pilot.Medals[MedalNo]; else return 0; }
	void	SetMedal(LB_MEDAL MedalNo, uchar Medal) {if(MedalNo < NUM_MEDALS) Pilot.Medals[MedalNo] = Medal;}

	void	SetFlightHours(float Hours)				{Pilot.FlightHours = Hours;}
	void	UpdateFlightHours(float Hours)			{Pilot.FlightHours += Hours;}

	_TCHAR	*GetPicture(void)						{return Pilot.Picture;}
	long	GetPictureResource(void)				{return Pilot.PictureResource;}
	void	SetPicture(_TCHAR *filename)			{if(_tcslen(filename) <= FILENAME_LEN) _tcscpy(Pilot.Picture,filename); Pilot.PictureResource = 0;}
	void	SetPicture(long imageID)				{Pilot.PictureResource = imageID; _tcscpy(Pilot.Picture,"");}

	_TCHAR	*GetPatch(void)							{return Pilot.Patch;}
	long	GetPatchResource(void)					{return Pilot.PatchResource;}
	void	SetPatch(_TCHAR *filename)				{if(_tcslen(filename) <= FILENAME_LEN) _tcscpy(Pilot.Patch,filename); Pilot.PatchResource = 0;}
	void	SetPatch(long imageID)					{Pilot.PatchResource = imageID; _tcscpy(Pilot.Patch,"");}

	_TCHAR *Name(void)								{return Pilot.Name;}
	_TCHAR *NameWRank(void);
	void	SetName(_TCHAR *Name)					{if(_tcslen(Name) <= _NAME_LEN_) _tcscpy(Pilot.Name,Name);}
	
	_TCHAR *Callsign(void)							{return Pilot.Callsign;}
	void	SetCallsign(_TCHAR *Callsign)			{if(_tcslen(Callsign) <= _CALLSIGN_LEN_) _tcscpy(Pilot.Callsign,Callsign);}
	
	_TCHAR *Squadron(void)							{return Pilot.Squadron;}
	void	SetSquadron(_TCHAR *Squadron)			{if(_tcslen(Squadron) <= _NAME_LEN_) _tcscpy(Pilot.Squadron,Squadron);}
	
	int		CheckPassword(_TCHAR *Pwd);
	int		SetPassword(_TCHAR *Password);	
	int		GetPassword(_TCHAR *Pwd);
	
	_TCHAR *Personal(void)							{return Pilot.Personal;}
	void	SetPersonal(_TCHAR *Personal)			{if(_tcslen(Personal) <= PERSONAL_TEXT_LEN) _tcscpy(Pilot.Personal,Personal);}
	
	_TCHAR *OptionsFile(void)						{return Pilot.OptionsFile;}
	void	SetOptionsFile(_TCHAR *OptionsFile)		{if(_tcslen(OptionsFile) <= _CALLSIGN_LEN_) _tcscpy(Pilot.OptionsFile,OptionsFile);}
	
	float	AceFactor(void)							{return Pilot.AceFactor;}
	void	SetAceFactor(float Factor)				{Pilot.AceFactor = Factor;SaveData();}
	
	LB_RANK Rank(void)								{return Pilot.Rank;}
	_TCHAR *Commissioned(void)						{return Pilot.Commissioned;}
	void	SetCommissioned(_TCHAR *Date)			{if(_tcslen(Date) <= COMM_LEN) _tcscpy(Pilot.Commissioned,Date);}
	float	FlightHours(void)						{return Pilot.FlightHours;}
	short	TotalKills(void);
	short	TotalKilled(void);

	void SetVoice(short newvoice)						{Pilot.voice = newvoice;}
	uchar Voice(void)								{return (uchar)Pilot.voice;}
};

extern class LogBookData LogBook;
extern class LogBookData UI_logbk;
#endif
