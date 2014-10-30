/**************************************************************************
*
* CampWP.h
*
* Campaign waypoint manipulation routines
*
**************************************************************************/

#ifndef CAMPWP_H
#define CAMPWP_H

#include "Cmpglobl.h"

extern bool g_bPrecisionWaypoints; //Wombat778 11-5-2003

class CampBaseClass;
typedef CampBaseClass* CampEntity;

// Waypoint actions
#define WP_NOTHING 0
#define WP_TAKEOFF 1
#define WP_ASSEMBLE 2
#define WP_POSTASSEMBLE 3
#define WP_REFUEL       4
#define WP_REARM        5
#define WP_PICKUP 6 // Pick up a unit
#define WP_LAND 7
#define WP_TIMING 8 // Just cruise around wasting time
#define WP_CASCP 9 // CAS contact point

#define WP_ESCORT 10 // Engage engaging fighters
#define WP_CA 11 // Engage all enemy aircraft
#define WP_CAP 12 // Patrol area for enemy aircraft
#define WP_INTERCEPT    13 // Engage specific enemy aircraft
#define WP_GNDSTRIKE 14 // Engage enemy units at target
#define WP_NAVSTRIKE 15 // Engage enemy shits at target
#define WP_SAD 16 // Engage any enemy at target
#define WP_STRIKE 17 // Destroy enemy installation at target
#define WP_BOMB 18 // Strategic bomb enemy installation at target 
#define WP_SEAD 19 // Suppress enemy air defense at target
#define WP_ELINT 20 // Electronic intellicence (AWACS, JSTAR, ECM)
#define WP_RECON 21 // Photograph target location
#define WP_RESCUE 22 // Rescue a pilot at location
#define WP_ASW 23
#define WP_TANKER 24 // Respond to tanker requests
#define WP_AIRDROP 25
#define WP_JAM 26

// M.N. fix for Airlift missions
//#define WP_LAND2 27 // this is our 2nd landing waypoint for Airlift missions
// supply will be given at WP_LAND MNLOOK -> Change in string.txt 377

#define WP_B5
#define WP_B6
#define WP_B7
#define WP_FAC 30

#define WP_MOVEOPPOSED 40 // These are movement wps
#define WP_MOVEUNOPPOSED 41
#define WP_AIRBORNE 42
#define WP_AMPHIBIOUS 43
#define WP_DEFEND 44 // These are action wps
#define WP_REPAIR 45
#define WP_RESERVE 46
#define WP_AIRDEFENSE 47
#define WP_FIRESUPPORT 48
#define WP_SECURE 49

#define WP_LAST 50

// WP flags
#define WPF_TARGET 0x0001 // This is a target wp
#define WPF_ASSEMBLE 0x0002 // Wait for other elements here
#define WPF_BREAKPOINT 0x0004 // Break point
#define WPF_IP 0x0008 // IP waypoint
#define WPF_TURNPOINT 0x0010 // Turn point
#define WPF_CP 0x0020 // Contact point
#define WPF_REPEAT 0x0040 // Return to previous WP until time is exceeded
#define WPF_TAKEOFF 0x0080
#define WPF_LAND 0x0100 // Suck aircraft back into squadron
#define WPF_DIVERT 0x0200 // This is a divert WP (deleted upon completion of divert)
#define WPF_ALTERNATE 0x0400 // Alternate landing site
// Climb profile flags
#define WPF_HOLDCURRENT 0x0800 // Stay at current altitude until last minute
// Other stuff
#define WPF_REPEAT_CONTINUOUS 0x1000 // Do this until the end of time
#define WPF_IN_PACKAGE 0x2000 // This is a package-coordinated wp
// Even better "Other Stuff"
#define WPF_TIME_LOCKED 0x4000 // This waypoint will have an arrive time as given, and will not be changed
#define WPF_SPEED_LOCKED 0x8000 // This waypoint will have a speed as given, and will not be changed.
#define WPF_REFUEL_INFORMATION  0x10000 // This waypoint is only an informational waypoint, no mission waypoint
#define WPF_REQHELP 0x20000 // This divert waypoint is one from a request help call

#define WPF_CRITICAL_MASK 0x07FF // If it's one of these, we can't skip this waypoint

// time recalculation flags
#define WPTS_KEEP_DEPARTURE_TIMES 0x01 // Don't shift departure times when updating waypoint times
#define WPTS_SET_ALTERNATE_TIMES 0x02 // Set wp times for alternate waypoints

#define GRIDZ_SCALE_FACTOR 10 // How many feet per pt of Z.

#define MINIMUM_ASL_ALTITUDE 5000 // Below this # of feet, the WP is considered AGL

// ============================================
// WayPoint Class
// ============================================

class WayPointClass;
typedef WayPointClass* WayPoint;

class WayPointClass
{
#ifdef USE_SH_POOLS
public:
    // Overload new/delete to use a SmartHeap fixed size pool
    void *operator new(size_t size)
    {
        ShiAssert(size == sizeof(WayPointClass));
        return MemAllocFS(pool);
    };
    void operator delete(void *mem)
    {
        if (mem) MemFreeFS(mem);
    };
    static void InitializeStorage()
    {
        pool = MemPoolInitFS(sizeof(WayPointClass), 200, 0);
    };
    static void ReleaseStorage()
    {
        MemPoolFree(pool);
    };
    static MEM_POOL pool;
#endif
private:
    GridIndex           GridX; // Waypoint's X,Y and Z coordinates (in km from southwest corner)
    GridIndex GridY;
    short GridZ; // Z is in 10s of feet
    CampaignTime       Arrive;
    CampaignTime Depart; // This is only used for loiter waypoints
    VU_ID TargetID;
    uchar              Action;
    uchar RouteAction;
    uchar Formation;
    uchar TargetBuilding;
    ulong Flags; // Various wp flags
    short Tactic; // Tactic to use here
protected:
    float Speed;
    WayPoint PrevWP; // Make this one public for kicks..
    WayPoint NextWP; // Make this one public for kicks..
private:
    // These functions are intended for use by the Sim (They use sim coordinates and times)
    //Wombat778 11-05-2003 new variable to hold a Sim location rather than a grid location
    BIG_SCALAR SimX;
    BIG_SCALAR SimY;
    BIG_SCALAR SimZ;

public:
    WayPointClass();
    WayPointClass(
        GridIndex x, GridIndex y,
        int alt, int speed, CampaignTime arr, CampaignTime station, uchar action, int flags
    );
    WayPointClass(VU_BYTE **stream, long *rem);
    WayPointClass(FILE* fp);
    int SaveSize(void);
    int Save(VU_BYTE **stream);
    int Save(FILE* fp);

    // These functions are intended for general use
    void SetWPTarget(VU_ID e)
    {
        TargetID = e;
    }
    void SetWPTargetBuilding(uchar t)
    {
        TargetBuilding = t;
    }
    void SetWPAction(int a)
    {
        Action = (uchar) a;
    }
    void SetWPRouteAction(int a)
    {
        RouteAction = (uchar) a;
    }
    void SetWPFormation(int f)
    {
        Formation = (uchar) f;
    }
    void SetWPFlags(ulong f)
    {
        Flags = (ulong) f;
    }
    void SetWPFlag(ulong f)
    {
        Flags or_eq (ulong) f;
    }
    void UnSetWPFlag(ulong f)
    {
        Flags and_eq compl ((ulong)(f));
    }
    void SetWPTactic(int f)
    {
        Tactic = (short) f;
    }
    VU_ID GetWPTargetID(void)
    {
        return TargetID;
    }
    CampEntity GetWPTarget(void)
    {
        return (CampEntity)vuDatabase->Find(TargetID);
    }
    uchar GetWPTargetBuilding(void)
    {
        return TargetBuilding;
    }
    int GetWPAction(void)
    {
        return (int)Action;
    }
    int GetWPRouteAction(void)
    {
        return (int)RouteAction;
    }
    int GetWPFormation(void)
    {
        return (int)Formation;
    }
    ulong GetWPFlags(void)
    {
        return (ulong)Flags;
    }
    int GetWPTactic(void)
    {
        return (int)Tactic;
    }
    WayPoint GetNextWP(void)
    {
        return NextWP;
    }
    WayPoint GetPrevWP(void)
    {
        return PrevWP;
    }

    void SetNextWP(WayPointClass *next);
    void SetPrevWP(WayPointClass *prev);
    void UnlinkNextWP(void);

    void SplitWP(void);
    void InsertWP(WayPointClass *);
    void DeleteWP(void);

    void CloneWP(WayPoint w);
    void SetWPTimes(CampaignTime t);
    float DistanceTo(WayPoint w);

    // These functions are intended for use by the campaign (They use Campaign Coordinates and times)
    void SetWPAltitude(int alt);// { GridZ = (short)(alt/GRIDZ_SCALE_FACTOR); }
    void SetWPAltitudeLevel(int alt);// { GridZ = (short)alt; }
    void SetWPStationTime(CampaignTime t)
    {
        Depart = Arrive + t;
    }
    void SetWPDepartTime(CampaignTime t)
    {
        Depart = t;
    }
    void SetWPArrive(CampaignTime t)
    {
        Arrive = t;
    }
    void SetWPSpeed(float s)
    {
        Speed = s;
    }
    float GetWPSpeed(void)
    {
        return Speed;
    }
    void SetWPLocation(GridIndex x, GridIndex y);//  { GridX = x; GridY = y; }
    int GetWPAltitude()
    {
        return (int)(GridZ * GRIDZ_SCALE_FACTOR);
    }
    int GetWPAltitudeLevel()
    {
        return GridZ;
    }
    CampaignTime GetWPStationTime()
    {
        return (int)(Depart - Arrive);
    }
    CampaignTime GetWPArrivalTime()
    {
        return Arrive;
    }
    CampaignTime GetWPDepartureTime()
    {
        return Depart;
    }
    void AddWPTimeDelta(CampaignTime dt)
    {
        Arrive += dt;
        Depart += dt;
    }
    void GetWPLocation(GridIndex* x, GridIndex* y) const
    {
        *x = GridX;
        *y = GridY;
    }
    // Wombat778 11-5-2003 Changed to a new calculation.
    // Takes the sim position and stores it in a var, and also updates the GridX/Y/Z.
    // This is to allow waypoints to be placed more narrowly than the 1KM grid.
    // Hopefully, the sim wont care about the new more precise
    // waypoint locations.
    void SetLocation(float x, float y, float z);
    void GetLocation(float* x, float* y, float* z) const;
};

// ===================================================
// Global functions
// ===================================================

extern void DeleteWPList(WayPoint w);

// Sets a set of waypoint times to start at waypoint w at time start. Returns duration of mission
extern CampaignTime SetWPTimes(WayPoint w, CampaignTime start, int speed, int flags);

// Shifts a set of waypoints by time delta. Returns duration of mission
extern CampaignTime SetWPTimes(WayPoint w, long delta, int flags);

// Sets a set of waypoint times to start at waypoint w as soon as we can get there from x,y.
extern CampaignTime SetWPTimes(WayPoint w, GridIndex x, GridIndex y, int speed, int flags);

extern WayPoint CloneWPList(WayPoint w);
extern WayPoint CloneWPToList(WayPoint w, WayPoint stop);

extern WayPoint CloneWPList(WayPointClass wps[], int waypoints);

// KCK: This function requires that the graphic's altitude map is loaded
extern float AdjustAltitudeForMSL_AGL(float x, float y, float z);

extern float SetWPSpeed(WayPoint wp);

#endif
