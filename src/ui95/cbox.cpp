#include <windows.h>
#include "chandler.h"

#ifdef _UI95_PARSER_ // List of Keywords bitand functions to handle them

enum
{
    CBOX_NOTHING = 0,
    CBOX_SETUP,
    CBOX_SETCOLOR,
};

char *C_Box_Tokens[] =
{
    "[NOTHING]",
    "[SETUP]",
    "[COLOR]",
    0,
};

#endif

C_Box::C_Box() : C_Base()
{
    _SetCType_(_CNTL_BOX_);
    Color_ = 0;
    DefaultFlags_ = C_BIT_ENABLED bitor C_BIT_REMOVE;
}

C_Box::C_Box(char **stream) : C_Base(stream)
{
}

C_Box::C_Box(FILE *fp) : C_Base(fp)
{
}

C_Box::~C_Box()
{
}

long C_Box::Size()
{
    return(0);
}

void C_Box::Setup(long ID, short Type)
{
    SetID(ID);
    SetType(Type);
    SetDefaultFlags();
    SetReady(1);
}


void C_Box::Cleanup(void)
{
}

void C_Box::SetColor(COLORREF color)
{
    Color_ = color;
}
void C_Box::Refresh()
{
    if (Flags_ bitand C_BIT_INVISIBLE or Parent_ == NULL)
        return;

    Parent_->SetUpdateRect(GetX(), GetY(), GetX() + GetW() + 1, GetY() + GetH() + 1, GetFlags(), GetClient());
}

void C_Box::Draw(SCREEN *surface, UI95_RECT *cliprect)
{
    if (Flags_ bitand C_BIT_INVISIBLE or Parent_ == NULL)
        return;

    Parent_->DrawHLine(surface, Color_, GetX(), GetY(), GetW() + 1, GetFlags(), GetClient(), cliprect);
    Parent_->DrawHLine(surface, Color_, GetX(), GetY() + GetH(), GetW() + 1, GetFlags(), GetClient(), cliprect);
    Parent_->DrawVLine(surface, Color_, GetX(), GetY(), GetH() + 1, GetFlags(), GetClient(), cliprect);
    Parent_->DrawVLine(surface, Color_, GetX() + GetW(), GetY(), GetH() + 1, GetFlags(), GetClient(), cliprect);
}

#ifdef _UI95_PARSER_

short C_Box::LocalFind(char *token)
{
    short i = 0;

    while (C_Box_Tokens[i])
    {
        if (strnicmp(token, C_Box_Tokens[i], strlen(C_Box_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_Box::LocalFunction(short ID, long P[], _TCHAR *, C_Handler *)
{
    switch (ID)
    {
        case CBOX_SETUP:
            Setup(P[0], (short)P[1]);
            break;

        case CBOX_SETCOLOR:
            SetColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;
    }
}

extern char ParseSave[];
extern char ParseCRLF[];

#endif // PARSER
