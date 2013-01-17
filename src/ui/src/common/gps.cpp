//
// Global Updater for the UI concering CAMPAIGN stuff
//
#include <windows.h>
#include "campbase.h"
#include "camplist.h"
#include "unit.h"
#include "division.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "cmap.h"
#include "gps.h"
#include "urefresh.h"
#include "debuggr.h"
#include "userids.h"
#include "campaign.h"
#include "Falclib\Include\IsBad.h"

extern C_Handler *gMainHandler;

extern BOOL gMoviePlaying;

extern VU_ID gSelectedFlightID;	// Last flight Selected (in ATO,Mission)
extern VU_ID gSelectedPackage;	// Current Package Selected (ATO)
extern VU_ID gSelectedEntity;	// Current Entity (Squadron/Unit/Objective) in OOB

void RemoveMissionCB(TREELIST *item);

void GPS_RemoveCB(void *rec)
{
	UI_Refresher *record=(UI_Refresher*)rec;

	if(record)
		record->Remove();
	delete record;
}

// Returns TRUE if I want to insert newitem before list item
static BOOL GPSTreeSortCB(TREELIST *list,TREELIST *newitem)
{
	_TCHAR *first,*second;
	long fval,sval;

	if(list && newitem)
	{
		if(list->Item_->_GetCType_() == _CNTL_SQUAD_)
			first=((C_Squadron*)list->Item_)->GetName();
		else if(list->Item_->_GetCType_() == _CNTL_ENTITY_)
			first=((C_Entity*)list->Item_)->GetName();
		else if(list->Item_->_GetCType_() == _CNTL_TEXT_)
			first=((C_Text*)list->Item_)->GetText();
		else
			first=NULL;

		if(newitem->Item_->_GetCType_() == _CNTL_SQUAD_)
			second=((C_Squadron*)newitem->Item_)->GetName();
		else if(newitem->Item_->_GetCType_() == _CNTL_ENTITY_)
			second=((C_Entity*)newitem->Item_)->GetName();
		else if(list->Item_->_GetCType_() == _CNTL_TEXT_)
			second=((C_Text*)newitem->Item_)->GetText();
		else
			second=NULL;

		if(!first || !second)
			return(FALSE);

		if(isdigit(*first) && isdigit(*second))
		{
			fval=atol(first);
			sval=atol(second);
			if(sval < fval)
				return(TRUE);
			else if(fval < sval)
				return(FALSE);
			// if(fval & sval are equal... let strcmp figure it out
		}
		else if(isdigit(*first))
			return(FALSE);
		else if(isdigit(*second))
			return(TRUE);
		if(_tcsicmp(second,first) < 0)
			return(TRUE);
	}
	return(FALSE);
}

// Returns TRUE if I want to insert newitem before list item
static BOOL GPSMissionSortPriorityCB(TREELIST *list,TREELIST *newitem)
{
	if(!list || !newitem)
		return(FALSE);

	if(((C_Mission*)newitem->Item_)->GetPriorityID() < ((C_Mission*)list->Item_)->GetPriorityID())
		return(TRUE);
	else if (((C_Mission*)newitem->Item_)->GetPriorityID() == ((C_Mission*)list->Item_)->GetPriorityID() &&
	         ((C_Mission*)newitem->Item_)->GetTakeOffTime() < ((C_Mission*)list->Item_)->GetTakeOffTime())
		return(TRUE);
	return(FALSE);
}

// Returns TRUE if I want to insert newitem before list item
static BOOL GPSMissionSortTimeCB(TREELIST *list,TREELIST *newitem)
{
	if(!list || !newitem)
		return(FALSE);

	if(((C_Mission*)newitem->Item_)->GetTakeOffTime() < ((C_Mission*)list->Item_)->GetTakeOffTime())
		return(TRUE);
	return(FALSE);
}

// Returns TRUE if I want to insert newitem before list item
static BOOL GPSMissionSortMissionCB(TREELIST *list,TREELIST *newitem)
{
	if(!list || !newitem)
		return(FALSE);

	if(_tcsicmp(((C_Mission*)newitem->Item_)->GetMission(),((C_Mission*)list->Item_)->GetMission()) < 0)
		return(TRUE);
	else if(!_tcsicmp(((C_Mission*)newitem->Item_)->GetMission(),((C_Mission*)list->Item_)->GetMission()) &&
			((C_Mission*)newitem->Item_)->GetTakeOffTime() < ((C_Mission*)list->Item_)->GetTakeOffTime())
		return(TRUE);
	return(FALSE);
}

// Returns TRUE if I want to insert newitem before list item
static BOOL GPSMissionSortStatusCB(TREELIST *list,TREELIST *newitem)
{
	if(!list || !newitem)
		return(FALSE);

	if(((C_Mission*)newitem->Item_)->GetStatusID() < ((C_Mission*)list->Item_)->GetStatusID())
		return(TRUE);
	else if(((C_Mission*)newitem->Item_)->GetStatusID() == ((C_Mission*)list->Item_)->GetStatusID() &&
			((C_Mission*)newitem->Item_)->GetTakeOffTime() < ((C_Mission*)list->Item_)->GetTakeOffTime())
		return(TRUE);
	return(FALSE);
}

// Returns TRUE if I want to insert newitem before list item
static BOOL GPSMissionSortPackageCB(TREELIST *list,TREELIST *newitem)
{
	if(!list || !newitem)
		return(FALSE);

	if(((C_Mission*)newitem->Item_)->GetPackageID() < ((C_Mission*)list->Item_)->GetPackageID())
		return(TRUE);
	else if(((C_Mission*)newitem->Item_)->GetPackageID() == ((C_Mission*)list->Item_)->GetPackageID() &&
			((C_Mission*)newitem->Item_)->GetTakeOffTime() < ((C_Mission*)list->Item_)->GetTakeOffTime())
		return(TRUE);
	return(FALSE);
}

void SelectMissionSortCB(long ID,short hittype,C_Base *control)
{
	C_TreeList *tree;
	F4CSECTIONHANDLE *Leave;
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	tree=(C_TreeList*)control->Parent_->FindControl(MISSION_LIST_TREE);
	if(tree)
	{
		Leave=UI_Enter(control->Parent_);
		switch(ID)
		{
			case SORT_PRIORITY:
				tree->SetSortCallback(GPSMissionSortPriorityCB);
				break;
			case SORT_STATUS:
				tree->SetSortCallback(GPSMissionSortStatusCB);
				break;
			case SORT_ROLE:
				tree->SetSortCallback(GPSMissionSortMissionCB);
				break;
			case SORT_PACKAGE:
				tree->SetSortCallback(GPSMissionSortPackageCB);
				break;
			case SORT_TAKEOFF:
			default:
				tree->SetSortCallback(GPSMissionSortTimeCB);
				break;
		}
		tree->ReorderBranch(tree->GetRoot());
		tree->RecalcSize();
		control->Parent_->RefreshClient(tree->GetClient());
		UI_Leave(Leave);
	}
}

GlobalPositioningSystem::GlobalPositioningSystem()
{
	Flags=0;
	Allowed_=UR_NOTHING;
	GPS_Hash=NULL;
	MisTree_=NULL;
	AtoTree_=NULL;
	Map_=NULL;

	ObjectiveMenu_=0;
	UnitMenu_=0;
	WaypointMenu_=0;
	SquadronMenu_=0;
	MissionMenu_=0;

	TeamNo_=-1;
}

GlobalPositioningSystem::~GlobalPositioningSystem()
{
	if(GPS_Hash)
		Cleanup();
}

void GlobalPositioningSystem::Setup()
{
	GPS_Hash=new C_Hash;
	GPS_Hash->Setup(_GPS_HASH_SIZE_);
	GPS_Hash->SetCallback(GPS_RemoveCB);
	GPS_Hash->SetFlags(C_BIT_REMOVE);
}

void GlobalPositioningSystem::Cleanup()
{
	if(GPS_Hash)
	{
		delete GPS_Hash;
		GPS_Hash=NULL;
	}

	MisTree_=NULL;
	AtoTree_=NULL;
	OOBTree_=NULL;
	Map_=NULL;
	Allowed_=UR_NOTHING;
}

void GlobalPositioningSystem::Clear()
{
	if(GPS_Hash)
	{
		delete GPS_Hash;
		GPS_Hash=NULL;
	}
	Setup();
}

void GlobalPositioningSystem::SetMissionTree(C_TreeList *tree)
{
	MisTree_=tree;
	MisTree_->SetSortType(TREE_SORT_CALLBACK);
	MisTree_->SetSortCallback(GPSMissionSortTimeCB);
	MisTree_->SetDelCallback(RemoveMissionCB);
}

void GlobalPositioningSystem::SetATOTree(C_TreeList *tree)
{
	AtoTree_=tree;
}

void GlobalPositioningSystem::SetOOBTree(C_TreeList *tree)
{
	OOBTree_=tree;
	OOBTree_->SetSortCallback(GPSTreeSortCB);
	OOBTree_->SetSortType(TREE_SORT_CALLBACK);
}

void GlobalPositioningSystem::UpdateDivisions()
{
	UI_Refresher *cur;
	Division div;
	Unit u;
	int t;

	for(t=0;t<NUM_TEAMS;t++) 
	{
		div=GetFirstDivision(t);
		while(div)
		{
			u=div->GetFirstUnitElement();
			if(u)
			{
				cur=(UI_Refresher*)GPS_Hash->Find(u->GetCampID() | UR_DIVISION);
				if(!cur)
				{
					// create a new one
					cur=new UI_Refresher;
					if(cur)
					{
						cur->Setup(div,this,Allowed_);
						//GPS_Hash->Add(div->nid | UR_DIVISION,cur); // this looks wrong
						GPS_Hash->Add(u->GetCampID() | UR_DIVISION,cur); // JPO - hope this is better.
					}
				}
				else
				{
					// check to update current
					cur->Update(div,Allowed_);
				}
			}
			div=GetNextDivision(div);
		}
	}
}

void GlobalPositioningSystem::Update()
{
	if(gMoviePlaying) // If a Movie is playing... don't execute... because the movie might stutter
		return;

	gMainHandler->EnterCritical();
	CampEnterCriticalSection();

	VuListIterator iter(AllCampList);
	CampEntity entity;
	UI_Refresher *cur;

	GPS_Hash->SetCheck(GPS_Hash->GetCheck()^1);

	UpdateDivisions();

	entity=GetFirstEntity(&iter);
	while(entity)
	{
		// FRB - CTD's here
		if(F4IsBadReadPtr(entity,sizeof(CampEntity)))
			continue;
		if(!entity->IsDead())
		{
			cur=(UI_Refresher*)GPS_Hash->Find(entity->GetCampID());
			if(!cur)
			{
				if(entity->IsUnit() && ((Unit)entity)->Inactive())
				{
				}
				else
				{
					// create a new one
					cur=new UI_Refresher;
					if(cur)
					{
						cur->Setup(entity,this,Allowed_);
						GPS_Hash->Add(entity->GetCampID(),cur);
					}
				}
			}
			else
			{
				if(cur->GetType() != entity->GetType())
				{
					GPS_Hash->Remove(entity->GetCampID());

					if(entity->IsUnit() && ((Unit)entity)->Inactive())
					{
					}
					else
					{
						// create a new one
						cur=new UI_Refresher;
						if(cur)
						{
							cur->Setup(entity,this,Allowed_);
							GPS_Hash->Add(entity->GetCampID(),cur);
						}
					}
				}
				else
				{
					// check to update current
					cur->Update(entity,Allowed_);
				}
			}
		}
		entity=GetNextEntity(&iter);
	}
	GPS_Hash->RemoveOld();
	if(MisTree_ && (Flags & _GPS_RESORT_MISSION_))
	{
		MisTree_->ReorderBranch(MisTree_->GetRoot());
		Flags |= _GPS_MISSION_RESIZE_;
	}
	if(MisTree_ && (Flags & _GPS_MISSION_RESIZE_))
	{
		MisTree_->RecalcSize();
		if(MisTree_->GetParent())
			MisTree_->GetParent()->RefreshClient(MisTree_->GetClient());
	}
	if(AtoTree_ && (Flags & _GPS_ATO_RESIZE_))
	{
		AtoTree_->RecalcSize();
		if(AtoTree_->GetParent())
			AtoTree_->GetParent()->RefreshClient(AtoTree_->GetClient());
	}
	if(OOBTree_ && (Flags & _GPS_OOB_RESIZE_))
	{
		OOBTree_->RecalcSize();
		if(OOBTree_->GetParent())
			OOBTree_->GetParent()->RefreshClient(OOBTree_->GetClient());
	}
	Flags=0;
	CampLeaveCriticalSection();
	gMainHandler->LeaveCritical();
}

void *GlobalPositioningSystem::Find(long ID)
{
	return(GPS_Hash->Find(ID));
}

void *GlobalPositioningSystem::GetFirst()
{
	if(GPS_Hash)
		return(GPS_Hash->GetFirst(&Cur_,&CurIdx_));
	return(NULL);
}

void *GlobalPositioningSystem::GetNext()
{
	if(GPS_Hash)
		return(GPS_Hash->GetNext(&Cur_,&CurIdx_));
	return(NULL);
}
