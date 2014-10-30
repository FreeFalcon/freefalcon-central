#include <windows.h>
#include "chandler.h"

#ifdef _UI95_PARSER_ // List of Keywords bitand functions to handle them
char g_sVersion[100];

enum
{
    CTXT_NOTHING = 0,
    CTXT_SETUP,
    CTXT_SETFGCOLOR,
    CTXT_SETBGCOLOR,
    CTXT_SETBGIMAGE,
    CTXT_SETTEXT,
    CTXT_SETFIXEDWIDTH,
};

char *C_Txt_Tokens[] =
{
    "[NOTHING]",
    "[SETUP]",
    "[FGCOLOR]",
    "[BGCOLOR]",
    "[BGIMAGE]",
    "[SETTEXT]",
    "[FIXEDWIDTH]",
    0,
};

#endif

C_Text::C_Text() : C_Base()
{
    _SetCType_(_CNTL_TEXT_);
    Text_ = NULL;
    FixedSize_ = 0;
    TimerCallback_ = NULL;
    BgImage_ = NULL;
    DefaultFlags_ = C_BIT_ENABLED bitor C_BIT_REMOVE;
}

C_Text::C_Text(char **stream) : C_Base(stream)
{
}

C_Text::C_Text(FILE *fp) : C_Base(fp)
{
}

C_Text::~C_Text()
{
    if (Text_)
        Cleanup();
}

long C_Text::Size()
{
    return(0);
}

void C_Text::Setup(long ID, short Type)
{
    SetID(ID);
    SetType(Type);
    SetDefaultFlags();

    if (Text_ == NULL)
    {
        Text_ = new O_Output;
        Text_->SetOwner(this);
    }
}

void C_Text::SetFixedWidth(long w)
{
    // If you get this assert, some one called SetFixedWidth (or [FIXEDWIDTH] n ... in the scrips)
    // AFTER assigning a string to the control
    // THIS is a BIG NO NO... unless it started out as fixed width PRIOR to SetText, bitand the string size
    // is just being changed
    if ( not (Flags_ bitand C_BIT_FIXEDSIZE))
    {
        F4Assert( not Text_->GetText());
    }

    FixedSize_ = w + 1;
    Text_->SetTextWidth(FixedSize_);
    Flags_ or_eq C_BIT_FIXEDSIZE;
}

void C_Text::SetText(_TCHAR *text)
{
    F4CSECTIONHANDLE* Leave;
    Leave = UI_Enter(Parent_);

    if (Text_)
    {
        if (Flags_ bitand C_BIT_FIXEDSIZE)
            Text_->SetText(text);
        else
            SetTextID(gStringMgr->GetText(gStringMgr->AddText(text)));

        Text_->SetFlags(GetFlags());

        if (GetFlags() bitand C_BIT_WORDWRAP)
            SetH(Text_->GetH());
        else
            SetWH(Text_->GetW(), Text_->GetH());
    }

    SetReady(1);
    UI_Leave(Leave);
}

void C_Text::SetTextID(_TCHAR *text)
{
    F4CSECTIONHANDLE* Leave;
    Leave = UI_Enter(Parent_);

    if (Text_)
    {
        Text_->SetText(text);
        Text_->SetFlags(GetFlags());

        if (GetFlags() bitand C_BIT_WORDWRAP)
            SetH(Text_->GetH());
        else
            SetWH(Text_->GetW(), Text_->GetH());
    }

    SetReady(1);
    UI_Leave(Leave);
}

void C_Text::SetText(long txtID)
{
    SetTextID(gStringMgr->GetString(txtID));
}

BOOL C_Text::TimerUpdate()
{
    if (TimerCallback_)
        return(TimerCallback_(this));

    return(FALSE);
}

void C_Text::Cleanup(void)
{
    if (BgImage_)
    {
        BgImage_->Cleanup();
        delete BgImage_;
        BgImage_ = NULL;
    }

    if (Text_)
    {
        Text_->Cleanup();
        delete Text_;
        Text_ = NULL;
    }
}

void C_Text::SetBgImage(long ImageID, short x, short y)
{
    if (BgImage_ == NULL)
    {
        BgImage_ = new O_Output;
        BgImage_->SetOwner(this);
    }

    BgImage_->SetFlags(GetFlags());
    BgImage_->SetImage(ImageID);
    BgImage_->SetXY(x, y);
    SetFlagBitOn(C_BIT_USEBGIMAGE);
}

void C_Text::SetFGColor(COLORREF fore)
{
    if (Text_)
        Text_->SetFgColor(fore);
}

void C_Text::SetBGColor(COLORREF back)
{
    if (Text_)
        Text_->SetBgColor(back);
}

void C_Text::SetFont(long FontID)
{
    Font_ = FontID;

    if (Text_)
        Text_->SetFont(FontID);
}

void C_Text::SetFlags(long flags)
{
    SetControlFlags(flags);

    if (BgImage_)
        BgImage_->SetFlags(flags);

    if (Text_)
        Text_->SetFlags(flags);
}

void C_Text::Refresh()
{
    if ( not Ready() or GetFlags() bitand C_BIT_INVISIBLE or Parent_ == NULL)
        return;

    if (BgImage_ and GetFlags() bitand C_BIT_USEBGIMAGE)
        BgImage_->Refresh();

    if (Text_)
        Text_->Refresh();
}

void C_Text::Draw(SCREEN *surface, UI95_RECT *cliprect)
{
    if (GetFlags() bitand C_BIT_INVISIBLE)
        return;

    if ( not (GetFlags() bitand C_BIT_ABSOLUTE))
        if ((GetY() + GetH() + Parent_->VY_[GetClient()]) < Parent_->ClientArea_[GetClient()].top)
            return;

    if (BgImage_ and GetFlags() bitand C_BIT_USEBGIMAGE)
        BgImage_->Draw(surface, cliprect);

    if (Text_)
        Text_->Draw(surface, cliprect);
}

void C_Text::SetSubParents(C_Window *)
{
    if (BgImage_)
        BgImage_->SetInfo();

    if (Text_)
    {
        Text_->SetFlags(Flags_);
        Text_->SetInfo();

        if (GetFlags() bitand C_BIT_WORDWRAP)
            SetH(Text_->GetH());
        else
            SetWH(Text_->GetW(), Text_->GetH());
    }
}

void C_VersionText::Setup(long id, short type)
{
    C_Text::Setup(id, type);

    // query file version
    // get size of version info
    BYTE *lpVersionData;
    DWORD dwDataSize = GetFileVersionInfoSize("FFViper.exe", 0);

    if (dwDataSize == 0)
    {
        DWORD dwDataSize = GetFileVersionInfoSize("FFViper.exe", 0);
        //get the version info
        lpVersionData = new BYTE[dwDataSize];
        GetFileVersionInfo("FFViper.exe", 0, dwDataSize, (void**)lpVersionData);
    }
    else
    {
        //get the version info
        lpVersionData = new BYTE[dwDataSize];
        GetFileVersionInfo("FFViper.exe", 0, dwDataSize, (void**)lpVersionData);
    }

    //find translation table
    UINT nQuerySize;
    DWORD* pTransTable;
    VerQueryValue(lpVersionData, _T("\\VarFileInfo\\Translation"), (void **)&pTransTable, &nQuerySize);

    // Swap the words to have lang-charset in the correct format
    DWORD dwLangCharset = MAKELONG(HIWORD(pTransTable[0]), LOWORD(pTransTable[0]));

    //perform the query
    LPVOID lpData;
    char strBlockName[100];
    sprintf(strBlockName, "\\StringFileInfo\\%08lx\\FileVersion", dwLangCharset);
    VerQueryValue((void **)lpVersionData, strBlockName, &lpData, &nQuerySize);

    //format the version string
    //char sVersion[100];
    // sfr: using MP version right now
    sprintf(g_sVersion, "FFViper : %s", (char *)lpData);
    //sprintf(sVersion,"FFViper MP version");

    C_Text::SetText(g_sVersion);
}

#ifdef _UI95_PARSER_
short C_Text::LocalFind(char *token)
{
    short i = 0;

    while (C_Txt_Tokens[i])
    {
        if (strnicmp(token, C_Txt_Tokens[i], strlen(C_Txt_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_Text::LocalFunction(short ID, long P[], _TCHAR *, C_Handler *)
{
    switch (ID)
    {
        case CTXT_SETUP:
            Setup(P[0], (short)P[1]);
            break;

        case CTXT_SETFGCOLOR:
            SetFGColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CTXT_SETBGCOLOR:
            SetBGColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CTXT_SETBGIMAGE:
            SetBgImage(P[0], (short)P[1], (short)P[2]);
            break;

        case CTXT_SETTEXT:
            SetText(P[0]);
            break;

        case CTXT_SETFIXEDWIDTH:
            SetFixedWidth(P[0]);
            break;
    }
}

extern char ParseSave[];
extern char ParseCRLF[];

#endif // PARSER
