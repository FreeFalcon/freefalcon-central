/***************************************************************************\
	UI_IA.cpp
	Peter Ward
	December 3, 1996

	Main UI screen stuff for falcon
\***************************************************************************/
#include <windows.h>
#include "entity.h"
#include "feature.h"
#include "vehicle.h"
#include "evtparse.h"
#include "Mesg.h"
#include "MsgInc\DamageMsg.h"
#include "MsgInc\WeaponFireMsg.h"
#include "MsgInc\DeathMessage.h"
#include "MsgInc\MissileEndMsg.h"
#include "MsgInc\LandingMessage.h"
#include "MsgInc\EjectMsg.h"
#include "MsgInc\PlayerStatusMsg.h"
#include "PlayerOp.h"
#include "classtbl.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "acmi\src\include\acmirec.h"
#include "ui_ia.h"
#include "events.h"
#include "userids.h"
#include "textids.h"
#include "MissEval.h"
#include "CampStr.h"

// External function prototypes
_TCHAR *UI_WordWrap(C_Window *win,_TCHAR *str,long fontid,short width,BOOL *status);
void DeleteGroupList( long ID );

// ==================================
// Global sorted event list
// ==================================

EventElement *SortedEventList=NULL;

// ==================================
// Event list maintenance routines
// ==================================

void AddtoEventList(EventElement *theEvent)
	{
	EventElement	*cur,*last=NULL,*newone;

	while (theEvent)
		{
		newone = new EventElement;
		newone->next = NULL;
		newone->eventTime = theEvent->eventTime;
		newone->vuIdData1 = theEvent->vuIdData1;
		newone->vuIdData2 = theEvent->vuIdData2;
		strcpy(newone->eventString,theEvent->eventString);

		cur = SortedEventList;
		while (cur && cur->eventTime < newone->eventTime)
			{
			last = cur;

			// Sanity check
			if (cur->next && cur->next->eventTime < cur->eventTime)
			{
				ShiAssert(0);
				break;
			}

			cur = cur->next;
			}

		if (last)
			{
			newone->next = cur;
			last->next = newone;
			}
		else
			{
			newone->next = SortedEventList;
			SortedEventList = newone;
			}
		theEvent = theEvent->next;
		}
	}

void ClearSortedEventList()
	{
	EventElement *cur,*last;

	cur=SortedEventList;
	while(cur)
		{
		last=cur;
		cur=cur->next;
		delete last;
		}
	SortedEventList=NULL;
	}

// ==================================================
// The thing which makes the list and adds to windows
// ==================================================

static void AddMessageToWindow(C_Window *win, long client, int *y, CampaignTime eventtime, _TCHAR *eventstr)
{
	C_Text *txt;
	_TCHAR time_str[80];
	int wrap_w;

	// Add it to window
	// Do time string
	GetTimeString(eventtime, time_str);
	txt=new C_Text;
	txt->Setup(C_DONT_CARE,C_TYPE_LEFT);
	txt->SetFixedWidth(_tcsclen(time_str)+1);
	txt->SetText(time_str);
	txt->SetXY(45,*y);
	txt->SetFlagBitOn(C_BIT_RIGHT);
	txt->SetFGColor(0x00f0f0f0);
	txt->SetFont(win->Font_);
	txt->SetFlagBitOn(C_BIT_LEFT);
	txt->SetClient(static_cast<short>(client));
	win->AddControl(txt);

	// Do event string
	wrap_w = win->ClientArea_[client].right-win->ClientArea_[client].left - 48;
	txt=new C_Text;
	txt->Setup(C_DONT_CARE,C_TYPE_LEFT);
	txt->SetFixedWidth(_tcsclen(eventstr)+1);
	txt->SetXY(48,*y);
	txt->SetW(wrap_w);
	txt->SetFGColor(0x00f0f0f0);
	txt->SetFont(win->Font_);
	txt->SetFlagBitOn(C_BIT_LEFT|C_BIT_WORDWRAP);
	txt->SetClient(static_cast<short>(client));
	txt->SetText(eventstr);
	win->AddControl(txt);
	*y+=txt->GetH();
}

// ProcessEventList now simply builds an ordered event list from the MissionEvaluator 
// and add the messages to a window
void ProcessEventList(C_Window *win,long client)
	{
	EventElement		*cur;
	FlightDataClass		*flight_data;
	PilotDataClass		*pilot_data;
	int					i,y=0;

	// Build an event list from the MissionEvaluator class
	flight_data = TheCampaign.MissionEvaluator->flight_data;
	while (flight_data)
		{
		AddtoEventList(flight_data->root_event);
		pilot_data = flight_data->pilot_list;
		while (pilot_data)
			{
			for (i=0;i<pilot_data->weapon_types;i++)
				AddtoEventList(pilot_data->weapon_data[i].root_event);
			pilot_data = pilot_data->next_pilot;
			}
		flight_data = flight_data->next_flight;
		}
	
	cur=SortedEventList;
	while(cur)
		{
		AddMessageToWindow(win,client,&y,cur->eventTime,cur->eventString);
		cur=cur->next;
		}

	// KCK: Peter suggested we move this to somewhere else
	ClearSortedEventList();
	win->ScanClientArea(client);
	win->RefreshWindow();
	}

// ProcessEventList now simply builds an ordered event list from the MissionEvaluator 
// and add the messages to a window
// This is called by ACMI Import which is also responible for
// cleaning up the sorted list
EventElement * ProcessEventListForACMI(void)
{
	FlightDataClass		*flight_data;
	PilotDataClass		*pilot_data;
	int					i;//,y=0;

	// Build an event list from the MissionEvaluator class
	flight_data = TheCampaign.MissionEvaluator->flight_data;
	while (flight_data)
	{
		AddtoEventList(flight_data->root_event);
		pilot_data = flight_data->pilot_list;
		while (pilot_data)
		{
			for (i=0;i<pilot_data->weapon_types;i++)
				AddtoEventList(pilot_data->weapon_data[i].root_event);
			pilot_data = pilot_data->next_pilot;
		}
		flight_data = flight_data->next_flight;
	}
	
	return SortedEventList;
}

/*
** This is called by the ACMI UI where the events are being read off
** the tape as an array, not a linked list
*/
void ProcessEventArray(C_Window *win, void *events, int count )
{
	C_Text *txt;
	ACMITextEvent *evList,*cur;
	_TCHAR *Time_str = NULL,*Message_str = NULL;
	int y,fh,wrap_w;
	int i;

	evList=(ACMITextEvent *)events;
	if(evList == NULL)
		return;

	DeleteGroupList( win->GetID() );

	y=0;
	fh=gFontList->GetHeight(win->Font_);
	wrap_w=win->ClientArea_[0].right-win->ClientArea_[0].left - 40;
	for ( i = 0; i < count; i++ )
	{
		cur = &evList[i];

		if (!F4IsBadReadPtr(cur->timeStr, sizeof(_TCHAR)))
			Time_str = (_TCHAR *)cur->timeStr;

		if (!F4IsBadReadPtr(cur->msgStr, sizeof(_TCHAR)))
			Message_str = (_TCHAR *)cur->msgStr;

		txt=new C_Text;
		txt->Setup(C_DONT_CARE,C_TYPE_LEFT);
		txt->SetFixedWidth(_tcsclen(Time_str)+1);
		txt->SetText(Time_str);
		txt->SetXY(0,y);
		txt->SetFGColor(0x0000ff00);
		txt->SetFont(win->Font_);
		txt->SetFlagBitOn(C_BIT_LEFT);
		txt->SetUserNumber( 0, cur->intTime );
		txt->SetUserNumber( _UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_ );

		win->AddControl(txt);

		txt=new C_Text;
		txt->Setup(C_DONT_CARE,C_TYPE_LEFT);
		txt->SetFixedWidth(_tcsclen(Message_str)+1);
		txt->SetXY(34,y);
		txt->SetW(wrap_w);
		txt->SetFGColor(0x00f0f0f0);
		txt->SetFont(win->Font_);
		txt->SetFlagBitOn(C_BIT_LEFT|C_BIT_WORDWRAP);
		txt->SetUserNumber( _UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_ );
		txt->SetText(Message_str);
		win->AddControl(txt);
		y+=txt->GetH();
	}
	win->ScanClientArea(0);
	win->RefreshWindow();
}




















// Everything below here is out of date
#if 0

_TCHAR *FindCallsign(uchar team,uchar call,uchar flight,uchar slot);

void CheckEject(VU_ID Pilot,float damage,float fuel);
void CheckLanding(VU_ID Pilot,float damage,float fuel);

// #define _WRITE_TO_FILE_ 1

// External Variables
extern WEAPONUSAGE *WeaponUsage;


// Internal function prototypes

// Internal Variables
#if 0
static DEBRIEF_EVENT UIEvent[EID_NUM_STRINGS];
#else // Since I don't have a reader yet...
static DEBRIEF_EVENT UIEvent[EID_NUM_STRINGS]=
{
	{ " ", 0 },
	{ "%s fired %s",             2, 1, 2 }, // Gun message
	{ "%s launched %s",          2, 1, 2 }, // Missile message
	{ "%s dropped %s",           2, 1, 2 }, // Bomb message
	{ "%s launched %s",          2, 1, 2 }, // Rocket message
	{ "%s destroyed",            1, 1 }, // Death message
	{ "%s damaged",              1, 1 }, // Weapon damage message
	{ "%s collided with %s",     2, 1, 4 }, // Mid air collision damage message
	{ "%s hit the ground",       1, 1 }, // Hit ground damage message
	{ "%s hit a building",       1, 1 }, // Hit a building damage message
	{ "%s damaged by explosion", 1, 1 }, // Proximity damage message
	{ "%s damaged by debris",    1, 1 }, // Hit debris damage message
	{ "%s",                      1, 3 }, // Missile End Message
	{ "%s landed",               1, 1 },
	{ "%s ejected",              1, 1 },
	{ "%s pilot ejected",        1, 1 },
	{ "%s entered arena",        1, 1 },
	{ "%s left arena",           1, 1 },
};
#endif

static _TCHAR UIEventOutput[EVT_MAX_PARAMS][32];

// Code

void TimeToAscii (ulong time, char* time_str)
{
	long day, hour, min, sec;

   // convert from milliseconds to seconds
	time /= 1000;

	day = (int)(time / (24 * 3600));
	time -= day * 24 * 3600;
	hour = (int)(time / 3600);
	time -= hour * 3600;
	min = (int)(time / 60);
	time -= min * 60;
	sec = (int)(time);

	_stprintf(time_str,"%02d:%02d", min, sec);
}

#ifdef _WRITE_TO_FILE_
long BuildEvent(EventElement *theEvent,_TCHAR output[])
{
	FalconDamageMessage			dmm(FalconNullId,NULL);
	FalconWeaponsFire			wfm(FalconNullId,NULL);
	FalconDeathMessage			dtm(FalconNullId,NULL);
	FalconMissileEndMessage		mem(FalconNullId,NULL);
	FalconLandingMessage		lnd(FalconNullId,NULL);
	FalconEjectMessage			ejc(FalconNullId,NULL);
	FalconPlayerStatusMessage	psm(FalconNullId,NULL);
	int retval=0;

	if(theEvent == NULL)
		return(FALSE);

	switch(theEvent->idData.type)
	{
		case WeaponFireMsg:
			// Check if this was fired by this pilot/weapon combo
			wfm.Decode(&(theEvent->eventData),0);
			theEvent->eventData -= theEvent->idData.size;
			_stprintf(output,"WFM: Weapon VU_ID(%1d) Fired By VU_ID(%1d)",wfm.dataBlock.fWeaponUID.num_,wfm.dataBlock.fEntityID.num_);
			return(1);
			break;

		case DamageMsg:
			// Check if this was fired by this pilot/weapon combo
			dmm.Decode(&(theEvent->eventData),0);
			theEvent->eventData -= theEvent->idData.size;
			_stprintf(output,"DMG: Weapon VU_ID(%1d) Fired By VU_ID(%1d) Target VU_ID(%1d)",dmm.dataBlock.fWeaponUID.num_,dmm.dataBlock.fEntityID.num_,dmm.dataBlock.dEntityID.num_);
			return(1);
			break;
		case DeathMessage:
			dtm.Decode(&(theEvent->eventData),0);
			theEvent->eventData -= theEvent->idData.size;
			_stprintf(output,"DTH: Weapon VU_ID(%1d) Fired By VU_ID(%1d) Target VU_ID(%1d)",dtm.dataBlock.fWeaponUID.num_,dtm.dataBlock.fEntityID.num_,dtm.dataBlock.dEntityID.num_);
			return(1);
			break;
		case LandingMessage:
			lnd.Decode(&(theEvent->eventData),0);
			theEvent->eventData -= theEvent->idData.size;
			_stprintf(output,"landing score: %f",lnd.dataBlock.score);
			return(1);
			break;
		case MissileEndMsg:
			mem.Decode(&(theEvent->eventData),0);
			theEvent->eventData -= theEvent->idData.size;
			_stprintf(output,"MEM: Weapon VU_ID(%1d) Fired By VU_ID(%1d) Target VU_ID(%1d)",mem.dataBlock.fWeaponUID.num_,mem.dataBlock.fEntityID.num_,mem.dataBlock.dEntityID.num_);
			return(1);
			break;
		case EjectMsg:
			ejc.Decode(&(theEvent->eventData),0);
			theEvent->eventData -= theEvent->idData.size;
			_stprintf(output,"EJC: Eject VU_ID(%1d)",ejc.dataBlock.ePlaneID.num_);
			return(1);
			break;
		case PlayerStatusMsg:
			psm.Decode(&(theEvent->eventData),0);
			theEvent->eventData -= theEvent->idData.size;
			_stprintf(output,"PSM: VU_ID(%1d) [%s] %1ld",psm.EntityId().num_,psm.dataBlock.callsign,psm.dataBlock.state);
			return(1);
			break;
	}
	_stprintf(output,"Message not of type (WFM,DMM,DTH,LND,MEM,PSM) (0x%08lx or %1ld)",(long)(theEvent->idData.type),(long)(theEvent->idData.type));
	return(2);
}
#endif

void FindTimeOffset(long Offset,_TCHAR *buffer)
{
	long theTime=Offset / VU_TICS_PER_SECOND;

	_stprintf(buffer,"+%01ld:%02ld",theTime/60,theTime%60);
}

static VU_ID SaveCollided1,SaveCollided2;
static long SaveCollidedTime=0;
BOOL CheckAlreadyCollided(VU_ID ID1,VU_ID ID2,long gameTime)
{
	if((gameTime - SaveCollidedTime) < 3000)
	{
		if(ID1 == SaveCollided1 && ID2 == SaveCollided2)
			return(TRUE);
		if(ID2 == SaveCollided1 && ID1 == SaveCollided2)
			return(TRUE);
	}
	SaveCollided1=ID1;
	SaveCollided2=ID2;
	SaveCollidedTime=gameTime;
	return(FALSE);
}

/*
	BuildMessage - Builds the strings for debriefing an Event saved in "lastevt.acm"
		input	 -	EventElement *theEvent			Event Record
					int           DbrfType			Debriefing type:
									0 = Instant Action
									1 = Dogfight

					VU_ID         TrackID - 0, or ID of messages you are interested in
					              (Based on shooter)
					long          TrackTime - time of message which we are tracking (the one that set TrackID)

		output	 -	_TCHAR		  output[4][40]		Array of strings where:
									[0] = Time
									[1] = Who/What is doing this
									[2] = Who/What they are doing this too
									[3] = Weapon Used

		returns	 -	String ID to use to print "output" variables
					0 = Don't use this string...
					1 or greater... string ID to use

	Note: The calling routine is responsible for using [Return Value] to build the output string based on
		  language ordering of the Parameters
*/




/*
	// HOW TO GET THE MESSAGES YOU WANT IN ProcessEventList

	unsigned char mask[EVT_MESSAGE_BITS];

	// Clear Masks
	for(y=0;y<EVT_MESSAGE_BITS;y++)
		mask[y]=0;

	// Desired messages
	mask[WeaponFireMsg >> 3] |= 0x01 << (WeaponFireMsg & 0x0007);
	mask[DeathMessage >> 3] |= 0x01 << (DeathMessage & 0x0007);
	mask[DamageMsg >> 3] |= 0x01 << (DamageMsg & 0x0007);
	mask[MissileEndMsg >> 3] |= 0x01 << (MissileEndMsg & 0x0007);
	mask[LandingMessage >> 3] |= 0x01 << (LandingMessage & 0x0007);
	mask[EjectMsg >> 3] |= 0x01 << (EjectMsg & 0x0007);
	mask[PlayerStatusMsg >> 3] |= 0x01 << (PlayerStatusMsg & 0x0007);

*/

static void AddMessageToFile(FILE *ofp,long eventID,_TCHAR output[][32],BOOL TrackFlag)
{
	_TCHAR *eventstr;
	_TCHAR buffer[200];

	eventstr=AssembleEventString(eventID,output);

	if(!eventstr)
		return;

	// Add it to window
	_stprintf(buffer,"%-10s   %s\n",UIEventOutput[EVT_OUT_TIME],eventstr);
	fwrite(buffer,_tcsclen(buffer)*sizeof(_TCHAR),1,ofp);
}
/*
	// HOW TO GET THE MESSAGES YOU WANT IN SaveEventList

	unsigned char mask[EVT_MESSAGE_BITS];

	// Clear Masks
	for(y=0;y<EVT_MESSAGE_BITS;y++)
		mask[y]=0;

	// Desired messages
	mask[WeaponFireMsg >> 3] |= 0x01 << (WeaponFireMsg & 0x0007);
	mask[DeathMessage >> 3] |= 0x01 << (DeathMessage & 0x0007);
	mask[DamageMsg >> 3] |= 0x01 << (DamageMsg & 0x0007);
	mask[MissileEndMsg >> 3] |= 0x01 << (MissileEndMsg & 0x0007);
	mask[LandingMessage >> 3] |= 0x01 << (LandingMessage & 0x0007);
	mask[EjectMsg >> 3] |= 0x01 << (EjectMsg & 0x0007);
	mask[PlayerStatusMsg >> 3] |= 0x01 << (PlayerStatusMsg & 0x0007);

*/

/* This function is no longer used
void SaveEventList(FILE *ofp,unsigned char mask[],int DebriefType)
{
	EventElement *evList,*cur;
	long eventMessage;
	BOOL TrackFlag,Done;
	VU_ID TrackID;
	long TrackTime;
	long dummy;

	evList=ReadEventFile("lastflt", mask);
	if(evList == NULL)
		return;

	TrackID=FalconNullId;

	cur=evList;
	while(cur)
	{
		eventMessage=BuildMessage(cur,UIEventOutput,DebriefType,&TrackID,&TrackTime,&dummy);
		if(eventMessage)
		{
			TrackFlag=FALSE;
			Done=FALSE;
			while(eventMessage && !Done)
			{
				if(TrackID == FalconNullId)
					Done=TRUE;
				AddMessageToFile(ofp,eventMessage,UIEventOutput,TrackFlag);
				TrackFlag=TRUE;
				if(!Done)
					eventMessage=BuildMessage(cur,UIEventOutput,DebriefType,&TrackID,&TrackTime,&dummy);
			}

		}
		cur=cur->next;
	}
	DisposeEventList(evList);
}
*/

#endif
