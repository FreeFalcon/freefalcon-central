#ifndef _ICP_H
#define _ICP_H

#include <cISO646>
#ifndef _WINDOWS_
#include <windows.h>
#endif

#include "stdhdr.h"
#include "fack.h"

// ================================= \\
// General Mode Information
// ================================= \\

#define ICP_NAV_BUTTON_ID 1012 //
#define ICP_AA_BUTTON_ID 1013 // These must match the values in the cockpit
#define ICP_AG_BUTTON_ID 1014 // script files...
#define ICP_ILS_BUTTON_ID 1015 //

#define ICP_MODE_NAME_LEN 7
//MI changed for ICP Stuff
//#define NUM_ICP_MODES 13
//#else
#define NUM_ICP_MODES 16
//#endif

extern char *ICPModeNames[NUM_ICP_MODES];

#define NONE_MODE 0
#define STPT_MODE 1
#define DLINK_MODE 2
#define MARK_MODE 3
#define ILS_MODE 4
#define CRUS_MODE 5
#define COMM1_MODE 6
#define COMM2_MODE 7
#define FACK_MODE 8
#define ALOW_MODE 9
#define NAV_MODE 10
#define AA_MODE 11
#define AG_MODE 12
//MI added for ICP Stuff
#define IFF_MODE 20
#define LIST_MODE 21
#define THREE_MODE 22
#define SIX_MODE 23
#define EIGHT_MODE 24
#define NINE_MODE 25
#define ZERO_MODE 26
#define CNI_MODE 27
#define UP_MODE 28
#define DOWN_MODE 29
#define CLEAR_MODE 30
#define SEQ_MODE 31
#define EWS_MODE 55

#define NONE_UPDATE 0x00
#define STPT_UPDATE 0x01
#define DLINK_UPDATE 0x02
#define MARK_UPDATE 0x04
#define ILS_UPDATE 0x08
#define CRUS_UPDATE 0x10
#define COMM_UPDATE 0x20
#define FACK_UPDATE 0x40
#define ALOW_UPDATE 0x80
#define CNI_UPDATE 0x100

#define NONE_BUTTON NONE_MODE
#define STPT_BUTTON STPT_MODE
#define DLINK_BUTTON DLINK_MODE
#define MARK_BUTTON MARK_MODE
#define ILS_BUTTON ILS_MODE
#define CRUS_BUTTON CRUS_MODE
#define COMM1_BUTTON COMM1_MODE
#define COMM2_BUTTON COMM2_MODE
#define FACK_BUTTON FACK_MODE
#define ALOW_BUTTON ALOW_MODE
#define NAV_BUTTON NAV_MODE
#define AA_BUTTON AA_MODE
#define AG_BUTTON AG_MODE
#define PREV_BUTTON AG_MODE + 1
#define NEXT_BUTTON AG_MODE + 2
#define ENTR_BUTTON AG_MODE + 3
//MI added for ICP Stuff
#define IFF_BUTTON IFF_MODE
#define LIST_BUTTON LIST_MODE
#define ONE_BUTTON ILS_MODE
#define TWO_BUTTON ALOW_MODE
#define THREE_BUTTON THREE_MODE
#define FOUR_BUTTON STPT_MODE
#define FIFE_BUTTON CRUS_MODE
#define SIX_BUTTON SIX_MODE
#define SEVEN_BUTTON MARK_MODE
#define EIGHT_BUTTON EIGHT_MODE
#define NINE_BUTTON NINE_MODE
#define ZERO_BUTTON ZERO_MODE
#define CNI_BUTTON CNI_MODE
#define UP_BUTTON UP_MODE
#define DOWN_BUTTON DOWN_MODE
#define CLEAR_BUTTON CLEAR_MODE
#define SEQ_BUTTON SEQ_MODE

#define MAX_DED_LEN 26
#define MAX_PFL_LEN 26

// ================================= \\
// Steerpoint Specific Information
// ================================= \\

#define ICP_TOS_STR 0
#define ICP_FT_STR 1
#define ICP_ETA_STR 2
#define ICP_KTS_STR 3

#define ICP_STPT_NAME_LEN 9
#define NUM_WAY_TYPES 3
#define NUM_ACTION_TYPES 31

#define MAX_PGMS 5 //Maximum number of EWS progs (4)


class AircraftClass;
class SimBaseClass;
class WayPointClass;
class ImageBuffer;

class CPButtonObject;


extern char *ICPWayPtNames[NUM_WAY_TYPES];
extern char *ICPWayPtActionTable[NUM_ACTION_TYPES];

typedef enum ICPWayPtType
{
    Way_STPT,
    Way_IP,
    Way_TGT
};

// ================================= \\
// Datalink Specific Information
// ================================= \\


#define NUM_DLINK_TYPES 4
extern char *ICPDLINKNames[NUM_DLINK_TYPES];

typedef enum ICPDLINKType
{
    DLink_IP,
    DLink_TGT,
    Dlink_EGR,
    Dlink_CP
};

// ================================= \\
// Mark Specific Information
// ================================= \\

class ICPClass
{

    // Pointers to the Outside World
    AircraftClass *mpOwnship;
    WayPointClass *mpWayPoints;


    // General Mode Variables
    long mICPPrimaryMode;
    long mICPSecondaryMode;
    long mICPTertiaryMode;
    char mpSelectedModeName[ICP_MODE_NAME_LEN];
    int mUpdateFlags;
    int mDirtyFlag;

    // Output Strings for DED
    char mpLine1[60];
    char mpLine2[60];
    char mpLine3[60];
    //MI added for ICP Stuff
    char TacanBand;
    char timeStr[16];

    // Pointer to Exclusive Buttons
    CPButtonObject *mpTertiaryExclusiveButton;
    CPButtonObject *mpSecondaryExclusiveButton;
    CPButtonObject *mpPrimaryExclusiveButton;

    // Steer Point Mode Specific
    WayPointClass *mpPreviousWayPt;
    int mNumWayPts;
    int mWPIndex;

    // Fault Mode Specific
    int mFaultNum;
    int mFaultFunc;

    // Mark Mode Specific
    int mMarkIndex;

    // DLink Mode Specific
    int mDLinkIndex;

    // Cruise Mode Specific
    int mCruiseMarkIndex;
    int mCruiseDLinkIndex;
    int mCruiseWPIndex;
    WayPointClass *mpCruiseWP;
    enum {STPT_LIST, MARK_LIST, DLINK_LIST} mList;

public:
    //Main Stuff
    //*****************
    //Override buttons*
    //*****************
    //Comms
    void ExecCOMMMode(void);
    void ENTRUpdateCOMMMode(void);
    void PNUpdateCOMMMode(int, int);
    void ExecCOMM1Mode(void);
    void ExecCOMM2Mode(void);
    //IFF
    void ExecIFFMode(void);
    //List + Misc
    void ExecLISTMode(void);
    void ExecMISCMode(void);

    //Main buttons
    void OneButton(int);
    void TwoButton(int);
    void ThreeButton(int);
    void FourButton(int);
    void FifeButton(int);
    void SixButton(int);
    void SevenButton(int);
    void EightButton(int);
    void NineButton(int);
    void ZeroButton(int);
    void HandleENTR(int);
    void HandlePrevNext(int, int);
    void CNISwitch(int);

    void ExecCNIMode(void);
    void ExecILSMode(void);
    BOOL GetCMDSTR(void)
    {
        return CMDSTRG;
    };
    int ILSPageSel;
    void ENTRUpdateILSMode(void);
    void PNUpdateILSMode(int, int);
    void ExecALOWMode(void);
    void PNUpdateALOWMode(int, int);
    void ExecSTPTMode(void);
    void PNUpdateSTPTMode(int, int);
    void CheckAutoSTPT(void);
    void ExecCRUSMode(void);
    void PNUpdateCRUSMode(int, int);
    void StepCruise(void);
    void FindEDR(long, char*);
    void ExecTimeMode(void);
    void ExecMARKMode(void);
    void PNUpdateMARKMode(int, int);
    void ENTRUpdateMARKMode(void);
    void ExecFIXMode(void);
    void ExecACALMode(void);
    void ExecFACKMode(void);
    void PNUpdateFACKMode(int, int);

    //List pages
    void ExecDESTMode(void);
    void ExecOA1Mode(void);
    void ExecOA2Mode(void);
    void ExecBingo(void);
    void ExecVIPMode(void);
    void ExecNAVMode(void);
    void ExecMANMode(void);
    void ExecINSMode(void);
    void ExecEWSMode(void);
    void ChaffPGM(void);
    void FlarePGM(void);
    void ExecMODEMode(void);
    void UpdateMODEMode(void);
    void ExecVRPMode(void);
    void ExecINTGMode(void);
    void ExecDLINKMode(void);
    void PNUpdateDLINKMode(int, int);

    //Misc pages
    void ExecCORRMode(void);
    void ExecMAGVMode(void);
    void ExecOFPMode(void);
    void ExecINSMMode(void);
    void ExecLASRMode(void);
    void ExecGPSMode(void);
    void ExecDRNGMode(void);
    void ExecBullMode(void);
    void ExecWPTMode(void);
    void ExecHARMMode(void);
    void ExecWinAmpMode(void); // Retro 3Jan2004
    BOOL CheckForHARM(void);
    bool ShowBullseyeInfo;
    bool transmitingvoicecom1;
    bool transmitingvoicecom2;

    //Supporting functions
    void InitStuff(void);
    void ClearStrings(void);
    void ClearFlags(void);
    void HandleManualInput(int button);
    void ClearInput(void);
    int CheckMode(void);
    void EnterLat(void);
    void EnterLong(void);
    void EnterALOW(void);
    void EnterTCN(void);
    void EnterBingo(void);
    void PushedSame(int);
    void NewMode(int);
    int ManualInput(void);
    void CNIBackup(void);
    void ScratchPad(int Line, int Start, int End);
    void MakeInverted(int Line, int Start, int End);
    void FormatTime(long, char*);
    void FormatRadioString(void);
    void FillDEDMatrix(int Line, int Start, char*, int Inverted = 0);
    void LeaveCNI(void);
    void ClearDigits(void);
    void ClearInverted(int Line, int Start, int End);
    void ClearString(void);
    void CheckDigits(void);
    void ResetSubPages(void);
    void ICPEnter(void);
    void FakeILSFreq(void);
    void ResetInput(void);
    void WrongInput(void);
    int CheckBackupPages(void);
    void LeaveCNIPage(void);

    //STPT and DEST stuff
    float latitude, cosLatitude, longitude;
    int latDeg, longDeg, heading;
    float latMin, longMin, xCurr, yCurr, zCurr;
    char latStr[40];
    char longStr[40];
    bool ShowWind, OA1, OA2, MAN;
    void EnterOA(void);
    float windSpeed;

    void GetValues(float Angle, float *a, float *b, int Range);

    //Cruise page stuff
    bool Cruise_RNG, Cruise_HOME, Cruise_EDR, Cruise_TOS;
    void CruiseRNG(void);
    void CruiseEDR(void);
    void CruiseHOME(void);
    void CruiseTOS(void);
    void GetWind(void);
    void AddSTPT(int Line, int Pos);
    int HomeWP, RangeWP, TOSWP;
    void StepHOMERNGSTPT(int mode);
    int GetHOMERNGSTPTNum(int var, int mode);
    int CruiseMode;
    int GetCruiseIndex(void)
    {
        return CruiseMode;
    };
    void SetCruiseIndex(int NewIndex)
    {
        CruiseMode = NewIndex;
    };

    //Time page
    bool running;
    bool stopped;
    VU_TIME Start, Difference;
    //PFL stuff
    void ChangeToCNI(void);
    bool m_FaultDisplay; // should we show a fault
    FaultClass::type_FSubSystem m_subsystem; // current fault and function
    int m_function;
    void PflFault(FaultClass::type_FSubSystem sys, int func);
    void PflNoFaults();
    void ExecPfl();


    //EWS page
    bool EWSMain, PGMChaff, PGMFlare, EWS_JAMMER_ON, EWS_BINGO_ON;
    void EWSOnOff(void);
    void EWSEnter(void);
    void EnterBurst(void);
    void EnterSalvo(void);
    unsigned int FlareBingo, ChaffBingo;
    bool BQ, BI, SQ, SI;
    //unsigned int iCHAFF_BQ, iCHAFF_SQ, iFLARE_BQ, iFLARE_SQ;
    unsigned int iCHAFF_BQ[MAX_PGMS], iCHAFF_SQ[MAX_PGMS], iFLARE_BQ[MAX_PGMS], iFLARE_SQ[MAX_PGMS];
    float fCHAFF_BI[MAX_PGMS], fCHAFF_SI[MAX_PGMS], fFLARE_BI[MAX_PGMS], fFLARE_SI[MAX_PGMS];
    int CPI, FPI; //chaff and Flare program index
    void StepEWSProg(int mode);
    void ShowFlareIndex(int Line, int Pos);
    void ShowChaffIndex(int Line, int Pos);


    //Offset Aimpoints
    bool OA_RNG, OA_BRG, OA_ALT;
    unsigned int iOA_RNG, iOA_ALT, iOA_RNG2, iOA_ALT2;
    float fOA_BRG, fOA_BRG2;
    void SetOA(void);
    //VIP
    bool VIP_RNG, VIP_BRG, VIP_ALT;
    unsigned int iVIP_RNG, iVIP_ALT;
    float fVIP_BRG;
    void EnterVIP(void);
    void SetVIP(void);
    //VRP
    bool VRP_RNG, VRP_BRG, VRP_ALT;
    unsigned int iVRP_RNG, iVRP_ALT;
    float fVRP_BRG;
    void EnterVRP(void);
    void SetVRP(void);



    //MODE page
    bool IN_AG, IN_AA, AA_SELECT, IsSelected;

    bool Manual_Input;
    unsigned int tempvar;
    float tempvar1;
    int AddUp(void);
    float AddUpFloat(void);
    long AddUpLong(void);
    unsigned int InputsMade, PossibleInputs;

    //Backup pages
    void UHFBackup(void);
    void VHFBackup(void);
    void IFFBackup(void);
    void ILSBackup(void);

    //for flashing things
    unsigned int flash;
public:

    int mIdNum;
    int mCycleBits;

    char DEDLines[5][MAX_DED_LEN];
    char Invert[5][MAX_DED_LEN];
    char tempstr[40];
    char InputString[20];

    //PFL
    char PFLLines[5][MAX_PFL_LEN];
    char PFLInvert[5][MAX_PFL_LEN];
    void FillPFLMatrix(int Line, int Start, char*, int Inverted = 0);
    void ClearPFLLines(void);

    float PREUHF, PREVHF;
    double UHFChann, VHFChann, MagVar;

    //Variables that hold our Flags and stuff
    unsigned int ICPModeFlags, LastMode, ILSOn;
    //Manual Input stuff
    unsigned int CommChannel, TacanChannel, CurrChannel, Digit1, Digit2, Digit3;
    unsigned int Input_Digit1, Input_Digit2, Input_Digit3, Input_Digit4, Input_Digit5, Input_Digit6, Input_Digit7;
    float LATDegrees, LATMinutes, LATSeconds, LONGDegrees, LONGMinutes, LONGSeconds;
    float Lat, Long, SetLat, SetLong, cosLat, WPAlt;
    int ClearCount, HSICourse, WhichRadio;
    bool MadeInput, CMDSTRG;

    //Manual Wspan stuff
    float ManWSpan, ManualWSpan;
    void EnterWSpan(void);

    long ManualBingo, ManualALOW, level, total;
    bool EDITMSLFLOOR, TFADV;
    char Freq[7];

    //Flagdefinitions
    enum ModeFlags
    {
        //Master Modes
        MODE_A_A = 0x1,
        MODE_A_G = 0x2,
        MODE_LIST = 0x4,
        MISC_MODE = 0x8,
        MODE_IFF = 0x10,
        MODE_COMM1 = 0x20,
        MODE_COMM2 = 0x40,
        MODE_CNI = 0x80,
        MODE_DLINK = 0x100,
        BLOCK_MODE = 0x200,
        CHAFF_BINGO = 0x400,
        FLARE_BINGO = 0x800,
        FLASH_FLAG = 0x1000,
        MODE_FACK = 0x2000,
        EDIT_LAT = 0x4000,
        EDIT_LONG = 0x8000,
        EDIT_JAMMER = 0x10000,
        EWS_EDIT_BINGO  = 0x20000,
        EDIT_VHF = 0x40000,
        EDIT_UHF = 0x80000,
        EDIT_STPT = 0x100000,
    };

    //Functions for our Flags
    void SetICPFlag(int newFlag)
    {
        ICPModeFlags or_eq newFlag;
    };
    void ClearICPFlag(int newFlag)
    {
        ICPModeFlags and_eq compl newFlag;
    };
    int IsICPSet(int testFlag)
    {
        return ICPModeFlags bitand testFlag ? 1 : 0;
    };

    //IFF Stuff
    unsigned int IFFModes;
    enum IFFFlags
    {
        MODE_1 = 0x1,
        MODE_2 = 0x2,
        MODE_3 = 0x4,
        MODE_4 = 0x8,
        MODE_4B = 0x10,
        MODE_4OUT = 0x20,
        MODE_4LIT = 0x40,
        MODE_C = 0x80,
    };
    void SetIFFFlag(int newFlag)
    {
        IFFModes or_eq newFlag;
    };
    void ClearIFFFlag(int newFlag)
    {
        IFFModes and_eq compl newFlag;
    };
    int IsIFFSet(int testFlag)
    {
        return IFFModes bitand testFlag ? 1 : 0;
    };
    void ToggleIFFFlag(int Flag)
    {
        IFFModes xor_eq Flag;
    };
    void FillIFFString(char *string);

    void EnterINTG(void);
    int Mode1Code, Mode2Code, Mode3Code;
    float GetNumScans(void);

    //INS stuff
    float INSTime;
    int INSLine; //which line we are editing
    char INSLong[20];
    char INSLat[20];
    void GetINSInfo(void);
    char altStr[10];
    char INSHead[10];
    float INSLATDiff, INSLONGDiff, INSALTDiff;
    float StartLat, StartLong;
    bool EnteredHDG;
    bool INSEnter;
    int INSHDGDiff;
    int INSEnterPush(void)
    {
        return INSEnter;
    };
    void ClearINSEnter(void)
    {
        INSEnter = FALSE;
    };
    void EnterINSStuff(void);
    bool FillStrings;

    //Laser Page
    int LaserTime;
    int LaserLine;
    int LaserCode;
    void EnterLaser(void);


    //bool ShowWindNow(void) {return ShowWind;};

    ICPClass();
    ~ICPClass();
    void Exec();
    void HandleInput(int, CPButtonObject *);
    void DisplayBlit(void);
    void DisplayDraw(void);
    void SetOwnship(void);
    CPButtonObject* GetTertiaryExclusiveButton(void);
    // void SetSecondaryExclusiveButton(CPButtonObject *);
    CPButtonObject* GetSecondaryExclusiveButton(void);
    // void SetPrimaryExclusiveButton(CPButtonObject *);
    void InitPrimaryExclusiveButton(CPButtonObject *);
    void InitTertiaryExclusiveButton(CPButtonObject *);
    CPButtonObject* GetPrimaryExclusiveButton(void);
    void SetDirtyFlag(void)
    {
        mDirtyFlag = TRUE;
    };
    void GetDEDStrings(char*, char*, char*);
    long GetICPPrimaryMode(void)
    {
        return mICPPrimaryMode;
    };
    long GetICPSecondaryMode(void)
    {
        return mICPSecondaryMode;
    };
    long GetICPTertiaryMode(void)
    {
        return mICPTertiaryMode;
    };
    void SetICPTertiaryMode(long mode)
    {
        mICPTertiaryMode = mode;
    };
    int  GetICPWPIndex(void)
    {
        return mWPIndex;
    };
    void SetICPWPIndex(int newWp)
    {
        mWPIndex = newWp;
    };
    void SetICPUpdateFlag(int newFlag)
    {
        mUpdateFlags or_eq newFlag;
    };

    void SetICPSecondaryMode(int num)
    {
        mICPSecondaryMode = num;
    };

    //MI for volume control in the cockpit
    int Comm1Volume, Comm2Volume;
};
#endif
