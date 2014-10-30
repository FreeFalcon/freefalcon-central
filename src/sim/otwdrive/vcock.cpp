#include "Graphics/Include/canvas3d.h"
#include "Graphics/Include/drawbsp.h"
#include "Graphics/Include/renderow.h"
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

#define PAN_LIMIT 150.0F
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
        if (FindBestResolution() < 1280)
        {
            resScale = 512.0f / (float)txRes;
            txRes = 512;
            tyRes = 512;
        }

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

    // Cobra - Lower screen resolutions need a smaller canvas (font is too small)
    if (FindBestResolution() < 1280)
    {
        if (tLeft > 1)
            tLeft = (int)(resScale * (float)tLeft);

        if (tRight > 1)
            tRight = (int)(resScale * (float)tRight);

        if (tTop > 1)
            tTop = (int)(resScale * (float)tTop);

        if (tBottom > 1)
            tBottom = (int)(resScale * (float)tBottom);
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
                if (FindBestResolution() < 1280)
                {
                    resScale = 512.0f / (float)txRes;
                    txRes = 512;
                    tyRes = 512;
                }

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

            // Cobra - Lower screen resolutions need a smaller canvas (font is too small)
            if (FindBestResolution() < 1280)
            {
                if (tLeft > 1)
                    tLeft = (int)(resScale * (float)tLeft);

                if (tRight > 1)
                    tRight = (int)(resScale * (float)tRight);

                if (tTop > 1)
                    tTop = (int)(resScale * (float)tTop);

                if (tBottom > 1)
                    tBottom = (int)(resScale * (float)tBottom);
            }

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

            // Cobra - Lower screen resolutions need a smaller canvas (font is too small)
            if (FindBestResolution() < 1280)
            {
                if (tLeft > 1)
                    tLeft = (int)(resScale * (float)tLeft);

                if (tRight > 1)
                    tRight = (int)(resScale * (float)tRight);

                if (tTop > 1)
                    tTop = (int)(resScale * (float)tTop);

                if (tBottom > 1)
                    tBottom = (int)(resScale * (float)tBottom);
            }

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
            float actualtilt;
            float actualrollrate;
            float actualpan;
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
            float XOffset = 12.0f * headPan.y / (pt[1].y - pt[0].y) * tanf(DTR * 60.0f);
            float YOffset = 12.0f * headPan.z / (pt[0].z - pt[2].z) * tanf(DTR * 60.0f);

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

        if (vHUDrenderer)
            vHUDrenderer->DrawRttQuad();

        if (vRWRrenderer)
            vRWRrenderer->DrawRttQuad();

        if (vDEDrenderer)
            vDEDrenderer->DrawRttQuad();

        if (vPFLrenderer)
            vPFLrenderer->DrawRttQuad();

        if (g_b3dMFDLeft)
        {
            if (MfdDisplay[0]->GetDrawable() and MfdDisplay[0]->GetDrawable()->GetDisplay())
            {
                MfdDisplay[0]->GetDrawable()->GetDisplay()->DrawRttQuad();
            }
        }

        if (g_b3dMFDRight)
        {
            if (MfdDisplay[1]->GetDrawable() and MfdDisplay[1]->GetDrawable()->GetDisplay())
            {
                MfdDisplay[1]->GetDrawable()->GetDisplay()->DrawRttQuad();
            }
        }
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
    gSelectedCursor = 9; //Wombat778 10-11-2003 set the cursor to the default green cursor

    if ((vuxRealTime - gTimeLastMouseMove < SI_MOUSE_TIME_DELTA) and not InExitMenu()) //Wombat778 10-15-2003 added so mouse cursor would disappear after a few seconds standing still. Also dont want two cursors when exit menu is up
    {
        //Wombat778 10-15-2003 Added the following so that mouse cursor could be drawn in green if over a button, red otherwise
        if (g_b3DClickableCursorChange)
        {
            gSelectedCursor = 0;

            for (i = 0 ; i < Button3DList.numbuttons ; i++)
            {
                Tpoint Pos = Button3DList.buttons[i].loc;
                Pos.x += headPan.x * B3D_POSITION_SCALING;
                Pos.y += headPan.y * B3D_POSITION_SCALING;
                Pos.z += headPan.z * B3D_POSITION_SCALING;

                renderer->TransformCameraCentricPoint(&Pos, &t1);

                if (sqrt(((gxPos - t1.x) * (gxPos - t1.x)) + ((gyPos - t1.y) * (gyPos - t1.y)))  < (float)(DisplayOptions.DispWidth / 1600.0f) * (Button3DList.buttons[i].dist / (1.5f * (float)GetFOV()))) //Wombat778 10-15-2003 changes changex with gxPos
                {
                    gSelectedCursor = 9;
                    break;
                }
            }
        }

        //Wombat778 12-16-2003 moved to vcock.cpp
        //ClipAndDrawCursor(OTWDriver.pCockpitManager->GetCockpitWidth(), OTWDriver.pCockpitManager->GetCockpitHeight());//Wombat778 10-10-2003  Draw the Mouse cursor if 3d clickable cockpit enabled

    }


    if (Button3DList.clicked) //Wombat778 10-11-2003 check if the mouse button has been clicked while in the 3d cockpit
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

                renderer->TransformCameraCentricPoint(&Pos, &t1);
                tempdistance = sqrt(((gxPos - t1.x) * (gxPos - t1.x)) + ((gyPos - t1.y) * (gyPos - t1.y)));

                //Normalize the distance so it is affected by the FOV and by the resolution
                //Todo: add something about the SA bar.  Currently, the dist increases too much when it is active
                float td = ((float) DisplayOptions.DispWidth / 1600.0f) * (Button3DList.buttons[i].dist / (1.5f * (float)GetFOV()));

                if (tempdistance < td)
                    if (tempdistance < closestdistance) //if the cursor is near more than 1 button, find the closest one
                    {
                        closestdistance = tempdistance;
                        closestbutton = i;
                        tempdistance = 9999;

                    }
            }
        }

        if (closestbutton not_eq 9999)
            if (Button3DList.buttons[closestbutton].function)
            {
                //Wombat778 03-06-04 Send the buttonid of the function, which should stop a ctd in not-realistic avionics
                if (Button3DList.buttons[closestbutton].buttonId < 0)
                    //Wombat778 03-06-04 Use callfunc instead of directly calling funcs, so they can be captured
                    CallFunc(Button3DList.buttons[closestbutton].function, 1, KEY_DOWN, NULL);
                else
                    //Wombat778 03-06-04 Use callfunc instead of directly calling funcs, so they can be captured
                    CallFunc(Button3DList.buttons[closestbutton].function, 1, KEY_DOWN, OTWDriver.pCockpitManager->GetButtonPointer(Button3DList.buttons[closestbutton].buttonId));

                F4SoundFXSetDist(Button3DList.buttons[closestbutton].sound, FALSE, 0.0f, 1.0f);
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

                if (tempfunc)
                    Button3DList.buttons[Button3DList.numbuttons].buttonId = UserFunctionTable.GetButtonId(tempfunc); //GetButtonId is a terribly slow function because it has to traverse a hash table.

                Button3DList.numbuttons++;
            }
        }

        fclose(Button3DDataFile);
        return true;
    }

    return false;
}



