#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include "CmpGlobl.h"
#include "ListADT.h"
#include "F4Vu.h"
#include "APITypes.h"
#include "Objectiv.h"
#include "Strategy.h"
#include "Unit.h"
#include "Find.h"
#include "Path.h"
#include "ASearch.h"
#include "Campaign.h"
#include "Update.h"
#include "f4vu.h"
#include "CampList.h"
#include "gtm.h"
#include "team.h"
#include "gndunit.h"
#include "gtmobj.h"
#include "Manager.h"
#include "MsgInc\NavalTaskingMsg.h"
#include "FalcSess.h"
#include "ClassTbl.h"

#include "Debuggr.h"

// ======================================================================
// NTM (Naval Tasking Manager) Build ground tasking orders for each team
// ======================================================================

// =====================
// Class Functions
// =====================

// constructors
NavalTaskingManagerClass::NavalTaskingManagerClass(ushort type, Team t) : CampManagerClass(type, t)
	{
	flags = 0;
//	unitList = new FalconPrivateList(&AllNavalFilter);	
//	unitList->Init();
	unitList = NULL;
	tod = 0;			
	topPriority = 0;
	done = 0;			
	}

NavalTaskingManagerClass::NavalTaskingManagerClass(VU_BYTE **stream) : CampManagerClass(stream)
	{
	memcpy(&flags, *stream, sizeof(short));					*stream += sizeof(short);
//	unitList = new FalconPrivateList(&AllNavalFilter);	
//	unitList->Init();
	unitList = NULL;
	tod = 0;			
	topPriority = 0;
	done = 0;			
	}

NavalTaskingManagerClass::NavalTaskingManagerClass(FILE *file) : CampManagerClass(file)
	{
	fread(&flags, sizeof(short), 1, file);
//	unitList = new FalconPrivateList(&AllNavalFilter);	
//	unitList->Init();
	unitList = NULL;
	tod = 0;			
	topPriority = 0;
	done = 0;			
	}

NavalTaskingManagerClass::~NavalTaskingManagerClass()
	{
	if (unitList)
		{
		unitList->DeInit();
		delete unitList;
		}
	}

int NavalTaskingManagerClass::SaveSize (void)
	{
	return CampManagerClass::SaveSize()
		+ sizeof(short);
	}

int NavalTaskingManagerClass::Save (VU_BYTE **stream)
	{
	CampManagerClass::Save(stream);
	memcpy(*stream, &flags, sizeof(short));					*stream += sizeof(short);
	return NavalTaskingManagerClass::SaveSize();
	}

int NavalTaskingManagerClass::Save (FILE *file)
	{
	int	retval=0;

	if (!file)
		return 0;
	retval += CampManagerClass::Save(file);
	retval += fwrite(&flags, sizeof(short), 1, file);
	return retval;
	}

int NavalTaskingManagerClass::Task (void)
	{               
	return 0;
	}

void NavalTaskingManagerClass::DoCalculations(void)
	{
	MissionRequestClass	mis;
	Unit				unit;
	CAMPREGLIST_ITERATOR		mit(AllRealList);
	int					j;
	Objective			o;

	// Target all naval units
	unit = (Unit) mit.GetFirst();
	while (unit)
		{
		if (unit->IsTaskForce() && GetRoE(owner,unit->GetTeam(),ROE_NAVAL_FIRE) == ROE_ALLOWED)
			{
			mis.requesterID = FalconNullId;
			unit->GetLocation(&mis.tx,&mis.ty);
			mis.vs = unit->GetTeam();
			mis.who = owner;
			j = 30 + (rand()+unit->GetCampID())%60;
			mis.tot = Camp_GetCurrentTime() + j*CampaignMinutes;
			mis.tot_type = TYPE_NE;
			mis.targetID = unit->Id();
			mis.target_num = 255;
			mis.mission = AMIS_ASHIP;
			mis.roe_check = ROE_NAVAL_FIRE;
			// Determine if they're active or static
			o = FindNearestObjective(mis.tx, mis.ty, NULL, 2);
			if (o && o->GetType() == TYPE_PORT)
				{
				if (unit->GetSType() == STYPE_UNIT_SEA_TANKER || unit->GetSType() == STYPE_UNIT_SEA_TRANSPORT)
					mis.context = enemyNavalForceUnloading;
				else
					mis.context = enemyNavalForceStatic;
				}
			else
				mis.context = enemyNavalForceActive;
			mis.RequestMission();
			}
		unit = (Unit) mit.GetNext();
		}
	}

// Sends a message to the NTM
void NavalTaskingManagerClass::SendNTMMessage (VU_ID from, short message, short data1, short data2, VU_ID data3)
	{
	VuTargetEntity				*target = (VuTargetEntity*) vuDatabase->Find(OwnerId());
	FalconNavalTaskingMessage*		tontm = new FalconNavalTaskingMessage(Id(), target);

	if (this)
		{
		tontm->dataBlock.from = from;
		tontm->dataBlock.team = owner;
		tontm->dataBlock.messageType = message;
		tontm->dataBlock.data1 = data1;
		tontm->dataBlock.data2 = data2;
		tontm->dataBlock.enemy = data3;
		FalconSendMessage(tontm,TRUE);
		}
	}

int NavalTaskingManagerClass::Handle(VuFullUpdateEvent *event)
	{
	NavalTaskingManagerClass* tmpGTM = (NavalTaskingManagerClass*)(event->expandedData_);

	// Copy in new data
	memcpy(&flags, &tmpGTM->flags, sizeof(short));
	return (VuEntity::Handle(event));
	}

// ==================
// Global functions
// ==================

