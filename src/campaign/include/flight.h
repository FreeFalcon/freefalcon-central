
#ifndef FLIGHT_H
#define FLIGHT_H

#include "package.h"
#include "Find.h"
#include "airunit.h"
#include "Pilot.h"
#include "loadout.h"
#include "SIM/INCLUDE/initdata.h"


class PackageClass;
enum MissionTypeEnum;

// ========================================
// Flight specific mission evaluation flags
// ========================================

#define FEVAL_MISSION_STARTED 0x01 // Mission is in it's 'critical section'
#define FEVAL_GOT_TO_TARGET 0x02 // We've arrived at the target
#define FEVAL_ON_STATION 0x04 // We're in our VOL timewise (not spacewise)
#define FEVAL_START_COLD 0x10 // not really evaluation flag, start mission from cold, sneaked in so its transfered.... JPO

// 2002-02-12 added by MN
#define FLIGHT_ON_STATION 0x20 // this is used for player flights. Only get 
// AWACS BVR threat warnings when checked in

#define AIRCRAFT_NOT_ASSIGNED 0 // planeStats values
#define AIRCRAFT_MISSING 1
#define AIRCRAFT_DEAD 2
#define AIRCRAFT_RTB 3
#define AIRCRAFT_AVAILABLE 4

// =========================
// Flight Class
// =========================

class FlightClass : public AirUnitClass
{
#ifdef USE_SH_POOLS
public:
    // Overload new/delete to use a SmartHeap fixed size pool
    void *operator new(size_t size)
    {
        ShiAssert(size == sizeof(FlightClass));
        return MemAllocFS(pool);
    };
    void operator delete(void *mem)
    {
        if (mem) MemFreeFS(mem);
    };
    static void InitializeStorage()
    {
        pool = MemPoolInitFS(sizeof(FlightClass), 200, 0);
    };
    static void ReleaseStorage()
    {
        MemPoolFree(pool);
    };
    static MEM_POOL pool;
#endif

private:
    int dirty_flight; // Which elements are dirty

    long fuel_burnt; // Amount of fuel used since takeoff
    CampaignTime last_move; // Time we moved last
    CampaignTime last_combat; // Last time this entity fired its weapons
    CampaignTime time_on_target; // Time on target
    CampaignTime mission_over_time; // Time off station/target
    CampaignTime last_enemy_lock_time; // Last time an enemy locked us
    VU_ID mission_target; // Our target, or flight we're attached to
    VU_ID assigned_target; // Target we're supposed to IMMEDIATELY attack
    VU_ID enemy_locker; // ID of enemy flight which is locking us
    WayPointClass override_wp; // Divert waypoint or other overriding waypoint
    LoadoutStruct *loadout; // A custom loadout from the Payload window
    uchar loadouts; // Number of loadouts we have recorded (1 or # of ac)
    uchar mission;      // Unit's mission
    uchar old_mission; // Previous mission, if we've been diverted
    uchar last_direction; // Direction of last move
    uchar priority; // Mission priority
    uchar mission_id; // Our mission id
    uchar eval_flags; // Mission evaluation flags
    uchar mission_context; // Our mission context
    VU_ID package; // Our parent package
    VU_ID squadron; // Our parent squadron
    VU_ID requester; // ID of entity requesting our mission

public:
    uchar slots[PILOTS_PER_FLIGHT]; // Which vehicle slots this flight is using
    uchar pilots[PILOTS_PER_FLIGHT]; // Which squadron pilots we're using
    uchar plane_stats[PILOTS_PER_FLIGHT]; // The status of this aircraft
    uchar player_slots[PILOTS_PER_FLIGHT];// Which player pilot is in this slot
    uchar last_player_slot; // Slot # of last pilot in this slot
    uchar callsign_id; // Index into callsign table
    uchar callsign_num;
    // Locals
    float last_collision_x; // Last point AWACS vectored us to
    float last_collision_y;

    uchar tacan_channel; // Support for tankers
    uchar tacan_band; // Support for tankers

    // Access Functions
    CampaignTime GetLastMove(void)
    {
        return last_move;
    }
    CampaignTime GetLastCombat(void)
    {
        return last_combat;
    }
    CampaignTime GetTimeOnTarget(void)
    {
        return time_on_target;
    }
    CampaignTime GetMissionOverTime(void)
    {
        return mission_over_time;
    }
    CampaignTime GetLastEnemyLockTime(void)
    {
        return last_enemy_lock_time;
    }
    VU_ID GetAssignedTarget(void)
    {
        return assigned_target;
    }
    VU_ID GetEnemyLocker(void)
    {
        return enemy_locker;
    }
    WayPoint GetOverrideWP(void);
    LoadoutStruct *GetLoadout(void)
    {
        return loadout;
    }
    uchar GetLoadouts(void)
    {
        return loadouts;
    }
    uchar GetOriginalMission(void)
    {
        return old_mission;
    }
    uchar GetLastDirection(void)
    {
        return last_direction;
    }
    uchar GetEvalFlags(void)
    {
        return eval_flags;
    }
    uchar GetMissionContext(void)
    {
        return mission_context;
    }
    VU_ID GetRequesterID(void)
    {
        return requester;
    }

    // virtual int CombatClass (void) { return SimACDefTable[((Falcon4EntityClassType*)EntityType())->vehicleDataIndex].combatClass; } // 2002-02-25 ADDED BY S.G. FlightClass needs to have a combat class like aircrafts.
    virtual int CombatClass(void);  // 2002-03-04 MODIFIED BY S.G. Moved inside Flight.cpp
    void SetLastDirection(uchar);
    void SetPackage(VU_ID);
    void SetEvalFlag(uchar, int reset = 0);  // 2002-02-19 MODIFIED BY S.G. Added the ability to reset the flag to whatever is passed without ORing the bits toghether
    void ClearEvalFlag(uchar);
    void SetAssignedTarget(VU_ID targetId);
    void ClearAssignedTarget(void);
    void SetOverrideWP(WayPoint w, bool ReqHelpHint = false);

    void MakeStoresDirty(void);

    // Dirty Data
    void ClearDirty(void);
    void MakeFlightDirty(Dirty_Flight bits, Dirtyness score);
    void WriteDirty(unsigned char **stream);
    //sfr: added rem
    void ReadDirty(VU_BYTE **stream, long *rem);

    // Other Functions
    FlightClass(ushort type, Unit parent, Unit squadron);
    FlightClass(VU_BYTE **stream, long *rem);
    virtual ~FlightClass();
    virtual int SaveSize(void);
    virtual int Save(VU_BYTE **stream);
    virtual VU_ERRCODE RemovalCallback();

    // event Handlers
    virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);

    // virtuals handled by flight.h
    virtual int GetDeaggregationPoint(int slot, CampEntity *ent);
    virtual int ShouldDeaggregate(void);
    virtual int Reaction(CampEntity what, int zone, float range);
    virtual int MoveUnit(CampaignTime time);
    virtual int DoCombat(void);
    virtual int ChooseTactic(void);
    virtual int CheckTactic(int tid);
    virtual int DetectOnMove(void);
    virtual int ChooseTarget(void);
    virtual int Real(void)
    {
        return 1;
    }
    virtual int CollectWeapons(uchar* dam, MoveType m, short w[], uchar wc[], int dist);
#if HOTSPOT_FIX
    virtual CampaignTime MaxUpdateTime() const
    {
        return FLIGHT_MOVE_CHECK_INTERVAL * CampaignSeconds;
    }
#else
    virtual CampaignTime UpdateTime(void)
    {
        return FLIGHT_MOVE_CHECK_INTERVAL * CampaignSeconds;
    }
#endif
    virtual CampaignTime CombatTime(void)
    {
        return FLIGHT_COMBAT_CHECK_INTERVAL * CampaignSeconds;
    }
    virtual int IsFlight() const
    {
        return TRUE;
    }
    virtual int GetDetectionRange(int mt);
    virtual int GetRadarMode(void)
    {
        return FEC_RADAR_SEARCH_100;
    }
    virtual int IsSPJamming(void);
    virtual int IsAreaJamming(void);
    virtual int HasSPJamming(void);
    virtual int HasAreaJamming(void);
    virtual int GetVehicleDeagData(SimInitDataClass *simdata, int remote);

    virtual void SetUnitLastMove(CampaignTime t)
    {
        last_move = t;
    }
    virtual void SetCombatTime(CampaignTime t)
    {
        last_combat = t;
    }
    virtual void SetBurntFuel(long fuel)
    {
        fuel_burnt = fuel;
    }
    virtual void SetUnitMission(uchar mis);
    virtual void SetUnitPriority(int p)
    {
        priority = (uchar)p;
    }
    virtual void SetUnitMissionID(int id)
    {
        mission_id = (uchar)id;
    }
    virtual void SetUnitMissionTarget(VU_ID id)
    {
        mission_target = id;
    }
    virtual void SetUnitTOT(CampaignTime tot)
    {
        time_on_target = tot;
    }
    virtual void SetUnitSquadron(VU_ID ID)
    {
        squadron = ID;
    }
    // virtual void SetUnitTakeoffSlot (int ts) { takeoff_slot = ts; }
    virtual void SimSetLocation(float x, float y, float z);
    virtual void GetRealPosition(float *x, float *y, float *z);
    virtual void SimSetOrientation(float yaw, float pitch, float roll);
    virtual void SetLoadout(LoadoutStruct *loadout, int count);
    virtual void RemoveLoadout(void);
    virtual LoadoutStruct *GetLoadout(int ac);
    virtual int GetNumberOfLoadouts(void)
    {
        return loadouts;
    }

    virtual CampaignTime GetMoveTime(void);
    virtual CampaignTime GetCombatTime(void)
    {
        return (TheCampaign.CurrentTime > last_combat) ? TheCampaign.CurrentTime - last_combat : 0;
    }
    virtual int GetBurntFuel(void)
    {
        return fuel_burnt;
    }
    virtual MissionTypeEnum GetUnitMission(void)
    {
        return (MissionTypeEnum)mission;
    }
    virtual int GetUnitCurrentRole() const;
    virtual int GetUnitPriority(void)
    {
        return (int)priority;
    }
    virtual int GetUnitWeaponId(int hp, int slot);
    virtual int GetUnitWeaponCount(int hp, int slot);
    virtual int GetUnitMissionID(void)
    {
        return (int)mission_id;
    }
    virtual CampEntity GetUnitMissionTarget(void)
    {
        return (CampEntity)vuDatabase->Find(mission_target);
    }
    virtual VU_ID GetUnitMissionTargetID(void)
    {
        return mission_target;
    }
    virtual CampaignTime GetUnitTOT(void)
    {
        return time_on_target;
    }
    virtual Unit GetUnitSquadron(void)
    {
        return FindUnit(squadron);
    }
    virtual VU_ID GetUnitSquadronID(void)
    {
        return squadron;
    }
    virtual CampEntity GetUnitAirbase(void);
    virtual VU_ID GetUnitAirbaseID(void);
    // virtual int GetUnitTakeoffSlot (void) { return (int)takeoff_slot; }
    virtual int LoadWeapons(void *squadron, uchar *dam, MoveType mt, int num, int type_flags, int guide_flags);
    virtual int DumpWeapons(void);
    virtual CampaignTime ETA(void);
    virtual F4PFList GetKnownEmitters(void);
    virtual int BuildMission(MissionRequestClass *mis);
    virtual void GetUnitAssemblyPoint(int type, GridIndex *x, GridIndex *y);
    virtual Unit GetUnitParent(void)
    {
        return (Unit)vuDatabase->Find(package);
    }
    virtual VU_ID GetUnitParentID(void)
    {
        return package;
    }
    virtual void SetUnitParent(Unit p)
    {
        SetPackage(p->Id());
    }
    virtual void IncrementTime(CampaignTime dt)
    {
        last_move += dt;
    }
    virtual int GetBestVehicleWeapon(int, uchar*, MoveType, int, int*);
    virtual void UseFuel(long);

    // Core functions
    int DetectVs(AircraftClass *ac, float *d, int *combat, int *spotted, int *estr);
    int DetectVs(CampEntity e, float *d, int *combat, int *spotted, int *estr);
    PackageClass* GetUnitPackage(void)
    {
        return (PackageClass*)vuDatabase->Find(package);
    }
    int PickRandomPilot(int seed);
    int GetAdjustedPlayerSlot(int pslot);
    int GetPilotSquadronID(int pilotSlot)
    {
        return pilots[pilotSlot];
    }
    PilotClass* GetPilotData(int pilotSlot);
    int GetPilotID(int pilotSlot);
    int GetPilotCallNumber(int pilot_slot);
    uchar GetPilotVoiceID(int pilotSlot);
    int GetPilotCount(void); // Returns # of pilots in flight (including players)
    int GetACCount(void); // Returns # of aircraft in flight
    int GetFlightLeadSlot(void); // Returns slot of flightleader
    int GetFlightLeadCallNumber(void); // Returns the callnumber (1-36) of the flightleader
    uchar GetFlightLeadVoiceID(void); // Returns the voiceId of the flightleader
    int GetAdjustedAircraftSlot(int aircraftNum);
    long CalculateFuelAvailable(int aircraftNum);
    int HasWeapons(void);
    int HasFuel(int limit = 9);  // 2002-02-20 MODIFIED BY S.G. Added 'limit' which defaults to 9 so 9/12 is the same as 3/4 used in the original code
    int CanAbort(void); // returns 1 if don't have enough fuel or weapons
    // 2001-04-03 ADDED BY S.G. THE Standoff jammer GETTER WASN'T DEFINED
    Flight GetECMFlight(void);
    // END OF ADDED SECTION
    Flight GetAWACSFlight(void);
    Flight GetJSTARFlight(void);
    Flight GetFACFlight(void);
    Flight GetTankerFlight(void);
    Flight GetFlightController(void);
    int FindCollisionPoint(FalconEntity *target, vector *collPoint, int noAWACS);
    void RegisterLock(FalconEntity *locker);

    // Component accessers (Sim Flight emulators)
    void SendComponentMessage(int command, VuEntity *sender);
    int AirbaseOperational(Objective airbase);  // JPO - is the airbase still suitable for takeoff/landings

    // 2001-04-03 ADDED BY S.G. NEED SOMETHING TO HOLD THE ecmFlightClassPtr VARIABLE
    FlightClass *ecmFlightPtr;
    // 2001-06-25 ADDED BY S.G. NEED SOMETHING TO HOLD WHO FIRED AT IT SO ONLY ONE SIM PLANE WILL LAUNCH HARMS AT AN AGGREGATED TARGET
    FalconEntity *shotAt;
    AircraftClass *whoShot;
    // 2001-10-11 ADDED by M.N.
    unsigned int refuel; // How much fuel has to be taken from a tanker
};
typedef FlightClass* Flight;

// =================================
// Support functions
// =================================

FlightClass* NewFlight(int type, Unit parent, Unit squad);

int RegroupFlight(Flight flight);

void RegroupAircraft(AircraftClass *ac);

void CancelFlight(Flight flight);

void UpdateSquadronStatus(Flight flight, int landed, int playchatter);

WayPoint ResetCurrentWP(Unit u);

void AbortFlight(Flight flight);

Objective FindAlternateStrip(Flight flight);



#endif
