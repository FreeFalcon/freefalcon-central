#include <stdio.h>
#include <conio.h>
#include <stddef.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <math.h>
#include "CmpGlobl.h"
#include "ListADT.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "ASearch.h"
#include "Path.h"
#include "Find.h"
#include "Campaign.h"
#include "ATM.h"
#include "update.h"
#include "loadout.h"
#include "gndunit.h"
#include "team.h"
#include "Debuggr.h"
#include "AIInput.h"
#include "classtbl.h"

#ifdef DEBUG
#define ROBIN_GDEBUG

extern int gDumping;
extern int gCheckConstructFunction;
extern char	OrderStr[GORD_LAST][15];

#include "CampStr.h"
#endif

#ifdef ROBIN_GDEBUG
uchar TrackingOn[MAX_CAMP_ENTITIES];
#endif

// ============================================
// Externals
// ============================================

extern VU_ID_NUMBER vuAssignmentId;
extern VU_ID_NUMBER vuLowWrapNumber;
extern VU_ID_NUMBER vuHighWrapNumber;
extern VU_ID_NUMBER lastNonVolitileId;
extern VU_ID_NUMBER lastLowVolitileId;
extern VU_ID_NUMBER lastVolitileId;

#ifdef CAMPTOOL
extern unsigned char        SHOWSTATS;
#endif

#ifdef CAMPTOOL
// Renaming tool stuff
extern VU_ID_NUMBER RenameTable[65536];
extern int gRenameIds;
#endif

extern int BreakNumber[4];				// none, Ground, Air, Naval unit's break number
extern int HDelta[7];

extern int ScorePosition (Unit battalion, int role, int role_score, Objective o, GridIndex x, GridIndex y, int owned_by_us);
extern Objective FindBestPosition(Unit battalion, Brigade brigade, int role, F4PFList nearlist);
extern int OnValidObjective (Unit e, int role, F4PFList nearlist);
extern int GetNewRole (Unit e, Unit brig);

extern int FindUnitSupportRole(Unit u);

extern FILE
	*save_log,
	*load_log;

extern int
	start_save_stream,
	start_load_stream;

// ============================================
// Prototypes
// ============================================

// ==================================
// Some module globals - to save time
// ==================================

extern int haveWeaps;
extern int ourRange;
int ourObjDist;
int ourObjOwner;
int ourFrontDist;
extern int theirDomain;

// =================================
// Smart heap pool stuff
// =================================

#ifdef USE_SH_POOLS
MEM_POOL	BrigadeClass::pool;
#endif

// ============================================
// Brigade class Functions
// ============================================

// KCK: ALL BRIGADE CONSTRUCTION SHOULD USE THIS FUNCTION!
BrigadeClass* NewBrigade (int type)
	{
	BrigadeClass	*new_brigade;
#ifdef DEBUG
	gCheckConstructFunction = 1;
#endif
	VuEnterCriticalSection();
	lastVolitileId = vuAssignmentId;
	vuAssignmentId = lastNonVolitileId;
	vuLowWrapNumber = FIRST_NON_VOLITILE_VU_ID_NUMBER;
	vuHighWrapNumber = LAST_NON_VOLITILE_VU_ID_NUMBER;
	new_brigade = new BrigadeClass (type);
	lastNonVolitileId = vuAssignmentId;
	vuAssignmentId = lastVolitileId;
	vuLowWrapNumber = FIRST_VOLITILE_VU_ID_NUMBER;
	vuHighWrapNumber = LAST_VOLITILE_VU_ID_NUMBER;
	VuExitCriticalSection();
#ifdef DEBUG
	gCheckConstructFunction = 0;
#endif
	return new_brigade;
	}

// constructors
BrigadeClass::BrigadeClass(int type) : GroundUnitClass(type)
	{
#ifdef DEBUG
	ShiAssert (gCheckConstructFunction);
#endif

	elements = 0;
	c_element = 0;
	memset (element,0,sizeof(VU_ID)*MAX_UNIT_CHILDREN);
	fullstrength = 0;
	SetParent(1);
	}

BrigadeClass::BrigadeClass(VU_BYTE **stream) : GroundUnitClass(stream)
	{
	if (load_log)
	{
		fprintf (load_log, "%08x BrigadeClass ", *stream - start_load_stream);
		fflush (load_log);
	}

	memset (element,0,sizeof(VU_ID)*MAX_UNIT_CHILDREN);
	memcpy(&elements, *stream, sizeof(uchar));						*stream += sizeof(uchar);
	memcpy(element, *stream, sizeof(VU_ID)*elements);				*stream += sizeof(VU_ID)*elements;
#ifdef DEBUG
	for (int i=0; i<elements; i++)
		element[i].num_ &= 0x0000ffff;
#endif
	c_element = 0;
	fullstrength = 0;

#ifdef DEBUG
	char	buffer[256];

	sprintf(buffer,"campaign\\save\\dump\\%d.BRI",GetCampID());
	unlink(buffer);
#endif
	}

BrigadeClass::~BrigadeClass (void)
	{
	if (IsAwake())
		Sleep();
	}

int BrigadeClass::SaveSize (void)
	{
#if 0
#ifdef DEBUG
	int			i;
	Unit		e;
	for (i=0; i<elements; i++)
		{
		e = GetUnitElement(i);
		if (!e)
			RemoveChild(element[i]);
		}
#endif
#endif
	return GroundUnitClass::SaveSize() 
		+ sizeof(uchar)
		+ sizeof(VU_ID) * elements;
	}

int BrigadeClass::Save (VU_BYTE **stream)
	{
	GroundUnitClass::Save(stream);
	if (save_log)
	{
		fprintf (save_log, "%08x BrigadeClass ", *stream - start_save_stream);
		fflush (save_log);
	}

#ifdef CAMPTOOL
	if (gRenameIds)
		{
		for (int i=0; i<elements; i++)
			element[i].num_ = RenameTable[element[i].num_];
		}
#endif
	memcpy(*stream, &elements, sizeof(uchar));						*stream += sizeof(uchar);
	memcpy(*stream, element, sizeof(VU_ID)*elements);				*stream += sizeof(VU_ID)*elements;
	return GroundUnitClass::SaveSize();
	}

// event handlers
int BrigadeClass::Handle(VuFullUpdateEvent *event)
	{
	return (GroundUnitClass::Handle(event));
	}

int BrigadeClass::MoveUnit (CampaignTime time)
	{
	Unit			e;
	int				en,me,be,te,toorder,role;
	F4PFList		nearlist = NULL;
	Objective		o;
	
	// Check if we have a valid objective
	o = GetUnitObjective();
	if (!o || !TeamInfo[GetTeam()]->gtm->IsValidObjective(GetOrders(),o))
		{
		if (o && (GetOrders() == GORD_CAPTURE || GetOrders() == GORD_ASSAULT || GetOrders() == GORD_AIRBORNE))
			SetUnitOrders(GORD_SECURE,o->Id());
		else
			{
			o = FindRetreatPath(this,3,FIND_SECONDARYONLY);
			if (!o)
				{
				// We've been cut off - surrender?
				CheckForSurrender();
				return -1;
				}
			SetUnitOrders(GORD_RESERVE,o->Id());
			}
		}

	// Check if we have elements requesting orders
	role = GetUnitCurrentRole();
	toorder = te = 0;
	e = GetFirstUnitElement();
	while (e)
		{
		te++;
		if (!e->Assigned())
			toorder++;
		e = GetNextUnitElement();
		}
	if (!te)
		{
		KillUnit();
		return 0;
		}

	// Support brigades just update their position and return
	if (FindUnitSupportRole(this))
		{
		UpdateParentStatistics();
		return 0;
		}

	// Check to make sure our orders are still valid.
//	if (!CheckTactic(GetUnitTactic()))
	ChooseTactic();

	// Upon new orders, reset our element's ordered flags && collect list of possible positions
	if (Ordered() || toorder)
		{
		Objective	o;

		o = GetUnitObjective();
		if (role == GRO_ATTACK)
			nearlist = GetChildObjectives(o, MAXLINKS_FROM_SO_OFFENSIVE, FIND_STANDARDONLY);
		else
			nearlist = GetChildObjectives(o, MAXLINKS_FROM_SO_DEFENSIVE, FIND_STANDARDONLY);

		// Eliminate any objectives we've previously been unable to find a path to

		// Clear all but attack orders (once we attack, we only stop when we break)
		e = GetFirstUnitElement();
		while (e)
			{
			if (!e->Broken() && !e->Engaged() && e->Assigned() && e->GetUnitCurrentRole() != GRO_ATTACK)
				{
				e->SetAssigned(0);
				toorder++;
				}
			else if (e->Assigned() && !OnValidObjective(e, e->GetUnitCurrentRole(), nearlist))
				{
				e->SetAssigned(0);
				toorder++;
				}
			e = GetNextUnitElement();
			}

		// Loop in here until all our elements are assigned
		while (toorder)
			{
			// Order elements
			e = GetFirstUnitElement();
			while (e)
				{
				if (!e->Assigned())
					OrderElement(e, nearlist);
				e = GetNextUnitElement();
				}
			// Check for pre-empted elements
			for (en=toorder=0; en<te; en++)
				{
				e = GetUnitElement(en);
				if (e && !e->Assigned())
					toorder++;
				}
			}

		if (nearlist)
			{
			nearlist->DeInit();
			delete nearlist;
			nearlist = NULL;
			}
		}

	// Make sure at least somebody is doing our job
	for (me=be=en=0; en<te; en++)
	{
		e = GetUnitElement(en);
		if (e)
		{
			if (e->Broken())
			{
				be++;
			}
			else if (e->GetUnitCurrentRole() == role)
			{
				me++;
			}
		}
	}

	// Check for broken status
	if (be > me)
		SetBroken(1);

	// Check if we're still valid to perform our orders
	if (!me)
		{
		if (role == GRO_ATTACK)
			SetOrders (GORD_DEFEND);				// Switch to defense orders
		else if (GetOrders() != GORD_RESERVE)
			SetUnitObjective(FalconNullId);		// We'll pick a reserve location next time through
		}

	UpdateParentStatistics();

	return 0;
	}

int BrigadeClass::Reaction (CampEntity e, int zone, float range)
	{
	return 0;
	} 

int BrigadeClass::ChooseTactic (void)
	{
	int			priority=0,tid;

	haveWeaps = -1;
	tid = GTACTIC_BRIG_SECURE;
	while (tid < FirstGroundTactic + GroundTactics && !priority)
		{
		priority = CheckTactic(tid);
		if (!priority)
			tid++;
		}

	// Make Adjustments due to tactic
	if (tid == GTACTIC_BRIG_WITHDRAW)
		{
		Objective	o;
		o = GetUnitObjective();
		if (!o || !o->IsSecondary())
			{
			// Find a retreat path
			o = FindRetreatPath(this,3,FIND_SECONDARYONLY);
			if (o)
				SetUnitOrders(GORD_RESERVE,o->Id());
			// KCK: This will cause the whole brigade to surrender if the first element get's 
			// cutoff and we're withdrawing. So I'm axing it. Instead, we'll just wait until
			// the element surrenders.
//			else
//				CheckForSurrender();
			SetOrdered(1);
			}
		}
	if (GetUnitTactic() != tid)
		SetOrdered(1);
	SetUnitTactic(tid);
#ifdef ROBIN_GDEBUG
	if (TrackingOn[GetCampID()])
		MonoPrint("Brigade %d (%s) chose tactic %s.\n",GetCampID(),OrderStr[GetUnitOrders()],TacticsTable[tid].name);
#endif
	return tid;
	}

int BrigadeClass::CheckTactic (int tid)
	{
	Objective	o;

	if (tid < 1)
		return 0;
	if (haveWeaps < 0)
		{
		FalconEntity	*e;
		GridIndex		x,y,dx,dy;

		e = GetTarget();
		if (Engaged() && !e)
			SetEngaged(0);
		if (GetUnitSupply() > 20)
			haveWeaps = 1;
		else
			haveWeaps = 0;
		GetLocation(&x,&y);
		o = GetUnitObjective();
		ourObjOwner = 0;
		if (o && o->GetTeam() == GetTeam())
			ourObjOwner = 1;
		if (o)
			o->GetLocation(&dx,&dy);
		else
			GetUnitDestination(&dx,&dy);
		ourObjDist = FloatToInt32(Distance(x,y,dx,dy));
		}
	if (!CheckUnitType(tid, GetDomain(), GetType()))
		return 0;
	if (!CheckTeam(tid,GetTeam()))
		return 0;
	if (!CheckEngaged(tid,Engaged()))
		return 0;
	if (!CheckCombat(tid,Combat()))
		return 0;
	if (!CheckLosses(tid,Losses()))
		return 0;
	if (!CheckRetreating(tid,Retreating()))
		return 0;
	if (!CheckAction(tid,GetUnitOrders()))
		return 0;
	if (!CheckOwned(tid,ourObjOwner))
		return 0;
	if (TeamInfo[GetTeam()]->GetGroundAction()->actionType != GACTION_OFFENSIVE && !CheckRole(tid,0))
		return 0;
	if (!CheckRange(tid,ourObjDist))
		return 0;
//	if (!CheckDistToFront(tid,ourFrontDist))
//		return 0;
	if (!CheckStatus(tid,Broken()))
		return 0;
//	if (!CheckOdds(tid,odds))
//		return 0;
	return GetTacticPriority(tid);
	}

void BrigadeClass::SetUnitOrders (int neworders, VU_ID oid)
	{
	Objective	o,so;
	GridIndex	x,y,dx,dy;
	Unit		e;

	SetOrdered(1);
	SetUnitTactic(0);

	o = FindObjective(oid);

#ifdef DEBUG
	if (gDumping)
		{
		FILE	*fp;
		int		id1;
		char	buffer[256];
		char	name1[80],name2[80],timestr[80];

		sprintf(buffer,"campaign\\save\\dump\\%d.BRI",GetCampID());

		fp = fopen(buffer,"a");
		if (fp)
			{
			if (o)
				{
				o->GetName(name1,79,FALSE);
				id1 = o->GetCampID();
				}
			else
				{
				sprintf(name1,"NONE");
				id1 = 0;
				}
			GetName(name2,79,FALSE);
			GetTimeString(TheCampaign.CurrentTime,timestr);
			sprintf(buffer,"%s (%d) ordered to %s %s (%d) @ %s.\n",name2,GetCampID(),OrderStr[neworders],name1,id1,timestr);
			fprintf(fp,buffer);
			fclose(fp);
			}
		else
			gDumping = 0;
		}
#endif
	
	if (!o)
		return;

	o->GetLocation(&dx,&dy);
	GetLocation(&x,&y);
	if ((x != dx || y != dy) && GetMovementType() != NoMove)
		{
		SetMoving(1);
		SetUnitDestination(dx,dy);
		}
	SetTempDest(0);

	if (neworders == GetOrders() && oid == GetUnitObjectiveID())
		return;
	
	DisposeWayPoints();
	SetUnitObjective(oid);
	GroundUnitClass::SetUnitOrders(neworders);

	// Reset component ordered state
	e = GetFirstUnitElement();
	while (e)
		{
		e->SetAssigned(0);
		e = GetNextUnitElement();
		}

	// If this is near the front, send a low priority request for an enemy recon patrol
	if (o->IsNearfront())
		{
		MissionRequestClass		mis;
		mis.tot = Camp_GetCurrentTime() + (rand()%MIN_TASK_GROUND + 30) * CampaignMinutes;
		mis.vs = GetTeam();
		mis.tot_type = TYPE_NE;
		o->GetLocation(&mis.tx,&mis.ty);
		mis.targetID = Id();
		mis.mission = AMIS_RECONPATROL;
		mis.roe_check = ROE_AIR_OVERFLY;
		mis.RequestEnemyMission();
		}

	// Let's make sure our objective tree jives with our assignment
	if (o->IsSecondary())
		{
		so = o->GetObjectiveParent();
		if (so)
			SetUnitPrimaryObj(so->Id());
		SetUnitSecondaryObj(o->Id());
		}
	else if (o->IsPrimary())
		{
		SetUnitPrimaryObj(o->Id());
		SetUnitSecondaryObj(o->Id());
		}
	else
		{
		so = o->GetObjectiveParent();
		if (so)
			{
			SetUnitSecondaryObj(so->Id());
			o = so->GetObjectiveParent();
			if (o)
				SetUnitPrimaryObj(o->Id());
			}
		}
	}


void BrigadeClass::SetUnitDivision (int d)
	{
	Unit			e;

	SetDivision (d);
	e = GetFirstUnitElement();
	while (e)
		{
		e->SetUnitDivision(d);
		e = GetNextUnitElement();
		}
	}

// This calculates the maximum speed we can travel as a whole unit
int BrigadeClass::GetUnitSpeed (void)
	{
	int			speed = 9999;
	Battalion   e;

	e = (Battalion)GetFirstUnitElement();
	if (!e)
		return GetCruiseSpeed();
	while (e)
		{
		if (speed > e->GetCruiseSpeed())
			speed = e->GetCruiseSpeed();
		e = (Battalion)GetNextUnitElement();
		}
	if (speed > 9000)
		MonoPrint("Warning: Unit %d exceeding maximum warp!\n",GetCampID());
	return speed;
	}

int BrigadeClass::GetUnitSupply (void)
	{
	int			sup,els;
	Unit		e;

	sup = els = 0;
	e = GetFirstUnitElement();
	while (e)
		{
		els++;
		sup += e->GetUnitSupply();
		e = GetNextUnitElement();
		}
	if (els)
		sup /= els;
	return sup;
	}

int BrigadeClass::GetUnitMorale (void)
	{
	int			morale,els;
	Unit		e;

	morale = els = 0;
	e = GetFirstUnitElement();
	while (e)
		{
		els++;
		morale += e->GetUnitMorale();
		e = GetNextUnitElement();
		}
	if (els)
		morale /= els;
	return morale;
	}

int OnValidObjective (Unit e, int role, F4PFList nearlist)
	{
	VuListIterator	vuit(nearlist);
	Objective		bo = GetFirstObjective(&vuit);

	while (bo && bo->Id() != e->GetUnitObjectiveID())
		bo = GetNextObjective(&vuit);
	if (bo)
		{
		GridIndex	x,y;
		e->GetLocation(&x,&y);
		if (ScorePosition (e, role, 100, bo, x, y, (bo->GetTeam() == e->GetTeam())? 1:0) < -30000)
			bo = NULL;
		}
	if (bo)
		return 1;
	return 0;
	}

int GetNewRole (Unit e, Unit brig)
	{
	int		brole = brig->GetUnitCurrentRole();
	int		role = e->GetUnitNormalRole();
	if (e->Broken())
		role = GRO_RESERVE;
	// Swap special roles
	if (role == GRO_AIRBORNE || role == GRO_ASSAULT)
		role = brole;
	// Modify assignment role by normal role
	if (role == GRO_ATTACK && brole != GRO_ATTACK)
		role = brole;
	else if (role == GRO_DEFENSE && brole == GRO_ATTACK)
		role = GRO_ATTACK;
	return role;
	}

// Order elements will find the best _final_ positions for this brigades elements and order them to go there.
// It's up to the elements to determine the method of getting there.
int BrigadeClass::OrderElement(Unit e, F4PFList nearlist)
	{
	Objective		o,bo;
	int				neworders = GetUnitOrders();
	int				role;

	// Check to see if there's something more important to do
	o = GetUnitObjective();
	role = GetNewRole(e,this);
	// Determine orders by role
	neworders = GetGroundOrders(role);

	switch (role)
		{
		case GRO_ATTACK:
		case GRO_ASSAULT:
		case GRO_AIRBORNE:
		case GRO_RECON:
			// Assign to best offensive objective
			bo = FindBestPosition(e, this, GRO_ATTACK, nearlist);
			break;
		case GRO_DEFENSE:
		case GRO_ENGINEER:
		case GRO_AIRDEFENSE:
		case GRO_FIRESUPPORT:
		case GRO_RESERVE:
			bo = FindBestPosition(e, this, role, nearlist);
			break;
		default:
			// Assign to an objective out of the way
			bo = FindBestPosition(e, this, GRO_RESERVE, nearlist);
			break;
		}

	// If we didn't find a location, then look for a reserve location
	if (!bo)
		{
		neworders = GORD_RESERVE;
		bo = FindBestPosition(e, this, GRO_RESERVE, nearlist);
		}
	if (bo)
		o = bo;

	ShiAssert (o);

	e->SetUnitOrders(neworders,o->Id());
	e->SetAssigned(1);

//	ReorganizeUnit(e);		// This needs to only reposition element e
	ReorganizeUnit();

#ifdef ROBIN_GDEBUG
	if (TrackingOn[GetCampID()])
		{
		GridIndex x,y;
		o->GetLocation(&x,&y);
		MonoPrint("Battalion %d ordered to %s obj %d at %d,%d.\n",e->GetCampID(),OrderStr[neworders],o->GetCampID(),x,y);
		}
#endif
	return 1;
	}

Unit BrigadeClass::GetFirstUnitElement (void)
	{
	c_element = 0;
	return FindUnit(element[c_element]);
	}

Unit BrigadeClass::GetNextUnitElement (void)
	{
	c_element++;
	while (c_element < elements)
		{
		if (element[c_element])
			return FindUnit(element[c_element]);
		c_element++;
		}
	c_element=0;
	return NULL;
	}

Unit BrigadeClass::GetPrevUnitElement (Unit e)
	{
	for (int i=1; i<elements; i++)
		{
		if (element[i] == e->Id())
			return (Unit)vuDatabase->Find(element[i-1]);
		}
	return NULL;
	}

Unit BrigadeClass::GetUnitElement (int en)
	{
	if (en < elements && element[c_element])
		return (Unit)vuDatabase->Find(element[en]);
	return NULL;
	}

Unit BrigadeClass::GetUnitElementByID (int eid)
	{
	if (eid < elements)
		return (Unit)vuDatabase->Find(element[eid]);

	return NULL;
	}

void BrigadeClass::AddUnitChild (Unit e)
	{
	int		i=0;
	
	while (element[i] && i < MAX_UNIT_CHILDREN)
		i++;
	if (i < MAX_UNIT_CHILDREN)
		{
		element[i] = e->Id();
//		((Battalion)e)->SetUnitElement(i);
		}
	if (i >= elements)
		elements = i+1;
	}

void BrigadeClass::DisposeChildren (void)
	{
	Unit		e;

	while (element[0])
		{
		e = (Unit)vuDatabase->Find(element[0]);
		if (e)
			e->KillUnit();
		else
			RemoveChild(element[0]);
		}
	}

void BrigadeClass::RemoveChild (VU_ID	eid)
	{
	int		i=0,j;
	Unit	e;

	while (i < elements)
		{
		if (element[i] == eid)
			{
			for (j=i; j < MAX_UNIT_CHILDREN-1; j++)
				{
				element[j] = element[j+1];
				e = FindUnit(element[j]);
//				if (e)
//					((Battalion)e)->SetUnitElement(j);
				}
			element[j] = FalconNullId;
			elements--;
			}
		else
			i++;
		}
	}

int GetPriority (Unit e)
	{
	int		ep;

	if (!e)
		return 0;

	ep = OrderPriority[e->GetUnitOrders()] + OrderPriority[GetGroundOrders(e->GetUnitNormalRole())];
	if (e->Broken())
		ep /= 2;
	return ep;
	}

// Sorts the brigade's elements by relative importance
void BrigadeClass::ReorganizeUnit (void)
	{
	Unit		e,ne;
	int			i,j;

	for (i=0; i<elements; i++)
		{
		e = GetUnitElement(i);
		for (j=i; j<elements; j++)
			{
			ne = GetUnitElement(j);
			if (ne && e && GetPriority(ne) > GetPriority(e))
				{
				element[i] = ne->Id();
				element[j] = e->Id();
				e = ne;
				}
			}
		}
	}

/*
// This reorganizes a brigade's element battalions. If the unit is engaged it calls ReorganizeEngagedUnit instead.
// Otherwise it picks the best unit for the position.
void BrigadeClass::ReorganizeUnit (void)
	{
	Unit	e,te;
	int		ce=0,i;
	Unit	pos[GPOS_SUPPORT3+1];

#ifdef ROBIN_GDEBUG
	MonoPrint("Reorganizing brigade %d: ",GetCampID());
#endif
	SetOrdered(1);
	if (Engaged())
		{
		ReorganizeEngagedUnit();
		return;
		}

	memset(pos,0,sizeof(Unit)*GPOS_SUPPORT3+1);
	e = GetFirstUnitElement();
	if (!e)
		{
		// Dang, we're dead.
		SetDead(1);
		return;
		}
	while (e)
		{
		e->SetUnitPosition(0);
		e->SetAssigned(0);
		e = GetNextUnitElement();
		}

	// Fill each position with the best unit for the spot
	for (i=0,ce=0; i<=GPOS_SUPPORT3; i++)
		{
		e = NULL;
		if (i >= GPOS_RECON1 && i <= GPOS_RECON3)
			{
			e = BestElement(this, Foot, GRO_RECON);
			if (e && (e->GetUnitClassData())->Scores[GRO_RECON] < 5)
				{
				e->SetAssigned(0);
				e = NULL;
				}
			if (e)
				ce++;
			}
		else if (i >= GPOS_COMBAT1 && i <= GPOS_COMBAT3)
			{
			switch (GetUnitOrders())
				{
				case GORD_CAPTURE:
					e = BestElement(this, Foot, GRO_ATTACK);
					break;
				default:
					e = BestElement(this, Foot, GRO_DEFENSE);
					break;
				}
			if (e)
				ce++;
			}
		else if (i >= GPOS_RESERVE1 && i <= GPOS_RESERVE3)
			{
			e = BestElement(this, NoMove, GRO_RESERVE);
			}
		else if (i >= GPOS_SUPPORT1)
			{
			e = BestElement(this, Air, GRO_AIRDEFENSE);
			if (!e)
				{
				e = BestElement(this, Foot, GRO_FIRESUPPORT);
				if (!e)
					e = BestElement(this, Foot, GRO_ENGINEER);
				}
			if (!e)
				{
				e = GetFirstUnitElement();
				while (e && e->Assigned())
					e = GetNextUnitElement();
				}
			}
		pos[i] = e;
		if (e)
			e->SetUnitPosition(i);
		}
#ifdef 0
	// if we don't have any reserve units, but have recon or combat elements, assign one to reserve
	for (i=GPOS_COMBAT3; ce > 1 && i>GPOS_RECON1 && !pos[GPOS_RESERVE1]; i--)
		{
		if (pos[i])
			{
			pos[GPOS_RESERVE1] = pos[i];
			pos[i] = NULL;
			pos[GPOS_RESERVE1]->SetUnitPosition(GPOS_RESERVE1);
			ce--;
			}
		}
#endif
	// Check if we're broken (ie, no combat elements) (support only units?)
	if (!ce)
		SetBroken(1);

	// Clear out current child entries
	memset(element,0,sizeof(VU_ID)*MAX_UNIT_CHILDREN);
	// Now assign element #s sequentially
	for (i=1,ce=0,te=NULL; i<GPOS_SUPPORT3; i++)
		{
		if (pos[i])
			{
			element[ce] = pos[i]->Id();
			((Battalion)pos[i])->SetUnitElement(ce);
			te = pos[i];
#ifdef ROBIN_GDEBUG
			if (i == GPOS_COMBAT1 || i == GPOS_RESERVE1 || i == GPOS_SUPPORT1)
				MonoPrint("| ");
			MonoPrint("%d  ",pos[i]->GetCampID());
#endif
			ce++;
			}
		}
	// Special case: Only one subunit- assign it the 'zero' position
	if (te && ce==1)
		te->SetUnitPosition(0);
#ifdef ROBIN_GDEBUG
	MonoPrint("\n");
#endif
	}
*/

// This should move broken units to reserve, and unbroken reserves to combat positions
/*
void BrigadeClass::ReorganizeEngagedUnit (void)
	{
	Unit	e;
	int		ce=0,be=0,i,j,f;
	Unit	pos[GPOS_SUPPORT3+1];

	memset(pos,0,sizeof(Unit)*GPOS_SUPPORT3+1);
	// Find out which positions are taken
	e = GetFirstUnitElement();
	while (e)
		{
		i = e->GetUnitPosition();
		while(pos[i])
			i++;
		pos[i] = e;
		if (i <= GPOS_COMBAT3 && !e->Broken() && !e->Retreating())
			ce++;
		if (i <= GPOS_COMBAT3 && (e->Broken() || e->Retreating()))
			be++;
		e = GetNextUnitElement();
		}
	if (!ce)
		{
		SetBroken(1);
		return;
		}
	if (!be)
		return;					// Nothing to do

	// Move Broken units to reserve
	for (i=0; i<=GPOS_COMBAT3; i++)
		{
		if (pos[i] && pos[i]->Broken())
			{
			for (j=GPOS_RESERVE1,f=0; j<=GPOS_RESERVE3 && !f; j++)
				{
				if (!pos[j])
					{
					pos[j] = pos[i];
					pos[i] = (Unit)-1;
					f = 1;
					}
				}
			}
		}
	// Move Unbroken reserves to empty positions
	for (i=GPOS_RESERVE1; i<=GPOS_RESERVE3; i++)
		{
		if (pos[i] && !pos[i]->Broken() && !pos[i]->Retreating())
			{
			for (j=GPOS_RECON1,f=0; j<=GPOS_COMBAT3 && !f; j++)
				{
				if ((int)pos[j] == -1)
					{
					pos[j] = pos[i];
					pos[i] = NULL;
					f = 1;
					}
				}
			}
		}

	// Clear out current child entries and re-enter in order
	memset(element,0,sizeof(VU_ID)*MAX_UNIT_CHILDREN);
	for (i=1,ce=0; i<GPOS_SUPPORT3; i++)
		{
		if (pos[i] && (int)pos[i] != -1)
			{
			element[ce] = pos[i]->Id();
			((Battalion)pos[i])->SetUnitElement(ce);
			pos[i]->SetUnitPosition(i);
#ifdef ROBIN_GDEBUG
			MonoPrint("%d  ",pos[i]->GetCampID());
#endif
			ce++;
			}
		}
#ifdef ROBIN_GDEBUG
	MonoPrint("\n");
#endif
	}
*/

int BrigadeClass::UpdateParentStatistics (void)
	{
	GridIndex	nx,ny,x,y;
	int			engaged=0,combat=0,loss=0,te=0;
	Unit		e;

	// Update unit wide statistics. NOTE: some delay here- since elements are unmoved.
	nx = ny = te = 0;
	e = GetFirstUnitElement();
	while (e)
		{
		if (e->Engaged())
			engaged = 1;
		if (e->Combat())
			combat = 1;
		if (e->Losses())
			loss = 1;
		e->GetLocation(&x,&y);
//		nx += x;
//		ny += y;
		if (!nx && !ny)
			e->GetLocation(&nx,&ny);
		te++;
		e = GetNextUnitElement();
		}
	if (!te)
		{
		KillUnit();
		return 0;
		}
	if (!engaged)
		{
		SetEngaged(0);
		SetTarget(NULL);
		}
	SetEngaged(engaged);
	SetCombat(combat);
	SetLosses(loss);
	// Set our new averaged position
//	x = nx / te;
//	y = ny / te;
//	SetLocation(x,y);
	ShiAssert (nx && ny);
	// Set our position to our first element
	SetLocation(nx,ny);
	return te;
	}

int BrigadeClass::GetUnitSupplyNeed (int have)
	{
	int			supply=0;
	Unit		u;

	u = GetFirstUnitElement();
	while (u)
		{
		supply += u->GetUnitSupplyNeed(have);
		u = GetNextUnitElement();
		}
	return supply;
	}

int BrigadeClass::GetUnitFuelNeed (int have)
	{
	int			fuel = 0;
	Unit		u;

	u = GetFirstUnitElement();
	while (u)
		{
		fuel += u->GetUnitFuelNeed(have);
		u = GetNextUnitElement();
		}
	return fuel;
	}

void BrigadeClass::SupplyUnit (int supply, int fuel)
	{
	Unit		u;
	float		sr,fr;
	int			stu,ftu;

	// KCK: Actually - This function is probably never called
	// The problem is handled from within Supply.cpp -> SupplyUnit()
	sr = ((float)supply / (float)GetUnitSupplyNeed(FALSE));
	fr = ((float)fuel / (float)GetUnitFuelNeed(FALSE));

	u = GetFirstUnitElement();
	while (u)
		{
		stu = FloatToInt32(sr * u->GetUnitSupplyNeed(FALSE));
		ftu = FloatToInt32(fr * u->GetUnitFuelNeed(FALSE));
		u->SupplyUnit(stu,ftu);
		u = GetNextUnitElement();
		}
	}

int BrigadeClass::RallyUnit (int minutes)
	{
	Unit	e;
	int		rallied = 1;
	int		role, gotnon = 0;

	role = GetUnitNormalRole();
	e = GetFirstUnitElement();
	while (e)
		{
		if (e->RallyUnit(minutes))
			rallied = 0;
		if (role != GRO_FIRESUPPORT && e->GetUnitNormalRole() != GRO_FIRESUPPORT)
			gotnon = 1;
		e = GetNextUnitElement();
		}

	// KCK: We're going to convert Infantry/Armored/Etc Brigades which have lost all combat
	// battalions into another brigade type (ie: Artillery, if that's all that's left). This
	// Will allow the artillery to then do something usefull rather than sitting on reserve
	// missions
	if (!gotnon && role != GRO_FIRESUPPORT)
		{
		e = GetFirstUnitElement();
		if (e)
			SetUnitSType(e->GetSType());
		}

	if (rallied)
		SetBroken(0);

	return Broken();
	}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void BrigadeClass::SetFullStrength (short s)
{
	fullstrength = s;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
