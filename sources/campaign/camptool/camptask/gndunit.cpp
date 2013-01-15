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
#include "Campaign.h"
#include "update.h"
#include "loadout.h"
#include "gndunit.h"
#include "team.h"
#include "GTM.h"
#include "MsgInc\GndTaskingMsg.h"
#include "AIInput.h"
#include "Mission.h"
#include "classtbl.h"
#include "Aircrft.h"
#include "FalcSess.h"



// ============================================
// Externals
// ============================================

extern int HDelta[7];
extern MissionDataType MissionData [AMIS_OTHER];
extern int FriendlyTerritory (GridIndex x, GridIndex y, int team);
extern int maxSearch;

#ifdef DEBUG
#include "CampStr.h"
extern DWORD	gAverageBattalionDetectionTime;
extern int		gBattalionDetects;
#define LOG_ERRORS
#endif DEBUG

#ifdef CAMPTOOL
// Renaming tool stuff
extern VU_ID_NUMBER RenameTable[65536];
extern int gRenameIds;
#endif

// KCK: Move to AIInput eventually
#define	MIN_RETREAT_DISTANCE_IF_BEHIND_LINES	20		// In km

// ============================================
// Prototypes
// ============================================

int RequestAirborneTransport (Unit u);
int RequestMarineTransport (Unit u);

// ==================================
// Some module globals - to save time
// ==================================

extern int haveWeaps;
extern int ourRange;
extern int theirDomain;

extern FILE
	*save_log,
	*load_log;

extern int
	start_save_stream,
	start_load_stream;

// ============================================
// Globals
// ============================================

int OrderPriority[GORD_LAST] = { 2, 10, 9, 8, 8, 7, 6, 4, 4, 4, 5, 3 };

// ============================================
// Ground Unit class
// ============================================

// constructors
GroundUnitClass::GroundUnitClass(int type) : UnitClass(type)
	{
	orders = 0;
	division = 0;
	pobj = FalconNullId;
	sobj = FalconNullId;
	aobj = FalconNullId;
	dirty_ground_unit = 0;
	}	

GroundUnitClass::GroundUnitClass(VU_BYTE **stream) : UnitClass(stream)
	{
	Objective		o;

	if (load_log)
	{
		fprintf (load_log, "%08x GroundUnitClass ", *stream - start_load_stream);
		fflush (load_log);
	}

	dirty_ground_unit = 0;
	memcpy(&orders, *stream, sizeof(uchar));				*stream += sizeof(uchar);
	memcpy(&division, *stream, sizeof(short));				*stream += sizeof(short);
	memcpy(&aobj, *stream, sizeof(VU_ID));					*stream += sizeof(VU_ID);
#ifdef DEBUG
	aobj.num_ &= 0xffff;
#endif
	o = FindObjective(aobj);
	// Find Secondary objective from actual
	if (o && !o->IsSecondary() && !o->IsPrimary())
		o = o->GetObjectiveParent();
	if (o)
		sobj = o->Id();
	else
		sobj = FalconNullId;
	// Find Primary objective from seconday
	if (o && !o->IsPrimary())
		o = o->GetObjectiveParent();
	if (o)
		pobj = o->Id();
	else
		pobj = FalconNullId;
	}

GroundUnitClass::~GroundUnitClass (void)
	{
	}

int GroundUnitClass::SaveSize (void)
	{
	return UnitClass::SaveSize()
		+ sizeof(uchar)
		+ sizeof(VU_ID)
		+ sizeof(short);
	}

int GroundUnitClass::Save (VU_BYTE **stream)
	{
	UnitClass::Save(stream);
	if (save_log)
	{
		fprintf (save_log, "%08x GroundUnitClass ", *stream - start_save_stream);
		fflush (save_log);
	}

	memcpy(*stream, &orders, sizeof(uchar));				*stream += sizeof(uchar);
	memcpy(*stream, &division, sizeof(short));				*stream += sizeof(short);
#ifdef CAMPTOOL
	if (gRenameIds)
		aobj.num_ = RenameTable[aobj.num_];
#endif
	memcpy(*stream, &aobj, sizeof(VU_ID));					*stream += sizeof(VU_ID);
	return GroundUnitClass::SaveSize();
	}

// event handlers
int GroundUnitClass::Handle(VuFullUpdateEvent *event)
	{
	// copy data from temp entity to current entity
	GroundUnitClass* tmp_ent = (GroundUnitClass*)(event->expandedData_);

	orders = tmp_ent->orders;
	aobj = tmp_ent->aobj;
	sobj = tmp_ent->sobj;
	pobj = tmp_ent->pobj;
	return (UnitClass::Handle(event));
	}

MoveType GroundUnitClass::GetMovementType (void)
	{
	UnitClassDataType*	uc;

/*	if (GetUnitTactic() == GTACTIC_MOVE_AIRBORNE)
		return LowAir;
	else if (GetUnitTactic() == GTACTIC_MOVE_MARINE)
		return Naval;
*/	
	uc = GetUnitClassData();
	
	ShiAssert (uc);

	return uc->MovementType;
	}

// This returns the best possible movement type between these two objectives
MoveType GroundUnitClass::GetObjMovementType (Objective o, int n)
	{
	float				cost,acost;
	UnitClassDataType*	uc;
	MoveType			amt;

	if (GetSType() != STYPE_UNIT_AIRMOBILE && GetSType() != STYPE_UNIT_MARINE)
		return GetMovementType();

	uc = GetUnitClassData();
	if (!uc)
		return Tracked;
	amt = uc->MovementType;
	cost = o->GetNeighborCost(n,amt);
	if (GetSType() == STYPE_UNIT_AIRMOBILE)
		amt = LowAir;
	if (GetSType() == STYPE_UNIT_MARINE)
		amt = Naval;
	acost = o->GetNeighborCost(n,amt);
	if (acost < cost)
		return amt;
	return uc->MovementType;
	}

// Detects other units. Returns 0 if nothing detected, 1 if detected, -1 on error (movement blocked)
int GroundUnitClass::DetectOnMove (void)
	{
	Objective			o;
	Team				who;
	float				d;
	int					combat=0,retval=0,spot=0,estr=0,capture,nomove=0;
	GridIndex			x,y;

	// Check for collision vs enemy units
	retval = ChooseTarget();
	if (retval < 0)
		return -1;

	// Check if our offensive has started yet
	if (retval && GetUnitCurrentRole() == GRO_ATTACK && TheCampaign.CurrentTime < TeamInfo[GetTeam()]->GetGroundAction()->actionTime)
		return -1;

	// Skip detection every third move until in enemy territory
	who = GetTeam();
	GetLocation(&x,&y);
	if (!(GetUnitMoved()%3) && FriendlyTerritory(x,y,who))
		return 0;

	// Now check vs enemy objectives for detection purposes
#ifdef VU_GRID_TREE_Y_MAJOR
	VuGridIterator	detit(ObjProxList,YPos(),XPos(),(BIG_SCALAR)GridToSim(MAX_GROUND_SEARCH));
#else
	VuGridIterator	detit(ObjProxList,XPos(),YPos(),(BIG_SCALAR)GridToSim(MAX_GROUND_SEARCH));
#endif
	o = (Objective) detit.GetFirst();
	while (o)
		{
		if (o->GetTeam() != who)
			{
			capture = 0;
			DetectVs(o,&d,&combat,&spot,&capture,&nomove,&estr);
			if (nomove)
				return -1;
			if (capture)
				{
				CaptureObjective(o, GetOwner(), this);
				if (!Parent())
					{
					Unit parent = GetUnitParent();
					ShiAssert(parent);
					if (parent) // JB 010710 CTD
						parent->SetOrdered(1);
					}
				}
			}
		o = (Objective) detit.GetNext();
		}
	return retval;
	}
// 2000-11-17 THIS FUNCTION WAS BROUGHT BACK TO ITS ORIGINAL STATE. THE CODE I ADDED WAS WORKING AS EXPECTED.

int GroundUnitClass::ChooseTarget (void)
	{
	FalconEntity		*artTarget,*react_against=NULL,*air_react_against=NULL;
	CampEntity			e;
	float				d,react_distance,air_react_distance;
	int					react,best_reaction=1,best_air_react=1,combat,retval=0,pass=0,spot=0,estr=0,capture=0,nomove=0;
	int					search_dist;
	Team				who;

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
//	CalculateSOJ(detit); 2002-02-19 REMOVED BY S.G. eFalcon 1.10 SOJ code removed

	e = (CampEntity)detit.GetFirst();
	while (e)
		{
		if (GetRoE(who,e->GetTeam(),ROE_GROUND_FIRE) == ROE_ALLOWED)
			{
			combat = 0;
			react = DetectVs(e,&d,&combat,&spot,&capture,&nomove,&estr);
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
				if (!IsEmitting() && class_data->RadarVehicle < 255 && GetNumVehicles(class_data->RadarVehicle)) {
					SetEmitting(1);

					// 2002-03-22 ADDED BY S.G. Since someone else was searching for us (that's why we were off), make sure the next radar step will be FEC_RADAR_AQUIRE
					if (GetRadarMode() < FEC_RADAR_AQUIRE && GetRadarMode() != FEC_RADAR_SEARCH_100) {
						SetStepSearchMode(FEC_RADAR_AQUIRE);
						SetRadarMode(FEC_RADAR_CHANGEMODE);
						SetAQUIREtimer(SimLibElapsedTime);
						SetSEARCHtimer(SimLibElapsedTime);
					}
				}
				SetEngaged(1);
				SetCombat(combat);
				}
			}
		e = (CampEntity)detit.GetNext();
		}
	SetOdds ((GetTotalVehicles() * 10) / (estr+10));

/*	This is outdated by the check vs deaggregated flights above
	// Check vs all players if we're an airdefense thingy, capible of at least some range
	if (GetAproxWeaponRange(LowAir))
		{
		FalconSessionEntity		*session;
		VuSessionsIterator		sit(FalconLocalGame);
		session = (FalconSessionEntity*) sit.GetFirst();
		while (session)
			{
			AircraftClass	*player = (AircraftClass*) session->GetPlayerEntity();
			if (player && GetRoE(who,session->GetTeam(),ROE_AIR_FIRE) == ROE_ALLOWED && player->IsAirplane())
				{
				combat = 0;
				react = DetectVs(player,&d,&combat,&spot,&capture,&nomove,&estr);
				if (react >= best_air_react && d < air_react_distance)
					{
					best_air_react = react;
					air_react_distance = d;
					air_react_against = player;
					SetEngaged(1);
					SetCombat(combat);
					}
				}
			session = (FalconSessionEntity*) sit.GetNext();
			}
		}
*/

	if (!Parent() && best_reaction > 1)
		EngageParent(this,react_against);

#ifdef DEBUG
	gBattalionDetects++;
	gAverageBattalionDetectionTime = (gAverageBattalionDetectionTime*(gBattalionDetects-1) + GetTickCount() - timec) / gBattalionDetects;
#endif

	if (air_react_against)
		{
		// KCK: Yet another reason to make this whole function Battalion ONLY
		ShiAssert( IsBattalion() );
		((Battalion)this)->SetAirTarget(air_react_against);
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

/* 2001-03-31 COMMENTED OUT BY S.G. TOO MANY CHANGES TO TRACK THEM ALL
int GroundUnitClass::DetectVs (AircraftClass *ac, float *d, int *combat, int *spot, int *capture, int *nomove, int *estr)
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

int GroundUnitClass::DetectVs (AircraftClass *ac, float *d, int *combat, int *spot, int *capture, int *nomove, int *estr)
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

/* 2001-03-31 COMMENTED OUT BY S.G. TOO MANY CHANGES TO TRACK THEM ALL
int GroundUnitClass::DetectVs (CampEntity e, float *d, int *combat, int *spot, int *capture, int *nomove, int *estr)
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
	if (det & FRIENDLY_IN_RANGE && ((Unit)e)->GetTargetID() == Id())
		*estr += ((Unit)e)->GetTotalVehicles();
	if (det & ENEMY_SAME_HEX)
		{
		if (e->IsBattalion())
			*nomove = 1; // retval = -1;
		if (e->IsObjective())
			*capture = 1;
		}

	return react;
	}
*/
int GroundUnitClass::DetectVs (CampEntity e, float *d, int *combat, int *spot, int *capture, int *nomove, int *estr)
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
			e->SetSpotted(GetTeam(),TheCampaign.CurrentTime, (!e->IsFlight() || CanItIdentify(this, e, *d, e->GetMovementType()))); // 2002-02-11 MODIFIED BY S.G. Say 'identified if it's not a flight or it has the hability to identify
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
	if (det & FRIENDLY_IN_RANGE && ((Unit)e)->GetTargetID() == Id())
		*estr += ((Unit)e)->GetTotalVehicles();
	if (det & ENEMY_SAME_HEX)
		{
		if (e->IsBattalion())
			*nomove = 1; // retval = -1;
		if (e->IsObjective())
			*capture = 1;
		}

	return react;
	}

#if 0 // eFalcon 1.10
int GroundUnitClass::DetectVs (CampEntity e, float *d, int *combat, int *spot, int *capture, int *nomove, int *estr)
	{
	int	react = 0;
	int det;

	det = Detected(this,e,d);
	if (!(det & REACTION_MASK))
		return 0;
	
	// JB SOJ Don't react unless we can detect
	if (det & ENEMY_DETECTED)
	{
		react = Reaction(e,det,*d);
		e->SetSpotted(GetTeam(),TheCampaign.CurrentTime);
		if (det & ENEMY_IN_RANGE && react)
			*combat = 1;
	}
	// JB SOJ

	if (det & FRIENDLY_DETECTED)
		{
		SetSpotted(e->GetTeam(),TheCampaign.CurrentTime);
		*spot = 1;
		}
	if (det & FRIENDLY_IN_RANGE && ((Unit)e)->GetTargetID() == Id())
		*estr += ((Unit)e)->GetTotalVehicles();
	if (det & ENEMY_SAME_HEX)
		{
		if (e->IsBattalion())
			*nomove = 1; // retval = -1;
		if (e->IsObjective())
			*capture = 1;
		}

	return react;
	}
#endif
		
// Core functions
int GroundUnitClass::CheckForSurrender (void)
	{
	GridIndex	x,y;

	// If we got here, we assume we're cut off.
	// What we do is sit tight and call for help.
	// However, if our morale breaks, we'll surrender
	if (Broken() || (IsBattalion() && !MoraleCheck(4,0)))
		{
		KillUnit();
		return 0;			// Zero is we surrendered
		}
	// Otherwise, hang out until we're rescued
	GetLocation(&x,&y);
	SetUnitDestination(x,y);
	SetTempDest(1);
	// KCK TODO: Request an air mission here with proper context
	return 1;
	}

int GroundUnitClass::GetUnitNormalRole (void)
	{
	UnitClassDataType*	uc;

	uc = GetUnitClassData();
	if (!uc)
		return 0;
	return uc->Role;
	}

int GroundUnitClass::GetUnitCurrentRole (void)
	{
	return GetGroundRole(orders);
	}

int GroundUnitClass::BuildMission(void)
	{
	return BuildGroundWP(this);
	}

// ============================
// Ground unit global functions
// ============================

Unit BestElement(Unit u, int at, int role)
	{
	Unit	e,be=NULL;
	int		s,bs = -1;
	UnitClassDataType	*uc;

	e = u->GetFirstUnitElement();
	while (e)
		{
//		s = e->GetCombatStrength(at,0) * (e->GetUnitClassData())->Scores[role];
		uc = e->GetUnitClassData();
		if (uc)
			{
			switch (role)
				{
				case GRO_AIRDEFENSE:
				case GRO_FIRESUPPORT:
				case GRO_ENGINEER:
					if (uc->Role != role)
						s = -1;
					else
						s = e->GetTotalVehicles() * uc->Scores[role];  // s = e->GetUnitGROScore(role)
					break;
				case GRO_RESERVE:
				case GRO_ATTACK:
				case GRO_DEFENSE:
				case GRO_RECON:
				default:
					if (uc->Role == GRO_AIRDEFENSE || uc->Role == GRO_FIRESUPPORT || uc->Role == GRO_ENGINEER)
						s = -1;
					else
						s = e->GetTotalVehicles() * uc->Scores[role];
					break;
				}
			}
		else
			s = 0;
		s -= FloatToInt32(0.1F*s)*e->GetUnitElement();		// Keep elements from switching friviously
		if (e->Retreating())
			s /= 2;
		if (!e->Assigned() && s > bs && (role == GRO_RESERVE || !e->Broken()))
			{
			be = e;
			bs = s;
			}
		e = u->GetNextUnitElement();
		}
	if (be)
		be->SetAssigned(1);
	return be;
	}

int FindNextBest (int d, int pos[])
   {
	if (d%2)		// if diagonal
		{
		if (!pos[(d+10)%16])
			return (d+10)%16;
		if (!pos[d+8])
			return d+8;
		}
	if (!pos[(d+1)%8])
		return (d+1)%8;
	if (!pos[(d+7)%8])
		return (d+7)%8;
	if (!pos[(d+2)%8])
		return (d+2)%8;
	if (!pos[(d+6)%8])
		return (d+6)%8;
   return Here;
   }

int GetThisWPAction (Unit u, Objective o, Objective n, int d, Team us, float *cost)
	{
	int	action;

	if (o->GetType() == TYPE_SEA || n->GetType() == TYPE_SEA)
		{
		action = WP_AMPHIBIOUS;
		*cost = o->GetNeighborCost(d,Naval);
		}
	else if (GetRoE(us,n->GetTeam(),ROE_GROUND_FIRE))
		action = WP_MOVEOPPOSED;
	else if (u->GetUnitNormalRole() == GRO_AIRBORNE)
		{
		action = WP_AIRBORNE;
		*cost = o->GetNeighborCost(d,LowAir);
		}
	else
		action = WP_MOVEUNOPPOSED;
	return action;
	}

int BuildGroundWP (Unit u)
	{
	PathClass		path,path2;
	Objective		n,o,t;
	GridIndex		ux,uy,ox,oy,nx,ny,tx,ty,bx=0,by=0;
	int				i,d,speed,us,action,didcas=0;
	float			cost,dist,bai_dist=MAX_BAI_DIST;
	CampaignTime	time,bai_time = Camp_GetCurrentTime();
	Brigade			brig;

	u->DisposeWayPoints();
	u->GetLocation(&ux,&uy);
	u->GetUnitDestination(&tx,&ty);
	dist = Distance (ux,uy,tx,ty);
	time = Camp_GetCurrentTime();
	brig = (Brigade)u->GetUnitParent();
	if (brig)
		speed = brig->GetUnitSpeed();
	else
		speed = u->GetUnitSpeed();
	if (dist < 2.0F)
		{
		// Just add one waypoint at our final destination
		u->AddUnitWP(tx,ty,0,speed,time,0,0);
		return 1;
		}
	if (u->GetUnitTactic() == GTACTIC_MOVE_AIRBORNE)
		{
		// KCK: We assume we can always find transports.
		RequestAirborneTransport(u);
		return 1;			// We've got a ticket to fly!
		}
	us = u->GetTeam();

	if (u->GetType() == TYPE_BATTALION && ((Battalion)u)->last_obj != FalconNullId)
		o = FindObjective(((Battalion)u)->last_obj);
	else
		o = FindNearestObjective (ux,uy,NULL);

	if (u->GetUnitTactic() == GTACTIC_MOVE_MARINE)
		{
		if (o && o->GetType() == TYPE_PORT)
			{
			// KCK: We assume we can always find transports.
			RequestMarineTransport(u);
			return 1;			// We've got a ticket to ride!
			}
		// KCK TODO: plan wp route to nearest port
		return 1;
		}

	t = u->GetUnitObjective();
	if (!t)
		t = FindNearestObjective (tx,ty,NULL);
	if (o==t || dist < 1.0)
		{
		// Just add one waypoint at our final destination
		u->AddUnitWP(tx,ty,0,speed,time,0,WP_MOVEUNOPPOSED);
		return 1;
		}
	if (!o || !t)
		return 0;
	if (u->GetUnitObjectivePath(&path,o,t) < 1)			// Avoid enemy objectives
		{
		int	ok = u->CheckForSurrender();
#ifdef LOG_ERRORS
		char	buffer[1280],name1[80],name2[80],name3[80],timestr[80];
		FILE	*fp;

		sprintf(buffer,"campaign\\save\\dump\\errors.log");
		fp = fopen(buffer,"a");
		if (fp)
			{
			GridIndex		ex,ey;
			u->GetName(name1,79,FALSE);
			o->GetName(name2,79,FALSE);
			t->GetName(name3,79,FALSE);
			o->GetLocation(&ox,&oy);
			t->GetLocation(&ex,&ey);
			GetTimeString(TheCampaign.CurrentTime,timestr);
			sprintf(buffer,"%s (%d) %d,%d couldn't find obj path from %s (%d) %d,%d to %s (%d) %d,%d @ %s\n",name1,u->GetCampID(),ux,uy,name2,o->GetCampID(),ox,oy,name3,t->GetCampID(),ex,ey,timestr);
			fprintf(fp,buffer);
			if (!ok)
				{
				sprintf(buffer,"%s (%d) %d,%d surrendered @ %s\n",name1,u->GetCampID(),ux,uy,timestr);
				fprintf(fp,buffer);
				}
			fclose(fp);
			}
#endif
		return -1;
		}

	// Find cost (in time) to get to the first WP, and determine which obj to use as start point
	o->GetLocation(&ox,&oy);
	d = path.GetDirection(0);
	n = o->GetNeighbor(d);
	n->GetLocation(&nx,&ny);
	if (DistSqu(ux,uy,nx,ny) < DistSqu(ox,oy,nx,ny))
		{
		o = n;
		ox = nx;
		oy = ny;
		i = 1;	// First step in path
		}
	else
		{
		i = 0;	// Zeroth step in path
		}
	if (u->GetUnitGridPath(&path2,ux,uy,ox,oy) > 0)
		{
		if (u->IsBattalion())
			((Battalion)u)->path.CopyPath(&path2);		// Might as well use the path.
		cost = path2.GetCost();
		time += TimeToArrive(cost,(float)speed);
		}
	else
		{
		int ok = u->CheckForSurrender();
#ifdef LOG_ERRORS
		char	buffer[1280],name1[80],name2[80],timestr[80];
		FILE	*fp;

		sprintf(buffer,"campaign\\save\\dump\\errors.log");
		fp = fopen(buffer,"a");
		if (fp)
			{
			u->GetName(name1,79,FALSE);
			o->GetName(name2,79,FALSE);
			GetTimeString(TheCampaign.CurrentTime,timestr);
			sprintf(buffer,"%s (%d) couldn't move to %s (%d) %d,%d from %d,%d @ %s\n",name1,u->GetCampID(),name2,o->GetCampID(),ox,oy,ux,uy,timestr);
			fprintf(fp,buffer);
			fclose(fp);
			if (!ok)
				{
				sprintf(buffer,"%s (%d) %d,%d surrendered @ %s\n",name1,u->GetCampID(),ux,uy,timestr);
				fprintf(fp,buffer);
				}
			}
#endif
		return -1;
		}

	// Add our first WP
	action = GetThisWPAction (u,o,o,0,us,&cost);
	u->AddUnitWP(ox,oy,0,speed,time,0,action);

	// Now set the rest of the path
	for (;i<path.GetLength(); i++)
		{
		d = path.GetDirection(i);
		n = o->GetNeighbor(d);
		n->GetLocation(&ox,&oy);
		cost = o->GetNeighborCost(d,u->GetObjMovementType(o,d));
		action = GetThisWPAction (u,o,n,d,us,&cost);
		time += TimeToArrive(cost,(float)speed);
		if (!didcas && (time - Camp_GetCurrentTime())/CampaignMinutes < MissionData[AMIS_BAI].max_time)
			{
			dist = DistanceToFront(ox,oy);
			if (dist > MAX_BAI_DIST/5 && dist < bai_dist)
				{
				bai_time = time;
				bai_dist = dist;
				bx = ox;
				by = oy;
				}
			}

		// Check to see if we're aproaching the enemy
		if (n->IsFrontline() && GetRoE(us,n->GetTeam(),ROE_GROUND_CAPTURE) == ROE_ALLOWED)
			{
			// Set our time to our offensive time, if our offensive hasn't started yet
			if (time < TeamInfo[us]->GetGroundAction()->actionTime)
				time = TeamInfo[us]->GetGroundAction()->actionTime;
			if (!didcas)
				{
				Unit enemy = FindNearestEnemyUnit(ox, oy, 10);
				// Request CAS vs enemy unit nearest to our destination
				if (enemy)
				{
					RequestOCCAS(/*u*/ enemy, ox, oy, time);	// M.N. Doesn't make sense to make two CASRequests vs us....
					// Request CAS vs us as well
					RequestOCCAS(u, ox, oy, time);	// added {} so that only cas against us if enemy unit was found
				}
				didcas = 1;
				}
			}
		// KCK Hackish: limit these sort of plans to 60 minutes in the future. Hope we don't
		// Change them before then..
		// Also - limit to first unit element if moving in brigade column,
		// so we don't generate a request for every battalion in a brigade
		if (time - Camp_GetCurrentTime() < 60*CampaignMinutes && 
		   (!u->GetUnitElement() || u->GetUnitTactic() != GTACTIC_MOVE_BRIGADE_COLUMN))
			{
			if (n->GetType() == TYPE_BRIDGE && (n->GetTeam() == us || TeamInfo[n->GetTeam()]->GetInitiative() < 40))
				{
				// Send request for bridge interdiction
				MissionRequestClass	mis;
				mis.requesterID = u->Id();
				mis.tot = time;
				mis.vs = us;
				mis.tot_type = TYPE_LT;
				mis.tx = ox;
				mis.ty = oy;
				mis.targetID = n->Id();
				mis.mission = AMIS_INTSTRIKE;
				mis.roe_check = ROE_AIR_ATTACK;
				if (n->IsNearfront())
					mis.priority = 50;
				if (u->GetUnitOrders() == GORD_CAPTURE)
					mis.context = enemyUnitAdvanceBridge;
				else
					mis.context = enemyUnitMoveBridge;
				mis.RequestEnemyMission();
				}
			if (n->GetType() == TYPE_ROAD || n->GetType() == TYPE_INTERSECT)
				{
				// Send request for interdiction
				MissionRequestClass	mis;
				if (brig)
					mis.requesterID = brig->Id();
				else
					mis.requesterID = u->Id();
				mis.tot = time;
				mis.vs = us;
				mis.tot_type = TYPE_EQ;
				mis.tx = ox;
				mis.ty = oy;
//				if (brig)
//					mis.targetID = brig->Id();
//				else
					mis.targetID = u->Id();
				mis.mission = AMIS_INT;
				mis.roe_check = ROE_AIR_ATTACK;
				if (u->GetUnitOrders() == GORD_CAPTURE)
					mis.context = enemyUnitAdvance;
				else
					mis.context = enemyUnitMove;
				mis.RequestEnemyMission();
				}
			}
		// If it's the last step in path, let's set it to our actual destination
		if (n == t)
			u->AddUnitWP(tx,ty,0,speed,time,0,action);
		else
			u->AddUnitWP(ox,oy,0,speed,time,0,action);
		o = n;
		}
	// Check for any BAI missions we can fly
	if (!didcas && bai_dist < MAX_BAI_DIST && dist > MAX_BAI_DIST/5)
		RequestBAI(u, bx, by, bai_time);
	return 1;
	}

CampaignHeading GetAverageHeading (Path path)
   {
   CampaignHeading h,first;
   int             total, cur, i=0, bonus=0, num=1;
   float           dir;

   if (path == NULL)
      return Here;
   first = (CampaignHeading) path->GetDirection(i);
   total = (int) first;
   for (i=1; i<4; i++)
      {
      cur = (int) path->GetDirection(i);
      if (cur >= 0)
         {
         if ((cur-first) > 4)
            cur -=8;
         if ((first-cur) > 4)
            cur +=8;
         if (cur > first)
            bonus++;
         if (cur < first)
            bonus--;
         total += cur;
         num++;
         }
      }
   dir = (float) total/num;
   if (bonus < 0)
      dir += 0.5F;
   if (dir < 0)
      dir += 8;
   if (dir > 7)
      dir -= 8;
   h = (CampaignHeading) dir;
   return h;
   }

int SOSecured(Objective o, Team who)
	{
	Objective	n,nn;
	int			i,j;

	if (o->GetTeam() != who)
		return 0;
	for (i=0; i<o->NumLinks(); i++)
		{
		n = o->GetNeighbor(i);
		if (n && n->GetObjectiveParentID() == o->Id())
			{
			if (n->GetTeam() != who)
				return 0;
			for (j=0; j<n->NumLinks(); j++)
				{
				nn = n->GetNeighbor(j);
				if (nn && nn->GetObjectiveParentID() == o->Id())
					{
					if (nn->GetTeam() != who)
						return 0;
					}
				}
			}
		}
	return 1;
	}

int CalculateOpposingStrength(Unit u, F4PFList list)
	{
	Unit			e;
	int			us,them,str=0;
	VuListIterator	myit(list);

	us = u->GetTeam();
	e = GetFirstUnit(&myit);
	while (e)
		{
		them = e->GetTeam();
		if (GetRoE(us,them,ROE_GROUND_FIRE) && e->GetDomain() == DOMAIN_LAND && e->Combat())
			{
			if (e->GetTargetID() == u->Id())
				str += e->GetCombatStrength(Foot,0);
			}
		e = GetNextUnit(&myit);
		}
	return str;
	}

// This checks if a unit is ready- 
// A unit is ready if it has no waypoints (at destination) or if it's detached.
int CheckReady (Unit u)
	{
	WayPoint	w;

	if (!u->Detached())
		{
		w = u->GetCurrentUnitWP();
		if (w)
			return 0;
		}
	return 1;
	}

/*
// Checks to see if someone is in our way, and tries to go around if so.
CampaignHeading GetAlternateHeading (Unit u, GridIndex x, GridIndex y, GridIndex nx, GridIndex ny, CampaignHeading h)
	{
	int					i;
	CampaignHeading		ah,bh;
	GridIndex			X,Y,tx,ty;
	Unit				e;
	float				d,bd;
	costtype			cost;

	if (h == Here)
		return h;
	ah = h;
	X = x + dx[h];
	Y = y + dy[h];
	// Check if it's our destination - if so, no use going around
	u->GetUnitDestination(&tx,&ty);
	if (X == tx && Y == ty)
		{
		u->SetDetached(1);						// Detach until we get to our destination
		u->SetUnitDestination(x,y);
//		(u->GetUnitParent())->SetOrdered(1);
		return Here;
		}
	// Is this place ok?
	e = FindUnitByXY(AllRealList,X,Y,DOMAIN_LAND);
	if (!e)
		return h;
//  Got rid of this -\v because two moving units would never pass each other.
//	else if (e->Moving() && !e->Engaged())
//		return Here;		// It's moving, just wait your turn..
	else if (e->GetTeam() != u->GetTeam())
		bh = Here;			// Never move through enemy units
	else
		bh = h;				// We'll go ahead and move if it's a friendly and no way around
	// Check left and right, take whatever's closest if both ok.
	bd = 999.0F;
	for (i=1; i<7; i++)
		{
		ah = (CampaignHeading)((h + HDelta[i] + 8)%8);
		X = x + dx[ah];
		Y = y + dy[ah];
		e = FindUnitByXY(AllRealList,X,Y,DOMAIN_LAND);
		d = Distance(X,Y,nx,ny);
		cost = u->GetUnitMovementCost(X,Y,ah);
		d += cost/2.0F;
		if (!e &&  cost < MAX_COST && d < bd)
			{
			bh = ah;
			bd = d;
			}
		}
	if (bh != h)
		u->ClearUnitPath();
	return bh;
	}
*/

int GetActionFromOrders(int orders)
	{
	switch (orders)
		{
		case GORD_CAPTURE:
			return WP_DEFEND;
			break;
		case GORD_DEFEND:
			return WP_DEFEND;
			break;
		case GORD_SUPPORT:
			return WP_FIRESUPPORT;
			break;
		case GORD_REPAIR:
			return WP_REPAIR;
			break;
		case GORD_AIRDEFENSE:
			return WP_AIRDEFENSE;
			break;
		default:
			return WP_RESERVE;
			break;
		}
	}

// We want to provide support for a particular unit -
// So we do what is necessary to get in range of it's objective
int PositionToSupportUnit (Battalion e)
	{
	int			d,h,i=0,bp=-1;
	int			range,ret;
	GridIndex	mex,mey,ex,ey,ox,oy;
	MoveType	mt;
	PathClass	path;
	Brigade		brigade;
	Unit		me;
	Objective	bo=NULL;

	ShiAssert (e->GetUnitOrders() == GORD_SUPPORT);

	// We need a minimal usefull range
	mt = NoMove;
	range = e->GetAproxWeaponRange(mt);
	if (range < 5)
		return 0;

	// Brigade elements only
	brigade = (Brigade) e->GetUnitParent();
	if (!brigade)
		return 0;
	me = brigade->GetFirstUnitElement();
	while (me)
		{
		if (me->GetUnitOrders() != GORD_SUPPORT && OrderPriority[me->GetUnitOrders()] > bp)
			{
			bp = OrderPriority[me->GetUnitOrders()];
			bo = me->GetUnitObjective();
			}
		me = brigade->GetNextUnitElement();
		}
	if (!bo)
		return 0;

	// Check tolerance
	e->GetLocation(&ex,&ey);
	bo->GetLocation(&mex,&mey);
	d = FloatToInt32(Distance(ex,ey,mex,mey));
	if (d > range-2)
		{
		// Get closer - Find a path from here to the objective
		ret = e->GetUnitGridPath(&path,ex,ey,mex,mey);
		}
	else if (d < 2)
		{
		// Back off - Find a path from the objective to a friendly secondline objective
		Objective	o;
		o = FindRetreatPath(e,3,0);
		if (!o)
			return e->CheckForSurrender();
		o->GetLocation(&ox,&oy);
		ret = e->GetUnitGridPath(&path,mex,mey,ox,oy);
		ex = mex;
		ey = mey;
		}
	else
		{
		// We're fine where we are
		e->SetUnitDestination(ex,ey);
		e->SetTempDest(1);
		return 1;
		}

	// Couldn't find a way there
	if (ret < 1)
		return 0;

	// Now determine our new location
	while (i < path.GetLength() && (d < 2 || d > range-2))
		{
		h = path.GetDirection(i);
		ex += dx[h];
		ey += dy[h];
		d = FloatToInt32(Distance(ex,ey,mex,mey));
		i++;
		}

	// Copy in new data
	e->SetUnitDestination(ex,ey);
	e->SetTempDest(1);
	return 1;
	}

int ScorePosition (Unit battalion, int role, int role_score, Objective o, GridIndex x, GridIndex y, int owned_by_us)
	{
	int			score,dist_from_front;
	GridIndex	ox,oy;
	
	o->GetLocation(&ox,&oy);
	// Check if this is a busted bridge
	if (role != GRO_ENGINEER && o->GetType() == TYPE_BRIDGE && !o->GetObjectiveStatus())
		return -32000;

	switch (role)
		{
		case GRO_ATTACK:
			// Find a good offensive position
			score = role_score/3 - FloatToInt32(DistanceToFront(ox,oy)) - FloatToInt32(Distance(ox,oy,x,y)) - o->GetObjectiveScore()*10;		
			if (owned_by_us)
				score = -32000;							// Don't assign
			else
				score += 100;
			break;
		case GRO_DEFENSE:
			// Find a good defensive position
			score = role_score/3 - FloatToInt32(DistanceToFront(ox,oy)) - FloatToInt32(Distance(ox,oy,x,y)) - o->GetObjectiveScore()*4;
			if (!owned_by_us)
				score = -32000;							// Don't assign
			else if (o->Abandoned())
				score -= 800;							// We're pulling out of here.
			break;
		case GRO_RESERVE:
			// Assign to an objective out of the way
			dist_from_front = FloatToInt32(DistanceToFront(ox,oy));
			score = dist_from_front - FloatToInt32(Distance(ox,oy,x,y));
			if (!owned_by_us)
				score = -32000;							// Don't assign
			else if (!dist_from_front)
				score -= 200;							// We want at least a second-line objective.
			else if (battalion->Broken())
				score += dist_from_front*5;
			// Try to prevent ping-pinging reserve units
			if (score > -32000 && o->Id() == battalion->GetUnitObjectiveID())
				score += 25;
			break;
		case GRO_AIRDEFENSE:
			// Assign to defend the primary objective, if possible
			score = role_score/3 + FloatToInt32(DistanceToFront(ox,oy)) - FloatToInt32(Distance(ox,oy,x,y)) - o->GetObjectiveScore()*50;
			if (!owned_by_us)
				score -= 500;
			if (o->Abandoned())
				score -= 500;
			if (o->IsSecondary() || o->SamSite())		// KCK: Sam site? It'll double up pretty often.
				score += 100;
			break;
		case GRO_FIRESUPPORT:
			// Assign to support a good frontline objective (enemy if available)
			score = role_score/3 - FloatToInt32(DistanceToFront(ox,oy)) - FloatToInt32(Distance(ox,oy,x,y)) - o->GetObjectiveScore()*10;
			if (!owned_by_us)
				score += 500;
			if (owned_by_us && o->Abandoned())
				score -= 500;
			if (o->IsSecondary())
				score += 100;
			break;
		case GRO_ENGINEER:
			// Assign to any bridges we may want to cross, otherwise nothing
			if (owned_by_us && o->GetType() == TYPE_BRIDGE && o->GetObjectiveStatus() < 50)
				score = 1000;							// Bonus for a damaged bridge
			else
				score = -32000;
			break;
		default:
//			MonoPrint ("ScorePosition() Error: We should never get here!\n");
			score = -32000;
			break;
		}

	return score;
	}

Objective FindBestPosition(Unit battalion, Brigade brigade, int role, F4PFList nearlist)
	{
	int				owned_by_us,score,bests=-32000,our_team,role_score;
	Objective		o,no,p,besto=NULL;
	GridIndex		x,y;
	VuListIterator	myit(nearlist);
	Battalion		other_battalion;

	// Collect the data
	role_score = battalion->GetUnitRoleScore(role, CALC_MAX, USE_VEH_COUNT);
	battalion->GetLocation(&x,&y);
	our_team = battalion->GetTeam();

	// Traverse objective options
	no = GetFirstObjective(&myit);
	while (no)
		{
		o = no;
		no = GetNextObjective(&myit);

		// Determine owner
		if (o->GetTeam() == our_team)
			owned_by_us = 1;
		else
			owned_by_us = 0;

		// Check for invalid offensive objectives
		if (!owned_by_us)
			{
			if (brigade->GetUnitCurrentRole() != GRO_ATTACK || !o->IsFrontline() || GetRoE(our_team,o->GetTeam(),ROE_GROUND_CAPTURE) != ROE_ALLOWED)
				{
				nearlist->Remove(o);
				continue;
				}
			p = o->GetObjectiveParent();
			if (!o->IsSecondary() && (!p || (p->Id() != brigade->GetUnitObjectiveID() && p->GetTeam() != our_team)))
				{
				nearlist->Remove(o);
				continue;
				}
			}

		// Score this position
		score = ScorePosition (battalion, role, role_score, o, x, y, owned_by_us);
		if (score <= bests)
			continue;

		// Check if it's already assigned to another brigade member
		// If so, we'll replace them if our score is higher && reassign them
		// Fire support is allowed to double up on objectives with other non-fire support units
		other_battalion = (Battalion) brigade->GetFirstUnitElement();
		while (other_battalion)
			{
			if (other_battalion != battalion && other_battalion->Assigned() && other_battalion->GetUnitObjectiveID() == o->Id())
				{
				// This guy's already assigned - check his score
				int			other_role,other_role_score,other_score;
				GridIndex	oex,oey;
				other_role = other_battalion->GetUnitCurrentRole();
				other_role_score = other_battalion->GetUnitRoleScore(other_role, CALC_MAX, USE_VEH_COUNT);
				other_battalion->GetLocation(&oex,&oey);
				if ((role != GRO_FIRESUPPORT && other_role != GRO_FIRESUPPORT) || role == other_role)
					{
					other_score = ScorePosition (other_battalion, other_role, other_role_score, o, oex, oey, owned_by_us);
					if (other_score >= score)
						score = -32000;
					else
						other_battalion->SetAssigned(0);
					}
				}
			other_battalion = (Battalion) brigade->GetNextUnitElement();
			}
		if (score > bests)
			{
			bests = score;
			besto = o;
			}
		}

	return besto;
	}

 void ClassifyUnitElements (Unit u, int *recon, int *combat, int *reserve, int *support)
	{
	Unit		e;
	int		pos;
	int		rc,cb,re,su;

	rc = cb = re = su = 0;
	e = u->GetFirstUnitElement();
	while (e)
		{
		pos = e->GetUnitPosition();
		if (pos <= GPOS_RECON3)
			rc++;
		else if (pos >= GPOS_COMBAT1 && pos <= GPOS_COMBAT3)
			cb++;
		else if (pos >= GPOS_RESERVE1 && pos <= GPOS_RESERVE3)
			re++;
		else if (pos >= GPOS_SUPPORT1 && pos <= GPOS_SUPPORT3)
			su++;
		e = u->GetNextUnitElement();
		}
	*recon = rc;
	*combat = cb;
	*reserve = re;
	*support = su;
	}

// Returns standard orders for support elements.
// Engineers arn't support elements- They should be their own parent, and ordered by request
int GetPositionOrders (Unit e)
	{
	if (e->GetUnitPosition() >= GPOS_SUPPORT1)
		{
		switch(e->GetUnitNormalRole())
			{
			case GRO_FIRESUPPORT:
				return GORD_SUPPORT;
			case GRO_AIRDEFENSE:
				return GORD_AIRDEFENSE;
			default:
				return GORD_RESERVE;
			}
		}
	return GORD_RESERVE;
	}

// Ask for Artillery support
// KCK NOTE: Two theories here - 1) Send GTM message, have it deal with it.
//							or	 2) Find it ourselves and send the message.	(I picked this one)
Unit RequestArtillerySupport (Unit req, Unit target)
	{
	VuGridIterator		*myit;
	Unit				u,art=NULL;
	GridIndex			tx,ty,x,y;
	int					score,best=9999,team = req->GetTeam(),d;
	MoveType			mt;

	if (!target)
		return NULL;
	target->GetLocation(&tx,&ty);
#ifdef VU_GRID_TREE_Y_MAJOR
	myit = new VuGridIterator(RealUnitProxList,target->YPos(),target->XPos(),(BIG_SCALAR)GridToSim((short)(MAX_GROUND_SEARCH)));
#else
	myit = new VuGridIterator(RealUnitProxList,target->XPos(),target->YPos(),(BIG_SCALAR)GridToSim((short)(MAX_GROUND_SEARCH)));
#endif
	mt = target->GetMovementType();

	// Try to find an artillery unit
	u = (Unit) myit->GetFirst();
	while (u)
		{
//		if (u->GetSType() == STYPE_UNIT_TOWED_ARTILLERY || u->GetSType() == STYPE_UNIT_SP_ARTILLERY)
		if (u->GetUnitNormalRole() == GRO_FIRESUPPORT && u->GetTeam() == team)
			{
			u->GetLocation(&x,&y);
			d = FloatToInt32(Distance(x,y,tx,ty));
			if (u->GetAproxHitChance(mt,d))
				{
				score = d;
				if (u->GetUnitDivision() == req->GetUnitDivision())
					score -= 20;		// Bonus for same division
				if (u->GetUnitParentID() == req->GetUnitParentID())
					score -= 20;		// Bonus for same brigade
				if (u->Engaged())
					score += 50;		// Penalty for someone already shooting
				if (score < best)
					{
					best = score;
					art = u;
					}
				}
			}
		u = (Unit)myit->GetNext();
		}
	if (art)
		{
		// We need to actually send this unit a message
		art->SendUnitMessage(target->Id(), FalconUnitMessage::unitSupport, 0, 0, 0);
		}
	delete myit;
	return art;
	}

// Ask for Immediate CAS
int RequestCAS (int team, Unit target)
	{
	VuGridIterator		*myit;
	Unit				u;
	GridIndex			tx,ty;

	if (!target)
		return NULL;
	target->GetLocation(&tx,&ty);
#ifdef VU_GRID_TREE_Y_MAJOR
	myit = new VuGridIterator(RealUnitProxList,target->YPos(),target->XPos(),(BIG_SCALAR)GridToSim((short)(MAX_AIR_SEARCH)));
#else
	myit = new VuGridIterator(RealUnitProxList,target->XPos(),target->YPos(),(BIG_SCALAR)GridToSim((short)(MAX_AIR_SEARCH)));
#endif

	// Try to find an available CAS flight
	u = (Unit) myit->GetFirst();
	while (u)
		{
		if (u->GetDomain() == DOMAIN_AIR && u->GetTeam() == team && u->GetUnitCurrentRole() == ARO_GA && u->GetUnitPriority() == 0)
			{
			// This is a reasonable enough flight to meet a CAS request, so request the support and return 1.
			// The ATM will sort out the details of who actually gets it.
			MissionRequestClass		mis;
			int		bonus=0;

			mis.tot = Camp_GetCurrentTime();
			mis.vs = target->GetTeam();
			mis.who = team;
			mis.tot_type = TYPE_NE;
			mis.flags = AMIS_IMMEDIATE;
			target->GetLocation(&mis.tx,&mis.ty);
			mis.targetID = target->Id();
			mis.mission = AMIS_CAS;
			if (target->Engaged())
				bonus += 20;
			if (target->Combat())
				bonus += 20;
			mis.priority = bonus;
			mis.roe_check = ROE_GROUND_FIRE;
			mis.RequestMission();
			return 1;						// We'll assume CAS will arrive
			}
		u = (Unit)myit->GetNext();
		}
	delete myit;
	return 0;
	}

// Request either CAS or artillery (or possibly both)
// Return 1 if we found something, 0 otherwise
int RequestSupport (Unit req, Unit target)
	{
	VuGridIterator		*myit;
	Unit				u,art=NULL;
	GridIndex			tx,ty,x,y;
	Int32				score,best=9999,d,foundcas=0,team = req->GetTeam();
	MoveType			mt;

	if (!target)
		return NULL;
	target->GetLocation(&tx,&ty);
#ifdef VU_GRID_TREE_Y_MAJOR
	myit = new VuGridIterator(RealUnitProxList,target->YPos(),target->XPos(),(BIG_SCALAR)GridToSim((short)(MAX_AIR_SEARCH)));
#else
	myit = new VuGridIterator(RealUnitProxList,target->XPos(),target->YPos(),(BIG_SCALAR)GridToSim((short)(MAX_AIR_SEARCH)));
#endif
	mt = target->GetMovementType();

	// Try to find an available CAS flight or an artillery unit
	u = (Unit) myit->GetFirst();
	while (u)
		{
		if (!foundcas && u->GetDomain() == DOMAIN_AIR && u->GetTeam() == team && u->GetUnitCurrentRole() == ARO_GA && u->GetUnitPriority() == 0)
			{
			// This is a reasonable enough flight to meet a CAS request, so request the support and stop looking
			// The ATM will sort out the details of who actually gets it.
			MissionRequestClass		mis;
			int		bonus=0;

			mis.tot = Camp_GetCurrentTime();
			mis.vs = target->GetTeam();
			mis.who = (uchar) team;
			mis.tot_type = TYPE_NE;
			mis.flags = AMIS_IMMEDIATE;
			target->GetLocation(&mis.tx,&mis.ty);
			mis.targetID = target->Id();
			mis.mission = AMIS_CAS;
			if (target->Engaged())
				bonus += 20;
			if (target->Combat())
				bonus += 20;
			mis.priority = bonus;
			mis.roe_check = ROE_GROUND_FIRE;
			mis.RequestMission();
			foundcas = 1;
			}
		else if (u->GetDomain() == DOMAIN_LAND && u->GetUnitNormalRole() == GRO_FIRESUPPORT && u->GetTeam() == team)
			{
			u->GetLocation(&x,&y);
			d = FloatToInt32(Distance(x,y,tx,ty));
			if (u->GetAproxHitChance(mt,d))
				{
				score = d;
				if (u->GetUnitDivision() == req->GetUnitDivision())
					score -= 20;		// Bonus for same division
				if (u->GetUnitParentID() == req->GetUnitParentID())
					score -= 20;		// Bonus for same brigade
				if (u->Engaged())
					score += 50;		// Penalty for someone already shooting
				if (score < best)
					{
					best = score;
					art = u;
					}
				}
			}
		u = (Unit)myit->GetNext();
		}
	if (art)
		{
		// We need to actually send this unit a message
		art->SendUnitMessage(target->Id(), FalconUnitMessage::unitSupport, 0, 0, 0);
		}
	delete myit;
	if (art || foundcas)
		return 1;
	return 0;
	}

// Send request for an enemy on call CAS mission in the vicinity
void RequestOCCAS (Unit u, GridIndex x, GridIndex y, CampaignTime time)
	{
	MissionRequestClass	mis;
	int					timeleft;
	Brigade				brig = (Brigade) u->GetUnitParent();

	timeleft = (int) ((time - Camp_GetCurrentTime())/CampaignMinutes);
	if (timeleft < MissionData[AMIS_ONCALLCAS].min_time || timeleft > MissionData[AMIS_ONCALLCAS].max_time)
		return;		// Not in required time parameters
	mis.tot = time;
	mis.vs = u->GetTeam();
	mis.tot_type = TYPE_GE;
	mis.tx = x;
	mis.ty = y;
	if (brig)
		{
		mis.requesterID = brig->Id();
//		mis.targetID = brig->Id();
		}
	else
		{
		mis.requesterID = u->Id();
//		mis.targetID = u->Id();
		}
	mis.targetID = u->Id();
	mis.mission = AMIS_ONCALLCAS;
	if (u->GetUnitCurrentRole() == GRO_ATTACK)
		mis.context = enemyUnitAttacking;
	else
		mis.context = enemyUnitDefending;
	mis.roe_check = ROE_GROUND_FIRE;
	mis.RequestEnemyMission();
	}

// Send request for an enemy BAI mission in the vicinity
void RequestBAI (Unit u, GridIndex x, GridIndex y, CampaignTime time)
	{
	MissionRequestClass	mis;
	int					timeleft;
	Brigade				brig = (Brigade) u->GetUnitParent();

	timeleft = (int) ((time - Camp_GetCurrentTime())/CampaignMinutes);
	if (timeleft < MissionData[AMIS_BAI].min_time || timeleft > MissionData[AMIS_BAI].max_time)
		return;		// Not in required time parameters
	mis.tot = time;
	mis.vs = u->GetTeam();
	mis.tot_type = TYPE_GE;
	mis.tx = x;
	mis.ty = y;
	if (brig)
		{
		mis.requesterID = brig->Id();
//		mis.targetID = brig->Id();
		}
	else
		{
		mis.requesterID = u->Id();
//		mis.targetID = u->Id();
		}
	mis.targetID = u->Id();
	mis.mission = AMIS_BAI;
	if (u->GetUnitCurrentRole() == GRO_ATTACK)
		mis.context = enemyUnitAdvance;
	else
		mis.context = enemyUnitMove;
	mis.roe_check = ROE_GROUND_FIRE;
	mis.RequestEnemyMission();
	}

int RequestAirborneTransport (Unit u)
	{
	MissionRequestClass	mis;

	// Check if we've already made a request
	if (u->GetCargoId() != FalconNullId)
		return 1;

	// Mark unit as waiting for a transport reply. If we get a replay before we time out,
	// we've got a ticket.
	u->SetCargoId(u->Id());

	// Make the request
	mis.who = u->GetTeam();
	mis.vs = 0;
	mis.flags = REQF_NEEDRESPONSE | REQF_ONETRY;
	mis.tot = TheCampaign.CurrentTime + 10*CampaignMinutes;
	mis.tot_type = TYPE_NE;
	u->GetUnitDestination(&mis.tx,&mis.ty);
	mis.requesterID = u->Id();
	mis.mission = AMIS_AIRCAV;
	mis.context = friendlyUnitAirborneMovement;
	mis.aircraft = u->GetTotalVehicles();
	mis.RequestMission();
	return 1;
	}

int RequestMarineTransport (Unit u)
	{
	// KCK TODO!
	return 1;
	}

extern float ReliefCost[RELIEF_TYPES];
extern float CoverValues[COVER_TYPES];

uchar Offsets[8][6][3] = { { {0,8,8}, {0,7,8}, {0,0,8}, {0,1,8}, {8,8,8}, {8,8,8} },		// Heading 0
						   { {1,8,8}, {1,0,8}, {1,2,8}, {1,1,8}, {1,0,0}, {1,2,2} },		// Heading 1
						   { {2,8,8}, {2,1,8}, {2,2,8}, {2,3,8}, {8,8,8}, {8,8,8} },		// Heading 2
	                       { {3,8,8}, {3,2,8}, {3,4,8}, {3,3,8}, {3,2,2}, {3,4,4} },		// Heading 3
						   { {4,8,8}, {4,3,8}, {4,4,8}, {4,5,8}, {8,8,8}, {8,8,8} },		// Heading 4
						   { {5,8,8}, {5,4,8}, {5,6,8}, {5,5,8}, {5,4,4}, {5,6,6} },		// Heading 5
						   { {6,8,8}, {6,5,8}, {6,6,8}, {6,7,8}, {8,8,8}, {8,8,8} },		// Heading 6
						   { {7,8,8}, {7,8,8}, {7,0,8}, {7,7,8}, {7,6,6}, {7,0,0} } };		// Heading 7


float CoverValue (GridIndex x, GridIndex y, int roadok)
	{
	float	cov;
	if (!roadok && GetRoad(x,y))
		cov = 0.5F;
	else
		cov = CoverValues[GetCover(x,y)];
	return cov * ReliefCost[GetRelief(x,y)];
	}

// This function attempts to find the best place to station a ground unit direction h from x,y.
void FindBestCover (GridIndex x, GridIndex y, CampaignHeading h, GridIndex *cx, GridIndex *cy, int roadok)
	{
	GridIndex	tx,ty;
	float		cov,bcov=0.0F;
	int			i,j;

	// default location
	*cx = x;
	*cy = y;
	if (h < 0 || h > 7)
		return;

	// Traverse possible locations, adding up cover values (1/2 value for surrounding terrain)
	for (i=0; i<4+2*(h%2); i++)
		{
		tx = x + dx[Offsets[h][i][0]] + dx[Offsets[h][i][1]] + dx[Offsets[h][i][2]];
		ty = y + dy[Offsets[h][i][0]] + dy[Offsets[h][i][1]] + dy[Offsets[h][i][2]];
		if (GetMovementCost(tx,ty,Foot,0,Here) > MAX_COST)
			continue;
		cov = CoverValue (tx, ty, roadok);
		for (j=0; j<8; j++)
			cov += CoverValue (tx+dx[j], ty+dy[j], roadok) / 2.0F;
		if (cov > bcov)
			{
			bcov = cov;
			*cx = tx;
			*cy = ty;
			}
		}
	}

// Try and find the nearest objective of the specified depth (number of moves)
// away from the front line. We need to actually follow links to make sure 
// it's possible to move here.
Objective FindRetreatPath(Unit u, int depth, int flags)
	{
	ListClass			looklist;
	ListNode			lp;	
	Objective			s,o;
	GridIndex			x,y,sx,sy;
	int					n,dist,odist;
	uchar				team;

	// Reset search array
	memset(CampSearch,0,sizeof(uchar)*MAX_CAMP_ENTITIES);
	u->GetLocation(&x,&y);
	team = u->GetTeam();
	sx = x;
	sy = y;
	s = FindNearestFriendlyObjective(team, &sx, &sy, 0);
	if (s)
		{
		dist = FloatToInt32(Distance(x,y,sx,sy));
		if (dist > MIN_RETREAT_DISTANCE_IF_BEHIND_LINES)
			{
			// Shucks, we're probably gonna surrender.
			return NULL;
			}
		looklist.InsertNewElement(dist,s,LADT_SORTED_LIST);

		while (lp = looklist.GetFirstElement())
			{
			CampSearch[s->GetCampID()] = 1;
			dist = lp->GetKey();
			s = (Objective) lp->GetUserData();

			// Add all our neighbors to the list
			for (n=0; n<s->NumLinks(); n++)
				{
				o = s->GetNeighbor(n);
				if (o && !CampSearch[o->GetCampID()] && team == o->GetTeam())
					{
					o->GetLocation(&x,&y);
					odist = dist + FloatToInt32(Distance(x,y,sx,sy)); 
					looklist.InsertNewElement(odist,o,LADT_SORTED_LIST);
					if (flags & FIND_SECONDARYONLY && !o->IsSecondary())
						continue;
					if (depth > 2 && (o->IsFrontline() || o->IsSecondline() || o->IsThirdline()))
						continue;
					else if (depth == 2 && (o->IsFrontline() || o->IsSecondline()))
						continue;
					else if (o->IsFrontline())
						continue;
					// The list should purge itself here, when it gets deleted
					return o;
					}
				}
			looklist.Remove(lp);
			}
		}
	// Shucks, we're probably gonna surrender.
	return NULL;
	}

/* Old Version - This worked, but sometimes found some pretty bizarre
/* locations, as it essentially looked at low VU_IDs first rather than
/* based on distance
Objective FindRetreatPath(Unit u, int depth, int flags)
	{
	FalconPrivateList	looklist(&AllObjFilter);
	VuListIterator		myit(&looklist);
	Objective		s,o;
	int				n,team;
	GridIndex		x,y;

	// Reset search array
	memset(CampSearch,0,sizeof(uchar)*MAX_CAMP_ENTITIES);
	u->GetLocation(&x,&y);
	s = FindNearestObjective(x,y,NULL);
	if (!s)
		return NULL;
	looklist.ForcedInsert(s);
	team = u->GetTeam();

	// Keep branching out in objective links until I find a valid objective
	while (s = GetFirstObjective(&myit))
		{
		CampSearch[s->GetCampID()] = 1;
		for (n=0; n<s->NumLinks(); n++)
			{
			o = s->GetNeighbor(n);
			if (o && !CampSearch[o->GetCampID()] && team == o->GetTeam())
				{
				looklist.ForcedInsert(o);
				if (flags & FIND_SECONDARYONLY && !o->IsSecondary())
					continue;
				if (depth > 2 && (o->IsFrontline() || o->IsSecondline() || o->IsThirdline()))
					continue;
				else if (depth == 2 && (o->IsFrontline() || o->IsSecondline()))
					continue;
				else if (o->IsFrontline())
					continue;
				return o;
				}
			}
		if (s)
			looklist.Remove(s);
		}
	// Shucks, we're probably gonna surrender.
	return NULL;
	}
*/

int MinAdjustLevel(Unit u)
	{
	Unit		e;
	int			adjt=0,num=0;

	if (u->Real())
		return FloatToInt32(u->AdjustForSupply()*100.0F);

	e = u->GetFirstUnitElement();
	while (e)
		{
		adjt += FloatToInt32(e->AdjustForSupply()*100.0F);
		num++;
		e = u->GetNextUnitElement();
		}
	if (!num)
		return 0;
	return adjt/num;
	}

int FindUnitSupportRole(Unit u)
	{
	UnitClassDataType*	uc;

	uc = u->GetUnitClassData();
	if (!uc)
		return 0;
	if (uc->Role == GRO_FIRESUPPORT || uc->Role == GRO_AIRDEFENSE || uc->Role == GRO_ENGINEER)
		return uc->Role;
	return 0;
	}

int GetGroundRole (int orders)
	{
	int role = GRO_RESERVE;

	switch (orders)
		{
		case GORD_CAPTURE:
		case GORD_SECURE:
			role = GRO_ATTACK;
			break;
		case GORD_ASSAULT:
			role = GRO_ATTACK;
//			role = GRO_ASSAULT;			// We need GRO_ASSAULT to do this, but the role is attack
			break;
		case GORD_AIRBORNE:
			role = GRO_ATTACK;
//			role = GRO_AIRBORNE;		// We need GRO_AIRBORNE to do this, but the role is attack
			break;
		case GORD_COMMANDO:
			role = GRO_AIRDEFENSE;
//			role = GRO_AIRBORNE;		// We need GRO_AIRBORNE to do this, but the role is air defense
			break;
		case GORD_DEFEND:
			role = GRO_DEFENSE;
			break;
		case GORD_SUPPORT:
			role = GRO_FIRESUPPORT;
			break;
		case GORD_REPAIR:
			role = GRO_ENGINEER;
			break;
		case GORD_AIRDEFENSE:
			role = GRO_AIRDEFENSE;
			break;
		case GORD_RADAR:
		case GORD_RECON:
			role = GRO_RECON;
			break;
		case GORD_RESERVE:
		default:
			role = GRO_RESERVE;
			break;
		}
	return role;
	}

int GetGroundOrders (int role)
	{
	int orders = GRO_RESERVE;

	switch (role)
		{
		case GRO_ATTACK:
			return GORD_CAPTURE;
			break;
		case GRO_ASSAULT:
			return GORD_ASSAULT;
			break;
		case GRO_AIRBORNE:
			return GORD_AIRBORNE;
			break;
		case GRO_DEFENSE:
			return GORD_DEFEND;
			break;
		case GRO_AIRDEFENSE:
			return GORD_AIRDEFENSE;
			break;
		case GRO_FIRESUPPORT:
			return GORD_SUPPORT;
			break;
		case GRO_ENGINEER:
			return GORD_REPAIR;
			break;
		case GRO_RECON:
			return GORD_RADAR;
			break;
		case GRO_RESERVE:
		default:
			return GORD_RESERVE;
			break;
		}
	return 0;
	}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void GroundUnitClass::SetOrders (uchar o)
{
	orders = o;

	MakeGndUnitDirty (DIRTY_ORDERS, DDP[111].priority);
	//	MakeGndUnitDirty (DIRTY_ORDERS, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void GroundUnitClass::SetDivision (short d)
{
	division = d;

	MakeGndUnitDirty (DIRTY_DIVISION, DDP[112].priority);
	//	MakeGndUnitDirty (DIRTY_DIVISION, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void GroundUnitClass::SetAObj (VU_ID id)
{
	aobj = id;

	MakeGndUnitDirty (DIRTY_AOBJ, DDP[113].priority);
	//	MakeGndUnitDirty (DIRTY_AOBJ, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void GroundUnitClass::SetSObj (VU_ID id)
{
	sobj = id;

	MakeGndUnitDirty (DIRTY_SOBJ, DDP[114].priority);
	//	MakeGndUnitDirty (DIRTY_SOBJ, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void GroundUnitClass::SetPObj (VU_ID id)
{
	pobj = id;

	MakeGndUnitDirty (DIRTY_POBJ, DDP[115].priority);
	//	MakeGndUnitDirty (DIRTY_POBJ, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void GroundUnitClass::MakeGndUnitDirty (Dirty_Ground_Unit bits, Dirtyness score)
{
	if (!IsLocal())
		return;

	if (VuState() != VU_MEM_ACTIVE)
		return;

	if (!IsAggregate())
	{
		score = (Dirtyness) ((int) score * 10);
	}

	dirty_ground_unit |= bits;

	MakeDirty (DIRTY_GROUND_UNIT, score);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void GroundUnitClass::WriteDirty (uchar **stream)
{
	unsigned char
		*ptr;

	ptr = *stream;

	MonoPrint ("  GU %08x", dirty_ground_unit);//me123

	// Encode it up
	*(uchar*)ptr = (uchar) dirty_ground_unit;
	ptr += sizeof (uchar);

	if (dirty_ground_unit & DIRTY_ORDERS)
	{
		*(uchar*)ptr = orders;
		ptr += sizeof (uchar);
	}

	if (dirty_ground_unit & DIRTY_DIVISION)
	{
		*(short*)ptr = division;
		ptr += sizeof (short);
	}

	if (dirty_ground_unit & DIRTY_AOBJ)
	{
		*(VU_ID*)ptr = aobj;
		ptr += sizeof (VU_ID);
	}

	if (dirty_ground_unit & DIRTY_SOBJ)
	{
		*(VU_ID*)ptr = sobj;
		ptr += sizeof (VU_ID);
	}

	if (dirty_ground_unit & DIRTY_POBJ)
	{
		*(VU_ID*)ptr = pobj;
		ptr += sizeof (VU_ID);
	}

	dirty_ground_unit = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void GroundUnitClass::ReadDirty (uchar **stream)
{
	unsigned char
		*ptr,
		bits;
	ShiAssert(FALSE == F4IsBadReadPtr(stream, 8)); // JPO check
	// JB 010221 CTD
	if (!stream || F4IsBadReadPtr(stream, sizeof(unsigned char*)))
		return;

	ptr = *stream;

	// JB 010221 CTD
	if (!ptr || F4IsBadWritePtr(ptr, sizeof(unsigned char)))
		return;

	bits = *(uchar*)ptr;
	ptr += sizeof (uchar);

	//MonoPrint ("  GU %08x", bits);

	if (bits & DIRTY_ORDERS)
	{
		if (F4IsBadWritePtr(ptr, sizeof(uchar))) // JB 010221 CTD
			return; // JB 010221 CTD

		orders = *(uchar*)ptr;
		ptr += sizeof (uchar);
	}

	if (bits & DIRTY_DIVISION)
	{
		if (F4IsBadWritePtr(ptr, sizeof(short))) // JB 010221 CTD
			return; // JB 010221 CTD

		division = *(short*)ptr;
		ptr += sizeof (short);
	}

	if (bits & DIRTY_AOBJ)
	{
		if (F4IsBadWritePtr(ptr, sizeof(VU_ID))) // JB 010221 CTD
			return; // JB 010221 CTD

		aobj = *(VU_ID*)ptr;
		ptr += sizeof (VU_ID);
	}

	if (bits & DIRTY_SOBJ)
	{
		if (F4IsBadWritePtr(ptr, sizeof(VU_ID))) // JB 010221 CTD
			return; // JB 010221 CTD

		sobj = *(VU_ID*)ptr;
		ptr += sizeof (VU_ID);
	}

	if (bits & DIRTY_POBJ)
	{
		if (F4IsBadWritePtr(ptr, sizeof(VU_ID))) // JB 010221 CTD
			return; // JB 010221 CTD

		pobj = *(VU_ID*)ptr;
		ptr += sizeof (VU_ID);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
