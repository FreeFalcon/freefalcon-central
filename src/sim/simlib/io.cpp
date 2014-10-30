/******************************************************************************/
/*                                                                            */
/*  Unit Name : io.cpp                                                        */
/*                                                                            */
/*  Abstract  : Source file for functions implementing the SIMLIB_IO_CLASS.   */
/*                                                                            */
/*  Dependencies : Auto-Generated                                             */
/*                                                                            */
/*  Operating System : MS-DOS 6.2, Windows 3.1                                */
/*                                                                            */
/*  Compiler : MSVC V1.5                                                      */
/*                                                                            */
/*  Naming Conventions :                                                      */
/*                                                                            */
/*      Public Functions    : Mixed case with no underscores                  */
/*      Private Functions   : Mixed case with no underscores                  */
/*      Public Functions    : Mixed case with no underscores                  */
/*      Global Variables    : Mixed case with no underscores                  */
/*      Classless Functions : Mixed case with no underscores                  */
/*      Classes             : All upper case seperated by an underscore       */
/*      Defined Constants   : All upper case seperated by an underscore       */
/*      Macros              : All upper case seperated by an underscore       */
/*      Structs/Types       : All upper case seperated by an underscore       */
/*      Private Variables   : All lower case seperated by an underscore       */
/*      Public Variables    : All lower case seperated by an underscore       */
/*      Local Variables     : All lower case seperated by an underscore       */
/*                                                                            */
/*  Development History :                                                     */
/*  Date      Programer           Description                                 */
/*----------------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                               */
/*                                                                            */
/******************************************************************************/

/***************************************************************************/
// Retro 31Dec2003
// Major major rewrite
//
/***************************************************************************/
#include "stdhdr.h"
#include "simio.h"
#include "f4find.h"

#include "mouselook.h" // Retro 18Jan2004

extern int g_nIdleCutoffPad;

AxisMapping AxisMap;
AxisCalibration AxisShapes;

/*----------------------------------------------------*/
/* Memory Allocation for externals declared elsewhere */
/*----------------------------------------------------*/
SIMLIB_IO_CLASS   IO;

/*****************************************************************************/
/* constructor, by Retro */
/*****************************************************************************/
SIMLIB_IO_CLASS::SIMLIB_IO_CLASS()
{
    for (int i = 0; i < AXIS_MAX; i++)
    {
        analog[i].engrValue = 0.F;
        analog[i].isUsed = false;
        analog[i].center = 0;
        analog[i].cutoff = 15000;
        analog[i].ioVal = 0;
        analog[i].isReversed = false;
        analog[i].smoothingFactor = 0; // Retro 19Feb2004
    }

    mouseWheelPresent = false;
    idleCutoffPad = g_nIdleCutoffPad;
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_INT SIMLIB_IO_CLASS::Init (SIM_FILE_NAME)           */
/*                                                                  */
/* Description:                                                     */
/*    Initialize the I/O subsystem and read in any old calibration  */
/*    data.                                                         */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_FILE_NAME fname - pathname of the file to be opened.      */
/*                                                                  */
/* Outputs:                                                         */
/*    SIMLIB_OK for success, SIMLIB_ERR for failure.                */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_INT SIMLIB_IO_CLASS::Init(char*)
{
    // Retro:
    // called on entering the 3d from the ui.. dunno..
    // ..the original code checked (through mmsystem.h - ugh) if a stick was connected..
    for (int i = 0; i < AXIS_MAX; i++)
    {
        analog[i].engrValue  = 0;
        analog[i].ioVal = 0;
    }

    for (int i = 0; i < SIMLIB_MAX_DIGITAL * SIM_NUMDEVICES; i++)
    {
        digital[i] = FALSE;
    }

    return SIMLIB_OK;
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_FLOAT SIMLIB_IO_CLASS::ReadAnalog  (SIM_INT)        */
/*                                                                  */
/* Description:                                                     */
/*    Get the value of a particular analog input channel            */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_INT id - I/O Channel number                               */
/*                                                                  */
/* Outputs:                                                         */
/*    Scaled output value for the desired channel                   */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_FLOAT SIMLIB_IO_CLASS::ReadAnalog(GameAxis_t id)
{
    // Retro: this returns a 'normalized' float of an in-game axis
    // only for pitch/bank/yaw/throttle, uses original MPS algorithms (mostly :p)
    // values range from -1..1 or 0..1.5 depending on axis
    return (analog[id].engrValue);
}

/*****************************************************************************/
// Retro 28Dec2003
// returns integer value for ALL axis, range from -10000...10000 or
// 0...15000 depending on axis
/*****************************************************************************/
SIM_INT SIMLIB_IO_CLASS::GetAxisValue(GameAxis_t id)
{
    return (analog[id].ioVal);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_INT SIMLIB_IO_CLASS::ReadDigital (SIM_INT)          */
/*                                                                  */
/* Description:                                                     */
/*    Get the value of a particular digital input                   */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_INT id - channel to read                                  */
/*                                                                  */
/* Outputs:                                                         */
/*    TRUE if pushed, FALSE it not.                                 */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_INT SIMLIB_IO_CLASS::ReadDigital(SIM_INT id)
{
    return (digital[id]);
}

/***************************************************************************/
// Reads the 'soft' properties of an in-game axis. (non DX-related)
// only variables of interest: center-offset (==abdetent for throttle) and
// 'isReversed'-flag
// I guess ultimately that should be merged into the AxisMap struct
/***************************************************************************/
int SIMLIB_IO_CLASS::ReadFile(void)
{
    size_t success = 0;
    char path[_MAX_PATH];
    long size;
    SIMLIB_ANALOG_TYPE temp[SIMLIB_MAX_ANALOG];
    FILE *fp;

    sprintf(path, "%s\\config\\joystick.cal", FalconDataDirectory);

    fp = fopen(path, "rb");

    if ( not fp)
        return FALSE;

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size not_eq sizeof(SIMLIB_ANALOG_TYPE)*SIMLIB_MAX_ANALOG)
        return FALSE;

    success = fread(temp, sizeof(SIMLIB_ANALOG_TYPE), SIMLIB_MAX_ANALOG, fp);
    fclose(fp);

    if (success not_eq SIMLIB_MAX_ANALOG)
        return FALSE;

    for (int i = 0; i < SIMLIB_MAX_ANALOG; i++)
    {
        analog[i].center = temp[i].center;
        analog[i].cutoff = temp[i].cutoff;
        analog[i].isReversed = temp[i].isReversed;
        analog[i].smoothingFactor = temp[i].smoothingFactor; // Retro 19Feb2004
    }

    return TRUE;
}

/***************************************************************************/
// Saves the 'soft' properties of an in-game axis. (non DX-related)
// only variables of interest: center-offset (==abdetent for throttle) and
// 'isReversed'-flag
// I guess ultimately that should be merged into the AxisMap struct
/***************************************************************************/
int SIMLIB_IO_CLASS::SaveFile(void)
{
    size_t success = 0;
    char path[_MAX_PATH];
    FILE *fp;

    sprintf(path, "%s\\config\\joystick.cal", FalconDataDirectory);

    fp = fopen(path, "wb");

    if ( not fp)
        return FALSE;

    success = fwrite(analog, sizeof(SIMLIB_ANALOG_TYPE), SIMLIB_MAX_ANALOG, fp);
    fclose(fp);

    if (success not_eq SIMLIB_MAX_ANALOG)
        return FALSE;

    return TRUE;
}

/*****************************************************************************/
// Reads the physical properties of an in-game axis, like deadzone, saturation,
// 'real' device number and axis associated with that in-game axis
// Validation of those read values is done later (in siloop.cpp)
/*****************************************************************************/
int SIMLIB_IO_CLASS::ReadAxisMappingFile()
{
    size_t success = 0;
    AxisMapping temp;
    long size;
    FILE* fp;

    char path[_MAX_PATH];
    sprintf(path, "%s\\config\\axismapping.dat", FalconDataDirectory);

    fp = fopen(path, "rb");

    if ( not fp)
        return FALSE;

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size not_eq sizeof(AxisMapping))
        return FALSE;

    success = fread(&temp, sizeof(AxisMapping), 1, fp);
    fclose(fp);

    if (success not_eq 1)
        return FALSE;

    AxisMap = temp;

    return TRUE;
}

/*****************************************************************************/
// Saves the physical properties of an in-game axis, like deadzone, saturation,
// 'real' device number and axis associated with that in-game axis
/*****************************************************************************/
int SIMLIB_IO_CLASS::WriteAxisMappingFile()
{
    size_t success = 0;
    char path[_MAX_PATH];
    FILE *fp;

    sprintf(path, "%s\\config\\axismapping.dat", FalconDataDirectory);

    fp = fopen(path, "wb");

    if ( not fp)
        return FALSE;

    SaveGUIDAndCount();

    success = fwrite(&AxisMap, sizeof(AxisMapping), 1, fp);
    fclose(fp);

    if (success not_eq 1)
        return FALSE;

    return TRUE;
}

/*****************************************************************************/
// Retro 2Jan2004
/*****************************************************************************/
void ResetDeviceAxis(DeviceAxis* t)
{
    ShiAssert(t);
    t->Device = t->Axis = t->Saturation = -1;
    t->Deadzone = 100;
}

/*****************************************************************************/
// Should reset all variable connected to the SIMLIB_IO_CLASS class
/*****************************************************************************/
void SIMLIB_IO_CLASS::Reset()
{
    ResetDeviceAxis(&AxisMap.Pitch);
    ResetDeviceAxis(&AxisMap.Bank);
    ResetDeviceAxis(&AxisMap.Yaw);
    ResetDeviceAxis(&AxisMap.Throttle);
    ResetDeviceAxis(&AxisMap.Throttle2);
    ResetDeviceAxis(&AxisMap.BrakeLeft);
    ResetDeviceAxis(&AxisMap.BrakeRight);
    ResetDeviceAxis(&AxisMap.FOV);
    ResetDeviceAxis(&AxisMap.PitchTrim);
    ResetDeviceAxis(&AxisMap.YawTrim);
    ResetDeviceAxis(&AxisMap.BankTrim);
    ResetDeviceAxis(&AxisMap.AntElev);
    ResetDeviceAxis(&AxisMap.RngKnob);
    ResetDeviceAxis(&AxisMap.CursorX);
    ResetDeviceAxis(&AxisMap.CursorY);
    ResetDeviceAxis(&AxisMap.Comm1Vol);
    ResetDeviceAxis(&AxisMap.Comm2Vol);
    ResetDeviceAxis(&AxisMap.MSLVol);
    ResetDeviceAxis(&AxisMap.ThreatVol);
    ResetDeviceAxis(&AxisMap.InterComVol);
    ResetDeviceAxis(&AxisMap.HudBrt);
    ResetDeviceAxis(&AxisMap.RetDepr);
    ResetDeviceAxis(&AxisMap.Zoom);
    AxisMap.FlightControlDevice = -1;
    memset(&AxisMap.FlightControllerGUID, 0, sizeof(GUID));
    AxisMap.totalDeviceCount = 0;

    for (int i = 0; i < AXIS_MAX; i++)
        SetAnalogIsUsed((GameAxis_t)i, false);

    for (int i = 0; i < SIMLIB_MAX_ANALOG; i++)
    {
        analog[i].center = 0;
        analog[i].cutoff = 15000;
        analog[i].isReversed = false;
        analog[i].isUsed = false;
        // Retro 20Feb2004 .smoothingFactor does not get reset here yet as it can not get set in the UI
        // other analog[] struct members are reset in ResetInputs();
    }

    ResetAllInputs();
}
#include "PilotInputs.h"
/*****************************************************************************/
// Should reset all axis values / button presses / POV presses
/*****************************************************************************/
void SIMLIB_IO_CLASS::ResetAllInputs()
{
    for (int i = 0; i < SIMLIB_MAX_ANALOG; i++)
    {
        analog[i].engrValue = 0.F;
        analog[i].ioVal = 0;
    }

    for (int i = 0; i < SIMLIB_MAX_DIGITAL * SIM_NUMDEVICES; i++)
        digital[i] = false;

    for (int i = 0; i < SIMLIB_MAX_POV; i++)
        povHatAngle[i] = (unsigned long) - 1; // Retro 10Jan2004 - looks daft but serves

    // a purpose: 0xFFFF is 'center' for the POV

    theMouseWheelAxis.ResetAxisValue(); // Retro 17Jan2004

    UserStickInputs.Reset(); // Retro 21Jan2004
}

/*****************************************************************************/
// Sets some sanity check variables: number of enumerated devices and GUID
// of the primary control stick. so I can see on next load if the user
// attached/detached devices. If that is the case, all axis are reset and the
// user should remap
/*****************************************************************************/
void SIMLIB_IO_CLASS::SaveGUIDAndCount()
{
    if (AxisMap.FlightControlDevice not_eq -1)
    {
        HRESULT hres;
        DIDEVICEINSTANCE devinst;
        devinst.dwSize = sizeof(DIDEVICEINSTANCE);

        hres = gpDIDevice[AxisMap.FlightControlDevice]->GetDeviceInfo(&devinst);

        AxisMap.FlightControllerGUID = devinst.guidInstance;
        AxisMap.totalDeviceCount = gTotalJoy;
    }
}

int SIMLIB_IO_CLASS::LoadAxisCalibrationFile()
{
    size_t success = 0;
    AxisCalibration temp;
    long size;
    FILE* fp;

    char path[_MAX_PATH];
    sprintf(path, "%s\\config\\axiscurves.cal", FalconDataDirectory);

    fp = fopen(path, "rb");

    if ( not fp)
        return FALSE;

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size not_eq sizeof(AxisCalibration))
        return FALSE;

    success = fread(&temp, sizeof(AxisCalibration), 1, fp);
    fclose(fp);

    if (success not_eq 1)
        return FALSE;

    AxisShapes = temp;

    return TRUE;
}

// this global array glues all properties of a real axis to an in-game axis
// THE ORDERING IN THIS ARRAY HAS TO BE THE SAME AS IN THE GameAxis_t ENUM 
GameAxisSetup_t AxisSetup[AXIS_MAX] =
{
    // device axis deadzone saturation unipolar?
    { &AxisMap.Pitch.Device, &AxisMap.Pitch.Axis, &AxisMap.Pitch.Deadzone, &AxisMap.Pitch.Saturation, false},
    { &AxisMap.Bank.Device, &AxisMap.Bank.Axis, &AxisMap.Bank.Deadzone, &AxisMap.Bank.Saturation, false},
    { &AxisMap.Yaw.Device, &AxisMap.Yaw.Axis, &AxisMap.Yaw.Deadzone, &AxisMap.Yaw.Saturation, false},
    { &AxisMap.Throttle.Device, &AxisMap.Throttle.Axis, 0, &AxisMap.Throttle.Saturation,  true},
    { &AxisMap.Throttle2.Device, &AxisMap.Throttle2.Axis, 0, &AxisMap.Throttle2.Saturation, true},
    { &AxisMap.PitchTrim.Device, &AxisMap.PitchTrim.Axis, &AxisMap.PitchTrim.Deadzone, &AxisMap.PitchTrim.Saturation,  false},
    { &AxisMap.YawTrim.Device, &AxisMap.YawTrim.Axis, &AxisMap.YawTrim.Deadzone, &AxisMap.YawTrim.Saturation, false},
    { &AxisMap.BankTrim.Device, &AxisMap.BankTrim.Axis, &AxisMap.BankTrim.Deadzone, &AxisMap.BankTrim.Saturation,  false},
    { &AxisMap.BrakeLeft.Device, &AxisMap.BrakeLeft.Axis, 0, &AxisMap.BrakeLeft.Saturation, true},
    // { &AxisMap.BrakeRight.Device, &AxisMap.BrakeRight.Axis, 0, &AxisMap.BrakeRight.Saturation, true},
    { &AxisMap.FOV.Device, &AxisMap.FOV.Axis, 0, &AxisMap.FOV.Saturation, true},
    { &AxisMap.AntElev.Device, &AxisMap.AntElev.Axis, &AxisMap.AntElev.Deadzone, &AxisMap.AntElev.Saturation, false},
    { &AxisMap.CursorX.Device, &AxisMap.CursorX.Axis, &AxisMap.CursorX.Deadzone, &AxisMap.CursorX.Saturation, false},
    { &AxisMap.CursorY.Device, &AxisMap.CursorY.Axis, &AxisMap.CursorY.Deadzone, &AxisMap.CursorY.Saturation, false},
    { &AxisMap.RngKnob.Device, &AxisMap.RngKnob.Axis, &AxisMap.RngKnob.Deadzone, &AxisMap.RngKnob.Saturation, false},
    { &AxisMap.Comm1Vol.Device, &AxisMap.Comm1Vol.Axis, 0, &AxisMap.Comm1Vol.Saturation, true},
    { &AxisMap.Comm2Vol.Device, &AxisMap.Comm2Vol.Axis, 0, &AxisMap.Comm2Vol.Saturation, true},
    { &AxisMap.MSLVol.Device, &AxisMap.MSLVol.Axis, 0, &AxisMap.MSLVol.Saturation, true},
    { &AxisMap.ThreatVol.Device, &AxisMap.ThreatVol.Axis, 0, &AxisMap.ThreatVol.Saturation, true},
    { &AxisMap.HudBrt.Device, &AxisMap.HudBrt.Axis, 0, &AxisMap.HudBrt.Saturation, true},
    { &AxisMap.RetDepr.Device, &AxisMap.RetDepr.Axis, 0, &AxisMap.RetDepr.Saturation, true},
    { &AxisMap.Zoom.Device, &AxisMap.Zoom.Axis, 0, &AxisMap.Zoom.Saturation, true},
    { &AxisMap.InterComVol.Device, &AxisMap.InterComVol.Axis, 0, &AxisMap.InterComVol.Saturation, true},
};
