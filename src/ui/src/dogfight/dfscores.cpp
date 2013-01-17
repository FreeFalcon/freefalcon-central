/*

	Dogfight Scoring routines

*/

#include <windows.h>
#include "entity.h"
#include "Mesg.h"
#include "MsgInc\DamageMsg.h"
#include "MsgInc\WeaponFireMsg.h"
#include "MsgInc\DeathMessage.h"
#include "MsgInc\MissileEndMsg.h"
#include "MsgInc\LandingMessage.h"
#include "MsgInc\EjectMsg.h"
#include "MsgInc\PlayerStatusMsg.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "evtparse.h"
#include "events.h"
#include "uicomms.h"
#include "dfcomms.h"
#include "userids.h"
#include "textids.h"
#include "events.h"
#include "dogfight.h"
#include "MissEval.h"

short TeamTotals[MAX_DOGFIGHT_TEAMS][3]; // Kills=0,Deaths=1,Total=2
short TeamUsed[MAX_DOGFIGHT_TEAMS];
short TeamRank[MAX_DOGFIGHT_TEAMS];
PilotSortClass *SortedPilotList=NULL;

extern C_Handler *gMainHandler;
extern C_SoundBite *gDogfightBites;
extern long DFTeamNameStrIDs[];
extern COLORREF DFTeamColors[];

enum
{
	_FIRST_PLACE_=0x0001,
	_SECOND_PLACE_=0x0002,
	_THIRD_PLACE_=0x0004,
	_LAST_PLACE_=0x0008,
	_MOST_KILLS_=0x0010,
	_FEWEST_KILLS_=0x0020,
	_MOST_FRAGS_=0x0040,
	_MOST_DEATHS_=0x0080,
};

long DFTeamNameStrIDs[]=
{
	0,
	TXT_CRIMSONFLIGHT,
	TXT_SHARKFLIGHT,
	TXT_VIPREFLIGHT,
	TXT_TIGERFLIGHT,
};

COLORREF DFTeamColors[]= // COLORREF format is BGR)
{
	// Neutral Team
	0x00ffffff,	// (RGB 255,255,255)
	// Crimson Team
	0x002303c1, // (RGB 193,3,35)
	// Shark Team
	0x00918316, // (RGB 22,131,151)
	// USA Team
	0x00ffffff,	// (RGB 255,255,255)
//	// Viper Team
//	0x00179f05, // (RGB 5,159,23)
	// Tiger Team
	0x0003c0e8, // (RGB 232,192,3)
};

void TallyTeamKills(void)
	{
	int		i;
	short	kills[MAX_DOGFIGHT_TEAMS];
	short	deaths[MAX_DOGFIGHT_TEAMS];
	short	score[MAX_DOGFIGHT_TEAMS];

	memset(TeamTotals,0,sizeof(short)*MAX_DOGFIGHT_TEAMS*3);
	memset(TeamUsed,0,sizeof(short)*MAX_DOGFIGHT_TEAMS);

	TheCampaign.MissionEvaluator->GetTeamKills(kills);
	TheCampaign.MissionEvaluator->GetTeamDeaths(deaths);
	TheCampaign.MissionEvaluator->GetTeamScore(score);

	// Tally Kills (AI & Human)
	for(i=0;i<MAX_DOGFIGHT_TEAMS;i++)
		{
		TeamTotals[i][0] = kills[i];
		TeamTotals[i][1] = deaths[i];
		TeamTotals[i][2] = score[i];
		}
	}

void AddtoSortedList(PilotDataClass *pilot_ptr, int team)
	{
	PilotSortClass	*cur,*last=NULL,*newone;

	newone = new PilotSortClass(pilot_ptr);
	newone->team = static_cast<short>(team);

	cur = SortedPilotList;
	while (cur && cur->pilot_data->score > pilot_ptr->score)
		{
		last = cur;
		cur = cur->next;
		}

	if (last)
		{
		newone->next = cur;
		last->next = newone;
		}
	else
		{
		newone->next = SortedPilotList;
		SortedPilotList = newone;
		}
	}

void ClearSortedPilotList()
	{
	PilotSortClass *cur,*last;

	cur=SortedPilotList;
	while(cur)
	{
		last=cur;
		cur=cur->next;
		delete last;
	}
	SortedPilotList=NULL;
	}

long FigureOutHowIDid()
	{
	long			HowIDid = 0;
	short			place=0,playerrank=0;
	int				team=0,kills=0;
	int				MostKills=0,LeastKills=0,MostDeaths=0,MostFrags=0;
	PilotSortClass	*cur=NULL,*player=NULL,*MKills=NULL,*LKills=NULL,*MDeaths=NULL,*MFrags=NULL;

	team=FalconLocalSession->GetCountry();

	MostKills=-1;
	MostDeaths=-1;
	LeastKills=10000;
	MostFrags=-1;
	MKills=NULL;
	MDeaths=NULL;
	LKills=NULL;
	MFrags=NULL;

	cur=SortedPilotList;
	place=0;
	while(cur)
		{
		if(cur->pilot_data == TheCampaign.MissionEvaluator->player_pilot)
			{
			playerrank=place;
			player=cur;
			}

		kills = cur->pilot_data->aa_kills;

/*		for(i=0;i<MAX_DOGFIGHT_TEAMS;i++)
			{
			if(i != cur->team)
				kills += cur->pilot_data->kills[i][VS_AI] + cur->pilot_data->kills[i][VS_HUMAN];
			}
*/

		if(kills > MostKills)
			{
			MKills=cur;
			MostKills=kills;
			}
		if(kills < LeastKills)
			{
			LKills=cur;
			LeastKills=kills;
			}
/*
		if((cur->pilot_data->kills[cur->team][VS_AI] + cur->pilot_data->kills[cur->team][VS_HUMAN]) > MostFrags)
			{
			MFrags = cur;
			MostFrags = cur->pilot_data->kills[cur->team][VS_AI] + cur->pilot_data->kills[cur->team][VS_HUMAN];
			}
*/
		if((cur->pilot_data->deaths[VS_AI] + cur->pilot_data->deaths[VS_HUMAN]) > MostDeaths)
			{
			MDeaths = cur;
			MostDeaths = cur->pilot_data->deaths[VS_AI] + cur->pilot_data->deaths[VS_HUMAN];
			}
		cur=cur->next;
		place++;
		}

	if(player == MKills)
		HowIDid |= _MOST_KILLS_;
	if(player == MDeaths)
		HowIDid |= _MOST_DEATHS_;
	if(player == MFrags)
		HowIDid |= _MOST_FRAGS_;
	if(player == LKills)
		HowIDid |= _FEWEST_KILLS_;

	if(SimDogfight.GetGameType() != dog_Furball)
		{
		if(TeamRank[0] == team && TeamUsed[1])
			HowIDid |= _FIRST_PLACE_;
		else
			HowIDid |= _LAST_PLACE_;
		if(TeamRank[1] == team && TeamUsed[2])
			HowIDid |= _SECOND_PLACE_;
		else
			HowIDid |= _LAST_PLACE_;
		if(TeamRank[2] == team && TeamUsed[3])
			HowIDid |= _THIRD_PLACE_;
		else
			HowIDid |= _LAST_PLACE_;
		if(TeamRank[3] == team)
			HowIDid |= _LAST_PLACE_;
		}
	else
		{
		if(player && !player->next)
			place=_LAST_PLACE_;
		else
			{
			if(!playerrank)
				HowIDid |= _FIRST_PLACE_;
			if(playerrank == 1)
				HowIDid |= _SECOND_PLACE_;
			if(playerrank == 2)
				HowIDid |= _THIRD_PLACE_;
			}
		}

	return(HowIDid);
	}

void PlayDogfightBite()
{
	long SoundID;
	long HowIDid;

	HowIDid=FigureOutHowIDid();
	if(HowIDid & (_FIRST_PLACE_ | _MOST_KILLS_) && SimDogfight.GetGameType() != dog_Furball)
	{
		SoundID=gDogfightBites->Pick(DF5);
		if(SoundID)
			gSoundMgr->PlaySound(SoundID);
	}
	else if(HowIDid & _FIRST_PLACE_)
	{
		SoundID=gDogfightBites->Pick(DF1);
		if(SoundID)
			gSoundMgr->PlaySound(SoundID);
	}
	else if((HowIDid & _MOST_KILLS_) && SimDogfight.GetGameType() != dog_Furball)
	{
		SoundID=gDogfightBites->Pick(DF6);
		if(SoundID)
			gSoundMgr->PlaySound(SoundID);
	}
	else if(HowIDid & _SECOND_PLACE_)
	{
		SoundID=gDogfightBites->Pick(DF2);
		if(SoundID)
			gSoundMgr->PlaySound(SoundID);
	}
	else if(HowIDid & _THIRD_PLACE_)
	{
		SoundID=gDogfightBites->Pick(DF3);
		if(SoundID)
			gSoundMgr->PlaySound(SoundID);
	}
	else if(HowIDid & _LAST_PLACE_)
	{
		SoundID=gDogfightBites->Pick(DF4);
		if(SoundID)
			gSoundMgr->PlaySound(SoundID);
	}
	else if(HowIDid & _MOST_FRAGS_)
	{
		SoundID=gDogfightBites->Pick(DF8);
		if(SoundID)
			gSoundMgr->PlaySound(SoundID);
	}
	else if(HowIDid & _FEWEST_KILLS_)
	{
		SoundID=gDogfightBites->Pick(DF7);
		if(SoundID)
			gSoundMgr->PlaySound(SoundID);
	}
	else if(HowIDid & _MOST_DEATHS_)
	{
		SoundID=gDogfightBites->Pick(DF9);
		if(SoundID)
			gSoundMgr->PlaySound(SoundID);
	}
}

void DisplayDogfightResults()
{
	C_Window	*win;
	C_Text		*txt;
	int			Y,i,j,kills,human_only=0;
	BOOL		ShowTeams;
	long		PlayerClient;
	_TCHAR		buffer[80];
	PilotSortClass		*cur;
	FlightDataClass		*flight_data;
	PilotDataClass		*pilot_data;

	for(i=0;i<MAX_DOGFIGHT_TEAMS;i++)
		TeamRank[i]=static_cast<short>(i);

	// Tally Kills
	TallyTeamKills();

	for(i=1;i<MAX_DOGFIGHT_TEAMS;i++) // Bubble sort :)
		for(j=0;j<i;j++)
			if(TeamTotals[TeamRank[j]][2] < TeamTotals[TeamRank[i]][2])
			{
				Y=TeamRank[i];
				TeamRank[i]=TeamRank[j];
				TeamRank[j]=static_cast<short>(Y);
			}

	if(SimDogfight.GetGameType() == dog_Furball)
	{
		ShowTeams=FALSE;
		PlayerClient=4;
	}
	else
	{
		ShowTeams=TRUE;
		PlayerClient=3;
	}

	// Sort
	win=gMainHandler->FindWindow(DF_DBRF_WIN);
	if(win)
	{
		// Build a pilot list sorted by score from the MissionEvaluator.
		flight_data = TheCampaign.MissionEvaluator->flight_data;
		while (flight_data)
			{
			pilot_data = flight_data->pilot_list;
			while (pilot_data)
				{
				if (!human_only || pilot_data->pilot_flags & PFLAG_PLAYER_CONTROLLED)
					{
					AddtoSortedList(pilot_data,flight_data->flight_team);
					TeamUsed[flight_data->flight_team] = 1;
					}
				pilot_data = pilot_data->next_pilot;
				}
			flight_data = flight_data->next_flight;
			}

		if(ShowTeams)
		{
			win->DisableCluster(200);
			win->EnableCluster(100);
			// Display Team scores
			Y=0;
			for(i=0;i<MAX_DOGFIGHT_TEAMS;i++)
			{
				if(TeamUsed[TeamRank[i]])
				{
					txt=new C_Text;
					txt->Setup(C_DONT_CARE,C_TYPE_LEFT);
					txt->SetText(gStringMgr->GetString(DFTeamNameStrIDs[TeamRank[i]]));
					txt->SetXY(0,Y);
					txt->SetFGColor(DFTeamColors[TeamRank[i]]);
					txt->SetFont(win->Font_);
					txt->SetClient(2);
					txt->SetFlagBitOn(C_BIT_LEFT);
					win->AddControl(txt);

					_stprintf(buffer,"%1d",TeamTotals[TeamRank[i]][2]);

					txt=new C_Text;
					txt->Setup(C_DONT_CARE,C_TYPE_RIGHT);
					txt->SetFixedWidth(_tcsclen(buffer)+1);
					txt->SetText(buffer);
					txt->SetXY(140,Y);
					txt->SetFGColor(DFTeamColors[TeamRank[i]]);
					txt->SetFont(win->Font_);
					txt->SetClient(2);
					txt->SetFlagBitOn(C_BIT_RIGHT);
					win->AddControl(txt);

					_stprintf(buffer,"%1d/%1d", TeamTotals[TeamRank[i]][0],TeamTotals[TeamRank[i]][1]);

					txt=new C_Text;
					txt->Setup(C_DONT_CARE,C_TYPE_CENTER);
					txt->SetFixedWidth(_tcsclen(buffer)+1);
					txt->SetText(buffer);
					txt->SetXY(200,Y);
					txt->SetFGColor(DFTeamColors[TeamRank[i]]);
					txt->SetFont(win->Font_);
					txt->SetClient(2);
					txt->SetFlagBitOn(C_BIT_HCENTER);
					win->AddControl(txt);

					Y+=gFontList->GetHeight(win->Font_);
				}
			}
		}
		else
		{
			win->DisableCluster(100);
			win->EnableCluster(200);
		}

		// Show individual scores
		Y = 0;
		cur = SortedPilotList;
		while (cur)
			{
			// Add all the text she-it
			txt=new C_Text;
			txt->Setup(C_DONT_CARE,C_TYPE_LEFT);
			txt->SetFixedWidth(_tcsclen(cur->pilot_data->pilot_name)+1);
			txt->SetText(cur->pilot_data->pilot_name);
			txt->SetXY(0,Y);
			txt->SetFGColor(DFTeamColors[cur->team]);
			txt->SetFont(win->Font_);
			txt->SetClient(static_cast<short>(PlayerClient));
			txt->SetFlagBitOn(C_BIT_LEFT);
			win->AddControl(txt);
			
			_stprintf(buffer,"%1d",cur->pilot_data->score);

			txt=new C_Text;
			txt->Setup(C_DONT_CARE,C_TYPE_RIGHT);
			txt->SetFixedWidth(_tcsclen(buffer)+1);
			txt->SetText(buffer);
			txt->SetXY(140,Y);
			txt->SetFGColor(DFTeamColors[cur->team]);
			txt->SetFont(win->Font_);
			txt->SetClient(static_cast<short>(PlayerClient));
			txt->SetFlagBitOn(C_BIT_RIGHT);
			win->AddControl(txt);
		
			kills = cur->pilot_data->aa_kills;

//			for (i=0,kills=0;i<MAX_DOGFIGHT_TEAMS;i++)
//				kills += cur->pilot_data->kills[i][VS_AI] + cur->pilot_data->kills[i][VS_HUMAN];

			_stprintf(buffer,"%1d/%1d", kills, cur->pilot_data->deaths[VS_AI] + cur->pilot_data->deaths[VS_HUMAN]);

			txt=new C_Text;
			txt->Setup(C_DONT_CARE,C_TYPE_CENTER);
			txt->SetFixedWidth(_tcsclen(buffer)+1);
			txt->SetText(buffer);
			txt->SetXY(200,Y);
			txt->SetFGColor(DFTeamColors[cur->team]);
			txt->SetFont(win->Font_);
			txt->SetClient(static_cast<short>(PlayerClient));
			txt->SetFlagBitOn(C_BIT_HCENTER);
			win->AddControl(txt);

			Y += gFontList->GetHeight(win->Font_);

			cur = cur->next;
			}		

		win->ScanClientAreas();

		// Play the bite and clean up our lists
		PlayDogfightBite();
		ClearSortedPilotList();
	}

	// Tell the dogfight manager it's ok to clear the scores
	SimDogfight.SetLocalFlag(DF_VIEWED_SCORES);
}

void SaveDogfightResults(char *)
{
}
