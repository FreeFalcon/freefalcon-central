#include <stdio.h>
#include <conio.h>
#include <stddef.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <math.h>
#include "CmpGlobl.h"
#include "F4Vu.h"
#include "ListADT.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "ASearch.h"
#include "Path.h"
#include "Find.h"
#include "APITypes.h"
#include "Campaign.h"
#include "ATM.h"
#include "CampList.h"
#include "campwp.h"
#include "update.h"
#include "loadout.h"
#include "campweap.h"
#include "airunit.h"
#include "tactics.h"
#include "Team.h"
#include "Feature.h"
#include "MsgInc\UnitMsg.h"
#include "MsgInc\FalconFlightPlanMsg.h"
#include "Pilot.h"
#include "AIInput.h"
#include "CmpClass.h"
#include "classtbl.h"
#include "weaplist.h"
#include "falcsess.h"
#include "Supply.h"
#include "Dispcfg.h"
#include "FalcUser.h"
#include "teamdata.h"
#include "misseval.h"

#include "debuggr.h"
extern int g_npercentage_available_aircraft; //me123
extern int g_nminimum_available_aircraft; //me123
// ============================================
// Externals
// ============================================

extern unsigned char			SHOWSTATS;
extern int				gCampDataVersion;
extern char	MissStr[AMIS_OTHER][16];

extern int doUI;
extern int RegroupFlight (Flight flight);
extern void UI_Refresh (void);

extern VU_ID_NUMBER vuAssignmentId;
extern VU_ID_NUMBER vuLowWrapNumber;
extern VU_ID_NUMBER vuHighWrapNumber;
extern VU_ID_NUMBER lastNonVolitileId;
extern VU_ID_NUMBER lastLowVolitileId;
extern VU_ID_NUMBER lastVolitileId;

extern VU_ID gCurrentFlightID;	// Current Mission Flight (Mission Window) Also sets gSelectedFlight

extern int g_nRelocationWait; // JB 010728
extern int g_nAirbaseReloc; // 2001-08-31 S.G.
#define AirBaseRelocTeamOnly 1 // 2001-08-31 S.G.
#define AirBaseRelocNoFar 2 // 2001-08-31 S.G.
extern bool g_bHelosReloc;	// 2002-01-01 A.S.

extern FILE
	*save_log,
	*load_log;

extern int
	start_save_stream,
	start_load_stream;

#ifdef CAMPTOOL
// Renaming tool stuff
extern VU_ID_NUMBER RenameTable[65536];
extern int gRenameIds;
#endif

#ifdef DEBUG
extern int gCheckConstructFunction;
#endif

// -------------------------
// Local Function Prototypes
// =========================

float GetRange (Unit u, int type, int id);
void AbortFlight (Unit flight);
int PilotAvailable (Squadron sq, int pn);
void SetPilotStatus (Squadron sq, int pn);
void UnsetPilotStatus (Squadron sq, int pn);

// ==================================
// Some module globals - to save time
// ==================================

extern int haveWeaps;
extern int haveFuel;
extern int ourRange;
extern int theirDomain;

// =================================
// Smart heap pool stuff
// =================================

#ifdef USE_SH_POOLS
MEM_POOL	SquadronClass::pool;
#endif

extern bool g_bEnableABRelocation;	// OW AB Relocation fix
extern bool g_bScramble; //TJL 11/02/03 Enable Scramble Missions


// ============================================
// SquadronClass Functions
// ============================================

// KCK: ALL SQUADRON CONSTRUCTION SHOULD USE THIS FUNCTION!
SquadronClass* NewSquadron (int type)
	{
	SquadronClass	*new_squadron;
#ifdef DEBUG
	gCheckConstructFunction = 1;
#endif
	VuEnterCriticalSection();
	lastVolitileId = vuAssignmentId;
	vuAssignmentId = lastNonVolitileId;
	vuLowWrapNumber = FIRST_NON_VOLITILE_VU_ID_NUMBER;
	vuHighWrapNumber = LAST_NON_VOLITILE_VU_ID_NUMBER;
	new_squadron = new SquadronClass (type);
	lastNonVolitileId = vuAssignmentId;
	vuAssignmentId = lastVolitileId;
	vuLowWrapNumber = FIRST_VOLITILE_VU_ID_NUMBER;
	vuHighWrapNumber = LAST_VOLITILE_VU_ID_NUMBER;
	VuExitCriticalSection();
#ifdef DEBUG
	gCheckConstructFunction = 0;
#endif
	return new_squadron;
	}

SquadronClass::SquadronClass(int type) : AirUnitClass(type)
	{
	specialty = 0;
	UnitClassDataType *uc;
	uc = (UnitClassDataType*) Falcon4ClassTable[type-VU_LAST_ENTITY_TYPE].dataPtr;
	if (uc)
		memcpy(stores,SquadronStoresDataTable[uc->SpecialIndex].Stores,MAXIMUM_WEAPTYPES);
	else
		memset(stores,0,MAXIMUM_WEAPTYPES);
	fuel = uc->Fuel * 24 * SQUADRON_MISSIONS_PER_HOUR*MIN_RESUPPLY*2/60;
	memset(schedule,0,sizeof(long)*VEHICLES_PER_UNIT);
	airbase_id = FalconNullId;
	hot_spot = FalconNullId;
	assigned = 0;
	memset(rating,0,ARO_OTHER);
	aa_kills = ag_kills = as_kills = an_kills = 0;
	missions_flown = mission_score = 0;
	total_losses = 0;
	pilot_losses = 0;
	dirty_squadron = 0;
	squadron_patch = 0;
	last_resupply_time = 0;	
	last_resupply = 0;		
	InitPilots();
	SetParent(1);
// 2001-07-05 MODIFIED BY S.G. NEW VARIABLE TO ZERO
	squadronRetaskAt = 0;
	}

SquadronClass::SquadronClass(VU_BYTE **stream) : AirUnitClass(stream)
	{
	if (load_log)
	{
		fprintf (load_log, "%08x SquadronClass ", *stream - start_load_stream);
		fflush (load_log);
	}

	memcpy(&fuel, *stream, sizeof(long));							*stream += sizeof(long); 
	memcpy(&specialty, *stream, sizeof(uchar));						*stream += sizeof(uchar); 
	
	if (gCampDataVersion < 69)
	{
	    memset(stores,0,MAXIMUM_WEAPTYPES);
	    memcpy(stores, *stream, sizeof(uchar)*200);					*stream += sizeof(uchar)*200; 
	}
	else if (gCampDataVersion < 72)
	{
	    memset(stores,0,MAXIMUM_WEAPTYPES);
	    memcpy (stores, *stream, sizeof(uchar)*220);			*stream += sizeof(uchar)*220; 
	}
	else
	{
	    memcpy(stores, *stream, sizeof(uchar)*MAXIMUM_WEAPTYPES);	*stream += sizeof(uchar)*MAXIMUM_WEAPTYPES; 
	}
	 
	if (gCampDataVersion < 47)
		{
		if (gCampDataVersion >= 29)
			{
			memcpy(pilot_data, *stream, 8*PILOTS_PER_SQUADRON);		*stream += 8*PILOTS_PER_SQUADRON; 
			// Reinit them
			InitPilots();
			}
		else
			{
			memcpy(pilot_data, *stream, 8*36);						*stream += 8*36;
			// Reinit them
			InitPilots();
			}
		}
	else
		{
		memcpy(pilot_data, *stream, sizeof(PilotClass)*PILOTS_PER_SQUADRON);	*stream += sizeof(PilotClass)*PILOTS_PER_SQUADRON; 
		if (gCampDataVersion < 55)
			InitPilots();
		}
	if (pilot_data[1].pilot_id < 0)
		InitPilots();
	memcpy(schedule, *stream, sizeof(long)*VEHICLES_PER_UNIT);		*stream += sizeof(long)*VEHICLES_PER_UNIT; 
	memcpy(&airbase_id, *stream, sizeof(VU_ID));					*stream += sizeof(VU_ID); 
	memcpy(&hot_spot, *stream, sizeof(VU_ID));						*stream += sizeof(VU_ID); 
#ifdef DEBUG
//	airbase_id.num_ &= 0x0000ffff;
//	hot_spot.num_ &= 0x0000ffff;
#endif
	if (gCampDataVersion >= 6 && gCampDataVersion < 16)
		{
		VU_ID		junk;
		memcpy(&junk, *stream, sizeof(VU_ID));					*stream += sizeof(VU_ID); 
		}
	memcpy(rating, *stream, sizeof(uchar)*ARO_OTHER);				*stream += sizeof(uchar)*ARO_OTHER; 
	memcpy(&aa_kills, *stream, sizeof(short));						*stream += sizeof(short); 
	memcpy(&ag_kills, *stream, sizeof(short));						*stream += sizeof(short); 
	memcpy(&as_kills, *stream, sizeof(short));						*stream += sizeof(short); 
	memcpy(&an_kills, *stream, sizeof(short));						*stream += sizeof(short); 
	memcpy(&missions_flown, *stream, sizeof(short));				*stream += sizeof(short); 
	memcpy(&mission_score, *stream, sizeof(short));					*stream += sizeof(short); 
	memcpy(&total_losses, *stream, sizeof(uchar));					*stream += sizeof(uchar);
	if (gCampDataVersion >= 9)
		{
		memcpy(&pilot_losses, *stream, sizeof(uchar));				*stream += sizeof(uchar);
		}
	else
		pilot_losses=0;
	if (gCampDataVersion < 41)
		BuildElements();
	if (gCampDataVersion < 45)
		squadron_patch = AssignUISquadronID(GetUnitNameID());
	else
		{
		memcpy(&squadron_patch, *stream, sizeof(uchar));			*stream += sizeof(uchar);
		if (gCampDataVersion < 50)
			squadron_patch = AssignUISquadronID(GetUnitNameID());
		}
	last_resupply_time = 0;	
	last_resupply = 0;		
	dirty_squadron = 0;
// 2001-07-05 MODIFIED BY S.G. NEW VARIABLE TO ZERO
	squadronRetaskAt = 0;
	}

SquadronClass::~SquadronClass (void)
	{
	if (IsAwake())
		Sleep();
	}

int SquadronClass::SaveSize (void)
	{
	return AirUnitClass::SaveSize()
		+ sizeof(long)
		+ sizeof(uchar) 
		+ sizeof(uchar)*MAXIMUM_WEAPTYPES
		+ sizeof(PilotClass)*PILOTS_PER_SQUADRON
		+ sizeof(long)*VEHICLES_PER_UNIT
		+ sizeof(VU_ID)		
		+ sizeof(VU_ID)
		+ sizeof(uchar)*ARO_OTHER
		+ sizeof(short)
		+ sizeof(short)	
		+ sizeof(short)	
		+ sizeof(short)	
		+ sizeof(short)	
		+ sizeof(short)	
		+ sizeof(uchar)
		+ sizeof(uchar)
		+ sizeof(uchar);
	}

int SquadronClass::Save (VU_BYTE **stream)
	{
	AirUnitClass::Save(stream);
	if (save_log)
	{
		fprintf (save_log, "%08x SquadronClass ", *stream - start_save_stream);
		fflush (save_log);
	}
	memcpy(*stream, &fuel, sizeof(long));							*stream += sizeof(long); 
	memcpy(*stream, &specialty, sizeof(uchar));						*stream += sizeof(uchar); 
	memcpy(*stream, stores, sizeof(uchar)*MAXIMUM_WEAPTYPES);		*stream += sizeof(uchar)*MAXIMUM_WEAPTYPES; 
	memcpy(*stream, pilot_data, sizeof(PilotClass)*PILOTS_PER_SQUADRON);	*stream += sizeof(PilotClass)*PILOTS_PER_SQUADRON; 
	memcpy(*stream, schedule, sizeof(long)*VEHICLES_PER_UNIT);		*stream += sizeof(long)*VEHICLES_PER_UNIT; 
#ifdef CAMPTOOL
	if (gRenameIds)
		airbase_id.num_ = RenameTable[airbase_id.num_];
#endif
	memcpy(*stream, &airbase_id, sizeof(VU_ID));					*stream += sizeof(VU_ID); 
#ifdef CAMPTOOL
	if (gRenameIds)
		hot_spot.num_ = RenameTable[hot_spot.num_];
#endif
	memcpy(*stream, &hot_spot, sizeof(VU_ID));						*stream += sizeof(VU_ID); 
	memcpy(*stream, rating, sizeof(uchar)*ARO_OTHER);				*stream += sizeof(uchar)*ARO_OTHER; 
	memcpy(*stream, &aa_kills, sizeof(short));						*stream += sizeof(short); 
	memcpy(*stream, &ag_kills, sizeof(short));						*stream += sizeof(short); 
	memcpy(*stream, &as_kills, sizeof(short));						*stream += sizeof(short); 
	memcpy(*stream, &an_kills, sizeof(short));						*stream += sizeof(short); 
	memcpy(*stream, &missions_flown, sizeof(short));				*stream += sizeof(short); 
	memcpy(*stream, &mission_score, sizeof(short));					*stream += sizeof(short); 
	memcpy(*stream, &total_losses, sizeof(uchar));					*stream += sizeof(uchar); 
	memcpy(*stream, &pilot_losses, sizeof(uchar));					*stream += sizeof(uchar); 
	memcpy(*stream, &squadron_patch, sizeof(uchar));				*stream += sizeof(uchar);
	return SquadronClass::SaveSize();
	}

// event handlers
int SquadronClass::Handle(VuFullUpdateEvent *event)
	{
	// copy data from temp entity to current entity
	SquadronClass* tmp_ent = (SquadronClass*)(event->expandedData_);

	fuel = tmp_ent->fuel;
	memcpy(stores, tmp_ent->stores, sizeof(uchar)*MAXIMUM_WEAPTYPES);
	memcpy(pilot_data, tmp_ent->pilot_data, sizeof(PilotClass)*PILOTS_PER_SQUADRON);
	memcpy(schedule, tmp_ent->schedule, sizeof(long)*VEHICLES_PER_UNIT);
	airbase_id = tmp_ent->airbase_id;
	hot_spot = tmp_ent->hot_spot;
	aa_kills = tmp_ent->aa_kills;
	ag_kills = tmp_ent->ag_kills;
	as_kills = tmp_ent->as_kills;
	an_kills = tmp_ent->an_kills;
	missions_flown = tmp_ent->missions_flown;
	mission_score = tmp_ent->mission_score;
	total_losses = tmp_ent->total_losses;
	pilot_losses = tmp_ent->pilot_losses;
	squadron_patch = tmp_ent->squadron_patch;
	return (AirUnitClass::Handle(event));
	}

int SquadronClass::MoveUnit (CampaignTime time)
	{
	GridIndex       x,y,nx,ny;
	VuGridIterator*	myit = NULL;
	Objective		o,bo=NULL;
	float			fd;
	int				range,score,i,want_alert=0,bs=-999;
	CampEntity		ab;
	
/*  Don't recall squadrons - per Gilman
	if (GetTotalVehicles() < GetFullstrengthVehicles() / 4)
		{
		if (this == FalconLocalSession->GetPlayerSquadron())
			PostMessage(FalconDisplay.appWin,FM_SQUADRON_RECALLED,0,0);
		KillUnit();
		}
*/


//TJL 11/02/03 Enable Scramble missions

	if (g_bScramble)
	{
	// Set up an alert bird for this squadron
	if (rating[ARO_CA] > 4)
		{
		// KCK: Check if we have available aircraft 
		// NOTE: We might want to make sure we always ask for at least one
		// alert flight.
		for (i=0; i<VEHICLES_PER_UNIT/2; i++)
			{
			if (!schedule[i])
				want_alert = 1;
			}

		if (want_alert)
			{
//#ifdef DEBUG
			MonoPrint("Requesting alert bird for squadron #%d.\n",GetCampID());
//#endif
			MissionRequestClass	mis;
			// JB 010728 Make the wait time configurable
			// MN 020102 This is not the relocation timer - check above
//			mis.tot = Camp_GetCurrentTime() + g_nRelocationWait * CampaignHours;			// hang around for a few hours
			mis.tot = Camp_GetCurrentTime() + 3 * CampaignHours;
			mis.requesterID = Id();
			mis.who = GetTeam();
			mis.vs = GetEnemyTeam(mis.who);
			mis.tot_type = TYPE_NE;
			GetLocation(&mis.tx,&mis.ty);
			mis.targetID = FalconNullId;
			mis.mission = AMIS_ALERT;
			mis.roe_check = ROE_AIR_ENGAGE;
			mis.flags = REQF_ONETRY	| REQF_USE_REQ_SQUAD | REQF_USERESERVES;
			mis.priority = 255;											// High priority
			mis.RequestMission();
			}
		}
	}
	else
	{
	}

//TJL 11/02/03 End Scramble


	// OW AB Relocation fix
	if(g_bEnableABRelocation)
	{
		if(SimLibElapsedTime < 32450000.0f)
			return 0; //me123 dont relocate before the campaign has begun
	}

	ab = FindEntity(airbase_id);

// A.S. begin
	CampEntity		ab_old;  // A.S. new variable
	if (g_bHelosReloc)
		{
		ab_old = ab;  
		}
// A.S. end



	ShiAssert (!ab || ab->IsObjective() || ab->IsTaskForce() || (ab == this && DontPlan()));

	if (!ab || ab->IsObjective() || ab == this)
		{
		// Don't plan flag used to mean don't rebase for squadrons
		if (DontPlan())
			{
// 2001-08-06 MODIFIED BY S.G. FRIENDLY BASE WILL DO THE JOB ALL RIGHT. NO NEED TO LIMIT IT TO OUR TEAM.
//			if (ab->GetTeam() != GetTeam())
			if (!ab || !GetRoE(ab->GetTeam(), GetTeam(), ROE_AIR_USE_BASES))
				{
				if (this == FalconLocalSession->GetPlayerSquadron())
					PostMessage(FalconDisplay.appWin,FM_SQUADRON_RECALLED,0,0);
				KillUnit();
				}
			return 0;
			}

		// If airbase is non-functional, force a rebase
// 2001-08-03 MODIFIED BY S.G. ONLY IF CAPTURED SHOULD IT RELOCATE. DESTROYED AIRBASE STILL OWN BY US WILL REPAIR EVENTUALLY.
//		if (ab && ab->IsObjective() && ((Objective)ab)->GetAdjustedDataRate() < 1)
		if (ab && ab->IsObjective() && !GetRoE(ab->GetTeam(), GetTeam(), ROE_AIR_USE_BASES))
			ab = NULL;
// Added by A.S. 1.1.2002.  Helos will be reallocated if armybase is destoyed. 
		if (g_bHelosReloc) 
			{
			if (ab && ab->IsObjective() && IsHelicopter() && ((Objective)ab)->GetAdjustedDataRate() < 1) 
				{  
				ab = NULL;																
			//	FILE *deb;
			//	deb = fopen("c:\\temp\\realloc.txt", "a");
			//	fprintf(deb, "ArmyBase  ID = %d  team = %x  type = %x  TIME = %d\n",  ab_old, ab_old->GetTeam, ab_old->GetType, TheCampaign.CurrentTime/(3600*1000));
			//	fclose(deb);
				}
			}
// A.S. end
		// Check airbase location - if to near or far from front, relocate
		GetLocation(&x,&y);
		fd = DistanceToFront(x,y);
		range = GetUnitRange();
// 2001-07-05 MODIFIED BY S.G. DON'T RELOCATE IF TOO FAR FROM FLOT IF GLOBALLY SET TO ACT THAT WAY
//		if (fd < 999.0F && (fd < range/30 || fd > range/3 || !ab))		// We're to close or to far from the front or don't have an airbase
		if (fd < 999.0F && (fd < range/30 || (!(g_nAirbaseReloc & AirBaseRelocNoFar) && fd > range/3) || !ab))		// We're to close or to far from the front or don't have an airbase
			{
			// Find a better base for us
			UnitClassDataType	*uc = GetUnitClassData();
			ATMAirbaseClass		*atmbase;
			Team				us = GetTeam();
			CAMPREGLIST_ITERATOR		myit(AllObjList);
			o = (Objective) myit.GetFirst();
			while (o)
				{
// 2001-07-05 MODIFIED BY S.G. ONLY USE YOUR OWN AIRBASE IF GLOBALLY SET TO ACT THAT WAY
//				if ((o->GetType() == TYPE_AIRBASE && !IsHelicopter() && GetRoE(o->GetTeam(),us,ROE_AIR_USE_BASES)) ||
//					(o->GetType() == TYPE_ARMYBASE && IsHelicopter() && GetRoE(o->GetTeam(),us,ROE_AIR_USE_BASES)))
				int enter = FALSE;
				if (g_nAirbaseReloc & AirBaseRelocTeamOnly) {
					if ((o->GetType() == TYPE_AIRBASE && !IsHelicopter() && o->GetTeam() == us) ||
						(o->GetType() == TYPE_ARMYBASE && IsHelicopter() && o->GetTeam() == us))
						enter = TRUE;
				}
				else {
					if ((o->GetType() == TYPE_AIRBASE && !IsHelicopter() && GetRoE(o->GetTeam(),us,ROE_AIR_USE_BASES)) ||
						(o->GetType() == TYPE_ARMYBASE && IsHelicopter() && GetRoE(o->GetTeam(),us,ROE_AIR_USE_BASES)))
						enter = TRUE;
				}
				if (enter)
// END OF MODIFIED SECTION
					{
					o->GetLocation(&nx,&ny);
					fd = DistanceToFront(nx,ny);
					if (fd > range/15 && o->GetAdjustedDataRate() > 0)
						{
						score = o->GetObjectiveStatus()*5 - FloatToInt32(fd);
						// Adjust by number of squadrons already based here.
						atmbase = TeamInfo[us]->atm->FindATMAirbase (o->Id());
						if (atmbase && atmbase->usage)
						// JB 010328 from Mad__Max
							//score /= atmbase->usage;
						{
							if (o != ab)  score /= (atmbase->usage+1);
							if (o == ab) score /= atmbase->usage;
						}
						// JB 010328 from Mad__Max

						if (score > bs)
							{
							bo = o;
							bs = score;
							}
						}
					}
				o = (Objective) myit.GetNext();
				}
			if (bo)
				{
				if (bo != ab)
					{
					bo->GetLocation(&nx,&ny);
					SetLocation(nx,ny);
					SetUnitAirbase(bo->Id());
					TeamInfo[us]->atm->AddToAirbaseList(bo);
					if (this == FalconLocalSession->GetPlayerSquadron())
						PostMessage(FalconDisplay.appWin,FM_SQUADRON_REBASED,0,0);
// 2001-07-05 MODIFIED BY S.G. RETASK IN ONE DAY ONLY
// 020102 M.N. Variable relocate time
					squadronRetaskAt = Camp_GetCurrentTime() + CampaignHours * g_nRelocationWait;
// A.S.  begin: retask time for Helos only 1 hour 
					if (g_bHelosReloc) 
						{
						if (ab_old && ab_old->IsObjective() && ((Objective)ab_old)->GetAdjustedDataRate() < 1)  
							{
							if ( IsHelicopter() ) 
								{
								squadronRetaskAt = Camp_GetCurrentTime() + CampaignHours * 1;
							//	FILE *deb;
							//	deb = fopen("c:\\temp\\realloc.txt", "a");
							//	fprintf(deb, "====> squadronRetaskAt ID = %d  ID_neu %d  team = %x  type = %x  TIME = %d\n\n", ab_old, bo, ab_old->GetTeam, ab_old->GetType, TheCampaign.CurrentTime/(3600*1000));
							//	fclose(deb);
								}
							}
						}
// A.S. end +++++++++++++++++
					}
				}
			else
				{
				// We're lost
				if (this == FalconLocalSession->GetPlayerSquadron())
					PostMessage(FalconDisplay.appWin,FM_SQUADRON_RECALLED,0,0);
				KillUnit();
				return 0;
				}
			}
		}
	

	// Set up an alert bird for this squadron
	//TJL 10/31/03 Move This
/*	if (rating[ARO_CA] > 5)
		{
		// KCK: Check if we have available aircraft 
		// NOTE: We might want to make sure we always ask for at least one
		// alert flight.
		for (i=0; i<VEHICLES_PER_UNIT/2; i++)
			{
			if (!schedule[i])
				want_alert = 1;
			}

		if (want_alert)
			{
#ifdef DEBUG
//			MonoPrint("Requesting alert bird for squadron #%d.\n",GetCampID());
#endif
			MissionRequestClass	mis;
			// JB 010728 Make the wait time configurable
			// MN 020102 This is not the relocation timer - check above
//			mis.tot = Camp_GetCurrentTime() + g_nRelocationWait * CampaignHours;			// hang around for a few hours
			mis.tot = Camp_GetCurrentTime() + 3 * CampaignHours;
			mis.requesterID = Id();
			mis.who = GetTeam();
			mis.vs = GetEnemyTeam(mis.who);
			mis.tot_type = TYPE_NE;
			GetLocation(&mis.tx,&mis.ty);
			mis.targetID = FalconNullId;
			mis.mission = AMIS_ALERT;
			mis.roe_check = ROE_AIR_ENGAGE;
			mis.flags = REQF_ONETRY	| REQF_USE_REQ_SQUAD | REQF_USERESERVES;
			mis.priority = 255;											// High priority
			mis.RequestMission();
			}
		} */

	return 0;
	}
 


int SquadronClass::GetUnitSupplyNeed (int have)
	{

	if(g_bEnableABRelocation)
	{
		// OW AB Relocation fix
		MoveUnit(0);//me123 relocate test
	}

	int		want=0,got=0,i;
	UnitClassDataType*	uc;

	// Squadrons need supply based on their munitions
	uc = GetUnitClassData();
	if (!uc)
		return 0;
	for (i=0; i<MAXIMUM_WEAPTYPES; i++)
		{
		want += SquadronStoresDataTable[uc->SpecialIndex].Stores[i];
		got += GetUnitStores(i);
		}
	if (have)
		return got / SQUADRON_PT_SUPPLY;
	return (want - got) / SQUADRON_PT_SUPPLY;
	}

int SquadronClass::GetUnitFuelNeed (int have)
	{
	int		need=0;
	UnitClassDataType*	uc;

	uc = GetUnitClassData();
	if (!uc)
		return 0;
	// Squadrons want enough fuel to load each plane SQUADRON_MISSIONS_PER_HOUR times per hour for 2 supply periods
	if (have)
		return GetSquadronFuel()/SUPPLY_PT_FUEL;
	else
		{
		need = (uc->Fuel * GetTotalVehicles() * SQUADRON_MISSIONS_PER_HOUR*2*MIN_RESUPPLY)/60;
		if (need < fuel)
			return 0;		// KCK: We've lost so many aircraft, we've now got spare fuel.
		return (need - GetSquadronFuel())/SUPPLY_PT_FUEL;
		}
	}

void SquadronClass::SupplyUnit (int supply, int fuel)
	{
	int		need,i;
	float	ratio;

	UseFuel(-1*fuel*SUPPLY_PT_FUEL);

	// Now let's add our munititions
	need = GetUnitSupplyNeed(FALSE);
	if (need > 0)
		ratio = (float)supply/(float)need;
	else
		ratio = 1.0F;
	for (i=0; i<MAXIMUM_WEAPTYPES; i++)
		{
		if (SquadronStoresDataTable[class_data->SpecialIndex].Stores[i])
			{
			need = SquadronStoresDataTable[class_data->SpecialIndex].Stores[i] - GetUnitStores(i);
			SetUnitStores(i,GetUnitStores(i) + FloatToInt32(need*ratio));
			}
		}
	}

int SquadronClass::NumActivePilots (void)
	{
	int		i,num=0;

	for (i=0; i<PILOTS_PER_SQUADRON; i++)
		{
		if (GetPilotData(i)->pilot_status == PILOT_AVAILABLE || GetPilotData(i)->pilot_status == PILOT_IN_USE)
			num++;
		}
	return num;
	}

void SquadronClass::InitPilots (void)
	{
	int		i,last_commander;

	// Start with full load of pilots
	last_commander = PILOTS_PER_SQUADRON/3;
	for (i=0; i<PILOTS_PER_SQUADRON; i++)
		{
// 2000-11-17 MODIFIED BY S.G. NEED TO PASS THE 'airExperience' OF THE TEAM SO I CAN USE IT AS A BASE
//		GetPilotData(i)->ResetStats();
		GetPilotData(i)->ResetStats(TeamInfo[GetOwner()]->airExperience);
		if (!i)								// First slot is Colonel.
			GetPilotData(i)->pilot_id = GetAvailablePilot(TeamInfo[GetOwner()]->firstColonel,TeamInfo[GetOwner()]->firstCommander, GetOwner());
		else if (i < last_commander)		// First 1/3 are commanders
			GetPilotData(i)->pilot_id = GetAvailablePilot(TeamInfo[GetOwner()]->firstCommander,TeamInfo[GetOwner()]->firstWingman, GetOwner());
		else								// otherwise normal wingmen
			GetPilotData(i)->pilot_id = GetAvailablePilot(TeamInfo[GetOwner()]->firstWingman,TeamInfo[GetOwner()]->lastWingman, GetOwner());
		}
	// Special code to select certain squadron leaders.
	if (GetNameId() == 36)
		{
		pilot_data[0].pilot_id = 1;
		pilot_data[0].pilot_skill_and_rating = 0x44;
		}
	else if (GetNameId() == 80)
		pilot_data[0].pilot_id = 2;
	else if (GetNameId() == 35)
		pilot_data[0].pilot_id = 3;
	}

void SquadronClass::ReinforcePilots (int max_new_pilots)
	{
	int		i,added=0,result;

	for (i=0; i<PILOTS_PER_SQUADRON; i++)
		{
		if (pilot_data[i].pilot_id == 1 && (pilot_data[i].pilot_status == PILOT_MIA || pilot_data[i].pilot_status == PILOT_KIA))
			pilot_data[i].pilot_status = PILOT_AVAILABLE;
		if (GetPilotData(i)->pilot_status == PILOT_RESCUED)
			{
			GetPilotData(i)->pilot_status = PILOT_AVAILABLE;
			}
		else if (GetPilotData(i)->pilot_status == PILOT_MIA)
			{
			result = rand()%3;
			if (!result)				// 33% chance of a rescue
				GetPilotData(i)->pilot_status = PILOT_RESCUED;
			else if (result == 1)		// 33% chance of KIA
				{
				GetPilotData(i)->pilot_status = PILOT_KIA;
				SetPilotLosses(pilot_losses + 1);
				}
			}
		else if (GetPilotData(i)->pilot_status == PILOT_KIA && added < max_new_pilots)
			{
// 2000-11-17 MODIFIED BY S.G. NEED TO PASS THE 'airExperience' OF THE TEAM SO I CAN USE IT AS A BASE
//			GetPilotData(i)->ResetStats();
			GetPilotData(i)->ResetStats(TeamInfo[GetOwner()]->airExperience);
			if (!i)									// First slot is Colonel.
				GetPilotData(i)->pilot_id = GetAvailablePilot(TeamInfo[GetOwner()]->firstColonel,TeamInfo[GetOwner()]->firstCommander, GetOwner());
			else if (i < PILOTS_PER_SQUADRON/3)		// First 1/3 are commanders
				GetPilotData(i)->pilot_id = GetAvailablePilot(TeamInfo[GetOwner()]->firstCommander,TeamInfo[GetOwner()]->firstWingman, GetOwner());
			else									// otherwise normal wingmen
				GetPilotData(i)->pilot_id = GetAvailablePilot(TeamInfo[GetOwner()]->firstWingman,TeamInfo[GetOwner()]->lastWingman, GetOwner());
			}
		}
	}

void SquadronClass::ScoreKill (int pilot, int killtype)
	{
	switch (killtype)
		{
		case ASTAT_AAKILL:
		case ASTAT_PKILL:
			pilot_data[pilot].aa_kills++;
			aa_kills++;
			break;
		case ASTAT_AGKILL:
			pilot_data[pilot].ag_kills++;
			ag_kills++;
			break;
		case ASTAT_ASKILL:
			pilot_data[pilot].as_kills++;
			as_kills++;
			break;
		case ASTAT_ANKILL:
			pilot_data[pilot].an_kills++;
			an_kills++;
			break;
		default:
			return;
			break;
		}
	}

void SquadronClass::DisposeChildren (void)
	{
	// Clears out all flights waiting for takeoff.
	// Intended for use when the squadron is recalled.
	CAMPREGLIST_ITERATOR	ait(AllAirList);
	Unit			u;

	u = (Unit) ait.GetFirst();
	while (u)
		{
		if (u->IsFlight() && !u->Moving() && ((Flight)u)->GetUnitSquadronID() == Id())
			RegroupFlight((Flight)u);
		u = (Unit) ait.GetNext();
		}
	}

uchar SquadronClass::GetAvailableStores (int i) 
	{ 
	int		have, max;

	// Check for infinate stuff
	if (i == SquadronStoresDataTable[class_data->SpecialIndex].infiniteAA ||
		i == SquadronStoresDataTable[class_data->SpecialIndex].infiniteAG ||
		i == SquadronStoresDataTable[class_data->SpecialIndex].infiniteGun)
		return 4;

	have = GetUnitStores(i);
	max = SquadronStoresDataTable[class_data->SpecialIndex].Stores[i];

	ShiAssert(max);
	if(max)
		return (have * 4) / max;
	else
		return have;
	}

void SquadronClass::ShiftSchedule (void)
{
	int		i;
	
	for (i=0; i<VEHICLES_PER_UNIT; i++)
	{
		if (!GetNumVehicles(i))
			SetSchedule(i, 0xFFFFFFFF);			// Nothing here, set as used.
		else
			ShiftSchedule(i);					// [i] = sq->schedule[i] >> 1;
	}
}

// Find up to num aircraft which are free from cycle sb to fb. Returns # actually found
int SquadronClass::FindAvailableAircraft (MissionRequest mis)
	{
	int		cb,i,got=0,snum=0,tav,ls;
	uchar	free[VEHICLES_PER_UNIT] = {0};

	if (mis->start_block >= ATM_MAX_CYCLES)
		return 0;
	if (mis->flags & REQF_USERESERVES)
		tav = GetTotalVehicles();							// Use any available aircraft
	else if (FloatToInt32(GetTotalVehicles() * g_npercentage_available_aircraft/100) > g_nminimum_available_aircraft)
		tav = FloatToInt32(GetTotalVehicles() * g_npercentage_available_aircraft/100);		// Save 1/4 as reserve
	else tav = g_nminimum_available_aircraft; // JPO fixup.
	// Mark our last allowed slot (so we don't assign reserve aircraft
	for (i=0;i<VEHICLES_PER_UNIT && tav>0; i++)
		tav -= GetNumVehicles(i);
	ls = i;
	memset(free,1,ls);

	// find all free slots
	for (cb=mis->start_block; cb<=mis->final_block; cb++)
		{
		for (i=0;i<ls; i++)
			{
			if (GetSchedule(i) & (1 << cb))
				free[i] = 0;
			}
		}

	// now collect the rest of our aircraft (regardless of match)
	for (i=0; i<ls && got < mis->aircraft; i++)
		{
		if (free[i] && GetNumVehicles(i))
			{
			mis->slots[snum] = i;
			got += GetNumVehicles(i);
			snum++;
			}
		}

#ifdef DEBUG
	for (i=0; i<PILOTS_PER_FLIGHT; i++)
		{
		for (ls=0; ls<PILOTS_PER_FLIGHT; ls++)
			{
			ShiAssert (i == ls || mis->slots[i] == 255 || mis->slots[ls] != mis->slots[i]);
			}
		}
	ShiAssert (!got || mis->slots[0] != 255);
#endif

	return got;
	}

// Sets schedule usage for the passed period and slot numbers
void SquadronClass::ScheduleAircraft (Flight fl, MissionRequest mis)

	{
	int		i,j,sn,nv,nr,role,got=0;
	//TJL 10/30/03
	int want_alert = 0;
	VehicleClassDataType *vc;

	role = MissionData[mis->mission].skill;
	if (MissionData[mis->mission].flags & AMIS_DONT_USE_AC)
		{
		// Just fill up our slots sequentially
		for (i=0; i<VEHICLES_PER_UNIT; i++)
			{
			if (mis->aircraft < 12)
				nv = 2;
			else
				nv = 3;
			if (nv + got > mis->aircraft)
				nv = mis->aircraft-got;
			fl->SetNumVehicles(i,nv);
			got += nv;
			}
		memset(fl->slots,255,PILOTS_PER_FLIGHT);
		// Fake got for pilot assignments
		if (got > 4)
			got = 4;
		}
	else
		{
		// schedule the aircraft
		for (sn=0; sn<PILOTS_PER_FLIGHT; sn++)
			{
			if (mis->slots[sn] < VEHICLES_PER_UNIT)
				{
				// KCK: Add turn-around time to final block to determine when
				// aircraft will be available next
				int	finalBlock = mis->final_block + (AIRCRAFT_TURNAROUND_TIME_MINUTES / MIN_PLAN_AIR);
				if (finalBlock >= ATM_MAX_CYCLES)
					finalBlock = ATM_MAX_CYCLES;
				for (j=mis->start_block; j<=mis->final_block; j++)
					SetSchedule(mis->slots[sn], (1 << j));
				}
			}

		fl->SetRoster(0);
		for (i=0; i<PILOTS_PER_FLIGHT; i++)
			{
			if (mis->slots[i] < VEHICLES_PER_UNIT)
				{
				nv = GetNumVehicles(mis->slots[i]);
				if (nv+got > mis->aircraft)
					nv = mis->aircraft-got;
				got += nv;
				fl->slots[i] = mis->slots[i];
				// KCK NOTE: doing this is safe, since flight isn't inserted yet.
				fl->SetNumVehicles(fl->slots[i], nv);
				SetAssigned (GetAssigned() + nv);
				// Lower score for this role, to prevent repicks of same mission
				// KCK NOTE: this is local only - other machines will not get this data
				nr = FloatToInt32(0.75F * GetRating(role)) + 1;
				if (nr < 1)
					nr = 1;
				SetRating(role, nr);
				}
			}
		}

	// Set aircraft availablity bits
	for (i=0; i<PILOTS_PER_FLIGHT; i++)
		{
		if (i<got)
			fl->plane_stats[i] = AIRCRAFT_AVAILABLE;
		else
			fl->plane_stats[i] = AIRCRAFT_NOT_ASSIGNED;
		fl->MakeFlightDirty (DIRTY_PLANE_STATS, DDP[120].priority);
//				fl->MakeFlightDirty (DIRTY_PLANE_STATS, SEND_RELIABLE);
		fl->pilots[i] = NO_PILOT;
		}
	fl->last_player_slot = PILOTS_PER_FLIGHT;

	// Large flights (i.e.: transport 'copters) only use 4 pilots
	if (got > PILOTS_PER_FLIGHT)
		got = PILOTS_PER_FLIGHT;

	// Find and fill an empty takeoff slot at this airbase
	fl->SetUnitAirbase(GetUnitAirbaseID());
	
	// Name this flight
	vc = GetVehicleClassData(fl->GetVehicleID(0));
	fl->callsign_id = vc->CallsignIndex;
	GetCallsignID(&fl->callsign_id,&fl->callsign_num,vc->CallsignSlots);
	if (fl->callsign_num)
		SetCallsignID(fl->callsign_id,fl->callsign_num);
	}


// Sets schedule usage for the passed period and slot numbers
int SquadronClass::AssignPilots (Flight fl)
	{
	int		plane,pilot,got;

	for (plane=0; plane<PILOTS_PER_FLIGHT; plane++)
		{
		got = FALSE;
		if (fl->plane_stats[plane] == AIRCRAFT_AVAILABLE && fl->pilots[plane] == NO_PILOT)
			{
			if (!plane)
				{
				// Commander goes in first slot
				for (pilot=0; pilot<PILOTS_PER_SQUADRON/3 && !got; pilot++)
					{
					if (GetPilotData(pilot)->pilot_status == PILOT_AVAILABLE)
						{
						fl->pilots[plane] = pilot;
						GetPilotData(pilot)->pilot_status = PILOT_IN_USE;
						got = TRUE;
						fl->MakeFlightDirty (DIRTY_PILOTS, DDP[121].priority);
						//	fl->MakeFlightDirty (DIRTY_PILOTS, SEND_RELIABLE);
						fl->MakeFlightDirty (DIRTY_PLANE_STATS, DDP[122].priority);
						//  fl->MakeFlightDirty (DIRTY_PLANE_STATS, SEND_RELIABLE);
						}
					}
				}
			if (!got)
				{
				// Now Wingmen
				for (pilot=PILOTS_PER_SQUADRON-1; pilot>=0 && !got; pilot--)
					{
					if (GetPilotData(pilot)->pilot_status == PILOT_AVAILABLE)
						{
						fl->pilots[plane] = pilot;
						GetPilotData(pilot)->pilot_status = PILOT_IN_USE;
						got = TRUE;
						fl->MakeFlightDirty (DIRTY_PILOTS, DDP[123].priority);
						//	fl->MakeFlightDirty (DIRTY_PILOTS, SEND_RELIABLE);
						fl->MakeFlightDirty (DIRTY_PLANE_STATS, DDP[124].priority);
						//	fl->MakeFlightDirty (DIRTY_PLANE_STATS, SEND_RELIABLE);
						}
					}
				}
			if (fl->pilots[plane] == NO_PILOT)
				return FALSE;
			}
		}
	fl->SetPilots(TRUE);
	if (doUI && FalconLocalSession->GetPlayerFlight() && (fl->Id() == gCurrentFlightID || fl->InPackage()))
		{
		TheCampaign.MissionEvaluator->PreMissionEval(FalconLocalSession->GetPlayerFlight(),FalconLocalSession->GetPilotSlot());
		UI_Refresh();
		}
	return TRUE;
	}

void SquadronClass::UpdateSquadronStores (short weapon[HARDPOINT_MAX], uchar weapons[HARDPOINT_MAX], int lbsfuel, int planes)
	{
	int			i,j,n,done=0;
	long		f;
	int			weaparray[HARDPOINT_MAX] = {0};
	int			weapsarray[HARDPOINT_MAX] = {0};

	// Consolidate the weapons (we need to do this to minimize rounding errors)
	for (i=0; i<HARDPOINT_MAX; i++)
		{
		if (!(WeaponDataTable[weapon[i]].Flags & WEAP_INFINITE_MASK))
			{
			for (j=0,done=0; j<HARDPOINT_MAX && !done; j++)
				{
				if (weapon[i] == weaparray[j] || !weaparray[j])
					{
					weaparray[j] = weapon[i];
					weapsarray[j] += weapons[i] * planes;
					done = 1;
					}
				}
			}
		}

	// Record the stuff
	for (i=0; i<HARDPOINT_MAX && weaparray[i]; i++)
		{	
		// One 'supply point' is worth four weapons (or 1, if we're loading less than 4)	
		if (weapsarray[i] > 0)
			n = GetUnitStores(weaparray[i]) - ((weapsarray[i]+3)/4);
		else
			n = GetUnitStores(weaparray[i]) - ((weapsarray[i]-3)/4);
		if (n < 0)
			n = 0;
		if (n > 255)
			n = 255;
		SetUnitStores(weaparray[i], n);
		}
	f = GetSquadronFuel() - ((lbsfuel * planes) / 100);
	if (f < 0)
		f = 0;
	SetSquadronFuel(f);

	if (!IsLocal())
		{
		// Send a message to host notifying him of the changes to the squadron's weapon loads
		VuSessionEntity				*target = (VuSessionEntity*) vuDatabase->Find(OwnerId());
		FalconFlightPlanMessage		*msg = new FalconFlightPlanMessage(Id(), target);
		uchar						*buffer;

		msg->dataBlock.type = FalconFlightPlanMessage::squadronStores;
		msg->dataBlock.size = HARDPOINT_MAX + HARDPOINT_MAX * sizeof(short) + sizeof(long)*2;
		msg->dataBlock.data = buffer = new uchar[msg->dataBlock.size];
		memcpy(buffer,weapon,HARDPOINT_MAX * sizeof(short));		buffer += HARDPOINT_MAX * sizeof(short);
		memcpy(buffer,weapons,HARDPOINT_MAX);		buffer += HARDPOINT_MAX;
		memcpy(buffer,&lbsfuel,sizeof(long));		buffer += sizeof(long);
		memcpy(buffer,&planes,sizeof(long));		buffer += sizeof(long);
		FalconSendMessage(msg, TRUE);		
		}
	}

// M.N. use this derived function to resupply the squad stores from aborted flights

void SquadronClass::ResupplySquadronStores (short weapon[HARDPOINT_MAX], uchar weapons[HARDPOINT_MAX], int lbsfuel, int planes)
	{
	int			i,j,n,done=0;
	long		f;
	int			weaparray[HARDPOINT_MAX] = {0};
	int			weapsarray[HARDPOINT_MAX] = {0};

	// Consolidate the weapons (we need to do this to minimize rounding errors)
	for (i=0; i<HARDPOINT_MAX; i++)
		{
		if (!(WeaponDataTable[weapon[i]].Flags & WEAP_INFINITE_MASK))
			{
			for (j=0,done=0; j<HARDPOINT_MAX && !done; j++)
				{
				if (weapon[i] == weaparray[j] || !weaparray[j])
					{
					weaparray[j] = weapon[i];
					weapsarray[j] += weapons[i] * planes;
					done = 1;
					}
				}
			}
		}

// M.N. add the weapsarray and fuel brought back to the airbase to the current unit stores again

	// Record the stuff
	for (i=0; i<HARDPOINT_MAX && weaparray[i]; i++)
		{	
		// One 'supply point' is worth four weapons (or 1, if we're loading less than 4)	
		if (weapsarray[i] > 0)
			n = GetUnitStores(weaparray[i]) + ((weapsarray[i]+3)/4);
		else
			n = GetUnitStores(weaparray[i]) + ((weapsarray[i]-3)/4);
		if (n < 0)
			n = 0;
		if (n > 255)
			n = 255;
		SetUnitStores(weaparray[i], n);
		}
	f = GetSquadronFuel() + ((lbsfuel * planes) / 100);
	if (f < 0)
		f = 0;
	SetSquadronFuel(f);

	if (!IsLocal())
		{
		// Send a message to host notifying him of the changes to the squadron's weapon loads
		VuSessionEntity				*target = (VuSessionEntity*) vuDatabase->Find(OwnerId());
		FalconFlightPlanMessage		*msg = new FalconFlightPlanMessage(Id(), target);
		uchar						*buffer;

		msg->dataBlock.type = FalconFlightPlanMessage::squadronStores;
		msg->dataBlock.size = HARDPOINT_MAX + HARDPOINT_MAX * sizeof(short) + sizeof(long)*2;
		msg->dataBlock.data = buffer = new uchar[msg->dataBlock.size];
		memcpy(buffer,weapon,HARDPOINT_MAX * sizeof(short));		buffer += HARDPOINT_MAX * sizeof(short);
		memcpy(buffer,weapons,HARDPOINT_MAX);		buffer += HARDPOINT_MAX;
		memcpy(buffer,&lbsfuel,sizeof(long));		buffer += sizeof(long);
		memcpy(buffer,&planes,sizeof(long));		buffer += sizeof(long);
		FalconSendMessage(msg, TRUE);		
		}
	}



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SquadronClass::ShiftSchedule (int i)
{
	schedule[i] >>= 1;
	MakeSquadronDirty (DIRTY_SCHEDULE, DDP[125].priority);
	//	MakeSquadronDirty (DIRTY_SCHEDULE, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SquadronClass::SetSchedule (int i, ulong a)
{
	if ((schedule[i] | a) != (schedule[i]))
	{
		schedule[i] |= a;
		MakeSquadronDirty (DIRTY_SCHEDULE, DDP[126].priority);
		//		MakeSquadronDirty (DIRTY_SCHEDULE, SEND_EVENTUALLY);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SquadronClass::ClearSchedule (int i)
{
	schedule[i] = 0;
	MakeSquadronDirty (DIRTY_SCHEDULE, DDP[127].priority);
	//	MakeSquadronDirty (DIRTY_SCHEDULE, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SquadronClass::SetRating (int i, uchar r)
{
	rating[i] = r;
	MakeSquadronDirty (DIRTY_RATING, DDP[128].priority);
	//	MakeSquadronDirty (DIRTY_RATING, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SquadronClass::SetAssigned (uchar a)
{
	assigned = a;
	MakeSquadronDirty (DIRTY_ASSIGNED, DDP[129].priority);
	//	MakeSquadronDirty (DIRTY_ASSIGNED, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SquadronClass::SetPilotLosses(uchar a)
{
	pilot_losses = a;
	MakeSquadronDirty (DIRTY_PILOT_LOSSES, DDP[130].priority);
	//	MakeSquadronDirty (DIRTY_PILOT_LOSSES, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SquadronClass::SetTotalLosses (uchar a)
{
	total_losses = a;
	MakeSquadronDirty (DIRTY_TOTAL_LOSSES, DDP[131].priority);
	//	MakeSquadronDirty (DIRTY_TOTAL_LOSSES, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SquadronClass::SetMissionScore (short i)
{
	mission_score = i;
	MakeSquadronDirty (DIRTY_MISSION_SCORE, DDP[132].priority);
	//	MakeSquadronDirty (DIRTY_MISSION_SCORE, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SquadronClass::SetMissionsFlown (short i)
{
	missions_flown = i;
	MakeSquadronDirty (DIRTY_MISSIONS_FLOWN, DDP[133].priority);
	//	MakeSquadronDirty (DIRTY_MISSIONS_FLOWN, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SquadronClass::SetAAKills (short i)
{
	aa_kills = i;
	MakeSquadronDirty (DIRTY_AAKILLS, DDP[134].priority);
	//	MakeSquadronDirty (DIRTY_AAKILLS, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SquadronClass::SetAGKills (short i)
{
	ag_kills = i;
	MakeSquadronDirty (DIRTY_AGKILLS, DDP[135].priority);
	//	MakeSquadronDirty (DIRTY_AGKILLS, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SquadronClass::SetANKills (short i)
{
	an_kills = i;
	MakeSquadronDirty (DIRTY_ANKILLS, DDP[136].priority);
	//	MakeSquadronDirty (DIRTY_ANKILLS, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SquadronClass::SetASKills (short i)
{
	as_kills = i;
	MakeSquadronDirty (DIRTY_ASKILLS, DDP[137].priority);
	//	MakeSquadronDirty (DIRTY_ASKILLS, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SquadronClass::SetHotSpot (VU_ID id)
{
	hot_spot = id;
	MakeSquadronDirty (DIRTY_HOT_SPOT, DDP[138].priority);
	//	MakeSquadronDirty (DIRTY_HOT_SPOT, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SquadronClass::SetUnitAirbase (VU_ID id)
{
	airbase_id = id;
	MakeSquadronDirty (DIRTY_AIRBASE, DDP[139].priority);
	//	MakeSquadronDirty (DIRTY_AIRBASE, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SquadronClass::UseFuel (long f)
{
	fuel -= f;
	MakeSquadronDirty (DIRTY_FUEL, DDP[140].priority);
	//	MakeSquadronDirty (DIRTY_FUEL, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SquadronClass::SetSquadronFuel (long f)
{
	fuel = f;
	MakeSquadronDirty (DIRTY_FUEL, DDP[141].priority);
	//	MakeSquadronDirty (DIRTY_FUEL, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SquadronClass::SetUnitStores (int i, unsigned char s)
{
	stores[i] = s;
	MakeSquadronDirty (DIRTY_SQUAD_STORES, DDP[142].priority);
	//	MakeSquadronDirty (DIRTY_SQUAD_STORES, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SquadronClass::SetLastResupply (int s)			
{ 
	last_resupply = s; 
	MakeSquadronDirty (DIRTY_SQUAD_RESUP, DDP[143].priority);
	//	MakeSquadronDirty (DIRTY_SQUAD_RESUP, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SquadronClass::MakeSquadronDirty (Dirty_Squadron bits, Dirtyness score)
{
	if (!IsLocal())
		return;

	if (VuState() != VU_MEM_ACTIVE)
		return;

	if (!IsAggregate())
	{
		score = (Dirtyness) ((int) score * 10);
	}

	dirty_squadron |= bits;

	MakeDirty (DIRTY_SQUADRON, score);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SquadronClass::WriteDirty (unsigned char **stream)
{
	unsigned char
		*ptr;

	ptr = *stream;

	//MonoPrint ("  SQ %08x", dirty_squadron);

	// Encode it up
	*(ushort*)ptr = (ushort) dirty_squadron;
	ptr += sizeof (ushort);

	if (dirty_squadron & DIRTY_SCHEDULE)
	{
		memcpy (ptr, schedule, sizeof (schedule));
		ptr += sizeof (schedule);
	}

	if (dirty_squadron & DIRTY_RATING)
	{
		memcpy (ptr, rating, sizeof (rating));
		ptr += sizeof (rating);
	}

	if (dirty_squadron & DIRTY_ASSIGNED)
	{
		*(uchar*)ptr = assigned;
		ptr += sizeof (uchar);
	}

	if (dirty_squadron & DIRTY_PILOT_LOSSES)
	{
		*(uchar*)ptr = pilot_losses;
		ptr += sizeof (uchar);
	}

	if (dirty_squadron & DIRTY_TOTAL_LOSSES)
	{
		*(uchar*)ptr = total_losses;
		ptr += sizeof (uchar);
	}

	if (dirty_squadron & DIRTY_MISSION_SCORE)
	{
		*(short*)ptr = mission_score;
		ptr += sizeof (short);
	}

	if (dirty_squadron & DIRTY_MISSIONS_FLOWN)
	{
		*(short*)ptr = missions_flown;
		ptr += sizeof (short);
	}

	if (dirty_squadron & DIRTY_AAKILLS)
	{
		*(short*)ptr = aa_kills;
		ptr += sizeof (short);
	}

	if (dirty_squadron & DIRTY_AGKILLS)
	{
		*(short*)ptr = ag_kills;
		ptr += sizeof (short);
	}

	if (dirty_squadron & DIRTY_ANKILLS)
	{
		*(short*)ptr = an_kills;
		ptr += sizeof (short);
	}

	if (dirty_squadron & DIRTY_HOT_SPOT)
	{
		*(VU_ID*)ptr = hot_spot;
		ptr += sizeof (VU_ID);
	}

	if (dirty_squadron & DIRTY_AIRBASE)
	{
		*(VU_ID*)ptr = airbase_id;
		ptr += sizeof (VU_ID);
	}

	if (dirty_squadron & DIRTY_FUEL)
	{
		*(long*)ptr = fuel;
		ptr += sizeof (long);
	}

	if (dirty_squadron & DIRTY_SQUAD_STORES)
	{
		memcpy (ptr, stores, sizeof (stores));
		ptr += sizeof (stores);
	}

	if (dirty_squadron & DIRTY_SQUAD_RESUP)
	{
		memcpy (ptr, &last_resupply, sizeof (uchar));
		ptr += sizeof (uchar);
	}

	dirty_squadron = 0;

	*stream = ptr;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SquadronClass::ReadDirty (unsigned char **stream)
{
	unsigned char
		*ptr;

	unsigned short
		bits;

	ptr = *stream;

	bits = *(ushort*)ptr;
	ptr += sizeof (ushort);

	//MonoPrint ("  SQ %08x", bits);

	if (bits & DIRTY_SCHEDULE)
	{
		memcpy (schedule, ptr, sizeof (schedule));
		ptr += sizeof (schedule);
	}

	if (bits & DIRTY_RATING)
	{
		memcpy (rating, ptr, sizeof (rating));
		ptr += sizeof (rating);
	}

	if (bits & DIRTY_ASSIGNED)
	{
		assigned = *(uchar*)ptr;
		ptr += sizeof (uchar);
	}

	if (bits & DIRTY_PILOT_LOSSES)
	{
		pilot_losses = *(uchar*)ptr;
		ptr += sizeof (uchar);
	}

	if (bits & DIRTY_TOTAL_LOSSES)
	{
		total_losses = *(uchar*)ptr;
		ptr += sizeof (uchar);
	}

	if (bits & DIRTY_MISSION_SCORE)
	{
		mission_score = *(short*)ptr;
		ptr += sizeof (short);
	}

	if (bits & DIRTY_MISSIONS_FLOWN)
	{
		missions_flown = *(short*)ptr;
		ptr += sizeof (short);
	}

	if (bits & DIRTY_AAKILLS)
	{
		aa_kills = *(short*)ptr;
		ptr += sizeof (short);
	}

	if (bits & DIRTY_AGKILLS)
	{
		ag_kills = *(short*)ptr;
		ptr += sizeof (short);
	}

	if (bits & DIRTY_ANKILLS)
	{
		an_kills = *(short*)ptr;
		ptr += sizeof (short);
	}

	if (bits & DIRTY_HOT_SPOT)
	{
		hot_spot = *(VU_ID*)ptr;
		ptr += sizeof (VU_ID);
	}

	if (bits & DIRTY_AIRBASE)
	{
		airbase_id = *(VU_ID*)ptr;
		ptr += sizeof (VU_ID);
	}

	if (bits & DIRTY_FUEL)
	{
		fuel = *(long*)ptr;
		ptr += sizeof (long);
	}

	if (bits & DIRTY_SQUAD_STORES)
	{
		memcpy (stores, ptr, sizeof (stores));
		ptr += sizeof (stores);
	}

	if (bits & DIRTY_SQUAD_RESUP)
	{
		memcpy (&last_resupply, ptr, sizeof (uchar));
		ptr += sizeof (uchar);
	}

	*stream = ptr;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
