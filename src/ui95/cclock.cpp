#include <windows.h>
#include "chandler.h"


#ifdef _UI95_PARSER_ // List of Keywords bitand functions to handle them

enum
{
    CLK_NOTHING = 0,
    CLK_SETUP,
    CLK_SETNORMCOLOR,
    CLK_SETSELCOLOR,
    CLK_SETCURSORCOLOR,
    CLK_SETSEP0,
    CLK_SETSEP1,
    CLK_SETSEP2,
    CLK_USEDAY,
};

char *C_Clk_Tokens[] =
{
    "[NOTHING]",
    "[SETUP]",
    "[NORMCOLOR]",
    "[SELCOLOR]",
    "[CURSORCOLOR]",
    "[SEP0]",
    "[SEP1]",
    "[SEP2]",
    "[USEDAY]",
    0,
};

#endif

C_Clock::C_Clock() : C_Control()
{
    _SetCType_(_CNTL_CLOCK_);
    Defaultflags_ = C_BIT_ENABLED bitor C_BIT_REMOVE bitor C_BIT_SELECTABLE bitor C_BIT_MOUSEOVER;
    Font_ = 1;
    Day_ = NULL;
    Hour_ = NULL;
    Minute_ = NULL;
    Second_ = NULL;
    CurEdit_ = NULL;
    Sep0_ = NULL;
    Sep1_ = NULL;
    Sep2_ = NULL;
    TimerCallback_ = NULL;
    Section_ = 0;
    NormColor_ = 0xcccccc;
    SelColor_ = 0x00ff00;
    CursorColor_ = 0xffffff;
}

C_Clock::C_Clock(char **stream) : C_Control(stream)
{
}

C_Clock::C_Clock(FILE *fp) : C_Control(fp)
{
}

C_Clock::~C_Clock()
{
}

long C_Clock::Size()
{
    return(0);
}

void C_Clock::Setup(long ID, short Type)
{
    SetID(ID);
    SetType(Type);
    SetDefaultFlags();

    Hour_ = new C_EditBox;
    Hour_->Setup(2, C_TYPE_INTEGER);
    Hour_->SetFlags(Flags_ bitor C_BIT_RIGHT);
    Hour_->SetMaxLen(2);
    Hour_->SetMaxInteger(23);
    Hour_->SetMinInteger(0);
    Hour_->SetInteger(0);

    Minute_ = new C_EditBox;
    Minute_->Setup(3, C_TYPE_INTEGER);
    Minute_->SetFlags(Flags_ bitor C_BIT_LEADINGZEROS bitor C_BIT_RIGHT);
    Minute_->SetMaxLen(2);
    Minute_->SetMaxInteger(59);
    Minute_->SetMinInteger(0);
    Minute_->SetInteger(0);

    Second_ = new C_EditBox;
    Second_->Setup(4, C_TYPE_INTEGER);
    Second_->SetFlags(Flags_ bitor C_BIT_LEADINGZEROS bitor C_BIT_RIGHT);
    Second_->SetMaxLen(2);
    Second_->SetMaxInteger(59);
    Second_->SetMinInteger(0);
    Second_->SetInteger(0);

    Sep1_ = new O_Output;
    Sep1_->SetOwner(this);

    Sep2_ = new O_Output;
    Sep2_->SetOwner(this);

    SetReady(0);
}

void C_Clock::Cleanup()
{
    if (Day_)
    {
        Day_->Cleanup();
        delete Day_;
        Day_ = NULL;
    }

    if (Hour_)
    {
        Hour_->Cleanup();
        delete Hour_;
        Hour_ = NULL;
    }

    if (Minute_)
    {
        Minute_->Cleanup();
        delete Minute_;
        Minute_ = NULL;
    }

    if (Second_)
    {
        Second_->Cleanup();
        delete Second_;
        Second_ = NULL;
    }

    if (Sep0_)
    {
        Sep0_->Cleanup();
        delete Sep0_;
        Sep0_ = NULL;
    }

    if (Sep1_)
    {
        Sep1_->Cleanup();
        delete Sep1_;
        Sep1_ = NULL;
    }

    if (Sep2_)
    {
        Sep2_->Cleanup();
        delete Sep2_;
        Sep2_ = NULL;
    }

    CurEdit_ = NULL;
}

void C_Clock::EnableDay()
{
    Day_ = new C_EditBox;
    Day_->Setup(1, C_TYPE_INTEGER);
    Day_->SetFlags(Flags_ bitor C_BIT_RIGHT);
    Day_->SetMaxLen(2);
    Day_->SetMinInteger(1);
    Day_->SetMaxInteger(48);
    Day_->SetInteger(1);

    Sep0_ = new O_Output;
    Sep0_->SetOwner(this);
}

void C_Clock::SetSep0Str(long TextID)
{
    if (Sep0_)
        Sep0_->SetText(gStringMgr->GetString(TextID));
}

void C_Clock::SetSep1Str(long TextID)
{
    if (Sep1_)
        Sep1_->SetText(gStringMgr->GetString(TextID));
}

void C_Clock::SetSep2Str(long TextID)
{
    if (Sep2_)
        Sep2_->SetText(gStringMgr->GetString(TextID));
}

BOOL C_Clock::TimerUpdate()
{
    if (TimerCallback_)
        return(TimerCallback_(this));

    return(FALSE);
}

void C_Clock::SetXY(long x, long y)
{
    x_ = x;
    y_ = y;
    SetSubParents(Parent_);
}

void C_Clock::SetFont(long ID)
{
    Font_ = ID;
    SetSubParents(Parent_);
}

void C_Clock::SetSubParents(C_Window *Parent)
{
    long  x, w, h;

    if ( not Parent)
        return;

    x = 0;

    w = gFontList->StrWidth(Font_, "00");
    h = gFontList->GetHeight(Font_);

    if (Day_)
    {
        Day_->SetFont(Font_);
        Day_->SetClient(GetClient());
        Day_->SetFlags(Day_->GetFlags() bitor (Flags_ bitor C_BIT_RIGHT));
        Day_->SetXYWH(GetX() + x, GetY(), w, h);
        Day_->SetFgColor(NormColor_);
        Day_->SetCursorColor(CursorColor_);
        Day_->SetParent(Parent);
        Day_->SetSubParents(Parent);
        x += w;
    }

    if (Sep0_)
    {
        Sep0_->SetFont(Font_);
        Sep0_->SetFlags(Flags_);
        Sep0_->SetXY(x, 0);
        Sep0_->SetFgColor(NormColor_);
        Sep0_->SetInfo();
        x += Sep0_->GetW();
    }

    Hour_->SetFont(Font_);
    Hour_->SetClient(GetClient());
    Hour_->SetFlags(Hour_->GetFlags() bitor (Flags_ bitor C_BIT_RIGHT));
    Hour_->SetXYWH(GetX() + x, GetY(), w, h);
    Hour_->SetFgColor(NormColor_);
    Hour_->SetCursorColor(CursorColor_);
    Hour_->SetParent(Parent);
    Hour_->SetSubParents(Parent);
    x += w;

    Sep1_->SetFont(Font_);
    Sep1_->SetFlags(Flags_);
    Sep1_->SetXY(x, 0);
    Sep1_->SetFgColor(NormColor_);
    Sep1_->SetInfo();
    x += Sep1_->GetW();

    Minute_->SetFont(Font_);
    Minute_->SetClient(GetClient());
    Minute_->SetFlags(Minute_->GetFlags() bitor (Flags_ bitor C_BIT_LEADINGZEROS bitor C_BIT_RIGHT));
    Minute_->SetXYWH(GetX() + x, GetY(), w, h);
    Minute_->SetFgColor(NormColor_);
    Minute_->SetCursorColor(CursorColor_);
    Minute_->SetParent(Parent);
    Minute_->SetSubParents(Parent);
    x += w;

    Sep2_->SetFont(Font_);
    Sep2_->SetFlags(Flags_);
    Sep2_->SetXY(x, 0);
    Sep2_->SetFgColor(NormColor_);
    Sep2_->SetInfo();
    x += Sep2_->GetW();

    Second_->SetFont(Font_);
    Second_->SetClient(GetClient());
    Second_->SetFlags(Second_->GetFlags() bitor (Flags_ bitor C_BIT_LEADINGZEROS bitor C_BIT_RIGHT));
    Second_->SetXYWH(GetX() + x, GetY(), w, h);
    Second_->SetFgColor(NormColor_);
    Second_->SetCursorColor(CursorColor_);
    Second_->SetParent(Parent);
    Second_->SetSubParents(Parent);
    x += w;

    CurEdit_ = Hour_;
    CurEdit_->SetFgColor(SelColor_);

    SetWH(x, h);
    SetReady(1);
}

long C_Clock::CheckHotSpots(long relX, long relY)
{
    if (Flags_ bitand C_BIT_INVISIBLE or not (Flags_ bitand C_BIT_ENABLED) or not Ready())
        return(0);

    if (relX >= GetX() and relX <= (GetX() + GetW()) and relY >= GetY() and relY <= (GetY() + GetH()))
    {
        Section_ = 0;

        if (Day_)
            Section_ = Day_->CheckHotSpots(relX, relY);

        if ( not Section_)
            Section_ = Hour_->CheckHotSpots(relX, relY);

        if ( not Section_)
            Section_ = Minute_->CheckHotSpots(relX, relY);

        if ( not Section_)
            Section_ = Second_->GetID();

        return(GetID());
    }

    return(0);
}

void C_Clock::SetTime(long theTime)
{
    long d, h, m, s;

    s = theTime % 60;
    m = (theTime / 60) % 60;
    h = (theTime / 3600) % 24;
    d = theTime / (3600 * 24) + 1;

    if (Day_)
        Day_->SetInteger(d);

    if (Hour_)
        Hour_->SetInteger(h);

    if (Minute_)
        Minute_->SetInteger(m);

    if (Second_)
        Second_->SetInteger(s);

    Refresh();

}

long C_Clock::GetTime()
{
    long theTime;

    theTime = 0;

    if (Day_)
        theTime += (Day_->GetInteger() - 1) * 3600 * 24;

    if (Hour_)
        theTime += Hour_->GetInteger() * 3600;

    if (Minute_)
        theTime += Minute_->GetInteger() * 60;

    if (Second_)
        theTime += Second_->GetInteger();

    return(theTime);
}

void C_Clock::SetDefaultFlags()
{
    SetFlags(Defaultflags_);
}

long C_Clock::GetDefaultFlags()
{
    return(Defaultflags_);
}

BOOL C_Clock::Process(long ID, short HitType)
{
    F4CSECTIONHANDLE *Leave;

    gSoundMgr->PlaySound(GetSound(HitType));

    if (Callback_)
        (*Callback_)(ID, HitType, this);

    if (HitType == C_TYPE_LMOUSEUP)
    {
        if (CurEdit_->GetID() not_eq Section_)
        {
            Leave = UI_Enter(Parent_);

            if (CurEdit_)
            {
                CurEdit_->SetFgColor(NormColor_);
                CurEdit_->Refresh();
            }

            if (Section_ == 1)
                CurEdit_ = Day_;
            else if (Section_ == 2)
                CurEdit_ = Hour_;
            else if (Section_ == 3)
                CurEdit_ = Minute_;
            else
                CurEdit_ = Second_;

            CurEdit_->SetFgColor(SelColor_);
            CurEdit_->Refresh();
            UI_Leave(Leave);
        }

        return(TRUE);
    }

    return(FALSE);
}

void C_Clock::Refresh()
{
    if ( not Ready() or Flags_ bitand C_BIT_INVISIBLE or Parent_ == NULL)
        return;

    Parent_->SetUpdateRect(GetX(), GetY(), GetX() + GetW(), GetY() + GetH(), Flags_, GetClient());
}

void C_Clock::Draw(SCREEN *surface, UI95_RECT *cliprect)
{
    if ( not Ready() or Flags_ bitand C_BIT_INVISIBLE or Parent_ == NULL)
        return;

    if (Sep0_)
        Sep0_->Draw(surface, cliprect);

    if (Sep1_)
        Sep1_->Draw(surface, cliprect);

    if (Sep2_)
        Sep2_->Draw(surface, cliprect);

    if (Day_)
        Day_->Draw(surface, cliprect);

    if (Hour_)
        Hour_->Draw(surface, cliprect);

    if (Minute_)
        Minute_->Draw(surface, cliprect);

    if (Second_)
        Second_->Draw(surface, cliprect);

    if (MouseOver_ or (GetFlags() bitand C_BIT_FORCEMOUSEOVER))
        HighLite(surface, cliprect);
}


#ifdef _UI95_PARSER_
short C_Clock::LocalFind(char *token)
{
    short i = 0;

    while (C_Clk_Tokens[i])
    {
        if (strnicmp(token, C_Clk_Tokens[i], strlen(C_Clk_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_Clock::LocalFunction(short ID, long P[], _TCHAR *, C_Handler *)
{
    switch (ID)
    {
        case CLK_SETUP:
            Setup(P[0], (short)P[1]);
            break;

        case CLK_SETNORMCOLOR:
            SetNormColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CLK_SETSELCOLOR:
            SetSelColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CLK_SETCURSORCOLOR:
            SetCursorColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CLK_SETSEP0:
            SetSep0Str(P[0]);
            break;

        case CLK_SETSEP1:
            SetSep1Str(P[0]);
            break;

        case CLK_SETSEP2:
            SetSep2Str(P[0]);
            break;

        case CLK_USEDAY:
            EnableDay();
            break;
    }
}

#endif // PARSER
