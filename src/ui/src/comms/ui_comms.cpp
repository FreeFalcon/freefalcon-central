/***************************************************************************\
 UI_COMMS.cpp
 Peter Ward
 December 3, 1996

 Comms Handling stuff for thw UI
\***************************************************************************/
#include <windows.h>
#include "falclib.h"
#include "f4vu.h"
#include "Mesg.h"
#include "msginc/sendchatmessage.h"
#include "msginc/requestlogbook.h"
#include "falcmesg.h"
#include "sim/include/otwdrive.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "uicomms.h"
#include "userids.h"
#include "textids.h"
#include "sim/include/commands.h"
#include "CmpClass.h"
#include "flight.h"
#include "queue.h"
#include "Dispcfg.h"
#include "FalcSnd/voicemanager.h"
#include "FalcSnd/voicefilter.h"
#include "remotelb.h"
#include "F4Find.h"
#include "fsound.h" // MLR for F4ReloadSFX
//me123
#include "campwp.h"
#include "ui/include/tac_class.h"
#include "ui/include/te_defs.h"
#include "TimerThread.h"
#include "acmi/src/include/acmirec.h"
#include "sim/include/simdrive.h"
// Begin - Uplink stuff
#include "include/comsup.h"
#include "fsound.h"
#include "SoundFX.h"
#include "graphics/include/drawparticlesys.h"
#include "SIM/include/sfx.h"

#pragma warning(disable:4192)
#import "gnet/bin/core.tlb"
#import "gnet/bin/shared.tlb" named_guids
#pragma warning(default:4192)

extern GNETCORELib::IUplinkPtr m_pUplink;
// End - Uplink stuff

enum
{
    MUTE_IMAGE        = 2000,
    ICON_IMAGE        = 2001,
    SND_HOMER   = 500034,
};


extern C_Handler *gMainHandler;
extern C_Parser *gMainParser;

extern int COLoaded;
extern int F4GameType;
int DoDFUpdate = 0;

extern int CommandsKeyCombo;
extern int CommandsKeyComboMod;

// Variables for use with Input functions
extern unsigned int chatterCount;
extern char chatterStr[256];
extern short AsciiAllowed;
extern unsigned int MaxInputLength;
extern void (*UseInputFn)();
extern void (*DiscardInputFn)();
// me123 for server remote control
extern void tactical_accept_mission(void);


//
static long gCommsUniqueID = 1;
long _IsF16_ = 0;

void GenericCloseWindowCB(long ID, short hittype, C_Base *control);
void GenericTimerCB(long ID, short hittype, C_Base *control);
void DeleteGroupList(long ID);
//void AddPlayerToGame(FalconSessionEntity *entity);
void RemovePlayerFromGame(VU_ID player);
//void MovePlayerAround(FalconSessionEntity *player);
void AddressInputCB(long ID, short hittype, C_Base *);
void ClearDFTeamLists();
void CloseWindowCB(long ID, short hittype, C_Base *control);
void LoadCommsWindows();
void CommsSetup();
void CopyPBToWindow(long ID, long Client);
void GetPlayerInfo(VU_ID ID);
void TallyPlayerSquadrons();
void RebuildGameTree();
void SetSingle_Comms_Ctrls();
void UI_Help_Guide_CB(long ID, short hittype, C_Base *ctrl);
void CheckFlyButton();
void CheckPlayersFlight(FalconSessionEntity *session);
BOOL AddWordWrapTextToWindow(C_Window *win, short *x, short *y, short startcol, short endcol, COLORREF color, _TCHAR *str, long Client = 0);
void MutePlayerCB(long ID, short hittype, C_Base *control);
void IgnorePlayerCB(long ID, short hittype, C_Base *control);
void UI_Refresh(void);
void DisplayLogbook(LB_PILOT *Pilot, IMAGE_RSC *Photo, IMAGE_RSC *Patch, BOOL EditFlag);

extern void CheckForNewPlayer(FalconSessionEntity *session);

// keep track of chat message Y position in window
static long CurChatY = 0;
static short PeopleChatType = 1;

C_TreeList *People = NULL;
C_TreeList *DogfightGames = NULL;
C_TreeList *TacticalGames = NULL;
C_TreeList *CampaignGames = NULL;

BOOL gNewMessage = FALSE;
extern short ViewLogBook;

static void HookupCommsControls(long ID);
extern void HookupServerBrowserControls(long ID);
static void UpdatePlayerListCB(short uptype, VU_ID GameID, VU_ID PlayerID);
static void SelectChatCB(long ID, short hittype, C_Base *control);
void RefreshGameListCB(long ID, short hittype, C_Base *control);
void BlinkCommsButtonTimerCB(long ID, short hittype, C_Base *control);
void UpdateDFPlayerList();
void BuildDFPlayerList();

void Phone_New_CB(long, short, C_Base *);
void Phone_Apply_CB(long, short, C_Base *);
void Phone_Remove_CB(long, short, C_Base *);
void Phone_Connect_CB(long, short, C_Base *);
void Phone_ConnectType_CB(long, short, C_Base *);

void ViewRemoteLBCB(long, short hittype, C_Base *)
{
    C_TreeList  *tree;
    TREELIST    *item;
    C_Player *plyr;
    UI_RequestLogbook *rlb;
    FalconSessionEntity *session;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (gPopupMgr->GetCallingType() == C_TYPE_CONTROL)
    {
        tree = (C_TreeList*)gPopupMgr->GetCallingControl();

        if (tree)
        {
            item = tree->GetLastItem();

            if (item)
            {
                plyr = (C_Player *)item->Item_;

                if (plyr)
                {
                    //if(plyr->GetVUID() not_eq FalconLocalSessionId)
                    {
                        // Go Ask for Logbook info bitand Open logbook...
                        // Log book should be EMPTY, until data appears...
                        // if(Player closes logbook before it's all received)
                        // cancel
                        session = (FalconSessionEntity*)vuDatabase->Find(plyr->GetVUID());

                        if (session)
                        {
                            rlb = new UI_RequestLogbook(FalconNullId, session);
                            rlb->dataBlock.fromID = FalconLocalSessionId;

                            FalconSendMessage(rlb, true);
                            ViewLogBook = 1;
                        }
                    }
                }
            }
        }
    }

    gPopupMgr->CloseMenu();
}

void CommsErrorDialog(long TitleID, long MessageID, void (*OKCB)(long, short, C_Base*), void (*CancelCB)(long, short, C_Base*))
{
    C_Window *win;
    C_Button *btn;
    C_Text *txt;

    if ( not MessageID or not gMainHandler)
        return;

    win = gMainHandler->FindWindow(COMMLINK_WIN);

    if (win)
    {
        txt = (C_Text*)win->FindControl(TITLE_LABEL);

        if (txt)
            txt->SetText(TitleID);

        btn = (C_Button *)win->FindControl(ALERT_CANCEL);

        if (btn)
        {
            if (OKCB)
            {
                btn->SetAllLabel(TXT_OK);
                btn->SetFlagBitOn(C_BIT_ENABLED);
                btn->SetCallback(OKCB);
            }
            else if (CancelCB)
            {
                btn->SetAllLabel(TXT_CANCEL);
                btn->SetFlagBitOn(C_BIT_ENABLED);
                btn->SetCallback(CancelCB);
            }
        }

        txt = (C_Text*)win->FindControl(COMMLINK_MESSAGE);

        if (txt)
            txt->SetText(MessageID);

        gMainHandler->ShowWindow(win);
        gMainHandler->WindowToFront(win);
    }
}

void UI_CommsErrorMessage(WORD error)
{
    long messageid = 0;

    switch (error)
    {
        case F4COMMS_CONNECTED:
            messageid = TXT_COMMS_CONNECTED;
            break;

        case F4COMMS_ERROR_TCP_NOT_AVAILABLE:
            messageid = TXT_COMMS_NO_TCP;
            break;

        case F4COMMS_ERROR_UDP_NOT_AVAILABLE:
            messageid = TXT_COMMS_NO_UDP;
            break;

        case F4COMMS_ERROR_MULTICAST_NOT_AVAILABLE:
            messageid = TXT_COMMS_NO_MULTICAST;
            break;

        case F4COMMS_ERROR_FAILED_TO_CREATE_GAME:
            messageid = TXT_COMMS_NO_GAME_CREATE;
            break;

        case F4COMMS_ERROR_COULDNT_CONNECT_TO_SERVER:
            messageid = TXT_COMMS_NO_SERVER;
            break;

        case F4COMMS_PENDING:
            messageid = TXT_COMMS_PENDING;
            break;
    }

    if (messageid)
        CommsErrorDialog(TXT_COMMS_TITLE, messageid, GenericCloseWindowCB, NULL);
}

void LoadCommsWindows()
{
    long ID;
    C_PopupList *menu;

    if (COLoaded) return;

    if (_LOAD_ART_RESOURCES_)
        gMainParser->LoadImageList("comm_res.lst");
    else
        gMainParser->LoadImageList("comm_art.lst");

    gMainParser->LoadSoundList("comm_snd.lst");
    gMainParser->LoadWindowList("comm_scf.lst"); // Modified by M.N. - add art/art1024 by LoadWindowList

    ID = gMainParser->GetFirstWindowLoaded();

    while (ID)
    {
        HookupCommsControls(ID);
        ID = gMainParser->GetNextWindowLoaded();
    }

    if (gCommsMgr)
        gCommsMgr->SetCallback(game_PlayerPool, UpdatePlayerListCB);

    COLoaded++;

    CommsSetup();

    menu = gPopupMgr->GetMenu(CHAT_POP);

    if (menu)
    {
        menu->SetCallback(MID_MUTE, MutePlayerCB);
        menu->SetCallback(MID_IGNORE, IgnorePlayerCB);
        menu->SetCallback(MID_LOGBOOK, ViewRemoteLBCB);
    }
}

void CommsSetup()
{
    C_Window *win;
    C_TimerHook *tmr;

    CurChatY = 0;

    win = gMainHandler->FindWindow(UI_MAIN_SCREEN);

    if (win)
    {
        tmr = new C_TimerHook;
        tmr->Setup(C_DONT_CARE, C_TYPE_TIMER);
        tmr->SetUpdateCallback(GenericTimerCB);
        tmr->SetRefreshCallback(BlinkCommsButtonTimerCB);
        tmr->SetUserNumber(_UI95_TIMER_DELAY_, 2 * _UI95_TICKS_PER_SECOND_);

        win->AddControl(tmr);
    }

    CopyPBToWindow(PB_WIN, 0);
}

static void DisconnectCommsCB(long, short hittype, C_Base *)
{
    if ((hittype not_eq C_TYPE_LMOUSEUP) or ( not gCommsMgr->Online()))
    {
        return;
    }

    gCommsMgr->StopComms();
    RebuildGameTree();
    UI_Refresh();
    SetSingle_Comms_Ctrls();
    CheckFlyButton();
}

static void OpenPhoneBookCB(long, short hittype, C_Base *control)
{
    C_Window *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gMainHandler->EnableWindowGroup(control->GetGroup());
    win = gMainHandler->FindWindow(PB_WIN);
    gMainHandler->WindowToFront(win);
}
extern bool g_bServer;
extern int MainLastGroup, TacLastGroup;
extern int PlannerLoaded, TACSelLoaded, CPSelectLoaded;
void LoadTacticalWindows(void);
void RemoveTacticalEdit(void);
void CleanupTacticalEngagementUI();
void DisableScenarioInfo();
void LeaveCurrentGame();
void LoadTacEngSelectWindows();
void LoadCampaignSelectWindows();
extern void ACMI_ImportFile(void);
extern int g_nmissiletrial;
extern bool g_bDrawBoundingBox;
extern float g_fSoundDopplerFactor, g_fSoundRolloffFactor;// MLR 2003-10-17
extern int g_nSoundUpdateMS; // MLR 2003/11/03

// RV - Biker - Who does need - we do
float g_nboostguidesec = 0; //me123 how many sec we are in boostguide mode
float g_nterminalguiderange = 0; //me123 what range we transfere to terminal guidence
float g_nboostguideSensorPrecision = 0; //me123
float g_nsustainguideSensorPrecision = 0 ; //me123
float g_nterminalguideSensorPrecision = 0 ; //me123
float g_nboostguideLead = 0 ; //me123
float g_nsustainguideLead = 0 ; //me123
float g_nterminalguideLead = 0 ; //me123
float g_nboostguideGnav = 0 ; //me123
float g_nsustainguideGnav = 0 ; //me123
float g_nterminalguideGnav = 0 ; //me123
float g_nboostguideBwap = 0; //me123
float g_nsustainguideBwap = 0; //me123
float g_nterminalguideBwap = 0 ; //me123
float g_nMpdelaytweakfactor = 0;

bool g_bDrawBoundingBox = false; //VP_changes if it is false BoundBoxes will not be drawn
int g_nmissiletrial = 0;
//extern float clientbwforupdatesmodifyer;
//extern float hostbwforupdatesmodifyer;
//extern float MinBwForOtherData;
extern int g_nShowDebugLabels;
//extern float Posupdsize ;
extern unsigned long gFuelState;
extern bool g_bActivateDebugStuff;
extern bool g_bNoSound;
extern bool g_bNoTrails;
extern void LoadTrails();

// me123 for remote server comands via chat
void ServerChatCommand(_TCHAR *msg)
{
    //if ( not g_bServer) return;
    //me123 the server understands a few commands

    // find the command
    char* arga;
    char* argb;
    _TCHAR message[100] = "";
    strncpy(message, msg, sizeof(message) - 1); // JPO - lets me careful out there ;-)
    message[99] = '\0';
    arga = strtok(message, " ");
    argb = strtok(NULL, " ");


    if (g_nShowDebugLabels)
    {
        if (arga and argb and not stricmp(arga, ".trail"))
        {
            if (atoi(argb))
            {
                g_bNoTrails = 0;
            }
            else
            {
                g_bNoTrails = 1;
            }

            //F4SoundFXSetDist(SFX_THUNDER,0,0,1);
        }

        if (arga and not stricmp(arga, ".trailreload"))
        {
            LoadTrails();
        }

        if (arga and not stricmp(arga, ".psreload"))
        {
            DrawableParticleSys::LoadParameters();
        }

        if (arga and not stricmp(arga, ".sndreload"))
        {
            F4ReloadSFX();
            //F4SoundFXSetDist(SFX_THUNDER,0,0,1);
        }

        if (arga and argb and not stricmp(arga, ".snd"))
        {
            if (atoi(argb))
            {
                g_bNoSound = 0;
            }
            else
            {
                g_bNoSound = 1;
            }

            //F4SoundFXSetDist(SFX_THUNDER,0,0,1);
        }

        if (arga and argb and not stricmp(arga, ".sndms"))
        {
            g_nSoundUpdateMS = atoi(argb);
            //F4SoundFXSetDist(SFX_THUNDER,0,0,1);
        }

        if (arga and argb and not stricmp(arga, ".snddop"))
        {
            g_fSoundDopplerFactor = (float)atof(argb);
            //F4SoundFXSetDist(SFX_THUNDER,0,0,1);
        }

        if (arga and argb and not stricmp(arga, ".sndro"))
        {
            g_fSoundRolloffFactor = (float)atof(argb);
            //F4SoundFXSetDist(SFX_THUNDER,0,0,1);
        }

        // COBRA - RED - SFX Activating cheat '.sfx {SfxNr}'
        if ((arga and argb and not stricmp(arga, ".sfx")) or (arga and not stricmp(arga, ".")))
        {
            static int sfx = 0;
            static float Dist = 300;
            static Tpoint vc = OTWDriver.GetEyePosition();;
            char* argc;
            argc = strtok(NULL, " ");

            float Pan, Tilt, d = 300;

            if ( not stricmp(arga, ".")) goto doit;

            if (argc) Dist = (float)atof(argc);
            else Dist = 300.0f;

            //vc.x=currentPos_.x;
            sfx = atoi(argb);
            //vc=OTWDriver.GetEyePosition();
            OTWDriver.GetCameraPanTilt(&Pan, &Tilt);
            vc.x = ObserverPosition.x - Dist * cos(Pan) * cos(Tilt);
            vc.y = ObserverPosition.y - Dist * sin(Pan) * cos(Tilt);
            vc.z = ObserverPosition.z + Dist * sin(Tilt);

        doit:

            OTWDriver.AddSfxRequest(
                new SfxClass(sfx, // type
                             &vc, // world pos
                             60.0f, // time to live
                             1.0f));
        }


        // 2002-02-21 MN Allow to change the set of debug labels via the chat line
        if (arga and argb and not stricmp(arga, ".label") and g_bActivateDebugStuff)
        {
            int newlabels = strtol(argb, NULL, 0); // atoi(argb); 2002-04-01 MODIFIED BY S.G. strtol will parse the inpuy string looking for standard base like 0x
            g_nShowDebugLabels = newlabels;
        }

        // Changes fuel level of players aircraft - for refuel testings
        if (arga and argb and not stricmp(arga, ".fuel") and g_bActivateDebugStuff)
        {
            unsigned long newfuel = atol(argb);
            gFuelState = newfuel;
        }

        if (g_bServer)
        {
            if (arga and argb and not stricmp(arga, ".loadte"))
            {
                DisableScenarioInfo();
                LeaveCurrentGame();
                RuleMode = rTACTICAL_ENGAGEMENT;
                TheCampaign.Flags or_eq CAMP_TACTICAL;

                if ( not TACSelLoaded)
                    LoadTacEngSelectWindows();

                _TCHAR buffer[MAX_PATH];
                strcpy(buffer, FalconCampaignSaveDirectory);
                strcat(buffer, "\\");
                strcat(buffer, argb);
                strcat(buffer, ".tac");
                current_tactical_mission = new tactical_mission(buffer);
                LoadTacticalWindows();
                gMainHandler->EnterCritical();
                FalconLocalSession->SetCountry(2);
                tactical_accept_mission();
                gMainHandler->LeaveCritical();
                SetTimeCompression(0);
                MainLastGroup = 3000;
            }

            if (arga and argb and not stricmp(arga, ".acmi"))
            {
                if ( not stricmp(argb, "on"))
                {
                    if ( not gACMIRec.IsRecording())
                    {
                        //F4EnterCriticalSection( _csect );
                        gACMIRec.StartRecording();
                        //F4LeaveCriticalSection( _csect );
                    }
                }
                else if ( not stricmp(argb, "off"))
                {
                    if (gACMIRec.IsRecording())
                    {
                        //F4EnterCriticalSection( _csect );
                        gACMIRec.StopRecording();
                        //F4LeaveCriticalSection( _csect );
                    }
                }

                else if ( not stricmp(argb, "dofile"))
                {
                    if (gACMIRec.IsRecording())
                        gACMIRec.StopRecording();

                    ACMI_ImportFile();
                }
            }

            if (0 and arga and argb and not stricmp(arga, ".loadcam"))
            {
                DisableScenarioInfo();
                LeaveCurrentGame();
                RuleMode = rCAMPAIGN;

                if ( not CPSelectLoaded)
                    LoadCampaignSelectWindows();

                _TCHAR buffer[MAX_PATH];
                strcpy(buffer, FalconCampaignSaveDirectory);
                strcat(buffer, argb);
                strcat(buffer, ".cam");
                current_tactical_mission = new tactical_mission(buffer);
                LoadTacticalWindows();
                gMainHandler->EnterCritical();
                FalconLocalSession->SetCountry(2);
                tactical_accept_mission();
                gMainHandler->LeaveCritical();
                SetTimeCompression(0);
                MainLastGroup = 3000;
            }

            if (arga and not stricmp(arga, ".quit"))
            {
                tactical_mission_loaded = FALSE;
                RemoveTacticalEdit();
                gMainHandler->EnterCritical();
                TheCampaign.EndCampaign();
                CleanupTacticalEngagementUI();
                TacLastGroup = 0;
                gMainHandler->DisableSection(200);
                gMainHandler->SetSection(100);
                gMainHandler->EnableWindowGroup(100);
                gMainHandler->EnableWindowGroup(MainLastGroup);
                gMainHandler->LeaveCritical();
            }
        }

        if (arga and argb and not stricmp(arga, ".mistrail") and g_bActivateDebugStuff)
        {
            g_nmissiletrial = atoi(argb);
        }
        else if (arga and not stricmp(arga, ".boundb") and g_bActivateDebugStuff)
        {
            if (g_bDrawBoundingBox)
                g_bDrawBoundingBox = false;
            else
                g_bDrawBoundingBox = true;
        }
        // RV - Biker - Who does need - we do
        else if (arga and argb and not stricmp(arga, ".bgs") and g_bActivateDebugStuff)
        {
            g_nboostguidesec = (float)atof(argb);
        }
        else if (arga and argb and not stricmp(arga, ".tgr") and g_bActivateDebugStuff)
        {
            g_nterminalguiderange = (float)atof(argb);
        }
        else if (arga and argb and not stricmp(arga, ".bgsp") and g_bActivateDebugStuff)
        {
            g_nboostguideSensorPrecision = (float)atof(argb);
        }
        else if (arga and argb and not stricmp(arga, ".sgsp") and g_bActivateDebugStuff)
        {
            g_nsustainguideSensorPrecision = (float)atof(argb);
        }
        else if (arga and argb and not stricmp(arga, ".tgsp") and g_bActivateDebugStuff)
        {
            g_nterminalguideSensorPrecision = (float)atof(argb);
        }
        else if (arga and argb and not stricmp(arga, ".bgl") and g_bActivateDebugStuff)
        {
            g_nboostguideLead = (float)atof(argb);
        }
        else if (arga and argb and not stricmp(arga, ".sgl") and g_bActivateDebugStuff)
        {
            g_nsustainguideLead = (float)atof(argb);
        }
        else if (arga and argb and not stricmp(arga, ".tgl") and g_bActivateDebugStuff)
        {
            g_nterminalguideLead = (float)atof(argb);
        }
        else if (arga and argb and not stricmp(arga, ".bggn") and g_bActivateDebugStuff)
        {
            g_nboostguideGnav = (float)atof(argb);
        }
        else if (arga and argb and not stricmp(arga, ".sggn") and g_bActivateDebugStuff)
        {
            g_nsustainguideGnav = (float)atof(argb);
        }
        else if (arga and argb and not stricmp(arga, ".tggn") and g_bActivateDebugStuff)
        {
            g_nterminalguideGnav = (float)atof(argb);
        }
        else if (arga and argb and not stricmp(arga, ".bgbw") and g_bActivateDebugStuff)
        {
            g_nboostguideBwap = (float)atof(argb);
        }
        else if (arga and argb and not stricmp(arga, ".sgbw") and g_bActivateDebugStuff)
        {
            g_nsustainguideBwap = (float)atof(argb);
        }
        else if (arga and argb and not stricmp(arga, ".tgbw") and g_bActivateDebugStuff)
        {
            g_nterminalguideBwap = (float)atof(argb);
        }
        else if (arga and argb and not stricmp(arga, ".cbw") and g_bActivateDebugStuff)
        {
            //clientbwforupdatesmodifyer = float(atoi(argb)/1000.0f);
        }
        else if (arga and argb and not stricmp(arga, ".hbw") and g_bActivateDebugStuff)
        {
            //hostbwforupdatesmodifyer = float(atoi(argb)/1000.0f);
        }
        else if (arga and argb and not stricmp(arga, ".tf") and g_bActivateDebugStuff)
        {
            g_nMpdelaytweakfactor = float(atoi(argb));
        }
        else if (arga and argb and not stricmp(arga, ".mbw") and g_bActivateDebugStuff)
        {
            //MinBwForOtherData = float(atoi(argb));
        }
        else if (arga and argb and not stricmp(arga, ".pos"))
        {
            //Posupdsize = float(atoi(argb));
        }
    } // end g_nShowDebugLabels
}

static _TCHAR chatbuf[512];

void AddMessageToChatWindow(VU_ID from, _TCHAR *message)
{
    C_Window            *win;
    C_Text              *txt;
    FalconSessionEntity *session;
    COLORREF             color;
    win = gMainHandler->FindWindow(CHAT_WIN);
    ServerChatCommand(message); //me123

    if (win)
    {
        if (from == FalconLocalSessionId)
        {
            color = 0xcccccc;
            //session=(FalconSessionEntity*)vuLocalSessionEntity;
            session = FalconLocalSession;
        }
        else if (from not_eq FalconNullId)
        {
            color = 0x00ff00;
            session = (FalconSessionEntity*)vuDatabase->Find(from);
        }
        else
        {
            color = 0x0000ff;
            session = NULL;
        }

        if (session)
        {
            _tcscpy(chatbuf, session->GetPlayerCallsign());
            _tcscat(chatbuf, ": ");
        }
        else if (from not_eq FalconNullId)
        {
            _tcscpy(chatbuf, "Unknown");
            _tcscat(chatbuf, ": ");
        }
        else
            chatbuf[0] = 0;

        _tcscat(chatbuf, message);

        txt = new C_Text;
        txt->Setup(C_DONT_CARE, C_TYPE_NORMAL);
        txt->SetFixedWidth(_tcsclen(chatbuf) + 1);
        txt->SetFont(win->Font_);
        txt->SetText(chatbuf);
        txt->SetXY(3, CurChatY);
        txt->SetW(330);
        txt->SetFGColor(color);
        txt->SetClient(2);
        txt->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
        txt->SetFlagBitOn(C_BIT_LEFT bitor C_BIT_WORDWRAP);
        win->AddControl(txt);
        txt->Refresh();

        CurChatY += txt->GetH(); // - gFontList->GetHeight(win->Font_);

        win->SetVirtualY(txt->GetY() - win->ClientArea_[txt->GetClient()].top, txt->GetClient());
        win->AdjustScrollbar(txt->GetClient());
        win->RefreshClient(txt->GetClient());

        win->ScanClientArea(2);
        // gSoundMgr->PlaySound(SND_HOMER);
    }
}

void BlinkCommsButtonTimerCB(long, short, C_Base *control)
{
    C_Button *btn;

    if ( not gCommsMgr) return;

    if ( not gNewMessage or not gCommsMgr->Online()) return;

    btn = (C_Button *)control->Parent_->FindControl(CO_MAIN_CTRL);

    if (btn)
    {
        if ( not (btn->GetFlags() bitand C_BIT_FORCEMOUSEOVER))
        {
            // gSoundMgr->PlaySound(SND_HOMER);
            btn->SetFlagBitOn(C_BIT_FORCEMOUSEOVER);
        }
        else
            btn->SetFlagBitOff(C_BIT_FORCEMOUSEOVER);

        btn->Refresh();
    }
}

BOOL FilterIncommingMessage(FalconSessionEntity *session)
{
    Package pkg;
    Flight flt;
    VU_ID MyPackageID;
    BOOL retval = FALSE;
    float myx, sx, myy, sy, dx, dy, dist;
    VuEntity *ent;

    if (VM)
    {
        switch (VM->GetRadioFreq(0))
        {
            case rcfFlight5:
            case rcfFlight1:
            case rcfFlight2:
            case rcfFlight3:
            case rcfFlight4:

                if (session->GetPlayerFlightID() == FalconLocalSession->GetPlayerFlightID())
                    retval = TRUE;

                break;

            case rcfPackage5:
            case rcfPackage1:
            case rcfPackage2:
            case rcfPackage3:
            case rcfPackage4:

            case rcfFromPackage:
                flt = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());

                if ( not flt)
                    break;

                pkg = (Package)flt->GetUnitParent();

                if ( not pkg)
                    break;

                MyPackageID = pkg->Id();

                flt = (Flight)vuDatabase->Find(session->GetPlayerFlightID());

                if (flt)
                {
                    pkg = (Package)flt->GetUnitParent();

                    if (pkg and pkg->Id() == MyPackageID)
                        retval = TRUE;
                }

                break;

            case rcfProx: // 40nm range
                ent = FalconLocalSession->GetPlayerEntity();

                if ( not ent)
                    break;

                myx = ent->XPos();
                myy = ent->YPos();

                ent = session->GetPlayerEntity();

                if (ent)
                {
                    sx = ent->XPos();
                    sy = ent->YPos();

                    dx = myx - sx;
                    dy = myy - sy;

                    dist = static_cast<float>(sqrt(dx * dx + dy * dy) * FT_TO_NM);

                    if (dist <= 40.0f)
                        retval = TRUE;
                }

                break;

            case rcfTeam:
                if (session->GetTeam() == FalconLocalSession->GetTeam())
                    retval = TRUE;

                break;

            case rcfAll:
                retval = TRUE;
                break;
        }

        switch (VM->GetRadioFreq(1))
        {
            case rcfFlight5:
            case rcfFlight1:
            case rcfFlight2:
            case rcfFlight3:
            case rcfFlight4:

                if (session->GetPlayerFlightID() == FalconLocalSession->GetPlayerFlightID())
                    retval = TRUE;

                break;

            case rcfPackage5:
            case rcfPackage1:
            case rcfPackage2:
            case rcfPackage3:
            case rcfPackage4:

            case rcfFromPackage:
                flt = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());

                if ( not flt)
                    break;

                pkg = (Package)flt->GetUnitParent();

                if ( not pkg)
                    break;

                MyPackageID = pkg->Id();

                flt = (Flight)vuDatabase->Find(session->GetPlayerFlightID());

                if (flt)
                {
                    pkg = (Package)flt->GetUnitParent();

                    if (pkg and pkg->Id() == MyPackageID)
                        retval = TRUE;
                }

                break;

            case rcfProx: // 40nm range
                ent = FalconLocalSession->GetPlayerEntity();

                if ( not ent)
                    break;

                myx = ent->XPos();
                myy = ent->YPos();

                ent = session->GetPlayerEntity();

                if (ent)
                {
                    sx = ent->XPos();
                    sy = ent->YPos();

                    dx = myx - sx;
                    dy = myy - sy;

                    dist = static_cast<float>(sqrt(dx * dx + dy * dy) * FT_TO_NM);

                    if (dist <= 40.0f)
                        retval = TRUE;
                }

                break;

            case rcfTeam:
                if (session->GetTeam() == FalconLocalSession->GetTeam())
                    retval = TRUE;

                break;

            case rcfAll:
                retval = TRUE;
                break;
        }
    }

    return(retval);
}
// This function creates a CHATSTR class... which gets destroyed in ProcessChatStr()



void ReceiveChatString(VU_ID from, _TCHAR *message)
{
    CHATSTR *msg;

    msg = new CHATSTR;
    msg->ID_ = from;
    // OW - allocation did not include the terminating zero char
#if 0
    msg->Text_ = new _TCHAR[_tcsclen(message)];
#else
    msg->Text_ = new _TCHAR[_tcsclen(message) + 1];
#endif

    _tcscpy(msg->Text_, message);
    PostMessage(gCommsMgr->AppWnd_, FM_RECEIVE_CHAT, 0, (long)msg);


}


// This function is responsible for deleting msg
void ProcessChatStr(CHATSTR *msg)
{
    if ( not msg)
        return;

    ServerChatCommand(msg->Text_); //me123

    if (gMainHandler) // Assume UI is running
    {
        TREELIST *item;
        C_Player *plyr;

        if ( not People)
            return;

        item = People->Find(msg->ID_.creator_);

        if (item)
        {
            plyr = (C_Player*)item->Item_;

            if (plyr and (plyr->GetMute())) // Filter out Chat messages we don't want to hear
                return;
        }

        AddMessageToChatWindow(msg->ID_, msg->Text_);

        if ( not (gMainHandler->GetWindowFlags(CHAT_WIN) bitand C_BIT_ENABLED))
            gNewMessage = TRUE;
    }
    else if (VM) // Assume Sim is running (AND VM is initialized)
    {
        FalconSessionEntity *session;

        session = (FalconSessionEntity*)vuDatabase->Find(msg->ID_);

        if (session)
        {
            if ( not FilterIncommingMessage(session))
                return;

            _tcscpy(chatbuf, session->GetPlayerCallsign());
            _tcscat(chatbuf, ": ");
        }
        else if (msg->ID_ not_eq FalconNullId)
        {
            _tcscpy(chatbuf, "Unknown");
            _tcscat(chatbuf, ": ");
        }
        else
            chatbuf[0] = '\0';

        _tcscat(chatbuf, msg->Text_);

        OTWDriver.ShowMessage(chatbuf);
    }

    if (msg->Text_) delete[] msg->Text_; // OW - the message string was not deleted resulting in a memory leak

    delete msg;
}

void SendChatStringCB(long, short hittype, C_Base *control)
{
    TREELIST *cur;
    C_Player *plyr;
    UI_SendChatMessage *chat;

    if (hittype not_eq DIK_RETURN or control == NULL or not gCommsMgr->Online())
        return;

    AddMessageToChatWindow(FalconLocalSessionId, ((C_EditBox *)control)->GetText());

    cur = People->GetRoot();

    while (cur)
    {
        if (cur->Type_ == C_TYPE_ITEM)
        {
            plyr = (C_Player*)cur->Item_;

            if (plyr and plyr->GetState() and not plyr->GetIgnore())
            {
                FalconSessionEntity *session = (FalconSessionEntity*) vuDatabase->Find(plyr->GetVUID());

                if (session and session not_eq FalconLocalSession)
                {
                    chat = new UI_SendChatMessage(FalconNullId, session);

                    chat->dataBlock.from = FalconLocalSessionId;
                    chat->dataBlock.size = static_cast<short>((_tcsclen(((C_EditBox *)control)->GetText()) + 1) * sizeof(_TCHAR));
                    chat->dataBlock.message = new _TCHAR[chat->dataBlock.size];
                    memcpy(chat->dataBlock.message, ((C_EditBox *)control)->GetText(), chat->dataBlock.size);
                    FalconSendMessage(chat, TRUE);
                }
            }
        }

        cur = cur->Next;
    }

    ((C_EditBox*)control)->SetText(TXT_SPACE);
    control->Refresh();
}

static void SetOnlineStatus(long ID)
{
    C_Window *win;
    C_Base *ctrl;

    win = gMainHandler->FindWindow(ID);

    if (win)
    {
        ctrl = win->FindControl(SINGLE_FLY_CTRL);

        if (ctrl)
        {
            ctrl->Refresh();

            if (gCommsMgr->Online())
                ctrl->SetReady(0);
            else
                ctrl->SetReady(1);

            ctrl->Refresh();
        }

        ctrl = win->FindControl(SINGLE_COMMIT_CTRL);

        if (ctrl)
        {
            ctrl->Refresh();

            if (gCommsMgr->Online())
                ctrl->SetReady(0);
            else
                ctrl->SetReady(1);

            ctrl->Refresh();
        }

        ctrl = win->FindControl(COMMS_FLY_CTRL);

        if (ctrl)
        {
            ctrl->Refresh();

            if (gCommsMgr->Online())
                ctrl->SetReady(1);
            else
                ctrl->SetReady(0);

            ctrl->Refresh();
        }

        ctrl = win->FindControl(COMMS_COMMIT_CTRL);

        if (ctrl)
        {
            ctrl->Refresh();

            if (gCommsMgr->Online())
                ctrl->SetReady(1);
            else
                ctrl->SetReady(0);

            ctrl->Refresh();
        }

        ctrl = win->FindControl(DF_OFFLINE_CTRL);

        if (ctrl)
        {
            ctrl->Refresh();

            if (gCommsMgr->Online())
                ctrl->SetReady(0);
            else
                ctrl->SetReady(1);

            ctrl->Refresh();
        }

        ctrl = win->FindControl(DF_HOST_CTRL);

        if (ctrl)
        {
            ctrl->Refresh();

            if (gCommsMgr->Online())
                ctrl->SetReady(1);
            else
                ctrl->SetReady(0);

            ctrl->Refresh();
        }
    }

#if 0

    if (People)
    {
        if (gCommsMgr->Online())
            People->SetReady(1);
        else
            People->SetReady(0);

        if (People->GetParent())
            People->GetParent()->RefreshClient(People->GetClient());
    }

#endif
}


void SetSingle_Comms_Ctrls()
{
    // SetOnlineStatus(IA_TOOLBAR_WIN); // Single player ONLY for Instant Action
    SetOnlineStatus(DF_TOOLBAR_WIN);
    SetOnlineStatus(DF_PLAY_TOOLBAR_WIN);
    SetOnlineStatus(TAC_TOOLBAR_WIN);
    SetOnlineStatus(TAC_LOAD_TOOLBAR);
    SetOnlineStatus(CP_TOOLBAR);
    SetOnlineStatus(CS_TOOLBAR_WIN);
}

static VU_ID SearchID;
static VU_ID *tmpID;

BOOL TreeSearchCB(TREELIST *item)
{
    if (item == NULL) return(FALSE);

    if (item->Item_ == NULL) return(FALSE);

    if (item->Type_ == C_TYPE_ITEM)
    {
        if (((C_Player*)item->Item_)->GetVUID() == SearchID)
            return(TRUE);
    }
    else if (item->Type_ == C_TYPE_MENU)
    {
        tmpID = (VU_ID*)item->Item_->GetUserPtr(_UI95_VU_ID_SLOT_);

        if (tmpID == NULL)
            return(FALSE);

        if (*tmpID == SearchID)
            return(TRUE);
    }

    return(FALSE);
}

TREELIST *StartTreeSearch(VU_ID findme, TREELIST *top, C_TreeList *tree)
{
    SearchID = findme;
    tree->SetSearchCB(TreeSearchCB);
    return(tree->SearchWithCB(top));
}

static void UpdatePlayerListCB(short, VU_ID, VU_ID)
{
}

static BOOL TrimmerFindSession(VU_ID playerid, VuGameEntity *game)
{
    VuSessionsIterator sessionWalker(game);
    FalconSessionEntity *session;

    session = (FalconSessionEntity*)sessionWalker.GetFirst();

    while (session)
    {
        if (session->Id() == playerid)
            return(TRUE);

        session = (FalconSessionEntity*)sessionWalker.GetNext();
    }

    return(FALSE);
}

static void TrimPlayerTree(C_TreeList *tree, TREELIST *branch)
{
    VU_ID *tmpID;
    TREELIST *limb;
    VuGameEntity *game;
    C_Player *plyr;

    limb = NULL;

    while (branch)
    {
        if (branch->Type_ == C_TYPE_MENU)
        {
            tmpID = (VU_ID*)branch->Item_->GetUserPtr(_UI95_VU_ID_SLOT_);

            if (tmpID) // this is a game vu_id
            {
                game = (VuGameEntity*)vuDatabase->Find(*tmpID);

                if (game == NULL)
                    limb = branch;
                else if (branch->Child)
                    TrimPlayerTree(tree, branch->Child);
            }
            else
                TrimPlayerTree(tree, branch->Child);
        }
        else if (branch->Type_ == C_TYPE_ITEM)
        {
            tmpID = (VU_ID*)branch->Parent->Item_->GetUserPtr(_UI95_VU_ID_SLOT_);

            if (tmpID == NULL)
                game = vuPlayerPoolGroup;
            else
                game = (VuGameEntity*)vuDatabase->Find(*tmpID);

            plyr = (C_Player*)branch->Item_;

            if ( not TrimmerFindSession(plyr->GetVUID(), game))
                limb = branch;
        }

        branch = branch->Next;

        if (limb)
        {
            tree->DeleteItem(limb);
            limb = NULL;
        }
    }
}

static TREELIST *AddGameToList(VuGameEntity *game, TREELIST *parent, C_TreeList *tree)
{
    C_Button *btn;
    TREELIST *group;
    VU_ID *tmpID;
    _TCHAR *name;

    name = game->GameName();

    btn = new C_Button;
    btn->Setup(C_DONT_CARE, C_TYPE_CUSTOM, 0, 0);
    btn->SetFlagBitOn(C_BIT_ENABLED);
    btn->SetText(C_STATE_0, name);
    btn->SetText(C_STATE_1, name);
    btn->SetText(C_STATE_2, name);
    btn->SetFgColor(C_STATE_0, 0x00e0e0e0);
    btn->SetFgColor(C_STATE_1, 0x0000ff00);
    btn->SetFgColor(C_STATE_2, 0x0000ffff);
    tmpID = new VU_ID;
    *tmpID = game->Id();
    btn->SetUserCleanupPtr(_UI95_VU_ID_SLOT_, tmpID);

    group = tree->CreateItem(gCommsUniqueID++, C_TYPE_MENU, btn);

    if (parent)
        tree->AddChildItem(parent, group);
    else
    {
        tree->AddItem(tree->GetRoot(), group);

        if (FalconLocalGame == game)
            btn->SetState(2);
    }

    if (game == gCommsMgr->GetTargetGame())
        btn->Process(0, C_TYPE_LMOUSEUP);

    return(group);
}

static TREELIST *CreatePlayerButton(C_TreeList *tree, FalconSessionEntity *session)
{
    TREELIST *item;
    C_Player *player;
    TCHAR *aux = session->GetPlayerCallsign();

    player = new C_Player;
    player->Setup(session->Id().creator_, 0);
    player->InitEntity();
    player->SetFont(tree->GetFont());
    player->SetIcon(8, 8, ICON_IMAGE);
    player->SetName(20, 0, aux);

    if (session == FalconLocalSession)
        player->SetColor(0x0000ffff, 0x0000ffff, 0x0000ffff);
    else
    {
        player->SetColor(0x00e0e0e0, 0x0000ff00, 0x0000aa);
        player->SetStatus(8, 8, MUTE_IMAGE);
    }

    player->SetVUID(session->Id());

    item = tree->CreateItem(session->Id().creator_, C_TYPE_ITEM, player);
    return(item);
}


void UpdateGameTreeBranch(long branchid, VuGameEntity *game, C_TreeList *tree, TREELIST *parent, BOOL IsChild) // branchid is the game_<GameType> therefore add 1 for valid ID
{
    TREELIST *group, *player;
    C_Button *btn;

    if (game == NULL)
        return;

    if (branchid not_eq game_PlayerPool)
    {
        group = StartTreeSearch(game->Id(), parent, tree);

        if (group == NULL)
        {
            if (IsChild)
                group = AddGameToList(game, parent, tree);
            else
                group = AddGameToList(game, NULL, tree);
        }
        else
        {
            btn = (C_Button*)group->Item_;

            if (btn)
            {
                if (strcmp(game->GameName(), btn->GetText(C_STATE_0)))
                {
                    btn->SetText(C_STATE_0, gStringMgr->GetText(gStringMgr->AddText(game->GameName())));
                    btn->SetText(C_STATE_1, gStringMgr->GetText(gStringMgr->AddText(game->GameName())));
                }
            }
        }

        parent = group;
    }

    VuSessionsIterator sessionWalker(game);
    FalconSessionEntity *session;

    session = (FalconSessionEntity*)sessionWalker.GetFirst();

    while (session)
    {
        player = StartTreeSearch(session->Id(), parent, tree);

        if (player == NULL)
        {
            player = CreatePlayerButton(tree, session);
            tree->AddChildItem(parent, player);
        }

        session = (FalconSessionEntity*)sessionWalker.GetNext();
    }
}


static void RemoveOldPeopleTreeCB(TREELIST *old)
{
    _TCHAR buffer[60];

    _stprintf(buffer, "%s %s", ((C_Player*)old->Item_)->GetName(), gStringMgr->GetString(TXT_LEFT_GAME));

    ReceiveChatString(FalconNullId, buffer);

    // Begin Uplink stuff
    if (m_pUplink not_eq NULL and FalconLocalGame and FalconLocalGame->IsLocal())
    {
        try
        {
            m_pUplink->RemovePlayer(((C_Player*)old->Item_)->GetName());
        }

        catch (_com_error e)
        {
            MonoPrint("StartCampaignGame: Error 0x%X occured during UpLink startup", e.Error());
        }
    }

    // End Uplink stuff
}

// Call this function EVERYTIME we move to a different game
void MakeLocalGameTree(VuGameEntity *game)
{
    C_Window *win;
    C_Button *btn;
    TREELIST *item;
    C_Text *txt;
    VU_ID *tmpID;

    if ( not People)
        return;

    if (People->GetRoot())
        People->DeleteBranch(People->GetRoot());

    win = gMainHandler->FindWindow(CHAT_WIN);

    if ( not win)
        return;

    if (game)
    {
        txt = new C_Text;
        txt->Setup(C_DONT_CARE, 0);
        txt->SetFGColor(0xeeeeee);
        txt->SetFixedWidth(30);
        txt->SetFont(People->GetFont());

        if (game == (VuGameEntity*)vuPlayerPoolGroup)
        {
            txt->SetText(TXT_PLAYERPOOL);
            win->DisableCluster(1);
            win->DisableCluster(2);
            win->DisableCluster(3);
            win->DisableCluster(4);
            win->RefreshWindow();
        }
        else
        {
            txt->SetText(game->GameName());
            tmpID = new VU_ID;
            *tmpID = game->Id();
            txt->SetUserCleanupPtr(_UI95_VU_ID_SLOT_, tmpID);

            switch (((FalconGameEntity*)game)->GetGameType())
            {
                case game_Dogfight:
                    win->DisableCluster(3);
                    win->EnableCluster(1);
                    win->EnableCluster(2);
                    win->EnableCluster(4);
                    win->RefreshWindow();
                    break;

                case game_TacticalEngagement:
                case game_Campaign:
                    win->EnableCluster(1);
                    win->EnableCluster(2);
                    win->EnableCluster(3);
                    win->EnableCluster(4);
                    win->RefreshWindow();
                    break;
            }
        }

        item = People->CreateItem(1, C_TYPE_ROOT, txt);

        if (item)
            People->AddItem(People->GetRoot(), item);

        btn = (C_Button*)win->FindControl(CHAT_ALL);

        if (btn)
            btn->SetState(1);

        btn = (C_Button*)win->FindControl(CHAT_TEAM);

        if (btn)
            btn->SetState(0);

        btn = (C_Button*)win->FindControl(CHAT_PACKAGE);

        if (btn)
            btn->SetState(0);

        btn = (C_Button*)win->FindControl(CHAT_FLIGHT);

        if (btn)
            btn->SetState(0);

        btn = (C_Button*)win->FindControl(CHAT_DISCONNECT);

        if (btn)
        {
            if (game == vuPlayerPoolGroup)
                btn->SetFlagBitOff(C_BIT_INVISIBLE);
            else
                btn->SetFlagBitOn(C_BIT_INVISIBLE);
        }

        PeopleChatType = 1;
        DeleteGroupList(CHAT_WIN);
        CurChatY = 0;
    }
}

static void CheckPlayerGroup(FalconSessionEntity *session, C_Player *plyr)
{
    Flight player, me;

    if (session == FalconLocalSession)
    {
        plyr->SetState(1);
        plyr->SetMute(0);
    }
    else if (PeopleChatType == 1) // CHAT_ALL
    {
        plyr->SetState(1);
        plyr->SetMute(0);
    }
    else if (PeopleChatType == 2) // CHAT_TEAM
    {
        if (FalconLocalSession->GetTeam() == session->GetTeam())
        {
            plyr->SetState(1);
            plyr->SetMute(0);
        }
        else
        {
            plyr->SetState(0);
            plyr->SetMute(1);
        }
    }
    else if (PeopleChatType == 3) // CHAT_PACKAGE (ONLY Campaign bitand TE)
    {
        me = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());
        player = (Flight)vuDatabase->Find(session->GetPlayerFlightID());

        if (me and player)
        {
            if (me->GetUnitPackage() == player->GetUnitPackage())
            {
                plyr->SetState(1);
                plyr->SetMute(0);
            }
            else
            {
                plyr->SetState(0);
                plyr->SetMute(1);
            }
        }
        else
        {
            plyr->SetState(0);
            plyr->SetMute(1);
        }
    }
    else if (PeopleChatType == 4) // CHAT_FLIGHT
    {
        if (FalconLocalSession->GetPlayerFlightID() == session->GetPlayerFlightID() and FalconLocalSession->GetPlayerFlightID() not_eq FalconNullId)
        {
            plyr->SetState(1);
            plyr->SetMute(0);
        }
        else
        {
            plyr->SetState(0);
            plyr->SetMute(1);
        }
    }
    else // Ignore them
    {
        plyr->SetState(0);
        plyr->SetMute(1);
    }
}

void CheckChatFilters(FalconSessionEntity *session)
{
    TREELIST *item;
    C_Player *player;

    if ( not People)
        return;

    item = People->Find(session->Id().creator_);

    if (item)
    {
        player = (C_Player*)item->Item_;

        if (player)
        {
            player->SetName(session->GetPlayerCallsign());
            CheckPlayerGroup(session, player);
            player->Refresh();
        }
    }
}

void MutePlayerCB(long, short hittype, C_Base *)
{
    C_TreeList  *tree;
    TREELIST    *item;
    C_Player *plyr;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (gPopupMgr->GetCallingType() == C_TYPE_CONTROL)
    {
        tree = (C_TreeList*)gPopupMgr->GetCallingControl();

        if (tree)
        {
            item = tree->GetLastItem();

            if (item)
            {
                plyr = (C_Player *)item->Item_;

                if (plyr)
                {
                    if (plyr->GetVUID() not_eq FalconLocalSessionId)
                    {
                        if (plyr->GetMute())
                            plyr->SetMute(0);
                        else
                            plyr->SetMute(1);
                    }
                }
            }
        }
    }

    gPopupMgr->CloseMenu();
}

void IgnorePlayerCB(long, short hittype, C_Base *)
{
    C_TreeList  *tree;
    TREELIST    *item;
    C_Player *plyr;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (gPopupMgr->GetCallingType() == C_TYPE_CONTROL)
    {
        tree = (C_TreeList*)gPopupMgr->GetCallingControl();

        if (tree)
        {
            item = tree->GetLastItem();

            if (item)
            {
                plyr = (C_Player *)item->Item_;

                if (plyr)
                {
                    if (plyr->GetVUID() not_eq FalconLocalSession->Id())
                    {
                        if (plyr->GetIgnore())
                            plyr->SetIgnore(0);
                        else
                            plyr->SetIgnore(1);
                    }
                }
            }
        }
    }

    gPopupMgr->CloseMenu();
}

void UpdateLocalGameTree()
{
    TREELIST *player;
    C_Text *txt;
    VU_ID *tmpID = NULL;
    long Age;
    _TCHAR buffer[60];

    if ( not People)
        return;

    if ( not FalconLocalGame)
        return;

    Age = GetCurrentTime();

    if (People->GetRoot())
    {
        txt = (C_Text*)People->GetRoot()->Item_;

        if (txt)
        {
            tmpID = (VU_ID*)txt->GetUserPtr(_UI95_VU_ID_SLOT_);

            if (tmpID)
            {
                if (FalconLocalGame->Id() not_eq *tmpID)
                    MakeLocalGameTree(FalconLocalGame);
            }
            else if ((VuGameEntity*)FalconLocalGame not_eq vuPlayerPoolGroup)
                MakeLocalGameTree(FalconLocalGame);
        }
    }
    else
        MakeLocalGameTree(FalconLocalGame);

    if (People->GetRoot())
        People->GetRoot()->Item_->SetUserNumber(100, Age);

    VuSessionsIterator sessionWalker(FalconLocalGame);
    FalconSessionEntity *session;

    session = (FalconSessionEntity*)sessionWalker.GetFirst();

    while (session)
    {
        player = StartTreeSearch(session->Id(), People->GetRoot(), People);

        if (player == NULL)
        {
            player = CreatePlayerButton(People, session);

            if (player)
            {
                People->AddItem(People->GetRoot(), player);

                if (PeopleChatType == 1)
                    player->Item_->SetState(1);
                else
                    CheckPlayerGroup(session, (C_Player*)player->Item_);

                player->Item_->SetUserNumber(100, Age);
                player->Item_->SetMenu(CHAT_POP);
                _stprintf(buffer, "%s %s", ((C_Player*)player->Item_)->GetName(), gStringMgr->GetString(TXT_JOINED_GAME));
                ReceiveChatString(FalconNullId, buffer);

                // Begin Uplink stuff
                if (m_pUplink not_eq NULL and FalconLocalGame and FalconLocalGame->IsLocal())
                {
                    try
                    {
                        m_pUplink->AddPlayer(((C_Player*)player->Item_)->GetName());
                    }

                    catch (_com_error e)
                    {
                        MonoPrint("StartCampaignGame: Error 0x%X occured during UpLink startup", e.Error());
                    }
                }

                // End Uplink stuff
            }
        }
        else
        {
            player->Item_->SetUserNumber(100, Age);
        }

        session = (FalconSessionEntity*)sessionWalker.GetNext();
    }

    People->SetDelCallback(RemoveOldPeopleTreeCB);
    People->RemoveOld(100, Age);
    People->SetDelCallback(NULL);
    People->RecalcSize();

    if (People->Parent_)
        People->Parent_->RefreshClient(People->GetClient());
}

void RebuildGameTree()
{
    _TCHAR *gamename;
    FalconGameType gametype;

    if (People)
        People->Refresh();

    if (DogfightGames)
        DogfightGames->Refresh();

    if (TacticalGames)
        TacticalGames->Refresh();

    if (CampaignGames)
        CampaignGames->Refresh();

    if ((gCommsMgr) and (gCommsMgr->Online()))
    {
        UpdateLocalGameTree(); // Updates Game in Chat window (Current area we are in)

        if (DogfightGames)
            TrimPlayerTree(DogfightGames, DogfightGames->GetRoot());

        if (TacticalGames)
            TrimPlayerTree(TacticalGames, TacticalGames->GetRoot());

        if (CampaignGames)
            TrimPlayerTree(CampaignGames, CampaignGames->GetRoot());

        VuDatabaseIterator dbiter;
        VuTypeFilter filter(static_cast<unsigned short>(F4GameType + VU_LAST_ENTITY_TYPE));
        FalconGameEntity *game;

        game = (FalconGameEntity*)dbiter.GetFirst(&filter);

        while (game)
        {
            gametype = game->GetGameType();
            gamename = game->GameName();

            switch (gametype)
            {
                case game_Dogfight:
                    if (DogfightGames)
                        UpdateGameTreeBranch(gametype, game, DogfightGames, DogfightGames->GetRoot(), FALSE);

                    break;

                case game_TacticalEngagement:
                    if (TacticalGames)
                        UpdateGameTreeBranch(gametype, game, TacticalGames, TacticalGames->GetRoot(), FALSE);

                    break;

                case game_Campaign:
                    if (CampaignGames)
                        UpdateGameTreeBranch(gametype, game, CampaignGames, CampaignGames->GetRoot(), FALSE);

                    break;
            }

            game = (FalconGameEntity*)dbiter.GetNext(&filter);
        }
    }
    else
    {
        if (People)
            People->DeleteBranch(People->GetRoot());

        if (DogfightGames)
            DogfightGames->DeleteBranch(DogfightGames->GetRoot());

        if (TacticalGames)
            TacticalGames->DeleteBranch(TacticalGames->GetRoot());

        if (CampaignGames)
            CampaignGames->DeleteBranch(CampaignGames->GetRoot());
    }

    if (People)
    {
        People->RecalcSize();

        if (People->Parent_)
            People->Parent_->RefreshClient(People->GetClient());
    }

    if (DogfightGames)
    {
        DogfightGames->RecalcSize();

        if (DogfightGames->Parent_)
            DogfightGames->Parent_->RefreshClient(DogfightGames->GetClient());
    }

    if (TacticalGames)
    {
        TacticalGames->RecalcSize();

        if (TacticalGames->Parent_)
            TacticalGames->Parent_->RefreshClient(TacticalGames->GetClient());
    }

    if (CampaignGames)
    {
        CampaignGames->RecalcSize();

        if (CampaignGames->Parent_)
            CampaignGames->Parent_->RefreshClient(CampaignGames->GetClient());
    }
}

void RemoveFromTree(C_TreeList *list, VU_ID ID)
{
    TREELIST *item;

    if ( not list or ID == FalconNullId)
        return;

    item = StartTreeSearch(ID, list->GetRoot(), list);

    if (item)
    {
        list->Refresh();

        if (item->Type_ == C_TYPE_MENU)
        {
            if (item->Child)
                list->DeleteBranch(item->Child);
        }

        list->DeleteItem(item);
        list->RecalcSize();

        if (list->Parent_)
            list->Parent_->RefreshClient(list->GetClient());
    }
}

// This function gets called when VU adds messages to my Queue
void UI_UpdateGameList()
{
    QUEUEITEM *q;
    FalconGameEntity *game;
    FalconSessionEntity *session;
    TREELIST *gamelist, *player;
    C_TreeList *theTree;
    FalconGameType gametype;

    if ( not gUICommsQ)
        return;

    q = gUICommsQ->Root_;

    while (q)
    {
        switch (q->Type)
        {
            case _Q_GAME_ADD_:
                game = (FalconGameEntity*)vuDatabase->Find(q->GameID);

                if (game)
                {
                    gametype = game->GetGameType();

                    switch (gametype)
                    {
                        case game_Dogfight:
                            theTree = DogfightGames;
                            break;

                        case game_TacticalEngagement:
                            theTree = TacticalGames;
                            break;

                        case game_Campaign:
                            theTree = CampaignGames;
                            break;

                        default:
                            theTree = NULL;
                            break;
                    }

                    if (theTree)
                    {
                        // Add to Game's TreeList
                        UpdateGameTreeBranch(gametype, game, theTree, theTree->GetRoot(), FALSE);
                        theTree->RecalcSize();

                        if (theTree->GetParent())
                            theTree->GetParent()->RefreshClient(theTree->GetClient());
                    }
                }

                break;

            case _Q_GAME_REMOVE_:
                RemoveFromTree(DogfightGames, q->GameID);
                RemoveFromTree(TacticalGames, q->GameID);
                RemoveFromTree(CampaignGames, q->GameID);

                // Remove game from gCommsMgr... if current
                if (q->GameID == gCommsMgr->GetTargetGameID())
                {
                    gCommsMgr->LookAtGame(NULL);
                    ClearDFTeamLists();
                }

                break;

            case _Q_GAME_UPDATE_:
                game = (FalconGameEntity*)vuDatabase->Find(q->GameID);

                if (game)
                {
                    if (game == FalconLocalGame)
                        UpdateLocalGameTree();

                    gametype = game->GetGameType();

                    switch (gametype)
                    {
                        case game_Dogfight:
                            theTree = DogfightGames;
                            break;

                        case game_TacticalEngagement:
                            theTree = TacticalGames;
                            break;

                        case game_Campaign:
                            theTree = CampaignGames;
                            break;

                        default:
                            theTree = NULL;
                            break;
                    }

                    if (theTree)
                    {
                        // Add to Game's TreeList
                        UpdateGameTreeBranch(gametype, game, theTree, theTree->GetRoot(), FALSE);
                        theTree->RecalcSize();

                        if (theTree->GetParent())
                            theTree->GetParent()->RefreshClient(theTree->GetClient());
                    }
                }

                break;

            case _Q_SESSION_ADD_:
                game = (FalconGameEntity*)vuDatabase->Find(q->GameID);
                session = (FalconSessionEntity*)vuDatabase->Find(q->SessionID);

                if (game and session)
                {
                    if (game == FalconLocalGame)
                        UpdateLocalGameTree();

                    gametype = game->GetGameType();

                    switch (gametype)
                    {
                        case game_Dogfight:
                            CheckFlyButton();
                            theTree = DogfightGames;
                            break;

                        case game_TacticalEngagement:
                            GetPlayerInfo(q->SessionID);
                            TallyPlayerSquadrons();
                            theTree = TacticalGames;
                            break;

                        case game_Campaign:
                            GetPlayerInfo(q->SessionID);
                            TallyPlayerSquadrons();
                            theTree = CampaignGames;
                            break;

                        default:
                            theTree = NULL;
                    }

                    if (theTree)
                    {
                        // Add to Game's Window List
                        gamelist = StartTreeSearch(game->Id(), theTree->GetRoot(), theTree);

                        if (gamelist)
                        {
                            player = StartTreeSearch(session->Id(), gamelist, theTree);

                            if (player == NULL)
                            {
                                player = CreatePlayerButton(theTree, session);
                                theTree->AddChildItem(gamelist, player);
                                theTree->RecalcSize();

                                if (theTree->GetParent())
                                    theTree->GetParent()->RefreshClient(theTree->GetClient());
                            }
                        }

                        /*
                         if(game == gCommsMgr->GetGame())
                         {
                         // Add to gCommsMgr game lists (if selected)
                         AddPlayerToGame(session);
                         }
                        */
                    }
                }

                break;

            case _Q_SESSION_REMOVE_:
                UpdateLocalGameTree();
                RemoveFromTree(DogfightGames, q->SessionID);
                RemoveFromTree(TacticalGames, q->SessionID);
                RemoveFromTree(CampaignGames, q->SessionID);
                // Remove player from gCommsMgr game lists (if selected)
                game = (FalconGameEntity*)gCommsMgr->GetGame();

                if (game)
                {
                    // if(game->Id() == q->GameID)
                    // RemovePlayerFromGame(q->SessionID);
                    if (game->GetGameType() == game_Campaign or game->GetGameType() == game_TacticalEngagement)
                        TallyPlayerSquadrons();

                    // else if(game->GetGameType() == game_Dogfight)
                    // {
                    // CheckFlyButton();
                    // }
                }

                break;

            case _Q_SESSION_UPDATE_:
                session = (FalconSessionEntity*)vuDatabase->Find(q->SessionID);

                if (session)
                {
                    CheckChatFilters(session);

                    if (gCommsMgr->GetGame() not_eq vuPlayerPoolGroup)
                    {
                        if (gCommsMgr->GetGame())
                        {
                            if (gCommsMgr->GetGame()->Id() == q->GameID)
                            {
                                CheckForNewPlayer(session);
                                CheckPlayersFlight(session);
                            }
                        }
                    }
                }

                break;
        }

        q = gUICommsQ->Remove();
    }
}

static void PeopleSelectCB(long, short hittype, C_Base *)
{
    TREELIST *item;
    C_Player *player;

    if (hittype not_eq C_TYPE_LMOUSEUP or not People)
        return;

    item = People->GetLastItem();

    if (item->Type_ not_eq C_TYPE_ITEM)
        return;

    player = (C_Player*)item->Item_;

    if ( not player)
        return;

    player->SetState(static_cast<short>(player->GetState() xor 1));
    player->Refresh();
}

static void SelectChatFilterCB(long, short hittype, C_Base *control)
{
    VuSessionsIterator sessionWalker(FalconLocalGame);
    FalconSessionEntity *session;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    PeopleChatType = static_cast<short>(control->GetCluster()); // 1=All,2=team,3=package,4=flight

    session = (FalconSessionEntity*)sessionWalker.GetFirst();

    while (session)
    {
        CheckChatFilters(session);
        session = (FalconSessionEntity*)sessionWalker.GetNext();
    }

}

void UI_Refresh(void)
{
    if ( not FalconLocalGame or not gCommsMgr or not gMainHandler)
    {
        return;
    }

    // Do UI Notification stuff
    switch (FalconLocalGame->GetGameType())
    {
        case game_Dogfight:
            PostMessage(FalconDisplay.appWin, FM_REFRESH_DOGFIGHT, 0, 0);
            break;

        case game_TacticalEngagement:
            PostMessage(FalconDisplay.appWin, FM_REFRESH_TACTICAL, 0, 0);
            break;

        case game_Campaign:
            PostMessage(FalconDisplay.appWin, FM_REFRESH_CAMPAIGN, 0, 0);
            break;

        default:
            break;
    }
}


void ViewRemoteLogbook(long playerID)
{
    F4CSECTIONHANDLE *Leave;
    C_Window *win;
    RemoteLB *remlb;
    IMAGE_RSC *pic, *pat;

    win = gMainHandler->FindWindow(LOG_WIN);

    if (win)
    {
        Leave = UI_Enter(win);

        remlb = (RemoteLB*)gCommsMgr->GetRemoteLB(playerID);

        if (remlb)
        {
            if (remlb->Pilot_.PictureResource)
            {
                pic = gImageMgr->GetImage(remlb->Pilot_.PictureResource);
            }
            else
            {
                pic = NULL;
            }

            if (remlb->Pilot_.PatchResource)
            {
                pat = gImageMgr->GetImage(remlb->Pilot_.PatchResource);
            }
            else
            {
                pat = NULL;
            }

            DisplayLogbook(&remlb->Pilot_, pic, pat, FALSE);
            gMainHandler->ShowWindow(win);
            gMainHandler->WindowToFront(win);
        }

        UI_Leave(Leave);
    }
}

void SendTextToFlight()
{
    VuSessionsIterator sessionWalker(FalconLocalGame);
    FalconSessionEntity *session;
    UI_SendChatMessage *chat;

    session = (FalconSessionEntity*)sessionWalker.GetFirst();

    while (session)
    {
        if (session->GetPlayerFlightID() == FalconLocalSession->GetPlayerFlightID())
        {
            chat = new UI_SendChatMessage(FalconNullId, session);

            chat->dataBlock.from = FalconLocalSessionId;
            chat->dataBlock.size = static_cast<short>((strlen(chatterStr) + 1) * sizeof(char));
            chat->dataBlock.message = new char[chat->dataBlock.size];
            memcpy(chat->dataBlock.message, chatterStr, chat->dataBlock.size);
            FalconSendMessage(chat, TRUE);
        }

        session = (FalconSessionEntity*)sessionWalker.GetNext();
    }
}

void SendTextToPackage()
{
    Flight flt;
    Package pkg;
    VU_ID MyPackageID;
    VuSessionsIterator sessionWalker(FalconLocalGame);
    FalconSessionEntity *session;
    UI_SendChatMessage *chat;

    flt = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());

    if ( not flt)
        return;

    pkg = (Package)flt->GetUnitParent();

    if ( not pkg)
        return;

    MyPackageID = pkg->Id();

    session = (FalconSessionEntity*)sessionWalker.GetFirst();

    while (session)
    {
        flt = (Flight)vuDatabase->Find(session->GetPlayerFlightID());

        if (flt)
        {
            pkg = (Package)flt->GetUnitParent();

            if (pkg and pkg->Id() == MyPackageID)
            {
                chat = new UI_SendChatMessage(FalconNullId, session);

                chat->dataBlock.from = FalconLocalSessionId;
                chat->dataBlock.size = static_cast<short>((strlen(chatterStr) + 1) * sizeof(char));
                chat->dataBlock.message = new char[chat->dataBlock.size];
                memcpy(chat->dataBlock.message, chatterStr, chat->dataBlock.size);
                FalconSendMessage(chat, TRUE);
            }
        }

        session = (FalconSessionEntity*)sessionWalker.GetNext();
    }

}

void SendTextToRange()
{
    VuSessionsIterator sessionWalker(FalconLocalGame);
    FalconSessionEntity *session;
    UI_SendChatMessage *chat;
    float myx, sx, myy, sy, dx, dy, dist;
    VuEntity *ent;

    ent = FalconLocalSession->GetPlayerEntity();

    if ( not ent)
        return;

    myx = ent->XPos();
    myy = ent->YPos();

    session = (FalconSessionEntity*)sessionWalker.GetFirst();

    while (session)
    {
        ent = session->GetPlayerEntity();

        if (ent)
        {
            sx = ent->XPos();
            sy = ent->YPos();

            dx = myx - sx;
            dy = myy - sy;

            dist = static_cast<float>(sqrt(dx * dx + dy * dy) * FT_TO_NM);

            if (dist <= 40.0f)
            {
                chat = new UI_SendChatMessage(FalconNullId, session);

                chat->dataBlock.from = FalconLocalSessionId;
                chat->dataBlock.size = static_cast<short>((strlen(chatterStr) + 1) * sizeof(char));
                chat->dataBlock.message = new char[chat->dataBlock.size];
                memcpy(chat->dataBlock.message, chatterStr, chat->dataBlock.size);
                FalconSendMessage(chat, TRUE);
            }
        }

        session = (FalconSessionEntity*)sessionWalker.GetNext();
    }
}

void SendTextToTeam()
{
    VuSessionsIterator sessionWalker(FalconLocalGame);
    FalconSessionEntity *session;
    UI_SendChatMessage *chat;

    session = (FalconSessionEntity*)sessionWalker.GetFirst();

    while (session)
    {
        if (session->GetTeam() == FalconLocalSession->GetTeam())
        {
            chat = new UI_SendChatMessage(FalconNullId, session);

            chat->dataBlock.from = FalconLocalSessionId;
            chat->dataBlock.size = static_cast<short>((strlen(chatterStr) + 1) * sizeof(char));
            chat->dataBlock.message = new char[chat->dataBlock.size];
            memcpy(chat->dataBlock.message, chatterStr, chat->dataBlock.size);
            FalconSendMessage(chat, TRUE);
        }

        session = (FalconSessionEntity*)sessionWalker.GetNext();
    }

}

void SendTextToEveryOne()
{
    VuSessionsIterator sessionWalker(FalconLocalGame);
    FalconSessionEntity *session;
    UI_SendChatMessage *chat;

    session = (FalconSessionEntity*)sessionWalker.GetFirst();

    while (session)
    {
        chat = new UI_SendChatMessage(FalconNullId, session);

        chat->dataBlock.from = FalconLocalSessionId;
        chat->dataBlock.size = static_cast<short>((strlen(chatterStr) + 1) * sizeof(char));
        chat->dataBlock.message = new char[chat->dataBlock.size];
        memcpy(chat->dataBlock.message, chatterStr, chat->dataBlock.size);
        FalconSendMessage(chat, TRUE);

        session = (FalconSessionEntity*)sessionWalker.GetNext();
    }
}

// sfr: send messages here
void SendTextMessageToChannel()
{
    int curChannel, curRadio;

    if (VM)
    {
        curRadio = VM->Radio();
        curChannel = VM->GetRadioFreq(curRadio);

        switch (curChannel)
        {
            case rcfOff:
                break;

            case rcfFlight1:
            case rcfFlight2:
            case rcfFlight3:
            case rcfFlight4:
            case rcfFlight5:
                SendTextToFlight();
                break;

            case rcfPackage1:
            case rcfPackage2:
            case rcfPackage3:
            case rcfPackage4:
            case rcfPackage5:

                SendTextToPackage();
                break;

            case rcfFromPackage:
                SendTextToPackage();
                break;

            case rcfProx: // 40nm range
                SendTextToRange();
                break;

            case rcfTeam:
                SendTextToTeam();
                break;

            case rcfAll:
                SendTextToEveryOne();
                break;
        }
    }
}

void SimOpenChatBox(unsigned long, int state, void *)
{
    if ((state bitand KEY_DOWN))
    {
        CommandsKeyCombo = -2;
        CommandsKeyComboMod = -2;

        memset(chatterStr, 0, sizeof(chatterStr));
        chatterCount = 0;

        UseInputFn = SendTextMessageToChannel;
        DiscardInputFn = NULL;
        MaxInputLength = 60;
        AsciiAllowed = 0; // All

        OTWDriver.SetFrontTextFlags(OTWDriver.GetFrontTextFlags() bitor SHOW_CHATBOX);
    }
}


static void HookupCommsControls(long ID)
{
    C_Window *winme;
    C_Button *ctrl;
    C_EditBox *ebox;
    C_TreeList *tree;

    winme = gMainHandler->FindWindow(ID);

    if (winme == NULL)
        return;

    // Hook up IDs here
    // Hook up Close Button
    ctrl = (C_Button *)winme->FindControl(CHAT_ALL);

    if (ctrl)
        ctrl->SetCallback(SelectChatFilterCB);

    ctrl = (C_Button *)winme->FindControl(CHAT_TEAM);

    if (ctrl)
        ctrl->SetCallback(SelectChatFilterCB);

    ctrl = (C_Button *)winme->FindControl(CHAT_PACKAGE);

    if (ctrl)
        ctrl->SetCallback(SelectChatFilterCB);

    ctrl = (C_Button *)winme->FindControl(CHAT_FLIGHT);

    if (ctrl)
        ctrl->SetCallback(SelectChatFilterCB);

    ctrl = (C_Button *)winme->FindControl(CHAT_DISCONNECT);

    if (ctrl)
        ctrl->SetCallback(DisconnectCommsCB);

    ctrl = (C_Button *)winme->FindControl(CHAT_PBOOK);

    if (ctrl)
        ctrl->SetCallback(OpenPhoneBookCB);

    ctrl = (C_Button *)winme->FindControl(CLOSE_WINDOW);

    if (ctrl)
        ctrl->SetCallback(CloseWindowCB);

    ebox = (C_EditBox *)winme->FindControl(TEXT_OUT);

    if (ebox)
        ebox->SetCallback(SendChatStringCB);

    ctrl = (C_Button *)winme->FindControl(PB_NEW);

    if (ctrl)
        ctrl->SetCallback(Phone_New_CB);

    ctrl = (C_Button *)winme->FindControl(PB_APPLY);

    if (ctrl)
        ctrl->SetCallback(Phone_Apply_CB);

    ctrl = (C_Button *)winme->FindControl(PB_REMOVE);

    if (ctrl)
        ctrl->SetCallback(Phone_Remove_CB);

    ctrl = (C_Button *)winme->FindControl(PB_CONNECT);

    if (ctrl)
        ctrl->SetCallback(Phone_Connect_CB);

    ctrl = (C_Button *)winme->FindControl(PB_CANCEL);

    if (ctrl)
        ctrl->SetCallback(CloseWindowCB);

    C_Base *b = (C_EditBox *)winme->FindControl(IP_ADDRESS_1);

    if (b)
        b->SetCallback(AddressInputCB);


    // ctrl=(C_Button *)winme->FindControl(CON_TYPE_MODEM);
    // if(ctrl)
    // ctrl->SetCallback(Phone_ConnectType_CB);

    // ctrl=(C_Button *)winme->FindControl(CON_TYPE_INTERNET);
    // if(ctrl)
    // ctrl->SetCallback(Phone_ConnectType_CB);

    // ctrl=(C_Button *)winme->FindControl(CON_TYPE_LAN);
    // if(ctrl)
    // ctrl->SetCallback(Phone_ConnectType_CB);

    // ctrl=(C_Button *)winme->FindControl(CON_TYPE_SERIAL);
    // if(ctrl)
    // ctrl->SetCallback(Phone_ConnectType_CB);

    // ctrl=(C_Button *)winme->FindControl(CON_TYPE_JETNET);
    // if(ctrl)
    // ctrl->SetCallback(Phone_ConnectType_CB);

    tree = (C_TreeList*)winme->FindControl(PEOPLE_TREE);

    if (tree)
    {
        People = tree;
        tree->SetCallback(PeopleSelectCB);
    }

    // Help GUIDE thing
    ctrl = (C_Button*)winme->FindControl(UI_HELP_GUIDE);

    if (ctrl)
        ctrl->SetCallback(UI_Help_Guide_CB);

    HookupServerBrowserControls(JETNET_WIN);
}
