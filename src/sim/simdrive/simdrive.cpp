#include "falcsess.h"
#include "f4thread.h"
#include "fsound.h"
#include "soundfx.h"
#include "f4vu.h"
#include "simveh.h"
#include "simfile.h"
#include "simio.h"
#include "PilotInputs.h"
#include "team.h"
#include "camp2sim.h"
#include "ClassTbl.h"
#include "CampLib.h"
#include "otwdrive.h"
#include "simdrive.h"
#include "falcmesg.h"
#include "f4error.h"
#include "initdata.h"
#include "simfiltr.h"
#include "fcc.h"
#include "datadir.h"
#include "dogfight.h"
#include "iaction.h"
#include "unit.h"
#include "division.h"
#include "objectiv.h"
#include "OwnResult.h"
#include "MsgInc/SimTimingMsg.h"
#include "ThreadMgr.h"
#include "mvrdef.h"
#include "aircrft.h"
#include "acmi/src/include/acmirec.h"
#include "simfeat.h"
#include "feature.h"
#include "PlayerOp.h"
#include "resource.h"
#include "sinput.h"
#include "commands.h"
#include "cpmanager.h"
#include "inpFunc.h"
#include "tacan.h"
#include "navsystem.h"
#include "missile.h"
#include "bomb.h"
#include "chaff.h"
#include "flare.h"
#include "debris.h"
#include "object.h"
#include "sfx.h"
#include "ground.h"
#include "guns.h"
#include "Persist.h"
#include "Find.h"
#include "TimerThread.h"
#include "rwr.h" // Goes once the RWR data is in the class table (if that ever happens)
#include "falcsnd/voicemanager.h"
#include "Graphics/Include/draw2d.h"
#include "Graphics/Include/drawtrcr.h"
#include "Graphics/Include/drawgrnd.h"
#include "Graphics/Include/drawbldg.h"
#include "Graphics/Include/drawbsp.h"
#include "Graphics/Include/drawbrdg.h"
#include "Graphics/Include/drawovc.h"
#include "Graphics/Include/drawplat.h"
#include "Graphics/Include/drawrdbd.h"
#include "Graphics/Include/drawsgmt.h"
#include "Graphics/Include/drawpuff.h"
#include "Graphics/Include/drawshdw.h"
#include "Graphics/Include/drawpnt.h"
#include "Graphics/Include/drawguys.h"
#include "Graphics/Include/drawpole.h"
#include "airframe.h"
#include "digi.h"
#include "evtparse.h"
#include "voicecomunication/voicecom.h"//me123

// KCK added include stuff
#include "CmpClass.h"
#include "weather.h"
#include "falcsess.h"
#include "Campaign.h"
#include "simloop.h"
#include "campwp.h"
#include "sms.h"
#include "hardpnt.h"
#include "GameMgr.h"
#include "digi.h"
#include "helo.h"
#include "hdigi.h"
#include "radar.h"
#include "fault.h"
#include "simvudrv.h"
#include "listadt.h"
#include "UI/INCLUDE/tac_class.h"
#include "UI/INCLUDE/te_defs.h"
#include "msginc/atcmsg.h"
//me123
#include "flight.h"
#include "simbase.h"
#include "codelib/tools/lists/lists.h"
#include "acmi/src/include/acmitape.h"

#ifdef USE_SH_POOLS
MEM_POOL gTextMemPool;
MEM_POOL gObjMemPool;
MEM_POOL gReadInMemPool;
extern MEM_POOL gTacanMemPool;
#endif

void GraphicsDataPoolInitializeStorage(void);
void GraphicsDataPoolReleaseStorage(void);

extern void SavePersistantList(char* scenario);
extern C_Handler *gMainHandler;
extern int gNumWeaponsInAir;
extern HWND mainMenuWnd;
extern int FileVerify(void);

extern ulong gBumpTime;
extern int gBumpFlag;
// Control Defines
//#define DISPLAY_BLOW_COUNT

// Exported Globals
SimulationDriver SimDriver;

// Imported Variables
extern void CalcTransformMatrix(SimBaseClass* theObject);

// Static Variables
#ifdef DISPLAY_BLOW_COUNT
static int BlowCount = 0;
#endif
static int debugging = FALSE;
int EndFlightFlag = FALSE;
int gOutOfSimFlag = TRUE;

void ReadAllAirframeData(void);
void ReadAllMissileData(void);
void ReadAllBombData(void); // 2003-11-10 MLR for exported bomb data
void ReadDigitalBrainData(void);

void FreeAllAirframeData(void);
void FreeAllMissileData(void);
void FreeAllBombData(void); // 2003-11-10 MLR for exported bomb data
void FreeDigitalBrainData(void);

void JoystickStopAllEffects(void);

static unsigned int __stdcall SimExecWrapper(void* myself);
//static void ProximityCheck (SimBaseClass* testObj);

typedef struct NewRequestListItem
{
    VU_ID requestId;
    struct NewRequestListItem* next;
} NewRequestList;

static NewRequestList* requestList = NULL;

float SimLibLastMajorFrameTime;


#ifdef CHECK_PROC_TIMES
ulong gMaxAircraftProcTime = 0;
ulong gMaxGndProcTime = 0;
ulong gMaxATCProcTime = 0;
ulong gLastAircraftProcTime = 0;
ulong gLastGndProcTime = 0;
ulong gLastATCProcTime = 0;
ulong gAveAirProcTime = 0;
ulong gAveGndProcTime = 0;
int numAircraft = 0;
int numGrndVeh = 0;
ulong gLastSoundFxTime = 0;
ulong gMaxSoundFxTime = 0;
ulong gLastSFXTime = 0;
ulong gMaxSFXTime = 0;
ulong gILSTime = 0;
ulong gMaxILSTime = 0;
ulong gOtherMaxTime = 0;
ulong gOtherLastTime = 0;
ulong gAveOtherProcTime = 0;
int numOther = 0;
ulong AllObjTime = 0;
static ulong gWhole = 0;

extern ulong gACMI;
extern ulong gSpecCase;
extern ulong gTurret;
extern ulong gLOD;
extern ulong gFire;
extern ulong gMove;
extern ulong gThink;
extern ulong gTarg;
extern ulong gRadar;
extern ulong gFireCont;
extern ulong gSimVeh;
extern ulong gSFX;

extern ulong gCommChooseTarg;
extern ulong gSelNewTarg;
extern ulong gRadarTarg;
extern ulong gConfirmTarg;
extern ulong gDoWeapons;
extern ulong gSelWeapon;

extern ulong gSelBestWeap;
extern int numCalls;

extern ulong gWeapRng;
extern int numWRng;
extern ulong gWeapHCh;
extern int numWHCh;
extern ulong gWeapScore;
extern int numWeapScore;
extern ulong gCanShoot;
extern int numCanShoot;

extern ulong gRotTurr;
extern ulong gTurrCalc;
extern ulong gKeepAlive;
extern ulong gFireTime;

extern ulong terrTime;
#endif

//MI
extern FireControlComputer::FCCMasterMode playerLastMasterMode;
extern FireControlComputer::FCCSubMode playerLastSubMode;
// ==========================================================

SimulationDriver::SimulationDriver(void)
{
    SimLibFrameElapsed = static_cast<float>(vuxGameTime);
    SimLibElapsedTime = static_cast<SIM_ULONG>(SimLibFrameElapsed);
    UPDATE_SIM_ELAPSED_SECONDS; // COBRA - RED - Scale Elapsed Seconds

    SimLibFrameCount = 0;
    objectList = NULL;
    combinedList = NULL;
    combinedFeatureList = NULL;
    ObjsWithNoCampaignParentList = NULL;
    featureList = NULL;
    campUnitList = NULL;
    campObjList = NULL;
    motionOn = TRUE;
    playerEntity = NULL;
    doEvent = FALSE;
    doFile = FALSE;
    doExit = FALSE;
    doGraphicsExit = FALSE;
    facList = NULL;
    tankerList = NULL;
    atcList = NULL;
    avtrOn = FALSE;
    lastRealTime = 0;
    inCycle = 0; // MLR 1/2/2005 -
}

SimulationDriver::~SimulationDriver(void)
{
}


// KCK: Startup is called on FreeFalcon startup before the loop calling Cycle is started.
void SimulationDriver::Startup(void)
{
    SetFrameDescription(50, 1);
    motionOn = TRUE;

    // Prep database filtering
    objectList = NULL;
    campUnitList = NULL;
    campObjList = NULL;
    combinedList = NULL;
    combinedFeatureList = NULL;
    ObjsWithNoCampaignParentList = NULL;

    //Prep Object Data
    // Check file integrity
    FileVerify();

    SimMoverDefinition::ReadSimMoverDefinitionData();
    ReadDigitalBrainData();
    ReadAllMissileData();
    ReadAllBombData(); // 2003-11-10 MLR for exported bomb data
    ReadAllAirframeData();
    ReadAllRadarData(); // JPO addition to read .dat files

    AllSimFilter allSimFilter;
    SimDynamicTacanFilter dynamicTacanFilter;

    SimObjectFilter objectFilter;
    objectList = new FalconPrivateOrderedList(&objectFilter);
    objectList->Register();

    SimFeatureFilter featureFilter;
    featureList = new FalconPrivateOrderedList(&featureFilter);
    featureList->Register();


    UnitFilter unitFilter(0, 1, 0, 0);
    campUnitList = new FalconPrivateOrderedList(&unitFilter);
    campUnitList->Register();

    ObjFilter objectiveFilter(0);
    campObjList = new FalconPrivateOrderedList(&objectiveFilter);
    campObjList->Register();


    facList = new FalconPrivateList(&allSimFilter);
    facList->Register();

    SimAirfieldFilter airbaseFilter;
    atcList = new VuFilteredList(&airbaseFilter);
    atcList->Register();

    tankerList = new VuFilteredList(&dynamicTacanFilter);
    tankerList->Register();


    CombinedSimFilter combinedFilter;
    combinedList =  new FalconPrivateOrderedList(&combinedFilter);
    combinedList->Register();

    combinedFeatureList = new FalconPrivateOrderedList(&combinedFilter);
    combinedFeatureList->Register();

    ObjsWithNoCampaignParentList = new FalconPrivateList(&allSimFilter);
    ObjsWithNoCampaignParentList->Register();

    EndFlightFlag = FALSE;

#ifndef NO_TIMER_THREAD
#if MF_DONT_PROCESS_DELETE or VU_USE_ENUM_FOR_TYPES
    FalconMessageFilter messageFilter(FalconEvent::SimThread, 0);
#else
    FalconMessageFilter messageFilter(FalconEvent::SimThread, VU_DELETE_EVENT_BITS);
#endif
    vuThread = new VuThread(&messageFilter, F4_EVENT_QUEUE_SIZE);
#endif

    last_elapsedTime = 0;
    lastRealTime = vuxGameTime;
    lastFlyState = -1;
    curFlyState = 0;
}


// SCR:  Called at exit of application after the loop calling "Cycle" has gone away.
void SimulationDriver::Cleanup(void)
{
    FreeAllAirframeData();
    FreeAllMissileData();
    FreeAllBombData(); // 2003-11-10 MLR for exported bomb data
    FreeDigitalBrainData();
    SimMoverDefinition::FreeSimMoverDefinitionData();

    objectList->Unregister();
    delete objectList;
    featureList->Unregister();
    delete featureList;
    campUnitList->Unregister();
    delete campUnitList;
    campObjList->Unregister();
    delete campObjList;

    atcList->Unregister();
    delete atcList;
    facList->Unregister();
    delete facList;
    tankerList->Unregister();
    delete tankerList;

    combinedList->Unregister();
    delete combinedList;

    combinedFeatureList->Unregister();
    delete combinedFeatureList;

    ObjsWithNoCampaignParentList->Unregister();
    delete ObjsWithNoCampaignParentList;

    ObjsWithNoCampaignParentList = NULL;
    combinedFeatureList = NULL;
    combinedList = NULL;
    tankerList = NULL;
    objectList = NULL;
    featureList = NULL;
    campUnitList = NULL;
    campObjList = NULL;
#ifndef NO_TIMER_THREAD
#if CAP_DISPATCH
    vuThread->Update(-1);
#else
    vuThread->Update();
#endif
    delete vuThread;
    vuThread = NULL;
#endif
}


// SCR:  To be called each time we go from UI to Sim (once code is unified)
void SimulationDriver::Enter(void)
{
    nextATCTime = 0;
    OwnResults.ClearData();
    gNumWeaponsInAir = 0;

    if (SimDriver.doEvent)
    {
        F4EventFile = OpenCampFile("lastflt", "acm", "wb");
    }
    else
    {
        F4EventFile = NULL;
    }

    if (IO.Init((char *)"joy1.dat") not_eq SIMLIB_OK)
    {
        MonoPrint("No Joystick Connected\n");
    }

    OTWDriver.SetActive(TRUE);
}

// Called each time we go from Sim to UI
void SimulationDriver::Exit(void)
{
    // sfr: this cannot happen in MP
#define NO_KILL_ON_EXIT 1
#if not NO_KILL_ON_EXIT
    SimBaseClass *theObject, *nextObject;
    VuListIterator objectWalker(objectList);

    // Kill All remaining awake sim Entities
    for (
        theObject = static_cast<SimBaseClass*>(objectWalker.GetFirst());
        theObject not_eq NULL;
        theObject = nextObject
    )
    {
        nextObject = static_cast<SimBaseClass*>(objectWalker.GetNext());
        theObject->SetDead(TRUE);
    }

#endif

    if (F4EventFile)
    {
        fclose(F4EventFile);
    }

    F4EventFile = NULL;

    if (gACMIRec.IsRecording())
    {
        gACMIRec.StopRecording();
        SimDriver.SetAVTR(FALSE);
    }

    F4SoundFXEnd();
    F4SoundStop();
}

void SimulationDriver::Cycle()
{
    //START_PROFILE("SIMCYCLE_BEGIN");
    //VuListIterator objectWalker (objectList);
    static int runFrame, runGraphics = false;
    long elapsedTime;
    float gndz;

    // Catch up for time elapsed
    //ShiAssert(lastRealTime <= vuxGameTime);
    if (lastRealTime > vuxGameTime)
    {
        lastRealTime = vuxGameTime;
    }

    elapsedTime = vuxGameTime - lastRealTime;
    curFlyState = FalconLocalSession->GetFlyState();
    RefreshVoiceFreqs();//me123

    if ((elapsedTime >= 10) and (gameCompressionRatio))
    {
        // Check if the graphics are runnning and read inputs, if so.
        if (curFlyState == FLYSTATE_FLYING or curFlyState == FLYSTATE_DEAD)
        {
            UserStickInputs.Update();
            runGraphics = true;
        }
        else
        {
            runGraphics = false;
        }

        SimLibLastMajorFrameTime = SimLibMajorFrameTime;
        SimLibMajorFrameTime = max(0.01F, ((float)(elapsedTime)) / SEC_TO_MSEC);

        for (SimLibMinorPerMajor = 1; SimLibMinorPerMajor < 100; SimLibMinorPerMajor++)
        {
            SimLibMinorFrameTime = SimLibMajorFrameTime / (float)SimLibMinorPerMajor;

            if (SimLibMinorFrameTime <= 0.05f)
            {
                break;
            }
        }

        runFrame = true;

        SimLibMajorFrameRate = 1.0F / SimLibMajorFrameTime;
        SimLibMinorFrameRate = 1.0F / SimLibMinorFrameTime;

        lastRealTime = vuxGameTime;
    }
    else
    {
        runFrame = false;
    }

    lastFlyState = curFlyState;
    last_elapsedTime = elapsedTime;

    // doFile is a flag that tells us a state change for ACMI recording
    // was made.  If TRUE, we need to toggle ACMI recording here.
    // we need to do this here because when recording starts it must
    // walk thru all objects (via a callback to SimDriver::InitACMIRecord)
    // and this must be thread safe
    //
    // SCR:  6/18/98  Can this go to a better place once the sim/graphics threads
    //                are merged?
    if (doFile)
    {
        gACMIRec.ToggleRecording();
        doFile = FALSE;
    }

#if not NEW_SERVER_VIEWPOINT

    //me123 the following function is needed for host to send updates while in the ui
    //with the new mp code.
    if (vuLocalSessionEntity and vuLocalGame and vuLocalGame->IsLocal())
    {
        // we are hosting a game
        VuGameEntity *game = vuLocalSessionEntity->Game();
        VuSessionsIterator Sessioniter(game);
        VuSessionEntity*   sess;
        sess = Sessioniter.GetFirst();
        int flying = FALSE;

        //FLYSTATE_IN_UI
        while (sess and not flying)
        {
            // sfr: why get from DB???
            // this is fucking hack. They get from DB because UI can be closing while this is runnig
            // definitely NOT SAFE
            //if (((FalconSessionEntity*)vuDatabase->Find(sess->Id()))->GetFlyState () not_eq FLYSTATE_IN_UI)
            if (((FalconSessionEntity*)sess)->GetFlyState() not_eq FLYSTATE_IN_UI)
            {
                flying = TRUE;
            }
            else
            {
                sess = Sessioniter.GetNext();
            }
        }

        if (
            flying and not OTWDriver.IsActive() and FalconLocalSession->GetFlyState() == FLYSTATE_IN_UI and 
 not curFlyState and not doGraphicsExit and not doExit and not TheCampaign.IsSuspended()
        )
        {
            Enter();
        }
        else if (
            flying and OTWDriver.IsActive() and FalconLocalSession->GetFlyState() == FLYSTATE_IN_UI and 
 not curFlyState and not doGraphicsExit and not doExit and not TheCampaign.IsSuspended()
        )
        {
            if ((sess->CameraCount() > 0) and OTWDriver.GetViewpoint())
            {
                VuEntity *e = sess->GetCameraEntity(0);
                OTWDriver.SetOwnshipPosition(e->XPos(), e->YPos(), e->ZPos());
                OTWDriver.ServerSetviewPoint();
            }
        }
    }

#endif
    //STOP_PROFILE("SIMCYCLE_BEGIN");

    //START_PROFILE("SIMCYCLE_RUNFRAME");
    if (runFrame)
    {
#if NEW_SERVER_VIEWPOINT
        // update all players viewpoints
        FalconGameEntity *game = FalconLocalGame;

        if (game and game->IsLocal())
        {
            OTWDriver.UpdateViewpoints();
        }

#endif

        // Update special effects
        // sfr: shouldnt this be inside OTWDriver.Cycle??
        OTWDriver.DoSfxActiveList();

        // Do each entity's thinking
        inCycle = 1; // MLR 1/2/2005 -
        cycleAgain = 1; // MLR 1/2/2005 -
        int processObjsAddedInCycle = 0;
        //do
        {
#define NO_CAMP_CRITICAL 1
#if NO_CAMP_CRITICAL
#else
            F4ScopeLock cl(campCritical);
#endif
            cycleAgain = 0; // MLR 1/2/2005 -
            //VuListIterator objectWalker(combinedList);
            // sfr: back to object list only...
            VuListIterator objectWalker(objectList);

            for (
                SimBaseClass *theObject = (SimBaseClass*)objectWalker.GetFirst(), *next = NULL;
                theObject not_eq NULL;
                theObject = next
            )
            {
                next = static_cast<SimBaseClass*>(objectWalker.GetNext());

                // sfr: remove object during list scan
                if (theObject->IsSetRemoveFlag())
                {
                    vuDatabase->Remove(theObject);
                }
                else if ( not theObject->IsAwake())
                {
                    SimDriver.RemoveFromObjectList(theObject);
                    continue;
                }

                // MLR 1/2/2005 - Since objects are added to the DB during the cycle,
                // some object miss thier initial Exec() call
                // this causes things like bombs bitand missiles launching from behind the a/c.
                // SimBaseObject (and derived) entities that are created during
                // Cycle will be marked for "post processing" so
                // that they are Exec()ed for the current frame.
                // There is code in ::SimBaseClass that sets the flag FELF_ADDED_DURING_SIMDRIVER_CYCLE.
                // We need to make sure we clear it here.
                if (
 not processObjsAddedInCycle or
                    (processObjsAddedInCycle and theObject->IsSetFELocalFlag(FELF_ADDED_DURING_SIMDRIVER_CYCLE))
                )
                {
                    theObject->UnSetFELocalFlag(FELF_ADDED_DURING_SIMDRIVER_CYCLE);

                    if (theObject->EntityDriver())
                    {
                        if ( not (
                                (theObject->IsSetFalcFlag(FEC_PLAYER_ENTERING)) and 
                                (theObject->IsLocal()) and 
                                (RunningDogfight())
                            ))
                        {
                            theObject->EntityDriver()->Exec(vuxGameTime);
                        }
                    }

                    if ( not theObject->IsLocal())
                    {
                        CalcTransformMatrix(theObject);
                        //LRKLUDGE
                        gndz = OTWDriver.GetGroundLevel(theObject->XPos(), theObject->YPos());

                        if (theObject->ZPos() > gndz)
                        {
                            theObject->SetPosition(theObject->XPos(), theObject->YPos(), gndz);
                        }

                        //LRKLUDGE
                        if (theObject->ZPos() < -200000.0F)
                        {
                            theObject->SetPosition(theObject->XPos(), theObject->YPos(), -200000.0F);
                        }
                    }
                }
            }

            processObjsAddedInCycle = 1;
        }
        //while (cycleAgain);

        inCycle = 0; // MLR 1/2/2005 -
        SimLibFrameCount ++;
        SimLibElapsedTime += FloatToInt32(SimLibMajorFrameTime * SEC_TO_MSEC + 0.5F);
        UPDATE_SIM_ELAPSED_SECONDS; // COBRA - RED - Scale Elapsed Seconds
    }

    //STOP_PROFILE("SIMCYCLE_RUNFRAME");

    if (RunningDogfight())
    {
        SimDogfight.UpdateDogfight();
    }

    // Things to run once per loop when sim is running
    if (runGraphics)
    {
        // if we're not in a cockpit view, play wind noise
        if ( not OTWDriver.DisplayInCockpit())
        {
            //edg note: since this was moved down here from above, the current
            // frame count is now beyond what the positional sound driver expects
            // for the current frame.  I'm just going to brackett this call with
            // a dec/inc of framecount so that nothing else that may be effected
            // by framecount will be effected.
            //SimLibFrameCount--; // MLR 5/16/2004 -
            float x, y, z;
            Tpoint wind;
            wind.x = 0;
            wind.y = 0;
            wind.z = 0;

            if ((WeatherClass*)realWeather)
            {
                ((WeatherClass*)realWeather)->WindHeadingAt(&wind);
            }

            x = OTWDriver.cameraVel.x - wind.x;
            y = OTWDriver.cameraVel.y - wind.y;
            z = OTWDriver.cameraVel.z - wind.z;

            float v = sqrt(x * x + y * y + z * z);
            float vol = (v - 10) * 1000;

            if (vol > 0)
            {
                vol = 0;
            }

            float pit = (v + 100) / 100;

            if (pit > 2)
            {
                pit = 2.0f;
            }

            F4SoundFXSetDist(SFX_WIND, 0, pit, v);
            //SimLibFrameCount++; // MLR 5/16/2004 -
        }

    }

    //START_PROFILE("SIMCYCLE_SOUND");
    if (gameCompressionRatio)
    {
        // run positional Sound FX
        F4SoundFXPositionDriver(SimLibFrameCount - 2, SimLibFrameCount - 1);
    }
    else
    {
        F4SoundFXPositionDriver(SimLibFrameCount + 1, SimLibFrameCount + 1);
    }

    //STOP_PROFILE("SIMCYCLE_SOUND");

    //START_PROFILE("SIMCYCLE_ATC");
    // need to run while in UI for other players
    // Update ATC for each airbase
    UpdateATC();

    if (gNavigationSys)
    {
        gNavigationSys->ExecIls(); // Needed for the Hud and Adi
    }

    //STOP_PROFILE("SIMCYCLE_ATC");

    // check if the simloop thread has told us the sim is to be shut down
    if (doExit)
    {
        Exit();
        doExit = FALSE;
        SetEvent(SimulationLoopControl::wait_for_sim_cleanup);
    }

    // check if the simloop thread has told us the graphics are to be shut down
    if (doGraphicsExit)
    {
        OTWDriver.Exit();
        doGraphicsExit = FALSE;
        FalconLocalSession->SetFlyState(FLYSTATE_IN_UI);
        // we done our part
        SetEvent(SimulationLoopControl::wait_for_graphics_cleanup);
    }
}

// Attempt by Retro 19Mar2004
void SimulationDriver::Pause(void)
{
    SetTimeCompression(0);
    F4SilenceVoices();

    // And no, I don´t know if I need this..
    motionOn = 1;
    SimLibElapsedTime = vuxGameTime;
    UPDATE_SIM_ELAPSED_SECONDS; // COBRA - RED - Scale Elapsed Seconds

}
// Retro attempt ends (hooray)

void SimulationDriver::NoPause(void)
{
    SetTimeCompression(1);
    F4HearVoices();
    SimLibElapsedTime = vuxGameTime;
    UPDATE_SIM_ELAPSED_SECONDS; // COBRA - RED - Scale Elapsed Seconds

}


void SimulationDriver::TogglePause(void)
{
    if (targetCompressionRatio)
    {
        SetTimeCompression(0);
        F4SilenceVoices();
    }
    else
    {
        SetTimeCompression(1);
        F4HearVoices();
    }

    motionOn = 1;

    SimLibElapsedTime = vuxGameTime;
    UPDATE_SIM_ELAPSED_SECONDS; // COBRA - RED - Scale Elapsed Seconds

}


void SimulationDriver::SetPlayerEntity(SimMoverClass* newObject)
{
    if (playerEntity == newObject)
    {
        return;
    }

    // 2002-02-11 MODIFIED BY S.G. A player can also be a EjectedPilotClass
    if (newObject == NULL or newObject->IsAirplane() or newObject->IsEject())
    {
        // sfr: reference / derefence stuff
        if (playerEntity not_eq NULL)
        {
            VuDeReferenceEntity(playerEntity);
        }

        if (newObject not_eq NULL)
        {
            VuReferenceEntity(newObject);
        }

        playerEntity = newObject;
        //MI reset our avionics
        playerLastMasterMode = FireControlComputer::Nav;
        playerLastSubMode = FireControlComputer::ETE;
    }


    ///VWF HACK: The following is a hack to make the Tac Eng Instrument
    // Landing Mission agree with the Manual
    if (
        RunningTactical() and 
        current_tactical_mission and 
        current_tactical_mission->get_type() == tt_training and 
 not strcmpi(current_tactical_mission->get_title(), "10 Instrument Landing") and 
        gNavigationSys
    )
    {
        VU_ID ATCId;
        gNavigationSys->SetInstrumentMode(NavigationSystem::ILS_TACAN);

        if (gTacanList)
        {
            int range, type;
            float ilsf;
            gTacanList->GetVUIDFromChannel(101, TacanList::X, TacanList::AG, &ATCId, &range, &type, &ilsf);
        }

        if (ATCId not_eq FalconNullId and playerEntity)
        {
            FalconATCMessage* atcMsg = new FalconATCMessage(ATCId, FalconLocalGame);
            atcMsg->dataBlock.type = 0;
            atcMsg->dataBlock.from = playerEntity->Id();
            FalconSendMessage(atcMsg, FALSE);
        }
    }

    // If there is no player vehicle, turn off force feedback
    if ( not playerEntity)
    {
        JoystickStopAllEffects();
    }
}

AircraftClass *SimulationDriver::GetPlayerAircraft() const
{
    return (playerEntity and playerEntity->IsAirplane()) ?
           ((AircraftClass*)(playerEntity)) : NULL
           ;
}


void SimulationDriver::UpdateIAStats(SimBaseClass* oldEntity)
{
    if ((oldEntity->IsSetFlag(MOTION_MSL_AI)) or
        (oldEntity->IsSetFlag(MOTION_BMB_AI)))
    {
        //InstantAction.ExpendWeapons(oldEntity);
        return;
    }
    else if ( not (oldEntity->IsSetFlag(MOTION_AIR_AI)) and 
 not (oldEntity->IsSetFlag(MOTION_GND_AI)) and 
 not (oldEntity->IsSetFlag(MOTION_HELO_AI)))
    {
        return;
    }

    //InstantAction.CountCoup(oldEntity);
}

void SimulationDriver::SetFrameDescription(int mSecPerFrame, int numMinorFrames)
{
    SimLibMinorPerMajor  = numMinorFrames;
    SimLibMajorFrameTime = (float)mSecPerFrame * 0.001F * numMinorFrames;
    SimLibMajorFrameRate = 1.0F / SimLibMajorFrameTime;
    SimLibMinorFrameTime = SimLibMajorFrameTime / (float)SimLibMinorPerMajor;
    SimLibMinorFrameRate = 1.0F / SimLibMinorFrameTime;
}

// =================================================
// KCK: Campaign Wake/Sleep functions
// These assume the entities have already been added
// during a deaggregate. these just make them sim-
// aware or sim-blind.
// =================================================

// KCK: This is called by the Campaign to wake a deaggregated objective/unit
// Basically, we do everything we need to to make these entities Sim-Ready/Aware.
// Add to lists, flights, create drawable object, etc.
void SimulationDriver::WakeCampaignBase(int isUnit, CampBaseClass* baseEntity, TailInsertList *comps)
{
    SimBaseClass* theObject = NULL;
    int vehicles = 0, last_to_add = 0, woken = 0;

    ShiAssert(baseEntity);

    if (isUnit)
    {
        vehicles = ((Unit)baseEntity)->GetTotalVehicles();

        if (vehicles > 4)
        {
            // edg: a better detail setting algorithm.  Minimum vehicles is 3.
            // use a quadratic function on slider percentage.
            // so, if the battalion is 48 units the slider settings will give:
            // 0 = 3
            // 1 = 4
            // 2 = 10
            //  3 = 20
            //  4 = 33
            //  5 = 48
            int num = PlayerOptions.ObjectDeaggLevel();
            num *= num;
            last_to_add = 2 + vehicles * num / 10000;
        }
        else
        {
            last_to_add = vehicles;
        }
    }

    // Add our list of objects.
    VuListIterator cit(comps);

    for (
        theObject = static_cast<SimBaseClass*>(cit.GetFirst());
        theObject not_eq NULL;
        theObject = static_cast<SimBaseClass*>(cit.GetNext())
    )
    {
        // KCK: Decide not to wake some ground vehicles/features -
        if (isUnit)
        {
            // Wake vehicles by percentage
            if (
                (woken <= last_to_add) or
                (theObject->GetSlot() == ((Unit)baseEntity)->class_data->RadarVehicle)
            )
            {
                WakeObject(theObject);
                woken++;
            }
        }
        else
        {
            // Wake features by detail level
            if (theObject->displayPriority <= PlayerOptions.BuildingDeaggLevel())
            {
                WakeObject(theObject);
                woken++;
            }
        }

        theObject->SetCampaignObject(baseEntity);
    }
}

// KCK: This is called from the Campaign.
// Sleep an entire sim flight
void SimulationDriver::SleepCampaignFlight(TailInsertList *flightList)
{
    SimBaseClass* theObject;
    VuListIterator flit(flightList);

    // Put all objects in this flight to sleep
    theObject = (SimBaseClass*)flit.GetFirst();
    ShiAssert(theObject == NULL or FALSE == F4IsBadReadPtr(theObject, sizeof * theObject));

    //while (theObject) // JB 010306 CTD
    while (theObject and not F4IsBadReadPtr(theObject, sizeof(SimBaseClass))) // JB 010306 CTD
    {
        theObject->Sleep();
        theObject = (SimBaseClass*)flit.GetNext();
    }
}

// ====================================================
// KCK: These are the general sim Wake/Sleep functions.
// They make the object/flight sim-aware/sim-blind.
// ====================================================

// This call makes the sim aware of this object
void SimulationDriver::WakeObject(SimBaseClass* theObject)
{
    if ( not theObject or theObject->IsAwake())
    {
        return;
    }

    // If it's a player only object, wait until a player actually attaches before allowing the wake.
    if (theObject->IsSetFalcFlag(FEC_PLAYERONLY))
    {
        GameManager.CheckPlayerStatus(theObject);

        if ( not theObject->IsSetFalcFlag(FEC_HASPLAYERS))
        {
            return;
        }
    }

    theObject->Wake();
}

// This call makes the sim ignore this object
void SimulationDriver::SleepObject(SimBaseClass* theObject)
{
    if ( not theObject or not theObject->IsAwake())
        return;

    theObject->Sleep();
}

void SimulationDriver::UpdateRemoteData(void)
{
    SimBaseClass* theObject;
    VuListIterator updateWalker(objectList);

    theObject = (SimBaseClass*)updateWalker.GetFirst();

    while (theObject)
    {
        if ( not theObject->IsLocal())
        {
            CalcTransformMatrix(theObject);
            theObject->Exec();

            if (theObject->EntityDriver())
            {
                theObject->EntityDriver()->Exec(SimLibElapsedTime);

                //LRKLUDGE
                if (theObject->ZPos() > 100.0F)
                {
                    //MonoPrint ("Clamping underground position for remote entity\n");
                    theObject->SetPosition(theObject->XPos(), theObject->YPos(), 100.0F);
                }

                //LRKLUDGE
                if (theObject->ZPos() < -200000.0F)
                {
                    //MonoPrint ("Clamping underground position for remote entity\n");
                    theObject->SetPosition(theObject->XPos(), theObject->YPos(), -200000.0F);
                }
            }

            if (requestList and requestList->requestId == theObject->Id())
            {
                NewRequestList* tmpRequest;
                //MonoPrint ("Clearing requested object %d\n", theObject->Id().num_);
                tmpRequest = requestList;
                requestList = requestList->next;
                delete tmpRequest;
            }
        }

        theObject = (SimBaseClass*)updateWalker.GetNext();
    }
}


void SimulationDriver::UpdateEntityLists()
{
}

SimBaseClass* SimulationDriver::FindNearestThreat(float* bearing, float* range, float* altitude)
{
    SimBaseClass* retval = NULL;
    SimBaseClass* theObject;
    VuListIterator updateWalker(objectList);
    float myX, myY, myZ;
    float tmpRange;
    Team myTeam;

    if ( not playerEntity)
        return NULL;

    myX = ((SimBaseClass*)playerEntity)->XPos();
    myY = ((SimBaseClass*)playerEntity)->YPos();
    myZ = ((SimBaseClass*)playerEntity)->ZPos();
    myTeam = ((SimBaseClass*)playerEntity)->GetTeam();

    theObject = (SimBaseClass*)updateWalker.GetFirst();

    while (theObject)
    {
        if (theObject->IsAirplane() and not theObject->IsDead() and not theObject->OnGround() and not theObject->IsEject() and not theObject->IsDying() and 
            GetTTRelations((Team)theObject->GetTeam(), myTeam) >= Hostile and theObject->GetCampaignObject()->GetSpotted(myTeam))
        {
            if (theObject->GetSType() == STYPE_AIR_FIGHTER or
                theObject->GetSType() == STYPE_AIR_FIGHTER_BOMBER)
            {
                if (retval == NULL)
                {
                    *range = (theObject->XPos() - myX) * (theObject->XPos() - myX) +
                             (theObject->YPos() - myY) * (theObject->YPos() - myY);
                    retval = theObject;
                }
                else
                {
                    tmpRange = (theObject->XPos() - myX) * (theObject->XPos() - myX) +
                               (theObject->YPos() - myY) * (theObject->YPos() - myY);

                    if (tmpRange < *range)
                    {
                        *range = tmpRange;
                        retval = theObject;
                    }
                }
            }
        }

        theObject = (SimBaseClass*)updateWalker.GetNext();
    }

    if (retval)
    {
        *bearing = (float)atan2(retval->YPos() - myY, retval->XPos() - myX) * RTD;
        *range = (float)sqrt(*range);
        *altitude = -retval->ZPos();
    }

    return (retval);
}

SimBaseClass* SimulationDriver::FindNearestThreat(short *x, short *y, float* altitude)
{
    SimBaseClass* retval = NULL;
    SimBaseClass* theObject = NULL;
    VuListIterator updateWalker(objectList);
    float myX = 0.0F, myY = 0.0F, myZ = 0.0F;
    float tmpRange = 0.0F, range = 0.0F;
    Team myTeam = 0;

    if ( not playerEntity)
        return NULL;

    myX = ((SimBaseClass*)playerEntity)->XPos();
    myY = ((SimBaseClass*)playerEntity)->YPos();
    myZ = ((SimBaseClass*)playerEntity)->ZPos();
    myTeam = ((SimBaseClass*)playerEntity)->GetTeam();

    theObject = (SimBaseClass*)updateWalker.GetFirst();

    while (theObject)
    {
        if (theObject->IsAirplane() and not theObject->IsDead() and not theObject->OnGround() and not theObject->IsEject() and not theObject->IsDying() and 
            GetTTRelations((Team)theObject->GetTeam(), myTeam) >= Hostile and theObject->GetCampaignObject()->GetSpotted(myTeam))
        {
            if (theObject->GetSType() == STYPE_AIR_FIGHTER or
                theObject->GetSType() == STYPE_AIR_FIGHTER_BOMBER)
            {
                if (retval == NULL)
                {
                    *x = SimToGrid(theObject->YPos());
                    *y = SimToGrid(theObject->XPos());
                    range = (theObject->XPos() - myX) * (theObject->XPos() - myX) +
                            (theObject->YPos() - myY) * (theObject->YPos() - myY);
                    retval = theObject;
                }
                else
                {
                    tmpRange = (theObject->XPos() - myX) * (theObject->XPos() - myX) +
                               (theObject->YPos() - myY) * (theObject->YPos() - myY);

                    if (tmpRange < range)
                    {
                        *x = SimToGrid(theObject->YPos());
                        *y = SimToGrid(theObject->XPos());
                        range = tmpRange;
                        retval = theObject;
                    }
                }
            }
        }

        theObject = (SimBaseClass*)updateWalker.GetNext();
    }

    if (retval)
    {
        *altitude = -retval->ZPos();
    }

    return (retval);
}

SimBaseClass* SimulationDriver::FindNearestThreat(AircraftClass* aircraft, short *x, short *y, float* altitude)
{
    SimBaseClass* retval = NULL;
    SimBaseClass* theObject = NULL;
    VuListIterator updateWalker(objectList);
    float myX = 0.0F, myY = 0.0F, myZ = 0.0F;
    float tmpRange = 0.0F, range = 0.0F;
    Team myTeam = 0;

    if ( not playerEntity)
        return NULL;

    myX = aircraft->XPos();
    myY = aircraft->YPos();
    myZ = aircraft->ZPos();
    myTeam = aircraft->GetTeam();

    theObject = (SimBaseClass*)updateWalker.GetFirst();

    while (theObject)
    {
        if (theObject->IsAirplane() and not theObject->IsDead() and not theObject->OnGround() and not theObject->IsEject() and not theObject->IsDying() and 
            GetTTRelations((Team)theObject->GetTeam(), myTeam) >= Hostile and theObject->GetCampaignObject()->GetSpotted(myTeam))
        {
            if (theObject->GetSType() == STYPE_AIR_FIGHTER or
                theObject->GetSType() == STYPE_AIR_FIGHTER_BOMBER)
            {
                if (retval == NULL)
                {
                    *x = SimToGrid(theObject->YPos());
                    *y = SimToGrid(theObject->XPos());
                    range = (theObject->XPos() - myX) * (theObject->XPos() - myX) +
                            (theObject->YPos() - myY) * (theObject->YPos() - myY);
                    retval = theObject;
                }
                else
                {
                    tmpRange = (theObject->XPos() - myX) * (theObject->XPos() - myX) +
                               (theObject->YPos() - myY) * (theObject->YPos() - myY);

                    if (tmpRange < range)
                    {
                        *x = SimToGrid(theObject->YPos());
                        *y = SimToGrid(theObject->XPos());
                        range = tmpRange;
                        retval = theObject;
                    }
                }
            }
        }

        theObject = (SimBaseClass*)updateWalker.GetNext();
    }

    if (retval)
    {
        *altitude = -retval->ZPos();
    }

    return (retval);
}

SimBaseClass* SimulationDriver::FindNearestEnemyPlane(AircraftClass* aircraft, short *x, short *y, float* altitude)
{
    SimBaseClass* retval = NULL;
    SimBaseClass* theObject = NULL;
    VuListIterator updateWalker(objectList);
    float myX = 0.0F, myY = 0.0F, myZ = 0.0F;
    float tmpRange = 0.0F, range = 0.0F;
    Team myTeam = 0;

    if ( not playerEntity)
        return NULL;

    myX = aircraft->XPos();
    myY = aircraft->YPos();
    myZ = aircraft->ZPos();
    myTeam = aircraft->GetTeam();

    theObject = (SimBaseClass*)updateWalker.GetFirst();

    while (theObject)
    {
        if (theObject->IsAirplane() and not theObject->IsDead() and not theObject->OnGround() and 
            GetTTRelations((Team)theObject->GetTeam(), myTeam) >= Hostile and theObject->GetCampaignObject()->GetSpotted(myTeam))
        {
            if (retval == NULL)
            {
                *x = SimToGrid(theObject->YPos());
                *y = SimToGrid(theObject->XPos());
                range = (theObject->XPos() - myX) * (theObject->XPos() - myX) +
                        (theObject->YPos() - myY) * (theObject->YPos() - myY);
                retval = theObject;
            }
            else
            {
                tmpRange = (theObject->XPos() - myX) * (theObject->XPos() - myX) +
                           (theObject->YPos() - myY) * (theObject->YPos() - myY);

                if (tmpRange < range)
                {
                    *x = SimToGrid(theObject->YPos());
                    *y = SimToGrid(theObject->XPos());
                    range = tmpRange;
                    retval = theObject;
                }
            }
        }

        theObject = (SimBaseClass*)updateWalker.GetNext();
    }

    if (retval)
    {
        *altitude = -retval->ZPos();
    }

    return (retval);
}

CampBaseClass* SimulationDriver::FindNearestCampThreat(AircraftClass* aircraft, short *x, short *y, float* altitude)
{
    CampBaseClass* retval = NULL;
    CampBaseClass* theUnit = NULL;

    float myX = 0.0F, myY = 0.0F, myZ = 0.0F;
    float tmpRange = 0.0F, range = 0.0F;
    Team myTeam = 0;

    if ( not playerEntity)
        return NULL;

    // Don't look for campaign threats in dogfight. Everything is deaggregated anyway, and
    // this often finds people who havn't entered the sim yet.
    if (FalconLocalGame and FalconLocalGame->GetGameType() == game_Dogfight)
    {
        return NULL;
    }

    myX = aircraft->XPos();
    myY = aircraft->YPos();
    myZ = aircraft->ZPos();
    myTeam = (Team)aircraft->GetTeam();

#ifndef VU_GRID_TREE_Y_MAJOR
    VuGridIterator myit(RealUnitProxList, (BIG_SCALAR)myX, (BIG_SCALAR)myY, (BIG_SCALAR)GridToSim(100));
#else
    VuGridIterator myit(RealUnitProxList, (BIG_SCALAR)myY, (BIG_SCALAR)myX, (BIG_SCALAR)GridToSim(100));
#endif

    theUnit = (CampBaseClass*)myit.GetFirst();

    while (theUnit)
    {
        if (theUnit->IsFlight() and not theUnit->IsDead() and GetTTRelations((Team)theUnit->GetTeam(), myTeam) >= Hostile and theUnit->GetSpotted(myTeam))
        {
            if (theUnit->GetSType() == STYPE_UNIT_FIGHTER or
                theUnit->GetSType() == STYPE_UNIT_FIGHTER_BOMBER)
            {
                if (retval == NULL)
                {
                    *x = SimToGrid(theUnit->YPos());
                    *y = SimToGrid(theUnit->XPos());
                    range = (theUnit->XPos() - myX) * (theUnit->XPos() - myX) +
                            (theUnit->YPos() - myY) * (theUnit->YPos() - myY);
                    retval = theUnit;
                }
                else
                {
                    tmpRange = (theUnit->XPos() - myX) * (theUnit->XPos() - myX) +
                               (theUnit->YPos() - myY) * (theUnit->YPos() - myY);

                    if (tmpRange < range)
                    {
                        *x = SimToGrid(theUnit->YPos());
                        *y = SimToGrid(theUnit->XPos());
                        range = tmpRange;
                        retval = theUnit;
                    }
                }
            }
        }

        theUnit = (CampBaseClass*)myit.GetNext();
    }

    if (retval)
    {
        *altitude = -retval->ZPos();
    }

    return (retval);
}

CampBaseClass* SimulationDriver::FindNearestCampEnemy(AircraftClass* aircraft, short *x, short *y, float* altitude)
{
    CampBaseClass* retval = NULL;
    CampBaseClass* theUnit = NULL;

    float myX = 0.0F, myY = 0.0F, myZ = 0.0F;
    float tmpRange = 0.0F, range = 0.0F;
    Team myTeam = 0;

    if ( not playerEntity)
        return NULL;

    // Don't look for campaign threats in dogfight. Everything is deaggregated anyway, and
    // this often finds people who havn't entered the sim yet.
    if (FalconLocalGame and FalconLocalGame->GetGameType() == game_Dogfight)
    {
        return NULL;
    }

    myX = aircraft->XPos();
    myY = aircraft->YPos();
    myZ = aircraft->ZPos();
    myTeam = aircraft->GetTeam();

#ifndef VU_GRID_TREE_Y_MAJOR
    VuGridIterator myit(RealUnitProxList, (BIG_SCALAR)myX, (BIG_SCALAR)myY, (BIG_SCALAR)GridToSim(100));
#else
    VuGridIterator myit(RealUnitProxList, (BIG_SCALAR)myY, (BIG_SCALAR)myX, (BIG_SCALAR)GridToSim(100));
#endif

    theUnit = (CampBaseClass*)myit.GetFirst();

    while (theUnit)
    {
        if (theUnit->IsFlight() and not theUnit->IsDead() and GetTTRelations((Team)theUnit->GetTeam(), myTeam) >= Hostile and theUnit->GetSpotted(myTeam))
        {
            if (retval == NULL)
            {
                *x = SimToGrid(theUnit->YPos());
                *y = SimToGrid(theUnit->XPos());
                range = (theUnit->XPos() - myX) * (theUnit->XPos() - myX) +
                        (theUnit->YPos() - myY) * (theUnit->YPos() - myY);
                retval = theUnit;
            }
            else
            {
                tmpRange = (theUnit->XPos() - myX) * (theUnit->XPos() - myX) +
                           (theUnit->YPos() - myY) * (theUnit->YPos() - myY);

                if (tmpRange < range)
                {
                    *x = SimToGrid(theUnit->YPos());
                    *y = SimToGrid(theUnit->XPos());
                    range = tmpRange;
                    retval = theUnit;
                }
            }
        }

        theUnit = (CampBaseClass*)myit.GetNext();
    }

    if (retval)
    {
        *altitude = -retval->ZPos();
    }

    return (retval);
}

//RAS - 16Jan04 - Check for traffic routine
// This will find the nearest non-hostile aircraft.  If within the trafficCheckRange and trafficCheck Alt,
// It will check to see if there is a possible traffic conflict by calling the FindTrafficConflict routine.
// If traffic is inside the priTrafficDist distance, it will find the aircraft closest to your altitude
// and then check for a conflict.
// Due to Falcon4's calls, it will only call out distances like this: 1,2,3,4,5,10  (no 6,7,8 or 9 mile calls)

SimBaseClass* SimulationDriver::FindNearestTraffic(AircraftClass* aircraft, ObjectiveClass *self, float* altitude)
{
    SimBaseClass* retval = NULL;
    SimBaseClass* theObject = NULL;
    VuListIterator updateWalker(objectList);
    float myX = 0.0F, myY = 0.0F, myAlt = 0.0F;
    float tmpRange = 0.0F, trafficRange = 0.0F, tmpTrafficAlt = 0.0F, trafficAlt = 0.0F;
    Team myTeam = 0;
    int trafficCheckRange = 10; // Range to check for traffic
    int trafficCheckAlt = 2000; // Relative altitude to check for traffic
    int priTrafficDist = 5; // Priority traffic distance (not fully implimented)


    if ( not playerEntity)
        return NULL;

    myX = aircraft->XPos(); // My X position
    myY = aircraft->YPos(); // My Y position
    myAlt = -aircraft->ZPos(); // My Altitude
    myTeam = aircraft->GetTeam(); // Team info


    theObject = (SimBaseClass*)updateWalker.GetFirst();

    while (theObject)
    {
        // check Flight callsign so doesn't call out your flight
        // check it is an airplane and not dead
        // check that it is not on the ground
        // checks that it is not hostile
        if (aircraft->GetCallsignIdx() not_eq theObject->GetCallsignIdx() and theObject->IsAirplane() and not theObject->IsDead() and not theObject->OnGround() and 
            GetTTRelations((Team)theObject->GetTeam(), myTeam) <= Neutral)
        {
            if (retval == NULL)
            {
                // Range to traffic
                trafficRange = (theObject->XPos() - myX) * (theObject->XPos() - myX) +
                               (theObject->YPos() - myY) * (theObject->YPos() - myY);

                // Altitude of traffic
                trafficAlt = -theObject->ZPos();

                // Check to see if traffic inside trafficCheckRange
                if (SimToGrid(sqrt(trafficRange)) <= trafficCheckRange) //SimToGrid and sqrt convert to NM
                {
                    // Check to see if altitude of traffic falls within trafficCheckAlt limits
                    if (abs(trafficAlt - myAlt) <= trafficCheckAlt)
                    {
                        FindTrafficConflict(theObject, aircraft, self); // Check for conflict

                        if (self->brain->trafficCheck == conflictTraffic) // Traffic is a conflict
                        {
                            retval = theObject; // Set retval to current traffic
                            self->brain->trafficCheck = newTraffic; // This is new traffic
                        }
                    }
                }
            }
            // Arrive here if retval is already set to traffic that is found.  Here we look to see
            // if this traffic is closer than the traffic already set
            else
            {
                // temp Range to traffic
                tmpRange = (theObject->XPos() - myX) * (theObject->XPos() - myX) +
                           (theObject->YPos() - myY) * (theObject->YPos() - myY);

                // temp Altitude of traffic
                tmpTrafficAlt = -theObject->ZPos();

                // if traffic is inside the priority traffic range set by priTrafficDist, find the
                // aircraft closest to my altitude even if it's farther away
                if (abs(tmpTrafficAlt - myAlt) < abs(trafficAlt - myAlt) and SimToGrid(sqrt(tmpRange))
                    <= priTrafficDist)
                {
                    FindTrafficConflict(theObject, aircraft, self); // Check for conflict

                    // a conflict was found and set in FindTrafficConflict function
                    if (self->brain->trafficCheck == conflictTraffic)
                    {
                        // this return value to this traffic because it is more of a threat
                        trafficRange = tmpRange; // setup traffic range
                        retval = theObject;
                        self->brain->trafficCheck = priorityTraffic;
                    }
                }
                else
                {
                    if (tmpRange < trafficRange)
                    {
                        FindTrafficConflict(theObject, aircraft, self);

                        if (self->brain->trafficCheck == conflictTraffic)
                        {
                            trafficRange = tmpRange;
                            retval = theObject;
                            self->brain->trafficCheck = newTraffic;
                        }
                    }
                }
            }
        }

        theObject = (SimBaseClass*)updateWalker.GetNext();
    }

    if (retval)
    {
        *altitude = -retval->ZPos();

        if (retval == self->brain->pLastTraffic)
        {
            self->brain->trafficCheck = oldTraffic;
        }

        self->brain->pLastTraffic = retval; //Store last traffic called out
        self->brain->trafficAltitude = *altitude; // Store traffic altitude
        self->brain->trafficRange = trafficRange; // Store traffic range
    }
    else
        self->brain->pLastTraffic = NULL;

    return (retval);
}


// RAS - 18Jan04 - Check for Traffic Conflict code
void SimulationDriver::FindTrafficConflict(SimBaseClass *traffic, AircraftClass *myAircraft, ObjectiveClass *self)
{

    float myHdg = 0.0F, trafficHdg = 0.0F;
    float xdiff = 0.0F, ydiff = 0.0F, angle = 0.0F;
    int hdgToTraffic = 0; // heading to traffic
    int hdgToMyPlane = 0; // heading to my plane
    int relativeBearing = 0; // relative heading from nose of my plane to traffic
    int parallelTrafficHdg = 0; // normalized parallel hdg referenced to hdgToMyPlane
    int normalizedTrafficHdg = 0; // normalized hdg of traffic referenced to hdgToMyPlane
    float myKIAS = 0, trafficKIAS = 0; // airspeed of me and traffic

    // Fine Tune conflict resolution with these numbers
    int pureFwdOffset = 10; // add or subtract this heading from hdgToMyPlane
    int pureRearOffset = 5;
    int leadFwdOffset = 10; // fwd of 3/9 line, add or sub hdg from traffic's hdg
    int leadRearOffset = 5; // behind 3/9 line, add or sub hdg from traffic's hdg
    float speedThreshold = 5.0F; // overtaking speed threshold in GetKias


    myHdg = myAircraft->Yaw() * RTD; // Find my heading

    if (myHdg < 0.0F)
        myHdg += 360.0F;

    myKIAS = myAircraft->GetKias(); // My airspeed

    trafficHdg = traffic->Yaw() * RTD; // Find traffic's heading

    if (trafficHdg < 0.0F)
        trafficHdg += 360.0F;

    trafficKIAS = traffic->GetKias(); // Traffic's airspeed


    xdiff = traffic->XPos() - myAircraft->XPos(); // get traffic's X Pos
    ydiff = traffic->YPos() - myAircraft->YPos(); // get traffic's Y Pos

    angle = (float)atan2(ydiff, xdiff); // get radian angle from traffic to my plane
    //angle = angle - myAircraft->Yaw();
    hdgToTraffic =  FloatToInt32(RTD * angle); // convert to degrees

    if (hdgToTraffic < 0)
        hdgToTraffic = 360 + hdgToTraffic;

    // Find heading to Traffic
    hdgToMyPlane = hdgToTraffic - 180;

    if (hdgToMyPlane < 0)
        hdgToMyPlane = 360 + hdgToMyPlane;

    // Setup Relative bearing
    if (hdgToTraffic > myHdg)
    {
        relativeBearing = static_cast<int>(hdgToTraffic - myHdg);

        if (relativeBearing < 0)
            relativeBearing = 360 + relativeBearing;
    }
    else
    {
        static_cast<int>(relativeBearing = hdgToTraffic - (int)myHdg);

        if (relativeBearing < 0)
            relativeBearing = 360 + relativeBearing;
    }

    // Normalize all values so they are based off of hdgToMyPlane = 0 (makes it all relative)
    parallelTrafficHdg = static_cast<int>(myHdg - hdgToMyPlane);

    if (parallelTrafficHdg < 0)
        parallelTrafficHdg = 360 + parallelTrafficHdg;

    normalizedTrafficHdg = static_cast<int>(trafficHdg - hdgToMyPlane);

    if (normalizedTrafficHdg < 0)
        normalizedTrafficHdg = 360 + normalizedTrafficHdg;

    // Check for conflicts start here.  There are 6 sectors described below.

    // Sector's are setup relative to my planes heading
    // Sector I = 005 to 090
    // Sector II = 091 to 175
    // Sector III = 185 to 269
    // Sector IV = 270 to 355
    // Front Sector = 356 to 004
    // Rear Sector = 176 to 184


    // In Sector I
    if (relativeBearing >= 5 and relativeBearing <= 90)
    {
        if ((normalizedTrafficHdg <= (parallelTrafficHdg - leadFwdOffset))  and 
            (normalizedTrafficHdg >= pureFwdOffset))
        {
            self->brain->trafficCheck = conflictTraffic; //possible conflict
        }
        else
            self->brain->trafficCheck = noTraffic; //no conflict

        return;
    }


    // In Sector II
    if (relativeBearing >= 91 and relativeBearing <= 180)
    {
        if ((normalizedTrafficHdg <= (parallelTrafficHdg - leadRearOffset))  and 
            (normalizedTrafficHdg >= pureRearOffset))
        {
            if (abs(trafficKIAS - myKIAS) > speedThreshold)
                self->brain->trafficCheck = conflictTraffic; //possible conflict
        }
        else
            self->brain->trafficCheck = noTraffic; //no conflict

        return;
    }


    // In Sector III
    if (relativeBearing >= 181 and relativeBearing <= 269)
    {
        if ((normalizedTrafficHdg >= (parallelTrafficHdg + leadRearOffset))  and 
            (normalizedTrafficHdg <= 360 - pureRearOffset))
        {
            if (abs(trafficKIAS - myKIAS) >= speedThreshold)
                self->brain->trafficCheck = conflictTraffic; //possible conflict
        }
        else
            self->brain->trafficCheck = noTraffic; //no conflict

        return;
    }


    // In Sector IV
    if (relativeBearing >= 270 and relativeBearing <= 355)
    {
        if ((normalizedTrafficHdg >= (parallelTrafficHdg + leadFwdOffset))  and 
            (normalizedTrafficHdg <= 360 - pureFwdOffset))
        {
            self->brain->trafficCheck = conflictTraffic; //possible conflict
        }
        else
            self->brain->trafficCheck = noTraffic; //no conflict

        return;
    }


    // In Forward Sector
    if (relativeBearing >= 355 and relativeBearing <= 359
        or relativeBearing >= 0 and relativeBearing <= 4)
    {
        if (relativeBearing >= 355 and relativeBearing <= 360)
        {
            if ((normalizedTrafficHdg >= parallelTrafficHdg)  and 
                (normalizedTrafficHdg <= 360))
            {
                self->brain->trafficCheck = conflictTraffic; //possible conflict
            }
            else
                self->brain->trafficCheck = noTraffic; //no conflict
        }
        else
        {
            if ((normalizedTrafficHdg <= parallelTrafficHdg)  and 
                (normalizedTrafficHdg >= 0))
            {
                self->brain->trafficCheck = conflictTraffic; //possible conflict
            }
            else
                self->brain->trafficCheck = noTraffic; //no conflict
        }

        return;
    }


    // In Rear Sector
    if (relativeBearing >= 176 and relativeBearing <= 184)
    {
        if (relativeBearing >= 176 and relativeBearing <= 180)
        {
            if (normalizedTrafficHdg <= parallelTrafficHdg)
                if (abs(trafficKIAS - myKIAS) >= speedThreshold)
                    self->brain->trafficCheck = conflictTraffic; //possible conflict
                else
                    self->brain->trafficCheck = noTraffic; //no conflict

        }
        else
        {
            if ((normalizedTrafficHdg >= parallelTrafficHdg)  and 
                (normalizedTrafficHdg <= 360))
                if (abs(trafficKIAS - myKIAS) >= speedThreshold)
                    self->brain->trafficCheck = conflictTraffic; //possible conflict
                else
                    self->brain->trafficCheck = noTraffic; //no conflict
        }

        return;
    }
}




/*
 ** Description:
 ** This function is called from ACMI when recording is toggled to ON.
 ** We need to walk the object lists and get initial states and stuff
 ** for them and write them out to recording file.
 */
void SimulationDriver::InitACMIRecord(void)
{
    int i;
    int numSwitches;
    SimFeatureClass* theObject;
    SimBaseClass *leadObject;
    SimMoverClass *theMover;
    VuListIterator objectWalker(objectList);
    VuListIterator featureWalker(featureList);
    ACMIGenPositionRecord genPos;
    ACMIMissilePositionRecord misPos;
    ACMIAircraftPositionRecord airPos;
    ACMISwitchRecord switchRec;
    ACMIDOFRecord DOFRec;
    ACMITodOffsetRecord todRec;
    ACMIFeaturePositionRecord featPos;
    ACMIFeatureStatusRecord featStat;

    // record the tod offset
    todRec.hdr.time = OTWDriver.todOffset;
    gACMIRec.TodOffsetRecord(&todRec);

    // make sure the player's f16 is first in list
    if (playerEntity)
    {
        theMover = playerEntity;
        airPos.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
        airPos.data.type = theMover->Type();

        // sfr: remove JB check
        if ( not F4IsBadReadPtr((DrawableBSP*)(theMover->drawPointer), sizeof(DrawableBSP)) and theMover->GetTeam() >= 0 and not F4IsBadReadPtr(TeamInfo[theMover->GetTeam()], sizeof(TeamClass))) // JB 010326 CTD
            airPos.data.uniqueID = ACMIIDTable->Add(theMover->Id(), (char*)((DrawableBSP*)(theMover->drawPointer))->Label(), TeamInfo[theMover->GetTeam()]->GetColor()); //.num_;

        airPos.data.x = theMover->XPos();
        airPos.data.y = theMover->YPos();
        airPos.data.z = theMover->ZPos();
        airPos.data.roll = theMover->Roll();
        airPos.data.pitch = theMover->Pitch();
        airPos.data.yaw = theMover->Yaw();
        RadarClass *radar = (RadarClass*)FindSensor(theMover, SensorClass::Radar);
#if NO_REMOTE_BUGGED_TARGET

        if (radar and radar->CurrentTarget())
        {
            airPos.RadarTarget = ACMIIDTable->Add(radar->CurrentTarget()->BaseData()->Id(), NULL, 0); //.num_;
        }

#else

        if (radar and radar->RemoteBuggedTarget)
        {
            //me123 add record for online targets
            airPos.RadarTarget = ACMIIDTable->Add(radar->RemoteBuggedTarget->Id(), NULL, 0); //.num_;
        }
        else if (radar and radar->CurrentTarget())
        {
            airPos.RadarTarget = ACMIIDTable->Add(radar->CurrentTarget()->BaseData()->Id(), NULL, 0); //.num_;
        }

#endif
        else
        {
            airPos.RadarTarget = -1;
        }

        gACMIRec.AircraftPositionRecord(&airPos);

        // now we need to save the switches
        numSwitches = theMover->GetNumSwitches();

        for (i = 0; i < numSwitches; i++)
        {
            switchRec.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
            switchRec.data.type = theMover->Type();
            switchRec.data.uniqueID = ACMIIDTable->Add(theMover->Id(), NULL, 0); //.num_;
            switchRec.data.switchNum = i;
            switchRec.data.switchVal =
                switchRec.data.prevSwitchVal = theMover->GetSwitch(i);
            gACMIRec.SwitchRecord(&switchRec);
        }

        // now we need to save the dofs
        numSwitches = theMover->GetNumDOFs();

        for (i = 0; i < numSwitches; i++)
        {
            DOFRec.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
            DOFRec.data.type = theMover->Type();
            DOFRec.data.uniqueID = ACMIIDTable->Add(theMover->Id(), NULL, 0); //.num_;
            DOFRec.data.DOFNum = i;
            DOFRec.data.DOFVal =
                DOFRec.data.prevDOFVal = theMover->GetDOFValue(i);
            gACMIRec.DOFRecord(&DOFRec);
        }
    }

    // Do Each Entity (movers)
    theMover = (SimMoverClass*)objectWalker.GetFirst();

    while (theMover)
    {
        // we've already done the player's f16
        if (theMover == playerEntity)
        {
            theMover = (SimMoverClass*)objectWalker.GetNext();
            continue;
        }

        //
        // type of ACMI rec we write depends on type of object....
        //


        // missile
        if (theMover->IsMissile())
        {
            misPos.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
            misPos.data.type = theMover->Type();
            misPos.data.uniqueID = ACMIIDTable->Add(theMover->Id(), NULL, TeamInfo[theMover->GetTeam()]->GetColor()); //.num_;
            misPos.data.x = theMover->XPos();
            misPos.data.y = theMover->YPos();
            misPos.data.z = theMover->ZPos();
            misPos.data.roll = theMover->Roll();
            misPos.data.pitch = theMover->Pitch();
            misPos.data.yaw = theMover->Yaw();
            // remove strcpy(misPos.data.label,"");
            // remove misPos.data.teamColor = TeamInfo[theMover->GetTeam()]->GetColor();
            gACMIRec.MissilePositionRecord(&misPos);
        }
        // bombs
        else if (theMover->IsBomb())
        {
            genPos.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
            genPos.data.type = theMover->Type();
            genPos.data.uniqueID = ACMIIDTable->Add(theMover->Id(), NULL, TeamInfo[theMover->GetTeam()]->GetColor()); //.num_;
            genPos.data.x = theMover->XPos();
            genPos.data.y = theMover->YPos();
            genPos.data.z = theMover->ZPos();
            genPos.data.roll = theMover->Roll();
            genPos.data.pitch = theMover->Pitch();
            genPos.data.yaw = theMover->Yaw();
            // remove strcpy(genPos.data.label,"");// = NULL;
            //VWF
            // remove genPos.data.teamColor = TeamInfo[theMover->GetTeam()]->GetColor();

            if (((BombClass *)theMover)->IsSetBombFlag(BombClass::IsFlare))
                gACMIRec.FlarePositionRecord((ACMIFlarePositionRecord *)&genPos);
            else if (((BombClass *)theMover)->IsSetBombFlag(BombClass::IsChaff))
                gACMIRec.ChaffPositionRecord((ACMIChaffPositionRecord *)&genPos);
            else
                gACMIRec.GenPositionRecord(&genPos);
        }
        // aircraft
        // TODO: anything special for ownship?
        // TODO: save DOF's and states?
        else if (theMover->IsAirplane())
        {
            airPos.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
            airPos.data.type = theMover->Type();
            airPos.data.uniqueID = ACMIIDTable->Add(theMover->Id(), (char *)((DrawableBSP*)(theMover->drawPointer))->Label(), TeamInfo[theMover->GetTeam()]->GetColor()); //.num_;
            // remove _mbsnbcpy((unsigned char*)airPos.data.label, (unsigned char *)((DrawableBSP*)(theMover->drawPointer))->Label(), ACMI_LABEL_LEN - 1);
            // remove airPos.data.teamColor = TeamInfo[theMover->GetTeam()]->GetColor();
            airPos.data.x = theMover->XPos();
            airPos.data.y = theMover->YPos();
            airPos.data.z = theMover->ZPos();
            airPos.data.roll = theMover->Roll();
            airPos.data.pitch = theMover->Pitch();
            airPos.data.yaw = theMover->Yaw();
            RadarClass *radar = (RadarClass*)FindSensor(theMover, SensorClass::Radar);
#if NO_REMOTE_BUGGED_TARGET

            if (radar and radar->CurrentTarget())
            {
                airPos.RadarTarget = ACMIIDTable->Add(radar->CurrentTarget()->BaseData()->Id(), NULL, 0); //.num_;
            }

#else

            if (radar and radar->RemoteBuggedTarget)
            {
                //me123 add record for online targets
                airPos.RadarTarget = ACMIIDTable->Add(radar->RemoteBuggedTarget->Id(), NULL, 0); //.num_;
            }
            else if (radar and radar->CurrentTarget())
            {
                airPos.RadarTarget = ACMIIDTable->Add(radar->CurrentTarget()->BaseData()->Id(), NULL, 0); //.num_;
            }

#endif
            else
            {
                airPos.RadarTarget = -1;
            }

            gACMIRec.AircraftPositionRecord(&airPos);

            // now we need to save the switches
            numSwitches = theMover->GetNumSwitches();

            for (i = 0; i < numSwitches; i++)
            {
                switchRec.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
                switchRec.data.type = theMover->Type();
                switchRec.data.uniqueID = ACMIIDTable->Add(theMover->Id(), NULL, 0); //.num_;
                switchRec.data.switchNum = i;
                switchRec.data.switchVal =
                    switchRec.data.prevSwitchVal = theMover->GetSwitch(i);
                gACMIRec.SwitchRecord(&switchRec);
            }

            // now we need to save the dofs
            numSwitches = theMover->GetNumDOFs();

            for (i = 0; i < numSwitches; i++)
            {
                DOFRec.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
                DOFRec.data.type = theMover->Type();
                DOFRec.data.uniqueID = ACMIIDTable->Add(theMover->Id(), NULL, 0); //.num_;
                DOFRec.data.DOFNum = i;
                DOFRec.data.DOFVal =
                    DOFRec.data.prevDOFVal = theMover->GetDOFValue(i);
                gACMIRec.DOFRecord(&DOFRec);
            }
        }

        // everything else (helos, ground vehicles...)
        else
        {
            genPos.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
            genPos.data.type = theMover->Type();
            genPos.data.uniqueID = ACMIIDTable->Add(theMover->Id(), NULL, TeamInfo[theMover->GetTeam()]->GetColor()); //.num_;
            genPos.data.x = theMover->XPos();
            genPos.data.y = theMover->YPos();
            genPos.data.z = theMover->ZPos();
            genPos.data.roll = theMover->Roll();
            genPos.data.pitch = theMover->Pitch();
            genPos.data.yaw = theMover->Yaw();
            // remove genPos.data.teamColor = TeamInfo[theMover->GetTeam()]->GetColor();
            gACMIRec.GenPositionRecord(&genPos);
        }

        // next one in the loop
        theMover = (SimMoverClass*)objectWalker.GetNext();
    }

    // Do Each feature
    theObject = (SimFeatureClass*)featureWalker.GetFirst();

    while (theObject)
    {
        featPos.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
        featPos.data.type = theObject->Type();
        featPos.data.uniqueID = ACMIIDTable->Add(theObject->Id(), NULL, 0); //.num_;
        featPos.data.x = theObject->XPos();
        featPos.data.y = theObject->YPos();
        featPos.data.z = theObject->ZPos();
        featPos.data.roll = theObject->Roll();
        featPos.data.pitch = theObject->Pitch();
        featPos.data.yaw = theObject->Yaw();
        featPos.data.slot = theObject->GetSlot();
        featPos.data.specialFlags = theObject->featureFlags;
        leadObject = theObject->GetCampaignObject()->GetComponentLead();

        if (leadObject and leadObject->Id().num_ not_eq theObject->Id().num_)
            featPos.data.leadUniqueID = ACMIIDTable->Add(leadObject->Id(), NULL, 0); //.num_;
        else
            featPos.data.leadUniqueID = -1;

        gACMIRec.FeaturePositionRecord(&featPos);

        // TODO: we'll probably need to write out what state it's in
        // once ACMI supports state change events
        featStat.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
        featStat.data.uniqueID = ACMIIDTable->Add(theObject->Id(), NULL, 0); //.num_;
        featStat.data.newStatus = (theObject->Status() bitand VIS_TYPE_MASK);
        featStat.data.prevStatus = (theObject->Status() bitand VIS_TYPE_MASK);
        gACMIRec.FeatureStatusRecord(&featStat);

        // next one in the loop
        theObject = (SimFeatureClass*)featureWalker.GetNext();
    }

}

#if 0
void ProximityCheck(SimBaseClass* thisObj)
{
    VuEntity* theObject;

    theFlight = SimFlight::RootSimFlight;

    while (theFlight)
    {
        if (theFlight->campaignType == SimFlight::CampaignInstantAction)
        {
            VuListIterator updateWalker(theFlight->elements);
            int isClose = FALSE;

            theObject = updateWalker.GetFirst();

            while (theObject)
            {
                if (fabs(thisObj->XPos() - theObject->XPos()) < 20.0F * NM_TO_FT and 
                    fabs(thisObj->YPos() - theObject->YPos()) < 20.0F * NM_TO_FT)
                {
                    isClose = TRUE;
                    break;
                }

                theObject = updateWalker.GetNext();
            }

            if ( not isClose)
            {
                theObject = updateWalker.GetFirst();

                while (theObject)
                {
                    ((SimBaseClass*)theObject)->Sleep();
                    theObject = updateWalker.GetNext();
                }
            }
        }

        theFlight = theFlight->next;
    }
}
#endif


void SimulationDriver::POVKludgeFunction(DWORD povHatAngle)   // VWF POV Kludge 11/24/97, Yeah its a mess, remove after demo
{

    //static DWORD previousAngle = -1;

    //if((povHatAngle == -1 and previousAngle not_eq -1) or
    // (povHatAngle not_eq -1 and previousAngle == -1)) {

    if (OTWDriver.GetOTWDisplayMode() == OTWDriverClass::Mode3DCockpit  or
        OTWDriver.GetOTWDisplayMode() == OTWDriverClass::ModePadlockF3  or // 2002-02-17 MODIFIED BY S.G. Needed now for the 'break lock by POV' to work
        OTWDriver.GetOTWDisplayMode() == OTWDriverClass::ModePadlockEFOV or // 2002-02-17 MODIFIED BY S.G. Needed now for the 'break lock by POV' to work
        OTWDriver.GetOTWDisplayMode() == OTWDriverClass::ModeOrbit  or
        OTWDriver.GetOTWDisplayMode() == OTWDriverClass::ModeChase  or
        OTWDriver.GetOTWDisplayMode() == OTWDriverClass::ModeSatellite)
    {
        /*
          if(povHatAngle == -1) {
          OTWDriver.ViewTiltHold();
          OTWDriver.ViewSpinHold();
          }
          else if((povHatAngle > POV_NW and povHatAngle < 36000) or povHatAngle < POV_NE) {
          OTWViewUp(0, KEY_DOWN, NULL);
          }
          else if(povHatAngle > POV_NE and povHatAngle < POV_SE) {
          OTWViewRight(0, KEY_DOWN, NULL);
          }
          else if(povHatAngle > POV_SE and povHatAngle < POV_SW) {
          OTWViewDown(0, KEY_DOWN, NULL);
          }
          else if(povHatAngle > POV_SW and povHatAngle < POV_NW) {
          OTWViewLeft(0, KEY_DOWN, NULL);
          }*/

        if (povHatAngle == -1)
        {
            OTWDriver.ViewTiltHold();
            OTWDriver.ViewSpinHold();
        }
        else if (((povHatAngle >= (POV_NW + POV_HALF_RANGE)) and (povHatAngle < 36000)) or (povHatAngle < POV_N + POV_HALF_RANGE))
        {
            OTWViewUp(0, KEY_DOWN, NULL);
        }
        else if ((povHatAngle >= (POV_NE - POV_HALF_RANGE)) and (povHatAngle < POV_NE + POV_HALF_RANGE))
        {
            OTWViewRight(0, KEY_DOWN, NULL);
            OTWViewUp(0, KEY_DOWN, NULL);
        }
        else if ((povHatAngle >= (POV_E - POV_HALF_RANGE)) and (povHatAngle < POV_E + POV_HALF_RANGE))
        {
            OTWViewRight(0, KEY_DOWN, NULL);
        }
        else if ((povHatAngle >= (POV_SE - POV_HALF_RANGE)) and (povHatAngle < POV_SE + POV_HALF_RANGE))
        {
            OTWViewRight(0, KEY_DOWN, NULL);
            OTWViewDown(0, KEY_DOWN, NULL);
        }
        else if ((povHatAngle >= (POV_S - POV_HALF_RANGE)) and (povHatAngle < POV_S + POV_HALF_RANGE))
        {
            OTWViewDown(0, KEY_DOWN, NULL);
        }
        else if ((povHatAngle >= (POV_SW - POV_HALF_RANGE)) and (povHatAngle < POV_SW + POV_HALF_RANGE))
        {
            OTWViewDown(0, KEY_DOWN, NULL);
            OTWViewLeft(0, KEY_DOWN, NULL);
        }
        else if ((povHatAngle >= (POV_W - POV_HALF_RANGE)) and (povHatAngle < POV_W + POV_HALF_RANGE))
        {
            OTWViewLeft(0, KEY_DOWN, NULL);
        }
        else if ((povHatAngle >= (POV_NW - POV_HALF_RANGE)) and (povHatAngle < POV_NW + POV_HALF_RANGE))
        {
            OTWViewLeft(0, KEY_DOWN, NULL);
            OTWViewUp(0, KEY_DOWN, NULL);
        }
    }
    else if (OTWDriver.GetOTWDisplayMode() == OTWDriverClass::Mode2DCockpit and povHatAngle not_eq -1)
    {

        if (((povHatAngle >= (POV_NW + POV_HALF_RANGE)) and (povHatAngle < 36000)) or (povHatAngle < POV_N + POV_HALF_RANGE))
        {
            gSelectedCursor = OTWDriver.pCockpitManager->POVDispatch(POV_N, gxPos, gyPos);
        }
        else if ((povHatAngle >= (POV_NE - POV_HALF_RANGE)) and (povHatAngle < POV_NE + POV_HALF_RANGE))
        {
            gSelectedCursor = OTWDriver.pCockpitManager->POVDispatch(POV_NE, gxPos, gyPos);
        }
        else if ((povHatAngle >= (POV_E - POV_HALF_RANGE)) and (povHatAngle < POV_E + POV_HALF_RANGE))
        {
            gSelectedCursor = OTWDriver.pCockpitManager->POVDispatch(POV_E, gxPos, gyPos);
        }
        else if ((povHatAngle >= (POV_SE - POV_HALF_RANGE)) and (povHatAngle < POV_SE + POV_HALF_RANGE))
        {
            gSelectedCursor = OTWDriver.pCockpitManager->POVDispatch(POV_SE, gxPos, gyPos);
        }
        else if ((povHatAngle >= (POV_S - POV_HALF_RANGE)) and (povHatAngle < POV_S + POV_HALF_RANGE))
        {
            gSelectedCursor = OTWDriver.pCockpitManager->POVDispatch(POV_S, gxPos, gyPos);
        }
        else if ((povHatAngle >= (POV_SW - POV_HALF_RANGE)) and (povHatAngle < POV_SW + POV_HALF_RANGE))
        {
            gSelectedCursor = OTWDriver.pCockpitManager->POVDispatch(POV_SW, gxPos, gyPos);
        }
        else if ((povHatAngle >= (POV_W - POV_HALF_RANGE)) and (povHatAngle < POV_W + POV_HALF_RANGE))
        {
            gSelectedCursor = OTWDriver.pCockpitManager->POVDispatch(POV_W, gxPos, gyPos);
        }
        else if ((povHatAngle >= (POV_NW - POV_HALF_RANGE)) and (povHatAngle < POV_NW + POV_HALF_RANGE))
        {
            gSelectedCursor = OTWDriver.pCockpitManager->POVDispatch(POV_NW, gxPos, gyPos);
        }

        /*
         if(((povHatAngle >= (POV_NW + POV_HALF_RANGE)) and (povHatAngle < 65535)) or (povHatAngle < POV_N + POV_HALF_RANGE)) {
         gSelectedCursor = OTWDriver.pCockpitManager->Dispatch(CP_MOUSE_BUTTON0, DisplayOptions.DispWidth / 2, 0);
         }
         else if((povHatAngle >= (POV_NE - POV_HALF_RANGE)) and (povHatAngle < POV_NE + POV_HALF_RANGE)) {
         gSelectedCursor = OTWDriver.pCockpitManager->Dispatch(CP_MOUSE_BUTTON0, DisplayOptions.DispWidth, 0);
         }
         else if((povHatAngle >= (POV_E - POV_HALF_RANGE)) and (povHatAngle < POV_E + POV_HALF_RANGE)) {
         gSelectedCursor = OTWDriver.pCockpitManager->Dispatch(CP_MOUSE_BUTTON0, DisplayOptions.DispWidth, DisplayOptions.DispHeight / 2);
         }
         else if((povHatAngle >= (POV_SE - POV_HALF_RANGE)) and (povHatAngle < POV_SE + POV_HALF_RANGE)) {
         gSelectedCursor = OTWDriver.pCockpitManager->Dispatch(CP_MOUSE_BUTTON0, DisplayOptions.DispWidth, DisplayOptions.DispHeight);
         }
         else if((povHatAngle >= (POV_S - POV_HALF_RANGE)) and (povHatAngle < POV_S + POV_HALF_RANGE)) {
         gSelectedCursor = OTWDriver.pCockpitManager->Dispatch(CP_MOUSE_BUTTON0, DisplayOptions.DispWidth / 2 + 2, DisplayOptions.DispHeight);
         }
         else if((povHatAngle >= (POV_SW - POV_HALF_RANGE)) and (povHatAngle < POV_SW + POV_HALF_RANGE)) {
         gSelectedCursor = OTWDriver.pCockpitManager->Dispatch(CP_MOUSE_BUTTON0, 0, DisplayOptions.DispHeight);
         }
         else if((povHatAngle >= (POV_W - POV_HALF_RANGE)) and (povHatAngle < POV_W + POV_HALF_RANGE)) {
         gSelectedCursor = OTWDriver.pCockpitManager->Dispatch(CP_MOUSE_BUTTON0, 0, DisplayOptions.DispHeight / 2);
         }
         else if((povHatAngle >= (POV_NW - POV_HALF_RANGE)) and (povHatAngle < POV_NW + POV_HALF_RANGE)) {
         gSelectedCursor = OTWDriver.pCockpitManager->Dispatch(CP_MOUSE_BUTTON0, 0, 0);
         }
         */
    }

    //}

    //previousAngle = povHatAngle;
}

// MLR 1/7/2005 - These two functions are designed to fix the problem where
// weapons are launched and the initial position is behind the object that
// released the weapon
/*void SimulationDriver::AddToDatabase (FalconEntity *theObject)
{
 vuDatabase->Insert((VuEntity *)theObject);
 if(InCycle())
 {
 theObject->SetFELocalFlag(FELF_ADDED_DURING_SIMDRIVER_CYCLE);
 ObjAddedDuringCycle();
 }
}*/

void SimulationDriver::AddToObjectList(VuEntity* theObject)
{
    objectList->ForcedInsert(theObject);
    combinedList->ForcedInsert(theObject);
}

void SimulationDriver::AddToFeatureList(VuEntity* theObject)
{
    featureList->ForcedInsert(theObject);
    combinedFeatureList->ForcedInsert(theObject);
}

void SimulationDriver::AddToCampUnitList(VuEntity* theObject)
{
    campUnitList->ForcedInsert(theObject);
    combinedList->ForcedInsert(theObject);
    // MonoPrint ("Adding %d\n", theObject->Id().num_);
}

void SimulationDriver::AddToCampFeatList(VuEntity* theObject)
{
    campObjList->ForcedInsert(theObject);
    combinedFeatureList->ForcedInsert(theObject);
}

void SimulationDriver::AddToCombUnitList(VuEntity* theObject)
{
    combinedList->ForcedInsert(theObject);
}

void SimulationDriver::AddToCombFeatList(VuEntity* theObject)
{
    combinedFeatureList->ForcedInsert(theObject);
}


void SimulationDriver::RemoveFromFeatureList(VuEntity* theObject)
{
    featureList->Remove(theObject);
    combinedFeatureList->Remove(theObject);
}

void SimulationDriver::RemoveFromObjectList(VuEntity* theObject)
{
    objectList->Remove(theObject);
    combinedList->Remove(theObject);
}

void SimulationDriver::RemoveFromCampUnitList(VuEntity* theObject)
{
    campUnitList->Remove(theObject);
    combinedList->Remove(theObject);
    // MonoPrint ("Removeing %d\n", theObject->Id().num_);
}

void SimulationDriver::RemoveFromCampFeatList(VuEntity* theObject)
{
    campObjList->Remove(theObject);
    combinedFeatureList->Remove(theObject);
}

void SimulationDriver::InitializeSimMemoryPools(void)
{
    MonoPrint("Initializing Sim Memory Pools....\n");

#ifdef CHECK_LEAKAGE
    dbgMemSetCheckpoint(3);
    // dbgMemSetSafetyLevel( MEM_SAFETY_DEBUG );
#endif

    // Allocate Memory Pools
#ifdef USE_SH_POOLS
    AirframeClass::InitializeStorage();
    AircraftClass::InitializeStorage();
    MissileClass::InitializeStorage();
    MissileInFlightData::InitializeStorage();
    BombClass::InitializeStorage();
    ChaffClass::InitializeStorage();
    FlareClass::InitializeStorage();
    DebrisClass::InitializeStorage();
    SimObjectType::InitializeStorage();
    SimObjectLocalData::InitializeStorage();
    SfxClass::InitializeStorage();
    GunClass::InitializeStorage();
    GroundClass::InitializeStorage();
    GNDAIClass::InitializeStorage();
    SimFeatureClass::InitializeStorage();

    Drawable2D::InitializeStorage();
    DrawableTracer::InitializeStorage();
    DrawableGroundVehicle::InitializeStorage();
    DrawableBuilding::InitializeStorage();
    DrawableBSP::InitializeStorage();
    DrawableShadowed::InitializeStorage();
    DrawableBridge::InitializeStorage();
    DrawableOvercast::InitializeStorage();

    // Don't need this: Scott has done some evil #define overloading DrawableOvercast
    //DrawableOvercast2::InitializeStorage();

    DrawablePlatform::InitializeStorage();
    DrawableRoadbed::InitializeStorage();
    DrawableTrail::InitializeStorage();
    TrailElement::InitializeStorage();
    DrawablePuff::InitializeStorage();
    DrawablePoint::InitializeStorage();
    DrawableGuys::InitializeStorage();

    displayList::InitializeStorage();
    drawPtrList::InitializeStorage();
    sfxRequest::InitializeStorage();

    WayPointClass::InitializeStorage();
    SMSBaseClass::InitializeStorage();
    SMSClass::InitializeStorage();
    BasicWeaponStation::InitializeStorage();
    AdvancedWeaponStation::InitializeStorage();
    DigitalBrain::InitializeStorage();
    HelicopterClass::InitializeStorage();
    HeliBrain::InitializeStorage();
    SimVuDriver::InitializeStorage();
    SimVuSlave::InitializeStorage();

    ObjectGeometry::InitializeStorage();

    ListElementClass::InitializeStorage();
    ListClass::InitializeStorage();

    UnitDeaggregationData::InitializeStorage();
    MissionRequestClass::InitializeStorage();
    DivisionClass::InitializeStorage();
    EventElement::InitializeStorage();
    ATCBrain::InitializeStorage();

    gFaultMemPool = MemPoolInit(0);
    gTextMemPool = MemPoolInit(0);
    gObjMemPool = MemPoolInit(0);
    gReadInMemPool = MemPoolInit(0);
    gCockMemPool = MemPoolInit(0);
    gTacanMemPool = MemPoolInit(0);
#endif

    // OW
    GraphicsDataPoolInitializeStorage();
}

void SimulationDriver::ReleaseSimMemoryPools(void)
{
    MonoPrint("Releasing Sim Memory Pools....\n");

#ifdef CHECK_LEAKAGE
    dbgMemSetCheckpoint(4);

    if (lastErrorFn)
    {
        dbgMemReportLeakage(MemDefaultPool, 3, 4);
        MemSetErrorHandler(lastErrorFn);
        dbgMemSetDefaultErrorOutput(DBGMEM_OUTPUT_FILE, "simleak.txt");
        dbgMemReportLeakage(MemDefaultPool, 3, 4);
        MemSetErrorHandler(errPrint);
    }
    else
    {
        dbgMemSetDefaultErrorOutput(DBGMEM_OUTPUT_FILE, "simleak.txt");
        dbgMemReportLeakage(MemDefaultPool, 3, 4);
    }

#endif

    // Free Memory Pools
#ifdef USE_SH_POOLS
    AirframeClass::ReleaseStorage();
    AircraftClass::ReleaseStorage();
    DigitalBrain::ReleaseStorage();
    MissileClass::ReleaseStorage();
    MissileInFlightData::ReleaseStorage();
    BombClass::ReleaseStorage();
    ChaffClass::ReleaseStorage();
    FlareClass::ReleaseStorage();
    DebrisClass::ReleaseStorage();
    SimObjectType::ReleaseStorage();
    SimObjectLocalData::ReleaseStorage();
    SfxClass::ReleaseStorage();
    GunClass::ReleaseStorage();
    GroundClass::ReleaseStorage();
    GNDAIClass::ReleaseStorage();
    SimFeatureClass::ReleaseStorage();

    Drawable2D::ReleaseStorage();
    DrawableTracer::ReleaseStorage();
    DrawableGroundVehicle::ReleaseStorage();
    DrawableBuilding::ReleaseStorage();
    DrawableBSP::ReleaseStorage();
    DrawableShadowed::ReleaseStorage();
    DrawableBridge::ReleaseStorage();
    DrawableOvercast::ReleaseStorage();

    // Don't need this: Scott has done some evil #define overloading DrawableOvercast
    //DrawableOvercast2::ReleaseStorage();

    DrawablePlatform::ReleaseStorage();
    DrawableRoadbed::ReleaseStorage();
    DrawableTrail::ReleaseStorage();
    TrailElement::ReleaseStorage();
    DrawablePuff::ReleaseStorage();
    DrawablePoint::ReleaseStorage();
    DrawableGuys::ReleaseStorage();

    displayList::ReleaseStorage();
    drawPtrList::ReleaseStorage();
    sfxRequest::ReleaseStorage();

    WayPointClass::ReleaseStorage();
    SMSBaseClass::ReleaseStorage();
    SMSClass::ReleaseStorage();
    BasicWeaponStation::ReleaseStorage();
    AdvancedWeaponStation::ReleaseStorage();

    HelicopterClass::ReleaseStorage();
    HeliBrain::ReleaseStorage();

    SimVuDriver::ReleaseStorage();
    SimVuSlave::ReleaseStorage();

    ObjectGeometry::ReleaseStorage();

    ListElementClass::ReleaseStorage();
    ListClass::ReleaseStorage();

    UnitDeaggregationData::ReleaseStorage();
    MissionRequestClass::ReleaseStorage();
    DivisionClass::ReleaseStorage();

    EventElement::ReleaseStorage();
    ATCBrain::ReleaseStorage();

    MemPoolFree(gFaultMemPool);
    MemPoolFree(gTextMemPool);
    MemPoolFree(gObjMemPool);
    MemPoolFree(gReadInMemPool);
    MemPoolFree(gCockMemPool);
    MemPoolFree(gTacanMemPool);
#endif

    // OW
    GraphicsDataPoolReleaseStorage();
}


// called by graphics shutdown (OTWDriver.Exit()).  We may want to do
// this more inteligently.....
//extern MEM_POOL glMemPool;
void SimulationDriver::ShrinkSimMemoryPools(void)
{
#if 0
    // Free Memory Pools
#ifdef USE_SH_POOLS
    i += MemPoolShrink(MemDefaultPool);
    // i += MemPoolShrink( glMemPool );
    i += MemPoolShrink(AirframeClass::pool);
    i += MemPoolShrink(AircraftClass::pool);
    i += MemPoolShrink(MissileClass::pool);
    i += MemPoolShrink(BombClass::pool);
    i += MemPoolShrink(SimObjectType::pool);
    i += MemPoolShrink(SimObjectLocalData::pool);
    i += MemPoolShrink(SfxClass::pool);
    i += MemPoolShrink(GunClass::pool);
    i += MemPoolShrink(GroundClass::pool);
    i += MemPoolShrink(GNDAIClass::pool);
    i += MemPoolShrink(SimFeatureClass::pool);

    i += MemPoolShrink(Drawable2D::pool);
    i += MemPoolShrink(DrawableTracer::pool);
    i += MemPoolShrink(DrawableGroundVehicle::pool);
    i += MemPoolShrink(DrawableBuilding::pool);
    i += MemPoolShrink(DrawableBSP::pool);
    i += MemPoolShrink(DrawableShadowed::pool);
    i += MemPoolShrink(DrawableBridge::pool);
    i += MemPoolShrink(DrawableOvercast::pool);

    i += MemPoolShrink(DrawablePlatform::pool);
    i += MemPoolShrink(DrawableRoadbed::pool);
    i += MemPoolShrink(DrawableTrail::pool);
    i += MemPoolShrink(TrailElement::pool);
    i += MemPoolShrink(DrawablePuff::pool);
    i += MemPoolShrink(DrawablePoint::pool);
    i += MemPoolShrink(DrawableGuys::pool);

    i += MemPoolShrink(displayList::pool);
    i += MemPoolShrink(drawPtrList::pool);
    i += MemPoolShrink(sfxRequest::pool);

    i += MemPoolShrink(WayPointClass::pool);
    i += MemPoolShrink(SMSBaseClass::pool);
    i += MemPoolShrink(SMSClass::pool);
    i += MemPoolShrink(BasicWeaponStation::pool);
    i += MemPoolShrink(AdvancedWeaponStation::pool);

    MonoPrint("Sim Memory Pools shrunk by %d bytes\n", i);
#endif
#endif
}
