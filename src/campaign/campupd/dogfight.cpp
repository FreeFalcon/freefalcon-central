#include "stdhdr.h"
#include "dogfight.h"
#include "initData.h"
#include "simbase.h"
#include "campwp.h"
#include "camp2sim.h"
#include "loadout.h"
#include "entity.h"
#include "simveh.h"
#include "sms.h"
#include "SimDrive.h"
#include "mission.h"
#include "classtbl.h"
#include "otwdrive.h"
#include "MsgInc/SendDogfightInfo.h"
#include "MsgInc/RequestDogfightInfo.h"
#include "MsgInc/RegenerationMsg.h"
#include "MsgInc/SendEvalMsg.h"
#include "F4Find.h"

// OW FIXME: this had to be added after installing the DX8 Beta 1 SDK
//Retro_dead 15Jan2004 #define DIRECTINPUT_VERSION 0x0700
//Retro_dead 15Jan2004 #include "dinput.h"

#include "falcsess.h"
#include "uicomms.h"
#include "aircrft.h"
#include "CampList.h"
#include "Flight.h"
#include "Mission.h"
#include "CampWP.h"
#include "SimMover.h"
#include "MissEval.h"
#include "GameMgr.h"
#include "TimerThread.h"
#include "Team.h"
#include "Campaign.h"
#include "ui95/CHandler.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern void SetTime(unsigned long currentTime);
extern void CheckFlyButton(void);

// Imported Variables
extern uchar DefaultDamageMods[OtherDam + 1];
extern uchar calltable[5][5];
extern C_Handler *gMainHandler;

extern int g_nDFRegenerateFix;

static int winCounted = FALSE;
static int roundEnded = FALSE;

static int action_cam_started = FALSE;
static int action_cam_time = 0;

static int restart_matchplay_round = 0;

static const long REGEN_WAIT_TIME = 5 * 1000;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DogfightClass SimDogfight;

char DogfightClass::settings_filename[MAX_PATH];

float DFOffsetX[5] = { 0.0F,  0.7F,  0.7F, -0.7F, -0.7F };
float DFOffsetY[5] = { 0.0F, -0.7F,  0.7F, -0.7F,  0.7F };
float startHeading[5] = { 0.0F, -45.0F * DTR, 45.0F * DTR, 135.0F * DTR, -135.0F * DTR };

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DogfightClass::DogfightClass(void)
{
    gameType = dog_Furball;
    numRadarMissiles = 0;
    numRearAspectMissiles = 0;
    numAllAspectMissiles = 0;
    startRange = 0.0F;
    startAltitude = 0.0F;
    startX = 1412500.0F;
    startY = 1412500.0F;
    xRatio = 0.5F;
    yRatio = 0.5F;
    startTime = 43000;
    rounds = 5;
    flags = 0;
    localFlags = 0;
    regenerationQueue = NULL;
    strcpy(settings_filename, FalconCampaignSaveDirectory);
    strcat(settings_filename, "\\default.DFS");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DogfightClass::~DogfightClass(void)
{
    if (regenerationQueue)
    {
        regenerationQueue->Unregister();
        delete regenerationQueue;
        regenerationQueue = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void DogfightClass::ApplySettings(void)
{
    // This will update all our dogfight flights to mimic the new settings
    Unit unit;
    int i;

    // KCK: Should this be done on the host only, or should everyone assume we have the same data?
    // if ( not FalconLocalGame or not FalconLocalGame->IsLocal())
    // return;

    if ( not TheCampaign.IsLoaded())
    {
        return;
    }

    startY = xRatio * TheCampaign.TheaterSizeX * FEET_PER_KM;
    startX = yRatio * TheCampaign.TheaterSizeY * FEET_PER_KM;

    if (startRange < 1)
    {
        startRange = 1;
    }

    if (startTime == 0)
    {
        startTime = 1;
    }

    SetTime(startTime);

    if (FalconLocalGame and FalconLocalGame->IsLocal())
    {
        VuListIterator flit(AllRealList);
        unit = (Unit)flit.GetFirst();

        while (unit)
        {
            if (unit->IsFlight())
            {
                ApplySettingsToFlight((Flight)unit);
            }

            unit = (Unit)flit.GetNext();
        }
    }

    for (i = 0; i < MAX_DOGFIGHT_TEAMS; i++)
    {
        if (gameType == dog_Furball)
        {
            if (TeamInfo[i])
            {
                TeamInfo[i]->stance[i] = War;
            }
        }
        else
        {
            if (TeamInfo[i])
            {
                TeamInfo[i]->stance[i] = Allied;
            }
        }
    }

    lastUpdateTime = 0;

    if ( not regenerationQueue)
    {
        ShiAssert(this == &SimDogfight);
        regenerationQueue = new TailInsertList();
        regenerationQueue->Register();
    }

    if (rounds < 1)
    {
        rounds = 1;
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void DogfightClass::ApplySettingsToFlight(Flight flight)
{
    float x, y, z;
    WayPoint w;
    int p, i, cid = 0;
    CampaignTime ttc = FloatToInt32(startRange / 1500.0F) * CampaignSeconds; // find time to convergence in campaign time

    x = startX + DFOffsetX[flight->GetTeam()] * startRange * 0.5F;
    y = startY + DFOffsetY[flight->GetTeam()] * startRange * 0.5F;

    while (calltable[flight->GetTeam()][cid] not_eq flight->callsign_id)
    {
        cid++;
    }

    z = startAltitude - 500.0F * cid - 2500.0F * (flight->callsign_num - 1);
    flight->SimSetLocation(x, y, z);

    if (FalconLocalGame and FalconLocalGame->IsLocal())
    {
        CampEnterCriticalSection();
        flight->DisposeWayPoints();

        // Start waypoint
        w = flight->AddUnitWP(0, 0, 0, 0, startTime, 0, WP_CA);
        w->SetLocation(x, y, startAltitude);
        flight->SetCurrentUnitWP(w);

        // Center waypoint
        w = flight->AddUnitWP(0, 0, 0, 0, startTime + ttc, 0, WP_CA);
        w->SetLocation(startX, startY, startAltitude);

        // Repeat waypoint
        w = flight->AddUnitWP(0, 0, 0, 0, startTime + ttc, 3 * CampaignHours, WP_CA);
        w->SetLocation(x, y, startAltitude);
        w->SetWPFlags(WPF_REPEAT);

        // Load weapons
        flight->RemoveLoadout();
        flight->UseFuel(1);
        flight->LoadWeapons(NULL, DefaultDamageMods, Air, numRadarMissiles, WEAP_FORCE_ON_ONE, WEAP_RADAR);
        flight->LoadWeapons(NULL, DefaultDamageMods, Air, numAllAspectMissiles, WEAP_FORCE_ON_ONE, WEAP_HEATSEEKER);
        flight->LoadWeapons(NULL, DefaultDamageMods, Air, numRearAspectMissiles, WEAP_FORCE_ON_ONE, WEAP_HEATSEEKER bitor WEAP_REAR_ASPECT);

        if (flags bitand DF_ECM_AVAIL)
        {
            flight->LoadWeapons(NULL, DefaultDamageMods, Air, 1, WEAP_ECM, 0);
        }

        // Load the gun (for guns only case)
        flight->LoadWeapons(NULL, DefaultDamageMods, Air, 2, WEAP_GUN, 0);
        CampLeaveCriticalSection();
    }

    flight->SetUnitLastMove(startTime);
    flight->SetFalcFlag(FEC_REGENERATING);

    // Check if there are any players in this flight
    for (i = 0, p = 0; i < PILOTS_PER_FLIGHT; i++)
    {
        if (flight->player_slots[i] not_eq NO_PILOT)
        {
            p++;
        }
    }

    // Set our player flag appropriately
    if (p)
    {
        flight->SetFalcFlag(FEC_PLAYERONLY);
        flight->SetFinal(0);
    }
    else
    {
        flight->UnSetFalcFlag(FEC_PLAYERONLY);
        flight->SetFinal(1);
    }

    flight->SetMoving(1);
    flight->SetNoAbort(1);
    flight->SetCurrentWaypoint(2);
    flight->SetUnitMission(AMIS_SWEEP);

    MonoPrint("Apply Settings To Flight %08x %f %f %f\n", flight, x, y, z);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void DogfightClass::SendSettings(FalconSessionEntity *target)
{
    UI_SendDogfightInfo *settings;

    if (FalconLocalGame) // and FalconLocalGame->IsLocal()) // We should send the settings even if we are a client and not a host - RH
    {
        if (target)
        {
            settings = new UI_SendDogfightInfo(FalconNullId, target);
        }
        else
        {
            settings = new UI_SendDogfightInfo(FalconNullId, FalconLocalGame);
        }

        settings->dataBlock.from = FalconLocalSessionId;
        settings->dataBlock.game = FalconLocalGame->Id();
        memcpy(&settings->dataBlock.settings.gameType, &SimDogfight.gameType, sizeof(DogfightType));
        memcpy(&settings->dataBlock.settings.gameStatus, &SimDogfight.gameStatus, sizeof(DogfightStatus));
        memcpy(&settings->dataBlock.settings.startTime, &SimDogfight.startTime, sizeof(CampaignTime));
        memcpy(&settings->dataBlock.settings.startRange, &SimDogfight.startRange, sizeof(float));
        memcpy(&settings->dataBlock.settings.startAltitude, &SimDogfight.startAltitude, sizeof(float));
        memcpy(&settings->dataBlock.settings.xRatio, &SimDogfight.xRatio, sizeof(float));
        memcpy(&settings->dataBlock.settings.yRatio, &SimDogfight.yRatio, sizeof(float));
        memcpy(&settings->dataBlock.settings.flags, &SimDogfight.flags, sizeof(short));
        memcpy(&settings->dataBlock.settings.rounds, &SimDogfight.rounds, sizeof(uchar));
        memcpy(&settings->dataBlock.settings.currentRound, &SimDogfight.currentRound, sizeof(uchar));
        memcpy(&settings->dataBlock.settings.numRadarMissiles, &SimDogfight.numRadarMissiles, sizeof(uchar));
        memcpy(&settings->dataBlock.settings.numRearAspectMissiles, &SimDogfight.numRearAspectMissiles, sizeof(uchar));
        memcpy(&settings->dataBlock.settings.numAllAspectMissiles, &SimDogfight.numAllAspectMissiles, sizeof(uchar));

        FalconSendMessage(settings, TRUE);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void DogfightClass::ReceiveSettings(DogfightClass *tmpSettings)
{
    if ( not tmpSettings)
    {
        return;
    }

    memcpy(&SimDogfight.gameType, &tmpSettings->gameType, sizeof(DogfightType));
    memcpy(&SimDogfight.gameStatus, &tmpSettings->gameStatus, sizeof(DogfightStatus));
    memcpy(&SimDogfight.startTime, &tmpSettings->startTime, sizeof(CampaignTime));
    memcpy(&SimDogfight.startRange, &tmpSettings->startRange, sizeof(float));
    memcpy(&SimDogfight.startAltitude, &tmpSettings->startAltitude, sizeof(float));
    memcpy(&SimDogfight.xRatio, &tmpSettings->xRatio, sizeof(float));
    memcpy(&SimDogfight.yRatio, &tmpSettings->yRatio, sizeof(float));
    memcpy(&SimDogfight.flags, &tmpSettings->flags, sizeof(short));
    memcpy(&SimDogfight.rounds, &tmpSettings->rounds, sizeof(uchar));
    memcpy(&SimDogfight.currentRound, &tmpSettings->currentRound, sizeof(uchar));
    memcpy(&SimDogfight.numRadarMissiles, &tmpSettings->numRadarMissiles, sizeof(uchar));
    memcpy(&SimDogfight.numRearAspectMissiles, &tmpSettings->numRearAspectMissiles, sizeof(uchar));
    memcpy(&SimDogfight.numAllAspectMissiles, &tmpSettings->numAllAspectMissiles, sizeof(uchar));

    ApplySettings();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void DogfightClass::RequestSettings(FalconGameEntity *game)
{
    UI_RequestDogfightInfo *settings;

    if (game)
    {
        VuTargetEntity *target = (VuTargetEntity*) vuDatabase->Find(game->OwnerId());
        settings = new UI_RequestDogfightInfo(FalconNullId, target);
        settings->dataBlock.requester_id = FalconLocalSessionId;
        FalconSendMessage(settings, TRUE);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int DogfightClass::ReadyToStart(void)
{
    int retval = FALSE;
    Unit unit;
    int teamUsed[MAX_DOGFIGHT_TEAMS] = {0};
    int i, numTeams = 0;

    // Check if it's ok to hit the 'Fly' button
    if ( not (flags bitand DF_GAME_OVER))
    {
        VuListIterator flit(AllRealList);
        unit = (Unit)flit.GetFirst();

        while (unit)
        {
            teamUsed[unit->GetTeam()] ++;
            unit = (Unit)flit.GetNext();
        }

        for (i = 0; i < MAX_DOGFIGHT_TEAMS; i++)
        {
            if (teamUsed[i])
            {
                numTeams ++;
            }
        }

        // Check for more than one team in team play
        if (gameType == dog_Furball or numTeams > 1)
        {
            retval = TRUE;
        }
    }

    return retval;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void DogfightClass::SetFilename(char *filename)
{
    strcpy(settings_filename, filename);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void DogfightClass::LoadSettings(void)
{
    FILE *fp;

    fp = fopen(settings_filename, "rb");

    if ( not fp)
    {
        return;
    }

    fread(&gameType, sizeof(DogfightType), 1, fp);
    fread(&gameStatus, sizeof(DogfightStatus), 1, fp);
    fread(&startTime, sizeof(CampaignTime), 1, fp);
    fread(&startRange, sizeof(float), 1, fp);
    fread(&startAltitude, sizeof(float), 1, fp);
    fread(&xRatio, sizeof(float), 1, fp);
    fread(&yRatio, sizeof(float), 1, fp);
    fread(&flags, sizeof(short), 1, fp);
    fread(&rounds, sizeof(uchar), 1, fp);
    fread(&currentRound, sizeof(uchar), 1, fp);
    fread(&numRadarMissiles, sizeof(uchar), 1, fp);
    fread(&numRearAspectMissiles, sizeof(uchar), 1, fp);
    fread(&numAllAspectMissiles, sizeof(uchar), 1, fp);

    // KCK: reality check ratios (for old save files)
    if (xRatio > 1.0F)
    {
        xRatio = 0.5F;
    }

    if (yRatio > 1.0F)
    {
        yRatio = 0.5F;
    }

    fclose(fp);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void DogfightClass::SaveSettings(char *filename)
{
    FILE *fp;

    fp = fopen(filename, "wb");

    if ( not fp)
    {
        return;
    }

    fwrite(&gameType, sizeof(DogfightType), 1, fp);
    fwrite(&gameStatus, sizeof(DogfightStatus), 1, fp);
    fwrite(&startTime, sizeof(CampaignTime), 1, fp);
    fwrite(&startRange, sizeof(float), 1, fp);
    fwrite(&startAltitude, sizeof(float), 1, fp);
    fwrite(&xRatio, sizeof(float), 1, fp);
    fwrite(&yRatio, sizeof(float), 1, fp);
    fwrite(&flags, sizeof(short), 1, fp);
    fwrite(&rounds, sizeof(uchar), 1, fp);
    fwrite(&currentRound, sizeof(uchar), 1, fp);
    fwrite(&numRadarMissiles, sizeof(uchar), 1, fp);
    fwrite(&numRearAspectMissiles, sizeof(uchar), 1, fp);
    fwrite(&numAllAspectMissiles, sizeof(uchar), 1, fp);

    fclose(fp);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void DogfightClass::UpdateDogfight(void)
{
    if (vuxRealTime > lastUpdateTime + VU_TICS_PER_SECOND)
    {
        // Check for new players
        TheCampaign.MissionEvaluator->RebuildEvaluationData();

        // Update the game status
        UpdateGameStatus();

        // Update local game status
        if (localGameStatus == dog_EndRound)
        {
            // Only Matchplay gets here :-)
            if ((restart_matchplay_round) and (vuxRealTime > static_cast<VU_TIME>(restart_matchplay_round)))
            {
                MonoPrint("Ending Round Now\n");

                RoundOver();

                if (GameOver())
                {
                    EndGame();
                    OTWDriver.EndFlight();
                }
                else
                {
                    EndRound();
                }

                restart_matchplay_round = 0;
            }
            else
            {
                MonoPrint("Still waiting to end round %d\n", restart_matchplay_round - vuxRealTime);

                if ((gameType == dog_TeamMatchplay) and (CheckRoundOver() == 0))
                {
                    RoundOver();

                    if (GameOver())
                    {
                        EndGame();
                        OTWDriver.EndFlight();
                    }
                    else
                    {
                        EndRound();
                    }

                    restart_matchplay_round = 0;
                }
            }
        }
        else if (localGameStatus == dog_Flying)
        {
            if ((gameType == dog_TeamMatchplay) and (CheckRoundOver() <= 1))
            {
                MonoPrint("GameStatus: dog EndRound\n");
                restart_matchplay_round = vuxRealTime + 10 * VU_TICS_PER_SECOND;
                localGameStatus = dog_EndRound;
            }
            else if (GameOver())
            {
                EndGame();
                OTWDriver.EndFlight();
            }
            else if (GameManager.NoMorePlayers(FalconLocalGame))
            {
                EndGame();
            }
            else if (gameType not_eq dog_TeamMatchplay)
            {
                RegenerateAvailableAircraft();
            }
        }
        else if (localGameStatus == dog_Starting)
        {
            if (gameType == dog_TeamMatchplay)
            {
                // Wait until all players are no longer flying before resetting
                if (gameStatus not_eq dog_Flying)
                {
                    ResetRound();
                }

                // Wait until all players are ready before restarting
                if (GameManager.AllPlayersReady(FalconLocalGame))
                {
                    GameManager.ReleasePlayer(FalconLocalSession);
                    action_cam_started = FALSE;
                    action_cam_time = 0;
                    OTWDriver.SetExitMenu(FALSE);
                    MonoPrint("GameStatus: dog Flying\n");
                    localGameStatus = dog_Flying;
                }
            }
            else
            {
                RegenerateAvailableAircraft();
                MonoPrint("GameStatus: dog Flying\n");
                localGameStatus = dog_Flying;
            }
        }
        else
        {
            // Check for game restart
            if ((localFlags bitand DF_VIEWED_SCORES) and (gameStatus == dog_Waiting))
            {
                RestartGame();
            }

            if ((FalconLocalSession->GetFlyState() >= FLYSTATE_LOADING) and (FalconLocalSession->GetFlyState() <= FLYSTATE_FLYING))
            {
                MonoPrint("GameStatus: dog Starting\n");
                localGameStatus = dog_Starting;
            }
        }

        lastUpdateTime = vuxRealTime;

        if (( not SimDriver.GetPlayerEntity()) and ( not action_cam_started) and ( not action_cam_time))
        {
            action_cam_time = vuxRealTime + 10 * VU_TICS_PER_SECOND;

            action_cam_started = TRUE;
        }

        if ((action_cam_started) and (static_cast<VU_TIME>(action_cam_time) > vuxRealTime))
        {
            OTWDriver.ToggleActionCamera();
            action_cam_started = FALSE;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void DogfightClass::UpdateGameStatus(void)
{
    VuSessionsIterator sessionWalker(FalconLocalGame);
    FalconSessionEntity *session;
    DogfightStatus newStatus = dog_Waiting;

    session = (FalconSessionEntity*)sessionWalker.GetFirst();

    while (session and newStatus not_eq dog_Flying)
    {
        if (session->GetFlyState() not_eq FLYSTATE_IN_UI)
        {
            newStatus = dog_Starting;
        }

        if (session->GetFlyState() == FLYSTATE_FLYING)
        {
            newStatus = dog_Flying;
        }

        session = (FalconSessionEntity*)sessionWalker.GetNext();
    }

    gameStatus = newStatus;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void DogfightClass::RegenerateAircraft(AircraftClass *aircraft)
{
    // Queue the aircraft for eventual regeneration
    if ( not aircraft)
        return;

    ShiAssert(aircraft->IsDead());
    ShiAssert(aircraft->IsLocal());

    // If this is the player, display the scores
    if (aircraft == FalconLocalSession->GetPlayerEntity())
    {
        // KCK: This was intended to eventually allow regen only on keypress. Set automatically here
        SetLocalFlag(DF_PLAYER_REQ_REGEN);
        OTWDriver.SetFrontTextFlags(OTWDriver.GetFrontTextFlags() bitor SHOW_DOGFIGHT_SCORES);
    }

    // Set the time of death to NOW for use in delaying the regeneration.
    aircraft->timeOfDeath = vuxGameTime;

    // Put the aircraft into the regeneration queue to wait its turn
    if (regenerationQueue)
    {
        regenerationQueue->ForcedInsert(aircraft);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int DogfightClass::AdjustClassId(int oldid, int team)
{
    Falcon4EntityClassType *classPtr = &Falcon4ClassTable[oldid];

    return GetClassID(classPtr->vuClassData.classInfo_[0], classPtr->vuClassData.classInfo_[1], classPtr->vuClassData.classInfo_[2], classPtr->vuClassData.classInfo_[3], classPtr->vuClassData.classInfo_[4], team, VU_ANY, VU_ANY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int DogfightClass::GameOver(void)
{
    int i, over = FALSE;

    switch (gameType)
    {
        case dog_TeamMatchplay:
        case dog_TeamFurball:
        {
            {
                short score[MAX_DOGFIGHT_TEAMS];
                TheCampaign.MissionEvaluator->GetTeamScore(score);

                for (i = 0; i < MAX_DOGFIGHT_TEAMS; i++)
                {
                    if (score[i] >= rounds)
                    {
                        TheCampaign.MissionEvaluator->RegisterWin(i);
                        over = TRUE;
                    }
                }

                return over;
            }
            break;
        }

        default:
        {
            if (TheCampaign.MissionEvaluator->GetMaxScore() >= rounds)
            {
                TheCampaign.MissionEvaluator->RegisterWin(-1);
                return TRUE;
            }

            break;
        }
    }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int DogfightClass::CheckRoundOver(void)
{
    SimBaseClass *theObject;
    int activeAC[MAX_DOGFIGHT_TEAMS] = {0}, activeTeams = 0, team, lastTeam = 0;
    CampEntity campEntity;

    VuListIterator updateWalker(SimDriver.objectList);
    theObject = (SimBaseClass*)updateWalker.GetFirst();

    while (theObject)
    {
        campEntity = theObject->GetCampaignObject();

        if (campEntity and campEntity->IsFlight())
        {
            team = campEntity->GetTeam();

            if ( not activeAC[team])
            {
                activeTeams++;
            }

            activeAC[team]++;
            lastTeam = team;
        }

        theObject = (SimBaseClass*)updateWalker.GetNext();
    }

    return activeTeams;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void DogfightClass::RoundOver(void)
{
    SimBaseClass *theObject;
    int activeAC[MAX_DOGFIGHT_TEAMS] = {0}, activeTeams = 0, team, lastTeam = 0;
    CampEntity campEntity;

    {
        VuListIterator updateWalker(SimDriver.objectList);
        theObject = (SimBaseClass*)updateWalker.GetFirst();

        while (theObject)
        {
            campEntity = theObject->GetCampaignObject();

            if (campEntity and campEntity->IsFlight())
            {
                team = campEntity->GetTeam();

                if ( not activeAC[team])
                {
                    activeTeams++;
                }

                activeAC[team]++;
                lastTeam = team;
            }

            // 2002-04-10 MN force a kill on all alive simlist objects
            if ( not (theObject->IsDead()) and (g_nDFRegenerateFix bitand 0x02))
                theObject->SetDead(TRUE);

            theObject = (SimBaseClass*)updateWalker.GetNext();
        }
    }

    if (activeTeams == 1)
    {
        MonoPrint("And the round goes to %d\n", lastTeam);
        TheCampaign.MissionEvaluator->RegisterRoundWon(lastTeam);
    }
    else
    {
        MonoPrint("Draw\n");
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void DogfightClass::EndGame(void)
{
    if (regenerationQueue)
    {
        regenerationQueue->Purge();
    }

    SetTimeCompression(0);
    MonoPrint("Game has ended\n");
    MonoPrint("GameStatus: dog Waiting\n");
    localGameStatus = gameStatus = dog_Waiting;
    flags or_eq DF_GAME_OVER;
    FalconLocalSession->SetFlyState(FLYSTATE_IN_UI);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void DogfightClass::RestartGame(void)
{
    MonoPrint("Restarting Game\n");
    // Reset game and mission evaluator after everyone has returned to the UI and viewed their scores
    flags and_eq compl DF_GAME_OVER;
    localFlags and_eq compl DF_VIEWED_SCORES;
    TheCampaign.MissionEvaluator->PreDogfightEval();
    ApplySettings();

    if (gMainHandler)
    {
        CheckFlyButton();
    }

    // Have the master resend the settings just to make sure everyone is in sync.
    if (FalconLocalGame and FalconLocalGame->IsLocal())
    {
        SendSettings(NULL);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Kill of any remaining entities and prepair for a reset

void DogfightClass::EndRound(void)
{
    SimBaseClass *theObject;

    {
        VuListIterator updateWalker(SimDriver.objectList);
        theObject = (SimBaseClass*)updateWalker.GetFirst();

        while (theObject)
        {
            theObject->SetDead(1);
            theObject = (SimBaseClass*)updateWalker.GetNext();
        }
    }

    MonoPrint("GameStatus: dog Starting\n");
    localGameStatus = dog_Starting;
    FalconLocalSession->SetFlyState(FLYSTATE_DEAD);
    GameManager.LockPlayer(FalconLocalSession);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Resurrect everyone and prepair to wait til they're all ready

void DogfightClass::ResetRound(void)
{
    // localGameStatus = gameStatus = dog_Starting;
    RegenerateAvailableAircraft();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
int gLastRegenCount = 0;
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void DogfightClass::RegenerateAvailableAircraft(void)
{
    if (regenerationQueue)
    {
        SimBaseClass *theObject;
        FalconRegenerationMessage *msg;
        int restartIt;

        VuListIterator it(regenerationQueue);
        theObject = (SimBaseClass*)it.GetFirst();

        while (theObject)
        {
            ShiAssert(theObject->IsLocal());
            restartIt = FALSE;

            if (vuxGameTime > static_cast<VU_TIME>(theObject->timeOfDeath + REGEN_WAIT_TIME) or gameType == dog_TeamMatchplay)
            {
                if (theObject not_eq FalconLocalSession->GetPlayerEntity() or IsSetLocalFlag(DF_PLAYER_REQ_REGEN) or gameType == dog_TeamMatchplay)
                {
                    msg = new FalconRegenerationMessage(theObject->Id(), FalconLocalGame);
                    msg->dataBlock.newx = startX;
                    msg->dataBlock.newy = startY;
                    msg->dataBlock.newz = startAltitude;
                    msg->dataBlock.newyaw = startHeading[theObject->GetTeam()];
                    FalconSendMessage(msg, TRUE);

                    // If this is the player, clear the regen flag and turn off the scores display
                    if (theObject == FalconLocalSession->GetPlayerEntity())
                    {
                        UnSetLocalFlag(DF_PLAYER_REQ_REGEN);
                        OTWDriver.SetFrontTextFlags(OTWDriver.GetFrontTextFlags() bitand compl SHOW_DOGFIGHT_SCORES);
                    }

#ifdef DEBUG
                    gLastRegenCount++;
#endif
                    // Note: This is kinda broken inside VU and may result in skipping the next list entry.
                    // This is mostly tolerable here since it should get handled next time through the
                    // loop anyway, with the important exception of match play. So we'll do a GetFirst()
                    // every time we actually remove an entry
                    regenerationQueue->Remove(theObject);
                    restartIt = TRUE;
                }
            }

            if (restartIt)
            {
                theObject = (SimBaseClass*)it.GetFirst();
            }
            else
            {
                theObject = (SimBaseClass*)it.GetNext();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
