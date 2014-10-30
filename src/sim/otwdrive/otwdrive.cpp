#include <time.h>
#include "otwdrive.h"
#include "Graphics/Include/Setup.h"
#include "Graphics/Include/TimeMgr.h"
#include "Graphics/Include/TOD.h"
#include "Graphics/Include/tviewpnt.h"
#include "Graphics/Include/RenderOW.h"
#include "Graphics/Include/canvas3d.h"
#include "Graphics/Include/drawsgmt.h"
#include "Graphics/Include/objlist.h"
#include "Graphics/Include/drawbsp.h"
#include "Graphics/Include/drawpnt.h"
#include "Graphics/Include/drawGuys.h"
#include "Graphics/Include/drawGrnd.h"
#include "Graphics/Include/RViewPnt.h"
#include "Graphics/Include/TerrTex.h" // JB 010616
#include "resource.h"
#include "stdhdr.h"
#include "Graphics/DXEngine/DXVBManager.h"

extern bool g_bUse_DX_Engine;


#include "ClassTbl.h"
#include "hud.h"
#include "simdrive.h"
#include "mfd.h"
#include "fsound.h"
#include "camp2sim.h"
#include "hardpnt.h"
#include "missile.h"
#include "bomb.h"
#include "f4error.h"
#include "simfiltr.h"
#include "falclib/include/f4find.h"
#include "falcmesg.h"
#include "MsgInc/SimDataToggle.h"
#include "cpmanager.h"
#include "ui/include/falcuser.h"
#include "sinput.h"
#include "dispcfg.h"
#include "Graphics/Include/Loader.h"
#include "ThreadMgr.h"
#include "playerop.h"
#include "SoundFX.h"
#include "sms.h"
#include "rwr.h"
#include "aircrft.h"
#include "airframe.h"
#include "sfx.h"
#include "object.h"
#include "fakerand.h"
#include "dispopts.h"
#include "Ground.h"
#include "flightData.h"
#include "popmenu.h"
#include "flight.h"
#include "lantirn.h"
#include "IVibeData.h"
#include "Weather.h"
#include "DrawParticleSys.h"

#include "SimIO.h" // Retro 9Jan2004

#include "radiosubtitle.h" // Retro 16Dec2003
#include "falcsnd/winampfrontend.h" // Retro 3Jan2004
#include "mouselook.h" // Retro 18Jan2004
extern bool g_bPilotEntertainment; // Retro 3Jan2004
extern bool g_bEnableTrackIR; // Cobra - Animated Pilot's head


extern "C" {
#include "codelib/resources/reslib/src/resmgr.h"
}

extern HINSTANCE hInst;
extern int F4FlyingEyeType;
extern HWND mainMenuWnd;
extern VU_ID gVmPlayVU_ID;
extern int narrowFOV;
//extern bool g_b3DClickableCockpit; //Wombat778 10-10-2003
extern char g_strForceCockpitDat[0x40]; //Wombat778 10-10-2003
//extern char g_strForcePadlockDat[0x40]; //Wombat778 10-10-2003
extern int g_nForceCockpitResolution; //Wombat778 4-02-04
int FindBestResolution(void); //Wombat778 4-03-04
extern float g_fHybridPitThreshold1; //Wombat778 11-18-04
extern float g_fHybridPitThreshold2; //Wombat778 11-18-04
extern int   g_nHybridPitModeDelay; //Wombat778 11-18-04

extern bool g_bEnableDisplacementCam; // Retro 25Dec2003

// when in an external camera position we need to tell the player bubble
// where we are.  So we create one of these thingy's
FalconEntity *gOtwCameraLocation = NULL;

int endAbort = 0;
unsigned long nextCampObjectHeightRefresh = 0;

// for debugging memory leakage while sim is running
#ifdef DEBUG
//#define CHECK_LEAKAGE
#endif

#ifdef CHECK_LEAKAGE
unsigned int leakChkPt = 24;
extern MEM_BOOL MEM_CALLBACK errPrint(MEM_ERROR_INFO *errorInfo);
extern MEM_ERROR_FN lastErrorFn;
#endif

extern bool g_bEnableMfdSize; // a.s. enables resizing of Mfds
extern float g_fMfd_p_Size;   // a.s.


// Padlock On Left
#define F3PADLOCK_TOP          1.0F
#define F3PADLOCK_LEFT        -0.6F
#define F3PADLOCK_BOTTOM      -1.0F
#define F3PADLOCK_RIGHT        1.0F
#define F3INSTRUMENT_TOP      -0.37F
#define F3INSTRUMENT_LEFT     -1.0F
#define F3INSTRUMENT_BOTTOM   -1.0F
#define F3INSTRUMENT_RIGHT     F3PADLOCK_LEFT + 0.01F
#define F3LOCATOR_TOP          1.0F
#define F3LOCATOR_LEFT         F3INSTRUMENT_LEFT
#define F3LOCATOR_BOTTOM       F3INSTRUMENT_TOP - 0.01F
#define F3LOCATOR_RIGHT        F3INSTRUMENT_RIGHT


DWORD p3DpitHilite; // Cobra - 3D pit high night lighting color
DWORD p3DpitLolite; // Cobra - 3D pit low night lighting color

// For HUD coloring
/*
   static DWORD HUDcolor[] = {
   0xff00ff00,
   0xff0000ff,
   0xffff0000,
   0xffffff00,
   0xffff00ff,
   0xff00ffff,
   0xff7f0000,
   0xff007f00,
   0xff00007f,
   0xff7f7f00,
   0xff007f7f,
   0xff7f007f,
   0xff7f7f7f,
   0xffffffff,
   0xff000000,
   0xff00bf00,
   };
   const DWORD* OTWDriverClass::hudColor = HUDcolor;
   const int OTWDriverClass::NumHudColors = sizeof(HUDcolor)/sizeof(HUDcolor[0]);
   */

// MFD Placement
int MfdSize = 154;
RECT VirtualMFD[OTWDriverClass::NumPopups + 1] =
{
    { 0,             480 - MfdSize, MfdSize - 1, 479},
    { 640 - MfdSize, 480 - MfdSize, 639,         479},
    { 640 - MfdSize, 0,             639,         MfdSize - 1},
    { 0,             0,             MfdSize - 1, MfdSize - 1},
    { 0,             0,             MfdSize - 1, MfdSize - 1}
};


OTWDriverClass OTWDriver;

DrawableBSP *endDialogObject;

HANDLE gSharedMemHandle;
void* gSharedMemPtr = NULL;
// JPO - for the new hardware
HANDLE gIntellivibeShared;
void *gSharedIntellivibe;
IntellivibeData g_intellivibeData;

OTWDriverClass::OTWDriverClass(void)
{
    memset(textMessage, 0, sizeof(textMessage));
    memset(textTimeLeft, 0, sizeof(textTimeLeft));
    showFrontText = 0;
    viewPoint = NULL;
    renderer = NULL;
    vrCockpit = NULL;
    endDialogObject = NULL;

    endFlightTimer = 0;
    actionCameraTimer = 0;
    actionCameraMode = FALSE;
    HybridPitModeEnabled = 0; //Wombat778 11-18-04

    flybyTimer = 0;
    endFlightPointSet = FALSE;
    endFlightVec.x = 0.0f;
    endFlightVec.y = 0.0f;
    endFlightVec.z = -1.0f;
    showPos = FALSE;
    showAero = FALSE;
    autoScale = FALSE;

    otwPlatform.reset(NULL);
    //otwTrackPlatform = NULL;
    lastotwPlatform.reset(NULL);
    OTWImage = NULL;
    litObjectRoot = NULL;
    nearObjectRoot = NULL;
    sfxRequestRoot = NULL;
    sfxActiveRoot = NULL;
    OTWWin = NULL;
    //   objectCriticalSection = NULL;
    numThreats = 0;
    viewSwap = 0;
    tgtId = -1;
    isActive = FALSE;
    isShutdown = TRUE;
    objectScale = 1.0F;
    viewStep = 0;
    tgtStep = 0;
    getNewCameraPos = FALSE;
    eyeFly = FALSE;
    weatherCmd = 0;
    doWeather = FALSE;
    nextCampObjectHeightRefresh = 0;
    mUseHeadTracking = FALSE;

    //   if (objectCriticalSection == NULL)
    //      objectCriticalSection = F4CreateCriticalSection("objectCriticalSection");

    flyingEye = NULL;

    padlockWindow[0][0] = F3PADLOCK_LEFT;
    padlockWindow[0][1] = F3PADLOCK_TOP;
    padlockWindow[0][2] = F3PADLOCK_RIGHT;
    padlockWindow[0][3] = F3PADLOCK_BOTTOM;
    padlockWindow[1][0] = F3INSTRUMENT_LEFT;
    padlockWindow[1][1] = F3INSTRUMENT_TOP;
    padlockWindow[1][2] = F3INSTRUMENT_RIGHT;
    padlockWindow[1][3] = F3INSTRUMENT_BOTTOM;
    padlockWindow[2][0] = F3LOCATOR_LEFT;
    padlockWindow[2][1] = F3LOCATOR_TOP;
    padlockWindow[2][2] = F3LOCATOR_RIGHT;
    padlockWindow[2][3] = F3LOCATOR_BOTTOM;
    padlockPriority = PriorityNone;
    mpPadlockPriorityObject = NULL;

    snapStatus = PRESNAP;
    PadlockOccludedTime = 0.0F;
    snapDir = 0.0F;
    stopState = STOP_STATE0;

    mpPadlockCandidate = NULL;
    mPadlockCandidateID  = FalconNullId;

    mObjectOccluded = TRUE;

    mTDTimeout = 0.0F;

    mIsSlewInit = FALSE;
    mSlewPStart = 0.0F;
    mSlewTStart = 0.0F;

    eyeHeadRoll = 0.0F;
    eyePan = 0.0F;
    eyeTilt = 0.0F;
    chaseAz = 0.0F;
    chaseEl = 0.0F;
    chaseRange = -500.0F;
    azDir = 0.0F;
    elDir = 0.0F;
    slewRate = 40.0f * DTR; // 40 deg per sec
    todOffset = 0.0f;
    mPadlockTimeout = 0.0F;
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
    otwResolution = FalconDisplayConfiguration::Sim;
    takeScreenShot = FALSE;

    pilotEyePos.x = 15;  // MLR 12/1/2003 - Pilots Eye position
    pilotEyePos.y =  0;
    pilotEyePos.z = -3;

    headMotion = YAW_PITCH;

    frameTime = 0;

    // initialize ejection cam.
    ejectCam = 0;
    prevChase = 0;

    // ASSO: old canvas
    vcInfo.vHUDrenderer = NULL;
    vcInfo.vRWRrenderer = NULL;
    vcInfo.vMACHrenderer = NULL;

    // ASSO: new canvas
    vHUDrenderer = NULL;
    vRWRrenderer = NULL;
    vDEDrenderer = NULL;
    vPFLrenderer = NULL;

    vBoresightY = 0.75f; // ASSO:

    pCockpitManager = NULL;
    pPadlockCPManager = NULL;
    pMenuManager = NULL;

    mDoSidebar = FALSE;

    PadlockF3_InitSidebar();

    pVColors[0][0] = 0xff7c4706; //padlock background
    pVColors[1][0] = CalculateNVGColor(pVColors[0][0]);

    pVColors[0][1] = 0xff5ee75e; //padlock liftline, green
    pVColors[1][1] = CalculateNVGColor(pVColors[0][1]);

    pVColors[0][2] = 0xff00ffff; //padlock viewport box 3 sides, yellow
    pVColors[1][2] = CalculateNVGColor(pVColors[0][2]);

    pVColors[0][3] = 0xffffffff; //padlock viewport top, white
    pVColors[1][3] = CalculateNVGColor(pVColors[0][3]);

    pVColors[0][4] = 0xff0000ff; //padlock zero level tickmark, red
    pVColors[1][4] = CalculateNVGColor(pVColors[0][4]);

    pVColors[0][5] = 0xff777777; //vcock primary needle color, gray
    pVColors[1][5] = CalculateNVGColor(pVColors[0][5]);

    pVColors[0][6] = 0xff1111aa; //vcock secondary needle color, redish
    pVColors[1][6] = CalculateNVGColor(pVColors[0][6]);

    pVColors[0][7] = 0xFF009CFF; // vcock DED color
    pVColors[1][7] = CalculateNVGColor(pVColors[0][7]);

    // Cobra

    p3DpitHilite = 0xFF0000FF; // Cobra - 3D pit high night lighting color
    p3DpitLolite = 0xFF000080; // Cobra - 3D pit low night lighting color


    vrCockpitModel[0] = VIS_VRCOCKPIT;
    vrCockpitModel[1] = VIS_VRCOCKPIT_MP;
    vrCockpitModel[2] = VIS_F16C;
    vrCockpitModel[3] = VIS_CF16A;

    //JAM 10May04
    bVCockZBuffering = FALSE;

    // Create Shared Memory object for data output
    gSharedMemHandle = CreateFileMapping((HANDLE)0xFFFFFFFF, NULL, PAGE_READWRITE,
                                         0, sizeof(FlightData), "FalconSharedMemoryArea");

    if (gSharedMemHandle)
    {
        gSharedMemPtr = MapViewOfFile(gSharedMemHandle, FILE_MAP_WRITE, 0, 0, 0);
    }
    else
    {
        MonoPrint("CreateFileMapping for shared data failed\n");
        CloseHandle(gSharedMemHandle);
    }

    // Create Shared Memory object for other output
    gIntellivibeShared = CreateFileMapping((HANDLE)0xFFFFFFFF, NULL, PAGE_READWRITE,
                                           0, sizeof(IntellivibeData), "FalconIntellivibeSharedMemoryArea");

    if (gIntellivibeShared)
    {
        gSharedIntellivibe = MapViewOfFile(gIntellivibeShared, FILE_MAP_WRITE, 0, 0, 0);
    }
    else
    {
        MonoPrint("CreateFileMapping for shared ivibe data failed\n");
        CloseHandle(gIntellivibeShared);
    }

    cs_update = F4CreateCriticalSection("cs_update"); // JB 010616
    bKeepClean = FALSE; // JB 010616
    liftlinecolor = (0xFF0096FF); // M.N. 011223
    simObjectPtr = NULL; // M.N. 020102 - probably caused the CalcRelGeom CTD

    ProfilerActive = false; // Retro 16/10/03
    DisplayProfiler = false; // Retro 16/10/03

    xDir = new CamDisplacement(1.F, 3.F, 1.F); // Retro 23Dec2003
    yDir = new CamDisplacement(-1.F, -3.F, 1.F); // Retro 23Dec2003
    zDir = new CamDisplacement(1.F, 3.F, 1.F); // Retro 23Dec2003

    displaceCamera = false; // Retro 25Dec2003

    // 1 meter per second ? not sure bout the distance units (again)
    cameraDisplacementRate = 1.0F; // Retro 24Dec2003 - this should be constant
    currentFPS = 0.0f;//Cobra

#if NEW_SERVER_VIEWPOINT
    vmMutex = F4CreateCriticalSection("viewpoint mutex");
#endif
}

OTWDriverClass::~OTWDriverClass(void)
{
#if NEW_SERVER_VIEWPOINT
    F4DestroyCriticalSection(vmMutex);
#endif

    // Close shared memory area
    if (gSharedMemPtr)
    {
        UnmapViewOfFile(gSharedMemPtr);
        gSharedMemPtr = NULL;
    }

    CloseHandle(gSharedMemHandle);

    if (gSharedIntellivibe)
    {
        UnmapViewOfFile(gSharedIntellivibe);
        gSharedIntellivibe = NULL;
    }

    CloseHandle(gIntellivibeShared);

    ClearSfxLists();

    if (cs_update)
    {
        F4DestroyCriticalSection(cs_update);
        cs_update = NULL;
    }

    //   if (objectCriticalSection)
    //   {
    //      F4DestroyCriticalSection (objectCriticalSection);
    //      objectCriticalSection = NULL;
    //   }

    // Retro 23Dec2003 start
    if (xDir)
    {
        delete(xDir);
        xDir = 0;
    }

    if (yDir)
    {
        delete(yDir);
        yDir = 0;
    }

    if (zDir)
    {
        delete(zDir);
        zDir = 0;
    }

    // Retro 23Dec2003 end
}


void OTWDriverClass::SetFOV(float horizontalFOV)
{
    renderer->SetFOV(horizontalFOV);
}


float OTWDriverClass::GetFOV(void)
{
    return renderer->GetFOV();
}


/*
 ** This function sets the otwTrackPlatform and VU ref's it.  If there
 ** is a current one, deref it.
 */
#if 0
// sfr: not needed anymore
void OTWDriverClass::SetTrackPlatform(SimBaseClass *obj)
{

    ShiAssert(IsActive());

    /*--------------------------------------------*/
    /* Set the local ownship ptr for this session */
    /*--------------------------------------------*/
    if (otwTrackPlatform)
    {
        VuDeReferenceEntity(otwTrackPlatform);
    }

    otwTrackPlatform = obj;

    if (otwTrackPlatform)
    {
        VuReferenceEntity(otwTrackPlatform);
    }
}
#endif

/*
 ** RunActionCamera looks for interesting things going on and sets the
 **  Camera Mode accordingly.   It essentially does this by prioritizing
 **  any weapons flying or objects firing.
 */
void OTWDriverClass::RunActionCamera(void)
{
    SimBaseClass* newObject = NULL;
    SimBaseClass* prevObject = NULL;
    SimBaseClass* theObject;
    SimBaseClass* firingObject = NULL;
    SimBaseClass* weaponObject = NULL;
    SimBaseClass* talkObject = NULL;
    BOOL foundCurrent = FALSE;
    SimObjectType *targetPtr;

    azDir = 0.5f;
    elDir = 0.5f;

    // if otwPlatform is NULL try finding a new one
    if (otwPlatform.get() == NULL)
    {
        viewStep = 1;
        FindNewOwnship();

        if (otwPlatform.get() == NULL)
        {
            return;
        }
    }

    // 1st off, check to see if either platform or trackplatform is a
    // weapon.  If so check to see if its exploded or not.  If it hasn't
    // just continue with current mode
    /*
     ** edg: this was lurking on missiles too long
     if ( otwPlatform and otwPlatform->IsWeapon() and not otwPlatform->IsEject() )
     {
     if ( not otwPlatform->IsDead() )
     {
    // check back in several secs
    actionCameraTimer = vuxRealTime + 8000;
    return;
    }
    }
    */


    // check to see if either platform is firing
    if ((otwPlatform.get() not_eq NULL) and otwPlatform->IsFiring())
    {
        // check back in several secs
        actionCameraTimer = vuxRealTime + 8000;
        return;
    }
    else if (otwTrackPlatform.get() and otwTrackPlatform->IsFiring())
    {
        // check back in several secs
        actionCameraTimer = vuxRealTime + 8000;
        return;
    }

    // walk the update list
    {
        VuListIterator updateWalker(SimDriver.objectList);
        theObject = (SimBaseClass*)updateWalker.GetFirst();

        while (theObject)
        {
            // we don't want to deal with campaign objects....
            if (
 not theObject->IsSim() or
 not theObject->IsAwake()
                /* or theObject->IsEject()*/
            ) // 2002-02-12 ADDED BY S.G. Make sure we are skipping ejected pilots REMOVED FOR NOW
            {
                // get next object in list
                theObject = (SimBaseClass*)updateWalker.GetNext();
                continue;
            }

            // is this an object( missile or bomb)
            if (
                (theObject->IsMissile() or
                 (
                     theObject->IsBomb() and 
 not (((BombClass*)theObject)->IsSetBombFlag(BombClass::IsFlare bitor BombClass::IsChaff)))
                ) and 
 not theObject->IsEject() and 
                (otwPlatform.get() not_eq weaponObject)
            )
            {
                // don't track missiles with no targets
                if ( not (theObject->IsMissile() and ((SimMoverClass *)theObject)->targetPtr == NULL))
                    weaponObject = theObject;
            }
            // is object firing?
            else if (theObject->IsFiring())
            {
                firingObject = theObject;
            }
            // is this object sending radio message?
            else if (theObject->Id() == gVmPlayVU_ID)
            {
                talkObject = theObject;
            }
            else if ( not (theObject->IsGroundVehicle() and ((SimMoverClass *)theObject)->targetPtr == NULL))
            {
                // is this the same as current?
                if (otwPlatform.get() == theObject)
                {
                    foundCurrent = TRUE;
                }
                // else, is this the first found prior to current?
                else if (prevObject == NULL and foundCurrent == FALSE)
                {
                    prevObject = theObject;
                }
                // else, if we've already found the current one, this
                // object must be the next one after current
                else if (foundCurrent == TRUE and newObject == NULL)
                {
                    newObject = theObject;
                }
            }

            // get next object in list
            theObject = (SimBaseClass*)updateWalker.GetNext();
        } // end while the object
    }

    // now we've either got in order of priority:  weaponObject, firingObject
    // newObject, prevObject.  Set a mode accordingly....
    if (talkObject)
    {
        gVmPlayVU_ID = vuNullId;

        // get target pointer
        targetPtr = ((SimMoverClass *)talkObject)->targetPtr;

        // no campaign objects
        if (targetPtr and not targetPtr->BaseData()->IsSim())
        {
            targetPtr = NULL;
        }

        // if the object's got a target, set it as the tracked
        // object, otherwise set track platform to NULL
        if (targetPtr)
        {
            // we can do either a weapon mode or incoming mode shot
            if ((int)(NRANDPOS * 3.0f))
            {
                SetTrackPlatform((SimBaseClass*)targetPtr->BaseData());
                SetGraphicsOwnship(talkObject);
                mOTWDisplayMode = ModeWeapon;
            }
            else
            {
                SetGraphicsOwnship((SimBaseClass*)targetPtr->BaseData());
                SetTrackPlatform(talkObject);
                mOTWDisplayMode = ModeIncoming;
            }
        }
        // no target just do a chase, orbit or satellite
        else
        {
            SetGraphicsOwnship(talkObject);
            SetTrackPlatform(NULL);

            switch ((int)(NRANDPOS * 10.0f))
            {
                case 0:
                    mOTWDisplayMode = ModeSatellite;
                    break;

                case 1:
                case 5:
                    mOTWDisplayMode = ModeOrbit;
                    break;

                case 2:
                case 3:
                case 4:
                    mOTWDisplayMode = ModeFlyby;
                    break;

                default:
                    mOTWDisplayMode = ModeChase;
                    break;
            }
        }

        if (otwPlatform->GetVt() < 30.0f and mOTWDisplayMode == ModeFlyby)
        {
            mOTWDisplayMode = ModeChase;
        }

        SelectExternal();
        actionCameraTimer = vuxRealTime + 8000;

    } // end if talk
    else if (weaponObject)
    {
        SimBaseClass *parent;

        // get target pointer
        targetPtr = ((SimMoverClass *)weaponObject)->targetPtr;
        parent = (SimBaseClass *)((SimWeaponClass *)weaponObject)->Parent();

        // no campaign objects
        if (targetPtr and not targetPtr->BaseData()->IsSim())
        {
            targetPtr = NULL;
        }

        // if the object's got a target, set it as the tracked
        // object, otherwise set track platform to NULL
        if (targetPtr)
        {
            // we can do either a weapon mode or incoming mode shot
            if ((int)(NRANDPOS * 3.0f))
            {
                SetTrackPlatform((SimBaseClass*)targetPtr->BaseData());
                SetGraphicsOwnship(weaponObject);
                mOTWDisplayMode = ModeWeapon;
            }
            else
            {
                SetGraphicsOwnship((SimBaseClass*)targetPtr->BaseData());
                SetTrackPlatform(weaponObject);
                mOTWDisplayMode = ModeIncoming;
            }
        }
        else if (parent)
        {
            if (parent->IsSim())
            {
                SetGraphicsOwnship(parent);
                SetTrackPlatform(weaponObject);
            }
            else
            {
                SetGraphicsOwnship(weaponObject);
                SetTrackPlatform(parent);
            }

            mOTWDisplayMode = ModeWeapon;
        }
        // no target just do a chase, orbit or satellite
        else
        {
            SetGraphicsOwnship(weaponObject);
            SetTrackPlatform(NULL);

            switch ((int)(NRANDPOS * 10.0f))
            {
                case 0:
                    mOTWDisplayMode = ModeSatellite;
                    break;

                case 1:
                case 5:
                    mOTWDisplayMode = ModeOrbit;
                    break;

                case 2:
                case 3:
                case 4:
                    mOTWDisplayMode = ModeFlyby;
                    break;

                default:
                    mOTWDisplayMode = ModeChase;
                    break;
            }
        }

        if (otwPlatform->GetVt() < 30.0f and mOTWDisplayMode == ModeFlyby)
        {
            mOTWDisplayMode = ModeChase;
        }

        SelectExternal();
        actionCameraTimer = vuxRealTime + 8000;

    } // end if weapon
    else if (firingObject)
    {
        // get target pointer
        targetPtr = ((SimMoverClass *)firingObject)->targetPtr;

        // no campaign objects
        if (targetPtr and not targetPtr->BaseData()->IsSim())
        {
            targetPtr = NULL;
        }

        // if the object's got a target, set it as the tracked
        // object, otherwise set track platform to NULL
        if (targetPtr)
        {
            // shpw view either from target or viceversa perspective
            if ((int)(NRANDPOS * 3.0f))
            {
                SetTrackPlatform((SimBaseClass*)targetPtr->BaseData());
                SetGraphicsOwnship(firingObject);
            }
            else
            {
                SetGraphicsOwnship((SimBaseClass*)targetPtr->BaseData());
                SetTrackPlatform(firingObject);
            }

            mOTWDisplayMode = ModeTarget;
        }
        // no target just do a chase, orbit or satellite
        else
        {
            SetGraphicsOwnship(firingObject);
            SetTrackPlatform(NULL);

            switch ((int)(NRANDPOS * 10.0f))
            {
                case 0:
                    mOTWDisplayMode = ModeSatellite;
                    break;

                case 1:
                    mOTWDisplayMode = ModeOrbit;
                    break;

                case 5:
                case 2:
                case 3:
                case 4:
                    mOTWDisplayMode = ModeFlyby;
                    break;

                default:
                    mOTWDisplayMode = ModeChase;
                    break;
            }
        }

        if (otwPlatform->GetVt() < 30.0f and mOTWDisplayMode == ModeFlyby)
        {
            mOTWDisplayMode = ModeChase;
        }

        SelectExternal();
        actionCameraTimer = vuxRealTime + 8000;

    } // end if firing object
    // just hang on current object
    else if ( not otwPlatform->IsDead() and (int)(NRANDPOS * 2.0f))
    {
        switch ((int)(NRANDPOS * 10.0f))
        {
            case 0:
                mOTWDisplayMode = ModeSatellite;
                break;

            case 1:
                mOTWDisplayMode = ModeOrbit;
                break;

            case 3:
            case 2:
            case 4:
            case 5:
                mOTWDisplayMode = ModeFlyby;
                break;

            default:
                mOTWDisplayMode = ModeChase;
                break;
        }

        SelectExternal();
        actionCameraTimer = vuxRealTime + 3000;
    }
    else if (newObject)
    {
        // get target pointer
        targetPtr = ((SimMoverClass *)newObject)->targetPtr;

        // no campaign objects
        if (targetPtr and not targetPtr->BaseData()->IsSim())
        {
            targetPtr = NULL;
        }

        // if the object's got a target, set it as the tracked
        // object, otherwise set track platform to NULL
        if (targetPtr and (int)(NRANDPOS * 3.0f))
        {
            // shpw view either from target or viceversa perspective
            if ((int)(NRANDPOS * 3.0f))
            {
                SetTrackPlatform((SimBaseClass*)targetPtr->BaseData());
                SetGraphicsOwnship(newObject);
            }
            else
            {
                SetGraphicsOwnship((SimBaseClass*)targetPtr->BaseData());
                SetTrackPlatform(newObject);
            }

            mOTWDisplayMode = ModeTarget;
        }
        // no target just do a chase, orbit or satellite
        else
        {
            SetGraphicsOwnship(newObject);
            SetTrackPlatform(NULL);

            switch ((int)(NRANDPOS * 10.0f))
            {
                case 0:
                    mOTWDisplayMode = ModeSatellite;
                    break;

                case 3:
                    mOTWDisplayMode = ModeOrbit;
                    break;

                case 1:
                case 2:
                case 4:
                case 5:
                    mOTWDisplayMode = ModeFlyby;
                    break;

                default:
                    mOTWDisplayMode = ModeChase;
                    break;
            }
        }

        if (otwPlatform->GetVt() < 30.0f and mOTWDisplayMode == ModeFlyby)
        {
            mOTWDisplayMode = ModeChase;
        }

        SelectExternal();
        actionCameraTimer = vuxRealTime + 8000;

    } // end if new object
    else if (prevObject)
    {
        // get target pointer
        targetPtr = ((SimMoverClass *)prevObject)->targetPtr;

        // no campaign objects
        if (targetPtr and not targetPtr->BaseData()->IsSim())
        {
            targetPtr = NULL;
        }

        // if the object's got a target, set it as the tracked
        // object, otherwise set track platform to NULL
        if (targetPtr and (int)(NRANDPOS * 8.0f))
        {
            // shpw view either from target or viceversa perspective
            if ((int)(NRANDPOS * 8.0f))
            {
                SetTrackPlatform((SimBaseClass*)targetPtr->BaseData());
                SetGraphicsOwnship(prevObject);
            }
            else
            {
                SetGraphicsOwnship((SimBaseClass*)targetPtr->BaseData());
                SetTrackPlatform(prevObject);
            }

            mOTWDisplayMode = ModeTarget;
        }
        // no target just do a chase, orbit or satellite
        else
        {
            SetGraphicsOwnship(prevObject);
            SetTrackPlatform(NULL);

            switch ((int)(NRANDPOS * 10.0f))
            {
                case 0:
                    mOTWDisplayMode = ModeSatellite;
                    break;

                case 3:
                    mOTWDisplayMode = ModeOrbit;
                    break;

                case 1:
                case 2:
                case 4:
                case 5:
                    mOTWDisplayMode = ModeFlyby;
                    break;

                default:
                    mOTWDisplayMode = ModeChase;
                    break;
            }
        }

        if (otwPlatform->GetVt() < 30.0f and mOTWDisplayMode == ModeFlyby)
        {
            mOTWDisplayMode = ModeChase;
        }

        SelectExternal();
        actionCameraTimer = vuxRealTime + 8000;

    } // end if new object

    // don't use orbit cam on ground
    if ((otwPlatform.get() not_eq NULL) and otwPlatform->OnGround() and mOTWDisplayMode == ModeOrbit)
    {
        mOTWDisplayMode = ModeChase;
        SelectExternal();
    }

    if (mOTWDisplayMode == ModeFlyby)
        actionCameraTimer += 6000;

    if (mOTWDisplayMode == ModeChase)
        actionCameraTimer += 6000;


    // randomly adjust the FOV
    if (mOTWDisplayMode == ModeChase or
        mOTWDisplayMode == ModeFlyby or
        mOTWDisplayMode == ModeSatellite)
    {
        // no telefoto when object on ground
        if (otwPlatform->OnGround())
        {
            if (GetFOV() < 60.0F * DTR)
            {
                narrowFOV = FALSE;
                SetFOV(60.0F * DTR);
            }
        }
        else if (rand() bitand 0x00000004)
        {
            if (GetFOV() > 30.0F * DTR)
            {
                narrowFOV = TRUE;
                SetFOV(20.0F * DTR);
            }
            else
            {
                narrowFOV = FALSE;
                SetFOV(60.0F * DTR);
            }
        }
    }
    else
    {
        if (GetFOV() < 60.0F * DTR)
        {
            narrowFOV = FALSE;
            SetFOV(60.0F * DTR);
        }
    }

}

//Wombat778 11-18-04 Run the hybrid 2D/3D pit

void OTWDriverClass::RunHybridPitMode(float pan, float tilt)
{
    static float lastpan = pan;
    static float lasttilt = tilt;
    static long  lasttime = vuxRealTime;

    if (mOTWDisplayMode == Mode2DCockpit)
    {
        if (fabs(pan - lastpan) > g_fHybridPitThreshold1 * DTR or fabs(tilt - lasttilt) > g_fHybridPitThreshold1 * DTR)
        {
            SetOTWDisplayMode(Mode3DCockpit);

            //Wombat778 12-03-04 Reset Hybrid Pit mode
            if ( not GetHybridPitMode())
                ToggleHybridPitMode();

            lastpan = pan;
            lasttilt = tilt;
            lasttime = vuxRealTime;
        }
    }

    if (mOTWDisplayMode == Mode3DCockpit)
    {
        if (fabs(pan - lastpan) > g_fHybridPitThreshold2 * DTR or fabs(tilt - lasttilt) > g_fHybridPitThreshold2 * DTR)
        {
            lastpan = pan;
            lasttilt = tilt;
            lasttime = vuxRealTime;
        }
        else if (abs((long)vuxRealTime - lasttime) > g_nHybridPitModeDelay)
        {
            SetOTWDisplayMode(Mode2DCockpit);

            //Wombat778 12-03-04 Reset Hybrid Pit mode
            if ( not GetHybridPitMode())
                ToggleHybridPitMode();
        }


    }
}



/*
 ** Walk thru the objectList and find an object that meets the criteria
 ** of the view mode, focus and current objects
 **
 ** SCR 9/22/98 -- This function makes some pretty scary assumptions about
 ** what focusObj, and currObj are going to be.  Between this function
 ** and the stuff that manages and uses otwPlatform and otwTrackPlatform
 ** we have some pretty hairy restrictions on what can or can't be a
 ** campaign (or even non-aircraftClass) thing.
 ** I think the way to fix this would be to make otwPlatform and otwTrackPlatform
 ** be of type FalconEntity.  This would require many touch up fixes to the avionics
 ** and HUD interface to ensure that nothing other than the player airplane is used as
 ** "ownship".  Of course this should be done anyway.
 */
SimBaseClass *OTWDriverClass::FindNextViewObject(
    FalconEntity *focusObj, SimBaseClass *currObj, ViewFindMode vMode
)
{
    SimBaseClass* newObject = NULL;
    SimBaseClass* theObject;
    BOOL foundCurrent = FALSE;
    SimObjectType *targetPtr;

    // not sure yet, but I think focusObj should be non-NULL
    if (focusObj == NULL)
    {
        tgtStep = 0;
        return NULL;
    }

    VuListIterator updateWalker(SimDriver.objectList);

    // step in fwd direction
    if (tgtStep >= 0)
    {
        // this function is structures such as a big switch statement by
        // vMode.  Within each case stmt, we loop thru the object list looking
        // for an object that meets the criteria for that mode.
        switch (vMode)
        {
                // look for flying weapons belonging to focusObj
            case NEXT_WEAPON:
                // walk the update list
                theObject = (SimBaseClass*)updateWalker.GetFirst();

                while (theObject)
                {
                    // is this an object( missile or bomb) owned by focus obj?
                    if (theObject->IsSim() and theObject->IsWeapon() and 
                        ((SimWeaponClass*)theObject)->Parent() == focusObj)
                    {
                        // 2002-02-15 ADDED BY S.G. Don't toggle to chaff and flares (NOTE THE '!' AT THE FRONT TO REVERSE THE CONDITION)
                        if ( not (theObject->IsBomb() and (((BombClass *)theObject)->IsSetBombFlag(BombClass::IsChaff) or ((BombClass *)theObject)->IsSetBombFlag(BombClass::IsFlare))))
                        {
                            // END OF ADDED SECTION 2002-02-15
                            // is this the same as current?
                            if (theObject == currObj)
                            {
                                foundCurrent = TRUE;
                            }
                            // else, is this the first found prior to current?
                            else if (newObject == NULL and foundCurrent == FALSE)
                            {
                                newObject = theObject;
                            }
                            // else, if we've already found the current one, this
                            // object must be the next one after current, we're done
                            else if (foundCurrent == TRUE)
                            {
                                newObject = theObject;
                                break;
                            }
                        }
                    }

                    // get next object in list
                    theObject = (SimBaseClass*)updateWalker.GetNext();
                } // end while the object

                // did we find a match?
                if (newObject)
                {
                    // get target pointer
                    targetPtr = ((SimMoverClass *)newObject)->targetPtr;

                    // if the object's got a target, set it as the tracked
                    // object, otherwise set track platform to NULL
                    if (targetPtr)
                        SetTrackPlatform((SimBaseClass*)targetPtr->BaseData());
                    else
                        SetTrackPlatform(NULL);
                }

                break;

                // look for friendly aircraft
            case NEXT_AIR_FRIEND:
                // walk the update list
                theObject = (SimBaseClass*)updateWalker.GetFirst();

                while (theObject)
                {
                    // is this an object( missile or bomb) owned by focus obj?
                    if ((theObject->IsAirplane() or theObject->IsHelicopter()) and 
                        theObject not_eq focusObj and 
                        theObject->GetTeam() == focusObj->GetTeam())
                    {
                        // is this the same as current?
                        if (theObject == currObj)
                        {
                            foundCurrent = TRUE;
                        }
                        // else, is this the first found prior to current?
                        else if (newObject == NULL and foundCurrent == FALSE)
                        {
                            newObject = theObject;
                        }
                        // else, if we've already found the current one, this
                        // object must be the next one after current, we're done
                        else if (foundCurrent == TRUE)
                        {
                            newObject = theObject;
                            break;
                        }
                    }

                    // get next object in list
                    theObject = (SimBaseClass*)updateWalker.GetNext();
                } // end while the object


                // did we find a match?
                if (newObject)
                {
                    ShiAssert(focusObj->IsSim());
                    SetTrackPlatform((SimBaseClass*)focusObj);
                }

                break;

                // look for enemy aircraft
            case NEXT_AIR_ENEMY:

                // walk the update list
                theObject = (SimBaseClass*)updateWalker.GetFirst();

                while (theObject)
                {
                    // is this an object( missile or bomb) owned by focus obj?
                    if ((theObject->IsAirplane() or theObject->IsHelicopter()) and 
                        theObject->GetTeam() not_eq focusObj->GetTeam())
                    {
                        // is this the same as current?
                        if (theObject == currObj)
                        {
                            foundCurrent = TRUE;
                        }
                        // else, is this the first found prior to current?
                        else if (newObject == NULL and foundCurrent == FALSE)
                        {
                            newObject = theObject;
                        }
                        // else, if we've already found the current one, this
                        // object must be the next one after current, we're done
                        else if (foundCurrent == TRUE)
                        {
                            newObject = theObject;
                            break;
                        }
                    }

                    // get next object in list
                    theObject = (SimBaseClass*)updateWalker.GetNext();
                } // end while the object


                // did we find a match?
                if (newObject)
                {
                    ShiAssert(focusObj->IsSim());
                    SetTrackPlatform((SimBaseClass*)focusObj);
                }

                break;

                // look for friendly ground vehicle
            case NEXT_GROUND_FRIEND:

                // walk the update list
                theObject = (SimBaseClass*)updateWalker.GetFirst();

                while (theObject)
                {
                    // is this an object( missile or bomb) owned by focus obj?
                    if (theObject->IsGroundVehicle() and 
                        theObject->GetTeam() == focusObj->GetTeam())
                    {
                        // is this the same as current?
                        if (theObject == currObj)
                        {
                            foundCurrent = TRUE;
                        }
                        // else, is this the first found prior to current?
                        else if (newObject == NULL and foundCurrent == FALSE)
                        {
                            newObject = theObject;
                        }
                        // else, if we've already found the current one, this
                        // object must be the next one after current, we're done
                        else if (foundCurrent == TRUE)
                        {
                            newObject = theObject;
                            break;
                        }
                    }

                    // get next object in list
                    theObject = (SimBaseClass*)updateWalker.GetNext();
                } // end while the object


                // did we find a match?
                if (newObject)
                {
                    ShiAssert(focusObj->IsSim());
                    SetTrackPlatform((SimBaseClass*)focusObj);
                }

                break;

                // look for enemy ground
                // edg note: this now looks for ANY enemy
                // s.g. note: not anymore since I added the NEXT_ENEMY mode.
            case NEXT_GROUND_ENEMY:

                // walk the update list
                theObject = (SimBaseClass*)updateWalker.GetFirst();

                while (theObject)
                {
                    // is this an object( missile or bomb) owned by focus obj?
                    if (theObject->IsGroundVehicle() and 
                        theObject->GetTeam() not_eq focusObj->GetTeam())
                        // if ( theObject->GetTeam() not_eq focusObj->GetTeam() ) // 2002-02-16 MODIFIED BY S.G. NEXT_ENEMY handles now both air and ground so fixate this one on ground only...
                    {
                        // is this the same as current?
                        if (theObject == currObj)
                        {
                            foundCurrent = TRUE;
                        }
                        // else, is this the first found prior to current?
                        else if (newObject == NULL and foundCurrent == FALSE)
                        {
                            newObject = theObject;
                        }
                        // else, if we've already found the current one, this
                        // object must be the next one after current, we're done
                        else if (foundCurrent == TRUE)
                        {
                            newObject = theObject;
                            break;
                        }
                    }

                    // get next object in list
                    theObject = (SimBaseClass*)updateWalker.GetNext();
                } // end while the object


                // did we find a match?
                if (newObject)
                {
                    ShiAssert(focusObj->IsSim());
                    SetTrackPlatform((SimBaseClass*)focusObj);
                }

                break;

                // 2002-02-16 ADDED BY S.G. look for any enemy
            case NEXT_ENEMY:

                // walk the update list
                theObject = (SimBaseClass*)updateWalker.GetFirst();

                while (theObject)
                {
                    // is this an object( missile or bomb) owned by focus obj?
                    if (theObject->GetTeam() not_eq focusObj->GetTeam())
                    {
                        // 2002-02-25 ADDED BY S.G. Don't toggle to chaff and flares (NOTE THE '!' AT THE FRONT TO REVERSE THE CONDITION)
                        if ( not (theObject->IsBomb() and (((BombClass *)theObject)->IsSetBombFlag(BombClass::IsChaff) or ((BombClass *)theObject)->IsSetBombFlag(BombClass::IsFlare))))
                        {
                            // END OF ADDED SECTION 2002-02-25
                            // is this the same as current?
                            if (theObject == currObj)
                            {
                                foundCurrent = TRUE;
                            }
                            // else, is this the first found prior to current?
                            else if (newObject == NULL and foundCurrent == FALSE)
                            {
                                newObject = theObject;
                            }
                            // else, if we've already found the current one, this
                            // object must be the next one after current, we're done
                            else if (foundCurrent == TRUE)
                            {
                                newObject = theObject;
                                break;
                            }
                        }
                    }

                    // get next object in list
                    theObject = (SimBaseClass*)updateWalker.GetNext();
                } // end while the object


                // did we find a match?
                if (newObject)
                {
                    ShiAssert(focusObj->IsSim());
                    SetTrackPlatform((SimBaseClass*)focusObj);
                }

                break;

                // look for flying weapons that have us pinned
            case NEXT_INCOMING:

                // walk the update list
                theObject = (SimBaseClass*)updateWalker.GetFirst();

                while (theObject)
                {
                    // is this an object( missile or bomb) owned by focus obj?
                    if (theObject->IsSim() and theObject->IsMissile() and 
                        ((SimMoverClass *)theObject)->targetPtr and 
                        ((SimMoverClass *)theObject)->targetPtr->BaseData() == focusObj)
                    {
                        // is this the same as current?
                        if (theObject == currObj)
                        {
                            foundCurrent = TRUE;
                        }
                        // else, is this the first found prior to current?
                        else if (newObject == NULL and foundCurrent == FALSE)
                        {
                            newObject = theObject;
                        }
                        // else, if we've already found the current one, this
                        // object must be the next one after current, we're done
                        else if (foundCurrent == TRUE)
                        {
                            newObject = theObject;
                            break;
                        }
                    }

                    // get next object in list
                    theObject = (SimBaseClass*)updateWalker.GetNext();
                } // end while the object


                // did we find a match?
                if (newObject)
                {
                    // get target pointer
                    targetPtr = ((SimMoverClass *)newObject)->targetPtr;

                    // if the object's got a target, set it as the tracked
                    // object, otherwise set track platform to NULL
                    if (targetPtr)
                        SetTrackPlatform((SimBaseClass*)targetPtr->BaseData());
                    else
                        SetTrackPlatform(NULL);
                }

                break;


                // probably should assert here.
            default:
                break;
        }
    }
    else // step in backwards direction
    {
        // this function is structures such as a big switch statement by
        // vMode.  Within each case stmt, we loop thru the object list looking
        // for an object that meets the criteria for that mode.
        switch (vMode)
        {
                // look for flying weapons belonging to focusObj
            case NEXT_WEAPON:

                // walk the update list
                theObject = (SimBaseClass*)updateWalker.GetFirst();

                while (theObject)
                {
                    // is this an object( missile or bomb) owned by focus obj?
                    if (theObject->IsSim() and theObject->IsWeapon() and 
                        ((SimWeaponClass*)theObject)->Parent() == focusObj)
                    {
                        // 2002-02-15 ADDED BY S.G. Don't toggle to chaff and flares (NOTE THE '!' AT THE FRONT TO REVERSE THE CONDITION)
                        if ( not (theObject->IsBomb() and (((BombClass *)theObject)->IsSetBombFlag(BombClass::IsFlare bitor BombClass::IsChaff))))
                        {
                            // END OF ADDED SECTION 2002-02-15
                            // is this the same as current?
                            // if we've got a newObject we're done
                            if (theObject == currObj and newObject)
                            {
                                break;
                            }
                            // else assign new object -- we always want the last
                            // one found either prior to current object or after
                            // current object
                            else
                            {
                                newObject = theObject;
                            }
                        }
                    }

                    // get next object in list
                    theObject = (SimBaseClass*)updateWalker.GetNext();
                } // end while the object


                // did we find a match?
                if (newObject)
                {
                    // get target pointer
                    targetPtr = ((SimMoverClass *)newObject)->targetPtr;

                    // if the object's got a target, set it as the tracked
                    // object, otherwise set track platform to NULL
                    if (targetPtr)
                        SetTrackPlatform((SimBaseClass*)targetPtr->BaseData());
                    else
                        SetTrackPlatform(NULL);
                }

                break;

                // look for friendly aircraft
            case NEXT_AIR_FRIEND:

                // walk the update list
                theObject = (SimBaseClass*)updateWalker.GetFirst();

                while (theObject)
                {
                    // is this an object( missile or bomb) owned by focus obj?
                    if ((theObject->IsAirplane() or theObject->IsHelicopter()) and 
                        theObject not_eq focusObj and 
                        // not theObject->IsEject() and // 2002-02-12 ADDED BY S.G. Make sure we are skipping ejected pilots REMOVED FOR NOW
                        theObject->GetTeam() == focusObj->GetTeam())
                    {
                        // is this the same as current?
                        // if we've got a newObject we're done
                        if (theObject == currObj and newObject)
                        {
                            break;
                        }
                        // else assign new object -- we always want the last
                        // one found either prior to current object or after
                        // current object
                        else
                        {
                            newObject = theObject;
                        }
                    }

                    // get next object in list
                    theObject = (SimBaseClass*)updateWalker.GetNext();
                } // end while the object


                // did we find a match?
                if (newObject)
                {
                    ShiAssert(focusObj->IsSim());
                    SetTrackPlatform((SimBaseClass*)focusObj);
                }

                break;

                // look for enemy aircraft
            case NEXT_AIR_ENEMY:

                // walk the update list
                theObject = (SimBaseClass*)updateWalker.GetFirst();

                while (theObject)
                {
                    // is this an object( missile or bomb) owned by focus obj?
                    if ((theObject->IsAirplane() or theObject->IsHelicopter()) and 
 not theObject->IsEject() and // 2002-02-12 ADDED BY S.G. Make sure we are skipping ejected pilots
                        theObject->GetTeam() not_eq focusObj->GetTeam())
                    {
                        // is this the same as current?
                        // if we've got a newObject we're done
                        if (theObject == currObj and newObject)
                        {
                            break;
                        }
                        // else assign new object -- we always want the last
                        // one found either prior to current object or after
                        // current object
                        else
                        {
                            newObject = theObject;
                        }
                    }

                    // get next object in list
                    theObject = (SimBaseClass*)updateWalker.GetNext();
                } // end while the object


                // did we find a match?
                if (newObject)
                {
                    ShiAssert(focusObj->IsSim());
                    SetTrackPlatform((SimBaseClass*)focusObj);
                }

                break;

                // look for friendly ground vehicle
            case NEXT_GROUND_FRIEND:

                // walk the update list
                theObject = (SimBaseClass*)updateWalker.GetFirst();

                while (theObject)
                {
                    // is this an object( missile or bomb) owned by focus obj?
                    if (theObject->IsGroundVehicle() and 
                        theObject->GetTeam() == focusObj->GetTeam())
                    {
                        // is this the same as current?
                        // if we've got a newObject we're done
                        if (theObject == currObj and newObject)
                        {
                            break;
                        }
                        // else assign new object -- we always want the last
                        // one found either prior to current object or after
                        // current object
                        else
                        {
                            newObject = theObject;
                        }
                    }

                    // get next object in list
                    theObject = (SimBaseClass*)updateWalker.GetNext();
                } // end while the object


                // did we find a match?
                if (newObject)
                {
                    ShiAssert(focusObj->IsSim());
                    SetTrackPlatform((SimBaseClass*)focusObj);
                }

                break;

                // look for enemy ground
            case NEXT_GROUND_ENEMY:

                // walk the update list
                theObject = (SimBaseClass*)updateWalker.GetFirst();

                while (theObject)
                {
                    // is this an object( missile or bomb) owned by focus obj?
                    if (theObject->IsGroundVehicle() and 
                        theObject->GetTeam() not_eq focusObj->GetTeam())
                    {
                        // is this the same as current?
                        // if we've got a newObject we're done
                        if (theObject == currObj and newObject)
                        {
                            break;
                        }
                        // else assign new object -- we always want the last
                        // one found either prior to current object or after
                        // current object
                        else
                        {
                            newObject = theObject;
                        }
                    }

                    // get next object in list
                    theObject = (SimBaseClass*)updateWalker.GetNext();
                } // end while the object


                // did we find a match?
                if (newObject)
                {
                    ShiAssert(focusObj->IsSim());
                    SetTrackPlatform((SimBaseClass*)focusObj);
                }

                break;

                // 2002-02-16 ADDED BY S.G. look for any enemy
            case NEXT_ENEMY:

                // walk the update list
                theObject = (SimBaseClass*)updateWalker.GetFirst();

                while (theObject)
                {
                    // is this an object( missile or bomb) owned by focus obj?
                    if (theObject->GetTeam() not_eq focusObj->GetTeam())
                    {
                        // is this the same as current?
                        // if we've got a newObject we're done
                        // 2002-02-25 ADDED BY S.G. Don't toggle to chaff and flares (NOTE THE '!' AT THE FRONT TO REVERSE THE CONDITION)
                        if ( not (theObject->IsBomb() and (((BombClass *)theObject)->IsSetBombFlag(BombClass::IsChaff) or ((BombClass *)theObject)->IsSetBombFlag(BombClass::IsFlare))))
                        {
                            // END OF ADDED SECTION 2002-02-25
                            if (theObject == currObj and newObject)
                            {
                                break;
                            }
                            // else assign new object -- we always want the last
                            // one found either prior to current object or after
                            // current object
                            else
                            {
                                newObject = theObject;
                            }
                        }
                    }

                    // get next object in list
                    theObject = (SimBaseClass*)updateWalker.GetNext();
                } // end while the object


                // did we find a match?
                if (newObject)
                {
                    ShiAssert(focusObj->IsSim());
                    SetTrackPlatform((SimBaseClass*)focusObj);
                }

                break;

                // look for flying weapons that have us pinned
            case NEXT_INCOMING:

                // walk the update list
                theObject = (SimBaseClass*)updateWalker.GetFirst();

                while (theObject)
                {
                    // is this an object( missile or bomb) owned by focus obj?
                    if (theObject->IsSim() and 
                        theObject->IsMissile() and 
                        ((SimMoverClass *)theObject)->targetPtr and 
                        ((SimMoverClass *)theObject)->targetPtr->BaseData() == focusObj)
                    {
                        // is this the same as current?
                        // if we've got a newObject we're done
                        if (theObject == currObj and newObject)
                        {
                            break;
                        }
                        // else assign new object -- we always want the last
                        // one found either prior to current object or after
                        // current object
                        else
                        {
                            newObject = theObject;
                        }
                    }

                    // get next object in list
                    theObject = (SimBaseClass*)updateWalker.GetNext();
                } // end while the object


                // did we find a match?
                if (newObject)
                {
                    // get target pointer
                    targetPtr = ((SimMoverClass *)newObject)->targetPtr;

                    // if the object's got a target, set it as the tracked
                    // object, otherwise set track platform to NULL
                    if (targetPtr)
                        SetTrackPlatform((SimBaseClass*)targetPtr->BaseData());
                    else
                        SetTrackPlatform(NULL);
                }

                break;


                // probably should assert here.
            default:
                break;
        }
    }

    // reset target stepping
    tgtStep = 0;

    // return the newObject if any
    return newObject;
}


/*--------------------------------------------*/
/* Set the local ownship ptr for this session */
/*--------------------------------------------*/
void OTWDriverClass::SetGraphicsOwnship(SimBaseClass* obj)
{
    int i;
    AircraftClass* avionicsObj;
    SimBaseClass* curPlatform = otwPlatform.get();

    // sanity check
    if (obj and not obj->IsSim())
    {
        return;
    }

    if (otwPlatform.get() == obj)
    {
        return;
    }

    ShiAssert( not IsShutdown());

    // check for bad position of object and reject -- a bandaid
    if (obj)
    {
        if (
            _isnan(obj->XPos()) or not _finite(obj->XPos()) or
            _isnan(obj->YPos()) or not _finite(obj->YPos()) or
            _isnan(obj->ZPos()) or not _finite(obj->ZPos())
        )
        {
            return;
        }
    }


    // sfr added fine interest stuff
    // Let go of the old one
    if (otwPlatform.get() not_eq NULL)
    {
        // Make sure we don't leave it hidden (could have happened in UpdateVehicleDrawables)
        if (otwPlatform->drawPointer)
        {
            otwPlatform->drawPointer->SetInhibitFlag(FALSE);
        }

#if FINE_INT
        FalconLocalSession->RemoveFromFineInterest(otwPlatform.get(), false);
#endif
        // sfr: no need anymore cause of smartpointers
        //VuDeReferenceEntity (otwPlatform);
    }

    otwPlatform.reset(obj);

    // sfr added fine interest stuff
    // Establish the new one
    if (otwPlatform.get() not_eq NULL)
    {
        // Make sure there is a cockpit up
        if ( not curPlatform)
        {
            SetOTWDisplayMode(Mode2DCockpit);
        }

#if FINE_INT

        // add fine interest if platform is not ourselves
        if (otwPlatform.get() not_eq FalconLocalSession->GetPlayerEntity())
        {
            FalconLocalSession->AddToFineInterest(otwPlatform.get(), false);
        }

#endif
        // sfr: no need anymore, smartpointer
        //VuReferenceEntity (otwPlatform);

        //if (otwPlatform->IsLocal() and otwPlatform->IsSetFlag(MOTION_OWNSHIP) and otwPlatform->IsAirplane())
        if ((otwPlatform.get() == SimDriver.GetPlayerAircraft()) and otwPlatform->IsAirplane())
        {
            avionicsObj = static_cast<AircraftClass*>(otwPlatform.get());
            // MLR 12/1/2003 - Exports the old EyeFromCG stuff
            pilotEyePos = *(avionicsObj->af->GetPilotEyePos());
        }
        else
        {
            avionicsObj = NULL;
        }
    }
    else
    {
        SetOTWDisplayMode(ModeNone);
        avionicsObj = NULL;
    }

    // Tell everybody else about the change
    if (TheHud)
    {
        TheHud->SetOwnship(avionicsObj);
    }

    for (i = 0; i < NUM_MFDS; i++)
    {
        if (MfdDisplay[i])
        {
            MfdDisplay[i]->SetOwnship(avionicsObj);
        }
    }

    if (pCockpitManager)
    {
        pCockpitManager->SetOwnship(avionicsObj);
    }

    if (pPadlockCPManager)
    {
        pPadlockCPManager->SetOwnship(avionicsObj);
        pPadlockCPManager->SetActivePanel(PADLOCK_PANEL);
    }
}

#if not NEW_SERVER_VIEWPOINT
void OTWDriverClass::ServerSetviewPoint(void)
{
    viewPoint->Update(&ownshipPos);
}
#endif


void OTWDriverClass::Enter(void)
{
    Tpoint viewPos;
    Trotation viewRotation;
    int i;
    F4SoundEntering3d(); // MLR 12/13/2003 - Just inits some data for the sound code

    gOtwCameraLocation = new SpotEntity(F4FlyingEyeType + VU_LAST_ENTITY_TYPE);
    vuDatabase->/*Quick*/Insert(gOtwCameraLocation);

    // try shrinking the default pool for better memory compactness
#ifdef USE_SH_POOLS
#ifdef DEBUG

    if (MemDefaultPool)
    {
        // MonoPrint( "Shrinking Default Memory Pool by %d Bytes\n",
        MemPoolShrink(MemDefaultPool)/* )*/;
    }

#else

    if (MemDefaultPool)
        MemPoolShrink(MemDefaultPool);

#endif
#endif

    OTWDisplayMode startMode = Mode2DCockpit;

    SetShutdown(0);
    // FRB - ALERT
    // sfr: we are changing gfx context, flush all
    TheLoader.WaitLoader();

    OTWWin = FalconDisplay.appWin;
    SetResolution(FalconDisplayConfiguration::Sim);

    FalconDisplay.EnterMode(
        FalconDisplayConfiguration::Sim,
        DisplayOptions.DispVideoCard,
        DisplayOptions.DispVideoDriver
    );

    OTWImage = FalconDisplay.GetImageBuffer();

    endFlightTimer = 0;
    endFlightPointSet = FALSE;
    endFlightVec.x = 0.0f;
    endFlightVec.y = 0.0f;
    endFlightVec.z = -1.0f;
    otwTrackPlatform.reset(NULL);

    // Prep exit menu stuff;
    endAbort = FALSE;
    exitMenuOn = FALSE;


    // Init Text Messages (to you)
    memset(textTimeLeft, 0, sizeof(textTimeLeft));
    memset(textMessage, 0, sizeof(textMessage));
    showFrontText = 0;

    if ( not flyingEye)
    {
        // create a special "Flying Eye" type..
        flyingEye = new SpotEntity(F4FlyingEyeType + VU_LAST_ENTITY_TYPE);
        vuDatabase->/*Quick*/Insert(flyingEye);
    }

    //   MonoPrint("Doing DeviceDependentGraphicsSetup... %d\n",vuxRealTime);
    // Setup the device dependent graphics stuff
    DeviceDependentGraphicsSetup(&FalconDisplay.theDisplayDevice);
    //   MonoPrint("Done with DeviceDependentGraphicsSetup... %d\n",vuxRealTime);

    // Prep popups
    for (i = 0; i < NumPopups; i++)
        popupHas[i] = -1;

    // Set the initial time of day
    // TODO:  Fix this so it gets the right time AND day
    // TheTimeManager.SetTime(vuxGameTime + (unsigned long)FloatToInt32(todOffset * 1000.0F) );

    doWeather = PlayerOptions.WeatherOn();
    // sfr: holy freaking shit?? who did this???
    //lastotwPlatform = (SimBaseClass*)(-1);
    lastotwPlatform.reset(NULL);

    viewPoint = new RViewPoint;

    //JAM 13Dec03
    viewPoint->Setup(
        PlayerOptions.TerrainDistance()*FEET_PER_KM, PlayerOptions.MaxTerrainLevel(), 4, DisplayOptions.bZBuffering
    );

    bKeepClean = FALSE; // JB 010616

    // set the special effects detail level
    SfxClass::SetLOD(PlayerOptions.SfxLevel);

    if (otwPlatform.get() not_eq NULL)
    {
        SetGraphicsOwnship(otwPlatform.get());
    }

    // Set the initial map location
    //   MonoPrint("Starting loader.. %d\n",vuxRealTime);
    viewPoint->Update(&ownshipPos);

    // KCK: TEMPORARY FOR TIMING STATISTICS- REMOVE BEFORE FLIGHT
    // TheLoader.WaitForLoader();

    //   MonoPrint("Initializing renderer.. %d\n",vuxRealTime);
    renderer = new RenderOTW;
    renderer->Setup(OTWImage, viewPoint);

    // COBRA - DX - Switching btw Old and New Engine - Initialize DX Engine and VB Manager
    if (g_bUse_DX_Engine)
    {
        TheVbManager.Setup(OTWImage->GetDisplayDevice()->GetDefaultRC()->m_pD3D);
    }

    SetupSplashScreen();
    SplashScreenUpdate(0);

    // Register for future time of day updates for the objects with day/night states (lights)
    TheTimeManager.RegisterTimeUpdateCB(TimeUpdateCallback, this);

    // Work out a decent name for us.
    Vis_Types eCPVisType = (Vis_Types)MapVisId(VIS_F16C);
    // ---------**********-----------***********-----------**********-----
    TCHAR* eCPName = NULL;
    TCHAR* eCPNameNCTR = NULL;

    FlightClass *pFlight = FalconLocalSession->GetPlayerFlight();

    if (pFlight)
    {
        int vid = pFlight->GetVehicleID(0);  // CT number
        Falcon4EntityClassType *pClass = &Falcon4ClassTable[vid];

        if (pClass)
        {
            eCPVisType = (Vis_Types) pClass->visType[0]; // Parent Normal number

            if (pClass->dataPtr)
            {
                eCPName = ((VehicleClassDataType*)(pClass->dataPtr))->Name;
                eCPNameNCTR = ((VehicleClassDataType*)(pClass->dataPtr))->NCTR;
            }
        }
    }

    // init the virtual cockpit canvases
    //   MonoPrint("Initializing virtual cockpit.. %d\n",vuxRealTime);
    VCock_Init(eCPVisType, eCPName, eCPNameNCTR);
    //Wombat778 10-10-2003  Initialize buttons for clickable cockpit
    Button3D_Init(eCPVisType, eCPName, eCPNameNCTR);
    // MonoPrint("Initializing hud.. %d\n",vuxRealTime);

    tgtId = 0;
    objectScale = PlayerOptions.ObjectMagnification();
    viewStep = 0;
    tgtStep = 0;
    getNewCameraPos = FALSE;
    eyeFly = FALSE;

    // Initialize the ViewRotation matrix
    viewRotation = IMatrix;
    headMatrix = IMatrix;
    cameraRot = IMatrix;

    // Update object position
    viewPos.x     = 110000.0F;
    viewPos.y     = 137000.0F;
    viewPos.z     = -15000.0F;

    // Initialize the cockpit manager for the 2D cockpit
    ////////////////////////////////////////////////////////////////////////////////
    //Wombat778 4-15-04 Notes
    //
    //There are 5 possible cockpit resolutions, and numerous possible screen resolutions.
    //
    // This code first figures out what is the best cockpit resolution to use,
    // given the current screen resolution.
    // Then, it searches for the best cockpit to rescale that actually exists,
    // given the best cockpit resolution.
    // The selection order is always as follows:
    // 1) Native Resolution pit
    // 2) Higher resolution pit
    // 3) lower resolution pit
    /////////////////////////////////////////////////////////////////////////////////////////
    //Wombat778 4-03-04
    // Converted to findbestresolution,
    // and converted all of the following to use caluclated scales rather than constants.
    // sfr: take out invalid chars


    // RV - Biker - Read res from dat file
    int resX = 0;
    int resY = 0;

    char strCPFile[MAX_PATH];

    // RV - Biker - Don't use fallback for widescreen stuff because this will cause problems
    FindCockpit("ckpit_res.dat", (Vis_Types)eCPVisType, eCPName, eCPNameNCTR, strCPFile, FALSE);

    FILE *pcockpitResFile = fopen(strCPFile, "r");

    if (pcockpitResFile not_eq NULL)
    {
        const int lineLen = MAX_LINE_BUFFER - 1;
        char plineBuffer[MAX_LINE_BUFFER] = "";
        char* plinePtr;
		char* ptoken = NULL;
        char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};

        if ( not feof(pcockpitResFile))
        {
            fgets(plineBuffer, lineLen, pcockpitResFile);
            plinePtr = plineBuffer;
            ptoken = FindToken(&plinePtr, pseparators);
        }

        while ( not feof(pcockpitResFile) and strcmpi(ptoken, END_MARKER))
        {
            if ( not strcmpi(ptoken, "resX"))
            {
                ptoken = FindToken(&plinePtr, pseparators);
                sscanf(ptoken, "%d", &resX);
            }

            if ( not strcmpi(ptoken, "resY"))
            {
                ptoken = FindToken(&plinePtr, pseparators);
                sscanf(ptoken, "%d", &resY);
            }

            fgets(plineBuffer, lineLen, pcockpitResFile);
            plinePtr = plineBuffer;
            ptoken = FindToken(&plinePtr, pseparators);
        }

        fclose(pcockpitResFile);
    }

    if (resX > 0 and resY > 0)
    {
        pCockpitManager = new CockpitManager(OTWImage, "ws_ckpit.dat", TRUE, (float)DisplayOptions.DispWidth / float(resX), DisplayOptions.DispHeight / float(resY), FALSE, eCPVisType, eCPName, eCPNameNCTR);
        pPadlockCPManager = new CockpitManager(OTWImage, "ws_plock.dat", FALSE, (float)DisplayOptions.DispWidth / float(resX), DisplayOptions.DispHeight / float(resY), FALSE, eCPVisType, eCPName, eCPNameNCTR);
    }
    else
    {
        if (FindBestResolution() == 640)
        {
            switch (FindCockpitResolution(COCKPIT_FILE_6x4, COCKPIT_FILE_8x6, COCKPIT_FILE_10x7, COCKPIT_FILE_12x9, COCKPIT_FILE_16x12, eCPVisType, eCPName, eCPNameNCTR))
            {
                case 2:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_8x6, TRUE, (float)DisplayOptions.DispWidth / 800.0f, DisplayOptions.DispHeight / 600.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_8x6, FALSE, (float)DisplayOptions.DispWidth / 800.0f, DisplayOptions.DispHeight / 600.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;

                case 3:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_10x7, TRUE, (float)DisplayOptions.DispWidth / 1024.0f, DisplayOptions.DispHeight / 768.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_10x7, FALSE, (float)DisplayOptions.DispWidth / 1024.0f, DisplayOptions.DispHeight / 768.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;

                case 4:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_12x9, TRUE, (float)DisplayOptions.DispWidth / 1280.0f, DisplayOptions.DispHeight / 960.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_12x9, FALSE, (float)DisplayOptions.DispWidth / 1280.0f, DisplayOptions.DispHeight / 960.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;

                case 5:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_16x12, TRUE, (float)DisplayOptions.DispWidth / 1600.0f, DisplayOptions.DispHeight / 1200.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_16x12, FALSE, (float)DisplayOptions.DispWidth / 1600.0f, DisplayOptions.DispHeight / 1200.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;

                default:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_6x4, TRUE, (float)DisplayOptions.DispWidth / 640.0f, DisplayOptions.DispHeight / 480.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_6x4, FALSE, (float)DisplayOptions.DispWidth / 640.0f, DisplayOptions.DispHeight / 480.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;
            }
        }
        else if (FindBestResolution() == 800)
        {
            switch (FindCockpitResolution(COCKPIT_FILE_8x6, COCKPIT_FILE_10x7, COCKPIT_FILE_12x9, COCKPIT_FILE_16x12, COCKPIT_FILE_6x4, eCPVisType, eCPName, eCPNameNCTR))
            {
                default:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_8x6, TRUE, (float)DisplayOptions.DispWidth / 800.0f, DisplayOptions.DispHeight / 600.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_8x6, FALSE, (float)DisplayOptions.DispWidth / 800.0f, DisplayOptions.DispHeight / 600.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;

                case 2:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_10x7, TRUE, (float)DisplayOptions.DispWidth / 1024.0f, DisplayOptions.DispHeight / 768.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_10x7, FALSE, (float)DisplayOptions.DispWidth / 1024.0f, DisplayOptions.DispHeight / 768.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;

                case 3:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_12x9, TRUE, (float)DisplayOptions.DispWidth / 1280.0f, DisplayOptions.DispHeight / 960.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_12x9, FALSE, (float)DisplayOptions.DispWidth / 1280.0f, DisplayOptions.DispHeight / 960.0f,  FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;

                case 4:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_16x12, TRUE, (float)DisplayOptions.DispWidth / 1600.0f, DisplayOptions.DispHeight / 1200.0f,  FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_16x12, FALSE, (float)DisplayOptions.DispWidth / 1600.0f, DisplayOptions.DispHeight / 1200.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;

                case 5:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_6x4, TRUE, (float)DisplayOptions.DispWidth / 640.0f, DisplayOptions.DispHeight / 480.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_6x4, FALSE, (float)DisplayOptions.DispWidth / 640.0f, DisplayOptions.DispHeight / 480.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;
            }
        }
        else if (FindBestResolution() == 1024)
        {
            switch (FindCockpitResolution(COCKPIT_FILE_10x7, COCKPIT_FILE_12x9, COCKPIT_FILE_16x12, COCKPIT_FILE_8x6, COCKPIT_FILE_6x4, eCPVisType, eCPName, eCPNameNCTR))
            {
                default:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_10x7, TRUE, (float)DisplayOptions.DispWidth / 1024.0f, DisplayOptions.DispHeight / 768.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_10x7, FALSE, (float)DisplayOptions.DispWidth / 1024.0f, DisplayOptions.DispHeight / 768.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;

                case 2:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_12x9, TRUE, (float)DisplayOptions.DispWidth / 1280.0f, DisplayOptions.DispHeight / 960.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_12x9, FALSE, (float)DisplayOptions.DispWidth / 1280.0f, DisplayOptions.DispHeight / 960.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;

                case 3:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_16x12, TRUE, (float)DisplayOptions.DispWidth / 1600.0f, DisplayOptions.DispHeight / 1200.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_16x12, FALSE, (float)DisplayOptions.DispWidth / 1600.0f, DisplayOptions.DispHeight / 1200.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;

                case 4:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_8x6, TRUE, (float)DisplayOptions.DispWidth / 800.0f, DisplayOptions.DispHeight / 600.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_8x6, FALSE, (float)DisplayOptions.DispWidth / 800.0f, DisplayOptions.DispHeight / 600.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;

                case 5:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_6x4, TRUE, (float)DisplayOptions.DispWidth / 640.0f, DisplayOptions.DispHeight / 480.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_6x4, FALSE, (float)DisplayOptions.DispWidth / 640.0f, DisplayOptions.DispHeight / 480.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;
            }
        }
        else if (FindBestResolution() == 1280)
        {
            switch (FindCockpitResolution(COCKPIT_FILE_12x9, COCKPIT_FILE_16x12, COCKPIT_FILE_10x7, COCKPIT_FILE_8x6, COCKPIT_FILE_6x4, eCPVisType, eCPName, eCPNameNCTR)) //Wombat778 10-12-2003 Changed to use the findcockpitresolution function to make sure that order of loading is right
            {
                case 1:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_12x9, TRUE, (float)DisplayOptions.DispWidth / 1280.0f, DisplayOptions.DispHeight / 960.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_12x9, FALSE, (float)DisplayOptions.DispWidth / 1280.0f, DisplayOptions.DispHeight / 960.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;

                case 2:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_16x12, TRUE, (float)DisplayOptions.DispWidth / 1600.0f, DisplayOptions.DispHeight / 1200.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR); //Load 1600 pit and scale down to 1280
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_16x12, FALSE, (float)DisplayOptions.DispWidth / 1600.0f, DisplayOptions.DispHeight / 1200.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;

                default:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_10x7, TRUE, (float)DisplayOptions.DispWidth / 1024.0f, DisplayOptions.DispHeight / 768.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR); //Load 1024 pit and scale up to 1280
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_10x7, FALSE, (float)DisplayOptions.DispWidth / 1024.0f, DisplayOptions.DispHeight / 768.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;

                case 4:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_8x6, TRUE, (float)DisplayOptions.DispWidth / 800.0f, DisplayOptions.DispHeight / 600.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_8x6, FALSE, (float)DisplayOptions.DispWidth / 800.0f, DisplayOptions.DispHeight / 600.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;

                case 5:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_6x4, TRUE, (float)DisplayOptions.DispWidth / 640.0f, DisplayOptions.DispHeight / 480.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_6x4, FALSE, (float)DisplayOptions.DispWidth / 640.0f, DisplayOptions.DispHeight / 480.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;

            }
        }
        else if (FindBestResolution() == 1600)
        {
            switch (FindCockpitResolution(COCKPIT_FILE_16x12, COCKPIT_FILE_12x9, COCKPIT_FILE_10x7, COCKPIT_FILE_8x6, COCKPIT_FILE_6x4, eCPVisType, eCPName, eCPNameNCTR))
            {
                case 1:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_16x12, TRUE, (float)DisplayOptions.DispWidth / 1600.0f, DisplayOptions.DispHeight / 1200.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR); //Load the 1600 pit
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_16x12, FALSE, (float)DisplayOptions.DispWidth / 1600.0f, DisplayOptions.DispHeight / 1200.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;

                case 2:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_12x9, TRUE, (float)DisplayOptions.DispWidth / 1280.0f, DisplayOptions.DispHeight / 960.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR); //Load 1280 pit and scale up to 1600
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_12x9, FALSE, (float)DisplayOptions.DispWidth / 1280.0f, DisplayOptions.DispHeight / 960.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;

                default:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_10x7, TRUE, (float)DisplayOptions.DispWidth / 1024.0f, DisplayOptions.DispHeight / 768.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR); //Load 1024 pit and scale up to 1600
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_10x7, FALSE, (float)DisplayOptions.DispWidth / 1024.0f, DisplayOptions.DispHeight / 768.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;

                case 4:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_8x6, TRUE, (float)DisplayOptions.DispWidth / 800.0f, DisplayOptions.DispHeight / 600.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_8x6, FALSE, (float)DisplayOptions.DispWidth / 800.0f, DisplayOptions.DispHeight / 600.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;

                case 5:
                    pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_6x4, TRUE, (float)DisplayOptions.DispWidth / 640.0f, DisplayOptions.DispHeight / 480.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_6x4, FALSE, (float)DisplayOptions.DispWidth / 640.0f, DisplayOptions.DispHeight / 480.0f, FALSE, eCPVisType, eCPName, eCPNameNCTR);
                    break;
            }
        }

        else
        {
            // Unsupported resolution, but fill in w/ 8x6
            pCockpitManager = new CockpitManager(OTWImage, COCKPIT_FILE_8x6, TRUE, 1.0f, 1.0f, TRUE);
            pPadlockCPManager = new CockpitManager(OTWImage, PADLOCK_FILE_8x6, FALSE, 1.0f, 1.0f, TRUE);
            startMode = ModeHud;
        }
    }

    pMenuManager = new MenuManager(DisplayOptions.DispWidth, DisplayOptions.DispHeight);

    TheHud = new HudClass;
    theLantirn = new LantirnClass;

    // Adjust MFD Position for screen res
    //   MonoPrint("Initializing MFDs.. %d\n",vuxRealTime);
    MfdSize = 154 * DisplayOptions.DispHeight / 480;

    if (g_bEnableMfdSize)
        MfdSize = (int)((g_fMfd_p_Size / 100.0F) * 154.0F) * DisplayOptions.DispHeight / 480; //a.s.

    VirtualMFD[0].top    = DisplayOptions.DispHeight - MfdSize;
    VirtualMFD[0].right  = MfdSize - 1;
    VirtualMFD[0].bottom = DisplayOptions.DispHeight - 1;
    VirtualMFD[1].left   = DisplayOptions.DispWidth - MfdSize;
    VirtualMFD[1].top    = DisplayOptions.DispHeight - MfdSize;
    VirtualMFD[1].right  = DisplayOptions.DispWidth - 1;
    VirtualMFD[1].bottom = DisplayOptions.DispHeight - 1;
    VirtualMFD[2].left   = DisplayOptions.DispWidth - MfdSize;
    VirtualMFD[2].right  = DisplayOptions.DispWidth - 1;
    VirtualMFD[2].bottom = MfdSize - 1;
    VirtualMFD[3].right  = MfdSize - 1;
    VirtualMFD[3].bottom = MfdSize - 1;
    VirtualMFD[4].right  = MfdSize - 1;
    VirtualMFD[4].bottom = MfdSize - 1;

    // RED - Create the drawables shared by the MFDs
    MFDClass::CreateDrawables();

    for (i = 0; i < NUM_MFDS; i++)
    {
        MfdDisplay[i] = new MFDClass(i, renderer);

        // we only need to set the virtual position once
        MfdDisplay[i]->UpdateVirtualPosition(&Origin, &IMatrix);
    }


    MfdDisplay[0]->SetNewMode(MFDClass::MfdMenu);
    MfdDisplay[1]->SetNewMode(MFDClass::MfdMenu);

    //   MonoPrint("Initializing DrawableBSPs.. %d\n",vuxRealTime);
    CreateCockpitGeometry(&vrCockpit, vrCockpitModel[0], vrCockpitModel[1]);

    /*if(renderer->GetAlphaMode())*/
    vrCockpit->SetSwitchMask(0, 1);
    // else vrCockpit->SetSwitchMask( 0, 0);

    if (PlayerOptions.SimVisualCueMode == VCLiftLine or PlayerOptions.SimVisualCueMode == VCBoth)
        vrCockpit->SetSwitchMask(1, 1);
    else 
        vrCockpit->SetSwitchMask(1, 0);

    if (PlayerOptions.SimVisualCueMode == VCReflection or PlayerOptions.SimVisualCueMode == VCBoth)
        vrCockpit->SetSwitchMask(3, 1);
    else 
        vrCockpit->SetSwitchMask(3, 0);

    // Load the f16
    DrawableBSP::LockAndLoad(vrCockpitModel[2]);  // f16

    // Load the damage f16
    DrawableBSP::LockAndLoad(vrCockpitModel[3]);
    //////////////////////

    // Lock and load bsp's for ejected pilot.
    DrawableBSP::LockAndLoad(MapVisId(VIS_EJECT1));
    DrawableBSP::LockAndLoad(MapVisId(VIS_EJECT2));
    DrawableBSP::LockAndLoad(MapVisId(VIS_EJECT3));
    DrawableBSP::LockAndLoad(MapVisId(797));
    DrawableBSP::LockAndLoad(MapVisId(VIS_END_MISSION));

    // renderer->SetTerrainTextureLevel( PlayerOptions.TextureLevel() );
    // renderer->SetSmoothShadingMode( PlayerOptions.GouraudOn() );

    renderer->SetHazeMode(PlayerOptions.HazingOn());
    renderer->SetDitheringMode(PlayerOptions.HazingOn());

    renderer->SetFilteringMode(PlayerOptions.FilteringOn());
    renderer->SetObjectDetail(PlayerOptions.ObjectDetailLevel());
    // renderer->SetAlphaMode(PlayerOptions.AlphaOn());
    renderer->SetObjectTextureState(TRUE);//PlayerOptions.ObjectTexturesOn());

    //Get player options
    doGLOC   = PlayerOptions.BlackoutOn() ? TRUE : FALSE;

    if ( not PlayerOptions.NameTagsOn())
    {
        // Make sure name tags are off if they're not allowed
        DrawableBSP::drawLabels = FALSE;
        DrawablePoint::drawLabels = FALSE;
    }

    // Initialize ejection cam.
    ejectCam = 0;
    prevChase = 0;

    // Initialize the campaign/sim object height refresh loop
    nextCampObjectHeightRefresh = 0;

    SetOTWDisplayMode(startMode);

    drawInfoBar = PlayerOptions.getInfoBar(); // Retro 16Dec2003

    takePrettyScreenShot = OFF; // Retro 7May2004
    LabelState = FALSE; // Retro 8May2004

    if (g_bEnableDisplacementCam)
        displaceCamera = true; // Retro 23Dec2003
    else
        displaceCamera = false; // Retro 25Dec2003

    // Retro 16Dec2003 - wipes internal structs clear of messages of last mission
    // 'drawSubTitles' doesn´t destroy or create the radiolabel class - it just governs if the labels are created
    // with this variable the user can temporarily kill the subtitles, however if he wants them off alltogether he
    // has to do this in the UI
    if ((PlayerOptions.getSubtitles()) and (radioLabel))
    {
        drawSubTitles = true; // Retro 16Dec2003
        radioLabel->ResetAll();
    }
    else
    {
        drawSubTitles = false; // Retro 21Dec2003
    }

    if ((g_bPilotEntertainment) and (winamp)) // Retro 3Jan2004 (all) - looking for that winamp window..
    {
        winamp->InitWinAmp();
    }

    IO.ResetAllInputs(); // Retro 9Jan2004

    extern void SetThrottleInActive();
    SetThrottleInActive(); // Retro 4Jan2004

    // Retro 18Jan2004
    theMouseView.Reset();
    // Retro ends

    // Retro 1Feb2004 - preinit those values so that on first frame the values are shown
    // if the ac has no dual engine / no flaps then the display routines are NOT entered
    // in subsequent frames

    // RV - Biker - Never seen any difference in left and right engine so this is worthless
    //showEngine = true;
    showEngine = false;
    showFlaps = false;
    // Retro 1Feb2004 end

    slewRate = (float)PlayerOptions.GetKeyboardPOVPanningSensitivity() * DTR;

    // Retro 15Feb2004 - load the default clickable pit mode into a variable that may
    // be changed by the user in the 3d
    extern bool clickableMouseMode;
    clickableMouseMode = PlayerOptions.GetClickablePitMode();

    // init sound fx
    F4SoundFXInit();

    // Start up Direct Input
    SetupDIMouseAndKeyboard(hInst, FalconDisplay.appWin);
    SetFocus(FalconDisplay.appWin);

    // sfr: this is the correct place to set it.
    SetActive(TRUE);

#ifdef CHECK_LEAKAGE
#if 0

    if (lastErrorFn)
    {
        dbgMemReportLeakage(MemDefaultPool, 2, leakChkPt);
        MemSetErrorHandler(lastErrorFn);
        dbgMemSetDefaultErrorOutput(DBGMEM_OUTPUT_FILE, "bsimleak.txt");
        dbgMemReportLeakage(MemDefaultPool, 2, leakChkPt);
        MemSetErrorHandler(errPrint);
    }
    else
    {
        dbgMemSetDefaultErrorOutput(DBGMEM_OUTPUT_FILE, "bsimleak.txt");
        dbgMemReportLeakage(MemDefaultPool, 2, leakChkPt);
    }

#endif
#endif
}


int OTWDriverClass::Exit(void)
{
    otwPlatform.reset(NULL);
    OTWWin = NULL;
    OTWImage = NULL;

    F4SoundLeaving3d(); // MLR 12/2/2003 - Stop all sounds when going back to the UI // MLR 1/25/2004 - changed function

    // Set ourselves "out of the sim"
    // So we know we can't deaggregate anything else, create any new drawable objects or
    // otherwise do anything stupid.
    SetActive(FALSE);
    SetGraphicsOwnship(NULL);
    ClearSfxLists();
    // Cobra - Clear the PS Alist
    DrawableParticleSys::ClearParticleList();


    SetShutdown(1);


    // Stop receiving time updates
    TheTimeManager.ReleaseTimeUpdateCB(TimeUpdateCallback, this);


    for (int i = 0; i < NUM_MFDS; i++)
    {
        delete MfdDisplay[i];
        MfdDisplay[i] = NULL;
    }

    // RED - Destroy the drawables shared by the MFDs
    MFDClass::DestroyDrawables();

    if (renderer)
    {
        renderer->Cleanup();
        delete renderer;
        renderer = NULL;
    }

    F4EnterCriticalSection(cs_update); // JB 010616

    if (viewPoint)
    {
        viewPoint->Cleanup();
        delete viewPoint;
        viewPoint = NULL;
    }

    //JAM 19Nov03
    if (realWeather)
    {
        realWeather->Cleanup();
    }

    // Clean up the cockpit stuff
    VCock_Cleanup();
    delete pCockpitManager;
    pCockpitManager = NULL;
    delete pPadlockCPManager;
    pPadlockCPManager = NULL;

    // Clean up the HUD
    delete TheHud;
    TheHud = NULL;

    delete theLantirn;
    theLantirn = NULL;
    // Unlock locked stuff
    DrawableBSP::Unlock(MapVisId(VIS_END_MISSION));
    DrawableBSP::Unlock(MapVisId(797));
    DrawableBSP::Unlock(MapVisId(VIS_EJECT1));
    DrawableBSP::Unlock(MapVisId(VIS_EJECT2));
    DrawableBSP::Unlock(MapVisId(VIS_EJECT3));

    // MLR - this fucks shit up
    // Unlock damage f16
    //DrawableBSP::Unlock (MapVisId(VIS_CF16A));
    /////////////////////////
    //DrawableBSP::Unlock(MapVisId(VIS_F16C));

    DrawableBSP::Unlock(MapVisId(VIS_F16C));

    if (vrCockpit)
    {
        DrawableBSP::Unlock(vrCockpit->GetID());
        delete vrCockpit;
        vrCockpit = NULL;
    }

    SetExitMenu(FALSE);

    if (flyingEye)
    {
        vuDatabase->Remove(flyingEye);
    }

    flyingEye = NULL;

    CleanupDIMouseAndKeyboard();

    // Make sure we don't leave ourselves in NVG mode
    if (TheTimeOfDay.GetNVGmode())
    {
        TheTimeOfDay.SetNVGmode(FALSE);
    }

    OTWDriver.CleanViewpoint(); // JB 010615
    // FRB - ALERT
    // sfr: we are changing gfx mode, flush loader
    TheLoader.WaitLoader();

    DeviceDependentGraphicsCleanup(&FalconDisplay.theDisplayDevice);
    FalconDisplay.LeaveMode();
    F4LeaveCriticalSection(cs_update); // JB 010616

    ejectCam = 0;
    prevChase = 0;

    // end the sound effects.
    F4SoundFXEnd();

    delete pMenuManager;
    pMenuManager = NULL;

#ifdef CHECK_LEAKAGE

    if (lastErrorFn)
    {
        dbgMemReportLeakage(MemDefaultPool, 25, leakChkPt);
        MemSetErrorHandler(lastErrorFn);
        dbgMemSetDefaultErrorOutput(DBGMEM_OUTPUT_FILE, "simleak.txt");
        dbgMemReportLeakage(MemDefaultPool, 25, leakChkPt);
        MemSetErrorHandler(errPrint);
    }
    else
    {
        dbgMemSetDefaultErrorOutput(DBGMEM_OUTPUT_FILE, "simleak.txt");
        dbgMemReportLeakage(MemDefaultPool, 25, leakChkPt);
    }

    leakChkPt++;
    dbgMemSetCheckpoint(leakChkPt);
#endif

    SimDriver.ShrinkSimMemoryPools();

    // cleanup camera
    vuDatabase->Remove(gOtwCameraLocation);

    ObjectLOD::ReleaseLodList();
    TheVbManager.Release();

    // sfr: this is extremely important.
    // dont let the loader do anything right now, since the graphics context
    // was cleaned and not restored yet. When UI starts up again, it will restart loader.
    // FRB - Causes Recon to hang after leaving 3D
    //TheLoader.SetPause(true);

    return endAbort;
}

void OTWDriverClass::ObjectSetData(SimBaseClass *obj, Tpoint *simView, Trotation *viewRotation)
{
    viewRotation->M11 = obj->dmx[0][0];
    viewRotation->M21 = obj->dmx[0][1];
    viewRotation->M31 = obj->dmx[0][2];

    viewRotation->M12 = obj->dmx[1][0];
    viewRotation->M22 = obj->dmx[1][1];
    viewRotation->M32 = obj->dmx[1][2];

    viewRotation->M13 = obj->dmx[2][0];
    viewRotation->M23 = obj->dmx[2][1];
    viewRotation->M33 = obj->dmx[2][2];
    // sfr: test horizon bug
    //*viewRotation = IMatrix;

    // Update object position
    simView->x     = obj->XPos();
    simView->y     = obj->YPos();
    simView->z     = obj->ZPos();

    /*
    // Do we want Control surface data?
    if ( not obj->IsSimObjective() and not obj->IsLocal())
    {
    FalconSimDataToggle *dataRequest;

    if ((fabs (obj->XPos() - flyingEye->XPos()) < 3.0F * NM_TO_FT and 
    fabs (obj->YPos() - flyingEye->YPos()) < 3.0F * NM_TO_FT) or obj->IsSetFlag(MOTION_OWNSHIP))
    {
    if ( not ((SimMoverClass*)obj)->DataRequested())
    {
    ((SimMoverClass*)obj)->SetDataRequested(TRUE);
    dataRequest = new FalconSimDataToggle (obj->Id(), FalconLocalGame);
    dataRequest->dataBlock.flag = 1;
    dataRequest->dataBlock.entityID = obj->Id();
    MonoPrint ("%8ld Requesing Data\n", SimLibElapsedTime);
    FalconSendMessage (dataRequest,FALSE);
    }
    }
    else if (((SimMoverClass*)obj)->DataRequested())
    {
    ((SimMoverClass*)obj)->SetDataRequested(FALSE);
    dataRequest = new FalconSimDataToggle (obj->Id(), FalconLocalGame);
    dataRequest->dataBlock.flag = -1;
    dataRequest->dataBlock.entityID = obj->Id();
    MonoPrint ("%8ld Canceling Request\n", SimLibElapsedTime);
    FalconSendMessage (dataRequest,FALSE);
    }
    }
    */
}

// JB 010604 start
RViewPoint* OTWDriverClass::GetViewpoint()
{
    InitViewpoint();

    return viewPoint;
};

void OTWDriverClass::InitViewpoint()
{
    F4EnterCriticalSection(cs_update);

    if ( not viewPoint and not bKeepClean and Texture::IsSetup())
    {
        viewPoint = new RViewPoint;
        viewPoint->Setup(PlayerOptions.TerrainDistance()*FEET_PER_KM, PlayerOptions.MaxTerrainLevel(), 4, DisplayOptions.bZBuffering);
        viewPoint->Update(&ownshipPos);
    }

    F4LeaveCriticalSection(cs_update);
}

void OTWDriverClass::CleanViewpoint()
{
    F4EnterCriticalSection(cs_update);
    bKeepClean = TRUE;

    if (viewPoint)
    {
        viewPoint->Cleanup();
        delete viewPoint;
        viewPoint = NULL;
    }

    F4LeaveCriticalSection(cs_update);
}

int OTWDriverClass::GetGroundType(float x, float y)
{
    int type = COVERAGE_PLAINS;

    F4EnterCriticalSection(cs_update);

    if (viewPoint)
    {
        type = viewPoint->GetGroundType(x, y);
    }

    F4LeaveCriticalSection(cs_update);

    return type;
}

// JB 010604 end

void OTWDriverClass::UpdateCameraFocus()
{
    BIG_SCALAR
    focusX,
    focusY,
    focusZ;
    float groundHeight;

    otwPlatform->GetFocusPoint(focusX, focusY, focusZ);

    flyingEye->SetPosition(focusX, focusY, focusZ);
    flyingEye->SetYPR(otwPlatform->Yaw(), otwPlatform->Pitch(), otwPlatform->Roll());

    focusPoint.x = focusX;
    focusPoint.y = focusY;
    focusPoint.z = focusZ;

    // make sure camera never goes below the ground
    groundHeight = GetApproxGroundLevel(focusPoint.x, focusPoint.y);
    F4EnterCriticalSection(cs_update);

    if (viewPoint and viewPoint->IsReady() and // JB 010604
        focusPoint.z - groundHeight > -100.0f)
    {
        groundHeight = viewPoint->GetGroundLevel(focusPoint.x, focusPoint.y);

        if (focusPoint.z >= groundHeight)
            focusPoint.z = groundHeight - 5.0f;
    }

    F4LeaveCriticalSection(cs_update);
}

void OTWDriverClass::FindNewOwnship(void)
{
    SimBaseClass* newObject = NULL;
    SimBaseClass* theObject = NULL;
    SimBaseClass* newOwnship = NULL;

    if (viewStep > 0)
    {
        newObject = NULL;
        VuListIterator updateWalker(SimDriver.objectList);
        theObject = (SimBaseClass*)updateWalker.GetFirst();
        newObject = (SimBaseClass*)updateWalker.GetNext();

        while (newObject)
        {
            if (otwPlatform.get() == theObject)
            {
                break;
            }

            theObject = newObject;
            newObject = (SimBaseClass*)updateWalker.GetNext();
        }

        if ( not newObject)
        {
            // Get the first one
            newObject = (SimBaseClass*)updateWalker.GetFirst();
        }
    }
    else if (viewStep < 0)
    {
        newObject = NULL;
        VuListIterator updateWalker(SimDriver.objectList);
        theObject = (SimBaseClass*)updateWalker.GetFirst();

        while (theObject)
        {
            if (otwPlatform.get() == theObject)
            {
                break;
            }

            newObject = theObject;
            theObject = (SimBaseClass*)updateWalker.GetNext();
        }

        if ( not newObject)
        {
            // Get the last one
            theObject = (SimBaseClass*)updateWalker.GetFirst();

            while (theObject)
            {
                newObject = theObject;
                theObject = (SimBaseClass*)updateWalker.GetNext();
            }
        }
    }

    if (newObject)
    {
        newOwnship = newObject;

        // make sure we're no longer in any cockpit-type mode if the
        // newobject isn't playerEntity
        if (newObject not_eq SimDriver.GetPlayerAircraft() and DisplayInCockpit())
        {
            SelectDisplayMode(ModeOrbit);
        }
    }

    viewStep = 0;
    SetGraphicsOwnship(newOwnship);
}

float OTWDriverClass::GetGroundLevel(float x, float y, Tpoint* normal)
{
#if NEW_SERVER_VIEWPOINT
    float bestRet = 0.0f;
    int bestLod = 10; // @TODO use MAXLOD
    // best normal and current one
    Tpoint
    *bestNormal = (normal == NULL) ? NULL : new Tpoint,
     *cNormal = (normal == NULL) ? NULL : new Tpoint;
    // x1 = x2;
#define COPYNORMAL(x1, x2) do { if (x1){ *x1 = *x2; } } while (0)

    InitViewpoint();
    F4EnterCriticalSection(cs_update);

    // try ownship viewpoint first
    if (viewPoint and viewPoint->IsReady())
    {
        bestRet = viewPoint->GetGroundLevel(x, y, bestNormal, &bestLod);
    }

    F4LeaveCriticalSection(cs_update);

    if (bestLod > 0)
    {
        // now try remote viewpoints to see if we can get a better one
        F4ScopeLock sl(vmMutex);

        for (
            std::map < VuBin<FalconSessionEntity>, TViewPoint*>::iterator it = viewpointMap.begin();
            bestLod > 0 and it not_eq viewpointMap.end();
            ++it
        )
        {
            TViewPoint *v = it->second;
            int lod;
            float ret = v->GetGroundLevel(x, y, cNormal, &lod);

            // see if we have a better lod with this viewpoint, if not, continue
            if (lod < bestLod)
            {
                bestLod = lod;
                bestRet = ret;
                COPYNORMAL(bestNormal, cNormal);
            }
        }
    }

    // final normal copy
    COPYNORMAL(normal, bestNormal);
    return bestRet;


#else
    InitViewpoint();
    F4EnterCriticalSection(cs_update);

    if (viewPoint and viewPoint->IsReady())
    {
        float level = viewPoint->GetGroundLevel(x, y, normal);
        F4LeaveCriticalSection(cs_update);
        return level;
    }

    F4LeaveCriticalSection(cs_update);
    return (0.0F);
#endif
}

float OTWDriverClass::GetApproxGroundLevel(float x, float y)
{
    InitViewpoint();

    F4EnterCriticalSection(cs_update);

    if (viewPoint and viewPoint->IsReady())
    {
        ShiAssert(FALSE == F4IsBadReadPtr(viewPoint, sizeof * viewPoint)); // JPO CTD check
        float level = viewPoint->GetGroundLevelApproximation(x, y);
        F4LeaveCriticalSection(cs_update);
        return level;
    }

    F4LeaveCriticalSection(cs_update);

    return (0.0F);
}

void OTWDriverClass::GetAreaFloorAndCeiling(float *floor, float *ceiling)
{
    InitViewpoint();

    F4EnterCriticalSection(cs_update);

    if (viewPoint and viewPoint->IsReady())
    {
        viewPoint->GetAreaFloorAndCeiling(floor, ceiling);
    }
    else
    {
        *floor = 0.0F;
        // sfr: return something valid...
        //*ceiling = 0.0F;
        *ceiling = -100000.0F;
    }

    F4LeaveCriticalSection(cs_update);
}

float OTWDriverClass::DistanceFromCloudEdge()
{
    return (-5000.0F);
}

void OTWDriverClass::RemoveObjectFromDrawList(SimBaseClass* theObject)
{
    ShiAssert(theObject);
    ShiAssert(theObject->drawPointer);

    F4EnterCriticalSection(cs_update);

    if (
        viewPoint and viewPoint->IsReady() and // JB 010604
        theObject->drawPointer->InDisplayList()
    )
    {
        viewPoint->RemoveObject(theObject->drawPointer);

        // RED - NEW PS TRAILS - No more needed a remove
        /*if (theObject->IsMissile() and 
         ((MissileClass*)theObject)->trail and 
         ((MissileClass*)theObject)->trail->InDisplayList() )
        {
         viewPoint->RemoveObject(((MissileClass*)theObject)->trail);
        }*/

        if (
            theObject->IsGroundVehicle() and 
            ((GroundClass*)theObject)->truckDrawable and 
            ((GroundClass*)theObject)->truckDrawable->InDisplayList()
        )
        {
            viewPoint->RemoveObject(((GroundClass*)theObject)->truckDrawable);
        }
    }

    F4LeaveCriticalSection(cs_update);
}

int OTWDriverClass::GetGroundIntersection(euler* dir, vector* pos)
{
    int retval = 0;
    Tpoint viewDir, groundPos;
    mlTrig trigYaw, trigPitch;

    InitViewpoint();

    F4EnterCriticalSection(cs_update);

    if (viewPoint)
    {
        // JB 010616
        mlSinCos(&trigYaw,   dir->yaw);
        mlSinCos(&trigPitch, dir->pitch);

        viewDir.x =  trigYaw.cos * trigPitch.cos;
        viewDir.y =  trigYaw.sin * trigPitch.cos;
        viewDir.z = -trigPitch.sin;

        retval = viewPoint->GroundIntersection(&viewDir, &groundPos);
    }

    F4LeaveCriticalSection(cs_update);

    if (retval)
    {
        pos->x = groundPos.x;
        pos->y = groundPos.y;
        pos->z = groundPos.z;
    }

    return (retval);
}


int OTWDriverClass::CheckLOS(FalconEntity *pt1, FalconEntity *pt2)
{
    Tpoint start, finish;
    // sfr: default value is 1 if terrain is not loadedd
    int LOS = 1;

    InitViewpoint();

    F4EnterCriticalSection(cs_update);

    if (viewPoint)  // JB 010616
    {
        start.x = pt1->XPos();
        start.y = pt1->YPos();
        start.z = pt1->ZPos();

        finish.x = pt2->XPos();
        finish.y = pt2->YPos();
        finish.z = pt2->ZPos();

        LOS = viewPoint->LineOfSight(&start, &finish);

        // 2002-02-26 ADDED BY S.G. No LOS, try the other way around...
        if ( not LOS)
        {
            LOS = viewPoint->LineOfSight(&finish, &start);
        }

        // END OF ADDED SECTION 2002-02-26
    }

    F4LeaveCriticalSection(cs_update);

    return LOS;
}

int OTWDriverClass::CheckCloudLOS(FalconEntity *pt1, FalconEntity *pt2)
{
    Tpoint start, finish;
    // sfr: default value 1 if not loaded
    int LOS = 1;

    InitViewpoint();

    F4EnterCriticalSection(cs_update);

    if (viewPoint)  // JB 010616
    {
        start.x = pt1->XPos();
        start.y = pt1->YPos();
        start.z = pt1->ZPos();

        finish.x = pt2->XPos();
        finish.y = pt2->YPos();
        finish.z = pt2->ZPos();

        LOS = viewPoint->CloudLineOfSight(&start, &finish);
    }

    F4LeaveCriticalSection(cs_update);

    return LOS;
}

int OTWDriverClass::CheckCompositLOS(FalconEntity *pt1, FalconEntity *pt2)
{
    Tpoint start, finish;
    // sfr: defalt value 1 if not loaded
    int LOS = 1;

    InitViewpoint();

    F4EnterCriticalSection(cs_update);

    if (viewPoint)  // JB 010616
    {
        start.x = pt1->XPos();
        start.y = pt1->YPos();
        start.z = pt1->ZPos();

        finish.x = pt2->XPos();
        finish.y = pt2->YPos();
        finish.z = pt2->ZPos();

        LOS = FloatToInt32(viewPoint->CompositLineOfSight(&start, &finish));
    }

    F4LeaveCriticalSection(cs_update);

    return LOS;
}


void OTWDriverClass::ScrollMessages()
{
    short i;

    i = 1;

    while (i < (MAX_CHAT_LINES) and textMessage[i - 1][0])
    {
        strncpy(textMessage[i - 1], textMessage[i], sizeof(char)*MAX_CHAT_LENGTH);
        textTimeLeft[i - 1] = textTimeLeft[i];
        i++;
    }

    memset(textMessage[i - 1], 0, sizeof(char)*MAX_CHAT_LENGTH);
    textTimeLeft[i - 1] = 0;

    if (textMessage[0][0])
    {
        showFrontText or_eq SHOW_MESSAGES;
    }
    else
    {
        showFrontText and_eq compl SHOW_MESSAGES;
    }
}

void OTWDriverClass::ShowMessage(char* msg)
{
    short i;

    if (msg)
    {
        F4SoundFXSetDist(SFX_CP_ICP1, TRUE, 0.0f, 1.0f);

        showFrontText or_eq SHOW_MESSAGES;

        if (textMessage[MAX_CHAT_LINES - 1][0])
        {
            // All 5 lines displayed
            ScrollMessages();
        }

        i = 0;

        while (textMessage[i][0] and i < (MAX_CHAT_LINES - 1))
        {
            i++;
        }

        strcpy(textMessage[i], msg);
        textTimeLeft[i] = vuxRealTime + (15 * VU_TICS_PER_SECOND);
    }
}

void PositandOrientSetData(float x, float y, float z, float pitch, float roll, float yaw,
                           Tpoint* simView, Trotation* viewRotation)
{
    float costha, sintha, cosphi, sinphi, cospsi, sinpsi;

    costha = (float)cos(pitch);
    sintha = (float)sin(pitch);
    cosphi = (float)cos(roll);
    sinphi = (float)sin(roll);
    cospsi = (float)cos(yaw);
    sinpsi = (float)sin(yaw);

    viewRotation->M11 = cospsi * costha;
    viewRotation->M21 = sinpsi * costha;
    viewRotation->M31 = -sintha;

    viewRotation->M12 = -sinpsi * cosphi + cospsi * sintha * sinphi;
    viewRotation->M22 = cospsi * cosphi + sinpsi * sintha * sinphi;
    viewRotation->M32 = costha * sinphi;

    viewRotation->M13 = sinpsi * sinphi + cospsi * sintha * cosphi;
    viewRotation->M23 = -cospsi * sinphi + sinpsi * sintha * cosphi;
    viewRotation->M33 = costha * cosphi;

    simView->x = x;
    simView->y = y;
    simView->z = z;
}

void DecomposeMatrix(Trotation* matrix, float* pitch, float* roll, float* yaw)
{
    float tmp1, tmp2;

    *pitch = -(float)asin(matrix->M31);
    tmp1 = max(min(matrix->M21 / (float)cos(*pitch), 1.0F), -1.0F);    // sin
    tmp2 = max(min(matrix->M11 / (float)cos(*pitch), 1.0F), -1.0F);    // cos
    *yaw = (float)atan2(tmp1, tmp2);
    tmp1 = max(min(matrix->M32 / (float)cos(*pitch), 1.0F), -1.0F);

    *roll = (float)atan2(tmp1, (float)sqrt(1.0F - tmp1 * tmp1));

}
//Wombat778 4-03-04 Helper to allow scaling to nonstandard resolutions

int FindBestResolution(void)
{
    if (DisplayOptions.DispWidth > 1280)
        return 1600;

    if (DisplayOptions.DispWidth > 1024)
        return 1280;

    if (DisplayOptions.DispWidth > 800)
        return 1024;

    if (DisplayOptions.DispWidth > 640)
        return 800;

    return 640;
}

#if NEW_SERVER_VIEWPOINT
// sfr: viewpoint stuff
void OTWDriverClass::AddViewpoint(FalconSessionEntity *session)
{
    FalconGameEntity *g = FalconLocalGame;
    FalconEntity *e;

    if (g == NULL or not g->IsLocal() or session == NULL or ((e = session->GetPlayerEntity()) == NULL))
    {
        return;
    }

    TViewPoint *vp = new TViewPoint;
    float td = PlayerOptions.TerrainDistance() * FEET_PER_KM;
    float ranges[5];

    for (int i = 4; i >= 0; --i)
    {
        ranges[i] = (i == 4) ? td * 1.414f : td;
        td /= 2.0f;
    }

    vp->Setup(PlayerOptions.MaxTerrainLevel(), 4, ranges);

    F4ScopeLock sl(vmMutex);

    if (viewpointMap.insert(std::make_pair(VuBin<FalconSessionEntity>(session), vp)).second == false)
    {
        // insertion failed (duplicate)
        vp->Cleanup();
        delete vp;
    }
}

void OTWDriverClass::RemoveViewpoint(FalconSessionEntity *session)
{
    if (session == NULL)
    {
        return;
    }

    VuBin<FalconSessionEntity> sb(session);
    F4ScopeLock sl(vmMutex);
    std::map<VuBin<FalconSessionEntity>, TViewPoint*>::iterator it = viewpointMap.find(sb);

    if (it not_eq viewpointMap.end())
    {
        TViewPoint *vp = it->second;
        vp->Cleanup();
        delete vp;
        viewpointMap.erase(it);
    }
}

void OTWDriverClass::UpdateViewpoints()
{
    F4ScopeLock sl(vmMutex);

    for (
        std::map<VuBin<FalconSessionEntity>, TViewPoint*>::iterator it = viewpointMap.begin();
        it not_eq viewpointMap.end();
        ++it
    )
    {
        TViewPoint *tv = it->second;
        FalconEntity *fe = it->first->GetPlayerEntity();
        Tpoint p;
        p.x = fe->XPos();
        p.y = fe->YPos();
        p.z = fe->ZPos();
        tv->Update(&p);
    }
}

#endif
