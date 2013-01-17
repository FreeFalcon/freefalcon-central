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
#include "Manager.h"
#include "update.h"
#include "loadout.h"
#include "gndunit.h"
#include "team.h"
#include "Battalion.h"
#include "Debuggr.h"
#include "Gtmobj.h"
#include "MsgInc\GndTaskingMsg.h"
#include "classtbl.h"
#include "AIInput.h"
#include "PtData.h"
#include "Graphics\Include\TMap.h"
#include "Camp2Sim.h"
#include "FalcSess.h"
#include "radar.h"

#include "graphics\include\drawpnt.h"
#include "Graphics\Include\TMap.h"

#define ROBIN_GDEBUG

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

#ifdef DEBUG
extern int gDumping;
extern char	OrderStr[GORD_LAST][15];
extern int gCheckConstructFunction;

#include "CampStr.h"
#define LOG_ERRORS
#endif

#ifdef ROBIN_DEBUG
extern char	OrderStr[GORD_LAST][15];
extern uchar TrackingOn[MAX_CAMP_ENTITIES];
#endif

#ifdef DEBUG_TIMING
extern DWORD	gAverageBattalionDetectionTime,gAverageBattalionMoveTime;
extern int		gBattalionDetects,gBattalionMoves;
#endif

#ifdef CAMPTOOL
// Renaming tool stuff
extern VU_ID_NUMBER RenameTable[65536];
extern int gRenameIds;
#endif

extern int gCampDataVersion;

extern bool g_bOldSamActivity;

extern int GetArrivalSpeed (Unit u);
extern int FindUnitSupportRole(Unit u);
extern int PositionToSupportUnit (Battalion e);
extern void FindVehiclePosition(SimInitDataClass *initData);

// ============================================
// Prototypes
// ============================================

extern FILE
	*save_log,
	*load_log;

extern int
	start_save_stream,
	start_load_stream;

// ==================================
// Some module globals - to save time
// ==================================

extern int haveWeaps;
extern int ourRange;
extern int ourObjDist;
extern int ourObjOwner;
extern int ourFrontDist;
extern int theirDomain;

// ==================================
// Formation offsets
// ==================================

#ifdef USE_FLANKS
char LeftFlankOffset[3][9][2] =  { { {-1,-1}, {-1, 0}, {-1, 1}, { 0, 1}, { 1, 1}, { 1, 0}, { 1,-1}, { 0,-1}, { 0, 0} },
								   { {-1, 1}, { 0, 1}, { 1, 1}, { 1, 0}, { 1,-1}, { 0,-1}, {-1,-1}, {-1, 0}, { 0, 0} },
								   { {-1, 0}, {-1, 1}, { 0, 1}, { 1, 1}, { 1, 0}, { 1,-1}, { 0,-1}, {-1,-1}, { 0, 0} } };
char RightFlankOffset[3][9][2] = { { { 1,-1}, { 0,-1}, {-1,-1}, {-1, 0}, {-1, 1}, {-1, 0}, { 1, 1}, { 1, 0}, { 0, 0} },
								   { { 1,-1}, { 0,-1}, {-1,-1}, {-1, 0}, {-1, 1}, {-1, 0}, { 1, 1}, { 1, 0}, { 0, 0} },
								   { { 1, 0}, { 1,-1}, { 0,-1}, {-1,-1}, {-1, 0}, {-1, 1}, { 0, 1}, { 1, 1}, { 0, 0} } };
#endif

// =================================
// Smart heap pool stuff
// =================================

#ifdef USE_SH_POOLS
MEM_POOL	BattalionClass::pool;
#endif

// ============================================
// Battalion class Functions
// ============================================

// KCK: ALL BATTALION CONSTRUCTION SHOULD USE THIS FUNCTION!
BattalionClass* NewBattalion (int type, Unit parent)
	{
	BattalionClass	*new_battalion;
#ifdef DEBUG
	gCheckConstructFunction = 1;
#endif
	VuEnterCriticalSection();
	lastVolitileId = vuAssignmentId;
	vuAssignmentId = lastNonVolitileId;
	vuLowWrapNumber = FIRST_NON_VOLITILE_VU_ID_NUMBER;
	vuHighWrapNumber = LAST_NON_VOLITILE_VU_ID_NUMBER;
	new_battalion = new BattalionClass (type, parent);
	lastNonVolitileId = vuAssignmentId;
	vuAssignmentId = lastVolitileId;
	vuLowWrapNumber = FIRST_VOLITILE_VU_ID_NUMBER;
	vuHighWrapNumber = LAST_VOLITILE_VU_ID_NUMBER;
	VuExitCriticalSection();
#ifdef DEBUG
	gCheckConstructFunction = 0;
#endif
	return new_battalion;
	}

// constructors
BattalionClass::BattalionClass(int type, Unit parent) : GroundUnitClass(type)
	{
#ifdef DEBUG
	ShiAssert (gCheckConstructFunction);
#endif

#ifdef USE_FLANKS
	lfx = lfy = rfx = rfy = 0;				// KCK hack.. should be set to starting location (which is unknown here)
#endif
	supply = 100;
	last_move = 0;	
	last_combat = 0;
	SetSpottedTime (0);
	fatigue = 0;
	morale = 100;
//	element = 0;
	position = 0;
	fullstrength = 0;
	// Marco Edit - originally wasn't set
	SetRadarMode(FEC_RADAR_OFF);
	// End Marco Edit

	if(parent)
		SetReinforcement (parent->GetReinforcement());
	else
		SetReinforcement (0);
	// Inherit parent's inactive status;

	if (parent)
	{
		SetUnitFlags (parent->GetUnitFlags () & U_INACTIVE);
	}

	dirty_battalion = 0;
	last_obj = FalconNullId;
	heading = final_heading = North;
	deag_data = NULL;
	if (parent)
		{
		SetUnitParent(parent);
		parent->AddUnitChild(this);
		SetParent(0);
		}
	else
		{
		parent_id = FalconNullId;
		SetParent(1);
		}
	air_target = FalconNullId;
	search_mode = FEC_RADAR_OFF;
	step_search_mode = FEC_RADAR_OFF; // 2002-03-22 ADDED BY S.G. Our radar step search mode var needs to be set as well
	radar_mode = FEC_RADAR_OFF; // jpo init
	missiles_flying = 0;
	SEARCHtimer = 0;
	AQUIREtimer = 0;
	}

BattalionClass::BattalionClass(VU_BYTE **stream) : GroundUnitClass(stream)
	{
	if (load_log)
	{
		fprintf (load_log, "%08x BattalionClass ", *stream - start_load_stream);
		fflush (load_log);
	}

	memcpy(&last_move, *stream, sizeof(CampaignTime));		*stream += sizeof(CampaignTime); 
	memcpy(&last_combat, *stream, sizeof(CampaignTime));	*stream += sizeof(CampaignTime); 
	memcpy(&parent_id, *stream, sizeof(VU_ID));				*stream += sizeof(VU_ID);
	memcpy(&last_obj, *stream, sizeof(VU_ID));				*stream += sizeof(VU_ID);
#ifdef DEBUG
	parent_id.num_ &= 0x0000ffff;
	last_obj.num_ &= 0x0000ffff;
#endif
#ifdef USE_FLANKS
	memcpy(&lfx, *stream, sizeof(GridIndex));				*stream += sizeof(GridIndex);
	memcpy(&lfy, *stream, sizeof(GridIndex));				*stream += sizeof(GridIndex);
	memcpy(&rfx, *stream, sizeof(GridIndex));				*stream += sizeof(GridIndex);
	memcpy(&rfy, *stream, sizeof(GridIndex));				*stream += sizeof(GridIndex);
#endif
	memcpy(&supply, *stream, sizeof(Percentage));			*stream += sizeof(Percentage);
	memcpy(&fatigue, *stream, sizeof(Percentage));			*stream += sizeof(Percentage);
	memcpy(&morale, *stream, sizeof(Percentage));			*stream += sizeof(Percentage);
	memcpy(&heading, *stream, sizeof(uchar));				*stream += sizeof(uchar);
	memcpy(&final_heading, *stream, sizeof(uchar));			*stream += sizeof(uchar);
	if (gCampDataVersion < 15)
		{
		uchar	dummy;
		memcpy(&dummy, *stream, sizeof(uchar));				*stream += sizeof(uchar); 
		}
	memcpy(&position, *stream, sizeof(uchar));				*stream += sizeof(uchar);
	fullstrength = 0;

	dirty_battalion = 0;
#ifdef DEBUG
#ifdef USE_FLANKS
	// Flank sanity check
	GridIndex	x,y;
	CampBaseClass::GetLocation(&x,&y);	
	if (rfx - x > 5 || rfx - x < -5)		// 5 km for right flank
		rfx = x;
	if (rfy = y > 5 || rfy - y < -5)
		rfy = y;
	if (lfx - x > 2 || lfx - x < -2)		// Only 2 for left - since we don't use this for column
		lfx = x;
	if (lfy = y > 2 || lfy - y < -2)
		lfy = y;
#endif
#endif
	
	deag_data = NULL;
	// KCK Temporary
	if (last_move > Camp_GetCurrentTime())
		last_move = Camp_GetCurrentTime();
	if (GetLastCheck() > Camp_GetCurrentTime())
		SetLastCheck (Camp_GetCurrentTime());
	if (last_combat > Camp_GetCurrentTime())
		last_combat = Camp_GetCurrentTime();

	// KCK: Move this to somewhere load-only
	// We need to add this battalion's vehicles to its primary objective's assigned rating,
	// so we keep our scoring system valid after a load.
	POData		pd;
	Objective	o;
	o = UnitClass::GetUnitPrimaryObj();
	if (o)
		{
		if (pd = GetPOData(o))
			pd->ground_assigned[GetTeam()] += GetTotalVehicles();
		}
/*	o = UnitClass::GetUnitSecondaryObj();
	if (o)
		{
		if (sd = GetSOData(o))
			sd->assigned[GetTeam()] += GetTotalVehicles();
		}
*/

#ifdef DEBUG
	char	buffer[256];

	sprintf(buffer,"campaign\\save\\dump\\%d.BAT",GetCampID());
	unlink(buffer);
#endif
	air_target = FalconNullId;
	search_mode = FEC_RADAR_OFF;
	step_search_mode = FEC_RADAR_OFF; // 2002-03-22 ADDED BY S.G. Our radar step search mode var needs to be set as well
	radar_mode = FEC_RADAR_OFF; // JPO init
	missiles_flying = 0;
	SEARCHtimer = 0;
	AQUIREtimer = 0;
	}

BattalionClass::~BattalionClass (void)
	{
	if (IsAwake())
		Sleep();

	if (deag_data)
		{
		delete deag_data;
		deag_data = NULL;
		}
	}

int BattalionClass::SaveSize (void)
	{
	return GroundUnitClass::SaveSize() 
		+ sizeof(CampaignTime)
		+ sizeof(CampaignTime)
		+ sizeof(VU_ID)
		+ sizeof(VU_ID)
#ifdef USE_FLANKS
		+ sizeof(GridIndex)
		+ sizeof(GridIndex)
		+ sizeof(GridIndex)
		+ sizeof(GridIndex)
#endif
		+ sizeof(Percentage)
		+ sizeof(Percentage)
		+ sizeof(Percentage)
//		+ path->SaveSize()
		+ sizeof(uchar)
		+ sizeof(uchar)
		+ sizeof(uchar);
	}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

int BattalionClass::Save (VU_BYTE **stream)
	{
	GroundUnitClass::Save(stream);
	if (save_log)
	{
		fprintf (save_log, "%08x BattalionClass ", *stream - start_save_stream);
		fflush (save_log);
	}

	memcpy(*stream, &last_move, sizeof(CampaignTime));		*stream += sizeof(CampaignTime);
	memcpy(*stream, &last_combat, sizeof(CampaignTime));	*stream += sizeof(CampaignTime);
#ifdef CAMPTOOL
	if (gRenameIds)
		parent_id.num_ = RenameTable[parent_id.num_];
#endif
	memcpy(*stream, &parent_id, sizeof(VU_ID));				*stream += sizeof(VU_ID);
#ifdef CAMPTOOL
	if (gRenameIds)
		last_obj.num_ = RenameTable[last_obj.num_];
#endif
	memcpy(*stream, &last_obj, sizeof(VU_ID));				*stream += sizeof(VU_ID);
#ifdef USE_FLANKS
	memcpy(*stream, &lfx, sizeof(GridIndex));				*stream += sizeof(GridIndex);
	memcpy(*stream, &lfy, sizeof(GridIndex));				*stream += sizeof(GridIndex);
	memcpy(*stream, &rfx, sizeof(GridIndex));				*stream += sizeof(GridIndex);
	memcpy(*stream, &rfy, sizeof(GridIndex));				*stream += sizeof(GridIndex);
#endif
	memcpy(*stream, &supply, sizeof(Percentage));			*stream += sizeof(Percentage);
	memcpy(*stream, &fatigue, sizeof(Percentage));			*stream += sizeof(Percentage);
	memcpy(*stream, &morale, sizeof(Percentage));			*stream += sizeof(Percentage);
//	path->Save(stream);
//	memcpy(*stream, &pathHist, sizeof(pathtype));			*stream += sizeof(pathtype);
	memcpy(*stream, &heading, sizeof(uchar));				*stream += sizeof(uchar);
	memcpy(*stream, &final_heading, sizeof(uchar));			*stream += sizeof(uchar);
//	memcpy(*stream, &element, sizeof(uchar));				*stream += sizeof(uchar);
	memcpy(*stream, &position, sizeof(uchar));				*stream += sizeof(uchar);
	return SaveSize();
	}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

// event handlers
int BattalionClass::Handle(VuFullUpdateEvent *event)
	{
	// copy data from temp entity to current entity
	BattalionClass* tmp_ent = (BattalionClass*)(event->expandedData_);

	last_move = tmp_ent->last_move;
	last_combat = tmp_ent->last_combat;
#ifdef USE_FLANKS
	lfx = tmp_ent->lfx;
	lfy = tmp_ent->lfy;
	rfx = tmp_ent->rfx;
	rfy = tmp_ent->rfy;
#endif
	supply = tmp_ent->supply;
	fatigue = tmp_ent->fatigue;
	morale = tmp_ent->morale;
	heading = tmp_ent->heading;
	final_heading = tmp_ent->final_heading;
//	element = tmp_ent->element;
	position = tmp_ent->position;
	return (GroundUnitClass::Handle(event));
	}

int BattalionClass::MoveUnit (CampaignTime time)
	{
	GridIndex       x,y,nx,ny;
	int				moving = 1;
	CampaignHeading	h;
	PathClass		temp_path;
	WayPoint		pw = NULL,w = NULL;
	Objective		lo = NULL;

#ifdef DEBUG_TIMING
	DWORD			timec = GetTickCount();
#endif

	haveWeaps = -1;

	// Check if we have a valid objective
	lo = GetUnitObjective();
	if (!lo || (Parent() && FalconLocalGame->GetGameType() == game_Campaign && !TeamInfo[GetTeam()]->gtm->IsValidObjective(GetOrders(),lo)))
		{
		if (Parent())
			lo = FindRetreatPath(this,3,0);
		if (lo)
			SetUnitOrders(GORD_RESERVE,lo->Id());
		else
			{
#ifdef ROBIN_DEBUG
			MonoPrint("Error: Battalion %d not assigned! - vegitating instead.\n",GetCampID());
#endif
			SetLastCheck (GetLastCheck () + CampaignMinutes);
			return 0;
			}
		}

	GetLocation(&x,&y);
	// VP_changes for tracing DB 
	/*
		FILE* deb = fopen("c:\\traceA10\\dbrain.txt", "a");
		fprintf(deb, "BattalionClass MoveUnit x=%f y=%f \n", x, y );   
        fclose(deb);
     */

	// Check if we're being transported
	ShiAssert( !Cargo() );

	// Check to make sure our orders and tactics are still valid.
	ChooseTactic();

	// Make some adjustments for certain tactics 
	if (GetUnitTactic() == GTACTIC_DELAY_FALLBACK)
		{
		// Keep falling back 1 km until our tactic changes
		lo = FindRetreatPath(this,1,0);
		if (!lo)
			{
			// This unit is cut off.
			CheckForSurrender();
			}
		else
			{
			GetLocation(&x,&y);
			lo->GetLocation(&nx,&ny);
			GetUnitGridPath(&temp_path,x,y,nx,ny);
			h = GetNextMoveDirection();
			x += dx[h];
			y += dy[h];
			SetUnitDestination(x,y);
			SetTempDest(1);
			}
		}
	else if (GetUnitTactic() == GTACTIC_SUPPORT_UNIT)
		{
		if (Ordered() && PositionToSupportUnit(this) < 1)
			{
			SetUnitOrders(GORD_RESERVE,GetUnitObjectiveID());
			return 0;
			}
		}
	else if (GetUnitTactic() == GTACTIC_MOVE_BRIGADE_COLUMN && GetUnitElement())
		{
		// We want to follow the previous battalion, unless we're closer to our destination in which
		// case we hang out off the road and wait for the other unit to pass
		Unit		u = NULL,brig;
		GridIndex	px,py;
		brig = GetUnitParent();
		if (brig)
			u = brig->GetPrevUnitElement(this);
		if (u && u != this && u->GetUnitTactic() == GTACTIC_MOVE_BRIGADE_COLUMN)
			{
			pw = u->GetCurrentUnitWP();
			if (pw)
				pw->GetWPLocation(&px,&py);
			else
				u->GetLocation(&px,&py);
			// Follow our previous element (temporarily)
			SetUnitDestination(px,py);
			SetTempDest(1);
			}
		else if (TempDest())
			PickFinalLocation();
		}
	else if (TempDest())
		PickFinalLocation();

	GetUnitDestination(&nx,&ny);
	if (Ordered())
		{
		ClearUnitPath();
		DisposeWayPoints();
		SetOrdered(0);
		}

	// Get our next waypoint, or build a set of waypoints if we don't have any
	if (x != nx || y != ny)
		{
		w = GetCurrentUnitWP();
		if (!w)
			{
			if (BuildGroundWP(this) < 0)				// Build a path
				{
				SetUnitObjective(FalconNullId);			// We failed for some reason, so clear our objective
				return 0;
				}
			w = GetCurrentUnitWP();
			}
		if (w)
			w->GetWPLocation(&nx,&ny);
		}
	else 
		{
		if (Retreating() && !Engaged())					// We've retreated to our destination
			SetRetreating(0);
		if (final_heading < 255)
			heading = final_heading;
		}

	// Check if we've arrived at this waypoint, and get next one if so.
	// KCK WARNING: This is duplicated at the bottom of this function -
	// we may be able to remove it from this point.
	if (w && x==nx && y==ny)
		{
		FinishUnitWP();
		w = GetCurrentUnitWP();
		if (w)
			w->GetWPLocation(&nx,&ny);
		// Also set our last objective, if we're on one
		if (lo = GetObjectiveByXY(x,y))
			last_obj = lo->Id();
		}

	// Make some adjustments for certain tactics
	if (GetUnitTactic() == GTACTIC_MOVE_BRIGADE_COLUMN && GetUnitElement())
		{
		Unit		u=NULL,brig;
		GridIndex	px,py,pwx,pwy;
		// Our next waypoint shouldn't be closer to the brigade's destination than the previous element's
		brig = GetUnitParent();
		if (brig)
			u = brig->GetPrevUnitElement(this);
		if (u)
			{
			u->GetLocation(&px,&py);
			pw = u->GetCurrentUnitWP();
			if (pw)
				{
				pw->GetWPLocation(&pwx,&pwy);
				if (DistSqu(x,y,px,py) < 25.0F || DistSqu(x,y,pwx,pwy) < DistSqu(px,py,pwx,pwy))
					{
					nx = x;		// Don't move right now - wait for previous element to pass
					ny = y;
					w = NULL;
					}
				}
			}
		}
	else if (GetUnitTactic() == GTACTIC_MOVE_HOLD)
		{
		// Hang out here til we switch tactics
		nx = x;
		ny = y;
		}
	else if (GetUnitTactic() == GTACTIC_MOVE_AIRBORNE)
		{
		if (GetCargoId() == Id())
			{
			// We're in wait mode - either time out or continue waiting
			if (TheCampaign.CurrentTime > last_move + 10 * CampaignMinutes)
				{
				SetRefused(1);
				SetUnitTactic(0);
				SetCargoId (FalconNullId);
				}
			return 0;
			}
		// Otherwise, hang out and wait for the transports 
		nx = x;
		ny = y;
		}

#ifdef ROBIN_DEBUG
#ifdef CAMPTOOL
	// Display what we're doing, if it's on..
	if (SHOWSTATS && Engaged() && TrackingOn[GetCampID()])
#endif
		{
		Objective	o;
		GridIndex	dx,dy;
		CampEntity	tar;
		int			tid = 0;

		GetUnitDestination(&dx,&dy);
		tar = (CampEntity) GetTarget();
		if (tar)
			tid = tar->GetCampID();
		o = GetUnitObjective();
		if (o)
			MonoPrint("Bat %d: %d,%d->%d,%d (%d @ %d,%d), T:%d, F:%d, E:%d, S:%d",GetCampID(),x,y,nx,ny,o->GetCampID(),dx,dy,GetUnitTactic(),GetUnitFormation(),tid,GetUnitSpeed());
		else
			MonoPrint("Bat %d: %d,%d->%d,%d (%d,%d), T:%d, F:%d, E:%d, S:%d",GetCampID(),x,y,nx,ny,dx,dy,GetUnitTactic(),GetUnitFormation(),tid,GetUnitSpeed());
		}
#endif

	// Now try to actually move
	if (x != nx || y != ny)
		{
		SetMoving(1);
		if (GetNextMoveDirection() == Here)
			{
			if (GetUnitGridPath(&temp_path,x,y,nx,ny) <= 0)
				{
#ifdef LOG_ERRORS
				char	buffer[1280],name1[80],timestr[80];
				FILE	*fp;

				sprintf(buffer,"campaign\\save\\dump\\errors.log");
				fp = fopen(buffer,"a");
				if (fp)
					{
					GetName(name1,79,FALSE);
					GetTimeString(TheCampaign.CurrentTime,timestr);
					sprintf(buffer,"%s (%d) %d,%d couldn't find path to %d,%d @ %s\n",name1,GetCampID(),x,y,nx,ny,timestr);
					fprintf(fp,buffer);
					fclose(fp);
					}
#endif
				// Couldn't find a path (usually a destroyed bridge),
				// so clear our waypoints, rebuild and quit (we'll move next time we check)
				ClearUnitPath();
				DisposeWayPoints();
				BuildGroundWP(this);
				SetUnitLastMove(time);
				return 0;
				}
			else
				path.CopyPath(&temp_path);
			}
		}
	else
		{
		// Clear offensive unit's assigned flags, so they're retasked by the brigade
		if (GetUnitCurrentRole() == GRO_ATTACK)
			{
			Objective o = GetUnitObjective();
			if (o->GetTeam() == GetTeam())
				{
				SetAssigned(0);
				o->SetAbandoned(0);
				}
			}
		// Clear the moves in case of residue
		ClearUnitPath();	
		SetMoving(0);
		}

	if (!IsAggregate())
		return 0;

	while (moving)
		{
#ifdef DEBUG
		if (!IsAggregate())
			MonoPrint("Updating deaggregate unit #%d.\n",GetCampID());
#endif
		int	formation = GetUnitFormation();
		h = GetNextMoveDirection();
		if (h != Here)
			{
			// Below is ok, if we've got time for it - otherwise, we drive through other units.
//			h = GetAlternateHeading(this,x,y,nx,ny,h);					// Check if we need to go around
			if (GetUnitTactic() == GTACTIC_DELAY_FALLBACK)
				heading = (h+4)%8;			// We back up when falling back
			else
				heading = h;
			}
		if (h<0 || h>7)
			{
			ClearUnitPath();
			ChangeUnitLocation(Here);
			moving = 0;
			if (fatigue > 0)					// Regain fatigue (faster if on reserve orders)
				fatigue -= ((GetUnitOrders() == GORD_RESERVE)? 1 : rand()%2) * REGAIN_RATE_MULTIPLIER_FOR_TE;
			}
#ifdef USE_FLANKS
		if (formation == GFORM_WEDGE || formation == GFORM_ECHELON || formation == GFORM_LINE)
			{
			// Move our flanks towards their 'correct' position (This will happen both before and after any move)
			GridIndex tx,ty;
			tx = x + LeftFlankOffset[formation-GFORM_WEDGE][heading][0];		// Left flank
			ty = y + LeftFlankOffset[formation-GFORM_WEDGE][heading][1];
			ph = DirectionTo(lfx,lfy,tx,ty);
			lfx += dx[ph];
			lfy += dy[ph];
			tx = x + RightFlankOffset[formation-GFORM_WEDGE][heading][0];		// Right flank
			ty = y + RightFlankOffset[formation-GFORM_WEDGE][heading][1];
			ph = DirectionTo(rfx,rfy,tx,ty);
			rfx += dx[ph];
			rfy += dy[ph];
			}
		else if (formation == GFORM_DISPERSED)
			{
			// Just move the flanks to within one of the unit
			if (DistSqu(x,y,lfx,lfy) > 1)
				{
				ph = DirectionTo(lfx,lfy,x,y);
				lfx += dx[ph];
				lfy += dy[ph];
				}
			if (DistSqu(x,y,rfx,rfy) > 1)
				{
				ph = DirectionTo(rfx,rfy,x,y);
				rfx += dx[ph];
				rfy += dy[ph];
				}
			}
#endif
		if (moving && ChangeUnitLocation(h)>0)
			{
			SetUnitNextMove();
			GetLocation(&x,&y);
			// Store move in path history, in opposite order
			if (GetUnitMoved() > 20)
				{
				supply--;
//				fatigue++;
				SetUnitMoved(0);
				}
#ifdef USE_FLANKS
			if (formation == GFORM_COLUMN || formation == GFORM_OVERWATCH)
				{
				int	i;
				// Update flank info - Left flank on unit, right flank 3 km behind
				lfx = rfx = x; lfy = rfy = y;
				for (i=1,ph=0; ph != Here; i++)
					{
					ph = (GetPreviousDirection(i)+4)%8;
					if (ph != Here)
						{
						rfx += dx[ph];
						rfy += dy[ph];
						}
					}
				}
#endif // USE_FLANKS
#ifdef ROBIN_DEBUG
#ifdef CAMPTOOL
			// Display what we're doing, if it's on..
			if (SHOWSTATS && Engaged() && TrackingOn[GetCampID()])
#endif
				MonoPrint(" - Moved!");
#endif
			}
		else
			moving = 0;
		}

#ifdef ROBIN_DEBUG
#ifdef CAMPTOOL
			// Display what we're doing, if it's on..
		if (SHOWSTATS && Engaged() && TrackingOn[GetCampID()])
#endif
			MonoPrint("\n");
#endif

	// Check once again (after movement) if we've arrived at this waypoint, and increment wp pointer if so.
	if (w && x==nx && y==ny)
		{
		FinishUnitWP();
		// Also set our last objective, if we're on one
		if (lo = GetObjectiveByXY(x,y))
			last_obj = lo->Id();
		}

#ifdef DEBUG_TIMING
	gBattalionMoves++;
	gAverageBattalionMoveTime = (gAverageBattalionMoveTime*(gBattalionMoves-1) + GetTickCount() - timec) / gBattalionMoves;
#endif

	return 0;
	}

int BattalionClass::DoCombat (void)
	{
	int		combat;

	SetCombatTime(TheCampaign.CurrentTime);

	if (Engaged())
		{
		FalconEntity	*e = GetTarget();
		FalconEntity	*a = GetAirTarget();

		// Check vs our Ground Target
		if (!e)											
			{
			SetTargeted(0);
			SetTarget(NULL);
			}
		else
			{
			if (Combat() && IsAggregate())
				{
				combat = ::DoCombat(this,e);
				if (combat <= 0 || Targeted())
					{
					SetTarget(NULL);						// Clear targeting data so we can look for another
					SetTargeted(0);
					}
				}
			// Check if our target should call in Artillery/CAS against us (Only for enemy battalions with bad odds)
			if (e->IsUnit() && e->GetDomain() == DOMAIN_LAND && ((Unit)e)->GetOdds() < 20 && !((Unit)e)->Supported())
				{
				// Look for artillery or air support
				if (RequestSupport((Unit)e,this))
					((Unit)e)->SetSupported(1);
				}
			}
		SetSupported(0);									// Clear supported flag so we can ask again
		// Check vs our Air Target
		if (!a)
			SetAirTarget(NULL);
		else if (Combat() && IsAggregate())
			{
			combat = ::DoCombat(this,a);
			if (combat < 0)
				SetAirTarget(NULL);							// Clear targeting data so we can look for another
			}

		// KCK HACK: Ground units engaged with air targets AND ground units were shooting much more often
		// because their last combat time was being adjusted during the radar aquisition phase. This
		// will simply force the combat time to current time if we're ever engaged with a ground unit
		if (e)
			SetCombatTime(TheCampaign.CurrentTime);
		// End HACK:
		}
	else if (Losses())
		{
		SetLosses(0);										// Clear losses flag when we disengage
		SetSupported(0);									// Clear supported flag so we can ask again
		}

	return 0;
	}

CampaignHeading FindBestHeading (Objective o, int type, int own)
	{
	Int32			bd=9999,d,i,count=0;
	GridIndex		x,y,nx,ny;
	CampaignHeading	h = 0,nh;
	Objective		n;

	if (!o)
		return Here;
	o->GetLocation(&x,&y);
	// Collect neighbor information
	for (i=0; i<o->NumLinks(); i++)
		{
		n = o->GetNeighbor(i);
		if (n)
			{
			n->GetLocation(&nx,&ny);
			d = FloatToInt32(DistanceToFront(nx,ny) * type);
			if (n->GetTeam() == own)
				d += type;
			if (d < bd)									// This objective is more important to face
				{
				h = DirectionTo(x,y,nx,ny);			
				bd = d;
				count = 1;
				}
			else if (d == bd)							// This objective is equally important to face
				{
				count++;
				if (count > 2)							// To many choices, we're going to sit on the actual objective
					return Here;
				nh = DirectionTo(x,y,nx,ny);
				if (abs(nh-h) <= 2)
					h = (CampaignHeading)((h+nh)/2);	// We can split the difference
				else if (abs(nh-h) >= 6)
					h = (CampaignHeading)((((h+nh)/2) + 4)%8);
				else
					return Here;						// Fuck it - we're going to sit on the actual objective
				}
			}
		}
	return h;
	}

void BattalionClass::SetUnitOrders (int neworders, VU_ID oid)
	{
#ifdef DEBUG
	if (gDumping)
		{
		FILE	*fp;
		int		id1;
		char	buffer[256];
		char	name1[80],name2[80],timestr[80];
		Objective	o = (Objective)vuDatabase->Find(oid);

		sprintf(buffer,"campaign\\save\\dump\\%d.BAT",GetCampID());

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

	if (neworders == GetOrders() && oid == GetUnitObjectiveID())
		return;

/*	if (Cargo() || cargo_id != FalconNullId)
		{
		// KCK: We're in mid-transport, or waiting for pickup what o-what to do?
		// Probably be cool if we re-routed the transports to new location.
		// For now, ignore.
		return;
		}
*/

	SetRefused(0);
	SetUnitObjective(oid);
	SetUnitLastMove(Camp_GetCurrentTime());
	// Force an immediate check
	SetLastCheck(0);

	// If we've received new orders, and arn't engaged, let's halt our retreat
	if (Retreating() && (!Engaged() || neworders != GORD_RESERVE))
		SetRetreating(0);

	GroundUnitClass::SetUnitOrders(neworders);

	PickFinalLocation();

	SetOrdered(1);
	}

void BattalionClass::PickFinalLocation (void)
	{
	Objective		o;
	CampaignHeading	h;
	GridIndex		x,y,dx,dy;

	DisposeWayPoints();
	o = GetUnitObjective();
	if (!o)
		{
		// KCK: We should pick a retreat path during the next cycle.
		return;
		}
	o->GetLocation(&x,&y);
	o->GetLocation(&dx,&dy);

	// Figure out where we want to hang out to complete our orders for this objective
	switch (GetUnitOrders())
		{
		// For these, we want to sit in front of the objective
		case GORD_DEFEND:
		case GORD_RECON:
			if (o->GetTeam() != GetTeam())		// Counter attack
				{
				SetUnitOrders (GORD_CAPTURE, o->Id());
				return;
				}
			h = FindBestHeading(o,1,GetTeam());
			if (h < 8)
				final_heading = h;
			else
				final_heading = 255;
			FindBestCover(x,y,h,&dx,&dy, TRUE);
			break;
		// For these, we want to sit behind the objective
		case GORD_SUPPORT:
		case GORD_AIRDEFENSE:
		case GORD_RADAR:
			// In the case of us being artillery/SAM/radar sits on our objective type, we sit on objective
			if (o->ArtillerySite() && GetUnitNormalRole() == GRO_FIRESUPPORT)
				{
				final_heading = 255;
				break;
				}
			if (o->SamSite() && GetUnitNormalRole() == GRO_AIRDEFENSE)
				{
				final_heading = 255;
				break;
				}
			if (o->RadarSite() && GetUnitNormalRole() == GRO_RECON)
				{
				final_heading = 255;
				break;
				}
			// Otherwise do as reserve
		case GORD_RESERVE:
		case GORD_COMMANDO:
			// We want to face forward, but be positioned behind.
			h = FindBestHeading(o,-1,GetTeam());
			final_heading = (h+4)%8;				
			FindBestCover(x,y,h,&dx,&dy, FALSE);
			break;
		// For these, we want to sit on the objective
		case GORD_CAPTURE:
		case GORD_SECURE:
		case GORD_ASSAULT:
		case GORD_AIRBORNE:
			final_heading = 255;
			break;
//		case GORD_AIRDEFENSE:
		case GORD_REPAIR:
		default:
			final_heading = Here;
			break;
		}

	GetLocation(&x,&y);
	if ((x != dx || y != dy) && GetMovementType() != NoMove)
		SetMoving(1);
	else
		GetLocation(&dx,&dy);
	SetUnitDestination(dx,dy);
	SetTempDest(0);
	}
	
float BattalionClass::GetSpeedModifier (void)
	{
	float	d;

// 2001-04-10 MODIFIED BY S.G. TimeOfDay RETURNS THE TIME OF THE DAY IN MILISECONDS! THAT'S NOT WHAT WE WANT... TimeOfDayGeneral WILL DO WHAT WE WANT
//	switch (TimeOfDay())			// Time of day modifiers
	switch (TimeOfDayGeneral())		// Time of day modifiers
		{
		case TOD_NIGHT:
			d = 0.7F;
			break;
		case TOD_DAWNDUSK:
			d = 0.85F;
			break;
		default:
			d = 1.0F;				
			break;
		}
	switch (GetUnitFormation())		// Unit's movement mode modifier
		{
		case GFORM_DISPERSED:
			d *= 1.5F;
			break;
		case GFORM_WEDGE:
		case GFORM_ECHELON:
			d *= 0.8F;
			break;
		default:
			break;
		}
	return d;
	}

// This is our cruise speed (from entity table)
int BattalionClass::GetCruiseSpeed (void)
	{
	return FloatToInt32(0.8F * class_data->MovementSpeed * GetSpeedModifier());
	}

int BattalionClass::GetCombatSpeed (void)
	{
	return FloatToInt32(0.6F * class_data->MovementSpeed * GetSpeedModifier());
	}

// Max speed (from entity table)
int BattalionClass::GetMaxSpeed (void)
	{
	return FloatToInt32(class_data->MovementSpeed * GetSpeedModifier());
	}

// This is the speed we're actually going
int BattalionClass::GetUnitSpeed (void)
	{
	int		mspeed,cspeed,aspeed;
	Brigade	brig = NULL;

	mspeed = GetMaxSpeed();				// Fastest we can go
	if (GetUnitTactic() == GTACTIC_MOVE_BRIGADE_COLUMN)
		brig = (Brigade)GetUnitParent();
	if (brig)
		cspeed = brig->GetUnitSpeed();	// Brigade cruise speed
	else
		cspeed = GetCruiseSpeed();		// Cruise speed
	if (Broken())
		{
		mspeed *=2;						// We get a bonus when we're running away.
		cspeed *=2;
		}
	else if (Engaged())
		{
		cspeed = mspeed = GetCombatSpeed();
		if (GetUnitCurrentRole() == GRO_ATTACK)
			{
			mspeed /=2;					// When we're offensively engaged, we must slow down, period.
			cspeed /=2;
			}
		}
	aspeed = GetArrivalSpeed(this);		// Rough guess as to how fast we need to go.
	if (cspeed > aspeed)
		return cspeed;					// Use cruise speed if it's fast enough
	return mspeed;						// Otherwise, use max speed
	}

CampaignTime BattalionClass::UpdateTime (void)
	{
	switch (GetMovementType())
		{
		case NoMove:
		default:
			return CampaignDay;
		case Air:
		case LowAir:
			return FLIGHT_MOVE_CHECK_INTERVAL * CampaignSeconds;
		case Naval:
			return NAVAL_MOVE_CHECK_INTERVAL * CampaignSeconds;
		case Foot:
			return FOOT_MOVE_CHECK_INTERVAL * CampaignSeconds;
		case Tracked:
		case Wheeled:
		case Rail:
			return TRACKED_MOVE_CHECK_INTERVAL * CampaignSeconds;
		}
	return GROUND_UPDATE_CHECK_INTERVAL * CampaignSeconds; 
	}

int BattalionClass::GetUnitSupplyNeed (int have)
	{
	if (have)
		return (GetTotalVehicles() * GetUnitSupply())/100;
	else
		return (GetTotalVehicles() * (100 - GetUnitSupply()))/100;
	}

int BattalionClass::GetUnitFuelNeed (int have)
	{
	if (GetMovementType() == Wheeled || GetMovementType() == Tracked)
		{
		if (have)
			return (GetTotalVehicles() * GetUnitSupply())/100;
		else
			return (GetTotalVehicles() * (100 - GetUnitSupply()))/100;
		}
	return 0;
	}

void BattalionClass::SupplyUnit (int supply, int fuel)
	{
	int		got;

	if (GetMovementType() == Wheeled || GetMovementType() == Tracked)
		supply = supply+fuel/2;
	got = (supply*100)/GetTotalVehicles();
	SetUnitSupply(GetUnitSupply()+got);
	}

int BattalionClass::GetDetectionRange (int mt)
	{
	int					dr=0;
	UnitClassDataType*	uc;

	uc = GetUnitClassData();
	ShiAssert(uc);
	if (IsEmitting() && uc->RadarVehicle < 255 && GetNumVehicles(uc->RadarVehicle))
// 2001-04-21 MODIFIED BY S.G. ABOVE 250 HAS A NEW MEANING SO USE THE UNIT ELECTRONIC DETECTION RANGE INSTEAD...
//		dr = uc->Detection[mt];
		dr = GetElectronicDetectionRange(mt);
	else
		SetEmitting(0);
	if (dr < VisualDetectionRange[mt])
		dr = GetVisualDetectionRange(mt);
	return dr;
	}

int BattalionClass::GetElectronicDetectionRange (int mt)
	{
	if (class_data->RadarVehicle < 255 && GetNumVehicles(class_data->RadarVehicle))
// 2001-04-21 MODIFIED BY S.G. ABOVE 250 HAS A NEW MEANING SO USE THE UNIT ELECTRONIC DETECTION RANGE INSTEAD...
//		return class_data->Detection[mt];
		{
			if (class_data->Detection[mt] > 250)
				{
				return 250 + (class_data->Detection[mt] - 250) * 50;
				}

			return class_data->Detection[mt];
		}
	return 0;
	}

void BattalionClass::ReturnToSearch (void)
	{
	if (missiles_flying < 1 && IsEmitting())
		{
// 2002-04-08 MN fix - this must also use Sylvain's new step_search_mode, not the old search_mode, or we will fall back from guide to search...
		// 2002-04-11 ADDED BY S.G. If we have no missiles in the air anymore and we were off originally, switch back off, otherwise use the interim guidance step
		if (search_mode == FEC_RADAR_OFF)
			radar_mode = FEC_RADAR_OFF;
		else
		// END OF ADDED SECTION
			radar_mode = step_search_mode; // search_mode

		if (radar_mode == FEC_RADAR_OFF)
			SetEmitting(0);
		}
	else if (!IsEmitting())
		radar_mode = FEC_RADAR_OFF;
	}

int BattalionClass::CanShootWeapon (int wid)
	{
	if (class_data->RadarVehicle < 255 && (class_data->VehicleType[class_data->RadarVehicle])) // 2002-02-07 MODIFIED BY S,G. Added check for RadarVehicle being less than 255 since 255 means 'no radar' and can cause a CTD if not checked.
	{
	RadarDataSet* radarData = &radarDatFileTable[((VehicleClassDataType *)Falcon4ClassTable[class_data->VehicleType[class_data->RadarVehicle]].dataPtr)->RadarType];
	if (radarData && WeaponDataTable[wid].GuidanceFlags & WEAP_RADAR && missiles_flying >= radarData->Maxmissilesintheair)
		return FALSE;
	}
	else if (WeaponDataTable[wid].GuidanceFlags & WEAP_RADAR && missiles_flying >= 1)
		return FALSE;
	// Check for radar guidance, and make adjustments if necessary
	if (!(WeaponDataTable[wid].GuidanceFlags & WEAP_RADAR) || GetRadarMode() == FEC_RADAR_GUIDE || GetRadarMode() == FEC_RADAR_SEARCH_100)
		return TRUE;
	return FALSE;
	}

int BattalionClass::StepRadar(int t, int d, float range)//me123 modifyed to take tracking/detection parameter
	{
#ifdef DEBUG
	char label[40] = "AGG -";
#endif
	int	radMode = GetRadarMode();
	int newMode = FEC_RADAR_OFF; // 2002-03-21 ADDED BY S.G. In order to accomodate the radar label, I'm using a var to hold the return value instead of calling 'return' for the if body themself. 'else' were also added in front of the 'if' so they are exclusive
	if (IsAggregate() || g_bOldSamActivity)
		{
		// Check if we still have any radar vehicles
		if (class_data->RadarVehicle == 255 || !GetNumVehicles(class_data->RadarVehicle))
			newMode = FEC_RADAR_OFF;

		// Check if we're already in our fire state
		else if (radMode == FEC_RADAR_GUIDE || radMode == FEC_RADAR_SEARCH_100)
			newMode = radMode;

		// Check for switch over to guide
		else if (radMode == FEC_RADAR_AQUIRE)
			{
			SetRadarMode(FEC_RADAR_GUIDE);
			newMode = FEC_RADAR_GUIDE;
			}
		else
			{
			SetRadarMode(FEC_RADAR_AQUIRE);
			// KCK: Good operators could shoot before going to guide mode. Check skill and return TRUE
			if (GetRadarMode() == FEC_RADAR_AQUIRE && rand()%100 < TeamInfo[GetOwner()]->airDefenseExperience - MINIMUM_EXP_TO_FIRE_PREGUIDE)
				SetRadarMode(FEC_RADAR_GUIDE);
			newMode = GetRadarMode();
			}

#ifdef DEBUG
		extern int g_nShowDebugLabels;

		if (g_nShowDebugLabels & 0x04) {
			switch (newMode) {
				case FEC_RADAR_SEARCH_100:
					strcat(label," S100");
					break;
				case FEC_RADAR_SEARCH_1:
					strcat(label," S1");
					break;
				case FEC_RADAR_SEARCH_2:
					strcat(label," S2");
					break;
				case FEC_RADAR_SEARCH_3:
					strcat(label," S3");
					break;
				case FEC_RADAR_GUIDE:
					strcat(label," Guide");
					break;
				case FEC_RADAR_AQUIRE:
					strcat(label," Acquire");
					break;
				case FEC_RADAR_CHANGEMODE:// do nothing
					strcat(label," Changemode");
					break;
				default:
					// No emissions this time
					strcat(label," OFF");
					break;
			}

			if (draw_pointer) {
				if (g_nShowDebugLabels & 0x04)
					draw_pointer->SetLabel(label,draw_pointer->LabelColor());
				else {
					sprintf(label," ");
					draw_pointer->SetLabel(label,draw_pointer->LabelColor());
				}
			}
		}
#endif
		return newMode;
		}

	assert (range);

/*FEC_RADAR_OFF			0x00	   	// Radar always off
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
	if (!d && !t) SetRadarMode(step_search_mode);

	if (GetRadarMode() == FEC_RADAR_CHANGEMODE && step_search_mode >= FEC_RADAR_SEARCH_1 )
	SetRadarMode(step_search_mode);// we are changing mode.. realy not off
	
	switch (GetRadarMode())
	{
	case FEC_RADAR_OFF:
	timetosearch = radarData->Timetosearch1 - skill;
	if (range <= radarData->Rangetosearch1 && !SEARCHtimer) SEARCHtimer = SimLibElapsedTime;
	else if (range >= radarData->Rangetosearch1 || SimLibElapsedTime - SEARCHtimer > timetosearch+6000.0f)SEARCHtimer = 0;
	if (range <= radarData->Rangetosearch1 && SEARCHtimer && SimLibElapsedTime - SEARCHtimer > timetosearch )
	{
	SEARCHtimer = SimLibElapsedTime;
	step_search_mode = FEC_RADAR_SEARCH_1 ;
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
			step_search_mode = FEC_RADAR_SEARCH_2;
			SEARCHtimer = SimLibElapsedTime;
		}
		break;
	case FEC_RADAR_SEARCH_2:
		AQUIREtimer = SimLibElapsedTime;
	    timetosearch = radarData->Timetosearch2 - skill;
		if (!SEARCHtimer) SEARCHtimer = SimLibElapsedTime;
		if (d && range <= radarData->Rangetosearch3 &&  SimLibElapsedTime - SEARCHtimer >= timetosearch)
		{
			step_search_mode =FEC_RADAR_SEARCH_3;
			SetRadarMode(FEC_RADAR_CHANGEMODE);
			SEARCHtimer = SimLibElapsedTime;
		}
		else if (!d)// no detection step search down
		{
		SetRadarMode(FEC_RADAR_CHANGEMODE);
		step_search_mode = FEC_RADAR_SEARCH_1 ;
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
		step_search_mode =FEC_RADAR_AQUIRE;
		SetRadarMode(FEC_RADAR_CHANGEMODE);
		AQUIREtimer = SimLibElapsedTime;		
		}
		else if(!d)//  no detection step search down
		{
		SetRadarMode(FEC_RADAR_CHANGEMODE);
		step_search_mode = FEC_RADAR_SEARCH_2 ;
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
		step_search_mode = FEC_RADAR_SEARCH_3 ; 
		}
		else if (t && range <= radarData->Rangetoguide && SimLibElapsedTime - AQUIREtimer >= timetoaquire)
		{
		step_search_mode =FEC_RADAR_GUIDE;
		SetRadarMode(FEC_RADAR_CHANGEMODE);
		return FEC_RADAR_GUIDE;
		}
		break;
	case FEC_RADAR_GUIDE:
		AQUIREtimer = SimLibElapsedTime;
		step_search_mode = FEC_RADAR_AQUIRE;
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
	if (out == FEC_RADAR_OFF) out = step_search_mode;
	return out;
	}

int BattalionClass::GetVehicleDeagData (SimInitDataClass *simdata, int remote)
	{
	static CampEntity		ent;
	static int				round;
	int						i;

	// Reinitialize static vars upon query of first vehicle
	if (simdata->vehicleInUnit < 0)
		{
		simdata->vehicleInUnit = 0;
		ent = NULL;
		if (!remote)
			{
			simdata->ptIndex = GetDeaggregationPoint(0, &ent);
			if (simdata->ptIndex) {
				// Yuck!  The first call returns only the list index, NOT a real point index.
				// To ensure we have at least one point we have to actually query for it
				// then reset again...
				simdata->ptIndex = GetDeaggregationPoint(0, &ent);
				ent = NULL;
				GetDeaggregationPoint(0, &ent);
			}

			round = 0;
			if (simdata->ptIndex == DPT_ERROR_NOT_READY)
				return -1;
			else if (simdata->ptIndex == DPT_ERROR_CANT_PLACE)
				{
				ShiAssert(0);
				KillUnit();
				return -1;
				}
			}
		}
	else
		simdata->vehicleInUnit++;

	// Three positioning schemes
	if (!remote)
		{
		if (deag_data)						// Place in our last location
			{
			int				vis;
			vis = GetNumVehicles(simdata->campSlot) - simdata->inSlot;
			simdata->x = deag_data->position_data[simdata->campSlot*3 + vis-1].x;
			simdata->y = deag_data->position_data[simdata->campSlot*3 + vis-1].y;
			simdata->z = 0.0F;				// We don't store data for air units
			simdata->ptIndex = 0;
			simdata->heading = deag_data->position_data[simdata->campSlot*3 + vis-1].heading;
			}
		else if (simdata->ptIndex)			// Place on a ground point, facing outward
			{
			float	bx=simdata->x,by=simdata->y;
			// Find the next point
			simdata->ptIndex = GetDeaggregationPoint(simdata->campSlot, &ent);
			if (!simdata->ptIndex)
				{
				// Reuse the old points, but with an offset
				ent = NULL;
				GetDeaggregationPoint(simdata->campSlot, &ent);		// Reset
				simdata->ptIndex = GetDeaggregationPoint(simdata->campSlot, &ent);
				round++;
				}

//			ShiAssert( simdata->ptIndex );	// Point list with none of the point we wanted!
			// HACK to tolerate bad data -- Shouldn't have to test this.
			if (simdata->ptIndex) {
				TranslatePointData(ent, simdata->ptIndex, &simdata->x, &simdata->y);
				simdata->y += round*50.0F;
				// Face away from center - NOTE: may need to use different facing sceme by pt type
				simdata->heading = (float)atan2(simdata->x-bx,simdata->y-by);
				}
			else 
				{
				// We're stuck -- no data for the points we needed, so let the ground AI decide...
				simdata->heading = 45.0F * DTR * heading;
				FindVehiclePosition(simdata);
				}
			simdata->z = 0.0F;
			}
		else								// KCK: Let the ground AI decide where to put them
			{
			simdata->heading = 45.0F * DTR * heading;
			FindVehiclePosition(simdata);
			}
		}

	// Determine skill (Sim only uses it for anti-air stuff right now, so bow to expedience)
//	if (GetRClass() == RCLASS_AIRDEFENSE)
		simdata->skill = ((TeamInfo[GetOwner()]->airDefenseExperience - 60) / 10) + rand()%3 - 1;
//	else
//		simdata->skill = ((TeamInfo[GetOwner()]->groundExperience - 60) / 10) + rand()%3 - 1;

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
	return  MOTION_GND_AI;
	}

int BattalionClass::GetUnitElement (void)
	{
	Brigade		brig = (Brigade)GetUnitParent();

	if (brig)
		{
		for (int i=0; i<MAX_UNIT_CHILDREN; i++)
			{
			if (Id() == brig->GetElement(i))
				return i;
			}
		ShiAssert ( 0 );
		}
	return 0;
	}

int BattalionClass::RallyUnit (int minutes)
	{
	int		maxMorale,increase;

	if (!fullstrength)
		fullstrength = GetFullstrengthVehicles();

	maxMorale = (GetTotalVehicles() * 100) / fullstrength;

	// Regain MORALE_REGAIN_RATE % of maxMorale each hour we've been waiting
	if (/*!Engaged() &&*/ GetUnitMorale() < maxMorale)
		{
		increase = (MORALE_REGAIN_RATE * minutes * maxMorale) / 6000;
		if (increase < 1)
			increase = 1;
		SetUnitMorale(GetUnitMorale() + increase);
		}
	if (GetUnitMorale() >= maxMorale)
		{
		SetUnitMorale(maxMorale);
		SetBroken(0);
		}

	return Broken();
	}

// Note: getting the pointer clears the data
UnitDeaggregationData* BattalionClass::GetUnitDeaggregationData (void)
	{
	UnitDeaggregationData *deagDataPtr = deag_data;
	deag_data = NULL;
	return deagDataPtr;
	}

void BattalionClass::ClearDeaggregationData (void)
	{
	if (deag_data)
		delete deag_data;
	deag_data = NULL;
	}

int BattalionClass::GetDeaggregationPoint (int slot, CampEntity *installation)
	{
	int			pt=0,type;
	static int	last_pt, last_support, ptListType, index = 0;
	
	if (!*installation)
		{
		// We're looking for a new list, so clear statics
		last_pt = index = last_support = ptListType = 0;

		// Check if we care about placement
		if (!Moving())
			{
			if (GetUnitNormalRole() == GRO_AIRDEFENSE)
				ptListType = SAMListType;
			else if (GetUnitNormalRole() == GRO_FIRESUPPORT)
				ptListType = ArtListType;
			}
		
		if (ptListType)
			{
			// Find the appropriate installation
			GridIndex	x,y;
			Objective	o;
			GetLocation(&x,&y);
			o = FindNearestObjective (x,y,NULL,1);
			*installation = o;

			// Find the appropriate list
			if (o)
				{
				ObjClassDataType	*oc = o->GetObjectiveClassData();
				index = oc->PtDataIndex;
				while (index)
					{
					if (PtHeaderDataTable[index].type == ptListType)
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
					fprintf(fp, "Obj %s @ %d,%d: No header list of type %d.\n",name,x,y,ptListType);
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
		if (ptListType == SAMListType && slot == uc->RadarVehicle)
			type = RadarPt;
		else if (ptListType == SAMListType && !vc->HitChance[LowAir])
			type = SupportPt;
		else if (ptListType == ArtListType && !vc->HitChance[NoMove])
			type = SupportPt;
		else if (ptListType == SAMListType)
			type = SAMPt;
		else
			type = ArtilleryPt;

		// Return the next point, if it's the base type 
		// NOTE: Log error if we don't have enough points of this type
		if (last_pt && ((ptListType == SAMListType && type == SAMPt) || (ptListType == SAMListType && type == SAMPt)))
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
		// Return the next support point, if it's a support point
		if (last_support && type == SupportPt)
			{
			last_support = pt = GetNextPt(last_support);
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
		
		// Otherwise, find one of the appropriate type
		pt = GetFirstPt(index);
		while (pt)
			{
			if (PtDataTable[pt].type == type)
				{
				if (ptListType == SAMListType && type == SAMPt)
					last_pt = pt;
				if (ptListType == ArtListType && type == ArtilleryPt)
					last_pt = pt;
				if (type == SupportPt)
					last_support = pt;
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

int BattalionClass::Reaction (CampEntity e, int knowledge, float range)
{
	int			score = 0, neworders, enemy_threat_bonus = 1;
	CampEntity	et = NULL;								// enemy's target
	MoveType	tmt,omt;
	GridIndex	x,y,ex,ey;
	
	if (!e || e->IsObjective())					// Ignore objectives for target canidates
		return 0;
	
	// Some basic info on us.
	omt = GetMovementType();
	tmt = e->GetMovementType();
	neworders = GetUnitOrders();
	
	// Aircraft on ground are ignored (technically, we could shoot at them.. but..)
	if (e->IsFlight() && !((Flight)e)->Moving())
		return 0;
	
	// KCK HACK: Don't shoot at FAC aircraft.
	if (e->IsFlight() && ((Flight)e)->GetUnitMission() == AMIS_FAC)
		return 0;
	
	// Score their threat to us
	if (knowledge & FRIENDLY_DETECTED)
		enemy_threat_bonus++;
	if (knowledge & FRIENDLY_IN_RANGE)
		enemy_threat_bonus += 2;
	et = ((Unit)e)->GetCampTarget();
	
	// All units score vs player units
	if (e->IsPlayer())
	{
		//we just want to make sure we get some score, so we are at least considered as a target
		score += 1;
		//score += (GetAproxHitChance(tmt,0)*enemy_threat_bonus)/5;
	}
	
	// All units score vs enemy engaged with this unit
	if (et == this)
		score += (e->GetAproxHitChance(omt,0)*enemy_threat_bonus)/5;
	
	// Bonus if we can shoot them
	if (knowledge & ENEMY_IN_RANGE)
		score += (GetAproxHitChance(tmt,FloatToInt32(range/2.0F))+4)/5;
	
	// Bonus for them being our target
	GetUnitDestination(&x,&y);
	e->GetLocation(&ex,&ey);
	if (x == ex && y == ey && e->IsUnit())
		score += 2 + GetAproxHitChance(tmt,FloatToInt32(range/2.0F))/5;
	
	// Now score for our ability to kill them, if we're on that sort of mission type
	switch (neworders)
	{
		case GORD_CAPTURE:
		case GORD_SECURE:
			// Added bonus for them being engaged with any unit
			if (et && et->IsUnit())
				score += (e->GetAproxHitChance(omt,0)*enemy_threat_bonus)/5;
			break;
		case GORD_AIRDEFENSE:
			// Added bonus for them attacking
			if (et && (tmt == Air || tmt == LowAir))
				score += (GetAproxHitChance(tmt,0)*enemy_threat_bonus)/5;
			break;
		case GORD_DEFEND:
			// Added bonus for them attacking
			if (((Unit)e)->Engaged())
				score += (e->GetAproxHitChance(omt,0)*enemy_threat_bonus)/5;
			break;
		case GORD_RESERVE:
		case GORD_SUPPORT:
		case GORD_REPAIR:
		case GORD_RECON:
		case GORD_RADAR:
			// The worst threat scores highest
			score += (e->GetAproxHitChance(omt,0)*enemy_threat_bonus)/5;
			break;
		default:
			break;
	}
	
	// Minimal reaction against broken units
	if (e->IsBattalion() && ((Unit)e)->Broken())
		score = 1;
	
	return score;
} 

int BattalionClass::ChooseTactic (void)
{
	int			priority=0,tid;
	
	tid = FirstGroundTactic;
	while ((tid < FirstGroundTactic + GroundTactics) && (priority<2))
	{
		priority = CheckTactic(tid) + rand()%2;
		if (priority<2)
			tid++;
	}
	
	if (tid != GetTactic())
		SetOrdered(1);
	
	// Make Adjustments due to tactic
	if (tid == GTACTIC_WITHDRAW)
	{
		SetAssigned(0);		
		if (GetUnitTactic() != GTACTIC_WITHDRAW)
		{
			Objective	o;
			Brigade		brigade;
			
			// Check for objective abandonment (possibly only if we're nearby)
			if (GetUnitOrders() == GORD_DEFEND && ourObjDist < 5)
			{
				o = GetUnitObjective();
				if (o)
					o->SetAbandoned(1);
			}

 			MonoPrint ("Retreating\n");

			SetRetreating(1);
			// Re-request orders from our brigade, if we have one
			brigade = (Brigade) GetUnitParent();
			if (brigade)
				// Force the brigade to reassign us immediately
				brigade->SetLastCheck(0);
			else
			{
				// Otherwise, retreat as best we can
				o = FindRetreatPath(this,3,0);
				if (o)
					SetUnitOrders(GORD_RESERVE,o->Id());
			}
		}
	}
	
#ifdef ROBIN_DEBUG
	if (GetUnitTactic() != tid && TrackingOn[GetCampID()] || (GetUnitParent() && TrackingOn[GetUnitParent()->GetCampID()]))
	{
		CampEntity target = (CampEntity) GetTarget();
		MonoPrint("Battalion %d (%s) chose tactic %s vs %d.\n",GetCampID(),OrderStr[GetUnitOrders()],TacticsTable[tid].name,(target)? target->GetCampID():0);
	}
#endif
	SetUnitTactic(tid);
	
	return tid;
}

int BattalionClass::CheckTactic (int tid)
	{
	if (haveWeaps < 0)
		{
		GridIndex		x,y,dx,dy;
		Objective		o;
		FalconEntity	*e = GetTarget();

		if (Engaged() && !e)
			SetEngaged(0);
		if (GetUnitSupply() > 30)
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
	if (!CheckRange(tid,ourObjDist))
		return 0;
//	if (!CheckDistToFront(tid,ourFrontDist))
//		return 0;
	if (!CheckStatus(tid,Broken()))
		return 0;
	if (!CheckOdds(tid,GetOdds()))
		return 0;
	if (CheckWeapons(tid) == 1 && !haveWeaps)
		return 0;
	if (!CheckRole(tid,GetUnitNormalRole()))
		return 0;
	if (CheckSpecial(tid) == 1 && GetUnitParentID() == FalconNullId)
		return 0;						// Check if part of a brigade
	if (CheckSpecial(tid) == 2 && TeamInfo[GetTeam()]->GetGroundAction()->actionType != GACTION_OFFENSIVE)
		return 0;						// KCK Check if our offensive's started yet.
	// Refused() means our request was refused. These are no longer valid tactics
	if (!CheckAirborne(tid,!Refused()))
		return 0;
	if (!CheckMarine(tid,!Refused()))
		return 0;

	return GetTacticPriority(tid);
	}

float BattalionClass::AdjustForSupply(void)
	{
	return (supply/100.0F) * ((100-fatigue)/100.0F) * (morale/100.0F) * GroundExperienceAdjustment(GetOwner());
	}

void BattalionClass::SimSetLocation (float x, float y, float z)
	{
	GridIndex	cx,cy,nx,ny;

	// Check if battalion has moved, or needs to do detection
	GetLocation(&cx,&cy);
	nx = SimToGrid(y);
	ny = SimToGrid(x);
	if (cx != nx || cy != ny)
		{
		SetPosition(x,y,z);
		MakeCampBaseDirty (DIRTY_POSITION, DDP[90].priority);
		//	MakeCampBaseDirty (DIRTY_POSITION, SEND_SOON);
		MakeCampBaseDirty (DIRTY_ALTITUDE, DDP[91].priority);
		//	MakeCampBaseDirty (DIRTY_ALTITUDE, SEND_SOON);
		SetUnitNextMove();
		SetUnitLastMove(Camp_GetCurrentTime());
		DetectOnMove();
		}
	// Currently, we're going to assume we'll do detection on Campaign thread
	}

void BattalionClass::GetRealPosition (float *x, float *y, float *z)
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
		*z = TheMap.GetMEA(XPos(),YPos());;
		return;
		}

	speed = (float) GetUnitSpeed() * KPH_TO_FPS;
	dist = speed * movetime;
	heading = h * 45.0F * DTR;
	mlSinCos(&sincos, heading);
	*x = XPos() + dist * sincos.cos;
	*y = YPos() + dist * sincos.sin;
	*z = TheMap.GetMEA(*x,*y);
	}

void BattalionClass::HandleRequestReceipt(int type, int them, VU_ID flight)
	{
	switch (type)
		{
		case AMIS_AIRCAV:
			SetCargoId (flight);
			break;
		default:
			break;
		}
	}

CampaignTime BattalionClass::GetMoveTime (void)
	{
	if (last_move && TheCampaign.CurrentTime > last_move)
		return TheCampaign.CurrentTime - last_move;

	last_move = TheCampaign.CurrentTime;

	return 0;
	}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void BattalionClass::SetFullStrength (uchar fs)
{
	fullstrength = fs;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void BattalionClass::SetUnitMorale (int m)
	{
	if (m > 100)
		m = 100;
	if (m < 0)
		m = 0;
	morale = m;
	MakeBattalionDirty (DIRTY_MORALE, DDP[92].priority);
	//	MakeBattalionDirty (DIRTY_MORALE, SEND_EVENTUALLY);
	}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void BattalionClass::SetUnitFatigue (int f)
	{
	if (f > 100)
		f = 100;
	if (f < 0)
		f = 0;
	fatigue = f;
	MakeBattalionDirty (DIRTY_FATIGUE, DDP[93].priority);
	//	MakeBattalionDirty (DIRTY_FATIGUE, SEND_EVENTUALLY);
	}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void BattalionClass::SetUnitSupply (int s)
	{
	if (s > 100)
		s = 100;
	if (s < 0)
		s = 0;
	supply = s;
	MakeBattalionDirty (DIRTY_SUPPLY, DDP[94].priority);
	//	MakeBattalionDirty (DIRTY_SUPPLY, SEND_EVENTUALLY);
	}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void BattalionClass::MakeBattalionDirty (Dirty_Battalion bits, Dirtyness score)
{
	if (!IsLocal())
		return;

	if (VuState() != VU_MEM_ACTIVE)
		return;

	if (!IsAggregate())
	{
		score = (Dirtyness) ((int) score * 10);
	}

	dirty_battalion |= bits;

	MakeDirty (DIRTY_BATTALION, score);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void BattalionClass::WriteDirty (unsigned char **stream)
{
	unsigned char
		*ptr;

	ptr = *stream;

	//MonoPrint ("  BC %08x", dirty_battalion);

	// Encode it up
	*(uchar*)ptr = (uchar) dirty_battalion;
	ptr += sizeof (uchar);

	if (dirty_battalion & DIRTY_SUPPLY)
	{
		*(Percentage*)ptr = supply;
		ptr += sizeof (Percentage);
	}

	if (dirty_battalion & DIRTY_MORALE)
	{
		*(Percentage*)ptr = morale;
		ptr += sizeof (Percentage);
	}

	if (dirty_battalion & DIRTY_FATIGUE)
	{
		*(Percentage*)ptr = fatigue;
		ptr += sizeof (Percentage);
	}

	dirty_battalion = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void BattalionClass::ReadDirty (unsigned char **stream)
{
	unsigned char
		*ptr,
		bits;
	ShiAssert(FALSE == F4IsBadReadPtr(stream, 4));

	// JB 010221 CTD
	if (!stream || F4IsBadReadPtr(stream, sizeof(unsigned char*)))
		return;

	ptr = *stream;

	// JB 010221 CTD
	if (!ptr || F4IsBadWritePtr(ptr, sizeof(unsigned char)))
		return;

	bits = *(uchar*)ptr;
	ptr += sizeof (uchar);

	if (F4IsBadWritePtr(ptr, sizeof(Percentage))) // JB 010221 CTD
		return; // JB 010221 CTD

	//MonoPrint ("  BC %08x", bits);

	if (bits & DIRTY_SUPPLY)
	{
		supply = *(Percentage*)ptr;
		ptr += sizeof (Percentage);
	}

	if (bits & DIRTY_MORALE)
	{
		morale = *(Percentage*)ptr;
		ptr += sizeof (Percentage);
	}

	if (bits & DIRTY_FATIGUE)
	{
		fatigue = *(Percentage*)ptr;
		ptr += sizeof (Percentage);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

