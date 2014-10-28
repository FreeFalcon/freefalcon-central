#ifndef _FAULT_H
#define _FAULT_H

#ifndef _WINDOWS_
#include <windows.h>
#endif

#ifdef USE_SH_POOLS
extern MEM_POOL gFaultMemPool;
#endif

//-------------------------------------------------
// Many typedefs to model DED faults
//-------------------------------------------------
//-------------------------------------------------
// Class Defintion
//-------------------------------------------------


class FaultClass
{

public:
    enum { MAX_MFL = 17 // 15 + T/O Land
         };
    typedef enum type_FSubSystem
    {
        amux_fault, // Avionics Data Bus. If amux and bmux failed_fault, fail all avionics (very rare)
        blkr_fault, // Interference Blanker. If failed_fault, radio drives radar crazy
        bmux_fault, // Avionics Data Bus. If amux and bmux failed_fault, fail all avionics (very rare)
        cadc_fault, // Central Air Data Computer. If failed no baro altitude
        cmds_fault, // Countermeasures dispenser system, breaks chaff and flare
        dlnk_fault, // Improved Data Modem. If failed no data transfer in
        dmux_fault, // Weapon Data Bus. If failed_fault, no weapons fire on given station
        dte_fault,  // Just for show
        eng_fault,  // Engine. If failed usually means fire_fault, but could mean hydraulics
        eng2_fault, // Engine 2 faults TJL 01/11/04
        epod_fault, // ECM pod can't shut up
        fcc_fault,  // Fire Control Computer. If failed_fault, no weapons solutions
        fcr_fault,  // Fire Control Radar. If failed_fault, no radar
        flcs_fault, // Digital Flight Control System. If failed one or more control surfaces stuck
        fms_fault,  // Fuel Measurement System. If failed_fault, fuel gauge stuck
        gear_fault, // Landing Gear. If failed one or more wheels stuck
        gps_fault,  // Global positioning system failure, no change if CADC or INS
        harm_fault, // Broken Harms
        hud_fault,  // Heads Up Display. If failed_fault, no HUD
        iff_fault,  // Identification_fault, Friend or Foe. If failed_fault, no IFF
        ins_fault,  // Inertial Navigation System. If failed_fault, no change in waypoint data
        isa_fault,  // Integrated servo actuator. If failed_fault, no rudder
        mfds_fault, // Multi Function Display Set. If an MFD fails_fault, it shows noise
        msl_fault,  // Missile Slave Loop. If failed_fault, missile will not slave to radar
        ralt_fault, // Radar Altimeter. If failed_fault, no ALOW warning
        rwr_fault,  // Radar Warning Reciever. If failed_fault, no rwr
        sms_fault,  // Stores Management System. If failed_fault, no weapon or inventory display_fault, and no launch
        tcn_fault,  // TACAN. If failed no TACAN data
        ufc_fault,  // Up Front Controller. If failed_fault, UFC/DED inoperative
        NumFaultListSubSystems,
        landing, takeoff, // pseudo entries for MFL
        TotalFaultStrings,
    };

    //-------------------------------------------------

    typedef enum type_FFunction
    {
        nofault = 0x0,    //0
        bus     = 0x1,
        slnt    = 0x2,
        chaf    = 0x4,
        flar    = 0x8,    // 5
        dmux    = 0x10,
        dual    = 0x20,
        sngl    = 0x40,
        a_p     = 0x80,   // 8
        rudr    = 0x100,
        all     = 0x200,
        xmtr    = 0x400,
        a_i     = 0x800,  // 12
        a_b     = 0x1000,
        pfl     = 0x2000,
        efire   = 0x4000,
        hydr    = 0x8000, //16
        m_3     = 0x10000,
        m_c     = 0x20000,
        slv     = 0x40000,
        lfwd    = 0x80000, //20
        rfwd    = 0x100000,
        sta1    = 0x200000,
        sta2    = 0x400000,
        sta3    = 0x800000, // 24
        sta4    = 0x1000000,
        sta5    = 0x2000000,
        sta6    = 0x4000000,
        sta7    = 0x8000000, // 28
        sta8    = 0x10000000,
        sta9    = 0x20000000,
        ldgr    = 0x40000000,
        fl_out  = 0x80000000, // 32
        NumFaultFunctions = 33
    };



    //-------------------------------------------------

    typedef enum type_FSeverity
    {
        cntl, degr,
        fail, low,
        rst, temp,
        warn,       no_fail,
        NumFaultSeverity
    };


    //-------------------------------------------------
    // Structures for returning data when calling
    // GetFault() and GetFaultNames()
    //-------------------------------------------------

    struct str_FEntry
    {
        unsigned int               elFunction;
        type_FSeverity elSeverity;
    };

    //-------------------------------------------------

    struct str_FNames
    {
        const char* elpFSubSystemNames;
        const char* elpFFunctionNames;
        const char* elpFSeverityNames;
    };

    struct str_DmgProbs
    {
        int    numFuncs;
        float  systemProb;
        float* funcProb;
    };

    struct FaultListItem
    {
        type_FSubSystem type; // which system
        int subtype; // what fault number
        int no; // count
        int time; // when in seconds
    };

private:
    str_FEntry mpFaultList[TotalFaultStrings];
    int mFaultCount;
    FaultListItem mMflList[MAX_MFL];
    VU_TIME mStartTime;
    int mLastMfl;
public:
    enum   // fault flags
    {
        FF_NONE = 0x00,
        FF_PSUEDO = 0x01, // not a real fault
        FF_PFL = 0x02, // significant for PFL
    };
    static const struct InitFaultData  // JPO - initialisation structure
    {
        char *mpFSSName;
        type_FFunction mBreakable; // the functions
        float mSProb; // system probability
        float *mFProb; // function probability
        int mCount; // count of these bitand breakables
        unsigned int flags;
    } mpFaultData[TotalFaultStrings];
    static const char* mpFFunctionNames[NumFaultFunctions];
    static const char* mpFSeverityNames[NumFaultSeverity];

    type_FFunction PickFunction(type_FSubSystem);
    type_FSubSystem PickSubSystem(int systemBits);
    BOOL IsFlagSet();
    void ClearFlag();
    void SetFault(type_FSubSystem, type_FFunction, type_FSeverity, BOOL);
    void ClearFault(type_FSubSystem);
    void ClearFault(type_FSubSystem, type_FFunction);

    void GetFault(type_FSubSystem, str_FEntry*);
    int GetFault(type_FSubSystem);

    void GetFaultNames(type_FSubSystem, int, str_FNames*);

    int GetFaultCount(void)
    {
        return mFaultCount;
    }
    void TotalPowerFailure(); // JPO
    void RandomFailure(); // THW 2003-1120
    int      Breakable(type_FSubSystem id)
    {
        return mpFaultData[id].mBreakable;
    };

    // MFL support
    void AddMflList(VU_TIME time, type_FSubSystem type, int subtype); // add new MFL entry
    bool GetMflEntry(int n, const char **name, int *subsys, int *count, char timestr[]);
    int GetMflListCount()
    {
        return mLastMfl;
    };
    void ClearMfl()
    {
        mLastMfl = 0;
    };
    void SetStartTime(VU_TIME time)
    {
        mStartTime = time;
    };

    BOOL FindFirstFunction(type_FSubSystem sys, int *functionp);
    BOOL FindNextFunction(type_FSubSystem sys, int *functionp);
    BOOL GetFirstFault(type_FSubSystem *subsystemp, int *functionp);
    BOOL GetNextFault(type_FSubSystem *subsystemp, int *functionp);
    FaultClass();
    ~FaultClass();
};

#endif
