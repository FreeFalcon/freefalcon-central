#include <windows.h>
#include "chandler.h"

#ifdef _UI95_PARSER_ // List of Keywords bitand functions to handle them

enum
{
    CANM_NOTHING = 0,
    CANM_SETUP,
    CANM_DIRECTION,
};

char *C_Anm_Tokens[] =
{
    "[NOTHING]",
    "[SETUP]",
    "[DIRECTION]",
    0,
};

#endif

C_Anim::C_Anim() : C_Base()
{
    Anim_ = NULL;
    _SetCType_(_CNTL_ANIMATION_);
    SetReady(0);
    DefaultFlags_ = C_BIT_ENABLED bitor C_BIT_REMOVE bitor C_BIT_TIMER;
}

C_Anim::C_Anim(char **stream) : C_Base(stream)
{
}

C_Anim::C_Anim(FILE *fp) : C_Base(fp)
{
}

C_Anim::~C_Anim()
{
}

long C_Anim::Size()
{
    return(0);
}
void C_Anim::Setup(long ID, short Type, long AnimID)
{
    SetID(ID);
    SetType(Type);
    SetDefaultFlags();
    SetAnim(AnimID);
}

void C_Anim::Cleanup()
{
    if (Anim_)
    {
        Anim_->Cleanup();
        delete Anim_;
        Anim_ = NULL;
    }
}

void C_Anim::SetAnim(long ID)
{
    SetReady(0);

    if (Anim_ == NULL)
    {
        Anim_ = new O_Output;
        Anim_->SetOwner(this);
    }

    Anim_->SetFlags(GetFlags());
    Anim_->SetAnim(ID);

    if (Anim_->Ready())
    {
        SetWH(Anim_->GetW(), Anim_->GetH());
        SetReady(1);
    }
}

void C_Anim::SetDirection(short dir)
{
    if (Anim_)
        Anim_->SetDirection(dir);
}

void C_Anim::SetFlags(long flags)
{
    SetControlFlags(flags);

    if (Anim_)
    {
        Anim_->SetFlags(flags);
        Anim_->SetInfo();
    }
}

void C_Anim::Refresh()
{
    if (GetFlags() bitand C_BIT_INVISIBLE or Parent_ == NULL or not Ready())
        return;

    if (Anim_)
        Anim_->Refresh();
}

void C_Anim::Draw(SCREEN *surface, UI95_RECT *cliprect)
{
    if (GetFlags() bitand C_BIT_INVISIBLE or Parent_ == NULL or not Ready())
        return;

    if (Anim_)
        Anim_->Draw(surface, cliprect);
}

BOOL C_Anim::TimerUpdate()
{
    if ( not (GetFlags() bitand C_BIT_ENABLED))
        return(FALSE);

    if ( not Ready()) return(FALSE);

    switch (GetType())
    {
        case C_TYPE_LOOP:
            Anim_->SetFrame(Anim_->GetFrame() + Anim_->GetDirection());

            if (Anim_->GetFrame() < 0)
                Anim_->SetFrame(Anim_->GetAnim()->Anim->Frames - 1);

            if (Anim_->GetFrame() >= Anim_->GetAnim()->Anim->Frames)
                Anim_->SetFrame(0);

            return(TRUE);
            break;

        case C_TYPE_STOPATEND:
            Anim_->SetFrame(Anim_->GetFrame() + Anim_->GetDirection());

            if (Anim_->GetFrame() < 0)
            {
                Anim_->SetFrame(0);
                return(FALSE);
            }

            if (Anim_->GetFrame() >= Anim_->GetAnim()->Anim->Frames)
            {
                Anim_->SetFrame(Anim_->GetAnim()->Anim->Frames - 1);
                return(FALSE);
            }

            return(TRUE);
            break;

        case C_TYPE_PINGPONG:
            Anim_->SetFrame(Anim_->GetFrame() + Anim_->GetDirection());

            if ((Anim_->GetFrame() < 0) or (Anim_->GetFrame() >= Anim_->GetAnim()->Anim->Frames))
            {
                Anim_->SetFrame(Anim_->GetFrame() - Anim_->GetDirection());
                Anim_->SetDirection(-Anim_->GetDirection());
            }

            return(TRUE);
            break;
    }

    return(FALSE);
}

#ifdef _UI95_PARSER_
short C_Anim::LocalFind(char *token)
{
    short i = 0;

    while (C_Anm_Tokens[i])
    {
        if (strnicmp(token, C_Anm_Tokens[i], strlen(C_Anm_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_Anim::LocalFunction(short ID, long P[], _TCHAR *, C_Handler *)
// not void C_Anim::LocalFunction(short ID,long P[],_TCHAR *str,C_Handler *Hndlr)
{

    switch (ID)
    {
        case CANM_SETUP:
            Setup(P[0], (short)P[1], P[2]);
            break;

        case CANM_DIRECTION:
            SetDirection((short)P[0]);
            break;
    }
}

extern char ParseSave[];
extern char ParseCRLF[];


#endif // PARSER

