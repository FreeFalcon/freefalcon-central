/*

UI Comms Driver stuff (ALL of it which is NOT part of VU)

*/

#include <windows.h>
#include "resource.h"
#include <tchar.h>
#include "f4vu.h"
#include "mesg.h"
#include "falcmesg.h"
#include "falclib/include/f4find.h"
#include "uicomms.h"
#include "falcuser.h"
#include "F4Comms.h"
#include "chandler.h"
#include "remotelb.h"
#include "CmpClass.h"
#include "Dispcfg.h"
#include "userids.h"
#include "remotelb.h"
#include "logbook.h"
#include "msginc/requestlogbook.h"
#include "msginc/sendlogbook.h"
#include "msginc/sendimage.h"
#include "stats.h"
#include "f4thread.h"


DWORD gUI_Tracking_Flag = 0;

extern F4CSECTIONHANDLE* campCritical;
ulong gStartConnectTime = 0;
short ViewLogBook = 0;

void StartUITracking();
void SetTime(unsigned long currentTime);
void RemoteLBCleanupCB(void *rec); // cleanup callback
void ShutdownCampaign(void);
void StartCommsQueue();

UIComms *gCommsMgr = NULL;


UIComms::UIComms()
{
    int i;
    TargetGame_ = FalconNullId;

    Online_ = FALSE;
    Status_ = 0;
    GameStats_ = NULL;
    InCampaign_ = 0;
    RemoteLogbooks_ = NULL;

    User_[0] = '\0'; // OW

    for (i = 0; i < game_MaxGameTypes; i++)
    {
        Callback_[i] = NULL;
    }
}

UIComms::~UIComms()
{
}

BOOL UIComms::Setup(HWND hwnd)
{
    AppWnd_ = hwnd;
    SetUserInfo();
    StartUITracking();

    StartCommsQueue();
    return(TRUE);
}

void UIComms::SetCallback(int game, void (*rtn)(short, VU_ID, VU_ID))
{
    if (game < game_MaxGameTypes)
        Callback_[game] = rtn;
}

void UIComms::StartComms(ComDataClass *ComData)
{
    InitCommsStuff(ComData);
}

void UIComms::StartCommsDoneCB(int success)
{
    if (success == F4COMMS_CONNECTED)
    {
        if ( not RemoteLogbooks_)
        {
            RemoteLogbooks_ = new UI_Hash;
            RemoteLogbooks_->Setup(20);
            RemoteLogbooks_->SetCallback(RemoteLBCleanupCB);
        }

        Online_ = TRUE;
        TheCampaign.SetOnlineStatus(1);
    }
    else
    {
        Online_ = FALSE;
        TheCampaign.SetOnlineStatus(0);
    }

    PostMessage(AppWnd_, FM_ONLINE_STATUS, success, 0);
}

void UIComms::SetStatsFile(char *filename)
{
    if (GameStats_)
    {
        if ( not stricmp(filename, GameStats_->GetSaveName()))
            return;

        delete GameStats_;
        GameStats_ = NULL;
    }

    GameStats_ = new PlayerStats;
    GameStats_->SetName(filename);
    GameStats_->LoadStats();

    if (FalconLocalGame and (FalconLocalGame->GetGameType() == game_Campaign or FalconLocalGame->GetGameType() == game_TacticalEngagement))
    {
        LoadStats();
    }
}

void UIComms::StopComms()
{
    //sfr: send a message to everyone saying were out
    VuSessionEvent *vuse = new VuSessionEvent(vuLocalSessionEntity.get(), VU_SESSION_CLOSE, vuGlobalGroup);
    vuse->RequestReliableTransmit();
    vuse->RequestOutOfBandTransmit();
    VuMessageQueue::PostVuMessage(vuse);
    //vuse->Send();
    // wait for message send
    Sleep(100);

    F4EnterCriticalSection(campCritical);
    VuEnterCriticalSection();

    TheCampaign.SetOnlineStatus(0);
    EndCommsStuff();
    TheCampaign.CurrentGame.reset();
    //if (TheCampaign.CurrentGame){
    // VuDeReferenceEntity(TheCampaign.CurrentGame);
    // TheCampaign.CurrentGame = NULL;
    //}
    TargetGame_ = FalconNullId;

    if (RemoteLogbooks_)
    {
        RemoteLogbooks_->Cleanup();
        delete RemoteLogbooks_;
        RemoteLogbooks_ = NULL;
    }

    Online_ = FALSE;
    VuExitCriticalSection();
    F4LeaveCriticalSection(campCritical);
}

void UIComms::Cleanup()
{
    if (Online_)
        StopComms();

    if (GameStats_)
    {
        delete GameStats_;
        GameStats_ = NULL;
    }
}

void UIComms::SetUserInfo()
{
    DWORD type;
    DWORD size;
    HKEY theKey;
    long retval;

    size = 63;
    retval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Network\\logon", 0, KEY_ALL_ACCESS, &theKey);

    if (retval == ERROR_SUCCESS)
        retval = RegQueryValueEx(theKey, "username", 0, &type, (LPBYTE)&User_[0], &size);

    if (theKey) RegCloseKey(theKey);
}

BOOL UIComms::LookAtGame(VuGameEntity* game)
{
    if ( not game or game == vuPlayerPoolGroup)
    {
        TargetGame_ = FalconNullId;
        TheCampaign.CurrentGame.reset();
        //if (TheCampaign.CurrentGame){
        // VuDeReferenceEntity(TheCampaign.CurrentGame);
        // TheCampaign.CurrentGame = NULL;
        //}
    }
    else
    {
        TargetGame_ = game->Id();
    }

    return 1;
}

VuGameEntity* UIComms::GetTargetGame()
{
    // This game isn't in the database yet, but we're still interested.
    if (TheCampaign.CurrentGame)
    {
        return TheCampaign.CurrentGame.get();
    }

    if (TargetGame_ == FalconNullId)
        return NULL;

    VuGameEntity* game = (VuGameEntity*) vuDatabase->Find(TargetGame_);

    if ( not game)
        TargetGame_ = FalconNullId;

    return game;
}

void *UIComms::GetRemoteLB(long playerID)
{
    RemoteLB *lbptr;

    if ( not RemoteLogbooks_) // Major problem...
        return(NULL);

    lbptr = (RemoteLB*)RemoteLogbooks_->Find(playerID);
    return(lbptr);
}

void UIComms::ReceiveLogbook(VU_ID from, LB_PILOT *pilot)
{
    RemoteLB *lbptr;

    if ( not RemoteLogbooks_) // Major problem...
        return;

    lbptr = (RemoteLB*)RemoteLogbooks_->Find(from.creator_);

    if (lbptr)
    {
        // We already have this pilot... compare pilot/patch... see if we need new ones... beyond that... just overwrite
        //if(pilot->photo or pilot->patch has changed request it if custom
        // otherwise delete remote images if not null
        lbptr->SetPilotData(pilot);
    }
    else // New Pilot joining us
    {
        lbptr = new RemoteLB;
        lbptr->SetPilotData(pilot);

        RemoteLogbooks_->Add(from.creator_, lbptr);
        //if(pilot->   photo or pilot->patch... request it if custom
    }

    if (ViewLogBook)
    {
        PostMessage(AppWnd_, FM_REMOTE_LOGBOOK, 0, from.creator_);
        ViewLogBook = 0;
    }
}

void UIComms::ReceiveImage(VU_ID from, uchar type, short sec, short blksize, long offset, long size, uchar *data)
{
    RemoteLB *lbptr;

    if ( not RemoteLogbooks_) // Major problem...
        return;

    lbptr = (RemoteLB*)RemoteLogbooks_->Find(from.creator_);

    if (lbptr)
    {
        // We already have this pilot... compare pilot/patch... see if we need new ones... beyond that... just overwrite
        lbptr->ReceiveImage(type, sec, blksize, offset, size, data);
    } // if not found... we don't care anyway
}

void UIComms::SendLogbook(VU_ID requester)
{
    FalconSessionEntity *session;
    UI_SendLogbook *lbmsg;

    session = (FalconSessionEntity*)vuDatabase->Find(requester);

    if (session)
    {
        lbmsg = new UI_SendLogbook(FalconNullId, session);

        lbmsg->dataBlock.fromID = FalconLocalSessionId;
        memcpy(&lbmsg->dataBlock.Pilot, &LogBook.Pilot, sizeof(LB_PILOT));
        memset(lbmsg->dataBlock.Pilot.Password, 0, sizeof(lbmsg->dataBlock.Pilot.Password)); // don't send password (just for paranioa)
        FalconSendMessage(lbmsg, TRUE);
    }
}

void UIComms::SendImage(uchar, VU_ID requester)
{
    FalconSessionEntity *session;
    UI_SendImage *sndimg;

    // finish this routine
    return;

    session = (FalconSessionEntity*)vuDatabase->Find(requester);

    if (session)
    {
        sndimg = new UI_SendImage(FalconNullId, session);

        FalconSendMessage(sndimg, TRUE);
    }
}

void UIComms::LoadStats()
{
    StatList *stats;

    if ( not GameStats_)
        return;

    stats = GameStats_->Find(TheCampaign.GetCreatorIP(), TheCampaign.GetCreationTime(), TheCampaign.GetCreationIter());

    if (stats)
    {
        FalconLocalSession->SetKill(FalconSessionEntity::_AIR_KILLS_, stats->data.aa_kills);
        FalconLocalSession->SetKill(FalconSessionEntity::_GROUND_KILLS_, stats->data.ag_kills);
        FalconLocalSession->SetKill(FalconSessionEntity::_NAVAL_KILLS_, stats->data.an_kills);
        FalconLocalSession->SetKill(FalconSessionEntity::_STATIC_KILLS_, stats->data.as_kills);
        FalconLocalSession->SetMissions(stats->data.missions);
        FalconLocalSession->SetRating(stats->data.rating);
    }
    else
    {
        FalconLocalSession->SetKill(FalconSessionEntity::_AIR_KILLS_, 0);
        FalconLocalSession->SetKill(FalconSessionEntity::_GROUND_KILLS_, 0);
        FalconLocalSession->SetKill(FalconSessionEntity::_NAVAL_KILLS_, 0);
        FalconLocalSession->SetKill(FalconSessionEntity::_STATIC_KILLS_, 0);
        FalconLocalSession->SetMissions(0);
        FalconLocalSession->SetRating(0);
    }
}

void UIComms::SaveStats()
{
    if ( not GameStats_)
        return;

    GameStats_->AddStat(TheCampaign.GetCreatorIP(), TheCampaign.GetCreationTime(), TheCampaign.GetCreationIter(),
                        FalconLocalSession->GetKill(FalconSessionEntity::_AIR_KILLS_),
                        FalconLocalSession->GetKill(FalconSessionEntity::_GROUND_KILLS_),
                        FalconLocalSession->GetKill(FalconSessionEntity::_NAVAL_KILLS_),
                        FalconLocalSession->GetKill(FalconSessionEntity::_STATIC_KILLS_),
                        FalconLocalSession->GetMissions(),
                        FalconLocalSession->GetRating());
}

void UIComms::UpdateGameIter()
{
    // Tell OTHER machines to change there Game Iter (revision) number and Save their stats
}

void UIComms::RemoteUpdateIter(long newIter)
{
    SaveStats();
    TheCampaign.SetCreationIter(newIter);
}
