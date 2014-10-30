#include <windows.h>
#include "chandler.h"

#ifdef _UI95_PARSER_ // List of Keywords bitand functions to handle them

enum
{
    CSCR_NOTHING = 0,
    CSCR_SETUP,
    CSCR_SETBGIMAGE,
    CSCR_SETSLIDERIMAGE,
    CSCR_SETSLIDERRECT,
    CSCR_SETCOLORS,
    CSCR_SETBUTTONIMAGES,
    CSCR_SETDISTANCE,
    CSCR_SETBGLINE,
};

char *C_Scr_Tokens[] =
{
    "[NOTHING]",
    "[SETUP]",
    "[BGIMAGE]",
    "[SLIDERIMAGE]",
    "[SLIDERRECT]",
    "[COLORS]",
    "[BUTTONIMAGES]",
    "[INCREMENT]",
    "[BGLINE]",
    0,
};

#endif

C_ScrollBar::C_ScrollBar() : C_Control()
{
    _SetCType_(_CNTL_SCROLLBAR_);
    BGX_ = 0;
    BGY_ = 0;
    BGW_ = 0;
    BGH_ = 0;
    SX_ = 0;
    SY_ = 0;
    ControlPressed_ = 0;

    MinPos_ = 0;
    Distance_ = 0;
    MaxPos_ = 0;
    VirtualW_ = -1;
    VirtualH_ = -1;

    lite_ = 0;
    medium_ = 0;
    dark_ = 0;

    LineColor_ = 0;

    Plus_ = NULL;
    Minus_ = NULL;
    BgImage_ = NULL;
    Slider_ = NULL;
    SliderRect_.left = 0;
    SliderRect_.top = 0;
    SliderRect_.right = 0;
    SliderRect_.bottom = 0;

    DefaultFlags_ = C_BIT_ENABLED bitor C_BIT_REMOVE bitor C_BIT_SELECTABLE bitor C_BIT_MOUSEOVER;
}

C_ScrollBar::C_ScrollBar(char **stream) : C_Control(stream)
{
}

C_ScrollBar::C_ScrollBar(FILE *fp) : C_Control(fp)
{
}

C_ScrollBar::~C_ScrollBar()
{
}

long C_ScrollBar::Size()
{
    return(0);
}

void C_ScrollBar::Setup(long ID, short Type)
{
    SetReady(0);
    SetID(ID);
    SetType(Type);
    SetDefaultFlags();
    SetGroup(0);

    BGX_ = 0;
    BGY_ = 0;
    BGW_ = 0;
    BGH_ = 0;
    SX_ = 0;
    SY_ = 0;
    BgImage_ = NULL;
    Slider_ = NULL;
    ControlPressed_ = 0;
    Distance_ = 1;
    MinPos_ = 0;
    MaxPos_ = 1;
    Range_ = 1.0f;

    Plus_ = NULL;
    Minus_ = NULL;
}

void C_ScrollBar::Cleanup()
{
    if (BgImage_)
    {
        BgImage_->Cleanup();
        delete BgImage_;
        BgImage_ = NULL;
    }

    if (Plus_)
    {
        Plus_->Cleanup();
        delete Plus_;
        Plus_ = NULL;
    }

    if (Minus_)
    {
        Minus_->Cleanup();
        delete Minus_;
        Minus_ = NULL;
    }

    Slider_ = NULL;
    SetReady(0);
}

void C_ScrollBar::SetBgImage(long ImageID)
{
    if ( not BgImage_)
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

void C_ScrollBar::SetSliderImage(long SliderID)
{
    Slider_ = gImageMgr->GetImage(SliderID);

    if (Slider_)
    {
        if (GetW() < Slider_->Header->w)
            SetW(Slider_->Header->w);

        if (GetH() < Slider_->Header->h)
            SetH(Slider_->Header->h);
    }

    SetReady(1);
}

void C_ScrollBar::SetColors(COLORREF lite, COLORREF medium, COLORREF dark)
{
    lite_ = lite;
    medium_ = medium;
    dark_ = dark;
}

void C_ScrollBar::SetButtonImages(long MinusUp, long MinusDown, long PlusUp, long PlusDown)
{
    IMAGE_RSC *tmp;

    tmp = gImageMgr->GetImage(MinusUp);

    if (tmp == NULL)
    {
        if (Minus_)
        {
            Minus_->Cleanup();
            delete Minus_;
            Minus_ = NULL;
        }
    }
    else if (Minus_ == NULL)
    {
        Minus_ = new C_Button;
        Minus_->Setup(4, C_TYPE_NORMAL, GetX(), GetY());
        Minus_->SetImage(C_STATE_0, MinusUp);
        Minus_->SetImage(C_STATE_1, MinusDown);
        Minus_->SetImage(C_STATE_SELECTED, MinusUp);
        Minus_->SetImage(C_STATE_DISABLED, MinusUp);
        Minus_->SetFlags(GetFlags());

        if (GetType() == C_TYPE_VERTICAL)
            Minus_->SetX(GetX() + GetW() / 2 - Minus_->GetW() / 2);
        else if (GetType() == C_TYPE_HORIZONTAL)
            Minus_->SetY(GetY() + GetH() / 2 - Minus_->GetH() / 2);
    }
    else
    {
        Minus_->SetImage(C_STATE_0, MinusUp);
        Minus_->SetImage(C_STATE_1, MinusDown);
        Minus_->SetImage(C_STATE_SELECTED, MinusUp);
        Minus_->SetImage(C_STATE_DISABLED, MinusUp);
        Minus_->SetFlags(GetFlags());

        if (GetType() == C_TYPE_VERTICAL)
            Minus_->SetX(GetX() + GetW() / 2 - Minus_->GetW() / 2);
        else if (GetType() == C_TYPE_HORIZONTAL)
            Minus_->SetY(GetY() + GetH() / 2 - Minus_->GetH() / 2);
    }

    tmp = gImageMgr->GetImage(PlusUp);

    if (tmp == NULL)
    {
        if (Plus_)
        {
            Plus_->Cleanup();
            delete Plus_;
            Plus_ = NULL;
        }
    }
    else if (Plus_ == NULL)
    {
        Plus_ = new C_Button;
        Plus_->Setup(5, C_TYPE_NORMAL, 0, 0);
        Plus_->SetImage(C_STATE_0, PlusUp);
        Plus_->SetImage(C_STATE_1, PlusDown);
        Plus_->SetImage(C_STATE_SELECTED, PlusUp);
        Plus_->SetImage(C_STATE_DISABLED, PlusUp);
        Plus_->SetFlags(GetFlags());

        if (GetType() == C_TYPE_VERTICAL)
            Plus_->SetXY(GetX() + GetW() / 2 - Plus_->GetW() / 2, GetY() + GetH() - (tmp->Header->h));
        else if (GetType() == C_TYPE_HORIZONTAL)
            Plus_->SetXY(GetX() + GetW() - (tmp->Header->w), GetY() + GetH() / 2 - Plus_->GetH() / 2);
    }
    else
    {
        Plus_->SetImage(C_STATE_0, PlusUp);
        Plus_->SetImage(C_STATE_1, PlusDown);
        Plus_->SetImage(C_STATE_SELECTED, PlusUp);
        Plus_->SetImage(C_STATE_DISABLED, PlusUp);
        Plus_->SetFlags(GetFlags());

        if (GetType() == C_TYPE_VERTICAL)
            Plus_->SetXY(GetX() + GetW() / 2 - Plus_->GetW() / 2, GetY() + GetH() - (tmp->Header->h));
        else if (GetType() == C_TYPE_HORIZONTAL)
            Plus_->SetXY(GetX() + GetW() - (tmp->Header->w), GetY() + GetH() / 2 - Plus_->GetH() / 2);
    }
}

void C_ScrollBar::CalcRanges()
{
    UI95_RECT rect;

    rect.left = 0;
    rect.top = 0;
    rect.right = rect.left + GetW();
    rect.bottom = rect.top + GetH();

    switch (GetType())
    {
        case C_TYPE_VERTICAL:
            if (Minus_)
                rect.top += Minus_->GetH();

            if (Plus_)
                rect.bottom -= Plus_->GetH();

            MinPos_ = rect.top;
            MaxPos_ = rect.bottom;

            if (Slider_)
                MaxPos_ -= Slider_->Header->h;
            else
            {
                SliderRect_.left = 0;
                SliderRect_.right = GetW();
                SliderRect_.top = 0;
                SliderRect_.bottom = GetW();
                MaxPos_ -= GetW();
            }

            if (SY_ < MinPos_)
                SY_ = MinPos_;

            if (SY_ > MaxPos_)
                SY_ = MaxPos_;

            if (GetFlags() bitand C_BIT_USELINE)
            {
                BGX_ = GetW() / 2;
                BGY_ = rect.top;
                BGW_ = 1;
                BGH_ = rect.bottom - rect.top;
            }

            break;

        case C_TYPE_HORIZONTAL:
            if (Minus_)
                rect.left += Minus_->GetH();

            if (Plus_)
                rect.right -= Plus_->GetH();

            MinPos_ = rect.left;
            MaxPos_ = rect.right;

            if (Slider_)
                MaxPos_ -= Slider_->Header->w;
            else
            {
                SliderRect_.left = 0;
                SliderRect_.right = GetH();
                SliderRect_.top = 0;
                SliderRect_.bottom = GetH();
                MaxPos_ -= GetH();
            }

            if (SX_ < MinPos_)
                SX_ = MinPos_;

            if (SX_ > MaxPos_)
                SX_ = MaxPos_;

            if (GetFlags() bitand C_BIT_USELINE)
            {
                BGY_ = rect.left;
                BGX_ = GetH() / 2;
                BGW_ = rect.right - rect.left;
                BGH_ = 1;
            }

            break;
    }

}

void C_ScrollBar::UpdatePosition()
{
    long vl;

    if (GetType() == C_TYPE_VERTICAL)
    {
        vl = (VirtualH_ + Parent_->ClientArea_[GetClient()].bottom - Parent_->ClientArea_[GetClient()].top);

        if ( not vl) vl = 1;

        SY_ = ((Parent_->VY_[GetClient()] - Parent_->ClientArea_[GetClient()].top) * (MaxPos_ - MinPos_) / vl) + MinPos_;

        if (SY_ < MinPos_)
            SY_ = MinPos_;

        if (SY_ > MaxPos_)
            SY_ = MaxPos_;
    }
    else if (GetType() == C_TYPE_HORIZONTAL)
    {
        vl = (VirtualW_ + Parent_->ClientArea_[GetClient()].right - Parent_->ClientArea_[GetClient()].left);

        if ( not vl) vl = 1;

        SX_ = ((Parent_->VX_[GetClient()] - Parent_->ClientArea_[GetClient()].left) * (MaxPos_ - MinPos_) / vl) + MinPos_;

        if (SX_ < MinPos_)
            SX_ = MinPos_;

        if (SX_ > MaxPos_)
            SX_ = MaxPos_;
    }
}

long C_ScrollBar::CheckHotSpots(long relX, long relY)
{
    if (GetFlags() bitand C_BIT_INVISIBLE or not (GetFlags() bitand C_BIT_ENABLED) or not Ready())
        return(0);

    if (relX < GetX() or relX > (GetX() + GetW()) or relY < GetY() or relY > (GetY() + GetH()))
        return(0);

    if (Minus_)
    {
        ControlPressed_ = static_cast<short>(Minus_->CheckHotSpots(relX, relY)); 

        if (ControlPressed_)
        {
            SetRelXY(relX - GetX(), relY - GetY());
            return(GetID());
        }
    }

    if (Plus_)
    {
        ControlPressed_ = static_cast<short>(Plus_->CheckHotSpots(relX, relY)); 

        if (ControlPressed_)
        {
            SetRelXY(relX - GetX(), relY - GetY());
            return(GetID());
        }
    }

    switch (GetType())
    {
        case C_TYPE_VERTICAL:
            if (Slider_)
            {
                if (relY < (GetY() + SY_)) //+MinPos_))
                    ControlPressed_ = 1;
                else if (relY > (GetY() + SY_ + (Slider_->Header->h))) //+MinPos_))
                    ControlPressed_ = 2;
                else
                    ControlPressed_ = 3;
            }
            else
            {
                if (relY < (GetY() + SY_)) //+MinPos_))
                    ControlPressed_ = 1;
                else if (relY > (GetY() + SY_ + (SliderRect_.bottom - SliderRect_.top))) //+MinPos_))
                    ControlPressed_ = 2;
                else
                    ControlPressed_ = 3;
            }

            SetRelXY(relX - GetX(), relY - GetY());
            return(GetID());
            break;

        case C_TYPE_HORIZONTAL:
            if (Slider_)
            {
                if (relX < (GetX() + SX_ + MinPos_))
                    ControlPressed_ = 1;
                else if (relX > (GetX() + SX_ + (Slider_->Header->w) + MinPos_))
                    ControlPressed_ = 2;
                else
                    ControlPressed_ = 3;
            }
            else
            {
                if (relX < (GetX() + SX_ + MinPos_))
                    ControlPressed_ = 1;
                else if (relX > (GetX() + SX_ + (SliderRect_.right - SliderRect_.left) + MinPos_))
                    ControlPressed_ = 2;
                else
                    ControlPressed_ = 3;
            }

            SetRelXY(relX - GetX(), relY - GetY());
            return(GetID());
            break;
    }

    return(0);
}

BOOL C_ScrollBar::Process(long, short HitType)
{
    if ( not Ready()) return(FALSE);

    gSoundMgr->PlaySound(GetSound(HitType));

    switch (GetType())
    {
        case C_TYPE_VERTICAL:
            if (-VirtualH_ < (Parent_->ClientArea_[GetClient()].bottom - Parent_->ClientArea_[GetClient()].top))
                return(FALSE);

            break;

        case C_TYPE_HORIZONTAL:
            if (-VirtualW_ < (Parent_->ClientArea_[GetClient()].right - Parent_->ClientArea_[GetClient()].left))
                return(FALSE);

            break;
    }

    switch (ControlPressed_)
    {
        case 1: // Bar Minus side
            if (HitType not_eq C_TYPE_LMOUSEUP)
                break;

            switch (GetType())
            {
                case C_TYPE_VERTICAL:
                    Parent_->VY_[GetClient()] += (Parent_->ClientArea_[GetClient()].bottom - Parent_->ClientArea_[GetClient()].top);

                    if (Parent_->VY_[GetClient()] > Parent_->ClientArea_[GetClient()].top)
                        Parent_->VY_[GetClient()] = Parent_->ClientArea_[GetClient()].top;

                    UpdatePosition();
                    Refresh();
                    Parent_->RefreshClient(GetClient());
                    break;

                case C_TYPE_HORIZONTAL:
                    Parent_->VX_[GetClient()] += (Parent_->ClientArea_[GetClient()].right - Parent_->ClientArea_[GetClient()].left);

                    if (Parent_->VX_[GetClient()] > Parent_->ClientArea_[GetClient()].left)
                        Parent_->VX_[GetClient()] = Parent_->ClientArea_[GetClient()].left;

                    UpdatePosition();
                    Refresh();
                    Parent_->RefreshClient(GetClient());
                    break;
            }

            break;

        case 2: // Bar Plus Side
            if (HitType not_eq C_TYPE_LMOUSEUP)
                break;

            switch (GetType())
            {
                case C_TYPE_VERTICAL:
                    Parent_->VY_[GetClient()] -= (Parent_->ClientArea_[GetClient()].bottom - Parent_->ClientArea_[GetClient()].top);

                    if (Parent_->VY_[GetClient()] < (VirtualH_ + (Parent_->ClientArea_[GetClient()].bottom - Parent_->ClientArea_[GetClient()].top) + Parent_->ClientArea_[GetClient()].top))
                        Parent_->VY_[GetClient()] = VirtualH_ + (Parent_->ClientArea_[GetClient()].bottom - Parent_->ClientArea_[GetClient()].top) + Parent_->ClientArea_[GetClient()].top;

                    UpdatePosition();
                    Refresh();
                    Parent_->RefreshClient(GetClient());
                    break;

                case C_TYPE_HORIZONTAL:
                    Parent_->VX_[GetClient()] -= (Parent_->ClientArea_[GetClient()].right - Parent_->ClientArea_[GetClient()].left);

                    if (Parent_->VX_[GetClient()] < (VirtualW_ + (Parent_->ClientArea_[GetClient()].right - Parent_->ClientArea_[GetClient()].left) + Parent_->ClientArea_[GetClient()].left))
                        Parent_->VX_[GetClient()] = VirtualW_ + (Parent_->ClientArea_[GetClient()].right - Parent_->ClientArea_[GetClient()].left) + Parent_->ClientArea_[GetClient()].left;

                    UpdatePosition();
                    Refresh();
                    Parent_->RefreshClient(GetClient());
                    break;
            }

            break;

        case 3: // Slider knob
            break;

        case 4: // Minus Button
            Minus_->Process(4, HitType);

            if (HitType not_eq C_TYPE_LMOUSEUP and HitType not_eq C_TYPE_REPEAT)
                break;

            switch (GetType())
            {
                case C_TYPE_VERTICAL:
                    Parent_->VY_[GetClient()] += Distance_;

                    if (Parent_->VY_[GetClient()] > Parent_->ClientArea_[GetClient()].top)
                        Parent_->VY_[GetClient()] = Parent_->ClientArea_[GetClient()].top;

                    UpdatePosition();
                    Refresh();
                    Parent_->RefreshClient(GetClient());
                    break;

                case C_TYPE_HORIZONTAL:
                    Parent_->VX_[GetClient()] += Distance_;

                    if (Parent_->VX_[GetClient()] > Parent_->ClientArea_[GetClient()].left)
                        Parent_->VX_[GetClient()] = Parent_->ClientArea_[GetClient()].left;

                    UpdatePosition();
                    Refresh();
                    Parent_->RefreshClient(GetClient());
                    break;
            }

            break;

        case 5: // Plus Button
            Plus_->Process(5, HitType);

            if (HitType not_eq C_TYPE_LMOUSEUP and HitType not_eq C_TYPE_REPEAT)
                break;

            switch (GetType())
            {
                case C_TYPE_VERTICAL:
                    Parent_->VY_[GetClient()] -= Distance_;

                    if (Parent_->VY_[GetClient()] < (VirtualH_ + (Parent_->ClientArea_[GetClient()].bottom - Parent_->ClientArea_[GetClient()].top) + Parent_->ClientArea_[GetClient()].top))
                        Parent_->VY_[GetClient()] = VirtualH_ + (Parent_->ClientArea_[GetClient()].bottom - Parent_->ClientArea_[GetClient()].top) + Parent_->ClientArea_[GetClient()].top;

                    UpdatePosition();
                    Refresh();
                    Parent_->RefreshClient(GetClient());
                    break;

                case C_TYPE_HORIZONTAL:
                    Parent_->VX_[GetClient()] -= Distance_;

                    if (Parent_->VX_[GetClient()] < (VirtualW_ + (Parent_->ClientArea_[GetClient()].right - Parent_->ClientArea_[GetClient()].left) + Parent_->ClientArea_[GetClient()].left))
                        Parent_->VX_[GetClient()] = VirtualW_ + (Parent_->ClientArea_[GetClient()].right - Parent_->ClientArea_[GetClient()].left) + Parent_->ClientArea_[GetClient()].left;

                    UpdatePosition();
                    Refresh();
                    Parent_->RefreshClient(GetClient());
                    break;
            }

            break;
    }

    return(TRUE);
}

BOOL C_ScrollBar::Dragable(long)
{
    switch (GetType())
    {
        case C_TYPE_VERTICAL:
            if (-VirtualH_ < (Parent_->ClientArea_[GetClient()].bottom - Parent_->ClientArea_[GetClient()].top))
                return(FALSE);

            break;

        case C_TYPE_HORIZONTAL:
            if (-VirtualW_ < (Parent_->ClientArea_[GetClient()].right - Parent_->ClientArea_[GetClient()].left))
                return(FALSE);

            break;
    }

    return(ControlPressed_ == 3);
}

void C_ScrollBar::Refresh()
{
    if ( not Ready() or GetFlags() bitand C_BIT_INVISIBLE or Parent_ == NULL)
        return;

    Parent_->SetUpdateRect(GetX(), GetY(), GetX() + GetW(), GetY() + GetH(), GetFlags(), GetClient());
}

void C_ScrollBar::Draw(SCREEN *surface, UI95_RECT *cliprect)
{
    UI95_RECT s, rect;

    if ( not Ready()) return;

    if (GetFlags() bitand C_BIT_INVISIBLE)
        return;

    if (GetFlags() bitand C_BIT_USEBGIMAGE)
        BgImage_->Draw(surface, cliprect);

    if (GetFlags() bitand C_BIT_USELINE)
    {
        if (GetType() == C_TYPE_HORIZONTAL)
            Parent_->DrawHLine(surface, LineColor_, GetX() + BGX_, GetY() + BGY_, BGW_, C_BIT_ABSOLUTE, 0, cliprect);
        else
            Parent_->DrawVLine(surface, LineColor_, GetX() + BGX_, GetY() + BGY_, BGH_, C_BIT_ABSOLUTE, 0, cliprect);

    }

    if (Minus_)
        Minus_->Draw(surface, cliprect);

    if (Plus_)
        Plus_->Draw(surface, cliprect);

    if (Slider_)
    {
        s.left = 0;
        s.top = 0;
        s.right = Slider_->Header->w;
        s.bottom = Slider_->Header->h;

        rect.left = GetX() + SX_;
        rect.top = GetY() + SY_;
        rect.right = rect.left + Slider_->Header->w;
        rect.bottom = rect.top + Slider_->Header->h;

        if ( not Parent_->ClipToArea(&s, &rect, &Parent_->Area_))
            return;

        if ( not Parent_->ClipToArea(&s, &rect, cliprect))
            return;

        rect.left += Parent_->GetX();
        rect.top += Parent_->GetY();
        rect.right += Parent_->GetX();
        rect.bottom += Parent_->GetY();

        Slider_->Blit(surface, s.left, s.top, s.right - s.left, s.bottom - s.top, rect.left, rect.top);
    }

    if (MouseOver_ or (GetFlags() bitand C_BIT_FORCEMOUSEOVER))
        HighLite(surface, cliprect);
}

BOOL C_ScrollBar::Wheel(int increment, WORD MouseX, WORD MouseY)
{
    // long x,y;
    F4CSECTIONHANDLE* Leave;

    if (GetFlags() bitand C_BIT_INVISIBLE)
    {
        return(FALSE);
    }

    Leave = UI_Enter(Parent_);
    Refresh();

    // get position
    switch (GetType())
    {
        case C_TYPE_VERTICAL:
            SY_ += increment;

            if (SY_ < MinPos_)
            {
                SY_ = MinPos_;
            }

            if (SY_ > MaxPos_)
            {
                SY_ = MaxPos_;
            }

            Parent_->VY_[GetClient()] = (SY_ - MinPos_) * (VirtualH_ + Parent_->ClientArea_[GetClient()].bottom - Parent_->ClientArea_[GetClient()].top) / (MaxPos_ - MinPos_) + Parent_->ClientArea_[GetClient()].top;
            Parent_->RefreshClient(GetClient());
            break;

        case C_TYPE_HORIZONTAL:
            SX_ += increment;

            if (SX_ < MinPos_)
            {
                SX_ = MinPos_;
            }

            if (SX_ > MaxPos_)
            {
                SX_ = MaxPos_;
            }

            Parent_->VX_[GetClient()] = (SX_ - MinPos_) * (VirtualW_ + Parent_->ClientArea_[GetClient()].right - Parent_->ClientArea_[GetClient()].left) / (MaxPos_ - MinPos_) + Parent_->ClientArea_[GetClient()].left;
            Parent_->RefreshClient(GetClient());
            break;
    }

    Refresh();
    UI_Leave(Leave);
    return(TRUE);
}

BOOL C_ScrollBar::Drag(GRABBER *Drag, WORD MouseX, WORD MouseY, C_Window *)
{
    long x, y;
    F4CSECTIONHANDLE* Leave;

    if (GetFlags() bitand C_BIT_INVISIBLE)
        return(FALSE);

    Leave = UI_Enter(Parent_);
    Refresh();
    x = Drag->ItemX_ + (MouseX - Drag->StartX_);
    y = Drag->ItemY_ + (MouseY - Drag->StartY_);

    switch (GetType())
    {
        case C_TYPE_VERTICAL:
            SY_ = y;

            if (SY_ < MinPos_)
                SY_ = MinPos_;

            if (SY_ > MaxPos_)
                SY_ = MaxPos_;

            Parent_->VY_[GetClient()] = (SY_ - MinPos_) * (VirtualH_ + Parent_->ClientArea_[GetClient()].bottom - Parent_->ClientArea_[GetClient()].top) / (MaxPos_ - MinPos_) + Parent_->ClientArea_[GetClient()].top;
            Parent_->RefreshClient(GetClient());
            break;

        case C_TYPE_HORIZONTAL:
            SX_ = x;

            if (SX_ < MinPos_)
                SX_ = MinPos_;

            if (SX_ > MaxPos_)
                SX_ = MaxPos_;

            Parent_->VX_[GetClient()] = (SX_ - MinPos_) * (VirtualW_ + Parent_->ClientArea_[GetClient()].right - Parent_->ClientArea_[GetClient()].left) / (MaxPos_ - MinPos_) + Parent_->ClientArea_[GetClient()].left;
            Parent_->RefreshClient(GetClient());
            break;
    }

    Refresh();
    UI_Leave(Leave);
    return(TRUE);
}


void C_ScrollBar::GetItemXY(long , long *x, long *y)
{
    *x = SX_;
    *y = SY_;
}

void C_ScrollBar::SetSubParents(C_Window *Parent)
{
    if (Minus_)
    {
        Minus_->SetParent(Parent);

        if (GetType() == C_TYPE_VERTICAL)
            Minus_->SetX(GetX() + GetW() / 2 - Minus_->GetW() / 2);
        else if (GetType() == C_TYPE_HORIZONTAL)
            Minus_->SetY(GetY() + GetH() / 2 - Minus_->GetH() / 2);

        Minus_->SetFlags(GetFlags());
        Minus_->SetSubParents(Parent);
    }

    if (Plus_)
    {
        Plus_->SetParent(Parent);

        if (GetType() == C_TYPE_VERTICAL)
        {
            Plus_->SetX(GetX() + GetW() / 2 - Plus_->GetW() / 2);
            Plus_->SetY(GetY() + GetH() - Plus_->GetH());
        }
        else if (GetType() == C_TYPE_HORIZONTAL)
        {
            Plus_->SetX(GetX() + GetW() - Plus_->GetW());
            Plus_->SetY(GetY() + GetH() / 2 - Plus_->GetH() / 2);
        }

        Plus_->SetFlags(GetFlags());
        Plus_->SetSubParents(Parent);
    }

    CalcRanges();
}

#ifdef _UI95_PARSER_
short C_ScrollBar::LocalFind(char *token)
{
    short i = 0;

    while (C_Scr_Tokens[i])
    {
        if (strnicmp(token, C_Scr_Tokens[i], strlen(C_Scr_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_ScrollBar::LocalFunction(short ID, long P[], _TCHAR *, C_Handler *)
{
    switch (ID)
    {
        case CSCR_SETUP:
            Setup(P[0], (short)P[1]);
            break;

        case CSCR_SETBGIMAGE:
            SetBgImage(P[0]);
            break;

        case CSCR_SETSLIDERIMAGE:
            SetSliderImage(P[0]);
            break;

        case CSCR_SETSLIDERRECT:
            SetSliderRect((short)P[0], (short)P[1], (short)P[2], (short)P[3]);
            break;

        case CSCR_SETCOLORS:
            SetColors(P[0] bitor (P[1] << 8) bitor (P[2] << 16), P[3] bitor (P[4] << 8) bitor (P[5] << 16), P[6] bitor (P[7] << 8) bitor (P[8] << 16));
            break;

        case CSCR_SETBUTTONIMAGES:
            SetButtonImages(P[0], P[1], P[2], P[3]);
            break;

        case CSCR_SETDISTANCE:
            if (P[0] >= 1 and P[0] < 50)
                SetDistance(P[0]);

            break;

        case CSCR_SETBGLINE:
            SetLineColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            SetFlagBitOn(C_BIT_USELINE);
            break;
    }
}

extern char ParseSave[];
extern char ParseCRLF[];

#endif // PARSER
