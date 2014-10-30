#include <windows.h>
#include "chandler.h"

enum
{
    _CUSTOM_MAX_CTRLS_ = 20,
};

C_Custom::C_Custom() : C_Control()
{
    _SetCType_(_CNTL_CUSTOM_);
    DefaultFlags_ = C_BIT_ENABLED bitor C_BIT_MOUSEOVER;

    Section_ = 0;

    Count_ = 0;
    ItemValues_ = NULL;
    Items_ = NULL;
}

C_Custom::C_Custom(char **stream) : C_Control(stream)
{
}

C_Custom::C_Custom(FILE *fp) : C_Control(fp)
{
}

C_Custom::~C_Custom()
{
    if (Count_)
        Cleanup();
}

long C_Custom::Size()
{
    return(0);
}

void C_Custom::Setup(long ID, short Type, short NumCtrls)
{
    short i;

    SetID(ID);
    SetType(Type);
    SetDefaultFlags();

    if (Count_)
        return;

    SetReady(0);

    if (NumCtrls > 0 and NumCtrls < _CUSTOM_MAX_CTRLS_)
    {
        Count_ = NumCtrls;
        Items_ = new O_Output[Count_];
#ifdef USE_SH_POOLS
        ItemValues_ = (long*)MemAllocPtr(UI_Pools[UI_GENERAL_POOL], sizeof(long) * (Count_), FALSE);
#else
        ItemValues_ = new long[Count_];
#endif

        for (i = 0; i < Count_; i++)
        {
            Items_[i].SetOwner(this);
            ItemValues_[i] = 0;
        }

        SetReady(1);
    }
}

void C_Custom::Cleanup(void)
{
    short i;

    if ( not Count_)
        return;

    for (i = 0; i < Count_; i++)
        Items_[i].Cleanup();

    delete Items_;
#ifdef USE_SH_POOLS
    MemFreePtr(ItemValues_);
#else
    delete ItemValues_;
#endif

    Items_ = NULL;
    ItemValues_ = NULL;
    Count_ = 0;
    Section_ = 0;
}

O_Output *C_Custom::GetItem(long idx)
{
    if (idx < Count_)
        return(&Items_[idx]);

    return(NULL);
}

void C_Custom::SetValue(long idx, long value)
{
    if (idx < Count_)
        ItemValues_[idx] = value;
}

long C_Custom::GetValue(long idx)
{
    if (idx < Count_)
        return(ItemValues_[idx]);

    return(0);
}

long C_Custom::CheckHotSpots(long relX, long relY)
{
    short i;

    if (Ready() and not (GetFlags() bitand C_BIT_INVISIBLE) and Parent_)
    {
        if (relX >= GetX() and relY >= GetY() and relX < (GetX() + GetW()) and relY < (GetY() + GetH()))
        {
            Section_ = 0;

            relX -= GetX();
            relY -= GetY();

            for (i = 0; i < Count_; i++)
            {
                if (relX >= Items_[i].GetX() and relX < (Items_[i].GetX() + Items_[i].GetW()) and 
                    relY >= Items_[i].GetY() and relY < (Items_[i].GetY() + Items_[i].GetH()))
                    Section_ = i;
            }

            return(GetID());
        }
    }

    return(0);
}

BOOL C_Custom::Process(long ID, short HitType)
{
    gSoundMgr->PlaySound(GetSound(HitType));

    if (Callback_)
        (*Callback_)(ID, HitType, this);

    return(TRUE);
}

void C_Custom::Refresh()
{
    short i;

    if ((GetFlags() bitand C_BIT_INVISIBLE) or not Parent_ or not Ready())
        return;

    for (i = 0; i < Count_; i++)
        Items_[i].Refresh();
}

void C_Custom::Draw(SCREEN *surface, UI95_RECT *cliprect)
{
    short i;

    if ((GetFlags() bitand C_BIT_INVISIBLE) or not Parent_ or not Ready())
        return;

    for (i = 0; i < Count_; i++)
        Items_[i].Draw(surface, cliprect);

    if (MouseOver_ or (GetFlags() bitand C_BIT_FORCEMOUSEOVER))
        HighLite(surface, cliprect);
}
