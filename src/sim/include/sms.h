#ifndef _SMS_H
#define _SMS_H

#include "hardpnt.h"
#include "fcc.h" //MI

// Forward declaration of class pointers
class SimVehicleClass;
class SimBaseClass;
class FalconPrivateOrderedList;
class SmsDrawable;
class SimWeaponClass;
class GunClass;
class MissileClass;
class BombClass;
class AircraftClass;
class FireControlComputer; //MI
class AirframeClass; // RV - I-Hawk

// ==================================================================
// SMSBaseClass
//
// By Kevin K, 6/23
//
// This holds the minimum needed to keep track of weapons
// ==================================================================

enum JettisonMode // MLR 3/2/2004 - Jettison Mode
{
    JettisonNone    = 0,
    Emergency       = 1 << 1,
    SelectiveWeapon = 1 << 2,
    SelectiveRack   = 1 << 3 bitor SelectiveWeapon,
    SelectivePylon  = 1 << 4 bitor SelectiveRack,
    RippedOff       = 1 << 5
};

class SMSBaseClass
{
public:
#ifdef USE_SH_POOLS
    // Overload new/delete to use a SmartHeap fixed size pool
    void *operator new(size_t size)
    {
        ShiAssert(size == sizeof(SMSBaseClass));
        return MemAllocFS(pool);
    };
    void operator delete(void *mem)
    {
        if (mem) MemFreeFS(mem);
    };
    static void InitializeStorage()
    {
        pool = MemPoolInitFS(sizeof(SMSBaseClass), 200, 0);
    };
    static void ReleaseStorage()
    {
        MemPoolFree(pool);
    };
    static MEM_POOL pool;
#endif

    enum JDAMtargetingMode {PB, TOO};
    enum MasterArmState {Safe, Sim, Arm};
    enum CommandFlags
    {
        Loftable                = 0x0001,
        HasDisplay              = 0x0002,
        HasBurstHeight          = 0x0004,
        UnlimitedAmmoFlag       = 0x0008,
        EmergencyJettisonFlag   = 0x0010,
        Firing                  = 0x0020,
        LGBOnBoard              = 0x0040,
        HTSOnBoard              = 0x0080,
        SPJamOnBoard            = 0x0100,
        GunOnBoard              = 0x0200,
        Trainable               = 0x0400,
        TankJettisonFlag  = 0x0800 // 2002-02-20 ADDED BY S.G. Flag it if we have jettisoned our tanks (mainly for digis)
    };
    BasicWeaponStation **hardPoint;

    SMSBaseClass(SimVehicleClass *newOwnship, short *weapId, uchar *weapCnt, int advanced = FALSE);
    virtual ~SMSBaseClass();
    virtual void AddWeaponGraphics(void);
    virtual void FreeWeaponGraphics(void);
    virtual SimWeaponClass* GetCurrentWeapon(void);

    GunClass* GetGun(int hardpoint);
    MissileClass* GetMissile(int hardpoint);
    BombClass* GetBomb(int hardpoint);

    int GetCurrentWeaponHardpoint(void)
    {
        return curHardpoint;
    };
    WeaponType GetCurrentWeaponType(void);  // MLR who writes this shit? ----> weaponId???? { return (short)hardPoint[curHardpoint]->weaponId; };
    short GetCurrentWeaponIndex(void);
    float GetCurrentWeaponRangeFeet(void);
    void LaunchWeapon(void);
    void DetachWeapon(int hardpoint, SimWeaponClass *theWeapon);
    int CurStationOK(void)
    {
        return StationOK(curHardpoint);
    };
    int StationOK(int n);
    int NumHardpoints(void)
    {
        return numHardpoints;
    };
    int CurHardpoint(void)
    {
        return curHardpoint;
    };
    int  NumCurrentWpn(void)
    {
        return numCurrentWpn;
    };
    void SetCurHardpoint(int newPoint)
    {
        curHardpoint = newPoint;
    }; // This should go one day
    SimVehicleClass* Ownship(void)
    {
        return ownship;
    };
    MasterArmState MasterArm(void)
    {
        return masterArm;
    };
    void SetMasterArm(MasterArmState newState)
    {
        masterArm = newState;
    };
    void StepMasterArm(void);
    void StepCatIII(void);
    void ReplaceMissile(int, MissileClass*);
    void ReplaceRocket(int);
    void ReplaceBomb(int, BombClass*);

    void SetFlag(int newFlag)
    {
        flags or_eq newFlag;
    };
    void ClearFlag(int newFlag)
    {
        flags and_eq compl newFlag;
    };
    int  IsSet(int newFlag)
    {
        return flags bitand newFlag;
    };

    float GetWeaponRangeFeet(int hardpoint);

    void SelectBestWeapon(uchar *dam, int mt, int range_km, int guns_only = FALSE, int alt_feet = -1);  // 2002-03-09 MODIFIED BY S.G. Added the alt_feet variable so it knows the altitude of the target as well as it range

    MasterArmState masterArm; //MI moved from protected
    //MI
    bool BHOT;
    bool GndJett;
    bool FEDS;
    bool DrawFEDS;
    bool Powered; //for mav's
    float MavCoolTimer;
    enum MavSubModes { PRE, VIS, BORE};
    MavSubModes MavSubMode;
    void ToggleMavPower(void)
    {
        Powered = not Powered;
    };
    void StepMavSubMode(bool init = FALSE);
    bool JDAMPowered;//Cobra
    float JDAMInitTimer;//Cobra
    JDAMtargetingMode JDAMtargeting;//Cobra PB or TOO

    // RV - I-Hawk - HARM power functions
    bool GetHARMPowerState(void)
    {
        return HARMPowered;
    }
    float GetHARMInitTimer(void)
    {
        return HARMInitTimer;
    }
    void ToggleHARMPower(void)
    {
        HARMPowered = not HARMPowered;
    }

    int GetCurrentHardpoint(void)
    {
        return curHardpoint;
    };

protected:
    int numHardpoints;
    int curHardpoint;
    int numCurrentWpn;
    int flags;
    SimVehicleClass* ownship;

    // RV - I-Hawk - HARM power
    bool HARMPowered;
    float HARMInitTimer;
};

// ==================================================================
// SMSClass
//
// This is Leon's origional class, now used only for aircraft/helos
// ==================================================================
struct SMSAirGroundBombProfile
{
    int rippleCount;
    int rippleInterval;
    int fuzeNoseTail;
    int burstAltitude;
    int releaseAngle;
    float C1ArmDelay1, C1ArmDelay2, C2ArmDelay;
    //C1AD1, C1AD2, C2AD;
    bool releasePair;
    FireControlComputer::FCCSubMode subMode;
};



class SMSClass : public SMSBaseClass
{


#ifdef USE_SH_POOLS
public:
    // Overload new/delete to use a SmartHeap fixed size pool
    void *operator new(size_t size)
    {
        ShiAssert(size == sizeof(SMSClass));
        return MemAllocFS(pool);
    };
    void operator delete(void *mem)
    {
        if (mem) MemFreeFS(mem);
    };
    static void InitializeStorage()
    {
        pool = MemPoolInitFS(sizeof(SMSClass), 200, 0);
    };
    static void ReleaseStorage()
    {
        MemPoolFree(pool);
    };
    static MEM_POOL pool;
#endif

protected:

    char flash;
    int /* rippleInterval, pair, rippleCount,*/ curRippleCount;
    float burstHeight;
    unsigned long nextDrop;
    void ReleaseCurWeapon(int stationUnderTest);
    //void JettisonStation (int stationNum, int rippedOff = FALSE, );
    int JettisonStation(int stationNum, JettisonMode mode);
    void SetupHardpointImage(BasicWeaponStation *hp, int count);


    void RunRockets(void);
    void FireRocket(int hpId, BombClass *theLau);
    int runRockets;

public:
    int IsFiringRockets(void)
    {
        return runRockets;
    }
    int numOnBoard[wcNoWpn + 1];
    int curWpnNum;
    int lastWpnStation, lastWpnNum;
    WeaponType curWeaponType;
    WeaponDomain curWeaponDomain;
    WeaponClass curWeaponClass;
    SmsDrawable* drawable;
    //SimWeaponClass* curWeapon; // sfr: using smartpointer
    VuBin<SimWeaponClass> curWeapon;
    short curWeaponId;

    SMSClass(SimVehicleClass *newOwnship, short *weapId, uchar *weapCnt);
    virtual ~SMSClass();

    virtual void AddWeaponGraphics(void);
    virtual void FreeWeaponGraphics(void);
    virtual SimWeaponClass* GetCurrentWeapon(void)
    {
        return curWeapon.get();
    };

    void SetCurrentWeapon(int station, SimWeaponClass *weapon = 0);
    void SetWeaponType(WeaponType newMode);
    void IncrementStores(WeaponClass wClass, int count);
    void DecrementStores(WeaponClass wClass, int count);
    void SelectiveJettison(void);
    void EmergencyJettison(void);
    void JettisonWeapon(int hardpoint);
    void RemoveWeapon(int hardpoint);
    void AGJettison(void);
    int  DidEmergencyJettison(void)
    {
        return flags bitand EmergencyJettisonFlag;
    }
    int  DidJettisonedTank(void)
    {
        return flags bitand TankJettisonFlag;    // 2002-02-20 ADDED BY S.G. Helper to know if our tanks where jettisoned
    }
    void TankJettison(void);  // 2002-02-20 ADDED BY S.G. Will jettison the tanks (if empty) and set TankJettisonFlag
    int  WeaponStep(int symFlag = FALSE);
    int  FindWeapon(int indexDesired);
    int  FindWeaponClass(WeaponClass weaponDesired, int needWeapon = TRUE);
    int  FindWeaponType(WeaponType weaponDesired);
    int  DropBomb(int allowRipple = TRUE);
    int  LaunchMissile(void);
    int  LaunchRocket(void);
    void SetUnlimitedGuns(int flag);
    int UnlimitedAmmo(void)
    {
        return flags bitand UnlimitedAmmoFlag;
    };
    void SetUnlimitedAmmo(int newFlag);
    int  HasHarm(void)
    {
        return (flags bitand HTSOnBoard ? TRUE : FALSE);
    };
    int  HasLGB(void)
    {
        return (flags bitand LGBOnBoard ? TRUE : FALSE);
    };
    int  HasTrainable(void);
    int  HasSPJammer(void)
    {
        return (flags bitand SPJamOnBoard ? TRUE : FALSE);
    };
    int  HasWeaponClass(WeaponClass classDesired);
    void FreeWeapons(void);
    void Exec(void);
    void SetPlayerSMS(int flag);
    //void SetPair (int flag);
    void IncrementRippleCount(void);
    void DecrementRippleCount(void);
    void SetRippleInterval(int rippledistance);  // Marco Edit - for AI A2G
    void IncrementRippleInterval(void);
    void DecrementRippleInterval(void);
    void IncrementBurstHeight(void);
    void DecrementBurstHeight(void);
    void Incrementarmingdelay(void);//me123 status test. addet
    void ResetCurrentWeapon(void);
    //void SetRippleCount (int newVal); {rippleCount = newVal;}; // MLR 4/3/2004 -
    //int  RippleCount(void) {return rippleCount;};
    int  CurRippleCount(void)
    {
        return curRippleCount;
    }; // JB 010708
    //int  RippleInterval(void) {return rippleInterval;};
    //int  Pair(void) {return pair;};
    WeaponType GetNextWeapon(WeaponDomain);
    WeaponType GetNextWeaponSpecific(WeaponDomain);
    void SelectWeapon(WeaponType ntype, WeaponDomain domainDesired);
    void RemoveStore(int station, int storeId);
    void AddStore(int station, int storeId, int visible);
    void ChooseLimiterMode(int hardpoint);
    void RipOffWeapons(float noseAngle);
    float armingdelay;//me123 status ok. armingdelay addet
    int aim120id; // JPO Aim120 Id no.
    int AimId()
    {
        return aim120id + 1;
    };
    void NextAimId()
    {
        aim120id = (aim120id + 1) % 4;
    };
    enum Aim9Mode { WARM, COOLING, COOL, WARMING } aim9mode;
    Aim9Mode GetCoolState()
    {
        return aim9mode;
    };
    void SetCoolState(Aim9Mode state)
    {
        aim9mode = state;
    };
    // AIM9 time left for cooling
    /*VU_TIME aim9cooltime;
    // AIM9 cooling time left - approx. 3.5 hours worth
    VU_TIME aim9coolingtimeleft;
    // AIM9 - time to warm up after removing coolant
    VU_TIME aim9warmtime;*/
    float aim9cooltime;
    float aim9coolingtimeleft;
    float aim9warmtime;

    int    curProfile;
    struct SMSAirGroundBombProfile agbProfile[2];
    //MI
    //int Prof1RP, Prof2RP;
    //int Prof1RS, Prof2RS;
    //int Prof1NSTL, Prof2NSTL;
    //int C2BA;
    //int angle;
    //float C1AD1, C1AD2, C2AD;
    //bool Prof1Pair, Prof2Pair, Prof1;
    //FireControlComputer::FCCSubMode Prof1SubMode, Prof2SubMode;

    int  GetCurrentWeaponId(void);
    int  SetCurrentHpByWeaponId(int WeaponId); // used to find the next non-empty hardpoint carrying the the specified weaponID
    void StepAAWeapon(void); // MLR 2/8/2004 - revised function names, was StepWeaponClass
    void StepAGWeapon(void);  // MLR 2/8/2004 - new, to step AG weapons like AA weapons
    void StepWeaponByID(void); // MLR 1/31/2004 - was StepAAWeaponByID - now can step AG weapons aswell

    int  SetCurrentHardPoint(int hpId, int findSimilar = 1); // MLR 3/13/2004 - Sets the current HP, if the HP is empty, a similar HP may be found

    // AirGroundBomb Profile access
    void NextAGBProfile(void)
    {
        curProfile = (curProfile + 1) bitand 1;
    }
    void SetAGBProfile(int i)
    {
        curProfile = i;
    }

    int GetAGBRippleCount(void)
    {
        return agbProfile[curProfile].rippleCount;
    }
    int GetAGBRippleInterval(void)
    {
        return agbProfile[curProfile].rippleInterval;
    }
    int GetAGBFuze(void)
    {
        return agbProfile[curProfile].fuzeNoseTail;
    }
    int GetAGBBurstAlt(void)
    {
        return agbProfile[curProfile].burstAltitude;
    }
    int GetAGBReleaseAngle(void)
    {
        return agbProfile[curProfile].releaseAngle;
    }
    float GetAGBC1ArmDelay1(void)
    {
        return agbProfile[curProfile].C1ArmDelay1;
    }
    float GetAGBC1ArmDelay2(void)
    {
        return agbProfile[curProfile].C1ArmDelay2;
    }
    float GetAGBC2ArmDelay(void)
    {
        return agbProfile[curProfile].C2ArmDelay;
    }
    bool GetAGBPair(void)
    {
        return agbProfile[curProfile].releasePair;
    }
    FireControlComputer::FCCSubMode  GetAGBSubMode(void)
    {
        return agbProfile[curProfile].subMode;
    }

    void SetAGBRippleCount(int x)
    {
        agbProfile[curProfile].rippleCount    = x;
    }
    void SetAGBRippleInterval(int x)
    {
        agbProfile[curProfile].rippleInterval = x;
    }
    void SetAGBFuze(int x)
    {
        agbProfile[curProfile].fuzeNoseTail   = x;
    }
    void SetAGBBurstAlt(int x)
    {
        agbProfile[curProfile].burstAltitude  = x;
    }
    void SetAGBReleaseAngle(int x)
    {
        agbProfile[curProfile].releaseAngle   = x;
    }
    void SetAGBC1ArmDelay1(float x)
    {
        agbProfile[curProfile].C1ArmDelay1    = x;
    }
    void SetAGBC1ArmDelay2(float x)
    {
        agbProfile[curProfile].C1ArmDelay2    = x;
    }
    void SetAGBC2ArmDelay(float x)
    {
        agbProfile[curProfile].C2ArmDelay     = x;
    }
    void SetAGBPair(bool x)
    {
        agbProfile[curProfile].releasePair    = x;
    }
    void SetAGBSubMode(FireControlComputer::FCCSubMode SubMode)
    {
        agbProfile[curProfile].subMode = SubMode;
    }

    friend SmsDrawable;
};

VuBin<SimWeaponClass> InitWeaponList(
    FalconEntity* parent, ushort weapid, int weapClass, int num,
    SimWeaponClass* initFunc(FalconEntity* parent, ushort type, int slot),
    int *loadOrder = 0
);

#endif
