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
SIMLIB_IO_CLASS IO;

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
        analog[i].engrValue = 0;
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
    FILE* fp;

    sprintf(path, "%s/config/joystick.cal", FalconDataDirectory);

    fp = fopen(path, "rb");

    if (not fp)
        return FALSE;

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size not_eq sizeof(SIMLIB_ANALOG_TYPE) * SIMLIB_MAX_ANALOG)
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
    FILE* fp;

    sprintf(path, "%s/config/joystick.cal", FalconDataDirectory);

    fp = fopen(path, "wb");

    if (not fp)
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
    sprintf(path, "%s/config/axismapping.dat", FalconDataDirectory);

    fp = fopen(path, "rb");

    if (not fp)
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
    FILE* fp;

    sprintf(path, "%s/config/axismapping.dat", FalconDataDirectory);

    fp = fopen(path, "wb");

    if (not fp)
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
#include "pilotinputs.h"
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
        povHatAngle[i] =
            (unsigned long)-1; // Retro 10Jan2004 - looks daft but serves

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
    // #19: snapshot the GUIDs of all devices by their current indices -- on the next start
    // it lets axis Device indices be remapped to the new enumeration
    memcpy(AxisMap.DeviceGUIDs, gDIDevGUIDs, sizeof(AxisMap.DeviceGUIDs));
    AxisMap.totalDeviceCount = gTotalJoy;

    if (AxisMap.FlightControlDevice not_eq -1)
    {
        HRESULT hres;
        DIDEVICEINSTANCE devinst;
        devinst.dwSize = sizeof(DIDEVICEINSTANCE);

        hres = gpDIDevice[AxisMap.FlightControlDevice]->GetDeviceInfo(&devinst);

        AxisMap.FlightControllerGUID = devinst.guidInstance;
    }
}

// #19: remap one saved Device index to the current enumeration by GUID.
// Returns the device's current index, or -1 if the device is currently absent.
static int RemapDeviceIndexByGUID_(int savedIdx, const GUID* savedGUIDs)
{
    if (savedIdx < SIM_JOYSTICK1) // keyboard/mouse/unset -- index is fixed
        return savedIdx;

    if (savedIdx >= SIM_NUMDEVICES)
        return -1;

    static const GUID zeroGUID = {0};
    GUID g = savedGUIDs[savedIdx];

    if (memcmp(&g, &zeroGUID, sizeof(GUID)) == 0) // no saved GUID for the slot
        return -1;

    for (int i = SIM_JOYSTICK1; i < SIM_NUMDEVICES; ++i)
    {
        if (memcmp(&gDIDevGUIDs[i], &g, sizeof(GUID)) == 0)
            return i;
    }

    return -1; // device is not connected right now
}

static void RemapAxis_(DeviceAxis* da, const GUID* savedGUIDs)
{
    if (not da)
        return;

    if (da->Device < SIM_JOYSTICK1) // keyboard/mouse/unset
        return;

    int n = RemapDeviceIndexByGUID_(da->Device, savedGUIDs);

    // Only improve the index on a GUID match. If the GUID is not found -- do NOT touch
    // (keep the index binding as is): it won't get worse than before the changes.
    if (n >= 0)
        da->Device = n;
}

void SIMLIB_IO_CLASS::RemapAxisMappingByGUID()
{
    const GUID* sg = AxisMap.DeviceGUIDs;

    // If there are no saved GUIDs (old .dat format or a save without GUID) -- do NOT remap,
    // so we don't wipe a working index binding (otherwise the sanity check in siloop
    // would see FlightControlDevice=-1 and reset everything to keyboard). Activates only
    // when the file actually contains device GUIDs.
    static const GUID zeroGUID = {0};
    bool hasGUIDs = false;

    for (int i = SIM_JOYSTICK1; i < SIM_NUMDEVICES; ++i)
    {
        if (memcmp(&sg[i], &zeroGUID, sizeof(GUID)) not_eq 0)
        {
            hasGUIDs = true;
            break;
        }
    }

    if (not hasGUIDs)
        return;

    RemapAxis_(&AxisMap.Pitch, sg);
    RemapAxis_(&AxisMap.Bank, sg);
    RemapAxis_(&AxisMap.Yaw, sg);
    RemapAxis_(&AxisMap.Throttle, sg);
    RemapAxis_(&AxisMap.Throttle2, sg);
    RemapAxis_(&AxisMap.BrakeLeft, sg);
    RemapAxis_(&AxisMap.BrakeRight, sg);
    RemapAxis_(&AxisMap.FOV, sg);
    RemapAxis_(&AxisMap.PitchTrim, sg);
    RemapAxis_(&AxisMap.YawTrim, sg);
    RemapAxis_(&AxisMap.BankTrim, sg);
    RemapAxis_(&AxisMap.AntElev, sg);
    RemapAxis_(&AxisMap.RngKnob, sg);
    RemapAxis_(&AxisMap.CursorX, sg);
    RemapAxis_(&AxisMap.CursorY, sg);
    RemapAxis_(&AxisMap.Comm1Vol, sg);
    RemapAxis_(&AxisMap.Comm2Vol, sg);
    RemapAxis_(&AxisMap.MSLVol, sg);
    RemapAxis_(&AxisMap.ThreatVol, sg);
    RemapAxis_(&AxisMap.InterComVol, sg);
    RemapAxis_(&AxisMap.HudBrt, sg);
    RemapAxis_(&AxisMap.RetDepr, sg);
    RemapAxis_(&AxisMap.Zoom, sg);

    // the lead flight-control device -- also only on a GUID match
    if (AxisMap.FlightControlDevice >= SIM_JOYSTICK1)
    {
        int n = RemapDeviceIndexByGUID_(AxisMap.FlightControlDevice, sg);

        if (n >= 0)
            AxisMap.FlightControlDevice = n;
    }
}

int SIMLIB_IO_CLASS::LoadAxisCalibrationFile()
{
    size_t success = 0;
    AxisCalibration temp;
    long size;
    FILE* fp;

    char path[_MAX_PATH];
    sprintf(path, "%s/config/axiscurves.cal", FalconDataDirectory);

    fp = fopen(path, "rb");

    if (not fp)
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
GameAxisSetup_t AxisSetup[AXIS_MAX] = {
    // device axis deadzone saturation unipolar?
    {&AxisMap.Pitch.Device, &AxisMap.Pitch.Axis, &AxisMap.Pitch.Deadzone,
     &AxisMap.Pitch.Saturation, false},
    {&AxisMap.Bank.Device, &AxisMap.Bank.Axis, &AxisMap.Bank.Deadzone,
     &AxisMap.Bank.Saturation, false},
    {&AxisMap.Yaw.Device, &AxisMap.Yaw.Axis, &AxisMap.Yaw.Deadzone,
     &AxisMap.Yaw.Saturation, false},
    {&AxisMap.Throttle.Device, &AxisMap.Throttle.Axis, 0,
     &AxisMap.Throttle.Saturation, true},
    {&AxisMap.Throttle2.Device, &AxisMap.Throttle2.Axis, 0,
     &AxisMap.Throttle2.Saturation, true},
    {&AxisMap.PitchTrim.Device, &AxisMap.PitchTrim.Axis,
     &AxisMap.PitchTrim.Deadzone, &AxisMap.PitchTrim.Saturation, false},
    {&AxisMap.YawTrim.Device, &AxisMap.YawTrim.Axis, &AxisMap.YawTrim.Deadzone,
     &AxisMap.YawTrim.Saturation, false},
    {&AxisMap.BankTrim.Device, &AxisMap.BankTrim.Axis,
     &AxisMap.BankTrim.Deadzone, &AxisMap.BankTrim.Saturation, false},
    {&AxisMap.BrakeLeft.Device, &AxisMap.BrakeLeft.Axis, 0,
     &AxisMap.BrakeLeft.Saturation, true},
    {&AxisMap.FOV.Device, &AxisMap.FOV.Axis, 0, &AxisMap.FOV.Saturation, true},
    {&AxisMap.AntElev.Device, &AxisMap.AntElev.Axis, &AxisMap.AntElev.Deadzone,
     &AxisMap.AntElev.Saturation, false},
    {&AxisMap.CursorX.Device, &AxisMap.CursorX.Axis, &AxisMap.CursorX.Deadzone,
     &AxisMap.CursorX.Saturation, false},
    {&AxisMap.CursorY.Device, &AxisMap.CursorY.Axis, &AxisMap.CursorY.Deadzone,
     &AxisMap.CursorY.Saturation, false},
    {&AxisMap.RngKnob.Device, &AxisMap.RngKnob.Axis, &AxisMap.RngKnob.Deadzone,
     &AxisMap.RngKnob.Saturation, false},
    {&AxisMap.Comm1Vol.Device, &AxisMap.Comm1Vol.Axis, 0,
     &AxisMap.Comm1Vol.Saturation, true},
    {&AxisMap.Comm2Vol.Device, &AxisMap.Comm2Vol.Axis, 0,
     &AxisMap.Comm2Vol.Saturation, true},
    {&AxisMap.MSLVol.Device, &AxisMap.MSLVol.Axis, 0,
     &AxisMap.MSLVol.Saturation, true},
    {&AxisMap.ThreatVol.Device, &AxisMap.ThreatVol.Axis, 0,
     &AxisMap.ThreatVol.Saturation, true},
    {&AxisMap.HudBrt.Device, &AxisMap.HudBrt.Axis, 0,
     &AxisMap.HudBrt.Saturation, true},
    {&AxisMap.RetDepr.Device, &AxisMap.RetDepr.Axis, 0,
     &AxisMap.RetDepr.Saturation, true},
    {&AxisMap.Zoom.Device, &AxisMap.Zoom.Axis, 0, &AxisMap.Zoom.Saturation,
     true},
    {&AxisMap.InterComVol.Device, &AxisMap.InterComVol.Axis, 0,
     &AxisMap.InterComVol.Saturation, true},
    // AXIS_BRAKE_RIGHT (last, matching the enum) - right toe brake for differential braking
    {&AxisMap.BrakeRight.Device, &AxisMap.BrakeRight.Axis, 0,
     &AxisMap.BrakeRight.Saturation, true},
};
