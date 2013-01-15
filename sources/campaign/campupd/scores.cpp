#include "stdhdr.h"
#include "dogfight.h"
#include "initData.h"
#include "simbase.h"
#include "campwp.h"
#include "camp2sim.h"
#include "loadout.h"
#include "entity.h"
#include "simveh.h"
#include "sms.h"
#include "SimDrive.h"
#include "mission.h"
#include "classtbl.h"
#include "otwdrive.h"
#include "MsgInc\SendDogfightInfo.h"
#include "MsgInc\RequestDogfightInfo.h"
#include "MsgInc\RegenerationMsg.h"

// OW FIXME: this had to be added after installing the DX8 Beta 1 SDK
//Retro_dead 15Jan2004	#define DIRECTINPUT_VERSION 0x0700
//Retro_dead 15Jan2004	#include "dinput.h"

#include "falcsess.h"
#include "uicomms.h"
#include "aircrft.h"
#include "CampList.h"
#include "Flight.h"
#include "Mission.h"
#include "CampWP.h"
#include "SimMover.h"
#include "MissEval.h"
#include "GameMgr.h"
#include "TimerThread.h"
#include "Team.h"
#include "teamdata.h"
#include "tac_class.h"


extern tactical_mission
	*current_tactical_mission;

// Scoring Variables
// Instant action
// Dogfight
// Tactical Engagement
long	gRefreshScoresList=0;
long	gScoreColor[10];
_TCHAR	gScoreName[10][30];
_TCHAR	gScorePoints[10][10];
// ===========================================
// Global Functions
// ===========================================
//
// Dogfight scoring routine
void MakeDogfightTopTen (int mode)
	{
	FlightDataClass		*flight_ptr;
	PilotDataClass		*pilot_data;
	int					i,j,worst,numscores;
	int					tmps,score[10] = {0};				// Top 10 scores
	int					active[10] = {0};
	long tmpclr;
	_TCHAR				tmpn[30];		// Top 10 names

	// Clear Global Text Strings
	memset(gScoreName,0,sizeof(_TCHAR)*10*30);
	memset(gScorePoints,0,sizeof(_TCHAR)*10*10);
	memset(gScoreColor,0,sizeof(long)*10);

	numscores=0;

	if (mode == dog_TeamMatchplay)
		{
		short	tscore[MAX_DOGFIGHT_TEAMS] = {0};

		// Determine active teams
		flight_ptr = TheCampaign.MissionEvaluator->flight_data;
		while (flight_ptr)
			{
			if (flight_ptr->flight_team)
				active[flight_ptr->flight_team - 1]++;
			flight_ptr = flight_ptr->next_flight;
			}

		TheCampaign.MissionEvaluator->GetTeamScore(tscore);
		for (i=1; i<MAX_DOGFIGHT_TEAMS; i++)
			{
			if (active[i-1])
				{
				score[i-1] = tscore[i];
				strcpy(gScoreName[i-1],TeamInfo[i]->GetName());
				}
			}
		}
	else
		{
		// Check out all players/Teams
		flight_ptr = TheCampaign.MissionEvaluator->flight_data;
		while (flight_ptr)
		{
			pilot_data = flight_ptr->pilot_list;
			while (pilot_data)
			{
				if (mode == dog_Furball)
				{
					// Individual mode, collect 10 highest scores (we'll sort later)
					if(numscores < 10)
					{
						score[numscores] = pilot_data->score;
						active[numscores] = 1;
						strcpy(gScoreName[numscores],pilot_data->pilot_callsign);
					}
					else
					{
						for (i=1,worst=0; i < 10; i++)
						{
							if (score[i] < score[worst])
								worst = i;
						}
						if (score[worst] < pilot_data->score)
						{
							score[worst] = pilot_data->score;
							active[worst] = 1;
							strcpy(gScoreName[worst],pilot_data->pilot_callsign);
						}
					}
				}
				else if (mode == dog_TeamFurball)
				{
					// Team mode, collect all 4 team scores (we'll sort later)
					score[flight_ptr->flight_team] += pilot_data->score;
					active[flight_ptr->flight_team]++;
					strcpy(gScoreName[flight_ptr->flight_team],TeamInfo[flight_ptr->flight_team]->GetName());
				}
				numscores++;
				pilot_data = pilot_data->next_pilot;
			}
			flight_ptr = flight_ptr->next_flight;
		}
		}

	// Sort by points
	for(i=1;i<10;i++)
	{
		if (active[i])
		{
			for(j=0;j<i;j++)
			{
				if(score[i] > score[j] || !active[j])
				{
					_tcscpy(tmpn,gScoreName[i]);
					tmps=score[i];
					tmpclr=gScoreColor[i];

					_tcscpy(gScoreName[i],gScoreName[j]);
					score[i]=score[j];
					gScoreColor[i]=gScoreColor[j];

					_tcscpy(gScoreName[j],tmpn);
					score[j]=tmps;
					gScoreColor[j]=tmpclr;

					tmps=active[i];
					active[i] = active[j];
					active[j] = tmps;
				}
			}
		}
	}
	for (i=0; i<10; i++)
		_stprintf(gScorePoints[i],"%1ld",score[i]);
}


void MakeTacEngScoreList()
{
	_TCHAR tmpn[30];
	long tmps,score[NUM_TEAMS],tmpclr;
	short i,j,idx;

	// Clear Global Text Strings
	memset(gScoreName,0,sizeof(_TCHAR)*10*30);
	memset(gScorePoints,0,sizeof(_TCHAR)*10*10);
	memset(gScoreColor,0,sizeof(long)*10);

	current_tactical_mission->calculate_victory_points();
	idx=0;
	for(i=1;i<NUM_TEAMS;i++)
	{
		if(TeamInfo[i])
		{
			_tcscpy(gScoreName[idx],TeamInfo[i]->GetName());
			score[idx]=current_tactical_mission->get_team_points(i);
			gScoreColor[idx]=TeamInfo[i]->GetColor();
			idx++;
		}
	}

	// Sort by points
	for(i=1;i < idx;i++)
	{
		for(j=0;j<i;j++)
		{
			if(score[j] < score[i])
			{
				_tcscpy(tmpn,gScoreName[i]);
				tmps=score[i];
				tmpclr=gScoreColor[i];

				_tcscpy(gScoreName[i],gScoreName[j]);
				score[i]=score[j];
				gScoreColor[i]=gScoreColor[j];

				_tcscpy(gScoreName[j],tmpn);
				score[j]=tmps;
				gScoreColor[j]=tmpclr;
			}
		}
	}
	for (i=0; i<idx; i++)
		_stprintf(gScorePoints[i],"%1ld",score[i]);
}