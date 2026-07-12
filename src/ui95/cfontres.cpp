#include <windows.h>
#include "chandler.h"



//XX
extern DWORD RGB565toRGB8(WORD sc);


C_Fontmgr::C_Fontmgr()
{
    ID_ = 0;

    name_[0] = 0;
    height_ = 0;
    first_ = 0;
    last_ = 0;

    bytesperline_ = 0;

    fNumChars_ = 0;
    fontTable_ = NULL;

    dSize_ = 0;
    fontData_ = NULL;

    kNumKerns_ = 0;
    kernList_ = NULL;
}

C_Fontmgr::~C_Fontmgr()
{
    if (fontTable_ or fontData_ or kernList_)
        Cleanup();
}

void C_Fontmgr::Setup(long ID, char *fontfile)
{
    FILE *fp;

    ID_ = ID;

    fp = fopen(fontfile, "rb");

    if ( not fp)
    {
        MonoPrint("FONT error: %s not opened\n", fontfile);
        return;
    }

    fread(&name_, 32, 1, fp);
    fread(&height_, sizeof(long), 1, fp);
    fread(&first_, sizeof(short), 1, fp);
    fread(&last_, sizeof(short), 1, fp);
    fread(&bytesperline_, sizeof(long), 1, fp);
    fread(&fNumChars_, sizeof(long), 1, fp);
    fread(&kNumKerns_, sizeof(long), 1, fp);
    fread(&dSize_, sizeof(long), 1, fp);

    if (fNumChars_)
    {
        fontTable_ = new CharStr[fNumChars_];
        fread(fontTable_, sizeof(CharStr), fNumChars_, fp);
    }

    if (kNumKerns_)
    {
        kernList_ = new KerningStr[kNumKerns_];
        fread(kernList_, sizeof(KerningStr), kNumKerns_, fp);
    }

    if (dSize_)
    {
#ifdef USE_SH_POOLS
        fontData_ = (char*)MemAllocPtr(UI_Pools[UI_GENERAL_POOL], sizeof(char) * (dSize_), FALSE);
#else
        fontData_ = new char[dSize_];
#endif
        fread(fontData_, dSize_, 1, fp);
    }

    fclose(fp);
}

void C_Fontmgr::Save(char *filename)
{
    FILE *fp;

    fp = fopen(filename, "wb");

    if ( not fp)
    {
        MonoPrint("FONT error: can't create %s\n", filename);
        return;
    }

    fwrite(&name_, 32, 1, fp);
    fwrite(&height_, sizeof(long), 1, fp);
    fwrite(&first_, sizeof(short), 1, fp);
    fwrite(&last_, sizeof(short), 1, fp);
    fwrite(&bytesperline_, sizeof(long), 1, fp);
    fwrite(&fNumChars_, sizeof(long), 1, fp);
    fwrite(&kNumKerns_, sizeof(long), 1, fp);
    fwrite(&dSize_, sizeof(long), 1, fp);

    if (fNumChars_)
    {
        fwrite(fontTable_, sizeof(CharStr), fNumChars_, fp);
    }

    if (kNumKerns_)
    {
        fwrite(kernList_, sizeof(KerningStr), kNumKerns_, fp);
    }

    if (dSize_)
    {
        fwrite(fontData_, dSize_, 1, fp);
    }

    fclose(fp);
}

void C_Fontmgr::Cleanup()
{
    ID_ = 0;
    first_ = 0;
    last_ = 0;
    bytesperline_ = 0;

    fNumChars_ = 0;

    if (fontTable_)
    {
        delete fontTable_;
        fontTable_ = NULL;
    }

    kNumKerns_ = 0;

    if (kernList_)
    {
        delete kernList_;
        kernList_ = NULL;
    }

    dSize_ = 0;

    if (fontData_)
    {
#ifdef USE_SH_POOLS
        MemFreePtr(fontData_);
#else
        delete fontData_;
#endif
        fontData_ = NULL;
    }
}

long C_Fontmgr::Width(_TCHAR *str)
{
    long i;
    long size;
    long thechar;

    if ( not str)
        return(0);

    size = 0;
    i = 0;

    while (str[i])
        //while( not F4IsBadReadPtr(&(str[i]), sizeof(_TCHAR)) and str[i]) // JB 010401 CTD (too much CPU)
    {
        thechar = str[i] bitand 0xff;

        if (thechar >= first_ and thechar <= last_)
        {
            thechar -= first_;
            size += fontTable_[thechar].lead + fontTable_[thechar].w + fontTable_[thechar].trail;
        }

        i++;
    }

    return(size + 1);
}

long C_Fontmgr::Width(_TCHAR *str, long len)
{
    long i;
    long size;
    long thechar;

    if ( not str)
        return(0);

    size = 0;
    i = 0;

    while (str[i] and i < len)
    {
        thechar = str[i] bitand 0xff;

        if (thechar >= first_ and thechar <= last_)
        {
            thechar -= first_;
            size += fontTable_[thechar].lead + fontTable_[thechar].w + fontTable_[thechar].trail;
        }

        i++;
    }

    return(size + 1);
}

long C_Fontmgr::Height()
{
    return(height_);
}

CharStr *C_Fontmgr::GetChar(short ID)
{
    if (fontTable_ and ID >= first_ and ID <= last_)
        return(&fontTable_[ID - first_]);

    return(NULL);
}


extern WORD RGB8toRGB565(DWORD); //XX (chandler.cpp)

// true if the string contains at least one character outside the bitmap-font range
// (Cyrillic CP1251 0xC0-0xFF etc.) -- such text is drawn via GDI.
bool C_Fontmgr::NeedGDI(_TCHAR *str, long length)
{
    if ( not str)
        return false;

    // Treat any byte >= 0x80 as non-Latin (Cyrillic CP1251 etc.) and route
    // to GDI: the bitmap fonts here are Western; for 0x80-0xFF they're either empty or
    // 'box' glyphs, even if the code nominally falls in [first_, last_].
    for (long k = 0; k < length and str[k]; ++k)
    {
        if ((unsigned char)str[k] >= 0x80)
            return true;
    }

    return false;
}

// GDI path: render the whole string with a system TTF (RUSSIAN_CHARSET, Cyrillic)
// into a 32-bit DIB (white text on black = a coverage mask), then alpha-composite
// onto the SCREEN surface (16- or 32-bit). cliprect is optional.
void C_Fontmgr::DrawGDI(SCREEN *surface, _TCHAR *str, long length, WORD color, long x, long y, UI95_RECT *cliprect)
{
    if ( not surface or not str or length <= 0)
        return;

    int fh = (height_ > 2) ? (int)height_ : 12;

    HDC memDC = CreateCompatibleDC(NULL);

    if ( not memDC)
        return;

    HFONT hFont = CreateFontA(-(fh - 4), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              RUSSIAN_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                              ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
    HGDIOBJ oldFont = SelectObject(memDC, hFont);

    SIZE sz;
    ZeroMemory(&sz, sizeof(sz));
    GetTextExtentPoint32A(memDC, (LPCSTR)str, (int)length, &sz);
    int tw = sz.cx;
    int th = fh;

    if (tw <= 0 or tw > 4096)
    {
        SelectObject(memDC, oldFont);
        DeleteObject(hFont);
        DeleteDC(memDC);
        return;
    }

    BITMAPINFO bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = tw;
    bi.bmiHeader.biHeight = -th; // top-down
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    void *bits = NULL;
    HBITMAP dib = CreateDIBSection(memDC, &bi, DIB_RGB_COLORS, &bits, NULL, 0);

    if ( not dib)
    {
        SelectObject(memDC, oldFont);
        DeleteObject(hFont);
        DeleteDC(memDC);
        return;
    }

    HGDIOBJ oldBmp = SelectObject(memDC, dib);

    ZeroMemory(bits, (size_t)tw * th * 4); // black background
    SetBkMode(memDC, TRANSPARENT);
    SetTextColor(memDC, RGB(255, 255, 255)); // white -> pixel intensity = coverage
    TextOutA(memDC, 0, 0, (LPCSTR)str, (int)length);
    GdiFlush();

    DWORD trgb = RGB565toRGB8(color);
    int tr = (int)((trgb >> 16) & 0xFF);
    int tg = (int)((trgb >> 8) & 0xFF);
    int tb = (int)(trgb & 0xFF);
    bool b32 = (surface->bpp == 32);

    long clipL = 0, clipT = 0, clipR = surface->width, clipB = surface->height;

    if (cliprect)
    {
        if (cliprect->left > clipL) clipL = cliprect->left;
        if (cliprect->top > clipT) clipT = cliprect->top;
        if (cliprect->right < clipR) clipR = cliprect->right;
        if (cliprect->bottom < clipB) clipB = cliprect->bottom;
    }

    DWORD *srcpix = (DWORD *)bits;

    for (int py = 0; py < th; ++py)
    {
        long dy = y + py;

        if (dy < clipT or dy >= clipB)
            continue;

        for (int px = 0; px < tw; ++px)
        {
            int a = (int)(srcpix[py * tw + px] & 0xFF); // coverage 0..255

            if ( not a)
                continue;

            long dx = x + px;

            if (dx < clipL or dx >= clipR)
                continue;

            DWORD bg;
            WORD *d16 = NULL;
            DWORD *d32 = NULL;

            if (b32)
            {
                d32 = ((DWORD *)surface->mem) + (dy * surface->width + dx);
                bg = *d32;
            }
            else
            {
                d16 = surface->mem + (dy * surface->width + dx);
                bg = RGB565toRGB8(*d16);
            }

            int br = (int)((bg >> 16) & 0xFF);
            int bgg = (int)((bg >> 8) & 0xFF);
            int bb = (int)(bg & 0xFF);
            int rr = (tr * a + br * (255 - a)) / 255;
            int gg = (tg * a + bgg * (255 - a)) / 255;
            int bbv = (tb * a + bb * (255 - a)) / 255;
            DWORD outc = ((DWORD)rr << 16) | ((DWORD)gg << 8) | (DWORD)bbv;

            if (b32)
                *d32 = outc;
            else
                *d16 = RGB8toRGB565(outc);
        }
    }

    SelectObject(memDC, oldBmp);
    SelectObject(memDC, oldFont);
    DeleteObject(dib);
    DeleteObject(hFont);
    DeleteDC(memDC);
}


void C_Fontmgr::Draw(SCREEN *surface, _TCHAR *str, long length, WORD color, long x, long y)
{
    if (NeedGDI(str, length))
    {
        DrawGDI(surface, str, length, color, x, y, NULL);
        return;
    }

    long idx, i, j;
    long xoffset, yoffset;
    unsigned long thechar;
    unsigned char *sstart, *sptr, seg = 0;
    WORD *dstart, *dptr, *dendh, *dendv;
    bool b32 = surface->bpp == 32;//XX

    if ( not fontData_)
        return;

    if ( not str)
        return;

    idx = 0;
    xoffset = x;
    yoffset = y;

    if (b32) //XX
        dendv = surface->mem + 2 * (surface->width * surface->height);
    else
        dendv = surface->mem + surface->width * surface->height; // Make sure we don't go past the end of the surface


    while (str[idx] and idx < length)
    {
        thechar = str[idx] bitand 0xff;

        if (thechar >= (unsigned long)first_ and thechar <= (unsigned long)last_) 
        {
            thechar -= first_;
            xoffset += fontTable_[thechar].lead;

            sstart = (unsigned char *)(fontData_ + (thechar * bytesperline_ * height_));

            if (b32)
            {
                dstart = surface->mem + 2 * ((yoffset * surface->width) + xoffset);
                dendh = surface->mem + 2 * ((yoffset * surface->width) + surface->width);
            }
            else
            {
                dstart = surface->mem + (yoffset * surface->width) + xoffset;
                dendh = surface->mem + (yoffset * surface->width) + surface->width;
            }


            for (i = 0; i < height_ and dstart < dendv; i++)
            {
                dptr = dstart;
                sptr = sstart;

                for (j = 0; j < fontTable_[thechar].w; j++)
                {
                    if (dptr < dendh)
                    {
                        if ( not (j bitand 0x7))
                            seg = *sptr++;

                        //XX
                        //if(seg bitand 1)
                        // *dptr++=color;
                        //else
                        // dptr++;

                        if (seg bitand 1)
                        {
                            if (b32)
                                *((DWORD*)(dptr)) = RGB565toRGB8(color);
                            else
                                *dptr = color;
                        }

                        seg >>= 1;
                        ++dptr;

                        if (b32) //XX
                            ++dptr;
                    }
                }

                sstart += bytesperline_;
                dstart += surface->width;
                dendh += surface->width;

                if (b32)  //XX
                {
                    dstart += surface->width;
                    dendh += surface->width;
                }
            }

            xoffset += fontTable_[thechar].w + fontTable_[thechar].trail;
        }

        idx++;
    }
}

void C_Fontmgr::DrawSolid(SCREEN *surface, _TCHAR *str, long length, WORD color, WORD bgcolor, long x, long y)
{
    long idx, i, j;
    long xoffset, yoffset;
    unsigned long thechar;
    unsigned char *sstart, *sptr, seg = 0;
    WORD *dstart, *dptr, *dendh, *dendv;

    if ( not fontData_)
        return;

    if ( not str)
        return;

    bool b32 = surface->bpp == 32;//XX

    idx = 0;
    xoffset = x;
    yoffset = y;

    if (b32) //XX
        dendv = surface->mem + 2 * (surface->width * surface->height);
    else
        dendv = surface->mem + surface->width * surface->height; // Make sure we don't go past the end of the surface

    while (str[idx] and idx < length)
    {
        thechar = str[idx] bitand 0xff;

        if (thechar >= (unsigned long)first_ and thechar <= (unsigned long)last_) 
        {
            thechar -= first_;

            sstart = (unsigned char *)(fontData_ + (thechar * bytesperline_ * height_));

            if (b32) //XX word->dword
            {
                dstart = surface->mem + 2 * ((yoffset * surface->width) + xoffset);
                dendh = surface->mem + 2 * ((yoffset * surface->width) + surface->width);
            }
            else
            {
                dstart = surface->mem + (yoffset * surface->width) + xoffset;
                dendh = surface->mem + (yoffset * surface->width) + surface->width;
            }

            for (i = 0; i < height_ and dstart < dendv; i++)
            {
                dptr = dstart;
                sptr = sstart;

                for (j = 0; j < fontTable_[thechar].lead; j++)
                    if (dptr < dendh)
                    {
                        if (b32)
                        {
                            *((DWORD*)(dptr)) = RGB565toRGB8(bgcolor);
                            dptr += 2;
                        }
                        else
                            *dptr++ = bgcolor;
                    }

                for (j = 0; j < fontTable_[thechar].w; j++)
                {
                    if ( not (j bitand 0x7))
                        seg = *sptr++;

                    if (dptr < dendh)
                    {
                        if (b32) //XX
                        {
                            if (seg bitand 1)
                                *((DWORD*)(dptr)) = RGB565toRGB8(color);
                            else
                                *((DWORD*)(dptr)) = RGB565toRGB8(bgcolor);

                            dptr += 2;
                        }
                        else
                        {
                            if (seg bitand 1)
                                *dptr++ = color;
                            else
                                *dptr++ = bgcolor;
                        }

                        seg >>= 1;
                    }
                }

                for (j = 0; j < fontTable_[thechar].trail; j++)
                    if (dptr < dendh)
                    {
                        if (b32) //XX
                        {
                            *((DWORD*)(dptr)) = RGB565toRGB8(bgcolor);
                            dptr += 2;
                        }
                        else
                            *dptr++ = bgcolor;
                    }

                sstart += bytesperline_;
                dstart += surface->width;
                dendh += surface->width;

                if (b32) //XX
                {
                    dstart += surface->width;
                    dendh += surface->width;
                }
            }

            xoffset += fontTable_[thechar].lead + fontTable_[thechar].w + fontTable_[thechar].trail;
        }

        idx++;
    }
}

void C_Fontmgr::Draw(SCREEN *surface, _TCHAR *str, WORD color, long x, long y)
{
    if (str) Draw(surface, str, _tcsclen(str), color, x, y);
}

void C_Fontmgr::DrawSolid(SCREEN *surface, _TCHAR *str, WORD color, WORD bgcolor, long x, long y)
{
    if (str) DrawSolid(surface, str, _tcsclen(str), color, bgcolor, x, y);
}

void C_Fontmgr::_Draw16(SCREEN *surface, _TCHAR *str, long length, WORD color, long x, long y, UI95_RECT *cliprect)
// not void C_Fontmgr::Draw(SCREEN *surface,_TCHAR *str,short length,WORD color,long x,long y,UI95_RECT *cliprect)
{
    if (NeedGDI(str, length))
    {
        DrawGDI(surface, str, length, color, x, y, cliprect);
        return;
    }

    long idx, i, j;
    long xoffset, yoffset;
    unsigned long thechar;
    unsigned char *sstart, *sptr, seg = 0;
    WORD *dstart, *dptr;
    WORD *dendh, *dendv;
    WORD *dclipx, *dclipy;

    if ( not fontData_)
        return;

    if ( not str)
        return;

    idx = 0;
    xoffset = x;
    yoffset = y;
    dclipy = surface->mem + (cliprect->top * surface->width);
    dendv = surface->mem + (cliprect->bottom * surface->width); // Make sure we don't go past the end of the surface

    while (str[idx] and idx < length)
    {
        thechar = str[idx] bitand 0xff;

        if (thechar >= (unsigned long)first_ and thechar <= (unsigned long)last_)
        {
            thechar -= first_;
            xoffset += fontTable_[thechar].lead;

            sstart = (unsigned char *)(fontData_ + (thechar * bytesperline_ * height_));
            dstart = surface->mem + (yoffset * surface->width) + xoffset;
            dclipx = surface->mem + (yoffset * surface->width) + cliprect->left;
            dendh = dclipx + (cliprect->right - cliprect->left);

            for (i = 0; i < height_ and dstart < dendv; i++)
            {
                if (dstart >= dclipy)
                {
                    dptr = dstart;
                    sptr = sstart;

                    for (j = 0; j < fontTable_[thechar].w; j++)
                    {
                        if ( not (j bitand 0x7))
                        {
                            seg = *sptr++;

                            if ( not seg)
                            {
                                j += 7;
                                dptr += 8;
                                continue;
                            }
                        }

                        if (dptr < dendh)
                        {
                            if (dptr >= dclipx)
                            {
                                if (seg bitand 1)
                                    *dptr++ = color;
                                else
                                    dptr++;
                            }
                            else
                                dptr++;

                            seg >>= 1;
                        }
                    }
                }

                sstart += bytesperline_;
                dclipx += surface->width;
                dstart += surface->width;
                dendh += surface->width;
            }

            xoffset += fontTable_[thechar].w + fontTable_[thechar].trail;
        }

        idx++;
    }
}

void C_Fontmgr::_Draw32(SCREEN *surface, _TCHAR *str, long length, DWORD dwColor, long x, long y, UI95_RECT *cliprect)
{
    if (NeedGDI(str, length))
    {
        DrawGDI(surface, str, length, RGB8toRGB565(dwColor), x, y, cliprect);
        return;
    }

    long idx, i, j;
    long xoffset, yoffset;
    unsigned long thechar;
    unsigned char *sstart, *sptr, seg = 0;

    DWORD *dstart, *dptr;
    DWORD *dendh, *dendv;
    DWORD *dclipx, *dclipy;

    if ( not fontData_)
        return;

    if ( not str)
        return;

    DWORD *surfmem = (DWORD*) surface->mem;

    idx = 0;
    xoffset = x;
    yoffset = y;
    dclipy = surfmem + (cliprect->top * surface->width);
    dendv = surfmem + (cliprect->bottom * surface->width); // Make sure we don't go past the end of the surface

    while (str[idx] and idx < length)
    {
        thechar = str[idx] bitand 0xff;

        if (thechar >= (unsigned long)first_ and thechar <= (unsigned long)last_)
        {
            thechar -= first_;
            xoffset += fontTable_[thechar].lead;

            sstart = (unsigned char *)(fontData_ + (thechar * bytesperline_ * height_));

            dstart = surfmem + (yoffset * surface->width) + xoffset;
            dclipx = surfmem + (yoffset * surface->width) + cliprect->left;
            dendh = dclipx + (cliprect->right - cliprect->left);

            for (i = 0; i < height_ and dstart < dendv; i++)
            {
                if (dstart >= dclipy)
                {
                    dptr = dstart;
                    sptr = sstart;

                    for (j = 0; j < fontTable_[thechar].w; j++)
                    {
                        if ( not (j bitand 0x7))
                        {
                            seg = *sptr++;

                            if ( not seg)
                            {
                                j += 7;
                                dptr += 8;
                                continue;
                            }
                        }

                        if (dptr < dendh)
                        {
                            if (dptr >= dclipx)
                            {
                                if (seg bitand 1)
                                    *dptr++ = dwColor;
                                else
                                    dptr++;
                            }
                            else
                                dptr++;

                            seg >>= 1;
                        }
                    }
                }

                sstart += bytesperline_;
                dclipx += surface->width;
                dstart += surface->width;
                dendh += surface->width;
            }

            xoffset += fontTable_[thechar].w + fontTable_[thechar].trail;
        }

        idx++;
    }
}

//XX
void C_Fontmgr::DrawSolid(SCREEN *surface, _TCHAR *str, long length, WORD color, WORD bgcolor, long x, long y, UI95_RECT *cliprect)
{
    if (NeedGDI(str, length))
    {
        // Cyrillic: skip the background fill under the text (row highlight is drawn
        // separately), pass only the text itself to GDI
        DrawGDI(surface, str, length, color, x, y, cliprect);
        return;
    }

    if (surface->bpp == 32)
        _DrawSolid32(surface, str, length, RGB565toRGB8(color), RGB565toRGB8(bgcolor), x, y, cliprect);
    else
        _DrawSolid16(surface, str, length, color, bgcolor, x, y, cliprect);
}
//XX
void C_Fontmgr::_DrawSolid16(SCREEN *surface, _TCHAR *str, long length, WORD color, WORD bgcolor, long x, long y, UI95_RECT *cliprect)
// not void C_Fontmgr::DrawSolid(SCREEN *surface,_TCHAR *str,short length,WORD color,WORD bgcolor,long x,long y,UI95_RECT *cliprect)
{
    long idx, i, j;
    long xoffset, yoffset;
    unsigned long thechar;
    unsigned char *sstart, *sptr, seg = 0;
    WORD *dstart, *dptr;
    WORD *dendh, *dendv;
    WORD *dclipx, *dclipy;

    if ( not fontData_)
        return;

    if ( not str)
        return;

    idx = 0;
    xoffset = x;
    yoffset = y;
    dclipy = surface->mem + (cliprect->top * surface->width);
    dendv = surface->mem + (cliprect->bottom * surface->width); // Make sure we don't go past the end of the surface

    while (str[idx] and idx < length)
    {
        thechar = str[idx] bitand 0xff;

        if (thechar >= (unsigned long)first_ and thechar <= (unsigned long)last_)
        {
            thechar -= first_;

            sstart = (unsigned char *)(fontData_ + (thechar * bytesperline_ * height_));
            dstart = surface->mem + (yoffset * surface->width) + xoffset;
            dclipx = surface->mem + (yoffset * surface->width) + cliprect->left;
            dendh = dclipx + (cliprect->right - cliprect->left);

            for (i = 0; i < height_ and dstart < dendv; i++)
            {
                if (dstart >= dclipy)
                {
                    dptr = dstart;
                    sptr = sstart;

                    for (j = 0; j < fontTable_[thechar].lead; j++)
                    {
                        if (dptr < dendh)
                        {
                            if (dptr >= dclipx)
                                *dptr++ = bgcolor;
                            else
                                dptr++;
                        }
                    }

                    for (j = 0; j < fontTable_[thechar].w; j++)
                    {
                        if (dptr < dendh)
                        {
                            if ( not (j bitand 0x7))
                                seg = *sptr++;

                            if (dptr >= dclipx)
                            {
                                if (seg bitand 1)
                                    *dptr++ = color;
                                else
                                    *dptr++ = bgcolor;
                            }
                            else
                                dptr++;

                            seg >>= 1;
                        }
                    }

                    for (j = 0; j < fontTable_[thechar].trail; j++)
                    {
                        if (dptr < dendh)
                        {
                            if (dptr >= dclipx)
                                *dptr++ = bgcolor;
                            else
                                dptr++;
                        }
                    }
                }

                sstart += bytesperline_;
                dclipx += surface->width;
                dstart += surface->width;
                dendh += surface->width;
            }

            xoffset += fontTable_[thechar].lead + fontTable_[thechar].w + fontTable_[thechar].trail;
        }

        idx++;
    }
}

//XX
void C_Fontmgr::_DrawSolid32(SCREEN *surface, _TCHAR *str, long length, DWORD color, DWORD bgcolor, long x, long y, UI95_RECT *cliprect)
{
    long idx, i, j;
    long xoffset, yoffset;
    unsigned long thechar;
    unsigned char *sstart, *sptr, seg = 0;
    DWORD *dstart, *dptr;
    DWORD *dendh, *dendv;
    DWORD *dclipx, *dclipy;

    if ( not fontData_)
        return;

    if ( not str)
        return;

    DWORD *surfmem = (DWORD*)surface->mem;

    idx = 0;
    xoffset = x;
    yoffset = y;
    dclipy = surfmem + (cliprect->top * surface->width);
    dendv = surfmem + (cliprect->bottom * surface->width); // Make sure we don't go past the end of the surface

    while (str[idx] and idx < length)
    {
        thechar = str[idx] bitand 0xff;

        if (thechar >= (unsigned long)first_ and thechar <= (unsigned long)last_)
        {
            thechar -= first_;

            sstart = (unsigned char *)(fontData_ + (thechar * bytesperline_ * height_));
            dstart = surfmem + (yoffset * surface->width) + xoffset;
            dclipx = surfmem + (yoffset * surface->width) + cliprect->left;
            dendh = dclipx + (cliprect->right - cliprect->left);

            for (i = 0; i < height_ and dstart < dendv; i++)
            {
                if (dstart >= dclipy)
                {
                    dptr = dstart;
                    sptr = sstart;

                    for (j = 0; j < fontTable_[thechar].lead; j++)
                    {
                        if (dptr < dendh)
                        {
                            if (dptr >= dclipx)
                                *dptr++ = bgcolor;
                            else
                                dptr++;
                        }
                    }

                    for (j = 0; j < fontTable_[thechar].w; j++)
                    {
                        if (dptr < dendh)
                        {
                            if ( not (j bitand 0x7))
                                seg = *sptr++;

                            if (dptr >= dclipx)
                            {
                                if (seg bitand 1)
                                    *dptr++ = color;
                                else
                                    *dptr++ = bgcolor;
                            }
                            else
                                dptr++;

                            seg >>= 1;
                        }
                    }

                    for (j = 0; j < fontTable_[thechar].trail; j++)
                    {
                        if (dptr < dendh)
                        {
                            if (dptr >= dclipx)
                                *dptr++ = bgcolor;
                            else
                                dptr++;
                        }
                    }
                }

                sstart += bytesperline_;
                dclipx += surface->width;
                dstart += surface->width;
                dendh += surface->width;
            }

            xoffset += fontTable_[thechar].lead + fontTable_[thechar].w + fontTable_[thechar].trail;
        }

        idx++;
    }
}



void C_Fontmgr::Draw(SCREEN *surface, _TCHAR *str, WORD color, long x, long y, UI95_RECT *cliprect)
{
    if (str)
    {
        //XX
        if (surface->bpp == 32)
            _Draw32(surface, str, _tcsclen(str), RGB565toRGB8(color), x, y, cliprect);
        else
            _Draw16(surface, str, _tcsclen(str), color, x, y, cliprect);
    }
}

void C_Fontmgr::DrawSolid(SCREEN *surface, _TCHAR *str, WORD color, WORD bgcolor, long x, long y, UI95_RECT *cliprect)
{
    if (str)
        DrawSolid(surface, str, _tcsclen(str), color, bgcolor, x, y, cliprect);
}

//XX
void C_Fontmgr::Draw(SCREEN *surface, _TCHAR *str, long length, WORD color, long x, long y, UI95_RECT *cliprect)
{
    if (str)
    {
        if (NeedGDI(str, length))
        {
            DrawGDI(surface, str, length, color, x, y, cliprect);
            return;
        }

        //XX
        if (surface->bpp == 32)
            _Draw32(surface, str, length, RGB565toRGB8(color), x, y, cliprect);
        else
            _Draw16(surface, str, length, color, x, y, cliprect);
    }
}
