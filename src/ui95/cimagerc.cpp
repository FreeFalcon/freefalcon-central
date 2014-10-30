#include <windows.h>
#include <ddraw.h>
#include "debuggr.h"
#include "dxutil/ddutil.h"
#include "ui/include/targa.h"
#include "chandler.h"

#ifdef _UI95_PARSER_

enum
{
    CIMG_NOTHING = 0,
    CIMG_LOADIMAGE,
    CIMG_LOADFILE,
    CIMG_ADDIMAGE,
    CIMG_LOADRES,
    CIMG_LOADPRIVATERES,
};

char *C_Img_Tokens[] =
{
    "[NOTHING]",
    "[LOADIMAGE]",
    "[LOADFILE]",
    "[ADDIMAGE]",
    "[LOADRES]",
    "[LOADPRIVATERES]",
    0,
};

extern C_Parser *gMainParser;

#endif // PARSER

C_Image *gImageMgr = NULL;

#define _IMAGE_HASH_SIZE_ (512)
#define _IMAGE_SUB_HASH_SIZE_ (20)

static void DelResCB(void *me)
{
    C_Resmgr *res;

    res = (C_Resmgr*)me;

    if (res)
    {
        res->Cleanup();
        delete res;
    }
}

C_Image::C_Image()
{
    Root_ = NULL;
    Finder_ = NULL;
    ColorKey_ = 0x7c1f;
    red_shift_ = 0;
    green_shift_ = 0;
    blue_shift_ = 0;
    ColorOrder_ = NULL;
    IDOrder_ = NULL;

#ifdef _UI95_PARSER_
    LastID_ = 0;
#endif
}

C_Image::~C_Image()
{
    if (Root_)
        Cleanup();
}

void C_Image::Setup()
{
    if (Root_ or Finder_)
        Cleanup();

    Root_ = new C_Hash;
    Root_->Setup(_IMAGE_HASH_SIZE_);
    Root_->SetFlags(C_BIT_REMOVE);
    Root_->SetCallback(DelResCB);

    Finder_ = new C_Hash;
    Finder_->Setup(_IMAGE_HASH_SIZE_);
}

void C_Image::Cleanup()
{
    if (Root_)
    {
        Root_->Cleanup();
        delete Root_;
        Root_ = NULL;
    }

    if (Finder_)
    {
        Finder_->Cleanup();
        delete Finder_;
        Finder_ = NULL;
    }

    if (ColorOrder_)
    {
        ColorOrder_->Cleanup();
        delete ColorOrder_;
        ColorOrder_ = NULL;
    }

    if (IDOrder_)
    {
        IDOrder_->Cleanup();
        delete IDOrder_;
        IDOrder_ = NULL;
    }
}

C_Resmgr *C_Image::AddImage(long ID, long LastID, UI95_RECT *rect, short x, short y)
{
    C_Resmgr *newres = NULL;
    IMAGE_RSC *newentry = NULL;
    IMAGE_RSC *prior = NULL;
    char *orig8 = NULL;
    char *data8 = NULL;
    char *dptr8 = NULL;
    char *sptr8 = NULL;
    WORD *orig16 = NULL;
    WORD *data16 = NULL;
    WORD *dptr16 = NULL;
    WORD *sptr16 = NULL;
    WORD *Palette = NULL;
    int neww = 0, newh = 0; 
    int i = 0, j = 0; 

    if (Root_->Find(ID) or Finder_->Find(ID))
    {
        MonoPrint("cimagerc error: [ID %1ld] Already used (AddImage)\n", ID);
        return(NULL);
    }

    prior = (IMAGE_RSC*)Finder_->Find(LastID);

    if ( not prior)
    {
        MonoPrint("NO prior image to reference (%1ld)\n", ID);
        return(NULL);
    }

    if (prior->Header->Type not_eq _RSC_IS_IMAGE_)
    {
        MonoPrint("(%1ld) is NOT an IMAGE_RSC (type=%1d)\n", ID, prior->Header->Type);
        return(NULL);
    }

    if (rect->left >= prior->Header->w or rect->top >= prior->Header->h)
    {
        MonoPrint("AddImage [ID %1ld] is outside of prior image's [ID %1ld] area\n", ID, LastID);
        return(NULL);
    }

    if (rect->right > prior->Header->w)
        rect->right = prior->Header->w;

    if (rect->bottom > prior->Header->h)
        rect->bottom = prior->Header->h;

    neww = rect->right  - rect->left;
    newh = rect->bottom - rect->top;

    if (prior->Header->flags bitand _RSC_8_BIT_)
    {
#ifdef USE_SH_POOLS
        data8 = (char*)MemAllocPtr(UI_Pools[UI_ART_POOL], sizeof(char) * (neww * newh + (prior->Header->palettesize * 2)), FALSE);
#else
        data8 = new char[neww * newh + (prior->Header->palettesize * 2)];
#endif
        Palette = (WORD*)(data8 + neww * newh);
        memcpy((char *)Palette, (char *)(prior->Owner->GetData() + prior->Header->paletteoffset), prior->Header->palettesize * 2);
        orig8 = (char *)(prior->Owner->GetData() + prior->Header->imageoffset);
        dptr8 = data8;
        orig8 += rect->top * prior->Header->w;

        for (i = rect->top; i < rect->bottom; i++)
        {
            sptr8 = orig8 + rect->left;

            for (j = rect->left; j < rect->right; j++)
                *dptr8++ = *sptr8++;

            orig8 += prior->Header->w;
        }
    }
    else
    {
#ifdef USE_SH_POOLS
        data16 = (WORD*)MemAllocPtr(UI_Pools[UI_ART_POOL], sizeof(WORD) * (neww * newh), FALSE);
#else
        data16 = new WORD[neww * newh];
#endif
        orig16 = (WORD *)(prior->Owner->GetData() + prior->Header->imageoffset);
        dptr16 = data16;
        orig16 += rect->top * prior->Header->w;

        for (i = rect->top; i < rect->bottom; i++)
        {
            sptr16 = orig16 + rect->left;

            for (j = rect->left; j < rect->right; j++)
                *dptr16++ = *sptr16++;

            orig16 += prior->Header->w;
        }
    }

    newres = new C_Resmgr;
    newres->Setup(ID);
    newres->SetScreenFormat(red_shift_, green_shift_, blue_shift_);
    newres->SetColorKey(ColorKey_);

    newentry = new IMAGE_RSC;
    newentry->ID = ID;
    newentry->Owner = newres;
    newentry->Header = new ImageHeader;
    newentry->Header->Type = _RSC_IS_IMAGE_;
    newentry->Header->ID[0] = 0;
    newentry->Header->flags = prior->Header->flags bitor _RSC_USECOLORKEY_;
    newentry->Header->centerx = x;
    newentry->Header->centery = y;
    newentry->Header->w = (short)neww; 
    newentry->Header->h = (short)newh; 
    newentry->Header->imageoffset = 0;

    if (newentry->Header->flags bitand _RSC_8_BIT_)
    {
        newres->SetData(data8);
        newentry->Header->palettesize = prior->Header->palettesize;
        newentry->Header->paletteoffset = neww * newh;
    }
    else
    {
        newentry->Header->palettesize = 0;
        newentry->Header->paletteoffset = 0;
        newres->SetData((char *)data16);
    }

    newres->AddIndex(ID, newentry);

    Root_->Add(ID, newres);
    Finder_->Add(ID, newentry);

    return(newres);
}

C_Resmgr *C_Image::AddImage(long ID, long LastID, short x, short y, short w, short h, short cx, short cy)
{
    C_Resmgr *newres = NULL;
    IMAGE_RSC *newentry = NULL;
    IMAGE_RSC *prior = NULL;
    char *orig8 = NULL;
    char *data8 = NULL;
    char *dptr8 = NULL;
    char *sptr8 = NULL;
    WORD *orig16 = NULL;
    WORD *data16 = NULL;
    WORD *dptr16 = NULL;
    WORD *sptr16 = NULL;
    WORD *Palette = NULL;
    int neww = 0, newh = 0; 
    int i = 0, j = 0; 

    if (Root_->Find(ID) or Finder_->Find(ID))
    {
        MonoPrint("cimagerc error: [ID %1ld] Already used (AddImage)\n", ID);
        return(NULL);
    }

    prior = (IMAGE_RSC*)Finder_->Find(LastID);

    if ( not prior)
    {
        MonoPrint("NO prior image to reference (%1ld)\n", ID);
        return(NULL);
    }

    if (prior->Header->Type not_eq _RSC_IS_IMAGE_)
    {
        MonoPrint("(%1ld) is NOT an IMAGE_RSC (type=%1d)\n", ID, prior->Header->Type);
        return(NULL);
    }

    if ( not prior->Owner)
    {
        MonoPrint("(%1ld) Data_ not loaded\n", ID, prior->Header->Type);
        return(NULL);
    }

    if (x >= prior->Header->w or y >= prior->Header->h)
    {
        MonoPrint("AddImage [ID %1ld] is outside of prior image's [ID %1ld] area\n", ID, LastID);
        return(NULL);
    }

    if ((x + w) > prior->Header->w)
        w = (short)(prior->Header->w - x); 

    if ((y + h) > prior->Header->h)
        h = (short)(prior->Header->h - y); 

    neww = w;
    newh = h;

    if (prior->Header->flags bitand _RSC_8_BIT_)
    {
#ifdef USE_SH_POOLS
        data8 = (char*)MemAllocPtr(UI_Pools[UI_ART_POOL], sizeof(char) * (neww * newh + (prior->Header->palettesize * 2)), FALSE);
#else
        data8 = new char[neww * newh + (prior->Header->palettesize * 2)];
#endif
        Palette = (WORD*)(data8 + neww * newh);
        memcpy((char *)Palette, (char *)(prior->Owner->GetData() + prior->Header->paletteoffset), prior->Header->palettesize * 2);
        orig8 = (char *)(prior->Owner->GetData() + prior->Header->imageoffset);
        dptr8 = data8;
        orig8 += y * prior->Header->w;

        for (i = y; i < (y + h); i++)
        {
            sptr8 = orig8 + x;

            for (j = x; j < (x + w); j++)
                *dptr8++ = *sptr8++;

            orig8 += prior->Header->w;
        }
    }
    else
    {
#ifdef USE_SH_POOLS
        data16 = (WORD*)MemAllocPtr(UI_Pools[UI_ART_POOL], sizeof(WORD) * (neww * newh), FALSE);
#else
        data16 = new WORD[neww * newh];
#endif
        orig16 = (WORD *)(prior->Owner->GetData() + prior->Header->imageoffset);
        dptr16 = data16;
        orig16 += y * prior->Header->w;

        for (i = y; i < (y + h); i++)
        {
            sptr16 = orig16 + x;

            for (j = x; j < (x + w); j++)
                *dptr16++ = *sptr16++;

            orig16 += prior->Header->w;
        }
    }

    newres = new C_Resmgr;
    newres->Setup(ID);
    newres->SetScreenFormat(red_shift_, green_shift_, blue_shift_);
    newres->SetColorKey(ColorKey_);

    newentry = new IMAGE_RSC;
    newentry->ID = ID;
    newentry->Owner = newres;
    newentry->Header = new ImageHeader;
    newentry->Header->Type = _RSC_IS_IMAGE_;
    newentry->Header->ID[0] = 0;
    newentry->Header->flags = prior->Header->flags bitor _RSC_USECOLORKEY_;
    newentry->Header->centerx = cx;
    newentry->Header->centery = cy;
    newentry->Header->w = (short)neww; 
    newentry->Header->h = (short)newh; 
    newentry->Header->imageoffset = 0;

    if (newentry->Header->flags bitand _RSC_8_BIT_)
    {
        newres->SetData(data8);
        newentry->Header->palettesize = prior->Header->palettesize;
        newentry->Header->paletteoffset = neww * newh;
    }
    else
    {
        newentry->Header->palettesize = 0;
        newentry->Header->paletteoffset = 0;
        newres->SetData((char*)data16);
    }

    newres->AddIndex(ID, newentry);

    Root_->Add(ID, newres);
    Finder_->Add(ID, newentry);

    return(newres);
}

long C_Image::BuildColorTable(WORD *, long , long , long)
{
#if 0
    long sidx;
    long count;

    if ( not img or not w or not h)
        return(0);

    if (ColorOrder_)
    {
        ColorOrder_->Cleanup();
        delete ColorOrder_;
    }

    if (IDOrder_)
    {
        IDOrder_->Cleanup();
        delete IDOrder_;
    }

    ColorOrder_ = new C_Hash;
    ColorOrder_->Setup(256);

    IDOrder_ = new C_Hash;
    IDOrder_->Setup(256);

    if (UseColorKey)
    {
        ColorOrder_->Add(ColorKey_, (void*)1);
        IDOrder_->Add(1, (void*)ColorKey_);
        count = 1;
    }
    else
        count = 0;

    sidx = w * h;

    while (sidx)
    {
        if ( not ColorOrder_->Find(img[sidx]))
        {
            ColorOrder_->Add(img[sidx], (void*)(count + 1));
            IDOrder_->Add(count, (void*)img[sidx]);
            count++;
        }

        sidx--;
    }

    return(count);
#endif
    return(0);
}

void C_Image::MakePalette(WORD *, long)
{
#if 0
    long i;

    if ( not dest or not entries)
        return;

    for (i = 0; i < entries; i++)
        dest[i] = (WORD)IDOrder_->Find(i);

#endif
}

void C_Image::ConvertTo8Bit(WORD *, unsigned char *, long, long)
{

#if 0
    long i, j, didx, start, sidx;

    if ( not src or not w or not h or not dest)
        return;

    if (dest)
    {
        didx = 0;
        start = (h - 1) * w;

        for (i = 0; i < h; i++)
        {
            sidx = start;

            for (j = 0; j < w; j++)
                dest[didx++] = (short)ColorOrder_->Find(src[sidx++]) - 1;

            start -= w;
        }
    }

#endif
}

void C_Image::CopyArea(WORD *src, WORD *dest, long w, long h)
{
    long i, j, didx, start, sidx;

    if ( not src or not w or not h or not dest)
        return;

    if (dest)
    {
        didx = 0;
        start = (h - 1) * w;

        for (i = 0; i < h; i++)
        {
            sidx = start;

            for (j = 0; j < w; j++)
                dest[didx++] = src[sidx++];

            start -= w;
        }
    }
}

C_Resmgr *C_Image::LoadImage(long ID, char *file, short x, short y)
{
    BITMAPINFO bmi;
    char *cptr;
    BOOL retval;
#if 0
    long colors;
    unsigned char *Image8;
#endif
    WORD *Image16;
    C_Resmgr *newres;
    IMAGE_RSC *newentry;

    if (Root_->Find(ID) or Finder_->Find(ID))
    {
        MonoPrint("cimagerc error: [ID %1ld] Already used (LoadFile [%s])\n", ID, file);
        return(NULL);
    }

    retval = LoadTargaFile(file, &cptr, &bmi);

    if ( not retval)
    {
        MonoPrint("Failed to load %s\n", file);
        return(NULL);
    }

    newres = new C_Resmgr;
    newres->Setup(ID);
    newres->SetColorKey(ColorKey_);
    newres->SetScreenFormat(red_shift_, green_shift_, blue_shift_);

    newentry = new IMAGE_RSC;
    newentry->ID = ID;
    newentry->Owner = newres;
    newentry->Header = new ImageHeader;
    newentry->Header->Type = _RSC_IS_IMAGE_;
    newentry->Header->ID[0] = 0;

    if (x == -1 and y == -1)
    {
        x = (short)(bmi.bmiHeader.biWidth  / 2);
        y = (short)(bmi.bmiHeader.biHeight / 2);
    }

    newentry->Header->centerx = x;
    newentry->Header->centery = y;
    newentry->Header->w = (short)bmi.bmiHeader.biWidth; 
    newentry->Header->h = (short)bmi.bmiHeader.biHeight; 
    newentry->Header->imageoffset = 0;
    newentry->Header->palettesize = 0;
    newentry->Header->paletteoffset = 0;
#if 0
    colors = BuildColorTable((WORD*)cptr, bmi.bmiHeader.biWidth, bmi.bmiHeader.biHeight, 0);

    if (colors and colors <= 256)
    {
#ifdef USE_SH_POOLS
        Image8 = (unsigned char*)MemAllocPtr(UI_Pools[UI_ART_POOL], sizeof(unsigned char) * (bmi.bmiHeader.biWidth * bmi.bmiHeader.biHeight + colors * 2), FALSE);
#else
        Image8 = new unsigned char[bmi.bmiHeader.biWidth * bmi.bmiHeader.biHeight + colors * 2];
#endif
        ConvertTo8Bit((WORD*)cptr, Image8, bmi.bmiHeader.biWidth, bmi.bmiHeader.biHeight);

        newentry->Header->flags = _RSC_8_BIT_;
        newentry->Header->palettesize = colors;
        newentry->Header->paletteoffset = bmi.bmiHeader.biWidth * bmi.bmiHeader.biHeight;
        MakePalette((WORD *)(Image8 + newentry->Header->paletteoffset), colors);
        newres->SetData((char *)Image8);
    }
    else
    {
#endif
        newentry->Header->flags = _RSC_16_BIT_ bitor _RSC_USECOLORKEY_;
#ifdef USE_SH_POOLS
        Image16 = (WORD*)MemAllocPtr(UI_Pools[UI_ART_POOL], sizeof(WORD) * (bmi.bmiHeader.biWidth * bmi.bmiHeader.biHeight), FALSE);
#else
        Image16 = new WORD[bmi.bmiHeader.biWidth * bmi.bmiHeader.biHeight];
#endif
        CopyArea((WORD*)cptr, Image16, bmi.bmiHeader.biWidth, bmi.bmiHeader.biHeight);
        newres->SetData((char *)Image16);
#if 0
    }

#endif
    delete cptr;
    newres->AddIndex(ID, newentry);
    newres->ConvertToScreen();

    Root_->Add(ID, newres);
    Finder_->Add(ID, newentry);

#ifdef _UI95_PARSER_
    LastID_ = ID;
#endif
    return(newres);
}

C_Resmgr *C_Image::LoadFile(long ID, char *file, short x, short y)
{
    BITMAPINFO bmi;
    char *cptr;
    BOOL retval;
#if 0
    long colors;
    unsigned char *Image8;
#endif
    WORD *Image16;
    C_Resmgr *newres;
    IMAGE_RSC *newentry;

    if (Root_->Find(ID) or Finder_->Find(ID))
    {
        MonoPrint("cimagerc error: [ID %1ld] Already used (LoadFile [%s])\n", ID, file);
        return(NULL);
    }

    retval = LoadTargaFile(file, &cptr, &bmi);

    if ( not retval)
    {
        MonoPrint("Failed to load %s\n", file);
        return(NULL);
    }

    newres = new C_Resmgr;
    newres->Setup(ID);
    newres->SetColorKey(ColorKey_);
    newres->SetScreenFormat(red_shift_, green_shift_, blue_shift_);

    newentry = new IMAGE_RSC;
    newentry->ID = ID;
    newentry->Owner = newres;
    newentry->Header = new ImageHeader;
    newentry->Header->Type = _RSC_IS_IMAGE_;
    newentry->Header->ID[0] = 0;

    if (x == -1 and y == -1)
    {
        x = (short)(bmi.bmiHeader.biWidth  / 2);
        y = (short)(bmi.bmiHeader.biHeight / 2);
    }

    newentry->Header->centerx = x;
    newentry->Header->centery = y;
    newentry->Header->w = (short)bmi.bmiHeader.biWidth; 
    newentry->Header->h = (short)bmi.bmiHeader.biHeight; 
    newentry->Header->imageoffset = 0;
    newentry->Header->palettesize = 0;
    newentry->Header->paletteoffset = 0;
#if 0
    colors = BuildColorTable((WORD*)cptr, bmi.bmiHeader.biWidth, bmi.bmiHeader.biHeight, 0);

    if (colors and colors <= 256)
    {
        Image8 = new unsigned char[bmi.bmiHeader.biWidth * bmi.bmiHeader.biHeight + colors * 2];
        ConvertTo8Bit((WORD*)cptr, Image8, bmi.bmiHeader.biWidth, bmi.bmiHeader.biHeight);

        newentry->Header->flags = _RSC_8_BIT_;
        newentry->Header->palettesize = colors;
        newentry->Header->paletteoffset = bmi.bmiHeader.biWidth * bmi.bmiHeader.biHeight;
        MakePalette((WORD *)(Image8 + newentry->Header->paletteoffset), colors);
        newres->SetData((char *)Image8);
    }
    else
    {
#endif
        newentry->Header->flags = _RSC_16_BIT_;
#ifdef USE_SH_POOLS
        Image16 = (WORD*)MemAllocPtr(UI_Pools[UI_ART_POOL], sizeof(WORD) * (bmi.bmiHeader.biWidth * bmi.bmiHeader.biHeight), FALSE);
#else
        Image16 = new WORD[bmi.bmiHeader.biWidth * bmi.bmiHeader.biHeight];
#endif
        CopyArea((WORD*)cptr, Image16, bmi.bmiHeader.biWidth, bmi.bmiHeader.biHeight);
        newres->SetData((char *)Image16);
#if 0
    }

#endif
    delete cptr;
    newres->AddIndex(ID, newentry);
    newres->ConvertToScreen();

    Root_->Add(ID, newres);
    Finder_->Add(ID, newentry);

#ifdef _UI95_PARSER_
    LastID_ = ID;
#endif
    return(newres);
}

C_Resmgr *C_Image::LoadPrivateRes(long ID, char *filename)
{
    C_Resmgr *res;

    if ( not ID or not filename or not Root_)
        return(NULL);

    if (Root_->Find(ID))
        return(NULL);

    res = new C_Resmgr;

    if ( not res)
        return(NULL);

    res->Setup(ID, filename, gMainParser->GetTokenHash());
    res->SetScreenFormat(red_shift_, green_shift_, blue_shift_);
    res->SetColorKey(ColorKey_);
    res->LoadData();

    Root_->Add(ID, res);
    return(res);
}

C_Resmgr *C_Image::LoadRes(long ID, char *filename)
{
    C_HASHNODE *current;
    long curidx;
    C_Resmgr *res;
    C_Hash   *resIDs;
    IMAGE_RSC *rec;

    res = LoadPrivateRes(ID, filename);

    if (res)
    {
        resIDs = res->GetIDList();

        if (resIDs)
        {
            rec = (IMAGE_RSC*)resIDs->GetFirst(&current, &curidx);

            while (rec)
            {
                Finder_->Add(rec->ID, rec);
                rec = (IMAGE_RSC*)resIDs->GetNext(&current, &curidx);
            }

            return(res);
        }
    }

    return(NULL);
}

IMAGE_RSC *C_Image::GetImage(long ID)
{
    IMAGE_RSC *tmp;

    tmp = (IMAGE_RSC *)Finder_->Find(ID);

    if (tmp and tmp->Header->Type == _RSC_IS_IMAGE_)
        return(tmp);

    return(NULL);
}

C_Resmgr *C_Image::GetImageRes(long ID)
{
    return((C_Resmgr*)Root_->Find(ID));
}

BOOL C_Image::RemoveImage(long ID)
{
    IMAGE_RSC *rsc;

    rsc = GetImage(ID);

    if (rsc)
    {
        if (rsc->Owner->GetType() == _RSC_SINGLE_)
        {
            Finder_->Remove(ID);
            Root_->Remove(rsc->Owner->GetID());
            return(TRUE);
        }
    }

    return(FALSE);
}

#ifdef _UI95_PARSER_
short C_Image::LocalFind(char *token)
{
    short i = 0;

    while (C_Img_Tokens[i])
    {
        if (strnicmp(token, C_Img_Tokens[i], strlen(C_Img_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_Image::LocalFunction(short ID, long P[], _TCHAR *str, C_Handler *)
{
    switch (ID)
    {
        case CIMG_LOADIMAGE:
            LoadImage(P[0], str, (short)P[1], (short)P[2]);
            break;

        case CIMG_LOADFILE:
            LoadFile(P[0], str, (short)P[1], (short)P[2]);
            break;

        case CIMG_ADDIMAGE:
            AddImage(P[0], LastID_, (short)P[1], (short)P[2], (short)P[3], (short)P[4], (short)P[5], (short)P[6]);
            break;

        case CIMG_LOADRES:
            LoadRes(P[0], str);
            break;

        case CIMG_LOADPRIVATERES:
            LoadPrivateRes(P[0], str);
            break;
    }
}

#endif // PARSER
