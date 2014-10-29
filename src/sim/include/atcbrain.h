#ifndef _ATC_BRAIN_H
#define _ATC_BRAIN_H

#ifdef DEBUG
//#define TEST_HACK_THAT_LEAKS // This is a BAD thing, but might help track down a bug -- remove soon
#endif

class AircraftClass;
class ObjectiveClass;
class FlightClass;
class SimBaseClass;
class CampBaseClass;

#define SIN_THREE_DEG_GLIDE    0.05233595624294F
#define TAN_THREE_DEG_GLIDE   0.05240777928304F
#define TAN_SIX_DEG_GLIDE  0.1051042352657F

// ranges are the square of the distance (ie. twr range of 225 is really 15nm)
enum
{
    TOWER_RANGE = 625, //RAS-5Oct04-changed from 15nm to 25nm (was 225)
    APPROACH_RANGE = 900,
    ATC_DROP_RANGE = 1600,
    ATC_VOICE = 12,
    APPROACH_VOICE = 13,
    LAND_TIME_DELTA = 60000,
    TAKEOFF_TIME_DELTA = 10000, // FRB - was 15 secs
    WINGMAN_WAIT_TIME = 15000,  // FRB - was 30 secs
    FINAL_TIME = 120000,
    BASE_TIME = 60000,
    SLOT_TIME = 10000, // FRB - was 15 secs
    EMER_SLOT = 60000
};

typedef enum
{
    noATC,

    lReqClearance,
    lReqEmerClearance,
    lIngressing,
    lTakingPosition,
    lAborted,
    lEmerHold,
    lHolding,
    lFirstLeg,
    lToBase,
    lToFinal,
    lOnFinal,
    lClearToLand,
    lLanded,
    lTaxiOff,
    lEmergencyToBase,
    lEmergencyToFinal,
    lEmergencyOnFinal,
    lCrashed,

    tReqTaxi,
    tReqTakeoff,
    tEmerStop,
    tTaxi,
    tWait,
    tHoldShort,
    tPrepToTakeRunway,
    tTakeRunway,
    tTakeoff,
    tFlyOut,
    tTaxiBack,
} AtcStatusEnum;

//RAS - 16Jan04 Check for Traffic
typedef enum
{
    noTraffic,
    oldTraffic,
    newTraffic,
    conflictTraffic,
    priorityTraffic,
} TrafficStatusEnum;

#ifdef TEST_HACK_THAT_LEAKS
class runwayQueueStruct
{
public:
    runwayQueueStruct()
    {
        deletor = NULL;
        deleteLine = 0;
    };
    ~runwayQueueStruct()
    {
        ShiAssert( not "We don't want to do this while testing (except for shutdown)");
    };
#else
typedef struct runwayQueueStruct
{
#endif

    VU_ID aircraftID; //which plane is it
    AtcStatusEnum status; //at what point in the landing/takeoff process
    VU_TIME schedTime; //when scheduled to be on runway
    VU_TIME lastContacted; //time last talked to
    // int curTaxiPoint; //used for tracking players
    // int timer; //at what time have I been in my current state too long
    int rwindex; //what runway I'm supposed to use
    runwayQueueStruct *next;
    runwayQueueStruct *prev;

#ifdef TEST_HACK_THAT_LEAKS
    void *deletor;
    int deleteLine;
#endif

#ifdef USE_SH_POOLS
public:
    // Overload new/delete to use a SmartHeap fixed size pool
    void *operator new(size_t size)
    {
        return MemAllocPtr(pool, size, 0);
    };
    void operator delete(void *mem)
    {
        if (mem) MemFreePtr(mem);
    };
    static void InitializeStorage()
    {
        pool = MemPoolInit(0);
    };
    static void ReleaseStorage()
    {
        MemPoolFree(pool);
    };
    static MEM_POOL pool;
#endif
#ifdef TEST_HACK_THAT_LEAKS
};
#else
} runwayQueueStruct;
#endif

typedef struct runwayStatsStruct
{
    int rwIndexes[2]; //rwindexes into PtHeaderDataTable for this runway
    float halfwidth;
    float halfheight;
    float centerX;
    float centerY;
    unsigned short state : 3; //VIS_DESTROYED, VIS_DAMAGED, VIS_NORMAL, VIS_REPAIRED
    unsigned short numInQueue : 13; //how many are waiting to use this runway
    AircraftClass *rnwyInUse;
    VU_ID nextEmergency;
} runwayStatsStruct;

class ATCBrain
{
#ifdef USE_SH_POOLS
public:
    // Overload new/delete to use a SmartHeap fixed size pool
    void *operator new(size_t size)
    {
        ShiAssert(size == sizeof(ATCBrain));
        return MemAllocFS(pool);
    };
    void operator delete(void *mem)
    {
        if (mem) MemFreeFS(mem);
    };
    static void InitializeStorage()
    {
        pool = MemPoolInitFS(sizeof(ATCBrain), 20, 0);
    };
    static void ReleaseStorage()
    {
        MemPoolFree(pool);
    };
    static MEM_POOL pool;


#endif
private:
    uchar voice;
    short callsign;
    ObjectiveClass* self;
    int numRwys;
    ulong minDeagTime;
    runwayStatsStruct *runwayStats;
    runwayQueueStruct **runwayQueue;
    runwayQueueStruct *inboundQueue;

    void ProcessInbound(void);
    void ProcessRunways(void);
    void ProcessPlayers(void);
    void ProcessQueue(int queue);

    void CheckFinalApproach(AircraftClass *aircraft, runwayQueueStruct *info);
    void ReschedulePlanes(int queue);
    void RescheduleFlightTakeoff(int queue, FlightClass *flight);
    void ReorderFlight(int queue, FlightClass *flight, AtcStatusEnum status);
    runwayQueueStruct* NextToTakeoff(int queue);
    runwayQueueStruct*  NextToLand(int queue);
    void CalcRunwayDimensions(int queue);

    //debug only
    void CheckList(runwayQueueStruct *list);

public:
    ATCBrain(ObjectiveClass* mySelf);
    ~ATCBrain(void);
    void Exec(void);
    void SendCmdMessage(AircraftClass* aircraft, runwayQueueStruct* info);
    ObjectiveClass* Self(void)
    {
        return self;
    }
    int NumRunways(void)
    {
        return numRwys;
    }
    int NumOperableRunways(void);
    runwayStatsStruct* GetRunwayStats(void)
    {
        return runwayStats;
    }
    runwayQueueStruct* InList(VU_ID aircraftID);
    uchar Voice(void)
    {
        return voice;
    }
    short Approach(void)
    {
        return callsign;
    }
    short Tower(void)
    {
        return (short)(callsign + 46);
    }
    short Name(void)
    {
        return callsign;
    }
    ulong MinDeagTime(void)
    {
        return minDeagTime;
    }

    void RequestClearance(AircraftClass* aircraft, int addflight = FALSE);
    void RequestEmerClearance(AircraftClass* aircraft);
    void RequestTakeoff(AircraftClass* aircraft);
    void RequestTaxi(AircraftClass* aircraft);
    void AbortApproach(AircraftClass* aircraft);

    void AddInbound(AircraftClass* aircraft);
    void AddInboundFlight(FlightClass *flight);
    runwayQueueStruct* AddTraffic(VU_ID aircraftID, AtcStatusEnum status, int rwindex, long schedTime);
    runwayQueueStruct* AddToList(runwayQueueStruct* list, runwayQueueStruct* info);
    void RemoveTraffic(VU_ID aircraftID, int queue);
    runwayQueueStruct* RemoveFromList(runwayQueueStruct* list, runwayQueueStruct* info);
    void RemoveInbound(runwayQueueStruct* info);
    void RemoveFromAllOtherATCs(AircraftClass *aircraft);
    void RemoveFromAllATCs(AircraftClass *aircraft);
    ulong RemovePlaceHolders(VU_ID id);
    void SetEmergency(int queue);

    int GetRunwayTexture(int component);
    int GetRunwayName(int rwindex); //this has a hack for the unexpected 23R\L and 05R\L
    short GetTextureIdFromHdg(int hdg, int ltrt);
    int GetOppositeRunway(int rwindex);

    void FindNextEmergency(int queue);
    ulong FindFlightTakeoffTime(FlightClass *flight, int queue);
    int FindBestTakeoffRunway(int checklist = FALSE);
    int FindBestLandingRunway(FalconEntity* landing, int checklist = FALSE);
    void FindEmergencyLandingRunway(int *queue, int *rwindex, FalconEntity* landing);

    void FindAbortPt(AircraftClass* aircraft, float *x, float *y, float *z);
    int FindTakeoffPt(FlightClass* flight, int vehicleInUnit, int rwindex, float *x, float *y);
    int FindRunwayPt(FlightClass* flight, int vehicleInUnit, int rwindex, float *x, float *y);
    void FindFinalPt(AircraftClass* approaching, int rwindex, float *x, float *y);
    AtcStatusEnum FindBasePt(AircraftClass* approaching, int rwindex, float finalX, float finalY, float *x, float *y);
    AtcStatusEnum FindFirstLegPt(AircraftClass* approaching, int rwindex, ulong schedTime, float baseX, float baseY, int usebase, float *x, float *y);

    ulong GetNextAvailRunwayTime(int queue, ulong rwTime, ulong delta);
    float DetermineAngle(AircraftClass* approaching, int rwindex, AtcStatusEnum status);
    float GetAltitude(AircraftClass* aircraft, AtcStatusEnum status);
    void CalculateMinMaxTime(AircraftClass* aircraft, int rwindex, AtcStatusEnum status, ulong *min, ulong *max, float cosAngle);
    int CalculateStandRateTurnToPt(AircraftClass *aircraft, float x, float y, float *finalHdg);

    int IsOnRunway(float x, float y);
    int IsOnRunway(AircraftClass* aircraft);
    int IsOnRunway(int taxipoint);
    int IsOverRunway(AircraftClass* aircraft);
    int IsRunwayBlocked(int rwindex);
    int UseSectionTakeoff(FlightClass *flight, int rwindex);

    void MakeVectorCall(AircraftClass *aircraft, VuTargetEntity *target);
    int GetLandingNumber(runwayQueueStruct* landInfo);
    int GetTakeoffNumber(runwayQueueStruct* takeoffInfo);

    int CheckVector(AircraftClass *aircraft, runwayQueueStruct* info);
    int CheckLanding(AircraftClass *aircraft, runwayQueueStruct* landInfo);
    int CheckTakeoff(AircraftClass *aircraft, runwayQueueStruct* info);
    int CheckIfBlockingRunway(AircraftClass *aircraft, runwayQueueStruct* info);

    void GiveOrderToSection(AircraftClass *us, AtcStatusEnum status, int section);
    void GiveOrderToWingman(AircraftClass *us, AtcStatusEnum status);

    // RAS - 16Jan04 - Find Nearest Traffic
    void CheckForTraffic(AircraftClass *aircraft, runwayQueueStruct *playerInfo);
    VU_TIME checkTrafficTime; // next time traffic should be checked
    VU_TIME lastTrafficCallTime; // last time any traffic called out
    SimBaseClass *pLastTraffic; // store traffic found
    float trafficRange; // traffic range
    float oldTrafficRange; // store last traffic range
    float trafficAltitude; // traffic altitude
    TrafficStatusEnum trafficCheck; // status of traffic found
    bool trafficInSightFlag; // flag when traffic is in sight

};

// this is the max dist squared we use for potential fields in functions below
//#define MAX_RANGE_COLL  (180.0f)
#define MAX_RANGE_COLL  (20.0f)
#define MAX_RANGE_SQ  (MAX_RANGE_COLL * MAX_RANGE_COLL)
#define AVOID_RANGE 10.0F  // FRB - was 20'
#define MAX_AZ   (70.0f * DTR)  // FRB - was 90
#define TAXI_CHECK_DIST 60.0F  // FRB - was 50'

//I did this so I could have ONE function that is called from different places
SimBaseClass* CheckPointGlobal(AircraftClass *self, float x, float y);
SimBaseClass* CheckTaxiPointGlobal(AircraftClass *self, float x, float y);
SimBaseClass* CheckPointGlobal(CampBaseClass *unit, float x, float y);

#endif
