#include "stdafx.h"

C_Player::C_Player() : C_Control()
{
    _SetCType_(_CNTL_ENTITY_);
    State_ = 0;
    Icon_ = NULL;
    Name_ = NULL;
    Status_ = NULL;
    muted_ = 0;
    ignored_ = 0;
    Owner_ = NULL;
    Color_[0] = 0;
    Color_[1] = 0;
    Color_[2] = 0;
    Defaultflags_ = C_BIT_ENABLED bitor C_BIT_REMOVE bitor C_BIT_MOUSEOVER bitor C_BIT_USEBGFILL;
}

C_Player::C_Player(char **stream) : C_Control(stream)
{
}

C_Player::C_Player(FILE *fp) : C_Control(fp)
{
}

C_Player::~C_Player()
{
}

long C_Player::Size()
{
    return(0);
}

void C_Player::Setup(long ID, short Type)
{
    SetID(ID);
    SetType(Type);
    SetDefaultFlags();
}

void C_Player::InitEntity()
{
    Icon_ = new O_Output;
    Icon_->SetOwner(this);
    Icon_->SetFlags(Flags_ bitor C_BIT_HCENTER bitor C_BIT_VCENTER);
    Name_ = new O_Output;
    Name_->SetOwner(this);
    Name_->SetFlags(Flags_);
    Name_->SetTextWidth(20);
    Status_ = new O_Output;
    Status_->SetOwner(this);
    Status_->SetFlags(Flags_ bitor C_BIT_HCENTER bitor C_BIT_VCENTER);

    SetReady(1);
}

void C_Player::Cleanup()
{
    if (Icon_)
    {
        Icon_->Cleanup();
        delete Icon_;
        Icon_ = NULL;
    }

    if (Name_)
    {
        Name_->Cleanup();
        delete Name_;
        Name_ = NULL;
    }

    if (Status_)
    {
        Status_->Cleanup();
        delete Status_;
        Status_ = NULL;
    }
}

void C_Player::SetFont(long ID)
{
    if (Name_)
    {
        Name_->SetFont(ID);
        Name_->SetInfo();
    }
}

long C_Player::CheckHotSpots(long relX, long relY)
{
    if (relX >= GetX() and relX <= (GetX() + GetW()) and relY >= GetY() and relY <= (GetY() + GetH()))
        return(GetID());

    return(0);
}

void C_Player::SetDefaultFlags()
{
    SetFlags(Defaultflags_);
}

long C_Player::GetDefaultFlags()
{
    return(Defaultflags_);
}

BOOL C_Player::Process(long ID, short HitType)
{
    gSoundMgr->PlaySound(GetSound(HitType));

    if (Callback_)
        (*Callback_)(ID, HitType, this);

    return(FALSE);
}

void C_Player::Refresh()
{
    if ( not Ready() or Flags_ bitand C_BIT_INVISIBLE or Parent_ == NULL)
        return;

    Parent_->SetUpdateRect(GetX(), GetY(), GetX() + GetW(), GetY() + GetH(), Flags_, GetClient());
}

void C_Player::Draw(SCREEN *surface, UI95_RECT *cliprect)
{
    if ( not Ready() or GetFlags() bitand C_BIT_INVISIBLE or Parent_ == NULL)
        return;

    if (Icon_)
        Icon_->Draw(surface, cliprect);

    if (Name_)
        Name_->Draw(surface, cliprect);

    if (Status_ and muted_)
        Status_->Draw(surface, cliprect);

    if (MouseOver_ or (GetFlags() bitand C_BIT_FORCEMOUSEOVER))
        HighLite(surface, cliprect);
}

void C_Player::SetState(short newstate)
{
    State_ = static_cast<short>(newstate bitand 1);

    if (Name_)
    {
        if (ignored_)
            Name_->SetFgColor(Color_[2]);
        else
            Name_->SetFgColor(Color_[State_]);

        Name_->Refresh();
    }
}

void C_Player::SetMute(short mute)
{
    muted_ = static_cast<short>(mute bitand 1);

    if (Status_)
        Status_->Refresh();
}

void C_Player::SetIgnore(short ignore)
{
    ignored_ = static_cast<short>(ignore bitand 1);

    if (Name_)
    {
        if (ignored_)
            Name_->SetFgColor(Color_[2]);
        else
            Name_->SetFgColor(Color_[State_]);

        Name_->Refresh();
    }
}

void C_Player::SetSubParents(C_Window *)
{
    long w, h;

    w = 0;
    h = 0;

    if (Icon_)
    {
        if ((Icon_->GetX() + Icon_->GetW()) > w)
            w = Icon_->GetX() + Icon_->GetW();

        if ((Icon_->GetY() + Icon_->GetH()) > w)
            h = Icon_->GetY() + Icon_->GetH();
    }

    if (Name_)
    {
        w = Name_->GetX() + Name_->GetW();
        h = Name_->GetY() + Name_->GetH();
    }

    if (Status_)
    {
        if ((Status_->GetX() + Status_->GetW()) > w)
            w = Status_->GetX() + Status_->GetW();

        if ((Status_->GetY() + Status_->GetH()) > w)
            h = Status_->GetY() + Status_->GetH();
    }

    SetWH(w, h);
}

