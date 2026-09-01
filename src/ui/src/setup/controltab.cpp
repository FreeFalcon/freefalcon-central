#include "falclib.h"
#include "chandler.h"
#include "userids.h"
#include "playerop.h"
#include <mmsystem.h>
#include "sim/include/stdhdr.h"
#include "sim/include/simio.h"
#include "ui_setup.h"
#include <tchar.h>
#include "sim/include/inpfunc.h"
#include "sim/include/commands.h"
#include "sim/include/controlsxml.h" // #53: BMS function labels from controls.xml
#include "logbook.h" // #53: UI_logbk.Callsign() for "SETTINGS FOR:"
#include "f4find.h"
#include "sim/include/sinput.h"

//temporary until logbook is working
#include "uicomms.h"

bool JoyEffectPlaying = false;

#include "sim/include/ffeedbk.h" // Retro 25Dec2003 for instant FFB feedback

#pragma warning(disable : 4706) // assignment within conditional expression

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
    DEVICE_IDX, // #53 device cell: SIM device index this cell belongs to
};

KeyVars KeyVar = {FALSE, 0, 0, 0, 0, FALSE, FALSE};
KeyMap UndisplayedKeys[300] = {NULL, 0, 0, 0, 0, 0, 0, 0};
int NumUndispKeys = 0;
int NumDispKeys = 0;
extern int NoRudder;
extern long
    mHelmetIsUR; // hack for Italian 1.06 version - Only gets set to true IF UR Helmet detected
extern long mHelmetID;

extern int setABdetent;
extern int setIdleCutoff; // Retro 1Feb2004

extern unsigned int NumberOfPOVs; // Retro 26Dec2003
int InitializeValueBars = 1; // Retro 26Dec2003, pulled up
//CalibrateStruct Calibration = {FALSE,1,0,TRUE};
extern int hasForceFeedback; // Retro 27Dec2003

void SetThrottleAndRudderBars(C_Base *control); // Retro 17Jan2004

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
#define RESCALE(in, inmin, inmax, outmin, outmax)                              \
    ((float)(in) - (inmin)) * ((outmax) - (outmin)) / ((inmax) - (inmin)) +    \
        (outmin)
/************************************************************************/
// Retro Macro/Define definitions end
/************************************************************************/

//defined in this file
int CreateKeyMapList(C_Window *win);
//SIM_INT Calibrate ( void );


//defined in another file
void InitKeyDescrips(void);
void CleanupKeys(void);
void SetDeleteCallback(void (*cb)(long, short, C_Base *));
void SaveAFile(long TitleID, _TCHAR *filespec, _TCHAR *excludelist[],
               void (*YesCB)(long, short, C_Base *),
               void (*NoCB)(long, short, C_Base *), _TCHAR *filename);
void LoadAFile(long TitleID, _TCHAR *filespec, _TCHAR *excludelist[],
               void (*YesCB)(long, short, C_Base *),
               void (*NoCB)(long, short, C_Base *));
void CloseWindowCB(long ID, short hittype, C_Base *control);
void AreYouSure(long TitleID, long MessageID,
                void (*OkCB)(long, short, C_Base *),
                void (*CancelCB)(long, short, C_Base *));
void AreYouSure(long TitleID, _TCHAR *text, void (*OkCB)(long, short, C_Base *),
                void (*CancelCB)(long, short, C_Base *));
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
    if (not win)
        return;

    C_Line *line;
    C_Button *button;
    int count = 1;

    line = (C_Line *)win->FindControl(KEYCODES - count);

    while (line)
    {
        button = (C_Button *)win->FindControl(KEYCODES + count);

        if (not button or button->GetUserNumber(EDITABLE) not_eq -1)
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
extern AxisIDStuff
    DIAxisNames[SIM_NUMDEVICES *
                8]; /* '8' is defined by dinput: 8 axis maximum per device */
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
    DeviceAxis *theDeviceAxis;
} UIInputStuff_t;

UIInputStuff_t UIInputStuff[AXIS_MAX] = {
    //   InGameAxis Axis Listbox Value Bar Deadzone Listbox Saturation Listbox Reverse Button DeviceAxis Struct
    // #24: pitch/roll now have their own axis-selection dropdowns (like rudder),
    // instead of being assigned implicitly via the "controller" dropdown (FlightControlDevice).
    {AXIS_PITCH, SETUP_ADVANCED_PITCH_AXIS, SETUP_ADVANCED_PITCH_VAL,
     SETUP_ADVANCED_PITCH_DEADZONE, SETUP_ADVANCED_SAT_PITCH, 0,
     &AxisMap.Pitch},
    {AXIS_ROLL, SETUP_ADVANCED_BANK_AXIS, SETUP_ADVANCED_BANK_VAL,
     SETUP_ADVANCED_BANK_DEADZONE, SETUP_ADVANCED_SAT_BANK, 0, &AxisMap.Bank},
    {AXIS_FOV, SETUP_ADVANCED_FOV, SETUP_ADVANCED_FOV_VAL, 0,
     SETUP_ADVANCED_SAT_FOV, SETUP_ADVANCED_REVERSE_FOV, &AxisMap.FOV},
    {AXIS_YAW, SETUP_ADVANCED_RUDDER_AXIS, SETUP_ADVANCED_RUDDER_VAL,
     SETUP_ADVANCED_RUDDER_AXIS_DEADZONE, SETUP_ADVANCED_SAT_YAW,
     SETUP_ADVANCED_REVERSE_RUDDER, &AxisMap.Yaw},
    {AXIS_THROTTLE, SETUP_ADVANCED_THROTTLE_AXIS, SETUP_ADVANCED_THROTTLE_VAL,
     0, SETUP_ADVANCED_SAT_THROTTLE, 0, &AxisMap.Throttle},
    {AXIS_THROTTLE2, SETUP_ADVANCED_THROTTLE2_AXIS,
     SETUP_ADVANCED_THROTTLE2_VAL, 0, SETUP_ADVANCED_SAT_THROTTLE2, 0,
     &AxisMap.Throttle2},
    // #53 trim roll/pitch/yaw axes removed from the UI (rarely mapped to an axis; trim is on the stick/HOTAS buttons).
    {AXIS_BRAKE_LEFT, SETUP_ADVANCED_BRAKE_LEFT, SETUP_ADVANCED_BRAKE_LEFT_VAL,
     0, SETUP_ADVANCED_SAT_BRAKELEFT, SETUP_ADVANCED_REVERSE_BRAKE_LEFT,
     &AxisMap.BrakeLeft},
    {AXIS_BRAKE_RIGHT, SETUP_ADVANCED_BRAKE_RIGHT,
     SETUP_ADVANCED_BRAKE_RIGHT_VAL, 0, SETUP_ADVANCED_SAT_BRAKERIGHT,
     SETUP_ADVANCED_REVERSE_BRAKE_RIGHT,
     &AxisMap.BrakeRight}, // #57 differential braking
    {AXIS_ANT_ELEV, SETUP_ADVANCED_ANT_ELEV, SETUP_ADVANCED_ANT_ELEV_VAL,
     SETUP_ADVANCED_ANT_ELEV_DEADZONE, SETUP_ADVANCED_SAT_ANT_ELEV,
     SETUP_ADVANCED_REVERSE_ANT_ELEV, &AxisMap.AntElev},
    {AXIS_CURSOR_X, SETUP_ADVANCED_CURSOR_X, SETUP_ADVANCED_CURSOR_X_VAL,
     SETUP_ADVANCED_CURSOR_X_DEADZONE, SETUP_ADVANCED_SAT_CURSOR_X,
     SETUP_ADVANCED_REVERSE_CURSOR_X, &AxisMap.CursorX},
    {AXIS_CURSOR_Y, SETUP_ADVANCED_CURSOR_Y, SETUP_ADVANCED_CURSOR_Y_VAL,
     SETUP_ADVANCED_CURSOR_Y_DEADZONE, SETUP_ADVANCED_SAT_CURSOR_Y,
     SETUP_ADVANCED_REVERSE_CURSOR_Y, &AxisMap.CursorY},
    {AXIS_RANGE_KNOB, SETUP_ADVANCED_RANGE_KNOB, SETUP_ADVANCED_RANGE_KNOB_VAL,
     SETUP_ADVANCED_RANGE_KNOB_DEADZONE, SETUP_ADVANCED_SAT_RNG_KNOB,
     SETUP_ADVANCED_REVERSE_RANGE_KNOB, &AxisMap.RngKnob},
    {AXIS_COMM_VOLUME_1, SETUP_ADVANCED_COMM1_VOL, SETUP_ADVANCED_COMM1_VOL_VAL,
     0, SETUP_ADVANCED_SAT_COMM1_VOL, SETUP_ADVANCED_REVERSE_COMM1_VOL,
     &AxisMap.Comm1Vol},
    {AXIS_COMM_VOLUME_2, SETUP_ADVANCED_COMM2_VOL, SETUP_ADVANCED_COMM2_VOL_VAL,
     0, SETUP_ADVANCED_SAT_COMM2_VOL, SETUP_ADVANCED_REVERSE_COMM2_VOL,
     &AxisMap.Comm2Vol},
    {AXIS_MSL_VOLUME, SETUP_ADVANCED_MSL_VOL, SETUP_ADVANCED_MSL_VOL_VAL, 0,
     SETUP_ADVANCED_SAT_MSL_VOL, SETUP_ADVANCED_REVERSE_MSL_VOL,
     &AxisMap.MSLVol},
    {AXIS_THREAT_VOLUME, SETUP_ADVANCED_THREAT_VOL,
     SETUP_ADVANCED_THREAT_VOL_VAL, 0, SETUP_ADVANCED_SAT_THREAT_VOL,
     SETUP_ADVANCED_REVERSE_THREAT_VOL, &AxisMap.ThreatVol},
    {AXIS_HUD_BRIGHTNESS, SETUP_ADVANCED_HUD_BRIGHT,
     SETUP_ADVANCED_HUD_BRIGHT_VAL, 0, SETUP_ADVANCED_SAT_HUD_BRT,
     SETUP_ADVANCED_REVERSE_HUD_BRIGHT, &AxisMap.HudBrt},
    {AXIS_RET_DEPR, SETUP_ADVANCED_RET_DEPR, SETUP_ADVANCED_RET_DEPR_VAL, 0,
     SETUP_ADVANCED_SAT_RET_DEPR, SETUP_ADVANCED_REVERSE_RET_DEPR,
     &AxisMap.RetDepr},
    {AXIS_ZOOM, SETUP_ADVANCED_ZOOM, SETUP_ADVANCED_ZOOM_VAL, 0,
     SETUP_ADVANCED_SAT_ZOOM, SETUP_ADVANCED_REVERSE_ZOOM, &AxisMap.Zoom},
    {AXIS_INTERCOM_VOLUME, SETUP_ADVANCED_INTERCOM_VOL,
     SETUP_ADVANCED_INTERCOM_VOL_VAL, 0, SETUP_ADVANCED_SAT_INTERCOM_VOL,
     SETUP_ADVANCED_REVERSE_INTERCOM_VOL, &AxisMap.InterComVol},
};

/************************************************************************/
// Yuck.. you know, this should actually go into the listbox class methinks
/************************************************************************/
bool SetListBoxItemData(C_ListBox *theLB, const long ID,
                        const short theItemIndex, const long theItemData)
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
long GetListBoxItemData(C_ListBox *theLB, const int theItemIndex,
                        const long theIndex = 0)
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
void SaveAxisMappings(C_Window *win)
{
    if (win)
    {
        for (int i = 0; i < AXIS_MAX; i++)
        {
            if (UIInputStuff[i].AxisLB == 0)
                continue;

            C_ListBox *listbox =
                (C_ListBox *)win->FindControl(UIInputStuff[i].AxisLB);

            if (listbox)
            {
                long index = GetListBoxItemData(listbox, 0);

                if (index > -2)
                {
                    if (index not_eq -1)
                    {
                        UIInputStuff[i].theDeviceAxis->Device =
                            DIAxisNames[index].DXDeviceID;
                        UIInputStuff[i].theDeviceAxis->Axis =
                            DIAxisNames[index].DXAxisID;
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
// This callback function doesn�t do much, it just hides the other
// tabs
/************************************************************************/
// #51 INPUT field (RefreshJoystickCB): on entering/switching a tab, re-read the baseline of
// pressed buttons so that ALREADY-held ones (stuck 3-position switches etc.) do not produce a false
// "edge" -> not shown / not assigned. true = capture prev on the next poll.
bool g_editPollReseed = true;
// #52 deferred key-list rebuild: ClearKeyCB MUST NOT call UpdateKeyMapList synchronously
// (it would delete the very Clear button that was clicked -> UAF after returning to the dispatcher). We set a flag,
// consumed in RefreshJoystickCB (periodic poll, outside the button callback stack).
bool g_keyListNeedRebuild = false;
// Artscout - 2026: when the deferred rebuild was triggered by a context-menu "Clear" (not a full
// open/search/reset/load), keep the current scroll position instead of snapping back to the top.
bool g_keyListPreserveScroll = false;
// Artscout - 2026: modal button-assign dialog flag (defined further down, used by the controls
// window's OK/Back/Cancel above its definition).
extern bool g_baWindowOpen;
void ControlTab_KeepButtonAssignFront(
    void); // Artscout - 2026: defined in the assign-window block

// #53 forward declarations (defined further down) needed by AdvancedControlCB to wire the
// CONTROLS SETUP tab (key capture + search box) when the window opens.
BOOL KeystrokeCB(unsigned char DKScanCode, unsigned char Ascii,
                 unsigned char ShiftStates, long RepeatCount);
void KeyListSearchCB(long ID, short hittype, C_Base *control);
int UpdateKeyMapList(char *fname, int flag);
void RefreshJoystickCB(long ID, short hittype,
                       C_Base *control); // #53 timer cb (defined below)
void GenericTimerCB(long ID, short hittype,
                    C_Base *control); // #53 (defined in ui_setup.cpp)
extern char
    g_keyFilter[64]; // #22 search substring (defined below, before KeystrokeCB)
extern int
    g_keyDevFilter; // #22 device filter (defined below, before KeystrokeCB)

void SetupControlTabsCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    g_editPollReseed =
        true; // #51 tab changed -> re-baseline buttons for the INPUT field

    int i = 1;

    while (control->GetUserNumber(i))
    {
        control->Parent_->HideCluster(control->GetUserNumber(i));
        i++;
    }

    control->Parent_->UnHideCluster(control->GetUserNumber(0));

    // #53 AXIS SETUP (cluster 10002): recompute client-1 scroll on activation. SetClientArea
    // resets the scroll to the top; ScanClientArea recomputes the virtual height from the rows
    // and shows/hides the vertical scrollbar (cwindow.cpp). Without this the slider stays
    // invisible and -- since C_ScrollBar::Wheel bails when invisible -- the wheel does not scroll
    // anywhere over the page.
    if (control->GetUserNumber(0) == 10002)
    {
        C_Window *w = (C_Window *)control->Parent_;
        UI95_RECT ca = w->GetClientArea(1);
        w->SetClientArea(ca.left, ca.top, ca.right - ca.left,
                         ca.bottom - ca.top, 1);
        w->ScanClientArea(1);
        w->RefreshClient(1);
    }

    control->Parent_->RefreshWindow();

    Cluster = control->GetUserNumber(0);
}

// #53 forward decl: SaveKeyMapList is defined later in this file but used by the Apply path.
BOOL SaveKeyMapList(char *filename);

// #53 forward decl: the profile link reuses the SIMULATION-page logbook callback (ui_setup.cpp).
void SetupOpenLogBookCB(long ID, short hittype, C_Base *control);

// #53 forward decl: joystick test panel button (defined later in this file), wired in AdvancedControlCB.
// (CalibrateCB is commented out in this fork -> CALIBRATE button stays unwired, as in the original.)
void SetABDetentCB(long ID, short hittype, C_Base *control);

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

    if (win == NULL)
        return;

    win = gMainHandler->FindWindow(SETUP_CONTROL_ADVANCED_WIN);

    if (not win)
        return;

    /* array of pointers to all axis listboxes in this sheet */
    C_ListBox *listbox;

    // check contents of axis listboxes and saves..
    SaveAxisMappings(win);

    // check contents of deadzone listboxes..
    listbox = (C_ListBox *)0;

    for (int j = 0; j < AXIS_MAX; j++)
    {
        int index = -1;

        if (UIInputStuff[j].DeadzoneLB == 0)
            continue;

        listbox = (C_ListBox *)win->FindControl(UIInputStuff[j].DeadzoneLB);

        if (listbox)
        {
            index = listbox->GetTextID();

            if (index >= 0)
            {
                switch (
                    index) // not every axis used the deadzone though.. only bipolar ones
                {
                case SETUP_ADVANCED_DZ_SMALL:
                    UIInputStuff[j].theDeviceAxis->Deadzone = g_nDeadzoneSmall;
                    break; // 1%

                default:
                    ShiAssert(
                        false); // fallthrough intentional to give me 5% deadzone in these cases..

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
    listbox = (C_ListBox *)0;

    for (int j = 0; j < AXIS_MAX; j++)
    {
        int index = -1;

        if (UIInputStuff[j].SaturationLB == 0)
            continue;

        listbox = (C_ListBox *)win->FindControl(UIInputStuff[j].SaturationLB);

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
                    UIInputStuff[j].theDeviceAxis->Saturation =
                        g_nSaturationSmall;
                    break;

                case SETUP_ADVANCED_DZ_MEDIUM:
                    UIInputStuff[j].theDeviceAxis->Saturation =
                        g_nSaturationMedium;
                    break;

                case SETUP_ADVANCED_DZ_LARGE:
                    UIInputStuff[j].theDeviceAxis->Saturation =
                        g_nSaturationLarge;
                    break;
                }
            }
        }
        else
            ShiAssert(false);
    }


    C_Button *button = (C_Button *)0;
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

    {
        extern AxisMapping AxisMap;
        ControlsXml_WriteAxes(
            &AxisMap); // #53: axes + soft props into the profile axismapping.xml (#57: replaces joystick.cal)
    }
    SetupGameAxis();

    // #53: persist keyboard + device-button bindings too. In the new flow (entered from the
    // SIMULATION page, left via Back/OK/Apply) the old main-window save path is never reached,
    // so without this device-button assignments would be lost on re-entry.
    SaveKeyMapList(PlayerOptions.GetKeyfile());

    /* PROBLEM: have to call the 'SetThrottleAndRudderBars' functions in the setup->controls tab.. hmm */
    win = gMainHandler->FindWindow(SETUP_WIN);

    if (win == NULL)
        return;

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

    // Artscout - 2026: modal — can't leave the controls window while the assign dialog is up.
    if (g_baWindowOpen)
    {
        ControlTab_KeepButtonAssignFront();
        return;
    }

    /* this takes care of saving */
    AdvancedControlApplyCB(ID, hittype, control);

    /* ..and quit */
    CloseWindowCB(ID, hittype, control);
}

// #53 forward decls for the Back button (defined in ui_setup.cpp)
void SetupRadioCB(long ID, short hittype, C_Base *control);

/************************************************************************/
// #53 "Back" button (SIMULATION image): apply, close this window, and return to the
// SIMULATION page of the main options window.
/************************************************************************/
void AdvancedControlBackCB(long ID, short hittype, C_Base *control)
{
    if ((hittype not_eq C_TYPE_LMOUSEUP))
        return;

    // Artscout - 2026: modal — can't leave the controls window while the assign dialog is up.
    if (g_baWindowOpen)
    {
        ControlTab_KeepButtonAssignFront();
        return;
    }

    AdvancedControlApplyCB(ID, hittype, control); // save axis changes
    CloseWindowCB(ID, hittype, control); // close the controls window

    // return to the main options window and switch it to the SIMULATION tab. The controls
    // window is EXCLUSIVE, so bring SETUP_WIN back up and to the front before selecting the tab.
    C_Window *sw = gMainHandler->FindWindow(SETUP_WIN);

    if (sw)
    {
        gMainHandler->ShowWindow(sw);
        gMainHandler->WindowToFront(sw);

        C_Button *simTab = (C_Button *)sw->FindControl(SIM_TAB);

        if (simTab)
        {
            // #53 select the SIMULATION radio tab visually (green): mirror C_Button::Process for a
            // radio click -- reset the group, then set this tab down. SetupRadioCB only swaps the
            // clusters, it does not move the radio highlight off the CONTROLLERS tab.
            sw->SetGroupState(simTab->GetGroup(), 0);
            simTab->SetState(C_STATE_1);
            simTab->Refresh();

            SetupRadioCB(SIM_TAB, C_TYPE_LMOUSEUP, simTab);
        }
    }
}

/************************************************************************/
// Called when the user presses 'Cancel' (the widget in the upper right
// corner. Just leaves the window without making changes (provided the
// user didn�t press APPLY first..
/************************************************************************/
void AdvancedControlCancelCB(long ID, short hittype, C_Base *control)
{
    if ((hittype not_eq C_TYPE_LMOUSEUP))
        return;

    // Artscout - 2026: modal — can't leave the controls window while the assign dialog is up.
    if (g_baWindowOpen)
    {
        ControlTab_KeepButtonAssignFront();
        return;
    }

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

    PlayerOptions.SetMouseLook(not PlayerOptions.GetMouseLook());
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

    C_Button *b = static_cast<C_Button *>(control);
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

    C_Button *button = (C_Button *)control;

    if (button)
        if (hasForceFeedback == TRUE)
            PlayerOptions.SetFFB(not PlayerOptions.GetFFB());
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

    C_Button *button = (C_Button *)control;

    if (button)
        PlayerOptions.SetClickablePitMode(
            not PlayerOptions.GetClickablePitMode());
}

/************************************************************************/
// "Enable 2D TrackIR" callback function. If no TIR present, or if NP
// software was not active at startup, the button stays always unlit
// and the user won�t be able to make a change to that option.
/************************************************************************/
void TrackIR2dCB(long ID, short hittype, C_Base *control)
{
    if ((hittype not_eq C_TYPE_LMOUSEUP))
        return;

    C_Button *button = (C_Button *)control;

    if (button)
        if (g_bEnableTrackIR)
            PlayerOptions.SetTrackIR2d(not PlayerOptions.Get2dTrackIR());
        else
            button->SetState(C_STATE_0);
}

/************************************************************************/
// "Enable 3D TrackIR" callback function. If no TIR present, or if NP
// software was not active at startup, the button stays always unlit
// and the user won�t be able to make a change to that option.
/************************************************************************/
void TrackIR3dCB(long ID, short hittype, C_Base *control)
{
    if ((hittype not_eq C_TYPE_LMOUSEUP))
        return;

    C_Button *button = (C_Button *)control;

    if (button)
        if (g_bEnableTrackIR)
            PlayerOptions.SetTrackIR3d(not PlayerOptions.Get3dTrackIR());
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

    C_Button *button = (C_Button *)control;

    if (button)
        if (PlayerOptions.GetAxisShaping() == false)
        {
            int result = IO.LoadAxisCalibrationFile();

            if (result == TRUE)
                PlayerOptions.SetAxisShaping(true);
            else // don�t change the options but set the button back to 'unlit'
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

    if (not control)
    {
        ShiAssert(false);
        return;
    }

    int smin, smax, pos;

    smax = ((C_Slider *)control)->GetSliderMax();
    smin = ((C_Slider *)control)->GetSliderMin();
    pos = ((C_Slider *)control)->GetSliderPos();

    // if mouselook is disabled don�t allow the ball to move..
    if (PlayerOptions.GetMouseLook() == false)
    {
        pos =
            (int)RESCALE(PlayerOptions.GetMouseLookSensitivity() * 1000,
                         g_nMouseLookSensMin, g_nMouseLookSensMax, smin, smax);
        ((C_Slider *)control)->SetSliderPos(pos);
        return;
    }

    float theSens =
        RESCALE(pos, smin, smax, g_nMouseLookSensMin, g_nMouseLookSensMax);

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

    if (not control)
    {
        ShiAssert(false);
        return;
    }

    int smin, smax, pos;

    smax = ((C_Slider *)control)->GetSliderMax();
    smin = ((C_Slider *)control)->GetSliderMin();
    pos = ((C_Slider *)control)->GetSliderPos();

    // if no mouse wheel detected don�t allow the ball to move..
    if (IO.MouseWheelExists() == false)
    {
        pos = (int)RESCALE(PlayerOptions.GetMouseWheelSensitivity(),
                           g_nMouseWheelSensMin, g_nMouseWheelSensMax, smin,
                           smax);
        ((C_Slider *)control)->SetSliderPos(pos);
        return;
    }

    PlayerOptions.SetMouseWheelSensitivity((int)RESCALE(
        pos, smin, smax, g_nMouseWheelSensMin, g_nMouseWheelSensMax));
}

/************************************************************************/
// Retro 18Jan2004
// Callback function for the POV / Keyboard panning sensitivity slider
/************************************************************************/
void KeyPOVPanningSensitivityCB(long ID, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_MOUSEMOVE)
        return;

    if (not control)
    {
        ShiAssert(false);
        return;
    }

    int smin, smax, pos;

    smax = ((C_Slider *)control)->GetSliderMax();
    smin = ((C_Slider *)control)->GetSliderMin();
    pos = ((C_Slider *)control)->GetSliderPos();

    PlayerOptions.SetKeyboardPOVPanningSensitivity(
        (int)RESCALE(pos, smin, smax, g_nKeyPOVSensMin, g_nKeyPOVSensMax));
}

/************************************************************************/
// Marks already mapped axis so that they don�t get drawn in other
// axis listboxes than the one they are mapped to
/************************************************************************/
void MarkMappedAxis()
{
    if (not gTotalJoy)
        return;

    // mark all unmapped..
    for (int i = 0; DIAxisNames[i].DXAxisName; i++)
    {
        DIAxisNames[i].isMapped = false;
    }

    // mark the used ones mapped..
    for (int i = 0; DIAxisNames[i].DXAxisName; i++)
    {
        for (int j = 0; j < AXIS_MAX; j++)
        {
            // #53 skip empty UIInputStuff slots (trim axes were removed -> trailing entries are
            // zero-initialized with theDeviceAxis == NULL). Other loops guard via AxisLB==0.
            if (UIInputStuff[j].AxisLB == 0 or
                not UIInputStuff[j].theDeviceAxis)
                continue;

            if (DIAxisNames[i].DXDeviceID ==
                UIInputStuff[j].theDeviceAxis->Device)
            {
                if (DIAxisNames[i].DXAxisID ==
                    UIInputStuff[j].theDeviceAxis->Axis)
                {
                    DIAxisNames[i].isMapped = true;
                }
            }
        }

        // #24: pitch/roll are now regular selectable axes (present in UIInputStuff with a real
        // Device/Axis), so they are marked mapped by the common loop above. The former special case
        // based on FlightControlDevice was removed (its ShiAssert Pitch.Device==FlightControlDevice
        // would fire if pitch is assigned to a device separate from POV/FFB).
    }
}

/************************************************************************/
// Fills axis listboxes with only unused axis. Plus the one axis they are
// mapped to of course.
// the listbox itemdata contains the index of this axis in the array of
// enumerated axis. I need this for saving.
/************************************************************************/
void FillListBox(C_ListBox *theLB, const int theUIAxisIndex)
{
    if (theLB)
    {
        if (gTotalJoy)
        {
            // loop through all enumerated axis and add the unmapped ones..
            for (int i = 0; DIAxisNames[i].DXAxisName; i++)
            {
                theLB->AddItem(i + SIM_JOYSTICK1, C_TYPE_ITEM,
                               DIAxisNames[i].DXAxisName);
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
                if (DIAxisNames[i].DXDeviceID ==
                    UIInputStuff[theUIAxisIndex].theDeviceAxis->Device)
                {
                    if (DIAxisNames[i].DXAxisID ==
                        UIInputStuff[theUIAxisIndex].theDeviceAxis->Axis)
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
// (plus the one that�s mapped to this axis) get listed.
//
// SHOULD ONLY BE DONE ONCE
/************************************************************************/
void PopulateAllListBoxes(C_Window *win)
{
    if (win)
    {
        C_ListBox *listbox;

        for (int UIAxisIndex = 0; UIAxisIndex < AXIS_MAX; UIAxisIndex++)
        {
            if (UIInputStuff[UIAxisIndex].AxisLB == 0)
                continue;

            listbox =
                (C_ListBox *)win->FindControl(UIInputStuff[UIAxisIndex].AxisLB);

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
// (plus the one that�s mapped to this axis) get listed.
//
/************************************************************************/
void RePopulateAllListBoxes(C_Window *win)
{
    if (win)
    {
        C_ListBox *listbox;

        for (int UIAxisIndex = 0; UIAxisIndex < AXIS_MAX; UIAxisIndex++)
        {

            if (UIInputStuff[UIAxisIndex].AxisLB == 0)
                continue;

            listbox =
                (C_ListBox *)win->FindControl(UIInputStuff[UIAxisIndex].AxisLB);

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
                            listbox->SetItemFlags(i + SIM_JOYSTICK1,
                                                  C_BIT_ENABLED);
                        }
                        else
                        {
                            listbox->SetItemFlags(i + SIM_JOYSTICK1,
                                                  C_BIT_INVISIBLE);
                        }

                        // this one is the axis that is mapped to the axis the listbox is about
                        // it is of course mapped so we have to do some fancy coding..
                        if (DIAxisNames[i].DXDeviceID ==
                            UIInputStuff[UIAxisIndex].theDeviceAxis->Device)
                        {
                            if (DIAxisNames[i].DXAxisID ==
                                UIInputStuff[UIAxisIndex].theDeviceAxis->Axis)
                            {
                                // Add the axis and hilight it
                                listbox->SetItemFlags(i + SIM_JOYSTICK1,
                                                      C_BIT_ENABLED);
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

    if (not me)
        return;

    C_ListBox *listbox = (C_ListBox *)me;

    if (not listbox)
        return;

    /* pointer to mommy */
    C_Window *win;

    win = gMainHandler->FindWindow(SETUP_WIN);

    if (win == NULL)
        return;

    win = gMainHandler->FindWindow(SETUP_CONTROL_ADVANCED_WIN);

    if (not win)
        return;

    // k now I figure we can start working ;)

    // looking in the keyboard itemdata for the index of the Axis I am in..
    int i = GetListBoxItemData(listbox, 1, 1);

    if (i == -2) // whoops, error in the above routine..
        return;

    // now I know in which axis box I am
    long index = GetListBoxItemData(listbox, 0);

    if (index == -2) // whoops, error in the above routine..
        return;

    if ((index == -1) and (UIInputStuff[i].theDeviceAxis->Device == -1) and
        (UIInputStuff[i].theDeviceAxis->Axis == -1))
    {
        return; // no change
    }
    else if (index == -1)
    {
        // change from an axis to keyboard
        // now how do I find out the old axis ? hmm
        for (int j = 0; DIAxisNames[j].DXAxisName; j++)
        {
            if ((DIAxisNames[j].DXDeviceID ==
                 UIInputStuff[i].theDeviceAxis->Device) and
                (DIAxisNames[j].DXAxisID ==
                 UIInputStuff[i].theDeviceAxis->Axis))
            {
                ShiAssert(DIAxisNames[j].isMapped == true);

                DIAxisNames[j].isMapped = false;
                UIInputStuff[i].theDeviceAxis->Device = -1;
                UIInputStuff[i].theDeviceAxis->Axis = -1;

                AdvancedControlApplyCB(0, C_TYPE_LMOUSEUP,
                                       0); // Retro 27Mar2004

                RePopulateAllListBoxes(win);
            }
        }

        return;
    }
    else
    {
        // was not keyboard
        // so we have the change from one axis to another axis
        if ((DIAxisNames[index].DXDeviceID ==
             UIInputStuff[i].theDeviceAxis->Device) and
            (DIAxisNames[index].DXAxisID ==
             UIInputStuff[i].theDeviceAxis->Axis))
        {
            ShiAssert(DIAxisNames[index].isMapped == true);
            return; // no change
        }
        else
        {
            // super-special exception case:
            // I don�t want the mouse axis to act as a throttle
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
                if ((DIAxisNames[j].DXDeviceID ==
                     UIInputStuff[i].theDeviceAxis->Device) and
                    (DIAxisNames[j].DXAxisID ==
                     UIInputStuff[i].theDeviceAxis->Axis))
                {
                    DIAxisNames[j].isMapped = false;
                }
            }

            UIInputStuff[i].theDeviceAxis->Device =
                DIAxisNames[index].DXDeviceID;
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

    if (win == NULL)
        return;

    win = gMainHandler->FindWindow(SETUP_CONTROL_ADVANCED_WIN);

    if (not win)
        return;

    // #53 declare client 2 (CONTROLS SETUP table area) here in code — a 3rd [CLIENTAREA] in
    // the .scf window header breaks the parse. x y WIDTH HEIGHT, fitted to the grey panel
    // of WIN_SETUP_NEW (panel 161,125..860,688). Controls tagged [CLIENT] 2 then offset/clip
    // to this rect and the table scrollbars work.
    win->SetClientArea(
        163, 155, 682, 528,
        2); // 163 = TBL_LEFT; top 155 (header row at +2, data at +22)

    // #53 client 3 = MFD joystick-cal window (same 140x140 @273,273 as the old CONTROLLERS page in
    // setup.scf). The JOY_INDICATOR crosshair lives on client 3 and is positioned in RefreshJoystickCB.
    win->SetClientArea(273, 273, 140, 140, 3);

    // #53 initialize the axis value-bar scale here (from THROTTLE_VAL in THIS window). The old
    // init in HookupSetupControls (ui_setup.cpp) runs while building SETUP_WIN and bails with
    // `if (!win2) return;` if the controls window is not loaded yet -> AxisValueBoxWScale stays 0
    // -> every axis indicator computes width 0 (looks "dead"). Do it on window open instead.
    {
        C_Line *vbar = (C_Line *)win->FindControl(SETUP_ADVANCED_THROTTLE_VAL);

        if (vbar)
        {
            AxisValueBox.left = vbar->GetX();
            AxisValueBox.right = vbar->GetX() + vbar->GetW();
            AxisValueBox.top = vbar->GetY();
            AxisValueBox.bottom = vbar->GetY() + vbar->GetH();
            AxisValueBoxHScale = (float)vbar->GetH();
            AxisValueBoxWScale = (float)vbar->GetW();
        }
    }

    // #53 refresh axis "isUsed" flags from the current AxisMap on window open. The value bars
    // only fill for axes where IO.AnalogIsUsed() is true (set by SetupGameAxis from the actual
    // device->axis mapping); otherwise every indicator computes width 0 and looks empty.
    SetupGameAxis();

    // #53 the joystick-poll timer (RefreshJoystickCB -> ButtonAssignAutodetectPoll, axis
    // indicators) used to live on SETUP_WIN cluster 8004 (old Controllers page) and never
    // ticked once controls moved here. Add a timer to THIS window (no cluster = fires on every
    // tab) so autodetect and the axis indicators work. Added once.
    {
        static bool s_ctlTimerAdded = false;

        if (not s_ctlTimerAdded)
        {
            C_TimerHook *tmr = new C_TimerHook;

            if (tmr)
            {
                tmr->Setup(C_DONT_CARE, C_TYPE_TIMER);
                tmr->SetUpdateCallback(GenericTimerCB);
                tmr->SetRefreshCallback(RefreshJoystickCB);
                tmr->SetUserNumber(_UI95_TIMER_DELAY_, 1);
                win->AddControl(tmr);
                s_ctlTimerAdded = true;
            }
        }
    }

    MarkMappedAxis();

    PopulateAllListBoxes(win);

    // fill and init the deadzone listboxes..
    listbox = (C_ListBox *)0;

    for (int j = 0; j < AXIS_MAX; j++)
    {
        if (UIInputStuff[j].DeadzoneLB == 0)
            continue;

        listbox = (C_ListBox *)win->FindControl(UIInputStuff[j].DeadzoneLB);

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
    listbox = (C_ListBox *)0;

    for (int j = 0; j < AXIS_MAX; j++)
    {

        if (UIInputStuff[j].SaturationLB == 0)
            continue;

        listbox = (C_ListBox *)win->FindControl(UIInputStuff[j].SaturationLB);

        if (listbox)
        {
            int theSat = UIInputStuff[j].theDeviceAxis->Saturation;

            if (theSat ==
                SATURATION_NONE) // no saturation. this value is -1  (do not use unsigned with this )
                listbox->SetValue(SETUP_ADVANCED_SAT_NONE);
            else if (
                theSat >=
                g_nSaturationSmall) // 1% saturation, I 'borrowed' the DZ item for this
                listbox->SetValue(SETUP_ADVANCED_DZ_SMALL);
            else if (theSat >= g_nSaturationMedium)
                listbox->SetValue(
                    SETUP_ADVANCED_DZ_MEDIUM); // 5% saturation, I 'borrowed' the DZ item for this
            else // whatever (smaller than 9500) (MEDIUM)
                listbox->SetValue(
                    SETUP_ADVANCED_DZ_LARGE); // 10% saturation, I 'borrowed' the DZ item for this
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

    // #53 register the 3 tabs: CONTROLS SETUP / AXIS SETUP / ADVANCED.
    // The label is set for BOTH states (C_STATE_0/1), otherwise the pressed
    // tab shows a slot with no text and the label "disappears".

    // #53 Tabs are image buttons with baked-in text (SETUP_JOY="CONTROLLERS",
    // SETUP_FLCTL="FLIGHT CONTROLS", B_ADV="ADVANCED"); the DOWN image is the green
    // active state. No SetText (that drew a mismatched-font label over the image).

    // FLIGHT CONTROLS (cluster 10002 = stick/throttle/brake axes) — SETUP_FLCTL image
    button = (C_Button *)win->FindControl(SETUP_ADVANCED_FLIGHT_TAB);

    if (button not_eq NULL)
    {
        button->SetState(C_STATE_0);
        button->SetCallback(SetupControlTabsCB);
    }
    else
        ShiAssert(false);

    // #53 AVIONICS (cluster 10003 = Radar Ant Elev and below) — SETUP_AVCTL image. Restored as a
    // separate tab so the axis list is split across two tabs and neither needs scrolling.
    button = (C_Button *)win->FindControl(SETUP_ADVANCED_AVIONICS_TAB);

    if (button not_eq NULL)
    {
        button->SetState(C_STATE_0);
        button->SetCallback(SetupControlTabsCB);
    }
    else
        ShiAssert(false);

    // ADVANCED (reuses the GENERAL tab id, cluster 10001) — B_ADV image
    button = (C_Button *)win->FindControl(SETUP_ADVANCED_GENERAL_TAB);

    if (button not_eq NULL)
    {
        button->SetState(C_STATE_0);
        button->SetCallback(SetupControlTabsCB);
    }
    else
        ShiAssert(false);

    // CONTROLS SETUP (reuses the MAIN tab id, cluster 10005 = button list) — SETUP_JOY image, default
    button = (C_Button *)win->FindControl(SETUP_CONTROL_TAB_MAIN);

    if (button not_eq NULL)
    {
        button->SetState(C_STATE_1);
        button->SetCallback(SetupControlTabsCB);

        // CONTROLS SETUP is selected when the window opens
        SetupControlTabsCB(SETUP_CONTROL_TAB_MAIN, C_TYPE_LMOUSEUP, button);
    }
    else
        ShiAssert(false);

    button = (C_Button *)win->FindControl(SETUP_ADVANCED_ENABLE_MOUSELOOK);

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

    // #53 "Enable Touch Buddy" checkbox removed from the UI; hardcoded OFF for now.
    PlayerOptions.SetTouchBuddy(false);


    // #53 "Enable 2D TrackIR" checkbox removed from the UI; hardcoded OFF (2D cockpit is dead).
    PlayerOptions.SetTrackIR2d(false);

    // TrackIR callbacks.. check the funcs itself for more explanation
    button = (C_Button *)win->FindControl(SETUP_ADVANCED_ENABLE_3DTIR);

    if (button not_eq NULL)
    {
        if ((g_bEnableTrackIR == true) and
            (PlayerOptions.Get3dTrackIR() == true))
            button->SetState(C_STATE_1);
        else
            button->SetState(C_STATE_0);

        button->SetCallback(TrackIR3dCB);
        button->Refresh();
    }
    else
        ShiAssert(false);

    // Retro 27Jan2004 - Axisshaping button + callback
    button = (C_Button *)win->FindControl(SETUP_ADVANCED_ENABLE_AXISSHAPING);

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

    // this config var doesn�t influence loading or unloading FFB effects,
    // however effect playback is (de)activated on it
    // still need to look at centering though
    button = (C_Button *)win->FindControl(SETUP_ADVANCED_ENABLE_FFB);

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

    // #53 "Set 3D cockpit default" (clickable-pit mode) checkbox removed from the UI; hardcoded ON.
    PlayerOptions.SetClickablePitMode(true);

    // Retro 15Jan2004 - mouselook sensitivity slider
    C_Slider *sldr;

    sldr = (C_Slider *)win->FindControl(SETUP_ADVANCED_MOUSELOOK_SENS);

    if (sldr)
    {
        if ((PlayerOptions.GetMouseLookSensitivity() * 1000) <
            g_nMouseLookSensMin)
            PlayerOptions.SetMouseLookSensitivity(g_nMouseLookSensMin / 1000.f);

        if ((PlayerOptions.GetMouseLookSensitivity() * 1000) >
            g_nMouseLookSensMax)
            PlayerOptions.SetMouseLookSensitivity(g_nMouseLookSensMax / 1000.f);

        int smin, smax, pos;

        smax = sldr->GetSliderMax();
        smin = sldr->GetSliderMin();

        pos =
            (int)RESCALE((PlayerOptions.GetMouseLookSensitivity() * 1000.f),
                         g_nMouseLookSensMin, g_nMouseLookSensMax, smin, smax);

        sldr->SetSliderPos(pos);

        sldr->SetSliderRange(smin, smax);
        sldr->SetCallback(MouseLookSensitivityCB);
        sldr->Refresh();
    }
    else
        ShiAssert(false);

    // Retro 15Jan2004 ends

    // Retro 17JAn2004 - mousewheel sensitivity slider
    sldr = (C_Slider *)win->FindControl(SETUP_ADVANCED_MOUSEWHEEL_SENS);

    if (sldr)
    {
        if (PlayerOptions.GetMouseWheelSensitivity() < g_nMouseWheelSensMin)
            PlayerOptions.SetMouseWheelSensitivity(g_nMouseWheelSensMin);

        if (PlayerOptions.GetMouseWheelSensitivity() > g_nMouseWheelSensMax)
            PlayerOptions.SetMouseWheelSensitivity(g_nMouseWheelSensMax);

        int smin, smax, pos;

        smax = sldr->GetSliderMax();
        smin = sldr->GetSliderMin();

        pos = (int)RESCALE(PlayerOptions.GetMouseWheelSensitivity(),
                           g_nMouseWheelSensMin, g_nMouseWheelSensMax, smin,
                           smax);

        sldr->SetSliderPos(pos);

        sldr->SetSliderRange(smin, smax);
        sldr->SetCallback(MouseWheelSensitivityCB);
        sldr->Refresh();
    }
    else
        ShiAssert(false);

    // Retro 17Jan2004 ends

    // Retro 18Jan2004 - keyboard / POV panning sensitivity slider
    sldr = (C_Slider *)win->FindControl(SETUP_ADVANCED_KEYPOV_SENS);

    if (sldr)
    {
        if (PlayerOptions.GetKeyboardPOVPanningSensitivity() < g_nKeyPOVSensMin)
            PlayerOptions.SetKeyboardPOVPanningSensitivity(g_nKeyPOVSensMin);

        if (PlayerOptions.GetKeyboardPOVPanningSensitivity() > g_nKeyPOVSensMax)
            PlayerOptions.SetKeyboardPOVPanningSensitivity(g_nKeyPOVSensMax);

        int smin, smax, pos;

        smax = sldr->GetSliderMax();
        smin = sldr->GetSliderMin();

        pos = (int)RESCALE(PlayerOptions.GetKeyboardPOVPanningSensitivity(),
                           g_nKeyPOVSensMin, g_nKeyPOVSensMax, smin, smax);

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

    // #53: OK applies everything (axes + key/button bindings) and closes. The separate Apply
    // button was removed — OK now does apply+close, so Apply is redundant.
    button = (C_Button *)win->FindControl(OK);

    if (button)
        button->SetCallback(AdvancedControlOKCB);

    // #53 "Back" button (SIMULATION image): apply + close + return to the SIMULATION page
    button = (C_Button *)win->FindControl(SETUP_CONTROL_BACK);

    if (button)
        button->SetCallback(AdvancedControlBackCB);

    // Cancel: quit without saving
    button = (C_Button *)win->FindControl(CANCEL);

    if (button)
        button->SetCallback(AdvancedControlCancelCB);

    // #53 bottom-left corner exit (B_BACK) / ESC: same as the SIMULATION-image Back button
    // (apply + close + return to the SIMULATION page).
    button = (C_Button *)win->FindControl(CLOSE_WINDOW);

    if (button)
        button->SetCallback(AdvancedControlBackCB);

    // #53 joystick test panel buttons (moved from the old CONTROLLERS page).
    // CALIBRATE actually recenters the stick (RecenterJoystickCB); SET_AB_DETENT: LMB = set AB
    // detent, RMB = set idle cutoff (SetABDetentCB). All persist to axismapping.xml (#57).
    button = (C_Button *)win->FindControl(CALIBRATE);

    if (button)
        button->SetCallback(RecenterJoystickCB);

    button = (C_Button *)win->FindControl(SET_AB_DETENT);

    if (button)
        button->SetCallback(SetABDetentCB);

    // register callbacks for the axis listboxes..
    for (int j = 0; j < AXIS_MAX; j++)
    {
        if (UIInputStuff[j].AxisLB == 0)
            continue;

        listbox = (C_ListBox *)win->FindControl(UIInputStuff[j].AxisLB);

        if (listbox)
            listbox->SetCallback(AxisChangeCB);
        else
            ShiAssert(false);
    }

    InitializeValueBars = 1; // Retro 26Dec2003

    // #53 CONTROLS SETUP tab wiring: this window now hosts the key/button list, so the
    // keyboard-capture callback and the search box live here (used to be on SETUP_WIN).
    win->SetKBCallback(KeystrokeCB);

    {
        C_EditBox *sb = (C_EditBox *)win->FindControl(SETUP_KEY_SEARCH);

        if (sb)
        {
            sb->SetText("");
            sb->SetCallback(KeyListSearchCB);
        }
    }

    // #53 "SETTINGS FOR:" static label + the active profile name as a clickable green link that
    // opens the logbook (same behaviour as the SIMULATION page). Both controls are cluster-less
    // in the .scf, so they show on every tab. The link reuses the SET_LOGBOOK id/callback.
    {
        C_Text *pf = (C_Text *)win->FindControl(SETUP_CTL_PROFILE);

        if (pf)
        {
            pf->SetText("SETTINGS FOR:");
            pf->Refresh();
        }

        C_Button *lb = (C_Button *)win->FindControl(SET_LOGBOOK);

        if (lb)
        {
            lb->SetText(0, UI_logbk.Callsign());
            lb->SetCallback(SetupOpenLogBookCB);
            lb->Refresh();
        }

        // #53 "SEARCH BY ACTION:" label (same font/size as SETTINGS FOR)
        C_Text *sl = (C_Text *)win->FindControl(SETUP_CTL_SEARCHLBL);

        if (sl)
        {
            sl->SetText("SEARCH BY ACTION:");
            sl->Refresh();
        }
    }

    // reset filters and (re)build the list from the active profile (controls.xml + keyboard.xml)
    g_keyFilter[0] = 0;
    g_keyDevFilter = -1;
    UpdateKeyMapList(PlayerOptions.GetKeyfile(), TRUE);

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

void ButtonAssignAutodetectPoll(
    void); // #18: button autodetect (defined below in the assign-window block)
void RebuildKeyListDeferred(
    void); // #52 deferred key-list rebuild (defined below)
void ControlTab_KeepButtonAssignFront(
    void); // Artscout - 2026: modal — keep assign dialog on top (defined below)

void RefreshJoystickCB(long, short, C_Base *)
{
    // #52 deferred rebuild after "Clear" (could not be done synchronously from the button callback).
    if (g_keyListNeedRebuild)
    {
        g_keyListNeedRebuild = false;
        RebuildKeyListDeferred();
        return; // list rebuilt this frame; the rest is updated from the next one
    }

    static SIM_FLOAT JoyXPrev, JoyYPrev, RudderPrev, ThrottlePrev, ABDetentPrev;
    static SIM_FLOAT IdleCutoffPrev; // Retro 1Feb2004
    static DWORD ButtonPrev[SIMLIB_MAX_DIGITAL * SIM_NUMDEVICES],
        POVPrev; // Retro 31Dec2003

    static int state = 1; // Retro 26Dec2003
    C_Bitmap *bmap;
    C_Window *win;
    C_Line *line;
    C_Button *button;

    GetJoystickInput();

    ButtonAssignAutodetectPoll(); // #18: button autodetect for the assign window (if open)
    ControlTab_KeepButtonAssignFront(); // Artscout - 2026: keep the modal assign dialog on top

    // Retro 14Feb2004 - autocenter
    if ((hasForceFeedback) and (PlayerOptions.GetFFB()))
    {
        JoystickPlayEffect(JoyAutoCenter, 10000);
    }

#define UPDATE_ALWAYS // Retro 13Jan2004

    // #53 the button list moved to SETUP_CONTROL_ADVANCED_WIN (CONTROLS SETUP tab).
    // The joystick visualizations (JOY_INDICATOR/RUDDER/THROTTLE/POV) stayed in the old window —
    // here FindControl returns NULL for them and the blocks are simply skipped (all null-guarded).
    win = gMainHandler->FindWindow(SETUP_CONTROL_ADVANCED_WIN);

    if (win not_eq NULL)
    {
#ifndef UPDATE_ALWAYS // Retro 13Jan2004

        //test to see if joystick moved, if so update the control
        if ((IO.analog[AXIS_ROLL].engrValue not_eq JoyXPrev) or
            (IO.analog[AXIS_PITCH].engrValue not_eq JoyYPrev) or
            InitializeValueBars) // Retro 31Dec2003
#endif
        {
            bmap = (C_Bitmap *)win->FindControl(JOY_INDICATOR);

            if (bmap not_eq NULL)
            {
                bmap->Refresh();
                bmap->SetX((int)(JoyScale + IO.analog[AXIS_ROLL].engrValue *
                                                JoyScale)); // Retro 31Dec2003
                bmap->SetY((int)(JoyScale + IO.analog[AXIS_PITCH].engrValue *
                                                JoyScale)); // Retro 31Dec2003
                bmap->Refresh();
                win->RefreshClient(
                    3); // #53 JOY_INDICATOR is on client 3 (MFD cal window)
            }
        }

        if (IO.AnalogIsUsed(AXIS_YAW)) // Retro 31Dec2003
        {
#ifndef UPDATE_ALWAYS // Retro 13Jan2004

            //test to see if rudder moved, if so update the control
            if (((IO.analog[AXIS_YAW].engrValue not_eq RudderPrev) or
                 InitializeValueBars) and
                state) // Retro 31Dec2003
#endif
            {
                line = (C_Line *)win->FindControl(RUDDER);

                if (line not_eq NULL)
                {
                    line->Refresh();
                    line->SetY(
                        (int)(Rudder.top + RudderScale -
                              IO.analog[AXIS_YAW].engrValue * RudderScale +
                              .5)); // Retro 31Dec2003

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
            if (((IO.analog[AXIS_THROTTLE].engrValue not_eq ThrottlePrev) or
                 InitializeValueBars) and
                state) // Retro 31Dec2003
#endif
            {
                line = (C_Line *)win->FindControl(THROTTLE);

                if (line not_eq NULL)
                {
                    line->Refresh();
                    line->SetY(FloatToInt32(
                        static_cast<float>(Throttle.top +
                                           IO.analog[AXIS_THROTTLE].ioVal /
                                               15000.0F * ThrottleScale +
                                           .5))); // Retro 31Dec2003

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
                    line->SetY(FloatToInt32(
                        static_cast<float>(Throttle.top +
                                           IO.analog[AXIS_THROTTLE2].ioVal /
                                               15000.0F * ThrottleScale +
                                           .5))); // Retro 31Dec2003
                else
                    line->SetY(FloatToInt32(
                        static_cast<float>(Throttle.top +
                                           IO.analog[AXIS_THROTTLE].ioVal /
                                               15000.0F * ThrottleScale +
                                           .5))); // Retro 31Dec2003

                if (line->GetY() < Throttle.top)
                    line->SetY(Throttle.top);

                if (line->GetY() > Throttle.bottom)
                    line->SetY(Throttle.bottom);

                line->SetH(Throttle.bottom - line->GetY());
                line->Refresh();
            }

            // Retro 13Jan2004 end

#ifndef UPDATE_ALWAYS // Retro 13Jan2004

            if (ABDetentPrev not_eq IO.analog[AXIS_THROTTLE].center or
                InitializeValueBars) // Retro 31Dec2003
#endif
            {
                line = (C_Line *)win->FindControl(AB_DETENT);

                if (line not_eq NULL)
                {
                    line->Refresh();
                    line->SetY(FloatToInt32(
                        static_cast<float>(Throttle.top +
                                           IO.analog[AXIS_THROTTLE].center /
                                               15000.0F * ThrottleScale +
                                           .5))); // Retro 31Dec2003

                    if (line->GetY() <= Throttle.top - 1)
                        line->SetY(Throttle.top);

                    if (line->GetY() >= Throttle.bottom)
                        line->SetY(Throttle.bottom + 1);

                    line->Refresh();
                }
            }

#ifndef UPDATE_ALWAYS // Retro 13Jan2004

            if (IdleCutoffPrev not_eq IO.analog[AXIS_THROTTLE].cutoff or
                InitializeValueBars) // Retro 31Dec2003
#endif
            {
                line = (C_Line *)win->FindControl(SETUP_IDLE_CUTOFF);

                if (line not_eq NULL)
                {
                    line->Refresh();
                    line->SetY(FloatToInt32(
                        static_cast<float>(Throttle.top +
                                           IO.analog[AXIS_THROTTLE].cutoff /
                                               15000.0F * ThrottleScale +
                                           .5))); // Retro 31Dec2003

                    if (line->GetY() <= Throttle.top - 1)
                        line->SetY(Throttle.top);

                    if (line->GetY() >= Throttle.bottom)
                        line->SetY(Throttle.bottom + 1);

                    line->Refresh();
                }
            }
        }


        unsigned long i;

        // EDGE detection for button assignment: which buttons were held in the previous frame.
        // Without it, 3-position switches (permanently "pressed") hijacked the assignment —
        // you could not assign ANY button. Now we assign only on a NEW press.
        static char s_editPrevDigital[SIMLIB_MAX_DIGITAL * SIM_NUMDEVICES] = {
            0};
        // #51 "armed-after-release": a button becomes "armed" (ready to be shown/
        // assigned) ONLY after we have seen it RELEASED at least once. Permanently pressed ones
        // (always =1: stuck buttons, 3-pos switches in the active position) never get armed ->
        // are not mixed in, NO MATTER HOW MANY there are and regardless of when the window opened (the baseline
        // above is timing-dependent and failed with several stuck buttons).
        static char s_editArmed[SIMLIB_MAX_DIGITAL * SIM_NUMDEVICES] = {0};

        // #51 baseline: on the first poll after opening/switching the tab, capture the CURRENT
        // state as prev -> already-held (stuck/permanently-pressed) buttons do not give
        // a false edge (not shown in INPUT and not self-assigned). We react only to
        // NEW presses after entering.
        if (g_editPollReseed)
        {
            for (i = 0; i < SIMLIB_MAX_DIGITAL * SIM_NUMDEVICES; i++)
                s_editPrevDigital[i] = (IO.digital[i] != 0);

            g_editPollReseed = false;
        }

        for (i = 0; i < SIMLIB_MAX_DIGITAL * SIM_NUMDEVICES;
             i++) // Retro 31Dec2003
        {
            // Retro 31Dec2003:
            // actually I only want to show the buttons on the flight control device here..
            // if FFB is enabled then the user also gets effects
            if (AxisMap.FlightControlDevice not_eq -1)
            {
                int theIndex = (AxisMap.FlightControlDevice - SIM_JOYSTICK1) *
                               SIMLIB_MAX_DIGITAL;

                if ((i >= (unsigned long)theIndex) and
                    (i < (unsigned long)theIndex + 8))
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

            // #51 arm a button as soon as we see it RELEASED (cur==0). A permanently pressed one
            // never reaches here -> stays unarmed -> is filtered out below.
            if (not IO.digital[i])
                s_editArmed[i] = 1;

            // On the EDGE (new press) AND only if the button is armed (was released at least once):
            // otherwise stuck/permanently-pressed 3-pos switches show "Button N" (spam) and
            // self-assign.
            if (IO.digital[i] and not s_editPrevDigital[i] and s_editArmed[i])
            {
                C_Text *text = (C_Text *)win->FindControl(CONTROL_KEYS);

                if (text)
                {
                    char string[_MAX_PATH];
                    text->Refresh();
                    // #51 show "<device(truncated)> btn N" instead of the uninformative
                    // global "Button 369". gDIDevNames is indexed by the device's SIM index
                    // (as filled in sijoy.cpp: gDIDevNames[SIM_JOYSTICK1+joy]); SIM index =
                    // SIM_JOYSTICK1 + i/128, local button = i%128+1. The name is truncated (%.14s).
                    {
                        int dev = SIM_JOYSTICK1 + (i / SIMLIB_MAX_DIGITAL);
                        int localBtn = i % SIMLIB_MAX_DIGITAL;
                        const char *dn =
                            (dev >= SIM_JOYSTICK1 and dev < SIM_NUMDEVICES and
                             gDIDevNames[dev]) ?
                                gDIDevNames[dev] :
                                NULL;

                        if (dn)
                            sprintf(string, "%.14s  %s %d", dn,
                                    gStringMgr->GetString(TXT_BUTTON),
                                    localBtn + 1);
                        else
                            sprintf(string, "%s %d",
                                    gStringMgr->GetString(TXT_BUTTON), i + 1);
                    }
                    text->SetText(string);
                    text->Refresh();
                }

                // (the outer if already guarantees an edge — new press only)
                if (KeyVar.EditKey)
                {
                    button = (C_Button *)win->FindControl(KeyVar.CurrControl);
                    UserFunctionTable.SetButtonFunction(
                        i, (InputFunctionType)button->GetUserPtr(FUNCTION_PTR),
                        button->GetUserNumber(BUTTON_ID));
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

                        C_Button *tButton =
                            (C_Button *)win->FindControl(KEYCODES);

                        while (tButton)
                        {
                            if (func ==
                                (InputFunctionType)tButton->GetUserPtr(5))
                            {
                                C_Text *temp = (C_Text *)win->FindControl(
                                    tButton->GetID() - KEYCODES + MAPPING);

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
                                tButton = (C_Button *)win->FindControl(
                                    KEYCODES + i++);
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

                // Artscout - 2026 (#95): scroll the function list to the action bound to the just-pressed
                // button. Rising-edge only (the outer if already filters held/stuck 3-pos switches), and not
                // while the modal button-assign popup is up. Find the visible row whose FUNCTION_PTR matches
                // the button's bound function and scroll the client area to it (replaces the removed
                // FUNCTION_LIST text-display -- the "small window that showed the bound action").
                extern bool g_baActive;
                if (not g_baActive)
                {
                    InputFunctionType sfunc =
                        UserFunctionTable.GetButtonFunction(i, NULL);
                    C_Button *anchor = (C_Button *)win->FindControl(KEYCODES);
                    C_Line *vln = (C_Line *)win->FindControl(VLINE);

                    if (sfunc and anchor and vln)
                    {
                        C_Button *srow = NULL;

                        for (int n = 0; n < NumDispKeys; ++n)
                        {
                            C_Button *rb =
                                (C_Button *)win->FindControl(KEYCODES + n);

                            if (rb and (InputFunctionType) rb->GetUserPtr(
                                           FUNCTION_PTR) == sfunc)
                            {
                                srow = rb;
                                break;
                            }
                        }

                        long rowH = vln->GetH();

                        if (srow and rowH > 0)
                        {
                            long kc = anchor->GetClient();
                            int count = srow->GetID() - KEYCODES;
                            int lead = (count > 2) ?
                                           (count - 2) :
                                           0; // 2-row lead-in for context

                            // Draw model (cwindow.cpp): a control's screen Y = control.Y + VY_, visible window
                            // = [ClientArea.top, ClientArea.bottom]; SetVirtualY(y) sets VY_ = -y. So to put the
                            // target row (at anchor->GetY() + rowH*count) at the client top: VY_ = ca.top -
                            // (anchor->GetY() + rowH*lead) -> y = anchor->GetY() + rowH*lead - ca.top.
                            UI95_RECT ca = win->GetClientArea(kc);
                            long y = anchor->GetY() + rowH * lead - ca.top;

                            win->SetVirtualY(
                                y, kc); // scroll the client to the target row
                            win->ScanClientArea(
                                kc); // clamp to range + re-sync scrollbar visibility
                            win->AdjustScrollbar(
                                kc); // move the slider to match

                            // Artscout - 2026 (#95): highlight the target row's text -- rows colour their text
                            // via SetFgColor(0,...) (SetButtonColor: green/white). Set it yellow here and
                            // restore the previously highlighted row with SetButtonColor. Reversible; a list
                            // rebuild recolours all rows to normal anyway.
                            static int s_hiliteRow = -1;
                            if (s_hiliteRow >= 0 and s_hiliteRow != count)
                            {
                                C_Button *prev = (C_Button *)win->FindControl(
                                    KEYCODES + s_hiliteRow);
                                if (prev)
                                    SetButtonColor(
                                        prev); // restore normal green/white
                            }
                            srow->SetFgColor(
                                0, RGB(255, 255, 0)); // highlight: yellow text
                            srow->Refresh();
                            s_hiliteRow = count;

                            win->RefreshClient(kc); // redraw the list
                        }
                    }
                }
            }

            // remember the button state for edge detection on the next frame
            s_editPrevDigital[i] = (IO.digital[i] != 0);
        }

        int Direction;
        int flags = 0;

        for (i = 0; i < NumberOfPOVs; i++) // Retro 26Dec2003
        {
            Direction = 0;

            if ((IO.povHatAngle[i] < 2250 or IO.povHatAngle[i] > 33750) and
                IO.povHatAngle[i] not_eq -1)
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
                UserFunctionTable.SetPOVFunction(
                    i, Direction,
                    (InputFunctionType)button->GetUserPtr(FUNCTION_PTR),
                    button->GetUserNumber(BUTTON_ID));
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
                    sprintf(button, "%s %d : %s",
                            gStringMgr->GetString(TXT_POV), i + 1,
                            gStringMgr->GetString(TXT_UP + Direction));
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
                            C_Text *temp = (C_Text *)win->FindControl(
                                tButton->GetID() - KEYCODES + MAPPING);

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
                            tButton =
                                (C_Button *)win->FindControl(KEYCODES + i++);
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
        IdleCutoffPrev = static_cast<float>(
            IO.analog[AXIS_THROTTLE].cutoff); // Retro 1Feb2004

        POVPrev = IO.povHatAngle[0];
    }

    //if(Calibration.calibrating)
    // Calibrate();

    // Retro - trying to get some of this shit into my advanced controller window..
    win = gMainHandler->FindWindow(SETUP_CONTROL_ADVANCED_WIN);

    if (not win)
        return;

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
                    newWidth = (float)FloatToInt32(static_cast<float>(
                        IO.analog[UIInputStuff[i].InGameAxis].ioVal / 15000.0F *
                            AxisValueBoxWScale +
                        .5));
                else
                    newWidth = (float)FloatToInt32(static_cast<float>(
                        AxisValueBoxWScale / 2. +
                        IO.analog[UIInputStuff[i].InGameAxis].ioVal / 20000.0F *
                            AxisValueBoxWScale +
                        .5));

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
} //RefreshJoystickCB

SIM_INT CalibrateFile(void)
{
    int i, numAxis;
    FILE *filePtr;

    char fileName[_MAX_PATH];
    sprintf(fileName, "%s/config/joystick.dat", FalconDataDirectory);

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
 sprintf(filename,"%s/config/joystick.dat",FalconDataDirectory);
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


// #22: function-list filter in the main controls window (needed already in KeystrokeCB
// for the ESC search reset, hence declared here, above first use).
// g_keyFilter   — search substring over function name/description (empty = show all).
// g_keyDevFilter— device whose bindings are shown in the LEFT column: < SIM_JOYSTICK1
//                 (Keyboard) = keyboard combos as before; >= SIM_JOYSTICK1 = the button
//                 of that joystick assigned to the function (or empty).
char g_keyFilter[64] = "";
int g_keyDevFilter = -1;
// true = UpdateKeyMapList only rebuilds the rows (without re-reading the function table),
// so the filter does not lose unsaved button assignments (buttonTable in memory).
static bool g_keyListDisplayOnly = false;

// UpdateKeyMapList is defined below — forward declaration for KeystrokeCB/SaveKeyMapList.
int UpdateKeyMapList(char *fname, int flag);

BOOL KeystrokeCB(unsigned char DKScanCode, unsigned char,
                 unsigned char ShiftStates, long)
{
    if (DKScanCode == DIK_ESCAPE)
    {
        // #22: ESC while the search field is active — clear text, reset the filter, release
        // focus (restore default key scanning) and do NOT close the settings window.
        C_Window *sw = gMainHandler->FindWindow(
            SETUP_CONTROL_ADVANCED_WIN); // #53 list is in the new window

        if (sw and sw->GetCurControl() and
            sw->GetCurControl()->GetID() == SETUP_KEY_SEARCH)
        {
            ((C_EditBox *)sw->GetCurControl())->SetText("");
            sw->ClearActiveControl();
            g_keyFilter[0] = 0;
            g_keyListDisplayOnly = true;
            UpdateKeyMapList(PlayerOptions.GetKeyfile(), TRUE);
            g_keyListDisplayOnly = false;
            return TRUE; // ate the ESC — do not close the window
        }

        return FALSE;
    }

    if (Cluster ==
        10005) // #53 the button list is now in the new window's CONTROLS SETUP cluster
    {
        // #22: if the function search field is active — do NOT intercept keys (otherwise the window's
        // KB callback eats the character before the editbox and nothing is typed into search). Return
        // FALSE → C_Window::CheckKeyboard routes the key to the active control (editbox).
        {
            C_Window *sw = gMainHandler->FindWindow(
                SETUP_CONTROL_ADVANCED_WIN); // #53 list is in the new window

            if (sw and sw->GetCurControl() and
                sw->GetCurControl()->GetID() == SETUP_KEY_SEARCH)
                return FALSE;
        }

        if (DKScanCode == DIK_LSHIFT or DKScanCode == DIK_RSHIFT or
            DKScanCode == DIK_LCONTROL or DKScanCode == DIK_RCONTROL or
            DKScanCode == DIK_LMENU or DKScanCode == DIK_RMENU or
            DKScanCode == 0x45)
            return TRUE;

        if (GetAsyncKeyState(VK_SHIFT) bitand 0x8001)
            ShiftStates or_eq _SHIFT_DOWN_;
        else
            ShiftStates and_eq compl _SHIFT_DOWN_;

        //int flags = ShiftStates;
        int flags = ShiftStates +
                    (KeyVar.CommandsKeyCombo << SECOND_KEY_SHIFT) +
                    (KeyVar.CommandsKeyComboMod << SECOND_KEY_MOD_SHIFT);

        C_Window *win;
        int CommandCombo = 0;

        win = gMainHandler->FindWindow(
            SETUP_CONTROL_ADVANCED_WIN); // #53 button list is in the new window

        if (KeyVar.EditKey)
        {
            C_Button *button;
            long ID;

            button = (C_Button *)win->FindControl(KeyVar.CurrControl);
            flags = ShiftStates +
                    (button->GetUserNumber(FLAGS) bitand SECOND_KEY_MASK);
            KeyVar.CommandsKeyCombo =
                (button->GetUserNumber(FLAGS) bitand KEY1_MASK) >>
                SECOND_KEY_SHIFT;
            KeyVar.CommandsKeyComboMod =
                (button->GetUserNumber(FLAGS) bitand MOD1_MASK) >>
                SECOND_KEY_MOD_SHIFT;

            //here is where we need to change the key combo for the function
            if (DKScanCode not_eq button->GetUserNumber(KEY2) or
                flags not_eq button->GetUserNumber(FLAGS))
            {
                char keydescrip[_MAX_PATH];
                keydescrip[0] = 0;
                int pmouse, pbutton;
                InputFunctionType theFunc;
                InputFunctionType oldFunc;
                theFunc = (InputFunctionType)button->GetUserPtr(FUNCTION_PTR);

                //is the key combo already used?
                if (oldFunc = UserFunctionTable.GetFunction(DKScanCode, flags,
                                                            &pmouse, &pbutton))
                {
                    C_Button *temp;

                    ID = UserFunctionTable.GetControl(DKScanCode, flags);
                    KeyVar.OldControl = ID;


                    //there is a function mapped but it's not visible
                    //don't allow user to remap this key combo
                    if (not ID and oldFunc)
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
                UserFunctionTable.RemoveFunction(button->GetUserNumber(KEY2),
                                                 button->GetUserNumber(FLAGS));

                //add function into it's new place
                UserFunctionTable.AddFunction(
                    DKScanCode, flags, button->GetUserNumber(BUTTON_ID),
                    button->GetUserNumber(MOUSE_SIDE), theFunc);
                UserFunctionTable.SetControl(DKScanCode, flags,
                                             KeyVar.CurrControl);

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
                ShiAssert(FALSE ==
                          IsBadStringPtr(KeyDescrips[DKScanCode], MAX_PATH));

                if (KeyVar.CommandsKeyCombo > 0)
                {
                    DoShiftStates(firstKey, KeyVar.CommandsKeyComboMod);
                    DoShiftStates(mods, ShiftStates);

                    if (KeyDescrips[DKScanCode])
                        _stprintf(totalDescrip, "%s%s : %s%s", firstKey,
                                  KeyDescrips[KeyVar.CommandsKeyCombo], mods,
                                  KeyDescrips[DKScanCode]);
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
                        temp->SetUserNumber(FLAGS,
                                            temp->GetUserNumber(FLAGS) bitand
                                                SECOND_KEY_MASK);
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
                    _stprintf(totalDescrip, "%s%s : %s%s", firstKey,
                              KeyDescrips[KeyVar.CommandsKeyCombo], mods,
                              KeyDescrips[DKScanCode]);
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
            function = UserFunctionTable.GetFunction(DKScanCode, flags, &pmouse,
                                                     &pbutton);

            text = (C_Text *)win->FindControl(FUNCTION_LIST);

            if (text)
            {
                text->Refresh();

                if (function)
                {
                    C_Text *temp;
                    C_Button *btn;
                    long ID;

                    ID = UserFunctionTable.GetControl(DKScanCode, flags);

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

                    if (not KeyVar.EditKey and temp)
                    {
                        UI95_RECT Client;
                        Client = win->GetClientArea(temp->GetClient());

                        win->SetVirtualY(temp->GetY() - Client.top,
                                         temp->GetClient());
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

// defined later in the file — forward declaration for block #18
void BuildControllerList(C_ListBox *lbox);

/****************************************************************************/
// #18 — the "Button assignment" window (SETUP_BTNASSIGN_WIN).
// Opened from KeycodeCB instead of inline capture. List = functions, column =
// button of the selected device. Assignment: pick a button from the dropdown (manual)
// OR autodetect (the existing per-frame capture, KeyVar.EditKey stays TRUE).
// Modifier — keyboard only. Device identity is stable (GUID, #19).
/****************************************************************************/
static InputFunctionType g_baTargetFunc = NULL; // function being assigned
static int g_baTargetCpId = 0; // cockpit button id (from the source row)
static long g_baSourceCtrl = 0; // the source row control on the main screen
static int g_baDevice = -1; // SIM index of the selected device
static int g_baMod = 0; // selected modifier (0..7, keyboard)
static char g_baSearch[64] = {
    0}; // function search string (not used in the popup)
static int g_baStagedButton = -1; // selected/detected button (applied on OK)
static bool g_baStagedClear =
    false; // #53 "Clear" staged: OK clears this function's binding on g_baDevice
static int g_baOpenDevice =
    -1; // #53 device to preselect when opening (set by a device-cell click)
bool g_baActive = false; // assign window is open (autodetect gate)
// Artscout - 2026: g_baActive is the *autodetect* gate and is cleared by the poll once a button
// is captured, so it can't tell whether the window is still on screen. This dedicated flag tracks
// the window's open/closed state for the modal behaviour (block leaving options while it is up).
bool g_baWindowOpen = false;
bool g_baPollInit = false; // first poll frame: only capture prev (fresh IO)
short g_baPrevDigital[SIMLIB_MAX_DIGITAL * SIM_NUMDEVICES] = {
    0}; // prev button state (for CHANGE detection)
short g_baStable[SIMLIB_MAX_DIGITAL * SIM_NUMDEVICES] = {
    0}; // #15: consecutive polls in the current state (since the last change)
short g_baChanged[SIMLIB_MAX_DIGITAL * SIM_NUMDEVICES] = {
    0}; // #15: button changed state AFTER the window opened (excludes untouched "permanently pressed" ones)
short g_baArmed[SIMLIB_MAX_DIGITAL * SIM_NUMDEVICES] = {
    0}; // #51: armed (seen released) -> permanently-pressed ones are not detected
#define BA_HOLD_THRESHOLD                                                      \
    6 // a changed button must stay stable for N polls -> catches a switch click, rejects oscillation/jitter

static const char *g_baModNames[8] = {
    "None", "Shift",     "Ctrl",     "Ctrl+Shift",
    "Alt",  "Alt+Shift", "Ctrl+Alt", "Ctrl+Alt+Shift"};

static char baLower(char c)
{
    return (c >= 'A' and c <= 'Z') ? (char)(c + 32) : c;
}

// case-insensitive substring search
static bool baMatch(const char *hay, const char *needle)
{
    if (not needle or not needle[0])
        return true;

    if (not hay)
        return false;

    for (int i = 0; hay[i]; ++i)
    {
        int k = 0;

        while (needle[k] and hay[i + k] and
               baLower(hay[i + k]) == baLower(needle[k]))
            ++k;

        if (not needle[k])
            return true;
    }

    return false;
}

// fill the button dropdown of the selected device (manual mode)
static void FillButtonAssignButtonList(C_Window *win)
{
    if (not win)
        return;

    C_ListBox *bl = (C_ListBox *)win->FindControl(BTNASSIGN_BUTTON_LIST);

    if (not bl)
        return;

    bl->RemoveAllItems();

    if (g_baDevice >= SIM_JOYSTICK1 and g_baDevice < SIM_NUMDEVICES)
    {
        int cnt = gDIDevButtons[g_baDevice];
        char s[32];

        for (int b = 0; b < cnt; ++b)
        {
            sprintf(s, "Button %d", b + 1);
            bl->AddItem(b + 1, C_TYPE_ITEM, s); // item id = btn+1
        }
    }

    bl->Refresh();
}

// fill the function list; column = button of the selected device (empty if none)
static void FillButtonAssignFuncList(C_Window *win)
{
    if (not win)
        return;

    C_ListBox *fl = (C_ListBox *)win->FindControl(BTNASSIGN_FUNC_LIST);

    if (not fl)
        return;

    fl->RemoveAllItems();

    bool isJoy = (g_baDevice >= SIM_JOYSTICK1 and g_baDevice < SIM_NUMDEVICES);
    int base = (g_baDevice - SIM_JOYSTICK1) * SIMLIB_MAX_DIGITAL;
    int btnCount = isJoy ? gDIDevButtons[g_baDevice] : 0;
    int n = GetUserFunctionCount();

    for (int i = 0; i < n; ++i)
    {
        char *name = GetUserFunctionName(i);

        if (not name)
            continue;

        if (not baMatch(name, g_baSearch))
            continue;

        char col[24] = "";

        if (isJoy)
        {
            InputFunctionType func = GetUserFunctionByIndex(i);

            for (int b = 0; b < btnCount; ++b)
            {
                int cp;

                if (UserFunctionTable.GetButtonFunction(base + b, &cp) == func)
                {
                    sprintf(col, "Btn %d", b + 1);
                    break;
                }
            }
        }

        char row[160];
        sprintf(row, "%-34.34s %s", name, col);
        fl->AddItem(i + 1, C_TYPE_ITEM, row); // item id = idx+1
    }

    fl->Refresh();
}

// #18: per-frame autodetect — catches a NEW button press (edge), sets
// the device+button in the popup. Called from RefreshJoystickCB while g_baActive.
void ButtonAssignAutodetectPoll(void)
{
    if (not g_baActive)
        return;

    // #53 g_baStable[i] is reused as the press-order (rise sequence) of button i; s_holdCand/
    // s_holdCount track how long the candidate detent has been held before committing.
    static int s_riseSeq = 0;
    static int s_holdCand = -1;
    static int s_holdCount = 0;

    // first frame after opening: capture the BASE state of all buttons from the FRESH
    // IO.digital. Untouched "permanently pressed" switches then produce no "change".
    if (g_baPollInit)
    {
        for (int i = 0; i < SIMLIB_MAX_DIGITAL * SIM_NUMDEVICES; ++i)
        {
            g_baPrevDigital[i] = (short)(IO.digital[i] != 0);
            g_baStable[i] = 0; // rise-order: 0 = not pressed
            g_baChanged[i] = 0;
            // #51 do NOT pre-arm from a snapshot (timing-dependent: if IO.digital on the open frame
            // is not fresh, ALL held buttons read as 0 -> get armed -> fire; with several
            // stuck ones this is exactly what broke). Start all UNarmed; arm ONLY when we actually
            // see a release (cur==0, below). Already-pressed ones never get armed — no matter how many.
            g_baArmed[i] = 0;
        }

        s_riseSeq = 0;
        s_holdCand = -1;
        s_holdCount = 0;
        g_baPollInit = false;
        return;
    }

    C_Window *baw = gMainHandler->FindWindow(SETUP_BTNASSIGN_WIN);

    // #53 device-locked autodetect: only scan buttons of the device chosen by the clicked cell
    // (g_baDevice). Buttons on other devices are ignored (no device switching in the window).
    if (g_baDevice < SIM_JOYSTICK1 or g_baDevice >= SIM_NUMDEVICES)
        return;

    int base = (g_baDevice - SIM_JOYSTICK1) * SIMLIB_MAX_DIGITAL;
    int cnt = gDIDevButtons[g_baDevice];

    if (cnt <= 0 or cnt > SIMLIB_MAX_DIGITAL)
        cnt = SIMLIB_MAX_DIGITAL;

    // #53 dual-detent triggers: a full pull presses detent-1 (e.g. btn1) THEN detent-2 (btn6).
    // Pick the ARMED + currently-held button with the LATEST rising edge (the deepest detent
    // being held) and commit only once it is held stable for a short while (BA_HOLD_THRESHOLD).
    // Permanently-pressed buttons never get armed (#51) -> stuck switches/3-pos toggles ignored.
    int cand = -1, candSeq = -1;

    for (int b = 0; b < cnt; ++b)
    {
        int i = base + b;
        short cur = (short)(IO.digital[i] != 0);

        if (not cur)
        {
            g_baArmed[i] = 1; // released -> armed
            g_baPrevDigital[i] = 0;
            g_baStable[i] = 0; // rise-order cleared
            continue;
        }

        if (not g_baPrevDigital[i]) // rising edge
        {
            g_baPrevDigital[i] = 1;

            if (g_baArmed[i])
                g_baStable[i] = (short)(++s_riseSeq); // remember press order
        }

        if (g_baArmed[i] and
            g_baStable[i] > candSeq) // latest-risen held armed button
        {
            candSeq = g_baStable[i];
            cand = b;
        }
    }

    if (cand < 0) // nothing (armed) held -> reset hold tracking
    {
        s_holdCand = -1;
        s_holdCount = 0;
        return;
    }

    if (cand == s_holdCand)
        s_holdCount++;
    else
    {
        s_holdCand = cand;
        s_holdCount = 1;
    }

    if (s_holdCount < BA_HOLD_THRESHOLD)
        return; // keep holding the desired detent to commit

    // commit the held detent (device stays g_baDevice)
    g_baStagedButton = cand;
    g_baActive = false;

    if (baw)
    {
        C_Text *dl = (C_Text *)baw->FindControl(BTNASSIGN_DEVICE_LABEL);

        if (dl and gDIDevNames[g_baDevice])
        {
            dl->SetText(gDIDevNames[g_baDevice]);
            dl->Refresh();
        }

        FillButtonAssignButtonList(baw);

        C_ListBox *bl = (C_ListBox *)baw->FindControl(BTNASSIGN_BUTTON_LIST);

        if (bl)
        {
            bl->SetValue(g_baStagedButton + 1);
            bl->Refresh();
        }

        C_Text *d = (C_Text *)baw->FindControl(BTNASSIGN_DETECTED);

        if (d)
        {
            char s[96];
            sprintf(s, "Detected: Button %d", g_baStagedButton + 1);
            d->SetText(s);
            d->Refresh();
        }

        // Artscout - 2026: SetText shrinks a non-fixed C_Text to the new (shorter) string and the
        // vacated area isn't repainted, leaving a tail of the longer prompt ("...assign"). Force a
        // full window redraw so the background under the old text is cleared.
        baw->RefreshWindow();
    }
}

// manual button selection from the dropdown -> staged (applied on OK)
void ButtonAssignButtonCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_SELECT)
        return;

    int btn = ((C_ListBox *)control)->GetTextID() - 1;

    if (btn < 0)
        return;

    g_baStagedButton = btn;

    C_Text *d = (C_Text *)control->Parent_->FindControl(BTNASSIGN_DETECTED);

    if (d)
    {
        char s[80];
        sprintf(s, "Selected: Button %d", btn + 1);
        d->SetText(s);
        d->Refresh();
        ((C_Window *)control->Parent_)
            ->RefreshWindow(); // clear any tail of the longer prompt
    }
}

// #53 "Clear" staged action: on OK, remove this function's binding from the selected device
// (all of that device's buttons currently bound to the function). Staged so Cancel is a no-op.
void ButtonAssignClearCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    g_baStagedClear = true;
    g_baStagedButton = -1; // clearing wins over any pending assignment
    g_baActive =
        false; // stop autodetect so it does not stage a button over the clear

    C_Text *d =
        control ? (C_Text *)control->Parent_->FindControl(BTNASSIGN_DETECTED) :
                  NULL;

    if (d)
    {
        const char *dn =
            (g_baDevice >= SIM_JOYSTICK1 and g_baDevice < SIM_NUMDEVICES and
             gDIDevNames[g_baDevice]) ?
                gDIDevNames[g_baDevice] :
                "this device";
        char s[120];
        sprintf(s, "Will clear binding on %.40s (press OK)", dn);
        d->SetText(s);
        d->Refresh();
        ((C_Window *)control->Parent_)
            ->RefreshWindow(); // clear any tail of the previous text
    }
}

// OK: apply the staged action — either clear this device's binding, or assign the staged
// button of the selected device to the function.
void ButtonAssignOkCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (g_baStagedClear and g_baTargetFunc and g_baDevice >= SIM_JOYSTICK1 and
        g_baDevice < SIM_NUMDEVICES)
    {
        // #53 unbind every button of g_baDevice currently mapped to this function
        int base = (g_baDevice - SIM_JOYSTICK1) * SIMLIB_MAX_DIGITAL;
        int cnt = gDIDevButtons[g_baDevice];

        if (cnt <= 0 or cnt > SIMLIB_MAX_DIGITAL)
            cnt = SIMLIB_MAX_DIGITAL;

        for (int b = 0; b < cnt; ++b)
        {
            if (UserFunctionTable.GetButtonFunction(base + b, NULL) ==
                g_baTargetFunc)
                UserFunctionTable.SetButtonFunction(base + b, NULL, -1);
        }

        KeyVar.Modified = TRUE;
        g_keyListNeedRebuild = true; // refresh the table next frame
    }
    else if (g_baTargetFunc and g_baStagedButton >= 0 and
             g_baDevice >= SIM_JOYSTICK1 and g_baDevice < SIM_NUMDEVICES)
    {
        int buttonId = (g_baDevice - SIM_JOYSTICK1) * SIMLIB_MAX_DIGITAL +
                       g_baStagedButton;
        UserFunctionTable.SetButtonFunction(buttonId, g_baTargetFunc,
                                            g_baTargetCpId);
        KeyVar.Modified = TRUE;
        g_keyListNeedRebuild = true;
    }

    g_baActive = false;
    g_baWindowOpen = false;
    gMainHandler->HideWindow(control->Parent_);
}

// Cancel: close without applying, turn off autodetect
void ButtonAssignCancelCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    g_baActive = false;
    g_baWindowOpen = false;
    gMainHandler->HideWindow(control->Parent_);
}

// Artscout - 2026: modal helpers used by the setup window. While the assign dialog is open the
// user must not be able to leave options; only its own OK/Cancel close it.
bool ControlTab_IsButtonAssignOpen(void)
{
    return g_baWindowOpen;
}

void ControlTab_ForceCloseButtonAssign(void)
{
    g_baActive = false;
    g_baWindowOpen = false;

    C_Window *win = gMainHandler->FindWindow(SETUP_BTNASSIGN_WIN);

    if (win)
        gMainHandler->HideWindow(win);
}

// Keep the dialog on top of the setup window while it is open (called once per poll frame).
void ControlTab_KeepButtonAssignFront(void)
{
    if (not g_baWindowOpen)
        return;

    C_Window *win = gMainHandler->FindWindow(SETUP_BTNASSIGN_WIN);

    if (win)
        gMainHandler->WindowToFront(win);
}

void ButtonAssignModCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_SELECT)
        return;

    g_baMod = ((C_ListBox *)control)->GetTextID() - 1;
}

// #53: human-readable description for a function pointer, taken from the MAPPING column of the
// controls table (the same text shown in the function list), NOT the internal callback name.
static char *ControlTab_DescribeFunction(InputFunctionType func)
{
    if (not func)
        return NULL;

    C_Window *win = gMainHandler->FindWindow(SETUP_CONTROL_ADVANCED_WIN);

    if (not win)
        return NULL;

    int n = 0;
    C_Button *b;

    while ((b = (C_Button *)win->FindControl(KEYCODES + n)) != NULL)
    {
        if (func == (InputFunctionType)b->GetUserPtr(FUNCTION_PTR))
        {
            C_Text *t =
                (C_Text *)win->FindControl(b->GetID() - KEYCODES + MAPPING);
            return t ? t->GetText() : NULL;
        }

        n++;
    }

    return NULL;
}

// pick a different target function from the list
void ButtonAssignFuncCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_SELECT)
        return;

    int idx = ((C_ListBox *)control)->GetTextID() - 1;
    g_baTargetFunc = GetUserFunctionByIndex(idx);
    g_baTargetCpId =
        0; // cockpit button id is unknown for an arbitrarily chosen function

    C_Text *t = (C_Text *)control->Parent_->FindControl(BTNASSIGN_TITLE);

    if (t)
    {
        char s[160];
        char *nm = ControlTab_DescribeFunction(
            g_baTargetFunc); // #53: human-readable description, not the callback name

        if (not nm)
            nm = GetUserFunctionName(idx); // fallback if not found in the table

        sprintf(s, "Assign: %s", nm ? nm : "?");
        t->SetText(s);
    }
}

void ButtonAssignSearchCB(long, short, C_Base *control)
{
    C_EditBox *eb = (C_EditBox *)control;
    char *txt = eb->GetText();
    strncpy(g_baSearch, txt ? txt : "", sizeof(g_baSearch) - 1);
    g_baSearch[sizeof(g_baSearch) - 1] = 0;
    FillButtonAssignFuncList(control->Parent_);
}

void ButtonAssignDeviceCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_SELECT)
        return;

    C_ListBox *lb = (C_ListBox *)control;
    g_baDevice = lb->GetTextID() - 1;

    C_Window *win = control->Parent_;
    C_Text *lbl = (C_Text *)win->FindControl(BTNASSIGN_DEVICE_LABEL);

    if (lbl)
    {
        char *nm = lb->GetText();

        if (nm)
            lbl->SetText(nm);
    }

    FillButtonAssignButtonList(win);
    FillButtonAssignFuncList(win);
}

// #51 "Detect" button: arms autodetect for ONE press (for devices — a single press
// of an armed button; permanently-pressed stay silent). Pressing again re-arms (if it caught the wrong one).
// TODO #53 (new UI): keyboard — full mode (modifiers + two-key combos do not cancel
// detect); filter detection by the device of the clicked cell.
void ButtonAssignDetectCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    g_baPollInit =
        true; // the first frame captures the baseline from fresh IO.digital
    g_baActive =
        true; // arm a single capture (ButtonAssignAutodetectPoll clears g_baActive itself)

    C_Window *w = control ? (C_Window *)control->Parent_ :
                            gMainHandler->FindWindow(SETUP_BTNASSIGN_WIN);

    if (w)
    {
        C_Text *d = (C_Text *)w->FindControl(BTNASSIGN_DETECTED);

        if (d)
        {
            d->SetText("Press key or button to assign...");
            d->Refresh();
            w->RefreshWindow(); // clear any tail of a previous (longer) message
        }
    }
}

static void SetupButtonAssignWindow(C_Window *win)
{
    if (not win)
        return;

    C_Text *t;

    if ((t = (C_Text *)win->FindControl(BTNASSIGN_LBL_DEVICE)))
        t->SetText("Device:");

    if ((t = (C_Text *)win->FindControl(BTNASSIGN_LBL_BUTTON)))
        t->SetText("Button:");

    if ((t = (C_Text *)win->FindControl(BTNASSIGN_LBL_MOD)))
        t->SetText("Modifier:");

    if ((t = (C_Text *)win->FindControl(BTNASSIGN_LBL_SEARCH)))
        t->SetText("Search:");

    if ((t = (C_Text *)win->FindControl(BTNASSIGN_AUTODETECT)))
        t->SetText(
            "Hold the button/switch you want to assign"); // #15: hint for the new debounce (hold until capture)

    if ((t = (C_Text *)win->FindControl(BTNASSIGN_TITLE)))
    {
        char s[160];
        char *nm = ControlTab_DescribeFunction(
            g_baTargetFunc); // #53: human-readable description, not the callback name

        if (not nm)
            nm = g_baTargetFunc ? FindStringFromFunction(g_baTargetFunc) :
                                  NULL; // fallback

        sprintf(s, "Assign: %s", nm ? nm : "?");
        t->SetText(s);
    }

    C_ListBox *dev = (C_ListBox *)win->FindControl(BTNASSIGN_DEVICE_LIST);

    if (dev)
    {
        BuildControllerList(dev);
        dev->SetCallback(ButtonAssignDeviceCB);
    }

    C_ListBox *mod = (C_ListBox *)win->FindControl(BTNASSIGN_MODIFIER_LIST);

    if (mod)
    {
        mod->RemoveAllItems();

        for (int m = 0; m < 8; ++m)
            mod->AddItem(m + 1, C_TYPE_ITEM, (char *)g_baModNames[m]);

        mod->SetValue(g_baMod + 1);
        mod->SetCallback(ButtonAssignModCB);
        // Artscout - 2026: the dropdown sits 3px above the "Modifier:" label baseline in the
        // .scf layout; nudge it down to line up.
        mod->SetXY(mod->GetX(), mod->GetY() + 3);
        mod->Refresh();
    }

    C_ListBox *bl = (C_ListBox *)win->FindControl(BTNASSIGN_BUTTON_LIST);

    if (bl)
        bl->SetCallback(ButtonAssignButtonCB);

    C_ListBox *fl = (C_ListBox *)win->FindControl(BTNASSIGN_FUNC_LIST);

    if (fl)
        fl->SetCallback(ButtonAssignFuncCB);

    C_EditBox *eb = (C_EditBox *)win->FindControl(BTNASSIGN_SEARCH);

    if (eb)
    {
        eb->SetText("");
        eb->SetCallback(ButtonAssignSearchCB);
    }

    // OK / Cancel (BTNASSIGN_ASSIGN / BTNASSIGN_OPEN) — button text + callbacks
    C_Button *okb = (C_Button *)win->FindControl(BTNASSIGN_ASSIGN);

    if (okb)
    {
        okb->SetText(0, "OK");
        okb->SetCallback(ButtonAssignOkCB);
    }

    C_Button *cab = (C_Button *)win->FindControl(BTNASSIGN_OPEN);

    if (cab)
    {
        cab->SetText(0, "Cancel");
        cab->SetCallback(ButtonAssignCancelCB);
    }

    // #51 Detect button — arms autodetect for one press
    C_Button *detb = (C_Button *)win->FindControl(BTNASSIGN_DETECT);

    if (detb)
    {
        detb->SetText(0, "Redetect");
        detb->SetCallback(ButtonAssignDetectCB);
    }

    // #53 Clear button: stages "remove this function's binding on the selected device"
    C_Button *clrb = (C_Button *)win->FindControl(BTNASSIGN_CLEAR);

    if (clrb)
    {
        clrb->SetText(0, "Clear");
        clrb->SetCallback(ButtonAssignClearCB);
    }

    C_Text *lbl = (C_Text *)win->FindControl(BTNASSIGN_DEVICE_LABEL);

    if (lbl)
    {
        if (g_baDevice >= SIM_JOYSTICK1 and g_baDevice < SIM_NUMDEVICES and
            gDIDevNames[g_baDevice])
            lbl->SetText(gDIDevNames[g_baDevice]);
        else if (g_baDevice == SIM_KEYBOARD)
            lbl->SetText("Keyboard");
    }

    FillButtonAssignButtonList(win);
    FillButtonAssignFuncList(win);
}

void OpenButtonAssignWindow(InputFunctionType func, int cpId, long sourceCtrl)
{
    C_Window *win = gMainHandler->FindWindow(SETUP_BTNASSIGN_WIN);

    if (not win)
        return;

    g_baTargetFunc = func;
    g_baTargetCpId = cpId;
    g_baSourceCtrl = sourceCtrl;
    g_baSearch[0] = 0;
    g_baMod = 0;
    g_baStagedButton = -1;
    g_baStagedClear = false; // #53 fresh open: nothing staged to clear

    // #53 if opened from a specific device column cell, preselect that device so the
    // Clear/Assign actions target it; otherwise default to the first joystick (or keyboard).
    if (g_baOpenDevice >= SIM_JOYSTICK1 and g_baOpenDevice < SIM_NUMDEVICES)
        g_baDevice = g_baOpenDevice;
    else
        g_baDevice = (gTotalJoy > 0) ? SIM_JOYSTICK1 : SIM_KEYBOARD;

    g_baOpenDevice = -1; // consume the one-shot preselect

    // #51 auto-arm autodetect IMMEDIATELY on open (for ONE press — catch and stop, no "jumps").
    // The user need not press a button; the "Redetect" button restarts capture (if it caught the wrong one).
    g_baPollInit = true;
    g_baActive = true;

    SetupButtonAssignWindow(win);

    // prompt in the detected field (like everywhere: "press a key/button")
    {
        C_Text *d = (C_Text *)win->FindControl(BTNASSIGN_DETECTED);

        if (d)
            d->SetText("Press key or button to assign...");
    }

    gMainHandler->ShowWindow(win);
    gMainHandler->WindowToFront(win);
    g_baWindowOpen =
        true; // Artscout - 2026: modal — blocks leaving options until OK/Cancel
}


// #53 right-click context menu on a keyboard cell (replaces the per-row Clear button #52).
// The handler opens the cell's attached menu (KEYCTX_MENU) automatically on right-click
// (chandler WM_RBUTTONUP -> gPopupMgr->OpenMenu(control->GetMenu(), ...)). These statics
// remember which row/function the menu was opened for (captured in KeyCtxMenuOpenCB).
static InputFunctionType g_ctxFunc = NULL;
static int g_ctxBtnId = -1;
static long g_ctxCtrlId = 0;
static int g_ctxDevice = -1; // #53 device index if the cell is a device cell
static bool g_ctxIsKeyboard =
    false; // #53 true if the cell is the keyboard column

// Capture the right-clicked cell (function + which column) when the context menu opens.
void KeyCtxMenuOpenCB(C_Base *, C_Base *caller)
{
    if (not caller)
        return;

    g_ctxFunc = (InputFunctionType)caller->GetUserPtr(FUNCTION_PTR);
    g_ctxBtnId = caller->GetUserNumber(BUTTON_ID);
    g_ctxCtrlId = caller->GetID();
    g_ctxDevice = caller->GetUserNumber(DEVICE_IDX); // 0 for keyboard cells
    // keyboard cells use ids KEYCODES..KEYCODES+rows; device cells use DEVCELL_BASE..
    g_ctxIsKeyboard =
        (g_ctxCtrlId >= KEYCODES and g_ctxCtrlId < KEYCODES + 10000);
}

// Context menu "Assign..." -> keyboard cell: cyan key-capture (KeystrokeCB binds the next key);
// device cell: open the assign window targeting that device.
void KeyCtxAssignCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP and hittype not_eq C_TYPE_RMOUSEUP)
        return;

    if (not g_ctxFunc)
        return;

    if (g_ctxIsKeyboard)
    {
        C_Window *win = gMainHandler->FindWindow(SETUP_CONTROL_ADVANCED_WIN);
        C_Button *b = win ? (C_Button *)win->FindControl(g_ctxCtrlId) : NULL;

        if (b)
        {
            KeyVar.CurrControl = g_ctxCtrlId;
            KeyVar.EditKey =
                TRUE; // next keypress (KeystrokeCB) binds the combo
            b->SetFgColor(0, RGB(0, 255, 255));
            b->Refresh();
        }
    }
    else
    {
        g_baOpenDevice =
            g_ctxDevice; // assign on the device of the clicked cell
        OpenButtonAssignWindow(g_ctxFunc, g_ctxBtnId, g_ctxCtrlId);
    }
}

// Context menu "Clear" -> unbind this function from ALL device buttons (keyboard combo is
// left intact). The list is rebuilt next frame (deferred: clearing synchronously would
// delete the control we are inside -> UAF).
void KeyCtxClearCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP and hittype not_eq C_TYPE_RMOUSEUP)
        return;

    if (not g_ctxFunc)
        return;

    BOOL changed = FALSE;

    for (int b = 0; b < SIMLIB_MAX_DIGITAL * SIM_NUMDEVICES; ++b)
    {
        if (UserFunctionTable.GetButtonFunction(b, NULL) == g_ctxFunc)
        {
            UserFunctionTable.SetButtonFunction(b, NULL, -1);
            changed = TRUE;
        }
    }

    if (changed)
        KeyVar.Modified = TRUE;

    g_keyListNeedRebuild = true;
    g_keyListPreserveScroll =
        true; // a Clear shouldn't scroll the table back to the top
}

// #53 build (once) and register the keyboard-cell context menu in the popup manager.
// Style/font are taken from the KEYCODES template so the menu matches the list.
void EnsureKeyCtxMenu(C_Button *Keycodes)
{
    static bool s_built = false;

    if (s_built or not Keycodes or not gPopupMgr)
        return;

    if (gPopupMgr->GetMenu(
            KEYCTX_MENU)) // already present (e.g. reopened window)
    {
        s_built = true;
        return;
    }

    C_PopupList *menu = new C_PopupList;

    if (not menu)
        return;

    menu->Setup(KEYCTX_MENU, C_TYPE_NORMAL, gMainHandler, 0, 0);
    menu->SetFont(Keycodes->GetFont());
    menu->SetNormColor(RGB(230, 230, 230));
    menu->SetSelColor(RGB(0, 255, 0));
    menu->SetDisColor(RGB(102, 102, 102));
    menu->SetBgColor(RGB(0, 0, 0));
    menu->SetBarColor(RGB(65, 128, 173));
    menu->SetBorderColor(RGB(65, 128, 173));
    menu->SetOpaque(100);

    menu->AddItem(KEYCTX_ASSIGN, C_TYPE_ITEM, "Assign...", 0);
    menu->AddItem(KEYCTX_CLEAR, C_TYPE_ITEM, "Clear", 0);
    menu->SetCallback(KEYCTX_ASSIGN, KeyCtxAssignCB);
    menu->SetCallback(KEYCTX_CLEAR, KeyCtxClearCB);
    menu->SetOpenCallback(KeyCtxMenuOpenCB);

    gPopupMgr->AddMenu(menu);
    s_built = true;
}

void KeycodeCB(long ID, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    // #53 the keyboard cell uses the cyan key-capture (KeystrokeCB binds the next key combo).
    // Device buttons are assigned via the device cells / assign window, NOT here — the assign
    // window's autodetect is joystick-only and cannot capture keyboard keys.
    if (KeyVar.EditKey)
    {
        // restore the colour of the previously-edited cell
        C_Button *prev =
            (C_Button *)control->Parent_->FindControl(KeyVar.CurrControl);

        if (prev)
            SetButtonColor(prev);
    }

    // click the cell that is already being edited -> stop editing
    if (KeyVar.CurrControl == ID and KeyVar.EditKey)
    {
        KeyVar.EditKey = FALSE;
        return;
    }

    KeyVar.CurrControl = ID;

    if (control->GetUserNumber(EDITABLE) < 1)
    {
        KeyVar.EditKey = FALSE; // this row is not remappable
    }
    else
    {
        KeyVar.EditKey = TRUE; // next keypress (KeystrokeCB) binds the combo
        ((C_Button *)control)
            ->SetFgColor(0, RGB(0, 255, 255)); // cyan = "press a key"
        ((C_Button *)control)->Refresh();
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

// Artscout - 2026: table column geometry + device count, used by AddKeyMapLines below to clamp the
// row separators to the table's real right border. Full definitions live further down (identical
// macro redefinition is legal; the count is defined once here and shared).
#ifndef DEVCOL_X0
#define DEVCOL_X0 340
#endif
#ifndef DEVCOL_W
#define DEVCOL_W 200
#endif
static int g_tblDevCount = 0;

int AddKeyMapLines(C_Window *win, C_Line *Hline, C_Line *Vline, int count)
{
    int retval = TRUE;

    if (not win)
        return FALSE;

    // Artscout - 2026: clamp each row separator to the actual right border of the table
    // (rightmost column separator = DEVCOL_X0 + g_tblDevCount*DEVCOL_W - 12). The template Hline
    // spans the full client width, so without clamping the row lines run past the right border
    // into the empty area, looking like extra (phantom) cells.
    int hlRightX = DEVCOL_X0 + g_tblDevCount * DEVCOL_W - 12;
    int hlWidth = hlRightX - Hline->GetX();

    if (hlWidth < 1)
        hlWidth = Hline->GetW(); // safety: fall back to the template width

    C_Line *line;
    line = (C_Line *)win->FindControl(HLINE + count);

    if (not line)
    {
        line = new C_Line;

        if (line)
        {
            line->Setup(HLINE + count, Hline->GetType());
            line->SetColor(RGB(191, 191, 191));
            line->SetXYWH(Hline->GetX(), Hline->GetY() + Vline->GetH() * count,
                          hlWidth, Hline->GetH());
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
    else
    {
        // existing line kept across rebuilds: re-clamp its width (device count may have changed)
        line->SetWH(hlWidth, Hline->GetH());
        line->Refresh();
    }

    line = (C_Line *)win->FindControl(VLINE + count);

    if (not line)
    {
        line = new C_Line;

        if (line)
        {
            line->Setup(VLINE + count, Vline->GetType());
            line->SetColor(RGB(191, 191, 191));
            line->SetXYWH(Vline->GetX(), Vline->GetY() + Vline->GetH() * count,
                          Vline->GetW(), Vline->GetH());
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

// #22: g_keyFilter / g_keyDevFilter / g_keyListDisplayOnly and the forward declaration of
// UpdateKeyMapList are declared ABOVE (before KeystrokeCB) — needed there too (ESC search reset).

// Case-insensitive substring search (without depending on the platform strcasestr).
static bool ContainsNoCase(const char *hay, const char *needle)
{
    if (not needle or not needle[0])
        return true;

    if (not hay)
        return false;

    size_t nl = strlen(needle);

    for (const char *p = hay; *p; ++p)
    {
        size_t k = 0;

        while (k < nl and p[k] and
               tolower((unsigned char)p[k]) ==
                   tolower((unsigned char)needle[k]))
            ++k;

        if (k == nl)
            return true;
    }

    return false;
}

// #22: does a list row pass the search filter? We match the visible description AND the function name.
bool KeyListRowVisible(const char *funcName, const char *descrip)
{
    if (g_keyFilter[0] == 0)
        return true;

    return ContainsNoCase(descrip, g_keyFilter) or
           ContainsNoCase(funcName, g_keyFilter);
}

// #22: find the button number of the selected device (g_keyDevFilter) assigned to the function.
// Returns a 0-based button number, or -1 if the function is not assigned on this device.
static int FindDeviceButtonForFunc(InputFunctionType func)
{
    if (g_keyDevFilter < SIM_JOYSTICK1 or g_keyDevFilter >= SIM_NUMDEVICES or
        not func)
        return -1;

    int base = (g_keyDevFilter - SIM_JOYSTICK1) * SIMLIB_MAX_DIGITAL;
    int cnt = gDIDevButtons[g_keyDevFilter];

    if (cnt <= 0 or cnt > SIMLIB_MAX_DIGITAL)
        cnt = SIMLIB_MAX_DIGITAL;

    for (int b = 0; b < cnt; ++b)
    {
        int cp;

        if (UserFunctionTable.GetButtonFunction(base + b, &cp) == func)
            return b;
    }

    return -1;
}

// #53 controls table column geometry (client 2 of SETUP_CONTROL_ADVANCED_WIN).
// Column 0 = function description (MAPPING template X), column 1 = keyboard combo
// (KEYCODES template X), then one column per detected game device starting at DEVCOL_X0.
// #53 column layout (client-relative x). Sized so desc + keyboard + 3 device columns FIT
// the grey panel width WITHOUT horizontal scroll — ui95's horizontal client scroll mis-sets
// VX_ when ClientArea.left != 0, so we avoid h-overflow entirely (then VX_ = ClientArea.left).
#define TBL_LEFT 163 // = client 2 left (see SetClientArea in AdvancedControlCB)
#define TBL_HDR_Y 158 // absolute Y of the (separate, always-visible) header row
#define MAPCOL_X 3 // description column x (matches MAPPING template)
#define KEYCOL_X                                                               \
    190 // #53 keyboard column x (matches KEYCODES template; +40 = wider FUNCTION column)
#define DEVCOL_X0                                                              \
    340 // X of the first device column (full device names; table h-scrolls)
#define DEVCOL_W                                                               \
    200 // width/step of a device column (wide enough for full device names)
#define TBL_MAX_DEVCOLS                                                        \
    14 // SIM_NUMDEVICES - SIM_JOYSTICK1 (max device columns)
#define KEYCELL_W 135 // mouse-over/click width of the keyboard cell (150..285)

// #53 row height of the current table (= VLINE template height); used to size cell
// mouse-over hotspots uniformly for keyboard and device cells.
static int g_tblRowH = 19;

// #53 list of present game devices (joysticks/HOTAS) mapped to visible column indices.
// Rebuilt on every full list rebuild so columns track plugged/unplugged devices.
static int g_tblDevs[TBL_MAX_DEVCOLS];
// g_tblDevCount defined earlier (Artscout - 2026: moved up for AddKeyMapLines)

static void RebuildTableDeviceList()
{
    g_tblDevCount = 0;

    for (int dev = SIM_JOYSTICK1;
         dev < SIM_NUMDEVICES and g_tblDevCount < TBL_MAX_DEVCOLS; ++dev)
        if (gDIDevButtons[dev] > 0)
            g_tblDevs[g_tblDevCount++] = dev;
}

// #53 0-based button number on a SPECIFIC device assigned to func, or -1 if none.
static int FindDeviceButtonForFuncDev(InputFunctionType func, int dev)
{
    if (dev < SIM_JOYSTICK1 or dev >= SIM_NUMDEVICES or not func)
        return -1;

    int base = (dev - SIM_JOYSTICK1) * SIMLIB_MAX_DIGITAL;
    int cnt = gDIDevButtons[dev];

    if (cnt <= 0 or cnt > SIMLIB_MAX_DIGITAL)
        cnt = SIMLIB_MAX_DIGITAL;

    for (int b = 0; b < cnt; ++b)
    {
        int cp;

        if (UserFunctionTable.GetButtonFunction(base + b, &cp) == func)
            return b;
    }

    return -1;
}

// #53 guard: KeyDescrips is a [256] array (DIK scancodes), may be NULL or have empty
// slots. Any bad index (corrupt data) must NOT crash the options window (was a 0xFDFDFDFD crash).
static const char *SafeKeyDescrip(int idx)
{
    if (KeyDescrips and idx >= 0 and idx < 256 and KeyDescrips[idx])
        return KeyDescrips[idx];

    return "";
}

void UpdateKeyMapButton(C_Button *button, KeyMap &Map, int count)
{
    if (not button) // #53 template missing -> never deref (was a 0x0 crash)
        return;

    int flags = Map.mod2 + (Map.key1 << SECOND_KEY_SHIFT) +
                (Map.mod1 << SECOND_KEY_MOD_SHIFT);

    button->SetMenu(KEYCTX_MENU); // #53 right-click context menu (Assign/Clear)
    // #53 uniform mouse-over highlight: a fixed hotspot the size of the keyboard cell
    // (a text button has no image, so HighLite only draws with a FIXED hotspot).
    button->SetFixedHotSpot(1);
    button->SetHotSpot(-3, 0, KEYCELL_W, g_tblRowH);
    button->SetMouseOverColor(RGB(255, 255, 255));
    button->SetMouseOverPerc(35);
    button->SetUserNumber(KEY2, Map.key2);
    button->SetUserNumber(FLAGS, flags);
    button->SetUserNumber(BUTTON_ID, Map.buttonId);
    button->SetUserNumber(MOUSE_SIDE, Map.mouseSide);
    button->SetUserNumber(EDITABLE, Map.editable);
    button->SetUserPtr(FUNCTION_PTR, (void *)Map.func);

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
            _stprintf(totalDescrip, "%s%s : %s%s", firstMod,
                      SafeKeyDescrip(Map.key1), secondMod,
                      SafeKeyDescrip(Map.key2));
        }
        else
        {
            DoShiftStates(totalDescrip, Map.mod2);
            strcat(totalDescrip, SafeKeyDescrip(Map.key2));
        }

        UserFunctionTable.SetControl(
            Map.key2, flags, KEYCODES + count); //define this as KEYCODES
        button->SetText(0, totalDescrip);
    }

    // #22: device-filter mode — in the left column show the button of the selected
    // joystick assigned to this function (or empty). Keyboard bindings (UserNumber/
    // SetControl above) stay untouched; only the displayed text changes.
    if (g_keyDevFilter >= SIM_JOYSTICK1)
    {
        int b = FindDeviceButtonForFunc(Map.func);

        if (b >= 0)
        {
            char s[32];
            sprintf(s, "Btn %d", b + 1);
            button->SetText(0, s);
        }
        else
        {
            button->SetText(0, "");
        }
    }

    SetButtonColor(button);
}

int UpdateKeyMap(C_Window *win, C_Button *Keycodes, int height, KeyMap &Map,
                 HotSpotStruct HotSpot, int count)
{
    C_Button *button;
    long flags;

    flags = Map.mod2 + (Map.key1 << SECOND_KEY_SHIFT) +
            (Map.mod1 << SECOND_KEY_MOD_SHIFT);

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
            button->Setup(KEYCODES + count, Keycodes->GetType(),
                          Keycodes->GetX(), Keycodes->GetY() + height * count);

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

// #52 geometry of the "Clear" button (list client coords, CLIENT 3, client width ~288;
// slider outside at abs 822). Easy to nudge here after a visual layout check.
#define CLEAR_BTN_X 250
#define CLEAR_BTN_W 34

// #52 "Clear" for a list row: remove the device BUTTON binding of the function (across ALL
// devices). The keyboard combo is NOT touched (only the button, as the user asked). Then
// rebuild the list (displayOnly: keep unsaved assignments, refresh the left column).
void ClearKeyCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP or not control)
        return;

    InputFunctionType func =
        (InputFunctionType)control->GetUserPtr(FUNCTION_PTR);

    if (not func)
        return;

    BOOL changed = FALSE;

    for (int b = 0; b < SIMLIB_MAX_DIGITAL * SIM_NUMDEVICES; ++b)
    {
        if (UserFunctionTable.GetButtonFunction(b, NULL) == func)
        {
            UserFunctionTable.SetButtonFunction(b, NULL, -1);
            changed = TRUE;
        }
    }

    if (changed)
        KeyVar.Modified = TRUE;

    // #52 do NOT rebuild synchronously here (we would delete ourselves -> UAF). Defer to the next frame.
    g_keyListNeedRebuild = true;
}

// #52 deferred key-list rebuild (called from RefreshJoystickCB, not from a button callback).
void RebuildKeyListDeferred(void)
{
    g_keyListDisplayOnly = true;
    UpdateKeyMapList(PlayerOptions.GetKeyfile(), TRUE);
    g_keyListDisplayOnly = false;
}

// #52 per-row "Clear" button at index count (CLEARBTN+count) — created in CODE (colored, like
// KEYCODES). If it already exists — only update the bound function. Style/client taken from
// the KEYCODES template button; position — right edge of the client (CLEAR_BTN_X), Y per row.
void UpdateClearButton(C_Window *win, C_Button *Keycodes, C_Text *Mapping,
                       int height, InputFunctionType func, int count)
{
    C_Button *btn = (C_Button *)win->FindControl(CLEARBTN + count);

    if (btn)
    {
        btn->SetUserPtr(FUNCTION_PTR, (void *)func);
        btn->Refresh();
        return;
    }

    btn = new C_Button;

    if (not btn)
        return;

    btn->Setup(CLEARBTN + count, C_TYPE_NORMAL, 0, 0);
    btn->SetClient(Keycodes->GetClient());
    btn->SetGroup(Keycodes->GetGroup());
    btn->SetCluster(Keycodes->GetCluster());
    btn->SetFont(Keycodes->GetFont());
    btn->SetColor(C_STATE_0, RGB(255, 90, 90)); // up — reddish
    btn->SetColor(C_STATE_1, RGB(230, 230, 230)); // down
    btn->SetColor(C_STATE_DISABLED, RGB(102, 102, 102));
    btn->SetXYWH(CLEAR_BTN_X, Mapping->GetY() + height * count, CLEAR_BTN_W,
                 height - 4);
    btn->SetText(0, "Clr");
    btn->SetUserPtr(FUNCTION_PTR, (void *)func);
    btn->SetCallback(ClearKeyCB);
    btn->SetFlagBitOn(C_BIT_ENABLED);

    win->AddControl(btn);
    btn->Refresh();
}

int UpdateMappingDescrip(C_Window *win, C_Text *Mapping, int height,
                         _TCHAR *descrip, int count)
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

// #53 click on a device cell -> open the button-assignment window preselecting that device,
// so the user can re-assign or Clear that device's binding (Clear committed on OK).
void DeviceCellCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP or not control)
        return;

    InputFunctionType func =
        (InputFunctionType)control->GetUserPtr(FUNCTION_PTR);
    int dev = control->GetUserNumber(DEVICE_IDX);

    if (not func)
        return;

    g_baOpenDevice =
        dev; // one-shot preselect consumed by OpenButtonAssignWindow
    OpenButtonAssignWindow(func, 0, 0);
}

// #53 per-device columns for one table row: for every present device show the assigned
// button ("Btn N") in that device's column, or leave the cell empty. Cells are clickable
// buttons cloned from the KEYCODES template (same client/group/cluster/flags) so they
// scroll with the table; a click opens the assign window targeting that device.
void UpdateDeviceCells(C_Window *win, C_Button *Keycodes, C_Line *Vline,
                       KeyMap &Map, int count)
{
    if (not win or not Keycodes or not Vline)
        return;

    int rowH = Vline->GetH();
    int y = Keycodes->GetY() + rowH * count;

    for (int vis = 0; vis < g_tblDevCount; ++vis)
    {
        long id = DEVCELL_BASE + vis * 1000 + count;
        int b = FindDeviceButtonForFuncDev(Map.func, g_tblDevs[vis]);

        char s[32];

        if (b >= 0)
            sprintf(s, "Btn %d", b + 1);
        else
            s[0] = 0;

        C_Button *cell = (C_Button *)win->FindControl(id);

        if (cell)
        {
            cell->Refresh();
            // #53 keep reused cells uniformly bright green (color is otherwise only set on creation)
            cell->SetColor(C_STATE_0, RGB(0, 255, 0));
            cell->SetColor(C_STATE_1, RGB(0, 255, 0));
            cell->SetColor(C_STATE_DISABLED, RGB(0, 255, 0));
            cell->SetText(0, s);
            cell->SetUserPtr(FUNCTION_PTR, (void *)Map.func);
            cell->SetUserNumber(DEVICE_IDX, g_tblDevs[vis]);
            cell->Refresh();
            continue;
        }

        // #53 always create the cell (even empty) so every device column is clickable
        // (right-click menu + mouse-over highlight + click-to-assign on that device).
        cell = new C_Button;

        if (not cell)
            continue;

        cell->Setup(id, C_TYPE_NORMAL, DEVCOL_X0 + vis * DEVCOL_W, y);
        cell->SetClient(Keycodes->GetClient());
        cell->SetGroup(Keycodes->GetGroup());
        cell->SetCluster(Keycodes->GetCluster());
        cell->SetFont(Keycodes->GetFont());
        cell->SetFlags(Keycodes->GetFlags());
        // #53 device cell text: uniform bright green in every state
        cell->SetColor(C_STATE_0, RGB(0, 255, 0));
        cell->SetColor(C_STATE_1, RGB(0, 255, 0));
        cell->SetColor(C_STATE_DISABLED, RGB(0, 255, 0));
        cell->SetText(0, s);
        cell->SetUserPtr(FUNCTION_PTR, (void *)Map.func);
        cell->SetUserNumber(DEVICE_IDX, g_tblDevs[vis]);
        cell->SetCallback(DeviceCellCB);
        cell->SetMenu(
            KEYCTX_MENU); // #53 same right-click menu as keyboard cells
        cell->SetFlagBitOn(C_BIT_ENABLED);
        // #53 uniform mouse-over highlight (fixed hotspot the size of the device cell)
        cell->SetFixedHotSpot(1);
        cell->SetHotSpot(-2, 0, DEVCOL_W - 14, g_tblRowH);
        cell->SetMouseOverColor(RGB(255, 255, 255));
        cell->SetMouseOverPerc(35);

        win->AddControl(cell);
        cell->Refresh();
    }
}

// #53 create/update one header label INSIDE client 2 (client-relative x, y=2 = top row above
// the data rows at y=22+). In-client so the header scrolls WITH the columns: widths stay in
// sync and it can't drift off-bounds on horizontal scroll. clientX matches the column x.
static void SetTableHeader(C_Window *win, long id, int clientX, long client,
                           long cluster, long font, const char *txt)
{
    C_Text *t = (C_Text *)win->FindControl(id);

    if (not t)
    {
        t = new C_Text;

        if (not t)
            return;

        t->Setup(id, C_TYPE_LEFT);
        t->SetClient(client);
        t->SetCluster(cluster);
        t->SetFont(font);
        t->SetFlagBitOn(C_BIT_LEFT);
        t->SetXY(clientX, 2);
        win->AddControl(t);
    }
    else
    {
        t->SetXY(clientX, 2);
    }

    t->SetFGColor(RGB(0, 255, 0)); // #53 all table headers: bright green

    t->SetText((char *)txt);
    t->Refresh();
}

// #53 table header (top row of client 2, scrolls with the columns): "FUNCTION" | "KEYBOARD"
// | one column per detected device (full product name). Same column x as the cells, so the
// header and the data columns always line up (incl. under horizontal scroll).
void BuildTableHeader(C_Window *win, C_Button *Keycodes, C_Text *Mapping)
{
    if (not win or not Keycodes or not Mapping)
        return;

    long client = Mapping->GetClient();
    long cluster = Mapping->GetCluster();
    long font = Mapping->GetFont();

    SetTableHeader(win, TBLHDR_FUNC, MAPCOL_X, client, cluster, font,
                   "FUNCTION");
    SetTableHeader(win, TBLHDR_KEY, KEYCOL_X, client, cluster, font,
                   "KEYBOARD");

    // device headers: truncated text + full name as a hover tooltip (help text). A C_Button
    // is used (not C_Text) because only C_Control supports SetHelpText; the handler shows the
    // tooltip near the cursor (CheckHelpText) and it does not intercept clicks.
    static char s_devHelpName[TBL_MAX_DEVCOLS][80] = {{0}};
    static long s_devHelpId[TBL_MAX_DEVCOLS] = {0};

    for (int vis = 0; vis < g_tblDevCount; ++vis)
    {
        int dev = g_tblDevs[vis];
        const char *dn = (dev >= SIM_JOYSTICK1 and dev < SIM_NUMDEVICES and
                          gDIDevNames[dev]) ?
                             gDIDevNames[dev] :
                             "Device";

        // register the full name as a string ONCE per (column, name) -> tooltip id
        if (strncmp(s_devHelpName[vis], dn, sizeof(s_devHelpName[vis]) - 1) !=
            0)
        {
            strncpy(s_devHelpName[vis], dn, sizeof(s_devHelpName[vis]) - 1);
            s_devHelpName[vis][sizeof(s_devHelpName[vis]) - 1] = 0;
            s_devHelpId[vis] = gStringMgr->AddText(dn);
        }

        char trunc[32];
        sprintf(trunc, "%.27s", dn); // truncated to the column width

        long id = DEVHDR_BASE + vis;
        int x = DEVCOL_X0 + vis * DEVCOL_W;
        C_Button *hb = (C_Button *)win->FindControl(id);

        if (not hb)
        {
            hb = new C_Button;

            if (hb)
            {
                hb->Setup(id, C_TYPE_NORMAL, x, 2);
                hb->SetClient(client);
                hb->SetCluster(cluster);
                hb->SetFont(font);
                hb->SetFlagBitOn(C_BIT_LEFT);
                hb->SetFlagBitOn(C_BIT_ENABLED);
                win->AddControl(hb);
            }
        }
        else
        {
            hb->SetXY(x, 2);
        }

        if (hb)
        {
            hb->SetColor(
                C_STATE_0,
                RGB(0, 255,
                    0)); // #53 device headers: bright green (bold via the MAPPING font)
            hb->SetText(0, trunc);
            hb->SetHelpText(s_devHelpId[vis]); // full name on hover
            hb->Refresh();
        }
    }

    // remove stale device headers if the device count dropped
    for (int vis = g_tblDevCount; vis < TBL_MAX_DEVCOLS; ++vis)
        if (win->FindControl(DEVHDR_BASE + vis))
            win->RemoveControl(DEVHDR_BASE + vis);
}

// #53 full-height vertical separators between columns (client 2), so device columns are
// visually divided like the keyboard column. Called after the list is (re)built so the
// height matches the row count (header row at client-y 2, data rows start at 22).
void UpdateColumnSeparators(C_Window *win, C_Button *Keycodes, C_Line *Vline,
                            int rowCount)
{
    if (not win or not Keycodes or not Vline)
        return;

    int rowH = Vline->GetH();
    int top = 2;
    int h = 20 + rowH * rowCount; // header band + all data rows

    // column 0 = boundary before the keyboard column (matches the per-row VLINE x),
    // columns 1..N = boundary before each device column, column N+1 = right border of the table.
    for (int col = 0; col < TBL_MAX_DEVCOLS + 1; ++col)
    {
        long id = DEVSEP_BASE + col;
        bool wanted = (col == 0) ? true : (col - 1 <= g_tblDevCount);

        if (wanted)
        {
            int x = (col == 0) ? Vline->GetX() :
                                 (DEVCOL_X0 + (col - 1) * DEVCOL_W - 12);

            C_Line *ln = (C_Line *)win->FindControl(id);

            if (not ln)
            {
                ln = new C_Line;

                if (not ln)
                    continue;

                ln->Setup(id, Vline->GetType());
                ln->SetClient(Vline->GetClient());
                ln->SetGroup(Vline->GetGroup());
                ln->SetCluster(Vline->GetCluster());
                ln->SetFlags(Vline->GetFlags());
                ln->SetColor(RGB(191, 191, 191));
                win->AddControl(ln);
            }

            ln->SetXYWH(x, top, 1, h);
            ln->Refresh();
        }
        else if (win->FindControl(id))
        {
            win->RemoveControl(id);
        }
    }

    // Artscout - 2026: header/data separator (the line just under the header row). It used to be a
    // static 1100px-wide [LINE] in the .scf with no id, so it always ran past the last device column
    // (the "stray over-long line / phantom cell under the header"). It is now code-owned (id
    // TBLHDR_SEP) and clamped to the real table right border, exactly like the column separators.
    int rightX = DEVCOL_X0 + g_tblDevCount * DEVCOL_W - 12;
    C_Line *hsep = (C_Line *)win->FindControl(TBLHDR_SEP);

    if (not hsep)
    {
        hsep = new C_Line;

        if (hsep)
        {
            hsep->Setup(TBLHDR_SEP, Vline->GetType());
            hsep->SetClient(Vline->GetClient()); // table client (2)
            hsep->SetCluster(Vline->GetCluster());
            hsep->SetFlags(Vline->GetFlags());
            hsep->SetColor(RGB(191, 191, 191));
            win->AddControl(hsep);
        }
    }

    if (hsep)
    {
        // y=20: between the header row (client-y 2) and the first data row (y=22)
        hsep->SetXYWH(1, 20, rightX - 1, 1);
        hsep->Refresh();
    }
}

int SetHdrStatusLine(C_Window *win, C_Button *Keycodes, C_Line *Vline,
                     KeyMap &Map, HotSpotStruct HotSpot, int count)
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
            client = win->GetClientArea(
                Vline->GetClient()); // #53 table client (was hardcoded 3)

            line->Setup(KEYCODES - count, 0);
            //line->SetXYWH( Keycodes->GetX() + HotX,
            // Keycodes->GetY() + Vline->GetH()*count + HotY,
            // HotW,HotH);
            line->SetXYWH(Keycodes->GetX() + HotSpot.X,
                          Keycodes->GetY() + Vline->GetH() * count + HotSpot.Y,
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


// #20: device GUID for a joystick buttonId (joyIndex = buttonId/128). Writes a 32-character
// hex into out and returns true if the device for this id is known (GUID non-zero).
// Lets buttonId be remapped on the next start by the stable GUID (like axes #19).
static bool FormatButtonDeviceGUID(int buttonId, char *out)
{
    int dev = SIM_JOYSTICK1 + (buttonId / SIMLIB_MAX_DIGITAL);

    if (dev < SIM_JOYSTICK1 or dev >= SIM_NUMDEVICES)
        return false;

    static const GUID zero = {0};
    const GUID &g = gDIDevGUIDs[dev];

    if (memcmp(&g, &zero, sizeof(GUID)) == 0)
        return false;

    const unsigned char *b = (const unsigned char *)&g;

    for (int i = 0; i < (int)sizeof(GUID); ++i)
        sprintf(out + i * 2, "%02X", b[i]);

    return true;
}

BOOL SaveKeyMapList(char *filename)
{
    if (not KeyVar.Modified)
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

    win = gMainHandler->FindWindow(
        SETUP_CONTROL_ADVANCED_WIN); // #53 button list is in the new window

    if (not win)
        return FALSE;

    // #22: the keyboard section is written from the VISIBLE list rows. If a search/device
    // filter is active, some keyboard rows are hidden and would fall out of the file. So we
    // clear the filter and rebuild the FULL list in display-only mode (buttonTable in
    // memory is untouched — button assignments are saved below from buttonTable as usual).
    if (g_keyFilter[0] or g_keyDevFilter >= SIM_JOYSTICK1)
    {
        g_keyFilter[0] = 0;
        g_keyDevFilter = -1;
        g_keyListDisplayOnly = true;
        UpdateKeyMapList(filename, TRUE);
        g_keyListDisplayOnly = false;
    }

    // #53: save into the active profile's XML (keyboard.xml + <GUID>.xml), NOT into keystrokes.key.
    // --- keyboard: from the visible list rows (only real combos, KEY2>=0) ---
    static CxKbBind kbArr[1200];
    int nkb = 0;

    button = (C_Button *)win->FindControl(KEYCODES);
    count = 0;

    while (button and nkb < 1200)
    {
        flags = button->GetUserNumber(FLAGS);
        mod2 = flags bitand MOD2_MASK;
        key1 = (flags bitand KEY1_MASK) >> SECOND_KEY_SHIFT;
        mod1 = (flags bitand MOD1_MASK) >> SECOND_KEY_MOD_SHIFT;

        if (key1 == 0xff)
        {
            key1 = -1;
            mod1 = 0;
        }

        int k2 = button->GetUserNumber(KEY2);
        theFunc = (InputFunctionType)button->GetUserPtr(FUNCTION_PTR);
        funcDescrip = theFunc ? FindStringFromFunction(theFunc) : NULL;

        if (funcDescrip and k2 >= 0)
        {
            strncpy(kbArr[nkb].func, funcDescrip, sizeof(kbArr[nkb].func) - 1);
            kbArr[nkb].func[sizeof(kbArr[nkb].func) - 1] = 0;
            kbArr[nkb].k2 = k2;
            kbArr[nkb].m2 = mod2;
            kbArr[nkb].k1 = key1;
            kbArr[nkb].m1 = mod1;
            kbArr[nkb].cpbtn = button->GetUserNumber(BUTTON_ID);
            kbArr[nkb].mouse = button->GetUserNumber(MOUSE_SIDE);
            kbArr[nkb].editable = button->GetUserNumber(EDITABLE);
            nkb++;
        }

        count++;
        button = (C_Button *)win->FindControl(KEYCODES + count);
    }

    // #71: PRESERVE multi-bind-per-function. The DCS-style table shows ONE editable row per function,
    // so functions that carry SEVERAL keyboard binds -- the radio comms-menu stepper (OTWRadioMenuStep/
    // StepBack) has chord variants Q->Q with m1=0 AND m1=1 for the menu-active key combo -- would lose
    // every bind except the one displayed, and the AWACS/Tower menu stopped paging (repeat-Q closed it).
    // These chord/system binds are flagged editable==-2 (non-editable). Re-append every editable==-2 bind
    // from the active profile that a UI row did not already produce, so saving never collapses them.
    {
        static CxKbBind oldKb[1200];
        int oldN = ControlsXml_ReadKeyboard(oldKb, 1200);

        for (int oi = 0; oi < oldN and nkb < 1200; oi++)
        {
            if (oldKb[oi].editable != -2)
                continue; // user-editable binds are authoritative from the UI rows above

            bool dup = false;

            for (int j = 0; j < nkb; j++)
                if (strcmp(kbArr[j].func, oldKb[oi].func) == 0 and
                    kbArr[j].k2 == oldKb[oi].k2 and
                    kbArr[j].m2 == oldKb[oi].m2 and
                    kbArr[j].k1 == oldKb[oi].k1 and kbArr[j].m1 == oldKb[oi].m1)
                {
                    dup = true;
                    break;
                }

            if (not dup)
                kbArr[nkb++] = oldKb[oi];
        }
    }

    ControlsXml_WriteKeyboard(kbArr, nkb);

    // --- device buttons: from buttonTable, one <GUID>.xml file per device ---
    // (if there was no file but a binding appeared — it is created; if no bindings — it is removed).
    for (int dev = SIM_JOYSTICK1; dev < SIM_NUMDEVICES; dev++)
    {
        int base = (dev - SIM_JOYSTICK1) * SIMLIB_MAX_DIGITAL;
        char guidStr[2 * sizeof(GUID) + 1];

        if (not FormatButtonDeviceGUID(base, guidStr))
            continue; // device not present / has no GUID

        static CxBtnBind devArr[SIMLIB_MAX_DIGITAL];
        int nd = 0;

        for (int b = 0; b < SIMLIB_MAX_DIGITAL and nd < SIMLIB_MAX_DIGITAL; b++)
        {
            int cp;
            theFunc = UserFunctionTable.GetButtonFunction(base + b, &cp);

            if (not theFunc)
                continue;

            funcDescrip = FindStringFromFunction(theFunc);

            if (not funcDescrip)
                continue;

            strncpy(devArr[nd].func, funcDescrip, sizeof(devArr[nd].func) - 1);
            devArr[nd].func[sizeof(devArr[nd].func) - 1] = 0;
            devArr[nd].id = base + b;
            devArr[nd].cpbtn = cp;
            devArr[nd].dir = 0;
            devArr[nd].isPov = 0;
            nd++;
        }

        ControlsXml_WriteDevice(guidStr, devArr, nd);
    }

    return TRUE;
}

void SaveKeyCB(long, short hittype, C_Base *control)
{
    C_Window *win;
    C_EditBox *ebox;
    _TCHAR fname[MAX_PATH];

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    win = gMainHandler->FindWindow(SAVE_WIN);

    if (not win)
        return;

    gMainHandler->HideWindow(win);
    gMainHandler->HideWindow(control->Parent_);

    ebox = (C_EditBox *)win->FindControl(FILE_NAME);

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
    C_EditBox *ebox;
    _TCHAR fname[MAX_PATH];
    FILE *fp;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    ebox = (C_EditBox *)control->Parent_->FindControl(FILE_NAME);

    if (ebox)
    {
        //dpc EmptyFilenameSaveFix, modified by MN - added a warning to enter a filename
        if (g_bEmptyFilenameFix)
        {
            if (_tcslen(ebox->GetText()) == 0)
            {
                AreYouSure(TXT_WARNING, TXT_ENTER_FILENAME, CloseWindowCB,
                           CloseWindowCB);
                return;
            }
        }

        //end EmptyFilenameSaveFix
        _stprintf(fname, "config/%s.key", ebox->GetText());
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
    SaveAFile(TXT_SAVE_KEYBOARD, "config/*.KEY", NULL, VerifySaveKeyCB,
              CloseWindowCB, "");
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

    // #52 this row's old Clear button (kept for cleanup of stale controls)
    button = (C_Button *)win->FindControl(CLEARBTN + count);

    if (button)
    {
        win->RemoveControl(CLEARBTN + count);
        retval = TRUE;
    }

    // #53 this row's per-device cells (iterate all possible columns, not just current count,
    // so cells survive device list changes are still removed)
    for (int vis = 0; vis < TBL_MAX_DEVCOLS; ++vis)
    {
        long id = DEVCELL_BASE + vis * 1000 + count;

        if (win->FindControl(id))
        {
            win->RemoveControl(id);
            retval = TRUE;
        }
    }

    //need to remove them if they exist
    return retval;
}


int UpdateKeyMapList(char *fname, int flag)
{
    FILE *fp;

    char filename[_MAX_PATH];
    C_Window *win;


    win = gMainHandler->FindWindow(
        SETUP_CONTROL_ADVANCED_WIN); // #53 button list is in the new window (CONTROLS SETUP tab)

    if (not win)
    {
        KeyVar.NeedUpdate = TRUE;
        return FALSE;
    }

    if (flag)
        sprintf(filename, "%s/config/%s.key", FalconDataDirectory, fname);
    else
        sprintf(filename, "%s/config/keystrokes.key", FalconDataDirectory);

    // #53: the list is built from the catalog+profile (XML); keystrokes.key is not opened.
    // In "display-only" mode (rebuild for a filter) we do NOT re-read the function table —
    // otherwise ClearTable+LoadFunctionTables would wipe unsaved assignments (kept in memory until Apply).
    if (not g_keyListDisplayOnly)
    {
        UserFunctionTable.ClearTable();
        LoadFunctionTables(fname);
    }

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

    Keycodes = button =
        (C_Button *)win->FindControl(KEYCODES); //define this as KEYCODES
    Mapping = text =
        (C_Text *)win->FindControl(MAPPING); //define this as MAPPING
    Hline = (C_Line *)win->FindControl(HLINE); //define this as HLINE
    Vline = (C_Line *)win->FindControl(VLINE); //define this as VLINE

    // #53 if the .scf templates are missing, bail out instead of crashing later
    if (not Keycodes or not Mapping or not Hline or not Vline)
    {
        SetCursor(gCursors[CRSR_F16]);
        return FALSE;
    }

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

    // #53: only a real reload (ClearTable+LoadFunctionTables above) resets the dirty flag. A
    // display-only rebuild (filter/search refresh, or the deferred rebuild after an assignment)
    // must NOT clear it — otherwise unsaved bindings live in memory but SaveKeyMapList early-outs
    // on (!Modified) and never writes them to disk.
    if (not g_keyListDisplayOnly)
        KeyVar.Modified = FALSE;

    NumUndispKeys = 0;

    SetCursor(gCursors[CRSR_WAIT]);

    // #53: the list is built from the catalog (controls.xml — ALL functions, incl. CMS) plus
    // the keyboard combos of the active profile (keyboard.xml). Device bindings are shown as
    // dedicated per-device columns (see UpdateDeviceCells).
    static CxKbBind s_kb[1200];
    int s_nkb = ControlsXml_ReadKeyboard(s_kb, 1200);

    // #53 refresh the device-column list and the header row before (re)building the rows.
    if (Vline)
        g_tblRowH = Vline->GetH(); // #53 row height for uniform cell hotspots
    RebuildTableDeviceList();
    // #53 row separators span the actual table width (desc..last device column right edge).
    // Artscout - 2026: end the template Hline (the row-0 separator, just under the header) exactly
    // at the right border (DEVCOL_X0 + count*DEVCOL_W - 12), same as the clamped per-row lines in
    // AddKeyMapLines. Without the -12 (and the GetX() offset) it ran past the border, leaving a
    // single stray line under the header and a phantom cell corner on the right.
    if (Hline)
        Hline->SetXYWH(Hline->GetX(), Hline->GetY(),
                       (DEVCOL_X0 + g_tblDevCount * DEVCOL_W - 12) -
                           Hline->GetX(),
                       Hline->GetH());
    BuildTableHeader(win, Keycodes, Mapping);
    EnsureKeyCtxMenu(Keycodes);

    int totalFuncs = GetUserFunctionCount();

    for (int fi = 0; fi < totalFuncs; fi++)
    {
        theFunc = GetUserFunctionByIndex(fi);
        const char *nm = GetUserFunctionName(fi);

        if (not theFunc or not nm or not nm[0])
            continue;

        // label (BMS from controls.xml, otherwise the raw name), truncated to column width
        const char *lbl = ControlsXml_GetLabel(nm);

        if (not lbl or not lbl[0])
            lbl = nm;

        strncpy(descrip, lbl, sizeof(descrip) - 1);
        descrip[sizeof(descrip) - 1] = 0;
        parsed = descrip;
        parsed[40] =
            0; // #53 wider description column (no Clear button anymore)

        // #22: search filter (matches both label and function name)
        if (not KeyListRowVisible(nm, parsed))
            continue;

        KeyMap Map;
        Map.func = theFunc;
        Map.buttonId = -1;
        Map.mouseSide = 0;
        Map.editable = 1;
        Map.key1 = 0;
        Map.mod1 = 0;
        Map.key2 = -1; // default = "not assigned"
        Map.mod2 = 0;
        strncpy(Map.descrip, parsed, sizeof(Map.descrip) - 1);
        Map.descrip[sizeof(Map.descrip) - 1] = 0;

        // keyboard combo from keyboard.xml (if any)
        for (int ki = 0; ki < s_nkb; ki++)
            if (strcmp(s_kb[ki].func, nm) == 0)
            {
                Map.buttonId = s_kb[ki].cpbtn;
                Map.mouseSide = s_kb[ki].mouse;
                Map.editable = s_kb[ki].editable;
                Map.key2 = s_kb[ki].k2;
                Map.mod2 = s_kb[ki].m2;
                Map.key1 = s_kb[ki].k1;
                Map.mod1 = s_kb[ki].m1;
                break;
            }

        if (not count)
        {
            //first time through .. special case
            UpdateKeyMapButton(button, Map, count);

            Mapping->Refresh();
            Mapping->SetText(parsed);
            Mapping->Refresh();
        }
        else
        {
            // #53 SetHdrStatusLine (old per-row blue 65,128,173 highlight bar) removed —
            // it filled the new table rows blue; the table doesn't use header status bars.
            UpdateKeyMap(win, Keycodes, Vline->GetH(), Map, HotSpot, count);
            UpdateMappingDescrip(win, Mapping, Vline->GetH(), parsed, count);
            AddKeyMapLines(win, Hline, Vline, count);
        }

        // #53 per-device button columns for this row (replaces the #52 Clear button;
        // clearing is now the right-click action on the keyboard cell, see KeycodeCB)
        UpdateDeviceCells(win, Keycodes, Vline, Map, count);

        count++;

        if (count > 10000)
            break;
    }

    NumDispKeys = count;

    // #22: with zero matches (the search filter found nothing) do NOT delete the template
    // KEYCODES/MAPPING row (it comes from .scf; deleting it would break the next rebuild).
    // We clear its text and start cleanup from index 1.
    int cleanupStart = count;

    if (count == 0)
    {
        C_Button *tmplBtn = (C_Button *)win->FindControl(KEYCODES);
        C_Text *tmplTxt = (C_Text *)win->FindControl(MAPPING);

        if (tmplBtn)
            tmplBtn->SetText(0, "");
        if (tmplTxt)
            tmplTxt->SetText("");

        cleanupStart = 1;
    }

    while (RemoveExcessControls(win, cleanupStart++))
        ;

    // #53 column separators sized to the final row count
    UpdateColumnSeparators(win, Keycodes, Vline, count);

    // #50 After a FULL list rebuild (window open / search filter / ESC reset / load /
    // default) recompute the scrollbar for the new row count and scroll to the top. Without this
    // the slider does not appear (regression) and "disappears" when filtering (previously AdjustScrollbar
    // was called ONLY when editing a single key — 2659/2766, but not on a rebuild).
    if (Keycodes)
    {
        long kc = Keycodes->GetClient();
        // #53 scroll the client to its origin (top-left) on rebuild so the header row is
        // visible. SetClientArea(left,top,w,h) resets VX_=left, VY_=top (ScanClientArea does
        // NOT reset them when content overflows — it only clamps the far edge).
        // Artscout - 2026: a context-menu "Clear" leaves the row count unchanged, so the user
        // expects to stay where they were scrolled. Skip the origin reset in that case — VY_
        // stays valid and ScanClientArea below only re-clamps/re-syncs the slider.
        if (not g_keyListPreserveScroll)
        {
            UI95_RECT ca = win->GetClientArea(kc);
            win->SetClientArea(ca.left, ca.top, ca.right - ca.left,
                               ca.bottom - ca.top, kc);
        }
        // ScanClientArea recomputes the virtual height from the actual rows AND shows/
        // hides VScroll (cwindow.cpp:446/452). Without it the slider does not appear after a rebuild
        // (regression), and since it is INVISIBLE the wheel over the list does not scroll either (C_ScrollBar::Wheel
        // returns FALSE immediately when C_BIT_INVISIBLE). AdjustScrollbar only moves the slider.
        win->ScanClientArea(kc);
        win->RefreshClient(kc);
    }

    g_keyListPreserveScroll = false; // one-shot: consumed by this rebuild

    gMainHandler->WindowToFront(win);

    SetCursor(gCursors[CRSR_F16]);
    return TRUE;
}

void LoadKeyCB(long, short hittype, C_Base *control)
{
    C_EditBox *ebox;
    _TCHAR fname[MAX_PATH];

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gMainHandler->HideWindow(control->Parent_);

    ebox = (C_EditBox *)control->Parent_->FindControl(FILE_NAME);

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
    LoadAFile(TXT_LOAD_KEYBOARD, "config/*.KEY", NULL, LoadKeyCB,
              CloseWindowCB);
}

int CreateKeyMapList(char *filename)
{
    FILE *fp;
    char path[_MAX_PATH];

    C_Window *win;

    win = gMainHandler->FindWindow(
        SETUP_CONTROL_ADVANCED_WIN); // #53 button list is in the new window

    if (not win)
        return FALSE;

    sprintf(path, "%s/config/%s.key", FalconDataDirectory, filename);

    fp = fopen(path, "rt");

    if (not fp)
    {
        sprintf(path, "%s/config/keystrokes.key", FalconDataDirectory);
        fp = fopen(path, "rt");

        if (not fp)
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

    Keycodes = (C_Button *)win->FindControl(KEYCODES); //define this as KEYCODES
    Mapping = (C_Text *)win->FindControl(MAPPING); //define this as MAPPING
    Hline = (C_Line *)win->FindControl(HLINE); //define this as HLINE
    Vline = (C_Line *)win->FindControl(VLINE); //define this as VLINE

    // #53 if the .scf templates are missing, bail out instead of crashing later
    if (not Keycodes or not Mapping or not Hline or not Vline)
    {
        fclose(fp);
        return FALSE;
    }

    client =
        win->GetClientArea(Keycodes ? Keycodes->GetClient() :
                                      2); // #53 table client (was hardcoded 3)

    // #53 refresh device columns + header before the initial build
    if (Vline)
        g_tblRowH = Vline->GetH(); // #53 row height for uniform cell hotspots
    RebuildTableDeviceList();
    // #53 row separators span the actual table width (desc..last device column right edge).
    // Artscout - 2026: end the template Hline (the row-0 separator, just under the header) exactly
    // at the right border (DEVCOL_X0 + count*DEVCOL_W - 12), same as the clamped per-row lines in
    // AddKeyMapLines. Without the -12 (and the GetX() offset) it ran past the border, leaving a
    // single stray line under the header and a phantom cell corner on the right.
    if (Hline)
        Hline->SetXYWH(Hline->GetX(), Hline->GetY(),
                       (DEVCOL_X0 + g_tblDevCount * DEVCOL_W - 12) -
                           Hline->GetX(),
                       Hline->GetH());
    BuildTableHeader(win, Keycodes, Mapping);
    EnsureKeyCtxMenu(Keycodes);

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

        if (sscanf(buff, "%s %d %d %x %x %x %x %d %[^\n]s", funcName, &buttonId,
                   &mouseSide, &key2, &mod2, &key1, &mod1, &editable,
                   &descrip) < 8)
            continue;

        if (key2 == -2)
            continue;

        theFunc = FindFunctionFromString(funcName);

        keydescrip[0] = 0;

        if (not theFunc)
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

        // #53: the BMS label from controls.xml overrides the legacy keystrokes.key description.
        // controls.xml is generated offline (tools\gen_controls_xml.py); the label has no quotes.
        {
            const char *bmsLabel = ControlsXml_GetLabel(funcName);

            if (bmsLabel and bmsLabel[0])
            {
                strncpy(parsed, bmsLabel, _MAX_PATH - 2);
                parsed[_MAX_PATH - 2] = 0;
            }
        }

        parsed[40] =
            0; // #53 wider description column (no Clear button anymore)

        // #22: search filter — skip non-matching rows WITHOUT incrementing count (rows stay
        // contiguous with no gaps; extras are removed by RemoveExcessControls and the scrollbar
        // is recomputed). Matches both description and function name.
        if (not KeyListRowVisible(funcName, parsed))
            continue;

        if (not count)
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
            // #53 SetHdrStatusLine removed (old per-row blue bar; filled the table blue)
            UpdateKeyMap(win, Keycodes, Vline->GetH(), Map, HotSpot, count);

            UpdateMappingDescrip(
                win, Mapping, Vline->GetH(), parsed,
                count); //this will add the control if it doesn't exist

            AddKeyMapLines(win, Hline, Vline, count);
        }

        // #53 per-device columns (replaces the #52 Clear button; clear is now the right-click
        // context menu). Keeps the initial build consistent with UpdateKeyMapList.
        UpdateDeviceCells(win, Keycodes, Vline, Map, count);

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
    const int POVSymbols[] = {LEFT_HAT, RIGHT_HAT, CENTER_HAT, UP_HAT,
                              DOWN_HAT};
    const int POVSymbolCount = sizeof(POVSymbols) / sizeof(int);
    C_Button *button = NULL;

    for (int i = 0; i < POVSymbolCount; i++)
    {
        button = (C_Button *)control->Parent_->FindControl(POVSymbols[i]);

        if (button not_eq NULL)
        {
            if (not isActive)
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
        if (not IO.AnalogIsUsed(AXIS_THROTTLE))
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

        if (not IO.AnalogIsUsed(AXIS_YAW))
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
// #22: callback of the function search field in the main window. Fires on Enter/Escape
// (C_EditBox calls the callback on DIK_RETURN/DIK_ESCAPE). We read the text into g_keyFilter
// and rebuild the list (the filter is applied in UpdateKeyMapList).
void KeyListSearchCB(long, short, C_Base *control)
{
    C_EditBox *eb = (C_EditBox *)control;
    const _TCHAR *txt = eb ? eb->GetText() : NULL;

    if (txt)
    {
        strncpy(g_keyFilter, txt, sizeof(g_keyFilter) - 1);
        g_keyFilter[sizeof(g_keyFilter) - 1] = 0;
    }
    else
    {
        g_keyFilter[0] = 0;
    }

    g_keyListDisplayOnly = true;
    UpdateKeyMapList(PlayerOptions.GetKeyfile(), TRUE);
    g_keyListDisplayOnly = false;
}

// #22: callback of the SEPARATE device-filter dropdown (NOT JOYSTICK_SELECT — that one changes
// POV/FFB). It only changes the left column display of the list, without side effects.
// item id = SIM index+1 (like BuildControllerList): Keyboard => keys, joystick => buttons.
void KeyListDevFilterCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_SELECT)
        return;

    C_ListBox *lbox = (C_ListBox *)control;
    g_keyDevFilter = lbox->GetTextID() - 1;

    g_keyListDisplayOnly = true;
    UpdateKeyMapList(PlayerOptions.GetKeyfile(), TRUE);
    g_keyListDisplayOnly = false;
}

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

    if (newcontroller ==
        AxisMap.FlightControlDevice) // nothing changed, so no action required
    {
        return;
    }

    IO.Reset(); // set all axis (real and game) back to nada (off)

    if (AxisMap.FlightControlDevice ==
        SIM_KEYBOARD) // hrmmm... no sure what�s up here
    {
        SaveKeyMapList("laptop");
        UpdateKeyMapList(PlayerOptions.GetKeyfile(), TRUE);
    }

    if ((newcontroller == SIM_MOUSE) or (newcontroller == SIM_NUMDEVICES))
    {
        // ??? not allowed
        ShiAssert(false);
    }
    else if (newcontroller == SIM_KEYBOARD) // hrmmm... no sure what�s up here
    {
        SaveKeyMapList(PlayerOptions.GetKeyfile());
        UpdateKeyMapList("laptop", TRUE);

        SetJoystickAndPOVSymbols(
            false, control); // kill POV symbol (the keyboard has none)
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
        NumberOfPOVs =
            CurJoyCaps.dwPOVs; // Wombat778 4-27-04 Dont limit to 1 POV

        if (NumberOfPOVs > 0) // Retro 26Dec2003
            hasPOV = TRUE; // Retro 26Dec2003

        int POVSymbols[] = {LEFT_HAT, RIGHT_HAT, CENTER_HAT, UP_HAT, DOWN_HAT};
        const int POVSymbolCount = sizeof(POVSymbols) / sizeof(int);

        for (int i = 0; i < POVSymbolCount; i++)
        {
            button = (C_Button *)control->Parent_->FindControl(POVSymbols[i]);

            if (button not_eq NULL)
            {
                if (not hasPOV)
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

        hres = gpDIDevice[newcontroller]->GetObjectInfo(&devobj, DIJOFS_X,
                                                        DIPH_BYOFFSET);

        if (hres == DI_OK)
        {
            hres = gpDIDevice[newcontroller]->GetObjectInfo(&devobj, DIJOFS_Y,
                                                            DIPH_BYOFFSET);

            if (hres == DI_OK)
            {
                // FlightControlDevice = the leading device for POV/FFB.
                AxisMap.FlightControlDevice = newcontroller;

                // #24: pitch/roll are NO LONGER overwritten by the controller dropdown — they
                // are chosen in the axis window. Here we set them only as a DEFAULT (X/Y of this
                // device) if the axis is not assigned yet, so a new user immediately
                // gets working controls "out of the box". Already-assigned axes are left alone.
                if (AxisMap.Bank.Device == -1)
                {
                    AxisMap.Bank.Axis = DX_XAXIS;
                    AxisMap.Bank.Device = newcontroller;
                }

                if (AxisMap.Pitch.Device == -1)
                {
                    AxisMap.Pitch.Axis = DX_YAXIS;
                    AxisMap.Pitch.Device = newcontroller;
                }
            }
        }

        /* now check if device has a throttle */
        bool weHaveACougerUser =
            false; // ahh... that dumb metal POS has the throttle on RZ since it has no rudder.. or whatever..

        if (gDIDevNames[newcontroller])
        {
            if (not strcmp(gDIDevNames[newcontroller], "HOTAS Cougar Joystick"))
            {
                weHaveACougerUser = true;
            }
        }

        if (weHaveACougerUser == false)
        {
            hres = gpDIDevice[newcontroller]->GetObjectInfo(
                &devobj, DIJOFS_SLIDER(0), DIPH_BYOFFSET);

            if (hres == DI_OK)
            {
                AxisMap.Throttle.Device = newcontroller;
                AxisMap.Throttle.Axis = DX_SLIDER0;
            }
        }
        else
        {
            hres = gpDIDevice[newcontroller]->GetObjectInfo(&devobj, DIJOFS_Z,
                                                            DIPH_BYOFFSET);

            if (hres == DI_OK)
            {
                weHaveACougerUser = true;
                AxisMap.Throttle.Device = newcontroller;
                AxisMap.Throttle.Axis = DX_ZAXIS;
            }
        }

        /* now check if device has a rudder */
        if (weHaveACougerUser ==
            false) // cougar has no rudder and the RZ is taken anyways..
        {
            hres = gpDIDevice[newcontroller]->GetObjectInfo(&devobj, DIJOFS_RZ,
                                                            DIPH_BYOFFSET);

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

    // this function draws/hides the rudder/throttle bars, depending on if they�re mapped..
    SetThrottleAndRudderBars(control);
    {
        extern AxisMapping AxisMap;
        ControlsXml_WriteAxes(
            &AxisMap); // #53/#57: axes + soft props (center/ABDetent/reversed) into axismapping.xml
    }

    InitializeValueBars = 1; // Retro 26Dec2003
}

/************************************************************************/
// fills the controller listbox in the setup->controller tab
// there�s a strange 'thrustmaster-only' hack there, dunno why..
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
            if (not stricmp(gDIDevNames[SIM_JOYSTICK1 + i], "tm"))
            {
                delete[] gDIDevNames[SIM_JOYSTICK1 + i];
                gDIDevNames[SIM_JOYSTICK1 + i] = new TCHAR[MAX_PATH];
                _tcscpy(gDIDevNames[SIM_JOYSTICK1 + i], "Thrustmaster");
            }

            lbox->AddItem(i + SIM_JOYSTICK1 + 1, C_TYPE_ITEM,
                          gDIDevNames[SIM_JOYSTICK1 + i]);

            if (mHelmetIsUR and i == mHelmetID)
                lbox->SetItemFlags(i + SIM_JOYSTICK1 + 1, 0);
        }
    }

    lbox->Refresh();
}
