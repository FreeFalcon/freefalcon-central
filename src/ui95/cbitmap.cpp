#include <windows.h>
#include "chandler.h"

#ifdef _UI95_PARSER_ // List of Keywords bitand functions to handle them

enum
{
    CBMP_NOTHING = 0,
    CBMP_SETUP,
    CBMP_SETSTRETCHRECT,
    CBMP_SETPERCENT,
};

char *C_Bmp_Tokens[] =
{
    "[NOTHING]",
    "[SETUP]",
    "[STRETCHRECT]",
    "[PERCENT]",
    0,
};

#endif

C_Bitmap::C_Bitmap() : C_Base()
{
    Image_ = NULL;
    TimerCallback_ = NULL;
    _SetCType_(_CNTL_BITMAP_);
    SetReady(0);
    DefaultFlags_ = C_BIT_ENABLED bitor C_BIT_REMOVE;
}

C_Bitmap::C_Bitmap(char **stream) : C_Base(stream)
{
}

C_Bitmap::C_Bitmap(FILE *fp) : C_Base(fp)
{
}

C_Bitmap::~C_Bitmap()
{
}

long C_Bitmap::Size()
{
    return(0);
}

void C_Bitmap::Setup(long ID, short Type, long ImageID)
{
    SetID(ID);
    SetType(Type);
    SetDefaultFlags();
    SetImage(ImageID);
}

void C_Bitmap::Cleanup()
{
    if (Image_)
    {
        Image_->Cleanup();
        delete Image_;
        Image_ = NULL;
    }
}

void C_Bitmap::SetImage(long ID)
{
    SetReady(0);

    if (Image_ == NULL)
    {
        Image_ = new O_Output;
        Image_->SetOwner(this);
    }

    Image_->SetFlags(GetFlags());
    Image_->SetImage(ID);

    SetWH(Image_->GetW(), Image_->GetH());
    SetReady(1);
}

void C_Bitmap::SetImage(IMAGE_RSC *tmp)
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

    SetWH(Image_->GetW(), Image_->GetH());
    SetReady(1);
}

void C_Bitmap::SetFlags(long flags)
{
    SetControlFlags(flags);

    if (Image_)
    {
        Image_->SetFlags(flags);
        Image_->SetInfo();
    }
}

BOOL C_Bitmap::TimerUpdate()
{
    if (TimerCallback_)
        return(TimerCallback_(this));

    return(FALSE);
}
void C_Bitmap::Refresh()
{
    if (GetFlags() bitand C_BIT_INVISIBLE or Parent_ == NULL)
        return;

    if (Image_)
        Image_->Refresh();
}

void C_Bitmap::Draw(SCREEN *surface, UI95_RECT *cliprect)
{
    if (GetFlags() bitand C_BIT_INVISIBLE)
        return;

    if (Image_)
        Image_->Draw(surface, cliprect);
}

#ifdef _UI95_PARSER_
short C_Bitmap::LocalFind(char *token)
{
    short i = 0;

    while (C_Bmp_Tokens[i])
    {
        if (strnicmp(token, C_Bmp_Tokens[i], strlen(C_Bmp_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_Bitmap::LocalFunction(short ID, long P[], _TCHAR *, C_Handler *)
{
    switch (ID)
    {
        case CBMP_SETUP:
            Setup(P[0], (short)P[1], P[2]);
            break;

        case CBMP_SETPERCENT:
            if (Image_)
            {
                Image_->SetFrontPerc((short)P[0]);
                Image_->SetBackPerc((short)(100 - P[0]));
            }

            break;
    }
}

extern char ParseSave[];
extern char ParseCRLF[];

#endif // PARSER

