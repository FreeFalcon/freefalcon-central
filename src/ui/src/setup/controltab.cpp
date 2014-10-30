#include "falclib.h"
#include "chandler.h"
#include "userids.h"
#include "PlayerOp.h"
#include <mmsystem.h>
#include "sim/include/stdhdr.h"
#include "sim/include/simio.h"
#include "ui_setup.h"
#include <tchar.h>
#include "sim/include/inpFunc.h"
#include "sim/include/commands.h"
#include "f4find.h"
#include "sim/include/sinput.h"

//temporary until logbook is working
#include "uicomms.h"

bool JoyEffectPlaying = false;

#include "sim/include/ffeedbk.h" // Retro 25Dec2003 for instant FFB feedback

#pragma warning (disable : 4706) // assignment within conditional expression

extern C_Handler *gMainHandler;
extern C_Parser *gMainParser;
extern char **KeyDescrips;
extern long Cluster;
extern bool g_bEmptyFilenameFix; // 2002-04-18 MN

#define SECOND_KEY_MASK 0xFFFF00
#define MOD2_MASK 0x0000FF
#define KEY1_MASK 0x00FF00
#define MOD1_MASK 0xFF0000

typedef struct
{
    InputFunctionType func;
    int buttonId;
    int mouseSide;
    int key2;
    int mod2;
    int key1;
    int mod1;
    int editable;
    char descrip[_MAX_PATH];
} KeyMap;

typedef struct
{
    int X;
    int Y;
    int W;
    int H;
} HotSpotStruct;

enum
{
    KEY2,
    FLAGS,
    BUTTON_ID,
    MOUSE_SIDE,
    EDITABLE,
    FUNCTION_PTR,
};

KeyVars KeyVar = {FALSE, 0, 0, 0, 0, FALSE, FALSE};
KeyMap UndisplayedKeys[300] = {NULL, 0, 0, 0, 0, 0, 0, 0};
int NumUndispKeys = 0;
int NumDispKeys = 0;
extern int NoRudder;
extern long mHelmetIsUR; // hack for Italian 1.06 version - Only gets set to true IF UR Helmet detected
extern long mHelmetID;

extern int setABdetent;
extern int setIdleCutoff; // Retro 1Feb2004

extern unsigned int NumberOfPOVs; // Retro 26Dec2003
int InitializeValueBars = 1; // Retro 26Dec2003, pulled up
//CalibrateStruct Calibration = {FALSE,1,0,TRUE};
extern int hasForceFeedback; // Retro 27Dec2003

void SetThrottleAndRudderBars(C_Base* control); // Retro 17Jan2004

/************************************************************************/
// Retro Macro/Define definitions
/************************************************************************/
#define SATURATION_NONE -1 // no axis saturation
extern int g_nSaturationSmall;
extern int g_nSaturationMedium;
extern int g_nSaturationLarge;

extern int g_nDeadzoneSmall;
extern int g_nDeadzoneMedium;
extern int g_nDeadzoneLarge;
extern int g_nDeadzoneHuge;

extern int g_nMouseLookSensMax;
extern int g_nMouseLookSensMin;

extern int g_nMouseWheelSensMax;
extern int g_nMouseWheelSensMin;

extern int g_nKeyPOVSensMax;
extern int g_nKeyPOVSensMin;

// macro out of soundtab.cpp
#define RESCALE(in,inmin,inmax,outmin,outmax) ((float)(in) - (inmin)) * ((outmax) - (outmin)) / ((inmax) - (inmin)) + (outmin)
/************************************************************************/
// Retro Macro/Define definitions end
/************************************************************************/

//defined in this file
int CreateKeyMapList(C_Window *win);
//SIM_INT Calibrate ( void );


//defined in another file
void InitKeyDescrips(void);
void CleanupKeys(void);
void SetDeleteCallback(void (*cb)(long, short, C_Base*));
void SaveAFile(long TitleID, _TCHAR *filespec, _TCHAR *excludelist[], void (*YesCB)(long, short, C_Base*), void (*NoCB)(long, short, C_Base*), _TCHAR *filename);
void LoadAFile(long TitleID, _TCHAR *filespec, _TCHAR *excludelist[], void (*YesCB)(long, short, C_Base*), void (*NoCB)(long, short, C_Base*));
void CloseWindowCB(long ID, short hittype, C_Base *control);
void AreYouSure(long TitleID, long MessageID, void (*OkCB)(long, short, C_Base*), void (*CancelCB)(long, short, C_Base*));
void AreYouSure(long TitleID, _TCHAR *text, void (*OkCB)(long, short, C_Base*), void (*CancelCB)(long, short, C_Base*));
void SetJoystickCenter(void);
void DelSTRFileCB(long ID, short hittype, C_Base *control);
void DelDFSFileCB(long ID, short hittype, C_Base *control);
void DelLSTFileCB(long ID, short hittype, C_Base *control);
void DelCamFileCB(long ID, short hittype, C_Base *control);
void DelTacFileCB(long ID, short hittype, C_Base *control);
void DelTGAFileCB(long ID, short hittype, C_Base *control);
void DelVHSFileCB(long ID, short hittype, C_Base *control);
void DelKeyFileCB(long ID, short hittype, C_Base *control);

///////////////
////ControlsTab
///////////////

void HideKeyStatusLines(C_Window *win)
{
    if ( not win)
        return;

    C_Line *line;
    C_Button *button;
    int count = 1;

    line = (C_Line *)win->FindControl(KEYCODES - count);

    while (line)
    {
        button = (C_Button *)win->FindControl(KEYCODES + count);

        if ( not button or button->GetUserNumber(EDITABLE) not_eq -1)
        {
            line->SetFlagBitOn(C_BIT_INVISIBLE);
            line->Refresh();
        }

        line = (C_Line *)win->FindControl(KEYCODES - ++count);
    }
}

void SetButtonColor(C_Button *button)
{
    if (button->GetUserNumber(EDITABLE) < 1)
        button->SetFgColor(0, RGB(0, 255, 0)); //green
    else
    {
        button->SetFgColor(0, RGB(230, 230, 230)); //white
    }

    button->Refresh();
}

void RecenterJoystickCB(long, short hittype, C_Base *)
{
    if ((hittype not_eq C_TYPE_LMOUSEUP))
        return;

    SetJoystickCenter();
}

///////////**************************************/////////////////
// Retro
///////////**************************************/////////////////
extern AxisMapping AxisMap;
extern AxisIDStuff DIAxisNames[SIM_NUMDEVICES * 8]; /* '8' is defined by dinput: 8 axis maximum per device */
extern void SetupGameAxis();
extern int CheckForForceFeedback(const int theJoyIndex);
extern GameAxisSetup_t AxisSetup[AXIS_MAX];
extern RECT AxisValueBox;
extern float AxisValueBoxHScale;
extern float AxisValueBoxWScale;
extern bool g_bEnableTrackIR;

typedef struct
{
    GameAxis_t InGameAxis;
    int AxisLB;
    int AxisValueBar;
    int DeadzoneLB;
    int SaturationLB;
    int ReverseBtn;
    DeviceAxis* theDeviceAxis;
} UIInputStuff_t;

UIInputStuff_t UIInputStuff[AXIS_MAX] =
{
    //   InGameAxis Axis Listbox Value Bar Deadzone Listbox Saturation Listbox Reverse Button DeviceAxis Struct
    { AXIS_PITCH, 0, SETUP_ADVANCED_PITCH_VAL, SETUP_ADVANCED_PITCH_DEADZONE, SETUP_ADVANCED_SAT_PITCH, 0, &AxisMap.Pitch },
    { AXIS_ROLL, 0, SETUP_ADVANCED_BANK_VAL, SETUP_ADVANCED_BANK_DEADZONE, SETUP_ADVANCED_SAT_BANK, 0, &AxisMap.Bank },
    { AXIS_FOV, SETUP_ADVANCED_FOV, SETUP_ADVANCED_FOV_VAL, 0, SETUP_ADVANCED_SAT_FOV, SETUP_ADVANCED_REVERSE_FOV, &AxisMap.FOV },
    { AXIS_YAW, SETUP_ADVANCED_RUDDER_AXIS, SETUP_ADVANCED_RUDDER_VAL, SETUP_ADVANCED_RUDDER_AXIS_DEADZONE, SETUP_ADVANCED_SAT_YAW, SETUP_ADVANCED_REVERSE_RUDDER, &AxisMap.Yaw },
    { AXIS_THROTTLE, SETUP_ADVANCED_THROTTLE_AXIS, SETUP_ADVANCED_THROTTLE_VAL, 0, SETUP_ADVANCED_SAT_THROTTLE, 0, &AxisMap.Throttle },
    { AXIS_THROTTLE2, SETUP_ADVANCED_THROTTLE2_AXIS, SETUP_ADVANCED_THROTTLE2_VAL, 0, SETUP_ADVANCED_SAT_THROTTLE2, 0, &AxisMap.Throttle2 },
    { AXIS_TRIM_ROLL, SETUP_ADVANCED_AILERON_TRIM, SETUP_ADVANCED_BANK_TRIM_VAL, SETUP_ADVANCED_AILERON_TRIM_DEADZONE, SETUP_ADVANCED_SAT_BANKTRIM, SETUP_ADVANCED_REVERSE_BANK_TRIM, &AxisMap.BankTrim },
    { AXIS_TRIM_PITCH, SETUP_ADVANCED_TRIM_PITCH, SETUP_ADVANCED_PITCH_TRIM_VAL, SETUP_ADVANCED_TRIM_PITCH_DEADZONE, SETUP_ADVANCED_SAT_PITCHTRIM, SETUP_ADVANCED_REVERSE_PITCH_TRIM, &AxisMap.PitchTrim },
    { AXIS_TRIM_YAW, SETUP_ADVANCED_TRIM_YAW, SETUP_ADVANCED_YAW_TRIM_VAL, SETUP_ADVANCED_TRIM_YAW_DEADZONE, SETUP_ADVANCED_SAT_YAWTRIM, SETUP_ADVANCED_REVERSE_YAW_TRIM, &AxisMap.YawTrim },
    { AXIS_BRAKE_LEFT, SETUP_ADVANCED_BRAKE_LEFT, SETUP_ADVANCED_BRAKE_LEFT_VAL, 0, SETUP_ADVANCED_SAT_BRAKELEFT, SETUP_ADVANCED_REVERSE_BRAKE_LEFT, &AxisMap.BrakeLeft },
    // { AXIS_BRAKE_RIGHT, SETUP_ADVANCED_BRAKE_RIGHT, SETUP_ADVANCED_BRAKE_RIGHT_VAL, 0, SETUP_ADVANCED_SAT_BRAKERIGHT, SETUP_ADVANCED_REVERSE_BRAKE_RIGHT, &AxisMap.BrakeRight },
    { AXIS_ANT_ELEV, SETUP_ADVANCED_ANT_ELEV, SETUP_ADVANCED_ANT_ELEV_VAL, SETUP_ADVANCED_ANT_ELEV_DEADZONE, SETUP_ADVANCED_SAT_ANT_ELEV, SETUP_ADVANCED_REVERSE_ANT_ELEV, &AxisMap.AntElev },
    { AXIS_CURSOR_X, SETUP_ADVANCED_CURSOR_X, SETUP_ADVANCED_CURSOR_X_VAL, SETUP_ADVANCED_CURSOR_X_DEADZONE, SETUP_ADVANCED_SAT_CURSOR_X, SETUP_ADVANCED_REVERSE_CURSOR_X, &AxisMap.CursorX },
    { AXIS_CURSOR_Y, SETUP_ADVANCED_CURSOR_Y, SETUP_ADVANCED_CURSOR_Y_VAL, SETUP_ADVANCED_CURSOR_Y_DEADZONE, SETUP_ADVANCED_SAT_CURSOR_Y, SETUP_ADVANCED_REVERSE_CURSOR_Y, &AxisMap.CursorY },
    { AXIS_RANGE_KNOB, SETUP_ADVANCED_RANGE_KNOB, SETUP_ADVANCED_RANGE_KNOB_VAL, SETUP_ADVANCED_RANGE_KNOB_DEADZONE, SETUP_ADVANCED_SAT_RNG_KNOB, SETUP_ADVANCED_REVERSE_RANGE_KNOB, &AxisMap.RngKnob },
    { AXIS_COMM_VOLUME_1, SETUP_ADVANCED_COMM1_VOL, SETUP_ADVANCED_COMM1_VOL_VAL, 0, SETUP_ADVANCED_SAT_COMM1_VOL, SETUP_ADVANCED_REVERSE_COMM1_VOL, &AxisMap.Comm1Vol },
    { AXIS_COMM_VOLUME_2, SETUP_ADVANCED_COMM2_VOL, SETUP_ADVANCED_COMM2_VOL_VAL, 0, SETUP_ADVANCED_SAT_COMM2_VOL, SETUP_ADVANCED_REVERSE_COMM2_VOL, &AxisMap.Comm2Vol },
    { AXIS_MSL_VOLUME, SETUP_ADVANCED_MSL_VOL, SETUP_ADVANCED_MSL_VOL_VAL, 0, SETUP_ADVANCED_SAT_MSL_VOL, SETUP_ADVANCED_REVERSE_MSL_VOL, &AxisMap.MSLVol },
    { AXIS_THREAT_VOLUME, SETUP_ADVANCED_THREAT_VOL, SETUP_ADVANCED_THREAT_VOL_VAL, 0, SETUP_ADVANCED_SAT_THREAT_VOL, SETUP_ADVANCED_REVERSE_THREAT_VOL, &AxisMap.ThreatVol },
    { AXIS_HUD_BRIGHTNESS, SETUP_ADVANCED_HUD_BRIGHT, SETUP_ADVANCED_HUD_BRIGHT_VAL, 0, SETUP_ADVANCED_SAT_HUD_BRT, SETUP_ADVANCED_REVERSE_HUD_BRIGHT, &AxisMap.HudBrt },
    { AXIS_RET_DEPR, SETUP_ADVANCED_RET_DEPR, SETUP_ADVANCED_RET_DEPR_VAL, 0, SETUP_ADVANCED_SAT_RET_DEPR, SETUP_ADVANCED_REVERSE_RET_DEPR, &AxisMap.RetDepr },
    { AXIS_ZOOM, SETUP_ADVANCED_ZOOM, SETUP_ADVANCED_ZOOM_VAL, 0, SETUP_ADVANCED_SAT_ZOOM, SETUP_ADVANCED_REVERSE_ZOOM, &AxisMap.Zoom },
    { AXIS_INTERCOM_VOLUME, SETUP_ADVANCED_INTERCOM_VOL, SETUP_ADVANCED_INTERCOM_VOL_VAL, 0, SETUP_ADVANCED_SAT_INTERCOM_VOL, SETUP_ADVANCED_REVERSE_INTERCOM_VOL, &AxisMap.InterComVol },
};

/************************************************************************/
// Yuck.. you know, this should actually go into the listbox class methinks
/************************************************************************/
bool SetListBoxItemData(C_ListBox* theLB, const long ID, const short theItemIndex, const long theItemData)
{
    // ID seems to be the index of the item in the listbox.
    if (theLB)
    {
        theLB->SetItemUserData(ID, theItemIndex, theItemData);
        return true;
    }
    else
    {
        return false;
    }
}

/************************************************************************/
// Yuck.. you know, this should actually go into the listbox class methinks
// return values only valid if >= -1 (of course THAT should NOT get into
// the class in anyone volunteers to do that)
/************************************************************************/
long GetListBoxItemData(C_ListBox* theLB, const int theItemIndex, const long theIndex = 0)
{
    if (theLB)
    {
        LISTBOX *item;

        if (theIndex == 0)
        {
            item = theLB->FindID(theLB->GetTextID());
        }
        else
        {
            item = theLB->FindID(theIndex);
        }

        if ((item) and (item->Label_))
            return item->Label_->GetUserNumber(theItemIndex);
    }

    return -2; // return values only valid if >= -1
}

/************************************************************************/
//
/************************************************************************/
void SaveAxisMappings(C_Window* win)
{
    if (win)
    {
        for (int i = 0; i < AXIS_MAX; i++)
        {
            if (UIInputStuff[i].AxisLB == 0)
                continue;

            C_ListBox* listbox = (C_ListBox *)win->FindControl(UIInputStuff[i].AxisLB);

            if (listbox)
            {
                long index = GetListBoxItemData(listbox, 0);

                if (index > -2)
                {
                    if (index not_eq -1)
                    {
                        UIInputStuff[i].theDeviceAxis->Device = DIAxisNames[index].DXDeviceID;
                        UIInputStuff[i].theDeviceAxis->Axis = DIAxisNames[index].DXAxisID;
                    }
                    else
                    {
                        // mapped to keyboard (deactivated the axis)
                        UIInputStuff[i].theDeviceAxis->Device = -1;
                        UIInputStuff[i].theDeviceAxis->Axis = -1;
                    }
                }
            }
            else
            {
                ShiAssert(false);
            }
        }
    }
    else
    {
        ShiAssert(false);
    }
}

/************************************************************************/
// This callback function doesn´t do much, it just hides the other
// tabs
/************************************************************************/
void SetupControlTabsCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    int i = 1;

    while (control->GetUserNumber(i))
    {
        control->Parent_->HideCluster(control->GetUserNumber(i));
        i++;
    }

    control->Parent_->UnHideCluster(control->GetUserNumber(0));

    control->Parent_->RefreshWindow();

    Cluster = control->GetUserNumber(0);
}

/************************************************************************/
// Called when the user presses either the 'Apply' or the 'OK' button
// in the advanced controller sheet
/************************************************************************/
void AdvancedControlApplyCB(long ID, short hittype, C_Base *control)
{
    if ((hittype not_eq C_TYPE_LMOUSEUP))
        return;

    /* pointer to mommy */
    C_Window *win;

    win = gMainHandler->FindWindow(SETUP_WIN);

    if (win == NULL) return;

    win = gMainHandler->FindWindow(SETUP_CONTROL_ADVANCED_WIN);

    if ( not win) return;

    /* array of pointers to all axis listboxes in this sheet */
    C_ListBox *listbox;

    // check contents of axis listboxes and saves..
    SaveAxisMappings(win);

    // check contents of deadzone listboxes..
    listbox = (C_ListBox*)0;

    for (int j = 0; j < AXIS_MAX; j++)
    {
        int index = -1;

        if (UIInputStuff[j].DeadzoneLB == 0)
            continue;

        listbox = (C_ListBox*)win->FindControl(UIInputStuff[j].DeadzoneLB);

        if (listbox)
        {
            index = listbox->GetTextID();

            if (index >= 0)
            {
                switch (index) // not every axis used the deadzone though.. only bipolar ones 
                {
                    case SETUP_ADVANCED_DZ_SMALL:
                        UIInputStuff[j].theDeviceAxis->Deadzone = g_nDeadzoneSmall;
                        break; // 1%

                    default:
                        ShiAssert(false); // fallthrough intentional to give me 5% deadzone in these cases..

                    case SETUP_ADVANCED_DZ_MEDIUM:
                        UIInputStuff[j].theDeviceAxis->Deadzone = g_nDeadzoneMedium;
                        break; // 5%

                    case SETUP_ADVANCED_DZ_LARGE:
                        UIInputStuff[j].theDeviceAxis->Deadzone = g_nDeadzoneLarge;
                        break; // 10%

                    case SETUP_ADVANCED_DZ_HUGE:
                        UIInputStuff[j].theDeviceAxis->Deadzone = g_nDeadzoneHuge;
                        break; // 50%
                }
            }
        }
        else
            ShiAssert(false);
    }

    // check contents of saturtaion listboxes..
    listbox = (C_ListBox*)0;

    for (int j = 0; j < AXIS_MAX; j++)
    {
        int index = -1;

        if (UIInputStuff[j].SaturationLB == 0)
            continue;

        listbox = (C_ListBox*)win->FindControl(UIInputStuff[j].SaturationLB);

        if (listbox)
        {
            index = listbox->GetTextID();

            if (index >= 0)
            {
                switch (index)
                {
                    case SETUP_ADVANCED_SAT_NONE:
                        UIInputStuff[j].theDeviceAxis->Saturation = SATURATION_NONE;
                        break;

                    default:
                        ShiAssert(false);

                    case SETUP_ADVANCED_DZ_SMALL:
                        UIInputStuff[j].theDeviceAxis->Saturation = g_nSaturationSmall;
                        break;

                    case SETUP_ADVANCED_DZ_MEDIUM:
                        UIInputStuff[j].theDeviceAxis->Saturation = g_nSaturationMedium;
                        break;

                    case SETUP_ADVANCED_DZ_LARGE:
                        UIInputStuff[j].theDeviceAxis->Saturation = g_nSaturationLarge;
                        break;
                }
            }
        }
        else
            ShiAssert(false);
    }


    C_Button* button = (C_Button*)0;
    // check 'reversed' buttons..

    for (int j = 0; j < AXIS_MAX; j++) // Retro 15Jan2004
    {
        if (UIInputStuff[j].ReverseBtn == 0)
            continue;

        button = (C_Button *)win->FindControl(UIInputStuff[j].ReverseBtn);

        if (button not_eq NULL)
        {
            if (button->GetState() == C_STATE_1)
                IO.SetAnalogIsReversed(UIInputStuff[j].InGameAxis, true);
            else
                IO.SetAnalogIsReversed(UIInputStuff[j].InGameAxis, false);
        }
        else
            ShiAssert(false);
    }

    IO.SaveFile();
    IO.WriteAxisMappingFile();
    SetupGameAxis();

    /* PROBLEM: have to call the 'SetThrottleAndRudderBars' functions in the setup->controls tab.. hmm */
    win = gMainHandler->FindWindow(SETUP_WIN);

    if (win == NULL) return;

    button = (C_Button *)win->FindControl(SETUP_CONTROL_ADVANCED);

    if (button)
    {
        SetThrottleAndRudderBars(button);
    }
    else
        ShiAssert(false);
}

/************************************************************************/
// Called when the user presses 'OK'. Calls the Apply CB to make changes
// to mappings, then shut down the window
/************************************************************************/
void AdvancedControlOKCB(long ID, short hittype, C_Base *control)
{
    if ((hittype not_eq C_TYPE_LMOUSEUP))
        return;

    /* this takes care of saving */
    AdvancedControlApplyCB(ID, hittype, control);

    /* ..and quit */
    CloseWindowCB(ID, hittype, control);
}

/************************************************************************/
// Called when the user presses 'Cancel' (the widget in the upper right
// corner. Just leaves the window without making changes (provided the
// user didn´t press APPLY first..
/************************************************************************/
void AdvancedControlCancelCB(long ID, short hittype, C_Base *control)
{
    if ((hittype not_eq C_TYPE_LMOUSEUP))
        return;

    /* just quit without saving */
    CloseWindowCB(ID, hittype, control);
}

/************************************************************************/
// "Enable Mouse Look" Callback function
/************************************************************************/
void MouseLookCB(long ID, short hittype, C_Base *control)
{
    if ((hittype not_eq C_TYPE_LMOUSEUP))
    {
        return;
    }

    PlayerOptions.SetMouseLook( not PlayerOptions.GetMouseLook());
}

/************************************************************************/
// "Enable Touch-buddy" callback function
/************************************************************************/
void TouchBuddyCB(long ID, short hittype, C_Base *control)
{
    if ((hittype not_eq C_TYPE_LMOUSEUP))
    {
        return;
    }

    C_Button *b = static_cast<C_Button*>(control);
    PlayerOptions.SetTouchBuddy(b->GetState() == C_STATE_1);
}

/************************************************************************/
// "Enable Force Feedback" callback function. If no FFB device is present
// (or no FFB device mapped as flight control device) then the button
// stays always "unlit"
/************************************************************************/
void EnableFFBCB(long ID, short hittype, C_Base *control)
{
    if ((hittype not_eq C_TYPE_LMOUSEUP))
        return;

    C_Button* button = (C_Button*)control;

    if (button)
        if (hasForceFeedback == TRUE)
            PlayerOptions.SetFFB( not PlayerOptions.GetFFB());
        else
            button->SetState(C_STATE_0);
}

/************************************************************************/
// Retro 14Feb2004
// Toggle Clickable Mode in 3d cockpit
// FALSE if player should enter 3d pit in 'panning mode' (ie mouse slews the
// view), TRUE if mouse can manipulate cockpit controls
/************************************************************************/
void ToggleClickableModeCB(long ID, short hittype, C_Base *control)
{
    if ((hittype not_eq C_TYPE_LMOUSEUP))
        return;

    C_Button* button = (C_Button*)control;

    if (button)
        PlayerOptions.SetClickablePitMode( not PlayerOptions.GetClickablePitMode());
}

/************************************************************************/
// "Enable 2D TrackIR" callback function. If no TIR present, or if NP
// software was not active at startup, the button stays always unlit
// and the user won´t be able to make a change to that option.
/************************************************************************/
void TrackIR2dCB(long ID, short hittype, C_Base *control)
{
    if ((hittype not_eq C_TYPE_LMOUSEUP))
        return;

    C_Button* button = (C_Button*)control;

    if (button)
        if (g_bEnableTrackIR)
            PlayerOptions.SetTrackIR2d( not PlayerOptions.Get2dTrackIR());
        else
            button->SetState(C_STATE_0);
}

/************************************************************************/
// "Enable 3D TrackIR" callback function. If no TIR present, or if NP
// software was not active at startup, the button stays always unlit
// and the user won´t be able to make a change to that option.
/************************************************************************/
void TrackIR3dCB(long ID, short hittype, C_Base *control)
{
    if ((hittype not_eq C_TYPE_LMOUSEUP))
        return;

    C_Button* button = (C_Button*)control;

    if (button)
        if (g_bEnableTrackIR)
            PlayerOptions.SetTrackIR3d( not PlayerOptions.Get3dTrackIR());
        else
            button->SetState(C_STATE_0);
}

/************************************************************************/
//
/************************************************************************/
void AxisShapingCB(long ID, short hittype, C_Base *control)
{
    if ((hittype not_eq C_TYPE_LMOUSEUP))
        return;

    C_Button* button = (C_Button*)control;

    if (button)
        if (PlayerOptions.GetAxisShaping() == false)
        {
            int result = IO.LoadAxisCalibrationFile();

            if (result == TRUE)
                PlayerOptions.SetAxisShaping(true);
            else // don´t change the options but set the button back to 'unlit'
                button->SetState(C_STATE_0);
        }
        else
            PlayerOptions.SetAxisShaping(false);
}

/************************************************************************/
// Retro 15Jan2004
// Callback function for the mouselook axis sensitivity slider
// this applies equally to mouse x and mouse y axis
/************************************************************************/
void MouseLookSensitivityCB(long ID, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_MOUSEMOVE)
        return;

    if ( not control)
    {
        ShiAssert(false);
        return;
    }

    int smin, smax, pos;

    smax = ((C_Slider *)control)->GetSliderMax();
    smin = ((C_Slider *)control)->GetSliderMin();
    pos  = ((C_Slider *)control)->GetSliderPos();

    // if mouselook is disabled don´t allow the ball to move..
    if (PlayerOptions.GetMouseLook() == false)
    {
        pos = (int) RESCALE(PlayerOptions.GetMouseLookSensitivity() * 1000, g_nMouseLookSensMin, g_nMouseLookSensMax, smin, smax);
        ((C_Slider *)control)->SetSliderPos(pos);
        return;
    }

    float theSens = RESCALE(pos, smin, smax, g_nMouseLookSensMin, g_nMouseLookSensMax);

    theSens /= 1000;

    PlayerOptions.SetMouseLookSensitivity(theSens);
}

/************************************************************************/
// Retro 17Jan2004
// Callback function for the mousewheel (z) axis sensitivity slider
/************************************************************************/
void MouseWheelSensitivityCB(long ID, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_MOUSEMOVE)
        return;

    if ( not control)
    {
        ShiAssert(false);
        return;
    }

    int smin, smax, pos;

    smax = ((C_Slider *)control)->GetSliderMax();
    smin = ((C_Slider *)control)->GetSliderMin();
    pos  = ((C_Slider *)control)->GetSliderPos();

    // if no mouse wheel detected don´t allow the ball to move..
    if (IO.MouseWheelExists() == false)
    {
        pos = (int) RESCALE(PlayerOptions.GetMouseWheelSensitivity(), g_nMouseWheelSensMin, g_nMouseWheelSensMax, smin, smax);
        ((C_Slider *)control)->SetSliderPos(pos);
        return;
    }

    PlayerOptions.SetMouseWheelSensitivity((int)RESCALE(pos, smin, smax, g_nMouseWheelSensMin, g_nMouseWheelSensMax));
}

/************************************************************************/
// Retro 18Jan2004
// Callback function for the POV / Keyboard panning sensitivity slider
/************************************************************************/
void KeyPOVPanningSensitivityCB(long ID, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_MOUSEMOVE)
        return;

    if ( not control)
    {
        ShiAssert(false);
        return;
    }

    int smin, smax, pos;

    smax = ((C_Slider *)control)->GetSliderMax();
    smin = ((C_Slider *)control)->GetSliderMin();
    pos  = ((C_Slider *)control)->GetSliderPos();

    PlayerOptions.SetKeyboardPOVPanningSensitivity((int) RESCALE(pos, smin, smax, g_nKeyPOVSensMin, g_nKeyPOVSensMax));
}

/************************************************************************/
// Marks already mapped axis so that they don´t get drawn in other
// axis listboxes than the one they are mapped to
/************************************************************************/
void MarkMappedAxis()
{
    if ( not gTotalJoy)
        return;

    // mark all unmapped..
    for (int i = 0 ; DIAxisNames[i].DXAxisName; i++)
    {
        DIAxisNames[i].isMapped = false;
    }

    // mark the used ones mapped..
    for (int i = 0; DIAxisNames[i].DXAxisName; i++)
    {
        for (int j = 0; j < AXIS_MAX; j++)
        {
            if (DIAxisNames[i].DXDeviceID == UIInputStuff[j].theDeviceAxis->Device)
            {
                if (DIAxisNames[i].DXAxisID == UIInputStuff[j].theDeviceAxis->Axis)
                {
                    DIAxisNames[i].isMapped = true;
                }
            }
        }

        // flight control axis are not selectable in the advanced control
        // screen, however they are of course mapped.
        if (DIAxisNames[i].DXDeviceID == AxisMap.FlightControlDevice)
        {
            if (DIAxisNames[i].DXAxisID == AxisMap.Pitch.Axis)
            {
                DIAxisNames[i].isMapped = true;
                ShiAssert(AxisMap.Pitch.Device == AxisMap.FlightControlDevice);
            }

            if (DIAxisNames[i].DXAxisID == AxisMap.Bank.Axis)
            {
                DIAxisNames[i].isMapped = true;
                ShiAssert(AxisMap.Bank.Device == AxisMap.FlightControlDevice);
            }
        }
    }
}

/************************************************************************/
// Fills axis listboxes with only unused axis. Plus the one axis they are
// mapped to of course.
// the listbox itemdata contains the index of this axis in the array of
// enumerated axis. I need this for saving.
/************************************************************************/
void FillListBox(C_ListBox* theLB, const int theUIAxisIndex)
{
    if (theLB)
    {
        if (gTotalJoy)
        {
            // loop through all enumerated axis and add the unmapped ones..
            for (int i = 0; DIAxisNames[i].DXAxisName; i++)
            {
                theLB->AddItem(i + SIM_JOYSTICK1, C_TYPE_ITEM, DIAxisNames[i].DXAxisName);
                SetListBoxItemData(theLB, i + SIM_JOYSTICK1, 0, i);

                if (DIAxisNames[i].isMapped == false)
                {
                    theLB->SetItemFlags(i + SIM_JOYSTICK1, C_BIT_ENABLED);
                }
                else
                {
                    theLB->SetItemFlags(i + SIM_JOYSTICK1, C_BIT_INVISIBLE);
                }

                // this one is the axis that is mapped to the axis the listbox is about
                // it is of course mapped so we have to do some fancy coding..
                if (DIAxisNames[i].DXDeviceID == UIInputStuff[theUIAxisIndex].theDeviceAxis->Device)
                {
                    if (DIAxisNames[i].DXAxisID == UIInputStuff[theUIAxisIndex].theDeviceAxis->Axis)
                    {
                        // Add the axis and hilight it
                        theLB->SetItemFlags(i + SIM_JOYSTICK1, C_BIT_ENABLED);
                        theLB->SetValue(i + SIM_JOYSTICK1);
                        ShiAssert(DIAxisNames[i].isMapped == true);
                    }
                }
            }

            theLB->Refresh();
        }
    }
    else
    {
        ShiAssert(false);
    }
}

/************************************************************************/
// Clear all axis listboxes and refill them. Only axis that are unmapped
// (plus the one that´s mapped to this axis) get listed.
//
// SHOULD ONLY BE DONE ONCE 
/************************************************************************/
void PopulateAllListBoxes(C_Window* win)
{
    if (win)
    {
        C_ListBox *listbox;

        for (int UIAxisIndex = 0; UIAxisIndex < AXIS_MAX; UIAxisIndex++)
        {
            if (UIInputStuff[UIAxisIndex].AxisLB == 0)
                continue;

            listbox = (C_ListBox *)win->FindControl(UIInputStuff[UIAxisIndex].AxisLB);

            if (listbox)
            {
                // clear em..
                listbox->RemoveAllItems();

                // all have the keyboard available
                listbox->AddItem(1, C_TYPE_ITEM, TXT_KEYBOARD);

                // keyboards have special itemdata.. its ALWAYS on index1 and it has actually
                // TWO of them: the second one tells in which AxisUIListbox it is..
                SetListBoxItemData(listbox, 1, 0, -1);
                SetListBoxItemData(listbox, 1, 1, UIAxisIndex);

                listbox->SetValue(1);

                // fill it with other available (unmapped) axis
                FillListBox(listbox, UIAxisIndex);

            }
            else
                ShiAssert(false);
        }
    }
    else
    {
        ShiAssert(false);
    }
}

/************************************************************************/
// Clear all axis listboxes and refill them. Only axis that are unmapped
// (plus the one that´s mapped to this axis) get listed.
//
/************************************************************************/
void RePopulateAllListBoxes(C_Window* win)
{
    if (win)
    {
        C_ListBox *listbox;

        for (int UIAxisIndex = 0; UIAxisIndex < AXIS_MAX; UIAxisIndex++)
        {

            if (UIInputStuff[UIAxisIndex].AxisLB == 0)
                continue;

            listbox = (C_ListBox *)win->FindControl(UIInputStuff[UIAxisIndex].AxisLB);

            if (listbox)
            {
                if (gTotalJoy)
                {
                    // loop through all enumerated axis and add the unmapped ones..
                    for (int i = 0; DIAxisNames[i].DXAxisName; i++)
                    {
                        if (DIAxisNames[i].isMapped == false)
                        {
                            // SetListBoxItemData(listbox,i+SIM_JOYSTICK1,0,i);
                            listbox->SetItemFlags(i + SIM_JOYSTICK1, C_BIT_ENABLED);
                        }
                        else
                        {
                            listbox->SetItemFlags(i + SIM_JOYSTICK1, C_BIT_INVISIBLE);
                        }

                        // this one is the axis that is mapped to the axis the listbox is about
                        // it is of course mapped so we have to do some fancy coding..
                        if (DIAxisNames[i].DXDeviceID == UIInputStuff[UIAxisIndex].theDeviceAxis->Device)
                        {
                            if (DIAxisNames[i].DXAxisID == UIInputStuff[UIAxisIndex].theDeviceAxis->Axis)
                            {
                                // Add the axis and hilight it
                                listbox->SetItemFlags(i + SIM_JOYSTICK1, C_BIT_ENABLED);
                                // SetListBoxItemData(listbox,i+SIM_JOYSTICK1,0,i);
                                listbox->SetValue(i + SIM_JOYSTICK1);
                                ShiAssert(DIAxisNames[i].isMapped == true);
                            }
                        }
                    }

                    listbox->Refresh();
                }
            }
            else
            {
                ShiAssert(false);
            }
        }
    }
    else
    {
        ShiAssert(false);
    }
}

/************************************************************************/
// Shared callback function for the axis listboxes
// I find out in what listbox I am by querying the SECOND itemdata
// of the 'keyboard' listbox entry (which ALWAYS is at index 1)
// then I look if an axis actually changed (or if the user clicked on the
// currently mapped one, and only then I handle the whole housekeeping
// stuff like setting 'activation' flags etc. At the end, I cause ALL
// axis listboxes to refresh so that only currently unmapped listboxes
// are shown.
/************************************************************************/
void AxisChangeCB(long, short hittype, C_Base *me)
{
    if ((hittype not_eq C_TYPE_SELECT))
        return;

    if ( not me)
        return;

    C_ListBox *listbox = (C_ListBox*)me;

    if ( not listbox)
        return;

    /* pointer to mommy */
    C_Window *win;

    win = gMainHandler->FindWindow(SETUP_WIN);

    if (win == NULL) return;

    win = gMainHandler->FindWindow(SETUP_CONTROL_ADVANCED_WIN);

    if ( not win) return;

    // k now I figure we can start working ;)

    // looking in the keyboard itemdata for the index of the Axis I am in..
    int i = GetListBoxItemData(listbox, 1, 1);

    if (i == -2) // whoops, error in the above routine..
        return;

    // now I know in which axis box I am 
    long index = GetListBoxItemData(listbox, 0);

    if (index == -2) // whoops, error in the above routine..
        return;

    if ((index == -1) and (UIInputStuff[i].theDeviceAxis->Device == -1) and (UIInputStuff[i].theDeviceAxis->Axis == -1))
    {
        return; // no change
    }
    else if (index == -1)
    {
        // change from an axis to keyboard
        // now how do I find out the old axis ? hmm
        for (int j = 0; DIAxisNames[j].DXAxisName; j++)
        {
            if ((DIAxisNames[j].DXDeviceID == UIInputStuff[i].theDeviceAxis->Device) and 
                (DIAxisNames[j].DXAxisID == UIInputStuff[i].theDeviceAxis->Axis))
            {
                ShiAssert(DIAxisNames[j].isMapped == true);

                DIAxisNames[j].isMapped = false;
                UIInputStuff[i].theDeviceAxis->Device = -1;
                UIInputStuff[i].theDeviceAxis->Axis = -1;

                AdvancedControlApplyCB(0, C_TYPE_LMOUSEUP, 0); // Retro 27Mar2004

                RePopulateAllListBoxes(win);
            }
        }

        return;
    }
    else
    {
        // was not keyboard
        // so we have the change from one axis to another axis
        if ((DIAxisNames[index].DXDeviceID == UIInputStuff[i].theDeviceAxis->Device) and 
            (DIAxisNames[index].DXAxisID == UIInputStuff[i].theDeviceAxis->Axis))
        {
            ShiAssert(DIAxisNames[index].isMapped == true);
            return; // no change
        }
        else
        {
            // super-special exception case:
            // I don´t want the mouse axis to act as a throttle 
            if (DIAxisNames[index].DXDeviceID == SIM_MOUSE)
            {
                if ((UIInputStuff[i].AxisLB == SETUP_ADVANCED_THROTTLE_AXIS) or
                    (UIInputStuff[i].AxisLB == SETUP_ADVANCED_THROTTLE2_AXIS))
                {
                    // NEED TO RESET THE OLD NAME HERE 
                    RePopulateAllListBoxes(win); // no change 
                    return; // tadaa 
                }
            }

            // change from axis to another axis
            DIAxisNames[index].isMapped = true;

            // now how do I find out the old axis ? hmm
            for (int j = 0; DIAxisNames[j].DXAxisName; j++)
            {
                if ((DIAxisNames[j].DXDeviceID == UIInputStuff[i].theDeviceAxis->Device) and 
                    (DIAxisNames[j].DXAxisID == UIInputStuff[i].theDeviceAxis->Axis))
                {
                    DIAxisNames[j].isMapped = false;
                }
            }

            UIInputStuff[i].theDeviceAxis->Device = DIAxisNames[index].DXDeviceID;
            UIInputStuff[i].theDeviceAxis->Axis = DIAxisNames[index].DXAxisID;

            AdvancedControlApplyCB(0, C_TYPE_LMOUSEUP, 0); // Retro 27Mar2004

            RePopulateAllListBoxes(win);
            return;
        }
    }

    ShiAssert(false); // should never come here
}

/************************************************************************/
/* prepares the advanced win and displays it */
/************************************************************************/
void AdvancedControlCB(long, short hittype, C_Base *)
{

    if ((hittype not_eq C_TYPE_LMOUSEUP))
        return;

    /* pointer to mommy */
    C_Window *win;

    /* array of pointers to all axis listboxes in this sheet */
    C_ListBox *listbox;

    win = gMainHandler->FindWindow(SETUP_WIN);

    if (win == NULL) return;

    win = gMainHandler->FindWindow(SETUP_CONTROL_ADVANCED_WIN);

    if ( not win) return;

    MarkMappedAxis();

    PopulateAllListBoxes(win);

    // fill and init the deadzone listboxes..
    listbox = (C_ListBox*)0;

    for (int j = 0; j < AXIS_MAX; j++)
    {
        if (UIInputStuff[j].DeadzoneLB == 0)
            continue;

        listbox = (C_ListBox*)win->FindControl(UIInputStuff[j].DeadzoneLB);

        if (listbox)
        {
            int theDead = UIInputStuff[j].theDeviceAxis->Deadzone;

            if (theDead <= g_nDeadzoneSmall) // 1%
                listbox->SetValue(SETUP_ADVANCED_DZ_SMALL);
            else if (theDead <= g_nDeadzoneMedium) // 5%
                listbox->SetValue(SETUP_ADVANCED_DZ_MEDIUM);
            else if (theDead <= g_nDeadzoneLarge) // 10%
                listbox->SetValue(SETUP_ADVANCED_DZ_LARGE);
            else // whatever (greater than 10%)
                listbox->SetValue(SETUP_ADVANCED_DZ_HUGE);
        }
        else
            ShiAssert(false);
    }

    // fill and init the saturation listboxes..
    listbox = (C_ListBox*)0;

    for (int j = 0; j < AXIS_MAX; j++)
    {

        if (UIInputStuff[j].SaturationLB == 0)
            continue;

        listbox = (C_ListBox*)win->FindControl(UIInputStuff[j].SaturationLB);

        if (listbox)
        {
            int theSat = UIInputStuff[j].theDeviceAxis->Saturation;

            if (theSat == SATURATION_NONE) // no saturation. this value is -1  (do not use unsigned with this )
                listbox->SetValue(SETUP_ADVANCED_SAT_NONE);
            else if (theSat >= g_nSaturationSmall) // 1% saturation, I 'borrowed' the DZ item for this
                listbox->SetValue(SETUP_ADVANCED_DZ_SMALL);
            else if (theSat >= g_nSaturationMedium)
                listbox->SetValue(SETUP_ADVANCED_DZ_MEDIUM);// 5% saturation, I 'borrowed' the DZ item for this
            else // whatever (smaller than 9500) (MEDIUM)
                listbox->SetValue(SETUP_ADVANCED_DZ_LARGE); // 10% saturation, I 'borrowed' the DZ item for this
        }
        else
            ShiAssert(false);
    }

    C_Button *button;

    // 'reversed' buttons..
    for (int j = 0; j < AXIS_MAX; j++) // Retro 15Jan2004
    {

        if (UIInputStuff[j].ReverseBtn == 0)
            continue;

        button = (C_Button *)win->FindControl(UIInputStuff[j].ReverseBtn);

        if (button not_eq NULL)
        {
            if (IO.AnalogIsReversed(UIInputStuff[j].InGameAxis) == true)
                button->SetState(C_STATE_1);
            else
                button->SetState(C_STATE_0);

            button->Refresh();
        }
        else
            ShiAssert(false);
    }

    // register tab callbacks
    button = (C_Button *)win->FindControl(SETUP_ADVANCED_FLIGHT_TAB);

    if (button not_eq NULL)
    {
        button->SetState(C_STATE_0); // 'unpress' this tab
        button->SetCallback(SetupControlTabsCB);
    }
    else
        ShiAssert(false);

    button = (C_Button *)win->FindControl(SETUP_ADVANCED_AVIONICS_TAB);

    if (button not_eq NULL)
    {
        button->SetState(C_STATE_0); // 'unpress' this tab
        button->SetCallback(SetupControlTabsCB);
    }
    else
        ShiAssert(false);

    button = (C_Button *)win->FindControl(SETUP_ADVANCED_SOUND_TAB);

    if (button not_eq NULL)
    {
        button->SetState(C_STATE_0); // 'unpress' this tab
        button->SetCallback(SetupControlTabsCB);
    }
    else
        ShiAssert(false);

    button = (C_Button *)win->FindControl(SETUP_ADVANCED_GENERAL_TAB);

    if (button not_eq NULL)
    {
        button->SetState(C_STATE_1); // fudging the 'General' tab to be pressed
        button->SetCallback(SetupControlTabsCB);

        //make sure general tab is selected on entering
        SetupControlTabsCB(SETUP_ADVANCED_GENERAL_TAB, C_TYPE_LMOUSEUP, button);
    }
    else
        ShiAssert(false);

    button = (C_Button*)win->FindControl(SETUP_ADVANCED_ENABLE_MOUSELOOK);

    if (button not_eq NULL)
    {
        if (PlayerOptions.GetMouseLook() == true)
            button->SetState(C_STATE_1);
        else
            button->SetState(C_STATE_0);

        button->SetCallback(MouseLookCB);
        button->Refresh();
    }
    else
        ShiAssert(false);

    // touch buddy callback
    button = (C_Button*)win->FindControl(SETUP_ADVANCED_ENABLE_TOUCHBUDDY);

    if (button not_eq NULL)
    {
        button->SetState(PlayerOptions.GetTouchBuddy() == true ? C_STATE_1 : C_STATE_0);
        button->SetCallback(TouchBuddyCB);
        button->Refresh();
    }
    else
    {
        ShiAssert(false);
    }


    // TrackIR callbacks.. check the funcs itself for more explanation
    button = (C_Button*)win->FindControl(SETUP_ADVANCED_ENABLE_2DTIR);

    if (button not_eq NULL)
    {
        if ((g_bEnableTrackIR == true) and (PlayerOptions.Get2dTrackIR() == true))
            button->SetState(C_STATE_1);
        else
            button->SetState(C_STATE_0);

        button->SetCallback(TrackIR2dCB);
        button->Refresh();
    }
    else
        ShiAssert(false);

    // TrackIR callbacks.. check the funcs itself for more explanation
    button = (C_Button*)win->FindControl(SETUP_ADVANCED_ENABLE_3DTIR);

    if (button not_eq NULL)
    {
        if ((g_bEnableTrackIR == true) and (PlayerOptions.Get3dTrackIR() == true))
            button->SetState(C_STATE_1);
        else
            button->SetState(C_STATE_0);

        button->SetCallback(TrackIR3dCB);
        button->Refresh();
    }
    else
        ShiAssert(false);

    // Retro 27Jan2004 - Axisshaping button + callback
    button = (C_Button*)win->FindControl(SETUP_ADVANCED_ENABLE_AXISSHAPING);

    if (button not_eq NULL)
    {
        if (PlayerOptions.GetAxisShaping() == true)
            button->SetState(C_STATE_1);
        else
            button->SetState(C_STATE_0);

        button->SetCallback(AxisShapingCB);
        button->Refresh();
    }
    else
        ShiAssert(false);

    // Retro 27Jan2004 end

    // this config var doesn´t influence loading or unloading FFB effects,
    // however effect playback is (de)activated on it
    // still need to look at centering though
    button = (C_Button*)win->FindControl(SETUP_ADVANCED_ENABLE_FFB);

    if (button not_eq NULL)
    {
        if ((hasForceFeedback) and (PlayerOptions.GetFFB()))
            button->SetState(C_STATE_1);
        else
            button->SetState(C_STATE_0);

        button->SetCallback(EnableFFBCB);
        button->Refresh();
    }
    else
        ShiAssert(false);

    // Retro 14Feb2004 - this button governs if mouselook or 3d clickable cockpit
    // is activated when entering the 3d cockpit the first time.
    button = (C_Button*)win->FindControl(SETUP_ADVANCED_3DCOCKPIT_DEFAULT);

    if (button not_eq NULL)
    {
        if (PlayerOptions.GetClickablePitMode()) // if TRUE then we´re in 'clickable mode'
            button->SetState(C_STATE_1);
        else
            button->SetState(C_STATE_0);

        button->SetCallback(ToggleClickableModeCB);
        button->Refresh();
    }
    else
        ShiAssert(false);

    // Retro 15Jan2004 - mouselook sensitivity slider
    C_Slider *sldr;

    sldr = (C_Slider*)win->FindControl(SETUP_ADVANCED_MOUSELOOK_SENS);

    if (sldr)
    {
        if ((PlayerOptions.GetMouseLookSensitivity() * 1000) < g_nMouseLookSensMin)
            PlayerOptions.SetMouseLookSensitivity(g_nMouseLookSensMin / 1000.f);

        if ((PlayerOptions.GetMouseLookSensitivity() * 1000) > g_nMouseLookSensMax)
            PlayerOptions.SetMouseLookSensitivity(g_nMouseLookSensMax / 1000.f);

        int smin, smax, pos;

        smax = sldr->GetSliderMax();
        smin = sldr->GetSliderMin();

        pos = (int)RESCALE((PlayerOptions.GetMouseLookSensitivity() * 1000.f), g_nMouseLookSensMin, g_nMouseLookSensMax, smin, smax);

        sldr->SetSliderPos(pos);

        sldr->SetSliderRange(smin, smax);
        sldr->SetCallback(MouseLookSensitivityCB);
        sldr->Refresh();

    }
    else
        ShiAssert(false);

    // Retro 15Jan2004 ends

    // Retro 17JAn2004 - mousewheel sensitivity slider
    sldr = (C_Slider*)win->FindControl(SETUP_ADVANCED_MOUSEWHEEL_SENS);

    if (sldr)
    {
        if (PlayerOptions.GetMouseWheelSensitivity() < g_nMouseWheelSensMin)
            PlayerOptions.SetMouseWheelSensitivity(g_nMouseWheelSensMin);

        if (PlayerOptions.GetMouseWheelSensitivity() > g_nMouseWheelSensMax)
            PlayerOptions.SetMouseWheelSensitivity(g_nMouseWheelSensMax);

        int smin, smax, pos;

        smax = sldr->GetSliderMax();
        smin = sldr->GetSliderMin();

        pos = (int) RESCALE(PlayerOptions.GetMouseWheelSensitivity(), g_nMouseWheelSensMin, g_nMouseWheelSensMax, smin, smax);

        sldr->SetSliderPos(pos);

        sldr->SetSliderRange(smin, smax);
        sldr->SetCallback(MouseWheelSensitivityCB);
        sldr->Refresh();

    }
    else
        ShiAssert(false);

    // Retro 17Jan2004 ends

    // Retro 18Jan2004 - keyboard / POV panning sensitivity slider
    sldr = (C_Slider*)win->FindControl(SETUP_ADVANCED_KEYPOV_SENS);

    if (sldr)
    {
        if (PlayerOptions.GetKeyboardPOVPanningSensitivity() < g_nKeyPOVSensMin)
            PlayerOptions.SetKeyboardPOVPanningSensitivity(g_nKeyPOVSensMin);

        if (PlayerOptions.GetKeyboardPOVPanningSensitivity() > g_nKeyPOVSensMax)
            PlayerOptions.SetKeyboardPOVPanningSensitivity(g_nKeyPOVSensMax);

        int smin, smax, pos;

        smax = sldr->GetSliderMax();
        smin = sldr->GetSliderMin();

        pos = (int) RESCALE(PlayerOptions.GetKeyboardPOVPanningSensitivity(), g_nKeyPOVSensMin, g_nKeyPOVSensMax, smin, smax);

        sldr->SetSliderPos(pos);

        sldr->SetSliderRange(smin, smax);
        sldr->SetCallback(KeyPOVPanningSensitivityCB);
        sldr->Refresh();

    }
    else
        ShiAssert(false);

    // Retro 18Jan2004 ends

    // register callbacks for the buttons found on this sheet..
#define NO_EXTRA_WIDGETS // Retro 27Mar2004

    button = (C_Button*)win->FindControl(AAPPLY); // this is actually the OK button

    if (button)
        button->SetCallback(AdvancedControlOKCB);

#ifndef NO_EXTRA_WIDGETS // Retro 27Mar2004
    button = (C_Button*)win->FindControl(APPLY); // the 'real' apply button

    if (button)
        button->SetCallback(AdvancedControlApplyCB);

    button = (C_Button*)win->FindControl(CANCEL); // the 'x' widget in the upper left corner

    if (button)
        button->SetCallback(AdvancedControlCancelCB);

#endif // NO_EXTRA_WIDGETS

    // register callbacks for the axis listboxes..
    for (int j = 0; j < AXIS_MAX; j++)
    {
        if (UIInputStuff[j].AxisLB == 0)
            continue;

        listbox = (C_ListBox*)win->FindControl(UIInputStuff[j].AxisLB);

        if (listbox)
            listbox->SetCallback(AxisChangeCB);
        else
            ShiAssert(false);
    }

    InitializeValueBars = 1; // Retro 26Dec2003

    /* make it official */
    gMainHandler->ShowWindow(win);
    gMainHandler->WindowToFront(win);
}
///////////**************************************/////////////////
// Retro ends
///////////**************************************/////////////////

void SetABDetentCB(long, short hittype, C_Base *)
{
    // Retro 1Feb2004
    if ((hittype == C_TYPE_LMOUSEUP))
    {
        setABdetent = TRUE;
    }
    else if ((hittype == C_TYPE_RMOUSEDOWN))
    {
        setIdleCutoff = TRUE;
    }
    else
    {
        return; // do nothing
    }
}

void RefreshJoystickCB(long, short, C_Base *)
{

    static SIM_FLOAT JoyXPrev, JoyYPrev, RudderPrev, ThrottlePrev, ABDetentPrev;
    static SIM_FLOAT IdleCutoffPrev; // Retro 1Feb2004
    static DWORD ButtonPrev[SIMLIB_MAX_DIGITAL * SIM_NUMDEVICES], POVPrev; // Retro 31Dec2003

    static int state = 1; // Retro 26Dec2003
    C_Bitmap *bmap;
    C_Window *win;
    C_Line *line;
    C_Button *button;

    GetJoystickInput();

    // Retro 14Feb2004 - autocenter
    if ((hasForceFeedback) and (PlayerOptions.GetFFB()))
    {
        JoystickPlayEffect(JoyAutoCenter, 10000);
    }

#define UPDATE_ALWAYS // Retro 13Jan2004

    win = gMainHandler->FindWindow(SETUP_WIN);

    if (win not_eq NULL)
    {
#ifndef UPDATE_ALWAYS // Retro 13Jan2004

        //test to see if joystick moved, if so update the control
        if ((IO.analog[AXIS_ROLL].engrValue not_eq JoyXPrev) or (IO.analog[AXIS_PITCH].engrValue not_eq JoyYPrev) or InitializeValueBars) // Retro 31Dec2003
#endif
        {
            bmap = (C_Bitmap *)win->FindControl(JOY_INDICATOR);

            if (bmap not_eq NULL)
            {
                bmap->Refresh();
                bmap->SetX((int)(JoyScale + IO.analog[AXIS_ROLL].engrValue * JoyScale)); // Retro 31Dec2003
                bmap->SetY((int)(JoyScale + IO.analog[AXIS_PITCH].engrValue * JoyScale)); // Retro 31Dec2003
                bmap->Refresh();
                win->RefreshClient(1);
            }
        }

        if (IO.AnalogIsUsed(AXIS_YAW)) // Retro 31Dec2003
        {
#ifndef UPDATE_ALWAYS // Retro 13Jan2004

            //test to see if rudder moved, if so update the control
            if (((IO.analog[AXIS_YAW].engrValue not_eq RudderPrev) or InitializeValueBars) and state) // Retro 31Dec2003
#endif
            {
                line = (C_Line *)win->FindControl(RUDDER);

                if (line not_eq NULL)
                {
                    line->Refresh();
                    line->SetY((int)(Rudder.top + RudderScale - IO.analog[AXIS_YAW].engrValue * RudderScale + .5)); // Retro 31Dec2003

                    if (line->GetY() < Rudder.top)
                        line->SetY(Rudder.top);

                    if (line->GetY() > Rudder.bottom)
                        line->SetY(Rudder.bottom);

                    line->SetH(Rudder.bottom - line->GetY());
                    line->Refresh();
                }
            }
        }

        if (IO.AnalogIsUsed(AXIS_THROTTLE)) // Retro 31Dec2003
        {
            //test to see if throttle moved, if so update the control
#ifndef UPDATE_ALWAYS // Retro 13Jan2004
            if (((IO.analog[AXIS_THROTTLE].engrValue not_eq ThrottlePrev) or InitializeValueBars) and state) // Retro 31Dec2003
#endif
            {
                line = (C_Line *)win->FindControl(THROTTLE);

                if (line not_eq NULL)
                {
                    line->Refresh();
                    line->SetY(FloatToInt32(static_cast<float>(Throttle.top + IO.analog[AXIS_THROTTLE].ioVal / 15000.0F * ThrottleScale + .5))); // Retro 31Dec2003

                    if (line->GetY() < Throttle.top)
                        line->SetY(Throttle.top);

                    if (line->GetY() > Throttle.bottom)
                        line->SetY(Throttle.bottom);

                    line->SetH(Throttle.bottom - line->GetY());
                    line->Refresh();
                }
            }

            // Retro 13Jan2004 - dual throttle display =)
            line = (C_Line *)win->FindControl(THROTTLE2);

            if (line not_eq NULL)
            {
                line->Refresh();

                if (IO.AnalogIsUsed(AXIS_THROTTLE2))
                    line->SetY(FloatToInt32(static_cast<float>(Throttle.top + IO.analog[AXIS_THROTTLE2].ioVal / 15000.0F * ThrottleScale + .5))); // Retro 31Dec2003
                else
                    line->SetY(FloatToInt32(static_cast<float>(Throttle.top + IO.analog[AXIS_THROTTLE].ioVal / 15000.0F * ThrottleScale + .5))); // Retro 31Dec2003

                if (line->GetY() < Throttle.top)
                    line->SetY(Throttle.top);

                if (line->GetY() > Throttle.bottom)
                    line->SetY(Throttle.bottom);

                line->SetH(Throttle.bottom - line->GetY());
                line->Refresh();
            }

            // Retro 13Jan2004 end

#ifndef UPDATE_ALWAYS // Retro 13Jan2004

            if (ABDetentPrev not_eq IO.analog[AXIS_THROTTLE].center or InitializeValueBars) // Retro 31Dec2003
#endif
            {
                line = (C_Line *)win->FindControl(AB_DETENT);

                if (line not_eq NULL)
                {
                    line->Refresh();
                    line->SetY(FloatToInt32(static_cast<float>(Throttle.top + IO.analog[AXIS_THROTTLE].center / 15000.0F * ThrottleScale + .5))); // Retro 31Dec2003

                    if (line->GetY() <= Throttle.top - 1)
                        line->SetY(Throttle.top);

                    if (line->GetY() >= Throttle.bottom)
                        line->SetY(Throttle.bottom + 1);

                    line->Refresh();
                }
            }

#ifndef UPDATE_ALWAYS // Retro 13Jan2004

            if (IdleCutoffPrev not_eq IO.analog[AXIS_THROTTLE].cutoff or InitializeValueBars) // Retro 31Dec2003
#endif
            {
                line = (C_Line *)win->FindControl(SETUP_IDLE_CUTOFF);

                if (line not_eq NULL)
                {
                    line->Refresh();
                    line->SetY(FloatToInt32(static_cast<float>(Throttle.top + IO.analog[AXIS_THROTTLE].cutoff / 15000.0F * ThrottleScale + .5))); // Retro 31Dec2003

                    if (line->GetY() <= Throttle.top - 1)
                        line->SetY(Throttle.top);

                    if (line->GetY() >= Throttle.bottom)
                        line->SetY(Throttle.bottom + 1);

                    line->Refresh();
                }
            }
        }


        unsigned long i;

        for (i = 0; i < SIMLIB_MAX_DIGITAL * SIM_NUMDEVICES; i++) // Retro 31Dec2003
        {
            // Retro 31Dec2003:
            // actually I only want to show the buttons on the flight control device here..
            // if FFB is enabled then the user also gets effects
            if (AxisMap.FlightControlDevice not_eq -1)
            {
                int theIndex = (AxisMap.FlightControlDevice - SIM_JOYSTICK1) * SIMLIB_MAX_DIGITAL;

                if ((i >= (unsigned long) theIndex) and (i  < (unsigned long) theIndex + 8))
                {
                    // Retro 14Feb2004 - only do this when FFB is available (duh)
                    if ((hasForceFeedback) and (PlayerOptions.GetFFB()))
                    {
                        if (IO.digital[theIndex])
                        {
                            JoyEffectPlaying = true;
                            JoystickPlayEffect(JoyFireEffect, 0);
                        }
                        else if (JoyEffectPlaying)
                        {
                            JoystickStopEffect(JoyFireEffect);
                            JoyEffectPlaying = false;
                        }
                    }

                    button = (C_Button *)win->FindControl(J1 + i % 8);

                    if (button not_eq NULL)
                    {
                        if (IO.digital[i])
                            button->SetState(C_STATE_1);
                        else
                            button->SetState(C_STATE_0);

                        button->Refresh();
                    }
                }
            }

            if (IO.digital[i])
            {
                C_Text *text = (C_Text *)win->FindControl(CONTROL_KEYS);

                if (text)
                {
                    char string[_MAX_PATH];
                    text->Refresh();
                    sprintf(string, "%s %d", gStringMgr->GetString(TXT_BUTTON), i + 1);
                    text->SetText(string);
                    text->Refresh();
                }

                if (KeyVar.EditKey)
                {
                    button = (C_Button *)win->FindControl(KeyVar.CurrControl);
                    UserFunctionTable.SetButtonFunction(i, (InputFunctionType)button->GetUserPtr(FUNCTION_PTR), button->GetUserNumber(BUTTON_ID));
                    KeyVar.EditKey = FALSE;
                    KeyVar.Modified = TRUE;
                    SetButtonColor(button);
                }

                text = (C_Text *)win->FindControl(FUNCTION_LIST);

                if (text)
                {
                    InputFunctionType func;
                    char *descrip;

                    text->Refresh();

                    if (func = UserFunctionTable.GetButtonFunction(i, NULL))
                    {
                        int i = 0;

                        C_Button *tButton = (C_Button *)win->FindControl(KEYCODES);

                        while (tButton)
                        {
                            if (func == (InputFunctionType)tButton->GetUserPtr(5))
                            {
                                C_Text *temp = (C_Text *)win->FindControl(tButton->GetID() - KEYCODES + MAPPING);

                                if (temp)
                                {
                                    descrip = temp->GetText();

                                    if (descrip)
                                    {
                                        text->SetText(descrip);
                                        break;
                                    }
                                }

                                text->SetText("");
                                break;
                            }
                            else
                            {
                                tButton = (C_Button *)win->FindControl(KEYCODES + i++);
                            }
                        }
                    }
                    else
                    {
                        if (i == 0)
                        {
                            text->SetText(TXT_FIRE_GUN);
                        }
                        else if (i == 1)
                        {
                            text->SetText(TXT_FIRE_WEAPON);
                        }
                        else
                        {
                            text->SetText(TXT_NO_FUNCTION);
                        }
                    }

                    text->Refresh();
                }
            }
        }

        int Direction;
        int flags = 0;

        for (i = 0; i < NumberOfPOVs; i++) // Retro 26Dec2003
        {
            Direction = 0;

            if ((IO.povHatAngle[i] < 2250 or IO.povHatAngle[i] > 33750) and IO.povHatAngle[i] not_eq -1)
            {
                flags or_eq 0x01;
                Direction = 0;
            }
            else if (IO.povHatAngle[i] < 6750)
            {
                flags or_eq 0x03;
                Direction = 1;
            }
            else if (IO.povHatAngle[i] < 11250)
            {
                flags or_eq 0x02;
                Direction = 2;
            }
            else if (IO.povHatAngle[i] < 15750)
            {
                flags or_eq 0x06;
                Direction = 3;
            }
            else if (IO.povHatAngle[i] < 20250)
            {
                flags or_eq 0x04;
                Direction = 4;
            }
            else if (IO.povHatAngle[i] < 24750)
            {
                flags or_eq 0x0C;
                Direction = 5;
            }
            else if (IO.povHatAngle[i] < 29250)
            {
                flags or_eq 0x08;
                Direction = 6;
            }
            else if (IO.povHatAngle[i] < 33750)
            {
                flags or_eq 0x09;
                Direction = 7;
            }

            if (KeyVar.EditKey and IO.povHatAngle[i] not_eq -1)
            {
                C_Button *button;

                button = (C_Button *)win->FindControl(KeyVar.CurrControl);
                UserFunctionTable.SetPOVFunction(i, Direction, (InputFunctionType)button->GetUserPtr(FUNCTION_PTR), button->GetUserNumber(BUTTON_ID));
                KeyVar.EditKey = FALSE;
                KeyVar.Modified = TRUE;
                SetButtonColor(button);
            }

            C_Text *text = (C_Text *)win->FindControl(FUNCTION_LIST);

            if (text and IO.povHatAngle[i] not_eq -1)
            {
                C_Text *text2 = (C_Text *)win->FindControl(CONTROL_KEYS);

                if (text2)
                {
                    char button[_MAX_PATH];
                    text2->Refresh();
                    sprintf(button, "%s %d : %s", gStringMgr->GetString(TXT_POV), i + 1, gStringMgr->GetString(TXT_UP + Direction));
                    text2->SetText(button);
                    text2->Refresh();
                }

                InputFunctionType func;
                char *descrip;

                text->Refresh();

                if (func = UserFunctionTable.GetPOVFunction(i, Direction, NULL))
                {
                    int i = 0;

                    C_Button *tButton = (C_Button *)win->FindControl(KEYCODES);

                    while (tButton)
                    {
                        if (func == (InputFunctionType)tButton->GetUserPtr(5))
                        {
                            C_Text *temp = (C_Text *)win->FindControl(tButton->GetID() - KEYCODES + MAPPING);

                            if (temp)
                            {
                                descrip = temp->GetText();

                                if (descrip)
                                {
                                    text->SetText(descrip);
                                    break;
                                }
                            }

                            text->SetText("");
                            break;
                        }
                        else
                        {
                            tButton = (C_Button *)win->FindControl(KEYCODES + i++);
                        }
                    }
                }
                else
                {
                    text->SetText(TXT_NO_FUNCTION);
                }

                text->Refresh();
            }
        }

        button = (C_Button *)win->FindControl(UP_HAT);

        if (button not_eq NULL and button->GetState() not_eq C_STATE_DISABLED)
        {
            if (flags bitand 0x01)
                button->SetState(C_STATE_1);
            else
                button->SetState(C_STATE_0);

            button->Refresh();
        }

        button = (C_Button *)win->FindControl(RIGHT_HAT);

        if (button not_eq NULL and button->GetState() not_eq C_STATE_DISABLED)
        {
            if (flags bitand 0x02)
                button->SetState(C_STATE_1);
            else
                button->SetState(C_STATE_0);

            button->Refresh();
        }

        button = (C_Button *)win->FindControl(DOWN_HAT);

        if (button not_eq NULL and button->GetState() not_eq C_STATE_DISABLED)
        {
            if (flags bitand 0x04)
                button->SetState(C_STATE_1);
            else
                button->SetState(C_STATE_0);

            button->Refresh();
        }

        button = (C_Button *)win->FindControl(LEFT_HAT);

        if (button not_eq NULL and button->GetState() not_eq C_STATE_DISABLED)
        {
            if (flags bitand 0x08)
                button->SetState(C_STATE_1);
            else
                button->SetState(C_STATE_0);

            button->Refresh();
        }

        InitializeValueBars = 0; // Retro 26Dec2003

        JoyXPrev = IO.analog[AXIS_ROLL].engrValue;
        JoyYPrev = IO.analog[AXIS_PITCH].engrValue;
        ThrottlePrev = IO.analog[AXIS_THROTTLE].engrValue;
        RudderPrev = IO.analog[AXIS_YAW].engrValue;
        ABDetentPrev = static_cast<float>(IO.analog[AXIS_THROTTLE].center);
        IdleCutoffPrev = static_cast<float>(IO.analog[AXIS_THROTTLE].cutoff); // Retro 1Feb2004

        POVPrev = IO.povHatAngle[0];
    }

    //if(Calibration.calibrating)
    // Calibrate();

    // Retro - trying to get some of this shit into my advanced controller window..
    win = gMainHandler->FindWindow(SETUP_CONTROL_ADVANCED_WIN);

    if ( not win) return;

    for (int i = 0; i < AXIS_MAX; i++)
    {
        if (UIInputStuff[i].AxisValueBar == 0)
            continue;

        line = (C_Line *)win->FindControl(UIInputStuff[i].AxisValueBar);

        if (line not_eq NULL)
        {
            line->Refresh();

            float newWidth = 0;

            if (IO.AnalogIsUsed(UIInputStuff[i].InGameAxis) == true)
            {
                if (AxisSetup[UIInputStuff[i].InGameAxis].isUniPolar == true)
                    newWidth  = (float) FloatToInt32(static_cast<float>(IO.analog[UIInputStuff[i].InGameAxis].ioVal / 15000.0F * AxisValueBoxWScale + .5));
                else
                    newWidth = (float) FloatToInt32(static_cast<float>(AxisValueBoxWScale / 2. + IO.analog[UIInputStuff[i].InGameAxis].ioVal / 20000.0F * AxisValueBoxWScale + .5));

                if (newWidth < 0)
                {
                    newWidth = 0;
                }

                if (newWidth > AxisValueBoxWScale)
                {
                    newWidth = AxisValueBoxWScale;
                }
            }
            else
            {
                newWidth = 0;
            }

            line->SetW((long)newWidth);
            line->Refresh();
        }
        else
        {
            ShiAssert(false);
        }
    }
}//RefreshJoystickCB

SIM_INT CalibrateFile(void)
{
    int i, numAxis;
    FILE* filePtr;

    char fileName[_MAX_PATH];
    sprintf(fileName, "%s\\config\\joystick.dat", FalconDataDirectory);

    filePtr = fopen(fileName, "rb");

    if (filePtr not_eq NULL)
    {
        fread(&numAxis, sizeof(int), 1, filePtr);

        for (i = 0; i < numAxis; i++)
        {
            fread(&(IO.analog[i]), sizeof(SIMLIB_ANALOG_TYPE), 1, filePtr);
        }

        fclose(filePtr);

        return (TRUE);
    }

    return FALSE;
}
/*
void StopCalibrating(C_Base *control)
{
 C_Text *text;
 C_Button *button;

 Calibration.calibrating = FALSE;
 Calibration.step = 0;
 Calibration.disp_text = TRUE;
 Calibration.state = 1;

 Calibration.calibrated = CalibrateFile();

 text=(C_Text *)control->Parent_->FindControl(CAL_TEXT);
 text->Refresh();
 text->SetFlagBitOn(C_BIT_INVISIBLE);
 text->Refresh();

 text=(C_Text *)control->Parent_->FindControl(CAL_TEXT2);
 text->Refresh();
 text->SetFlagBitOn(C_BIT_INVISIBLE);
 text->Refresh();

 button = (C_Button *)control->Parent_->FindControl(CALIBRATE);
 button->SetState(C_STATE_0);
 button->Refresh();
}*/

/*
SIM_INT Calibrate ( void )
{
 int retval;

 if (S_joyret == JOYERR_NOERROR)
 {
 retval = SIMLIB_OK;
 int size;
 FILE* filePtr;
 C_Base *control;
 C_Window *win;
 C_Text *text,*text2;
 C_Button *button;
 RECT client;

 if(Calibration.state)
 {
 Calibration.state = 0;
 Calibration.disp_text = TRUE;
 //waiting for user to let go of all buttons
 for(int i =0;i < S_joycaps.wNumButtons;i++)
 {
 if(IO.digital[i]) //button pressed
 {
 Calibration.state = 1;
 break;
 }
 }
 }
 else
 {
 win = gMainHandler->FindWindow(SETUP_WIN);
 control = win->FindControl(JOY_INDICATOR);

 text=(C_Text *)win->FindControl(CAL_TEXT);
 text2=(C_Text *)win->FindControl(CAL_TEXT2);

 switch(Calibration.step)
 {
 int i;

 case 0:

 if(Calibration.disp_text)
 {
 MonoPrint ("Center the joystick, throttle, and rudder and push a button.\n");
 if(text not_eq NULL)
 {
 text->Refresh();
 text->SetFlagBitOff(C_BIT_INVISIBLE);
 text->SetText(TXT_CTR_JOY);
 text->Refresh();
 }

 if(text2)
 {
 text2->Refresh();
 text2->SetFlagBitOff(C_BIT_INVISIBLE);
 text2->Refresh();
 }

 if( not Calibration.calibrated)
 {
 IO.analog[0].mUp = IO.analog[0].mDown = IO.analog[0].bUp = IO.analog[0].bDown = 0.0F;
 IO.analog[1].mUp = IO.analog[1].mDown = IO.analog[1].bUp = IO.analog[1].bDown = 1.1F;
 IO.analog[2].mUp = IO.analog[2].mDown = IO.analog[2].bUp = IO.analog[2].bDown = 2.2F;
 IO.analog[3].mUp = IO.analog[3].mDown = IO.analog[3].bUp = IO.analog[3].bDown = 3.3F;
 }

 IO.analog[0].min = IO.analog[1].min = IO.analog[2].min = IO.analog[3].min = 65536.0F;
 IO.analog[0].max = IO.analog[1].max = IO.analog[2].max = IO.analog[3].max = 0.0F;

 IO.analog[0].isUsed = IO.analog[1].isUsed = TRUE;

 if ( not (S_joycaps.wCaps bitand JOYCAPS_HASZ))
 {
 IO.analog[2].isUsed = FALSE;
 IO.analog[2].max = 0;
 IO.analog[2].min = -1;
 IO.analog[2].engrValue = 1.0F;
 }
 else
 {
 IO.analog[2].isUsed = TRUE;
 }

 if ( not (S_joycaps.wCaps bitand JOYCAPS_HASR))
 {
 IO.analog[3].isUsed= FALSE;
 IO.analog[3].max = 1;
 IO.analog[3].min = -1;
 IO.analog[3].engrValue = 0.0F;
 }
 else
 {
 IO.analog[3].isUsed = TRUE;
 }

 IO.analog[2].center = 32768;

 Calibration.disp_text = FALSE;
 }

 IO.analog[0].center = IO.analog[0].ioVal;
 IO.analog[1].center = IO.analog[1].ioVal;
 IO.analog[3].center = IO.analog[3].ioVal;

 for(i =0;i < S_joycaps.wNumButtons;i++)
 {
 if(IO.digital[i])
 Calibration.state = 1; //button pressed
 }

 if(Calibration.state)
 Calibration.step++;

 break;

 case 1:

 if(Calibration.disp_text)
 {
 MonoPrint ("Move the joystick to the corners and push a button.\n");
 if(text not_eq NULL)
 {
 text->Refresh();
 text->SetText(TXT_MV_JOY);
 text->Refresh();
 }
 Calibration.disp_text = FALSE;
 }

 IO.analog[0].max = max(IO.analog[0].max, IO.analog[0].ioVal);
 IO.analog[1].max = max(IO.analog[1].max, IO.analog[1].ioVal);
 IO.analog[0].min = min(IO.analog[0].min, IO.analog[0].ioVal);
 IO.analog[1].min = min(IO.analog[1].min, IO.analog[1].ioVal);

 for(i =0;i < S_joycaps.wNumButtons;i++)
 {
 if(IO.digital[i])
 Calibration.state = 1; //button pressed
 }

 if(Calibration.state)
 Calibration.step++;

 break;

 case 2:

 if (S_joycaps.wCaps bitand JOYCAPS_HASZ)
 {
 if(Calibration.disp_text)
 {
 MonoPrint ("Move the throttle to the ends and push a button\n");
 if(text not_eq NULL and IO.analog[2].isUsed)
 {
 text->Refresh();
 text->SetText(TXT_MV_THR);
 text->Refresh();
 }
 Calibration.disp_text = FALSE;
 }

 IO.analog[2].max = max(IO.analog[2].max, IO.analog[2].ioVal);
 IO.analog[2].min = min(IO.analog[2].min, IO.analog[2].ioVal);

 for(i =0;i < S_joycaps.wNumButtons;i++)
 {
 if(IO.digital[i])
 Calibration.state = 1; //button pressed
 }

 if(Calibration.state)
 Calibration.step++;
 }
 else
 {
 Calibration.step++;
 Calibration.state = 1;
 }
 break;

 case 3:
 if (S_joycaps.wCaps bitand JOYCAPS_HASR)
 {
 if(Calibration.disp_text)
 {
 MonoPrint ("Move the rudder to the ends and push a button\n");
 if(text not_eq NULL and IO.analog[3].isUsed)
 {
 text->Refresh();
 text->SetText(TXT_MV_RUD);
 text->Refresh();
 }
 Calibration.disp_text = FALSE;
 }

 IO.analog[3].max = max(IO.analog[3].max, IO.analog[3].ioVal);
 IO.analog[3].min = min(IO.analog[3].min, IO.analog[3].ioVal);

 for(int i =0;i < S_joycaps.wNumButtons;i++)
 {
 if(IO.digital[i])
 Calibration.state = 1; //button pressed
 }

 if(Calibration.state)
 Calibration.step++;

 }
 else
 {
 Calibration.step++;
 Calibration.state = 1;
 }
 break;

 case 4:
 for (i=0; i<S_joycaps.wNumAxes; i++)
 {
 if (IO.analog[i].isUsed)
 {
 IO.analog[i].mUp = 1.0F /(IO.analog[i].max - IO.analog[i].center);
 IO.analog[i].bUp = -IO.analog[i].mUp * IO.analog[i].center;
 IO.analog[i].mDown = 1.0F / (IO.analog[i].center - IO.analog[i].min);
 IO.analog[i].bDown = -IO.analog[i].mDown * IO.analog[i].center;
 }
 else
 {
 IO.analog[i].mUp = IO.analog[i].mDown = 0.0F;
 IO.analog[i].bUp = IO.analog[i].bDown = 0.0F;
 }
 }

 char filename[_MAX_PATH];
 sprintf(filename,"%s\\config\\joystick.dat",FalconDataDirectory);
 filePtr = fopen (filename, "wb");
 if(filePtr)
 {
 fwrite (&S_joycaps.wNumAxes, sizeof(int), 1, filePtr);

 for (i=0; i<S_joycaps.wNumAxes; i++)
 {
 fwrite (&(IO.analog[i]), sizeof(SIMLIB_ANALOG_TYPE), 1, filePtr);
 }
 fclose (filePtr);
 }
 else
 {
 ShiAssert(filePtr == NULL);
 }

 button = (C_Button *)win->FindControl(CALIBRATE);
 button->SetState(C_STATE_0);
 button->Refresh();



 if(text not_eq NULL)
 {
 text->Refresh();
 text->SetFlagBitOn(C_BIT_INVISIBLE);
 text->Refresh();
 }

 if(text2)
 {
 text2->Refresh();
 text2->SetFlagBitOn(C_BIT_INVISIBLE);
 text2->Refresh();
 }

 client = win->GetClientArea(1);

 if(control)
 {
 size = ((C_Bitmap *)control)->GetH();
 }

 JoyScale = (float)(client.right - client.left - size)/2.0F;
 RudderScale = (Rudder.bottom - Rudder.top )/2.0F;
 ThrottleScale = (Throttle.bottom - Throttle.top )/2.0F;

 Calibration.calibrated = TRUE;
 Calibration.calibrating = FALSE;
 Calibration.step = 0;
 Calibration.state = 1;
 }
 }

 }
 else if (S_joyret == MMSYSERR_NODRIVER)
 {
 SimLibPrintError ("MMSYSERR No Driver");
 return SIMLIB_ERR;
 }
 else if (S_joyret == MMSYSERR_INVALPARAM)
 {
 SimLibPrintError ("MMSYSERR Invalid Parameter");
 return SIMLIB_ERR;
 }

 return retval;
}




void CalibrateCB(long ID,short hittype,C_Base *control)
{
 if((hittype not_eq C_TYPE_LMOUSEUP))
 return;

 Calibration.calibrating = 1;
 //Calibrate();

}//CalibrateCB

*/

//function assumes you have passed a char * that has enough memory allocated
void DoShiftStates(char *mods, int ShiftStates)
{
    int plus = 0;

    if (ShiftStates bitand _SHIFT_DOWN_)
    {
        strcat(mods, gStringMgr->GetString(TXT_SHIFT_KEY));
        plus++;
    }

    if (ShiftStates bitand _CTRL_DOWN_)
    {
        if (plus)
        {
            strcat(mods, "+");
            plus--;
        }

        strcat(mods, gStringMgr->GetString(TXT_CONTROL_KEY));
        plus++;
    }

    if (ShiftStates bitand _ALT_DOWN_)
    {
        if (plus)
        {
            strcat(mods, "+");
            plus--;
        }

        strcat(mods, gStringMgr->GetString(TXT_ALTERNATE_KEY));
        plus++;
    }

    if (plus)
        strcat(mods, " ");
}


BOOL KeystrokeCB(unsigned char DKScanCode, unsigned char, unsigned char ShiftStates, long)
{
    if (DKScanCode == DIK_ESCAPE)
        return FALSE;

    if (Cluster == 8004)
    {
        if (DKScanCode == DIK_LSHIFT or DKScanCode == DIK_RSHIFT or \
            DKScanCode == DIK_LCONTROL or DKScanCode == DIK_RCONTROL or \
            DKScanCode == DIK_LMENU or DKScanCode == DIK_RMENU or \
            DKScanCode == 0x45)
            return TRUE;

        if (GetAsyncKeyState(VK_SHIFT) bitand 0x8001)
            ShiftStates or_eq _SHIFT_DOWN_;
        else
            ShiftStates and_eq compl _SHIFT_DOWN_;

        //int flags = ShiftStates;
        int flags = ShiftStates + (KeyVar.CommandsKeyCombo << SECOND_KEY_SHIFT) + (KeyVar.CommandsKeyComboMod << SECOND_KEY_MOD_SHIFT);

        C_Window *win;
        int CommandCombo = 0;

        win = gMainHandler->FindWindow(SETUP_WIN);

        if (KeyVar.EditKey)
        {
            C_Button *button;
            long ID;

            button = (C_Button *)win->FindControl(KeyVar.CurrControl);
            flags = ShiftStates + (button->GetUserNumber(FLAGS) bitand SECOND_KEY_MASK);
            KeyVar.CommandsKeyCombo = (button->GetUserNumber(FLAGS) bitand KEY1_MASK) >> SECOND_KEY_SHIFT;
            KeyVar.CommandsKeyComboMod = (button->GetUserNumber(FLAGS) bitand MOD1_MASK) >> SECOND_KEY_MOD_SHIFT;

            //here is where we need to change the key combo for the function
            if (DKScanCode not_eq button->GetUserNumber(KEY2) or flags not_eq button->GetUserNumber(FLAGS))
            {
                char keydescrip[_MAX_PATH];
                keydescrip[0] = 0;
                int pmouse, pbutton;
                InputFunctionType theFunc;
                InputFunctionType oldFunc;
                theFunc = (InputFunctionType)button->GetUserPtr(FUNCTION_PTR);

                //is the key combo already used?
                if (oldFunc = UserFunctionTable.GetFunction(DKScanCode, flags, &pmouse, &pbutton))
                {
                    C_Button *temp;

                    ID = UserFunctionTable.GetControl(DKScanCode, flags);
                    KeyVar.OldControl = ID;


                    //there is a function mapped but it's not visible
                    //don't allow user to remap this key combo
                    if ( not ID and oldFunc)
                        return TRUE;


                    temp = (C_Button *)win->FindControl(ID);

                    if (temp and (temp->GetUserNumber(EDITABLE) < 1))
                    {
                        //this keycombo is not remappable
                        return TRUE;
                    }

                    //remove old function from place user wants to use
                    UserFunctionTable.RemoveFunction(DKScanCode, flags);
                }

                //remove function that's being remapped from it's old place
                UserFunctionTable.RemoveFunction(button->GetUserNumber(KEY2), button->GetUserNumber(FLAGS));

                //add function into it's new place
                UserFunctionTable.AddFunction(DKScanCode, flags, button->GetUserNumber(BUTTON_ID), button->GetUserNumber(MOUSE_SIDE), theFunc);
                UserFunctionTable.SetControl(DKScanCode, flags, KeyVar.CurrControl);

                //mark that the keymapping needs to be saved
                KeyVar.Modified = TRUE;

                //setup button with new values
                button->SetUserNumber(KEY2, DKScanCode);
                button->SetUserNumber(FLAGS, flags);

                char mods[40] = {0};
                _TCHAR firstKey[MAX_PATH] = {0};
                _TCHAR totalDescrip[MAX_PATH] = {0};

                // JPO crash log detection.
                ShiAssert(DKScanCode >= 0 and DKScanCode < 256);
                ShiAssert(FALSE == IsBadStringPtr(KeyDescrips[DKScanCode], MAX_PATH));

                if (KeyVar.CommandsKeyCombo > 0)
                {
                    DoShiftStates(firstKey, KeyVar.CommandsKeyComboMod);
                    DoShiftStates(mods, ShiftStates);

                    if (KeyDescrips[DKScanCode])
                        _stprintf(totalDescrip, "%s%s : %s%s", firstKey, KeyDescrips[KeyVar.CommandsKeyCombo], mods, KeyDescrips[DKScanCode]);
                }
                else
                {
                    DoShiftStates(totalDescrip, ShiftStates);

                    if (KeyDescrips[DKScanCode])
                        strcat(totalDescrip, KeyDescrips[DKScanCode]);
                }

                //DoShiftStates(keydescrip,ShiftStates);
                //strcat(keydescrip,KeyDescrips[DKScanCode]);

                button->Refresh();
                button->SetText(0, totalDescrip);
                button->Refresh();

                if (KeyVar.OldControl)
                {
                    //if we unmapped another function to map this one we
                    //need to update the first functions buttton
                    C_Button *temp;

                    temp = (C_Button *)win->FindControl(KeyVar.OldControl);

                    //strcpy(keydescrip,"No Key Assigned");
                    if (temp)
                    {
                        SetButtonColor(temp);
                        temp->SetUserNumber(KEY2, -1);
                        temp->SetUserNumber(FLAGS, temp->GetUserNumber(FLAGS) bitand SECOND_KEY_MASK);
                        temp->Refresh();
                        temp->SetText(0, TXT_NO_KEY);
                        temp->Refresh();
                    }
                }
            }

            SetButtonColor(button);
        }

        if (KeyVar.OldControl)
        {
            // if we stole another functions mapping, move to the function
            // and leave ourselves in edit mode
            C_Button *temp;
            UI95_RECT Client;

            temp = (C_Button *)win->FindControl(KeyVar.OldControl);

            if (temp)
            {
                temp->SetFgColor(0, RGB(0, 255, 255));

                Client = win->GetClientArea(temp->GetClient());

                win->SetVirtualY(temp->GetY() - Client.top, temp->GetClient());
                win->AdjustScrollbar(temp->GetClient());
                win->RefreshClient(temp->GetClient());
            }

            KeyVar.CurrControl = KeyVar.OldControl;
            KeyVar.OldControl = 0;
        }
        else
        {
            //key changed leave edit mode
            KeyVar.EditKey = FALSE;
        }

        C_Text *text;

        if (DKScanCode == 0xC5)
            DKScanCode = 0x45;

        //build description for display at bottom of window
        if (KeyDescrips[DKScanCode])
        {

            //if(DKScanCode == 0x44)

            char mods[40] = {0};
            char *descrip = NULL;
            int pmouse, pbutton;
            InputFunctionType function;

            text = (C_Text *)win->FindControl(CONTROL_KEYS); //CONTROL_KEYS

            if (text)
            {
                _TCHAR firstKey[MAX_PATH] = {0};
                _TCHAR totalDescrip[MAX_PATH] = {0};

                if (KeyVar.CommandsKeyCombo > 0)
                {
                    DoShiftStates(firstKey, KeyVar.CommandsKeyComboMod);
                    DoShiftStates(mods, ShiftStates);
                    _stprintf(totalDescrip, "%s%s : %s%s", firstKey, KeyDescrips[KeyVar.CommandsKeyCombo], mods, KeyDescrips[DKScanCode]);
                }
                else
                {
                    DoShiftStates(totalDescrip, ShiftStates);
                    strcat(totalDescrip, KeyDescrips[DKScanCode]);
                }

                //DoShiftStates(mods,ShiftStates);
                //strcat(mods,KeyDescrips[DKScanCode]);
                text->Refresh();
                text->SetText(totalDescrip);
                text->Refresh();
            }

            //flags = flags + (KeyVar.CommandsKeyCombo << SECOND_KEY_SHIFT) + (KeyVar.CommandsKeyComboMod << SECOND_KEY_MOD_SHIFT);
            function = UserFunctionTable.GetFunction(DKScanCode, flags, &pmouse, &pbutton);

            text = (C_Text *)win->FindControl(FUNCTION_LIST);

            if (text)
            {
                text->Refresh();

                if (function)
                {
                    C_Text *temp;
                    C_Button *btn;
                    long ID;

                    ID = UserFunctionTable.GetControl(DKScanCode, flags) ;

                    CommandCombo = 0;

                    btn = (C_Button *)win->FindControl(ID);

                    if (btn)
                        CommandCombo = btn->GetUserNumber(EDITABLE);

                    if (CommandCombo == -1)
                    {
                        KeyVar.CommandsKeyCombo = DKScanCode;
                        KeyVar.CommandsKeyComboMod = ShiftStates;
                    }
                    else
                    {
                        KeyVar.CommandsKeyCombo = 0;
                        KeyVar.CommandsKeyComboMod = 0;
                    }

                    ID = ID - KEYCODES + MAPPING;
                    temp = (C_Text *)win->FindControl(ID);

                    if (temp)
                        descrip = temp->GetText();

                    if (descrip)
                        text->SetText(descrip);
                    else
                        text->SetText(TXT_NO_FUNCTION);

                    if ( not KeyVar.EditKey and temp)
                    {
                        UI95_RECT Client;
                        Client = win->GetClientArea(temp->GetClient());

                        win->SetVirtualY(temp->GetY() - Client.top, temp->GetClient());
                        win->AdjustScrollbar(temp->GetClient());
                        win->RefreshClient(temp->GetClient());
                    }
                }
                else
                {
                    text->SetText(TXT_NO_FUNCTION);
                    KeyVar.CommandsKeyCombo = 0;

                    KeyVar.CommandsKeyComboMod = 0;
                }

                text->Refresh();
            }
        }

        return TRUE;
    }


    return FALSE;
}

void KeycodeCB(long ID, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (KeyVar.EditKey)
    {
        C_Button *button;

        button = (C_Button *)control->Parent_->FindControl(KeyVar.CurrControl);

        SetButtonColor(button);
    }

    if (KeyVar.CurrControl == ID and KeyVar.EditKey)
    {
        KeyVar.EditKey = FALSE;
        return;
    }

    if (control->GetUserNumber(EDITABLE) < 1)
    {
        KeyVar.CurrControl = ID;
        KeyVar.EditKey = FALSE;
    }
    else
    {
        KeyVar.CurrControl = ID;
        ((C_Button *)control)->SetFgColor(0, RGB(0, 255, 255));
        ((C_Button *)control)->Refresh();
        KeyVar.EditKey = TRUE;
    }

    return;
}

int AddUndisplayedKey(KeyMap &Map)
{
    if (NumUndispKeys < 300)
    {
        UndisplayedKeys[NumUndispKeys].func = Map.func;
        UndisplayedKeys[NumUndispKeys].buttonId = Map.buttonId;
        UndisplayedKeys[NumUndispKeys].mouseSide = Map.mouseSide;
        UndisplayedKeys[NumUndispKeys].key2 = Map.key2;
        UndisplayedKeys[NumUndispKeys].mod2 = Map.mod2;
        UndisplayedKeys[NumUndispKeys].key1 = Map.key1;
        UndisplayedKeys[NumUndispKeys].mod1 = Map.mod1;
        UndisplayedKeys[NumUndispKeys].editable = Map.editable;
        strcpy(UndisplayedKeys[NumUndispKeys].descrip, Map.descrip);
        NumUndispKeys++;
        return TRUE;
    }

    return FALSE;
}

int AddKeyMapLines(C_Window *win, C_Line *Hline, C_Line *Vline, int count)
{
    int retval = TRUE;

    if ( not win)
        return FALSE;

    C_Line *line;
    line = (C_Line *)win->FindControl(HLINE + count);

    if ( not line)
    {
        line = new C_Line;

        if (line)
        {
            line->Setup(HLINE + count, Hline->GetType());
            line->SetColor(RGB(191, 191, 191));
            line->SetXYWH(Hline->GetX(), Hline->GetY() + Vline->GetH()*count, Hline->GetW(), Hline->GetH());
            line->SetFlags(Hline->GetFlags());
            line->SetClient(Hline->GetClient());
            line->SetGroup(Hline->GetGroup());
            line->SetCluster(Hline->GetCluster());

            win->AddControl(line);
            line->Refresh();
        }
        else
            retval = FALSE;
    }

    line = (C_Line *)win->FindControl(VLINE + count);

    if ( not line)
    {
        line = new C_Line;

        if (line)
        {
            line->Setup(VLINE + count, Vline->GetType());
            line->SetColor(RGB(191, 191, 191));
            line->SetXYWH(Vline->GetX(), Vline->GetY() + Vline->GetH()*count, Vline->GetW(), Vline->GetH());
            line->SetFlags(Vline->GetFlags());
            line->SetClient(Vline->GetClient());
            line->SetGroup(Vline->GetGroup());
            line->SetCluster(Vline->GetCluster());

            win->AddControl(line);
            line->Refresh();
        }
        else
            retval = FALSE;
    }


    return retval;
}

void UpdateKeyMapButton(C_Button *button, KeyMap &Map, int count)
{
    int flags = Map.mod2 + (Map.key1 << SECOND_KEY_SHIFT) + (Map.mod1 << SECOND_KEY_MOD_SHIFT);

    button->SetUserNumber(KEY2, Map.key2);
    button->SetUserNumber(FLAGS, flags);
    button->SetUserNumber(BUTTON_ID, Map.buttonId);
    button->SetUserNumber(MOUSE_SIDE, Map.mouseSide);
    button->SetUserNumber(EDITABLE, Map.editable);
    button->SetUserPtr(FUNCTION_PTR, (void*)Map.func);

    if (Map.key2 == -1)
    {
        button->SetText(0, TXT_NO_KEY);
    }
    else
    {

        _TCHAR totalDescrip[MAX_PATH] = {0};

        if (Map.key1 > 0)
        {
            _TCHAR firstMod[MAX_PATH] = {0};
            _TCHAR secondMod[MAX_PATH] = {0};
            DoShiftStates(firstMod, Map.mod1);
            DoShiftStates(secondMod, Map.mod2);
            _stprintf(totalDescrip, "%s%s : %s%s", firstMod, KeyDescrips[Map.key1], secondMod, KeyDescrips[Map.key2]);
        }
        else
        {
            DoShiftStates(totalDescrip, Map.mod2);
            strcat(totalDescrip, KeyDescrips[Map.key2]);
        }

        UserFunctionTable.SetControl(Map.key2, flags, KEYCODES + count); //define this as KEYCODES
        button->SetText(0, totalDescrip);
    }

    SetButtonColor(button);

}

int UpdateKeyMap(C_Window *win, C_Button *Keycodes, int height, KeyMap &Map, HotSpotStruct HotSpot, int count)
{
    C_Button *button;
    long flags;

    flags = Map.mod2 + (Map.key1 << SECOND_KEY_SHIFT) + (Map.mod1 << SECOND_KEY_MOD_SHIFT);

    button = (C_Button *)win->FindControl(KEYCODES + count);

    if (button)
    {
        button->Refresh();
        UpdateKeyMapButton(button, Map, count);
        button->Refresh();
        return TRUE;
    }
    else
    {
        button = new C_Button;

        if (button)
        {
            button->Setup(KEYCODES + count, Keycodes->GetType(), Keycodes->GetX(),
                          Keycodes->GetY() + height * count);

            button->SetClient(Keycodes->GetClient());
            button->SetGroup(Keycodes->GetGroup());
            button->SetCluster(Keycodes->GetCluster());
            button->SetFont(Keycodes->GetFont());
            button->SetFlags(Keycodes->GetFlags());
            button->SetCallback(KeycodeCB);
            button->SetHotSpot(HotSpot.X, HotSpot.Y, HotSpot.W, HotSpot.H);

            if (Keycodes->GetSound(1))
                button->SetSound((Keycodes->GetSound(1))->ID, 1);

            UpdateKeyMapButton(button, Map, count);

            win->AddControl(button);
            button->Refresh();
            return TRUE;
        }
    }

    return FALSE;
}

int UpdateMappingDescrip(C_Window *win, C_Text *Mapping, int height, _TCHAR *descrip, int count)
{
    C_Text *text;

    text = (C_Text *)win->FindControl(MAPPING + count);

    if (text)
    {
        text->Refresh();
        text->SetText(descrip);
        text->Refresh();
        return TRUE;
    }
    else
    {
        text = new C_Text;

        if (text)
        {
            text->Setup(MAPPING + count, Mapping->GetType());
            text->SetFGColor(Mapping->GetFGColor());
            text->SetBGColor(Mapping->GetBGColor());
            text->SetClient(Mapping->GetClient());
            text->SetGroup(Mapping->GetGroup());
            text->SetCluster(Mapping->GetCluster());
            text->SetFont(Mapping->GetFont());
            text->SetFlags(Mapping->GetFlags());
            text->SetXY(Mapping->GetX(), Mapping->GetY() + height * count);
            text->SetText(descrip);

            win->AddControl(text);
            text->Refresh();
            return TRUE;
        }
    }

    return FALSE;
}

int SetHdrStatusLine(C_Window *win, C_Button *Keycodes, C_Line *Vline, KeyMap &Map, HotSpotStruct HotSpot, int count)
{
    C_Line *line;

    line = (C_Line *)win->FindControl(KEYCODES - count);

    if (line)
    {
        if (Map.editable not_eq -1)
        {
            line->SetFlagBitOn(C_BIT_INVISIBLE);
        }
        else
        {
            line->SetFlagBitOff(C_BIT_INVISIBLE);
        }

        line->Refresh();
        return TRUE;
    }
    else
    {
        line = new C_Line;

        if (line)
        {
            UI95_RECT client;
            client = win->GetClientArea(3);

            line->Setup(KEYCODES - count, 0);
            //line->SetXYWH( Keycodes->GetX() + HotX,
            // Keycodes->GetY() + Vline->GetH()*count + HotY,
            // HotW,HotH);
            line->SetXYWH(Keycodes->GetX() + HotSpot.X,
                          Keycodes->GetY() + Vline->GetH()*count + HotSpot.Y,
                          client.right - client.left, HotSpot.H);

            line->SetColor(RGB(65, 128, 173)); //lt blue
            line->SetFlags(Vline->GetFlags());
            line->SetClient(Vline->GetClient());
            line->SetGroup(Vline->GetGroup());
            line->SetCluster(Vline->GetCluster());
            win->AddControl(line);
            line->Refresh();

            //this is a header, so we put a lt blue line behind it
            if (Map.editable not_eq -1)
            {
                line->SetFlagBitOn(C_BIT_INVISIBLE);
            }

            line->Refresh();
            return TRUE;
        }
    }

    return FALSE;
}


BOOL SaveKeyMapList(char *filename)
{
    if ( not KeyVar.Modified)
        return TRUE;

    KeyVar.Modified = FALSE;

    FILE *fp;
    C_Window *win;
    C_Button *button;
    InputFunctionType theFunc;
    char *funcDescrip;
    int i, key1, mod1, mod2, flags, count = 0;

    C_Text *text;
    char descrip[_MAX_PATH];

    win = gMainHandler->FindWindow(SETUP_WIN);

    if ( not win)
        return FALSE;

    char path[_MAX_PATH];
    sprintf(path, "%s\\config\\%s.key", FalconDataDirectory, filename);

    fp = fopen(path, "wt");

    if ( not fp)
        return FALSE;

    button = (C_Button *)win->FindControl(KEYCODES);

    while (button)
    {
        //int pmouse,pbutton;

        flags = button->GetUserNumber(FLAGS);
        mod2 = flags bitand MOD2_MASK;
        key1 = (flags bitand KEY1_MASK) >> SECOND_KEY_SHIFT;
        mod1 = (flags bitand MOD1_MASK) >> SECOND_KEY_MOD_SHIFT;

        theFunc = (InputFunctionType)button->GetUserPtr(FUNCTION_PTR);
        funcDescrip = FindStringFromFunction(theFunc);

        text = (C_Text *)win->FindControl(MAPPING + count);

        if (text)
            sprintf(descrip, "%c%s%c", '"', text->GetText(), '"');
        else
            strcpy(descrip, "");

        if (key1 == 0xff)
        {
            key1 = -1;
            mod1 = 0;
        }

        fprintf(fp, "%s %d %d %#X %X %#X %X %d %s\n", funcDescrip, button->GetUserNumber(BUTTON_ID), button->GetUserNumber(MOUSE_SIDE), button->GetUserNumber(KEY2), mod2, key1, mod1, button->GetUserNumber(EDITABLE), descrip);

        count++;
        button = (C_Button *)win->FindControl(KEYCODES + count);
    }

    for (i = 0; i < NumUndispKeys; i++)
    {
        funcDescrip = FindStringFromFunction(UndisplayedKeys[i].func);

        fprintf(fp, "%s %d %d %#X %d %#X %d %d %s\n",
                funcDescrip,
                UndisplayedKeys[i].buttonId ,
                UndisplayedKeys[i].mouseSide,
                UndisplayedKeys[i].key2,
                UndisplayedKeys[i].mod2,
                UndisplayedKeys[i].key1,
                UndisplayedKeys[i].mod1,
                UndisplayedKeys[i].editable,
                UndisplayedKeys[i].descrip
               );
    }

    for (i = 0; i < UserFunctionTable.NumButtons; i++)
    {
        int cpButtonID;
        theFunc = UserFunctionTable.GetButtonFunction(i, &cpButtonID);

        if (theFunc)
        {
            funcDescrip = FindStringFromFunction(theFunc);

            fprintf(fp, "%s %d %d -2 0 0x0 0\n", funcDescrip, i, cpButtonID);
        }
    }

    for (i = 0; i < UserFunctionTable.NumPOVs; i++)
    {
        int cpButtonID;

        for (int j = 0; j < 8; j++)
        {
            theFunc = UserFunctionTable.GetPOVFunction(i, j, &cpButtonID);

            if (theFunc)
            {
                funcDescrip = FindStringFromFunction(theFunc);

                fprintf(fp, "%s %d %d -3 %d 0x0 0\n", funcDescrip, i, cpButtonID, j);
            }
        }
    }

    fclose(fp);

    return TRUE;
}

void SaveKeyCB(long, short hittype, C_Base *control)
{
    C_Window *win;
    C_EditBox * ebox;
    _TCHAR fname[MAX_PATH];

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    win = gMainHandler->FindWindow(SAVE_WIN);

    if ( not win)
        return;

    gMainHandler->HideWindow(win);
    gMainHandler->HideWindow(control->Parent_);

    ebox = (C_EditBox*)win->FindControl(FILE_NAME);

    if (ebox)
    {
        _tcscpy(fname, ebox->GetText());

        if (fname[0] == 0)
            return;

        KeyVar.Modified = TRUE;

        if (SaveKeyMapList(fname))
            _tcscpy(PlayerOptions.keyfile, ebox->GetText());

    }
}

void VerifySaveKeyCB(long ID, short hittype, C_Base *control)
{
    C_EditBox * ebox;
    _TCHAR fname[MAX_PATH];
    FILE *fp;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    ebox = (C_EditBox*)control->Parent_->FindControl(FILE_NAME);

    if (ebox)
    {
        //dpc EmptyFilenameSaveFix, modified by MN - added a warning to enter a filename
        if (g_bEmptyFilenameFix)
        {
            if (_tcslen(ebox->GetText()) == 0)
            {
                AreYouSure(TXT_WARNING, TXT_ENTER_FILENAME, CloseWindowCB, CloseWindowCB);
                return;
            }
        }

        //end EmptyFilenameSaveFix
        _stprintf(fname, "config\\%s.key", ebox->GetText());
        fp = fopen(fname, "r");

        if (fp)
        {
            fclose(fp);
            AreYouSure(TXT_WARNING, TXT_FILE_EXISTS, SaveKeyCB, CloseWindowCB);
        }
        else
            SaveKeyCB(ID, hittype, control);
    }
}


void SaveKeyButtonCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    SetDeleteCallback(DelKeyFileCB);
    SaveAFile(TXT_SAVE_KEYBOARD, "config\\*.KEY", NULL, VerifySaveKeyCB, CloseWindowCB, "");
}


int RemoveExcessControls(C_Window *win, int count)
{
    C_Button *button;
    C_Text *text;
    C_Line *line;
    int retval = FALSE;

    button = (C_Button *)win->FindControl(KEYCODES + count);

    if (button)
    {
        win->RemoveControl(KEYCODES + count);
        retval = TRUE;
    }

    text = (C_Text *)win->FindControl(MAPPING + count);

    if (text)
    {
        win->RemoveControl(MAPPING + count);
        retval = TRUE;
    }

    line = (C_Line *)win->FindControl(KEYCODES - count);

    if (line)
    {
        win->RemoveControl(KEYCODES - count);
        retval = TRUE;
    }

    line = (C_Line *)win->FindControl(HLINE + count);

    if (line)
    {
        win->RemoveControl(HLINE + count);
        retval = TRUE;
    }

    line = (C_Line *)win->FindControl(VLINE + count);

    if (line)
    {
        win->RemoveControl(VLINE + count);
        retval = TRUE;
    }

    //need to remove them if they exist
    return retval;
}


int UpdateKeyMapList(char *fname, int flag)
{
    FILE *fp;

    char filename[_MAX_PATH];
    C_Window *win;


    win = gMainHandler->FindWindow(SETUP_WIN);

    if ( not win)
    {
        KeyVar.NeedUpdate = TRUE;
        return FALSE;
    }

    if (flag)
        sprintf(filename, "%s\\config\\%s.key", FalconDataDirectory, fname);
    else
        sprintf(filename, "%s\\config\\keystrokes.key", FalconDataDirectory);

    fp = fopen(filename, "rt");

    if ( not fp)
        return FALSE;

    UserFunctionTable.ClearTable();
    LoadFunctionTables(fname);

    char keydescrip[_MAX_PATH];
    C_Button *button;
    C_Text *text;

    keydescrip[0] = 0;

    int count = 0;
    int key1, mod1;
    int key2, mod2, editable;
    // int flags =0,
    int buttonId, mouseSide;
    InputFunctionType theFunc;
    char buff[_MAX_PATH];
    char funcName[_MAX_PATH];
    char descrip[_MAX_PATH];
    char *parsed;
    HotSpotStruct HotSpot;

    C_Line *Vline;
    C_Line *Hline;
    C_Button *Keycodes;
    C_Text *Mapping;

    Keycodes = button = (C_Button *)win->FindControl(KEYCODES); //define this as KEYCODES
    Mapping = text = (C_Text *)win->FindControl(MAPPING);     //define this as MAPPING
    Hline = (C_Line *)win->FindControl(HLINE);     //define this as HLINE
    Vline = (C_Line *)win->FindControl(VLINE);     //define this as VLINE

    HotSpot.X = -3;
    HotSpot.Y = -1;

    if (Vline)
    {
        HotSpot.W = Vline->GetX() - 1;
        HotSpot.H = Vline->GetH() - 1;
    }
    else
    {
        HotSpot.W = 30;
        HotSpot.H = 12;
    }

    KeyVar.Modified = FALSE;

    NumUndispKeys = 0;

    SetCursor(gCursors[CRSR_WAIT]);

    while (fgets(buff, _MAX_PATH, fp))
    {

        if (buff[0] == ';' or buff[0] == '\n' or buff[0] == '#')
            continue;

        if (sscanf(buff, "%s %d %d %x %x %x %x %d %[^\n]s", funcName, &buttonId, &mouseSide, &key2, &mod2, &key1, &mod1, &editable, &descrip) < 8)
            continue;

        if (key2 < -1)
            continue;

        theFunc = FindFunctionFromString(funcName);

        keydescrip[0] = 0;

        if ( not theFunc)
            continue;

        KeyMap Map;
        Map.func = theFunc;
        Map.buttonId = buttonId;
        Map.mouseSide = mouseSide;
        Map.editable = editable;
        strcpy(Map.descrip, descrip);
        Map.key1 = key1;
        Map.mod1 = mod1;
        Map.key2 = key2;
        Map.mod2 = mod2;

        if (editable == -2)
        {
            AddUndisplayedKey(Map);

            ShiAssert(NumUndispKeys < 300);

            continue;
        }

        parsed = descrip + 1;
        parsed[strlen(descrip) - 2] = 0;
        parsed[37] = 0;

        if ( not count)
        {
            //first time through .. special case
            UpdateKeyMapButton(button, Map, count);

            Mapping->Refresh();
            Mapping->SetText(parsed);
            Mapping->Refresh();
        }
        else
        {
            SetHdrStatusLine(win, Keycodes, Vline, Map, HotSpot, count);
            //the rest of the times through
            UpdateKeyMap(win, Keycodes, Vline->GetH(), Map, HotSpot, count);

            UpdateMappingDescrip(win, Mapping, Vline->GetH(), parsed, count); //this will add the control if it doesn't exist

            AddKeyMapLines(win, Hline, Vline, count);
        }

        count++;

        if (count > 10000)
            break;
    }

    NumDispKeys = count;

    fclose(fp);

    while (RemoveExcessControls(win, count++));

    gMainHandler->WindowToFront(win);

    SetCursor(gCursors[CRSR_F16]);
    return TRUE;
}

void LoadKeyCB(long, short hittype, C_Base *control)
{
    C_EditBox * ebox;
    _TCHAR fname[MAX_PATH];

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gMainHandler->HideWindow(control->Parent_);

    ebox = (C_EditBox*)control->Parent_->FindControl(FILE_NAME);

    if (ebox)
    {
        _tcscpy(fname, ebox->GetText());

        for (unsigned long i = 0; i < _tcslen(fname); i++)
            if (fname[i] == '.')
                fname[i] = 0;

        if (fname[0] == 0)
            return;

        KeyVar.Modified = TRUE;

        if (UpdateKeyMapList(fname, USE_FILENAME))
            _tcscpy(PlayerOptions.keyfile, ebox->GetText());

    }
}


void LoadKeyButtonCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    SetDeleteCallback(DelKeyFileCB);
    LoadAFile(TXT_LOAD_KEYBOARD, "config\\*.KEY", NULL, LoadKeyCB, CloseWindowCB);
}

int CreateKeyMapList(char *filename)
{
    FILE *fp;
    char path[_MAX_PATH];

    C_Window *win;

    win = gMainHandler->FindWindow(SETUP_WIN);

    if ( not win)
        return FALSE;

    sprintf(path, "%s\\config\\%s.key", FalconDataDirectory, filename);

    fp = fopen(path, "rt");

    if ( not fp)
    {
        sprintf(path, "%s\\config\\keystrokes.key", FalconDataDirectory);
        fp = fopen(path, "rt");

        if ( not fp)
            return FALSE;
    }

    int count = 0;
    int key1, mod1;
    int key2, mod2;
    // int flags =0,
    int buttonId, mouseSide, editable;
    InputFunctionType theFunc;
    char buff[_MAX_PATH];
    char funcName[_MAX_PATH];
    char keydescrip[_MAX_PATH];
    char descrip[_MAX_PATH];
    char *parsed;
    HotSpotStruct HotSpot;
    //int  HotX,HotY,HotW,HotH;
    UI95_RECT client;

    C_Button *Keycodes;
    C_Text *Mapping;
    C_Line *Vline;
    C_Line *Hline;

    keydescrip[0] = 0;

    client = win->GetClientArea(3);
    Keycodes = (C_Button *)win->FindControl(KEYCODES); //define this as KEYCODES
    Mapping = (C_Text *)win->FindControl(MAPPING);     //define this as MAPPING
    Hline = (C_Line *)win->FindControl(HLINE);     //define this as HLINE
    Vline = (C_Line *)win->FindControl(VLINE);     //define this as VLINE

    HotSpot.X = -3;
    HotSpot.Y = -1;

    if (Vline)
    {
        HotSpot.W = Vline->GetX() - 1;
        HotSpot.H = Vline->GetH() - 1;
    }
    else
    {
        HotSpot.W = 30;
        HotSpot.H = 12;
    }

    NumDispKeys = 0;
    NumUndispKeys = 0;

    while (fgets(buff, _MAX_PATH, fp))
    {
        if (buff[0] == ';' or buff[0] == '\n' or buff[0] == '#')
            continue;

        if (sscanf(buff, "%s %d %d %x %x %x %x %d %[^\n]s", funcName, &buttonId, &mouseSide, &key2, &mod2, &key1, &mod1, &editable, &descrip) < 8)
            continue;

        if (key2 == -2)
            continue;

        theFunc = FindFunctionFromString(funcName);

        keydescrip[0] = 0;

        if ( not theFunc)
            continue;

        KeyMap Map;
        Map.func = theFunc;
        Map.buttonId = buttonId;
        Map.mouseSide = mouseSide;
        Map.editable = editable;
        strcpy(Map.descrip, descrip);
        Map.key1 = key1;
        Map.mod1 = mod1;
        Map.key2 = key2;
        Map.mod2 = mod2;

        if (editable == -2)
        {
            AddUndisplayedKey(Map);

            ShiAssert(NumUndispKeys < 300);

            continue;
        }

        parsed = descrip + 1;
        parsed[strlen(descrip) - 2] = 0;
        parsed[37] = 0;

        if ( not count)
        {
            NumDispKeys++;
            //first time through .. special case
            UpdateKeyMapButton(Keycodes, Map, count);

            Keycodes->SetCallback(KeycodeCB);
            Keycodes->SetHotSpot(HotSpot.X, HotSpot.Y, HotSpot.W, HotSpot.H);
            Keycodes->Refresh();

            Mapping->Refresh();
            Mapping->SetText(parsed);
            Mapping->Refresh();
        }
        else
        {
            NumDispKeys++;
            SetHdrStatusLine(win, Keycodes, Vline, Map, HotSpot, count);
            //the rest of the times through
            UpdateKeyMap(win, Keycodes, Vline->GetH(), Map, HotSpot, count);

            UpdateMappingDescrip(win, Mapping, Vline->GetH(), parsed, count); //this will add the control if it doesn't exist

            AddKeyMapLines(win, Hline, Vline, count);
        }

        count++;

        if (count > 10000)
            break;
    }

    fclose(fp);
    return FALSE;

}


void SetKeyDefaultCB(long, short, C_Base *)
{
    if (UpdateKeyMapList(0, USE_DEFAULT))
        KeyVar.Modified = TRUE;
}

/************************************************************************/
// Hides/Shows the POV HAT for controllers that feature them
/************************************************************************/
void SetJoystickAndPOVSymbols(const bool isActive, C_Base *control)
{
    const int POVSymbols[] = { LEFT_HAT, RIGHT_HAT, CENTER_HAT, UP_HAT, DOWN_HAT };
    const int POVSymbolCount = sizeof(POVSymbols) / sizeof(int);
    C_Button *button = NULL;

    for (int i = 0; i < POVSymbolCount; i++)
    {
        button = (C_Button *)control->Parent_->FindControl(POVSymbols[i]);

        if (button not_eq NULL)
        {
            if ( not isActive)
                button->SetFlagBitOn(C_BIT_INVISIBLE);
            else
                button->SetFlagBitOff(C_BIT_INVISIBLE);

            button->Refresh();
        }
        else
            ShiAssert(false);
    }
}

/************************************************************************/
// Retro:
// Hides/Shows the Throttle and Rudder Bars, depending on if a device
// that features them is defined
/************************************************************************/
void SetThrottleAndRudderBars(C_Base *control)
{
    C_Line *line = NULL;

    // Retro 17Jan2004 - have to cater for 2 throttles now 
    /* do throttle mumbo-jumbo */
    line = (C_Line *)control->Parent_->FindControl(THROTTLE);

    if (line not_eq NULL)
    {
        // line->Refresh();
        if ( not IO.AnalogIsUsed(AXIS_THROTTLE))
        {
            line->SetColor(RGB(130, 130, 130)); //grey
            line->SetH(Throttle.bottom - Throttle.top);
            line->SetY(Throttle.top);
            line->Refresh();

            line = (C_Line *)control->Parent_->FindControl(THROTTLE2);

            if (line not_eq NULL)
            {
                line->SetColor(RGB(130, 130, 130)); //grey
                line->SetH(Throttle.bottom - Throttle.top);
                line->SetY(Throttle.top);
                line->Refresh();
            }
            else
            {
                ShiAssert(false);
            }
        }
        else
        {
            line->SetColor(RGB(60, 123, 168)); //blue
            line->Refresh();
            line = (C_Line *)control->Parent_->FindControl(THROTTLE2);

            if (line not_eq NULL)
            {
                line->SetColor(RGB(60, 123, 168)); //blue
                line->Refresh();
            }
            else
            {
                ShiAssert(false);
            }
        }

    }
    else
    {
        ShiAssert(false);
    }

    /* same shit for rudder */
    line = (C_Line *)control->Parent_->FindControl(RUDDER);

    if (line not_eq NULL)
    {
        line->Refresh();

        if ( not IO.AnalogIsUsed(AXIS_YAW))
        {
            line->SetColor(RGB(130, 130, 130)); //grey
            line->SetH(Rudder.bottom - Rudder.top);
            line->SetY(Rudder.top);
        }
        else
            line->SetColor(RGB(60, 123, 168)); //blue

        line->Refresh();
    }
}

/************************************************************************/
// Called when the user manipulates the 'controller' listbox in the
// setup->controller tab
/************************************************************************/
void ControllerSelectCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_SELECT)
        return;

    /************************************************************************/
    // Retro 31Dec2003
    // Totally rewrote that callback..
    /************************************************************************/
    C_ListBox *lbox;
    lbox = (C_ListBox *)control;
    int newcontroller;

    newcontroller = lbox->GetTextID() - 1;

    if (newcontroller == AxisMap.FlightControlDevice) // nothing changed, so no action required
    {
        return;
    }

    IO.Reset(); // set all axis (real and game) back to nada (off)

    if (AxisMap.FlightControlDevice == SIM_KEYBOARD) // hrmmm... no sure what´s up here
    {
        SaveKeyMapList("laptop");
        UpdateKeyMapList(PlayerOptions.GetKeyfile(), TRUE);
    }

    if ((newcontroller == SIM_MOUSE) or (newcontroller == SIM_NUMDEVICES))
    {
        // ??? not allowed 
        ShiAssert(false);
    }
    else if (newcontroller == SIM_KEYBOARD) // hrmmm... no sure what´s up here
    {
        SaveKeyMapList(PlayerOptions.GetKeyfile());
        UpdateKeyMapList("laptop", TRUE);

        SetJoystickAndPOVSymbols(false, control); // kill POV symbol (the keyboard has none)
    }
    else // at last, a reasonable user
    {
        C_Line *line = NULL;
        C_Button *button = NULL;
        int hasPOV = FALSE;


        // Retro 27Jan2004 - disable custon axis shaping first
        PlayerOptions.SetAxisShaping(false);

        /* check if current device has a POV hat and enable/disable the symbols accordingly */

        // Retro 26Dec2003
        DIDEVCAPS CurJoyCaps;
        CurJoyCaps.dwSize = sizeof(DIDEVCAPS);
        gpDIDevice[newcontroller]->GetCapabilities(&CurJoyCaps);

        //NumberOfPOVs = (CurJoyCaps.dwPOVs>0)?1:0; // Retro 26Dec2003
        NumberOfPOVs = CurJoyCaps.dwPOVs; // Wombat778 4-27-04 Dont limit to 1 POV

        if (NumberOfPOVs > 0) // Retro 26Dec2003
            hasPOV = TRUE; // Retro 26Dec2003

        int POVSymbols[] = { LEFT_HAT, RIGHT_HAT, CENTER_HAT, UP_HAT, DOWN_HAT };
        const int POVSymbolCount = sizeof(POVSymbols) / sizeof(int);

        for (int i = 0; i < POVSymbolCount; i++)
        {
            button = (C_Button *)control->Parent_->FindControl(POVSymbols[i]);

            if (button not_eq NULL)
            {
                if ( not hasPOV)
                    button->SetFlagBitOn(C_BIT_INVISIBLE);
                else
                    button->SetFlagBitOff(C_BIT_INVISIBLE);

                button->Refresh();
            }
            else
                ShiAssert(false);
        }

        // had the FFB check here, moved down a bit...

        /* now check if device has x/y axis (pro forma I hope) */
        DIDEVICEOBJECTINSTANCE devobj;
        HRESULT hres;
        devobj.dwSize = sizeof(DIDEVICEOBJECTINSTANCE);

        hres = gpDIDevice[newcontroller]->GetObjectInfo(&devobj, DIJOFS_X, DIPH_BYOFFSET);

        if (hres == DI_OK)
        {
            hres = gpDIDevice[newcontroller]->GetObjectInfo(&devobj, DIJOFS_Y, DIPH_BYOFFSET);

            if (hres == DI_OK)
            {
                // I only allow complete 'sets' for the flight control device..
                AxisMap.FlightControlDevice = newcontroller;
                AxisMap.Bank.Axis = DX_XAXIS;
                AxisMap.Bank.Device = newcontroller; // also not really necessary but nice for sanity..
                AxisMap.Pitch.Axis = DX_YAXIS;
                AxisMap.Pitch.Device = newcontroller; // also not really necessary but nice for sanity..
            }
        }

        /* now check if device has a throttle */
        bool weHaveACougerUser = false; // ahh... that dumb metal POS has the throttle on RZ since it has no rudder.. or whatever..

        if (gDIDevNames[newcontroller])
        {
            if ( not strcmp(gDIDevNames[newcontroller], "HOTAS Cougar Joystick"))
            {
                weHaveACougerUser = true;
            }
        }

        if (weHaveACougerUser == false)
        {
            hres = gpDIDevice[newcontroller]->GetObjectInfo(&devobj, DIJOFS_SLIDER(0), DIPH_BYOFFSET);

            if (hres == DI_OK)
            {
                AxisMap.Throttle.Device = newcontroller;
                AxisMap.Throttle.Axis = DX_SLIDER0;
            }
        }
        else
        {
            hres = gpDIDevice[newcontroller]->GetObjectInfo(&devobj, DIJOFS_Z, DIPH_BYOFFSET);

            if (hres == DI_OK)
            {
                weHaveACougerUser = true;
                AxisMap.Throttle.Device = newcontroller;
                AxisMap.Throttle.Axis = DX_ZAXIS;
            }
        }

        /* now check if device has a rudder */
        if (weHaveACougerUser == false) // cougar has no rudder and the RZ is taken anyways..
        {
            hres = gpDIDevice[newcontroller]->GetObjectInfo(&devobj, DIJOFS_RZ, DIPH_BYOFFSET);

            if (hres == DI_OK)
            {
                AxisMap.Yaw.Device = newcontroller;
                AxisMap.Yaw.Axis = DX_RZAXIS;
            }
        }

        /* now check if this thing has FFB */
        // return value intentionally disregarded
        // (does only indicate the result anyway, FFB is activated/deactivated inside the function
        CheckForForceFeedback(newcontroller - SIM_JOYSTICK1);

        SetupGameAxis();
    }

    // this function draws/hides the rudder/throttle bars, depending on if they´re mapped..
    SetThrottleAndRudderBars(control);
    IO.WriteAxisMappingFile();
    IO.SaveFile(); // for centering and ABDetent info

    InitializeValueBars = 1; // Retro 26Dec2003
}

/************************************************************************/
// fills the controller listbox in the setup->controller tab
// there´s a strange 'thrustmaster-only' hack there, dunno why..
/************************************************************************/
void BuildControllerList(C_ListBox *lbox)
{
    lbox->RemoveAllItems();

    //lbox->AddItem(SIM_MOUSE + 1,C_TYPE_ITEM,TXT_MOUSE);
    lbox->AddItem(SIM_KEYBOARD + 1, C_TYPE_ITEM, TXT_KEYBOARD);
    //lbox->AddItem(SIM_NUMDEVICES + 1,C_TYPE_ITEM,TXT_INTELLIPOINT);

    if (gTotalJoy)
    {
        for (int i = 0; i < gTotalJoy; i++)
        {
            if ( not stricmp(gDIDevNames[SIM_JOYSTICK1 + i], "tm"))
            {
                delete [] gDIDevNames[SIM_JOYSTICK1 + i];
                gDIDevNames[SIM_JOYSTICK1 + i] = new TCHAR[MAX_PATH];
                _tcscpy(gDIDevNames[SIM_JOYSTICK1 + i], "Thrustmaster");
            }

            lbox->AddItem(i + SIM_JOYSTICK1 + 1, C_TYPE_ITEM, gDIDevNames[SIM_JOYSTICK1 + i]);

            if (mHelmetIsUR and i == mHelmetID)
                lbox->SetItemFlags(i + SIM_JOYSTICK1 + 1, 0);
        }
    }

    lbox->Refresh();
}
