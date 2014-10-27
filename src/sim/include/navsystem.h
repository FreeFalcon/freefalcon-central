#ifndef _NAVSYSTEM_H
#define _NAVSYSTEM_H

#include "stdhdr.h"
#include "simdrive.h"
#include "f4error.h"
#include "tacan.h"
#include "msginc/dlinkmsg.h"

extern char* mpPointTypeNames[];

const int MAX_MARKPOINTS = 10;
const int MAX_DLINKPOINTS = 5;

//MI OA stuff
const int MAX_DESTOA = 3;
const int MAX_VIPOA = 2;
const int MAX_VRPOA = 2;


extern float FALCON_ORIGIN_LAT, FALCON_ORIGIN_LONG;
void SetLatLong(float latitude, float longitude);
void ResetLatLong(void);
void GetLatLong(float *latitude, float *longitude);

class TacanList;
class ObjectiveClass;
class AircraftClass;

class NavigationSystem
{
public:

    typedef enum UHF_Mode_Type
    {
        UHF_NORM,
        UHF_BACKUP
    } UHF_Mode_Type;

    typedef enum Point_Type
    {
        NODATA,
        GMPOINT,
        POS
    };

    typedef struct Mark_Struct
    {
        Point_Type pointType;
        char pLatStr[40];
        char pLongStr[40];
        WayPointClass* pWaypoint;
    } Mark_Struct;

    //MI OA Stuff
    typedef struct DEST_OA_Struct
    {
        Point_Type pointType;
        char pLatStr[40];
        char pLongStr[40];
        WayPointClass* pWaypoint;
    } DEST_OA_Struct;

    typedef struct VIP_OA_Struct
    {
        Point_Type pointType;
        char pLatStr[40];
        char pLongStr[40];
        WayPointClass* pWaypoint;
    } VIP_OA_Struct;

    typedef struct VRP_OA_Struct
    {
        Point_Type pointType;
        char pLatStr[40];
        char pLongStr[40];
        WayPointClass* pWaypoint;
    } VRP_OA_Struct;

    typedef struct DLink_Struct
    {
        FalconDLinkMessage::DLinkPointType pointType;
        WayPointClass* pWaypoint;
        char pTypeStr[5];
        char pTarget[15]; // class pointer, primary target
        char pThreat[15]; // class pointer, primary threat
        char attackHeading[4];
        char distance[5];
    } DLinkStruct;

    typedef enum Type
    {
        AIRBASE,
        CARRIER,
        TANKER,
        TOTAL_TYPES
    };

    // Storage for Tacan Data
    typedef enum Tacan_Channel_Src
    {
        ICP,
        AUXCOMM,
        TOTAL_SRC
    };

    typedef enum Instrument_Mode
    {
        NAV,
        ILS_NAV,
        ILS_TACAN,
        TACAN,
        TOTAL_MODES
    };

    typedef enum Attribute
    {
        X_POS,
        Y_POS,
        Z_POS,
        RWY_NUM,
        GP_DEV,
        GS_DEV,
        AIRBASE_ID,
        RANGE,
        ILSFREQ,
        TOTAL_ATTRIBUTES
    };

    typedef struct Tacan_Data_Str
    {
        int channel;
        int digits[3]; //Element #2 = MSB, Element #0 = LSB
        TacanList::StationSet set;
        VU_ID vuID;
        int range;
        int ttype;
        float ilsfreq;
    } Tacan_Data_Str;

    typedef struct Ils_Data_Str
    {
        float frontx;
        float fronty;
        float backx;
        float backy;
        float z;
        int rwyidx;
        char rwynum[4];
        int heading;
        float sinHeading;
        float cosHeading;
        float gpDeviation;
        float gsDeviation;
        VU_ID vuID; // id of airbase
        ObjectiveClass* pobjective;
    } Ils_Data_Str;

    typedef struct Tacan_Data_LL_Str
    {
        Tacan_Data_LL_Str *pNext;
        Tacan_Data_LL_Str *pPrevious;
        Tacan_Data_Str *pData;
        Type type;
    } Tacan_Data_LL_Str;

private:
    //---------------------------------------------------------------
    // Storage for UHF Modes
    //---------------------------------------------------------------

    UHF_Mode_Type mUHFMode;

    //---------------------------------------------------------------
    // Storage for Datalink and Markpoint lists
    //---------------------------------------------------------------

    int mCurrentMark;
    Mark_Struct mpMarkPoints[MAX_MARKPOINTS];

    int mCurrentDLink;
    DLink_Struct mpDLinkPoints[MAX_DLINKPOINTS];

    //MI OA Stuff
    int mCurrentDESTOA;
    DEST_OA_Struct mpDESTOA[MAX_DESTOA];
    int mCurrentVIPOA;
    VIP_OA_Struct mpVIPOA[MAX_VIPOA];
    int mCurrentVRPOA;
    VRP_OA_Struct mpVRPOA[MAX_VRPOA];


    char mpDLinkThreat[15];
    char mpDLinkTarget[15];

    //---------------------------------------------------------------
    // Control Variables and Lists
    //---------------------------------------------------------------

    Instrument_Mode mInstrumentMode; // (Instrument Mode Sel Switch)
    Tacan_Channel_Src mCurrentTCNSrc; // (AUXCOMM Master Switch)
    Tacan_Data_Str mpCurrentTCN[TOTAL_SRC]; // Tacan station that the ICP and AUXCOMM currently point to
    TacanList::Domain mpCurrentDomain[TOTAL_SRC]; // (AUXCOMM TR/AA_TR Switch and ICP Tacan Type)

    Ils_Data_Str mpCurrentIls;

    //---------------------------------------------------------------
    // Station list held by the ICP
    //---------------------------------------------------------------

    Tacan_Data_LL_Str* mpMissionTacans; // Stations that are preset at the start of mission, info found from waypoint list
    Tacan_Data_LL_Str* mpCurrentMissionTacan;

    //---------------------------------------------------------------
    // Utility Functions
    //---------------------------------------------------------------

    void CopyMissionTCNData(Tacan_Data_Str*, Tacan_Data_LL_Str*); // Just a conveience function
    void FindTacanStation(Tacan_Channel_Src tsource, int tacan_no, TacanList::StationSet xy,
                          VU_ID* object, int *rangep, int *type, float *ilsfreq); // Nice wrapper interface for searching
    // the TacanList, when TacanList
    // changes its search interface we
    // only have to change the guts of // this function
public:

    //---------------------------------------------------------------
    // Constructors and Destructors
    //---------------------------------------------------------------

    NavigationSystem();
    ~NavigationSystem();


    //---------------------------------------------------------------
    // UHF stuff
    //---------------------------------------------------------------
    void ToggleUHFSrc(void);
    UHF_Mode_Type GetUHFSrc(void);
    void                        SetUHFSrc(UHF_Mode_Type);
    //---------------------------------------------------------------
    // Get the values of the Tacan Station and ILS Data
    //---------------------------------------------------------------

    BOOL GetTCNAttribute(Attribute, float*);
    BOOL GetTCNPosition(float*xpos, float *ypos, float *zpos);
    BOOL GetILSAttribute(Attribute, VU_ID*);
    BOOL GetILSAttribute(Attribute, float*);
    BOOL GetILSAttribute(Attribute, char*); //Pass pointer to char[4], function will fill

    //---------------------------------------------------------------
    // Initialize the Mission Pre-set Tacan List
    //---------------------------------------------------------------

    void SetMissionTacans(AircraftClass*);
    void DeleteMissionTacans(void);

    //---------------------------------------------------------------
    // Control Switches and buttons
    //---------------------------------------------------------------

    void SetDomain(Tacan_Channel_Src, TacanList::Domain);
    void SetDomain(Type);
    TacanList::Domain GetDomain(Tacan_Channel_Src);
    TacanList::Domain ToggleDomain(Tacan_Channel_Src); // Returns new state
    void ToggleControlSrc(void);
    Tacan_Channel_Src GetControlSrc(void);
    void                        SetControlSrc(Tacan_Channel_Src);

    //---------------------------------------------------------------
    // Overall Instrument Mode
    //---------------------------------------------------------------

    void SetInstrumentMode(Instrument_Mode);
    Instrument_Mode GetInstrumentMode(void);
    void StepInstrumentMode(void);


    //---------------------------------------------------------------
    // ILS Functions
    //---------------------------------------------------------------
    void SetIlsData(VU_ID, int);
    void ExecIls(void);

    //---------------------------------------------------------------
    // Mark and DLink Functions
    //---------------------------------------------------------------

    void GetMarkPoint(Point_Type*, char*, char*);
    void GetMarkWayPoint(WayPointClass**);
    void GetMarkWayPoint(int, WayPointClass**);
    int GetMarkIndex(void);
    void SetMarkPoint(Point_Type, float, float, float, long);
    void GotoPrevMark(void);
    void GotoNextMark(void);

    void GetDataLink(FalconDLinkMessage::DLinkPointType*, int*, char*, char*, char*, char*, char*);
    void SetDataLinks(char, USHORT, USHORT, FalconDLinkMessage::DLinkPointType*, short*, short*, short*, long*);
    void GetDLinkWayPoint(WayPointClass**);
    void GetDLinkWayPoint(int, WayPointClass**);
    void GotoPrevDLink(void);
    void GotoNextDLink(void);
    int GetDLinkIndex(void);

    //MI OA Stuff
    void GetDESTOA(Point_Type*, char*, char*);
    void GetDESTOAPoint(WayPointClass**);
    void GetDESTOAPoint(int, WayPointClass**);
    int GetDESTOAIndex(void);
    void SetDESTOAPoint(Point_Type, float, float, float, int);
    void GotoPrevDESTOA(void);
    void GotoNextDESTOA(void);

    void GetVIPOA(Point_Type*, char*, char*);
    void GetVIPOAPoint(WayPointClass**);
    void GetVIPOAPoint(int, WayPointClass**);
    int GetVIPOAIndex(void);
    void SetVIPOAPoint(Point_Type, float, float, float, long);

    void GetVRPOA(Point_Type*, char*, char*);
    void GetVRPOAPoint(WayPointClass**);
    void GetVRPOAPoint(int, WayPointClass**);
    int GetVRPOAIndex(void);
    void SetVRPOAPoint(Point_Type, float, float, float, long);

    //---------------------------------------------------------------
    // Tacan Station Functions
    //---------------------------------------------------------------

    void SetTacanChannel(Tacan_Channel_Src, int, int); // For setting a single digit
    int GetTacanChannel(Tacan_Channel_Src, int); // For getting a single digit

    void SetTacanChannel(Tacan_Channel_Src, int); // For setting the full channel
    int GetTacanChannel(Tacan_Channel_Src); // For getting the full channel

    void SetTacanChannel(Tacan_Channel_Src, int, TacanList::StationSet); // For setting everything
    void GetTacanChannel(Tacan_Channel_Src, int*, TacanList::StationSet*); // For getting everything

    void SetTacanBand(Tacan_Channel_Src, TacanList::StationSet); // For setting the band
    TacanList::StationSet GetTacanBand(Tacan_Channel_Src); // For getting the band

    void GetTacanVUID(Tacan_Channel_Src, VU_ID*); // For getting the Id of the tacan object

    void StepTacanChannelDigit(Tacan_Channel_Src, int, int dir = 1);
    void StepTacanBand(Tacan_Channel_Src);

    void StepPreviousTacan(void); // For ICP List only
    void StepNextTacan(void); // For ICP List only
    void GetHomeID(VU_ID*); // For ICP List only
    void GetCurrentID(VU_ID*); // For ICP List only
    Type GetType(void); // For ICP List only

    void SetIlsFromTacan(); // MD -- 20040605: for uncoupling ILS activation and ATC radio calls
    //---------------------------------------------------------------
    // Miscellaneous Functions
    //--------------------------------------------------------------

    BOOL IsTCNTanker(void);
    BOOL IsTCNCarrier(void);
    BOOL IsTCNAirbase(void);
    void GetAirbase(VU_ID*); // Simple call for Dave's ATC Brain

    //MI to get ILS stuff
    void GetILSData(float *LocDev, float *finalHeading, float *finalGS, float *DistToSta);
    int GetCurTCNRange()
    {
        return mpCurrentTCN[mCurrentTCNSrc].range;
    };
    int GetCurTCNType()
    {
        return mpCurrentTCN[mCurrentTCNSrc].ttype;
    };
    float GetCurTCNILS()
    {
        return mpCurrentTCN[mCurrentTCNSrc].ilsfreq;
    };

    //MI
protected:
    float xfrontdiff;
    float yfrontdiff;
    float xbackdiff;
    float ybackdiff;
    float zdiff;
    float bearingToLocalizer;
    float distToToLocalizer;
    float el;
    float approach;
};


extern NavigationSystem* gNavigationSys;

extern char* Point_Type_Str[7];


//====================================================
// Utility Functions
//====================================================

extern float ConvertNavtoRad(float);
extern float ConvertRadtoNav(float);
extern void BuildLatLongStr(float, float, char*, char*);
extern void ApproxLatLong(float, float, float*, float*);

#endif
