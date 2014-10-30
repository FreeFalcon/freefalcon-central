#include <windows.h>
#include "chandler.h"

C_Help::C_Help() : C_Control()
{
    _SetCType_(_CNTL_HELP_);

    Picture_ = NULL;
    Text_ = NULL;
    FgColor_ = 0xffffff;
    Font_ = 1;

    DefaultFlags_ = C_BIT_ENABLED bitor C_BIT_REMOVE bitor C_BIT_MOUSEOVER;
}

C_Help::C_Help(char **stream) : C_Control(stream)
{
}

C_Help::C_Help(FILE *fp) : C_Control(fp)
{
}

C_Help::~C_Help()
{
    Cleanup();
}

long C_Help::Size()
{
    return(0);
}

void C_Help::Setup(long ID, short Type)
{
    SetID(ID);
    SetType(Type);
    SetDefaultFlags();
    SetReady(1);

    if ( not Picture_)
    {
        Picture_ = new O_Output;
        Picture_->SetOwner(this);
    }

    if ( not Text_)
    {
        Text_ = new O_Output;
        Text_->SetOwner(this);
    }
}

void C_Help::Cleanup(void)
{
    if (Picture_)
    {
        Picture_->Cleanup();
        delete Picture_;
        Picture_ = NULL;
    }

    if (Text_)
    {
        Text_->Cleanup();
        delete Text_;
        Text_ = NULL;
    }
}

void C_Help::SetImage(long x, long y, long ImageID)
{
    Picture_->SetImage(ImageID);
    Picture_->SetXY(x, y);
    Picture_->SetInfo();
}

void C_Help::SetText(long x, long y, long w, long TextID)
{
    Text_->SetText(gStringMgr->GetString(TextID));
    Text_->SetXY(x, y);
    Text_->SetFgColor(FgColor_);
    Text_->SetWordWrapWidth(w);
    Text_->SetInfo();
}

long C_Help::CheckHotSpots(long , long)
{
    if (GetFlags() bitand C_BIT_INVISIBLE or not (GetFlags() bitand C_BIT_ENABLED) or not Ready())
        return(0);

    return(0);
}

BOOL C_Help::Process(long ID, short HitType)
{
    gSoundMgr->PlaySound(GetSound(HitType));

    if (Callback_)
        (*Callback_)(ID, HitType, this);

    return(FALSE);
}

void C_Help::Refresh()
{
    if (GetFlags() bitand C_BIT_INVISIBLE or Parent_ == NULL)
        return;

    Parent_->SetUpdateRect(GetX(), GetY(), GetX() + GetW(), GetY() + GetH(), GetFlags(), GetClient());
}

void C_Help::Draw(SCREEN *surface, UI95_RECT *cliprect)
{
    if (GetFlags() bitand C_BIT_INVISIBLE or Parent_ == NULL)
        return;

    if (Picture_)
        Picture_->Draw(surface, cliprect);

    if (Text_)
        Text_->Draw(surface, cliprect);
}

void C_Help::SetSubParents(C_Window *parent)
{
    int w = 0, h = 0;

    if ( not parent)
        return;

    if (Picture_)
    {
        Picture_->SetFlags(GetFlags());
        Picture_->SetInfo();
        w = Picture_->GetX() + Picture_->GetW();
        h = Picture_->GetY() + Picture_->GetH();
    }

    if (Text_)
    {
        Text_->SetFlags(GetFlags());
        Text_->SetFgColor(FgColor_);
        Text_->SetFont(Font_);
        Text_->SetInfo();

        if ((Text_->GetX() + Text_->GetW()) > w)
            w = Text_->GetX() + Text_->GetW();

        if ((Text_->GetY() + Text_->GetH()) > h)
            h = Text_->GetY() + Text_->GetH();
    }

    SetWH(w, h + 2);
}
