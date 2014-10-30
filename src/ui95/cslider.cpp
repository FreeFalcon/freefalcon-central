#include <windows.h>
#include "chandler.h"

#ifdef _UI95_PARSER_ // List of Keywords bitand functions to handle them

enum
{
    CSLD_NOTHING = 0,
    CSLD_SETUP,
    CSLD_SETBGIMAGE,
    CSLD_SETSLIDERIMAGE,
    CSLD_SETSTEPS,
};

char *C_Sld_Tokens[] =
{
    "[NOTHING]",
    "[SETUP]",
    "[BGIMAGE]",
    "[SLIDERIMAGE]",
    "[STEPS]",
    0,
};

#endif

C_Slider::C_Slider() : C_Control()
{
    _SetCType_(_CNTL_SLIDER_);
    BGX_ = 0;
    BGY_ = 0;
    SX_ = 0;
    SY_ = 0;
    Steps_ = 0;

    MinPos_ = 0;
    MaxPos_ = 0;

    BgImage_ = NULL; // draw at x,y of control
    Slider_ = NULL;

    DefaultFlags_ = C_BIT_ENABLED bitor C_BIT_REMOVE bitor C_BIT_SELECTABLE bitor C_BIT_MOUSEOVER;
}

C_Slider::C_Slider(char **stream) : C_Control(stream)
{
}

C_Slider::C_Slider(FILE *fp) : C_Control(fp)
{
}

C_Slider::~C_Slider()
{
}

long C_Slider::Size()
{
    return(0);
}

void C_Slider::Setup(long ID, short Type)
{
    SetReady(0);
    SetID(ID);
    SetType(Type);
    SetDefaultFlags();
    SetGroup(0);

    BGX_ = 0;
    BGY_ = 0;
    SX_ = 0;
    SY_ = 0;
    Steps_ = 0;
    BgImage_ = NULL;
    Slider_ = NULL;
    MinPos_ = 0;
    MaxPos_ = 1;

    Callback_ = NULL;
}

void C_Slider::Cleanup()
{
    BgImage_ = NULL;
    Slider_ = NULL;
    SetReady(0);
}

void C_Slider::SetBgImage(long ImageID)
{
    BgImage_ = gImageMgr->GetImage(ImageID);
}

void C_Slider::SetSliderImage(long SliderID)
{
    Slider_ = gImageMgr->GetImage(SliderID);
    SetReady(1);
}

void C_Slider::SetSliderRange(const long Min, const long Max)
{
    MinPos_ = static_cast<short>(Min);
    MaxPos_ = static_cast<short>(Max);
}

void C_Slider::SetSliderPos(long Pos) 
{
    long dist;

    if (Pos < MinPos_)
    {
        Pos = MinPos_;
    }

    if (Steps_ > 1)
    {
        dist = (MaxPos_ - MinPos_) / Steps_;

        if ( not dist)
        {
            dist = 1;
        }

        Pos = ((Pos - MinPos_ + dist / 2) / dist) * dist + MinPos_;
    }

    if (Pos > MaxPos_)
    {
        Pos = MaxPos_;
    }

    if (GetType() == C_TYPE_VERTICAL)
    {
        SY_ = Pos;
    }
    else
    {
        SX_ = Pos;
    }
}

long C_Slider::CheckHotSpots(long relX, long relY)
{
    // check visibility, enabled and ready
    if ((GetFlags() bitand C_BIT_INVISIBLE) or not (GetFlags() bitand C_BIT_ENABLED) or not Ready())
    {
        return(0);
    }


    if (
        /*
         (relX < (GetX()+SX_)) or
         (relX > (GetX()+(Slider_->Header->w)+SX_)) or
         (relY < (GetY()+SY_)) or
         (relY > (GetY()+(Slider_->Header->h)+SY_))*/
        (relX < GetX()) or (relX > (GetX() + GetW())) or
        (relY < GetY()) or (relY > (GetY() + GetH()))
    )
    {
        return(0);
    }

    switch (GetType())
    {
        case C_TYPE_VERTICAL:
            SetRelXY(relX - GetX(), relY - GetY());
            return(GetID());
            break;

        case C_TYPE_HORIZONTAL:
            SetRelXY(relX - GetX(), relY - GetY());
            return(GetID());
            break;
    }

    return(0);
}

BOOL C_Slider::Wheel(int increments, WORD MouseX, WORD MouseY)
{
    F4CSECTIONHANDLE* Leave;

    if (GetFlags() bitand C_BIT_INVISIBLE)
    {
        return(FALSE);
    }

    Leave = UI_Enter(Parent_);
    Refresh();
    // compute a step distance
    long delta;

    if (Steps_ > 0)
    {
        // compute distance and appy increments
        delta = ((MaxPos_ - MinPos_) / Steps_) * increments;
    }
    else
    {
        // just a single increment
        delta = increments;
    }

    if (Type_ == C_TYPE_VERTICAL)
    {
        SY_ += delta;

        // check limits
        if (SY_ < MinPos_)
        {
            SY_ = MinPos_;
        }

        if (SY_ > MaxPos_)
        {
            SY_ = MaxPos_;
        }
    }
    else
    {
        SX_ += delta;

        // check limits
        if (SX_ < MinPos_)
        {
            SX_ = MinPos_;
        }

        if (SX_ > MaxPos_)
        {
            SX_ = MaxPos_;
        }
    }

    if (Callback_)
    {
        // sfr: this is a hack so wheel can be handled by events
        // callbacks should process C_TYPE_MOUSE_WHEEL events though
        //(*Callback_)(GetID(),C_TYPE_MOUSEWHEEL,this);
        (*Callback_)(GetID(), C_TYPE_MOUSEMOVE, this);
    }

    Refresh();
    UI_Leave(Leave);
    return(TRUE);
}

BOOL C_Slider::Process(long ID, short HitType)
{
    gSoundMgr->PlaySound(GetSound(HitType));

    if (Callback_)
        (*Callback_)(ID, HitType, this);

    return(TRUE);
}

void C_Slider::Refresh()
{
    if ( not Ready() or GetFlags() bitand C_BIT_INVISIBLE or Parent_ == NULL)
        return;

    Parent_->SetUpdateRect(GetX() + SX_, GetY() + SY_, GetX() + SX_ + GetW() + 1, GetY() + SY_ + GetH() + 1, GetFlags(), GetClient());
}

void C_Slider::Draw(SCREEN *surface, UI95_RECT *cliprect)
{
    UI95_RECT rect, s;

    if ( not Ready()) return;

    if (GetFlags() bitand C_BIT_INVISIBLE)
        return;

    rect.left = GetX();
    rect.right = rect.left + GetW();
    rect.top = GetY();
    rect.bottom = rect.top + GetH();

    // if(BgImage_)
    // {
    // BgImage_->Blit(0,0,BgImage_->Header->w,BgImage_->Header->h,rect.left,rect.top,800,dest);
    // //Parent_->Blit(BgImage_->Image,&BgImage_->rect,&rect,GetFlags(),GetClient());
    // }
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

        if (GetFlags() bitand C_BIT_ABSOLUTE)
        {
            if ( not Parent_->ClipToArea(&s, &rect, &Parent_->Area_))
                return;
        }
        else
        {
            rect.left += Parent_->VX_[GetClient()];
            rect.top += Parent_->VY_[GetClient()];
            rect.right += Parent_->VX_[GetClient()];
            rect.bottom += Parent_->VY_[GetClient()];

            if ( not Parent_->ClipToArea(&s, &rect, &Parent_->ClientArea_[GetClient()]))
                return;
        }

        if ( not Parent_->ClipToArea(&s, &rect, cliprect))
            return;

        rect.left += Parent_->GetX();
        rect.top += Parent_->GetY();
        rect.right += Parent_->GetX();
        rect.bottom += Parent_->GetY();

        Slider_->Blit(surface, s.left, s.top, s.right - s.left, s.bottom - s.top, rect.left, rect.top);

        if (MouseOver_ or (GetFlags() bitand C_BIT_FORCEMOUSEOVER))
            HighLite(surface, cliprect);
    }
}

void C_Slider::HighLite(SCREEN *surface, UI95_RECT *cliprect)
{
    UI95_RECT clip, tmp;

    clip.left = GetX() + SX_;
    clip.top = GetY() + SY_;

    if (Flags_ bitand C_BIT_RIGHT)
        clip.left -= GetW();
    else if (Flags_ bitand C_BIT_HCENTER)
        clip.left -= GetW() / 2;

    if (Flags_ bitand C_BIT_BOTTOM)
        clip.top -= GetH();
    else if (Flags_ bitand C_BIT_VCENTER)
        clip.top -= GetH() / 2;

    if ( not (Flags_ bitand C_BIT_ABSOLUTE))
    {
        clip.left += Parent_->VX_[Client_];
        clip.top += Parent_->VY_[Client_];
    }

    clip.right = clip.left + Slider_->Header->w;
    clip.bottom = clip.top + Slider_->Header->h;

    if ( not Parent_->ClipToArea(&tmp, &clip, cliprect))
        return;

    if ( not (Flags_ bitand C_BIT_ABSOLUTE))
        if ( not Parent_->ClipToArea(&tmp, &clip, &Parent_->ClientArea_[Client_]))
            return;

    Parent_->BlitTranslucent(surface, MouseOverColor_, MouseOverPercent_, &clip, C_BIT_ABSOLUTE, 0);
}

BOOL C_Slider::MouseOver(long relx, long rely, C_Base *)
{
    // Don't want to do anything here

    if (GetFlags() bitand C_BIT_INVISIBLE or not (GetFlags() bitand C_BIT_ENABLED) or not Ready())
        return(FALSE);

    if (relx >= (GetX() + SX_) and relx < (GetX() + GetW() + SX_) and 
        rely >= (GetY() + SY_) and rely < (GetY() + GetH() + SY_))
    {
        return(TRUE);
    }

    return(FALSE);
}

BOOL C_Slider::Drag(GRABBER *Drag, WORD MouseX, WORD MouseY, C_Window *)
{
    long x, y;
    float dist;
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

            if (Steps_ > 0)
            {
                dist = (float)(MaxPos_ - MinPos_) / (float)Steps_;
                SY_ = static_cast<long>((static_cast<float>(SY_ - MinPos_) + dist / 2.0) / dist); 
                SY_ = static_cast<long>(static_cast<float>(SY_) * dist) + MinPos_;    
            }

            if (SY_ > MaxPos_)
                SY_ = MaxPos_;

            break;

        case C_TYPE_HORIZONTAL:
            SX_ = x;

            if (SX_ < MinPos_)
                SX_ = MinPos_;

            if (Steps_ > 0)
            {
                dist = (float)(MaxPos_ - MinPos_) / (float)Steps_;
                SX_ = static_cast<long>((static_cast<float>(SX_ - MinPos_) + dist / 2.0) / dist); 
                SX_ = static_cast<long>(static_cast<float>(SX_) * dist)  + MinPos_;  
            }

            if (SX_ > MaxPos_)
                SX_ = MaxPos_;

            break;
    }

    Refresh();

    if (Callback_)
        (*Callback_)(GetID(), C_TYPE_MOUSEMOVE, this);

    UI_Leave(Leave);
    return(TRUE);
}

void C_Slider::GetItemXY(long , long *x, long *y)
{
    *x = SX_;
    *y = SY_;
}

void C_Slider::SetSubParents(C_Window *)
{
    if (Slider_)
    {
        switch (GetType())
        {
            case C_TYPE_VERTICAL:
                SetSliderRange(0, GetH() - (Slider_->Header->h));
                break;

            case C_TYPE_HORIZONTAL:
                SetSliderRange(0, GetW() - (Slider_->Header->w));
                break;
        }
    }
}

#ifdef _UI95_PARSER_

short C_Slider::LocalFind(char *token)
{
    short i = 0;

    while (C_Sld_Tokens[i])
    {
        if (strnicmp(token, C_Sld_Tokens[i], strlen(C_Sld_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_Slider::LocalFunction(short ID, long P[], _TCHAR *, C_Handler *)
{
    switch (ID)
    {
        case CSLD_SETUP:
            Setup(P[0], (short)P[1]);
            break;

        case CSLD_SETBGIMAGE:
            SetBgImage(P[0]);
            break;

        case CSLD_SETSLIDERIMAGE:
            SetSliderImage(P[0]);
            break;

        case CSLD_SETSTEPS:
            SetSteps((short)P[0]);
            break;
    }
}

extern char ParseSave[];
extern char ParseCRLF[];

#endif // PARSER
