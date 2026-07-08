#include "Graphics/Include/canvas3d.h"
#include "Graphics/DXEngine/OpenXRBackend.h"   // VR: HMD head-tracking (independent of TrackIR)
#include "Graphics/DXEngine/D3D11Backend.h"     // Artscout - 2026 (VR): eye size for click hit-test scaling
#include "Graphics/Include/drawbsp.h"
#include "Graphics/Include/renderow.h"
#include "Graphics/Include/texbank.h"   // PHASE 5: TheTextureBank.WaitUpdates() for synchronous loading of cockpit textures
#include "stdhdr.h"
#include "soundfx.h"
#include "fsound.h"
#include "playerrwr.h"
#include "sms.h"
#include "simdrive.h"
#include "aircrft.h"
#include "simweapn.h"
#include "cpmanager.h"
#include "hud.h"
#include "mfd.h"
#include "airframe.h"
#include "otwdrive.h"
#include "Graphics/Include/tod.h"
#include "flightData.h"
#include "vdial.h"
#include "fack.h"
#include "dofsnswitches.h"
#include "sinput.h" //Wombat778 10-10-2003  Added for 3d clickable cockpit
#include "commands.h" //Wombat778 10-10-2003  Added for 3d clickable cockpit
#include "FakeRand.h"
#include "cphsi.h"

extern bool g_bUse_DX_Engine; // COBRA - RED

#include "TrackIR.h" // Retro 24Dez2004
extern bool g_bEnableTrackIR; // Retro 24Dez2004
extern bool g_bTrackIRon; // Retro 24Dez2004
extern bool g_bUse6DOFTir; // Retro 24Dez2004
extern TrackIR theTrackIRObject; // Retro 24Dez2004
extern float g_fTIRMinimumFOV; // Cobra
extern float g_fTIRMaximumFOV; // Cobra
extern int g_n6DOFTIR; // Cobra
extern float g_fDefaultFOV;  //Wombat778 10-31-2003
extern float g_fNarrowFOV;  //Wombat778 2-21-2004
extern int narrowFOV;

extern DWORD p3DpitHilite; // Cobra - 3D pit high night lighting color
extern DWORD p3DpitLolite; // Cobra - 3D pit low night lighting color
extern int curColorIdx;

/* S.G. FOR HMS CODE */
#include "missile.h"

//MI for ICP stuff
extern bool g_bRealisticAvionics ;
#include "navsystem.h"

//Wombat778 3D Cockpit variables
//extern bool g_b3DClickableCockpit;
extern bool g_b3DClickableCockpitDebug;
extern bool g_b3DRTTCockpitDebug;
extern int FindBestResolution(void); //Wombat778 4-03-04

// RV - Biker - Theater switching stuff
extern char FalconCockpitThrDirectory[];

void CallFunc(InputFunctionType theFunc, unsigned long val, int state, void* pButton); //Wombat778 03-06-04

float resScale = 0.66667f;
// #7 AA RTT font: display-text size multiplier for the enlarged RTT atlas (1024).
// Zones scale by resScale, but the pixel font does not; this multiplier is applied
// in ScreenText/ScreenTextWidth/Height ONLY during the RTT pass (g_rttBatchActive).
// 1.0 = no scale (atlas 512/high resolution). Set in the 1024 blocks below.
float g_rttFontScale = 1.0f;

// #7 display SSAA (variant 2, VR goal): a SINGLE supersampling multiplier for the RTT atlas.
// Atlas = 768*SS, zones *SS (resScale), font *SS (g_rttFontScale) -- all three consistent,
// so the apparent symbology size is preserved, but there are SS times more pixels. DrawRttQuad
// (bilinear) downscales atlas->panel = clean AA of lines/circle/text WITHOUT 'jumps' in
// height or 'clunkiness' (previously at high-res zones did NOT scale -> text '+2x').
// 1.0 = off (as before). 2.0 = 4x samples. 3.0 = 9x samples (atlas 768*3=2304,
// zones fit: max 750*3=2250 < 2304). Atlas memory 2304^2*4 ~ 21 MB -- acceptable.
float g_rttSS = 3.0f;
//ATARIBABY start added for new 3d pit code
float DEDw;
float DEDh;
//externalised dynamic head params
extern float g_fDyn_Head_TiltMul;
extern float g_fDyn_Head_TiltRndGMul;
extern float g_fDyn_Head_RollMul;
extern float g_fDyn_Head_PanMul;
extern float g_fDyn_Head_TiltRateMul;
extern float g_fDyn_Head_TiltGRateMul;
extern float g_fDyn_Head_RollRate;
extern float g_fDyn_Head_PanRate;

extern int gameCompressionRatio; //added to know if sim is paused to not do MoveByRate stuff

extern bool g_bUseNew3dpit;
extern bool g_bINS;

static float LastMainADIPitch3d = 0.0F;
static float LastMainADIRoll3d = 0.0F;
static float LastBUPPitch3d = 0.0F;
static float LastBUPRoll3d = 0.0F;
static float ADIPitch3d = 0.0F;
static float ADIRoll3d = 0.0F;
static float BUPADIRoll3d = 0.0F;
static float BUPADIPitch3d = 0.0F;
static float HYDA3d = 0.0F;
static float HYDB3d = 0.0F;

//magnetic compass
static float MAGCOMPASS3d = 0.0F;

//ILS needles
static float hILS = -1.1F;
static float vILS = -1.1F;
static float hILSneedle = -1.0F;
static float vILSneedle = -1.0F;
static long  prevILStime = vuxGameTime;

//HSI To/From flags - evalueated in OTWLOOP.CPP
extern int HSITOFROM3d;

//start declarations for dynamic head
static float BobbingPreviousRoll = 0.0f;
static float BobbingPreviousPan = 0.0f;
float BobbingRollRate = 0.0f;
float BobbingTilt = 0.0f;
float BobbingPan = 0.0f;
//ATARIBABY disabled now - fwd/back lean cause normals problems and i not know solution yet
//float BobbingAccel = 0.0f;
static long  BobbingPreviousTime = vuxGameTime;
//ATARIBABY end

// JB 010802
extern bool g_b3dCockpit;
extern bool g_b3dHUD;
extern bool g_b3dMFDLeft;
extern bool g_b3dMFDRight;
extern bool g_b3dRWR;
extern bool g_b3dICP;
extern bool g_b3dDials;
extern bool g_b3dDynamicPilotHead; // JB 010804
extern bool g_b3DClickableCursorChange; //Wombat778 10-15-2003
//extern bool g_b3DExpandedHeadRange; //Wombat778 10-15-2003
extern int g_n3DHeadPanRange; //Wombat778 2-21-2004
extern int g_n3DHeadTiltRange; //Wombat778 2-21-2004

// Artscout - 2026: head pan limit (3D cockpit free-look yaw). Was 150 deg -> you could not look
// straight back, and at the limit the view "snapped over the shoulder". Raised to 180 so the head
// turns the full hemisphere both ways (look directly aft) -- effectively no limit, which is what VR
// needs anyway (and the snap-over no longer triggers in normal use).
#define PAN_LIMIT 180.0F
extern void* gSharedMemPtr;

using namespace std;
extern string RemoveInvalidChars(const string &instr);

void OTWDriverClass::VCock_CheckStopStates(float dT)
{
    if (stopState == STOP_STATE0)
    {
        if ((azDir > 0.0F and eyePan <= PAN_LIMIT * DTR) or (azDir < 0.0F and eyePan >= PAN_LIMIT * DTR))
        {

            stopState = STOP_STATE1;
            eyePan = min(max(eyePan, -PAN_LIMIT * DTR), PAN_LIMIT * DTR);
            F4SoundFXSetDist(SFX_CP_UGH, TRUE, 0.0f, 1.0f);
        }
        else
        {
            VCock_RunNormalMotion(dT);
        }
    }
    else if (stopState == STOP_STATE1)
    {
        if ((azDir > 0.0F and eyePan <= -PAN_LIMIT * DTR) or (azDir < 0.0F and eyePan >= PAN_LIMIT * DTR))
        {
            stopState = STOP_STATE1;
        }
        else if (azDir == 0.0F)
        {
            stopState = STOP_STATE2;
        }
        else
        {
            stopState = STOP_STATE0;
        }
    }
    else if (stopState == STOP_STATE2)
    {
        if ((azDir > 0.0F and eyePan <= -PAN_LIMIT * DTR) or (azDir < 0.0F and eyePan >= PAN_LIMIT * DTR))
        {
            headMotion = HEAD_TRANSISTION1;
            initialTilt = eyeTilt;
            eyePan = min(max(eyePan, -PAN_LIMIT * DTR), PAN_LIMIT * DTR);

            if (eyePan <= -PAN_LIMIT * DTR)
            {
                snapDir = LTOR;
            }
            else
            {
                snapDir = RTOL;
            }
        }
        else if (azDir == 0.0F)
        {
            VCock_RunNormalMotion(dT);
            stopState = STOP_STATE2;
        }
        else
        {
            stopState = STOP_STATE0;
            VCock_RunNormalMotion(dT);
        }
    }
    else if (stopState == STOP_STATE3)
    {
        if (azDir == 0.0F)
        {
            stopState = STOP_STATE2;
        }
    }
}

void OTWDriverClass::VCock_RunNormalMotion(float dT)
{
    stopState = STOP_STATE0;

    if ( not mUseHeadTracking)
    {
        eyePan -= azDir * slewRate * 4.0F * dT;
        eyeTilt += elDir * slewRate * 4.0F * dT;

        //Wombat778 2-21-04 Removed this as it doesnt seem pointful

        /*    if(eyeTilt <= -90.0F * DTR) {
        eyePan = min(max(eyePan, -PAN_LIMIT * DTR), PAN_LIMIT * DTR);
        eyeTilt = min(max(eyeTilt, -140.0F * DTR), 25.0F * DTR);
        // eyeTilt = min(max(eyeTilt, -150.0F * DTR), 25.0F * DTR);
        // BuildHeadMatrix(TRUE, YAW_PITCH, eyePan + 180.0F * DTR, -(eyeTilt + 180.0F * DTR), 0.0F);
        BuildHeadMatrix(TRUE, YAW_PITCH, (eyePan + 180.0F * DTR) + BobbingPan, -(eyeTilt + 180.0F * DTR) + BobbingTilt, BobbingRollRate);  //ATARIBABY dynamic head added
        }
        else
        {
        */
        //Wombat778 2-21-2004  Changed the expandedheadrange variable to the following independant adjustments.  This should allow a suitable head range
        // to be selected as more complete 3d pits get built in the future
        switch (g_n3DHeadPanRange)
        {
            case 0: //MPS default pan stops
                eyePan = min(max(eyePan, -PAN_LIMIT * DTR), PAN_LIMIT * DTR);
                break;

            case 1: //Stops removed.  +-180degrees
                eyePan = min(max(eyePan, -180.0f * DTR), 180.0f * DTR);
                break;

            case 2: //Wraparound left/right
                if (eyePan > 180.0f * DTR) eyePan -= 360.0f * DTR;
                else if (eyePan < -180.0f * DTR) eyePan += 360.0f * DTR;

                break;

            default:
                eyePan = min(max(eyePan, -PAN_LIMIT * DTR), PAN_LIMIT * DTR);
                break;
        }

        switch (g_n3DHeadTiltRange)
        {

            case 0: //MPS default tilt
                eyeTilt = min(max(eyeTilt, -140.0F * DTR), 25.0F * DTR);
                break;

            case 1: //BMS default tilt.  Takes FOV into account
                if (GetFOV() < 60.0F * DTR) //Wombat778 10-23-2003  Dont do anything with FOV if it is greater than 60
                    eyeTilt = min(max(eyeTilt, -140.0F * DTR), (35.0F + ((60.0F - (GetFOV() * RTD))) *
                                      0.395F) * DTR);
                else
                    eyeTilt = min(max(eyeTilt, -140.0F * DTR), 40.0F * DTR);

                break;

            case 2: //Significantly expanded tilt range.  Can look 90 degrees down
                eyeTilt = min(max(eyeTilt, -140.0F * DTR), 90.0F * DTR);
                break;

            case 3: //Full vertical range +- 180 degrees
                eyeTilt = min(max(eyeTilt, -180.0F * DTR), 180.0F * DTR);
                break;

            case 4: //Wraparound tilt
                if (eyeTilt > 180.0f * DTR) eyeTilt -= 360.0f * DTR;
                else if (eyeTilt < -180.0f * DTR) eyeTilt += 360.0f * DTR;

                break;

            default:
                if (GetFOV() < 60.0F * DTR)
                    eyeTilt = min(max(eyeTilt, -110.0F * DTR),
                                      (35.0F + ((60.0F - (GetFOV() * RTD))) * 0.395F) * DTR);
                else
                    eyeTilt = min(max(eyeTilt, -110.0F * DTR), 35.0F * DTR);

                break;
        }


        // BuildHeadMatrix(FALSE, YAW_PITCH, eyePan, eyeTilt, 0.0F);

        //Wombat778 2-24-2004 Added this to stop the head from flipping when looking above 90 degrees up
        //Wombat778 3-12-2003 changes >= 90.0 to > 90.0 which prevents a flip when looking directly down.
        //ATARIBABY added BobbingTilt change to tilt angle checks and branched for look up/down

        if (eyeTilt + BobbingTilt < -90.0F * DTR)
            BuildHeadMatrix(TRUE, YAW_PITCH, (eyePan + 180.0F * DTR) + BobbingPan, -(eyeTilt + 180.0F * DTR) + BobbingTilt, BobbingRollRate);  //ATARIBABY dynamic head added
        else if (fabs(eyeTilt + BobbingTilt) > 90.0F * DTR)
            BuildHeadMatrix(TRUE, YAW_PITCH, eyePan + BobbingPan, eyeTilt + BobbingTilt, BobbingRollRate); //ATARIBABY dynamic head added
        else
            BuildHeadMatrix(FALSE, YAW_PITCH, eyePan + BobbingPan, eyeTilt + BobbingTilt, BobbingRollRate); //ATARIBABY dynamic head added

        //    }
    }
    else
    {

        eyePan = cockpitFlightData.headYaw;
        eyeTilt = cockpitFlightData.headPitch;
        eyeHeadRoll = cockpitFlightData.headRoll;

        if (eyeTilt <= -90.0F * DTR)
            BuildHeadMatrix(TRUE, YAW_PITCH, eyePan, eyeTilt, eyeHeadRoll);
        else
            BuildHeadMatrix(FALSE, YAW_PITCH, eyePan, eyeTilt, eyeHeadRoll);
    }
}



void OTWDriverClass::VCock_Glance(float dT)
{
    // No glances when using a head tracker
    if (mUseHeadTracking)
        return;

    if (padlockGlance == GlanceNose)   // if player glances forward
    {

        if ( not mIsSlewInit)
        {
            mIsSlewInit = TRUE;
            mSlewPStart = eyePan;
            mSlewTStart = eyeTilt;
        }

        PadlockF3_SlewCamera(mSlewPStart, mSlewTStart, 0.0F, 0.0F, 5.0F, 0.001F, dT);
    }
    else if (padlockGlance == GlanceTail)   // if player glances back
    {

        if (eyePan < 0.0F)
        {

            if ( not mIsSlewInit)
            {
                mIsSlewInit = TRUE;
                mSlewPStart = eyePan;
                mSlewTStart = eyeTilt;
            }

            PadlockF3_SlewCamera(mSlewPStart, mSlewTStart, -180.0F * DTR,  0.0F, 5.0F, 0.001F, dT);
        }
        else if (eyePan > 0.0F)
        {

            if ( not mIsSlewInit)
            {
                mIsSlewInit = TRUE;
                mSlewPStart = eyePan;
                mSlewTStart = eyeTilt;
            }

            PadlockF3_SlewCamera(mSlewPStart, mSlewTStart, 180.0F * DTR, 0.0F, 5.0F, 0.001F, dT);
        }
        else
        {
            eyePan = 0.001F;
        }
    }
    else
    {
        padlockGlance = GlanceNone;
    }
}



void OTWDriverClass::VCock_GiveGilmanHead(float dT)
{
    // No limits when using a head tracker
    if (padlockGlance not_eq GlanceNone)
    {
        VCock_Glance(dT);
        BuildHeadMatrix(FALSE, YAW_PITCH, eyePan, eyeTilt, 0.0F);
    }
    else
    {
        if (headMotion == YAW_PITCH)
        {
            if (eyePan <= -PAN_LIMIT * DTR or  eyePan >= PAN_LIMIT * DTR)
            {
                VCock_CheckStopStates(dT);
            }
            else
            {
                VCock_RunNormalMotion(dT);
            }
        }

        if (headMotion == HEAD_TRANSISTION1)
        {

            if (initialTilt <= -90.0F * DTR)
            {
                BuildHeadMatrix(TRUE, YAW_PITCH, eyePan, eyeTilt, 0.0F);
                headMotion = HEAD_TRANSISTION2;
            }
            else if (initialTilt > -90.0F * DTR and eyeTilt > -92.0F * DTR)
            {

                eyeTilt -= slewRate * 10.0F * dT;

                eyeTilt = max(eyeTilt, -92.0F * DTR);

                if (eyeTilt >= -90.0F * DTR)
                {
                    BuildHeadMatrix(FALSE, YAW_PITCH, eyePan, eyeTilt, 0.0F);
                }
                else
                {
                    BuildHeadMatrix(TRUE, YAW_PITCH, eyePan, eyeTilt, 0.0F);
                }
            }
            else
            {
                eyeTilt = -92.0F * DTR;
                headMotion = HEAD_TRANSISTION2;
            }
        }

        if (headMotion == HEAD_TRANSISTION2)
        {


            if ((snapDir == RTOL or snapDir == LTOR) and ((eyePan >= PAN_LIMIT * DTR) or (eyePan <= -PAN_LIMIT * DTR)))
            {
                eyePan -= snapDir * slewRate * 10.0F * dT;

                if (eyePan > 180.0F * DTR)
                {
                    eyePan = -360.0F * DTR + eyePan;
                }
                else if (eyePan < -180.0F * DTR)
                {
                    eyePan = 360.0F * DTR + eyePan;
                }

                if (eyePan < 0.0F)
                {
                    eyePan = min(eyePan, -PAN_LIMIT * DTR);

                    if (eyePan == -PAN_LIMIT * DTR)
                    {
                        headMotion = HEAD_TRANSISTION3;
                    }
                }
                else
                {

                    eyePan = max(eyePan, PAN_LIMIT * DTR);

                    if (eyePan == PAN_LIMIT * DTR)
                    {
                        headMotion = HEAD_TRANSISTION3;
                    }
                }

                BuildHeadMatrix(TRUE, YAW_PITCH, eyePan, eyeTilt, 0.0F);
            }
            else
            {
                eyePan = max(min(eyePan, -PAN_LIMIT * DTR), PAN_LIMIT * DTR);
                headMotion = HEAD_TRANSISTION3;
            }
        }

        if (headMotion == HEAD_TRANSISTION3)
        {

            if (/*azDir and */ initialTilt >= -92.0F * DTR)
            {

                if (eyeTilt < initialTilt)
                {
                    eyeTilt += slewRate * 10.0F * dT;
                    eyeTilt = min(eyeTilt, initialTilt);
                }
                else
                {
                    stopState = STOP_STATE3;

                    eyeTilt = initialTilt;
                    headMotion = YAW_PITCH;
                }

                if (eyeTilt >= -90.0F * DTR)
                {
                    BuildHeadMatrix(FALSE, YAW_PITCH, eyePan, eyeTilt, 0.0F);
                }
                else
                {
                    BuildHeadMatrix(TRUE, YAW_PITCH, eyePan, eyeTilt, 0.0F);
                }
            }
            else
            {
                stopState = STOP_STATE3;

                eyeTilt = initialTilt;
                headMotion = YAW_PITCH;
                BuildHeadMatrix(TRUE, YAW_PITCH, eyePan, eyeTilt, 0.0F);
            }
        }
    }

    // Combine the head and airplane matrices
    MatrixMult(&ownshipRot, &headMatrix, &cameraRot);
}


// points defining the virtual HUD and other instruments
// points given from ART dept
// Modified by leonr based on hud overwrites
//Tpoint vHUDul = { 20.063f , -3.089f, -0.481f };
//Tpoint vHUDur = { 20.063f ,  3.089f, -0.481f };
//Tpoint vHUDll = { 20.063f , -3.089f, 4.877f };
// edg changed again....
// Tpoint vHUDul = { 20.063f , -2.75f, -0.21f };
// Tpoint vHUDur = { 20.063f ,  2.75f, -0.21f };
// Tpoint vHUDll = { 20.063f , -2.75f, 4.6f };

bool bRTTTarget = false;
bool hasPFL = false;
// 1600 RTT canvas size
int txRes = 768;
int tyRes = 768;
int tBpp = 32;
// <= 1024 RTT canvas size
//int txRes = 512;
//int tyRes = 512;

Tpoint vHUDul = { 20.063f , -2.75f, -0.456f };
Tpoint vHUDur = { 20.063f ,  2.75f, -0.456f };
Tpoint vHUDll = { 20.063f , -2.75f, 4.633f };
int tHUDleft = 1; // ASSO:
int tHUDtop = 1;
int tHUDright = 430;
int tHUDbottom = 430;

Tpoint vRWRul = { 18.780f , -4.368f, 5.147f };
Tpoint vRWRur = { 18.779f , -2.486f, 5.147f };
Tpoint vRWRll = { 18.676f , -4.368f, 7.018f };
int tRWRleft = 250; // ASSO:
int tRWRtop = 500;
int tRWRright = 430;
int tRWRbottom = 680;

Tpoint vMACHul = { 21.178f , -1.853f, 8.934f };
Tpoint vMACHur = { 21.178f , -0.053f, 8.934f };
Tpoint vMACHll = { 21.085f , -1.853f, 10.732f };



Tpoint vDEDul = { 18.637f ,  2.577f, 5.180f };
Tpoint vDEDur = { 18.637f ,  6.777f, 5.180f };
Tpoint vDEDll = { 18.474f ,  2.577f, 6.165f };
int tDEDleft = 1; // ASSO:
int tDEDtop = 500;
int tDEDright = 200;
int tDEDbottom = 580;

Tpoint vPFLul = { 18.637f ,  6.577f, 5.180f };
Tpoint vPFLur = { 18.637f ,  10.777f, 5.180f };
Tpoint vPFLll = { 18.474f ,  6.577f, 6.165f };
int tPFLleft = 1; // ASSO:
int tPFLtop = 600;
int tPFLright = 200;
int tPFLbottom = 680;

int trMFDleft = 550;
int trMFDtop = 250;
int trMFDright = 750;
int trMFDbottom = 450;

int tlMFDleft = 550;
int tlMFDtop = 1;
int tlMFDright = 750;
int tlMFDbottom = 200;

char dedStr1[60];
char dedStr2[60];
char dedStr3[60];

//First line
char string1[60] = "";
char string2[60] = "";
char string3[60] = "";
char string4[60] = "";
//Second Line
char string5[60] = "";
char string6[60] = "";
char string7[60] = "";
char string8[60] = "";
//Third Line
char string9[60] = "";
char string10[60] = "";
char string11[60] = "";
char string12[60] = "";
//Fourth Line
char string13[60] = "";
char string14[60] = "";
char string15[60] = "";
char string16[60] = "";


//-------------------------------------------------
Tpoint vOILul = { 17.990f ,  7.976f, 8.823f };
Tpoint vOILur = { 17.990f ,  8.676f, 8.823f };
Tpoint vOILll = { 17.870f ,  7.976f, 9.512f };

int vOILepts = 3;
float vOILvals[3] = {0.0f, 100.0f, 103.3f};
float vOILpts[3] = { -0.646f, 0.723f, 0.513f};
//-------------------------------------------------
Tpoint vNOZul = { 17.800f ,  8.076f, 9.906f };
Tpoint vNOZur = { 17.800f ,  9.076f, 9.906f };
Tpoint vNOZll = { 17.627f ,  8.076f, 10.891f };

int vNOZepts = 2;
float vNOZvals[2] = {0.0F, 100.0F};
float vNOZpts[2] = {0.944F, 2.269F};
//-------------------------------------------------
Tpoint vRPMul = { 17.575f ,  8.076f, 11.186f };
Tpoint vRPMur = { 17.575f ,  9.376f, 11.186f };
Tpoint vRPMll = { 17.349f ,  8.076f, 12.467f };

int vRPMepts = 4;
float vRPMvals[4] = {0.0F, 60.0F, 100.0F, 110.0F};
float vRPMpts[4] = {1.571F, 0.0F, 3.142F, 2.307F};
//-------------------------------------------------
Tpoint vFTITul = { 17.226f ,  8.675f, 13.156f };
Tpoint vFTITur = { 17.226f ,  9.875f, 13.156f };
Tpoint vFTITll = { 17.017f ,  8.675f, 14.338f };

int vFTITepts = 6;
float vFTITvals[6] = {2.0F, 6.0F, 8.0F, 9.0F, 10.0F, 12.0F};
float vFTITpts[6] = { -0.319F, -1.445F, -2.808F, 2.412F, 1.208F, 0.621F};
//-------------------------------------------------
Tpoint vALTul = { 21.178f ,  0.247f, 8.934f };
Tpoint vALTur = { 21.178f ,  2.047f, 8.934f };
Tpoint vALTll = { 21.085f ,  0.239f, 10.732f };

int vALTepts = 2;
float vALTvals[2] = {0.0F, 1000.0F};
float vALTpts[2] = {1.57F, 1.571F};

//-------------------------------------------------

#include "cpres.h"

// ASSO: new RTT canvas
bool OTWDriverClass::VCock_SetRttCanvas(char** plinePtr, Render2D** canvaspp, int dev)
{
    Tpoint ul, ur, ll;
    int tLeft, tTop, tRight, tBottom;
    char cBlend = 'c';
    float cAlpha = 1.0f;
    // extern int txRes, tyRes, tBpp;
    extern bool bRTTTarget;

    // Missing rttTarget line in 3dckpit.dat?
    if ( not bRTTTarget)
    {
        // Cobra - Lower screen resolutions need a smaller canvas (font is too small)
        // PHASE: 1024 instead of 512 = ~1:1 RTT atlas (shared by ALL displays:
        // HUD/DED/RWR/MFD/PFL). resScale also scales the zones (below), so
        // the apparent font size is preserved, while lines/fonts render into more
        // pixels -> DrawRttQuad (bilinear) downscales = antialiasing on all displays.
        // #7 SSAA: a single multiplier for atlas/zones/font (see g_rttSS). Without a gate by
        // resolution -- always supersample. Zone base = 768 (zones are hardcoded for it).
        resScale       = g_rttSS;
        g_rttFontScale = g_rttSS;
        txRes = (int)(768.0f * g_rttSS);
        tyRes = (int)(768.0f * g_rttSS);

        VirtualDisplay::SetupRttTarget(txRes, tyRes, tBpp);
    }

    // Canvas3D *canvas;
    Render2D* canvas;

    // no PFL data
    if (dev == 6)
    {
        ul.x = vPFLul.x;
        ul.y = vPFLul.y;
        ul.z = vPFLul.z;
        ur.x = vPFLur.x;
        ur.y = vPFLur.y;
        ur.z = vPFLur.z;
        ll.x = vPFLll.x;
        ll.y = vPFLll.y;
        ll.z = vPFLll.z;
        tLeft = tPFLleft;
        tTop = tPFLtop;
        tRight = tPFLright;
        tBottom = tPFLbottom;
        hasPFL = false;
    }
    else
    {
        char *ptoken = FindToken(plinePtr, "=;\n");

        if (sscanf(ptoken, "%f %f %f %f %f %f %f %f %f %d %d %d %d %c %f",
                   &ul.x, &ul.y, &ul.z,
                   &ur.x, &ur.y, &ur.z,
                   &ll.x, &ll.y, &ll.z,
                   &tLeft, &tTop, &tRight, &tBottom,
                   &cBlend, &cAlpha) == 9)
        {
            // no RTT canvas data. Use default.
            switch (dev)
            {
                case 1: // hud
                    tLeft = tHUDleft;
                    tTop = tHUDtop;
                    tRight = tHUDright;
                    tBottom = tHUDbottom;
                    break;

                case 2: // rwr
                    tLeft = tRWRleft;
                    tTop = tRWRtop;
                    tRight = tRWRright;
                    tBottom = tRWRbottom;
                    break;

                case 3: // ded
                    tLeft = tDEDleft;
                    tTop = tDEDtop;
                    tRight = tDEDright;
                    tBottom = tDEDbottom;
                    break;

                case 4: // pfl
                    tLeft = tPFLleft;
                    tTop = tPFLtop;
                    tRight = tPFLright;
                    tBottom = tPFLbottom;
                    hasPFL = false;
                    break;
            }
        }
        else
        {
            if (dev == 4)
                hasPFL = true;
        }
    }

    // #7 SSAA: always scale zones by resScale(=g_rttSS) -- consistent with atlas/font.
    if (tLeft > 1)
        tLeft = (int)(resScale * (float)tLeft);

    if (tRight > 1)
        tRight = (int)(resScale * (float)tRight);

    if (tTop > 1)
        tTop = (int)(resScale * (float)tTop);

    if (tBottom > 1)
        tBottom = (int)(resScale * (float)tBottom);

    // Artscout - 2026 (HUD): optionally widen the HUD glass FOV by scaling the canvas quad around its
    // own center. Our stock HUD glass is narrow (~7.8deg half) -> true-angular symbology (262mr ASEC)
    // overflows and the tapes cram inward. Scaling the canvas widens the FOV consistently: the HUD
    // half-angle (derived from this canvas) grows, so the existing symbology lands at correct F-16
    // angles while the FPM/pitch-ladder stay world-aligned. HUD only (dev 1). g_fHudCanvasScale=1=stock.
    extern float g_fHudCanvasScale;
    if (dev == 1 && g_fHudCanvasScale != 1.0f)
    {
        // FOV(horizontal) = half-width(Y) / distance(X); FOV(vertical) = half-height(Z) / distance(X).
        // So scale ONLY the Y/Z extents around the glass center, keep X (distance) -> the half-angle
        // grows cleanly by the factor. (Scaling X too would move the glass instead of widening the FOV.)
        Tpoint lr; lr.y = ur.y + (ll.y - ul.y); lr.z = ur.z + (ll.z - ul.z);
        const float cy = (ul.y + ur.y + ll.y + lr.y) * 0.25f;
        const float cz = (ul.z + ur.z + ll.z + lr.z) * 0.25f;
        const float s = g_fHudCanvasScale;
        ul.y = cy + (ul.y - cy) * s; ul.z = cz + (ul.z - cz) * s;
        ur.y = cy + (ur.y - cy) * s; ur.z = cz + (ur.z - cz) * s;
        ll.y = cy + (ll.y - cy) * s; ll.z = cz + (ll.z - cz) * s;
    }

    *canvaspp = canvas = new Render2D;
    canvas->Setup(renderer->GetImageBuffer());
    canvas->SetRttCanvas(&ul, &ur, &ll, cBlend, cAlpha);
    canvas->SetRttRect(tLeft, tTop, tRight, tBottom);
    return true;
}


// ASSO: old canvas
bool OTWDriverClass::VCock_SetCanvas(char **plinePtr, Canvas3D **canvaspp)
{
    Tpoint ul, ur, ll;
    Canvas3D *canvas;
    char *ptoken = FindToken(plinePtr, "=;\n");

    if (sscanf(ptoken, "%f %f %f %f %f %f %f %f %f",
               &ul.x, &ul.y, &ul.z,
               &ur.x, &ur.y, &ur.z,
               &ll.x, &ll.y, &ll.z) not_eq 9)
    {
        ShiAssert( not "Failed to parse canvas");
        *canvaspp = NULL;
        return false;
    }

    *canvaspp = canvas = new Canvas3D;
    canvas->Setup(renderer);
    canvas->SetCanvas(&ul, &ur, &ll);
    canvas->Update(&Origin, (struct Trotation *)&IMatrix);

    return true;
}

void
OTWDriverClass::VCock_ParseVDial(FILE *fp)
{
    VDialInitStr vdialInitStr;
    static const char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
    int valuesIndex = 0;
    int pointsIndex = 0;
    char plineBuffer[MAX_LINE_BUFFER];
    char *plinePtr, *ptoken;
    Tpoint ur, ul, ll;

    ZeroMemory(&vdialInitStr, sizeof vdialInitStr);
    vdialInitStr.callback = -1;

    fgets(plineBuffer, sizeof plineBuffer, fp);
    plinePtr = plineBuffer;
    ptoken = FindToken(&plinePtr, pseparators);
    vdialInitStr.ppoints = NULL;
    vdialInitStr.pvalues = NULL;

    while (strcmpi(ptoken, END_MARKER))
    {

        if ( not strcmpi(ptoken, PROP_NUMENDPOINTS_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &vdialInitStr.endPoints);
            vdialInitStr.ppoints = new float[vdialInitStr.endPoints];
            vdialInitStr.pvalues = new float[vdialInitStr.endPoints];
        }
        else if ( not strcmpi(ptoken, PROP_POINTS_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);

            while (ptoken)
            {
                F4Assert(pointsIndex < vdialInitStr.endPoints);
                sscanf(ptoken, "%f", &vdialInitStr.ppoints[pointsIndex]);
                ptoken = FindToken(&plinePtr, pseparators);
                pointsIndex++;
            }
        }
        else if ( not strcmpi(ptoken, PROP_VALUES_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);

            while (ptoken)
            {
                F4Assert(valuesIndex < vdialInitStr.endPoints);
                sscanf(ptoken, "%f", &vdialInitStr.pvalues[valuesIndex]);
                ptoken = FindToken(&plinePtr, pseparators);
                valuesIndex++;
            }
        }
        else if ( not strcmpi(ptoken, PROP_RADIUS0_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%f", &vdialInitStr.radius);
        }
        else if ( not strcmpi(ptoken, PROP_COLOR0_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &vdialInitStr.color);
        }
        else if ( not strcmpi(ptoken, PROP_CALLBACKSLOT_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &vdialInitStr.callback);
        }
        else if ( not strcmpi(ptoken, PROP_DESTLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");

            if (sscanf(ptoken, "%f %f %f %f %f %f %f %f %f",
                       &ul.x, &ul.y, &ul.z,
                       &ur.x, &ur.y, &ur.z,
                       &ll.x, &ll.y, &ll.z) == 9)
            {
                vdialInitStr.pUL = &ul;
                vdialInitStr.pUR = &ur;
                vdialInitStr.pLL = &ll;
            }
        }
        else
        {
            F4Assert( not "Unknown Line in dial defn");
        }

        fgets(plineBuffer, sizeof plineBuffer, fp);
        plinePtr = plineBuffer;
        ptoken = FindToken(&plinePtr, pseparators);
    }

    vdialInitStr.pRender = renderer;
    VDial *vdial = new VDial(&vdialInitStr);
    mpVDials.push_back(vdial);
    delete [] vdialInitStr.ppoints;
    delete [] vdialInitStr.pvalues;
}

bool
OTWDriverClass::VCock_Init(int eCPVisType, TCHAR* eCPName, TCHAR* eCPNameNCTR)
{
    char strCPFile[MAX_PATH];
    static const TCHAR *pCPFile = "3dckpit.dat";
    CP_HANDLE* pcockpitDataFile;
    static const char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
    extern Tpoint lMFDul, lMFDur, lMFDll;
    extern int ltMFDleft, ltMFDtop, ltMFDright, ltMFDbottom; // ASSO:
    extern char lcMFDblend;
    extern float lcMFDalpha;
    extern Tpoint rMFDul, rMFDur, rMFDll;
    extern int rtMFDleft, rtMFDtop, rtMFDright, rtMFDbottom; //, txRes, tyRes, tBpp; // ASSO:
    extern char rcMFDblend;
    extern float rcMFDalpha;
    extern bool bRTTTarget;
    int DebugLineNum;
    bool quitFlag = false;
    g_bUseNew3dpit = false; //Use new 3dpit code - needs new 3d pit model - assume not
    bRTTTarget = false; // RTT needs RTT dimensions in 3dckpit.dat

    // RV - RED - Init to default value
    vBoresightY = 0.75f;

    // COBRA - RED - Default Hud Color if not assigned by DAT FILE
    // TheHud->SetHudColor(DEFAULT_HUD_COLOR);

    // RV - Biker - Use fallback for cockpit path
    //FindCockpit(pCPFile, (Vis_Types)eCPVisType, eCPName, eCPNameNCTR, strCPFile);
    FindCockpit(pCPFile, (Vis_Types)eCPVisType, eCPName, eCPNameNCTR, strCPFile, TRUE);

    pcockpitDataFile = CP_OPEN(strCPFile, "r");

    F4Assert(pcockpitDataFile); //Error: Couldn't open file
    DebugLineNum = 0;

    while ( not quitFlag)
    {
        char plineBuffer[MAX_LINE_BUFFER];
        char *plinePtr, *ptoken;
        char *presult = fgets(plineBuffer, sizeof plineBuffer, pcockpitDataFile);
        DebugLineNum ++;
        quitFlag = (presult == NULL);

        if (quitFlag or *plineBuffer == '/' or *plineBuffer == '\n')
            continue;

        plinePtr = plineBuffer;
        ptoken = FindToken(&plinePtr, pseparators);

        // ASSO:
        if ( not strcmpi(ptoken, PROP_3D_RTTTARGET))    // the rttTarget :
        {
            ptoken = FindToken(&plinePtr, "=;\n");

            if (sscanf(ptoken, "%d %d %d",
                       &txRes, &tyRes, &tBpp) >= 2)
            {
                // Cobra - Lower screen resolutions need a smaller canvas (font is too small)
                // PHASE: 1024 = RTT atlas ~1:1 (antialiasing of all displays), see
                // VCock_SetRttCanvas. resScale scales the zones -> apparent size preserved.
                // #7: the high-res atlas experiment was reverted -- glyph size in the atlas (=pixels=
                // stability) and the VISIBLE size are linked via g_rttFontScale (one number), they can't
                // be separated by tuning. A large atlas gave sharpness only at the cost of +2x size; at
                // normal size there's no gain. The real fix for swimming at normal size is
                // supersampling the display panels (per-sample shading on an MSAA target), separately.
                // #7 SSAA (variant 2): supersample the atlas by a single multiplier g_rttSS, as in
                // VCock_SetRttCanvas. Zone base = 768; atlas = 768*SS; zones/font *SS. All three
                // consistent -> apparent size preserved, AA via the bilinear downscale of DrawRttQuad.
                resScale       = g_rttSS;
                g_rttFontScale = g_rttSS;
                txRes = (int)(768.0f * g_rttSS);
                tyRes = (int)(768.0f * g_rttSS);

                VirtualDisplay::SetupRttTarget(txRes, tyRes, tBpp);
                bRTTTarget = true;
            }
        }
        else if ( not strcmpi(ptoken, PROP_HUD_STR))   // the hud
        {
            if ( not VCock_SetRttCanvas(&plinePtr, &vHUDrenderer, 1))    // ASSO:
            {

                plinePtr = plinePtr; // Release mode compile warning
                F4Assert("Bad HUD description");
            }
        }
        else if ( not strcmpi(ptoken, PROP_RWR_STR))   //  the rwr
        {
            if ( not VCock_SetRttCanvas(&plinePtr, &vRWRrenderer, 2))   // ASSO:
            {
                plinePtr = plinePtr; // Release mode compile warning
                F4Assert("Bad RWR description");
            }
        }
        else if ( not strcmpi(ptoken, TYPE_DED_STR))   //  the ded
        {
            if ( not VCock_SetRttCanvas(&plinePtr, &vDEDrenderer, 3))   // ASSO:
            {
                plinePtr = plinePtr; // Release mode compile warning
                F4Assert("Bad DED description");
            }
        }
        else if ( not strcmpi(ptoken, PROP_DED_PFL))   //  the pfl
        {
            if ( not VCock_SetRttCanvas(&plinePtr, &vPFLrenderer, 4))   // ASSO:
            {
                plinePtr = plinePtr; // Release mode compile warning
                F4Assert("Bad PFL description");
            }
        }
        else if ( not strcmpi(ptoken, TYPE_MACHASI_STR))   //  the rwr
        {
            if ( not VCock_SetCanvas(&plinePtr, &vcInfo.vMACHrenderer))
            {
                plinePtr = plinePtr; // Release mode compile warning
                F4Assert("Bad MACH description");
            }
        }
        else if ( not strcmpi(ptoken, PROP_MFDLEFT_STR))  // left MFD
        {
            Tpoint ul, ur, ll;
            int tLeft, tTop, tRight, tBottom; // ASSO:
            char cBlend = 'c';
            float cAlpha = 1.0f;
            ptoken = FindToken(&plinePtr, "=;\n");

            if (sscanf(ptoken, "%f %f %f %f %f %f %f %f %f %d %d %d %d %c %f",
                       &ul.x, &ul.y, &ul.z,
                       &ur.x, &ur.y, &ur.z,
                       &ll.x, &ll.y, &ll.z,
                       &tLeft, &tTop, &tRight, &tBottom,
                       &cBlend, &cAlpha) == 9)
            {
                tLeft = tlMFDleft;
                tTop = tlMFDtop;
                tRight = tlMFDright;
                tBottom = tlMFDbottom;
            }

            // #7 SSAA: always scale MFD zones by resScale(=g_rttSS) -- consistent with atlas/font.
            if (tLeft > 1)
                tLeft = (int)(resScale * (float)tLeft);

            if (tRight > 1)
                tRight = (int)(resScale * (float)tRight);

            if (tTop > 1)
                tTop = (int)(resScale * (float)tTop);

            if (tBottom > 1)
                tBottom = (int)(resScale * (float)tBottom);

            lMFDul = ul;
            lMFDur = ur;
            lMFDll = ll;
            ltMFDleft = tLeft;
            ltMFDtop = tTop;
            ltMFDright = tRight;
            ltMFDbottom = tBottom;
            lcMFDblend = cBlend;
            lcMFDalpha = cAlpha;
        }
        else if ( not strcmpi(ptoken, PROP_MFDRIGHT_STR))  // right MFD
        {
            Tpoint ul, ur, ll;
            int tLeft, tTop, tRight, tBottom; // ASSO:
            char cBlend = 'c';
            float cAlpha = 1.0f;
            ptoken = FindToken(&plinePtr, "=;\n");

            if (sscanf(ptoken, "%f %f %f %f %f %f %f %f %f %d %d %d %d %c %f",
                       &ul.x, &ul.y, &ul.z,
                       &ur.x, &ur.y, &ur.z,
                       &ll.x, &ll.y, &ll.z,
                       &tLeft, &tTop, &tRight, &tBottom,
                       &cBlend, &cAlpha) == 9)
            {
                tLeft = trMFDleft;
                tTop = trMFDtop;
                tRight = trMFDright;
                tBottom = trMFDbottom;
            }

            // #7 SSAA: always scale MFD zones by resScale(=g_rttSS) -- consistent with atlas/font.
            if (tLeft > 1)
                tLeft = (int)(resScale * (float)tLeft);

            if (tRight > 1)
                tRight = (int)(resScale * (float)tRight);

            if (tTop > 1)
                tTop = (int)(resScale * (float)tTop);

            if (tBottom > 1)
                tBottom = (int)(resScale * (float)tBottom);

            rMFDul = ul;
            rMFDur = ur;
            rMFDll = ll;
            rtMFDleft = tLeft;
            rtMFDtop = tTop;
            rtMFDright = tRight;
            rtMFDbottom = tBottom;
            rcMFDblend = cBlend;
            rcMFDalpha = cAlpha;
        }
        else if ( not strcmpi(ptoken, TYPE_DIAL_STR))
        {
            VCock_ParseVDial(pcockpitDataFile);
        }
        else if ( not strcmpi(ptoken, PROP_3D_PADBACKGROUND))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &pVColors[0][0]);
            pVColors[1][0] = CalculateNVGColor(pVColors[0][0]);
        }
        else if ( not strcmpi(ptoken, PROP_3D_PADLIFTLINE))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &pVColors[0][1]);
            pVColors[1][1] = CalculateNVGColor(pVColors[0][1]);
        }
        else if ( not strcmpi(ptoken, PROP_3D_PADBOXSIDE))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &pVColors[0][2]);
            pVColors[1][2] = CalculateNVGColor(pVColors[0][2]);
        }
        else if ( not strcmpi(ptoken, PROP_3D_PADBOXTOP))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &pVColors[0][3]);
            pVColors[1][3] = CalculateNVGColor(pVColors[0][3]);
        }
        else if ( not strcmpi(ptoken, PROP_3D_PADTICK))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &pVColors[0][4]);
            pVColors[1][4] = CalculateNVGColor(pVColors[0][4]);
        }
        else if ( not strcmpi(ptoken, PROP_3D_NEEDLE0))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &pVColors[0][5]);
            pVColors[1][5] = CalculateNVGColor(pVColors[0][5]);
        }
        else if ( not strcmpi(ptoken, PROP_3D_NEEDLE1))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &pVColors[0][6]);
            pVColors[1][6] = CalculateNVGColor(pVColors[0][6]);
        }
        else if ( not strcmpi(ptoken, PROP_3D_DED))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &pVColors[0][7]);
            pVColors[1][7] = CalculateNVGColor(pVColors[0][7]);
        }
        else if ( not strcmpi(ptoken, PROP_3D_RWR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &pVColors[0][8]);
            pVColors[1][8] = CalculateNVGColor(pVColors[0][8]);
        }
        else if ( not strcmpi(ptoken, PROP_3D_HILIGHT))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &p3DpitHilite);
        }
        else if ( not strcmpi(ptoken, PROP_3D_LOLIGHT))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &p3DpitLolite);
        }
        else if ( not strcmpi(ptoken, PROP_3D_COCKPIT))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &vrCockpitModel[0]);
        }
        else if ( not strcmpi(ptoken, PROP_3D_COCKPITDF))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &vrCockpitModel[1]);
        }
        else if ( not strcmpi(ptoken, PROP_3D_MAINMODEL))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &vrCockpitModel[2]);
        }
        else if ( not strcmpi(ptoken, PROP_3D_DAMAGEDMODEL))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &vrCockpitModel[3]);
        }
        //JAM 10May04
        else if ( not strcmpi(ptoken, PROP_3D_ZBUFFERING))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &bVCockZBuffering);
        }
        else if ( not strcmpi(ptoken, PROP_LIFT_LINE_COLOR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &liftlinecolor);
        }
        else if ( not strcmpi(ptoken, PROP_3D_BORESIGHT_Y))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%f", &vBoresightY);
        }
        else if ( not strcmpi(ptoken, PROP_3D_USE_NEW_3DPIT))
        {
            int bset = 0;
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &bset);

            if (bset)
                g_bUseNew3dpit = true;
            else
                g_bUseNew3dpit = false;

        }
        else
        {
            F4Assert( not "Unknown Line in 3dfile");
        }
    }

    // Check for missing PFL RTT data
    if ( not vPFLrenderer)
    {
        if ( not VCock_SetRttCanvas(NULL, &vPFLrenderer, 6))
        {
            // ASSO:
            F4Assert("Bad PFL description");
        }
    }

    return true;
}

#if 0
// JPO old fixed version of 3-d cockpit
/*
** InitVirtualCockpit
*/
void
OTWDriverClass::VCock_Init(void)
{
    VDialInitStr vdialInitStr;

    mNumVDials = 5;
    mpVDials = new VDial*[mNumVDials];

    vcInfo.vHUDrenderer = new Canvas3D;
    vcInfo.vHUDrenderer->Setup(renderer);
    vcInfo.vHUDrenderer->SetCanvas(&vHUDul, &vHUDur, &vHUDll);
    vcInfo.vHUDrenderer->Update(&Origin, (struct Trotation *)&IMatrix);

    vcInfo.vRWRrenderer = new Canvas3D;
    vcInfo.vRWRrenderer->Setup(renderer);
    vcInfo.vRWRrenderer->SetCanvas(&vRWRul, &vRWRur, &vRWRll);
    vcInfo.vRWRrenderer->Update(&Origin, (struct Trotation *)&IMatrix);

    vcInfo.vMACHrenderer = new Canvas3D;
    vcInfo.vMACHrenderer->Setup(renderer);
    vcInfo.vMACHrenderer->SetCanvas(&vMACHul, &vMACHur, &vMACHll);
    vcInfo.vMACHrenderer->Update(&Origin, (struct Trotation *)&IMatrix);

    vcInfo.vDEDrenderer = new Canvas3D;
    vcInfo.vDEDrenderer->Setup(renderer);
    vcInfo.vDEDrenderer->SetCanvas(&vDEDul, &vDEDur, &vDEDll);
    vcInfo.vDEDrenderer->Update(&Origin, (struct Trotation *)&IMatrix);

    //---------------------------------->
    // Oil Gauge
    vdialInitStr.callback = 43;
    vdialInitStr.pUL = &vOILul;
    vdialInitStr.pUR = &vOILur;
    vdialInitStr.pLL = &vOILll;
    vdialInitStr.pRender = renderer;
    vdialInitStr.radius = 0.85F;
    vdialInitStr.color = pVColors[0][5];
    vdialInitStr.endPoints = vOILepts;
    vdialInitStr.pvalues = vOILvals;
    vdialInitStr.ppoints = vOILpts;

    mpVDials[0] = new VDial(&vdialInitStr);
    //<----------------------------------

    //---------------------------------->
    // Nozzle Position
    vdialInitStr.callback = 41;
    vdialInitStr.pUL = &vNOZul;
    vdialInitStr.pUR = &vNOZur;
    vdialInitStr.pLL = &vNOZll;
    vdialInitStr.pRender = renderer;
    vdialInitStr.radius = 0.85F;
    vdialInitStr.color = pVColors[0][5];
    vdialInitStr.endPoints = vNOZepts;
    vdialInitStr.pvalues = vNOZvals;
    vdialInitStr.ppoints = vNOZpts;

    mpVDials[1] = new VDial(&vdialInitStr);
    //<----------------------------------

    //---------------------------------->
    // RPM Gauge
    vdialInitStr.callback = 40;
    vdialInitStr.pUL = &vRPMul;
    vdialInitStr.pUR = &vRPMur;
    vdialInitStr.pLL = &vRPMll;
    vdialInitStr.pRender = renderer;
    vdialInitStr.radius = 0.85F;
    vdialInitStr.color = pVColors[0][5];
    vdialInitStr.endPoints = vRPMepts;
    vdialInitStr.pvalues = vRPMvals;
    vdialInitStr.ppoints = vRPMpts;

    mpVDials[2] = new VDial(&vdialInitStr);
    //<----------------------------------

    //---------------------------------->
    // FTIT Indicator
    vdialInitStr.callback = 42;
    vdialInitStr.pUL = &vFTITul;
    vdialInitStr.pUR = &vFTITur;
    vdialInitStr.pLL = &vFTITll;
    vdialInitStr.pRender = renderer;
    vdialInitStr.radius = 0.85F;
    vdialInitStr.color = pVColors[0][5];
    vdialInitStr.endPoints = vFTITepts;
    vdialInitStr.pvalues = vFTITvals;
    vdialInitStr.ppoints = vFTITpts;

    mpVDials[3] = new VDial(&vdialInitStr);
    //<----------------------------------


    //---------------------------------->
    // Altimeter
    vdialInitStr.callback = 44;
    vdialInitStr.pUL = &vALTul;
    vdialInitStr.pUR = &vALTur;
    vdialInitStr.pLL = &vALTll;
    vdialInitStr.pRender = renderer;
    vdialInitStr.radius = 0.85F;
    vdialInitStr.color = pVColors[0][5];
    vdialInitStr.endPoints = vALTepts;
    vdialInitStr.pvalues = vALTvals;
    vdialInitStr.ppoints = vALTpts;

    mpVDials[4] = new VDial(&vdialInitStr);
    //<----------------------------------
}

#endif

// ASSOCIATOR extract desired digit from number
// digit is position of desired number from right to left and starting from 0
inline int ExtractDigit(float number, int digit)
{
    return (int)(number / pow((long)10, (double)digit)) % 10;
}

//ATARIBABY move to target value by defined rate (Borrowed MOVEDOF from surface.cpp);
float OTWDriverClass::MoveByRate(float oldval, float newval, float rate)
{
    float changeval;
    float value = oldval;

    if (value == newval or not gameCompressionRatio) return value; // all done

    changeval = rate * DTR * SimLibMajorFrameTime;

    if (value > newval)
    {
        value -= changeval;

        if (value <= newval)
        {
            value = newval;
        }
    }
    else if (value < newval)
    {
        value += changeval;

        if (value >= newval)
        {
            value = newval;
        }
    }

    return value;
}



// COBRA DX - Red - Head is calculated once for all
void OTWDriverClass::VCock_HeadCalc(void)
{
    if ((mUseHeadTracking) and (g_n6DOFTIR)) // Retro 24Dez2004
    {

        // Use TIR 6 DOF - Cobra
        if (g_n6DOFTIR == 1)
        {
            Tpoint Pan;
            Pan.x = theTrackIRObject.getZ() / 16383.0f * -1.25f; // Cobra - changed from +/-4' to +/-1.75'
            Pan.y = theTrackIRObject.getX() / 16383.0f * -0.50f;
            Pan.z = theTrackIRObject.getY() / 16383.0f * -0.75f;

            // If using DX Engine, orient head with the cockpit/platform, head movements oriented with head rotation
            if (g_bUse_DX_Engine)
            {
                MatrixMult(&headMatrix, &Pan, &headPan);
                MatrixMult(&OTWDriver.ownshipRot, &headPan, &headOrigin);
            }
            else headOrigin = headPan;
        }
        else // g_n6DOFTIR = 2 - Hold Viewpoint at 0,0,0 and use FOV zoom for forward/back movement - Cobra
        {
            float x = 0.0f;
            float fov = 0.0f;
            headOrigin.x = headOrigin.y = headOrigin.z = 0.0f;
            headPan = headOrigin;

            x = -(theTrackIRObject.getZ() / 16383.0f); // +/-1.0

            if (x > 0.0f)
                fov = g_fDefaultFOV - (x * (g_fDefaultFOV - g_fTIRMinimumFOV));
            else if (x < 0.0f)
                fov = g_fDefaultFOV + (-x * (g_fTIRMaximumFOV - g_fDefaultFOV));
            else
                fov = g_fDefaultFOV;

            OTWDriver.SetFOV(fov * DTR);

            if (fov == g_fDefaultFOV)
                narrowFOV = FALSE;
            else
                narrowFOV = TRUE;
        }
    }
    else // Retro 24Dez2004
    {
        //ATARIBABY start new dynamic head movement more like old DID EF2000 days :-)
        if (g_b3dDynamicPilotHead)
        {
            float actualtilt = 0.0f;	// FIX: assigned only when dt!=0, else RTC uninitialized
            float actualrollrate = 0.0f;
            float actualpan = 0.0f;
            //ATARIBABY disabled now - fwd/back lean cause normals problems and i not know solution yet
            //float actualaccel;

            //ATARIBABY disabled now - fwd/back lean cause normals problems and i not know solution yet
            //Tpoint origin = {0.0, 0.0, 0.0};

            float dt = (vuxGameTime - BobbingPreviousTime) / 2000.0f;

            if (dt)
            {
                //ATARIBABY disabled now - fwd/back lean cause normals problems and i not know solution yet
                //get accel in X axis (forward/backward accel)
                //actualaccel = -SimDriver.GetPlayerAircraft()->af->nxcgb;

                //compute actual tilt change
                actualtilt = ((cockpitFlightData.gs - 1.0F) * 0.015F) * g_fDyn_Head_TiltMul;
                // with higher Gs add more random shaking
                int gs = (int)cockpitFlightData.gs;
                gs = gs - 1;

                if (gs not_eq 0)
                    actualtilt = actualtilt + (((rand() % gs) / 2000.0F) * g_fDyn_Head_TiltRndGMul);
                else
                    actualtilt = 0;

                //compute actual roll change
                actualrollrate = cockpitFlightData.roll - BobbingPreviousRoll;

                if (actualrollrate > PI)
                    actualrollrate = -(PI - cockpitFlightData.roll + PI + BobbingPreviousRoll);
                else if (actualrollrate < -PI)
                    actualrollrate = PI + cockpitFlightData.roll + PI - BobbingPreviousRoll;

                actualrollrate = (actualrollrate * 0.15F) * g_fDyn_Head_RollMul;
                //compute actual pan
                actualpan = cockpitFlightData.yaw - BobbingPreviousPan;

                if (actualpan > PI)
                    actualpan = -(PI - cockpitFlightData.yaw + PI + BobbingPreviousPan);
                else if (actualpan < -PI)
                    actualpan = PI + cockpitFlightData.yaw + PI - BobbingPreviousPan;

                actualpan = actualpan * g_fDyn_Head_PanMul;

                // my old crappy execution
                //Head roll move damping
                // if (actualrollrate < BobbingRollRate and BobbingRollRate > -0.5F )
                // BobbingRollRate = BobbingRollRate - 0.001F;
                // if (actualrollrate > BobbingRollRate and BobbingRollRate < 0.5F)
                // BobbingRollRate = BobbingRollRate + 0.001F;
                //Head tilt move damping
                // if (actualtilt < BobbingTilt and BobbingTilt > -0.5F)
                // BobbingTilt = BobbingTilt - 0.001F;
                // if (actualtilt > BobbingTilt and BobbingTilt < 0.5F)
                // BobbingTilt = BobbingTilt + 0.001F;
                //Head pan move damping
                // if (actualpan < BobbingPan and BobbingPan > -0.5F)
                // BobbingPan = BobbingPan - 0.001F;
                // if (actualpan > BobbingPan and BobbingPan < 0.5F)
                // BobbingPan = BobbingPan + 0.001F;

                BobbingPreviousTime = vuxGameTime;
            }

            BobbingPreviousRoll = cockpitFlightData.roll;
            BobbingPreviousPan = cockpitFlightData.yaw;

            //better execution
            BobbingTilt = MoveByRate(BobbingTilt, actualtilt, ((abs(cockpitFlightData.gs) + 1.0F) * g_fDyn_Head_TiltGRateMul) * g_fDyn_Head_TiltRateMul); //tilt rate is G's sensitive
            BobbingTilt = max(-0.13F, min(0.13f, BobbingTilt));
            BobbingRollRate = MoveByRate(BobbingRollRate, actualrollrate, g_fDyn_Head_RollRate); //change this to alter roll speed
            BobbingRollRate = max(-0.08F, min(0.08f, BobbingRollRate));
            BobbingPan = MoveByRate(BobbingPan, actualpan, g_fDyn_Head_PanRate); //change this to alter pan speed
            BobbingPan = max(-0.10F, min(0.10f, BobbingPan));
            //ATARIBABY disabled now - fwd/back lean cause normals problems and i not know solution yet
            //BobbingAccel = MoveByRate(BobbingAccel,actualaccel, 30.0f); //change this to alter accel speed

            //ATARIBABY disabled now - fwd/back lean cause normals pro blems and i not know solution yet
            //origin.x = max(-0.9F,min(0.9f,BobbingAccel * 2.0f));
            //origin.y = 0.0;
            //origin.z = 0.0;

            //ATARIBABY disabled now - fwd/back lean cause normals problems and i not know solution yet
            //renderer->SetCamera( &origin, &headMatrix );

            headOrigin = headPan = Origin;
        }
        else
            headOrigin = headPan = Origin;

        //ATARIBABY end
    } // Retro 24Dez2004


    // COBRA - RED - Introduced Airframe Vibrations
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (playerAC)
    {
        PitTurbulence = playerAC->GetTurbulence();
        Tpoint Ho;
        headPan.x += PitTurbulence.x;
        headPan.y += PitTurbulence.y;
        headPan.z += PitTurbulence.z;
        MatrixMult(&OTWDriver.ownshipRot, &PitTurbulence, &Ho);
        headOrigin.x += Ho.x;
        headOrigin.y += Ho.y;
        headOrigin.z += Ho.z;
    }

    // VR head-tracking (independent of TrackIR / mUseHeadTracking): override the head look
    // angles from the HMD orientation. Runs every 3D frame here, just before the pit draw
    // (VCock_DrawThePit) builds the head matrix from eyePan/eyeTilt/eyeHeadRoll.
    {
        // Gate on g_bVrFrameActive (presenting stereo this frame), not g_bUseOpenXR (enabled in options):
        // with the headset OFF the flat view must keep mouse/TrackIR head control, not stale HMD angles.
        extern bool g_bVrFrameActive;
        float vy, vp, vr;
        if (g_bVrFrameActive && g_pOpenXRBackend &&
            g_pOpenXRBackend->GetHeadYawPitchRoll(&vy, &vp, &vr))
        {
            eyePan = vy;
            eyeTilt = vp;
            eyeHeadRoll = vr;
            // The active 3D-pit head path (VCock_DrawThePit dynamic-head branch) reads roll
            // from BobbingRollRate (not eyeHeadRoll) and ADDS BobbingPan/Tilt to the look.
            // Drive roll through it and zero the bob so the HMD is the sole head motion.
            BobbingRollRate = vr;
            BobbingPan = 0.0f;
            BobbingTilt = 0.0f;

            // Build the head matrix DIRECTLY from the HMD orientation basis (gimbal-free) instead of
            // BuildHeadMatrix(yaw,pitch,roll): the latter derives 'right' from (0,0,1) x forward, which
            // divides by zero at pitch +-90 (look straight down at your feet) and wanders near the
            // poles -> the old "can't look below / angles jump" limit. Load [at|right|up] as the head
            // matrix columns, then derive the world camera (cameraRot = ownshipRot*headMatrix) so the
            // cockpit (drawn with headMatrix) and the world stay locked together this frame.
            // Artscout - 2026 (VR quad-views): in quad mode the FOCUS views (2,3) are GAZE-tracked --
            // their pose looks where the eyes look, not straight ahead. Render each view from ITS OWN
            // orientation (GetEyeBasis) so it matches the submitted view pose; otherwise the focus inset
            // doubles and the cockpit/displays follow the gaze. Stereo (flag off) keeps the shared head
            // basis (proven), since both eyes share the head orientation there.
            int xeye = g_pOpenXRBackend->CurrentEye();
            float hAt[3], hRt[3], hUp[3];
            // All views share the HEAD orientation (the runtime returns the same pose orientation for
            // periphery and focus -- the gaze/cant is in the off-center FOV, handled by the off-axis
            // projection, not by rotating the pose). So build the head matrix from the head basis.
            bool gotBasis = g_pOpenXRBackend->GetHeadBasis(hAt, hRt, hUp);
            if (gotBasis)
            {
                headMatrix.M11 = hAt[0]; headMatrix.M21 = hAt[1]; headMatrix.M31 = hAt[2];
                headMatrix.M12 = hRt[0]; headMatrix.M22 = hRt[1]; headMatrix.M32 = hRt[2];
                headMatrix.M13 = hUp[0]; headMatrix.M23 = hUp[1]; headMatrix.M33 = hUp[2];
            }
            else
                BuildHeadMatrix(FALSE, YAW_PITCH, eyePan, eyeTilt, eyeHeadRoll);
            MatrixMult(&ownshipRot, &headMatrix, &cameraRot);

            // 6DOF head position (lean in/out/sideways) + per-eye IPD, accumulated in the aircraft
            // BODY frame, then mirrored into WORLD via ownshipRot -- EXACTLY the engine's headPan /
            // headOrigin convention (see the turbulence block below: headPan += body, headOrigin +=
            // ownshipRot*body). headPan feeds the RTT cockpit displays (VCock_Exec: Pan = headPan *
            // RTT_POSITION_SCALING), headOrigin feeds the cockpit/world eyepoint. Putting the offset
            // ONLY in headOrigin (previous attempt) left the HUD/MFD/DED/RWR drifting with the gaze
            // and without per-eye parallax. Body frame (not head-relative) = absolute HMD tracking.
            // Artscout - 2026 (VR): split head-translation from the per-eye IPD. The RTT cockpit
            // displays read Pan = headPan * RTT_POSITION_SCALING(10.35), so ANY IPD left in headPan is
            // amplified ~10x and the HUD/MFD/DED/RWR diverge per eye (left eye->left, right->right --
            // the long-standing "RWR depth" bug). The natural per-eye parallax for those fixed-distance
            // panels already comes from the camera eyepoint (headOrigin gets the IPD). So: head lean ->
            // headPan + headOrigin (displays follow lean); IPD -> headOrigin ONLY (camera parallax, no
            // 10x display divergence).
            Tpoint posHead; posHead.x = posHead.y = posHead.z = 0.0f;   // head translation (lean)
            float hpf, hpr, hpd;
            if (g_pOpenXRBackend->GetHeadPosFeet(&hpf, &hpr, &hpd)) { posHead.x += hpf; posHead.y += hpr; posHead.z += hpd; }
            Tpoint posCam = posHead;                                    // camera also gets the IPD
            if (xeye >= 0) posCam.y += g_pOpenXRBackend->GetEyeLateralOffsetFeet(xeye);  // IPD on body right axis
            headPan.x += posHead.x; headPan.y += posHead.y; headPan.z += posHead.z;       // displays: lean only
            Tpoint posW; MatrixMult(&OTWDriver.ownshipRot, &posCam, &posW);
            headOrigin.x += posW.x; headOrigin.y += posW.y; headOrigin.z += posW.z;        // camera: lean + IPD
        }
    }
}


void OTWDriverClass::CockAttachWeapons(void)
{
    int stationNum;
    SMSClass *sms = SimDriver.GetPlayerAircraft()->Sms;
    DrawableBSP* child;

    for (stationNum = 1; stationNum < sms->NumHardpoints(); stationNum++)
    {
        // MLR 2/20/2004 - new rack code, compatible with SP3 still
        child = sms->hardPoint[stationNum]->GetTopDrawable();

        if (child) vrCockpit->AttachChild(child, stationNum - 1);
    }
}


void OTWDriverClass::CockDetachWeapons(void)
{
    int stationNum;
    SMSClass *sms = SimDriver.GetPlayerAircraft()->Sms;
    DrawableBSP* child;

    for (stationNum = 1; stationNum < sms->NumHardpoints(); stationNum++)
    {
        // MLR 2/20/2004 - new rack code, compatible with SP3 still
        child = sms->hardPoint[stationNum]->GetTopDrawable();

        if (child) vrCockpit->DetachChild(child, stationNum - 1);
    }
}

// COBRA DX - Red - The pit is draw in another call - OTW stuff (not cockpit stuff)
void OTWDriverClass::VCock_DrawThePit(void)
{
    int oldState;

    // COBRA - DX - if using DX Engine, PIT has to be Oriented as in 3D WORLD SPACE
    vrCockpit->orientation = OTWDriver.ownshipRot;
    renderer->SetCamera(&headOrigin, &headMatrix);
    oldState = renderer->GetObjectTextureState();
    renderer->SetObjectTextureState(TRUE);
    // Artscout - 2026: #72 the cockpit-fidelity pass (FF_COCKPIT) is NOT toggled here: the pit is only
    // QUEUED at this point and flushed later (otwloop FlushPolyLists), by when a flag set here is already
    // stale. It is driven instead from CDXEngine::SetPitMode, which the VB manager calls at FLUSH time
    // per pit object (dxvbmanager.cpp:819) -> the flag is live exactly when the pit surfaces draw.
    vrCockpit->Draw(renderer);
    renderer->SetObjectTextureState(oldState);
}



/*
** DoVirtualCockpit
*/

float HudScale = 4.0f;
float CXX = 1.0f, CXY = 1.0f;

// SCALING FOR OFFSETTING THE RTTs IN THE PIT
#define RTT_POSITION_SCALING 10.35f
// SCALING FOR OFFSETTING THE 3D BUTTONS IN THE PIT
#define B3D_POSITION_SCALING 569.0f

// Artscout - 2026 (VR): magnetic 3D-cursor anchor. The clickable-cockpit hit-test below finds the
// nearest 3D button to the pointer; we store that button's camera-centric position so the per-eye
// overlay (otwloop) can project it into BOTH eyes and draw the cursor AT the switch with correct
// stereo depth -- a flat 2D cursor at one screen position sits at infinity and visually doubles, and
// can't be aimed accurately. The cockpit coordinate scale is unknown (B3D_POSITION_SCALING=569, not
// feet), so we anchor to a KNOWN button position instead of unprojecting the mouse at a guessed depth.
// This same primitive (ray -> nearest button -> 3D point) will drive Touch-controller aiming later,
// fed by the controller aim pose instead of the mouse ray.
Tpoint g_vrCursorAnchor = { 0.0f, 0.0f, 0.0f };
bool   g_vrCursorAnchorValid = false;
// Artscout - 2026 (VR mouse): true when the anchor is SNAPPED to a real button (exact depth) vs a FREE
// cursor at a guessed panel depth. The free cursor is only safe in the full-FOV periphery views; in the
// zoomed gaze/focus views its depth-guess stereo error is magnified (jumps, eye mismatch), so otwloop
// draws the free cursor in periphery only and the snapped cursor (correct depth) in all views.
bool   g_vrCursorAnchorSnapped = false;
// Artscout - 2026 (VR mouse): index of the button the cursor is magnetically snapped to (the GREEN one).
// On click we fire THIS button directly instead of re-projecting in the click loop -- the click happens
// while the mouse is still, a frame or two after the last hover, by which time the gaze/camera has moved
// and a fresh projection lands elsewhere (ey way off). Firing the snapped button = clicking what you see.
int    g_vrCursorAnchorButton = -1;

// Artscout - 2026: multi-position rotary switch groups. Each F-16 selector (MASTER ARM, MAIN PWR, RF, INS,
// the HUD display-mode wafers, etc.) is authored in 3dbuttons.dat as one setter function per position, each at
// its own tiny loc. With a VR laser / mouse this is fiddly. We collapse every group to ONE hotspot (its centroid)
// and CYCLE through the positions: LMB / thumbstick-up = next, RMB / thumbstick-down = prev. Position order is the
// physical rotary order below. Names must match 3dbuttons.dat exactly; unresolved names are skipped. Runtime cfg
// "VrSwitchGroups" (g_bVrSwitchGroups) turns the whole thing off (each position stays its own spot). Defined here
// (above VCock_Exec) so both the click handler and Button3D_Init can see the table.
struct VrSwitchGroupDef { const char* name; const char* funcs[6]; int count; };
static const VrSwitchGroupDef s_switchGroupDefs[] =
{
    { "MasterArm",   { "SimSafeMasterArm", "SimArmMasterArm", "SimSimMasterArm" }, 3 },
    { "RF",          { "SimRFNorm", "SimRFQuiet", "SimRFSilent" }, 3 },
    { "HSIMode",     { "SimHSIIlsTcn", "SimHSITcn", "SimHSINav", "SimHSIIlsNav" }, 4 },
    { "FuelSel",     { "SimFuelSwitchTest", "SimFuelSwitchNorm", "SimFuelSwitchResv", "SimFuelSwitchWingInt", "SimFuelSwitchWingExt", "SimFuelSwitchCenterExt" }, 6 },
    { "RightAP",     { "SimRightAPUp", "SimRightAPMid", "SimRightAPDown" }, 3 },
    { "LeftAP",      { "SimLeftAPUp", "SimLeftAPMid", "SimLeftAPDown" }, 3 },
    { "EWSMode",     { "SimEWSModeOff", "SimEWSModeStby", "SimEWSModeMan", "SimEWSModeSemi", "SimEWSModeAuto" }, 5 },
    { "EWSProg",     { "SimEWSProgOne", "SimEWSProgTwo", "SimEWSProgThree", "SimEWSProgFour" }, 4 },
    { "AVTR",        { "SimAVTRSwitchOff", "SimAVTRSwitchAuto", "SimAVTRSwitchOn" }, 3 },
    { "MainPower",   { "SimMainPowerOff", "SimMainPowerBatt", "SimMainPowerMain" }, 3 },
    { "Epu",         { "SimEpuOff", "SimEpuAuto", "SimEpuOn" }, 3 },
    { "FuelPump",    { "SimFuelPumpOff", "SimFuelPumpNorm", "SimFuelPumpAft", "SimFuelPumpFwd" }, 4 },
    { "RALT",        { "SimRALTOFF", "SimRALTSTDBY", "SimRALTON" }, 3 },
    { "Scales",      { "SimScalesOff", "SimScalesVAH", "SimScalesVVVAH" }, 3 },
    { "PitchLadder", { "SimPitchLadderOff", "SimPitchLadderFPM", "SimPitchLadderATTFPM" }, 3 },
    { "HUDDED",      { "SimHUDDEDOff", "SimHUDDEDPFL", "SimHUDDEDDED" }, 3 },
    { "Reticle",     { "SimReticleOff", "SimReticleStby", "SimReticlePri" }, 3 },
    { "HUDVel",      { "SimHUDVelocityCAS", "SimHUDVelocityTAS", "SimHUDVelocityGND" }, 3 },
    { "HUDAlt",      { "SimHUDAltRadar", "SimHUDAltBaro", "SimHUDAltAuto" }, 3 },
    { "HUDBrt",      { "SimHUDBrtDay", "SimHUDBrtAuto", "SimHUDBrtNight" }, 3 },
    { "AirSource",   { "SimAirSourceOff", "SimAirSourceNorm", "SimAirSourceDump", "SimAirSourceRam" }, 4 },
    { "INS",         { "SimINSOff", "SimINSNorm", "SimINSNav", "SimINSInFlt" }, 4 },
};
static const int s_numSwitchGroups = (int)(sizeof(s_switchGroupDefs) / sizeof(s_switchGroupDefs[0]));
int  g_vrSwitchGroupCur[64];              // current position index per group (state; cycled on click)
static int  s_switchGroupMember[64][6];   // Button3DList index of each resolved position (-1 = missing)

// Artscout - 2026 (VR controllers): laser-ray state. g_vrRayOrigin -> g_vrCursorAnchor is the green line
// (drawn per-eye next to the cursor). g_vrCtrlRayActive tells the mouse hover/anchor blocks to stand down
// this frame (the controller drives the pick). g_bVrZoomActive = A-button toggle (zoom behaviour TBD).
Tpoint g_vrRayOrigin = { 0.0f, 0.0f, 0.0f };
Tpoint g_vrRayDir    = { 0.0f, 0.0f, 1.0f };   // aim direction (body, unit) -- forward axis of the controller model
Tpoint g_vrGripPoint = { 0.0f, 0.0f, 0.0f };   // controller grip position (marker), body button-units
bool   g_vrGripValid = false;
bool   g_vrRayActive = false;
// Artscout - 2026 (VR controllers): the ACTUAL selected-button center (headPan-adjusted, button units),
// captured in the view-0 pick. When a button is under the ray we draw a RING at THIS point (the switch that
// will be clicked) instead of a cross at the ray's free endpoint -- so the highlight sits on the real switch
// (kills the "beam-end vs detected-button" mismatch), and shows a CROSS at the free endpoint otherwise.
Tpoint g_vrHitPoint = { 0.0f, 0.0f, 0.0f };
bool   g_vrHitValid = false;
bool   g_vrCtrlRayActive = false;
bool   g_bVrZoomActive = false;

// ===================== VR controller MODEL (v1: shaded solid OBJ, no texture) =====================
// Artscout - 2026: draw the real controller mesh (SteamVR/Oculus render models, bundled as OBJ under
// art\ckptart\controllers\) oriented by the grip pose, as the middle tier of the "hands -> model ->
// wireframe" chain. v1 = flat-shaded solid (STATE_GOURAUD, geometry face normals) so we skip texture
// (DDS) loading; v2 will add the diffuse texture. Rendered per-eye via DrawTriangle (depth-tested).
#include <vector>
#include <string.h>
struct VrTri { float p[3][3]; float n[3]; float uv[3][2]; };   // 3 verts (local) + face normal + UVs
struct VrModelCache { bool tried; bool ok; char key[64]; char tex[128]; bool srvTried; void* srv; std::vector<VrTri> tris; };
static VrModelCache s_vrModel[2];                      // per hand (L/R meshes differ)

// Parse an OBJ's .mtl for the map_Kd diffuse texture BASENAME (into out). Empty if none. (v2 textures.)
static void VrParseMtlTex(const char* mtlName, char* out, int cap)
{
    out[0] = 0;
    char path[256]; sprintf(path, "%scontrollers\\%s", COCKPIT_DIR, mtlName);
    FILE* f = fopen(path, "r"); if (!f) return;
    char line[512];
    while (fgets(line, sizeof(line), f))
    {
        char tok[256];
        if (sscanf(line, " map_Kd %255s", tok) == 1)
        {
            char* b = tok; for (char* p = tok; *p; ++p) if (*p == '/' or *p == '\\') b = p + 1;   // strip path
            strncpy(out, b, cap - 1); out[cap - 1] = 0;
            break;
        }
    }
    fclose(f);
}

// Load + cache the OBJ (once). Parses v/vt/f (quads fan-triangulated) with per-vertex UV, the mtllib's map_Kd
// diffuse texture, and a per-face geometry normal (shading). Positions/UVs are model-local.
static bool VrLoadCtrlObj(VrModelCache* m, const char* baseName)
{
    if (m->tried and strcmp(m->key, baseName) == 0) return m->ok;   // already resolved this model
    m->tried = true; m->ok = false; m->tris.clear(); m->tex[0] = 0; m->srvTried = false; m->srv = 0;
    strncpy(m->key, baseName, sizeof(m->key) - 1); m->key[sizeof(m->key) - 1] = 0;

    char path[256];
    sprintf(path, "%scontrollers\\%s.obj", COCKPIT_DIR, baseName);
    FILE* f = fopen(path, "r");
    if (!f)
    {
        char db[300]; sprintf(db, "VR ctrl model: cannot open %s\n", path);
        OutputDebugStringA(db); FILE* d = fopen("vrmodel_diag.txt", "a"); if (d) { fputs(db, d); fclose(d); }
        return false;
    }

    std::vector<float> vp;   // positions x,y,z
    std::vector<float> vt;   // texcoords u,v
    char line[512];
    while (fgets(line, sizeof(line), f))
    {
        if (line[0] == 'v' and line[1] == ' ')
        {
            float x, y, z;
            if (sscanf(line + 2, "%f %f %f", &x, &y, &z) == 3) { vp.push_back(x); vp.push_back(y); vp.push_back(z); }
        }
        else if (line[0] == 'v' and line[1] == 't')
        {
            float u = 0.0f, v = 0.0f;
            if (sscanf(line + 3, "%f %f", &u, &v) >= 1) { vt.push_back(u); vt.push_back(v); }
        }
        else if (strncmp(line, "mtllib", 6) == 0)
        {
            char mn[128]; if (sscanf(line + 6, " %127s", mn) == 1) VrParseMtlTex(mn, m->tex, sizeof(m->tex));
        }
        else if (line[0] == 'f' and line[1] == ' ')
        {
            int pidx[16], tidx[16], cnt = 0;
            char* tok = strtok(line + 2, " \t\r\n");
            while (tok and cnt < 16)
            {
                int pi = 0, ti = 0;
                if (sscanf(tok, "%d/%d", &pi, &ti) >= 1 and pi != 0) { pidx[cnt] = pi; tidx[cnt] = ti; cnt++; }
                tok = strtok(NULL, " \t\r\n");
            }
            int nv = (int)(vp.size() / 3);
            int nt = (int)(vt.size() / 2);
            for (int i = 1; i + 1 < cnt; ++i)               // fan-triangulate the face
            {
                const int fa[3] = { 0, i, i + 1 };
                VrTri tr; bool bad = false;
                for (int k = 0; k < 3; ++k)
                {
                    int a = pidx[fa[k]]; a = (a > 0) ? a - 1 : nv + a;
                    if (a < 0 or a >= nv) { bad = true; break; }
                    tr.p[k][0] = vp[a*3]; tr.p[k][1] = vp[a*3+1]; tr.p[k][2] = vp[a*3+2];
                    int t = tidx[fa[k]]; t = (t > 0) ? t - 1 : (t < 0 ? nt + t : -1);
                    if (t >= 0 and t < nt) { tr.uv[k][0] = vt[t*2]; tr.uv[k][1] = 1.0f - vt[t*2+1]; }   // OBJ V -> D3D V (flip)
                    else { tr.uv[k][0] = 0.0f; tr.uv[k][1] = 0.0f; }
                }
                if (bad) continue;
                float ux = tr.p[1][0]-tr.p[0][0], uy = tr.p[1][1]-tr.p[0][1], uz = tr.p[1][2]-tr.p[0][2];
                float wx = tr.p[2][0]-tr.p[0][0], wy = tr.p[2][1]-tr.p[0][1], wz = tr.p[2][2]-tr.p[0][2];
                tr.n[0] = uy*wz - uz*wy; tr.n[1] = uz*wx - ux*wz; tr.n[2] = ux*wy - uy*wx;
                float ln = sqrtf(tr.n[0]*tr.n[0] + tr.n[1]*tr.n[1] + tr.n[2]*tr.n[2]);
                if (ln > 1e-9f) { tr.n[0]/=ln; tr.n[1]/=ln; tr.n[2]/=ln; }
                m->tris.push_back(tr);
            }
        }
    }
    fclose(f);
    m->ok = !m->tris.empty();
    {
        char db[220]; sprintf(db, "VR ctrl model %s: %d tris, tex='%s', ok=%d\n", baseName, (int)m->tris.size(), m->tex, (int)m->ok);
        OutputDebugStringA(db); FILE* d = fopen("vrmodel_diag.txt", "a"); if (d) { fputs(db, d); fclose(d); }
    }
    return m->ok;
}

// Artscout - 2026 (VR controller model): draw the controller mesh into the cockpit polygon list. Called from
// VCock_DrawThePit (per eye), BEFORE otwloop's FlushPolyLists, so DrawTriangle's queued polys flush with the
// pit -- the proven, batched, depth-tested solid path (in VCock_Exec the list was already flushed -> nothing
// drew). Grip pose/basis fetched fresh (VCock_Exec's pick hasn't run yet). Index/Oculus by interaction profile.
void OTWDriverClass::VCock_DrawControllerModel(void)
{
    extern bool  g_bVrControllerModel, g_bVrRayFlipH, g_bVrRayFlipV;
    extern float g_fVrModelScale, g_fVrRayIpd;
    if (not g_bVrControllerModel or g_pOpenXRBackend == NULL or not g_pOpenXRBackend->ControllerActive()) return;

    extern bool  g_bVrUseHands, g_bVrModelOpaque;
    extern float g_fVrModelCull;
    const float sc = B3D_POSITION_SCALING;

    int curEye = g_pOpenXRBackend->CurrentEye(); if (curEye < 0) curEye = 0;
    float ipdY = g_pOpenXRBackend->GetEyeLateralOffsetFeet(curEye) * sc * g_fVrRayIpd;
    const float M2B = 3.28084f * sc * g_fVrModelScale;          // metres -> button units
    float Lx = 0.3f, Ly = -0.5f, Lz = -0.8f; float Ll = sqrtf(Lx*Lx + Ly*Ly + Lz*Lz); Lx/=Ll; Ly/=Ll; Lz/=Ll;

    // Runtime mesh orientation (VrModelYaw/Pitch/Roll, deg): rotate the model in its OWN frame so any asset
    // (controller/hands) aligns to the grip pose without a rebuild. R = Ry(yaw)*Rx(pitch)*Rz(roll), applied
    // to every local vertex + normal below.
    extern float g_fVrModelYaw, g_fVrModelPitch, g_fVrModelRoll;
    float cy = (float)cos(g_fVrModelYaw*DTR),  sy = (float)sin(g_fVrModelYaw*DTR);
    float cp = (float)cos(g_fVrModelPitch*DTR), sp = (float)sin(g_fVrModelPitch*DTR);
    float cr = (float)cos(g_fVrModelRoll*DTR),  sr = (float)sin(g_fVrModelRoll*DTR);
    float R00 = cy*cr + sy*sp*sr, R01 = -cy*sr + sy*sp*cr, R02 = sy*cp;
    float R10 = cp*sr,            R11 = cp*cr,             R12 = -sp;
    float R20 = -sy*cr + cy*sp*sr, R21 = sy*sr + cy*sp*cr, R22 = cy*cp;

    // Draw BOTH hands, each with its OWN mesh (left->*_left, right->*_right) at its OWN grip pose. The ray/
    // cursor is NOT tied to the model -- it stays on the active hand (GetActiveHand, switched by grip squeeze)
    // in the pick block. A hand whose controller isn't tracked this frame is simply skipped.
    static std::vector<D3D11Backend::VrTriVtx> sv;
    for (int hnd = 0; hnd < 2; ++hnd)   // 0 = left, 1 = right
    {
        float go[3], bf[3], br[3], bu[3];
        if (not g_pOpenXRBackend->GetControllerGripBody(hnd, go))  continue;   // this hand not tracked
        if (not g_pOpenXRBackend->GetControllerGripBasis(hnd, bf, br, bu)) continue;
        if (g_bVrRayFlipH) { go[1] = -go[1]; bf[1] = -bf[1]; br[1] = -br[1]; bu[1] = -bu[1]; }
        if (g_bVrRayFlipV) { go[2] = -go[2]; bf[2] = -bf[2]; br[2] = -br[2]; bu[2] = -bu[2]; }
        Tpoint gp; gp.x = go[0] * sc; gp.y = go[1] * sc; gp.z = go[2] * sc;

        const char* base;
        if (g_bVrUseHands)                       // hand/glove meshes (art\ckptart\controllers\glove_left/right.obj)
            base = (hnd == 0) ? "glove_left" : "glove_right";
        else
        {
            char prof[128] = "";
            g_pOpenXRBackend->GetInteractionProfile(hnd, prof, sizeof(prof));
            bool isIndex = (prof[0] == 0) or (strstr(prof, "index") != NULL) or (strstr(prof, "knuckles") != NULL);
            base = (hnd == 0)
                ? (isIndex ? "valve_controller_knu_1_0_left"  : "oculus_cv1_controller_left")
                : (isIndex ? "valve_controller_knu_1_0_right" : "oculus_cv1_controller_right");
        }
        if (not VrLoadCtrlObj(&s_vrModel[hnd], base) or s_vrModel[hnd].tris.empty()) continue;

        // Build a screen-space TRIANGLE LIST (CPU-projected, flat-shaded) and hand it to the D3D11 renderer's
        // direct colour-tri path -- straight into the current eye RTV, depth-off overlay. This bypasses the legacy
        // poly-list / 2D-immediate paths that never reached the eye for our mesh.
        sv.clear();
        const VrTri* T = &s_vrModel[hnd].tris[0];
        const int    nT = (int)s_vrModel[hnd].tris.size();
        for (int ti = 0; ti < nT; ++ti)
        {
            const VrTri& tr = T[ti];
            ThreeDVertex vv[3]; bool ok = true;
            for (int k = 0; k < 3; ++k)
            {
                float lx = tr.p[k][0], ly = tr.p[k][1], lz = tr.p[k][2];
                float mx = R00*lx + R01*ly + R02*lz, my = R10*lx + R11*ly + R12*lz, mz = R20*lx + R21*ly + R22*lz;
                Tpoint wp;
                wp.x = gp.x + (br[0]*mx + bu[0]*my + bf[0]*mz) * M2B;
                wp.y = gp.y + (br[1]*mx + bu[1]*my + bf[1]*mz) * M2B + ipdY;
                wp.z = gp.z + (br[2]*mx + bu[2]*my + bf[2]*mz) * M2B;
                renderer->TransformCameraCentricPoint(&wp, &vv[k]);
                if (vv[k].csZ >= -1.0f) ok = false;   // behind the eye
            }
            if (not ok) continue;
            float nmx = R00*tr.n[0] + R01*tr.n[1] + R02*tr.n[2];
            float nmy = R10*tr.n[0] + R11*tr.n[1] + R12*tr.n[2];
            float nmz = R20*tr.n[0] + R21*tr.n[1] + R22*tr.n[2];
            float nwx = br[0]*nmx + bu[0]*nmy + bf[0]*nmz;
            float nwy = br[1]*nmx + bu[1]*nmy + bf[1]*nmz;
            float nwz = br[2]*nmx + bu[2]*nmy + bf[2]*nmz;
            float d = nwx*Lx + nwy*Ly + nwz*Lz; if (d < 0) d = -d;
            float sh = 0.55f + 0.45f*d; if (sh > 1.0f) sh = 1.0f;   // brighter floor so the textured hands aren't too dark
            unsigned rr = (unsigned)(0.62f*sh*255.0f), gg = (unsigned)(0.64f*sh*255.0f), bb = (unsigned)(0.68f*sh*255.0f);
            unsigned col = 0xFF000000u | (rr << 16) | (gg << 8) | bb;   // ARGB, opaque
            D3D11Backend::VrTriVtx t0 = { vv[0].x, vv[0].y, col, tr.uv[0][0], tr.uv[0][1] };
            D3D11Backend::VrTriVtx t1 = { vv[1].x, vv[1].y, col, tr.uv[1][0], tr.uv[1][1] };
            D3D11Backend::VrTriVtx t2 = { vv[2].x, vv[2].y, col, tr.uv[2][0], tr.uv[2][1] };
            sv.push_back(t0); sv.push_back(t1); sv.push_back(t2);
        }
        // Lazy-load the diffuse texture (once) from the OBJ's mtllib map_Kd; NULL -> flat vertex-colour (v1 look).
        VrModelCache* mc = &s_vrModel[hnd];
        if (not mc->srvTried)
        {
            mc->srvTried = true;
            if (mc->tex[0] and g_pD3D11Backend) { char tp[256]; sprintf(tp, "%scontrollers\\%s", COCKPIT_DIR, mc->tex); mc->srv = g_pD3D11Backend->LoadModelTexture(tp); }
        }
        if (g_pD3D11Backend and not sv.empty())
            g_pD3D11Backend->DrawVrModelTris(&sv[0], (int)sv.size(), mc->srv, g_bVrModelOpaque ? 1 : 0, (int)g_fVrModelCull);
    }
}

void OTWDriverClass::VCock_Exec(void)
{
#if 1
    renderer->ChangeFontSet(&VirtualDisplay::Font3D);   // ASFO:

    int i;
    PlayerRwrClass *rwr;
    float x1, y1, x2, y2;
    mlTrig trig;
    SMSClass *sms = SimDriver.GetPlayerAircraft()->Sms;
    int oldFont = VirtualDisplay::CurFont();

    // Make sure we don't get in here when we shouldn't
    ShiAssert(otwPlatform);
    ShiAssert(otwPlatform->IsSetFlag(MOTION_OWNSHIP));
    //ShiAssert( otwPlatform == SimDriver.GetPlayerAircraft() );
    ShiAssert(sms); // If we legally might not have one, then we'd have to skip the ordinance...

    /*
    ** Render the 3d cockpit object
    */

    ShiAssert(vrCockpit);

    if ( not vrCockpit) // CTD fix
        return;


    // MLR 2003-10-12
    // I moved all my previous animation code to the AircraftClass,
    // it's more readily (more likely) to get updated there as new
    // DOFs and Switches are added.
    //
    // Also both 2d and 3d pit had the exact same duplicate code.
    SimDriver.GetPlayerAircraft()->CopyAnimationsToPit(vrCockpit);


    {
        // MLR 2003-10-05 This needs to be moved so it only runs once
        DrawableBSP *bsp = (DrawableBSP*)SimDriver.GetPlayerAircraft()->drawPointer;
        int t = bsp->GetTextureSet();
        vrCockpit->SetTextureSet(t % vrCockpit->GetNTextureSet());

        // PHASE 5 (D3D11, white cockpit panels): SetTextureSet references the cockpit texture set
        // into the loader queue, but the async loader thread is unreliable under D3D11 (loads part and stalls:
        // 912/2046 ok, 1308/1888/3599/3212/1290 -- not). Drain the queue synchronously (an engine
        // mechanism, as in RenderOTW::PreLoadScene). After loading the queue is empty -> WaitUpdates
        // early-returns (zero cost). Loads ALL cockpit textures before drawing.
        TheTextureBank.WaitUpdates();
    }



    // master caution light
    /*ATARIBABY Master Caution Light fix - not updated in virtual cockpit
    Use cockpitFlightData.IsSet(FlightData::MasterCaution) instead of pCockpitManager->mMiscStates.GetMasterCautionLight()
    Looks like pCockpitManager->mMiscStates.GetMasterCautionLight() is updated only if 2d caution light is in view
    Added Main Power check to all caution lights to copy 2d pit functionality*/
    // sfr: will test this using callbacks
#if 1

    if (cockpitFlightData.IsSet(FlightData::MasterCaution) and not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
        vrCockpit->SetSwitchMask(2, 1);
    else
        vrCockpit->SetSwitchMask(2, 0);

#endif


    //ATARIBABY but big thanx to ASSOCIATOR, new 3dpit start
    if (g_bUseNew3dpit)
    {
        //******************************************
        // LIGHTS
        //******************************************

        // AR/RDY light
        if (cockpitFlightData.IsSet(FlightData::RefuelRDY) and not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_ARRDY_LIGHT, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_ARRDY_LIGHT, 0);

        // AR/NWS light
        if (cockpitFlightData.IsSet(FlightData::RefuelAR) and not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_ARNWS_LIGHT, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_ARNWS_LIGHT, 0);

        // AR/DISC light
        if (cockpitFlightData.IsSet(FlightData::RefuelDSC) and not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_ARDISC_LIGHT, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_ARDISC_LIGHT, 0);

        // AOA BELOW light
        if (cockpitFlightData.IsSet(FlightData::AOABelow) and not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_AOABELOW_LIGHT, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_AOABELOW_LIGHT, 0);

        // AOA ON light
        if (cockpitFlightData.IsSet(FlightData::AOAOn) and not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_AOAON_LIGHT, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_AOAON_LIGHT, 0);

        // AOA ABOVE light
        if (cockpitFlightData.IsSet(FlightData::AOAAbove) and not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_AOAABOVE_LIGHT, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_AOAABOVE_LIGHT, 0);

        //EYEBROW CAUTION lights
        //ENG FIRE
        if (cockpitFlightData.IsSet(FlightData::ENG_FIRE) and not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_EYEBROW_ENGFIRE, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_EYEBROW_ENGFIRE, 0);

        //ENGINE
        if (cockpitFlightData.IsSet(FlightData::EngineFault) and not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_EYEBROW_ENGINE, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_EYEBROW_ENGINE, 0);

        //HYD/OIL
        if ((cockpitFlightData.IsSet(FlightData::HYD) or cockpitFlightData.IsSet(FlightData::OIL)) and not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_EYEBROW_HYDOIL, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_EYEBROW_HYDOIL, 0);

        //FLCS
        if ((cockpitFlightData.IsSet(FlightData::FltControlSys) or cockpitFlightData.IsSet(FlightData::DUAL)) and not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_EYEBROW_FLCS, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_EYEBROW_FLCS, 0);

        //TO/LDG config
        if (cockpitFlightData.IsSet(FlightData::T_L_CFG) and not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_EYEBROW_TOLDG, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_EYEBROW_TOLDG, 0);

        //CANOPY
        int canopyopen;

        if (SimDriver.GetPlayerAircraft()->IsComplex())
            canopyopen = SimDriver.GetPlayerAircraft()->GetDOFValue(COMP_CANOPY_DOF) > 0;
        else
            canopyopen = SimDriver.GetPlayerAircraft()->GetDOFValue(SIMP_CANOPY_DOF) > 0;

        if ((cockpitFlightData.IsSet(FlightData::CAN) or cockpitFlightData.IsSet(FlightData::OXY_LOW) or canopyopen) and not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_EYEBROW_CANOPY, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_EYEBROW_CANOPY, 0);

        //TF-FAIL
        if (cockpitFlightData.IsSet(FlightData::TF) and not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_EYEBROW_TFFAIL, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_EYEBROW_TFFAIL, 0);

        //Interior lights
        // COBRA - RED - Canopy does not forces interior light... and needs no power up...
        if (SimDriver.GetPlayerAircraft()->GetInteriorLight())
        {
            vrCockpit->SetSwitchMask(COMP_3DPIT_INTERIOR_LIGHTS, 1);
            SimDriver.GetPlayerAircraft()->SetSwitch(COMP_3DPIT_INTERIOR_LIGHTS, 1);
            SimDriver.GetPlayerAircraft()->SetAcStatusBits(AircraftClass::ACSTATUS_PITLIGHT);
        }
        else
        {
            vrCockpit->SetSwitchMask(COMP_3DPIT_INTERIOR_LIGHTS, 0);
            SimDriver.GetPlayerAircraft()->SetSwitch(COMP_3DPIT_INTERIOR_LIGHTS, 0);
            SimDriver.GetPlayerAircraft()->ClearAcStatusBits(AircraftClass::ACSTATUS_PITLIGHT);
        }

        //Instrument lights
        if ((SimDriver.GetPlayerAircraft()->GetInstrumentLight()) and not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_INSTRUMENT_LIGHTS, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_INSTRUMENT_LIGHTS, 0);

        //******************************************
        // New 3D cockpit Lights
        //******************************************
        // Caution Panel lights
        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL1_1, SimDriver.GetPlayerAircraft()->mFaults->GetFault(flt_cont_fault));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL1_2, SimDriver.GetPlayerAircraft()->mFaults->GetFault(elec_fault));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL1_3, SimDriver.GetPlayerAircraft()->mFaults->GetFault(probeheat_fault));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL1_4, SimDriver.GetPlayerAircraft()->mFaults->GetFault(lef_fault));  // LEF sub'ed for C ADC ????

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL1_5, SimDriver.GetPlayerAircraft()->mFaults->GetFault(stores_config_fault));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL1_6, SimDriver.GetPlayerAircraft()->mFaults->GetFault(lastFault));  // no act sub'ed for AFT NOT ENGAGED ???

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL1_7, SimDriver.GetPlayerAircraft()->mFaults->GetFault(fwd_fuel_low_fault));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL1_8, SimDriver.GetPlayerAircraft()->mFaults->GetFault(aft_fuel_low_fault));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL2_1, cockpitFlightData.IsSet(FlightData::EngineFault));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL2_2, SimDriver.GetPlayerAircraft()->mFaults->GetFault(sec_fault));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL2_3, SimDriver.GetPlayerAircraft()->mFaults->GetFault(fueloil_hot_fault));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL2_4, SimDriver.GetPlayerAircraft()->mFaults->GetFault(le_flaps_fault));  // Flaps fault sub'ed for INLET ICING ???

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL2_5, SimDriver.GetPlayerAircraft()->mFaults->GetFault(overheat_fault));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL2_6, SimDriver.GetPlayerAircraft()->mFaults->GetFault(ecm_fault));  // ecm fault sub'ed for ECC ???

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL2_7, SimDriver.GetPlayerAircraft()->mFaults->GetFault(buc_fault));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL2_8, SimDriver.GetPlayerAircraft()->mFaults->GetFault(fuel_low_fault));  // Fuel Low fault sub'ed for blank

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL3_1, SimDriver.GetPlayerAircraft()->mFaults->GetFault(avionics_fault));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL3_2, SimDriver.GetPlayerAircraft()->mFaults->GetFault(equip_host_fault));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL3_3, SimDriver.GetPlayerAircraft()->mFaults->GetFault(radar_alt_fault));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL3_4, SimDriver.GetPlayerAircraft()->mFaults->GetFault(iff_fault));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL3_5, SimDriver.GetPlayerAircraft()->mFaults->GetFault(lastFault));  // no act sub'ed for NUCLEAR ???

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL3_6, SimDriver.GetPlayerAircraft()->mFaults->GetFault(fuel_trapped));  // Fuel trapped fault sub'ed for ECC ???

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL3_7, SimDriver.GetPlayerAircraft()->mFaults->GetFault(fuel_home));  // Fuel "Bingo" fault sub'ed for blank

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL3_8, SimDriver.GetPlayerAircraft()->mFaults->GetFault(lastFault));  // blank

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL4_1, SimDriver.GetPlayerAircraft()->mFaults->GetFault(seat_notarmed_fault));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL4_2, SimDriver.GetPlayerAircraft()->mFaults->GetFault(nws_fault));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL4_3, SimDriver.GetPlayerAircraft()->mFaults->GetFault(anti_skid_fault));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL4_4, SimDriver.GetPlayerAircraft()->mFaults->GetFault(hook_fault));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL4_5, SimDriver.GetPlayerAircraft()->mFaults->GetFault(oxy_low_fault));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL4_6, SimDriver.GetPlayerAircraft()->mFaults->GetFault(cabin_press_fault));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL4_7, SimDriver.GetPlayerAircraft()->mFaults->GetFault(lastFault));  // blank

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_FAULT_COL4_8, SimDriver.GetPlayerAircraft()->mFaults->GetFault(lastFault));  // blank

        // Indicator lights
        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_TFR_STBY, cockpitFlightData.IsSet(FlightData::TFR_STBY));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_ECM_PWR, cockpitFlightData.IsSet(FlightData::EcmPwr));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_ECM_FAIL, cockpitFlightData.IsSet(FlightData::EcmFail));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_EPU_ON, cockpitFlightData.IsSet(FlightData::EPUOn));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_JFS_ON, cockpitFlightData.IsSet(FlightData::JFSOn));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_EPU_HYD, cockpitFlightData.IsSet(FlightData::Hydrazine));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_EPU_AIR, cockpitFlightData.IsSet(FlightData::Air));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_PWR_FLCSPGM, cockpitFlightData.IsSet(FlightData::FlcsPmg));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_PWR_MAINGEN, cockpitFlightData.IsSet(FlightData::MainGen));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_PWR_STBYGEN, cockpitFlightData.IsSet(FlightData::StbyGen));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_PWR_EPUGEN, cockpitFlightData.IsSet(FlightData::EpuGen));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_PWR_EPUPMG, cockpitFlightData.IsSet(FlightData::EpuPmg));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_PWR_TOFLCS, cockpitFlightData.IsSet(FlightData::ToFlcs));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_PWR_FLCSRLY, cockpitFlightData.IsSet(FlightData::FlcsRly));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_PWR_BATFAIL, cockpitFlightData.IsSet(FlightData::BatFail));

        if ( not SimDriver.GetPlayerAircraft()->mainPower == AircraftClass::MainPowerOff)
            vrCockpit->SetSwitchMask(COMP_3DPIT_AVTR_ON, SimDriver.AVTROn());

        //******************************************
        // INSTRUMNETS
        //******************************************

        //ADI and BACKUP ADI stuff
        if (g_bRealisticAvionics and g_bINS)
        {
            if (SimDriver.GetPlayerAircraft()->INSState(AircraftClass::BUP_ADI_OFF_IN))
            {
                //make a check for the BUP ADI energy here when ready
                BUPADIPitch3d = cockpitFlightData.pitch;
                BUPADIRoll3d = cockpitFlightData.roll;
                LastBUPPitch3d = BUPADIPitch3d;
                LastBUPRoll3d = BUPADIRoll3d;

                //set BUP ADI OFF mark off
                vrCockpit->SetSwitchMask(COMP_3DPIT_BACKUP_ADI_OFFMARK, 0);
            }
            else
            {
                BUPADIPitch3d = LastBUPPitch3d;
                BUPADIRoll3d = LastBUPRoll3d;

                //set BUP ADI OFF mark on
                vrCockpit->SetSwitchMask(COMP_3DPIT_BACKUP_ADI_OFFMARK, 1);
            }

            if ( not SimDriver.GetPlayerAircraft()->INSState(AircraftClass::INS_ADI_OFF_IN))
            {
                //stay where you currently are
                ADIPitch3d = LastMainADIPitch3d;
                ADIRoll3d = LastMainADIRoll3d;
            }
            else
            {
                ADIPitch3d = cockpitFlightData.pitch;
                ADIRoll3d = cockpitFlightData.roll;
                LastMainADIPitch3d = ADIPitch3d;
                LastMainADIRoll3d = ADIRoll3d;
            }

        }
        else
        {
            ADIPitch3d = cockpitFlightData.pitch;
            ADIRoll3d = cockpitFlightData.roll;
            BUPADIPitch3d = ADIPitch3d;
            BUPADIRoll3d = ADIRoll3d;
        }

        //MAIN ADI ball
        //Roll
        vrCockpit->SetDOFangle(COMP_3DPIT_ADI_ROLL, -ADIRoll3d);
        //Pitch
        vrCockpit->SetDOFangle(COMP_3DPIT_ADI_PITCH, -ADIPitch3d);

        //MAIN ADI OFF flag
        if ( not SimDriver.GetPlayerAircraft()->INSState(AircraftClass::INS_ADI_OFF_IN))
            vrCockpit->SetSwitchMask(COMP_3DPIT_ADI_OFF_FLAG, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_ADI_OFF_FLAG, 0);

        //MAIN ADI AUX flag
        if ( not SimDriver.GetPlayerAircraft()->INSState(AircraftClass::INS_ADI_AUX_IN))
            vrCockpit->SetSwitchMask(COMP_3DPIT_ADI_AUX_FLAG, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_ADI_AUX_FLAG, 0);

        //MAIN ADI LOC flag
        if (SimDriver.GetPlayerAircraft()->LOCValid == FALSE or SimDriver.GetPlayerAircraft()->currentPower == AircraftClass::PowerNone)
            vrCockpit->SetSwitchMask(COMP_3DPIT_ADI_LOC_FLAG, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_ADI_LOC_FLAG, 0);

        //MAIN ADI GS flag
        if (SimDriver.GetPlayerAircraft()->GSValid == FALSE or SimDriver.GetPlayerAircraft()->currentPower == AircraftClass::PowerNone)
            vrCockpit->SetSwitchMask(COMP_3DPIT_ADI_GS_FLAG, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_ADI_GS_FLAG, 0);

        //MAIN ADI ILS
        float dtILS = (float)(vuxGameTime - prevILStime);

        if (dtILS)
        {
            if (hILSneedle < hILS)
                hILSneedle = hILSneedle + 0.05F;

            if (hILSneedle > hILS)
                hILSneedle = hILSneedle - 0.05F;

            if (vILSneedle < vILS)
                vILSneedle = vILSneedle + 0.05F;

            if (vILSneedle > vILS)
                vILSneedle = vILSneedle - 0.05F;

            vrCockpit->SetDOFangle(COMP_3DPIT_ILSV_NEEDLE, vILSneedle / 10.0F);
            vrCockpit->SetDOFangle(COMP_3DPIT_ILSH_NEEDLE, -hILSneedle / 10.0F);

            //i use this timer for other needles as well
            if (SimDriver.GetPlayerAircraft()->af->HydraulicA() and HYDA3d < 3.64F)
            {
                HYDA3d = HYDA3d + 0.1F;
            }
            else if ( not SimDriver.GetPlayerAircraft()->af->HydraulicA() and HYDA3d > 0.0F)
            {
                HYDA3d = HYDA3d - 0.1F;
            }

            if (SimDriver.GetPlayerAircraft()->af->HydraulicB() and HYDB3d < 3.64F)
            {
                HYDB3d = HYDB3d + 0.1F;
            }
            else if ( not SimDriver.GetPlayerAircraft()->af->HydraulicB() and HYDB3d > 0.0F)
            {
                HYDB3d = HYDB3d - 0.1F;
            }

            prevILStime = vuxGameTime;
        }

        if (gNavigationSys)
        {
            if ((gNavigationSys->GetInstrumentMode() == NavigationSystem::ILS_TACAN or
                 gNavigationSys->GetInstrumentMode() == NavigationSystem::ILS_NAV) and 
                gNavigationSys->GetILSAttribute(NavigationSystem::GP_DEV, &hILS))
            {

                gNavigationSys->GetILSAttribute(NavigationSystem::GP_DEV, &hILS);
                gNavigationSys->GetILSAttribute(NavigationSystem::GS_DEV, &vILS);
                hILS *= RTD;
                vILS *= RTD;
                hILS = min(max(hILS, -3.75F), 3.75F) / 3.75F;
                vILS = min(max(vILS, -0.75F), 0.75F) / 0.75F;
            }
            else
            {
                hILS = -1.1F;
                vILS = -1.1F;
            }

            if (hILSneedle > -1.1F or vILSneedle > -1.1F)
                vrCockpit->SetSwitchMask(COMP_3DPIT_ILS_VISIBLE, 1);
            else
                vrCockpit->SetSwitchMask(COMP_3DPIT_ILS_VISIBLE, 0);
        }

        hILSneedle = MoveByRate(hILSneedle, hILS, 320.0F);
        vILSneedle = MoveByRate(vILSneedle, vILS, 320.0F);
        vrCockpit->SetDOFangle(COMP_3DPIT_ILSV_NEEDLE, vILSneedle / 10.0F);
        vrCockpit->SetDOFangle(COMP_3DPIT_ILSH_NEEDLE, -hILSneedle / 10.0F);

        //ADI backup ball
        //Roll
        vrCockpit->SetDOFangle(COMP_3DPIT_BACKUP_ADI_ROLL, -BUPADIRoll3d);
        //Pitch
        vrCockpit->SetDOFangle(COMP_3DPIT_BACKUP_ADI_PITCH, -BUPADIPitch3d);

        //backup magnetic compass
        MAGCOMPASS3d = cockpitFlightData.yaw;
        vrCockpit->SetDOFangle(COMP_3DPIT_MAG_COMPASS, MAGCOMPASS3d);

        //HSI
        //current heading
        vrCockpit->SetDOFangle(COMP_3DPIT_HSI_HDG, -cockpitFlightData.currentHeading * 0.017453292F);
        //desired course
        vrCockpit->SetDOFangle(COMP_3DPIT_HSI_CRS, cockpitFlightData.desiredCourse * 0.017453292F);
        //desired heading
        vrCockpit->SetDOFangle(COMP_3DPIT_HSI_DHDG, cockpitFlightData.desiredHeading * 0.017453292F);
        //beacon course
        vrCockpit->SetDOFangle(COMP_3DPIT_HSI_BCN, cockpitFlightData.bearingToBeacon * 0.017453292F);

        //HSI TO/FROM flags
        BOOL crsToTrueFlag = HSITOFROM3d;

        if (g_bRealisticAvionics)
        {
            if (gNavigationSys)
            {
                if (gNavigationSys->GetInstrumentMode() == NavigationSystem::NAV)
                    crsToTrueFlag = FALSE;
            }
        }

        if (crsToTrueFlag == TRUE)   // to
        {
            vrCockpit->SetSwitchMask(COMP_3DPIT_HSI_TO_FLAG, 1);
            vrCockpit->SetSwitchMask(COMP_3DPIT_HSI_FROM_FLAG, 0);
        }
        else if (crsToTrueFlag == 2)   // from
        {
            vrCockpit->SetSwitchMask(COMP_3DPIT_HSI_TO_FLAG, 0);
            vrCockpit->SetSwitchMask(COMP_3DPIT_HSI_FROM_FLAG, 1);
        }
        else   // to/from both off
        {
            vrCockpit->SetSwitchMask(COMP_3DPIT_HSI_TO_FLAG, 0);
            vrCockpit->SetSwitchMask(COMP_3DPIT_HSI_FROM_FLAG, 0);
        }

        //HSI course deviation needle
        float hsidev = cockpitFlightData.courseDeviation;

        if (hsidev > 90) hsidev = 180 - hsidev;

        if (hsidev < -90) hsidev = - (180 + hsidev);

        vrCockpit->SetDOFangle(COMP_3DPIT_HSI_CRSDEV, hsidev);

        //HSI OFF flag
        if ( not SimDriver.GetPlayerAircraft()->INSState(AircraftClass::INS_HSI_OFF_IN))
            vrCockpit->SetSwitchMask(COMP_3DPIT_HSI_OFF_FLAG, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_HSI_OFF_FLAG, 0);

        //HSI ILSWARN flag
        if (cockpitFlightData.IsSetHsi(FlightData::IlsWarning))
            vrCockpit->SetSwitchMask(COMP_3DPIT_HSI_ILSWARN_FLAG, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_HSI_ILSWARN_FLAG, 0);

        //HSI CRSWARN flag
        if (cockpitFlightData.IsSetHsi(FlightData::CourseWarning))
            vrCockpit->SetSwitchMask(COMP_3DPIT_HSI_CRSWARN_FLAG, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_HSI_CRSWARN_FLAG, 0);

        //HSI distance to beacon digital readout
        float hsidist = cockpitFlightData.distanceToBeacon;
        vrCockpit->SetDOFangle(COMP_3DPIT_HSI_DIST_DIGIT3, ExtractDigit(hsidist, 0) * 0.6283F);
        vrCockpit->SetDOFangle(COMP_3DPIT_HSI_DIST_DIGIT2, ExtractDigit(hsidist, 1) * 0.6283F);
        vrCockpit->SetDOFangle(COMP_3DPIT_HSI_DIST_DIGIT1, ExtractDigit(hsidist, 2) * 0.6283F);

        //HSI course digital readout
        float hsicrs = cockpitFlightData.desiredCourse;
        vrCockpit->SetDOFangle(COMP_3DPIT_HSI_CRS_DIGIT3, ExtractDigit(hsicrs, 0) * 0.6283F);
        vrCockpit->SetDOFangle(COMP_3DPIT_HSI_CRS_DIGIT2, ExtractDigit(hsicrs, 1) * 0.6283F);
        vrCockpit->SetDOFangle(COMP_3DPIT_HSI_CRS_DIGIT1, ExtractDigit(hsicrs, 2) * 0.6283F);

        //fuel flow digital readout
        float fuelflow = cockpitFlightData.fuelFlow;
        float fuelflowdigit3 = (((long) fuelflow) % 1000) / 1000.0F;
        vrCockpit->SetDOFangle(COMP_3DPIT_FUELFLOW_DIGIT3, fuelflowdigit3 * (2 * PI));
        vrCockpit->SetDOFangle(COMP_3DPIT_FUELFLOW_DIGIT2, ExtractDigit(fuelflow, 3) * 0.6283F);
        vrCockpit->SetDOFangle(COMP_3DPIT_FUELFLOW_DIGIT1, ExtractDigit(fuelflow, 4) * 0.6283F);


        //******************************************
        // NEEDLES
        //******************************************

        //G-Meter needle
        vrCockpit->SetDOFangle(COMP_3DPIT_G_NEEDLE, (float) cockpitFlightData.gs);

        //ASI needle
        float value = cockpitFlightData.kias;

        if (value < 80.0F)
            value = 80.0F;
        else if (value > 850.0F)
            value = 850.0F;

        value = value / 100.0F;
        //ASI Instrument has a Log10 scale.
        vrCockpit->SetDOFangle(COMP_3DPIT_ASI_NEEDLE, (float)((log10(value) * 5.8F) + 0.6F));

        //ASI mach digital readout
        float machNumber;
        int machfirstDigit;
        int machsecondDigit;
        machNumber = cockpitFlightData.mach;
        machfirstDigit = (int) machNumber;
        machsecondDigit = (int)(10.0F * (machNumber - ((float) machfirstDigit)));

        vrCockpit->SetDOFangle(COMP_3DPIT_ASIMACH_DIGIT1, (float) machfirstDigit * 0.6283F);
        vrCockpit->SetDOFangle(COMP_3DPIT_ASIMACH_DIGIT2, (float) machsecondDigit * 0.6283F);

        //ALTIMETER needle
        float altneedle = (((long) - cockpitFlightData.z) % 1000) / 1000.0F;
        vrCockpit->SetDOFangle(COMP_3DPIT_ALT_NEEDLE, (float) altneedle * (2 * PI));

        //ALTIMETER digital readout
        float alt;
        int altfirstDigit;
        int altsecondDigit;
        alt = -cockpitFlightData.z;
        altfirstDigit = (int) alt / 10000;
        altsecondDigit = (int)(((alt / 10000) - altfirstDigit) * 10.0F) ;

        vrCockpit->SetDOFangle(COMP_3DPIT_ALT_DIGIT1, (float) altfirstDigit * 0.6283F);
        vrCockpit->SetDOFangle(COMP_3DPIT_ALT_DIGIT2, (float) altsecondDigit * 0.6283F);
        vrCockpit->SetDOFangle(COMP_3DPIT_ALT_DIGIT3, (float) altneedle * (2 * PI));

        //ALTIMETER PNEU flag - if main generator not running then PNEU flag apears
        if ( not SimDriver.GetPlayerAircraft()->af->GeneratorRunning(AirframeClass::GenMain))
        {
            vrCockpit->SetSwitchMask(COMP_3DPIT_ALTPNEU_FLAG, 1);
        }
        else
        {
            vrCockpit->SetSwitchMask(COMP_3DPIT_ALTPNEU_FLAG, 0);
        }

        //total fuel digital readout
        vrCockpit->SetDOFangle(COMP_3DPIT_FUEL_DIGIT5, 0);
        vrCockpit->SetDOFangle(COMP_3DPIT_FUEL_DIGIT4, 0);
        vrCockpit->SetDOFangle(COMP_3DPIT_FUEL_DIGIT3, ExtractDigit(cockpitFlightData.total, 2) * 0.6283F);
        vrCockpit->SetDOFangle(COMP_3DPIT_FUEL_DIGIT2, ExtractDigit(cockpitFlightData.total, 3) * 0.6283F);
        vrCockpit->SetDOFangle(COMP_3DPIT_FUEL_DIGIT1, ExtractDigit(cockpitFlightData.total, 4) * 0.6283F);

        //FUEL FWD needle
        vrCockpit->SetDOFangle(COMP_3DPIT_FUELFWD_NEEDLE, (float) cockpitFlightData.fwd * 0.00010F);
        //FUEL AFT needle
        vrCockpit->SetDOFangle(COMP_3DPIT_FUELAFT_NEEDLE, (float) cockpitFlightData.aft * 0.00010F);

        //OIL press
        vrCockpit->SetDOFangle(COMP_3DPIT_OIL_NEEDLE, (float) cockpitFlightData.oilPressure * 0.057F);

        //NOZZLE pos
        vrCockpit->SetDOFangle(COMP_3DPIT_NOZ_NEEDLE, (float) cockpitFlightData.nozzlePos * 0.042F);

        float rpm = cockpitFlightData.rpm;
        float needle;

        //RPM
        //match F16 RPM scale
        if (rpm < 60)
        {
            needle = rpm * 0.03315F;
        }
        else if (rpm < 70)
        {
            needle = 1.989F + ((rpm - 60.0F) * 0.0541F);
        }
        else if (rpm < 100)
        {
            needle = 2.53F + ((rpm - 70.0F) * 0.079F);
        }
        else
        {
            needle = 4.9F + ((rpm - 100.0F) * 0.08266F);
        }

        vrCockpit->SetDOFangle(COMP_3DPIT_RPM_NEEDLE, needle);

        //FTIT
        float ftit;
        rpm = SimDriver.GetPlayerAircraft()->af->oldp01[0];

        // FTIT values from Sylvain :-)
        if (rpm < 0.2F)
        {
            ftit = 5.1F * rpm / 0.2f; // JPO adapt for < idle speeds.
        }
        else if (rpm < 0.6225F) // 0.9^4.5
        {
            ftit = 5.1F + (rpm - 0.2F) / 0.4225F * 1.0F;
        }
        else if (rpm < 1.0F)
        {
            ftit = 6.1F + (rpm - 0.6225F) / 0.3775F * 1.5F;
        }
        else
        {
            ftit = 7.6F + (rpm - 1.0F) / 0.53F * 0.4F; // 0.53 is full afterburner
        }

        if (ftit > 12.0F)
        {
            ftit = 12.0F;
        }

        //match F16 FTIT scale
        if (ftit < 2.0F)
        {
            needle = 0;
        }
        else if (ftit < 7.0F)
        {
            needle = (ftit - 2.0F) * 0.345F;
        }
        else if (ftit < 10.0F)
        {
            needle = 1.727F + ((ftit - 7.0F) * 1.035F);
        }
        else
        {
            needle = 4.834F + ((ftit - 10.0F) * 0.375F);
        }

        vrCockpit->SetDOFangle(COMP_3DPIT_FTIT_NEEDLE, needle);

        //HYD A/B
        if (SimDriver.GetPlayerAircraft()->af->HydraulicA() and HYDA3d < 3.64F)
        {
            HYDA3d = MoveByRate(HYDA3d, 3.64F, 200);
        }
        else if ( not SimDriver.GetPlayerAircraft()->af->HydraulicA() and HYDA3d > 0.0F)
        {
            HYDA3d = MoveByRate(HYDA3d, 0.0F, 200);
        }

        if (SimDriver.GetPlayerAircraft()->af->HydraulicB() and HYDB3d < 3.64F)
        {
            HYDB3d = MoveByRate(HYDB3d, 3.64F, 200);
        }
        else if ( not SimDriver.GetPlayerAircraft()->af->HydraulicB() and HYDB3d > 0.0F)
        {
            HYDB3d = MoveByRate(HYDB3d, 0.0F, 200);
        }

        //HYD A
        vrCockpit->SetDOFangle(COMP_3DPIT_HYDA_NEEDLE, HYDA3d);

        //HYD B
        vrCockpit->SetDOFangle(COMP_3DPIT_HYDB_NEEDLE, HYDB3d);

        //EPU fuel
        vrCockpit->SetDOFangle(COMP_3DPIT_EPU_NEEDLE, cockpitFlightData.epuFuel * 0.04241F);

        //AOA tape
        if (cockpitFlightData.IsSetHsi(FlightData::AOA))
        {
            vrCockpit->SetSwitchMask(COMP_3DPIT_AOA_OFF_FLAG, 1);
            vrCockpit->SetDOFangle(COMP_3DPIT_AOA, -0.69F);
            vrCockpit->SetDOFangle(COMP_3DPIT_AOA_DIAL, 19.0f * DTR);
        }
        else
        {
            float aoa = -cockpitFlightData.alpha;

            if (aoa > 35.0F)
                aoa = 35.0F;

            if (aoa < -35.0F)
                aoa = -35.0F;

            vrCockpit->SetSwitchMask(COMP_3DPIT_AOA_OFF_FLAG, 0);
            vrCockpit->SetDOFangle(COMP_3DPIT_AOA, aoa * 0.01146F);
            // AOA Dial
            aoa += 19.0f;
            vrCockpit->SetDOFangle(COMP_3DPIT_AOA_DIAL, aoa * DTR);
        }

        //VVI tape
        if (cockpitFlightData.IsSetHsi(FlightData::VVI))
        {
            vrCockpit->SetSwitchMask(COMP_3DPIT_VVI_OFF_FLAG, 1);
            vrCockpit->SetDOFangle(COMP_3DPIT_VVI, -0.69F);
            vrCockpit->SetDOFangle(COMP_3DPIT_VVI_DIAL, -6.0F * DTR);
        }
        else
        {
            float vvi = cockpitFlightData.zDot * 0.06F;

            if (vvi > 6.0F)
                vvi = 6.0F;

            if (vvi < -6.0F)
                vvi = -6.0F;

            vrCockpit->SetSwitchMask(COMP_3DPIT_VVI_OFF_FLAG, 0);
            vrCockpit->SetDOFangle(COMP_3DPIT_VVI, vvi * 0.06981F);
            vrCockpit->SetDOFangle(COMP_3DPIT_VVI_DIAL, vvi * DTR);
        }

        //=======================================================
        // New 3D pit switch/knob animation - FRB
        // What time is it?
        VU_TIME currentTime;
        VU_TIME remainder;
        VU_TIME hours;
        VU_TIME minutes;
        VU_TIME seconds;
        // Get current time convert from ms to secs
        currentTime = vuxGameTime / 1000;
        remainder = currentTime % 86400; //86400 secs in a day
        hours = remainder / 3600; // 3600 secs in an hour
        remainder = remainder - hours * 3600;

        if (hours > 12)
        {
            hours -= 12;
        }

        minutes = remainder / 60;
        seconds = remainder - minutes * 60;
        // add back fraction of hour and fraction of minutes so that hour and min hand doesn't pop
        float Hours = (float)hours;
        float Minutes = (float)minutes;
        float Seconds = (float)seconds;
        Hours += (Minutes * 0.01667F); // minutes * 1/60
        Minutes += (Seconds * 0.01667F);
        vrCockpit->SetDOFangle(COMP_3DPIT_CLOCK_HRS, Hours * 30.0F * DTR); // degrees per hour
        vrCockpit->SetDOFangle(COMP_3DPIT_CLOCK_MINS, Minutes * 6.0F * DTR); // degrees per minute
        vrCockpit->SetDOFangle(COMP_3DPIT_CLOCK_SECS, Seconds * 6.0F * DTR); // degrees per second

        PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::RWR);

        // RWR Launch warning light
        if (theRwr)
        {
            if (theRwr->LaunchIndication() and (vuxRealTime bitand 0x200))
                vrCockpit->SetSwitchMask(COMP_3DPIT_RWR_LAUNCH, 1);
            else
                vrCockpit->SetSwitchMask(COMP_3DPIT_RWR_LAUNCH, 0);

            // RWR switches
            if (theRwr->IsPriority() not_eq FALSE)
                vrCockpit->SetSwitchMask(COMP_3DPIT_RWR_PRIORITY, 2);
            else
                vrCockpit->SetSwitchMask(COMP_3DPIT_RWR_PRIORITY, 1);

            if (theRwr->TargetSep() not_eq FALSE)
                vrCockpit->SetSwitchMask(COMP_3DPIT_RWR_TGT_SEP, 2);
            else
                vrCockpit->SetSwitchMask(COMP_3DPIT_RWR_TGT_SEP, 1);

            if (theRwr->ShowUnknowns() not_eq FALSE)
                vrCockpit->SetSwitchMask(COMP_3DPIT_RWR_UNKS, 2);
            else
                vrCockpit->SetSwitchMask(COMP_3DPIT_RWR_UNKS, 1);

            if (theRwr->ShowNaval() not_eq FALSE)
                vrCockpit->SetSwitchMask(COMP_3DPIT_RWR_NAVAL, 2);
            else
                vrCockpit->SetSwitchMask(COMP_3DPIT_RWR_NAVAL, 1);

            if (theRwr->ShowLowAltPriority() not_eq FALSE)
                vrCockpit->SetSwitchMask(COMP_3DPIT_RWR_GND_PRI, 2);
            else
                vrCockpit->SetSwitchMask(COMP_3DPIT_RWR_GND_PRI, 1);

            if (theRwr->ShowSearch() not_eq FALSE)
                vrCockpit->SetSwitchMask(COMP_3DPIT_RWR_SEARCH, 2);
            else
                vrCockpit->SetSwitchMask(COMP_3DPIT_RWR_SEARCH, 1);

            vrCockpit->SetSwitchMask(COMP_3DPIT_RWR_HNDOFF, 1);  // Momentary Sw
        }

        // Master Arm switch
        if (SimDriver.GetPlayerAircraft()->Sms->MasterArm() == SMSBaseClass::Arm)
            vrCockpit->SetSwitchMask(COMP_3DPIT_MASTER_ARM, 2);
        else if (SimDriver.GetPlayerAircraft()->Sms->MasterArm() ==  SMSBaseClass::Sim)
            vrCockpit->SetSwitchMask(COMP_3DPIT_MASTER_ARM, 4);
        else // safe
            vrCockpit->SetSwitchMask(COMP_3DPIT_MASTER_ARM, 1);

        // HUD Scale switch
        if (TheHud->GetScalesSwitch() < 3)
            vrCockpit->SetSwitchMask(COMP_3DPIT_HUD_VAH, 1 << (2 - TheHud->GetScalesSwitch()));
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_HUD_VAH, 1);

        // HUD Pitch ladder switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_HUD_FPM_LADD, 1 << TheHud->GetFPMSwitch());
        // HUD Color wheel
        vrCockpit->SetSwitchMask(COMP_3DPIT_ICP_BRT_WHEEL, 1 << curColorIdx);
        // HUD Contrast wheel
        vrCockpit->SetSwitchMask(COMP_3DPIT_ICP_BRT_WHEEL, 1 << ((int)(TheHud->ContWheelPos * 10)));
        // ICP DriftCo switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_ICP_DRIFTCO, 1 << TheHud->GetDriftCOSwitch());

        // Cat I/III switch
        if (SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::CATLimiterIII))
            vrCockpit->SetSwitchMask(COMP_3DPIT_STORES_CAT, 2);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_STORES_CAT, 1);

        // Thrust reverser switch
        if (SimDriver.GetPlayerAircraft()->af->thrustReverse == 0)
            vrCockpit->SetSwitchMask(COMP_3DPIT_REV_THRUSTER, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_REV_THRUSTER, 2);

        // HSI Course knob
        int val = 1 << ((int)(OTWDriver.pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_CRS) / 36.0f));
        vrCockpit->SetSwitchMask(COMP_3DPIT_HSI_COURSE, val);
        // HSI Heading knob
        val = 1 << ((int)(OTWDriver.pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_HEADING) / 36.0f));
        vrCockpit->SetSwitchMask(COMP_3DPIT_HSI_HEADING, val);

        //
        // MPO switch
        if (SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::MPOverride))
            vrCockpit->SetSwitchMask(COMP_3DPIT_MPO, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_MPO, 2);

        // Silence the horn
        if (SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::HornSilenced))
            vrCockpit->SetSwitchMask(COMP_3DPIT_SILENCE_HORN, 2);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_SILENCE_HORN, 1);

        // HSI Mode switch
        if (gNavigationSys->GetInstrumentMode() == NavigationSystem::ILS_TACAN)
            vrCockpit->SetSwitchMask(COMP_3DPIT_HSI_MODE, 1);
        else if (gNavigationSys->GetInstrumentMode() == NavigationSystem::TACAN)
            vrCockpit->SetSwitchMask(COMP_3DPIT_HSI_MODE, 2);
        else if (gNavigationSys->GetInstrumentMode() == NavigationSystem::NAV)
            vrCockpit->SetSwitchMask(COMP_3DPIT_HSI_MODE, 4);
        else if (gNavigationSys->GetInstrumentMode() == NavigationSystem::ILS_NAV)
            vrCockpit->SetSwitchMask(COMP_3DPIT_HSI_MODE, 8);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_HSI_MODE, 1);

        // HUD DED/PFL switch
        if (TheHud->GetDEDSwitch() == HudClass::PFL_DATA)
            vrCockpit->SetSwitchMask(COMP_3DPIT_HUD_DED_PFL, 2);
        else if (TheHud->GetDEDSwitch() == HudClass::DED_DATA)
            vrCockpit->SetSwitchMask(COMP_3DPIT_HUD_DED_PFL, 4);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_HUD_DED_PFL, 1);

        // HUD velocity switch
        if (TheHud->GetVelocitySwitch() == HudClass::CAS)
            vrCockpit->SetSwitchMask(COMP_3DPIT_HUD_VELOCITY, 4);
        else if (TheHud->GetVelocitySwitch() == HudClass::TAS)
            vrCockpit->SetSwitchMask(COMP_3DPIT_HUD_VELOCITY, 2);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_HUD_VELOCITY, 1);

        // HUD radar altitude switch (RAL/BARO)
        if (TheHud->GetRadarSwitch() == HudClass::ALT_RADAR)
            vrCockpit->SetSwitchMask(COMP_3DPIT_HUD_RAL_BARO, 4);
        else if (TheHud->GetRadarSwitch() == HudClass::BARO)
            vrCockpit->SetSwitchMask(COMP_3DPIT_HUD_RAL_BARO, 2);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_HUD_RAL_BARO, 1);

        // HUD brightness switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_HUD_DAY_NITE, 1 << TheHud->GetBrightnessSwitch());

        // Chaff Remaining units digit
        if (((AircraftClass*)(SimDriver.GetPlayerEntity()))->HasPower(AircraftClass::ChaffFlareCount))
        {
            val = ((AircraftClass*)(SimDriver.GetPlayerEntity()))->counterMeasureStation[CHAFF_STATION].weaponCount;
            vrCockpit->SetDOFangle(COMP_3DPIT_CHAFF_DIGIT1, ExtractDigit((float)val, 0) * 0.6283F);
            vrCockpit->SetDOFangle(COMP_3DPIT_CHAFF_DIGIT2, ExtractDigit((float)val, 1) * 0.6283F);
            vrCockpit->SetDOFangle(COMP_3DPIT_CHAFF_DIGIT3, ExtractDigit((float)val, 2) * 0.6283F);
        }

        // Flare Remaining units digit
        if (((AircraftClass*)(SimDriver.GetPlayerEntity()))->HasPower(AircraftClass::ChaffFlareCount))
        {
            val = ((AircraftClass*)(SimDriver.GetPlayerEntity()))->counterMeasureStation[FLARE_STATION].weaponCount;
            vrCockpit->SetDOFangle(COMP_3DPIT_FLARE_DIGIT1, ExtractDigit((float)val, 0) * 0.6283F);
            vrCockpit->SetDOFangle(COMP_3DPIT_FLARE_DIGIT2, ExtractDigit((float)val, 1) * 0.6283F);
            vrCockpit->SetDOFangle(COMP_3DPIT_FLARE_DIGIT3, ExtractDigit((float)val, 2) * 0.6283F);
        }

        // Aux Comm Tacan channel left digit
        val = gNavigationSys->GetTacanChannel(NavigationSystem::AUXCOMM, 2);
        vrCockpit->SetDOFangle(COMP_3DPIT_TACAN_LEFT, val * 0.6283F);
        // Aux Comm Tacan channel middle digit
        val = gNavigationSys->GetTacanChannel(NavigationSystem::AUXCOMM, 1);
        vrCockpit->SetDOFangle(COMP_3DPIT_TACAN_CENTER, val * 0.6283F);
        // Aux Comm Tacan channel right digit
        val = gNavigationSys->GetTacanChannel(NavigationSystem::AUXCOMM, 0);
        vrCockpit->SetDOFangle(COMP_3DPIT_TACAN_RIGHT, val * 0.6283F);

        // Aux Comm Tacan channel band (X/Y)
        if (gNavigationSys->GetTacanBand(NavigationSystem::AUXCOMM) == TacanList::X)
            vrCockpit->SetSwitchMask(COMP_3DPIT_TACAN_BAND, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_TACAN_BAND, 2);

        // Aux Comm source switch
        if (gNavigationSys->GetControlSrc() == NavigationSystem::AUXCOMM)
            vrCockpit->SetSwitchMask(COMP_3DPIT_AUX_COMM_SRC, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_AUX_COMM_SRC, 2);

        // Aux Comm Master switch
        val = gNavigationSys->GetDomain(NavigationSystem::AUXCOMM) + 1;
        vrCockpit->SetSwitchMask(COMP_3DPIT_AUX_COMM_MSTR, val);
        // EPU switch
        val = 1 << SimDriver.GetPlayerAircraft()->af->GetEpuSwitch();
        vrCockpit->SetSwitchMask(COMP_3DPIT_EPU, val);

        // Alt gear switch/lever
        if (SimDriver.GetPlayerAircraft()->af->altGearDeployed == true)
            vrCockpit->SetSwitchMask(COMP_3DPIT_ALT_GEAR, 2);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_ALT_GEAR, 1);

        // HUD Radar altitude switch
        if (SimDriver.GetPlayerAircraft()->af->platform->RALTStatus == AircraftClass::ROFF)
            vrCockpit->SetSwitchMask(COMP_3DPIT_RALT_PWR, 1);
        else if (SimDriver.GetPlayerAircraft()->af->platform->RALTStatus == AircraftClass::RON)
            vrCockpit->SetSwitchMask(COMP_3DPIT_RALT_PWR, 4);
        else if (SimDriver.GetPlayerAircraft()->af->platform->RALTStatus == AircraftClass::RSTANDBY)
            vrCockpit->SetSwitchMask(COMP_3DPIT_RALT_PWR, 2);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_RALT_PWR, 1);

        // JSF start switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_JSF_START, SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::JfsStart + 1));
        // SMS power switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_SMS_PWR, 1 << SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::SMSPower));
        // FCC power switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_FCC_PWR, 1 << SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::FCCPower));
        // MFD power switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_MFD_PWR, 1 << SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::MFDPower));
        // UFC power switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_UFC_PWR, 1 << SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::UFCPower));
        // GPS power switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_GPS_PWR, 1 << SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::GPSPower));
        // DL power switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_DL_PWR, 1 << SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::DLPower));
        // MAP power switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_MAP_PWR, 1 << SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::MAPPower));
        // Right hardpoints power switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_RIGHT_HPT_PWR, 1 << SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::RightHptPower));
        // Left hardpoints power switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_LEFT_HPT_PWR, 1 << SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::LeftHptPower));
        // HUD power switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_HUD_PWR, 1 << SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::HUDPower));
        // FCR power switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_FCR_PWR, 1 << SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::FCRPower));
        // Fuel Control switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_FUEL_QTY, 1 << (SimDriver.GetPlayerAircraft()->af->GetFuelSwitch()));
        // Fuel pump switch
        val = SimDriver.GetPlayerAircraft()->af->GetFuelPump() + 1;
        vrCockpit->SetSwitchMask(COMP_3DPIT_REFUEL_PUMP, val);

        // Refuel master switch
        if (SimDriver.GetPlayerAircraft()->af->IsEngineFlag(AirframeClass::MasterFuelOff))
            vrCockpit->SetSwitchMask(COMP_3DPIT_REFUEL_MSTR, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_REFUEL_MSTR, 2);

        // Air source switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_AIR_SOURCE, 1 << SimDriver.GetPlayerAircraft()->af->GetAirSource());

        // Landing lights switch
        if (SimDriver.GetPlayerAircraft()->IsAcStatusBitsSet(AircraftClass::ACSTATUS_EXT_LANDINGLIGHT))
            vrCockpit->SetSwitchMask(COMP_3DPIT_LAND_LIGHT, 2);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_LAND_LIGHT, 1);

        // Parking brake switch
        if (SimDriver.GetPlayerAircraft()->af->PBON == TRUE)
            vrCockpit->SetSwitchMask(COMP_3DPIT_PARK_BRAKE, 2);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_PARK_BRAKE, 1);

        // Hook switch
        if (SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::Hook))
            vrCockpit->SetSwitchMask(COMP_3DPIT_HOOK, 2);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_HOOK, 1);

        // Laser switch
        if (SimDriver.GetPlayerAircraft()->FCC->LaserArm)
            vrCockpit->SetSwitchMask(COMP_3DPIT_LASER_ARM, 2);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_LASER_ARM, 1);

        // Refuel door switch
        if (SimDriver.GetPlayerAircraft()->af->IsEngineFlag(AirframeClass::FuelDoorOpen))
            vrCockpit->SetSwitchMask(COMP_3DPIT_REFUEL_DOOR, 2);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_REFUEL_DOOR, 1);

        // Autopilot left switch
        // Left switch Middle position
        if (SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::RollHold))
            vrCockpit->SetSwitchMask(COMP_3DPIT_LT_AP_SW, 4);
        // Left switch down position
        else if (SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::StrgSel))
            vrCockpit->SetSwitchMask(COMP_3DPIT_LT_AP_SW, 1);
        // Left switch up position
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_LT_AP_SW, 2);

        // Autopilot left switch
        // Right switch up position
        if (SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::AltHold))
            vrCockpit->SetSwitchMask(COMP_3DPIT_RT_AP_SW, 4);
        // Right switch down position
        else if (SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::AttHold))
            vrCockpit->SetSwitchMask(COMP_3DPIT_RT_AP_SW, 1);
        // Right switch middle position (off)
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_RT_AP_SW, 1);

        // HUD reticle switch
        if (TheHud->WhichMode == 1) // PRI
            vrCockpit->SetSwitchMask(COMP_3DPIT_HUD_RETICLE, 2);

        if (TheHud->WhichMode == 2) // STBY
            vrCockpit->SetSwitchMask(COMP_3DPIT_HUD_RETICLE, 4);

        if (TheHud->WhichMode == 0) // Off
            vrCockpit->SetSwitchMask(COMP_3DPIT_HUD_RETICLE, 1);

        // Interior light switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_INTERIOR_LITE, 1 << SimDriver.GetPlayerAircraft()->GetInteriorLight());
        // Instrument light switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_INSTR_LITE, 1 << SimDriver.GetPlayerAircraft()->GetInstrumentLight());
        // Spot light switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_SPOT_LITE, 1 << SimDriver.GetPlayerAircraft()->GetSpotLight());
        // EWS RWR power switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_EWS_RWR_PWR, 1 << SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::EWSRWRPower));
        // EWS jammer power
        vrCockpit->SetSwitchMask(COMP_3DPIT_EWS_JMR_PWR, 1 << SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::EWSJammerPower));
        // EWS chaff power
        vrCockpit->SetSwitchMask(COMP_3DPIT_EWS_CHAFF_PWR, 1 << SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::EWSChaffPower));
        // EWS flares
        vrCockpit->SetSwitchMask(COMP_3DPIT_EWS_FLARE_PWR, 1 << SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::EWSFlarePower));
        // EWS PGM switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_EWS_MODE, 1 << SimDriver.GetPlayerAircraft()->EWSPGM());
        // EWS Program switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_EWS_PROG, 1 << SimDriver.GetPlayerAircraft()->EWSProgNum);
        // Main power switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_MAIN_PWR, 1 << SimDriver.GetPlayerAircraft()->mainPower);

        // Silence Betty (VMS)
        if (SimDriver.GetPlayerAircraft()->playBetty)
            vrCockpit->SetSwitchMask(COMP_3DPIT_VMS_PWR, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_VMS_PWR, 2);

        // RF emissions switch
        if (SimDriver.GetPlayerAircraft()->RFState == 0) //NORM
            vrCockpit->SetSwitchMask(COMP_3DPIT_RF_QUIET, 2);
        else if (SimDriver.GetPlayerAircraft()->RFState == 2)  //SILENT --> No CARA, no TFR, no Radar
            vrCockpit->SetSwitchMask(COMP_3DPIT_RF_QUIET, 4);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_RF_QUIET, 1); //QUIET --> no Radar

        // RWR power switch
        if (theRwr and theRwr->IsOn())
            vrCockpit->SetSwitchMask(COMP_3DPIT_RWR_PWR, 2);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_RWR_PWR, 1);

        // External light power switch
        if (SimDriver.GetPlayerAircraft()->ExtlState(AircraftClass::Extl_Main_Power))
            vrCockpit->SetSwitchMask(COMP_3DPIT_EXT_LITE_MSTR, 2);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_EXT_LITE_MSTR, 1);

        // External collision light switch
        if (SimDriver.GetPlayerAircraft()->ExtlState(AircraftClass::Extl_Anti_Coll))
            vrCockpit->SetSwitchMask(COMP_3DPIT_ANTI_COLL, 2);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_ANTI_COLL, 1);

        // External light flash switch
        if (SimDriver.GetPlayerAircraft()->ExtlState(AircraftClass::Extl_Flash))
            vrCockpit->SetSwitchMask(COMP_3DPIT_EXT_FLASH, 2);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_EXT_FLASH, 1);

        // External collision wing/tail switch
        if (SimDriver.GetPlayerAircraft()->ExtlState(AircraftClass::Extl_Wing_Tail))
            vrCockpit->SetSwitchMask(COMP_3DPIT_EXT_WING, 2);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_EXT_WING, 1);

        // AVTR SWITCH
        if (SimDriver.GetPlayerAircraft()->AVTRState(AircraftClass::AVTR_AUTO))
            vrCockpit->SetSwitchMask(COMP_3DPIT_AVTR_SW, 2);
        else if (SimDriver.GetPlayerAircraft()->AVTRState(AircraftClass::AVTR_ON))
            vrCockpit->SetSwitchMask(COMP_3DPIT_AVTR_SW, 4);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_AVTR_SW, 1);

        // IFF power switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_IFF_PWR, SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::IFFPower) + 1);
        // IFF query switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_IFF_QUERY, 1);

        // INS switch COMP_3DPIT_IFF_PWR
        if (SimDriver.GetPlayerAircraft()->INSState(AircraftClass::INS_AlignNorm))
            vrCockpit->SetSwitchMask(COMP_3DPIT_INS_MODE, 2);
        else if (SimDriver.GetPlayerAircraft()->INSState(AircraftClass::INS_Nav))
            vrCockpit->SetSwitchMask(COMP_3DPIT_INS_MODE, 4);
        else if (SimDriver.GetPlayerAircraft()->INSState(AircraftClass::INS_AlignFlight))
            vrCockpit->SetSwitchMask(COMP_3DPIT_INS_MODE, 8);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_INS_MODE, 1);

        // LEF lock switch
        if (SimDriver.GetPlayerAircraft()->LEFLocked)
            vrCockpit->SetSwitchMask(COMP_3DPIT_LEF_FLAPS, 2);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_LEF_FLAPS, 1);

        // Alt flaps switch
        if (SimDriver.GetPlayerAircraft()->TEFExtend == TRUE)
            vrCockpit->SetSwitchMask(COMP_3DPIT_ALT_FLAPS, 2);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_ALT_FLAPS, 1);

        // AP Trim switch
        if (SimDriver.GetPlayerAircraft()->TrimAPDisc == TRUE)
            vrCockpit->SetSwitchMask(COMP_3DPIT_TRIM_AP, 2);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_TRIM_AP, 1);

        // Pitch trim
        vrCockpit->SetDOFangle(COMP_3DPIT_TRIM_PITCH, cockpitFlightData.TrimPitch);
        val = (int)(5.0f + (cockpitFlightData.TrimPitch * 10.0f)); // 5 + (+/-5)
        vrCockpit->SetSwitchMask(COMP_3DPIT_TRIM_PITCH_SW, val);
        // Yaw trim
        vrCockpit->SetDOFangle(COMP_3DPIT_TRIM_YAW, cockpitFlightData.TrimYaw);
        val = (int)(5.0f + (cockpitFlightData.TrimYaw * 10.0f)); // 5 + (+/-5)
        vrCockpit->SetSwitchMask(COMP_3DPIT_TRIM_YAW_SW, (int)(cockpitFlightData.TrimYaw));
        // Roll trim
        vrCockpit->SetDOFangle(COMP_3DPIT_TRIM_ROLL, cockpitFlightData.TrimRoll);
        val = (int)(5.0f + (cockpitFlightData.TrimRoll * 10.0f)); // 5 + (+/-5)
        vrCockpit->SetSwitchMask(COMP_3DPIT_TRIM_ROLL_SW, (int)(cockpitFlightData.TrimRoll));
        // Comm - Missile volume
        val = 1 << (8 - SimDriver.GetPlayerAircraft()->MissileVolume);
        vrCockpit->SetSwitchMask(COMP_3DPIT_MISSILE_VOL, val);
        // Comm - Threat volume
        val = 1 << (8 - SimDriver.GetPlayerAircraft()->ThreatVolume);
        vrCockpit->SetSwitchMask(COMP_3DPIT_THREAT_VOL, val);
        // Comm1 volume switch
        val = 1 << (8 - OTWDriver.pCockpitManager->mpIcp->Comm1Volume);
        vrCockpit->SetSwitchMask(COMP_3DPIT_COMM1_VOL, val);
        // Comm2 volume switch
        val = 1 << (8 - OTWDriver.pCockpitManager->mpIcp->Comm2Volume);
        vrCockpit->SetSwitchMask(COMP_3DPIT_COMM2_VOL, val);
        // Fuel transfer switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_FUEL_EXT_TRANS, 1 << SimDriver.GetPlayerAircraft()->af->IsEngineFlag(AirframeClass::WingFirst));
        // Sym wheel switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_ICP_SYM_WHEEL, 1 << ((int)(TheHud->SymWheelPos * 10.0f)));

        // Canopy switch
        if (SimDriver.GetPlayerAircraft()->af->canopyState == true)
            vrCockpit->SetSwitchMask(COMP_3DPIT_CANOPY, 2);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_CANOPY, 1);

        // Drag chute switch
        if (SimDriver.GetPlayerAircraft()->af->dragChute == AirframeClass::DRAGC_STOWED)
            vrCockpit->SetSwitchMask(COMP_3DPIT_DRAGCHUTE, 1);
        else
            vrCockpit->SetSwitchMask(COMP_3DPIT_DRAGCHUTE, 2);

        // ICP Previous/Next rocker OFF
        vrCockpit->SetSwitchMask(COMP_3DPIT_ICP_NEXT, 1);
        // ICP DED rocker OFF
        vrCockpit->SetSwitchMask(COMP_3DPIT_ICP_DED, 1);
        // TACAN channel
        val = gNavigationSys->GetTacanChannel(NavigationSystem::AUXCOMM, 2);
        vrCockpit->SetDOFangle(COMP_3DPIT_TACAN_LEFT, val * 0.6283F);
        val = gNavigationSys->GetTacanChannel(NavigationSystem::AUXCOMM, 1);
        vrCockpit->SetDOFangle(COMP_3DPIT_TACAN_CENTER, val * 0.6283F);
        val = gNavigationSys->GetTacanChannel(NavigationSystem::AUXCOMM, 0);
        vrCockpit->SetDOFangle(COMP_3DPIT_TACAN_RIGHT, val * 0.6283F);
        // Ejection Seat Arm switch
        vrCockpit->SetSwitchMask(COMP_3DPIT_SEAT_ARM, SimDriver.GetPlayerAircraft()->SeatArmed + 1);

        // end New 3D pit switch/knob animation
    } //ATARIBABY new 3dpit end


    // Scale to 3D world coords the rtt positions
    Tpoint Pan = headPan;
    Pan.z *= RTT_POSITION_SCALING;
    Pan.y *= RTT_POSITION_SCALING;
    Pan.x *= RTT_POSITION_SCALING;
    // Artscout - 2026 (VR): per-eye IPD so the RTT display panels CONVERGE at their depth instead of
    // diverging per eye. headPan carries only head lean (IPD was split out to headOrigin for the world
    // camera); the displays render from Pan, so inject the IPD here on the body-right axis (Pan.y).
    // Sign/magnitude tunable (g_fVrDisplayIpd) -- verify with the desktop mirror.
    {
        extern float g_fVrDisplayIpd; extern bool g_bVrFrameActive;
        int dxeye = (g_bVrFrameActive and g_pOpenXRBackend) ? g_pOpenXRBackend->CurrentEye() : -1;
        if (dxeye >= 0)
            Pan.y += g_pOpenXRBackend->GetEyeLateralOffsetFeet(dxeye) * g_fVrDisplayIpd;
    }
    // Artscout - 2026 (VR #61): world-frame RTT panels. Draw the RTT quads with the SAME camera as the
    // BSP cockpit (headOrigin) and let DrawRttQuad map the canvas into the real cockpit world
    // (ownshipRot * canvas/RTT_POSITION_SCALING) -> the panel's stereo DEPTH matches the physical
    // panel (no "symbology in front" + no g_fVrDisplayIpd hack). Legacy Pan path stays when off.
    // Artscout - 2026 (HUD 3D glass): the collimated HUD REQUIRES the world-frame camera (headOrigin), so
    // g_bHud3DGlass implies the world-cam path -- no separate g_bVrRttWorldCam needed. (World-cam is also
    // the #61 depth fix for the panels, so this is strictly better.)
    extern bool g_bVrRttWorldCam, g_bHud3DGlass;
    const bool rttWorldCam = g_bVrRttWorldCam or g_bHud3DGlass;
    if (rttWorldCam)
    {
        extern Trotation g_rttWorldRot; extern float g_rttWorldScale;
        g_rttWorldRot   = OTWDriver.ownshipRot;
        g_rttWorldScale = 1.0f / RTT_POSITION_SCALING;
        // World camera (cameraRot = ownshipRot*headMatrix) because the canvas is mapped into WORLD
        // (ownshipRot*canvas). Using headMatrix (body) here rotated the panels off by the heading.
        renderer->SetCamera(&headOrigin, &cameraRot);
    }
    else
        renderer->SetCamera(&Pan, &headMatrix);


    // ASSO: BEGIN
    if (renderer->HasRttTarget())
    {
        renderer->EndDraw(); //588
        renderer->StartRtt(renderer);


        // DX - COBRA - RED - The AA texture corruption Problem?
        renderer->StartDraw();
        renderer->SetBackground(0x00000000);
        renderer->ClearDraw(); //588
        // renderer->ClearZBuffer();

        //
        // Do HUD
        //
        if (vHUDrenderer) // JPO - use basic info
        {
            vHUDrenderer->AdjustRttViewport();

            if (g_b3DRTTCockpitDebug)
            {
                //ATARIBABY debug frame around surface
                vHUDrenderer->SetColor(0x0000ffff);
                vHUDrenderer->Line(-0.995F, -0.995F, 0.995F, -0.995F);
                vHUDrenderer->Line(-0.995F, 0.995F, 0.995F, 0.995F);
                vHUDrenderer->Line(-0.995F, 0.995F, -0.995F, -0.995F);
                vHUDrenderer->Line(0.995F, 0.995F, 0.995F, -0.995F);
            }

            vHUDrenderer->SetColor(TheHud->GetHudColor());

            // Get the RTT Canvas coords, UL / UR / LL
            Tpoint pt[3];
            vHUDrenderer->GetRttCanvas(pt);

            // set the HUD half angle
            float hudangy, hudangx, ratio, VRatio;
            hudangy = (pt[1].y - pt[0].y) * 0.50f;
            hudangx = pt[1].x;

            // the hud half angle -- ratio of tangents (?)
            ratio = (hudangy / hudangx);

            // RV - RED - the Hud texture is supposed to be square
            // may be it's drawn not square... to keep Hud symbology aligned with OTW
            // calculate the verticale ratio and assign it as Hud Vertical aspect ratio
            VRatio = (pt[1].y - pt[0].y) / (pt[2].z - pt[0].z);

            TheHud->SetHalfAngle((float)atan(ratio) * RTD, 1.0f, VRatio);
            // TheHud->SetHalfAngle(atan (hudangy/hudangx) * RTD);

            // hack  move borsight height to boresighty from 3dckpit.dat. default 0.75f
            hudWinY[BORESIGHT_CROSS_WINDOW] = vBoresightY; // ASSO:

            TheHud->SetTarget(TheHud->Ownship()->targetPtr);
            //vcInfo.vHUDrenderer->SetFont(pCockpitManager->HudFont());
            vHUDrenderer->SetFont(pCockpitManager->HudFont());

            // infinite projection - Hud Offset - Hud is offsetted same value as Head
            // This makes Hud to be always aligned with observer center
            // Artscout - 2026: gated + tunable (FFViper.cfg HudCollimate / HudCollimateScale). The
            // collimation only shifts with 6DOF head TRANSLATION (TrackIR/VR/bobbing); with no head
            // movement the offset is 0 and the HUD sits on the glass (correct -- no parallax). Scale
            // exaggerates the shift to see/verify the effect; HudCollimate 0 disables it for compare.
            extern bool  g_bHudCollimate;
            extern float g_fHudCollimateScale;
            extern bool  g_bHud3DGlass, g_bVrRttWorldCam;
            float XOffset = 0.0f, YOffset = 0.0f;

            // Artscout - 2026: the fake 2D collimation (origin shift) is REPLACED by true optical
            // collimation when the 3D glass is ACTIVE (VR + world-frame RTT camera): the HUD quad is then
            // composited at infinity along the boresight (g_rttWorldOfs=headOrigin at the composite below),
            // so leave the 2D offset at 0 to avoid double-compensation. In the flat path the 3D glass is
            // never active, so the original fake collimation keeps working there.
            const bool hudGlassActive = g_bHud3DGlass
                and g_pD3D11Backend and g_pD3D11Backend->XrEyeActive();
            if (g_bHudCollimate and not hudGlassActive)
            {
                XOffset = g_fHudCollimateScale * 12.0f * headPan.y / (pt[1].y - pt[0].y) * tanf(DTR * 60.0f);
                YOffset = g_fHudCollimateScale * 12.0f * headPan.z / (pt[0].z - pt[2].z) * tanf(DTR * 60.0f);
            }

            vHUDrenderer->AdjustOriginInViewport(XOffset, YOffset);

            TheHud->Display(vHUDrenderer, true);

            VirtualDisplay::SetFont(oldFont);
            renderer->SetColor(0xff00ff00);
            // restore hud half angle
            TheHud->SetHalfAngle((float)atan(0.25 * (float)tan(30.0F * DTR)) * RTD);
            // hack  restore borsight height to 0.60.  sigh.
            hudWinY[BORESIGHT_CROSS_WINDOW] = 0.60f;

        }


        //
        // Do RWR
        //
        rwr = (PlayerRwrClass*)FindSensor((SimMoverClass *)otwPlatform.get(), SensorClass::RWR);

        if (vRWRrenderer and rwr)
        {
            vRWRrenderer->AdjustRttViewport();

            if (g_b3DRTTCockpitDebug)
            {
                //ATARIBABY debug frame around surface
                vRWRrenderer->SetColor(0x0000ffff);
                vRWRrenderer->Line(-0.995F, -0.995F, 0.995F, -0.995F);
                vRWRrenderer->Line(-0.995F, 0.995F, 0.995F, 0.995F);
                vRWRrenderer->Line(-0.995F, 0.995F, -0.995F, -0.995F);
                vRWRrenderer->Line(0.995F, 0.995F, 0.995F, -0.995F);
            }

            vRWRrenderer->SetColor(pVColors[OTWDriver.renderer->GetGreenMode() not_eq 0][8]);
            rwr->SetGridVisible(FALSE);
            vHUDrenderer->SetFont(pCockpitManager->MFDFont());
            rwr->Display(vRWRrenderer);
            VirtualDisplay::SetFont(oldFont);
        }

        //
        // Do DED
        //
        //I aligned DED and PFL column readouts for 3Dpit RTT they looks exactly as 2d DED .
        //If fonts get too big for 3d RTT, not change anything and wait for
        //configurable fonts for RTT, please. Thanx
        //PLF and DED can be fitted into each "character boxes" (26x5 matrix) by changing
        //PLF and DED RTT surface resolution to match pixel size of fonts.

        if (pCockpitManager->mpIcp and vDEDrenderer)
        {
            vDEDrenderer->AdjustRttViewport();
            vHUDrenderer->SetFont(pCockpitManager->DEDFont());

            if (g_b3DRTTCockpitDebug)
            {
                //ATARIBABY debug frame around surface
                vDEDrenderer->SetColor(0x0000ffff);
                vDEDrenderer->Line(-0.995F, -0.995F, 0.995F, -0.995F);
                vDEDrenderer->Line(-0.995F, 0.995F, 0.995F, 0.995F);
                vDEDrenderer->Line(-0.995F, 0.995F, -0.995F, -0.995F);
                vDEDrenderer->Line(0.995F, 0.995F, 0.995F, -0.995F);
            }

            if ( not g_bRealisticAvionics)
            {
                pCockpitManager->mpIcp->Exec();
                //MI changed for ICP Stuff
                pCockpitManager->mpIcp->GetDEDStrings(dedStr1, dedStr2, dedStr3);

                // Check for DED/Avionics failure
                F4Assert(SimDriver.GetPlayerAircraft());
                F4Assert(SimDriver.GetPlayerAircraft()->mFaults);

                // DED is orange :)
                vDEDrenderer->SetColor(pVColors[OTWDriver.renderer->GetGreenMode() not_eq 0][7]);

                if ( not SimDriver.GetPlayerAircraft()->mFaults->GetFault(FaultClass::ufc_fault))
                {
                    vDEDrenderer->TextLeft(-0.90F, 0.99F, dedStr1, FALSE);
                    vDEDrenderer->TextLeft(-0.90F, 0.33F, dedStr2, FALSE);
                    vDEDrenderer->TextLeft(-0.90F, -0.33F, dedStr3, FALSE);
                }
            }
            else
            {
                //MI modified for ICP Stuff
                if ( not SimDriver.GetPlayerAircraft()->mFaults->GetFault(FaultClass::ufc_fault) and 
                    SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::UFCPower))
                {
                    pCockpitManager->mpIcp->Exec();

                    // Check for DED/Avionics failure
                    F4Assert(SimDriver.GetPlayerAircraft());
                    F4Assert(SimDriver.GetPlayerAircraft()->mFaults);

                    // DED is orange :)
                    vDEDrenderer->SetColor(pVColors[OTWDriver.renderer->GetGreenMode() not_eq 0][7]);

                    //ATARIBABY
                    float stepx;
                    stepx = 0.0753F;

                    float x;
                    float y = 0.90F;

                    char buf[2];

                    for (int j = 0; j < 5; j++)
                    {
                        x = -0.98F;

                        for (int i = 0; i < 26; i++)
                        {
                            buf[0] = pCockpitManager->mpIcp->DEDLines[j][i];
                            buf[1] = '\0';

                            if (buf[0] not_eq ' ' and pCockpitManager->mpIcp->Invert[j][i] == 0)
                                vDEDrenderer->TextLeft(x, y, buf, pCockpitManager->mpIcp->Invert[j][i]);
                            else if (pCockpitManager->mpIcp->Invert[j][i] == 2)
                                vDEDrenderer->TextLeft(x, y, buf, pCockpitManager->mpIcp->Invert[j][i]);

                            x += stepx;
                        }

                        y -= 0.325F;
                    }

                    //ATARIBABY end
                }
            }

            renderer->SetColor(pVColors[OTWDriver.renderer->GetGreenMode() not_eq 0][6]);
            VirtualDisplay::SetFont(oldFont); // ASSO:
        }

        //
        // Do PFL
        //

        if (hasPFL and pCockpitManager->mpIcp and vPFLrenderer)
        {
            vPFLrenderer->AdjustRttViewport();
            vHUDrenderer->SetFont(pCockpitManager->DEDFont());

            if (g_b3DRTTCockpitDebug)
            {
                //ATARIBABY debug frame around surface
                vPFLrenderer->SetColor(0x0000ffff);
                vPFLrenderer->Line(-0.995F, -0.995F, 0.995F, -0.995F);
                vPFLrenderer->Line(-0.995F, 0.995F, 0.995F, 0.995F);
                vPFLrenderer->Line(-0.995F, 0.995F, -0.995F, -0.995F);
                vPFLrenderer->Line(0.995F, 0.995F, 0.995F, -0.995F);
            }

            if ( not g_bRealisticAvionics)
            {
                pCockpitManager->mpIcp->Exec();
                //MI changed for ICP Stuff
                pCockpitManager->mpIcp->GetDEDStrings(dedStr1, dedStr2, dedStr3);

                // Check for DED/Avionics failure
                F4Assert(SimDriver.GetPlayerAircraft());
                F4Assert(SimDriver.GetPlayerAircraft()->mFaults);

                // DED is orange :)
                vPFLrenderer->SetColor(pVColors[OTWDriver.renderer->GetGreenMode() not_eq 0][7]);

                {
                    vPFLrenderer->TextLeft(-0.90F, 0.99F, dedStr1, FALSE);
                    vPFLrenderer->TextLeft(-0.90F, 0.33F, dedStr2, FALSE);
                    vPFLrenderer->TextLeft(-0.90F, -0.33F, dedStr3, FALSE);
                }
            }
            else
            {
                if (SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::PFDPower))
                {
                    pCockpitManager->mpIcp->ExecPfl(); //ATARIBABY ExecPfl() instead Exec() is needed

                    // Check for DED/Avionics failure
                    F4Assert(SimDriver.GetPlayerAircraft());
                    F4Assert(SimDriver.GetPlayerAircraft()->mFaults);

                    // PFL is orange :)
                    vPFLrenderer->SetColor(pVColors[OTWDriver.renderer->GetGreenMode() not_eq 0][7]);

                    //ATARIBABY
                    float stepx;
                    stepx = 0.0753F;

                    float x;
                    float y = 0.90F;

                    char buf[2];

                    for (int j = 0; j < 5; j++)
                    {
                        x = -0.98F;

                        for (int i = 0; i < 26; i++)
                        {
                            buf[0] = pCockpitManager->mpIcp->PFLLines[j][i];
                            buf[1] = '\0';

                            if (buf[0] not_eq ' ' and pCockpitManager->mpIcp->PFLInvert[j][i] == 0)
                                vPFLrenderer->TextLeft(x, y, buf, pCockpitManager->mpIcp->PFLInvert[j][i]);
                            else if (pCockpitManager->mpIcp->PFLInvert[j][i] == 2)
                                vPFLrenderer->TextLeft(x, y, buf, pCockpitManager->mpIcp->PFLInvert[j][i]);

                            x += stepx;
                        }

                        y -= 0.325F;
                    }

                    //ATARIBABY end

                    VirtualDisplay::SetFont(oldFont); // ASSO:
                }
            }

            renderer->SetColor(pVColors[OTWDriver.renderer->GetGreenMode() not_eq 0][6]);
        }


        // DX - COBRA - RED - The AA texture corruption Problem?
        // renderer->FinishFrame();

        //
        // Do MFDs
        //
        // don't need to update pos here since it's always relative
        // to origin and identity matrix
        // MfdDisplay[i]->UpdateVirtualPosition(&Origin, &IMatrix);

        if (g_b3dMFDLeft)
        {
            //MfdDisplay[0]->SetImageBuffer(OTWImage, viewportBounds.left, viewportBounds.top, viewportBounds.right, viewportBounds.bottom);
            VirtualDisplay::SetFont(0);//pCockpitManager->MFDFont());
            MfdDisplay[0]->Exec(FALSE, TRUE); // ASSO:
            VirtualDisplay::SetFont(oldFont);
        }

        if (g_b3dMFDRight)
        {
            //MfdDisplay[1]->SetImageBuffer(OTWImage, viewportBounds.left, viewportBounds.top, viewportBounds.right, viewportBounds.bottom);
            VirtualDisplay::SetFont(0);//pCockpitManager->MFDFont());
            MfdDisplay[1]->Exec(FALSE, TRUE); // ASSO:
            VirtualDisplay::SetFont(oldFont);
        }


        // renderer->FinishFrame();
        renderer->FinishRtt();

        // renderer->StartDraw();

        // Artscout - 2026 (VR HUD 3D glass): composite the HUD at OPTICAL INFINITY along the boresight.
        // Setting g_rttWorldOfs = headOrigin makes the eye offset cancel in DrawRttQuad's projection (the
        // world-frame camera is also at headOrigin), so the symbology collimates: no per-eye convergence,
        // stable under head translation, conformal with the world. Only the HUD gets the offset; the
        // panel displays (RWR/DED/PFL) keep g_rttWorldOfs=0 so they stay fixed on their cockpit panels.
        extern bool g_bHud3DGlass;
        const bool hudGlass = g_bHud3DGlass
            and g_pD3D11Backend and g_pD3D11Backend->XrEyeActive();
        if (hudGlass)
        {
            // Arm the aperture stencil clip BEFORE the glass plate: the plate (drawn at the PHYSICAL glass
            // position, g_rttWorldOfs still 0 -> fixed in the cockpit, with parallax) writes the stencil
            // aperture bit, and DrawRttQuad clips the collimated symbology to it.
            extern bool g_bRttHudClip, g_bHud3DGlassClip;
            g_bRttHudClip = g_bHud3DGlassClip;

            // Glass plate (faint Fresnel green tint + the stencil mark). Drawn even when the tint is ~0 if
            // the clip is on, because the symbology then needs the stencil mark (alpha 0 still writes it).
            extern float g_fHud3DGlassTint, g_fHud3DGlassFresnel;
            if (vHUDrenderer and (g_fHud3DGlassTint > 0.001f or g_bRttHudClip))
            {
                // Fresnel: the glass shows MORE at grazing view angles. headMatrix.M11 = cos(angle between
                // the head look and the boresight/glass normal) -- 1 head-on, <1 when looking from above/
                // the side. Boost the tint alpha as it falls off so the pane "lights up" edge-on like glass.
                float c = headMatrix.M11; if (c < 0.0f) c = 0.0f; if (c > 1.0f) c = 1.0f;
                float ga = g_fHud3DGlassTint * (1.0f + g_fHud3DGlassFresnel * (1.0f - c) * (1.0f - c));
                if (ga > 0.9f) ga = 0.9f;
                vHUDrenderer->DrawGlassPlate(0.30f, 0.55f, 0.40f, ga);   // subtle green tint, Fresnel-boosted
            }

            extern Tpoint g_rttWorldOfs;
            g_rttWorldOfs = headOrigin;   // collimate the symbology (eye offset cancels in projection)
        }

        if (vHUDrenderer)
            vHUDrenderer->DrawRttQuad();

        if (hudGlass)
        {
            extern Tpoint g_rttWorldOfs;
            g_rttWorldOfs.x = g_rttWorldOfs.y = g_rttWorldOfs.z = 0.0f;   // panels stay fixed on the cockpit
            extern bool g_bRttHudClip;
            g_bRttHudClip = false;                                        // panels composite without Z-test
        }

        if (vRWRrenderer)
        {
            // Artscout - 2026 (VR #61 RWR): nudge the RWR symbology canvas onto the BSP scope (its 3Dckpit.dat
            // depth floats it in front). Only around the RWR quad; reset so other panels are unaffected.
            extern float g_rttCanvasFwd, g_fVrRwrFwd;
            g_rttCanvasFwd = g_fVrRwrFwd;
            vRWRrenderer->DrawRttQuad();
            g_rttCanvasFwd = 0.0f;
        }

        if (vDEDrenderer)
            vDEDrenderer->DrawRttQuad();

        if (vPFLrenderer)
            vPFLrenderer->DrawRttQuad();

        if (g_b3dMFDLeft)
        {
            // Artscout - 2026: composite via MFDClass so THIS MFD's atlas zone/3D-panel canvas are
            // re-applied first (the display may be SHARED via mavDisplay between both MFDs -> the other
            // MFD otherwise composited the wrong zone = WPN black when SMS also showed the Maverick).
            if (MfdDisplay[0]->GetDrawable() and MfdDisplay[0]->GetDrawable()->GetDisplay())
            {
                MfdDisplay[0]->DrawRttComposite();
            }
        }

        if (g_b3dMFDRight)
        {
            if (MfdDisplay[1]->GetDrawable() and MfdDisplay[1]->GetDrawable()->GetDisplay())
            {
                MfdDisplay[1]->DrawRttComposite();
            }
        }

        // DIAG (RTT): raw atlas overlay -- DISABLED (diagnosis obtained: uneven height = texel-bleed
        // of font rows, fixed by an inset). Enable if needed.
        //renderer->DrawRttDebugOverlay();
    }

    // ASSO: END


    if ( not g_bUseNew3dpit) //ATARIBABY start Disabled if using new 3dpit code
    {
        if (vcInfo.vMACHrenderer)
        {
            /* Do MACH indictator */
            float GetKias = ((AircraftClass *)otwPlatform.get())->af->vcas;

            GetKias = (float)fmod(GetKias, 1000.0f);
            GetKias = GetKias * 0.001f * 2.0F * PI;

            x1 = 0.0f;
            y1 = 0.0f;
            mlSinCos(&trig, GetKias);
            x2 = 0.85f * trig.cos;
            y2 = 0.85f * -trig.sin;

            vcInfo.vMACHrenderer->Line(x1, y1, x2, y2);
        }

        renderer->SetColor(pVColors[OTWDriver.renderer->GetGreenMode() not_eq 0][5]);

        for (i = 0; static_cast<unsigned int>(i) < mpVDials.size(); i++)
            mpVDials[i]->Exec(SimDriver.GetPlayerAircraft());
    }

    //ATARIBABY end

    //Wombat778 10-11-2003 The meat of the clickable cockpit.  Looks for the closest button and executes it.  Also displays button locations in debug mode.
    //Why execute the commands here, you may ask.  Well, because I spent all night trying to get it to run from simouse.cpp and couldnt (strange memory corruption)
    //So, here it is.  It is a hack, but it works.  If you don't like, then YOU fix it;-)

    ThreeDVertex t1;

    // Artscout - 2026: THREE explicit mouse-pick modes -- FLAT (stock F4 desktop), XR stereo, XR quad.
    // FLAT is the pristine reference hit-test: no VR projection, no IPD parallax, no detect bias, no cursor
    // magnet, the stock radius, and NO per-view gating (it runs on every call). The XR branches engage ONLY
    // when we are actually presenting stereo THIS frame (g_bVrFrameActive) AND an XR eye target is bound --
    // so with VR enabled in the options but the headset OFF, the pick is byte-for-byte the flat path.
    //   xrView0  : limit hover/snap/click to the periphery pass (view 0) where the visible cursor lives.
    //              In FLAT it is always true (no views); in stereo CurrentEye()<=0; in quad it selects the
    //              periphery. xrQuad/xrStereo are split out so the quad focus-projection can be tuned alone.
    extern bool g_bVrFrameActive;
    const bool xrPick   = g_bVrFrameActive and g_pD3D11Backend and g_pD3D11Backend->XrEyeActive()
                          and g_pD3D11Backend->XrEyeW() > 0 and g_pOpenXRBackend != NULL;
    // Artscout - 2026 (#58/#60): branch off the ACTUAL session view config (IsQuadViews), not the
    // g_bUseQuadViews option -- the session is created once and not recreated on an in-game toggle, so the
    // option can disagree with reality until restart. Reality keeps the mouse calibration matched to the render.
    const bool sessionQuad = xrPick and g_pOpenXRBackend->IsQuadViews();
    const bool xrQuad   = sessionQuad;
    const bool xrStereo = xrPick and not sessionQuad;
    const bool xrView0  = (not xrPick) or (g_pOpenXRBackend->CurrentEye() <= 0);
    (void)xrQuad;

    // Artscout - 2026 (#58 VR mouse): pick the clickable-cockpit calibration set for the active VR mode.
    // Quad-views was tuned against the focus view's narrow gaze FOV; plain stereo projects through the full
    // eye FOV, so it needs its own residual-bias / snap-radius / IPD scale. xrStereo -> *Stereo variants
    // (default == quad values), else the quad set. Flat path never reads these (xrPick false).
    extern float g_fVrCursorMagnet, g_fVrDetectBiasX, g_fVrDetectBiasY, g_fVrCursorIpd;
    extern float g_fVrCursorMagnetStereo, g_fVrDetectBiasXStereo, g_fVrDetectBiasYStereo, g_fVrCursorIpdStereo;
    const float vrMagnet  = xrStereo ? g_fVrCursorMagnetStereo : g_fVrCursorMagnet;
    const float vrBiasX   = xrStereo ? g_fVrDetectBiasXStereo  : g_fVrDetectBiasX;
    const float vrBiasY   = xrStereo ? g_fVrDetectBiasYStereo  : g_fVrDetectBiasY;
    const float vrCursIpd = xrStereo ? g_fVrCursorIpdStereo    : g_fVrCursorIpd;

    // Artscout - 2026 (VR mouse): per-eye IPD parallax in BUTTON units (body-right axis). Added to each
    // button's Pos.y below, exactly like headPan, so TransformCameraCentricPoint (which drops the camera
    // position) still gets the left-eye lateral shift. After the perspective divide this becomes a depth-
    // dependent screen correction -- the reason a constant DetectBias couldn't fit ICP and the MFD at once.
    float vrIpdButtonY = 0.0f;
    if (xrPick and g_pOpenXRBackend->CurrentEye() <= 0)
    {
        vrIpdButtonY = g_pOpenXRBackend->GetEyeLateralOffsetFeet(0) * B3D_POSITION_SCALING * vrCursIpd;
    }
    // Artscout - 2026 (VR): reset the cursor color ONLY in view 0 (where the hover/green test runs).
    // VCock_Exec runs once per view; resetting it every pass let the LAST pass (view 3, hover gated off)
    // leave it at the default GREEN -> cursor always green even pointing at the sky. View 0 owns the color.
    if (xrView0)
        gSelectedCursor = 9; //Wombat778 10-11-2003 set the cursor to the default green cursor

    // Artscout - 2026 (VR mouse): project the buttons with the EXACT projection the 3D cockpit BSP is
    // drawn with -- SetVRFrustum(view 0 angles) + SetCamera(headOrigin, headMatrix) (see the per-eye loop
    // in otwloop and VCock_DrawThePit). Earlier this used SetFOV(cfr-cfl), which derives the VERTICAL FOV
    // from the horizontal FOV and the screen aspect ratio. The XR eye is near-square (e.g. 1914x1890),
    // not the flat 16:9, so SetFOV's vertical scale was wrong and the projected buttons drifted vertically
    // -- the error growing toward the screen edges (the cursor had to sit well ABOVE a button to hit it).
    // SetVRFrustum reproduces the EXACT projection the cockpit BSP is rendered with: independent H/V FOV
    // (2/(tanR-tanL), 2/(tanU-tanD)) AND the left-eye stereo off-axis (asymmetric frustum). The off-axis is
    // REQUIRED: ground truth ([VRICP]) showed the left-eye horizontal off-axis shifts the cockpit ~330px,
    // and the buttons must get the same shift to line up with the visible cockpit/cursor. NOTE: the projected
    // ThreeDVertex comes out in DISPLAY pixel space here (the renderer's xRes/yRes is DispWidth/DispHeight
    // during VCock_Exec, NOT the eye), so the hit-test below compares t1.x/y to gxPos/gyPos DIRECTLY -- no
    // eye->display rescale (an earlier *dw/ew double-scaled the buttons ~1.337x and spread them out).
    if (xrPick and g_pOpenXRBackend->CurrentEye() <= 0)
    {
        float cfl, cfr, cfu, cfd;
        if (g_pOpenXRBackend->GetEyeFovAngles(0, &cfl, &cfr, &cfu, &cfd))
        {
            // Artscout - 2026 (VR): keep the HORIZONTAL off-axis (real cfl/cfr -- ground truth showed the
            // left-eye stereo cant shifts the cockpit ~330px sideways), but SYMMETRIZE the vertical (offY=0):
            // [VRICP] showed the cockpit's vertical is ~symmetric, so a symmetric vertical matches best while
            // preserving the true VFOV (height tanU-tanD unchanged: angU'=-angD'=atan((tanU-tanD)/2)).
            float vh = (float)atan((tan(cfu) - tan(cfd)) * 0.5f);
            renderer->SetVRFrustum(cfl, cfr, vh, -vh);
        }
        renderer->SetCamera(&headOrigin, &headMatrix);
    }
    else if (not xrPick)
    {
        // Artscout - 2026 (FLAT mouse): the RTT display DrawRttQuads above (HUD world-cam #61, MFD atlas)
        // leave the renderer's camera/projection in DISPLAY-CANVAS space. Re-assert the cockpit perspective
        // (GetFOV -- the SAME value the hit-test radius below uses) + the head orientation so the buttons
        // project into the SAME DispWidth screen space as gxPos/gyPos. The stock F4 path drew the dials
        // INLINE with the cockpit camera, so it never needed this; our RTT-atlas display path disturbs the
        // camera, so without this restore the buttons projected to garbage and the hover/click never matched
        // (cursor stayed yellow, never turned green on a switch). This is the FLAT mirror of the VR branch.
        renderer->SetFOV(GetFOV());
        renderer->SetCamera(&headOrigin, &headMatrix);
    }

    // Artscout - 2026 (VR controllers): laser-pointer pick. When a controller is tracked, the ACTIVE hand's
    // aim ray picks the nearest 3D clickable button and drives the SAME cursor/click machinery as the VR
    // mouse (g_vrCursorAnchor / g_vrCursorAnchorButton / g_vrCursorAnchorSnapped + Button3DList.clicked).
    // Trigger = primary (left) click; thumbstick up/right = left (increment), down/left = right (decrement);
    // A = zoom toggle, B = recenter. Falls back to the mouse when no controller (g_vrCtrlRayActive stays
    // false, the mouse hover/anchor blocks below run as before). View 0 only, like the mouse.
    g_vrCtrlRayActive = false;
    if (xrView0) { g_vrRayActive = false; g_vrGripValid = false; g_vrHitValid = false; }   // reset once/frame (view 0); persists for other eyes
    {
        extern bool  g_bVrControllers, g_bVrRayFlipH, g_bVrRayFlipV;
        extern float g_fVrRayRadius, g_fVrRayReach, g_fVrThumbThresh, g_fVrKnobRepeatMs, g_fVrRayOriginOfs;
        if (xrPick and xrView0 and g_bVrControllers and g_pOpenXRBackend->ControllerActive())
        {
            int   hnd = g_pOpenXRBackend->GetActiveHand();
            float ao[3], ad[3];
            if (g_pOpenXRBackend->GetControllerAimBody(hnd, ao, ad))
            {
                g_vrCtrlRayActive = true;
                // In-headset axis calibration: flip the ray's right (H) / up-down (V) axis if inverted.
                if (g_bVrRayFlipH) { ao[1] = -ao[1]; ad[1] = -ad[1]; }
                if (g_bVrRayFlipV) { ao[2] = -ao[2]; ad[2] = -ad[2]; }
                const float sc = B3D_POSITION_SCALING;
                Tpoint rD; rD.x = ad[0]; rD.y = ad[1]; rD.z = ad[2];               // unit dir (body button axes)
                // Fine-align the ray to the hand model's FINGER: build a right/up frame off the aim dir, tilt
                // the direction (VrRayPitch/Yaw), and shift the origin sideways (VrRayOriginUp/Right) + forward
                // (VrRayOriginOfs) so the beam leaves the fingertip instead of the wrist. All runtime cfg knobs.
                extern float g_fVrRayPitch, g_fVrRayYaw, g_fVrRayOriginUp, g_fVrRayOriginRight;
                // Roll-AWARE frame: use the grip ORIENTATION (right/up), which rotates WITH the hand, so the
                // origin offset + tilt stay on the finger when you roll your wrist. (A world-up-derived frame
                // ignored roll -> the beam slid off the finger when the palm turned.) Fallback = world-up frame.
                Tpoint rRt, rUp;
                float gbf[3], gbr[3], gbu[3];
                if (g_pOpenXRBackend->GetControllerGripBasis(hnd, gbf, gbr, gbu))
                {
                    if (g_bVrRayFlipH) { gbr[1] = -gbr[1]; gbu[1] = -gbu[1]; }
                    if (g_bVrRayFlipV) { gbr[2] = -gbr[2]; gbu[2] = -gbu[2]; }
                    rRt.x = gbr[0]; rRt.y = gbr[1]; rRt.z = gbr[2];
                    rUp.x = gbu[0]; rUp.y = gbu[1]; rUp.z = gbu[2];
                }
                else
                {
                    Tpoint wU = { 0.0f, 0.0f, -1.0f };
                    rRt.x = rD.y*wU.z - rD.z*wU.y; rRt.y = rD.z*wU.x - rD.x*wU.z; rRt.z = rD.x*wU.y - rD.y*wU.x;
                    float rl = sqrtf(rRt.x*rRt.x + rRt.y*rRt.y + rRt.z*rRt.z);
                    if (rl < 1e-3f) { rRt.x = 0.0f; rRt.y = 1.0f; rRt.z = 0.0f; rl = 1.0f; }
                    rRt.x/=rl; rRt.y/=rl; rRt.z/=rl;
                    rUp.x = rRt.y*rD.z - rRt.z*rD.y; rUp.y = rRt.z*rD.x - rRt.x*rD.z; rUp.z = rRt.x*rD.y - rRt.y*rD.x;
                }
                float yw = g_fVrRayYaw*(float)DTR, pt = g_fVrRayPitch*(float)DTR;
                Tpoint rd1 = { rD.x*cosf(yw)+rRt.x*sinf(yw), rD.y*cosf(yw)+rRt.y*sinf(yw), rD.z*cosf(yw)+rRt.z*sinf(yw) };   // yaw about up
                Tpoint rd2 = { rd1.x*cosf(pt)+rUp.x*sinf(pt), rd1.y*cosf(pt)+rUp.y*sinf(pt), rd1.z*cosf(pt)+rUp.z*sinf(pt) }; // pitch about right
                float dl = sqrtf(rd2.x*rd2.x + rd2.y*rd2.y + rd2.z*rd2.z); if (dl > 1e-6f) { rd2.x/=dl; rd2.y/=dl; rd2.z/=dl; }
                rD = rd2;
                Tpoint rO;                                                          // ray origin (button units)
                rO.x = ao[0]*sc + rD.x*g_fVrRayOriginOfs + rRt.x*g_fVrRayOriginRight + rUp.x*g_fVrRayOriginUp;
                rO.y = ao[1]*sc + rD.y*g_fVrRayOriginOfs + rRt.y*g_fVrRayOriginRight + rUp.y*g_fVrRayOriginUp;
                rO.z = ao[2]*sc + rD.z*g_fVrRayOriginOfs + rRt.z*g_fVrRayOriginRight + rUp.z*g_fVrRayOriginUp;
                g_vrRayOrigin = rO; g_vrRayDir = rD; g_vrRayActive = true;
                // Controller marker position (grip pose), same frame/flips as the ray.
                float go[3];
                if (g_pOpenXRBackend->GetControllerGripBody(hnd, go))
                {
                    if (g_bVrRayFlipH) go[1] = -go[1];
                    if (g_bVrRayFlipV) go[2] = -go[2];
                    g_vrGripPoint.x = go[0] * sc; g_vrGripPoint.y = go[1] * sc; g_vrGripPoint.z = go[2] * sc;
                    g_vrGripValid = true;
                }

                // Read inputs first so the single pick loop can also resolve the fire-button (left/right variant).
                OpenXRBackend::ControllerState cs; cs.triggerDown = false;
                int fireMb = 0;   // 1 = left (increment/press), 2 = right (decrement)
                if (g_pOpenXRBackend->GetControllerState(hnd, &cs))
                {
                    static bool s_prevTrig = false, s_prevA = false, s_prevB = false;
                    static int  s_thumbDir = 0, s_holdFrames = 0;
                    if (cs.triggerDown and not s_prevTrig) fireMb = 1;
                    s_prevTrig = cs.triggerDown;

                    int dir = 0;
                    if (fabs(cs.thumbY) >= fabs(cs.thumbX)) { if (cs.thumbY > g_fVrThumbThresh) dir = 1; else if (cs.thumbY < -g_fVrThumbThresh) dir = -1; }
                    else                                    { if (cs.thumbX > g_fVrThumbThresh) dir = 1; else if (cs.thumbX < -g_fVrThumbThresh) dir = -1; }
                    int repFrames = (int)(g_fVrKnobRepeatMs * 0.09f); if (repFrames < 1) repFrames = 1;  // ms->frames @~90fps
                    if (dir != 0)
                    {
                        bool rep = false;
                        if (dir != s_thumbDir) { rep = true; s_holdFrames = 0; }
                        else if (++s_holdFrames >= repFrames) { rep = true; s_holdFrames = 0; }
                        if (rep and fireMb == 0) fireMb = (dir > 0) ? 1 : 2;
                    }
                    s_thumbDir = dir;

                    if (cs.buttonB and not s_prevB) g_pOpenXRBackend->Recenter();
                    s_prevB = cs.buttonB;
                    if (cs.buttonA and not s_prevA) g_bVrZoomActive = not g_bVrZoomActive;  // zoom application TBD
                    s_prevA = cs.buttonA;
                }

                // Pick the button the ray points MOST DIRECTLY at (smallest ANGULAR deviation perp/t), not the
                // nearest one along the ray. With the hand close to a dense panel, a nearest-t + absolute-radius
                // test lets many switches qualify and picks whichever is closest to the HAND -> "aims by hand
                // position, no precision". Angular selection makes it a true laser pointer: rotate the wrist and
                // the selected switch changes by DIRECTION, range-independent. The absolute radius gate stays as
                // a coarse "the ray passes within the switch's (scaled) hotspot" filter. bestFire = same, matched
                // to the fire mousebutton (2-way toggle left/right variant on one loc).
                int    bestAny = -1;  float bestAnyAng = 1.0e30f; float bestAnyT = 0.0f; Tpoint anyPos = { 0, 0, 0 };
                int    bestFire = -1; float bestFireAng = 1.0e30f; float bestFireT = 0.0f; Tpoint firePos = { 0, 0, 0 };
                for (i = 0; i < Button3DList.numbuttons; i++)
                {
                    Tpoint P = Button3DList.buttons[i].loc;
                    P.x += headPan.x * sc; P.y += headPan.y * sc; P.z += headPan.z * sc;
                    float wx = P.x - rO.x, wy = P.y - rO.y, wz = P.z - rO.z;
                    float t  = wx * rD.x + wy * rD.y + wz * rD.z;
                    if (t <= 1.0f) continue;                       // behind / on top of the controller
                    float qx = wx - t * rD.x, qy = wy - t * rD.y, qz = wz - t * rD.z;
                    float perp = sqrtf(qx * qx + qy * qy + qz * qz);
                    if (perp >= Button3DList.buttons[i].dist * g_fVrRayRadius) continue;
                    float ang = perp / t;                          // ~tan(angle off the ray) -> how directly aimed
                    if (ang < bestAnyAng) { bestAnyAng = ang; bestAnyT = t; bestAny = i; anyPos = P; }
                    if (fireMb and Button3DList.buttons[i].mousebutton == fireMb and ang < bestFireAng) { bestFireAng = ang; bestFireT = t; bestFire = i; firePos = P; }
                }

                // NO snapping (it only got in the way): the cursor stays ON THE BEAM -- at the hit depth when a
                // button is under it, else a fixed reach -- so it shows exactly where you point. Click uses bestAny.
                float anchorT = (bestAny >= 0) ? bestAnyT : g_fVrRayReach;
                g_vrCursorAnchor.x = rO.x + rD.x * anchorT;
                g_vrCursorAnchor.y = rO.y + rD.y * anchorT;
                g_vrCursorAnchor.z = rO.z + rD.z * anchorT;
                g_vrCursorAnchorValid   = true;
                g_vrCursorAnchorButton  = bestAny;
                g_vrCursorAnchorSnapped = (bestAny >= 0);
                if (bestAny >= 0)
                {
                    gSelectedCursor = 9;                          // green: a button is under the beam
                    g_vrHitPoint = anyPos;                        // the real switch center (headPan-adjusted) -> ring it
                    g_vrHitValid = true;
                }

                // Fire the matching-mousebutton button under the ray; the click loop below executes the snapped
                // button. Only the click target changes -- the visible cursor stays on the beam.
                if (fireMb and bestFire >= 0)
                {
                    g_vrCursorAnchorButton  = bestFire;
                    g_vrCursorAnchorSnapped = true;
                    Button3DList.clicked    = fireMb;
                }

                // Visible cursor + laser line. Project the 3D anchor to the DISPLAY pixel (SetVRFrustum is set
                // for view 0 above) and steer the shared mouse cursor there -- otwloop maps gxPos/gyPos into
                // each eye, reusing the #58 VR cursor (both eyes). Keep the cursor alive (no mouse motion in
                // VR). Then draw the GREEN ray line (view 0) from the controller to the cursor.
                // Beam + cross + controller marker are drawn PER-EYE (stereo) in the block after this one --
                // here (view 0) we only computed the pick. gTimeLastMouseMove kept fresh so any 2D fallback
                // cursor does not time out mid-aim.
                gTimeLastMouseMove = vuxRealTime;
            }
        }
    }

    // Artscout - 2026 (VR controllers): PER-EYE stereo render of the beam + endpoint cross + controller
    // marker, as real 3D points projected with THIS eye's OWN camera. VCock_Exec runs once per view, so
    // the block runs for EVERY eye and reproduces the EXACT projection the cockpit BSP was drawn with in
    // that eye (otwloop's per-eye setup: SetFOV for stereo / SetVRFrustum for quad + SetCamera(headOrigin))
    // PLUS this eye's IPD lateral offset in button units. Because TransformCameraCentricPoint drops the
    // camera POSITION, the IPD (which the BSP gets from the view matrix) is re-injected on the point here.
    // Drawing in BOTH eyes with the correct per-eye disparity lets the cross fuse binocularly AT the switch
    // -- a view-0-only (mono) cross floats at the wrong depth (rivalry), which is why it read as "off".
    // The pick (anchor / origin / grip) is computed once in view 0 above and reused for every eye pass.
    if (g_vrRayActive and g_vrCursorAnchorValid and xrPick and g_pOpenXRBackend)
    {
        // Re-assert THIS eye's projection + camera. Between VCock_DrawThePit (which set the per-eye camera)
        // and here, DrawScene and the RTT display quads disturb the renderer camera, and the view-0 setup
        // above only ran for view 0 -- so the non-left eye would otherwise project with a stale camera and
        // the cross would double. Mirror otwloop's BSP setup EXACTLY so the cross lands on the BSP button.
        int curEye = g_pOpenXRBackend->CurrentEye(); if (curEye < 0) curEye = 0;
        float efl, efr, efu, efd;
        if (g_pOpenXRBackend->GetEyeFovAngles(curEye, &efl, &efr, &efu, &efd))
        {
            // Match the projection the BSP cockpit was actually drawn with THIS eye -- exactly.
            // STEREO: the BSP uses SetFOV(efr-efl) -> a SYMMETRIC frustum (NO horizontal off-axis); the eye
            // separation comes entirely from the camera position (headOrigin IPD). Adding the eye's real
            // asymmetric off-axis here (an earlier SetVRFrustum(efl,efr,..) attempt) DOUBLED the horizontal
            // disparity -> the whole beam/cursor split in two. So keep it symmetric. The ONE thing SetFOV got
            // wrong for us: it derives the VERTICAL fov from the current xRes/yRes aspect, which during
            // VCock_Exec is DispWidth/DispHeight (~16:9), NOT the near-square eye (eyeW/eyeH) the BSP was drawn
            // into -> the cross diverged vertically (0 at centre, ~1 MFD at the bottom row). Reproduce SetFOV
            // but with the EYE aspect: symmetric H half-angle (efr-efl)/2, symmetric V half = atan(tan(H)*eh/ew).
            // QUAD: the BSP genuinely uses the off-axis per-view frustum (otwloop SetVRFrustum), so pass it through.
            if (sessionQuad)
                renderer->SetVRFrustum(efl, efr, efu, efd);
            else
            {
                float hh = (efr - efl) * 0.5f;
                int   ew = g_pD3D11Backend->XrEyeW(), eh = g_pD3D11Backend->XrEyeH();
                float vhalf = (ew > 0) ? (float)atan(tan(hh) * (double)eh / (double)ew) : hh;
                renderer->SetVRFrustum(-hh, hh, vhalf, -vhalf);   // symmetric like SetFOV, but EYE aspect
            }
        }
        renderer->SetCamera(&headOrigin, &headMatrix);

        // Artscout - 2026 (VR controller model): draw the mesh NOW -- this eye's camera is set and the cockpit
        // was already flushed (before VCock_Exec), so the mesh overlays on top. Immediate D3D11 path (DrawTL)
        // into the current eye RTV; the beam/cross below then draw over the mesh.
        VCock_DrawControllerModel();

        // This eye's IPD parallax in button units (body-right axis), the piece TransformCameraCentricPoint
        // drops -> the beam/cross stereo disparity. Scaled by VrRayIpd (its OWN knob, independent of the
        // mouse cursor's VrCursorIpd): raise/lower until the ring sits AT the switch depth; <0 flips the eye
        // sign, 0 = mono/flat. The other eye's GetEyeLateralOffsetFeet flips sign -> the stereo separation.
        extern float g_fVrRayIpd;
        float ipdY = g_pOpenXRBackend->GetEyeLateralOffsetFeet(curEye) * B3D_POSITION_SCALING * g_fVrRayIpd;

        // Endpoint: ALWAYS the point ON THE BEAM (rO + rD*t) -- no snapping to the button centre. The
        // hand-placed clickable hotspots are often offset from the visible switch art, so jumping the cursor
        // onto them reads as "stuck / off" ("прилипание"); keeping it on the beam shows exactly where the
        // controller points. g_vrCursorAnchor already sits at the hit depth when over a button (bestAnyT),
        // else at the free reach. g_vrHitValid only picks the glyph (ring = a clickable is under the beam).
        Tpoint aE = g_vrCursorAnchor; aE.y += ipdY;
        Tpoint oE = g_vrRayOrigin;    oE.y += ipdY;
        ThreeDVertex tA, tO;
        renderer->TransformCameraCentricPoint(&aE, &tA);
        if (tA.csZ < -30.0f)
        {
            // Colour signals state (0x00BBGGRR): GREEN when the ray is on a clickable switch (ring), ORANGE when
            // aiming at nothing clickable (cross). Beam takes the same colour so the whole pointer reads at a glance.
            renderer->SetColor(g_vrHitValid ? 0x0000FF00 : 0x0000A5FF);
            renderer->TransformCameraCentricPoint(&oE, &tO);
            if (tO.csZ < -30.0f)
                renderer->Render2DLine(tO.x, tO.y, tA.x, tA.y);   // the beam
            if (g_vrHitValid)
            {
                // RING at the beam end -> "a clickable switch is under the ray" (glyph only, on beam). 1.5x size.
                const float rr = 7.5f;
                const int   segs = 12;
                float pX = tA.x + rr, pY = tA.y;   // s = 0
                for (int s = 1; s <= segs; ++s)
                {
                    float a  = (float)s * (2.0f * PI / (float)segs);
                    float nX = tA.x + rr * (float)cos(a);
                    float nY = tA.y + rr * (float)sin(a);
                    renderer->Render2DLine(pX, pY, nX, nY);
                    pX = nX; pY = nY;
                }
            }
            else
            {
                const float cx = 7.5f;            // free-aim cross (pointing at nothing clickable), 1.5x size
                renderer->Render2DLine(tA.x - cx, tA.y, tA.x + cx, tA.y);
                renderer->Render2DLine(tA.x, tA.y - cx, tA.x, tA.y + cx);
            }
            // Artscout - 2026 (VR hands): PRIORITY chain per user -- real hands (XR_EXT_hand_tracking) if the
            // runtime provides them, else the wireframe controller (EXT_render_model glTF is a future slot).
            // Draw the 26-joint skeleton for every tracked hand (Index feeds it from the grip finger sensors),
            // mapped to body units + this eye's IPD exactly like the grip marker. Only the ACTIVE hand falls
            // back to the wireframe when its skeleton is absent, so you always see the pointer.
            const int VR_HAND_JOINTS = 26;
            static const int handBones[24][2] = {
                {1,2},{1,6},{1,11},{1,16},{1,21},            // wrist -> finger metacarpals
                {2,3},{3,4},{4,5},                            // thumb
                {6,7},{7,8},{8,9},{9,10},                     // index
                {11,12},{12,13},{13,14},{14,15},             // middle
                {16,17},{17,18},{18,19},{19,20},             // ring
                {21,22},{22,23},{23,24},{24,25}              // little
            };
            extern bool g_bVrRayFlipH, g_bVrRayFlipV;
            const int   activeHnd = g_pOpenXRBackend->GetActiveHand();
            bool drewActiveHand = false;
            for (int hnd2 = 0; hnd2 < 2; ++hnd2)
            {
                float jb[26][3]; bool jv[26];
                if (!g_pOpenXRBackend->GetHandJointsBody(hnd2, jb, jv)) continue;   // hand not tracked this frame
                ThreeDVertex jp[26]; bool jok[26];
                for (int i = 0; i < VR_HAND_JOINTS; ++i)
                {
                    if (!jv[i]) { jok[i] = false; continue; }
                    float by = jb[i][1], bz = jb[i][2];
                    if (g_bVrRayFlipH) by = -by;         // same axis flips as the ray/grip
                    if (g_bVrRayFlipV) bz = -bz;
                    Tpoint pj;
                    pj.x = jb[i][0] * B3D_POSITION_SCALING;
                    pj.y = by * B3D_POSITION_SCALING + ipdY;
                    pj.z = bz * B3D_POSITION_SCALING;
                    renderer->TransformCameraCentricPoint(&pj, &jp[i]);
                    jok[i] = (jp[i].csZ < -30.0f);
                }
                for (int b = 0; b < 24; ++b)
                {
                    int a0 = handBones[b][0], a1 = handBones[b][1];
                    if (jok[a0] and jok[a1]) renderer->Render2DLine(jp[a0].x, jp[a0].y, jp[a1].x, jp[a1].y);
                }
                if (hnd2 == activeHnd) drewActiveHand = true;
            }

            // Artscout - 2026 (VR controller model): middle tier of hands -> MODEL -> wireframe. When hands
            // aren't drawn, try the real controller mesh oriented by the grip pose (Index vs Oculus by the
            // runtime's interaction profile). v1 = flat-shaded solid. Falls through to the wireframe if the
            // model is off / not loaded / grip pose missing.
            extern bool g_bVrControllerModel;
            bool drewModel = false;
            if (false)   // Artscout - 2026: controller MODEL now drawn solid in VCock_DrawThePit (poly-list); this Render2DTri overlay path retired
            {
                char prof[128] = "";
                g_pOpenXRBackend->GetInteractionProfile(activeHnd, prof, sizeof(prof));
                bool isIndex = (prof[0] == 0) or (strstr(prof, "index") != NULL) or (strstr(prof, "knuckles") != NULL);
                const char* base = (activeHnd == 0)
                    ? (isIndex ? "valve_controller_knu_1_0_left"  : "oculus_cv1_controller_left")
                    : (isIndex ? "valve_controller_knu_1_0_right" : "oculus_cv1_controller_right");
                float bf[3], br[3], bu[3];
                if (VrLoadCtrlObj(&s_vrModel[activeHnd], base) and g_pOpenXRBackend->GetControllerGripBasis(activeHnd, bf, br, bu))
                {
                    if (g_bVrRayFlipH) { bf[1] = -bf[1]; br[1] = -br[1]; bu[1] = -bu[1]; }   // match the ray/grip flips
                    if (g_bVrRayFlipV) { bf[2] = -bf[2]; br[2] = -br[2]; bu[2] = -bu[2]; }
                    extern float g_fVrModelScale;
                    const float M2B = 3.28084f * B3D_POSITION_SCALING * g_fVrModelScale;    // metres -> button units
                    float Lx = 0.3f, Ly = -0.5f, Lz = -0.8f; float Ll = sqrtf(Lx*Lx + Ly*Ly + Lz*Lz); Lx/=Ll; Ly/=Ll; Lz/=Ll;
                    // STATE_ALPHA_GOURAUD = untextured vertex-colour with DEPTH OFF (like the canopy glass plate).
                    // The mesh rasterised fine under STATE_GOURAUD (drawn=8926, csZ<0) but was hidden by the
                    // cockpit depth -- the hand sits behind the near panels. Drawing depth-off makes it an overlay
                    // (visible like the beam). v1.1 will add backface culling so the solid reads correctly.
                    renderer->context.RestoreState(STATE_ALPHA_GOURAUD);
                    const VrTri* T = &s_vrModel[activeHnd].tris[0];
                    const int   nT = (int)s_vrModel[activeHnd].tris.size();
                    int nDrawn = 0; float s0x = 0, s0y = 0, s0z = 0;   // DIAG: how many tris drew + a sample projection
                    for (int ti = 0; ti < nT; ++ti)
                    {
                        const VrTri& tr = T[ti];
                        ThreeDVertex vv[3]; bool infront = true;
                        for (int k = 0; k < 3; ++k)
                        {
                            float mx = tr.p[k][0], my = tr.p[k][1], mz = tr.p[k][2];
                            Tpoint wp;
                            wp.x = g_vrGripPoint.x + (br[0]*mx + bu[0]*my + bf[0]*mz) * M2B;
                            wp.y = g_vrGripPoint.y + (br[1]*mx + bu[1]*my + bf[1]*mz) * M2B + ipdY;
                            wp.z = g_vrGripPoint.z + (br[2]*mx + bu[2]*my + bf[2]*mz) * M2B;
                            renderer->TransformCameraCentricPoint(&wp, &vv[k]);
                            if (vv[k].csZ >= -1.0f) infront = false;   // behind the eye -> skip tri
                        }
                        if (!infront) continue;
                        float nwx = br[0]*tr.n[0] + bu[0]*tr.n[1] + bf[0]*tr.n[2];
                        float nwy = br[1]*tr.n[0] + bu[1]*tr.n[1] + bf[1]*tr.n[2];
                        float nwz = br[2]*tr.n[0] + bu[2]*tr.n[1] + bf[2]*tr.n[2];
                        float d = nwx*Lx + nwy*Ly + nwz*Lz; if (d < 0) d = -d;   // two-sided lambert
                        float sh = 0.35f + 0.65f*d; if (sh > 1.0f) sh = 1.0f;
                        // Fill via Render2DTri -- the SAME proven 2D-immediate path as the beam's Render2DLine
                        // (DrawTriangle queues into the poly-list batch that never flushes in this context).
                        // Flat per-tri grey shade, depth-off overlay. Skip clipped/off-screen tris (the UInt16
                        // cast in Render2DTri would wrap a negative/huge coord into a stray triangle).
                        if (vv[0].clipFlag or vv[1].clipFlag or vv[2].clipFlag) continue;
                        if (nDrawn == 0) { s0x = vv[0].x; s0y = vv[0].y; s0z = vv[0].csZ; }
                        int sv = (int)(sh * 255.0f); if (sv < 40) sv = 40; if (sv > 255) sv = 255;
                        // OPAQUE grey: alpha byte MUST be 0xFF -- under STATE_ALPHA_GOURAUD (alpha blend) an
                        // alpha of 0 makes the fill fully transparent (why the mesh was invisible while the
                        // now-fixed sun billboard, with its own opaque colour, showed). 0xAABBGGRR.
                        renderer->SetColor(0xFF000000u | ((DWORD)sv << 16) | ((DWORD)sv << 8) | (DWORD)sv);
                        renderer->Render2DTri(vv[0].x, vv[0].y, vv[1].x, vv[1].y, vv[2].x, vv[2].y);   // float args; self-clips off-screen
                        ++nDrawn;
                    }
                    { static int s_md = 0; if ((s_md++ % 90) == 0) {
                        char db[320]; sprintf(db, "VRMODEL grip=%.0f,%.0f,%.0f F=%.2f,%.2f,%.2f nT=%d drawn=%d M2B=%.0f ipdY=%.0f sampleXY=%.0f,%.0f csZ=%.1f\n",
                            g_vrGripPoint.x, g_vrGripPoint.y, g_vrGripPoint.z, bf[0], bf[1], bf[2], nT, nDrawn, M2B, ipdY, s0x, s0y, s0z);
                        OutputDebugStringA(db); FILE* d = fopen("vrmodel_diag.txt", "a"); if (d) { fputs(db, d); fclose(d); } } }
                    renderer->SetColor(0x0000FF00);   // restore green for any later line draws
                    drewModel = true;
                }
            }

            (void)drewModel;
            if (!drewActiveHand and not g_bVrControllerModel and g_vrGripValid)   // wireframe fallback only when the mesh MODEL is OFF
            {
                // Orthonormal basis from the CALIBRATED aim direction -> the model points exactly where the ray
                // does, so no separate axis calibration is needed. up = body-up (-z) orthogonalised to forward;
                // right = forward x up. Roll about the aim axis is not tracked (purely cosmetic). This replaces
                // the flat square placeholder with a hand-oriented controller so you can see where you point.
                Tpoint f = g_vrRayDir;
                float fl2 = sqrtf(f.x * f.x + f.y * f.y + f.z * f.z);
                if (fl2 > 1e-4f) { f.x /= fl2; f.y /= fl2; f.z /= fl2; }
                Tpoint wu = { 0.0f, 0.0f, -1.0f };                          // body up = -z (z is down)
                Tpoint r = { f.y * wu.z - f.z * wu.y, f.z * wu.x - f.x * wu.z, f.x * wu.y - f.y * wu.x };
                float rl = sqrtf(r.x * r.x + r.y * r.y + r.z * r.z);
                if (rl < 1e-3f) { r.x = 0.0f; r.y = 1.0f; r.z = 0.0f; rl = 1.0f; }   // aim near vertical
                r.x /= rl; r.y /= rl; r.z /= rl;
                Tpoint u = { r.y * f.z - r.z * f.y, r.z * f.x - r.x * f.z, r.x * f.y - r.y * f.x };
                // Body box: barrel a bit ahead of the grip, handle extending back into the hand (button units).
                const float FR = 40.0f, BK = -180.0f, HW = 26.0f, HH = 20.0f;
                const float slong[2] = { FR, BK }, swide[2] = { HW, -HW }, shigh[2] = { HH, -HH };
                ThreeDVertex cv[8]; bool cok[8]; int ci = 0;
                for (int a = 0; a < 2; ++a) for (int b = 0; b < 2; ++b) for (int c = 0; c < 2; ++c)
                {
                    Tpoint p;
                    p.x = g_vrGripPoint.x + f.x * slong[a] + r.x * swide[b] + u.x * shigh[c];
                    p.y = g_vrGripPoint.y + f.y * slong[a] + r.y * swide[b] + u.y * shigh[c] + ipdY;
                    p.z = g_vrGripPoint.z + f.z * slong[a] + r.z * swide[b] + u.z * shigh[c];
                    renderer->TransformCameraCentricPoint(&p, &cv[ci]);
                    cok[ci] = (cv[ci].csZ < -30.0f);
                    ci++;
                }
                // corner index = a*4 + b*2 + c (a: front/back, b: +/-width, c: +/-height)
                static const int edges[12][2] = {
                    {0,1},{1,3},{3,2},{2,0},   // front face
                    {4,5},{5,7},{7,6},{6,4},   // back face
                    {0,4},{1,5},{2,6},{3,7}    // connectors
                };
                for (int e = 0; e < 12; ++e)
                {
                    int i0 = edges[e][0], i1 = edges[e][1];
                    if (cok[i0] and cok[i1])
                        renderer->Render2DLine(cv[i0].x, cv[i0].y, cv[i1].x, cv[i1].y);
                }
            }
        }
    }

    if ((vuxRealTime - gTimeLastMouseMove < SI_MOUSE_TIME_DELTA) and not InExitMenu()) //Wombat778 10-15-2003 added so mouse cursor would disappear after a few seconds standing still. Also dont want two cursors when exit menu is up
    {
        //Wombat778 10-15-2003 Added the following so that mouse cursor could be drawn in green if over a button, red otherwise
        // Artscout - 2026 (VR): run the hover (green) hit-test ONLY in view 0 (periphery). VCock_Exec runs
        // once per view (4x in quad), and the focus passes (2/3) project buttons with the zoomed gaze
        // camera but compare to the periphery mouse gxPos -> garbage, and the LAST pass (view 3) would
        // overwrite view 0's correct green with a miss. Gate to view 0, like the magnetic anchor below.
        if (g_b3DClickableCursorChange and xrView0 and not g_vrCtrlRayActive)   // Artscout - 2026: controller ray owns the pick when active
        {
            gSelectedCursor = 0;

            for (i = 0 ; i < Button3DList.numbuttons ; i++)
            {
                Tpoint Pos = Button3DList.buttons[i].loc;
                Pos.x += headPan.x * B3D_POSITION_SCALING;
                Pos.y += headPan.y * B3D_POSITION_SCALING;
                Pos.z += headPan.z * B3D_POSITION_SCALING;
                Pos.y += vrIpdButtonY;   // Artscout - 2026 (VR): left-eye IPD parallax (depth-correct detect)

                renderer->TransformCameraCentricPoint(&Pos, &t1);
                if (t1.csZ >= 0.0f) continue;   // Artscout - 2026 (VR): skip buttons behind the camera

                // Artscout - 2026 (VR): t1 is ALREADY in DISPLAY pixel space (renderer xRes/yRes == DispWidth
                // /DispHeight during VCock_Exec), the same space as gxPos/gyPos and the drawn cursor -- so
                // compare directly, no eye->display rescale (a former *dw/ew double-scaled the buttons).
                float hoverTd = (float)(DisplayOptions.DispWidth / 1600.0f) * (Button3DList.buttons[i].dist / (1.5f * (float)GetFOV()));
                if (xrPick)
                {
                    t1.x += vrBiasX; t1.y += vrBiasY;   // VR only: zero the small IPD/off-axis residual (mode-specific)
                    hoverTd *= vrMagnet;   // headset can't aim to the tight stock radius
                }

                if (sqrt(((gxPos - t1.x) * (gxPos - t1.x)) + ((gyPos - t1.y) * (gyPos - t1.y)))  < hoverTd) //Wombat778 10-15-2003 changes changex with gxPos
                {
                    gSelectedCursor = 9;
                    break;
                }
            }
        }

        //Wombat778 12-16-2003 moved to vcock.cpp
        //ClipAndDrawCursor(OTWDriver.pCockpitManager->GetCockpitWidth(), OTWDriver.pCockpitManager->GetCockpitHeight());//Wombat778 10-10-2003  Draw the Mouse cursor if 3d clickable cockpit enabled

        // Artscout - 2026 (VR): capture the nearest 3D button as the magnetic cursor anchor (left-eye
        // pass only -- view 0). Uses the SAME criterion as the click loop below (nearest within the per-
        // button hit radius, in DispWidth space), so the cursor snaps onto exactly the button that would
        // be clicked. otwloop projects g_vrCursorAnchor into each eye for a stereo-correct 3D cursor.
        if (xrPick and g_pOpenXRBackend->CurrentEye() <= 0 and not g_vrCtrlRayActive)   // Artscout - 2026: skip when the controller ray drives the anchor
        {
            g_vrCursorAnchorValid = false;
            g_vrCursorAnchorButton = -1;
            bool  snapped = false;
            float bestD = 1.0e30f;
            float nearDist = 1.0e30f, freeDepth = 0.0f;   // depth of the button nearest the mouse (for the free cursor)
            for (i = 0 ; i < Button3DList.numbuttons ; i++)
            {
                Tpoint Pos = Button3DList.buttons[i].loc;
                Pos.x += headPan.x * B3D_POSITION_SCALING;
                Pos.y += headPan.y * B3D_POSITION_SCALING;
                Pos.z += headPan.z * B3D_POSITION_SCALING;
                Pos.y += vrIpdButtonY;   // Artscout - 2026 (VR): left-eye IPD parallax (depth-correct detect)

                ThreeDVertex tp;
                renderer->TransformCameraCentricPoint(&Pos, &tp);
                if (tp.csZ >= 0.0f) continue;   // Artscout - 2026 (VR): skip buttons behind the camera (1/z flip -> false far snap)
                // Artscout - 2026 (VR): tp is already in DISPLAY pixel space (xRes/yRes == DispWidth during
                // VCock_Exec), the same space as gxPos/gyPos -- compare directly, no eye->display rescale.
                // g_fVrDetectBias* zeroes the small residual (IPD parallax / off-axis fold mismatch).
                float ex = tp.x + vrBiasX;
                float ey = tp.y + vrBiasY;
                float d  = (gxPos - ex) * (gxPos - ex) + (gyPos - ey) * (gyPos - ey);
                float td = (float)(DisplayOptions.DispWidth / 1600.0f) * (Button3DList.buttons[i].dist / (1.5f * (float)GetFOV())) * vrMagnet;
                if (d < nearDist) { nearDist = d; freeDepth = sqrtf(Pos.x * Pos.x + Pos.y * Pos.y + Pos.z * Pos.z); }
                if (d < td * td and d < bestD)
                {
                    bestD = d;
                    g_vrCursorAnchor = Pos;
                    g_vrCursorAnchorValid = true;
                    g_vrCursorAnchorButton = i;   // remember WHICH button -> click it directly
                    snapped = true;
                }
            }
            // Artscout - 2026 (VR mouse): FREE cursor. If the mouse is not over a clickable button, place
            // the cursor on the mouse ray at the nearest panel's depth so a 3D cursor is ALWAYS drawn --
            // including inside the focus/gaze view (where a flat DispWidth cursor lands wrong and the
            // periphery cursor is hidden under the focus overlay). UnTransformPoint gives the world-space
            // ray for the mouse pixel; scaling it by the panel depth makes the cursor track the mouse and
            // project correctly in every view (same pose, per-view FOV/off-axis). This is the closed loop
            // the user needs to aim: see the cursor move -> bring it onto a switch -> it snaps green.
            if (not snapped)
            {
                Tpoint pix, ray;
                pix.x = (float)gxPos;   // DISPLAY pixel space (renderer xRes == DispWidth here), no eye rescale
                pix.y = (float)gyPos;
                pix.z = 0.0f;
                renderer->UnTransformPoint(&pix, &ray);   // normalized world-space ray from the camera
                if (freeDepth <= 1.0f) freeDepth = 100.0f;
                g_vrCursorAnchor.x = ray.x * freeDepth;
                g_vrCursorAnchor.y = ray.y * freeDepth;
                g_vrCursorAnchor.z = ray.z * freeDepth;
                g_vrCursorAnchorValid = true;   // always draw a 3D cursor in VR (free or snapped)
            }
            g_vrCursorAnchorSnapped = snapped;
        }

    }


    // Artscout - 2026 (VR): process the click ONLY in view 0 (periphery), like the hover/anchor. VCock_Exec
    // runs once per view; if a focus pass (2/3, zoomed gaze camera) consumed the click, buttons projected
    // to wild vertical coords (ey far off-screen) -> no match / wrong button. Gating to view 0 makes the
    // click use the same periphery camera as the visible cursor and the hover/snap test.
    if (Button3DList.clicked and xrView0) //Wombat778 10-11-2003 check if the mouse button has been clicked while in the 3d cockpit (xrView0: FLAT always, VR periphery only)
    {
        float closestdistance = 9999; //set these variables to a high value so that we know when it is uninitialized (there is a button 0)
        float tempdistance = 9999;
        int closestbutton = 9999;

        for (i = 0 ; i < Button3DList.numbuttons ; i++)
        {

            if (Button3DList.buttons[i].mousebutton == Button3DList.clicked) //Wombat778 11-07-2003 Added so that the left and right mouse button can be differentiated
            {
                Tpoint Pos = Button3DList.buttons[i].loc;
                Pos.x += headPan.x * B3D_POSITION_SCALING;
                Pos.y += headPan.y * B3D_POSITION_SCALING;
                Pos.z += headPan.z * B3D_POSITION_SCALING;
                Pos.y += vrIpdButtonY;   // Artscout - 2026 (VR): left-eye IPD parallax (depth-correct detect)

                renderer->TransformCameraCentricPoint(&Pos, &t1);

                if (t1.csZ >= 0.0f) continue;   // Artscout - 2026 (VR): skip buttons BEHIND the camera --
                                                // their 1/z flips the projection to wild coords and false-
                                                // matches near gxPos (the "snap flies off far" bug).

                // Artscout - 2026 (VR): t1 is already in DISPLAY pixel space (xRes/yRes == DispWidth during
                // VCock_Exec), the same space as gxPos/gyPos -- compare directly, no eye->display rescale.
                // g_fVrDetectBias* zeroes the small residual (IPD parallax / off-axis fold mismatch).
                bool vrHit = xrPick;
                if (vrHit) { t1.x += vrBiasX; t1.y += vrBiasY; }

                tempdistance = sqrt(((gxPos - t1.x) * (gxPos - t1.x)) + ((gyPos - t1.y) * (gyPos - t1.y)));

                //Normalize the distance so it is affected by the FOV and by the resolution
                //Todo: add something about the SA bar.  Currently, the dist increases too much when it is active
                float td = ((float) DisplayOptions.DispWidth / 1600.0f) * (Button3DList.buttons[i].dist / (1.5f * (float)GetFOV()));
                if (vrHit) td *= vrMagnet;   // Artscout - 2026 (VR): match the enlarged hover/anchor snap radius (mode-specific)

                if (tempdistance < td)
                    if (tempdistance < closestdistance) //if the cursor is near more than 1 button, find the closest one
                    {
                        closestdistance = tempdistance;
                        closestbutton = i;
                    }
            }
        }

        // Artscout - 2026 (VR mouse): prefer the magnetically-snapped (GREEN) button. That is the button
        // the cursor is visibly sitting on; re-projecting here lands elsewhere because the click fires a
        // frame or two after the last hover (gaze/camera moved). Fire what the user sees.
        if (g_pOpenXRBackend and g_vrCursorAnchorSnapped and g_vrCursorAnchorButton >= 0
            and g_vrCursorAnchorButton < Button3DList.numbuttons
            and Button3DList.buttons[g_vrCursorAnchorButton].mousebutton == Button3DList.clicked)
            closestbutton = g_vrCursorAnchorButton;

        // Artscout - 2026: if the hit button belongs to a multi-position rotary group, don't fire it directly --
        // advance the group's current position (LMB / thumb-up = next, RMB / thumb-down = prev, skipping any
        // unresolved slots) and fire THAT position's setter. Keeps the tracked index in sync with the sim (every
        // change goes through here), so the switch steps one detent per click from a single hotspot.
        int firebutton = closestbutton;
        if (closestbutton not_eq 9999)
        {
            int grp = Button3DList.buttons[closestbutton].groupId;
            if (grp >= 0 and grp < s_numSwitchGroups)
            {
                int dir = (Button3DList.clicked == 2) ? -1 : 1;
                int cnt = s_switchGroupDefs[grp].count;
                int nxt = g_vrSwitchGroupCur[grp];
                for (int step = 0; step < cnt; step++) { nxt = (nxt + dir + cnt) % cnt; if (s_switchGroupMember[grp][nxt] >= 0) break; }
                g_vrSwitchGroupCur[grp] = nxt;
                if (s_switchGroupMember[grp][nxt] >= 0) firebutton = s_switchGroupMember[grp][nxt];
            }
        }

        if (firebutton not_eq 9999)
            if (Button3DList.buttons[firebutton].function)
            {
                //Wombat778 03-06-04 Send the buttonid of the function, which should stop a ctd in not-realistic avionics
                if (Button3DList.buttons[firebutton].buttonId < 0)
                    //Wombat778 03-06-04 Use callfunc instead of directly calling funcs, so they can be captured
                    CallFunc(Button3DList.buttons[firebutton].function, 1, KEY_DOWN, NULL);
                else
                    //Wombat778 03-06-04 Use callfunc instead of directly calling funcs, so they can be captured
                    CallFunc(Button3DList.buttons[firebutton].function, 1, KEY_DOWN, OTWDriver.pCockpitManager->GetButtonPointer(Button3DList.buttons[firebutton].buttonId));

                F4SoundFXSetDist(Button3DList.buttons[firebutton].sound, FALSE, 0.0f, 1.0f);
            }

        Button3DList.clicked = 0;
    }

    if (g_b3DClickableCockpitDebug)
    {
        //Wombat778 10-10-2003 Draw Locations of the 3d buttons when Debug mode is enabled


        for (i = 0 ; i < Button3DList.numbuttons ; i++)
        {

            // if (i==Button3DList.debugbutton)
            // renderer->SetColor(pVColors[TheTimeOfDay.GetNVGmode() not_eq 0][7]);
            // else
            // renderer->SetColor (0x000000FF); //RED
            Tpoint Pos = Button3DList.buttons[i].loc;
            Pos.x += headPan.x * B3D_POSITION_SCALING;
            Pos.y += headPan.y * B3D_POSITION_SCALING;
            Pos.z += headPan.z * B3D_POSITION_SCALING;

            renderer->TransformCameraCentricPoint(&Pos, &t1);

            if (t1.csZ < 0)   //Wombat778 10-11-2003 Only show those points in front of us. Why it does this is beyond me.
            {

                renderer->SetColor(0x000000FF); //RED



                renderer->Render2DPoint(t1.x, t1.y);
                renderer->Render2DPoint(t1.x, t1.y - 1);
                renderer->Render2DPoint(t1.x, t1.y + 1);
                renderer->Render2DPoint(t1.x - 1, t1.y);
                renderer->Render2DPoint(t1.x - 1, t1.y - 1);
                renderer->Render2DPoint(t1.x - 1, t1.y + 1);
                renderer->Render2DPoint(t1.x + 1, t1.y);
                renderer->Render2DPoint(t1.x + 1, t1.y - 1);
                renderer->Render2DPoint(t1.x + 1, t1.y + 1);


                renderer->SetColor(0x0000ffff); //Yellow

                //Normalize the distance so it is affected by the FOV and by the resolution
                //Todo: add something about the SA bar.  Currently, the dist increases too much when it is active
                float td = ((float) DisplayOptions.DispWidth / 1600.0f) * (Button3DList.buttons[i].dist / (1.5f * (float)GetFOV()));

                renderer->Render2DPoint(t1.x - td, t1.y);
                renderer->Render2DPoint(t1.x + td, t1.y);
                renderer->Render2DPoint(t1.x, t1.y + td);
                renderer->Render2DPoint(t1.x, t1.y - td);

            }

            //Button3DList.buttons[i].dist);


        }

        renderer->SetColor(pVColors[TheTimeOfDay.GetNVGmode() not_eq 0][5]);
    }

#if 0
    /*
    ** Do ALT indictator
    */
    float alt = -((AircraftClass *)otwPlatform)->af->z;
    alt = fmod(alt, 1000.0f);
    alt = alt * 0.001f * 2.0 * PI;

    x1 = 0.0f;
    y1 = 0.0f;
    mlSinCos(&trig, alt);
    x2 = 0.85f * trig.cos;
    y2 = 0.85f * -trig.sin;
    vcInfo.vALTrenderer->Line(x1, y1, x2, y2);

    /*
    ** Do OIL indictator
    */
    float oil = ((AircraftClass *)otwPlatform)->af->rpm;
    oil = oil  * 2.0 * PI;

    x1 = 0.0f;
    y1 = 0.0f;
    mlSinCos(&trig, oil);
    x2 = 0.85f * trig.cos;
    y2 = 0.85f * -trig.sin;
    vcInfo.vOILrenderer->Line(x1, y1, x2, y2);

    /*
    ** Do NOZ indictator
    */
    float noz = ((AircraftClass *)otwPlatform)->af->rpm;
    noz = noz  * 2.0 * PI;

    x1 = 0.0f;
    y1 = 0.0f;
    mlSinCos(&trig, noz);
    x2 = 0.85f * trig.cos;
    y2 = 0.85f * -trig.sin;
    vcInfo.vNOZrenderer->Line(x1, y1, x2, y2);

    /*
    ** Do RPM indictator
    */
    float rpm = ((AircraftClass *)otwPlatform)->af->rpm;
    rpm = rpm  * 2.0 * PI;

    x1 = 0.0f;
    y1 = 0.0f;
    mlSinCos(&trig, rpm);
    x2 = 0.85f * trig.cos;
    y2 = 0.85f * -trig.sin;
    vcInfo.vRPMrenderer->Line(x1, y1, x2, y2);

    /*
    ** Do FTIT indictator
    */
    x1 = 0.0f;
    y1 = 0.0f;
    x2 = 0.0f;
    y2 = 0.85f;
    vcInfo.vFTITrenderer->Line(x1, y1, x2, y2);
#endif

    // 2001-01-31 ADDED BY S.G. SO HMS EQUIPPED PLANE HAS TWO GREEN CONCENTRIC CIRCLE IN PADLOCK VIEW
    VehicleClassDataType *vc = (VehicleClassDataType *)Falcon4ClassTable[otwPlatform->Type() - VU_LAST_ENTITY_TYPE].dataPtr;

    if (vc and vc->Flags bitand 0x20000000)
    {
        MissileClass* theMissile;
        theMissile = (MissileClass*)(SimDriver.GetPlayerAircraft()->Sms->GetCurrentWeapon());

        // First, make sure we have a Aim9 in uncage mode selected...
        if (SimDriver.GetPlayerAircraft()->Sms->curWeaponType == wtAim9)
        {
            if (theMissile and theMissile->isCaged == 0)
            {
                theMissile->RunSeeker();

                if ( not theMissile->targetPtr or vuxRealTime bitand 0x100)  // JB 010712 Flash when we have a target locked up
                {
                    float xDiff, left, right, top, bottom;

                    renderer->GetViewport(&left, &top, &right, &bottom);
                    renderer->SetColor(TheHud->GetHudColor());

                    xDiff = right - left;
                    renderer->CenterOriginInViewport();
                    //renderer->Circle(0.0f, 0.0f, xDiff / 30.0F);
                    renderer->Circle(0.0f, 0.0f, xDiff / 20.0F);
                    renderer->Line(-xDiff / 50.0f, 0.0f, xDiff / 50.0f, 0.0f);
                    renderer->Line(0.0f, -xDiff / 50.0f, 0.0f, xDiff / 50.0f);
                }
            }
        }
    }

    // END OF ADDED SECTION

    renderer->SetColor(TheHud->GetHudColor());
    renderer->SetCamera(&cameraPos, &cameraRot);
#endif
}

// sfr: end of 3d pit

/*
** CleanupVirtualCockpit
*/
void
OTWDriverClass::VCock_Cleanup(void)
{
    // int i;

    for (unsigned int i = 0; i < mpVDials.size(); i++)
    {
        delete mpVDials[i];
    }

    mpVDials.clear();

    if (vcInfo.vHUDrenderer)
    {
        vcInfo.vHUDrenderer->Cleanup();
        delete vcInfo.vHUDrenderer;
        vcInfo.vHUDrenderer = NULL;
    }

    if (vcInfo.vRWRrenderer)
    {
        vcInfo.vRWRrenderer->Cleanup();
        delete vcInfo.vRWRrenderer;
        vcInfo.vRWRrenderer = NULL;
    }

    if (vcInfo.vMACHrenderer)
    {
        vcInfo.vMACHrenderer->Cleanup();
        delete vcInfo.vMACHrenderer;
        vcInfo.vMACHrenderer = NULL;
    }

    if (vcInfo.vDEDrenderer)
    {
        vcInfo.vDEDrenderer->Cleanup();
        delete vcInfo.vDEDrenderer;
        vcInfo.vDEDrenderer = NULL;
    }

    if (vcInfo.vPFLrenderer)
    {
        vcInfo.vPFLrenderer->Cleanup();
        delete vcInfo.vPFLrenderer;
        vcInfo.vPFLrenderer = NULL;
    }

    // ASSO
    if (vHUDrenderer)
    {
        vHUDrenderer->Cleanup();
        delete vHUDrenderer;
        vHUDrenderer = NULL;
    }

    if (vRWRrenderer)
    {
        vRWRrenderer->Cleanup();
        delete vRWRrenderer;
        vRWRrenderer = NULL;
    }

    if (vDEDrenderer)
    {
        vDEDrenderer->Cleanup();
        delete vDEDrenderer;
        vDEDrenderer = NULL;
    }

    if (vPFLrenderer)
    {
        vPFLrenderer->Cleanup();
        delete vPFLrenderer;
        vPFLrenderer = NULL;
    }

    VirtualDisplay::CleanupRttTarget();
}

// Resolve the group table against the freshly-loaded button list: tag members, collapse their loc to the group
// centroid, and give the hotspot both a left (next) and right (prev) member so either mouse button / thumb dir
// enters the cycler. Called once after Button3D_Init parses the file.
static void BuildSwitchGroups(Button3DListType* L)
{
    for (int g = 0; g < s_numSwitchGroups; g++)
    {
        g_vrSwitchGroupCur[g] = 0;
        const VrSwitchGroupDef& D = s_switchGroupDefs[g];
        int   found = 0;
        float cx = 0.0f, cy = 0.0f, cz = 0.0f;
        for (int p = 0; p < D.count; p++)
        {
            s_switchGroupMember[g][p] = -1;
            InputFunctionType f = FindFunctionFromString((char*)D.funcs[p]);
            if (not f) continue;
            for (int b = 0; b < L->numbuttons; b++)
            {
                if (L->buttons[b].function == f and L->buttons[b].groupId < 0)
                {
                    s_switchGroupMember[g][p] = b;
                    L->buttons[b].groupId  = g;
                    L->buttons[b].groupPos = p;
                    cx += L->buttons[b].loc.x; cy += L->buttons[b].loc.y; cz += L->buttons[b].loc.z;
                    found++;
                    break;
                }
            }
        }
        if (found >= 2)
        {
            cx /= found; cy /= found; cz /= found;
            for (int p = 0; p < D.count; p++)
            {
                int b = s_switchGroupMember[g][p];
                if (b < 0) continue;
                L->buttons[b].loc.x = cx; L->buttons[b].loc.y = cy; L->buttons[b].loc.z = cz;   // one shared hotspot
                L->buttons[b].mousebutton = (p == 1) ? 2 : 1;   // >=1 left + >=1 right member so both dirs pick here
            }
        }
        else
        {
            for (int p = 0; p < D.count; p++) { int b = s_switchGroupMember[g][p]; if (b >= 0) L->buttons[b].groupId = -1; }
        }
    }
}

//Wombat778 10-10-2003 Load 3d buttons

bool
OTWDriverClass::Button3D_Init(int eCPVisType, TCHAR* eCPName, TCHAR* eCPNameNCTR)
{
    char strCPFile[MAX_PATH];
    static const TCHAR *buttonfile = "3dbuttons.dat";
    static const TCHAR *vcockfile = "3dckpit.dat"; //Wombat778 10-15-2003
    FILE* Button3DDataFile;
    char templine[256];
    char tempfunction[256];

    //    FindCockpit(pCPFile, (Vis_Types)eCPVisType, eCPName, eCPNameNCTR, strCPFile);

    //Wombat778 10-15-2003 Replaced the findcockpit call with a sequence that should mean that a button file only loads if it is in
    //the same folder as the 3d cockpit file.  This should solve the problem of an f-16 button file with another planes 3d pit.

    if (eCPVisType == MapVisId(VIS_F16C))
        // RV - Biker
        //sprintf(strCPFile, "%s%s", FalconCockpitThrDirectory, buttonfile);
        sprintf(strCPFile, "%s\\%s", FalconCockpitThrDirectory, buttonfile);
    else
    {
        // RV - Biker
        //sprintf(strCPFile, "%s%d\\%s", FalconCockpitThrDirectory, MapVisId(eCPVisType), vcockfile);
        sprintf(strCPFile, "%s\\%d\\%s", FalconCockpitThrDirectory, MapVisId(eCPVisType), vcockfile);

        // RV - Biker - No more res manager
        //if(ResExistFile(strCPFile))
        if (FileExists(strCPFile))
            // RV - Biker
            //sprintf(strCPFile, "%s%d\\%s", FalconCockpitThrDirectory, MapVisId(eCPVisType), buttonfile);
            sprintf(strCPFile, "%s\\%d\\%s", FalconCockpitThrDirectory, MapVisId(eCPVisType), buttonfile);
        else
        {
            std::string name = RemoveInvalidChars(string(eCPName, 15));

            // RV - Biker
            //sprintf(strCPFile, "%s%s\\%s", FalconCockpitThrDirectory, name.c_str(), vcockfile);
            sprintf(strCPFile, "%s\\%s\\%s", FalconCockpitThrDirectory, name.c_str(), vcockfile);

            // RV - Biker - No more res manager
            //if(ResExistFile(strCPFile))
            if (FileExists(strCPFile))
                // RV - Biker
                //sprintf(strCPFile, "%s%s\\%s", FalconCockpitThrDirectory, name.c_str(), buttonfile);
                sprintf(strCPFile, "%s\\%s\\%s", FalconCockpitThrDirectory, name.c_str(), buttonfile);
            else
            {
                std::string nameNCTR = RemoveInvalidChars(string(eCPNameNCTR, 5));
                // RV - Biker
                //sprintf(strCPFile, "%s%s\\%s", FalconCockpitThrDirectory, nameNCTR.c_str(), vcockfile);
                sprintf(strCPFile, "%s\\%s\\%s", FalconCockpitThrDirectory, nameNCTR.c_str(), vcockfile);

                // RV - Biker - No more res manager
                //if(ResExistFile(strCPFile))
                if (FileExists(strCPFile))
                    // RV - Biker
                    //sprintf(strCPFile, "%s%s\\%s", FalconCockpitThrDirectory, nameNCTR.c_str(), buttonfile);
                    sprintf(strCPFile, "%s\\%s\\%s", FalconCockpitThrDirectory, nameNCTR.c_str(), buttonfile);
                else
                {
                    // F16C fallback
                    // RV - Biker - Here read from default cockpit dir
                    //sprintf(strCPFile, "%s%s", FalconCockpitThrDirectory, buttonfile);
                    sprintf(strCPFile, "%s\\%s", COCKPIT_DIR, buttonfile);
                }
            }
        }
    }

    Button3DDataFile = fopen(strCPFile, "r");


    Button3DList.numbuttons = 0;
    Button3DList.debugbutton = 0;
    Button3DList.clicked = 0; //Wombat778 10-15-2003 removed clickx and clicky

    if (Button3DDataFile)
    {
        if ( not feof(Button3DDataFile))
            fgets(templine, 256, Button3DDataFile); //Just read a dummy line for comments etc..

        while ( not feof(Button3DDataFile))
        {
            fgets(templine, 256, Button3DDataFile);
            int matchedfields = sscanf(templine, "%s %f %f %f %f %d %d", tempfunction, //Wombat778 11-08-2003
                                       &Button3DList.buttons[Button3DList.numbuttons].loc.x,
                                       &Button3DList.buttons[Button3DList.numbuttons].loc.y,
                                       &Button3DList.buttons[Button3DList.numbuttons].loc.z,
                                       &Button3DList.buttons[Button3DList.numbuttons].dist,
                                       &Button3DList.buttons[Button3DList.numbuttons].sound,
                                       &Button3DList.buttons[Button3DList.numbuttons].mousebutton);


            if (matchedfields == 6)
                Button3DList.buttons[Button3DList.numbuttons].mousebutton = 1; //Wombat778 11-08-2003 Added so there will still be compatibility with old files. Default to left mouse button.

            if (matchedfields >= 6) //Wombat778 11-08-2003 changed to allow compatibility with old files 11-7-2003 added mousebutton field to allow LMB/RMB usage.
            {
                InputFunctionType tempfunc;
                tempfunc = FindFunctionFromString(tempfunction);
                //Wombat778 03-06-04 Find and store the buttonid of the function, which should stop a ctd in not-realistic avionics.
                Button3DList.buttons[Button3DList.numbuttons].function = tempfunc;
                Button3DList.buttons[Button3DList.numbuttons].groupId  = -1;   // Artscout - 2026: assigned by BuildSwitchGroups
                Button3DList.buttons[Button3DList.numbuttons].groupPos = 0;

                if (tempfunc)
                    Button3DList.buttons[Button3DList.numbuttons].buttonId = UserFunctionTable.GetButtonId(tempfunc); //GetButtonId is a terribly slow function because it has to traverse a hash table.

                Button3DList.numbuttons++;
            }
        }

        fclose(Button3DDataFile);

        // Artscout - 2026: collapse multi-position rotaries to one cyclable hotspot (unless disabled via cfg).
        extern bool g_bVrSwitchGroups;
        if (g_bVrSwitchGroups) BuildSwitchGroups(&Button3DList);

        return true;
    }

    return false;
}



