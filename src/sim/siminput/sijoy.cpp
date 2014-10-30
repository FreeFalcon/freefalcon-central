#include "F4Thread.h"
#include "sinput.h"
#include "simio.h"
#include "simdrive.h"
#include "OTWDrive.h"
#include "inpFunc.h"
#include "cpmanager.h"
#include "ffeedbk.h"
#include "datadir.h"
#include "flightData.h"
#include "ui/include/logbook.h"
#include "aircrft.h"
#include "weather.h"

#include "profiler.h" // Retro 26Dec2003
#include "mouselook.h" // Retro 18Jan2004

#pragma warning(push,4)

BOOL CALLBACK JoystickEnumEffectTypeProc(LPCDIEFFECTINFO pei, LPVOID pv);
BOOL JoystickCreateEffect(DWORD dwEffectFlags);

#define BUTTON_PRESSED 0x80
#define KEY_DOWN 0x8
#define REPEAT_DELAY 200

static DWORD LastPressed[SIMLIB_MAX_DIGITAL*SIM_NUMDEVICES] = {0};

static DWORD LastPressedPOV[SIMLIB_MAX_POV] = {0};
static int LastDirPOV[SIMLIB_MAX_POV] = {0};
int center = FALSE;
int setABdetent = FALSE;
int setIdleCutoff = FALSE; // Retro 1Feb2004
long mHelmetIsUR = FALSE; // hack for UR Helmet detected
long mHelmetID;
float UR_HEAD_VIEW = 160.0f;
float UR_PREV_X = 0.0f;
float UR_PREV_Y = 0.0f;

#include "TrackIR.h" // Retro 26/09/03
extern bool g_bEnableTrackIR; // Retro 26/09/03
extern TrackIR theTrackIRObject; // Retro 27/09/03

extern int DisableSmoothing;
extern bool g_bUseNewSmoothing; // Retro 21Feb2004
extern int NoRudder;
static int JoyOutput[SIMLIB_MAX_ANALOG][2] = {0};
extern int PickleOverride;
extern int TriggerOverride;
extern int UseKeyboardThrottle;

int hasForceFeedback = FALSE;

extern int g_nFFEffectAutoCenter; // JB 020306 Don't stop the centering FF effect

enum
{
    NewInput = 0,
    OldInput = 1,
    MAX_DIFF = 10000,
};

unsigned int NumberOfPOVs = 0; // Retro 26Dec2003, want to get rid of 1) gCurJoyCaps and 2) NumHats

void CallFunc(InputFunctionType theFunc, unsigned long val, int state, void* pButton); //Wombat778 03-06-04


extern AxisMapping AxisMap;

LPDIRECTINPUTEFFECT* gForceFeedbackEffect;
int* gForceEffectIsRepeating = NULL;
int* gForceEffectHasDirection = NULL;
int gNumEffectsLoaded = 0;

int g_nThrottleID = DIJOFS_Z; // OW

//#define THE_MPS_WAY_OF_LIFE // Retro 2Jan2004 - with this enabled, old 'IO.analog[].engrVal' algorithm is used. Else it´s mine =)
#define AUTOCENTERFUN // this should bring back autocentering with the FFB-button in the advanced controls tab disabled
#define NO_CENTER_FOR_MY_AXIS_PLEASE // Retro 9Jan2004 - what´s the point ? Doesn´t work too good anyways BTW (has offset)
#define USE_IDLE_CUTOFF // Retro 2Feb2004 - with this enable we use analog[].cutoff as well as the ABDetent
#define SYMMETRIC_THROTTLEDETENTS // Retro 2Feb2004 - ABDetent and cuttof var are set for BOTH throttles by the LEFT throttle 

/*****************************************************************************/
//
/*****************************************************************************/
void SetJoystickCenter(void)
{
    center = TRUE;
}


/*****************************************************************************/
// Bla.. shitty function that should fudge it so that throttles are NOT
// active when entering the 3d.. the user will have to move it a bit before
// it really 'works' - this was working in the older input code version but
// this functioniality seems to have been lost with the switch to the new
// code.. like I said,.. bla
/*****************************************************************************/
long throttleInactiveValue = 0;
bool throttleInactive = false;
void SetThrottleInActive()
{
    if (IO.AnalogIsUsed(AXIS_THROTTLE) == false)
        return;

    ReadThrottle();

    throttleInactiveValue = IO.GetAxisValue(AXIS_THROTTLE);
    IO.analog[AXIS_THROTTLE].engrValue = 0.0F;
    throttleInactive = true;
}

void resetStaticPOVButtonStates() // Retro 24Aug2004
{
    int i;

    for (i = 0; i < SIMLIB_MAX_DIGITAL * SIM_NUMDEVICES; i++)
    {
        LastPressed[i] = 0;
    }

    for (i = 0; i < SIMLIB_MAX_POV; i++)
    {
        LastPressedPOV[i] = 0;
        LastDirPOV[i] = 0;
    }
}

/*****************************************************************************/
//
/*****************************************************************************/
inline void ProcessJoystickInput(GameAxis_t axis, long *value)
{
    if (g_bUseNewSmoothing == false) // Retro 19Feb2004 - this old 'smoothing code' does exactly NADA for me..
    {
        JoyOutput[axis][OldInput] = JoyOutput[axis][NewInput];
        JoyOutput[axis][NewInput] = *value;

        if ( not DisableSmoothing)
        {
            int diff = abs(JoyOutput[axis][NewInput] - JoyOutput[axis][OldInput]) * FloatToInt32((SimLibMajorFrameTime / 0.1F));

            if (diff > MAX_DIFF)
            {
                *value = JoyOutput[axis][OldInput];
                JoyOutput[axis][NewInput] = JoyOutput[axis][OldInput] +
                                            (JoyOutput[axis][NewInput] -
                                             JoyOutput[axis][OldInput]) / 2;
            }
        }

        IO.analog[axis].ioVal = *value;
    }
    else // This one´s better imo, (C) H. Kern of the TU Vienna.. showed it to me in "Konstruktion systemfähiger Messgeräte" ;)
    {
        if (IO.analog[axis].smoothingFactor)
        {
            IO.analog[axis].ioVal = IO.analog[axis].ioVal -
                                    IO.analog[axis].ioVal / IO.analog[axis].smoothingFactor +
                                    *value / IO.analog[axis].smoothingFactor;
        }
        else
        {
            IO.analog[axis].ioVal = *value;
        }
    }
}

/*****************************************************************************/
//
/*****************************************************************************/
void GetURHelmetInput()
{
    HRESULT hRes;
    DIJOYSTATE joyState;
    float headx, heady, vx, vy;
    static long PrevButtonStates;

    hRes = ((LPDIRECTINPUTDEVICE2)gpDIDevice[SIM_JOYSTICK1 + mHelmetID])->Poll();

    hRes = gpDIDevice[SIM_JOYSTICK1 + mHelmetID]->GetDeviceState(sizeof(DIJOYSTATE), &joyState);

    switch (hRes)
    {
        case DI_OK:
            headx = (float)(joyState.lX);
            heady = -(float)(joyState.lY);

            headx = headx * UR_HEAD_VIEW;
            heady = heady * UR_HEAD_VIEW;

            headx = headx / 10000.0f;
            heady = heady / 10000.0f;

            vx = (headx * 0.8f + UR_PREV_X * 0.2f);
            vy = (heady * 0.8f + UR_PREV_Y * 0.2f);

            if (vy > 25.0f) vy = 25.0f; // peg

            if (vy < -85.0f) vy = -85.0f; // peg

            cockpitFlightData.headYaw   = vx * DTR;
            cockpitFlightData.headPitch = vy * DTR;

            UR_PREV_X = vx;
            UR_PREV_Y = vy;

            break;

        default:
            AcquireDeviceInput(SIM_JOYSTICK1 + mHelmetID, TRUE);
            break;
    }
}
#include "fmath.h"
/*****************************************************************************/
//
/*****************************************************************************/
void GetTrackIRInput() // Retro 26/09/03
{
#if 0 // Retro 24Dez2004 - deprecated
    theTrackIRObject.GetTrackIR_ViewValues(&cockpitFlightData.headYaw, &cockpitFlightData.headPitch);
#else
    theTrackIRObject.Poll();

    cockpitFlightData.headYaw = theTrackIRObject.getYaw();
    cockpitFlightData.headPitch = theTrackIRObject.getPitch();
    cockpitFlightData.headRoll = theTrackIRObject.getRoll();
#endif
}

/*****************************************************************************/
//
/*****************************************************************************/
void GetJoystickInput()
{
    Prof(GetJoystickInput);

    HRESULT hRes;
    DIJOYSTATE joyState;

    if ( not gTotalJoy)
        return; // returning if we don´t have a stick

    long device_axis_values[SIM_NUMDEVICES][8]; // 8 axis in a DIJOYSTATE structure.. don´t think we´ll switch to DIJOYSTATE2

    /*******************************************************************************/
    // Polling all devices..
    /*******************************************************************************/
    for (int i = SIM_JOYSTICK1; i < gTotalJoy + SIM_JOYSTICK1; i++)
    {
        // hRes = ((LPDIRECTINPUTDEVICE2)gpDIDevice[i])->Poll();
        hRes = gpDIDevice[i]->Poll(); // Retro 21Jan2004

        // Retro 21Jan2004
        if ((hRes == DIERR_INPUTLOST) or (hRes == DIERR_NOTACQUIRED))
        {
            hRes = gpDIDevice[i]->Acquire();

            if (hRes not_eq DI_OK)
            {
#pragma warning(disable:4127)
                ShiAssert(false);
#pragma warning(default:4127)
                continue;
            }
        }

        // Retro 21Jan2004 end

        hRes = gpDIDevice[i]->GetDeviceState(sizeof(DIJOYSTATE), &joyState);

        switch (hRes)
        {
            case DI_OK:
            {
                /*******************************************************************************/
                // note the values of ALL axis
                /*******************************************************************************/
                device_axis_values[i][DX_XAXIS] = joyState.lX;
                device_axis_values[i][DX_YAXIS] = joyState.lY;
                device_axis_values[i][DX_ZAXIS] = joyState.lZ;
                device_axis_values[i][DX_RXAXIS] = joyState.lRx;
                device_axis_values[i][DX_RYAXIS] = joyState.lRy;
                device_axis_values[i][DX_RZAXIS] = joyState.lRz;
                device_axis_values[i][DX_SLIDER0] = joyState.rglSlider[0];
                device_axis_values[i][DX_SLIDER1] = joyState.rglSlider[1];
                break;
            }

            default:
            {
                AcquireDeviceInput(i, TRUE);
                break;
            }
        }

        /*******************************************************************************/
        // polling the buttons (but NOT the POVs) of ALL connected joystick devices
        // device 0 gets inputbuttons 0-31, device 1 gets 32-63, device 2 gets 64-95 etc
        //
        // could be optimized by 1) only polling devices with axis assigned and 2)
        // only polling devices that own buttons (and only these buttons, not 32)
        /*******************************************************************************/
        for (int j = 0; j < SIMLIB_MAX_DIGITAL; j++)
        {
            IO.digital[j + (SIMLIB_MAX_DIGITAL * (i - SIM_JOYSTICK1))] = (short)(joyState.rgbButtons[j] bitand BUTTON_PRESSED);
        }

        /*******************************************************************************/
        // poll POV ONLY of current controller.. dunno if I should expand that
        /*******************************************************************************/
        if (i == AxisMap.FlightControlDevice)
        {
            BOOL POVCentered;
            unsigned int j = 0;

            for (j = 0; j < NumberOfPOVs; j++) // Retro 26Dec2003
            {
                POVCentered = (LOWORD(joyState.rgdwPOV[j]) == 0xFFFF);

                if (POVCentered)
                    IO.povHatAngle[j] = (unsigned long) - 1;
                else
                    IO.povHatAngle[j] = joyState.rgdwPOV[j];
            }
        }
    }

    if (IO.MouseWheelExists() == true) // Retro 17Jan2004
        device_axis_values[SIM_MOUSE][DX_MOUSEWHEEL] = theMouseWheelAxis.GetAxisValue();

    /*******************************************************************************/
    // Copy and process flight control (roll and pitch) info
    /*******************************************************************************/
    if ((IO.AnalogIsUsed(AXIS_PITCH)) and (IO.AnalogIsUsed(AXIS_ROLL)))
    {
        ProcessJoystickInput(AXIS_PITCH, &device_axis_values[AxisMap.FlightControlDevice][AxisMap.Pitch.Axis]);
        ProcessJoystickInput(AXIS_ROLL, &device_axis_values[AxisMap.FlightControlDevice][AxisMap.Bank.Axis]);

#ifdef THE_MPS_WAY_OF_LIFE
        //IO.analog[0].engrValue = min(max((joyState.lX + IO.analog[0].center)/1000.0f, -1.0F),1.0F);
        IO.analog[AXIS_PITCH].engrValue = (float)device_axis_values[AxisMap.FlightControlDevice][AxisMap.Pitch.Axis] + IO.analog[AXIS_PITCH].center;

        if (IO.analog[AXIS_PITCH].engrValue * IO.analog[AXIS_PITCH].center > 0)
            IO.analog[AXIS_PITCH].engrValue /= (9400.0F + (float)abs(IO.analog[AXIS_PITCH].center));
        else
            IO.analog[AXIS_PITCH].engrValue /= (9400.0F - (float)abs(IO.analog[AXIS_PITCH].center));

        //IO.analog[1].engrValue = min(max((joyState.lY + IO.analog[1].center)/1000.0f, -1.0F),1.0F);
        IO.analog[AXIS_ROLL].engrValue = (float)device_axis_values[AxisMap.FlightControlDevice][AxisMap.Bank.Axis] + IO.analog[AXIS_ROLL].center;

        if (IO.analog[AXIS_ROLL].engrValue * IO.analog[AXIS_ROLL].center > 0)
            IO.analog[AXIS_ROLL].engrValue /= (10000.0F + (float)abs(IO.analog[AXIS_ROLL].center));
        else
            IO.analog[AXIS_ROLL].engrValue /= (10000.0F - (float)abs(IO.analog[AXIS_ROLL].center));

#else // Retro 2Jan2003

        if (g_bUseNewSmoothing == false)
        {
            IO.analog[AXIS_PITCH].engrValue = (float)device_axis_values[AxisMap.FlightControlDevice][AxisMap.Pitch.Axis] + IO.analog[AXIS_PITCH].center;

            if (IO.analog[AXIS_PITCH].engrValue > 0)
                IO.analog[AXIS_PITCH].engrValue /= Abs(10000.0F + IO.analog[AXIS_PITCH].center);
            else
                IO.analog[AXIS_PITCH].engrValue /= Abs(-10000.0F + IO.analog[AXIS_PITCH].center);
        }
        else
        {
            IO.analog[AXIS_PITCH].engrValue = (float)IO.analog[AXIS_PITCH].ioVal + IO.analog[AXIS_PITCH].center;

            if (IO.analog[AXIS_PITCH].engrValue > 0)
                IO.analog[AXIS_PITCH].engrValue /= Abs(10000.0F + IO.analog[AXIS_PITCH].center);
            else
                IO.analog[AXIS_PITCH].engrValue /= Abs(-10000.0F + IO.analog[AXIS_PITCH].center);
        }

        if (g_bUseNewSmoothing == false)
        {
            IO.analog[AXIS_ROLL].engrValue = (float)device_axis_values[AxisMap.FlightControlDevice][AxisMap.Bank.Axis] + IO.analog[AXIS_ROLL].center;

            if (IO.analog[AXIS_ROLL].engrValue > 0)
                IO.analog[AXIS_ROLL].engrValue /= Abs(10000.0F + IO.analog[AXIS_ROLL].center);
            else
                IO.analog[AXIS_ROLL].engrValue /= Abs(-10000.0F + IO.analog[AXIS_ROLL].center);
        }
        else
        {
            IO.analog[AXIS_ROLL].engrValue = (float)IO.analog[AXIS_ROLL].ioVal + IO.analog[AXIS_ROLL].center;

            if (IO.analog[AXIS_ROLL].engrValue > 0)
                IO.analog[AXIS_ROLL].engrValue /= Abs(10000.0F + IO.analog[AXIS_ROLL].center);
            else
                IO.analog[AXIS_ROLL].engrValue /= Abs(-10000.0F + IO.analog[AXIS_ROLL].center);
        }

#endif

        if (center)
        {
            IO.analog[AXIS_PITCH].center = device_axis_values[AxisMap.FlightControlDevice][AxisMap.Pitch.Axis] * -1;
            IO.analog[AXIS_ROLL].center = device_axis_values[AxisMap.FlightControlDevice][AxisMap.Bank.Axis] * -1;
        }
    }

    /*******************************************************************************/
    // Copy and process throttle data (if available)
    // engrVal goes from 0 to 1.5
    /*******************************************************************************/
    if (IO.AnalogIsUsed(AXIS_THROTTLE))
    {
        ProcessJoystickInput(AXIS_THROTTLE, &device_axis_values[AxisMap.Throttle.Device][AxisMap.Throttle.Axis]);

        if (( not UseKeyboardThrottle) or
            (abs(JoyOutput[AXIS_THROTTLE][OldInput] - device_axis_values[AxisMap.Throttle.Device][AxisMap.Throttle.Axis]) > 500.0F))
        {
            UseKeyboardThrottle = FALSE;
#ifdef USE_IDLE_CUTOFF
            long maxThrottleVal = IO.analog[AXIS_THROTTLE].cutoff;
#endif

            // not in afterburner.. throttle 0 result in 0.0F, throttle in ABDetent results in 1.0F - OK
            if ((IO.analog[AXIS_THROTTLE].center) and 
                (device_axis_values[AxisMap.Throttle.Device][AxisMap.Throttle.Axis] > IO.analog[AXIS_THROTTLE].center))
            {
#ifndef USE_IDLE_CUTOFF
                IO.analog[AXIS_THROTTLE].engrValue = (15000.0F - device_axis_values[AxisMap.Throttle.Device][AxisMap.Throttle.Axis]) / (15000.0F - IO.analog[AXIS_THROTTLE].center);
#else
                IO.analog[AXIS_THROTTLE].engrValue = ((float)maxThrottleVal - device_axis_values[AxisMap.Throttle.Device][AxisMap.Throttle.Axis]) / ((float)maxThrottleVal - IO.analog[AXIS_THROTTLE].center);
                IO.analog[AXIS_THROTTLE].engrValue = max(IO.analog[AXIS_THROTTLE].engrValue, 0.0F);
#endif
            }
            // in afterburner - full throttle give 1.5F, exactly ABDetent throttle gives 1.0F - OK
            else if (IO.analog[AXIS_THROTTLE].center)
            {
                IO.analog[AXIS_THROTTLE].engrValue = 1.0F + (IO.analog[AXIS_THROTTLE].center - device_axis_values[AxisMap.Throttle.Device][AxisMap.Throttle.Axis]) / (IO.analog[AXIS_THROTTLE].center * 2.0F);
            }
            // no abdetent set ?? throttle scales linearly between 0.0F and 1.5F - OK
            else
            {
#ifndef USE_IDLE_CUTOFF
                IO.analog[AXIS_THROTTLE].engrValue = (15000.0F - device_axis_values[AxisMap.Throttle.Device][AxisMap.Throttle.Axis]) / 10000.0F;
#else
                IO.analog[AXIS_THROTTLE].engrValue = ((float)maxThrottleVal - device_axis_values[AxisMap.Throttle.Device][AxisMap.Throttle.Axis]) / (float)maxThrottleVal * 1.5F;
#endif
            }

            // Retro 4Jan2004 - blaaa bad code :/
            // see SetThrottleInActive() for explanation
            if (throttleInactive == true)
            {
                if (abs(throttleInactiveValue - device_axis_values[AxisMap.Throttle.Device][AxisMap.Throttle.Axis]) < 5000)
                    IO.analog[AXIS_THROTTLE].engrValue = 0.0F; // no throttle ouput before the user moves the stick..
                else
                {
                    throttleInactive = false;
                    throttleInactiveValue = 0;
                }
            }
        }

        if (setABdetent)
        {
            IO.analog[AXIS_THROTTLE].center = device_axis_values[AxisMap.Throttle.Device][AxisMap.Throttle.Axis];
        }

        if (setIdleCutoff)
        {
            IO.analog[AXIS_THROTTLE].cutoff  = device_axis_values[AxisMap.Throttle.Device][AxisMap.Throttle.Axis];
        }
    }

    /*******************************************************************************/
    // Copy and process throttle2 data (if available)
    // engrVal goes from 0 to 1.5
    /*******************************************************************************/
    if (IO.AnalogIsUsed(AXIS_THROTTLE2))
    {
        ProcessJoystickInput(AXIS_THROTTLE2, &device_axis_values[AxisMap.Throttle2.Device][AxisMap.Throttle2.Axis]);

        if (( not UseKeyboardThrottle) or
            (abs(JoyOutput[AXIS_THROTTLE2][OldInput] - device_axis_values[AxisMap.Throttle2.Device][AxisMap.Throttle2.Axis]) > 500.0F))
        {
            UseKeyboardThrottle = FALSE;
#ifdef USE_IDLE_CUTOFF
            long maxThrottleVal = IO.analog[AXIS_THROTTLE2].cutoff;
#endif

            // not in afterburner.. throttle 0 result in 0.0F, throttle in ABDetent results in 1.0F - OK
            if ((IO.analog[AXIS_THROTTLE2].center) and 
                (device_axis_values[AxisMap.Throttle2.Device][AxisMap.Throttle2.Axis] > IO.analog[AXIS_THROTTLE2].center))
            {
#ifndef USE_IDLE_CUTOFF
                IO.analog[AXIS_THROTTLE2].engrValue = (15000.0F - device_axis_values[AxisMap.Throttle2.Device][AxisMap.Throttle2.Axis]) / (15000.0F - IO.analog[AXIS_THROTTLE2].center);
#else
                IO.analog[AXIS_THROTTLE2].engrValue = ((float)maxThrottleVal - device_axis_values[AxisMap.Throttle2.Device][AxisMap.Throttle2.Axis]) / ((float)maxThrottleVal - IO.analog[AXIS_THROTTLE2].center);
                IO.analog[AXIS_THROTTLE2].engrValue = max(IO.analog[AXIS_THROTTLE2].engrValue, 0.0F);
#endif
            }
            // in afterburner - full throttle gives 1.5F, exactly ABDetent throttle gives 1.0F - OK
            else if (IO.analog[AXIS_THROTTLE2].center)
            {
                IO.analog[AXIS_THROTTLE2].engrValue = 1.0F + (IO.analog[AXIS_THROTTLE2].center - device_axis_values[AxisMap.Throttle2.Device][AxisMap.Throttle2.Axis]) / (IO.analog[AXIS_THROTTLE2].center * 2.0F);
            }
            // no abdetent set ?? throttle scales linearly between 0.0F and 1.5F - OK
            else
            {
#ifndef USE_IDLE_CUTOFF
                IO.analog[AXIS_THROTTLE2].engrValue = (15000.0F - device_axis_values[AxisMap.Throttle2.Device][AxisMap.Throttle2.Axis]) / 10000.0F;
#else
                IO.analog[AXIS_THROTTLE2].engrValue = ((float)maxThrottleVal - device_axis_values[AxisMap.Throttle2.Device][AxisMap.Throttle2.Axis]) / (float)maxThrottleVal * 1.5F;
#endif
            }
        }

        if (setABdetent)
        {
#ifndef SYMMETRIC_THROTTLEDETENTS
            IO.analog[AXIS_THROTTLE2].center = device_axis_values[AxisMap.Throttle2.Device][AxisMap.Throttle2.Axis];
#else
            IO.analog[AXIS_THROTTLE2].center = device_axis_values[AxisMap.Throttle.Device][AxisMap.Throttle.Axis];
#endif
        }

        if (setIdleCutoff)
        {
#ifndef SYMMETRIC_THROTTLEDETENTS
            IO.analog[AXIS_THROTTLE2].cutoff  = device_axis_values[AxisMap.Throttle2.Device][AxisMap.Throttle2.Axis];
#else
            IO.analog[AXIS_THROTTLE2].cutoff  = device_axis_values[AxisMap.Throttle.Device][AxisMap.Throttle.Axis];
#endif
        }
    }

    /*******************************************************************************/
    // Copy and process rudder data (if available)
    /*******************************************************************************/
    if (IO.AnalogIsUsed(AXIS_YAW))
    {
        ProcessJoystickInput(AXIS_YAW, &device_axis_values[AxisMap.Yaw.Device][AxisMap.Yaw.Axis]);
#ifdef THE_MPS_WAY_OF_LIFE
        //IO.analog[3].engrValue = min(max((joyState.lRz + IO.analog[3].center)/1000.0f, -1.0F),1.0F);
        IO.analog[AXIS_YAW].engrValue = (float)device_axis_values[AxisMap.Yaw.Device][AxisMap.Yaw.Axis] + IO.analog[AXIS_YAW].center;

        if (IO.analog[AXIS_YAW].engrValue * IO.analog[AXIS_YAW].center > 0)
            IO.analog[AXIS_YAW].engrValue /= (10000.0F + (float)abs(IO.analog[AXIS_YAW].center));
        else
            IO.analog[AXIS_YAW].engrValue /= (10000.0F - (float)abs(IO.analog[AXIS_YAW].center));

#else // Retro 2Jan2003

        if (g_bUseNewSmoothing == false)
        {
            IO.analog[AXIS_YAW].engrValue = (float)device_axis_values[AxisMap.Yaw.Device][AxisMap.Yaw.Axis] + IO.analog[AXIS_YAW].center;

            if (IO.analog[AXIS_YAW].engrValue > 0)
                IO.analog[AXIS_YAW].engrValue /= Abs(10000.0F + IO.analog[AXIS_YAW].center);
            else
                IO.analog[AXIS_YAW].engrValue /= Abs(-10000.0F + IO.analog[AXIS_YAW].center);
        }
        else
        {
            IO.analog[AXIS_YAW].engrValue = (float)IO.analog[AXIS_YAW].ioVal + IO.analog[AXIS_YAW].center;

            if (IO.analog[AXIS_YAW].engrValue > 0)
                IO.analog[AXIS_YAW].engrValue /= Abs(10000.0F + IO.analog[AXIS_YAW].center);
            else
                IO.analog[AXIS_YAW].engrValue /= Abs(-10000.0F + IO.analog[AXIS_YAW].center);
        }

#endif

        if (center)
        {
            IO.analog[AXIS_YAW].center = device_axis_values[AxisMap.Yaw.Device][AxisMap.Yaw.Axis] * -1;
        }

        if (IO.analog[AXIS_YAW].isReversed == true) // Retro 13Jan2004
        {
            IO.analog[AXIS_YAW].engrValue *= -1;
            IO.analog[AXIS_YAW].ioVal *= -1;
        }
    }

    /*******************************************************************************/
    // Copy and process additional axis
    // These are the axis added by Retro; their processing is a bit less elaborate,
    // and they don´t have ABDetent functionality. Unipolar Axis don´t have
    // center functionality either.
    /*******************************************************************************/
    extern GameAxisSetup_t AxisSetup[AXIS_MAX];

    for (int a = AXIS_TRIM_PITCH; a < AXIS_MAX; a++)
    {
        if (IO.AnalogIsUsed((GameAxis_t)a) == true)
        {
            // unipolar axis.. these have no 'center'
            if (AxisSetup[a].isUniPolar == true)
            {
                if (IO.analog[(GameAxis_t)a].isReversed == false)
                {
                    ProcessJoystickInput((GameAxis_t)a, &device_axis_values[*AxisSetup[a].device][*AxisSetup[a].axis]);
                }
                else
                {
                    long reversedValue = 15000 - device_axis_values[*AxisSetup[a].device][*AxisSetup[a].axis];
                    ProcessJoystickInput((GameAxis_t)a, &reversedValue);
                }
            }
            // bipolar axis..
            else
            {
#ifndef NO_CENTER_FOR_MY_AXIS_PLEASE // Retro 9Jan2004
                long correctedVal = device_axis_values[*AxisSetup[a].device][*AxisSetup[a].axis] + IO.analog[(GameAxis_t)a].center;
#else
                long correctedVal = device_axis_values[*AxisSetup[a].device][*AxisSetup[a].axis];
#endif // NO_CENTER_FOR_MY_AXIS_PLEASE

                if (IO.analog[(GameAxis_t)a].isReversed)
                    correctedVal *= (-1);

                ProcessJoystickInput((GameAxis_t)a, &correctedVal);

#ifndef NO_CENTER_FOR_MY_AXIS_PLEASE // Retro 9Jan2004

                if (center)
                {
                    IO.analog[(GameAxis_t)a].center = device_axis_values[*AxisSetup[a].device][*AxisSetup[a].axis] * -1;
                }

#endif // NO_CENTER_FOR_MY_AXIS_PLEASE
            }
        }
    }

    if (center)
    {
        IO.SaveFile();
        center = FALSE;
    }

    if (setABdetent)
    {
        IO.SaveFile();
        setABdetent = FALSE;
    }

    if (setIdleCutoff) // Retro 1Feb2004
    {
        IO.SaveFile();
        setIdleCutoff = FALSE;
    }

    /*******************************************************************************/
    // Other VR stuff.. and TrackIR polling..
    /*******************************************************************************/
    if (mHelmetIsUR)
    {
        GetURHelmetInput();
    }
    else if ((g_bEnableTrackIR) and (PlayerOptions.Get3dTrackIR() == true)) // Retro 26/09/03
    {
        GetTrackIRInput();
    }
}

/*******************************************************************************/
// Retro: I know process the buttons of ALL connected sticks. POV hat is only
// evaluated for the flight stick though
/*******************************************************************************/
void ProcessJoyButtonAndPOVHat(void)
{
    Prof(ProcessJoyButtonAndPOVHat);

    unsigned int i;
    InputFunctionType theFunc;

    for (i = 0; i < SIMLIB_MAX_DIGITAL * SIM_NUMDEVICES; i++)
    {
#if 0 // Retro 24Aug2004

        if (IO.digital[i])
        {
            DWORD curTicks = GetTickCount();

            if (curTicks > LastPressed[i] + REPEAT_DELAY)
            {
                int ID;
                theFunc = UserFunctionTable.GetButtonFunction(i, &ID);

                if (theFunc)
                {
                    if (ID  < 0)
                    {
                        // theFunc(1, KEY_DOWN, NULL);
                        CallFunc(theFunc, 1, KEY_DOWN, NULL); //Wombat778 03-06-04 Use callfunc instead of directly calling funcs, so they can be captured
                    }
                    else
                    {
                        // theFunc(1, KEY_DOWN, OTWDriver.pCockpitManager->GetButtonPointer(ID));
                        CallFunc(theFunc, 1, KEY_DOWN, OTWDriver.pCockpitManager->GetButtonPointer(ID)); //Wombat778 03-06-04 Use callfunc instead of directly calling funcs, so they can be captured
                        OTWDriver.pCockpitManager->Dispatch(ID, 0);//the 0 should be mousside but I don't have anywhere
                    } //to store it and all functions currently use 0. ;)
                }
                else if ((i % SIMLIB_MAX_DIGITAL) == 0)
                {
                    TriggerOverride = TRUE;
                }
                else if ((i % SIMLIB_MAX_DIGITAL) == 1)
                {
                    PickleOverride = TRUE;
                }

                LastPressed[i] = curTicks;
            }
        }
        else if (LastPressed[i])
        {
            LastPressed[i] = 0;
            int ID;
            theFunc = UserFunctionTable.GetButtonFunction(i, &ID);

            if (theFunc)
                //theFunc(1, 0, NULL);
                CallFunc(theFunc, 1, 0, NULL); //Wombat778 03-06-04 Use callfunc instead of directly calling funcs, so they can be captured

            // else if(i == 0)
            else if ((i % SIMLIB_MAX_DIGITAL) == 0)
            {
                TriggerOverride = FALSE;
            }
            // else if(i == 1)
            else if ((i % SIMLIB_MAX_DIGITAL) == 1)
            {
                PickleOverride = FALSE;
            }
        }

#else // Retro 24Aug2004 - this version just generates one DOWN event when a key is pressed and one UP event when released

        // the one above sent multiple DOWN events every 200 ticks.. not necessary imo
        if (IO.digital[i])
        {
            if (LastPressed[i] == 0)
            {
                int ID;
                theFunc = UserFunctionTable.GetButtonFunction(i, &ID);

                if (theFunc)
                {
                    if (ID  < 0)
                    {
                        CallFunc(theFunc, 1, KEY_DOWN, NULL); //Wombat778 03-06-04 Use callfunc instead of directly calling funcs, so they can be captured
                    }
                    else
                    {
                        CallFunc(theFunc, 1, KEY_DOWN, OTWDriver.pCockpitManager->GetButtonPointer(ID)); //Wombat778 03-06-04 Use callfunc instead of directly calling funcs, so they can be captured
                        OTWDriver.pCockpitManager->Dispatch(ID, 0);//the 0 should be mousside but I don't have anywhere
                    } //to store it and all functions currently use 0. ;)
                }
                else if ((i % SIMLIB_MAX_DIGITAL) == 0)
                {
                    TriggerOverride = TRUE;
                }
                else if ((i % SIMLIB_MAX_DIGITAL) == 1)
                {
                    PickleOverride = TRUE;
                }

                LastPressed[i] = 1;
            }
        }
        else if (LastPressed[i])
        {
            LastPressed[i] = 0;
            int ID;
            theFunc = UserFunctionTable.GetButtonFunction(i, &ID);

            if (theFunc)
            {
                CallFunc(theFunc, 1, 0, NULL); //Wombat778 03-06-04 Use callfunc instead of directly calling funcs, so they can be captured
            }
            else if ((i % SIMLIB_MAX_DIGITAL) == 0)
            {
                TriggerOverride = FALSE;
            }
            else if ((i % SIMLIB_MAX_DIGITAL) == 1)
            {
                PickleOverride = FALSE;
            }
        }

#endif // Retro 24Aug2004

    }

    for (i = 0; i < NumberOfPOVs; i++) // Retro 26Dec2003
    {
        if (IO.povHatAngle[i] not_eq -1)
        {
            int ID;
            InputFunctionType theFunc;
            int Direction = 0;

            if ((IO.povHatAngle[i] < 2250 or IO.povHatAngle[i] > 33750) and IO.povHatAngle[i] not_eq -1)
                Direction = 0;
            else if (IO.povHatAngle[i] < 6750)
                Direction = 1;
            else if (IO.povHatAngle[i] < 11250)
                Direction = 2;
            else if (IO.povHatAngle[i] < 15750)
                Direction = 3;
            else if (IO.povHatAngle[i] < 20250)
                Direction = 4;
            else if (IO.povHatAngle[i] < 24750)
                Direction = 5;
            else if (IO.povHatAngle[i] < 29250)
                Direction = 6;
            else if (IO.povHatAngle[i] < 33750)
                Direction = 7;

            if (LastDirPOV[i] not_eq Direction)
            {
                theFunc = UserFunctionTable.GetPOVFunction(i, LastDirPOV[i], &ID);

                if (theFunc)
                    //theFunc(1, 0, NULL);
                    CallFunc(theFunc, 1, 0, NULL); //Wombat778 03-06-04 Use callfunc instead of directly calling funcs, so they can be captured

                LastDirPOV[i] = Direction;
            }

            DWORD curTicks = GetTickCount();

            if (curTicks > LastPressedPOV[i] + REPEAT_DELAY)
            {
                theFunc = UserFunctionTable.GetPOVFunction(i, Direction, &ID);

                if (theFunc)
                {
                    if (ID  < 0)
                    {
                        //theFunc(1, KEY_DOWN, NULL);
                        CallFunc(theFunc, 1, KEY_DOWN, NULL); //Wombat778 03-06-04 Use callfunc instead of directly calling funcs, so they can be captured
                    }
                    else
                    {
                        //theFunc(1, KEY_DOWN, OTWDriver.pCockpitManager->GetButtonPointer(ID));
                        CallFunc(theFunc, 1, KEY_DOWN, OTWDriver.pCockpitManager->GetButtonPointer(ID)); //Wombat778 03-06-04 Use callfunc instead of directly calling funcs, so they can be captured
                        OTWDriver.pCockpitManager->Dispatch(ID, 0);//the 0 should be mousside but I don't have anywhere
                    } //to store it and all functions currently use 0. ;)
                }
                else
                    SimDriver.POVKludgeFunction(IO.povHatAngle[i]);

                LastPressedPOV[i] = curTicks;
            }
        }
        else if (LastPressedPOV[i])
        {
            LastPressedPOV[i] = 0;
            int ID;
            theFunc = UserFunctionTable.GetPOVFunction(i, LastDirPOV[i], &ID);

            if (theFunc)
                //theFunc(1, 0, NULL);
                CallFunc(theFunc, 1, 0, NULL); //Wombat778 03-06-04 Use callfunc instead of directly calling funcs, so they can be captured
            else
                SimDriver.POVKludgeFunction(IO.povHatAngle[i]);
        }

    }

    //SimDriver.POVKludgeFunction(IO.povHatAngle[0]);

}

/*****************************************************************************/
//
/*****************************************************************************/
float ReadThrottle(void)
{

    HRESULT hRes;
    DIJOYSTATE joyState;

    if ((gTotalJoy) and (IO.AnalogIsUsed(AXIS_THROTTLE) == true)) // Retro 4Jan2004
    {

        hRes = ((LPDIRECTINPUTDEVICE2)gpDIDevice[AxisMap.Throttle.Device])->Poll();

        // Retro 21Jan2004
        if ((hRes == DIERR_INPUTLOST) or (hRes == DIERR_NOTACQUIRED))
        {
            hRes = gpDIDevice[AxisMap.Throttle.Device]->Acquire();

            if (hRes not_eq DI_OK)
            {
#pragma warning(disable:4127)
                ShiAssert(false);
#pragma warning(default:4127)
                return 0.;
            }
        }

        // Retro 21Jan2004 end

        hRes = gpDIDevice[AxisMap.Throttle.Device]->GetDeviceState(sizeof(DIJOYSTATE), &joyState);

        //ShiAssert(hRes == DI_OK); // Retro 4Jan2004  // MLR 5/2/2004 - driving me nuts

        long theDeviceValue = 0;

        switch (hRes)
        {
            case DI_OK:
                switch (AxisMap.Throttle.Axis)
                {
                    case DX_XAXIS:
                        theDeviceValue = joyState.lX;
                        break;

                    case DX_YAXIS:
                        theDeviceValue = joyState.lY;
                        break;

                    case DX_ZAXIS:
                        theDeviceValue = joyState.lZ;
                        break;

                    case DX_RXAXIS:
                        theDeviceValue = joyState.lRx;
                        break;

                    case DX_RYAXIS:
                        theDeviceValue = joyState.lRy;
                        break;

                    case DX_RZAXIS:
                        theDeviceValue = joyState.lRz;
                        break;

                    case DX_SLIDER0:
                        theDeviceValue = joyState.rglSlider[0];
                        break;

                    case DX_SLIDER1:
                        theDeviceValue = joyState.rglSlider[1];
                        break;
                }

                JoyOutput[AXIS_THROTTLE][OldInput] = JoyOutput[AXIS_THROTTLE][NewInput];
                JoyOutput[AXIS_THROTTLE][NewInput] = theDeviceValue;

                IO.analog[AXIS_THROTTLE].ioVal = theDeviceValue; // Retro 11Jan2004

                if (IO.analog[AXIS_THROTTLE].center and theDeviceValue > IO.analog[AXIS_THROTTLE].center)
                    IO.analog[AXIS_THROTTLE].engrValue = (15000.0F - theDeviceValue) / (15000.0F - IO.analog[AXIS_THROTTLE].center);
                else if (IO.analog[AXIS_THROTTLE].center)
                    IO.analog[AXIS_THROTTLE].engrValue = 1.0F + (IO.analog[AXIS_THROTTLE].center - theDeviceValue) / (IO.analog[AXIS_THROTTLE].center * 2.0F);
                else
                    IO.analog[AXIS_THROTTLE].engrValue = (15000.0F - theDeviceValue) / 10000.0F;

                break;

            default:
                AcquireDeviceInput(AxisMap.Throttle.Device, TRUE);

                if (SimDriver.GetPlayerAircraft() and SimDriver.GetPlayerAircraft()->OnGround())
                    IO.analog[AXIS_THROTTLE].engrValue = 0.0f;
                else
                    IO.analog[AXIS_THROTTLE].engrValue = 1.0f;
        }
    }
    else
    {
        if (SimDriver.GetPlayerAircraft() and SimDriver.GetPlayerAircraft()->OnGround())
            IO.analog[AXIS_THROTTLE].engrValue = 0.0f;
        else
            IO.analog[AXIS_THROTTLE].engrValue = 1.0f;
    }

    return IO.analog[AXIS_THROTTLE].engrValue;
}

static int AxisCount = 0; // Retro
#define NUM_OF_STICK_AXIS 8 /* '8' is defined by dinput: 8 axis maximum per device */
AxisIDStuff DIAxisNames[SIM_NUMDEVICES*NUM_OF_STICK_AXIS];

#ifndef USE_DINPUT_8
/*****************************************************************************/
// Retro 31Dec2003
// all this one does is to note the name of every axis located on a device -
// I don´t care if it´s x,y,z,rx,ry,rz,sl0 or sl1 yet
// - just note its name (copy it into that globat array above) and be done
// with it
/*****************************************************************************/
BOOL FAR PASCAL EnumDeviceObjects(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
    char* DevName = (char*)pvRef;

    if ( not pvRef)
        return FALSE;

    ShiAssert(lpddoi->tszName);

    char DevNum[80];
    sprintf(DevNum, " (%i)", gTotalJoy);

    // these strings are deleted on shutdown, in cleanupdiall()
    DIAxisNames[AxisCount].DXAxisName = (char*)malloc(strlen(lpddoi->tszName) + strlen(" - ") + strlen(DevName) + strlen(DevNum) + 1);
    strcpy(DIAxisNames[AxisCount].DXAxisName, lpddoi->tszName);
    strcat(DIAxisNames[AxisCount].DXAxisName, " - ");
    strcat(DIAxisNames[AxisCount].DXAxisName, DevName);
    strcat(DIAxisNames[AxisCount].DXAxisName, DevNum);

    switch (lpddoi->dwOfs)
    {
        case DIJOFS_X:
            DIAxisNames[AxisCount].DXAxisID = DX_XAXIS;
            break;

        case DIJOFS_Y:
            DIAxisNames[AxisCount].DXAxisID = DX_YAXIS;
            break;

        case DIJOFS_Z:
            DIAxisNames[AxisCount].DXAxisID = DX_ZAXIS;
            break;

        case DIJOFS_RX:
            DIAxisNames[AxisCount].DXAxisID = DX_RXAXIS;
            break;

        case DIJOFS_RY:
            DIAxisNames[AxisCount].DXAxisID = DX_RYAXIS;
            break;

        case DIJOFS_RZ:
            DIAxisNames[AxisCount].DXAxisID = DX_RZAXIS;
            break;

        case DIJOFS_SLIDER(0):
            DIAxisNames[AxisCount].DXAxisID = DX_SLIDER0;
            break;

        case DIJOFS_SLIDER(1):
            DIAxisNames[AxisCount].DXAxisID = DX_SLIDER1;
            break;
    }

    DIAxisNames[AxisCount].DXDeviceID = gTotalJoy + SIM_JOYSTICK1;

    AxisCount++;

    return TRUE;
}
#else // USE_DINPUT_8
/*****************************************************************************/
// Retro 16Jan2004 - with dinput8, enumerating device objects seems busted
// it´s picking up imaginary axis, and does not see real ones.. so I have to
// look for the real existing axis this way. Functionally it is the same
// as the CallBack function above (EnumDeviceObjects) but it handles all
// possible axis on a joystick at once.
//
// Of course, should the dataformat change (to joystick2) then we´d have to
// change a bit here (and in the rest of the code )
/*****************************************************************************/
void CheckAxisOnDevice(LPDIRECTINPUTDEVICE8 pdev, const char* DevName)
{
    ShiAssert(pdev);
    ShiAssert(DevName);

    DIDEVICEOBJECTINSTANCE devobj;
    HRESULT hres;
    devobj.dwSize = sizeof(DIDEVICEOBJECTINSTANCE);

    char DevNum[80];
    sprintf(DevNum, " (%i)", gTotalJoy);

    typedef struct
    {
        int Offset;
        int theAx;
    } AxisOffsets_t;

    AxisOffsets_t AxisOffsets[NUM_OF_STICK_AXIS] =
    {
        { DIJOFS_X, DX_XAXIS },
        { DIJOFS_Y, DX_YAXIS },
        { DIJOFS_Z, DX_ZAXIS },
        { DIJOFS_RX, DX_RXAXIS },
        { DIJOFS_RY, DX_RYAXIS },
        { DIJOFS_RZ, DX_RZAXIS },
        { DIJOFS_SLIDER(0), DX_SLIDER0 },
        { DIJOFS_SLIDER(1), DX_SLIDER1 }
    };

    for (int i = 0; i < NUM_OF_STICK_AXIS; i++)
    {
        hres = pdev->GetObjectInfo(&devobj, AxisOffsets[i].Offset, DIPH_BYOFFSET);

        if (hres == DI_OK)
        {
            // these strings are deleted on shutdown, in cleanupdiall()
            DIAxisNames[AxisCount].DXAxisName = (char*)malloc(strlen(devobj.tszName) + strlen(" - ") + strlen(DevName) + strlen(DevNum) + 1);
            strcpy(DIAxisNames[AxisCount].DXAxisName, devobj.tszName);
            strcat(DIAxisNames[AxisCount].DXAxisName, " - ");
            strcat(DIAxisNames[AxisCount].DXAxisName, DevName);
            strcat(DIAxisNames[AxisCount].DXAxisName, DevNum);

            DIAxisNames[AxisCount].DXAxisID = AxisOffsets[i].theAx;

            DIAxisNames[AxisCount].DXDeviceID = gTotalJoy + SIM_JOYSTICK1;
            AxisCount++;
        }
    }
}

/*****************************************************************************/
// brrrr... trying to get the mousewheel as 'just another axis'
//
// of course it isn´t that clear cut: mousewheel has no deadzone, no saturation
// and I can´t set range props. instead I´ll have to clamp the values depending
// on if the mapped axis is bipolar or unipolar
/*****************************************************************************/
void CheckForMouseAxis(void)
{
#ifdef USE_DINPUT_8
    LPDIRECTINPUTDEVICE8 mouse = 0;
    HRESULT hr = gpDIObject->CreateDevice(GUID_SysMouse, &mouse, NULL);

    if (hr not_eq DI_OK)
        return;

    hr = mouse->SetDataFormat(&c_dfDIMouse);

    if (hr == DI_OK)
    {

        DIDEVICEINSTANCE pdidi;
        pdidi.dwSize = sizeof(DIDEVICEINSTANCE);
        char DevName[100] = {0};

        hr = mouse->GetDeviceInfo(&pdidi);

        if (hr == DI_OK)
        {
            strncpy(DevName, pdidi.tszProductName, 100);
        }
        else
        {
            sprintf(DevName, "Mouse");
        }

        DIDEVICEOBJECTINSTANCE devobj;
        devobj.dwSize = sizeof(DIDEVICEOBJECTINSTANCE);

        hr = mouse->GetObjectInfo(&devobj, DIMOFS_Z,  DIPH_BYOFFSET);

        if (hr == DI_OK)
        {
            // found a mouse with a wheel
            DIAxisNames[AxisCount].DXAxisName = (char*)malloc(strlen(devobj.tszName) + strlen(" - ") + strlen(DevName) + 1);
            strcpy(DIAxisNames[AxisCount].DXAxisName, devobj.tszName);
            strcat(DIAxisNames[AxisCount].DXAxisName, " - ");
            strcat(DIAxisNames[AxisCount].DXAxisName, DevName);

            DIAxisNames[AxisCount].DXAxisID = DX_MOUSEWHEEL;

            DIAxisNames[AxisCount].DXDeviceID = SIM_MOUSE; // SIM_MOUSE = 0
            AxisCount++;

            IO.SetMouseWheelExists(true);
        }
    }

    mouse->Release();
    mouse = 0;
#else
    // no mousewheel axis 
#endif
}
#endif // USE_DINPUT_8

/*****************************************************************************/
// by Retro 28Dec2003 (put it into its own function to be able to call it)
//
// (De)activates the FFB autocenter function. If FFB is enabled in-game then
// autocenter goes OFF (we do it ourselves then), else we turn it back ON so
// that at least centering spring forces are there, else it feels like ass.
//
// I´m ASSuming that this IS a FFB stick  You can´t check this with
// HasForceFeedback however (at least not here) 
/*****************************************************************************/
int ActivateAutoCenter(const bool OnOff, const int theJoyIndex)
{
    ShiAssert(theJoyIndex >= 0);
    HRESULT hres;

    // have to unacquire in order to set new properties 
    hres = gpDIDevice[theJoyIndex + SIM_JOYSTICK1]->Unacquire();

    DIPROPDWORD DIPropAutoCenter;

    // Disable stock auto-center
    DIPropAutoCenter.diph.dwSize = sizeof(DIPropAutoCenter);
    DIPropAutoCenter.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    DIPropAutoCenter.diph.dwObj = 0;
    DIPropAutoCenter.diph.dwHow = DIPH_DEVICE;

    if (OnOff == false)
        DIPropAutoCenter.dwData = DIPROPAUTOCENTER_OFF;
    else
        DIPropAutoCenter.dwData = DIPROPAUTOCENTER_ON;

    //Wombat778 Put back to original code because fix for FF Centering is now it atmos.cpp.  This should be cleaner and better.
    if ( not VerifyResult(gpDIDevice[SIM_JOYSTICK1 + theJoyIndex]->SetProperty(DIPROP_AUTOCENTER, &DIPropAutoCenter.diph)))
    {
        if (OnOff == false)
            OutputDebugString("Failed to turn auto-center off.\n");
        else
            OutputDebugString("Failed to turn auto-center on.\n");

        hres = gpDIDevice[theJoyIndex + SIM_JOYSTICK1]->Acquire();
        return FALSE;
    }
    else
    {
        hres = gpDIDevice[theJoyIndex + SIM_JOYSTICK1]->Acquire();
        return TRUE;
    }
}

/*****************************************************************************/
// by Retro 25Dec2003 (put it into its own function to be able to call it)
// Check if a device has FFB support. Called on startup, and when the primary
// device is changed (in the config tab)
// FFB is only supported for the primary flight device (the one with the PITCH
// and BANK axis)
//
// theJoyIndex is the index, with SIM_JOYSTICK1 deducted  
/*****************************************************************************/
int CheckForForceFeedback(const int theJoyIndex)
{
    ShiAssert(theJoyIndex >= 0);
    HRESULT hres;

#ifndef AUTOCENTERFUN
    // have to unacquire in order to set new properties 
    hres = gpDIDevice[theJoyIndex + SIM_JOYSTICK1]->Unacquire();
#endif

    DIDEVCAPS devcaps;
    devcaps.dwSize = sizeof(DIDEVCAPS);
    hres = gpDIDevice[theJoyIndex + SIM_JOYSTICK1]->GetCapabilities(&devcaps);

    if (theJoyIndex == AxisMap.FlightControlDevice - SIM_JOYSTICK1)  // Retro 31Dec2003
    {
        // a total shit sandwich here.. FFB only for the primary device  if nothing is yet mapped
        // then it will have to wait till the user selects it in the appropriate screen
        if (devcaps.dwFlags bitand DIDC_FORCEFEEDBACK)
        {
            //Got it
            OutputDebugString("ForceFeedback device found.\n");

            // we're supporting ForceFeedback
            if ( not JoystickCreateEffect(0xffffffff))
            {
                OutputDebugString("JoystickCreateEffects() failed - ForceFeedback disabled\n");
                hasForceFeedback =  FALSE;
#ifndef AUTOCENTERFUN
                hres = gpDIDevice[theJoyIndex + SIM_JOYSTICK1]->Acquire();
#endif
                return FALSE;
            }
            else
            {
#ifndef AUTOCENTERFUN
                DIPROPDWORD DIPropAutoCenter;

                // Disable stock auto-center
                DIPropAutoCenter.diph.dwSize = sizeof(DIPropAutoCenter);
                DIPropAutoCenter.diph.dwHeaderSize = sizeof(DIPROPHEADER);
                DIPropAutoCenter.diph.dwObj = 0;
                DIPropAutoCenter.diph.dwHow = DIPH_DEVICE;
                DIPropAutoCenter.dwData = DIPROPAUTOCENTER_OFF;

                //Wombat778 Put back to original code because fix for FF Centering is now it atmos.cpp.  This should be cleaner and better.
                if ( not VerifyResult(gpDIDevice[SIM_JOYSTICK1 + theJoyIndex]->SetProperty(DIPROP_AUTOCENTER, &DIPropAutoCenter.diph)))
                {
                    OutputDebugString("Failed to turn auto-center off.\n");
                    hasForceFeedback =  FALSE;
                    hres = gpDIDevice[theJoyIndex + SIM_JOYSTICK1]->Acquire();
                    return FALSE;
                }
                else
                {
                    hasForceFeedback =  TRUE;
                    hres = gpDIDevice[theJoyIndex + SIM_JOYSTICK1]->Acquire();
                    return TRUE;
                }

#else
                // autocenter will get turned OFF if FFB is ENABLED 
                hasForceFeedback =  ActivateAutoCenter( not PlayerOptions.GetFFB(), theJoyIndex);
                return hasForceFeedback;
#endif
            }
        }

        hasForceFeedback =  FALSE;
#ifndef AUTOCENTERFUN
        hres = gpDIDevice[theJoyIndex + SIM_JOYSTICK1]->Acquire();
#endif
        return FALSE; // no ffb stick 
    }

#ifndef AUTOCENTERFUN
    hres = gpDIDevice[theJoyIndex + SIM_JOYSTICK1]->Acquire();
#endif
    return FALSE; // device checked is not the primary flight stick (the only one that has FFB effects) 
}

/*****************************************************************************/
// Device Enumeration Callback Function (DECAF)
/*****************************************************************************/
BOOL FAR PASCAL InitJoystick(LPCDIDEVICEINSTANCE pdinst, LPVOID pvRef)
{
    BOOL SetupResult;

#ifndef USE_DINPUT_8 // Retro 15Jan2004
    LPDIRECTINPUTDEVICE7 pdev;
#else
    LPDIRECTINPUTDEVICE8 pdev;
#endif

    // LPDIRECTINPUTDEVICE pdev; // Retro 15Jan2004
    HWND hWndMain = *((HWND *)pvRef);
    DIDEVCAPS devcaps;
    DIDEVICEOBJECTINSTANCE devobj;

    devobj.dwSize = sizeof(DIDEVICEOBJECTINSTANCE);

    devcaps.dwSize = sizeof(DIDEVCAPS);

    /*****************************************************************************/
    // Create the device
    /*****************************************************************************/
#ifndef USE_DINPUT_8 // Retro 15Jan2004
    SetupResult = VerifyResult(gpDIObject->CreateDeviceEx(pdinst->guidInstance, IID_IDirectInputDevice7, (void **) &pdev, NULL));
#else
    HRESULT hr = gpDIObject->CreateDevice(pdinst->guidInstance, &pdev, NULL);
    SetupResult = (hr == DI_OK) ? TRUE : FALSE;
#endif

    if ( not SetupResult)
    {
        return DIENUM_CONTINUE;
    }

    /*****************************************************************************/
    // set joystick data format
    /*****************************************************************************/
    SetupResult = VerifyResult(pdev->SetDataFormat(&c_dfDIJoystick));

    /*****************************************************************************/
    // so what do we have here ?
    /*****************************************************************************/
    SetupResult = VerifyResult(pdev->GetCapabilities(&devcaps));

    /*****************************************************************************/
    // set the cooperative level
    /*****************************************************************************/
    if (SetupResult)
    {
#ifdef NDEBUG
        SetupResult = VerifyResult(pdev->SetCooperativeLevel(hWndMain, DISCL_EXCLUSIVE bitor DISCL_FOREGROUND));
#else // bye bye FFB :/
        SetupResult = VerifyResult(pdev->SetCooperativeLevel(hWndMain, DISCL_EXCLUSIVE bitor DISCL_BACKGROUND));
#endif
    }

#ifndef USE_DINPUT_8 // Retro 16Jan2004
    /*****************************************************************************/
    // Enumerate all axis on the device
    /*****************************************************************************/
    pdev->EnumObjects(EnumDeviceObjects, (void*)&pdinst->tszProductName, DIDFT_AXIS);
#else
    CheckAxisOnDevice(pdev, pdinst->tszProductName);
#endif

    if (SetupResult)
    {
        /***************************************************/
        // Convert it to a Device2 so we can Poll() it.
        //  (Will also increment reference count if it succeeds)
        /***************************************************/
#ifndef USE_DINPUT_8 // Retro 15Jan2004
        SetupResult = VerifyResult(pdev->QueryInterface(IID_IDirectInputDevice2, (LPVOID *)&gpDIDevice[SIM_JOYSTICK1 + gTotalJoy]));
#else
        gpDIDevice[SIM_JOYSTICK1 + gTotalJoy] = pdev;
#endif
    }

    /*****************************************************************************/
    // Check for Force Feedback
    /*****************************************************************************/
    if (SetupResult)
        CheckForForceFeedback(gTotalJoy);
    else
        hasForceFeedback = FALSE;

    if (SetupResult)
    {
        /*****************************************************************************/
        // use pdinst to get name of joystick for display purposes
        /*****************************************************************************/
        int len = _tcslen(pdinst->tszProductName);
        gDIDevNames[SIM_JOYSTICK1 + gTotalJoy] = new _TCHAR[len + 1];

        if (gDIDevNames[SIM_JOYSTICK1 + gTotalJoy])
            _tcscpy(gDIDevNames[SIM_JOYSTICK1 + gTotalJoy], pdinst->tszProductName);

        if ( not strcmp(pdinst->tszProductName, "Union Reality Gear"))
        {
            OutputDebugString("UR Helmet found\n");
            OTWDriver.SetHeadTracking(TRUE);
            mHelmetIsUR = 1;
            mHelmetID = gTotalJoy;
        }

        gTotalJoy++;

        /*****************************************************************************/
        // we've used up our whole array, sorry no more
        /*****************************************************************************/
        if ((SIM_JOYSTICK1 + gTotalJoy) >= SIM_NUMDEVICES)
            return DIENUM_STOP;
    }

#ifndef USE_DINPUT_8 // Retro 15Jan2004
    //if the call to QueryInterface succeeded the reference count was incremented to 2
    //so this will drop it to 1 or 0.
    pdev->Release();
#endif
    return DIENUM_CONTINUE;
}

/*****************************************************************************/
//
/*****************************************************************************/
//BOOL JoystickCreateEffect(DWORD dwEffectFlags)
BOOL JoystickCreateEffect(DWORD)
{
    FILE* fPtr = NULL;
    unsigned int effectType = 0;
    int numEffects = 0;
    char effectName[20] = {0};
    int i = 0;
    unsigned int j = 0, k = 0;
    DIEFFECT effectHolder;
    DIENVELOPE envelopeHolder;
    DICUSTOMFORCE customHolder;
    DIPERIODIC periodicHolder;
    DICONSTANTFORCE constantHolder;
    DIRAMPFORCE rampHolder;
    DICONDITION* conditionHolder = NULL;
    char str1[80] = {0}, str2[80] = {0};
    DWORD* axesArray = NULL;
    long* dirArray = NULL;
    long* forceData = NULL;
    char dataFileName[_MAX_PATH];
    GUID guidEffect;
    DWORD SetupResult = FALSE;

    // Retro 27Dec2003
    /* no point in inquiring force feedback caps of a not (yet) mapped device */
    /* this propably means that  FF has to be restarted after a controller change */
    /* or I just run this stuff on assigning controllers (have to find out FF caps anyway */

    if (AxisMap.FlightControlDevice < SIM_JOYSTICK1)
        return FALSE;

    LPDIRECTINPUTDEVICE2 joystickDevice = (LPDIRECTINPUTDEVICE2)gpDIDevice[AxisMap.FlightControlDevice];

    int guidType = 0;

    sprintf(dataFileName, "%s\\config\\feedback.ini", FalconDataDirectory);

    fPtr = fopen(dataFileName, "r");

    // Find the file
    if ( not fPtr)
    {
        OutputDebugString("Unable to open force feedback data file");
        return FALSE;
    }

    fclose(fPtr);

    JoystickReleaseEffects();

    // Start parsing
    numEffects = GetPrivateProfileInt("EffectData", "numEffects", 0, dataFileName);
    gNumEffectsLoaded = numEffects;
    gForceFeedbackEffect = new LPDIRECTINPUTEFFECT[numEffects];
    gForceEffectIsRepeating = new int[numEffects];
    gForceEffectHasDirection = new int[numEffects];

    // Read each effect
    for (i = 0; i < numEffects; i++)
    {
        ZeroMemory(&effectHolder, sizeof effectHolder);
        ZeroMemory(&envelopeHolder, sizeof envelopeHolder);
        effectHolder.dwSize = sizeof(DIEFFECT);
        envelopeHolder.dwSize = sizeof(DIENVELOPE);
        // Set the key
        sprintf(effectName, "Effect%d", i);
        gForceFeedbackEffect[i] = NULL;
        gForceEffectIsRepeating[i] = FALSE;
        gForceEffectHasDirection[i] = FALSE;

        // Read all the common data for this effect
        effectHolder.dwFlags = GetPrivateProfileInt(effectName, "flags", 0, dataFileName);
        effectHolder.dwDuration = GetPrivateProfileInt(effectName, "duration", 0, dataFileName);
        effectHolder.dwSamplePeriod = GetPrivateProfileInt(effectName, "SamplePeriod", 0, dataFileName);
        effectHolder.dwGain = GetPrivateProfileInt(effectName, "gain", 0, dataFileName);
        effectHolder.dwTriggerButton = GetPrivateProfileInt(effectName, "triggerButton", 0, dataFileName);
        effectHolder.dwTriggerRepeatInterval = GetPrivateProfileInt(effectName, "triggerRepeatInterval", 0, dataFileName);
        effectHolder.cAxes = GetPrivateProfileInt(effectName, "numAxes", 0, dataFileName);
        gForceEffectHasDirection[i] = GetPrivateProfileInt(effectName, "hasDirection", 0, dataFileName);

        if (axesArray)
        {
            delete axesArray;
            axesArray = NULL;
        }

        if (dirArray)
        {
            delete dirArray;
            dirArray = NULL;
        }

        if (effectHolder.cAxes)
        {
            axesArray = new DWORD [effectHolder.cAxes];
            effectHolder.rgdwAxes = axesArray;

            dirArray = new long [effectHolder.cAxes];
            effectHolder.rglDirection = dirArray;

            for (j = 0; j < effectHolder.cAxes; j++)
            {
                sprintf(str1, "axes%d", j);
                sprintf(str2, "direction%d", j);
                k = GetPrivateProfileInt(effectName, str1, 0, dataFileName);

                switch (k)
                {
                    case 0:
                        effectHolder.rgdwAxes[j] = DIJOFS_X;
                        break;

                    case 1:
                        effectHolder.rgdwAxes[j] = DIJOFS_Y;
                        break;

                    case 2:
                        effectHolder.rgdwAxes[j] = g_nThrottleID;
                        break;

                    case 4:
                        effectHolder.rgdwAxes[j] = DIJOFS_RX;
                        break;

                    case 5:
                        effectHolder.rgdwAxes[j] = DIJOFS_RY;
                        break;

                    case 6:
                        effectHolder.rgdwAxes[j] = DIJOFS_RZ;
                        break;
                }

                effectHolder.rglDirection[j] = GetPrivateProfileInt(effectName, str2, 0, dataFileName);
            }
        }
        else
        {
            effectHolder.rgdwAxes = NULL;
            effectHolder.rglDirection = NULL;
        }

        // Check for envelope
        j = GetPrivateProfileInt(effectName, "envelopeAttackLevel", -1, dataFileName);

        if (j not_eq 0xffffffff)
        {
            envelopeHolder.dwSize = sizeof(DIENVELOPE);
            envelopeHolder.dwAttackLevel = j;
            envelopeHolder.dwAttackTime = GetPrivateProfileInt(effectName, "envelopeAttackTime", 0, dataFileName);
            envelopeHolder.dwFadeLevel = GetPrivateProfileInt(effectName, "envelopeFadeLevel", 0, dataFileName);
            envelopeHolder.dwFadeTime = GetPrivateProfileInt(effectName, "envelopeFadeTime", 0, dataFileName);
            effectHolder.lpEnvelope = &envelopeHolder;
        }
        else
        {
            effectHolder.lpEnvelope = NULL;
        }

        effectType = GetPrivateProfileInt(effectName, "effectType", 0, dataFileName);

        switch (effectType)
        {
            case DIEFT_CUSTOMFORCE:
                customHolder.cChannels = GetPrivateProfileInt(effectName, "customForceChannels", 0, dataFileName);
                customHolder.dwSamplePeriod = GetPrivateProfileInt(effectName, "customForceSamplePeriod", 0, dataFileName);
                customHolder.cSamples = GetPrivateProfileInt(effectName, "customForceSamples", 0, dataFileName);

                if (forceData)
                {
                    delete forceData;
                    forceData = NULL;
                }

                forceData = new long[customHolder.cChannels * customHolder.cSamples];
                customHolder.rglForceData = forceData;

                for (j = 0; j < customHolder.cChannels; j++)
                {
                    for (k = 0; k < customHolder.cSamples; k++)
                    {
                        sprintf(str1, "customForceForceChannel%dSample%d", j, k);
                        customHolder.rglForceData[j * k + j] = GetPrivateProfileInt(effectName, str1, 0, dataFileName);
                    }
                }

                // Add the sizes
                effectHolder.cbTypeSpecificParams = 3 * sizeof(DWORD) + sizeof(long) * customHolder.cChannels * customHolder.cSamples;
                effectHolder.lpvTypeSpecificParams = &customHolder;

                // enumerate for a custom force effect
                SetupResult = VerifyResult(joystickDevice->EnumEffects((LPDIENUMEFFECTSCALLBACK)JoystickEnumEffectTypeProc,
                                           &guidEffect, DIEFT_CUSTOMFORCE));

                guidEffect = GUID_Square;

                if ( not SetupResult)
                {
                    OutputDebugString("EnumEffects(Costum Force) failed\n");
                    continue;
                }

                break;

            case DIEFT_PERIODIC:
                periodicHolder.dwMagnitude = GetPrivateProfileInt(effectName, "periodicForceMagnitude", 0, dataFileName);
                periodicHolder.lOffset = GetPrivateProfileInt(effectName, "periodicForceOffset", 0, dataFileName);
                periodicHolder.dwPhase = GetPrivateProfileInt(effectName, "periodicForcePhase", 0, dataFileName);
                periodicHolder.dwPeriod = GetPrivateProfileInt(effectName, "periodicForcePeriod", 0, dataFileName);
                gForceEffectIsRepeating[i] = TRUE;

                // Add the sizes
                effectHolder.cbTypeSpecificParams = 3 * sizeof(DWORD) + sizeof(long);
                effectHolder.lpvTypeSpecificParams = &periodicHolder;

                // enumerate for a periodic force effect

                // enumerate for a periodic force effect
                guidType = GetPrivateProfileInt(effectName, "periodicType", -1, dataFileName);

                switch (guidType)
                {
                    case 0:
                        guidEffect = GUID_Sine;
                        break;

                    case 1:
                        guidEffect = GUID_Square;
                        break;

                    case 2:
                        guidEffect = GUID_Triangle;
                        break;

                    case 3:
                        guidEffect = GUID_SawtoothUp;
                        break;

                    case 4:
                        guidEffect = GUID_SawtoothDown;
                        break;

                    default:
                        SetupResult = VerifyResult(joystickDevice->EnumEffects((LPDIENUMEFFECTSCALLBACK)JoystickEnumEffectTypeProc,
                                                   &guidEffect, DIEFT_PERIODIC));
                        break;
                }

                if ( not SetupResult)
                {
                    OutputDebugString("EnumEffects(Periodic Force) failed\n");
                    continue;
                }

                break;

            case DIEFT_CONSTANTFORCE:
                constantHolder.lMagnitude = GetPrivateProfileInt(effectName, "constantForceMagnitude", 0, dataFileName);

                // Add the sizes
                effectHolder.cbTypeSpecificParams = sizeof(long);
                effectHolder.lpvTypeSpecificParams = &constantHolder;

                // enumerate for a constant force effect
                SetupResult = VerifyResult(joystickDevice->EnumEffects((LPDIENUMEFFECTSCALLBACK)JoystickEnumEffectTypeProc,
                                           &guidEffect, DIEFT_CONSTANTFORCE));

                if ( not SetupResult)
                {
                    OutputDebugString("EnumEffects(Constant Force) failed\n");
                    continue;
                }

                break;

            case DIEFT_RAMPFORCE:
                rampHolder.lStart = GetPrivateProfileInt(effectName, "rampForceStart", 0, dataFileName);
                rampHolder.lEnd = GetPrivateProfileInt(effectName, "rampForceEnd", 0, dataFileName);

                // Add the sizes
                effectHolder.cbTypeSpecificParams = 2 * sizeof(long);
                effectHolder.lpvTypeSpecificParams = &rampHolder;

                // enumerate for a ramp force effect
                SetupResult = VerifyResult(joystickDevice->EnumEffects((LPDIENUMEFFECTSCALLBACK)JoystickEnumEffectTypeProc,
                                           &guidEffect, DIEFT_RAMPFORCE));

                if ( not SetupResult)
                {
                    OutputDebugString("EnumEffects(Ramp Force) failed\n");
                    continue;
                }

                break;

            case DIEFT_CONDITION:
                if (conditionHolder)
                {
                    delete conditionHolder;
                    conditionHolder = NULL;
                }

                conditionHolder = new DICONDITION[effectHolder.cAxes];

                for (j = 0; j < effectHolder.cAxes; j++)
                {
                    conditionHolder[j].lOffset = GetPrivateProfileInt(effectName, "conditionOffset", 0, dataFileName);
                    conditionHolder[j].lPositiveCoefficient = GetPrivateProfileInt(effectName, "conditionPositiveCoefficient", 0, dataFileName);
                    conditionHolder[j].lNegativeCoefficient = GetPrivateProfileInt(effectName, "conditionNegativeCoefficient", 0, dataFileName);
                    conditionHolder[j].dwPositiveSaturation = GetPrivateProfileInt(effectName, "conditionPositiveSaturation", 0, dataFileName);
                    conditionHolder[j].dwNegativeSaturation = GetPrivateProfileInt(effectName, "conditionNegativeSaturation", 0, dataFileName);
                    conditionHolder[j].lDeadBand = GetPrivateProfileInt(effectName, "conditionDeadband", 0, dataFileName);
                }

                // Add the size
                effectHolder.cbTypeSpecificParams = sizeof(DICONDITION) * effectHolder.cAxes;
                effectHolder.lpvTypeSpecificParams = conditionHolder;

                guidEffect = GUID_Spring;
                break;

            default:
                effectHolder.cbTypeSpecificParams = 0;
                effectHolder.lpvTypeSpecificParams = NULL;
                break;
        }

        // call CreateEffect()
        SetupResult = VerifyResult(joystickDevice->CreateEffect(guidEffect, &effectHolder,
                                   &gForceFeedbackEffect[i], NULL));
    }

    if (axesArray)
    {
        delete axesArray;
        axesArray = NULL;
    }

    if (dirArray)
    {
        delete dirArray;
        dirArray = NULL;
    }

    if (forceData)
    {
        delete forceData;
        forceData = NULL;
    }

    if (conditionHolder)
    {
        delete conditionHolder;
        conditionHolder = NULL;
    }

    return TRUE;
}

/*****************************************************************************/
//
/*****************************************************************************/
void JoystickReleaseEffects(void)
{
    if ( not hasForceFeedback)
        return;

    // Get rid of any old effects
    ShiAssert(gNumEffectsLoaded > 0 and gNumEffectsLoaded < 20); // arbitrary for now JPO

    if (gNumEffectsLoaded > 20)
        return; // arbitray sanity

    for (int i = 0; i < gNumEffectsLoaded; i++)
    {
        // gForceFeedbackEffect[i]->Unload();
        ShiAssert(FALSE == F4IsBadReadPtr(gForceFeedbackEffect[i], sizeof(*gForceFeedbackEffect[i])));

        if (F4IsBadReadPtr(gForceFeedbackEffect[i], sizeof(*gForceFeedbackEffect[i])))
            continue;

        gForceFeedbackEffect[i]->Release();
    }

    delete[] gForceFeedbackEffect;
    gForceFeedbackEffect = NULL;
    delete[] gForceEffectIsRepeating;
    gForceEffectIsRepeating = NULL;
    delete[] gForceEffectHasDirection;
    gForceEffectHasDirection = NULL;
    gForceFeedbackEffect = NULL;
    gNumEffectsLoaded = 0;
}

/*****************************************************************************/
//
/*****************************************************************************/
void JoystickStopAllEffects(void)
{
    int i;

    if ( not hasForceFeedback)
        return;

    for (i = 0; i < gNumEffectsLoaded; i++)
    {
        if (i not_eq g_nFFEffectAutoCenter)
            JoystickStopEffect(i);
    }
}

// Retro 14Feb2004
#ifndef NDEBUG
int lastStartedEffect = -1;
int lastStoppedEffect = -1;
#endif

/*****************************************************************************/
//
/*****************************************************************************/
void JoystickStopEffect(int effectNum)
{
    if ( not hasForceFeedback or effectNum >= gNumEffectsLoaded or not gForceFeedbackEffect or not gForceFeedbackEffect[effectNum])
        return;

    ShiAssert(FALSE == F4IsBadReadPtr(gForceFeedbackEffect[effectNum], sizeof * gForceFeedbackEffect[effectNum]));
    VerifyResult(gForceFeedbackEffect[effectNum]->Stop());

#ifndef NDEBUG

    // Retro 14Jan2004
    if (effectNum not_eq lastStoppedEffect)
    {
        lastStoppedEffect = effectNum;
        MonoPrint("Stopped effect %d\n", effectNum);
    }

#endif
}

/*****************************************************************************/
//
/*****************************************************************************/
int JoystickPlayEffect(int effectNum, int data)
{
    DWORD           SetupResult;
    DIEFFECT        diEffect;
    LONG            rglDirections[2] = { 0, 0 };

    if ( not hasForceFeedback or effectNum >= gNumEffectsLoaded or not gForceFeedbackEffect or not gForceFeedbackEffect[effectNum])
        return FALSE;

    if (PlayerOptions.GetFFB() == false) // Retro 27Dec2003 - returning false here.. dunno if this is too clever though
        return FALSE;

    ShiAssert(FALSE == F4IsBadReadPtr(&gForceEffectHasDirection[effectNum], sizeof gForceEffectHasDirection[effectNum]));
    ShiAssert(FALSE == F4IsBadReadPtr(gForceFeedbackEffect[effectNum], sizeof * gForceFeedbackEffect[effectNum]));

    // initialize DIEFFECT structure
    memset(&diEffect, 0, sizeof(DIEFFECT));
    diEffect.dwSize = sizeof(DIEFFECT);

    if (gForceEffectHasDirection[effectNum])
    {
        // set the direction
        // since this is a polar coordinate effect, we will pass the angle
        // in as the direction relative to the x-axis, and will leave 0
        // for the y-axis direction

        // Direction is passed in in degrees, we convert to 100ths
        // of a degree to make it easier for the caller.
        rglDirections[0]        = data * 100;
        diEffect.dwFlags        = DIEFF_OBJECTOFFSETS bitor DIEFF_POLAR;
        diEffect.cAxes          = 2;
        diEffect.rglDirection   = rglDirections;
        SetupResult = VerifyResult(gForceFeedbackEffect[effectNum]->SetParameters(&diEffect, DIEP_DIRECTION));

        if ( not SetupResult)
        {
            OutputDebugString("SetParameters(Bounce effect) failed\n");
            return FALSE;
        }
    }

    if (effectNum == JoyRunwayRumble1 or effectNum == JoyRunwayRumble2)
    {
        DIPERIODIC periodicHolder;

        periodicHolder.dwMagnitude = 2000;
        periodicHolder.lOffset = 0;
        periodicHolder.dwPhase = 0;
        periodicHolder.dwPeriod = data;
        diEffect.cbTypeSpecificParams = sizeof(DIPERIODIC);
        diEffect.lpvTypeSpecificParams = &periodicHolder;
        SetupResult = VerifyResult(gForceFeedbackEffect[effectNum]->SetParameters(&diEffect, DIEP_TYPESPECIFICPARAMS));

        if ( not SetupResult)
        {
            OutputDebugString("SetParameters(Runway Rumble) failed\n");
            return FALSE;
        }
    }

    if (effectNum == JoyAutoCenter)
    {
        DICONDITION conditionHolder[2];

        conditionHolder[0].lOffset = 0;
        conditionHolder[0].lPositiveCoefficient = data;
        conditionHolder[0].lNegativeCoefficient = data;
        conditionHolder[0].dwPositiveSaturation = data;
        conditionHolder[0].dwNegativeSaturation = data;
        conditionHolder[0].lDeadBand = 100;
        conditionHolder[1].lOffset = 0;
        conditionHolder[1].lPositiveCoefficient = data;
        conditionHolder[1].lNegativeCoefficient = data;
        conditionHolder[1].dwPositiveSaturation = data;
        conditionHolder[1].dwNegativeSaturation = data;
        conditionHolder[1].lDeadBand = 100;
        diEffect.cbTypeSpecificParams = sizeof(DICONDITION) * 2;
        diEffect.lpvTypeSpecificParams = conditionHolder;

        SetupResult = VerifyResult(gForceFeedbackEffect[effectNum]->SetParameters(&diEffect, DIEP_TYPESPECIFICPARAMS));

        if ( not SetupResult)
        {
            OutputDebugString("SetParameters(Auto Center) failed\n");
            return FALSE;
        }
    }

    // play the effect
    SetupResult = VerifyResult(gForceFeedbackEffect[effectNum]->Start(1, 0));

    if ( not SetupResult)
    {
        //JoystickCreateEffect(1);
        MonoPrint("Start Effect %d Failed\n", effectNum);
        return FALSE;
    }

#ifndef NDEBUG // Retro 14Feb2004

    if (effectNum not_eq lastStartedEffect)
    {
        lastStartedEffect = effectNum;
        MonoPrint("Started effect %d\n", effectNum);
    }

#endif

    return TRUE;
}

/*****************************************************************************/
//
/*****************************************************************************/
BOOL CALLBACK JoystickEnumEffectTypeProc(LPCDIEFFECTINFO pei, LPVOID pv)
{
    GUID *pguidEffect = NULL;

    // validate pv
    // BUGBUG

    // report back the guid of the effect we enumerated
    if (pv)
    {

        pguidEffect = (GUID *)pv;

        *pguidEffect = pei->guid;

    }

    // BUGBUG - look at this some more....
    return DIENUM_STOP;

}
#pragma warning(pop)
