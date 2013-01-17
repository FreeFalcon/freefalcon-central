/**********************************************************************************
*
* cmpevent.cpp
*
* Campaign event manager
*
***********************************************************************************/

#include <io.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "Falclib.h"
#include "cmpglobl.h"
#include "cmpevent.h"
#include "team.h"
#include "atm.h"
#include "f4find.h"
#include "CUIEvent.h"
#include "Campaign.h"
#include "CmpClass.h"
#include "Find.h"
#include "Debuggr.h"
#include "Team.h"
#include "Brief.h"
#include "GTMObj.h"
#include "falcsess.h"
#include "DispCfg.h"
#include "FalcUser.h"
#include "PlayerOp.h"
#include "MsgInc/CampEventDataMsg.h"

//sfr: for checks
#include "InvalidBufferException.h"

#include "debuggr.h"

EventClass**	CampEvents=NULL;
short			CE_Events=0;

#define	CE_MAX_TRIGGERED		3

// ============================ 
// External Function Prototypes
// ============================

extern void UI_AddMovieToList(long ID,long timestamp,_TCHAR *Description);
extern void ReadComments (FILE* fh);
extern char* ReadToken (FILE *fp, char name[], int len);
extern char* ReadMemToken (char **data, char name[], int len);

// ================================
// External variables 2002-04-17 MN
// ================================

extern float FLOTDrawDistance;
extern int FLOTSortDirection;
extern int TheaterXPosition;
extern int TheaterYPosition;

// ============================
// Other prototypes
// ============================

int ReadScriptedTriggerFile (char* scenario);

// =========================================
// Event Class
// =========================================

EventClass::EventClass (short id)
{
	event = id;
	flags = 0;
}

EventClass::EventClass (FILE* file)
{
	if (!file)
		return;
	fread(&event, sizeof(short), 1, file);		
	fread(&flags, sizeof(short), 1, file);	
}

EventClass::EventClass (uchar **stream, long *rem)
{
	if ((rem <= 0) || (!stream)){
		return;
	}

	memcpychk(&event, stream, sizeof(short), rem);
	memcpychk(&flags, stream, sizeof(short), rem);
}

EventClass::~EventClass (void)
{
}

int EventClass::Save (FILE* file)
{
	if (!file)
		return 0;
	fwrite(&event, sizeof(short), 1, file);		
	fwrite(&flags, sizeof(short), 1, file);	
	return 1;
}

void EventClass::DoEvent (void)
{
	SetEvent(1);
	MonoPrint("CampEvent: Event %d activated.\n",event);
}

void EventClass::SetEvent (int status)
{
	CampEventDataMessage	*msg = new CampEventDataMessage(vuLocalSession,FalconLocalGame);
	msg->dataBlock.message = CampEventDataMessage::eventMessage;
	msg->dataBlock.event = event;
	if (status)
	{
		flags |= CE_FIRED;
		msg->dataBlock.status = 1;
	}
	else
	{
		flags &= ~CE_FIRED;
		msg->dataBlock.status = 0;
	}
	FalconSendMessage(msg,TRUE);
}


// ======================================
// Global functions
// ======================================

int CheckTriggers(char *scenario)
{
	if (!FalconLocalGame || !FalconLocalGame->IsLocal())
		return 0;

	if (FalconLocalSession->GetTeam() == 255)
		return 0;

	ReadScriptedTriggerFile(scenario);
	return 0;
}

int ReadNumberOfEvents (char* scenario)
{
	char		token[121];
	int			done=0;
	FILE		*fp;

	CE_Events = 0;
	if ((fp = OpenCampFile(scenario,"tri","r")) == NULL)
		return 0;
	while (!done)
	{
		ReadComments(fp);
		ReadToken(fp,token,120);
		if (!token[0])
			continue;
		if (strncmp(token,"#TOTAL_EVENTS",13)==0){
			char *sptr = strchr(token,' ');
			if (sptr)
				sptr++;
			CE_Events = atoi(sptr);
			done = 1;
		}
	}
	CloseCampFile (fp);
	if (CampEvents != NULL)
		delete [] CampEvents;

	CampEvents = new EventClass*[CE_Events];
	return CE_Events;
}

void SetInitialEvents (char* scenario)
	{
	FILE*		fp;
	char		token[121];
	int			done = 0;

	if ((fp = OpenCampFile(scenario,"tri","r")) == NULL)
		return;
	while (!done)
		{
		ReadComments(fp);
		ReadToken(fp,token,120);
		if (!token[0])
			continue;
		if (strncmp(token,"#SET_EVENT",10)==0)
			{
			char	*sptr = strchr(token,' ');
			int		i = 0;
			if (sptr)
				sptr++;
			i = atoi(sptr);
			CampEvents[i]->SetEvent(1);
			}
		else if (strncmp(token,"#RESET_EVENT",10)==0)
			{
			char	*sptr = strchr(token,' ');
			int		i = 0;
			if (sptr)
				sptr++;
			i = atoi(sptr);
			CampEvents[i]->SetEvent(0);
			}
		else if (strncmp(token,"#SET_TEMPO",10)==0)
			{
/*			char	*sptr;
			if (sptr = strchr(token,' '))
				sptr++;
			TheCampaign.Tempo = atoi (sptr);
*/
			}
		else if (strncmp(token,"#CHANGE_PRIORITIES",18)==0)
			{
/*			char	*sptr;
			int		team,i;
			if (sptr = strchr(token,' '))
				sptr++;
			team = atoi(sptr);
			if (sptr = strchr(sptr,' '))
				sptr++;
			i = atoi(sptr);
			if (TeamInfo[team])
				TeamInfo[team]->ReadPriorityFile(i);
*/
			}
		if (strcmp(token,"#ENDINIT")==0)
			done = 1;
		}
	CloseCampFile(fp);
	}


void ReadSpecialCampaignData (char* scenario)
	{
	FILE*		fp;
	char		token[121];
	int			done = 0;

	// Initialise variables to default values
	TheaterXPosition = Map_Max_X/2;
	TheaterYPosition = Map_Max_Y/2;
	FLOTSortDirection = 0;
	FLOTDrawDistance = 50.0f;

	if ((fp = OpenCampFile(scenario,"tri","r")) == NULL)
		return;
	while (!done)
		{
		ReadComments(fp);
		ReadToken(fp,token,120);
		if (!token[0])
			continue;
// 2002-04-17 MN these have originally been read from Falcon4.AII - but now from trigger files so we can
// set them individually for each campaign :-)
		if (strncmp(token,"#BULLSEYE_X",11)==0) // bullseye reference point X position
			{
			char	*sptr = strchr(token,' ');
			int		i = 0;
			if (sptr)
				sptr++;
			i = atoi(sptr);
			TheaterXPosition = i;
			}
		else if (strncmp(token,"#BULLSEYE_Y",11)==0) // bullseye reference point Y position
			{
			char	*sptr = strchr(token,' ');
			int		i = 0;
			if (sptr)
				sptr++;
			i = atoi(sptr);
			TheaterYPosition = i;
			}
		else if (strncmp(token,"#FLOT_SORTDIRECTION",19)==0) // 0 = West-East, 1 = North-South
			{
			char	*sptr = strchr(token,' ');
			int		i = 0;
			if (sptr)
				sptr++;
			i = atoi(sptr);
			FLOTSortDirection = i;
			}
		else if (strncmp(token,"#FLOT_DRAWDISTANCE",11)==0) // bullseye reference point X position
			{
			char	*sptr = strchr(token,' ');
			float		i = 0.0f;
			if (sptr)
				sptr++;
			i = (float)atof(sptr);
			FLOTDrawDistance = i;
			}
		if (strcmp(token,"#ENDINIT")==0)
			done = 1;
		}
	CloseCampFile(fp);
	}


int NewCampaignEvents (char* scenario)
	{
	// Read in and allocate the event database
	ReadNumberOfEvents(scenario);
	for (int i=0; i<CE_Events; i++)
		CampEvents[i] = new EventClass(i);
	// Set any events we want initially flagged
	SetInitialEvents(scenario);
	return 1;
	}

// Load both events and triggers
int LoadCampaignEvents(char* filename, char* scenario)
	{
	uchar			/* *data,*/ *data_ptr;
	short			i,events;

	ReadNumberOfEvents(scenario);
	CampaignData cd = ReadCampFile (filename, "evt");
	if (cd.dataSize == -1){
		return 0;
	}

	data_ptr = (uchar *)cd.data;

	events = *((short*)data_ptr);
	data_ptr += sizeof(short);
	long dataSize = cd.dataSize - sizeof(short);

	if (events > CE_Events)
		events = CE_Events;

	for (i=0; i<events; i++)
		CampEvents[i] = new EventClass(&data_ptr, &dataSize);

	for (;i<CE_Events; i++)
		CampEvents[i] = new EventClass(i);

	delete cd.data;
	return 1;
	}

// Save both events and triggers
int SaveCampaignEvents(char* filename)
	{
	FILE*		fp;
	int			i;

	if ((fp = OpenCampFile (filename, "evt", "wb")) == NULL)
		return 0;
	if (CampEvents)
	{
		fwrite(&CE_Events,sizeof(short),1,fp);
		for (i=0; i<CE_Events; i++)
			CampEvents[i]->Save(fp);
	}
	else
	{
		CE_Events = 0;
		fwrite (&CE_Events, sizeof (short), 1, fp);
	}
	CloseCampFile(fp);	
	return 1;
	}

void DisposeCampaignEvents (void)
	{
	int			i;

	if (!CampEvents || !CE_Events)
		return;
	for (i=0; i<CE_Events; i++)
		delete CampEvents[i];;
	delete [] CampEvents;
	CampEvents = NULL;
	}



















int ReadScriptedTriggerFile (char* filename)
{
	FILE*		fp;
	int			i,done=0,initdone=0,curr_stack=0,stack_active[MAX_STACK] = { 1 };
	char		token[128],*sptr;
	_TCHAR		eol[2] = { '\n', 0 };
	Objective	o;
	Team		team;

	if ((fp = OpenCampFile(filename,"tri","r")) == NULL)
	{
		ShiAssert ( 0 );
		return 0;
	}

	ShiAssert (CE_Events > 0);

	// Read # of events
	ReadToken(fp,token,120);
	
	while (!done)
	{
		ReadComments(fp);
		ReadToken(fp,token,120);
		if (!token[0])
			continue;

		// Check for still in init section
		if (strcmp(token,"#ENDINIT")==0)
			initdone = 1;
		else if (strcmp(token,"#ENDSCRIPT")==0)
		{
			done = 1;
			continue;
		}
		if (!initdone)
			continue;

		// Handle standard tokens
		if (strncmp(token,"#IF",3)==0)
		{
			curr_stack++;
			if (!stack_active[curr_stack-1])
				stack_active[curr_stack] = 0;
			else
				stack_active[curr_stack] = 1;
		}
		else if (strcmp(token,"#ELSE")==0)
		{
			if (curr_stack>0 && stack_active[curr_stack-1])
				stack_active[curr_stack] = !stack_active[curr_stack];
			continue;
		}
		else if (strcmp(token,"#ENDIF")==0)
		{
			if (!curr_stack)
				MonoPrint("<script reading Error - unmatched #ENDIF>\n");
			else
				curr_stack--;
			continue;
		}

		// Check for section activity
		if (stack_active[curr_stack])
		{
			// This section is active, handle tokens
			if (strncmp(token,"#IF",3)==0)
			{
				if (curr_stack >= MAX_STACK)
				{
					MonoPrint("<Brief Reading Error - stack overflow. Max stacks = %d",MAX_STACK);
					CloseCampFile(fp);
					return 0;
					}
				// Add all our if conditions here
				if (strncmp(token,"#IF_EVENT_PLAYED",16)==0)
				{
					if (sptr = strchr(token,' '))
						sptr++;
					i = atoi(sptr);
					if (i > CE_Events || !CampEvents[i]->HasFired())
						stack_active[curr_stack] = 0;
					else
						stack_active[curr_stack] = 1;
				}
				else if (strncmp(token,"#IF_MAIN_TARGET",15)==0)
				{
					if (sptr = strchr(token,' '))
						sptr++;
					if (*sptr == 'F')
						team = FalconLocalSession->GetTeam();
					else
						team = GetEnemyTeam(FalconLocalSession->GetTeam());
					if (sptr = strchr(sptr,' '))
						sptr++;
					i = atoi(sptr);
					if (TeamInfo[team]->gtm->priorityObj == i)
						stack_active[curr_stack] = 1;
					else
						stack_active[curr_stack] = 0;
				}
				else if (strncmp(token,"#IF_TROOPS_COMMITTED",20)==0)
				{
					if (sptr = strchr(token,' '))
						sptr++;
					if (*sptr == 'F')
						team = FalconLocalSession->GetTeam();
					else
						team = GetEnemyTeam(FalconLocalSession->GetTeam());
					if (sptr = strchr(sptr,' '))
						sptr++;
					i = atoi(sptr);
					o = (Objective) GetEntityByCampID(i);
					if (!o)
						stack_active[curr_stack] = 0;
					else
					{
						POData	pod = GetPOData(o);
						if (pod && pod->ground_assigned[team])
							stack_active[curr_stack] = 1;
						else
							stack_active[curr_stack] = 0;
					}
				}
				else if (strncmp(token,"#IF_CONTROLLED",14)==0)
				{
					char	type;
					if (sptr = strchr(token,' '))
						sptr++;
					team = atoi(sptr);
					if (sptr = strchr(sptr,' '))
						sptr++;
					type = *sptr;
					if (sptr = strchr(sptr,' '))
						sptr++;
					if (type == 'A')
					{
						// And logic
						stack_active[curr_stack] = 1;
						while (stack_active[curr_stack] && sptr && atoi(sptr))
						{
							o = (Objective) GetEntityByCampID(atoi(sptr));
							if (o && o->GetTeam() != team)
								stack_active[curr_stack] = 0;
							if (sptr = strchr(sptr,' '))
								sptr++;
						}
					}
					else
					{
						// Or logic
						stack_active[curr_stack] = 0;
						while (!stack_active[curr_stack] && sptr && atoi(sptr))
						{
							o = (Objective) GetEntityByCampID(atoi(sptr));
							if (o && o->GetTeam() == team)
								stack_active[curr_stack] = 1;
							if (sptr = strchr(sptr,' '))
								sptr++;
						}
					}
				}
				else if (strncmp(token,"#IF_INITIATIVE",14)==0)
				{
					stack_active[curr_stack] = 0;
					if (sptr = strchr(token,' '))
						sptr++;
					team = *sptr - '0';
					if (sptr = strchr(sptr,' '))
						sptr++;
					if (*sptr == 'G')
					{
						if (sptr = strchr(sptr,' '))
							sptr++;
						i = atoi(sptr);
						if (TeamInfo[team]->GetInitiative() >= i)
							stack_active[curr_stack] = 1;
					}
					else
					{
						if (sptr = strchr(sptr,' '))
							sptr++;
						i = atoi(sptr);
						if (TeamInfo[team]->GetInitiative() <= i)
							stack_active[curr_stack] = 1;
					}
				}
				else if (strncmp(token,"#IF_SUPPLY",10)==0)
				{
					stack_active[curr_stack] = 0;
					if (sptr = strchr(token,' '))
						sptr++;
					team = *sptr - '0';
					if (sptr = strchr(sptr,' '))
						sptr++;
					if (*sptr == 'G')
					{
						if (sptr = strchr(sptr,' '))
							sptr++;
						i = atoi(sptr);
						if (TeamInfo[team]->GetCurrentStats()->supplyLevel >= i)
							stack_active[curr_stack] = 1;
					}
					else
					{
						if (sptr = strchr(sptr,' '))
							sptr++;
						i = atoi(sptr);
						if (TeamInfo[team]->GetCurrentStats()->supplyLevel <= i)
							stack_active[curr_stack] = 1;
					}
				}
				else if (strncmp(token,"#IF_PLAYER_DIFFICULTY",21)==0)
				{
					stack_active[curr_stack] = 0;
					if (sptr = strchr(token,' '))
						sptr++;
					if (*sptr == 'G')
					{
						if (sptr = strchr(sptr,' '))
							sptr++;
						i = atoi(sptr);
						if (PlayerOptions.CampaignEnemyGroundExperience() >= i)
							stack_active[curr_stack] = 1;
					}
					else
					{
						if (sptr = strchr(sptr,' '))
							sptr++;
						i = atoi(sptr);
						if (PlayerOptions.CampaignEnemyGroundExperience() <= i)
							stack_active[curr_stack] = 1;
					}
				}
				else if (strncmp(token,"#IF_PRI_CONTROLLED_LT",21)==0)
				{					
					int	controlled = 0;
					if (sptr = strchr(token,' '))
						sptr++;
					if (*sptr == 'F')
						team = FalconLocalSession->GetTeam();
					else
						team = GetEnemyTeam(FalconLocalSession->GetTeam());
					if (sptr = strchr(sptr,' '))
						sptr++;
					i = atoi(sptr);
					VuListIterator	poit(POList);
					o = GetFirstObjective(&poit);
					while (o){
						if (o->GetTeam() == team)
							controlled++;
						o = GetNextObjective(&poit);
					}
					if (controlled < i)
						stack_active[curr_stack] = 1;
					else
						stack_active[curr_stack] = 0;
				}
				else if (strncmp(token,"#IF_ON_OFFENSIVE",16)==0)
				{
					if (sptr = strchr(token,' '))
						sptr++;
					team = atoi(sptr);
					if (TeamInfo[team] && TeamInfo[team]->GetGroundAction() && TeamInfo[team]->GetGroundAction()->actionType == GACTION_OFFENSIVE)
						stack_active[curr_stack] = 1;
					else
						stack_active[curr_stack] = 0;

/*					int				offensive_assigned = 0;
					if (sptr = strchr(token,' '))
						sptr++;
					if (*sptr == 'F')
						team = FalconLocalSession->GetTeam();
					else
						team = GetEnemyTeam(FalconLocalSession->GetTeam());
					if (sptr = strchr(sptr,' '))
						sptr++;
					// Check if we have offensive units assigned
					if (TeamInfo[team]->GetGroundAction()->actionType != GACTION_OFFENSIVE)
						stack_active[curr_stack] = 0;
					else
						stack_active[curr_stack] = 1;
*/
				}
				else if (strncmp(token,"#IF_FORCE_RATIO",15)==0)
				{
					Team	opposite;
					char	type,func;
					int		os,ts,ratio;

					if (sptr = strchr(token,' '))
						sptr++;
					type = *sptr;
					if (sptr = strchr(sptr,' '))
						sptr++;
					team = atoi(sptr);
					if (sptr = strchr(sptr,' '))
						sptr++;
					opposite = atoi(sptr);
					if (sptr = strchr(sptr,' '))
						sptr++;
					func = *sptr;
					if (sptr = strchr(sptr,' '))
						sptr++;
					i = atoi (sptr);
					switch (type)
					{
						case 'A':
							os = TeamInfo[team]->GetCurrentStats()->aircraft;
							ts = TeamInfo[opposite]->GetCurrentStats()->aircraft;
							break;
						case 'G':
							os = TeamInfo[team]->GetCurrentStats()->groundVehs;
							ts = TeamInfo[opposite]->GetCurrentStats()->groundVehs;
							break;
						case 'N':
							os = TeamInfo[team]->GetCurrentStats()->ships;
							ts = TeamInfo[opposite]->GetCurrentStats()->ships;
							break;
						default:
							os = TeamInfo[team]->GetCurrentStats()->groundVehs + TeamInfo[team]->GetCurrentStats()->aircraft;
							ts = TeamInfo[opposite]->GetCurrentStats()->groundVehs + TeamInfo[opposite]->GetCurrentStats()->aircraft;
							break;
					}
					ratio = os * 10 / ts;
					stack_active[curr_stack] = 0;
					if (func == 'G' && ratio >= i)
						stack_active[curr_stack] = 1;
					else if (func == 'L' && ratio <= i)
						stack_active[curr_stack] = 1;
				}
				else if (strncmp(token,"#IF_BORDOM_HOURS",16)==0)
				{
					if (sptr = strchr(token,' '))
						sptr++;
					if (((TheCampaign.CurrentTime - TheCampaign.lastMajorEvent) / CampaignHours) > static_cast<CampaignTime>(atoi(sptr)))
						stack_active[curr_stack] = 1;
					else
						stack_active[curr_stack] = 0;
				}
				else if (strncmp(token,"#IF_CAMPAIGN_DAY",16)==0)
				{
					stack_active[curr_stack] = 0;
					if (sptr = strchr(token,' '))
						sptr++;
					if (*sptr == 'G')
					{
						if (sptr = strchr(sptr,' '))
							sptr++;
						if (TheCampaign.GetCampaignDay() >= atoi(sptr))
							stack_active[curr_stack] = 1;
					}
					else
					{
						if (sptr = strchr(sptr,' '))
							sptr++;
						if (TheCampaign.GetCampaignDay() <= atoi(sptr))
							stack_active[curr_stack] = 1;
					}
				}
				else if (strncmp(token,"#IF_REINFORCEMENT",16)==0)
				{
					stack_active[curr_stack] = 0;
					if (sptr = strchr(token,' '))
						sptr++;
					team = atoi(sptr);
					if (sptr = strchr(sptr,' '))
						sptr++;
					if (*sptr == 'G')
					{
						if (sptr = strchr(sptr,' '))
							sptr++;
						if (TeamInfo[team]->GetReinforcement() >= atoi(sptr))
							stack_active[curr_stack] = 1;
					}
					else
					{
						if (sptr = strchr(sptr,' '))
							sptr++;
						if (TeamInfo[team]->GetReinforcement() <= atoi(sptr))
							stack_active[curr_stack] = 1;
					}
				}
				else if (strncmp(token,"#IF_RANDOM_CHANCE",16)==0)
				{
					stack_active[curr_stack] = 0;
					if (sptr = strchr(token,' '))
						sptr++;
					if ((rand()%100) < atoi(sptr))
						stack_active[curr_stack] = 1;
				}
				else
					stack_active[curr_stack] = 0;
				continue;
			}

			// special tokens
			if (strncmp(token,"#PLAY_MOVIE",11)==0)
				{
//				_TCHAR					str[128] = {0};
				CampEventDataMessage	*msg = new CampEventDataMessage(vuLocalSession,FalconLocalGame);

				if (sptr = strchr(token,' '))
					sptr++;
				i = atoi(sptr);
				// queue movie
//				AddIndexedStringToBuffer(1160+i-100,str);
//				UI_AddMovieToList(i,TheCampaign.CurrentTime,str);
				msg->dataBlock.message = CampEventDataMessage::playMovie;
				msg->dataBlock.event = i;
				FalconSendMessage(msg,TRUE);
				continue;
			}
			else if (strncmp(token,"#CHANGE_RELATIONS",17)==0)
			{
				int	with,rel;
				if (sptr = strchr(token,' '))
					sptr++;
				team = atoi(sptr);
				if (sptr = strchr(sptr,' '))
					sptr++;
				with = atoi(sptr);
				if (sptr = strchr(sptr,' '))
					sptr++;
				rel = atoi(sptr);
				if (TeamInfo[team] && TeamInfo[with])
				{
					SetTTRelations(team,with,rel);
					if (rel == Allied)
					{
						SetTeam (team,with);
						TeamInfo[team]->flags &= ~TEAM_ACTIVE;
					}
				}
			}
			else if (strncmp(token,"#DO_EVENT",9)==0)
			{
				if (sptr = strchr(token,' '))
					sptr++;
				i = atoi(sptr);
				if (i < CE_Events && i > 0)
					CampEvents[i]->DoEvent();
				continue;
			}
			else if (strncmp(token,"#SHIFT_INITIATIVE",16)==0)
			{
				int	to,amount;
				if (sptr = strchr(token,' '))
					sptr++;
				team = atoi(sptr);
				if (sptr = strchr(sptr,' '))
					sptr++;
				to = atoi(sptr);
				if (sptr = strchr(sptr,' '))
					sptr++;
				amount = atoi(sptr);
				if (TeamInfo[team] && TeamInfo[to])
					TransferInitiative (team, to, amount);				
				continue;
			}
			else if (strncmp(token,"#RESET_EVENT",12)==0)
			{
				if (sptr = strchr(token,' '))
					sptr++;
				i = atoi(sptr);
				if (i < CE_Events && i > 0)
					CampEvents[i]->SetEvent(0);
				continue;
			}
			else if (strncmp(token,"#END_GAME",9)==0)
			{
				if (sptr = strchr(token,' '))
					sptr++;
				// Post the campaign over message
				PostMessage(FalconDisplay.appWin,FM_CAMPAIGN_OVER,atoi(sptr),1); 
				continue;
			}
			else if (strcmp(token,"#RESET_BORDOM_TIMEOUT")==0)
			{
				TheCampaign.lastMajorEvent = TheCampaign.CurrentTime;
				continue;
			}
			else if (strncmp(token,"#CHANGE_PRIORITIES",18)==0)
			{
/*				if (sptr = strchr(token,' '))
					sptr++;
				team = atoi(sptr);
				if (sptr = strchr(sptr,' '))
					sptr++;
				i = atoi(sptr);
				if (TeamInfo[team])
					TeamInfo[team]->ReadPriorityFile(i);
*/
			}
			else if (strncmp(token,"#SET_MINIMUM_SUPPLIES",18)==0)
			{
				int		s,f,r;

				if (sptr = strchr(token,' '))
					sptr++;
				team = atoi(sptr);
				if (sptr = strchr(sptr,' '))
					sptr++;
				s = atoi(sptr);
				if (sptr = strchr(sptr,' '))
					sptr++;
				f = atoi(sptr);
				if (sptr = strchr(sptr,' '))
					sptr++;
				r = atoi(sptr);
				if (TeamInfo[team]->GetSupplyAvail() < s)
					TeamInfo[team]->SetSupplyAvail(s);
				if (TeamInfo[team]->GetFuelAvail() < f)
					TeamInfo[team]->SetFuelAvail(f);
				if (TeamInfo[team]->GetReplacementsAvail() < r)
					TeamInfo[team]->SetReplacementsAvail(r);
			}
			else if (strncmp(token,"#SET_PAK_PRIORITY",17)==0)
			{
				CampEntity	e;
				POData		pod;
				if (sptr = strchr(token,' '))
					sptr++;
				team = atoi(sptr);
				if (sptr = strchr(sptr,' '))
					sptr++;
				e = (CampEntity) GetEntityByCampID(atoi(sptr));
				if (sptr = strchr(sptr,' '))
					sptr++;
				i = atoi(sptr);
				if (e && e->IsObjective())
				{
					pod = GetPOData((Objective)e);
#ifdef DEBUG
					ShiAssert(pod);
					if (!pod)
						continue;
#endif
					pod->ground_priority[team] = pod->air_priority[team] = i;
					// KCK: player_priority only used now if >= 0
//					if (!(pod->flags & GTMOBJ_PLAYER_SET_PRIORITY))
//						pod->player_priority[team] =  i;
					pod->flags |= GTMOBJ_SCRIPTED_PRIORITY;
				}
			}
			else if (strncmp(token,"#SET_TEMPO",10)==0)
			{
/*				if (sptr = strchr(token,' '))
					sptr++;
				TheCampaign.Tempo = atoi (sptr);
*/
			}
			else if (strncmp(token,"#TOTAL_EVENTS",13)==0 || strncmp(token,"#SET",4)==0 || strcmp(token,"#ENDINIT")==0)
			{
				// KCK: For initialization only.
			}
			else
			{
				MonoPrint("CampEvent.cpp: Unrecognized token: %s\n",token);
			}
			// End active stack section
		}
		// End token handler
	}
	CloseCampFile(fp);
	return 1;
}