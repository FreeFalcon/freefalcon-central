#include "falclib.h"
#include "chandler.h"
#include "ui/include/textids.h"

extern bool g_bHiResUI; // M.N.
extern bool g_bLogUiErrors; // JPO

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

#include "campaign/include/cmpclass.h"
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

long UI_FILESIZE(UI_HANDLE fp)
{
    long size = 0;

    if (UI_SEEK(fp, 0l, SEEK_END))
        return(0);

    size = UI_TELL(fp);

    if (UI_SEEK(fp, 0l, SEEK_SET))
        return(0);

    return(size);
}
// ALL RESMGR CODE ADDITIONS AND END HERE

C_Hash *TokenErrorList = NULL;
FILE *errorfp;
extern char FalconUIArtDirectory[];
extern char FalconUIArtThrDirectory[];
extern char FalconUISoundDirectory[];
static char filebuf[_MAX_PATH];

#ifdef _UI95_PARSER_

//#define _LOG_ERRORS_

enum
{
    CPARSE_NOTHING = 0,
    CPARSE_WINDOW,
    CPARSE_BUTTON,
    CPARSE_TEXT,
    CPARSE_EDITBOX,
    CPARSE_LISTBOX,
    CPARSE_SCROLLBAR,
    CPARSE_TREELIST,
    CPARSE_FONT,
    CPARSE_IMAGE,
    CPARSE_BITMAP,
    CPARSE_LINE,
    CPARSE_BOX,
    CPARSE_MARQUE,
    CPARSE_ANIMATION,
    CPARSE_CURSOR,
    CPARSE_SOUND,
    CPARSE_SLIDER,
    CPARSE_POPUP,
    CPARSE_PANNER,
    CPARSE_ANIM,
    CPARSE_STRING,
    CPARSE_MOVIE,
    CPARSE_FILL,
    CPARSE_TREE,
    CPARSE_CLOCK,
    CPARSE_TILE,
    CPARSE_VERSIONTEXT
};

static char *C_All_Tokens[] =
{
    "[NOTHING]",
    "[WINDOW]",
    "[BUTTON]",
    "[TEXT]",
    "[EDITBOX]",
    "[LISTBOX]",
    "[SCROLLBAR]",
    "[TREELIST]",
    "[MAKEFONT]",
    "[IMAGE]",
    "[BITMAP]",
    "[LINE]",
    "[BOX]",
    "[MARQUE]",
    "[ANIMATION]",
    "[LOCATOR]",
    "[SOUND]",
    "[SLIDER]",
    "[POPUPMENU]",
    "[PANNER]",
    "[ANIM]",
    "[STRINGLIST]",
    "[MOVIE]",
    "[FILL]",
    "[TREELIST]",
    "[CLOCK]",
    "[TILE]",
    "[VERSIONTEXT]", //sfr
    0,
};

static ID_TABLE UI95_Table[] =
{
    {"NULL", NULL},
    {"NID", C_DONT_CARE},
    {"C_DONT_CARE", C_DONT_CARE},
    {"C_STATE_0", C_STATE_0},
    {"C_STATE_1", C_STATE_1},
    {"C_STATE_2", C_STATE_2},
    {"C_STATE_3", C_STATE_3},
    {"C_STATE_4", C_STATE_4},
    {"C_STATE_5", C_STATE_5},
    {"C_STATE_6", C_STATE_6},
    {"C_STATE_7", C_STATE_7},
    {"C_STATE_8", C_STATE_8},
    {"C_STATE_9", C_STATE_9},
    {"C_STATE_10", C_STATE_10},
    {"C_STATE_11", C_STATE_11},
    {"C_STATE_12", C_STATE_12},
    {"C_STATE_13", C_STATE_13},
    {"C_STATE_14", C_STATE_14},
    {"C_STATE_15", C_STATE_15},
    {"C_STATE_16", C_STATE_16},
    {"C_STATE_17", C_STATE_17},
    {"C_STATE_18", C_STATE_18},
    {"C_STATE_19", C_STATE_19},
    {"C_STATE_20", C_STATE_20},
    {"C_STATE_UP", C_STATE_0},
    {"C_STATE_DOWN", C_STATE_1},
    {"C_STATE_DISABLED", C_STATE_DISABLED},
    {"C_STATE_SELECTED", C_STATE_SELECTED},
    {"C_STATE_MOUSE", C_STATE_MOUSE},
    {"C_TYPE_NOTHING", C_TYPE_NOTHING},
    {"C_TYPE_NORMAL", C_TYPE_NORMAL},
    {"C_TYPE_TOGGLE", C_TYPE_TOGGLE},
    {"C_TYPE_SELECT", C_TYPE_SELECT},
    {"C_TYPE_RADIO", C_TYPE_RADIO},
    {"C_TYPE_CUSTOM", C_TYPE_CUSTOM},
    {"C_TYPE_SIZEX", C_TYPE_SIZEX},
    {"C_TYPE_SIZEY", C_TYPE_SIZEY},
    {"C_TYPE_SIZEXY", C_TYPE_SIZEXY},
    {"C_TYPE_SIZEW", C_TYPE_SIZEW},
    {"C_TYPE_SIZEH", C_TYPE_SIZEH},
    {"C_TYPE_SIZEWH", C_TYPE_SIZEWH},
    {"C_TYPE_DRAGX", C_TYPE_DRAGX},
    {"C_TYPE_DRAGY", C_TYPE_DRAGY},
    {"C_TYPE_DRAGXY", C_TYPE_DRAGXY},
    {"C_TYPE_TEXT", C_TYPE_TEXT},
    {"C_TYPE_PASSWORD", C_TYPE_PASSWORD},
    {"C_TYPE_INTEGER", C_TYPE_INTEGER},
    {"C_TYPE_FLOAT", C_TYPE_FLOAT},
    {"C_TYPE_FILENAME", C_TYPE_FILENAME},
    {"C_TYPE_MENU", C_TYPE_MENU},
    {"C_TYPE_LEFT", C_TYPE_LEFT},
    {"C_TYPE_CENTER", C_TYPE_CENTER},
    {"C_TYPE_RIGHT", C_TYPE_RIGHT},
    {"C_TYPE_ROOT", C_TYPE_ROOT},
    {"C_TYPE_INFO", C_TYPE_INFO},
    {"C_TYPE_ITEM", C_TYPE_ITEM},
    {"C_TYPE_LMOUSEDOWN", C_TYPE_LMOUSEDOWN},
    {"C_TYPE_LMOUSEUP", C_TYPE_LMOUSEUP},
    {"C_TYPE_LMOUSEDBLCLK", C_TYPE_LMOUSEDBLCLK},
    {"C_TYPE_RMOUSEDOWN", C_TYPE_RMOUSEDOWN},
    {"C_TYPE_RMOUSEUP", C_TYPE_RMOUSEUP},
    {"C_TYPE_RMOUSEDBLCLK", C_TYPE_RMOUSEDBLCLK},
    {"C_TYPE_MOUSEOVER", C_TYPE_MOUSEOVER},
    {"C_TYPE_MOUSEREPEAT", C_TYPE_MOUSEREPEAT},
    {"C_TYPE_EXCLUSIVE", C_TYPE_EXCLUSIVE},
    {"C_TYPE_MOUSEMOVE", C_TYPE_MOUSEMOVE},
    {"C_TYPE_VERTICAL", C_TYPE_VERTICAL},
    {"C_TYPE_HORIZONTAL", C_TYPE_HORIZONTAL},
    {"C_TYPE_LOOP", C_TYPE_LOOP},
    {"C_TYPE_STOPATEND", C_TYPE_STOPATEND},
    {"C_TYPE_PINGPONG", C_TYPE_PINGPONG},
    {"C_TYPE_TIMER", C_TYPE_TIMER},
    {"C_TYPE_TRANSLUCENT", C_TYPE_TRANSLUCENT},
    {"C_TYPE_IPADDRESS", C_TYPE_IPADDRESS},
    {NULL, -1},
};

static ID_TABLE UI95_BitTable[] =
{
    {"NULL", NULL},
    {"C_BIT_NOTHING", C_BIT_NOTHING},
    {"C_BIT_FIXEDSIZE", C_BIT_FIXEDSIZE},
    {"C_BIT_LEADINGZEROS", C_BIT_LEADINGZEROS},
    {"C_BIT_VERTICAL", C_BIT_VERTICAL},
    {"C_BIT_HORIZONTAL", C_BIT_HORIZONTAL},
    {"C_BIT_USEOUTLINE", C_BIT_USEOUTLINE},
    {"C_BIT_LEFT", C_BIT_LEFT},
    {"C_BIT_RIGHT", C_BIT_RIGHT},
    {"C_BIT_TOP", C_BIT_TOP},
    {"C_BIT_BOTTOM", C_BIT_BOTTOM},
    {"C_BIT_HCENTER", C_BIT_HCENTER},
    {"C_BIT_VCENTER", C_BIT_VCENTER},
    {"C_BIT_ENABLED", C_BIT_ENABLED},
    {"C_BIT_DRAGABLE", C_BIT_DRAGABLE},
    {"C_BIT_INVISIBLE", C_BIT_INVISIBLE},
    {"C_BIT_FORCEMOUSEOVER", C_BIT_FORCEMOUSEOVER},
    {"C_BIT_USEBGIMAGE", C_BIT_USEBGIMAGE},
    {"C_BIT_TIMER", C_BIT_TIMER},
    {"C_BIT_ABSOLUTE", C_BIT_ABSOLUTE},
    {"C_BIT_SELECTABLE", C_BIT_SELECTABLE},
    {"C_BIT_OPAQUE", C_BIT_OPAQUE},
    {"C_BIT_CANTMOVE", C_BIT_CANTMOVE},
    {"C_BIT_USELINE", C_BIT_USELINE},
    {"C_BIT_WORDWRAP", C_BIT_WORDWRAP},
    {"C_BIT_REMOVE", C_BIT_REMOVE},
    {"C_BIT_NOCLEANUP", C_BIT_NOCLEANUP},
    {"C_BIT_TRANSLUCENT", C_BIT_TRANSLUCENT},
    {"C_BIT_USEBGFILL", C_BIT_USEBGFILL},
    {"C_BIT_MOUSEOVER", C_BIT_MOUSEOVER},
    {"C_BIT_NOLABEL", C_BIT_NOLABEL},
    {NULL, -1}, // LAST Record in this list
};

static ID_TABLE UI95_FontTable[] =
{
    {"FALSE", FALSE},
    {"TRUE", TRUE},
    {"FW_DONTCARE", FW_DONTCARE},
    {"FW_THIN", FW_THIN},
    {"FW_EXTRALIGHT", FW_EXTRALIGHT},
    {"FW_ULTRALIGHT", FW_ULTRALIGHT},
    {"FW_LIGHT", FW_LIGHT},
    {"FW_NORMAL", FW_NORMAL},
    {"FW_REGULAR", FW_REGULAR},
    {"FW_MEDIUM", FW_MEDIUM},
    {"FW_SEMIBOLD", FW_SEMIBOLD},
    {"FW_DEMIBOLD", FW_DEMIBOLD},
    {"FW_BOLD", FW_BOLD},
    {"FW_EXTRABOLD", FW_EXTRABOLD},
    {"FW_ULTRABOLD", FW_ULTRABOLD},
    {"FW_HEAVY", FW_HEAVY},
    {"FW_BLACK", FW_BLACK},
    {"ANSI_CHARSET", ANSI_CHARSET},
    {"DEFAULT_CHARSET", DEFAULT_CHARSET},
    {"SYMBOL_CHARSET", SYMBOL_CHARSET},
    {"SHIFTJIS_CHARSET", SHIFTJIS_CHARSET},
    {"GB2312_CHARSET", GB2312_CHARSET},
    {"HANGEUL_CHARSET", HANGEUL_CHARSET},
    {"CHINESEBIG5_CHARSET", CHINESEBIG5_CHARSET},
    {"OEM_CHARSET", OEM_CHARSET},
    {"JOHAB_CHARSET", JOHAB_CHARSET},
    {"HEBREW_CHARSET", HEBREW_CHARSET},
    {"ARABIC_CHARSET", ARABIC_CHARSET},
    {"GREEK_CHARSET", GREEK_CHARSET},
    {"TURKISH_CHARSET", TURKISH_CHARSET},
    {"THAI_CHARSET", THAI_CHARSET},
    {"EASTEUROPE_CHARSET", EASTEUROPE_CHARSET},
    {"RUSSIAN_CHARSET", RUSSIAN_CHARSET},
    {"MAC_CHARSET", MAC_CHARSET},
    {"BALTIC_CHARSET", BALTIC_CHARSET},
    {"OUT_CHARACTER_PRECIS", OUT_CHARACTER_PRECIS},
    {"OUT_DEFAULT_PRECIS", OUT_DEFAULT_PRECIS},
    {"OUT_DEVICE_PRECIS", OUT_DEVICE_PRECIS},
    {"OUT_OUTLINE_PRECIS", OUT_OUTLINE_PRECIS},
    {"OUT_RASTER_PRECIS", OUT_RASTER_PRECIS},
    {"OUT_STRING_PRECIS", OUT_STRING_PRECIS},
    {"OUT_STROKE_PRECIS", OUT_STROKE_PRECIS},
    {"OUT_TT_ONLY_PRECIS", OUT_TT_ONLY_PRECIS},
    {"OUT_TT_PRECIS", OUT_TT_PRECIS},
    {"CLIP_DEFAULT_PRECIS", CLIP_DEFAULT_PRECIS},
    {"CLIP_CHARACTER_PRECIS", CLIP_CHARACTER_PRECIS},
    {"CLIP_STROKE_PRECIS", CLIP_STROKE_PRECIS},
    {"CLIP_MASK", CLIP_MASK},
    {"CLIP_EMBEDDED", CLIP_EMBEDDED},
    {"CLIP_LH_ANGLES", CLIP_LH_ANGLES},
    {"CLIP_TT_ALWAYS", CLIP_TT_ALWAYS},
    {"DEFAULT_QUALITY", DEFAULT_QUALITY},
    {"DRAFT_QUALITY", DRAFT_QUALITY},
    {"PROOF_QUALITY", PROOF_QUALITY},
    {"DEFAULT_PITCH", DEFAULT_PITCH},
    {"FIXED_PITCH", FIXED_PITCH},
    {"VARIABLE_PITCH", VARIABLE_PITCH},
    {"FF_DECORATIVE", FF_DECORATIVE},
    {"FF_DONTCARE", FF_DONTCARE},
    {"FF_MODERN", FF_MODERN},
    {"FF_ROMAN", FF_ROMAN},
    {"FF_SCRIPT", FF_SCRIPT},
    {"FF_SWISS", FF_SWISS},
    { NULL, -1},
};

#define _START_BASE_ID_ 3500000

char ParseSave[80];
char ParseCRLF[3] = {13, 10, 0};
extern char FalconUIArtDirectory[];
extern char FalconUISoundDirectory[];

C_Parser::C_Parser()
{
    Idx_ = 0; // index into script
    script_ = NULL; // script file (read into memory)
    scriptlen_ = 0;
    tokenlen_ = 0;
    P_Idx_ = 0;
    str_ = NULL;

    Perror_ = NULL;

    Handler_ = NULL;
    Window_ = NULL;
    Control_ = NULL;
    Anim_ = NULL;
    Font_ = NULL;
    Image_ = NULL;
    Anim_ = NULL;
    Sound_ = NULL;
    Popup_ = NULL;
    String_ = NULL;

    TokenOrder_ = NULL;

    memset(&P_[0], 0, sizeof(long)*PARSE_MAX_PARAMS);
}

C_Parser::~C_Parser()
{
    if (script_)
        delete script_;
}

void C_Parser::Setup(C_Handler *handler, C_Image *ImgMgr, C_Font *FontList, C_Sound *SndMgr, C_PopupMgr *PopupMgr, C_Animation *AnimMgr, C_String *StringMgr, C_Movie *MovieMgr)
{
    Handler_ = handler;
    Image_ = ImgMgr;
    Anim_ = AnimMgr;
    Font_ = FontList;
    Sound_ = SndMgr;
    Popup_ = PopupMgr;
    String_ = StringMgr;
    Movie_ = MovieMgr;

    TokenOrder_ = new C_Hash;
    TokenOrder_->Setup(PARSE_HASH_SIZE);
    TokenOrder_->SetFlags(C_BIT_REMOVE);

    TokenErrorList = new C_Hash;
    TokenErrorList->Setup(512);
    TokenErrorList->SetFlags(C_BIT_REMOVE);

    AddInternalIDs(UI95_BitTable);
    AddInternalIDs(UI95_Table);
    AddInternalIDs(UI95_FontTable);

    Sound_->SetIDTable(TokenOrder_);

    if (g_bLogUiErrors)
    {
        Perror_ = UI_OPEN("ui95err.log", "a");

        if (Perror_)
            fprintf(Perror_, "Setup Parser\n");
    }
}

void C_Parser::Cleanup()
{

    if (g_bLogUiErrors)
    {

        if (Perror_)
            fprintf(Perror_, "Cleanup Parser\n");

        if (Perror_)
            UI_CLOSE(Perror_);
    }

    Perror_ = NULL;

    TokenOrder_->Cleanup();
    delete TokenOrder_;
    TokenOrder_ = NULL;

    TokenErrorList->Cleanup();
    delete TokenErrorList;
    TokenErrorList = NULL;

    Handler_ = NULL;
    Window_ = NULL;
    Control_ = NULL;
    Font_ = NULL;
    Image_ = NULL;
    Anim_ = NULL;
    Sound_ = NULL;
    String_ = NULL;
}

void C_Parser::AddInternalIDs(ID_TABLE tbl[])
{
    short i;

    i = 0;

    while (tbl[i].Label[0])
    {

        TokenOrder_->AddTextID(tbl[i].Value, tbl[i].Label);
        i++;
    }
}

long C_Parser::TokenizeIDs(char *idfile, long size)
{
    long idx, count, expecting;

    idx = 0;

    // remove ALL white space
    while (idx < size)
    {
        if (idfile[idx] <= ' ' or idfile[idx] == ',')
            idfile[idx] = 0;

        idx++;
    }

    idx = 0;
    count = 0;
    expecting = 0;

    // expecting: 0-looking for ID,1-looking for end of id
    //            2-looking for value,3-looking for end of value

    while (idx <= size)
    {
        if ( not idfile[idx])
        {
            if (expecting == 1)
                expecting = 2;
            else if (expecting == 3)
            {
                expecting = 0;
                count++;
            }
        }
        else
        {
            if (expecting == 0)
            {
                if (isdigit(idfile[idx]) or idfile[idx] == '-')
                    idfile[idx] = 0;
                else
                    expecting = 1;
            }
            else if (expecting == 2)
            {
                if (isdigit(idfile[idx]) or idfile[idx] == '-')
                    expecting = 3;
                else
                    idfile[idx] = 0;
            }
            else if (expecting == 3)
            {
                if ( not isdigit(idfile[idx]))
                {
                    idfile[idx] = 0;
                    count++;
                    expecting = 0;
                }
            }
        }

        idx++;
    }

    return(count);
}

void C_Parser::LoadIDTable(char *filename)
{
    UI_HANDLE ifp;
    long size;
    long count, i, idx;
    char *idfile;
    char *token;
    long ID;

    // ifp=UI_OPEN(filename,"rb");
    ifp = OpenArtFile(filename, FalconUIArtThrDirectory, FalconUIArtDirectory, 0);

    if (ifp == NULL)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadIDTable load failed (%s)\n", filename);
        }
    }

    size = UI_FILESIZE(ifp);

    if ( not size)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadIDTable seek end failed (%s)\n", filename);
        }

        UI_CLOSE(ifp);
    }

    idfile = new char [size + 5]; // just in case :)

    if (UI_READ(idfile, size, 1, ifp) not_eq 1)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadIDTable read failed (%s)\n", filename);
        }

        UI_CLOSE(ifp);
        delete idfile;
        return;
    }

    idfile[size] = 0;

    UI_CLOSE(ifp);

    count = TokenizeIDs(idfile, size);

    if (count)
    {
        i = 0;
        idx = 0;

        while (i < count)
        {
            while ( not idfile[idx] and idx < size)
                idx++;

            token = &idfile[idx];

            while (idfile[idx] and idx < size)
                idx++;

            while ( not idfile[idx] and idx < size)
                idx++;

            ID = atol(&idfile[idx]);

            while (idfile[idx] and idx < size)
                idx++;

            TokenOrder_->AddTextID(ID, token);
            i++;
        }
    }

    delete idfile;
}

void C_Parser::LoadIDList(char *filename)
{
    UI_HANDLE ifp;
    long size;
    char *listfile, *lfp;
    long i;

    memset(&WindowList_[0], 0, sizeof(long)*MAX_WINDOWS_IN_LIST);
    WinIndex_ = 0;
    WinLoaded_ = 0;

#if 0
    char filebuf[_MAX_PATH];
    strcpy(filebuf, FalconUIArtDirectory); // FreeFalcon root

    if (g_bHiResUI)
        strcat(filebuf, "\\art1024"); // HiResUI
    else
        strcat(filebuf, "\\art"); // LoResUI

    strcat(filebuf, "\\");
    strcat(filebuf, filename);
    ifp = UI_OPEN(filebuf, "rb");
#endif
    ifp = OpenArtFile(filename, FalconUIArtThrDirectory, FalconUIArtDirectory);

    if (ifp == NULL)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadIDTable read failed (%s)\n", filename);
        }

        return;
    }

    size = UI_FILESIZE(ifp);

    if ( not size)
    {
        UI_CLOSE(ifp);
        return;
    }

    listfile = new char [size + 5]; // just in case :)

    if (UI_READ(listfile, size, 1, ifp) not_eq 1)
    {
        delete listfile;
        UI_CLOSE(ifp);
        return;
    }

    listfile[size] = 0;

    UI_CLOSE(ifp);

    for (i = 0; i < size; i++)
        if (listfile[i] < 32)
            listfile[i] = 0;

    lfp = listfile;
    i = 0;

    while (i < size)
    {
        while ( not (*lfp) and i < size)
        {
            lfp++;
            i++;
        }

        if (*lfp)
        {
            if (*lfp not_eq '#')
            {
                //strcpy(filebuf,FalconUIArtDirectory);
                //strcat(filebuf,"\\");
                //strcat(filebuf,lfp);
                LoadIDTable(lfp);
            }

            while ((*lfp) and i < size)
            {
                lfp++;
                i++;
            }
        }
    }

    delete listfile;
}

long C_Parser::FindID(char *token)
{
    return(TokenOrder_->FindTextID(token));
}

long C_Parser::FindToken(char *token)
{
    long i;

    i = 0;

    while (C_All_Tokens[i])
    {
        if ( not strnicmp(token, C_All_Tokens[i], strlen(C_All_Tokens[i])))
            return(i);

        i++;
    }

    return(0);
}

BOOL C_Parser::LoadScript(char *filename)
{
    UI_HANDLE ifp;
    long size;

    ifp = OpenArtFile(filename, FalconUIArtThrDirectory, FalconUIArtDirectory, 0);

    // ifp=UI_OPEN(filename,"rb");
    if (ifp == NULL)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadScript load failed (%s)\n", filename);
        }

        return(FALSE);
    }

    size = UI_FILESIZE(ifp);

    if ( not size)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadScript seek start failed (%s)\n", filename);
        }

        UI_CLOSE(ifp);
        return(FALSE);
    }

    scriptlen_ = size;

    if (script_)
        delete script_;

    script_ = new char [size + 5]; // just in case :)

    if (script_) memset(script_, 0, size + 5); // OW

    if (UI_READ(script_, size, 1, ifp) not_eq 1)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadScript read failed (%s)\n", filename);
        }

        delete script_;
        UI_CLOSE(ifp);
        return(FALSE);
    }

    script_[size] = 0;

    UI_CLOSE(ifp);
    return(TRUE);
}

UI_HANDLE C_Parser::OpenArtFile(char *filename, const char *thrdir, const char *maindir, int hirescapable)
{
    UI_HANDLE ifp;

    // absolute path
    if (isalpha(filename[0]) and filename[1] == ':' and filename[2] == '\\')
    {
        return UI_OPEN(filename, "rb");
    }

    // try theater first
    strcpy(filebuf, thrdir); // FreeFalcon thr root dir

    if (hirescapable)
    {
        // sfr: unset for old UI
#define NIGHTFALCON_UI 1

#if NIGHTFALCON_UI
        strcat(filebuf, "\\art");
#else

        if (g_bHiResUI)
        {
            strcat(filebuf, "\\art1024"); // HiResUI
        }
        else
        {
            strcat(filebuf, "\\art"); // LoResUI
        }

#endif
    }

    strcat(filebuf, "\\");
    strcat(filebuf, filename);
    ifp = UI_OPEN(filebuf, "rb");

    if (ifp not_eq NULL)
    {
        return ifp;    // got the main one
    }

    // try main dir
    strcpy(filebuf, maindir); // FreeFalcon main root dir

    if (hirescapable)
    {
#if NIGHTFALCON_UI
        strcat(filebuf, "\\art");
#else

        if (g_bHiResUI)
        {
            strcat(filebuf, "\\art1024"); // HiResUI
        }
        else
        {
            strcat(filebuf, "\\art"); // LoResUI
        }

#endif
    }

    strcat(filebuf, "\\");
    strcat(filebuf, filename);
    return UI_OPEN(filebuf, "rb");
}

BOOL C_Parser::LoadWindowList(char *filename)
{
    UI_HANDLE ifp;
    long size;
    char *listfile, *lfp;
    long i;
    C_Window *win;

    memset(&WindowList_[0], 0, sizeof(long)*MAX_WINDOWS_IN_LIST);
    WinIndex_ = 0;
    WinLoaded_ = 0;

    if (g_bLogUiErrors)
    {
        if (Perror_)
            fprintf(Perror_, "LoadWindowList processing (%s)\n", filename);
    }

    ifp = OpenArtFile(filename, FalconUIArtThrDirectory, FalconUIArtDirectory);

    if (ifp == NULL)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadWindowList open failed (%s)\n", filename);
        }

        return(FALSE);
    }

    if (g_bLogUiErrors)
    {
        if (Perror_)
            fprintf(Perror_, "Open Art file found as %s\n", filebuf);
    }


    size = UI_FILESIZE(ifp);

    if ( not size)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadWindowList seek start failed (%s)\n", filename);
        }

        UI_CLOSE(ifp);
        return(FALSE);
    }

    listfile = new char [size + 5]; // just in case :)

    if (UI_READ(listfile, size, 1, ifp) not_eq 1)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadWindowList read failed (%s)\n", filename);
        }

        delete listfile;
        UI_CLOSE(ifp);
        return(FALSE);
    }

    listfile[size] = 0;

    UI_CLOSE(ifp);

    for (i = 0; i < size; i++)
        if (listfile[i] < 32)
            listfile[i] = 0;

    lfp = listfile;
    i = 0;

    while (i < size)
    {
        while ( not (*lfp) and i < size)
        {
            lfp++;
            i++;
        }

        if (*lfp)
        {
            if (g_bLogUiErrors)
            {
                if (Perror_)
                    fprintf(Perror_, "LoadWindowList Parsing Window (%s)\n", lfp);
            }

            if (*lfp not_eq '#')
            {
                //strcpy(filebuf,FalconUIArtDirectory);
                //strcat(filebuf,"\\");
                //strcat(filebuf,lfp);
                win = ParseWindow(lfp);

                if (win)
                {
                    WindowList_[WinLoaded_ ++] = win->GetID();
                    Handler_->AddWindow(win, win->GetFlags());
                    win->ScanClientAreas();
                }
                else
                {

                    if (g_bLogUiErrors)
                    {

                        if (Perror_)
                        {
                            fprintf(Perror_, "LoadWindowList NO Window returned (%s)\n", lfp);
                        }
                    }
                }
            }

            while ((*lfp) and i < size)
            {
                lfp++;
                i++;
            }
        }
    }

    delete listfile;
    return(TRUE);
}

BOOL C_Parser::LoadPopupMenuList(char *filename)
{
    UI_HANDLE ifp;
    long size;
    char *listfile, *lfp;
    long i;
    C_PopupList *Menu;

    if (g_bLogUiErrors)
    {
        if (Perror_)
            fprintf(Perror_, "LoadPopupMenuList processing (%s)\n", filename);
    }

#if 0
    strcpy(filebuf, FalconUIArtDirectory);
    strcat(filebuf, "\\");
    strcat(filebuf, filename);
    ifp = UI_OPEN(filebuf, "rb");
#endif
    ifp = OpenArtFile(filename, FalconUIArtThrDirectory, FalconUIArtDirectory, 0);

    if (ifp == NULL)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadPopupMenuList open failed (%s)\n", filename);
        }

        return(FALSE);
    }

    size = UI_FILESIZE(ifp);

    if ( not size)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadPopupMenuList seek start failed (%s)\n", filename);
        }

        UI_CLOSE(ifp);
        return(FALSE);
    }

    listfile = new char [size + 5]; // just in case :)

    if (UI_READ(listfile, size, 1, ifp) not_eq 1)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadPopupMenuList read failed (%s)\n", filename);
        }

        delete listfile;
        UI_CLOSE(ifp);
        return(FALSE);
    }

    listfile[size] = 0;

    UI_CLOSE(ifp);

    for (i = 0; i < size; i++)
        if (listfile[i] < 32)
            listfile[i] = 0;

    lfp = listfile;
    i = 0;

    while (i < size)
    {
        while ( not (*lfp) and i < size)
        {
            lfp++;
            i++;
        }

        if (*lfp)
        {
            if (g_bLogUiErrors)
            {
                if (Perror_)
                    fprintf(Perror_, "LoadPopupMenuList Parsing PopMenu (%s)\n", lfp);
            }

            //strcpy(filebuf,FalconUIArtDirectory);
            //strcat(filebuf,"\\");
            //strcat(filebuf,lfp);
            Menu = (C_PopupList *)ParsePopupMenu(lfp);

            if (Menu)
                Popup_->AddMenu(Menu);

            else
            {
                if (g_bLogUiErrors)
                {
                    if (Perror_)
                        fprintf(Perror_, "LoadPopupMenuList NO Popup Menu returned (%s)\n", lfp);
                }
            }

            while ((*lfp) and i < size)
            {
                lfp++;
                i++;
            }
        }
    }

    delete listfile;
    return(TRUE);
}

BOOL C_Parser::LoadImageList(char *filename)
{
    UI_HANDLE ifp;
    long size;
    char *listfile, *lfp;
    long i;

#if 0
    strcpy(filebuf, FalconUIArtDirectory); // FreeFalcon root

    if (g_bHiResUI)
        strcat(filebuf, "\\art1024"); // HiResUI
    else
        strcat(filebuf, "\\art"); // LoResUI

    strcat(filebuf, "\\");
    strcat(filebuf, filename);
    ifp = UI_OPEN(filebuf, "rb");
#endif
    ifp = OpenArtFile(filename, FalconUIArtThrDirectory, FalconUIArtDirectory);

    if (ifp == NULL)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadImageList open failed (%s)\n", filename);
        }

        return(FALSE);
    }

    size = UI_FILESIZE(ifp);

    if ( not size)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadImageList seek start failed (%s)\n", filename);
        }

        UI_CLOSE(ifp);
        return(FALSE);
    }

    listfile = new char [size + 5]; // just in case :)

    if (UI_READ(listfile, size, 1, ifp) not_eq 1)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadImageList read failed (%s)\n", filename);
        }

        delete listfile;
        UI_CLOSE(ifp);
        return(FALSE);
    }

    listfile[size] = 0;

    UI_CLOSE(ifp);

    for (i = 0; i < size; i++)
        if (listfile[i] < 32)
            listfile[i] = 0;

    lfp = listfile;
    i = 0;

    while (i < size)
    {
        while ( not (*lfp) and i < size)
        {
            lfp++;
            i++;
        }

        if (*lfp)
        {
            //strcpy(filebuf,FalconUIArtDirectory);
            //strcat(filebuf,"\\");
            //strcat(filebuf,lfp);
            ParseImage(lfp);

            while ((*lfp) and i < size)
            {
                lfp++;
                i++;
            }
        }
    }

    delete listfile;
    return(TRUE);
}

BOOL C_Parser::LoadSoundList(char *filename)
{
    UI_HANDLE ifp;
    long size;
    char *listfile, *lfp;
    long i;

#if 0
    strcpy(filebuf, FalconUIArtDirectory); // FreeFalcon root

    if (g_bHiResUI)
        strcat(filebuf, "\\art1024"); // HiResUI
    else
        strcat(filebuf, "\\art"); // LoResUI

    strcat(filebuf, "\\");
    strcat(filebuf, filename);
    ifp = UI_OPEN(filebuf, "rb");
#endif
    ifp = OpenArtFile(filename, FalconUIArtThrDirectory, FalconUIArtDirectory);

    if (ifp == NULL)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadSoundList open failed (%s)\n", filename);
        }

        return(FALSE);
    }

    size = UI_FILESIZE(ifp);

    if ( not size)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadSoundList seek start failed (%s)\n", filename);
        }

        UI_CLOSE(ifp);
        return(FALSE);
    }

    listfile = new char [size + 5]; // just in case :)

    if (UI_READ(listfile, size, 1, ifp) not_eq 1)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadSoundList read failed (%s)\n", filename);
        }

        delete listfile;
        UI_CLOSE(ifp);
        return(FALSE);
    }

    listfile[size] = 0;

    UI_CLOSE(ifp);

    for (i = 0; i < size; i++)
        if (listfile[i] < 32)
            listfile[i] = 0;

    lfp = listfile;
    i = 0;

    while (i < size)
    {
        while ( not (*lfp) and i < size)
        {
            lfp++;
            i++;
        }

        if (*lfp)
        {
            char filebuf[_MAX_PATH];
            strcpy(filebuf, FalconUISoundDirectory);
            strcat(filebuf, "\\");
            strcat(filebuf, lfp);
            ParseSound(filebuf);

            while ((*lfp) and i < size)
            {
                lfp++;
                i++;
            }
        }
    }

    delete listfile;
    return(TRUE);
}

BOOL C_Parser::LoadStringList(char *filename)
{
    UI_HANDLE ifp;
    long size;
    char *listfile, *lfp;
    long i;

#if 0
    strcpy(filebuf, FalconUIArtDirectory); // FreeFalcon root

    if (g_bHiResUI)
        strcat(filebuf, "\\art1024"); // HiResUI
    else
        strcat(filebuf, "\\art"); // LoResUI

    strcat(filebuf, "\\");
    strcat(filebuf, filename);
    ifp = UI_OPEN(filebuf, "rb");
#endif
    ifp = OpenArtFile(filename, FalconUIArtThrDirectory, FalconUIArtDirectory);

    if (ifp == NULL)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadStringList open failed (%s)\n", filename);
        }

        return(FALSE);
    }

    size = UI_FILESIZE(ifp);

    if ( not size)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadStringList seek start failed (%s)\n", filename);
        }

        UI_CLOSE(ifp);
        return(FALSE);
    }

    listfile = new char [size + 5]; // just in case :)

    if (UI_READ(listfile, size, 1, ifp) not_eq 1)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadStringList read failed (%s)\n", filename);
        }

        delete listfile;
        UI_CLOSE(ifp);
        return(FALSE);
    }

    listfile[size] = 0;

    UI_CLOSE(ifp);

    for (i = 0; i < size; i++)
        if (listfile[i] < 32)
            listfile[i] = 0;

    lfp = listfile;
    i = 0;

    while (i < size)
    {
        while ( not (*lfp) and i < size)
        {
            lfp++;
            i++;
        }

        if (*lfp)
        {
            //strcpy(filebuf,FalconUIArtDirectory);
            //strcat(filebuf,"\\");
            //strcat(filebuf,lfp);
            ParseString(lfp);

            while ((*lfp) and i < size)
            {
                lfp++;
                i++;
            }
        }
    }

    delete listfile;
    return(TRUE);
}

BOOL C_Parser::LoadMovieList(char *filename)
{
    UI_HANDLE ifp;
    long size;
    char *listfile, *lfp;
    long i;

#if 0
    strcpy(filebuf, FalconUIArtDirectory); // FreeFalcon root

    if (g_bHiResUI)
        strcat(filebuf, "\\art1024"); // HiResUI
    else
        strcat(filebuf, "\\art"); // LoResUI

    strcat(filebuf, "\\");
    strcat(filebuf, filename);
    ifp = UI_OPEN(filebuf, "rb");
#endif
    ifp = OpenArtFile(filename, FalconUIArtThrDirectory, FalconUIArtDirectory);

    if (ifp == NULL)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadMovieList open failed (%s)\n", filename);
        }

        return(FALSE);
    }

    size = UI_FILESIZE(ifp);

    if ( not size)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadMovieList seek start failed (%s)\n", filename);
        }

        UI_CLOSE(ifp);
        return(FALSE);
    }

    listfile = new char [size + 5]; // just in case :)

    if (UI_READ(listfile, size, 1, ifp) not_eq 1)
    {
        if (g_bLogUiErrors)
        {
            if (Perror_)
                fprintf(Perror_, "LoadMovieList read failed (%s)\n", filename);
        }

        delete listfile;
        UI_CLOSE(ifp);
        return(FALSE);
    }

    listfile[size] = 0;

    UI_CLOSE(ifp);

    for (i = 0; i < size; i++)
        if (listfile[i] < 32)
            listfile[i] = 0;

    lfp = listfile;
    i = 0;

    while (i < size)
    {
        while ( not (*lfp) and i < size)
        {
            lfp++;
            i++;
        }

        if (*lfp)
        {
            // RV - Biker - Theater switching stuff
            char filebuf[_MAX_PATH];
            strcpy(filebuf, FalconUISoundDirectory);
            strcat(filebuf, "\\");
            strcat(filebuf, lfp);
            ParseMovie(filebuf);

            //ParseMovie(lfp); // Don't tack on movie directory... handled by PlayMovie function
            while ((*lfp) and i < size)
            {
                lfp++;
                i++;
            }
        }
    }

    delete listfile;
    return(TRUE);
}

enum
{
    SECTION_FINDTOKEN = 0,
    SECTION_PROCESSTOKEN,
    SECTION_FINDSUBTOKEN,
    SECTION_PROCESSSUBTOKEN,
    SECTION_FINDPARAMS,
    SECTION_PROCESSPARAMS,
};

enum
{
    TOKEN_NOTHING = 0,
    TOKEN_WINDOW,
    TOKEN_COMMON,
    TOKEN_LOCAL,
    TOKEN_FONT,
    TOKEN_IMAGE,
    TOKEN_SOUND,
    TOKEN_STRING,
};

BOOL C_Parser::ParseScript(char *filename)
{
    int Done, Comment, Found, InString; //,Finished=0;;
    int TokenID, Section, TokenType;

    if (LoadScript(filename) == FALSE)
        return(FALSE);

    Idx_ = 0;
    P_Idx_ = 0;
    tokenlen_ = 0;

    Done = 0;
    Comment = 0;
    InString = 0;
    Section = SECTION_FINDTOKEN;
    TokenType = TOKEN_NOTHING;

    while ( not Done)
    {
        switch (Section)
        {
            case SECTION_FINDTOKEN:
                // Look for token starting with '['
                Found = 0;

                while ( not Found and not Done)
                {
                    switch (script_[Idx_])
                    {
                        case '[':
                            if ( not Comment and not InString)
                            {
                                Found = 1;
                                break;
                            }

                            Idx_++;
                            break;

                        case '"':
                            InString = 1 - InString;
                            Idx_++;
                            break;

                        case '#':
                            Comment = 1;
                            Idx_++;
                            break;

                        case 0x0a:
                        case 0x0d:
                            Comment = 0;
                            Idx_++;
                            break;

                        default:
                            Idx_++;
                    }

                    if (Idx_ >= scriptlen_)
                        Done = 1;
                }

                tokenlen_ = 1;

                while (script_[Idx_ + tokenlen_] not_eq ']' and (Idx_ + tokenlen_) < scriptlen_)
                    tokenlen_++;

                tokenlen_++;

                if ((Idx_ + tokenlen_) >= scriptlen_)
                {
                    Done = 1;
                    break;
                }

                if (Found == 1)
                    Section = SECTION_PROCESSTOKEN;

                break;

            case SECTION_PROCESSTOKEN:
                TokenID = FindToken(&script_[Idx_]);

                if (TokenID)
                {
                    Section = SECTION_FINDSUBTOKEN;

                    switch (TokenID)
                    {
                        case CPARSE_WINDOW:
                            Handler_->AddWindow(WindowParser(), C_BIT_NOTHING);
                            break;

                        case CPARSE_FONT:
                            TokenType = TOKEN_FONT;
                            Idx_ += tokenlen_;
                            tokenlen_ = 0;
                            break;

                        case CPARSE_SOUND:
                            TokenType = TOKEN_SOUND;
                            Idx_ += tokenlen_;
                            tokenlen_ = 0;
                            break;

                        case CPARSE_STRING:
                            TokenType = TOKEN_STRING;
                            Idx_ += tokenlen_;
                            tokenlen_ = 0;
                            break;

                        default:
                            Section = SECTION_FINDTOKEN;
                            Idx_ += tokenlen_;
                            tokenlen_ = 0;
                            break;
                    }
                }
                else
                {
                    Section = SECTION_FINDTOKEN;
                    Idx_ += tokenlen_;
                    tokenlen_ = 0;
                }

                break;
        }
    }

    return(TRUE);
}

char *C_Parser::FindIDStr(long ID)
{
    sprintf(ValueStr, "%1ld", ID);
    return(&ValueStr[0]);
}

long C_Parser::AddNewID(char *label, long)
{
    return(TokenOrder_->AddText(label));
}

C_Base *C_Parser::ControlParser()
{
    long Done = 0, Comment = 0, Found = 0, InString = 0, Finished = 0;
    long TokenID = 0, Section = 0, TokenType = 0;

    Done = 0;
    Comment = 0;
    InString = 0;
    Control_ = NULL;
    Section = SECTION_PROCESSTOKEN;
    TokenType = TOKEN_NOTHING;

    while ( not Done)
    {
        switch (Section)
        {
            case SECTION_FINDTOKEN:
                // Look for token starting with '['
                Found = 0;

                while ( not Found and not Done)
                {
                    switch (script_[Idx_])
                    {
                        case '[':
                            if ( not Comment and not InString)
                            {
                                Found = 1;
                                break;
                            }

                            Idx_++;
                            break;

                        case '"':
                            InString = 1 - InString;
                            Idx_++;
                            break;

                        case '#':
                            Comment = 1;
                            Idx_++;
                            break;

                        case 0x0a:
                        case 0x0d:
                            Comment = 0;
                            Idx_++;
                            break;

                        default:
                            Idx_++;
                    }

                    if (Idx_ >= scriptlen_)
                        Done = 1;
                }

                tokenlen_ = 0;

                while (script_[Idx_ + tokenlen_] not_eq ']' and (Idx_ + tokenlen_) < scriptlen_)
                    tokenlen_++;

                tokenlen_++;

                if ((Idx_ + tokenlen_) >= scriptlen_)
                {
                    Done = 1;
                    break;
                }

                if (Found == 1)
                    Section = SECTION_PROCESSTOKEN;

                break;

            case SECTION_PROCESSTOKEN:
                TokenID = FindToken(&script_[Idx_]);

                if (TokenID)
                {
                    Section = SECTION_FINDSUBTOKEN;

                    switch (TokenID)
                    {
                        case CPARSE_WINDOW:
                            Done = 1;
                            break;

                        case CPARSE_BUTTON:
                            TokenType = TOKEN_COMMON;

                            if (Control_) delete Control_;

                            Control_ = new C_Button;
                            break;

                        case CPARSE_TEXT:
                            TokenType = TOKEN_COMMON;

                            if (Control_) delete Control_;

                            Control_ = new C_Text;
                            break;

                        case CPARSE_VERSIONTEXT:
                            TokenType = TOKEN_COMMON;

                            if (Control_) delete Control_;

                            Control_ = new C_VersionText;
                            break;
                            break;

                        case CPARSE_EDITBOX:
                            TokenType = TOKEN_COMMON;

                            if (Control_) delete Control_;

                            Control_ = new C_EditBox;
                            break;

                        case CPARSE_LISTBOX:
                            TokenType = TOKEN_COMMON;

                            if (Control_) delete Control_;

                            Control_ = new C_ListBox;
                            break;

                        case CPARSE_SLIDER:
                            TokenType = TOKEN_COMMON;

                            if (Control_) delete Control_;

                            Control_ = new C_Slider;
                            break;

                            //case CPARSE_ACMI:
                            // TokenType=TOKEN_COMMON;
                            // Control_=new C_Acmi;
                            // break;
                        case CPARSE_PANNER:
                            TokenType = TOKEN_COMMON;

                            if (Control_) delete Control_;

                            Control_ = new C_Panner;
                            break;

                        case CPARSE_SCROLLBAR:
                            TokenType = TOKEN_COMMON;

                            if (Control_) delete Control_;

                            Control_ = new C_ScrollBar;
                            break;

                        case CPARSE_TREELIST:
                            TokenType = TOKEN_COMMON;

                            if (Control_) delete Control_;

                            Control_ = new C_TreeList;
                            break;

                        case CPARSE_BITMAP:
                            TokenType = TOKEN_COMMON;

                            if (Control_) delete Control_;

                            Control_ = new C_Bitmap;
                            break;

                        case CPARSE_TILE:
                            TokenType = TOKEN_COMMON;

                            if (Control_) delete Control_;

                            Control_ = new C_Tile;
                            break;

                        case CPARSE_ANIM:
                            TokenType = TOKEN_COMMON;

                            if (Control_) delete Control_;

                            Control_ = new C_Anim;
                            break;

                        case CPARSE_CURSOR:
                            TokenType = TOKEN_COMMON;

                            if (Control_) delete Control_;

                            Control_ = new C_Cursor;
                            break;

                        case CPARSE_MARQUE:
                            TokenType = TOKEN_COMMON;

                            if (Control_) delete Control_;

                            Control_ = new C_Marque;
                            break;

                        case CPARSE_BOX:
                            TokenType = TOKEN_COMMON;

                            if (Control_) delete Control_;

                            Control_ = new C_Box;
                            break;

                        case CPARSE_LINE:
                            TokenType = TOKEN_COMMON;

                            if (Control_) delete Control_;

                            Control_ = new C_Line;
                            break;

                        case CPARSE_CLOCK:
                            TokenType = TOKEN_COMMON;

                            if (Control_) delete Control_;

                            Control_ = new C_Clock;
                            break;

                        case CPARSE_FILL:
                            TokenType = TOKEN_COMMON;

                            if (Control_) delete Control_;

                            Control_ = new C_Fill;
                            break;

                        case CPARSE_TREE:
                            TokenType = TOKEN_COMMON;

                            if (Control_) delete Control_;

                            Control_ = new C_TreeList;
                            break;

                        case CPARSE_ANIMATION:
                        case CPARSE_FONT:
                        case CPARSE_SOUND:
                        case CPARSE_STRING:
                            Done = 1;
                            break;

                        default:
                            MonoPrint("ControlParser: Token NOT FOUND [%s]\n", &script_[Idx_]);
                            Section = SECTION_FINDTOKEN;
                            break;
                    }

                    Idx_ += tokenlen_;
                    tokenlen_ = 0;
                }
                else
                {
                    Section = SECTION_FINDTOKEN;
                    Idx_ += tokenlen_;
                    tokenlen_ = 0;
                }

                break;

            case SECTION_FINDSUBTOKEN:
                // Look for token starting with '['
                Found = 0;

                while ( not Found and not Done)
                {
                    switch (script_[Idx_])
                    {
                        case '[':
                            if ( not Comment and not InString)
                            {
                                Found = 1;
                                break;
                            }

                            Idx_++;
                            break;

                        case '"':
                            InString = 1 - InString;
                            Idx_++;
                            break;

                        case '#':
                            Comment = 1;
                            Idx_++;
                            break;

                        case 0x0a:
                        case 0x0d:
                            Comment = 0;
                            Idx_++;
                            break;

                        default:
                            Idx_++;
                    }

                    if (Idx_ >= scriptlen_)
                        Done = 1;
                }

                tokenlen_ = 0;

                while (script_[Idx_ + tokenlen_] not_eq ']' and (Idx_ + tokenlen_) < scriptlen_)
                    tokenlen_++;

                tokenlen_++;

                if ((Idx_ + tokenlen_) >= scriptlen_)
                {
                    Done = 1;
                    break;
                }

                if (Found == 1)
                    Section = SECTION_PROCESSSUBTOKEN;

                break;

            case SECTION_PROCESSSUBTOKEN:
                TokenID = FindToken(&script_[Idx_]);

                if (TokenID)
                {
                    // if found... this is a MAIN keyword NOT a Control/Window keyword
                    Done = 1;
                    break;
                }

                switch (TokenType)
                {
                    case TOKEN_COMMON:
                    case TOKEN_LOCAL:
                        TokenID = Control_->BaseFind(&script_[Idx_]);

                        if (TokenID)
                        {
                            TokenType = TOKEN_COMMON;
                            Section = SECTION_FINDPARAMS;
                            Idx_ += tokenlen_;
                            tokenlen_ = 0;
                        }
                        else
                        {
                            TokenID = Control_->LocalFind(&script_[Idx_]);

                            if (TokenID)
                            {
                                Section = SECTION_FINDPARAMS;
                                TokenType = TOKEN_LOCAL;
                                Idx_ += tokenlen_;
                                tokenlen_ = 0;
                            }
                            else
                            {
                                Section = SECTION_FINDSUBTOKEN;
                                Idx_++;
                            }
                        }

                        break;
                }

                break;

            case SECTION_FINDPARAMS:
                P_Idx_ = 0; // start with 0 parameters

                // Repeat until token char '[' found (or EOF)
                Finished = 0;

                while ( not Finished)
                {
                    // Find NON white space
                    Found = 0;

                    while ( not Found and not Done and not Finished)
                    {
                        switch (script_[Idx_])
                        {
                            case ' ':
                            case ',':
                            case 0x09:
                            case 0x0a:
                            case 0x0d:
                                Idx_++;
                                break;

                            case '[':
                                Finished = 1;
                                Section = SECTION_PROCESSPARAMS;
                                break;

                            default:
                                Found = 1;
                                break;
                        }

                        if (Idx_ >= scriptlen_)
                        {
                            Finished = 1;
                            Section = SECTION_PROCESSPARAMS;
                        }
                    }

                    if (Found)
                    {
                        Found = 0;

                        if (script_[Idx_] == '"') // string
                        {
                            tokenlen_ = 1;
                            str_ = &script_[Idx_ + tokenlen_];

                            // Find closing (")
                            while ( not Found and not Finished)
                            {
                                if (script_[Idx_ + tokenlen_] == '"')
                                    Found = 1;
                                else
                                {
                                    if ((Idx_ + tokenlen_) >= scriptlen_)
                                    {
                                        Finished = 1;
                                        Section = SECTION_PROCESSPARAMS;
                                    }
                                    else
                                        tokenlen_++;
                                }
                            }

                            if (Found)
                                script_[Idx_ + tokenlen_] = 0; // make NULL terminated string

                            Idx_ += tokenlen_ + 1;
                            tokenlen_ = 0;
                        }
                        else if (isdigit(script_[Idx_]) or script_[Idx_] == '-') // Number
                        {
                            // find white space
                            Found = 0;
                            tokenlen_ = 1;

                            while ( not Found)
                            {
                                switch (script_[Idx_ + tokenlen_])
                                {
                                    case ' ':
                                    case ',':
                                    case 0x09:
                                    case 0x0a:
                                    case 0x0d:
                                        Found = 1;
                                        break;

                                    default:
                                        tokenlen_++;
                                        break;
                                }

                                if ((Idx_ + tokenlen_) >= scriptlen_)
                                {
                                    Found = 1;
                                    Finished = 1;
                                    Section = SECTION_PROCESSPARAMS;
                                }
                            }

                            if (Found and P_Idx_ < PARSE_MAX_PARAMS)
                                P_[P_Idx_++] = atol(&script_[Idx_]);

                            Idx_ += tokenlen_;
                            tokenlen_ = 0;
                        }
                        else
                        {
                            // Look for ID in tables
                            // Look for white space
                            Found = 0;
                            tokenlen_ = 0;

                            while ( not Found and not Finished)
                            {
                                switch (script_[Idx_ + tokenlen_])
                                {
                                    case ' ':
                                    case ',':
                                    case 0x09:
                                    case 0x0a:
                                    case 0x0d:
                                        Found = 1;
                                        break;

                                    default:
                                        tokenlen_++;
                                        break;
                                }

                                if ((Idx_ + tokenlen_) >= scriptlen_)
                                {
                                    Finished = 1;
                                    Found = 1;
                                    Section = SECTION_PROCESSPARAMS;
                                }
                            }

                            if (Found and P_Idx_ < PARSE_MAX_PARAMS)
                            {
                                script_[Idx_ + tokenlen_] = 0;
                                P_[P_Idx_++] = FindID(&script_[Idx_]);

                                if (P_[P_Idx_ - 1] < 0 and strcmp(&script_[Idx_], "NID"))
                                    TokenErrorList->AddText(&script_[Idx_]);
                            }

                            Idx_ += tokenlen_ + 1;
                            tokenlen_ = 0;
                        }
                    }
                }

                break;

            case SECTION_PROCESSPARAMS:
                switch (TokenType)
                {
                    case TOKEN_COMMON:
                        Control_->BaseFunction(static_cast<short>(TokenID), P_, str_, Handler_);
                        break;

                    case TOKEN_LOCAL:
                        Control_->LocalFunction(static_cast<short>(TokenID), P_, str_, Handler_);
                        break;
                }

                P_Idx_ = 0;
                P_[0] = 0;
                P_[1] = 0;
                P_[2] = 0;
                P_[3] = 0;
                P_[4] = 0;
                P_[5] = 0;
                P_[6] = 0;
                P_[7] = 0;
                str_ = NULL;
                Section = SECTION_FINDSUBTOKEN;
                break;
        }
    }

    return(Control_);
}

C_Window *C_Parser::WindowParser()
{
    long Done = 0, Comment = 0, Found = 0, InString = 0, Finished = 0;
    long TokenID = 0, Section = 0, TokenType = 0;

    Done = 0;
    Comment = 0;
    InString = 0;
    Section = SECTION_PROCESSTOKEN;
    TokenType = TOKEN_NOTHING;

    while ( not Done)
    {
        switch (Section)
        {
            case SECTION_FINDTOKEN:
                // Look for token starting with '['
                Found = 0;

                while ( not Found and not Done)
                {
                    switch (script_[Idx_])
                    {
                        case '[':
                            if ( not Comment and not InString)
                            {
                                Found = 1;
                                break;
                            }

                            Idx_++;
                            break;

                        case '"':
                            InString = 1 - InString;
                            Idx_++;
                            break;

                        case '#':
                            Comment = 1;
                            Idx_++;
                            break;

                        case 0x0a:
                        case 0x0d:
                            Comment = 0;
                            Idx_++;
                            break;

                        default:
                            Idx_++;
                    }

                    if (Idx_ >= scriptlen_)
                        Done = 1;
                }

                tokenlen_ = 0;

                while (script_[Idx_ + tokenlen_] not_eq ']' and (Idx_ + tokenlen_) < scriptlen_)
                    tokenlen_++;

                tokenlen_++;

                if ((Idx_ + tokenlen_) >= scriptlen_)
                {
                    Done = 1;
                    break;
                }

                if (Found == 1)
                    Section = SECTION_PROCESSTOKEN;

                break;

            case SECTION_PROCESSTOKEN:
                TokenID = FindToken(&script_[Idx_]);

                if (TokenID)
                {
                    switch (TokenID)
                    {
                        case CPARSE_WINDOW:
                            TokenType = TOKEN_WINDOW;
                            Window_ = new C_Window;
                            Section = SECTION_FINDSUBTOKEN;
                            Idx_ += tokenlen_;
                            tokenlen_ = 0;
                            break;

                        case CPARSE_BUTTON:
                        case CPARSE_TEXT:
                        case CPARSE_BOX:
                        case CPARSE_LINE:
                        case CPARSE_CLOCK:
                        case CPARSE_FILL:
                        case CPARSE_TREE:
                        case CPARSE_EDITBOX:
                        case CPARSE_LISTBOX:

                            //case CPARSE_ACMI:
                        case CPARSE_PANNER:
                        case CPARSE_SLIDER:
                        case CPARSE_TREELIST:
                        case CPARSE_BITMAP:
                        case CPARSE_TILE:
                        case CPARSE_ANIM:
                        case CPARSE_CURSOR:
                        case CPARSE_MARQUE:
                        case CPARSE_ANIMATION:
                        case CPARSE_VERSIONTEXT:
                            Window_->AddControl(ControlParser());
                            Section = SECTION_FINDTOKEN;
                            break;

                        case CPARSE_SCROLLBAR:
                            Window_->AddScrollBar((C_ScrollBar *)ControlParser());
                            Section = SECTION_FINDTOKEN;
                            break;

                        case CPARSE_FONT:
                            TokenType = TOKEN_FONT;
                            Idx_ += tokenlen_;
                            tokenlen_ = 0;
                            break;

                        case CPARSE_SOUND:
                            TokenType = TOKEN_SOUND;
                            Idx_ += tokenlen_;
                            tokenlen_ = 0;
                            break;

                        case CPARSE_STRING:
                            TokenType = TOKEN_STRING;
                            Idx_ += tokenlen_;
                            tokenlen_ = 0;
                            break;

                        case CPARSE_IMAGE:
                            TokenType = TOKEN_IMAGE;
                            Idx_ += tokenlen_;
                            tokenlen_ = 0;
                            break;

                        default:
                            MonoPrint("ControlParser: Token NOT FOUND [%s]\n", &script_[Idx_]);
                            Section = SECTION_FINDTOKEN;
                            Idx_ += tokenlen_;
                            tokenlen_ = 0;
                            break;
                    }
                }
                else
                {
                    Section = SECTION_FINDTOKEN;
                    Idx_ += tokenlen_;
                    tokenlen_ = 0;
                }

                break;

            case SECTION_FINDSUBTOKEN:
                // Look for token starting with '['
                Found = 0;

                while ( not Found and not Done)
                {
                    switch (script_[Idx_])
                    {
                        case '[':
                            if ( not Comment and not InString)
                            {
                                Found = 1;
                                break;
                            }

                            Idx_++;
                            break;

                        case '"':
                            InString = 1 - InString;
                            Idx_++;
                            break;

                        case '#':
                            Comment = 1;
                            Idx_++;
                            break;

                        case 0x0a:
                        case 0x0d:
                            Comment = 0;
                            Idx_++;
                            break;

                        default:
                            Idx_++;
                    }

                    if (Idx_ >= scriptlen_)
                        Done = 1;
                }

                tokenlen_ = 0;

                while (script_[Idx_ + tokenlen_] not_eq ']' and (Idx_ + tokenlen_) < scriptlen_)
                    tokenlen_++;

                tokenlen_++;

                if ((Idx_ + tokenlen_) >= scriptlen_)
                {
                    Done = 1;
                    break;
                }

                if (Found == 1)
                    Section = SECTION_PROCESSSUBTOKEN;

                break;

            case SECTION_PROCESSSUBTOKEN:
                TokenID = FindToken(&script_[Idx_]);

                if (TokenID)
                {
                    // if found... this is a MAIN keyword NOT a Control/Window keyword
                    Section = SECTION_PROCESSTOKEN;

                    switch (TokenID)
                    {
                        case CPARSE_WINDOW:
                        case CPARSE_FONT:
                            Done = 1;
                            break;

                        default:
                            Section = SECTION_FINDTOKEN;
                            break;
                    }

                    break;
                    Idx_ += tokenlen_;
                    tokenlen_ = 0;
                }

                switch (TokenType)
                {
                    case TOKEN_WINDOW:
                        TokenID = Window_->LocalFind(&script_[Idx_]);

                        if (TokenID)
                        {
                            Section = SECTION_FINDPARAMS;
                            Idx_ += tokenlen_;
                            tokenlen_ = 0;
                        }
                        else
                        {
                            Section = SECTION_FINDSUBTOKEN;
                            Idx_ += tokenlen_;
                            tokenlen_ = 0;
                        }

                        break;
                }

                break;

            case SECTION_FINDPARAMS:
                P_Idx_ = 0; // start with 0 parameters

                // Repeat until token char '[' found (or EOF)
                Finished = 0;

                while ( not Finished)
                {
                    // Find NON white space
                    Found = 0;

                    while ( not Found and not Done and not Finished)
                    {
                        switch (script_[Idx_])
                        {
                            case ' ':
                            case ',':
                            case 0x09:
                            case 0x0a:
                            case 0x0d:
                                Idx_++;
                                break;

                            case '[':
                                Finished = 1;
                                Section = SECTION_PROCESSPARAMS;
                                break;

                            default:
                                Found = 1;
                                break;
                        }

                        if (Idx_ >= scriptlen_)
                        {
                            Finished = 1;
                            Section = SECTION_PROCESSPARAMS;
                        }
                    }

                    if (Found)
                    {
                        Found = 0;

                        if (script_[Idx_] == '"') // string
                        {
                            tokenlen_ = 1;
                            str_ = &script_[Idx_ + tokenlen_];

                            // Find closing (")
                            while ( not Found and not Finished)
                            {
                                if (script_[Idx_ + tokenlen_] == '"')
                                    Found = 1;
                                else
                                {
                                    if ((Idx_ + tokenlen_) >= scriptlen_)
                                    {
                                        Finished = 1;
                                        Section = SECTION_PROCESSPARAMS;
                                    }
                                    else
                                        tokenlen_++;
                                }
                            }

                            if (Found)
                                script_[Idx_ + tokenlen_] = 0; // make NULL terminated string

                            Idx_ += tokenlen_ + 1;
                            tokenlen_ = 0;
                        }
                        else if (isdigit(script_[Idx_]) or script_[Idx_] == '-') // Number
                        {
                            // find white space
                            Found = 0;
                            tokenlen_ = 1;

                            while ( not Found)
                            {
                                switch (script_[Idx_ + tokenlen_])
                                {
                                    case ' ':
                                    case ',':
                                    case 0x09:
                                    case 0x0a:
                                    case 0x0d:
                                        Found = 1;
                                        break;

                                    default:
                                        tokenlen_++;
                                        break;
                                }

                                if ((Idx_ + tokenlen_) >= scriptlen_)
                                {
                                    Found = 1;
                                    Finished = 1;
                                    Section = SECTION_PROCESSPARAMS;
                                }
                            }

                            if (Found and P_Idx_ < PARSE_MAX_PARAMS)
                                P_[P_Idx_++] = atol(&script_[Idx_]);

                            Idx_ += tokenlen_;
                            tokenlen_ = 0;
                        }
                        else
                        {
                            // Look for ID in tables
                            // Look for white space
                            Found = 0;
                            tokenlen_ = 0;

                            while ( not Found and not Finished)
                            {
                                switch (script_[Idx_ + tokenlen_])
                                {
                                    case ' ':
                                    case ',':
                                    case 0x09:
                                    case 0x0a:
                                    case 0x0d:
                                        Found = 1;
                                        break;

                                    default:
                                        tokenlen_++;
                                        break;
                                }

                                if ((Idx_ + tokenlen_) >= scriptlen_)
                                {
                                    Finished = 1;
                                    Found = 1;
                                    Section = SECTION_PROCESSPARAMS;
                                }
                            }

                            if (Found and P_Idx_ < PARSE_MAX_PARAMS)
                            {
                                script_[Idx_ + tokenlen_] = 0;
                                P_[P_Idx_++] = FindID(&script_[Idx_]);

                                if (P_[P_Idx_ - 1] < 0 and strcmp(&script_[Idx_], "NID"))
                                    TokenErrorList->AddText(&script_[Idx_]);
                            }

                            Idx_ += tokenlen_ + 1;
                            tokenlen_ = 0;
                        }
                    }
                }

                break;

            case SECTION_PROCESSPARAMS:
                switch (TokenType)
                {
                    case TOKEN_WINDOW:
                        Window_->LocalFunction(static_cast<short>(TokenID), P_, str_, Handler_);
                        break;
                }

                P_Idx_ = 0;
                P_[0] = 0;
                P_[1] = 0;
                P_[2] = 0;
                P_[3] = 0;
                P_[4] = 0;
                P_[5] = 0;
                P_[6] = 0;
                P_[7] = 0;
                str_ = NULL;
                Section = SECTION_FINDSUBTOKEN;
                break;
        }
    }

    return(Window_);
}

C_Window *C_Parser::ParseWindow(char *filename)
{
    long Done = 0, Comment = 0, Found = 0, InString = 0; //,Finished=0;;
    long TokenID = 0, Section = 0, TokenType = 0;

    if (LoadScript(filename) == FALSE)
        return(FALSE);

    Idx_ = 0;
    P_Idx_ = 0;
    tokenlen_ = 0;

    Done = 0;
    Comment = 0;
    InString = 0;
    Section = SECTION_FINDTOKEN;
    TokenType = TOKEN_NOTHING;

    while ( not Done)
    {
        switch (Section)
        {
            case SECTION_FINDTOKEN:
                // Look for token starting with '['
                Found = 0;

                while ( not Found and not Done)
                {
                    switch (script_[Idx_])
                    {
                        case '[':
                            if ( not Comment and not InString)
                            {
                                Found = 1;
                                break;
                            }

                            Idx_++;
                            break;

                        case '"':
                            InString = 1 - InString;
                            Idx_++;
                            break;

                        case '#':
                            Comment = 1;
                            Idx_++;
                            break;

                        case 0x0a:
                        case 0x0d:
                            Comment = 0;
                            Idx_++;
                            break;

                        default:
                            Idx_++;
                    }

                    if ((Idx_) >= scriptlen_)
                        Done = 1;
                }

                tokenlen_ = 0;

                while (script_[Idx_ + tokenlen_] not_eq ']' and (Idx_ + tokenlen_) < scriptlen_)
                    tokenlen_++;

                tokenlen_++;

                if ((Idx_ + tokenlen_) >= scriptlen_)
                {
                    Done = 1;
                    break;
                }

                if (Found == 1)
                    Section = SECTION_PROCESSTOKEN;

                break;

            case SECTION_PROCESSTOKEN:
                TokenID = FindToken(&script_[Idx_]);

                if (TokenID)
                {
                    switch (TokenID)
                    {
                        case CPARSE_WINDOW:
                            return(WindowParser());
                            break;

                        default:
                            Idx_ += tokenlen_;
                            tokenlen_ = 0;
                            Section = SECTION_FINDTOKEN;
                            break;
                    }
                }
                else
                {
                    Section = SECTION_FINDTOKEN;
                    Idx_ += tokenlen_;
                    tokenlen_ = 0;
                }

                break;
        }
    }

    return(NULL);
}

C_Base *C_Parser::ParseControl(char *filename)
{
    long Done = 0, Comment = 0, Found = 0, InString = 0; //,Finished=0;;
    long TokenID = 0, Section = 0, TokenType = 0;

    if (LoadScript(filename) == FALSE)
        return(FALSE);

    Idx_ = 0;
    P_Idx_ = 0;
    tokenlen_ = 0;

    Done = 0;
    Comment = 0;
    InString = 0;
    Section = SECTION_FINDTOKEN;
    TokenType = TOKEN_NOTHING;

    while ( not Done)
    {
        switch (Section)
        {
            case SECTION_FINDTOKEN:
                // Look for token starting with '['
                Found = 0;

                while ( not Found and not Done)
                {
                    switch (script_[Idx_])
                    {
                        case '[':
                            if ( not Comment and not InString)
                            {
                                Found = 1;
                                break;
                            }

                            Idx_++;
                            break;

                        case '"':
                            InString = 1 - InString;
                            Idx_++;
                            break;

                        case '#':
                            Comment = 1;
                            Idx_++;
                            break;

                        case 0x0a:
                        case 0x0d:
                            Comment = 0;
                            Idx_++;
                            break;

                        default:
                            Idx_++;
                    }

                    if (Idx_ >= scriptlen_)
                        Done = 1;
                }

                tokenlen_ = 0;

                while (script_[Idx_ + tokenlen_] not_eq ']' and (Idx_ + tokenlen_) < scriptlen_)
                    tokenlen_++;

                tokenlen_++;

                if ((Idx_ + tokenlen_) >= scriptlen_)
                {
                    Done = 1;
                    break;
                }

                if (Found == 1)
                    Section = SECTION_PROCESSTOKEN;

                break;

            case SECTION_PROCESSTOKEN:
                TokenID = FindToken(&script_[Idx_]);

                if (TokenID)
                {
                    switch (TokenID)
                    {
                        case CPARSE_BUTTON:
                        case CPARSE_TEXT:
                        case CPARSE_BOX:
                        case CPARSE_LINE:
                        case CPARSE_CLOCK:
                        case CPARSE_FILL:
                        case CPARSE_TREE:
                        case CPARSE_EDITBOX:
                        case CPARSE_LISTBOX:

                            //case CPARSE_ACMI:
                        case CPARSE_PANNER:
                        case CPARSE_SLIDER:
                        case CPARSE_SCROLLBAR:
                        case CPARSE_TREELIST:
                        case CPARSE_BITMAP:
                        case CPARSE_TILE:
                        case CPARSE_ANIM:
                        case CPARSE_CURSOR:
                        case CPARSE_MARQUE:
                        case CPARSE_ANIMATION:
                            return(ControlParser());
                            break;
                    }

                    Idx_ += tokenlen_;
                    tokenlen_ = 0;
                    Section = SECTION_FINDTOKEN;
                }
                else
                {
                    Section = SECTION_FINDTOKEN;
                    Idx_ += tokenlen_;
                    tokenlen_ = 0;
                }

                break;
        }
    }

    return(NULL);
}

C_Image *C_Parser::ParseImage(char *filename)
{
    long Done = 0, Comment = 0, Found = 0, InString = 0, Finished = 0;;
    long TokenID = 0, Section = 0, TokenType = 0;
    long ImageID = 0;

    Idx_ = 0;
    P_Idx_ = 0;
    tokenlen_ = 0;

    if (Image_ == NULL)
    {
        Image_ = new C_Image;
        Image_->Setup();
    }

    if (Anim_ == NULL)
    {
        Anim_ = new C_Animation;
        Anim_->Setup();
    }

    if (LoadScript(filename) == FALSE)
        return(NULL);

    Done = 0;
    Comment = 0;
    InString = 0;
    Section = SECTION_FINDTOKEN;
    TokenType = TOKEN_NOTHING;

    while ( not Done)
    {
        switch (Section)
        {
            case SECTION_FINDTOKEN:
                // Look for token starting with '['
                Found = 0;

                while ( not Found and not Done)
                {
                    switch (script_[Idx_])
                    {
                        case '[':
                            if ( not Comment and not InString)
                            {
                                Found = 1;
                                break;
                            }

                            Idx_++;
                            break;

                        case '"':
                            InString = 1 - InString;
                            Idx_++;
                            break;

                        case '#':
                            Comment = 1;
                            Idx_++;
                            break;

                        case 0x0a:
                        case 0x0d:
                            Comment = 0;
                            Idx_++;
                            break;

                        default:
                            Idx_++;
                    }

                    if ((Idx_) >= scriptlen_)
                        Done = 1;
                }

                tokenlen_ = 0;

                while (script_[Idx_ + tokenlen_] not_eq ']' and (Idx_ + tokenlen_) < scriptlen_)
                    tokenlen_++;

                tokenlen_++;

                if ((Idx_ + tokenlen_) >= scriptlen_)
                {
                    Done = 1;
                    break;
                }

                if (Found == 1)
                    Section = SECTION_PROCESSTOKEN;

                break;

            case SECTION_PROCESSTOKEN:
                TokenID = Image_->LocalFind(&script_[Idx_]);

                if (TokenID)
                {
                    Section = SECTION_FINDPARAMS;
                    Idx_ += tokenlen_;
                    tokenlen_ = 0;
                    TokenType = 1;
                    break;
                }

                TokenID = Anim_->LocalFind(&script_[Idx_]);

                if (TokenID)
                {
                    Section = SECTION_FINDPARAMS;
                    Idx_ += tokenlen_;
                    tokenlen_ = 0;
                    TokenType = 2;
                    break;
                }

                TokenType = 0;
                Section = SECTION_FINDTOKEN;
                Idx_ += tokenlen_;
                tokenlen_ = 0;
                break;

            case SECTION_FINDPARAMS:
                P_Idx_ = 0; // start with 0 parameters

                // Repeat until token char '[' found (or EOF)
                Finished = 0;

                while ( not Finished)
                {
                    // Find NON white space
                    Found = 0;

                    while ( not Found and not Done and not Finished)
                    {
                        switch (script_[Idx_])
                        {
                            case ' ':
                            case ',':
                            case 0x09:
                            case 0x0a:
                            case 0x0d:
                                Idx_++;
                                break;

                            case '[':
                                Finished = 1;
                                Section = SECTION_PROCESSPARAMS;
                                break;

                            default:
                                Found = 1;
                                break;
                        }

                        if (Idx_ >= scriptlen_)
                        {
                            Finished = 1;
                            Section = SECTION_PROCESSPARAMS;
                        }
                    }

                    if (Found)
                    {
                        Found = 0;

                        if (script_[Idx_] == '"') // string
                        {
                            tokenlen_ = 1;
                            str_ = &script_[Idx_ + tokenlen_];

                            // Find closing (")
                            while ( not Found and not Finished)
                            {
                                if (script_[Idx_ + tokenlen_] == '"')
                                    Found = 1;
                                else
                                {
                                    if ((Idx_ + tokenlen_) >= scriptlen_)
                                    {
                                        Finished = 1;
                                        Section = SECTION_PROCESSPARAMS;
                                    }
                                    else
                                        tokenlen_++;
                                }
                            }

                            if (Found)
                                script_[Idx_ + tokenlen_] = 0; // make NULL terminated string

                            Idx_ += tokenlen_ + 1;
                            tokenlen_ = 0;
                        }
                        else if (isdigit(script_[Idx_]) or script_[Idx_] == '-') // Number
                        {
                            // find white space
                            Found = 0;
                            tokenlen_ = 1;

                            while ( not Found)
                            {
                                switch (script_[Idx_ + tokenlen_])
                                {
                                    case ' ':
                                    case ',':
                                    case 0x09:
                                    case 0x0a:
                                    case 0x0d:
                                        Found = 1;
                                        break;

                                    default:
                                        tokenlen_++;
                                        break;
                                }

                                if ((Idx_ + tokenlen_) >= scriptlen_)
                                {
                                    Found = 1;
                                    Finished = 1;
                                    Section = SECTION_PROCESSPARAMS;
                                }
                            }

                            if (Found and P_Idx_ < PARSE_MAX_PARAMS)
                                P_[P_Idx_++] = atol(&script_[Idx_]);

                            Idx_ += tokenlen_;
                            tokenlen_ = 0;
                        }
                        else
                        {
                            // Look for ID in tables
                            // Look for white space
                            Found = 0;
                            tokenlen_ = 0;

                            while ( not Found and not Finished)
                            {
                                switch (script_[Idx_ + tokenlen_])
                                {
                                    case ' ':
                                    case ',':
                                    case 0x09:
                                    case 0x0a:
                                    case 0x0d:
                                        Found = 1;
                                        break;

                                    default:
                                        tokenlen_++;
                                        break;
                                }

                                if ((Idx_ + tokenlen_) >= scriptlen_)
                                {
                                    Finished = 1;
                                    Found = 1;
                                    Section = SECTION_PROCESSPARAMS;
                                }
                            }

                            if (Found and P_Idx_ < PARSE_MAX_PARAMS)
                            {
                                script_[Idx_ + tokenlen_] = 0;
                                ImageID = FindID(&script_[Idx_]);

                                if (ImageID == -1)
                                    ImageID = AddNewID(&script_[Idx_], _START_BASE_ID_);

                                P_[P_Idx_++] = ImageID;
                            }

                            Idx_ += tokenlen_ + 1;
                            tokenlen_ = 0;
                        }
                    }
                }

                break;

            case SECTION_PROCESSPARAMS:
                if (TokenType == 1)
                    Image_->LocalFunction(static_cast<short>(TokenID), P_, str_, Handler_);
                else if (TokenType == 2)
                    Anim_->LocalFunction(static_cast<short>(TokenID), P_, str_, Handler_);

                TokenType = 0;
                P_Idx_ = 0;
                P_[0] = 0;
                P_[1] = 0;
                P_[2] = 0;
                P_[3] = 0;
                P_[4] = 0;
                P_[5] = 0;
                P_[6] = 0;
                P_[7] = 0;
                str_ = NULL;
                Section = SECTION_FINDTOKEN;
                break;
        }
    }

    return(Image_);
}

C_Font *C_Parser::ParseFont(char *filename)
{
    long Done = 0, Comment = 0, Found = 0, InString = 0, Finished = 0;;
    long TokenID = 0, Section = 0, TokenType = 0;
    long FontID = 0, NewID = 0;
    LOGFONT logfont = {0};

    Idx_ = 0;
    P_Idx_ = 0;
    tokenlen_ = 0;

    if (Font_ == NULL)
    {
        Font_ = new C_Font;
        Font_->Setup(Handler_);
    }

    if (LoadScript(filename) == FALSE)
        return(NULL);

    Done = 0;
    Comment = 0;
    InString = 0;
    Section = SECTION_FINDTOKEN;
    TokenType = TOKEN_NOTHING;

    memset(&logfont, 0, sizeof(LOGFONT));

    while ( not Done)
    {
        switch (Section)
        {
            case SECTION_FINDTOKEN:
                // Look for token starting with '['
                Found = 0;

                while ( not Found and not Done)
                {
                    switch (script_[Idx_])
                    {
                        case '[':
                            if ( not Comment and not InString)
                            {
                                Found = 1;
                                break;
                            }

                            Idx_++;
                            break;

                        case '"':
                            InString = 1 - InString;
                            Idx_++;
                            break;

                        case '#':
                            Comment = 1;
                            Idx_++;
                            break;

                        case 0x0a:
                        case 0x0d:
                            Comment = 0;
                            Idx_++;
                            break;

                        default:
                            Idx_++;
                    }

                    if ((Idx_) >= scriptlen_)
                        Done = 1;
                }

                tokenlen_ = 0;

                while (script_[Idx_ + tokenlen_] not_eq ']' and (Idx_ + tokenlen_) < scriptlen_)
                    tokenlen_++;

                tokenlen_++;

                if ((Idx_ + tokenlen_) >= scriptlen_)
                {
                    Done = 1;
                    break;
                }

                if (Found == 1)
                    Section = SECTION_PROCESSTOKEN;

                break;

            case SECTION_PROCESSTOKEN:
                TokenID = Font_->FontFind(&script_[Idx_]);

                if (TokenID)
                {
                    Section = SECTION_FINDPARAMS;
                    Idx_ += tokenlen_;
                    tokenlen_ = 0;
                }
                else
                {
                    Section = SECTION_FINDTOKEN;
                    Idx_ += tokenlen_;
                    tokenlen_ = 0;
                }

                break;

            case SECTION_FINDPARAMS:
                P_Idx_ = 0; // start with 0 parameters

                // Repeat until token char '[' found (or EOF)
                Finished = 0;

                while ( not Finished)
                {
                    // Find NON white space
                    Found = 0;

                    while ( not Found and not Done and not Finished)
                    {
                        switch (script_[Idx_])
                        {
                            case ' ':
                            case ',':
                            case 0x09:
                            case 0x0a:
                            case 0x0d:
                                Idx_++;
                                break;

                            case '[':
                                Finished = 1;
                                Section = SECTION_PROCESSPARAMS;
                                break;

                            default:
                                Found = 1;
                                break;
                        }

                        if (Idx_ >= scriptlen_)
                        {
                            Finished = 1;
                            Section = SECTION_PROCESSPARAMS;
                        }
                    }

                    if (Found)
                    {
                        Found = 0;

                        if (script_[Idx_] == '"') // string
                        {
                            tokenlen_ = 1;
                            str_ = &script_[Idx_ + tokenlen_];

                            // Find closing (")
                            while ( not Found and not Finished)
                            {
                                if (script_[Idx_ + tokenlen_] == '"')
                                    Found = 1;
                                else
                                {
                                    if ((Idx_ + tokenlen_) >= scriptlen_)
                                    {
                                        Finished = 1;
                                        Section = SECTION_PROCESSPARAMS;
                                    }
                                    else
                                        tokenlen_++;
                                }
                            }

                            if (Found)
                                script_[Idx_ + tokenlen_] = 0; // make NULL terminated string

                            Idx_ += tokenlen_ + 1;
                            tokenlen_ = 0;
                        }
                        else if (isdigit(script_[Idx_]) or script_[Idx_] == '-') // Number
                        {
                            // find white space
                            Found = 0;
                            tokenlen_ = 1;

                            while ( not Found)
                            {
                                switch (script_[Idx_ + tokenlen_])
                                {
                                    case ' ':
                                    case ',':
                                    case 0x09:
                                    case 0x0a:
                                    case 0x0d:
                                        Found = 1;
                                        break;

                                    default:
                                        tokenlen_++;
                                        break;
                                }

                                if ((Idx_ + tokenlen_) >= scriptlen_)
                                {
                                    Found = 1;
                                    Finished = 1;
                                    Section = SECTION_PROCESSPARAMS;
                                }
                            }

                            if (Found and P_Idx_ < PARSE_MAX_PARAMS)
                                P_[P_Idx_++] = atol(&script_[Idx_]);

                            Idx_ += tokenlen_;
                            tokenlen_ = 0;
                        }
                        else
                        {
                            // Look for ID in tables
                            // Look for white space
                            Found = 0;
                            tokenlen_ = 0;

                            while ( not Found and not Finished)
                            {
                                switch (script_[Idx_ + tokenlen_])
                                {
                                    case ' ':
                                    case ',':
                                    case 0x09:
                                    case 0x0a:
                                    case 0x0d:
                                        Found = 1;
                                        break;

                                    default:
                                        tokenlen_++;
                                        break;
                                }

                                if ((Idx_ + tokenlen_) >= scriptlen_)
                                {
                                    Finished = 1;
                                    Found = 1;
                                    Section = SECTION_PROCESSPARAMS;
                                }
                            }

                            if (Found and P_Idx_ < PARSE_MAX_PARAMS)
                            {
                                script_[Idx_ + tokenlen_] = 0;
                                FontID = FindID(&script_[Idx_]);

                                if (FontID == -1)
                                {
                                    if (FontID == -1)
                                        FontID = AddNewID(&script_[Idx_], 1);
                                }

                                P_[P_Idx_++] = FontID;
                            }

                            Idx_ += tokenlen_ + 1;
                            tokenlen_ = 0;
                        }
                    }
                }

                break;

            case SECTION_PROCESSPARAMS:
                Font_->FontFunction(static_cast<short>(TokenID), P_, str_, &logfont, &NewID);
                P_Idx_ = 0;
                P_[0] = 0;
                P_[1] = 0;
                P_[2] = 0;
                P_[3] = 0;
                P_[4] = 0;
                P_[5] = 0;
                P_[6] = 0;
                P_[7] = 0;
                str_ = NULL;
                Section = SECTION_FINDTOKEN;
                break;
        }
    }

    return(Font_);
}

C_Sound *C_Parser::ParseSound(char *filename)
{
    long Done = 0, Comment = 0, Found = 0, InString = 0, Finished = 0;;
    long TokenID = 0, Section = 0, TokenType = 0;
    long SoundID = 0;

    Idx_ = 0;
    P_Idx_ = 0;
    tokenlen_ = 0;

    if (Sound_ == NULL)
    {
        Sound_ = new C_Sound;
        Sound_->Setup();
    }

    if (LoadScript(filename) == FALSE)
        return(NULL);

    Done = 0;
    Comment = 0;
    InString = 0;
    Section = SECTION_FINDTOKEN;
    TokenType = TOKEN_NOTHING;

    while ( not Done)
    {
        switch (Section)
        {
            case SECTION_FINDTOKEN:
                // Look for token starting with '['
                Found = 0;

                while ( not Found and not Done)
                {
                    switch (script_[Idx_])
                    {
                        case '[':
                            if ( not Comment and not InString)
                            {
                                Found = 1;
                                break;
                            }

                            Idx_++;
                            break;

                        case '"':
                            InString = 1 - InString;
                            Idx_++;
                            break;

                        case '#':
                            Comment = 1;
                            Idx_++;
                            break;

                        case 0x0a:
                        case 0x0d:
                            Comment = 0;
                            Idx_++;
                            break;

                        default:
                            Idx_++;
                    }

                    if ((Idx_) >= scriptlen_)
                        Done = 1;
                }

                tokenlen_ = 0;

                while (script_[Idx_ + tokenlen_] not_eq ']' and (Idx_ + tokenlen_) < scriptlen_)
                    tokenlen_++;

                tokenlen_++;

                if ((Idx_ + tokenlen_) >= scriptlen_)
                {
                    Done = 1;
                    break;
                }

                if (Found == 1)
                    Section = SECTION_PROCESSTOKEN;

                break;

            case SECTION_PROCESSTOKEN:
                TokenID = Sound_->LocalFind(&script_[Idx_]);

                if (TokenID)
                {
                    Section = SECTION_FINDPARAMS;
                    Idx_ += tokenlen_;
                    tokenlen_ = 0;
                }
                else
                {
                    Section = SECTION_FINDTOKEN;
                    Idx_ += tokenlen_;
                    tokenlen_ = 0;
                }

                break;

            case SECTION_FINDPARAMS:
                P_Idx_ = 0; // start with 0 parameters

                // Repeat until token char '[' found (or EOF)
                Finished = 0;

                while ( not Finished)
                {
                    // Find NON white space
                    Found = 0;

                    while ( not Found and not Done and not Finished)
                    {
                        switch (script_[Idx_])
                        {
                            case ' ':
                            case ',':
                            case 0x09:
                            case 0x0a:
                            case 0x0d:
                                Idx_++;
                                break;

                            case '[':
                                Finished = 1;
                                Section = SECTION_PROCESSPARAMS;
                                break;

                            default:
                                Found = 1;
                                break;
                        }

                        if (Idx_ >= scriptlen_)
                        {
                            Finished = 1;
                            Section = SECTION_PROCESSPARAMS;
                        }
                    }

                    if (Found)
                    {
                        Found = 0;

                        if (script_[Idx_] == '"') // string
                        {
                            tokenlen_ = 1;
                            str_ = &script_[Idx_ + tokenlen_];

                            // Find closing (")
                            while ( not Found and not Finished)
                            {
                                if (script_[Idx_ + tokenlen_] == '"')
                                    Found = 1;
                                else
                                {
                                    if ((Idx_ + tokenlen_) >= scriptlen_)
                                    {
                                        Finished = 1;
                                        Section = SECTION_PROCESSPARAMS;
                                    }
                                    else
                                        tokenlen_++;
                                }
                            }

                            if (Found)
                                script_[Idx_ + tokenlen_] = 0; // make NULL terminated string

                            Idx_ += tokenlen_ + 1;
                            tokenlen_ = 0;
                        }
                        else if (isdigit(script_[Idx_]) or script_[Idx_] == '-') // Number
                        {
                            // find white space
                            Found = 0;
                            tokenlen_ = 1;

                            while ( not Found)
                            {
                                switch (script_[Idx_ + tokenlen_])
                                {
                                    case ' ':
                                    case ',':
                                    case 0x09:
                                    case 0x0a:
                                    case 0x0d:
                                        Found = 1;
                                        break;

                                    default:
                                        tokenlen_++;
                                        break;
                                }

                                if ((Idx_ + tokenlen_) >= scriptlen_)
                                {
                                    Found = 1;
                                    Finished = 1;
                                    Section = SECTION_PROCESSPARAMS;
                                }
                            }

                            if (Found and P_Idx_ < PARSE_MAX_PARAMS)
                                P_[P_Idx_++] = atol(&script_[Idx_]);

                            Idx_ += tokenlen_;
                            tokenlen_ = 0;
                        }
                        else
                        {
                            // Look for ID in tables
                            // Look for white space
                            Found = 0;
                            tokenlen_ = 0;

                            while ( not Found and not Finished)
                            {
                                switch (script_[Idx_ + tokenlen_])
                                {
                                    case ' ':
                                    case ',':
                                    case 0x09:
                                    case 0x0a:
                                    case 0x0d:
                                        Found = 1;
                                        break;

                                    default:
                                        tokenlen_++;
                                        break;
                                }

                                if ((Idx_ + tokenlen_) >= scriptlen_)
                                {
                                    Finished = 1;
                                    Found = 1;
                                    Section = SECTION_PROCESSPARAMS;
                                }
                            }

                            if (Found and P_Idx_ < PARSE_MAX_PARAMS)
                            {
                                script_[Idx_ + tokenlen_] = 0;
                                SoundID = FindID(&script_[Idx_]);

                                if (SoundID == -1)
                                    SoundID = AddNewID(&script_[Idx_], 1);

                                P_[P_Idx_++] = SoundID;
                            }

                            Idx_ += tokenlen_ + 1;
                            tokenlen_ = 0;
                        }
                    }
                }

                break;

            case SECTION_PROCESSPARAMS:
                Sound_->LocalFunction(static_cast<short>(TokenID), P_, str_, Handler_);
                P_Idx_ = 0;
                P_[0] = 0;
                P_[1] = 0;
                P_[2] = 0;
                P_[3] = 0;
                P_[4] = 0;
                P_[5] = 0;
                P_[6] = 0;
                P_[7] = 0;
                str_ = NULL;
                Section = SECTION_FINDTOKEN;
                break;
        }
    }

    return(Sound_);
}

C_String *C_Parser::ParseString(char *filename)
{
    int InString = 0;
    short Done = 0, Comment = 0, Found = 0, Finished = 0;
    short TokenID = 0, Section = 0, TokenType = 0;
    long StringID = 0;
    BOOL AddFlag = FALSE;
    char buffer[80];

    Idx_ = 0;
    P_Idx_ = 0;
    tokenlen_ = 0;

    if (String_ == NULL)
    {
        String_ = new C_String;
        String_->Setup(TXT_LAST_TEXT_ID);
    }

    if (LoadScript(filename) == FALSE)
        return(NULL);

    Done = 0;
    Comment = 0;
    InString = 0;
    Section = SECTION_FINDTOKEN;
    TokenType = TOKEN_NOTHING;

    while ( not Done)
    {
        switch (Section)
        {
            case SECTION_FINDTOKEN:
                // Look for token starting with '['
                Found = 0;

                while ( not Found and not Done)
                {
                    switch (script_[Idx_])
                    {
                        case '[':
                            if ( not Comment and not InString)
                            {
                                Found = 1;
                                break;
                            }

                            Idx_++;
                            break;

                        case '"':
                            InString = 1 - InString;
                            Idx_++;
                            break;

                        case '#':
                            Comment = 1;
                            Idx_++;
                            break;

                        case 0x0a:
                        case 0x0d:
                            Comment = 0;
                            Idx_++;
                            break;

                        default:
                            Idx_++;
                    }

                    if ((Idx_) >= scriptlen_)
                        Done = 1;
                }

                tokenlen_ = 0;

                while (script_[Idx_ + tokenlen_] not_eq ']' and (Idx_ + tokenlen_) < scriptlen_)
                    tokenlen_++;

                tokenlen_++;

                if ((Idx_ + tokenlen_) >= scriptlen_)
                {
                    Done = 1;
                    break;
                }

                if (Found == 1)
                    Section = SECTION_PROCESSTOKEN;

                break;

            case SECTION_PROCESSTOKEN:
                TokenID = String_->LocalFind(&script_[Idx_]);

                if (TokenID)
                {
                    Section = SECTION_FINDPARAMS;
                    Idx_ += tokenlen_;
                    tokenlen_ = 0;
                }
                else
                {
                    Section = SECTION_FINDTOKEN;
                    Idx_ += tokenlen_;
                    tokenlen_ = 0;
                }

                break;

            case SECTION_FINDPARAMS:
                P_Idx_ = 0; // start with 0 parameters

                // Repeat until token char '[' found (or EOF)
                Finished = 0;

                while ( not Finished)
                {
                    // Find NON white space
                    Found = 0;

                    while ( not Found and not Done and not Finished)
                    {
                        switch (script_[Idx_])
                        {
                            case ' ':
                            case ',':
                            case 0x09:
                            case 0x0a:
                            case 0x0d:
                                Idx_++;
                                break;

                            case '[':
                                Finished = 1;
                                Section = SECTION_PROCESSPARAMS;
                                break;

                            default:
                                Found = 1;
                                break;
                        }

                        if (Idx_ >= scriptlen_)
                        {
                            Finished = 1;
                            Section = SECTION_PROCESSPARAMS;
                        }
                    }

                    if (Found)
                    {
                        Found = 0;

                        if (script_[Idx_] == '"') // string
                        {
                            tokenlen_ = 1;
                            str_ = &script_[Idx_ + tokenlen_];

                            // Find closing (")
                            while ( not Found and not Finished)
                            {
                                if (script_[Idx_ + tokenlen_] == '"')
                                    Found = 1;
                                else
                                {
                                    if ((Idx_ + tokenlen_) >= scriptlen_)
                                    {
                                        Finished = 1;
                                        Section = SECTION_PROCESSPARAMS;
                                    }
                                    else
                                        tokenlen_++;
                                }
                            }

                            if (Found)
                                script_[Idx_ + tokenlen_] = 0; // make NULL terminated string

                            Idx_ += tokenlen_ + 1;
                            tokenlen_ = 0;
                        }
                        else if (isdigit(script_[Idx_]) or script_[Idx_] == '-') // Number
                        {
                            // find white space
                            Found = 0;
                            tokenlen_ = 1;

                            while ( not Found)
                            {
                                switch (script_[Idx_ + tokenlen_])
                                {
                                    case ' ':
                                    case ',':
                                    case 0x09:
                                    case 0x0a:
                                    case 0x0d:
                                        Found = 1;
                                        break;

                                    default:
                                        tokenlen_++;
                                        break;
                                }

                                if ((Idx_ + tokenlen_) >= scriptlen_)
                                {
                                    Found = 1;
                                    Finished = 1;
                                    Section = SECTION_PROCESSPARAMS;
                                }
                            }

                            if (Found and P_Idx_ < PARSE_MAX_PARAMS)
                                P_[P_Idx_++] = atol(&script_[Idx_]);

                            Idx_ += tokenlen_;
                            tokenlen_ = 0;
                        }
                        else
                        {
                            // Look for ID in tables
                            // Look for white space
                            Found = 0;
                            tokenlen_ = 0;

                            while ( not Found and not Finished)
                            {
                                switch (script_[Idx_ + tokenlen_])
                                {
                                    case ' ':
                                    case ',':
                                    case 0x09:
                                    case 0x0a:
                                    case 0x0d:
                                        Found = 1;
                                        break;

                                    default:
                                        tokenlen_++;
                                        break;
                                }

                                if ((Idx_ + tokenlen_) >= scriptlen_)
                                {
                                    Finished = 1;
                                    Found = 1;
                                    Section = SECTION_PROCESSPARAMS;
                                }
                            }

                            if (Found and P_Idx_ < PARSE_MAX_PARAMS)
                            {
                                script_[Idx_ + tokenlen_] = 0;
                                StringID = FindID(&script_[Idx_]);

                                if (StringID == -1)
                                {
                                    _tcscpy(buffer, &script_[Idx_]);
                                    StringID = -2;
                                }

                                P_[P_Idx_++] = StringID;
                            }

                            Idx_ += tokenlen_ + 1;
                            tokenlen_ = 0;
                        }
                    }
                }

                break;

            case SECTION_PROCESSPARAMS:
                if (P_[0] == -2)
                    AddFlag = TRUE;
                else
                    AddFlag = FALSE;

                String_->LocalFunction(static_cast<short>(TokenID), P_, str_, Handler_);

                if (AddFlag)
                    TokenOrder_->AddTextID(String_->GetLastID(), buffer);

                P_Idx_ = 0;
                P_[0] = 0;
                P_[1] = 0;
                P_[2] = 0;
                P_[3] = 0;
                P_[4] = 0;
                P_[5] = 0;
                P_[6] = 0;
                P_[7] = 0;
                str_ = NULL;
                Section = SECTION_FINDTOKEN;
                break;
        }
    }

    return(String_);
}

C_Movie *C_Parser::ParseMovie(char *filename)
{
    int InString = 0;
    short Done = 0, Comment = 0, Found = 0, Finished = 0;;
    short TokenID = 0, Section = 0, TokenType = 0;
    long MovieID = 0;

    Idx_ = 0;
    P_Idx_ = 0;
    tokenlen_ = 0;

    if (Movie_ == NULL)
    {
        Movie_ = new C_Movie;
        Movie_->Setup();
    }

    if (LoadScript(filename) == FALSE)
        return(NULL);

    Done = 0;
    Comment = 0;
    InString = 0;
    Section = SECTION_FINDTOKEN;
    TokenType = TOKEN_NOTHING;

    while ( not Done)
    {
        switch (Section)
        {
            case SECTION_FINDTOKEN:
                // Look for token starting with '['
                Found = 0;

                while ( not Found and not Done)
                {
                    switch (script_[Idx_])
                    {
                        case '[':
                            if ( not Comment and not InString)
                            {
                                Found = 1;
                                break;
                            }

                            Idx_++;
                            break;

                        case '"':
                            InString = 1 - InString;
                            Idx_++;
                            break;

                        case '#':
                            Comment = 1;
                            Idx_++;
                            break;

                        case 0x0a:
                        case 0x0d:
                            Comment = 0;
                            Idx_++;
                            break;

                        default:
                            Idx_++;
                    }

                    if ((Idx_) >= scriptlen_)
                        Done = 1;
                }

                tokenlen_ = 0;

                while (script_[Idx_ + tokenlen_] not_eq ']' and (Idx_ + tokenlen_) < scriptlen_)
                    tokenlen_++;

                tokenlen_++;

                if ((Idx_ + tokenlen_) >= scriptlen_)
                {
                    Done = 1;
                    break;
                }

                if (Found == 1)
                    Section = SECTION_PROCESSTOKEN;

                break;

            case SECTION_PROCESSTOKEN:
                TokenID = Movie_->LocalFind(&script_[Idx_]);

                if (TokenID)
                {
                    Section = SECTION_FINDPARAMS;
                    Idx_ += tokenlen_;
                    tokenlen_ = 0;
                }
                else
                {
                    Section = SECTION_FINDTOKEN;
                    Idx_ += tokenlen_;
                    tokenlen_ = 0;
                }

                break;

            case SECTION_FINDPARAMS:
                P_Idx_ = 0; // start with 0 parameters

                // Repeat until token char '[' found (or EOF)
                Finished = 0;

                while ( not Finished)
                {
                    // Find NON white space
                    Found = 0;

                    while ( not Found and not Done and not Finished)
                    {
                        switch (script_[Idx_])
                        {
                            case ' ':
                            case ',':
                            case 0x09:
                            case 0x0a:
                            case 0x0d:
                                Idx_++;
                                break;

                            case '[':
                                Finished = 1;
                                Section = SECTION_PROCESSPARAMS;
                                break;

                            default:
                                Found = 1;
                                break;
                        }

                        if (Idx_ >= scriptlen_)
                        {
                            Finished = 1;
                            Section = SECTION_PROCESSPARAMS;
                        }
                    }

                    if (Found)
                    {
                        Found = 0;

                        if (script_[Idx_] == '"') // string
                        {
                            tokenlen_ = 1;
                            str_ = &script_[Idx_ + tokenlen_];

                            // Find closing (")
                            while ( not Found and not Finished)
                            {
                                if (script_[Idx_ + tokenlen_] == '"')
                                    Found = 1;
                                else
                                {
                                    if ((Idx_ + tokenlen_) >= scriptlen_)
                                    {
                                        Finished = 1;
                                        Section = SECTION_PROCESSPARAMS;
                                    }
                                    else
                                        tokenlen_++;
                                }
                            }

                            if (Found)
                                script_[Idx_ + tokenlen_] = 0; // make NULL terminated string

                            Idx_ += tokenlen_ + 1;
                            tokenlen_ = 0;
                        }
                        else if (isdigit(script_[Idx_]) or script_[Idx_] == '-') // Number
                        {
                            // find white space
                            Found = 0;
                            tokenlen_ = 1;

                            while ( not Found)
                            {
                                switch (script_[Idx_ + tokenlen_])
                                {
                                    case ' ':
                                    case ',':
                                    case 0x09:
                                    case 0x0a:
                                    case 0x0d:
                                        Found = 1;
                                        break;

                                    default:
                                        tokenlen_++;
                                        break;
                                }

                                if ((Idx_ + tokenlen_) >= scriptlen_)
                                {
                                    Found = 1;
                                    Finished = 1;
                                    Section = SECTION_PROCESSPARAMS;
                                }
                            }

                            if (Found and P_Idx_ < PARSE_MAX_PARAMS)
                                P_[P_Idx_++] = atol(&script_[Idx_]);

                            Idx_ += tokenlen_;
                            tokenlen_ = 0;
                        }
                        else
                        {
                            // Look for ID in tables
                            // Look for white space
                            Found = 0;
                            tokenlen_ = 0;

                            while ( not Found and not Finished)
                            {
                                switch (script_[Idx_ + tokenlen_])
                                {
                                    case ' ':
                                    case ',':
                                    case 0x09:
                                    case 0x0a:
                                    case 0x0d:
                                        Found = 1;
                                        break;

                                    default:
                                        tokenlen_++;
                                        break;
                                }

                                if ((Idx_ + tokenlen_) >= scriptlen_)
                                {
                                    Finished = 1;
                                    Found = 1;
                                    Section = SECTION_PROCESSPARAMS;
                                }
                            }

                            if (Found and P_Idx_ < PARSE_MAX_PARAMS)
                            {
                                script_[Idx_ + tokenlen_] = 0;
                                MovieID = FindID(&script_[Idx_]);

                                if (MovieID == -1)
                                    MovieID = AddNewID(&script_[Idx_], 1);

                                P_[P_Idx_++] = MovieID;
                            }

                            Idx_ += tokenlen_ + 1;
                            tokenlen_ = 0;
                        }
                    }
                }

                break;

            case SECTION_PROCESSPARAMS:
                Movie_->LocalFunction(static_cast<short>(TokenID), P_, str_, Handler_);
                P_Idx_ = 0;
                P_[0] = 0;
                P_[1] = 0;
                P_[2] = 0;
                P_[3] = 0;
                P_[4] = 0;
                P_[5] = 0;
                P_[6] = 0;
                P_[7] = 0;
                str_ = NULL;
                Section = SECTION_FINDTOKEN;
                break;
        }
    }

    return(Movie_);
}

C_Base *C_Parser::PopupParser()
{
    int InString = 0;
    short Done = 0, Comment = 0, Found = 0, Finished = 0;;
    short Section = 0, TokenType = 0;
    long TokenID = 0;

    Done = 0;
    Comment = 0;
    InString = 0;
    Section = SECTION_PROCESSTOKEN;
    TokenType = TOKEN_NOTHING;

    while ( not Done)
    {
        switch (Section)
        {
            case SECTION_FINDTOKEN:
                // Look for token starting with '['
                Found = 0;

                while ( not Found and not Done)
                {
                    switch (script_[Idx_])
                    {
                        case '[':
                            if ( not Comment and not InString)
                            {
                                Found = 1;
                                break;
                            }

                            Idx_++;
                            break;

                        case '"':
                            InString = 1 - InString;
                            Idx_++;
                            break;

                        case '#':
                            Comment = 1;
                            Idx_++;
                            break;

                        case 0x0a:
                        case 0x0d:
                            Comment = 0;
                            Idx_++;
                            break;

                        default:
                            Idx_++;
                    }

                    if (Idx_ >= scriptlen_)
                        Done = 1;
                }

                tokenlen_ = 0;

                while (script_[Idx_ + tokenlen_] not_eq ']' and (Idx_ + tokenlen_) < scriptlen_)
                    tokenlen_++;

                tokenlen_++;

                if ((Idx_ + tokenlen_) >= scriptlen_)
                {
                    Done = 1;
                    break;
                }

                if (Found == 1)
                    Section = SECTION_PROCESSTOKEN;

                break;

            case SECTION_PROCESSTOKEN:
                TokenID = FindToken(&script_[Idx_]);

                if (TokenID)
                {
                    Section = SECTION_FINDSUBTOKEN;

                    switch (TokenID)
                    {
                        case CPARSE_POPUP:
                            TokenType = TOKEN_COMMON;
                            Control_ = new C_PopupList;

                            break;

                        default:
                            Section = SECTION_FINDTOKEN;
                            break;
                    }

                    Idx_ += tokenlen_;
                    tokenlen_ = 0;
                }
                else
                {
                    Section = SECTION_FINDTOKEN;
                    Idx_ += tokenlen_;
                    tokenlen_ = 0;
                }

                break;

            case SECTION_FINDSUBTOKEN:
                // Look for token starting with '['
                Found = 0;

                while ( not Found and not Done)
                {
                    switch (script_[Idx_])
                    {
                        case '[':
                            if ( not Comment and not InString)
                            {
                                Found = 1;
                                break;
                            }

                            Idx_++;
                            break;

                        case '"':
                            InString = 1 - InString;
                            Idx_++;
                            break;

                        case '#':
                            Comment = 1;
                            Idx_++;
                            break;

                        case 0x0a:
                        case 0x0d:
                            Comment = 0;
                            Idx_++;
                            break;

                        default:
                            Idx_++;
                    }

                    if (Idx_ >= scriptlen_)
                        Done = 1;
                }

                tokenlen_ = 0;

                while (script_[Idx_ + tokenlen_] not_eq ']' and (Idx_ + tokenlen_) < scriptlen_)
                    tokenlen_++;

                tokenlen_++;

                if ((Idx_ + tokenlen_) >= scriptlen_)
                {
                    Done = 1;
                    break;
                }

                if (Found == 1)
                    Section = SECTION_PROCESSSUBTOKEN;

                break;

            case SECTION_PROCESSSUBTOKEN:
                TokenID = FindToken(&script_[Idx_]);

                if (TokenID)
                {
                    // if found... this is a MAIN keyword NOT a Control/Window keyword
                    Done = 1;
                    break;
                }

                switch (TokenType)
                {
                    case TOKEN_COMMON:
                    case TOKEN_LOCAL:
                        TokenID = Control_->BaseFind(&script_[Idx_]);

                        if (TokenID)
                        {
                            TokenType = TOKEN_COMMON;
                            Section = SECTION_FINDPARAMS;
                            Idx_ += tokenlen_;
                            tokenlen_ = 0;
                        }
                        else
                        {
                            TokenID = Control_->LocalFind(&script_[Idx_]);

                            if (TokenID)
                            {
                                Section = SECTION_FINDPARAMS;
                                TokenType = TOKEN_LOCAL;
                                Idx_ += tokenlen_;
                                tokenlen_ = 0;
                            }
                            else
                            {
                                Section = SECTION_FINDSUBTOKEN;
                                Idx_++;
                            }
                        }

                        break;
                }

                break;

            case SECTION_FINDPARAMS:
                P_Idx_ = 0; // start with 0 parameters

                // Repeat until token char '[' found (or EOF)
                Finished = 0;

                while ( not Finished)
                {
                    // Find NON white space
                    Found = 0;

                    while ( not Found and not Done and not Finished)
                    {
                        switch (script_[Idx_])
                        {
                            case ' ':
                            case ',':
                            case 0x09:
                            case 0x0a:
                            case 0x0d:
                                Idx_++;
                                break;

                            case '[':
                                Finished = 1;
                                Section = SECTION_PROCESSPARAMS;
                                break;

                            default:
                                Found = 1;
                                break;
                        }

                        if (Idx_ >= scriptlen_)
                        {
                            Finished = 1;
                            Section = SECTION_PROCESSPARAMS;
                        }
                    }

                    if (Found)
                    {
                        Found = 0;

                        if (script_[Idx_] == '"') // string
                        {
                            tokenlen_ = 1;
                            str_ = &script_[Idx_ + tokenlen_];

                            // Find closing (")
                            while ( not Found and not Finished)
                            {
                                if (script_[Idx_ + tokenlen_] == '"')
                                    Found = 1;
                                else
                                {
                                    if ((Idx_ + tokenlen_) >= scriptlen_)
                                    {
                                        Finished = 1;
                                        Section = SECTION_PROCESSPARAMS;
                                    }
                                    else
                                        tokenlen_++;
                                }
                            }

                            if (Found)
                                script_[Idx_ + tokenlen_] = 0; // make NULL terminated string

                            Idx_ += tokenlen_ + 1;
                            tokenlen_ = 0;
                        }
                        else if (isdigit(script_[Idx_]) or script_[Idx_] == '-') // Number
                        {
                            // find white space
                            Found = 0;
                            tokenlen_ = 1;

                            while ( not Found)
                            {
                                switch (script_[Idx_ + tokenlen_])
                                {
                                    case ' ':
                                    case ',':
                                    case 0x09:
                                    case 0x0a:
                                    case 0x0d:
                                        Found = 1;
                                        break;

                                    default:
                                        tokenlen_++;
                                        break;
                                }

                                if ((Idx_ + tokenlen_) >= scriptlen_)
                                {
                                    Found = 1;
                                    Finished = 1;
                                    Section = SECTION_PROCESSPARAMS;
                                }
                            }

                            if (Found and P_Idx_ < PARSE_MAX_PARAMS)
                                P_[P_Idx_++] = atol(&script_[Idx_]);

                            Idx_ += tokenlen_;
                            tokenlen_ = 0;
                        }
                        else
                        {
                            // Look for ID in tables
                            // Look for white space
                            Found = 0;
                            tokenlen_ = 0;

                            while ( not Found and not Finished)
                            {
                                switch (script_[Idx_ + tokenlen_])
                                {
                                    case ' ':
                                    case ',':
                                    case 0x09:
                                    case 0x0a:
                                    case 0x0d:
                                        Found = 1;
                                        break;

                                    default:
                                        tokenlen_++;
                                        break;
                                }

                                if ((Idx_ + tokenlen_) >= scriptlen_)
                                {
                                    Finished = 1;
                                    Found = 1;
                                    Section = SECTION_PROCESSPARAMS;
                                }
                            }

                            if (Found and P_Idx_ < PARSE_MAX_PARAMS)
                            {
                                script_[Idx_ + tokenlen_] = 0;
                                P_[P_Idx_++] = FindID(&script_[Idx_]);

                                if (P_[P_Idx_ - 1] < 0 and strcmp(&script_[Idx_], "NID"))
                                    TokenErrorList->AddText(&script_[Idx_]);
                            }

                            Idx_ += tokenlen_ + 1;
                            tokenlen_ = 0;
                        }
                    }
                }

                break;

            case SECTION_PROCESSPARAMS:
                switch (TokenType)
                {
                    case TOKEN_COMMON:
                        Control_->BaseFunction(static_cast<short>(TokenID), P_, str_, Handler_);
                        break;

                    case TOKEN_LOCAL:
                        Control_->LocalFunction(static_cast<short>(TokenID), P_, str_, Handler_);
                        break;
                }

                P_Idx_ = 0;
                P_[0] = 0;
                P_[1] = 0;
                P_[2] = 0;
                P_[3] = 0;
                P_[4] = 0;
                P_[5] = 0;
                P_[6] = 0;
                P_[7] = 0;
                str_ = NULL;
                Section = SECTION_FINDSUBTOKEN;
                break;
        }
    }

    return(Control_);
}

C_Base *C_Parser::ParsePopupMenu(char *filename)
{
    //short Finished=0;
    short Done = 0, Comment = 0, Found = 0;
    short Section = 0, TokenType = 0;
    int InString = 0;
    long TokenID = 0;


    if (Popup_ == NULL)
    {
        Popup_ = new C_PopupMgr;
        Popup_->Setup(Handler_);
    }

    if (LoadScript(filename) == FALSE)
        return(FALSE);

    Idx_ = 0;
    P_Idx_ = 0;
    tokenlen_ = 0;

    Done = 0;
    Comment = 0;
    InString = 0;
    Section = SECTION_FINDTOKEN;
    TokenType = TOKEN_NOTHING;

    while ( not Done)
    {
        switch (Section)
        {
            case SECTION_FINDTOKEN:
                // Look for token starting with '['
                Found = 0;

                while ( not Found and not Done)
                {
                    switch (script_[Idx_])
                    {
                        case '[':
                            if ( not Comment and not InString)
                            {
                                Found = 1;
                                break;
                            }

                            Idx_++;
                            break;

                        case '"':
                            InString = 1 - InString;
                            Idx_++;
                            break;

                        case '#':
                            Comment = 1;
                            Idx_++;
                            break;

                        case 0x0a:
                        case 0x0d:
                            Comment = 0;
                            Idx_++;
                            break;

                        default:
                            Idx_++;
                    }

                    if (Idx_ >= scriptlen_)
                        Done = 1;
                }

                tokenlen_ = 0;

                while (script_[Idx_ + tokenlen_] not_eq ']' and (Idx_ + tokenlen_) < scriptlen_)
                    tokenlen_++;

                tokenlen_++;

                if ((Idx_ + tokenlen_) >= scriptlen_)
                {
                    Done = 1;
                    break;
                }

                if (Found == 1)
                    Section = SECTION_PROCESSTOKEN;

                break;

            case SECTION_PROCESSTOKEN:
                TokenID = FindToken(&script_[Idx_]);

                if (TokenID)
                {
                    switch (TokenID)
                    {
                        case CPARSE_POPUP:
                            return(PopupParser());
                            break;
                    }

                    Idx_ += tokenlen_;
                    tokenlen_ = 0;
                    Section = SECTION_FINDTOKEN;
                }
                else
                {
                    Section = SECTION_FINDTOKEN;
                    Idx_ += tokenlen_;
                    tokenlen_ = 0;
                }

                break;
        }
    }

    return(NULL);
}

C_SoundBite *C_Parser::ParseSoundBite(char *filename)
{
    C_SoundBite *Bite = NULL;
    short Done = 0, Comment = 0, Found = 0, InString = 0, Finished = 0;
    short Section = 0;

    if (LoadScript(filename) == FALSE)
        return(NULL);

    Bite = new C_SoundBite;

    if ( not Bite)
        return(NULL);

    Bite->Setup();

    Done = 0;
    Comment = 0;
    InString = 0;
    Section = SECTION_FINDPARAMS;

    while ( not Done)
    {
        switch (Section)
        {
            case SECTION_FINDPARAMS:
                P_Idx_ = 0; // start with 0 parameters

                // Repeat until token char '[' found (or EOF)
                Finished = 0;

                while ( not Finished)
                {
                    // Find NON white space
                    Found = 0;

                    while ( not Found and not Done and not Finished)
                    {
                        switch (script_[Idx_])
                        {
                            case ' ':
                            case ',':
                            case 0x09:
                                Idx_++;
                                break;

                            case 0x0a:
                            case 0x0d:
                                Comment = 0;
                                Finished = 1;
                                Section = SECTION_PROCESSPARAMS;
                                Idx_++;
                                break;

                            case '#': // Comment
                                Comment = 1;
                                Idx_++;
                                break;

                            default:
                                if ( not Comment)
                                {
                                    Found = 1;
                                    break;
                                }

                                Idx_++;
                                break;
                        }

                        if (Idx_ >= scriptlen_)
                        {
                            Finished = 1;
                            Section = SECTION_PROCESSPARAMS;
                        }
                    }

                    if (Found)
                    {
                        Found = 0;

                        if (script_[Idx_] == '"') // string
                        {
                            tokenlen_ = 1;
                            str_ = &script_[Idx_ + tokenlen_];

                            // Find closing (")
                            while ( not Found and not Finished)
                            {
                                if (script_[Idx_ + tokenlen_] == '"')
                                    Found = 1;
                                else
                                {
                                    if ((Idx_ + tokenlen_) >= scriptlen_)
                                    {
                                        Finished = 1;
                                        Section = SECTION_PROCESSPARAMS;
                                    }
                                    else
                                        tokenlen_++;
                                }
                            }

                            if (Found)
                                script_[Idx_ + tokenlen_] = 0; // make NULL terminated string

                            Idx_ += tokenlen_ + 1;
                            tokenlen_ = 0;
                        }
                        else if (isdigit(script_[Idx_]) or script_[Idx_] == '-') // Number
                        {
                            // find white space
                            Found = 0;
                            tokenlen_ = 1;

                            while ( not Found)
                            {
                                switch (script_[Idx_ + tokenlen_])
                                {
                                    case ' ':
                                    case ',':
                                    case 0x09:
                                    case 0x0a:
                                    case 0x0d:
                                        Found = 1;
                                        break;

                                    default:
                                        tokenlen_++;
                                        break;
                                }

                                if ((Idx_ + tokenlen_) >= scriptlen_)
                                {
                                    Found = 1;
                                    Finished = 1;
                                    Section = SECTION_PROCESSPARAMS;
                                }
                            }

                            if (Found and P_Idx_ < PARSE_MAX_PARAMS)
                                P_[P_Idx_++] = atol(&script_[Idx_]);

                            Idx_ += tokenlen_;
                            tokenlen_ = 0;
                        }
                        else
                        {
                            // Look for ID in tables
                            // Look for white space
                            Found = 0;
                            tokenlen_ = 0;

                            while ( not Found and not Finished)
                            {
                                switch (script_[Idx_ + tokenlen_])
                                {
                                    case ' ':
                                    case ',':
                                    case 0x09:
                                    case 0x0a:
                                    case 0x0d:
                                        Found = 1;
                                        break;

                                    default:
                                        tokenlen_++;
                                        break;
                                }

                                if ((Idx_ + tokenlen_) >= scriptlen_)
                                {
                                    Finished = 1;
                                    Found = 1;
                                    Section = SECTION_PROCESSPARAMS;
                                }
                            }

                            if (Found and P_Idx_ < PARSE_MAX_PARAMS)
                            {
                                script_[Idx_ + tokenlen_] = 0;
                                P_[P_Idx_++] = FindID(&script_[Idx_]);

                                if (P_[P_Idx_ - 1] < 0 and strcmp(&script_[Idx_], "NID"))
                                    TokenErrorList->AddText(&script_[Idx_]);
                            }

                            Idx_ += tokenlen_ + 1;
                            tokenlen_ = 0;
                        }
                    }
                }

                break;

            case SECTION_PROCESSPARAMS:
                if (P_Idx_)
                    Bite->Add(P_[0], P_[1]);

                P_Idx_ = 0;
                P_[0] = 0;
                P_[1] = 0;
                P_[2] = 0;
                P_[3] = 0;
                P_[4] = 0;
                P_[5] = 0;
                P_[6] = 0;
                P_[7] = 0;
                str_ = NULL;

                if (Idx_ >= scriptlen_)
                    Done = 1;

                Section = SECTION_FINDPARAMS;
                break;
        }
    }

    return(Bite);
}

void C_Parser::LogError(char *str)
{
    if (g_bLogUiErrors)
    {
        if (Perror_)
            fprintf(Perror_, "%s\n", str);
    }
}
#endif // this goes at end of THIS file
