#include <windows.h>
#include "chandler.h"

#ifdef _UI95_PARSER_ // List of Keywords bitand functions to handle them

enum
{
    CPAN_NOTHING = 0,
    CPAN_SETUP,
    CPAN_SETBGIMAGE,
    CPAN_SETIMAGE,
    CPAN_SETDEADZONE,
};

char *C_Pan_Tokens[] =
{
    "[NOTHING]",
    "[SETUP]",
    "[BGIMAGE]",
    "[PANIMAGE]",
    "[DEADZONE]",
    0,
};

#endif

C_Panner::C_Panner() : C_Control()
{
    short i;

    _SetCType_(_CNTL_PANNER_);
    SX_ = 0;
    SY_ = 0;
    state_ = 0;
    DeadX_ = 0;
    DeadY_ = 0;
    DeadW_ = 0;
    DeadH_ = 0;

    BgImage_ = NULL; // draw at x,y of control

    for (i = 0; i < PAN_MAX_IMAGES; i++)
        Image_[i] = NULL;

    DefaultFlags_ = C_BIT_ENABLED bitor C_BIT_REMOVE bitor C_BIT_DRAGABLE bitor C_BIT_MOUSEOVER;
}

C_Panner::C_Panner(char **stream) : C_Control(stream)
{
}

C_Panner::C_Panner(FILE *fp) : C_Control(fp)
{
}

C_Panner::~C_Panner()
{
    Cleanup();
}

long C_Panner::Size()
{
    return(0);
}

void C_Panner::Setup(long ID, short Type)
{
    SetID(ID);
    SetType(Type);
    SetDefaultFlags();
}

void C_Panner::Cleanup()
{
    short i;

    if (BgImage_)
    {
        BgImage_->Cleanup();
        delete BgImage_;
        BgImage_ = NULL;
    }

    for (i = 0; i < PAN_MAX_IMAGES; i++)
    {
        if (Image_[i])
        {
            Image_[i]->Cleanup();
            delete Image_[i];
            Image_[i] = NULL;
        }
    }

    SetReady(0);
}

void C_Panner::SetBgImage(long ImageID)
{
    if (BgImage_ == NULL)
    {
        BgImage_ = new O_Output;
        BgImage_->SetOwner(this);
    }

    BgImage_->SetImage(ImageID);
}

void C_Panner::SetImage(short state, long ImageID)
{
    IMAGE_RSC *tmp;

    if (state < PAN_MAX_IMAGES)
    {
        if (Image_[state] == NULL)
        {
            Image_[state] = new O_Output;
            Image_[state]->SetOwner(this);
        }

        tmp = gImageMgr->GetImage(ImageID);

        if (tmp)
        {
            Image_[state]->SetImage(tmp);
            //Image_[state]->SetXY(tmp->x,tmp->y);
        }

        if ( not state and Image_[state])
            SetWH(Image_[state]->GetW(), Image_[state]->GetH());

        if (Image_[0]->Ready())
            SetReady(1);
    }
}

long C_Panner::CheckHotSpots(long relX, long relY)
{
    if (GetFlags() bitand C_BIT_INVISIBLE or not (GetFlags() bitand C_BIT_ENABLED) or not Ready() or not Parent_)
    {
        return(0);
    }

    // sfr: fixes CTD with the panner with only one image
    // try to get image from the current state (can happen with both mouse buttons being pressed)
    O_Output *image;

    // try original image instead (fallback)
    if (Image_[state_] == NULL)
    {
        image = Image_[0];
    }
    else
    {
        image = Image_[state_];
    }

    // even then... return
    if (image == NULL)
    {
        return 0;
    }

    // check image hotspot
    if (
        relX >= (GetX() - image->GetX()) and 
        relX <= (GetX() + GetW() - image->GetX()) and 
        relY >= (GetY() - image->GetY()) and 
        relY <= (GetY() + GetH() - image->GetY())
    )
    {
        SetRelXY(relX - GetX(), relY - GetY());
        return(GetID());
    }

    return(0);
}

BOOL C_Panner::Process(long, short HitType)
{
    F4CSECTIONHANDLE *Leave;
    int w, h;

    Leave = UI_Enter(Parent_);

    switch (HitType)
    {
        case C_TYPE_LMOUSEDOWN:
        case C_TYPE_REPEAT:
            w = (Image_[0]->GetW());
            h = (Image_[0]->GetH());

            SX_ = (GetRelX() - w / 2);
            SY_ = (GetRelY() - h / 2);

            if (SX_ < 0)
            {
                if (SX_ < (DeadW_ / 2))
                    SX_ += DeadW_ / 2;
                else
                    SX_ = 0;
            }

            if (SY_ < 0)
            {
                if (SY_ < (DeadH_ / 2))
                    SY_ += DeadH_ / 2;
                else
                    SY_ = 0;
            }

            if (SX_ > 0)
            {
                if (SX_ > (DeadW_ / 2))
                    SX_ -= DeadW_ / 2;
                else
                    SX_ = 0;
            }

            if (SY_ > 0)
            {
                if (SY_ > (DeadH_ / 2))
                    SY_ -= DeadH_ / 2;
                else
                    SY_ = 0;
            }

            SX_ /= 4;
            SY_ /= 4;

            switch (GetType())
            {
                case C_TYPE_DRAGX:
                    state_ = 1;

                    if (SX_ < 0)
                        state_ = 2;
                    else if (SX_ > 0)
                        state_ = 3;

                    break;

                case C_TYPE_DRAGY:
                    state_ = 1;

                    if (SY_ < 0)
                        state_ = 2;
                    else if (SY_ > 0)
                        state_ = 3;

                    break;

                case C_TYPE_DRAGXY:
                    if (SX_ < 0 and SY_ < 0)
                        state_ = 2;
                    else if (SX_ == 0 and SY_ < 0)
                        state_ = 3;
                    else if (SX_ > 0 and SY_ < 0)
                        state_ = 4;
                    else if (SX_ < 0 and SY_ == 0)
                        state_ = 5;
                    else if (SX_ > 0 and SY_ == 0)
                        state_ = 6;
                    else if (SX_ < 0 and SY_ > 0)
                        state_ = 7;
                    else if (SX_ == 0 and SY_ > 0)
                        state_ = 8;
                    else if (SX_ > 0 and SY_ > 0)
                        state_ = 9;
                    else
                        state_ = 1;

                    break;
            }

            Refresh();
            break;

        case C_TYPE_LDROP:
        case C_TYPE_LMOUSEDBLCLK:
        case C_TYPE_RMOUSEDOWN:
        case C_TYPE_RMOUSEUP:
        case C_TYPE_LMOUSEUP:
        case C_TYPE_RMOUSEDBLCLK:
            state_ = 0;
            SX_ = 0;
            SY_ = 0;
            Refresh();
            break;
    }

    UI_Leave(Leave);
    gSoundMgr->PlaySound(GetSound(HitType));

    if (Callback_)
        (*Callback_)(GetID(), HitType, this);

    return(TRUE);
}


void C_Panner::Refresh()
{
    if ( not Ready() or GetFlags() bitand C_BIT_INVISIBLE or Parent_ == NULL)
        return;

    if (BgImage_)
        BgImage_->Refresh();

    if (Image_[state_])
        Image_[state_]->Refresh();
}

void C_Panner::Draw(SCREEN *surface, UI95_RECT *cliprect)
{
    int i;

    if ( not Ready()) return;

    if (GetFlags() bitand C_BIT_INVISIBLE)
        return;

    if (BgImage_)
        BgImage_->Draw(surface, cliprect);

    i = state_;

    if (Image_[i] == NULL) i = 0;

    if (Image_[i])
        Image_[i]->Draw(surface, cliprect);

    if (MouseOver_ or (GetFlags() bitand C_BIT_FORCEMOUSEOVER))
        HighLite(surface, cliprect);
}

BOOL C_Panner::Drag(GRABBER *, WORD MouseX, WORD MouseY, C_Window *)
{
    int w, h;

    return(FALSE);

    F4CSECTIONHANDLE* Leave;

    if (GetFlags() bitand C_BIT_INVISIBLE)
        return(FALSE);

    Leave = UI_Enter(Parent_);
    w = (Image_[0]->GetW());
    h = (Image_[0]->GetH());

    SX_ = (short)(MouseX - Parent_->GetX() - GetX() - DeadX_); 
    SY_ = (short)(MouseY - Parent_->GetY() - GetY() - DeadY_); 

    if (SX_ > -DeadW_ and SX_ < DeadW_) SX_ = 0;

    if (SY_ > -DeadH_ and SY_ < DeadH_) SY_ = 0;

    switch (GetType())
    {
        case C_TYPE_DRAGX:
            state_ = 1;

            if (SX_ < 0)
                state_ = 2;
            else if (SX_ > 0)
                state_ = 3;

            Refresh();
            break;

        case C_TYPE_DRAGY:
            state_ = 1;

            if (SY_ < 0)
                state_ = 2;
            else if (SY_ > 0)
                state_ = 3;

            Refresh();
            break;

        case C_TYPE_DRAGXY:
            state_ = 1;

            if (SX_ < 0 and SY_ < 0)
                state_ = 2;
            else if (SX_ == 0 and SY_ < 0)
                state_ = 3;
            else if (SX_ > 0 and SY_ < 0)
                state_ = 4;
            else if (SX_ < 0 and SY_ == 0)
                state_ = 5;
            else if (SX_ > 0 and SY_ == 0)
                state_ = 6;
            else if (SX_ < 0 and SY_ > 0)
                state_ = 7;
            else if (SX_ == 0 and SY_ > 0)
                state_ = 8;
            else if (SX_ > 0 and SY_ > 0)
                state_ = 9;

            Refresh();
            break;
    }

    if (Callback_)
        (*Callback_)(GetID(), C_TYPE_MOUSEMOVE, this);

    UI_Leave(Leave);
    return(TRUE);
}

BOOL C_Panner::Drop(GRABBER *, WORD , WORD , C_Window *)
{
    state_ = 0;
    Refresh();
    return(FALSE);
}

void C_Panner::GetItemXY(long , long *x, long *y)
{
    *x = 0;
    *y = 0;
}

void C_Panner::SetSubParents(C_Window *)
{
    short i;

    if (BgImage_)
        BgImage_->SetFlags(GetFlags());

    for (i = 0; i < PAN_MAX_IMAGES; i++)
        if (Image_[i])
            Image_[i]->SetFlags(GetFlags());
}

#ifdef _UI95_PARSER_
short C_Panner::LocalFind(char *token)
{
    short i = 0;

    while (C_Pan_Tokens[i])
    {
        if (strnicmp(token, C_Pan_Tokens[i], strlen(C_Pan_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_Panner::LocalFunction(short ID, long P[], _TCHAR *, C_Handler *)
{
    switch (ID)
    {
        case CPAN_SETUP:
            Setup(P[0], (short)P[1]);
            break;

        case CPAN_SETBGIMAGE:
            SetBgImage(P[0]);
            break;

        case CPAN_SETIMAGE:
            SetImage((short)P[0], P[1]);
            break;

        case CPAN_SETDEADZONE:
            SetDeadZone((short)P[0], (short)P[1], (short)P[2], (short)P[3]);
            break;
    }
}

extern char ParseSave[];
extern char ParseCRLF[];

#endif // PARSER
