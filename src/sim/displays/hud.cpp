#include "stdhdr.h"
#include <float.h>
#include "hud.h"
#include "Graphics/DXEngine/DXEngine.h"
#include "Graphics/DXEngine/DXVBManager.h"
#include "guns.h"
#include "aircrft.h"
#include "fcc.h"
#include "object.h"
#include "airframe.h"
#include "otwdrive.h"
#include "playerop.h"
#include "Graphics/Include/RenderOW.h"
#include "Graphics/Include/Mono2d.h"
#include "simdrive.h"
#include "atcbrain.h"
#include "campbase.h"
#include "ptdata.h"
#include "simfeat.h"
#include "mfd.h"
#include "camp2sim.h"
#include "fack.h"
#include "cpmanager.h"
#include "objectiv.h"
#include "fsound.h"
#include "soundfx.h"
#include "smsdraw.h"
#include "flightData.h"
#include "fack.h"
#include "lantirn.h"
#include "missile.h"
#include "radardoppler.h"
#include "mlTrig.h"
#include "statestack.h"
#include "weather.h"

#include "graphics/include/tod.h"  // MD -- 20040108: addded for dynamic HUD lighting


//MI for ICP stuff
#include "icp.h"
extern bool g_bRealisticAvionics;
extern bool g_bNoRPMOnHud;
extern bool g_bINS;
extern bool g_bNewPitchLadder;
extern float g_fReconCameraHalfFOV;
extern float g_fReconCameraOffset;
extern bool g_bBrightHUD;
//HUD Fixes on/off switch. Smeghead 14-Oct-2003.
extern bool g_bHUDFix;
extern float g_fHUDonlySize;

//TJL 11/09/03 Enable HUD AOA
extern bool g_bhudAOA;
extern bool shootCue; //TJL 01/28/04

void GetBullseyeToOwnship(char *string);   //Wombat778 10-16-2003
// For HUD coloring
static DWORD HUDcolor[] =
{
    0xff00ff00,
    0xff0000ff,
    0xffff0000,
    0xffffff00,
    0xffff00ff,
    0xff00ffff,
    // 0xff7f0000,
    // 0xff007f00,
    0xff00007f,
    // 0xff7f7f00,
    // 0xff007f7f,
    // 0xff7f007f,
    // 0xff7f7f7f,
    0xfffffffe,
    // 0xff222222,
    // 0xff00bf00,

    /* r,g,b,a
     0x00ff00ff,
     0xff0000ff,
     0x0000ffff,
     0x00ffffff,
     0xff00ffff,
     0xffff00ff,
     0x00007fff,
     0x007f00ff,
     0x7f0000ff,
     0x007f7fff,
     0x7f7f00ff,
     0x7f007fff,
     0x7f7f7fff,
     0xfeffffff,
     0x222222ff,
     0x00bf00ff,
    */
};
const int NumHudColors = sizeof(HUDcolor) / sizeof(HUDcolor[0]);
int curColorIdx;

Pcolor HudClass::hudColor;

int HudClass::flash = FALSE;
//MI
int HudClass::Warnflash = FALSE;

//Note: DrawWindowString decrements by 1
//Adjust up all DrawWindowString calls by 1
float hudWinX[NUM_WIN] =
{
    -0.15F,  -0.62F, -0.7F,  -0.7F,   -0.7F,   // 0..4     // sfr: airspeed C correction (index 1, from -.70 to -.62)
    0.55F,  -0.8F,  -0.8F,   0.0F,    0.55F,  // 5..9
    -0.0F,   -0.0F,   0.55F,  0.55F,  -0.9F,   //10..14
    0.0F,    0.0F,   0.0F,   0.65F,  -0.5F,   //15..19
    -0.5F,   -0.5F,  -0.5F,  -0.5F,    0.55F,  //20..24 //MI tweaked
    0.55F,  -0.3F,  -0.20F, -0.175F, -0.65F,  //25..29
    0.0F,    0.52F,  0.35F,  0.55F,  -0.7F,   //30..34
    -0.9F,    0.52F,  0.65F, -0.70F,   0.65F,  //35..39   // sfr: altitude correction (index 39, from .65 to 50) // RED - RESTORED
    -0.325F, -0.325F, 0.56F, -0.3F,   -0.05F,  //40..44
    0.8F,   -0.65F, -0.75F, -0.75F,  -0.65F,  //45-49 TJL 03/07/04
    0.8F,   -0.8F,  -0.90F,  -0.8F,   -0.8F   //50-54
};

float hudWinY[NUM_WIN] =
{
    0.80F,   0.23F, -0.15F, -0.23F,   0.55F,  // 0..4 //MI value 2 from 0.20 to 0.25 and value 5 from 0.45 to 0.55
    0.23F,  -0.31F, -0.39F,  0.0F,   -0.39F,  // 5..9 //MI value 1 from 0.19 to -0.25
    0.2F,    0.05F, -0.47F, -0.55F,  -0.47F,  //10..14
    0.0F,    0.0F,   0.0F,   0.68F,  -0.45F,  //15..19
    -0.53F,  -0.61F, -0.69F, -0.78F,  -0.31F,  //20..24 DED stuff
    -0.20F,   0.32F, -0.03F, -0.11F,  -0.07F,  //25..29
    0.0F,   -0.06F,  0.21F,  0.51F,  -0.55F,  //30..34
    -0.63F,  -0.13F,  0.60F, -0.1F,   -0.1F,   //35..39
    0.85F,  -0.3F,   0.03F, -0.3F,    0.60F,  //40..44
    0.05F,  -0.13F, -0.23F, -0.3F,   -0.37F,  //45 (F18VVI) 46 AOA 47 Mach 48 G  49 GMax
    0.07F,   0.07F, -0.02F,  0.07F,   0.07F   //50 F14/F15 VVI location, AOA, F15 TAS,
};

float hudWinWidth[NUM_WIN] =
{
    0.1F,    0.05F,  0.15F,  0.15F,   0.15F,
    0.05F,   0.25F,  0.25F,  0.0F,    0.25F,
    0.65F,   0.3F,   0.25F,  0.25F,   0.35F,
    0.0F,    0.0F,   0.0F,   0.25F,   1.0F,
    1.0F,    1.0F,   1.0F,   1.0F,    0.3F,
    0.3F,    0.6F,   0.4F,   0.35F,   0.1F,
    0.0F,    0.1F,   0.2F,   0.15F,   0.15F,
    0.35F,   0.1F,   0.25F,  0.08F,   0.08F,   // 35-39, sfr: corrected width for caret (idx 38, .15 from to .08), same for alttiude (idx 39)
    0.65F,   0.65F,  0.06F,  0.5F,    0.1F,   // 40- 44
    0.15F,   0.15F,  0.15F,  0.15F,   0.15F,  //45-49
    0.15F,   0.15F,  0.15F,  0.15F,   0.15F,  //50 - 54
};

float hudWinHeight[NUM_WIN] =
{
    0.06F,   0.06F, 0.06F,   0.06F,   0.06F,
    0.06F,   0.06F, 0.06F,   0.0F,    0.06F,
    0.1F,    0.06F, 0.06F,   0.06F,   0.06F,
    0.0F,    0.0F,  0.0F,    0.06F,   0.06F,
    0.06F,   0.06F, 0.06F,   0.06F,   0.06F,
    0.06F,   0.07F, 0.06F,   0.06F,   0.06F,
    0.0F,    0.06F, 0.06F,   0.06F,   0.06F,
    0.06F,   0.06F, 0.06F,   0.55F,   0.55F,
    0.1F,    0.1F,  0.45F,   1.7F,    0.05F,
    0.55F,   0.06F, 0.06F,   0.06F,   0.06F,//45-49
    0.06F,   0.06F, 0.06F,   0.06F,   0.06F//50-54
};

char *hudNumbers[101] =
{
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "10", "11", "12", "13", "14", "15", "16", "17", "18", "19",
    "20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
    "30", "31", "32", "33", "34", "35", "36", "37", "38", "39",
    "40", "41", "42", "43", "44", "45", "46", "47", "48", "49",
    "50", "51", "52", "53", "54", "55", "56", "57", "58", "59",
    "60", "61", "62", "63", "64", "65", "66", "67", "68", "69",
    "70", "71", "72", "73", "74", "75", "76", "77", "78", "79",
    "80", "81", "82", "83", "84", "85", "86", "87", "88", "89",
    "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "100"
};

HudClass *TheHud = NULL;

HudClass::HudClass(void) : DrawableClass()
{
    headingPos      = Low;
    ownship         = NULL;
    HudData.tgtId   = -1;
    // RV - Biker
    //lowAltWarning   = 10.0F;//Cobra this thing is soo annoying on takeoff.  From 300 to 10
    lowAltWarning   = 300.0F;
    maxGs           = 0.0F;

    SetHalfAngle((float)atan(0.25F * tan(30.0F * DTR)) * RTD); //MI halfangle is degrees
    waypointX = 0.0F;
    waypointY = 0.0F;
    waypointZ = 0.0F;
    waypointRange = 0.0F;
    waypointArrival = 0.0F;
    waypointBearing = 0.0F;
    waypointRange = 0.0F;
    waypointAz = 0.0F;
    waypointEl = 0.0F;
    waypointValid = FALSE;
    //curRwy = NULL;
    curRwy = 0;

    SetScalesSwitch(VAH);
    SetFPMSwitch(ATT_FPM);
    SetVelocitySwitch(CAS);

    dedSwitch = DED_OFF;
    radarSwitch = RADAR_AUTO;
    brightnessSwitch = DAY;
    display = NULL;
    curColorIdx = 0;
    curHudColor = 0xff00ff00;
    SetHudColor(HUDcolor[curColorIdx]);
    //SYM Wheel
    SymWheelPos = 1.0F;
    ContWheelPos = 0.5f;
    SetLightLevel();
    targetPtr = NULL;
    targetData = NULL;
    eegsFrameNum = 0;
    lastEEGSstepTime = 0;
    pixelXCenter = 320.0F;
    pixelYCenter = 240.0F;
    // 2000-11-10 ADDED BY S.G. FOR THE Drift C/O switch
    driftCOSwitch = DRIFT_CO_OFF;
    // END OF ADDED SECTION
    //MI MSLFloor stuff
    MSLFloor = 10000;

    if (-cockpitFlightData.z >= MSLFloor)
        WasAboveMSLFloor = TRUE;
    else
        WasAboveMSLFloor = FALSE;

    TFAdv = 400;
    DefaultTargetSpan = 35.0f;
    WhichMode = 0;
    OA1Az = 0.0F;
    OA1Elev = 0.0F;
    OA1Valid = FALSE;
    OA2Az = 0.0F;
    OA2Elev = 0.0F;
    OA2Valid = FALSE;
    VIPAz = 0.0F;
    VIPElev = 0.0F;
    VIPValid = FALSE;
    VRPAz = 0.0F;
    VRPElev = 0.0F;
    VRPValid = FALSE;
    CalcRoll = CalcBank = TRUE;

    //added check for font so PW's pit looks good too
    if (OTWDriver.pCockpitManager->HudFont() == 1)
    {
        YRALTText = -0.17F;
        YALText = -0.24F;
    }
    else
    {
        YRALTText = -0.16F;
        YALText = -0.25F;
    }

    //MI check when to hide the funnel
    HideFunnel = FALSE;
    HideFunnelTimer = SimLibElapsedTime;
    ShowFunnelTimer = SimLibElapsedTime;
    SetHideTimer = FALSE;
    SetShowTimer = FALSE;

    SlantRange = 0.0F;
    RET_CENTER = hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F;
    ReticlePosition = 0; //for the wheel
    RetPos = 0; //controls where to move it

    sprintf(SpeedText, "");
    //MI init
    shootCue = 0;//TJL 01/28/04
    fpmConstrained = false; //TJL 03/11/04Made this accessible
    hudDelayTimer = SimLibElapsedTime;//Cobra
    hudAltDelayTimer = SimLibElapsedTime;//Cobra
    hudRAltDelayTimer = SimLibElapsedTime;//Cobra
    aspeedHud = 0; //Cobra
    altHud = 0.0f; //Cobra
    altHudn = 0.0f;
    raltHud = 0.0f; //Cobra
    vvid = 0; //Cobra
    HudBrightness = 1.0f; // COBRA - RED -
    AutoHudCx = 0.0f; // COBRA - RED -
}


HudClass::~HudClass(void)
{
    ClearTarget();
}

void HudClass::SetOwnship(AircraftClass* newOwnship)
{
    // Quit now if this is a redundant call
    if (ownship == newOwnship)
        return;

    // Drop any target we may have had
    ClearTarget();

    // Take the new ownship pointer
    ownship = newOwnship;

    // Initialize the gunsight and FCC pointer
    if (newOwnship)
    {
        FCC = ownship->FCC;
        ShiAssert(FCC);

        // Reset our history data to something reasonable for this platform.
        // TODO:  Could do a better job here by assuming level flight and calculating a false history...
        int i;

        for (i = 0; i < NumEEGSFrames; i++)
        {
            eegsFrameArray[i].time = SimLibElapsedTime - (NumEEGSFrames - i);
            eegsFrameArray[i].x = newOwnship->XPos();
            eegsFrameArray[i].y = newOwnship->YPos();
            eegsFrameArray[i].z = newOwnship->ZPos();
            eegsFrameArray[i].vx = 0.0f;
            eegsFrameArray[i].vy = 0.0f;
            eegsFrameArray[i].vz = 0.0f;
        }

        for (i = 0; i < NumEEGSSegments; i++)
        {
            funnel1X[i] = 0.0f;
            funnel1Y[i] = 0.0f;
            funnel2X[i] = 0.0f;
            funnel2Y[i] = 0.0f;
        }
    }
    else
    {
        FCC = NULL;
    }

    eegsFrameNum = 0;
    lastEEGSstepTime = 0;
}

void HudClass::SetTarget(SimObjectType* newTarget)
{
    if (newTarget == targetPtr)
        return;

    ClearTarget();

    if (newTarget)
    {
        ShiAssert(newTarget->BaseData() not_eq (FalconEntity*)0xDDDDDDDD);
        newTarget->Reference();
        targetPtr = newTarget;
        targetData = newTarget->localData;
    }
}

void HudClass::ClearTarget(void)
{
    if (targetPtr)
    {
        targetPtr->Release();
        targetPtr = NULL;
        targetData = NULL;
        shootCue = 0;//TJL 01/28/04
    }
}

void HudClass::DisplayInit(ImageBuffer* image)
{
    DisplayExit();

    privateDisplay = new Render2D;
    ((Render2D*)privateDisplay)->Setup(image);

    privateDisplay->SetColor(0xff00ff00);
}



void HudClass::Display(VirtualDisplay *newDisplay, bool gTranslucent)
{
    char tmpStr[240];
    mlTrig rollTrig;

    // We must have one to draw...
    ShiAssert(ownship);

    // Various ways to be broken
    if (ownship->mFaults and 
        (
            (ownship->mFaults->GetFault(FaultClass::flcs_fault) bitand FaultClass::dmux) or
            ownship->mFaults->GetFault(FaultClass::dmux_fault) or
            ownship->mFaults->GetFault(FaultClass::hud_fault)
        )
       )
    {
        //MI still allow STBY reticle for bombing
        if (FCC->GetSubMode() == FireControlComputer::MAN and WhichMode == 2)
            DrawMANReticle();

        return;
    }

    // JPO - check systems have power
    if ( not ownship->HasPower(AircraftClass::HUDPower))
    {
        return;
    }

    display = newDisplay;

    // COBRA - RED - ALPHA forced for HUD Display
    display->ForceAlpha = gTranslucent;
    // Do we draw flashing things this frame?
    flash = (vuxRealTime bitand 0x200);
    Warnflash = (vuxRealTime bitand 0x080);
    mlSinCos(&rollTrig, cockpitFlightData.roll);
    alphaHudUnits = RadToHudUnitsX(cockpitFlightData.alpha * DTR - cockpitFlightData.windOffset * rollTrig.sin);

    // 2000-11-10 MODIFIED BY S.G. TO HANDLE THE 'driftCO' switch
    // if (ownship->OnGround()) {
    if (ownship->OnGround() or driftCOSwitch == DRIFT_CO_ON)
    {
#if 1
        // With weight on wheels, the jet turns on "drift cutout" to keep the fpm centered on the HUD
        betaHudUnits = 0.0f;
#else
        // While I'm looking for the pitch ladder clipping bug...
        betaHudUnits = RadToHudUnitsY(cockpitFlightData.beta * DTR + cockpitFlightData.windOffset * rollTrig.cos);
#endif
    }
    else
    {
        betaHudUnits = RadToHudUnitsY(cockpitFlightData.beta * DTR + cockpitFlightData.windOffset * rollTrig.cos);
    }

    //MI
    if (g_bRealisticAvionics)
    {
        if ( not (WhichMode == 2 and FCC->GetSubMode() == FireControlComputer::MAN))
        {
            DrawBoresightCross();
            DrawAirspeed();
            DrawAltitude();

            // Marco Edit
            if (FCC and FCC->GetMasterMode() not_eq FireControlComputer::Dogfight)
                DrawHeading(); // Don't draw heading in Dogfight Mode
        }
    }
    else
    {
        DrawBoresightCross();
        DrawAirspeed();
        DrawAltitude();

        // Marco Edit
        if (FCC and FCC->GetMasterMode() not_eq FireControlComputer::Dogfight)
            DrawHeading(); // Don't draw heading in Dogfight Mode
    }

#if 0 // Turned off to evaluate easy mode HUD clutter level...  SCR 9/18/98
#ifdef _DEBUG
    {
        int i;
        int days, hours, minutes;
        double cur_time;

        cur_time = SimLibElapsedTime / SEC_TO_MSEC;
        days = FloatToInt32(cur_time / (3600.0F * 24.0F));
        cur_time -= days * 3600.0F * 24.0F;
        hours = FloatToInt32(cur_time / 3600.0F);
        cur_time -= hours * 3600.0F;
        minutes = FloatToInt32(cur_time / 60.0F);
        cur_time -= minutes * 60.0F;
        sprintf(tmpStr, "%3d-%02d:%02d:%05.2f", days, hours, minutes, (float)cur_time);
        display->TextRight(0.95F, 0.9F, tmpStr, 0);
        ShiAssert(strlen(tmpStr) < sizeof(tmpStr));

        sprintf(tmpStr, "T%-4d", HudData.tgtId);
        ShiAssert(strlen(tmpStr) < sizeof(tmpStr));
        display->TextLeft(-0.9F, 0.9F, tmpStr, 0);
        sprintf(tmpStr, "C%3d", ownship->Id().num_);
        ShiAssert(strlen(tmpStr) < sizeof(tmpStr));
        display->TextLeft(-0.9F, 0.8F, tmpStr, 0);

        sprintf(tmpStr, "S%3.0f", ownship->af->dbrake * 100.0F);
        ShiAssert(strlen(tmpStr) < sizeof(tmpStr));
        display->TextLeft(-0.9F, 0.6F, tmpStr, 0);

        for (i = 0; i < 10; i++)
            if (debstr[i][0])
                display->TextLeft(-0.8F, 0.8F - 0.1F * i, debstr[i], 0);
    }
#endif
#endif

    if (fpmSwitch == FPM_AUTO)
    {
        if (fabs(cockpitFlightData.pitch) > 10.0f * DTR)
        {
            DrawPitchLadder();
        }
        else
        {
            DrawHorizonLine();
        }
    }
    else if ((ownship and ownship->af and ownship->af->gearPos > 0.5F) or (fpmSwitch == ATT_FPM))
    {
        if (FCC and FCC->GetMasterMode() not_eq FireControlComputer::Dogfight)
            DrawPitchLadder();//me123 status ok. don't draw ladders in dogfight mode
    }

    if (IsSOI())
    {
        if (g_bRealisticAvionics)
            display->TextLeft(-0.8F, 0.65F, "*", 0);
        else
            display->TextLeft(-0.8F, 0.65F, "SOI", 0);
    }

    DrawAlphaNumeric();

    //TJL 03/07/04 Draw Other HUDs 04/17/04 Make non-F16 default to F18 HUD
    if (ownship->af->GetTypeAC() == 8 or ownship->af->GetTypeAC() == 9 or ownship->af->GetTypeAC() == 10)
    {
        DrawF18HUD();
    }
    else if (ownship->af->GetTypeAC() == 6 or ownship->af->GetTypeAC() == 7)
    {
        DrawF14HUD();
    }
    else if (ownship->af->GetTypeAC() == 3 or ownship->af->GetTypeAC() == 4 or ownship->af->GetTypeAC() == 5)
    {
        DrawF15HUD();
    }
    else if (ownship->af->GetTypeAC() == 12)
    {
        DrawA10HUD();
    }

    if (( not FCC->postDrop or flash) and 
        fpmSwitch not_eq FPM_OFF and 
        FCC and FCC->GetMasterMode() not_eq FireControlComputer::Dogfight) // JPO not show in DGFT
        DrawFPM();

    switch (FCC->GetMasterMode())
    {
        case FireControlComputer::AAGun:
            DrawTDBox();
            DrawGuns();
            break;

        case FireControlComputer::AGGun:
            DrawAirGroundGravity();
            break;
            /*
               case FireControlComputer::Gun:
                   if (FCC->GetSubMode() not_eq FireControlComputer::STRAF)
                {
                DrawTDBox();
                DrawGuns();
                   }
                   else
                DrawAirGroundGravity();
                  break;
            */

        case FireControlComputer::Dogfight:
            DrawDogfight();
            break;

        case FireControlComputer::MissileOverride:
            DrawMissileOverride();
            break;

        case FireControlComputer::Missile:
            DrawAirMissile();
            break;

        case FireControlComputer::AirGroundHARM:
            DrawHarm();
            break;

        case FireControlComputer::AirGroundMissile:
            DrawGroundMissile();
            break;

        case FireControlComputer::ILS:
            DrawILS();

            //MI
            if (OTWDriver.pCockpitManager)
            {
                if (OTWDriver.pCockpitManager->mpIcp->GetCMDSTR())
                    DrawCMDSTRG();
            }

            break;

        case FireControlComputer::Nav:
            DrawNav();
            break;

        case FireControlComputer::AirGroundBomb:
            DrawAirGroundGravity();
            break;

        case FireControlComputer::AirGroundRocket:
            if ( not g_bRealisticAvionics) // MLR 5/30/2004 -
            {
                DrawAirGroundRocket();
            }
            else
            {
                DrawAirGroundGravity();
            }

            break;

        case FireControlComputer::AirGroundLaser:
            DrawTargetingPod();
            break;

        case FireControlComputer::AirGroundCamera:
            DrawRPod();
            break;
    }

    // Check ground Collision
    if (Warnflash and ownship and ownship->mFaults->GetFault(alt_low))
    {
        display->AdjustOriginInViewport(0.0F, hudWinY[BORESIGHT_CROSS_WINDOW] +
                                        hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F);
        display->AdjustOriginInViewport(0.0F, MISSILE_RETICLE_OFFSET);
        display->Line(0.4F,  0.4F, -0.4F, -0.4F);
        display->Line(0.4F, -0.4F, -0.4F,  0.4F);
        display->AdjustOriginInViewport(0.0F, -MISSILE_RETICLE_OFFSET);
        display->AdjustOriginInViewport(0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
                                                hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
    }

    //MI removed RPM indication
    //M.N. we need RPM indication for Flightmodel testing
    if ( not g_bNoRPMOnHud/* or not g_bRealisticAvionics*/)
    {
        if (ownship)
        {
            sprintf(tmpStr, "RPM %3d", FloatToInt32(ownship->af->rpm * 100.0F + 0.99F));
            ShiAssert(strlen(tmpStr) < sizeof(tmpStr));
            display->TextLeft(-0.9F, -0.75F, tmpStr, 0);
        }
    }

    //TJL 11/08/03 HUD Alpha for aircraft with AOA in the HUD
    if ( not ownship->IsF16() and g_bhudAOA)
    {
        //TJL 11/10/03 HUD AOA Greek Letter Alpha
        display->Line(-0.95F, 0.74F, -0.95F, 0.76F);
        display->Line(-0.95F, 0.76F, -0.93F, 0.78F);
        display->Line(-0.93F, 0.78F, -0.91F, 0.78F);
        display->Line(-0.91F, 0.78F, -0.85F, 0.72F);
        display->Line(-0.95F, 0.74F, -0.93F, 0.72F);
        display->Line(-0.93F, 0.72F, -0.91F, 0.72F);
        display->Line(-0.91F, 0.72F, -0.85F, 0.78F);


        //TJL 11/08/03 HUD AOA numbers
        sprintf(tmpStr, "%02.1f", ownship->af->alpha);
        ShiAssert(strlen(tmpStr) < sizeof(tmpStr));
        display->TextLeft(-0.80F, 0.8F, tmpStr, 0);
    }

    //End Alpha

    //////////////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
    // COBRA - RED - HACK This forces a Context VB Flush to solve a loss of vertices
    // still have to find why
    /////////////////////////////////////////////////////////////////////////////////////

    display->TextCenter(0, 0, "", 0);

    // COBRA - RED - END OF HACK  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\


    display->ZeroRotationAboutOrigin();
    display->CenterOriginInViewport();

    // COBRA - RED - ALPHA forced for HUD Display
    display->ForceAlpha = gTranslucent = false;
}

//-------------------------------
// Scales Switch
//-------------------------------

int HudClass::GetScalesSwitch(void)
{

    return scalesSwitch;
}

void HudClass::SetScalesSwitch(ScalesSwitch state)
{
    if (PlayerOptions.GetAvionicsType() == ATEasy)
    {
        scalesSwitch = H;
    }
    else
    {
        scalesSwitch = state;
    }
}

void HudClass::CycleScalesSwitch(void)
{

    switch (scalesSwitch)
    {
        case VV_VAH:
            SetScalesSwitch(VAH);
            break;

        case VAH:
            SetScalesSwitch(SS_OFF);
            break;

        case SS_OFF:
            SetScalesSwitch(VV_VAH);
            break;

        default:
            SetScalesSwitch(VAH);
            break;
    }
}

//-------------------------------
// FPM Switch
//-------------------------------

int HudClass::GetFPMSwitch(void)
{

    return fpmSwitch;
}

void HudClass::SetFPMSwitch(FPMSwitch state)
{
    if (PlayerOptions.GetAvionicsType() == ATEasy)
    {
        fpmSwitch = FPM_AUTO;
    }
    else
    {
        fpmSwitch = state;
    }
}

void HudClass::CycleFPMSwitch(void)
{
    switch (fpmSwitch)
    {
        case ATT_FPM:
            SetFPMSwitch(FPM);
            break;

        case FPM:
            SetFPMSwitch(FPM_OFF);
            break;

        case FPM_OFF:
            SetFPMSwitch(ATT_FPM);
            break;

        default:
            SetFPMSwitch(ATT_FPM);
            break;
    }
}

// 2000-11-10 FUNCTIONS ADDED BY S.G. FOR THE Drift C/O switch
//-------------------------------
// Drift C/O Switch
//-------------------------------

int HudClass::GetDriftCOSwitch(void)
{

    return driftCOSwitch;
}

void HudClass::SetDriftCOSwitch(DriftCOSwitch state)
{

    driftCOSwitch = state;
}

void HudClass::CycleDriftCOSwitch(void)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (playerAC and playerAC->IsSetFlag(MOTION_OWNSHIP))
    {
        switch (driftCOSwitch)
        {
            case DRIFT_CO_ON:
                driftCOSwitch = DRIFT_CO_OFF;
                break;

            case DRIFT_CO_OFF:
                driftCOSwitch = DRIFT_CO_ON;
                break;
        }
    }
}

// END OF ADDED SECTION

//-------------------------------
// DED Switch
//-------------------------------

int HudClass::GetDEDSwitch(void)
{

    return dedSwitch;
}

void HudClass::SetDEDSwitch(DEDSwitch state)
{

    dedSwitch = state;
}

void HudClass::CycleDEDSwitch(void)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (playerAC and playerAC->IsSetFlag(MOTION_OWNSHIP))
    {
        //MI
        if ( not g_bRealisticAvionics)
        {
            switch (dedSwitch)
            {
                case DED_DATA:
                    dedSwitch = DED_OFF;
                    break;

                case DED_OFF:
                    dedSwitch = DED_DATA;
                    break;
            }
        }
        else
        {
            switch (dedSwitch)
            {
                case DED_OFF:
                    dedSwitch = PFL_DATA;
                    break;

                case PFL_DATA:
                    dedSwitch = DED_DATA;
                    break;

                case DED_DATA:
                    dedSwitch = DED_OFF;
                    break;

                default:
                    break;
            }
        }
    }
}

//-------------------------------
// Velocity Switch
//-------------------------------

int HudClass::GetVelocitySwitch(void)
{
    return velocitySwitch;
}

void HudClass::SetVelocitySwitch(VelocitySwitch state)
{
    if (PlayerOptions.GetAvionicsType() == ATEasy)
    {
        velocitySwitch = CAS;
    }
    else
    {
        velocitySwitch = state;
    }
}

void HudClass::CycleVelocitySwitch(void)
{

    switch (velocitySwitch)
    {
        case CAS:
            SetVelocitySwitch(TAS);
            break;

        case TAS:
            SetVelocitySwitch(GND_SPD);
            break;

        case GND_SPD:
            SetVelocitySwitch(CAS);
            break;
    }
}

//-------------------------------
// Radar Switch
//-------------------------------

int HudClass::GetRadarSwitch(void)
{

    return radarSwitch;
}

void HudClass::SetRadarSwitch(RadarSwitch state)
{

    radarSwitch = state;
}

void HudClass::CycleRadarSwitch(void)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (playerAC and playerAC->IsSetFlag(MOTION_OWNSHIP))
    {
        switch (radarSwitch)
        {
            case ALT_RADAR:
                radarSwitch = BARO;
                break;

            case BARO:
                radarSwitch = RADAR_AUTO;
                break;

            case RADAR_AUTO:
                radarSwitch = ALT_RADAR;
                break;
        }
    }
}

//-------------------------------
// Brightness Switch
//-------------------------------

int HudClass::GetBrightnessSwitch(void)
{

    return brightnessSwitch;
}

void HudClass::SetBrightnessSwitch(BrightnessSwitch state)
{

    brightnessSwitch = state;
}

// MD -- 20040108: changed cycling order to fix bug where cockpit and internal state were
// out of sequence.
void HudClass::CycleBrightnessSwitch(void)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (playerAC and playerAC->IsSetFlag(MOTION_OWNSHIP))
    {
        switch (brightnessSwitch)
        {
            case DAY:
                brightnessSwitch = NIGHT;
                break;

            case BRIGHT_AUTO:
                brightnessSwitch = DAY;
                break;

            case NIGHT:
                brightnessSwitch = BRIGHT_AUTO;
                break;

            default:
                brightnessSwitch = DAY;
                break;
        }

        SetLightLevel();
    }
}
//MI
void HudClass::CycleBrightnessSwitchUp(void)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (playerAC and playerAC->IsSetFlag(MOTION_OWNSHIP))
    {
        // MD -- 20040108: Comment out the power control since this switch doesn't do that
        //if( not playerAC->HasPower(AircraftClass::HUDPower))
        // playerAC->PowerOn(AircraftClass::HUDPower);
        switch (brightnessSwitch)
        {
                //case OFF:  // MD -- 20040108: commented out since there's no such state
                // brightnessSwitch = NIGHT;
                //break;
            case NIGHT:
                brightnessSwitch = BRIGHT_AUTO;
                break;

            case BRIGHT_AUTO:
                brightnessSwitch = DAY;
                break;

            default:
                break;
        }

        SetLightLevel();
    }
}
void HudClass::CycleBrightnessSwitchDown(void)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (playerAC and playerAC->IsSetFlag(MOTION_OWNSHIP))
    {
        switch (brightnessSwitch)
        {
            case DAY:
                brightnessSwitch = BRIGHT_AUTO;
                break;

            case BRIGHT_AUTO:
                brightnessSwitch = NIGHT;
                break;

                //case NIGHT:  // MD -- 20040108: commented out since there's no such state
                // brightnessSwitch = OFF;
                // playerAC->PowerOff(AircraftClass::HUDPower);
                //break;
            default:
                break;
        }

        SetLightLevel();
    }
}


void HudClass::DrawAlphaNumeric(void)
{
    //MI not here in BUP reticle mode
    if (g_bRealisticAvionics and WhichMode == 2 and FCC->GetSubMode() == FireControlComputer::MAN)
        return;

    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    char tmpStr[40];

    // Window 3 (Master Arm / ILS)
    //TJL 03/07/04 F16 specific or default HUD
    if (PlayerOptions.GetAvionicsType() not_eq ATEasy and (ownship->IsF16() or ownship->af->GetTypeAC() == 0))
    {
        if (FCC and FCC->GetMasterMode() == FireControlComputer::ILS)
            DrawWindowString(3, "ILS");
        else switch (ownship->Sms->MasterArm())
            {
                case SMSBaseClass::Safe:

                    //MI not here in real
                    if ( not g_bRealisticAvionics)
                        DrawWindowString(3, "SAF");

                    break;

                case SMSBaseClass::Arm:
                    DrawWindowString(3, "ARM");
                    break;

                case SMSBaseClass::Sim:
                    DrawWindowString(3, "SIM");
                    break;
            }
    }

    // Window 4 (Mach Number)
    //TJL 03/07/04 F16 specific or default HUD
    if (ownship->IsF16() or ownship->af->GetTypeAC() == 0)
    {
        float mach =  cockpitFlightData.mach;

        if (mach < 0.1f)
            mach = 0.1f;

        sprintf(tmpStr, "%.2f", mach);
        ShiAssert(strlen(tmpStr) < 40);

        if (FCC and FCC->GetMasterMode() not_eq FireControlComputer::Dogfight) //JPG 29 Apr 04 - Not here in DF override
        {
            DrawWindowString(4, tmpStr);
        }
    }

    // Window 5 (Gs)
    //MI (JPO - fixed elsewhere )
    //TJL 03/07/04 F16 specific or default HUD
    if (ownship->IsF16() or ownship->af->GetTypeAC() == 0)
    {
        sprintf(tmpStr, "%.1f", cockpitFlightData.gs);
        ShiAssert(strlen(tmpStr) < 40);
        DrawWindowString(5, tmpStr);
    }

    //TJL 03/07/04 F16 specific or default HUD
    if (PlayerOptions.GetAvionicsType() not_eq ATEasy and (ownship->IsF16() or ownship->af->GetTypeAC() == 0))
    {
        //MI changed for INS stuff
        if (g_bINS and g_bRealisticAvionics)
        {
            if ( not ownship->INSState(AircraftClass::INS_Aligned) and 
                ownship->INSState(AircraftClass::INS_AlignNorm) and (cockpitFlightData.kias <= 1.0F
                       and not ownship->INS60kts) or ownship->INSState(AircraftClass::INS_AlignFlight))
            {
                sprintf(tmpStr, "ALIGN");
            }
            else if (ownship->INSState(AircraftClass::INS_Aligned) and 
                     ownship->INSState(AircraftClass::INS_AlignNorm) and cockpitFlightData.kias <= 1.0F
                    and not ownship->INS60kts or ownship->INSState(AircraftClass::INS_AlignFlight))
            {
                if (flash)
                    sprintf(tmpStr, "ALIGN");
                else
                {
                    if (cockpitFlightData.gs > maxGs)
                        maxGs = cockpitFlightData.gs;

                    sprintf(tmpStr, "%.1f", maxGs);
                }
            }
            else
            {
                if (cockpitFlightData.gs > maxGs)
                    maxGs = cockpitFlightData.gs;

                sprintf(tmpStr, "%.1f", maxGs);
            }
        }
        else if (ownship->IsF16() or ownship->af->GetTypeAC() == 0)
        {
            // Window 7 (Max Gs)
            if (cockpitFlightData.gs > maxGs)
                maxGs = cockpitFlightData.gs;

            sprintf(tmpStr, "%.1f", maxGs);
        }

        ShiAssert(strlen(tmpStr) < 40);

        if (FCC and FCC->GetMasterMode() not_eq FireControlComputer::Dogfight) //JPG 29 Apr 04 - Not here in DF override
        {
            DrawWindowString(7, tmpStr);
        }
    }

    // Window 8 (Master/Sub Mode)
    if (ownship->Sms->drawable and ownship->Sms->drawable->DisplayMode() == SmsDrawable::SelJet)
        DrawWindowString(8, "JETT");
    //TJL F16 specific or default HUD
    else if (ownship->IsF16() or ownship->af->GetTypeAC() == 0)
    {
        //MI
        if ( not g_bRealisticAvionics)
        {
            if (ownship->Sms->NumCurrentWpn() > 0)
                sprintf(tmpStr, "%d %s", ownship->Sms->NumCurrentWpn(), FCC->subModeString);
            else
                sprintf(tmpStr, "%s", FCC->subModeString);
        }
        else
        {
            if (FCC and FCC->IsAGMasterMode())
            {
                if (ownship->Sms and ownship->Sms->curWeaponType == wtAgm65)
                {
                    if (ownship->Sms->MavSubMode == SMSBaseClass::PRE)
                        sprintf(tmpStr, "PRE");
                    else if (ownship->Sms->MavSubMode == SMSBaseClass::VIS)
                        sprintf(tmpStr, "VIS");
                    else
                        sprintf(tmpStr, "BORE");
                }
                else
                    sprintf(tmpStr, "%s", FCC->subModeString);
            }
            else
            {
                if (ownship->Sms->NumCurrentWpn() > 0)
                    sprintf(tmpStr, "%d %s", ownship->Sms->NumCurrentWpn(), FCC->subModeString);
                else
                    sprintf(tmpStr, "%s", FCC->subModeString);
            }
        }

        DrawWindowString(8, tmpStr);
    }

    // Window 10 (Range)
    // Done in missile or gun mode section

    // Window 11 (Master Caution)
    //MI Warn reset is correct
    int ofont = display->CurFont();
    display->SetFont(3);

    if ( not g_bRealisticAvionics)
    {
        if (ownship->mFaults->MasterCaution() and flash)
        {
            DrawWindowString(11, "WARN");
        }
    }
    else
    {
        if (ownship->mFaults->WarnReset() and Warnflash)
        {
            //Fuel doesn't flash warning
            if ( not ownship->mFaults->GetFault(fuel_low_fault) and 
 not ownship->mFaults->GetFault(fuel_trapped) and 
 not ownship->mFaults->GetFault(fuel_home))
                DrawWindowString(11, "WARN");
        }
    }

    display->SetFont(ofont);

    // Window 12, 15 (Bingo, trapped or home Fuel) JPO additions


    if (ownship->mFaults->GetFault(fuel_low_fault) or
        ownship->mFaults->GetFault(fuel_trapped) or
        ownship->mFaults->GetFault(fuel_home))
    {
        //MI Warn Reset is correct
        if ( not g_bRealisticAvionics)
        {
            if (ownship->mFaults->MasterCaution() and flash)
                DrawWindowString(12, "FUEL");
        }
        else
        {
            if (ownship->mFaults->WarnReset() and Warnflash)
                DrawWindowString(12, "FUEL");
        }

        // RV - RED - REWRITTEN THIS STUFF in a decent way
        if (PlayerOptions.GetAvionicsType() not_eq ATEasy)
        {
            char tempstr[10] = "";

            // Check various Fuel Situations
            if (ownship->mFaults->GetFault(fuel_low_fault)) sprintf(tempstr, "FUEL");

            if (ownship->mFaults->GetFault(fuel_trapped))  sprintf(tempstr, "TRP FUEL");

            if (ownship->mFaults->GetFault(fuel_home)) sprintf(tempstr, "FUEL %03d", ownship->af->HomeFuel / 100);

            // if any warn, draw it
            if (tempstr[0]) DrawWindowString(15, tempstr);
        }

        //MI warn reset is correct
        if ( not g_bRealisticAvionics)
        {
            if (ownship->mFaults->GetFault(fuel_low_fault) and ownship->mFaults->MasterCaution() and 
                F4SoundFXPlaying(ownship->af->GetBingoSnd()))  // JB 010425
            {
                F4SoundFXSetDist(ownship->af->GetBingoSnd(), FALSE, 0.0f, 1.0f);
            }
        }

        //MI fix for Bingo warning. This get's called in cautions.cpp
#if 0
        else
        {
            if (ownship->mFaults->GetFault(fuel_low_fault) and 
                ownship->mFaults->WarnReset() and 
                F4SoundFXPlaying(ownship->af->GetBingoSnd()))  // JB 010425
            {
                F4SoundFXSetDist(ownship->af->GetBingoSnd(), FALSE, 0.0f, 1.0f);
            }
        }

#endif
    }
    //Wombat778 10-16-2003 added as per MIRV  (draw bullseye info on hud)

    else if ((OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo) and (PlayerOptions.GetAvionicsType() not_eq ATEasy))
    {
        char tempstr[15] = "";
        GetBullseyeToOwnship(tempstr);
        DrawWindowString(15, tempstr);
    }


    // Window 13 (Closure)
    // Done in missile or gun section

    // Window 14 (Steerpoint)
    // Done in nav section

    // Window 19 (Variable)
    // Done in missile or gun section

    //MI they wanted DED data, so here it is.....
    //if (dedSwitch == DED_DATA and OTWDriver.GetOTWDisplayMode() == OTWDriverClass::ModeHud)
    if (dedSwitch == DED_DATA or dedSwitch == PFL_DATA)// and OTWDriver.GetOTWDisplayMode() == OTWDriverClass::ModeHud)
    {
        if (OTWDriver.pCockpitManager)
        {

            if ( not g_bRealisticAvionics and dedSwitch == DED_DATA)
            {
                char line1[40];
                char line2[40];
                char line3[40];

                OTWDriver.pCockpitManager->mpIcp->Exec();
                OTWDriver.pCockpitManager->mpIcp->GetDEDStrings(line1, line2, line3);

                DrawWindowString(21, line1);
                DrawWindowString(22, line2);
                DrawWindowString(23, line3);
            }
            else
            {
                if ( not playerAC->HasPower(AircraftClass::UFCPower) or
                    (FCC and FCC->GetMasterMode() == FireControlComputer::Dogfight))
                    return;

                OTWDriver.pCockpitManager->mpIcp->Exec();
                OTWDriver.pCockpitManager->mpIcp->ExecPfl();

                char line1[27] = "";
                char line2[27] = "";
                char line3[27] = "";
                char line4[27] = "";
                char line5[27] = "";
                line1[26] = '\0';
                line2[26] = '\0';
                line3[26] = '\0';
                line4[26] = '\0';
                line5[26] = '\0';
                static float xPos = -0.58F;  // JPG 16 Dec 03 - was -0.70F
                float yPos = -0.60F;

                for (int j = 0; j < 5; j++)
                {
                    for (int i = 0; i < 26; i++)
                    {
                        switch (j)
                        {
                            case 0:
                                if (dedSwitch == DED_DATA)
                                    line1[i] = OTWDriver.pCockpitManager->mpIcp->DEDLines[j][i];
                                else if (dedSwitch == PFL_DATA)
                                    line1[i] = OTWDriver.pCockpitManager->mpIcp->PFLLines[j][i];

                                break;

                            case 1:
                                if (dedSwitch == DED_DATA)
                                    line2[i] = OTWDriver.pCockpitManager->mpIcp->DEDLines[j][i];
                                else if (dedSwitch == PFL_DATA)
                                    line2[i] = OTWDriver.pCockpitManager->mpIcp->PFLLines[j][i];

                                break;

                            case 2:
                                if (dedSwitch == DED_DATA)
                                    line3[i] = OTWDriver.pCockpitManager->mpIcp->DEDLines[j][i];
                                else if (dedSwitch == PFL_DATA)
                                    line3[i] = OTWDriver.pCockpitManager->mpIcp->PFLLines[j][i];

                                break;

                            case 3:
                                if (dedSwitch == DED_DATA)
                                    line4[i] = OTWDriver.pCockpitManager->mpIcp->DEDLines[j][i];
                                else if (dedSwitch == PFL_DATA)
                                    line4[i] = OTWDriver.pCockpitManager->mpIcp->PFLLines[j][i];

                                break;

                            case 4:
                                if (dedSwitch == DED_DATA)
                                    line5[i] = OTWDriver.pCockpitManager->mpIcp->DEDLines[j][i];
                                else if (dedSwitch == PFL_DATA)
                                    line5[i] = OTWDriver.pCockpitManager->mpIcp->PFLLines[j][i];

                                break;
                        }
                    }

                    switch (j)
                    {
                        case 0:
                            display->TextLeft(xPos, yPos + 0.03F, line1, 0);
                            break;

                        case 1:
                            display->TextLeft(xPos, yPos + 0.03F, line2, 0);
                            break;

                        case 2:
                            display->TextLeft(xPos, yPos + 0.03F, line3, 0);
                            break;

                        case 3:
                            display->TextLeft(xPos, yPos + 0.03F, line4, 0);
                            break;

                        case 4:
                            display->TextLeft(xPos, yPos + 0.03F, line5, 0);
                            break;

                        default:
                            break;
                    }

                    yPos -= 0.08F;
                }
            }
        }
    }

    //MI TFR info if needed
    if (theLantirn and theLantirn->IsEnabled() and ownship and ownship->mFaults and not ownship->mFaults->WarnReset()
       and ownship->RFState not_eq 2)
    {
        char tempstr[20] = "";

        if (theLantirn->evasize  == 1 and flash)
            sprintf(tempstr, "FLY UP");
        else if (theLantirn->evasize  == 2 and flash)
            sprintf(tempstr, "OBSTACLE");

        if (theLantirn->SpeedUp and not flash)
            sprintf(tempstr, "SLOW");

        display->TextCenter(0, 0.25, tempstr, 0);
    }

}//End function

void HudClass::DrawWindowString(int window, char *str, int boxed)
{
    float x, y, width, height;

    window --;

    x = hudWinX[window];
    y = hudWinY[window];
    width = hudWinWidth[window];
    height = hudWinHeight[window];

    if (x > 0.01F)
        display->TextRight(x + width, y + height * 0.5F, str, boxed);
    else if (x < -0.01F)
        display->TextLeft(x, y + height * 0.5F, str, boxed);
    else
        display->TextCenter(x, y + height * 0.5F, str, boxed);
}
const static float Tadpolesize = 0.029F;
const static float Linelenght = 0.07F;
void HudClass::DrawFPM(void)
{
    //MI not here in BUP reticle mode
    if (g_bRealisticAvionics and WhichMode == 2 and FCC->GetSubMode() == FireControlComputer::MAN)
        return;

    float dx, dy;

    ShiAssert(ownship);

    dx = betaHudUnits;
    dy = hudWinY[BORESIGHT_CROSS_WINDOW] +
         hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F -
         alphaHudUnits;

    //MI -- Make the FPM a bit bigger....why did they make it so complicated??
    if ( not g_bRealisticAvionics)
    {
        display->Line(-0.0075F + dx, 0.015F + dy, 0.0075F + dx, 0.015F + dy);
        display->Line(0.0075F + dx, 0.015F + dy, 0.015F + dx, 0.0075F + dy);
        display->Line(0.015F + dx, 0.0075F + dy, 0.015F + dx, -0.0075F + dy);
        display->Line(0.015F + dx, -0.0075F + dy, 0.0075F + dx, -0.015F + dy);
        display->Line(0.0075F + dx, -0.015F + dy, -0.0075F + dx, -0.015F + dy);
        display->Line(-0.0075F + dx, -0.015F + dy, -0.015F + dx, -0.0075F + dy);
        display->Line(-0.015F + dx, -0.0075F + dy, -0.015F + dx, 0.0075F + dy);
        display->Line(-0.015F + dx, 0.0075F + dy, -0.0075F + dx, 0.015F + dy);
        display->Line(0.015F + dx, dy, 0.05F + dx, dy);
        display->Line(-0.015F + dx, dy, -0.05F + dx, dy);
        display->Line(dx, 0.015F + dy, dx, 0.05F + dy);
    }
    else
    {
        //HUD_Fixes.pdf #2 - restrain FPM within bounds of HUD. Smeg, 14-Oct-2003.
        if (g_bHUDFix == true)
        {
            float constraint = 1.00F - Linelenght - Tadpolesize;
            //bool fpmConstrained = false; TJL I need this
            fpmConstrained = false;

            if (dx < -constraint) //Left edge
            {
                dx = -constraint;
                fpmConstrained = true;
            }
            else if (dx > constraint) //Right
            {
                dx = constraint;
                fpmConstrained = true;
            }

            if (dy < -constraint) //Bottom
            {
                dy = -constraint;
                fpmConstrained = true;
            }
            else if (dy > constraint) //Top
            {
                dy = constraint;
                fpmConstrained = true;
            }

            //If FPM was constrained to HUD, then draw a cross on top of it to warn that it's unreliable.
            if (fpmConstrained == TRUE)
            {
                float crossSize = Tadpolesize; //mirv sez that the cross covers only the FPM circle.
                display->Line(-crossSize + dx, -crossSize + dy, crossSize + dx, crossSize + dy);
                display->Line(-crossSize + dx, crossSize + dy, crossSize + dx, -crossSize + dy);
            }
        } //End of HUD fix.

        //MI
        if (g_bINS and g_bRealisticAvionics)
        {
            if (ownship->INSState(AircraftClass::INS_HUD_FPM))
            {
                display->Circle(dx, dy, Tadpolesize);
                display->Line(dx + Tadpolesize, dy, dx + Tadpolesize + Linelenght, dy);
                display->Line(dx - Tadpolesize, dy, dx - Tadpolesize - Linelenght, dy);
                display->Line(dx, dy + Tadpolesize, dx, dy + Tadpolesize + Linelenght - 0.025f);
            }
        }
        else
        {
            display->Circle(dx, dy, Tadpolesize);
            display->Line(dx + Tadpolesize, dy, dx + Tadpolesize + Linelenght, dy);
            display->Line(dx - Tadpolesize, dy, dx - Tadpolesize - Linelenght, dy);
            display->Line(dx, dy + Tadpolesize, dx, dy + Tadpolesize + Linelenght - 0.025f);
        }


    }

    // AOA Bracket 11 - 15 Degrees
    // Check for gear down
    //TJL 03/08/04
    if (((AircraftClass*)ownship)->af->gearPos > 0.5F)
    {
        float aoaOffset = 0.0f;

        if (ownship->af->GetTypeAC() == 8 or ownship->af->GetTypeAC() == 9 or ownship->af->GetTypeAC() == 10)
        {
            aoaOffset = cockpitFlightData.alpha - 8.0F;
        }
        else if (ownship->af->GetTypeAC() == 6 or ownship->af->GetTypeAC() == 7)
        {
            aoaOffset = cockpitFlightData.alpha - 15.0F;
        }
        else
        {
            aoaOffset = cockpitFlightData.alpha - 13.0F;
        }

        //aoaOffset = min ( max (aoaOffset, -2.0F), 2.0F) / 2.0F;
        aoaOffset = aoaOffset / 2.0F;
        display->Line(-0.15F + dx, -0.12F + 0.12F * aoaOffset + dy,
                      -0.15F + dx, 0.12F + 0.12F * aoaOffset + dy);
        display->Line(-0.15F + dx, -0.12F + 0.12F * aoaOffset + dy,
                      -0.13F + dx, -0.12F + 0.12F * aoaOffset + dy);
        display->Line(-0.15F + dx,  0.12F + 0.12F * aoaOffset + dy,
                      -0.13F + dx, 0.12F + 0.12F * aoaOffset + dy);
    }
}

void HudClass::DrawTDBox(void)
{
    if (targetData)
    {
        display->AdjustOriginInViewport(0.0F, (hudWinY[BORESIGHT_CROSS_WINDOW] +
                                               hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));

        DrawDesignateMarker(Square, targetData->az, targetData->el, targetData->droll);

        display->AdjustOriginInViewport(0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
                                                hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
    }
}


void HudClass::DrawDesignateMarker(DesignateShape shape, float az, float el, float dRoll)
{
    float xPos, yPos;
    char tmpStr[12];
    mlTrig trig;
    float offset = MRToHudUnits(45.0F);

    xPos = RadToHudUnitsX(az);
    yPos = RadToHudUnitsY(el);

    if (fabs(az) < 90.0F * DTR and 
        fabs(el) < 90.0F * DTR and 
        fabs(xPos) < 0.90F and fabs(yPos + hudWinY[BORESIGHT_CROSS_WINDOW] +
                                   hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F) < 0.90F)
    {
        if (shape == Square)
        {
            display->AdjustOriginInViewport(xPos, yPos);
            display->Line(-0.05F, -0.05F, -0.05F,  0.05F);
            display->Line(-0.05F, -0.05F,  0.05F, -0.05F);
            display->Line(0.05F,  0.05F, -0.05F,  0.05F);
            display->Line(0.05F,  0.05F,  0.05F, -0.05F);
            display->AdjustOriginInViewport(-xPos, -yPos);
        }
        else if (shape == Circle)
        {
            display->Circle(xPos, yPos, 0.05F);
        }
    }
    else
    {
        mlSinCos(&trig, dRoll);
        xPos = offset * trig.sin;
        yPos = offset * trig.cos;
        display->Line(0.0f, 0.0f, xPos, yPos);
        sprintf(tmpStr, "%.0f", (float)acos(cos(az) * cos(el)) * RTD);
        ShiAssert(strlen(tmpStr) < sizeof(tmpStr));
        display->TextRight(-0.075F, display->TextHeight() * 0.5F, tmpStr, 0);
    }
}


void HudClass::DrawBoresightCross(void)
{
    float xCenter = hudWinX[BORESIGHT_CROSS_WINDOW] + hudWinWidth[BORESIGHT_CROSS_WINDOW] * 0.5F;
    float yCenter = hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F;

    //TJL 03/11/04 F16 only for boresight cross (others have waterline) or default HUD
    if (ownship->IsF16() or ownship->af->GetTypeAC() == 0)
    {
        //end


        display->Line(xCenter + 0.025F, yCenter, xCenter + 0.05F, yCenter);
        display->Line(xCenter - 0.025F, yCenter, xCenter - 0.05F, yCenter);
        display->Line(xCenter, yCenter + 0.025F, xCenter, yCenter + 0.05F);
        display->Line(xCenter, yCenter - 0.025F, xCenter, yCenter - 0.05F);
    }

    if (HudData.IsSet(HudDataType::RadarNoRad))
    {
        //MI
        if ( not g_bRealisticAvionics)
            display->TextCenter(xCenter, yCenter + 0.075F, "NO RAD", 0);
        else
            display->TextCenter(xCenter, yCenter + 0.15F, "NO RAD", 0);
    }

    if (HudData.IsSet(HudDataType::RadarBoresight bitor HudDataType::RadarSlew))
    {
        if ( not g_bRealisticAvionics)
        {
            yCenter -= RadToHudUnitsY(3.0F * DTR);
            display->Line(xCenter + 0.05F, yCenter, xCenter + 0.1F, yCenter);
            display->Line(xCenter - 0.05F, yCenter, xCenter - 0.1F, yCenter);
            display->Line(xCenter, yCenter + 0.05F, xCenter, yCenter + 0.15F);
            display->Line(xCenter, yCenter - 0.05F, xCenter, yCenter - 0.15F);
            display->Point(xCenter, yCenter);
        }
        else
        {
            //MI draw the cross correct
            if (HudData.IsSet(HudDataType::RadarSlew))
            {
                yCenter -= RadToHudUnitsY(3.0F * DTR);
                display->Line(xCenter + 0.02F, yCenter, xCenter + 0.1F, yCenter);
                display->Line(xCenter - 0.02F, yCenter, xCenter - 0.1F, yCenter);
                display->Line(xCenter, yCenter + 0.02F, xCenter, yCenter + 0.0533F);
                display->Line(xCenter, yCenter + 0.0733F, xCenter, yCenter + 0.1066F);
                display->Line(xCenter, yCenter + 0.1266F, xCenter, yCenter + 0.16F);
                display->Line(xCenter, yCenter - 0.02F, xCenter, yCenter - 0.0533F);
                display->Line(xCenter, yCenter - 0.0733F, xCenter, yCenter - 0.1066F);
                display->Line(xCenter, yCenter - 0.1266F, xCenter, yCenter - 0.16F);
                display->Point(xCenter, yCenter);
            }
            else
            {
                yCenter -= RadToHudUnitsY(3.0F * DTR);
                display->Line(xCenter + 0.02F, yCenter, xCenter + 0.1F, yCenter);
                display->Line(xCenter - 0.02F, yCenter, xCenter - 0.1F, yCenter);
                display->Line(xCenter, yCenter + 0.02F, xCenter, yCenter + 0.16F);
                display->Line(xCenter, yCenter - 0.02F, xCenter, yCenter - 0.16F);
                display->Point(xCenter, yCenter);
            }
        }

        if (HudData.IsSet(HudDataType::RadarSlew))
        {
            display->Circle(xCenter + HudData.radarAz / (60.0F * DTR) * 0.1F,
                            yCenter + HudData.radarEl / (60.0F * DTR) * 0.15F, 0.025F);
        }
    }

    if (HudData.IsSet(HudDataType::RadarVertical))
    {
        display->Line(xCenter, yCenter - 0.15F, xCenter, yCenter - 2.0F);
    }
}


void HudClass::DrawHorizonLine(void)
{
    float dx, dy;
    float x1, x2, y;

    // Pitch ladder is centered about the flight path marker
    dx = betaHudUnits;
    dy = -alphaHudUnits;
    display->AdjustOriginInViewport(dx, hudWinY[BORESIGHT_CROSS_WINDOW] +
                                    hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F + dy);

    // AdjustRotationAboutOrigin about nose marker
    display->AdjustRotationAboutOrigin(-cockpitFlightData.roll);
    x1 = hudWinWidth[PITCH_LADDER_WINDOW] * 0.25F;
    x2 = hudWinWidth[PITCH_LADDER_WINDOW] * 2.00F;
    y = -cockpitFlightData.gamma * RTD * degreesForScreen;

    display->Line(x1, y,  x2, y);
    display->Line(-x1, y, -x2, y);

    // Put the display offsets back the way they were
    display->ZeroRotationAboutOrigin();
    display->AdjustOriginInViewport(-dx, -hudWinY[BORESIGHT_CROSS_WINDOW] -
                                    hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F - dy);
}

const static float PitchLadderDiff = 0.15F;
void HudClass::DrawPitchLadder(void)
{

    //MI INS stuff
    if (g_bRealisticAvionics and g_bINS)
    {
        if (
            ownship and 
            ownship->INSState(AircraftClass::INS_PowerOff) or
 not ownship->INSState(AircraftClass::INS_HUD_STUFF)
        )
        {
            return;
        }
    }

    //MI not here in BUP reticle mode
    if (g_bRealisticAvionics and WhichMode == 2 and FCC->GetSubMode() == FireControlComputer::MAN)
    {
        return;
    }

    char tmpStr[12];
    float vert[18][2];
    int i, a;
    float dx, dy;
    bool drawGhostHorizon = true;
    //radius was originally calculated as 8.0F * degressForScreen, but that screws up
    //for full-screen HUD. Fixed radius at value that seems to work well for 2D, 3D and
    //fullscreen.
    float ghostRadius = 0.88F;

    // Pitch ladder is centered about the boresight cross
    display->AdjustOriginInViewport(
        0.0F,
        hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F
    );


    dx = betaHudUnits;
    dy = -alphaHudUnits;

    // Well, really it is centered on the flight path marker
    display->AdjustOriginInViewport(dx, dy);

    // Rotate about nose marker
    // display->AdjustRotationAboutOrigin(-cockpitFlightData.roll);
    display->AdjustRotationAboutOrigin(-ownship->GetMu());

    /*-------------------------*/
    /* elevation in tenths      */
    /*-------------------------*/
    a = FloatToInt32(cockpitFlightData.gamma * 10.0F * RTD);
    float offset = cockpitFlightData.gamma * 10.0F * RTD - a;
    i = a % 50;

    //MI Smaller pitchladder
    if ( not g_bRealisticAvionics)
    {
        vert[0][0] = hudWinWidth[PITCH_LADDER_WINDOW] * 0.25F;
        vert[1][0] = hudWinWidth[PITCH_LADDER_WINDOW] * 0.35F;
        vert[2][0] = hudWinWidth[PITCH_LADDER_WINDOW] * 0.45F;
        vert[3][0] = hudWinWidth[PITCH_LADDER_WINDOW] * 0.55F;
        vert[4][0] = hudWinWidth[PITCH_LADDER_WINDOW] * 0.65F;
        vert[5][0] = hudWinWidth[PITCH_LADDER_WINDOW] * 0.75F;
        vert[6][0] = vert[5][0];
    }
    else
    {
        vert[0][0] = (hudWinWidth[PITCH_LADDER_WINDOW] - PitchLadderDiff) * 0.25F;
        vert[1][0] = (hudWinWidth[PITCH_LADDER_WINDOW] - PitchLadderDiff) * 0.35F;
        vert[2][0] = (hudWinWidth[PITCH_LADDER_WINDOW] - PitchLadderDiff) * 0.45F;
        vert[3][0] = (hudWinWidth[PITCH_LADDER_WINDOW] - PitchLadderDiff) * 0.55F;
        vert[4][0] = (hudWinWidth[PITCH_LADDER_WINDOW] - PitchLadderDiff) * 0.65F;
        vert[5][0] = (hudWinWidth[PITCH_LADDER_WINDOW] - PitchLadderDiff) * 0.75F;
        vert[6][0] = vert[5][0];
    }

    vert[7][0] = -vert[0][0];
    vert[8][0] = -vert[1][0];
    vert[9][0] = -vert[2][0];
    vert[10][0] = -vert[3][0];
    vert[11][0] = -vert[4][0];
    vert[12][0] = -vert[5][0];
    vert[13][0] = -vert[5][0];

    //MI Smaller pitchladder
    if ( not g_bRealisticAvionics)
    {
        vert[14][0] = hudWinWidth[PITCH_LADDER_WINDOW] * 1.00F;
        vert[15][0] = -vert[14][0];
        vert[16][0] = hudWinWidth[PITCH_LADDER_WINDOW] * 2.00F;
        vert[17][0] = -vert[16][0];
    }
    else
    {
        vert[14][0] = (hudWinWidth[PITCH_LADDER_WINDOW] - PitchLadderDiff) * 1.00F;
        vert[15][0] = -vert[14][0];

        //HUD_Fixes.pdf #1 - +if enabled, extend horizon line out beyond edges
        //of pitch ladder, otherwise do old stuff. Smeg, 14-Oct-2003.
        if (g_bHUDFix == true)
        {
            vert[16][0] = 1.00F;
            vert[17][0] = -1.00F;
        }
        else
        {
            vert[16][0] = (hudWinWidth[PITCH_LADDER_WINDOW] - PitchLadderDiff) * 1.00F;
            vert[17][0] = -vert[16][0];
        }
    }

    if ( not g_bNewPitchLadder)
    {
        vert[0][1] = -(0.1F * i + 20.0F) * degreesForScreen;
    }
    else
    {
        vert[0][1] = -(0.1F * (i + offset) + 20.0F) * degreesForScreen;
    }

    a = (a - i) / 10 - 20; // starting number

    // JPO - draw the 2.5 degree line when gear is down and locked.
    if (
        ((AircraftClass*)ownship)->af->gearPos > 0.5F and 
        (a < -2) and ((a + 50) > -2)
    )  // JPO we surround the -2.5 line
    {
        float delta = (float)a - -2.5f;
        vert[0][1] = vert[0][1] - delta * degreesForScreen; // adjust

        vert[1][1] = vert[0][1];
        vert[2][1] = vert[0][1];
        vert[3][1] = vert[0][1];
        vert[4][1] = vert[0][1];
        vert[5][1] = vert[0][1];
        vert[7][1] = vert[0][1];
        vert[8][1] = vert[0][1];
        vert[9][1] = vert[0][1];
        vert[10][1] = vert[0][1];
        vert[11][1] = vert[0][1];
        vert[12][1] = vert[0][1];
        vert[14][1] = vert[0][1];
        vert[15][1] = vert[0][1];

        display->Line(vert[0][0], vert[0][1], vert[1][0], vert[1][1]);
        display->Line(vert[2][0], vert[2][1], vert[3][0], vert[3][1]);
        display->Line(vert[4][0], vert[4][1], vert[5][0], vert[5][1]);
        display->Line(vert[7][0], vert[7][1], vert[8][0], vert[8][1]);
        display->Line(vert[9][0], vert[9][1], vert[10][0], vert[10][1]);
        display->Line(vert[11][0], vert[11][1], vert[12][0], vert[12][1]);

        // reset it
        vert[0][1] = -(0.1F * i + 20.0F) * degreesForScreen;
    }

    //By default, set things up so the ghost horizon will be drawn, unless decided otherwise.
    drawGhostHorizon = true;

    for (i = 0; i < 30; i++)
    {
        if ((a >= -90) and (a <= 90))
        {
            //MI no - is drawn on the real pitchladder
            if ( not g_bRealisticAvionics)
            {
                sprintf(tmpStr , "%d", a);
            }
            else
            {
                if (a > 0)
                {
                    sprintf(tmpStr , "%d", a);
                }
                else
                {
                    sprintf(tmpStr, "%d", -a);
                }
            }

            ShiAssert(strlen(tmpStr) < 12);

            //If pitch latter for lines above horizon...
            if (a > 0)
            {
                vert[5][1] = vert[0][1];
                vert[6][1] = vert[0][1] - hudWinWidth[PITCH_LADDER_WINDOW] * 0.1F;
                vert[7][1] = vert[0][1];
                vert[12][1] = vert[0][1];
                vert[13][1] = vert[6][1];
                vert[14][1] = vert[0][1];
                vert[15][1] = vert[0][1];

                //If the HUD fix is applied, then we may have buggered up the x position of the
                //two tick marks by moving them to the inside.. Restore it here to make sure.
                vert[6][0] = vert[5][0];
                vert[13][0] = vert[12][0];

                display->Line(vert[0][0], vert[0][1], vert[5][0], vert[5][1]);
                display->Line(vert[5][0], vert[5][1], vert[6][0], vert[6][1]);
                display->Line(vert[7][0], vert[7][1], vert[12][0], vert[12][1]);
                display->Line(vert[12][0], vert[12][1], vert[13][0], vert[13][1]);
                display->TextCenterVertical(vert[14][0], vert[14][1], tmpStr, 0);
                display->TextCenterVertical(vert[15][0], vert[15][1], tmpStr, 0);
            }
            //Else if below horizon...
            else if (a < 0)
            {
                float horizontalLevel = 0;

                //HUD fix #3b: pitch ladder bars below horizon should be slanted downwards,
                //from 8.3 degrees down for the -5deg line, to 45deg down on the -90deg line.
                //Smeg, 15-Oct-2003
                if ((g_bHUDFix == true) and (g_bRealisticAvionics))
                {
                    //Determine angle down from horizontal.
                    float angleDown = (-a - 5) * ((45.0F - 8.3F) / 85.0F) + 8.3F;
                    float tanAngleDown = tanf(angleDown * (2 * static_cast<float>(DPI) / 360.0F));
                    horizontalLevel = vert[0][1];

                    //Correct vertical position of each vertice...Right side.
                    vert[0][1] = horizontalLevel - (tanAngleDown * vert[0][0]);
                    vert[1][1] = horizontalLevel - (tanAngleDown * vert[1][0]);
                    vert[2][1] = horizontalLevel - (tanAngleDown * vert[2][0]);
                    vert[3][1] = horizontalLevel - (tanAngleDown * vert[3][0]);
                    vert[4][1] = horizontalLevel - (tanAngleDown * vert[4][0]);
                    vert[5][1] = horizontalLevel - (tanAngleDown * vert[5][0]);
                    //vert[6][1] = vert[5][1] + hudWinWidth[PITCH_LADDER_WINDOW] * 0.1F; moved to inside.

                    //Left side is a copy of the right side.
                    vert[7][1] = vert[0][1];
                    vert[8][1] = vert[1][1];
                    vert[9][1] = vert[2][1];
                    vert[10][1] = vert[3][1];
                    vert[11][1] = vert[4][1];
                    vert[12][1] = vert[5][1];
                    //vert[13][1] = vert[6][1]; moved to inside.

                    //Text stays level with start of mark.
                    vert[14][1] = horizontalLevel;
                    vert[15][1] = horizontalLevel;

                    //Now, sort out the vertical ticks. On the -ve ladder, they should be on the
                    //inside of the lines, not the outside, and should be 3/4 the size of the originals.
                    vert[6][1]  = vert[0][1] + hudWinWidth[PITCH_LADDER_WINDOW] * 0.075F;
                    vert[13][1] = vert[6][1];
                    vert[6][0]  = vert[0][0];
                    vert[13][0] = vert[7][0];

                }
                else //Do old horizontal lines as hud fixes are either off or not realistic Avionics.
                {
                    vert[1][1] = vert[0][1];
                    vert[2][1] = vert[0][1];
                    vert[3][1] = vert[0][1];
                    vert[4][1] = vert[0][1];
                    vert[5][1] = vert[0][1];
                    vert[6][1] = vert[0][1] + hudWinWidth[PITCH_LADDER_WINDOW] * 0.1F;
                    vert[7][1] = vert[0][1];
                    vert[8][1] = vert[0][1];
                    vert[9][1] = vert[0][1];
                    vert[10][1] = vert[0][1];
                    vert[11][1] = vert[0][1];
                    vert[12][1] = vert[0][1];
                    vert[13][1] = vert[6][1];
                    vert[14][1] = vert[0][1];
                    vert[15][1] = vert[0][1];

                }

                //Draw Right side ladder line - 3 dashes.
                display->Line(vert[0][0], vert[0][1], vert[1][0], vert[1][1]);
                display->Line(vert[2][0], vert[2][1], vert[3][0], vert[3][1]);
                display->Line(vert[4][0], vert[4][1], vert[5][0], vert[5][1]);

                //Left-side line.
                display->Line(vert[7][0], vert[7][1], vert[8][0], vert[8][1]);
                display->Line(vert[9][0], vert[9][1], vert[10][0], vert[10][1]);
                display->Line(vert[11][0], vert[11][1], vert[12][0], vert[12][1]);

                //Now do vertical ticks. Positioning is different for -ve pitch ladder
                //if we're applying the HUD fix.
                if ((g_bHUDFix == true) and (a < 0) and (g_bRealisticAvionics))
                {
                    display->Line(vert[0][0], vert[0][1], vert[6][0], vert[6][1]);
                    display->Line(vert[7][0], vert[7][1], vert[13][0], vert[13][1]);

                    //Return vertice[0][1] back to its original value prior to messing with the sloped
                    //lines. Otherwise, we'll end up accumulating a bunch of errors that will eventually
                    //result in the horizon line being drawn in the wrong place as everything uses this
                    //as the vertical reference, apparently.
                    vert[0][1] = horizontalLevel;
                }
                else
                {
                    display->Line(vert[5][0], vert[5][1], vert[6][0], vert[6][1]);
                    display->Line(vert[12][0], vert[12][1], vert[13][0], vert[13][1]);
                }

                display->TextCenterVertical(vert[14][0], vert[14][1], tmpStr, 0);
                display->TextCenterVertical(vert[15][0], vert[15][1], tmpStr, 0);
            }

            //Otherwise, must be horizon line.
            else
            {
                ShiAssert(a == 0);
                vert[7][1] = vert[0][1];
                vert[16][1] = vert[0][1];
                vert[17][1] = vert[0][1];

                //If HUD fixes are applied, only draw horizon line if it's outside the imaginary circle
                //that's used for drawing ghost horizon line.
                drawGhostHorizon = CheckGhostHorizon(ghostRadius, dx, dy, vert[16][0], vert[16][1], vert[17][0], vert[17][1]);

                if (drawGhostHorizon == false)
                {
                    display->Line(vert[0][0], vert[0][1], vert[16][0], vert[16][1]);
                    display->Line(vert[7][0], vert[7][1], vert[17][0], vert[17][1]);
                }
            }
        }

        a += 5;

        vert[0][1] +=  5.0F * degreesForScreen;
    }

    //HUD fix #3a - ghost horizon when normal horizon line not drawn. Smeghead, 14-Oct-2003.
    if (drawGhostHorizon == true)
    {
        //How much has the origin been shifted for the display? It was shifted once vertically
        //for the boresight, then once more for the FPM.
        float xOffset = dx;
        float yOffset = hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F + dy;

        //Vertices for horizon dotted line - 3 dashes either side of centre.
        float ghostVert[12][2] =
        {
            { -1.000F, 0.000F}, { -0.833F, 0.000F},
            { -0.667F, 0.000F}, { -0.500F, 0.000F},
            { -0.333F, 0.000F}, { -0.166F, 0.000F},
            {0.166F, 0.000F}, {0.333F, 0.000F},
            {0.500F, 0.000F}, {0.667F, 0.000F},
            {0.833F, 0.000F}, {1.000F, 0.000F},
        };
        //Ghost horizon is situated perpendicular to 8-degree circle that's centred on the HUD.
        //Presumably we could do this by setting the origin to a point on that circle, then
        //rotating it to the aircraft's angle of bank. Since the angle of bank is already set
        //above when we start drawing the pitch ladder, we don't need to care about that.
        float angleFromCentre = -ownship->GetMu();

        float xPos;
        float yPos;

        //Ghost horizon needs to be drawn opposite way round if nose is above horizon.
        if (cockpitFlightData.pitch > 0)
        {
            yPos = -cos(angleFromCentre) * ghostRadius;
            xPos = -sin(angleFromCentre) * ghostRadius;
        }
        else
        {
            yPos = cos(angleFromCentre) * ghostRadius;
            xPos = sin(angleFromCentre) * ghostRadius;
        }

        //Shift origin to position on circle.
        display->AdjustOriginInViewport(xPos - xOffset, yPos - yOffset);

        //Draw dashed line - 3 dashes on each side.
        display->Line(ghostVert[0][0], ghostVert[0][1], ghostVert[1][0], ghostVert[1][1]);
        display->Line(ghostVert[2][0], ghostVert[2][1], ghostVert[3][0], ghostVert[3][1]);
        display->Line(ghostVert[4][0], ghostVert[4][1], ghostVert[5][0], ghostVert[5][1]);

        display->Line(ghostVert[6][0],  ghostVert[6][1],  ghostVert[7][0],  ghostVert[7][1]);
        display->Line(ghostVert[8][0],  ghostVert[8][1],  ghostVert[9][0],  ghostVert[9][1]);
        display->Line(ghostVert[10][0], ghostVert[10][1], ghostVert[11][0], ghostVert[11][1]);

        //Done drawing. Put the origin back the way it was
        display->AdjustOriginInViewport(-(xPos - xOffset), -(yPos - yOffset));
    } //End of HUD fix.

    display->ZeroRotationAboutOrigin();
    display->AdjustOriginInViewport(-dx, -dy);

    display->AdjustOriginInViewport(
        0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F)
    );
}

//HUD fix #3a - ghost horizon when normal horizon line not drawn. Smeghead, 18-Oct-2003.
bool HudClass::CheckGhostHorizon(float radius, float xOffset, float yOffset, float horizX1, float horizY1, float horizX2, float horizY2)
{
    bool ghostHorizonDrawn = false;

    //Ghost horizon only drawn if HUD fixes are used and we're on realistic avionics.
    if ((g_bHUDFix == false) or not (g_bRealisticAvionics))
    {
        return ghostHorizonDrawn;
    }

    //This function decides whether the normal horizon line should be drawn (true) or the
    //ghost horizon (false). It works by determining whether the normal horizon line
    //intersects the circle around which the ghost horizon would be drawn. If it intersects,
    //the horizon is visible in the HUD. If it doesn't, the ghost horizon needs to be
    //drawn. A bit of a pain in the arse, but it's the only way to be sure this works
    //for all possible cases.

    //Points at either end of horizon line. These need to be corrected because origin is
    //first offset to boresight cross, then offset to FPM position relative to that. However,
    //offset needed depends on the roll angle, so we need to correct for that, too.
    float horizOffset = sinf(-ownship->GetMu()) * (-xOffset);
    float vertOffset = cosf(-ownship->GetMu()) * (-yOffset - (hudWinY[BORESIGHT_CROSS_WINDOW] +
                       hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));

    float x1 = horizX1 - horizOffset;
    float y1 = horizY1 - vertOffset;
    float x2 = horizX2 - horizOffset;
    float y2 = horizY2 - vertOffset;

    /* //Only enabled if debugging needed...
    //Draw some stuff that's useful for eyeballing whether this actually works or not.
    display->Circle(horizOffset, vertOffset, radius);
    display->Line(horizOffset, vertOffset, horizOffset, vertOffset + 2);
    display->Line(horizOffset, vertOffset, horizOffset, vertOffset - 2);
    display->Line(horizOffset, vertOffset, horizOffset + 2, vertOffset);
    display->Line(horizOffset, vertOffset, horizOffset - 2, vertOffset);
    */

    //Intersection check.
    float dx = x2 - x1;
    float dy = y2 - y1;
    double drSquared = (dx * dx) + (dy * dy);
    double D  = (x1 * y2) - (x2 * y1);

    double discriminant = (radius * radius) * drSquared - (D * D);

    //If discriminant is less than zero, the horizon line didn't intersect the circle, so
    //the ghost horizon line must be drawn.
    if (discriminant < 0)
    {
        ghostHorizonDrawn = true;
    }

    return (ghostHorizonDrawn);
}

float HudClass::MRToHudUnits(float mr)
{
    float retval;

    if (mr * 0.001F > 2.0F * halfAngle * DTR)
    {
        retval = 2.0F;
    }
    else
    {
        retval = (float)(tan(mr * 0.001F) / tan(halfAngle * DTR));
    }

    return (retval);
}

float HudClass::RadToHudUnits(float rad)
{
    float retval;

    if (rad > 2.0F * halfAngle * DTR)
    {
        //retval = 2.0F;
        retval = (float)(rad / (halfAngle * DTR));
    }
    else
    {
        retval = (float)(tan(rad) / tan(halfAngle * DTR));
    }

    return retval;
}

float HudClass::HudUnitsToRad(float hudUnits)
{
    float retval;

    retval = (float)atan(hudUnits * tan(halfAngle * DTR));
    return (retval);
}

void HudClass::GetBoresightPos(float* xPos, float* yPos)
{
    *xPos = hudWinX[BORESIGHT_CROSS_WINDOW] + hudWinWidth[BORESIGHT_CROSS_WINDOW] * 0.5F;
    *yPos = hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F;
}

void HudClass::SetHalfAngle(float newAngle, float HScale, float VScale)
{
    halfAngle = newAngle;
    degreesForScreen = 1.0F / halfAngle;
    mVScale = VScale;
    mHScale = HScale;
}

void HudClass::PushButton(int whichButton, int whichMFD)
{
    switch (whichButton)
    {
        case 0:
            MfdDisplay[whichMFD]->SetNewMode(MFDClass::MfdMenu);
            break;

        case 14:
            MFDSwapDisplays();
            break;
    }
}

HudDataType::HudDataType(void)
{
    tgtAz = tgtEl = tgtAta = tgtDroll = 0.0F;
    tgtId = -1;
    flags = 0;
}

#include "SimIO.h" // Retro 4Jan2004
#include "fmath.h"

// COBRA - RED - Completely rewritten
DWORD HudClass::GetHudColor(void)
{
    // COBRA - RED - Hud Cxes based on plane pitch and time o day
    AutoHudCx = ObserverPitch;

    if (ObserverPitch < (-0.05f)) AutoHudCx *= 1.2f; // if going down faster changes

    // a copy of the selected hud color
    Pcolor Color = hudColor;

    // if NVG change it to whiteish
    if (TheDXEngine.GetState() == DX_NVG)
    {
        Color.r = 0.8f;
        Color.g = 1.0f;
        Color.b = 0.9f;
    }


    //Set Up the Contrast, the Hud Color and the Light Level and Apply them
    SetLightLevel();
    curHudColor = curHudColor bitand 0xff000000; // mantain alpha
    curHudColor += ((int)(Color.b * 255 * (0.5F + HudContrast / 2))) << 16;
    curHudColor += ((int)(Color.g * 255 * (0.5F + HudContrast / 2))) << 8;
    curHudColor += ((int)(Color.r * 255 * (0.5F + HudContrast / 2)));
    //curHudColor += (((int)(hudColor*HudContrast))<<8);

    // curHudColor = HUDcolor[curColorIdx];
    // CalculateBrightness(SymWheelPos, &curHudColor);

    return curHudColor;
}

void HudClass::SetHudColor(DWORD newColor)
{
    // Find fixed colors index
    curColorIdx = 0;

    while (newColor not_eq HUDcolor[curColorIdx])
    {
        curColorIdx++;

        if (curColorIdx > NumHudColors)
        {
            curColorIdx = 0;
            break;
        }
    }

    if (TheHud)
    {
        SymWheelPos = 0.5F;
    }

    hudColor.a = ((float)((HUDcolor[curColorIdx] bitand 0xff000000) >> 24)) / 255.0f;
    hudColor.b = ((float)((HUDcolor[curColorIdx] bitand 0xff0000) >> 16)) / 255.0f;
    hudColor.g = ((float)((HUDcolor[curColorIdx] bitand 0xff00) >> 8)) / 255.0f;
    hudColor.r = ((float)(HUDcolor[curColorIdx] bitand 0xff)) / 255.0f;
    //Set Up the Contrast, the Hud Color and the Light Level and Apply them
    SetLightLevel();
    curHudColor = curHudColor bitand 0xff000000; // mantain alpha
    curHudColor += ((int)(hudColor.b * 255 * (0.5F + HudContrast / 2))) << 16;
    curHudColor += ((int)(hudColor.g * 255 * (0.5F + HudContrast / 2))) << 8;
    curHudColor += ((int)(hudColor.r * 255 * (0.5F + HudContrast / 2)));
}

void HudClass::SetContrastLevel(void)
{
}

void HudClass::HudColorStep(void)
{
    curColorIdx ++;
    curColorIdx %= NumHudColors;
    SetHudColor(HUDcolor[curColorIdx]);
    // SetLightLevel();
}

// COBRA - RED - Completely rewritten, now brigthness works on Hud Alpha Level
void HudClass::CalculateBrightness(float percent, DWORD* color)
{
    HudBrightness = 1.0f * percent;
    *color = (*color bitand 0x00ffffff) bitor ((FloatToInt32(255 * percent)) << 24);
}

// COBRA - RED - Completely rewritten
void HudClass::SetLightLevel(void)
{
    float Value;

    curHudColor = HUDcolor[curColorIdx];

    // COBRA - RED - Non realistic
    if ( not g_bRealisticAvionics)
    {
        if (brightnessSwitch == BRIGHT_AUTO)
        {
            CalculateBrightness(0.8F, &curHudColor);
            HudContrast = 0.8f;
        }
        else if (brightnessSwitch == NIGHT)
        {
            CalculateBrightness(0.6F, &curHudColor);
            HudContrast = 0.8f;
        }
        else if (brightnessSwitch == DAY)
        {
            CalculateBrightness(0.9F, &curHudColor);
            HudContrast = 1.0f;
        }

        return; // if Non realistic exits here
    }

    if (IO.AnalogIsUsed(AXIS_HUD_BRIGHTNESS) == true) // Retro 4Jan2004 (whole if)
        Value = (float)IO.GetAxisValue(AXIS_HUD_BRIGHTNESS) / 20000.0F;
    else
        Value = SymWheelPos;

    // Calculate valued based on Hud Brightness Mode Switch
    float lightLevel;

    switch (brightnessSwitch)
    {

            // Auto Mode - Brightness is based on Pitch of the Plane, the higher, the brighter
        case BRIGHT_AUTO:
            lightLevel = hudColor.a * TheTimeOfDay.GetAmbientValue() * 0.2f;
            lightLevel += AutoHudCx * 0.5f * TheTimeOfDay.GetAmbientValue();
            lightLevel += 0.3f * Value;

            if (lightLevel > HUD_MAX_BRIGHT_DAY)lightLevel = HUD_MAX_BRIGHT_DAY; // check for limits

            if (lightLevel < 0.2f)lightLevel = 0.2f; //

            CalculateBrightness(lightLevel, &curHudColor); // apply
            break;

        case NIGHT:
            CalculateBrightness(hudColor.a * (HUD_MAX_BRIGHT_NIGHT * .25f + HUD_MAX_BRIGHT_NIGHT * .74f * Value), &curHudColor);
            break;

        case DAY:
            if (g_bBrightHUD)
                CalculateBrightness(1.0F, &curHudColor);
            else
                CalculateBrightness(hudColor.a * (HUD_MAX_BRIGHT_DAY * .25f + HUD_MAX_BRIGHT_DAY * .74f * Value), &curHudColor);

            break;

        default:
            CalculateBrightness(Value, &curHudColor);
    }

    if ((g_bBrightHUD) and (brightnessSwitch == DAY))
        HudContrast = 1.0f;
    else
        HudContrast = hudColor.a + 1.0f - HUD_MAX_BRIGHT_DAY + HudBrightness * (1.0f - hudColor.a); //0.5f + TheTimeOfDay.GetAmbientValue() * lightLevel * 3.0f;

    if (HudContrast > 1.0f) HudContrast = 1.0f;

    if (HudContrast < 0.0f) HudContrast = 0.0f;
}




VuEntity* HudClass::CanSeeTarget(int type, VuEntity* entity, FalconEntity* platform)
{
    //MI camera fix
#if 0
    Tpoint point;
    ThreeDVertex pixel;
    VuEntity* retval = NULL;
    WeaponClassDataType *wc;
    float top, left, bottom, right;
    float dx, dy, dz;
    float maxRangeSqrd;
    float curRangeSqrd;

    wc = (WeaponClassDataType*)Falcon4ClassTable[type - VU_LAST_ENTITY_TYPE].dataPtr;

    maxRangeSqrd = (wc->Range * KM_TO_FT) * (wc->Range * KM_TO_FT);

    point.x = entity->XPos();
    point.y = entity->YPos();
    point.z = OTWDriver.GetGroundLevel(point.x, point.y);

    dx = point.x - platform->XPos();
    dy = point.y - platform->YPos();
    dz = point.z - platform->ZPos();

    curRangeSqrd = dx * dx + dy * dy + dz * dz;

    if (curRangeSqrd < maxRangeSqrd)
    {
        // Check LOS
        if (OTWDriver.CheckLOS(platform, (FalconEntity*)entity))
        {
            // For ownship, check if in the circle
            if (platform == ownship)
            {
                OTWDriver.renderer->TransformPoint(&point, &pixel);

                if (pixel.clipFlag == ON_SCREEN)
                {
                    // It's in front, but is it in the circle?
                    bottom = pixelYCenter + 6.0F * sightRadius;
                    top    = pixelYCenter - 6.0F * sightRadius;
                    left   = pixelXCenter - 6.0F * sightRadius;
                    right  = pixelXCenter + 6.0F * sightRadius;

                    if (pixel.x > left and pixel.x < right and pixel.y < bottom and pixel.y > top)
                    {
                        retval = entity;
                    }
                }
            }
            else // In range and good LOS enough for AI
            {
                retval = entity;
            }
        }
    }

    return retval;
#else
    Tpoint point;
    Trotation RR, RR2;
    float cosP, sinP, cosY, sinY, sinR, cosR = 0.0F;
    float el, az = 0.0F;
    VuEntity* retval = NULL;
    WeaponClassDataType *wc;
    float dx, dy, dz;
    float maxRangeSqrd;
    float curRangeSqrd;
    float CameraHalfFOV = 1.4F * DTR;
    float offset = -8.0F * DTR;

    wc =
        (WeaponClassDataType*)Falcon4ClassTable[type - VU_LAST_ENTITY_TYPE].dataPtr;

    maxRangeSqrd = (wc->Range * KM_TO_FT) * (wc->Range * KM_TO_FT);

    point.x = entity->XPos();
    point.y = entity->YPos();
    point.z = OTWDriver.GetGroundLevel(point.x, point.y);

    dx = point.x - platform->XPos();
    dy = point.y - platform->YPos();
    dz = point.z - platform->ZPos();
    point.x = dx;
    point.y = dy;
    point.z = dz;
    curRangeSqrd = dx * dx + dy * dy + dz * dz;

    if (curRangeSqrd < maxRangeSqrd)
    {
        // Check LOS
        if (OTWDriver.CheckLOS(platform, (FalconEntity*)entity))
        {
            // For ownship, check if within camera FOV
            if (platform == ownship)
            {
                //dpc - Rotation Matrix should be done once per every camera shot, not for
                //each entity, ah well...
                //But it doesn't look like it's hogging down FPS so it remains here...
                //I should probably calculate one matrix that does all three - Yaw, Pitch
                //and Roll but this works
                //and I don't want to mess it up...End effect is the same.

                cosP = (float)cos(- platform->Pitch());
                sinP = (float)sin(- platform->Pitch());
                cosY = (float)cos(- platform->Yaw());
                sinY = (float)sin(- platform->Yaw());
                cosR = (float)cos(- platform->Roll());
                sinR = (float)sin(- platform->Roll());

                RR.M11 = cosY;
                RR.M21 = sinY;
                RR.M31 = 0;

                RR.M12 = -sinY;
                RR.M22 = cosY;
                RR.M32 = 0;

                RR.M13 = 0;
                RR.M23 = 0;
                RR.M33 = 1;
                //First rotation is only doing (-Yaw)
                point.x = dx * RR.M11 + dy * RR.M12 + dz * RR.M13;
                point.y = dx * RR.M21 + dy * RR.M22 + dz * RR.M23;
                point.z = dx * RR.M31 + dy * RR.M32 + dz * RR.M33;
                dx = point.x;
                dy = point.y;
                dz = point.z;

                //JAM 27Sep03 - These are floats
                cosY = (float)cosf(0);
                sinY = (float)sinf(0);
                cosR = (float)cosf(0);
                sinR = (float)sinf(0);
                //JAM

                RR2.M11 = cosP;
                RR2.M21 = 0;
                RR2.M31 = -sinP;

                RR2.M12 = 0;
                RR2.M22 = 1;
                RR2.M32 = 0;

                RR2.M13 = sinP;
                RR2.M23 = 0;
                RR2.M33 = cosP;
                //Second rotation is doing (-Pitch)
                point.x = dx * RR2.M11 + dy * RR2.M12 + dz * RR2.M13;
                point.y = dx * RR2.M21 + dy * RR2.M22 + dz * RR2.M23;
                point.z = dx * RR2.M31 + dy * RR2.M32 + dz * RR2.M33;
                dx = point.x;
                dy = point.y;
                dz = point.z;

                if (point.x > 0.0F)     //check to see if entity is in front of us (maybe not needed but anyway...)
                {
                    //One last rotation: (-Roll) this time
                    point.y = dy * cosR - dz * sinR;
                    point.z = dy * sinR + dz * cosR;

                    el = (float)atan2(-point.z, (float)sqrt(point.x * point.x + point.y * point.y));
                    az = (float)atan2(point.y, point.x);

                    if (fabs(el - g_fReconCameraOffset * DTR) < g_fReconCameraHalfFOV * DTR and fabs(az) < g_fReconCameraHalfFOV * DTR)
                    {
                        retval = entity;
                    }
                }
            }
            else // In range and good LOS enough for AI
            {
                retval = entity;
            }
        }
    }

    return retval;
#endif
}
void HudClass::DrawDTOSSBox(void)
{
    if (targetData)
    {
        display->AdjustOriginInViewport(0.0F, (hudWinY[BORESIGHT_CROSS_WINDOW] +
                                               hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));

        DrawTDMarker(targetData->az, targetData->el, targetData->droll, 0.03F);

        display->AdjustOriginInViewport(0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
                                                hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
    }
}
void HudClass::DrawTDMarker(float az, float el, float dRoll, float size)
{
    float xPos, yPos;
    char tmpStr[12];
    mlTrig trig;
    float offset = MRToHudUnits(45.0F);
    //MI
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(playerAC, SensorClass::Radar);
    xPos = RadToHudUnitsX(az);
    yPos = RadToHudUnitsY(el);

    if (fabs(az) < 90.0F * DTR and fabs(el) < 90.0F * DTR and fabs(xPos) < 0.90F and 
        fabs(yPos + hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F) < 0.90F)
    {
        display->AdjustOriginInViewport(xPos, yPos);

        //MI
        if (g_bRealisticAvionics)
        {
            if (playerAC and playerAC->Sms and playerAC->Sms
                ->curWeaponType == wtAgm65 and playerAC->Sms->curWeapon)
            {
                if (playerAC->Sms->MavSubMode == SMSBaseClass::VIS)
                {
                    if (FCC->preDesignate)
                    {
                        display->Line(-size, -size, -size, size);
                        display->Line(-size, -size, size, -size);
                        display->Line(size, size, -size, size);
                        display->Line(size, size, size, -size);
                    }
                    else
                        display->Circle(0.0F, 0.0F, size);
                }
                else if (playerAC->Sms->MavSubMode == SMSBaseClass::PRE)
                {
                    display->Line(-size, -size, -size, size);
                    display->Line(-size, -size, size, -size);
                    display->Line(size, size, -size, size);
                    display->Line(size, size, size, -size);
                }
                else
                {
                    display->Line(0.0F,  0.14F,  0.0F, -0.14F);   // JPG 11 Dec 03 - Let's draw a BORE EO reticle instead of a box
                    display->Line(0.14F,  0.0F, -0.14F,  0.0F);
                }
            }
            else
            {
                display->Line(-size, -size, -size, size);
                display->Line(-size, -size, size, -size);
                display->Line(size, size, -size, size);
                display->Line(size, size, size, -size);

                //MI add a GO STT readout if we're a SARH and not in STT
                if (ownship and g_bRealisticAvionics)
                {
                    if (ownship->Sms and ownship->Sms->curWeapon and ownship->Sms->curWeapon->IsMissile() and 
                        ((MissileClass *)ownship->Sms->GetCurrentWeapon())->GetSeekerType() == SensorClass::RadarHoming and 
                        theRadar and not theRadar->IsSet(RadarDopplerClass::STTingTarget))
                    {
                        display->TextCenter(0.0F, -0.1F, "GO STT", 0);
                    }
                }
            }
        }
        else
        {
            display->Line(-size, -size, -size, size);
            display->Line(-size, -size, size, -size);
            display->Line(size, size, -size, size);
            display->Line(size, size, size, -size);
        }

        display->AdjustOriginInViewport(-xPos, -yPos);
    }
    else
    {
        mlSinCos(&trig, dRoll);
        xPos = offset * trig.sin;
        yPos = offset * trig.cos;
        display->Line(0.0f, 0.0f, xPos, yPos);
        sprintf(tmpStr, "%.0f", (float)acos(cos(az) * cos(el)) * RTD);
        ShiAssert(strlen(tmpStr) < sizeof(tmpStr));
        display->TextRight(-0.075F, display->TextHeight() * 0.5F, tmpStr, 0);
    }
}
void HudClass::DrawAATDBox(void)
{
    if (targetData)
    {
        display->AdjustOriginInViewport(0.0F, (hudWinY[BORESIGHT_CROSS_WINDOW] +
                                               hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));

        DrawTDMarker(targetData->az, targetData->el, targetData->droll, 0.07F);

        display->AdjustOriginInViewport(0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
                                                hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
    }
}

void HudClass::DrawF18HUD(void)
{
    headingPos = High;
    g_bhudAOA = 0;

    //TJL 03/07/04 VVI #45
    //Cobra.  Let's fix this to increment by 10 and add in the delay factor
    int vvi = (((FloatToInt32(-cockpitFlightData.zDot * 60)) + 5) / 10) * 10;

    if (vuxRealTime bitand 0x040)
        vvid = vvi;

    sprintf(tmpStr, "%d", vvid);
    ShiAssert(strlen(tmpStr) < 40);
    DrawWindowString(46, tmpStr);

    //AOA 46
    //HUD AOA Greek Letter Alpha -65 X -13 Y
    if (ownship->af->gearPos < 0.5F or (ownship->af->gearPos > 0.5F and 
                                        (ownship->af->alpha > 10.0F or ownship->af->alpha < 6.0F)))
    {

        display->Line(-0.80F, -0.13F, -0.80F, -0.15F);
        display->Line(-0.80F, -0.15F, -0.78F, -0.17F);
        display->Line(-0.78F, -0.17F, -0.76F, -0.17F);
        display->Line(-0.76F, -0.17F, -0.70F, -0.11F);
        display->Line(-0.80F, -0.13F, -0.78F, -0.11F);
        display->Line(-0.78F, -0.11F, -0.76F, -0.11F);
        display->Line(-0.76F, -0.11F, -0.70F, -0.17F);

        //TJL 11/08/03 HUD AOA numbers
        sprintf(tmpStr, "%02.1f", ownship->af->alpha);
        ShiAssert(strlen(tmpStr) < sizeof(tmpStr));
        DrawWindowString(47, tmpStr);
    }

    //Mach 47
    if (ownship->af->gearPos < 0.5F)
    {
        sprintf(tmpStr, "M %.2f", cockpitFlightData.mach);
        ShiAssert(strlen(tmpStr) < 40);
        DrawWindowString(48, tmpStr);
    }

    //G 48
    if (ownship->af->gearPos < 0.5F or (ownship->af->gearPos > 0.5 and maxGs > 4.0f))
    {
        sprintf(tmpStr, "G %.1f", cockpitFlightData.gs);
        ShiAssert(strlen(tmpStr) < 40);
        DrawWindowString(49, tmpStr);
    }

    //G-Max 49
    if (cockpitFlightData.gs > 4.0F and cockpitFlightData.gs > maxGs)
    {
        maxGs = cockpitFlightData.gs;
    }

    if (maxGs > 4.0F)
    {
        sprintf(tmpStr, "%.1f", maxGs);
        DrawWindowString(50, tmpStr);
    }

    //Waterline
    if (fpmConstrained or ownship->af->gearPos > 0.5)
    {
        display->Line(-0.09F, 0.6F, -0.05F, 0.6F);
        display->Line(-0.05F, 0.6F, -0.03F, 0.57F);
        display->Line(-0.03F, 0.57F, 0.0F, 0.6F);
        display->Line(0.0F, 0.6F, 0.03F, 0.57F);
        display->Line(0.03F, 0.57F, 0.05F, 0.6F);
        display->Line(0.05F, 0.6F, 0.09F, 0.6F);
    }

}

void HudClass::DrawF14HUD(void)
{
    headingPos = High;
    g_bhudAOA = 0;

    //TJL 03/07/04 VVI #45
    //Cobra.  Let's fix this to increment by 10 and add in the delay factor
    int vvi = (((FloatToInt32(-cockpitFlightData.zDot * 60)) + 5) / 10) * 10;

    if (vuxRealTime bitand 0x040)
        vvid = vvi;

    sprintf(tmpStr, "%d", vvid);
    ShiAssert(strlen(tmpStr) < 40);
    DrawWindowString(51, tmpStr);

    //AOA 46
    //HUD AOA Greek Letter Alpha -65 X -13 Y
    if (ownship->af->gearPos < 0.5F or (ownship->af->gearPos > 0.5F and 
                                        (ownship->af->alpha > 17.0F or ownship->af->alpha < 13.0F)))
    {

        display->Line(-0.80F, -0.13F, -0.80F, -0.15F);
        display->Line(-0.80F, -0.15F, -0.78F, -0.17F);
        display->Line(-0.78F, -0.17F, -0.76F, -0.17F);
        display->Line(-0.76F, -0.17F, -0.70F, -0.11F);
        display->Line(-0.80F, -0.13F, -0.78F, -0.11F);
        display->Line(-0.78F, -0.11F, -0.76F, -0.11F);
        display->Line(-0.76F, -0.11F, -0.70F, -0.17F);


        //TJL 11/08/03 HUD AOA numbers
        sprintf(tmpStr, "%02.1f", ownship->af->alpha);
        ShiAssert(strlen(tmpStr) < sizeof(tmpStr));
        DrawWindowString(47, tmpStr);
    }

    //Mach 47
    if (ownship->af->gearPos < 0.5F)
    {
        sprintf(tmpStr, "M %.2f", cockpitFlightData.mach);
        ShiAssert(strlen(tmpStr) < 40);
        DrawWindowString(48, tmpStr);
    }

    //G 48
    if (ownship->af->gearPos < 0.5F or (ownship->af->gearPos > 0.5 and maxGs > 4.0f))
    {
        sprintf(tmpStr, "G %.1f", cockpitFlightData.gs);
        ShiAssert(strlen(tmpStr) < 40);
        DrawWindowString(49, tmpStr);
    }

    //G-Max 49
    if (cockpitFlightData.gs > 4.0F and cockpitFlightData.gs > maxGs)
    {
        maxGs = cockpitFlightData.gs;
    }

    if (maxGs > 4.0F)
    {
        sprintf(tmpStr, "%.1f", maxGs);
        DrawWindowString(50, tmpStr);
    }

    //TJL 03/06/04 F-14 Specific HUD warning per -1

    if ((ownship->af->tefPos > 0.0f or ownship->af->lefPos > 0.0f) and 
        ownship->GetKias() > 225.0f and flash)
    {
        DrawWindowString(12, "RDC SPEED");
    }
    else if (ownship->af->mach > 2.4f and flash)
    {
        DrawWindowString(12, "RDC SPEED");
    }

    //Waterline
    if (fpmConstrained or ownship->af->gearPos > 0.5)
    {
        display->Line(-0.09F, 0.6F, -0.05F, 0.6F);
        display->Line(-0.05F, 0.6F, -0.03F, 0.57F);
        display->Line(-0.03F, 0.57F, 0.0F, 0.6F);
        display->Line(0.0F, 0.6F, 0.03F, 0.57F);
        display->Line(0.03F, 0.57F, 0.05F, 0.6F);
        display->Line(0.05F, 0.6F, 0.09F, 0.6F);
    }

}

void HudClass::DrawF15HUD(void)
{
    headingPos = High;
    g_bhudAOA = 0;

    //TJL 03/07/04 VVI #45
    //Cobra.  Let's fix this to increment by 10 and add in the delay factor
    int vvi = (((FloatToInt32(-cockpitFlightData.zDot * 60)) + 5) / 10) * 10;

    if (vuxRealTime bitand 0x040)
        vvid = vvi;

    sprintf(tmpStr, "%d", vvid);
    ShiAssert(strlen(tmpStr) < 40);
    DrawWindowString(51, tmpStr);

    //AOA 46
    //HUD AOA Greek Letter Alpha -65 X -13 Y

    display->Line(-0.92F, 0.05F, -0.92F, 0.07F);
    display->Line(-0.92F, 0.07F, -0.90F, 0.09F);
    display->Line(-0.90F, 0.09F, -0.88F, 0.09F);
    display->Line(-0.88F, 0.09F, -0.82F, 0.03F);
    display->Line(-0.92F, 0.05F, -0.90F, 0.03F);
    display->Line(-0.90F, 0.03F, -0.88F, 0.03F);
    display->Line(-0.88F, 0.03F, -0.82F, 0.09F);

    //TJL 11/08/03 HUD AOA numbers
    sprintf(tmpStr, "%02.1f", ownship->af->alpha);
    ShiAssert(strlen(tmpStr) < sizeof(tmpStr));
    DrawWindowString(52, tmpStr);

    //Mach 47
    if (ownship->af->gearPos < 0.5F)
    {
        sprintf(tmpStr, "M %.2f", cockpitFlightData.mach);
        ShiAssert(strlen(tmpStr) < 40);
        DrawWindowString(48, tmpStr);
    }

    //G 48
    if (ownship->af->gearPos < 0.5F or (ownship->af->gearPos > 0.5 and maxGs > 4.0f))
    {
        sprintf(tmpStr, "G %.1f", cockpitFlightData.gs);
        ShiAssert(strlen(tmpStr) < 40);
        DrawWindowString(49, tmpStr);
    }

    //G-Max 49
    if (cockpitFlightData.gs > 4.0F and cockpitFlightData.gs > maxGs)
    {
        maxGs = cockpitFlightData.gs;
    }

    if (maxGs > 4.0F)
    {
        sprintf(tmpStr, "%.1f", maxGs);
        DrawWindowString(50, tmpStr);
    }

    //True Airspeed

    sprintf(tmpStr, "T %d", FloatToInt32(cockpitFlightData.vt * FTPSEC_TO_KNOTS));
    ShiAssert(strlen(tmpStr) < 40);
    DrawWindowString(53, tmpStr);



}

void HudClass::DrawA10HUD(void)
{

    //

}
