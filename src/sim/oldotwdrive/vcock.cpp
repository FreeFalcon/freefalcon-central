#include "Graphics\Include\canvas3d.h"
#include "Graphics\Include\drawbsp.h"
#include "Graphics\Include\renderow.h"
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
#include "Graphics\Include\tod.h"
#include "flightData.h"
#include "vdial.h"
#include "fack.h"
#include "dofsnswitches.h"

#include "sinput.h"		//Wombat778 10-10-2003  Added for 3d clickable cockpit
#include "commands.h"		//Wombat778 10-10-2003  Added for 3d clickable cockpit

/* S.G. FOR HMS CODE */ #include "missile.h"

//MI for ICP stuff
extern bool g_bRealisticAvionics ;
#include "navsystem.h"

//Wombat778 3D Cockpit variables
//extern bool g_b3DClickableCockpit;
extern bool g_b3DClickableCockpitDebug;
extern int g_n3DHeadPanRange; //Wombat778 2-21-2004 
extern int g_n3DHeadTiltRange; //Wombat778 2-21-2004


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

#define PAN_LIMIT 130.0F
extern void* gSharedMemPtr;

void OTWDriverClass::VCock_CheckStopStates(float dT)
{
	if(stopState == STOP_STATE0) {
		if((azDir > 0.0F && eyePan <= PAN_LIMIT * DTR) || (azDir < 0.0F && eyePan >= PAN_LIMIT * DTR)){

			stopState = STOP_STATE1;
			eyePan	= min(max(eyePan, -PAN_LIMIT * DTR), PAN_LIMIT * DTR);
			F4SoundFXSetDist(SFX_CP_UGH, TRUE, 0.0f, 1.0f);
		}
		else {
			VCock_RunNormalMotion(dT);
		}
	}
	else if(stopState == STOP_STATE1) {
		if((azDir > 0.0F && eyePan <= -PAN_LIMIT * DTR) || (azDir < 0.0F && eyePan >= PAN_LIMIT * DTR)){
			stopState = STOP_STATE1;
		}
		else if(azDir == 0.0F) {
			stopState = STOP_STATE2;
		}
		else {
			stopState = STOP_STATE0;
		}
	}
	else if(stopState == STOP_STATE2) {
		if((azDir > 0.0F && eyePan <= -PAN_LIMIT * DTR) || (azDir < 0.0F && eyePan >= PAN_LIMIT * DTR)){
			headMotion	= HEAD_TRANSISTION1;
			initialTilt	= eyeTilt;
			eyePan		= min(max(eyePan, -PAN_LIMIT * DTR), PAN_LIMIT * DTR);
			if(eyePan <= -PAN_LIMIT * DTR) {
				snapDir	= LTOR;
			}
			else {
				snapDir = RTOL;
			}
		}
		else if(azDir == 0.0F) {
			VCock_RunNormalMotion(dT);
			stopState = STOP_STATE2;
		}
		else {
			stopState = STOP_STATE0;
			VCock_RunNormalMotion(dT);
		}
	}
	else if(stopState == STOP_STATE3) {
		if(azDir == 0.0F) {
			stopState = STOP_STATE2;
		}
	}
}

void OTWDriverClass::VCock_RunNormalMotion(float dT) 
{
	stopState	= STOP_STATE0;

   if (!mUseHeadTracking)
   {
	   eyePan		-= azDir * slewRate * 4.0F * dT;
	   eyeTilt		+= elDir * slewRate * 4.0F * dT;
/*
	   if(eyeTilt <= -90.0F * DTR) {
		   eyePan		= min(max(eyePan, -PAN_LIMIT * DTR), PAN_LIMIT * DTR);
		   eyeTilt		= min(max(eyeTilt, -140.0F * DTR), 25.0F * DTR);
   //		eyeTilt		= min(max(eyeTilt, -150.0F * DTR), 25.0F * DTR);
		   BuildHeadMatrix(TRUE, YAW_PITCH, eyePan + 180.0F * DTR, -(eyeTilt + 180.0F * DTR), 0.0F);
	   }
	   else { */
	   //Wombat778 2-21-2004  Changed the expandedheadrange variable to the following independant adjustments.  This should allow a suitable head range
	   // to be selected as more complete 3d pits get built in the future
		   switch (g_n3DHeadPanRange)
		   {
		   case 0:																	//MPS default pan stops
			   eyePan		= min(max(eyePan, -PAN_LIMIT * DTR), PAN_LIMIT * DTR);			   
			   break;		   
		   case 1:																	//Stops removed.  +-180degrees
			   eyePan		= min(max(eyePan, -180.0f * DTR), 180.0f * DTR);			
			   break;
		   case 2:																	//Wraparound left/right
			   if (eyePan > 180.0f * DTR) eyePan-=360.0f *DTR;
			   else if (eyePan < -180.0f * DTR) eyePan+=360.0f *DTR;			   
			   break;
		   default:
			   eyePan		= min(max(eyePan, -PAN_LIMIT * DTR), PAN_LIMIT * DTR);			   
			   break;
		   }			   

		   switch (g_n3DHeadTiltRange)
		   {

   		   case 0:																	//MPS default tilt
			   eyeTilt		= min(max(eyeTilt, -140.0F * DTR), 25.0F * DTR);			  
			   break;
		   case 1:																	//BMS default tilt.  Takes FOV into account
			   if (GetFOV() < 60.0F * DTR)											//Wombat778 10-23-2003  Dont do anything with FOV if it is greater than 60
				   eyeTilt		= min(max(eyeTilt, -140.0F * DTR), (35.0F + ((60.0F - (GetFOV()* RTD)))*0.395) * DTR); 
			   else
				   eyeTilt		= min(max(eyeTilt, -140.0F * DTR), 35.0F * DTR);
			   break;
		   case 2:																	//Significantly expanded tilt range.  Can look 90 degrees down		
			   eyeTilt		= min(max(eyeTilt, -140.0F * DTR), 90.0F * DTR);
			   break;
		   case 3:																	//Full vertical range +- 180 degrees
			   eyeTilt		= min(max(eyeTilt, -180.0F * DTR), 180.0F * DTR);			   
			   break;
		   case 4:																	//Wraparound tilt
			   if (eyeTilt > 180.0f * DTR) eyeTilt-=360.0f *DTR;
			   else if (eyeTilt < -180.0f * DTR) eyeTilt+=360.0f *DTR;
			   break;
		   default:
			   if (GetFOV() < 60.0F * DTR)										
				   eyeTilt		= min(max(eyeTilt, -110.0F * DTR), (35.0F + ((60.0F - (GetFOV()* RTD)))*0.395) * DTR); 
			   else
				   eyeTilt		= min(max(eyeTilt, -110.0F * DTR), 35.0F * DTR);
			   break;
		   }			   

   }
   else
   {

      eyePan = cockpitFlightData.headYaw;
      eyeTilt = cockpitFlightData.headPitch;
      eyeHeadRoll = cockpitFlightData.headRoll;
   	BuildHeadMatrix(FALSE, YAW_PITCH, eyePan, eyeTilt, eyeHeadRoll);
   }
}



void OTWDriverClass::VCock_Glance(float dT)
{
   // No glances when using a head tracker
   if (mUseHeadTracking)
      return;

	if(padlockGlance == GlanceNose) {					// if player glances forward

		if(!mIsSlewInit) {
			mIsSlewInit = TRUE;
			mSlewPStart				= eyePan;
			mSlewTStart				= eyeTilt;
		}
		PadlockF3_SlewCamera(mSlewPStart, mSlewTStart, 0.0F, 0.0F, 5.0F, 0.001F, dT);
	}
	else if (padlockGlance == GlanceTail) {			// if player glances back

		if(eyePan < 0.0F) {

			if(!mIsSlewInit) {
				mIsSlewInit = TRUE;
				mSlewPStart				= eyePan;
				mSlewTStart				= eyeTilt;
			}

			PadlockF3_SlewCamera(mSlewPStart, mSlewTStart, -180.0F * DTR,  0.0F, 5.0F, 0.001F, dT);
		}
		else if(eyePan > 0.0F) {

			if(!mIsSlewInit) {
				mIsSlewInit = TRUE;
				mSlewPStart				= eyePan;
				mSlewTStart				= eyeTilt;
			}

			PadlockF3_SlewCamera(mSlewPStart, mSlewTStart, 180.0F * DTR, 0.0F, 5.0F, 0.001F, dT);
		}
		else {
			eyePan	= 0.001F;
		}
	}
	else {
		padlockGlance = GlanceNone;
	}
}



void OTWDriverClass::VCock_GiveGilmanHead(float dT)
{
   // No limits when using a head tracker
	if(padlockGlance != GlanceNone) {
		VCock_Glance(dT);
		BuildHeadMatrix(FALSE, YAW_PITCH, eyePan, eyeTilt, 0.0F);
	}
	else {
		if(headMotion == YAW_PITCH) {
			if(eyePan <= -PAN_LIMIT * DTR ||  eyePan >= PAN_LIMIT * DTR) {
				VCock_CheckStopStates(dT);
			}
			else {
				VCock_RunNormalMotion(dT);
			}
		}

		if(headMotion == HEAD_TRANSISTION1) {

			if(initialTilt <= -90.0F * DTR) {
				BuildHeadMatrix(TRUE, YAW_PITCH, eyePan, eyeTilt, 0.0F);
				headMotion	= HEAD_TRANSISTION2;
			}
			else if(initialTilt > -90.0F * DTR && eyeTilt > -92.0F * DTR) {

				eyeTilt -= slewRate * 10.0F * dT;

				eyeTilt = max(eyeTilt, -92.0F * DTR);
				if(eyeTilt >= -90.0F * DTR) {
					BuildHeadMatrix(FALSE, YAW_PITCH, eyePan, eyeTilt, 0.0F);
				}
				else {
					BuildHeadMatrix(TRUE, YAW_PITCH, eyePan, eyeTilt, 0.0F);
				}
			}
			else {
				eyeTilt		= -92.0F * DTR;
				headMotion	= HEAD_TRANSISTION2;
			}
		}

		if(headMotion == HEAD_TRANSISTION2) {


			if((snapDir == RTOL || snapDir == LTOR) && ((eyePan >= PAN_LIMIT * DTR) || (eyePan <= -PAN_LIMIT * DTR))) {
				eyePan		-= snapDir * slewRate * 10.0F * dT;
				if(eyePan > 180.0F * DTR) {
					eyePan = -360.0F * DTR + eyePan;
				}
				else if(eyePan < -180.0F * DTR) {
					eyePan = 360.0F * DTR + eyePan;
				}

				if(eyePan < 0.0F) {
					eyePan = min(eyePan, -PAN_LIMIT * DTR);
					if(eyePan == -PAN_LIMIT * DTR) {
						headMotion = HEAD_TRANSISTION3;			
					}
				}
				else {

					eyePan = max(eyePan, PAN_LIMIT * DTR);
					if(eyePan == PAN_LIMIT * DTR) {
						headMotion = HEAD_TRANSISTION3;			
					}
				}
				BuildHeadMatrix(TRUE, YAW_PITCH, eyePan, eyeTilt, 0.0F);
			}
			else {
				eyePan	= max(min(eyePan, -PAN_LIMIT * DTR), PAN_LIMIT * DTR);
				headMotion = HEAD_TRANSISTION3;
			}
		}

		if(headMotion == HEAD_TRANSISTION3) {

			if(/*azDir &&*/ initialTilt >= -92.0F * DTR){

				if(eyeTilt < initialTilt) {
					eyeTilt += slewRate * 10.0F * dT;
					eyeTilt = min(eyeTilt, initialTilt);
				}
				else {
					stopState = STOP_STATE3;

					eyeTilt = initialTilt;
					headMotion	= YAW_PITCH;
				}

				if(eyeTilt >= -90.0F * DTR) {
					BuildHeadMatrix(FALSE, YAW_PITCH, eyePan, eyeTilt, 0.0F);
				}
				else {
					BuildHeadMatrix(TRUE, YAW_PITCH, eyePan, eyeTilt, 0.0F);
				}
			}
			else {
				stopState = STOP_STATE3;

				eyeTilt = initialTilt;
				headMotion	= YAW_PITCH;
				BuildHeadMatrix(TRUE, YAW_PITCH, eyePan, eyeTilt, 0.0F);
			}
		}
	}
	// Combine the head and airplane matrices
	MatrixMult (&ownshipRot, &headMatrix, &cameraRot);
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

Tpoint vHUDul = { 20.063f , -2.75f, -0.456f };
Tpoint vHUDur = { 20.063f ,  2.75f, -0.456f };
Tpoint vHUDll = { 20.063f , -2.75f, 4.633f };

Tpoint vRWRul = { 18.780f , -4.368f, 5.147f };
Tpoint vRWRur = { 18.779f , -2.486f, 5.147f };
Tpoint vRWRll = { 18.676f , -4.368f, 7.018f };

Tpoint vMACHul = { 21.178f , -1.853f, 8.934f };
Tpoint vMACHur = { 21.178f , -0.053f, 8.934f };
Tpoint vMACHll = { 21.085f , -1.853f, 10.732f };



Tpoint vDEDul = { 18.637f ,  2.577f, 5.180f };
Tpoint vDEDur = { 18.637f ,  6.777f, 5.180f };
Tpoint vDEDll = { 18.474f ,  2.577f, 6.165f };

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
Tpoint	vOILul = { 17.990f ,  7.976f, 8.823f };
Tpoint	vOILur = { 17.990f ,  8.676f, 8.823f };
Tpoint	vOILll = { 17.870f ,  7.976f, 9.512f };

int		vOILepts		= 3;
float		vOILvals[3] = {0.0f, 100.0f, 103.3f};
float		vOILpts[3]	= {-0.646f, 0.723f, 0.513f};
//-------------------------------------------------
Tpoint	vNOZul = { 17.800f ,  8.076f, 9.906f };
Tpoint	vNOZur = { 17.800f ,  9.076f, 9.906f };
Tpoint	vNOZll = { 17.627f ,  8.076f, 10.891f };

int		vNOZepts		= 2;
float		vNOZvals[2] = {0.0F, 100.0F};
float		vNOZpts[2]	= {0.944F, 2.269F};
//-------------------------------------------------
Tpoint	vRPMul = { 17.575f ,  8.076f, 11.186f };
Tpoint	vRPMur = { 17.575f ,  9.376f, 11.186f };
Tpoint	vRPMll = { 17.349f ,  8.076f, 12.467f };

int		vRPMepts		= 4;
float		vRPMvals[4]	= {0.0F, 60.0F, 100.0F, 110.0F};
float		vRPMpts[4]	= {1.571F, 0.0F, 3.142F, 2.307F};
//-------------------------------------------------
Tpoint	vFTITul = { 17.226f ,  8.675f, 13.156f };
Tpoint	vFTITur = { 17.226f ,  9.875f, 13.156f };
Tpoint	vFTITll = { 17.017f ,  8.675f, 14.338f };

int		vFTITepts	= 6;
float		vFTITvals[6] = {2.0F, 6.0F, 8.0F, 9.0F, 10.0F, 12.0F};
float		vFTITpts[6]	= {-0.319F, -1.445F, -2.808F, 2.412F, 1.208F, 0.621F};
//-------------------------------------------------
Tpoint vALTul = { 21.178f ,  0.247f, 8.934f };
Tpoint vALTur = { 21.178f ,  2.047f, 8.934f };
Tpoint vALTll = { 21.085f ,  0.239f, 10.732f };

int		vALTepts		= 2;
float		vALTvals[2]	= {0.0F, 1000.0F};
float		vALTpts[2]	= {1.57F, 1.571F};

//-------------------------------------------------

#include "cpres.h"

bool OTWDriverClass::VCock_SetCanvas(char **plinePtr, Canvas3D **canvaspp)
{
    Tpoint ul, ur, ll;
    Canvas3D *canvas;
    char *ptoken = FindToken(plinePtr, "=;\n");	
    if (sscanf(ptoken, "%f %f %f %f %f %f %f %f %f", 
	&ul.x, &ul.y, &ul.z,
	&ur.x, &ur.y, &ur.z,
	&ll.x, &ll.y, &ll.z) != 9) {
	ShiAssert(!"Failed to parse canvas");
	*canvaspp = NULL;
	return false;
    }
    *canvaspp = canvas = new Canvas3D;
    canvas->Setup(renderer);
    canvas->SetCanvas(&ul, &ur, &ll);
    canvas->Update( (struct Tpoint *)&Origin, (struct Trotation *)&IMatrix );
    
    return true;
}

void
OTWDriverClass::VCock_ParseVDial(FILE *fp)
{
    VDialInitStr vdialInitStr;
    static const char		pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
    int				valuesIndex = 0;
    int				pointsIndex = 0;
    char			plineBuffer[MAX_LINE_BUFFER];
    char *plinePtr, *ptoken;
    Tpoint ur, ul, ll;
    
    ZeroMemory(&vdialInitStr, sizeof vdialInitStr);
    vdialInitStr.callback = -1;

    fgets(plineBuffer, sizeof plineBuffer, fp);
    plinePtr = plineBuffer;
    ptoken = FindToken(&plinePtr, pseparators);	
    vdialInitStr.ppoints = NULL;
    vdialInitStr.pvalues = NULL;
    
    while(strcmpi(ptoken, END_MARKER)){
	
	if(!strcmpi(ptoken, PROP_NUMENDPOINTS_STR)) {
	    ptoken = FindToken(&plinePtr, pseparators);	
	    sscanf(ptoken, "%d", &vdialInitStr.endPoints);
	    vdialInitStr.ppoints = new float[vdialInitStr.endPoints];
	    vdialInitStr.pvalues = new float[vdialInitStr.endPoints];
	}
	else if(!strcmpi(ptoken, PROP_POINTS_STR)) {
	    ptoken = FindToken(&plinePtr, pseparators);
	    while(ptoken) {
		F4Assert(pointsIndex < vdialInitStr.endPoints);
		sscanf(ptoken, "%f", &vdialInitStr.ppoints[pointsIndex]);
		ptoken = FindToken(&plinePtr, pseparators);
		pointsIndex++;
	    }
	}
	else if(!strcmpi(ptoken, PROP_VALUES_STR)) {
	    ptoken = FindToken(&plinePtr, pseparators);
	    while(ptoken) {
		F4Assert(valuesIndex < vdialInitStr.endPoints);
		sscanf(ptoken, "%f", &vdialInitStr.pvalues[valuesIndex]);
		ptoken = FindToken(&plinePtr, pseparators);
		valuesIndex++;
	    }
	}
	else if(!strcmpi(ptoken, PROP_RADIUS0_STR)) {
	    ptoken = FindToken(&plinePtr, pseparators);	
	    sscanf(ptoken, "%f", &vdialInitStr.radius);
	}
	else if(!strcmpi(ptoken, PROP_COLOR0_STR)) {
	    ptoken = FindToken(&plinePtr, pseparators);	
	    sscanf(ptoken, "%lx", &vdialInitStr.color);
	}
	else if(!strcmpi(ptoken, PROP_CALLBACKSLOT_STR)) {
	    ptoken = FindToken(&plinePtr, pseparators);	
	    sscanf(ptoken, "%d", &vdialInitStr.callback);
	}
	else if (!strcmpi(ptoken, PROP_DESTLOC_STR)) {
	    ptoken = FindToken(&plinePtr, "=;\n");	
	    if (sscanf(ptoken, "%f %f %f %f %f %f %f %f %f", 
		&ul.x, &ul.y, &ul.z,
		&ur.x, &ur.y, &ur.z,
		&ll.x, &ll.y, &ll.z) == 9) {
		vdialInitStr.pUL = &ul;
		vdialInitStr.pUR = &ur;
		vdialInitStr.pLL = &ll;
	    }
	}
	else {
	    F4Assert(!"Unknown Line in dial defn");
	}
	
	fgets(plineBuffer, sizeof plineBuffer, fp);
	plinePtr = plineBuffer;
	ptoken = FindToken(&plinePtr, pseparators);	
    }
    vdialInitStr.pRender		= renderer;
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
    CP_HANDLE*			pcockpitDataFile;
    static const char		pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
    extern Tpoint lMFDul, lMFDur, lMFDll;
    extern Tpoint rMFDul, rMFDur, rMFDll;

    int DebugLineNum;
    bool quitFlag = false;
    
    FindCockpit(pCPFile, (Vis_Types)eCPVisType, eCPName, eCPNameNCTR, strCPFile);

    pcockpitDataFile = CP_OPEN(strCPFile, "r");
  
    F4Assert(pcockpitDataFile);			//Error: Couldn't open file
    DebugLineNum = 0;

    while(!quitFlag) {
	char			plineBuffer[MAX_LINE_BUFFER];
	char *plinePtr, *ptoken;
	char *presult	= fgets(plineBuffer, sizeof plineBuffer, pcockpitDataFile);
	DebugLineNum ++;
	quitFlag	= (presult == NULL);
	
	if (quitFlag || *plineBuffer == '/' || *plineBuffer == '\n')
	    continue;
	plinePtr = plineBuffer;
	ptoken = FindToken(&plinePtr, pseparators);

	if (!strcmpi(ptoken, PROP_HUD_STR)) { // the hud
	    if (!VCock_SetCanvas(&plinePtr, &vcInfo.vHUDrenderer)) {
				plinePtr = plinePtr; // Release mode compile warning
				F4Assert("Bad HUD description");
			}
	}
	else if (!strcmpi(ptoken, PROP_RWR_STR)) { //  the rwr
	    if (!VCock_SetCanvas(&plinePtr, &vcInfo.vRWRrenderer)) {
				plinePtr = plinePtr; // Release mode compile warning
				F4Assert("Bad RWR description");
			}
	}
	else if (!strcmpi(ptoken, TYPE_DED_STR)) { //  the rwr
	    if (!VCock_SetCanvas(&plinePtr, &vcInfo.vDEDrenderer)) {
				plinePtr = plinePtr; // Release mode compile warning
				F4Assert("Bad DED description");
			}
	}
	else if (!strcmpi(ptoken, TYPE_MACHASI_STR)) { //  the rwr
	    if (!VCock_SetCanvas(&plinePtr, &vcInfo.vMACHrenderer)) {
				plinePtr = plinePtr; // Release mode compile warning
				F4Assert("Bad MACH description");
			}
	}
	else if (!strcmpi(ptoken, PROP_MFDLEFT_STR)) {// left MFD
	    Tpoint ul, ur, ll;
	    ptoken = FindToken(&plinePtr, "=;\n");	
	    if (sscanf(ptoken, "%f %f %f %f %f %f %f %f %f", 
		&ul.x, &ul.y, &ul.z,
		&ur.x, &ur.y, &ur.z,
		&ll.x, &ll.y, &ll.z) == 9) {
		lMFDul = ul;
		lMFDur = ur;
		lMFDll = ll;
	    }
	}
	else if (!strcmpi(ptoken, PROP_MFDRIGHT_STR)) {// right MFD
	    Tpoint ul, ur, ll;
	    ptoken = FindToken(&plinePtr, "=;\n");	
	    if (sscanf(ptoken, "%f %f %f %f %f %f %f %f %f", 
		&ul.x, &ul.y, &ul.z,
		&ur.x, &ur.y, &ur.z,
		&ll.x, &ll.y, &ll.z) == 9) {
		rMFDul = ul;
		rMFDur = ur;
		rMFDll = ll;
	    }
	}
	else if(!strcmpi(ptoken, TYPE_DIAL_STR)) {
	    VCock_ParseVDial(pcockpitDataFile);
	}
	else if (!strcmpi(ptoken, PROP_3D_PADBACKGROUND)) {
	    ptoken = FindToken(&plinePtr, pseparators);	
	    sscanf(ptoken, "%lx", &pVColors[0][0]);
	    pVColors[1][0] = CalculateNVGColor(pVColors[0][0]);
	}
	else if (!strcmpi(ptoken, PROP_3D_PADLIFTLINE)) {
	    ptoken = FindToken(&plinePtr, pseparators);	
	    sscanf(ptoken, "%lx", &pVColors[0][1]);
	    pVColors[1][1] = CalculateNVGColor(pVColors[0][1]);
	}
	else if (!strcmpi(ptoken, PROP_3D_PADBOXSIDE)) {
	    ptoken = FindToken(&plinePtr, pseparators);	
	    sscanf(ptoken, "%lx", &pVColors[0][2]);
	    pVColors[1][2] = CalculateNVGColor(pVColors[0][2]);
	}
	else if (!strcmpi(ptoken, PROP_3D_PADBOXTOP)) {
	    ptoken = FindToken(&plinePtr, pseparators);	
	    sscanf(ptoken, "%lx", &pVColors[0][3]);
	    pVColors[1][3] = CalculateNVGColor(pVColors[0][3]);
	}
	else if (!strcmpi(ptoken, PROP_3D_PADTICK)) {
	    ptoken = FindToken(&plinePtr, pseparators);	
	    sscanf(ptoken, "%lx", &pVColors[0][4]);
	    pVColors[1][4] = CalculateNVGColor(pVColors[0][4]);
	}
	else if (!strcmpi(ptoken, PROP_3D_NEEDLE0)) {
	    ptoken = FindToken(&plinePtr, pseparators);	
	    sscanf(ptoken, "%lx", &pVColors[0][5]);
	    pVColors[1][5] = CalculateNVGColor(pVColors[0][5]);
	}
	else if (!strcmpi(ptoken, PROP_3D_NEEDLE1)) {
	    ptoken = FindToken(&plinePtr, pseparators);	
	    sscanf(ptoken, "%lx", &pVColors[0][6]);
	    pVColors[1][6] = CalculateNVGColor(pVColors[0][6]);
	}
	else if (!strcmpi(ptoken, PROP_3D_DED)) {
	    ptoken = FindToken(&plinePtr, pseparators);	
	    sscanf(ptoken, "%lx", &pVColors[0][7]);
	    pVColors[1][7] = CalculateNVGColor(pVColors[0][7]);
	}
	else if (!strcmpi(ptoken, PROP_3D_COCKPIT)) {
	    ptoken = FindToken(&plinePtr, pseparators);	
	    sscanf(ptoken, "%d", &vrCockpitModel[0]);
	}
	else if (!strcmpi(ptoken, PROP_3D_COCKPITDF)) {
	    ptoken = FindToken(&plinePtr, pseparators);	
	    sscanf(ptoken, "%d", &vrCockpitModel[1]);
	}
	else if (!strcmpi(ptoken, PROP_3D_MAINMODEL)) {
	    ptoken = FindToken(&plinePtr, pseparators);	
	    sscanf(ptoken, "%d", &vrCockpitModel[2]);
	}
	else if (!strcmpi(ptoken, PROP_3D_DAMAGEDMODEL)) {
	    ptoken = FindToken(&plinePtr, pseparators);	
	    sscanf(ptoken, "%d", &vrCockpitModel[3]);
	}
	else if (!strcmpi(ptoken, PROP_LIFT_LINE_COLOR)) {
		ptoken = FindToken(&plinePtr, pseparators);
		sscanf(ptoken, "%lx", &liftlinecolor);
	}
	else {
	    F4Assert(!"Unknown Line in 3dfile");
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
OTWDriverClass::VCock_Init( void )
{
    VDialInitStr vdialInitStr;
    
    mNumVDials		= 5;
    mpVDials			= new VDial*[mNumVDials];
    
    vcInfo.vHUDrenderer = new Canvas3D;
   	vcInfo.vHUDrenderer->Setup(renderer);
	vcInfo.vHUDrenderer->SetCanvas(&vHUDul, &vHUDur, &vHUDll);
	vcInfo.vHUDrenderer->Update( (struct Tpoint *)&Origin, (struct Trotation *)&IMatrix );
	
   	vcInfo.vRWRrenderer = new Canvas3D;
   	vcInfo.vRWRrenderer->Setup(renderer);
   	vcInfo.vRWRrenderer->SetCanvas(&vRWRul, &vRWRur, &vRWRll);
   	vcInfo.vRWRrenderer->Update( (struct Tpoint *)&Origin, (struct Trotation *)&IMatrix );

   	vcInfo.vMACHrenderer = new Canvas3D;
   	vcInfo.vMACHrenderer->Setup(renderer);
   	vcInfo.vMACHrenderer->SetCanvas(&vMACHul, &vMACHur, &vMACHll);
   	vcInfo.vMACHrenderer->Update( (struct Tpoint *)&Origin, (struct Trotation *)&IMatrix );

   	vcInfo.vDEDrenderer = new Canvas3D;
   	vcInfo.vDEDrenderer->Setup(renderer);
   	vcInfo.vDEDrenderer->SetCanvas(&vDEDul, &vDEDur, &vDEDll);
   	vcInfo.vDEDrenderer->Update( (struct Tpoint *)&Origin, (struct Trotation *)&IMatrix );

		//---------------------------------->
		// Oil Gauge
		vdialInitStr.callback	= 43;
		vdialInitStr.pUL			= &vOILul;
		vdialInitStr.pUR			= &vOILur;
		vdialInitStr.pLL			= &vOILll;
		vdialInitStr.pRender		= renderer;
		vdialInitStr.radius		= 0.85F;
		vdialInitStr.color		= pVColors[0][5];
		vdialInitStr.endPoints	= vOILepts;
		vdialInitStr.pvalues		= vOILvals;
		vdialInitStr.ppoints		= vOILpts;

		mpVDials[0] = new VDial(&vdialInitStr);
		//<----------------------------------

		//---------------------------------->
		// Nozzle Position
		vdialInitStr.callback	= 41;
		vdialInitStr.pUL			= &vNOZul;
		vdialInitStr.pUR			= &vNOZur;
		vdialInitStr.pLL			= &vNOZll;
		vdialInitStr.pRender		= renderer;
		vdialInitStr.radius		= 0.85F;
		vdialInitStr.color		= pVColors[0][5];
		vdialInitStr.endPoints	= vNOZepts;
		vdialInitStr.pvalues		= vNOZvals;
		vdialInitStr.ppoints		= vNOZpts;

		mpVDials[1] = new VDial(&vdialInitStr);
		//<----------------------------------

		//---------------------------------->
		// RPM Gauge
		vdialInitStr.callback	= 40;
		vdialInitStr.pUL			= &vRPMul;
		vdialInitStr.pUR			= &vRPMur;
		vdialInitStr.pLL			= &vRPMll;
		vdialInitStr.pRender		= renderer;
		vdialInitStr.radius		= 0.85F;
		vdialInitStr.color		= pVColors[0][5];
		vdialInitStr.endPoints	= vRPMepts;
		vdialInitStr.pvalues		= vRPMvals;
		vdialInitStr.ppoints		= vRPMpts;

		mpVDials[2] = new VDial(&vdialInitStr);
		//<----------------------------------

		//---------------------------------->
		// FTIT Indicator
		vdialInitStr.callback	= 42;
		vdialInitStr.pUL			= &vFTITul;
		vdialInitStr.pUR			= &vFTITur;
		vdialInitStr.pLL			= &vFTITll;
		vdialInitStr.pRender		= renderer;
		vdialInitStr.radius		= 0.85F;
		vdialInitStr.color		= pVColors[0][5];
		vdialInitStr.endPoints	= vFTITepts;
		vdialInitStr.pvalues		= vFTITvals;
		vdialInitStr.ppoints		= vFTITpts;

		mpVDials[3] = new VDial(&vdialInitStr);
		//<----------------------------------


		//---------------------------------->
		// Altimeter
		vdialInitStr.callback	= 44;
		vdialInitStr.pUL			= &vALTul;
		vdialInitStr.pUR			= &vALTur;
		vdialInitStr.pLL			= &vALTll;
		vdialInitStr.pRender		= renderer;
		vdialInitStr.radius		= 0.85F;
		vdialInitStr.color		= pVColors[0][5];
		vdialInitStr.endPoints	= vALTepts;
		vdialInitStr.pvalues		= vALTvals;
		vdialInitStr.ppoints		= vALTpts;

		mpVDials[4] = new VDial(&vdialInitStr);
		//<----------------------------------
}

#endif
/*
** DoVirtualCockpit
*/
void
OTWDriverClass::VCock_Exec( void )
{
    int				i;
    Tpoint			tempLight, worldLight;
    PlayerRwrClass	*rwr;
    float			x1, y1, x2, y2;
    mlTrig			trig;
    DrawableBSP*	child;
    int				stationNum;
    SMSClass		*sms = SimDriver.playerEntity->Sms;
    int				oldState;
    int oldFont = VirtualDisplay::CurFont();

	//Codec's moving surfraces in 3D pit
	/* MLR 2003-10-06 commented these out
	static const int dofmap[] = { 	
	    COMP_LT_STAB, 	
		COMP_RT_STAB,
		COMP_LT_FLAP,
		COMP_RT_FLAP,
		COMP_RUDDER,
		COMP_LT_LEF,
		COMP_RT_LEF,
		COMP_LT_AIR_BRAKE_TOP,
		COMP_LT_AIR_BRAKE_BOT,
		COMP_RT_AIR_BRAKE_TOP,
		COMP_RT_AIR_BRAKE_BOT,
		COMP_CANOPY_DOF,
	};
	static const int dofmap_size = sizeof(dofmap) / sizeof(dofmap[0]);

	//MI
	static const int switchmap[] = {
		COMP_WING_VAPOR,
		COMP_TAIL_STROBE,
		COMP_NAV_LIGHTS,
	};
	static const int switchmap_size = sizeof(switchmap) / sizeof(switchmap[0]);
	*/

    // Make sure we don't get in here when we shouldn't
    ShiAssert( otwPlatform );
    ShiAssert( otwPlatform->IsSetFlag(MOTION_OWNSHIP) );
    ShiAssert( otwPlatform == SimDriver.playerEntity );
    ShiAssert( sms );	// If we legally might not have one, then we'd have to skip the ordinance...
    
	/*
	** Render the 3d cockpit object
	*/
/*
// VWF 3/3/99 Added for Chris W.'s demo
	static float angle = 0.0F;
	static float offset = 0.0F;

	angle += PI/180.0F;
	offset += 0.01F;

	if(offset >= 0.25F) {
		offset = -0.25F;
	}

	vrCockpit->SetDOFangle(0, angle );
	vrCockpit->SetDOFangle(1, angle );

	vrCockpit->SetDOFangle(12, angle );
	vrCockpit->SetDOFangle(13, angle );
	vrCockpit->SetDOFangle(14, angle );
	vrCockpit->SetDOFangle(15, angle );
	vrCockpit->SetDOFangle(16, angle );
	vrCockpit->SetDOFangle(17, angle );
//	vrCockpit->SetDOFangle(18, angle );

//	vrCockpit->SetDOFoffset(19, offset );
//	vrCockpit->SetDOFoffset(20, offset );
*/	

	ShiAssert(vrCockpit);
	if (!vrCockpit) // CTD fix
		return;

	/* MLR 2003-10-05 replace by new code
	//Codec's moving surfraces in 3D pit
	for (i = 0; i < dofmap_size; i++)
	{
		if(dofmap[i] == COMP_LT_STAB || dofmap[i] == COMP_RT_STAB)
			vrCockpit->SetDOFangle(dofmap[i], -SimDriver.playerEntity->GetDOFValue(dofmap[i]));
		else
			vrCockpit->SetDOFangle(dofmap[i], SimDriver.playerEntity->GetDOFValue(dofmap[i]));
	}

	//MI
	for(i = 0; i < switchmap_size; i++)
		vrCockpit->SetSwitchMask(switchmap[i], SimDriver.playerEntity->GetSwitch(switchmap[i]));
	*/

	// MLR 2003-10-12
	// I moved all my previous animation code to the AircraftClass, 
	// it's more readily (more likely) to get updated there as new 
	// DOFs and Switches are added.
	//
	// Also both 2d and 3d pit had the exact same duplicate code.
	SimDriver.playerEntity->CopyAnimationsToPit(vrCockpit);


	{   // MLR 2003-10-05 This needs to be moved so it only runs once
		DrawableBSP *bsp=(DrawableBSP*)SimDriver.playerEntity->drawPointer;
		int t = bsp->GetTextureSet();
		vrCockpit->SetTextureSet(t % vrCockpit->GetNTextureSet());
	}



    // master caution light
    if ( pCockpitManager->mMiscStates.GetMasterCautionLight() )
	vrCockpit->SetSwitchMask( 2, 1);
    else
	vrCockpit->SetSwitchMask( 2, 0);

    // Setup the local lighting and transform environment (body relative)
    renderer->GetLightDirection( &worldLight );
    MatrixMultTranspose( &ownshipRot, &worldLight, &tempLight ); 
    renderer->SetLightDirection( &tempLight );
    
    static float PreviousRoll = 0.0f;
    static float PreviousOmega = 0.0f;
    static long PreviousTime = vuxGameTime;
    
    if (g_b3dDynamicPilotHead) // JB 010804
    {
	Tpoint origin = {0.0, 0.0, 0.0};
	float dt = (vuxGameTime - PreviousTime) / 2000.0;

	if (dt)
	{
	    origin.y = (eyePan / PI) * cos(eyeTilt);
	    float rollrate = cockpitFlightData.roll - PreviousRoll;
	    if (rollrate > PI)
		rollrate = -(PI - cockpitFlightData.roll + PI + PreviousRoll);
	    else if (rollrate < -PI)
		rollrate = PI + cockpitFlightData.roll + PI - PreviousRoll;
	    
	    float omega = rollrate / dt;
	    float alpha = (PreviousOmega - omega) / dt;
	    origin.y += alpha / -54;
	    origin.z = cockpitFlightData.gs * 0.04f;
	    PreviousOmega = omega;
	    PreviousTime = vuxGameTime;
	}
	
	PreviousRoll = cockpitFlightData.roll;
	
	renderer->SetCamera( &origin, &headMatrix );
    }
    else
	renderer->SetCamera( &Origin, &headMatrix );

    // Attach the appropriate ordinance
    if (g_b3dCockpit) // JB 010802
    {
	for (stationNum=1; stationNum < sms->NumHardpoints(); stationNum++)
	{
	    if (sms->hardPoint[stationNum]->GetRack()) {
		child = sms->hardPoint[stationNum]->GetRack();
		vrCockpit->AttachChild( child, stationNum-1 );
	    } else if (sms->hardPoint[stationNum]->weaponPointer && sms->hardPoint[stationNum]->weaponPointer->drawPointer) {
		child = (DrawableBSP*)(sms->hardPoint[stationNum]->weaponPointer->drawPointer);
		vrCockpit->AttachChild( child, stationNum-1 );
	    }
	}
	
	// Draw the cockpit object
	oldState = renderer->GetObjectTextureState();
	renderer->SetObjectTextureState( TRUE );
	vrCockpit->Draw( renderer );
	renderer->SetObjectTextureState( oldState );
    }
    
    // Put the light back into world space
    renderer->SetLightDirection( &worldLight );
	
    
    /*
    ** Do HUD
    */
    if (vcInfo.vHUDrenderer) // JPO - use basic info
    {
	renderer->SetColor (TheHud->GetHudColor());
	// set the HUD half angle
	float hudangy, hudangx, ratio;
	hudangy = ( vHUDur.y - vHUDul.y ) * 0.50f;
	hudangx = vHUDur.x;
	
	// the hud half angle -- ratio of tangents (?)
	// ratio = ( hudangy/hudangx )/tan( 30.0f * DTR );
	ratio = ( hudangy/hudangx );
	
	TheHud->SetHalfAngle((float)atan (ratio) * RTD);
	// TheHud->SetHalfAngle(atan (hudangy/hudangx) * RTD);
	
	// hack!  move borsight height to 0.75.  sigh.
	hudWinY[BORESIGHT_CROSS_WINDOW] = 0.75f;
	
	TheHud->SetTarget( TheHud->Ownship()->targetPtr );
	VirtualDisplay::SetFont(pCockpitManager->HudFont());
	TheHud->Display(vcInfo.vHUDrenderer);
	VirtualDisplay::SetFont(oldFont);
	renderer->SetColor (0xff00ff00);
	// restore hud half angle
	TheHud->SetHalfAngle((float)atan (0.25 * (float)tan(30.0F * DTR)) * RTD);
	// hack!  restore borsight height to 0.60.  sigh.
	hudWinY[BORESIGHT_CROSS_WINDOW] = 0.60f;
    }

    /*
    ** Do MFDs
    */
    // don't need to update pos here since it's always relative
    // to origin and identity matrix
    // MfdDisplay[i]->UpdateVirtualPosition(&Origin, &IMatrix);
    if (g_b3dMFDLeft) // JB 010802
	MfdDisplay[0]->Exec(TRUE, TRUE);
    
    if (g_b3dMFDRight) // JB 010802
	MfdDisplay[1]->Exec(TRUE, TRUE);

	/*
	** Do RWR
    */
    rwr = (PlayerRwrClass*)FindSensor( (SimMoverClass *)otwPlatform, SensorClass::RWR);
    if (rwr && vcInfo.vRWRrenderer)
    {
	rwr->SetGridVisible(FALSE);
	rwr->Display(vcInfo.vRWRrenderer);
    }
    
    /*
    ** Do DED
    */
    //MI Original Code
    if(pCockpitManager->mpIcp && vcInfo.vDEDrenderer) 
    {
	if(!g_bRealisticAvionics)
	{ 
	    pCockpitManager->mpIcp->Exec();
	    //MI changed for ICP Stuff
	    pCockpitManager->mpIcp->GetDEDStrings( dedStr1, dedStr2, dedStr3 );
	    
	    // Check for DED/Avionics failure
	    F4Assert (SimDriver.playerEntity);
	    F4Assert (SimDriver.playerEntity->mFaults);
	    
	    // DED is orange :)
	    OTWDriver.renderer->SetColor(pVColors[TheTimeOfDay.GetNVGmode() != 0][7]);
	    
	    if (!SimDriver.playerEntity->mFaults->GetFault(FaultClass::ufc_fault))
	    {
		vcInfo.vDEDrenderer->TextLeft( -0.90F, 0.99F, dedStr1, FALSE);
		vcInfo.vDEDrenderer->TextLeft( -0.90F, 0.33F, dedStr2, FALSE);
		vcInfo.vDEDrenderer->TextLeft( -0.90F, -0.33F, dedStr3, FALSE);
	    } 
	}
	else
	{
	    //MI modified for ICP Stuff
	    if(SimDriver.playerEntity->mFaults->GetFault(FaultClass::ufc_fault))
		return;
	    
	    if(!SimDriver.playerEntity->HasPower(AircraftClass::UFCPower))
		return;
	    
	    pCockpitManager->mpIcp->Exec();
	    
	    // Check for DED/Avionics failure
	    F4Assert (SimDriver.playerEntity);
	    F4Assert (SimDriver.playerEntity->mFaults);
	    
	    // DED is orange :)
	    OTWDriver.renderer->SetColor(pVColors[TheTimeOfDay.GetNVGmode() != 0][7]);
	    
	    char line1[30] = "";
	    char line2[30] = "";
	    char line3[30] = "";
	    char line4[30] = "";
	    char line5[30] = "";
	    line1[29] = '\0';
	    line2[29] = '\0';
	    line3[29] = '\0';
	    line4[29] = '\0';
	    line5[29] = '\0';
	    for(int j = 0; j < 5; j++)
	    {
		for(int i = 0; i < 26; i++)
		{
		    switch(j)
		    {
		    case 0:
			line1[i] = pCockpitManager->mpIcp->DEDLines[j][i];
			break;
		    case 1:
			line2[i] = pCockpitManager->mpIcp->DEDLines[j][i];
			break;
		    case 2:
			line3[i] = pCockpitManager->mpIcp->DEDLines[j][i];
			break;
		    case 3:
			line4[i] = pCockpitManager->mpIcp->DEDLines[j][i];
			break;
		    case 4:
			line5[i] = pCockpitManager->mpIcp->DEDLines[j][i];
			break;				
		    }
		}
	    }
	    //Line1
	    vcInfo.vDEDrenderer->TextLeft(-0.90F + 0.20F, 1.0F, line1);
	    //Line2
	    vcInfo.vDEDrenderer->TextLeft(-0.90F + 0.20F, 0.60F, line2);
	    //Line3
	    vcInfo.vDEDrenderer->TextLeft(-0.90F + 0.20F, 0.20F, line3);
	    //Line4
	    vcInfo.vDEDrenderer->TextLeft(-0.90F + 0.20F, -0.20F, line4);
	    //Line5
	    vcInfo.vDEDrenderer->TextLeft(-0.90F + 0.20F, -0.60F, line5);
	}
	
	renderer->SetColor ( pVColors[TheTimeOfDay.GetNVGmode() != 0][6] );
    }
    
	
    if (vcInfo.vMACHrenderer) {
	/* Do MACH indictator */
	float kias = ((AircraftClass *)otwPlatform)->af->vcas;
	
	kias = (float)fmod( kias, 1000.0f );
	kias = kias * 0.001f * 2.0F * PI;
	
	x1 = 0.0f;
	y1 = 0.0f;
	mlSinCos (&trig, kias);
	x2 = 0.85f * trig.cos;
	y2 = 0.85f * -trig.sin;
	
	vcInfo.vMACHrenderer->Line( x1, y1, x2, y2 );
    }
    
    renderer->SetColor ( pVColors[TheTimeOfDay.GetNVGmode() != 0][5] );
    
    
    
    for(i = 0; i < mpVDials.size(); i++)
	mpVDials[i]->Exec(SimDriver.playerEntity);

	//Wombat778 10-11-2003 The meat of the clickable cockpit.  Looks for the closest button and executes it.  Also displays button locations in debug mode.
	//Why execute the commands here, you may ask.  Well, because I spent all night trying to get it to run from simouse.cpp and couldnt (strange memory corruption)
	//So, here it is.  It is a hack, but it works.  If you don't like, then YOU fix it;-)

		ThreeDVertex t1;
		gSelectedCursor = 9;				//Wombat778 10-11-2003 set the cursor to the default green cursor

		
		if ((vuxRealTime - gTimeLastMouseMove < SI_MOUSE_TIME_DELTA) && !InExitMenu()) //Wombat778 10-15-2003 added so mouse cursor would disappear after a few seconds standing still. Also dont want two cursors when exit menu is up
		{
			//Wombat778 10-15-2003 Added the following so that mouse cursor could be drawn in green if over a button, red otherwise
			if (g_b3DClickableCursorChange) 
			{
				gSelectedCursor = 0;
				for ( i = 0 ; i < Button3DList.numbuttons ; i++ )  
				{
					renderer->TransformCameraCentricPoint(&Button3DList.buttons[i].loc,&t1);			
					if (sqrt(((gxPos-t1.x)*(gxPos-t1.x))+((gyPos-t1.y)*(gyPos-t1.y)))  <  (float)(DisplayOptions.DispWidth/ 1600.0f) * (Button3DList.buttons[i].dist/(1.5f*(float)GetFOV())))  //Wombat778 10-15-2003 changes changex with gxPos
					{
						gSelectedCursor = 9;
						break;
					}
				}
			}		
		//Wombat778 12-16-2003 moved to vcock.cpp
		//ClipAndDrawCursor(OTWDriver.pCockpitManager->GetCockpitWidth(), OTWDriver.pCockpitManager->GetCockpitHeight());//Wombat778 10-10-2003  Draw the Mouse cursor if 3d clickable cockpit enabled

		}

		
		if (Button3DList.clicked)			//Wombat778 10-11-2003 check if the mouse button has been clicked while in the 3d cockpit
		{
			float closestdistance=9999;	//set these variables to a high value so that we know when it is uninitialized (there is a button 0)
			float tempdistance=9999;
			int closestbutton=9999;

			for ( i = 0 ; i < Button3DList.numbuttons ; i++ )  
			{
				
				if (Button3DList.buttons[i].mousebutton==Button3DList.clicked)		//Wombat778 11-07-2003 Added so that the left and right mouse button can be differentiated
				{
	
					renderer->TransformCameraCentricPoint(&Button3DList.buttons[i].loc,&t1);
					tempdistance=sqrt(((gxPos-t1.x)*(gxPos-t1.x))+((gyPos-t1.y)*(gyPos-t1.y)));
			
					//Normalize the distance so it is affected by the FOV and by the resolution
					//Todo: add something about the SA bar.  Currently, the dist increases too much when it is active
					float td = ((float) DisplayOptions.DispWidth/ 1600.0f) * (Button3DList.buttons[i].dist/(1.5f*(float)GetFOV()));	

					if (tempdistance < td)
						if (tempdistance < closestdistance)			//if the cursor is near more than 1 button, find the closest one
						{
							closestdistance=tempdistance;
							closestbutton=i;
							tempdistance = 9999;

						}
				}
			}

			if (closestbutton != 9999) 
				if (Button3DList.buttons[closestbutton].function)
				{
					Button3DList.buttons[closestbutton].function(1, KEY_DOWN, NULL);
					F4SoundFXSetDist(Button3DList.buttons[closestbutton].sound, FALSE, 0.0f, 1.0f);
				}
		Button3DList.clicked=0;		
		}
		
		if (g_b3DClickableCockpitDebug) {		//Wombat778 10-10-2003 Draw Locations of the 3d buttons when Debug mode is enabled

		
			for ( i = 0 ; i < Button3DList.numbuttons ; i++ )  {

		//		if (i==Button3DList.debugbutton) 
		//			renderer->SetColor(pVColors[TheTimeOfDay.GetNVGmode() != 0][7]);
		//		else
		//			renderer->SetColor (0x000000FF);		//RED
	
				renderer->TransformCameraCentricPoint(&Button3DList.buttons[i].loc,&t1);

				if (t1.csZ<0) {			//Wombat778 10-11-2003 Only show those points in front of us. Why it does this is beyond me.

					renderer->SetColor (0x000000FF);		//RED

				

					renderer->Render2DPoint((UInt16)t1.x,(UInt16)t1.y);
					renderer->Render2DPoint((UInt16)t1.x,(UInt16)t1.y-1);
					renderer->Render2DPoint((UInt16)t1.x,(UInt16)t1.y+1);
					renderer->Render2DPoint((UInt16)t1.x-1,(UInt16)t1.y);
					renderer->Render2DPoint((UInt16)t1.x-1,(UInt16)t1.y-1);
					renderer->Render2DPoint((UInt16)t1.x-1,(UInt16)t1.y+1);
					renderer->Render2DPoint((UInt16)t1.x+1,(UInt16)t1.y);
					renderer->Render2DPoint((UInt16)t1.x+1,(UInt16)t1.y-1);
					renderer->Render2DPoint((UInt16)t1.x+1,(UInt16)t1.y+1);


					renderer->SetColor (0x0000ffff);		//Yellow
							
					//Normalize the distance so it is affected by the FOV and by the resolution
					//Todo: add something about the SA bar.  Currently, the dist increases too much when it is active
					float td = ((float) DisplayOptions.DispWidth/ 1600.0f) * (Button3DList.buttons[i].dist/(1.5f*(float)GetFOV()));	

					renderer->Render2DPoint((UInt16)t1.x-td,(UInt16)t1.y);
					renderer->Render2DPoint((UInt16)t1.x+td,(UInt16)t1.y);
					renderer->Render2DPoint((UInt16)t1.x,(UInt16)t1.y+td);
					renderer->Render2DPoint((UInt16)t1.x,(UInt16)t1.y-td);	
				
				}

				 //Button3DList.buttons[i].dist);
				
			
			}
		renderer->SetColor ( pVColors[TheTimeOfDay.GetNVGmode() != 0][5] );
		}
								
	



#if 0
	/*
	** Do ALT indictator
	*/
	float alt = -((AircraftClass *)otwPlatform)->af->z;
	alt = fmod( alt, 1000.0f );
	alt = alt * 0.001f * 2.0 * PI;

	x1 = 0.0f;
	y1 = 0.0f;
   mlSinCos (&trig, alt);
	x2 = 0.85f * trig.cos;
	y2 = 0.85f * -trig.sin;
    vcInfo.vALTrenderer->Line( x1, y1, x2, y2 );

	/*
	** Do OIL indictator
	*/
	float oil = ((AircraftClass *)otwPlatform)->af->rpm;
	oil = oil  * 2.0 * PI;

	x1 = 0.0f;
	y1 = 0.0f;
   mlSinCos (&trig, oil);
	x2 = 0.85f * trig.cos;
	y2 = 0.85f * -trig.sin;
    vcInfo.vOILrenderer->Line( x1, y1, x2, y2 );

	/*
	** Do NOZ indictator
	*/
	float noz = ((AircraftClass *)otwPlatform)->af->rpm;
	noz = noz  * 2.0 * PI;

	x1 = 0.0f;
	y1 = 0.0f;
   mlSinCos (&trig, noz);
	x2 = 0.85f * trig.cos;
	y2 = 0.85f * -trig.sin;
    vcInfo.vNOZrenderer->Line( x1, y1, x2, y2 );

	/*
	** Do RPM indictator
	*/
	float rpm = ((AircraftClass *)otwPlatform)->af->rpm;
	rpm = rpm  * 2.0 * PI;

	x1 = 0.0f;
	y1 = 0.0f;
   mlSinCos (&trig, rpm);
	x2 = 0.85f * trig.cos;
	y2 = 0.85f * -trig.sin;
    vcInfo.vRPMrenderer->Line( x1, y1, x2, y2 );

	/*
	** Do FTIT indictator
	*/
	x1 = 0.0f;
	y1 = 0.0f;
	x2 = 0.0f;
	y2 = 0.85f;
    vcInfo.vFTITrenderer->Line( x1, y1, x2, y2 );
#endif

// 2001-01-31 ADDED BY S.G. SO HMS EQUIPPED PLANE HAS TWO GREEN CONCENTRIC CIRCLE IN PADLOCK VIEW
	VehicleClassDataType	*vc	= (VehicleClassDataType *)Falcon4ClassTable[otwPlatform->Type() - VU_LAST_ENTITY_TYPE].dataPtr;
	if (vc && vc->Flags & 0x20000000)
	{
		MissileClass* theMissile;
		theMissile = (MissileClass*)(SimDriver.playerEntity->Sms->curWeapon);

		// First, make sure we have a Aim9 in uncage mode selected...
		if (SimDriver.playerEntity->Sms->curWeaponType == wtAim9)
		{
			if (theMissile && theMissile->isCaged == 0)
			{
				theMissile->RunSeeker();

				if (!theMissile->targetPtr || vuxRealTime & 0x100 ) // JB 010712 Flash when we have a target locked up
				{
					float xDiff, left, right, top, bottom;

					renderer->GetViewport(&left, &top, &right, &bottom);
					renderer->SetColor (TheHud->GetHudColor());

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


	renderer->SetColor (TheHud->GetHudColor());

	// restore camera
	renderer->SetCamera( &cameraPos, &cameraRot );

	// Detach the appropriate ordinance
	if (g_b3dCockpit) // JB 010802
	{
		for (stationNum=1; stationNum < sms->NumHardpoints(); stationNum++)
		{
			if (sms->hardPoint[stationNum]->GetRack()) {
				child = sms->hardPoint[stationNum]->GetRack();
				vrCockpit->DetachChild( child, stationNum-1 );
			} else if (sms->hardPoint[stationNum]->weaponPointer && sms->hardPoint[stationNum]->weaponPointer->drawPointer) {
				child = (DrawableBSP*)(sms->hardPoint[stationNum]->weaponPointer->drawPointer);
				vrCockpit->DetachChild( child, stationNum-1 );
			}
		}
	}
}


/*
** CleanupVirtualCockpit
*/
void
OTWDriverClass::VCock_Cleanup( void )
{
		int i;

		for(i = 0; i < mpVDials.size(); i++) {
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
}

//Wombat778 10-10-2003 Load 3d buttons

bool
OTWDriverClass::Button3D_Init(int eCPVisType, TCHAR* eCPName, TCHAR* eCPNameNCTR)
{
    char strCPFile[MAX_PATH];
    static const TCHAR *buttonfile = "3dbuttons.dat";
	static const TCHAR *vcockfile = "3dckpit.dat";			//Wombat778 10-15-2003
    FILE*			Button3DDataFile;
	char templine[256];
	char tempfunction[256];
    
//    FindCockpit(pCPFile, (Vis_Types)eCPVisType, eCPName, eCPNameNCTR, strCPFile);

	//Wombat778 10-15-2003 Replaced the findcockpit call with a sequence that should mean that a button file only loads if it is in
	//the same folder as the 3d cockpit file.  This should solve the problem of an f-16 button file with another planes 3d pit.

	if(eCPVisType == MapVisId(VIS_F16C))
		sprintf(strCPFile, "%s%s", COCKPIT_DIR, buttonfile);
	else
	{
		sprintf(strCPFile, "%s%d\\%s", COCKPIT_DIR, MapVisId(eCPVisType), vcockfile);
		
		if(ResExistFile(strCPFile))
			sprintf(strCPFile, "%s%d\\%s", COCKPIT_DIR, MapVisId(eCPVisType), buttonfile);
		else
		{
			sprintf(strCPFile, "%s%s\\%s", COCKPIT_DIR, eCPName, vcockfile);			
			if(ResExistFile(strCPFile))
				sprintf(strCPFile, "%s%s\\%s", COCKPIT_DIR, eCPName, buttonfile);			
			else
			{
				sprintf(strCPFile, "%s%s\\%s", COCKPIT_DIR, eCPNameNCTR, vcockfile);				
				if(ResExistFile(strCPFile))
					sprintf(strCPFile, "%s%s\\%s", COCKPIT_DIR, eCPNameNCTR, buttonfile);
				else
				{
					// F16C fallback
					sprintf(strCPFile, "%s%s", COCKPIT_DIR, buttonfile);	
				}
			}
		}
	}
	
    Button3DDataFile = fopen(strCPFile, "r");
  
    F4Assert(Button3DDataFile);			//Error: Couldn't open file

	Button3DList.numbuttons=0;
	Button3DList.debugbutton=0;
	Button3DList.clicked=0;  //Wombat778 10-15-2003 removed clickx and clicky

	if (Button3DDataFile) {
		if (!feof(Button3DDataFile)) 
			fgets(templine, 256, Button3DDataFile);				//Just read a dummy line for comments etc..

		while (!feof(Button3DDataFile)) {
			fgets(templine, 256, Button3DDataFile);
			int matchedfields = sscanf(templine, "%s %f %f %f %f %d %d", tempfunction,				//Wombat778 11-08-2003
										   &Button3DList.buttons[Button3DList.numbuttons].loc.x,
										   &Button3DList.buttons[Button3DList.numbuttons].loc.y,
										   &Button3DList.buttons[Button3DList.numbuttons].loc.z,
										   &Button3DList.buttons[Button3DList.numbuttons].dist,
										   &Button3DList.buttons[Button3DList.numbuttons].sound,
										   &Button3DList.buttons[Button3DList.numbuttons].mousebutton);


			if (matchedfields == 6) 
				Button3DList.buttons[Button3DList.numbuttons].mousebutton=1;			//Wombat778 11-08-2003 Added so there will still be compatibility with old files. Default to left mouse button.

			if (matchedfields >= 6)		//Wombat778 11-08-2003 changed to allow compatibility with old files 11-7-2003 added mousebutton field to allow LMB/RMB usage.
			{			
				Button3DList.buttons[Button3DList.numbuttons].function = FindFunctionFromString (tempfunction);
				Button3DList.numbuttons++;
			}
		}
		fclose(Button3DDataFile);
		return true;
	}
	return false;
}



