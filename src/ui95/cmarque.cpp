#include <windows.h>
#include "chandler.h"

#ifdef _UI95_PARSER_ // List of Keywords bitand functions to handle them

enum
{
    CMRQ_NOTHING = 0,
    CMRQ_SETUP,
    CMRQ_SETFGCOLOR,
    CMRQ_SETBGCOLOR,
    CMRQ_SETFONT,
    CMRQ_SETBGIMAGE,
    CMRQ_SETDIRECTION,
};

char *C_Mrq_Tokens[] =
{
    "[NOTHING]",
    "[SETUP]",
    "[FGCOLOR]",
    "[BGCOLOR]",
    "[Font]",
    "[BGIMAGE]",
    "[DIRECTION]",
    0,
};

#endif

C_Marque::C_Marque() : C_Base()
{
    _SetCType_(_CNTL_MARQUE_);
    Position_ = 0;
    Direction_ = -2; // move 10 pixels to left per frame
    MarqueLen_ = -1;
    Text_ = NULL;
    BgImage_ = NULL;
    DefaultFlags_ = C_BIT_ENABLED bitor C_BIT_REMOVE bitor C_BIT_TIMER;
}

C_Marque::C_Marque(char **stream) : C_Base(stream)
{
}

C_Marque::C_Marque(FILE *fp) : C_Base(fp)
{
}

C_Marque::~C_Marque()
{
    if (Text_)
        Cleanup();
}

long C_Marque::Size()
{
    return(0);
}

void C_Marque::Setup(long ID, short Type, _TCHAR *text)
{
    SetID(ID);
    SetType(Type);
    SetDefaultFlags();
    Direction_ = -2; // move 2 pixels to left per frame
    MarqueLen_ = -1;
    Position_ = 0;

    SetText(text);
}

void C_Marque::Setup(long ID, short Type, long txtID)
{
    Setup(ID, Type, gStringMgr->GetString(txtID));
}

void C_Marque::SetText(_TCHAR *text)
{
    Position_ = 10;

    if (Text_ == NULL)
    {
        Text_ = new O_Output;
        Text_->SetOwner(this);
    }

    Text_->SetFont(Font_);
    Text_->SetFlags(GetFlags());
    Text_->SetText(text);
    MarqueLen_ = Text_->GetW();
    SetReady(1);
}

void C_Marque::SetText(long txtID)
{
    Position_ = 10;
    SetText(gStringMgr->GetString(txtID));
}

void C_Marque::Cleanup(void)
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

void C_Marque::SetFont(long FontID)
{
    Font_ = FontID;

    if (Text_)
        Text_->SetFont(FontID);
}

void C_Marque::SetFlags(long flags)
{
    SetControlFlags(flags);

    if (Text_)
        Text_->SetFlags(flags);
}

void C_Marque::SetFGColor(COLORREF fore)
{
    if (Text_)
        Text_->SetFgColor(fore);
}

void C_Marque::SetBGColor(COLORREF back)
{
    if (Text_)
        Text_->SetBgColor(back);
}

void C_Marque::SetBGImage(long ImageID)
{
    if (BgImage_ == NULL)
    {
        BgImage_ = new O_Output;
        BgImage_->SetOwner(this);
    }

    BgImage_->SetImage(ImageID);

    if (BgImage_->Ready())
        SetFlagBitOn(C_BIT_USEBGIMAGE);
    else
        SetFlagBitOff(C_BIT_USEBGIMAGE);
}

void C_Marque::Refresh()
{
    if ( not Ready() or GetFlags() bitand C_BIT_INVISIBLE or Parent_ == NULL)
        return;

    Parent_->SetUpdateRect(GetX(), GetY(), GetX() + GetW(), GetY() + GetH(), GetFlags(), GetClient());
}

void C_Marque::Draw(SCREEN *surface, UI95_RECT *cliprect)
{
    UI95_RECT dummy, rect;

    if ( not Ready()) return;

    if (GetFlags() bitand C_BIT_INVISIBLE)
        return;

    if ( not (GetFlags() bitand C_BIT_ENABLED))
        return;

    rect.left = GetX();
    rect.top = GetY();
    rect.right = rect.left + GetW();
    rect.bottom = rect.top + GetH();

    if (Parent_->ClipToArea(&dummy, &rect, cliprect))
    {
        if (GetFlags() bitand C_BIT_USEBGIMAGE)
            BgImage_->Draw(surface, &rect);

        if (Text_)
            Text_->Draw(surface, &rect);
    }
}

BOOL C_Marque::TimerUpdate()
{
    Position_ += Direction_;

    switch (GetType())
    {
        case C_TYPE_VERTICAL:
            Text_->SetY(Position_);

            if (Position_ < -(MarqueLen_))
                Position_ = GetH();

            break;

        case C_TYPE_HORIZONTAL:
            Text_->SetX(Position_);

            if (Position_ < -(MarqueLen_))
                Position_ = GetW();

            break;
    }

    return(TRUE);
}

void C_Marque::SetSubParents(C_Window *)
{
    if (BgImage_)
        BgImage_->SetInfo();

    if (Text_)
    {
        Text_->SetInfo();

        switch (GetType())
        {
            case C_TYPE_VERTICAL:
                MarqueLen_ = Text_->GetH();
                break;

            case C_TYPE_HORIZONTAL:
                MarqueLen_ = Text_->GetW();
                break;
        }
    }

    if ( not GetW() or not GetH())
        SetWH(Parent_->GetW(), Parent_->GetH());
}

#ifdef _UI95_PARSER_
short C_Marque::LocalFind(char *token)
{
    short i = 0;

    while (C_Mrq_Tokens[i])
    {
        if (strnicmp(token, C_Mrq_Tokens[i], strlen(C_Mrq_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_Marque::LocalFunction(short ID, long P[], _TCHAR *, C_Handler *)
{
    switch (ID)
    {
        case CMRQ_SETUP:
            Setup(P[0], (short)P[1], P[2]);
            break;

        case CMRQ_SETFGCOLOR:
            SetFGColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CMRQ_SETBGCOLOR:
            SetBGColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CMRQ_SETFONT:
            SetFont(P[0]);
            break;

        case CMRQ_SETBGIMAGE:
            SetBGImage(P[0]);
            break;

        case CMRQ_SETDIRECTION:
            SetDirection((short)P[0]);
            break;
    }
}

extern char ParseSave[];
extern char ParseCRLF[];

#endif // PARSER
