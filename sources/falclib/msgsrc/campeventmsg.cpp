/*
 * Machine Generated source file for message "Campaign Event Message".
 * NOTE: The functions here must be completed by hand.
 * Generated on 18-February-1997 at 18:23:02
 * Generated from file EVENTS.XLS by Kevin Klemmick
 */

#include "mesg.h"
#include "CUIEvent.h"
#include "CampStr.h"
#include "Camplib.h"
#include "Sfx.h"
#include "Find.h"
#include "OtwDrive.h"
#include "CmpClass.h"
#include "Brief.h"
#include "F4Version.h"
#include "campaign.h"
#include "classtbl.h"
#include "Cmpclass.h"
#include "Campaign.h"
#include "falclib.h"
#include "Falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "MissEval.h"
#include "Graphics/Include/drawparticlesys.h"

//sfr: added here for checks
#include "InvalidBufferException.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gTextMemPool;
#endif

extern int InterestingSFX (float x, float y);
extern void ConstructOrderedSentence (_TCHAR *string, _TCHAR *format, ... );
extern void ConstructOrderedGenderedSentence (short maxsize, _TCHAR *string, EventDataClass *data, ... );
extern void UI_UpdateEventList (void);


// =================================
// EventDataClass stuff
// (This is admittidly missplaced)
// =================================

EventDataClass::EventDataClass (void)
{
	formatId = 0;
	xLoc = yLoc = 0;
	memset(vuIds,0,sizeof(VU_ID)*CUI_ME);
	memset(owners,0,sizeof(short)*CUI_MD);
	memset(textIds,0,sizeof(short)*CUI_MS);
}

// =================================
// FalconCampEventMessage
// =================================

FalconCampEventMessage::FalconCampEventMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (CampEventMsg, FalconEvent::CampaignThread, entityId, target, loopback)
{
	memset(&dataBlock,0,sizeof(dataBlock));
}

FalconCampEventMessage::FalconCampEventMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (CampEventMsg, FalconEvent::CampaignThread, senderid, target)
{
	memset(&dataBlock,0,sizeof(dataBlock));
	type;
}

FalconCampEventMessage::~FalconCampEventMessage(void)
{
}

int FalconCampEventMessage::Process(uchar autodisp)
{
	_TCHAR		text[512];

	// KCK: This critical section can be removed if we can keep the camaign from 
	// dispatching messages during an EndCampaign() call
	CampEnterCriticalSection();
	if (autodisp || !TheCampaign.IsLoaded())
	{
		CampLeaveCriticalSection();
		return -1;
	}

#ifdef USE_SH_POOLS
	CampUIEventElement *event = (CampUIEventElement *)MemAllocPtr( gTextMemPool, sizeof(CampUIEventElement), FALSE );
#else
	CampUIEventElement *event = new CampUIEventElement();
#endif

	// Do Visual effects here (only air explosion on losses, right now)
	if (dataBlock.eventType == campLosses && InterestingSFX(GridToSim(dataBlock.data.yLoc), GridToSim(dataBlock.data.xLoc)))
	{
		VehicleClassDataType	*vc;
		vc = GetVehicleClassData(-1 * dataBlock.data.textIds[2]);
		if (vc && Falcon4ClassTable[vc->Index].vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_AIR)
		{
			Tpoint    pos;
			pos.x = GridToSim(dataBlock.data.xLoc);
			pos.y = GridToSim(dataBlock.data.yLoc);
			pos.z = -15000;
			//OTWDriver.AddSfxRequest( new SfxClass( SFX_AIR_EXPLOSION,&pos,12.0F,400.0F) );
			Tpoint PSvec;
			PSvec.x = 0;
			PSvec.y = 0;
			PSvec.z = 0;
			DrawableParticleSys::PS_AddParticleEx((SFX_AIR_EXPLOSION + 1),
								 &pos,
								 &PSvec);
		}
	}

	// Now do the news event
	event->x = dataBlock.data.xLoc;
	event->y = dataBlock.data.yLoc;
	event->time = TheCampaign.CurrentTime;
	event->flags = 0; // dataBlock.flags;
	event->team = dataBlock.team;

	ConstructOrderedGenderedSentence (512, text, &dataBlock.data);

#ifdef USE_SH_POOLS
	event->eventText = (_TCHAR *)MemAllocPtr( gTextMemPool, sizeof(_TCHAR)*(_tcslen(text)+1), FALSE );
#else
	event->eventText = new _TCHAR[_tcslen(text)+1];
#endif
	_stprintf(event->eventText,text);

	ShiAssert(_tcslen(text)+1 < 512);

	// Register the event with the mission evaluator and record in the event list
	TheCampaign.MissionEvaluator->RegisterEvent(event->x, event->y, event->team, dataBlock.eventType, event->eventText);
	TheCampaign.AddCampaignEvent(event);

	CampLeaveCriticalSection();
	UI_UpdateEventList();
	return 0;
}

