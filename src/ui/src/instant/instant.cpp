/***************************************************************************\
 instant.cpp
 Peter Ward
 December 3, 1996

 Main UI screen stuff for FreeFalcon
\***************************************************************************/
#include <windows.h>
#include "falclib.h"
#include "targa.h"
#include "dxutil/ddutil.h"
#include "Graphics/Include/imagebuf.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "entity.h"
#include "feature.h"
#include "vehicle.h"
#include "evtparse.h"
#include "Mesg.h"
#include "MsgInc/DamageMsg.h"
#include "MsgInc/WeaponFireMsg.h"
#include "MsgInc/DeathMessage.h"
#include "MsgInc/MissileEndMsg.h"
#include "MsgInc/LandingMessage.h"
#include "MsgInc/EjectMsg.h"
#include "falcuser.h"
#include "falclib/include/f4find.h"
#include "f4error.h"
#include "uicomms.h"
#include "ui_ia.h"
#include "userids.h"
#include "textids.h"
#include "CmpClass.h"
#include "ThreadMgr.h"
#include "PlayerOp.h"
#include "classtbl.h"
#include "iaction.h"
#include "Graphics/Include/TMap.h" // JPO for map sizes
#include "fakerand.h" //THW for random startup position

//JAM 21Nov03
#include "weather.h"
#include "timerthread.h"

#define _USE_REGISTRY_ 1 // 0=No,1=Yes

enum // Instant action scoring stuff
{
    _A_LOW_SCORE_ = 250000,
    _A_MEDIUM_SCORE_ = 500000,
};

static IDirectDraw *DDraw;
IDirectDrawSurface *UI95_CreateDDSurface(IDirectDraw *DD, DWORD width, DWORD height);
void ProcessEventList(C_Window *win, long client);
void SetSingle_Comms_Ctrls();
void RemoveWeaponUsageList();
extern C_Handler *gMainHandler;
extern char gUI_CampaignFile[];
void UI_Help_Guide_CB(long ID, short hittype, C_Base *ctrl);
void EncryptBuffer(uchar startkey, uchar *buffer, long length);
void DecryptBuffer(uchar startkey, uchar *buffer, long length);

char *gUBuffer;

C_SoundBite *gInstantBites = NULL;


// Parameters for Instant Action
UI_IA InstantActionSettings =
{
    _MISSION_AIR_TO_AIR_,
    _PILOT_LEVEL_NOVICE_,
    _NO_SAM_SITES_,
    _NO_AAA_SITES_,
};

long TotalScore = 0;
long Bonus = 0;
long Penalty = 0;
long LandingBonus = 0;
long LivingBonus = 0;

kill_list *WeaponUsage = NULL;
kill_list *AircraftKills = NULL;
kill_list *ObjectKills = NULL;
kill_list *BonusList = NULL;

VU_ID IAPlayerID = FalconNullId;

HighScoreList Scores;

extern C_Parser *gMainParser;
extern int MainLastGroup;
extern int IALoaded;

void CloseWindowCB(long ID, short hittype, C_Base *control);
static void HookupIAControls(long ID);
static void SetupInstantAction();
static void InsertScoreCB(long ID, short hittype, C_Base *control);
static void OpenIAMunitionsCB(long ID, short hittype, C_Base *control);
void OpenMunitionsWindowCB(long ID, short hittype, C_Base *control);

_TCHAR *AddCommas(_TCHAR *buf)
{
    int i, j, k, len;
    _TCHAR newbuf[25];
    _TCHAR *comma;

    comma = gStringMgr->GetString(TXT_COMMA_PLACE);

    if ( not comma)
        return(buf);

    j = 0;
    k = 0;
    len = _tcsclen(buf);
    i = len;

    if (buf[0] == '-')
    {
        i--;
        len--;
        newbuf[j++] = buf[k++];
    }

    while (i)
    {
        if ((i % 3) == 0 and i > 1 and i < len)
            newbuf[j++] = comma[0];

        newbuf[j++] = buf[k++];
        i--;
    }

    newbuf[j++] = 0;
    _tcscpy(buf, newbuf);
    return(buf);
}

void GetHighScores()
{
#if _USE_REGISTRY_
    DWORD type, size;
    HKEY theKey;
    long retval;
#else
    HANDLE fp;
    DWORD br;
#endif

#if _USE_REGISTRY_
    size = sizeof(HighScoreList);
    retval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, FALCON_REGISTRY_KEY, 0, KEY_ALL_ACCESS, &theKey);
    retval = RegQueryValueEx(theKey, "initData", 0, &type, (LPBYTE)&Scores, &size);
    RegCloseKey(theKey);

    if (retval not_eq ERROR_SUCCESS)
    {
        // ShiAssert(strcmp("Failed Reg Load,I would Clear Here","But Not this time") == 0);
        // memset(&Scores[0],0,size);
        return;
    }

#else

    fp = CreateFile("highscore.bin", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (fp == INVALID_HANDLE_VALUE)
        return;

    ReadFile(fp, &Scores, sizeof(HighScoreList), &br, NULL);
    CloseHandle(fp);

#endif
    DecryptBuffer(0x38, (uchar*)&Scores, sizeof(HighScoreList));

    if (Scores.CheckSum) // Someone tampered with data... reset it
        memset(&Scores, 0, sizeof(HighScoreList));
}

void SaveHighScores()
{
#if _USE_REGISTRY_
    DWORD size;
    HKEY theKey;
    long retval;
#else
    HANDLE fp;
    DWORD br;
#endif

    EncryptBuffer(0x38, (uchar*)&Scores, sizeof(HighScoreList));

#if _USE_REGISTRY_
    size = sizeof(HighScoreList);
    retval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, FALCON_REGISTRY_KEY, 0, KEY_ALL_ACCESS, &theKey);

    if (retval == ERROR_SUCCESS)
        retval = RegSetValueEx(theKey, "initData", 0, REG_BINARY, (LPBYTE)&Scores, size);

    RegCloseKey(theKey);
#else

    fp = CreateFile("highscore.bin", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (fp == INVALID_HANDLE_VALUE)
        return;

    WriteFile(fp, &Scores, sizeof(HighScoreList), &br, NULL);
    CloseHandle(fp);
#endif
    DecryptBuffer(0x38, (uchar*)&Scores, sizeof(HighScoreList));
}

int VisualIDCost[][2] =
{
    { VIS_AIM120, 1000 },
    { VIS_AIM9M,   100 },
    { VIS_AIM7,    200 },

    { VIS_AGM65B,   50 },
    { VIS_AGM65D,   50 },
    { VIS_AGM65G,   50 },
    { VIS_AGM88,    50 },

    { VIS_IL28,   1000 },
    { VIS_TU16N,  2000 },
    { VIS_SU25,   2500 },
    { VIS_MIG19,  5000 },
    { VIS_MIG21, 10000 },
    { VIS_MIG23, 10000 },
    { VIS_MIG25, 15000 },
    { VIS_MIG29, 15000 },
    { VIS_SU27,  20000 },

    { VIS_F14,  -50000 },
    { VIS_F15C,  -50000 },
    { VIS_F16C,  -50000 },
    { VIS_F18A,  -50000 },
    { VIS_F18D,  -50000 },
    { VIS_B52G,  -50000 },
    { VIS_KC10, -50000 },
    { VIS_A10,  -50000 },
    { 0, 0 },
};

int FindCost(int ID)
{
    int i;

    i = 0;

    while (VisualIDCost[i][0] not_eq 0)
    {
        if (MapVisId(VisualIDCost[i][0]) == ID)
            return(VisualIDCost[i][1]);

        i++;
    }

    return(500);
}

int AddWeaponToUsageList(int ID)
{
    kill_list *cur, *newwp;
    int COST;
    short visID;

    visID = Falcon4ClassTable[ID].visType[0];
    COST = FindCost(visID);
    COST /= 4; // ECTS HACK

    cur = WeaponUsage;

    while (cur)
    {
        if (cur->id == ID)
        {
            cur->num ++;
            cur->points -= COST;
            return -COST;
        }

        cur = cur->next;
    }

    newwp = new kill_list;
    newwp->id = ID;
    newwp->num = 1;
    newwp->points = -COST;
    newwp->next = NULL;

    if (WeaponUsage == NULL)
    {
        WeaponUsage = newwp;
    }
    else
    {
        cur = WeaponUsage;

        while (cur->next)
            cur = cur->next;

        cur->next = newwp;
    }

    return -COST;
}

int AddAircraftToKillsList(int ID)
{
    kill_list *cur, *newwp;
    int COST;
    short visID;

    visID = Falcon4ClassTable[ID].visType[0];
    COST = FindCost(visID);

    cur = AircraftKills;

    while (cur)
    {
        if (cur->id == ID)
        {
            cur->num ++;
            cur->points += COST;
            return COST;
        }

        cur = cur->next;
    }

    newwp = new kill_list;
    newwp->id = ID;
    newwp->num = 1;
    newwp->points = COST;
    newwp->next = NULL;

    if (AircraftKills == NULL)
    {
        AircraftKills = newwp;
    }
    else
    {
        cur = AircraftKills;

        while (cur->next)
            cur = cur->next;

        cur->next = newwp;
    }

    return COST;
}

int AddObjectToKillsList(int ID)
{
    kill_list *cur, *newwp;
    int COST;
    short visID;

    visID = Falcon4ClassTable[ID].visType[0];
    COST = FindCost(visID);

    cur = ObjectKills;

    while (cur)
    {
        if (cur->id == ID)
        {
            cur->num ++;
            cur->points += COST;
            return COST;
        }

        cur = cur->next;
    }

    newwp = new kill_list;
    newwp->id = ID;
    newwp->num = 1;
    newwp->points = COST;
    newwp->next = NULL;

    if (ObjectKills == NULL)
    {
        ObjectKills = newwp;
    }
    else
    {
        cur = ObjectKills;

        while (cur->next)
            cur = cur->next;

        cur->next = newwp;
    }

    return COST;
}

int score_player_ejected(void)
{
    kill_list *newwp;

    newwp = new kill_list;
    newwp->id = 0;
    newwp->num = 0;
    newwp->points = 0;
    newwp->next = NULL;

    if (BonusList)
    {
        BonusList->next = newwp;
    }
    else
    {
        BonusList = newwp;
    }

    return 0;
}

void RemoveBonusList()
{
    kill_list *cur, *prev;

    cur = BonusList;

    while (cur)
    {
        prev = cur;
        cur = cur->next;
        delete prev;
    }

    BonusList = NULL;
}

void RemoveWeaponUsageList()
{
    kill_list *cur, *prev;

    cur = WeaponUsage;

    while (cur)
    {
        prev = cur;
        cur = cur->next;
        delete prev;
    }

    WeaponUsage = NULL;
}

void RemoveAircraftKillsList()
{
    kill_list *cur, *prev;

    cur = AircraftKills;

    while (cur)
    {
        prev = cur;
        cur = cur->next;
        delete prev;
    }

    AircraftKills = NULL;
}

void RemoveObjectKillsList()
{
    kill_list *cur, *prev;

    cur = ObjectKills;

    while (cur)
    {
        prev = cur;
        cur = cur->next;
        delete prev;
    }

    ObjectKills = NULL;
}

void AddIAVehicleKill(VU_ID, VU_ID Victim, VehicleClassDataType *)
{
    // Don't care... Leon does this
    if (Victim == IAPlayerID)
        Penalty += 2000;
}

void CheckEject(VU_ID Pilot, float damage, float fuel)
{
    if (Pilot == IAPlayerID)
    {
        if (damage > 80.0f and fuel > 100.0f)
            LivingBonus = -50000;
        else
            LivingBonus = 50000;
    }
}

void CheckLanding(VU_ID Pilot, float damage, float fuel)
{
    if (Pilot == IAPlayerID)
    {
        LandingBonus = 50000;

        if (damage < 50.0f or fuel == 0.0f)
            LandingBonus = 150000;
    }
}

void LoadInstantActionWindows()
{
    long ID;
    C_Window *win;
    C_Button *ctrl;

    if (IALoaded) return;

    if (_LOAD_ART_RESOURCES_)
        gMainParser->LoadImageList("ia_res.lst");
    else
        gMainParser->LoadImageList("ia_art.lst");

    gMainParser->LoadSoundList("ia_snd.lst");

    if ( not gInstantBites)
        gInstantBites = gMainParser->ParseSoundBite("art\\instant\\uidia.scf");

    gMainParser->LoadWindowList("ia_scf.lst"); // Modified by M.N. - add art/art1024 by LoadWindowList

    ID = gMainParser->GetFirstWindowLoaded();

    while (ID)
    {
        HookupIAControls(ID);
        ID = gMainParser->GetNextWindowLoaded();
    }

    SetSingle_Comms_Ctrls();
    win = gMainHandler->FindWindow(IA_HS_WIN);

    if (win)
    {
        ctrl = (C_Button *)win->FindControl(CLOSE_WINDOW);

        if (ctrl)
            ctrl->SetCallback(InsertScoreCB);
    }

    SetupInstantAction();
    IALoaded++;
}

static void InstantActionFlyCB(long, short hittype, C_Base *)
{
    C_Window *win;
    C_Button *btn;
    C_Cursor *crsr;
    C_Clock *clk;
    float XPos, YPos;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    win = gMainHandler->FindWindow(IA_SETTINGS_WIN);

    if (win)
    {
        InstantActionSettings.MissionType = _MISSION_AIR_TO_AIR_;
        instant_action::set_start_mode('f');

        btn = (C_Button *)win->FindControl(IA_MIS_MUD_CTRL);

        if (btn)
        {
            if (btn->GetState())
            {
                InstantActionSettings.MissionType = _MISSION_AIR_TO_GROUND_;
                instant_action::set_start_mode('m');
            }
        }

        InstantActionSettings.PilotLevel = _PILOT_LEVEL_NOVICE_;;
        btn = (C_Button *)win->FindControl(IA_LVL_CADET_CTRL);

        if (btn)
        {
            if (btn->GetState())
                InstantActionSettings.PilotLevel = _PILOT_LEVEL_CADET_;
        }

        btn = (C_Button *)win->FindControl(IA_LVL_ROOKIE_CTRL);

        if (btn)
        {
            if (btn->GetState())
                InstantActionSettings.PilotLevel = _PILOT_LEVEL_ROOKIE_;
        }

        btn = (C_Button *)win->FindControl(IA_LVL_VETERAN_CTRL);

        if (btn)
        {
            if (btn->GetState())
                InstantActionSettings.PilotLevel = _PILOT_LEVEL_VETERAN_;
        }

        btn = (C_Button *)win->FindControl(IA_LVL_ACE_CTRL);

        if (btn)
        {
            if (btn->GetState())
                InstantActionSettings.PilotLevel = _PILOT_LEVEL_ACE_;
        }

        InstantActionSettings.SamSites = _NO_SAM_SITES_;
        btn = (C_Button *)win->FindControl(IA_AD_SAMS_CTRL);

        if (btn)
        {
            if (btn->GetState())
                InstantActionSettings.SamSites = _SAM_SITES_;
        }

        InstantActionSettings.AAASites = _NO_AAA_SITES_;
        btn = (C_Button *)win->FindControl(IA_AD_AAA_CTRL);

        if (btn)
        {
            if (btn->GetState())
                InstantActionSettings.AAASites = _AAA_SITES_;
        }
    }

    win = gMainHandler->FindWindow(IA_MAP_WIN);

    if (win)
    {
        crsr = (C_Cursor *)win->FindControl(IA_MAP_CURSOR);

        if (crsr)
        {
            // JPO - fix for big theaters.
            // 13119.9 is default ft/block
            XPos = (float)(crsr->GetX() + crsr->GetW() / 2 - crsr->MinX_) /
                   (float)(crsr->MaxX_ - crsr->MinX_);
            XPos *= 4096.0f * TheMap.BlocksWide() * TheMap.FeetPerBlock() / 13119.9F;
            YPos = (float)((crsr->MaxY_ - crsr->MinY_) - (crsr->GetY() + crsr->GetH() / 2 - crsr->MinY_)) /
                   (float)(crsr->MaxY_ - crsr->MinY_);
            YPos *=  4096.0f * TheMap.BlocksHigh() * TheMap.FeetPerBlock() / 13119.9F;

            instant_action::set_start_position(YPos * FEET_PER_KM / 1000.0f, XPos * FEET_PER_KM / 1000.0f);
        }
    }

    win = gMainHandler->FindWindow(IA_SUA);

    if (win)
    {
        clk = (C_Clock*)win->FindControl(TIME_ID);

        if (clk)
            instant_action::set_start_time(clk->GetTime());
        else
            instant_action::set_start_time(static_cast<long>(12.0F * 60.0F * 60.0F));
    }

    ShiAssert( not TheCampaign.IsLoaded());

    // Load a campaign here
    strcpy(gUI_CampaignFile, "Instant");

    ShiAssert(gameCompressionRatio == 0);

    TheCampaign.SetOnlineStatus(0);

    TheCampaign.LoadCampaign(game_InstantAction, gUI_CampaignFile);

    instant_action::set_start_wave(InstantActionSettings.PilotLevel);
    instant_action::create_player_flight();

    PostMessage(gMainHandler->GetAppWnd(), FM_START_INSTANTACTION, 0, 0);
}

static void InsertScoreCB(long, short hittype, C_Base *)
{
    C_Window *win;
    int i, j, idx;
    C_EditBox *ebox;
    C_Text *txt;
    _TCHAR buf[MAX_NAME_LENGTH + 1];

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    i = 0;

    while (i < MAX_SCORES and TotalScore <= Scores.Scores[i].Score and Scores.Scores[i].Name[0] not_eq 0)
        i++;

    idx = MAX_SCORES;

    if (i < MAX_SCORES)
    {
        for (j = MAX_SCORES - 2; j >= i; j--)
        {
            memset(Scores.Scores[j + 1].Name, 0, sizeof(_TCHAR)*MAX_NAME_LENGTH);
            _tcscpy(Scores.Scores[j + 1].Name, Scores.Scores[j].Name);
            Scores.Scores[j + 1].Score = Scores.Scores[j].Score;
        }

        idx = i;
        memset(Scores.Scores[i].Name, 0, sizeof(_TCHAR)*MAX_NAME_LENGTH);
        Scores.Scores[i].Name[0] = ' ';
        Scores.Scores[i].Score = TotalScore;

        ebox = NULL;
        win = gMainHandler->FindWindow(IA_HS_WIN);

        if (win)
        {
            ebox = (C_EditBox *)win->FindControl(IA_HS_NAME_EDIT);
            win->SetControl(0);
            gMainHandler->DisableWindowGroup(win->GetGroup());
        }

        if (ebox)
        {
            _tcsncpy(Scores.Scores[i].Name, ebox->GetText(), MAX_NAME_LENGTH);

            if ( not Scores.Scores[i].Name[0])
            {
                Scores.Scores[i].Name[0] = ' ';
                Scores.Scores[i].Name[1] = 0;
            }
        }
        else
        {
            Scores.Scores[i].Name[0] = ' ';
            Scores.Scores[i].Name[1] = 0;
        }

        win = gMainHandler->FindWindow(IA_SH_WIN);

        if (win)
        {
            for (i = 0; i < MAX_SCORES; i++)
            {
                if (Scores.Scores[i].Name[0] not_eq 0)
                {
                    // Set Name
                    txt = (C_Text *)win->FindControl(TEXT_1 + i * 2);

                    if (txt)
                    {
                        txt->SetText(Scores.Scores[i].Name);

                        if (i == idx)
                            txt->SetFGColor(0x00ff00);
                        else
                            txt->SetFGColor(0xffffff);
                    }

                    // Set Score
                    txt = (C_Text *)win->FindControl(TEXT_1 + i * 2 + 1);

                    if (txt)
                    {
                        _stprintf(buf, "%1ld", Scores.Scores[i].Score);
                        txt->SetText(AddCommas(buf));

                        if (i == idx)
                            txt->SetFGColor(0x00ff00);
                        else
                            txt->SetFGColor(0xffffff);
                    }
                }
            }
        }

        win->RefreshWindow();
        SaveHighScores();
    }

    win = gMainHandler->FindWindow(IA_DBRF_WIN);

    if (win)
        gMainHandler->EnableWindowGroup(win->GetGroup());
}

static void HighScoreKeyboardCB(long ID, short DKKey, C_Base *control)
{
    if (DKKey == DIK_RETURN)
        InsertScoreCB(ID, C_TYPE_LMOUSEUP, control);
}

void ChangeTimeCB(long, short hittype, C_Base *control)
{
    C_Clock *clk;
    short dir = 0;
    long value, flag;

    if (hittype not_eq C_TYPE_LMOUSEUP and hittype not_eq C_TYPE_REPEAT)
        return;

    dir = static_cast<short>(control->GetUserNumber(1));

    clk = (C_Clock*)control->Parent_->FindControl(control->GetUserNumber(0));

    if (clk)
    {
        flag = 0;

        if (clk->GetCurrentCtrl())
        {
            value = clk->GetLast() + dir;

            if (value < clk->GetCurrentCtrl()->GetMinInteger())
            {
                value = clk->GetCurrentCtrl()->GetMaxInteger();
                flag = clk->GetCurrentCtrl()->GetID();
            }
            else if (value > clk->GetCurrentCtrl()->GetMaxInteger())
            {
                value = clk->GetCurrentCtrl()->GetMinInteger();
                flag = clk->GetCurrentCtrl()->GetID();
            }

            clk->SetLast(value);
        }
        else
        {
            value = clk->GetSecond() + dir;

            if (value < clk->GetSecondCtrl()->GetMinInteger())
            {
                value = clk->GetSecondCtrl()->GetMaxInteger();
                flag = clk->GetSecondCtrl()->GetID();
            }
            else if (value > clk->GetSecondCtrl()->GetMaxInteger())
            {
                value = clk->GetSecondCtrl()->GetMinInteger();
                flag = clk->GetSecondCtrl()->GetID();
            }

            clk->SetSecond(value);
        }

        if (flag)
        {
            if (flag == 4)
            {
                flag = 0;
                value = clk->GetMinute() + dir;

                if (value < clk->GetMinuteCtrl()->GetMinInteger())
                {
                    value = clk->GetMinuteCtrl()->GetMaxInteger();
                    flag = clk->GetMinuteCtrl()->GetID();
                }
                else if (value > clk->GetMinuteCtrl()->GetMaxInteger())
                {
                    value = clk->GetMinuteCtrl()->GetMinInteger();
                    flag = clk->GetMinuteCtrl()->GetID();
                }

                clk->SetMinute(value);
            }

            if (flag == 3)
            {
                flag = 0;
                value = clk->GetHour() + dir;

                if (value < clk->GetHourCtrl()->GetMinInteger())
                {
                    value = clk->GetHourCtrl()->GetMaxInteger();
                    flag = clk->GetHourCtrl()->GetID();
                }
                else if (value > clk->GetHourCtrl()->GetMaxInteger())
                {
                    value = clk->GetHourCtrl()->GetMinInteger();
                    flag = clk->GetHourCtrl()->GetID();
                }

                clk->SetHour(value);
            }

            if (flag == 2)
            {
                if (clk->GetDayCtrl())
                {
                    flag = 0;
                    value = clk->GetDay() + dir;

                    if (value < clk->GetDayCtrl()->GetMinInteger() - 1)
                    {
                        value = clk->GetDayCtrl()->GetMaxInteger() - 1;
                        flag = clk->GetDayCtrl()->GetID();
                    }
                    else if (value > clk->GetDayCtrl()->GetMaxInteger() - 1)
                    {
                        value = clk->GetDayCtrl()->GetMinInteger() - 1;
                        flag = clk->GetDayCtrl()->GetID();
                    }

                    clk->SetDay(value);
                }
            }

        }

        clk->Refresh();
    }
}

static void HookupIAControls(long ID)
{
    C_Window *winme;
    C_Button *ctrl;
    C_EditBox *ebox;
    C_Clock *clk;
    long hour; //THW 2004-02-18 Random daytime
    winme = gMainHandler->FindWindow(ID);

    if (winme == NULL)
        return;

    // Hook up IDs here

    // Time/Date CB
    clk = (C_Clock *)winme->FindControl(TIME_ID);

    if (clk)
    {
        //clk->SetHour(12);
        //THW 2004-02-18 Random daytime
        hour = (4 + (rand() % 15));
        clk->SetHour(hour);

        //Sunrise ~4:40h, Sunset ~19:10h
        if (hour == 4)
        {
            clk->SetMinute(40 + (rand() % 19));
        }
        else if (hour == 19)
        {
            clk->SetMinute(0 + (rand() % 10));
        }
        else
        {
            clk->SetMinute(0 + (rand() % 59));
        }

        clk->SetSecond(0);
        clk->Refresh();
    }

    // Hook up Instant Action Buttons
    // Set Fly Button for IA
    ctrl = (C_Button *)winme->FindControl(SINGLE_FLY_CTRL);

    if (ctrl)
        ctrl->SetCallback(InstantActionFlyCB);

    ctrl = (C_Button *)winme->FindControl(MUNITIONS_BUTTON);

    if (ctrl)
        ctrl->SetCallback(OpenIAMunitionsCB);

    // editbox for high score callback
    ebox = (C_EditBox *)winme->FindControl(IA_HS_NAME_EDIT);

    if (ebox)
    {
        ebox->SetCallback(HighScoreKeyboardCB);

        if ( not *ebox->GetText())
        {
            ebox->SetText(UI_logbk.Name());
        }
    }

    // Hook up Close Button
    ctrl = (C_Button *)winme->FindControl(CLOSE_WINDOW);

    if (ctrl)
        ctrl->SetCallback(CloseWindowCB);

    ctrl = (C_Button *)winme->FindControl(TIME_EARLIER);

    if (ctrl)
        ctrl->SetCallback(ChangeTimeCB);

    ctrl = (C_Button *)winme->FindControl(TIME_LATER);

    if (ctrl)
        ctrl->SetCallback(ChangeTimeCB);

    // Help GUIDE thing
    ctrl = (C_Button*)winme->FindControl(UI_HELP_GUIDE);

    if (ctrl)
        ctrl->SetCallback(UI_Help_Guide_CB);
}

static void SetIAStartup(C_Window *win)
{
    C_Button *btn;

    btn = (C_Button *)win->FindControl(IA_MIS_FTR_CTRL);

    if (btn)
    {
        if (InstantActionSettings.MissionType == _MISSION_AIR_TO_AIR_)
            btn->SetState(1);
        else
            btn->SetState(0);

        btn->Refresh();
    }

    btn = (C_Button *)win->FindControl(IA_MIS_MUD_CTRL);

    if (btn)
    {
        if (InstantActionSettings.MissionType == _MISSION_AIR_TO_GROUND_)
            btn->SetState(1);
        else
            btn->SetState(0);

        btn->Refresh();
    }

    btn = (C_Button *)win->FindControl(IA_LVL_NOVICE_CTRL);

    if (btn)
    {
        if (InstantActionSettings.PilotLevel == _PILOT_LEVEL_NOVICE_)
            btn->SetState(1);
        else
            btn->SetState(0);

        btn->Refresh();
    }

    btn = (C_Button *)win->FindControl(IA_LVL_CADET_CTRL);

    if (btn)
    {
        if (InstantActionSettings.PilotLevel == _PILOT_LEVEL_CADET_)
            btn->SetState(1);
        else
            btn->SetState(0);

        btn->Refresh();
    }

    btn = (C_Button *)win->FindControl(IA_LVL_ROOKIE_CTRL);

    if (btn)
    {
        if (InstantActionSettings.PilotLevel == _PILOT_LEVEL_ROOKIE_)
            btn->SetState(1);
        else
            btn->SetState(0);

        btn->Refresh();
    }

    btn = (C_Button *)win->FindControl(IA_LVL_VETERAN_CTRL);

    if (btn)
    {
        if (InstantActionSettings.PilotLevel == _PILOT_LEVEL_VETERAN_)
            btn->SetState(1);
        else
            btn->SetState(0);

        btn->Refresh();
    }

    btn = (C_Button *)win->FindControl(IA_LVL_ACE_CTRL);

    if (btn)
    {
        if (InstantActionSettings.PilotLevel == _PILOT_LEVEL_ACE_)
            btn->SetState(1);
        else
            btn->SetState(0);

        btn->Refresh();
    }

    btn = (C_Button *)win->FindControl(IA_AD_SAMS_CTRL);

    if (btn)
    {
        if (InstantActionSettings.SamSites == _SAM_SITES_)
            btn->SetState(1);
        else
            btn->SetState(0);

        btn->Refresh();
    }

    btn = (C_Button *)win->FindControl(IA_AD_AAA_CTRL);

    if (btn)
    {
        if (InstantActionSettings.AAASites == _AAA_SITES_)
            btn->SetState(1);
        else
            btn->SetState(0);

        btn->Refresh();
    }
}

static void SetMapStartup(C_Window *win)
{
    C_Cursor *crsr;
    float XPos, YPos;
    float start_x, start_y; //THW 2004-02-14 Random map location

    crsr = (C_Cursor *)win->FindControl(IA_MAP_CURSOR);

    if (crsr)
    {
        instant_action::get_start_position(start_x, start_y);
        /*THW 2004-02-14 Random map location
        XPos=start_y/(FEET_PER_KM/1000.0f*4096.0f*256.0f);
        YPos=1.0f-(start_x/(FEET_PER_KM/1000.0f*4096.0f*256.0f));
        */
        XPos = PRANDFloatPos() * 0.74f + 0.13f;
        YPos = 1.0f - (PRANDFloatPos() * 0.74f + 0.13f);
        XPos *= ((float)(crsr->MaxX_ - crsr->MinX_));
        YPos *= ((float)(crsr->MaxY_ - crsr->MinY_));
        XPos -= crsr->GetW() / 2;
        YPos -= crsr->GetH() / 2;

        if (XPos < 0.0f)
            XPos = 0.0f;

        if (XPos > (float)(win->ClientArea_[0].right - crsr->GetW()))
            XPos = (float)win->ClientArea_[0].right - crsr->GetW();

        if (YPos < 0.0f)
            YPos = 0.0f;

        if (YPos > (float)(win->ClientArea_[0].bottom - crsr->GetH()))
            YPos = (float)win->ClientArea_[0].bottom - crsr->GetH();

        crsr->SetXY((int)XPos + crsr->MinX_, (int)YPos + crsr->MinY_);
        win->RefreshWindow();
    }
}

/*
 for(y=0;y<EVT_MESSAGE_BITS;y++)
 mask[y]=0;

 mask[WeaponFireMsg >> 3] or_eq 0x01 << (WeaponFireMsg bitand 0x0007);
 mask[DeathMessage >> 3] or_eq 0x01 << (DeathMessage bitand 0x0007);
 mask[DamageMsg >> 3] or_eq 0x01 << (DamageMsg bitand 0x0007);
 mask[MissileEndMsg >> 3] or_eq 0x01 << (MissileEndMsg bitand 0x0007);
 mask[LandingMessage >> 3] or_eq 0x01 << (LandingMessage bitand 0x0007);
 mask[EjectMsg >> 3] or_eq 0x01 << (EjectMsg bitand 0x0007);
 mask[PlayerStatusMsg >> 3] or_eq 0x01 << (PlayerStatusMsg bitand 0x0007);

*/

static void ProcessKills(C_Window *win)
{
    int y, fh;
    C_Text *txt;
    _TCHAR buffer[100];
    kill_list *cur;
    long UseFont;

    y = 0;
    UseFont = win->Font_;
    fh = gFontList->GetHeight(UseFont);

    if (AircraftKills)
    {
        cur = AircraftKills;

        while (cur)
        {
            _stprintf(buffer, "%s", GetClassName(cur->id));
            txt = new C_Text;
            txt->Setup(C_DONT_CARE, C_TYPE_LEFT);
            txt->SetText(buffer);
            txt->SetXY(20, y);
            txt->SetFGColor(0xf0f0f0);
            txt->SetFont(UseFont);
            txt->SetFlagBitOn(C_BIT_LEFT);
            txt->SetClient(2);
            win->AddControl(txt);

            _stprintf(buffer, "(%1d)", cur->num);
            txt = new C_Text;
            txt->Setup(C_DONT_CARE, C_TYPE_LEFT);
            txt->SetFixedWidth(_tcsclen(buffer) + 1);
            txt->SetText(buffer);
            txt->SetXY(130, y);
            txt->SetFGColor(0xf0f0f0);
            txt->SetFont(UseFont);
            txt->SetFlagBitOn(C_BIT_LEFT);
            txt->SetClient(2);
            win->AddControl(txt);

            _stprintf(buffer, "%1ld", cur->points);
            AddCommas(buffer);
            txt = new C_Text;
            txt->Setup(C_DONT_CARE, C_TYPE_RIGHT);
            txt->SetFixedWidth(_tcsclen(buffer) + 1);
            txt->SetText(buffer);
            txt->SetXY(215, y);
            txt->SetFGColor(0x00ff00);
            txt->SetFont(UseFont);
            txt->SetFlagBitOn(C_BIT_RIGHT);
            txt->SetClient(2);
            win->AddControl(txt);

            TotalScore += cur->points;

            y += fh;
            cur = cur->next;
        }

        RemoveAircraftKillsList();
    }

    if (ObjectKills)
    {
        cur = ObjectKills;

        while (cur)
        {
            _stprintf(buffer, "%s", GetClassName(cur->id));
            txt = new C_Text;
            txt->Setup(C_DONT_CARE, C_TYPE_LEFT);
            txt->SetText(buffer);
            txt->SetXY(20, y);
            txt->SetFGColor(0xf0f0f0);
            txt->SetFont(UseFont);
            txt->SetFlagBitOn(C_BIT_LEFT);
            txt->SetClient(2);
            win->AddControl(txt);

            _stprintf(buffer, "(%1d)", cur->num);
            txt = new C_Text;
            txt->Setup(C_DONT_CARE, C_TYPE_LEFT);
            txt->SetFixedWidth(_tcsclen(buffer) + 1);
            txt->SetText(buffer);
            txt->SetXY(130, y);
            txt->SetFGColor(0xf0f0f0);
            txt->SetFont(UseFont);
            txt->SetFlagBitOn(C_BIT_LEFT);
            txt->SetClient(2);
            win->AddControl(txt);

            _stprintf(buffer, "%1ld", cur->points);
            AddCommas(buffer);
            txt = new C_Text;
            txt->Setup(C_DONT_CARE, C_TYPE_RIGHT);
            txt->SetFixedWidth(_tcsclen(buffer) + 1);
            txt->SetText(buffer);
            txt->SetXY(215, y);
            txt->SetFGColor(0x00ff00);
            txt->SetFont(UseFont);
            txt->SetFlagBitOn(C_BIT_RIGHT);
            txt->SetClient(2);
            win->AddControl(txt);

            TotalScore += cur->points;

            y += fh;
            cur = cur->next;
        }

        RemoveObjectKillsList();
    }

    txt = (C_Text *)win->FindControl(IA_DBRF_BONUS_FIELD);

    if (txt)
    {
        sprintf(buffer, "%1ld", Bonus - Penalty + LivingBonus + LandingBonus);
        AddCommas(buffer);
        txt->SetText(buffer);
    }

    txt = (C_Text *)win->FindControl(REALISM_MULTIPLIER);

    if (txt)
    {
        sprintf(buffer, "%1ld%%", (long)(PlayerOptions.Realism * 100.00f + 0.5f));
        txt->SetText(buffer);
    }

    // JMB 010711 (total score is always zero)
    // TotalScore *= static_cast<long>(PlayerOptions.Realism);
    TotalScore = long(TotalScore * PlayerOptions.Realism);

    txt = (C_Text *)win->FindControl(IA_DBRF_SCORE_FIELD);

    if (txt)
    {
        sprintf(buffer, "%1ld", TotalScore);
        AddCommas(buffer);
        txt->SetText(buffer);
    }

    win->RefreshWindow();

    /*

     int i,j,index;
     int y,fh;
     C_Text *txt;
     long UseFont;
     _TCHAR buffer[100];
     KillList *airList,*gndList;
     int aircnt,gndcnt;
     VehicleClassDataType *vc;

     y=0;
     UseFont=win->Font_;
     fh=gFontList->GetHeight(UseFont);
     aircnt=0;
     gndcnt=0;

     if(instant_action::get_num_aa_kills ())
     {
     airList=new KillList[instant_action::get_num_air_target_types () + instant_action::get_num_air_threat_types ()];
     for(i=0;i<instant_action::get_num_air_target_types ();i++)
     {
     airList[aircnt].descriptionIndex=instant_action::get_air_target_description_index(0, i);
     airList[aircnt].numKilled=0;
     airList[aircnt].points=0;
     for(j=0;j<=instant_action::get_num_levels();j++)
     {
     airList[aircnt].numKilled+=instant_action::get_air_target_num_killed(j,i);
     airList[aircnt].points+=instant_action::get_air_target_points(j,i);
     }
     aircnt++;
     }

     for(i=0;i<instant_action::get_num_air_threat_types ();i++)
     {
     index=-1;
     for(j=0;j<aircnt;j++)
     if(airList[j].descriptionIndex == instant_action::get_air_threat_description_index (0, i))
     index=j;

     if(index >= 0)
     {
     for(j=0;j<=instant_action::get_num_levels();j++)
     {
     airList[aircnt].numKilled+=instant_action::get_air_threat_num_killed(j, i);
     airList[aircnt].points+=instant_action::get_air_threat_points (j, i);
     }
     }
     else
     {
     airList[aircnt].descriptionIndex=instant_action::get_air_threat_description_index (0, i);
     airList[aircnt].numKilled=0;
     airList[aircnt].points=0;
     for(j=0;j<=instant_action::get_num_levels();j++)
     {
     airList[aircnt].numKilled+=instant_action::get_air_threat_num_killed (j,i);
     airList[aircnt].points+=instant_action::get_air_threat_points (j,i);
     }
     aircnt++;
     }
     }

     for(i=0;i<aircnt;i++)
     {
     if(airList[i].numKilled > 0)
     {
     _stprintf(buffer,"%1d",airList[i].numKilled);
     txt=new C_Text;
     txt->Setup(C_DONT_CARE,C_TYPE_RIGHT);
     txt->SetFixedWidth(_tcsclen(buffer)+1);
     txt->SetText(buffer);
     txt->SetXY(35,y);
     txt->SetFGColor(0xf0f0f0);
     txt->SetFont(UseFont);
     txt->SetFlagBitOn(C_BIT_RIGHT);
     txt->SetClient(2);
     win->AddControl(txt);

     vc = GetVehicleClassData (airList[i].descriptionIndex);
     _stprintf(buffer,"%s",vc->Name);
     txt=new C_Text;
     txt->Setup(C_DONT_CARE,C_TYPE_LEFT);
     txt->SetText(buffer);
     txt->SetXY(45,y);
     txt->SetFGColor(0xf0f0f0);
     txt->SetFont(UseFont);
     txt->SetFlagBitOn(C_BIT_LEFT);
     txt->SetClient(2);
     win->AddControl(txt);

     _stprintf(buffer,"%1ld",airList[i].points);
     AddCommas(buffer);
     txt=new C_Text;
     txt->Setup(C_DONT_CARE,C_TYPE_RIGHT);
     txt->SetFixedWidth(_tcsclen(buffer)+1);
     txt->SetText(buffer);
     txt->SetXY(215,y);
     txt->SetFGColor(0x00ff00);
     txt->SetFont(UseFont);
     txt->SetFlagBitOn(C_BIT_RIGHT);
     txt->SetClient(2);
     win->AddControl(txt);

     TotalScore+=airList[i].points;

     y+=fh;
     }
     }
     delete airList;
     }
     if(instant_action::get_num_ag_kills ())
     {
     gndList=new KillList[instant_action::get_num_gnd_target_types () + instant_action::get_num_gnd_threat_types ()];
     for(i=0;i<instant_action::get_num_gnd_target_types ();i++)
     {
     gndList[gndcnt].descriptionIndex=instant_action::get_gnd_target_description_index(0,i);
     gndList[gndcnt].numKilled=0;
     gndList[gndcnt].points=0;
     for(j=0;j<=instant_action::get_num_levels();j++)
     {
     gndList[gndcnt].numKilled+=instant_action::get_gnd_target_num_killed (j,i);
     gndList[gndcnt].points+=instant_action::get_gnd_target_points (j,i);
     }
     gndcnt++;
     }

     for(i=0;i<instant_action::get_num_gnd_threat_types ();i++)
     {
     index=-1;
     for(j=0;j<gndcnt;j++)
     if(gndList[j].descriptionIndex == instant_action::get_gnd_threat_description_index(0,i))
     index=j;

     if(index >= 0)
     {
     for(j=0;j<=instant_action::get_num_levels();j++)
     {
     gndList[index].numKilled+=instant_action::get_gnd_threat_num_killed (j,i);
     gndList[index].points+=instant_action::get_gnd_threat_points (j,i);
     }
     }
     else
     {
     gndList[gndcnt].descriptionIndex=instant_action::get_gnd_threat_description_index (0,i);
     gndList[gndcnt].numKilled=0;
     gndList[gndcnt].points=0;
     for(j=0;j<=instant_action::get_num_levels();j++)
     {
     gndList[gndcnt].numKilled+=instant_action::get_gnd_threat_num_killed (j,i);
     gndList[gndcnt].points+=instant_action::get_gnd_threat_points (j,i);
     }
     gndcnt++;
     }
     }

     for(i=0;i<gndcnt;i++)
     {
     if(gndList[i].numKilled > 0)
     {
     _stprintf(buffer,"%1d",gndList[i].numKilled);
     txt=new C_Text;
     txt->Setup(C_DONT_CARE,C_TYPE_RIGHT);
     txt->SetFixedWidth(_tcsclen(buffer)+1);
     txt->SetText(buffer);
     txt->SetXY(35,y);
     txt->SetFGColor(0xf0f0f0);
     txt->SetFont(UseFont);
     txt->SetFlagBitOn(C_BIT_RIGHT);
     txt->SetClient(2);
     win->AddControl(txt);

     vc = GetVehicleClassData (gndList[i].descriptionIndex);
     _stprintf(buffer,"%s",vc->Name);
     txt=new C_Text;
     txt->Setup(C_DONT_CARE,C_TYPE_LEFT);
     txt->SetText(buffer);
     txt->SetXY(45,y);
     txt->SetFGColor(0xf0f0f0);
     txt->SetFont(UseFont);
     txt->SetFlagBitOn(C_BIT_LEFT);
     txt->SetClient(2);
     win->AddControl(txt);

     _stprintf(buffer,"%1ld",gndList[i].points);
     AddCommas(buffer);
     txt=new C_Text;
     txt->Setup(C_DONT_CARE,C_TYPE_RIGHT);
     txt->SetFixedWidth(_tcsclen(buffer)+1);
     txt->SetText(buffer);
     txt->SetXY(215,y);
     txt->SetFGColor(0x00ff00);
     txt->SetFont(UseFont);
     txt->SetFlagBitOn(C_BIT_RIGHT);
     txt->SetClient(2);
     win->AddControl(txt);

     TotalScore+=gndList[i].points;

     y+=fh;
     }
     }
     delete gndList;
     }

     TotalScore *= PlayerOptions.Realism;
     txt=(C_Text *)win->FindControl(IA_DBRF_SCORE_FIELD);
     if(txt)
     {
     sprintf(buffer,"%1ld",TotalScore);
     AddCommas(buffer);
     txt->SetText(buffer);
     }
     win->RefreshWindow();
    */
}

static void ProcessBonus(void)
{
    kill_list *cur;

    cur = BonusList;

    while (cur)
    {
        Bonus += cur->points;

        cur = cur->next;
    }

    RemoveBonusList();
}

static void ProcessWeapons(C_Window *win)
{
    int y, fh;
    C_Text *txt;
    _TCHAR buffer[100];
    WeaponClassDataType *wc;
    kill_list *cur;
    long UseFont;

    y = 0;
    UseFont = win->Font_;
    fh = gFontList->GetHeight(UseFont);

    if (WeaponUsage)
    {
        cur = WeaponUsage;

        while (cur)
        {
            wc = (WeaponClassDataType *)Falcon4ClassTable[cur->id].dataPtr;
            _stprintf(buffer, "%s", wc->Name);
            txt = new C_Text;
            txt->Setup(C_DONT_CARE, C_TYPE_LEFT);
            txt->SetText(buffer);
            txt->SetXY(20, y);
            txt->SetFGColor(0xf0f0f0);
            txt->SetFont(UseFont);
            txt->SetFlagBitOn(C_BIT_LEFT);
            txt->SetClient(3);
            win->AddControl(txt);

            _stprintf(buffer, "(%1d)", cur->num);
            txt = new C_Text;
            txt->Setup(C_DONT_CARE, C_TYPE_LEFT);
            txt->SetFixedWidth(_tcsclen(buffer) + 1);
            txt->SetText(buffer);
            txt->SetXY(130, y);
            txt->SetFGColor(0xf0f0f0);
            txt->SetFont(UseFont);
            txt->SetFlagBitOn(C_BIT_LEFT);
            txt->SetClient(3);
            win->AddControl(txt);

            _stprintf(buffer, "%1ld", cur->points);
            AddCommas(buffer);
            txt = new C_Text;
            txt->Setup(C_DONT_CARE, C_TYPE_RIGHT);
            txt->SetFixedWidth(_tcsclen(buffer) + 1);
            txt->SetText(buffer);
            txt->SetXY(215, y);
            txt->SetFGColor(0x00ff00);
            txt->SetFont(UseFont);
            txt->SetFlagBitOn(C_BIT_RIGHT);
            txt->SetClient(3);
            win->AddControl(txt);

            TotalScore += cur->points;

            y += fh;
            cur = cur->next;
        }

        RemoveWeaponUsageList();
    }
    else
    {
        txt = new C_Text;
        txt->Setup(C_DONT_CARE, C_TYPE_LEFT);
        txt->SetText(TXT_NO_WEAPONS_USED);
        txt->SetXY(20, y);
        txt->SetFGColor(0xf0f0f0);
        txt->SetFont(UseFont);
        txt->SetFlagBitOn(C_BIT_LEFT);
        txt->SetClient(3);
        win->AddControl(txt);
    }

    txt = (C_Text *)win->FindControl(IA_DBRF_BONUS_FIELD);

    if (txt)
    {
        sprintf(buffer, "%1ld", Bonus - Penalty + LivingBonus + LandingBonus);
        AddCommas(buffer);
        txt->SetText(buffer);
    }

    txt = (C_Text *)win->FindControl(REALISM_MULTIPLIER);

    if (txt)
    {
        sprintf(buffer, "%1ld%%", (long)(PlayerOptions.Realism * 100.00f + 0.5f));
        txt->SetText(buffer);
    }

    win->RefreshWindow();
}

void CheckHighScore(long TotalScore)
{
    C_Window *win;
    int i;
    C_Text *txt;
    long SoundID;
    _TCHAR buffer[25];

    i = 0;

    while (i < MAX_SCORES and TotalScore <= Scores.Scores[i].Score and Scores.Scores[i].Name[0] not_eq 0)
        i++;

    if (TotalScore < 0)
    {
        SoundID = gInstantBites->Pick(IA1);

        if (SoundID)
            gSoundMgr->PlaySound(SoundID);
    }
    else if (TotalScore < _A_LOW_SCORE_)
    {
        SoundID = gInstantBites->Pick(IA2);

        if (SoundID)
            gSoundMgr->PlaySound(SoundID);
    }
    else if (TotalScore < _A_MEDIUM_SCORE_)
    {
        SoundID = gInstantBites->Pick(IA3);

        if (SoundID)
            gSoundMgr->PlaySound(SoundID);
    }
    else if ( not i)
    {
        SoundID = gInstantBites->Pick(IA8);

        if (SoundID)
            gSoundMgr->PlaySound(SoundID);
    }
    else if (i == 1)
    {
        SoundID = gInstantBites->Pick(IA7);

        if (SoundID)
            gSoundMgr->PlaySound(SoundID);
    }
    else if (i >= 2 and i <= 6)
    {
        SoundID = gInstantBites->Pick(IA6);

        if (SoundID)
            gSoundMgr->PlaySound(SoundID);
    }
    else if (i >= 7 and i <= 11)
    {
        SoundID = gInstantBites->Pick(IA5);

        if (SoundID)
            gSoundMgr->PlaySound(SoundID);
    }
    else
    {
        SoundID = gInstantBites->Pick(IA4);

        if (SoundID)
            gSoundMgr->PlaySound(SoundID);
    }

    if (i < MAX_SCORES)
    {
        win = gMainHandler->FindWindow(IA_HS_WIN);

        if (win)
        {
            gMainHandler->EnableWindowGroup(win->GetGroup());
            txt = (C_Text *)win->FindControl(IA_HS_FIELD);

            if (txt)
            {
                _stprintf(buffer, "%1ld", TotalScore);
                AddCommas(buffer);
                txt->SetText(buffer);
            }

            gMainHandler->WindowToFront(win);
            win->RefreshWindow();
            win->SetControl(IA_HS_NAME_EDIT);
        }
    }
    else
    {
        win = gMainHandler->FindWindow(IA_DBRF_WIN);
        gMainHandler->EnableWindowGroup(win->GetGroup());
    }
}

static void SetupInstantAction()
{
    C_Window *win;
    C_Text *txt;
    _TCHAR buf[20];
    int i;

    win = gMainHandler->FindWindow(IA_SETTINGS_WIN);

    if (win)
    {
        SetIAStartup(win);
        win->RefreshWindow();
    }

    GetHighScores();

    win = gMainHandler->FindWindow(IA_SH_WIN);

    if (win)
    {
        for (i = 0; i < MAX_SCORES; i++)
        {
            if (Scores.Scores[i].Name[0] not_eq 0)
            {
                // Set Name
                txt = (C_Text *)win->FindControl(TEXT_1 + i * 2);

                if (txt)
                {
                    txt->SetText(Scores.Scores[i].Name);
                }

                // Set Score
                txt = (C_Text *)win->FindControl(TEXT_1 + i * 2 + 1);

                if (txt)
                {
                    _stprintf(buf, "%1ld", Scores.Scores[i].Score);
                    txt->SetText(AddCommas(buf));
                }
            }
        }

        win->RefreshWindow();
    }

    win = gMainHandler->FindWindow(IA_MAP_WIN);

    if (win)
        SetMapStartup(win);

    if ((MainLastGroup == 1000))
    {
        // these functions (with scoring) MUST be in this order to work properly
        TotalScore = 0;
        Bonus = 10000;
        Penalty = 0;
        LivingBonus = 0;
        LandingBonus = 0;

        win = gMainHandler->FindWindow(IA_DBRF_WIN);

        if (win)
        {
            ProcessEventList(win, 1);
        }

        //for(i=InstantAction.iaStartLevel;i<InstantAction.iaCurLevel;i++)
        //Bonus+=20000*i;
        win = gMainHandler->FindWindow(IA_DBRF_WIN);

        if (win)
        {
            ProcessBonus();
            ProcessWeapons(win); // Bonus is part of this window
            TotalScore += Bonus - Penalty + LivingBonus + LandingBonus;
            ProcessKills(win); // TotalScore is part of this window
            win->ScanClientAreas();
            win->RefreshWindow();
        }

        CheckHighScore(TotalScore);

        TheCampaign.EndCampaign();
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void OpenIAMunitionsCB(long ID, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if ( not TheCampaign.IsLoaded())
    {
        // Load a campaign here
        strcpy(gUI_CampaignFile, "Instant");

        TheCampaign.SetOnlineStatus(0);
        TheCampaign.LoadCampaign(game_InstantAction, gUI_CampaignFile);

        instant_action::create_player_flight();
    }

    OpenMunitionsWindowCB(ID, hittype, control);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
