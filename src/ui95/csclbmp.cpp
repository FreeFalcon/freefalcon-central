#include <windows.h>
#include "chandler.h"

#ifdef _UI95_PARSER_ // List of Keywords bitand functions to handle them

enum
{
    CSMP_NOTHING = 0,
    CSMP_SETUP,
    CSMP_SETSTRETCHRECT,
};

char *C_Smp_Tokens[] =
{
    "[NOTHING]",
    "[SETUP]",
    "[STRETCHRECT]",
    0,
};

#endif

extern WORD UIColorTable[201][256];
extern WORD rShift[], bShift[], gShift[];

C_ScaleBitmap::C_ScaleBitmap() : C_Base()
{
    short i;

    ImageID_ = 0;
    Image_ = NULL;
    Overlay_ = NULL;
    UseOverlay_ = 0;
    r_shift_ = 0;
    g_shift_ = 0;
    b_shift_ = 0;

    for (i = 0; i < 16; i++)
        Palette_[i] = NULL;

    _SetCType_(_CNTL_SCALEBITMAP_);
    SetReady(0);
    DefaultFlags_ = C_BIT_ENABLED;
}

C_ScaleBitmap::C_ScaleBitmap(char **stream) : C_Base(stream)
{
}

C_ScaleBitmap::C_ScaleBitmap(FILE *fp) : C_Base(fp)
{
}

C_ScaleBitmap::~C_ScaleBitmap()
{
}

long C_ScaleBitmap::Size()
{
    return(0);
}

void C_ScaleBitmap::Setup(long ID, short Type, long ImageID)
{
    SetID(ID);
    SetType(Type);
    SetDefaultFlags();
    SetImage(ImageID);
}

void C_ScaleBitmap::Cleanup()
{
    long i;

    if (Image_)
    {
        Image_->Cleanup();
        delete Image_;
        Image_ = NULL;
    }

    if (Overlay_)
#ifdef USE_SH_POOLS
        MemFreePtr(Overlay_);

#else
        delete Overlay_;
#endif

    for (i = 1; i < 16; i++)
        if (Palette_[i])
#ifdef USE_SH_POOLS
            MemFreePtr(Palette_[i]);

#else
            delete Palette_[i];
#endif
}

void C_ScaleBitmap::InitOverlay()
{
    IMAGE_RSC *img;
    DWORD tmp;

    if ( not Image_ or not Parent_)
        return;

    img = Image_->GetImage();

    if ( not img)
        return;

    if (Overlay_)
#ifdef USE_SH_POOLS
        MemFreePtr(Overlay_);

#else
        delete Overlay_;
#endif

#ifdef USE_SH_POOLS
    Overlay_ = (BYTE*)MemAllocPtr(UI_Pools[UI_ART_POOL], sizeof(BYTE) * (img->Header->w * img->Header->h), FALSE);
#else
    Overlay_ = new BYTE[img->Header->w * img->Header->h];
#endif

    if (Overlay_)
    {

        Parent_->GetRGBValues(tmp, r_shift_, tmp, g_shift_, tmp, b_shift_);
        //Parent_->GetRGBValues(&tmp,&r_shift_,&tmp,&g_shift_,&tmp,&b_shift_);
        memset(Overlay_, 0, sizeof(BYTE) * (img->Header->w * img->Header->h));
    }
}

void C_ScaleBitmap::ClearOverlay()
{
    if (Overlay_)
        memset(Overlay_, 0, sizeof(BYTE)*GetW()*GetH());
}

void C_ScaleBitmap::PreparePalette(COLORREF color)
{
    int i, j, perc, bperc;
    IMAGE_RSC *img;
    WORD usecolor;
    long r, g, b;

    if ( not Image_)
        return;

    img = Image_->GetImage();

    if ( not img)
        return;

    for (i = 1; i < 16; i++)
        if (Palette_[i])
#ifdef USE_SH_POOLS
            MemFreePtr(Palette_[i]);

#else
            delete Palette_[i];
#endif
    Palette_[0] = img->GetPalette();

    for (i = 1; i < 16; i++)
#ifdef USE_SH_POOLS
        Palette_[i] = (WORD*)MemAllocPtr(UI_Pools[UI_ART_POOL], sizeof(WORD) * (img->Header->palettesize), FALSE);

#else
        Palette_[i] = new WORD[img->Header->palettesize];
#endif

    usecolor = UI95_RGB24Bit(color);

    for (i = 1; i < 10; i++)
    {
        perc = 10 * i;
        bperc = 100 - perc;

        for (j = 0; j < img->Header->palettesize; j++)
        {
            r = rShift[UIColorTable[100][UIColorTable[perc][(usecolor >> r_shift_) bitand 0x1f] +
                                         UIColorTable[bperc][(Palette_[0][j] >> r_shift_) bitand 0x1f]]];
            g = gShift[UIColorTable[100][UIColorTable[perc][(usecolor >> g_shift_) bitand 0x1f] +
                                         UIColorTable[bperc][(Palette_[0][j] >> g_shift_) bitand 0x1f]]];
            b = bShift[UIColorTable[100][UIColorTable[perc][(usecolor >> b_shift_) bitand 0x1f] +
                                         UIColorTable[bperc][(Palette_[0][j] >> b_shift_) bitand 0x1f]]];

            Palette_[i][j] = static_cast<short>(r bitor b bitor b);
        }
    }
}

void C_ScaleBitmap::SetImage(long ID)
{
    SetReady(0);

    if (Image_ == NULL)
    {
        Image_ = new O_Output;
        Image_->SetOwner(this);
    }

    Image_->SetFlags(GetFlags());
    Image_->SetScaleImage(ID);

    ImageID_ = ID;
    SetWH(Image_->GetW(), Image_->GetH());
    SetReady(1);
}

void C_ScaleBitmap::SetImage(IMAGE_RSC *tmp)
{
    if (tmp == NULL)
        return;

    if (Image_ == NULL)
    {
        Image_ = new O_Output;
        Image_->SetOwner(this);
    }

    Image_->SetFlags(GetFlags());
    Image_->SetScaleImage(tmp);

    SetWH(Image_->GetW(), Image_->GetH());
    SetReady(1);
}

void C_ScaleBitmap::SetFlags(long flags)
{
    SetControlFlags(flags);

    if (Image_)
        Image_->SetFlags(flags);
}

void C_ScaleBitmap::Refresh()
{
    if (GetFlags() bitand C_BIT_INVISIBLE or Parent_ == NULL)
        return;

    Image_->Refresh();
}

void C_ScaleBitmap::Draw(SCREEN *surface, UI95_RECT *cliprect)
{
    if (GetFlags() bitand C_BIT_INVISIBLE)
        return;

    if (Image_)
    {
        if (Overlay_ and UseOverlay_)
            Image_->Blend4Bit(surface, Overlay_, Palette_, cliprect);
        else
            Image_->Draw(surface, cliprect);
    }
}

#ifdef _UI95_PARSER_
short C_ScaleBitmap::LocalFind(char *token)
{
    short i = 0;

    while (C_Smp_Tokens[i])
    {
        if (strnicmp(token, C_Smp_Tokens[i], strlen(C_Smp_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_ScaleBitmap::LocalFunction(short ID, long P[], _TCHAR *, C_Handler *)
{
    switch (ID)
    {
        case CSMP_SETUP:
            Setup(P[0], (short)P[1], P[2]);
            break;
    }
}

extern char ParseSave[];
extern char ParseCRLF[];


#endif // PARSER

