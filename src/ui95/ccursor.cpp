#include <windows.h>
#include "chandler.h"

#ifdef _UI95_PARSER_ // List of Keywords bitand functions to handle them

enum
{
    CCUR_NOTHING = 0,
    CCUR_SETUP,
    CCUR_SETRANGES,
    CCUR_SETCOLOR,
    CCUR_SETBOXCOLOR,
    CCUR_SETPERCENTAGE,
};

char *C_Cur_Tokens[] =
{
    "[NOTHING]",
    "[SETUP]",
    "[RANGES]",
    "[COLOR]",
    "[BOXCOLOR]",
    "[PERCENT]",
    0,
};

#endif

C_Cursor::C_Cursor() : C_Control()
{
    _SetCType_(_CNTL_CURSOR_);
    Color_ = 0;
    BoxColor_ = 0;
    MinX_ = 0;
    MinY_ = 0;
    MaxX_ = 0;
    MaxY_ = 0;
    Percent_ = 40;
    DefaultFlags_ = C_BIT_ENABLED bitor C_BIT_REMOVE bitor C_BIT_DRAGABLE;
}

C_Cursor::C_Cursor(char **stream) : C_Control(stream)
{
}

C_Cursor::C_Cursor(FILE *fp) : C_Control(fp)
{
}

C_Cursor::~C_Cursor()
{
}

long C_Cursor::Size()
{
    return(0);
}

void C_Cursor::Setup(long ID, short Type)
{
    SetID(ID);
    SetType(Type);
    SetDefaultFlags();
    SetReady(1);
}

void C_Cursor::Cleanup()
{
}

void C_Cursor::Refresh()
{
    if ( not Ready() or GetFlags() bitand C_BIT_INVISIBLE or Parent_ == NULL)
        return;

    Parent_->SetUpdateRect(MinX_, MinY_, MaxX_ + 2, MaxY_ + 2, GetFlags(), GetClient());
}

void C_Cursor::Draw(SCREEN *surface, UI95_RECT *cliprect)
{
    UI95_RECT s, rect;

    if ( not Ready())
        return;

    if (GetFlags() bitand C_BIT_INVISIBLE or not Parent_)
        return;


    if (Flags_ bitand C_BIT_TRANSLUCENT)
    {
        rect.left = GetX();
        rect.top = GetY();
        rect.right = rect.left + GetW() + 2;
        rect.bottom = rect.top + GetH() + 2;
        s = rect; // just so its set to something JPO

        if ( not Parent_->ClipToArea(&s, &rect, cliprect))
            return;

        Parent_->BlitTranslucent(surface, BoxColor_, Percent_, &rect, Flags_, Client_);
    }
    else
    {
        Parent_->DrawHLine(surface, BoxColor_, GetX(), GetY(), GetW() + 1, GetFlags(), GetClient(), cliprect);
        Parent_->DrawHLine(surface, BoxColor_, GetX(), GetY() + GetH(), GetW() + 1, GetFlags(), GetClient(), cliprect);
        Parent_->DrawVLine(surface, BoxColor_, GetX(), GetY(), GetH() + 1, GetFlags(), GetClient(), cliprect);
        Parent_->DrawVLine(surface, BoxColor_, GetX() + GetW(), GetY(), GetH() + 1, GetFlags(), GetClient(), cliprect);
    }

    Parent_->DrawHLine(surface, Color_, GetX() + GetW() / 2 - 2, GetY() + GetH() / 2, 5, GetFlags(), GetClient(), cliprect);
    Parent_->DrawVLine(surface, Color_, GetX() + GetW() / 2, GetY() + GetH() / 2 - 2, 5, GetFlags(), GetClient(), cliprect);
}

long C_Cursor::CheckHotSpots(long relx, long rely)
{
    if (GetFlags() bitand C_BIT_INVISIBLE or not (GetFlags() bitand C_BIT_ENABLED))
        return(0);

    if (relx >= (GetX()) and rely >= (GetY()) and relx <= (GetX() + GetW()) and rely <= (GetY() + GetH()))
    {
        SetRelXY(relx - GetX(), rely - GetY());
        return(GetID());
    }

    return(0);
};

BOOL C_Cursor::Process(long, short HitType)
{
    gSoundMgr->PlaySound(GetSound(HitType));

    if (Callback_)
        (*Callback_)(GetID(), HitType, this);

    return(TRUE);
}

BOOL C_Cursor::Drag(GRABBER *Drag, WORD MouseX, WORD MouseY, C_Window *)
{
    long x, y;
    F4CSECTIONHANDLE* Leave;

    if (GetFlags() bitand C_BIT_INVISIBLE or not (GetFlags() bitand C_BIT_ENABLED) or not (GetFlags() bitand C_BIT_DRAGABLE))
        return(FALSE);

    Leave = UI_Enter(Parent_);
    x = Drag->ItemX_ + (MouseX - Drag->StartX_);
    y = Drag->ItemY_ + (MouseY - Drag->StartY_);

    if (x < MinX_) x = MinX_;

    if ((x + GetW()) > MaxX_) x = MaxX_ - GetW();

    if (y < MinY_) y = MinY_;

    if ((y + GetH()) > MaxY_) y = MaxY_ - GetH();

    Refresh();
    SetXY(x, y);
    Refresh();
    UI_Leave(Leave);

    if (Callback_)
        (*Callback_)(GetID(), C_TYPE_MOUSEMOVE, this);

    return(TRUE);
}

#ifdef _UI95_PARSER_

short C_Cursor::LocalFind(char *token)
{
    short i = 0;

    while (C_Cur_Tokens[i])
    {
        if (strnicmp(token, C_Cur_Tokens[i], strlen(C_Cur_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_Cursor::LocalFunction(short ID, long P[], _TCHAR *, C_Handler *)
{
    switch (ID)
    {
        case CCUR_SETUP:
            Setup(P[0], (short)P[1]);
            break;

        case CCUR_SETRANGES:
            SetRanges((short)P[0], (short)P[1], (short)P[2], (short)P[3]);
            break;

        case CCUR_SETCOLOR:
            SetColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CCUR_SETBOXCOLOR:
            SetBoxColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CCUR_SETPERCENTAGE:
            SetPercentage((short)P[0]);
            break;
    }
}

extern char ParseSave[];
extern char ParseCRLF[];

#endif // PARSER
