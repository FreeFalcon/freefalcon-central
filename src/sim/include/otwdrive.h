#ifndef _OTWDRIVE_H
#define _OTWDRIVE_H

#include <vector>
#include <map>

#include "graphics/include/matrix.h"
#include "falclib/include/f4thread.h"
#include "falclib/include/f4vu.h"
#include "Graphics/Include/TerrTex.h"
#include "inpFunc.h" //Wombat778 10-10-2003 added for 3d clickable cockpit
//#include "cpmanager.h" //sfr: dependencies: other files may include this
#include "simbase.h"


#ifdef USE_SH_POOLS
#include "SmartHeap/Include/smrtheap.h"
#endif

class SfxClass;
class SimBaseClass;
class RViewPoint;
class RenderOTW;
class Render2D;
class ImageBuffer;
class DrawableObject;
class DrawableTrail;
class Drawable2D;
class DrawableBSP;
class SimObjectType;
class CockpitManager;
class EjectedPilotClass;  // MPS 9/9/97
class Canvas3D;
class VDial;
class MenuManager;

// sfr: allows multiple server viewpoints and remove me123 hacks
#define NEW_SERVER_VIEWPOINT 1
#if NEW_SERVER_VIEWPOINT
class TViewPoint;
#endif

enum
{
    // enums for CHAT Messages
    MAX_CHAT_LINES = 10,
    MAX_CHAT_LENGTH = 100,

    // flags for Displaying at the very end if the display loop
    SHOW_CHATBOX = 0x00000001,
    SHOW_MESSAGES = 0x00000002,
    SHOW_PAUSE = 0x00000010,
    SHOW_X2 = 0x00000020,
    SHOW_X4 = 0x00000040,
    SHOW_TE_SCORES = 0x00000100,
    SHOW_DOGFIGHT_SCORES = 0x00000200,
};

// The time to enable the Big load
#define BIG_LOAD_TIMEOUT 5 * 1000


// Defines for Virtual Cockpit Head Motion
#define YAW_PITCH 1
#define PITCH_YAW 0

#define HEAD_TRANSISTION1 2
#define HEAD_TRANSISTION2 3
#define HEAD_TRANSISTION3 4

#define STOP_STATE0 0
#define STOP_STATE1 1
#define STOP_STATE2 2
#define STOP_STATE3 3

#define LTOR 1
#define RTOL -1

#define PAN_AND_TILT 0
#define PAN_ONLY 1
#define TILT_ONLY 2
#define NO_PAN_OR_TILT 3
// End V Cockpit Defines

class displayList
{
public:
    displayList(void);
    ~displayList(void);

    DrawableObject* object;
    DrawableObject* object1;
    float x, y, z;
    int data1;
    displayList* next;

#ifdef USE_SH_POOLS
public:
    // Overload new/delete to use a SmartHeap fixed size pool
    void *operator new(size_t size)
    {
        ShiAssert(size == sizeof(displayList));
        return MemAllocFS(pool);
    };
    void operator delete(void *mem)
    {
        if (mem) MemFreeFS(mem);
    };
    static void InitializeStorage()
    {
        pool = MemPoolInitFS(sizeof(displayList), 200, 0);
    };
    static void ReleaseStorage()
    {
        MemPoolFree(pool);
    };
    static MEM_POOL pool;
#endif
};

class drawPtrList
{
public:
    drawPtrList(void) {};
    ~drawPtrList(void) {};

    DrawableObject *drawPointer;
    float value;

    drawPtrList *prev;
    drawPtrList *next;

#ifdef USE_SH_POOLS
public:
    // Overload new/delete to use a SmartHeap fixed size pool
    void *operator new(size_t size)
    {
        ShiAssert(size == sizeof(drawPtrList));
        return MemAllocFS(pool);
    };
    void operator delete(void *mem)
    {
        if (mem) MemFreeFS(mem);
    };
    static void InitializeStorage()
    {
        pool = MemPoolInitFS(sizeof(drawPtrList), 10, 0);
    };
    static void ReleaseStorage()
    {
        MemPoolFree(pool);
    };
    static MEM_POOL pool;
#endif
};

class sfxRequest
{
#ifdef USE_SH_POOLS
public:
    // Overload new/delete to use a SmartHeap fixed size pool
    void *operator new(size_t size)
    {
        ShiAssert(size == sizeof(sfxRequest));
        return MemAllocFS(pool);
    };
    void operator delete(void *mem)
    {
        if (mem) MemFreeFS(mem);
    };
    static void InitializeStorage()
    {
        pool = MemPoolInitFS(sizeof(sfxRequest), 200, 0);
    };
    static void ReleaseStorage()
    {
        MemPoolFree(pool);
    };
    static MEM_POOL pool;
#endif
public:
    sfxRequest(void);
    ~sfxRequest(void);

    sfxRequest *next; // next in Q
    SfxClass *sfx; // pointer to sfx object
};

// this structire is used for the virtual cockpit instrumentation
typedef struct
{
    Canvas3D *vHUDrenderer;
    Canvas3D *vRWRrenderer;
    Canvas3D *vMACHrenderer;
    Canvas3D *vDEDrenderer;
    Canvas3D *vPFLrenderer;
} VirtualCockpitInfo;

//Wombat778 10-10-2003 Structure representing a 3d button for the 3d clickable cockpit
typedef struct
{
    Tpoint loc;
    float dist;
    InputFunctionType function;
    int sound;
    int mousebutton; //Wombat778 11-7-2003
    int buttonId; //Wombat778 03-06-04
} Button3D;

#define MAX_3D_BUTTONS 1000

//Wombat778 10-10-2003 Structure holding all the 3d buttons
typedef struct
{
    Button3D buttons[MAX_3D_BUTTONS];
    int numbuttons;
    int debugbutton;
    int clicked;  //Wombat778 10-15-2003 removed clickx and clicky because gxPos and gyPos should do just as well
} Button3DListType;


void PositandOrientSetData(float x, float y, float z, float pitch, float roll, float yaw,
                           Tpoint* simView, Trotation* viewRotation);


class OTWDriverClass
{
public:
    OTWDriverClass(void);
    ~OTWDriverClass(void);


#if NEW_SERVER_VIEWPOINT
    /** updates all server viewpoints. Duplicates are discarded. */
    void UpdateViewpoints();
    /** adds a viewpoint for this session so server can load terrain correctly for it. */
    void AddViewpoint(FalconSessionEntity *session);
    /** removes the viewpoint for this session. */
    void RemoveViewpoint(FalconSessionEntity *session);
#else
    void ServerSetviewPoint(void);//me123 alow the server to set the viewpoint even in ui
#endif

    void Enter(void); // Called when going from UI to SIM
    int  Exit(void); // Called when going from SIM to UI

    void Exec(void); // Main graphics loop (will go away with thread unification)
    void Cycle(void); // Set up and draw one frame of graphics

    void Reset3DParameters(void);
    void RenderFirstFrame(void);
    void RenderFrame(void);

public:
    enum {NumPopups = 4};
    enum OTWDisplayMode
    {
        ModeNone,
        ModeHud,
        Mode2DCockpit,
        Mode3DCockpit,
        ModePadlockF3,
        ModePadlockEFOV,
        ModeOrbit,
        ModeChase,
        ModeAirFriendly,
        ModeGroundFriendly,
        ModeAirEnemy,
        ModeGroundEnemy,
        ModeTarget,
        ModeWeapon,
        ModeSatellite,
        ModeIncoming,
        ModeFlyby,
        ModeTargetToSelf,
        ModeTargetToWeapon,
        ModeCount
    };

    Button3DListType Button3DList; //Wombat778 10-10-2003 list of 3d buttons for clickable cockpit;

    // this function can be used to determine if we're a 1st person or
    // other camera view
    inline BOOL DisplayInCockpit(void)
    {
        if (mOTWDisplayMode >= ModeHud and mOTWDisplayMode <= ModePadlockEFOV)
            return TRUE;

        return FALSE;
    }

    int numThreats, viewSwap, tgtId;
    int insertMode;
    int takeScreenShot;
    // Retro 7-8May2004
    // I want to have a function that disables all 2d text before taking a screenshot. To do this,
    // I put this little state machine (SM) here.. on pressing the 'takePrettyScreenShit' key, I set the
    // SM into the 'execute' mode, and note and deactivate the state of the labels. The drawFrontText()
    // routine will note the state of the SM, deactivate the other 2d stuff, take a screenshot, and advance
    // the state of the SM by one one (to CLEANUP).
    // In the next frame, the 2d stuff is active again, the drawFrontText()
    // routines reactivates the labels and put the SM into the OFF state. Now (and only now) the user may take
    // another prettyScreenShot. Yay 
    enum PrettyScreenShotState { OFF = 0, EXECUTE, CLEANUP } takePrettyScreenShot;
    int LabelState;
    // Retro 7-8May2004 end

    void SetOTWDisplayMode(OTWDisplayMode);
    OTWDisplayMode GetOTWDisplayMode(void);
    void CleanupDisplayMode(OTWDisplayMode);
    void SelectDisplayMode(OTWDisplayMode, OTWDisplayMode = ModeNone); //Wombat778 11-17-04
    void Select2DCockpitMode(bool = true); //Wombat778 added argument
    void Select3DCockpitMode(void);
    void SelectF3PadlockMode(void);

    void SelectExternal(void);
    void Cleanup2DCockpitMode(void);
    void Draw2DHud(void);

    float GetGroundLevel(float x, float y, Tpoint* normal = NULL);
    float GetApproxGroundLevel(float x, float y);
    void GetAreaFloorAndCeiling(float *floor, float *ceiling);
    int   GetGroundIntersection(euler* dir, vector* point);

    void SetGraphicsOwnship(SimBaseClass*);
    SimBaseClass* GraphicsOwnship() const
    {
        return otwPlatform.get();
    }
    SimBaseClass* GetTrackPlatform() const
    {
        return otwTrackPlatform.get();
    }
    void SetTrackPlatform(SimBaseClass *sb)
    {
        otwTrackPlatform.reset(sb);
    }

    enum ViewFindMode
    {
        NEXT_AIR_FRIEND  = 0,
        NEXT_GROUND_FRIEND,
        NEXT_AIR_ENEMY,
        NEXT_GROUND_ENEMY,
        NEXT_INCOMING,
        NEXT_TARGETING_ME,
        NEXT_WEAPON,
        NEXT_WINGMAN,
        NEXT_ENEMY // 2002-02-16 ADDED BY S.G. So we can differentiate from ModeTarget and ModeTargetToSelf to ModeGroundEnemy and ModeAirEnemy. That way, we can restrict ModeGroundEnemy to ground enemy and activate ModeAirEnemy
    };
    SimBaseClass *FindNextViewObject(FalconEntity *focusObj, SimBaseClass *currObj, ViewFindMode mode);

    // These functions are the fastest way to manipulate objects on the main graphics thread ONLY
    void InsertObject(DrawableObject*);
    void RemoveObject(DrawableObject*, int deleteObject = FALSE);
    void AttachObject(DrawableBSP*, DrawableBSP*, int s);
    void DetachObject(DrawableBSP*, DrawableBSP*, int s);
    void TrimTrail(DrawableTrail*, int);
    void AddTrailHead(DrawableTrail *dTrail, float, float, float);
    void AddTrailTail(DrawableTrail *dTrail, float, float, float);

    void AddToLitList(DrawableBSP*);
    void RemoveFromLitList(DrawableBSP*);
    void AddToNearList(DrawableObject*, float rSqrd);
    void FlushNearList(void);

    void CreateVisualObject(SimBaseClass*, float newScale = 1.0F);
    void CreateVisualObject(SimBaseClass*, int, Tpoint *simView, Trotation *viewRotation, float objectScale = 1.0F);
    void CreateVisualObject(SimBaseClass*, int, float newScale = 1.0F);

    // add an sfx request
    void AddSfxRequest(SfxClass *sfxptr);

    void ObjectSetData(SimBaseClass*, Tpoint*, Trotation*);

    // Give the camera the position and orientation of the object
    // that is its focus.
    void UpdateCameraFocus();

    void InsertObjectIntoDrawList(SimBaseClass*);
    ImageBuffer* OTWImage;
    HWND OTWWin;
    RViewPoint* GetViewpoint(void);
    void InitViewpoint(void);  // JB 010616
    void CleanViewpoint(void); // JB 010616
    int headMotion;
    // F4CSECTIONHANDLE* objectCriticalSection;
    //      void LockObject (void) {F4EnterCriticalSection (objectCriticalSection);};
    //      void UnLockObject (void) {F4LeaveCriticalSection (objectCriticalSection);};

    void SetScale(float newScale)
    {
        objectScale = newScale;
    };
    void SetDetail(int newLevel);
    float Scale(void)
    {
        return objectScale;
    };
    float todOffset;
    void  SetFOV(float horizontalFOV);
    float GetFOV(void);
    int CheckLOS(FalconEntity *pt1, FalconEntity *pt2);
    int CheckCloudLOS(FalconEntity *pt1, FalconEntity *pt2);
    int CheckCompositLOS(FalconEntity *pt1, FalconEntity *pt2);
    int IsActive(void)
    {
        return isActive;
    };
    void SetActive(int active)
    {
        isActive = active;
    };
    int IsShutdown(void)
    {
        return isShutdown;
    };
    void SetShutdown(int s)
    {
        isShutdown = s;
    };

    // PJW Routines
    void ScrollMessages();
    void ShowMessage(char* msg);
    void DisplayChatBox(void);
private:
    void DisplayInfoBar(void); // Retro 16Dec2003
    void DrawSubTitles(void); // Retro 16Dec2003
    void DisplayProfilerText(void); // Retro 21Dec2003
    bool ProfilerActive; // Retro 16/10/03
    bool DisplayProfiler; // Retro 16/10/03
    bool CameraChange; // RED - Camera Changed, preload All
public:
    void ToggleProfilerDisplay(void); // Retro 21Dec2003
    void ToggleProfilerActive(void); // Retro 21Dec2003
    bool IsProfilerDisplaying()
    {
        return DisplayProfiler;    // Retro 21Dec2003
    }
    void DisplayFrontText(void);
    void SetFrontTextFlags(long flags)
    {
        showFrontText = flags;
    }
    long GetFrontTextFlags()
    {
        return(showFrontText);
    }


    void ViewTiltUp(void);
    void ViewTiltDown(void);
    void ViewTiltHold(void);
    void ViewSpinLeft(void);
    void ViewSpinRight(void);
    void ViewSpinHold(void);
    void ViewReset(void);
    void ViewZoomIn(void);
    void ViewZoomOut(void);
    int  ViewRelativePanTilt(float, float); //Wombat778 10-08-2003
    void NVGToggle(void);
    void IDTagToggle(void);
    void CampTagToggle(void);
    void ViewStepNext(void);
    void ViewStepPrev(void);
    void TargetStepNext(void);
    void TargetStepPrev(void);
    void ToggleGLOC(void);
    void StepHeadingScale(void);
    void EndFlight(void);
    void ToggleBilinearFilter(void);
    void ToggleEyeFly(void);
    void ToggleShading(void);
    void ToggleHaze(void);
    void ToggleLocationDisplay(void);
    void ToggleAeroDisplay(void);
    void ToggleFlapDisplay(void);//TJL 11/09/03 On/Off Flap display
    void ToggleEngineDisplay(void); // Retro 1Feb2004
    void StartLocationEntry(void);
    void ToggleRoof(void);
    void ToggleFrameRate(void);
    void ScaleDown(void);
    void ScaleUp(void);
    void DetailDown(void);
    void DetailUp(void);
    void TextureUp(void);
    void TextureDown(void);
    void ToggleAlpha(void);
    void ToggleWeather(void);
    void ToggleAutoScale(void);
    void StringInDone(void);
    void StringBackspace(void);
    void StringDelete(void);
    void StringLeft(void);
    void StringRight(void);
    void StringInsert(void);
    void SetKeyCombo(void);
    void StringDefault(DWORD val);
    void RescaleAllObjects(void);
    void SetPopup(int popNum, int flag);
    void GlanceForward(void);
    void GlanceAft(void);
    void EyeFlyStateStep(void);
    void SetCameraPanTilt(float pan, float tilt)
    {
        eyePan = pan, eyeTilt = tilt;
    };
    void GetCameraPanTilt(float *pan, float *tilt)
    {
        *pan = eyePan, *tilt = eyeTilt;
    }; //Wombat778 3-19-04
    void DoSfxActiveList(void);
    void DoSfxDrawList(void);
    void ClearSfxLists(void);
    void SetOwnshipPosition(float x, float y, float z)
    {
        focusPoint.x = ownshipPos.x = x;
        focusPoint.y = ownshipPos.y = y;
        focusPoint.z = ownshipPos.z = z;
    };
    void SetResolution(int newRes)
    {
        otwResolution = newRes;
    };
    void ToggleActionCamera(void);
    void RunActionCamera(void);
    void ToggleThrustReverseDisplay(void);//Cobra

    void StartEjectCam(EjectedPilotClass *ejectedPilot, int startChaseMode = 0);
    void SetEjectCamChaseMode(EjectedPilotClass *ejectedPilot, int chaseMode);
    float DistanceFromCloudEdge(void);
    void ExitMenu(unsigned long i);
    void Timeout(void);
    void SetExitMenu(int newVal);
    void ChangeExitMenu(int newVal);
    int InExitMenu(void)
    {
        return exitMenuOn;
    };
    void StartExitMenuCountdown(void);
    void CancelExitMenuCountdown(void);
    int HandleMouseClick(long x, long y);

    float GetDoppler(float x, float y, float z, float dx, float dy, float dz);

    void SetEndFlightPoint(float x, float y, float z)
    {
        endFlightPoint.x = x;
        endFlightPoint.y = y;
        endFlightPoint.z = z;
        endFlightPointSet = TRUE;
        float Z = GetGroundLevel(x, y);

        if (z + 50.0f > Z)
            endFlightPoint.z = Z - 50.0f;

    }

    void SetEndFlightVec(float x, float y, float z)
    {
        endFlightVec.x = x;
        endFlightVec.y = y;
        endFlightVec.z = z;
    }

    void SetChaseAzEl(float az, float el)
    {
        chaseAz = az;
        chaseEl = el;
    }

private:
    // Require to keep track of things that need time of day updates
    drawPtrList *litObjectRoot;

    // Required to keep track of the special case drawables which are closer than our object of interest
    drawPtrList *nearObjectRoot;

    // special effects stuff
    sfxRequest *sfxRequestRoot;
    sfxRequest *sfxActiveRoot;

    float initialTilt;
    float snapDir;
    int stopState;

    int popupHas[NumPopups];
    int otwResolution;
    Tpoint ownshipPos;
    float viewTiltForMask;//, hudHalfAngle[2];
    float objectScale;
    void RemoveObjectFromDrawList(SimBaseClass*);
    void FindNewOwnship(void);
    void UpdateEntityLists(void);
    void DoPopUps(void);

    void SetInternalCameraPosition(float dT);
    void SetExternalCameraPosition(float dT);
    void SetFlybyCameraPosition(float dT);
    void SetEyeFlyCameraPosition(float dT);

    // Retro 23Dec2003 Start
    struct CamDisplacement
    {
        float direction; // are we going left or righ ? up or down ?
        float maxDispl; // how far are we going ?
        float scale; // how fast are we going ?
        float Displ;
        CamDisplacement(float dir, float max, float scal)
        {
            direction = dir;
            maxDispl = max;
            scale = scal;
            Displ = 0.F;
        }
    };

    CamDisplacement *xDir, *yDir, *zDir;
    float cameraDisplacementRate; // this should be a const actually, but I dunno how to change it..

    void DisplaceTheCamera(float dT); // Retro 23Dec2003
    void ReInitDisplacement(CamDisplacement* theDisp); // Retro 23Dec2003
    bool displaceCamera;
public:
    void toggleDisplaceCamera(void); // Retro 23Dec2003
    Tpoint OTWDriverClass::GetEyePosition(void); // COBRA - RED -

private:
    // Retro 23Dec2003 End
    void UpdateVehicleDrawables(void);
    static void TimeUpdateCallback(void *self);
    void UpdateAllLitObjects(void);
    void UpdateOneLitObject(drawPtrList *entry, float lightLevel);
    void BuildExternalNearList(void);

    void DrawIDTags(void);
    void GetUserPosition(void);
    void ShowPosition(void);
    void ShowAerodynamics(void);
    void ShowFlaps(void);
    void ShowEngine(void); // Retro 1Feb2004
    void ShowCatMessage(void); // RV - Biker - Show AC is attached to catapult
    void EyeFly(void);
    void FindNearestBuilding(void);
    void TakeScreenShot(void);
    void CreatePreRequestedObjects(void);
    void AddToPreRequestList(SimBaseClass*);
    void AddToPreRequestList(SimBaseClass*, int);
    void RemoveFromPreRequestList(SimBaseClass*);
    void CreateWeaponObjects(void);
    void DrawExitMenu(void);
    RViewPoint* viewPoint;
    void ShowThrustReverse(void);//Cobra




    int isActive;
    int isShutdown;
    int doWeather;
    int viewStep;
    int tgtStep;
    int doGLOC;
    int autoScale;
    int showPos;
    int showAero;
    //TJL 11/09/03 Flaps On/Off
    int showFlaps;
    int showEngine; // Retro 1Feb2004
    int getNewCameraPos;
    int eyeFly;
    int weatherCmd;
    int exitMenuOn;
    int showThrustReverse; //Cobra

    // Ejection camera stuff.
    int ejectCam;
    int prevChase;

    float padlockWindow[3][4];
    float eyePan, eyeTilt, eyeHeadRoll;
    float chaseAz;
    float chaseEl;
    float chaseRange;
    float azDir;
    float elDir;
    float slewRate;
    Tpoint chaseCamPos; // desired position for chase Camera
    float  chaseCamRoll; // current roll for chase camera

    void DrawTracers(void);
    float currentFPS;//Cobra

private:
    OTWDisplayMode mOTWDisplayMode;
    // track platform is used by some of the view modes
    // sfr: container for simbase
    typedef VuBin<SimBaseClass> SimBaseBin;
    // sfr: changed to smartpointers
    //SimBaseClass* otwTrackPlatform;
    //SimBaseClass* otwPlatform;
    //SimBaseClass* lastotwPlatform;
    SimBaseBin otwTrackPlatform;
    SimBaseBin otwPlatform;
    SimBaseBin lastotwPlatform;
    Render2D* mfdVirtualDisplay;
    FalconEntity* flyingEye;
    long showFrontText; // flags for drawing text in front of EVERYTHING else
    VU_TIME textTimeLeft[MAX_CHAT_LINES];
    char textMessage[MAX_CHAT_LINES][MAX_CHAT_LENGTH]; // 5 lines MAX... before scrolling up
    float e1;
    float e2;
    float e3;
    float e4;
    unsigned long frameStart, lastFrame, frameTime, fcount;
    Tpoint focusPoint;

public:
    Tpoint cameraPos;
    Trotation cameraRot;
    Tpoint cameraVel;  // MLR 12/1/2003 - Needed in sound code for listener velocity
    float GetFPS()
    {
        return (currentFPS);
    };//Cobra

private:
    Tpoint pilotEyePos;  // MLR 12/1/2003 - Support placement of pilots skull

    bool SkipSwap; // RED - Skip swapping, this is 1st frame...
    // these vars are used when the sim is ending
    Tpoint endFlightPoint;
    Tpoint endFlightVec;
    unsigned long endFlightTimer;
    unsigned long flybyTimer;
    unsigned long exitMenuTimer;
    BOOL endFlightPointSet;

    BOOL actionCameraMode;
    unsigned long actionCameraTimer;

    //Wombat778 11-18-04
    void RunNew2DTrackIR(float, float);

    int HybridPitModeEnabled;
    void RunHybridPitMode(float, float);

public:
    void ToggleHybridPitMode();
    int GetHybridPitMode();

private:

    //Wombat778 end
    BOOL drawInfoBar; // Retro 16Dec2003
    BOOL drawSubTitles; // Retro 16Dec2003

public: // Retro 16Dec2003
    void ToggleInfoBar(); // Retro 16Dec2003
    void ToggleSubTitles(); // Retro 16Dec2003
    void DisplayAxisValues(); // Retro 1Jan2004

private: // Retro 16Dec2003
    // For external camera views
    void DrawExternalViewTarget(void);
    // ---------------------------------
    // The following stuff is maintainted by VWF
    // ---------------------------------

public:
    RenderOTW* renderer;
    Trotation ownshipRot;
    Trotation headMatrix;
    Tpoint headOrigin, headPan;

    // ---------------------------------
    // Menu Pointers
    // ---------------------------------
    MenuManager* pMenuManager;
    //

    // ---------------------------------
    // 2D Cockpit Pointers
    // ---------------------------------
    ImageBuffer* cockpitImage;
    CockpitManager* pCockpitManager;
    CockpitManager* pPadlockCPManager;
    //

private:

    //JAM 10May04
    BOOL bVCockZBuffering;

    // [0][?] daycolor [?][0] padlock background
    // [1][?] nvgcolor [?][1] padlock liftline
    // [?][2] padlock viewport box 3 sides
    // [?][3] padlock viewport top
    // [?][4] padlock zero level tickmark
    // [?][5] vcock primary needle color
    // [?][6] vcock secondary needle color
    // [?][7] vcock ded color
    // [?][7] vcock rwr color
    long pVColors[2][11];


    // ---------------------------------
    // Camera Transformations
    // ---------------------------------
    float headx;
    float heady;
    float headz;
    void CalculateHeadRoll(float, Tpoint*, Tpoint*, Tpoint*);
    void BuildHeadMatrix(int, int, float, float, float);

public :

    // ---------------------------------
    // Padlocking Enums
    // ---------------------------------
    enum PadlockPriority {PriorityNone, PriorityAA, PriorityAG, PriorityMissile};
private:
    enum GlanceDirection {GlanceNone, GlanceNose, GlanceTail};
    enum SnapStatus {PRESNAP, TRACKING, SNAPPING, POSTSNAP};
    //

    // ---------------------------------
    // General Padlocking Member Variables
    // ---------------------------------
    SimBaseClass* mpPadlockCandidate;
    // MN
    VU_TIME sunlooktimer;
    // 2000-11-15 MADE PUBLIC BY S.G. SO FindAircraftTarget CAN RETURN THE PADLOCKED OBJECT AS WELL

public:
    SimBaseClass* mpPadlockPriorityObject;
    SimObjectType* simObjectPtr; // moved from function into the class - used to check if pilot can "see" the padlocked object

private:
    VU_ID             mPadlockCandidateID;
    float mPadlockTimeout;
    float mTDTimeout;
    float PadlockOccludedTime;
    PadlockPriority padlockPriority;
    BOOL mIsSlewInit;
    float mSlewPStart;
    float mSlewTStart;
    BOOL mObjectOccluded;

public:

    // ---------------------------------
    // General Padlocking Routines
    // ---------------------------------
    // 2001-01-29 ADD BY S.G. SO HMS HAS A TARGET TO LOOK AT
    void SetmpPadlockPriorityObject(SimBaseClass* newObject);
    // END OF ADDED FUNCTION
    void Padlock_SetPriority(PadlockPriority newPriority);
    long GetLiftLineColor(void)
    {
        return liftlinecolor;
    }

private:
    void Padlock_FindNextPriority(BOOL);
    void Padlock_FindRealisticPriority(BOOL);
    void Padlock_FindEnhancedPriority(BOOL);

    BOOL Padlock_ConsiderThisObject(SimBaseClass*, BOOL, float, float, float);
    BOOL Padlock_DetermineRelativePriority(SimBaseClass*, float, BOOL, SimBaseClass*, float, BOOL);
    void Padlock_CheckPadlock(float);
    BOOL Padlock_CheckOcclusion(float, float);
    int Padlock_RankAGPriority(SimBaseClass*, BOOL);
    int Padlock_RankAAPriority(SimBaseClass*, BOOL);
    int Padlock_RankNAVPriority(SimBaseClass*, BOOL);
    void Padlock_DrawSquares(BOOL);
    //

    // ---------------------------------
    // Variables for F3Padlock Diagram
    // ---------------------------------
    BOOL mDoSidebar;

    GlanceDirection padlockGlance;
    SnapStatus snapStatus;

    float mMaxPadTilt;
    float mMinPadTilt;
    float mMaxPadPan;
    float mMinPadPan;
    float mPadRho;
    float mMaxPadC;

    float mZeroLineRadius;
    float m30LineRadius;
    float m45LineRadius;
    float m60LineRadius;
    float mMaxTiltLineRadius;

    float mWedgeTipX;
    float mWedgeTipY;

    float mWedgeLeftX;
    float mWedgeY;

    float hBlindArc;
    float vBlindArc;
    float blindA;
    float blindB;
    //

    // ---------------------------------
    // FreeFalcon 3 Padlock Slaming Variables
    // ---------------------------------

    float mPrevPRate;
    float mPrevPError;
    float mPrevTRate;
    float mPrevTError;

    // ---------------------------------
    // FreeFalcon 3 Padlock Functions
    // ---------------------------------


    int  PadlockF3_SlewCamera(float, float, float, float, float, float, float);
    int  PadlockF3_SlamCamera(float*, float, float*, float*, float, float*, float, float, float);

    // 2000-11-13 MODIFIED BY S.G. NOW RETURNS AN INT
    //void PadlockF3_SetCamera (float);
    int PadlockF3_SetCamera(float);
    void PadlockF3_InitSidebar(void);
    void PadlockF3_DrawSidebar(float, float, float, RenderOTW*);
    void PadlockF3_MapAnglesToSidebar(float, float, float, float, float*, float*);
    void PadlockF3_CalcCamera(float);
    void PadlockF3_Draw(void);

    // ---------------------------------
    // EFOV Padlock Functions
    // ---------------------------------
    void PadlockEFOV_Draw(void);
    void PadlockEFOV_DrawBox(SimBaseClass*, float, float, float, float, float, float);

    // ---------------------------------
    // Virtual Cockpit variables
    // ---------------------------------
    float vBoresightY; // ASSO:
    VirtualCockpitInfo vcInfo; // ASSO: old canvas

    Render2D* vHUDrenderer; // ASSO:
    Render2D* vRWRrenderer; // ASSO:
    Render2D* vDEDrenderer; // ASSO:
    Render2D* vPFLrenderer; // ASSO:

    DrawableBSP* vrCockpit;
    int vrCockpitModel[4]; // the models to use. cockpit, df cockpit, f15, damaged f16
    int                  mUseHeadTracking;
    std::vector<VDial*> mpVDials;
    long liftlinecolor;

    // ---------------------------------
    // Virtual Cockpit Functions
    // ---------------------------------
    void VCock_Glance(float);
    void VCock_GiveGilmanHead(float);
    void VCock_RunNormalMotion(float);
    void VCock_CheckStopStates(float);
    void VCock_DrawThePit(void);
    void VCock_HeadCalc(void);
    void VCock_Exec(void);
    void VCock_Cleanup(void);
    void VCock_ParseVDial(FILE *fp);
    bool VCock_Init(int eCPVisType, TCHAR* eCPName, TCHAR* eCPNameNCTR);
    bool VCock_SetCanvas(char **plinePtr, Canvas3D **canvaspp); // ASSO: old cancas
    bool VCock_SetRttCanvas(char** plinePtr, Render2D** canvaspp, int dev); // ASSO: new RTT canvas
    float MoveByRate(float oldval, float newval, float rate); //ATARIBABY move to target value by defined rate (Borrowed MOVEDOF from surface.cpp);

    Tpoint PitTurbulence;
    DWORD BigLoadTimeOut;
    // ---------------------------------
    // Padlock and Virtual Cock Access
    // ---------------------------------

public:
    DrawableBSP *GetVirtualCockpit()
    {
        return vrCockpit;
    }
    void ToggleSidebar(void);
    void SetHeadTracking(int flag)
    {
        mUseHeadTracking = flag;
    }
    int IsHeadTracking(void)
    {
        return mUseHeadTracking;
    }

    // ---------------------------------
    // End VWF Stuff
    // ---------------------------------

    //
    // ---------------------------------
    // Clickable Cockpit Functions //Wombat778 10-10-2003
    // ---------------------------------
    bool Button3D_Init(int eCPVisType, TCHAR* eCPName, TCHAR* eCPNameNCTR);



    // ---------------------------------
    // Splash Screen Stuff
    // ---------------------------------
    void SetupSplashScreen(void);
    void CleanupSplashScreen(void);
    void SplashScreenUpdate(int frame);
    void ShowSimpleWaitScreen(char *name);

private:
    F4CSECTIONHANDLE* cs_update; // JB 010616
    BOOL bKeepClean; // JB 010616

#if NEW_SERVER_VIEWPOINT
    // mutex for viewpoint map
    F4CSECTIONHANDLE *vmMutex;
    /** sfr: these are the viewpoints a server must keep for all players which are ingame so it
    * can load terrain stuff and make ground computations correctly
    */
    std::map < VuBin<FalconSessionEntity>, TViewPoint*> viewpointMap;
#endif


public:
    int GetGroundType(float x, float y);   // JB 010616
    void CockAttachWeapons(void);
    void CockDetachWeapons(void);


};

extern OTWDriverClass OTWDriver;
//**************************************************************************************************
// COBRA - RED - Updated by the OTW Observer Position and Rotation
// These values are Public to be use all over the sources
extern Tpoint ObserverPosition;
extern float ObserverYaw, ObserverPitch, ObserverRoll;
// COBRA - RED - End
//**************************************************************************************************


#endif
