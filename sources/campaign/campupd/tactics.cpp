// ***************************************************************************
// Tactics.cpp
//
// Stuff used to deal with unit RoE and tactics
// ***************************************************************************

#include <stdio.h>
#include <conio.h>
#include <stddef.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include "cmpglobl.h"
#include "campwp.h"
#include "tactics.h"
#include "f4find.h"

// ===============================
// Reaction Tables
// ===============================

/*
ReactData	AirReactionTable[WP_LAST][REACT_LAST_BASIC + ARO_OTHER];
ReactData	GroundReactionTable[GORD_LAST][REACT_LAST_BASIC + GORD_LAST];
ReactData	NavalReactionTable[NORD_OTHER][REACT_LAST_BASIC + NORD_OTHER];
ReactData	ObjReactionTable[REACT_LAST_BASIC];
*/
#define CHECK_ANY		255

//ReactData ReactionTable[WP_LAST][WP_LAST];	// List of reaction priorities
ReactData ReactionTable[WP_LAST];	// List of reaction priorities

// =========================
// Tactics Indexes and Table
// =========================

short AirTactics = 0;
short GroundTactics = 0;
short NavalTactics = 0;
short FirstAirTactic = 0;
short FirstGroundTactic = 0;
short FirstNavalTactic = 0;
short TotalTactics = 0;

TacticData *TacticsTable = NULL;

// ===============================
// Reaction Stuff
// ===============================

int CheckReaction(int awp, int zone)
	{
	return ReactionTable[awp].reaction[zone];
	}


// ===================================
// Tactics stuff
// ===================================

int LoadTactics (char* name)
	{
	FILE*			fp;
	char			filename[MAX_PATH];

	sprintf(filename,"%s\\%s.tt",FalconCampaignSaveDirectory,name);
	if ((fp = F4OpenFile(filename, "rb")) == NULL)
		return 0;
	fread(&AirTactics,sizeof(short),1,fp);
	fread(&GroundTactics,sizeof(short),1,fp);
	fread(&NavalTactics,sizeof(short),1,fp);
	FirstAirTactic = 1;
	FirstGroundTactic = 1 + AirTactics;
	FirstNavalTactic = 1 + AirTactics + GroundTactics;
	TotalTactics = 1 + AirTactics + GroundTactics + NavalTactics;
	TacticsTable = new TacticData[TotalTactics];
	fread(TacticsTable,sizeof(TacticData),TotalTactics,fp);
	fclose(fp);
	return 1;
	}

void FreeTactics (void)
	{
	delete [] TacticsTable;
	TacticsTable = NULL;
	}

int CheckTeam(int tid, int team)
	{
	if (TacticsTable[tid].team == CHECK_ANY)
		return 1;
	if (TacticsTable[tid].team != team)
		return 0;
	return 1;
	}

int CheckUnitType(int tid, int domain, int type)
	{
	if (TacticsTable[tid].domainType == CHECK_ANY)
		return 1;
	if (TacticsTable[tid].domainType != domain || TacticsTable[tid].unitSize != type)
		return 0;
	return 1;
	}

int CheckRange(int tid, int rng)
	{
	if (TacticsTable[tid].minRangeToDest == CHECK_ANY)
		return 1;
	if (TacticsTable[tid].minRangeToDest > rng || TacticsTable[tid].maxRangeToDest < rng)
		return 0;
	return 1;
	}

int CheckDistToFront(int tid, int dist)
	{
	if (TacticsTable[tid].distToFront == CHECK_ANY)
		return 1;
	if (TacticsTable[tid].distToFront > dist)
		return 0;
	return 1;
	}

int CheckAction(int tid, int act)
	{
	int		i;

	if (TacticsTable[tid].actionList[0] == CHECK_ANY)
		return 1;
	for (i=0; i<10 && TacticsTable[tid].actionList[i] != CHECK_ANY; i++)
		{
		if (TacticsTable[tid].actionList[i] == act)
			return 1;
		}
	return 0;
	}

int CheckStatus(int tid, int status)
	{
	if (TacticsTable[tid].broken  == CHECK_ANY)
		return 1;
	if (!TacticsTable[tid].broken && status)
		return 0;
	if (TacticsTable[tid].broken && !status)
		return 0;
	return 1;
	}

int CheckLosses(int tid, int losses)
	{
	if (TacticsTable[tid].losses  == CHECK_ANY)
		return 1;
	if (!TacticsTable[tid].losses  && losses)
		return 0;
	if (TacticsTable[tid].losses && !losses)
		return 0;
	return 1;
	}

int CheckEngaged(int tid, int engaged)
	{
	if (TacticsTable[tid].engaged  == CHECK_ANY)
		return 1;
	if (!TacticsTable[tid].engaged && engaged)
		return 0;
	if (TacticsTable[tid].engaged && !engaged)
		return 0;
	return 1;
	}

int CheckCombat(int tid, int combat)
	{
	if (TacticsTable[tid].combat  == CHECK_ANY)
		return 1;
	if (!TacticsTable[tid].combat && combat)
		return 0;
	if (TacticsTable[tid].combat && !combat)
		return 0;
	return 1;
	}

int CheckRetreating(int tid, int retreat)
	{
	if (TacticsTable[tid].retreating == CHECK_ANY)
		return 1;
	if (!TacticsTable[tid].retreating && retreat)
		return 0;
	if (TacticsTable[tid].retreating && !retreat)
		return 0;
	return 1;
	}

int CheckOwned(int tid, int o)
	{
	if (TacticsTable[tid].owned == CHECK_ANY)
		return 1;
	if (!TacticsTable[tid].owned && o)
		return 0;
	if (TacticsTable[tid].owned && !o)
		return 0;
	return 1;
	}

int CheckAirborne(int tid, int airborne)		// These two need some thought
	{
	if (TacticsTable[tid].airborne == CHECK_ANY)
		return 1;
	if (!TacticsTable[tid].airborne && airborne)
		return 0;
	if (TacticsTable[tid].airborne && !airborne)
		return 0;
	return 1;
	}

int CheckMarine(int tid, int marine)			//
	{
	if (TacticsTable[tid].marine == CHECK_ANY)
		return 1;
	if (!TacticsTable[tid].marine && marine)
		return 0;
	if (TacticsTable[tid].marine && !marine)
		return 0;
	return 1;
	}

int CheckOdds(int tid, int odds)
	{
	if (TacticsTable[tid].minOdds == CHECK_ANY)
		return 1;
	if (TacticsTable[tid].minOdds > odds)
		return 0;
	return 1;
	}

int CheckRole(int tid, int role)
	{
	if (TacticsTable[tid].role == CHECK_ANY)
		return 1;
	if (TacticsTable[tid].role != role)
		return 0;
	return 1;
	}

int CheckSpecial(int tid)
	{
	return TacticsTable[tid].special;
	}

int CheckFuel(int tid, int fuel)
	{
	if (TacticsTable[tid].fuel == CHECK_ANY)
		return 1;
	if (TacticsTable[tid].fuel > fuel)
		return 0;
	return 1;
	}

int CheckWeapons(int tid)
	{
	return TacticsTable[tid].weapons;
	}

int GetTacticPriority(int tid)
	{
	return TacticsTable[tid].priority;
	}

int GetTacticFormation(int tid)
	{
	return TacticsTable[tid].formation;
	}

