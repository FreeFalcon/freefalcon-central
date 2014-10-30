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
#include "vutypes.h"
#include "Campaign.h"
#include "ATM.h"
#include "CampList.h"
#include "campwp.h"
#include "update.h"
#include "loadout.h"
#include "campweap.h"
#include "Package.h"
#include "airunit.h"
#include "tactics.h"
#include "Team.h"
#include "Feature.h"
#include "MsgInc/AirTaskingMsg.h"
#include "MsgInc/RadioChatterMsg.h"
#include "MsgInc/DivertMsg.h"
#include "MsgInc/WingmanMsg.h"
#include "MsgInc/AWACsMsg.h"
#include "MsgInc/FalconFlightPlanMsg.h"
#include "wingorder.h"
#include "AIInput.h"
#include "CmpClass.h"
#include "MissEval.h"
#include "classtbl.h"
#include "PtData.h"
#include "Tacan.h"
#include "SimVeh.h"
#include "Camp2sim.h"
#include "falcsess.h"
#include "objectiv.h"
#include "atcbrain.h"
#include "dirtybits.h"
#include "Aircrft.h"
#include "CampMap.h"
#include "GndAI.h"
#include "CampStr.h"
#include "otwdrive.h"
#include "MsgInc/AWACSMsg.h"
#include "FalcSnd/VoiceMapper.h"
/* 2001-06-07 S.G. */
#include "Navunit.h"

/* 2001-09-07 S.G. RP5 */
extern bool g_bRP5Comp;
extern bool g_bUseRC135;
/* 2001-12-10 M.N. */
#include "SIM/Include/aircrft.h"

#include "Graphics/Include/TMap.h"

//sfr: added for checks
#include "InvalidBufferException.h"

#include <time.h>
#include "debuggr.h"

extern int g_nChatterInterval; // FRB - message interval time

#define ENEMY_LOCK_TIMEOUT 4000 // We'll time out an enemy lock in this many ms
#define ADD_TO_KNOWN_EMITTER_DIST 40 // Distance (km) below which to add to our known emitter list

#ifdef DEBUG
int gMaxMoved = 0;
#endif

// ============================================
// Externals
// ============================================

extern unsigned char SHOWSTATS;
extern short NumWeaponTypes;

extern void UnsetPilotStatus(Squadron sq, int pn);
extern int FindTaxiPt(Flight flight, Objective airbase, int checklist);
extern void UI_Refresh(void);

#ifdef DEBUG_TIMING
extern DWORD gAverageFlightMoveTime;
extern int  gFlightMoves;
#endif

#ifdef CAMPTOOL
// Renaming tool stuff
extern VU_ID_NUMBER RenameTable[65536];
extern int gRenameIds;
#endif

#ifdef DEBUG
extern int gCheckConstructFunction;
extern DWORD gAverageFlightDetectionTime;
extern int gFlightDetects;
#endif DEBUG
extern bool g_bAWACSRequired;
extern bool g_bLoadoutSquadStoreResupply;
extern float g_fIdentFactor; // 2002-03-07 S.G.

void AdjustOffset(float c, float s, float *x, float*y, float xo, float yo);
void AdjustOffset(float heading, float *x, float*y, float xo, float yo);

extern AIOffsetType SquadFormations[ GNDAI_FORM_END ][NO_OF_SQUADS];
extern AIOffsetType PlatoonFormations[ GNDAI_FORM_END ][NO_OF_PLATOONS];
extern AIOffsetType CompanyFormations[ GNDAI_FORM_END ][NO_OF_COMPANIES];

extern FILE *save_log, *load_log;

extern int start_save_stream, start_load_stream;

// -------------------------
// Local Function Prototypes
// =========================

void RatePilot(Flight flight, int pilotSlot, int newRating);
void GoHome(Flight flight);

// ==================================
// Some module globals - to save time
// ==================================

int haveWeaps;
int haveFuel;
int ourRange;
int theirDomain;
int ourMission;

// 2001-07-07 MODIFIED BY S.G. TRYING TO GET THE PLANES TO APPEAR ON THE CORRECT POSITION
//float VFormRight[4] = { 0, -500, 500, 1000 };
//float VFormAhead[4] = { 0, -500, -500, -1000 };
float VFormRight[4] = { 0, -5655,   6235,  12318 };
float VFormAhead[4] = { 0, -5655, -17129, -23865 };
long LastawackWarning = SimLibElapsedTime;//me123

#ifdef DEBUG
CampaignTime gLastCombatBonus = 0;
int gCampPlayerInput = Average;
#endif

extern int gCampDataVersion;
extern int g_nFlightVisualBonus;
extern bool g_bRealisticAttrition; // JB 010710

// =================================
// Smart heap pool stuff
// =================================

#ifdef USE_SH_POOLS
MEM_POOL FlightClass::pool;
#endif

// ============================================
// FlightClass Functions
// ============================================

// KCK: ALL FLIGHT CONSTRUCTION SHOULD USE THIS FUNCTION
FlightClass* NewFlight(int type, Unit parent, Unit squad)
{
    FlightClass *new_flight;
    /*VuEnterCriticalSection();
    lastVolatileId = vuAssignmentId;
    vuAssignmentId = lastLowVolitileId;
    vuAssignmentId = lastFlightId;
    vuLowWrapNumber = (FIRST_LOW_VOLATILE_VU_ID_NUMBER + LAST_LOW_VOLATILE_VU_ID_NUMBER)/2;
    vuHighWrapNumber = LAST_LOW_VOLATILE_VU_ID_NUMBER;*/
    new_flight = new FlightClass(type, parent, squad);
    //lastFlightId = vuAssignmentId;
    // lastLowVolitileId = vuAssignmentId;
    /*vuAssignmentId = lastVolatileId;
    vuLowWrapNumber = FIRST_VOLATILE_VU_ID_NUMBER;
    vuHighWrapNumber = LAST_VOLATILE_VU_ID_NUMBER;
    VuExitCriticalSection();*/
    return new_flight;
}

FlightClass::FlightClass(ushort type, Unit parent, Unit squad) : AirUnitClass(type, GetIdFromNamespace(FlightNS))
{
    UnitClassDataType* uc;

    fuel_burnt = 0;
    last_move = 0;
    last_combat = 0;
    SetSpottedTime(0);
    time_on_target = 0;
    mission_over_time = 0;
    mission_target = FalconNullId;
    enemy_locker = FalconNullId;
    requester = FalconNullId;
    mission_context = 0;
    loadout = NULL;
    loadouts = 0;
    mission = old_mission = 0;
    last_direction = Here;
    priority = 0;
    mission_id = 0;
    eval_flags = 0;
    memset(slots, 255, PILOTS_PER_FLIGHT);
    memset(player_slots, 255, PILOTS_PER_FLIGHT);
    memset(pilots, 0, sizeof(uchar)*PILOTS_PER_FLIGHT);
    memset(plane_stats, 0, sizeof(uchar)*PILOTS_PER_FLIGHT);
    last_player_slot = 0;
    callsign_id = 0;
    callsign_num = 0;

    if (parent)
        SetReinforcement(parent->GetReinforcement());
    else
        SetReinforcement(0);

    if (squad)
        squadron = squad->Id();
    else
        squadron = FalconNullId;

    if (parent)
    {
        package = parent->Id();
        parent->AddUnitChild(this);
        SetParent(0);
    }
    else
    {
        package = FalconNullId;
        SetParent(1);
    }

    uc = GetUnitClassData();

    if (uc and uc->Flags bitand U_COMBAT)
        SetCombat(1);
    else
        SetCombat(0);

    dirty_flight = 0;
    assigned_target = FalconNullId;
    override_wp.SetWPAltitudeLevel(-1);

    tacan_channel = -1;
    tacan_band = -1;

    // 2001-04-03 ADDED BY S.G. ecmFlightClassPtr NEEDS TO BE INITIALIZED TO -1 MEANING IT HAS NEVER BEEN READ YET
    ecmFlightPtr = (FlightClass *)(unsigned) - 1;
    // 2001-06-25 ADDED BY S.G. NEED TO INIT OUR NEW MEMBERS
    shotAt = NULL;
    whoShot = NULL;
    // 2001-10-11 ADDED by M.N.
    refuel = 0;
    last_collision_x = last_collision_y = 0; // JPO
}

FlightClass::FlightClass(VU_BYTE **stream, long *rem) : AirUnitClass(stream, rem)
{
    Package pack;

    if (load_log)
    {
        fprintf(load_log, "%08x FlightClass ", *stream - start_load_stream);
        fflush(load_log);
    }

    if (share_.id_.creator_ == vuLocalSession.creator_) // and share_.id_.num_ > lastFlightId){
    {
        FlightNS.UseId(share_.id_.num_);
        //lastFlightId = share_.id_.num_;
        //printf("does this happen?");
    }

    memcpychk(&pos_.z_, stream, sizeof(BIG_SCALAR), rem);
    memcpychk(&fuel_burnt, stream, sizeof(long), rem);

    if (gCampDataVersion < 65)
        fuel_burnt = 0;

    memcpychk(&last_move, stream, sizeof(CampaignTime), rem);
    memcpychk(&last_combat, stream, sizeof(CampaignTime), rem);
    memcpychk(&time_on_target, stream, sizeof(CampaignTime), rem);
    memcpychk(&mission_over_time, stream, sizeof(CampaignTime), rem);
    memcpychk(&mission_target, stream, sizeof(short), rem);

    if (gCampDataVersion < 24)
    {
        char use_loadout = 0;
        uchar weapons[HARDPOINT_MAX];

        loadouts = 1;
        loadout = new LoadoutStruct[loadouts];

        if (gCampDataVersion >= 8)
        {
            memcpychk(&use_loadout, stream, sizeof(char), rem);

            if (use_loadout)
            {
                LoadoutArray junk;
                memcpychk(&junk, stream, sizeof(LoadoutArray), rem);
                loadout[0] = junk.Stores[0];
            }
        }

        if (gCampDataVersion < 18)
        {
            short weapon[HARDPOINT_MAX];
            memcpychk(weapon, stream, sizeof(short)*HARDPOINT_MAX, rem);

            if ( not use_loadout)
            {
                for (int i = 0; i < HARDPOINT_MAX; i++)
                    loadout[0].WeaponID[i] = (short) weapon[i];
            }
        }
        else
        {
            uchar weapon[HARDPOINT_MAX];
            memcpychk(weapon, stream, sizeof(uchar)*HARDPOINT_MAX, rem);

            if ( not use_loadout)
                memcpy(loadout[0].WeaponID, weapon, sizeof(short)*HARDPOINT_MAX);
        }

        memcpychk(weapons, stream, sizeof(uchar)*HARDPOINT_MAX, rem);

        if ( not use_loadout)
            memcpy(loadout[0].WeaponCount, weapons, sizeof(uchar)*HARDPOINT_MAX);
    }
    else
    {
        memcpychk(&loadouts, stream, sizeof(uchar), rem);
        loadout = new LoadoutStruct[loadouts];

        for (int i = 0; i < loadouts; i++)
        {
            if (gCampDataVersion <= 72)
            {
                int j;

                for (j = 0; j < HARDPOINT_MAX; j++)
                {
                    memcpychk(&(loadout[i].WeaponID[j]), stream, sizeof(uchar), rem);
                }

                for (j = 0; j < HARDPOINT_MAX; j++)
                {
                    memcpychk(&(loadout[i].WeaponCount[j]), stream, sizeof(uchar), rem);
                }
            }
            else
            {
                memcpychk(&loadout[i], stream, sizeof(LoadoutStruct), rem);
            }
        }
    }

    memcpychk(&mission, stream, sizeof(uchar), rem);

    if (gCampDataVersion > 65)
    {
        memcpychk(&old_mission, stream, sizeof(uchar), rem);
    }
    else
    {
        old_mission = mission;
    }

    // FRB - Alert
    // sfr: gotta do yaw stuff here, otherwise well get stuck north
    last_direction = Here;
    uchar ld;
    memcpychk(&ld, stream, sizeof(uchar), rem);
    SetLastDirection(ld);
    memcpychk(&priority, stream, sizeof(uchar), rem);
    memcpychk(&mission_id, stream, sizeof(uchar), rem);

    if (gCampDataVersion < 14)
    {
        uchar dummy;
        memcpychk(&dummy, stream, sizeof(uchar), rem);
    }

    memcpychk(&eval_flags, stream , sizeof(uchar), rem);

    if (gCampDataVersion > 65)
    {
        memcpychk(&mission_context, stream , sizeof(uchar), rem);
    }
    else
    {
        mission_context = 0;
    }

    memcpychk(&package, stream, sizeof(VU_ID), rem);

    // Attach us to our package, incase we missed it before
    pack = (PackageClass*) vuDatabase->Find(package);

    if (pack)
    {
        pack->AddUnitChild(this);
    }

    memcpychk(&squadron, stream, sizeof(VU_ID), rem);

    if (gCampDataVersion > 65)
    {
        memcpychk(&requester, stream, sizeof(VU_ID), rem);
    }
    else
    {
        requester = FalconNullId;
    }

#ifdef DEBUG
    package.num_ and_eq 0x0000ffff;
    squadron.num_ and_eq 0x0000ffff;
#endif
    memcpychk(slots, stream, sizeof(uchar)*PILOTS_PER_FLIGHT, rem);
    memcpychk(pilots, stream, sizeof(uchar)*PILOTS_PER_FLIGHT, rem);
    memcpychk(plane_stats, stream, sizeof(uchar)*PILOTS_PER_FLIGHT, rem);
    memcpychk(player_slots, stream, sizeof(uchar)*PILOTS_PER_FLIGHT, rem);
    memcpychk(&last_player_slot, stream, sizeof(uchar), rem);
    memcpychk(&callsign_id, stream, sizeof(uchar), rem);
    memcpychk(&callsign_num, stream, sizeof(uchar), rem);

    if (gCampDataVersion >= 72)
    {
        memcpychk(&refuel, stream, sizeof(unsigned int), rem);
    }
    else
    {
        refuel = 0;
    }

    enemy_locker = FalconNullId;
    last_enemy_lock_time = 0;

    tacan_channel = -1;
    tacan_band = -1;
    WayPoint w = GetCurrentUnitWP();

    if (mission == AMIS_TANKER)// and w and (w->GetWPFlags() bitand WPF_TARGET))
    {
        // Add a Tacan if this is a tanker on station.
        SetTacan(1);
    }

    assigned_target = FalconNullId;
    override_wp.SetWPAltitudeLevel(-1);
    dirty_flight = 0;

    UI_Refresh();
    // 2001-04-03 ADDED BY S.G. ecmFlightClassPtr NEEDS TO BE INITIALIZED TO -1 MEANING IT HAS NEVER BEEN READ YET
    ecmFlightPtr = (FlightClass *)(unsigned) - 1;
    // 2001-06-25 ADDED BY S.G. NEED TO INIT OUR NEW MEMBERS
    shotAt = NULL;
    whoShot = NULL;
    // 2001-10-11 ADDED by M.N.
    // refuel = 0; done above now
    last_collision_x = last_collision_y = 0; // JPO
}

FlightClass::~FlightClass(void)
{
    if (IsAwake())
        Sleep();

    if (loadout)
        RemoveLoadout();
}

int FlightClass::SaveSize(void)
{
    int
    size;

    size = AirUnitClass::SaveSize()
           + sizeof(BIG_SCALAR)
           + sizeof(long)
           + sizeof(CampaignTime)
           + sizeof(CampaignTime)
           + sizeof(CampaignTime)
           + sizeof(CampaignTime)
           + sizeof(short)
           + sizeof(uchar)
           + sizeof(LoadoutStruct) * loadouts
           + sizeof(uchar)
           + sizeof(uchar)
           + sizeof(uchar)
           + sizeof(uchar)
           + sizeof(uchar)
           + sizeof(uchar)
           + sizeof(uchar)
           + sizeof(VU_ID)
           + sizeof(VU_ID)
           + sizeof(VU_ID)
           + sizeof(uchar) * PILOTS_PER_FLIGHT
           + sizeof(uchar) * PILOTS_PER_FLIGHT
           + sizeof(uchar) * PILOTS_PER_FLIGHT
           + sizeof(uchar) * PILOTS_PER_FLIGHT
           + sizeof(uchar)
           + sizeof(uchar)
           + sizeof(uchar)
           + sizeof(unsigned int);

    return size;
}

int FlightClass::Save(VU_BYTE **stream)
{
#ifdef _DEBUG
    VU_BYTE *start = *stream;
#endif
    AirUnitClass::Save(stream);

    if (save_log)
    {
        fprintf(save_log, "%08x FlightClass ", *stream - start_save_stream);
        fflush(save_log);
    }

    memcpy(*stream, &pos_.z_, sizeof(BIG_SCALAR));
    *stream += sizeof(BIG_SCALAR);
    memcpy(*stream, &fuel_burnt, sizeof(long));
    *stream += sizeof(long);
    memcpy(*stream, &last_move, sizeof(CampaignTime));
    *stream += sizeof(CampaignTime);
    memcpy(*stream, &last_combat, sizeof(CampaignTime));
    *stream += sizeof(CampaignTime);
    memcpy(*stream, &time_on_target, sizeof(CampaignTime));
    *stream += sizeof(CampaignTime);
    memcpy(*stream, &mission_over_time, sizeof(CampaignTime));
    *stream += sizeof(CampaignTime);
    memcpy(*stream, &mission_target, sizeof(short));
    *stream += sizeof(short);

    memcpy(*stream, &loadouts, sizeof(uchar));
    *stream += sizeof(uchar);

    for (int i = 0; i < loadouts; i++)
    {
        memcpy(*stream, &loadout[i], sizeof(LoadoutStruct));
        *stream += sizeof(LoadoutStruct);
    }

    memcpy(*stream, &mission, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, &old_mission, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, &last_direction, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, &priority, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, &mission_id, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, &eval_flags, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream , &mission_context, sizeof(uchar));
    *stream += sizeof(uchar);

#ifdef CAMPTOOL

    if (gRenameIds)
        package.num_ = RenameTable[package.num_];

#endif
    memcpy(*stream, &package, sizeof(VU_ID));
    *stream += sizeof(VU_ID);
#ifdef CAMPTOOL

    if (gRenameIds)
        squadron.num_ = RenameTable[squadron.num_];

#endif
    memcpy(*stream, &squadron, sizeof(VU_ID));
    *stream += sizeof(VU_ID);
    memcpy(*stream, &requester, sizeof(VU_ID));
    *stream += sizeof(VU_ID);
    memcpy(*stream, slots, sizeof(uchar)*PILOTS_PER_FLIGHT);
    *stream += sizeof(uchar) * PILOTS_PER_FLIGHT;
    memcpy(*stream, pilots, sizeof(uchar)*PILOTS_PER_FLIGHT);
    *stream += sizeof(uchar) * PILOTS_PER_FLIGHT;
    memcpy(*stream, plane_stats, sizeof(uchar)*PILOTS_PER_FLIGHT);
    *stream += sizeof(uchar) * PILOTS_PER_FLIGHT;
    memcpy(*stream, player_slots, sizeof(uchar)*PILOTS_PER_FLIGHT);
    *stream += sizeof(uchar) * PILOTS_PER_FLIGHT;
    memcpy(*stream, &last_player_slot, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, &callsign_id, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, &callsign_num, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, &refuel, sizeof(unsigned int));
    *stream += sizeof(unsigned int);
#ifdef _DEBUG
    ShiAssert(*stream - start == SaveSize()); // keep us honest JPO
#endif
    return FlightClass::SaveSize();
}

VU_ERRCODE FlightClass::RemovalCallback()
{
    UI_Refresh();
    return AirUnitClass::RemovalCallback();
}

// event Handlers
VU_ERRCODE FlightClass::Handle(VuFullUpdateEvent *event)
{
    // copy data from temp entity to current entity
    Flight tmp_ent = (Flight)(event->expandedData_.get());

    // KCK: Allow this to happen - it makes multiplayer force on force possible
    // ShiAssert ( not IsLocal() );

    memcpy(&pos_.z_, &tmp_ent->pos_.z_, sizeof(BIG_SCALAR));
    memcpy(&fuel_burnt, &tmp_ent->fuel_burnt, sizeof(long));
    memcpy(&last_move, &tmp_ent->last_move, sizeof(CampaignTime));
    memcpy(&last_combat, &tmp_ent->last_combat, sizeof(CampaignTime));
    memcpy(&time_on_target, &tmp_ent->time_on_target, sizeof(CampaignTime));
    memcpy(&mission_over_time, &tmp_ent->mission_over_time, sizeof(CampaignTime));
    memcpy(&mission_target, &tmp_ent->mission_target, sizeof(short));
    memcpy(&mission, &tmp_ent->mission, sizeof(uchar));
    memcpy(&old_mission, &tmp_ent->old_mission, sizeof(uchar));
    memcpy(&loadouts, &tmp_ent->loadouts, sizeof(uchar));
    loadout = new LoadoutStruct[loadouts];

    for (int i = 0; i < loadouts; i++)
        memcpy(&loadout[i], &tmp_ent->loadout[i], sizeof(LoadoutStruct));

    last_direction = Here;
    uchar ld;
    memcpy(&ld, &tmp_ent->last_direction, sizeof(uchar));
    SetLastDirection(ld);
    memcpy(&priority, &tmp_ent->priority, sizeof(uchar));
    memcpy(&mission_id, &tmp_ent->mission_id, sizeof(uchar));
    memcpy(&eval_flags, &tmp_ent->eval_flags, sizeof(uchar));
    memcpy(&mission_context, &tmp_ent->mission_context, sizeof(uchar));
    memcpy(&package, &tmp_ent->package, sizeof(VU_ID));
    memcpy(&squadron, &tmp_ent->squadron, sizeof(VU_ID));
    memcpy(&requester, &tmp_ent->requester, sizeof(VU_ID));
    memcpy(slots, tmp_ent->slots, sizeof(uchar)*PILOTS_PER_FLIGHT);
    memcpy(pilots, tmp_ent->pilots, sizeof(uchar)*PILOTS_PER_FLIGHT);
    memcpy(plane_stats, tmp_ent->plane_stats, sizeof(uchar)*PILOTS_PER_FLIGHT);
    memcpy(player_slots, tmp_ent->player_slots, sizeof(uchar)*PILOTS_PER_FLIGHT);
    memcpy(&last_player_slot, &tmp_ent->last_player_slot, sizeof(uchar));
    memcpy(&callsign_id, &tmp_ent->callsign_id, sizeof(uchar));
    memcpy(&callsign_num, &tmp_ent->callsign_num, sizeof(uchar));

    return AirUnitClass::Handle(event);
}

int FlightClass::MoveUnit(CampaignTime time)
{
    GridIndex x, y, nx = 0, ny = 0, ox, oy;
    CampaignHeading h;
    uchar moving = 1, moved = 0, set_yaw = 0, tactic, follow_wps = FALSE;
    WayPoint w = NULL, ow = NULL;
    FalconEntity *e;

    if ( not Final())
    {
        return 0;
    }

    w = GetCurrentUnitWP();

    if ( not w)
    {
        return -1;
    }

    // VP_changes for tracing DB
    /*
       FILE* deb = fopen("c:\\traceA10\\dbrain.txt", "a");
       fprintf(deb, "FlightClass MoveUnit \n" );
       fclose(deb);
       */

    if (w->GetWPAction() == WP_TAKEOFF)
    {
        // Check for takeoff
        if (Camp_GetCurrentTime() > w->GetWPDepartureTime())
        {
            // Check for holdshort timeout
            if (IsSetFalcFlag(FEC_HOLDSHORT) and Camp_GetCurrentTime() < w->GetWPDepartureTime() + CampaignMinutes)
            {
                return 0;
            }

            UnSetFalcFlag(FEC_HOLDSHORT);
            // JPO - check airbase is still ok. if not force an ATM reassesment
            // and cancel the flight anyway.
            // also check the team owns the airbase.
            Objective o = (Objective)GetUnitAirbase();

            // Cobra : removed comment by sfr
            // sfr: we only cancel aggregated flights
            if ((AirbaseOperational(o) == FALSE) and IsAggregate())
            {
                CancelFlight(this);
                ShiAssert(TeamInfo[GetTeam()] and TeamInfo[GetTeam()]->atm and o);

                if (TeamInfo[GetTeam()] and TeamInfo[GetTeam()]->atm and o)
                {
                    TeamInfo[GetTeam()]->atm->SendATMMessage(
                        o->Id(), GetTeam(), FalconAirTaskingMessage::atmZapAirbase, 0, 0, NULL, 0
                    );
                }

                return 0;
            }
        }
        else
        {
            // Don't bother looking at us again until we're ready to takeoff, or ready to schedule pilots
            //Cobra
            Objective o = (Objective)GetUnitAirbase();

            //Cobra clean up the list if airbase destroyed
            //sfr: dont cancel deagged flights
            if (
                (AirbaseOperational(o) == FALSE) and /* and this not_eq FalconLocalSession->GetPlayerFlight()*/
                IsAggregate()
            )
            {
                CancelFlight(this);
                ShiAssert(TeamInfo[GetTeam()] and TeamInfo[GetTeam()]->atm and o);

                if (TeamInfo[GetTeam()] and TeamInfo[GetTeam()]->atm and o)
                {
                    TeamInfo[GetTeam()]->atm->SendATMMessage(
                        o->Id(), GetTeam(), FalconAirTaskingMessage::atmZapAirbase, 0, 0, NULL, 0
                    );
                    return 0;
                }
            }

            //End
            CampaignTime nextTime = TheCampaign.CurrentTime;

            if (
 not HasPilots() and (((long)(w->GetWPDepartureTime() - TheCampaign.CurrentTime)) <= PILOT_ASSIGN_TIME)
            )
            {
                Squadron sq = (Squadron)GetUnitSquadron();

                if ( not sq or sq->AssignPilots(this))
                {
                    nextTime = w->GetWPDepartureTime();
                }
            }
            else if ( not HasPilots())
            {
                nextTime = w->GetWPDepartureTime() - PILOT_ASSIGN_TIME;
            }
            else
            {
                nextTime = w->GetWPDepartureTime();
            }

            SetLastCheck(nextTime + 1 - UpdateTime());
            SetUnitLastMove(nextTime);
            SetCombatTime(nextTime);
            return 0;
        }
    }

    Squadron sq = (Squadron)GetUnitSquadron();

    // Check for pilots once more
    if ( not HasPilots())
    {
        //Squadron sq = (Squadron)GetUnitSquadron();
        if (sq and not sq->AssignPilots(this))
        {
            CancelFlight(this);

            return -1;
        }
    }

    SetMoving(1);
    SetEmitting(1);

#ifdef GILMANS_BEAM_TACTIC

    if (Locked() and TheCampaign.CurrentTime - last_enemy_lock_time > ENEMY_LOCK_TIMEOUT)
    {
        SetLocked(0);
    }

#endif

    // This is it for deaggregated flights
    if ( not IsAggregate())
    {
        return 0;
    }

    // Check tactics
    w = ResetCurrentWP(this);

    if ( not w)
    {
        return -1;
    }

    tactic = ChooseTactic();

    // 2002-02-12 MN check if target is occupied by us now and set tactic to abort if so
    // 2002-03-03 MN fix, only check for strike mission flights, fixes aborting resupply flights
    if (GetUnitMission() > AMIS_SEADESCORT and GetUnitMission() < AMIS_FAC)
    {
        WayPoint tw = GetCurrentUnitWP();

        while (tw)
        {
            if ( not (tw->GetWPFlags() bitand WPF_TARGET))
            {
                tw = tw->GetNextWP();
                continue;
            }

            CampEntity target;
            target = tw->GetWPTarget();

            if (target and (target->GetTeam() == GetTeam()))
            {
                tactic = ATACTIC_ABORT;
            }

            break;
        }
    }

    // Two options- follow waypoints, or do a tactic
    GetLocation(&x, &y);

    if (
        (tactic == ATACTIC_ENGAGE_AIR) or
        (tactic == ATACTIC_SHOOT_AND_RUN) or
        (tactic == ATACTIC_ENGAGE_STRIKE) or
        (tactic == ATACTIC_ENGAGE_SURFACE)
    )
    {
        // We're engaged- head towards/intercept our target
        vector collPoint;
        e = GetTarget();
        e->GetLocation(&nx, &ny);

        if ((e->IsAirplane() or e->IsUnit()) and DistSqu(x, y, nx, ny) > 10 * 10 and FindCollisionPoint(e, &collPoint, FALSE))
        {
            // KCK HACK: < x km, just fly towards their location
            // sfr: xy order
            ::vector pos = {collPoint.x, collPoint.y};
            //nx = SimToGrid(collPoint.y);
            //ny = SimToGrid(collPoint.x);
            ConvertSimToGrid(&pos, &nx, &ny);
        }

        // Don't stick around in one place if we're already there, keep flying around
        if (x == nx and y == ny)
        {
            nx = x + dx[last_direction];
            ny = y + dy[last_direction];
        }

        set_yaw = 1;
    }

#ifdef GILMANS_BEAM_TACTIC
    else if (tactic == ATACTIC_BEAM)
    {
        int eh, hd;
        e = (CampEntity) vuDatabase->Find(enemy_locker);

        if ( not e or not e->IsFlight())
        {
            SetLocked(0);
            return 0;
        }

        // Determine heading to beam
        h = last_direction;
        eh = ((Flight)e)->last_direction;
        hd = (eh - last_direction);

        if (hd < -4)
            hd += 8;
        else if (hd > 4)
            hd -= 8;

        if (hd <= 0)
            h = (eh + 10) % 8;
        else if (hd > 0)
            h = (eh + 6) % 8;

        // Set our destination a few km to that direction
        nx = x + 5 * dx[h];
        ny = y + 5 * dy[h];
        set_yaw = 1;
    }

#endif
    else if (tactic == ATACTIC_ENGAGE_DEF)
    {
        // What should we do here? Currently, keep following our waypoints
        tactic = 0;
        follow_wps = TRUE;
        set_yaw = 1;
    }
    else if (tactic == ATACTIC_RETROGRADE)
    {
        // Pick a good location to run to
        // How about our home airbase?
        Objective o = (Objective)GetUnitAirbase();

        if (AirbaseOperational(o))
        {
            // JPO - 2nd test to see if we can land
            o->GetLocation(&nx, &ny);

            if (x == nx and y == ny)
            {
                // RV - Biker don't unload Air Mobile over water
                if (this->Cargo() and GetCover(nx, ny) not_eq Water)
                {
                    this->UnloadUnit();
                }

                // If we ran all the way home, land and quit.
                UpdateSquadronStatus(this, TRUE, FALSE);
                return -1;
            }
        }
        else
        {
            AbortFlight(this);
        }

        set_yaw = 1;
    }
    else if (tactic == ATACTIC_ABORT)
    {
        if (GetUnitMission() not_eq AMIS_ABORT)
        {
            // If we're not flying an abort path, make one
            // KCK NOTE: Abort for campaign aircraft is essentially RTB right now - we fly home
            AbortFlight(this);
            DumpWeapons();
            SetUnitMission(AMIS_ABORT);
            SetUnitTOT(0);
            GoHome(this);
            w = GetCurrentUnitWP();

            if (w)
            {
                SetCurrentUnitWP(w);
                w->GetWPLocation(&nx, &ny);
            }
            else
            {
                // If we can't find a valid landing place, just regroup now..
                RegroupFlight(this);
                return -1;
            }

            SetUnitDestination(nx, ny);
        }

        follow_wps = TRUE;
    }

    if ( not tactic or not nx or not ny)
    {
        // Otherwise, follow waypoints
        ShiAssert(w);
        w->GetWPLocation(&nx, &ny);
        follow_wps = TRUE;

        // If we're ahead of schedule, circle (we'll eventually get somewhere where we need to turn towards the WP)
        if (ETA() < w->GetWPArrivalTime() and not Aborted())
        {
            h = last_direction + 4 bitand 0x7;
            nx = x + dx[h];
            ny = y + dy[h];
        }

        set_yaw = 2;
    }

    CampaignTime start_moving = last_move;

    // Move, if we're not at destination
    if (x not_eq nx or y not_eq ny)
    {
        if (w)
        {
            // 2001-03-27 HACK BY S.G. IF THE NEXT WAYPOINT IS A REPEAT WAYPOINT, USE OUR CURRENT LOCATION
            if (w->GetNextWP() and (w->GetNextWP()->GetWPFlags() bitand WPF_REPEAT))
            {
                ow = NULL;
            }
            else
            {
                // END OF HACK
                ow = w->GetPrevWP();
            }
        }

        if (ow)
        {
            ow->GetWPLocation(&ox, &oy);
        }
        else
        {
            GetLocation(&ox, &oy);
        }

        while (moving)
        {
            h = DirectionTo(ox, oy, nx, ny, x, y);

            if (h > 7)
            {
                moving = 0;
                h = Here;
            }

            // This is kinda hacky - basically, limit change in direction to 45 deg per move
            if (h > last_direction)
            {
                if (h - last_direction < 5)
                {
                    h = (last_direction + 1) bitand 0x07;
                }
                else
                {
                    h = (last_direction + 7) bitand 0x07;
                }
            }
            else if (h < last_direction)
            {
                if (last_direction - h < 5)
                {
                    h = (last_direction + 7) bitand 0x07;
                }
                else
                {
                    h = (last_direction + 1) bitand 0x07;
                }
            }

            if (ChangeUnitLocation(h) > 0)
            {
                moved++;
                GetLocation(&x, &y);

                // Check for WP arrival
                if (follow_wps and DistSqu(x, y, nx, ny) < 1.2F)
                {
                    ox = nx;
                    oy = ny;
                    w = ResetCurrentWP(this);

                    if ( not w)
                    {
                        return -1;
                    }

                    w->GetWPLocation(&nx, &ny);
                }

                // If we leave the map, act as if we landed.
                if ((x < 0) or (x >= Map_Max_X) or (y < 0) or (y >= Map_Max_Y))
                {
                    UpdateSquadronStatus(this, TRUE, FALSE);
                    return -1;
                }

                SetLastDirection(h);
            }
            else
            {
                moving = 0;
            }

            // Now do combat
            if (GetCombatTime() > CombatTime())
            {
                DoCombat();
            }
        }

        if (moved)
        {
            CampaignTime move_time = last_move - start_moving;
            UnitClassDataType *uc = GetUnitClassData();

            if (move_time > CampaignMinutes)
            {
                move_time = CampaignMinutes;
            }

            UseFuel((move_time * uc->Rate) / CampaignMinutes);
            TheCampaign.MissionEvaluator->RegisterMove(this);

            // Adjust altitude - JPO convert so we are speaking the same coordinate system
            // JB 020217 Reduce altitude of units have have run out of fuel.
            // Kill units which have hit the ground.
            if (CalculateFuelAvailable(255) == 0)
            {
                int newalt = (int)(-ZPos() - 250);

                if (newalt <= TheMap.GetMEA(XPos(), YPos()))
                {
                    KillUnit();
                }
                else
                {
                    SetUnitAltitude(newalt);
                }
            }
            else if (
                w and not (w->GetWPFlags() bitand WPF_HOLDCURRENT) and 
                FloatToInt32(ZPos()) not_eq FloatToInt32(
                    -1.0F * AdjustAltitudeForMSL_AGL(XPos(), YPos(), -1.0F * w->GetWPAltitude())
                )
            )
            {
                int newalt, curalt = GetUnitAltitude();
                int max_climb = FloatToInt32(moved * KM_TO_FT * 0.5F);
                newalt = FloatToInt32(-1.0F * AdjustAltitudeForMSL_AGL(XPos(), YPos(), -1.0F * w->GetWPAltitude()));

                if (newalt - curalt > max_climb)
                {
                    newalt = curalt + max_climb;
                }
                else if (curalt - newalt > max_climb)
                {
                    newalt = curalt - max_climb;
                }

                SetUnitAltitude(newalt);
            }

            if (set_yaw == 1)
            {
                // Set yaw to current heading
                SetYPR(last_direction * 45 * DTR, 0.0F, 0.0F);
            }
            else if (set_yaw == 2)
            {
                // Set yaw to heading to next waypoint
                float xd, yd, zd;
                w->GetLocation(&xd, &yd, &zd);
                xd -= XPos();
                yd -= YPos();
                SM_SCALAR y = static_cast<float>(atan2(yd, xd));

                // sfr: fixing the wpt bug
                if (y < 0)
                {
                    y += VU_TWOPI;
                }

                uchar d = static_cast<uchar>((y * RTD) / 45);
                SetLastDirection(d);
            }
        }
    }


    if (Engaged())
    {
        // Drop off any cargo we're carrying
        if (Cargo() and Losses())
        {
            AbortFlight(this); // We'll drop cargo on an abort
        }
    }

    return 0;
}

int FlightClass::DoCombat(void)
{
    SetCombatTime(TheCampaign.CurrentTime);

    if (Engaged() and Combat() and Final())
    {
        FalconEntity *e = GetTarget();
        int result = 1;

        if ( not e)
        {
            SetTarget(NULL);
            SetEngaged(0);
            SetCombat(0);
            return 0;
        }

#ifdef KEV_DEBUG
        MonoPrint("Flight %d (%s::%s) vs %d at %d.\n", GetCampID(), MissStr[mission], TacticsTable[tactic].name, e->GetCampID(), TheCampaign.CurrentTime);
#endif

        // Check if our tactic allows for shooting
        if (GetTactic() >= ATACTIC_IGNORE)
        {
#ifdef KEV_DEBUG
            MonoPrint("   unable to fire due to tactic.\n");
#endif
            return 0;
        }

        // Check if target is in our shooting arc
        GridIndex ux, uy, tx, ty;
        int headto;
        GetLocation(&ux, &uy);
        e->GetLocation(&tx, &ty);
        headto = DirectionTo(ux, uy, tx, ty);

        if (headto == Here or abs(headto - last_direction) <= 1 or abs(headto - last_direction) >= 7)
            result = ::DoCombat(this, e);

#ifdef KEV_DEBUG
        else
            MonoPrint("   unable to fire due to position (%d vs %d).\n", last_direction, headto);

#endif

        if (result < 0)
            SetTarget(NULL);
        else if (result > 0)
            SetCombatTime(Camp_GetCurrentTime());
    }

    return 0;
}

CampaignTime FlightClass::ETA(void)
{
    GridIndex   x, y, tx, ty;
    WayPoint w;
    float       d;
    int speed;

    w = GetCurrentUnitWP();

    if ( not w)
        return CampaignDay;

    GetLocation(&x, &y);
    w->GetWPLocation(&tx, &ty);
    speed = GetCruiseSpeed();
    d = Distance(x, y, tx, ty);
    return  Camp_GetCurrentTime() + TimeToArrive(d, (float)speed);
}

// This dumps all non-AA weapons.
int FlightClass::DumpWeapons()
{
    int i, hp;

    for (i = 0; i < loadouts; i++)
    {
        for (hp = 0; hp < HARDPOINT_MAX; hp++)
        {
            // RV - Biker - Don't drop fuel tanks and jammers in 2d
            if (
                loadout[i].WeaponID[hp] and not GetWeaponHitChance(loadout[i].WeaponID[hp], Air) and 
 not (GetWeaponFlags(loadout[i].WeaponID[hp]) bitand WEAP_FUEL) and 
 not (GetWeaponFlags(loadout[i].WeaponID[hp]) bitand WEAP_ECM)
            )
            {
                loadout[i].WeaponID[hp] = 0;
                loadout[i].WeaponCount[hp] = 0;
            }
        }
    }

    return 1;
}

int FlightClass::GetUnitCurrentRole() const
{
    return MissionData[mission].skill;
}

CampEntity FlightClass::GetUnitAirbase(void)
{
    // KCK: Two ways to do this -
    // 1) Get squadron's airbase
    // 2) Get airbase from takeoff waypoint's target
    // I'm taking option 2 since squadron's airbase can change mid-flight, etc..
    WayPoint w = GetFirstUnitWP();

    if ( not w)
    {
        return 0;
    }

    //Why does it has to be takeoff wp? What about Instant Action?
    //ShiAssert(w and w->GetWPAction() == WP_TAKEOFF)
    return w->GetWPTarget();
}

VU_ID FlightClass::GetUnitAirbaseID(void)
{
    // KCK: Two ways to do this -
    // 1) Get squadron's airbase
    // 2) Get airbase from takeoff waypoint's target
    // I'm taking option 2 since squadron's airbase can change mid-flight, etc..
    WayPoint w = GetFirstUnitWP();
    ShiAssert(w and w->GetWPAction() == WP_TAKEOFF)
    return w->GetWPTargetID();
}

int FlightClass::ShouldDeaggregate(void)
{
    if (IsHelicopter())
        return TRUE;

    WayPoint w = GetCurrentUnitWP();

    if (w and w->GetWPAction() == WP_TAKEOFF)
    {
        CampEntity installation = GetUnitAirbase();
        ShiAssert(installation);

        if (installation)
        {
            if (installation->IsObjective())
            {
                int pt = FindTaxiPt(this, (Objective)installation, FALSE);

                if (pt <= 0)
                    return FALSE; // Not ready to take off yet
            }
            else if (installation->IsTaskForce())
                return FALSE;
            else if (installation->IsSquadron())
                return FALSE;
        }
    }

    return TRUE;
}

int FlightClass::GetDeaggregationPoint(int slot, CampEntity *installation)
{
    int pt = 0;

    // RED - CTD Fix - Always make it NULL, even if Heli..
    *installation = NULL;

    if (IsHelicopter())
    {
        return pt;
    }

    WayPoint w = GetCurrentUnitWP();

    if (w and w->GetWPAction() == WP_TAKEOFF)
    {
        *installation = GetUnitAirbase();

        if (*installation == NULL)
        {
            return DPT_ERROR_NOT_READY;
        }

        if ((*installation)->IsObjective())
        {
            pt = FindTaxiPt((Flight)this, (Objective) * installation, TRUE);

            if ( not pt)
            {
                return DPT_ERROR_NOT_READY; // Not ready to take off yet
            }
        }
        else if ((*installation)->IsTaskForce())
            return DPT_ONBOARD_CARRIER;
        else if ((*installation)->IsSquadron())
            return 0;
    }

    return pt;
}

int FlightClass::Reaction(CampEntity e, int knowledge, float range)
{
    int score = 0, enemy_threat_bonus = 1;
    MoveType tmt, omt;
    WayPoint w;
    int approxhitchance; // JB 010711

    if ( not e)
        return 0;

    if (GetUnitMission() == AMIS_NONE)
        return 0;

    // Some basic info.
    omt = GetMovementType();
    tmt = e->GetMovementType();

    if (e->IsFlight() and not ((Flight)e)->Moving())
        return 0; // Aircraft on ground are ignored (technically, strike aircraft could hit them.. but..)

    // Score their threat to us
    if (knowledge bitand FRIENDLY_DETECTED)
        enemy_threat_bonus++;

    if (knowledge bitand FRIENDLY_IN_RANGE)
        enemy_threat_bonus++;

    // Bonus for them being our target
    w = GetCurrentUnitWP();

    if (w and w->GetWPFlags() bitand WPF_TARGET and e->Id() == w->GetWPTargetID())
        score += 2 + GetAproxHitChance(tmt, FloatToInt32(range / 2.0F)) / 10;

    if (assigned_target and (e->Id() == assigned_target or (e->IsUnit() and ((Unit)e)->GetUnitParentID() == assigned_target)))
        score += 2 + GetAproxHitChance(tmt, FloatToInt32(range / 2.0F)) / 5;

    // No more checks necessary vs objectives
    if ( not e->IsUnit())
        return score;

    // we're a little interested if they're targetting us.
    if (((Unit)e)->GetTargetID() == Id())
        score += e->GetAproxHitChance(omt, 0) / 10 * enemy_threat_bonus;
    // JB 010711 Flights on A2G missions do not engage other flights unless they are spotted.
    else if (e->IsFlight() and e->GetTeam() not_eq GetTeam() and not GetSpotted(e->GetTeam()))
    {
        switch (GetUnitMission())
        {
            case AMIS_SEADSTRIKE:
            case AMIS_SEADESCORT:
            case AMIS_OCASTRIKE:
            case AMIS_INTSTRIKE:
            case AMIS_STRIKE:
            case AMIS_DEEPSTRIKE:
            case AMIS_STSTRIKE:
            case AMIS_STRATBOMB:
            case AMIS_FAC:
            case AMIS_ONCALLCAS:
            case AMIS_PRPLANCAS:
            case AMIS_CAS:
            case AMIS_SAD:
            case AMIS_INT:
            case AMIS_BAI:
            case AMIS_BDA:
            case AMIS_SAR:
                return 0;
        }
    }

    // Now score for our ability to kill them, if we're on that sort of mission type
    switch (GetUnitMission())
    {
        case AMIS_BARCAP:
        case AMIS_BARCAP2:
        case AMIS_TARCAP:

            // 2001-04-20 ADDED BY S.G. DON'T REACT IF TOO FAR FROM US...
            if ((e->GetDomain() == DOMAIN_AIR) and (range >= 60 * NM_TO_KM))  //Cobra changed to 60 from 30
            {
                return 0;
            }

            // END OF ADDED SECTION

            // Added bonus for them being attack aircraft
            if (e->IsFlight() and (((Flight)e)->GetUnitCurrentRole() == ARO_GA or ((Flight)e)->GetUnitCurrentRole() == ARO_S or ((Flight)e)->GetUnitCurrentRole() == ARO_SB))
                score += (e->GetAproxHitChance(NoMove, 0) / 10) * enemy_threat_bonus;

            // Added bonus for being the correct mission
            if (e->Id() == mission_target)
                score += GetAproxHitChance(tmt, 0) / 10;

            // Continued for sweep
        case AMIS_INTERCEPT:
        case AMIS_SWEEP:
            if (e->GetDomain() == DOMAIN_AIR)
                score += GetAproxHitChance(tmt, 0) / 10;

            break;

        case AMIS_ESCORT:
        case AMIS_HAVCAP:

            // 2001-04-20 ADDED BY S.G. DON'T REACT IF TOO FAR FROM US...
            if (range >= 25 * NM_TO_KM)
                return 0;

            // END OF ADDED SECTION
            // return 0;

            if ( not g_bRP5Comp)
            {
                // JB 010711
                approxhitchance = GetAproxHitChance(tmt, 0);

                if (approxhitchance < 20)
                    return 0;

                // Bonus just for being aircraft
                if (e->GetDomain() == DOMAIN_AIR)
                    score += approxhitchance / 20;
            }
            else
            {
                // 2001-04-05 MODIFIED BY S.G. NEED TO MAKE HAVCAP AND ESCORT IGNORE BOMBERS. I'LL IGNORE OUR HIT CHANCE AND CONCENTRATE ON OUR TARGET HIT CHANCE ON US
                // score += GetAproxHitChance(tmt,0)/20;
                score += e->GetAproxHitChance(omt, 0) / 26;
            }

            // Added bonus for them attacking the unit we're protecting
            if (((Unit)e)->GetTargetID() == mission_target)
                score += (e->GetAproxHitChance(omt, 0) / 10) * enemy_threat_bonus;

            break;

        case AMIS_SEADESCORT:

            // Added bonus for non-air types attacking the unit we're protecting
            // 2001-06-07 MODIFIED BY S.G. NON AIR, SO IT'S A BATTALION OR NAVAL, RIGHT? USE ITS *AIR* TARGET, NOT ITS *GROUND* TARGET
            // 2001-06-07 NEVER IMPLEMENTED FOR FUTURE TESTS
            if ( not e->IsFlight() and ((Unit)e)->GetTargetID() == mission_target)
                score += (e->GetAproxHitChance(omt, 0) / 10) * enemy_threat_bonus;

            /* {
             FalconEntity *target = NULL;

             if (e->IsBattalion() and ((BattalionClass *)e)->GetAirTargetID() not_eq FalconNullId)
             target = (FalconEntity *) vuDatabase->Find(((BattalionClass *)e)->GetAirTargetID());
             else if (e->IsTaskForce() and ((BattalionClass *)e)->GetAirTargetID() not_eq FalconNullId)
             target = (FalconEntity *) vuDatabase->Find(((TaskForceClass *)e)->GetAirTargetID());

             if (target and target->IsSim())
             target = ((SimBaseClass *)target)->GetCampaignObject();

             if (target and target->Id() == mission_target)
             score += (e->GetAproxHitChance(omt,0)/10) * enemy_threat_bonus;
             }
             */// END OF MODIFIED SECTION

            // Continued for SEAD Strike
        case AMIS_SEADSTRIKE:

            // Added bonus for any SEAD types
            if (e->GetDomain() == DOMAIN_LAND and e->GetSType() == STYPE_UNIT_AIR_DEFENSE)
                score += GetAproxHitChance(tmt, 0) / 10;

            // 2001-06-07 ADDED BY S.G. IF HE TARGETS US, ADD BONUS
            // 2001-06-07 NEVER IMPLEMENTED FOR FUTURE TESTS
            /* {
             FalconEntity *target = NULL;

             if (e->IsBattalion())
             target = (FalconEntity *) vuDatabase->Find(((BattalionClass *)e)->GetAirTargetID());
             else if (e->IsTaskForce())
             target = (FalconEntity *) vuDatabase->Find(((TaskForceClass *)e)->GetAirTargetID());

             if (target and target->IsSim())
             target = ((SimBaseClass *)target)->GetCampaignObject();

             if (target and target->Id() == mission_target)
             score += (e->GetAproxHitChance(omt,0)/10) * enemy_threat_bonus;
             }
             */// END OF ADDED SECTION
            break;

        case AMIS_SAD:
        case AMIS_BAI:
        case AMIS_INT:
            score += GetAproxHitChance(tmt, 0) / 10;
            break;

        case AMIS_CAS:
        case AMIS_ONCALLCAS:
        case AMIS_PRPLANCAS:
            if (e->IsBattalion())
            {
                score += GetAproxHitChance(tmt, 0) / 10 + e->GetAproxHitChance(NoMove, 0) / 10 * enemy_threat_bonus;

                if (((Unit)e)->Engaged())
                    score += 2;

                // Added bonus for being the correct mission
                if (e->Id() == mission_target)
                    score += GetAproxHitChance(tmt, 0) / 10;
            }

            break;

        case AMIS_ASW:
        case AMIS_ASHIP:
            if (e->GetDomain() == DOMAIN_SEA)
                score += GetAproxHitChance(tmt, 0) / 10;

            // Added bonus for being the correct mission
            if (e->Id() == mission_target)
                score += GetAproxHitChance(tmt, 0) / 10;

            break;

        default:
            break;
    }

    // Everyone is interested if these enemies are extremely close
    if (range < MIN_IGNORE_RANGE)
    {
        score += GetAproxHitChance(tmt, FloatToInt32(range / 2.0F)) / 5; // our chance to hit them
        score += e->GetAproxHitChance(omt, 0) / 5 * enemy_threat_bonus; // their chance to hit us
        score += FloatToInt32(MIN_IGNORE_RANGE - range) * 5; // range bonus
    }

    // Helicopters are lower priority than aircraft
    if (e->IsFlight() and ((Flight)e)->IsHelicopter() and score > 3)
        score /= 4;

    // KCK HACK: FAC aircraft are very low priority
    if (score and e->IsFlight() and ((Flight)e)->GetUnitMission() == AMIS_FAC)
        return 1;

    return score;
}

int FlightClass::ChooseTactic(void)
{
    int priority = 0, tid;

    haveWeaps = -1;
    tid = FirstAirTactic;

    // MD -- 20041228: fixing a small campaign bug here.  It was possible for the loop to
    // go past the max number of air tactics and trip the assert condition below the loop.
    // fixed it now so that planes that really ought to abort, really do abort.
    //while (tid < FirstAirTactic + AirTactics and not priority)
    while (tid < AirTactics and not priority)
    {
        priority = CheckTactic(tid);

        if ( not priority)
            tid++;
    }

    ShiAssert(tid < FirstAirTactic + AirTactics);

    if (GetUnitTactic() not_eq ATACTIC_ABORT and tid == ATACTIC_ABORT)
    {
        // Send radio calls on aborts
        if (MissionData[mission].flags bitand AMIS_EXPECT_DIVERT)
        {
            FalconRadioChatterMessage* radioMessage;

            if ( not haveWeaps)
                SendCallToAWACS(this, rcENDCAPARMS, FalconLocalGame);
            else
                SendCallToAWACS(this, rcENDCAPFUEL, FalconLocalGame);

            if (rand() % 2)
                radioMessage = CreateCallFromAwacs(this, rcRELIEVED, FalconLocalGame);
            else
                radioMessage = CreateCallFromAwacs(this, rcDISMISSED, FalconLocalGame);

            radioMessage->dataBlock.time_to_play = CampaignSeconds; // Delay response.
            FalconSendMessage(radioMessage, FALSE);
        }
    }

#if 0 //#ifdef DEBUG

    // if (tid not_eq tactic)
    // MonoPrint("Flight %d (%s) chose tactic %s.\n",GetCampID(),MissStr[mission],TacticsTable[tid].name);
    if ( not IsAggregate())
        MonoPrint("Deag flight %d (%s, %s) chose tactic %s.\n", GetCampID(), MissStr[mission], GetUnitClassData()->Name, TacticsTable[tid].name);

#endif

    SetUnitTactic(tid);
    return tid;
}

int FlightClass::CheckTactic(int tid)
{
    if (haveWeaps < 0) // We've not collected our stats yet
    {
        ourMission = GetUnitMission();
        haveWeaps = HasWeapons();
        haveFuel = HasFuel();
    }

    // Mark us as having weapons for purposes of the non-combat tactics if we've
    // already reached our target and are on a mission type which allows for this.
    // 2001-03-31 MODIFIED BY S.G. REMOVED THE TEST FOR FEVAL_GOT_TO_TARGET FROM THE EQUATION. THIS PREVENTS A2G AIRCRAFT FROM ABORTING
    // if ( not haveWeaps and tid >= ATACTIC_ENGAGE_DEF and MissionData[mission].flags bitand AMIS_NO_TARGETABORT and eval_flags bitand FEVAL_GOT_TO_TARGET)
    if ( not haveWeaps and tid >= ATACTIC_ENGAGE_DEF and MissionData[mission].flags bitand AMIS_NO_TARGETABORT)
        haveWeaps++;

    // Special check for beam tactic
    if (tid == ATACTIC_BEAM)
    {
#ifdef GILMANS_BEAM_TACTIC

        if ( not Locked())
            return 0;
        else
        {
            int d;
            GridIndex x, y, ex, ey;
            Flight e = (Flight) vuDatabase->Find(enemy_locker);

            // Check if our locker exists and has us locked
            if ( not e or e->GetUnitTactic() not_eq ATACTIC_ENGAGE_AIR)
            {
                SetLocked(0);
                return 0;
            }

            // Check tactic range
            GetLocation(&x, &y);
            e->GetLocation(&ex, &ey);
            d = FloatToInt32(Distance(x, y, ex, ey));

            if ( not CheckRange(tid, d))
            {
                SetLocked(0);
                return 0;
            }
        }

        return GetTacticPriority(tid);
#else
        return 0;
#endif
    }

    // Now do the check
    if ( not CheckStatus(tid, Aborted()))
        return 0;

    if ( not CheckEngaged(tid, Engaged()))
        return 0;

    if ( not CheckAction(tid, ourMission))
        return 0;

    if ( not CheckFuel(tid, haveFuel))
        return 0;

    if (Engaged() and not CheckOdds(tid, GetOdds()))
        return 0;

    if (CheckWeapons(tid) == 1 and not haveWeaps)
        return 0;

    if (CheckWeapons(tid) == 2 and (Fired() or not haveWeaps))
    {
        // Marco Edit - if we're out of weapons then we want to abort
        //   if we haven't reached our target yet - if we have then we want to continue our mission steerpoints
        //   since we're on our way home and might need to avoid a threat along our target -> airfield route
        if (tid == ATACTIC_IGNORE and not haveWeaps)
        {
            WayPoint w ;
            w = GetCurrentUnitWP();

            // Look for the Mission Target after this steerpoint - if we find it then we
            // Abort
            while (w)
            {
                if (w->GetWPFlags() bitand WPF_TARGET)
                    return 0;

                w = w->GetNextWP();
            }
        }
    }

    if (tid == ATACTIC_ENGAGE_STRIKE)
    {
        // Make sure our target is our target objective
        // To avoid picking this tactic vs a ground unit enroute.
        if (GetTargetID() not_eq mission_target)
            return 0;
    }

    if (CheckSpecial(tid) > 0)
    {
        if ( not Engaged() or CheckSpecial(tid) not_eq theirDomain)
            return 0;
    }

    if (tid == ATACTIC_ENGAGE_DEF and not CheckRange(tid, ourRange))
        return 0;

    // Some more special case stuff
    if (tid == ATACTIC_ENGAGE_AIR or tid == ATACTIC_SHOOT_AND_RUN)
    {
        // In most cases, we don't want to drive into enemy territory chasing an aborted flight
        // Check for this case and don't choose this tactic if it comes up
        GridIndex x, y;
        Unit e = (Unit) GetTarget();
        GetLocation(&x, &y);

        if ( not e or (e->Aborted() and ::GetOwner(TheCampaign.CampMapData, x, y) == e->GetTeam()))
            return 0;
    }

    return GetTacticPriority(tid);
}

void FlightClass::SetUnitMission(uchar mis)
{
    if (mission not_eq mis)
    {
        mission = mis;
        MakeFlightDirty(DIRTY_MISSION, DDP[95].priority);
        // MakeFlightDirty (DIRTY_MISSION, SEND_EVENTUALLY);

        if (IsTacan() and mis not_eq AMIS_TANKER)
        {
            // Cancel tacan in case of abort
            SetTacan(0);
        }
        else if ( not IsTacan() and mis == AMIS_TANKER)
        {
            SetTacan(1);
        }
    }
}

// 2002-02-11 HELPER BY S.G.
// Will return true if it can identify the target
int CanItIdentify(CampEntity us, CampEntity them, float d, int mt)
{
    float mrs;
    int couldIdent = FALSE;

    // Can you identify visually?
    if (d < GetVisualDetectionRange(mt))
        return TRUE;

    // We can only identify ground thing visually
    if (them->OnGround())
        return FALSE;

    if (us->IsUnit())
    {
        if (GetVehicleClassData(((UnitClass *)us)->class_data->VehicleType[0])->Flags bitand (VEH_HAS_NCTR bitor VEH_HAS_EXACT_RWR))
            couldIdent = TRUE;
    }
    else if (us->IsObjective())
    {
        if (((ObjectiveClass *)us)->HasNCTR())
            couldIdent = TRUE;
    }

    // If we can ident, randomize a bit, and tend to say no against the edge of the envelope
    if (couldIdent)
    {
        mrs = (float)(us->GetDetectionRange(mt)) * g_fIdentFactor; // 2002-03-07 MODIFIED BY S.G. Don't id at full detection range but at a percentage of it

        // If too far, we can't id
        if (d > mrs)
            return FALSE;

        // In 'range', further you are, less chance you have of identifying the target
        if ((int)((mrs - d) / mrs * 100.0f) > rand() % 100)
            return TRUE;
    }

    return FALSE;
}

// Detects other units. Returns 0 if nothing detected, 1 if detected, -1 on error (movement blocked)
// For air units we look at the air defense and emitter lists, and check our WP target. We
// ignore everything else.
// Only real units do detection
int FlightClass::DetectOnMove(void)
{
    if ( not Engaged() and not (GetUnitMoved() % 5))
    {
        return 0;
    }

    return ChooseTarget();
}


int FlightClass::ChooseTarget()
{
    if (IsChecked() or not Final())
    {
        return Engaged();
    }

    if (GetUnitAltitude() == 0)
    {
        return 0;
    }

    FalconEntity *old_target, *react_against = NULL;
    CampEntity e;
    Team who;
    float d, react_distance = 9999.0F;
    int react, enemy, best_reaction = 1, combat, spot = 0, i, ix, ostr, estr = 0, was_engaged, retval = 0;
    int air_search_dist, ground_search_dist;
    int roeg[NUM_TEAMS], roea[NUM_TEAMS];
    GridIndex x, y;

    who = GetTeam();

    for (i = 0; i < NUM_TEAMS; i++)
    {
        roeg[i] = GetRoE(who, i, ROE_GROUND_FIRE);
        roea[i] = GetRoE(who, i, ROE_AIR_FIRE);
    }

    was_engaged = Engaged();
    old_target = GetTarget();
    ShiAssert(
 not old_target or old_target->IsFlight() or old_target->IsBattalion() or
        old_target->IsTaskForce() or old_target->IsObjective() or old_target->IsAirplane()
    );

    SetEngaged(0);
    SetCombat(0);
    SetChecked();

    // Choose which map to use
    GetLocation(&x, &y);
    i = (y / MAP_RATIO) * MRX + (x / MAP_RATIO);

    if (i < 0)
    {
        i = 0;
    }

    // 2001-06-08 MODIFIED BY S.G.
    // USE SamMapSize INSTEAD SINCE RadarMapSize IS 0 UNTIL FIVE MINUTES INTO THE TE/CAMPAIGN
    // if (i > TheCampaign.RadarMapSize)
    // i = TheCampaign.RadarMapSize;
    if (i > TheCampaign.SamMapSize)
    {
        i = TheCampaign.SamMapSize;
    }

    if (who == FalconLocalSession->GetTeam())
    {
        ix = 4;
    }
    else
    {
        ix = 0;
    }

    if (GetMovementType() == Air)
    {
        ix += 2;
    }

    // Find enemy team
    enemy = GetEnemyTeam(who);

    // Check vs SAMs, if any
    // KCK NOTE: Currently only SEAD ac will bother to engage Air Defenses
    // 2001-06-07 MODIFIED BY S.G. SINCE I COMMENTED THE EmitterList QUERY BELOW,
    // LETS FORGET ABOUT THE SAM MAP WHICH IS NOT ACCURATE ANYWAY
    // 2001-06-19 MODIFIED BY S.G. SEAD STRIKES AND ESCORTS GO IN, NOT JUST SEAD ESCORTS
    // if (GetUnitMission() == AMIS_SEADESCORT and (TheCampaign.SamMapData[i] >> ix) bitand 0x03)
    if (GetUnitCurrentRole() == ARO_SEAD and (TheCampaign.SamMapData[i] >> ix) bitand 0x03)
    {
        VuListIterator detit(AirDefenseList);
        e = (CampEntity)detit.GetFirst();

        while (e)
        {
            /* ADDED BY S.G. SO DEAD UNIT ARE NOT TARGETED */
            if ( not e->IsDead() and roeg[e->GetTeam()] == ROE_ALLOWED)
            {
                // 2001-06-07 ADDED BY S.G. SO NON RADAR VEHICLE UNITS ARE NO LONGER A CANDIDATE FOR INCLUSION
                // Don't use the emitting status since they can be on or off depending on AIDS.
                // Also, SAM can turn on their radar, then turn them back off
                // Also, only battalion makes it to the AirDefenseList
                // so I can safely cast it as a battalion class
                // Therefore, in its current version, check how many radar vehicles are left
                // (if the radar vehicle position is above 15, it's a AAA
                // so assume there is one until the unit is dead).
                // 2001-06-19 MODIFIED BY S.G. SEAD STRIKES AND ESCORTS GO IN,
                // NOT JUST SEAD ESCORTS (SEE COMMENTS WITHIN FOR MORE DETAILS)
                //if (((BattalionClass *)e)->class_data->RadarVehicle < 255 and 
                // (((BattalionClass *)e)->class_data->RadarVehicle > 15 or
                // ((BattalionClass *)e)->GetNumVehicles(((BattalionClass *)e)->class_data->RadarVehicle)))
                int enter = FALSE;

                // Must have a radar vehicle (AAA even has more than one)
                if (((BattalionClass *)e)->class_data->RadarVehicle < 255 and e->IsEmitting())
                {
                    // If the radar vehicle index is less than 16, it's a SAM.
                    // Check if it still has a radar vehicle
                    if (((BattalionClass *)e)->class_data->RadarVehicle < 16)
                    {
                        if (((BattalionClass *)e)->GetNumVehicles(
                                ((BattalionClass *)e)->class_data->RadarVehicle)
                           )
                        {
                            GridIndex   x, y, tx, ty;
                            float       d; // VP_changes this definition should be removed
                            GetLocation(&x, &y);
                            e->GetLocation(&tx, &ty);
                            d = Distance(x, y, tx, ty);

                            if (d <= class_data->Range[e->GetMovementType()])
                            {
                                enter = TRUE;
                            }
                        }
                    }
                    // If the radar vehicle index is more than 15, it's an AAA.
                    else
                    {
                        // Only SEAD ESCORT deals with AAA
                        if (GetUnitMission() == AMIS_SEADESCORT)
                        {
                            // If the distance between us and the target is more than 18, we skip it
                            GridIndex   x, y, tx, ty;
                            float       d; // VP_changes this definition should be removed
                            GetLocation(&x, &y);
                            e->GetLocation(&tx, &ty);
                            d = Distance(x, y, tx, ty);

                            if (d <= 18)
                            {
                                enter = TRUE;
                            }
                        }
                    }
                }

                if (enter)
                {
                    // END OF ADDED SECTION (EXCEPT FOR INDENTATION)
                    combat = 0;
                    // VP_changes here d should be determined
                    react = DetectVs(e, &d, &combat, &spot, &estr);

                    // 2001-06-26 ADDED BY S.G. SO NON EMITTING RADAR ARE 'TONED DOWN'
                    // BUT MAKE SURE YOU DON'T TONE DOWN TO ZERO AND ITS DISTANCE IS ARTIFICIALLY INCREASED
                    // if ( not e->IsEmitting()) {
                    // if (react > 1)
                    // react = react / 2;
                    // d *= 1.5f;
                    // }
                    // END OF ADDED SECTION
                    if (react >= best_reaction and d < react_distance)
                    {
                        best_reaction = react;
                        react_distance = d;
                        react_against = e;
                        SetEngaged(1);
                        SetCombat(combat);
                    }

                    if (spot)
                    {
                        // Send radio messages for new contacts
                        FalconRadioChatterMessage *msg = new FalconRadioChatterMessage(Id(), FalconLocalGame);
                        msg->dataBlock.from = Id();
                        msg->dataBlock.to = MESSAGE_FOR_TEAM;
                        msg->dataBlock.voice_id = ((Flight)this)->GetFlightLeadVoiceID();
                        msg->dataBlock.message = rcSAMUP;
                        msg->dataBlock.edata[0] = ((Flight)this)->callsign_id;
                        msg->dataBlock.edata[1] = ((Flight)this)->GetFlightLeadCallNumber();
                        // JB/Marco 010117
                        // Type of Radar/SAM
                        msg->dataBlock.edata[2] = (short)(((Unit)e)->GetVehicleID(0));
                        // Location of Radar/SAM
                        e->GetLocation(&msg->dataBlock.edata[3], &msg->dataBlock.edata[4]);
                        FalconSendMessage(msg, FALSE);
                        // JB/Marco 010117
                    }

                    // 2001-06-07 ADDED BY S.G. FORGET ABOUT THIS RADAR IF IT IS NOW DEAD
                }
                else if (old_target == e)
                {
                    old_target = NULL;
                }

                // END OF ADDED SECTION (EXCEPT FOR INDENTATION
            }

            e = (CampEntity)detit.GetNext();
        }

        // 2001-10-24 ADDED BY S.G. It's possible that our 'AirDefenseList'
        // got updated before we had a chance to finish our attack which can
        // potentially result in the unit being removed from the 'AirDefenseList'.
        // Because of this, old_target will not get cleared. Check now to make sure it is still valid...
        if (
            old_target and old_target->IsBattalion() and 
            ((BattalionClass *)old_target)->class_data->RadarVehicle < 16 and 
            ((BattalionClass *)old_target)->GetNumVehicles(
                ((BattalionClass *)old_target)->class_data->RadarVehicle
            ) == 0
        )
        {
            old_target = NULL;
        }
    }


    // Check vs other aircraft (over the air radius)
    air_search_dist = GetDetectionRange(Air);
    //Cobra, we let DB tell us what is reasonable; See if this helps AI detect father out
    /*if (air_search_dist > MAX_AIR_SEARCH and GetUnitMission() not_eq AMIS_AWACS)
      air_search_dist = MAX_AIR_SEARCH; */ // Reasonable max search distance for non-awacs flights
    BIG_SCALAR rad = GridToSim(air_search_dist);
#ifdef VU_GRID_TREE_Y_MAJOR
    VuGridIterator gridit(RealUnitProxList, YPos(), XPos(), rad);
#else
    VuGridIterator gridit(RealUnitProxList, XPos(), YPos(), rad);
#endif

    e = (CampEntity)gridit.GetFirst();

    while (e)
    {

        if (
            /* ADDED BY S.G. SO DEAD UNIT ARE NOT TARGETED */ not e->IsDead() and /* END OF ADDED SECTION */
            e->IsFlight() and roea[e->GetTeam()] == ROE_ALLOWED and ((Unit)e)->Moving() and e not_eq this
            // ((Unit)e)->current_wp > 1)
        )
        {
            combat = 0;
            react = DetectVs(e, &d, &combat, &spot, &estr);

            if (react >= best_reaction and d < react_distance)
            {
                best_reaction = react;
                react_distance = d;
                react_against = e;
                SetEngaged(1);
                SetCombat(combat);
            }

            if (spot or d < 70.0f and LastawackWarning + 20000.0f < SimLibElapsedTime)
            {
                //me123 addet or check
                // Send radio messages for new contacts
                FalconRadioChatterMessage *msg = NULL;

                bool mAWACSavail = false;

                if ( not g_bAWACSRequired or GetFlightController())
                {
                    mAWACSavail = true; // We've a flight controller
                }

                // MN - if we don't have a flight controller but require AWACS,
                // check if there's an AWACS at all...
                if ( not mAWACSavail)
                {
                    Unit nu, cf;
                    VuListIterator myit(AllAirList);
                    nu = (Unit) myit.GetFirst();

                    while (nu and not mAWACSavail)
                    {
                        cf = nu;
                        nu = (Unit) myit.GetNext();

                        if ( not cf->IsFlight() or cf->IsDead())
                        {
                            continue;
                        }

                        if (cf->GetUnitMission() == AMIS_AWACS and cf->GetTeam() == GetTeam())
                        {
                            mAWACSavail = true;
                        }
                    }
                }

                if (GetUnitMission() == AMIS_AWACS)
                {
                    if (((Unit)e)->GetCurrentWaypoint() < 3)
                    {
                        msg = CreateCallFromAwacs(this, rcENEMYLAUNCH, FalconLocalGame);
                        e->GetLocation(&msg->dataBlock.edata[4], &msg->dataBlock.edata[5]);
                        msg->dataBlock.edata[2] = -1; // Just say a general warning message, not
                        msg->dataBlock.edata[3] = -1; // AWACS warning another AWACS...
                    }
                    else
                    {
                        msg = CreateCallFromAwacs(this, rcBVRTHREATWARN, FalconLocalGame);
                        e->GetLocation(&msg->dataBlock.edata[4], &msg->dataBlock.edata[5]);
                        msg->dataBlock.edata[6] = ((Unit)e)->GetUnitAltitude();
                        msg->dataBlock.edata[2] = -1; // Just say a general warning message, not
                        msg->dataBlock.edata[3] = -1; // AWACS warning another AWACS...
                    }
                }
                else if (GetTotalVehicles() > 0 and mAWACSavail) //me123 from 1
                {
                    //me123 multichanges here
                    msg = CreateCallFromAwacs(this, rcBVRTHREATWARN, FalconLocalGame);
                    e->GetLocation(&msg->dataBlock.edata[4], &msg->dataBlock.edata[5]);
                    msg->dataBlock.edata[6] = ((Unit)e)->GetUnitAltitude();
                    LastawackWarning = SimLibElapsedTime;
                    RequestIntercept((Flight)this, enemy);//Cobra Let Awacs request Intercepts

                    if (GetFlightController())
                    {
                        msg->dataBlock.from = GetFlightController()->Id();
                        msg->dataBlock.voice_id = (uchar)(GetFlightController())->GetFlightLeadVoiceID();
                        msg->dataBlock.edata[2] = (GetFlightController())->callsign_id;
                    }
                    else
                    {
                        msg->dataBlock.from = FalconNullId;
                        msg->dataBlock.voice_id = GetDefaultAwacsVoice(); // JPO VOICEFIX
                        msg->dataBlock.edata[2] = gDefaultAWACSCallSign;
                    }
                }

                if (msg)
                {
                    FalconSendMessage(msg, FALSE);
                }
            }
        }

        e = (CampEntity)gridit.GetNext();
    }

    // Check vs Waypoint target, if any
    WayPoint w = GetCurrentUnitWP();
    // 2001-06-17 MODIFIED BY S.G. SO WE CHECK THE NEXT WAYPOINT IN CASE
    // THE DISTANCE BETWEEN THIS WAYPOINT AND TARGET WAYPOINT (NEXT ONE) IS TOO SHORT
    // if (w and w->GetWPFlags() bitand WPF_TARGET)
    // 2001-10-19 ADDED BY S.G. Don't check range if flying toward the attack waypoint (weird way of doing it)
    int towardTarget = TRUE;

    if (
        w and ((w->GetWPFlags() bitand WPF_TARGET) or
              ((w = w->GetNextWP()) and (w->GetWPFlags() bitand WPF_TARGET) and not (towardTarget = FALSE)))
    )
    {
        if ( not towardTarget)
        {
            GridIndex   x, y, tx, ty;
            float       d = 0.0F; // this is temporary change only to run application
            GetLocation(&x, &y);
            w->GetWPLocation(&tx, &ty);
            d = Distance(x, y, tx, ty);
        }

        e = w->GetWPTarget();
        // VP_changes Oct 1:Run-Time Check Failure #3 - The variable 'd' is being used without being defined.

        GridIndex   x, y, tx, ty;
        GetLocation(&x, &y);
        w->GetWPLocation(&tx, &ty);
        d = Distance(x, y, tx, ty);

        if (e and (towardTarget or d > class_data->Range[e->GetMovementType()]))
        {
            // END OF MODIFIED SECTION (EXCEPT FOR BLOCK INDENTATION)
            Unit parent = NULL;
            int element = 0;
            // e = w->GetWPTarget();  // S.G. DONE ABOVE

            if (e and e->IsUnit() and ((Unit)e)->Father())
            {
                // Check vs each element in a father unit
                parent = (Unit)e;
                e = parent->GetUnitElement(element);
            }

            while (e and roeg[e->GetTeam()] == ROE_ALLOWED)
            {
                // 2001-06-11 ADDED BY S.G. SO DEAD UNIT ARE NOT TARGETED
                if ( not e->IsDead())
                {
                    // END OF ADDED SECTION (EXCEPT FOR BLOCK INDENTATION)
                    combat = 0;
                    react = DetectVs(e, &d, &combat, &spot, &estr);

                    if (react >= best_reaction and d < react_distance)
                    {
                        best_reaction = react;
                        react_distance = d;
                        react_against = e;
                        SetEngaged(1);
                        SetCombat(combat);
                    }
                }

                if (parent)
                {
                    element++;
                    e = parent->GetUnitElement(element);
                }
                else
                {
                    e = NULL;
                }
            }

            /* S.G.*/
        }
    }

    // Check vs all players
    FalconSessionEntity *session;
    VuSessionsIterator sit(FalconLocalGame);
    session = (FalconSessionEntity*) sit.GetFirst();

    while (session)
    {
        AircraftClass *player = (AircraftClass*) session->GetPlayerEntity();

        if (player and /* ADDED BY S.G. SO DEAD UNIT ARE NOT TARGETED */ not player->IsDead() and /* END OF ADDED SECTION */ session->GetTeam() < NUM_TEAMS and roea[session->GetTeam()] == ROE_ALLOWED and player->IsAirplane())
        {
            combat = 0;
            react = DetectVs(player, &d, &combat, &spot, &estr);

            if (react >= best_reaction and d < react_distance)
            {
                best_reaction = react;
                react_distance = d;
                react_against = player;
                SetEngaged(1);
                SetCombat(combat);
            }
        }

        session = (FalconSessionEntity*) sit.GetNext();
    }

    // Check vs ground units, if we're of a valid mission type
    if (
        (GetUnitMission() == AMIS_JSTAR or GetUnitCurrentRole() == ARO_GA or GetUnitCurrentRole() == ARO_FAC)
        /* and not react_against*/
    )
    {
        ground_search_dist = GetDetectionRange(Tracked);

        if (ground_search_dist > MAX_GROUND_SEARCH and GetUnitMission() not_eq AMIS_JSTAR)
            ground_search_dist = MAX_GROUND_SEARCH; // Reasonable max search distance for non-awacs flights

#ifdef VU_GRID_TREE_Y_MAJOR
        VuGridIterator gridit(RealUnitProxList, YPos(), XPos(), (BIG_SCALAR)GridToSim(ground_search_dist));
#else
        VuGridIterator gridit(RealUnitProxList, XPos(), YPos(), (BIG_SCALAR)GridToSim(ground_search_dist));
#endif
        e = (CampEntity)gridit.GetFirst();

        while (e)
        {
            if (e->IsBattalion() and 
                /* ADDED BY S.G. SO DEAD UNIT ARE NOT TARGETED */ not e->IsDead() and /* END OF ADDED SECTION */
                roeg[e->GetTeam()] == ROE_ALLOWED
               )
            {
                combat = 0;
                react = DetectVs(e, &d, &combat, &spot, &estr);

                if (react >= best_reaction and d < react_distance)
                {
                    best_reaction = react;
                    react_distance = d;
                    react_against = e;
                    SetEngaged(1);
                    SetCombat(combat);
                }
            }

            e = (CampEntity)gridit.GetNext();
        }
    }

    // Check vs Emitters, if we're SEAD
    // 2001-03-17 MODIFIED BY S.G. LETS TRY THAT CanDetect ON EVERY GROUND EMITTER...
    //            BUT WAIT SINCE THIS CODE ALREADY DOES IT SO I'LL COMBINED BOTH INTO ONE
#if 0

    if (GetUnitCurrentRole() == ARO_SEAD /* and not react_against*/ and ::GetOwner(TheCampaign.CampMapData, x, y) not_eq who)
    {
        VuListIterator detit(EmitterList);
        e = (CampEntity)detit.GetFirst();

        while (e)
        {
            if (/* ADDED BY S.G. SO DEAD UNIT ARE NOT TARGETED */ not e->IsDead() and /* END OF ADDED SECTION */ roeg[e->GetTeam()] == ROE_ALLOWED)
            {
                react = DetectVs(e, &d, &combat, &spot, &estr);

                if (react >= best_reaction and d < react_distance)
                {
                    best_reaction = react;
                    react_distance = d;
                    react_against = e;
                    SetEngaged(1);
                    SetCombat(combat);
                }
            }

            e = (CampEntity)detit.GetNext();
        }
    }

    // Check to see if we've been detected by ground radar
    if ((TheCampaign.RadarMapData) and ( not GetSpotted(enemy)) and (((TheCampaign.RadarMapData[i] >> ix) bitand 0x03) > (rand() % 4)))
    {
        // KCK: Realistically, we should do a CanDetect() on every ground emitter which could see us,
        // to check for detection, but that's pretty costly.
        // Right now, if the map region is marked as detected, we'll set ourselves detected.
        UnitClassDataType *uc = GetUnitClassData();

        // KCK: Stealth AC don't get spotted by the enemy
        if ( not (uc->Flags bitand VEH_STEALTH))
        {
            if ( not GetSpotted(enemy))
                RequestIntercept((Flight)this, enemy);

            SetSpotted(enemy, TheCampaign.CurrentTime);
        }
    }

#else

    // Don't even start testing if we have been spotted in the last ReconLossTime[GetMovementType()]/8
    if (Camp_GetCurrentTime() - GetSpotTime() > ReconLossTime[GetMovementType()] / 8 or not ((GetSpotted() >> enemy) bitand 0x01))
    {
        VuListIterator detit(EmitterList);
        e = (CampEntity)detit.GetFirst();

        while (e)
        {
            /* ADDED BY S.G. SO DEAD UNIT ARE NOT TARGETED */
            if ( not e->IsDead() and roeg[e->GetTeam()] == ROE_ALLOWED)
            {
                // This Code is the original "// Check vs Emitters, if we're SEAD"
                // 2001-06-07 REMOVED BY S.G. SEAD STRIKES SHOULD GO AGAINST THEIR TARGET (DONE ABOVE),
                // SEAD ESCORT AGAINST THE AIR DEFENSE, NOT EMITTERS (DONE ABOVE)...
                /*
                // if (GetUnitCurrentRole() == ARO_SEAD and ::GetOwner(TheCampaign.CampMapData,x,y) not_eq who) {
                // 2001-06-07 ADDED BY S.G. SO DEAD RADAR ARE NO LONGER A CANDIDATE FOR INCLUSION
                if (e->IsEmitting()) {
                // END OF ADDED SECTION (EXCEPT FOR INDENTATION
                react = DetectVs(e, &d, &combat, &spot, &estr);
                if (react >= best_reaction and d < react_distance) {
                best_reaction = react;
                react_distance = d;
                react_against = e;
                SetEngaged(1);
                SetCombat(combat);
                }
                // 2001-06-07 ADDED BY S.G. FORGET ABOUT THIS RADAR IF IT IS NOW DEAD
                }
                else if (old_target == e)
                old_target = NULL;
                // END OF ADDED SECTION (EXCEPT FOR INDENTATION
                }
                */
                // This Code is OUR "Check to see if we've been detected by OBJECTIVE ground radar"
                // No need to test for unit ground radar since they'll do it themselve...
                // plus we let CanDetect handle stealth flights
                // Stop the loop once we got spotted
                if (e->IsObjective() and e->CanDetect(this) and ((ObjectiveClass *)e)->IsGCI())
                {
                    if ( not GetSpotted(e->GetTeam()))
                        RequestIntercept((Flight)this, e->GetTeam());

                    float range = Distance(XPos(), YPos(), e->XPos(), e->YPos()) / GRID_SIZE_FT;
                    SetSpotted(enemy, TheCampaign.CurrentTime, CanItIdentify(e, this, range, this->GetMovementType())); // 2002-02-11 MODIFIED BY S.G. Test if identifed by ground radar
                    break;
                }
            }

            e = (CampEntity)detit.GetNext();
        }

        int placeHolder = 0;
    }

#endif
    // END OF MODIFIED SECTION

    // Check vs assigned target, if any and if not already chosen
    if (assigned_target not_eq FalconNullId)
    {
        int undivert = FALSE;

        e = (CampEntity) vuDatabase->Find(assigned_target);

        if (e and (e->IsPackage() or e->IsBrigade()))
            e = ((Unit)e)->GetFirstUnitElement();

        // KCK: Check if assigned target is still viable
        if (e and e->IsUnit() and ((e->IsFlight() and ((Unit)e)->Broken()) or ((Unit)e)->IsDead()))
            undivert = TRUE;

        if (e and /* ADDED BY S.G. SO DEAD UNIT ARE NOT TARGETED */ not e->IsDead() and /* END OF ADDED SECTION */ e not_eq react_against)
        {
            int tstr = 0;
            combat = 0;
            react = DetectVs(e, &d, &combat, &spot, &tstr) + 1;

            if (react >= best_reaction and d < react_distance)
            {
                best_reaction = react;
                react_distance = d;
                react_against = e;
                SetEngaged(1);
                SetCombat(combat);
            }
            else
            {
                // KCK: This whole section of code is pretty suspect.
                // I am attempting to allow AWACs to retask another flight when we
                // run into some higher priority target. Perhaps the better thing to
                // do in this case is send another mission request and clear this
                // flight's assigned target - but 'undiverting' is actually a very
                // error prone task.
                // Clear our hold on this target, so someone else can be tasked
                // NOTE: this momentarily sets this entity as unspotted.
                e->SetSpotted(GetTeam(), 0);
                undivert = TRUE;
                // KCK: Not sure about this - it does keep AWACS from retasking us, though
                // priority = react;
            }
        }
        else
            undivert = TRUE;

        // If we've set the "undivert" flag, we need to return to our origional mission
        // KCK: This has not been well tested and is almost guarenteed to be buggy.
        if (undivert)
        {
            MissionRequestClass mis;
            requester = FalconNullId;
            ClearAssignedTarget();
            SetUnitMission(old_mission);
            ClearDivertWayPoints(this);
            // Update the mission evaluator
            mis.mission = old_mission;
            mis.targetID = mission_target;
            TheCampaign.MissionEvaluator->RegisterDivert(this, &mis);
            SetDiverted(0);
            MakeFlightDirty(DIRTY_DIVERT_INFO, DDP[96].priority);
            // MakeFlightDirty (DIRTY_DIVERT_INFO, SEND_SOON);
        }
    }

    // Check vs current target, if any and if not already chosen
    if (old_target and old_target not_eq react_against and /* ADDED BY S.G. SO DEAD UNIT ARE NOT TARGETED */ not old_target->IsDead() and /* END OF ADDED SECTION */ roeg[old_target->GetTeam()] == ROE_ALLOWED)
    {
        int tstr = 0;
        combat = 0;

        if (old_target->IsAirplane())
            react = DetectVs((AircraftClass*)old_target, &d, &combat, &spot, &tstr);
        else if (old_target->IsCampaign())
            react = DetectVs((CampBaseClass*)old_target, &d, &combat, &spot, &tstr);
        else
        {
            d = 9999.0F;
            react = 0;
        }

        if (react >= best_reaction and d < react_distance)
        {
            best_reaction = react;
            react_distance = d;
            react_against = old_target;
            SetEngaged(1);
            SetCombat(combat);
        }
    }

    if ( not was_engaged and Engaged() and not (rand() % 6))
    {
        if (/* ADDED BY S.G. SO DEAD UNIT ARE NOT TARGETED */ not react_against->IsDead() and /* END OF ADDED SECTION */react_against->IsFlight())
        {
            // Report engagements for RadioChatter
            GridIndex x2, y2;
            FalconRadioChatterMessage *msg = new FalconRadioChatterMessage(Id(), FalconLocalGame);
            msg->dataBlock.from = Id();
            msg->dataBlock.to = MESSAGE_FOR_TEAM;
            msg->dataBlock.voice_id = ((Flight)this)->GetPilotVoiceID(0);
            react_against->GetLocation(&x2, &y2);

            if (1)//me123 (GetUnitCurrentRole() == ARO_CA)
            {
                msg->dataBlock.message = rcENGAGINGB;
                msg->dataBlock.edata[0] = ((Flight)this)->callsign_id;
                msg->dataBlock.edata[1] = ((Flight)this)->GetFlightLeadCallNumber();
                msg->dataBlock.edata[2] = (((Unit)react_against)->GetVehicleID(0)) * 2;
                msg->dataBlock.edata[3] = x2;
                msg->dataBlock.edata[4] = y2;
                msg->dataBlock.edata[5] = GetUnitAltitude();
                //msg->dataBlock.time_to_play = g_nChatterInterval * CampaignSeconds; // FRB - Chatter control
            }
            else
            {
                msg->dataBlock.message = rcENGDEFENSIVEA;
                msg->dataBlock.edata[0] = ((Flight)this)->callsign_id;
                msg->dataBlock.edata[1] = ((Flight)this)->GetFlightLeadCallNumber();
                msg->dataBlock.edata[2] = x2;
                msg->dataBlock.edata[3] = y2;
                msg->dataBlock.edata[4] = GetUnitAltitude();
                //msg->dataBlock.time_to_play = g_nChatterInterval * CampaignSeconds; // FRB - Chatter control
            }

            FalconSendMessage(msg, FALSE);
        }
    }

    ShiAssert( not react_against or react_against->IsFlight() or react_against->IsBattalion() or react_against->IsTaskForce() or react_against->IsObjective() or react_against->IsAirplane());

    if (react_against /* ADDED BY S.G. SO DEAD UNIT ARE NOT TARGETED */ and not react_against->IsDead() /* END OF ADDED SECTION */)
    {
        SetTarget(react_against);
        ostr = GetUnitScore(this, react_against->GetMovementType());
        retval = 1;
    }
    else
    {
        ostr = GetUnitScore(this, Air);
        retval = 0;
        SetTarget(NULL);
    }

    if (react_distance < 1.1F)
    {
        // KCK - due to timing, it's possible to overfly our target before we get a chance to shoot.
        // Make sure we're able to shoot right now
        SetCombatTime(0);
    }

#if 0 //#ifdef DEBUG

    if ( not IsAggregate())
    {
        if (react_against and react_against->IsUnit())
            MonoPrint("Deag flight %d (%s, %s) chose target %d (%s, %s).\n", GetCampID(), GetUnitClassData()->Name, MissStr[GetUnitMission()], react_against->GetCampID(), ((Unit)react_against)->GetUnitClassData()->Name, MissStr[((Unit)react_against)->GetUnitMission()]);
        else if (react_against)
            MonoPrint("Deag flight %d (%s, %s) chose target %d\n", GetCampID(), GetUnitClassData()->Name, MissStr[GetUnitMission()], react_against->GetCampID());
        else
            MonoPrint("Deag flight %d (%s, %s) chose no target.\n", GetCampID(), GetUnitClassData()->Name, MissStr[GetUnitMission()]);
    }

#endif

    // These odds arn't very accurate - as it only counts strength vs us from potential targets,
    // not everyone who is actually engaing us
    if (estr)
    {
        Flight flight = (Flight)this;
        Package pack = (Package)GetUnitParent();

        // Add any escorts we have to our strength
        for (int i = GetUnitElement() + 1; pack and flight; i++)
        {
            flight = (Flight)pack->GetUnitElement(i);
            ostr += GetUnitScore(flight, Air);
        }

        SetOdds((ostr * 10) / estr);
    }
    else
        SetOdds(ostr * 10);

    // Recheck our tactic if we've chosen a new target
    if (old_target not_eq react_against /* ADDED BY S.G. SO DEAD UNIT ARE NOT TARGETED */ and react_against and not react_against->IsDead() /* END OF ADDED SECTION */)
        ChooseTactic();

    return retval;
}

void FlightClass::SimSetLocation(float x, float y, float z)
{
    GridIndex cx, cy, nx, ny;

    // Check if flight has moved, and evaluate current situation if so
    GetLocation(&cx, &cy);
    // sfr: xy order
    ::vector pos = { x, y };
    ConvertSimToGrid(&pos, &nx, &ny);

    //nx = SimToGrid(y);
    //ny = SimToGrid(x);
    if (cx not_eq nx or cy not_eq ny or fabs(z - ZPos()) > 500)
    {
        SetPosition(x, y, z);
        //MakeCampBaseDirty (DIRTY_POSITION, DDP[97].priority);
        MakeCampBaseDirty(DIRTY_POSITION, SEND_SOON);
        //MakeCampBaseDirty (DIRTY_ALTITUDE, DDP[98].priority);
        MakeCampBaseDirty(DIRTY_ALTITUDE, SEND_SOON);
        TheCampaign.MissionEvaluator->RegisterMove(this);
    }
}

void FlightClass::SimSetOrientation(float yaw, float, float)
{
    if (yaw < 0.0F)
    {
        yaw += 2.0F * PI;
    }

    SetLastDirection((int)(yaw * RTD) / 45);
}

void FlightClass::GetRealPosition(float *x, float *y, float *z)
{
    // This will use the last move time to determine the real x,y bitand z of the unit
    WayPoint w = GetCurrentUnitWP();
    float movetime = (float)(SimLibElapsedTime - last_move) / VU_TICS_PER_SECOND;
    float nx, ny, nz;
    float speed;
    float heading;
    float dist;
    mlTrig sincos;

    if (SimLibElapsedTime > last_move or not w)
    {
        *x = XPos();
        *y = YPos();
        *z = ZPos();
        return;
    }
    else
    {
        // edg: prevent negative movetime
        movetime = 1;
    }

    w->GetLocation(&nx, &ny, &nz);
    nz = AdjustAltitudeForMSL_AGL(nx, ny, nz);
    speed = (float) GetUnitSpeed() * KPH_TO_FPS;
    dist = speed * movetime;
    heading = (float) atan2(ny - YPos(), nx - XPos());
    mlSinCos(&sincos, heading);
    *x = XPos() + dist * sincos.cos;
    *y = YPos() + dist * sincos.sin;

    if (nz > ZPos())
    {
        *z = ZPos() + dist * 0.5F;

        if (*z > nz)
            *z = nz;
    }
    else if (nz < ZPos())
    {
        *z = ZPos() - dist * 0.5F;

        if (*z < nz)
            *z = nz;
    }
    else
        *z = nz;
}

CampaignTime FlightClass::GetMoveTime(void)
{
    if (last_move and TheCampaign.CurrentTime > last_move)
        return TheCampaign.CurrentTime - last_move;

    last_move = TheCampaign.CurrentTime;

    return 0;
}

int FlightClass::BuildMission(MissionRequestClass *mis)
{
    Package pack;
    WayPoint w = NULL;
    CampaignTime time;
    uchar* damageMods;
    MoveType mt = NoMove;
    CampEntity target;
    int i, needweaps = 0;
    long length, fuelNeeded, fuelAvail;
    Squadron squad;

    SetUnitTOT(mis->tot); // Temporary time on target for timing.
    SetUnitPriority(mis->priority);

    pack = (Package)GetUnitParent();
    squad = (Squadron)GetUnitSquadron();
    target = (CampEntity) vuDatabase->Find(mis->targetID);
    mission_target = mis->targetID;
    mission_context = mis->context;
    requester = mis->requesterID;

    if (mis->flags bitand AMIS_IMMEDIATE)
    {
        //MakeFlightDirty (DIRTY_DIVERT_INFO, DDP[99].priority);
        MakeFlightDirty(DIRTY_DIVERT_INFO, SEND_SOON);
        SetUnitMission(mis->mission);
        // Build the divert path
        ClearDivertWayPoints(this);
        BuildDivertPath(this, mis);
        // Update the mission evaluator
        TheCampaign.MissionEvaluator->RegisterDivert(this, mis);
        SetDiverted(1);
        return PRET_SUCCESS;
    }
    else
    {
        // Full scheduled mission, we've got a bit to do..
        SetUnitMission(mis->mission);
        old_mission = mis->mission;
        DisposeWayPoints();

        if ( not BuildPathToTarget(this, mis, squad->GetUnitAirbaseID()))
            return PRET_ABORTED;

        length = SetWPTimes(this, mis);

        if ( not length and mis->mission not_eq AMIS_ALERT)
            return PRET_ABORTED;

        SetCurrentUnitWP(GetFirstUnitWP());
        AddInformationWPs(this, mis);
    }

    // For divertable missions, we set our priority to 0 if we're not currently
    // being diverted, because we want to be available for anything which comes up
    if ((MissionData[mis->mission].flags bitand AMIS_EXPECT_DIVERT) and not Diverted() and mis->mission not_eq AMIS_ONCALLCAS)
    {
        SetUnitPriority(0);
    }

    // Check time tolerences and plan for a reserve
    w = GetFirstUnitWP();

    if ((mis->flags bitand REQF_ALLOW_ERRORS) and length < 0)
    {
        length = CampaignHours; // If it's an impossible mission and we're allowing errors, assume 1 hour
    }

    if (length < 0)
    {
        // Impossible takeoff time, if we need TOT <= to that requested, abort
        if (mis->tot_type <= TYPE_EQ and mis->tot_type > TYPE_NE)
        {
            return PRET_CANCELED; // This TOT is inflexible
        }

        if (pack->GetFlights() or mission_id)
        {
            return PRET_CANCELED; // Additional flights already planned
        }

        // Otherwise, just adjust our TOT for takeoff in 5 minutes
        time = -length + 5 * CampaignMinutes;
        length = SetWPTimes(w, time, 0);
        // Adjust our Package's times as well, so additional flights are coordinated
        SetWPTimes(pack->GetIngress(), time, 0);
        SetWPTimes(pack->GetEgress(), time, 0);
        pack->GetMissionRequest()->tot += time;
        mis->tot += time;
    }

    length += 2 * RESERVE_MINUTES * CampaignMinutes;

    // Schedule our takeoff time with the ATM, or adjust if necessary,
    // and set package takeoff time if it's earlier than that listed.
    if (mission not_eq AMIS_ALERT)
    {
        int timedelta;
        timedelta = TeamInfo[GetTeam()]->atm->FindTakeoffSlot(squad->GetUnitAirbaseID(), w);

        if (timedelta == 0xFFFFFFFF)
        {
            if ( not (mis->flags bitand REQF_ALLOW_ERRORS))
                return PRET_CANCELED;
        }
        else
            length += timedelta;

        if (pack->GetTakeoff() > w->GetWPArrivalTime() or not pack->GetTakeoff())
            pack->SetTakeoff(w->GetWPArrivalTime());
    }

    // Set our time_on_target and mission_over_time from our waypoints
    i = 0;

    while (w)
    {
        if (w->GetWPFlags() bitand WPF_TARGET or w->GetWPFlags() bitand WPF_CP)
        {
            if ( not i)
                time_on_target = w->GetWPArrivalTime();

            mission_over_time = w->GetWPArrivalTime();
            i++;
        }

        if (w->GetWPFlags() bitand WPF_REPEAT and w->GetWPDepartureTime() > mission_over_time)
            mission_over_time = w->GetWPDepartureTime();

        w = w->GetNextWP();
    }

    // RV - Biker - Check if we've set some bullshit WP altitude
    float x, y, z;
    float groundAlt;
    int adjustedAlt;

    // Loop through all WPs
    w = GetFirstUnitWP();

    while (w)
    {
        w->GetLocation(&x, &y, &z);
        // Altitude from this is with pos. sign
        groundAlt = TheMap.GetMEA(x, y);

        // Check if we're below ground level (don't adjust for landing and takeoff WPs?)
        if (-z <= groundAlt and w->GetWPAction() not_eq WP_LAND and w->GetWPAction() not_eq WP_TAKEOFF)
        {
            // Maybe do something diff for choppers or AC?
            adjustedAlt = int((groundAlt + 250) / 500) * 500 + 500;
            w->SetWPAltitude(adjustedAlt);
            w->GetLocation(&x, &y, &z);
        }

        w = w->GetNextWP();
    }

    // Arm and fuel flight
    // KCK TODO: Calculate altitude adjusted burn rate..
    fuelNeeded = ((int)(length / CampaignMinutes) * class_data->Rate); // lbs of fuel needed
    fuelAvail = CalculateFuelAvailable(255);

    if (fuelNeeded > class_data->Fuel)
    {
        // Not enough fuel to complete mission - load fuel tanks until we have enough or
        // can't load anymore.
        while (fuelNeeded > fuelAvail and LoadWeapons(squad, DefaultDamageMods, NoMove, 2, WEAP_FUEL, 0))
            fuelAvail = CalculateFuelAvailable(255);

        // 2001-10-16 REMOVED by M.N. Not needed here -> The AddTankerWaypoints function will decide
        // Check if we're still way out of range
        // if ( not (mis->flags bitand REQF_ALLOW_ERRORS) and fuelNeeded > fuelAvail + fuelAvail/2)
        // return PRET_CANCELED;
        // Otherwise require tankers
        if (fuelNeeded > fuelAvail and not (MissionData[mis->mission].flags bitand AMIS_FUDGE_RANGE))
        {
            pack->SetPackageFlags(AMIS_ADDTANKER bitor AMIS_NEEDTANKER);
            refuel = fuelNeeded - fuelAvail; // We use this for AddTankerWayPoints
        }
    }

    if (mis->mission == AMIS_AIRCAV)
    {
        SetCargoId(mis->requesterID);
    }
    else
    {
        SetCargoId(FalconNullId);
    }

    if (mission not_eq AMIS_ALERT)
    {
        // Mark our takeoff and landing times as being used
        // KCK WARNING: if this flight later get's the axe, this slot will still be marked full
        TeamInfo[GetTeam()]->atm->ScheduleAircraft(squad->GetUnitAirbaseID(), wp_list, mis->aircraft);
    }

    // KCK Hack to allow re-plans: Finish here if we've already been inserted
    if (VuState() == VU_MEM_ACTIVE)
    {
        return PRET_SUCCESS;
    }

    // Find information about our expected target
    mt = NoMove;
    damageMods = DefaultDamageMods;

    // 2001-07-10 MODIFIED BY S.G. SEAD ESCORT USES THE DEFAULT MT/DAMAGEMOD INSTEAD OF THE ONE BASED ON ITS TARGET
    // if (target)
    if (target and mission not_eq AMIS_SEADESCORT)
    {
        if (target->IsUnit() and not ((Unit)target)->Real())
            target = ((Unit)target)->GetFirstUnitElement();

        if (target)
        {
            damageMods = target->GetDamageModifiers();
            mt = target->GetMovementType();

            if (target->IsObjective() and mis->target_num < 255)
            {
                FeatureClassDataType* fc;
                fc = GetFeatureClassData(((Objective)target)->GetFeatureID(mis->target_num));

                if (fc)
                    damageMods = fc->DamageMod;
            }
        }
    }


    // ADDED BY MN - SEAD strikes against SAM's shouldn't get AGM-88 loaded
    // when there is no radar target anymore - worthless weapons if we have fuel tanks loaded
    // sfr: added IsBattalion check. How can you be so sure its a batallion??
    bool hasRadarVehicle = false;

    if (
        target and target->IsBattalion() and 
        ((BattalionClass *)target)->class_data->RadarVehicle < 255 /* and e->IsEmitting()*/
    )  // current emitting status is not interesting here..
    {
        // If the radar vehicle index is less than 16, it's a SAM. Check if it still has a radar vehicle
        if (((BattalionClass *)target)->class_data->RadarVehicle < 16)
        {
            if (((BattalionClass *)target)->GetNumVehicles(((BattalionClass *)target)->class_data->RadarVehicle))
            {
                hasRadarVehicle = true;
            }
        }
    } // this makes it so that AAA targets are only engaged with CBU's.

    // Load mission specific stuff here
    switch (GetUnitMission())
    {
        case AMIS_SEADSTRIKE:
            ShiAssert(mission_target not_eq FalconNullId);

            if ( not hasRadarVehicle)
            {
                if (rand() bitand 1)
                    LoadWeapons(squad, damageMods, NoMove, 98, WEAP_DEAD_LOADOUT, 0);
                else
                    LoadWeapons(squad, damageMods, Tracked, 98, WEAP_DEAD_LOADOUT, 0);

                LoadWeapons(squad, damageMods, mt, 98, WEAP_CLUSTER, 0);
                needweaps = 1;
                break;
            }
            // RV - Biker - Check if we are main flight then do SEAD else DEAD
            else if (pack->GetMainFlight() == NULL)
            {
                LoadWeapons(squad, damageMods, mt, 6, 0, WEAP_ANTIRADATION);
                // RV - Biker - Remove this so we can make other ACs for DEAD
                //LoadWeapons(squad, damageMods, mt, 98, WEAP_CLUSTER, 0);
                needweaps = 1;
                break;
            }
            else
            {
                if (rand() bitand 1)
                    LoadWeapons(squad, damageMods, NoMove, 98, WEAP_DEAD_LOADOUT, 0);
                else
                    LoadWeapons(squad, damageMods, Tracked, 98, WEAP_DEAD_LOADOUT, 0);

                LoadWeapons(squad, damageMods, mt, 98, WEAP_CLUSTER, 0);
                LoadWeapons(squad, damageMods, mt, 2, 0, WEAP_ANTIRADATION);
                needweaps = 1;
                break;
            }

        case AMIS_SEADESCORT:
            LoadWeapons(squad, damageMods, mt, 6, 0, WEAP_ANTIRADATION);
            // RV - Biker - Remove this so we can make other ACs for DEAD
            //LoadWeapons(squad, damageMods, mt, 98, WEAP_CLUSTER, 0);
            needweaps = 1;
            break;

        case AMIS_OCASTRIKE:
            LoadWeapons(squad, damageMods, mt, 6, 0, 0);//Cobra
            ShiAssert(mission_target not_eq FalconNullId);
            needweaps = 1;
            break;

        case AMIS_INTSTRIKE:
        case AMIS_STRIKE:
        case AMIS_DEEPSTRIKE:
        case AMIS_STSTRIKE:
            //LoadWeapons(squad, damageMods, mt, 2, 0, 0); Cobra TJL 11/23 allows larger loadouts
            LoadWeapons(squad, damageMods, mt, 4, 0, 0);
            ShiAssert(mission_target not_eq FalconNullId);
            needweaps = 1;
            break;

        case AMIS_STRATBOMB:
            LoadWeapons(squad, damageMods, mt, 98, 0, WEAP_DUMB_ONLY);
            ShiAssert(mission_target not_eq FalconNullId);
            needweaps = 1;
            break;

            // RV - Biker - Give FAC some weapons too
        case AMIS_FAC:
            LoadWeapons(squad, damageMods, mt, 2, WEAP_FAC_LOADOUT, 0);
            LoadWeapons(squad, damageMods, Tracked, 98, WEAP_BAI_LOADOUT, 0);
            needweaps = 0;
            break;

        case AMIS_ONCALLCAS:
        case AMIS_PRPLANCAS:
        case AMIS_SAD:
        case AMIS_INT:
        case AMIS_BAI:
            LoadWeapons(squad, damageMods, mt, 98, WEAP_BAI_LOADOUT, 0);
            needweaps = 1;
            break;

        case AMIS_ASW:

            // RV - Biker - Now this is DEAD mission
            if (rand() bitand 1)
            {
                LoadWeapons(squad, DefaultDamageMods, NoMove, 98, WEAP_DEAD_LOADOUT, 0);
            }
            else
            {
                LoadWeapons(squad, DefaultDamageMods, Tracked, 98, WEAP_DEAD_LOADOUT, 0);
            }

            //LoadWeapons(squad, DefaultDamageMods, mt, 98, WEAP_CLUSTER, 0);
            needweaps = 1;
            SetUnitMission(AMIS_SEADSTRIKE);
            break;

        case AMIS_ASHIP:
            LoadWeapons(squad, damageMods, mt, 98, 0, 0);
            needweaps = 1;
            break;

        case AMIS_RECON:
        case AMIS_BDA:
        case AMIS_PATROL:
        case AMIS_RECONPATROL:
            LoadWeapons(squad, damageMods, NoMove, 1, WEAP_RECON, 0);
            needweaps = 0;
            break;

        case AMIS_BARCAP:
        case AMIS_BARCAP2:
        case AMIS_HAVCAP:
        case AMIS_TARCAP:
        case AMIS_RESCAP:
        case AMIS_AMBUSHCAP:
        case AMIS_SWEEP:
        case AMIS_ALERT:
        case AMIS_INTERCEPT:
        case AMIS_ESCORT:
            needweaps = 0;
            break;

        case AMIS_ECM:
            // RV - Biker - This should work for EA-6B
            LoadWeapons(squad, damageMods, NoMove, 6, WEAP_ECM, 0);
            needweaps = 0;
            break;

        default:
            needweaps = 0;
            break;
    }

    // RV - Biker - Find last HP
    VehicleClassDataType *vc = (VehicleClassDataType*) Falcon4ClassTable[GetVehicleID(0)].dataPtr;

    int lastHP = 0;
    int indexHP = 0;

    if (vc)
    {
        for (indexHP = 0; indexHP < HARDPOINT_MAX; indexHP++)
        {
            if (vc->Weapon[indexHP])
                lastHP = indexHP;
        }
    }
    else
    {
        lastHP = HARDPOINT_MAX;
    }

    int emptyHPs = 0;

    for (int i = 0; i < loadouts; i++)
    {
        // Don't count gun (HP 0)
        for (int hp = 1; hp <= lastHP; hp++)
        {
            if (loadout[i].WeaponID[hp] == 0)
                emptyHPs++;
        }
    }

    // RV - Biker - Jammers do overwrite all other weapons except fuel tanks and recon cams
    // don't load AA missiles for SAR choppers
    if (GetUnitMission() not_eq AMIS_SAR)
    {
        // RV - Biker - Load heat seekers more often
        if (needweaps == 1 or rand() bitand 1)
        {
            LoadWeapons(squad, DefaultDamageMods, Air, 2, 0, WEAP_RADAR);
            LoadWeapons(squad, DefaultDamageMods, Air, 4, 0, WEAP_HEATSEEKER);
        }

        LoadWeapons(squad, DefaultDamageMods, Air, 98, 0, 0);
    }

    // Check for internal jamming
    if (GetVehicleClassData(class_data->VehicleType[0])->Flags bitand VEH_HAS_JAMMER)
    {
        SetHasECM(1);
    }
    // Load jamming pod, if possible
    else if (LoadWeapons(squad, damageMods, NoMove, 1, WEAP_ECM, 0))
    {
        SetHasECM(1);
    }

    // RV - Biker - Load Chaff Flare pods
    LoadWeapons(squad, damageMods, NoMove, 1, WEAP_CHAFF_POD, 0);

    // RV - Biker - Check if we have some laser guided bombs loaded
    for (int i = 0; i < loadouts; i++)
    {
        // Don't count gun (HP 0)
        for (int hp = 1; hp <= lastHP; hp++)
        {
            if (WeaponDataTable[loadout[i].WeaponID[hp]].GuidanceFlags == WEAP_LASER)
            {
                // Here load laser pod...
                LoadWeapons(squad, damageMods, NoMove, 1, WEAP_LASER_POD, 0);
                break;
            }
        }
    }

    // RV - Biker - Check if we have some GPS guided bombs loaded
    if (GetUnitMission() == AMIS_ONCALLCAS or
        GetUnitMission() == AMIS_PRPLANCAS or
        GetUnitMission() == AMIS_SAD or
        GetUnitMission() == AMIS_INT or
        GetUnitMission() == AMIS_BAI or
        GetUnitMission() == AMIS_ASW)
    {
        for (int i = 0; i < loadouts; i++)
        {
            // Don't count gun (HP 0)
            for (int hp = 1; hp <= lastHP; hp++)
            {
                if (WeaponDataTable[loadout[i].WeaponID[hp]].Flags bitand WEAP_BOMBGPS)
                {
                    // Here load laser pod...
                    LoadWeapons(squad, damageMods, NoMove, 1, WEAP_LASER_POD, 0);
                    break;
                }
            }
        }
    }

    // RV - Biker - Check if we have some ground weapons when on AG mission
    int AGweaps = 0;

    if (needweaps == 1)
    {
        for (int i = 0; i < loadouts; i++)
        {
            // Don't count gun (HP 0)
            for (int hp = 1; hp <= lastHP; hp++)
            {
                AGweaps += GetWeaponHitChance(loadout[i].WeaponID[hp], NoMove);
                AGweaps += GetWeaponHitChance(loadout[i].WeaponID[hp], Foot);
                AGweaps += GetWeaponHitChance(loadout[i].WeaponID[hp], Wheeled);
                AGweaps += GetWeaponHitChance(loadout[i].WeaponID[hp], Tracked);
                AGweaps += GetWeaponHitChance(loadout[i].WeaponID[hp], Naval);
                AGweaps += GetWeaponHitChance(loadout[i].WeaponID[hp], Rail);
            }
        }
    }

    if (needweaps == 1 and AGweaps == 0)
    {
        return PRET_CANCELED;
    }

    // Last minute fixup for mission types for variety
    if (MissionData[mis->mission].skill == ARO_S and not mis->action_type)
    {
        if (squad->class_data->Flags bitand VEH_STEALTH)
            SetUnitMission(AMIS_STSTRIKE);
        else if (DistanceToFront(mis->tx, mis->ty) > 100.0F)
            SetUnitMission(AMIS_DEEPSTRIKE);
    }

    // Transfer aircraft, update scheduling and insert flight
    squad->ScheduleAircraft(this, mis);

    if ( not GetRoster())
    {
        return PRET_NO_ASSETS;
    }

    last_move = TheCampaign.CurrentTime;
    SetSendCreate(VuEntity::VU_SC_SEND_OOB);
    vuDatabase->/*Quick*/Insert(this);

    // Steal our weapons/fuel from the squadron
    squad->UpdateSquadronStores(loadout[0].WeaponID, loadout[0].WeaponCount, fuelAvail, GetTotalVehicles());

    return PRET_SUCCESS;
}

int FlightClass::LoadWeapons(void *squad, uchar *dam, MoveType mt, int num, int type_flags, int guide_flags)
{
    if ( not loadouts)
    {
        LoadoutStruct *load = new LoadoutStruct;
        SetLoadout(load, 1);
    }

    return ::LoadWeapons(squad, GetVehicleID(0), dam, mt, num, type_flags, guide_flags, loadout[0].WeaponID, loadout[0].WeaponCount);
}

int FlightClass::CollectWeapons(uchar* dam, MoveType m, short w[], uchar wc[], int dist)
{
    int i, ac, bw, hp, bhp, lhp = 0, maxCount, shots = 1, dropTwo = 0, next = 0, vehsPerRound = 1, rounds;
    VehicleClassDataType* vc;

    vc = GetVehicleClassData(class_data->VehicleType[0]);

    // If we're shooting at a ground unit, take two shots for each vehicle if we can
    // (Must be two shots with same weapon type)
    if (MOVE_GROUND(m) or m == NoMove)
        dropTwo = TRUE;

    rounds = GetNumberOfLoadouts();

    if (rounds == 1)
        vehsPerRound = GetTotalVehicles();

    // Now collect our weapons (per ac)
    for (ac = 0; ac < rounds; ac++)
    {
        if (plane_stats[ac] == AIRCRAFT_AVAILABLE or rounds == 1)
        {
            bw = GetBestVehicleWeapon(ac, dam, m, dist, &hp);

            if (bw > 0)
            {
                // Find the hardpoint with the most of this type of weapon)
                for (i = 0, bhp = 0, maxCount = 0; i < HARDPOINT_MAX; i++)
                {
                    if (bw == loadout[ac].WeaponID[i] and loadout[ac].WeaponCount[i] > maxCount)
                    {
                        maxCount = loadout[ac].WeaponCount[i];
                        bhp = i;
                    }

                    if (loadout[ac].WeaponID[i])
                        lhp = i;
                }

                // Figure out how many to shoot
                if (maxCount)
                {
                    // 2001-05-01 MODIFIED BY S.G. SO VisibleFlags STATE IS REVERSED (SET IS EXTERNAL AND CLEARED IS INTERNAL)
                    // if (bhp and (vc->VisibleFlags bitand (0x01 << bhp)))
                    if (bhp and not (vc->VisibleFlags bitand (0x01 << bhp)))
                        shots = maxCount; // Bomb-bay - drop everything at once
                    else if (GetWeaponFireRate(bw) <= maxCount)
                        shots = GetWeaponFireRate(bw); // Fire a volley
                    else
                        shots = 1; // Drop one

                    // Use up the ammo
                    loadout[ac].WeaponCount[bhp] -= shots;
                    w[next] = bw;
                    wc[next] = shots * vehsPerRound;

                    // Try to drop another bomb/weapon if we're loaded symetrically
                    // 2001-06-17 ADDED BY S.G. IF FIRING HARMS, ONLY ONE SHOT PER VEHICLE
                    if (WeaponDataTable[bw].GuidanceFlags bitand WEAP_ANTIRADATION)
                        dropTwo = FALSE;

                    // END OF ADDED SECTION
                    if (dropTwo and loadout[ac].WeaponID[lhp + 1 - bhp] == bw and loadout[ac].WeaponCount[lhp + 1 - bhp] >= shots)
                    {
                        wc[next] += shots * vehsPerRound;
                        loadout[ac].WeaponCount[lhp + 1 - bhp] -= shots;
                    }

                    next++;
                }
            }
        }
    }

    return 1;
}

#if 1 // Cobra version - FRB
F4PFList FlightClass::GetKnownEmitters(void)
{
    GridIndex x, y, fx, fy, nx, ny, ex, ey;
    float d, xd, yd;
    int step, dist;
    WayPoint w, nw;
    Team us;
    CampEntity e;
    F4PFList emit = new FalconPrivateList(&CampFilter);
    uchar added[MAX_CAMP_ENTITIES]; // Search data

    emit->Register();
    memset(added, 0, MAX_CAMP_ENTITIES);
    w = GetFirstUnitWP();
    nw = w->GetNextWP();
    us = GetTeam();

    while (w and nw)
    {
        w->GetWPLocation(&fx, &fy);
        nw->GetWPLocation(&nx, &ny);
        d = Distance(fx, fy, nx, ny);
        xd = (float)(nx - fx) / d;
        yd = (float)(ny - fy) / d;
        dist = FloatToInt32(d);

        for (step = 0; step <= dist; step += 10)
        {
            x = fx + (GridIndex)(xd * step + 0.5F);
            y = fy + (GridIndex)(yd * step + 0.5F);
            VuListIterator myit(EmitterList);
            e = (CampEntity) myit.GetFirst();

            //Cobra we will try to make this match the update function
            while (e)
            {
                if (
                    e->GetTeam() not_eq us and e->GetSpotted(us) and 
                    ( not e->IsUnit() or not ((Unit)e)->Moving()) and not added[e->GetCampID()] and 
                    e->GetElectronicDetectionRange(Air)
                )
                {
                    e->GetLocation(&ex, &ey);

                    if (Distance(ex, ey, x, y) < 200 /*ADD_TO_KNOWN_EMITTER_DIST*/)
                    {
                        emit->ForcedInsert(e);
                        added[e->GetCampID()]++;
                    }
                }

                e = (Unit) myit.GetNext();
            }
        }

        w = nw;
        nw = w->GetNextWP();
    }

    return emit;
}

#else
// RV version
F4PFList FlightClass::GetKnownEmitters(void)
{
    GridIndex x, y, fx, fy, nx, ny, ex, ey;
    float d, xd, yd;
    int step, dist;
    WayPoint w, nw;
    Team us;
    CampEntity e;
    F4PFList emit = new FalconPrivateList(&CampFilter);
    uchar added[MAX_CAMP_ENTITIES]; // Search data

    emit->Register();
    memset(added, 0, MAX_CAMP_ENTITIES);
    w = GetFirstUnitWP();
    nw = w->GetNextWP();
    us = GetTeam();

    // RV - Biker - New preplanned threats
    Flight jstar = GetJSTARFlight();
    int jstarDetectionChance = 0;
    float jstarDist = 500.0f;
    CampaignTime lastMove = 0;

    // If we didn't find JSTAR do other search
    if ( not jstar)
    {
        Unit nu, cf;
        VuListIterator new_myit(AllAirList);
        nu = (Unit) new_myit.GetFirst();

        while (nu and not jstar)
        {
            cf = nu;
            nu = (Unit) new_myit.GetNext();

            if ( not cf->IsFlight() or cf->IsDead())
                continue;

            // RV - Biker - Only those JSTAR flights which are in the air before we do takeoff can provide data
            if (cf->GetUnitMission() == AMIS_JSTAR and cf->GetTeam() == us and cf->GetUnitTOT() < Camp_GetCurrentTime())
            {
                jstar = (Flight)cf;
            }
        }
    }

    // RV - Biker - Loop through all our waypoints
    while (w and nw)
    {
        w->GetWPLocation(&fx, &fy);
        nw->GetWPLocation(&nx, &ny);
        d = Distance(fx, fy, nx, ny);
        xd = (float)(nx - fx) / d;
        yd = (float)(ny - fy) / d;
        dist = FloatToInt32(d);

        for (step = 0; step <= dist; step += 10)
        {
            x = fx + (GridIndex)(xd * step + 0.5F);
            y = fy + (GridIndex)(yd * step + 0.5F);
            VuListIterator myit(EmitterList);

            for (e = (CampEntity) myit.GetFirst(); e; e = (CampEntity) myit.GetNext())
            {
                if (e->IsObjective())
                {
                    continue;
                }

                // RV - Biker - Check if unit did move within last hour
                if (e->IsBattalion())
                {
                    if (SimLibElapsedTime > TheCampaign.GetTEStartTime() + 60 * CampaignMinutes)
                    {
                        lastMove = (Camp_GetCurrentTime() - ((BattalionClass*)e)->last_move) / CampaignHours;
                    }
                    else
                    {
                        lastMove = 10;
                    }

                    // FRB - TE's Units have always moved when started
                    if (FalconLocalGame and FalconLocalGame->GetGameType() == game_TacticalEngagement)
                        lastMove = 100;
                }

                // RV - Biker - If this is our target add it for sure
                if (mission == AMIS_SEADSTRIKE and e->GetTeam() not_eq us and e->IsBattalion() and e->Id() == requester)
                {
                    emit->ForcedInsert(e);
                    added[e->GetCampID()]++;
                    continue;
                }

                if (jstar)
                {
                    // This is a ELINT flight (e.g. RC-135)
                    if (jstar->class_data->Role == ROLE_ELINT)
                        jstarDetectionChance = 75;
                    else

                        // FRB - Give JSTAR SAM finder capabilities
                        if ( not g_bUseRC135)
                            jstarDetectionChance = 75;
                        else
                            jstarDetectionChance = 25;
                }

                if (e->GetTeam() not_eq us and e->GetSpotted(us) and ( not e->IsUnit() or not ((Unit)e)->Moving()) and not added[e->GetCampID()] and e->GetElectronicDetectionRange(Air))
                {
                    // RV - Biker - Those units which did move within the last hour only are detectable via JSTAR/ELINT
                    if (e->IsUnit() and e->IsBattalion() and lastMove == 0 and ((UnitClass*)e)->GetMaxSpeed() > 0 and rand() % 100 > jstarDetectionChance)
                    {
                        continue;
                    }
                    else
                    {
                        e->GetLocation(&ex, &ey);

                        if (Distance(ex, ey, x, y) < 200)
                        {
                            emit->ForcedInsert(e);
                            added[e->GetCampID()]++;
                        }
                    }
                }
            }
        }

        w = nw;
        nw = w->GetNextWP();
    }

    return emit;
}
#endif

void FlightClass::GetUnitAssemblyPoint(int type, GridIndex *x, GridIndex *y)
{
    Package p;

    p = (Package)GetUnitParent();

    if (p)
        p->GetUnitAssemblyPoint(type, x, y);
    else
        *x = *y = 0;
}

int FlightClass::GetBestVehicleWeapon(int ac, uchar *dam, MoveType mt, int range, int *hp)
{
    int i, str, bs, w, bw, bhp = -1;
    VehicleClassDataType* vc;

    if (ac > GetNumberOfLoadouts())
        ac = 0;

    vc = GetVehicleClassData(class_data->VehicleType[0]);
    ShiAssert(vc);

    bw = bs = 0;

    for (i = 0; i < HARDPOINT_MAX; i++)
    {
        w = GetUnitWeaponId(i, ac);

        if (w and GetUnitWeaponCount(i, ac))
        {
            str = GetWeaponScore(w, dam, mt, range);

            if (str > bs)
            {
                bw = w;
                bs = str;
                bhp = i;
            }
        }
    }

    *hp = bhp;
    return bw;
}

void FlightClass::UseFuel(long f)
{
    fuel_burnt += f;
}

// 2001-04-02 HELPER BY S.G.
// Will return true if it can use/set GCI
int CheckValidType(CampEntity u, CampEntity e)
{
    // If enemy is not a flight, GCI is automatic
    if ( not e->IsFlight())
        return TRUE;

    // If we're not a unit, GCI is automatic
    if ( not u->IsUnit())
        return TRUE;

    // Special case if we're a flight
    if (u->IsFlight())
    {
        // AWACS uses/sets GCI always
        if (u->GetSType() == STYPE_UNIT_AWACS)
            return TRUE;

        // Against air enemy, only fighter and fighter.bomber can use GCI
        if (u->GetSType() not_eq STYPE_UNIT_FIGHTER and u->GetSType() not_eq STYPE_UNIT_FIGHTER_BOMBER)
            return FALSE;

    }
    // Only air defense battalions uses/sets GCI
    else if (u->IsBattalion())
    {
        if (u->GetSType() not_eq STYPE_UNIT_AIR_DEFENSE)
            return FALSE;
    }
    // 2002-02-11 ADDED BY S.G. Only carrier and battleships uses/sets GCI
    else if (u->IsTaskForce())
    {
        if (u->GetSType() not_eq STYPE_UNIT_BATTLESHIP and u->GetSType() not_eq STYPE_UNIT_CARRIER)
            return FALSE;
    }

    // If we get here, we're a battalion or a valid flight stype

    // If our moral is broken, no GCI
    if (((UnitClass *)u)->Broken())
        return FALSE;

    // If skill is below the threshold, don't use/set GCI
    // 80 is cadets, 90 is veteran and 100 is ace.
    // Cadets have 25% chance, veteran 50% chance and ace 100% chance of using/setting GCI
    if (TeamInfo[u->GetTeam()] and TeamInfo[u->GetTeam()]->airExperience < 75 + rand() % 20)
        return FALSE;

    return TRUE;
}

/* 2001-03-31 COMMENTED OUT BY S.G. TOO MANY CHANGES TO TRACK THEM ALL
   int FlightClass::DetectVs (AircraftClass *ac, float *d, int *combat, int *spot, int *estr)
   {
   int react,det = Detected(this,ac,d);
   CampEntity e;

   if ( not (det bitand REACTION_MASK))
   return 0;

 *spot = 0;
 e = ac->GetCampaignObject();
 react = Reaction(e,det,*d);
 if (det bitand ENEMY_IN_RANGE and react)
 *combat = 1;
 if (det bitand FRIENDLY_DETECTED)
 {
 if ( not GetSpotted(e->GetTeam()))
 RequestIntercept(this, e->GetTeam());
 SetSpotted(e->GetTeam(),TheCampaign.CurrentTime);
 }
 return react;
 }
 */

int FlightClass::DetectVs(AircraftClass *ac, float *d, int *combat, int *spot, int *estr)
{
    int react, det;
    CampEntity e;

    *spot = 0;

    det = Detected(this, ac, d);

    e = ac->GetCampaignObject();

    // 2001-03-22 ADDED BY S.G. DETECTION DOESN'T INCLUDED SPOTTED,
    // ONLY THAT THIS ENTITY DETECTED THE OTHER BY ITSELF.
    int detTmp = det;

    // Check type of entity before GCI is used
    if (CheckValidType(this, e))
    {
        detTmp or_eq e->GetSpotted(GetTeam()) ? ENEMY_DETECTED : 0;
    }

    // Check type of entity before GCI is used
    if (CheckValidType(e, this))
    {
        detTmp or_eq GetSpotted(e->GetTeam()) ? FRIENDLY_DETECTED : 0;
    }

    // Use our temp detection mask which possibly includes GCI
    if ( not (detTmp bitand REACTION_MASK))
    {
        return 0;
    }

    // Reaction gets to use GCI as well
    react = Reaction(e, detTmp, *d);

    if (det bitand ENEMY_IN_RANGE and react)
    {
        *combat = 1;
    }

    if (det bitand FRIENDLY_DETECTED)
    {
        // Spotting will be set only if our enemy is aggregated or if he's an AWAC.
        // SensorFusion or GroundClass::Exec will hanlde deaggregated vehicles.
        if ((e->IsAggregate() and CheckValidType(e, this)) or (e->IsFlight() and e->GetSType() == STYPE_UNIT_AWACS))
        {
            if ( not GetSpotted(e->GetTeam()))
            {
                RequestIntercept(this, e->GetTeam());
            }

            // 2002-02-11 MODIFIED BY S.G. Added 'CanItIdentify' which query if the target can be identified
            SetSpotted(e->GetTeam(), TheCampaign.CurrentTime, CanItIdentify(this, e, *d, ac->GetMovementType()));
        }
    }

    return react;
}

/* 2001-03-31 COMMENTED OUT BY S.G. TOO MANY CHANGES TO TRACK THEM ALL
   int FlightClass::DetectVs (CampEntity e, float *d, int *combat, int *spot, int *estr)
   {
   int react,det;

 *spot = 0;
 det = Detected(this,e,d);
 if ( not (det bitand REACTION_MASK))
 return 0;
 react = Reaction(e,det,*d);
 if (det bitand ENEMY_DETECTED)
 {
 if ( not e->GetSpotted(GetTeam()))
 {
// Only mark as spotted if it's a new contact
 *spot = 1;
 }
 e->SetSpotted(GetTeam(),TheCampaign.CurrentTime);
 }
 if (det bitand ENEMY_IN_RANGE and react)
 *combat = 1;
 if (det bitand FRIENDLY_DETECTED)
 {
 if ( not GetSpotted(e->GetTeam()))
 RequestIntercept(this, e->GetTeam());
 SetSpotted(e->GetTeam(),TheCampaign.CurrentTime);
// Enemy's air strength added to enemy strength if they've got us 'locked'
if (e->IsUnit() and ((Unit)e)->GetTargetID() == Id())
 *estr += GetUnitScore ((Unit)e, Air);
 }
 return react;
 }
 */
int FlightClass::DetectVs(CampEntity e, float *d, int *combat, int *spot, int *estr)
{
    int react, det;

    *spot = 0;
    det = Detected(this, e, d);

    int detTmp = det;

    // Check type of entity before GCI is used
    if (CheckValidType(this, e))
        detTmp or_eq e->GetSpotted(GetTeam()) ? ENEMY_DETECTED : 0;

    // Check type of entity before GCI is used
    if (CheckValidType(e, this))
        detTmp or_eq GetSpotted(e->GetTeam()) ? FRIENDLY_DETECTED : 0;

    // Use our temp detection mask which possibly includes GCI
    if ( not (detTmp bitand REACTION_MASK))
        return 0;

    // Don't react if on a air to ground mission, even if within MinIngoreRange unless you're spotted.
    if (
        e->IsFlight() /* and not (eval_flags bitand FEVAL_GOT_TO_TARGET) */ and 
        GetUnitMission() >= AMIS_SEADSTRIKE and 
        GetUnitMission() <= AMIS_ECM and 
        (*d > MIN_IGNORE_RANGE or not GetSpotted(e->GetTeam()))
    )
    {
        react = 0;
    }
    else
    {
        // Reaction gets to use GCI as well
        react = Reaction(e, detTmp, *d);
    }

    // Spotting will be set only if we're aggregated or if we're an AWAC.
    // SensorFusion will handle spotting for deaggregated flights
    // I can't let SensorFusion handle the spotting for AWAC because this will put a too big toll on the CPU
    if (det bitand ENEMY_DETECTED)
    {
        if ((IsAggregate() and CheckValidType(this, e)) or GetSType() == STYPE_UNIT_AWACS)
        {
            if ( not e->GetSpotted(GetTeam()))
            {
                // Only mark as spotted if it's a new contact
                *spot = 1;
            }

            // 2002-02-11 MODIFIED BY S.G. Added 'CanItIdentify' which query if the target can be identified
            e->SetSpotted(GetTeam(), TheCampaign.CurrentTime, CanItIdentify(this, e, *d, e->GetMovementType()));
        }
    }

    if (det bitand ENEMY_IN_RANGE and react)
    {
        *combat = 1;
    }

    if (det bitand FRIENDLY_DETECTED)
    {
        // Spotting will be set only if our enemy is aggregated or if he's an AWAC.
        // SensorFusion or GroundClass::Exec will hanlde deaggregated vehicles.
        if ((e->IsAggregate() and CheckValidType(e, this)) or (e->IsFlight() and e->GetSType() == STYPE_UNIT_AWACS))
        {
            if ( not GetSpotted(e->GetTeam()))
            {
                RequestIntercept(this, e->GetTeam());
            }

            // 2002-02-11 MODIFIED BY S.G. Added 'CanItIdentify' which query if the target can be identified
            SetSpotted(e->GetTeam(), TheCampaign.CurrentTime, CanItIdentify(e, this, *d, this->GetMovementType()));
        }

        // Enemy's air strength added to enemy strength if they've got us 'locked'
        if (e->IsUnit() and ((Unit)e)->GetTargetID() == Id())
        {
            *estr += GetUnitScore((Unit)e, Air);
        }
    }

    return react;
}

int FlightClass::PickRandomPilot(int seed)
{
    int pilot, tries = 0;

    // JPO - we need to have one less than the max array size
    if ( not seed)
        pilot = rand() % (PILOTS_PER_FLIGHT - 1);
    else
        pilot = seed % (PILOTS_PER_FLIGHT - 1);

    // JB 010121
    if ( not plane_stats or not player_slots)
        return pilot;

    if (F4IsBadReadPtr(plane_stats, sizeof(uchar)) or F4IsBadReadPtr(player_slots, sizeof(uchar))) // JB 010317 CTD
        return pilot;

    while ((plane_stats[pilot] not_eq AIRCRAFT_AVAILABLE or player_slots[pilot] < 255) and tries < PILOTS_PER_FLIGHT)
    {
        pilot = (pilot + 1) % (PILOTS_PER_FLIGHT - 1);
        tries++;
    }

    return pilot;
}

int FlightClass::GetAdjustedPlayerSlot(int pslot)
{
    int i;

    if (pslot < PILOTS_PER_FLIGHT)
        return pslot;

    for (i = 0; i < PILOTS_PER_FLIGHT; i++)
    {
        if (player_slots[i] == pslot)
            return i;
    }

    ShiAssert( not "We failed to adjust a player pilot slot");
    return 0;
}

PilotClass* FlightClass::GetPilotData(int pilot_slot)
{
    Squadron squad = (Squadron)GetUnitSquadron();

    if ( not squad or pilot_slot >= PILOTS_PER_FLIGHT or pilots[pilot_slot] > PILOTS_PER_SQUADRON)
        return NULL;

    return squad->GetPilotData(pilots[pilot_slot]);
}

int FlightClass::GetPilotID(int pilot_slot)
{
    Squadron squad = (Squadron)GetUnitSquadron();

    if ( not squad or pilot_slot >= PILOTS_PER_FLIGHT or pilots[pilot_slot] > PILOTS_PER_SQUADRON)
        return 0;
    else
        return squad->GetPilotID(pilots[pilot_slot]);
}

int FlightClass::GetPilotCallNumber(int pilot_slot)
{
    pilot_slot = GetAdjustedPlayerSlot(pilot_slot);
    return (callsign_num - 1) * 4 + pilot_slot + 1;
}

uchar FlightClass::GetPilotVoiceID(int pilot_slot)
{
    while (pilot_slot < PILOTS_PER_FLIGHT and pilots[pilot_slot] == NO_PILOT)
        pilot_slot++;

    if (pilot_slot >= PILOTS_PER_FLIGHT)
        return 1;

    int pilot_id = GetPilotID(pilot_slot);

    if (PilotInfo[pilot_id].voice_id == 255)
        PilotInfo[pilot_id].AssignVoice(GetOwner());

    ShiAssert(PilotInfo[pilot_id].voice_id not_eq 255);

    return (uchar)PilotInfo[pilot_id].voice_id;
}

// Returns slot of flightleader
int FlightClass::GetFlightLeadSlot(void)
{
    int pilot = 0;

    // KCK: This line was previously not recognizing a player as being a valid flight lead.
    // This should fix the problem.
    // while ((plane_stats[pilot] not_eq AIRCRAFT_AVAILABLE or player_slots[pilot] < 255) and pilot < PILOTS_PER_FLIGHT)
    while (plane_stats[pilot] not_eq AIRCRAFT_AVAILABLE and pilot < PILOTS_PER_FLIGHT)
        pilot++;

    return pilot;
}

// Returns the callnumber (1-36) of the flightleader
int FlightClass::GetFlightLeadCallNumber(void)
{
    int callnum, lead = GetFlightLeadSlot();

    callnum = (callsign_num - 1) * 4 + lead + 1;
    return callnum;
}

// Returns the voiceId of the flightleader
uchar FlightClass::GetFlightLeadVoiceID(void)
{
    return GetPilotVoiceID(GetFlightLeadSlot());
}

int FlightClass::GetAdjustedAircraftSlot(int aircraft_num)
{
    // Find the aircraft_num's aircraft in the flight.
    int i;

    for (i = 0; i < PILOTS_PER_FLIGHT; i++)
    {
        if (plane_stats[i] == AIRCRAFT_AVAILABLE)
        {
            if ( not aircraft_num)
                return i;

            aircraft_num--;
        }
    }

    return 255;
}

long FlightClass::CalculateFuelAvailable(int acNum)
{
    // Determine fuel load, given current/UI loadout
    int i;
    UnitClassDataType *uc = GetUnitClassData();
    long fuel, maxFuel = uc->Fuel;

    if (acNum >= loadouts)
        acNum = 0;

    // Find max amount of fuel this plane can carry
    //if (loadout)
    if (loadout and not F4IsBadReadPtr(loadout[0].WeaponID, sizeof(short))) // JB 010326 CTD
    {
        for (i = 0; i < HARDPOINT_MAX; i++)
        {
            if (loadout[acNum].WeaponID[i] and WeaponDataTable[loadout[acNum].WeaponID[i]].Flags bitand WEAP_FUEL)
                maxFuel += WeaponDataTable[loadout[acNum].WeaponID[i]].Strength;
        }
    }

    fuel = maxFuel - fuel_burnt;

    if (fuel < 0)
        return 0;

    return fuel;
}

// Generally speaking, this is set up to be called by ChooseTactic.
// If a non-campaign thread is calling this pretty regularly, I'll have to
// rethink the use of the globals.
int FlightClass::HasWeapons(void)
{
    int weaps = 0, nogun = 1, i, ac, role = GetUnitCurrentRole();
    FalconEntity *e;

    // 2002-03-25 MN if we find out that we don't have the needed weapons later, revert back to our
    // previous mission profile
    int oldmission = ourMission, oldrole = role;


    ourRange = 9999;
    e = GetTarget();

    if (Engaged() and not e)
        SetEngaged(0);

    if (e)
    {
        GridIndex x, y, ex, ey;
        GetLocation(&x, &y);
        e->GetLocation(&ex, &ey);
        ourRange = FloatToInt32(Distance(x, y, ex, ey));
        theirDomain = e->GetDomain();

        if (role not_eq ARO_CA and theirDomain == DOMAIN_AIR)
        {
            if (ourRange < MIN_IGNORE_RANGE)
            {
                // KCK: If we're to close to ignore these aircraft, we've got to engage
                // I'm forcing us to act like a sweep mission until we're out of range.
                // NOTE: One problem may be that aircraft with no air kill capibility
                // may abort even if they have escorts. I may have to special case this.
                role = ARO_CA;
                ourMission = AMIS_SWEEP;
            }
        }
    }

    // If we have a target and are engaged, count guns for purposes of having weapons
    if ((e and Engaged()) or NoAbort())
        nogun = 0;

    for (ac = 0; ac < loadouts; ac++)
    {
        for (i = nogun; i < HARDPOINT_MAX and not weaps; i++)
        {
            if (role == ARO_CA)
            {
                if (loadout[ac].WeaponCount[i] and GetWeaponHitChance(loadout[ac].WeaponID[i], Air))
                    weaps++;
            }
            else if (role == ARO_S or role == ARO_GA or role == ARO_SB or role == ARO_SEAD)
            {
                if (loadout[ac].WeaponCount[i] and GetWeaponHitChance(loadout[ac].WeaponID[i], NoMove))
                    weaps++;
            }
            else if (role == ARO_ASW or role == ARO_ASHIP)
            {
                if (loadout[ac].WeaponCount[i] and GetWeaponHitChance(loadout[ac].WeaponID[i], Naval))
                    weaps++;
            }
            else
                weaps++; // Non-combat roles always 'have weapons'
        }
    }

    // MN continue with our original mission profile if we don't have the needed weapons (like RECONPATROL engaging fighters..)
    if ( not weaps)
    {
        role = oldrole;
        ourMission = oldmission;
    }

    return weaps;
}

int FlightClass::HasFuel(int limit)  // 2002-02-20 MODIFIED BY S.G Instead of simply looking at 3/4, can use an optional modifier.and base 12 instead of 4 for more 'precision'.
{

    limit = min(limit, 12); // Fox Mulder said to trust no one ;-)

    // Check for fuel (we're considered out when we've used 3/4 of our available fuel)
    // if (fuel_burnt < (class_data->Fuel * 3) / 4)
    if (fuel_burnt < class_data->Fuel * limit / 12)
        return 1;

    // Now a more costly check in case we have external tanks
    // if (CalculateFuelAvailable(255) > class_data->Fuel/4)
    if (CalculateFuelAvailable(255) > class_data->Fuel * (12 - limit) / 12)
        return 1;

    return 0;
}

int FlightClass::CanAbort(void)
{
    if ( not HasFuel() or not HasWeapons())
        return 1;

    // Check if off station time
    if ((eval_flags bitand FEVAL_MISSION_STARTED) and (eval_flags bitand FEVAL_GOT_TO_TARGET) and not (eval_flags bitand FEVAL_GOT_TO_TARGET))
        return 1;

    return 0;
}

// 2001-04-03 ADDED BY S.G. THE Standoff jammer GETTER WASN'T DEFINED
Flight FlightClass::GetECMFlight(void)
{
    // If -1, we haven't tried to read the ecmFlightPtr field yet...
    if (ecmFlightPtr == (FlightClass *)(unsigned) - 1)
    {
        // Assign NULL by default on the first read
        ecmFlightPtr = NULL;

        // Get the ECM flight from our package (if any)
        Package pack = (Package) vuDatabase->Find(package);

        if (pack)
            ecmFlightPtr = (Flight) vuDatabase->Find(pack->GetECM());
    }

    // If the ECM flight is now dead, we can't use it anymore...
    if (ecmFlightPtr and ecmFlightPtr->IsDead())
        ecmFlightPtr = NULL;

    return ecmFlightPtr;
}
// END OF ADDED SECTION

Flight FlightClass::GetAWACSFlight(void)
{
    Package pack = (Package) vuDatabase->Find(package);

    if (pack)
        return (Flight) vuDatabase->Find(pack->GetAwacs());

    return NULL;
}

Flight FlightClass::GetTankerFlight(void)
{
    Package pack = (Package) vuDatabase->Find(package);

    if (pack)
        return (Flight) vuDatabase->Find(pack->GetTanker());

    return NULL;
}

Flight FlightClass::GetJSTARFlight(void)
{
    Package pack = (Package) vuDatabase->Find(package);

    if (pack)
        return (Flight) vuDatabase->Find(pack->GetJStar());

    return NULL;
}

Flight FlightClass::GetFACFlight(void)
{
    Package pack = (Package) vuDatabase->Find(package);

    if (pack)
        return pack->GetFACFlight();

    return NULL;
}

// JPO - find who is contolling our flight.
Flight FlightClass::GetFlightController()
{
    Flight awacs;
    // Check FAC/JSTAR/AWACS callsign
    awacs = GetFACFlight();

    if ( not awacs)
        awacs = GetJSTARFlight();

    if ( not awacs)
        awacs = GetAWACSFlight();

    return awacs;
}

int FlightClass::FindCollisionPoint(FalconEntity *target, vector* collPoint, int noAWACS)
{
    int retval;

    // Put line seg equation into parametric form S = Q+tV,
    // translated into sphere's coord system.
    //
    // Note (this is important) that the way we have defined it,
    // the line segment includes all values of t where (0 <= t <= 1)
    //
    // solve for t on sphere's surface,  ie. where S.S = R*R, or
    // t*t(objVel.objVel) + t(2objVel.Q) + (Q.Q - ownSpeed*t * ownSpeed*t ) = 0
    // which is quadratic in t such that:
    // a = V.V - ownSpeed*ownSpeed
    // b = 2V.Q
    // c = Q.Q
    //

    // Need to find unit's heading/speed/deltas in sim coordinates
    // KCK NOTE: This is all kinda pointless if the target is manuevering. Is there a way to tell?
    mlTrig sincos;
    float speed = 0.0F;
    vector q;
    float a, b, c;
    float underRad;
    float minT, maxT;
    float ownSpeed;
    float xdel, ydel, zdel;

    q.x = target->XPos() - XPos();
    q.y = target->YPos() - YPos();
    q.z = target->ZPos() - ZPos();

    // Calculate interceptor's speed
    ownSpeed = GetUnitSpeed() * KPH_TO_FPS;
    ownSpeed = ownSpeed * ownSpeed;

    if (target->IsUnit())
        speed = (float)((Unit)target)->GetUnitSpeed() * KPH_TO_FPS;
    else if (target->IsSim())
        speed = GetVt();

    if (speed > 1.0F)
    {
        mlSinCos(&sincos, target->Yaw());
        xdel = speed * sincos.cos;
        ydel = speed * sincos.sin;
        zdel = 0.0F;
        a = (xdel * xdel + ydel * ydel) - ownSpeed;
        b = (xdel * q.x + ydel * q.y) * 2.0F;
        c = q.x * q.x + q.y * q.y + q.z * q.z;

        // First, see if there is a real solution (ie.  (b*b - 4*a*c) >= 0 )
        //
        underRad = b * b - 4.0F * a * c;

        if (underRad < 0.0F)
        {
            // line does not intersect sphere
            retval = FALSE;
        }
        else
        {
            // find the points where the intersection(s) happen
            retval = TRUE;

            if (underRad == 0.0F)
            {
                minT = maxT = -b / (2.0F * a);
                collPoint->x = target->XPos() + xdel * minT;
                collPoint->y = target->YPos() + ydel * minT;
                collPoint->z = target->ZPos() + zdel * minT;
            }
            else
            {
                minT = (-b - (float)sqrt(underRad)) / (2.0F * a);
                maxT = (-b + (float)sqrt(underRad)) / (2.0F * a);

                if (minT < 0.0F)
                    minT = maxT;

                if (minT < 0.0F)
                    retval = FALSE;
                else
                {
                    collPoint->x = target->XPos() + xdel * minT; // - XPos();
                    collPoint->y = target->YPos() + ydel * minT; // - YPos();
                    collPoint->z = target->ZPos() + zdel * minT; // - ZPos();
                }
            }
        }
    }
    else
    {
        // Just head towards this slow mover/static location
        collPoint->x = target->XPos();
        collPoint->y = target->YPos();
        collPoint->z = target->ZPos();
        retval = TRUE;
    }

    // KCK hackish: If the points moved by greater than x feet, send another awacs message
    if (assigned_target == target->Id() and DistSqu(collPoint->x, collPoint->y, last_collision_x, last_collision_y) > 10 * NM_TO_FT * 10 * NM_TO_FT)
    {
        last_collision_x = collPoint->x;
        last_collision_y = collPoint->y;

        if ( not noAWACS and target->IsCampaign())
            PlayDivertRadioCalls((CampEntity)target, mission, this, 1);
    }

    return (retval);
}

void FlightClass::RegisterLock(FalconEntity *locker)
{
#ifdef GILMANS_BEAM_TACTIC
    Unit camp_locker;
    int player_ac = 0;

    // Keep beaming the first enemy locker until lock is broken
    if (Locked())
        return;

    if (locker->IsSim())
    {
        camp_locker = (Unit)((SimBaseClass*)locker)->GetCampaignObject();
        player_ac = ((SimBaseClass*)locker)->IsSetFlag(MOTION_OWNSHIP);
    }
    else if (locker->IsUnit())
        camp_locker = (Unit)locker;
    else
        return;

    // Only register lock if it's a player or the enemy is engaging us specifically
    if ( not camp_locker or ( not player_ac and camp_locker->GetUnitTactic() not_eq ATACTIC_ENGAGE_AIR))
        return;

    SetLocked(1);
    last_enemy_lock_time = TheCampaign.CurrentTime;
    enemy_locker = camp_locker->Id();
#else
    return;
#endif
}

int FlightClass::GetDetectionRange(int mt)
{
    int dr;
    UnitClassDataType* uc = GetUnitClassData();

    ShiAssert(uc);
    // 2001-04-21 MODIFIED BY S.G.
    // ABOVE 250 HAS A NEW MEANING SO USE THE UNIT ELECTRONIC DETECTION RANGE INSTEAD...
    // dr = uc->Detection[mt];
    dr = GetElectronicDetectionRange(mt);

    // 2001-03-15 MODIFIED BY S.G. WHY *8??
    // THIS BRINGS THE DETECTION RANGE FOR A F16C TO 128 KM OTHER TYPE DON'T DO THIS ANYHOW...
    // if (dr < VisualDetectionRange[mt]*8)
    // dr = GetVisualDetectionRange(mt)*8;
    if (dr < VisualDetectionRange[mt] * g_nFlightVisualBonus)
    {
        dr = GetVisualDetectionRange(mt) * g_nFlightVisualBonus;
    }

    return dr;
}

int FlightClass::IsSPJamming(void)
{
    if (HasSPJamming() and (Engaged() or Locked() or GetUnitCurrentRole() == ARO_CA))
        return TRUE;

    return FALSE;
}

int FlightClass::IsAreaJamming(void)
{
    // JPO - change to basically jamming and active.
    if (HasAreaJamming() and 
        (eval_flags bitand FEVAL_ON_STATION)) // old test 2002-02-19 REINSTATED BY S.G. Fixed the eval_flags bug where the bit would not reset
        //     Moving() and 
        // not IsDead())
        return TRUE;

    return FALSE;
}

int FlightClass::HasSPJamming(void)
{
    if (GetUnitFlags() bitand U_HASECM)
        return TRUE;

    return FALSE;
}

int FlightClass::HasAreaJamming(void)
{
    if (mission == AMIS_ECM)
        return TRUE;

    return FALSE;
}
#include "atcbrain.h"

int FlightClass::GetVehicleDeagData(SimInitDataClass *simdata, int remote)
{
    static CampEntity ent;
    static int pilotSlot, queue = 0, fuelBurnt, rwindex = 0;
    int value, i;
    PilotClass *pc;
    runwayQueueStruct *info = NULL;
    mlTrig trig;

    // Reinitialize static vars upon query of first vehicle
    if (simdata->vehicleInUnit < 0)
    {
        WayPoint w;
        pilotSlot = 0;

        if ( not remote)
        {
            simdata->ptIndex = GetDeaggregationPoint(0, &ent);

            if (simdata->ptIndex == DPT_ERROR_NOT_READY)
            {
                return -1;
            }
            else if (simdata->ptIndex == DPT_ERROR_CANT_PLACE)
            {
                return -1;
            }
            else if (simdata->ptIndex == DPT_ONBOARD_CARRIER)
            {
                simdata->ptIndex = 0;
            }

            if (ent and ent->IsObjective())
            {
                ATCBrain *atc = ((Objective)ent)->brain;
                info = atc->InList(Id());

                if (info)
                {
                    rwindex = info->rwindex;
                    queue = GetQueue(rwindex);
                }
                else
                {
                    queue = 0;
                    rwindex = simdata->rwIndex;
                }
            }

            w = GetCurrentUnitWP();

            if (w)
            {
                // Find heading to next waypoint
                GridIndex ux, uy, wx, wy;
                GetLocation(&ux, &uy);
                w->GetWPLocation(&wx, &wy);
                simdata->heading = AngleTo(ux, uy, wx, wy);
            }
        }
        else
        {
            simdata->ptIndex = 0;
        }
    }
    else
    {
        pilotSlot++;
    }

    // Skip dead/missing slots
    while (plane_stats[pilotSlot] not_eq AIRCRAFT_AVAILABLE and pilotSlot < PILOTS_PER_FLIGHT)
    {
        pilotSlot++;
    }

    pilotSlot = pilotSlot % PILOTS_PER_FLIGHT;

    // vehicleInUnit is the pilot/vehicle slot in the case of Flights.
    simdata->vehicleInUnit = pilotSlot;
    // playerSlot is the id of any player pilots which are expected to be joining
    simdata->playerSlot = player_slots[pilotSlot];

    // Determine skill
    pc = GetPilotData(pilotSlot);

    if (pc)
    {
        simdata->skill = pc->GetPilotSkill();
    }
    else
    {
        // Need to have a squadron to have a pilot skill
        simdata->skill = pilots[pilotSlot] % PILOT_SKILL_RANGE;
    }

    if (simdata->skill > 4)
    {
        simdata->skill = 4;
    }

    if (simdata->skill < 0)
    {
        simdata->skill = 0;
    }

    // Determine location (local entities only)
    if ( not remote)
    {
        // Place on a taxi point (ground)
        if (simdata->ptIndex > 0)
        {
            float x, y;

            if (
                PtDataTable[simdata->ptIndex].type == TakeoffPt and 
                ((Objective)ent)->brain->UseSectionTakeoff(this, rwindex) and simdata->vehicleInUnit < 2)
            {
                simdata->ptIndex = ((Objective)ent)->brain->FindTakeoffPt(
                                       this, simdata->vehicleInUnit, rwindex, &simdata->x, &simdata->y
                                   );

                while (CheckPointGlobal(this, simdata->x, simdata->y))
                {
                    if ( not rwindex)
                    {
                        simdata->x -= 50.0F;
                    }
                    else if (PtHeaderDataTable[rwindex].ltrt < 0)
                    {
                        simdata->x -= -PtHeaderDataTable[rwindex].sinHeading * 50.0F;
                        simdata->y -= PtHeaderDataTable[rwindex].cosHeading * 50.0F;
                    }
                    else
                    {
                        simdata->x -= PtHeaderDataTable[rwindex].sinHeading * 50.0F;
                        simdata->y -= -PtHeaderDataTable[rwindex].cosHeading * 50.0F;
                    }
                }

                // Face the next point
                ((Objective)ent)->brain->FindRunwayPt(this, simdata->inSlot, rwindex, &x, &y);
                simdata->heading = (float)atan2((y - simdata->y), (x - simdata->x));
            }
            else
            {
                if (simdata->vehicleInUnit)
                {
                    int pt;
                    simdata->ptIndex = GetNextPtLoop(simdata->ptIndex);
                    pt = simdata->ptIndex;

                    while (pt)
                    {
                        // and PtDataTable[pt].type not_eq TaxiPt and PtDataTable[pt].type not_eq CritTaxiPt)
                        switch (PtDataTable[pt].type)
                        {
                            case TaxiPt:
                            case CritTaxiPt:
                            case LargeParkPt:
                            case SmallParkPt:
                                break;

                            default:
                                pt = GetNextPt(pt);

                                if (pt)
                                    simdata->ptIndex = pt;

                                continue;
                        }

                        break;
                    }
                }

                TranslatePointData(ent, simdata->ptIndex, &simdata->x, &simdata->y);

                while (CheckPointGlobal(this, simdata->x, simdata->y))
                {
                    if ( not rwindex)
                    {
                        simdata->x -= 50.0F;
                    }
                    else if (PtHeaderDataTable[rwindex].ltrt < 0)
                    {
                        simdata->x -= -PtHeaderDataTable[simdata->rwIndex].sinHeading * 50.0F;
                        simdata->y -= PtHeaderDataTable[simdata->rwIndex].cosHeading * 50.0F;
                    }
                    else
                    {
                        simdata->x -= PtHeaderDataTable[rwindex].sinHeading * 50.0F;
                        simdata->y -= -PtHeaderDataTable[rwindex].cosHeading * 50.0F;
                    }
                }

                // Face the next point
                TranslatePointData(ent, simdata->ptIndex - 1, &x, &y);
                simdata->heading = (float)atan2((y - simdata->y), (x - simdata->x));

            }

            simdata->z = OTWDriver.GetGroundLevel(simdata->x, simdata->y) - 5.0F;
        }
        else
        {
            mlSinCos(&trig, simdata->heading);

            if (GetTotalVehicles() <= 4)
            {
                simdata->x = simdata->x - VFormRight[simdata->vehicleInUnit] * trig.sin +
                             VFormAhead[simdata->vehicleInUnit] * trig.cos;
                simdata->y = simdata->y + VFormRight[simdata->vehicleInUnit] * trig.cos +
                             VFormAhead[simdata->vehicleInUnit] * trig.sin;
            }
            else
            {
                float right = CompanyFormations[1][simdata->campSlot / 4].x + PlatoonFormations[3][simdata->campSlot % 4].x + SquadFormations[2][simdata->inSlot].x;
                float ahead = CompanyFormations[1][simdata->campSlot / 4].y + PlatoonFormations[3][simdata->campSlot % 4].y + SquadFormations[2][simdata->inSlot].y;
                simdata->x = simdata->x + right * trig.cos - ahead * trig.sin;
                simdata->y = simdata->y + right * trig.sin - ahead * trig.cos;
            }

            simdata->ptIndex = 0;
        }
    }


    // Do weapon loadout
    if (GetNumberOfLoadouts() - 1 < simdata->vehicleInUnit)
    {
        value = 1;
    }
    else
    {
        value = simdata->vehicleInUnit;
    }

    for (i = 0; i < HARDPOINT_MAX; i++)
    {
        simdata->weapon[i] = GetUnitWeaponId(i, value);

        if (simdata->weapon[i])
            simdata->weapons[i] = GetUnitWeaponCount(i, value);
        else
            simdata->weapons[i] = 0;
    }

    // JB 020122 Reset the fuel when entering a dogfight.
    if (FalconLocalGame and FalconLocalGame->GetGameType() == game_Dogfight)
    {
        fuel_burnt = 0;
    }

    // Determine actual fuel for this aircraft
    simdata->fuel = CalculateFuelAvailable(value);

    // Do waypoints
    simdata->waypointList = CloneWPToList(GetFirstUnitWP(), NULL);

    return MOTION_AIR_AI;
}

int FlightClass::GetUnitWeaponId(int hp, int ac)
{
    if (ac >= loadouts)
        ac = 0;

    ShiAssert(loadouts);

    if (&loadout[ac])
        return loadout[ac].WeaponID[hp];

    return 0;
}

int FlightClass::GetUnitWeaponCount(int hp, int ac)
{
    if (ac >= loadouts)
        ac = 0;

    ShiAssert(loadouts);

    if (&loadout[ac])
        return loadout[ac].WeaponCount[hp];

    return 0;
}

void FlightClass::RemoveLoadout(void)
{
    if (loadout)
    {
        delete loadout;
        loadout = NULL;
    }

    loadouts = 0;
}

LoadoutStruct* FlightClass::GetLoadout(int ac)
{
    if (ac >= loadouts)
        ac = 0;

    ShiAssert(loadouts);

    return &loadout[ac];
}

void FlightClass::SetLoadout(LoadoutStruct *newload, int count)
{
    LoadoutStruct *oldload = loadout;

    // JPO strengthened the checks.
    ShiAssert(FALSE == F4IsBadReadPtr(class_data, sizeof * class_data)); // RH
    VehicleClassDataType *vc = GetVehicleClassData(class_data->VehicleType[0]);
    ShiAssert(FALSE == F4IsBadReadPtr(vc, sizeof * vc));
    int hasECM = (vc->Flags bitand VEH_HAS_JAMMER ? TRUE : FALSE);
    int i, j;

    loadouts = count;
    loadout = newload;
    delete oldload;

    // Check for ECM pods. Stop looking if we find even one
    for (i = 0; i < loadouts and not hasECM; i++)
    {
        for (j = 0; j < HARDPOINT_MAX and not hasECM; j++)
        {
            if ((loadout[i].WeaponCount[j] > 0) and (WeaponDataTable[loadout[i].WeaponID[j]].Flags bitand WEAP_ECM))
            {
                hasECM = TRUE;
            }
        }
    }

    SetHasECM(hasECM);

    if ( not IsLocal())
    {

        // Need to send data to the host
        VuSessionEntity *target = (VuSessionEntity*) vuDatabase->Find(OwnerId());
        FalconFlightPlanMessage *msg = new FalconFlightPlanMessage(Id(), target);
        uchar *buffer;
        long lbsfuel = 0;

        msg->dataBlock.type = FalconFlightPlanMessage::loadoutData;
        msg->dataBlock.size = HARDPOINT_MAX * loadouts + HARDPOINT_MAX * loadouts * sizeof(short) + sizeof(long) + sizeof(uchar);
        msg->dataBlock.data = buffer = new uchar[msg->dataBlock.size];
        memcpy(buffer, &lbsfuel, sizeof(long));
        buffer += sizeof(long);
        memcpy(buffer, &loadouts, sizeof(uchar));
        buffer += sizeof(uchar);

        for (i = 0; i < loadouts; i++)
        {
            // edg: for debugging -- we're getting bad data in loadouts
            // Ed, PLEASE don't do this sort of thing without some sort of testing #def,
            // or use the correct value - this would make adding new weapons difficult
#ifdef EDDEBUG
            if (loadout[i].WeaponID[0] > 203)
            {
                loadout[i].WeaponID[0] = 0;
            }

#endif
            ShiAssert(loadout[i].WeaponID[0] < NumWeaponTypes);

            memcpy(buffer, loadout[i].WeaponID, HARDPOINT_MAX * sizeof(short));
            buffer += HARDPOINT_MAX * sizeof(short);
            memcpy(buffer, loadout[i].WeaponCount, HARDPOINT_MAX);
            buffer += HARDPOINT_MAX;
        }

        FalconSendMessage(msg, TRUE);
    }

    MakeStoresDirty();
}

void FlightClass::SendComponentMessage(int command, VuEntity *sender)
{
    FalconWingmanMsg* wingCommand;

    wingCommand = new FalconWingmanMsg(Id(), FalconLocalGame);

    wingCommand->dataBlock.from = Id();
    wingCommand->dataBlock.to = AiAllButSender;
    wingCommand->dataBlock.command = command;
    wingCommand->dataBlock.newTarget = FalconNullId;

    FalconSendMessage(wingCommand, TRUE);
}

// =============================
// Support Functions
// =============================

// Regroup Flight should be called whenever a flight is removed with aircraft remaining.
int RegroupFlight(Flight flight)
{
    Squadron squad = (Squadron)flight->GetUnitSquadron();
    int i;
    GridIndex x = 0, y = 0;

    flight->GetLocation(&x, &y);

    // RV - Biker - Don't drop Air Mobile over water
    if (flight->Cargo() and GetCover(x, y) not_eq Water)
    {
        flight->UnloadUnit();
    }

    // Free our reference on this callsign
    UnsetCallsignID(flight->callsign_id, flight->callsign_num);

    flight->UnSetFalcFlag(FEC_REGENERATING);

    // ADDED BY S.G. SO DEAD UNITS DON'T HAVE THEIR PILOTS BACK IN BUSINESS AND AIRPLANE  BACK IN THE HANGAG
    // JB 010228 Commented out -- attrition rates way too high
    // JB 010710 Make configurable
    if (g_bRealisticAttrition and flight->IsDead())
        return 1;

    // END OF ADDED SECTION

    //  M.N. Put ammo that we eventually still have back into the ammo dump
    if (g_bLoadoutSquadStoreResupply and squad)
    {
        int fuelAvail;
        fuelAvail = flight->CalculateFuelAvailable(255);
        squad->ResupplySquadronStores(flight->GetLoadout(0)->WeaponID, flight->GetLoadout(0)->WeaponCount, fuelAvail, flight->GetTotalVehicles());
    }

    // Free up any remaining pilots
    for (i = 0; i < PILOTS_PER_FLIGHT; i++)
    {
        if (flight->pilots[i] not_eq NO_PILOT)
        {
            if (squad)
                squad->SetPilotStatus(flight->pilots[i], PILOT_AVAILABLE);

            flight->pilots[i] = NO_PILOT;
            flight->plane_stats[i] = AIRCRAFT_NOT_ASSIGNED;
            //flight->MakeFlightDirty (DIRTY_PLANE_STATS, DDP[100].priority);
            flight->MakeFlightDirty(DIRTY_PLANE_STATS, SEND_RELIABLE);
        }
    }

    // S.G. NOT NEEDED ANYMORE. WAS DONE ABOVE
    // JB 010228 Commented back in -- attrition rates way too high
    if (flight->IsDead())
    {
        return 1;
    }

    //
    if (TeamInfo[flight->GetTeam()])
        TeamInfo[flight->GetTeam()]->atm->SendATMMessage(flight->Id(), flight->GetTeam(), FalconAirTaskingMessage::atmNewACAvail, 0, 0, NULL, 0);

    // Set the final aircraft in the mission eval structure if this is one of the flights
    // in the player's package
    TheCampaign.MissionEvaluator->SetFinalAircraft(flight);

    // Kill the unit (just to get it all removed and shee-at)
    flight->KillUnit();
    return 1;
}

// Returns # of pilots in flight (including players)
int FlightClass::GetPilotCount(void)
{
    int i, count = 0;

    for (i = 0; i < PILOTS_PER_FLIGHT; i++)
    {
        if (pilots[i] not_eq NO_PILOT or player_slots[i] < 255)
            count++;
    }

    return(count);
}

// Returns # of pilots in flight (including players)
int FlightClass::GetACCount(void)
{
    int i, count = 0;

    for (i = 0; i < PILOTS_PER_FLIGHT; i++)
    {
        if (plane_stats[i] == AIRCRAFT_AVAILABLE)
            count++;
    }

    return(count);
}
class SmsClass;

// This function is intended to be called from the Sim Thread ONLY
void RegroupAircraft(AircraftClass *ac)
{
    int pilotSlot;
    Flight flight;
    Squadron squad; // M.N.

    ShiAssert(ac not_eq FalconLocalSession->GetPlayerEntity());

    if (ac->IsDead())
    {
        return;
    }

    flight = (Flight)ac->GetCampaignObject();

    // find last hardpoint

    // Set this vehicle as "Return To Base" so that record current state will not chalk
    // it up as a loss
    if (ac->vehicleInUnit < PILOTS_PER_FLIGHT)
    {
        pilotSlot = flight->GetAdjustedPlayerSlot(ac->pilotSlot);
        ShiAssert(flight->pilots[pilotSlot] not_eq NO_PILOT);
        ShiAssert(pilotSlot == ac->vehicleInUnit);
        flight->plane_stats[pilotSlot] = AIRCRAFT_RTB;
        flight->MakeFlightDirty(DIRTY_PLANE_STATS, SEND_RELIABLE);

        // 2001-12-16 MN Put ammo that we eventually still have back into the ammo dump
        squad = (Squadron)flight->GetUnitSquadron();

        if (g_bLoadoutSquadStoreResupply and squad)
        {
            int fuelAvail;
            fuelAvail = flight->CalculateFuelAvailable(255);
            // here only resupply 1 aircraft
            squad->ResupplySquadronStores(
                flight->GetLoadout(0)->WeaponID, flight->GetLoadout(0)->WeaponCount, fuelAvail, 1
            );
        }

        ac->SetRemoveFlag();
    }

    // Now update the flight's status
    flight->RecordCurrentState(NULL, FALSE);
}

void CancelFlight(Flight flight)
{
    ShiAssert(flight->VuState() == VU_MEM_ACTIVE);
    UpdateSquadronStatus(flight, TRUE, FALSE);
}

void UpdateSquadronStatus(Flight flight, int landed, int playchatter)
{
    Squadron squad = (Squadron)flight->GetUnitSquadron();
    int losses = 0, sendmessage = 0;
    FalconRadioChatterMessage *msg = NULL;

    if ( not squad)
        return;

    // Don't update the squadron if the flight is not local
    if ( not flight->IsLocal())
        return;

    if (playchatter)
    {
        // Send a radio chatter message
        msg = new FalconRadioChatterMessage(flight->Id(), FalconLocalGame);
        msg->dataBlock.from = flight->Id();
        msg->dataBlock.to = MESSAGE_FOR_TEAM;
    }

    // Update the stats
    for (int i = 0; i < PILOTS_PER_FLIGHT; i++)
    {
        switch (flight->plane_stats[i])
        {
            case AIRCRAFT_MISSING:
                if (flight->pilots[i] not_eq NO_PILOT)
                {
                    squad->SetPilotStatus(flight->pilots[i], PILOT_MIA);
                    flight->pilots[i] = NO_PILOT;
                    losses++;

                    if (playchatter)
                    {
                        msg->dataBlock.message = rcPILOTHITD;
                        msg->dataBlock.voice_id = flight->GetPilotVoiceID(i);
                        FalconSendMessage(msg, FALSE);
                        // Now send another...
                        msg = new FalconRadioChatterMessage(flight->Id(), FalconLocalGame);
                        msg->dataBlock.message = rcAIRMANDOWNA;
                        msg->dataBlock.from = flight->Id();
                        msg->dataBlock.to = MESSAGE_FOR_TEAM;
                        msg->dataBlock.voice_id = g_voicemap.PickVoice(VoiceMapper::VOICE_PILOT, flight->GetOwner()); // rand()%NUM_PILOT_VOICES; // JPO VOICEFIX
                        msg->dataBlock.edata[0] = flight->callsign_id;
                        msg->dataBlock.edata[1] = flight->GetFlightLeadCallNumber();
                        msg->dataBlock.time_to_play = 2 * CampaignSeconds;
                        FalconSendMessage(msg, FALSE);
                        // And yet another...
                        msg = new FalconRadioChatterMessage(flight->Id(), FalconLocalGame);
                        msg->dataBlock.message = rcAIRMANDOWNF; // changed from rcARIMANDOWNE (so it says "Setup RESCAP" now)
                        msg->dataBlock.from = flight->Id();
                        msg->dataBlock.to = MESSAGE_FOR_TEAM;
                        msg->dataBlock.voice_id = g_voicemap.PickVoice(VoiceMapper::VOICE_PILOT, flight->GetOwner()); //rand()%NUM_PILOT_VOICES; // JPO VOICEFIX
                        msg->dataBlock.time_to_play = 4 * CampaignSeconds;
                        msg->dataBlock.edata[0] = -1;
                        msg->dataBlock.edata[1] = -1;
                        //flight->GetLocation(&msg->dataBlock.edata[0],&msg->dataBlock.edata[1]);
                        sendmessage = 1;

                        // RV - Biker - No SAR missions at all with this
                        // if ( not (rand()%5) and RequestSARMission (flight))
                        if (RequestSARMission(flight))
                        {
                            // Generate a SAR radio call from awacs
                            FalconRadioChatterMessage* radioMessage;
                            radioMessage = CreateCallFromAwacs(flight, rcSARENROUTE);
                            radioMessage->dataBlock.time_to_play = CampaignSeconds;
                            FalconSendMessage(radioMessage, FALSE);
                        }
                    }
                }

                break;

            case AIRCRAFT_DEAD:
                if (flight->pilots[i] not_eq NO_PILOT)
                {
                    squad->SetPilotStatus(flight->pilots[i], PILOT_KIA);
                    flight->pilots[i] = NO_PILOT;
                    losses++;

                    if (playchatter)
                    {
                        msg->dataBlock.message = rcLASTWORDS;
                        msg->dataBlock.voice_id = flight->GetPilotVoiceID(i);
                        msg->dataBlock.edata[0] = 32767;
                        FalconSendMessage(msg, FALSE);
                        // Now send another...
                        msg = new FalconRadioChatterMessage(flight->Id(), FalconLocalGame);
                        msg->dataBlock.message = rcAIRMANDOWNB;
                        msg->dataBlock.from = flight->Id();
                        msg->dataBlock.to = MESSAGE_FOR_TEAM;
                        msg->dataBlock.voice_id = g_voicemap.PickVoice(VoiceMapper::VOICE_PILOT, flight->GetOwner()); //rand()%NUM_PILOT_VOICES; // JPO VOICEFIX
                        //M.N. changed to 32767 -> flexibly use randomized values of max available eval indexes
                        msg->dataBlock.edata[0] = 32767;
                        msg->dataBlock.time_to_play = 2 * CampaignSeconds;
                        sendmessage = 1;

                        // RV - Biker - No SAR missions at all with this
                        // if ( not (rand()%5) and RequestSARMission (flight))
                        if (RequestSARMission(flight))
                        {
                            // Generate a SAR radio call from awacs
                            FalconRadioChatterMessage* radioMessage;
                            radioMessage = CreateCallFromAwacs(flight, rcSARENROUTE);
                            radioMessage->dataBlock.time_to_play = CampaignSeconds;
                            FalconSendMessage(radioMessage, FALSE);
                        }
                    }
                }

                break;

            case AIRCRAFT_RTB:
                if (flight->pilots[i] not_eq NO_PILOT)
                {
                    RatePilot(flight, i, rand() % 5);
                    squad->SetPilotStatus(flight->pilots[i], PILOT_AVAILABLE);
                    flight->pilots[i] = NO_PILOT;

                    if (playchatter)
                    {
                        msg->dataBlock.message = rcPILOTHITA;
                        msg->dataBlock.voice_id = flight->GetPilotVoiceID(i);
                        sendmessage = 1;
                    }
                }

                break;

            case AIRCRAFT_AVAILABLE:
            case AIRCRAFT_NOT_ASSIGNED:
            default:
                if (landed and flight->pilots[i] not_eq NO_PILOT)
                {
                    squad->SetPilotStatus(flight->pilots[i], PILOT_AVAILABLE);
                    flight->pilots[i] = NO_PILOT;
                    flight->plane_stats[i] = AIRCRAFT_NOT_ASSIGNED;
                    //flight->MakeFlightDirty (DIRTY_PLANE_STATS, DDP[102].priority);
                    flight->MakeFlightDirty(DIRTY_PLANE_STATS, SEND_RELIABLE);
                }

                break;
        }
    }

    // Decide whether to send the message or not
    if (sendmessage)
        FalconSendMessage(msg, FALSE);
    else if (msg)
        delete msg;

    if (losses)
        squad->BroadcastUnitMessage(flight->Id(), FalconUnitMessage::unitSetVehicles, UMSG_FROM_RESERVE, losses, 0);

    if (landed or not flight->GetTotalVehicles())
        RegroupFlight(flight);
}

void RatePilot(Flight flight, int pilotSlot, int newRating)
{
    PilotClass *pc;
    int rating, ktmr;

    // pc was uninitialized, so if flight was null... crash
    if (flight)
    {
        pc = flight->GetPilotData(pilotSlot);

        if (pc and pc->missions_flown)
        {
            ktmr = (pc->aa_kills * 2 + pc->ag_kills + pc->as_kills + pc->an_kills * 2) / pc->missions_flown;

            if (ktmr > newRating)
                newRating++;

            if (ktmr < newRating and newRating > 0)
                newRating--;

            rating = pc->GetPilotRating() * (pc->missions_flown - 1);
            rating = (rating + newRating) / pc->missions_flown;
            pc->SetPilotSR(pc->GetPilotSkill(), rating);
        }
    }
}

WayPoint ResetCurrentWP(Unit u)
{
    WayPoint w;
    GridIndex x, y, ux, uy;
#ifdef DEBUG
    WayPoint ow;
#endif

    w = u->GetCurrentUnitWP();
#ifdef DEBUG
    ow = w;
#endif
    ShiAssert(w);

    // JPO Logic is: if we are past our departure time at the WP, we got to do something
    // But if we are past our arrival time, and its a repeating WP, we might need to go back.
    // So do the loop anyway. last bit was already commented out.
    while (w and 
           (w->GetWPDepartureTime() < Camp_GetCurrentTime() or // JPO Original test
            ((w->GetWPFlags() bitand WPF_REPEAT) and w->GetWPArrivalTime() < Camp_GetCurrentTime()))
          ) // and w->GetWPAction() not_eq WP_LAND)
    {
        if (w->GetWPFlags() bitand WPF_CRITICAL_MASK or // 2002-02-20 MODIFIED BY S.G. Needs to get here if it's a refuel waypoint under some condition as defined below. I could have added a WPF_REFUEL flag but that would have wasted a flag just to be used here anyway so we're hacking our way in
            (w->GetWPAction() == WP_REFUEL and u->IsFlight() and // Must be a flight over a WP_REFUEL waypoint
             (( not (((FlightClass *)u)->GetEvalFlags() bitand FEVAL_GOT_TO_TARGET) and not ((FlightClass *)u)->HasFuel(3)) or // We haven't reached our target, refuel if we have less than 3/4 of our capacity left, otherwise skip it
              ((((FlightClass *)u)->GetEvalFlags() bitand FEVAL_GOT_TO_TARGET) and not ((FlightClass *)u)->HasFuel(9))))) // We haven't reached our target, refuel if we have less than 1/4 of our capacity left, otherwise skip it
        {
            // Either keep heading here, or do our action and increment
            w->GetWPLocation(&x, &y);
            u->GetLocation(&ux, &uy);

            if (DistSqu(x, y, ux, uy) > 1.0F)
            {
                if (w->GetWPAction() == WP_TAKEOFF and u->IsAggregate())
                    u->SetLocation(x, y);

                return w;
            }

            if (DoWPAction((Flight)u) < 0)
                return NULL;

            if (w->GetWPFlags() bitand WPF_REPEAT)
                return u->GetCurrentUnitWP(); // We've already selected a waypoint in this case

            w = u->GetCurrentUnitWP(); // Make sure we've still got a good WP

            if ( not w)
                return NULL;
        }

        w = w->GetNextWP();
        u->SetCurrentUnitWP(w);
    }

    // Check for zero time and update to best guess from current location
    // These are usually abort fields which are only pointed to as a result of an abort
    if (w and w->GetWPArrivalTime() == 0)
    {
        GridIndex x, y;
        MonoPrint("Unit %d: Waypoint action %d (mission: %s) didn't have time. Setting times.\n", u->GetCampID(), w->GetWPAction(), MissStr[u->GetUnitMission()]);
        u->GetLocation(&x, &y);
        SetWPTimes(w, x, y, u->GetCombatSpeed(), 0);
    }

    // ShiAssert(w); - RH
    return w;
}

void GoHome(Flight flight)
{
    WayPoint w;

    w = flight->GetCurrentUnitWP();

    // RV - Biker - Check if we are already at way home
    if (w and w->GetWPAction() == WP_LAND)
    {
        return;
    }

    // w = flight->GetCurrentUnitWP();
    w = flight->GetFirstUnitWP();

    while (w and w->GetWPAction() not_eq WP_LAND)
    {
        w = w->GetNextWP();
    }

    if ( not w)
    {
        w = flight->GetFirstUnitWP();
    }
    // RV - Biker - Go to last WP before home base
    else
    {
        w = w->GetPrevWP();
    }

    flight->SetCurrentUnitWP(w);
}

void AbortFlight(Flight flight)
{
    Unit pack, e;
    GridIndex x = 0, y = 0;

    // MonoPrint ("Flight %d aborting. \n",flight->GetCampID());

    flight->GetLocation(&x, &y);

    // Drop any cargo we're carrying
    // RV - Biker - Only drop if we are not over water
    if (flight->Cargo() and GetCover(x, y) not_eq Water)
    {
        flight->UnloadUnit();
    }

    /*
    // If we've already gotten to our target, and it's past our TOT, don't abort
    if (((Flight)flight)->eval_flags bitand FEVAL_GOT_TO_TARGET and flight->GetUnitTOT() < TheCampaign.CurrentTime)
    return;
    */

    // Radio Chatter message
    FalconRadioChatterMessage *msg = new FalconRadioChatterMessage(flight->Id(), FalconLocalGame);
    msg->dataBlock.from = flight->Id();
    msg->dataBlock.to = MESSAGE_FOR_TEAM;
    msg->dataBlock.voice_id = flight->GetFlightLeadVoiceID();
    msg->dataBlock.message = rcFLIGHTRTB;
    msg->dataBlock.edata[0] = flight->callsign_id;
    msg->dataBlock.edata[1] = flight->GetFlightLeadCallNumber();
    FalconSendMessage(msg, FALSE);

    if ( not flight->Aborted())
        TheCampaign.MissionEvaluator->RegisterAbort(flight);

    flight->SetAborted(1);

    // If it's a critical component, abort the rest of the Mission Group
    if ( not flight->GetUnitMissionID())
    {
        pack = flight->GetUnitParent();

        if (pack)
        {
            pack->SetAborted(1);
            e = pack->GetFirstUnitElement();

            while (e and e->GetUnitMissionID())
            {
                AbortFlight((Flight)e);
                e = pack->GetNextUnitElement();
            }
        }
    }
}

Objective FindAlternateStrip(Flight flight)
{
    Objective o, bo = NULL;
    CampBaseClass   *homebase = NULL, *target = NULL; // 2001-10-09 M.N.
    GridIndex x, y, ox, oy;
    WayPoint w, pa_wp = NULL, target_wp = NULL;
    float d, bd = 9999.0F;
    VuGridIterator *oit;

    w = flight->GetFirstUnitWP();

    while (w and w->GetWPAction() not_eq WP_POSTASSEMBLE)
    {
        if (w->GetWPAction() == WP_POSTASSEMBLE)
            pa_wp = w;

        if (w->GetWPFlags() bitand WPF_TARGET)
        {
            if (w->GetWPTarget())
                target = w->GetWPTarget();

            target_wp = w;
        }

        if (w->GetWPAction() == WP_TAKEOFF)
            homebase = w->GetWPTarget();

        w = w->GetNextWP();
    }

    if (pa_wp)
    {
        pa_wp->GetWPLocation(&x, &y);
    }
    else if (target_wp)
    {
        target_wp->GetWPLocation(&x, &y);
    }
    else
    {
        flight->GetLocation(&x, &y);
    }

    /* if (homebaseID)
    homebase = (Objective) vuDatabase->Find(homebaseID);
    */
    // sfr: xy order
    ::vector pos;
    ConvertGridToSim(x, y, &pos);
#ifdef VU_GRID_TREE_Y_MAJOR
    oit = new VuGridIterator(ObjProxList, pos.y, pos.x, (BIG_SCALAR)GridToSim(MAX_AIR_SEARCH * 2));
#else
    oit = new VuGridIterator(ObjProxList, pos.x, pos.y, (BIG_SCALAR)GridToSim(MAX_AIR_SEARCH * 2));
#endif

    o = (Objective) oit->GetFirst();

    while (o)
    {
        if (
            (o->GetType() == TYPE_AIRBASE or o->GetType() == TYPE_AIRSTRIP) and 
            o->GetTeam() == flight->GetTeam() and not o->IsNearfront()
        )
        {
            // 2002-04-17 MN modified -
            // be the alternate airstrip neither a target
            // airbase nor the home airbase (airlift have airbases as target)
            if (
                target and 
                (target->GetType() == TYPE_AIRBASE or target->GetType() == TYPE_AIRSTRIP) and 
                o->GetCampID() == target->GetCampID() or
                homebase and o->GetCampID() == homebase->GetCampID()
            )
            {
                o = (Objective) oit->GetNext();
                continue;
            }

            o->GetLocation(&ox, &oy);
            d = Distance(x, y, ox, oy);

            if (d < bd)
            {
                bd = d;
                bo = o;
            }
        }

        o = (Objective) oit->GetNext();
    }

    delete oit;
    return bo;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if 0

void Loadout::Add(LOADOUT *load)
{
    LOADOUT *cur;

    if (Root_ == NULL)
        Root_ = load;
    else
    {
        cur = Root_;

        while (cur->Next)
            cur = cur->Next;

        cur->Next = load;
    }
}

LOADOUT *Loadout::Find(long ID)
{
    LOADOUT *cur;

    cur = Root_;

    while (cur)
    {
        if (cur->ID == ID)
            return(cur);

        cur = cur->Next;
    }

    return(NULL);
}

void Loadout::Remove(long ID)
{
    LOADOUT *cur, *delme;

    if (Root_ == NULL) return;

    cur = Root_;

    if (Root_->ID == ID)
    {
        Root_ = cur->Next;
        delete cur;
    }
    else
    {
        while (cur->Next)
        {
            if (cur->Next->ID == ID)
            {
                delme = cur->Next;
                cur->Next = cur->Next->Next;
                delete delme;
                return;
            }

            cur = cur->Next;
        }
    }
}

void Loadout::RemoveAll()
{
    LOADOUT *cur, *prev;

    cur = Root_;

    while (cur)
    {
        prev = cur;
        cur = cur->Next;
        delete prev;
    }

    Root_ = NULL;
}

#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/*
   void fixup_flight (Flight flight)
   {
   WayPointClass
 *wp,
 *pwp;

 CampaignHeading
 h;

 GridIndex
 x,
 y,
 dx,
 dy,
 wx,
 wy,
 pwx,
 pwy;

 int
 z,
 ndt,
 dt,
 current_time;

 float
 heading;

 current_time = TheCampaign.CurrentTime;

 wp = flight->GetFirstUnitWP ();
 while ((wp) and (wp->GetWPDepartureTime () < current_time) and wp->GetWPAction() not_eq WP_LAND)
 {
 wp = wp->GetNextWP ();

 if (wp)
 {
 flight->SetCurrentUnitWP (wp);

 wp->GetWPLocation (&x, &y);
 flight->SetUnitDestination (x, y);
 }
 }

 if (wp)
 {
 pwp = wp->GetPrevWP ();

 if (pwp)
 {
 wp->GetWPLocation (&wx, &wy);
 if (wp->GetWPFlags() bitand WPF_HOLDCURRENT)
 z = pwp->GetWPAltitude ();
 else
 z = wp->GetWPAltitude ();
 pwp->GetWPLocation (&pwx, &pwy);

 dx = wx - pwx;
 dy = wy - pwy;

 ndt = current_time - pwp->GetWPDepartureTime ();

 dt = wp->GetWPDepartureTime () - pwp->GetWPDepartureTime ();

 if (ndt > dt)
 ndt = dt;

 dx = dx * ndt / dt;
 dy = dy * ndt / dt;

 heading = AngleTo (pwx,pwy,wx,wy);
h = DirectionTo(pwx,pwy,wx,wy);

flight->SetYPR(heading,0.0F,0.0F);
flight->SetLastDirection (h);

flight->SetUnitLastMove (current_time);

flight->SetLocation (pwx + dx, pwy + dy);
flight->SetUnitAltitude (z);
}
else
{
 wp->GetWPLocation (&x, &y);
 flight->SetLocation (x, y);
 flight->SetUnitDestination (x, y);
 pwp = wp->GetNextWP();
 if (pwp)
 {
 pwp->GetWPLocation(&wx,&wy);
 heading = AngleTo(x,y,wx,wy);
 flight->SetYPR(heading,0.0F,0.0F);
 }
}
}
}


void fixup_flight_starting_positions (void)
{
 VuListIterator
 iter (AllAirList);

 UnitClass
 *unit;

 unit = GetFirstUnit (&iter);

 while (unit)
 {
 if (unit->IsFlight())
 {
 fixup_flight((Flight)unit);
 }

 unit = GetNextUnit (&iter);
 }
}
*/

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FlightClass::SetLastDirection(uchar d)
{
    if (last_direction not_eq d)
    {
        SM_SCALAR y = d * 45.0f / RTD;
        SetYPR(y, 0, 0);
        last_direction = d;
        // sfr: alone this should be very rare...
        // so im setting it very low priority, so when movement happens,
        // direction will also get updated
        //MakeFlightDirty(DIRTY_LAST_DIRECTION, DDP[103].priority);
        MakeFlightDirty(DIRTY_LAST_DIRECTION, SEND_EVENTUALLY);
    }

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FlightClass::SetPackage(VU_ID id)
{
    if (package not_eq id)
    {
        package = id;
        //MakeFlightDirty (DIRTY_PACKAGE_ID, DDP[104].priority);
        MakeFlightDirty(DIRTY_PACKAGE_ID, SEND_SOON);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FlightClass::SetEvalFlag(uchar f, int reset)
{
    // 2002-02-19 ADDED BY S.G. If reset not_eq 0
    // (defaults to 0 so it doesn't interfere with previous behavior),
    // what we passed becomes the new eval_flag, no question asked
    if (reset)
    {
        if (eval_flags not_eq f)
        {
            eval_flags = f;
            //MakeFlightDirty (DIRTY_EVAL_FLAGS, DDP[105].priority);
            MakeFlightDirty(DIRTY_EVAL_FLAGS, SEND_EVENTUALLY);
        }
    }
    else
    {
        // END OF ADDED SECTION 2002-02-19
        //if ( not (eval_flags bitand f))
        if ((eval_flags bitand f) not_eq f) // JB 010711
        {
            eval_flags or_eq f;
            //MakeFlightDirty (DIRTY_EVAL_FLAGS, DDP[106].priority);
            MakeFlightDirty(DIRTY_EVAL_FLAGS, SEND_EVENTUALLY);
        }
    }
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// JPO - hacky flag
void FlightClass::ClearEvalFlag(uchar f)
{
    if ((eval_flags bitand f))
    {
        eval_flags and_eq compl f;
        //MakeFlightDirty (DIRTY_EVAL_FLAGS, DDP[107].priority);
        MakeFlightDirty(DIRTY_EVAL_FLAGS, SEND_EVENTUALLY);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FlightClass::SetAssignedTarget(VU_ID targetId)
{
    if (assigned_target not_eq targetId)
    {
        assigned_target = targetId;
        //MakeFlightDirty (DIRTY_ASSIGNED_TARGET, DDP[108].priority);
        MakeFlightDirty(DIRTY_ASSIGNED_TARGET, SEND_SOON);
    }
}

void FlightClass::ClearAssignedTarget(void)
{
    if (assigned_target not_eq FalconNullId)
    {
        // KCK: Send an AWACS message to return to regular waypoints
        // KCK NOTE: This will probably play even if you run into a closer enemy
        PlayDivertRadioCalls(NULL, DIVERT_CANCLED, this, 1);
    }

    assigned_target = FalconNullId;
    SetOverrideWP(NULL);
    priority = 0;
    //MakeFlightDirty (DIRTY_ASSIGNED_TARGET, DDP[109].priority);
    MakeFlightDirty(DIRTY_ASSIGNED_TARGET, SEND_SOON);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FlightClass::SetOverrideWP(WayPoint w, bool ReqHelpHint)
{
    // NOTE: This COPIES the waypoint instead of setting it.
    // This way the waypoint will always be accessable and thread safe (not
    // necessarily accurate, however)
    if (w)
        override_wp.CloneWP(w);
    else
        override_wp.SetWPAltitudeLevel(-1);

    if (ReqHelpHint) // M.N. mark this as a help request WP
        override_wp.SetWPFlag(WPF_REQHELP);

    // MakeFlightDirty (DIRTY_OVERRIDE, SEND_EVENTUALLY);
}

WayPoint FlightClass::GetOverrideWP(void)
{
    if (override_wp.GetWPAltitudeLevel() == -1)
        return NULL;

    return &override_wp;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FlightClass::MakeStoresDirty(void)
{
    //MakeFlightDirty (DIRTY_STORES, DDP[110].priority);
    MakeFlightDirty(DIRTY_STORES, SEND_SOON);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FlightClass::MakeFlightDirty(Dirty_Flight bits, Dirtyness score)
{
    if (( not IsLocal()) or (VuState() not_eq VU_MEM_ACTIVE))
    {
        return;
    }

    if ( not IsAggregate())
    {
        if (score >= SEND_NOW)
        {
            score = SEND_RELIABLEANDOOB;
        }
        else
        {
            score = SEND_NOW;
        }
    }

    dirty_flight or_eq bits;

    MakeDirty(DIRTY_FLIGHT, score);
    // MonoPrint ("MakeFlightDirty bits,  score  %08x", bits, score);//me123
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FlightClass::WriteDirty(unsigned char **stream)
{
    unsigned char *ptr;

    ptr = *stream;

    // Encode it up
    *(unsigned short *)ptr = dirty_flight;
    ptr += sizeof(unsigned short);

    // sfr: send last direction if position is going too
    if (dirty_flight bitand DIRTY_LAST_DIRECTION)
    {
        *(uchar*)ptr = last_direction;
        ptr += sizeof(uchar);
    }

    if (dirty_flight bitand DIRTY_PACKAGE_ID)
    {
        *(VU_ID*)ptr = package;
        ptr += sizeof(VU_ID);
    }

    if (dirty_flight bitand DIRTY_MISSION)
    {
        *(uchar*)ptr = mission;
        ptr += sizeof(uchar);
    }

    if (dirty_flight bitand DIRTY_PLANE_STATS)
    {
        memcpy(ptr, plane_stats, sizeof(uchar)*PILOTS_PER_FLIGHT);
        ptr += sizeof(uchar) * PILOTS_PER_FLIGHT;
    }

    if (dirty_flight bitand DIRTY_PILOTS)
    {
        memcpy(ptr, player_slots, sizeof(uchar)*PILOTS_PER_FLIGHT);
        ptr += sizeof(uchar) * PILOTS_PER_FLIGHT;
        memcpy(ptr, pilots, sizeof(uchar)*PILOTS_PER_FLIGHT);
        ptr += sizeof(uchar) * PILOTS_PER_FLIGHT;
    }

    if (dirty_flight bitand DIRTY_EVAL_FLAGS)
    {
        *(uchar*)ptr = eval_flags;
        ptr += sizeof(uchar);
    }

    if (dirty_flight bitand DIRTY_ASSIGNED_TARGET)
    {
        *(VU_ID*)ptr = assigned_target;
        ptr += sizeof(VU_ID);
    }

    if (dirty_flight bitand DIRTY_STORES)
    {
        *(uchar*)ptr = loadouts;
        ptr += sizeof(uchar);

        for (int i = 0; i < loadouts; i++)
        {
            memcpy(ptr, &loadout[i], sizeof(LoadoutStruct));
            ptr += sizeof(LoadoutStruct);
        }
    }

    if (dirty_flight bitand DIRTY_DIVERT_INFO)
    {
        *(uchar*)ptr = mission_context;
        ptr += sizeof(uchar);
        *(VU_ID*)ptr = requester;
        ptr += sizeof(VU_ID);
    }

    dirty_flight = 0;

    *stream = ptr;
}

///////////////////////////////////////////////////////////////////////////////
//sfr: yeah, changed here also
void FlightClass::ReadDirty(VU_BYTE **stream, long *rem)
{
    unsigned short bits;

    memcpychk(&bits, stream, sizeof(unsigned short), rem);

    if (bits bitand DIRTY_LAST_DIRECTION)
    {
        uchar ld;
        memcpychk(&ld, stream, sizeof(uchar), rem);
        SetLastDirection(ld);
        //SetYPR(last_direction*45*DTR,0.0F,0.0F);
    }

    if (bits bitand DIRTY_PACKAGE_ID)
    {
        memcpychk(&package, stream, sizeof(VU_ID), rem);
    }

    if (bits bitand DIRTY_MISSION)
    {
        memcpychk(&mission, stream, sizeof(uchar), rem);
    }

    if (bits bitand DIRTY_PLANE_STATS)
    {
        memcpychk(plane_stats, stream, sizeof(uchar)*PILOTS_PER_FLIGHT, rem);
    }

    if (bits bitand DIRTY_PILOTS)
    {
        memcpychk(player_slots, stream, sizeof(uchar)*PILOTS_PER_FLIGHT, rem);
        memcpychk(pilots, stream, sizeof(uchar)*PILOTS_PER_FLIGHT, rem);
    }

    if (bits bitand DIRTY_EVAL_FLAGS)
    {
        memcpychk(&eval_flags, stream, sizeof(uchar), rem);
    }

    if (bits bitand DIRTY_ASSIGNED_TARGET)
    {
        memcpychk(&assigned_target, stream, sizeof(VU_ID), rem);
    }

    if (bits bitand DIRTY_STORES)
    {
        memcpychk(&loadouts, stream, sizeof(uchar), rem);

        if (loadout)
        {
            delete loadout;
        }

        loadout = new LoadoutStruct[loadouts];

        for (int i = 0; i < loadouts; i++)
        {
            memcpychk(&loadout[i], stream, sizeof(LoadoutStruct), rem);
        }
    }

    if (bits bitand DIRTY_DIVERT_INFO)
    {
        memcpychk(&mission_context, stream, sizeof(uchar), rem);
        memcpychk(&requester, stream, sizeof(VU_ID), rem);
    }

}

///////////////////////////////////////////////////////////////////////////////
// sfr: @todo WTF does this have to do with FlightClass?? Should be in Objective class
int FlightClass::AirbaseOperational(Objective airbase)
{
    if (airbase == NULL)
    {
        return FALSE;//me123
    }

    //JPO - to be operational, it must
    // exist, be an objective, have serviceable runways
    // and be owned by the same team as us.
    if (airbase->IsUnit())
    {
        // JB carrier
        return TRUE;
    }

    if (airbase == NULL or not airbase->IsObjective())
    {
        return FALSE;
    }

    //if ( not IsHelicopter() and (airbase->brain == NULL or airbase->brain->NumOperableRunways() <= 0))
    //return FALSE;  Cobra test
    if (( not IsHelicopter()) and (airbase->brain == NULL))
    {
        return FALSE;
    }

    if (airbase->brain)
    {
        if (airbase->brain->NumOperableRunways() == 0)
        {
            return FALSE;
        }
    }

    //if (airbase->GetTeam() not_eq GetTeam())
    if ( not GetRoE(airbase->GetTeam(), GetTeam(), ROE_AIR_USE_BASES))
    {
        return FALSE;
    }

    return TRUE;
}

// 2002-02-25 ADDED BY S.G. FlightClass needs to have a combat class like aircrafts.
int FlightClass::CombatClass()
{
    return SimACDefTable[Falcon4ClassTable[GetVehicleID(0)].vehicleDataIndex].combatClass;
}
