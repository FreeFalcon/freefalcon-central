// 
// Campevent conversion file
//

#define EVENT_MAKER 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "F4vu.h"
#include "CmpGlobl.h"
#include "CmpEvent.h"
#include "Team.h"
#include "F4Find.h"
#include "..\..\Campaign\CampUpd\CmpEvent.cpp"

int LoadCampaignEventsText (char* filename);
int SaveCampaignEvents(char* filename);
FILE* OpenCampFile (char *filename, char *ext, char *mode);

VU_ID FalconNullId;

extern EventClass**	CampEvents;
extern short		CE_Events;

// =====================
// Main
// =====================

int main (void)
	{
	LoadCampaignEventsText("Events.txt");
	SaveCampaignEvents("Falcon4");
	return 1;
	}

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

char* ReadName (FILE *fp, char name[], int len)
	{
	char buffer[80];
	char *sptr;

	fgets(buffer,80,fp);
	strncpy(name,buffer,len);
	if (name[len-1])
		name[len-1] = 0;
	sptr = strchr(name,'\n');
	if (sptr)
		*sptr = '\0';
	return name;
	}
		
int LoadCampaignEventsText (char* filename)
	{
	FILE*		fp;
	int			i,j,events,done,trignum;
	char		trigtype[20];

	if (CampEvents != NULL)
		F4FreeMemory(CampEvents);
	CE_Events = 0;
//	sprintf(eventfile,"%s.evt",filename);
	if ((fp = fopen(filename,"r+")) == NULL)
		return 0;
	
	ReadComments(fp);
	fscanf(fp,"%d\n",&CE_Events);
	if (CE_Events > 0)
		CampEvents = (EventClass**) F4AllocMemory(sizeof(EventClass*)*CE_Events);
	else
		{
		CampEvents = NULL;
		return 0;
		}
	
	events = CE_Events;
	while (events)
		{
		ReadComments(fp);
		fscanf(fp,"%d\n",&i);
		CampEvents[i] = new EventClass(i);
		ReadComments(fp);
		fscanf(fp,"%s %d %d %d %d %d\n",trigtype,&CampEvents[i]->priority,&CampEvents[i]->data[0],
			&CampEvents[i]->data[1],&CampEvents[i]->data[2],&CampEvents[i]->data[3]);
		sprintf(CampEvents[i]->name,trigtype);
		done = 0;
		while (!done)
			{
			ReadComments(fp);
			fscanf(fp,"%s %d", trigtype, &trignum);
			if (strstr(trigtype,"AND"))
				{
				CampEvents[i]->and_trigs = trignum;
				CampEvents[i]->and_triggers = (Trigger*) F4AllocMemory(sizeof(Trigger) * trignum);
				for (j=0; j<trignum; j++)
					{
					CampEvents[i]->and_triggers[j].flags = 0;
					fscanf(fp,"%s %d %d %d %d\n",CampEvents[i]->and_triggers[j].type,&CampEvents[i]->and_triggers[j].data[0],
						&CampEvents[i]->and_triggers[j].data[1],&CampEvents[i]->and_triggers[j].data[2],&CampEvents[i]->and_triggers[j].data[3]);
					}
				}
			if (strstr(trigtype,"OR"))
				{
				CampEvents[i]->or_trigs = trignum;
				CampEvents[i]->or_triggers = (Trigger*) F4AllocMemory(sizeof(Trigger) * trignum);
				for (j=0; j<trignum; j++)
					{
					CampEvents[i]->or_triggers[j].flags = 0;
					fscanf(fp,"%s %d %d %d %d\n",CampEvents[i]->or_triggers[j].type,&CampEvents[i]->or_triggers[j].data[0],
						&CampEvents[i]->or_triggers[j].data[1],&CampEvents[i]->or_triggers[j].data[2],&CampEvents[i]->or_triggers[j].data[3]);
					}
				}
			if (strstr(trigtype,"END"))
				done = 1;
			}
		events--;
		}
	fclose(fp);
	return 1;
	}
// Stubs

int GetTTRelations(uchar a, uchar b)
	{
	return 1;
	}

void SetTTRelations(uchar a, uchar b, int c)
	{
	}
