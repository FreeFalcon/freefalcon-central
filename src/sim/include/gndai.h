// GNDAI - this is where we define all the ground AI structs.
// By Mark McCubbin
// (c)1997
//

// Note: rather than define derived classes from a single vehicle (probably the
// C++ obvious way, I'm making this jump table driven with a single shared class
// this will keep things simple) - it may be cleaner for a C++ programmer if
// I split this into movement classes, ranks, formation bitand combat classes and use
// multiple inheritance - I can do this simply if someone really wants it).
//
//
#ifndef _GNDAI_H
#define _GNDAI_H

#include "campwp.h"
#include "object.h"

//edg: ARGH  WTFUCK?  Why do this define???????????????
#define TargetClass SimObjectType

#define GFORM_DISPERSED 0 // Scattered / Disorganized
#define GFORM_COLUMN 1 // Your standard column
#define GFORM_OVERWATCH 3 // Cautious column
#define GFORM_WEDGE 4
#define GFORM_ECHELON 5
#define GFORM_LINE 6

typedef enum
{
    GNDAI_FORM_DISPERSED = 0,
    GNDAI_FORM_COLUMN,
    GNDAI_FORM_UNUSED,
    GNDAI_FORM_OVERWATCH,
    GNDAI_FORM_WEDGE,
    GNDAI_FORM_ECHELON,
    GNDAI_FORM_LINE,
    GNDAI_FORM_END,
} GNDAIFormType;

typedef struct
{
    float x;
    float y;
} AIOffsetType;

#define NO_OF_SQUADS 3
#define NO_OF_PLATOONS 4
#define NO_OF_COMPANIES 4

// Rank defines - KCK
#define GNDAI_BATTALION_LEADER 0x01 // Battalion command vehicle
#define GNDAI_COMPANY_LEADER 0x02 // Company command vehicle
#define GNDAI_PLATOON_LEADER 0x04 // Platoon command vehicle
#define GNDAI_SQUAD_LEADER 0x08 // Squad command vehicle (actually, squads are only one vehicle

#define GNDAI_BATTALION_COMMANDER 0x0F // All of the above roles
#define GNDAI_COMPANY_COMMANDER 0x0E // All but Battalion Leader
#define GNDAI_PLATOON_COMMANDER 0x0C

// How are we moving?
#define GNDAI_MOVE_HALTED 0 // We're not moving
#define GNDAI_MOVE_WAYPOINT 1 // We're following waypoints
#define GNDAI_MOVE_GENERAL 2 // We're following a campaign path

// Move flags
#define GNDAI_MOVE_ROAD 0x0001 // We're moving along a road
#define GNDAI_MOVE_BRIDGE 0x0002 // We're moving over a bridge
#define GNDAI_MOVE_BATTALION 0x0004 // We're a battalion lead
#define GNDAI_MOVE_FIXED_POSITIONS 0x0008 // We've been assigned a position and heading, stick to it.
#define GNDAI_WENT_THROUGH 0x0010 // We've hit our mid waypoint already

class GroundClass;
class BattalionClass;
class WayPointClass;

// When running in the testbed, GNDAIClass should be defined as GNDAIClass : public SimVehicleClass
//
class GNDAIClass   //: public SimVehicleClass {
{
public:
    long lastMoveTime;
    UnitClass *parent_unit; // Generic unit class pointer to allow for naval units

    GNDAIFormType formation;
    int squad_id; // Used for formation coordination
    int platoon_id; // Used for formation coordination
    int company_id; // Used for formation coordination
    int rank;

    // Platform specific routines, these routines do the low-level grunt
    // work
    //
    BOOL Follow_Road(void);
    WayPointClass *Next_WayPoint(void);
    void  Fire(void);


    GNDAIClass(GroundClass *s, GNDAIClass *l, short r, int unit_id, int skill);
    ~GNDAIClass(void);
    void Process(void);
    void ProcessTargeting(void);
    void SetLeader(GNDAIClass*);

    // Managment routines
    //
    void Order_Battalion(void);
    void Order_Company(void);
    void Order_Platoon(void);
    void Order_Squad(void);
    int CheckThrough(void);

    void SetGroundTarget(SimObjectType *newTarget);
    void SetAirTarget(SimObjectType *newTarget);

    // when a leader dies, subordinates need to be promoted
    void PromoteSubordinates(void);

    // calc an LOD based on camera distance
    void SetDistLOD(void);

    // Movement routines
    //
    void Move_Towards_Dest(void);

    // Public vars
    //
    float ideal_x; // Best X bitand Y, heading
    float ideal_y;
    float ideal_h;

    float through_x; // Point through which everyone must travel
    float through_y; // (For column movement)

    float icosh; // sin and cos for ideal heading (leaders only)
    float  isinh;

    float leftToGoSq; // Distance squared remaining to travel

    float maxvel; // Fastest we can go (fps)
    float unitvel; // Unit's movement speed (as a whole battalion)

    float move_backwards; // Set to PI if we move backwards (towed)

    GroundClass *self; // Every AI Object has a Sim object
    GNDAIClass *leader; // Our immediate superior (NULL for Battalion commander)
    GNDAIClass *battalionCommand; // highest in formation ("this" for Battalion commander)

    ulong nextAirFire; // firing delay
    ulong nextGroundFire; // firing delay
    ulong nextTurretCalc; // when to recalculate for turret move

    ulong airFireRate; // rate governing fire freq vs air
    ulong gndFireRate; // rate governing fire freq vs ground

    int skillLevel; // How smart is this guy?

    float distLOD; // 0 - 1 LOD value based on cam dist

    // edg: these are used in campaign movement, mostly
    // for road and bridge following
    GridIndex gridX, gridY;
    GridIndex lastGridX, lastGridY;
    int moveDir;

    int moveState; // how are we moving?
    int moveFlags;

protected:
    SimObjectType *gndTargetPtr; // Our campaign parent's ground target
    SimObjectType *airTargetPtr; // Our campaign parent's air target
    // 2001-03-26 ADDED BY S.G. SINCE I NEED airTargetPtr DO TO LOS BEFORE SPOTTING THE TARGET
public:
    SimObjectType *GetAirTargetPtr(void)
    {
        return airTargetPtr;
    }
protected:
    // END OF ADDED SECTION


#ifdef USE_SH_POOLS
public:
    // Overload new/delete to use a SmartHeap fixed size pool
    void *operator new(size_t size)
    {
        ShiAssert(size == sizeof(GNDAIClass));
        return MemAllocFS(pool);
    };
    void operator delete(void *mem)
    {
        if (mem) MemFreeFS(mem);
    };
    static void InitializeStorage()
    {
        pool = MemPoolInitFS(sizeof(GNDAIClass), 200, 0);
    };
    static void ReleaseStorage()
    {
        MemPoolFree(pool);
    };
    static MEM_POOL pool;
#endif
};

#endif
