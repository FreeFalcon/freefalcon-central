#include "stdhdr.h"
#include "Graphics/Include/TOD.h"
#include "Graphics/Include/renderow.h"
#include "Graphics/Include/RViewPnt.h"
#include "Graphics/Include/canvas3d.h"
#include "Graphics/Include/Drawbsp.h"
#include "Graphics/Include/Drawgrnd.h"
#include "Graphics/Include/draw2d.h"
#include "Graphics/Include/TimeMgr.h"

#include "Graphics/DXEngine/DXEngine.h"
#include "Graphics/DXEngine/DXVBManager.h"
extern bool g_bUse_DX_Engine;

#include "TimerThread.h"
#include "hud.h"
#include "object.h"
#include "mfd.h"
#include "playerrwr.h"
#include "fsound.h"
#include "soundFX.h"
#include "cpmanager.h"
//MI extracting Data
#include "cphsi.h"
#include "ThreadMgr.h"
#include "aircrft.h"
#include "weather.h"
#include "simeject.h"
#include "resource.h"
#include "cpvbounds.h"
#include "playerop.h"
#include "dispopts.h"
#include "sfx.h"
#include "acmi/src/include/acmirec.h"
#include "camp2sim.h"
#include "SimLoop.h"
#include "simdrive.h"
#include "sinput.h"
#include "commands.h"
#include "dogfight.h"
#include "inpfunc.h"
#include "otwdrive.h"
#include "flightData.h"
#include "airframe.h"
#include "fack.h"
#include "campwp.h"
#include "sms.h"
#include "hardpnt.h"
#include "ui/include/uicomms.h"
#include "popmenu.h"
#include "digi.h"
#include "dofsnswitches.h"
#include "navsystem.h"
#include "falclib/include/fakerand.h"
#include "PilotInputs.h"
#include "IvibeData.h"

// OW needed for restoring textures after task switch
#include "graphics/include/texbank.h"
#include "graphics/include/fartex.h"
#include "graphics/include/terrtex.h"



#include "radiosubtitle.h" // Retro 20Dec2003
#include "missile.h" // Retro 20Dec2003 for the infobar
#include "profiler.h" // Retro 20Dec2003

#ifdef DEBUG
//#define SHOW_FRAME_RATE 1
//#define SHOW_FRAME_RATE 0
extern int gTrailNodeCount, gVoiceCount;
#endif

int tactical_is_training(void);

// normalized coordinates where we start drawing messages TO us
#define MESSAGE_X (-0.99f)
#define MESSAGE_Y (0.3f)

// screen position of Pause/X2/X4 text
// Screen coordinates... woohoo
// X is centered...
#define COMPRESS_Y (5.0f)
#define COMPRESS_SPACING (15.0f)

// Chat Box Size stuff
#define CHAT_BOX_HALF_WIDTH (200.0f)
#define CHAT_BOX_HALF_HEIGHT (10.0f)
#define CHAT_STR_X (190.0f)
#define CHAT_STR_Y (2.0f)

#define SCORENAME_X  (0.6f)
#define SCOREPOINT_X (0.9f)
#define SCORE_Y      (0.8f)

extern int weatherCondition; //JAM 16Nov03
extern long mHelmetIsUR; // hack for UR Helmet detected
extern bool g_bLookCloserFix;
extern bool g_bLensFlare; //THW 2003-11-10 Toggle Lens Flare

extern bool g_bCockpitAutoScale; //Wombat778 12-12-2003
extern bool g_bRatioHack; //Wombat778 12-12-2003
extern bool g_bACMIRecordMsgOff; // JPG 10 Jan 04

extern float g_fMaximumFOV; // Wombat778 1-15-03
extern float g_fMinimumFOV; // Wombat778 1-15-03

extern int g_nNewFPSCounter; //Wombat778 3-24-03

extern bool g_bNew2DTrackIR; //Wombat778 11-15-04

//ATARIBABY HSI To/From flags - nedded for VCOCK.CPP
int HSITOFROM3d;

enum
{
    FLY_BY_CAMERA = 0,
    CHASE_CAMERA,
    ORBIT_CAMERA,
    SATELLITE_CAMERA,
    WEAPON_CAMERA,
    TARGET_TO_WEAPON_CAMERA,
    ENEMY_AIRCRAFT_CAMERA,
    FRIENDLY_AIRCRAFT_CAMERA,
    ENEMY_GROUND_UNIT_CAMERA,
    FRIENDLY_GROUND_UNIT_CAMERA,
    INCOMING_MISSILE_CAMERA,
    TARGET_CAMERA,
    TARGET_TO_SELF_CAMERA,
    ACTION_CAMERA,
    RECORDING,
    NUM_CAMERA_VIEWS,
};

// Display strings
extern char CompressionStr[5][20];
extern char CameraLabel[16][40];
extern int lTestFlag1;

// Score strings for Dogfight/Tactical Engagement
extern long gRefreshScoresList;
extern long gScoreColor[10];
extern _TCHAR gScoreName[10][30];
extern _TCHAR gScorePoints[10][10];
extern void MakeTacEngScoreList(); // And their functions
extern void MakeDogfightTopTen(int mode);

// 2000-11-24 ADDED BY S.G. FOR PADLOCKING OPTIONS
#define PLockModeNormal 0
#define PLockModeNearLabelColor 1
#define PLockModeNoSnap 2
#define PLockModeBreakLock 4
extern int g_nPadlockMode;
// END OF ADDED SECTION

//MI
extern bool g_bNoMFDsIn1View;
extern bool g_bShowFlaps;

void CallInputFunction(unsigned long val, int state);

extern int ShowFrameRate;
extern SimBaseClass* eyeFlyTgt;
extern int gTotSfx;
extern int numObjsProcessed;
extern int numObjsInDrawList;

extern HWND mainMenuWnd;
extern void* gSharedMemPtr;
extern void *gSharedIntellivibe;

static char tmpStr[128];

// From OTWDrive.cpp
extern unsigned long nextCampObjectHeightRefresh;

// from SimInput
extern unsigned int chatterCount;
extern char chatterStr[256];


static int gSimTimeMax = 0;
static int gCampTimeMax = 0;
static int gGraphicsTimeLastMax = 0;
extern int gSimTime;
extern int gGraphicsTimeLast;
extern int gCampTime;
extern int gAveCampTime;
extern int gAveSimGraphicsTime;
void DebugMemoryReport(RenderOTW *renderer, int frameTime);

extern FalconEntity *gOtwCameraLocation;

char gAcmiStr[11];

LRESULT CALLBACK SimWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

#ifdef USE_SH_POOLS
extern MEM_POOL gFartexMemPool;
#endif

/* Retro TrackIR stuff.. */
#include "TrackIR.h" // Retro 26/09/03
extern bool g_bEnableTrackIR; // Retro 26/09/03
extern TrackIR theTrackIRObject; // Retro 27/09/03
extern int g_nTrackIRSampleFreq; // Retro 02/10/03
/* ..ends */


//**************************************************************************************************
// COBRA - RED - Updated by the OTW Observer Position and Rotation
// These values are Public to be use all over the sources
Tpoint ObserverPosition;
float ObserverYaw, ObserverPitch, ObserverRoll;
// COBRA - RED - End
//**************************************************************************************************

// sfr: if not profiling, we redefine to empty
//#define PROFILE_RENDER 1
//#if not PROFILE_RENDER
//#undef START_PROFILE
//#undef STOP_PROFILE
//#define START_PROFILE(x)
//#define STOP_PROFILE(x)
//#endif


void OTWDriverClass::Cycle(void)
{
    //START_PROFILE("OTWCYCLE");

    frameStart = timeGetTime();
    frameTime = (frameTime * 7 + (frameStart - lastFrame)) / 8;
    lastFrame = frameStart;

    // OW
    if (frameTime < 10)
    {
        frameTime = 5;
    }

    // This is a kinda annoying thing to do here, but this seems like the safest place for the short term...
    if (SimLibElapsedTime > nextCampObjectHeightRefresh)
    {

        FalconEntity* campUnit;
        VuListIterator vehicleWalker(SimDriver.combinedList);

        // Note:  We could do buildings too, but they generally deaggregate far enough away that it isn't a problem
        // Consider each element of the campaign object sim bubble
        for (
            campUnit = (FalconEntity*)vehicleWalker.GetFirst();
            campUnit;
            campUnit = (FalconEntity*)vehicleWalker.GetNext()
        )
        {

            // Only deal with battalions for now
            if (campUnit->IsBattalion())
            {

                // Lets go with an approximation since the x,y position is an approximation anyway
                float groundZ = GetApproxGroundLevel(campUnit->XPos(), campUnit->YPos());

                // This is kinda annoying -- all we want to do is hammer the Z value, but this is going
                // to do a bunch of grid tree maintenance (and other?) junk.
                campUnit->SetPosition(campUnit->XPos(), campUnit->YPos(), groundZ);

            }

        }

        // FRB - Sure would like toput those buildings on the ground.
        FalconEntity* theObject;
        VuListIterator featureWalker(SimDriver.featureList);
        theObject = (FalconEntity*)featureWalker.GetFirst();

        while (theObject)
        {
            // Lets go with an approximation since the x,y position is an approximation anyway
            float groundZ = GetApproxGroundLevel(theObject->XPos(), theObject->YPos());
            theObject->SetPosition(theObject->XPos(), theObject->YPos(), groundZ);

            theObject = (FalconEntity*)featureWalker.GetNext();
        }

        // Set the time for the next refresh
        nextCampObjectHeightRefresh = SimLibElapsedTime + 5000; // Do it every 5 seconds
    }


    // do we run the acton camera?
    if (actionCameraMode and actionCameraTimer <= vuxRealTime)
    {
        RunActionCamera();
    }

    //Wombat778 11-18-04 Run the Automatic hybrid mode (mode 1). Simouse handles non-trackir hybrid mode.
    if (
        GetHybridPitMode() == 1 and 
        (GetOTWDisplayMode() == Mode2DCockpit or GetOTWDisplayMode() == Mode3DCockpit)
    )
    {
        RunHybridPitMode(cockpitFlightData.headYaw, cockpitFlightData.headPitch);
    }

    // when end flight timer is set, we're ending the game when vuxGameTime
    // gets larger than it
    if (endFlightTimer not_eq 0)
    {
        // make sure stuff is set correctly here (ie no hud, etc...)
        SetOTWDisplayMode(ModeChase);

        if (SimDriver.GetPlayerAircraft() and not SimDriver.GetPlayerAircraft()->IsEject())
        {
            // make sure autopilot is on
            AircraftClass *playerAircraft = (AircraftClass *)SimDriver.GetPlayerAircraft();

            if (playerAircraft->OnGround())
            {
                if (playerAircraft->DBrain()->ATCStatus() < tReqTaxi)
                {
                    if (playerAircraft->AutopilotType() not_eq AircraftClass::APOff)
                    {
                        playerAircraft->SetAutopilot(AircraftClass::APOff);
                    }
                }
                else if (playerAircraft->AutopilotType() not_eq AircraftClass::CombatAP)
                {
                    playerAircraft->SetAutopilot(AircraftClass::CombatAP);
                }
            }
            else if (playerAircraft->AutopilotType() not_eq AircraftClass::ThreeAxisAP)
            {
                playerAircraft->SetAutopilot(AircraftClass::ThreeAxisAP);
            }
        }

        if (vuxRealTime >= endFlightTimer)
        {
            // We're done, so tell the TheSimLoop to shut us down after this cycle.
            SimulationLoopControl::StopGraphics();
        }
    }

    // handle the flyby camera timer
    if (flybyTimer not_eq 0)
    {
        // make sure stuff is set correctly here (ie no hud, etc...)
        mOTWDisplayMode = ModeChase;

        if (SimLibElapsedTime >= flybyTimer)
        {
            flybyTimer = 0;
        }
    }

    if (exitMenuTimer)
    {
        if (vuxRealTime > exitMenuTimer)
        {
            SetExitMenu(TRUE);
        }
    }

    if (textTimeLeft[0] and textTimeLeft[0] < vuxRealTime)
    {
        ScrollMessages();
    }

    // Check for weather change
    if ( not weatherCmd)
    {
    }
    else
    {
        float weatherRange;

        doWeather = 1 - doWeather;

        if (doWeather)
        {
            weatherRange = 32.0F * FEET_PER_KM;
        }
        else
        {
            weatherRange = 0.0F;
        }

        weatherCmd = 0;
    }

    DXContext *pCtx = OTWImage->GetDisplayDevice()->GetDefaultRC();
    HRESULT hr = pCtx->TestCooperativeLevel();

    if (FAILED(hr))
    {
        return;
    }

    if (hr == S_FALSE)
    {
        OTWImage->RestoreAll();
        TheTextureBank.RestoreAll();
        TheTerrTextures.RestoreAll();
        TheFarTextures.RestoreAll();
    }

    //STOP_PROFILE("OTWCYCLE");
    RenderFrame();

    // TODO:  It would be _REALLY_ nice to get some work done in here before blocking on the HW...
    // COBRA - DX - Yep...just done... flip pages just before starting a new frame
    //OTWImage->SwapBuffers(false);
}


void OTWDriverClass::Reset3DParameters(void)
{
    // Avoid any remaining from TV colors bitand Settings
    TheDXEngine.ResetState();
    TheDXEngine.ClearLights();
    renderer->context.SetNVGmode(FALSE);
    renderer->SetGreenMode(false); //sfr
}


void OTWDriverClass::RenderFirstFrame(void)
{
    // WARNING  WARNING  WARNING  WARNING  WARNING 
    // RED - INIT time stuff here, then call all timed callbacks to have an update
    // situation on 3D entry
    // Start Up with appropriate Time
    TheTimeManager.SetTime(vuxGameTime + (unsigned long)FloatToInt32(todOffset * 1000.0F));
    // update callbacks
    TheTimeManager.Refresh();
    // Refresh weather stuff
    realWeather->RefreshWeather(renderer);
    //if its dark, turn internal lights on
    float light = TheTimeOfDay.GetLightLevel();

    if (light < 0.5F)
    {
        SimInstrumentLight(0, KEY_DOWN, NULL);
        SimInteriorLight(0, KEY_DOWN, NULL);
    }

    // reset any remaining from previous situations
    Reset3DParameters();

    // Update pit data bitand light situation
    pCockpitManager->LoadCockpitDefaults();
    pCockpitManager->SetTOD(light);
    // Pre laod all objects
    renderer->PreLoadScene(NULL, NULL);
    // Skip the entry swap, we would have nothing in back buffer
    SkipSwap = true;
    // The Big Load is not incoming
    BigLoadTimeOut = 0;

}


void OTWDriverClass::DisplayChatBox(void)
{
    char tempChar;
    float strw;

    // Center Box on screen (Will ALWAYS be same size (in pixels w,h) regardless of screen resolution)
    // BASICALLY, put centerX,centerY at the centerpoint of where you want the box to draw
    float halfHeight = OTWDriver.renderer->TextHeight() * 0.75F;
    float halfWidth = 0.75F;

    OTWDriver.renderer->SetColor(0x997B5200);   // 60% alpha blue
    OTWDriver.renderer->context.RestoreState(STATE_ALPHA_SOLID);

    OTWDriver.renderer->Tri(-halfWidth, -halfHeight,
                            halfWidth, -halfHeight,
                            halfWidth,  halfHeight);
    OTWDriver.renderer->Tri(-halfWidth, -halfHeight,
                            -halfWidth,  halfHeight,
                            halfWidth,  halfHeight);

    // Outline Translucent BLUE box
    OTWDriver.renderer->SetColor(0xFF000000);   // black
    OTWDriver.renderer->Line(-halfWidth, -halfHeight, halfWidth, -halfHeight);
    OTWDriver.renderer->Line(-halfWidth, -halfHeight, -halfWidth,  halfHeight);
    OTWDriver.renderer->Line(halfWidth,  halfHeight, halfWidth, -halfHeight);
    OTWDriver.renderer->Line(halfWidth,  halfHeight, -halfWidth,  halfHeight);

    if (vuxRealTime bitand 0x100)
    {
        tempChar = chatterStr[chatterCount];

        if (chatterStr[0])
        {
            chatterStr[chatterCount] = 0;
            strw = (float)OTWDriver.renderer->TextWidth(chatterStr);
            chatterStr[chatterCount] = tempChar;
        }
        else
            strw = 0.0f;

        OTWDriver.renderer->SetColor(0xfffefefe); // Not perfect white so color won't change
        OTWDriver.renderer->Line(strw - (halfWidth * 0.95F), -halfHeight * 0.75F, strw - (halfWidth * 0.95F), halfHeight * 0.75F);
    }

    if (chatterStr[0])
    {
        OTWDriver.renderer->SetColor(0xff000000); // Not perfect white so color won't change
        OTWDriver.renderer->TextLeftVertical(-halfWidth * 0.95F, 0.0F, chatterStr);
    }
}

void OTWDriverClass::ToggleInfoBar() // Retro 20Dec2003
{
    drawInfoBar = not drawInfoBar;

    bool result = (drawInfoBar == TRUE) ? true : false;
    PlayerOptions.SetInfoBar(result);
}

//void OTWDriverClass::DisplayAxisValues()
//{
//}

// Retro 29Feb2004 - this really only needs to be done once, so I pulled it into global scope for now. Might make it a static member var of OTWDriverClass though
struct Mode2Cam
{
    OTWDriverClass::OTWDisplayMode theMode;
    short theCamID;
};

const static Mode2Cam theModeTable[] =
{
    {OTWDriverClass::ModeChase, FLY_BY_CAMERA},
    // {OTWDriverClass::ModeChase, CHASE_CAMERA}, // theCamID doesn´t matter here..
    {OTWDriverClass::ModeOrbit, ORBIT_CAMERA},
    {OTWDriverClass::ModeSatellite, SATELLITE_CAMERA},
    {OTWDriverClass::ModeWeapon, WEAPON_CAMERA},
    {OTWDriverClass::ModeTargetToWeapon, TARGET_TO_WEAPON_CAMERA},
    {OTWDriverClass::ModeAirEnemy, ENEMY_AIRCRAFT_CAMERA},
    {OTWDriverClass::ModeAirFriendly, FRIENDLY_AIRCRAFT_CAMERA},
    {OTWDriverClass::ModeGroundEnemy, ENEMY_GROUND_UNIT_CAMERA},
    {OTWDriverClass::ModeGroundFriendly, FRIENDLY_GROUND_UNIT_CAMERA},
    {OTWDriverClass::ModeIncoming, INCOMING_MISSILE_CAMERA},
    {OTWDriverClass::ModeTarget, TARGET_CAMERA},
    {OTWDriverClass::ModeTargetToSelf, TARGET_TO_SELF_CAMERA}
};

int ModeTableSize = sizeof(theModeTable) / sizeof(Mode2Cam);

void OTWDriverClass::DisplayInfoBar(void)
{
    Prof(DisplayInfoBar);
    // Center Box on screen (Will ALWAYS be same size (in pixels w,h) regardless of screen resolution)
    // BASICALLY, put centerX,centerY at the centerpoint of where you want the box to draw
    float halfHeight = OTWDriver.renderer->TextHeight() * 1.5F;
    float halfWidth = 1.F;

    OTWDriver.renderer->CenterOriginInViewport(); //Wombat778 4-10-04
    OTWDriver.renderer->SetColor(0x997B5200);   // 60% alpha blue
    OTWDriver.renderer->context.RestoreState(STATE_ALPHA_SOLID);

    OTWDriver.renderer->Tri(-halfWidth, -1.F,
                            halfWidth, -1.F,
                            halfWidth,  -1.F + halfHeight);
    OTWDriver.renderer->Tri(-halfWidth, -1.F,
                            -halfWidth,  -1.F + halfHeight,
                            halfWidth,  -1.F + halfHeight);

    // Outline Translucent BLUE box
    OTWDriver.renderer->SetColor(0xFF000000);   // black

    OTWDriver.renderer->Line(halfWidth, -1.F + halfHeight, -halfWidth, -1.F + halfHeight);

    short cameraID = 0;

    renderer->SetColor(0xff00ff00); // green

    if (actionCameraMode)
    {
        cameraID = ACTION_CAMERA;
    }
    else
    {
        OTWDisplayMode tmpMode = GetOTWDisplayMode();

        for (int i = 0; i < ModeTableSize; i++)
        {
            if (theModeTable[i].theMode == tmpMode)
            {
                if (tmpMode not_eq ModeChase)
                    cameraID = theModeTable[i].theCamID;
                else
                {
                    if (flybyTimer)
                        cameraID = FLY_BY_CAMERA;
                    else
                        cameraID = CHASE_CAMERA;
                }

                break;
            }
        }
    }

    // put a line telling what the camera focus is (label)
    if ((otwPlatform.get() not_eq NULL) and 
        otwPlatform->drawPointer and 
        *((DrawableBSP *)otwPlatform->drawPointer)->Label()
       )
    {
        // avg length of the below string is 120 chars.. so no sweat, cept maybe for null-pointers ?
#define LOCAL_STRING_LENGTH 256
        char tmpo[LOCAL_STRING_LENGTH];
        // callsign
        strcpy(tmpo, CameraLabel[cameraID]);
        strcat(tmpo, ": ");
        strcat(tmpo, ((DrawableBSP *)otwPlatform->drawPointer)->Label());

        // 2 issues here:
        // a) string could be longer as the locally allocated one (bad thing (tm)) - however that´s unlikely, see above
        // b) string could be longer than physical screen size.. FreeFalcon then displays nothing.. also a bit suboptimal..
        // solution for b) need to get renderer->TextWidth() working, if it is >1 then we don´t add a chunk.. or so..
        if (( not otwPlatform->IsGroundVehicle()) and ( not otwPlatform->IsBomb()))
        {
            char tmp[30];

            strcat(tmpo, " - heading: ");

            // heading
            SimMoverClass *mover = static_cast<SimMoverClass*>(otwPlatform.get());
            float theYaw = mover->Yaw() * RTD;

            if (theYaw < 0)
            {
                theYaw += 360.F;
            }

            sprintf(tmp, "%3.i", (int)theYaw);
            strcat(tmpo, tmp);
            strcat(tmpo, " degrees, alt: ");

            // height
            sprintf(tmp, "%5.i", (DrawableBSP*)otwPlatform->GetAltitude());
            strcat(tmpo, tmp);
            strcat(tmpo, " ft (BARO), speed: ");

            float fvalue;

            // speed
            if (otwPlatform->IsMissile())
            {
                MissileClass *mi = static_cast<MissileClass*>(otwPlatform.get());
                fvalue = mi->GetVt() * FTPSEC_TO_KNOTS;
                sprintf(tmp, "%4.f", fvalue);
                strcat(tmpo, tmp);
                strcat(tmpo, " kts (GS), aoa: ");

                fvalue = mi->alpha;
                sprintf(tmp, "%+3.1f", fvalue);
                strcat(tmpo, tmp);
                strcat(tmpo, " degrees");
            }
            else
            {
                fvalue = otwPlatform->GetKias();
                sprintf(tmp, "%4.f", fvalue);
                strcat(tmpo, tmp);
                strcat(tmpo, " kts (IAS)");
            }

            // power
            sprintf(tmp, ", power: %3.f", otwPlatform->PowerOutput() * 100.F);
            strcat(tmpo, tmp);
            strcat(tmpo, " percent");

            // G forces, only for ac
            if (otwPlatform->IsAirplane())
            {
                AircraftClass *ac = static_cast<AircraftClass*>(otwPlatform.get());
                fvalue = ac->GetNz();
                sprintf(tmp, ", %2.1f", fvalue);
                strcat(tmpo, tmp);
                strcat(tmpo, "G, aoa: ");

                fvalue = ac->GetAlpha();
                sprintf(tmp, "%+2.1f", fvalue);
                strcat(tmpo, tmp);
                strcat(tmpo, " degrees");
            }

        }

#if 0 // this would display the text/screenwidth ratio..
        char tmp[20];

        float blubb = renderer->ScreenTextWidth(tmpo) / (float)renderer->GetXRes();
        sprintf(tmp, "%f", blubb);
        renderer->TextCenter(0.0F, (-1.F + 2.F * OTWDriver.renderer->TextHeight()), tmp);
#endif
        renderer->TextCenter(0.0F, (-1.F + 1.2F * OTWDriver.renderer->TextHeight()), tmpo);
    }
    else
    {
        renderer->TextCenter(0.0F, (-1.F + 1.2F * OTWDriver.renderer->TextHeight()), CameraLabel[cameraID]);
    }
}

void OTWDriverClass::ToggleSubTitles() // Retro 20Dec2003
{
    drawSubTitles = not drawSubTitles;
}

/* RETRO RADIOMESS LABELS */
void OTWDriverClass::DrawSubTitles(void)  // Retro 16Dec2003 (all)
{
    Prof(DrawSubTitles);

    if (radioLabel)
    {
        ColouredSubTitle** theLabels = radioLabel->GetTimeSortedMessages(vuxGameTime);

        if (theLabels)
        {
            int i = 0;

            while (theLabels[i])
            {
                if (theLabels[i]->theString)
                {
                    renderer->SetColor(theLabels[i]->theColour);
                    // renderer->TextLeft(-0.95F,  (0.90F-i*0.03F), theLabels[i]->theString);
                    // Retro 10Jan2004 - lower so that they don´t collide with LEF/TEF display
                    renderer->TextLeft(-0.95F, (0.84F - i * 0.03F), theLabels[i]->theString);
                }

                free(theLabels[i]);
                theLabels[i] = 0;
                i++;
            }

            free(theLabels);
            theLabels = 0;
        }
    }
} // Retro radio label end


// All Y'All, put your TEXT stuff in here... if it is not PART of the F16

#define _DO_OWN_START_FRAME_
// PJW: For lack of a better place... I put this here

void OTWDriverClass::DisplayFrontText(void)
{
    //Prof(DisplayFrontText); // Retro 15/10/03
    //Prof_update(ProfilerActive); // Retro 16/10/03

    // Retro 7May2004 - for pretty screens we don´t want any 2d text on our screen
    // See OTWDriver.h for explanation
    if (takePrettyScreenShot == EXECUTE)
    {
        takeScreenShot = TRUE; // tell FreeFalcon to take a shot
        takePrettyScreenShot = CLEANUP; // advance state..
        return; // deactivate 2d text (by NOT drawing it :p)
    }
    else if (takePrettyScreenShot == CLEANUP)
    {
        takePrettyScreenShot = OFF; // advance state..
        DrawableBSP::drawLabels = LabelState; // reactivate labels again (if they were on)
    }

    // Retro 7-8May2004 end

    float centerx = DisplayOptions.DispWidth * 0.5f;
    float top;
    float left;
    float bottom;
    float right;
    short chat_cnt;
    int oldFont;

    // SetFont
    oldFont = VirtualDisplay::CurFont();
    VirtualDisplay::SetFont(pCockpitManager->GeneralFont());

    // save the current viewport
    OTWDriver.renderer->GetViewport(&left, &top, &right, &bottom);

#ifdef _DO_OWN_START_FRAME_
    OTWDriver.renderer->StartDraw();
#endif

    // set the new viewport
    OTWDriver.renderer->SetViewport(-1.0, 1.0, 1.0, -1.0);

    // Any Text stuff you want drawn below here
    if (vuxRealTime bitand 0x200) // Blink every half second
    {
        // Display the PAUSE/X2/X4 compression strings HERE
        if ((vuxRealTime bitand 0x200) and (remoteCompressionRequests or targetCompressionRatio not_eq 1))
        {
            float offset = 0.0f;
            long color;
            // THIS SHOULD BE TRANSLATABLE TEXT
            //
            offset = -(OTWDriver.renderer->ScreenTextWidth(CompressionStr[0]) + COMPRESS_SPACING +
                       OTWDriver.renderer->ScreenTextWidth(CompressionStr[2]) + COMPRESS_SPACING +
                       OTWDriver.renderer->ScreenTextWidth(CompressionStr[3]) + COMPRESS_SPACING) * 0.5f;

            if (( not targetCompressionRatio) or ( not FalconLocalSession->GetReqCompression()) or (remoteCompressionRequests bitand REMOTE_REQUEST_PAUSE))
            {
                // if ANY compression OR compression requests == 0... Draw PAUSE
                // if compression == our compression == all requested compressions
                // Use RED
                // if compression == our compression not_eq all requested compression
                // Use YELLOW
                // if compression not_eq our compression
                // Use GREEN
                //
                if (( not targetCompressionRatio) and (( not FalconLocalSession->GetReqCompression()) and (remoteCompressionRequests bitand REMOTE_REQUEST_PAUSE) or not gCommsMgr->Online()))
                    color = 0xff0000ff;
                else if (( not FalconLocalSession->GetReqCompression()))
                    color = 0xff00ff00;
                else
                    color = 0xff00ffff;

                OTWDriver.renderer->SetColor(color);
                OTWDriver.renderer->ScreenText(centerx + offset, COMPRESS_Y, CompressionStr[0]);
            }

            offset += OTWDriver.renderer->ScreenTextWidth(CompressionStr[0]) + COMPRESS_SPACING;

            if ((targetCompressionRatio == 2) or (FalconLocalSession->GetReqCompression() == 2) or (remoteCompressionRequests bitand REMOTE_REQUEST_2))
            {
                // if ANY compression OR compression requests == 2... Draw 2X
                // if compression == our compression == all requested compressions
                // Use RED
                // if compression == our compression not_eq all requested compression
                // Use YELLOW
                // if compression not_eq our compression
                // Use GREEN
                //
                if ((targetCompressionRatio == 2) and ((FalconLocalSession->GetReqCompression() == 2) and (remoteCompressionRequests bitand REMOTE_REQUEST_2) or not gCommsMgr->Online()))
                    color = 0xff0000ff;
                else if ((FalconLocalSession->GetReqCompression() == 2))
                    color = 0xff00ff00;
                else
                    color = 0xff00ffff;

                OTWDriver.renderer->SetColor(color);
                OTWDriver.renderer->ScreenText(centerx + offset, COMPRESS_Y, CompressionStr[2]);
            }

            // JB 010109 offset+=OTWDriver.renderer->ScreenTextWidth(CompressionStr[2]) + COMPRESS_SPACING;
            if ((targetCompressionRatio == 4) or (FalconLocalSession->GetReqCompression() == 4) or (remoteCompressionRequests bitand REMOTE_REQUEST_4))
            {
                // if ANY compression OR compression requests > 2... Draw 4X
                // if compression == our compression == all requested compressions
                // Use RED
                // if compression == our compression not_eq all requested compression
                // Use YELLOW
                // if compression not_eq our compression
                // Use GREEN
                //
                if ((targetCompressionRatio == 4) and ((FalconLocalSession->GetReqCompression() == 4) and (remoteCompressionRequests bitand REMOTE_REQUEST_4) or not gCommsMgr->Online()))
                    color = 0xff0000ff;
                else if ((FalconLocalSession->GetReqCompression() == 4))
                    color = 0xff00ff00;
                else
                    color = 0xff00ffff;

                OTWDriver.renderer->SetColor(color);
                OTWDriver.renderer->ScreenText(centerx + offset, COMPRESS_Y, CompressionStr[3]);
            }

            // JB 010109
            //offset+=OTWDriver.renderer->ScreenTextWidth(CompressionStr[3]) + COMPRESS_SPACING;
            if ((targetCompressionRatio == 8) or (FalconLocalSession->GetReqCompression() == 8) or (remoteCompressionRequests bitand REMOTE_REQUEST_8))
            {
                // if ANY compression OR compression requests > 2... Draw 8X
                // if compression == our compression == all requested compressions
                // Use RED
                // if compression == our compression not_eq all requested compression
                // Use YELLOW
                // if compression not_eq our compression
                // Use GREEN
                //
                if ((targetCompressionRatio == 8) and ((FalconLocalSession->GetReqCompression() == 8) and (remoteCompressionRequests bitand REMOTE_REQUEST_8) or not gCommsMgr->Online()))
                    color = 0xff0000ff;
                else if ((FalconLocalSession->GetReqCompression() == 8))
                    color = 0xff00ff00;
                else
                    color = 0xff00ffff;

                OTWDriver.renderer->SetColor(color);
                OTWDriver.renderer->ScreenText(centerx + offset, COMPRESS_Y, "x8");
            }

            //offset+=OTWDriver.renderer->ScreenTextWidth("x8") + COMPRESS_SPACING;
            if ((targetCompressionRatio == 16) or (FalconLocalSession->GetReqCompression() == 16) or (remoteCompressionRequests bitand REMOTE_REQUEST_16))
            {
                // if ANY compression OR compression requests > 2... Draw 16X
                // if compression == our compression == all requested compressions
                // Use RED
                // if compression == our compression not_eq all requested compression
                // Use YELLOW
                // if compression not_eq our compression
                // Use GREEN
                //
                if ((targetCompressionRatio == 16) and ((FalconLocalSession->GetReqCompression() == 16) and (remoteCompressionRequests bitand REMOTE_REQUEST_16) or not gCommsMgr->Online()))
                    color = 0xff0000ff;
                else if ((FalconLocalSession->GetReqCompression() == 16))
                    color = 0xff00ff00;
                else
                    color = 0xff00ffff;

                OTWDriver.renderer->SetColor(color);
                OTWDriver.renderer->ScreenText(centerx + offset, COMPRESS_Y, "x16");
            }

            //offset+=OTWDriver.renderer->ScreenTextWidth("x16") + COMPRESS_SPACING;
            if ((targetCompressionRatio == 32) or (FalconLocalSession->GetReqCompression() == 32) or (remoteCompressionRequests bitand REMOTE_REQUEST_32))
            {
                // if ANY compression OR compression requests > 2... Draw 32X
                // if compression == our compression == all requested compressions
                // Use RED
                // if compression == our compression not_eq all requested compression
                // Use YELLOW
                // if compression not_eq our compression
                // Use GREEN
                //
                if ((targetCompressionRatio == 32) and ((FalconLocalSession->GetReqCompression() == 32) and (remoteCompressionRequests bitand REMOTE_REQUEST_32) or not gCommsMgr->Online()))
                    color = 0xff0000ff;
                else if ((FalconLocalSession->GetReqCompression() == 32))
                    color = 0xff00ff00;
                else
                    color = 0xff00ffff;

                OTWDriver.renderer->SetColor(color);
                OTWDriver.renderer->ScreenText(centerx + offset, COMPRESS_Y, "x32");
            }

            //offset+=OTWDriver.renderer->ScreenTextWidth("x32") + COMPRESS_SPACING;
            if ((targetCompressionRatio == 64) or (FalconLocalSession->GetReqCompression() == 64) or (remoteCompressionRequests bitand REMOTE_REQUEST_64))
            {
                // if ANY compression OR compression requests > 2... Draw 64X
                // if compression == our compression == all requested compressions
                // Use RED
                // if compression == our compression not_eq all requested compression
                // Use YELLOW
                // if compression not_eq our compression
                // Use GREEN
                //
                if ((targetCompressionRatio == 64) and ((FalconLocalSession->GetReqCompression() == 64) and (remoteCompressionRequests bitand REMOTE_REQUEST_64) or not gCommsMgr->Online()))
                    color = 0xff0000ff;
                else if ((FalconLocalSession->GetReqCompression() == 64))
                    color = 0xff00ff00;
                else
                    color = 0xff00ffff;

                OTWDriver.renderer->SetColor(color);
                OTWDriver.renderer->ScreenText(centerx + offset, COMPRESS_Y, "x64");
            }

            //offset+=OTWDriver.renderer->ScreenTextWidth("x64") + COMPRESS_SPACING;
            if ((targetCompressionRatio == 128) or (FalconLocalSession->GetReqCompression() == 128) or (remoteCompressionRequests bitand REMOTE_REQUEST_128))
            {
                // if ANY compression OR compression requests > 2... Draw 128X
                // if compression == our compression == all requested compressions
                // Use RED
                // if compression == our compression not_eq all requested compression
                // Use YELLOW
                // if compression not_eq our compression
                // Use GREEN
                //
                if ((targetCompressionRatio == 128) and ((FalconLocalSession->GetReqCompression() == 128) and (remoteCompressionRequests bitand REMOTE_REQUEST_128) or not gCommsMgr->Online()))
                    color = 0xff0000ff;
                else if ((FalconLocalSession->GetReqCompression() == 128))
                    color = 0xff00ff00;
                else
                    color = 0xff00ffff;

                OTWDriver.renderer->SetColor(color);
                OTWDriver.renderer->ScreenText(centerx + offset, COMPRESS_Y, "x128");
            }

            //offset+=OTWDriver.renderer->ScreenTextWidth("x128") + COMPRESS_SPACING;
            if ((targetCompressionRatio == 256) or (FalconLocalSession->GetReqCompression() == 256) or (remoteCompressionRequests bitand REMOTE_REQUEST_256))
            {
                // if ANY compression OR compression requests > 2... Draw 256X
                // if compression == our compression == all requested compressions
                // Use RED
                // if compression == our compression not_eq all requested compression
                // Use YELLOW
                // if compression not_eq our compression
                // Use GREEN
                //
                if ((targetCompressionRatio == 256) and ((FalconLocalSession->GetReqCompression() == 256) and (remoteCompressionRequests bitand REMOTE_REQUEST_256) or not gCommsMgr->Online()))
                    color = 0xff0000ff;
                else if ((FalconLocalSession->GetReqCompression() == 256))
                    color = 0xff00ff00;
                else
                    color = 0xff00ffff;

                OTWDriver.renderer->SetColor(color);
                OTWDriver.renderer->ScreenText(centerx + offset, COMPRESS_Y, "x256");
            }

            //offset+=OTWDriver.renderer->ScreenTextWidth("x256") + COMPRESS_SPACING;
            if ((targetCompressionRatio == 512) or (FalconLocalSession->GetReqCompression() == 512) or (remoteCompressionRequests bitand REMOTE_REQUEST_512))
            {
                // if ANY compression OR compression requests > 2... Draw 512X
                // if compression == our compression == all requested compressions
                // Use RED
                // if compression == our compression not_eq all requested compression
                // Use YELLOW
                // if compression not_eq our compression
                // Use GREEN
                //
                if ((targetCompressionRatio == 512) and ((FalconLocalSession->GetReqCompression() == 512) and (remoteCompressionRequests bitand REMOTE_REQUEST_512) or not gCommsMgr->Online()))
                    color = 0xff0000ff;
                else if ((FalconLocalSession->GetReqCompression() == 512))
                    color = 0xff00ff00;
                else
                    color = 0xff00ffff;

                OTWDriver.renderer->SetColor(color);
                OTWDriver.renderer->ScreenText(centerx + offset, COMPRESS_Y, "x512");
            }

            //offset+=OTWDriver.renderer->ScreenTextWidth("x512") + COMPRESS_SPACING;
            if ((targetCompressionRatio == 1024) or (FalconLocalSession->GetReqCompression() == 1024) or (remoteCompressionRequests bitand REMOTE_REQUEST_1024))
            {
                // if ANY compression OR compression requests > 2... Draw 1024X
                // if compression == our compression == all requested compressions
                // Use RED
                // if compression == our compression not_eq all requested compression
                // Use YELLOW
                // if compression not_eq our compression
                // Use GREEN
                //
                if ((targetCompressionRatio == 1024) and ((FalconLocalSession->GetReqCompression() == 1024) and (remoteCompressionRequests bitand REMOTE_REQUEST_1024) or not gCommsMgr->Online()))
                    color = 0xff0000ff;
                else if ((FalconLocalSession->GetReqCompression() == 1024))
                    color = 0xff00ff00;
                else
                    color = 0xff00ffff;

                OTWDriver.renderer->SetColor(color);
                OTWDriver.renderer->ScreenText(centerx + offset, COMPRESS_Y, "x1024");
            }

            //offset+=OTWDriver.renderer->ScreenTextWidth("x1024") + COMPRESS_SPACING;
            // JB 010109
        }

        if (gameCompressionRatio and not SimDriver.MotionOn())
        {
            renderer->SetColor(0xff0000ff);
            renderer->ScreenText(centerx - OTWDriver.renderer->ScreenTextWidth(CompressionStr[4]) * 0.5f, COMPRESS_Y, CompressionStr[4]);
        }
    }

    if ( not g_bACMIRecordMsgOff and gACMIRec.IsRecording())
    {
        int pct;
        int j;

        renderer->SetColor(0xff0000ff);
        renderer->TextCenter(0.0F, 0.95F, CameraLabel[RECORDING]);

        // get the percent tape is full....
        pct = gACMIRec.PercentTapeFull();

        if (pct > 10)
            pct = 10;

        for (j = 0; j < pct; j++)
            gAcmiStr[j] = '+';

        renderer->TextCenter(0.0F, 0.92F, gAcmiStr);
    }

    if (showFrontText bitand (SHOW_TE_SCORES bitor SHOW_DOGFIGHT_SCORES))
    {
        float centerX = DisplayOptions.DispWidth / 2.0F;
        float centerY = DisplayOptions.DispHeight / 2.0F;
        short i;
        float x, y;
        float w, h;

        x = centerX + centerX * (SCORENAME_X - 0.02f);
        y = centerY - centerY * (SCORE_Y + 0.1f);
        w = centerX + (centerX * (SCOREPOINT_X + 0.02f)) - x;
        h = centerY * 12 * 0.05f;

        OTWDriver.renderer->SetColor(0x997B5200);   // 60% alpha blue
        OTWDriver.renderer->context.RestoreState(STATE_ALPHA_SOLID);

        OTWDriver.renderer->Render2DTri(x, y,  x + w, y,  x + w, y + h);
        OTWDriver.renderer->Render2DTri(x, y,  x, y + h,  x + w, y + h);

        // Outline Translucent BLUE box
        OTWDriver.renderer->SetColor(0xFF000000);   // black
        OTWDriver.renderer->Render2DLine(x, y, x + w, y);
        OTWDriver.renderer->Render2DLine(x, y + h, x + w, y + h);
        OTWDriver.renderer->Render2DLine(x, y, x, y + h);
        OTWDriver.renderer->Render2DLine(x + w, y, x + w, y + h);

        if (showFrontText bitand SHOW_DOGFIGHT_SCORES)
        {
            renderer->SetColor(0xfffefefe); // not quite white, so the color won't change
            renderer->TextCenter(SCORENAME_X + (SCOREPOINT_X - SCORENAME_X) * 0.5f,  SCORE_Y - (float)(-1) * 0.05f, "Sierra Hotel");

            if (gRefreshScoresList)
            {
                MakeDogfightTopTen(SimDogfight.GetGameType());
                gRefreshScoresList = 0;
            }
        }
        else if (showFrontText bitand SHOW_TE_SCORES)
        {
            renderer->SetColor(0xfffefefe); // not quite white, so the color won't change
            renderer->TextCenter(SCORENAME_X + (SCOREPOINT_X - SCORENAME_X) * 0.5f,  SCORE_Y - (float)(-1) * 0.05f, "Game Over");

            if (gRefreshScoresList)
            {
                MakeTacEngScoreList();
                gRefreshScoresList = 0;
            }
        }

        for (i = 0; i < 10; i++)
        {
            // renderer->SetColor(gScoreColor[i]); // Not set yet
            renderer->SetColor(0xfffefefe); // not quite white, so the color won't change

            if (gScoreName[i][0])
            {
                renderer->TextLeft(SCORENAME_X,  SCORE_Y - (float)i * 0.05f, gScoreName[i]);
                renderer->TextRight(SCOREPOINT_X,  SCORE_Y - (float)i * 0.05f, gScorePoints[i]);
            }
        }
    }

    // Display any Text Messages sent TO me
    if (showFrontText bitand (SHOW_MESSAGES) and textMessage[0][0]) // Check to see if there are any
    {
        chat_cnt = 0;

        while (chat_cnt < MAX_CHAT_LINES and textMessage[chat_cnt][0])
        {
            if (TheHud)
                renderer->SetColor(TheHud->GetHudColor());
            else
                renderer->SetColor(0xff00ff00);

            renderer->TextLeft(MESSAGE_X,  MESSAGE_Y - (float)chat_cnt * 0.05f, textMessage[chat_cnt]);
            chat_cnt++;
        }
    }

    // Display a ChatBox if I am typeing a message
    //if(flags bitand ShowChatBox) // put this check in
    if (showFrontText bitand (SHOW_CHATBOX))
        DisplayChatBox();

    if ((drawInfoBar) and ( not DisplayInCockpit())) // Retro 16Dec2003
    {
        DisplayInfoBar(); // Retro 16Dec2003
    }
    else // Retro 16Dec2003
    {
        // display text for some camera settings
        // TODO:  This should be a string table, not a slew of "if"s
        if ( not DisplayInCockpit())
        {
            short cameraID = 0;

            renderer->SetColor(0xff00ff00);

            if (actionCameraMode)
            {
                cameraID = ACTION_CAMERA;
            }
            else if (GetOTWDisplayMode() == ModeChase and flybyTimer)
            {
                cameraID = FLY_BY_CAMERA;
            }
            else if (GetOTWDisplayMode() == ModeChase and not flybyTimer)
            {
                cameraID = CHASE_CAMERA;
            }
            else if (GetOTWDisplayMode() == ModeOrbit)
            {
                cameraID = ORBIT_CAMERA;
            }
            else if (GetOTWDisplayMode() == ModeSatellite)
            {
                cameraID = SATELLITE_CAMERA;
            }
            else if (GetOTWDisplayMode() == ModeWeapon)
            {
                cameraID = WEAPON_CAMERA;
            }
            else if (GetOTWDisplayMode() == ModeTargetToWeapon)
            {
                cameraID = TARGET_TO_WEAPON_CAMERA;
            }
            else if (GetOTWDisplayMode() == ModeAirEnemy)
            {
                cameraID = ENEMY_AIRCRAFT_CAMERA;
            }
            else if (GetOTWDisplayMode() == ModeAirFriendly)
            {
                cameraID = FRIENDLY_AIRCRAFT_CAMERA;
            }
            else if (GetOTWDisplayMode() == ModeGroundEnemy)
            {
                cameraID = ENEMY_GROUND_UNIT_CAMERA;
            }
            else if (GetOTWDisplayMode() == ModeGroundFriendly)
            {
                cameraID = FRIENDLY_GROUND_UNIT_CAMERA;
            }
            else if (GetOTWDisplayMode() == ModeIncoming)
            {
                cameraID = INCOMING_MISSILE_CAMERA;
            }
            else if (GetOTWDisplayMode() == ModeTarget)
            {
                cameraID = TARGET_CAMERA;
            }
            else if (GetOTWDisplayMode() == ModeTargetToSelf)
            {
                cameraID = TARGET_TO_SELF_CAMERA;
            }

            // put a line telling what the camera focus is (label)
            if (
                (otwPlatform.get() not_eq NULL) and 
                otwPlatform->drawPointer and 
                *((DrawableBSP *)otwPlatform->drawPointer)->Label()
            )
            {
                strcpy(tmpStr, CameraLabel[cameraID]);
                strcat(tmpStr, ": ");
                strcat(tmpStr, ((DrawableBSP *)otwPlatform->drawPointer)->Label());
                renderer->TextCenter(0.0F, 0.89F, tmpStr);
            }
            else
                renderer->TextCenter(0.0F, 0.89F, CameraLabel[cameraID]);
        }
    } // Retro 16Dec2003

    if (showPos)
        ShowPosition();

    if (showAero)
        ShowAerodynamics();

    //if (g_bShowFlaps) TJL 11/09/03 Show Flaps
    if ( not showFlaps)
        ShowFlaps();

    if (getNewCameraPos)
        GetUserPosition();

    if (showEngine) // Retro 1Feb2004
        ShowEngine(); // Retro 1Feb2004

    // RV - Biker
    ShowCatMessage();

    if (showThrustReverse)//Cobra
        ShowThrustReverse();


    currentFPS = 1.0F / (float)(frameTime) * 1000.0F;//Cobra

#ifdef SHOW_FRAME_RATE

    if (ShowFrameRate)
    {

        if (gSimTime > gSimTimeMax)
            gSimTimeMax = gSimTime;

        if (gCampTime > gCampTimeMax)
            gCampTimeMax = gCampTime;

        if (gGraphicsTimeLast > gGraphicsTimeLastMax)
            gGraphicsTimeLastMax = gGraphicsTimeLast;

        renderer->SetColor(0xfff0f0f0);  // Keeps this color from randomly changing
        extern int vuentitycount, vuentitypeak;
        extern int vumessagecount, vumessagepeakcount;
        VirtualDisplay::SetFont(2);
        sprintf(tmpStr,
                "FPS %.2f GameTime=%.4f SFX=%3d PObjs=%3d DObjs=%3d Camp T=%2d AVE=%2d MAX=%4d; Sim T=%2d MAX=%4d;"
                "Graphics T=%2d AVE=%2d MAX=%4d;TrailNodes=%d Voices=%d; Bandwidth=%f",
                1.0F / (float)(frameTime) * 1000.0F,
                (float)(vuxGameTime / 1000.0),
                gTotSfx,
                numObjsProcessed,
                numObjsInDrawList,
                gCampTime,
                gAveCampTime,
                gCampTimeMax,
                gSimTime,
                gSimTimeMax,
                gGraphicsTimeLast,
                gAveSimGraphicsTime,
                gGraphicsTimeLastMax,
                gTrailNodeCount,
                gVoiceCount,
                0.0f
               );
        renderer->TextLeft(-0.95F,  0.95F, tmpStr);
        static int lastTime = 0;

        if (lastTime not_eq vuxGameTime)
        {
            MonoPrint(tmpStr);

            lastTime = vuxGameTime;
        }

        renderer->TextLeft(-0.95F,  0.95F, tmpStr);
        sprintf(tmpStr, "Textures Terr %4d, far %4d Lods %d LodTex %d, Entities cur %4d max %4d VuMessageCount %4d MAX=%4d",
                TheTerrTextures.LoadedTextureCount,
                TheFarTextures.LoadedTextureCount,
                ObjectLOD::lodsLoaded, TextureBankClass::textureCount,
                vuentitycount, vuentitypeak,
                vumessagecount, vumessagepeakcount);
        renderer->TextLeft(-0.95F,  0.90F, tmpStr);
        extern int ObjsDeagg,
               FeatsDeagg,
               UnitsDeagg,
               gObjectiveCount,
               SimFeatures,
               ObjectNodes,
               ObjectReferences,
               gUnitCount;
        sprintf(tmpStr, "Deagged: Objectives=%5d  Features=%5d - Units %5d",
                ObjsDeagg,
                FeatsDeagg,
                UnitsDeagg
               );
        renderer->TextLeft(-0.95F,  0.85F, tmpStr);

        sprintf(tmpStr, "Total:   Objectives=%5d  Features=%5d - Units %5d",
                gObjectiveCount,
                SimFeatures,
                gUnitCount
               );
        renderer->TextLeft(-0.95F,  0.80F, tmpStr);

        sprintf(tmpStr, "SimObjectType: Nodes %5d  Referenced %5d",
                ObjectNodes,
                ObjectReferences
               );
        renderer->TextLeft(-0.95F,  0.75F, tmpStr);
    }
    else
    {
        gCampTimeMax = 0;
        gSimTimeMax = 0;
        gGraphicsTimeLastMax = 0;
    }

#else

    if (ShowFrameRate)
    {
        //Wombat778 3-24-04  Added new FPS counter to actually count the number of frames that occurred in the last second
        if (g_nNewFPSCounter)
        {
            static float newsecond = 0;
            static int numframes = 0;
            static int lastfps = 0;

            numframes++;

            if ((vuxRealTime > newsecond) or (newsecond > vuxRealTime + (2000.0f / (float) g_nNewFPSCounter)))
            {
                newsecond = vuxRealTime + (1000.0f / (float) g_nNewFPSCounter);
                lastfps = numframes;
                numframes = 0;

            }

            int tmp = lTestFlag1;

            renderer->SetColor(0xfff0f0f0);  // Keeps this color from randomly changing

            lTestFlag1 = 0;
            // FPS here
            sprintf(tmpStr, "FPS %5d", lastfps * g_nNewFPSCounter);
            VirtualDisplay::SetFont(2);
            renderer->TextLeft(-0.95F,  0.95F, tmpStr, 2);
            lTestFlag1 = tmp;
        }
        else
        {
            int tmp = lTestFlag1;

            renderer->SetColor(0xfff0f0f0);  // Keeps this color from randomly changing

            lTestFlag1 = 0;
            sprintf(tmpStr, "FPS %5.1f", 1.0F / (float)(frameTime) * 1000.0F);
            VirtualDisplay::SetFont(2);
            renderer->TextLeft(-0.95F,  0.95F, tmpStr, 2);
            lTestFlag1 = tmp;
        }

#ifdef _USE_RED_PROFILER_
        LIST_PROFILES;
        char tmpStr[256];
        renderer->SetViewport(-1.0, 1.0, 1.0, -1.0);
        VirtualDisplay::SetFont(2);
        int a, y = 0;

        for (a = 0; a < MAX_PROFILES; a++)
        {
            if (REPORT_PROFILE_NR(a, tmpStr))
                renderer->TextLeft(-0.95F,  0.90F - (float)y++ / 100 * 5, tmpStr, 2);
        }

        y = 0;

        for (a = 0; a < MAX_MESSAGES; a++)
        {
            REPORT_MESSAGE(a, tmpStr);

            if (tmpStr[0]) renderer->TextRight(0.80F,  0.90F - (float)y++ / 100 * 5, tmpStr, 2);
        }

#endif
    }

#endif

    if (drawSubTitles)
    {
        // Retro 16Dec2003
        DrawSubTitles();  // Retro 16Dec2003
    }

#ifdef Prof_ENABLED
    DisplayProfilerText(); // Retro 21Dec2003
#endif

#ifdef AXISTEST
#pragma message("__________AXISTEST defined, remove before release __________")
    DisplayAxisValues(); // Retro 1Jan2004
#endif

    // if ((SimDriver.GetPlayerAircraft()) and (SimDriver.GetPlayerAircraft()->IsSetFalcFlag (FEC_INVULNERABLE)) and (vuxRealTime bitand 0x200))
    // {
    // renderer->SetColor (0xfffefefe); // Keeps this color from randomly changing
    // renderer->TextCenter (0.0F, 0.5F, "Invincible");
    // }

    //
    //
    // No more drawing AFTER this
    ///////////////////////////////////////////
#ifdef _DO_OWN_START_FRAME_
    OTWDriver.renderer->EndDraw();
#endif
    // restore the old viewport
    OTWDriver.renderer->SetViewport(left, top, right, bottom);
    VirtualDisplay::SetFont(oldFont);
}

/*****************************************************************************/
// Retro 21Dec2003
/*****************************************************************************/
void OTWDriverClass::ToggleProfilerDisplay(void)
{
    DisplayProfiler = not DisplayProfiler;
}

/*****************************************************************************/
// Retro 21Dec2003
/*****************************************************************************/
void OTWDriverClass::ToggleProfilerActive(void)
{
    ProfilerActive = not ProfilerActive;
}

/*****************************************************************************/
// Retro 21Dec2003
// requests a report from the profiler and displays it
// also displays a 'virtual cursor' so that user can navigate the call graph
/*****************************************************************************/
void OTWDriverClass::DisplayProfilerText(void)
{
#ifdef Prof_ENABLED // Retro 15/10/03

    if (DisplayProfiler)
    {
        Prof(DisplayProfilerOutput_Scope); // Retro 15/10/03

#define MAX_LINE_NUM 35

        char** theReport = Prof_dumpOverlay();

        if (theReport)
        {
            int virtualCursor = Prof_get_cursor();
            renderer->SetColor(0xffff0000);  // blue

#define XPOS_NAME -0.95f
#define XPOS_SELF -0.40f
#define XPOS_HIER -0.25f
#define XPOS_COUNT -0.1f

#define YPOS_START 0.90f
#define YPOS_DELTA 0.05f

            float xpos = XPOS_NAME;
            float ypos = YPOS_START;

            // first line is the title, with info about frametime, avg fps etc
            // second line can be speedstep-like timer warning
            for (int titleIndex = 0; titleIndex < 2; titleIndex++)
            {
                if (theReport[titleIndex])
                {
                    renderer->TextLeft(xpos,  ypos, theReport[titleIndex]);
                    ypos -= YPOS_DELTA;
                    free(theReport[titleIndex]);
                    theReport[titleIndex] = 0;
                }
            }

            // Retro 26Dec2003 - fixed display of "speedstep-like timer" warning
            for (int i = 2; i < (MAX_LINE_NUM * 4) + 2; i++)
            {
                if (theReport[i])
                {
                    if (i == (virtualCursor * 4) + 2)
                        renderer->SetColor(0xFF0000FF); // Retro, red

                    switch (i % 4)
                    {
                        case 1:
                            xpos = XPOS_COUNT;
                            break;

                        case 2:
                            xpos = XPOS_NAME;
                            ypos -= YPOS_DELTA;
                            break;

                        case 3:
                            xpos = XPOS_SELF;
                            break;

                        case 0:
                            xpos = XPOS_HIER;
                            break;
                    }

                    if (i == (virtualCursor * 4) + 6)
                        renderer->SetColor(0xffff0000); // Retro, get back to blue

                    renderer->TextLeft(xpos,  ypos, theReport[i]);
                }

                free(theReport[i]);
                theReport[i] = 0;
            }

            free(theReport);
            theReport = 0;
        }
    }

#endif // Prof_ENABLED - Retro 15/10/03
}



#include "simio.h"   // Retro 31Dec2003
extern SIMLIB_IO_CLASS IO;   // Retro 31Dec2003

void OTWDriverClass::RenderFrame()
{
    int i;
    float dT;
    static int count = 0;
    float top, bottom;
    ViewportBounds viewportBounds;
    Tpoint viewPos;
    int camCount;
    int oldFont;

    //START_PROFILE("OTWRENDER");
    // convert frame loop time to secs from ms
    dT = (float)frameTime * 0.001F;

    // clamp dT
    if (dT < 0.01f)
    {
        dT = 0.01f;
    }
    else if (dT > 0.5f)
    {
        dT = 0.5f;
    }

    // 2002-02-15 MOVED FROM BELOW BY S.G.
    // I need to set it to false if the current otwPlatform is not SimDriver.GetPlayerAircraft()
    BOOL okToDoCockpitStuff = TRUE;

    extern bool MouseMenuActive;

    // Retro 31Dec2003 start
    // the position here might not be the best.. has to coordinated with the g_bLookCloserFix I think..
    // Should be coordinated with wombat´s keypresses: if this active
    // is used, then the keypresses (and maybe the 'l' key) should
    // be deactivated
    if (( not actionCameraMode) and ( not MouseMenuActive))
    {
        // Retro 20Feb2004 - no FOV control in actioncam and when the 'Exit mission' menu is active
        if (IO.AnalogIsUsed(AXIS_FOV))
        {
            //Wombat778 1-15-03 rewrote slighty so that center of axis is the middle of the FOV range
            float theFOV = (g_fMaximumFOV - g_fMinimumFOV) * IO.GetAxisValue(AXIS_FOV) / 15000.0f;
            OTWDriver.SetFOV((g_fMinimumFOV + theFOV) * DTR);
        }
    }

    // Retro 31Dec2003 end

    // JAM 17Dec03 - Tidied up a little
    // If we're sitting in our own aircraft...
    if ((otwPlatform.get() not_eq NULL) and (otwPlatform.get() == SimDriver.GetPlayerAircraft()))
    {
        if (GetOTWDisplayMode() == ModePadlockF3 or (GetOTWDisplayMode() == Mode3DCockpit and mDoSidebar == TRUE))
        {
            pPadlockCPManager->SetNextView();
        }
        else if (GetOTWDisplayMode() == Mode2DCockpit)
        {
            pCockpitManager->SetNextView();

            // dpc LookCloserFix start
            // If Hud is present in 2D Cockpit view, make look closer zoom through the boresight cross in the HUD
            // This assumes that normal FOV = 60 deg., narrow FOV = 20 deg.
            // and that cockpit designer took time to align hud position correctly
            // (boresight cross should be drawn exactly at 0 deg. pan and 0 deg. tilt pixel
            // OTWDriver.SetCameraPanTilt(pCockpitManager->GetPan(), pCockpitManager->GetTilt());
            if (g_bLookCloserFix)
            {
                float pan = pCockpitManager->GetPan();
                float tilt = pCockpitManager->GetTilt();
                //Wombat778 10-31-2003 shouldnt be necessary anymore
                //extern int narrowFOV;
                extern ViewportBounds hudViewportBounds;
                float normHFOV = 60.0F * DTR;
                //float narrowHFOV = 20.0F * DTR;
                //Wombat778 9-27-2003 Modified to allow fix to work with the current FOV (not 20 degrees)
                float narrowHFOV = OTWDriver.GetFOV();
                float normVFOV = 2 * (float)atan2(3.0f * tan(normHFOV / 2), 4.0f);
                float narrowVFOV = 2 * (float)atan2(3.0f * tan(narrowHFOV / 2), 4.0f);
                float ratioH = normHFOV / narrowHFOV;
                float ratioV = normVFOV / narrowVFOV;

                //Wombat778 10-31-2003 changed looking at narrowFOV to checking actual FOV
                if (
                    OTWDriver.GetFOV() not_eq 60.0f and pCockpitManager->ShowHud()
                   and pCockpitManager->GetViewportBounds(&hudViewportBounds, BOUNDS_HUD)
                )
                {
                    //make sure we don't div with 0
                    if (fabs(pan) > 0.5 * DTR)
                    {
                        pan = pan / ratioH * (float)(tan(pan) * tan(narrowHFOV / 2) /
                                                     (tan(pan / ratioH) * tan(normHFOV / 2)))
                              ;
                    }
                    else
                    {
                        pan = pan / ratioH;
                    }

                    if (fabs(tilt) > 0.5 * DTR)
                    {
                        tilt = tilt /
                               ratioV * (float)(tan(tilt) * tan(narrowVFOV / 2) / (tan(tilt / ratioV) * tan(normVFOV / 2)))
                               ;
                    }
                    else
                    {
                        tilt = tilt / ratioV;
                    }
                }

                OTWDriver.SetCameraPanTilt(pan, tilt);
            }
            else
            {
                OTWDriver.SetCameraPanTilt(pCockpitManager->GetPan(), pCockpitManager->GetTilt());
            }

            //dpc LookCloserFix end
        }
    }
    // 2002-02-15 ADDED BY S.G.
    // If the otwPlatform is NOT us, don't do its cockpit stuff since I'll never get in his seat
    else
    {
        okToDoCockpitStuff = FALSE;
    }

    // Find new cockpit if needed
    if (viewStep not_eq 0)
    {
        lastotwPlatform = otwPlatform;
        FindNewOwnship();
    }

    // 2002-02-15 ADDED BY S.G.
    // If the otwPlatform is NOT us, don't do its cockpit stuff since I'll never get in his seat
    if ( not otwPlatform or otwPlatform.get() not_eq SimDriver.GetPlayerAircraft())
    {
        okToDoCockpitStuff = FALSE;
    }

    if ((GetOTWDisplayMode() == ModePadlockF3 or GetOTWDisplayMode() == Mode3DCockpit) and mDoSidebar)
    {
        // Set viewport for Padlock
        renderer->SetViewport(padlockWindow[0][0], padlockWindow[0][1], padlockWindow[0][2], padlockWindow[0][3]);
    }
    else if (otwPlatform.get() == SimDriver.GetPlayerAircraft() and pCockpitManager)
    {
        renderer->SetViewport(-1.0F, 1.0F, 1.0F, pCockpitManager->GetCockpitMaskTop());
    }
    else
    {
        renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);
    }

    // This updates the positions of all the drawable objects
    UpdateVehicleDrawables();

    // Add the F3 Padlock if required
    if ((GetOTWDisplayMode() == ModePadlockF3) and (otwPlatform.get() == SimDriver.GetPlayerAircraft()))
    {
        if (tgtStep)
        {
            Padlock_FindNextPriority(FALSE);
            tgtStep = 0;
        }
    }

    //START_PROFILE("RENDER 3DPIT");
    // Update flight instrument data used by HUD and cockpit
    // 2002-02-15 MODIFIED BY S.G. Added okToDoCockpitStuff so this is done only when the cockpit is from our plane
    if ((otwPlatform.get() not_eq NULL) and (otwPlatform->IsAirplane()) and okToDoCockpitStuff)
    {
        AircraftClass *ac = static_cast<AircraftClass*>(otwPlatform.get());
        FackClass* mFaults = ac->mFaults;
        float tmpVal, tmpVal2;//TJL 01/14/04 multi-engine

        // Check for massive hardware failure
        if ( not (mFaults and 
              mFaults->GetFault(FaultClass::cadc_fault) and 
              mFaults->GetFault(FaultClass::ins_fault) and 
              mFaults->GetFault(FaultClass::gps_fault)))
        {
            cockpitFlightData.x = ac->XPos();
            cockpitFlightData.y = ac->YPos();
            cockpitFlightData.z = ac->ZPos();
            cockpitFlightData.xDot = ac->XDelta();
            cockpitFlightData.yDot = ac->YDelta();
            cockpitFlightData.zDot = ac->ZDelta();
            cockpitFlightData.alpha = ac->GetAlpha();
            cockpitFlightData.beta = ac->GetBeta();
            cockpitFlightData.gamma = ac->GetGamma();
            cockpitFlightData.pitch = ac->Pitch();
            cockpitFlightData.roll = ac->Roll();
            cockpitFlightData.yaw = ac->Yaw();
            cockpitFlightData.mach = ac->af->mach;
            cockpitFlightData.kias = ac->GetKias();
            cockpitFlightData.vt = ac->GetVt();
            cockpitFlightData.gs = ac->GetNz();

            // Correct for wind
            float yaw = cockpitFlightData.yaw;

            if (yaw < 0.0F)
            {
                yaw += 360.0F * DTR;
            }

            Tpoint posit;
            posit.x = ac->XPos();
            posit.y = ac->YPos();
            posit.z = ac->ZPos();

            // Find angle between heading and wind
            yaw = ((WeatherClass*)realWeather)->WindHeadingAt(&posit) - yaw;

            // Project wind speed

            yaw = ((WeatherClass*)realWeather)->WindSpeedInFeetPerSecond(&posit) * (float)sin(yaw);

            // Find angle
            yaw = (float)atan2(yaw, cockpitFlightData.vt);
            cockpitFlightData.windOffset = yaw;
        }

        // Add Engine data
        cockpitFlightData.fuelFlow = ac->af->FuelFlow();
        cockpitFlightData.gearPos = ac->af->gearPos;
        cockpitFlightData.speedBrake = ac->af->dbrake;
        cockpitFlightData.ftit = ac->af->rpm * 135.0F + 700.0F;
        cockpitFlightData.ftit2 = ac->af->rpm2 * 135.0F + 700.0F;//TJL 01/14/04 multi-engine
        cockpitFlightData.rpm = 100.0F * ac->af->rpm;
        cockpitFlightData.rpm2 = 100.0F * ac->af->rpm2;//TJL 01/14/04 Multi-engine
        cockpitFlightData.internalFuel = ac->af->Fuel();
        cockpitFlightData.externalFuel = ac->af->ExternalFuel();
        // MD -- 20031011: make sure all fuel values needed are updated even if we aren't looking at the gauge
        ac->af->GetFuel(&cockpitFlightData.fwd, &cockpitFlightData.aft, &cockpitFlightData.total);
        cockpitFlightData.epuFuel = ac->af->EPUFuel();
        tmpVal = ac->af->rpm;
        tmpVal2 = ac->af->rpm2;//TJL 01/14/04 multi-engine

        // Nozzle Position
        if (tmpVal <= 0.0F)
        {
            tmpVal = 100.0F;
        }
        else if (tmpVal <= 0.83F)
        {
            tmpVal = 100.0F + (0.0F - 100.0F) / (0.83F) * tmpVal;
        }
        else if (tmpVal <= 0.99)
        {
            tmpVal = 0.0F;
        }
        else if (tmpVal <= 1.03)
        {
            tmpVal = (100.0F) / (1.03F - 0.99F) * (tmpVal - 0.99F);
        }
        else
        {
            tmpVal = 100.0F;
        }

        //TJL 01/14/04 Multi-engine (just adding to the spaghetti code)
        if (tmpVal2 <= 0.0F)
        {
            tmpVal2 = 100.0F;
        }
        else if (tmpVal2 <= 0.83F)
        {
            tmpVal2 = 100.0F + (0.0F - 100.0F) / (0.83F) * tmpVal2;
        }
        else if (tmpVal2 <= 0.99)
        {
            tmpVal2 = 0.0F;
        }
        else if (tmpVal2 <= 1.03)
        {
            tmpVal2 = (100.0F) / (1.03F - 0.99F) * (tmpVal2 - 0.99F);
        }
        else
        {
            tmpVal2 = 100.0F;
        }

        cockpitFlightData.nozzlePos = tmpVal;
        cockpitFlightData.nozzlePos2 = tmpVal2;

        //MI extracting Data
        // get the chaff/flare count
        cockpitFlightData.ChaffCount =
            ac->counterMeasureStation[CHAFF_STATION].weaponCount;
        cockpitFlightData.FlareCount =
            ac->counterMeasureStation[FLARE_STATION].weaponCount;


        // get the DED lines
        if (pCockpitManager->mpIcp)
        {
            OTWDriver.pCockpitManager->mpIcp->Exec();
            OTWDriver.pCockpitManager->mpIcp->ExecPfl();

            if (mFaults and 
                ac->HasPower(AircraftClass::UFCPower) and 
 not mFaults->GetFault(FaultClass::ufc_fault)
               )
            {
                for (int j = 0; j < 5; j++)
                {
                    for (int i = 0; i < 26; i++)
                    {
                        cockpitFlightData.DEDLines[j][i] = pCockpitManager->mpIcp->DEDLines[j][i];
                        cockpitFlightData.Invert[j][i] = pCockpitManager->mpIcp->Invert[j][i];
                    }
                }

                for (int h = 0; h < 5; h++)
                {
                    for (int k = 0; k < 26; k++)
                    {
                        cockpitFlightData.PFLLines[h][k] = pCockpitManager->mpIcp->PFLLines[h][k];
                        cockpitFlightData.PFLInvert[h][k] = pCockpitManager->mpIcp->PFLInvert[h][k];
                    }
                }

                //and UFC Tacan channel
                if (gNavigationSys)
                {
                    cockpitFlightData.UFCTChan = gNavigationSys->GetTacanChannel(NavigationSystem::ICP);
                }
            }
        }

        //update version of shared memarea
        cockpitFlightData.VersionNum = 110;

        //AUX Tacan channel
        if (gNavigationSys)
        {
            cockpitFlightData.AUXTChan = gNavigationSys->GetTacanChannel(NavigationSystem::AUXCOMM);
        }

        // get the position of the three landing gears
        cockpitFlightData.NoseGearPos = ac->GetDOFValue(ComplexGearDOF[0]);
        cockpitFlightData.LeftGearPos = ac->GetDOFValue(ComplexGearDOF[1]);
        cockpitFlightData.RightGearPos = ac->GetDOFValue(ComplexGearDOF[2]);

        // ADI data
        cockpitFlightData.AdiIlsHorPos = pCockpitManager->ADIGpDevReading;
        cockpitFlightData.AdiIlsVerPos = pCockpitManager->ADIGsDevReading;

        if (
            otwPlatform.get() == NULL or
            otwPlatform->IsExploding() or
            otwPlatform->IsDead() or
 not otwPlatform->IsAwake() or
            TheHud->Ownship() == NULL
        )
        {
            okToDoCockpitStuff = FALSE;
        }

        // HSI States
        if (okToDoCockpitStuff and pCockpitManager->mpHsi and SimDriver.GetPlayerAircraft())
        {
            pCockpitManager->mpHsi->Exec();
        }

        cockpitFlightData.courseState = pCockpitManager->mpHsi->GetState(CPHsi::HSI_STA_CRS_STATE);
        cockpitFlightData.headingState = pCockpitManager->mpHsi->GetState(CPHsi::HSI_STA_HDG_STATE);
        cockpitFlightData.totalStates = pCockpitManager->mpHsi->GetState(CPHsi::HSI_STA_TOTAL_STATES);
        // HSI Values
        cockpitFlightData.courseDeviation = pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_CRS_DEVIATION);
        cockpitFlightData.desiredCourse = pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_CRS);
        cockpitFlightData.distanceToBeacon = pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DISTANCE_TO_BEACON);
        cockpitFlightData.bearingToBeacon = pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_BEARING_TO_BEACON);
        cockpitFlightData.currentHeading = pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_CURRENT_HEADING);
        cockpitFlightData.desiredHeading = pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_HEADING);
        cockpitFlightData.deviationLimit = pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DEV_LIMIT);
        cockpitFlightData.halfDeviationLimit = pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_HALF_DEV_LIMIT);
        cockpitFlightData.localizerCourse = pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_LOCALIZER_CRS);
        cockpitFlightData.airbaseX = pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_AIRBASE_X);
        cockpitFlightData.airbaseY = pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_AIRBASE_Y);
        cockpitFlightData.totalValues = pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_TOTAL_VALUES);

        //ATARIBABY looks like FlightData::ToTrue is bad
        //cannot hold all needed states: false,true an 2
        //so i use my own for 3d pit
        HSITOFROM3d = pCockpitManager->mpHsi->GetFlag(CPHsi::HSI_FLAG_TO_TRUE);

        // HSI Flags
        if (pCockpitManager->mpHsi->GetFlag(CPHsi::HSI_FLAG_TO_TRUE))
            cockpitFlightData.SetHsiBit(FlightData::ToTrue);
        else
            cockpitFlightData.ClearHsiBit(FlightData::ToTrue);

        if (pCockpitManager->mpHsi->GetFlag(CPHsi::HSI_FLAG_ILS_WARN))
            cockpitFlightData.SetHsiBit(FlightData::IlsWarning);
        else
            cockpitFlightData.ClearHsiBit(FlightData::IlsWarning);

        if (pCockpitManager->mpHsi->GetFlag(CPHsi::HSI_FLAG_CRS_WARN))
            cockpitFlightData.SetHsiBit(FlightData::CourseWarning);
        else
            cockpitFlightData.ClearHsiBit(FlightData::CourseWarning);

        if (pCockpitManager->mpHsi->GetFlag(CPHsi::HSI_FLAG_INIT))
            cockpitFlightData.SetHsiBit(FlightData::Init);
        else
            cockpitFlightData.ClearHsiBit(FlightData::Init);

        if (pCockpitManager->mpHsi->GetFlag(CPHsi::HSI_FLAG_TOTAL_FLAGS))
            cockpitFlightData.SetHsiBit(FlightData::TotalFlags);
        else
            cockpitFlightData.ClearHsiBit(FlightData::TotalFlags);

        // MD -- 20031011: Moved here to ensure the bits are set regardless of whether the player
        // is looking at the cockpit panels or not.
        if (SimDriver.GetPlayerAircraft()->INSState(AircraftClass::INS_ADI_OFF_IN))
            cockpitFlightData.ClearHsiBit(FlightData::ADI_OFF);
        else
            cockpitFlightData.SetHsiBit(FlightData::ADI_OFF);

        if (SimDriver.GetPlayerAircraft()->INSState(AircraftClass::INS_ADI_AUX_IN))
            cockpitFlightData.ClearHsiBit(FlightData::ADI_AUX);
        else
            cockpitFlightData.SetHsiBit(FlightData::ADI_AUX);

        if (SimDriver.GetPlayerAircraft()->INSState(AircraftClass::INS_HSI_OFF_IN))
            cockpitFlightData.ClearHsiBit(FlightData::HSI_OFF);
        else
            cockpitFlightData.SetHsiBit(FlightData::HSI_OFF);

        if (SimDriver.GetPlayerAircraft()->INSState(AircraftClass::BUP_ADI_OFF_IN))
            cockpitFlightData.ClearHsiBit(FlightData::BUP_ADI_OFF);
        else
            cockpitFlightData.SetHsiBit(FlightData::BUP_ADI_OFF);

        if (SimDriver.AVTROn())
            cockpitFlightData.SetHsiBit(FlightData::AVTR);
        else
            cockpitFlightData.ClearHsiBit(FlightData::AVTR);

        if (SimDriver.GetPlayerAircraft()->GSValid == FALSE or SimDriver.GetPlayerAircraft()->currentPower == AircraftClass::PowerNone)
            cockpitFlightData.SetHsiBit(FlightData::ADI_GS);
        else
            cockpitFlightData.ClearHsiBit(FlightData::ADI_GS);

        if (SimDriver.GetPlayerAircraft()->LOCValid == FALSE or SimDriver.GetPlayerAircraft()->currentPower == AircraftClass::PowerNone)
            cockpitFlightData.SetHsiBit(FlightData::ADI_LOC);
        else
            cockpitFlightData.ClearHsiBit(FlightData::ADI_LOC);

        if (SimDriver.GetPlayerAircraft()->currentPower < AircraftClass::PowerEmergencyBus)
        {
            cockpitFlightData.SetHsiBit(FlightData::VVI);
            cockpitFlightData.SetHsiBit(FlightData::AOA);
        }
        else
        {
            cockpitFlightData.ClearHsiBit(FlightData::VVI);
            cockpitFlightData.ClearHsiBit(FlightData::AOA);
        }

        // MD -- 2003: end of SetHsiBit() fixes

        // Oil Pressure
        tmpVal = ac->af->rpm;
        tmpVal2 = ac->af->rpm2;//TJL 01/14/04 multi-engine

        if (tmpVal < 0.7F)
        {
            //tmpVal = 40.0F;
            //ATARIBABY fix
            tmpVal = tmpVal * 40 / 0.7f;
        }
        else if (tmpVal <= 0.85)
        {
            tmpVal = 40.0F + (100.0F - 40.0F) / (0.85F - 0.7F) * (tmpVal - 0.7F);
        }
        else if (tmpVal <= 1.0)
        {
            tmpVal = 100.0F;
        }
        else if (tmpVal <= 1.03)
        {
            tmpVal = 100.0F + (103.0F - 100.0F) / (1.03F - 1.0F) * (tmpVal - 1.00F);
        }
        else
        {
            tmpVal = 103.0F;
        }

        //TJL 01/14/04 Multi-engine (stop the insanity)
        if (tmpVal2 < 0.7F)
        {
            //tmpVal2 = 40.0F;
            //ATARIBABY fix
            tmpVal2 = tmpVal2 * 40 / 0.7f;
        }
        else if (tmpVal2 <= 0.85)
        {
            tmpVal2 = 40.0F + (100.0F - 40.0F) / (0.85F - 0.7F) * (tmpVal2 - 0.7F);
        }
        else if (tmpVal2 <= 1.0)
        {
            tmpVal2 = 100.0F;
        }
        else if (tmpVal2 <= 1.03)
        {
            tmpVal2 = 100.0F + (103.0F - 100.0F) / (1.03F - 1.0F) * (tmpVal2 - 1.00F);
        }
        else
        {
            tmpVal2 = 103.0F;
        }

        //Trim values
        cockpitFlightData.TrimPitch = UserStickInputs.ptrim;
        cockpitFlightData.TrimRoll = UserStickInputs.rtrim;
        cockpitFlightData.TrimYaw = UserStickInputs.ytrim;

        cockpitFlightData.oilPressure = tmpVal;
        cockpitFlightData.oilPressure2 = tmpVal2;

        //if ((gSharedMemPtr) and ( not g_bEnableTrackIR)) // Retro 02/10/03
        if (gSharedMemPtr)
        {
            //if ( not mHelmetIsUR)
            // Retro 8Jan2004 - almost killed the shared mem for good, silly me :/
            if (( not mHelmetIsUR) and ( not g_bEnableTrackIR))
            {
                cockpitFlightData.headYaw = ((FlightData*)gSharedMemPtr)->headYaw;
                cockpitFlightData.headPitch = ((FlightData*)gSharedMemPtr)->headPitch;
                cockpitFlightData.headRoll = ((FlightData*)gSharedMemPtr)->headRoll;
            }

            memcpy(gSharedMemPtr, &cockpitFlightData, sizeof(FlightData));
        }
    }

    //STOP_PROFILE("RENDER 3DPIT");

    // Set up the camera based on the current view chosen
    if (endFlightTimer or flybyTimer)
    {
        SetFlybyCameraPosition(dT);
    }
    else if (eyeFly)
    {
        SetEyeFlyCameraPosition(dT);
    }
    else if (otwPlatform.get() not_eq NULL)
    {
        // If we _were_ in eyeFly and now aren't, clean up.
        // SCR:  Could this be better done once at mode change???
        if (eyeFlyTgt)
        {
            eyeFlyTgt->drawPointer->SetLabel("", 0x0);
            eyeFlyTgt = NULL;
        }

        // Select on of the "standard" views
        if (DisplayInCockpit())
        {
            SetInternalCameraPosition(dT);
        }
        else
        {
            SetExternalCameraPosition(dT);
        }
    }
    else
    {
        SetExternalCameraPosition(dT);
        // SetFlybyCameraPosition(dT);
    }



    // Compute the new viewpoint position
    ObserverPosition.x = viewPos.x = focusPoint.x + cameraPos.x;
    ObserverPosition.y = viewPos.y = focusPoint.y + cameraPos.y;
    ObserverPosition.z = viewPos.z = focusPoint.z + cameraPos.z;

    ObserverYaw = flyingEye->Yaw();
    ObserverPitch = flyingEye->Pitch();
    ObserverRoll = flyingEye->Roll();

    if ((otwPlatform.get() not_eq NULL) and /*otwPlatform->IsAirplane() and */ gSharedIntellivibe)
    {
        g_intellivibeData.eyex = viewPos.x;
        g_intellivibeData.eyey = viewPos.y;
        g_intellivibeData.eyez = viewPos.z;
        g_intellivibeData.IsFrozen = SimDriver.MotionOn() == 0;
        g_intellivibeData.IsPaused = targetCompressionRatio == 0;
        g_intellivibeData.IsFiringGun = otwPlatform->IsFiring() not_eq 0;
        g_intellivibeData.IsEjecting = otwPlatform->IsEject() not_eq 0;
        g_intellivibeData.IsOnGround = otwPlatform->OnGround() not_eq 0;

        if (otwPlatform->IsAirplane())
        {
            AircraftClass *air = static_cast<AircraftClass*>(otwPlatform.get());
            g_intellivibeData.Gforce = air->GetNz();
        }

        memcpy(gSharedIntellivibe, &g_intellivibeData, sizeof(g_intellivibeData));
    }

    // edg: this is a band aid solution for the action cam.  It seems that
    // something is causing cameraPos to have wild numbers -- perhaps an
    // uninitialed matrix or something somewhere.  Since the maximum range for
    // this camera is +/- 20000, in satellite mode, we'll clamp the values
    // here....
    // LRKLUDGE Keep the action camera happy. NOTE: We need to find out why it is going out into left field
    // edg: actually, there's no reason to restrict this to action camera...

    // if ( actionCameraMode )
    {
        if (_isnan(viewPos.x) or not _finite(viewPos.x))
        {
            ShiWarning("Bad action camera X pos: Why?");
            viewPos.x = 250000.0f;
        }

        if (_isnan(viewPos.y) or not _finite(viewPos.y))
        {
            ShiWarning("Bad action camera Y pos: Why?");
            viewPos.y = 250000.0f;
        }

        if (_isnan(viewPos.z) or not _finite(viewPos.z))
        {
            ShiWarning("Bad action camera Z pos: Why?");
            viewPos.z = -10000.0f;
        }
    }

    // now we want to tell the bubble rebuild where our camera is if we're
    // not attached to the player.  we do this by setting attaching and/or
    // setting the position of the camera entity.
    camCount = FalconLocalSession->CameraCount();

    if (otwPlatform.get() not_eq SimDriver.GetPlayerAircraft())
    {
        gOtwCameraLocation->SetPosition(viewPos.x, viewPos.y, viewPos.z);

        if (camCount <= 1)
        {
            FalconLocalSession->AttachCamera(gOtwCameraLocation);
        }
        else
        {
            for (i = 0; i < camCount; i++)
            {
                if (FalconLocalSession->GetCameraEntity(i) == gOtwCameraLocation)
                {
                    break;
                }
            }

            if (i == camCount)
            {
                // sfr: THIS IS EXTREMELY DANGEROUS CODE
                // remove 2nd camera (should be mav camera) and insert ourself
                //if ( not F4IsBadReadPtr(FalconLocalSession->GetCameraEntity(1), sizeof(VuEntity))) // JB 010318 CTD
                if (FalconLocalSession->CameraCount() > 1)
                {
                    FalconLocalSession->RemoveCamera(FalconLocalSession->GetCameraEntity(1));
                }

                FalconLocalSession->AttachCamera(gOtwCameraLocation);
            }
        }
    }
    else if (camCount > 1)
    {
        // sfr: cleanup camera mess
        FalconLocalSession->RemoveCamera(gOtwCameraLocation);
    }

    // Keep the viewpoint above ground
    GetAreaFloorAndCeiling(&bottom, &top);

    if (viewPos.z > top)
    {
        viewPos.z = min(viewPos.z, GetGroundLevel(viewPos.x, viewPos.y) - 5.0F);
    }

    // Now update everyone
    viewPoint->Update(&viewPos);
    //F4SoundFXSetCamPos( viewPos.x, viewPos.y, viewPos.z );
    // MLR 2003-10-17  10-27 returned to normal
    F4SoundFXSetCamPosAndOrient(&viewPos, &cameraRot, &cameraVel);

    TheTimeManager.SetTime(vuxGameTime + (unsigned long)FloatToInt32(todOffset * 1000.0F));
    ((WeatherClass*)realWeather)->UpdateWeather();

    BuildExternalNearList();


    // Set up the black out effects
    if (
        DisplayInCockpit() and doGLOC and 
        (otwPlatform.get() == SimDriver.GetPlayerAircraft()) and (otwPlatform.get() not_eq NULL)
    )
    {
        AircraftClass *ac = static_cast<AircraftClass*>(otwPlatform.get());
        float glocFactor = ac->glocFactor;

        if (glocFactor >= 0.0F)
        {
            renderer->SetTunnelPercent(glocFactor, 0);
        }
        else
        {
            renderer->SetTunnelPercent(-glocFactor, FloatToInt32(-255.0F * glocFactor));
        }
    }
    else if (endFlightTimer)
    {
        float pct = (float)(endFlightTimer - vuxRealTime) / 5000.0f;
        pct = 1.0f - pct;
        pct *= pct;

        if (vuxRealTime > endFlightTimer)
            pct = 1.0f;

        renderer->SetTunnelPercent(pct, 0);
    }
    else
    {
        renderer->SetTunnelPercent(0.0F, 0);
    }

    // COBRA - DX - Flip here, to make some work parallel to the GPU and increase fps
    if (g_intellivibeData.In3D)
    {
        OTWImage->SwapBuffers(false);
    }

    //if( not SkipSwap){ OTWImage->SwapBuffers(false); }
    SkipSwap = false;

    // RED - If the camera changed, preload the scene objects
    // if(CameraChange) renderer->PreLoadScene(NULL, NULL), CameraChange = false;

    //JAM 12Dec03
    if (DisplayOptions.bZBuffering)
    {
        renderer->context.SetZBuffering(TRUE);
    }

    // Actually draw the scene
    renderer->context.StartFrame();
    renderer->StartDraw();
    // Avoid any remaining from TV colors bitand Settings
    TheDXEngine.ResetState();

    // Select the right mode
    if (renderer->context.NVGmode) TheDXEngine.SetState(DX_NVG);

    if (renderer->context.TVmode) TheDXEngine.SetState(DX_TV);

    if (renderer->context.IRmode) TheDXEngine.SetState(DX_NVG);

    // * WARNING * PASSED IN THE DrawScene() function, more approriate for all calls
    // Clear any light list to be rebuilt
    //TheDXEngine.ClearLights();
    // Clear the stencil Buffer used for the Pits
    TheDXEngine.ClearStencil();
    // reset 2D Engine

    // * WARNING * PASSED IN THE DrawScene() function, more approriate for all calls
    //TheDXEngine.DX2D_Reset();

    // COBRA - RED - REDO FOR DX ENGINE //
    // Get the Display mode here for following checks
    OTWDisplayMode DisplayMode = GetOTWDisplayMode();

    // Now check if in the Pit and the platform is still valid to eventually draw the Pit...
    if (
 not DisplayInCockpit() or
        otwPlatform.get() == NULL or
        otwPlatform->IsExploding() or
        otwPlatform->IsDead() or
 not otwPlatform->IsAwake() or
        TheHud->Ownship() == NULL or
        eyeFly
    )
    {
        // If not valid, then signal it
        okToDoCockpitStuff = FALSE;

        // and switch to external view
        switch (DisplayMode)
        {
            case Mode2DCockpit:
            case ModePadlockF3:
            case Mode3DCockpit:
            case ModePadlockEFOV:
            case ModeHud:
                SetOTWDisplayMode(ModeOrbit);
                break;

            default:
                break;
        }
    }

    // START_PROFILE("RENDER 3DPIT");
    // COBRA - RED - OK, here critical session starts, the Objects MUST NOT BE TOUCHED IN ANY WAY
    // sfr: @todo taking crits out
#if NO_VU_LOCK
#else
    VuEnterCriticalSection();
#endif

    // If in 3D Pit, the pit has to be drawn as 1st item helping z-Buffering
    if (okToDoCockpitStuff)
    {
        // Calc Turubulence for the 2D pit stuff
        pCockpitManager->SetTurbulence();

        // only Hud Mode
        if (DisplayMode == ModeHud)
        {
            // just orient head
            VCock_HeadCalc();
        }

        if (DisplayMode == Mode3DCockpit or DisplayMode == ModePadlockF3)
        {
            // DX - Attach Weapons HERE, BEFORE any rendering
            CockAttachWeapons();
            VCock_HeadCalc();
            // RED - SIGNAL THIS IS PIT STUFF...
            TheDXEngine.SetPitMode(true);
            // Draw the Pit
            VCock_DrawThePit();

            //ShiAssert(SimDriver.GetPlayerAircraft() == otwPlatform); //588
            //VCock_Exec(); //588

            // RED - END OF PIT STUFF....
            TheDXEngine.SetPitMode(false);
        }

        if (DisplayMode == Mode2DCockpit)
        {
            ShiAssert(SimDriver.GetPlayerAircraft() == otwPlatform);
            // DX - Attach Weapons HERE, BEFORE any rendering
            pCockpitManager->CockAttachWeapons();
            pCockpitManager->GeometryDraw();
        }
    }

    // Set the font here for labels
    oldFont = VirtualDisplay::CurFont();
    VirtualDisplay::SetFont(pCockpitManager->LabelFont());
    //STOP_PROFILE("RENDER 3DPIT");

    //START_PROFILE("RENDER DRAWSCENE");
    renderer->DrawScene((struct Tpoint *) &headOrigin, (struct Trotation *) &cameraRot);
    //STOP_PROFILE("RENDER DRAWSCENE");

    VirtualDisplay::SetFont(oldFont);

    //Now if in the pit the instumentation
    if (okToDoCockpitStuff)
    {
        // Flush the polys before rendering instruments
        if (DisplayOptions.bZBuffering)
        {
            // If camera changed, wait for updates
            if (CameraChange)
            {
                // Ok, camera changed, a tme out to setup fast loading
                BigLoadTimeOut = GetTickCount() + BIG_LOAD_TIMEOUT;
                //ObjectLOD::WaitUpdates();
                //TheTextureBank.WaitUpdates();
                CameraChange = false;
            }

            renderer->context.FlushPolyLists();
            renderer->ClearZBuffer();
        }

        if (DisplayMode  == ModePadlockF3)
        {
            ShiAssert(SimDriver.GetPlayerAircraft() == otwPlatform);
            Padlock_DrawSquares(TRUE);
            VCock_Exec();
        }

        if (DisplayMode  == Mode3DCockpit) //588
        {
            ShiAssert(SimDriver.GetPlayerAircraft() == otwPlatform);
            //START_PROFILE("VCOCK EXEC");
            VCock_Exec();
            //STOP_PROFILE("VCOCK EXEC");
        } //588

        if (DisplayMode  == ModePadlockEFOV)
        {
            ShiAssert(SimDriver.GetPlayerAircraft() == otwPlatform);
            Padlock_DrawSquares(TRUE);
        }
    }
    else
    {
        if ((otwPlatform.get() not_eq NULL) and otwPlatform->drawPointer)
        {
            if ( not otwPlatform->OnGround()) DrawExternalViewTarget();
        }

        if (g_bLensFlare) Draw2DLensFlare(renderer);
    }

    // If camera changed, wait for updates
    if (CameraChange)
    {
        // Ok, camera changed, a tme out to setup fast loading
        BigLoadTimeOut = GetTickCount() + BIG_LOAD_TIMEOUT;
        //ObjectLOD::WaitUpdates();
        //TheTextureBank.WaitUpdates();
        CameraChange = false;
    }


    if (BigLoadTimeOut > GetTickCount())
    {
        ObjectLOD::SetRatedLoad(false);
        TheTextureBank.SetRatedLoad(false);
    }
    else
    {
        ObjectLOD::SetRatedLoad(true);
        TheTextureBank.SetRatedLoad(true);
    }

    // Now flush anything coming from the Pits... they use a own camera
    if (DisplayOptions.bZBuffering)
    {
        renderer->context.FlushPolyLists();
    }

    // If in 3D Pit, the pit has to be drawn as 1st item helping z-Buffering
    if (okToDoCockpitStuff)
    {
        // DX - Detach Weapons HERE, AFTER any rendering
        if (DisplayMode == Mode3DCockpit or DisplayMode == ModePadlockF3) CockDetachWeapons();

        if (DisplayMode == Mode2DCockpit) pCockpitManager->CockDetachWeapons();
    }

    // COBRA - RED - REDO FOR DX ENGINE - END
    // sfr: @todo taking crits out
#if NO_VU_LOCK
#else
    VuExitCriticalSection();
#endif

    // Clear out the "near" list now that we're done drawing it
    FlushNearList();

    //START_PROFILE("RENDER 2DPIT");
    // Do the first layer of drawn cockpit stuff (pre BLT)
    if (okToDoCockpitStuff)
    {
        // Should we Draw the HUD?
        //START_PROFILE("HUD");
        if (pCockpitManager->ShowHud())
        {
            if (GetOTWDisplayMode() == ModePadlockEFOV or
                GetOTWDisplayMode() == ModeHud or
                GetOTWDisplayMode() == Mode2DCockpit
               )
            {
                Draw2DHud();
            }
        }

        //STOP_PROFILE("HUD");
        //START_PROFILE("PADLOCK");

        // Should we draw the EFOV window?
        SimMoverClass *mover = static_cast<SimMoverClass*>(otwPlatform.get());

        if ((GetOTWDisplayMode() == ModePadlockEFOV) and (mover->targetList))
        {
            // SCR: Was a condition above, but in EFOV view it MUST be the player, right?
            ShiAssert(otwPlatform->IsLocal());

            Padlock_CheckPadlock(dT);
            PadlockEFOV_Draw();
        }

        if (GetOTWDisplayMode() == ModePadlockF3 or GetOTWDisplayMode() == Mode3DCockpit)
        {
            if (mDoSidebar)
            {
                PadlockF3_Draw();
            }
        }

        //STOP_PROFILE("PADLOCK");

        // mfds and rwr
        if (GetOTWDisplayMode() == Mode2DCockpit)
        {
            renderer->EndDraw();

            //START_PROFILE("COCKPIT MANAGER EXEC");
            pCockpitManager->Exec();
            //STOP_PROFILE("COCKPIT MANAGER EXEC");

            renderer->StartDraw();
            float top, left, bottom, right;
            renderer->GetViewport(&left, &top, &right, &bottom); // save the current viewport
            renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F); // set fullscreen viewport
            pCockpitManager->DisplayBlit3D(); // draw 3d stuff
            renderer->EndDraw();
            renderer->SetViewport(left, top, right, bottom); // restore viewport

            // Draw in the 2D cockpit
            // sfr: it seems we need this for the map only... the rest is all 3d
            pCockpitManager->DisplayBlit();
            pCockpitManager->DisplayDraw();

            // mfds and RWR
            PlayerRwrClass* theRwr;
            AircraftClass *playerAircraft = (AircraftClass *)SimDriver.GetPlayerAircraft();

            if (playerAircraft)
            {
                // rwr
                //START_PROFILE("RWR");
                theRwr = (PlayerRwrClass*)FindSensor(playerAircraft, SensorClass::RWR);

                if (pCockpitManager->ShowRwr() and theRwr)
                {
                    if ( not gDoCockpitHack)
                    {
                        renderer->StartDraw();
                        pCockpitManager->GetViewportBounds(&viewportBounds, BOUNDS_RWR);
                        // COBRA - RED- Pit Vibrations
                        pCockpitManager->AddTurbulenceVp(&viewportBounds);
                        renderer->SetColor(0xFF00FF00);
                        renderer->SetViewport(
                            viewportBounds.left, viewportBounds.top, viewportBounds.right, viewportBounds.bottom
                        );
                        theRwr->SetGridVisible(FALSE);
                        theRwr->Display(renderer);
                        renderer->EndDraw();
                    }
                }

                //STOP_PROFILE("RWR");

                // mfds
                //START_PROFILE("MFD");
                oldFont = VirtualDisplay::CurFont();
                VirtualDisplay::SetFont(pCockpitManager->MFDFont());

                for (unsigned int i = BOUNDS_MFDLEFT; i <= BOUNDS_MFD4; ++i)
                {
                    if (pCockpitManager->GetViewportBounds(&viewportBounds, i))
                    {
                        // COBRA - RED- Pit Vibrations
                        unsigned int mfd = i - BOUNDS_MFDLEFT;
                        pCockpitManager->AddTurbulenceVp(&viewportBounds);
                        MfdDisplay[mfd]->SetImageBuffer(
                            OTWImage, viewportBounds.left, viewportBounds.top,
                            viewportBounds.right, viewportBounds.bottom
                        );
                        MfdDisplay[mfd]->Exec(FALSE, FALSE);
                    }
                }

                VirtualDisplay::SetFont(oldFont);
                //STOP_PROFILE("MFD");
            }
        }
        else if (
            (GetOTWDisplayMode() == ModeHud or GetOTWDisplayMode() == ModePadlockEFOV) and 
 not g_bNoMFDsIn1View) //MI added g_bNoMFDsIn1View check. Removes MFD's if TRUE
        {
            // SetFont
            oldFont = VirtualDisplay::CurFont();
            VirtualDisplay::SetFont(pCockpitManager->MFDFont());
            renderer->EndDraw();

            for (i = 0; i < NUM_MFDS; i++)
            {
                MfdDisplay[i]->Exec(TRUE, FALSE);
            }

            VirtualDisplay::SetFont(oldFont);
        }
        else if (GetOTWDisplayMode() == ModePadlockF3 or GetOTWDisplayMode() == Mode3DCockpit)
        {
            if (mDoSidebar and pPadlockCPManager)
            {
                // Draw in the 2D reference panels
                pPadlockCPManager->Exec();

                // OW
                float top, left, bottom, right;
                renderer->GetViewport(&left, &top, &right, &bottom); // save the current viewport
                renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F); // set fullscreen viewport
                pPadlockCPManager->DisplayBlit3D(); // draw 3d stuff
                renderer->EndDraw();
                renderer->SetViewport(left, top, right, bottom); // restore viewport

                // Draw in the 2D cockpit panels
                pPadlockCPManager->DisplayBlit();
                pPadlockCPManager->DisplayDraw();
            }
            else
            {
                renderer->EndDraw();
            }
        }
    }
    else
    {
        renderer->EndDraw();
    }

    //STOP_PROFILE("RENDER 2DPIT");


    // COBRA - RED - FOR NO-DEPTH MENUS, CLEAR THE ZBUFFER
    renderer->context.ClearBuffers(MPR_CI_ZBUFFER);

    // Draw wingman, tanker, awacs menus
    pMenuManager->DisplayDraw();


    //STOP_PROFILE("RENDER FRAME");
    // Draws ANY text in front of everything else
    // the Chat box at least MUST show up AFTER the cockpit is drawn

    //STOP_PROFILE("OTWRENDER");
    //STOP_PROFILE("OTW Loop");
    DisplayFrontText();
    /*MAIN_PROFILE("OTW Loop");*/
    //START_PROFILE("OTWRENDER");

    // Draw GLOC effect
    if (
        doGLOC and (otwPlatform.get() not_eq NULL) and 
        (otwPlatform.get() == SimDriver.GetPlayerAircraft()) and otwPlatform->IsLocal()
    )
    {
        renderer->DrawTunnelBorder();
    }
    else if (TheTimeOfDay.GetNVGmode() or endFlightTimer)
    {
        renderer->DrawTunnelBorder();
    }

    //JAM 18Nov03
    if (weatherCondition == INCLEMENT and cameraPos.z > realWeather->stratusZ)
    {
        if (DisplayInCockpit())
        {
            F4SoundFXSetDist(SFX_RAININT, FALSE, 10.f, 2.f);
        }
        else
        {
            F4SoundFXSetDist(SFX_RAINEXT, FALSE, 10.f, 2.f);
        }
    }

    // Show the exit menu if needed
    DrawExitMenu();

    if (takeScreenShot)
    {
        TakeScreenShot();

        if (pMenuManager)
        {
            pMenuManager->DeActivate();
        }
    }

    // Finish and swap the buffers
    // Force cursors if in exit menu

    if (InExitMenu())
    {
        int tmp = gSelectedCursor;
        gSelectedCursor = 1;
        ClipAndDrawCursor(
            OTWDriver.pCockpitManager->GetCockpitWidth(), OTWDriver.pCockpitManager->GetCockpitHeight()
        );
        gSelectedCursor = tmp;
    }
    // Wombat778 1-23-04 Changed from gTimeLastMouseMove to gTimeLastCursorUpdate because
    // gTimeLastMouseMove reports ALL changes in mouse movement, not just cursor updates.
    else if (
        gSimInputEnabled and 
        SimDriver.GetPlayerAircraft() and 
        vuxRealTime - /*gTimeLastMouseMove*/gTimeLastCursorUpdate < SI_MOUSE_TIME_DELTA
    )
    {
        //Wombat778 Draw the cursor in the 3d pit as well
        if (
            (GetOTWDisplayMode() == Mode2DCockpit or
             GetOTWDisplayMode() == Mode3DCockpit or
             GetOTWDisplayMode() == ModePadlockF3 or
             GetOTWDisplayMode() == ModePadlockEFOV
            ) and 
            (gSelectedCursor >= 0) and (otwPlatform.get() == SimDriver.GetPlayerAircraft())
        )
        {
            ClipAndDrawCursor(
                OTWDriver.pCockpitManager->GetCockpitWidth(), OTWDriver.pCockpitManager->GetCockpitHeight()
            );
        }

    }

    // DX - Finally finish the HW frame
    renderer->context.FinishFrame(NULL);
    count++;
    //STOP_PROFILE("OTWRENDER");
}

//JAM 27Dec03 - Bookmark

//Wombat778 11-17-04 helper to run new 2d trackir code

void OTWDriverClass::RunNew2DTrackIR(float pan, float tilt)
{

    static float lastpan = pan;
    static float lasttilt = tilt;

    //the 10.0f values are arbitrary, but seems to produce the right amount of jitter reduction while still being sensitive

    if (fabs(pan - lastpan) > 10.0f * DTR or fabs(tilt - lasttilt) > 10.0f * DTR)
    {
        lastpan = pan;
        lasttilt = tilt;
        pCockpitManager->Set2DPanelDirection(pan, tilt);
    }
}

void OTWDriverClass::SetInternalCameraPosition(float dT)
{
    Prof(SetInternalCameraPosition); // Retro 15/10/03

    // Display is 1st person, cockpit type....
    ShiAssert(DisplayInCockpit());


    UpdateCameraFocus();

    // courtesy of the code that calls this function otwPlatform is always valid
    cameraVel.x = otwPlatform->XDelta(); // MLR 12/9/2003 -
    cameraVel.y = otwPlatform->YDelta();
    cameraVel.z = otwPlatform->ZDelta();

#if 1  // MLR 12/1/2003 - Eye position is controlled by pilotEyePos, which is gotten from the players AC
    MatrixMult(&ownshipRot, &pilotEyePos, &cameraPos);
#else
    // OLD EYE PLACEMENT

    // These two constants are based on rough ruler measurements
    // from a schematic of the F16.
    // TODO:  Get these from the object some how.
    static const float EyeFromCGfwd = 15.0f;
    static const float EyeFromCGup =  3.0f;

    // Move the camera position from the CG to the cockpit
    cameraPos.x = EyeFromCGfwd * ownshipRot.M11 - EyeFromCGup * ownshipRot.M13;
    cameraPos.y = EyeFromCGfwd * ownshipRot.M21 - EyeFromCGup * ownshipRot.M23;
    cameraPos.z = EyeFromCGfwd * ownshipRot.M31 - EyeFromCGup * ownshipRot.M33;
#endif

    // Adjust for head angle (if enabled)
    if (GetOTWDisplayMode() == Mode2DCockpit)
    {
        if ((g_bEnableTrackIR) and (PlayerOptions.Get2dTrackIR() == true) and not GetHybridPitMode()) // Retro 27/09/03  //Wombat778 11-18-04 Dont run the 2d trackir code in hybrid mode
        {
#ifdef DEBUG_TRACKIR_STUFF
            FILE* fp = fopen("TIR_Debug_2.txt", "at");
            fprintf(fp, "%x", vuxRealTime);

            if (vuxRealTime bitand g_nTrackIRSampleFreq)
            {
                fprintf(fp, " - chk");

                if (theTrackIRObject.Get_Panning_Allowed())
                    fprintf(fp, " - allowed");
            }

            fprintf(fp, "\n");
            fclose(fp);
#endif

            /* sample time is user-configurable */
            if (g_bNew2DTrackIR) //Wombat778 11-15-04 New method of 2d Pit trackir movement
                RunNew2DTrackIR(cockpitFlightData.headYaw, cockpitFlightData.headPitch);

            else if (vuxRealTime bitand g_nTrackIRSampleFreq) // Retro 26/09/03 - check every 512 ms (default value)
            {
                if (theTrackIRObject.Get_Panning_Allowed())
                    SimDriver.POVKludgeFunction(theTrackIRObject.TrackIR_2D_Map()); // Retro 26/09/03
            }
            else
            {
                theTrackIRObject.Allow_2D_Panning();
            }
        }

        // Update the head matrix
        BuildHeadMatrix(FALSE, YAW_PITCH, eyePan, eyeTilt, 0.0F);
        // Combine the head and airplane matrices
        MatrixMult(&ownshipRot, &headMatrix, &cameraRot);

    }
    else if (GetOTWDisplayMode() == Mode3DCockpit)
    {
        VCock_GiveGilmanHead(dT);
    }
    else if (GetOTWDisplayMode() == ModePadlockF3)
    {
        // 2000-11-12 MODIFIED BY S.G. SO PANNING BREAKS THE PADLOCK
        // Padlock_CheckPadlock(dT);
        // PadlockF3_CalcCamera(dT);
        if (g_nPadlockMode bitand PLockModeBreakLock)
        {
            if (azDir == 0.0F and elDir == 0.0F)
            {
                Padlock_CheckPadlock(dT);
                PadlockF3_CalcCamera(dT);
            }
            else
            {
                float oldAzDir = azDir, oldElDir = elDir; // 2002-03-12 ADDED BY S.G. Lets remember these so I can set them back after
                float tmpEyePan  = eyePan;
                float tmpEyeTilt = eyeTilt;
                float tmpEyeHeadRoll = eyeHeadRoll;
                mIsSlewInit = FALSE;
                SetOTWDisplayMode(OTWDriverClass::Mode3DCockpit);
                eyePan  = tmpEyePan;
                eyeTilt = tmpEyeTilt;
                eyeHeadRoll = tmpEyeHeadRoll;
                azDir = oldAzDir; // 2002-03-12 ADDED BY S.G. Set them back to the saved value so head starts moving right away without requiring the key to let go and pushed again...
                elDir = oldElDir;
            }
        }
        else
        {
            Padlock_CheckPadlock(dT);
            PadlockF3_CalcCamera(dT);
        }

        // END OF MODIFIED SECTION
    }
    else
    {
        memcpy(&cameraRot, &ownshipRot, sizeof(Trotation));
    }
}

#ifdef MEM_DEBUG

#include "guns.h"
#include "simfeat.h"
#include "airframe.h"
#include "ground.h"
#include "gndai.h"
#include "missile.h"
#include "bomb.h"
#include "battalion.h"
#include "brigade.h"
#include "navunit.h"
#include "objectiv.h"
#include "package.h"
#include "squadron.h"
#include "flight.h"
#include "persist.h"
#include "helo.h"
#include "hdigi.h"
#include "atcbrain.h"
#include "limiters.h"
#include "simvudrv.h"
#include "listadt.h"
#include "falcsnd/voicemanager.h"
#include "objects/drawtrcr.h"
#include "objects/drawbldg.h"
#include "objects/drawbsp.h"
#include "objects/drawbrdg.h"
#include "objects/drawovc.h"
#include "objects/drawplat.h"
#include "objects/drawrdbd.h"
#include "objects/drawsgmt.h"
#include "objects/drawpuff.h"
#include "objects/drawshdw.h"
#include "objects/drawpnt.h"
#include "objects/drawguys.h"
#include "objects/drawpole.h"
#include "bsplib/objectlod.h"
#include "terrain/tblock.h"
#include "division.h"
#include "evtparse.h"

// edg: temp comment out

extern MEM_POOL glMemPool;
extern MEM_POOL graphicsDOFDataPool;
extern MEM_POOL vuRBNodepool;
extern MEM_POOL gDivVUIDs;
extern MEM_POOL gTextMemPool;
extern MEM_POOL gTexDBMemPool;
extern MEM_POOL gTPostMemPool;
extern MEM_POOL gFartexMemPool;
extern MEM_POOL gBSPLibMemPool;
extern MEM_POOL gObjMemPool;
extern MEM_POOL gReadInMemPool;
extern MEM_POOL gSoundMemPool;
extern MEM_POOL gTacanMemPool;
extern MEM_POOL gVuFilterMemPool;
extern MEM_POOL gInputMemPool;
extern "C" MEM_POOL gResmgrMemPool;

extern int ObjectNodes, ObjectReferences;

void DebugMemoryReport(RenderOTW *renderer, int frameTime)
{
    float row, col;
    int objCount = 0;
    int totCount = 0;
    int totSize = 0;

    sprintf(tmpStr, "%.2f SFX=%d Proc Objs=%d Draw Objs=%d CT=%d MAX=%d ST=%d MAX=%d GT=%d MAX=%d AVESGT=%d AVECT=%d MemC=%d MemS=%d",
            1.0F / (float)(frameTime) * 1000.0F,
            gTotSfx,
            numObjsProcessed,
            numObjsInDrawList,
            gCampTime,
            gCampTimeMax,
            gSimTime,
            gSimTimeMax,
            gGraphicsTimeLast,
            gGraphicsTimeLastMax,
            gAveSimGraphicsTime,
            gAveCampTime,
            dbgMemTotalCount(),
            dbgMemTotalSize());
    renderer->TextLeft(-0.95F,  0.95F, tmpStr);

    col = -0.95f;
    row =  0.90f;

    // DEFAULT POOL

    sprintf(tmpStr, "DEFAULT POOL C=%d S=%d",
            MemPoolCount(MemDefaultPool),
            MemPoolSize(MemDefaultPool));

    renderer->TextLeft(col, row, tmpStr);

    row -= 0.05f;

    // Campaign Stuff

    sprintf(tmpStr, "C Obj C=%d S=%d",
            MemPoolCount(ObjectiveClass::pool),
            MemPoolSize(ObjectiveClass::pool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "C Bat C=%d S=%d",
            MemPoolCount(BattalionClass::pool),
            MemPoolSize(BattalionClass::pool));

    renderer->TextLeft(col + 0.6F, row, tmpStr);

    sprintf(tmpStr, "C Bri C=%d S=%d",
            MemPoolCount(BrigadeClass::pool),
            MemPoolSize(BrigadeClass::pool));

    renderer->TextLeft(col + 1.2F, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "C Fli C=%d S=%d",
            MemPoolCount(FlightClass::pool),
            MemPoolSize(FlightClass::pool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "C Squ C=%d S=%d",
            MemPoolCount(SquadronClass::pool),
            MemPoolSize(SquadronClass::pool));

    renderer->TextLeft(col + 0.6F, row, tmpStr);

    sprintf(tmpStr, "C Pak C=%d S=%d",
            MemPoolCount(PackageClass::pool),
            MemPoolSize(PackageClass::pool));

    renderer->TextLeft(col + 1.2F, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "C Tsk C=%d S=%d",
            MemPoolCount(TaskForceClass::pool),
            MemPoolSize(TaskForceClass::pool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "C Per C=%d S=%d",
            MemPoolCount(SimPersistantClass::pool),
            MemPoolSize(SimPersistantClass::pool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);


    row -= 0.05f;

    // Drawables

    sprintf(tmpStr, "D 2D C=%d S=%d",
            MemPoolCount(Drawable2D::pool),
            MemPoolSize(Drawable2D::pool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "D Tcr C=%d S=%d",
            MemPoolCount(DrawableTracer::pool),
            MemPoolSize(DrawableTracer::pool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "D Gvh C=%d S=%d",
            MemPoolCount(DrawableGroundVehicle::pool),
            MemPoolSize(DrawableGroundVehicle::pool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "D bld C=%d S=%d",
            MemPoolCount(DrawableBuilding::pool),
            MemPoolSize(DrawableBuilding::pool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "D BSP C=%d S=%d",
            MemPoolCount(DrawableBSP::pool),
            MemPoolSize(DrawableBSP::pool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "D sha C=%d S=%d",
            MemPoolCount(DrawableShadowed::pool),
            MemPoolSize(DrawableShadowed::pool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "D brg C=%d S=%d",
            MemPoolCount(DrawableBridge::pool),
            MemPoolSize(DrawableBridge::pool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "D ovc C=%d S=%d",
            MemPoolCount(DrawableOvercast::pool),
            MemPoolSize(DrawableOvercast::pool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "D pla C=%d S=%d",
            MemPoolCount(DrawablePlatform::pool),
            MemPoolSize(DrawablePlatform::pool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "D rdb C=%d S=%d",
            MemPoolCount(DrawableRoadbed::pool),
            MemPoolSize(DrawableRoadbed::pool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "D seg C=%d S=%d",
            MemPoolCount(DrawableTrail::pool),
            MemPoolSize(DrawableTrail::pool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "D TrE C=%d S=%d",
            MemPoolCount(TrailElement::pool),
            MemPoolSize(TrailElement::pool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "D puf C=%d S=%d",
            MemPoolCount(DrawablePuff::pool),
            MemPoolSize(DrawablePuff::pool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "D pnt C=%d S=%d",
            MemPoolCount(DrawablePoint::pool),
            MemPoolSize(DrawablePoint::pool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "D guy C=%d S=%d",
            MemPoolCount(DrawableGuys::pool),
            MemPoolSize(DrawableGuys::pool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "T blk C=%d S=%d",
            MemPoolCount(TBlock::pool),
            MemPoolSize(TBlock::pool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "T LstE C=%d S=%d",
            MemPoolCount(TListEntry::pool),
            MemPoolSize(TListEntry::pool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "T BlkL C=%d S=%d",
            MemPoolCount(TBlockList::pool),
            MemPoolSize(TBlockList::pool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "G OLOD C=%d S=%d",
            MemPoolCount(ObjectLOD::pool),
            MemPoolSize(ObjectLOD::pool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "G 3DLib C=%d S=%d",
            MemPoolCount(glMemPool),
            MemPoolSize(glMemPool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "S List C=%d S=%d",
            MemPoolCount(displayList::pool),
            MemPoolSize(displayList::pool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "S SfxR C=%d S=%d",
            MemPoolCount(sfxRequest::pool),
            MemPoolSize(sfxRequest::pool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "S AirF C=%d S=%d",
            MemPoolCount(AirframeClass::pool),
            MemPoolSize(AirframeClass::pool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "S AirC C=%d S=%d",
            MemPoolCount(AircraftClass::pool),
            MemPoolSize(AircraftClass::pool));

    objCount += MemPoolCount(AircraftClass::pool);

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "S Miss C=%d S=%d",
            MemPoolCount(MissileClass::pool),
            MemPoolSize(MissileClass::pool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "S Bomb C=%d S=%d",
            MemPoolCount(BombClass::pool),
            MemPoolSize(BombClass::pool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "S SObj C=%d S=%d",
            MemPoolCount(SimObjectType::pool),
            MemPoolSize(SimObjectType::pool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "S Ldat C=%d S=%d",
            MemPoolCount(SimObjectLocalData::pool),
            MemPoolSize(SimObjectLocalData::pool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "S Sfx C=%d S=%d",
            MemPoolCount(SfxClass::pool),
            MemPoolSize(SfxClass::pool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "S Gun C=%d S=%d",
            MemPoolCount(GunClass::pool),
            MemPoolSize(GunClass::pool));

    renderer->TextLeft(col, row, tmpStr);

    objCount += MemPoolCount(GroundClass::pool);

    sprintf(tmpStr, "S Grnd C=%d S=%d",
            MemPoolCount(GroundClass::pool),
            MemPoolSize(GroundClass::pool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "S GAI C=%d S=%d",
            MemPoolCount(GNDAIClass::pool),
            MemPoolSize(GNDAIClass::pool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;

    objCount += MemPoolCount(SimFeatureClass::pool);

    sprintf(tmpStr, "S Feat C=%d S=%d",
            MemPoolCount(SimFeatureClass::pool),
            MemPoolSize(SimFeatureClass::pool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "S MIFD C=%d S=%d",
            MemPoolCount(MissileInFlightData::pool),
            MemPoolSize(MissileInFlightData::pool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "S Dlst C=%d S=%d",
            MemPoolCount(drawPtrList::pool),
            MemPoolSize(drawPtrList::pool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "Waypnt C=%d S=%d",
            MemPoolCount(WayPointClass::pool),
            MemPoolSize(WayPointClass::pool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "SMSBase C=%d S=%d",
            MemPoolCount(SMSBaseClass::pool),
            MemPoolSize(SMSBaseClass::pool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "SMS C=%d S=%d",
            MemPoolCount(SMSClass::pool),
            MemPoolSize(SMSClass::pool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "BWeapn C=%d S=%d",
            MemPoolCount(BasicWeaponStation::pool),
            MemPoolSize(BasicWeaponStation::pool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "AWeapn C=%d S=%d",
            MemPoolCount(AdvancedWeaponStation::pool),
            MemPoolSize(AdvancedWeaponStation::pool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "Helos C=%d S=%d",
            MemPoolCount(HelicopterClass::pool),
            MemPoolSize(HelicopterClass::pool));

    objCount += MemPoolCount(HelicopterClass::pool);

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "VM CONV C=%d S=%d",
            MemPoolCount(CONVERSATION::pool),
            MemPoolSize(CONVERSATION::pool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "VM BUFF C=%d S=%d",
            MemPoolCount(VM_BUFFLIST::pool),
            MemPoolSize(VM_BUFFLIST::pool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "VM CONV C=%d S=%d",
            MemPoolCount(VM_CONVLIST::pool),
            MemPoolSize(VM_CONVLIST::pool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "RWY Que C=%d S=%d",
            MemPoolCount(runwayQueueStruct::pool),
            MemPoolSize(runwayQueueStruct::pool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "H BRAIN C=%d S=%d",
            MemPoolCount(HeliBrain::pool),
            MemPoolSize(HeliBrain::pool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "AF DATA C=%d S=%d",
            MemPoolCount(AirframeDataPool),
            MemPoolSize(AirframeDataPool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "3P Lim C=%d S=%d",
            MemPoolCount(ThreePointLimiter::pool),
            MemPoolSize(ThreePointLimiter::pool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "Val Lim C=%d S=%d",
            MemPoolCount(ValueLimiter::pool),
            MemPoolSize(ValueLimiter::pool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "Pct Lim C=%d S=%d",
            MemPoolCount(PercentLimiter::pool),
            MemPoolSize(PercentLimiter::pool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "Line Lim C=%d S=%d",
            MemPoolCount(LineLimiter::pool),
            MemPoolSize(LineLimiter::pool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "Vu Drv C=%d S=%d",
            MemPoolCount(SimVuDriver::pool),
            MemPoolSize(SimVuDriver::pool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "Vu Slave C=%d S=%d",
            MemPoolCount(SimVuSlave::pool),
            MemPoolSize(SimVuSlave::pool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "Obj Geom C=%d S=%d",
            MemPoolCount(ObjectGeometry::pool),
            MemPoolSize(ObjectGeometry::pool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "List Class C=%d S=%d",
            MemPoolCount(ListClass::pool),
            MemPoolSize(ListClass::pool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "List Elem C=%d S=%d",
            MemPoolCount(ListElementClass::pool),
            MemPoolSize(ListElementClass::pool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;


    sprintf(tmpStr, "VuLinkNode C=%d S=%d",
            MemPoolCount(VuLinkNode::pool),
            MemPoolSize(VuLinkNode::pool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "VuRBNode C=%d S=%d",
            MemPoolCount(vuRBNodepool),
            MemPoolSize(vuRBNodepool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "DOF Data C=%d S=%d",
            MemPoolCount(graphicsDOFDataPool),
            MemPoolSize(graphicsDOFDataPool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;


    // sprintf( tmpStr, "Loadout C=%d S=%d",
    //  MemPoolCount( LoadoutStruct::pool ),
    //  MemPoolSize( LoadoutStruct::pool ) );

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "Vu Message C=%d S=%d",
            MemPoolCount(gVuMsgMemPool),
            MemPoolSize(gVuMsgMemPool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "Unit Deagg C=%d S=%d",
            MemPoolCount(UnitDeaggregationData::pool),
            MemPoolSize(UnitDeaggregationData::pool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "Division C=%d S=%d",
            MemPoolCount(DivisionClass::pool),
            MemPoolSize(DivisionClass::pool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "Miss Req C=%d S=%d",
            MemPoolCount(MissionRequestClass::pool),
            MemPoolSize(MissionRequestClass::pool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "Event El C=%d S=%d",
            MemPoolCount(EventElement::pool),
            MemPoolSize(EventElement::pool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;

    if (gDivVUIDs)
    {
        sprintf(tmpStr, "Div VUIDs C=%d S=%d",
                MemPoolCount(gDivVUIDs),
                MemPoolSize(gDivVUIDs));

        renderer->TextLeft(col, row, tmpStr);
    }

    sprintf(tmpStr, "Faults C=%d S=%d",
            MemPoolCount(gFaultMemPool),
            MemPoolSize(gFaultMemPool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "Text C=%d S=%d",
            MemPoolCount(gTextMemPool),
            MemPoolSize(gTextMemPool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "TPosts C=%d S=%d",
            MemPoolCount(gTPostMemPool),
            MemPoolSize(gTPostMemPool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "FarTex C=%d S=%d",
            MemPoolCount(gFartexMemPool),
            MemPoolSize(gFartexMemPool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "Cock C=%d S=%d",
            MemPoolCount(gCockMemPool),
            MemPoolSize(gCockMemPool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "BSPLib C=%d S=%d",
            MemPoolCount(gBSPLibMemPool),
            MemPoolSize(gBSPLibMemPool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "Obj Heap C=%d S=%d",
            MemPoolCount(gObjMemPool),
            MemPoolSize(gObjMemPool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "ReadIn C=%d S=%d",
            MemPoolCount(gReadInMemPool),
            MemPoolSize(gReadInMemPool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "ResMgr C=%d S=%d",
            MemPoolCount(gResmgrMemPool),
            MemPoolSize(gResmgrMemPool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "TexDB C=%d S=%d",
            MemPoolCount(gTexDBMemPool),
            MemPoolSize(gTexDBMemPool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "ATC Brain C=%d S=%d",
            MemPoolCount(ATCBrain::pool),
            MemPoolSize(ATCBrain::pool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "Palette C=%d S=%d",
            MemPoolCount(Palette::pool),
            MemPoolSize(Palette::pool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "Sound C=%d S=%d",
            MemPoolCount(gSoundMemPool),
            MemPoolSize(gSoundMemPool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    sprintf(tmpStr, "Vu Filter C=%d S=%d",
            MemPoolCount(gVuFilterMemPool),
            MemPoolSize(gVuFilterMemPool));

    renderer->TextLeft(col + 1.2f, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "Input C=%d S=%d",
            MemPoolCount(gInputMemPool),
            MemPoolSize(gInputMemPool));

    renderer->TextLeft(col, row, tmpStr);

    sprintf(tmpStr, "Tacan C=%d S=%d",
            MemPoolCount(gTacanMemPool),
            MemPoolSize(gTacanMemPool));

    renderer->TextLeft(col + 0.6f, row, tmpStr);

    row -= 0.05f;

#ifdef DEBUG
    sprintf(tmpStr, "SimObj Nodes=%d SimObj Refs=%d",
            ObjectNodes,
            ObjectReferences);
#endif
    renderer->TextLeft(col, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "Total Object Count %d", objCount);

    renderer->TextLeft(col, row, tmpStr);

    totCount += MemPoolCount(ObjectiveClass::pool);
    totSize += MemPoolSize(ObjectiveClass::pool) ;

    totCount += MemPoolCount(BattalionClass::pool);
    totSize += MemPoolSize(BattalionClass::pool) ;

    totCount += MemPoolCount(BrigadeClass::pool);
    totSize += MemPoolSize(BrigadeClass::pool) ;

    totCount += MemPoolCount(FlightClass::pool);
    totSize += MemPoolSize(FlightClass::pool) ;

    totCount += MemPoolCount(SquadronClass::pool);
    totSize += MemPoolSize(SquadronClass::pool) ;

    totCount += MemPoolCount(PackageClass::pool);
    totSize += MemPoolSize(PackageClass::pool) ;

    totCount += MemPoolCount(TaskForceClass::pool);
    totSize += MemPoolSize(TaskForceClass::pool) ;

    totCount += MemPoolCount(SimPersistantClass::pool);
    totSize += MemPoolSize(SimPersistantClass::pool) ;

    totCount += MemPoolCount(Drawable2D::pool);
    totSize += MemPoolSize(Drawable2D::pool) ;

    totCount += MemPoolCount(DrawableTracer::pool);
    totSize += MemPoolSize(DrawableTracer::pool) ;

    totCount += MemPoolCount(DrawableGroundVehicle::pool);
    totSize += MemPoolSize(DrawableGroundVehicle::pool) ;

    totCount += MemPoolCount(DrawableBuilding::pool);
    totSize += MemPoolSize(DrawableBuilding::pool) ;

    totCount += MemPoolCount(DrawableBSP::pool);
    totSize += MemPoolSize(DrawableBSP::pool) ;

    totCount += MemPoolCount(DrawableShadowed::pool);
    totSize += MemPoolSize(DrawableShadowed::pool) ;

    totCount += MemPoolCount(DrawableBridge::pool);
    totSize += MemPoolSize(DrawableBridge::pool) ;

    totCount += MemPoolCount(DrawableOvercast::pool);
    totSize += MemPoolSize(DrawableOvercast::pool) ;

    totCount += MemPoolCount(DrawablePlatform::pool);
    totSize += MemPoolSize(DrawablePlatform::pool) ;

    totCount += MemPoolCount(DrawableRoadbed::pool);
    totSize += MemPoolSize(DrawableRoadbed::pool) ;

    totCount += MemPoolCount(DrawableTrail::pool);
    totSize += MemPoolSize(DrawableTrail::pool) ;

    totCount += MemPoolCount(TrailElement::pool);
    totSize += MemPoolSize(TrailElement::pool) ;

    totCount += MemPoolCount(DrawablePuff::pool);
    totSize += MemPoolSize(DrawablePuff::pool) ;

    totCount += MemPoolCount(DrawablePoint::pool);
    totSize += MemPoolSize(DrawablePoint::pool) ;

    totCount += MemPoolCount(DrawableGuys::pool);
    totSize += MemPoolSize(DrawableGuys::pool) ;

    totCount += MemPoolCount(TBlock::pool);
    totSize += MemPoolSize(TBlock::pool) ;

    totCount += MemPoolCount(TListEntry::pool);
    totSize += MemPoolSize(TListEntry::pool) ;

    totCount += MemPoolCount(TBlockList::pool);
    totSize += MemPoolSize(TBlockList::pool) ;

    totCount += MemPoolCount(ObjectLOD::pool);
    totSize += MemPoolSize(ObjectLOD::pool) ;

    totCount += MemPoolCount(glMemPool);
    totSize += MemPoolSize(glMemPool) ;

    totCount += MemPoolCount(displayList::pool);
    totSize += MemPoolSize(displayList::pool) ;

    totCount += MemPoolCount(sfxRequest::pool);
    totSize += MemPoolSize(sfxRequest::pool) ;

    totCount += MemPoolCount(AirframeClass::pool);
    totSize += MemPoolSize(AirframeClass::pool) ;

    totCount += MemPoolCount(AircraftClass::pool);
    totSize += MemPoolSize(AircraftClass::pool) ;

    totCount += MemPoolCount(MissileClass::pool);
    totSize += MemPoolSize(MissileClass::pool) ;

    totCount += MemPoolCount(BombClass::pool);
    totSize += MemPoolSize(BombClass::pool) ;

    totCount += MemPoolCount(SimObjectType::pool);
    totSize += MemPoolSize(SimObjectType::pool) ;

    totCount += MemPoolCount(SimObjectLocalData::pool);
    totSize += MemPoolSize(SimObjectLocalData::pool) ;

    totCount += MemPoolCount(SfxClass::pool);
    totSize += MemPoolSize(SfxClass::pool) ;

    totCount += MemPoolCount(GunClass::pool);
    totSize += MemPoolSize(GunClass::pool) ;

    totCount += MemPoolCount(GroundClass::pool);
    totSize += MemPoolSize(GroundClass::pool) ;

    totCount += MemPoolCount(GNDAIClass::pool);
    totSize += MemPoolSize(GNDAIClass::pool) ;

    totCount += MemPoolCount(SimFeatureClass::pool);
    totSize += MemPoolSize(SimFeatureClass::pool) ;

    totCount += MemPoolCount(MissileInFlightData::pool);
    totSize += MemPoolSize(MissileInFlightData::pool) ;

    totCount += MemPoolCount(drawPtrList::pool);
    totSize += MemPoolSize(drawPtrList::pool) ;

    totCount += MemPoolCount(WayPointClass::pool);
    totSize += MemPoolSize(WayPointClass::pool) ;

    totCount += MemPoolCount(SMSBaseClass::pool);
    totSize += MemPoolSize(SMSBaseClass::pool) ;

    totCount += MemPoolCount(SMSClass::pool);
    totSize += MemPoolSize(SMSClass::pool) ;

    totCount += MemPoolCount(BasicWeaponStation::pool);
    totSize += MemPoolSize(BasicWeaponStation::pool) ;

    totCount += MemPoolCount(AdvancedWeaponStation::pool);
    totSize += MemPoolSize(AdvancedWeaponStation::pool) ;

    totCount += MemPoolCount(HelicopterClass::pool);
    totSize += MemPoolSize(HelicopterClass::pool) ;

    totCount += MemPoolCount(CONVERSATION::pool);
    totSize += MemPoolSize(CONVERSATION::pool) ;

    totCount += MemPoolCount(VM_BUFFLIST::pool);
    totSize += MemPoolSize(VM_BUFFLIST::pool) ;

    totCount += MemPoolCount(VM_CONVLIST::pool);
    totSize += MemPoolSize(VM_CONVLIST::pool) ;

    totCount += MemPoolCount(runwayQueueStruct::pool);
    totSize += MemPoolSize(runwayQueueStruct::pool) ;

    totCount += MemPoolCount(HeliBrain::pool);
    totSize += MemPoolSize(HeliBrain::pool) ;

    totCount += MemPoolCount(AirframeDataPool);
    totSize += MemPoolSize(AirframeDataPool) ;

    totCount += MemPoolCount(ThreePointLimiter::pool);
    totSize += MemPoolSize(ThreePointLimiter::pool) ;

    totCount += MemPoolCount(ValueLimiter::pool);
    totSize += MemPoolSize(ValueLimiter::pool) ;

    totCount += MemPoolCount(PercentLimiter::pool);
    totSize += MemPoolSize(PercentLimiter::pool) ;

    totCount += MemPoolCount(LineLimiter::pool);
    totSize += MemPoolSize(LineLimiter::pool) ;

    totCount += MemPoolCount(SimVuDriver::pool);
    totSize += MemPoolSize(SimVuDriver::pool) ;

    totCount += MemPoolCount(SimVuSlave::pool);
    totSize += MemPoolSize(SimVuSlave::pool) ;

    totCount += MemPoolCount(ObjectGeometry::pool);
    totSize += MemPoolSize(ObjectGeometry::pool) ;

    totCount += MemPoolCount(ListClass::pool);
    totSize += MemPoolSize(ListClass::pool) ;

    totCount += MemPoolCount(ListElementClass::pool);
    totSize += MemPoolSize(ListElementClass::pool) ;

    totCount += MemPoolCount(VuLinkNode::pool);
    totSize += MemPoolSize(VuLinkNode::pool) ;

    totCount += MemPoolCount(vuRBNodepool);
    totSize += MemPoolSize(vuRBNodepool) ;

    totCount += MemPoolCount(graphicsDOFDataPool);
    totSize += MemPoolSize(graphicsDOFDataPool) ;

    //  totCount += MemPoolCount( LoadoutStruct::pool );
    //  totSize += MemPoolSize( LoadoutStruct::pool ) ;

    totCount += MemPoolCount(gVuMsgMemPool);
    totSize += MemPoolSize(gVuMsgMemPool) ;

    totCount += MemPoolCount(UnitDeaggregationData::pool);
    totSize += MemPoolSize(UnitDeaggregationData::pool) ;

    totCount += MemPoolCount(DivisionClass::pool);
    totSize += MemPoolSize(DivisionClass::pool) ;

    totCount += MemPoolCount(MissionRequestClass::pool);
    totSize += MemPoolSize(MissionRequestClass::pool) ;

    totCount += MemPoolCount(EventElement::pool);
    totSize += MemPoolSize(EventElement::pool) ;

    if (gDivVUIDs)
    {
        totCount += MemPoolCount(gDivVUIDs);
        totSize += MemPoolSize(gDivVUIDs) ;
    }

    totCount += MemPoolCount(gFaultMemPool);
    totSize += MemPoolSize(gFaultMemPool) ;

    totCount += MemPoolCount(gTextMemPool);
    totSize += MemPoolSize(gTextMemPool) ;

    totCount += MemPoolCount(gTPostMemPool);
    totSize += MemPoolSize(gTPostMemPool) ;

    totCount += MemPoolCount(gFartexMemPool);
    totSize += MemPoolSize(gFartexMemPool) ;

    totCount += MemPoolCount(gCockMemPool);
    totSize += MemPoolSize(gCockMemPool) ;

    totCount += MemPoolCount(gBSPLibMemPool);
    totSize += MemPoolSize(gBSPLibMemPool) ;

    totCount += MemPoolCount(gObjMemPool);
    totSize += MemPoolSize(gObjMemPool) ;

    totCount += MemPoolCount(gReadInMemPool);
    totSize += MemPoolSize(gReadInMemPool) ;

    totCount += MemPoolCount(gResmgrMemPool);
    totSize += MemPoolSize(gResmgrMemPool) ;

    totCount += MemPoolCount(gTexDBMemPool);
    totSize += MemPoolSize(gTexDBMemPool) ;

    totCount += MemPoolCount(Palette::pool);
    totSize += MemPoolSize(Palette::pool) ;

    totCount += MemPoolCount(ATCBrain::pool);
    totSize += MemPoolSize(ATCBrain::pool) ;

    totCount += MemPoolCount(gSoundMemPool);
    totSize += MemPoolSize(gSoundMemPool) ;

    totCount += MemPoolCount(gInputMemPool);
    totSize += MemPoolSize(gInputMemPool) ;

    totCount += MemPoolCount(gTacanMemPool);
    totSize += MemPoolSize(gTacanMemPool) ;

    totCount += MemPoolCount(gVuFilterMemPool);
    totSize += MemPoolSize(gVuFilterMemPool) ;

    row -= 0.05f;

    sprintf(tmpStr, "Total Falc Pool Count %d, Size %d", totCount, totSize);

    renderer->TextLeft(col, row, tmpStr);

    row -= 0.05f;

    sprintf(tmpStr, "Total Falc+Def Pool Count %d, Size %d",
            MemPoolCount(MemDefaultPool) + totCount,
            MemPoolSize(MemDefaultPool) + totSize);


    renderer->TextLeft(col, row, tmpStr);

}
#endif // MEM_DEBUG
