#include <stdio.h>
#include <conio.h>
#include <stddef.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include "CmpGlobl.h"
#include "ListADT.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "ASearch.h"
#include "Path.h"
#include "Find.h"
#include "APITypes.h"
#include "Campaign.h"
#include "ATM.h"
#include "f4vu.h"
#include "CampList.h"
#include "campwp.h"
#include "update.h"
#include "loadout.h"
#include "navunit.h"
#include "tactics.h"
#include "Tacan.h"
#include "ClassTbl.h"
#include "Graphics\Include\TMap.h"
#include "PtData.h"
#include "Camp2Sim.h"
#include "Aircrft.h"
#include "radar.h"
#include "debuggr.h"

// ============================================
// Prototypes
// ============================================

WayPoint ResetCurrentWP (TaskForce tf);
WayPoint DoWPAction (TaskForce tf, WayPoint w);

// ============================================
// Defines and other nifty stuff
// ============================================

// ============================================
// Externals
// ============================================

extern unsigned char        SHOWSTATS;

extern CampaignHeading WithdrawUnit (Unit u);

extern VU_ID_NUMBER vuAssignmentId;
extern VU_ID_NUMBER vuLowWrapNumber;
extern VU_ID_NUMBER vuHighWrapNumber;
extern VU_ID_NUMBER lastNonVolitileId;
extern VU_ID_NUMBER lastLowVolitileId;
extern VU_ID_NUMBER lastVolitileId;

extern FILE
	*save_log,
	*load_log;

extern int
	start_save_stream,
	start_load_stream;

#ifdef DEBUG
extern int gCheckConstructFunction;
#endif

// =================================
// Smart heap pool stuff
// =================================

#ifdef USE_SH_POOLS
MEM_POOL	TaskForceClass::pool;
#endif

// ============================================
// TaskForce Class Functions
// ============================================

// KCK: ALL TASK FORCE CONSTRUCTION SHOULD USE THIS FUNCTION!
TaskForceClass* NewTaskForce (int type)
	{
	TaskForceClass	*new_taskforce;
#ifdef DEBUG
	gCheckConstructFunction = 1;
#endif
	VuEnterCriticalSection();
	lastVolitileId = vuAssignmentId;
	vuAssignmentId = lastNonVolitileId;
	vuLowWrapNumber = FIRST_NON_VOLITILE_VU_ID_NUMBER;
	vuHighWrapNumber = LAST_NON_VOLITILE_VU_ID_NUMBER;
	new_taskforce = new TaskForceClass (type);
	lastNonVolitileId = vuAssignmentId;
	vuAssignmentId = lastVolitileId;
	vuLowWrapNumber = FIRST_VOLITILE_VU_ID_NUMBER;
	vuHighWrapNumber = LAST_VOLITILE_VU_ID_NUMBER;
	VuExitCriticalSection();
#ifdef DEBUG
	gCheckConstructFunction = 0;
#endif
	return new_taskforce;
	}

TaskForceClass::TaskForceClass(int type) : UnitClass(type)
	{
#ifdef DEBUG
	ShiAssert (gCheckConstructFunction);
#endif
	orders = 0;
	supply = 100;
	air_target = FalconNullId;
	missiles_flying = 0;
	VU_TIME SEARCHtimer = 0;
	VU_TIME AQUIREtimer = 0;
	radar_mode = FEC_RADAR_OFF;
	search_mode = FEC_RADAR_OFF;
	last_combat = last_move = 0;
	last_direction = 0;
	SetParent(1);
	}

TaskForceClass::TaskForceClass(VU_BYTE **stream) : UnitClass(stream)
	{
	if (load_log)
	{
		fprintf (load_log, "%08x TaskForceClass ", *stream - start_load_stream);
		fflush (load_log);
	}

	memcpy(&orders, *stream, sizeof(uchar));				*stream += sizeof(uchar); 
	memcpy(&supply, *stream, sizeof(Percentage));			*stream += sizeof(Percentage); 
	air_target = FalconNullId;
	missiles_flying = 0;
	radar_mode = FEC_RADAR_OFF;
	search_mode = FEC_RADAR_OFF;
	last_combat = last_move = 0;
	last_direction = 0;

	if (GetSType() == STYPE_UNIT_CARRIER)
		{
		SetTacan(1);
		}
	}

TaskForceClass::~TaskForceClass (void)
	{
	if (IsAwake())
		Sleep();
	}

int TaskForceClass::SaveSize (void)
	{
	return UnitClass::SaveSize()
		+ sizeof(uchar) 
		+ sizeof(Percentage);
	}

int TaskForceClass::Save (VU_BYTE **stream)
	{
	UnitClass::Save(stream);
	if (save_log)
	{
		fprintf (save_log, "%08x TaskForceClass ", *stream - start_save_stream);
		fflush (save_log);
	}

	memcpy(*stream, &orders, sizeof(uchar));				*stream += sizeof(uchar); 
	memcpy(*stream, &supply, sizeof(Percentage));			*stream += sizeof(Percentage); 
	return TaskForceClass::SaveSize();
	}

// event handlers
int TaskForceClass::Handle(VuFullUpdateEvent *event)
	{
	// copy data from temp entity to current entity
	TaskForceClass* tmp_ent = (TaskForceClass*)(event->expandedData_);

	orders = tmp_ent->orders;
	supply = tmp_ent->supply;
	return (UnitClass::Handle(event));
	}

// This is the speed we're trying to go
int TaskForceClass::GetUnitSpeed (void)
	{
	return GetMaxSpeed();
	}

int TaskForceClass::GetVehicleDeagData (SimInitDataClass *simdata, int remote)
	{
	static CampEntity		ent;
	static int				round;
	int						dist, i, ptIndexAt;

	// Reinitialize static vars upon query of first vehicle
	if (simdata->vehicleInUnit < 0)
	{
		simdata->vehicleInUnit = 0;
		ent = NULL;

		if (!remote)
		{
			// Used only in port
			round = 0;
			simdata->ptIndex = GetDeaggregationPoint(0, &ent);
			if (simdata->ptIndex) {
				// Yuck!  The first call returns only the list index, NOT a real point index.
				// To ensure we have at least one set of points we have to actually query for them
				// then reset again...
				simdata->ptIndex = GetDeaggregationPoint(0, &ent);
				if (simdata->ptIndex) 
					simdata->ptIndex = GetDeaggregationPoint(0, &ent);
				ent = NULL;
				GetDeaggregationPoint(0, &ent);
			}

			// Used only at sea
			WayPoint			w;
			w = GetCurrentUnitWP();
			if (w)
			{
				// Find heading to next waypoint
				GridIndex	ux,uy,wx,wy;
				GetLocation(&ux,&uy);
				w->GetWPLocation(&wx,&wy);
				simdata->heading = AngleTo(ux,uy,wx,wy);
			}
		}
	}
	else
	{
		simdata->vehicleInUnit++;
	}

	if (!remote)
	{
		if (simdata->ptIndex) {
			// In port
			float	dx, dy;

			// Find the center point and direction point for this ship
			simdata->ptIndex = GetDeaggregationPoint(simdata->campSlot, &ent);
			ptIndexAt        = GetDeaggregationPoint(simdata->campSlot, &ent);

			if (!ptIndexAt)
			{
				ShiAssert( !simdata->ptIndex );	// We should always have an even number of points

				// Reuse the old points, but with an offset
				ent = NULL;
				GetDeaggregationPoint(simdata->campSlot, &ent);		// Reset
				simdata->ptIndex = GetDeaggregationPoint(simdata->campSlot, &ent);
				ptIndexAt        = GetDeaggregationPoint(simdata->campSlot, &ent);
				round++;
			}
			ShiAssert( ptIndexAt );		// We must have at least two points (center and toward)
			TranslatePointData(ent, simdata->ptIndex, &simdata->x, &simdata->y);

			// Face toward the "at" point
			dx = PtDataTable[ptIndexAt].yOffset - PtDataTable[simdata->ptIndex].yOffset; // KCK NOTE: axis' are reversed
			dy = PtDataTable[ptIndexAt].xOffset - PtDataTable[simdata->ptIndex].xOffset; // KCK NOTE: axis' are reversed
			simdata->heading = (float)atan2(dx,dy);

			// If we reused a point, shift our center point along the at vector
			simdata->x += dx * round;
			simdata->y += dy * round;
		} else {
			// At sea
			dist = (simdata->vehicleInUnit - 1) >> 2;
			switch ( (simdata->vehicleInUnit - 1) & 0x3 )
			{
			  case 0:
				simdata->x = XPos() - 1500.0f - 1500.0f * dist;
				simdata->y = YPos() - 1500.0f - 1500.0f * dist;
				break;
			  case 1:
				simdata->x = XPos() + 1500.0f + 1500.0f * dist;
				simdata->y = YPos() - 1500.0f - 1500.0f * dist;
				break;
			  case 2:
				simdata->x = XPos() + 1500.0f + 1500.0f * dist;
				simdata->y = YPos() + 1500.0f + 1500.0f * dist;
				break;
			  case 3:
			  default:
				simdata->x = XPos() - 1500.0f - 1500.0f * dist;
				simdata->y = YPos() + 1500.0f + 1500.0f * dist;
				break;
			}
		}
	}
	
	// We're always at sea level
	simdata->z = 0.0f;

	// Determine skill (Sim only uses it for anti-air stuff right now, so bow to expedience
	simdata->skill = ((TeamInfo[GetOwner()]->airDefenseExperience - 60) / 10) + rand()%3 - 1;
//	simdata->skill = ((TeamInfo[GetOwner()]->navalExperience - 60) / 10) + rand()%3 - 1;

	// Clamp it to legal sim side values
	if (simdata->skill > 4)
		simdata->skill = 4;
	if (simdata->skill < 0)
		simdata->skill = 0;

	// Weapon loadout
	for (i=0; i<HARDPOINT_MAX; i++)
		{
		simdata->weapon[i] = GetUnitWeaponId(i,simdata->campSlot);
		if (simdata->weapon[i])
			simdata->weapons[i] = GetUnitWeaponCount(i,simdata->campSlot);
		else
			simdata->weapons[i] = 0;
		}

	simdata->playerSlot = NO_PILOT;
	simdata->waypointList = CloneWPToList(GetFirstUnitWP(),NULL);

	return  MOTION_GND_AI;
	}	


int TaskForceClass::GetDeaggregationPoint (int slot, CampEntity *installation)
{
	int			pt=0,type;
	static int	last_pt, index = 0;
	
	if (!*installation)
	{
		// We're looking for a new list, so clear statics
		last_pt = index = 0;
		
		// Check if we care about placement
		if (!Moving())
		{
			// Find the appropriate installation
			GridIndex	x,y;
			Objective	o;
			GetLocation(&x,&y);
			o = FindNearestObjective (x,y,NULL,0);
			*installation = o;
			
			// Find the appropriate list
			if (o)
			{
				ObjClassDataType	*oc = o->GetObjectiveClassData();
				index = oc->PtDataIndex;
				while (index)
				{
					if (PtHeaderDataTable[index].type == DockListType)
					{
						// The first time we look, we just want to know if we have a list.
						// Return now.
						return index;
					}
					index = PtHeaderDataTable[index].nextHeader;
				}
#ifdef DEBUG
				FILE	*fp = fopen("PtDatErr.log","a");
				if (fp)
				{
					char		name[80];
					o->GetName(name,79,FALSE);
					fprintf(fp, "Obj %s @ %d,%d: No header list of type %d.\n",name,x,y,DockListType);
					fclose(fp);
				}
#endif
			}
		}
	}
	
	if (index)
	{
		// We have a list, and want to find the correct point
		UnitClassDataType		*uc = GetUnitClassData();
		VehicleClassDataType	*vc = GetVehicleClassData(uc->VehicleType[slot]);

		// Check which type of point we're looking for
		// TODO: Check ship type here...
//		type = SmallDockPt;
		type = LargeDockPt;

		// Return the next point, if it's the base type 
		// NOTE: Log error if we don't have enough points of this type
		if (last_pt)
		{
			last_pt = pt = GetNextPt(last_pt);
#ifdef DEBUG
			if (!pt || PtDataTable[pt].type != type)
			{
				FILE	*fp = fopen("PtDatErr.log","a");
				if (fp)
				{
					char		name[80];
					GridIndex	x,y;
					(*installation)->GetName(name,79,FALSE);
					(*installation)->GetLocation(&x,&y);
					fprintf(fp, "HeaderList %d (Obj %s @ %d,%d): Insufficient points of type %d.\n",index,name,x,y,type);
					fclose(fp);
				}
			}
#endif
			return pt;
		}

		// Find one of the appropriate type
		pt = GetFirstPt(index);
		while (pt)
		{
			if (PtDataTable[pt].type == type)
			{
				last_pt = pt;
				return pt;
			}
			pt = GetNextPt(pt);
		}
#ifdef DEBUG
		FILE	*fp = fopen("PtDatErr.log","a");
		if (fp)
		{
			char		name[80];
			GridIndex	x,y;
			(*installation)->GetName(name,79,FALSE);
			(*installation)->GetLocation(&x,&y);
			fprintf(fp, "HeaderList %d (Obj %s @ %d,%d): No points of type %d.\n",index,name,x,y,type);
			fclose(fp);
		}
#endif
	}
	return pt;
}


int TaskForceClass::MoveUnit (CampaignTime time)
	{
	GridIndex		x=0,y=0,nx=0,ny=0,ox=0,oy=0;
	WayPoint		w=NULL,ow=NULL;
	Objective		o=NULL;
	int				moving=1;
	CampaignHeading	h=0;

	// Naval units have two modes:
	// (a) Sit still in harbor
	// (b) Do a 20 km track (repeating waypoints)
	GetLocation(&x,&y);

	// Check for mode a
	o = FindNearestObjective(x,y,NULL,1);
	if (o && o->GetType() == TYPE_PORT)
		return TRUE;

	// Else mode b
	w = ResetCurrentWP(this);
	if (!w)
		{
		// ohoh, no waypoints - probably because we havn't built any yet.
		// Simple solution, create a repeating path 20 km north and back
		DisposeWayPoints();
		// 'Here'
		w = AddUnitWP(x, y, 0, 60,	TheCampaign.CurrentTime, 0, 0);	
		//MI we need to set a flag for each WP
		w->SetWPFlags(WPF_REPEAT);
		// 'There'
		w = AddUnitWP(x, y+20, 0, 60, TheCampaign.CurrentTime + 20*CampaignMinutes, 0, 0);
		//MI we need to set a flag for each WP
		w->SetWPFlags(WPF_REPEAT);
		// 'Here' again
		//MI this aint no Racetrack!
		//w = AddUnitWP(x, y+20, 0, 60, TheCampaign.CurrentTime + 40*CampaignMinutes, 0xffffffff, 0);		
		w = AddUnitWP(x, y, 0, 60, TheCampaign.CurrentTime + 40*CampaignMinutes, 0xffffffff, 0);		
		w->SetWPFlags(WPF_REPEAT);
		SetCurrentWaypoint (1);
		w = GetCurrentUnitWP();
		}
	w->GetWPLocation(&nx,&ny);

	// Move, if we're not at destination
	if (x!=nx || y!=ny)
		{
		if (w)
			ow = w->GetPrevWP();
		if (ow)			
			ow->GetWPLocation(&ox,&oy);
		else
			GetLocation(&ox,&oy);
		while (moving)
			{
			h = DirectionTo(ox,oy,nx,ny,x,y);
			if (h > 7)
				{
				moving = 0;
				h = Here;
				}
			// This is kinda hacky - basically, limit change in direction to 45 deg per move
			if (h > last_direction)
				{
				if (h - last_direction < 5)
					h = (last_direction+1) & 0x07;
				else
					h = (last_direction+7) & 0x07;
				}
			else if (h < last_direction)
				{
				if (last_direction - h < 5)
					h = (last_direction+7) & 0x07;
				else
					h = (last_direction+1) & 0x07;
				}

			if (ChangeUnitLocation(h)>0)
				last_direction = h;
			else
				moving = 0;

			// Now do combat
			if (GetCombatTime() > CombatTime())
				DoCombat();
			}
		}
	return 0;
	}

int TaskForceClass::DoCombat (void)
{
    int combat;
	SetCombatTime(TheCampaign.CurrentTime);

#if 0 // JPO mthis stuff now done in Choose Target - like Battalion
	// KCK: Super simple targetting (c)
	Team				who = GetTeam();
	CampEntity			e;
	FalconEntity		*react_against=NULL,*air_react_against=NULL;
	int					react,spot,best_reaction=1,best_air_react=1;
	int					search_dist;
	float				react_distance,air_react_distance,d;
	react_distance = air_react_distance = 9999.0F;

	SetEngaged(0);
	SetCombat(0);
	SetChecked();

	search_dist = GetDetectionRange(Air);
#ifdef VU_GRID_TREE_Y_MAJOR
	VuGridIterator detit(RealUnitProxList,YPos(),XPos(),(BIG_SCALAR)GridToSim(search_dist));
#else
	VuGridIterator detit(RealUnitProxList,XPos(),YPos(),(BIG_SCALAR)GridToSim(search_dist));
#endif
	e = (CampEntity)detit.GetFirst();
	while (e)
		{
		if (GetRoE(who,e->GetTeam(),ROE_GROUND_FIRE) == ROE_ALLOWED)
			{
			combat = 0;
			react = DetectVs(e,&d,&combat,&spot);
			if (!e->IsFlight() && react >= best_reaction && d < react_distance)
				{
				// React vs a ground/Naval target
				best_reaction = react;
				react_distance = d;
				react_against = e;
				SetEngaged(1);
				SetCombat(combat);
				}
			else if (e->IsFlight() && react >= best_air_react && d < air_react_distance)
				{
				// React vs an air target -
				best_air_react = react;
				air_react_distance = d;
				air_react_against = e;
				if (!e->IsAggregate())
					{
					// Pick a specific aircraft in the flight if it's deaggregated
					CampEnterCriticalSection();
					if (e->GetComponents())
						{
						VuListIterator	cit(e->GetComponents());
						FalconEntity	*fe;
						float			rsq,brsq=FLT_MAX;
		
						fe = (FalconEntity *)cit.GetFirst();
						while (fe)
							{
							rsq = DistSqu(XPos(),YPos(),fe->XPos(),fe->YPos());
							if (rsq < brsq)
								{
								air_react_against = fe;
								air_react_distance = (float)sqrt(rsq);
								brsq = rsq;
								}
							fe = (FalconEntity *)cit.GetNext();
							}
						}
					CampLeaveCriticalSection();
					}
				SetEngaged(1);
				SetCombat(combat);
				}
			}
		e = (CampEntity)detit.GetNext();
		}

	if (air_react_against)
		SetAirTarget(air_react_against);
	if (react_against)
		SetTarget(react_against);
#endif
	if (Engaged())
		{
		FalconEntity	*e = GetTarget();
		FalconEntity	*a = GetAirTarget();

		// Check vs our Ground Target
		if (!e)											
			SetTarget(NULL);
		else
			{
			if (Combat() && IsAggregate())
				{
				combat = ::DoCombat(this,e);
				if (combat <= 0 || Targeted())
					SetTargeted(0);
				}
			}
		// Check vs our Air Target
		if (!a)
			SetAirTarget(NULL);
		else if (Combat() && IsAggregate())
			{
			combat = ::DoCombat(this,a);
			if (combat < 0)
				SetAirTarget(NULL);							// Clear targeting data so we can look for another
			}
		}

	return 0;
	}

int TaskForceClass::Reaction (CampEntity e, int knowledge, float range)
	{
	int			score = 0, enemy_threat_bonus = 1;
	CampEntity	et = NULL;
	MoveType	tmt,omt;

	if (!e)	return 0;

	// Some basic info on us.
	omt = GetMovementType();
	tmt = e->GetMovementType();

	// Aircraft on ground are ignored (technically, we could shoot at them.. but..)
	if (e->IsFlight() && !((Flight)e)->Moving())
		return 0;

	// Score their threat to us
	if (knowledge & FRIENDLY_DETECTED)
		enemy_threat_bonus++;
	if (knowledge & FRIENDLY_IN_RANGE)
		enemy_threat_bonus += 2;
	et = ((Unit)e)->GetCampTarget();

	// All units score vs enemy engaged with this unit
	if (et == this)
		score += e->GetAproxHitChance(omt,0)/5 * enemy_threat_bonus;

	// Bonus if we can shoot them
	if (knowledge & ENEMY_IN_RANGE)
		score += GetAproxHitChance(tmt,FloatToInt32(range/2.0F))/5;

	// Added bonus for them attacking
	if (et && (tmt == Air || tmt == LowAir))
		score += GetAproxHitChance(tmt,0)/5 * enemy_threat_bonus;

	return score;
	}

/* 2002-02-11 COMMENTED OUT BY S.G. TOO MANY CHANGES TO TRACK THEM ALL (LIKE FOR OTHER UNIT TYPES)
int TaskForceClass::DetectVs (AircraftClass *ac, float *d, int *combat, int *spot)
	{
	int			react,det = Detected(this,ac,d);
	CampEntity	e;

	if (!(det & REACTION_MASK))
		return 0;
	
	e = ac->GetCampaignObject();
	react = Reaction(e,det,*d);
	if (det & ENEMY_IN_RANGE && react)
		*combat = 1;
	if (det & FRIENDLY_DETECTED)
		{
		SetSpotted(e->GetTeam(),TheCampaign.CurrentTime);
		*spot = 1;
		}
	return react;
	}
*/		
		
extern int CheckValidType(CampEntity u, CampEntity e);
extern int CanItIdentify(CampEntity us, CampEntity them, float d, int mt);

int TaskForceClass::DetectVs (AircraftClass *ac, float *d, int *combat, int *spot)
{
	int			react,det = Detected(this,ac,d);
	CampEntity	e;

	e = ac->GetCampaignObject();

// 2001-03-22 ADDED BY S.G. DETECTION DOESN'T INCLUDED SPOTTED, ONLY THAT THIS ENTITY DETECTED THE OTHER BY ITSELF.
	int detTmp = det;

	// Check type of entity before GCI is used
	if (CheckValidType(this, e))
		detTmp |= e->GetSpotted(GetTeam()) ? ENEMY_DETECTED : 0;

	// Check type of entity before GCI is used
	if (CheckValidType(e, this))
		detTmp |= GetSpotted(e->GetTeam()) ? FRIENDLY_DETECTED : 0;

	if (!(detTmp & REACTION_MASK))
		return 0;
	
	react = Reaction(e,detTmp,*d);

	if (det & ENEMY_IN_RANGE && react)
		*combat = 1;

	// Spotting will be set only if our enemy is aggregated or if he's an AWAC. SensorFusion or GroundClass::Exec will hanlde deaggregated vehicles.
	// I can't let SensorFusion handle the spotting for AWAC because this will put a too big toll on the CPU
	// e has to be a flight since it is derived from an aircraft class so less checks needs to be done here then against flights below
	if (det & FRIENDLY_DETECTED) {
		// Spotting will be set only if our enemy is aggregated or if he's an AWAC. SensorFusion or GroundClass::Exec will hanlde deaggregated vehicles.
		if ((e->IsAggregate() && CheckValidType(e, this)) || (e->IsFlight() && e->GetSType() == STYPE_UNIT_AWACS)) {
			SetSpotted(e->GetTeam(),TheCampaign.CurrentTime, CanItIdentify(this, e, *d, e->GetMovementType())); // 2002-02-11 MODIFIED BY S.G. Added 'CanItIdentify' which query if the target can be identified
			*spot = 1;
		}
	}
	return react;
}

/* 2002-02-11 COMMENTED OUT BY S.G. TOO MANY CHANGES TO TRACK THEM ALL (LIKE FOR OTHER UNIT TYPES)
int TaskForceClass::DetectVs (CampEntity e, float *d, int *combat, int *spot)
	{
	int		react,det;

	det = Detected(this,e,d);
	if (!(det & REACTION_MASK))
		return 0;
	react = Reaction(e,det,*d);
	if (det & ENEMY_DETECTED)
		e->SetSpotted(GetTeam(),TheCampaign.CurrentTime);
	if (det & ENEMY_IN_RANGE && react)
		*combat = 1;
	if (det & FRIENDLY_DETECTED)
		{
		SetSpotted(e->GetTeam(),TheCampaign.CurrentTime);
		*spot = 1;
		}

	return react;
	}

*/
int TaskForceClass::DetectVs (CampEntity e, float *d, int *combat, int *spot)
	{
	int		react,det;

	det = Detected(this,e,d);

	int detTmp = det;

	// Check type of entity before GCI is used
	if (CheckValidType(this, e))
		detTmp |= e->GetSpotted(GetTeam()) ? ENEMY_DETECTED : 0;

	// Check type of entity before GCI is used
	if (CheckValidType(e, this))
		detTmp |= GetSpotted(e->GetTeam()) ? FRIENDLY_DETECTED : 0;

	if (!(detTmp & REACTION_MASK))
		return 0;

	react = Reaction(e,detTmp,*d);

	// We'll spot our enemy if we're not broken
	if (det & ENEMY_DETECTED) {
		if (IsAggregate() && CheckValidType(this, e))
			e->SetSpotted(GetTeam(),TheCampaign.CurrentTime, (CanItIdentify(this, e, *d, e->GetMovementType()))); // 2002-02-11 MODIFIED BY S.G. Say 'identified if it has the hability to identify
	}
	if (det & ENEMY_IN_RANGE && react)
		*combat = 1;
	if (det & FRIENDLY_DETECTED) {
		// Spotting will be set only if our enemy is aggregated or if he's an AWAC. SensorFusion or GroundClass::Exec will hanlde deaggregated vehicles.
		if ((e->IsAggregate() && CheckValidType(e, this)) || (e->IsFlight() && e->GetSType() == STYPE_UNIT_AWACS)) {
			SetSpotted(e->GetTeam(),TheCampaign.CurrentTime, 1); // 2002-02-11 Modified by S.G. Ground units are always identified (doesn't change a thing)
			*spot = 1;
		}
	}

	return react;
	}


int TaskForceClass::ChooseTactic (void)
	{
	return NULL;
	}

int TaskForceClass::CheckTactic (int tid)
	{
	return 0;
	}

CampaignTime TaskForceClass::GetMoveTime (void)
	{
	if (last_move && TheCampaign.CurrentTime > last_move)
		return TheCampaign.CurrentTime - last_move;
	
	last_move = TheCampaign.CurrentTime;

	return 0;
	}

void TaskForceClass::GetRealPosition (float *x, float *y, float *z)
	{
	// This will use the last move time to determine the real x,y & z of the unit
	float			movetime = (float)(SimLibElapsedTime - last_move) / VU_TICS_PER_SECOND;
	float			speed;
	float			heading;
	float			dist;
	int				h = GetNextMoveDirection();
	mlTrig			sincos;

	if (h < 0 || h > 7 || SimLibElapsedTime < last_move)
		{
		*x = XPos();
		*y = YPos();
		*z = TheMap.GetMEA(XPos(),YPos());
		return;
		}

	speed = (float) GetUnitSpeed() * KPH_TO_FPS;
	dist = speed * movetime;
	heading = h * 45.0F * DTR;
	mlSinCos(&sincos, heading);
	*x = XPos() + dist * sincos.cos;
	*y = YPos() + dist * sincos.sin;
	*z = TheMap.GetMEA(XPos(),YPos());
	}

// ===========================================
// Support Functions
// ===========================================

WayPoint ResetCurrentWP (TaskForce tf)
	{
	WayPoint	w;
	GridIndex	x,y,ux,uy;

	w = tf->GetCurrentUnitWP();
	while (w && w->GetWPDepartureTime() < Camp_GetCurrentTime())
		{
		if (w->GetWPFlags())				// Make sure we actually get here, it's important
			{
			w->GetWPLocation(&x,&y);
			tf->GetLocation(&ux,&uy);
			if (DistSqu(x,y,ux,uy) > 2.0F)
				return w;
			if (DoWPAction(tf,w))
				return tf->GetCurrentUnitWP();
			}
		w = w->GetNextWP();
		tf->SetCurrentUnitWP(w);
		}
	return w;
	}

// Unit within operation area, needs to take it's Waypoint's action
// This currently is only called if this waypoint has a flag set
WayPoint DoWPAction (TaskForce tf, WayPoint w)
	{
	WayPoint	cw;

	if (!w || !tf)
		return NULL;

	// Check Actions
/*	int action = w->GetWPAction();
    switch (action)
		{
		case WP_NOTHING:
		default:
            break;
		}
*/
	
	// Check Flags
	if (w->GetWPFlags() & WPF_REPEAT)
		{
		// Check if we've been here long enough
		if (Camp_GetCurrentTime() > w->GetWPDepartureTime())
			{
			// If so, go on to the next wp (adjust their times from now)
			tf->AdjustWayPoints();
			}
		else
			{
			// If not, restore previous WP and readjust times
			cw = w->GetPrevWP();
			tf->SetCurrentUnitWP(cw);
			tf->AdjustWayPoints();
			return cw;
			}
		}

	return NULL;
	}

//MI added function for movement
int TaskForceClass::DetectOnMove (void)
{
	if (!Engaged() && !(GetUnitMoved()%5))
		return 0;
	return ChooseTarget();
}

// JPO addtions
int TaskForceClass::CanShootWeapon (int wid)
{
    if(WeaponDataTable[wid].GuidanceFlags & WEAP_RADAR && missiles_flying > 1)
	return FALSE;
    // Check for radar guidance, and make adjustments if necessary
    if (!(WeaponDataTable[wid].GuidanceFlags & WEAP_RADAR) || GetRadarMode() == FEC_RADAR_GUIDE || GetRadarMode() == FEC_RADAR_SEARCH_100)
	return TRUE;
    return FALSE;
}

void TaskForceClass::ReturnToSearch (void)
{
    if (missiles_flying < 1 && IsEmitting())
    {
	radar_mode = search_mode;
	if (radar_mode == FEC_RADAR_OFF)
	    SetEmitting(0);
    }
    else if (!IsEmitting())
	radar_mode = FEC_RADAR_OFF;
}

int TaskForceClass::StepRadar(int t, int d, float range)//me123 modifyed to take tracking/detection parameter
	{
	int	radMode = GetRadarMode();
	if (IsAggregate())
		{
		// Check if we still have any radar vehicles
		if (class_data->RadarVehicle == 255 || !GetNumVehicles(class_data->RadarVehicle))
			return FEC_RADAR_OFF;

		// Check if we're already in our fire state
		if (radMode == FEC_RADAR_GUIDE || radMode == FEC_RADAR_SEARCH_100)
			return radMode;

		// Check for switch over to guide
		if (radMode == FEC_RADAR_AQUIRE)
			{
			SetRadarMode(FEC_RADAR_GUIDE);
			return FEC_RADAR_GUIDE;
			}
		else
			{
			SetRadarMode(FEC_RADAR_AQUIRE);
			// KCK: Good operators could shoot before going to guide mode. Check skill and return TRUE
			if (GetRadarMode() == FEC_RADAR_AQUIRE && rand()%100 < TeamInfo[GetOwner()]->airDefenseExperience - MINIMUM_EXP_TO_FIRE_PREGUIDE)
				SetRadarMode(FEC_RADAR_GUIDE);
			return GetRadarMode();
			}
		}

	assert (range);
/*
FEC_RADAR_OFF			0x00	   	// Radar always off
FEC_RADAR_SEARCH_100	0x01	   	// Search Radar - 100 % of the time (always on)
FEC_RADAR_SEARCH_1		0x02	   	// Search Sequence #1
FEC_RADAR_SEARCH_2		0x03	   	// Search Sequence #2
FEC_RADAR_SEARCH_3		0x04	   	// Search Sequence #3
FEC_RADAR_AQUIRE		0x05	   	// Aquire Mode (looking for a target)
FEC_RADAR_GUIDE			0x06	   	// Missile in flight. Death is imminent*/


	// Check if we still have any radar vehicles
	if (class_data->RadarVehicle == 255 || !GetNumVehicles(class_data->RadarVehicle ))
		return FEC_RADAR_OFF;

	assert(radarDatFileTable != NULL);
	RadarDataSet* radarData = &radarDatFileTable[((VehicleClassDataType *)Falcon4ClassTable[class_data->VehicleType[class_data->RadarVehicle]].dataPtr)->RadarType];


	// Check if we're already in our fire state
	if (radMode == FEC_RADAR_SEARCH_100)
		return radMode;

	// Check for switch over to guide
	float skill = TeamInfo[GetOwner()]->airDefenseExperience/30.0f*1000; // from 1 - 3
	skill *= (float)radarData->Timeskillfactor;
	skill /=100.0f;
	float timetosearch ;
	float timetoaquire ;
	if (!d && !t) SetRadarMode(search_mode);

	if (GetRadarMode() == FEC_RADAR_CHANGEMODE && search_mode >= FEC_RADAR_SEARCH_1 )
	SetRadarMode(search_mode);// we are changing mode.. realy not off
	
	switch (GetRadarMode())
	{
	case FEC_RADAR_OFF:
	timetosearch = radarData->Timetosearch1 - skill;
	if (range <= radarData->Rangetosearch1 && !SEARCHtimer) SEARCHtimer = SimLibElapsedTime;
	else if (range >= radarData->Rangetosearch1 || SimLibElapsedTime - SEARCHtimer > timetosearch+6000.0f)SEARCHtimer = 0;
	if (range <= radarData->Rangetosearch1 && SEARCHtimer && SimLibElapsedTime - SEARCHtimer > timetosearch )
	{
	SEARCHtimer = SimLibElapsedTime;
	search_mode = FEC_RADAR_SEARCH_1 ;
	SetRadarMode(FEC_RADAR_SEARCH_1);
	}
		break;
	case FEC_RADAR_SEARCH_1:
		AQUIREtimer = SimLibElapsedTime;
		if (!SEARCHtimer) SEARCHtimer = SimLibElapsedTime;
		timetosearch = radarData->Timetosearch1 - skill;
		if (d && range <= radarData->Rangetosearch2 && SimLibElapsedTime - SEARCHtimer >= timetosearch)  
		{
			SetRadarMode(FEC_RADAR_CHANGEMODE);
			search_mode = FEC_RADAR_SEARCH_2;
			SEARCHtimer = SimLibElapsedTime;
		}
		break;
	case FEC_RADAR_SEARCH_2:
		AQUIREtimer = SimLibElapsedTime;
	    timetosearch = radarData->Timetosearch2 - skill;
		if (!SEARCHtimer) SEARCHtimer = SimLibElapsedTime;
		if (d && range <= radarData->Rangetosearch3 &&  SimLibElapsedTime - SEARCHtimer >= timetosearch)
		{
			search_mode =FEC_RADAR_SEARCH_3;
			SetRadarMode(FEC_RADAR_CHANGEMODE);
			SEARCHtimer = SimLibElapsedTime;
		}
		else if (!d)// no detection step search down
		{
		SetRadarMode(FEC_RADAR_CHANGEMODE);
		search_mode = FEC_RADAR_SEARCH_1 ;
		SEARCHtimer = SimLibElapsedTime;
		}
		break;
	case FEC_RADAR_SEARCH_3:
		AQUIREtimer = SimLibElapsedTime;
	    timetosearch = radarData->Timetosearch3 - skill;
		if (!SEARCHtimer) SEARCHtimer = SimLibElapsedTime;		
		// goto aquire ?
		if (d && range <= radarData->Rangetoacuire && SimLibElapsedTime - SEARCHtimer >= timetosearch )
		{
		search_mode =FEC_RADAR_AQUIRE;
		SetRadarMode(FEC_RADAR_CHANGEMODE);
		AQUIREtimer = SimLibElapsedTime;		
		}
		else if(!d)//  no detection step search down
		{
		SetRadarMode(FEC_RADAR_CHANGEMODE);
		search_mode = FEC_RADAR_SEARCH_2 ;
		SEARCHtimer = SimLibElapsedTime;
		}
		break;
	case FEC_RADAR_AQUIRE:
		SEARCHtimer = 0;
		timetoaquire = radarData->Timetoacuire - skill;
// only allow to be in aquire for the coast amount of time
		if (!t && !d && SimLibElapsedTime - AQUIREtimer >= (unsigned)radarData->Timetocoast)
		{
		SetRadarMode(FEC_RADAR_CHANGEMODE);
		search_mode = FEC_RADAR_SEARCH_3 ; 
		}
		else if (t && range <= radarData->Rangetoguide && SimLibElapsedTime - AQUIREtimer >= timetoaquire)
		{
		search_mode =FEC_RADAR_GUIDE;
		SetRadarMode(FEC_RADAR_CHANGEMODE);
		return FEC_RADAR_GUIDE;
		}
		break;
	case FEC_RADAR_GUIDE:
		AQUIREtimer = SimLibElapsedTime;
		search_mode = FEC_RADAR_AQUIRE;
		if (!t) SetRadarMode(FEC_RADAR_CHANGEMODE);
		break;
	}



/*	else if (SimLibElapsedTime - AQUIREtimer > timetoaquire)
		{
		// KCK: Good operators could shoot before going to guide mode. Check skill and return TRUE
		if (GetRadarMode() == FEC_RADAR_AQUIRE && rand()%100 < TeamInfo[GetOwner()]->airDefenseExperience - MINIMUM_EXP_TO_FIRE_PREGUIDE)
		{
		    search_mode = FEC_RADAR_AQUIRE ;
			SetRadarMode(FEC_RADAR_GUIDE);
		}
		}
*/
	int out = GetRadarMode();
	if (out == FEC_RADAR_OFF) out = search_mode;
	return out;
	}

int TaskForceClass::ChooseTarget (void)
{
    FalconEntity	*artTarget,*react_against=NULL,*air_react_against=NULL;
    CampEntity		e;
    float		d,react_distance,air_react_distance;
    int			react,best_reaction=1,best_air_react=1,combat,retval=0,pass=0,spot=0,estr=0,capture=0,nomove=0;
    int			search_dist;
    Team		who;
    
    if (IsChecked())
	return Engaged();
    
    who = GetTeam();
    react_distance = air_react_distance = 9999.0F;
    
#ifdef DEBUG
    DWORD				timec = GetTickCount();
#endif
    
    // Special case for fire support
    if (Targeted())
	artTarget = GetTarget();					// Save our target
    else
	artTarget = NULL;
    
    SetEngaged(0);
    SetCombat(0);
    SetChecked();
    
    search_dist = GetDetectionRange(Air);
    if (search_dist < MAX_GROUND_SEARCH)
	search_dist = MAX_GROUND_SEARCH;
#ifdef VU_GRID_TREE_Y_MAJOR
    VuGridIterator detit(RealUnitProxList,YPos(),XPos(),(BIG_SCALAR)GridToSim(search_dist));
#else
    VuGridIterator detit(RealUnitProxList,XPos(),YPos(),(BIG_SCALAR)GridToSim(search_dist));
#endif
//  CalculateSOJ(detit); 2002-02-19 REMOVED BY S.G. eFalcon 1.10 SOJ code removed

    e = (CampEntity)detit.GetFirst();
    while (e)
    {
	if (GetRoE(who,e->GetTeam(),ROE_GROUND_FIRE) == ROE_ALLOWED)
	{
	    combat = 0;
	    react = DetectVs(e,&d,&combat,&spot);
	    if (!e->IsFlight() && react >= best_reaction && d < react_distance)
	    {
		// React vs a ground/Naval target
		best_reaction = react;
		react_distance = d;
		react_against = e;
		SetEngaged(1);
		SetCombat(combat);
	    }
	    else if (e->IsFlight() && react >= best_air_react && d < air_react_distance)
	    {
		// React vs an air target -
		best_air_react = react;
		air_react_distance = d;
		air_react_against = e;
		if (!e->IsAggregate())
		{
		    // Pick a specific aircraft in the flight if it's deaggregated
		    CampEnterCriticalSection();
		    if (e->GetComponents())
		    {
			VuListIterator	cit(e->GetComponents());
			FalconEntity	*fe;
			float			rsq,brsq=FLT_MAX;
			
			fe = (FalconEntity *)cit.GetFirst();
			while (fe)
			{
			    rsq = DistSqu(XPos(),YPos(),fe->XPos(),fe->YPos());
			    if (rsq < brsq)
			    {
				air_react_against = fe;
				air_react_distance = (float)sqrt(rsq);
				brsq = rsq;
			    }
			    fe = (FalconEntity *)cit.GetNext();
			}
		    }
		    CampLeaveCriticalSection();
		}
		// Make sure our radar is on (if we have one)
		if (!IsEmitting() && class_data->RadarVehicle < 255 && GetNumVehicles(class_data->RadarVehicle))
		    SetEmitting(1);
		SetEngaged(1);
		SetCombat(combat);
	    }
	}
	e = (CampEntity)detit.GetNext();
    }
    SetOdds ((GetTotalVehicles() * 10) / (estr+10));
    
    if (!Parent() && best_reaction > 1)
	EngageParent(this,react_against);
    
    if (air_react_against)
    {
	SetAirTarget(air_react_against);
	retval = 1;
    }
    if (react_against)
    {
	SetTarget(react_against);
	SetTargeted(0);
	retval = 1;
    }
    else if (artTarget && (!artTarget->IsUnit() || ((Unit)artTarget)->Engaged()) && orders == GORD_SUPPORT)
    {
	// Keep blowing away this target until the target gets out of range, disengages, or we get new orders
	// (Target will get reset after a null DoCombat result)
	SetTarget(artTarget);
	SetTargeted(1);
	SetEngaged(1);
	SetCombat(1);
	return -1;			// We want to sit here and shoot until we can't any longer
    }
    if (nomove)
	return -1;
    return retval;
}
