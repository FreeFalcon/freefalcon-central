//
//
// cthook.CPP
//
// Use this to add timer based things into the Event Timer
//
// Created by Peter Ward, April 1997
//
//
//
//
#include <windows.h>
#include "chandler.h"

C_TimerHook::C_TimerHook() : C_Base()
{
    UpdateCallback_ = NULL;
    RefreshCallback_ = NULL;
    DrawCallback_ = NULL;
    _SetCType_(_CNTL_TIMERHOOK_);
    SetReady(0);
    DefaultFlags_ = C_BIT_ENABLED bitor C_BIT_TIMER bitor C_BIT_REMOVE;
}

C_TimerHook::C_TimerHook(char **stream) : C_Base(stream)
{
}

C_TimerHook::C_TimerHook(FILE *fp) : C_Base(fp)
{
}

C_TimerHook::~C_TimerHook()
{
}

long C_TimerHook::Size()
{
    return(0);
}

void C_TimerHook::Setup(long ID, short Type)
{
    SetFlags(DefaultFlags_);
    SetID(ID);
    SetType(Type);
}

void C_TimerHook::Cleanup()
{
}

BOOL C_TimerHook::TimerUpdate()
{
    if (UpdateCallback_)
        (*UpdateCallback_)(GetID(), C_TYPE_TIMER, this);

    if (Ready())
        return(TRUE);

    return(FALSE);
}

void C_TimerHook::Refresh()
{
    if ( not Ready() or (GetFlags() bitand C_BIT_INVISIBLE))
        return;

    Parent_->SetUpdateRect(GetX(), GetY(), GetX() + GetW(), GetY() + GetH(), GetFlags(), GetClient());

    if (RefreshCallback_)
        (*RefreshCallback_)(GetID(), C_TYPE_TIMER, this);
}

// This will ONLY Get Calle
void C_TimerHook::Draw(SCREEN *, UI95_RECT *cliprect)
{
    if ( not Ready() or (GetFlags() bitand C_BIT_INVISIBLE))
        return;

    if (GetFlags() bitand C_BIT_TIMER)
        SetReady(0);

    if (DrawCallback_)
    {
        if ( not (cliprect->left > (GetX() + GetW()) or cliprect->top > (GetY() + GetH()) or cliprect->right < GetX() or cliprect->bottom < GetY()))
            (*DrawCallback_)(GetID(), C_TYPE_TIMER, this);
    }
}

