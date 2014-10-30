
#ifndef UNIT_H
#define UNIT_H

#include "CampBase.h"

#include "sim/include/initdata.h"
#include "vehicle.h"
#include "Loadout.h"


#include "Campaign.h"
#include "ASearch.h"
#include "vutypes.h"
#include "Cmpglobl.h"
#include "campwp.h"
#include "falcmesg.h"
#include "Tactics.h"
#include "F4Vu.h"
#include "MsgInc/UnitMsg.h"
#include "MsgInc/UnitAssignmentMsg.h"
#include "Cmpclass.h"


enum MissionTypeEnum;

// =========================
// Types and Defines
// =========================

// Transmittable Flags
#define U_DEAD 0x1
#define U_B3 0x2
#define U_ASSIGNED 0x4
#define U_ORDERED 0x8
#define U_NO_PLANNING 0x10 // Don't run planning AI on this unit
#define U_PARENT 0x20
#define U_ENGAGED 0x40
#define U_B1 0x80
#define U_SCRIPTED 0x100 // Mission/Route scripted- Don't run planning AI
#define U_COMMANDO 0x200 // Act like a commando (hit commando sites and kill ourselves after x time)
#define U_MOVING 0x400
#define U_REFUSED 0x800 // A request for transport was refused
#define U_HASECM 0x1000 // This unit has defensive electronic countermeasures
#define U_CARGO 0x2000 // We're being carried by someone else (airborne/marine/carrier air)
#define U_COMBAT 0x4000
#define U_BROKEN 0x8000
#define U_LOSSES 0x10000
#define U_INACTIVE 0x20000 // Ignore this unit for all purposes (generally reinforcements)
#define U_FRAGMENTED 0x40000 // This is a unit fragment (separated from it's origional unit)

// Ground Unit Specific
#define U_TARGETED 0x100000 // Unit's targeting is being done externally
#define U_RETREATING 0x200000
#define U_DETACHED 0x400000
#define U_SUPPORTED 0x800000 // Support is coming to this unit's aide
#define U_TEMP_DEST 0x1000000 // This unit's current destination is not it's final destination

// Air Unit Specific
#define U_FINAL 0x100000 // Package elements finalized and sent, or flight contains actual a/c
#define U_HAS_PILOTS 0x200000 // Campaign has assigned this flight pilots
#define U_DIVERTED 0x400000 // This flight is currently being diverted
#define U_FIRED 0x800000 // This flight has taken a shot
#define U_LOCKED 0x1000000 // Someone is locked on us
#define U_IA_KILL 0x2000000 // Instant Action "Expects" this flight to be killed for the next level to start
#define U_NO_ABORT 0x4000000 // Whatever happens - whatever the loadout - don't ABORT

// 2002-02-13 ADDED BY MN for S.G.'s Identify - S.G. Wrong place. Needs to be in Falcon4.UCD so defined in Vehicle.h which is used by UnitClassDataType and VehicleClassDataType
//#define U_HAS_NCTR 0x10000000
//#define U_HAS_EXACT_RWR 0x20000000

// We use these for broad class types
#define RCLASS_AIR 0
#define RCLASS_GROUND 1
#define RCLASS_AIRDEFENSE 2
#define RCLASS_NAVAL 3

// Types of calculations for certain functions
#define CALC_TOTAL 0
#define CALC_AVERAGE 1
#define CALC_MEAN 2
#define CALC_MAX 3
#define CALC_MIN 4
#define CALC_PERCENTAGE 5

// Flags for what variables to take into account for certain function
#define USE_EXP 0x01
#define USE_VEH_COUNT 0x02
#define IGNORE_BROKEN 0x04

// ============================================
// externals
// ============================================

class TailInsertList;

// =========================
// Unit Class
// =========================

class UnitClass;
typedef UnitClass* Unit;
class UnitDeaggregationData;
class SimVehicleClass;
class DrawablePoint;

// sfr: avoid hotspots adding a random interval to first update time...
#define HOTSPOT_FIX 1

class UnitClass : public CampBaseClass
{
private:
    CampaignTime last_check; ///< Last time we checked this unit
#if HOTSPOT_FIX
    CampaignTime update_interval;   ///< last_check + update_interval for next update
#endif

    /** sfr: a unit is composed of at most VEHICLE_GROUPS_PER_UNIT (16 now)
    * vehicle groups. Each vehicle group can have up to 3 vehicles in it.
    * All vehicles in a group are of the same type.
    * The roster holds how many vehicles each group has. Each group uses 2 bits in roster,
    * meaning all 32 bits are used up for a unit with 16 groups.
    * Example: if roster == 0x0007, then group 0 has 3 vehicles, group 1 has 1 vehicle.
    */
    fourbyte roster;
    fourbyte unit_flags; // Various user flags
    GridIndex dest_x; // Destination
    GridIndex dest_y;
    VU_ID cargo_id; // id of our cargo, or our carrier unit
    VU_ID target_id; // Target we're engaged with (there can be only one (c))
    uchar moved;        // Moves since check
    uchar losses; // How many vehicles we've lost
    uchar tactic; // Current Unit tactic
    ushort current_wp; // Which WP we're heading to
    short name_id; // Element number, used only for naming
    short reinforcement; // What reinforcement level this unit becomes active at
    short odds; // How much shit is coming our way
    int dirty_unit;

public:
    UnitClassDataType *class_data;
    DrawablePoint *draw_pointer; // inserted into draw list when unit aggregated
    WayPoint wp_list;

public:
    // access functions
    CampaignTime GetLastCheck(void)
    {
        return last_check;
    }
    fourbyte GetRoster(void)
    {
        return roster;
    }
    fourbyte GetUnitFlags(void)
    {
        return unit_flags;
    }
    GridIndex GetDestX(void)
    {
        return dest_x;
    }
    GridIndex GetDestY(void)
    {
        return dest_y;
    }
    VU_ID GetCargoId(void)
    {
        return cargo_id;
    }
    VU_ID GetTargetId(void)
    {
        return target_id;
    }
    uchar GetMoved(void)
    {
        return moved;
    }
    uchar GetLosses(void)
    {
        return losses;
    }
    uchar GetTactic(void)
    {
        return tactic;
    }
    ushort GetCurrentWaypoint(void)
    {
        return current_wp;
    }
    short GetNameId(void)
    {
        return name_id;
    }
    short GetReinforcement(void)
    {
        return reinforcement;
    }
    short GetOdds(void)
    {
        return odds;
    }
    UnitClassDataType *GetClassData(void)
    {
        return class_data;
    }

    void SetLastCheck(CampaignTime);
    void SetRoster(fourbyte);
    void SetUnitFlags(fourbyte);
    void SetDestX(GridIndex);
    void SetDestY(GridIndex);
    void SetCargoId(VU_ID);
    void SetTargetId(VU_ID);
    void SetMoved(uchar);
    void SetLosses(uchar);
    void SetTactic(uchar);
    void SetCurrentWaypoint(ushort);
    void SetNameId(short);
    void SetReinforcement(short);
    void SetOdds(short);
    void MakeWaypointsDirty(void);

    // Dirty Functions
    void MakeUnitDirty(Dirty_Unit bits, Dirtyness score);
    void WriteDirty(unsigned char **stream);
    void ReadDirty(VU_BYTE **strptr, long *rem);

    // constructors and serial functions
    UnitClass(ushort type, VU_ID_NUMBER id);
    UnitClass(VU_BYTE **stream, long *rem);
    virtual ~UnitClass();
    virtual int SaveSize();
    virtual int Save(VU_BYTE **stream);

    // event Handlers
    virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);

    // Required pure virtuals handled by UnitClass
    virtual void SendDeaggregateData(VuTargetEntity *);
    virtual int RecordCurrentState(FalconSessionEntity*, int);
    virtual int Deaggregate(FalconSessionEntity* session);
    virtual int Reaggregate(FalconSessionEntity* session);
    virtual int TransferOwnership(FalconSessionEntity* session);
    virtual int Wake(void);
    virtual int Sleep(void);
    virtual void InsertInSimLists(float cameraX, float cameraY);
    virtual void RemoveFromSimLists(void);
    virtual void DeaggregateFromData(uchar* data,  long size);
    virtual void ReaggregateFromData(VU_BYTE* data, long size);
    virtual void TransferOwnershipFromData(VU_BYTE* data, long size);
    virtual int ResetPlayerStatus(void);
    virtual int ApplyDamage(FalconCampWeaponsFire *cwfm, uchar);
    virtual int ApplyDamage(DamType d, int *str, int where, short flags);
    virtual int DecodeDamageData(uchar *data, Unit shooter, FalconDeathMessage *dtm);
    virtual int CollectWeapons(uchar* dam, MoveType m, short w[], uchar wc[], int dist);
    virtual uchar* GetDamageModifiers(void);
    virtual _TCHAR* GetName(_TCHAR* buffer, int size, int object);
    virtual _TCHAR* GetFullName(_TCHAR* buffer, int size, int object);
    virtual _TCHAR* GetDivisionName(_TCHAR* buffer, int size, int object);
    virtual int GetHitChance(int mt, int range);
    virtual int GetAproxHitChance(int mt, int range);
    virtual int GetCombatStrength(int mt, int range);
    virtual int GetAproxCombatStrength(int mt, int range);
    // 2002-03-08 MODIFIED BY S.G. Need to pass it a target sometime so default to NULL for most cases
    virtual int GetWeaponRange(int mt, FalconEntity *target = NULL);
    virtual int GetAproxWeaponRange(int mt);
    virtual int GetDetectionRange(int mt); // Takes into account emitter status
    virtual int GetElectronicDetectionRange(int mt); // Max Electronic detection range, even if turned off
    virtual int CanDetect(FalconEntity* ent); // Nonzero if this entity can see ent
    virtual void GetComponentLocation(GridIndex* x, GridIndex* y, int component);
    virtual int GetComponentAltitude(int component);
    virtual float GetRCSFactor(void);
    virtual float GetIRFactor(void);

    // These are only really relevant for sam/airdefense/radar entities
    virtual int GetNumberOfArcs(void);
    virtual float GetArcRatio(int anum);
    virtual float GetArcRange(int anum);
    virtual void GetArcAngle(int anum, float* a1, float *a2);
    virtual int GetRadarType(void);

    // Addition Virtual functions required by all derived classes
    virtual int CanShootWeapon(int)
    {
        return TRUE;
    };
    virtual int GetDeaggregationPoint(int, CampEntity*)
    {
        return 0;
    }
    virtual UnitDeaggregationData* GetUnitDeaggregationData(void)
    {
        return NULL;
    }
    virtual int ShouldDeaggregate(void)
    {
        return TRUE;
    }
    virtual void ClearDeaggregationData(void) {}
    virtual int Reaction(CampEntity, int, float)
    {
        return 0;
    }
    virtual int MoveUnit(CampaignTime)
    {
        return 0;
    }
    virtual int DoCombat(void)
    {
        return 0;
    }
    virtual int ChooseTactic(void)
    {
        return 0;
    }
    virtual int CheckTactic(int)
    {
        return 1;
    }
    virtual int Father() const
    {
        return 0;
    }
    virtual int Real(void)
    {
        return 0;
    }
    virtual float AdjustForSupply(void)
    {
        return 1.0F;
    }
    virtual int GetUnitSpeed() const
    {
        return GetMaxSpeed();
    }
    virtual int DetectOnMove(void)
    {
        return -1;
    }
    virtual int ChooseTarget(void)
    {
        return -1;
    }
#if HOTSPOT_FIX
    /** returns interval for next update. It adds a random factor to max update time. */
    CampaignTime UpdateTime() const;
    /** returns max interval for this unit. */
    virtual CampaignTime MaxUpdateTime() const = 0;
#else
    virtual CampaignTime UpdateTime(void)
    {
        return CampaignDay;
    }
#endif
    virtual CampaignTime CombatTime(void)
    {
        return CampaignDay;
    }
    virtual int GetUnitSupplyNeed(int)
    {
        return 0;
    }
    virtual int GetUnitFuelNeed(int)
    {
        return 0;
    }
    virtual void SupplyUnit(int, int) {}
    virtual int GetVehicleDeagData(SimInitDataClass*, int)
    {
        return 0;
    }

    // Core functions
    void Setup(uchar stype, uchar sptype, Control who, Unit Parent);
    void SendUnitMessage(VU_ID id, short msg, short d1, short d2, short d3);
    void BroadcastUnitMessage(VU_ID id, short msg, short d1, short d2, short d3);
    int ChangeUnitLocation(CampaignHeading h);
    int MoraleCheck(int shot, int lost);
    virtual int IsUnit(void)
    {
        return TRUE;
    }

    // Unit flags
    void SetDead(int p);
    void SetAssigned(int p);
    void SetOrdered(int p);
    void SetDontPlan(int p);
    void SetParent(int p);
    void SetEngaged(int p);
    void SetScripted(int p);
    void SetCommando(int c);
    void SetMoving(int p);
    void SetRefused(int r);
    void SetHasECM(int e);
    void SetCargo(int c);
    void SetCombat(int p);
    void SetBroken(int p);
    void SetAborted(int p);
    void SetLosses(int p);
    void SetInactive(int i);
    void SetFragment(int f);
    void SetTargeted(int p);
    void SetRetreating(int p);
    void SetDetached(int p);
    void SetSupported(int s);
    void SetTempDest(int t);
    void SetFinal(int p);
    void SetPilots(int f);
    void SetDiverted(int d);
    void SetFired(int f);
    void SetLocked(int l);
    void SetIAKill(int f);
    void SetNoAbort(int f);
    virtual int IsDead() const
    {
        return (int)unit_flags bitand U_DEAD;
    }
    //int Dead() const { return IsDead(); }
    int Assigned() const
    {
        return (int)unit_flags bitand U_ASSIGNED;
    }
    int Ordered() const
    {
        return (int)unit_flags bitand U_ORDERED;
    }
    int DontPlan() const
    {
        return (int)unit_flags bitand U_NO_PLANNING;
    }
    int Parent() const
    {
        return (int)unit_flags bitand U_PARENT;
    }
    int Engaged() const
    {
        return (int)unit_flags bitand U_ENGAGED;
    }
    int Scripted()  const
    {
        return (int)unit_flags bitand U_SCRIPTED;
    }
    int Commando() const
    {
        return (int)unit_flags bitand U_COMMANDO;
    }
    int Moving() const
    {
        return (int)unit_flags bitand U_MOVING;
    }
    int Refused() const
    {
        return (int)unit_flags bitand U_REFUSED;
    }
    int Cargo() const
    {
        return (int)unit_flags bitand U_CARGO;
    }
    int Combat() const
    {
        return (int)unit_flags bitand U_COMBAT;
    }
    int Broken() const
    {
        return (int)unit_flags bitand U_BROKEN;
    }
    int Aborted() const
    {
        return (int)unit_flags bitand U_BROKEN;
    }
    int Losses() const
    {
        return (int)unit_flags bitand U_LOSSES;
    }
    int Inactive() const
    {
        return (int)unit_flags bitand U_INACTIVE;
    }
    int Fragment() const
    {
        return (int)unit_flags bitand U_FRAGMENTED;
    }
    int Targeted() const
    {
        return (int)unit_flags bitand U_TARGETED;
    }
    int Retreating() const
    {
        return (int)unit_flags bitand U_RETREATING;
    }
    int Detached() const
    {
        return (int)unit_flags bitand U_DETACHED;
    }
    int Supported() const
    {
        return (int)unit_flags bitand U_SUPPORTED;
    }
    int TempDest() const
    {
        return (int)unit_flags bitand U_TEMP_DEST;
    }
    int Final() const
    {
        return (int)unit_flags bitand U_FINAL;
    }
    int HasPilots(void)
    {
        return (int)unit_flags bitand U_HAS_PILOTS;
    }
    int Diverted(void)
    {
        return (int)unit_flags bitand U_DIVERTED;
    }
    int Fired(void)
    {
        return (int)unit_flags bitand U_FIRED;
    }
    int Locked(void)
    {
        return (int)unit_flags bitand U_LOCKED;
    }
    int IAKill(void)
    {
        return (int)unit_flags bitand U_IA_KILL;
    }
    int NoAbort(void)
    {
        return (int)unit_flags bitand U_NO_ABORT;
    }

    // Entity information
    UnitClassDataType* GetUnitClassData(void);
    char* GetUnitClassName(void);
    void SetUnitAltitude(int alt)
    {
        SetPosition(XPos(), YPos(), -1.0F * (float)alt);
        MakeCampBaseDirty(DIRTY_ALTITUDE, DDP[181].priority);
    }
    //void SetUnitAltitude (int alt) { SetPosition(XPos(),YPos(),-1.0F * (float)alt); MakeCampBaseDirty (DIRTY_ALTITUDE, SEND_SOON); }
    int GetUnitAltitude(void)
    {
        return FloatToInt32(ZPos() * -1.0F);
    }
    virtual void SimSetLocation(float x, float y, float z)
    {
        SetPosition(x, y, z);
        MakeCampBaseDirty(DIRTY_POSITION, DDP[182].priority);
        MakeCampBaseDirty(DIRTY_ALTITUDE, DDP[183].priority);
    }
    //virtual void SimSetLocation (float x, float y, float z) { SetPosition(x,y,z); MakeCampBaseDirty (DIRTY_POSITION, SEND_SOON); MakeCampBaseDirty (DIRTY_ALTITUDE, SEND_SOON); }
    virtual void SimSetOrientation(float, float, float) {}
    virtual void GetRealPosition(float*, float*, float*) {}
    virtual int GetBestVehicleWeapon(int, uchar*, MoveType, int, int*);
    virtual int GetVehicleHitChance(int slot, MoveType mt, int range, int hitflags);
    virtual int GetVehicleCombatStrength(int slot, MoveType mt, int range);
    virtual int GetVehicleRange(int slot, int mt, FalconEntity *target = NULL);  // 2002-03-08 MODIFIED BY S.G. Need to pass it a target sometime so default to NULL for most cases
    virtual int GetUnitWeaponId(int hp, int slot);
    virtual int GetUnitWeaponCount(int hp, int slot);

    // Unit_data information
    void SetUnitDestination(GridIndex x, GridIndex y)
    {
        dest_x = (GridIndex)(x + 1);
        dest_y = (GridIndex)(y + 1);
    }
    /** sets the number of vehicles in a given group of the unit. */
    void SetNumVehicles(int vg, int n)
    {
        SetRoster((roster bitand compl (3 << (vg * 2))) bitor ((n bitand 0x03) << (vg * 2)));
    }
    void SetTarget(FalconEntity *e)
    {
        target_id = (e) ? e->Id() : FalconNullId;
    }
    void SetUnitMoved(uchar m)
    {
        moved = m;
    }
    void SetUnitTactic(uchar t)
    {
        tactic = t;
    }
    void SetUnitReinforcementLevel(short r)
    {
        reinforcement = r;
    }
    void GetUnitDestination(GridIndex* x, GridIndex* y);
    /** gets number of vehicles in a given vehicle group */
    int GetNumVehicles(int vg)
    {
        return (int)((roster >> (vg * 2)) bitand 0x03);
    }
    FalconEntity* GetTarget(void)
    {
        return (FalconEntity*) vuDatabase->Find(target_id);
    }
    VU_ID GetTargetID(void)
    {
        return target_id;
    }
    SimBaseClass* GetSimTarget(void);
    CampBaseClass* GetCampTarget(void);
    CampEntity GetCargo(void);
    CampEntity GetTransport(void);
    VU_ID GetCargoID(void);
    VU_ID GetTransportID(void);
    int GetUnitMoved() const
    {
        return moved;
    }
    int GetUnitTactic() const
    {
        return tactic;
    }
    int GetUnitReinforcementLevel(void)
    {
        return reinforcement;
    }
    void AssignUnit(VU_ID mgr, VU_ID po, VU_ID so, VU_ID ao, int orders);
    void SetUnitNameID(short id)
    {
        name_id = id;
    }
    int SetUnitSType(char t);
    int SetUnitSPType(char t);
    int GetUnitNameID(void)
    {
        return name_id;
    }

    // Attribute data
    /** returns the id of vehicles in this group */
    VehicleID GetVehicleID(int vg);
    int GetTotalVehicles(void);
    int GetFullstrengthVehicles(void);
    int GetFullstrengthVehicles(int slot);
    int GetMaxSpeed() const;
    int GetCruiseSpeed() const;
    int GetCombatSpeed() const;
    int GetUnitEndurance(void);
    int GetUnitRange(void);
    int GetRClass(void);

    // Support routines
    CampaignTime GetUnitReassesTime(void);
    int CountUnitElements(void);
    Unit GetRandomElement(void);
    void ResetMoves(void);
    void ResetLocations(GridIndex x, GridIndex y);
    void ResetDestinations(GridIndex x, GridIndex y);
    void ResetFlags(void);
    void SortElementsByDistance(GridIndex x, GridIndex y);
    int FirstSP(void);
    Unit FindPrevUnit(short *type);
    void SaveUnits(int FHandle, int flags);
    void BuildElements(void);
    int ChangeVehicles(int a);
    int GetFormationCruiseSpeed() const;
    void KillUnit(void);
    int NoMission(void);
    int AtDestination(void);
    int GetUnitFormation() const;
    int GetUnitRoleScore(int role, int calcType, int use_to_calc);
    float GetUnitMovementCost(GridIndex x, GridIndex y, CampaignHeading h);
    int GetUnitObjectivePath(Path p, Objective o, Objective t);
    int GetUnitGridPath(Path p, GridIndex x, GridIndex y, GridIndex xx, GridIndex yy);
    void LoadUnit(Unit cargo);
    void UnloadUnit(void);
    CampaignTime GetUnitSupplyTime(void);

    // Waypoint routines
    WayPoint AddUnitWP(GridIndex x, GridIndex y, int alt, int speed, CampaignTime arr, int station, uchar mission);
    WayPoint AddWPAfter(WayPoint pw, GridIndex x, GridIndex y, int alt, int speed, CampaignTime arr, int station, uchar mission);
    void DeleteUnitWP(WayPoint w);
    int EncodeWaypoints(uchar **stream);
    //sfr: changed prototype
    //void DecodeWaypoints (uchar **stream);
    void DecodeWaypoints(VU_BYTE **stream, long *rem);
    WayPoint GetFirstUnitWP()
    {
        return wp_list;
    }
    WayPoint GetCurrentUnitWP() const;
    WayPoint GetUnitMissionWP();
    void FinishUnitWP(void);
    void DisposeWayPoints(void);
    void CheckBroken(void);
    void SetCurrentUnitWP(WayPoint w);
    void AdjustWayPoints(void);

    // Virtual Functions (These are empty except for those derived classes they belong to)
    // AirUnit virtuals
    // None

    // Flight virtuals
    virtual void SetUnitLastMove(CampaignTime) {}
    virtual void SetCombatTime(CampaignTime) {}
    virtual void SetBurntFuel(long) {}
    virtual void SetUnitMission(uchar) {}
    virtual void SetUnitRole(uchar) {}
    virtual void SetUnitPriority(int) {}
    virtual void SetUnitMissionID(int) {}
    virtual void SetUnitMissionTarget(int) {}
    virtual void SetUnitTOT(CampaignTime) {}
    virtual void SetUnitSquadron(VU_ID) {}
    virtual void SetUnitAirbase(VU_ID) {}
    virtual void SetLoadout(LoadoutStruct*, int)
    {
        ShiWarning("Shouldn't be here");
    }
    virtual int GetNumberOfLoadouts(void)
    {
        return 0;
    }
    virtual CampaignTime GetMoveTime(void)
    {
        return TheCampaign.CurrentTime - last_check;
    }
    virtual CampaignTime GetCombatTime(void)
    {
        return 0;
    }
    virtual VU_ID GetAirTargetID(void)
    {
        return FalconNullId;
    }
    virtual FalconEntity* GetAirTarget(void)
    {
        return NULL;
    }
    virtual int GetBurntFuel(void)
    {
        return 0;
    }
    virtual MissionTypeEnum GetUnitMission(void)
    {
        return (MissionTypeEnum)0;
    }
    virtual int GetUnitNormalRole(void)
    {
        return 0;
    }
    virtual int GetUnitCurrentRole() const
    {
        return 0;
    }
    virtual int GetUnitPriority(void)
    {
        return 0;
    }
    virtual CampEntity GetUnitMissionTarget(void)
    {
        return NULL;
    }
    virtual VU_ID GetUnitMissionTargetID(void)
    {
        return FalconNullId;
    }
    virtual int GetUnitMissionID(void)
    {
        return 0;
    }
    virtual CampaignTime GetUnitTOT(void)
    {
        return 0;
    }
    virtual Unit GetUnitSquadron(void)
    {
        return NULL;
    }
    virtual VU_ID GetUnitSquadronID(void)
    {
        return FalconNullId;
    }
    virtual CampEntity GetUnitAirbase(void)
    {
        return NULL;
    }
    virtual VU_ID GetUnitAirbaseID(void)
    {
        return FalconNullId;
    }
    virtual int LoadWeapons(void*, uchar*, MoveType, int, int, int)
    {
        return 0;
    }
    virtual int DumpWeapons(void)
    {
        return 0;
    }
    virtual CampaignTime ETA(void)
    {
        return 0;
    }
    virtual F4PFList GetKnownEmitters(void)
    {
        return NULL;
    }
    virtual int BuildMission(MissionRequestClass*)
    {
        return 0;
    }
    virtual void IncrementTime(CampaignTime)  {}
    virtual void UseFuel(long) {}

    // Squadron virtuals
    virtual void SetUnitSpecialty(int) {}
    virtual void SetUnitSupply(int) {}
    virtual void SetUnitMorale(int) {}
    virtual void SetSquadronFuel(long) {}
    virtual void SetUnitStores(int, uchar) {}
    virtual void SetLastResupply(int) {}
    virtual void SetLastResupplyTime(CampaignTime) {}
    virtual int GetUnitSpecialty(void)
    {
        return 0;
    }
    virtual int GetUnitSupply(void)
    {
        return 0;
    }
    virtual int GetUnitMorale(void)
    {
        return 0;
    }
    virtual long GetSquadronFuel(void)
    {
        return 0;
    }
    virtual uchar GetUnitStores(int)
    {
        return 0;
    }
    virtual CampaignTime GetLastResupplyTime(void)
    {
        return TheCampaign.CurrentTime;
    }
    virtual int GetLastResupply(void)
    {
        return 0;
    }

    // Package virtuals
    virtual int BuildPackage(MissionRequest, F4PFList)
    {
        return 0;
    }
    virtual void HandleRequestReceipt(int, int, VU_ID) {}
    virtual void SetUnitAssemblyPoint(int, GridIndex, GridIndex) {}
    virtual void GetUnitAssemblyPoint(int, GridIndex*, GridIndex*) {}

    // Ground Unit virtuals
    virtual void SetUnitPrimaryObj(VU_ID) {}
    virtual void SetUnitSecondaryObj(VU_ID) {}
    virtual void SetUnitObjective(VU_ID) {}
    virtual void SetUnitOrders(int) {}
    virtual void SetUnitOrders(int, VU_ID) {}
    virtual void SetUnitFatigue(int)  {}
    // virtual void SetUnitElement (int e) {}
    virtual void SetUnitMode(int) {}
    virtual void SetUnitPosition(int) {}
    virtual void SetUnitDivision(int) {}
    virtual void SetUnitHeading(int) {}
    virtual Objective GetUnitPrimaryObj(void)
    {
        return NULL;
    }
    virtual Objective GetUnitSecondaryObj(void)
    {
        return NULL;
    }
    virtual Objective GetUnitObjective(void)
    {
        return NULL;
    }
    virtual VU_ID GetUnitPrimaryObjID(void)
    {
        return FalconNullId;
    }
    virtual VU_ID GetUnitSecondaryObjID(void)
    {
        return FalconNullId;
    }
    virtual VU_ID GetUnitObjectiveID(void)
    {
        return FalconNullId;
    }
    virtual int GetUnitOrders(void)
    {
        return 0;
    }
    virtual int GetUnitFatigue(void)
    {
        return 0;
    }
    virtual int GetUnitElement(void)
    {
        return 0;
    }
    virtual int GetUnitMode(void)
    {
        return 0;
    }
    virtual int GetUnitPosition(void)
    {
        return 0;
    }
    virtual int GetUnitDivision(void)
    {
        return 0;
    }
    virtual int GetUnitHeading(void)
    {
        return Here;
    }
    virtual void SetUnitNextMove(void) {}
    virtual void ClearUnitPath(void) {}
    virtual int GetNextMoveDirection(void)
    {
        return Here;
    }
    virtual void SetUnitCurrentDestination(GridIndex, GridIndex) {}
    virtual void GetUnitCurrentDestination(GridIndex*, GridIndex*) {}
    virtual MoveType GetObjMovementType(Objective, int)
    {
        return CampBaseClass::GetMovementType();
    }
    virtual int CheckForSurrender(void)
    {
        return 1;
    }
    virtual int BuildMission(void)
    {
        return 0;
    }
    virtual int RallyUnit(int)
    {
        return 0;
    }

    // Battalion virtuals
    virtual Unit GetUnitParent() const
    {
        return NULL;
    }
    virtual VU_ID GetUnitParentID(void)
    {
        return FalconNullId;
    }
    virtual void SetUnitParent(Unit) {}
#ifdef USE_FLANKS
    virtual void GetLeftFlank(GridIndex *x, GridIndex *y)
    {
        GetLocation(x, y);
    }
    virtual void GetRightFlank(GridIndex *x, GridIndex *y)
    {
        GetLocation(x, y);
    }
#endif

    // Brigade virtuals
    virtual Unit GetFirstUnitElement() const
    {
        return NULL;
    }
    virtual Unit GetNextUnitElement() const
    {
        return NULL;
    }
    virtual Unit GetUnitElement(int)
    {
        return NULL;
    }
    virtual Unit GetUnitElementByID(int)
    {
        return NULL;
    }
    virtual Unit GetPrevUnitElement(Unit)
    {
        return NULL;
    }
    virtual void AddUnitChild(Unit) {}
    virtual void DisposeChildren(void) {}
    virtual void RemoveChild(VU_ID) {}
    virtual void ReorganizeUnit(void) {}
    virtual int UpdateParentStatistics(void)
    {
        return 0;
    }

    void CalculateSOJ(VuGridIterator &iter);
    // Naval Unit virtuals
    // None

protected:
    // JB SOJ
    CampEntity sojSource;
    int sojOctant;
    float sojRangeSq;
};

/* sfr: not used
class UnitDriver : public VuMaster
 {
 private:
 public:
 // No Data
 public:
 UnitDriver(VuEntity *entity);
 virtual ~UnitDriver();

 //virtual void AutoExec(VU_TIME timestamp);
 virtual VU_BOOL ExecModel(VU_TIME timestamp);
 };*/

// ============================================================
// Deaggregated data class
// ============================================================

class UnitPosition
{
public:
    float x, y, heading;
};

// This class is used to store unit positions while it's in an aggregate state
class UnitDeaggregationData
{

#ifdef USE_SH_POOLS
public:
    // Overload new/delete to use a SmartHeap fixed size pool
    void *operator new(size_t size)
    {
        ShiAssert(size == sizeof(UnitDeaggregationData));
        return MemAllocFS(pool);
    };
    void operator delete(void *mem)
    {
        if (mem) MemFreeFS(mem);
    };
    static void InitializeStorage()
    {
        pool = MemPoolInitFS(sizeof(UnitDeaggregationData), 50, 0);
    };
    static void ReleaseStorage()
    {
        MemPoolFree(pool);
    };
    static MEM_POOL pool;
#endif

public:
    short num_vehicles;
    UnitPosition position_data[VEHICLE_GROUPS_PER_UNIT * 3];

public:
    UnitDeaggregationData(void);
    ~UnitDeaggregationData();

    void StoreDeaggregationData(Unit theUnit);
};

// ============================================================
// Unit manipulation routines
// ============================================================

extern void SaveUnits(char* FileName);

extern int LoadUnits(char* FileName);

extern Unit GetFirstUnit(F4LIt l);

extern Unit GetNextUnit(F4LIt l);

extern Unit LoadAUnit(int Num, int FHandle, Unit parent);

extern DamageDataType GetDamageType(Unit u);

extern Unit ConvertUnit(Unit u, int domain, int type, int stype, int sptype);

extern int GetUnitRole(Unit u);

extern char* GetSizeName(int domain, int type, char *buffer);

extern char* GetDivisionName(int div, char *buffer, int size, int object);

extern int FindUnitNameID(Unit u);

extern Unit NewUnit(int domain, int type, int stype, int sptype, Unit parent);

//sfr: changed proto
//extern Unit NewUnit (short tid, VU_BYTE **stream);
extern Unit NewUnit(short tid, VU_BYTE **stream, long *rem);

extern float GetOdds(Unit us, CampEntity them, int range);

extern float GetRange(Unit us, CampEntity them);

extern int EncodeUnitData(VU_BYTE **stream, FalconSessionEntity *owner);

//sfr: added rem
extern int DecodeUnitData(VU_BYTE **stream, long *rem, FalconSessionEntity *owner);

#endif
