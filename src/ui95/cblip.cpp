#include <windows.h>
#include "chandler.h"

// Times are in Minutes
#define CUTOFF_TIME 60
#define STATE_7_TIME 45
#define STATE_6_TIME 30
#define STATE_5_TIME 15

#ifdef _UI95_PARSER_ // List of Keywords bitand functions to handle them

enum
{
    CBOX_NOTHING = 0,
    CBOX_SETUP,
    CBOX_SETCOLOR,
};

char *C_Blip_Tokens[] =
{
    "[NOTHING]",
    "[SETUP]",
    "[COLOR]",
    0,
};

#endif

C_Blip::C_Blip() : C_Base()
{
    Root_ = NULL;
    Drawer_ = NULL;
    Last_ = NULL;
    memset(BlipImg_, NULL, sizeof(IMAGE_RSC*) * 8 * 8);
    _SetCType_(_CNTL_BLIP_);
    DefaultFlags_ = C_BIT_ENABLED bitor C_BIT_REMOVE;
}

C_Blip::C_Blip(char **stream) : C_Base(stream)
{
}

C_Blip::C_Blip(FILE *fp) : C_Base(fp)
{
}

C_Blip::~C_Blip()
{
    if (Root_ or Drawer_)
        Cleanup();
}

long C_Blip::Size()
{
    return(0);
}

void C_Blip::Setup(long ID, short Type)
{
    SetID(ID);
    SetType(Type);
    SetDefaultFlags();
    SetReady(1);
}

void C_Blip::Cleanup(void)
{
    BLIP *cur, *last;

    cur = Root_;

    while (cur)
    {
        last = cur;
        cur = cur->Next;
        delete last;
    }

    Root_ = NULL;
    Last_ = NULL;

    if (Drawer_)
    {
        Drawer_->Cleanup();
        delete Drawer_;
        Drawer_ = NULL;
    }
}

void C_Blip::InitDrawer()
{
    Drawer_ = new O_Output;
    Drawer_->SetOwner(this);
    Drawer_->SetFlags(Flags_);

    Drawer_->SetImage(BlipImg_[0][0]);
}

void C_Blip::AddBlip(short x, short y, uchar side, long starttime) // time is in minutes
{
    BLIP *newblip, *cur;

    newblip = new BLIP;
    newblip->x = x;
    newblip->y = y;
    newblip->side = side;
    newblip->state = 0;
    newblip->time = starttime;
    newblip->Next = NULL;

    if ( not Root_)
        Root_ = newblip;
    else
    {
        cur = Root_;

        while (cur->Next)
            cur = cur->Next;

        cur->Next = newblip;
    }

    if (Last_)
        Last_->state = 4;

    Last_ = newblip;
}

void C_Blip::BlinkLast()
{
    if (Last_)
    {
        Last_->state++;
        Last_->state and_eq 0x03;
        Refresh(Last_);
    }
}

void C_Blip::Update(long curtime) // time is in minutes
{
    BLIP *cur;

    Refresh();

    while (Root_ and Root_->time < (curtime - CUTOFF_TIME))
    {
        cur = Root_;
        Root_ = Root_->Next;

        if (cur == Last_)
            Last_ = NULL;

        delete cur;
    }

    cur = Root_;

    while (cur and cur not_eq Last_)
    {
        if (cur->time < (curtime - STATE_7_TIME))
            cur->state = 7;
        else if (cur->time < (curtime - STATE_6_TIME))
            cur->state = 6;
        else if (cur->time < (curtime - STATE_5_TIME))
            cur->state = 5;
        else
            cur->state = 4;

        cur = cur->Next;
    }
}

void C_Blip::RemoveAll()
{
    BLIP *cur, *last;

    cur = Root_;

    while (cur)
    {
        last = cur;
        cur = cur->Next;
        delete last;
    }

    Root_ = NULL;
    Last_ = NULL;
}

void C_Blip::Refresh(BLIP *blip)
{
    F4CSECTIONHANDLE *Leave;

    if ((Flags_ bitand C_BIT_INVISIBLE) or not Parent_ or not Drawer_)
        return;

    Leave = UI_Enter(Parent_);
    Drawer_->SetXY(blip->x, blip->y);
    Drawer_->Refresh();
    UI_Leave(Leave);
}

void C_Blip::Refresh()
{
    BLIP *cur;
    F4CSECTIONHANDLE *Leave;

    if ((Flags_ bitand C_BIT_INVISIBLE) or not Parent_ or not Drawer_)
        return;

    Leave = UI_Enter(Parent_);
    cur = Root_;

    while (cur)
    {
        Drawer_->SetXY(cur->x, cur->y);
        Drawer_->Refresh();
        cur = cur->Next;
    }

    UI_Leave(Leave);
}

void C_Blip::Draw(SCREEN *surface, UI95_RECT *cliprect)
{
    BLIP *cur;

    if ((Flags_ bitand C_BIT_INVISIBLE) or not Parent_ or not Drawer_)
        return;

    cur = Root_;

    while (cur)
    {
        Drawer_->SetXY(cur->x, cur->y);
        Drawer_->SetImagePtr(BlipImg_[cur->side bitand 7][cur->state bitand 7]);
        Drawer_->Draw(surface, cliprect);
        cur = cur->Next;
    }
}

#ifdef _UI95_PARSER_

short C_Blip::LocalFind(char *token)
{
    short i = 0;

    while (C_Blip_Tokens[i])
    {
        if (strnicmp(token, C_Blip_Tokens[i], strlen(C_Blip_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}


extern char ParseSave[];
extern char ParseCRLF[];

#endif // PARSER
