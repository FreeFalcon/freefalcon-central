/***************************************************************************\
    ui_gen.cpp
    Peter Ward
 July 15, 1996

    This code contains a bunch of generic routines
\***************************************************************************/
#include <cISO646>
#include <windows.h>
#include "f4version.h"
#include <ddraw.h>
#include "unit.h"
#include "team.h"
#include "CmpGlobl.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "Listadt.h"
#include "objectiv.h"
#include "Campaign.h"
#include "campmap.h"
#include "campwp.h"
#include "campstr.h"
#include "squadron.h"
#include "feature.h"
#include "pilot.h"
#include "f4find.h"
#include "find.h"
#include "misseval.h"
#include "cmpclass.h"
#include "ui95_dd.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "AirUnit.h"
#include "uicomms.h"
#include "userids.h"
#include "textids.h"
#include "classtbl.h"
#include "brief.h"
#include "teamdata.h"
#include "division.h"
#include "cmap.h"
#include "gps.h"
#include "urefresh.h"
#include "credits.h"

#pragma warning(disable:4244) // for +=

extern C_Handler *gMainHandler;
extern C_Parser *gMainParser;
extern VU_ID gCurrentFlightID;
extern GlobalPositioningSystem *gGps;

extern char FalconPictureDirectory[_MAX_PATH]; // JB 010623
extern bool g_bHiResUI; // MN 020104
extern bool g_bMissionACIcons; // JB 020211
extern bool g_LargeTheater; // MN

// OW
//JAM 28Sep03
#define _DO_CREDITS_HACK

// ALL RESMGR CODE ADDITIONS START HERE
#define _USE_RES_MGR_ 1

#ifndef _USE_RES_MGR_ // DON'T USE RESMGR

#define UI_HANDLE FILE *
#define UI_OPEN   fopen
#define UI_READ   fread
#define UI_CLOSE  fclose
#define UI_SEEK   fseek
#define UI_TELL   ftell

#else // USE RESMGR

#include "cmpclass.h"
extern "C"
{
#include "codelib/resources/reslib/src/resmgr.h"
}

#define UI_HANDLE FILE *
#define UI_OPEN   RES_FOPEN
#define UI_READ   RES_FREAD
#define UI_CLOSE  RES_FCLOSE
#define UI_SEEK   RES_FSEEK
#define UI_TELL   RES_FTELL

#endif
// ALL RESMGR CODE ADDITIONS AND END HERE

enum
{
    BLUE_TEAM_ICONS = 565120000,
    BLUE_TEAM_ICONS_W = 565120001,
    CAMP_AIR_BASE_ICON          = 10003,
};

void DeleteGroupList(long ID);

#define COLLECTABLE_HP_OBJECTIVES 5
int GetTopPriorityObjectives(int team, _TCHAR* buffers[COLLECTABLE_HP_OBJECTIVES]);
int GetTeamSituation(Team t);
BOOL AddWordWrapTextToWindow(C_Window *win, short *x, short *y, short startcol, short endcol, COLORREF color, _TCHAR *str, long Client = 0);
void DelSTRFileCB(long ID, short hittype, C_Base *control);
void DelDFSFileCB(long ID, short hittype, C_Base *control);
void DelLSTFileCB(long ID, short hittype, C_Base *control);
void DelCamFileCB(long ID, short hittype, C_Base *control);
void DelTacFileCB(long ID, short hittype, C_Base *control);
void DelTGAFileCB(long ID, short hittype, C_Base *control);
void DelVHSFileCB(long ID, short hittype, C_Base *control);
void DelKeyFileCB(long ID, short hittype, C_Base *control);

_TCHAR LoadSaveFilename[MAX_PATH + 1];
void LoadAFile(long TitleID, _TCHAR *filespec, _TCHAR *excludelist[], void (*YesCB)(long, short, C_Base*), void (*NoCB)(long, short, C_Base*));
void SaveAFile(long TitleID, _TCHAR *filespec, _TCHAR *excludelist[], void (*YesCB)(long, short, C_Base*), void (*NoCB)(long, short, C_Base*), _TCHAR *filename);
void LoadVirtualFile(long TitleID, _TCHAR *filespec, _TCHAR *excludelist[], void (*YesCB)(long, short, C_Base*), void (*NoCB)(long, short, C_Base*), _TCHAR *virtuallist[], void (*VirtualCB)(long, short, C_Base*));
void EncryptBuffer(uchar startkey, uchar *buffer, long length);
extern IMAGE_RSC *gOccupationMap;

_TCHAR VirtualFileList[200][64];

void CloseWindowCB(long ID, short hittype, C_Base *control);
extern bool g_bEmptyFilenameFix;

static long Ordinals[] =
{
    TXT_ORD_0,
    TXT_ORD_1,
    TXT_ORD_2,
    TXT_ORD_3,
    TXT_ORD_4,
    TXT_ORD_5,
    TXT_ORD_6,
    TXT_ORD_7,
    TXT_ORD_8,
    TXT_ORD_9,
};

char *DontDeleteList[] =
{
    "keystrokes.key",
    "laptop.key",
    NULL,
};

_TCHAR *OrdinalString(long value)
{
    long ordinal;
    static _TCHAR buffer[20];

    if (gLangIDNum == F4LANG_FRENCH)
    {
        if (value == 1)
        {
            _stprintf(buffer, "%1ld%s", value, gStringMgr->GetString(Ordinals[1]));
        }
        else
        {
            _stprintf(buffer, "%1ld%s", value, gStringMgr->GetString(Ordinals[2]));
        }
    }
    else
    {
        ordinal = value % 10;

        if (ordinal < 0) ordinal = -ordinal;

        _stprintf(buffer, "%1ld%s", value, gStringMgr->GetString(Ordinals[ordinal]));
    }

    return(&buffer[0]);
}

void Uni_Float(_TCHAR *buffer)
{
    short i;
    _TCHAR *decimal;

    decimal = gStringMgr->GetString(TXT_DECIMAL_PLACE);

    if ( not decimal or not buffer)
        return;

    i = 0;

    while (buffer[i] and i < 20)
    {
        if (buffer[i] == '.')
            buffer[i] = decimal[0];

        i++;
    }
}

// This function work JUST LIKE strtok :)
static _TCHAR *WordWrap = NULL;
static _TCHAR *WordPtr = NULL;
static _TCHAR *NextPtr = NULL;
_TCHAR *UI_WordWrap(C_Window *win, _TCHAR *str, long fontid, short width, BOOL *status)
{
    _TCHAR *space;
    short i, done;
    short w;

    if ( not win or (str == NULL and (WordWrap == NULL or WordPtr == NULL)))
        return(NULL);

    if (str)
    {
        if (WordWrap)
            delete WordWrap;

        WordWrap = new _TCHAR[_tcslen(str) + 1];
        _tcscpy(WordWrap, str);
        WordPtr = WordWrap;
        NextPtr = NULL;
    }
    else
    {
        if (NextPtr == NULL)
        {
            delete WordWrap;
            WordWrap = NULL;
            WordPtr = NULL;
            NextPtr = NULL;
            return(NULL);
        }

        WordPtr = NextPtr;

        if (WordPtr)
        {
            if ( not _tcslen(WordPtr))
            {
                delete WordWrap;
                WordWrap = NULL;
                WordPtr = NULL;
                NextPtr = NULL;
                return(NULL);
            }
        }
    }

    i = 0;
    done = 0;
    space = NULL;

    while ( not done)
    {
        // find a space
        while (WordPtr[i] not_eq ' ' and (WordPtr[i] > 31))
            i++;

        if (i)
        {
            w = static_cast<short>(gFontList->StrWidth(fontid, WordPtr, i));

            if (w > width)
            {
                if (space == NULL)
                {
                    *status = FALSE;
                    NextPtr = &WordPtr[i];
                    done = 1;
                }
                else
                {
                    NextPtr = space + 1;
                    *space = 0;
                    *status = TRUE;
                    done = 1;
                }
            }
            else
            {
                if (WordPtr[i] == ' ')
                    space = &WordPtr[i++];
                else
                {
                    *status = TRUE;
                    NextPtr = NULL;
                    done = 1;
                }
            }
        }
        else
            i++;
    }

    return(WordPtr);
}

// Returns TRUE if I want to insert newitem before list item
BOOL FileNameSortCB(TREELIST *list, TREELIST *newitem)
{
    _TCHAR *first, *second;
    C_Button *btn1, *btn2;

    if ( not list or not newitem)
        return(FALSE);

    btn1 = (C_Button*)list->Item_;
    btn2 = (C_Button*)newitem->Item_;

    if ( not btn1 or not btn2)
        return(FALSE);

    first = btn1->GetText(0);
    second = btn2->GetText(0);

    if ( not first or not second)
        return(FALSE);

    if (_tcsicmp(second, first) < 0)
        return(TRUE);

    return(FALSE);
}

void GetVirtualFileList(C_Window *win, _TCHAR virtlist[200][64], long client, long group, long cluster, void (*cb)(long, short, C_Base*), long *startx, long *starty)
{
    C_Button *btn;
    C_EditBox *ebox;
    short i, half;

    if ( not virtlist or not win)
        return;

    half = static_cast<short>((win->ClientArea_[client].right - win->ClientArea_[client].left) / 2);

    if (half < 150)
        half = 0;

    i = 0;

    while (virtlist[i][0] and i < 200)
    {
        btn = new C_Button;
        btn->Setup(C_DONT_CARE, C_TYPE_RADIO, *startx, *starty);
        btn->SetFont(win->Font_);
        btn->SetText(C_STATE_0, virtlist[i]);
        btn->SetText(C_STATE_1, virtlist[i]);
        btn->SetText(C_STATE_DISABLED, virtlist[i]);
        btn->SetFgColor(C_STATE_0, 0xcccccc);
        btn->SetFgColor(C_STATE_1, 0x00ff00);
        btn->SetFgColor(C_STATE_DISABLED, 0x808080);
        btn->SetCallback(cb);
        btn->SetClient(static_cast<short>(client));
        btn->SetGroup(group);
        btn->SetCluster(cluster);
        btn->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
        btn->SetUserNumber(0, 1234); // Virtual
        win->AddControl(btn);

        if ( not (*starty) and not (*startx))
        {
            ebox = (C_EditBox*)win->FindControl(FILE_NAME);

            if (ebox)
                ebox->SetText(virtlist[i]);
        }

        if ( not (*startx) and half)
            (*startx) += half;
        else
        {
            (*startx) = 0;
            (*starty) += gFontList->GetHeight(win->Font_);
        }

        i++;
    }

    win->ScanClientArea(client);
}

void GetVirtualFileListTree(C_TreeList *tree, _TCHAR virtlist[200][64], long group)
{
    C_Button *btn;
    TREELIST *item;
    short i, UniqueID;

    if ( not virtlist or not tree)
        return;

    UniqueID = static_cast<short>(tree->GetUserNumber(0));

    if ( not UniqueID)
        UniqueID++;

    i = 0;

    while (virtlist[i][0] and i < 200)
    {
        btn = new C_Button;

        if (btn)
        {
            btn->Setup(UniqueID, C_TYPE_CUSTOM, 0, 0);
            btn->SetFont(tree->GetFont());
            btn->SetText(C_STATE_0, virtlist[i]);
            btn->SetText(C_STATE_1, virtlist[i]);
            btn->SetText(C_STATE_DISABLED, virtlist[i]);
            btn->SetFgColor(C_STATE_0, 0xcccccc);
            btn->SetFgColor(C_STATE_1, 0x00ff00);
            btn->SetFgColor(C_STATE_DISABLED, 0x808080);
            btn->SetUserNumber(0, 1234); // Virtual
            btn->SetCursorID(tree->GetCursorID());
            btn->SetDragCursorID(tree->GetDragCursorID());

            item = tree->CreateItem(UniqueID++, group, btn);

            if (item)
                tree->AddItem(tree->GetRoot(), item);
        }

        i++;
    }

    tree->SetUserNumber(0, UniqueID);
}

void GetFileList(C_Window *win, _TCHAR *fspec, _TCHAR *excludelist[], long client, long group, long cluster, void (*cb)(long, short, C_Base*), BOOL cutext, long *startx, long *starty)
{
    C_Button *btn;
    C_EditBox *ebox;
    WIN32_FIND_DATA filedata;
    HANDLE ffhnd;
    BOOL last, ignore;
    long i;
    _TCHAR *ptr, *extension;

    if ( not win or not cb) return;

    ffhnd = FindFirstFile(fspec, &filedata);
    last = (ffhnd not_eq INVALID_HANDLE_VALUE);

    while (last)
    {
        if (cutext)
        {
            ptr = filedata.cFileName;
            extension = NULL;

            while (*ptr)
            {
                if (*ptr == '.')
                {
                    extension = ptr;
                }

                ptr ++;
            }

            if (extension)
            {
                *extension = 0;
            }
        }

        ignore = FALSE;

        if (excludelist)
        {
            i = 0;

            while (excludelist[i] and not ignore)
            {
                if (stricmp(excludelist[i], filedata.cFileName) == 0)
                {
                    ignore = TRUE;
                }

                i++;
            }
        }

        if ( not ignore)
        {
            btn = new C_Button;
            btn->Setup(C_DONT_CARE, C_TYPE_RADIO, *startx, *starty);
            btn->SetFont(win->Font_);
            btn->SetText(C_STATE_0, filedata.cFileName);
            btn->SetText(C_STATE_1, filedata.cFileName);
            btn->SetText(C_STATE_DISABLED, filedata.cFileName);
            btn->SetFgColor(C_STATE_0, 0xcccccc);
            btn->SetFgColor(C_STATE_1, 0x00ff00);
            btn->SetFgColor(C_STATE_DISABLED, 0x808080);
            btn->SetCallback(cb);
            btn->SetClient(static_cast<short>(client));
            btn->SetGroup(group);
            btn->SetCluster(cluster);
            btn->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
            btn->SetUserNumber(0, 0); // Non virtual
            win->AddControl(btn);

            if ( not (*starty))
            {
                ebox = (C_EditBox*)win->FindControl(FILE_NAME);

                if (ebox)
                    ebox->SetText(btn->GetText(C_STATE_0));
            }

            (*starty) += gFontList->GetHeight(win->Font_);
        }

        last = FindNextFile(ffhnd, &filedata);
    }

    win->ScanClientArea(client);
}

void SetDeleteCallback(void (*cb)(long, short, C_Base*))
{
    C_PopupList *popup;

    popup = gPopupMgr->GetMenu(DELETE_FILE_POPUP);

    if (popup)
    {
        popup->SetCallback(LIST_DELETE, cb);
    }
}

void GetFileListTree(C_TreeList *tree, _TCHAR *fspec, _TCHAR *excludelist[], long group, BOOL cutext, long UseMenu)
{
    C_Button *btn;
    TREELIST *item;
    WIN32_FIND_DATA filedata;
    HANDLE ffhnd;
    BOOL last, ignore;
    long i, UniqueID;
    _TCHAR *ptr, *extension;

    if ( not tree) return;

    UniqueID = tree->GetUserNumber(0);

    if ( not UniqueID)
        UniqueID++;

    ffhnd = FindFirstFile(fspec, &filedata);
    last = (ffhnd not_eq INVALID_HANDLE_VALUE);

    while (last)
    {
        if (cutext)
        {
            ptr = filedata.cFileName;
            extension = NULL;

            while (*ptr)
            {
                if (*ptr == '.')
                {
                    extension = ptr;
                }

                ptr ++;
            }

            if (extension)
            {
                *extension = 0;
            }
        }

        ignore = FALSE;

        if (excludelist)
        {
            i = 0;

            while (excludelist[i] and not ignore)
            {
                if (stricmp(excludelist[i], filedata.cFileName) == 0)
                {
                    ignore = TRUE;
                }

                i++;
            }
        }

        if ( not ignore)
        {
            btn = new C_Button;

            if (btn)
            {
                btn->Setup(UniqueID, C_TYPE_CUSTOM, 0, 0);
                btn->SetFont(tree->GetFont());
                btn->SetText(C_STATE_0, filedata.cFileName);
                btn->SetText(C_STATE_1, filedata.cFileName);
                btn->SetText(C_STATE_DISABLED, filedata.cFileName);
                btn->SetFgColor(C_STATE_0, 0xcccccc);
                btn->SetFgColor(C_STATE_1, 0x00ff00);
                btn->SetFgColor(C_STATE_DISABLED, 0x808080);
                btn->SetUserNumber(0, 0);
                btn->SetCursorID(tree->GetCursorID());
                btn->SetDragCursorID(tree->GetDragCursorID());
                btn->SetMenu(UseMenu);

                item = tree->CreateItem(UniqueID++, group, btn);

                if (item)
                    tree->AddItem(tree->GetRoot(), item);
            }
        }

        last = FindNextFile(ffhnd, &filedata);
    }

    if (ffhnd not_eq INVALID_HANDLE_VALUE)
        FindClose(ffhnd);

    tree->SetUserNumber(0, UniqueID);
}


static void LoadSaveSelectFileCB(long, short hittype, C_Base *control)
{
    C_TreeList *tree;
    C_EditBox *ebox;
    C_Button *btn;
    C_Window *win;
    TREELIST *item;

    if (hittype == C_TYPE_LMOUSEUP)
    {
        if (control)
        {
            tree = (C_TreeList*)control;
            item = tree->GetLastItem();

            if (item)
            {
                tree->SetAllControlStates(0, tree->GetRoot());
                btn = (C_Button*)item->Item_;

                if (btn)
                {
                    btn->SetState(1);
                    btn->Refresh();
                    ebox = (C_EditBox*)btn->Parent_->FindControl(FILE_NAME);

                    if (ebox)
                    {
                        ebox->Refresh();
                        ebox->SetText(btn->GetText(C_STATE_0));
                        ebox->Refresh();
                    }
                }
            }

            tree->Refresh();
        }
    }
    else if (hittype == C_TYPE_LMOUSEDBLCLK)
    {
        if (control)
        {
            win = control->Parent_;

            if (win)
            {
                btn = (C_Button*)win->FindControl(LOAD);

                if (btn and not (btn->GetFlags() bitand C_BIT_INVISIBLE))
                {
                    if (btn->GetCallback())
                    {
                        btn->GetCallback()(btn->GetID(), C_TYPE_LMOUSEUP, btn);
                    }

                    return;
                }

                btn = (C_Button*)win->FindControl(LOAD_VIRTUAL);

                if (btn and not (btn->GetFlags() bitand C_BIT_INVISIBLE))
                {
                    if (btn->GetCallback())
                    {
                        btn->GetCallback()(btn->GetID(), C_TYPE_LMOUSEUP, btn);
                    }

                    return;
                }

                btn = (C_Button*)win->FindControl(SAVE);

                if (btn and not (btn->GetFlags() bitand C_BIT_INVISIBLE))
                {
                    if (btn->GetCallback())
                    {
                        btn->GetCallback()(btn->GetID(), C_TYPE_LMOUSEUP, btn);
                    }

                    return;
                }
            }
        }
    }
}

static void LoadVirtualSelectFileCB(long, short hittype, C_Base *control)
{
    C_TreeList *tree;
    C_EditBox *ebox;
    C_Button *btn;
    TREELIST *item;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (control)
    {
        tree = (C_TreeList*)control;
        item = tree->GetLastItem();

        if (item)
        {
            tree->SetAllControlStates(0, tree->GetRoot());
            btn = (C_Button*)item->Item_;

            if (btn)
            {
                btn->SetState(1);
                ebox = (C_EditBox*)btn->Parent_->FindControl(FILE_NAME);

                if (ebox)
                {
                    ebox->Refresh();
                    ebox->SetText(btn->GetText(C_STATE_0));
                    ebox->Refresh();

                    if (btn->GetUserNumber(0))
                    {
                        btn = (C_Button*)control->Parent_->FindControl(LOAD);

                        if (btn)
                            btn->SetFlagBitOn(C_BIT_INVISIBLE);

                        btn = (C_Button*)control->Parent_->FindControl(LOAD_VIRTUAL);

                        if (btn)
                        {
                            btn->SetFlagBitOff(C_BIT_INVISIBLE);
                            btn->Refresh();
                        }
                    }
                    else
                    {
                        btn = (C_Button*)control->Parent_->FindControl(LOAD_VIRTUAL);

                        if (btn)
                            btn->SetFlagBitOn(C_BIT_INVISIBLE);

                        btn = (C_Button*)control->Parent_->FindControl(LOAD);

                        if (btn)
                        {
                            btn->SetFlagBitOff(C_BIT_INVISIBLE);
                            btn->Refresh();
                        }
                    }
                }
            }
        }

        tree->Refresh();
    }
}

void LoadAFile(long TitleID, _TCHAR *filespec, _TCHAR *excludelist[], void (*YesCB)(long, short, C_Base*), void (*NoCB)(long, short, C_Base*))
{
    C_Window *win;
    C_Button *btn;
    C_Text *txt;
    C_TreeList *tree;
    C_EditBox *ebox;

    if ( not YesCB or not filespec)
        return;

    win = gMainHandler->FindWindow(SAVE_WIN);

    if (win)
    {
        txt = (C_Text*)win->FindControl(TITLE_LABEL);

        if (txt)
        {
            txt->Refresh();
            txt->SetText(TitleID);
            txt->Refresh();
        }

        btn = (C_Button *)win->FindControl(SAVE);

        if (btn)
        {
            btn->SetFlagBitOn(C_BIT_INVISIBLE);
            btn->SetCallback(NULL);
        }

        btn = (C_Button *)win->FindControl(LOAD_VIRTUAL);

        if (btn)
        {
            btn->SetFlagBitOn(C_BIT_INVISIBLE);
            btn->SetCallback(YesCB);
        }

        btn = (C_Button *)win->FindControl(LOAD);

        if (btn)
        {
            btn->SetFlagBitOff(C_BIT_INVISIBLE);
            btn->SetCallback(YesCB);
        }

        btn = (C_Button *)win->FindControl(CANCEL);

        if (btn)
        {
            btn->SetFlagBitOff(C_BIT_INVISIBLE);
            btn->SetCallback(NoCB);
        }

        btn = (C_Button *)win->FindControl(CLOSE_WINDOW);

        if (btn)
        {
            btn->SetFlagBitOff(C_BIT_INVISIBLE);
            btn->SetCallback(NoCB);
        }

        ebox = (C_EditBox*)win->FindControl(FILE_NAME);

        if (ebox)
        {
            ebox->Refresh();
            ebox->SetText("");
            ebox->SetFlagBitOff(C_BIT_ENABLED);
        }

        tree = (C_TreeList*)win->FindControl(FILELIST_TREE);

        if (tree)
        {
            tree->DeleteBranch(tree->GetRoot());
            tree->SetUserNumber(0, 1);
            tree->SetSortType(TREE_SORT_CALLBACK);
            tree->SetSortCallback(FileNameSortCB);
            tree->SetCallback(LoadSaveSelectFileCB);
            GetFileListTree(tree, filespec, excludelist, C_TYPE_ITEM, TRUE, DELETE_FILE_POPUP);
            tree->RecalcSize();

            if (tree->Parent_)
                tree->Parent_->RefreshClient(tree->GetClient());
        }

        gMainHandler->ShowWindow(win);
        gMainHandler->WindowToFront(win);
    }
}

void LoadVirtualFile(long TitleID, _TCHAR *filespec, _TCHAR *excludelist[], void (*YesCB)(long, short, C_Base*), void (*NoCB)(long, short, C_Base*), _TCHAR virtuallist[200][64], void (*VirtualCB)(long, short, C_Base*))
{
    C_Window *win;
    C_Button *btn;
    C_TreeList *tree;
    C_Text *txt;
    C_EditBox *ebox;

    if ( not YesCB or not filespec)
        return;

    win = gMainHandler->FindWindow(SAVE_WIN);

    if (win)
    {
        txt = (C_Text*)win->FindControl(TITLE_LABEL);

        if (txt)
        {
            txt->Refresh();
            txt->SetText(TitleID);
            txt->Refresh();
        }

        btn = (C_Button *)win->FindControl(SAVE);

        if (btn)
        {
            btn->SetFlagBitOn(C_BIT_INVISIBLE);
            btn->SetCallback(NULL);
        }

        btn = (C_Button *)win->FindControl(LOAD);

        if (btn)
        {
            btn->SetFlagBitOff(C_BIT_INVISIBLE);
            btn->SetCallback(YesCB);
        }

        btn = (C_Button *)win->FindControl(LOAD_VIRTUAL);

        if (btn)
        {
            btn->SetFlagBitOn(C_BIT_INVISIBLE);
            btn->SetCallback(VirtualCB);
        }

        btn = (C_Button *)win->FindControl(CANCEL);

        if (btn)
        {
            btn->SetFlagBitOff(C_BIT_INVISIBLE);
            btn->SetCallback(NoCB);
        }

        btn = (C_Button *)win->FindControl(CLOSE_WINDOW);

        if (btn)
        {
            btn->SetFlagBitOff(C_BIT_INVISIBLE);
            btn->SetCallback(NoCB);
        }

        ebox = (C_EditBox*)win->FindControl(FILE_NAME);

        if (ebox)
        {
            ebox->Refresh();
            ebox->SetText("");
            ebox->SetFlagBitOff(C_BIT_ENABLED);
        }

        tree = (C_TreeList*)win->FindControl(FILELIST_TREE);

        if (tree)
        {
            tree->DeleteBranch(tree->GetRoot());
            tree->SetUserNumber(0, 1);
            tree->SetSortType(TREE_SORT_CALLBACK);
            tree->SetSortCallback(FileNameSortCB);
            tree->SetCallback(LoadVirtualSelectFileCB);

            if (virtuallist)
                GetVirtualFileListTree(tree, virtuallist, C_TYPE_ITEM);

            GetFileListTree(tree, filespec, excludelist, C_TYPE_ITEM, TRUE, DELETE_FILE_POPUP);
            tree->RecalcSize();

            if (tree->Parent_)
                tree->Parent_->RefreshClient(tree->GetClient());
        }

        gMainHandler->ShowWindow(win);
        gMainHandler->WindowToFront(win);
    }
}

void SaveAFile(long TitleID, _TCHAR *filespec, _TCHAR *excludelist[], void (*YesCB)(long, short, C_Base*), void (*NoCB)(long, short, C_Base*), _TCHAR *filename)
{
    C_Window *win;
    C_Button *btn;
    C_Text *txt;
    C_EditBox *ebox;
    C_TreeList *tree;

    if ( not YesCB or not filespec)
        return;

    win = gMainHandler->FindWindow(SAVE_WIN);

    if (win)
    {
        txt = (C_Text*)win->FindControl(TITLE_LABEL);

        if (txt)
        {
            txt->Refresh();
            txt->SetText(TitleID);
            txt->Refresh();
        }

        btn = (C_Button *)win->FindControl(SAVE);

        if (btn)
        {
            btn->SetFlagBitOff(C_BIT_INVISIBLE);
            btn->SetCallback(YesCB);
        }

        btn = (C_Button *)win->FindControl(LOAD);

        if (btn)
        {
            btn->SetFlagBitOn(C_BIT_INVISIBLE);
            btn->SetCallback(YesCB);
        }

        btn = (C_Button *)win->FindControl(LOAD_VIRTUAL);

        if (btn)
        {
            btn->SetFlagBitOn(C_BIT_INVISIBLE);
            btn->SetCallback(YesCB);
        }

        btn = (C_Button *)win->FindControl(CANCEL);

        if (btn)
        {
            btn->SetFlagBitOff(C_BIT_INVISIBLE);
            btn->SetCallback(NoCB);
        }

        btn = (C_Button *)win->FindControl(CLOSE_WINDOW);

        if (btn)
        {
            btn->SetFlagBitOff(C_BIT_INVISIBLE);
            btn->SetCallback(NoCB);
        }

        ebox = (C_EditBox*)win->FindControl(FILE_NAME);

        if (ebox)
        {
            ebox->SetText(filename);
            ebox->Refresh();
            ebox->SetFlagBitOn(C_BIT_ENABLED);
        }

        tree = (C_TreeList*)win->FindControl(FILELIST_TREE);

        if (tree)
        {
            tree->DeleteBranch(tree->GetRoot());
            tree->SetUserNumber(0, 1);
            tree->SetSortType(TREE_SORT_CALLBACK);
            tree->SetSortCallback(FileNameSortCB);
            tree->SetCallback(LoadSaveSelectFileCB);
            GetFileListTree(tree, filespec, excludelist, C_TYPE_ITEM, TRUE, DELETE_FILE_POPUP);
            tree->RecalcSize();

            if (tree->Parent_)
                tree->Parent_->RefreshClient(tree->GetClient());
        }

        gMainHandler->ShowWindow(win);
        gMainHandler->WindowToFront(win);
    }
}

void CloseAYS()
{
    C_Window *win;
    win = gMainHandler->FindWindow(AYS_WIN);

    if (win)
        gMainHandler->HideWindow(win);
}

void AreYouSure(long TitleID, _TCHAR *text, void (*OkCB)(long, short, C_Base*), void (*CancelCB)(long, short, C_Base*))
{
    C_Window *win;
    C_Button *btn;
    C_Text *txt;
    short x, y;

    if ( not text)
        return;

    win = gMainHandler->FindWindow(AYS_WIN);

    if (win)
    {
        txt = (C_Text *)win->FindControl(MODAL_TITLE);

        if (txt)
            txt->SetText(TitleID);

        btn = (C_Button *)win->FindControl(ALERT_CANCEL);

        if (btn)
        {
            if (CancelCB)
            {
                btn->SetFlagBitOn(C_BIT_ENABLED);
                btn->SetCallback(CancelCB);
            }
            else
                btn->SetFlagBitOff(C_BIT_ENABLED);
        }

        btn = (C_Button *)win->FindControl(ALERT_OK);

        if (btn)
        {
            if (OkCB)
            {
                btn->SetFlagBitOn(C_BIT_ENABLED);
                btn->SetCallback(OkCB);
            }
            else
                btn->SetFlagBitOff(C_BIT_ENABLED);
        }

        x = 0;
        y = 0;
        DeleteGroupList(win->GetID());
        AddWordWrapTextToWindow(win, &x, &y, 0, static_cast<short>(win->ClientArea_[1].right - win->ClientArea_[1].left), 0xe0e0e0, text, 1);

        gMainHandler->ShowWindow(win);
        gMainHandler->WindowToFront(win);
    }
}

void AreYouSure(long TitleID, long MessageID, void (*OkCB)(long, short, C_Base*), void (*CancelCB)(long, short, C_Base*))
{
    AreYouSure(TitleID, gStringMgr->GetString(MessageID), OkCB, CancelCB);
}

BOOL CheckExclude(_TCHAR *filename, _TCHAR *directory, _TCHAR *ExcludeList[], _TCHAR *extension)
{
    _TCHAR fname[MAX_PATH];
    short i;

    i = 0;

    while (ExcludeList[i])
    {
        _stprintf(fname, "%s\\%s.%s", directory, ExcludeList[i], extension);

        if ( not _tcsicmp(filename, fname))
            return(TRUE);

        i++;
    }

    return(FALSE);
}

void VerifyDelete(long TitleID, void (*YesCB)(long, short, C_Base*), void (*NoCB)(long, short, C_Base*))
{
    AreYouSure(TitleID, TXT_DELETE_FILE, YesCB, NoCB);
}

void ExitVerify(long TitleID, void (*OkCB)(long, short, C_Base*), void (*CancelCB)(long, short, C_Base*))
{
    C_Window *win;
    C_Button *btn;
    C_Text *txt;

    win = gMainHandler->FindWindow(EXIT_WIN);

    if (win)
    {
        txt = (C_Text *)win->FindControl(MODAL_TITLE);

        if (txt)
            txt->SetText(TitleID);

        btn = (C_Button *)win->FindControl(ALERT_CANCEL);

        if (btn)
        {
            if (CancelCB)
            {
                btn->SetFlagBitOn(C_BIT_ENABLED);
                btn->SetCallback(CancelCB);
            }
            else
                btn->SetFlagBitOff(C_BIT_ENABLED);
        }

        btn = (C_Button *)win->FindControl(ALERT_OK);

        if (btn)
        {
            if (OkCB)
            {
                btn->SetFlagBitOn(C_BIT_ENABLED);
                btn->SetCallback(OkCB);
            }
            else
                btn->SetFlagBitOff(C_BIT_ENABLED);
        }

        gMainHandler->ShowWindow(win);
        gMainHandler->WindowToFront(win);
    }
}

void MakeVirtualListFromRsc(long ID, long startid)
{
    C_Resmgr *res;
    C_Hash *lst;
    C_HASHNODE *me;
    IMAGE_RSC *cur;
    long i = startid, curidx;

    if (i >= 200)
        return;

    VirtualFileList[i][0] = 0;

    res = gImageMgr->GetImageRes(ID);

    if (res)
    {
        lst = res->GetIDList();

        if (lst)
        {
            cur = (IMAGE_RSC*)lst->GetFirst(&me, &curidx);

            while (cur and i < 200)
            {
                _tcscpy(VirtualFileList[i++], cur->Header->ID);
                cur = (IMAGE_RSC*)lst->GetNext(&me, &curidx);
            }

            VirtualFileList[i][0] = 0;
        }
    }

}

extern int MRX;
extern int MRY;
extern int MAXOI;
extern short Map_Max_X;
extern short Map_Max_Y;

void MakeOccupationMap(IMAGE_RSC *Map)
{
    short x, y, i;
    uchar *ddPtr, *dst;
    uchar *src;
    WORD *Palette;
    long w, h;

    if ( not Map)
        return;

    if ( not Map->Owner)
        return;

    // HACK: Set uninitialized variables in CampMap
    // KCK TODO: Fix this, OR GetOwner() function for Terrain Team editor tool
    // MRX=Map_Max_X/MAP_RATIO;
    // MRY=Map_Max_Y/MAP_RATIO;
    // MAXOI = (sizeof(uchar)*MRX*MRY)/2;
    ShiAssert(MRX and MRY and MAXOI);

    Palette = Map->GetPalette();

    if (Palette)
    {
        Palette[0] = UI95_RGB24Bit(0);

        if (TheCampaign.Flags bitand CAMP_TACTICAL)
        {
            for (i = 1; i < NUM_TEAMS; i++)
            {
                if (TheCampaign.IsLoaded() and TeamInfo[i])
                    Palette[i] = UI95_RGB24Bit(TeamColorList[TeamInfo[i]->GetColor()]);
                else
                    Palette[i] = UI95_RGB24Bit(TeamColorList[TheCampaign.team_colour[i]]);
            }
        }
        else
        {
            for (i = 1; i < NUM_TEAMS; i++)
                Palette[i] = UI95_RGB24Bit(TeamColorList[i]);
        }
    }

    src = TheCampaign.CampMapData;
    ddPtr = (uchar*)Map->GetImage();

    if (ddPtr == NULL)
        return;

    if (src == NULL)
        return;

    w = Map->Header->w;
    h = Map->Header->h;
    dst = ddPtr + (h - 2) * w;
    src += w / 2;
    y = 1; // Skip pixels on edges of map

    while (y < (h - 2) and y < h)
    {
        x = 2;
        dst += 2;
        src++;

        while (x < (h - 2) and x < w)
        {
            ShiAssert(src >=  TheCampaign.CampMapData and 
                      src < TheCampaign.CampMapData + TheCampaign.CampMapSize);
            *dst++ = static_cast<uchar>((*src) bitand 0x0f);
            *dst++ = static_cast<uchar>((*src) >> 4);
            src++;
            x += 2;
        }

        src += ((w) - x) / 2;
        dst += (w - x);
        dst -= w * 2;
        y++;
    }
}

void MakeBigOccupationMap(IMAGE_RSC *Map)
{
    short x, y, i;
    uchar *ddPtr, *dst;
    uchar *src, *prevsrc;
    WORD *Palette;
    long w, h;

    if ( not Map)
        return;

    if ( not Map->Owner)
        return;

    // HACK: Set uninitialized variables in CampMap
    // KCK TODO: Fix this, OR GetOwner() function for Terrain Team editor tool
    // MRX=Map_Max_X/MAP_RATIO;
    // MRY=Map_Max_Y/MAP_RATIO;
    // MAXOI = (sizeof(uchar)*MRX*MRY)/2;
    ShiAssert(MRX and MRY and MAXOI);

    Palette = Map->GetPalette();

    if (Palette)
    {
        Palette[0] = UI95_RGB24Bit(0);

        if (TheCampaign.Flags bitand CAMP_TACTICAL)
        {
            for (i = 1; i < NUM_TEAMS; i++)
            {
                if (TheCampaign.IsLoaded() and TeamInfo[i])
                    Palette[i] = UI95_RGB24Bit(TeamColorList[TeamInfo[i]->GetColor()]);
                else
                    Palette[i] = UI95_RGB24Bit(TeamColorList[TheCampaign.team_colour[i]]);
            }
        }
        else
        {
            for (i = 1; i < NUM_TEAMS; i++)
                Palette[i] = UI95_RGB24Bit(TeamColorList[i]);
        }
    }

    src = TheCampaign.CampMapData;
    ddPtr = (uchar*)Map->GetImage();

    if (ddPtr == NULL)
        return;

    if (src == NULL)
        return;

    w = Map->Header->w;
    h = Map->Header->h;
    dst = ddPtr + (h - 2) * w;
    src += w / 2;
    y = 4; // Skip pixels on edges of map

    while (y < (h - 4) and y < h)
    {
        x = 4;
        dst += 4;
        src++;
        prevsrc = src;

        while (x < (h - 4) and x < w)
        {
            ShiAssert(src >=  TheCampaign.CampMapData and 
                      src < TheCampaign.CampMapData + TheCampaign.CampMapSize);
            *dst++ = static_cast<uchar>((*src) bitand 0x0f);
            *dst++ = static_cast<uchar>((*src) >> 4);
            *dst++ = static_cast<uchar>((*src) bitand 0x0f);
            *dst++ = static_cast<uchar>((*src) >> 4);
            src++;
            x += 4;
        }

        src += ((w) - x) / 2;
        dst += (w - x);
        dst -= w * 2;
        y++;
        x = 4;
        dst += 4;

        while (x < (h - 4) and x < w)
        {
            *dst++ = static_cast<uchar>((*prevsrc) bitand 0x0f);
            *dst++ = static_cast<uchar>((*prevsrc) >> 4);
            *dst++ = static_cast<uchar>((*prevsrc) bitand 0x0f);
            *dst++ = static_cast<uchar>((*prevsrc) >> 4);
            prevsrc++;
            x += 4;
        }

        dst += (w - x);
        dst -= w * 2;
        y++;
    }
}


static long PlaneIDTable[4][4] =
{
    { CB_1_1, 0, 0, 0,},
    { CB_2_1, CB_2_2, 0, 0,},
    { CB_3_1, CB_3_2, CB_3_3, 0,},
    { CB_4_1, CB_4_2, CB_4_3, CB_4_4,},
};

BOOL DisplayTarget(short MissType)
{
    switch (MissType)
    {
        case AMIS_BARCAP:
        case AMIS_BARCAP2:
        case AMIS_HAVCAP:
        case AMIS_TARCAP:
        case AMIS_RESCAP:
        case AMIS_AMBUSHCAP:
        case AMIS_SWEEP:
        case AMIS_FAC:
        case AMIS_ONCALLCAS:
        case AMIS_SAD:
        case AMIS_INT:
        case AMIS_BAI:
        case AMIS_PATROL:
        case AMIS_RECONPATROL:
            return(FALSE);
            break;

        default:
            return(TRUE);
            break;
    }

    return(FALSE);
}

void GetMissionTarget(Package curpackage, Flight curflight, _TCHAR Buffer[])
{
    WayPoint wp;
    CampEntity ent;
    GridIndex x = 0, y = 0;

    if (((curflight) and (curflight->GetUnitMission() not_eq AMIS_ABORT)) or ( not curflight))
    {
        ent = FindEntity(curpackage->GetMissionRequest()->targetID);

        if (ent and DisplayTarget(static_cast<short>(curflight->GetUnitMission())))
        {
            if (ent->IsObjective())
                ent->GetName(Buffer, 39, TRUE);
            else
                ent->GetName(Buffer, 39, FALSE);
        }
        else
        {
            wp = curflight->GetFirstUnitWP();

            if ( not wp)
            {
                _tcscpy(Buffer, gStringMgr->GetString(TXT_NO_TARGET));
                return;
            }

            while (wp)
            {
                if (wp->GetWPFlags() bitand WPF_TARGET)
                {
                    wp->GetWPLocation(&x, &y);

                    wp = NULL;
                }
                else
                {
                    wp = wp->GetNextWP();
                }
            }

            if ( not x and not y)
            {
                _tcscpy(Buffer, gStringMgr->GetString(TXT_NO_TARGET));
                return;
            }

            Buffer[0] = 0;
            AddLocationToBuffer('N', x, y, Buffer);
        }
    }
    else
        _tcscpy(Buffer, gStringMgr->GetString(TXT_ABORTED));
}

void GetFlightStatus(Flight element, _TCHAR buffer[])
{
    WayPoint wp;
    short found;

    wp = element->GetCurrentUnitWP();

    if (wp)
    {
        if (wp->GetWPAction() == WP_TAKEOFF)
            _tcscpy(buffer, gStringMgr->GetString(TXT_BRIEFING));
        else
        {
            found = 0;
            _tcscpy(buffer, gStringMgr->GetString(TXT_RETURNTOBASE));

            while (found == 0 and wp)
            {
                if (wp->GetWPAction() == WP_ASSEMBLE)
                {
                    _tcscpy(buffer, gStringMgr->GetString(TXT_ENROUTE));
                    found = 1;
                }
                else if (wp->GetWPAction() == WP_POSTASSEMBLE)
                {
                    _tcscpy(buffer, gStringMgr->GetString(TXT_EGRESS));
                    found = 1;
                }
                else if (wp->GetWPFlags() bitand WPF_TARGET)
                {
                    _tcscpy(buffer, gStringMgr->GetString(TXT_INGRESS));
                    found = 1;
                }
                else if (wp->GetWPAction() == WP_LAND)
                {
                    _tcscpy(buffer, gStringMgr->GetString(TXT_LANDING));
                    found = 0;
                }

                if (wp->GetWPAction() == WP_LAND)
                    wp = NULL;
                else
                    wp = wp->GetNextWP();
            }
        }
    }
    else
        _tcscpy(buffer, gStringMgr->GetString(TXT_RETURNTOBASE));
}

void UpdateMissionWindow(long ID)
{
    C_Window *win;
    C_Button *btn;
    C_Text *txt;
    TREELIST *cur;
    C_TreeList *tree;
    Package curpackage;
    Flight curflight;
    WayPoint wp;
    _TCHAR Task[200];
    _TCHAR Mission[200];
    _TCHAR TOT[200];
    _TCHAR Buffer[200];
    _TCHAR PilotName[40];
    F4CSECTIONHANDLE *Leave;
    FalconSessionEntity *session;
    short i, planecount;

    if ( not gMainHandler)
    {
        return;
    }

    win = gMainHandler->FindWindow(ID);

    if (win)
    {
        Leave = UI_Enter(win);
        curflight = (Flight)vuDatabase->Find(gCurrentFlightID);

        if (curflight)
            curpackage = (Package)curflight->GetUnitParent();
        else
            curpackage = NULL;

        tree = (C_TreeList*)win->FindControl(MISSION_LIST_TREE);

        if (tree)
        {
            cur = tree->GetRoot();

            while (cur)
            {
                if (((C_Mission*)cur->Item_)->GetVUID() == FalconLocalSession->GetPlayerFlightID())
                    cur->Item_->SetState(2);
                else
                    cur->Item_->SetState(0);

                if (((C_Mission*)cur->Item_)->GetVUID() == gCurrentFlightID)
                    cur->Item_->SetState(static_cast<short>(cur->Item_->GetState() bitor 1));

                cur = cur->Next;
            }
        }

        planecount = 0;

        if (curflight)
        {
            while (curflight->plane_stats[planecount] not_eq AIRCRAFT_NOT_ASSIGNED and planecount < PILOTS_PER_FLIGHT)
                planecount++;
        }

        if (curflight and curpackage and planecount)
        {
            txt = (C_Text *)win->FindControl(FLIGHT_LABEL);

            if (txt)
            {
                GetCallsign(curflight, Buffer);
                _tcscat(Buffer, ": ");
                _tcscat(Buffer, MissStr[curflight->GetUnitMission()]);
                txt->SetText(Buffer);
            }

            if (PlaneIDTable[planecount - 1][0])
            {
                for (i = 0; i < planecount; i++)
                {
                    btn = (C_Button *)win->FindControl(PlaneIDTable[planecount - 1][i]);

                    // JB 020211 Load the correct mission icons for each type of aircraft
                    if (g_bMissionACIcons)
                    {
                        UnitClassDataType *UnitPtr = NULL;

                        if (curflight)
                            UnitPtr = curflight->GetUnitClassData();

                        if (UnitPtr)
                        {
                            btn->SetImage(C_STATE_0, UnitPtr->IconIndex);
                            btn->SetImage(C_STATE_1, UnitPtr->IconIndex);
                            btn->SetImage(C_STATE_SELECTED, UnitPtr->IconIndex);
                            btn->SetImage(C_STATE_DISABLED, UnitPtr->IconIndex);
                        }
                    }

                    if (btn)
                    {
                        switch (curflight->plane_stats[i])
                        {
                            case AIRCRAFT_AVAILABLE:
                                session = gCommsMgr->FindCampaignPlayer(curflight->Id(), static_cast<uchar>(i));
                                btn->Refresh();

                                if (session)
                                    _stprintf(Buffer, "[%s]", session->GetPlayerName());
                                else
                                {
                                    if ( not TheCampaign.MissionEvaluator->GetPilotName(i, PilotName))
                                    {
                                        _stprintf(Buffer, "[%s]", gStringMgr->GetString(TXT_TBD));
                                    }
                                    else
                                        _stprintf(Buffer, "[%s]", PilotName);
                                }

                                btn->SetFlagBitOn(C_BIT_ENABLED);
                                btn->SetAllLabel(gStringMgr->GetText(gStringMgr->AddText(Buffer)));
                                break;

                            case AIRCRAFT_DEAD:
                                btn->SetFlagBitOff(C_BIT_ENABLED);
                                btn->SetAllLabel(TXT_KIA);
                                break;

                            case AIRCRAFT_RTB:
                                btn->SetFlagBitOff(C_BIT_ENABLED);
                                btn->SetAllLabel(TXT_RETURNTOBASE);
                                break;

                            case AIRCRAFT_MISSING:
                                btn->SetFlagBitOff(C_BIT_ENABLED);
                                btn->SetAllLabel(TXT_MIA);
                                break;

                            default:
                                btn->SetFlagBitOff(C_BIT_ENABLED);
                                btn->SetAllLabel(TXT_RETURNTOBASE);
                                break;
                        }
                    }
                }
            }

            // Get Task string
            if (curpackage->GetMissionRequest())
            {
                _tcscpy(Task, MissStr[curpackage->GetMissionRequest()->mission]);
            }

            // Get Mission string

            if ((curflight not_eq NULL) and (curflight->GetUnitMission() not_eq AMIS_ABORT))
            {
                GetMissionTarget(curpackage, curflight, Buffer);

                _tcscpy(Mission, Buffer);

                // Time on Target (TOT)
                _tcscpy(TOT, " ");
                wp = curflight->GetFirstUnitWP();

                while (wp)
                {
                    if (wp->GetWPFlags() bitand WPF_TARGET)
                    {
                        GetTimeString(wp->GetWPArrivalTime(), TOT);
                        wp = NULL;
                    }
                    else
                        wp = wp->GetNextWP();
                }
            }
            else
            {
                _tcscpy(Task, gStringMgr->GetString(TXT_RETURN_TO_BASE));
                _tcscpy(Mission, gStringMgr->GetString(TXT_MISSION_ABORTED));
                _tcscpy(TOT, gStringMgr->GetString(TXT_ABORTED));
            }

            // Task
            txt = (C_Text *)win->FindControl(ROLE_FIELD);

            if (txt)
            {
                txt->SetText(Task);
            }

            // Mission
            txt = (C_Text *)win->FindControl(MISSION_FIELD);

            if (txt)
            {
                txt->SetText(Mission);
            }

            // TOT
            txt = (C_Text *)win->FindControl(TOT_FIELD);

            if (txt)
            {
                txt->SetText(TOT);
            }

            for (i = 1; i < 5; i++)
                if (i == planecount)
                    win->UnHideCluster(i);
                else
                    win->HideCluster(i);

            win->UnHideCluster(5);
        }
        else
        {
            win->HideCluster(1);
            win->HideCluster(2);
            win->HideCluster(3);
            win->HideCluster(4);
            txt = (C_Text *)win->FindControl(FLIGHT_LABEL);

            if (txt)
            {
                _tcscpy(Buffer, gStringMgr->GetString(/*TXT_PLANNING*/TXT_NOT_AVAILABLE)); //Cobra
                txt->SetText(Buffer);
            }

            _tcscpy(Task, gStringMgr->GetString(TXT_NOT_AVAILABLE));
            _tcscpy(Mission, gStringMgr->GetString(TXT_NOT_AVAILABLE));
            _tcscpy(TOT, gStringMgr->GetString(TXT_NOT_AVAILABLE));

            // Task
            txt = (C_Text *)win->FindControl(ROLE_FIELD);

            if (txt)
            {
                txt->SetText(Task);
            }

            // Mission
            txt = (C_Text *)win->FindControl(MISSION_FIELD);

            if (txt)
            {
                txt->SetText(Mission);
            }

            // TOT
            txt = (C_Text *)win->FindControl(TOT_FIELD);

            if (txt)
            {
                txt->SetText(TOT);
            }
        }

        win->RefreshWindow();
        UI_Leave(Leave);
    }
}

static void MakeBar(C_Line *line, long valueID, long Team)
{
    long max = 1;
    long value = 0, w;

    switch (valueID)
    {
        case STAT_1:
            value = TeamInfo[Team]->GetCurrentStats()->aircraft;

            if (TeamInfo[2]->startStats.aircraft > TeamInfo[6]->startStats.aircraft)
                max = TeamInfo[2]->startStats.aircraft;
            else
                max = TeamInfo[6]->startStats.aircraft;

            break;

        case STAT_2:
            value = TeamInfo[Team]->GetCurrentStats()->airDefenseVehs;

            if (TeamInfo[2]->startStats.airDefenseVehs > TeamInfo[6]->startStats.airDefenseVehs)
                max = TeamInfo[2]->startStats.airDefenseVehs;
            else
                max = TeamInfo[6]->startStats.airDefenseVehs;

            break;

        case STAT_3:
            value = TeamInfo[Team]->GetCurrentStats()->groundVehs;

            if (TeamInfo[2]->startStats.groundVehs > TeamInfo[6]->startStats.groundVehs)
                max = TeamInfo[2]->startStats.groundVehs;
            else
                max = TeamInfo[6]->startStats.groundVehs;

            break;

        case STAT_4:
            value = TeamInfo[Team]->GetCurrentStats()->ships;

            if (TeamInfo[2]->startStats.ships > TeamInfo[6]->startStats.ships)
                max = TeamInfo[2]->startStats.ships;
            else
                max = TeamInfo[6]->startStats.ships;

            break;

        case STAT_5:
            value = TeamInfo[Team]->GetCurrentStats()->supplyLevel;

            if (TeamInfo[2]->startStats.supplyLevel > TeamInfo[6]->startStats.supplyLevel)
                max = TeamInfo[2]->startStats.supplyLevel;
            else
                max = TeamInfo[6]->startStats.supplyLevel;

            break;

        case STAT_6:
            value = TeamInfo[Team]->GetCurrentStats()->fuelLevel;

            if (TeamInfo[2]->startStats.fuelLevel > TeamInfo[6]->startStats.fuelLevel)
                max = TeamInfo[2]->startStats.fuelLevel;
            else
                max = TeamInfo[6]->startStats.fuelLevel;

            break;

        case STAT_7:
            value = TeamInfo[Team]->GetCurrentStats()->airbases;

            if (TeamInfo[2]->startStats.airbases > TeamInfo[6]->startStats.airbases)
                max = TeamInfo[2]->startStats.airbases;
            else
                max = TeamInfo[6]->startStats.airbases;

            break;

        case STAT_8:
            value = TeamInfo[Team]->GetCurrentStats()->aircraft;

            if (TeamInfo[2]->startStats.aircraft > TeamInfo[6]->startStats.aircraft)
                max = TeamInfo[2]->startStats.aircraft;
            else
                max = TeamInfo[6]->startStats.aircraft;

            break;

        case STAT_9:
            value = TeamInfo[Team]->GetCurrentStats()->groundVehs;

            if (TeamInfo[2]->startStats.groundVehs > TeamInfo[6]->startStats.groundVehs)
                max = TeamInfo[2]->startStats.groundVehs;
            else
                max = TeamInfo[6]->startStats.groundVehs;

            break;
    }

    if (line->GetUserNumber(0) == 0)
    {
        line->SetUserNumber(0, line->GetX());
        line->SetUserNumber(1, line->GetY());
        line->SetUserNumber(2, line->GetW());
        line->SetUserNumber(3, line->GetH());
    }

    if ( not max)
        max = 1000000;

    w = (line->GetUserNumber(2) * value) / max;

    if (w > line->GetUserNumber(2))
        w = line->GetUserNumber(2);

    line->SetW(w);

    line->Parent_->update_ or_eq C_DRAW_REFRESH;
    line->Parent_->SetUpdateRect(line->GetUserNumber(0), line->GetUserNumber(1),
                                 line->GetUserNumber(0) + line->GetUserNumber(2),
                                 line->GetUserNumber(1) + line->GetUserNumber(3),
                                 line->GetFlags(), line->GetClient());
}

_TCHAR *ObjStr[5] = {NULL, NULL, NULL, NULL, NULL};
void UpdateIntel(long ID)
{
    C_Window *win;
    C_ListBox *lbox;
    C_Line *line;
    C_Text *txt;
    short Team;
    short i;

    Team = FalconLocalSession->GetTeam();

    for (i = 0; i < 5; i++)
        if (ObjStr[i] == NULL)
        {
            ObjStr[i] = new _TCHAR[256];
            memset(ObjStr[i], 0, sizeof(_TCHAR) * 256);
        }

    i = static_cast<short>(GetTopPriorityObjectives(Team, ObjStr));

    win = gMainHandler->FindWindow(ID);

    //if(win) // JB 010222 CTD
    if (win and not F4IsBadReadPtr(win, sizeof(C_Window)) // JB 010222 CTD
       and TeamInfo[Team]) // JB 010614 CTD
    {
        if (TeamInfo[Team]->GetOffensiveAirAction()->actionType > AACTION_DCA)
            win->UnHideCluster(3);
        else
            win->HideCluster(3);

        lbox = (C_ListBox *)win->FindControl(OFFENSIVE_AIR_TEXT);

        if (lbox)
        {
            lbox->Refresh();

            if (TeamInfo[Team] and TeamInfo[Team]->GetOffensiveAirAction()->actionType)
                lbox->SetValue(TeamInfo[Team]->GetOffensiveAirAction()->actionType);
            else
                lbox->SetValue(0);

            lbox->Refresh();
        }

        txt = (C_Text *)win->FindControl(TIME_AIR_OP);

        if (txt)
        {
            txt->Refresh();

            if (TeamInfo[Team] and TeamInfo[Team]->GetOffensiveAirAction()->actionType)
            {
                _TCHAR timeStr[30] = {0};
                AddTimeToBuffer(TeamInfo[Team]->GetOffensiveAirAction()->actionStartTime, timeStr, FALSE);
                // KCK HACK: Can't seem to get this to size right. Remind me to ask Peter about
                // this when he gets in
                _tcscat(timeStr, "   ");
                txt->SetText(timeStr);
            }
            else
                txt->SetText("");

            txt->Refresh();
        }

        lbox = (C_ListBox *)win->FindControl(POSTURE_TEXT);

        if (lbox)
        {
            if (TeamInfo[Team])
            {
                lbox->SetValue(TeamInfo[Team]->GetGroundAction()->actionType);
                lbox->Refresh();
            }
        }

        if (TeamInfo[Team]->GetGroundAction()->actionType >= GACTION_MINOROFFENSIVE)
        {
            win->UnHideCluster(2);
            win->HideCluster(1);
        }
        else
        {
            win->UnHideCluster(1);
            win->HideCluster(2);
        }

        lbox = (C_ListBox *)win->FindControl(OFFENSIVE_GROUND_TEXT);

        if (lbox)
        {
            lbox->Refresh();

            if (TeamInfo[Team])
            {
                lbox->SetValue(TeamInfo[Team]->GetGroundAction()->actionType);
                lbox->Refresh();
            }
        }

        txt = (C_Text *)win->FindControl(TIME_GROUND_OP);

        if (txt)
        {
            _TCHAR timeStr[30] = {0};
            txt->Refresh();
            AddTimeToBuffer(TeamInfo[Team]->GetGroundAction()->actionTime, timeStr, FALSE);
            // KCK HACK: Can't seem to get this to size right. Remind me to ask Peter about
            // this when he gets in
            _tcscat(timeStr, "   ");
            txt->SetText(timeStr);
            txt->Refresh();
        }

        lbox = (C_ListBox *)win->FindControl(BAR_1);

        if (lbox)
        {
            line = (C_Line *)win->FindControl(BLUE_BAR_1);

            if (line)
                MakeBar(line, lbox->GetTextID(), 2);

            line = (C_Line *)win->FindControl(RED_BAR_1);

            if (line)
                MakeBar(line, lbox->GetTextID(), 6);
        }

        lbox = (C_ListBox *)win->FindControl(BAR_2);

        if (lbox)
        {
            line = (C_Line *)win->FindControl(BLUE_BAR_2);

            if (line)
                MakeBar(line, lbox->GetTextID(), 2);

            line = (C_Line *)win->FindControl(RED_BAR_2);

            if (line)
                MakeBar(line, lbox->GetTextID(), 6);
        }

        lbox = (C_ListBox *)win->FindControl(BAR_3);

        if (lbox)
        {
            line = (C_Line *)win->FindControl(BLUE_BAR_3);

            if (line)
                MakeBar(line, lbox->GetTextID(), 2);

            line = (C_Line *)win->FindControl(RED_BAR_3);

            if (line)
                MakeBar(line, lbox->GetTextID(), 6);
        }

        lbox = (C_ListBox *)win->FindControl(BAR_4);

        if (lbox)
        {
            line = (C_Line *)win->FindControl(BLUE_BAR_4);

            if (line)
                MakeBar(line, lbox->GetTextID(), 2);

            line = (C_Line *)win->FindControl(RED_BAR_4);

            if (line)
                MakeBar(line, lbox->GetTextID(), 6);
        }

        txt = (C_Text *)win->FindControl(OBJ_1);

        if (txt)
        {
            txt->Refresh();

            if (i > 0)
                txt->SetText(ObjStr[0]);
            else
                txt->SetText(" ");

            txt->Refresh();
        }

        txt = (C_Text *)win->FindControl(OBJ_2);

        if (txt)
        {
            txt->Refresh();

            if (i > 1)
                txt->SetText(ObjStr[1]);
            else
                txt->SetText(" ");

            txt->Refresh();
        }

        txt = (C_Text *)win->FindControl(OBJ_3);

        if (txt)
        {
            txt->Refresh();

            if (i > 2)
                txt->SetText(ObjStr[2]);
            else
                txt->SetText(" ");

            txt->Refresh();
        }

        txt = (C_Text *)win->FindControl(OBJ_4);

        if (txt)
        {
            txt->Refresh();

            if (i > 3)
                txt->SetText(ObjStr[3]);
            else
                txt->SetText(" ");

            txt->Refresh();
        }

        txt = (C_Text *)win->FindControl(OBJ_5);

        if (txt)
        {
            txt->Refresh();

            if (i > 4)
                txt->SetText(ObjStr[4]);
            else
                txt->SetText(" ");

            txt->Refresh();
        }
    }
}

void UpdateIntelBarCB(long ID, short hittype, C_Base *control)
{
    C_Line *line;
    C_Window *win;
    C_ListBox *lbox;

    if (hittype not_eq C_TYPE_SELECT)
        return;

    lbox = (C_ListBox *)control;
    win = control->Parent_;

    switch (ID)
    {
        case BAR_1:
            line = (C_Line *)win->FindControl(BLUE_BAR_1);

            if (line)
                MakeBar(line, lbox->GetTextID(), 2);

            line = (C_Line *)win->FindControl(RED_BAR_1);

            if (line)
                MakeBar(line, lbox->GetTextID(), 6);

            break;

        case BAR_2:
            line = (C_Line *)win->FindControl(BLUE_BAR_2);

            if (line)
                MakeBar(line, lbox->GetTextID(), 2);

            line = (C_Line *)win->FindControl(RED_BAR_2);

            if (line)
                MakeBar(line, lbox->GetTextID(), 6);

            break;

        case BAR_3:
            line = (C_Line *)win->FindControl(BLUE_BAR_3);

            if (line)
                MakeBar(line, lbox->GetTextID(), 2);

            line = (C_Line *)win->FindControl(RED_BAR_3);

            if (line)
                MakeBar(line, lbox->GetTextID(), 6);

            break;

        case BAR_4:
            line = (C_Line *)win->FindControl(BLUE_BAR_4);

            if (line)
                MakeBar(line, lbox->GetTextID(), 2);

            line = (C_Line *)win->FindControl(RED_BAR_4);

            if (line)
                MakeBar(line, lbox->GetTextID(), 6);

            break;
    }
}

short AddButtonToWindow(C_Window *, short, short, COLORREF, _TCHAR *)
{
    return(0);
}

short AddTextToWindow(C_Window *win, short x, short y, COLORREF color, _TCHAR *str)
{
    C_Text *txt;

    if (win == NULL or str == NULL)
        return(0);

    txt = new C_Text;
    txt->Setup(C_DONT_CARE, C_TYPE_NORMAL);
    txt->SetFixedWidth(_tcsclen(str) + 1);
    txt->SetText(str);
    txt->SetXY(x, y);
    txt->SetFGColor(color);
    txt->SetFont(win->Font_);
    txt->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
    txt->SetFlagBitOn(C_BIT_LEFT);

    win->AddControl(txt);

    return(static_cast<short>(txt->GetX() + txt->GetW()));
}

void AddHorizontalLineToWindow(C_Window *win, short *, short *y, short startcol, short endcol, COLORREF, long Client)
{
    C_Box
    *box;

    box = new C_Box;

    box->Setup(C_DONT_CARE, 0);
    box->SetXYWH(startcol, *y, endcol, 0);
    box->SetColor(0x00AD8041);
    box->SetClient(static_cast<short>(Client));
    box->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);

    win->AddControl(box);

    *y += 2;
}

BOOL AddWordWrapTextToWindow(C_Window *win, short *x, short *y, short, short endcol, COLORREF color, _TCHAR *str, long Client)
{
    C_Text *txt;
    short wrap_w;
    BOOL retval;

    retval = TRUE;

    if (win == NULL or str == NULL)
        return(0);

    wrap_w = static_cast<short>(max(50, endcol - *x));

    // Cobra Original code that crashes with Radeon 9800
    if (str)
    {
        txt = new C_Text;
        txt->Setup(C_DONT_CARE, C_TYPE_NORMAL);
        txt->SetFixedWidth(_tcsclen(str) + 1);
        txt->SetXY(*x, *y);
        txt->SetW(wrap_w);
        txt->SetFGColor(color);
        txt->SetFont(win->Font_);
        txt->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
        txt->SetFlagBitOn(C_BIT_LEFT bitor C_BIT_WORDWRAP);
        txt->SetText(str);
        txt->SetClient(static_cast<short>(Client));
        win->AddControl(txt);

        if (gFontList->StrWidth(win->Font_, str) > wrap_w)
        {
            *y += txt->GetH() - gFontList->GetHeight(win->Font_);
            *x += txt->GetW();
        }
        else
        {
            *x += gFontList->StrWidth(win->Font_, str);
        }

        retval = 1;

#if 0
        wrap = UI_WordWrap(win, str, win->Font_, wrap_w, &status);

        if ( not status)
            retval = status;

        while (wrap)
        {
            wrap_w = endcol - startcol;
            txt = new C_Text;
            txt->Setup(C_DONT_CARE, C_TYPE_NORMAL);
            txt->SetFixedWidth(_tcsclen(wrap) + 1);
            txt->SetText(wrap);
            txt->SetXY(*x, *y);
            txt->SetFGColor(color);
            txt->SetFont(win->Font_);
            txt->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
            txt->SetFlagBitOn(C_BIT_LEFT);
            txt->SetClient(Client);
            win->AddControl(txt);
            wrap = UI_WordWrap(win, NULL, win->Font_, wrap_w, &status);

            if (wrap)
            {
                *x = startcol;
                *y += gFontList->GetHeight(win->Font_);
            }

            if ( not status)
                retval = status;
        }

        *x = txt->GetX() + txt->GetW();
#endif
    }

    return(retval);
}

static CampUIEventElement *RetrieveEvent(short num)
{
    short i = 0;
    CampUIEventElement *retval = NULL;
    CampUIEventElement *evt;

    CampEnterCriticalSection();
    evt = TheCampaign.StandardEventQueue;

    while (evt and i < num)
    {
        evt = evt->next;
        i++;
    }

    if (evt)
    {
        retval = new uieventnode;
        memcpy(retval, evt, sizeof(CampUIEventElement));
        retval->eventText = new _TCHAR[_tcslen(evt->eventText) + 1];
        _tcscpy(retval->eventText, evt->eventText);
        retval->next = NULL;
    }

    CampLeaveCriticalSection();
    return(retval);
}

void RefreshMapEventList(long winID, long client)
{
    CampUIEventElement *evt;
    C_Window *win;
    C_Text *txt;
    C_Blip *blip;
    short x, y;
    F4CSECTIONHANDLE *Leave;

    win = gMainHandler->FindWindow(winID);

    if (win)
    {
        Leave = UI_Enter(win);
        evt = RetrieveEvent(0);
        txt = (C_Text*)win->FindControl(CP_EVENT);

        // M.N. clear last news text from last mission
        if ( not evt and txt)
        {
            char text[1];
            strcpy(text, "");
            txt->Refresh();
            txt->SetText(text);
            txt->Refresh();
        }

        if (evt)
        {
            if (txt)
            {
                txt->Refresh();
                txt->SetText(gStringMgr->GetText(gStringMgr->AddText(evt->eventText)));
                txt->Refresh();
            }

            blip = (C_Blip*)win->FindControl(9000000);

            if (blip)
            {
                x = static_cast<short>(evt->x / MAP_RATIO - 2);
                y = static_cast<short>(170 - (evt->y / MAP_RATIO) - 2);

                blip->AddBlip(x, y, evt->team, evt->time / (VU_TICS_PER_SECOND * 60));
                blip->Refresh();
            }

            delete evt->eventText;
            delete evt;
            win->RefreshClient(client);
        }

        UI_Leave(Leave);
    }
}

void RefreshEventList()
{
    CampUIEventElement *evt;
    C_Window *win;
    C_Text *txt;
    _TCHAR buffer[10];
    short y, wrap_w, i;

    win = gMainHandler->FindWindow(RVNTS_WIN);

    if (win)
    {
        DeleteGroupList(RVNTS_WIN);

        wrap_w = static_cast<short>(win->ClientArea_[0].right - win->ClientArea_[0].left - 40);

        y = 0;
        i = 0;
        evt = RetrieveEvent(i);

        while (evt)
        {
            if (evt->eventText)
            {
                GetTimeString(evt->time, buffer);
                buffer[5] = 0;

                txt = new C_Text;
                txt->Setup(C_DONT_CARE, C_TYPE_NORMAL);
                txt->SetFixedWidth(_tcsclen(buffer) + 1);
                txt->SetText(buffer);
                txt->SetFont(win->Font_);
                txt->SetXY(1, y);
                txt->SetFGColor(0xe0e0e0);
                txt->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
                win->AddControl(txt);
                txt->Refresh();

                txt = new C_Text;
                txt->Setup(C_DONT_CARE, C_TYPE_NORMAL);
                txt->SetFlagBitOn(C_BIT_WORDWRAP);
                txt->SetFixedWidth(_tcsclen(evt->eventText));
                txt->SetText(evt->eventText);
                txt->SetFont(win->Font_);
                txt->SetXY(40, y);
                txt->SetW(wrap_w);
                txt->SetFGColor(0xe0e0e0);
                txt->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
                win->AddControl(txt);
                y += txt->GetH() + 2;
            }

            delete evt->eventText;
            delete evt;
            i++;
            evt = RetrieveEvent(i);
        }

        win->ScanClientAreas();
        win->RefreshClient(0);
    }
}

void RelocateSquadron()
{
    C_Window *win;
    C_Bitmap *bmp;
    C_Text *txt;
    C_Resmgr *res;
    IMAGE_RSC *rsc;
    Objective Obj;
    Squadron sqd;
    _TCHAR buffer[80];
    GridIndex x, y;
    ObjClassDataType *ObjPtr;

    if ( not gMainHandler)
        return;

    sqd = (Squadron)FalconLocalSession->GetPlayerSquadron();

    if ( not sqd)
        return;

    Obj = (Objective)vuDatabase->Find(sqd->GetUnitAirbaseID());

    if ( not Obj)
        return;

    ObjPtr = Obj->GetObjectiveClassData();

    if ( not ObjPtr)
        return;

    win = gMainHandler->FindWindow(TRANSFER_WIN);

    if (win)
    {
        res = gImageMgr->GetImageRes(BLUE_TEAM_ICONS_W);

        txt = (C_Text*)win->FindControl(MY_SQUADRON);

        if (txt)
        {
            sqd = (Squadron)FalconLocalSession->GetPlayerSquadron();

            if (sqd)
            {
                sqd->GetName(buffer, 38, FALSE);
                txt->Refresh();
                txt->SetText(buffer);
                txt->Refresh();
            }
        }

        txt = (C_Text*)win->FindControl(NEW_BASE);

        if (txt)
        {
            Obj->GetName(buffer, 38, TRUE);
            txt->Refresh();
            txt->SetText(buffer);
            txt->Refresh();
        }

        bmp = (C_Bitmap *)win->FindControl(CS_MAP_WIN);

        if (bmp)
        {
            bmp->SetImage(gOccupationMap);
            bmp->Refresh();
        }

        bmp = (C_Bitmap *)win->FindControl(BASE_ICON);

        if (bmp)
        {
            bmp->Refresh();
            Obj->GetLocation(&x, &y); // campaign coords
            y = static_cast<short>(TheCampaign.TheaterSizeY - y);

            if (res)
            {
                int mapratio = MAP_RATIO;

                // 2002-02-01 MN This fixes relocation map for large theaters
                if (g_LargeTheater)
                {
                    mapratio *= 2;
                }

                bmp->SetXY(x / mapratio, y / mapratio);
                bmp->SetFlagBitOn(C_BIT_HCENTER bitor C_BIT_VCENTER);
                rsc = (IMAGE_RSC*)res->Find(ObjPtr->IconIndex);

                if (rsc)
                    bmp->SetImage(rsc);

                bmp->Refresh();
            }
        }

        gMainHandler->EnableWindowGroup(win->GetGroup());
    }
}

// Screen shot HACK stuff
void CloseItCB(long, short hit, C_Base *ctrl)
{
    if (hit not_eq C_TYPE_LMOUSEUP)
        return;

    gMainHandler->HideWindow(ctrl->Parent_);
}

#pragma pack(1)
typedef struct
{
    long First, Second, Third;
    WORD Width;
    WORD Height;
    WORD Last;
} BOGUS_HEADER;
#pragma pack()

BOGUS_HEADER TgaHeader =
{
    0x00020000,
    0x00000000,
    0x00000000,
    800, 600, // w,h
    0x0110,
};

BOGUS_HEADER TgaHeaderHiRes =
{
    0x00020000,
    0x00000000,
    0x00000000,
    1024, 768, // w,h
    0x0110,
};

extern WORD *gScreenShotBuffer;
void SaveTargaCB(long, short hittype, C_Base *control)
{
    _TCHAR filename[MAX_PATH];
    C_EditBox *ebox;
    FILE *fp;
    long y, i;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gMainHandler->HideWindow(control->Parent_);

    ebox = (C_EditBox*)control->Parent_->FindControl(FILE_NAME);

    if (ebox)
    {

        //dpc EmptyFilenameSaveFix, modified by MN - added a warning to enter a filename
        if (g_bEmptyFilenameFix)
        {
            if (_tcslen(ebox->GetText()) == 0)
            {
                AreYouSure(TXT_WARNING, TXT_ENTER_FILENAME, CloseWindowCB, CloseWindowCB);
                return;
            }
        }

        //end EmptyFilenameSaveFix

        // MN 020104 HiResUI support
        int w = 800, h = 600;

        if (g_bHiResUI)
        {
            w = 1024;
            h = 768;
        }

        // convert Mem to 555 format
        for (i = 0; i < w * h; i++)
            gScreenShotBuffer[i] = UI95_ScreenToTga(gScreenShotBuffer[i]);


        // Write file
        //MI put them where they belong
#if 0
        _stprintf(filename, "%s.tga", ebox->GetText());
#else
        _stprintf(filename, "%s\\%s.tga", FalconPictureDirectory, ebox->GetText());
#endif

        fp = fopen(filename, "wb");

        if (fp)
        {
            if (g_bHiResUI)
                fwrite(&TgaHeaderHiRes, sizeof(TgaHeaderHiRes), 1, fp);
            else
                fwrite(&TgaHeader, sizeof(TgaHeader), 1, fp);

            for (y = 0; y < h; y++)
                fwrite(&gScreenShotBuffer[((h - 1) - y)*w], w * sizeof(WORD), 1, fp);

            fclose(fp);
        }
    }
}

void SaveScreenShot()
{
    SetDeleteCallback(DelTGAFileCB);
    char path[1024];
    strcpy(path, FalconDataDirectory);
    strcat(path, "\\pictures\\*.tga");
    SaveAFile(gStringMgr->AddText("Save Screenshot"), path, NULL, SaveTargaCB, CloseItCB, "");
}

char MasterXOR[] = "FreeFalcon is your Master";

void EncryptBuffer(uchar startkey, uchar *buffer, long length)
{
    long i, xrlen, idx;
    uchar *ptr;
    uchar nextkey;

    if ( not buffer or length <= 0)
        return;

    idx = 0;
    xrlen = strlen(MasterXOR);

    ptr = buffer;

    for (i = 0; i < length; i++)
    {
        *ptr xor_eq MasterXOR[(idx++) % xrlen];
        *ptr xor_eq startkey;
        nextkey = *ptr++;
        startkey = nextkey;
    }
}

void DecryptBuffer(uchar startkey, uchar *buffer, long length)
{
    long i, xrlen, idx;
    uchar *ptr;
    uchar nextkey;

    if ( not buffer or length <= 0)
        return;

    idx = 0;
    xrlen = strlen(MasterXOR);

    ptr = buffer;

    for (i = 0; i < length; i++)
    {
        nextkey = *ptr;
        *ptr xor_eq startkey;
        *ptr++ xor_eq MasterXOR[(idx++) % xrlen];
        startkey = nextkey;
    }
}

void ScrollTimerCB(long, short, C_Base *base)
{
    C_Window *win;

    if ( not base)
        return;

    win = base->Parent_;

    if (win)
    {
        if (win->VY_[base->GetClient()] > base->GetUserNumber(0))
        {
            win->VY_[base->GetClient()] -= 2;
            win->RefreshClient(base->GetClient());
        }
    }
}

void LoadPeopleInfo(long client)
{
    C_Window *win = NULL;
    char *filedata = NULL;
    long size = 0;
    TitleStr *tit = NULL;
    PersonStr *per = NULL;
    LegalStr *leg = NULL;
    short type = 0;
    long offset = 0;
    long center = 0, y = 0;
    long LanguageID = 0;
    C_Text *txt = NULL;
    C_TimerHook *drawTimer = NULL;
    C_Resmgr *res = NULL;
    FLAT_RSC *rsc = NULL;

    res = new C_Resmgr;

    if ( not res)
        return;

    res->Setup(CREDITS_RES, "art\\resource\\credits", gMainParser->GetTokenHash());
    res->LoadData();

    switch (gLangIDNum)
    {
        case F4LANG_GERMAN:
            LanguageID = GERMAN_CREDITS;
            break;

        case F4LANG_FRENCH:
            LanguageID = FRENCH_CREDITS;
            break;

        case F4LANG_SPANISH:
            LanguageID = SPANISH_CREDITS;
            break;

        case F4LANG_ITALIAN:
            LanguageID = ITALIAN_CREDITS;
            break;

        case F4LANG_PORTUGESE:
            LanguageID = PORTUGUESE_CREDITS;
            break;

        case F4LANG_ENGLISH:
        case F4LANG_UK:
        default:
            LanguageID = ENGLISH_CREDITS;
            break;
    }

    rsc = (FLAT_RSC*)res->Find(LanguageID);

    if ( not rsc)
        return;

    filedata = (char*)rsc->GetData();
    size = rsc->Header->size;

    win = gMainHandler->FindWindow(EXIT_WIN);

    if (win)
    {
        center = ((win->ClientArea_[client].right - win->ClientArea_[client].left) / 2);

        y = win->GetH() / 2;
        offset = 0;

        //JAM 05Oct03
#if 0
        FILE *file;
        char strLine[256];
        char strID[256];
        char strVal[256];
        int font;

        file = fopen("art\\resource\\credits.txt", "r");

        if (file)
        {
            while (fgets(strLine, sizeof(strLine) / sizeof(strLine[0]), file))
            {
                ZeroMemory(strVal, sizeof(strVal));

                if (sscanf(strLine, "FONT %d", &font))
                    continue;

                sscanf(strLine, "%s %[^~]", strID, strVal);

                txt = new C_Text;
                txt->Setup(C_DONT_CARE, 0);
                txt->SetFont(font);
                txt->SetFGColor(0xffffff);
                txt->SetFlagBitOn(C_BIT_HCENTER);
                txt->SetClient(static_cast<short>(client));
                txt->SetXY(center, y);
                txt->SetText(gStringMgr->GetText(gStringMgr->AddText(strVal)));
                win->AddControl(txt);
                y += gFontList->GetHeight(font) + 10;
            }

            fclose(file);
        }

        //JAM
#else

        while (offset < size)
        {
            type = *((short*)&filedata[offset]);
            offset += sizeof(short);

            switch (type)
            {
                case _TITLE_:
                    tit = (TitleStr*)&filedata[offset];
                    DecryptBuffer(0x79, (uchar*)&filedata[offset], sizeof(TitleStr));
                    offset += sizeof(TitleStr);
                    txt = new C_Text;
                    txt->Setup(C_DONT_CARE, 0);
                    txt->SetFont(tit->FontID);
                    txt->SetFGColor(tit->ColorID);
                    txt->SetFlagBitOn(C_BIT_HCENTER);
                    txt->SetClient(static_cast<short>(client));
                    txt->SetXY(center, y);
                    txt->SetText(gStringMgr->GetText(gStringMgr->AddText(tit->Title)));
                    win->AddControl(txt);
                    y += gFontList->GetHeight(tit->FontID) + 10;
                    break;

                case _NAME_:
                    per = (PersonStr*)&filedata[offset];
                    DecryptBuffer(0x79, (uchar*)&filedata[offset], sizeof(PersonStr));
                    offset += sizeof(PersonStr);
                    txt = new C_Text;
                    txt->Setup(C_DONT_CARE, 0);
                    txt->SetFont(per->FontID);
                    txt->SetFGColor(per->ColorID);
#if 0

                    if (per->Job[0])
                    {
                        txt->SetFlagBitOn(C_BIT_RIGHT);
                        txt->SetXY(center - 10, y);
                    }
                    else
                    {
#endif
                        txt->SetFlagBitOn(C_BIT_HCENTER);
                        txt->SetXY(center, y);
#if 0
                    }

#endif
                    txt->SetClient(static_cast<short>(client));
                    txt->SetText(gStringMgr->GetText(gStringMgr->AddText(per->Name)));
                    win->AddControl(txt);
#if 0

                    if (per->Job[0])
                    {
                        txt = new C_Text;
                        txt->Setup(C_DONT_CARE, 0);
                        txt->SetFont(per->FontID);
                        txt->SetFGColor(per->ColorID);
                        txt->SetFlagBitOn(C_BIT_LEFT);
                        txt->SetClient(client);
                        txt->SetXY(center + 10, y);
                        txt->SetText(gStringMgr->GetText(gStringMgr->AddText(per->Job)));
                        win->AddControl(txt);
                    }

#endif
                    y += gFontList->GetHeight(per->FontID);
                    break;

                case _LEGAL_:
                    leg = (LegalStr*)&filedata[offset];
                    DecryptBuffer(0x79, (uchar*)&filedata[offset], sizeof(LegalStr));
                    offset += sizeof(LegalStr);
                    txt = new C_Text;
                    txt->Setup(C_DONT_CARE, 0);
                    txt->SetFont(leg->FontID);
                    txt->SetFGColor(leg->ColorID);
                    txt->SetFlagBitOn(C_BIT_LEFT);
                    txt->SetClient(static_cast<short>(client));
                    txt->SetXY(0, y);
                    txt->SetText(gStringMgr->GetText(gStringMgr->AddText(leg->Legal)));
                    win->AddControl(txt);
                    y += gFontList->GetHeight(leg->FontID);
                    break;

                case _BLANK_:
                    y += 40;
                    break;
            }
        }

        y += win->GetH() / 2;

#ifdef _DO_CREDITS_HACK // OW
        char strTmp[80];
        strTmp[0x0] = 'D';
        strTmp[0x1] = 'i';
        strTmp[0x2] = 'r';
        strTmp[0x3] = 'e';
        strTmp[0x4] = 'c';
        strTmp[0x5] = 't';
        strTmp[0x6] = 'X';
        strTmp[0x7] = ' ';
        strTmp[0x8] = '7';
        strTmp[0x9] = ' ';
        strTmp[0xa] = 'P';
        strTmp[0xb] = 'o';
        strTmp[0xc] = 'r';
        strTmp[0xd] = 't';
        strTmp[0xe] = '\0';

        txt = new C_Text;
        txt->Setup(C_DONT_CARE, 0);
        txt->SetFont(0xd);
        txt->SetFGColor(0xffffff);
        txt->SetFlagBitOn(C_BIT_HCENTER);
        txt->SetClient(static_cast<short>(client));
        txt->SetXY(center, y);
        txt->SetText(gStringMgr->GetText(gStringMgr->AddText(strTmp)));
        win->AddControl(txt);
        y += gFontList->GetHeight(0xd) + 10;

        // add loke and fmbls

        strTmp[0x0] = 'e';
        strTmp[0x1] = 'R';
        strTmp[0x2] = 'A';
        strTmp[0x3] = 'Z';
        strTmp[0x4] = 'O';
        strTmp[0x5] = 'R';
        strTmp[0x6] = '\0';
        txt = new C_Text;
        txt->Setup(C_DONT_CARE, 0);
        txt->SetFont(1);
        txt->SetFGColor(0xffffff);
        txt->SetFlagBitOn(C_BIT_HCENTER);
        txt->SetXY(center, y);
        txt->SetClient(static_cast<short>(client));
        txt->SetText(gStringMgr->GetText(gStringMgr->AddText(strTmp)));
        win->AddControl(txt);
        y += gFontList->GetHeight(1);

        strTmp[0x0] = 'M';
        strTmp[0x1] = 'i';
        strTmp[0x2] = 'r';
        strTmp[0x3] = 'v';
        strTmp[0x4] = '\0';
        txt = new C_Text;
        txt->Setup(C_DONT_CARE, 0);
        txt->SetFont(1);
        txt->SetFGColor(0xffffff);
        txt->SetFlagBitOn(C_BIT_HCENTER);
        txt->SetXY(center, y);
        txt->SetClient(static_cast<short>(client));
        txt->SetText(gStringMgr->GetText(gStringMgr->AddText(strTmp)));
        win->AddControl(txt);
        y += gFontList->GetHeight(1);

        strTmp[0x0] = 'P';
        strTmp[0x1] = 'a';
        strTmp[0x2] = 'u';
        strTmp[0x3] = 'l';
        strTmp[0x4] = '1';
        strTmp[0x5] = '2';
        strTmp[0x6] = '1';
        strTmp[0x7] = '2';
        strTmp[0x8] = '\0';
        txt = new C_Text;
        txt->Setup(C_DONT_CARE, 0);
        txt->SetFont(1);
        txt->SetFGColor(0xffffff);
        txt->SetFlagBitOn(C_BIT_HCENTER);
        txt->SetXY(center, y);
        txt->SetClient(static_cast<short>(client));
        txt->SetText(gStringMgr->GetText(gStringMgr->AddText(strTmp)));
        win->AddControl(txt);
        y += gFontList->GetHeight(1);

        strTmp[0x0] = 'L';
        strTmp[0x1] = 'o';
        strTmp[0x2] = 'k';
        strTmp[0x3] = 'e';
        strTmp[0x4] = '\0';
        txt = new C_Text;
        txt->Setup(C_DONT_CARE, 0);
        txt->SetFont(1);
        txt->SetFGColor(0xffffff);
        txt->SetFlagBitOn(C_BIT_HCENTER);
        txt->SetXY(center, y);
        txt->SetClient(static_cast<short>(client));
        txt->SetText(gStringMgr->GetText(gStringMgr->AddText(strTmp)));
        win->AddControl(txt);
        y += gFontList->GetHeight(1);

        strTmp[0x0] = 'F';
        strTmp[0x1] = 'm';
        strTmp[0x2] = 'b';
        strTmp[0x3] = 'l';
        strTmp[0x4] = 's';
        strTmp[0x5] = '\0';
        txt = new C_Text;
        txt->Setup(C_DONT_CARE, 0);
        txt->SetFont(1);
        txt->SetFGColor(0xffffff);
        txt->SetFlagBitOn(C_BIT_HCENTER);
        txt->SetXY(center, y);
        txt->SetClient(static_cast<short>(client));
        txt->SetText(gStringMgr->GetText(gStringMgr->AddText(strTmp)));
        win->AddControl(txt);
        y += gFontList->GetHeight(1);

        strTmp[0x0] = 'L';
        strTmp[0x1] = 'a';
        strTmp[0x2] = 'w';
        strTmp[0x3] = 'n';
        strTmp[0x4] = 'd';
        strTmp[0x5] = 'a';
        strTmp[0x6] = 'r';
        strTmp[0x7] = 'd';
        strTmp[0x8] = '\0';
        txt = new C_Text;
        txt->Setup(C_DONT_CARE, 0);
        txt->SetFont(1);
        txt->SetFGColor(0xffffff);
        txt->SetFlagBitOn(C_BIT_HCENTER);
        txt->SetXY(center, y);
        txt->SetClient(static_cast<short>(client));
        txt->SetText(gStringMgr->GetText(gStringMgr->AddText(strTmp)));
        win->AddControl(txt);
        y += gFontList->GetHeight(1);
#endif
#endif
        //JAM

        y += win->GetH() / 2;

        txt = new C_Text;
        txt->Setup(C_DONT_CARE, 0);

        if (leg)
        {
            txt->SetFont(leg->FontID);
            txt->SetFGColor(leg->ColorID);
        }

        txt->SetFlagBitOn(C_BIT_HCENTER);
        txt->SetClient(static_cast<short>(client));
        txt->SetXY(center, y);
        txt->SetText(gStringMgr->GetText(gStringMgr->AddText("Fini")));
        win->AddControl(txt);
        win->ScanClientAreas();
        y += win->GetH() / 2;
        txt = new C_Text;
        txt->Setup(C_DONT_CARE, 0);

        if (leg)
        {
            txt->SetFont(leg->FontID);
            txt->SetFGColor(leg->ColorID);
        }

        txt->SetFlagBitOn(C_BIT_HCENTER);
        txt->SetClient(static_cast<short>(client));
        txt->SetXY(center, y);
        txt->SetText(gStringMgr->GetText(gStringMgr->AddText("")));
        win->AddControl(txt);
        win->ScanClientAreas();

        drawTimer = new C_TimerHook;
        drawTimer->Setup(C_DONT_CARE, C_TYPE_NORMAL);
        drawTimer->SetClient(static_cast<short>(client));
        drawTimer->SetUpdateCallback(ScrollTimerCB);
        drawTimer->SetReady(1);
        drawTimer->SetUserNumber(0, (win->GetH() - 5) - y);
        win->AddControl(drawTimer);
    }

    res->Cleanup();
    delete res;
}

// Delete file functions...

BOOL CheckSystemFiles(char *Name, char *Ext)
{
    char filename[MAX_PATH];
    short i;

    strcpy(filename, Name);
    strcat(filename, ".");
    strcat(filename, Ext);

    i = 0;

    while (DontDeleteList[i])
    {
        if ( not stricmp(DontDeleteList[i], filename))
            return(TRUE);

        i++;
    }

    return(FALSE);
}

BOOL DeleteAFile(char *Path, char *Name, char *Ext)
{
    char Filename[MAX_PATH];

    if ( not Name)
        return(FALSE);

    Filename[0] = 0;

    if (Path)
    {
        strcpy(Filename, Path);
        strcat(Filename, "\\");
    }

    strcat(Filename, Name);

    if (Ext)
    {
        strcat(Filename, ".");
        strcat(Filename, Ext);
    }

    if ( not CheckSystemFiles(Name, Ext))
    {
        DeleteFile(Filename);
        return(TRUE);
    }

    return(FALSE);
}

void DelSTRFileCB(long, short, C_Base *)
{
    C_TreeList *tree;
    C_Button *btn;
    TREELIST *item;

    tree = (C_TreeList*)gPopupMgr->GetCallingControl();

    if (tree)
    {
        item = tree->GetLastItem();

        if (item)
        {
            btn = (C_Button*)item->Item_;

            if (btn)
            {
                if (DeleteAFile(FalconCampUserSaveDirectory, btn->GetText(0), "str"))
                {
                    tree->DeleteItem(item);
                    tree->RecalcSize();
                    tree->Parent_->RefreshClient(tree->GetClient());
                }
            }
        }
    }

    gPopupMgr->CloseMenu();
}

void DelDFSFileCB(long, short, C_Base *)
{
    C_TreeList *tree;
    C_Button *btn;
    TREELIST *item;

    tree = (C_TreeList*)gPopupMgr->GetCallingControl();

    if (tree)
    {
        item = tree->GetLastItem();

        if (item)
        {
            btn = (C_Button*)item->Item_;

            if (btn)
            {
                if (DeleteAFile(FalconCampUserSaveDirectory, btn->GetText(0), "dfs"))
                {
                    tree->DeleteItem(item);
                    tree->RecalcSize();
                    tree->Parent_->RefreshClient(tree->GetClient());
                }
            }
        }
    }

    gPopupMgr->CloseMenu();
}

void DelLSTFileCB(long, short, C_Base *)
{
    C_TreeList *tree;
    C_Button *btn;
    TREELIST *item;

    tree = (C_TreeList*)gPopupMgr->GetCallingControl();

    if (tree)
    {
        item = tree->GetLastItem();

        if (item)
        {
            btn = (C_Button*)item->Item_;

            if (btn)
            {
                if (DeleteAFile(NULL, btn->GetText(0), "lst"))
                {
                    tree->DeleteItem(item);
                    tree->RecalcSize();
                    tree->Parent_->RefreshClient(tree->GetClient());
                }
            }
        }
    }

    gPopupMgr->CloseMenu();
}

void DelCamFileCB(long, short, C_Base *)
{
    C_TreeList *tree;
    C_Button *btn;
    TREELIST *item;

    tree = (C_TreeList*)gPopupMgr->GetCallingControl();

    if (tree)
    {
        item = tree->GetLastItem();

        if (item)
        {
            btn = (C_Button*)item->Item_;

            if (btn)
            {
                if (DeleteAFile(FalconCampUserSaveDirectory, btn->GetText(0), "cam"))
                {
                    tree->DeleteItem(item);
                    tree->RecalcSize();
                    tree->Parent_->RefreshClient(tree->GetClient());
                }
            }
        }
    }

    gPopupMgr->CloseMenu();
}

void DelTacFileCB(long, short, C_Base *)
{
    C_TreeList *tree;
    C_Button *btn;
    TREELIST *item;

    tree = (C_TreeList*)gPopupMgr->GetCallingControl();

    if (tree)
    {
        item = tree->GetLastItem();

        if (item)
        {
            btn = (C_Button*)item->Item_;

            if (btn)
            {
                if (DeleteAFile(FalconCampUserSaveDirectory, btn->GetText(0), "tac"))
                {
                    tree->DeleteItem(item);
                    tree->RecalcSize();
                    tree->Parent_->RefreshClient(tree->GetClient());
                }
            }
        }
    }

    gPopupMgr->CloseMenu();
}

void DelTGAFileCB(long, short, C_Base *)
{
    C_TreeList *tree;
    C_Button *btn;
    TREELIST *item;

    tree = (C_TreeList*)gPopupMgr->GetCallingControl();

    if (tree)
    {
        item = tree->GetLastItem();

        if (item)
        {
            btn = (C_Button*)item->Item_;

            if (btn)
            {
                if (DeleteAFile(NULL, btn->GetText(0), "tga"))
                {
                    tree->DeleteItem(item);
                    tree->RecalcSize();
                    tree->Parent_->RefreshClient(tree->GetClient());
                }
            }
        }
    }

    gPopupMgr->CloseMenu();
}

void DelVHSFileCB(long, short, C_Base *)
{
    C_TreeList *tree;
    C_Button *btn;
    TREELIST *item;

    tree = (C_TreeList*)gPopupMgr->GetCallingControl();

    if (tree)
    {
        item = tree->GetLastItem();

        if (item)
        {
            btn = (C_Button*)item->Item_;

            if (btn)
            {
                if (DeleteAFile("acmibin", btn->GetText(0), "vhs"))
                {
                    tree->DeleteItem(item);
                    tree->RecalcSize();
                    tree->Parent_->RefreshClient(tree->GetClient());
                }
            }
        }
    }

    gPopupMgr->CloseMenu();
}

void DelKeyFileCB(long, short, C_Base *)
{
    C_TreeList *tree;
    C_Button *btn;
    TREELIST *item;

    tree = (C_TreeList*)gPopupMgr->GetCallingControl();

    if (tree)
    {
        item = tree->GetLastItem();

        if (item)
        {
            btn = (C_Button*)item->Item_;

            if (btn)
            {
                if (DeleteAFile("config", btn->GetText(0), "key"))
                {
                    tree->DeleteItem(item);
                    tree->RecalcSize();
                    tree->Parent_->RefreshClient(tree->GetClient());
                }
            }
        }
    }

    gPopupMgr->CloseMenu();
}
