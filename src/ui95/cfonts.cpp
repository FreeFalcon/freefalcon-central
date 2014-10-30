#include <windows.h>
#include "chandler.h"

#ifdef _UI95_PARSER_ // List of Keywords bitand functions to handle them

enum
{
    CFNT_NOTHING = 0,
    CFNT_ID,
    CFNT_lfHeight,
    CFNT_lfWidth,
    CFNT_lfEscapement,
    CFNT_lfOrientation,
    CFNT_lfWeight,
    CFNT_lfItalic,
    CFNT_lfUnderline,
    CFNT_lfStrikeOut,
    CFNT_lfCharSet,
    CFNT_lfOutPrecision,
    CFNT_lfClipPrecision,
    CFNT_lfQuality,
    CFNT_lfPitchAndFamily,
    CFNT_lfFaceName,
    CFNT_SPACING,
    CFNT_ADDFONT,
    CFNT_FONT,
    CFNT_LOAD
};

char *C_Fnt_Tokens[] =
{
    "[NOTHING]",
    "[ID]",
    "[lfHeight]",
    "[lfWidth]",
    "[lfEscapement]",
    "[lfOrientation]",
    "[lfWeight]",
    "[lfItalic]",
    "[lfUnderline]",
    "[lfStrikeOut]",
    "[lfCharSet]",
    "[lfOutPrecision]",
    "[lfClipPrecision]",
    "[lfQuality]",
    "[lfPitchAndFamily]",
    "[lfFaceName]",
    "[SPACING]",
    "[ADDFONT]",
    "[FONT]", // nothing done here
    "[LOADFONT]",
    0,
};

#endif

C_Font *gFontList = NULL;

static LOGFONT DefaultFont =
{
    16,
    0,
    0,
    0,
    FW_NORMAL,
    FALSE,
    FALSE,
    FALSE,
    ANSI_CHARSET,
    OUT_DEFAULT_PRECIS,
    CLIP_DEFAULT_PRECIS,
    PROOF_QUALITY,
    VARIABLE_PITCH bitor FF_SWISS,
    "",
};

static void HashDelCB(void *me)
{
    C_Fontmgr *fnt;

    fnt = (C_Fontmgr*)me;

    if (fnt)
    {
        fnt->Cleanup();
        delete fnt;
    }
}

C_Font::C_Font()
{
    Root_ = NULL;
    Spacing_ = 0;
    Fonts_ = NULL;
}

C_Font::~C_Font()
{
    if (Root_ or Fonts_)
        Cleanup();
}

void C_Font::Setup(C_Handler *handler)
{
    Handler_ = handler;
    Fonts_ = new C_Hash;
    Fonts_->Setup(5);
    Fonts_->SetFlags(C_BIT_REMOVE);
    Fonts_->SetCallback(HashDelCB);
}

void C_Font::Cleanup()
{
    FONTLIST *cur, *last;

    cur = Root_;

    while (cur)
    {
        last = cur;
        cur = cur->Next;
        DeleteObject(last->Font_);
        delete last->Widths_;
        delete last;
    }

    Root_ = NULL;

    if (Fonts_)
    {
        Fonts_->Cleanup();
        delete Fonts_;
        Fonts_ = NULL;
    }
}

BOOL C_Font::AddFont(long , LOGFONT *)
{

#if 0
    FONTLIST *newfont, *cur;
    HDC hdc;

    newfont = new FONTLIST;
    newfont->Spacing_ = Spacing_;
    newfont->Font_ = CreateFontIndirect(reqs);

    if (newfont->Font_ == NULL)
    {
        delete newfont;
        return(FALSE);
    }

    memcpy(&newfont->logfont, reqs, sizeof(LOGFONT));

    newfont->ID_ = ID;
    Handler_->GetDC(&hdc);
    SelectObject(hdc, newfont->Font_);
    GetTextMetrics(hdc, &newfont->Metrics_);
    newfont->Widths_ = new INT[newfont->Metrics_.tmLastChar + 1];

    if ( not GetCharWidth(hdc, 0, newfont->Metrics_.tmLastChar, &newfont->Widths_[0]))
    {
        VOID *lpMsgBuf;

        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER bitor FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);

        // Display the string.
        MessageBox(NULL, (char *)lpMsgBuf, "GetLastError", MB_OK bitor MB_ICONINFORMATION);

        // Free the buffer.
        LocalFree(lpMsgBuf);
    }

    Handler_->ReleaseDC(hdc);

    newfont->Next = NULL;

    if (Root_ == NULL)
    {
        Root_ = newfont;
    }
    else
    {
        cur = Root_;

        while (cur->Next)
            cur = cur->Next;

        cur->Next = newfont;
    }

    return(TRUE);
#endif
    return(FALSE);
}

FONTLIST *C_Font::FindID(long ID)
{
    FONTLIST *cur;

    cur = Root_;

    while (cur)
    {
        if (cur->ID_ == ID)
            return(cur);

        cur = cur->Next;
    }

    return(Root_);
}

HFONT C_Font::GetFont(long ID)
{
    FONTLIST *cur;

    cur = FindID(ID);

    if (cur)
        return(cur->Font_);

    return(NULL);
}

int C_Font::GetHeight(long ID)
{
#if 0 // Replacing OLD code
    FONTLIST *cur;

    cur = FindID(ID);

    if (cur)
        return(cur->Metrics_.tmHeight);

#else // New method
    C_Fontmgr *found;

    found = Find(ID);

    if (found)
        return(found->Height());

    found = Find(1);

    if (found)
        return(found->Height());

#endif
    return(0);
}

C_Fontmgr *C_Font::Find(long ID)
{
    C_Fontmgr *cur;

    if (Fonts_)
    {
        cur = (C_Fontmgr*)Fonts_->Find(ID);

        if ( not cur)
            cur = (C_Fontmgr*)Fonts_->Find(1);

        return(cur);
    }

    return(NULL);
}

void C_Font::LoadFont(long ID, char *filename)
{
    C_Fontmgr *newfont;

    if (Fonts_->Find(ID)) // Check to see if ID used
        return;

    newfont = new C_Fontmgr;
    newfont->Setup(ID, filename);

    if ( not newfont->Height())
    {
        delete newfont;
        return;
    }

    Fonts_->Add(ID, newfont);
}


int C_Font::StrWidth(long fontID, _TCHAR *Str)
{
#if 0 // Removing OLD method
    FONTLIST *cur;
    int w = 0, i;
    _TCHAR c;

    cur = FindID(fontID);

    if (cur)
    {
        i = 0;

        while (Str[i])
        {
            c = Str[i];

            if (c > cur->Metrics_.tmLastChar) c = 0;

            w += cur->Widths_[c] + cur->Spacing_;
            i++;
        }

        return(w + 1);
    }

    return(0);
#else
    C_Fontmgr *found;

    found = Find(fontID);

    if (found)
        return(found->Width(Str));

    return(0);
#endif
}

int C_Font::StrWidth(long fontID, _TCHAR *Str, int len)
{
#if 0 // Replacing OLD method
    FONTLIST *cur;
    int w = 0, i;
    _TCHAR c;

    cur = FindID(fontID);

    if (cur)
    {
        i = 0;

        while (Str[i] and i < len)
        {
            c = Str[i];

            if (c > cur->Metrics_.tmLastChar) c = 0;

            w += cur->Widths_[c] + cur->Spacing_;
            i++;
        }

        return(w + 1);
    }

    return(0);
#else // New method
    C_Fontmgr *found;

    found = Find(fontID);

    if (found)
        return(found->Width(Str, len));

    return(0);
#endif
}

#ifdef _UI95_PARSER_

short C_Font::FontFind(char *token)
{
    short i = 0;

    while (C_Fnt_Tokens[i])
    {
        if (strnicmp(token, C_Fnt_Tokens[i], strlen(C_Fnt_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_Font::FontFunction(short ID, long P[], _TCHAR *str, LOGFONT *lgfnt, long *NewID)
{
    switch (ID)
    {
        case CFNT_ID:
            *NewID = P[0];
            break;

        case CFNT_lfHeight:
            if (lgfnt)
                lgfnt->lfHeight or_eq P[0];

            break;

        case CFNT_lfWidth:
            if (lgfnt)
                lgfnt->lfWidth or_eq P[0];

            break;

        case CFNT_lfEscapement:
            if (lgfnt)
                lgfnt->lfEscapement or_eq P[0];

            break;

        case CFNT_lfOrientation:
            if (lgfnt)
                lgfnt->lfOrientation or_eq P[0];

            break;

        case CFNT_lfWeight:
            if (lgfnt)
                lgfnt->lfWeight or_eq P[0];

            break;

        case CFNT_lfItalic:
            if (lgfnt)
                lgfnt->lfItalic or_eq (BYTE)P[0];

            break;

        case CFNT_lfUnderline:
            if (lgfnt)
                lgfnt->lfUnderline or_eq (BYTE)P[0];

            break;

        case CFNT_lfStrikeOut:
            if (lgfnt)
                lgfnt->lfStrikeOut or_eq (BYTE)P[0];

            break;

        case CFNT_lfCharSet:
            if (lgfnt)
                lgfnt->lfCharSet or_eq (BYTE)P[0];

            break;

        case CFNT_lfOutPrecision:
            if (lgfnt)
                lgfnt->lfOutPrecision or_eq (BYTE)P[0];

            break;

        case CFNT_lfClipPrecision:
            if (lgfnt)
                lgfnt->lfClipPrecision or_eq (BYTE)P[0];

            break;

        case CFNT_lfQuality:
            if (lgfnt)
                lgfnt->lfQuality or_eq (BYTE)P[0];

            break;

        case CFNT_lfPitchAndFamily:
            if (lgfnt)
                lgfnt->lfPitchAndFamily or_eq (BYTE)P[0];

            break;

        case CFNT_lfFaceName:
            if (lgfnt)
                _tcsncpy(lgfnt->lfFaceName, str, LF_FACESIZE);

            break;

        case CFNT_SPACING:
            Spacing_ = P[0];
            break;

        case CFNT_ADDFONT:
            AddFont(*NewID, lgfnt);
            Spacing_ = 0;
            memset(lgfnt, 0, sizeof(LOGFONT));
            break;

        case CFNT_LOAD:
            LoadFont(P[0], str);
            break;
    }
}

#endif
