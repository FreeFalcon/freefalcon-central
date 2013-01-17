//
// Tactool.cpp
//
// Converts tactics and reaction files from text to binary
//

#define __MEMMGR_H__		// Don't use the memory manager
#define NO_MALLOC_MACRO

#include <stdio.h>
#include <conio.h>
#include <stddef.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include "cmpglobl.h"
#include "tactics.h"

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
//ReactData ReactionTable[WP_LAST];				// List of reaction priorities

// ===============================
// Global functions
// ===============================

void ReadComments (FILE* fh)
	{
	int					c;

	c = fgetc(fh);
	while (c == '\n')
		c = fgetc(fh);
	while (c == '/' && !feof(fh))
		{
		c = fgetc(fh);
		while (c != '\n' && !feof(fh))
			c = fgetc(fh);
		while (c == '\n')
			c = fgetc(fh);
		}
	ungetc(c,fh);
	}

/*
int SaveReactionTable (char* filename)
	{
	FILE*			fp;
	int			fc;

	fp = fopen(filename, "wb");
	if (!fp)
		return -1;
	fc = fwrite(ReactionTable,sizeof(ReactData),WP_LAST,fp);
	fclose(fp);
	return 0;
	}
*/

int SaveTactics (char* filename)
	{
	FILE*			fp;
	int			fc;

	fp = fopen(filename, "wb");
	if (!fp)
		return -1;
	fc = fwrite(&AirTactics,sizeof(short),1,fp);
	fc = fwrite(&GroundTactics,sizeof(short),1,fp);
	fc = fwrite(&NavalTactics,sizeof(short),1,fp);
	fwrite(TacticsTable,sizeof(TacticData),TotalTactics,fp);
	fclose(fp);
	return 0;
	}

/*
int LoadReactionTableText (char* filename)
	{
	int					i,x,dez,daz,raz,fc;
	FILE*					fh;
	char					buffer[30],comment=0;

	fh = fopen(filename,"r");
	if (fh==NULL)
	  return -1;

	// Read Number of entries
	fc = fscanf(fh,"%d\n",&x);
	while (comment != '\n')
		comment = fgetc(fh);
	if (fc < 0 || x < 1)
		return -1;

	for (i=0; i<x; i++)
		{
		fc = fscanf(fh, "%s %d",buffer,&dez);
		fc = fscanf(fh, "%d %d %d", &dez, &daz, &raz);
		ReactionTable[i].reaction[REACT_DETECTED] = dez;
		ReactionTable[i].reaction[REACT_DANGER] = daz;
		ReactionTable[i].reaction[REACT_IN_RANGE] = raz;
		}
	fclose(fh);
	return 1;
	}
*/

int LoadTacticsTableText (char* filename)
	{
	FILE*			fh;
	char			buffer[40];
	int				i,j,k,l,n,m,r,a,id,fc;
	
	fh = fopen(filename,"r");
	if (fh==NULL)
	  return -1;

	// Read Number of entries
	ReadComments(fh);
	fc = fscanf(fh,"%d %d %d\n",&AirTactics,&GroundTactics,&NavalTactics);
	FirstAirTactic = 1;
	FirstGroundTactic = AirTactics;
	FirstNavalTactic = AirTactics + GroundTactics;
	TotalTactics = 1 + AirTactics + GroundTactics + NavalTactics;
//	TacticsTable = (TacticData*) malloc(sizeof(TacticData)*TotalTactics);
	TacticsTable = new TacticData[TotalTactics];

	for (id=0; id<TotalTactics; id++)
		{
		ReadComments(fh);
		fscanf(fh, "%s", buffer);
		sprintf(TacticsTable[id].name,buffer);
		fscanf(fh, "%d %d %d %d %d %d %d", &n, &m, &i, &j, &k, &l, &r);
		TacticsTable[id].id = id;
		TacticsTable[id].team = m;
		TacticsTable[id].domainType = i;
		TacticsTable[id].unitSize = j;		
		TacticsTable[id].minRangeToDest = k;
		TacticsTable[id].maxRangeToDest = l;
		TacticsTable[id].distToFront = r;
		ReadComments(fh);
		fc = fscanf(fh, "%d %d %d %d %d %d %d %d", &i, &j, &n, &l, &r, &k, &a, &m);
		TacticsTable[id].broken = i;
		TacticsTable[id].engaged = j;
		TacticsTable[id].combat = n;
		TacticsTable[id].losses = l;
		TacticsTable[id].retreating = r;
		TacticsTable[id].owned = k;
		TacticsTable[id].airborne = a;
		TacticsTable[id].marine = m;
		ReadComments(fh);
		for (i=0; i<10; i++)
			{
			fc = fscanf(fh, "%d", &n);
			TacticsTable[id].actionList[i] = n;
			}
		ReadComments(fh);
		fc = fscanf(fh, "%d %d %d %d %d", &i, &r, &j, &n, &m);
		TacticsTable[id].minOdds = i;
		TacticsTable[id].role = r;
		TacticsTable[id].fuel = j;
		TacticsTable[id].weapons = n;
		TacticsTable[id].priority = m;
		fc = fscanf(fh, "%d %d", &i, &j);
		TacticsTable[id].formation = i;
		TacticsTable[id].special = j;
		}
	fclose(fh);
	return 1;
	}

void FreeTactics (void)
	{
	free(TacticsTable);
	TacticsTable = NULL;
	}

int main()
	{
	char			filename[80] = "Falcon4";
	char			extname[80];

/*	sprintf(extname,"%s.RTT",filename);
	LoadReactionTableText(extname);	
	sprintf(extname,"%s.RT",filename);
	SaveReactionTable(extname);
*/

	sprintf(extname,"%s.TTT",filename);
	LoadTacticsTableText(extname);	
	sprintf(extname,"%s.TT",filename);
	SaveTactics(extname);
	return 0;
	}

