#include "stdhdr.h"
#include "mfd.h"
#include "hud.h"
#include "sms.h"
#include "SmsDraw.h"
#include "airframe.h"
#include "Aircrft.h"
#include "missile.h"
#include "radar.h"
#include "PlayerRwr.h"
#include "fcc.h"
#include "otwdrive.h"
#include "playerop.h"
#include "Graphics/Include/render2d.h"
#include "Graphics/Include/canvas3d.h"
#include "Graphics/Include/tviewpnt.h"
#include "dispcfg.h"
#include "simdrive.h"
#include "camp2sim.h"
#include "fack.h"
#include "dispopts.h"
#include "digi.h"
#include "lantirn.h"
#include "otwdrive.h" //MI
#include "cpmanager.h" //MI
#include "icp.h" //MI
#include "aircrft.h" //MI
#include "fcc.h" //MI
#include "radardoppler.h" //MI
#include "object.h"
#include "popmenu.h" // a.s. begin
#include "Graphics/Include/renderow.h"  // a.s.
#include "Graphics/Include/grinline.h" // a.s. end

#include "flightData.h" // MD -- 20040727

//MI
void DrawBullseyeCircle(VirtualDisplay* display, float cursorX, float cursorY);


extern bool g_bRealisticAvionics;
extern bool g_bINS;


extern bool g_bEnableMfdColors; // a.s. begin: makes Mfds transparent and colored
extern float g_fMfdTransparency;// a.s.
extern float g_fMfdRed; // a.s.
extern float g_fMfdGreen; // a.s.
extern float g_fMfdBlue; // a.s.
unsigned long mycolor; // a.s. end

//ATARIBABY for 3d pit RTT MFD debug frame
extern bool g_b3DClickableCockpitDebug;
extern bool g_b3DRTTCockpitDebug;

// JB 010802
extern bool g_b3dMFDLeft;
extern bool g_b3dMFDRight;

extern bool g_bGreyMFD;
extern bool g_bGreyScaleMFD;
extern bool bNVGmode;

MFDClass* MfdDisplay[NUM_MFDS];
SensorClass* FindSensor(SimMoverClass* theObject, int sensorType);
//DrawableClass* MFDClass::ownDraw = NULL;
//DrawableClass* MFDClass::offDraw = NULL;
//DrawableClass* MFDClass::lantirnDraw = NULL;
DrawableClass* MFDClass::mfdDraw[MaxPrivMode];
DrawableClass* MFDClass::HadDrawable;


// location and dimensions of the 2 MFD's in the virtual cockpit
// JPO - set back - can now be overridden in the 3dckpit.dat file
Tpoint lMFDul = { 18.205f , -7.089f, 7.793f };
Tpoint lMFDur = { 18.205f , -2.770f, 7.793f };
Tpoint lMFDll = { 17.501f , -7.089f, 11.816f };
int ltMFDleft = 265; // ASSO:
int ltMFDtop = 5;
int ltMFDright = 415;
int ltMFDbottom = 155;
char lcMFDblend = 'g';
float lcMFDalpha = 1.0f;

Tpoint rMFDul = { 18.181f , 2.834f, 7.883f };
Tpoint rMFDur = { 18.181f , 6.994f, 7.883f };
Tpoint rMFDll = { 17.475f , 2.834f, 11.978f };
int rtMFDleft = 265; // ASSO:
int rtMFDtop = 265;
int rtMFDright = 415;
int rtMFDbottom = 415;
char rcMFDblend = 'g';
float rcMFDalpha = 1.0f;


extern int MfdSize;
extern RECT VirtualMFD[OTWDriverClass::NumPopups + 1];
#define NUM_MFD_INTENSITIES        5
#if 0
static int MFDColorTable[NUM_MFD_COLORS] =
{
    0x00FF00,
    0x00B000,
    0x007F00,
    0x003F00,
    0x001F00
};
#endif
// JPO - these masks are used to decided the colour intensity
static int MFDMasks[NUM_MFD_INTENSITIES] =
{
    0xffffff, // full intensity
    0xB0B0B0,
    0x7F7F7F,
    0x3F3F3F,
    0x1F1F1F, // lowest intensity
};

// RV - I-Hawk - write "HAD" instead of "HUD"
char *MFDClass::ModeNames[MAXMODES] =
{
    "  ", // blank
    "", // menu
    "TFR", "FLIR", "TEST", "DTE", "FLCS", "WPN", "TGP",
    "HSD", "FCR", "SMS", "RWR", /*"HUD,*/ "HAD",
};

MFDClass::MfdMode MFDClass::initialMfdL[MFDClass::MAXMM][3] =
{
    {FCRMode, MfdOff, TestMode}, // AA
    {FCRMode, MfdOff, TestMode}, // AG
    {FCRMode, MfdOff, TestMode}, // NAV
    {FCRMode, MfdOff, TestMode}, // MSL
    {FCRMode, MfdOff, TestMode}, // DGFT
};
MFDClass::MfdMode MFDClass::initialMfdR[MFDClass::MAXMM][3] =
{
    {SMSMode, FCCMode, WPNMode}, //AA
    {SMSMode, FCCMode, WPNMode}, //AG
    {FCCMode, SMSMode, FLCSMode}, //NAV
    {SMSMode, FCCMode, WPNMode}, //MSL
    {SMSMode, FCCMode, WPNMode}, //DGFT
};

MFDClass::MFDClass(int count, Render3D *r3d)
{
    id = count;
    drawable = NULL;
    viewPoint = NULL;
    ownship = NULL;
    curmm = 2;
    newmm = 2;

    if (count % 2 == 0)
    {
        memcpy(primarySecondary, initialMfdL, sizeof initialMfdL);
    }
    else
    {
        memcpy(primarySecondary, initialMfdR, sizeof initialMfdR);
    }

    changeMode = FALSE;

    for (int i = 0; i < MAXMM; i++)
        cursel[i] = 0;

    privateImage = new ImageBuffer;
    // COBRA - RED - MFD
    privateImage->Setup(&FalconDisplay.theDisplayDevice, MfdSize, MfdSize, /*VideoMem*/SystemMem, None);
    privateImage->SetChromaKey(0x0);
    image = OTWDriver.OTWImage;
    color = 0x00ff00;
    intensity = 0;
    mode = primarySecondary[curmm][0];
    restoreMode = mode;

    // Set bounds based on resolution
    vTop = vRight = -1.0F;
    vLeft = vBottom = 1.0F;
    vLeft = 1.0F - VirtualMFD[id].top / (DisplayOptions.DispHeight * 0.5F);
    vRight = 1.0F - VirtualMFD[id].bottom / (DisplayOptions.DispHeight * 0.5F);
    vTop = -1.0F + VirtualMFD[id].left / (DisplayOptions.DispWidth * 0.5F);
    vBottom = -1.0F + VirtualMFD[id].right / (DisplayOptions.DispWidth * 0.5F);

    viewPoint = OTWDriver.GetViewpoint();

    TGPWarning = 0; // RV - I-Hawk

    // RED - Passed into CreateDrawables
    /* if (mfdDraw[MfdOff] == NULL) mfdDraw[MfdOff] = new BlankMfdDrawable;
     if (mfdDraw[MfdMenu] == NULL) mfdDraw[MfdMenu] = new MfdMenuDrawable;
     if (mfdDraw[TFRMode] == NULL) mfdDraw[TFRMode] = new LantirnDrawable;
     if (mfdDraw[FLIRMode] == NULL) mfdDraw[FLIRMode] = new FlirMfdDrawable;
     if (mfdDraw[TestMode] == NULL) mfdDraw[TestMode] = new TestMfdDrawable;
     if (mfdDraw[DTEMode] == NULL) mfdDraw[DTEMode] = new DteMfdDrawable;
     if (mfdDraw[FLCSMode] == NULL) mfdDraw[FLCSMode] = new FlcsMfdDrawable;
     if (mfdDraw[WPNMode] == NULL) mfdDraw[WPNMode] = new WpnMfdDrawable;
     if (mfdDraw[TGPMode] == NULL) mfdDraw[TGPMode] = new TgpMfdDrawable;

     for (i = 0; i < MaxPrivMode; i++) {
     if (mfdDraw[i] == NULL){
     mfdDraw[i] = new BlankMfdDrawable;
     }
     }*/

    // ASSO: commented out old canvas setup
    // set up the virtual cockpit MFD
    virtMFD = new Canvas3D;
    ShiAssert(virtMFD);
    virtMFD->Setup(r3d);

    // left or right
    if (id == 0)
    {
        if (g_b3dMFDLeft)
        {
            // ASSO:
            tLeft = ltMFDleft;
            tTop = ltMFDtop;
            tRight = ltMFDright;
            tBottom = ltMFDbottom;
            cUL = lMFDul;
            cUR = lMFDur;
            cLL = lMFDll;
            cBlend = lcMFDblend;
            cAlpha = lcMFDalpha;
            //virtMFD->SetCanvas( &lMFDul, &lMFDur, &lMFDll );
        }
    }
    else
    {
        if (g_b3dMFDRight)
        {
            // ASSO:
            tLeft = rtMFDleft;
            tTop = rtMFDtop;
            tRight = rtMFDright;
            tBottom = rtMFDbottom;
            cUL = rMFDul;
            cUR = rMFDur;
            cLL = rMFDll;
            cBlend = rcMFDblend;
            cAlpha = rcMFDalpha;
            //virtMFD->SetCanvas( &rMFDul, &rMFDur, &rMFDll );
        }
    }

    //MI init vars
    EmergStoreMode = FCCMode;
}


MFDClass::~MFDClass()
{
    FreeDrawable();

    if (privateImage)
    {
        privateImage->Cleanup();
        delete privateImage;
    }

    mode = MfdOff;
    drawable = NULL;

    // RED - Passed into DestroyDrawables
    /* for (int i = 0; i < MaxPrivMode; i++) {
     if (mfdDraw[i]) {
     mfdDraw[i]->DisplayExit();
     delete mfdDraw[i];
     mfdDraw[i] = NULL;
     }
     }
    */
    delete virtMFD;
    virtMFD = NULL;
}





// RED - This function creates and assigns all the drawables shared by the various MFDs
// WARNING  it Has to be called BEFORE ANY MFD Use/allocation
void MFDClass::CreateDrawables(void)
{
    // 1st of all, kill anything already allocated
    DestroyDrawables();

    // then allocate new drawables
    if (mfdDraw[MfdOff] == NULL) mfdDraw[MfdOff] = new BlankMfdDrawable;

    if (mfdDraw[MfdMenu] == NULL) mfdDraw[MfdMenu] = new MfdMenuDrawable;

    if (mfdDraw[TFRMode] == NULL) mfdDraw[TFRMode] = new LantirnDrawable;

    if (mfdDraw[FLIRMode] == NULL) mfdDraw[FLIRMode] = new FlirMfdDrawable;

    if (mfdDraw[TestMode] == NULL) mfdDraw[TestMode] = new TestMfdDrawable;

    if (mfdDraw[DTEMode] == NULL) mfdDraw[DTEMode] = new DteMfdDrawable;

    if (mfdDraw[FLCSMode] == NULL) mfdDraw[FLCSMode] = new FlcsMfdDrawable;

    if (mfdDraw[WPNMode] == NULL) mfdDraw[WPNMode] = new WpnMfdDrawable;

    if (mfdDraw[TGPMode] == NULL) mfdDraw[TGPMode] = new TgpMfdDrawable;

    // RV - I-Hawk
    //Allocating a new HadDrawable... can't do up there with rest of private mods as
    //  when I tried to add a new private mode, it messed up all MFD modes order etc... so
    // managing HAD mode with it's own pointer
    if (HadDrawable == NULL) HadDrawable = new HadMfdDrawable;

    for (DWORD i = 0; i < MaxPrivMode; i++)
    {
        if (mfdDraw[i] == NULL)
        {
            mfdDraw[i] = new BlankMfdDrawable;
        }
    }

    // RV - I-Hawk - Handle the HadDrawable same as the others
    if (HadDrawable == NULL)
    {
        HadDrawable = new BlankMfdDrawable;
    }
}


// RED - This function destroy all the drawables shared by MFDs
// WARNING  it Has to be called AFTER ALL MFDs are destroyed
void MFDClass::DestroyDrawables(void)
{
    for (int i = 0; i < MaxPrivMode; i++)
    {
        if (mfdDraw[i])
        {
            mfdDraw[i]->DisplayExit();
            delete mfdDraw[i];
            mfdDraw[i] = NULL;
        }
    }

    // RV - I-Hawk - Handle the HadDrawable destruction along with the other mods
    if (HadDrawable)
    {
        HadDrawable->DisplayExit();
        delete HadDrawable;
        HadDrawable = NULL;
    }
}


char *MFDClass::ModeName(int n)
{
    ShiAssert(n >= 0 and n < MAXMODES);
    return ModeNames[n];
}

void MFDClass::SetNewRenderer(Render3D *r3d)
{
    virtMFD->ResetTargetRenderer(r3d);
}

void MFDClass::UpdateVirtualPosition(const Tpoint *loc, const Trotation *rot)
{
    virtMFD->Update(loc, (struct Trotation *) rot);
}

void MFDClass::SetOwnship(AircraftClass *newOwnship)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (playerAC and playerAC->IsSetFlag(MOTION_OWNSHIP) and ownship == playerAC and newOwnship not_eq playerAC)
    {
        // If we are jumping to another vehicle view ...
        restoreMode = mode; // ... save the current MFD state
    }

    ownship = newOwnship;
    FreeDrawable();
    drawable = NULL;

    // If we are jumping back into our F16
    if (playerAC and playerAC->IsSetFlag(MOTION_OWNSHIP) and ownship == playerAC)
    {
        SetMode(restoreMode); // restore the saved MFD state
    }
    else
    {
        SetMode(MfdOff); // Otherwise turn the MFD off
    }
}

void MFDClass::NextMode(void)
{
    if (g_bRealisticAvionics and id < 2)
    {
        cursel[curmm] --;

        if (cursel[curmm] < 0)
        {
            cursel[curmm] = 2;
        }

        mode = primarySecondary[curmm][cursel[curmm]];

        //MI addition
        if (mode == MfdOff)
            NextMode();

        SetMode(mode);
    }
    else
    {
        switch (mode)
        {
            case FCCMode:
                SetMode(FCRMode);
                break;

            case FCRMode:
                SetMode(SMSMode);
                break;

            case SMSMode:
                SetMode(RWRMode);
                break;

            case RWRMode:

                // M.N. added full realism mode
                if (PlayerOptions.GetAvionicsType() not_eq ATRealistic and PlayerOptions.GetAvionicsType() not_eq ATRealisticAV)
                {
                    SetMode(MfdOff);
                }
                else
                {
                    // SetMode (HUDMode)
                    SetMode(HADMode);    // RV - I-Hawk
                }

                break;

                //case HUDMode:
            case HADMode: // RV - I-Hawk -
                SetMode(MfdOff);
                break;

            case MfdOff:

                // M.N. added full realism mode
                if (PlayerOptions.GetAvionicsType() not_eq ATRealistic and PlayerOptions.GetAvionicsType() not_eq ATRealisticAV)
                {
                    SetMode(FCCMode);
                }
                else
                {
                    SetMode(MfdMenu);
                }

                break;

            case MfdMenu:
                if (theLantirn and theLantirn->IsEnabled())
                    SetMode(TFRMode);
                else
                    SetMode(FCCMode);

                break;

            case TFRMode:
                SetMode(FCCMode);
                break;

            default:
                SetMode(MfdMenu);
                break;
        }
    }

    // For in cockpit, you can't have two of the same thing
    if (mode not_eq MfdOff)
    {
        if (id == 0)
        {
            if (MfdDisplay[1]->mode == mode)
            {
                NextMode();
            }
        }
        else if (id == 1)
        {
            if (MfdDisplay[0]->mode == mode)
            {
                NextMode();
            }
        }
    }
}

// the other MFD wants this mode.
void MFDClass::StealMode(MfdMode mode)
{
    if (mode == MfdOff) return;

    for (int i = 0; i < 3; i++)
    {
        if (primarySecondary[curmm][i] == mode)
        {
            if (i == cursel[curmm])
            {
                SetNewMode(MfdOff);
            }

            primarySecondary[curmm][i] = MfdOff;
        }
    }
}

void MFDClass::SetMode(MfdMode newMode)
{
    if (ownship == NULL)
    {
        return;
    }

    MfdMode lastMode = mode;

    FreeDrawable();
    mode = newMode;

    switch (mode)
    {
        case FCRMode:
            drawable = FindSensor(ownship, SensorClass::Radar);
            break;

        case RWRMode:
            drawable = FindSensor(ownship, SensorClass::RWR);
            break;

        case SMSMode:
            drawable = ownship->FCC->Sms->drawable;
            break;

            // RV - I-Hawk - Replaced HUDMode with HADMode
            ///*case HUDMode:
            // drawable = TheHud;
            //break;*/

        case HADMode:
            drawable = HadDrawable;
            break;

        case FCCMode:
            drawable = ownship->FCC;
            break;

        case MfdMenu: // special case cos its not really a mode
            drawable = mfdDraw[MfdMenu];
            restoreMode = lastMode;
            ((MfdMenuDrawable*)mfdDraw[MfdMenu])->SetCurMode(lastMode);
            break;

        case MfdOff:
        case TFRMode:
        case TestMode:
        case FLIRMode:
        case DTEMode:
        case FLCSMode:
        case WPNMode:
        case TGPMode:
            drawable = mfdDraw[mode];
            break;

        default:
            ShiWarning("Bad MFD mode");
            drawable = NULL;
            break;
    }

    if (mode not_eq MfdMenu)
    {
        primarySecondary[curmm][cursel[curmm]] = mode;

        if (id == 0)
        {
            MfdDisplay[1]->StealMode(mode);
        }
        else if (id == 1)
        {
            MfdDisplay[0]->StealMode(mode);
        }
    }

    if (drawable)
    {
        drawable->SetMFD(id);
        drawable->viewPoint = viewPoint;

        if (mode not_eq MfdOff)
        {
            OTWDriver.SetPopup(id, TRUE);
        }
        else
        {
            OTWDriver.SetPopup(id, FALSE);
        }
    }
    else
    {
        OTWDriver.SetPopup(id, FALSE);
    }
}

void MFDClass::FreeDrawable()
{
    int i;
    int inUse = FALSE;

    // Don't bother if we don't have a drawable and a private display
    if (drawable and drawable->GetDisplay())
    {
        if (mode not_eq MfdOff)
        {
            for (i = 0; i < NUM_MFDS; i++)
            {
                if (MfdDisplay[i] and (this not_eq MfdDisplay[i]))
                {
                    if (MfdDisplay[i]->mode == mode)
                    {
                        inUse = TRUE;
                        break;
                    }
                }
            }
        }

        if ( not inUse)
        {
            drawable->DisplayExit();
        }

        // Drop our reference, even if someone else is drawing
        // NOTE:: This may cause a leak if no-one calls display
        // exit.
        mode = MfdOff;
    }

    drawable = NULL;
}

void MFDClass::ButtonPushed(int whichButton, int whichMFD)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (playerAC and playerAC->IsSetFlag(MOTION_OWNSHIP))
    {
        if (drawable)
        {
            drawable->PushButton(whichButton, whichMFD);
        }
    }
}

void MFDClass::SetNewMasterMode(int n)
{
    ShiAssert(n >= 0 and n < MAXMM);
    changeMode = TRUE_MODESWITCH;
    newmm = n;
}

void MFDClass::SetNewMode(MfdMode newMfdMode)
{

    newMode = newMfdMode;
    changeMode = TRUE_ABSOLUTE;
}

void MFDClass::SetNewModeAndPos(int n, MfdMode newMfdMode)
{
    ShiAssert(n >= 0 and n < 3);
    cursel[curmm] = n;
    newMode = newMfdMode;
    changeMode = TRUE_ABSOLUTE;
}

// MD -- 20040727: utility function to clear the cockpitFlightData MFD OSB labels which
// we need to do on MFD display mode switch.

void ClearFlightDataOsbLabels()
{
    for (int i = 0; i < 20; i++)
    {
        *(cockpitFlightData.leftMFD[i].line1) = '\0';
        *(cockpitFlightData.leftMFD[i].line2) = '\0';
        cockpitFlightData.leftMFD[i].inverted = FALSE;
    }

    for (int i = 0; i < 20; i++)
    {
        *(cockpitFlightData.rightMFD[i].line1) = '\0';
        *(cockpitFlightData.rightMFD[i].line2) = '\0';
        cockpitFlightData.rightMFD[i].inverted = FALSE;
    }
}


void MFDClass::Exec(int clearFrame, int virtualCockpit)
{
    float vpLeft, vpTop, vpRight, vpBottom;

    // MD -- 20040727: any time we change the content of the MFD display page
    // the shared memory copies of the OSB label text neds to be blanked so that
    // stale strings from the last mode aren't left by the drawing code for the
    // new mode which only writes the labels for the mode we are changing to now.
    if (changeMode not_eq FALSE)
    {
        ClearFlightDataOsbLabels();
    }

    if (changeMode == TRUE_NEXT)
    {
        NextMode();
        changeMode = FALSE;
    }
    else if (changeMode == TRUE_ABSOLUTE)
    {
        SetMode(newMode);
        changeMode = FALSE;
    }
    else if (changeMode == TRUE_MODESWITCH)
    {
        curmm = newmm;
        SetMode(primarySecondary[curmm][cursel[curmm]]);
        changeMode = FALSE;
    }

    if (virtualCockpit)
    {
        if (drawable)
        {
            if (drawable->GetDisplay())
            {
                drawable->GetDisplay()->SetRttCanvas(&cUL, &cUR, &cLL, cBlend, cAlpha);
                drawable->GetDisplay()->SetRttRect(tLeft, tTop, tRight, tBottom);
            }
        }
    }

    if (ownship and ownship->mFaults and 
        (
            (ownship->mFaults->GetFault(FaultClass::flcs_fault) bitand FaultClass::dmux) or
            ownship->mFaults->GetFault(FaultClass::dmux_fault) or
            ((ownship->mFaults->GetFault(FaultClass::mfds_fault) bitand FaultClass::lfwd) and (id % 2 == 0)) or
            ((ownship->mFaults->GetFault(FaultClass::mfds_fault) bitand FaultClass::rfwd) and (id % 2 == 1))
        )
       )
    {
        return;
    }

    if ( not ownship->HasPower(AircraftClass::MFDPower))
        return;

    // ASSO: rewritten the virtual cockpit branch
    if (virtualCockpit)
    {
        if (drawable)
        {
            drawable->SetMFD(id);

            if (drawable->GetDisplay())
            {
                if (((Render2D*)(drawable->GetDisplay()))->GetImageBuffer() not_eq image)
                    ((Render2D*)(drawable->GetDisplay()))->SetImageBuffer(image);

                ((Render2D*)(drawable->GetDisplay()))->GetViewport(&vpLeft, &vpTop, &vpRight, &vpBottom);
                ((Render2D*)(drawable->GetDisplay()))->SetViewport(vTop, vLeft, vBottom, vRight);
                //OTWDriver.SetPopup(id, TRUE);

                drawable->GetDisplay()->AdjustRttViewport();

                drawable->GetDisplay()->StartDraw();

                if (g_b3DRTTCockpitDebug)
                {
                    //ATARIBABY debug frame around surface
                    drawable->GetDisplay()->SetColor(0x0000ffff);
                    drawable->GetDisplay()->Line(-0.995F, -0.995F, 0.995F, -0.995F);
                    drawable->GetDisplay()->Line(-0.995F, 0.995F, 0.995F, 0.995F);
                    drawable->GetDisplay()->Line(-0.995F, 0.995F, -0.995F, -0.995F);
                    drawable->GetDisplay()->Line(0.995F, 0.995F, 0.995F, -0.995F);
                }

                drawable->SetIntensity(GetIntensity());   // JPO set intensity
                drawable->GetDisplay()->SetColor(Color());

                if (mode == RWRMode)
                {
                    if (g_bEnableMfdColors)  // a.s. begin
                    {
                        drawable->GetDisplay()->SetColor(0xff00ff00);
                    } // a.s. end

                    ((PlayerRwrClass*)drawable)->SetGridVisible(TRUE);
                }

                drawable->Display(drawable->GetDisplay());

                drawable->GetDisplay()->EndDraw();

                ((Render2D*)(drawable->GetDisplay()))->SetViewport(vpLeft, vpTop, vpRight, vpBottom);
            }
            else
            {
                OTWDriver.renderer->FinishRtt();
                drawable->DisplayInit(image);

                OTWDriver.renderer->StartRtt(OTWDriver.renderer);
                drawable->GetDisplay()->SetRttCanvas(&cUL, &cUR, &cLL, cBlend, cAlpha);
                drawable->GetDisplay()->SetRttRect(tLeft, tTop, tRight, tBottom);
                drawable->GetDisplay()->AdjustRttViewport();


            }
        }
    }
    else
    {
        if (drawable and not (mode == MfdOff and clearFrame))
        {
            drawable->SetMFD(id);

            if (drawable->GetDisplay())
            {
                if (((Render2D*)(drawable->GetDisplay()))->GetImageBuffer() not_eq image)
                {
                    ((Render2D*)(drawable->GetDisplay()))->SetImageBuffer(image);
                }

                // ASSO: reset the viewport to the main buffer size
                ((Render2D*)(drawable->GetDisplay()))->ResetRttViewport();

                ((Render2D*)(drawable->GetDisplay()))->GetViewport(&vpLeft, &vpTop, &vpRight, &vpBottom);
                ((Render2D*)(drawable->GetDisplay()))->SetViewport(vTop, vLeft, vBottom, vRight);
                OTWDriver.SetPopup(id, TRUE);
                drawable->GetDisplay()->StartDraw();

                if (clearFrame)
                {
                    if (g_bEnableMfdColors) // a.s. begin - makes Mfds transparent and colored
                    {
                        mycolor = (int)((g_fMfdRed / 100.0F) * 255.0F) + (int)((g_fMfdGreen / 100.0F) * 255.0F) * 0x100 + (int)((g_fMfdBlue / 100.0F) * 255.0F) * 0x10000 + (int)((g_fMfdTransparency / 100.0F) * 255.0F) * 0x1000000;

                        if (mode == FCCMode)
                        {
                            drawable->Display(drawable->GetDisplay());

                            OTWDriver.renderer->context.RestoreState(STATE_SOLID);
                            drawable->GetDisplay()->SetColor(mycolor);
                            OTWDriver.renderer->context.RestoreState(STATE_ALPHA_SOLID);
                            drawable->GetDisplay()->Tri(-1.0F, -1.0F, -1.0F, 1.0F, 1.0F, 1.0F);
                            drawable->GetDisplay()->Tri(-1.0F, -1.0F, 1.0F, -1.0F, 1.0F, 1.0F);

                            drawable->SetIntensity(0xffffffff);
                            drawable->GetDisplay()->SetColor(Color());

                            drawable->Display(drawable->GetDisplay()); // second Display is necessary for FCC
                            drawable->GetDisplay()->EndDraw();
                            ((Render2D*)(drawable->GetDisplay()))->SetViewport(vpLeft, vpTop, vpRight, vpBottom);  // EFOV bugfix

                            return;
                        }
                    } // a.s. end

                    drawable->GetDisplay()->SetColor(0xff000000);

                    if (g_bEnableMfdColors) // a.s. begin
                    {
                        OTWDriver.renderer->context.RestoreState(STATE_SOLID);
                        drawable->GetDisplay()->SetColor(mycolor);
                        OTWDriver.renderer->context.RestoreState(STATE_ALPHA_SOLID);
                    } // a.s. end

                    drawable->GetDisplay()->Tri(-1.0F, -1.0F, -1.0F, 1.0F, 1.0F, 1.0F);
                    drawable->GetDisplay()->Tri(-1.0F, -1.0F, 1.0F, -1.0F, 1.0F, 1.0F);
                }


                drawable->SetIntensity(GetIntensity()); // JPO set intensity
                drawable->GetDisplay()->SetColor(Color());

                if (mode == RWRMode)
                {
                    if (g_bEnableMfdColors)
                    {
                        drawable->GetDisplay()->SetColor(0xff00ff00);
                    }

                    ((PlayerRwrClass*)drawable)->SetGridVisible(TRUE);
                }

                drawable->Display(drawable->GetDisplay());
                drawable->GetDisplay()->EndDraw();
                ((Render2D*)(drawable->GetDisplay()))->SetViewport(vpLeft, vpTop, vpRight, vpBottom);
            }
            else
            {
                drawable->DisplayInit(image);
            }
        }
        else
        {
            OTWDriver.SetPopup(id, FALSE);
        }
    }
}

void MFDClass::Exit(void)
{
}

void MFDClass::SetImageBuffer(ImageBuffer* newImage, float top, float left,
                              float bottom, float right)
{
    Render2D* theDisplay;

    if (newImage == NULL)
        image = privateImage;
    else
        image = newImage;

    if (top == bottom)
    {
        vLeft = 1.0F - VirtualMFD[id].top / (DisplayOptions.DispHeight * 0.5F);
        vRight = 1.0F - VirtualMFD[id].bottom / (DisplayOptions.DispHeight * 0.5F);
        vTop = -1.0F + VirtualMFD[id].left / (DisplayOptions.DispWidth * 0.5F);
        vBottom = -1.0F + VirtualMFD[id].right / (DisplayOptions.DispWidth * 0.5F);
    }
    else
    {
        vTop = top;
        vBottom = bottom;
        vLeft = left;
        vRight = right;
    }

    if (drawable)
    {
        theDisplay = (Render2D*)drawable->GetDisplay();

        if (theDisplay)
        {
            if (theDisplay->GetImageBuffer() not_eq image)
                theDisplay->SetImageBuffer(image);

            //         theDisplay->SetViewport(vTop, vLeft, vBottom, vRight);
        }
    }
}

void MFDClass::IncreaseBrightness()
{
    intensity = max(intensity - 1, 0);
}

void MFDClass::DecreaseBrightness()
{
    intensity = min(intensity + 1, NUM_MFD_INTENSITIES - 1);
}

// JPO Default color - green tempered by mask
int MFDClass::Color()
{
    return 0xff00 bitand MFDMasks[intensity];
}

// default intensity mask
unsigned int MFDClass::GetIntensity()
{
    return MFDMasks[intensity];
}

void MFDSwapDisplays()
{
    DrawableClass* tmpDrawable = MfdDisplay[0]->GetDrawable();
    MFDClass::MfdMode tmpMode = MfdDisplay[0]->mode;

    MfdDisplay[0]->SetDrawable(MfdDisplay[1]->GetDrawable());
    MfdDisplay[0]->mode = MfdDisplay[1]->mode;

    MfdDisplay[1]->SetDrawable(tmpDrawable);
    MfdDisplay[1]->mode = tmpMode;

    tmpMode = MfdDisplay[0]->restoreMode;
    MfdDisplay[0]->restoreMode = MfdDisplay[1]->restoreMode;
    MfdDisplay[1]->restoreMode = tmpMode;

    for (int mm = 0; mm < MFDClass::MAXMM; mm++)
    {
        int tmp = MfdDisplay[0]->cursel[mm];
        MfdDisplay[0]->cursel[mm] = MfdDisplay[1]->cursel[mm];
        MfdDisplay[1]->cursel[mm] = tmp;

        for (int i = 0; i < 3; i++)
        {
            tmpMode = MfdDisplay[0]->primarySecondary[mm][i];
            MfdDisplay[0]->primarySecondary[mm][i] = MfdDisplay[1]->primarySecondary[mm][i];
            MfdDisplay[1]->primarySecondary[mm][i] = tmpMode;
        }
    }

    int t = MfdDisplay[0]->curmm;
    MfdDisplay[0]->curmm = MfdDisplay[1]->curmm;
    MfdDisplay[1]->curmm = t;

    if (MfdDisplay[0]->GetDrawable())
    {
        MfdDisplay[0]->GetDrawable()->SetMFD(0);
    }

    if (MfdDisplay[1]->GetDrawable())
    {
        MfdDisplay[1]->GetDrawable()->SetMFD(1);
    }
}

// Drawable Support
MfdDrawable::~MfdDrawable(void)
{
}

void MfdDrawable::DisplayInit(ImageBuffer* image)
{
    DisplayExit();

    privateDisplay = new Render2D;
    ((Render2D*)privateDisplay)->Setup(image);

    privateDisplay->SetColor(0xff00ff00);
}

void MfdDrawable::PushButton(int whichButton, int whichMFD)
{
    //MI
    FireControlComputer *FCC = MfdDisplay[whichMFD]->GetOwnShip()->GetFCC();
    ShiAssert(FCC not_eq NULL);

    MFDClass::MfdMode mode;
    int otherMfd = 1 - whichMFD;

    switch (whichButton)
    {
        case 10:
            break;

        case 11:
        case 12:
        case 13:
            mode = MfdDisplay[whichMFD]->GetSP(whichButton - 11);

            if (mode == MfdDisplay[whichMFD]->mode)
                mode = MFDClass::MfdMenu;

            // Check other MFD if needed;
            if (mode == MFDClass::MfdOff or otherMfd < 0 or MfdDisplay[otherMfd]->mode not_eq mode)
                MfdDisplay[whichMFD]->SetNewModeAndPos(whichButton - 11, mode);

            break;

        case 14:
            MFDSwapDisplays();
            break;
    }
}

void MfdDrawable::DefaultLabel(int button)
{
    int id = OnMFD();
    MFDClass *mfd = MfdDisplay[id];

    //MI
    FireControlComputer *FCC = MfdDisplay[id]->GetOwnShip()->GetFCC();
    ShiAssert(FCC not_eq NULL);

    switch (button)
    {
        case 10:
            if (mfd->mode == MFDClass::SMSMode)
            {
                LabelButton(button, "S-J");
            }
            else
                LabelButton(button, "DCLT");

            break;

        case 11:
            LabelButton(button, MFDClass::ModeName(mfd->GetSP(0)),
                        NULL, mfd->cursel[mfd->curmm] == 0);
            break;

        case 12:
            LabelButton(button, MFDClass::ModeName(mfd->GetSP(1)),
                        NULL, mfd->cursel[mfd->curmm] == 1);
            break;

        case 13:
            LabelButton(button, MFDClass::ModeName(mfd->GetSP(2)),
                        NULL, mfd->cursel[mfd->curmm] == 2);
            break;

        case 14:
            LabelButton(button, "SWAP");
            break;
    }
}

void MfdDrawable::BottomRow()
{
    for (int i = 10; i < 15; i++)
        DefaultLabel(i);
}

/*
 * JPO Draw a -\/-\/- type thingy
 */
void MfdDrawable::DrawReference(AircraftClass *self)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    const float yref = -0.8f;
    const float xref = -0.9f;
    //const float deltax[] = { 0.05f,  0.05f, 0.05f, 0.03f,  0.05f, 0.05f, 0.05f };
    //MI tweaked values
    const float deltax[] = { 0.05f,  0.02f, 0.02f, 0.005f,  0.02f, 0.02f, 0.05f };
    const float deltay[] = { 0.00f, -0.05f, 0.05f, 0.00f, -0.05f, 0.05f, 0.00f };
    const float RefAngle = 45.0f * DTR;
    float x = xref, y = yref;

    float offset = 0.0f;

    ShiAssert(self not_eq NULL and TheHud not_eq NULL);
    FireControlComputer *FCC = self->GetFCC();
    ShiAssert(FCC not_eq NULL);

    switch (FCC->GetMasterMode())
    {
        case FireControlComputer::AirGroundBomb:
        case FireControlComputer::AirGroundRocket: // MLR 4/3/2004 -
        case FireControlComputer::AirGroundLaser:
        case FireControlComputer::AirGroundCamera:
            if (FCC->inRange)
            {
                offset = FCC->airGroundBearing / RefAngle;
            }

            break;

        case FireControlComputer::Dogfight:
        case FireControlComputer::MissileOverride:
        case FireControlComputer::AAGun:
            //case (FireControlComputer::Gun and FCC->GetSubMode() not_eq FireControlComputer::STRAF):
        {
            //MI target
            RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(playerAC, SensorClass::Radar);

            if (theRadar and theRadar->CurrentTarget() and theRadar->CurrentTarget()->BaseData() and 
 not FCC->IsAGMasterMode())
            {
                float   dx, dy, xPos = 0.0F, tgtx, yPos = 0.0F;
                vector  collPoint;//newTarget->localData
                theRadar->TargetToXY(theRadar->CurrentTarget()->localData, 0,
                                     theRadar->GetDisplayRange(), &tgtx, &yPos);

                if (FindCollisionPoint((SimBaseClass*)theRadar->CurrentTarget()->BaseData(),
                                       self, &collPoint))
                {
                    collPoint.x -= self->XPos();
                    collPoint.y -= self->YPos();
                    collPoint.z -= self->ZPos();

                    dx = collPoint.x;
                    dy = collPoint.y;

                    xPos = (float)atan2(dy, dx) - self->Yaw();

                    if (xPos > (180.0F * DTR))
                        xPos -= 360.0F * DTR;
                    else if (xPos < (-180.0F * DTR))
                        xPos += 360.0F * DTR;
                }

                /*---------------------------*/
                /* steering point screen x,y */
                /*---------------------------*/
                if (fabs(xPos) < (60.0F * DTR))
                {
                    xPos = xPos / (60.0F * DTR);
                    xPos += tgtx;

                    offset = xPos;
                    offset -= TargetAz(self, theRadar->CurrentTarget()->BaseData()->XPos(),
                                       theRadar->CurrentTarget()->BaseData()->YPos());
                }
                else if (xPos > (60 * DTR))
                    offset = 1;
                else
                    offset = -1;
            }
        }
        break;

        case FireControlComputer::Nav:
        case FireControlComputer::ILS:
        default: //Catch all the other stuff
            if (TheHud->waypointValid)
            {
                offset = TheHud->waypointBearing / RefAngle;
            }

            break;
    }

    offset = min(max(offset, -1.0F), 1.0F);

    for (int i = 0; i < sizeof(deltax) / sizeof(deltax[0]); i++)
    {
        display->Line(x, y, x + deltax[i], y + deltay[i]);
        x += deltax[i];
        y += deltay[i];
    }

    float xlen = (x - xref);
    x = xref + xlen / 2 + offset * xlen / 2.0f;

    if (g_bINS)
    {
        if (playerAC and not playerAC->INSState(AircraftClass::INS_HSD_STUFF))
            return;
    }

    display->Line(x, yref + 0.086f, x, yref - 0.13f);
}

//Booster 12/09/2004 - Draw red PullUp cross in MFD if ALT_LOW warning is on
void MfdDrawable::DrawRedBreak(VirtualDisplay* display)
{
    int tmpColor = display->Color();
    int tmpWarnflash = (vuxRealTime bitand 0x080);

    if (tmpWarnflash)
    {
        display->SetColor(GetMfdColor(MFD_RED));
        display->Line(-0.634F, -0.64F, 0.64F, 0.634F);
        display->Line(-0.64F, -0.634F, 0.634F, 0.64F);
        display->Line(0.64F, -0.634F, -0.634F, 0.64F);
        display->Line(0.634F, -0.64F, -0.64F, 0.634F);
        display->SetColor(tmpColor);
    }
}

// RV - I-Hawk - The TGP attitude warning display function
void MfdDrawable::TGPAttitudeWarning(VirtualDisplay* display)
{
    int tmpColor = display->Color();
    int tempFont = display->CurFont();
    int tmpWarnflash = (vuxRealTime bitand 0x100);

    if (tmpWarnflash)
    {
        display->SetFont(2);
        display->SetColor(GetMfdColor(MFD_RED));

        static const float warningSide = 0.7f;

        // Many lines to create the painted box...
        for (float warningBottom = -0.18f; warningBottom < 0.25f; warningBottom += 0.0025f)
        {
            display->Line(-warningSide,   warningBottom,  warningSide,  warningBottom);
        }

        display->SetFont(1);
        display->SetColor(0xFF000000); // Black text
        display->TextCenter(0.4f, 0.16f, "CHECK");
        display->TextCenter(-0.35f, 0.16f, "CHECK");
        display->TextCenter(0.42f, 0.06f, "ATTITUDE");
        display->TextCenter(-0.37f, 0.06f, "ATTITUDE");
    }

    display->SetFont(tempFont);
    display->SetColor(tmpColor);
}

