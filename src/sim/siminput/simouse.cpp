#include "F4Thread.h"
#include "sinput.h"
#include "cpmanager.h"
#include "dispcfg.h"
#include "simdrive.h"
#include "dispopts.h"

#include "commands.h" //Wombat778 10-07-2003 added for scroll wheel support

#include "inpFunc.h" //Wombat778 10-07-2003 added for scroll wheel support

int gxFuzz;
int gyFuzz;
int gxPos;
int gyPos;
int gxLast;
int gyLast;
int gMouseSensitivity;
int gSelectedCursor;

//Wombat778 10-07-2003 added the next four lines for scroll wheel support
InputFunctionType scrollupfunc;
InputFunctionType scrolldownfunc;
InputFunctionType middlebuttonfunc;
//Wombat778 10-07-2003 end of added lines

VU_TIME gTimeLastMouseMove;
VU_TIME gTimeLastCursorUpdate; //Wombat778 1-23-03

static int didSpin = 0;
static int didTilt = 0;

extern bool g_bRealisticAvionics;
extern bool MouseMenuActive; // Retro 15Feb2004 - oh well. Not too pretty.
// This indicates that some sort of overlay menu
// is active, and that the mouse should act in '2d mode'
// currently only used for the 'exit' screen
bool clickableMouseMode = false; // Retro 15Feb2004 - this holds the CURRENT STATE of the
// clickable cockpit. The SAVED (and DEFAULT) value is in the
// playeroptions, and is only loaded on entering the pit
// (in otwdrive.cpp)

#define NEW_MOUSELOOK_HANDLING // Retro 16Jan2004
#ifndef NEW_MOUSELOOK_HANDLING // Retro 16Jan2004
extern float g_fMouseLookSensitivity; //Wombat778 10-08-2003
#endif

#include "SimIO.h" // Retro 17Jan2004
#include "mouselook.h" // Retro 18Jan2004

static const int MAX_AXIS_THROW; // Retro 18Jan2004

// sfr: touch buddy support
/** variable indicating mouse is inside client area */
static bool mouseIn = true;

/** synchronizes FreeFalcon and windows cursor */
static void SimMouseResyncCursors(const int x, const int y)
{
    UpdateCursorPosition(x - gxPos, y - gyPos);
}

void SimMouseStopProcessing()
{
    mouseIn = false;
}

void SimMouseResumeProcessing(const int x, const int y)
{
    mouseIn = true;
    SimMouseResyncCursors(x, y);
}
// end touchbuddy

//***********************************
// void OnSimMouseInput()
//***********************************

//void OnSimMouseInput(HWND hWnd)
void OnSimMouseInput(HWND)
{
    // sfr: touch buddy support
    if (PlayerOptions.GetTouchBuddy() and not mouseIn)
    {
        return;
    }

    DIDEVICEOBJECTDATA ObjData[DMOUSE_BUFFERSIZE];
    DWORD dwElements = 0;
    HRESULT hResult;
    UINT i = 0;
    int dx = 0, dy = 0, dz = 0; //Wombat778 10-07-2003  added dz=0 for scrollwheel
    int action = 0;
    BOOL passThru = TRUE;
    static BOOL         oneDown = FALSE;

    static int tempx = 0; //Wombat778 10-10-2003
    static int tempy = 0; //Wombat778 10-10-2003

#if 0 // Retro 15Feb2004
    dwElements = DMOUSE_BUFFERSIZE;
    hResult = gpDIDevice[SIM_MOUSE]->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), ObjData, &dwElements, 0);

    if (hResult == DIERR_INPUTLOST)
    {
        gpDeviceAcquired[SIM_MOUSE] = FALSE;
        return;
    }

#else // Retro 15Feb2004 - 'my version' tries to re-aquire by itself.
    dwElements = DMOUSE_BUFFERSIZE;
    hResult = gpDIDevice[SIM_MOUSE]->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), ObjData, &dwElements, 0);

    if ((hResult == DIERR_INPUTLOST) or (hResult == DIERR_NOTACQUIRED))
    {
        hResult = gpDIDevice[SIM_MOUSE]->Acquire();

        if (hResult not_eq DI_OK)
        {
#pragma warning(disable:4127)
            ShiAssert(false);
#pragma warning(default:4127)
            gpDeviceAcquired[SIM_MOUSE] = FALSE;
            return;
        }
    }

#endif

    if (SUCCEEDED(hResult))
    {


        gTimeLastMouseMove = vuxRealTime;

        for (i = 0; i < dwElements; i++)
        {

            if (ObjData[i].dwOfs == DIMOFS_X)
            {
                dx += ObjData[i].dwData;
                action = CP_MOUSE_MOVE;
            }
            else if (ObjData[i].dwOfs == DIMOFS_Y)
            {
                dy += ObjData[i].dwData;
                action = CP_MOUSE_MOVE;
            }
            else if (ObjData[i].dwOfs == DIMOFS_Z)   //Wombat778 10-07-2003 Added following for scrollwheel.  Wheel acts as mouse Z axis.
            {
                dz += ObjData[i].dwData;

                if (theMouseWheelAxis.IsWheelActive() == false) // Retro 21Jan2004 - wheel not mapped to an axis, use it otherwise
                {
                    if (dz > 0)   //Wombat778 11-16-2003  Changed from < to > .  It was backwards before.
                    {
                        if (scrollupfunc)
                            scrollupfunc(1, KEY_DOWN, NULL);
                    }
                    else
                    {
                        if (scrolldownfunc)
                            scrolldownfunc(1, KEY_DOWN, NULL);
                    }
                }

                action = CP_CHECK_EVENT; //Wombat778 10-07-2003 apparently this is a fake event...seems like the right thing to do
            }
            else if (ObjData[i].dwOfs == DIMOFS_BUTTON0 and not (ObjData[i].dwData bitand 0x80))
            {
                action = CP_MOUSE_BUTTON0;
            }
            else if (ObjData[i].dwOfs == DIMOFS_BUTTON1 and not (ObjData[i].dwData bitand 0x80))
            {
                action = CP_MOUSE_BUTTON1;
                oneDown = FALSE;
            }
            else if (ObjData[i].dwOfs == DIMOFS_BUTTON1 and (ObjData[i].dwData bitand 0x80))
            {
                action = static_cast<unsigned long>(-1);
                oneDown = TRUE;
            }
            else if ((ObjData[i].dwOfs == DIMOFS_BUTTON3) and (ObjData[i].dwData bitand 0x80))   // Retro 22Jan2004
            {
#if 0
                PlayerOptions.SetClickablePitMode( not PlayerOptions.GetClickablePitMode()); //Wombat778 1-22-04 moved to playeroptions.
#else
                clickableMouseMode = not clickableMouseMode; // Retro 15Feb2004
#endif
            } // Retro 22Jan2004

            else if (ObjData[i].dwOfs == DIMOFS_BUTTON2 and (ObjData[i].dwData bitand 0x80))   //Wombat778 10-07-2003 Added for middle mouse button support
            {
                if (middlebuttonfunc) middlebuttonfunc(1, KEY_DOWN, NULL);

                action = CP_CHECK_EVENT; //Wombat778 10-07-2003 same rationale as above
                theMouseWheelAxis.ResetAxisValue(); // Retro 18Jan2004
            }


            else
            {
                continue;
            }

            if (action == CP_MOUSE_BUTTON0 or action == CP_MOUSE_BUTTON1)
            {
                passThru = OTWDriver.HandleMouseClick(gxPos, gyPos);

                if (passThru)

                    //Wombat778 10-11-2003  This is a hack because I couldnt get the
                    // button finding code to run from here. go figure
                    if (
                        OTWDriver.GetOTWDisplayMode() == OTWDriverClass::Mode3DCockpit or
                        OTWDriver.GetOTWDisplayMode() == OTWDriverClass::ModePadlockF3 or
                        OTWDriver.GetOTWDisplayMode() == OTWDriverClass::ModePadlockEFOV
                    )
                    {
                        if (action == CP_MOUSE_BUTTON0)
                        {
                            //Wombat778 11-7-2003 the first button has been clicked
                            //The mouse button has been pressed.  gxPos and gyPos will be read by Vcock_exec.
                            OTWDriver.Button3DList.clicked = 1;
                        }
                        else
                        {
                            //Wombat778 11-7-2003 The second button has been clicked
                            OTWDriver.Button3DList.clicked = 2;
                        }

                        passThru = false; //Prevent the next line from running.
                    }
            }

            if (passThru)
            {
                gSelectedCursor = OTWDriver.pCockpitManager->Dispatch(action, gxPos, gyPos);
            }
        }

#ifdef USE_DINPUT_8

        /************************************************************************/
        // Retro 17Jan2004
        // "Faking" an absolute axis with the (relative) mouse z axis
        // Problem is that I can´t set the range as a dinput property, so I have
        // to clamp manually. Range is either 0..15000 for unipolar or -10000..10000
        // for bipolar axis. MouseWheelSensitivity is for this axis an INTEGER
        // (as opposed to the mouselook sensitivity)
        //
        // This calc should be done every frame (or rather, everytime new mousedata
        // is available), as the axis can be used for whatever the user wants
        // (except pitch/bank and throttle/throttle2)
        //
        // Problem: all this data is on the global scope. Also, this data is not
        // reinit when I exit/enter the 3d. it´s also not init correctly for all
        // axis (it inits to 0 which can be bad for some axis, ie FOV)
        /************************************************************************/
        if ((dz) and (IO.MouseWheelExists() == true))
        {
            // Retro 18Jan2004
            theMouseWheelAxis.AddToAxisValue(dz * PlayerOptions.GetMouseWheelSensitivity());
        }

#endif // ..ends


        // DO the mouse move if one happened

#ifdef NEW_MOUSELOOK_HANDLING // Retro 16Jan2004
#if 1 // Retro 22Jan2004 - reorganised the code somewhat

        if (dx or dy)
        {
            if (OTWDriver.GetOTWDisplayMode() == OTWDriverClass::Mode3DCockpit)
            {
                // reset these wacky variables when a mode change occured..
                if (didTilt)
                {
                    didTilt = FALSE;
                    OTWDriver.ViewTiltHold();
                    tempy = 0; //Wombat778 10-10-2003
                }

                if (didSpin)
                {
                    didSpin = FALSE;
                    OTWDriver.ViewSpinHold();
                    tempx = 0; //Wombat778 10-10-2003
                }

                // now, if we´re in panning mode, do some stuff with eyepan/eyetilt
#if 0 // Retro 15Feb2004

                if (PlayerOptions.GetClickablePitMode() == false) //Wombat778 1-22-04 moved to playeroptions
#else
                if (clickableMouseMode == false)
#endif
                {
                    if ( not oneDown) // Retro 22Jan2004 - the RMB can temporarily (when held down) force the 'opposite' mode
                    {
                        float MouseSensitivity = PlayerOptions.GetMouseLookSensitivity(); // Retro 16Jan2004
                        OTWDriver.ViewRelativePanTilt(dx * MouseSensitivity, dy * MouseSensitivity);
                    }
                    else
                        UpdateCursorPosition(dx, dy);

                }
                else // if we´re in clickable mode, move only the mousepointer
                {
                    if ( not oneDown) // Retro 22Jan2004 - the RMB can temporarily (when held down) force the 'opposite' mode
                        UpdateCursorPosition(dx, dy);
                    else
                    {
                        float MouseSensitivity = PlayerOptions.GetMouseLookSensitivity(); // Retro 16Jan2004
                        OTWDriver.ViewRelativePanTilt(dx * MouseSensitivity, dy * MouseSensitivity);
                    }
                }
            }
            else if (OTWDriver.GetOTWDisplayMode() == OTWDriverClass::Mode2DCockpit)
            {
                if (oneDown)
                {
                    /************************************************************************/
                    // Retain old method for the 2d cockpit and external views because
                    // eyepan/eyetilt doesnt do anything in those views
                    //
                    // Wombat778 10-10-2003
                    // added these because the previous method of 2d mouselook was crap.
                    /************************************************************************/
                    tempx += dx;
                    tempy += dy;

                    float MouseSensitivity = PlayerOptions.GetMouseLookSensitivity(); // Retro 16Jan2004

                    //Wombat778 10-10-2003 Look at the total distance mouse has moved rather than speed
                    if (tempx * MouseSensitivity > 20)
                    {
                        didSpin = TRUE;
                        OTWDriver.ViewSpinRight();
                        tempx = 0;
                    }

                    else if (tempx * MouseSensitivity < -20)
                    {
                        didSpin = TRUE;
                        OTWDriver.ViewSpinLeft();
                        tempx = 0;
                    }

                    if (tempy * MouseSensitivity < -20)
                    {
                        didTilt = TRUE;
                        OTWDriver.ViewTiltUp();
                        tempy = 0;
                    }

                    else if (tempy * MouseSensitivity > 20)
                    {
                        didTilt = TRUE;
                        OTWDriver.ViewTiltDown();
                        tempy = 0;
                    }

                    /************************************************************************/
                    // if the mouse has stopped moving quickly stop spinning or tilting
                    /************************************************************************/
                    if (abs(dy) < 3)
                    {
                        didTilt = FALSE;
                        OTWDriver.ViewTiltHold();
                    }

                    if (abs(dx) < 3)
                    {
                        didSpin = FALSE;
                        OTWDriver.ViewSpinHold();
                    }
                }
                else // let got of the RMB (oneDown == false) - set the various 'move' vars to false
                {
                    if (didTilt)
                    {
                        didTilt = FALSE;
                        OTWDriver.ViewTiltHold();
                        tempy = 0; //Wombat778 10-10-2003
                    }

                    if (didSpin)
                    {
                        didSpin = FALSE;
                        OTWDriver.ViewSpinHold();
                        tempx = 0; //Wombat778 10-10-2003
                    }

                    // still update the 2d cursor pos though..
                    UpdateCursorPosition(dx, dy);
                }
            }
            // in most of the other views (external..)
            else if (( not MouseMenuActive) and (PlayerOptions.GetMouseLook() == true))
            {
                /************************************************************************/
                // Retro 16Jan2004
                //
                // Making mouselook smoother by cirumvention the whole 'button-simulation'
                // stuff. Problems:
                // 1) there´s only one mouselook sensitivity variable, but inside<->outside
                // need different sensitivity
                // 2) this code would not have to be executed every frame (and also not
                // everytime new mousedata is there) but only everytime we are in a
                // outside view mode - however I can´t tell this here
                //
                // Solution to 1) the external mouselook values (range 0.2 - 1.5) get scaled up
                // by a factor or 10 so I get a range of 2 - 15 (nice integers)
                // of course, on scaling them back I also have to divide them by a factor
                // of 10
                // Solution to 2) maybe only accumulate the data here and do any calculations
                // on it in the otwdriver class (that only does this when needed)
                /************************************************************************/
                if (dx)
                    theMouseView.AddAzimuth((float)dx * 10.f * PlayerOptions.GetMouseLookSensitivity());

                if (dy)
                    theMouseView.AddElevation((float)dy * 10.f * PlayerOptions.GetMouseLookSensitivity());

                theMouseView.Compute(0, true);

                // Retro end
            }
            else if (MouseMenuActive) // Retro 15Feb2004
            {
                UpdateCursorPosition(dx, dy);
            }

        }
        /************************************************************************/
        // get here if the mouse hasnt moved at all
        // make sure any 'move' vars are set to off..
        /************************************************************************/
        else if (OTWDriver.GetOTWDisplayMode() == OTWDriverClass::Mode2DCockpit)
        {
            if (didTilt)
            {
                didTilt = FALSE;
                OTWDriver.ViewTiltHold();
                tempy = 0; //Wombat778 10-10-2003
            }

            if (didSpin)
            {
                didSpin = FALSE;
                OTWDriver.ViewSpinHold();
                tempx = 0; //Wombat778 10-10-2003
            }
        }

#else
        //MI don't move the head with the right mousebutton down
        //if(g_bRealisticAvionics) //Wombat778 9-27-2003  Changed to allow mouselook from CFG
        // oneDown = FALSE

        /************************************************************************/
        // This is wombat´s 3d cockpit mouselook
        /************************************************************************/
        if (dx or dy)
        {
            if (oneDown)
            {
                float MouseSensitivity = PlayerOptions.GetMouseLookSensitivity(); // Retro 16Jan2004

                /************************************************************************/
                //Wombat778 10-09-2003 function only is true if in 3d cockpit
                /************************************************************************/
                if ( not OTWDriver.ViewRelativePanTilt(dx * MouseSensitivity, dy * MouseSensitivity))
                {
                    if (OTWDriver.GetOTWDisplayMode() == OTWDriverClass::Mode2DCockpit)
                    {
                        /************************************************************************/
                        // Retain old method for the 2d cockpit and external views because
                        // eyepan/eyetilt doesnt do anything in those views
                        //
                        // Wombat778 10-10-2003
                        // added these because the previous method of 2d mouselook was crap.
                        /************************************************************************/
                        tempx += dx;
                        tempy += dy;


                        //Wombat778 10-10-2003 Look at the total distance mouse has moved rather than speed
                        if (tempx * MouseSensitivity > 20)
                        {
                            didSpin = TRUE;
                            OTWDriver.ViewSpinRight();
                            tempx = 0;
                        }

                        else if (tempx * MouseSensitivity < -20)
                        {
                            didSpin = TRUE;
                            OTWDriver.ViewSpinLeft();
                            tempx = 0;
                        }

                        if (tempy * MouseSensitivity < -20)
                        {
                            didTilt = TRUE;
                            OTWDriver.ViewTiltUp();
                            tempy = 0;
                        }

                        else if (tempy * MouseSensitivity > 20)
                        {
                            didTilt = TRUE;
                            OTWDriver.ViewTiltDown();
                            tempy = 0;
                        }

                        /************************************************************************/
                        // if the mouse has stopped moving quickly stop spinning or tilting
                        /************************************************************************/
                        if (abs(dy) < 3)
                        {
                            didTilt = FALSE;
                            OTWDriver.ViewTiltHold();
                        }

                        if (abs(dx) < 3)
                        {
                            didSpin = FALSE;
                            OTWDriver.ViewSpinHold();
                        }
                    }
                }

                /************************************************************************/
                // Wombat778 10-10-2003  get here if in 3d cockpit. Used for if in 3d
                // cockpit, but still spinning or tiling
                /************************************************************************/
                else
                {
                    if (didTilt)
                    {
                        didTilt = FALSE;
                        OTWDriver.ViewTiltHold();
                        tempy = 0; //Wombat778 10-10-2003
                    }

                    if (didSpin)
                    {
                        didSpin = FALSE;
                        OTWDriver.ViewSpinHold();
                        tempx = 0; //Wombat778 10-10-2003
                    }
                }
            }
            /************************************************************************/
            // get here if the RMB was not down.
            /************************************************************************/
            else
            {
                if (OTWDriver.GetOTWDisplayMode() == OTWDriverClass::Mode2DCockpit)
                {
                    if (didTilt)
                    {
                        didTilt = FALSE;
                        OTWDriver.ViewTiltHold();
                        tempy = 0; //Wombat778 10-10-2003
                    }

                    if (didSpin)
                    {
                        didSpin = FALSE;
                        OTWDriver.ViewSpinHold();
                        tempx = 0; //Wombat778 10-10-2003
                    }
                }

                if ((OTWDriver.GetOTWDisplayMode() not_eq OTWDriverClass::Mode3DCockpit) or
                    (OTWDriver.GetOTWDisplayMode() not_eq OTWDriverClass::Mode2DCockpit))
                {
                    // Update cursor position otherwise
                    UpdateCursorPosition(dx, dy);
                }
            }

            // we come here if 1) no RMB pressed 2) not in 2d pit mode
            // aargh this (whole mouselook) code is a freaking mess 
            if ((PlayerOptions.GetMouseLook() == true) and 
                (OTWDriver.GetOTWDisplayMode() not_eq OTWDriverClass::Mode3DCockpit) and 
                (OTWDriver.GetOTWDisplayMode() not_eq OTWDriverClass::Mode2DCockpit))
            {
                /************************************************************************/
                // Retro 16Jan2004
                //
                // Making mouselook smoother by cirumvention the whole 'button-simulation'
                // stuff. Problems:
                // 1) there´s only one mouselook sensitivity variable, but inside<->outside
                // need different sensitivity
                // 2) this code would not have to be executed every frame (and also not
                // everytime new mousedata is there) but only everytime we are in a
                // outside view mode - however I can´t tell this here
                //
                // Solution to 1) the external mouselook values (range 0.2 - 1.5) get scaled up
                // by a factor or 10 so I get a range of 2 - 15 (nice integers)
                // of course, on scaling them back I also have to divide them by a factor
                // of 10
                // Solution to 2) maybe only accumulate the data here and do any calculations
                // on it in the otwdriver class (that only does this when needed)
                /************************************************************************/
                if (dx)
                    theMouseView.AddAzimuth((float)dx * 10.f * PlayerOptions.GetMouseLookSensitivity());

                if (dy)
                    theMouseView.AddElevation((float)dy * 10.f * PlayerOptions.GetMouseLookSensitivity());

                theMouseView.Compute(0, true);
                // Retro end
            }

        }
        /************************************************************************/
        // get here if the mouse hasnt moved at all
        /************************************************************************/
        else if (OTWDriver.GetOTWDisplayMode() == OTWDriverClass::Mode2DCockpit)
        {
            if (didTilt)
            {
                didTilt = FALSE;
                OTWDriver.ViewTiltHold();
                tempy = 0; //Wombat778 10-10-2003
            }

            if (didSpin)
            {
                didSpin = FALSE;
                OTWDriver.ViewSpinHold();
                tempx = 0; //Wombat778 10-10-2003
            }
        }

#endif
#else

        if (dx or dy)
        {
            if (oneDown)
            {
                // Head pan in virtual cockpit

                //Wombat778 10-08-2003  The following function handles all of the crap below and makes mouselook smooth,

                if ( not OTWDriver.ViewRelativePanTilt(dx * g_fMouseLookSensitivity, dy * g_fMouseLookSensitivity)) //Wombat778 10-09-2003 function only is true if in 3d cockpit
                {
                    //Retain old method for the 2d cockpit and external views because eyepan/eyetilt doesnt do anything in those views

                    tempx += dx; //Wombat778 10-10-2003 added these because the previous method of 2d mouselook was crap.
                    tempy += dy;


                    if (tempx * g_fMouseLookSensitivity > 20) //Wombat778 10-10-2003 Look at the total distance mouse has moved rather than speed
                    {
                        didSpin = TRUE;
                        OTWDriver.ViewSpinRight();
                        tempx = 0;
                    }

                    else if (tempx * g_fMouseLookSensitivity < -20)
                    {
                        didSpin = TRUE;
                        OTWDriver.ViewSpinLeft();
                        tempx = 0;
                    }

                    if (tempy * g_fMouseLookSensitivity < -20)
                    {
                        didTilt = TRUE;
                        OTWDriver.ViewTiltUp();
                        tempy = 0;
                    }

                    else if (tempy * g_fMouseLookSensitivity > 20)
                    {
                        didTilt = TRUE;
                        OTWDriver.ViewTiltDown();
                        tempy = 0;
                    }


                    if (abs(dy) < 3)  //if the mouse has stopped moving quickly stop spinning or tilting
                    {
                        didTilt = FALSE;
                        OTWDriver.ViewTiltHold();
                    }

                    if (abs(dx) < 3)
                    {
                        didSpin = FALSE;
                        OTWDriver.ViewSpinHold();
                    }



                    //Wombat778 10-10-2003 REmoved because my way is better

                    /* if (dx * g_fMouseLookSensitivity > 3)
                     {
                     didSpin = TRUE;
                     OTWDriver.ViewSpinRight();
                     }
                     else if (dx * g_fMouseLookSensitivity < -6)
                     {
                     didSpin = TRUE;
                     OTWDriver.ViewSpinLeft();
                     }
                     else
                     {
                     didSpin = FALSE;
                     OTWDriver.ViewSpinHold();
                     }

                     if (dy * g_fMouseLookSensitivity < -6)
                     {
                     didTilt = TRUE;
                     OTWDriver.ViewTiltUp();
                     }
                     else if (dy * g_fMouseLookSensitivity > 6)
                     {
                     didTilt = TRUE;
                     OTWDriver.ViewTiltDown();
                     }
                     else
                     {
                     didTilt = FALSE;
                     OTWDriver.ViewTiltHold();
                     }*/
                }
                else //Wombat778 10-10-2003  get here if in 3d cockpit. Used for if in 3d cockpit, but still spinning or tiling
                {
                    if (didTilt)
                    {
                        didTilt = FALSE;
                        OTWDriver.ViewTiltHold();
                        tempy = 0; //Wombat778 10-10-2003
                    }

                    if (didSpin)
                    {
                        didSpin = FALSE;
                        OTWDriver.ViewSpinHold();
                        tempx = 0; //Wombat778 10-10-2003
                    }
                }

            }
            else //get here if the RMB was not down.
            {
                if (didTilt)
                {
                    didTilt = FALSE;
                    OTWDriver.ViewTiltHold();
                    tempy = 0; //Wombat778 10-10-2003
                }

                if (didSpin)
                {
                    didSpin = FALSE;
                    OTWDriver.ViewSpinHold();
                    tempx = 0; //Wombat778 10-10-2003
                }

                // Update cursor position otherwise
                UpdateCursorPosition(dx, dy);
            }
        }
        else //get here if the mouse hasnt moved at all
        {
            if (didTilt)
            {
                didTilt = FALSE;
                OTWDriver.ViewTiltHold();
                tempy = 0; //Wombat778 10-10-2003
            }

            if (didSpin)
            {
                didSpin = FALSE;
                OTWDriver.ViewSpinHold();
                tempx = 0; //Wombat778 10-10-2003
            }
        }

#endif
    }
}

//*******************************************************************
// void UpdateCursorPosition()
//
// Move our private cursor in the requested direction, subject to
// clipping, scaling and all that other stuff.
//
// This does not redraw the cursor.  That is done in the OTW routines
//*******************************************************************

void UpdateCursorPosition(DWORD xOffset, DWORD yOffset)
{

    // Pick up any leftover fuzz from last time.  This is importatnt
    // when scaling dow mouse motions.  Otherwise, the user can drag to
    // the right extremely slow tfor the length of the table and not
    // get anywhere.

    xOffset += gxFuzz;
    gxFuzz = 0;
    yOffset += gyFuzz;
    gyFuzz = 0;

    switch (gMouseSensitivity)
    {

        case LO_SENSITIVITY:
            gxFuzz = xOffset % 2; // Remember the fuzz for next time
            gyFuzz = yOffset % 2;

            xOffset /= 2;
            yOffset /= 2;
            break;

        case NORM_SENSITIVITY: // No Adjustments needed
        default:
            break;

        case HI_SENSITIVITY:
            xOffset *= 2; // Magnify
            yOffset *= 2;
            break;
    }

    gxLast = gxPos;
    gyLast = gyPos;

    gxPos += xOffset;
    gyPos += yOffset;

    // Constrain the cursor to the client area
    if (gxPos < 0)
    {
        gxPos = 0;
    }

    if (gxPos >= DisplayOptions.DispWidth)
    {
        gxPos = DisplayOptions.DispWidth - 1;
    }

    if (gyPos < 0)
    {
        gyPos = 0;
    }

    if (gyPos >= DisplayOptions.DispHeight)
    {
        gyPos = DisplayOptions.DispHeight - 1;
    }

    gTimeLastCursorUpdate = vuxRealTime; //Wombat778 1-23-03  added so we know the last time the cursor position moved.
}
