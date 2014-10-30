/***************************************************************************\
 UI_lgbk.cpp
 David Power
 July 14,1997

 Logbook screen for FreeFalcon
\***************************************************************************/
#include <windows.h>
#include <targa.h>
#include "falclib.h"
#include "dxutil/ddutil.h"
#include "Graphics/Include/imagebuf.h"
#include "dispcfg.h"
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
#include "find.h"
#include "flight.h"
#include "falcuser.h"
#include "f4find.h"
#include "falcsess.h"
#include "f4error.h"
#include "ui_ia.h"
#include "ui_dgfgt.h"
#include "playerop.h"
#include "userids.h"
#include "logbook.h"
#include "userids.h"
#include "cstringrc.h"
#include "textids.h"
//#include "ui_lgbk.h"

#define _USE_REGISTRY_ 1

extern C_Handler *gMainHandler;
extern C_Parser *gMainParser;

static void HookupPWControls(long ID);
static void HookupLBControls(long ID);
void CloseWindowCB(long ID, short hittype, C_Base *control);
void CloseLogWindowCB(long ID, short hittype, C_Base *control);
void UI_Help_Guide_CB(long ID, short hittype, C_Base *ctrl);
void Uni_Float(_TCHAR *buffer);
void LBSetupControls(IMAGE_RSC *Picture = NULL, IMAGE_RSC *Patch = NULL);
int SetImage(long ID, _TCHAR *filename , long ImageID);
int SetResourceImage(long ID, long ImageID);
void MakeVirtualListFromRsc(long ID, long startid);
void LoadAFile(long TitleID, _TCHAR *filespec, _TCHAR *excludelist[], void (*YesCB)(long, short, C_Base*), void (*NoCB)(long, short, C_Base*));
void LoadVirtualFile(long TitleID, _TCHAR *filespec, _TCHAR *excludelist[], void (*YesCB)(long, short, C_Base*), void (*NoCB)(long, short, C_Base*), _TCHAR virtuallist[200][64], void (*VirtualCB)(long, short, C_Base*));
void SaveLogBookCB(long ID, short hittype, C_Base *control);
int CheckCallsign(_TCHAR *filename);
void AwardDevices(C_Window *win, long ID, uchar Medal, uchar Number);
int UpdateKeyMapList(char *fname, int flag);
void GetPilotList(C_Window *win, _TCHAR *fspec, _TCHAR *excludelist[],
                  C_ListBox *lbox, BOOL cutext, BOOL exclude_te);
int SetPilot(_TCHAR *callsign, C_ListBox *lbox);
int SaveControlValues(void);
void AreYouSure(long TitleID, long MessageID, void (*OkCB)(long, short, C_Base*), void (*CancelCB)(long, short, C_Base*));
void AreYouSure(long TitleID, _TCHAR *text, void (*OkCB)(long, short, C_Base*), void (*CancelCB)(long, short, C_Base*));


//for testing
void AwardWindow(void);
void CourtMartialWindow(void);
void PromotionWindow(void);
void LoadCommonWindows(void);

_TCHAR *UI_WordWrap(C_Window *win, _TCHAR *str, long fontid, short width, BOOL *status);

extern _TCHAR VirtualFileList[200][64];
extern int LBLoaded;

enum
{
    PW_TITLE_BAR = 140002,
};

enum
{
    DEV_STAR = 0,
    DEV_OAK = 1,
};

extern C_String *gStringMgr;

long gFullRanksTxt[NUM_RANKS] = { TXT_SEC_LT,
                                    TXT_LEIUTENANT,
                                    TXT_CAPTAIN,
                                    TXT_MAJOR,
                                    TXT_LT_COL,
                                    TXT_COLONEL,
                                    TXT_BRIG_GEN,
                                };

long gRanksTxt[NUM_RANKS] =
{
    TXT_ABBRV_RANK_SEC_LT,
    TXT_ABBRV_RANK_LIEUTENANT,
    TXT_ABBRV_RANK_CAPTAIN,
    TXT_ABBRV_RANK_MAJOR,
    TXT_ABBRV_RANK_LT_COL,
    TXT_ABBRV_RANK_COLONEL,
    TXT_ABBRV_RANK_BRIG_GEN,
};
int LogState = 0;
long CurControl = 0;
int CurPatch = 0;
int CurPic = 0;

LogBookData UI_logbk;
int PWLoaded = 0;
RECT PicArea;



void F4DialogBox(_TCHAR *string, void (*YesCB)(long, short, C_Base*),
                 void (*NoCB)(long, short, C_Base*))
{
    C_Window *win;
    C_Button *btn;
    C_Text  *text;



    win = gMainHandler->FindWindow(DIALOG_WIN);

    if (win)
    {
        text = (C_Text *)win->FindControl(MESSAGE1);

        if (text)
        {
            text->Refresh();
            text->SetText(string);
            text->Refresh();
        }

#if 0
        fontid = text->GetFont();
        strptr = UI_WordWrap(win, string, fontid, 180, &status);

        if (strptr)
        {
            text->SetText(strptr);
            text->Refresh();
        }
    }

    strptr = UI_WordWrap(win, NULL, fontid, 180, &status);

    if (strptr)
    {
        text = (C_Text *)win->FindControl(MESSAGE2);

        if (text)
        {
            text->SetText(strptr);
            text->Refresh();
        }
    }

    strptr = UI_WordWrap(win, NULL, fontid, 180, &status);

    if (strptr)
    {
        text = (C_Text *)win->FindControl(MESSAGE3);

        if (text)
        {
            text->SetText(strptr);
            text->Refresh();
        }
    }

#endif

    btn = (C_Button *)win->FindControl(OK);

    if (btn)
    {
        if (YesCB)
            btn->SetCallback(YesCB);
        else
            btn->SetCallback(CloseWindowCB);

        if (NoCB)
            btn->SetX(btn->GetUserNumber(0));
        else
            btn->SetX(btn->GetUserNumber(1));
    }

    btn = (C_Button *)win->FindControl(CLOSE_WINDOW);

    if (btn)
    {
        if (NoCB)
            btn->SetCallback(NoCB);
        else
            btn->SetCallback(CloseWindowCB);
    }

    btn = (C_Button *)win->FindControl(CANCEL);

    if (btn)
    {
        if (NoCB)
        {
            btn->SetFlagBitOff(C_BIT_INVISIBLE);
            btn->SetCallback(NoCB);
        }
        else
        {
            btn->SetFlagBitOn(C_BIT_INVISIBLE);
            btn->SetCallback(CloseWindowCB);
        }
    }

    gMainHandler->ShowWindow(win);
    gMainHandler->WindowToFront(win);
}
}

void F4DialogBox(long ID, void (*YesCB)(long, short, C_Base*),
                 void (*NoCB)(long, short, C_Base*))
{
    F4DialogBox(gStringMgr->GetText(ID), YesCB, NoCB);
}

void ChoosePilotCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_SELECT)
        return;

    _TCHAR Pilot[MAX_PATH];
    _tcscpy(Pilot, ((C_ListBox *)control)->GetText());

    //return if current logbook is selected
    if ( not _tcscmp(Pilot, UI_logbk.Callsign()))
        return;

    UI_logbk.LoadData(Pilot);
    LBSetupControls();

}

void PasswordWindow(long TitleID, long MessageID, void (*YesCB)(long, short, C_Base*), void (*NoCB)(long, short, C_Base*))
{
    C_Window *win;
    C_Button *btn;
    C_Text *text;
    C_ListBox *lbox;
    C_EditBox *ebox;

    if ( not YesCB or not NoCB)
        return;

    if (LogState bitand LB_CHECKED)
        return;

    win = gMainHandler->FindWindow(PASSWORD_WIN);

    if (win)
    {
        lbox = (C_ListBox *)win->FindControl(LOGBOOK_LIST);

        if (lbox)
        {
            _TCHAR buf[MAX_PATH];

            if (TitleID == TXT_VERIFY_PASSWORD)
            {
                lbox->SetCallback(NULL);
                lbox->SetFlagBitOff(C_BIT_ENABLED);

                lbox->RemoveAllItems();

                C_Window *win = gMainHandler->FindWindow(LOG_WIN);

                if (win)
                    ebox = (C_EditBox *)win->FindControl(CALLSIGN_LIST);
                else
                    ebox = NULL;

                if (ebox not_eq NULL)
                    lbox->AddItem(1, C_TYPE_ITEM, ebox->GetText());
                else
                    lbox->AddItem(1, C_TYPE_ITEM, UI_logbk.Callsign());
            }
            else
            {
                _stprintf(buf, _T("%s\\config\\*.lbk"), FalconDataDirectory);
                GetPilotList(win, buf, NULL, lbox, TRUE, TRUE);

                SetPilot(UI_logbk.Callsign(), lbox);
                lbox->SetFlagBitOn(C_BIT_ENABLED);
                lbox->SetCallback(ChoosePilotCB);
            }
        }

        text = (C_Text *)win->FindControl(MESSAGE1);

        if (text)
        {
            text->Refresh();
            text->SetText(MessageID);
            text->Refresh();
        }

#if 0
        fontid = text->GetFont();
        strptr = UI_WordWrap(win, mess, fontid, 250, &status);

        if (strptr)
        {
            text->SetText(strptr);
        }
        else
            text->SetText(TXT_SPACE);

        text->Refresh();
    }

    text = (C_Text *)win->FindControl(MESSAGE2);

    if (text)
    {
        text->Refresh();
        strptr = UI_WordWrap(win, NULL, fontid, 250, &status);

        if (strptr)
        {
            text->SetText(strptr);
        }
        else
            text->SetText(TXT_SPACE);

        text->Refresh();
    }

#endif
    text = (C_Text*)win->FindControl(PW_TITLE_BAR);

    if (text)
    {
        text->SetText(TitleID);
        text->Refresh();
    }

    btn = (C_Button *)win->FindControl(OK);

    if (btn)
    {
        btn->SetCallback(YesCB);
    }

    ebox = (C_EditBox*)win->FindControl(PASSWORD);

    if (ebox)
    {
        win->SetControl(PASSWORD);
    }

    btn = (C_Button *)win->FindControl(CLOSE_WINDOW);

    if (btn)
    {
        btn->SetCallback(NoCB);
    }

    btn = (C_Button *)win->FindControl(LOG_NEW);

    if (btn)
    {
        btn->Refresh();
        btn->SetCallback(NoCB);

        if (TitleID == TXT_VERIFY_PASSWORD)
        {
            btn->SetLabel(0, TXT_CANCEL);
            btn->SetLabel(1, TXT_CANCEL);
        }
        else
        {
            btn->SetLabel(0, TXT_NEW);
            btn->SetLabel(1, TXT_NEW);
        }

        btn->Refresh();
    }

    gMainHandler->ShowWindow(win);
    gMainHandler->WindowToFront(win);
}
}

void CheckPasswordCB(long, short hittype, C_Base *control)
{
    C_EditBox * ebox;
    _TCHAR pwd[MAX_PATH];

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    ebox = (C_EditBox*)control->Parent_->FindControl(PASSWORD);

    if (ebox)
    {
        _tcscpy(pwd, ebox->GetText());
        ebox->SetText(_T(""));
        ebox->Refresh();

        if (UI_logbk.CheckPassword(pwd))
        {
            LogState or_eq LB_EDITABLE bitor LB_CHECKED;
            gMainHandler->HideWindow(control->Parent_);
            LogBook.LoadData(&UI_logbk.Pilot);
            UpdateKeyMapList(PlayerOptions.keyfile, 1);
        }
    }
}

void RealLoadLogbook() // without daves extra garbage
{
    long ID;

    if (LBLoaded)
        return;

    if (_LOAD_ART_RESOURCES_)
        gMainParser->LoadImageList("lb_res.lst");
    else
        gMainParser->LoadImageList("lb_art.lst");

    gMainParser->LoadSoundList("lb_snd.lst");
    gMainParser->LoadWindowList("lb_scf.lst"); // Modified by M.N. - add art/art1024 by LoadWindowList

    ID = gMainParser->GetFirstWindowLoaded();

    while (ID)
    {
        HookupLBControls(ID);
        ID = gMainParser->GetNextWindowLoaded();
    }

    LBLoaded++;
}

void LoadLogBookWindows(LB_PILOT *Pilot = &LogBook.Pilot, int flag = LB_EDITABLE,
                        IMAGE_RSC *Picture = NULL, IMAGE_RSC *Patch = NULL)
{
    UI_logbk.LoadData(Pilot);

    LogState or_eq flag bitor LB_REFRESH_PILOT;


    if (LBLoaded)
    {
        LBSetupControls(Patch, Picture);
        return;
    }

    C_Window *win = gMainHandler->FindWindow(LOG_WIN);

    if (win)
    {
        C_Box *box = (C_Box *)win->FindControl(50096);//PIC_BOX

        if (box)
        {
            PicArea.left = box->GetX();
            PicArea.right = box->GetX() + box->GetW();
            PicArea.top = box->GetY();
            PicArea.bottom = box->GetY() + box->GetH();
        }
    }

    LBSetupControls(Patch, Picture);
}

void NoPasswordCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    LogBook.Initialize();
    UI_logbk.Initialize();

    UpdateKeyMapList(PlayerOptions.keyfile, 0);
    LoadLogBookWindows();

    gMainHandler->HideWindow(control->Parent_);
}


int SetPilot(_TCHAR *callsign, C_ListBox *lbox)
{
    int i = 1;
    _TCHAR *CurCallsign;

    lbox->SetValue(i);

    while (i == lbox->GetTextID())
    {
        CurCallsign = lbox->GetText();

        if (CurCallsign)
        {
            if ( not _tcsicmp(CurCallsign, callsign))
            {
                return TRUE;
            }
        }

        lbox->SetValue(++i);
    }

    return FALSE;
}

void GetPilotList(C_Window *win, _TCHAR *fspec, _TCHAR *excludelist[],
                  C_ListBox *lbox, BOOL cutext, BOOL exclude_te)
{
    char path[100];
    WIN32_FIND_DATA filedata;
    HANDLE ffhnd;
    BOOL last, ignore;
    long i; //,y=0;
    int items = 0;
    _TCHAR *dst, *ptr, *extension;

    if ( not win or not lbox) return;

    ffhnd = FindFirstFile(fspec, &filedata);
    last = (ffhnd not_eq INVALID_HANDLE_VALUE);

    lbox->RemoveAllItems();

    if (exclude_te)
    {
        ptr = fspec;
        dst = path;
        extension = NULL;

        while (*ptr)
        {
            if (*ptr == '\\')
                extension = dst;

            *dst = *ptr;
            dst ++;
            ptr ++;
        }

        *dst = '\0';

        if (extension)
            *extension = '\0';
    }

    while (last)
    {
        if (cutext)
        {
            ptr = filedata.cFileName;
            extension = NULL;

            while (*ptr)
            {
                if (*ptr == '.')
                    extension = ptr;

                ptr ++;
            }

            if (extension)
                *extension = 0;
        }

        ignore = FALSE;

        if (excludelist)
        {
            i = 0;

            while (excludelist[i] and not ignore)
            {
                if (stricmp(excludelist[i], filedata.cFileName) == 0)
                    ignore = TRUE;

                i++;
            }
        }

        if ( not ignore)
        {
            items++;
            lbox->AddItem(items, C_TYPE_ITEM, filedata.cFileName);
        }

        last = FindNextFile(ffhnd, &filedata);
    }

    lbox->Refresh();

    if (ffhnd not_eq INVALID_HANDLE_VALUE)
        FindClose(ffhnd);  // JPO handle leak
}

void LBSetupControls(IMAGE_RSC *Picture, IMAGE_RSC *Patch)
{
    C_EditBox *ebox;
    C_Window *win;
    C_Button *button;
    C_Text *text;
    C_ListBox *lbox;
    long imageID;
    //TJL 12/01/03 Add Pointer here
    CAMP_STATS *camp = UI_logbk.GetCampaign();

    win = gMainHandler->FindWindow(LOG_WIN);

    if (win)
    {
        button = (C_Button *)win->FindControl(LOG_OK);

        if (button not_eq NULL)
        {
            if (LogState bitand LB_OPPONENT)
                button->SetFlagBitOn(C_BIT_INVISIBLE);
            else
                button->SetFlagBitOff(C_BIT_INVISIBLE);
        }

        button = (C_Button *)win->FindControl(LOG_NEW);

        if (button not_eq NULL)
        {
            if (LogState bitand LB_OPPONENT)
                button->SetFlagBitOn(C_BIT_INVISIBLE);
            else
                button->SetFlagBitOff(C_BIT_INVISIBLE);
        }

        button = (C_Button *)win->FindControl(LOG_CLEAR);

        if (button not_eq NULL)
        {
            if (LogState bitand LB_OPPONENT)
                button->SetFlagBitOn(C_BIT_INVISIBLE);
            else
                button->SetFlagBitOff(C_BIT_INVISIBLE);
        }

        button = (C_Button *)win->FindControl(LOG_CANCEL);

        if (button not_eq NULL)
        {
            if (LogState bitand LB_OPPONENT)
                button->SetFlagBitOn(C_BIT_INVISIBLE);
            else
                button->SetFlagBitOff(C_BIT_INVISIBLE);
        }


        button = (C_Button *)win->FindControl(PATCH_PIC);

        if (button)
        {
            if ( not (LogState bitand LB_EDITABLE) or LogState bitand LB_OPPONENT)
                button->SetFlagBitOff(C_BIT_ENABLED);
            else
                button->SetFlagBitOn(C_BIT_ENABLED);

            if (Patch)
            {
                _TCHAR buf[MAX_PATH];
                _stprintf(buf, _T("%s\\patches\\%s.tga"), FalconDataDirectory, Patch);
                SetImage(PATCH_PIC, buf, CurPatch);
            }
            else if (*(UI_logbk.GetPatch()) not_eq 0)
            {
                _TCHAR buf[MAX_PATH];
                _stprintf(buf, _T("%s\\patches\\%s.tga"), FalconDataDirectory, UI_logbk.GetPatch());
                SetImage(PATCH_PIC, buf, CurPatch);
            }
            else
            {
                imageID = NOPATCH;

                if (UI_logbk.GetPatchResource())
                    imageID = UI_logbk.GetPatchResource();

                button->SetImage(C_STATE_0, imageID);
                button->SetImage(C_STATE_1, imageID);
                button->SetImage(C_STATE_SELECTED, imageID);
                button->SetImage(C_STATE_DISABLED, imageID);
                button->Refresh();
                CurPatch = 0;
            }
        }

        button = (C_Button *)win->FindControl(PILOT_PIC);

        if (button)
        {
            if ( not (LogState bitand LB_EDITABLE) or LogState bitand LB_OPPONENT)
                button->SetFlagBitOff(C_BIT_ENABLED);
            else
                button->SetFlagBitOn(C_BIT_ENABLED);

            if (Picture)
            {
                button->SetImage(C_STATE_0, Picture);
                button->SetImage(C_STATE_1, Picture);
                button->SetImage(C_STATE_SELECTED, Picture);
                button->SetImage(C_STATE_DISABLED, Picture);
                button->Refresh();
            }
            else if (*(UI_logbk.GetPicture()) not_eq 0)
            {
                _TCHAR buf[MAX_PATH];
                _stprintf(buf, _T("%s\\pictures\\%s.tga"), FalconDataDirectory, UI_logbk.GetPicture());
                SetImage(PILOT_PIC, buf, CurPic);
            }
            else
            {
                imageID = NOFACE;

                if (UI_logbk.GetPictureResource())
                    imageID = UI_logbk.GetPictureResource();

                button->SetImage(C_STATE_0, imageID);
                button->SetImage(C_STATE_1, imageID);
                button->SetImage(C_STATE_SELECTED, imageID);
                button->SetImage(C_STATE_DISABLED, imageID);
                button->Refresh();
                CurPic = 0;
            }
        }


        ebox = (C_EditBox *)win->FindControl(PASSWORD_LIST);

        if (ebox)
        {
            if ( not (LogState bitand LB_EDITABLE) or LogState bitand LB_OPPONENT)
                ebox->SetFlagBitOff(C_BIT_ENABLED);
            else
                ebox->SetFlagBitOn(C_BIT_ENABLED);

            _TCHAR Pwd[PASSWORD_LEN + 1];

            if (UI_logbk.GetPassword(Pwd))
                ebox->SetText(Pwd);
            else
                ebox->SetText(_T(""));

            ebox->Refresh();
        }




        ebox = (C_EditBox *)win->FindControl(PILOT_LIST);

        if (ebox)
        {
            if ( not (LogState bitand LB_EDITABLE) or LogState bitand LB_OPPONENT)
                ebox->SetFlagBitOff(C_BIT_ENABLED);
            else
                ebox->SetFlagBitOn(C_BIT_ENABLED);

            ebox->SetText(UI_logbk.Name());
            ebox->Refresh();
        }

        ebox = (C_EditBox *)win->FindControl(CALLSIGN_LIST);

        if (ebox)
        {
            if ( not (LogState bitand LB_EDITABLE) or LogState bitand LB_OPPONENT)
                ebox->SetFlagBitOff(C_BIT_ENABLED);
            else
                ebox->SetFlagBitOn(C_BIT_ENABLED);

            ebox->SetText(UI_logbk.Callsign());
            ebox->Refresh();
        }

        if (LogState bitand LB_REFRESH_PILOT)
        {
            lbox = (C_ListBox *)win->FindControl(LOGBOOK_LIST);

            if (lbox)
            {
                _TCHAR buf[MAX_PATH];
                _stprintf(buf, _T("%s\\config\\*.lbk"), FalconDataDirectory);
                GetPilotList(win, buf, NULL, lbox, TRUE, TRUE);

                if (ebox)
                    SetPilot(ebox->GetText(), lbox);

                LogState and_eq compl LB_REFRESH_PILOT;
            }
        }

        lbox = (C_ListBox *)win->FindControl(VOICE_CHOICE);

        if (lbox)
        {
            lbox->SetValue(UI_logbk.Voice() + 1);
        }

        ebox = (C_EditBox *)win->FindControl(PERSONAL_TEXT);

        if (ebox)
        {
            if ( not (LogState bitand LB_EDITABLE) or LogState bitand LB_OPPONENT)
                ebox->SetFlagBitOff(C_BIT_ENABLED);
            else
                ebox->SetFlagBitOn(C_BIT_ENABLED);

            ebox->SetText(UI_logbk.Personal());
            ebox->Refresh();
        }

        ebox = (C_EditBox *)win->FindControl(SQUADRON_NAME);

        if (ebox)
        {
            if ( not (LogState bitand LB_EDITABLE) or LogState bitand LB_OPPONENT)
                ebox->SetFlagBitOff(C_BIT_ENABLED);
            else
                ebox->SetFlagBitOn(C_BIT_ENABLED);

            ebox->SetText(UI_logbk.Squadron());
            ebox->Refresh();
        }

        button = (C_Button *)win->FindControl(50095);//RANKS value was changed

        if (button)
        {
            button->SetState(static_cast<short>(UI_logbk.Rank()));
            button->SetHelpText(gFullRanksTxt[UI_logbk.Rank()]);
            button->Refresh();
        }

        text = (C_Text *)win->FindControl(COMMISSIONED_FIELD);

        if (text)
        {
            text->SetText(UI_logbk.Commissioned());
            text->Refresh();
        }

        _TCHAR buf[15];

        text = (C_Text *)win->FindControl(HOURS_FIELD);

        if (text)
        {
            _stprintf(buf, "%4.1f", UI_logbk.FlightHours());
            Uni_Float(buf);
            text->SetText(buf);
            text->Refresh();
        }

        text = (C_Text *)win->FindControl(ACE_FIELD);

        if (text)
        {
            _stprintf(buf, "%1.3f", UI_logbk.AceFactor());
            Uni_Float(buf);
            text->SetText(buf);
            text->Refresh();
        }

        //TJL 12/01/03 Test adding a new field to logbook
        //Add Total Score
        text = (C_Text *)win->FindControl(TOTAL_SCORE);

        if (text)
        {
            _stprintf(buf, _T("%6d"), camp->TotalScore);
            text->SetText(buf);
            text->Refresh();
        }

        //Add Mission Score
        text = (C_Text *)win->FindControl(MISSION_SCORE);

        if (text)
        {
            _stprintf(buf, _T("%6d"), camp->TotalMissionScore);
            text->SetText(buf);
            text->Refresh();
        }

        //Add Consecutive Missions
        text = (C_Text *)win->FindControl(CONSEC_MISSIONS);

        if (text)
        {
            _stprintf(buf, _T("%6d"), camp->ConsecMissions);
            text->SetText(buf);
            text->Refresh();
        }

        //Add Friendlies Killed
        text = (C_Text *)win->FindControl(FRIENDLIES_KILLED);

        if (text)
        {
            _stprintf(buf, _T("%6d"), camp->FriendliesKilled);
            text->SetText(buf);
            text->Refresh();
        }

        //AVG A/A kills  Kills/Camp Missions
        text = (C_Text *)win->FindControl(AVGAA_KILLS);

        if (text)
        {
            if (camp->Missions)
                _stprintf(buf, _T("%2.3f"), (double)camp->Kills / camp->Missions);
            else
                _stprintf(buf, "0");

            Uni_Float(buf);
            text->SetText(buf);
            text->Refresh();
        }

        //AVG A/G kills  Kills/Camp Missions
        text = (C_Text *)win->FindControl(AVGAG_KILLS);

        if (text)
        {
            if (camp->Missions)
                _stprintf(buf, _T("%2.3f"), ((double)camp->AirToGround + camp->Static + camp->Naval) / camp->Missions);
            else
                _stprintf(buf, "0");

            Uni_Float(buf);
            text->SetText(buf);
            text->Refresh();
        }

        //AVG Mis Duration
        text = (C_Text *)win->FindControl(AVGMIS_DURATION);

        if (text)
        {
            if (camp->Missions)
                _stprintf(buf, _T("%2.3f"), ((double)UI_logbk.FlightHours() / camp->Missions));
            else
                _stprintf(buf, "0");

            Uni_Float(buf);
            text->SetText(buf);
            text->Refresh();
        }



        //End

        text = (C_Text *)win->FindControl(FRAGRATIO_FIELD);

        if (text)
        {
            int kills = UI_logbk.TotalKills();
            int killed = UI_logbk.TotalKilled();
            _stprintf(buf, "%2d/%2d", kills, killed);
            text->SetText(buf);
            text->Refresh();
        }

        DF_STATS *dgft = UI_logbk.GetDogfight();

        text = (C_Text *)win->FindControl(DF_MATCH_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%3d"), dgft->MatchesWon + dgft->MatchesLost);
            text->SetText(buf);
            text->Refresh();
        }

        text = (C_Text *)win->FindControl(DF_WON_LOST_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%2d/%2d"), dgft->MatchesWon, dgft->MatchesLost);
            text->SetText(buf);
            text->Refresh();
        }

        //DF_MATCH_VSHUMANS_FIELD
        text = (C_Text *)win->FindControl(DF_MATCH_VSHUMANS_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%2d/%2d"), dgft->MatchesWonVHum, dgft->MatchesLostVHum);
            text->SetText(buf);
            text->Refresh();
        }

        //DF_KILL_FIELD
        text = (C_Text *)win->FindControl(DF_KILL_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%2d/%2d"), dgft->Kills, dgft->Killed);
            text->SetText(buf);
            text->Refresh();
        }

        //DF_H2H_FIELD
        //DF_VS_FIELD
        text = (C_Text *)win->FindControl(DF_VS_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%2d/%2d"), dgft->HumanKills, dgft->KilledByHuman);
            text->SetText(buf);
            text->Refresh();
        }

        //TJL 12/01/03 Bump this higher.
        //CAMP_STATS *camp = UI_logbk.GetCampaign();

        //CAMP_CAMPAIGNS_FIELD
        text = (C_Text *)win->FindControl(CAMP_CAMPAIGNS_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%d/%d/%d"), camp->GamesWon, camp->GamesLost, camp->GamesTied);
            text->SetText(buf);
            text->Refresh();
        }

        //CAMP_MISS_FIELD
        text = (C_Text *)win->FindControl(CAMP_MISS_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%4d"), camp->Missions);
            text->SetText(buf);
            text->Refresh();
        }

        //CAMP_RATING_FIELD
        text = (C_Text *)win->FindControl(CAMP_RATING_FIELD);

        if (text)
        {
            if (camp->Missions)
                _stprintf(buf, _T("%2.3f"), (double)camp->TotalMissionScore / camp->Missions);
            else
                _stprintf(buf, "0");

            Uni_Float(buf);
            text->SetText(buf);
            text->Refresh();
        }

        //CAMP_KILL_FIELD
        text = (C_Text *)win->FindControl(CAMP_KILL_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%2d/%2d"), camp->Kills, camp->Killed);
            text->SetText(buf);
            text->Refresh();
        }

        //CAMP_VS_FIELD
        text = (C_Text *)win->FindControl(CAMP_VS_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%2d/%2d"), camp->HumanKills, camp->KilledByHuman);
            text->SetText(buf);
            text->Refresh();
        }

        text = (C_Text *)win->FindControl(CAMP_AA_KILLS_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%4d"), camp->Kills);
            text->SetText(buf);
            text->Refresh();
        }

        text = (C_Text *)win->FindControl(CAMP_AG_KILLS_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%4d"), camp->AirToGround);
            text->SetText(buf);
            text->Refresh();
        }

        text = (C_Text *)win->FindControl(CAMP_STATIC_KILLS_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%4d"), camp->Static);
            text->SetText(buf);
            text->Refresh();
        }

        text = (C_Text *)win->FindControl(CAMP_NAVAL_KILLS_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%4d"), camp->Naval);
            text->SetText(buf);
            text->Refresh();
        }

        text = (C_Text *)win->FindControl(CAMP_DEATHS_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%4d"), camp->Killed);
            text->SetText(buf);
            text->Refresh();
        }

        //MEDALS_AFC
        //MEDALS_AM
        //MEDALS_DFC
        //MEDALS_SS
        //MEDALS_KC
        int i;
        //need to also do stars and oakleafs for multiple medals
        int CurrMedal = MEDALS_AFC;

        for (i = 0; i < NUM_MEDALS ; i++)
        {
            if (UI_logbk.Pilot.Medals[i])
            {
                button = (C_Button *)win->FindControl(CurrMedal);

                if (button)
                {
                    AwardDevices(win, CurrMedal, static_cast<uchar>(i), UI_logbk.Pilot.Medals[i]);
                    button->SetFlagBitOff(C_BIT_INVISIBLE);
                    button->SetState(static_cast<short>(i));
                    button->SetHelpText(gStringMgr->AddText(button->GetLabel(static_cast<short>(i))));
                    button->Refresh();
                    CurrMedal++;
                }
            }

        }

        for (i = CurrMedal; i < NUM_MEDALS + MEDALS_AFC ; i++)
        {
            button = (C_Button *)win->FindControl(i);

            if (button)
            {
                AwardDevices(win, i, 0, 0);
                button->SetFlagBitOn(C_BIT_INVISIBLE);
                button->Refresh();
            }
        }

        win->RefreshWindow();
    }
}

//ID is the control which needs devices,
//Medal is which medal(so we know what kind of devices to use)
//Number is how times it has been awarded
void AwardDevices(C_Window *win, long ID, uchar Medal, uchar Number)
{
    if ( not win)
        return;

    int Awarded = 0;
    int OakOrStar = DEV_OAK;

    if (Medal == KOREA_CAMPAIGN)
        OakOrStar = DEV_STAR;

    C_Button *button;

    int first_dev;
    int device = (ID - MEDALS_AFC) * 4 + MEDALS_AFC_DEVICE_1;
    first_dev = device;

    if (Number > 0)
        Awarded++;

    while (Number - Awarded > 4)
    {
        button = (C_Button *)win->FindControl(device++);

        if (button)
        {
            button->SetFlagBitOff(C_BIT_INVISIBLE);

            if (OakOrStar == DEV_OAK)
            {
                button->SetState(1);
                button->Refresh();
            }
            else
            {
                button->SetState(3);
                button->Refresh();
            }

            button->SetHelpText(gStringMgr->AddText("5"));

        }

        Awarded += 5;
    }

    if (Number > 9)
    {
        button = (C_Button *)win->FindControl(device++);

        if (button)
        {
            button->SetFlagBitOff(C_BIT_INVISIBLE);

            if (OakOrStar == DEV_OAK)
            {
                button->SetState(1);
                button->Refresh();
            }
            else
            {
                button->SetState(3);
                button->Refresh();
            }

            button->SetHelpText(gStringMgr->AddText("5"));
        }

    }
    else
    {
        while (Number - Awarded)
        {

            button = (C_Button *)win->FindControl(device++);

            if (button)
            {
                button->SetFlagBitOff(C_BIT_INVISIBLE);

                if (OakOrStar == DEV_OAK)
                {
                    button->SetState(0);
                    button->Refresh();
                }
                else
                {
                    button->SetState(2);
                    button->Refresh();
                }

                button->SetHelpText(gStringMgr->AddText("1"));
            }

            Awarded++;
        }
    }

    while ((device - first_dev) < 4)
    {
        button = (C_Button *)win->FindControl(device++);

        if (button)
        {
            button->SetFlagBitOn(C_BIT_INVISIBLE);
            button->Refresh();
        }
    }
}


void LoadTGACB(long, short hittype, C_Base *control)
{
    C_EditBox * ebox;
    _TCHAR fname[MAX_PATH];
    long imageID;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gMainHandler->HideWindow(control->Parent_);

    ebox = (C_EditBox*)control->Parent_->FindControl(FILE_NAME);

    if (ebox)
    {
        _tcscpy(fname, ebox->GetText());

        for (unsigned long i = 0; i < _tcslen(fname); i++)
            if (fname[i] == '.')
                fname[i] = 0;

        if (fname[0] == 0)
            return;

        _TCHAR buf[MAX_PATH];

        if (CurControl == PATCH_PIC)
        {
            imageID = CurPatch;
            _stprintf(buf, _T("%s\\patches\\%s.tga"), FalconDataDirectory, fname);
        }
        else
        {
            imageID = CurPic;
            _stprintf(buf, _T("%s\\pictures\\%s.tga"), FalconDataDirectory, fname);
        }

        if (SetImage(CurControl, buf, imageID))
        {
            if (CurControl == PATCH_PIC)
            {
                UI_logbk.SetPatch(fname);
                UI_logbk.SetSquadron(fname);

                C_Window *win;
                win = gMainHandler->FindWindow(LOG_WIN);

                if (win)
                {
                    ebox = (C_EditBox*)win->FindControl(SQUADRON_NAME);

                    if (ebox)
                    {
                        ebox->SetText(fname);
                        ebox->Refresh();
                    }
                }
            }
            else if (CurControl == PILOT_PIC)
                UI_logbk.SetPicture(fname);
        }
    }
}

void LoadVirtualTGACB(long, short hittype, C_Base *control)
{
    C_EditBox * ebox;
    _TCHAR fname[MAX_PATH];
    long imageID;
    C_Resmgr *res;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gMainHandler->HideWindow(control->Parent_);

    ebox = (C_EditBox*)control->Parent_->FindControl(FILE_NAME);

    if (ebox)
    {
        _tcscpy(fname, ebox->GetText());

        if (fname[0] == 0)
            return;

        if (CurControl == PATCH_PIC)
            res = gImageMgr->GetImageRes(PATCHES_RESOURCE);
        else
            res = gImageMgr->GetImageRes(PILOTS_RESOURCE);

        if (res)
        {
            imageID = gMainParser->FindID(fname);

            if (SetResourceImage(CurControl, imageID))
            {
                if (CurControl == PATCH_PIC)
                {
                    UI_logbk.SetPatch(imageID);
                }
                else if (CurControl == PILOT_PIC)
                    UI_logbk.SetPicture(imageID);
            }
        }
    }
}


void ChangeImageCB(long ID, short hittype, C_Base *control)
{
    if (LogState bitand LB_OPPONENT or not (LogState bitand LB_EDITABLE))
        return;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    CurControl = ID;

    if (CurControl == PATCH_PIC)
    {
        MakeVirtualListFromRsc(PATCHES_RESOURCE, 0);
        LoadVirtualFile(TXT_LOAD_PATCH, "patches\\*.tga", NULL, LoadTGACB, CloseWindowCB, VirtualFileList, LoadVirtualTGACB);
        //LoadAFile("patches\\*.tga",NULL,LoadTGACB,CloseWindowCB);
    }
    else
    {
        MakeVirtualListFromRsc(PILOTS_RESOURCE, 0);
        LoadVirtualFile(TXT_LOAD_PICTURE, "pictures\\*.tga", NULL, LoadTGACB, CloseWindowCB, VirtualFileList, LoadVirtualTGACB);
        //LoadAFile("pictures\\*.tga",NULL,LoadTGACB,CloseWindowCB);
    }

    control->Refresh();
}

int SetResourceImage(long ID, long ImageID)
{
    C_Window *win;
    win = gMainHandler->FindWindow(LOG_WIN);

    if (win not_eq NULL)
    {
        C_Button *button = (C_Button *)win->FindControl(ID);

        if (button)
        {
            button->Refresh();

            button->SetImage(C_STATE_0, ImageID);
            button->SetImage(C_STATE_1, ImageID);
            button->SetImage(C_STATE_SELECTED, ImageID);
            button->SetImage(C_STATE_DISABLED, ImageID);
            button->Refresh();
        }

        if (ID == PILOT_PIC)
            CurPic = 0;
        else if (ID == PATCH_PIC)
            CurPatch = 0;

        return TRUE;
    }

    return FALSE;
}

//sets image for button with ID == ID
int SetImage(long ID, _TCHAR *filename , long ImageID)
{
    C_Button *button;
    C_Window *win;
    C_Resmgr *resmgr = NULL;
    IMAGE_RSC *Image = NULL;
    // IMAGE_RSC *PrevImage=NULL;
    F4CSECTIONHANDLE *Leave;

    win = gMainHandler->FindWindow(LOG_WIN);

    if (win not_eq NULL)
    {
        long OldImageID = ImageID;
        Leave = UI_Enter(win);

        switch (ImageID)
        {
            case LOGBOOK_PICTURE_ID:
                ImageID = LOGBOOK_PICTURE_ID_2;
                break;

            case LOGBOOK_PICTURE_ID_2:
                ImageID = LOGBOOK_PICTURE_ID;
                break;

            case LOGBOOK_SQUADRON_ID:
                ImageID = LOGBOOK_SQUADRON_ID_2;
                break;

            case LOGBOOK_SQUADRON_ID_2:
                ImageID = LOGBOOK_SQUADRON_ID;
                break;

            default:
                if (ID == PATCH_PIC)
                    ImageID = LOGBOOK_PICTURE_ID;
                else if (ID == PILOT_PIC)
                    ImageID = LOGBOOK_SQUADRON_ID;

                break;
        };

        resmgr = gImageMgr->LoadImage(ImageID, filename, -1, -1);

        Image = gImageMgr->GetImage(ImageID);

        if ( not Image)
        {
            MonoPrint("FUNCTION: void SetImage()  -> Failed to load PHOTO file: %s \n", filename);
            UI_Leave(Leave);
            return FALSE;
        }

        if (CurControl == PATCH_PIC)
        {
            if (Image->Header->w > 96 or Image->Header->h > 96)
            {
                MonoPrint("Patch %s is too large\n", filename);
                UI_Leave(Leave);
                return FALSE;
            }

            if (Image->Header->w % 2)
            {
                MonoPrint("Patch %s is an odd width\n", filename);
                //return FALSE;
            }
        }
        else
        {
            if (Image->Header->w > 96 or Image->Header->h > 96)
            {
                MonoPrint("Picture %s is too large\n", filename);
                UI_Leave(Leave);
                return FALSE;
            }

            if (Image->Header->w % 2)
            {
                MonoPrint("Picture %s is an odd width\n", filename);
                //return FALSE;
            }
        }


        button = (C_Button *)win->FindControl(ID);

        if (button)
        {
            button->Refresh();
            /*
            int CenX,CenY,NewX,NewY;

            if(ID == PATCH_PIC)
            {
             CenX = 508;
             CenY = 322;
            }
            else if(ID == PILOT_PIC)
            {
             CenX = PicArea.left + (PicArea.right - PicArea.left)/2;
             CenY = PicArea.top + (PicArea.bottom - PicArea.top)/2;
            }


            NewX = CenX - (Image->Header->w/2);
            NewY = CenY - (Image->Header->h/2);

            button->SetXY(NewX,NewY);*/

            button->SetImage(C_STATE_0, ImageID);
            button->SetImage(C_STATE_1, ImageID);
            button->SetImage(C_STATE_SELECTED, ImageID);
            button->SetImage(C_STATE_DISABLED, ImageID);
            button->Refresh();
        }

        if (OldImageID)
            gImageMgr->RemoveImage(OldImageID);

        if (ID == PILOT_PIC)
            CurPic = ImageID;
        else if (ID == PATCH_PIC)
            CurPatch = ImageID;

        UI_Leave(Leave);
    }

    return TRUE;
}
/*
void LoadLogCB(long ID,short hittype,C_Base *control)
{
 C_EditBox * ebox;
 _TCHAR fname[MAX_PATH];

 if(hittype not_eq C_TYPE_LMOUSEUP)
 return;

 gMainHandler->HideWindow(control->Parent_);

 ebox=(C_EditBox*)control->Parent_->FindControl(FILE_NAME);
 if(ebox)
 {
 _tcscpy(fname,ebox->GetText());
 for(int i=0;i<_tcslen(fname);i++)
 if(fname[i] == '.')
 fname[i]=0;

 if(fname[0] == 0)
 return;
 UI_logbk.LoadData(fname);
 LogState and_eq compl LB_CHECKED;
 if( not UI_logbk.CheckPassword(_T("")))
 PasswordWindow(TXT_LOG_IN,TXT_LOG_IN_MESSAGE,CheckPasswordCB,NoPasswordCB);
 LBSetupControls();
 PlayerOptions.LoadOptions(UI_logbk.OptionsFile());
 UpdateKeyMapList(PlayerOptions.keyfile,1);
 }
}
*/
void PasswordChangeVerifiedCB(long, short hittype, C_Base *control)
{
    C_EditBox * ebox;
    C_EditBox * pwdbox;
    C_Window *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    win = gMainHandler->FindWindow(LOG_WIN);

    if (win == NULL)
        return;

    ebox = (C_EditBox*)control->Parent_->FindControl(PASSWORD);

    if (ebox)
    {
        pwdbox = (C_EditBox *)win->FindControl(PASSWORD_LIST);

        if (pwdbox not_eq NULL)
        {
            if (_tcscmp(ebox->GetText(), pwdbox->GetText()))
                return;

            UI_logbk.SetPassword(ebox->GetText());
            pwdbox->SetText(ebox->GetText());
            pwdbox->Refresh();
            ebox->SetText(_T(""));
            ebox->Refresh();
            gMainHandler->HideWindow(control->Parent_);
            control = win->FindControl(LOG_OK);
            SaveLogBookCB(LOG_OK, hittype, control);
            LogState or_eq LB_CHECKED;
        }
    }

}

void PwdVerifiedContLoading(long, short hittype, C_Base *control)
{
    C_EditBox * ebox;
    C_EditBox * pwdbox;
    C_Window *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    win = gMainHandler->FindWindow(LOG_WIN);

    if (win == NULL)
        return;

    ebox = (C_EditBox*)control->Parent_->FindControl(PASSWORD);

    if (ebox)
    {
        pwdbox = (C_EditBox *)win->FindControl(PASSWORD_LIST);

        if (pwdbox not_eq NULL)
        {
            if (_tcscmp(ebox->GetText(), pwdbox->GetText()))
                return;

            //password verified, save new value and continue opening
            UI_logbk.SetPassword(ebox->GetText());
            ebox->SetText(_T(""));
            ebox->Refresh();
            gMainHandler->HideWindow(control->Parent_);
            LogState or_eq LB_CHECKED;


            UI_logbk.SaveData();
            C_ListBox *lbox;
            lbox = (C_ListBox *)win->FindControl(LOGBOOK_LIST);

            if (lbox)
            {
                //if current log is selected do nothing
                if ( not _tcscmp(lbox->GetText(), UI_logbk.Callsign()))
                    return;

                if (UI_logbk.LoadData(lbox->GetText()))
                {
                    LogState and_eq compl LB_CHECKED;

                    if ( not UI_logbk.CheckPassword(_T("")))
                        PasswordWindow(TXT_LOG_IN, TXT_LOG_IN_MESSAGE, CheckPasswordCB, NoPasswordCB);

                    LBSetupControls();
                }
            }
        }
    }

}


void LoadPilotCB(long, short hittype, C_Base *control)
{
    C_EditBox *ebox;

    if (hittype == C_TYPE_SELECT)
    {
        //if password was changed, confirm it (do it here after list window is closed
        //so we don't have two exclusive windows at once)
        ebox = (C_EditBox *)control->Parent_->FindControl(PASSWORD_LIST);

        if (ebox not_eq NULL)
        {
            if ( not UI_logbk.CheckPassword(ebox->GetText()))
            {
                LogState and_eq compl LB_CHECKED;
                PasswordWindow(TXT_VERIFY_PASSWORD, TXT_VERIFY_PASS_MESSAGE, PwdVerifiedContLoading, CloseWindowCB);
                return;
            }
        }

        _TCHAR Pilot[MAX_PATH];
        _tcscpy(Pilot, ((C_ListBox *)control)->GetText());

        //return if current logbook is selected
        if ( not _tcscmp(Pilot, UI_logbk.Callsign()))
        {
            if (LogState bitand LB_INVALID_CALLSIGN)
            {
                AreYouSure(TXT_ERROR, TXT_INVALID_CALLSIGN, CloseWindowCB, NULL);
                LogState and_eq compl LB_INVALID_CALLSIGN;
                //return;
            }

            return;
        }

        ebox = (C_EditBox *)control->Parent_->FindControl(PASSWORD_LIST);

        if (ebox not_eq NULL)
        {
            SaveControlValues();
            UI_logbk.SaveData();

            //load data for selected pilot and check password
            if (UI_logbk.LoadData(Pilot))
            {
                LogState and_eq compl LB_CHECKED;

                //if password check is passed, allow access, otherwise
                //initialize UI_logbk and update controls
                if ( not UI_logbk.CheckPassword(_T("")))
                    PasswordWindow(TXT_LOG_IN, TXT_LOG_IN_MESSAGE, CheckPasswordCB, NoPasswordCB);
                else
                {
                    PlayerOptions.LoadOptions(UI_logbk.OptionsFile());
                    UpdateKeyMapList(PlayerOptions.keyfile, 1);
                }

                LBSetupControls();


                C_Window *stpwin;
                stpwin = gMainHandler->FindWindow(SETUP_WIN);

                if (stpwin)
                {
                    C_Button *button;
                    button = (C_Button *)stpwin->FindControl(SET_LOGBOOK);

                    if (button)
                    {
                        button->SetText(0, UI_logbk.Callsign());
                        button->Refresh();
                    }
                }

                //notify user the callsign entered was invalid
                if (LogState bitand LB_INVALID_CALLSIGN)
                {
                    AreYouSure(TXT_ERROR, TXT_INVALID_CALLSIGN, CloseWindowCB, NULL);
                    LogState and_eq compl LB_INVALID_CALLSIGN;
                    //return;
                }
            }
        }
    }
    else if (hittype == C_TYPE_LMOUSEUP)
    {


        //if callsign is invalid note it for later use
        ebox = (C_EditBox *)control->Parent_->FindControl(CALLSIGN_LIST);

        if (ebox not_eq NULL)
        {
            if ( not CheckCallsign(ebox->GetText()))
            {
                ebox->SetText(UI_logbk.Callsign());
                LogState or_eq LB_INVALID_CALLSIGN;
                ebox->Refresh();
            }

            C_EditBox *pbox;
            //if password was changed, confirm it in select event CB
            pbox = (C_EditBox *)control->Parent_->FindControl(PASSWORD_LIST);

            if (pbox not_eq NULL)
            {
                if ( not UI_logbk.CheckPassword(pbox->GetText()))
                {
                    return;
                }
            }

            //was the callsign changed? if so rename file, save change, and update list
            if (_tcsicmp(UI_logbk.Callsign(), ebox->GetText()))
            {
                _TCHAR orig[MAX_PATH];
                _TCHAR newfile[MAX_PATH];

                //rename file
                _stprintf(orig, _T("%s\\config\\%s.lbk"), FalconDataDirectory, UI_logbk.Callsign());
                _stprintf(newfile, _T("%s\\config\\%s.lbk"), FalconDataDirectory, ebox->GetText());
                _trename(orig, newfile);

                _stprintf(orig, _T("%s\\config\\%s.plc"), FalconDataDirectory, UI_logbk.Callsign());
                _stprintf(newfile, _T("%s\\config\\%s.plc"), FalconDataDirectory, ebox->GetText());
                _trename(orig, newfile);
                //store change in memory
                UI_logbk.SetCallsign(ebox->GetText());
            }

            //save changes to file
            UI_logbk.SaveData();

            //build new list and select correct pilot
            _TCHAR buf[MAX_PATH];
            _stprintf(buf, _T("%s\\config\\*.lbk"), FalconDataDirectory);
            GetPilotList(control->Parent_, buf, NULL, (C_ListBox *)control, TRUE, TRUE);
            SetPilot(UI_logbk.Callsign(), (C_ListBox *)control);
        }
    }
}

/*
void LoadLogBookCB(long ID,short hittype,C_Base *control)
{
 if(hittype not_eq C_TYPE_LMOUSEUP)
 return;

 LoadAFile("config\\*.lbk",NULL,LoadLogCB,CloseWindowCB);
}
*/

void OpenLogBookCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    SetCursor(gCursors[CRSR_WAIT]);

    LoadLogBookWindows();

    gMainHandler->EnableWindowGroup(control->GetGroup());
    SetCursor(gCursors[CRSR_F16]);

    if ( not LogBook.CheckPassword(_T("")))
        PasswordWindow(TXT_LOG_IN, TXT_LOG_IN_MESSAGE, CheckPasswordCB, NoPasswordCB);
}


void NewLogbookCB(long, short hittype, C_Base *)
{
    if (LogState bitand LB_OPPONENT)
        return;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    LogState or_eq LB_EDITABLE bitor LB_CHECKED;

    UI_logbk.Initialize();
    LBSetupControls();
}

void ClearCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    UI_logbk.Clear();
    gMainHandler->HideWindow(control->Parent_);
    LBSetupControls();
}

void ClearLogBookCB(long, short hittype, C_Base *)
{
    if (LogState bitand LB_OPPONENT)
        return;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    AreYouSure(TXT_WARNING, TXT_CLEAR_LOGBOOK, ClearCB, CloseWindowCB);
}

int CheckCallsign(_TCHAR *filename)
{
    if ( not _tcslen(filename))
        return FALSE;


    unsigned long pos = _tcscspn(filename, _T("\\/:?\"<>|"));

    if (pos < _tcslen(filename))
        return FALSE;


    _TCHAR *ptr = filename;

    int numchars = 0;

    for (; *ptr; ptr++)
    {
        if (_istalnum(*ptr))
            numchars++;
        else if (*ptr not_eq ' ')
            return FALSE;
    }

    if (numchars < 3)
        return FALSE;


    return TRUE;
}

int SaveControlValues(void)
{
    C_Window *win;
    C_EditBox *ebox;
    C_Text *text;
    C_ListBox *lbox;

    win = gMainHandler->FindWindow(LOG_WIN);

    if (win == NULL)
        return FALSE;



    ebox = (C_EditBox *)win->FindControl(CALLSIGN_LIST);

    if (ebox not_eq NULL)
    {
        if ( not CheckCallsign(ebox->GetText()))
        {
            AreYouSure(TXT_ERROR, TXT_INVALID_CALLSIGN, CloseWindowCB, CloseWindowCB);
            return FALSE;
        }

        _TCHAR *callsign;
        _TCHAR *ptr = callsign = ebox->GetText();

        ptr = callsign;

        while (*ptr == ' ')
            ptr++;

        callsign = ptr;
        ptr += (_tcslen(callsign) - 1);

        while (*ptr == ' ')
            ptr--;

        ptr++;
        *ptr = 0;

        if (_tcsicmp(UI_logbk.Callsign(), callsign))
        {
            _TCHAR orig[MAX_PATH];
            _TCHAR newfile[MAX_PATH];

            _stprintf(orig, _T("%s\\config\\%s.lbk"), FalconDataDirectory, UI_logbk.Callsign());
            _stprintf(newfile, _T("%s\\config\\%s.lbk"), FalconDataDirectory, callsign);
            _trename(orig, newfile);

            _stprintf(orig, _T("%s\\config\\%s.pop"), FalconDataDirectory, UI_logbk.OptionsFile());
            _stprintf(newfile, _T("%s\\config\\%s.pop"), FalconDataDirectory, callsign);
            _trename(orig, newfile);

            UI_logbk.SetOptionsFile(UI_logbk.Callsign());
            _stprintf(orig, _T("%s\\config\\%s.rul"), FalconDataDirectory, UI_logbk.Callsign());
            _stprintf(newfile, _T("%s\\config\\%s.rul"), FalconDataDirectory, callsign);
            _trename(orig, newfile);
        }

        UI_logbk.SetCallsign(callsign);

        ebox->Refresh();
        ebox->SetText(callsign);
        ebox->Refresh();

        C_Window *stpwin;
        stpwin = gMainHandler->FindWindow(SETUP_WIN);

        if (stpwin)
        {
            C_Button *button;
            button = (C_Button *)stpwin->FindControl(SET_LOGBOOK);

            if (button)
            {
                button->SetText(0, ebox->GetText());
                button->Refresh();
            }
        }
    }

    ebox = (C_EditBox *)win->FindControl(PILOT_LIST);

    if (ebox not_eq NULL)
    {
        UI_logbk.SetName(ebox->GetText());
    }

    ebox = (C_EditBox *)win->FindControl(PASSWORD_LIST);

    if (ebox not_eq NULL)
    {
        UI_logbk.SetPassword(ebox->GetText());
        ebox->SetText(_T(""));
        ebox->Refresh();
    }

    lbox = (C_ListBox *)win->FindControl(VOICE_CHOICE);

    if (lbox)
    {
        UI_logbk.SetVoice(static_cast<short>(lbox->GetTextID() - 1));
    }

    ebox = (C_EditBox *)win->FindControl(PERSONAL_TEXT);

    if (ebox)
    {
        UI_logbk.SetPersonal(ebox->GetText());
    }

    ebox = (C_EditBox *)win->FindControl(SQUADRON_NAME);

    if (ebox)
    {
        UI_logbk.SetSquadron(ebox->GetText());
    }

    text = (C_Text *)win->FindControl(COMMISSIONED_FIELD);

    if (text)
    {
        UI_logbk.SetCommissioned(text->GetText());
    }

    return TRUE;
}

void SaveLogBookCB(long ID, short hittype, C_Base *control)
{
    C_Window *win;
    C_EditBox *ebox;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (LogState bitand LB_OPPONENT or not (LogState bitand LB_EDITABLE))
    {
        CloseLogWindowCB(ID, hittype, control);
        return;
    }

    win = gMainHandler->FindWindow(LOG_WIN);

    if (win == NULL)
        return;


    ebox = (C_EditBox *)win->FindControl(PASSWORD_LIST);

    if (ebox not_eq NULL)
    {
        if ( not UI_logbk.CheckPassword(ebox->GetText()))
        {
            LogState and_eq compl LB_CHECKED;
            PasswordWindow(TXT_VERIFY_PASSWORD, TXT_VERIFY_PASS_MESSAGE, PasswordChangeVerifiedCB, CloseWindowCB);
            return;
        }
    }

    if ( not SaveControlValues())
        return;

    LogState or_eq LB_REFRESH_PILOT;
    UI_logbk.SaveData();
    LogBook.LoadData(&UI_logbk.Pilot);

    CloseLogWindowCB(ID, hittype, control);

}

void CloseLogWindowCB(long ID, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    PlayerOptions.LoadOptions(LogBook.OptionsFile());
    LogState and_eq (LB_CHECKED bitor LB_LOADED_ONCE);
    CloseWindowCB(ID, hittype, control);
}

void HookupLBControls(long ID)
{
    C_Window *winme;
    C_Button *ctrl;
    C_ListBox *lbox;

    winme = gMainHandler->FindWindow(ID);

    if (winme == NULL)
        return;

    // Hook up IDs here


    // Hook up Close Button
    ctrl = (C_Button *)winme->FindControl(CLOSE_WINDOW);

    if (ctrl not_eq NULL)
        ctrl->SetCallback(CloseWindowCB);

    ctrl = (C_Button *)winme->FindControl(LOG_NEW);

    if (ctrl not_eq NULL)
        ctrl->SetCallback(NewLogbookCB);

    ctrl = (C_Button *)winme->FindControl(LOG_CLEAR);

    if (ctrl not_eq NULL)
        ctrl->SetCallback(ClearLogBookCB);

    ctrl = (C_Button *)winme->FindControl(LOG_CANCEL);

    if (ctrl not_eq NULL)
        ctrl->SetCallback(CloseLogWindowCB);

    ctrl = (C_Button *)winme->FindControl(LOG_OK);

    if (ctrl not_eq NULL)
        ctrl->SetCallback(SaveLogBookCB);

    /*
     ctrl=(C_Button *)winme->FindControl(LOG_LOAD);
     if(ctrl not_eq NULL)
     ctrl->SetCallback(LoadLogBookCB);*/

    ctrl = (C_Button *)winme->FindControl(PATCH_PIC);

    if (ctrl not_eq NULL)
        ctrl->SetCallback(ChangeImageCB);

    ctrl = (C_Button *)winme->FindControl(PILOT_PIC);

    if (ctrl not_eq NULL)
        ctrl->SetCallback(ChangeImageCB);

    lbox = (C_ListBox *)winme->FindControl(LOGBOOK_LIST);

    if (lbox)
        lbox->SetCallback(LoadPilotCB);

    // Help GUIDE thing
    ctrl = (C_Button*)winme->FindControl(UI_HELP_GUIDE);

    if (ctrl)
        ctrl->SetCallback(UI_Help_Guide_CB);

}

void DisplayLogbook(LB_PILOT *Pilot, IMAGE_RSC *Photo, IMAGE_RSC *Patch, BOOL EditFlag)
{
    C_EditBox *ebox;
    C_Window *win;
    C_Button *button;
    C_Text *text;
    C_ListBox *lbox;
    _TCHAR buf[MAX_PATH];

    win = gMainHandler->FindWindow(LOG_WIN);

    if (win)
    {
        if (EditFlag)
        {
            button = (C_Button *)win->FindControl(LOG_NEW);

            if (button)
                button->SetFlagBitOff(C_BIT_INVISIBLE);

            button = (C_Button *)win->FindControl(LOG_CLEAR);

            if (button not_eq NULL)
                button->SetFlagBitOff(C_BIT_INVISIBLE);

            button = (C_Button *)win->FindControl(LOG_CANCEL);

            if (button not_eq NULL)
                button->SetFlagBitOff(C_BIT_INVISIBLE);

            button = (C_Button *)win->FindControl(LOG_OK);

            if (button not_eq NULL)
                button->SetFlagBitOff(C_BIT_INVISIBLE);

            button = (C_Button *)win->FindControl(CLOSE_WINDOW);

            if (button not_eq NULL)
                button->SetFlagBitOn(C_BIT_INVISIBLE);
        }
        else
        {
            button = (C_Button *)win->FindControl(LOG_NEW);

            if (button)
                button->SetFlagBitOn(C_BIT_INVISIBLE);

            button = (C_Button *)win->FindControl(LOG_CLEAR);

            if (button not_eq NULL)
                button->SetFlagBitOn(C_BIT_INVISIBLE);

            button = (C_Button *)win->FindControl(LOG_CANCEL);

            if (button not_eq NULL)
                button->SetFlagBitOn(C_BIT_INVISIBLE);

            button = (C_Button *)win->FindControl(LOG_OK);

            if (button not_eq NULL)
                button->SetFlagBitOn(C_BIT_INVISIBLE);

            button = (C_Button *)win->FindControl(CLOSE_WINDOW);

            if (button not_eq NULL)
                button->SetFlagBitOff(C_BIT_INVISIBLE);
        }

        button = (C_Button *)win->FindControl(PATCH_PIC);

        if (button)
        {
            if (EditFlag)
                button->SetFlagBitOn(C_BIT_ENABLED);
            else
                button->SetFlagBitOff(C_BIT_ENABLED);

            button->SetImage(C_STATE_0, Patch);
            button->SetImage(C_STATE_1, Patch);
            button->SetImage(C_STATE_DISABLED, Patch);
        }

        button = (C_Button *)win->FindControl(PILOT_PIC);

        if (button)
        {
            if (EditFlag)
                button->SetFlagBitOn(C_BIT_ENABLED);
            else
                button->SetFlagBitOff(C_BIT_ENABLED);

            button->SetImage(C_STATE_0, Photo);
            button->SetImage(C_STATE_1, Photo);
            button->SetImage(C_STATE_DISABLED, Photo);
        }


        ebox = (C_EditBox *)win->FindControl(PASSWORD_LIST);

        if (ebox)
        {
            if (EditFlag)
                ebox->SetFlagBitOn(C_BIT_ENABLED);
            else
                ebox->SetFlagBitOff(C_BIT_ENABLED);

            ebox->SetText(Pilot->Password);
        }

        ebox = (C_EditBox *)win->FindControl(PILOT_LIST);

        if (ebox)
        {
            if (EditFlag)
                ebox->SetFlagBitOn(C_BIT_ENABLED);
            else
                ebox->SetFlagBitOff(C_BIT_ENABLED);

            ebox->SetText(Pilot->Name);
        }

        ebox = (C_EditBox *)win->FindControl(CALLSIGN_LIST);

        if (ebox)
        {
            if (EditFlag)
                ebox->SetFlagBitOn(C_BIT_ENABLED);
            else
                ebox->SetFlagBitOff(C_BIT_ENABLED);

            ebox->SetText(Pilot->Callsign);
        }

        lbox = (C_ListBox *)win->FindControl(LOGBOOK_LIST);

        if (lbox)
        {
            if (EditFlag)
            {
                _stprintf(buf, _T("%s\\config\\*.lbk"), FalconDataDirectory);
                GetPilotList(win, buf, NULL, lbox, TRUE, TRUE);

                ebox = (C_EditBox *)win->FindControl(CALLSIGN_LIST); // just for good measure

                if (ebox)
                    SetPilot(ebox->GetText(), lbox);

                lbox->SetCallback(ChoosePilotCB);
                lbox->SetFlagBitOn(C_BIT_ENABLED);
            }
            else
                lbox->SetFlagBitOff(C_BIT_ENABLED);
        }

        lbox = (C_ListBox *)win->FindControl(VOICE_CHOICE);

        if (lbox)
        {
            if (EditFlag)
                lbox->SetFlagBitOn(C_BIT_ENABLED);
            else
                lbox->SetFlagBitOff(C_BIT_ENABLED);

            lbox->SetValue(Pilot->voice + 1);
        }

        ebox = (C_EditBox *)win->FindControl(PERSONAL_TEXT);

        if (ebox)
        {
            if (EditFlag)
                ebox->SetFlagBitOn(C_BIT_ENABLED);
            else
                ebox->SetFlagBitOff(C_BIT_ENABLED);

            ebox->SetText(Pilot->Personal);
        }

        ebox = (C_EditBox *)win->FindControl(SQUADRON_NAME);

        if (ebox)
        {
            if (EditFlag)
                ebox->SetFlagBitOn(C_BIT_ENABLED);
            else
                ebox->SetFlagBitOff(C_BIT_ENABLED);

            ebox->SetText(Pilot->Squadron);
        }

        button = (C_Button *)win->FindControl(50095);//RANKS value was changed

        if (button)
        {
            button->SetState(static_cast<short>(Pilot->Rank));
            button->SetHelpText(gFullRanksTxt[Pilot->Rank]);
        }

        text = (C_Text *)win->FindControl(COMMISSIONED_FIELD);

        if (text)
        {
            text->SetText(Pilot->Commissioned);
        }

        text = (C_Text *)win->FindControl(HOURS_FIELD);

        if (text)
        {
            _stprintf(buf, "%4.1f", Pilot->FlightHours);
            Uni_Float(buf);
            text->SetText(buf);
        }

        text = (C_Text *)win->FindControl(ACE_FIELD);

        if (text)
        {
            _stprintf(buf, "%1.3f", Pilot->AceFactor);
            Uni_Float(buf);
            text->SetText(buf);
        }

        text = (C_Text *)win->FindControl(FRAGRATIO_FIELD);

        if (text)
        {
            int kills = Pilot->Campaign.Kills;
            int killed = Pilot->Campaign.Killed;
            _stprintf(buf, "%2d/%2d", kills, killed);
            text->SetText(buf);
        }

        DF_STATS *dgft = &Pilot->Dogfight;

        text = (C_Text *)win->FindControl(DF_MATCH_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%3d"), dgft->MatchesWon + dgft->MatchesLost);
            text->SetText(buf);
        }

        text = (C_Text *)win->FindControl(DF_WON_LOST_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%2d/%2d"), dgft->MatchesWon, dgft->MatchesLost);
            text->SetText(buf);
        }

        //DF_MATCH_VSHUMANS_FIELD
        text = (C_Text *)win->FindControl(DF_MATCH_VSHUMANS_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%2d/%2d"), dgft->MatchesWonVHum, dgft->MatchesLostVHum);
            text->SetText(buf);
        }

        //DF_KILL_FIELD
        text = (C_Text *)win->FindControl(DF_KILL_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%2d/%2d"), dgft->Kills, dgft->Killed);
            text->SetText(buf);
        }

        //DF_H2H_FIELD
        //DF_VS_FIELD
        text = (C_Text *)win->FindControl(DF_VS_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%2d/%2d"), dgft->HumanKills, dgft->KilledByHuman);
            text->SetText(buf);
        }

        CAMP_STATS *camp = &Pilot->Campaign;

        //CAMP_CAMPAIGNS_FIELD
        text = (C_Text *)win->FindControl(CAMP_CAMPAIGNS_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%d/%d/%d"), camp->GamesWon, camp->GamesLost, camp->GamesTied);
            text->SetText(buf);
        }

        //CAMP_MISS_FIELD
        text = (C_Text *)win->FindControl(CAMP_MISS_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%4d"), camp->Missions);
            text->SetText(buf);
        }

        //CAMP_RATING_FIELD
        text = (C_Text *)win->FindControl(CAMP_RATING_FIELD);

        if (text)
        {
            if (camp->Missions)
                _stprintf(buf, _T("%2.3f"), (double)camp->TotalMissionScore / camp->Missions);
            else
                _stprintf(buf, "0");

            Uni_Float(buf);
            text->SetText(buf);
        }

        //CAMP_KILL_FIELD
        text = (C_Text *)win->FindControl(CAMP_KILL_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%2d/%2d"), camp->Kills, camp->Killed);
            text->SetText(buf);
        }

        //CAMP_VS_FIELD
        text = (C_Text *)win->FindControl(CAMP_VS_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%2d/%2d"), camp->HumanKills, camp->KilledByHuman);
            text->SetText(buf);
        }

        text = (C_Text *)win->FindControl(CAMP_AA_KILLS_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%4d"), camp->Kills);
            text->SetText(buf);
        }

        text = (C_Text *)win->FindControl(CAMP_AG_KILLS_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%4d"), camp->AirToGround);
            text->SetText(buf);
        }

        text = (C_Text *)win->FindControl(CAMP_STATIC_KILLS_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%4d"), camp->Static);
            text->SetText(buf);
        }

        text = (C_Text *)win->FindControl(CAMP_NAVAL_KILLS_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%4d"), camp->Naval);
            text->SetText(buf);
        }

        text = (C_Text *)win->FindControl(CAMP_DEATHS_FIELD);

        if (text)
        {
            _stprintf(buf, _T("%4d"), camp->Killed);
            text->SetText(buf);
        }

        //MEDALS_AFC
        //MEDALS_AM
        //MEDALS_DFC
        //MEDALS_SS
        //MEDALS_KC
        int i;
        //need to also do stars and oakleafs for multiple medals
        int CurrMedal = MEDALS_AFC;

        for (i = 0; i < NUM_MEDALS ; i++)
        {
            if (Pilot->Medals[i])
            {
                button = (C_Button *)win->FindControl(CurrMedal);

                if (button)
                {
                    AwardDevices(win, CurrMedal, static_cast<uchar>(i), Pilot->Medals[i]);
                    button->SetFlagBitOff(C_BIT_INVISIBLE);
                    button->SetState(static_cast<short>(i));
                    button->SetHelpText(gStringMgr->AddText(button->GetLabel(static_cast<short>(i))));
                    button->Refresh();
                    CurrMedal++;
                }
            }

        }

        for (i = CurrMedal; i < NUM_MEDALS + MEDALS_AFC ; i++)
        {
            button = (C_Button *)win->FindControl(i);

            if (button)
            {
                AwardDevices(win, i, 0, 0);
                button->SetFlagBitOn(C_BIT_INVISIBLE);
                button->Refresh();
            }
        }

        win->RefreshWindow();
    }
}
