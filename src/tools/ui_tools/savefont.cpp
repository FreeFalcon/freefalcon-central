// Routines to export the Selected Font into Targa format... with rectangles bitand kerning pairs?

#include <windows.h>
#include "chandler.h"
#include "textids.h"
#include "userids.h"

extern HINSTANCE hInst;
extern C_Handler *gMainHandler;

static HWND mywin;

long CurrentChar = -1;
long CurrentFontID = 1;
void DeleteGroupList(long ID);

long ItemID = 1;
char FontList[200][32];
LOGFONT fontlog[200];

LOGFONT CreatedLogFont;
C_Fontmgr *CreatedFont = NULL;

void ChooseFontCB(long ID, short hittype, C_Base *control);

static long LF_TRUE_FALSE[] =
{
    0,
    0,
    1,
    -5551212,
};

static long LF_CHARSET[] =
{
    0,
    ANSI_CHARSET,
    OEM_CHARSET,
    SYMBOL_CHARSET,
    -5551212,
};

static long LF_OUT_PRECISION[] =
{
    0,
    OUT_CHARACTER_PRECIS,
    OUT_DEFAULT_PRECIS,
    OUT_STRING_PRECIS,
    OUT_STROKE_PRECIS,
    -5551212,
};

static long LF_CLIP_PRECISION[] =
{
    0,
    CLIP_CHARACTER_PRECIS,
    CLIP_DEFAULT_PRECIS,
    CLIP_STROKE_PRECIS,
    -5551212,
};

static long LF_QUALITY[] =
{
    0,
    DEFAULT_QUALITY,
    DRAFT_QUALITY,
    PROOF_QUALITY,
    -5551212,
};

static long LF_PITCH[] =
{
    0,
    DEFAULT_PITCH,
    FIXED_PITCH,
    VARIABLE_PITCH,
    -5551212,
};

static long LF_FAMILY[] =
{
    0,
    FF_DECORATIVE,
    FF_DONTCARE,
    FF_MODERN,
    FF_ROMAN,
    FF_SCRIPT,
    FF_SWISS,
    -5551212,
};

long SearchList(long val, long list[])
{
    int i;

    for (i = 1; list[i] not_eq -5551212; i++)
        if (list[i] == val)
            return(i);

    return(0);
}

void SetWindowLOGFONT(LOGFONT *log)
{
    C_Window *win;
    C_EditBox *ebox;
    C_ListBox *lbox;

    if ( not log)
        return;

    win = gMainHandler->FindWindow(LOGFONT_WIN);

    if (win)
    {
        ebox = (C_EditBox*)win->FindControl(EB_HEIGHT);

        if (ebox)
        {
            ebox->SetInteger(log->lfHeight);
        }

        ebox = (C_EditBox*)win->FindControl(EB_WIDTH);

        if (ebox)
        {
            ebox->SetInteger(log->lfWidth);
        }

        ebox = (C_EditBox*)win->FindControl(EB_ESCAPEMENT);

        if (ebox)
        {
            ebox->SetInteger(log->lfEscapement);
        }

        ebox = (C_EditBox*)win->FindControl(EB_ORIENTATION);

        if (ebox)
        {
            ebox->SetInteger(log->lfOrientation);
        }

        ebox = (C_EditBox*)win->FindControl(EB_WEIGHT);

        if (ebox)
        {
            ebox->SetInteger(log->lfWeight);
        }

        lbox = (C_ListBox*)win->FindControl(LB_ITALIC);

        if (lbox)
        {
            lbox->SetValue(SearchList(log->lfItalic, LF_TRUE_FALSE));
        }

        lbox = (C_ListBox*)win->FindControl(LB_UNDERLINE);

        if (lbox)
        {
            lbox->SetValue(SearchList(log->lfUnderline, LF_TRUE_FALSE));
        }

        lbox = (C_ListBox*)win->FindControl(LB_STRIKEOUT);

        if (lbox)
        {
            lbox->SetValue(SearchList(log->lfStrikeOut, LF_TRUE_FALSE));
        }

        lbox = (C_ListBox*)win->FindControl(LB_CHARSET);

        if (lbox)
        {
            lbox->SetValue(SearchList(log->lfCharSet, LF_CHARSET));
        }

        lbox = (C_ListBox*)win->FindControl(LB_OUTPRECISION);

        if (lbox)
        {
            lbox->SetValue(SearchList(log->lfOutPrecision, LF_OUT_PRECISION));
        }

        lbox = (C_ListBox*)win->FindControl(LB_CLIPPRECISION);

        if (lbox)
        {
            lbox->SetValue(SearchList(log->lfClipPrecision, LF_OUT_PRECISION));
        }

        lbox = (C_ListBox*)win->FindControl(LB_QUALITY);

        if (lbox)
        {
            lbox->SetValue(SearchList(log->lfQuality, LF_QUALITY));
        }

        lbox = (C_ListBox*)win->FindControl(LB_PITCH);

        if (lbox)
        {
            lbox->SetValue(SearchList(log->lfPitchAndFamily bitand 3, LF_PITCH));
        }

        lbox = (C_ListBox*)win->FindControl(LB_FAMILY);

        if (lbox)
        {
            lbox->SetValue(SearchList(log->lfPitchAndFamily bitand 0xfc, LF_FAMILY));
        }

        ebox = (C_EditBox*)win->FindControl(EB_FACENAME);

        if (ebox)
        {
            ebox->SetText(log->lfFaceName);
        }

        win->update_ or_eq C_DRAW_REFRESHALL;
        win->RefreshWindow();
    }
}

void GetWindowLOGFONT(LOGFONT *log)
{
    C_Window *win;
    C_EditBox *ebox;
    C_ListBox *lbox;

    if ( not log)
        return;

    win = gMainHandler->FindWindow(LOGFONT_WIN);

    if (win)
    {
        ebox = (C_EditBox*)win->FindControl(EB_HEIGHT);

        if (ebox)
        {
            log->lfHeight = ebox->GetInteger();
        }

        ebox = (C_EditBox*)win->FindControl(EB_WIDTH);

        if (ebox)
        {
            log->lfWidth = ebox->GetInteger();
        }

        ebox = (C_EditBox*)win->FindControl(EB_ESCAPEMENT);

        if (ebox)
        {
            log->lfEscapement = ebox->GetInteger();
        }

        ebox = (C_EditBox*)win->FindControl(EB_ORIENTATION);

        if (ebox)
        {
            log->lfOrientation = ebox->GetInteger();
        }

        ebox = (C_EditBox*)win->FindControl(EB_WEIGHT);

        if (ebox)
        {
            log->lfWeight = ebox->GetInteger();
        }

        lbox = (C_ListBox*)win->FindControl(LB_ITALIC);

        if (lbox)
        {
            log->lfItalic = static_cast<BYTE>(LF_TRUE_FALSE[lbox->GetTextID()]);
        }

        lbox = (C_ListBox*)win->FindControl(LB_UNDERLINE);

        if (lbox)
        {
            log->lfUnderline = static_cast<BYTE>(LF_TRUE_FALSE[lbox->GetTextID()]);
        }

        lbox = (C_ListBox*)win->FindControl(LB_STRIKEOUT);

        if (lbox)
        {
            log->lfStrikeOut = static_cast<BYTE>(LF_TRUE_FALSE[lbox->GetTextID()]);
        }

        lbox = (C_ListBox*)win->FindControl(LB_CHARSET);

        if (lbox)
        {
            log->lfCharSet = static_cast<BYTE>(LF_CHARSET[lbox->GetTextID()]);
        }

        lbox = (C_ListBox*)win->FindControl(LB_OUTPRECISION);

        if (lbox)
        {
            log->lfOutPrecision = static_cast<BYTE>(LF_OUT_PRECISION[lbox->GetTextID()]);
        }

        lbox = (C_ListBox*)win->FindControl(LB_CLIPPRECISION);

        if (lbox)
        {
            log->lfClipPrecision = static_cast<BYTE>(LF_CLIP_PRECISION[lbox->GetTextID()]);
        }

        lbox = (C_ListBox*)win->FindControl(LB_QUALITY);

        if (lbox)
        {
            log->lfQuality = static_cast<BYTE>(LF_QUALITY[lbox->GetTextID()]);
        }

        log->lfPitchAndFamily = 0;
        lbox = (C_ListBox*)win->FindControl(LB_PITCH);

        if (lbox)
        {
            log->lfPitchAndFamily or_eq LF_PITCH[lbox->GetTextID()];
        }

        lbox = (C_ListBox*)win->FindControl(LB_FAMILY);

        if (lbox)
        {
            log->lfPitchAndFamily or_eq LF_FAMILY[lbox->GetTextID()];
        }

        ebox = (C_EditBox*)win->FindControl(EB_FACENAME);

        if (ebox)
        {
            strcpy(log->lfFaceName, ebox->GetText());
        }
    }
}

// Enumerated fonts
int CALLBACK testme(CONST LOGFONTA *logfnt, CONST TEXTMETRICA *metrics, DWORD, LPARAM)
{
    if (ItemID < 200)
    {
        if (metrics->tmPitchAndFamily bitand TMPF_TRUETYPE)
        {
            memcpy(&fontlog[ItemID], logfnt, sizeof(LOGFONT));
            strcpy(FontList[ItemID], logfnt->lfFaceName);
            ItemID++;
        }
    }

    return(1);
}

void GetAllFonts(HDC hdc, char *facename)
{
    EnumFonts(hdc, facename, testme, 0);
}

#define SCREEN_SIZE (500)

C_Fontmgr *FontToBFT(long ID, LOGFONT *logfont)
{
    FONTLIST *font;
    char buffer[100];
    long firstchar;
    long lastchar;
    long numchars;
    long DataSize;
    long KernSize;
    long xoff, yoff;
    long i, j, x, y;
    long charw, charh;
    long bytesperline;
    unsigned char *CharData, *CharPtr;
    HDC hdc;
    KERNINGPAIR *kerns;
    KerningStr *mykerns;
    ABC *ABCList;
    CharStr *MyChars;
    C_Fontmgr *newfont;

    // surface=UI95_CreateDDSurface(gMainDDraw,SCREEN_SIZE,SCREEN_SIZE);
    // if( not surface)
    // {
    // MonoPrint("Can't create DDSurface for Font conversion to BFT\n");
    // return(NULL);
    // }

    font = new FONTLIST;
    font->Font_ = CreateFontIndirect(logfont);

    if (font->Font_ == NULL)
    {
        delete font;
        return(NULL);
    }

    font->ID_ = ID;
    hdc = GetDC(mywin);
    SelectObject(hdc, font->Font_);
    GetTextFace(hdc, 99, buffer);
    GetTextMetrics(hdc, &font->Metrics_);
    ReleaseDC(mywin, hdc);

    // Get Kerning tables
    kerns = new KERNINGPAIR[256 * 256];
    mykerns = new KerningStr[256 * 256];
    ABCList = new ABC[256];

    hdc = GetDC(mywin);
    SelectObject(hdc, font->Font_);
    KernSize = GetKerningPairs(hdc, 65536, kerns);
    GetCharABCWidths(hdc, font->Metrics_.tmFirstChar, font->Metrics_.tmLastChar, ABCList);
    ReleaseDC(mywin, hdc);

    for (i = 0; i < KernSize; i++)
    {
        mykerns[i].first  = kerns[i].wFirst;
        mykerns[i].second = kerns[i].wSecond;
        mykerns[i].add    = static_cast<short>(kerns[i].iKernAmount);
    }

    delete kerns;
    // Generate font table

    firstchar = font->Metrics_.tmFirstChar;
    lastchar = font->Metrics_.tmLastChar;
    numchars = lastchar - firstchar + 1;

    MyChars = new CharStr[numchars];
    memset(MyChars, 0, sizeof(CharStr)*numchars);

    // Check All Kerning
    for (j = 0; j < KernSize; j++)
    {
        for (i = 0; i < numchars; i++)
            if ((firstchar + i) == mykerns[i].first)
                MyChars[i].flags = _FNT_CHECK_KERNING_;
    }

    for (i = 0; i < numchars; i++)
    {
        MyChars[i].lead = static_cast<char>(ABCList[i].abcA);
        MyChars[i].w = static_cast<BYTE>(ABCList[i].abcB);
        MyChars[i].trail = static_cast<char>(ABCList[i].abcC);
    }

    delete ABCList;

    // Create Font Resource

    newfont = new C_Fontmgr;
    newfont->SetID(ID);
    newfont->SetName(buffer);
    newfont->SetHeight(font->Metrics_.tmHeight);
    bytesperline = font->Metrics_.tmMaxCharWidth / 8 + 1;
    newfont->SetBPL(bytesperline);
    newfont->SetRange(firstchar, lastchar);
    newfont->SetTable(numchars, MyChars);
    newfont->SetKerning(KernSize, mykerns);
    CharData = new unsigned char[bytesperline * font->Metrics_.tmHeight * numchars];
    DataSize = numchars * font->Metrics_.tmHeight * bytesperline;
    newfont->SetData(DataSize, (char *)CharData);
    memset(CharData, 0, DataSize);

    CharPtr = CharData;

    buffer[0] = 0;
    buffer[1] = 0;
    hdc = GetDC(mywin);

    // Clear surface
    for (y = 0; y < SCREEN_SIZE; y++)
        for (x = 0; x < SCREEN_SIZE; x++)
            SetPixel(hdc, x, y, 0);

    for (i = 0; i < numchars; i++)
    {
        xoff = 10;
        yoff = 10;

        SelectObject(hdc, font->Font_);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, 0xffffff);

        buffer[0] = static_cast<char>(firstchar + i);

        if ( not TextOut(hdc, xoff, yoff, buffer, 1))
            MonoPrint("Failed to display character (%1d)\n", buffer[0]);

        xoff += MyChars[i].lead;
        charw = MyChars[i].w;
        charh = font->Metrics_.tmHeight;

        // Copy pixels that have been set
        // Use GetPixel(x,y) instead
        for (y = 0; y < charh; y++)
            for (x = 0; x < charw; x++)
                if (GetPixel(hdc, xoff + x, yoff + y))
                {
                    CharPtr[y * bytesperline + x / 8] or_eq 1 << (x % 8);
                    SetPixel(hdc, xoff + x, yoff + y, 0);
                }

        CharPtr += bytesperline * font->Metrics_.tmHeight;
    }

    ReleaseDC(mywin, hdc);

    return(newfont);
}

void ChoosePairCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    control->Refresh();
}

void PrintPairs(C_Window *win, long FontID)
{
    C_Button *btn;
    _TCHAR buffer[10];
    long i, j, numchars, startchar;
    C_Fontmgr *curfont;
    long charw, btnid;
    long x, y, numperline;

    curfont = gFontList->Find(FontID);

    if ( not curfont)
        return;

    startchar = curfont->First();
    numchars = curfont->Last() - startchar;
    charw = curfont->Width("M");
    numperline = (win->ClientArea_[2].right - win->ClientArea_[2].left) / (charw * 4);

    x = 5;
    y = 5;

    btnid = KERN_CHAR_BASE + startchar;

    for (i = 0; i < numchars; i++)
        win->RemoveControl(btnid + i);

    j = 0;

    for (i = 0; i < numchars; i++)
    {
        buffer[0] = static_cast<char>(CurrentChar);
        buffer[1] = static_cast<char>(startchar + i);
        buffer[2] = 0;

        btn = new C_Button;
        btn->Setup(btnid + i, C_TYPE_RADIO, x, y);
        btn->SetGroup(-100);
        btn->SetFont(FontID);
        btn->SetText(C_STATE_0, gStringMgr->GetText(gStringMgr->AddText(buffer)));
        btn->SetText(C_STATE_1, gStringMgr->GetText(gStringMgr->AddText(buffer)));
        btn->SetColor(C_STATE_0, 0xcccccc);
        btn->SetColor(C_STATE_1, 0x00ff00);
        btn->SetClient(2);
        btn->SetCallback(ChoosePairCB);
        btn->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);

        win->AddControl(btn);

        x += charw * 4;
        j++;

        if (j >= numperline)
        {
            x = 5;
            y += curfont->Height() + 5;
            j = 0;
        }
    }
}

void IncreaseLead(long, short hittype, C_Base *control)
{
    C_Fontmgr *cur;
    CharStr *chr;
    C_Text *txt;
    char buffer[10];

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    cur = gFontList->Find(CurrentFontID);

    if (cur)
    {
        if (CurrentChar >= cur->First() and CurrentChar <= cur->Last())
        {
            txt = (C_Text*)control->Parent_->FindControl(REFUEL_1);

            if (txt)
            {
                chr = cur->GetChar(static_cast<char>(CurrentChar));
                chr->lead++;
                sprintf(buffer, "%1d", chr->lead);
                txt->SetText(buffer);
                txt->Refresh();
            }

            control->Parent_->update_ or_eq C_DRAW_REFRESHALL;
            control->Parent_->RefreshWindow();
        }
    }
}

void DecreaseLead(long , short hittype, C_Base *control)
{
    C_Fontmgr *cur;
    CharStr *chr;
    C_Text *txt;
    char buffer[10];

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    cur = gFontList->Find(CurrentFontID);

    if (cur)
    {
        if (CurrentChar >= cur->First() and CurrentChar <= cur->Last())
        {
            txt = (C_Text*)control->Parent_->FindControl(REFUEL_1);

            if (txt)
            {
                chr = cur->GetChar(static_cast<short>(CurrentChar));
                chr->lead--;
                sprintf(buffer, "%1d", chr->lead);
                txt->SetText(buffer);
                txt->Refresh();
            }

            control->Parent_->update_ or_eq C_DRAW_REFRESHALL;
            control->Parent_->RefreshWindow();
        }
    }
}

void IncreaseTrail(long, short hittype, C_Base *control)
{
    C_Fontmgr *cur;
    CharStr *chr;
    C_Text *txt;
    char buffer[10];

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    cur = gFontList->Find(CurrentFontID);

    if (cur)
    {
        if (CurrentChar >= cur->First() and CurrentChar <= cur->Last())
        {
            txt = (C_Text*)control->Parent_->FindControl(REFUEL_3);

            if (txt)
            {
                chr = cur->GetChar(static_cast<short>(CurrentChar));
                chr->trail++;
                sprintf(buffer, "%1d", chr->trail);
                txt->SetText(buffer);
                txt->Refresh();
            }

            control->Parent_->update_ or_eq C_DRAW_REFRESHALL;
            control->Parent_->RefreshWindow();
        }
    }
}

void DecreaseTrail(long, short hittype, C_Base *control)
{
    C_Fontmgr *cur;
    CharStr *chr;
    C_Text *txt;
    char buffer[10];

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    cur = gFontList->Find(CurrentFontID);

    if (cur)
    {
        if (CurrentChar >= cur->First() and CurrentChar <= cur->Last())
        {
            txt = (C_Text*)control->Parent_->FindControl(REFUEL_3);

            if (txt)
            {
                chr = cur->GetChar(static_cast<short>(CurrentChar));
                chr->trail--;
                sprintf(buffer, "%1d", chr->trail);
                txt->SetText(buffer);
                txt->Refresh();
            }

            control->Parent_->update_ or_eq C_DRAW_REFRESHALL;
            control->Parent_->RefreshWindow();
        }
    }
}

void IncreaseWidth(long, short hittype, C_Base *control)
{
    C_Fontmgr *cur;
    CharStr *chr;
    C_Text *txt;
    char buffer[10];

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    cur = gFontList->Find(CurrentFontID);

    if (cur)
    {
        if (CurrentChar >= cur->First() and CurrentChar <= cur->Last())
        {
            txt = (C_Text*)control->Parent_->FindControl(REFUEL_2);

            if (txt)
            {
                chr = cur->GetChar(static_cast<short>(CurrentChar));
                chr->w++;
                sprintf(buffer, "%1d", chr->w);
                txt->SetText(buffer);
                txt->Refresh();
            }

            control->Parent_->update_ or_eq C_DRAW_REFRESHALL;
            control->Parent_->RefreshWindow();
        }
    }
}

void DecreaseWidth(long, short hittype, C_Base *control)
{
    C_Fontmgr *cur;
    CharStr *chr;
    C_Text *txt;
    char buffer[10];

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    cur = gFontList->Find(CurrentFontID);

    if (cur)
    {
        if (CurrentChar >= cur->First() and CurrentChar <= cur->Last())
        {
            txt = (C_Text*)control->Parent_->FindControl(REFUEL_2);

            if (txt)
            {
                chr = cur->GetChar(static_cast<short>(CurrentChar));
                chr->w--;
                sprintf(buffer, "%1d", chr->w);
                txt->SetText(buffer);
                txt->Refresh();
            }

            control->Parent_->update_ or_eq C_DRAW_REFRESHALL;
            control->Parent_->RefreshWindow();
        }
    }
}

void IncreaseKern(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;
}

void DecreaseKern(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;
}

void ChooseCharCB(long ID, short hittype, C_Base *control)
{
    C_Fontmgr *fnt;
    CharStr   *chr;
    C_Text *txt;
    char buffer[10];

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    CurrentChar = ID - FONT_CHAR_BASE;
    PrintPairs(control->Parent_, CurrentFontID);
    control->Refresh();
    fnt = gFontList->Find(CurrentFontID);

    if (fnt and CurrentChar  > 0)
    {
        chr = fnt->GetChar(static_cast<short>(CurrentChar));

        if (chr)
        {
            sprintf(buffer, "%1d", chr->lead);
            txt = (C_Text*)control->Parent_->FindControl(REFUEL_1);

            if (txt)
            {
                txt->SetText(buffer);
                txt->Refresh();
            }

            sprintf(buffer, "%1d", chr->w);
            txt = (C_Text*)control->Parent_->FindControl(REFUEL_2);

            if (txt)
            {
                txt->SetText(buffer);
                txt->Refresh();
            }

            sprintf(buffer, "%1d", chr->trail);
            txt = (C_Text*)control->Parent_->FindControl(REFUEL_3);

            if (txt)
            {
                txt->SetText(buffer);
                txt->Refresh();
            }
        }
        else
        {
            txt = (C_Text*)control->Parent_->FindControl(REFUEL_1);

            if (txt)
            {
                txt->SetText(" ");
                txt->Refresh();
            }

            txt = (C_Text*)control->Parent_->FindControl(REFUEL_2);

            if (txt)
            {
                txt->SetText(" ");
                txt->Refresh();
            }

            txt = (C_Text*)control->Parent_->FindControl(REFUEL_3);

            if (txt)
            {
                txt->SetText(" ");
                txt->Refresh();
            }
        }
    }
    else
    {
        txt = (C_Text*)control->Parent_->FindControl(REFUEL_1);

        if (txt)
        {
            txt->SetText(" ");
            txt->Refresh();
        }

        txt = (C_Text*)control->Parent_->FindControl(REFUEL_2);

        if (txt)
            if (txt)
            {
                txt->SetText(" ");
                txt->Refresh();
            }

        txt = (C_Text*)control->Parent_->FindControl(REFUEL_3);

        if (txt)
            if (txt)
            {
                txt->SetText(" ");
                txt->Refresh();
            }
    }
}

void MakeFontList(long FontID)
{
    C_Window *win;
    C_EditBox *ebox;
    long i, j, x, y;
    long startchar, numchars;
    long charw;
    long btnid;
    C_Fontmgr *curfont;
    _TCHAR buffer[120];
    long numperline;
    C_Button *btn;
    C_Text *txt;
    F4CSECTIONHANDLE *Leave;

    win = gMainHandler->FindWindow(FONT_ED_WIN);

    if ( not win or not FontID)
        return;

    curfont = gFontList->Find(FontID);

    if ( not curfont)
        return;

    Leave = UI_Enter(win);

    ebox = (C_EditBox*)win->FindControl(FONT_NAME_BOX);

    if (ebox)
        ebox->SetFont(FontID);

    DeleteGroupList(FONT_ED_WIN);

    charw = curfont->Width("M");
    numperline = (win->ClientArea_[1].right - win->ClientArea_[1].left) / (charw + 60);
    startchar = curfont->First();
    numchars = curfont->Last() - startchar;

    if (CurrentChar < startchar)
        CurrentChar = startchar;

    if (CurrentChar >= (startchar - numchars))
        CurrentChar = startchar;

    txt = (C_Text*)win->FindControl(FONT_NAME);

    if (txt)
    {
        txt->SetText(curfont->GetName());
        txt->Refresh();
    }

    _stprintf(buffer, "%1ld", curfont->Height());
    txt = (C_Text*)win->FindControl(PITCH_SIZE);

    if (txt)
    {
        txt->SetText(buffer);
        txt->Refresh();
    }

    x = 5;
    y = 5;

    btnid = FONT_CHAR_BASE + startchar;

    j = 0;

    for (i = 0; i < numchars; i++)
    {
        buffer[0] = static_cast<char>(startchar + i);
        buffer[1] = 0;

        btn = new C_Button;
        btn->Setup(btnid + i, C_TYPE_RADIO, x, y);
        btn->SetGroup(-100);
        btn->SetFont(FontID);
        btn->SetText(C_STATE_0, gStringMgr->GetText(gStringMgr->AddText(buffer)));
        btn->SetText(C_STATE_1, gStringMgr->GetText(gStringMgr->AddText(buffer)));
        sprintf(buffer, "[%1ld]", startchar + i);
        btn->SetAllLabel(gStringMgr->GetText(gStringMgr->AddText(buffer)));
        btn->SetLabelOffset(C_STATE_0, charw + 5, 0);
        btn->SetLabelOffset(C_STATE_1, charw + 5, 0);
        btn->SetColor(C_STATE_0, 0xcccccc);
        btn->SetColor(C_STATE_1, 0x00ff00);
        btn->SetClient(1);
        btn->SetCallback(ChooseCharCB);
        btn->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);

        win->AddControl(btn);

        x += charw + charw * 6;

        if (x >= win->ClientArea_[1].right - charw * 4)
        {
            x = 5;
            y += curfont->Height() + 5;
        }
    }

    PrintPairs(win, FontID);

    UI_Leave(Leave);
    win->ScanClientAreas();
    win->update_ or_eq C_DRAW_REFRESHALL;
    win->RefreshWindow();
}

void CreateTheFontCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (CreatedFont)
    {
        if (CreatedFont->GetID() == 100)
            gFontList->GetHash()->Remove(100);

        CreatedFont = NULL;
    }

    GetWindowLOGFONT(&CreatedLogFont);
    CreatedFont = FontToBFT(100, &CreatedLogFont);

    if (CreatedFont)
    {
        gFontList->GetHash()->Add(100, CreatedFont);
        ChooseFontCB(100, C_TYPE_SELECT, NULL);
    }
}

void CreateFontCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gMainHandler->EnableWindowGroup(-200);
}

void SaveFontCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    C_Window *win;
    C_EditBox *ebox;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    win = control->Parent_;

    if (win)
    {
        ebox = (C_EditBox*)win->FindControl(FONT_NAME_BOX);

        if (ebox)
        {
            if (CreatedFont)
                CreatedFont->Save(ebox->GetText());
        }
    }
}

void ChooseFontCB(long ID, short hittype, C_Base *control)
{
    C_ListBox *lbox;

    if (hittype not_eq C_TYPE_SELECT)
        return;

    if (control)
    {
        lbox = (C_ListBox*)control;
        CurrentFontID = lbox->GetTextID();
    }
    else
        CurrentFontID = ID;

    MakeFontList(CurrentFontID);
}

void ChooseAvailableFontCB(long, short hittype, C_Base *control)
{
    int idx;

    if (hittype not_eq C_TYPE_SELECT)
        return;

    idx = ((C_ListBox*)control)->GetTextID();
    memcpy(&CreatedLogFont, &fontlog[idx], sizeof(LOGFONT));
    SetWindowLOGFONT(&CreatedLogFont);
}

void InitFontTool()
{
    C_Window *win;
    C_ListBox *lbox;
    HDC hdc;
    int i;
    WNDCLASSEX myclass;
    static char FontWinName[] = "Font Window";

    myclass.cbSize = sizeof(myclass);
    myclass.style = CS_HREDRAW bitor CS_VREDRAW;
    myclass.lpfnWndProc = NULL;
    myclass.cbClsExtra = 0;
    myclass.cbWndExtra = 0;
    myclass.hInstance = hInst;
    myclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    myclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    myclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    myclass.lpszMenuName = NULL;
    myclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    RegisterClassEx(&myclass);
    mywin = CreateWindow(FontWinName, "Font Window",
                         WS_OVERLAPPEDWINDOW,
                         300,
                         300,
                         SCREEN_SIZE,
                         SCREEN_SIZE,
                         NULL,
                         NULL,
                         hInst,
                         NULL);

    ShowWindow(mywin, 0);
    UpdateWindow(mywin);

    win = gMainHandler->FindWindow(LOGFONT_WIN);

    if (win)
    {
        ItemID = 1;
        hdc = GetDC(mywin);
        GetAllFonts(hdc, NULL);
        ReleaseDC(mywin, hdc);

        lbox = (C_ListBox *)win->FindControl(LB_ENUM_FONTS);

        if (lbox)
        {
            lbox->RemoveAllItems();
            lbox->SetCallback(ChooseAvailableFontCB);

            for (i = 1; i < ItemID; i++)
                lbox->AddItem(i, C_TYPE_ITEM, FontList[i]);
        }
    }

    ChooseFontCB(1, C_TYPE_SELECT, NULL);
}
