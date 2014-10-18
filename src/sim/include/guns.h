#ifndef _GUNS_H
#define _GUNS_H

#include "geometry.h"
#include "simweapn.h"
#include "sms.h"
#include "hardpnt.h"

class SimObjectType;
class DrawableTracer;
class DrawableTrail;
class Drawable2D;
struct WeaponClassDataType;

struct Tpoint;

typedef struct
{
    float x, y, z;
    float xdot, ydot, zdot;
    int flying;
} GunTracerType;

typedef enum TracerCollisionMode
{
    COLLIDE_CHECK_ALL,
    COLLIDE_CHECK_ENEMY,
    COLLIDE_CHECK_NOFEATURE
};


// returned by tracer checks
#define TRACER_HIT_NOTHING 0x00000000
#define TRACER_HIT_FEATURE 0x00000001
#define TRACER_HIT_UNIT 0x00000002
#define TRACER_HIT_GROUND 0x00000004

class GunClass : public SimWeaponClass
{

#ifdef USE_SH_POOLS
public:
    // Overload new/delete to use a SmartHeap fixed size pool
    void *operator new(size_t size)
    {
        ShiAssert(size == sizeof(GunClass));
        return MemAllocFS(pool);
    };
    void operator delete(void *mem)
    {
        if (mem) MemFreeFS(mem);
    };
    static void InitializeStorage()
    {
        pool = MemPoolInitFS(sizeof(GunClass), 200, 0);
    };
    static void ReleaseStorage()
    {
        MemPoolFree(pool);
    };
    static MEM_POOL pool;
#endif

private:
    float xPos, yPos, zPos;
    float pitch, yaw;
    float dragFactor;
    int fireCount, bursts;
    int initialRounds;
    float fractionalRoundsRemaining; // Could make numRoundsRemaining a float, but this changed less code...
    DrawableTracer** tracers;
    Drawable2D** bullets;
    // ********** NEW TRAIL STUFF *************
    //DrawableTrail* smokeTrail;
    DWORD Trail;
    DWORD TrailIdNew;
    // ****************************************

    DrawableTrail* smokeTrail;
    TracerCollisionMode tracerMode;

    // these variables are used to do the series of muzzle tracers
    DrawableTracer **firstTracer;
    Tpoint *muzzleLoc;
    Tpoint *muzzleEnd;
    float *muzzleAlpha;
    float *muzzleWidth;

    int* trailState;
    int muzzleStart;
    float qTimer;
    void UpdateTracers(int firing);
    int trailID; // MLR 12/13/2003 - DrawableTrailID, has to be set before Exec() is called;


public:
    void SetTrailID(int ID)
    {
        trailID = ID;   // MLR 12/13/2003 - DrawableTrailID, has to be set before Exec() is called;
    }
    void InitTracers();
    void CleanupTracers();
    GunClass(int type);
    virtual ~GunClass();
    virtual void InitData();
    virtual void CleanupData();
private:
    void InitLocalData(int type);
    void CleanupLocalData();
public:
    enum GunStatus {Ready, Sim, Safe};
    float initBulletVelocity;
    GunStatus status;
    GunTracerType *bullet;
    float roundsPerSecond;
    int numRoundsRemaining;
    int numTracers;
    int numFirstTracers;
    int unlimitedAmmo;
    int numFlying;
    unsigned long FiremsgsendTime;

    void Init(float muzzleVel, int numRounds);
    void SetPosition(float xOffset, float yOffset, float zOffset, float pitch, float yaw);
    int Exec(int* fire, TransformMatrix dmx, ObjectGeometry *geomData, SimObjectType* objList, BOOL isOwnship);

    void NewBurst(void)
    {
        bursts++;
    };
    int GetCurrentBurst(void)
    {
        return bursts;
    };

    void SetTracerCollisionMode(TracerCollisionMode mode)
    {
        tracerMode = mode;
    }

    // new stuff for to support shells

    // typedefs and enums
    enum GunType
    {
        GUN_SHELL,
        GUN_TRACER,
        GUN_TRACER_BALL
    };

    virtual int IsGun(void)
    {
        return TRUE;
    };

    // member functions
    BOOL IsShell(void);
    BOOL IsTracer(void);
    BOOL ReadyToFire(void);
    float GetDamageAssessment(SimBaseClass *target, float range);
    void FireShell(SimObjectType *target);
    void UpdateShell(void);
    WeaponDomain GetSMSDomain(void);

    //MI to add a timer to AAA
    bool CheckAltitude; //Do we need to update the target Altitude?
    VU_TIME AdjustForAlt; //How long it takes the AAA to adjust for our new Alt
    float TargetAlt; //How high our target currently is

    // member variables
    WeaponDomain  gunDomain; // air, land, both
    SimObjectType  *shellTargetPtr; // set when shell flying
    GunType typeOfGun; // tracer or shell
    VU_TIME shellDetonateTime; // when it goes cablooey
    float minShellRange; // minimum for shell
    float maxShellRange; // max for shell
    WeaponClassDataType  *wcPtr; // pointer to weapon class data
};

#endif
