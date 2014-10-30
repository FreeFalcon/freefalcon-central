#include <windows.h>
#include "chandler.h"

#ifdef _UI95_PARSER_ // List of Keywords bitand functions to handle them

enum
{
    CLIN_NOTHING = 0,
    CLIN_SETUP,
    CLIN_SETCOLOR,
};

char *C_Line_Tokens[] =
{
    "[NOTHING]",
    "[SETUP]",
    "[COLOR]",
    0,
};

#endif

C_Line::C_Line() : C_Base()
{
    _SetCType_(_CNTL_LINE_);
    Color_ = 0;
    DefaultFlags_ = C_BIT_ENABLED bitor C_BIT_REMOVE;
}

C_Line::C_Line(char **stream) : C_Base(stream)
{
}

C_Line::C_Line(FILE *fp) : C_Base(fp)
{
}

C_Line::~C_Line()
{
}

long C_Line::Size()
{
    return(0);
}

void C_Line::Setup(long ID, short Type)
{
    SetID(ID);
    SetType(Type);
    SetDefaultFlags();
    SetReady(1);
}


void C_Line::Cleanup(void)
{
}

void C_Line::SetColor(COLORREF color)
{
    Color_ = color;
}

void C_Line::Refresh()
{
    if (GetFlags() bitand C_BIT_INVISIBLE or Parent_ == NULL)
        return;

    Parent_->SetUpdateRect(GetX(), GetY(), GetX() + GetW() + 1, GetY() + GetH() + 1, GetFlags(), GetClient());
}

void C_Line::Draw(SCREEN *surface, UI95_RECT *cliprect)
{
    if (GetFlags() bitand C_BIT_INVISIBLE or Parent_ == NULL)
        return;

    Parent_->BlitFill(surface, Color_, GetX(), GetY(), GetW(), GetH(), GetFlags(), GetClient(), cliprect);
}

#ifdef _UI95_PARSER_
short C_Line::LocalFind(char *token)
{
    short i = 0;

    while (C_Line_Tokens[i])
    {
        if (strnicmp(token, C_Line_Tokens[i], strlen(C_Line_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_Line::LocalFunction(short ID, long P[], _TCHAR *, C_Handler *)
{
    switch (ID)
    {
        case CLIN_SETUP:
            Setup(P[0], (short)P[1]);
            break;

        case CLIN_SETCOLOR:
            SetColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;
    }
}

extern char ParseSave[];
extern char ParseCRLF[];

#endif // PARSER
