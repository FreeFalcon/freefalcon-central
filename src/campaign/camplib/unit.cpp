#pragma warning (disable : 4786) // debug info truncation

#include <stdio.h>
#include <conio.h>
#include <stddef.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <math.h>


#include "Unit.h"
#include "CmpGlobl.h"
#include "ListADT.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "ASearch.h"
#include "Path.h"
#include "Find.h"
#include "vutypes.h"
#include "Campaign.h"
#include "ATM.h"
#include "GTM.h"
#include "CampList.h"
#include "campwp.h"
#include "update.h"
#include "loadout.h"
#include "gndunit.h"
#include "vehicle.h"
#include "team.h"
#include "initdata.h"
#include "f4find.h"
#include "F4Vu.h"
#include "AirUnit.h"
#include "GndUnit.h"
#include "NavUnit.h"
#include "AIInput.h"
#include "CUIEvent.h"
#include "Weather.h"
#include "PtData.h"
#include "CmpClass.h"
#include "MsgInc/AWACSMsg.h"
#include "MsgInc/CampDataMsg.h"
#include "MsgInc/AirTaskingMsg.h"
#include "MsgInc/CampWeaponFireMsg.h"
#include "MsgInc/RadioChatterMsg.h"
#include "MsgInc/SimCampMsg.h"
#include "MsgInc/FalconFlightPlanMsg.h"
#include "Persist.h"
#include "SimVeh.h"
#include "Sms.h"
#include "Hardpnt.h"
#include "CampStr.h"
#include "Utils/Lzss.h"
#include "FalcSess.h"
#include "SimDrive.h"
#include "classtbl.h"
#include "Camp2Sim.h"
#include "simdrive.h"
#include "otwdrive.h"
#include "simmover.h"
#include "f4error.h"
#include "Supply.h"
#include "Guns.h"
#include "CmpRadar.h"
#include "graphics/include/drawpnt.h"
#include "Graphics/Include/TMap.h"
#include "playerOp.h"
#include "Dogfight.h"
#include "atcbrain.h"
#include "dirtybits.h"
#include "GameMgr.h"
#include "digi.h"
#include "f4Version.h"
#include "debuggr.h"
#include "uiwin.h"
#include "aircrft.h"
#include "radarData.h"
#include "vu2/src/vu_priv.h"
#include "vucoll.h"
#include "Falclib/Include/IsBad.h"
/* 2001-03-15 S.G. 'CanDetect' */
#include "geometry.h"
/* 2001-03-20 S.G. 'CanDetect' */
#include "sim/include/radarData.h"
/* 2001-04-03 S.G. 'CanDetect' */
#include "flight.h"
#include "missile.h" // 2002-03-08 S.G.
#include "missdata.h" // 2002-03-08 S.G.

using namespace std;

extern bool g_bLimit2DRadarFight; // 2002-03-08 S.G.

// ============================================
// Defines and other nifty stuff
// ============================================
#ifdef USE_SH_POOLS
MEM_POOL UnitDeaggregationData::pool;
#endif
extern int g_nDeagTimer;
#define CAMPAIGN_LABELS

#define KEV_DEBUG 1
#ifdef DEBUG
#define KEEP_STATISTICS
#endif

#ifdef DEBUG
// #define DEAG_DEBUG
//#define DEBUG_COUNT // this can cause crashes when doing long debug runs 
#define KEEP_STATISTICS
int gUnitCount = 0;
int gCheckConstructFunction = 0;
#endif

#ifdef KEEP_STATISTICS
int AA_Kills = 0;
int AA_Shots = 0;
int GA_Kills = 0;
int GA_Shots = 0;
int NA_Kills = 0;
int NA_Shots = 0;
int GG_Kills = 0;
int GG_Shots = 0;
int AG_Kills = 0;
int AG_Shots = 0;
int ART_Kills = 0;
int ART_Shots = 0;
int AS_Kills = 0;
int AS_Shots = 0;
int AA_Saves = 0;
#endif

#define DEFAULT_MOVE    Tracked
extern float MIN_DEAD_PCT; // KCK: Strength below which a reaggregating vehicle will be considered dead

#define GND_OFFENSIVE_SUPPLY_TIME (20*CampaignMinutes)
#define GND_DEFENSIVE_SUPPLY_TIME (3*CampaignHours)
#define AIR_OFFENSIVE_SUPPLY_TIME ((ActionTimeOut/2)*CampaignHours) //A.S. 2001-12-09, to make cosistent with "ActionTimeOut"
#define AIR_DEFENSIVE_SUPPLY_TIME (ActionTimeOut*CampaignHours) //A.S. 2001-12-09, to make cosistent with "ActionTimeOut"

// #define AIR_OFFENSIVE_SUPPLY_TIME (12*CampaignHours) //A.S.
// #define AIR_DEFENSIVE_SUPPLY_TIME (24*CampaignHours) //A.S.

int CollectRad[] = {0, 20, 40, 40, 80, 150, 100};

int LoadingUnits = 0;

FILE
*save_log = 0,
 *load_log = 0;

int
start_save_stream,
start_load_stream;

extern short NumWeaponTypes;

// ============================================
// Externals
// ============================================

extern int gCurrentDataVersion;
extern unsigned char        SHOWSTATS;
extern SimulationDriver SimDriver;
extern int theirDomain;
extern float OffsetToMiddle;
extern int maxSearch;
extern short gRocketId;
extern long TeamSimColorList[NUM_TEAMS];
extern VU_ID gActiveFlightID;
extern bool g_bFireOntheMove; // FRB

extern bool g_bRealisticAttrition; // JB 010710

#ifdef CAMPTOOL
// Renaming tool stuff
extern VU_ID_NUMBER RenameTable[65536];
extern int gRenameIds;
#endif

_TCHAR* GetNumberName(int name_id, _TCHAR *buffer);
_TCHAR* GetSTypeName(int domain, int type, int stype, _TCHAR buffer[]);

#ifdef CAMPTOOL
extern void RedrawCell(MapData md, GridIndex x, GridIndex y);
extern void RedrawUnit(Unit u);
extern int DisplayOk(Unit u);
extern int displayCampaign;
#endif

//#ifdef DEBUG
//extern int gPlaceholderOnly;
//extern uchar ReservedIds[MAX_CAMP_ENTITIES];
//extern char gPlaceholderFile[80];
//int UnitsDeagg = 0;
//// JPO - keep track of units for memory leaks.
//#include <map>
//
//class UnitListCounter {
// typedef std::map<int, UnitClass *> ID2UNIT;
// int maxoccupancy;
//
// ID2UNIT unitlist;
// public:
// UnitListCounter() : maxoccupancy(0) { };
// void Report() {
// MonoPrint("%d (%d) units of %d left\n", gUnitCount, unitlist.size(), maxoccupancy);
// ID2UNIT::iterator it;
// for(it = unitlist.begin(); it not_eq unitlist.end(); it++) {
// UnitClass *ent = (*it).second;
// MonoPrint("Unit left %d campid %d ref %d\n", (*it).first,
// ent->GetCampId(), ent->RefCount());
// }
// unitlist.clear();
// }
// void AddObj(int id, UnitClass *entry) {
// unitlist[id] = entry;
// if (unitlist.size() > maxoccupancy)
// maxoccupancy = unitlist.size();
// };
// void DelObj(int id) {
// ID2UNIT::iterator it;
// it = unitlist.find(id);
// unitlist.erase(it);
// }
//};
//static UnitListCounter myunitlist;
//void UnitReport(void)
//{
// myunitlist.Report();
//}
//#endif

extern void UI_Refresh(void);
extern void update_active_flight(UnitClass *un);
extern void EvaluateKill(FalconDeathMessage *dtm, SimBaseClass *simShooter, CampBaseClass *campShooter, SimBaseClass *simTarget, CampBaseClass *campTarget);
extern int FriendlyTerritory(GridIndex x, GridIndex y, int team);

// -------------------------
// Local Function Prototypes
// =========================

int GetArrivalSpeed(const UnitClass *u);

// ============================================
// Class Functions
// ============================================

// constructors
UnitClass::UnitClass(ushort type, VU_ID_NUMBER id) : CampBaseClass(type, id)
{
    last_check = 0;
#if HOTSPOT_FIX
    update_interval = 0;
#endif
    roster = 0;
    unit_flags = 0;
    dest_x = dest_y = 0;
    target_id = FalconNullId;
    cargo_id = FalconNullId;
    moved = 0;
    losses = 0;
    tactic = 0;
    current_wp = 0;
    name_id = GetCampId();
    reinforcement = 0;
    odds = 50;
    wp_list = NULL;
    draw_pointer = NULL;
    class_data = (UnitClassDataType*) Falcon4ClassTable[type - VU_LAST_ENTITY_TYPE].dataPtr;
    dirty_unit = 0;

    SetUnitAltitude(0);
#ifdef DEBUG_COUNT
    gUnitCount++;
    myunitlist.AddObj(Id().num_, this);
#endif

    // JB SOJ
    sojSource = NULL;
    sojOctant = 0;
    sojRangeSq = 0.0;
}

UnitClass::UnitClass(VU_BYTE **stream, long *rem) : CampBaseClass(stream, rem)
{
#if HOTSPOT_FIX
    update_interval = 0;
#endif

    if (load_log)
    {
        fprintf(load_log, "%08x UnitClass ", *stream - start_load_stream);
        fflush(load_log);
    }

    //#ifdef CAMPTOOL
    // if (gRenameIds)
    // {
    // VU_ID new_id = FalconNullId;
    //
    // if (IsFlight() or IsPackage())
    // {
    // // Rename this ID
    // for (
    // new_id.num_ = FIRST_LOW_VOLATILE_VU_ID_NUMBER_1;
    // new_id.num_ < LAST_LOW_VOLATILE_VU_ID_NUMBER_2;
    // new_id.num_++
    // ){
    // if ( not vuDatabase->Find(new_id))
    // {
    // RenameTable[share_.id_.num_] = new_id.num_;
    // share_.id_ = new_id;
    // break;
    // }
    // }
    // }
    // else
    // {
    // // Rename this ID
    // for (new_id.num_ = FIRST_NON_VOLATILE_VU_ID_NUMBER; new_id.num_ < LAST_NON_VOLATILE_VU_ID_NUMBER; new_id.num_++)
    // {
    // if ( not vuDatabase->Find(new_id))
    // {
    // RenameTable[share_.id_.num_] = new_id.num_;
    // share_.id_ = new_id;
    // break;
    // }
    // }
    // }
    // }
    //#endif

    memcpychk(&last_check, stream, sizeof(CampaignTime), rem);
    memcpychk(&roster, stream, sizeof(fourbyte), rem);
    memcpychk(&unit_flags, stream, sizeof(fourbyte), rem);
    memcpychk(&dest_x, stream, sizeof(GridIndex), rem);
    memcpychk(&dest_y, stream, sizeof(GridIndex), rem);
    memcpychk(&target_id, stream, sizeof(VU_ID), rem);
#ifdef DEBUG
    // VU_ID_NUMBERs moved to 32 bits
    target_id.num_ and_eq 0xffff;
#endif

    if (gCampDataVersion > 1)
    {
        memcpychk(&cargo_id, stream, sizeof(VU_ID), rem);
#ifdef DEBUG
        // VU_ID_NUMBERs moved to 32 bits
        cargo_id.num_ and_eq 0xffff;
#endif
    }
    else
    {
        cargo_id = FalconNullId;
        SetCargo(0);
        SetRefused(0);
    }

    memcpychk(&moved, stream, sizeof(uchar), rem);
    memcpychk(&losses, stream, sizeof(uchar), rem);
    memcpychk(&tactic, stream, sizeof(uchar), rem);

    if (gCampDataVersion >= 71)
    {
        memcpychk(&current_wp, stream, sizeof(ushort), rem);
    }
    else
    {
        current_wp = 0;
        memcpychk(&current_wp, stream, sizeof(uchar), rem);
    }

    memcpychk(&name_id, stream, sizeof(short), rem);
    memcpychk(&reinforcement, stream, sizeof(short), rem);

    odds = 50;
    draw_pointer = NULL;

    if (load_log)
    {
        fprintf(load_log, "%08x Waypoints ", stream - start_load_stream);
        fflush(load_log);
    }

    wp_list = NULL;
    DecodeWaypoints(stream, rem);

    class_data = (UnitClassDataType*) Falcon4ClassTable[share_.entityType_ - VU_LAST_ENTITY_TYPE].dataPtr;
    dirty_unit = 0;

#ifdef DEBUG_COUNT
    gUnitCount++;
    myunitlist.AddObj(Id().num_, this);
#endif
    // JPO SOJ
    sojSource = NULL;
    sojOctant = 0;
    sojRangeSq = 0.0;
}

UnitClass::~UnitClass()
{
    ShiAssert( not draw_pointer);

    if (wp_list)
        DisposeWayPoints();

    wp_list = NULL;

    // Kill any cargo we were carrying
    Unit c;
    c = (Unit) GetCargo();

    if (c)
        c->KillUnit();


#ifdef DEBUG_COUNT
    gUnitCount--;
    myunitlist.DelObj(Id().num_);
#endif
}

int UnitClass::SaveSize(void)
{
    uchar wps = 0;
    int size = 0;
    WayPoint w;

    // Count waypoints
    // CampEnterCriticalSection();
    w = wp_list;

    while (w)
    {
        wps++;
        size += w->SaveSize();
        w = w->GetNextWP();
    }

    // CampLeaveCriticalSection();

    // return size
    size += CampBaseClass::SaveSize()
            + sizeof(CampaignTime)
            + sizeof(fourbyte)
            + sizeof(fourbyte)
            + sizeof(GridIndex)
            + sizeof(GridIndex)
            + sizeof(VU_ID)
            + sizeof(VU_ID)
            + sizeof(uchar)
            + sizeof(uchar)
            + sizeof(uchar)
            + sizeof(ushort)
            + sizeof(short)
            + sizeof(short)
            + sizeof(ushort);
    return size;
}

int UnitClass::Save(VU_BYTE **stream)
{
    CampBaseClass::Save(stream);

    if (save_log)
    {
        fprintf(save_log, "%08x UnitClass ", *stream - start_save_stream);
        fflush(save_log);
    }

    if ( not IsAggregate())
    {
        // KCK TODO: We need to send the deaggregated data as well
    }

    memcpy(*stream, &last_check, sizeof(CampaignTime));
    *stream += sizeof(CampaignTime);
    memcpy(*stream, &roster, sizeof(fourbyte));
    *stream += sizeof(fourbyte);
    memcpy(*stream, &unit_flags, sizeof(fourbyte));
    *stream += sizeof(fourbyte);
    memcpy(*stream, &dest_x, sizeof(GridIndex));
    *stream += sizeof(GridIndex);
    memcpy(*stream, &dest_y, sizeof(GridIndex));
    *stream += sizeof(GridIndex);
#ifdef CAMPTOOL

    if (gRenameIds)
        target_id.num_ = RenameTable[target_id.num_];

#endif
    memcpy(*stream, &target_id, sizeof(VU_ID));
    *stream += sizeof(VU_ID);
#ifdef CAMPTOOL

    if (gRenameIds)
        cargo_id.num_ = RenameTable[cargo_id.num_];

#endif
    memcpy(*stream, &cargo_id, sizeof(VU_ID));
    *stream += sizeof(VU_ID);
    memcpy(*stream, &moved, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, &losses, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, &tactic, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, &current_wp, sizeof(ushort));
    *stream += sizeof(ushort);
    memcpy(*stream, &name_id, sizeof(short));
    *stream += sizeof(short);
    memcpy(*stream, &reinforcement, sizeof(short));
    *stream += sizeof(short);

    if (save_log)
    {
        fprintf(save_log, "%08x Waypoints ", *stream - start_save_stream);
        fflush(save_log);
    }

    EncodeWaypoints(stream);

    return SaveSize();
}

int UnitClass::Handle(VuFullUpdateEvent *event)
{
    // copy data from temp entity to current entity
    UnitClass* tmp_ent = (UnitClass*)(event->expandedData_.get());

    DisposeWayPoints();

    memcpy(&last_check, &tmp_ent->last_check, sizeof(CampaignTime));
    memcpy(&roster, &tmp_ent->roster, sizeof(fourbyte));
    memcpy(&unit_flags, &tmp_ent->unit_flags, sizeof(fourbyte));
    memcpy(&dest_x, &tmp_ent->dest_x, sizeof(GridIndex));
    memcpy(&dest_y, &tmp_ent->dest_y, sizeof(GridIndex));
    memcpy(&target_id, &tmp_ent->target_id, sizeof(VU_ID));
    memcpy(&cargo_id, &tmp_ent->cargo_id, sizeof(VU_ID));
    memcpy(&moved, &tmp_ent->moved, sizeof(uchar));
    memcpy(&losses, &tmp_ent->losses, sizeof(uchar));
    memcpy(&tactic, &tmp_ent->tactic, sizeof(uchar));
    memcpy(&current_wp, &tmp_ent->current_wp, sizeof(ushort));
    memcpy(&name_id, &tmp_ent->name_id, sizeof(short));
    memcpy(&reinforcement, &tmp_ent->reinforcement, sizeof(short));

    wp_list = tmp_ent->wp_list;
    tmp_ent->wp_list = NULL;

    SetPosition(tmp_ent->XPos(), tmp_ent->YPos(), tmp_ent->ZPos());

    return CampBaseClass::Handle(event);
}

void UnitClass::SendUnitMessage(VU_ID id, short msg, short d1, short d2, short d3)
{
    VuTargetEntity* target = (VuTargetEntity*) vuDatabase->Find(OwnerId());
    FalconUnitMessage* um = new FalconUnitMessage(Id(), target);
    um->dataBlock.from = id;
    um->dataBlock.message = msg;
    um->dataBlock.data1 = d1;
    um->dataBlock.data2 = d2;
    um->dataBlock.data3 = d3;
    FalconSendMessage(um, TRUE);
}

void UnitClass::BroadcastUnitMessage(VU_ID id, short msg, short d1, short d2, short d3)
{
    FalconUnitMessage* um = new FalconUnitMessage(Id(), FalconLocalGame);
    um->dataBlock.from = id;
    um->dataBlock.message = msg;
    um->dataBlock.data1 = d1;
    um->dataBlock.data2 = d2;
    um->dataBlock.data3 = d3;
    FalconSendMessage(um, TRUE);
}

// This Apply damage is called only for local entities, and resolves the number and type
// of weapon shots vs this unit.
// HOWEVER, it them broadcasts a FalconWeaponFireMessage which will generate visual effects,
// update remote copies of this entity, call the mission evaluation/event storage routines,
// and add any craters we require.
int UnitClass::ApplyDamage(FalconCampWeaponsFire *cwfm, uchar bonusToHit)
{
    MoveType mt;
    Int32 i, hc, range, shot, currentLosses = 0;
    int str = 0, strength, flags, totalShots = 0;
    float bonus;
    DamType dt;
    GridIndex sx, sy, tx, ty;
    Unit shooter = (Unit)vuDatabase->Find(cwfm->dataBlock.shooterID);
    uchar addcrater = 0;

    if ( not IsLocal())
        return 0;

    // 2001-08-06 ADDED BY S.G. FIXING A POSSIBLE CTD. shooter SHOULD NOT BE NULL BUT IT WAS ONCE. NO TIME TO FIX SO HACKING IT.
    if ( not shooter)
        return 0;

    // Since this person is shooting at us, check if they're a higher concern than our current target.
    // If so, set our target to them and choose a new tactic.
    // 2001-07-31 MODIFIED BY S.G. SO GROUND UNIT ARE NOT CAUSING A REACTION.
    // if (IsFlight() and shooter->Id() not_eq GetTargetID())
    if (IsFlight() and not shooter->OnGround() and shooter->Id() not_eq GetTargetID())
    {
        float d;
        int c, s, e, r = 0;
        CampEntity ct = GetCampTarget();

        if (ct)
            r = ((Flight)this)->DetectVs(ct, &d, &c, &s, &e);

        if (((Flight)this)->DetectVs(shooter, &d, &c, &s, &e) > r)
        {
            // MonoPrint("Unit %d auto-engaging shooter %d.\n",GetCampID(),shooter->GetCampID());
            SetTarget(shooter);
            SetEngaged(1);
            ChooseTactic();
        }
    }

    // Bonus for player performance
    bonus = CombatBonus(shooter->GetTeam(), FalconNullId);

    ShiAssert(IsAggregate());

    gDamageStatusPtr = gDamageStatusBuffer + 1;
    gDamageStatusBuffer[0] = 0;
    shooter->GetLocation(&sx, &sy);
    GetLocation(&tx, &ty);
    range = FloatToInt32(Distance(sx, sy, tx, ty));
    mt = GetMovementType();
    SetEngaged(1);

    for (i = 0; i < MAX_TYPES_PER_CAMP_FIRE_MESSAGE and cwfm->dataBlock.weapon[i] and cwfm->dataBlock.shots[i]; i++)
    {
        hc = GetWeaponHitChance(cwfm->dataBlock.weapon[i], mt, range) + bonusToHit;

        // Flight's get bonuses to hit based on vehicle type (ground vehicles should too -
        // but at this point, we don't really know which vehicle shot which weapon)

        // A.S.
        if ( not CampBugFixes)
        {
            if (shooter->IsFlight())
                hc += GetVehicleClassData(shooter->GetVehicleID(0))->HitChance[mt];

            // If target is a flight, it get's it's air to air manueverability as a defensive bonus
            if (IsFlight() and hc > 5)
                hc -= GetVehicleClassData(GetVehicleID(0))->HitChance[mt];
        }

        else
        {
            // A.S. 2001-12-09, begin
            if (shooter->IsFlight() and IsFlight())  // Flights get only bonus for air targets as ground targets get no defensive bonus
                hc += GetVehicleClassData(shooter->GetVehicleID(0))->HitChance[mt];

            if (IsFlight() and hc > 5 and shooter->IsFlight())
                hc -= GetVehicleClassData(GetVehicleID(0))->HitChance[mt]; // no defensive bonus for flights, if shooter is on ground
        }

        // this makes AAA and SAMs more dangerous in 2D
        if (shooter->IsFlight() and (WeaponDataTable[cwfm->dataBlock.weapon[i]].Flags bitand WEAP_ONETENTH))
            hc = FloatToInt32(hc / 3.0F); // reduction for aircraft guns, which are heavily over-modeled

        // end added section


        // HARMs will snap to current radar vehicle, if we're emitting
        if ((WeaponDataTable[cwfm->dataBlock.weapon[i]].GuidanceFlags bitand WEAP_ANTIRADATION))// and IsEmitting()) Leonr Change for HARMS
        {
            cwfm->dataBlock.dPilotId = class_data->RadarVehicle;
            hc += 90; // KCK: Hackish - add a bonus if they kept their radar on.
        }

        hc = FloatToInt32(bonus * hc);

        // COMMENTED OUT BY S.G. NO MORE STUPID HACK
        // Commented back in by JB 010224 -- Caused high attrition rates

        if (shooter->IsFlight())
        {
            if (IsFlight())
                hc = FloatToInt32(hc / HitChanceAir); // default 3.5F -> Falcon4.AII
            else
                hc = FloatToInt32(hc / HitChanceGround); // default 1.5F
        }

        // END HACK

        if (hc < 1)
            hc = 1; // Minimum chance to hit of 1%

        // Tally the losses
        strength = GetWeaponStrength(cwfm->dataBlock.weapon[i]);
        str = 0;
        dt = (DamType)GetWeaponDamageType(cwfm->dataBlock.weapon[i]);

        // 2002-03-22 MN minimum of 95% hitchance for nukes
        if (dt == NuclearDam)
            hc = max(95, hc);

        flags = GetWeaponFlags(cwfm->dataBlock.weapon[i]);
        shot = 0;

        while (cwfm->dataBlock.shots[i] - shot > 0)
        {
            totalShots ++;

            if (rand() % 100 < hc)
            {
                str += strength;
                currentLosses += ApplyDamage(dt, &str, cwfm->dataBlock.dPilotId, flags);
            }
            else if (addcrater < 2 and GetDomain() == DOMAIN_LAND)
            {
                // Add a few craters for kicks
                addcrater++;
            }

            shot += 1 + (rand() % (currentLosses + 1)); // Random stray shots - let's be nice
        }
    }

    // Check morale
    MoraleCheck(totalShots, currentLosses);
    *gDamageStatusPtr = (uchar)GetUnitMorale();
    gDamageStatusPtr++;

#ifdef DEBUG
    // if (shooter->IsFlight())
    // MonoPrint("%d (%d,%d) took %d losses from %d (%d,%d). range = %d\n",GetCampID(),tx,ty,currentLosses,shooter->GetCampID(),sx,sy,range);
#endif

#ifdef KEEP_STATISTICS

    if (IsFlight())
    {
        if (shooter->IsFlight())
        {
            AA_Kills += currentLosses;
            AA_Shots += totalShots;
        }
        else if (shooter->IsBattalion())
        {
            GA_Kills += currentLosses;
            GA_Shots += totalShots;
        }
        else
        {
            NA_Kills += currentLosses;
            NA_Shots += totalShots;
        }
    }
    else if (IsBattalion())
    {
        if (shooter->IsFlight())
        {
            AG_Kills += currentLosses;
            AG_Shots += totalShots;
        }
        else if (shooter->IsBattalion() and shooter->GetUnitNormalRole() == GRO_FIRESUPPORT)
        {
            ART_Kills += currentLosses;
            ART_Shots += totalShots;
        }
        else
        {
            GG_Kills += currentLosses;
            GG_Shots += totalShots;
        }
    }

    //FILE *deb;    // A.S. debug
    //deb = fopen("c:\\temp\\stat.txt", "w");
    //fprintf(deb, "GG_Kills = %d  GA_Kills = %d  ART_Kills = %d  AA_Kills = %d  AG_Kills = %d  Time = %d\n", GG_Kills, GA_Kills, ART_Kills, AA_Kills, AG_Kills, TheCampaign.CurrentTime/(3600*1000));
    //fclose(deb);
    // end debug
#endif

    // Record # of craters to add
    *gDamageStatusPtr = addcrater;
    gDamageStatusPtr++;

    // Record the final state, to keep remote entities consitant
    memcpy(gDamageStatusPtr, &roster, sizeof(fourbyte));
    gDamageStatusPtr += sizeof(fourbyte);

    cwfm->dataBlock.size = gDamageStatusPtr - gDamageStatusBuffer;
    cwfm->dataBlock.data = new uchar[cwfm->dataBlock.size];
    memcpy(cwfm->dataBlock.data, gDamageStatusBuffer, cwfm->dataBlock.size);

    // Send the weapon fire message (with target's post-damage status)
    FalconSendMessage(cwfm, FALSE);

    return currentLosses;
}

// This ApplyDamage() simply chooses which vehicles to eliminate and sets their status appropriately
int UnitClass::ApplyDamage(DamType d, int* str, int where, short flags)
{
    int v, vehs, n, hp, count = 0, lost = 0;
    VehicleClassDataType *vc;
    GridIndex x, y;
    int cov, pilot, actually_lost = 0;
    UnitClassDataType* uc = GetUnitClassData();

    // Determine ground cover for land units
    if (GetDomain() == DOMAIN_LAND)
    {
        GetLocation(&x, &y);
        cov = GetGroundCover(GetCell(x, y)) / 2;

        if (Moving())
        {
            cov /= 2;
        }

        if (cov < 1)
        {
            cov = 1;
        }
    }
    else
    {
        cov = 1;
    }

    // Make sure we hit something if our target is gone
    if (where not_eq 255 and not GetNumVehicles(where))
    {
        where = 255;
    }

    // Start the pounding
    vehs = GetTotalVehicles();

    while (*str > 0 and vehs and count < MAX_DAMAGE_TRIES)
    {
        // Small units can avoid damage simply by not finding a vehicle
        if (where not_eq 255)
        {
            v = where;
        }
        else
        {
            v = rand() % VEHICLE_GROUPS_PER_UNIT;
        }

        n = GetNumVehicles(v);

        if (n > 0 and uc->VehicleType[v])
        {
            vc = GetVehicleClassData(uc->VehicleType[v]);

            if (vc->DamageMod[d])
            {
                // Normalize hitpoints by damage type
                hp = vc->HitPoints * 100 / vc->DamageMod[d];

                // Check if high explosive damage will do more
                if ((flags bitand WEAP_AREA) and vc->DamageMod[HighExplosiveDam] > vc->DamageMod[d])
                    hp = vc->HitPoints * 100 / vc->DamageMod[HighExplosiveDam];

                if (*str > hp or rand() % hp < *str)
                {
                    lost++;
                    actually_lost++;
                    vehs--;
                    SetNumVehicles(v, n - 1);
                    *gDamageStatusPtr = (uchar)v;
                    gDamageStatusPtr++;
                    gDamageStatusBuffer[0]++;
                    where = 255;

                    if ( not (flags bitand WEAP_AREA)) // Not area effect weapon, only get one kill per shot
                        *str = 0;

                    // Flight related stuff to do:
                    if (IsFlight())
                    {
                        // Pick a pilot to kill
                        pilot = ((Flight)this)->PickRandomPilot(0);

                        // COMMENTED OUT BY S.G. DON'T RTB WHEN KILLED
                        //   I'M ALSO FORCING A KIA INSTEAD OF MIA. STATISTIC CHANGES MIGHT BE REQUIRED
                        // JB 010228 Commented back in -- attrition rates way too high
                        // JB 010710 Make configurable
                        if ( not g_bRealisticAttrition and rand() % 2)
                        {
                            ((Flight)this)->plane_stats[pilot] = AIRCRAFT_RTB; // Aborted and returned to base
                            actually_lost--;
#ifdef KEEP_STATISTICS
                            AA_Saves++;
#endif
                        }
                        else if ( not g_bRealisticAttrition and rand() % 3)
                            ((Flight)this)->plane_stats[pilot] = AIRCRAFT_MISSING; // Missing
                        else
                            ((Flight)this)->plane_stats[pilot] = AIRCRAFT_DEAD; // Killed

                        //((Flight)this)->MakeFlightDirty (DIRTY_PLANE_STATS, DDP[32].priority);
                        ((Flight)this)->MakeFlightDirty(DIRTY_PLANE_STATS, SEND_RELIABLE);

                        *gDamageStatusPtr = (uchar)pilot;
                        gDamageStatusPtr++;
                        *gDamageStatusPtr = ((Flight)this)->plane_stats[pilot];
                        gDamageStatusPtr++;
                    }
                    else
                        pilot = 255;
                }

                if (flags bitand WEAP_AREA) // Area effect - halve strength and have another go.
                {
                    // Note: we halve the strength whether or not we killed the target
                    *str /= 2;

                    if (*str < MINIMUM_STRENGTH)
                        count = MAX_DAMAGE_TRIES; // Stop causing damage eventually, for efficiency
                }
            }
        }

        count++;
    }

    // Check for death
    if ( not vehs and not IsDead())
    {
#ifdef KEV_DEBUG
        // MonoPrint("Unit %d destroyed\n",GetCampID());
#endif

        if (IsBattalion())
            TransferInitiative(GetTeam(), GetEnemyTeam(GetTeam()), 1);

        KillUnit();
    }

    losses += lost;
    return actually_lost;
}

// This is where the guts of the damage routine take place.
// All players handle this message in order to keep unit status and event messages consistant
int UnitClass::DecodeDamageData(uchar *data, Unit shooter, FalconDeathMessage *dtm)
{
    int lost, i, n, v, pilot = 255, islocal = IsLocal(), addhulk = 0;
    uchar addcrater;

    // This 'decode' is called for both local and remote entities.
    // However, Local entities have already suffered the losses, so
    // Only need to do the kill evaluation
    lost = *data;
    data++;

    for (i = 0; i < lost; i++)
    {
        v = *data;
        data++;

        if ( not islocal)
        {
            // Score the kill
            n = GetNumVehicles(v);

            if (n)
            {
                SetNumVehicles(v, n - 1);
                losses++;
            }
        }

        if ( not addhulk and IsBattalion())
            addhulk = Falcon4ClassTable[GetVehicleID(v)].visType[VIS_DESTROYED];

        // Extra data for flights
        if (IsFlight())
        {
            pilot = *data;
            data++;
            ((Flight)this)->plane_stats[pilot] = *data;
            data++;
            //((Flight)this)->MakeFlightDirty (DIRTY_PLANE_STATS, DDP[33].priority);
            ((Flight)this)->MakeFlightDirty(DIRTY_PLANE_STATS, SEND_RELIABLE);
        }

        // Check for radar being down
        if (class_data->RadarVehicle == v and IsEmitting())
            SetEmitting(0);

        // Generate a death message if we or the shooter is a member of the package
        if (dtm)
        {
            dtm->dataBlock.dPilotID = pilot;
            dtm->dataBlock.dIndex = GetVehicleID(v) + VU_LAST_ENTITY_TYPE;
            // Update squadron and pilot statistics
            EvaluateKill(dtm, NULL, shooter, NULL, this);
        }
        else if (shooter->IsFlight())
            EvaluateKill(dtm, NULL, shooter, NULL, this);
    }

    SetUnitMorale(*data);
    data++;

    // Add any necessary craters/hulks
    addcrater = *data;
    data++;
    ShiAssert(addcrater < 10);

    if (addhulk)
        AddHulk(this, addhulk);

    if (addcrater)
        AddMissCraters(this, addcrater);

    // Record the final state, to keep remote entities consitant
    if ( not islocal)
        memcpy(&roster, data, sizeof(fourbyte));

    data += sizeof(fourbyte);

    // Special case shit to do..
    if (lost and IsFlight())
        UpdateSquadronStatus((Flight)this, FALSE, TRUE);
    else if (lost and islocal and GetDomain() == DOMAIN_SEA and GetType() == TYPE_TASKFORCE and GetSType() == STYPE_UNIT_CARRIER and not GetNumVehicles(0))
    {
        // A carrier is missing
        // Probably be cool to send a special news event here...
        TeamInfo[GetTeam()]->atm->SendATMMessage(Id(), GetTeam(), FalconAirTaskingMessage::atmZapAirbase, 0, 0, NULL, 0);
    }

    return lost;
}

// Units make a moral check whenever they take a loss
int UnitClass::MoraleCheck(int shot, int lost)
{
    int morale, vehs;

    if (IsDead() or not shot)
    {
        return 0;
    }

    vehs = GetTotalVehicles();

    if ( not vehs)
    {
        return 0;
    }

    morale = GetUnitMorale();

    if (morale > 0) // If we're tracking morale
    {
        // Subtract morale equal to our losses (% of current forces)
        morale -= (200 * lost / (lost + vehs)) + (shot + 9) / 10;

        if (morale < 1)
        {
            morale = 1;
        }

        if (IsBattalion())
        {
            SetUnitMorale(morale);
        }
    }
    else
    {
        // Otherwise come up with a decent % chance
        morale = (100 * (vehs + losses)) / (shot + lost * 2 + losses * 2);
    }

    // Check for a break in morale
    if ((morale < 65) and (morale < (rand() % 65)))
    {
        if (lost)
        {
            SetLosses(1); // Flag unit as taking losses
        }

        SetBroken(1);
#ifdef KEV_DEBUG
        // MonoPrint("Unit %d broken %d%% chance.\n",GetCampID(),100-morale);
#endif
        // Trigger a tactics check
        ChooseTactic();
        return 0;
    }

    return 1;
}

void UnitClass::SendDeaggregateData(VuTargetEntity *target)
{
    if (IsAggregate())
    {
        return;
    }

    uchar *ddptr;
    short wv;
    ushort value, wps;
    FalconSimCampMessage *msg;
    SimVehicleClass *vehicle = NULL;
    VU_ID_NUMBER num;
    VU_SESSION_ID addr;
    int totalsize, onesize, i, tc = 0, tr = 0, v/*,classID*/;
    WayPoint w;
    VU_ID vuid;
    long fuel;
    float pos, z;
    //VehicleClassDataType *vc;

    // Calculate unit data size
    wps = 0;
    totalsize = 0;

    w = GetFirstUnitWP();

    while (w)
    {
        totalsize += w->SaveSize();
        wps++;
        w = w->GetNextWP();
    }

    totalsize +=
        sizeof(VU_SESSION_ID) + sizeof(VU_SESSION_ID) + sizeof(fourbyte) +
        sizeof(fourbyte) + sizeof(float) + sizeof(ushort) + sizeof(ushort)
        ;

    if (IsFlight())
    {
        totalsize += sizeof(long) + sizeof(uchar) +
                     GetNumberOfLoadouts() * sizeof(short) * HARDPOINT_MAX +
                     GetNumberOfLoadouts() * sizeof(uchar) * HARDPOINT_MAX
                     ;
    }

    // Calculate per vehicle data size
    onesize =  sizeof(uchar) + sizeof(VU_ID_NUMBER) + sizeof(float) + sizeof(float) + sizeof(float) + sizeof(short);

    for (v = 0; v < VEHICLE_GROUPS_PER_UNIT; v++)
    {
        tr += GetNumVehicles(v);
    }

    /* sfr: not necessary
    VuListIterator myit(GetComponents());
    vehicle = (SimVehicleClass*) myit.GetFirst();
    while (vehicle){
     tc++;
     vehicle = (SimVehicleClass*) myit.GetNext();
    }*/
    totalsize += tr * onesize;

    msg = new FalconSimCampMessage(Id(), target);
    msg->dataBlock.message = FalconSimCampMessage::simcampDeaggregateFromData;
    msg->dataBlock.from = GetDeagOwner();
    msg->dataBlock.size = totalsize;
    msg->dataBlock.data = new uchar[totalsize];
    ddptr = msg->dataBlock.data;
    // pass some status stuff along, so we're on the same page.
    addr = GetDeagOwner().creator_;
    memcpy(ddptr, &addr, sizeof(VU_SESSION_ID));
    ddptr += sizeof(VU_SESSION_ID);
    addr = FalconLocalSession->Id().creator_;
    memcpy(ddptr, &addr, sizeof(VU_SESSION_ID));
    ddptr += sizeof(VU_SESSION_ID);
    memcpy(ddptr, &unit_flags, sizeof(fourbyte));
    ddptr += sizeof(fourbyte);
    memcpy(ddptr, &roster, sizeof(fourbyte));
    ddptr += sizeof(fourbyte);
    z = ZPos();
    memcpy(ddptr, &z, sizeof(float));
    ddptr += sizeof(float);
    value = (ushort) current_wp - 1;
    memcpy(ddptr, &value, sizeof(ushort));
    ddptr += sizeof(ushort);
    memcpy(ddptr, &wps, sizeof(ushort));
    ddptr += sizeof(ushort);
    w = GetFirstUnitWP();

    while (w)
    {
        w->Save(&ddptr);
        w = w->GetNextWP();
    }

    // Send the loadout array for anything which has it (probably only flights)
    if (IsFlight())
    {
        fuel = GetBurntFuel();
        memcpy(ddptr, &fuel, sizeof(long));
        ddptr += sizeof(long);
        value = (uchar) GetNumberOfLoadouts();
        memcpy(ddptr, &value, sizeof(uchar));
        ddptr += sizeof(uchar);

        for (v = 0; v < value; v++)
        {
            for (i = 0; i < HARDPOINT_MAX; i++)
            {
                wv = (short) GetUnitWeaponId(i, v);
                memcpy(ddptr, &wv, sizeof(short));
                ddptr += sizeof(short);
                wv = (uchar) GetUnitWeaponCount(i, v);
                memcpy(ddptr, &wv, sizeof(uchar));
                ddptr += sizeof(uchar);
            }
        }
    }

    {
        VuListIterator myit(GetComponents());

        for (
            vehicle = static_cast<SimVehicleClass*>(myit.GetFirst());
            vehicle not_eq NULL;
            vehicle = static_cast<SimVehicleClass*>(myit.GetNext())
        )
        {
            //sfr: not needed
            //classID = vehicle->Type() - VU_LAST_ENTITY_TYPE;
            //vc = GetVehicleClassData(classID);
            uchar slot = (uchar)vehicle->GetSlot();
            memcpy(ddptr, &slot, sizeof(uchar));
            ddptr += sizeof(uchar);
            num = vehicle->Id().num_;
            memcpy(ddptr, &num, sizeof(VU_ID_NUMBER));
            ddptr += sizeof(VU_ID_NUMBER);
            pos = vehicle->XPos();
            memcpy(ddptr, &pos, sizeof(float));
            ddptr += sizeof(float);
            pos = vehicle->YPos();
            memcpy(ddptr, &pos, sizeof(float));
            ddptr += sizeof(float);
            pos = vehicle->Yaw();
            memcpy(ddptr, &pos, sizeof(float));
            ddptr += sizeof(float);

            if (vehicle->IsAirplane())
            {
                if (((AircraftClass*)vehicle)->DBrain())
                {
                    num = ((AircraftClass*)vehicle)->DBrain()->GetTaxiPoint();
                }
                else
                {
                    num = 0;
                }
            }
            else
            {
                num = 0;
            }

            memcpy(ddptr, &num, sizeof(short));
            ddptr += sizeof(short);
        }
    }

    // Send Deaggregation data to everyone in the group
    FalconSendMessage(msg, TRUE);
}

int UnitClass::RecordCurrentState(FalconSessionEntity *session, int byReag)
{
    VehicleClassDataType *vehicle_class_data;

    int v, have = 0, total = 0, vehleft = 0, lastPilot = -1, pilotSlot = 0;
    long fuelUsed, maxFuelUsed = -64000;
    Flight fl = NULL;
    LoadoutStruct *loadData[PILOTS_PER_FLIGHT] = { 0 };

    int  hasECM;

    vehicle_class_data = GetVehicleClassData(class_data->VehicleType[0]);

    if (vehicle_class_data)
    {
        hasECM = vehicle_class_data->Flags bitand VEH_HAS_JAMMER ? TRUE : FALSE;
    }
    else
    {
        hasECM = FALSE;
    }

    SetRoster(0);

    // Check if it's died in the meantime
    if (IsDead())
        return 0;

    // Assume unaccounted for pilots are MIA, if this is a flight
    if (IsFlight())
    {
        fl = (Flight)this;

        for (v = 0; v < PILOTS_PER_FLIGHT; v++)
        {
            if (fl->plane_stats[v] == AIRCRAFT_AVAILABLE)
            {
                fl->plane_stats[v] = AIRCRAFT_DEAD;
                //fl->MakeFlightDirty (DIRTY_PLANE_STATS, DDP[34].priority);
                fl->MakeFlightDirty(DIRTY_PLANE_STATS, SEND_RELIABLE);
            }
        }
    }

    // Record current state of components
    if (GetComponents())
    {
        SimVehicleClass *vehicle;
        int num, hp, vid;
        SMSBaseClass *SMS;

        // Now plop the vehicles back in
        VuListIterator myit(GetComponents());
        vehicle = (SimVehicleClass*) myit.GetFirst();

        while (vehicle)
        {
            if (F4IsBadReadPtr(vehicle, sizeof(SimVehicleClass)))
            {
                vehicle = (SimVehicleClass*) myit.GetNext();
                continue;
            }

            if (session)
            {
                // FRB - CTD's here
                if ( not F4IsBadReadPtr(session, sizeof(FalconSessionEntity)))
                {
                    if ( not vehicle->IsSetFalcFlag(FEC_HASPLAYERS))
                    {
                        // if it has players - then they are responsible for changing owner - not the campaign entity.
                        vehicle->ChangeOwner(session->Id());
                    }
                }
            }

            v = vehicle->GetSlot();
            vid = GetVehicleID(v);
            ShiAssert(GetNumVehicles(v) < 3);

            // KCK: When we're reaggregating a unit, there are some cases where we want to
            // make sure a half dead or pilotless vehicle is recorded as being dead.
            //
            // Check if this vehicle is dead according to the sim, and simply ignore it, if so.
            // (This will cause it not be added back into the campaign data)
            if (byReag and vehicle->IsDead() and not vehicle->IsSetFalcFlag(FEC_REGENERATING))
            {
                // Do nothing
            }
            // Check if this vehicle is essentially out of action for the Campaign's purpose (even if it is
            // technically alive in the sim). Theoretically, we could register this fact with the mission evaluator.
            // (This will also cause it not be added back into the campaign data)
            else if (byReag and ( not vehicle->HasPilot() or vehicle->pctStrength < MIN_DEAD_PCT) and not vehicle->IsSetFalcFlag(FEC_REGENERATING))
            {
                // I once tried simulating a death message here, but it was painfull and error prone.
                // What will happen without this, is if a unit reaggregates while something is 'mostly
                // dead' it will be removed from the unit, but the fact it died (or the kill credit)
                // will not be recorded in the mission evaluator.
                // Maybe this is ok, or some day I could work out how to fake this death message.
                /* FalconDeathMessage *dtm = new FalconDeathMessage (Id(), FalconLocalGame);
                 dtm->dataBlock.dEntityID = dtm->dataBlock.fEntityID = vehicle->Id();
                 dtm->dataBlock.dCampID = dtm->dataBlock.fCampID = GetCampID();
                 dtm->dataBlock.dSide = dtm->dataBlock.fSide = GetOwner();
                 dtm->dataBlock.dPilotID = dtm->dataBlock.fPilotID = vehicle->pilotSlot;
                 dtm->dataBlock.dIndex = dtm->dataBlock.fIndex = vehicle->Type();
                 dtm->dataBlock.fWeaponID = ????;
                 dtm->dataBlock.fWeaponUID = FalconNullId;
                 dtm->dataBlock.damageType = FalconDamageType::OtherDamage;
                 dtm->dataBlock.deathPctStrength = 0;

                 FalconEntity *lastToHit = (FalconEntity*)vuDatabase->Find( vehicle->LastShooter() );
                 if (lastToHit and not lastToHit->IsEject() )
                 {
                 if (lastToHit->IsSim())
                 {
                 dtm->dataBlock.fPilotID = ((SimVehicleClass*)lastToHit)->pilotSlot;
                 dtm->dataBlock.fCampID = ((SimVehicleClass*)lastToHit)->GetCampaignObject()->GetCampID();
                 dtm->dataBlock.fSide = ((SimVehicleClass*)lastToHit)->GetCampaignObject()->GetOwner();
                 }
                 else
                 {
                 dtm->dataBlock.fPilotID = 0;
                 dtm->dataBlock.fCampID = ((CampBaseClass*)lastToHit)->GetCampID();
                 dtm->dataBlock.fSide = ((CampBaseClass*)lastToHit)->GetOwner();
                 }
                 dtm->dataBlock.fEntityID = lastToHit->Id();
                 dtm->dataBlock.fIndex = lastToHit->Type();
                 }
                 FalconSendMessage (dtm, TRUE);
                 */
            }
            else
            {
                // Put the thing back into the campaign unit
                SetNumVehicles(v, GetNumVehicles(v) + 1);
                vehleft++;
                pilotSlot = 0;

                if (fl and vehicle->vehicleInUnit < PILOTS_PER_FLIGHT)
                {
                    pilotSlot = fl->GetAdjustedPlayerSlot(vehicle->pilotSlot);
                    //ShiAssert (pilotSlot == vehicle->vehicleInUnit);
                    fl->plane_stats[pilotSlot] = AIRCRAFT_AVAILABLE;
                    //fl->MakeFlightDirty (DIRTY_PLANE_STATS, DDP[35].priority);
                    fl->MakeFlightDirty(DIRTY_PLANE_STATS, SEND_RELIABLE);

                    if (pilotSlot > lastPilot)
                    {
                        lastPilot = pilotSlot;
                    }
                }

                // Reaggregate our weapons
                SMS = vehicle->GetSMS();

                if (SMS)
                {
                    if (fl)
                    {
                        // Flights will get an all new loadout structure
                        if ( not loadData[pilotSlot])
                            loadData[pilotSlot] = new LoadoutStruct;

                        for (hp = 0; hp < HARDPOINT_MAX; hp++)
                        {
                            if (hp < SMS->NumHardpoints())
                            {
                                // This is as a result of Leon's Rocket hack
                                if (SMS->hardPoint[hp]->weaponId == gRocketId)
                                {
                                    loadData[pilotSlot]->WeaponID[hp] = (short) SMS->hardPoint[hp]->GetRackId();
                                    loadData[pilotSlot]->WeaponCount[hp] = min(1, SMS->hardPoint[hp]->weaponCount);
                                }
                                else
                                {
                                    loadData[pilotSlot]->WeaponID[hp] = (short) SMS->hardPoint[hp]->weaponId;

                                    if (WeaponDataTable[loadData[pilotSlot]->WeaponID[hp]].Flags bitand WEAP_ONETENTH)
                                        //Cobra 12/08/04 This is called when returning from sim
                                        //Causes gun rounds to be removed Removed "/10"
                                        loadData[pilotSlot]->WeaponCount[hp] = (uchar)(SMS->hardPoint[hp]->weaponCount/*/10*/);
                                    else
                                        loadData[pilotSlot]->WeaponCount[hp] = (uchar) SMS->hardPoint[hp]->weaponCount;
                                }

                                // check for ECM pod
                                if (WeaponDataTable[SMS->hardPoint[hp]->weaponId].Flags bitand WEAP_ECM)
                                {
                                    hasECM = TRUE;
                                }
                            }
                            else
                            {
                                loadData[pilotSlot]->WeaponID[hp] = 0;
                                loadData[pilotSlot]->WeaponCount[hp] = 0;
                            }

                            ShiAssert(loadData[pilotSlot]->WeaponID[hp] < NumWeaponTypes);
                        }

                        // Calculate fuel used
                        fuelUsed = fl->CalculateFuelAvailable(pilotSlot) - vehicle->GetTotalFuel();

                        if (fuelUsed > maxFuelUsed)
                            maxFuelUsed = fuelUsed;
                    }
                    else
                    {
                        // Average the weapons for all non-ac vehicles
                        for (hp = 0; hp < SMS->NumHardpoints(); hp++)
                        {
                            num = SMS->hardPoint[hp]->weaponCount;

                            if (WeaponDataTable[SMS->hardPoint[hp]->weaponId].Flags bitand WEAP_ONETENTH)
                                num /= 10;

                            have += num;
                            total += ((VehicleClassDataType*)Falcon4ClassTable[vid].dataPtr)->Weapons[hp];
                        }
                    }
                }
            }

            vehicle = (SimVehicleClass*) myit.GetNext();
        }
    }

    if ( not vehleft)
    {
        KillUnit();
        return vehleft;
    }

    // Do special stuff by unit domain
    if (fl)
    {
        // Copy in new loadout data
        LoadoutStruct *newLoad;
        ShiAssert(lastPilot > -1);
        newLoad = new LoadoutStruct[lastPilot + 1];

        //newLoad = (LoadoutStruct *)MemAllocPtr(LoadoutStruct::pool, sizeof(LoadoutStruct)*(lastPilot+1), FALSE );
        for (v = 0; v < lastPilot + 1; v++)
        {
            if (loadData[v])
            {
                memcpy(&newLoad[v], loadData[v], sizeof(LoadoutStruct));
                delete loadData[v];
                loadData[v] = NULL;
            }

            ShiAssert(newLoad[v].WeaponID[0] < NumWeaponTypes);
        }

        fl->SetLoadout(newLoad, lastPilot + 1);
        fl->UseFuel(maxFuelUsed);
        UpdateSquadronStatus(fl, FALSE, FALSE);

        // Set ECM flag
        SetHasECM(hasECM);
    }
    else
    {
        // Calclulate new supply level
        if (total)
            SetUnitSupply(100 * have / total);
    }

    // Check for carrier death
    if (IsTaskForce() and GetSType() == STYPE_UNIT_CARRIER and not GetNumVehicles(0))
    {
        // Probably be cool to send a special news event here...
        TeamInfo[GetTeam()]->atm->SendATMMessage(Id(), GetTeam(), FalconAirTaskingMessage::atmZapAirbase, 0, 0, NULL, 0);
    }

    return vehleft;
}


int UnitClass::Deaggregate(FalconSessionEntity* session)
{
    if ( not IsLocal() or not IsAggregate() or IsDead())
    {
        return 0;
    }

    SimInitDataClass simdata;
    int v, vehs, motiontype, inslot, rwIndex;
    VehicleID classID;
    VehicleClassDataType *vc;
    float x, y, z;
    VuEntity *newObject;

    memset(&simdata, 0, sizeof(simdata));

    simdata.rwIndex = rwIndex = 0;

    CampBaseClass *base = GetUnitAirbase();

    if (base not_eq NULL and GetCurrentWaypoint() == 1)
    {
        // for airbases, add to runway lists
        ATCBrain *brain = NULL;

        if (base->IsObjective())
        {
            // pt data works only for airbases
            ObjectiveClass *airbase = static_cast<ObjectiveClass*>(base);
            brain = airbase->brain;

            if (brain)
            {
                rwIndex = brain->FindBestTakeoffRunway(TRUE);
                simdata.rwIndex = rwIndex;
                ulong nextTOTime = brain->FindFlightTakeoffTime(
                                       static_cast<FlightClass*>(const_cast<UnitClass*>(this)), GetQueue(rwIndex)
                                   );
                brain->AddTraffic(this->Id(), noATC, rwIndex, nextTOTime);
            }
        }
    }

    //delete spi;
    //spi = new ScopeProfiler("UDEAG2", 6000);

    // Check for possible problems
    if (IsFlight())
    {
        CampEntity ent = NULL;
        simdata.ptIndex = GetDeaggregationPoint(0, &ent);
        ObjectiveClass *airbase = static_cast<ObjectiveClass*>(ent);
        ATCBrain *brain = NULL;

        if (airbase)
            brain = airbase->brain;

        if (brain)
            simdata.rwIndex = airbase->brain->FindBestTakeoffRunway(false);
        else
            simdata.rwIndex = 0;

        if (simdata.ptIndex == DPT_ERROR_NOT_READY)
        {
            return 0;
        }
        else if (simdata.ptIndex == DPT_ERROR_CANT_PLACE)
        {
            // ShiAssert(0);
            // Technically, if we get here the flight should have been canceled, so just do it.
            CancelFlight((Flight)this);
            return 0;
        }
    }

    if ( not session)
    {
        session = FalconLocalSession;
    }

    // Set the owner of the newly created deaggregated entities
    SetDeagOwner(session->Id());
    GetRealPosition(&x, &y, &z);

    SetComponents(new TailInsertList());
    GetComponents()->Register();

    // Fill the reusable simdata class.
    simdata.side = GetOwner();
    simdata.displayPriority = 0;
    simdata.campBase = this;
    simdata.vehicleInUnit = -1;

    if (IsFlight())
    {
        WayPoint w;
        simdata.callsignIdx = ((Flight)this)->callsign_id;
        simdata.callsignNum = ((Flight)this)->callsign_num;
        // Setup waypoint data
        simdata.numWaypoints = 0;
        w = GetFirstUnitWP();

        while (w)
        {
            simdata.numWaypoints++;
            w = w->GetNextWP();
        }

        simdata.currentWaypoint = current_wp - 1;
    }
    else
    {
        simdata.callsignIdx = 0;
        simdata.callsignNum = 0;
        simdata.waypointList = NULL;
        simdata.currentWaypoint = -1;
        simdata.numWaypoints = 0;
    }

    simdata.createType = SimInitDataClass::CampaignVehicle;
    simdata.createFlags = SIDC_SILENT_INSERT;

    if (session not_eq FalconLocalSession)
    {
        simdata.createFlags or_eq SIDC_REMOTE_OWNER;
        simdata.owner = session;
    }

    // VP_changes for tracing DB
    /* FILE* deb = fopen("c:\\traceA10\\dbrain.txt", "a");
     fprintf(deb, "UnitClass Deaggregate nV=%d Id=%d\n", VEHICLE_GROUPS_PER_UNIT, simdata.callsignIdx );
     fclose(deb);
     */
    // Now add all the vehicles
    for (v = 0; v < VEHICLE_GROUPS_PER_UNIT; v++)
    {
        vehs = GetNumVehicles(v);
        classID = GetVehicleID(v);
        inslot = 0;

        while (vehs and classID)
        {
            vc = GetVehicleClassData(classID);

            // Adjust to the correctly textured object (Dogfight only)
            if (FalconLocalGame->GetGameType() == game_Dogfight)
            {
                // sfr: added fallback code to DF here
                // so if we do not find the DF variant, use the common one
                VehicleID dfClassID = SimDogfight.AdjustClassId(GetVehicleID(v), GetTeam());

                if (dfClassID not_eq 0)
                {
                    classID = dfClassID;
                }
            }

            simdata.campSlot = v;
            simdata.inSlot = inslot;
            simdata.status = VIS_NORMAL;
            simdata.descriptionIndex = classID + VU_LAST_ENTITY_TYPE;
            simdata.flags = vc->Flags;

            // Set the position initially to the unit's real position
            simdata.x = x;
            simdata.y = y;
            simdata.z = _isnan(z) ? -14000.0f : z;

            // Now query for any offsets
            motiontype = GetVehicleDeagData(&simdata, FALSE);

            if (motiontype < 0)
            {
                // Technically, if we get here the flight should have been canceled, so just do it.
                CancelFlight((Flight)this);
                return 0;
            }

            // This actually adds the bugger
            newObject = AddObjectToSim(&simdata, motiontype);

            if (newObject)
            {
                GetComponents()->ForcedInsert(newObject);
            }

            vehs--;
            inslot++;
        }
    }

    SetAggregate(0);

    if (TheCampaign.IsOnline())
    {
        SendDeaggregateData(FalconLocalGame);
    }

    // Update our wake status
#if NEW_WAKE

    if (this->OwnerId() == vuLocalSessionEntity->Id())
    {
        Wake();
    }
    else
    {
        Sleep();
    }

#else

    if ((session == FalconLocalSession) or FalconLocalSession->InSessionBubble(this, 1.0F) > 0)
    {
        Wake();
    }
    else
    {
        SetAwake(0);
    }

#endif

    // Special case stuff for battalions
    if (IsBattalion())
    {
        ClearDeaggregationData();
    }

#if USE_VU_COLL_FOR_CAMPAIGN
    deaggregatedEntities->ForcedInsert(this);
#else
    deaggregatedMap->insert(CampBaseBin(this));
#endif
    return 1;
}

int UnitClass::Reaggregate(FalconSessionEntity* session)
{
    if (IsAggregate() or not IsLocal())
        return 0;

    if (session and session->GetPlayerEntity() and (session->GetPlayerFlight() == this))
    {
        // A player is in this unit, so don't reaggregate
        return 0;
    }

    // Record the current state here, and determine what we have remaining
    RecordCurrentState(NULL, TRUE);

    // We need to do some special case stuff:
    if (IsBattalion() and not IsDead())
    {
        ((Battalion)this)->deag_data = new UnitDeaggregationData;
        ((Battalion)this)->deag_data->StoreDeaggregationData(this);
    }

    if (IsAwake())
    {
        Sleep();
    }

    SetAggregate(1);

    // chill the sim vehicles
    if (GetComponents())
    {
        CampEnterCriticalSection();
        {
            VuEntity *vehicle, *next;
            VuListIterator myit(GetComponents());
            vehicle = myit.GetFirst();

            while (vehicle)
            {
                next = myit.GetNext();
                ((SimBaseClass*)vehicle)->SetRemoveFlag();
                vehicle = next;
            }
        }
        GetComponents()->Unregister();
        delete GetComponents();
        SetComponents(NULL);
        CampLeaveCriticalSection();
    }

    if (TheCampaign.IsOnline())
    {
        // Send Reaggregation data to everyone in the group
        FalconSimCampMessage *msg = new FalconSimCampMessage(Id(), FalconLocalGame);
        msg->dataBlock.message = FalconSimCampMessage::simcampReaggregateFromData;
        msg->dataBlock.from = GetDeagOwner();
        msg->dataBlock.size = 0;
        msg->dataBlock.data = NULL;
        FalconSendMessage(msg, TRUE);
    }

#if USE_VU_COLL_FOR_CAMPAIGN
    deaggregatedEntities->Remove(this);
#else
    deaggregatedMap->remove(this->Id());
#endif
    return 1;
}

int UnitClass::TransferOwnership(FalconSessionEntity* session)
{
    if (IsAggregate() or not IsLocal())
    {
        return 0;
    }

    // Record the current state here, and determine what we have remaining and Change ownership locally
    int vehleft = RecordCurrentState(session, FALSE);

    // Set the new owner of the deaggregated entities
    SetDeagOwner(session->Id());

    // Kill off unit or send owenership transfer message
    if (vehleft)
    {
        // Update our local wake status
        if (IsAwake() and not FalconLocalSession->InSessionBubble(this, REAGREGATION_RATIO))
        {
            Sleep();
        }
        else if (
 not IsAwake() and (
                session == FalconLocalSession or FalconLocalSession->InSessionBubble(this, 1.0F) > 0
            )
        )
        {
            Wake();
        }

        // Send the transfer owenership message
        FalconCampDataMessage *msg = new FalconCampDataMessage(Id(), FalconLocalGame);
        msg->dataBlock.type = FalconCampDataMessage::campDeaggregateStatusChangeData;
        msg->dataBlock.size = sizeof(VU_ID) + sizeof(fourbyte);
        msg->dataBlock.data = new uchar[msg->dataBlock.size];
        uchar *dataptr = msg->dataBlock.data;
        VU_ID vuid = session->Id();
        memcpy(dataptr, &vuid, sizeof(VU_ID));
        dataptr += sizeof(VU_ID);
        memcpy(dataptr, &roster, sizeof(fourbyte));
        dataptr += sizeof(fourbyte);
        FalconSendMessage(msg, TRUE);
    }

    return 1;
}

int UnitClass::Wake(void)
{
    // sfr: in MP, we need to run entities even if we are not inside game
#if not NEW_WAKE
    if ( not OTWDriver.IsActive())
    {
        return 0;
    }

#endif

    if (GetComponents())
    {
        SimDriver.WakeCampaignBase(TRUE, this, GetComponents());
        SetAwake(1);
    }
    else
    {
        return 0;
    }

    return 1;
}

int UnitClass::Sleep(void)
{
    SetAwake(0);

    // OTWDriver.LockObject ();
    if (GetComponents())
    {
        SimDriver.SleepCampaignFlight(GetComponents());
    }

    return 1;
}

#define VISUAL_CAMPAIGN_UNIT_MULTIPLIER 3.0F

void UnitClass::InsertInSimLists(float cameraX, float cameraY)
{
    float distsqu = (cameraX - XPos()) * (cameraX - XPos()) + (cameraY - YPos()) * (cameraY - YPos());
    float drawdist = EntityType()->bubbleRange_ * VISUAL_CAMPAIGN_UNIT_MULTIPLIER;

    // This case is for the destructor's sleep call.
    // Basically, we're going away, so don't put us in any lists.
    if (VuState() > VU_MEM_ACTIVE or Inactive())
    {
        return;
    }

#ifdef CAMPAIGN_LABELS

    if (PlayerOptions.NameTagsOn())
    {
        // char tmpStr[40];

        if (draw_pointer == NULL)
        {
            if (distsqu < drawdist * drawdist)
            {
                Tpoint pos;

                // get id of representative vehicle and its class data
                VehicleClassDataType* vc;
                vc = GetVehicleClassData(GetVehicleID(0));
                GetRealPosition(&pos.x, &pos.y, &pos.z);

                if (IsBattalion())
                    draw_pointer = new DrawablePoint(0xff666666, TRUE, &pos, 1.0f);
                else
                    draw_pointer = new DrawablePoint(0xff666666, FALSE, &pos, 1.0f);

                ShiAssert(TeamInfo[GetTeam()]);

                if (TeamInfo[GetTeam()])
                {
                    if (IsFlight())
                        draw_pointer->SetLabel(vc->Name, TeamSimColorList[TeamInfo[GetTeam()]->GetColor()]);
                    else
                    {
                        if (gLangIDNum <= F4LANG_GERMAN or not IsBattalion())
                            draw_pointer->SetLabel(GetUnitClassName(), TeamSimColorList[TeamInfo[GetTeam()]->GetColor()]);
                        else
                        {
                            _TCHAR part1[40], label[80];
                            GetSizeName(GetDomain(), GetType(), part1);
                            _stprintf(label, "%s %s", part1, GetUnitClassName());
                            draw_pointer->SetLabel(label, TeamSimColorList[TeamInfo[GetTeam()]->GetColor()]);
                        }
                    }
                }

                // put it in display
                OTWDriver.InsertObject(draw_pointer);
            }
        }
        else
        {
            // drawpointer not null, check distance and remove drawpointer if too far
            if (distsqu > drawdist * drawdist)
            {
                OTWDriver.RemoveObject(draw_pointer, TRUE);
                draw_pointer = NULL;
            }
        }
    }
    else if (draw_pointer)
    {
        OTWDriver.RemoveObject(draw_pointer, TRUE);
        draw_pointer = NULL;
    }

#endif

    // SetChecked(1);

    ShiAssert(Real());

    if (InSimLists())
        return;

    SetInSimLists(1);
    SimDriver.AddToCampUnitList(this);
}

void UnitClass::RemoveFromSimLists(void)
{
    // remove drawable from drawing
    if (draw_pointer)
    {
        OTWDriver.RemoveObject(draw_pointer, TRUE);
        draw_pointer = NULL;
    }

    if ( not InSimLists())
        return;

    SetInSimLists(0);
    SimDriver.RemoveFromCampUnitList(this);
}

//sfr: changed this proto
void UnitClass::DeaggregateFromData(VU_BYTE* data, long size)
{
    if (IsLocal() or not IsAggregate())
    {
        return;
    }

    int i, classID, inslot, motiontype;
    SimInitDataClass simdata;
    SimBaseClass* newObject;
    FalconSessionEntity* owning_session;
    FalconSessionEntity* creating_session;
    VU_ID vuid;
    ushort value, wps;
    VehicleClassDataType* vc;
    float z;
    WayPoint w, lw = NULL;
    long fuel;
    LoadoutStruct* loadlist = NULL;

    memset(&simdata, 0, sizeof(simdata));

    SetComponents(new TailInsertList());
    GetComponents()->Register();

    DisposeWayPoints();
    long *rem = &size;

    vuid.num_ = VU_SESSION_ENTITY_ID;
    memcpychk(&vuid.creator_, &data, sizeof(VU_SESSION_ID), rem);
    owning_session = (FalconSessionEntity*) vuDatabase->Find(vuid);
    memcpychk(&vuid.creator_, &data, sizeof(VU_SESSION_ID), rem);
    creating_session = (FalconSessionEntity*) vuDatabase->Find(vuid);
    memcpychk(&unit_flags, &data, sizeof(fourbyte), rem);
    memcpychk(&roster, &data, sizeof(fourbyte), rem);
    memcpychk(&z, &data, sizeof(float), rem);
    memcpychk(&value, &data, sizeof(ushort), rem);
    simdata.currentWaypoint = (Int32) value;
    current_wp = (ushort) value + 1;
    memcpychk(&wps, &data, sizeof(ushort), rem);
    simdata.numWaypoints = wps;

    while (wps)
    {
        w = new WayPointClass(&data, rem);

        if ( not wp_list)
        {
            wp_list = w;
        }
        else if (lw)
        {
            lw->InsertWP(w);
        }
        else
        {
            delete w;
        }

        lw = w;
        wps--;
    }

    simdata.waypointList = NULL;
    simdata.vehicleInUnit = -1;

    // sfr: @todo reorganize this, passing this stuff to FlightClass DeaggregateFromData
    if (IsFlight())
    {
        // Copy in flight's loadout data
        memcpychk(&fuel, &data, sizeof(long), rem);
        // This is fuel burnt
        SetBurntFuel(fuel);
        memcpychk(&value, &data, sizeof(uchar), rem);
        ShiAssert(value > 0);
        loadlist = new LoadoutStruct[value];

        //loadlist = (LoadoutStruct *)MemAllocPtr(LoadoutStruct::pool, sizeof(LoadoutStruct)*(value), FALSE );
        for (ushort v = 0; v < value; v++)
        {
            for (i = 0; i < HARDPOINT_MAX; i++)
            {
                memcpychk(&loadlist[v].WeaponID[i], &data, sizeof(short), rem);
                memcpychk(&loadlist[v].WeaponCount[i], &data, sizeof(uchar), rem);
            }
        }

        SetLoadout(loadlist, value);
        simdata.callsignIdx = ((Flight)this)->callsign_id;
        simdata.callsignNum = ((Flight)this)->callsign_num;
    }
    else
    {
        simdata.callsignIdx = 0;
        simdata.callsignNum = 0;
    }

    // Set the owner of the newly created deaggregated entities
    SetDeagOwner(owning_session->Id());

    simdata.forcedId.creator_ = creating_session->Id().creator_;

    SetAggregate(0);

    simdata.displayPriority = 0;
    simdata.side = GetOwner();
    simdata.campBase = this;
    simdata.createType = SimInitDataClass::CampaignVehicle;
    simdata.createFlags = SIDC_SILENT_INSERT bitor SIDC_FORCE_ID;
    simdata.ptIndex = 0;

    if (owning_session not_eq FalconLocalSession)
    {
        simdata.createFlags or_eq SIDC_REMOTE_OWNER;
        simdata.owner = owning_session;
    }

    for (unsigned int vg = 0; vg < VEHICLE_GROUPS_PER_UNIT; vg++)
    {
        int vehs = GetNumVehicles(vg);
        classID = GetVehicleID(vg);
        inslot = 0;

        while (vehs and classID)
        {
            // Adjust to the correctly textured object (Dogfight only)
            if (FalconLocalGame->GetGameType() == game_Dogfight)
            {
                classID = SimDogfight.AdjustClassId(GetVehicleID(vg), GetTeam());
            }

            uchar slot;
            memcpychk(&slot, &data, sizeof(uchar), rem);
            simdata.campSlot = slot;
            simdata.inSlot = inslot;

            if (slot * 3 + inslot >= 48)
            {
                printf("battalion heir will exceed\n");
            }

            // Set the position initially to the unit's real position
            simdata.x = XPos();
            simdata.y = YPos();
            simdata.z = ZPos();

            // Now query for any offsets
            motiontype = GetVehicleDeagData(&simdata, TRUE);

            if (motiontype < 0)
            {
                // Technically, if we get here the flight should have been canceled, so just do it.
                CancelFlight((Flight)this);
                return;
            }

            // Now decode positions and heading
            memcpychk(&simdata.forcedId.num_, &data, sizeof(VU_ID_NUMBER), rem);
            memcpychk(&simdata.x, &data, sizeof(float), rem);
            memcpychk(&simdata.y, bitand data, sizeof(float), rem);
            memcpychk(&simdata.heading, &data, sizeof(float), rem);
            memcpychk(&simdata.ptIndex, &data, sizeof(short), rem);

            vc = GetVehicleClassData(classID);
            simdata.status = VIS_NORMAL;
            simdata.descriptionIndex = classID + VU_LAST_ENTITY_TYPE;
            simdata.flags = vc->Flags;

            // This actually adds the bugger
            newObject = AddObjectToSim(&simdata, motiontype);

            if (newObject)
            {
                GetComponents()->ForcedInsert(newObject);
            }

            vehs--;
            inslot++;
        }
    }

    // Update our local wake status
#if NEW_WAKE

    if (this->OwnerId() == vuLocalSessionEntity->Id())
    {
        Wake();
    }
    else
    {
        Sleep();
    }

#else

    if ((owning_session == FalconLocalSession) or (FalconLocalSession->InSessionBubble(this, 1.0F) > 0))
    {
        Wake();
    }
    else
    {
        SetAwake(0);
    }

#endif

    //adds the unit to deaggregated list
#if USE_VU_COLL_FOR_CAMPAIGN
    deaggregatedEntities->ForcedInsert(this);
#else
    deaggregatedMap->insert(CampBaseBin(this));
#endif
}

//void UnitClass::ReaggregateFromData (int size, uchar* data)
void UnitClass::ReaggregateFromData(VU_BYTE *data, long size)
{
    if (IsLocal() or IsAggregate())
        return;

    if (IsAwake())
        Sleep();

    SetAggregate(1);

    if (GetComponents())
    {
        CampEnterCriticalSection();
        {
            // destroy iterator before the components
            VuListIterator myit(GetComponents());

            for (VuEntity * next, *vehicle = myit.GetFirst(); vehicle not_eq NULL; vehicle = next)
            {
                next = myit.GetNext();
                ((SimBaseClass*)vehicle)->SetRemoveFlag();
            }
        }
        GetComponents()->Unregister();
        delete GetComponents();
        SetComponents(NULL);
        CampLeaveCriticalSection();
    }

#if USE_VU_COLL_FOR_CAMPAIGN
    deaggregatedEntities->Remove(this);
#else
    deaggregatedMap->remove(this->Id());
#endif
}

//void UnitClass::TransferOwnershipFromData (int size, uchar* data)
void UnitClass::TransferOwnershipFromData(VU_BYTE* data, long size)
{
    if (IsAggregate() or IsLocal() or not data)
        return;

#ifndef NDEBUG
    MonoPrint("Transfering ownership of remote Unit #%d\n", GetCampID());
#endif

    VU_ID vuid;

    SetDeagOwner(*(VU_ID*) data);
    data += sizeof(VU_ID);
    // memcpy(&deag_owner, data, sizeof(VU_ID)); data += sizeof(VU_ID);
    memcpy(&roster, data, sizeof(fourbyte));
    data += sizeof(fourbyte);

    // Change ownership locally
    if (GetComponents())
    {
        VuEntity* vehicle;
        VuListIterator myit(GetComponents());
        vehicle = myit.GetFirst();

        while (vehicle)
        {
            ((SimBaseClass*)vehicle)->ChangeOwner(GetDeagOwner());
            vehicle = myit.GetNext();
        }
    }

    // Update our local wake status
    if (IsAwake() and not FalconLocalSession->InSessionBubble(this, REAGREGATION_RATIO))
    {
        Sleep();
    }
    else if (
 not IsAwake() and (
            GetDeagOwner() == FalconLocalSession->Id() or FalconLocalSession->InSessionBubble(this, 1.0F) > 0
        )
    )
    {
        Wake();
    }

    return;
}

int UnitClass::ResetPlayerStatus(void)
{
    return GameManager.CheckPlayerStatus(this);
}

// Returns 0 if nothing happened, -1 if unit moved 'Here', 1 if unit actually moved
int UnitClass::ChangeUnitLocation(CampaignHeading h)
{
    // counts number of time this function is called every second
    //static unsigned int count = 0;
    //static DWORD lastZero;
    //++count;
    //DWORD thisTime = GetTickCount();
    //if (thisTime - lastZero > 1000){
    // MonoPrint("change unit location: %d\n", count);
    // count = 0;
    // lastZero = thisTime;
    //}

    GridIndex x, y, X, Y;
    int nomove = 0;
    CampaignTime t;

    if (h < 0 or h > 8)
    {
        SetUnitLastMove(Camp_GetCurrentTime());
        return 0;
    }

    if ( not IsAggregate())
    {
        return 0;
    }

    if ( not Real())
    {
        return 0;
    }

    t = TimeToMove(this, h);

    if (GetMoveTime() >= t)
    {
        if (t < 1)
        {
            return 0;
        }

        IncrementTime(t);
        GetLocation(&x, &y);
        X = (GridIndex)x + dx[h];
        Y = (GridIndex)y + dy[h];

        if ((X < 0) or (X >= Map_Max_X) or (Y < 0) or (Y >= Map_Max_Y))
        {
            // KCK Hack: Teleport units off map back onto map
            if (X < 0)
            {
                X = 0;
            }

            if (Y < 0)
            {
                Y = 0;
            }

            if (X >= Map_Max_X)
            {
                X = (GridIndex)(Map_Max_X - 1);
            }

            if (Y >= Map_Max_Y)
            {
                Y = (GridIndex)(Map_Max_Y - 1);
            }
        }

        // Be optimistic- actually move the unit (if it's aggregated)
        // Only move it if its going to be on the map however.
        SetLocation(X, Y);
        SetUnitMoved(GetUnitMoved() + 1);

        // Detect things
        if (DetectOnMove() < 0)
        {
            nomove = 1;
        }

        // If we couldn't move, put ourselves back where we started
        if (h == Here or nomove)
        {
            SetLocation(x, y);
            return -1;
        }

        // If we get here, it's because we've moved..
#ifdef CAMPTOOL
        else if (DisplayOk(this) and displayCampaign)
        {
            RedrawCell(NULL, x, y);
            RedrawUnit(this);
        }

#endif

        if (IsBattalion())
        {
            ClearDeaggregationData();
        }

        return 1;
    }

    return 0;
}

// Add or subtract vehicles to a unit
int UnitClass::ChangeVehicles(int a)
{
    int i, v, n, vehs, mv;
    UnitClassDataType* uc;

    uc = GetUnitClassData();

    if ( not uc)
        return 0;

    mv = TeamInfo[GetTeam()]->max_vehicle[GetRClass()];
    vehs = GetTotalVehicles();

    for (i = 0; i < 50 and (vehs > 0 or a > 0) and a; i++)
    {
        v = rand() % mv;
        n = GetNumVehicles(v);

        if (a > 0 and n < uc->NumElements[v] and uc->VehicleType[v])
        {
            vehs++;

            if (losses) losses--;

            SetNumVehicles(v, n + 1);
            a--;
        }
        else if (a < 0 and n > 0 and uc->VehicleType[v])
        {
            vehs--;
            losses++;
            SetNumVehicles(v, n - 1);
            a++;
        }
    }

    ClearDeaggregationData();
    return vehs;
}

int UnitClass::NoMission(void)
{
    Unit e;
    int allplanned = 1;

    e = GetFirstUnitElement();

    while (e)
    {
        if (e->GetCurrentUnitWP() == NULL)
            allplanned = 0;

        e = GetNextUnitElement();
    }

    return not allplanned;
}

int UnitClass::AtDestination(void)
{
    GridIndex x, y, dx, dy;

    GetLocation(&x, &y);
    GetUnitDestination(&dx, &dy);

    if (x == dx and y == dy)
        return 1;

    return 0;
}

int UnitClass::GetUnitFormation() const
{
    int t;
    t = GetUnitTactic();

    if (t)
    {
        return GetTacticFormation(t);
    }

    return 0;
}

// Will flag a unit as dead, but won't free memory.
// check to see if the father is dead too.
void UnitClass::KillUnit(void)
{
    Unit  f;

#ifdef DEBUG

    // Check if we're killing the player flight inappropriately
    if (this == FalconLocalSession->GetPlayerFlight())
    {
        if ((FalconLocalSession->GetPlayerEntity()) and ( not ((SimBaseClass*)FalconLocalSession->GetPlayerEntity())->IsDead()) and ( not ((FalconEntity*)(FalconLocalSession->GetPlayerEntity()))->IsEject()))
        {
            *((unsigned int *) 0x00) = 0;
        }
    }

#endif

    if (IsSetFalcFlag(FEC_REGENERATING))
        return;

    if ( not IsAggregate())
    {
        VuTargetEntity* target = (VuTargetEntity*) vuDatabase->Find(FalconLocalGame->OwnerId());
        FalconSimCampMessage *msg = new FalconSimCampMessage(Id(), target);
        msg->dataBlock.from = FalconLocalSessionId;
        msg->dataBlock.message = FalconSimCampMessage::simcampReaggregate;
        FalconSendMessage(msg);
    }

#ifdef DEBUG
    // if (IsFlight())
    // {
    // ShiAssert(((Flight)this)->pilots[0] == NO_PILOT and ((Flight)this)->pilots[1] == NO_PILOT and ((Flight)this)->pilots[2] == NO_PILOT and ((Flight)this)->pilots[3] == NO_PILOT);
    // }
#endif

    if ( not IsDead())
        SetDead(1);

    // Regroup us, if we're a flight
    if (IsFlight())
        RegroupFlight((FlightClass*)this);

    f = GetUnitParent();

    if (Father())
        DisposeChildren();
    else
    {
        if (f)
            f->RemoveChild(Id());
    }

    if ( not f or f == this)
        return;

    if (f->CountUnitElements() == 0 and not f->IsDead())          // Nothing left in father
        f->KillUnit();

    if (IsSetFalcFlag(FEC_HASPLAYERS))
    {
        if (this == FalconLocalSession->GetPlayerFlight())
            FalconLocalSession->SetPlayerFlight(NULL);

        if (this == FalconLocalSession->GetPlayerSquadron())
            FalconLocalSession->SetPlayerSquadron(NULL);
    }
}

// =========================
// Flag setting stuff
// =========================

void UnitClass::SetDead(int d)
{
    // Start the death timeout clock
    SetLastCheck(TheCampaign.CurrentTime);

    // If we just died, send a CampUI 'destroyed' message to everyone here
    if ( not (unit_flags bitand U_DEAD) and d and (IsBattalion() or IsTaskForce()))
    {
        FalconCampEventMessage *newEvent = new FalconCampEventMessage(Id(), FalconLocalGame);

        newEvent->dataBlock.team = GetEnemyTeam(GetTeam());
        newEvent->dataBlock.eventType = FalconCampEventMessage::unitDestroyed;
        GetLocation(&newEvent->dataBlock.data.xLoc, &newEvent->dataBlock.data.yLoc);
        newEvent->dataBlock.data.owners[0] = GetOwner();

        if (IsBattalion())
        {
            newEvent->dataBlock.data.formatId = 1820;
            newEvent->dataBlock.data.vuIds[0] = Id();
            SendCampUIMessage(newEvent);
        }
        else
        {
            newEvent->dataBlock.data.formatId = 1822;
            newEvent->dataBlock.data.vuIds[0] = Id();
            SendCampUIMessage(newEvent);
        }
    }

    // Set the flags
    if (d)
    {
        if ( not (unit_flags bitand U_DEAD))
        {
            unit_flags or_eq U_DEAD;
            //MakeUnitDirty (DIRTY_UNIT_FLAGS, DDP[36].priority);
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
    else
    {
        if (unit_flags bitand U_DEAD)
        {
            unit_flags and_eq compl U_DEAD;
            //MakeUnitDirty (DIRTY_UNIT_FLAGS, DDP[37].priority);
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
}

void UnitClass::SetAssigned(int p)
{
    if (p)
        unit_flags or_eq U_ASSIGNED;
    else
        unit_flags and_eq compl U_ASSIGNED;
}

void UnitClass::SetOrdered(int p)
{
    if (p)
        unit_flags or_eq U_ORDERED;
    else
        unit_flags and_eq compl U_ORDERED;
}

void UnitClass::SetDontPlan(int p)
{
    if (p)
    {
        if ( not (unit_flags bitand U_NO_PLANNING))
        {
            unit_flags or_eq U_NO_PLANNING;
            //MakeUnitDirty (DIRTY_UNIT_FLAGS, DDP[38].priority);
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
    else
    {
        if (unit_flags bitand U_NO_PLANNING)
        {
            unit_flags and_eq compl U_NO_PLANNING;
            //MakeUnitDirty (DIRTY_UNIT_FLAGS, DDP[39].priority);
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
}

void UnitClass::SetParent(int p)
{
    if (p)
    {
        if ( not (unit_flags bitand U_PARENT))
        {
            unit_flags or_eq U_PARENT;
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
    else
    {
        if (unit_flags bitand U_PARENT)
        {
            unit_flags and_eq compl U_PARENT;
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
}

void UnitClass::SetEngaged(int p)
{
    if (p)
    {
        if ( not (unit_flags bitand U_ENGAGED))
        {
            unit_flags or_eq U_ENGAGED;
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
    else
    {
        if ((unit_flags bitand U_ENGAGED))
        {
            unit_flags and_eq compl U_ENGAGED;
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
}

void UnitClass::SetScripted(int p)
{
    unit_flags or_eq U_SCRIPTED;

    if ( not p)
    {
        unit_flags xor_eq U_SCRIPTED;
    }

    // MakeUnitDirty (DIRTY_UNIT_FLAGS, SEND_SOON);
}

void UnitClass::SetCommando(int c)
{
    unit_flags or_eq U_COMMANDO;

    if ( not c)
    {
        unit_flags xor_eq U_COMMANDO;
    }

    // MakeUnitDirty (DIRTY_UNIT_FLAGS, SEND_SOON);
}

void UnitClass::SetMoving(int p)
{
    if (p)
    {
        if ( not (unit_flags bitand U_MOVING))
        {
            unit_flags or_eq U_MOVING;
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
    else
    {
        if (unit_flags bitand U_MOVING)
        {
            unit_flags and_eq compl U_MOVING;
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
}

void UnitClass::SetRefused(int r)
{
    unit_flags or_eq U_REFUSED;

    if ( not r)
        unit_flags xor_eq U_REFUSED;

    // MakeUnitDirty (DIRTY_UNIT_FLAGS, SEND_SOON);
}

void UnitClass::SetHasECM(int e)
{
    if (e)
    {
        if ( not (unit_flags bitand U_HASECM))
        {
            unit_flags or_eq U_HASECM;
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
    else
    {
        if (unit_flags bitand U_HASECM)
        {
            unit_flags and_eq compl U_HASECM;
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
}

void UnitClass::SetCargo(int c)
{
    if (c)
    {
        if ( not (unit_flags bitand U_CARGO))
        {
            unit_flags or_eq U_CARGO;
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
    else
    {
        if (unit_flags bitand U_CARGO)
        {
            unit_flags xor_eq U_CARGO;
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
}

void UnitClass::SetCombat(int p)
{
    if (p)
    {
        if ( not (unit_flags bitand U_COMBAT))
        {
            unit_flags or_eq U_COMBAT;
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
    else
    {
        if (unit_flags bitand U_COMBAT)
        {
            unit_flags and_eq compl U_COMBAT;
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
}

void UnitClass::SetBroken(int p)
{
    // If we just broke, send a CampUI 'withdrawing/aborted' message to everyone here
    if ( not (unit_flags bitand U_BROKEN) and p and Real() and not IsDead())
    {
        FalconCampEventMessage *newEvent = new FalconCampEventMessage(Id(), FalconLocalGame);

        newEvent->dataBlock.team = GetTeam();
        newEvent->dataBlock.eventType = FalconCampEventMessage::unitWithdrawing;
        GetLocation(&newEvent->dataBlock.data.xLoc, &newEvent->dataBlock.data.yLoc);
        newEvent->dataBlock.data.owners[0] = GetOwner();

        if (GetDomain() == DOMAIN_AIR)
        {
            newEvent->dataBlock.data.formatId = 1812;
            newEvent->dataBlock.data.textIds[0] = -1 * GetVehicleID(0);
        }
        else if (GetDomain() == DOMAIN_SEA)
        {
            newEvent->dataBlock.data.formatId = 1811;
        }
        else
        {
            newEvent->dataBlock.data.vuIds[0] = Id();

            if (GetType() == TYPE_BATTALION)
            {
                if (GetUnitCurrentRole() == GRO_ATTACK)
                {
                    newEvent->dataBlock.data.formatId = 1813;
                }
                else
                {
                    newEvent->dataBlock.data.formatId = 1810;
                }

                if (GetUnitParentID() not_eq FalconNullId)
                {
                    newEvent->dataBlock.data.vuIds[0] = GetUnitParentID();
                }
            }
            else
            {
                if (GetUnitCurrentRole() == GRO_ATTACK)
                    newEvent->dataBlock.data.formatId = 1815;
                else
                    newEvent->dataBlock.data.formatId = 1814;
            }
        }

        SendCampUIMessage(newEvent);
    }

    // Now set the flag
    if (p)
    {
        if ( not (unit_flags bitand U_BROKEN))
        {
            unit_flags or_eq U_BROKEN;
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
    else
    {
        if (unit_flags bitand U_BROKEN)
        {
            unit_flags xor_eq U_BROKEN;
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
}

void UnitClass::SetAborted(int p)
{
    if (p)
    {
        if ( not (unit_flags bitand U_BROKEN))
        {
            unit_flags or_eq U_BROKEN;
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
    else
    {
        if (unit_flags bitand U_BROKEN)
        {
            unit_flags xor_eq U_BROKEN;
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
}

void UnitClass::SetLosses(int p)
{
    unit_flags or_eq U_LOSSES;

    if ( not p)
        unit_flags xor_eq U_LOSSES;

    // MakeUnitDirty (DIRTY_UNIT_FLAGS, SEND_SOON);
}

void UnitClass::SetInactive(int f)
{
    if (f)
    {
        if ( not (unit_flags bitand U_INACTIVE))
        {
            // inactivate: list handling will be done when its detected scanning the list

            /*AllUnitList->Remove(this);
            AllParentList->Remove(this);
            AllRealList->Remove(this);
            RealUnitProxList->Remove(this);

            InactiveList->Insert(this);*/

            unit_flags or_eq U_INACTIVE;
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
    else
    {
        if (unit_flags bitand U_INACTIVE)
        {
            // activate: have to find a place for list handlings here

            ClearDeaggregationData();
            unit_flags and_eq compl U_INACTIVE;
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);

            InactiveList->Remove(this);

            AllUnitList->Insert(this);
            AllParentList->Insert(this);

            if (Real())
            {
                AllRealList->Insert(this);
                RealUnitProxList->Insert(this);
            }
        }
    }
}

void UnitClass::SetFragment(int f)
{
    if (f)
    {
        if ( not (unit_flags bitand U_FRAGMENTED))
        {
            unit_flags or_eq U_FRAGMENTED;
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
    else
    {
        if (unit_flags bitand U_FRAGMENTED)
        {
            unit_flags and_eq compl U_FRAGMENTED;
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
}

void UnitClass::SetTargeted(int p)
{
    unit_flags or_eq U_TARGETED;

    if ( not p)
        unit_flags xor_eq U_TARGETED;

    // MakeUnitDirty (DIRTY_UNIT_FLAGS, SEND_SOON);
}

void UnitClass::SetRetreating(int p)
{
    if (p)
    {
        if ( not (unit_flags bitand U_RETREATING))
        {
            unit_flags or_eq U_RETREATING;
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
    else
    {
        if (unit_flags bitand U_RETREATING)
        {
            unit_flags and_eq compl U_RETREATING;
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
}

void UnitClass::SetDetached(int p)
{
    if (p)
    {
        if ( not (unit_flags bitand U_DETACHED))
        {
            unit_flags or_eq U_DETACHED;
            //MakeUnitDirty (DIRTY_UNIT_FLAGS, DDP[62].priority);
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
    else
    {
        if (unit_flags bitand U_DETACHED)
        {
            unit_flags and_eq compl U_DETACHED;
            //MakeUnitDirty (DIRTY_UNIT_FLAGS, DDP[63].priority);
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
}

void UnitClass::SetSupported(int s)
{
    unit_flags or_eq U_SUPPORTED;

    if ( not s)
        unit_flags xor_eq U_SUPPORTED;

    // MakeUnitDirty (DIRTY_UNIT_FLAGS, SEND_SOON);
}

void UnitClass::SetTempDest(int t)
{
    unit_flags or_eq U_TEMP_DEST;

    if ( not t)
        unit_flags xor_eq U_TEMP_DEST;

    // MakeUnitDirty (DIRTY_UNIT_FLAGS, SEND_SOON);
}

void UnitClass::SetFinal(int p)
{
    if (p and not (unit_flags bitand U_FINAL))
    {
        unit_flags or_eq U_FINAL;
        //MakeUnitDirty (DIRTY_UNIT_FLAGS, DDP[64].priority);
        MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_NOW);
    }
    else if ( not p and (unit_flags bitand U_FINAL))
    {
        unit_flags xor_eq U_FINAL;
        //MakeUnitDirty (DIRTY_UNIT_FLAGS, DDP[65].priority);
        MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_NOW);
    }
}

void UnitClass::SetPilots(int p)
{
    if (p)
    {
        if ( not (unit_flags bitand U_HAS_PILOTS))
        {
            unit_flags or_eq U_HAS_PILOTS;
            //MakeUnitDirty (DIRTY_UNIT_FLAGS, DDP[66].priority);
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
    else
    {
        if (unit_flags bitand U_HAS_PILOTS)
        {
            unit_flags and_eq compl U_HAS_PILOTS;
            //MakeUnitDirty (DIRTY_UNIT_FLAGS, DDP[67].priority);
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
}

void UnitClass::SetDiverted(int d)
{
    if (d)
    {
        if ( not (unit_flags bitand U_DIVERTED))
        {
            unit_flags or_eq U_DIVERTED;
            //MakeUnitDirty (DIRTY_UNIT_FLAGS, DDP[68].priority);
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
    else
    {
        if (unit_flags bitand U_DIVERTED)
        {
            unit_flags and_eq compl U_DIVERTED;
            //MakeUnitDirty (DIRTY_UNIT_FLAGS, DDP[69].priority);
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
}

void UnitClass::SetFired(int d)
{
    if (d)
    {
        if ( not (unit_flags bitand U_FIRED))
        {
            unit_flags or_eq U_FIRED;
            //MakeUnitDirty (DIRTY_UNIT_FLAGS, DDP[70].priority);
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
    else
    {
        if (unit_flags bitand U_FIRED)
        {
            unit_flags and_eq compl U_FIRED;
            //MakeUnitDirty (DIRTY_UNIT_FLAGS, DDP[71].priority);
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
}

void UnitClass::SetLocked(int l)
{
    if (l)
    {
        if ( not (unit_flags bitand U_LOCKED))
        {
            unit_flags or_eq U_LOCKED;
            //MakeUnitDirty (DIRTY_UNIT_FLAGS, DDP[72].priority);
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
    else
    {
        if (unit_flags bitand U_LOCKED)
        {
            unit_flags and_eq compl U_LOCKED;
            //MakeUnitDirty (DIRTY_UNIT_FLAGS, DDP[73].priority);
            MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
        }
    }
}

void UnitClass::SetIAKill(int i)
{
    unit_flags or_eq U_IA_KILL;

    if ( not i)
        unit_flags xor_eq U_IA_KILL;

    // MakeUnitDirty (DIRTY_UNIT_FLAGS, SEND_SOON);
}

void UnitClass::SetNoAbort(int i)
{
    unit_flags or_eq U_NO_ABORT;

    if ( not i)
    {
        unit_flags xor_eq U_NO_ABORT;
    }

    //MakeUnitDirty (DIRTY_UNIT_FLAGS, DDP[74].priority);
    MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_SOON);
}

// =====================
// Other Setters
// =====================

int UnitClass::SetUnitSType(char T)
{
    int tid, sp = 1;

    tid = GetClassID(GetDomain(), CLASS_UNIT, GetType(), T, GetSPType(), 0, 0, 0);

    while ( not tid and sp < 10)
        tid = GetClassID(GetDomain(), CLASS_UNIT, GetType(), T, sp++, 0, 0, 0);

    SetEntityType(tid + VU_LAST_ENTITY_TYPE);
    class_data = (UnitClassDataType*) Falcon4ClassTable[tid].dataPtr;
    BuildElements();
    return tid;
}

int UnitClass::SetUnitSPType(char T)
{
    int tid;

    tid = GetClassID(GetDomain(), CLASS_UNIT, GetType(), GetSType(), T, 0, 0, 0);

    if ( not tid)
        return 0;

    SetEntityType(tid + VU_LAST_ENTITY_TYPE);
    class_data = (UnitClassDataType*) Falcon4ClassTable[tid].dataPtr;
    BuildElements();
    return tid;
}

//char* UnitClass::GetName (_TCHAR* buffer, int size, int object)
char* UnitClass::GetName(_TCHAR* buffer, int size, int)
{
    int name_id;
    _TCHAR temp1[10], temp2[30], temp3[30], format[10];

    name_id = GetUnitNameID();
    GetNumberName(name_id, temp1);
    _tcscpy(format, "%s %s %s");
    _tcsnccpy(temp2, class_data->Name, 29);
    temp2[29] = 0;

    if (gLangIDNum == F4LANG_GERMAN)
    {
        // Replace space with hyphen, if necessary
        if (temp2[_tcslen(temp2) - 1] == '-')
            _tcscpy(format, "%s %s%s");
    }
    else if (gLangIDNum == F4LANG_FRENCH)
    {
        if (name_id == 1 and IsBattalion())
        {
            _TCHAR junk[30];

            ReadIndexedString(19, junk, 5);
            _tcscpy(temp1, "1");
            _tcscat(temp1, junk);
        }
    }

    GetSizeName(GetDomain(), GetType(), temp3);

    ShiAssert(((int)(_tcslen(temp1) + _tcslen(temp2) + _tcslen(temp3) + 3)) < size);

    if (gLangIDNum >= F4LANG_SPANISH)
        _sntprintf(buffer, size, format, temp1, temp3, temp2);
    else
        _sntprintf(buffer, size, format, temp1, temp2, temp3);

    return buffer;
}

char* UnitClass::GetFullName(_TCHAR* buffer, int size, int object)
{
    _TCHAR temp1[80], temp2[80];

    if (GetDomain() == DOMAIN_LAND)
    {
        if (Parent() and GetUnitDivision() == 0)
        {
            ReadIndexedString(167, temp2, 63);
            _sntprintf(buffer, size, "%s %s", GetName(temp1, size, object), temp2);
        }
        else if (Parent())
        {
            Division d = GetDivisionByUnit(this);

            if (d)
                _sntprintf(buffer, size, gUnitNameFormat, GetName(temp1, size, object), d->GetName(temp2, 63, object));
            else
                _sntprintf(buffer, size, gUnitNameFormat, GetName(temp1, size, object), ::GetDivisionName(GetUnitDivision(), temp2, 79, object));
        }
        else
        {
            Unit p;
            p = GetUnitParent();

            if ( not p)
            {
                ReadIndexedString(167, temp2, 63);
                _sntprintf(buffer, size, "%s %s", GetName(temp1, size, object), temp2);
            }
            else
                _sntprintf(buffer, size, gUnitNameFormat, GetName(temp1, size, object), p->GetFullName(temp2, size, object));
        }
    }
    else
        buffer = GetName(buffer, size, object);

    // _sntprintf should do this for us, but for some reason it sometimes doesn't
    buffer[size - 1] = 0;

    return buffer;
}

char* UnitClass::GetDivisionName(_TCHAR* buffer, int size, int object)
{
    if (GetDomain() == DOMAIN_LAND)
    {
        if (Parent() and GetUnitDivision() == 0)
            GetName(buffer, size, object);
        else if (GetUnitDivision() == 0)
        {
            Unit p = GetUnitParent();

            if (p)
                p->GetDivisionName(buffer, size, object);
        }
        else
        {
            Division d = GetDivisionByUnit(this);

            if (d)
                d->GetName(buffer, size, object);
            else
                ::GetDivisionName(GetUnitDivision(), buffer, size, object);
        }
    }
    else
        buffer = GetName(buffer, size, object);

    return buffer;
}

// Returns the exact hit chance for this unit.
int UnitClass::GetHitChance(int mt, int range)
{
    Unit T;
    int bhc = 0, hc = 0, i, mv;

    if (Father())
    {
        T = GetFirstUnitElement();

        while (T not_eq NULL)
        {
            hc += (100 - hc) * T->GetHitChance(mt, range) / 100;
            T = GetNextUnitElement();
        }

        return hc;
    }

    mv = TeamInfo[GetTeam()]->max_vehicle[GetRClass()];

    for (i = 0; i < mv; i++)
        hc += (100 - hc) * GetVehicleHitChance(i, (MoveType)mt, range, 0) / 100;

    return bhc;
}

// Returns maximum possible hit chance for this unit
int UnitClass::GetAproxHitChance(int mt, int range)
{
    UnitClassDataType* uc;

    uc = GetUnitClassData();

    if ( not uc)
        return 0;

    if (range < uc->Range[mt])
        return FloatToInt32(uc->HitChance[mt] * (1.25F - ((float)range / (uc->Range[mt] + 1))));

    return 0;
}

// Combat strength is Hit Chance * weapon Strength
int UnitClass::GetCombatStrength(int mt, int range)
{
    Unit T;
    int str = 0, i;

    if (Father())
    {
        T = GetFirstUnitElement();

        while (T not_eq NULL)
        {
            str += T->GetCombatStrength(mt, range);
            T = GetNextUnitElement();
        }

        return str;
    }

    for (i = 0; i < VEHICLE_GROUPS_PER_UNIT; i++)
        str += GetNumVehicles(i) * GetVehicleCombatStrength(i, (MoveType)mt, range);

    if (GetRClass() == RCLASS_AIR)
        str = FloatToInt32(str * AirExperienceAdjustment(GetOwner()));
    else if (GetRClass() == RCLASS_NAVAL)
        str = FloatToInt32(str * NavalExperienceAdjustment(GetOwner()));
    else if (GetRClass() == RCLASS_AIRDEFENSE)
        str = FloatToInt32(str * AirDefenseExperienceAdjustment(GetOwner()));
    else
        str = FloatToInt32(str * GroundExperienceAdjustment(GetOwner()));

    return str;
}

// Returns maximum possible damage at this range
int UnitClass::GetAproxCombatStrength(int mt, int range)
{
    Unit T;
    int str = 0;
    UnitClassDataType* uc;

    if (Father())
    {
        T = GetFirstUnitElement();

        while (T not_eq NULL)
        {
            str += T->GetAproxCombatStrength(mt, range);
            T = GetNextUnitElement();
        }

        return str;
    }

    uc = GetUnitClassData();

    if (uc and Real() and uc->Range[mt] >= range)
        return uc->Strength[mt];

    return 0;
}

// Returns maximum range to hit movement type (only really valid on real units)
int UnitClass::GetWeaponRange(int mt, FalconEntity *target)  // 2002-03-08 MODIFIED BY S.G. Need to pass it a target sometime so default to NULL for most cases
{
    int i, rng, mr;

    mr = 0;

    for (i = 0; i < VEHICLE_GROUPS_PER_UNIT; i++)
    {
        rng = GetVehicleRange(i, mt, target); // 2002-03-08 MODIFIED BY S.G. Added target which is passed to GetVehicleRamge

        if (rng > mr)
            mr = rng;
    }

    return mr;
}

// precalculated aproximation
int UnitClass::GetAproxWeaponRange(int mt)
{
    return class_data->Range[mt];
}

int UnitClass::GetDetectionRange(int mt)
{
    int dr;
    UnitClassDataType* uc;

    if ( not Real())
        return 0;

    uc = GetUnitClassData();
    ShiAssert(uc);
    // 2001-04-21 MODIFIED BY S.G. ABOVE 250 HAS A NEW MEANING SO USE THE UNIT ELECTRONIC DETECTION RANGE INSTEAD...
    // dr = uc->Detection[mt];
    dr = GetElectronicDetectionRange(mt);

    if (dr < VisualDetectionRange[mt])
        dr = GetVisualDetectionRange(mt);

    return dr;
}

int UnitClass::GetElectronicDetectionRange(int mt)
{
    // 2002-02-25 MODIFIED BY S.G. ABOVE 250 HAS A NEW MEANING SO USE THE UNIT ELECTRONIC DETECTION RANGE INSTEAD... WAS IN RP5, FORGOT TO ADD IT TO F4UT
    // return class_data->Detection[mt];
    if (class_data->Detection[mt] > 250)
    {
        return 250 + (class_data->Detection[mt] - 250) * 50;
    }

    return class_data->Detection[mt];
}

#if 0 // eFalcon 1.10
int UnitClass::CanDetect(FalconEntity* ent)
{
    float ds, mrs;
    float dr;
    MoveType mt;
    int detectiontype = 0;

    if (IsDead() or ent->IsDead() or (IsFlight() and not Moving()) or (ent->IsFlight() and not ((FlightClass*)ent)->Moving()))
        return 0;

    mt = ent->GetMovementType();
    ds = DistSqu(XPos(), YPos(), ent->XPos(), ent->YPos());

    float targetaangle;
    float targeteangle;
    float dx = ent->XPos() - XPos();
    float dy = ent->YPos() - YPos();
    float dz = ent->ZPos() - ZPos();

    // First see if our eyes will do the job
    float vdr = KM_TO_FT * (float) GetVisualDetectionRange(mt);
    vdr *= vdr;

    if (ds < vdr)
    {
        if (IsFlight() and ent->IsFlight())
        {
            targetaangle = fabs((float) atan2(dy, dx) - Yaw());

            if (targetaangle > PI)
                targetaangle = 2 * PI - targetaangle;

            if (targetaangle > 120.0 * DTR)
                return 0;

            // Don't use height limits when the enemy has been spotted by other sources
            if (ent->GetTeam() not_eq GetTeam() and ((FlightClass*) ent)->GetSpotted(GetTeam()))
                return DETECTED_VISUAL;

            targeteangle = (float) atan(-dz / (float) sqrt(dx * dx + dy * dy + 0.1F));

            if (targeteangle > 90.0 * DTR or targeteangle < -30.0 * DTR)
                return 0;
        }

        return DETECTED_VISUAL;
    }

    dr = GetDetectionRange(mt);

    if (dr > 250)
        dr = 250 + (dr - 250) * 50;

    dr = (float)(dr * KM_TO_FT);

    RadarDataType* radarData = &RadarDataTable[GetRadarType()];

    if (radarData and radarData->NominalRange < dr)
        dr = radarData->NominalRange;

    mrs = dr * dr;

    if (sojSource)
    {
        int entOctant = OctantTo(0.0F, 0.0F, ent->XPos() - XPos(), ent->YPos() - YPos());

        if ((entOctant == sojOctant or
             entOctant == (sojOctant + 1) % 8 or
             entOctant == (sojOctant - 1) % 8) and 
            sojRangeSq < mrs * 2.25)
        {
            mrs = mrs * sojRangeSq / (mrs * 2.25);
        }
    }

    if (ds > mrs)
        return 0;

    // Additional Detection requirements against aircraft
    if (ent->IsFlight())
    {
        // If we don't have a radar and the sucker isn't spotted yet, just bail out.
        if ( not IsEmitting() and not ((Flight)ent)->GetSpotted(GetTeam()))
            return 0;

        // If we're a batallion, try to use radar coverage masks
        if (IsBattalion())
        {
            GridIndex x, y;
            Objective o = NULL;

            if (Moving())
                return 0;
            else
            {
                // Try to use nearby objective's radar mask data
                GetLocation(&x, &y);
                o = FindNearestObjective(x, y, NULL, 1);

                if (o and o->HasRadarRanges())
                {
                    mrs = o->GetSiteRange(ent);
                    mrs *= mrs;

                    if (ds > mrs)
                        return 0;
                }
            }
        }


        // If we're an aircraft, we can't see below MEA very well with radar
        if (IsFlight())
        {
            float tl = TheMap.GetMEA(ent->XPos(), ent->YPos());

            if ((-1 * ent->ZPos() < tl) and (ds * 16.0f > mrs))
                return 0;
        }

        // Stealth aircraft act as if they're flying at double their range
        UnitClassDataType *uc = ((Flight)ent)->GetUnitClassData();

        if (uc->Flags bitand VEH_STEALTH and ds * 4.0F > mrs)
            return 0;

        // Don't use limits for radar when the enemy has been spotted by other sources
        if (ent->GetTeam() not_eq GetTeam() and ((FlightClass*) ent)->GetSpotted(GetTeam()))
            return DETECTED_RADAR;

        if (IsFlight())
        {
            targetaangle = fabs((float) atan2(dy, dx) - Yaw());

            if (targetaangle > PI)
                targetaangle = 2 * PI - targetaangle;

            if (targetaangle > 2 * radarData->ScanHalfAngle)
                return 0;

            targeteangle = (float) atan(-dz / (float) sqrt(dx * dx + dy * dy + 0.1F));

            if (fabs(targeteangle) > 60.0 * DTR)
                return 0;
        }

        // See if enemy flight is beaming us, and go vis only if so
        float yd = (float)fabs(ent->Yaw() - Yaw());

        if (yd > 180 * DTR) yd -= PI;

        if (yd > 60 * DTR and yd < 120 * DTR)
            return 0;
    }

    return DETECTED_RADAR;
}
#else // RP5
int UnitClass::CanDetect(FalconEntity* ent)
{
    float ds, mrs;
    MoveType mt;

    mt = ent->GetMovementType();
    ds = DistSqu(XPos(), YPos(), ent->XPos(), ent->YPos());
    mrs = (float)(GetDetectionRange(mt) * KM_TO_FT);
    mrs *= mrs;

    if (ds > mrs)
        return 0;

#ifdef DEBUG

    if (ent->IsAirplane())
        int i = 0;

    if (IsAirplane())
        int i = 0;

#endif

    // Additional Detection requirements against aircraft
    if (ent->IsFlight())
    {
        /* 2001-04-05 REMOVED BY S.G. TOO MANY CHANGES TO KEEP TRACK...
        // First see if our eyes will do the job
        float vdr = (float) GetVisualDetectionRange(mt);
        vdr *= vdr;
        if (ds < vdr)
        return DETECTED_VISUAL;

        // If we don't have a radar and the sucker isn't spotted yet, just bail out.
        if ( not IsEmitting() and not ((Flight)ent)->GetSpotted(GetTeam()))
        return 0;

        // If we're a batallion, try to use radar coverage masks
        if (IsBattalion())
        {
        GridIndex x,y;
        Objective o = NULL;
        if (Moving())
        return 0;
        else
        {
        // Try to use nearby objective's radar mask data
        GetLocation(&x,&y);
        o = FindNearestObjective (x, y, NULL, 1);
        if (o and o->HasRadarRanges())
        {
        mrs = o->GetSiteRange(ent);
        mrs *= mrs;
        if (ds > mrs)
        return 0;
        }
        }
        }
         */
        // 2001-03-15 ADDED BY S.G. SEE COMMENT WITHIN. BASICALLY, I NEED TO DO MORE TEST IF BOTH ARE FLIGHTS
        // 2001-03-22 REMOVED BY S.G. DON'T CHECK IF SPOTTED. THIS IS DONE IN DetectVs NOW. THAT WAY, SPOTTING CAN BE LOST
        if (IsFlight())
        {
            // Using the graphic's altitude map, adjust the altitude
            float ez = ent->ZPos() + TheMap.GetMEA(ent->XPos(), ent->YPos());
            float z =  ZPos() + TheMap.GetMEA(XPos(), YPos());

            // First, if the enemy is hugging the ground, we cannot see it (don't forget that's only if the enemy is't spotted)
            if (ez > -500.0f and z < -2000.0f)
                return 0;

            // Since both are flights, now deal with spherical coverage instead of cylindrical
            float az = TargetAz((FalconEntity *)this, (FalconEntity *)ent);
            float el = TargetEl((FalconEntity *)this, (FalconEntity *)ent);

            VehicleClassDataType* vc = GetVehicleClassData(class_data->VehicleType[0]); // That's ok, we are a flight and flight are made of the same airplane, so same radar for all of them
            float azLimit = RadarDataTable[vc->RadarType].ScanHalfAngle;

            // If we are not emitting, or if the enemy is outside our radar cone (generic, 60w 60h) and the enemy isn't spotted yet, limit to visual detection only...
            if ( not IsEmitting() or (fabs(az) > azLimit or fabs(el) > 60.0f * DTR))
            {
                // We have to go visual. We'll use a generic -30, +90 elevation and -175 to +175 azimuth coverage. If outside, not seen.
                if (el < -30.0f * DTR or el > 90.0f * DTR or fabs(az) > 175.0f * DTR)
                    return 0;

                // Now that we know the enemy is in our field of view (but not in our RADAR FOV) see if it's close enough to see with our eyes
                float vdr = (float) GetVisualDetectionRange(mt) * KM_TO_FT;
                vdr *= vdr;

                if (ds > vdr)
                    return 0;
            }

            // 2001-05-01 ADDED BY S.G. AWACS ARE AFFECTED BY SOJ AS MUCH AS GROUND RADAR...
            // If we get here, our flight can see the other flight. Now check if we are an AWAC and if so, are jammed by SOJ?
            if (GetSType() == STYPE_UNIT_AWACS)
            {
                Flight ecmFlight = ((FlightClass *)ent)->GetECMFlight();

                if (((FlightClass *)ent)->HasAreaJamming())
                    ecmFlight = (FlightClass *)ent;
                else if (ecmFlight)
                {
                    if ( not ecmFlight->IsAreaJamming())
                        ecmFlight = NULL;
                }

                if (ecmFlight)
                {
                    // Now, here's what we need to do:
                    // 1. For now jamming power has 360 degrees coverage
                    // 2. The radar range will be reduced by the ratio of its normal range and the jammer's range to the radar to the power of two
                    // 3. The jammer is dropping the radar gain, effectively dropping its detection distance
                    // 4. If the flight is outside this new range, it's not detected.

                    // Get the range of the SOJ to the radar
                    float jammerRange = DistSqu(ecmFlight->XPos(), ecmFlight->YPos(), XPos(), YPos());

                    // If the SOJ is within the radar normal range, 'adjust' it. If this is now less that ds (our range to the radar), return 0.
                    // SOJ can jamm even if outside the detection range of the radar
                    if (jammerRange < mrs * 2.25f)
                    {
                        jammerRange = jammerRange / (mrs * 2.25f); // No need to check for zero because jammerRange has to be LESS than mrs to go in
                        mrs *= jammerRange * jammerRange;

                        if (ds > mrs)
                            // We are being jammed. AWAC don't see well outside their plane so don't try visual detection...
                            return 0;
                    }
                }
            }

        }
        else
        {
            // Now see if our eyes will do the job (original code was also missing * KM_TO_FT)
            float vdr = (float) GetVisualDetectionRange(mt) * KM_TO_FT;
            vdr *= vdr;

            if (ds < vdr)
                return DETECTED_VISUAL;

            // We're not a flight so do battalion tests...

            // First test is are we emitting (then we can use radar coverage)
            if (IsEmitting())
            {
                // Here's the deal, we're not a flight but we are unit so we must be a battalion, but just check to be sure...
                // If we're a batallion, try to use radar coverage masks
                if (IsBattalion())
                {
                    GridIndex x, y;
                    Objective o = NULL;

                    if (Moving())
                        return 0;
                    else
                    {
                        // Try to use nearby objective's radar mask data
                        GetLocation(&x, &y);
                        o = FindNearestObjective(x, y, NULL, 1);

                        if (o and o->HasRadarRanges())
                        {
                            mrs = o->GetSiteRange(ent);
                            mrs *= mrs;

                            if (ds > mrs)
                                return 0;
                        }
                    }

                    // Get our attached stand off jammer flight, if any
                    // Make sure this SOJ is on station. If ent IS the SOJ, then it emits along the flightpaths as well
                    // This allows the SOJ flight to be protected as it reaches its station waypoints.

                    Flight ecmFlight = ((FlightClass *)ent)->GetECMFlight();

                    if (((FlightClass *)ent)->HasAreaJamming())
                        ecmFlight = (FlightClass *)ent;
                    else if (ecmFlight)
                    {
                        if ( not ecmFlight->IsAreaJamming())
                            ecmFlight = NULL;
                    }

                    if (ecmFlight)
                    {
                        // Now, here's what we need to do:
                        // 1. For now jamming power has 360 degrees coverage
                        // 2. The radar range will be reduced by the ratio of its normal range and the jammer's range to the radar to the power of two
                        // 3. The jammer is dropping the radar gain, effectively dropping its detection distance
                        // 4. If the flight is outside this new range, it's not detected.

                        // Get the range of the SOJ to the radar
                        float jammerRange = DistSqu(ecmFlight->XPos(), ecmFlight->YPos(), XPos(), YPos());

                        // If the SOJ is within the radar normal range, 'adjust' it. If this is now less that ds (our range to the radar), return 0.
                        // SOJ can jamm even if outside the detection range of the radar
                        if (jammerRange < mrs * 2.25f)
                        {
                            jammerRange = jammerRange / (mrs * 2.25f); // No need to check for zero because jammerRange has to be LESS than mrs to go in
                            mrs *= jammerRange * jammerRange;

                            if (ds > mrs)
                                return 0;
                        }
                    }
                }
            }
        }

        // See if enemy flight is beaming us, and go vis only if so
        float yd = (float)fabs(ent->Yaw() - Yaw());

        if (yd > 180 * DTR) yd -= PI;

        // 2001-04-02 MODIFIED BY S.G. BEAMING ANGLE TOO WIDE
        // if (yd > 60*DTR and yd < 120*DTR)
        if (yd > 80 * DTR and yd < 100 * DTR)
            return 0;

        // Stealth aircraft act as if they're flying at double their range
        UnitClassDataType *uc = ((Flight)ent)->GetUnitClassData();

        // 2001-04-29 MODIFIED BY S.G. IF IT'S A STEALTH AND IT GOT HERE, IT WASN'T DETECTED VISUALLY SO ABORT RIGHT NOW
        // if (uc->Flags bitand VEH_STEALTH and ds*4.0F > mrs)
        if (uc->Flags bitand VEH_STEALTH)
            return 0;

        // If we're an aircraft, we can't see below MEA very well with radar
        if (IsFlight())
        {
            float tl = TheMap.GetMEA(ent->XPos(), ent->YPos());

            if ((-1 * ent->ZPos() < tl) and (ds * 16.0f > mrs))
                return 0;
        }
    }

    return DETECTED_RADAR;
}
#endif

int UnitClass::GetNumberOfArcs(void)
{
    if ( not IsBattalion())
        return 0;

    // Only stationary air defense units even have any radar capibility
    //if ( not Moving() and GetUnitNormalRole() == GRO_AIRDEFENSE)
    if (( not Moving() or g_bFireOntheMove) and GetUnitNormalRole() == GRO_AIRDEFENSE)
    {
        Objective o = NULL;
        GridIndex x, y;
        GetLocation(&x, &y);
        o = FindNearestObjective(x, y, NULL, 1);

        if (o)
            return o->GetNumberOfArcs();
    }

    return 0;
}

float UnitClass::GetArcRatio(int anum)
{
    if ( not IsBattalion())
        return 0.0F;

    // Only stationary air defense units even have any radar capibility
    //if ( not Moving() and GetUnitNormalRole() == GRO_AIRDEFENSE)
    if (( not Moving() or g_bFireOntheMove) and GetUnitNormalRole() == GRO_AIRDEFENSE)
    {
        Objective o = NULL;
        GridIndex x, y;
        GetLocation(&x, &y);
        o = FindNearestObjective(x, y, NULL, 1);

        if (o)
            return o->GetArcRatio(anum);
    }

    return 0.0F;
}

float UnitClass::GetArcRange(int anum)
{
    if ( not IsBattalion())
        return 0.0F;

    // Only stationary air defense units even have any radar capibility
    //if ( not Moving() and GetUnitNormalRole() == GRO_AIRDEFENSE)
    if (( not Moving() or g_bFireOntheMove) and GetUnitNormalRole() == GRO_AIRDEFENSE)
    {
        Objective o = NULL;
        GridIndex x, y;
        GetLocation(&x, &y);
        o = FindNearestObjective(x, y, NULL, 1);

        if (o)
            return o->GetArcRange(anum);
    }

    return 0.0F;
}

void UnitClass::GetArcAngle(int anum, float* a1, float *a2)
{
    if ( not IsBattalion())
    {
        *a1 = 0.0F;
        *a2 = 2.0F * PI;
        return;
    }

    // Only stationary air defense units even have any radar capibility
    //if ( not Moving() and GetUnitNormalRole() == GRO_AIRDEFENSE)
    if (( not Moving() or g_bFireOntheMove) and GetUnitNormalRole() == GRO_AIRDEFENSE)
    {
        Objective o = NULL;
        GridIndex x, y;
        GetLocation(&x, &y);
        o = FindNearestObjective(x, y, NULL, 1);

        if (o)
            o->GetArcAngle(anum, a1, a2);
    }
}

float UnitClass::GetRCSFactor(void)
{
    UnitClassDataType* unitData = GetUnitClassData();
    return ((VehicleClassDataType *)Falcon4ClassTable[unitData->VehicleType[0]].dataPtr)->RCSfactor;
}

float UnitClass::GetIRFactor(void)
{
    // TODO:  THIS NEED TO RETURN IR DATA
    UnitClassDataType* unitData = GetUnitClassData();

    ShiAssert(unitData->VehicleType[0]);
    // 2000-11-17 MODIFIED BY S.G. SINCE WE HAVE AN IR DATA NOW, WHY NOT USE IT :-)
    //   return ((VehicleClassDataType *)Falcon4ClassTable[unitData->VehicleType[0]].dataPtr)->RCSfactor;

    // The IR signature is stored as a byte at offset 0x9E of the falcon4.vcd structure.
    // This byte, as well as 0x9D and 0x9F are used for padding originally.
    // The value range will be 0 to 2 with increments of 0.0078125
    unsigned char *pIrSign = (unsigned char *)((VehicleClassDataType *)Falcon4ClassTable[unitData->VehicleType[0]].dataPtr);

    if (pIrSign)
    {
        int iIrSign = (unsigned)pIrSign[0x9E];

        if (iIrSign)
            return (float)iIrSign / 128.0f;
    }

    return ((VehicleClassDataType *)Falcon4ClassTable[unitData->VehicleType[0]].dataPtr)->RCSfactor;
}

int UnitClass::GetRadarType(void)
{
    UnitClassDataType* unitData = GetUnitClassData();

    if (unitData and unitData->RadarVehicle < 255) // Naval unit CTD
    {
        return ((VehicleClassDataType *)Falcon4ClassTable[unitData->VehicleType[unitData->RadarVehicle]].dataPtr)->RadarType;
    }
    else
    {
        return RDR_NO_RADAR;
    }
}

void UnitClass::GetComponentLocation(GridIndex* x, GridIndex* y, int component)
{
    ::vector v;

    v.z = 0.0F;

    if (component < 255 and GetComponents())
    {
        SimVehicleClass *vehicle = (SimVehicleClass*) GetComponentNumber(component);

        if (vehicle)
        {
            v.x = vehicle->XPos();
            v.y = vehicle->YPos();
            ConvertSimToGrid(&v, x, y);
            return;
        }
    }

    v.x = XPos();
    v.y = YPos();
    ConvertSimToGrid(&v, x, y);
}

int UnitClass::GetComponentAltitude(int component)
{
    if (component < 255)
    {
        SimVehicleClass *vehicle = (SimVehicleClass*) GetComponentNumber(component);

        if (vehicle)
            return FloatToInt32(vehicle->ZPos() * -1.0F);
    }

    return FloatToInt32(ZPos() * -1.0F);
}

// This will return the slowest cruise speed of all units in a brigade/package/etc
int UnitClass::GetFormationCruiseSpeed() const
{
    int      speed = 9999;
    Unit     e;

    if ( not Father() or not GetFirstUnitElement())
        return GetCruiseSpeed();

    e = GetFirstUnitElement();

    while (e)
    {
        if (speed > e->GetCruiseSpeed())
            speed = e->GetCruiseSpeed();

        e = GetNextUnitElement();
    }

    return speed;
}

// This is our cruise speed (friendly area movement speed)
int UnitClass::GetCruiseSpeed() const
{
    //TJL 11/22/03 Remove 0.65F
    //return FloatToInt32(0.65F * class_data->MovementSpeed);
    return FloatToInt32(class_data->MovementSpeed);
}

// This is our combat speed (enemy area movement speed)
int UnitClass::GetCombatSpeed() const
{
    //TJL 11/22/03 Remove 0.85F
    //return FloatToInt32(0.85F * class_data->MovementSpeed);
    return FloatToInt32(class_data->MovementSpeed);
}

// Max speed (from entity table)
int UnitClass::GetMaxSpeed() const
{
    return class_data->MovementSpeed;
}

int UnitClass::GetUnitEndurance(void)
{
    int end;
    UnitClassDataType* uc;

    uc = GetUnitClassData();

    if ( not uc)
        return 0;

    end = (uc->Fuel / (uc->Rate + 1)) / 100;
    return end;
}

int UnitClass::GetUnitRange(void)
{
    UnitClassDataType* uc;

    uc = GetUnitClassData();

    if ( not uc)
        return 0;

    return uc->MaxRange;
}

int UnitClass::GetRClass(void)
{
    if (GetDomain() == DOMAIN_AIR)
        return RCLASS_AIR;
    else if (GetDomain() == DOMAIN_SEA)
        return RCLASS_NAVAL;
    else if (GetDomain() == DOMAIN_LAND and GetUnitNormalRole() == GRO_AIRDEFENSE)
        return RCLASS_AIRDEFENSE;
    else
        return RCLASS_GROUND;
}

SimBaseClass* UnitClass::GetSimTarget(void)
{
    FalconEntity *target;

    target = GetTarget();

    if ( not target)
        return NULL;;

    if (target->IsSim())
        return (SimBaseClass*) target;
    else if (target->IsCampaign())
    {
        // Get random component
        CampBaseClass *cTarget = (CampBaseClass*)target;
        SimBaseClass *theObj;
        int comp = 0;

        if (cTarget->IsUnit())
            comp = rand() % ((Unit)cTarget)->GetTotalVehicles();

        theObj = (SimBaseClass*)((CampEntity)target)->GetComponentEntity(comp);

        while (theObj)
        {
            if ( not theObj->IsDead() and not theObj->IsExploding() and theObj->IsAwake())
                return theObj;

            comp++;
            theObj = (SimBaseClass*)((CampEntity)target)->GetComponentEntity(comp);
        }
    }

    return NULL;
}

CampBaseClass* UnitClass::GetCampTarget(void)
{
    FalconEntity *target;

    target = GetTarget();

    if ( not target)
        return NULL;;

    if (target->IsSim())
        return ((SimBaseClass*)target)->GetCampaignObject();
    else
        return (CampBaseClass*)target;
}

CampEntity UnitClass::GetCargo(void)
{
    if ( not Cargo())
        return NULL;

    if ( not IsFlight() and not IsTaskForce())
        return NULL;

    return (CampEntity) vuDatabase->Find(cargo_id);
}

CampEntity UnitClass::GetTransport(void)
{
    if ( not Cargo())
        return NULL;

    if ( not IsBattalion() and not IsSquadron())
        return NULL;

    return (CampEntity) vuDatabase->Find(cargo_id);
}

VU_ID UnitClass::GetCargoID(void)
{
    if ( not Cargo())
        return FalconNullId;

    if ( not IsFlight() and not IsTaskForce())
        return FalconNullId;

    return cargo_id;
}

VU_ID UnitClass::GetTransportID(void)
{
    if ( not Cargo())
        return FalconNullId;

    if ( not IsBattalion() and not IsSquadron())
        return FalconNullId;

    return cargo_id;
}

void UnitClass::GetUnitDestination(GridIndex* X, GridIndex* Y)
{
    if ((dest_x == 0 or dest_y == 0) and not Parent())
    {
        GetUnitParent()->GetUnitDestination(X, Y);
    }
    else
    {
        *X = dest_x - 1;
        *Y = dest_y - 1;
    }
}

//void UnitClass::AssignUnit (VU_ID mgr, VU_ID po, VU_ID so, VU_ID ao, int orders)
void UnitClass::AssignUnit(VU_ID, VU_ID po, VU_ID so, VU_ID ao, int orders)
{
    VuTargetEntity* target = (VuTargetEntity*) vuDatabase->Find(OwnerId());
    FalconUnitAssignmentMessage* msg = new FalconUnitAssignmentMessage(Id(), target);

    msg->dataBlock.poid = po;
    msg->dataBlock.soid = so;
    msg->dataBlock.roid = ao;
    msg->dataBlock.orders = orders;
    FalconSendMessage(msg, TRUE);
}

//
// Support functions
//

float UnitClass::GetUnitMovementCost(GridIndex x, GridIndex y, CampaignHeading h)
{
    int flags = 0;

    if (GetUnitFormation() == GFORM_COLUMN)
        flags or_eq PATH_ROADOK;

    return GetMovementCost(x, y, GetMovementType(), flags, h);
}

int UnitClass::GetUnitObjectivePath(Path p, Objective o, Objective t)
{
    int flags = 0;

    if (GetDomain() == DOMAIN_LAND)
    {
        flags or_eq PATH_ROADOK;
        // KCK: Commented out to prevent units from stalling
        // if (GetUnitCurrentRole() == GRO_ATTACK or GetUnitCurrentRole() == GRO_FIRESUPPORT)
        flags or_eq PATH_ENEMYOK bitor PATH_ENEMYCOST;

        if (GetUnitNormalRole() == GRO_ENGINEER)
            flags or_eq PATH_ENGINEER;
    }

    maxSearch = OBJ_GROUND_PATH_MAX_SEARCH;
    return GetObjectivePath(p, o, t, GetMovementType(), GetTeam(), flags);
    maxSearch = MAX_SEARCH;
}

int UnitClass::GetUnitGridPath(Path p, GridIndex x, GridIndex y, GridIndex xx, GridIndex yy)
{
    int flags = 0, retval;

    if (GetDomain() == DOMAIN_LAND)
    {
        if (GetUnitFormation() == GFORM_COLUMN or GetUnitFormation() == GFORM_OVERWATCH or DistSqu(x, y, xx, yy) > 30)
            flags or_eq PATH_ROADOK bitor PATH_ENEMYCOST;

        if (GetUnitNormalRole() == GRO_ENGINEER)
            flags or_eq PATH_ENGINEER;
    }

    // Flights will never find a path inroute - only during planning and they should use
    // 2001-07-27 REMOVED BY S.G. ALLOWED IN RP5
    // ShiAssert ( GetMovementType() not_eq Air and GetMovementType() not_eq LowAir );

    maxSearch = GROUND_PATH_MAX;
    retval = GetGridPath(p, x, y, xx, yy, GetMovementType(), GetTeam(), flags);
    maxSearch = MAX_SEARCH;

    return retval;
}

void UnitClass::LoadUnit(Unit cargo)
{
    if (Cargo())
        return;

    Unit ourCargo = (Unit) vuDatabase->Find(cargo_id);

    if ( not ourCargo)
    {
        // KCK TODO: Abort Mission
        MonoPrint("Cargo is missing\n");
        return;
    }

    MonoPrint("Unit %d picking up unit %d.\n", GetCampID(), ourCargo->GetCampID());
    ourCargo->SetInactive(1);
    ourCargo->SetCargo(1);
    ourCargo->SetCargoId(Id());
    ourCargo->BroadcastUnitMessage(Id(), FalconUnitMessage::unitActivate, 1, 0, 0);
    SetCargo(1);
}

void UnitClass::UnloadUnit(void)
{
    if ( not Cargo())
        return;

    Unit ourCargo = (Unit) vuDatabase->Find(cargo_id);
    GridIndex x, y;

    // KCK: We should probably check to see if we're over valid territory,
    // and if not move towards valid territory (or throw the unit there)

    SetCargo(0);
    cargo_id = FalconNullId;

    if ( not ourCargo)
        return;

    // Play a radio message
    if (IsFlight())
    {
        FalconRadioChatterMessage *msg = new FalconRadioChatterMessage(Id(), FalconLocalGame);
        msg->dataBlock.from = Id();
        msg->dataBlock.to = MESSAGE_FOR_TEAM;
        msg->dataBlock.voice_id = ((Flight)this)->GetFlightLeadVoiceID();
        msg->dataBlock.message = rcAIRDROPAPPROACH;
        msg->dataBlock.edata[0] = ((Flight)this)->callsign_id;
        msg->dataBlock.edata[1] = ((Flight)this)->GetFlightLeadCallNumber();
        FalconSendMessage(msg, FALSE);
        msg = new FalconRadioChatterMessage(Id(), FalconLocalGame);
        msg->dataBlock.from = Id();
        msg->dataBlock.to = MESSAGE_FOR_TEAM;
        msg->dataBlock.voice_id = ((Flight)this)->GetFlightLeadVoiceID();
        msg->dataBlock.message = rcAIRDROPDONE;
        msg->dataBlock.edata[0] = ((Flight)this)->callsign_id;
        msg->dataBlock.edata[1] = ((Flight)this)->GetFlightLeadCallNumber();
        msg->dataBlock.time_to_play = CampaignMinutes;
        FalconSendMessage(msg, FALSE);
    }

    // Apply any damage sustained:
    // KCK TODO.
    GetLocation(&x, &y);
    MonoPrint("Unit %d dropping off unit %d at %d,%d.\n", GetCampID(), ourCargo->GetCampID(), x, y);
    ourCargo->SetCargo(0);
    ourCargo->SetCargoId(FalconNullId);
    ourCargo->SetLocation(x, y);
    ourCargo->BroadcastUnitMessage(Id(), FalconUnitMessage::unitActivate, 0, 0, 0);
}

CampaignTime UnitClass::GetUnitSupplyTime(void)
{
    CampaignTime time = compl 0;

    // Determine how long we need to wait to receive supplies
    if (IsBattalion())
    {
        if (GetUnitCurrentRole() == GRO_ATTACK)
            time = GND_OFFENSIVE_SUPPLY_TIME;
        else
            time = GND_DEFENSIVE_SUPPLY_TIME;
    }
    else if (IsSquadron())
    {
        // KCK: Air or Ground action?
        if (TeamInfo[GetTeam()]->GetGroundAction()->actionType == GACTION_OFFENSIVE)
            time = AIR_OFFENSIVE_SUPPLY_TIME;
        else
            time = AIR_DEFENSIVE_SUPPLY_TIME;
    }

    return time;
}

int UnitClass::CountUnitElements(void)
{
    Unit  e;
    int   i, els = 0;

    if ( not Father())
    {
        int mv;
        mv = TeamInfo[GetTeam()]->max_vehicle[GetRClass()];

        for (i = 0; i < mv; i++)
        {
            if (GetVehicleID((uchar)i))
                els++;
        }

        return els;
    }

    e = GetFirstUnitElement();

    while (e)
    {
        els++;
        e = GetNextUnitElement();
    }

    return els;
}

Unit UnitClass::GetRandomElement(void)
{
    Unit     e;
    int      els, dam;

    if ( not Father())
        return this;

    els = CountUnitElements();
    dam = rand() % (els + 1);

    for (els = 0, e = this; els < dam; els++)
    {
        if (els)
            e = GetNextUnitElement();
        else
            e = GetFirstUnitElement();
    }

    return e;
}

UnitClassDataType* UnitClass::GetUnitClassData(void)
{
    return class_data;
}

char* UnitClass::GetUnitClassName(void)
{
    if (class_data)
        return class_data->Name;
    else
        return "Nothing";
}

int UnitClass::GetTotalVehicles(void)
{
    short i = 0, t = 0;
    Unit e;

    if (Father())
    {
        e = GetUnitElement(i);

        while (e)
        {
            t += e->GetTotalVehicles();
            i++;
            e = GetUnitElement(i);
        }
    }
    else
    {
        if (TeamInfo[GetTeam()])
        {
            int mv = TeamInfo[GetTeam()]->max_vehicle[GetRClass()];

            for (i = 0; i < mv; i++)
            {
                t += GetNumVehicles(i);
            }
        }
    }

    return t;
}

// Gets number of vehicles we'd have if we were at full strength
// NOTE: This may not return what we expect for flights.
int UnitClass::GetFullstrengthVehicles(void)
{
    short i = 0, t = 0;
    Unit e;

    // KCK Hackish: We just assume all flights are fullstrength at 4 for score calculations
    if (IsFlight())
    {
        return 4;
    }

    if (Father())
    {
        e = GetUnitElement(i);

        while (e)
        {
            t += e->GetFullstrengthVehicles();
            i++;
            e = GetUnitElement(i);
        }
    }
    else
    {
        UnitClassDataType* uc;
        uc = GetUnitClassData();

        if (TeamInfo[GetTeam()])
        {
            int mv = TeamInfo[GetTeam()]->max_vehicle[GetRClass()];

            for (i = 0; i < mv; i++)
                t += uc->NumElements[i];
        }
    }

    return t;
}

int UnitClass::GetFullstrengthVehicles(int slot)
{
    if ( not Real())
        return 0;

    int mv = TeamInfo[GetTeam()]->max_vehicle[GetRClass()];

    if (slot >= mv)
        return 0;

    UnitClassDataType* uc = GetUnitClassData();
    return uc->NumElements[slot];
}

VehicleID UnitClass::GetVehicleID(int vg)
{
    UnitClassDataType* uc;
    uc = GetUnitClassData();

    if ( not uc)
    {
        return 0;
    }

    return uc->VehicleType[vg];
}

uchar* UnitClass::GetDamageModifiers(void)
{
    UnitClassDataType* uc;

    uc = GetUnitClassData();

    if ( not uc)
        return DefaultDamageMods;

    return uc->DamageMod;
}

int UnitClass::CollectWeapons(uchar* dam, MoveType m, short w[], uchar wc[], int dist)
{
    int cw = 0, bw, i, j, gotit, shot, total = 0, sup, max_salvos = 255;
    VehicleClassDataType* vc;

    // Quick check if we're totally out of supply
    sup = GetUnitSupply();

    if ( not sup)
        return 0;

    // KCK HACK: Ground units only take one salvo at a flight per weapon type (guided weapons only)
    if (MOVE_AIR(m) and not IsFlight())
        max_salvos = 1;

    if (GetTeam() < 0 or F4IsBadCodePtr((FARPROC) TeamInfo[GetTeam()]) or GetRClass() < 0 or F4IsBadReadPtr(&(TeamInfo[GetTeam()]->max_vehicle[GetRClass()]), sizeof(uchar))) // JB 010305 CTD
        return 0; // JB 010305 CTD

    int mv = TeamInfo[GetTeam()]->max_vehicle[GetRClass()];

    for (i = 0; i < mv and class_data->VehicleType[i] and cw < MAX_TYPES_PER_CAMP_FIRE_MESSAGE and wc[0] < max_salvos; i++)
    {
        bw = GetBestVehicleWeapon(i, dam, m, dist, &j);

        if (bw > 0 and CanShootWeapon(bw))
        {
            for (j = 0, gotit = 0; j < cw and not gotit; j++)
            {
                if (w[j] == bw)
                {
                    wc[j] += GetNumVehicles(i) * GetWeaponFireRate(w[j]);
                    gotit = 1;
                }
            }

            if ( not gotit)
            {
                w[cw] = bw;
                wc[cw] = GetNumVehicles(i) * GetWeaponFireRate(w[cw]);

                if (wc[cw] > max_salvos and (WeaponDataTable[w[cw]].GuidanceFlags bitand WEAP_GUIDED_MASK))
                    wc[cw] = GetWeaponFireRate(w[cw]);

                cw++;
            }
        }

        // Total this vehicle's ammunition for supply purposes
        vc = GetVehicleClassData(class_data->VehicleType[i]);

        for (j = 0; j < HARDPOINT_MAX and vc; j++)
        {
            if (vc->Weapons[j] < 255)
                total += vc->Weapons[j];
        }
    }

    if ( not total)
        return 0;

    // Now do supply
    for (i = 0, shot = 0; i < cw; i++)
    {
        wc[i] = (wc[i] * sup) / 100; // Adjust for supply

        if (wc[i] < 1)
            wc[i] = 1;

        shot += wc[i];
    }

    // sup = GetUnitSupply() - ((GetUnitSupply() * shot*2) / total) - 2; // The '*2' for shots makes up for non-popular weapons not getting shot
    sup = GetUnitSupply() - ((GetUnitSupply() * shot) / total); // The '*2' for shots makes up for non-popular weapons not getting shot

    if (sup < 0)
        sup = 0;

    SetUnitSupply(sup);
    return cw;
}

// This sets the current WP to the next in the list
void UnitClass::FinishUnitWP(void)
{
    current_wp++;

    if (GetCurrentUnitWP() == NULL)
    {
        ShiAssert( not IsFlight());
        current_wp = 0;
    }

    //MakeUnitDirty (DIRTY_WAYPOINT, DDP[75].priority);
    MakeUnitDirty(DIRTY_WAYPOINT, SEND_NOW);
}

WayPoint UnitClass::GetCurrentUnitWP() const
{
    WayPoint w;
    int i;

    if ( not current_wp)
        return NULL;

    w = wp_list;
    i = 1;

    while (w and i not_eq current_wp)
    {
        w = w->GetNextWP();
        i++;
    }

    return w;
}

void UnitClass::SetCurrentUnitWP(WayPoint w)
{
    int i;
    WayPoint tw;

    tw = wp_list;

    if ( not tw or not w)
    {
        // ShiAssert( not IsFlight());
        current_wp = 0;
        return;
    }

    i = 1;

    while (tw and tw not_eq w)
    {
        tw = tw->GetNextWP();
        i++;
    }

    current_wp = i;

    // KCK Hack to help out tacE flights which got saved with no mission target
    if (IsFlight() and GetUnitMissionTargetID() == FalconNullId and w->GetWPTargetID())
        SetUnitMissionTarget(w->GetWPTargetID());

    /* if (IsFlight() or IsTaskForce())
     {
    // Set yaw to current wp's heading
    float xd,yd,zd;
    w->GetLocation(&xd,&yd,&zd);
    xd -= XPos();
    yd -= YPos();
    SetYPR(atan2(yd,xd),0.0F,0.0F);
    }
     */
    //MakeUnitDirty (DIRTY_WAYPOINT, DDP[76].priority);
    MakeUnitDirty(DIRTY_WAYPOINT, SEND_NOW);
    return;
}

WayPoint UnitClass::GetUnitMissionWP(void)
{
    WayPoint w;

    w = GetFirstUnitWP();

    while (w)
    {
        if (w->GetWPFlags() bitand WPF_TARGET)
            return w;

        w = w->GetNextWP();
    }

    return NULL;
}

// This will adjust a unit waypoints to be accurate from the unit's current location,
// while keeping departure times the same.
// This should be called to update wp times after hitting a repeating waypoint
void UnitClass::AdjustWayPoints(void)
{
    WayPoint w;
    GridIndex x, y;
    Unit u;

    w = GetCurrentUnitWP();

    if (w)
    {
        GetLocation(&x, &y);
        SetWPTimes(w, x, y, GetCruiseSpeed(), WPTS_KEEP_DEPARTURE_TIMES);
    }

    u = GetFirstUnitElement();

    while (u)
    {
        u->AdjustWayPoints();
        u = GetNextUnitElement();
    }
}

void UnitClass::ResetMoves(void)
{
    Unit        n;
    GridIndex x, y;

    if (IsBattalion() or IsTaskForce())
    {
        ClearUnitPath();
        GetLocation(&x, &y);
        SetUnitDestination(x, y);
        SetTempDest(0);
    }

    if (Father())
    {
        n = GetFirstUnitElement();

        while (n not_eq NULL)
        {
            n->ResetMoves();
            n = GetNextUnitElement();
        }
    }
}

void UnitClass::ResetLocations(GridIndex x, GridIndex y)
{
    Unit e;

    SetLocation(x, y);

    if (Father())
    {
        e = GetFirstUnitElement();

        while (e)
        {
            e->ResetLocations(x, y);
            e = GetNextUnitElement();
        }
    }

#ifdef USE_FLANKS

    if (IsBattalion())
    {
        ((Battalion)this)->lfx = ((Battalion)this)->rfx = x;
        ((Battalion)this)->lfy = ((Battalion)this)->rfy = y;
    }

#endif
}

void UnitClass::ResetDestinations(GridIndex x, GridIndex y)
{
    Unit e;

    SetUnitDestination(x, y);

    if (Father())
    {
        e = GetFirstUnitElement();

        while (e)
        {
            e->ResetDestinations(x, y);
            e = GetNextUnitElement();
        }
    }
}

void UnitClass::DisposeWayPoints(void)
{
    WayPoint    w, t;

    w = wp_list;
    current_wp = 0;

    while (w not_eq NULL)
    {
        t = w;
        w = w->GetNextWP();
        delete t;
    }

    wp_list = NULL;

    if (IsFlight())
        //MakeUnitDirty (DIRTY_WP_LIST, DDP[77].priority);
        MakeUnitDirty(DIRTY_WP_LIST, SEND_NOW);

    //MakeUnitDirty (DIRTY_WAYPOINT, DDP[78].priority);
    MakeUnitDirty(DIRTY_WAYPOINT, SEND_NOW);
}

void UnitClass::CheckBroken(void)
{
    Unit e;

    if ( not Parent())
        return;

    e = GetFirstUnitElement();

    while (e and e->Broken())
        e = GetNextUnitElement();

    if (e)
        SetBroken(1);
}

void UnitClass::BuildElements(void)
{
    int i, R, mv = VEHICLE_GROUPS_PER_UNIT;

    // This is a hack- basically, Squadrons arn't "real", but need their elements built.
    if ( not Real() and not IsSquadron())
    {
        return;
    }

    R = 0;

    if ( not class_data)
    {
        return;
    }

    if (TeamInfo[GetTeam()])
    {
        mv = TeamInfo[GetTeam()]->max_vehicle[GetRClass()];
    }

    for (i = 0; i < mv; i++)
    {
        R = R xor (class_data->NumElements[i]) << (i * 2);
    }

    roster = R;
}

// Score for air/ground roles
int UnitClass::GetUnitRoleScore(int role, int calcType, int use_to_calc)
{
    int score = 0, s, i = 0;
    Unit e;

    if (calcType == CALC_MIN)
        score = 32000;

    if (Father())
    {
        e = GetUnitElement(i);

        while (e)
        {
            s = e->GetUnitRoleScore(role, calcType, use_to_calc);

            switch (calcType)
            {
                case CALC_MAX:
                    if (s > score)
                        score = s;

                    break;

                case CALC_MIN:
                    if (s < score)
                        score = s;

                    break;

                default:
                    score += s;
                    break;
            }

            i++;
            e = GetUnitElement(i);
        }

        if (calcType == CALC_AVERAGE)
            score /= i;
    }
    else
    {
        score = class_data->Scores[role];

        if (use_to_calc bitand USE_VEH_COUNT)
            score = score * GetTotalVehicles() / GetFullstrengthVehicles();

        if (use_to_calc bitand USE_EXP)
        {
            if (GetRClass() == RCLASS_AIR)
                score = FloatToInt32(score * AirExperienceAdjustment(GetOwner()));
            else if (GetRClass() == RCLASS_NAVAL)
                score = FloatToInt32(score * NavalExperienceAdjustment(GetOwner()));
            else if (GetRClass() == RCLASS_AIRDEFENSE)
                score = FloatToInt32(score * AirDefenseExperienceAdjustment(GetOwner()));
            else
                score = FloatToInt32(score * GroundExperienceAdjustment(GetOwner()));
        }

        if (use_to_calc bitand IGNORE_BROKEN and Broken())
            score /= 5;
    }

    return score;
}

int UnitClass::GetBestVehicleWeapon(int slot, uchar *dam, MoveType mt, int range, int *hp)
{
    int i, str, bs, w, ws, bw, bhp = -1;
    VehicleClassDataType* vc;

    ShiAssert(slot >= 0 and slot < VEHICLE_GROUPS_PER_UNIT);
    ShiAssert(class_data not_eq NULL);
    vc = GetVehicleClassData(class_data->VehicleType[slot]);

    if ( not vc)
        return 0;

    ShiAssert(vc);

    bw = bs = 0;

    for (i = 0; i < HARDPOINT_MAX; i++)
    {
        w = vc->Weapon[i];
        ws = vc->Weapons[i];
        ShiAssert(ws < 255)

        if (w and ws)
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

// Returns current vehicle best hit chance
int UnitClass::GetVehicleHitChance(int slot, MoveType mt, int range, int hitflags)
{
    VehicleClassDataType* vc;
    int i, bc = 0, hc = 0, wid;

    ShiAssert(slot >= 0 and slot < VEHICLE_GROUPS_PER_UNIT);
    ShiAssert(class_data not_eq NULL);
    vc = GetVehicleClassData(class_data->VehicleType[slot]);

    if ( not vc)
        return 0;

    for (i = 0; i < HARDPOINT_MAX; i++)
    {
        wid = GetUnitWeaponId(i, slot);

        if (wid and GetUnitWeaponCount(i, slot))
        {
            if (::GetWeaponRange(wid, mt) >= range)
                hc = GetWeaponHitChance(wid, mt);

            if (hc > bc)
                bc = hc;
        }
    }

    if ( not bc)
        return 0;

    // Return weapon hit chance plus inherent hit chance
    return bc + vc->HitChance[mt];
}

// Returns the exact strength at a given range
int UnitClass::GetVehicleCombatStrength(int slot, MoveType mt, int range)
{
    int i, str = 0, bs = 0, wid;
    VehicleClassDataType* vc;
    ShiAssert(slot >= 0 and slot < VEHICLE_GROUPS_PER_UNIT);
    ShiAssert(class_data not_eq NULL);

    vc = GetVehicleClassData(class_data->VehicleType[slot]);

    if ( not vc)
        return 0;

    for (i = 0; i < HARDPOINT_MAX; i++)
    {
        wid = GetUnitWeaponId(i, slot);

        if (wid and GetUnitWeaponCount(i, slot))
        {
            if (::GetWeaponRange(wid, mt) >= range)
                str = GetWeaponScore(wid, mt, range);

            if (str > bs)
                bs = str;
        }
    }

    return bs;
}

// Returns current vehicle maximum range
int UnitClass::GetVehicleRange(int slot, int mt, FalconEntity *target)  // 2002-03-08 MODIFIED BY S.G. Need to pass it a target sometime so default to NULL for most cases
{
    ShiAssert(slot >= 0 and slot < VEHICLE_GROUPS_PER_UNIT);
    ShiAssert(class_data not_eq NULL);
    int i, wid, rng, br = 0;
    VehicleClassDataType* vc;

    vc = GetVehicleClassData(class_data->VehicleType[slot]);

    if ( not vc)
        return 0;

    for (i = 0; i < HARDPOINT_MAX; i++)
    {
        wid = GetUnitWeaponId(i, slot);

        if (wid and GetUnitWeaponCount(i, slot))
        {
            // 2002-03-08 ADDED BY S.G. If we're a ground thingy, we're shooting at an air thingy and this weapon is a ... STYPE_MISSILE_SURF_AIR, we might be restricted to a min/max engagement range/altitude if we asked for it
            if (g_bLimit2DRadarFight and target and OnGround() and (mt == LowAir or mt == Air))
            {
                VU_BYTE *classInfoPtr = Falcon4ClassTable[WeaponDataTable[wid].Index].vuClassData.classInfo_;

                if (classInfoPtr[VU_DOMAIN] == DOMAIN_AIR and classInfoPtr[VU_CLASS] == CLASS_VEHICLE and classInfoPtr[VU_TYPE] == TYPE_MISSILE and classInfoPtr[VU_STYPE] == STYPE_MISSILE_SURF_AIR)
                {
                    MissileAuxData *auxData = NULL;
                    SimWeaponDataType* wpnDefinition = &SimWeaponDataTable[Falcon4ClassTable[WeaponDataTable[wid].Index].vehicleDataIndex];

                    if (wpnDefinition->dataIdx < numMissileDatasets)
                        auxData = missileDataset[wpnDefinition->dataIdx].auxData;

                    if (auxData)
                    {
                        float minAlt = auxData->MinEngagementAlt;
                        float minRange = auxData->MinEngagementRange;
                        float maxAlt = (float)WeaponDataTable[wid].MaxAlt * 1000.0f;
                        float diffX, diffY, diffZ, rangeSqr;

                        // If we haven't entered the MinEngagementAlt yet, use the one in the Falcon4.WCD file
                        if (minAlt < 0.0f)
                            minAlt = (float)(WeaponDataTable[wid].Name[18]) * 32.0F;

                        diffX = XPos() - target->XPos();
                        diffY = YPos() - target->YPos();
                        diffZ = ZPos() - target->ZPos();

                        // If our range is less than the min range, don't consider this weapon (used range squared to save a FPU costly sqrt)
                        rangeSqr = diffX * diffX + diffY * diffY;

                        if (rangeSqr < minRange * minRange)
                            continue;

                        // If less than min altitude or more than max altitude (in this case diffZ is POSITIVE if we're below the target), don't consider this weapon
                        if (diffZ < minAlt or diffZ > maxAlt)
                            continue;
                    }
                }
            }

            // END OF ADDED SECTION 2002-03-08

            rng = ::GetWeaponRange(wid, mt);

            if (rng > br)
                br = rng;
        }
    }

    return br;
}

int UnitClass::GetUnitWeaponId(int hp, int slot)
{
    ShiAssert(slot >= 0 and slot < VEHICLE_GROUPS_PER_UNIT);
    ShiAssert(hp >= 0 and hp < HARDPOINT_MAX);
    ShiAssert(class_data not_eq NULL);

    VehicleClassDataType* vc;

    vc = GetVehicleClassData(class_data->VehicleType[slot]);

    if ( not vc)
        return 0;

    return vc->Weapon[hp];
}

int UnitClass::GetUnitWeaponCount(int hp, int slot)
{
    ShiAssert(slot >= 0 and slot < VEHICLE_GROUPS_PER_UNIT);
    ShiAssert(hp >= 0 and hp < HARDPOINT_MAX);
    ShiAssert(class_data not_eq NULL);
    VehicleClassDataType* vc;
    int wc;

    vc = GetVehicleClassData(class_data->VehicleType[slot]);

    if ( not vc)
        return 0;

    wc = vc->Weapons[hp];

    // Use supply % to determine # of shots
    if (wc > 2)
    {
        // if (# of shots > 2) - supply % of shots
        wc = (wc * GetUnitSupply()) / 100;
    }
    else if (rand() % 100 > GetUnitSupply())
    {
        // otherwise - supply % of vehs have shots
        wc = 0;
    }

    return wc;
}

// ============================================
// Functions on units
// ============================================

void SaveUnits(char* scenario)
{
    long size;
    FILE *fp;
    uchar *buffer;

    size = EncodeUnitData(&buffer, NULL);

    if ((fp = OpenCampFile(scenario, "uni", "wb")) == NULL)
        return;

    fwrite(&size, sizeof(long), 1, fp);
    fwrite(buffer, size, 1, fp);
    CloseCampFile(fp);
    delete buffer;
}

//sfr: added to size(as remaining) to DecodeUnitData
int LoadUnits(char* scenario)
{
    long size;
    uchar /* *data,*/ *data_ptr;;


    CampaignData cd = ReadCampFile(scenario, "uni");

    if (cd.dataSize == -1)
    {
        return 0;
    }

    data_ptr = (uchar*)cd.data;
    long rem = cd.dataSize;

    //take size out and update pointer
    //this size is not used anywhere, so ill trust my size
    memcpychk(&size, &data_ptr, sizeof(long), &rem);

    //updated the call to include remaining value
    DecodeUnitData(&data_ptr, &rem, NULL);
    delete cd.data;

    // KCK HACK: Reset any saved off player slots
    VuListIterator myit(AllAirList);
    Unit u;
    u = (Unit) myit.GetFirst();

    while (u)
    {
        if (u->IsFlight())
            memset(((FlightClass*)u)->player_slots, NO_PILOT, PILOTS_PER_FLIGHT * sizeof(uchar));

        u = (Unit) myit.GetNext();
    }

    return 1;
}

Unit GetFirstUnit(F4LIt l)
{
    VuEntity* e;

    e = l->GetFirst();

    while (e)
    {
        if (GetEntityClass(e) == CLASS_UNIT)
            return (Unit)e;

        e = l->GetNext();
    }

    return NULL;
}

Unit GetNextUnit(F4LIt l)
{
    VuEntity *e;

    e = l->GetNext();

    while (e)
    {
        //if (e->VuState() not_eq VU_MEM_DELETED)
        if (GetEntityClass(e) == CLASS_UNIT)
        {
            return (Unit)e;
        }

        e = l->GetNext();
    }

    return NULL;
}

Unit GetUnitByID(VU_ID id)
{
#if VU_ALL_FILTERED
    VuListIterator it(AllUnitList);

    for (VuEntity *u = it.GetFirst(); u not_eq NULL; u = it.GetNext())
    {
        if (u->Id() == id)
        {
            return static_cast<Unit>(u);
        }
    }

    return NULL;
#else
    return (Unit)AllUnitList->Find(id);
#endif
}

Unit ConvertUnit(Unit u, int domain, int type, int stype, int sptype)
{
    Unit nu, p = NULL;
    GridIndex x, y;
    short z;

    nu = NewUnit(domain, type, stype, sptype, u->GetUnitParent());

    if ( not nu)
        return u;

    u->DisposeChildren();

    // Copy Data (Common Unit, CampBase, and VuEntity data)
    nu->SetLastCheck(u->GetLastCheck());
    nu->SetRoster(u->GetRoster());
    nu->SetUnitFlags(u->GetUnitFlags());
    nu->SetDestX(u->GetDestX());
    nu->SetDestY(u->GetDestY());
    nu->SetMoved(u->GetMoved());
    nu->SetLosses(u->GetLosses());
    nu->SetTactic(u->GetTactic());
    nu->SetCurrentWaypoint(u->GetCurrentWaypoint());
    nu->SetNameId(u->GetNameId());
    nu->wp_list = u->wp_list;
    nu->SetSpottedTime(u->GetSpotTime());
    nu->SetSpotted(u->GetOwner(), u->GetSpotted());
    nu->SetBaseFlags(u->GetBaseFlags());
    nu->SetOwner(u->GetOwner());
    nu->SetTargetId(u->GetTargetId());
    nu->SetCampId(u->GetCampId());
    u->GetLocation(&x, &y);
    z = u->GetAltitude();
    nu->SetLocation(x, y);
    nu->SetAltitude(z);
    nu->BuildElements();

    u->Remove();
    return nu;
}

int GetArrivalSpeed(const UnitClass *u)
{
    WayPoint w;
    GridIndex x, y, wx, wy;
    float d;
    CampaignTime t, c;
    int maxs, reqs;

    if (u->Father())
        maxs = u->GetFormationCruiseSpeed();
    else
        maxs = u->GetMaxSpeed();

    w = u->GetCurrentUnitWP();

    if ( not w)
        return maxs;

    u->GetLocation(&x, &y);
    w->GetWPLocation(&wx, &wy);
    d = Distance(x, y, wx, wy);
    t = w->GetWPArrivalTime();
    c = Camp_GetCurrentTime();

    if (t > c)
    {
        reqs = FloatToInt32(d * CampaignHours / (t - c));

        if (u->IsFlight() and reqs < maxs / 2)
            reqs = maxs / 2; // Minimal speed

        if (reqs < maxs)
            return reqs;
    }

    return maxs;
}

_TCHAR* GetSizeName(int domain, int type, _TCHAR *buffer)
{
    if (domain == DOMAIN_AIR)
    {
        if (type == TYPE_SQUADRON)
            ReadIndexedString(610, buffer, 19);
        else if (type == TYPE_FLIGHT)
            ReadIndexedString(611, buffer, 19);
        else if (type == TYPE_PACKAGE)
            ReadIndexedString(612, buffer, 19);
    }
    else if (domain == DOMAIN_LAND)
    {
        if (type == TYPE_BRIGADE)
            ReadIndexedString(614, buffer, 19);
        else if (type == TYPE_BATTALION)
            ReadIndexedString(615, buffer, 19);
    }
    else if (domain == DOMAIN_SEA)
        ReadIndexedString(616, buffer, 19);
    else
        ReadIndexedString(617, buffer, 19);

    return buffer;
}

_TCHAR* GetDivisionName(int div, int type, _TCHAR *buffer, int size, int object)
{
    _TCHAR temp1[10], temp2[20], temp3[20], format[10];

    GetNumberName(div, temp1);
    GetSTypeName(DOMAIN_LAND, 0, type, temp2);
    _tcscpy(format, "%s %s %s");

    if (gLangIDNum == F4LANG_GERMAN)
    {
        // Replace space with hyphen, if necessary
        if (temp2[_tcslen(temp2) - 1] == '-')
            _tcscpy(format, "%s %s%s");
    }

    ReadIndexedString(613, temp3, 19);

    ShiAssert(((int) _tcslen(temp1) + _tcslen(temp2) + _tcslen(temp3) + 3) < static_cast<unsigned long>(size));

    if (gLangIDNum >= F4LANG_SPANISH)
        _sntprintf(buffer, size, format, temp1, temp3, temp2);
    else
        _sntprintf(buffer, size, format, temp1, temp2, temp3);

    // _sntprintf should do this for us, but for some reason it sometimes doesn't
    buffer[size - 1] = 0;

    return buffer;
}

_TCHAR* GetDivisionName(int div, _TCHAR *buffer, int size, int object)
{
    Unit e;
    uchar count[50];
    int bcount = 0, btype = 0, i;

    if ( not div)
    {
        ReadIndexedString(167, buffer, 29);
        return buffer;
    }

    memset(count, 0, 50);

    {
        VuListIterator myit(AllUnitList);
        e = GetFirstUnit(&myit);

        while (e)
        {
            if (e->GetUnitDivision() == div and e->Parent())
                count[e->GetSType()]++;

            e = GetNextUnit(&myit);
        }
    }

    for (i = 0; i < 50; i++)
    {
        if (count[i] > bcount)
        {
            bcount = count[i];
            btype = i;
        }
    }

    return GetDivisionName(div, btype, buffer, size, object);
}

int FindUnitNameID(Unit u)
{
    Unit f, e;
    int div, element = 1;

    if (u->Parent())
    {
        div = u->GetUnitDivision();
        VuListIterator myit(AllUnitList);
        e = GetFirstUnit(&myit);

        while (e)
        {
            if (e->GetUnitDivision() == div and e->GetOwner() == u->GetOwner() and e->GetDomain() == DOMAIN_LAND and e->Parent())
            {
                if (e == u)
                {
                    if (u->GetUnitNameID())
                        return u->GetUnitNameID();
                }
                else
                    element++;
            }

            e = GetNextUnit(&myit);
        }
    }
    else
    {
        f = u->GetUnitParent();
        e = f->GetFirstUnitElement();

        while (e)
        {
            if (e == u)
            {
                if (u->GetUnitNameID())
                    return u->GetUnitNameID();
            }
            else
                element++;

            e = f->GetNextUnitElement();
        }
    }

    return element;
}

Unit NewUnit(int domain, int type, int stype, int sptype, Unit parent)
{
    Unit cur = NULL;
    ushort tid = GetClassID(domain, CLASS_UNIT, type, stype, sptype, 0, 0, 0);
    VU_ID_NUMBER id, low, hi;
    id = low = hi = 0;
    bool error = false;

    if ( not tid)
    {
        return NULL;
    }

    tid += VU_LAST_ENTITY_TYPE;

    if (domain == DOMAIN_AIR)
    {
        switch (type)
        {
            case TYPE_PACKAGE:
                cur = NewPackage(tid);
                break;

            case TYPE_FLIGHT:
                cur = NewFlight(tid, parent, NULL);
                break;

            case TYPE_SQUADRON:
                cur = NewSquadron(tid);
                break;

            default:
                error = true;
        }
    }
    else if (domain == DOMAIN_LAND)
    {
        switch (type)
        {
            case TYPE_BRIGADE:
                cur = NewBrigade(tid);
                break;

            case TYPE_BATTALION:
                cur = NewBattalion(tid, parent);
                break;

            default:
                error = true;
        }
    }
    else if (domain == DOMAIN_SEA)
    {
        switch (type)
        {
            case TYPE_TASKFORCE:
                cur = NewTaskForce(tid);
                break;

            default:
                error = true;
        }
    }

    if (error)
    {
        MessageBox(NULL, "Type conflict while creating unit.", "Error", MB_OK bitor MB_ICONSTOP bitor MB_SETFOREGROUND);
    }

    cur->BuildElements();
    vuDatabase->Insert(cur/*, id, low, hi*/);
    return cur;
}

//sfr: changed proto
//Unit NewUnit (short tid, VU_BYTE **stream)
Unit NewUnit(short tid, VU_BYTE **stream, long *rem)
{
    Unit cur = NULL;
    Falcon4EntityClassType* classPtr = &(Falcon4ClassTable[tid - VU_LAST_ENTITY_TYPE]);

    if ( not tid)
    {
        return NULL;
    }

    CampEnterCriticalSection();

    if (classPtr->vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_AIR)
    {
        if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_FLIGHT)
        {
            cur = new FlightClass(stream, rem);
        }
        else if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_SQUADRON)
        {
            cur = new SquadronClass(stream, rem);
        }
        else if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_PACKAGE)
        {
            cur = new PackageClass(stream, rem);
        }
        else
        {
            MessageBox(NULL, "Type conflict while creating unit.", "Error", MB_OK bitor MB_ICONSTOP bitor MB_SETFOREGROUND);
        }
    }
    else if (classPtr->vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_LAND)
    {
        if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_BRIGADE)
        {
            cur = new BrigadeClass(stream, rem);
        }
        else if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_BATTALION)
        {
            cur = new BattalionClass(stream, rem);
        }
        else
        {
            MessageBox(NULL, "Type conflict while creating unit.", "Error", MB_OK bitor MB_ICONSTOP bitor MB_SETFOREGROUND);
        }
    }
    else if (classPtr->vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_SEA)
    {
        if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_TASKFORCE)
        {
            cur = new TaskForceClass(stream, rem);
        }
        else
        {
            MessageBox(NULL, "Type conflict while creating unit.", "Error", MB_OK bitor MB_ICONSTOP bitor MB_SETFOREGROUND);
        }
    }
    else
    {
        MessageBox(NULL, "Type conflict while creating unit.", "Error", MB_OK bitor MB_ICONSTOP bitor MB_SETFOREGROUND);
        cur = NULL;
    }

    CampLeaveCriticalSection();
    return cur;
}

float GetRange(Unit us, CampEntity them)
{
    GridIndex x, y, tx, ty;

    theirDomain = 0;

    if ( not them)
        return 0.0F;

    // Set the domain while we're here
    if (them->GetDomain() == DOMAIN_AIR)
        theirDomain = 1;
    else if (them->IsUnit())
        theirDomain = 2;
    else if (them->IsObjective())
        theirDomain = 3;

    // return range
    us->GetLocation(&x, &y);
    them->GetLocation(&tx, &ty);
    return Distance(x, y, tx, ty);
}

// FRB - First taxiPoint selected <== GetDeaggregationPoint() (Flight.cpp)
int FindTaxiPt(Flight flight, Objective airbase, int checklist)
{
    int time_til_takeoff = 0, tp, rwindex = 0, pt, parkPt; // in 10 second blocks
    bool goanyway = false;
    ulong takeoff_time;
    pt = 0; // Cobra

    WayPoint w;
    runwayQueueStruct *info = NULL;

    w = flight->GetCurrentUnitWP();

    if (checklist and airbase and airbase->brain)
        info = airbase->brain->InList(flight->Id());

    if (info)
    {
        rwindex = info->rwindex;
        takeoff_time = info->schedTime;
    }
    else
    {
        if (airbase and airbase->brain)
            rwindex = airbase->brain->FindBestTakeoffRunway(checklist);

        takeoff_time = w->GetWPDepartureTime();
    }

    if ( not rwindex)
        return DPT_ERROR_CANT_PLACE; // Error, runway is toast

    if (takeoff_time > SimLibElapsedTime and w->GetWPAction() == WP_TAKEOFF)
        time_til_takeoff = (takeoff_time - SimLibElapsedTime) / (TAKEOFF_TIME_DELTA);

    // JPO - if this is true, we should be able to go anyway.as were after the min deag time.
    if (g_nDeagTimer > 0 and airbase and airbase->brain and SimLibElapsedTime > w->GetWPArrivalTime() - airbase->brain->MinDeagTime() - CampaignMinutes * g_nDeagTimer)
        goanyway = true;
    else if (flight->IsSetFalcFlag(FEC_PLAYER_ENTERING bitor FEC_HASPLAYERS)) // players go too
        goanyway = true;

    // Cobra - Determine parking/spawning spots later in Aircraft.cpp FindBestSpawningPoint()
    if (time_til_takeoff > PtHeaderDataTable[rwindex].count)
    {
        if ( not checklist)
            return 0; // Takeoff longer away than we have taxi pts for

        time_til_takeoff = PtHeaderDataTable[rwindex].count; // JPO - just go in this case
    }

    // Cobra - FRB - Replaced with my landme.cpp code
    if (time_til_takeoff < 0)
        time_til_takeoff = 0;

    tp = GetFirstPt(rwindex);
    int prevPt = tp;
    parkPt = -1; // FRB
    tp = GetNextPt(tp);

    while (tp and time_til_takeoff)
    {
        prevPt = tp;
        tp = GetNextPt(tp); // FRB - Look at all of them

        if (PtDataTable[tp].type == CritTaxiPt)
        {
            break;
        }

        // 17JAN04 - FRB - Locate a suitable parking spot
        if ((PtDataTable[tp].type == SmallParkPt) or (PtDataTable[tp].type == LargeParkPt))
        {
            if (PtDataTable[tp].flags bitand PT_OCCUPIED)
            {
                time_til_takeoff--;
                continue;  // Taken
            }
            else
            {
                parkPt = tp;
                time_til_takeoff--;
                continue;
            }
        }

        time_til_takeoff--;
    }

    // 17JAN04 - FRB - Use nearest Parking spot
    if (parkPt >= 0)
    {
        tp = parkPt;
        return tp;
    }

    if (tp)
    {
        return tp;
    }

    return prevPt;
}

int EncodeUnitData(VU_BYTE **stream, FalconSessionEntity *owner)
{
    long            size = 0, newsize;
    short num = 0, count = 0, type;
    Unit cur;
    VU_BYTE *buf, *sptr, *bufhead;
    //#ifdef DEBUG
    // //VU_BYTE *temp;
    //#endif
    VU_ID vuid, ownerid;
    // char buffer[100];

    if ( not AllUnitList)
        return 0;

    if (owner)
        ownerid = owner->Id();

    CampEnterCriticalSection();

    // Count # of units and calculate size
    {
        VuListIterator myit(AllUnitList);
        cur = GetFirstUnit(&myit);

        while (cur)
        {
            if (( not owner or cur->OwnerId() == ownerid) and not cur->IsDead())
            {
                size += cur->SaveSize() + sizeof(short);
                count++;
            }

            cur = GetNextUnit(&myit);
        }
    }

    // Count Inactive units
    {
        VuListIterator  iit(InactiveList);
        cur = GetFirstUnit(&iit);

        while (cur)
        {
            if (( not owner or cur->OwnerId() == ownerid) and not cur->IsDead())
            {
                size += cur->SaveSize() + sizeof(short);
                count++;
            }

            cur = GetNextUnit(&iit);
        }
    }

    buf = new VU_BYTE[size + 1];
    bufhead = buf;

    // save_log = fopen ("save.log", "w");
    // start_save_stream = (int) buf;

    // Save Units one at a time, with a domain/type header, so we can load it correctly
    {
        VuListIterator myit(AllUnitList);
        cur = GetFirstUnit(&myit);

        while (cur)
        {
            if (( not owner or cur->OwnerId() == ownerid) and not cur->IsDead())
            {
                type = cur->Type();
                memcpy(buf, &type, sizeof(short));
                buf += sizeof(short);
                cur->Save(&buf);

                if (save_log)
                {
                    fprintf(save_log, "\n");
                    fflush(save_log);
                }
            }

            cur = GetNextUnit(&myit);
        }
    }

    // Save Inactive units
    {
        VuListIterator  iit(InactiveList);
        cur = GetFirstUnit(&iit);

        while (cur)
        {
            if (( not owner or cur->OwnerId() == ownerid) and not cur->IsDead())
            {
                type = cur->Type();
                newsize = cur->SaveSize();
                memcpy(buf, &type, sizeof(short));
                buf += sizeof(short);
                cur->Save(&buf);
            }

            cur = GetNextUnit(&iit);
        }
    }
    ShiAssert((int)(buf - bufhead) == size); // JPO - this fired.

    CampLeaveCriticalSection();

    // Compress it and return
    *stream = new VU_BYTE[size + sizeof(short) + sizeof(long) + MAX_POSSIBLE_OVERWRITE];
    sptr = *stream;
    memcpy(sptr, &count, sizeof(short));
    sptr += sizeof(short);
    memcpy(sptr, &size, sizeof(long));
    sptr += sizeof(long);

    MonoPrint("Count=%1d,Size=%1ld\n", count, size);

    buf = bufhead;
    newsize = LZSS_Compress(buf, sptr, size);
    delete bufhead;

    MonoPrint("Encode Unit Data %d => %d\n", size, newsize);

    if (save_log)
    {
        fclose(save_log);
        save_log = 0;
    }

    return newsize + sizeof(short) + sizeof(long);
}

int DecodeUnitData(VU_BYTE **stream, long *rem, FalconSessionEntity *owner)
{
    long            size;
    short count, last_type, type, num;
    VU_ID vuid;
    Unit cur;
    VU_BYTE *buf, *bufhead, *last_buf, *start_buf;
    // char buffer[100];

    memcpychk(&count, stream, sizeof(short), rem);
    memcpychk(&size, stream, sizeof(long), rem);

    //we dont decode if we dont have to
    if (size == 0)
    {
        return 0;
    }

    //we use buf head to free buffer later
    buf = new VU_BYTE[size];
    bufhead = buf;

    //now we check the expand return
    if ((LZSS_Expand(*stream, rem[0], buf, size)) <= 0)
    {
        // char err[200];
        // sprintf(err, "%s %d: error expanding unit data", __FILE__, __LINE__);
        // throw std::InvalidBufferException(err);
    }

    rem[0] = size;

    start_buf = 0;
    type = 0;

    num = 0;

    while (count)
    {
        last_buf = start_buf;
        last_type = type;

        start_buf = buf;

        memcpychk(&type, &buf, sizeof(short), rem);

        cur = NewUnit(type, &buf, rem);

        if (load_log)
        {
            fprintf(load_log, "\n");
            fflush(load_log);
        }

        if (cur and owner)
        {
            cur->FalconEntity::SetOwnerId(owner->Id());
        }

        // sfr: dont think this is necessary since unit is not local... but anyway
        cur->SetSendCreate(VuEntity::VU_SC_DONT_SEND);
        vuDatabase->/*Silent*/Insert(cur);

        // Special case shit for tactical engagement
        if (FalconLocalGame and cur->IsBattalion() and FalconLocalGame->GetGameType() == game_TacticalEngagement)
        {
            // Emitters are always spotted and emitting in tactical engagement
            if (cur->GetRadarType() not_eq RDR_NO_RADAR)
            {
                cur->SetSearchMode(FEC_RADAR_SEARCH_1);//me123 + rand()%3);
                cur->SetEmitting(1);
            }
            else
                cur->SetEmitting(0);

            for (int i = 0; i < NUM_TEAMS; i++)
                cur->SetSpotted(i, TheCampaign.CurrentTime);
        }

        count--;
    }

    delete bufhead;

    if (load_log)
    {
        fclose(load_log);
        load_log = 0;
    }

    return 0;
}

// ============================================================
// Deaggregated data class
// ============================================================

UnitDeaggregationData::UnitDeaggregationData(void)
{
    num_vehicles = 0;
}

UnitDeaggregationData::~UnitDeaggregationData()
{
}

void UnitDeaggregationData::StoreDeaggregationData(Unit theUnit)
{
    if ( not theUnit or theUnit->IsAggregate() or theUnit->GetDomain() not_eq DOMAIN_LAND or not theUnit->GetComponents())
        return;

    int vehicles = 0, slot;
    int inSlot[VEHICLE_GROUPS_PER_UNIT] = { 0 };
    SimBaseClass* vehicle;

    // Init them to something reasonable
    for (slot = 0; slot < 3 * VEHICLE_GROUPS_PER_UNIT; slot++)
    {
        position_data[slot].x = theUnit->XPos();
        position_data[slot].y = theUnit->YPos();
        position_data[slot].heading = theUnit->Yaw();
    }

    VuListIterator myit(theUnit->GetComponents());
    vehicle = (SimBaseClass*) myit.GetFirst();

    while (vehicle)
    {
        slot = vehicle->GetSlot();
        position_data[slot * 3 + inSlot[slot]].x = vehicle->XPos();
        position_data[slot * 3 + inSlot[slot]].y = vehicle->YPos();
        position_data[slot * 3 + inSlot[slot]].heading = vehicle->Yaw();
        inSlot[slot]++;
        vehicles++;
        vehicle = (SimBaseClass*) myit.GetNext();
    }

    num_vehicles = vehicles;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UnitClass::SetCurrentWaypoint(ushort cw)
{
#ifdef DEBUG
    WayPoint tw = wp_list;
    int       i = cw;

    while (i and tw)
    {
        tw = tw->GetNextWP();
        i--;
    }

    ShiAssert(i == 0);

    if ( not cw)
        ShiAssert( not IsFlight());

#endif
    current_wp = cw;
    //MakeUnitDirty (DIRTY_WAYPOINT, DDP[79].priority);
    MakeUnitDirty(DIRTY_WAYPOINT, SEND_NOW);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UnitClass::SetLastCheck(CampaignTime t)
{
    last_check = t;
#if HOTSPOT_FIX
    CampaignTime max = MaxUpdateTime();
    int randFactor;
    CampaignTime tenSecs = 10 * CampaignSeconds;

    if (max <= tenSecs)
    {
        // somwhere between -max/2 and max/2
        randFactor = (rand() % max) - (max / 2);
    }
    else
    {
        // somewhere between max - 5 and max + 5
        randFactor = (rand() % tenSecs) - (tenSecs / 2);
    }

    update_interval = max + randFactor;
#endif
    // MakeUnitDirty (DIRTY_LAST_CHECK, SEND_WHENEVER);
}

#if HOTSPOT_FIX
CampaignTime UnitClass::UpdateTime() const
{
    return update_interval;
}
//CampaignTime UnitClass::MaxUpdateTime() const {
//}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UnitClass::SetRoster(fourbyte r)
{
    roster = r;
    MakeUnitDirty(DIRTY_ROSTER, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UnitClass::SetUnitFlags(fourbyte f)
{
    if (unit_flags not_eq f)
    {
        unit_flags = f;
        MakeUnitDirty(DIRTY_UNIT_FLAGS, SEND_EVENTUALLY);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UnitClass::SetDestX(GridIndex x)
{
    dest_x = x;
    MakeUnitDirty(DIRTY_DESTINATION, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UnitClass::SetDestY(GridIndex y)
{
    dest_y = y;
    MakeUnitDirty(DIRTY_DESTINATION, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UnitClass::SetCargoId(VU_ID cid)
{
    cargo_id = cid;
    MakeUnitDirty(DIRTY_CARGO_ID, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UnitClass::SetTargetId(VU_ID tid)
{
    target_id = tid;
    MakeUnitDirty(DIRTY_TARGET_ID, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UnitClass::SetMoved(uchar m)
{
    moved = m;
    MakeUnitDirty(DIRTY_MOVED, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UnitClass::SetLosses(uchar l)
{
    losses = l;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UnitClass::SetTactic(uchar t)
{
    tactic = t;
    MakeUnitDirty(DIRTY_TACTIC, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UnitClass::SetNameId(short nid)
{
    name_id = nid;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UnitClass::SetReinforcement(short r)
{
    reinforcement = r;
    MakeUnitDirty(DIRTY_REINFORCEMENT, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UnitClass::SetOdds(short o)
{
    odds = o;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UnitClass::MakeWaypointsDirty(void)
{
    // KCK: This is not a great way of doing this.
    // Basically, it works fine for flights (which don't change waypoints often-
    // in fact, only on player tweaking). However, it would be fatal to do this
    // for Battalions (bandwidth wise). The bitch is, in Force-on-Force TE, we
    // need to. We can either do a TE check here (kinda scary) or set up a new
    // message for ordering battalions, which is what I'm going to do.
    if (IsFlight())
    {
        if (IsLocal())
        {
            //MakeUnitDirty (DIRTY_WP_LIST, DDP[89].priority);
            MakeUnitDirty(DIRTY_WP_LIST, SEND_NOW);
        }
        else
        {
            // Need to send data to the host
            VuSessionEntity *target = (VuSessionEntity*) vuDatabase->Find(OwnerId());
            FalconFlightPlanMessage *msg = new FalconFlightPlanMessage(Id(), target);
            uchar tmp[1024], *ptr;

            ptr = tmp;
            msg->dataBlock.size = EncodeWaypoints(&ptr);
            msg->dataBlock.type = FalconFlightPlanMessage::waypointData;
            msg->dataBlock.data = new uchar[msg->dataBlock.size];
            ShiAssert(msg->dataBlock.size < 1024);
            memcpy(msg->dataBlock.data, tmp, msg->dataBlock.size);
            FalconSendMessage(msg, TRUE);
        }
    }
}

// Adds a waypoint to the end of a unit's list without changing the current waypoint
WayPoint UnitClass::AddUnitWP(GridIndex x, GridIndex y, int alt, int speed,
                              CampaignTime arr, int station, uchar mission)
{
    WayPoint    w, t;

    w = new WayPointClass(x, y, alt, speed, arr, station, mission, 0);

    if ( not wp_list)
    {
        wp_list = w;
        current_wp = 1;
    }
    else
    {
        t = wp_list;

        while (t->GetNextWP() not_eq NULL)
            t = t->GetNextWP();

        t->InsertWP(w);
    }

    MakeWaypointsDirty();
    return w;
}

// Adds a waypoint after waypoint pw in unit's list without changing the current waypoint
WayPoint UnitClass::AddWPAfter(WayPoint pw, GridIndex x, GridIndex y, int alt, int speed,
                               CampaignTime arr, int station, uchar mission)
{
    WayPoint    w;

    w = new WayPointClass(x, y, alt, speed, arr, station, mission, 0);

    if (pw and wp_list)
        pw->InsertWP(w);
    else
    {
        if (wp_list)
            w->InsertWP(wp_list);

        wp_list = w;
    }

    if (wp_list == NULL)
        current_wp = 1;

    MakeWaypointsDirty();
    return w;
}

void UnitClass::DeleteUnitWP(WayPoint w)
{
    WayPoint    t;

    CampEnterCriticalSection();
    t = wp_list;

    if ( not t)
        return;

    if (t == w)
        wp_list = w->GetNextWP();

    w->DeleteWP();
    CampLeaveCriticalSection();
    MakeWaypointsDirty();
}

int UnitClass::EncodeWaypoints(uchar **stream)
{
    uchar *start;
    ushort count = 0;
    WayPointClass *w;

    start = *stream;

    // Count waypoints
    w = wp_list;

    while (w)
    {
        count ++;
        w = w->GetNextWP();
    }

    if (save_log)
    {
        fprintf(save_log, "%d ", count);
        fflush(save_log);
    }

    memcpy(*stream, &count, sizeof(ushort));
    *stream += sizeof(ushort);

    // Save waypoints
    w = wp_list;

    while (w)
    {
        w->Save(stream);
        w = w->GetNextWP();
    }

    return *stream - start;
}

//sfr: changed function prototype
void UnitClass::DecodeWaypoints(VU_BYTE **stream, long *rem)
{
    ushort count;
    WayPointClass *new_list, *lw, *nw, *w;


    if (gCampDataVersion >= 71)
    {
        memcpychk(&count, stream, sizeof(ushort), rem);
    }
    else
    {
        count = 0;
        memcpychk(&count, stream, sizeof(uchar), rem);
    }

    if (load_log)
    {
        fprintf(load_log, "%d ", count);
        fflush(load_log);
    }

    // KCK: Rather than replace our waypoint list,
    // I'm going to copy the new list into our old one.
    // Although this is more time consuming considering
    // the copy, it lowers the possibily of running into
    // deleted waypoints in the UI (I doubt we'll be able
    // to critical section all references to waypoints in
    // time).
    new_list = lw = NULL;

    while (count)
    {
        w = new WayPointClass(stream, rem);

        if ( not lw)
            new_list = lw = w;
        else
            lw->InsertWP(w);

        lw = w;
        count --;
    }

    CampEnterCriticalSection();
    nw = new_list;
    w = wp_list;
    lw = NULL;

    while (w and nw)
    {
        w->CloneWP(nw);
        lw = w;
        w = w->GetNextWP();
        nw = nw->GetNextWP();
    }

    // Delete any extra old ones
    if (w == wp_list)
        wp_list = NULL;

    while (w)
    {
        lw = w;
        w = w->GetNextWP();
        lw->DeleteWP();
    }

    // Add any extra new ones
    if (nw)
    {
        if (nw->GetPrevWP())
            nw->GetPrevWP()->UnlinkNextWP();
        else
            new_list = NULL;

        if (lw)
            lw->SetNextWP(nw);
        else
            wp_list = nw;
    }

    if (new_list)
        DeleteWPList(new_list);

    CampLeaveCriticalSection();

    // Fix up Speeds
    w = wp_list;

    while (w)
    {
        ::SetWPSpeed(w);
        w = w->GetNextWP();
    }

    update_active_flight(this);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UnitClass::MakeUnitDirty(Dirty_Unit bits, Dirtyness score)
{
    if (( not IsLocal()) or (VuState() not_eq VU_MEM_ACTIVE))
    {
        return;
    }

    if ( not IsAggregate() and (score not_eq SEND_RELIABLEANDOOB))
    {
        // increase score for deagged units
        score = static_cast<Dirtyness>(score << 4);
    }

    dirty_unit or_eq bits ;
    MakeDirty(DIRTY_UNIT, score);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UnitClass::WriteDirty(unsigned char **stream)
{
    unsigned char
    *start,
    *ptr;

    start = *stream;
    ptr = *stream;

    // MonoPrint ("Send UC %08x\n", dirty_unit);

    // Encode it up
    *(ushort*) ptr = dirty_unit;
    ptr += sizeof(ushort);

    if (dirty_unit bitand DIRTY_WAYPOINT)
    {
        *(ushort*)ptr = current_wp;
        ptr += sizeof(ushort);
#ifdef DEBUG

        if ( not current_wp)
            ShiAssert( not IsFlight());

#endif
    }

    /* if (dirty_unit bitand DIRTY_LAST_CHECK)
     {
     *(CampaignTime*)ptr = last_check;
     ptr += sizeof (CampaignTime);
     }
     */
    if (dirty_unit bitand DIRTY_ROSTER)
    {
        *(fourbyte*)ptr = roster;
        ptr += sizeof(fourbyte);
    }

    if (dirty_unit bitand DIRTY_UNIT_FLAGS)
    {
        *(fourbyte*)ptr = unit_flags;
        ptr += sizeof(fourbyte);
    }

    if (dirty_unit bitand DIRTY_DESTINATION)
    {
        *(GridIndex*)ptr = dest_x;
        ptr += sizeof(GridIndex);

        *(GridIndex*)ptr = dest_y;
        ptr += sizeof(GridIndex);
    }

    if (dirty_unit bitand DIRTY_CARGO_ID)
    {
        *(VU_ID*)ptr = cargo_id;
        ptr += sizeof(VU_ID);
    }

    if (dirty_unit bitand DIRTY_TARGET_ID)
    {
        *(VU_ID*)ptr = target_id;
        ptr += sizeof(VU_ID);
    }

    if (dirty_unit bitand DIRTY_MOVED)
    {
        *(uchar*)ptr = moved;
        ptr += sizeof(uchar);
    }

    if (dirty_unit bitand DIRTY_TACTIC)
    {
        *(uchar*)ptr = tactic;
        ptr += sizeof(uchar);
    }

    if (dirty_unit bitand DIRTY_REINFORCEMENT)
    {
        *(short*)ptr = reinforcement;
        ptr += sizeof(short);
    }

    if (dirty_unit bitand DIRTY_WP_LIST)
    {
        EncodeWaypoints(&ptr);
    }

    dirty_unit = 0;

    *stream = ptr;

    // MonoPrint ("(%d)\n", *stream - start);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//sfr: changed proto and body, added checks
void UnitClass::ReadDirty(VU_BYTE **stream, long *rem)
{

    bool refresh_required;
    unsigned short bits;

    refresh_required = false;

    memcpychk(&bits, stream, sizeof(unsigned short), rem);

    // MonoPrint ("Recv UC %08x", bits);

    if (bits bitand DIRTY_WAYPOINT)
    {
        memcpychk(&current_wp, stream, sizeof(ushort), rem);
#ifdef DEBUG

        if ( not current_wp)
            ShiAssert( not IsFlight());

#endif
    }

    if (bits bitand DIRTY_ROSTER)
    {
        memcpychk(&roster, stream, sizeof(fourbyte), rem);
        refresh_required = true;
    }

    if (bits bitand DIRTY_UNIT_FLAGS)
    {
        memcpychk(&unit_flags, stream, sizeof(fourbyte), rem);
    }

    if (bits bitand DIRTY_DESTINATION)
    {

        memcpychk(&dest_x, stream, sizeof(GridIndex), rem);
        memcpychk(&dest_y, stream, sizeof(GridIndex), rem);
    }

    if (bits bitand DIRTY_CARGO_ID)
    {
        memcpychk(&cargo_id, stream, sizeof(VU_ID), rem);
    }

    if (bits bitand DIRTY_TARGET_ID)
    {
        memcpychk(&target_id, stream, sizeof(VU_ID), rem);
    }

    if (bits bitand DIRTY_MOVED)
    {
        memcpychk(&moved, stream, sizeof(uchar), rem);
    }

    if (bits bitand DIRTY_TACTIC)
    {
        memcpychk(&tactic, stream, sizeof(uchar), rem);
    }

    if (bits bitand DIRTY_REINFORCEMENT)
    {
        memcpychk(&reinforcement, stream, sizeof(short), rem);
    }

    if (bits bitand DIRTY_WP_LIST)
    {
        DecodeWaypoints(stream, rem);
        //sfr: refresh UI
        refresh_required = true;
    }

    if (refresh_required)
    {
        UI_Refresh();
    }
}

// JPO - extracted to a routine we can call
void UnitClass::CalculateSOJ(VuGridIterator &iter)
{
    Team who = GetTeam();
    CampEntity e;

    // JB SOJ
    sojSource = NULL;
    sojOctant = 0;
    // start my assuming the biggest distance possible.
    sojRangeSq = KM_TO_FT * GetElectronicDetectionRange(Air);

    if (sojRangeSq == 0) return; // no point looking further

    sojRangeSq *= sojRangeSq;
    e = (CampEntity)iter.GetFirst();

    while (e)
    {
        // JPO - use IsAreaJamming - the virtual functions will sort it out
        if (GetRoE(who, e->GetTeam(), ROE_GROUND_FIRE) == ROE_ALLOWED and 
            e->IsAreaJamming())
        {
            float rangesq = DistSqu(XPos(), YPos(), e->XPos(), e->YPos());

            if (rangesq < sojRangeSq)
            {
                sojSource = e;
                sojOctant = OctantTo(0.0F, 0.0F, e->XPos() - XPos(), e->YPos() - YPos());
                sojRangeSq = rangesq;
            }
        }

        e = (CampEntity)iter.GetNext();
    }

    // JB SOJ
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
