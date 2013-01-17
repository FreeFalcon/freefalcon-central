#include <stddef.h>
#include <fcntl.h>
#include <io.h>
#include "campmap.h"
#include "CmpGlobl.h"
#include "team.h"
#include "falcmesg.h"
#include "cmpClass.h"
#include "Dispcfg.h"
#include "Falcuser.h"
#include "entity.h" // 2002-04-20 MN for DDP datafile

// =========================================================================
// Includes all routines for determaining current state of relations between
// two countries
// =========================================================================

// This returns the team a country belongs to
Team GetTeam(Control country)
{
	if ((country < NUM_TEAMS) && (TeamInfo[country] != NULL)){
		return TeamInfo[country]->cteam;
	}
	else {
		return country;
	}
}

// Returns relations between a two countries
int GetCCRelations (Control who, Control with)
{
	if (TeamInfo[GetTeam(who)])
	{
		return TeamInfo[GetTeam(who)]->CStance(with);
	}
	else
	{
		return 0;
	}
}

int GetCTRelations (Control who, Team with)
{
	if (TeamInfo[GetTeam(who)])
	{
		return TeamInfo[GetTeam(who)]->TStance(with);
	}
	else
	{
		return 0;
	}
}

int GetTTRelations (Team who, Team with)
{
	if (TeamInfo[who])
	{
		return TeamInfo[who]->TStance(with);
	}
	else
	{
		return 0;
	}
}

int GetTCRelations (Team who, Control with)
{
	if (TeamInfo[who])
	{
		return TeamInfo[who]->CStance(with);
	}
	else
	{
		return 0;
	}
}

// Sets a country to a team
void SetTeam (Control country, int team)
{
	int i;

	// Leave the old teams
	for (i=0; i<NUM_TEAMS; i++){
		if (i != team && TeamInfo[i] && TeamInfo[i]->member[country]){
		    TeamInfo[i]->member[country] = 0;
		}
	}
	ShiAssert(team > 0 && team < NUM_TEAMS);
	TeamInfo[country]->cteam = (Team)team;
    TeamInfo[team]->member[country] = 1;
	//TeamInfo[team]->MakeTeamDirty (DIRTY_TEAM_RELATIONS, DDP[10].priority);
	TeamInfo[team]->MakeTeamDirty (DIRTY_TEAM_RELATIONS, SEND_NOW);
	TheCampaign.MakeCampMap(MAP_OWNERSHIP);
	PostMessage(FalconDisplay.appWin,FM_REFRESH_CAMPMAP,0,0);
}

void SetTTRelations (Team who, Team with, int rel)
{
	TeamInfo[who]->stance[with] = (short)rel;
	//TeamInfo[who]->MakeTeamDirty (DIRTY_TEAM_RELATIONS, DDP[11].priority);
	TeamInfo[who]->MakeTeamDirty (DIRTY_TEAM_RELATIONS, SEND_NOW);
}

// Sets a country's relations to a team
void SetCTRelations (Control who, Team with, int rel)
	{
	SetTTRelations(GetTeam(who),with,rel);
	}

