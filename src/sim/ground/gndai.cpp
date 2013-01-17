// GNDAI, this is where all the important stuff happens in the AI (or atleast, I 
// call routines that cause important stuff to happen here!).
// By Mark McCubbin 
// (c)1997
//
// Notes: Ultimately should be a complete replacement for Leon Ground AI, 
// done this was to ensure something is always working. (In the end Leon Exec 
// funtion will simply call Process (); for this class and nothing else!!!).
// 
// Notes: Transitions aren't handled as specified in the design docs (yet!).
// By Mark McCubbin 
// (c)1997
//
#include <process.h>
#include <math.h>

#include "stdhdr.h"
#include "mesg.h"
#include "otwdrive.h"
#include "initdata.h"
#include "waypoint.h"
#include "f4error.h"
#include "object.h"
#include "simobj.h"
#include "simdrive.h"
#include "Graphics/Include/drawsgmt.h"
#include "entity.h"
#include "classtbl.h"
#include "sms.h"
#include "fcc.h"
#include "PilotInputs.h"
#include "MsgInc/DamageMsg.h"
#include "Graphics/Include/Rviewpnt.h"
#include "guns.h"
#include "hardpnt.h"
#include "sfx.h"
#include "Unit.h"
#include "fsound.h"
#include "soundfx.h"
#include "fakerand.h"
#include "acmi/src/include/acmirec.h"
#include "Battalion.h"
#include "camp2sim.h"
#include "campbase.h"
#include "ui/include/ui_ia.h"
#include "Graphics/Include/drawbsp.h"
#include "handoff.h"
#include "gnddef.h"
#include "ground.h"
#include "radar.h"
#include "team.h"
#include "update.h"
#include "aircrft.h"
/* S.G. */
#include "simbase.h"
/* S.G. */
#include "Object.h"
/* MN */
#include "Find.h"
/* MN */
#include "Classtbl.h"

/* MN */extern int g_nlookAroundWaterTiles;

#ifdef CHECK_PROC_TIMES
ulong gCommChooseTarg = 0;
ulong gSelNewTarg = 0;
ulong gRadarTarg = 0;
ulong gConfirmTarg = 0;
#endif

#ifdef USE_SH_POOLS
MEM_POOL	GNDAIClass::pool;
#endif

void SetLabel (SimBaseClass* theObject);

extern GridIndex  	dx[17];
extern GridIndex	dy[17];

#define GNDAI_SINF(a) sinf(a)
#define GNDAI_COSF(a) cosf(a)

#define AI_SCALEFAC		50		// Used for debug code..
#define AI_COORD_SCALE	2

#define	OPTIMAL_VEHICLE_ROTATION	(90.0F * DTR)		// Optimal rotation speed (radians/second)

// edg: this is an attempt to reduce vehicles from going thru each other --
// just spread em out.  Scale existing table offsets by this amount
#define OFFSET_SCALE				(2.5f)
#define RANDOM_OFFSET_SCALE			(0.5f)

// The formation data is stored as relative offsets, the first coord is redundant
// each "leader" is actually the first coord in the list (therefore it has to be
// 0, 0 - if it were used ).
//

//MI
extern bool g_bRealisticAvionics;
extern bool g_bAGRadarFixes;
extern bool g_bFireOntheMove; // FRB - Test

void AdjustOffset(float c, float s, float *x, float*y, float xo, float yo);
void AdjustOffset(float heading, float *x, float*y, float xo, float yo);
static int InPosition (GNDAIClass* us);

// KCK: These need to be relative offsets (rotated by battalion's heading) using
// the above function
AIOffsetType SquadFormations[ GNDAI_FORM_END ][NO_OF_SQUADS] = {
	// Scatter mode
	{ {0, 0 }, {-100, -100 }, {100, -100} },

	// COLUMN
	{ {0, 0 }, { 0, -50 }, { 0, -100} },

	// Unused
	{ {0, 0 }, {-50, -50 }, {50, -50} },

	// OVERWATCH
	{ {0, 0 }, {-50, -50 }, {0, -100} },

	// Wedge
	{ {0, 0 }, {-50, -50 }, {50, -50} },

	// Echelon
	{ {0, 0 }, {50, -50 }, {100, -100} },

	// LINE
	{ {0, 0 }, {50, 0 }, {-50, 0} }
};

AIOffsetType PlatoonFormations[ GNDAI_FORM_END ][NO_OF_PLATOONS] = {
	// Scatter mode
	{ {0, 0 }, {-300, -300 }, {300, -300}, {0, -600 } },

	// COLUMN
	{ {0, 0 }, { 0, -250 }, { 0, -500}, { 0, -750 } },

	// Wedge
	{ {0, 0 }, { -250, -250 }, {250, -250}, { 500, -500} },

	// OVERWATCH
	{ {0, 0 }, { -125, -250 }, { 125, -250}, { 0, -500} },

	// Wedge
	{ {0, 0 }, { -250, -250 }, {250, -250}, { 500, -500} },

	// Echelon
	{ {0, 0 }, { 250, -250 }, { 500, -500}, { 0, -500} },

	// LINE
	{ {0, 0 }, { -250, 0 }, { 250, 0}, { 500, 0} },
};

AIOffsetType CompanyFormations[ GNDAI_FORM_END ][NO_OF_COMPANIES] = {
	// Scatter mode
	{ {0, 0 }, {-1000, -1000 }, {1000, -1000}, {0, -2000 }  },

	// COLUMN
	{ {0, 0 }, { 0, -1000 }, { 0, -2000}, { 0, -3000 } },

	// Wedge
	{ {0, 0 }, { -1000, -1000 }, {1000, -1000}, { 2000, -2000} },

	// OVERWATCH
	{ {0, 0 }, { -500, -1000 }, { 500, -1000}, { 0, -2000} },

	// Wedge
	{ {0, 0 }, { -1000, -1000 }, {1000, -1000}, { 2000, -2000} },

	// Echelon
	{ {0, 0 }, { 1000, -1000 }, { 2000, -500}, { 0, -2000} },

	// LINE
	{ {0, 0 }, { -1000, 0 }, { 1000, 0}, { 2000, 0} },

};

// Amount of feet a vehicle can be randomized away from it's "perfect" location
AIOffsetType FormationRandomness[ GNDAI_FORM_END ] = {
	// Scatter mode
	{ 50.0F * RANDOM_OFFSET_SCALE, 50.0F * RANDOM_OFFSET_SCALE  },

	// COLUMN
	{ 15.0F * RANDOM_OFFSET_SCALE, 10.0F * RANDOM_OFFSET_SCALE },

	// Wedge
	{ 25.0F * RANDOM_OFFSET_SCALE, 25.0F * RANDOM_OFFSET_SCALE },

	// OVERWATCH
	{ 25.0F * RANDOM_OFFSET_SCALE, 25.0F * RANDOM_OFFSET_SCALE },

	// Wedge
	{ 25.0F * RANDOM_OFFSET_SCALE, 25.0F * RANDOM_OFFSET_SCALE },

	// Echelon
	{ 25.0F * RANDOM_OFFSET_SCALE, 25.0F * RANDOM_OFFSET_SCALE },

	// LINE
	{ 10.0F * RANDOM_OFFSET_SCALE, 100.0F * RANDOM_OFFSET_SCALE },
};


// KCK: Had to fix this, as Mark still didn't have it correct.. Grrr..
typedef struct {
	int			leader_idx;
	int			rank;
	int			uid;
} BattalionInitType;

BattalionInitType BattalionHeir[48] = {
	{ -1, GNDAI_BATTALION_COMMANDER, 0 },
	{  0, GNDAI_SQUAD_LEADER, 1 },
	{  0, GNDAI_SQUAD_LEADER, 2 },

	{  0, GNDAI_PLATOON_COMMANDER, 1 },
	{  3, GNDAI_SQUAD_LEADER, 1 },
	{  3, GNDAI_SQUAD_LEADER, 2 },

	{  0, GNDAI_PLATOON_COMMANDER, 2 },
	{  6, GNDAI_SQUAD_LEADER, 1 },
	{  6, GNDAI_SQUAD_LEADER, 2 },

	{  0, GNDAI_PLATOON_COMMANDER, 3 },
	{  9, GNDAI_SQUAD_LEADER, 1 },
	{  9, GNDAI_SQUAD_LEADER, 2 },

	{  0, GNDAI_COMPANY_COMMANDER, 1 },
	{ 12, GNDAI_SQUAD_LEADER, 1 },
	{ 12, GNDAI_SQUAD_LEADER, 2 },

	{ 12, GNDAI_PLATOON_COMMANDER, 1 },
	{ 15, GNDAI_SQUAD_LEADER, 1 },
	{ 15, GNDAI_SQUAD_LEADER, 2 },

	{ 12, GNDAI_PLATOON_COMMANDER, 2 },
	{ 18, GNDAI_SQUAD_LEADER, 1 },
	{ 18, GNDAI_SQUAD_LEADER, 2 },

	{ 12, GNDAI_PLATOON_COMMANDER, 3 },
	{ 21, GNDAI_SQUAD_LEADER, 1 },
	{ 21, GNDAI_SQUAD_LEADER, 2 },

	{  0, GNDAI_COMPANY_COMMANDER, 2 },
	{ 24, GNDAI_SQUAD_LEADER, 1 },
	{ 24, GNDAI_SQUAD_LEADER, 2 },

	{ 24, GNDAI_PLATOON_COMMANDER, 1 },
	{ 27, GNDAI_SQUAD_LEADER, 1 },
	{ 27, GNDAI_SQUAD_LEADER, 2 },

	{ 24, GNDAI_PLATOON_COMMANDER, 2 },
	{ 30, GNDAI_SQUAD_LEADER, 1 },
	{ 30, GNDAI_SQUAD_LEADER, 2 },

	{ 24, GNDAI_PLATOON_COMMANDER, 3 },
	{ 33, GNDAI_SQUAD_LEADER, 1 },
	{ 33, GNDAI_SQUAD_LEADER, 2 },

	{  0, GNDAI_COMPANY_COMMANDER, 3 },
	{ 36, GNDAI_SQUAD_LEADER, 1 },
	{ 36, GNDAI_SQUAD_LEADER, 2 },

	{ 36, GNDAI_PLATOON_COMMANDER, 1 },
	{ 39, GNDAI_SQUAD_LEADER, 1 },
	{ 39, GNDAI_SQUAD_LEADER, 2 },

	{ 36, GNDAI_PLATOON_COMMANDER, 2 },
	{ 42, GNDAI_SQUAD_LEADER, 1 },
	{ 42, GNDAI_SQUAD_LEADER, 2 },

	{ 36, GNDAI_PLATOON_COMMANDER, 3 },
	{ 45, GNDAI_SQUAD_LEADER, 1 },
	{ 45, GNDAI_SQUAD_LEADER, 2 },
};

// This is used as a temporary pointer haven for vehicles being added
// It assumes vehicles are added in increasing order
static GNDAIClass	*simb[48];
static GNDAIClass	*batCmdr;

// Create a new GroundAI
GNDAIClass *NewGroundAI (GroundClass *us, int position, BOOL isFirst, int skill)
{
	GNDAIClass	*gai;

	if (isFirst)
	{
		// Reset our creation data
		memset(simb, 0, sizeof (GNDAIClass*)*48);
	}

	if (position)
	{
		// Check if our leader exists -
		//while (position && !simb[BattalionHeir[position].leader_idx]) // JB 010220 CTD
		while (position && BattalionHeir[position].leader_idx >= 0 && !simb[BattalionHeir[position].leader_idx]) // JB 010220 CTD
			position = BattalionHeir[position].leader_idx;
		if (BattalionHeir[position].leader_idx >= 0) // JB 001203 //+
			gai = simb[position] = new GNDAIClass (us, simb[BattalionHeir[position].leader_idx], (short)BattalionHeir[position].rank, BattalionHeir[position].uid, skill );
		else // JB 001203 //+
			gai = simb[position] = new GNDAIClass (us, NULL, BattalionHeir[position].rank, (short)BattalionHeir[position].uid, skill ); // JB 001203 //+
	}
	else
		gai = simb[position] = new GNDAIClass (us, NULL, BattalionHeir[position].rank, (short)BattalionHeir[position].uid, skill );

	if (isFirst) {
		batCmdr = gai;
	}
	gai->battalionCommand = batCmdr;

	return gai;
}

// Initialize the ground ai class
GNDAIClass::GNDAIClass ( GroundClass *s, GNDAIClass *l, short r, int unit_id, int skill )
{
	battalionCommand = NULL;
	mlTrig trig;
	parent_unit = (UnitClass*) s->GetCampaignObject();
	gndTargetPtr = NULL;
	airTargetPtr = NULL;
	leader = NULL;
	self = s;
	rank = r;
	skillLevel = skill;
	distLOD = 0.0f;
	ideal_x = self->XPos();
	ideal_y = self->YPos();
	ideal_h = self->Yaw();
	SetLeader(l);
	// Check for towed vehicles
	if (s->isTowed){
		move_backwards = PI;
	}
	else {
		move_backwards = 0.0F;
	}
	// Update current trig values
	if (!l || this == l){
		mlSinCos (&trig, ideal_h);
		isinh = trig.sin;
		icosh = trig.cos;
	}
	else {
		isinh = l->isinh;
		icosh = l->icosh;
	}
	through_x = through_y = 0.0F;
	leftToGoSq = 0.0F;

	// edg note: In instant action it seems there are no formations --
	// everyone object is created separately.  If leader is NULL, insure
	// our rank is battalion commander and leader is set
	if ( leader == NULL ){
		rank = GNDAI_BATTALION_COMMANDER;
	}

	// KCK: Not sure why he need all these, but I'll fix them to work right...
	if (rank & GNDAI_BATTALION_LEADER){
		squad_id = platoon_id = company_id = 0;
	}
	else if (rank & GNDAI_COMPANY_LEADER){
		squad_id = platoon_id = 0;
		company_id = unit_id;
	}
	else if (rank & GNDAI_PLATOON_LEADER){
		squad_id = 0;
		platoon_id = unit_id;
		company_id = l->company_id;
	}
	else {
		squad_id = unit_id;
		platoon_id = l->platoon_id;
		company_id = l->company_id;
	}

	VehicleClassDataType *vc = (VehicleClassDataType*)Falcon4ClassTable[self->Type()-VU_LAST_ENTITY_TYPE].dataPtr;
	maxvel = vc->MaxSpeed * KPH_TO_FPS;

	if (parent_unit){
		formation = (GNDAIFormType) parent_unit->GetUnitFormation();
		unitvel = (float)parent_unit->GetCruiseSpeed();
	}
	else {
		formation = GNDAI_FORM_WEDGE; //(GNDAIFormType)(rand () % GNDAI_FORM_COLUMN);
		unitvel = maxvel;
	}
	lastMoveTime = SimLibElapsedTime;

	// note: we can adjust this for AI levels / gun type
	airFireRate = (10 - skillLevel + rand() % (3 * (5-skillLevel)) ) * SEC_TO_MSEC;
	gndFireRate = (2 + rand()%6) * SEC_TO_MSEC;

	nextTurretCalc = SimLibElapsedTime;
	if (rank){
		nextGroundFire = nextAirFire = SimLibElapsedTime;
	}
	else {
		nextGroundFire = SimLibElapsedTime + ((int)(s->vehicleInUnit * SEC_TO_MSEC) % (int)gndFireRate);
		nextAirFire = SimLibElapsedTime + ((int)(s->vehicleInUnit * 5 * SEC_TO_MSEC) % (int)airFireRate);
	}

	if (!maxvel){
		// If we can't move, set ourselves to halted
		moveState = GNDAI_MOVE_HALTED;
	}
	else if (SimDriver.RunningCampaignOrTactical() && !parent_unit->IsTaskForce() ){
		moveState = GNDAI_MOVE_GENERAL;
	}
	else {
		// Otherwise assume we're using waypoints to move
		moveState = GNDAI_MOVE_WAYPOINT;
	}
	moveFlags = 0;
}


// Pick a leader. Heil Hitler!
void GNDAIClass::SetLeader (GNDAIClass* newLeader)
{
	leader = newLeader;	
	/*	GNDAIClass* oldLeader = leader;

	// edg: don't reference/deref if ourself
	if (newLeader != oldLeader)
	{
	leader = newLeader;
	if (oldLeader && oldLeader->self != self)
	{
	VuDeReferenceEntity(oldLeader->self);
	}
	if (newLeader)
	{
	if ( newLeader->self != self )
	VuReferenceEntity(newLeader->self);
	formation = newLeader->formation;
	}
	}
	*/
}

GNDAIClass::~GNDAIClass (void){
	SetGroundTarget(NULL);
	SetAirTarget(NULL);
}


void GNDAIClass::SetGroundTarget( SimObjectType *newTarget ){
	if (newTarget == gndTargetPtr)
		return;

	if (gndTargetPtr) {
		gndTargetPtr->Release(  );
	}

	if (newTarget) {
		newTarget->Reference(  );
		// 2001-03-21 ADDED BY S.G. SINCE WE ARE A SIM THINGY, WE CAN'T RELY ON THE DetectVs CODE TO FLAG THE TARGET AS DETECTED. I NEED TO DO IT MYSELF HERE
		if (newTarget->BaseData()->IsSim())
			((SimBaseClass *)newTarget->BaseData())->GetCampaignObject()->SetSpotted(self->GetCampaignObject()->GetTeam(),TheCampaign.CurrentTime, 1); // 2002-02-11 MODIFIED BY S.G. Added '1' to flag it identified
		else
			((CampBaseClass *)newTarget->BaseData())->SetSpotted(self->GetCampaignObject()->GetTeam(),TheCampaign.CurrentTime, 1); // 2002-02-11 MODIFIED BY S.G. Added '1' to flag it identified
		// END OF ADDED SECTION
	}

	gndTargetPtr = newTarget;
}


void GNDAIClass::SetAirTarget( SimObjectType *newTarget )
{
	if (newTarget == airTargetPtr)
		return;

	if (airTargetPtr) {
		airTargetPtr->Release(  );
	}

	// 2001-03-21 COMMENT BY S.G. INTENTIONNALY, I'M NOT SETTING SPOTTED HERE BECAUSE I NEED BETTER WAY THAN CAMPAIGN ENGINE TO MAKE DETECTION...
	if (newTarget) {
		newTarget->Reference(  );
	}

	airTargetPtr = newTarget;
}


/*
 ** Name: ProcessTargeting
 ** Description:
 **		In this function we do all the processing necessary to set up a
 **		target.   Only the highest leader will maintain a targetList.
 **		Subordinates off of the leader will use this list to determine
 **		their own targeting (ie setting their targetPtr) which is essentially
 **		based on the type of weapons they carry.
 */
void GNDAIClass::ProcessTargeting ( void )
{
	FalconEntity	*campTargetEntity;
	SimObjectType	*newTarget;

	// If we're towed and moving, don't bother holding a target
	// This will prevent us from shooting on the move...
	// TODO:  Make the moving state a discrete
	//        flag so the visual object can change state
	//        too.  Should have an intermediate state with
	//        no motion OR firing to model "set up/tear down".
	if (self->isTowed && (self->GetVt() > 0.1f) && !g_bFireOntheMove) {
		self->SetTarget( NULL );
		return;
	}

	// Are we the battalion commander?
	if ( this == battalionCommand ){
		parent_unit->UnsetChecked();
		parent_unit->ChooseTarget();

		// Get the battalion's ground target (if any)
		campTargetEntity = parent_unit->GetTarget();
		if (campTargetEntity){
			// Create our shared ground target object (campaign target)
			if (!gndTargetPtr || gndTargetPtr->BaseData() != campTargetEntity){
				SetGroundTarget( new SimObjectType(campTargetEntity) );
			}

			// KCK: As far as I can see, only range.ataFrom,az and el are being used from here.
			// Seems like we can just do those here for our targetPtr -
			CalcRelAzElRangeAta (self, gndTargetPtr);
		}
		else {
			SetGroundTarget( NULL );
		}

		// Get the battalion's air target (if any)
		campTargetEntity = parent_unit->GetAirTarget();
		if (campTargetEntity){
			// Create our shared ground target object (campaign target)
			if (!airTargetPtr || airTargetPtr->BaseData() != campTargetEntity){
				SetAirTarget( new SimObjectType(campTargetEntity) );
			}

			// 2000-09-11 ADDED BY S.G. SO WE GET THE azFrom and elFrom as well when the target is jamming
			if (airTargetPtr->BaseData()->IsSPJamming()) {
				// The next statement will set az, el, ata, ataFrom and range to what ground unit needs and leave
				// what we require (azFrom and elFrom) intact.
				// May be a new function just calculating that would be better...
				SimObjectType *tmpTarget = airTargetPtr->next;
				airTargetPtr->next = NULL;
				CalcRelGeom(self, airTargetPtr, NULL, 1.0F / SimLibMajorFrameTime);
				airTargetPtr->next = tmpTarget;
			}
			// END OF ADDED SECTION

			// KCK: As far as I can see, only range.ataFrom,az and el are being used from here.
			// Seems like we can just do those here for our targetPtr
			CalcRelAzElRangeAta (self, airTargetPtr);
		}
		else {
			AircraftClass		*player;
			AircraftClass		*best = NULL;
			int		bestReact = 1;
			Team	who = self->GetTeam();
			VuSessionsIterator		sit(FalconLocalGame);
			FalconSessionEntity* session = (FalconSessionEntity*) sit.GetFirst();
			while (session){
				player = (AircraftClass*) session->GetPlayerEntity();
				if (player && player->IsAirplane() && GetRoE(who,player->GetTeam(),ROE_GROUND_FIRE) == ROE_ALLOWED){
					int	react,det;
					float d;

					det = Detected(parent_unit,player,&d);
					//if (det & ENEMY_DETECTED) // JB SOJ
					// 2001-03-24 ADDED BY S.G. NEED TO CHECK IF DETECTED BEFORE WE CAN REACT...
					if (det & REACTION_MASK){
						// END OF ADDED SECTION EXCEPT FOR INDENTATION
						react = parent_unit->Reaction(player->GetCampaignObject(),det,d);
						if(react > bestReact){
							best = player;
							// 2001-03-24 ADDED BY S.G. LETS KEEP THIS ONE AS HIGHEST SHALL WE...
							bestReact = react;
							// END OF ADDED SECTION
						}
					}
				}
				session = (FalconSessionEntity*) sit.GetNext();
			}

			if (!best){
				SetAirTarget( NULL );
			}
			else {
				SetAirTarget( new SimObjectType(best) );
				CalcRelAzElRangeAta (self, airTargetPtr);
			}
		}

	}

	// SCR 10/19/98
	// We could assign targets to our subordinates here if we wanted a "push" model where
	// the leader did some intelligent assignment.  For now, we just let each vehicle
	// pick an element of the aggregate target obtained from the leader's AI.

	// Decide if we want to shoot at the battalion's air or ground target
	// If we are an emitter, we always choose the airtarget
	if (self->isGroundCapable && (!self->isEmitter || self->isShip)) { // JPO let ships decide.
		if (self->isAirCapable) {
			// Arbitrarily use a 2:1 distribution for vehicles which can do both
			if ((rand() % 3) > 1)
				newTarget = battalionCommand->gndTargetPtr;
			else
				newTarget = battalionCommand->airTargetPtr;
		} else {
			newTarget = battalionCommand->gndTargetPtr;
		}
	} else if (self->isAirCapable || self->isEmitter){
		newTarget = battalionCommand->airTargetPtr;
	}
	else 
		newTarget = NULL;

	CampEntity newUnit = NULL;
	CampEntity oldUnit = NULL;
	if (newTarget){
		if(newTarget->BaseData()->IsSim())
			newUnit = ((SimBaseClass*)newTarget->BaseData())->GetCampaignObject();
		else
			newUnit = (CampEntity)newTarget->BaseData();
	}

	if (self->targetPtr){
		if(self->targetPtr->BaseData()->IsSim())
			oldUnit = ((SimBaseClass*)self->targetPtr->BaseData())->GetCampaignObject();
		else
			oldUnit = (CampEntity)self->targetPtr->BaseData();
	}

	// Pick an element of the battalion's aggregate target as our personal target
	//	1. if we don't have a target
	//	2. our target is dead
	//	3. our commander wants us to shoot at another unit

	// RV - Biker - Switch to next target if we did take a hit (pctStrength <= 0.0f)
	//if(!self->targetPtr || self->targetPtr->BaseData()->IsDead() || newUnit != oldUnit)
	if (
		!self->targetPtr || 
		((SimBaseClass *)self->targetPtr->BaseData())->pctStrength <= 0.0f || 
		self->targetPtr->BaseData()->IsDead() || newUnit != oldUnit
	){
		self->SetTarget( SimCampHandoff( newTarget, self->targetList, HANDOFF_RANDOM ) );
	}
	else if (self->targetPtr->BaseData()->IsCampaign()){
		self->SetTarget( SimCampHandoff( self->targetPtr, self->targetList, HANDOFF_RANDOM ) );
	}

	if (self->targetPtr){ 
		if (self->isEmitter && !self->targetPtr->BaseData()->OnGround()){
			RadarClass* radar = (RadarClass*)FindSensor( self, SensorClass::Radar );
			ShiAssert( radar );
			bool tracking = FALSE;
			bool detecting = FALSE;
			float range = 0.0f;
			int i =0;
			/* 2002-03-21 MODIFIED BY S.G. If we're the emitter, we're also the radar's platform. 
			Therefore radar->platform->targetPtr is self->targetPtr. 
			There's an easier way to do this plus do some checks about our target state
			   for (i=0; i<((SimMoverClass*)(SimVehicleClass*)self)->numSensors; i++)
			   {
			   if (radar->platform->targetPtr->localData->sensorState[i]> SensorClass::Detection)
			   tracking = TRUE;
			   if (radar->platform->targetPtr->localData->sensorState[i]>= SensorClass::Detection)
			   detecting = TRUE;
			   if (radar->platform->targetPtr->localData->range > range) 
				range = radar->platform->targetPtr->localData->range ;
			   }
			   */

			// If handoff switched target, localData will have zeros only, 
			// fix this by calling CalcRelAzElRangeAta and then run a sensor sweep
			SimObjectLocalData* localData= self->targetPtr->localData;
			if (
				localData->ataFrom == 0.0f && 
				localData->az == 0.0f  && 
				localData->el == 0.0f && localData->range == 0.0f
			){
				CalcRelAzElRangeAta(self, self->targetPtr);

				radar->SetDesiredTarget(self->targetPtr);
				// Flag it as a first sweep so it doesn't have to wait before locking
				radar->SetFlag(RadarClass::FirstSweep); 
				radar->Exec(self->targetList);
			}
			// If our radar has no target or if it has the SAME base object
			// as us but different target pointers, switch our radar target pointer to our target pointer
			else if (
				!radar->CurrentTarget() || (
					radar->CurrentTarget() != self->targetPtr && 
					radar->CurrentTarget()->BaseData() == self->targetPtr->BaseData()
				)
			){
				radar->SetDesiredTarget(self->targetPtr);
				radar->SetFlag(RadarClass::FirstSweep); // Flag it as a first sweep since change
				radar->Exec(self->targetList);
			}

			for (i = 0; i < ((SimMoverClass*)self)->numSensors; i++){
				SimObjectLocalData* localData= self->targetPtr->localData;
				if (localData->sensorState[i] > SensorClass::Detection){
					tracking = TRUE;
				}

				if (localData->sensorState[i] >= SensorClass::Detection){
					detecting = TRUE;
				}

				if (localData->range > range){
					range = localData->range ;
				}
			}

			// 2002-01-29 MN add a debug label that shows the current SAM radar mode
#define SHOW_RADARSTATE
#ifdef SHOW_RADARSTATE
			char label[40];
			sprintf(label,"OFF");
			if (detecting)
				sprintf (label,"Detect");
			if (tracking)
				sprintf (label,"Track");
			//me123 modifyed to take tracking/detection parameter))
			switch (self->GetCampaignObject()->StepRadar(tracking, detecting, range) ){
				case FEC_RADAR_SEARCH_100:
					radar->SetEmitting(TRUE);
					radar->SetDesiredTarget(self->targetPtr);
					strcat(label," S100");
					break;
				case FEC_RADAR_SEARCH_1:
					radar->SetEmitting(TRUE);
					radar->SetDesiredTarget(self->targetPtr);
					strcat(label," S1");
					break;
				case FEC_RADAR_SEARCH_2:
					radar->SetEmitting(TRUE);
					radar->SetDesiredTarget(self->targetPtr);
					strcat(label," S2");
					break;
				case FEC_RADAR_SEARCH_3:
					radar->SetEmitting(TRUE);
					radar->SetDesiredTarget(self->targetPtr);
					strcat(label," S3");
					break;
				case FEC_RADAR_GUIDE:
					radar->SetEmitting(TRUE);
					radar->SetDesiredTarget(self->targetPtr);
					strcat(label," Guide");
					break;
				case FEC_RADAR_AQUIRE:
					radar->SetEmitting(TRUE);
					radar->SetDesiredTarget(self->targetPtr);
					strcat(label," Acquire");
					break;
				case FEC_RADAR_CHANGEMODE:// do nothing
					strcat(label," Changemode");
					break;
				default:
					// No emissions this time
					strcat(label," OFF");
					radar->SetEmitting(FALSE);
					radar->SetDesiredTarget(NULL);
					break;
			}
			char buf[40];
			sprintf(buf," %f",range);
			strcat(label,buf);

			extern int g_nShowDebugLabels;
			if (self->drawPointer){
				if (g_nShowDebugLabels & 0x04){
					((DrawableBSP*)self->drawPointer)->SetLabel(
						label,((DrawableBSP*)self->drawPointer)->LabelColor()
					);
				}
			}

#else

			//me123 modifyed to take tracking/detection parameter))
			switch (self->GetCampaignObject()->StepRadar(tracking,detecting, range) )
			{
				case FEC_RADAR_SEARCH_100:
				case FEC_RADAR_SEARCH_1:
				case FEC_RADAR_SEARCH_2:
				case FEC_RADAR_SEARCH_3:
				case FEC_RADAR_GUIDE:
				case FEC_RADAR_AQUIRE:
					radar->SetEmitting(TRUE);
					radar->SetDesiredTarget(self->targetPtr);
					break;
				case FEC_RADAR_CHANGEMODE:// do nothing
					break;
				default:
					// No emissions this time

					radar->SetEmitting(FALSE);
					radar->SetDesiredTarget(NULL);
					break;
			}
#endif
		}

		if( (!self->isEmitter || self->isAirCapable) && this != battalionCommand){
			self->SelectWeapon(FALSE);
			SimWeaponClass *theWeapon = self->Sms->GetCurrentWeapon();
			//if we can't shoot at the current target, don't shoot
			if (!theWeapon){
				self->SetTarget( NULL );
			}
		}
	}
	else if (self->isEmitter){
		self->ReturnToSearch();
	}
}

// Process, do all the work for the AI.
// i.e: decide where to go.
void GNDAIClass::Process ( void )
{
	// Do general house keeping for the indivual units as detailed in the AI
	// design doc - I suspect more functionality to be added.
	// Calculate the "ideal" position for the low level formations,
	// this information will be used to determine the actual position in 
	// Follow way point, this allows us to change formation in battle and the
	// vehicles should smoothly re-group.
	//
	// Note: Need to get "tatics" from the campiagn level (mode).
	//

	// KCK: This picks a location we want to get to
	// Probably it should also:
	// a) figure out how to move along the road, if that's what we're doing
	// b) figure out how to get to cover, if that's what were doing
	// c) figure out how to point at an enemy and shoot, if that's what we're doing

	// edg: I'm not entirely sure this is working correctly in IA.
	// additionally check for leader == NULL ( meaning I'm a batallion )
	// and follow waypoints....
	if ( leader == NULL && self->curWaypoint && rank != GNDAI_BATTALION_COMMANDER)
	{
		rank = GNDAI_BATTALION_COMMANDER;
	}
	else switch ( rank ) 
	{
		case GNDAI_BATTALION_COMMANDER:
		default:
			Order_Battalion();
			break;
		case GNDAI_COMPANY_COMMANDER:
			Order_Company();
			break;
		case GNDAI_PLATOON_COMMANDER:	
			Order_Platoon();
			break;
		case GNDAI_SQUAD_LEADER:
			Order_Squad();
			break;
	}

	if ((moveState == GNDAI_MOVE_WAYPOINT) && (ideal_x == self->XPos()) && (ideal_y == self->YPos()))
	{
		// if in waypoint mode choose another
		if ( self->curWaypoint )
		{
			switch ( rank ) 
			{
				case GNDAI_COMPANY_COMMANDER:
				case GNDAI_PLATOON_COMMANDER:	
				case GNDAI_SQUAD_LEADER:
				case GNDAI_BATTALION_COMMANDER:
				default:
					// get the next waypoint
					self->curWaypoint = self->curWaypoint->GetNextWP ();

					// if none, loop back to 1st waypoint
					if ( self->curWaypoint == NULL )
						self->curWaypoint = self->waypoint;
					break;
			}
		}
	}
}

void GNDAIClass::Order_Battalion (void)
{
	float deltaX, deltaY;
	float distToDestSqu,closeToRange;
	mlTrig	trig;
	Objective o;
	int i,j;
	int delta = 1-g_nlookAroundWaterTiles;


	moveFlags |= GNDAI_MOVE_BATTALION;		// We're the battalion lead

	// are we halted?
	if (moveState == GNDAI_MOVE_HALTED)
	{
		deltaX = deltaY = 0.0f;
		ideal_x = self->XPos();
		ideal_y = self->YPos();
		distToDestSqu = 0.0f;
	}
	else
	{
		parent_unit->SimSetLocation(self->XPos(),self->YPos(),0);
		deltaX = (ideal_x - self->XPos());
		deltaY = (ideal_y - self->YPos());
		distToDestSqu = deltaX * deltaX + deltaY * deltaY;
		if ( distToDestSqu < 50.0f )
		{
			// Move towards other ground targets (first priority)
			if ( self->targetPtr && self->targetPtr->BaseData()->OnGround() )
			{
				deltaX = self->targetPtr->BaseData()->XPos()  - self->XPos() ;
				deltaY = self->targetPtr->BaseData()->YPos()  - self->YPos() ;
				ideal_h = (float)atan2(deltaY, deltaX);
				// KCK: Stopping when in range wasn't really working (A unit with one artillery peice would
				// sit and snipe out of range of the majority of it's vehicles).
				// I'm going to try determining what to do by role type:
				// Basically, if we're attacking we'll close to 1 km range, otherwise, we'll stop
				// when within max range. That way defenders will sit still, artillery will sit still, but attackers
				// will always close to minimum range.
				if (parent_unit->GetUnitCurrentRole() == GRO_ATTACK)
					closeToRange = KM_TO_FT;
				else
					closeToRange = parent_unit->GetWeaponRange(self->targetPtr->BaseData()->GetMovementType())*KM_TO_FT - KM_TO_FT;
				if ( self->targetPtr->localData->range < closeToRange)
				{
					distToDestSqu = 0.0f;
					deltaX = 0.0f;
					deltaY = 0.0f;
					ideal_x = self->XPos();
					ideal_y = self->YPos();
				}
				else
				{
					deltaX *= 0.25f;
					deltaY *= 0.25f;
					ideal_x = self->XPos() + deltaX;
					ideal_y = self->YPos() + deltaY;
				}
			}
			else if ((moveState == GNDAI_MOVE_GENERAL) && parent_unit)
			{
				// Follow path
				lastGridX = gridX;
				lastGridY = gridY;
				// Update the parent battalion's position
				formation = (GNDAIFormType) parent_unit->GetUnitFormation();
				parent_unit->GetLocation (&gridX, &gridY);
				moveDir = parent_unit->GetNextMoveDirection();
				through_x = through_y = 0.0F;
				if (moveDir < 8)
				{
					// 2002-02-16 MN Aaaaaaaahh - WHO DID THIS BS - THEY MUST HAVE BEEN DRUNK !!! 
					// Look what they have done: since when are Sim coordinates SHORT ??? No wonder the ground units move strangely...
					// GridToSim returns a float !!! And ideal_x/y are floats, too... Aaaaaaaaaaaaahhhhh....;-)
					//					ideal_y = (short)GridToSim(gridX + dx[moveDir]);
					//					ideal_x = (short)GridToSim(gridY + dy[moveDir]);
					ideal_y = GridToSim(gridX + dx[moveDir]);
					ideal_x = GridToSim(gridY + dy[moveDir]);
					deltaX = ideal_x - self->XPos() ;
					deltaY = ideal_y - self->YPos() ;
					if (formation == GNDAI_FORM_COLUMN)
					{
						through_x = self->XPos();
						through_y = self->YPos();
					}
					ideal_h = moveDir*45*DTR;
				}
				else
				{
					// Force the battalion to do an update on the next Campaign update, if we don't
					// have a destination (NOTE: Often no destination is fine, but this isn't very
					// heavy anyway).
					((Battalion)self->GetCampaignObject())->SetLastCheck(0);
				}
				//				MonoPrint("Battalion Following Path: Next move = %d, heading = %3.3f\n",moveDir,ideal_h);
			}
			else if (self->curWaypoint)
			{
				// Follow waypoints
				self->curWaypoint->GetLocation(&ideal_x, &ideal_y, &ideal_h);
				deltaX = ideal_x - self->XPos() ;
				deltaY = ideal_y - self->YPos() ;
				ideal_h = (float)atan2(deltaY, deltaX);
			}
			// Calculate our trig values for formation offsets
			mlSinCos (&trig, ideal_h);
			isinh = trig.sin;
			icosh = trig.cos;
			distToDestSqu = deltaX * deltaX + deltaY * deltaY;
		}
	}

	// determine our next location's terrain type
	if (distToDestSqu)
	{
		// KCK: For simplicity, I'm going to check current tile's coverage. This should be
		// enough for our purposes until we get bridges to work.
		GridIndex	cx,cy;
		ShiAssert(parent_unit);
		parent_unit->GetLocation(&cx,&cy);
		// 2002-02-23 MN Let ground units traverse the water when there is a bridge - now that they move correctly...
		o = GetObjectiveByXY(cx,cy);
		// we may have 3 tile bridges in the future, so use g_nlookAroundWaterTiles instead of hardcoding
		if (!o) // can be at 2 tile bridges -> scan around our parent units location for a bridge
		{
			for (j=delta; j<g_nlookAroundWaterTiles; j++){
				for (i=delta; i<g_nlookAroundWaterTiles; i++){
					if (GetCover(cx+i,cy+j) != Water){
						continue;
					}
					o = GetObjectiveByXY(cx+i,cy+j);
					if (o){
						goto FoundBridge;
					}
				}
			}
		}
FoundBridge:
		if ( parent_unit->IsBattalion() )
		{
			if (GetCover(cx,cy) == Water && !(o && o->GetType() == TYPE_BRIDGE))
			{
				// that's it, we don't move anymore
				moveState = GNDAI_MOVE_HALTED;
				ideal_x = self->XPos();
				ideal_y = self->YPos();
			}

		}
		else // task force
		{
			// MN also look around tiles from the taskforce - should prevent having ships going on land
			bool onWater = true;
			for (j=delta; j<g_nlookAroundWaterTiles; j++)
			{
				for (i=delta; i<g_nlookAroundWaterTiles; i++)
				{
					if (GetCover(cx+i,cy+j) == Water)
						continue;
					onWater = false;
					goto NearCoast;
				}
			}
NearCoast:
			if (!onWater)
			{
				// that's it, we don't move anymore
				moveState = GNDAI_MOVE_HALTED;
				ideal_x = self->XPos();
				ideal_y = self->YPos();
			}

		}
	}
}

void GNDAIClass::Order_Company (void)
{
	ShiAssert( battalionCommand );	// This should always be TRUE, right?

	if (moveState != GNDAI_MOVE_HALTED)
	{
		// We can move, so get into formation
		formation = battalionCommand->formation;
		if (formation != GNDAI_FORM_COLUMN || !CheckThrough())
		{
			if (InPosition(this))
			{
				// Just rotate in place, if we're in position already
				ideal_h = leader->ideal_h;
			}
			else
			{
				// Otherwise, move out.
				ideal_x = leader->ideal_x;
				ideal_y = leader->ideal_y;
				AdjustOffset(battalionCommand->icosh, battalionCommand->isinh, &ideal_x, &ideal_y, CompanyFormations[formation][company_id].x, CompanyFormations[formation][company_id].y);
			}
		}
	}
	else if (!(moveFlags & GNDAI_MOVE_FIXED_POSITIONS))
	{
		ideal_h = leader->ideal_h;
	}
}

void GNDAIClass::Order_Platoon (void)
{
	ShiAssert( battalionCommand );	// This should always be TRUE, right?

	if (moveState != GNDAI_MOVE_HALTED)
	{
		// We can move, so get into formation
		formation = battalionCommand->formation;
		if (formation != GNDAI_FORM_COLUMN || !CheckThrough())
		{
			if (InPosition(this))
			{
				// Just rotate in place, if we're in position already
				ideal_h = leader->ideal_h;
			}
			else
			{
				// Otherwise, move out.
				ideal_x = leader->ideal_x;
				ideal_y = leader->ideal_y;
				AdjustOffset(battalionCommand->icosh, battalionCommand->isinh, &ideal_x, &ideal_y, CompanyFormations[formation][company_id].x, CompanyFormations[formation][company_id].y);
				AdjustOffset(battalionCommand->icosh, battalionCommand->isinh, &ideal_x, &ideal_y, PlatoonFormations[formation][platoon_id].x, PlatoonFormations[formation][platoon_id].y);
			}
		}
	}
	else if (!(moveFlags & GNDAI_MOVE_FIXED_POSITIONS))
	{
		ideal_h = leader->ideal_h;
	}
}

void GNDAIClass::Order_Squad (void)
{
	ShiAssert( battalionCommand );	// This should always be TRUE, right?

	if (moveState != GNDAI_MOVE_HALTED)
	{
		// We can move, so get into formation
		formation = battalionCommand->formation;
		if (formation != GNDAI_FORM_COLUMN || !CheckThrough())
		{
			if (InPosition(this))
			{
				// Just rotate in place, if we're in position already
				ideal_h = leader->ideal_h;
			}
			else
			{
				// Otherwise, move out.
				float	rx,ry;
				int		randx,randy;
				ideal_x = leader->ideal_x;
				ideal_y = leader->ideal_y;
				randx = FloatToInt32(FormationRandomness[formation].x);
				randy = FloatToInt32(FormationRandomness[formation].x);
				rx = (float)(rand() % randx) - (randx/2);
				ry = (float)(rand() % randy) - (randy/2);
				AdjustOffset(battalionCommand->icosh, battalionCommand->isinh, &ideal_x, &ideal_y, CompanyFormations[formation][company_id].x+rx, CompanyFormations[formation][company_id].y+ry);
				AdjustOffset(battalionCommand->icosh, battalionCommand->isinh, &ideal_x, &ideal_y, PlatoonFormations[formation][platoon_id].x, PlatoonFormations[formation][platoon_id].y);
				AdjustOffset(battalionCommand->icosh, battalionCommand->isinh, &ideal_x, &ideal_y, SquadFormations[formation][squad_id].x, SquadFormations[formation][squad_id].y);
			}
		}
	}
	else if (!(moveFlags & GNDAI_MOVE_FIXED_POSITIONS))
	{
		ideal_h = leader->ideal_h;
	}
}

int GNDAIClass::CheckThrough (void)
{
	ShiAssert( battalionCommand );

	// Check to see if we want to move through a turn point
	if (battalionCommand->through_x != through_x || battalionCommand->through_y != through_y)
	{
		moveFlags &= ~GNDAI_WENT_THROUGH;
		through_x = battalionCommand->through_x;
		through_y = battalionCommand->through_y;
	}
	if (!(moveFlags & GNDAI_WENT_THROUGH) && through_x && through_y)
	{
		ideal_x = through_x;
		ideal_y = through_y;
		return 1;
	}
	return 0;
}

// Move one frame's worth towards our destination
void GNDAIClass::Move_Towards_Dest ( void )
{
	float	delx=0.0F,dely=0.0F,delh=0.0F,rotvel= OPTIMAL_VEHICLE_ROTATION,speed=0.0F,tx=0.0F,ty=0.0F;
	mlTrig	trig;

	ShiAssert( battalionCommand );

	// get delta to next x and y location
	if ((moveState == GNDAI_MOVE_HALTED) || (battalionCommand->moveState == GNDAI_MOVE_HALTED)){
		delx = 0.0f;
		dely = 0.0f;
		moveState = GNDAI_MOVE_HALTED;
	}
	else {
		delx = ideal_x - self->XPos();
		dely = ideal_y - self->YPos();
		leftToGoSq = delx*delx + dely*dely;
		if (leftToGoSq)	{
			// determine direction we need to go
			if (leftToGoSq >= 100.0F){
				ideal_h = (float)atan2(dely, delx) + move_backwards;
			}
			else // if (leftToGoSq < 100.0F || ideal_h-battalionCommand->ideal_h > PI || ideal_h-battalionCommand->ideal_h < -PI)
			{
				// we're close enough to our destination to think again, or
				// we just don't want to bother backing up.
				ideal_x = self->XPos();
				ideal_y = self->YPos();
				if (formation == GNDAI_FORM_COLUMN)
				{
					moveFlags |= GNDAI_WENT_THROUGH;
					Process();
					//					Move_Towards_Dest();
					return;
				}
				ideal_h = battalionCommand->ideal_h + move_backwards;
				delx = dely = 0.0F;
			}
		}
		else {
			moveFlags |= GNDAI_WENT_THROUGH;
		}
	}

	// Check if we need to turn
	delh = ideal_h - self->Yaw();
	if (!(moveFlags & GNDAI_MOVE_FIXED_POSITIONS) && delh != 0.0F) {
		//if (delh >= PI || (delh < 0 && delh > -PI))
		//	rotvel = -1.0F * OPTIMAL_VEHICLE_ROTATION;
		//else if (delh <= -PI || delh > 0)
		//	rotvel = OPTIMAL_VEHICLE_ROTATION;

		if (delh >= PI || (delh < 0 && delh > -PI)){
			rotvel *= -1.0F;
		}
		delh = (float)min(fabs(delh),(2.0F*PI)-fabs(delh));
		if (delh < 2.0F * OPTIMAL_VEHICLE_ROTATION * SimLibMajorFrameTime){
			// Snap to new heading
			self->SetYPR(ideal_h, self->Pitch(), self->Roll());
			self->SetYPRDelta(0.0f, 0.0f, 0.0f);
		}
		else {
			// start rotating
			float  newyaw = self->Yaw()+rotvel*SimLibMajorFrameTime;
			if (newyaw > PI)
				newyaw -= 2.0F*PI;
			if (newyaw < -PI)
				newyaw += 2.0F*PI;
			self->SetYPR(newyaw, self->Pitch(), self->Roll());
			self->SetYPRDelta(rotvel, 0.0f, 0.0f);
		}
	}
	else {
		self->SetYPRDelta(0.0f, 0.0f, 0.0f);
	}

	if (delx || dely){
		// Calculate speed.
		if (delh > 5.0F * DTR){
			// cornering speed
			speed = min(20.0F,maxvel);
		}
		else if (rank == GNDAI_BATTALION_COMMANDER || formation == GNDAI_FORM_COLUMN){
			// go at unit cruise speed
			speed = unitvel;
		}
		else{
			// adjust speed to catch up/wait up (max at maxvel)
			speed = min(maxvel, (leftToGoSq / battalionCommand->leftToGoSq) * maxvel);
		}
		if (move_backwards > 0.0F){
			speed *= -1.0F;
		}
		mlSinCos (&trig, self->Yaw());
		tx = speed * trig.cos;
		ty = speed * trig.sin;
		if (fabs(delx) > fabs(tx) * SimLibMajorFrameTime){
			delx = tx;
		}
		if (fabs(dely) > fabs(ty) * SimLibMajorFrameTime){
			dely = ty;
		}
		//MI
		/*
		// sfr: removed VT
		if(g_bRealisticAvionics && g_bAGRadarFixes){
			self->SetVt(speed);
		}*/
	}
	/* sfr: removed setVt
	//MI
	else
	{
		if(g_bRealisticAvionics && g_bAGRadarFixes){
			self->SetVt(0.0F);
		}
	}*/
	// KCK: This is done in Exec()
	//	self->SetPosition (self->XPos()+delx, self->YPos()+dely, self->ZPos());

	// RV - Biker - Don't move at full speed when damaged
	float decFast = 0.5f*SimLibMajorFrameTime;
	float decSlow = 0.1f*SimLibMajorFrameTime;

	if (self->pctStrength > 0.75f)
		self->SetDelta (delx, dely, 0.0F);
	else if (self->pctStrength > 0.25f)
		self->SetDelta (max(delx*(self->pctStrength), self->XDelta()-decSlow), max(dely*(self->pctStrength), self->YDelta()-decSlow), 0.0F);
	else
		self->SetDelta (max(0.0f, self->XDelta()-decFast), max(0.0f, self->YDelta()-decFast), 0.0F);
}


// KCK: This is where 99% of the visibility will occur, and of course, Mark hasn't touched it yet..
// Basically, this determines where we're going to place a vehicle before we've even created it.
// It should fill the initData stucture with an x & y position, which is passed the the vehicle
// and broadcast to remote machines in the deaggregation message
// There are the following cases:
// a) We're on a road, in column:	place appropriately along road using the tile features.
// b) We're in a city:				place somewhere on roads
// c) We're in the open:			place anywhere but in water/buildings/forest/etc, in formation if possible
// All other cases are handled by the campaign, and will not cause this function to be called
//
// Data known upon entry:	initData.campSlot,inSlot 
//							initData.campUnit = the Campaign Unit
//							initData.heading = best guess heading
//							initData.x/y/z = position of campaign Unit
//
void FindVehiclePosition(SimInitDataClass *initData)
{
	// Calculate battalion position (0-47) of this vehicle
	int		position = initData->campSlot*3 + initData->inSlot;
	int		formation = ((UnitClass*)(initData->campBase))->GetUnitFormation();
	int		cur_leader_idx;

	// KCK: for now, place in formation (essentially, option 3 without checking for obsticals)
	cur_leader_idx = BattalionHeir[position].leader_idx;
	while (cur_leader_idx >= 0 && position)
	{
		if (BattalionHeir[position].rank == GNDAI_SQUAD_LEADER)
		{
			// Offset from our platoon leader + a random offset based on formation type

			float	rx,ry;
			int		randx,randy;
			randx = FloatToInt32(FormationRandomness[formation].x);
			randy = FloatToInt32(FormationRandomness[formation].y);
			rx = (float)(rand() % randx) - (randx/2);
			ry = (float)(rand() % randy) - (randy/2);

			AdjustOffset(initData->heading, &initData->x, &initData->y, SquadFormations[formation][BattalionHeir[position].uid].x+rx, SquadFormations[formation][BattalionHeir[position].uid].y+ry);

			//AdjustOffset(initData->heading, &initData->x, &initData->y, SquadFormations[formation][BattalionHeir[position].uid].x, SquadFormations[formation][BattalionHeir[position].uid].y);
		}
		if (BattalionHeir[position].rank == GNDAI_PLATOON_COMMANDER)
		{
			// Offset from our company leader
			AdjustOffset(initData->heading, &initData->x, &initData->y, PlatoonFormations[formation][BattalionHeir[position].uid].x, PlatoonFormations[formation][BattalionHeir[position].uid].y);
		}
		if (BattalionHeir[position].rank == GNDAI_COMPANY_COMMANDER)
		{
			// Offset from our battalion leader
			AdjustOffset(initData->heading, &initData->x, &initData->y, CompanyFormations[formation][BattalionHeir[position].uid].x, CompanyFormations[formation][BattalionHeir[position].uid].y);
		}
		position = cur_leader_idx;
		cur_leader_idx = BattalionHeir[position].leader_idx;
	}
	initData->z = 0;
}


void AdjustOffset(float c, float s, float *x, float*y, float xo, float yo)
{
	xo *= OFFSET_SCALE;
	yo *= OFFSET_SCALE;

	*x += xo * s + yo * c;
	*y -= xo * c - yo * s;
}

void AdjustOffset(float h, float *x, float*y, float xo, float yo)
{
	mlTrig trig;

	mlSinCos (&trig, h);

	xo *= OFFSET_SCALE;
	yo *= OFFSET_SCALE;

	*x += xo * trig.sin + yo * trig.cos;
	*y -= xo * trig.cos - yo * trig.sin;
}

/*
 ** This function is called when a unit dies.
 ** We need to try and find a new leader to promote and set others
 ** to point at him.
 */
void GNDAIClass::PromoteSubordinates (void)
{
	GNDAIClass	*bc = NULL;
	GroundClass *theObj;
	GroundClass *newLead = NULL;

	// sfr: @todo remove these checks
	if (!self || F4IsBadWritePtr(self, sizeof(GroundClass)) || F4IsBadCodePtr((FARPROC) self->GetCampaignObject())) // JB 010318 CTD
	{
		if (F4IsBadWritePtr(this, sizeof(GNDAIClass)))
			return;

		// set our own leader (if any) to NULL to deref
		SetLeader( NULL );
		battalionCommand = NULL;
		rank = 0;
		return;
	} // JB 010318 CTD

	// edg: changed from assert.  A unit may have been killed just after a
	// reagg and there may be no-one to promote!
	if ( !self->GetCampaignObject()->GetComponents())
	{
		// set our own leader (if any) to NULL to deref
		SetLeader( NULL );
		battalionCommand = NULL;
		rank = 0;
		return;
	}

	// KCK: In the case where we're simply reaggregating these guys, do a simple clean
	// up - just eliminate pointers to us - this is ok, because everyone will do this
	// shortly afterwards
	if (self->GetCampaignObject()->IsAggregate())
	{
		{
			VuListIterator	vehicleWalker( self->GetCampaignObject()->GetComponents() );
			theObj = (GroundClass*)vehicleWalker.GetFirst();
			while (theObj)
			{
				if (theObj->gai->battalionCommand == this)
					theObj->gai->battalionCommand = NULL;
				if (theObj->gai->leader == this)
					theObj->gai->SetLeader(NULL);
				theObj = (GroundClass*)vehicleWalker.GetNext();
			}
		}
		SetLeader( NULL );
		battalionCommand = NULL;
		rank = 0;
		return;
	}

	// Look for a new global commander, assuming we're the current one
	if (this == battalionCommand)
	{
		// Note who our supreme commander is.  Emperical evidence suggests we could get
		// here with one or more "expoding" objects in the list, so we have to find the
		// first NON-exploding object (if any) to be the new battalion commander.
		{
			VuListIterator	vehicleWalker( self->GetCampaignObject()->GetComponents() );
			theObj = (GroundClass*)vehicleWalker.GetFirst();
			while (theObj && theObj != self && (theObj->IsExploding() || theObj->IsDead()))
				theObj = (GroundClass*)vehicleWalker.GetNext();
		}

		if (theObj)
		{
			ShiAssert( theObj->gai );
			ShiAssert( theObj->VuState() == VU_MEM_ACTIVE );
			ShiAssert (rank == GNDAI_BATTALION_COMMANDER);
			bc = theObj->gai;
			bc->rank = rank = GNDAI_BATTALION_COMMANDER;
			bc->SetLeader( bc );
			// Make the new commander our leader, so that promotions will work
			leader = bc; 
		}
	}

	if (rank)
	{
		// Get the first vehicle in our battalion (or task force)
		VuListIterator	vehicleWalker( self->GetCampaignObject()->GetComponents() );
		theObj = (GroundClass*)vehicleWalker.GetFirst();
		// loop thru elements in flight
		while (theObj) 
		{

			ShiAssert( theObj->gai );
			ShiAssert( theObj != self );

			// KCK: This creates problems by allowing a vehicle to
			// null it's pointers prematurely. The easiest solution
			// is to allow dead vehicles to inherit command, and then
			// have them pass it along to the next guy when they call
			// PromoteSubordinates()
			/*			if ( theObj->IsExploding() || theObj->IsDead() ) 
						{
						SetLeader( NULL );
						battalionCommand = NULL;
						rank = 0;
						theObj = (GroundClass*)vehicleWalker.GetNext();
						continue;
						}
						*/
			ShiAssert(theObj->VuState() == VU_MEM_ACTIVE);

			// do we promote this guy?
			if ( theObj->gai->leader == this)
			{
				if (!newLead)
				{
					// We're the new leader - set some stuff
					newLead = theObj;
					newLead->gai->rank = rank;
					newLead->gai->SetLeader (leader);
					if (newLead->drawPointer)
						SetLabel (newLead);
				}
				else
				{
					// Subordinate set to New Leader
					theObj->gai->SetLeader (newLead->gai);
				}
			}

			// Make sure our battalion commander pointer is set correctly
			if (bc)
				theObj->gai->battalionCommand = bc;

			// Get the next member of our group
			theObj = (GroundClass*)vehicleWalker.GetNext();
		}
	}

	// set our own leader (if any) to NULL to deref
	SetLeader( NULL );
	battalionCommand = NULL;
	rank = 0;
}

#define LOD_MAX_DIST		20000.0f
#define LOD_MAX_DIST_SQU	(LOD_MAX_DIST*LOD_MAX_DIST)

/*
 ** Name: GetApproxViewDist
 ** Description:
 **		Sets the distLOD var based on distance from camera
 */
	void
GNDAIClass::SetDistLOD ( void )
{
	Tpoint viewLoc;
	float	xd,yd,zd,distsqu;

	// get view pos
	if (OTWDriver.GetViewpoint())
	{
		OTWDriver.GetViewpoint()->GetPos( &viewLoc );

		xd = viewLoc.x - self->XPos();
		yd = viewLoc.y - self->YPos();
		zd = viewLoc.z - self->ZPos();	
		distsqu = xd*xd + yd*yd + zd*zd;

		// Never label out beyond 10 NM
		if (distsqu < 100.0F)
		{
			distLOD = max (0.0F, (LOD_MAX_DIST_SQU - distsqu)/LOD_MAX_DIST_SQU);
			distLOD *= distLOD;
		}
		else
		{
			distLOD = 0.0F;
		}
	}
	else
	{
		distLOD = 0.0F;
	}
}

//
// This just returns 0 or 1 depending on wether the LeadVeh and/or we are moving or not.
//
static int InPosition (GNDAIClass* us)
{
	// WARNING:  There is potential for silliness here due to floating point precision errors.
	// Particularly the fact that MSVC/Intel don't always ensure that compare arguments are rounded
	// to the same precision before the compare.

	if (us->ideal_x != us->self->XPos() || 
			us->ideal_y != us->self->YPos() || 
			us->battalionCommand->self->XDelta() != 0.0F || 
			us->battalionCommand->self->YDelta() != 0.0F)

		return 0;
	else
		return 1;
}
