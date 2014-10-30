#include <cISO646>
#include <windows.h>
#include "chandler.h"

// IMAGE_RSC routines...
extern WORD UIColorTable[201][256];
extern WORD rShift[], bShift[], gShift[];


//XX
DWORD RGB565toRGB8(WORD sc)
{
    //unpack rgb565
    DWORD r = (sc >> 11) bitand 31;
    DWORD g = (sc >> 5) bitand 63;
    DWORD b = (sc bitand 31);
    r <<= 3;//scale 0..31 to 0..255
    g <<= 2;//scale 0..63 to 0..255
    b <<= 3;
    //pack rgb888
    return (r << 16) bitor (g << 8) bitor b; //ARGB
}
//XX
WORD RGB8toRGB565(DWORD sc)
{
    //unpack rgb565
    DWORD r = (sc >> 16) bitand 255;
    DWORD g = (sc >> 8) bitand 255;
    DWORD b = (sc bitand 255);
    r >>= 3;//scale to 0..31 from 0..255
    g >>= 2;//scale to 0..63 from 0..255
    b >>= 3;
    //pack rgb565
    return static_cast<WORD>((r << 11) bitor (g << 5) bitor b); //ARGB
}


// Drawing with Palettes

// This is for src size <= dest and same width
void IMAGE_RSC::Blit8BitFast(WORD *dest)
{
    unsigned char *sptr;
    WORD *Palette;
    WORD *dptr;
    long count;

    Palette = (WORD *)(Owner->Data_ + Header->paletteoffset);
    sptr = (unsigned char *)(Owner->Data_ + Header->imageoffset);
    dptr = dest;
    count = Header->w * Header->h;
#if 0

    while (count--)
        *dptr++ = Palette[*sptr++];

#else
#undef xor
	__asm
    {
        mov ESI, sptr
        mov EDI, dptr
        mov EBX, Palette
        mov ECX, count
        xor EDX, EDX
    };
    loop_here:
    _asm
    {
        xor EAX, EAX
        lodsb
        mov EDX, EAX
        ADD EDX, EDX
        mov AX,  [EBX+EDX]
        stosw
        loop  loop_here
    };
#define xor ^
#endif
}

// This is for src size <= dest and same width
void IMAGE_RSC::Blit8BitTransparentFast(WORD *dest)
{
    unsigned char *sptr;
    WORD *Palette;
    WORD *dptr;
    long count;

    Palette = (WORD *)(Owner->Data_ + Header->paletteoffset);
    sptr = (unsigned char *)(Owner->Data_ + Header->imageoffset);
    dptr = dest;
    count = Header->w * Header->h;

    while (count--)
    {
        if (*sptr)
            *dptr++ = Palette[*sptr++];
        else
        {
            dptr++;
            sptr++;
        }
    }
}

// This is for Entire source -> dest with different width
void IMAGE_RSC::Blit8Bit(long doffset, long dwidth, WORD *dest)
{
    uchar *sptr, *srcsize;
    WORD *Palette;
    WORD *dptr;
    long i;
    long dadd;

    Palette = (WORD *)(Owner->Data_ + Header->paletteoffset);
    sptr   = (uchar *)(Owner->Data_ + Header->imageoffset);
    srcsize = (uchar *)(sptr  + Header->w * Header->h);
    dptr = dest + doffset;

    dadd = dwidth - Header->w;

    while (sptr < srcsize)
    {
        i = Header->w;

        while (i--)
            *dptr++ = Palette[*sptr++];

        dptr += dadd;
    }
}

// This is for Entire source -> dest with different width... ignore color 0 (ColorKey)
void IMAGE_RSC::Blit8BitTransparent(long doffset, long dwidth, WORD *dest)
{
    uchar *sptr, *srcsize;
    WORD *Palette;
    WORD *dptr;
    long i;
    long dadd;

    Palette = (WORD *)(Owner->Data_ + Header->paletteoffset);
    sptr   = (uchar *)(Owner->Data_ + Header->imageoffset);
    srcsize = (uchar *)(sptr  + Header->w * Header->h);
    dptr = dest + doffset;

    dadd = dwidth - Header->w;

    while (sptr < srcsize)
    {
        i = Header->w;

        while (i--)
        {
            if (*sptr)
                *dptr++ = Palette[*sptr++];
            else
            {
                sptr++;
                dptr++;
            }
        }

        dptr += dadd;
    }
}

// This is Partial Src -> dest
void IMAGE_RSC::Blit8BitPart(long soffset, long scopy, long ssize, long doffset, long dwidth, WORD *dest)
{
    uchar *sptr, *srcsize;
    WORD *Palette;
    WORD *dptr;
    long i;
    long sadd, dadd;

    Palette = (WORD *)(Owner->Data_ + Header->paletteoffset);
    sptr   = (uchar *)(Owner->Data_ + Header->imageoffset + soffset);
    srcsize = (uchar *)(Owner->Data_ + Header->imageoffset + ssize);
    dptr = dest + doffset;

    sadd = Header->w - scopy;
    dadd = dwidth - scopy;

    while (sptr < srcsize)
    {
        i = scopy;

        while (i--)
            *dptr++ = Palette[*sptr++];

        sptr += sadd;
        dptr += dadd;
    }
}

// This is Partial Src -> dest... ignore color 0 (ColorKey)
void IMAGE_RSC::Blit8BitTransparentPart(long soffset, long scopy, long ssize, long doffset, long dwidth, WORD *dest)
{
    uchar *sptr, *srcsize;
    WORD *Palette;
    WORD *dptr;
    long i;
    long sadd, dadd;

    Palette = (WORD *)(Owner->Data_ + Header->paletteoffset);
    sptr   = (uchar *)(Owner->Data_ + Header->imageoffset + soffset);
    srcsize = (uchar *)(Owner->Data_ + Header->imageoffset + ssize);
    dptr = dest + doffset;

    sadd = Header->w - scopy;
    dadd = dwidth - scopy;

    while (sptr < srcsize)
    {
        i = scopy;

        while (i--)
        {
            if (*sptr)
                *dptr++ = Palette[*sptr++];
            else
            {
                sptr++;
                dptr++;
            }
        }

        sptr += sadd;
        dptr += dadd;
    }
}

//-------------------------------------------------------------------------------------------------


// This is for Entire source -> dest with different width
void IMAGE_RSC::_Blit8BitTo32(long doffset, long dwidth, DWORD *dest)
{
    uchar *sptr, *srcsize;
    WORD *Palette;
    DWORD *dptr;
    long i;
    long dadd;
    WORD sc;
    DWORD dc;

    Palette = (WORD *)(Owner->Data_ + Header->paletteoffset);
    sptr   = (uchar *)(Owner->Data_ + Header->imageoffset);
    srcsize = (uchar *)(sptr  + Header->w * Header->h);
    dptr = dest + doffset;

    dadd = dwidth - Header->w;

    while (sptr < srcsize)
    {
        i = Header->w;

        while (i--)
        {
            sc = Palette[ *sptr++ ];

            dc = RGB565toRGB8(sc);

            *dptr++ = dc;
        }

        dptr += dadd;
    }
}

// This is for Entire source -> dest with different width... ignore color 0 (ColorKey)
void IMAGE_RSC::_Blit8BitTransparentTo32(long doffset, long dwidth, DWORD *dest)
{
    uchar *sptr, *srcsize;
    WORD *Palette;
    DWORD *dptr;
    long i;
    long dadd;
    WORD sc;
    DWORD dc;


    Palette = (WORD *)(Owner->Data_ + Header->paletteoffset);
    sptr   = (uchar *)(Owner->Data_ + Header->imageoffset);
    srcsize = (uchar *)(sptr  + Header->w * Header->h);
    dptr = dest + doffset;

    dadd = dwidth - Header->w;

    while (sptr < srcsize)
    {
        i = Header->w;

        while (i--)
        {
            if (*sptr)
            {
                //*dptr++=Palette[*sptr++];
                sc = Palette[*sptr++];
                dc = RGB565toRGB8(sc);
                *dptr++ = dc;
            }
            else
            {
                sptr++;
                dptr++;
            }
        }

        dptr += dadd;
    }
}

// This is Partial Src -> dest
void IMAGE_RSC::_Blit8BitPartTo32(long soffset, long scopy, long ssize, long doffset, long dwidth, DWORD *dest)
{
    uchar *sptr, *srcsize;
    WORD *Palette;
    DWORD *dptr;
    long i;
    long sadd, dadd;
    WORD sc;
    DWORD dc;

    Palette = (WORD *)(Owner->Data_ + Header->paletteoffset);
    sptr   = (uchar *)(Owner->Data_ + Header->imageoffset + soffset);
    srcsize = (uchar *)(Owner->Data_ + Header->imageoffset + ssize);
    dptr = dest + doffset;

    sadd = Header->w - scopy;
    dadd = dwidth - scopy;

    while (sptr < srcsize)
    {
        i = scopy;

        while (i--)
        {
            // *dptr++=Palette[*sptr++];
            sc = Palette[*sptr++];
            dc = RGB565toRGB8(sc);
            *dptr++ = dc;
        }

        sptr += sadd;
        dptr += dadd;
    }
}

// This is Partial Src -> dest... ignore color 0 (ColorKey)
void IMAGE_RSC::_Blit8BitTransparentPartTo32(long soffset, long scopy, long ssize, long doffset, long dwidth, DWORD *dest)
{
    uchar *sptr, *srcsize;
    WORD *Palette;
    DWORD *dptr;
    long i;
    long sadd, dadd;
    WORD sc;
    DWORD dc;

    Palette = (WORD *)(Owner->Data_ + Header->paletteoffset);
    sptr   = (uchar *)(Owner->Data_ + Header->imageoffset + soffset);
    srcsize = (uchar *)(Owner->Data_ + Header->imageoffset + ssize);
    dptr = dest + doffset;

    sadd = Header->w - scopy;
    dadd = dwidth - scopy;

    while (sptr < srcsize)
    {
        i = scopy;

        while (i--)
        {
            if (*sptr)
            {
                //*dptr++=Palette[*sptr++];
                sc = Palette[*sptr++];
                dc = RGB565toRGB8(sc);
                *dptr++ = dc;
            }
            else
            {
                sptr++;
                dptr++;
            }
        }

        sptr += sadd;
        dptr += dadd;
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////


// Straight Bliting

// This is for src size <= dest and same width
void IMAGE_RSC::Blit16BitFast(WORD *dest)
{
    WORD *sptr;
    WORD *dptr;
    long count;

    sptr = (WORD *)(Owner->Data_ + Header->imageoffset);
    dptr = dest;
    count = Header->w * Header->h;
#if 0

    while (count--)
        *dptr++ = *sptr++;

#else
    __asm
    {
        mov ECX, count
        mov ESI, sptr
        mov EDI, dptr
        rep movsw
    };
#endif
}

// This is for src size <= dest and same width
void IMAGE_RSC::Blit16BitTransparentFast(WORD *dest)
{
    WORD *sptr;
    WORD *dptr;
    long count;

    sptr = (WORD *)(Owner->Data_ + Header->imageoffset);
    dptr = dest;
    count = Header->w * Header->h;
#if 0

    while (count--)
    {
        if (*sptr xor Owner->ColorKey_)
            *dptr++ = *sptr++;
        else
        {
            sptr++;
            dptr++;
        }
    }

#else
    WORD key = Owner->ColorKey_;
    __asm
    {
        mov ECX, count
        mov DX,  key
        mov ESI, sptr
        mov EDI, dptr
    };
    loop_here:
    __asm
    {
        lodsw
        cmp AX, DX
        je Skip_Byte
        stosw
        loop loop_here
        jmp  blit_done
    };
    Skip_Byte:
    __asm
    {
        add EDI, 2
        loop loop_here
    };
    blit_done:
#endif
    return;
}

// This is for Entire source -> dest with different width
void IMAGE_RSC::Blit16Bit(long doffset, long dwidth, WORD *dest)
{
    WORD *sptr, *srcsize;
    WORD *dptr;
    long i;
    long dadd;

    sptr   = (WORD *)(Owner->Data_ + Header->imageoffset);
    srcsize = (WORD *)(sptr  + Header->w * Header->h);
    dptr = dest + doffset;

    dadd = dwidth - Header->w;
#if 0

    while (sptr < srcsize)
    {
        i = Header->w;

        while (i--)
            *dptr++ = *sptr++;

        dptr += dadd;
    }

#else
    i = Header->w;
    __asm
    {
        mov EAX, i
        mov EDX, srcsize
        mov EBX, dadd
        add EBX, EBX
        mov ESI, sptr
        mov EDI, dptr
    };
    loop_here:
    __asm
    {
        mov ECX, EAX
        rep movsw

        add EDI, EBX
        cmp ESI, EDX
        jl loop_here
    };
#endif
}


//-------------------------------------------------------------------------------------------------



//XX This is for Entire source -> dest with different width
void IMAGE_RSC::_Blit16BitTo32(long doffset, long dwidth, DWORD *dest)
{
    WORD *sptr, *srcsize;
    DWORD *dptr;
    long i;
    long dadd;
    WORD sc;
    DWORD dc;

    sptr   = (WORD *)(Owner->Data_ + Header->imageoffset);
    srcsize = (WORD *)(sptr  + Header->w * Header->h);

    dptr = dest + doffset;

    dadd = dwidth - Header->w;

    while (sptr < srcsize)
    {
        i = Header->w;//width => row

        while (i--)
        {
            sc = *sptr++;
            dc = RGB565toRGB8(sc);
            *dptr++ = dc;
        }

        dptr += dadd;
    }
}

//XX This is for Entire source -> dest with different width... ignore ColorKey
void IMAGE_RSC::_Blit16BitTransparentTo32(long doffset, long dwidth, DWORD *dest)
{
    WORD *sptr, *srcsize;
    WORD Key = Owner->ColorKey_;
    long i;
    long dadd;
    DWORD *dptr;
    WORD sc;
    DWORD dc;

    sptr   = (WORD *)(Owner->Data_ + Header->imageoffset);
    srcsize = (WORD *)(sptr  + Header->w * Header->h);

    dptr = dest + doffset;

    dadd = dwidth - Header->w;

    while (sptr < srcsize)
    {
        i = Header->w;

        while (i--)
        {
            if (*sptr xor Owner->ColorKey_)
            {
                sc = *sptr++;
                dc = RGB565toRGB8(sc);
                //set
                *dptr++ = dc;
            }
            else
            {
                sptr++;
                dptr++;
            }
        }

        dptr += dadd;
    }
}

//XX
void IMAGE_RSC::_Blit16BitPartTo32(long soffset, long scopy, long ssize, long doffset, long dwidth, DWORD *dest)
{
    WORD *sptr, *srcsize;
    DWORD *dptr;
    long i;
    long sadd, dadd;
    WORD sc;
    DWORD dc;

    sptr   = (WORD *)(Owner->Data_ + Header->imageoffset) + soffset;
    srcsize = (WORD *)(Owner->Data_ + Header->imageoffset) + ssize;
    dptr = dest + doffset;

    sadd = Header->w - scopy;
    dadd = dwidth - scopy;

    while (sptr < srcsize)
    {
        i = scopy;

        while (i--)
        {
            sc = *sptr++;
            dc = RGB565toRGB8(sc);
            //set
            *dptr++ = dc;
        }

        sptr += sadd;
        dptr += dadd;
    }
}


// This is Partial Src -> dest... ignore ColorKey
void IMAGE_RSC::_Blit16BitTransparentPartTo32(long soffset, long scopy, long ssize, long doffset, long dwidth, DWORD *dest)
{
    WORD *sptr, *srcsize;
    DWORD *dptr;
    long i;
    long sadd, dadd;
    WORD sc;
    DWORD dc;

    sptr   = (WORD *)(Owner->Data_ + Header->imageoffset) + soffset;
    srcsize = (WORD *)(Owner->Data_ + Header->imageoffset) + ssize;
    dptr = dest + doffset;

    sadd = Header->w - scopy;
    dadd = dwidth - scopy;

    while (sptr < srcsize)
    {
        i = scopy;

        while (i--)
        {
            if (*sptr xor Owner->ColorKey_)
            {
                //*dptr++=*sptr++;
                sc = *sptr++;
                dc = RGB565toRGB8(sc);
                *dptr++ = dc;

            }
            else
            {
                sptr++;
                dptr++;
            }
        }

        sptr += sadd;
        dptr += dadd;
    }
}

//---------------------------------------------------------------------------------------

// This is for Entire source -> dest with different width... ignore ColorKey
void IMAGE_RSC::Blit16BitTransparent(long doffset, long dwidth, WORD *dest)
{
    WORD *sptr, *srcsize;
    WORD *dptr, Key = Owner->ColorKey_;
    long i;
    long dadd;

    sptr   = (WORD *)(Owner->Data_ + Header->imageoffset);
    srcsize = (WORD *)(sptr  + Header->w * Header->h);
    dptr = dest + doffset;

    dadd = dwidth - Header->w;
#if 0

    while (sptr < srcsize)
    {
        i = Header->w;

        while (i--)
        {
            if (*sptr xor Owner->ColorKey_)
                *dptr++ = *sptr++;
            else
            {
                sptr++;
                dptr++;
            }
        }

        dptr += dadd;
    }

#else
    i = Header->w;
    __asm
    {
        mov EDX, srcsize
        mov BX,  Key
        mov ESI, sptr
        mov EDI, dptr
    };
    start_blit:
    __asm
    {
        mov ECX, i
    };
    loop_here:
    __asm
    {
        lodsw
        cmp AX, BX
        je  Skip_Byte
        stosw
        loop loop_here

        add EDI, dadd
        add EDI, dadd
        cmp ESI, EDX
        jl start_blit
        jmp Blit_Done
    };
    Skip_Byte:
    __asm
    {
        add EDI, 2
        loop loop_here

        add EDI, dadd
        add EDI, dadd
        cmp ESI, EDX
        jl start_blit
    };
    Blit_Done:
#endif
    return;
}

// This is Partial Src -> dest
void IMAGE_RSC::Blit16BitPart(long soffset, long scopy, long ssize, long doffset, long dwidth, WORD *dest)
{
    WORD *sptr, *srcsize;
    WORD *dptr;
    long i;
    long sadd, dadd;

    sptr   = (WORD *)(Owner->Data_ + Header->imageoffset) + soffset;
    srcsize = (WORD *)(Owner->Data_ + Header->imageoffset) + ssize;
    dptr = dest + doffset;

    sadd = Header->w - scopy;
    dadd = dwidth - scopy;

    while (sptr < srcsize)
    {
        i = scopy;
#if 0

        while (i--)
            *dptr++ = *sptr++;

#else
        __asm
        {
            mov ECX, scopy
            mov ESI, sptr
            mov EDI, dptr
            rep movsw

            mov sptr, ESI
            mov dptr, EDI
        };
#endif
        sptr += sadd;
        dptr += dadd;
    }
}

// This is Partial Src -> dest... ignore ColorKey
void IMAGE_RSC::Blit16BitTransparentPart(long soffset, long scopy, long ssize, long doffset, long dwidth, WORD *dest)
{
    WORD *sptr, *srcsize;
    WORD *dptr;
    long i;
    long sadd, dadd;

    sptr   = (WORD *)(Owner->Data_ + Header->imageoffset) + soffset;
    srcsize = (WORD *)(Owner->Data_ + Header->imageoffset) + ssize;
    dptr = dest + doffset;

    sadd = Header->w - scopy;
    dadd = dwidth - scopy;

    while (sptr < srcsize)
    {
        i = scopy;

        while (i--)
        {
            if (*sptr xor Owner->ColorKey_)
                *dptr++ = *sptr++;
            else
            {
                sptr++;
                dptr++;
            }
        }

        sptr += sadd;
        dptr += dadd;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////


// Translucent Blending code

// This is for src size <= dest and same width
void IMAGE_RSC::Blend8BitFast(WORD *dest, long front, long back)
{
    unsigned char *sptr;
    WORD *Palette;
    WORD *dptr;
    long count;
    WORD r, g, b;
    long operc;

    operc = front + back;

    Palette = (WORD *)(Owner->Data_ + Header->paletteoffset);
    sptr = (unsigned char *)(Owner->Data_ + Header->imageoffset);
    dptr = dest;
    count = Header->w * Header->h;

    while (count--)
    {
        r = rShift[UIColorTable[operc][
                       UIColorTable[front][(Palette[*sptr] >> Owner->reds) bitand 0x1f] +
                       UIColorTable[back][(*dptr >> Owner->reds) bitand 0x1f]
                   ]];

        g = gShift[UIColorTable[operc][
                       UIColorTable[front][(Palette[*sptr] >> Owner->greens) bitand 0x1f] +
                       UIColorTable[back][(*dptr >> Owner->greens) bitand 0x1f]
                   ]];

        b = bShift[UIColorTable[operc][
                       UIColorTable[front][(Palette[*sptr] >> Owner->blues) bitand 0x1f] +
                       UIColorTable[back][(*dptr >> Owner->blues) bitand 0x1f]
                   ]];

        sptr++;
        *dptr++ = static_cast<WORD>(r bitor g bitor b);
    }
}

// This is for src size <= dest and same width
void IMAGE_RSC::Blend8BitTransparentFast(WORD *dest, long front, long back)
{
    unsigned char *sptr;
    WORD *Palette;
    WORD *dptr;
    long count;
    WORD r, g, b;
    long operc;

    operc = front + back;

    Palette = (WORD *)(Owner->Data_ + Header->paletteoffset);
    sptr = (unsigned char *)(Owner->Data_ + Header->imageoffset);
    dptr = dest;
    count = Header->w * Header->h;

    while (count--)
    {
        if (*sptr)
        {
            r = rShift[UIColorTable[operc][
                           UIColorTable[front][(Palette[*sptr] >> Owner->reds) bitand 0x1f] +
                           UIColorTable[back][(*dptr >> Owner->reds) bitand 0x1f]
                       ]];

            g = gShift[UIColorTable[operc][
                           UIColorTable[front][(Palette[*sptr] >> Owner->greens) bitand 0x1f] +
                           UIColorTable[back][(*dptr >> Owner->greens) bitand 0x1f]
                       ]];

            b = bShift[UIColorTable[operc][
                           UIColorTable[front][(Palette[*sptr] >> Owner->blues) bitand 0x1f] +
                           UIColorTable[back][(*dptr >> Owner->blues) bitand 0x1f]
                       ]];
            *dptr++ = static_cast<WORD>(r bitor g bitor b);
        }
        else
            dptr++;

        sptr++;
    }
}

// This is for Entire source -> dest with different width
/*
void IMAGE_RSC::Blend8Bit(long doffset,long dwidth,WORD *dest,long front,long back)
{
 uchar *sptr,*srcsize;
 WORD *Palette;
 WORD *dptr;
 WORD r,g,b;
 long operc;
 long i;
 long dadd;

 operc=front+back;

 Palette=(WORD *)(Owner->Data_ + Header->paletteoffset);
 sptr   =(uchar *)(Owner->Data_ + Header->imageoffset);
 srcsize=(uchar *)(sptr  + Header->w * Header->h);
 dptr=dest+doffset;

 dadd=dwidth - Header->w;
 while(sptr < srcsize)
 {
 i=Header->w;
 while(i--)
 {
 r=rShift[UIColorTable[operc][
 UIColorTable[front][(Palette[*sptr] >> Owner->reds) bitand 0x1f]+
 UIColorTable[back][(*dptr >> Owner->reds) bitand 0x1f]
 ]];

 g=gShift[UIColorTable[operc][
 UIColorTable[front][(Palette[*sptr] >> Owner->greens) bitand 0x1f]+
 UIColorTable[back][(*dptr >> Owner->greens) bitand 0x1f]
 ]];

 b=bShift[UIColorTable[operc][
 UIColorTable[front][(Palette[*sptr] >> Owner->blues) bitand 0x1f]+
 UIColorTable[back][(*dptr >> Owner->blues) bitand 0x1f]
 ]];

 sptr++;
 *dptr++=static_cast<WORD>(r|g|b);
 }
 dptr+=dadd;
 }
}
*/

//XX
void IMAGE_RSC::Blend8Bit(long doffset, long dwidth, WORD *dest, long front, long back, bool b32)
{
    uchar *sptr, *srcsize;
    WORD *Palette;
    WORD *dptr;
    DWORD r, g, b;
    long operc;
    long i;
    long dadd;
    DWORD* dptr2;

    operc = front + back;

    Palette = (WORD *)(Owner->Data_ + Header->paletteoffset);
    sptr   = (uchar *)(Owner->Data_ + Header->imageoffset);
    srcsize = (uchar *)(sptr  + Header->w * Header->h);

    //word dest
    dptr = dest + doffset;

    dadd = dwidth - Header->w;
    //dword
    dptr2 = ((DWORD*)dest) + doffset;

    while (sptr < srcsize)
    {
        i = Header->w;

        while (i--)
        {
            WORD dc;

            if ( not b32)
                dc = *dptr;
            else
                dc = RGB8toRGB565(*dptr2);

            r = rShift[UIColorTable[operc][
                           UIColorTable[front][(Palette[*sptr] >> Owner->reds) bitand 0x1f] +
                           UIColorTable[back][(dc >> Owner->reds) bitand 0x1f]
                       ]];

            g = gShift[UIColorTable[operc][
                           UIColorTable[front][(Palette[*sptr] >> Owner->greens) bitand 0x1f] +
                           UIColorTable[back][(dc >> Owner->greens) bitand 0x1f]
                       ]];

            b = bShift[UIColorTable[operc][
                           UIColorTable[front][(Palette[*sptr] >> Owner->blues) bitand 0x1f] +
                           UIColorTable[back][(dc >> Owner->blues) bitand 0x1f]
                       ]];

            sptr++;


            if ( not b32)
                *dptr = static_cast<WORD>(r bitor g bitor b); //565
            else
                *dptr2  = RGB565toRGB8(static_cast<WORD>(r bitor g bitor b));

            ++dptr;
            ++dptr2;
        }

        dptr += dadd;
        dptr2 += dadd;
    }
}



// This is for Entire source -> dest with different width... ignore color 0 (ColorKey)
void IMAGE_RSC::Blend8BitTransparent(long doffset, long dwidth, WORD *dest, long front, long back, bool b32)
{
    uchar *sptr, *srcsize;
    WORD *Palette;
    WORD *dptr;
    DWORD r, g, b;
    long operc;
    long i;
    long dadd;

    operc = front + back;

    Palette = (WORD *)(Owner->Data_ + Header->paletteoffset);
    sptr   = (uchar *)(Owner->Data_ + Header->imageoffset);
    srcsize = (uchar *)(sptr  + Header->w * Header->h);

    if ( not b32)
        dptr = dest + doffset;
    else
        dptr = dest + doffset * 2;

    dadd = dwidth - Header->w;

    if (b32)
        dadd *= 2;//word->dword

    while (sptr < srcsize)
    {
        i = Header->w;

        while (i--)
        {
            if (*sptr)
            {
                WORD dc;

                if ( not b32)
                    dc = *dptr;
                else
                    dc = RGB8toRGB565(*(reinterpret_cast<DWORD*>(dptr)));


                r = rShift[UIColorTable[operc][
                               UIColorTable[front][(Palette[*sptr] >> Owner->reds) bitand 0x1f] +
                               UIColorTable[back][(dc >> Owner->reds) bitand 0x1f]
                           ]];

                g = gShift[UIColorTable[operc][
                               UIColorTable[front][(Palette[*sptr] >> Owner->greens) bitand 0x1f] +
                               UIColorTable[back][(dc >> Owner->greens) bitand 0x1f]
                           ]];

                b = bShift[UIColorTable[operc][
                               UIColorTable[front][(Palette[*sptr] >> Owner->blues) bitand 0x1f] +
                               UIColorTable[back][(dc >> Owner->blues) bitand 0x1f]
                           ]];

                sptr++;

                if ( not b32)
                    *dptr++ = static_cast<WORD>(r bitor g bitor b);
                else
                {
                    DWORD dc = RGB565toRGB8(static_cast<WORD>(r bitor g bitor b));
                    *(reinterpret_cast<DWORD*>(dptr)) = dc;
                    dptr += 2;
                }
            }
            else
            {
                sptr++;

                dptr++;

                if (b32) dptr++;
            }
        }

        dptr += dadd;
    }
}

// This is Partial Src -> dest
void IMAGE_RSC::Blend8BitPart(long soffset, long scopy, long ssize, long doffset, long dwidth, WORD *dest, long front, long back, bool b32)
{
    uchar *sptr, *srcsize;
    WORD *Palette;
    WORD *dptr;
    DWORD r, g, b;
    long operc;
    long i;
    long sadd, dadd;

    operc = front + back;

    Palette = (WORD *)(Owner->Data_ + Header->paletteoffset);
    sptr   = (uchar *)(Owner->Data_ + Header->imageoffset + soffset);
    srcsize = (uchar *)(Owner->Data_ + Header->imageoffset + ssize);

    if ( not b32)
        dptr = dest + doffset;
    else
        dptr = dest + doffset * 2;

    sadd = Header->w - scopy;
    dadd = dwidth - scopy;

    if (b32)
        dadd *= 2;


    while (sptr < srcsize)
    {
        i = scopy;

        while (i--)
        {
            WORD dc;

            if ( not b32)
                dc = *dptr;
            else
                dc = RGB8toRGB565(*(reinterpret_cast<DWORD*>(dptr)));


            r = rShift[UIColorTable[operc][
                           UIColorTable[front][(Palette[*sptr] >> Owner->reds) bitand 0x1f] +
                           UIColorTable[back][(dc >> Owner->reds) bitand 0x1f]
                       ]];

            g = gShift[UIColorTable[operc][
                           UIColorTable[front][(Palette[*sptr] >> Owner->greens) bitand 0x1f] +
                           UIColorTable[back][(dc >> Owner->greens) bitand 0x1f]
                       ]];

            b = bShift[UIColorTable[operc][
                           UIColorTable[front][(Palette[*sptr] >> Owner->blues) bitand 0x1f] +
                           UIColorTable[back][(dc >> Owner->blues) bitand 0x1f]
                       ]];

            sptr++;

            //XX *dptr++=static_cast<WORD>(r|g|b);
            if ( not b32)
                *dptr++ = static_cast<WORD>(r bitor g bitor b);
            else
            {
                DWORD dc = RGB565toRGB8(static_cast<WORD>(r bitor g bitor b));
                *(reinterpret_cast<DWORD*>(dptr)) = dc;
                *dptr += 2;
            }

        }

        sptr += sadd;
        dptr += dadd;
    }
}

// This is Partial Src -> dest... ignore color 0 (ColorKey)
void IMAGE_RSC::Blend8BitTransparentPart(long soffset, long scopy, long ssize, long doffset, long dwidth, WORD *dest, long front, long back, bool b32)
{
    uchar *sptr, *srcsize;
    WORD *Palette;
    WORD *dptr;
    DWORD r, g, b;
    long operc;
    long i;
    long sadd, dadd;

    operc = front + back;

    Palette = (WORD *)(Owner->Data_ + Header->paletteoffset);
    sptr   = (uchar *)(Owner->Data_ + Header->imageoffset + soffset);
    srcsize = (uchar *)(Owner->Data_ + Header->imageoffset + ssize);

    if ( not b32)
        dptr = dest + doffset;
    else
        dptr = dest + doffset * 2;

    sadd = Header->w - scopy;

    if ( not b32)
        dadd = dwidth - scopy;
    else
        dadd = (dwidth - scopy) * 2;


    while (sptr < srcsize)
    {
        i = scopy;

        while (i--)
        {
            if (*sptr)
            {
                WORD dc;

                if ( not b32)
                    dc = *dptr;
                else
                    dc = RGB8toRGB565(*(reinterpret_cast<DWORD*>(dptr)));


                r = rShift[UIColorTable[operc][
                               UIColorTable[front][(Palette[*sptr] >> Owner->reds) bitand 0x1f] +
                               UIColorTable[back][(dc >> Owner->reds) bitand 0x1f]
                           ]];

                g = gShift[UIColorTable[operc][
                               UIColorTable[front][(Palette[*sptr] >> Owner->greens) bitand 0x1f] +
                               UIColorTable[back][(dc >> Owner->greens) bitand 0x1f]
                           ]];

                b = bShift[UIColorTable[operc][
                               UIColorTable[front][(Palette[*sptr] >> Owner->blues) bitand 0x1f] +
                               UIColorTable[back][(dc >> Owner->blues) bitand 0x1f]
                           ]];

                sptr++;

                //XX *dptr++=static_cast<WORD>(r|g|b);
                if ( not b32)
                    *dptr++ = static_cast<WORD>(r bitor g bitor b);
                else
                {
                    DWORD dc = RGB565toRGB8(static_cast<WORD>(r bitor g bitor b));
                    *(reinterpret_cast<DWORD*>(dptr)) = dc;

                    *dptr += 2;
                }
            }
            else
            {
                sptr++;
                dptr++;

                if (b32) dptr++; //to dword
            }
        }

        sptr += sadd;
        dptr += dadd;
    }
}

// Straight Blending

// This is for src size <= dest and same width
void IMAGE_RSC::Blend16BitFast(WORD *dest, long front, long back)
{
    WORD *sptr;
    WORD *dptr;
    long count;
    WORD r, g, b;
    long operc;

    operc = front + back;

    sptr = (WORD *)(Owner->Data_ + Header->imageoffset);
    dptr = dest;
    count = Header->w * Header->h;

    while (count--)
    {
        WORD dc;
        dc = *dptr;

        r = rShift[UIColorTable[operc][
                       UIColorTable[front][(*sptr >> Owner->reds) bitand 0x1f] +
                       UIColorTable[back][(dc >> Owner->reds) bitand 0x1f]
                   ]];

        g = gShift[UIColorTable[operc][
                       UIColorTable[front][(*sptr >> Owner->greens) bitand 0x1f] +
                       UIColorTable[back][(dc >> Owner->greens) bitand 0x1f]
                   ]];

        b = bShift[UIColorTable[operc][
                       UIColorTable[front][(*sptr >> Owner->blues) bitand 0x1f] +
                       UIColorTable[back][(dc >> Owner->blues) bitand 0x1f]
                   ]];

        sptr++;
        *dptr++ = static_cast<WORD>(r bitor g bitor b);
    }
}

// This is for src size <= dest and same width
void IMAGE_RSC::Blend16BitTransparentFast(WORD *dest, long front, long back)
{
    WORD *sptr;
    WORD *dptr;
    long count;
    WORD r, g, b;
    long operc;

    operc = front + back;

    sptr = (WORD *)(Owner->Data_ + Header->imageoffset);
    dptr = dest;
    count = Header->w * Header->h;

    while (count--)
    {
        if (*sptr xor Owner->ColorKey_)
        {
            WORD dc;
            dc = *dptr;

            r = rShift[UIColorTable[operc][
                           UIColorTable[front][(*sptr >> Owner->reds) bitand 0x1f] +
                           UIColorTable[back][(dc >> Owner->reds) bitand 0x1f]
                       ]];

            g = gShift[UIColorTable[operc][
                           UIColorTable[front][(*sptr >> Owner->greens) bitand 0x1f] +
                           UIColorTable[back][(dc >> Owner->greens) bitand 0x1f]
                       ]];

            b = bShift[UIColorTable[operc][
                           UIColorTable[front][(*sptr >> Owner->blues) bitand 0x1f] +
                           UIColorTable[back][(dc >> Owner->blues) bitand 0x1f]
                       ]];
            *dptr++ = static_cast<WORD>(r bitor g bitor b);
        }
        else
            dptr++;

        sptr++;
    }
}

// This is for Entire source -> dest with different width
void IMAGE_RSC::Blend16Bit(long doffset, long dwidth, WORD *dest, long front, long back, bool b32)
{
    WORD *sptr, *srcsize;
    WORD *dptr;
    WORD r, g, b;
    long operc;
    long i;
    long dadd;

    operc = front + back;

    sptr   = (WORD *)(Owner->Data_ + Header->imageoffset);
    srcsize = (WORD *)(sptr  + Header->w * Header->h);

    if ( not b32)
        dptr = dest + doffset;
    else
        dptr = dest + doffset * 2;

    if ( not b32)
        dadd = dwidth - Header->w;
    else
        dadd = (dwidth - Header->w) * 2;

    while (sptr < srcsize)
    {
        i = Header->w;

        while (i--)
        {
            WORD dc;

            if ( not b32)
                dc = *dptr;
            else
                dc = RGB8toRGB565(*(reinterpret_cast<DWORD*>(dptr)));


            r = rShift[UIColorTable[operc][
                           UIColorTable[front][(*sptr >> Owner->reds) bitand 0x1f] +
                           UIColorTable[back][(dc >> Owner->reds) bitand 0x1f]
                       ]];

            g = gShift[UIColorTable[operc][
                           UIColorTable[front][(*sptr >> Owner->greens) bitand 0x1f] +
                           UIColorTable[back][(dc >> Owner->greens) bitand 0x1f]
                       ]];

            b = bShift[UIColorTable[operc][
                           UIColorTable[front][(*sptr >> Owner->blues) bitand 0x1f] +
                           UIColorTable[back][(dc >> Owner->blues) bitand 0x1f]
                       ]];

            sptr++;

            if ( not b32)
                *dptr++ = static_cast<WORD>(r bitor g bitor b);
            else
            {
                *(reinterpret_cast<DWORD*>(dptr)) = RGB565toRGB8(static_cast<WORD>(r bitor g bitor b));

                *dptr += 2;
            }
        }

        dptr += dadd;
    }
}

// This is for Entire source -> dest with different width... ignore ColorKey
void IMAGE_RSC::Blend16BitTransparent(long doffset, long dwidth, WORD *dest, long front, long back, bool b32)
{
    WORD *sptr, *srcsize;
    WORD *dptr;
    WORD r, g, b;
    long operc;
    long i;
    long dadd;

    operc = front + back;

    sptr   = (WORD *)(Owner->Data_ + Header->imageoffset);
    srcsize = (WORD *)(sptr  + Header->w * Header->h);

    if ( not b32)
        dptr = dest + doffset;
    else
        dptr = dest + doffset * 2;

    if ( not b32)
        dadd = dwidth - Header->w;
    else
        dadd = (dwidth - Header->w) * 2;

    while (sptr < srcsize)
    {
        i = Header->w;

        while (i--)
        {
            if (*sptr xor Owner->ColorKey_)
            {
                WORD dc;

                if ( not b32)
                    dc = *dptr;
                else
                    dc = RGB8toRGB565(*(reinterpret_cast<DWORD*>(dptr)));

                r = rShift[UIColorTable[operc][
                               UIColorTable[front][(*sptr >> Owner->reds) bitand 0x1f] +
                               UIColorTable[back][(dc >> Owner->reds) bitand 0x1f]
                           ]];

                g = gShift[UIColorTable[operc][
                               UIColorTable[front][(*sptr >> Owner->greens) bitand 0x1f] +
                               UIColorTable[back][(dc >> Owner->greens) bitand 0x1f]
                           ]];

                b = bShift[UIColorTable[operc][
                               UIColorTable[front][(*sptr >> Owner->blues) bitand 0x1f] +
                               UIColorTable[back][(dc >> Owner->blues) bitand 0x1f]
                           ]];

                sptr++;

                if ( not b32)
                    *dptr++ = static_cast<WORD>(r bitor g bitor b);
                else
                {
                    *(reinterpret_cast<DWORD*>(dptr)) = RGB565toRGB8(static_cast<WORD>(r bitor g bitor b));
                    *dptr += 2;
                }
            }
            else
            {
                sptr++;

                if ( not b32)
                    dptr++;
                else
                    dptr += 2;
            }
        }

        //add calc in begin dep WORD|DWORD
        dptr += dadd;
    }
}

// This is Partial Src -> dest
void IMAGE_RSC::Blend16BitPart(long soffset, long scopy, long ssize, long doffset, long dwidth, WORD *dest, long front, long back, bool b32)
{
    WORD *sptr, *srcsize;
    WORD *dptr;
    WORD r, g, b;
    long operc;
    long i;
    long sadd, dadd;

    operc = front + back;

    sptr   = (WORD *)(Owner->Data_ + Header->imageoffset) + soffset;
    srcsize = (WORD *)(Owner->Data_ + Header->imageoffset) + ssize;

    if ( not b32)
        dptr = dest + doffset;
    else
        dptr = dest + doffset * 2;

    sadd = Header->w - scopy;

    if ( not b32)
        dadd = dwidth - scopy;
    else
        dadd = (dwidth - scopy) * 2;

    while (sptr < srcsize)
    {
        i = scopy;

        while (i--)
        {
            WORD dc;

            if ( not b32)
                dc = *dptr;
            else
                dc = RGB8toRGB565(*(reinterpret_cast<DWORD*>(dptr)));

            r = rShift[UIColorTable[operc][
                           UIColorTable[front][(*sptr >> Owner->reds) bitand 0x1f] +
                           UIColorTable[back][(dc >> Owner->reds) bitand 0x1f]
                       ]];

            g = gShift[UIColorTable[operc][
                           UIColorTable[front][(*sptr >> Owner->greens) bitand 0x1f] +
                           UIColorTable[back][(dc >> Owner->greens) bitand 0x1f]
                       ]];

            b = bShift[UIColorTable[operc][
                           UIColorTable[front][(*sptr >> Owner->blues) bitand 0x1f] +
                           UIColorTable[back][(dc >> Owner->blues) bitand 0x1f]
                       ]];

            sptr++;

            if ( not b32)
                *dptr++ = static_cast<WORD>(r bitor g bitor b);
            else
            {
                *(reinterpret_cast<DWORD*>(dptr)) = RGB565toRGB8(static_cast<WORD>(r bitor g bitor b));
                *dptr += 2;
            }

        }

        sptr += sadd;
        dptr += dadd;//dadd corrected in begin depending WORD,DWORD dest mem
    }
}

// This is Partial Src -> dest... ignore ColorKey
void IMAGE_RSC::Blend16BitTransparentPart(long soffset, long scopy, long ssize, long doffset, long dwidth, WORD *dest, long front, long back, bool b32)
{
    WORD *sptr, *srcsize;
    WORD *dptr;
    WORD r, g, b;
    long operc;
    long i;
    long sadd, dadd;

    operc = front + back;

    sptr   = (WORD *)(Owner->Data_ + Header->imageoffset) + soffset;
    srcsize = (WORD *)(Owner->Data_ + Header->imageoffset) + ssize;

    if ( not b32)
        dptr = dest + doffset;
    else
        dptr = dest + doffset * 2;

    sadd = Header->w - scopy;

    if ( not b32)
        dadd = dwidth - scopy;
    else
        dadd = (dwidth - scopy) * 2;


    while (sptr < srcsize)
    {
        i = scopy;

        while (i--)
        {
            if (*sptr xor Owner->ColorKey_)
            {
                WORD dc;

                if ( not b32)
                    dc = *dptr;
                else
                    dc = RGB8toRGB565(*(reinterpret_cast<DWORD*>(dptr)));

                r = rShift[UIColorTable[operc][
                               UIColorTable[front][(*sptr >> Owner->reds) bitand 0x1f] +
                               UIColorTable[back][(dc >> Owner->reds) bitand 0x1f]
                           ]];

                g = gShift[UIColorTable[operc][
                               UIColorTable[front][(*sptr >> Owner->greens) bitand 0x1f] +
                               UIColorTable[back][(dc >> Owner->greens) bitand 0x1f]
                           ]];

                b = bShift[UIColorTable[operc][
                               UIColorTable[front][(*sptr >> Owner->blues) bitand 0x1f] +
                               UIColorTable[back][(dc >> Owner->blues) bitand 0x1f]
                           ]];

                sptr++;


                //XX *dptr++=static_cast<WORD>(r|g|b);
                if ( not b32)
                    *dptr++ = static_cast<WORD>(r bitor g bitor b);
                else
                {
                    *(reinterpret_cast<DWORD*>(dptr)) = RGB565toRGB8(static_cast<WORD>(r bitor g bitor b));
                    *dptr += 2;
                }
            }
            else
            {
                sptr++;

                if ( not b32)
                    dptr++;
                else
                    dptr += 2;
            }
        }

        sptr += sadd;
        dptr += dadd;
    }
}

char *IMAGE_RSC::GetImage()
{
    if ( not Owner) return(NULL);

    if ( not Owner->Data_)
        return(NULL);

    if (Header->Type == _RSC_IS_IMAGE_)
        return(Owner->Data_ + Header->imageoffset);

    return(NULL);
}

WORD *IMAGE_RSC::GetPalette()
{
    if ( not Owner) return(NULL);

    if ( not Owner->Data_)
        return(NULL);

    if (Header->Type == _RSC_IS_IMAGE_)
        if (Header->flags bitand _RSC_8_BIT_)
            return((WORD*)(Owner->Data_ + Header->paletteoffset));

    return(NULL);
}

// VITAL NOTE:
//  it is VERY important that all the source bitand destination parameters are
//  within the DESTINATION
//  source checking is provided...
//  DESTINATION is NOT checked whatsoever
//
//  I personally am calling this from within the UI95 code where all clipping has already occured
//
void IMAGE_RSC::Blit(SCREEN *surface, long sx, long sy, long sw, long sh, long dx, long dy)
{
    long soff, doff, ssize;

    if ( not Owner)
        return;

    if ( not Owner->Data_)
        return;

    if (sx >= Header->w or sy >= Header->h)
        return;

    if ( not sx and not sy and sw >= Header->w and sh >= Header->h)
    {
        if (Header->flags bitand _RSC_USECOLORKEY_)
        {
            if (Header->flags bitand _RSC_8_BIT_)
            {
                //XX
                //if(Header->w == surface->width)
                // Blit8BitTransparentFast(surface->mem);
                //else
                if (surface->bpp == 32) //XX
                    _Blit8BitTransparentTo32(dy * surface->width + dx, surface->width, (DWORD*)surface->mem);
                else
                    Blit8BitTransparent(dy * surface->width + dx, surface->width, surface->mem);
            }
            else
            {
                //XX
                //if(Header->w == surface->width)
                // Blit16BitTransparentFast(surface->mem);
                //else
                if (surface->bpp == 32) //XX
                    _Blit16BitTransparentTo32(dy * surface->width + dx, surface->width, (DWORD*)surface->mem);
                else
                    Blit16BitTransparent(dy * surface->width + dx, surface->width, surface->mem);
            }
        }
        else
        {
            if (Header->flags bitand _RSC_8_BIT_)
            {
                //XX
                // if(0)
                //// if(Header->w == surface->width)
                // Blit8BitFast(surface->mem);
                // else
                if (surface->bpp == 32) //XX
                    _Blit8BitTo32(dy * surface->width + dx, surface->width, (DWORD*)surface->mem);
                else
                    Blit8Bit(dy * surface->width + dx, surface->width, surface->mem);
            }
            else
            {
                //XX
                //if(Header->w == surface->width)
                // Blit16BitFast(surface->mem);
                //else
                {
                    if (surface->bpp == 32) //XX
                        _Blit16BitTo32(dy * surface->width + dx, surface->width, (DWORD*)surface->mem);//XX
                    else
                        Blit16Bit(dy * surface->width + dx, surface->width, surface->mem);
                }
            }
        }

        return;
    }

    if ((sx + sw) > Header->w)
        sw = Header->w - sx;

    if ((sy + sh) > Header->h)
        sw = Header->h - sy;

    if (sw < 1 or sh < 1)
        return;

    soff = sy * Header->w + sx;
    doff = dy * surface->width + dx;
    ssize = Header->w * (sy + sh);

    if (Header->flags bitand _RSC_USECOLORKEY_)
    {
        if (Header->flags bitand _RSC_8_BIT_)
        {
            if (surface->bpp == 32)   //XX
                _Blit8BitTransparentPartTo32(soff, sw, ssize, dy * surface->width + dx, surface->width, (DWORD*)surface->mem);
            else
                Blit8BitTransparentPart(soff, sw, ssize, dy * surface->width + dx, surface->width, surface->mem);
        }
        else
        {
            if (surface->bpp == 32)   //XX
                _Blit16BitTransparentPartTo32(soff, sw, ssize, dy * surface->width + dx, surface->width, (DWORD*) surface->mem);
            else
                Blit16BitTransparentPart(soff, sw, ssize, dy * surface->width + dx, surface->width, surface->mem);
        }
    }
    else
    {
        if (Header->flags bitand _RSC_8_BIT_)
        {
            if (surface->bpp == 32)  //XX
                _Blit8BitPartTo32(soff, sw, ssize, dy * surface->width + dx, surface->width, (DWORD*) surface->mem);
            else
                Blit8BitPart(soff, sw, ssize, dy * surface->width + dx, surface->width, surface->mem);
        }
        else
        {
            if (surface->bpp == 32)  //XX
                _Blit16BitPartTo32(soff, sw, ssize, dy * surface->width + dx, surface->width, (DWORD*)surface->mem);
            else
                Blit16BitPart(soff, sw, ssize, dy * surface->width + dx, surface->width, surface->mem);
        }
    }
}

// VITAL NOTE:
//  it is VERY important that all the source bitand destination parameters are
//  within the DESTINATION
//  source checking is provided...
//  DESTINATION is NOT checked whatsoever
//
//  I personally am calling this from within the UI95 code where all clipping has already occured
//
void IMAGE_RSC::Blend(SCREEN *surface, long sx, long sy, long sw, long sh, long dx, long dy, long front, long back)
{
    long soff, doff, ssize;

    if ( not Owner->Data_)
        return;

    if (sx >= Header->w or sy >= Header->h)
        return;

    if ( not sx and not sy and sw >= Header->w and sh >= Header->h)
    {
        if (Header->flags bitand _RSC_USECOLORKEY_)
        {
            if (Header->flags bitand _RSC_8_BIT_)
            {
                //if(Header->w == surface->width)
                // Blend8BitTransparentFast(surface->mem,front,back);
                //else
                Blend8BitTransparent(dy * surface->width + dx, surface->width, surface->mem, front, back, surface->bpp == 32); //XX
            }
            else
            {
                //if(Header->w == surface->width)
                // Blend16BitTransparentFast(surface->mem,front,back);
                //else
                //XX
                Blend16BitTransparent(dy * surface->width + dx, surface->width, surface->mem, front, back, surface->bpp == 32);
            }
        }
        else
        {
            if (Header->flags bitand _RSC_8_BIT_)
            {
                //if(Header->w == surface->width)
                // Blend8BitFast(surface->mem,front,back);
                //else
                Blend8Bit(dy * surface->width + dx, surface->width, surface->mem, front, back, surface->bpp == 32); //XX
            }
            else
            {
                //if(Header->w == surface->width)
                // Blend16BitFast(surface->mem,front,back);
                //else
                //XX
                Blend16Bit(dy * surface->width + dx, surface->width, surface->mem, front, back, surface->bpp == 32); //XX
            }
        }

        return;
    }

    if ((sx + sw) > Header->w)
        sw = Header->w - sx;

    if ((sy + sh) > Header->h)
        sw = Header->h - sy;

    if (sw < 1 or sh < 1)
        return;

    soff = sy * Header->w + sx;
    doff = dy * surface->width + dx;
    ssize = Header->w * (sy + sh);

    if (Header->flags bitand _RSC_USECOLORKEY_)
    {
        if (Header->flags bitand _RSC_8_BIT_)
            Blend8BitTransparentPart(soff, sw, ssize, dy * surface->width + dx, surface->width, surface->mem, front, back, surface->bpp == 32); //XX
        else
            Blend16BitTransparentPart(soff, sw, ssize, dy * surface->width + dx, surface->width, surface->mem, front, back, surface->bpp == 32); //XX
    }
    else
    {
        if (Header->flags bitand _RSC_8_BIT_)
            Blend8BitPart(soff, sw, ssize, dy * surface->width + dx, surface->width, surface->mem, front, back, surface->bpp == 32); //XX
        else
            Blend16BitPart(soff, sw, ssize, dy * surface->width + dx, surface->width, surface->mem, front, back, surface->bpp == 32); //XX
    }
}

// NOTE: Although ScaleUp and ScaleDown have the same parameter lists... they are VERY different,
//      and won't work interchangably
// Shrink a Bitmap
void IMAGE_RSC::ScaleDown8(SCREEN *surface, long *Rows, long *Cols, long dx, long dy, long dw, long dh, long offx, long offy)
{
    unsigned char *sptr, *sline;
    WORD *dptr, *dline;
    WORD *Palette;
    int i, j, count;

    if ( not Owner)
        return;

    if ( not Owner->Data_)
        return;

    sptr = (unsigned char *)(Owner->Data_ + Header->imageoffset);
    Palette = (WORD*)(Owner->Data_ + Header->paletteoffset);

    dptr = surface->mem;

    dline = &dptr[(dy * surface->width) + dx];

    if (surface->bpp == 32) //XX w->dw
        dline += (dy * surface->width) + dx; //*2;

    for (i = offy; i < (offy + dh); i++)
    {
        sline = &sptr[Rows[i] * Header->w];
        count = 0;

        for (j = offx; j < (offx + dw); j++)
        {
            if (surface->bpp == 32)//XX
                ((DWORD*)dline)[count++] = RGB565toRGB8(Palette[ sline[Cols[j]] ]);
            else
                dline[count++] = Palette[sline[Cols[j]]];
        }

        dline += surface->width;

        if (surface->bpp == 32) //XX w->dw
            dline += surface->width;

    }
}

// Kludge for Threat circle overlays
void IMAGE_RSC::ScaleDown8Overlay(SCREEN *surface, long *Rows, long *Cols, long dx, long dy, long dw, long dh, long offx, long offy, BYTE *overlay, WORD *Palette[])
{
    unsigned char *sptr, *sline, *oline;
    WORD *dptr, *dline;
    int i, j, count;
    long palno, overidx;

    sptr = (unsigned char *)(Owner->Data_ + Header->imageoffset);

    dptr = surface->mem;
    dline = &dptr[(dy * surface->width) + dx];

    if (surface->bpp == 32)//XX
        dline += (dy * surface->width) + dx; //*=2;// w->dw

    overidx = (dy * surface->width) + dx;

    for (i = offy; i < (offy + dh); i++)
    {
        sline = &sptr[Rows[i] * Header->w];
        oline = &overlay[Rows[i] * Header->w];
        count = 0;

        for (j = offx; j < (offx + dw); j++)
        {
            palno = (long)(oline[Cols[j]] bitand 0x0f);

            if (surface->bpp == 32)
            {
                //XX
                ((DWORD*)dline)[count++] = RGB565toRGB8(Palette[palno][sline[Cols[j]]]);
            }
            else
                dline[count++] = Palette[palno][sline[Cols[j]]];
        }

        dline += surface->width;

        if (surface->bpp == 32) //XX
            dline += surface->width; //w -> dw
    }
}

// NOTE: Although ScaleUp and ScaleDown have the similar parameter lists... the meanings are VERY different,
//      and won't work interchangably
// Grow a Bitmap
void IMAGE_RSC::ScaleUp8(SCREEN *surface, long *Rows, long *Cols, long dx, long dy, long dw, long dh)
{
    unsigned char *sptr, *sline;
    WORD *dptr, *dline;
    WORD *Palette;
    WORD *cpyline;
    long i, j, rval, count;
    DWORD* cpyline2;

    sptr = (unsigned char *)(Owner->Data_ + Header->imageoffset);
    Palette = (WORD*)(Owner->Data_ + Header->paletteoffset);

    dptr = surface->mem;
    cpyline = new WORD[dw];
    cpyline2 = new DWORD[dw];//XX

    rval = -1;
    dline = &dptr[(dy * surface->width) + dx];

    if (surface->bpp == 32) //XX
        dline += (dy * surface->width) + dx; //*2;//w->dw offset

    for (i = 0; i < dh; i++)
    {
        sline = &sptr[Rows[i] * Header->w];

        if (surface->bpp == 32) //XX
        {
            if (Rows[i] == rval)
            {
                __asm
                {
                    mov ECX, count
                    mov ESI, cpyline2
                    mov EDI, dline
                    rep movsd
                };
            }
            else if (Rows[i] not_eq Rows[i + 1])
            {
                for (j = 0; j < dw; j++)
                    ((DWORD*)dline)[j] = RGB565toRGB8(Palette[sline[Cols[j]]]);
            }
            else
            {
                count = 0;

                for (j = 0; j < dw; j++)
                    cpyline2[count++] = RGB565toRGB8(Palette[sline[Cols[j + 1]]]);

                rval = Rows[i];
                __asm
                {
                    mov ECX, count
                    mov ESI, cpyline2
                    mov EDI, dline
                    rep movsd
                };
            }

            dline += surface->width * 2;
        }
        else
        {
            //16bits
            if (Rows[i] == rval)
            {
                __asm
                {
                    mov ECX, count
                    mov ESI, cpyline
                    mov EDI, dline
                    rep movsw
                };
                // memcpy(&dline[first],cpyline,count*sizeof(WORD));
            }
            else if (Rows[i] not_eq Rows[i + 1])
            {
                for (j = 0; j < dw; j++)
                    dline[j] = Palette[sline[Cols[j]]];
            }
            else
            {
                count = 0;

                for (j = 0; j < dw; j++)
                    cpyline[count++] = Palette[sline[Cols[j + 1]]];

                rval = Rows[i];
                __asm
                {
                    mov ECX, count
                    mov ESI, cpyline
                    mov EDI, dline
                    rep movsw
                };
                // memcpy(&dline[first],cpyline,count*sizeof(WORD));
            }

            dline += surface->width;
        }
    }

    delete cpyline;
    delete cpyline2;
}

void IMAGE_RSC::ScaleUp8Overlay(SCREEN *surface, long *Rows, long *Cols, long dx, long dy, long dw, long dh, BYTE *overlay, WORD *Palette[])
{
    unsigned char *sptr, *sline, *oline;
    WORD *dptr, *dline;
    WORD *cpyline;
    long i, j, rval, count;
    long palno;
    DWORD* cpyline2;

    sptr = (unsigned char *)(Owner->Data_ + Header->imageoffset);

    dptr = surface->mem;
    cpyline = new WORD[dw];
    cpyline2 = new DWORD[dw];//XX

    rval = -1;
    dline = &dptr[(dy * surface->width) + dx];

    if (surface->bpp == 32) //XX
        dline += (dy * surface->width) + dx; //*2;

    for (i = 0; i < dh; i++)
    {
        sline = &sptr[Rows[i] * Header->w];
        oline = &overlay[Rows[i] * Header->w];

        if (surface->bpp == 32)
        {
            if (Rows[i] == rval)
            {
                __asm
                {
                    mov ECX, count
                    mov ESI, cpyline2
                    mov EDI, dline
                    rep movsd
                };
            }
            else if (Rows[i] not_eq Rows[i + 1])
            {
                for (j = 0; j < dw; j++)
                {
                    palno = (long)(oline[Cols[j]] bitand 0x0f);
                    ((DWORD*)dline)[j] = RGB565toRGB8(Palette[palno][sline[Cols[j]]]);
                }
            }
            else
            {
                count = 0;

                for (j = 0; j < dw; j++)
                {
                    palno = (long)(oline[Cols[j + 1]] bitand 0x0f);
                    cpyline2[count++] = RGB565toRGB8(Palette[palno][sline[Cols[j + 1]]]);
                }

                rval = Rows[i];
                __asm
                {
                    mov ECX, count
                    mov ESI, cpyline2
                    mov EDI, dline
                    rep movsd
                };

            }

            dline += surface->width * 2;
        }
        else
        {
            //16bits
            if (Rows[i] == rval)
            {
                __asm
                {
                    mov ECX, count
                    mov ESI, cpyline
                    mov EDI, dline
                    rep movsw
                };
                // memcpy(&dline[first],cpyline,count*sizeof(WORD));
            }
            else if (Rows[i] not_eq Rows[i + 1])
            {
                for (j = 0; j < dw; j++)
                {
                    palno = (long)(oline[Cols[j]] bitand 0x0f);
                    dline[j] = Palette[palno][sline[Cols[j]]];
                }
            }
            else
            {
                count = 0;

                for (j = 0; j < dw; j++)
                {
                    palno = (long)(oline[Cols[j + 1]] bitand 0x0f);
                    cpyline[count++] = Palette[palno][sline[Cols[j + 1]]];
                }

                rval = Rows[i];
                __asm
                {
                    mov ECX, count
                    mov ESI, cpyline
                    mov EDI, dline
                    rep movsw
                };
                // memcpy(&dline[first],cpyline,count*sizeof(WORD));
            }

            dline += surface->width;
        }
    }

    delete cpyline;
    delete cpyline2;
}

