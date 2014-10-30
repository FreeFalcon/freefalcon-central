#include "Graphics/Include/Render2D.h"
#include "stdhdr.h"
#include "playerop.h"
#include "simveh.h"
#include "campwp.h"
#include "simdrive.h"
#include "hud.h"
#include "radar.h"
#include "otwdrive.h"
#include "cpmanager.h"
#include "icp.h"
#include "navsystem.h"
#include "harmpod.h"
#include "camplist.h"
#include "find.h"
#include "fcc.h"
#include "soundfx.h"
#include "alr56.h"
#include "fault.h"
#include "fack.h"
#include "aircrft.h"
#include "Object.h"
#include "flightdata.h"
#include "radardoppler.h" //MI
#include "AIInput.h" // 2002-03-28 MN
#include "team.h"//Cobra 11/28/04
#include "flight.h"//Cobra



#include "simio.h"  // MD -- 20040111: added for analog cursor support

extern bool g_bEnableColorMfd;
//MI for ICP stuff
extern bool g_bRealisticAvionics;
extern float g_fCursorSpeed;
extern bool g_bIFF;
extern bool g_bMLU;
extern float g_fRadarScale;
extern bool g_bINS;

extern bool g_bRQDFix;
extern bool g_bSetWaypointNumFix;

extern float g_fHSDSymbolSize; //Wombat778 11-13-2003

extern bool g_bHsdStptFix; // MD -- 20040727: added for correction to HSD nav point drawing

#define NUM_RANGE_RINGS    3
#define RING_SPACING       0.4F

FireControlComputer::HsdCnfgStates FireControlComputer::hsdcntlcfg[20] =
{
    {"FCR", HSDNOFCR}, // 0
    {"PRE", HSDNOPRE}, // 1
    {"AIFF", HSDNOAIFF}, // 2
    {"", HSDNONE}, // 3
    {"CNTL", HSDCNTL}, // 4
    {"LINE1", HSDNOLINE1}, // 5
    {"LINE2", HSDNOLINE2}, // 6
    {"LINE3", HSDNOLINE3}, // 7
    {"LINE4", HSDNONE}, // 8
    {"RINGS", HSDNORINGS}, // 9
    {"S-J", HSDNONE}, // 10 S-J
    {"SMS", HSDNONE}, // 11 SMS
    {"", HSDNONE}, // 12
    {"HSD", HSDNONE}, // 13 HSD
    {"SWAP", HSDNONE}, // 14 SWAP
    {"ADLNK", HSDNOADLNK}, // 15
    {"GDLNK", HSDNOGNDLNK}, // 16
    {"NAV3", HSDNONAV3}, // 17
    {"NAV2", HSDNONAV2}, // 18
    {"NAV1", HSDNONAV1}, // 19
};
int FireControlComputer::HsdRangeTbl[HSDRANGESIZE] =   // Range table JPO
{
    15, 30, 60, 120, 240
};
float CalcKIAS(float vt, float alt);
//void evaluate_flight_vc (WayPointClass *, double, double, double, double);
void DrawBullseyeData(VirtualDisplay* display, float cursorX, float cursorY);
//MI
void DrawCursorBullseyeData(VirtualDisplay* display, float cursorX, float cursorY);
void DrawBullseyeCircle(VirtualDisplay* display, float cursorX, float cursorY);
// JPG 16 Jan 04
void DrawSteerPointCursorData(VirtualDisplay* display, FalconEntity* platform, float cursorX, float cursorY);

void FireControlComputer::NavMode(void)
{
    float wpX, wpY, wpZ;
    float dx, dy, dz;
    float rx, ry, rz;
    float ttg = 0.0f;//Cobra 11/12/04
    WayPointClass* curWaypoint = platform->curWaypoint;
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if ( not IsHsdState(HSDCPL))
    {
        if (HSDRangeStepCmd > 0)
            HsdRangeIndex ++;
        else if (HSDRangeStepCmd < 0)
            HsdRangeIndex --;

        if (g_bRealisticAvionics) // lock values
            HsdRangeIndex = max(min(HsdRangeIndex, HSDRANGESIZE - 1), 0);
        else
            HsdRangeIndex = max(min(HsdRangeIndex, HSDRANGESIZE - 2), 0);

        HSDRange = (float)HsdRangeTbl[HsdRangeIndex];

        if (IsHsdState(HSDCEN))
            HSDRange /= 1.5f;
    }
    else
    {
        RadarClass* theRadar = (RadarClass*)FindSensor(platform, SensorClass::Radar);

        if (theRadar)
        {
            HSDRange = theRadar->GetRange();

            if ( not IsHsdState(HSDCEN))
            {
                HSDRange *= 1.5;
            }
        }
        else
        {
            ToggleHsdState(HSDCPL);
        }
    }

    HSDRangeStepCmd = 0;

    if (platform == (SimBaseClass*)(playerAC) and TheHud)
    {
        //evaluate_flight_vc (platform->curWaypoint, platform->XPos (), platform->YPos (), platform->ZPos (), 0); // need speed at the end of here...

        if (curWaypoint)
        {
            // Current waypoint
            curWaypoint->GetLocation(&wpX, &wpY, &wpZ);

            //MI add in INS Drift
            if (g_bINS and g_bRealisticAvionics)
            {
                if (playerAC)
                {
                    wpX += playerAC->GetINSLatDrift();
                    wpY += playerAC->GetINSLongDrift();
                }
            }

            TheHud->waypointX = wpX;
            TheHud->waypointY = wpY;
            TheHud->waypointZ = wpZ;

            dx = wpX - platform->XPos();
            dy = wpY - platform->YPos();
            dz = OTWDriver.GetApproxGroundLevel(wpX, wpY) - platform->ZPos();

            //MI add in INS Offset
            if (g_bINS and g_bRealisticAvionics)
            {
                if (playerAC)
                    dz -= playerAC->GetINSAltOffset();
            }

            // Range to current waypoint
            TheHud->waypointRange = (float)sqrt(dx * dx + dy * dy);

            //MI
            if (g_bRealisticAvionics)
                TheHud->SlantRange = (float)sqrt(dx * dx + dy * dy + dz * dz);

            // Heading error for current waypoint
            TheHud->waypointBearing = (float)atan2(dy, dx) - platform->Yaw();

            if (TheHud->waypointBearing > 180.0F * DTR)
                TheHud->waypointBearing -= 360.0F * DTR;
            else if (TheHud->waypointBearing < -180.0F * DTR)
                TheHud->waypointBearing += 360.0F * DTR;

            // Time to go

            // MD -- 20040219: add concept of pseudo waypoint tracking for GM SP ground stabilized
            if (playerAC and g_bRealisticAvionics)  // MLR 7/17/2004 - Added realistic check
            {
                RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(playerAC, SensorClass::Radar);

                if (theRadar and theRadar->IsSet(RadarDopplerClass::SP) and theRadar->IsSet(RadarDopplerClass::SP_STAB) and theRadar->GMSPWaypt())
                {
                    float x, y, z, dx, dy;
                    theRadar->GMSPWaypt()->GetLocation(&x, &y, &z);
                    dx = x - playerAC->XPos();
                    dy = y - playerAC->YPos();
                    ttg = ((float)sqrt(dx * dx + dy * dy)) / playerAC->GetVt();
                }
                else
                    ttg = ((float)curWaypoint->GetWPArrivalTime() - SimLibElapsedTime) / SEC_TO_MSEC;
            }
            else
                ttg = 0.0f;

            switch (subMode)
            {
                case TimeToGo:
                    if (ttg > 0.0F)
                        TheHud->waypointArrival = ttg;
                    else
                        TheHud->waypointArrival = 0.0F;

                    break;

                    //MI so CCIP and DTOS displays ETE
                case CCIP:
                case DTOSS:
                case ETE:
                case STRAF:
                case Aim120:
                case Aim9:

                    //case Gun: // MLR 2/8/2004 - this is not a valid submode
                    // ETE - delta Time to reach waypoint at current speed
                    if (platform->GetVt() > 2.0F)
                        TheHud->waypointArrival = TheHud->waypointRange / platform->GetVt();
                    else
                        TheHud->waypointArrival = -1.0F;

                    break;

                case ETA:
                    if (platform->GetVt() > 2.0F)
                        TheHud->waypointArrival = SimLibElapsedTime / SEC_TO_MSEC + TheHud->waypointRange / platform->GetVt();
                    else
                        TheHud->waypointArrival = -1.0F;

                    break;

                default:
                    if (ttg > 0.0F)
                        TheHud->waypointArrival = ttg;
                    else
                        TheHud->waypointArrival = 0.0F;

                    break;

            }


            if (g_bRQDFix)
            {

                // Desired Speed
                if (ttg > 0.0f)
                {
                    TheHud->waypointSpeed = TheHud->waypointRange / ttg;
                }
                else
                {
                    TheHud->waypointSpeed = 1700.0F;
                }

                //dpc RQD G/S fix - store waypointSpeed as GND relative value before conversion to CAS
                TheHud->waypointGNDSpeed = TheHud->waypointSpeed * FTPSEC_TO_KNOTS;
                TheHud->waypointSpeed = CalcKIAS(TheHud->waypointSpeed, -platform->ZPos());

                // Clamp as per -34
                TheHud->waypointSpeed = min(max(TheHud->waypointSpeed, 80.0F), 1700.0F);
                //dpc RQD G/S fix - clamp waypointGNDSpeed also
                TheHud->waypointGNDSpeed = min(max(TheHud->waypointGNDSpeed, 80.0F), 1700.0F);

            }
            else
            {
                // Desired Speed
                if (ttg > 0.0f)
                {
                    TheHud->waypointSpeed = TheHud->waypointRange / ttg;
                }
                else
                {
                    TheHud->waypointSpeed = 1700.0F;
                }

                TheHud->waypointSpeed = CalcKIAS(TheHud->waypointSpeed, -platform->ZPos());

                // Clamp as per -34
                TheHud->waypointSpeed = min(max(TheHud->waypointSpeed, 80.0F), 1700.0F);
            }

            rx = platform->dmx[0][0] * dx + platform->dmx[0][1] * dy + platform->dmx[0][2] * dz;
            ry = platform->dmx[1][0] * dx + platform->dmx[1][1] * dy + platform->dmx[1][2] * dz;
            rz = platform->dmx[2][0] * dx + platform->dmx[2][1] * dy + platform->dmx[2][2] * dz;

            TheHud->waypointAz  = (float)atan2(ry, rx);
            TheHud->waypointEl  = (float)atan(-rz / (float)sqrt(rx * rx + ry * ry + .1f));


            TheHud->waypointValid = TRUE;
        }
        else
        {
            TheHud->waypointValid = FALSE;
        }

        //MI send our OA's to the HUD
        if (g_bRealisticAvionics and platform->curWaypoint and platform->curWaypoint->GetWPFlags() bitand WPF_TARGET)
        {
            WayPointClass* curOA = NULL;
            WayPointClass* curVIP = NULL;
            WayPointClass* curVRP = NULL;
            int i;
            float OAXPos;
            float OAYPos;
            float OAZPos;

            ShiAssert(platform->curWaypoint not_eq NULL or not F4IsBadReadPtr(platform->curWaypoint, sizeof(WayPointClass)));

            for (i = 0; i < MAX_DESTOA; i++)
            {
                switch (i)
                {
                    case 1:
                        gNavigationSys->GetDESTOAPoint(i, &curOA);

                        if (curOA)
                        {
                            //Limit it to the TargetWP for the moment
                            if ( not F4IsBadReadPtr(curOA, sizeof(WayPointClass)))
                            {
                                curOA->GetLocation(&OAXPos, &OAYPos, &OAZPos);
                                float dx, dy, dz;
                                dx = OAXPos - platform->XPos();
                                dy = OAYPos - platform->YPos();
                                dz = OTWDriver.GetApproxGroundLevel(OAXPos, OAYPos) - platform->ZPos();
                                dz -= OAZPos;

                                float rx = platform->dmx[0][0] * dx + platform->dmx[0][1] * dy + platform->dmx[0][2] * dz;
                                float ry = platform->dmx[1][0] * dx + platform->dmx[1][1] * dy + platform->dmx[1][2] * dz;
                                float rz = platform->dmx[2][0] * dx + platform->dmx[2][1] * dy + platform->dmx[2][2] * dz;

                                TheHud->OA1Az  = (float)atan2(ry, rx);
                                TheHud->OA1Elev  = (float)atan(-rz / (float)sqrt(rx * rx + ry * ry + .1f));
                                TheHud->OA1Valid = TRUE;
                            }
                            else
                                TheHud->OA1Valid = FALSE;
                        }

                        break;

                    case 2:
                        gNavigationSys->GetDESTOAPoint(i, &curOA);

                        if (curOA)
                        {
                            //Limit it to the TargetWP for the moment
                            if ( not F4IsBadReadPtr(curOA, sizeof(WayPointClass)))
                            {
                                curOA->GetLocation(&OAXPos, &OAYPos, &OAZPos);
                                float dx, dy, dz;
                                dx = OAXPos - platform->XPos();
                                dy = OAYPos - platform->YPos();
                                dz = OTWDriver.GetApproxGroundLevel(OAXPos, OAYPos) - platform->ZPos();
                                dz -= OAZPos;

                                float rx = platform->dmx[0][0] * dx + platform->dmx[0][1] * dy + platform->dmx[0][2] * dz;
                                float ry = platform->dmx[1][0] * dx + platform->dmx[1][1] * dy + platform->dmx[1][2] * dz;
                                float rz = platform->dmx[2][0] * dx + platform->dmx[2][1] * dy + platform->dmx[2][2] * dz;

                                TheHud->OA2Az  = (float)atan2(ry, rx);
                                TheHud->OA2Elev  = (float)atan(-rz / (float)sqrt(rx * rx + ry * ry + .1f));
                                TheHud->OA2Valid = TRUE;
                            }
                            else
                                TheHud->OA2Valid = FALSE;
                        }

                        break;

                    default:
                        break;
                }
            }

            gNavigationSys->GetVIPOAPoint(0, &curVIP);

            if (curVIP)
            {
                if ( not F4IsBadReadPtr(curVIP, sizeof(WayPointClass)))
                {
                    //valid, draw VIP
                    curVIP->GetLocation(&OAXPos, &OAYPos, &OAZPos);
                    float dx, dy, dz;
                    dx = OAXPos - platform->XPos();
                    dy = OAYPos - platform->YPos();
                    dz = OTWDriver.GetApproxGroundLevel(OAXPos, OAYPos) - platform->ZPos();
                    dz -= OAZPos;

                    float rx = platform->dmx[0][0] * dx + platform->dmx[0][1] * dy + platform->dmx[0][2] * dz;
                    float ry = platform->dmx[1][0] * dx + platform->dmx[1][1] * dy + platform->dmx[1][2] * dz;
                    float rz = platform->dmx[2][0] * dx + platform->dmx[2][1] * dy + platform->dmx[2][2] * dz;

                    TheHud->VIPAz  = (float)atan2(ry, rx);
                    TheHud->VIPElev  = (float)atan(-rz / (float)sqrt(rx * rx + ry * ry + .1f));
                    TheHud->VIPValid = TRUE;
                }
                else
                    TheHud->VIPValid = FALSE;
            }

            gNavigationSys->GetVRPOAPoint(0, &curVRP);

            if (curVRP)
            {
                if ( not F4IsBadReadPtr(curVRP, sizeof(WayPointClass)))
                {
                    //valid, draw VRP
                    curVRP->GetLocation(&OAXPos, &OAYPos, &OAZPos);
                    float dx, dy, dz;
                    dx = OAXPos - platform->XPos();
                    dy = OAYPos - platform->YPos();
                    dz = OTWDriver.GetApproxGroundLevel(OAXPos, OAYPos) - platform->ZPos();
                    dz -= OAZPos;

                    float rx = platform->dmx[0][0] * dx + platform->dmx[0][1] * dy + platform->dmx[0][2] * dz;
                    float ry = platform->dmx[1][0] * dx + platform->dmx[1][1] * dy + platform->dmx[1][2] * dz;
                    float rz = platform->dmx[2][0] * dx + platform->dmx[2][1] * dy + platform->dmx[2][2] * dz;

                    TheHud->VRPAz  = (float)atan2(ry, rx);
                    TheHud->VRPElev  = (float)atan(-rz / (float)sqrt(rx * rx + ry * ry + .1f));
                    TheHud->VRPValid = TRUE;
                }
                else
                    TheHud->VRPValid = FALSE;
            }
        }
        else
        {
            TheHud->VIPValid = FALSE;
            TheHud->VRPValid = FALSE;
            TheHud->OA1Valid = FALSE;
            TheHud->OA2Valid = FALSE;
        }

        InitNewStptMode();
        StepPoint();
    }
}

void FireControlComputer::SetStptMode(FCCStptMode mode)
{
    if (platform == (SimVehicleClass*)SimDriver.GetPlayerAircraft())
    {
        mNewStptMode = mode;
    }
}


void FireControlComputer::InitNewStptMode(void)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (platform == (SimVehicleClass*) playerAC)
    {

        if (mNewStptMode not_eq mStptMode)
        {
            // Save the current
            if (mStptMode == FCCWaypoint)
            {
                mpSavedWaypoint = platform->curWaypoint;
                mSavedWayNumber = TheHud->waypointNum;
            }

            // MD -- 20040228: on mode change, cancel any GM SP ground stabilization
            if (mStptMode == FCCGMPseudoPoint)
            {
                RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(platform, SensorClass::Radar);

                if (theRadar->IsSet(RadarDopplerClass::SP) and theRadar->IsSet(RadarDopplerClass::SP_STAB))
                {
                    theRadar->SetGMSPWaypt(NULL);  // clear any existing point data
                    theRadar->ClearFlagBit(RadarDopplerClass::SP_STAB);
                    theRadar->ToggleAGcursorZero();
                }
            }

            // Restore the old
            switch (mNewStptMode)
            {

                case FCCWaypoint:

                    if ( not mpSavedWaypoint)
                    {
                        platform->curWaypoint = platform->waypoint;
                    }
                    else
                    {
                        platform->curWaypoint = mpSavedWaypoint;
                    }

                    TheHud->waypointNum = mSavedWayNumber;
                    mStptMode = mNewStptMode;
                    break;

                case FCCMarkpoint:
                    gNavigationSys->GetMarkWayPoint(&platform->curWaypoint);
                    TheHud->waypointNum = gNavigationSys->GetMarkIndex() + 20;
                    mStptMode = mNewStptMode;
                    break;

                case FCCDLinkpoint:
                    gNavigationSys->GetDLinkWayPoint(&platform->curWaypoint);
                    TheHud->waypointNum = gNavigationSys->GetDLinkIndex() + 30;
                    //mStptMode = FCCWaypoint; //MI outcommented. Causes our STPT to not be restored correctly.
                    mStptMode = FCCDLinkpoint;
                    break;

                case FCCGMPseudoPoint:
                {
                    RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(platform, SensorClass::Radar);
                    mpSavedWaypoint = platform->curWaypoint;
                    mSavedWayNumber = TheHud->waypointNum;
                    platform->curWaypoint = theRadar->GMSPWaypt();
                    mStptMode = mNewStptMode;
                }
                break;
            }
        }

        // mStptMode = mNewStptMode;
    }
}


void FireControlComputer::StepNextWayPoint(void)
{
    WayPointClass* nextWaypoint;
    WayPointClass* curWaypoint;
    int pntNum = TheHud->waypointNum;

    curWaypoint = platform->curWaypoint;

#if 1

    if (curWaypoint) // OW
        nextWaypoint = curWaypoint->GetNextWP();
    else
        nextWaypoint = NULL;

#else
    nextWaypoint = curWaypoint->GetNextWP();
#endif

    if (nextWaypoint)
    {
        platform->curWaypoint = nextWaypoint;
        pntNum ++;
    }
    else
    {
        platform->curWaypoint = platform->waypoint;
        pntNum = 0;
    }

    SetWaypointNum(pntNum);
}

void FireControlComputer::SetWaypointNum(int num)
{
    WayPointClass* curWaypoint = platform->waypoint;
    RadarClass* theRadar = (RadarClass*)FindSensor(platform, SensorClass::Radar);
    float rx, ry, rz;
    int i;

    TheHud->waypointNum = num;
    OTWDriver.pCockpitManager->mpIcp->SetICPWPIndex(TheHud->waypointNum);
    OTWDriver.pCockpitManager->mpIcp->SetICPUpdateFlag(STPT_UPDATE);
    platform->curWaypoint->GetLocation(&rx, &ry, &rz);

    //find the first waypoint
    for (i = 0; curWaypoint->GetPrevWP(); i++)
        curWaypoint = curWaypoint->GetPrevWP();

    //find the corect waypoint
    for (i = 0; i < num and curWaypoint; i++)

        // 2001-07-28 MODIFIED BY S.G. ITS THE *NEXT* WAYPOINT, NOT THE PREVIOUS NOT CHECKED SO COMMENTED OUT
        // 2002-04-18 MN let's add a config file variable to test that later - AI uses this function, too,
        // so we need to be sure it works...
        if ( not g_bSetWaypointNumFix)
            curWaypoint = curWaypoint->GetPrevWP();
        else
            curWaypoint = curWaypoint->GetNextWP();

    if (theRadar and curWaypoint)
    {
        platform->curWaypoint->GetLocation(&rx, &ry, &rz);
        theRadar->SetGroundPoint(rx, ry, rz);
    }

    if (g_bRealisticAvionics)
        OTWDriver.pCockpitManager->mpIcp->ClearStrings();
}



void FireControlComputer::StepPrevWayPoint(void)
{
    WayPointClass* nextWaypoint;
    WayPointClass* curWaypoint;

    curWaypoint = platform->curWaypoint;

    if (curWaypoint)
    {
        nextWaypoint = curWaypoint->GetPrevWP();

        if (nextWaypoint)
        {
            platform->curWaypoint = nextWaypoint;
            TheHud->waypointNum --;
        }
        else
        {
            nextWaypoint = platform->waypoint;
            TheHud->waypointNum = 0;

            while (nextWaypoint and nextWaypoint->GetNextWP())
            {
                nextWaypoint = nextWaypoint->GetNextWP();
                TheHud->waypointNum ++;
            }

            platform->curWaypoint = nextWaypoint;
        }
    }
    else
    {
        TheHud->waypointNum = 0;
    }

    OTWDriver.pCockpitManager->mpIcp->SetICPWPIndex(TheHud->waypointNum);
    OTWDriver.pCockpitManager->mpIcp->SetICPUpdateFlag(STPT_UPDATE);
}



void FireControlComputer::StepPoint(void)
{
    RadarClass* theRadar;
    float rx, ry, rz;

    if (waypointStepCmd == 1 or waypointStepCmd == -1 or waypointStepCmd == 127)
    {

        switch (mStptMode)
        {

            case FCCWaypoint:

                //MI 10/02/02 Why?
                //if ( not platform->OnGround())
            {
                if (waypointStepCmd == 1)
                {
                    StepNextWayPoint();
                }
                else if (waypointStepCmd == -1)
                {
                    StepPrevWayPoint();
                }
            }

            /*else
            {
               waypointStepCmd = 0;
            }*/
            OTWDriver.pCockpitManager->mpIcp->SetICPUpdateFlag(STPT_UPDATE);
            OTWDriver.pCockpitManager->mpIcp->SetICPUpdateFlag(CNI_UPDATE);
            break;

            case FCCMarkpoint:

                if (waypointStepCmd == 1)
                {
                    gNavigationSys->GotoNextMark();
                }
                else if (waypointStepCmd == -1)
                {
                    gNavigationSys->GotoPrevMark();
                }

                gNavigationSys->GetMarkWayPoint(&platform->curWaypoint);
                TheHud->waypointNum = gNavigationSys->GetMarkIndex() + 20;
                OTWDriver.pCockpitManager->mpIcp->SetICPUpdateFlag(MARK_UPDATE);
                OTWDriver.pCockpitManager->mpIcp->SetICPUpdateFlag(CNI_UPDATE);
                break;

            case FCCDLinkpoint:

                if (waypointStepCmd == 1)
                {
                    gNavigationSys->GotoNextDLink();
                }
                else if (waypointStepCmd == -1)
                {
                    gNavigationSys->GotoPrevDLink();
                }

                gNavigationSys->GetDLinkWayPoint(&platform->curWaypoint);
                TheHud->waypointNum = gNavigationSys->GetDLinkIndex() + 30;
                OTWDriver.pCockpitManager->mpIcp->SetICPUpdateFlag(DLINK_UPDATE);
                OTWDriver.pCockpitManager->mpIcp->SetICPUpdateFlag(CNI_UPDATE);
                break;
        }

        if (platform->curWaypoint)
        {
            theRadar = (RadarClass*) FindSensor(platform, SensorClass::Radar);

            if (theRadar)
            {
                platform->curWaypoint->GetLocation(&rx, &ry, &rz);
                theRadar->SetGroundPoint(rx, ry, rz);
            }
        }

        OTWDriver.pCockpitManager->mpIcp-> SetICPWPIndex(TheHud->waypointNum);


        waypointStepCmd = 0;
    }
}



// --------------------------------------------------------------
// FireControlComputer::NavDisplay
// --------------------------------------------------------------

void FireControlComputer::NavDisplay(void)
{
    //MI SOI
    CouldBeSOI = TRUE;

    if (IsSOI)
    {
        display->SetColor(GetMfdColor(MFD_GREEN));
        DrawBorder(); // JPO SOI
    }

    int i;
    char tmpStr[12];
    static const float arrowH = 0.0375f;
    static const float arrowW = 0.0433f;
    float radius;
    int tmpColor = display->Color();
    float basedir = 0.0;
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    //if (g_bEnableColorMfd) DrawBorder();
    if (IsHsdState(HSDFRZ))
    {
        float x, y;
        y = (frz_x - platform->XPos()) * FT_TO_NM / HSDRange;
        x = (frz_y - platform->YPos()) * FT_TO_NM / HSDRange;

        if ( not IsHsdState(HSDCEN))
            y -= 0.4f;

        display->AdjustOriginInViewport(x, y);
        basedir = platform->Yaw() - frz_dir;
        display->AdjustRotationAboutOrigin(basedir);
    }
    else if ( not IsHsdState(HSDCEN)) // JPO depressed view
        display->AdjustOriginInViewport(0.0F, -0.4F);

    //Wombat778 11-10-2003 Added to center on cursor in EXP modes

    if ((HSDZoom > 0) and IsSOI)
    {
        if ( not IsHsdState(HSDCEN))
        {
            if (HSDZoom == 2)
                display->AdjustOriginInViewport(-HSDXPos, (-HSDYPos) - 0.4F); //Adjust to the previously set HSDXPos and HSDYPos (set when zooming)
            else
                display->AdjustOriginInViewport(-HSDXPos, (-HSDYPos) - 1.2F); //Should only get here if the HSDZoom is 4
        }
        else
            display->AdjustOriginInViewport(-HSDXPos, -HSDYPos);
    }

    //Wombat778 11-10-2003 End of added code


    // RV - RED - WARNING...
    // THIS IS KINDA A HACK - The 2D MFDs may be stretched by pit scaling
    // Rings would became ovals ( as it should be ) but later they r turned by AC Yaw...
    // This make the Oval to physically turn...
    // To avoid this, we remove ANY ASPECT RATION SCALING in the NAV MODE MFD making Scale X = Scale Y
    // saving it and restoring after gfx Draw
    // This is the fastest and more harmless way to fix till new code
    // and works fine, as the draw is then stretched by Texture drawing...
    float OldScaleX = display->scaleX, OldScaleY = display->scaleY;
    display->scaleY = display->scaleX;

    // Add ownship marker
    display->SetColor(GetMfdColor(MFD_OWNSHIP));
    display->Line(0.0F, 0.0F, 0.0F, -0.15F);  // main fuselage
    display->Line(0.05F, -0.05F, -0.05F, -0.05F);  // wings
    display->Line(0.02F, -0.12F, -0.02F, -0.12F);  // tail


    // Rotate for heading
    display->AdjustRotationAboutOrigin(-platform->Yaw());

    //MI changed for Zoom
    if ( not g_bRealisticAvionics)
    {
        if ( not IsHsdState(HSDNORINGS) and not IsHsdState(HSDFRZ))
        {
            display->SetColor(GetMfdColor(MFD_LINES));
            // Add Cardinal Headings
            int maxrings = NUM_RANGE_RINGS;

            if (IsHsdState(HSDCEN))
                maxrings --;

            for (i = 0; i < maxrings; i++)
            {
                radius = (float)(i + 1) / maxrings;
                display->Circle(0.0F, 0.0F, radius);
                display->Line(0.0F,  radius + 0.05F,
                              0.0F,  radius - 0.05F);
                display->Line(0.0F, -radius + 0.05F,
                              0.0F, -radius - 0.05F);
                display->Line(radius + 0.05F,
                              0.0F,  radius - 0.05F,  0.0F);
                display->Line(-radius + 0.05F,
                              0.0F, -radius - 0.05F, 0.0F);
            }

            // Add North Arrow
            //MI
            if (g_bRealisticAvionics and g_bINS)
            {
                if (playerAC and playerAC->INSState(AircraftClass::INS_HSD_STUFF))
                    display->Tri(-0.05F, 1.0f / maxrings, 0.05F, 1.0f / maxrings, 0.0F, 1.0f / maxrings + 0.1f);
            }
            else
                display->Tri(-0.05F, 1.0f / maxrings, 0.05F, 1.0f / maxrings, 0.0F, 1.0f / maxrings + 0.1f);
        }
    }
    else
    {
        //MI this is not here if we have it expanded
        if ( not IsHsdState(HSDNORINGS) and not IsHsdState(HSDFRZ) and HSDZoom == 0)
        {
            display->SetColor(GetMfdColor(MFD_LINES));
            // Add Cardinal Headings
            int maxrings = NUM_RANGE_RINGS;

            if (IsHsdState(HSDCEN))
                maxrings --;

            for (i = 0; i < maxrings; i++)
            {
                radius = (float)(i + 1) / maxrings;
                display->Circle(0.0F, 0.0F, radius);
                display->Line(0.0F,  radius + 0.05F,
                              0.0F,  radius - 0.05F);
                display->Line(0.0F, -radius + 0.05F,
                              0.0F, -radius - 0.05F);
                display->Line(radius + 0.05F,
                              0.0F,  radius - 0.05F,  0.0F);
                display->Line(-radius + 0.05F,
                              0.0F, -radius - 0.05F, 0.0F);
            }

            // Add North Arrow
            //MI
            if (g_bRealisticAvionics and g_bINS)
            {
                if (playerAC and playerAC->INSState(AircraftClass::INS_HSD_STUFF))
                    display->Tri(-0.05F, 1.0f / maxrings, 0.05F, 1.0f / maxrings, 0.0F, 1.0f / maxrings + 0.1f);
            }
            else
                display->Tri(-0.05F, 1.0f / maxrings, 0.05F, 1.0f / maxrings, 0.0F, 1.0f / maxrings + 0.1f);
        }
    }

    display->SetColor(GetMfdColor(MFD_ROUTES));

    if ( not IsHsdState(HSDNONAV1))
        DrawNavPoints();

    if ( not IsHsdState(HSDNOPRE) and g_bRealisticAvionics)
        DrawPPThreats();

    if (PlayerOptions.GetAvionicsType() not_eq ATEasy)
    {
        display->SetColor(GetMfdColor(MFD_LINES));

        if ( not IsHsdState(HSDNOLINE2))
            DrawFLOT();

        // Draw additional data
        if ( not IsHsdState(HSDNOFCR))
        {
            display->ZeroRotationAboutOrigin();
            display->AdjustRotationAboutOrigin(basedir);
            display->SetColor(GetMfdColor(MFD_CURSOR));

            if (g_bRealisticAvionics) //MI changed
            {
                if (HSDZoom == 0 and not IsSOI)
                {
                    DrawGhostCursor();
                    display->SetColor(GetMfdColor(MFD_SWEEP));
                    DrawScanVolume();
                }
            }
            else
            {
                DrawGhostCursor();
                display->SetColor(GetMfdColor(MFD_SWEEP));
                DrawScanVolume();
            }
        }

        if (g_bRealisticAvionics)
        {
            display->SetColor(GetMfdColor(MFD_UNKNOWN));
            display->ZeroRotationAboutOrigin();
            display->AdjustRotationAboutOrigin(basedir);

            DrawBuggedTarget();
            //if ( not IsHsdState(HSDNOADLNK)) Cobra not needed we do this below in DrawWingmen
            // {
            display->ZeroRotationAboutOrigin();
            display->AdjustRotationAboutOrigin(basedir);
            DrawWingmen();
            //}

            //Cobra 11/27/04 AIFF
            if ( not IsHsdState(HSDNOAIFF))
            {
                display->SetColor(GetMfdColor(MFD_IFFFREIENDLY));
                display->ZeroRotationAboutOrigin();
                display->AdjustRotationAboutOrigin(basedir);
                DrawAIFF();
            }
        }

        display->ZeroRotationAboutOrigin();
        display->CenterOriginInViewport();
        display->SetColor(GetMfdColor(MFD_BULLSEYE));
        DrawBullseye();
    }
    else
    {
        display->ZeroRotationAboutOrigin();
        display->CenterOriginInViewport();
    }

    // RV - RED - WARNING...
    // THIS IS KINDA A HACK - The 2D MFDs may be stretched by pit scaling
    // Restore here the ORIGINAL MFDs Scaling
    display->scaleX = OldScaleX;
    display->scaleY = OldScaleY;

    if (g_bRealisticAvionics)
    {
        // JPO Extras for MLU MFD
        display->SetColor(GetMfdColor(MFD_LABELS));
        BottomRow();

        if (IsHsdState(HSDCNTL))
        {
            for (i = 0; i < 20; i ++)
            {
                if (hsdcntlcfg[i].mode not_eq HSDNONE)
                {
                    if (hsdcntlcfg[i].mode == HSDCNTL)
                        LabelButton(i, hsdcntlcfg[i].label, NULL, TRUE);
                    else
                        LabelButton(i, hsdcntlcfg[i].label, NULL, not IsHsdState(hsdcntlcfg[i].mode));
                }
            }
        }
        else
        {
            LabelButton(0, IsHsdState(HSDCEN) ? "CEN" : "DEP");
            LabelButton(1, IsHsdState(HSDCPL) ? "CPL" : "DCPL");

            //MI
            if (HSDZoom == 0)
                LabelButton(2, "NORM");
            else if (HSDZoom == 2)
                LabelButton(2, "EXP1");
            else
                LabelButton(2, "EXP2");

            LabelButton(4, "CNTL");
            LabelButton(6, "FRZ", NULL, IsHsdState(HSDFRZ));
        }

        display->SetColor(tmpColor);
    }
    else
    {
        display->SetColor(tmpColor);
        LabelButton(2,  "NORM");
        LabelButton(4,  "CNTL");
        LabelButton(11, "SMS");
        LabelButton(13, "HSD", NULL, 1);
        LabelButton(14, "SWAP");
    }

    /*--------*/
    /* arrows */
    /*--------*/

    // Add range and arrows - text is always present
    //MI this is not here if we have it expanded
    if (g_bRealisticAvionics)
    {
        if (IsSOI)
        {
            //draw the cursor here
            HSDDisplay();
        }

        if (HSDZoom not_eq 0)
            return;
    }

    sprintf(tmpStr, "%.0f", HSDRange);
    ShiAssert(strlen(tmpStr) < sizeof(tmpStr));
    float x18, y18;
    float x19, y19;
    GetButtonPos(18, &x18, &y18);
    GetButtonPos(19, &x19, &y19);
    float ymid = y18 + (y19 - y18) / 2;
    display->TextLeftVertical(x18, ymid, tmpStr);

    if ( not IsHsdState(HSDCPL) and not IsHsdState(HSDCNTL) and not IsHsdState(HSDFRZ))
    {
        /*----------*/
        /* up arrow */
        /*----------*/
        display->AdjustOriginInViewport(x19 + arrowW, y19 + arrowH / 2);

        if (HsdRangeIndex < HSDRANGESIZE - 1)
            display->Tri(0.0F, arrowH, arrowW, -arrowH, -arrowW, -arrowH);

        /*------------*/
        /* down arrow */
        /*------------*/
        display->CenterOriginInViewport();
        display->AdjustOriginInViewport(x18 + arrowW, y18 - arrowH / 2);

        if (HsdRangeIndex > 0)
            display->Tri(0.0F, -arrowH, arrowW, arrowH, -arrowW, arrowH);
    }

    display->CenterOriginInViewport();



}
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////



// --------------------------------------------------------------
// FireControlComputer::DrawNavPoints
// --------------------------------------------------------------

void FireControlComputer::DrawNavPoints(void)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    //MI
    if (g_bRealisticAvionics and g_bINS)
    {
        if (playerAC and not playerAC->INSState(AircraftClass::INS_HSD_STUFF))
            return;
    }

    if (g_bHsdStptFix)
    {
        // start with STPTs
        DrawWayPoints();

        // ...then do DL point(s)
        int tmpColor = display->Color();
        display->SetColor(GetMfdColor(MFD_DATALINK));
        DrawLinkPoints();
        display->SetColor(tmpColor);

        // ...and MARKs if there are any.
        tmpColor = display->Color();
        display->SetColor(GetMfdColor(MFD_DATALINK));
        DrawMarkPoints();
        display->SetColor(tmpColor);
    }
    else
    {

        switch (mStptMode)
        {
                // MD -- 20040228: do this when you are ground stabilized in GM SP as well
            case FCCGMPseudoPoint:
            case FCCWaypoint:
                DrawWayPoints();
                //MI OA stuff
                //MI 08/27/01 disabled. this stuff doesn't get displayed on the HSD AFAIK now
                /*if(g_bRealisticAvionics)
                {
                DrawDESTOAPoints();
                DrawVIPOAPoints();
                DrawVRPOAPoints();
                }*/
                break;

            case FCCDLinkpoint:
            {
                int tmpColor = display->Color();
                display->SetColor(GetMfdColor(MFD_DATALINK));
                DrawLinkPoints();
                display->SetColor(tmpColor);
            }
            break;

            case FCCMarkpoint:
            {
                int tmpColor = display->Color();
                display->SetColor(GetMfdColor(MFD_DATALINK));
                DrawMarkPoints();
                display->SetColor(tmpColor);
            }
            break;
        }
    }
}
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////


// --------------------------------------------------------------
// FireControlComputer::DrawWayPoints
// --------------------------------------------------------------

void FireControlComputer::DrawWayPoints()
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    //MI
    if (g_bRealisticAvionics and g_bINS)
    {
        if (playerAC and not playerAC->INSState(AircraftClass::INS_HSD_STUFF))
            return;
    }

    float x2;
    float y2;
    float displayX;
    float displayY;
    WayPointClass* curWaypoint;

    curWaypoint = platform->waypoint;

    if (curWaypoint)
    {

        MapWaypointToDisplay(curWaypoint, &x2, &y2);
        curWaypoint = curWaypoint->GetNextWP();

        while (curWaypoint)
        {

            MapWaypointToDisplay(curWaypoint, &displayX, &displayY);
            DrawPointPair(curWaypoint, x2, y2, displayX, displayY);

            x2 = displayX;
            y2 = displayY;
            curWaypoint = curWaypoint->GetNextWP();
        }
    }
}
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////


// --------------------------------------------------------------
// FireControlComputer::DrawMarkPoints
// --------------------------------------------------------------

void FireControlComputer::DrawMarkPoints(void)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    //MI
    if (g_bRealisticAvionics and g_bINS)
    {
        if (playerAC and not playerAC->INSState(AircraftClass::INS_HSD_STUFF))
            return;
    }

    WayPointClass* curWaypoint = NULL;
    int i;
    float displayX;
    float displayY;

    for (i = 0; i < MAX_MARKPOINTS; i++)
    {
        gNavigationSys->GetMarkWayPoint(i, &curWaypoint);

        if (curWaypoint)
        {
            MapWaypointToDisplay(curWaypoint, &displayX, &displayY);

            if (g_bRealisticAvionics)
                DrawMarkSymbol(displayX, displayY, 0);
            else
                DrawPointSymbol(curWaypoint, displayX, displayY);
        }
    }
}
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////



// --------------------------------------------------------------
// FireControlComputer::DrawLinkPoints
// --------------------------------------------------------------

void FireControlComputer::DrawLinkPoints(void)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    //MI
    if (g_bRealisticAvionics and g_bINS)
    {
        if (playerAC and not playerAC->INSState(AircraftClass::INS_HSD_STUFF))
            return;
    }

    WayPointClass* prevWaypoint = NULL;
    WayPointClass* curWaypoint = NULL;
    int i = 0;
    float x2 = 0.0F;
    float y2 = 0.0F;
    float displayX = 0.0F;
    float displayY = 0.0F;

    gNavigationSys->GetDLinkWayPoint(0, &prevWaypoint);

    if (prevWaypoint)
    {
        MapWaypointToDisplay(prevWaypoint, &x2, &y2);

        for (i = 1; i < MAX_DLINKPOINTS; i++)
        {
            gNavigationSys->GetDLinkWayPoint(i, &curWaypoint);

            if (curWaypoint)
            {
                MapWaypointToDisplay(curWaypoint, &displayX, &displayY);

                if (prevWaypoint)
                {
                    DrawPointPair(curWaypoint, x2, y2, displayX, displayY);
                }
                else
                {
                    DrawPointSymbol(curWaypoint, displayX, displayY);
                }
            }

            x2 = displayX;
            y2 = displayY;
            prevWaypoint = curWaypoint;
        }
    }
}
//MI
void FireControlComputer::DrawDESTOAPoints(void)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    //MI
    if (g_bRealisticAvionics and g_bINS)
    {
        if (playerAC and not playerAC->INSState(AircraftClass::INS_HSD_STUFF))
            return;
    }

    WayPointClass* curWaypoint = NULL;
    int i;
    float displayX;
    float displayY;

    for (i = 0; i < MAX_DESTOA; i++)
    {
        gNavigationSys->GetDESTOAPoint(i, &curWaypoint);

        if (curWaypoint)
        {
            //only do this is this is our target
            if (platform->curWaypoint->GetWPFlags() bitand WPF_TARGET)
            {
                MapWaypointToDisplay(curWaypoint, &displayX, &displayY);

                if (g_bRealisticAvionics)
                    DrawDESTOASymbol(displayX, displayY);
            }
        }
    }
}
void FireControlComputer::DrawVIPOAPoints(void)
{
    //MI
    /*if(g_bRealisticAvionics and g_bINS)
    {
     if(playerAC and not playerAC->INSState(AircraftClass::INS_HSD_NAV))
     return;
    }*/

    WayPointClass* curWaypoint = NULL;
    int i;
    float displayX;
    float displayY;

    for (i = 0; i < MAX_VIPOA; i++)
    {
        gNavigationSys->GetVIPOAPoint(i, &curWaypoint);

        if (curWaypoint)
        {
            //only do this is this is our target
            if (platform->curWaypoint->GetWPFlags() bitand WPF_TARGET)
            {
                MapWaypointToDisplay(curWaypoint, &displayX, &displayY);

                if (g_bRealisticAvionics)
                    DrawVIPOASymbol(displayX, displayY);
            }
        }
    }
}
void FireControlComputer::DrawVRPOAPoints(void)
{
    //MI
    /*if(g_bRealisticAvionics and g_bINS)
    {
     if(playerAC and not playerAC->INSState(AircraftClass::INS_HSD_NAV))
     return;
    }*/

    WayPointClass* curWaypoint = NULL;
    int i;
    float displayX;
    float displayY;

    for (i = 0; i < MAX_VRPOA; i++)
    {
        gNavigationSys->GetVRPOAPoint(i, &curWaypoint);

        if (curWaypoint)
        {
            //only do this is this is our target
            if (platform->curWaypoint->GetWPFlags() bitand WPF_TARGET)
            {
                MapWaypointToDisplay(curWaypoint, &displayX, &displayY);

                if (g_bRealisticAvionics)
                    DrawVRPOASymbol(displayX, displayY);
            }
        }
    }
}
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////



// --------------------------------------------------------------
// FireControlComputer::MapWaypointToDisplay
// --------------------------------------------------------------

void FireControlComputer::MapWaypointToDisplay(WayPointClass* pwaypoint, float* h, float* v)
{
    float wpX;
    float wpY;
    float wpZ;
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    pwaypoint->GetLocation(&wpX, &wpY, &wpZ);

    //MI
    if ( not g_bRealisticAvionics)
    {
        *v = (wpX - platform->XPos()) * FT_TO_NM / HSDRange;
        *h = (wpY - platform->YPos()) * FT_TO_NM / HSDRange;
    }
    else
    {
        //MI add in INS Drift
        if (g_bINS)
        {
            if (playerAC)
            {
                wpX += playerAC->GetINSLatDrift();
                wpY += playerAC->GetINSLongDrift();
            }
        }

        if (HSDZoom > 0)
        {
            *v = ((wpX - platform->XPos()) * FT_TO_NM / HSDRange) * HSDZoom;
            *h = ((wpY - platform->YPos()) * FT_TO_NM / HSDRange) * HSDZoom;
        }
        else
        {
            *v = (wpX - platform->XPos()) * FT_TO_NM / HSDRange;
            *h = (wpY - platform->YPos()) * FT_TO_NM / HSDRange;
        }
    }
}
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////



// --------------------------------------------------------------
// FireControlComputer::DrawPointPair
// --------------------------------------------------------------

void FireControlComputer::DrawPointPair(WayPointClass* curWaypoint, float x2, float y2, float displayX, float displayY)
{
    if ( not (curWaypoint->GetWPFlags() bitand WPF_ALTERNATE))
        display->Line(x2, y2, displayX, displayY);

    DrawPointSymbol(curWaypoint, displayX, displayY);
}
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////



// --------------------------------------------------------------
// FireControlComputer::DrawPointSymbol
// --------------------------------------------------------------

void FireControlComputer::DrawPointSymbol(WayPointClass* curWaypoint, float displayX, float displayY)
{
    if ( not g_bRealisticAvionics)
        return;

    int wpFlags;
    WayPointClass* theWaypt;

    RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(platform, SensorClass::Radar);

    if (theRadar and theRadar->IsSet(RadarDopplerClass::SP) and theRadar->IsSet(RadarDopplerClass::SP_STAB))
        theWaypt = mpSavedWaypoint;
    else
        theWaypt = ((SimVehicleClass*)platform)->curWaypoint;

    if ((curWaypoint not_eq theWaypt) or (vuxRealTime bitand 0x200))
    {

        wpFlags = curWaypoint->GetWPFlags();

        if (wpFlags bitand WPF_TARGET)
        {
            DrawTGTSymbol(displayX, displayY);
        }
        else if (wpFlags bitand WPF_IP)
        {
            DrawIPSymbol(displayX, displayY);
        }
        else
        {
            display->Circle(displayX, displayY, g_fHSDSymbolSize);   //Wombat778 11-13-2003 Make the symbol size configurable
        }
    }
}
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////


//static const float SMDELTA = 0.05f; // small distance for symbols //Wombat778 11-13-2003 removed to set in each function from g_bHSDSymbolSize
static const float LGDELTA = 0.1f; // larger distance for symbols
// --------------------------------------------------------------
// FireControlComputer::DrawTGTSymbol
// --------------------------------------------------------------

void FireControlComputer::DrawTGTSymbol(float displayX, float displayY)
{
    float SMDELTA = g_fHSDSymbolSize; //Wombat778 11-13-2003 Set SMDELTA to the external variable
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    //MI
    if (g_bRealisticAvionics and g_bINS)
    {
        if (playerAC and not playerAC->INSState(AircraftClass::INS_HSD_STUFF))
            return;
    }

    display->Line(displayX + SMDELTA, displayY - SMDELTA, displayX, displayY + SMDELTA);
    display->Line(displayX - SMDELTA, displayY - SMDELTA, displayX, displayY + SMDELTA);
    display->Line(displayX + SMDELTA, displayY - SMDELTA, displayX - SMDELTA, displayY - SMDELTA);
}
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////



// --------------------------------------------------------------
// FireControlComputer::DrawIPSymbol
// --------------------------------------------------------------

void FireControlComputer::DrawIPSymbol(float displayX, float displayY)
{
    float SMDELTA = g_fHSDSymbolSize; //Wombat778 11-13-2003 Set SMDELTA to the external variable
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    //MI
    if (g_bRealisticAvionics and g_bINS)
    {
        if (playerAC and not playerAC->INSState(AircraftClass::INS_HSD_STUFF))
            return;
    }

    display->Line(displayX + SMDELTA, displayY + SMDELTA, displayX + SMDELTA, displayY - SMDELTA);
    display->Line(displayX - SMDELTA, displayY + SMDELTA, displayX - SMDELTA, displayY - SMDELTA);
    display->Line(displayX + SMDELTA, displayY + SMDELTA, displayX - SMDELTA, displayY + SMDELTA);
    display->Line(displayX + SMDELTA, displayY - SMDELTA, displayX - SMDELTA, displayY - SMDELTA);
}

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
// --------------------------------------------------------------
// FireControlComputer::DrawMarkSymbol
// --------------------------------------------------------------

void FireControlComputer::DrawMarkSymbol(float displayX, float displayY, int type)
{
    float SMDELTA = g_fHSDSymbolSize; //Wombat778 11-13-2003 Set SMDELTA to the external variable
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    //MI
    if (g_bRealisticAvionics and g_bINS)
    {
        if (playerAC and not playerAC->INSState(AircraftClass::INS_HSD_STUFF))
            return;
    }

    float dist = type == 0 ? SMDELTA : LGDELTA;
    display->Line(displayX - dist, displayY - dist, displayX + dist, displayY + dist);
    display->Line(displayX - dist, displayY + dist, displayX + dist, displayY - dist);
}
//MI
const static float TriangleDist = 0.05F;
const static float CircleDia = 0.05F;
void FireControlComputer::DrawDESTOASymbol(float displayX, float displayY)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    //MI
    if (g_bRealisticAvionics and g_bINS)
    {
        if (playerAC and not playerAC->INSState(AircraftClass::INS_HSD_STUFF))
            return;
    }

    //put our input as the center of the triangle
    float XOffset = (TriangleDist * 1.3F) / 2;
    float YOffset = (TriangleDist * 1.75F) / 2;
    float X1 = displayX - XOffset;
    float Y1 = displayY - YOffset;
    display->Line(X1, Y1, X1 + XOffset * 2, Y1);
    display->Line(X1 + XOffset * 2, Y1, X1 + XOffset, Y1 + YOffset * 2);
    display->Line(X1 + XOffset, Y1 + YOffset * 2, X1, Y1);
}
void FireControlComputer::DrawVIPOASymbol(float displayX, float displayY)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    //MI
    if (g_bRealisticAvionics and g_bINS)
    {
        if (playerAC and not playerAC->INSState(AircraftClass::INS_HSD_STUFF))
            return;
    }

    display->Circle(displayX, displayY, CircleDia);
}
void FireControlComputer::DrawVRPOASymbol(float displayX, float displayY)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    //MI
    if (g_bRealisticAvionics and g_bINS)
    {
        if (playerAC and not playerAC->INSState(AircraftClass::INS_HSD_STUFF))
            return;
    }

    display->Circle(displayX, displayY, CircleDia);
}

void FireControlComputer::DrawFLOT(void)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    //MI
    if (g_bRealisticAvionics and g_bINS)
    {
        if (playerAC and not playerAC->INSState(AircraftClass::INS_HSD_STUFF))
            return;
    }

    float xPos, yPos;
    float x = 0, y = 0;
    class ListElementClass* curElement;
    GridIndex flotX, flotY;
    int tmpColor = display->Color();

    // Add the bullseye
    TheCampaign.GetBullseyeSimLocation(&xPos, &yPos);

    //MI changed
    if (g_bRealisticAvionics)
    {
        //MI add in INS Drift
        if (g_bINS)
        {
            if (playerAC)
            {
                xPos += playerAC->GetINSLatDrift();
                yPos += playerAC->GetINSLongDrift();
            }
        }

        if (HSDZoom == 0)
        {
            y = (xPos - platform->XPos()) * FT_TO_NM / HSDRange;
            x = (yPos - platform->YPos()) * FT_TO_NM / HSDRange;
        }
        else
        {
            y = (xPos - platform->XPos()) * FT_TO_NM / HSDRange * HSDZoom;
            x = (yPos - platform->YPos()) * FT_TO_NM / HSDRange * HSDZoom;
        }
    }
    else
    {
        y = (xPos - platform->XPos()) * FT_TO_NM / HSDRange;
        x = (yPos - platform->YPos()) * FT_TO_NM / HSDRange;
    }

    display->SetColor(GetMfdColor(MFD_BULLSEYE));
    display->Circle(x, y, 0.07F);
    display->Circle(x, y, 0.04F);
    display->Circle(x, y, 0.01F);

    // Draw the Flot
    if (FLOTList)
    {
        curElement = FLOTList->GetFirstElement();

        if (curElement)
        {
            tmpColor = display->Color();

            if ( not g_bEnableColorMfd)
                display->SetColor(0xFF008000);
            else
                display->SetColor(GetMfdColor(MFD_GREY));

            UnpackXY(curElement->GetUserData(), &flotX, &flotY);
            curElement = curElement->GetNext();

            float FlotX = GridToSim(flotX);
            float FlotY = GridToSim(flotY);

            //MI changed
            if (g_bRealisticAvionics)
            {
                //MI add in INS Drift
                if (g_bINS)
                {
                    if (playerAC)
                    {
                        FlotX += playerAC->GetINSLatDrift();
                        FlotY += playerAC->GetINSLongDrift();
                    }
                }

                if (HSDZoom == 0)
                {
                    y = (FlotY - platform->XPos()) * FT_TO_NM / HSDRange;
                    x = (FlotX - platform->YPos()) * FT_TO_NM / HSDRange;
                }
                else
                {
                    y = (FlotY - platform->XPos()) * FT_TO_NM / HSDRange * HSDZoom;
                    x = (FlotX - platform->YPos()) * FT_TO_NM / HSDRange * HSDZoom;
                }
            }
            else
            {
                y = (FlotY - platform->XPos()) * FT_TO_NM / HSDRange;
                x = (FlotX - platform->YPos()) * FT_TO_NM / HSDRange;
            }

            while (curElement)
            {
                UnpackXY(curElement->GetUserData(), &flotX, &flotY);
                curElement = curElement->GetNext();

                //MI changed
                if (g_bRealisticAvionics)
                {
                    if (HSDZoom == 0)
                    {
                        yPos = (GridToSim(flotY) - platform->XPos()) * FT_TO_NM / HSDRange;
                        xPos = (GridToSim(flotX) - platform->YPos()) * FT_TO_NM / HSDRange;
                    }
                    else
                    {
                        yPos = (GridToSim(flotY) - platform->XPos()) * FT_TO_NM / HSDRange * HSDZoom;
                        xPos = (GridToSim(flotX) - platform->YPos()) * FT_TO_NM / HSDRange * HSDZoom;
                    }
                }
                else
                {
                    yPos = (GridToSim(flotY) - platform->XPos()) * FT_TO_NM / HSDRange;
                    xPos = (GridToSim(flotX) - platform->YPos()) * FT_TO_NM / HSDRange;
                }

                if (DistSqu(x, y, xPos, yPos) < FLOTDrawDistance) // when FLOT points are too far away from each other, don't draw a line
                    display->Line(x, y, xPos, yPos);

                x = xPos;
                y = yPos;
            }

            display->SetColor(tmpColor);
        }
    }
}

void FireControlComputer::DrawBullseye(void)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    //MI
    /*if(g_bRealisticAvionics and g_bINS)
    {
     if(playerAC and not playerAC->INSState(AircraftClass::INS_HSD_STUFF))
     return;
    }*/

    RadarClass* theRadar = (RadarClass*)FindSensor(platform, SensorClass::Radar);
    float cursorX, cursorY;

    display->SetColor(GetMfdColor(MFD_BULLSEYE));

    if (theRadar)
    {
        theRadar->GetCursorPosition(&cursorX, &cursorY);

        //MI
        if ( not g_bRealisticAvionics)
            DrawBullseyeData(display, cursorX, cursorY);
        else
        {
            if (OTWDriver.pCockpitManager and OTWDriver.pCockpitManager->mpIcp and 
                OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
            {
                DrawBullseyeCircle(display, cursorX, cursorY);
                DrawCursorBullseyeData(display, cursorX, cursorY);
            }
            else
            {
                DrawReference(playerAC);
                DrawSteerPointCursorData(display, platform, cursorX, cursorY);
            }

            //Booster 15/09/2004 - Draw Pull Up cross on MFD-SMS if ground Collision
            if (playerAC->mFaults->GetFault(alt_low))
                DrawRedBreak(display);

            // RV - I-Hawk
            // Check, if 1 of the MFDs is showing TGP and the attitude warning is set, show it...
            // (works on all MFDs and not only here)
            for (int i = 0; i < 4; i++)
            {
                if ((MfdDisplay[i])->GetTGPWarning() and (MfdDisplay[i])->CurMode() == MFDClass::TGPMode)
                {
                    TGPAttitudeWarning(display);
                    break;
                }
            }
        }
    }
}


void FireControlComputer::DrawPPThreats(void)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    //MI
    if (g_bRealisticAvionics and g_bINS)
    {
        if (playerAC and not playerAC->INSState(AircraftClass::INS_HSD_STUFF))
            return;
    }

    mlTrig trig;
    float x2, y2, z2;
    float displayX, displayY;
    // we now use our own list of preplanned targets.
    // will consolidate with HTS soon.
    GroundListElement *gp;
    mlSinCos(&trig, platform->Yaw());
    float myX, myY, myZ;
    myX = platform->XPos();
    myY = platform->YPos();
    myZ = platform->ZPos();
    UpdatePlanned();

    // Draw all known emmitters
    for (gp = GetFirstGroundElement(); gp; gp = gp->GetNext())
    {
        gp->HandoffBaseObject();

        if (gp->BaseObject() == NULL) continue; // probably dead.

        y2 = (gp->BaseObject()->XPos() - myX) * FT_TO_NM;
        x2 = (gp->BaseObject()->YPos() - myY) * FT_TO_NM;
        z2 = (gp->BaseObject()->ZPos() - myZ) * FT_TO_NM;

        float targetdist = sqrt(x2 * x2 + y2 * y2 + z2 * z2);

        if (targetdist > gp->range + HSDRange) // JPO - optimise for those too far away.
            continue;

        if (targetdist < gp->range)
            display->SetColor(GetMfdColor(MFD_PREPLAN_INRANGE));
        else
            display->SetColor(GetMfdColor(MFD_PREPLAN));

        //MI changed
        if (g_bRealisticAvionics)
        {
            //MI add in INS Drift
            if (g_bINS)
            {
                if (playerAC)
                {
                    y2 += (playerAC->GetINSLatDrift() * FT_TO_NM);
                    x2 += (playerAC->GetINSLongDrift() * FT_TO_NM);
                }
            }

            if (HSDZoom == 0)
            {
                y2 /= HSDRange;
                x2 /= HSDRange;
            }
            else
            {
                y2 /= HSDRange;
                x2 /= HSDRange;
                y2 *= HSDZoom;
                x2 *= HSDZoom;
            }
        }
        else
        {
            y2 /= HSDRange;
            x2 /= HSDRange;
        }

        displayX = trig.cos * x2 - trig.sin * y2;
        displayY = trig.sin * x2 + trig.cos * y2;
        display->AdjustOriginInViewport(displayX, displayY);

        if (fabs(displayY) < 1 and fabs(displayX) < 1)   // JPO - don't bother if its off screen
        {
            DisplayMatrix savem;
            display->SaveDisplayMatrix(&savem);

            // JB 010730 Advanced and basic symbols are drawn sometimes on the HSD. This is an attempt to fix that.
            if (gp->symbol not_eq RWRSYM_ADVANCED_INTERCEPTOR and gp->symbol not_eq RWRSYM_BASIC_INTERCEPTOR)
                RwrClass::DrawSymbol(display, gp->symbol);  // This zeros display rotation, caused bad FLOT and bullseye drawing (now fixed I hope JPO)

            display->RestoreDisplayMatrix(&savem);
        }

        float rr = gp->range / HSDRange;

        if (HSDZoom > 0)
            rr *= HSDZoom;

        if (gp->IsSet(GroundListElement::RangeRing))
        {
            display->Circle(0.0f, 0.0f, rr);
        }

        display->AdjustOriginInViewport(-displayX, -displayY);
    }

#if 0

    // TODO:  Consolidate the HTS and HSD displays.
    // FORNOW:  Don't show preplanned emitters on the HSD
    /*
    HarmTargetingPod* theHarm = (HarmTargetingPod*)FindSensor (platform, SensorClass::HTS);
    HarmTargetingPod::ListElement* tmpElement;
    mlTrig trig;
    float x2, y2;
    float displayX, displayY;

       if (theHarm)
       {
          tmpElement = theHarm->EmmitterList();
          mlSinCos (&trig, platform->Yaw());

          // Draw all known emmitters
          while (tmpElement)
        {
       // TODO:  Consider filtering based on RWR settings here...

                y2 = (tmpElement->BaseObject()->XPos() - platform->XPos()) * FT_TO_NM / HSDRange;
                x2 = (tmpElement->BaseObject()->YPos() - platform->YPos()) * FT_TO_NM / HSDRange;
                displayX = trig.cos * x2 - trig.sin * y2;
                displayY = trig.sin * x2 + trig.cos * y2;
                display->AdjustOriginInViewport (displayX, displayY);
                theHarm->DrawSymbol (display, tmpElement->major, tmpElement->minor, tmpElement->flags);
                display->AdjustOriginInViewport (-displayX, -displayY);
                tmpElement = tmpElement->next;
             }
          }
       }
    */
#endif
}

void FireControlComputer::DrawGhostCursor(void)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(platform, SensorClass::Radar);

    //MI
    if (g_bRealisticAvionics)
    {
        if (g_bINS)
        {
            if (playerAC and not playerAC->INSState(AircraftClass::INS_HSD_STUFF))
                return;
        }

        // MD -- 20040113: supress ghost cursors with STT mode; we'll draw the cursor latched on to the bugged target instead
        if (theRadar and ((theRadar->GetRadarMode() == RadarClass::STBY) or theRadar->IsSet(RadarDopplerClass::STTingTarget)))
            return;
    }

    float cursorX, cursorY;
    static const float GCURS_LEN = 0.05f;
    static const float GCURS_OFF = 0.05f;


    if (theRadar)
    {
        theRadar->GetCursorPosition(&cursorY, &cursorX);
        cursorX /= HSDRange;
        cursorY /= HSDRange;

        //MI add in INS Drift
        /*if(g_bINS and g_bRealisticAvionics)
        {
          if(playerAC)
          {
           cursorX += playerAC->GetINSLatDrift();
           cursorY += playerAC->GetINSLongDrift();
          }
        }*/

        display->Line(cursorX - GCURS_OFF, cursorY - GCURS_LEN,
                      cursorX - GCURS_OFF, cursorY + GCURS_LEN);
        display->Line(cursorX + GCURS_OFF, cursorY - GCURS_LEN,
                      cursorX + GCURS_OFF, cursorY + GCURS_LEN);
    }
}

void FireControlComputer::DrawScanVolume(void)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    //MI
    if (g_bRealisticAvionics and g_bINS)
    {
        if (playerAC and not playerAC->INSState(AircraftClass::INS_HSD_STUFF))
            return;
    }

    RadarClass* theRadar = (RadarClass*)FindSensor(platform, SensorClass::Radar);

    if (theRadar and theRadar->IsOn())
    {
        float cone = theRadar->GetVolume();
        float scale = theRadar->GetRange() / HSDRange;
        mlTrig trig;
        mlSinCos(&trig, cone);
        display->AdjustRotationAboutOrigin(theRadar->SeekerAz());
        display->Line(0.0F,  0.0F,  trig.sin * scale, trig.cos * scale);
        display->Line(0.0F,  0.0F, -trig.sin * scale, trig.cos * scale);
        //display->AdjustRotationAboutOrigin(270*DTR);
        display->AdjustRotationAboutOrigin(272 * DTR); //Wombat778 11-13-2003 make the arc appear centered.  This is a hack to fix a bug in the arc code, but at least it is symmetrical
        display->Arc(0.0f, 0.0f, scale, 2 * PI - cone, 2 * PI);
        display->Arc(0.0f, 0.0f, scale, 0.0f, cone);
        // 2002-03-08 MN This was missing (was present in V1.071) and messed up FLOT and Bullseye on HSD
        display->ZeroRotationAboutOrigin();
    }
}

static const float SCH_ANG_INC = 11.5F;      /* velocity pointer angle increment  JPG - was 22.5   */
static const float SCH_FACT = 1600.0F;      /* velocity pointer length is the ratio */
/* of vt/SCH_FACT                       */
#define DD_LENGTH   0.2F        /* donkey dick length                   */
static const float trackScale = 0.05f;
static const float trackTriH = trackScale * (float)cos(DTR * 30.0f);
static const float trackTriV = trackScale * (float)sin(DTR * 30.0f);

void FireControlComputer::DrawBuggedTarget()
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    //MI
    if (g_bRealisticAvionics and g_bINS)
    {
        if (playerAC and not playerAC->INSState(AircraftClass::INS_HSD_STUFF))
            return;
    }

    float xPos, yPos, dir, speed;
    mlTrig trig;

    // MD -- 20040113: need RadarDopplerClass ptr to check flags for STT
    RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(platform, SensorClass::Radar);

    if (theRadar and theRadar->GetBuggedData(&xPos, &yPos, &dir, &speed))
    {
        //MI add in INS Drift
        if (g_bINS and g_bRealisticAvionics)
        {
            if (playerAC)
            {
                xPos += playerAC->GetINSLatDrift();
                yPos += playerAC->GetINSLongDrift();
            }
        }

        float x, y;

        //MI changed
        if (g_bRealisticAvionics)
        {
            if (HSDZoom == 0)
            {
                y = (xPos - platform->XPos()) * FT_TO_NM / HSDRange;
                x = (yPos - platform->YPos()) * FT_TO_NM / HSDRange;
            }
            else
            {
                y = (xPos - platform->XPos()) * FT_TO_NM / HSDRange * HSDZoom;
                x = (yPos - platform->YPos()) * FT_TO_NM / HSDRange * HSDZoom;
            }
        }
        else
        {
            y = (xPos - platform->XPos()) * FT_TO_NM / HSDRange;
            x = (yPos - platform->YPos()) * FT_TO_NM / HSDRange;
        }

        mlSinCos(&trig, platform->Yaw());
        float displayX = trig.cos * x - trig.sin * y;
        float displayY = trig.sin * x + trig.cos * y;

        // MD -- 20040113: Draw cursor at bugged target if the radar is in STT mode
        if (theRadar->IsSet(RadarDopplerClass::STTingTarget))
        {
            static const float GCURS_LEN = 0.05f;
            static const float GCURS_OFF = 0.05f;

            display->Line(displayX - GCURS_OFF, displayY - GCURS_LEN,
                          displayX - GCURS_OFF, displayY + GCURS_LEN);
            display->Line(displayX + GCURS_OFF, displayY - GCURS_LEN,
                          displayX + GCURS_OFF, displayY + GCURS_LEN);
        }

        display->AdjustOriginInViewport(displayX, displayY);

        float ang = dir - platform->Yaw();

        if (ang >= 0.0)
            ang = SCH_ANG_INC * (float)floor(ang / (SCH_ANG_INC * DTR));
        else
            ang = SCH_ANG_INC * (float)ceil(ang / (SCH_ANG_INC * DTR));

        display->AdjustRotationAboutOrigin(ang * DTR);

        display->Circle(0.0F, 0.0F, trackScale);
        display->Tri(0.0f, trackScale, trackTriH, -trackTriV, -trackTriH, -trackTriV);
        display->Line(0.0f, trackScale, 0.0f, trackScale + DD_LENGTH * speed / SCH_FACT);
        display->AdjustRotationAboutOrigin(-ang * DTR);
        display->AdjustOriginInViewport(-displayX, -displayY);
    }
}


void FireControlComputer::DrawWingmen()
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    //MI
    if (g_bRealisticAvionics and g_bINS)
    {
        if (playerAC and not playerAC->INSState(AircraftClass::INS_HSD_STUFF))
            return;
    }

    if (((AircraftClass*)platform)->mFaults->GetFault(FaultClass::dlnk_fault) or
 not ((AircraftClass*)platform)->HasPower(AircraftClass::DLPower))
        return;

    //Cobra
    for (int i = 0; i < 4/*platform->GetCampaignObject()->NumberOfComponents()*/; i++)
    {
        if (i == platform->vehicleInUnit) continue; // this is us

        AircraftClass *wingman = (AircraftClass *)platform->GetCampaignObject()->GetComponentNumber(i);

        if (wingman and not IsHsdState(HSDNOADLNK))
        {
            Draw1Wingman(wingman);//Cobra added HSDNOADLINK here
        }

        if (wingman and not IsHsdState(HSDNOGNDLNK))
        {
            Draw1WingmanGnd(wingman);//Cobra
        }
    }
}

//Cobra GDLINK
void FireControlComputer::Draw1WingmanGnd(AircraftClass *wing)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (g_bRealisticAvionics and g_bINS)
    {
        if (playerAC and not playerAC->INSState(AircraftClass::INS_HSD_STUFF))
            return;
    }

    //Cobra This is where the Digi draws his bugged Target
    //Will make it distinguish between A/A and A/G
    float xPos, yPos, speed;
    int alt = 0;
    float dir = 0.0f;
    float displayX = 0.0f;
    float displayY = 0.0f;
    float dsq = 0.0f;
    float wingBugX = 0.0f;
    float wingBugY = 0.0f;
    float x = 0.0f;
    float y = 0.0f;
    char no[10]/*, thealt[20]*/;
    mlTrig trig;

    SimObjectType *locked;
    RadarClass* theRadar = (RadarClass*)FindSensor(wing, SensorClass::Radar);
#if NO_REMOTE_BUGGED_TARGET

    if (theRadar == NULL or ((locked = theRadar->CurrentTarget()) == NULL))
    {
        return;
    }

#else

    if (((SimVehicleClass*)wing)->sensorArray[1]->RemoteBuggedTarget == NULL and 
        (theRadar == NULL or (locked = theRadar->CurrentTarget()) == NULL)) return;

#endif

    if (theRadar->GetRadarModeR() not_eq RadarClass::GM and theRadar->GetRadarModeR() not_eq RadarClass::GMT)
    {
        return;
    }

#if NO_REMOTE_BUGGED_TARGET
    FalconEntity *lockedData = locked->BaseData();
    xPos = lockedData->XPos();
    yPos = lockedData->YPos();
    alt  = FTOL(lockedData->ZPos() / 1000.0f);
    dir = lockedData->Yaw();
    speed = lockedData->GetVt();
#else

    if ( not ((SimVehicleClass*)wing)->sensorArray[1]->RemoteBuggedTarget)
    {
        xPos = locked->BaseData()->XPos();
        yPos = locked->BaseData()->YPos();
        alt  = (int)locked->BaseData()->ZPos() / 1000;
        dir = locked->BaseData()->Yaw();
        speed = locked->BaseData()->GetVt();
    }
    else
    {
        xPos = ((SimVehicleClass*)wing)->sensorArray[1]->RemoteBuggedTarget->XPos();
        yPos = ((SimVehicleClass*)wing)->sensorArray[1]->RemoteBuggedTarget->YPos();
        alt  = (int) - ((SimVehicleClass*)wing)->sensorArray[1]->RemoteBuggedTarget->ZPos() / 1000;
        dir = ((SimVehicleClass*)wing)->sensorArray[1]->RemoteBuggedTarget->Yaw();
        speed = 300.0f;
    }

#endif

    sprintf(no, "%d", wing->vehicleInUnit + 1);

    //MI adopt for zoom
    if (g_bRealisticAvionics)
    {
        //MI add in INS Drift
        if (g_bINS)
        {
            if (playerAC)
            {
                xPos += playerAC->GetINSLatDrift();
                yPos += playerAC->GetINSLongDrift();
            }
        }

        if (HSDZoom == 0)
        {
            y = (xPos - platform->XPos()) * FT_TO_NM / HSDRange;
            x = (yPos - platform->YPos()) * FT_TO_NM / HSDRange;
        }
        else
        {
            y = (xPos - platform->XPos()) * FT_TO_NM / HSDRange * HSDZoom;
            x = (yPos - platform->YPos()) * FT_TO_NM / HSDRange * HSDZoom;
        }
    }
    else
    {
        y = (xPos - platform->XPos()) * FT_TO_NM / HSDRange;
        x = (yPos - platform->YPos()) * FT_TO_NM / HSDRange;
    }

    mlSinCos(&trig, platform->Yaw());
    displayX = trig.cos * x - trig.sin * y;
    displayY = trig.sin * x + trig.cos * y;

    display->SetColor(GetMfdColor(MFD_UNKNOWN));
    dsq = displayX * displayX + displayY * displayY;

    if (dsq > 1.0f)   // off the display- so triange
    {
        float dist = (float)sqrt(dsq);
        displayX /= dist;
        displayY /= dist;
        display->AdjustOriginInViewport(displayX, displayY);
        dir = (float)atan2(displayY, displayX);
        display->AdjustRotationAboutOrigin(dir);
        display->Tri(-0.05f, -0.05f, 0.0f, 0.0f, 0.05f, -0.05f);
        display->AdjustRotationAboutOrigin(-dir);
    }
    else
    {
        display->AdjustOriginInViewport(displayX, displayY);

        float ang = dir - platform->Yaw();

        if (ang >= 0.0)
            ang = SCH_ANG_INC * (float)floor(ang / (SCH_ANG_INC * DTR));
        else
            ang = SCH_ANG_INC * (float)ceil(ang / (SCH_ANG_INC * DTR));

        display->AdjustRotationAboutOrigin(ang * DTR);
        display->Circle(0.0F, 0.0F, g_fRadarScale * 0.035f);
        display->AdjustRotationAboutOrigin(-ang * DTR);
    }

    display->TextCenterVertical(0.0f, 0.08f, no);
    display->AdjustOriginInViewport(-displayX, -displayY);

}//end function

void FireControlComputer::Draw1Wingman(AircraftClass *wing)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    //MI
    if (g_bRealisticAvionics and g_bINS)
    {
        if (playerAC and not playerAC->INSState(AircraftClass::INS_HSD_STUFF))
        {
            return;
        }
    }

    ShiAssert(wing not_eq NULL);
    char no[10], thealt[20];
    mlTrig trig;
    static const float wingBugX = 0.05f;
    static const float wingBugY = 0.05f;

    if (wing->mFaults->GetFault(FaultClass::dlnk_fault))
    {
        return;
    }

    float x = 0;
    float y = 0;
    float WingX = wing->XPos();
    float WingY = wing->YPos();

    //MI changed
    if (g_bRealisticAvionics)
    {
        //MI add in INS Drift
        if (g_bINS)
        {
            if (playerAC)
            {
                WingX += playerAC->GetINSLatDrift();
                WingY += playerAC->GetINSLongDrift();
            }
        }

        if (HSDZoom == 0)
        {
            y = (WingX - platform->XPos()) * FT_TO_NM / HSDRange;
            x = (WingY - platform->YPos()) * FT_TO_NM / HSDRange;
        }
        else
        {
            y = (WingX - platform->XPos()) * FT_TO_NM / HSDRange * HSDZoom;
            x = (WingY - platform->YPos()) * FT_TO_NM / HSDRange * HSDZoom;
        }
    }
    else
    {
        y = (WingX - platform->XPos()) * FT_TO_NM / HSDRange;
        x = (WingY - platform->YPos()) * FT_TO_NM / HSDRange;
    }

    float dir = wing->Yaw() - platform->Yaw();
    display->SetColor(GetMfdColor(MFD_DL_TEAM14));
    mlSinCos(&trig, platform->Yaw());
    float displayX = trig.cos * x - trig.sin * y;
    float displayY = trig.sin * x + trig.cos * y;
    float dsq = displayX * displayX + displayY * displayY;

    if (dsq > 1.0f)   // off the display - so a pointer
    {
        float dist = (float)sqrt(dsq);
        displayX /= dist;
        displayY /= dist;
        display->AdjustOriginInViewport(displayX, displayY);
        dir = (float)atan2(displayY, displayX);
        display->AdjustRotationAboutOrigin(dir);
        display->Tri(-0.05f, -0.05f, 0.0f, 0.0f, 0.05f, -0.05f);
    }
    else   // real thing
    {
        display->AdjustOriginInViewport(displayX, displayY);
        display->AdjustRotationAboutOrigin(dir);
        display->Arc(0.00f, -0.1f, 0.05f, 180.0f * DTR, 359.0f * DTR); // arc bit
        display->Line(0.0f, -0.07f, 0.0f, 0.0f);  // line bit
    }

    display->AdjustRotationAboutOrigin(-dir);
    int alt = FTOL(-wing->ZPos());
    alt = (alt + 500) / 1000;
    sprintf(thealt, "%d", alt);
    display->TextCenterVertical(0.0f, -0.12f, thealt);
    sprintf(no, "%d", wing->vehicleInUnit + 1);
    display->TextCenterVertical(0.0f, 0.08f, no);
    display->AdjustOriginInViewport(-displayX, -displayY);

    // Cobra This is where the Digi draws his bugged Target
    // Will make it distinguish between A/A and A/G

    float xPos, yPos, speed;
    RadarClass* theRadar = (RadarClass*)FindSensor(wing, SensorClass::Radar);
    SimObjectType *locked = (theRadar == NULL) ? NULL :  theRadar->CurrentTarget();

    if (
#if not NO_REMOTE_BUGGED_TARGET
        static_cast<SimVehicleClass*>(wing)->sensorArray[1]->RemoteBuggedTarget == NULL and 
#endif
        (theRadar == NULL or locked == NULL)
    )
    {
        return;
    }

#if NO_REMOTE_BUGGED_TARGET

    if (locked->localData->ata > 60.0f * DTR)
    {
        return;
    }

#else
    //Cobra Is this within the ATA??
    SimObjectType *ta = wing->targetPtr;
    SimObjectLocalData* localData = NULL;

    if (locked)
    {
        //no CTD on null pointers :)
        localData = ta->localData;

        if (localData->ata > 60 * DTR)
        {
            return;
        }
    }
    else
    {
        return;
    }

#endif

#if NO_REMOTE_BUGGED_TARGET
    FalconEntity *lockedData = locked->BaseData();

    if (lockedData)
    {
        xPos = lockedData->XPos();
        yPos = lockedData->YPos();
        alt  = FTOL(lockedData->ZPos() / 1000.0f);
        dir = lockedData->Yaw();
        speed = lockedData->GetVt();
    }

#else

    if ( not ((SimVehicleClass*)wing)->sensorArray[1]->RemoteBuggedTarget)
    {
        xPos = locked->BaseData()->XPos();
        yPos = locked->BaseData()->YPos();
        alt  = (int)locked->BaseData()->ZPos() / 1000;
        dir = locked->BaseData()->Yaw();
        speed = locked->BaseData()->GetVt();
    }
    else
    {
        xPos = ((SimVehicleClass*)wing)->sensorArray[1]->RemoteBuggedTarget->XPos();
        yPos = ((SimVehicleClass*)wing)->sensorArray[1]->RemoteBuggedTarget->YPos();
        alt  = (int) - ((SimVehicleClass*)wing)->sensorArray[1]->RemoteBuggedTarget->ZPos() / 1000;
        dir = ((SimVehicleClass*)wing)->sensorArray[1]->RemoteBuggedTarget->Yaw();
        speed = 300.0f;//me123 it ctd's ((SimVehicleClass*)wing)->sensorArray[1]->RemoteBuggedTarget->GetVt();
    }

#endif

    //Cobra we don't want to draw A/G stuff and certainly not with A/A symbology
    if ((theRadar->GetRadarModeR() not_eq RadarClass::AA) or (abs(alt) <= 1))
    {
        return;
    }

    //MI adopt for zoom
    if (g_bRealisticAvionics)
    {
        //MI add in INS Drift
        if (g_bINS)
        {
            if (playerAC)
            {
                xPos += playerAC->GetINSLatDrift();
                yPos += playerAC->GetINSLongDrift();
            }
        }

        if (HSDZoom == 0)
        {
            y = (xPos - platform->XPos()) * FT_TO_NM / HSDRange;
            x = (yPos - platform->YPos()) * FT_TO_NM / HSDRange;
        }
        else
        {
            y = (xPos - platform->XPos()) * FT_TO_NM / HSDRange * HSDZoom;
            x = (yPos - platform->YPos()) * FT_TO_NM / HSDRange * HSDZoom;
        }
    }
    else
    {
        y = (xPos - platform->XPos()) * FT_TO_NM / HSDRange;
        x = (yPos - platform->YPos()) * FT_TO_NM / HSDRange;
    }

    displayX = trig.cos * x - trig.sin * y;
    displayY = trig.sin * x + trig.cos * y;

    display->SetColor(GetMfdColor(MFD_UNKNOWN));
    dsq = displayX * displayX + displayY * displayY;

    if (dsq > 1.0f)   // off the display- so triange
    {
        float dist = (float)sqrt(dsq);
        displayX /= dist;
        displayY /= dist;
        display->AdjustOriginInViewport(displayX, displayY);
        dir = (float)atan2(displayY, displayX);
        display->AdjustRotationAboutOrigin(dir);
        display->Tri(-0.05f, -0.05f, 0.0f, 0.0f, 0.05f, -0.05f);
        display->AdjustRotationAboutOrigin(-dir);
    }
    else
    {
        display->AdjustOriginInViewport(displayX, displayY);

        float ang = dir - platform->Yaw();

        if (ang >= 0.0)
        {
            ang = SCH_ANG_INC * (float)floor(ang / (SCH_ANG_INC * DTR));
        }
        else
        {
            ang = SCH_ANG_INC * (float)ceil(ang / (SCH_ANG_INC * DTR));
        }

        display->AdjustRotationAboutOrigin(ang * DTR);
        display->Line(-wingBugX, 0.00f, -wingBugX, wingBugY);
        display->Line(-wingBugX, wingBugY,  wingBugX, wingBugY);
        display->Line(wingBugX, wingBugY,  wingBugX, 0.00f);
        display->Line(/*-wingBugX,*/0.00f, 0.10f, /*-wingBugY,*/ 0.00f, wingBugY /*DD_LENGTH*speed/SCH_FACT */);
        display->AdjustRotationAboutOrigin(-ang * DTR);
    }

    sprintf(thealt, "%d", abs(alt));
    display->TextCenterVertical(0.0f, -0.12f, thealt);
    display->TextCenterVertical(0.0f, 0.08f, no);
    display->AdjustOriginInViewport(-displayX, -displayY);
}

//MI
void FireControlComputer::ToggleHSDZoom(void)
{
    //Wombat778 11-10-2003 Changed so HSD will center on cursor when zoomed.
    if (HSDZoom == 0)
    {
        //Take the current cursor position and save it to the HSDXPos and HSDYPos variables
        HSDXPos = xPos * 2;
        HSDYPos = yPos * 2;

        if (IsSOI)
        {
            //if the MFD is the SOI, center the cursor in the display when zooming
            xPos = 0;
            yPos = 0;
        }

        HSDZoom = 2;
    }
    else if (HSDZoom == 2)
    {
        // Take the current cursor position and save it to the HSDXPos
        // and HSDYPos variables.  Adjust for the fact that we arent at the original zoom anymore
        HSDXPos = HSDZoom * (HSDXPos + (xPos));
        HSDYPos = HSDZoom * (HSDYPos + (yPos));

        if (IsSOI)
        {
            //if the MFD is the SOI, center the cursor in the display when zooming
            xPos = 0;
            yPos = 0;
        }

        HSDZoom = 4;
    }
    else
    {
        HSDZoom = 0;
    }
}

const static float beginline = 0.02f;
const static float endline = 0.05f;

void FireControlComputer::HSDDisplay(void)
{
    //add cursor symbol
    display->SetColor(GetMfdColor(MFD_CURSOR));
    display->Line(xPos + beginline, yPos, xPos + endline, yPos);
    display->Line(xPos - beginline, yPos, xPos - endline, yPos);
    display->Line(xPos, yPos + beginline, xPos, yPos + endline);
    display->Line(xPos, yPos - beginline, xPos, yPos - endline);

    //check for movement
    MoveCursor();
}
//MI
void FireControlComputer::MoveCursor(void)
{
    if (HSDCursorXCmd not_eq 0.0F or HSDCursorYCmd not_eq 0.0F)
    {
        if ((IO.AnalogIsUsed(AXIS_CURSOR_X) == true) and (IO.AnalogIsUsed(AXIS_CURSOR_Y) == true))
        {
            xPos += (HSDCursorXCmd / 10000.0F) * g_fCursorSpeed * (6.5F * CursorRate) * SimLibMajorFrameTime;
            yPos += (HSDCursorYCmd / 10000.0F) * g_fCursorSpeed * (6.5F * CursorRate) * SimLibMajorFrameTime;
        }
        else
        {
            xPos += HSDCursorXCmd * g_fCursorSpeed * curCursorRate * SimLibMajorFrameTime;
            yPos += HSDCursorYCmd * g_fCursorSpeed * curCursorRate * SimLibMajorFrameTime;

            curCursorRate = min(curCursorRate + CursorRate * SimLibMajorFrameTime * (4.0F), (6.5F) * CursorRate);
        }

        xPos = min(max(xPos, -1.0F), 1.0F);
        yPos = min(max(yPos, -1.0F), 1.0F);

        //range bump
        if (HSDZoom == 0)
        {
            //if we bump it' we decouple automatically
            if (yPos > 0.9F and HsdRangeTbl[HsdRangeIndex] < 240)
            {
                HSDRangeStepCmd = 1;
                yPos = 0.0F;

                if (IsHsdState(HSDCPL))
                    ToggleHsdState(HSDCPL);
            }
            else if (yPos < -0.9F and HsdRangeTbl[HsdRangeIndex] > 15)
            {
                HSDRangeStepCmd = -1;
                yPos = 0.0F;

                if (IsHsdState(HSDCPL))
                    ToggleHsdState(HSDCPL);
            }
        }
    }
    else
        curCursorRate = CursorRate;

    //check if our cursor is over a waypoint, only if their not decluttered
    if ( not IsHsdState(HSDNONAV1))
    {
        WayPointClass *tmpWp = platform->waypoint;

        if (tmpWp)
        {
            //do this stuff for all waypoints
            tmpWp = tmpWp->GetNextWP();

            while (tmpWp)
            {
                //cursor position
                MapWaypointToXY(tmpWp);

                if ( not IsHsdState(HSDCEN))
                    DispY -= 0.4F;

                float tolerance = 0.05F; //just about the size of the circle
                float CursWPRange = (float)sqrt((DispX - xPos) * (DispX - xPos) + (DispY - yPos) * (DispY - yPos));

                //CursWPRange;
                if (CursWPRange < tolerance and CursWPRange > -tolerance)
                {
                    //did we designate?
                    if (HSDDesignate == 1)
                    {
                        //need to find which waypoint our cursor is over now
                        if (platform->curWaypoint not_eq tmpWp)
                        {
                            ChangeSTPT(tmpWp);
                        }

                        HSDDesignate = 0;
                    }
                }

                tmpWp = tmpWp->GetNextWP();
            }
        }
    }

    //check for preplanned threads, only if not decluttered
    if ( not IsHsdState(HSDNOPRE))
        CheckPP();
}
//MI
void FireControlComputer::ChangeSTPT(WayPointClass *tmpWp)
{
    while (platform->curWaypoint and platform->curWaypoint not_eq tmpWp)
    {
        waypointStepCmd = 1;
        StepPoint();
    }
}
//MI
void FireControlComputer::MapWaypointToXY(WayPointClass *tmpWp)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    float wpX, wpY, wpZ;
    tmpWp->GetLocation(&wpX, &wpY, &wpZ);

    //add in INS Drift
    if (g_bINS)
    {
        if (playerAC)
        {
            wpX += playerAC->GetINSLatDrift();
            wpY += playerAC->GetINSLongDrift();
        }
    }

    mlTrig trig;
    mlSinCos(&trig, platform->Yaw());
    float y2 = (wpX - platform->XPos()) * FT_TO_NM;
    float x2 = (wpY - platform->YPos()) * FT_TO_NM;

    if (HSDZoom == 0)
    {
        y2 /= HSDRange;
        x2 /= HSDRange;
    }
    else
    {
        y2 /= HSDRange;
        x2 /= HSDRange;
        y2 *= HSDZoom;
        x2 *= HSDZoom;
    }

    DispX = trig.cos * x2 - trig.sin * y2;
    DispY = trig.sin * x2 + trig.cos * y2;
}
//MI
void FireControlComputer::CheckPP(void)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    GroundListElement *gp;
    mlTrig trig;
    mlSinCos(&trig, platform->Yaw());
    float myX, myY, x2, y2, displayX, displayY;
    myX = platform->XPos();
    myY = platform->YPos();
    UpdatePlanned();

    // Draw all known emmitters
    for (gp = GetFirstGroundElement(); gp; gp = gp->GetNext())
    {
        gp->HandoffBaseObject();

        if (gp->BaseObject() == NULL)
            continue; // probably dead.

        y2 = (gp->BaseObject()->XPos() - myX) * FT_TO_NM;
        x2 = (gp->BaseObject()->YPos() - myY) * FT_TO_NM;

        //MI add in INS Drift
        if (g_bINS)
        {
            if (playerAC)
            {
                y2 += (playerAC->GetINSLatDrift() * FT_TO_NM);
                x2 += (playerAC->GetINSLongDrift() * FT_TO_NM);
            }
        }

        if (HSDZoom == 0)
        {
            y2 /= HSDRange;
            x2 /= HSDRange;
        }
        else
        {
            y2 /= HSDRange;
            x2 /= HSDRange;
            y2 *= HSDZoom;
            x2 *= HSDZoom;
        }

        displayX = trig.cos * x2 - trig.sin * y2;
        displayY = trig.sin * x2 + trig.cos * y2;

        if ( not IsHsdState(HSDCEN))
            displayY -= 0.4F;

        float CursGPRange = (float)sqrt((displayX - xPos) * (displayX - xPos) +
                                        (displayY - yPos) * (displayY - yPos));
        float tolerance = 0.03F;

        if (CursGPRange < tolerance and CursGPRange > -tolerance)
        {
            if (HSDDesignate < 0)
            {
                gp->ClearFlag(GroundListElement::RangeRing);
                HSDDesignate = 0;
            }
            else if (HSDDesignate == 1)
            {
                gp->SetFlag(GroundListElement::RangeRing);
                HSDDesignate = 0;
            }
        }
    }
}
void FireControlComputer::DrawAIFF(void)//Cobra 11/27/04
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (g_bRealisticAvionics and g_bINS)
    {
        if (playerAC and not playerAC->INSState(AircraftClass::INS_HSD_STUFF))
            return;
    }

    //float xPos, yPos, dir, speed;
    mlTrig trig;

    SimObjectLocalData* rdrData;
    SimObjectType* rdrObj = platform->targetList;

    while (rdrObj)
    {
        rdrData = rdrObj->localData;

        if (rdrData->rdrY[0] > 1.0)
        {
            float x = rdrObj->BaseData()->XPos();
            float y = rdrObj->BaseData()->YPos();

            if (g_bINS and g_bRealisticAvionics)
            {
                if (playerAC)
                {
                    x += playerAC->GetINSLatDrift();
                    y += playerAC->GetINSLongDrift();
                }
            }

            float sx;
            float sy;

            if (HSDZoom == 0)
            {
                sy = (x - platform->XPos()) * FT_TO_NM / HSDRange;
                sx = (y - platform->YPos()) * FT_TO_NM / HSDRange;
            }
            else
            {
                sy = (x - platform->XPos()) * FT_TO_NM / HSDRange * HSDZoom;
                sx = (y - platform->YPos()) * FT_TO_NM / HSDRange * HSDZoom;
            }

            mlSinCos(&trig, platform->Yaw());
            float displayX = trig.cos * sx - trig.sin * sy;
            float displayY = trig.sin * sx + trig.cos * sy;

            float dsq = displayX * displayX + displayY * displayY;



            if (dsq < 1.0f and rdrData->ata <= 60.0f * DTR)//Cobra ATA constraints 60 degrees
            {
                if (rdrData->interrogated and TeamInfo[platform->GetTeam()]->TStance(rdrObj->BaseData()->GetTeam()) == Allied)
                {
                    display->AdjustOriginInViewport(displayX, displayY);
                    display->Circle(0, 0, 0.03f);
                    display->AdjustOriginInViewport(-displayX, -displayY);
                }
            }



        }

        rdrObj = rdrObj->next;
    }

}//Close function
