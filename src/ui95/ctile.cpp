#include <windows.h>
#include "chandler.h"

#ifdef _UI95_PARSER_ // List of Keywords bitand functions to handle them

enum
{
    CTILE_NOTHING = 0,
    CTILE_SETUP,
    CTILE_SETSTRETCHRECT,
    CTILE_SETPERCENT,
};

char *C_Tile_Tokens[] =
{
    "[NOTHING]",
    "[SETUP]",
    "[STRETCHRECT]",
    "[PERCENT]",
    0,
};

#endif

C_Tile::C_Tile() : C_Base()
{
    Image_ = NULL;
    _SetCType_(_CNTL_TILE_);
    SetReady(0);
    DefaultFlags_ = C_BIT_ENABLED bitor C_BIT_REMOVE;
}

C_Tile::C_Tile(char **stream) : C_Base(stream)
{
}

C_Tile::C_Tile(FILE *fp) : C_Base(fp)
{
}

C_Tile::~C_Tile()
{
}

long C_Tile::Size()
{
    return(0);
}

void C_Tile::Setup(long ID, short Type, long ImageID)
{
    SetID(ID);
    SetType(Type);
    SetDefaultFlags();
    SetImage(ImageID);
}

void C_Tile::Cleanup()
{
    if (Image_)
    {
        Image_->Cleanup();
        delete Image_;
        Image_ = NULL;
    }
}

void C_Tile::SetImage(long ID)
{
    SetReady(0);

    if (Image_ == NULL)
    {
        Image_ = new O_Output;
        Image_->SetOwner(this);
    }

    Image_->SetFlags(GetFlags());
    Image_->SetImage(ID);

    if (Image_->GetW() and Image_->GetH())
        SetReady(1);
}

void C_Tile::SetImage(IMAGE_RSC *tmp)
{
    if (tmp == NULL)
        return;

    if (Image_ == NULL)
    {
        Image_ = new O_Output;
        Image_->SetOwner(this);
    }

    Image_->SetFlags(GetFlags());
    Image_->SetImage(tmp);

    if (Image_->GetW() and Image_->GetH())
        SetReady(1);
}

void C_Tile::SetFlags(long flags)
{
    SetControlFlags(flags);

    if (Image_)
        Image_->SetFlags(flags);
}

void C_Tile::Refresh()
{
    if (GetFlags() bitand C_BIT_INVISIBLE or not Ready() or not Parent_)
        return;

    Parent_->SetUpdateRect(GetX(), GetY(), GetX() + GetW(), GetY() + GetH(), GetFlags(), GetClient());
}

void C_Tile::Draw(SCREEN *surface, UI95_RECT *cliprect)
{
    UI95_RECT clip;
    long x, y;

    if (GetFlags() bitand C_BIT_INVISIBLE or not Ready() or not Parent_)
        return;

    clip = *cliprect;

    if (GetFlags() bitand C_BIT_ABSOLUTE)
    {
        if (clip.left < GetX()) clip.left = GetX();

        if (clip.top < GetY()) clip.top = GetY();

        if (clip.right > (GetX() + GetW())) clip.right = GetX() + GetW();

        if (clip.bottom > (GetY() + GetH())) clip.bottom = GetY() + GetH();
    }

    y = 0;

    while (y < GetH())
    {
        x = 0;

        while (x < GetW())
        {
            Image_->SetXY(x, y);
            Image_->SetInfo();
            Image_->Draw(surface, &clip);

            if (GetFlags() bitand C_BIT_HORIZONTAL)
                x += Image_->GetW();
            else
                x = GetW();
        }

        if (GetFlags() bitand C_BIT_VERTICAL)
            y += Image_->GetH();
        else
            y = GetH();
    }
}

#ifdef _UI95_PARSER_
short C_Tile::LocalFind(char *token)
{
    short i = 0;

    while (C_Tile_Tokens[i])
    {
        if (strnicmp(token, C_Tile_Tokens[i], strlen(C_Tile_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_Tile::LocalFunction(short ID, long P[], _TCHAR *, C_Handler *)
{
    switch (ID)
    {
        case CTILE_SETUP:
            Setup(P[0], (short)P[1], P[2]);
            break;

        case CTILE_SETPERCENT:
            if (Image_)
            {
                Image_->SetFrontPerc((short)P[0]);
                Image_->SetBackPerc(100 - (short)P[0]);
            }

            break;
    }
}

extern char ParseSave[];
extern char ParseCRLF[];

#endif // PARSER

