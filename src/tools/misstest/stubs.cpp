#include "stdhdr.h"
#include "misslist.h"
#include "missile.h"
#include "acmi\src\include\acmirec.h"
#include "otwdrive.h"
#include "simdrive.h"
#include "sfx.h"
#include "terrain\tviewpnt.h"
#include "mvrdef.h"
#include "f4find.h"
extern "C" {
#include "codelib\resources\reslib\src\resmgr.h"
}
#include "classtbl.h"
#include "entity.h"

OTWDriverClass OTWDriver;
SimulationDriver SimDriver;
int GetClassID(uchar domain, uchar eclass, uchar type, uchar stype, uchar sp, uchar owner, uchar c6, uchar c7);

SimulationDriver::SimulationDriver (void)
{
   SimLibElapsedTime = vuxGameTime;
   SimLibFrameCount = 0;
   objectList = NULL;
   featureList = NULL;
   campUnitList = NULL;
   campObjList = NULL;
   playerEntity = NULL;
   doEvent = TRUE;
   doFile = FALSE;
   facList = NULL;
   tankerList = NULL;
//	airbaseList = NULL;
   atcList = NULL;
   avtrOn = TRUE;
   SimLibMajorFrameTime = 0.02F;
   SimLibMinorFrameTime = 0.02F;
   SimLibMinorFrameRate = 50.0F;
   SimLibMinorPerMajor = 1;
   motionOn = TRUE;

   // Prep database filtering
//   allSimList = NULL;
   objectList = NULL;
   campUnitList = NULL;
   campObjList = NULL;
}
  
SimulationDriver::~SimulationDriver (void)
{
}

void SimulationDriver::AddToObjectList (VuEntity* a)
{
}

void OTWDriverClass::AddSfxRequest (SfxClass* tmp)
{
}
OTWDriverClass::OTWDriverClass(void)
{
   memset (textMessage, 0, sizeof(textMessage));
   viewPoint = NULL;
   renderer = NULL;
//   rendererStretch = NULL;
//   OtwQuadPixel = FALSE;

   endFlightTimer = 0;
   actionCameraTimer = 0;
   actionCameraMode = FALSE;
   flybyTimer = 0;
   endFlightPointSet = FALSE;
   endFlightVec.x = 0.0f;
   endFlightVec.y = 0.0f;
   endFlightVec.z = -1.0f;
   showPos = FALSE;
   autoScale = FALSE;

   otwPlatform = NULL;
   otwTrackPlatform = NULL;
   lastotwPlatform = NULL;
   OTWImage = NULL;
   sfxRequestRoot = NULL;
   sfxActiveRoot = NULL;
   OTWWin = NULL;
//   objectCriticalSection = NULL;
   numThreats = 0;
   viewSwap = 0;
   tgtId = -1;
   renderer = NULL;
   viewPoint = NULL;
   objectScale = 1.0F;
   viewStep = 0;
   tgtStep = 0;
   getNewCameraPos = FALSE;
   eyeFly = FALSE;
   weatherCmd = 0;
   doWeather = FALSE;

//   if (objectCriticalSection == NULL)
//      objectCriticalSection = F4CreateCriticalSection();

   flyingEye = NULL;

   padlockPriority = PriorityNone;

   eyePan = 0.0F;
   eyeTilt = 0.0F;
   chaseAz = 0.0F;
   chaseEl = 0.0F;
   chaseRange = -500.0F;
   azDir = 0.0F;
   elDir = 0.0F;
   slewRate = 0.1F;
   todOffset = 0.0F;
   e1 = 1.0F;
   e2 = 0.0F;
   e3 = 0.0F;
   e4 = 0.0F;
   cameraPos.x = 0.0F;
   cameraPos.y = 0.0F;
   cameraPos.z = 0.0F;
   cameraRot = IMatrix;
   ownshipPos.x = focusPoint.x = 1480000.0F;
   ownshipPos.y = focusPoint.y = 1412000.0F;
   ownshipPos.z = focusPoint.z = -15000.0F;
   takeScreenShot = FALSE;

	// initialize ejection cam.
	ejectCam = 0;
	prevChase = 0;

    vcInfo.vHUDrenderer = NULL;
    vcInfo.vRWRrenderer = NULL;
    vcInfo.vMACHrenderer = NULL;
    vcInfo.vALTrenderer = NULL;
    vcInfo.vNOZrenderer = NULL;
    vcInfo.vOILrenderer = NULL;
    vcInfo.vRPMrenderer = NULL;
    vcInfo.vFTITrenderer = NULL;
}

OTWDriverClass::~OTWDriverClass(void)
{
   ClearSfxLists();

/*   if (objectCriticalSection)
   {
      F4DestroyCriticalSection (objectCriticalSection);
      objectCriticalSection = NULL;
   }*/
}

float OTWDriverClass::GetGroundLevel (float a, float b, Tpoint* c)
{
   return 0.0F;
}

void OTWDriverClass::ClearSfxLists(void)
{
}

void OTWDriverClass::CreateVisualObject(SimBaseClass* a, float b)
{
}

SfxClass::SfxClass(int a, int b, Tpoint* c, Tpoint* d, float e, float f)
{
}

SfxClass::SfxClass(float a, DrawableTrail* b)
{
}

void FalconSendMessage (VuMessage* a, int b)
{
}

int TViewPoint::GetGroundType (float a, float b)
{
   return 1;
}

VuMessage* VuxCreateMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target)
{
   return NULL;
}

unsigned char GetTeam (unsigned char a)
{
   return 0;
}

FILE* OpenCampFile (char *filename, char *ext, char *mode)
	{
	char	fullname[MAX_PATH],path[MAX_PATH];

	if (strcmp(ext,"cmp") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"obj") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"obd") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"uni") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"tea") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"wth") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"plt") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"mil") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"tri") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"evl") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"smd") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"sqd") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"ct") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"ini") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"ucd") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"ocd") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"fcd") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"vcd") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"wcd") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"wld") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"phd") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"pd") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"fed") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"rcd") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"swd") == 0)
		sprintf(path,FalconObjectDataDir);
	else
		sprintf(path,FalconCampaignSaveDirectory);

//	Outdated by resmgr:
//	if (!ResExistFile(filename))
//		ResAddPath(path, FALSE);
	sprintf(fullname,"%s\\%s.%s",path,filename,ext);
	return ResFOpen(fullname,mode);
	}

