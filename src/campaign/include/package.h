// ===================================================
// Mission Group Class
// ===================================================

#ifndef MISGROUP_H
#define MISGROUP_H

#include "mission.h"
#include "unit.h"
#include "listadt.h"
#include "airunit.h"

// Return values for BuildMissionGroup
#define PRET_NO_ASSETS 1
#define PRET_DELAYED 2
#define PRET_SUCCESS 3
#define PRET_ABORTED 4
#define PRET_CANCELED 5
#define PRET_REPEAT 6
#define PRET_HANDLED 7
#define PRET_NOTARGET 8
#define PRET_TIMEOUT 9

// Reponse receipt flags
#define PRESPONSE_CA 0x01
#define PRESPONSE_ESCORT 0x02
#define PRESPONSE_AWACS 0x04
#define PRESPONSE_JSTAR 0x08
#define PRESPONSE_TANKER 0x10
#define PRESPONSE_ECM 0x20

// =========================
// Package Class
// =========================

class PackageClass : public AirUnitClass
{
#ifdef USE_SH_POOLS
public:
    // Overload new/delete to use a SmartHeap fixed size pool
    void *operator new(size_t size)
    {
        ShiAssert(size == sizeof(PackageClass));
        return MemAllocFS(pool);
    };
    void operator delete(void *mem)
    {
        if (mem) MemFreeFS(mem);
    };
    static void InitializeStorage()
    {
        pool = MemPoolInitFS(sizeof(PackageClass), 200, 0);
    };
    static void ReleaseStorage()
    {
        MemPoolFree(pool);
    };
    static MEM_POOL pool;
#endif

private:
    uchar elements; // Number of child units
    uchar c_element; // Which one we're looking at
    VU_ID element[MAX_UNIT_CHILDREN]; // VU_IDs of elements
    VU_ID interceptor; // ID of enemy BARCAP/SWEEP/Etc flight, if any
    VU_ID awacs; // ID of any awacs support
    VU_ID jstar; // ID of any jstar support
    VU_ID ecm; // ID of any ecm support
    VU_ID tanker; // ID of any tanker support
    uchar wait_cycles; // How many cycles until timeout
    uchar flights; // flights in this package
    ushort wait_for; // Mission Requests to wait for (until timeout)
    // This stuff shouldn't change after init
    GridIndex iax, iay; // Ingress assembly point
    GridIndex eax, eay; // Egress assembly point (if any)
    GridIndex bpx, bpy; // Break point (if any)
    GridIndex tpx, tpy; // Turn point (if any)
    CampaignTime takeoff; // Earliest flight's takeoff time
    CampaignTime tp_time;
    ulong package_flags;
    uchar escort_type; // 2001-11-10 M.N.
    short caps; // capabilities required for this package
    short requests; // What other mission types we want.
    short responses; // What sort of reaction we've caused
    WayPoint ingress; // Ingress Route
    WayPoint egress; // Egress Route
    MissionRequestClass mis_request; // The origional request (lot'so repeated data here, we could trim)
    // Not added to i/o functions
    short aa_strength; // The combined Air to Air strength of this package
    int dirty_package;

public:
    // Access Functions

    VU_ID GetInterceptor(void)
    {
        return interceptor;
    }
    VU_ID GetAwacs(void)
    {
        return awacs;
    }
    VU_ID GetJStar(void)
    {
        return jstar;
    }
    VU_ID GetECM(void)
    {
        return ecm;
    }
    /* 2001-04-07 ADDED BY S.G. */ void SetECM(VU_ID newEcm)
    {
        ecm = newEcm;
    }
    VU_ID GetTanker(void)
    {
        return tanker;
    }
    Flight GetFACFlight(void);
    CampaignTime GetTakeoff(void)
    {
        return takeoff;
    }
    CampaignTime GetTPTime(void)
    {
        return tp_time;
    }
    uchar GetFlights(void)
    {
        return flights;
    }
    short GetResponses(void)
    {
        return responses;
    }
    WayPoint GetIngress(void)
    {
        return ingress;
    }
    WayPoint GetEgress(void)
    {
        return egress;
    }
    short GetAAStrength(void)
    {
        return aa_strength;
    }
    MissionRequestClass *GetMissionRequest(void)
    {
        return &mis_request;
    }

    void SetTanker(VU_ID);
    void SetTakeoff(CampaignTime);
    void SetPackageFlags(ulong);
    void SetTPTime(CampaignTime);

public:
    // constructors and serial functions
    PackageClass(ushort type);
    //sfr: changed prototype
    //PackageClass(VU_BYTE **stream);
    PackageClass(VU_BYTE **stream, long *rem);
    virtual ~PackageClass();
    virtual int SaveSize(void);
    virtual int Save(VU_BYTE **stream);

    // event Handlers
    virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);

    // required virtuals
    virtual int Reaction(CampEntity, int, float)
    {
        return 0;
    }
    virtual int MoveUnit(CampaignTime);
    virtual int ChooseTactic(void)
    {
        return 0;
    }
    virtual int CheckTactic(int)
    {
        return 0;
    }
    virtual int Father() const
    {
        return 1;
    }
    virtual int Real(void)
    {
        return 0;
    }
    virtual int IsPackage(void)
    {
        return TRUE;
    }

    // Dirty Data Stuff
    void MakePackageDirty(Dirty_Package bits, Dirtyness score);
    void WriteDirty(unsigned char **stream);
    void ReadDirty(VU_BYTE **stream, long *rem);

    // core functions
    virtual int BuildPackage(MissionRequest mis, F4PFList assemblyList);
    int RecordFlightAddition(Flight flight, MissionRequest mis, int targetd);
    void FindSupportFlights(MissionRequest mis, int targetd);
    virtual void HandleRequestReceipt(int type, int them, VU_ID flight);
    virtual Unit GetFirstUnitElement(void);
    virtual Unit GetNextUnitElement(void);
    virtual Unit GetUnitElement(int e);
    virtual Unit GetUnitElementByID(int eid);
    virtual void AddUnitChild(Unit e);
    virtual void DisposeChildren(void);
    virtual void RemoveChild(VU_ID eid);
    int CheckNeedRequests(void);
    void CancelFlight(Flight flight);
    void SetPackageType(uchar p)
    {
        mis_request.mission = p;
    }
    int GetPackageType(void)
    {
        return mis_request.mission;
    }
    VU_ID GetMainFlightID(void);
    Flight GetMainFlight(void);

    virtual void SetUnitAssemblyPoint(int type, GridIndex x, GridIndex y);
    virtual void GetUnitAssemblyPoint(int type, GridIndex *x, GridIndex *y);
};

typedef PackageClass* Package;

// ===================================================
// Global functions
// ===================================================

PackageClass* NewPackage(int type);

Flight AttachFlight(MissionRequest mis, Package pack);

#endif
