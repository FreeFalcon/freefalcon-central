/*
+---------------------------------------------------------------------------+
|    image.c                                                                |
+---------------------------------------------------------------------------+
|    Description:  This file contains functions to convert gif, pcx, lbm    |
|                  file format to raw data.                                 |
+---------------------------------------------------------------------------+
|    Created by Erick Jap                               January 12, 1994    |
+---------------------------------------------------------------------------+
  Copyright (c) 1994  Spectrum Holobyte, Inc.
*/

//___________________________________________________________________________

#include <cISO646>
#include "image.h"
#include <ddraw.h> //JAM 22Sep03


//___________________________________________________________________________


// mode = COLOR_32K --> convert to 32K color
//      = COLOR_64K --> convert to 64K color
//      = COLOR_16M --> convert to 16M color
// return ptr to new bmp if successful
GLubyte *ConvertImage(GLImageInfo *fi, GLint mode, GLuint *chromakey)
{
    GLuint i, j, r, g, b, a;
    GLuint totalsize, sizebmp = 1, rgbval[256];
    GLubyte *bmpptr;
    GLubyte *ptr;
    GLushort *sptr;
    GLulong *lptr;

    switch (mode)
    {
        default:
        case COLOR_256:
            return (0);

        case COLOR_32K:
        case COLOR_64K:
            sizebmp = 2;
            break;

        case COLOR_16M:
            sizebmp = 4;
            break;
    }

    totalsize = fi -> width * fi -> height;
    sizebmp *= totalsize;
    bmpptr = (unsigned char *) glAllocateMemory(sizebmp, 0);

    if ( not bmpptr) return (0);

    if (fi -> palette)   // bitmap has palette
    {
        lptr = fi -> palette;

        for (i = 0; i < 256; i++)
        {
            r = (GLint)(*lptr++);
            g = (r >>  8) bitand 0xFF;
            b = (r >> 16) bitand 0xFF;
            a = (r >> 24) bitand 0xFF;
            r and_eq 0xFF;
            j = 0;

            switch (mode)
            {
                case COLOR_32K: // 555 16 bit color
                    j = ((r >> 3) << 10) + ((g >> 3) << 5) + (b >> 3);
                    break;

                case COLOR_64K: // 565 16 bit color
                    j = ((r >> 3) << 11) + ((g >> 2) << 5) + (b >> 3);
                    break;

                case COLOR_16M: // RGBA 32 bit color
                    j = (a << 24) bitor (b << 16) bitor (g << 8) bitor r;
                    break;
            }

            rgbval[i] = j;
        }

        ptr = (unsigned char *) fi -> image;

        switch (mode)
        {
            case COLOR_32K:
            case COLOR_64K:
                sptr = (GLushort *) bmpptr;

                while (totalsize--)
                {
                    *sptr++ = (GLushort) rgbval[*ptr++];
                }

                break;

            case COLOR_16M:
                lptr = (GLulong *) bmpptr;

                while (totalsize--)
                {
                    *lptr++ = (GLuint) rgbval[*ptr++];
                }

                break;
        }

        if (chromakey)
        {
            *chromakey = rgbval[0];
        }
    }
    else
    {
        ptr = (unsigned char *) fi -> image;

        switch (mode)
        {
            case COLOR_32K:
                sptr = (GLushort *) bmpptr;

                while (totalsize--)
                {
                    b = *ptr++;
                    g = *ptr++;
                    r = *ptr++;
                    j = ((r >> 3) << 10) + ((g >> 3) << 5) + (b >> 3);
                    *sptr++ = (GLushort) j;
                }

                break;

            case COLOR_64K:
                sptr = (GLushort *) bmpptr;

                while (totalsize--)
                {
                    b = *ptr++;
                    g = *ptr++;
                    r = *ptr++;
                    j = ((r >> 3) << 11) + ((g >> 2) << 5) + (b >> 3);
                    *sptr++ = (GLushort) j;
                }

                break;

            case COLOR_16M:
                lptr = (GLulong *) bmpptr;

                while (totalsize--)
                {
                    b = *ptr++;
                    g = *ptr++;
                    r = *ptr++;
                    j = (b << 16) + (g << 8) + r + 0xff000000;
                    *lptr++ = (GLuint) j;
                }

                break;
        }

        if (chromakey)
        {
            *chromakey = 0xff000000;
        }
    }

    return ((GLubyte *) bmpptr);
}

// Only 24 bit, no colormap and uncompressed TGA file supported
GLint ReadTGA(CImageFileMemory *fi)
{
    TGA_HEADER tgaheader;

    fi -> glReadMem(&tgaheader, sizeof(tgaheader));

    if (tgaheader.imagetype not_eq 0x2) return BAD_FORMAT;

    fi -> glSetFilePosMem((GLuint) tgaheader.identsize, SEEK_CUR);

    if (tgaheader.colormaptype) return BAD_FORMAT;

    if (tgaheader.bits not_eq 24) return BAD_FORMAT;

    GLint i = tgaheader.height * tgaheader.width * 3;
    fi -> image.width = tgaheader.width;
    fi -> image.height = tgaheader.height;
    fi -> image.palette = NULL;
    fi -> image.image = (GLubyte *) glAllocateMemory(i , 0);

    if ( not fi -> image.image) return BAD_ALLOC;

    fi -> glReadMem(fi -> image.image, i);
    return GOOD_READ;
}

//JAM 22Sep03
GLint ReadDDS(CImageFileMemory *fi)
{
    DDSURFACEDESC2 ddsd;
    DWORD dwMagic;

    fi->glReadMem(&dwMagic, sizeof(DWORD));

    if (dwMagic not_eq MAKEFOURCC('D', 'D', 'S', ' ')) return BAD_FORMAT;

    if ( not fi->glReadMem(&ddsd, sizeof(DDSURFACEDESC2))) return BAD_FORMAT;

    // MLR 1/25/2004 - Little kludge so FF can read DDS files made by dxtex
    if (ddsd.dwLinearSize == 0)
    {
        if (ddsd.ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', 'T', '3') or
            ddsd.ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', 'T', '5'))
        {
            ddsd.dwLinearSize = ddsd.dwWidth * ddsd.dwWidth;
            ddsd.dwFlags or_eq DDSD_LINEARSIZE;
        }

        if (ddsd.ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', 'T', '1'))
        {
            ddsd.dwLinearSize = ddsd.dwWidth * ddsd.dwWidth / 2;
            ddsd.dwFlags or_eq DDSD_LINEARSIZE;
        }
    }


    fi->image.width = ddsd.dwWidth;
    fi->image.height = ddsd.dwHeight;
    fi->image.ddsd = ddsd;
    fi->image.palette = NULL;

    // Read first compressed mipmap
    ShiAssert(ddsd.dwFlags bitand DDSD_LINEARSIZE);

    fi->image.image = (GLubyte *)glAllocateMemory(ddsd.dwLinearSize, 0);

    if ( not fi->image.image) return BAD_ALLOC;

    fi->glReadMem(fi->image.image, ddsd.dwLinearSize);

    return GOOD_READ;
}
//JAM

// Read an 8 bit or 24 bit BMP file
GLint ReadBMP(CImageFileMemory *fi)
{
    BMP_HEADER bmpheader;
    BMP_INFO bmpinfo;
    BMP_RGBQUAD bmprgb;
    GLubyte *ptr, paddings[4];
    GLulong *palptr;
    GLint i, j, padBytes;

    fi -> glReadMem(&bmpheader, sizeof(bmpheader));
    fi -> glReadMem(&bmpinfo, sizeof(bmpinfo));

    if (bmpinfo.biCompression) return (BAD_COMPRESSION);

    if (bmpinfo.biBitCount not_eq 8 and bmpinfo.biBitCount not_eq 24)
        return (BAD_COLORDEPTH);

    i = bmpinfo.biClrUsed;

    if (bmpinfo.biBitCount == 24)
    {
        while (i--) fi -> glReadMem(&bmprgb, sizeof(bmprgb));

        fi -> image.palette = NULL;
    }
    else
    {
        if ( not i) i = 256;

        palptr = (GLulong *) glAllocateMemory(1024);

        if ( not palptr) return (BAD_ALLOC);

        fi -> image.palette = palptr;

        for (j = 0; j < i; j++)
        {
            fi -> glReadMem(&bmprgb, sizeof(bmprgb));
            *palptr++ = (bmprgb.rgbBlue << 16) bitor (bmprgb.rgbGreen << 8) bitor bmprgb.rgbRed;
        }
    }

    fi -> image.width = bmpinfo.biWidth;
    fi -> image.height = bmpinfo.biHeight;

    padBytes = bmpinfo.biWidth bitand 0x3;

    j = bmpinfo.biBitCount >> 3;
    i = j * (fi -> image.width * fi -> image.height);
    fi -> image.image = (GLubyte *) glAllocateMemory(i, 0);

    if ( not fi -> image.image) return (BAD_ALLOC);

    ptr = (unsigned char *) fi -> image.image + i;
    i = j * fi -> image.width;
    j = fi -> image.height;

    while (j--)
    {
        ptr -= i;
        fi -> glReadMem(ptr, i);

        if (padBytes) fi -> glReadMem(&paddings, padBytes);
    }

    return (GOOD_READ);
}


/*
+---------------------------------------------------------------------------+
|    ReadAPL                                                                |
+---------------------------------------------------------------------------+
|    Description:  Read a custom file format used to store alpha in each |
|                  palette entry.                                           |
|                                                                           |
|    Parameters:   fi   = output structure                                  |
+---------------------------------------------------------------------------+
|    Programmed by Scott Randolph                           June 9, 1997    |
+---------------------------------------------------------------------------+
*/
GLint ReadAPL(CImageFileMemory *fi)
{
    APL_HEADER aplheader;
    int imageSize;

    // Read and validate the image header
    fi -> glReadMem(&aplheader, sizeof(aplheader));

    if (aplheader.magic not_eq 0x030870)
    {
        return BAD_FORMAT;
    }

    fi -> image.width = aplheader.width;
    fi -> image.height = aplheader.height;
    imageSize = aplheader.width * aplheader.height;

    // Allocate memory for the image and its palette
    fi -> image.palette = (GLulong*)glAllocateMemory(1024, 0);

    if ( not fi -> image.palette)
    {
        return (BAD_ALLOC);
    }

    fi -> image.image = (GLubyte *)glAllocateMemory(imageSize, 0);

    if ( not fi -> image.image)
    {
        glReleaseMemory(fi -> image.palette);
        return (BAD_ALLOC);
    }

    // Read the palette then the image data
    if ((fi -> glReadMem(fi -> image.palette, 1024) not_eq 1024)       or
        (fi -> glReadMem(fi -> image.image,   imageSize) not_eq imageSize))
    {
        glReleaseMemory(fi -> image.palette);
        glReleaseMemory(fi -> image.palette);
        return BAD_READ;
    }

    return (GOOD_READ);
}


/*
+---------------------------------------------------------------------------+
|    UnpackGIF                                                              |
+---------------------------------------------------------------------------+
|    Description:  unpack GIF image                                         |
|                                                                           |
|    Parameters:   fi   = output structure                                  |
+---------------------------------------------------------------------------+
|    Programmed by Erick Jap                            January 12, 1994    |
+---------------------------------------------------------------------------+
*/
GLint UnpackGIF(CImageFileMemory *fi)
{
    GIFHEADER       gh;
    IMAGEBLOCK      iblk;
    GLint           t, b, c;
    GLulong *palOut, *palStop;
    GLubyte *palIn;
    GLubyte tempPalette[768];

    // make sure it's a GIF file
    if (fi -> glReadMem((GLubyte *)&gh, sizeof(gh)) not_eq sizeof(gh) or memcmp(gh.sig, "GIF", 3))
    {
        return(BAD_FILE);
    }

    // get screen dimensions
    fi -> image.width = gh.screenwidth;
    fi -> image.height = gh.screenheight;
    fi -> image.palette = NULL;

    // get colour map if there is one
    if (gh.flags bitand 0x80)
    {
        c = 3 * (1 << ((gh.flags bitand 7) + 1));

        if (fi -> glReadMem(tempPalette, c) not_eq c)
        {
            return(BAD_READ);
        }

        palOut = (GLulong *) glAllocateMemory(1024);

        if ( not palOut)
        {
            return BAD_ALLOC;
        }

        palIn = tempPalette;
        fi->image.palette = palOut;
        palStop = palOut + 256;

        while (palOut < palStop)
        {
            *palOut++ = 0xFF000000 bitor (palIn[2] << 16) bitor (palIn[1] << 8) bitor palIn[0];
            palIn += 3;
        }
    }

    while ((c = fi -> glReadCharMem()) == ',' or c == '!' or c == 0)
    {
        if (c == ',')
        {
            if (fi -> glReadMem(&iblk, sizeof(iblk)) not_eq sizeof(iblk))
            {
                glReleaseMemory((char *) fi -> image.palette);
                return(BAD_READ);
            }

            fi -> image.width = iblk.width;
            fi -> image.height = iblk.height;
            fi -> image.image = (GLubyte *) glAllocateMemory(fi->image.width * fi->image.height, 0);

            if ( not fi -> image.image)
            {
                glReleaseMemory((char *) fi->image.palette);
                return (BAD_ALLOC);
            }

            if (iblk.flags bitand 0x80)
            {
                b = 3 * (1 << ((iblk.flags bitand 0x0007) + 1));

                if (fi->glReadMem(tempPalette, b) not_eq c)
                {
                    glReleaseMemory((char *) fi->image.palette);
                    glReleaseMemory((char *) fi->image.image);
                    return(BAD_READ);
                }

                glReleaseMemory((char *) fi->image.palette);
                palOut = (GLulong *) glAllocateMemory(1024);

                if ( not palOut)
                {
                    return BAD_ALLOC;
                }

                palIn = tempPalette;
                fi->image.palette = palOut;
                palStop = palOut + 256;

                while (palOut < palStop)
                {
                    *palOut = 0xFF000000 bitor (palIn[2] << 16) bitor (palIn[1] << 8) bitor palIn[0];
                    palIn += 3;
                }
            }

            if ((c = fi->glReadCharMem()) == EOF)
            {
                glReleaseMemory((char *) fi->image.palette);
                glReleaseMemory((char *) fi->image.image);
                return(BAD_FILE);
            }

            t = GIF_UnpackImage(c, fi, iblk.flags);

            if (t not_eq GOOD_READ)
            {
                glReleaseMemory((char *) fi->image.palette);
                glReleaseMemory((char *) fi->image.image);
                return(t);              /* quit if there was an error */
            }
        }
        else if (c == '!') GIF_SkipExtension(fi);
    }

    return(GOOD_READ);
}       /* UnpackGIF */

/*
+---------------------------------------------------------------------------+
|    GIF_UnpackImage                                                        |
+---------------------------------------------------------------------------+
|    Description:  unpack an LZW compressed image                           |
|                                                                           |
|    Parameters:   bits = code                                              |
|                  fi   = output structure                                  |
+---------------------------------------------------------------------------+
|    Programmed by Erick Jap                            January 12, 1994    |
+---------------------------------------------------------------------------+
*/
GLint GIF_UnpackImage(GLint bits, CImageFileMemory *fi, GLint currentFlag)
{
    GLbyte  linebuffer[4096];
    GLubyte firstcodestack[4096]; /* Stack for first codes */
    GLubyte lastcodestack[4096];   /* Stack for previous code */
    GLuint  codestack[4096];      /* Stack for links */
    GLuint  wordmasktable[] =
    {
        0x0000, 0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f,
        0x00ff, 0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff
    };
    GLuint  inctable[] = { 8, 8, 4, 2, 0 };
    GLuint  startable[] = { 0, 4, 2, 1, 0 };

    GLuint offset;
    GLuint  bits2; /* Bits plus 1 */
    GLuint  codesize;       /* Current code size in bits */
    GLuint  codesize2;      /* Next codesize */
    GLuint  nextcode;       /* Next available table entry */
    GLuint  thiscode;       /* Code being expanded */
    GLuint  oldtoken;       /* Last symbol decoded */
    GLuint  currentcode;    /* Code just read */
    GLuint  oldcode;        /* Code read before this one */
    GLuint  bitsleft;       /* Number of bits left in *p */
    GLuint  blocksize;      /* Bytes in next block */
    GLuint  line = 0;         /* next line to write */
    GLuint  byte = 0;         /* next byte to write */
    GLuint  pass = 0;         /* pass number for interlaced pictures */
    GLubyte  *p;             /* Pointer to current byte in read buffer */
    GLubyte  *q;             /* Pointer past last byte in read buffer */
    GLubyte  b[256];         /* Read buffer */
    GLubyte  *u;             /* Stack pointer into firstcodestack */
    GLubyte  *buffer;        /* Pointer to image buffer */

    if (bits < 2 or bits > 8) return(BAD_SYMBOLSIZE);

    p = q = b;
    bitsleft = 8;
    bits2 = 1 << bits;
    nextcode = bits2 + 2;
    codesize = bits + 1;
    codesize2 = 1 << codesize;
    oldcode = oldtoken = (GLuint) NO_CODE;
    buffer = (GLubyte *) fi->image.image;

    for (;;)
    {
        if (bitsleft == 8)
        {
            if (++p >= q and (((blocksize = fi->glReadCharMem()) < 1) or
                             (q = (p = b) + fi->glReadMem(b, blocksize)) < (b + blocksize)))
                return(UNEXPECTED_EOF);

            bitsleft = 0;
        }

        thiscode = *p;

        if ((currentcode = (codesize + bitsleft)) <= 8)
        {
            *p >>= codesize;
            bitsleft = currentcode;
        }
        else
        {
            if (++p >= q and (((blocksize = fi->glReadCharMem()) < 1) or
                             (q = (p = b) + fi->glReadMem(b, blocksize)) < (b + blocksize)))
                return(UNEXPECTED_EOF);

            thiscode or_eq (*p << (8 - bitsleft));

            if (currentcode <= 16)
            {
                bitsleft = currentcode - 8;
                *p >>= bitsleft;
            }
            else
            {
                if (++p >= q and (((blocksize = fi->glReadCharMem()) < 1) or
                                 (q = (p = b) + fi->glReadMem(b, blocksize)) < (b + blocksize)))
                    return(UNEXPECTED_EOF);

                thiscode or_eq (*p << (16 - bitsleft));
                bitsleft = currentcode - 16;
                *p >>= bitsleft;
            }
        }

        thiscode and_eq wordmasktable[codesize];
        currentcode = thiscode;

        if (thiscode == (bits2 + 1)) break;     /* found EOI */

        if (thiscode > nextcode) return(BAD_CODE);

        if (thiscode == bits2)
        {
            nextcode = bits2 + 2;
            codesize = bits + 1;
            codesize2 = 1 << codesize;
            oldtoken = oldcode = (GLuint) NO_CODE;
            continue;
        }

        u = firstcodestack;

        if (thiscode == nextcode)
        {
            if (oldcode == NO_CODE) return(BAD_FIRSTCODE);

            *u++ = (GLubyte) oldtoken;
            thiscode = oldcode;
        }

        while (thiscode >= bits2)
        {
            *u++ = lastcodestack[thiscode];
            thiscode = codestack[thiscode];
        }

        oldtoken = thiscode;
#if 0

        do
        {
            linebuffer[byte++] = (GLubyte) thiscode;

            if (byte >= (GLuint) fi->image.width)
            {
                if (line < (GLuint) fi -> image.height)
                {
                    offset = (GLint) line * fi -> image.width;
                    memcpy(buffer + offset, linebuffer, fi -> image.width);
                }

                byte = 0;

                if (currentFlag bitand 0x40)
                {
                    line += inctable[pass];

                    if (line >= (GLuint) fi->image.height) line = startable[++pass];
                }
                else ++line;
            }

            if (u <= firstcodestack)
            {
                break;
            }

            thiscode = *--u;
        }
        while (1);

#endif

        while (u >= firstcodestack)
        {
            linebuffer[byte++] = (GLubyte) thiscode;

            if (byte >= (GLuint) fi->image.width)
            {
                if (line < (GLuint) fi -> image.height)
                {
                    offset = (GLint) line * fi -> image.width;
                    memcpy(buffer + offset, linebuffer, fi -> image.width);
                }

                byte = 0;

                if (currentFlag bitand 0x40)
                {
                    line += inctable[pass];

                    if (line >= (GLuint) fi->image.height) line = startable[++pass];
                }
                else
                    ++line;
            }

            thiscode = *--u;
        };

        if (nextcode < 4096 and oldcode not_eq NO_CODE)
        {
            codestack[nextcode] = oldcode;
            lastcodestack[nextcode] = (GLubyte) oldtoken;

            if (++nextcode >= codesize2 and codesize < 12)
                codesize2 = 1 << ++codesize;
        }

        oldcode = currentcode;
    }

    return(GOOD_READ);
}       /* GIF_UnpackImage */

/*
+---------------------------------------------------------------------------+
|    GIF_SkipExtension                                                      |
+---------------------------------------------------------------------------+
|    Description:  This function is called when the GIF decoder encounters  |
|                  an extension.                                            |
+---------------------------------------------------------------------------+
|    Programmed by Erick Jap                            January 12, 1994    |
+---------------------------------------------------------------------------+
*/
void GIF_SkipExtension(CImageFileMemory *fi)
{
    PLAINTEXT       pt;
    CONTROLBLOCK    cb;
    APPLICATION     ap;
    GLuint c, n, i;

    switch ((c = fi->glReadCharMem()))
    {
        case 0x0001:            /* plain text descriptor */
            if (fi->glReadMem((GLubyte *)&pt, sizeof(pt)) == sizeof(pt))
            {
                do
                {
                    if ((n = fi->glReadCharMem()) not_eq EOF)
                    {
                        for (i = 0; i < n; ++i) fi->glReadCharMem();
                    }
                }
                while (n > 0 and n not_eq EOF);
            }

            break;

        case 0x00f9:            /* graphic control block */
            fi->glReadMem((GLubyte *)&cb, sizeof(cb));
            break;

        case 0x00fe:            /* comment extension */
            do
            {
                if ((n = fi->glReadCharMem()) not_eq EOF)
                {
                    for (i = 0; i < n; ++i) fi->glReadCharMem();
                }
            }
            while (n > 0 and n not_eq EOF);

            break;

        case 0x00ff:            /* application extension */
            if (fi->glReadMem((GLubyte *)&ap, sizeof(ap)) == sizeof(ap))
            {
                do
                {
                    if ((n = fi->glReadCharMem()) not_eq EOF)
                    {
                        for (i = 0; i < n; ++i) fi->glReadCharMem();
                    }
                }
                while (n > 0 and n not_eq EOF);
            }

            break;

        default:                /* something else */
            n = fi->glReadCharMem();

            for (i = 0; i < n; ++i) fi->glReadCharMem();

            break;
    }
}       /* GIF_SkipExtension */

/*
+---------------------------------------------------------------------------+
|    UnpackLBM                                                              |
+---------------------------------------------------------------------------+
|    Description:  unpack LBM image                                         |
|                                                                           |
|    Parameters:   fi   = output structure                                  |
+---------------------------------------------------------------------------+
|    Programmed by Erick Jap                            January 12, 1994    |
+---------------------------------------------------------------------------+
*/
GLint UnpackLBM(CImageFileMemory *fi)
{
    GLbyte header [12];
    GLint size;
    LBM_BMHD *lpHeader;
    GLint  doIFF;

    fi->glSetFilePosMem(0, SEEK_SET);
    fi->glReadMem((GLubyte *) header, 12);

    if (memcmp((GLubyte *) &header[8], (GLubyte *) "PBM ", 4)) doIFF = 0;
    else doIFF = 1;

    fi->glReadMem((GLubyte *) header, 4);

    if (memcmp((GLubyte *) header, (GLubyte *) "BMHD", 4)) return NULL;

    fi->glReadMem((GLubyte *) &size, 4);
    size = motr2intl(size);
    lpHeader = (LBM_BMHD *) glAllocateMemory(size, 0);

    if (lpHeader == NULL) return (BAD_READ);

    fi->glReadMem(lpHeader, size);
    fi->image.width = (GLshort) motr2inti(lpHeader->width);
    fi->image.height = (GLshort) motr2inti(lpHeader->height);

    GLulong *lpPalette;
    lpPalette = ReadLBMColorMap(fi);

    if (lpPalette == NULL)
    {
        glReleaseMemory((char *) lpHeader);
        return (BAD_READ);
    }

    fi->image.palette = lpPalette;

    GLubyte *lpBitmap;
    lpBitmap = ReadLBMBody(fi, lpHeader, doIFF);

    if (lpBitmap == NULL)
    {
        glReleaseMemory((char *) lpHeader);
        glReleaseMemory((char *) lpPalette);
        return (BAD_READ);
    }

    fi->image.image = lpBitmap;
    glReleaseMemory((char *) lpHeader);
    return (GOOD_READ);
}       /* UnpackLBM */

GLulong *ReadLBMColorMap(CImageFileMemory *fi)
{
    GLint CMAP;
    GLint  size;
    GLubyte header [4];
    GLubyte LBMPalette[768];
    GLubyte *palIn;
    GLulong *finalPalette;
    GLulong *palOut, *palStop;

    CMAP = 0;
    fi->glSetFilePosMem(12L, SEEK_SET);   // Jump to the first Chunk

    do
    {
        fi->glReadMem(header, 4);
        fi->glReadMem((GLubyte *) &size, 4);
        size = motr2intl(size);

        if (memcmp(header, (GLubyte *) "CMAP", 4))
        {
            if (size bitand 1)   size++;                 // All offsets on an even boundary

            fi->glSetFilePosMem(size, SEEK_CUR);
        }
        else CMAP = 1;
    }
    while ( not CMAP);

    fi->glReadMem(LBMPalette, size);
    finalPalette = (GLulong *)glAllocateMemory(1024);

    if ( not finalPalette)
    {
        return NULL;
    }

    palOut = finalPalette;
    palStop = finalPalette + 256;
    palIn = LBMPalette;

    while (palOut < palStop)
    {
        *palOut++ = (palIn[2] << 16) bitor (palIn[1] << 8) bitor palIn[0];
        palIn += 3;
    }

    return finalPalette;
}

GLubyte *ReadLBMBody(CImageFileMemory *fi, LBM_BMHD *lpHeader, GLint doIFF)
{
    GLbyte  linebuffer[4096];
    GLint BODY;
    GLint size;
    GLint imageSize;
    GLint offset;
    GLint bytes, i;
    GLbyte header [4];
    GLbyte  *lpLine;
    GLubyte *lpBmp;
    GLubyte *LBMBody;

    BODY = 0;
    fi->glSetFilePosMem(12L, SEEK_SET);           // Jump to first Chunk

    do
    {
        fi->glReadMem((GLubyte *) header, 4);
        fi->glReadMem((GLubyte *) &size, 4);
        size = motr2intl(size);

        if (memcmp((GLubyte *) header, (GLubyte *)  "BODY", 4))
        {
            if (size bitand 1)   size++;                 // All offsets on an even boundary

            fi->glSetFilePosMem(size, SEEK_CUR);
        }
        else BODY = 1;
    }
    while ( not BODY);

    if (lpHeader->masking == 1)
        bytes = ((fi->image.width + 7) / 8) * (lpHeader->nPlanes + 1);
    else
        bytes = ((fi->image.width + 7) / 8) * lpHeader->nPlanes;

    //      Buffer to hold uncompressed data without mask info
    imageSize = ((fi->image.width + 7) / 8);

    if (lpHeader->nPlanes <= 4) imageSize *= lpHeader->nPlanes ;
    else if (lpHeader->nPlanes <= 8) imageSize <<= 3;

    offset = fi->image.width;
    offset = imageSize;             //      Set the offset to equal the number of bytes
    imageSize *= fi->image.height ;
    LBMBody = (GLubyte *) glAllocateMemory(imageSize, 0);
    lpLine = linebuffer;
    lpBmp = LBMBody;

    for (i = 0; i < fi->image.height; i++)
    {
        if (lpHeader->compression == 0)
            fi->glReadMem(lpLine, bytes);
        else
        {
            GLbyte  *p;
            GLint  c, j, n;
            p = lpLine;
            n = 0;

            do
            {
                c = fi->glReadCharMem() bitand 0xff;

                if (c bitand 0x80)
                {
                    if (c not_eq 0x80)
                    {
                        j = ((compl c) bitand 0xff) + 2;
                        c = fi->glReadCharMem();

                        while (j--) p[n++] = (GLubyte) c;
                    }
                }
                else
                {
                    j = (c bitand 0xff) + 1;

                    while (j--)
                        p[n++] = (GLubyte) fi->glReadCharMem();
                }
            }
            while (n < bytes);
        }

        if (doIFF)      // Old format, don't unleave
            memcpy(lpBmp, lpLine, (size_t) offset);
        else
        {
            GLint masktable[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
            GLint   bittable[8]  = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
            GLint  k, l, m;
            GLbyte *line;

            m = fi->image.width >> 3;

            if (fi->image.width bitand 0x07) m++;

            for (k = 0; k < fi->image.width; k++)
            {
                line = lpLine;
                lpBmp[k] = 0;

                for (l = 0; l < lpHeader->nPlanes; l++)
                {
                    if (line[k >> 3] bitand masktable[k bitand 0x0007])
                        lpBmp[k] or_eq bittable[l];

                    line += m;
                }
            }
        }

        lpBmp += offset;
    }

    return LBMBody;
}

/*
+---------------------------------------------------------------------------+
|    UnpackPCX                                                              |
+---------------------------------------------------------------------------+
|    Description:  unpack PCX image                                         |
|                                                                           |
|    Parameters:   fi   = output structure                                  |
+---------------------------------------------------------------------------+
|    Programmed by Erick Jap                            January 12, 1994    |
+---------------------------------------------------------------------------+
*/
GLint UnpackPCX(CImageFileMemory *fi)
{
    PCXHEAD pcx;
    GLint bytes;
    GLint i;
    GLint totalsize;
    GLubyte *image;
    GLubyte pcxPalette[768];
    GLubyte *palIn;
    GLulong *palOut, *palStop;

    if ((fi->glReadMem((GLubyte *)&pcx, sizeof(PCXHEAD)) not_eq sizeof(PCXHEAD)) or
        (pcx.manufacturer not_eq 10) or
        (pcx.version not_eq 5))
    {

        // The header data didn't match our expectations
        return (BAD_READ);
    }

    bytes = pcx.bytes_per_line;
    fi->image.width = pcx.xmax - pcx.xmin + 1;
    fi->image.height = pcx.ymax - pcx.ymin + 1;
    totalsize = (GLint) fi->image.width * (GLint) fi->image.height;

    // Allocate memory for the image data
    fi->image.image = (GLubyte *) glAllocateMemory(totalsize, 0);

    if ( not fi->image.image)
    {
        return (BAD_ALLOC);
    }

    // Read the image data
    image = fi->image.image;

    for (i = 0; i < fi->image.height; i++)
    {
        GLint n, j;
        GLubyte c;
        n = 0;

        do
        {
            c = (GLubyte) fi->glReadCharMem();

            if ((c bitand 0xc0) == 0xc0)
            {
                j = c bitand 0x3f;
                c = (GLubyte) fi->glReadCharMem();

                while (j--) image[n++] = c;
            }
            else image[n++] = c;
        }
        while (n < bytes);

        image += fi->image.width;
    }


    // Read the palette data
    if ((fi->glReadCharMem() not_eq 0xc) or ( not fi->glReadMem(pcxPalette, 768)))
    {
        glReleaseMemory((char *) fi->image.image);
        return (BAD_ALLOC);
    }

    // Allocate memory for the palette
    fi->image.palette = (GLulong *) glAllocateMemory(1024);

    if ( not fi->image.palette)
    {
        glReleaseMemory((char *) fi->image.image);
        return (BAD_ALLOC);
    }

    // Unpack the palette (24 bit to 32 bit)
    palOut = fi->image.palette;
    palStop = fi->image.palette + 256;
    palIn = pcxPalette;

    while (palOut < palStop)
    {
        *palOut++ = 0xFF000000 bitor (palIn[2] << 16) bitor (palIn[1] << 8) bitor palIn[0];
        palIn += 3;
    }


    return (GOOD_READ);
}       /* UnpackPCX */


/***************************************************************************\
 WritePCX
 Scott Randolph 6/1/98
\***************************************************************************/
GLint WritePCX(int fileHandle, GLImageInfo *image)
{
    int result;
    PCXHEAD pcxHeader;
    BYTE *RLLdata;
    int maxRLLdata;
    BYTE palette24[1 + 3 * 256];
    BYTE *outP;
    BYTE *inP;
    int i;
    int c;
    BYTE value;
    int run;

    // Write the PCX header
    memset(&pcxHeader, 0, sizeof(pcxHeader));
    pcxHeader.manufacturer = 10;
    pcxHeader.version = 5;
    pcxHeader.encoding = 1;
    pcxHeader.bits_per_pixel = 8;
    pcxHeader.xmax = (short)(image->width - 1);
    pcxHeader.ymax = (short)(image->height - 1);
    pcxHeader.hres = (short)image->width;
    pcxHeader.vres = (short)image->height;
    pcxHeader.colour_planes = 1;
    pcxHeader.bytes_per_line = (short)image->width;
    pcxHeader.palette_type = 1;
    result = write(fileHandle, &pcxHeader, sizeof(pcxHeader));
    ShiAssert(result == sizeof(pcxHeader));

    // Allocate space for the compressed image data
    maxRLLdata = image->width * image->height * 2;
    RLLdata = new BYTE[maxRLLdata];

    // Run length encode the image a line at a time then write it
    // Counts are written with top two bits set.  Image bytes are written
    // with the top two bits clear.  If an image byte has the top two bits set, it
    // must be preceeded by a count byte even if the count is 1.
    inP = image->image;
    outP = RLLdata;

    for (i = 0; i < image->height; i++)
    {

        run = 0;
        c = 0;

        ShiAssert(inP - image->image == i * image->width + c);

        while (c < image->width)
        {
            // Start a new run
            run = 1;
            value = *inP++;
            c++;

            ShiAssert(inP - image->image == i * image->width + c);

            // Add to the current run
            while ((value == *inP) and // Must have same value
                   (c < image->width) and // Must not cross row boundries
                   (run + 1 < 64))   // Run less than 64 bytes
            {
                inP++;
                run++;
                c++;
            }

            ShiAssert(inP - image->image == i * image->width + c);

            // Write out the run
            if ((run == 1) and ((value bitand 0xC0) not_eq 0xC0))
            {
                // Can just write the value byte
                *outP++ = value;
            }
            else
            {
                // Require a count byte
                *outP++ = (BYTE)(0xC0 bitor run);
                *outP++ = value;
            }

            ShiAssert(inP - image->image == i * image->width + c);

        }
    }

    ShiAssert(outP - RLLdata <= maxRLLdata);
    result = write(fileHandle, RLLdata, outP - RLLdata);
    ShiAssert(result == outP - RLLdata);

    // Free the compression buffer
    delete[] RLLdata;

    // Convert the palette to 24 bit and write it
    palette24[0] = 0xC; // Palette signature

    for (i = 0; i < 256; i++)
    {
        palette24[i * 3 + 1] = (BYTE)(image->palette[i]); // Red
        palette24[i * 3 + 2] = (BYTE)(image->palette[i] >> 8); // Green
        palette24[i * 3 + 3] = (BYTE)(image->palette[i] >> 16); // Blue
    }

    result = write(fileHandle, palette24, sizeof(palette24));
    ShiAssert(result == sizeof(palette24));

    return (GOOD_WRITE);
}


/***************************************************************************\
 WriteAPL
 Scott Randolph 6/1/98
\***************************************************************************/
GLint WriteAPL(int fileHandle, GLImageInfo *image)
{
    int result;
    static const UInt32 magic = 0x030870;

    // Write the magic number and the width and height to the file
    result = write(fileHandle, &magic, sizeof(magic));
    ShiAssert(result == sizeof(magic));
    result = write(fileHandle, &image->width, sizeof(image->width));
    ShiAssert(result == sizeof(image->width));
    result = write(fileHandle, &image->height, sizeof(image->height));
    ShiAssert(result == sizeof(image->height));

    // Write the palette data
    result = write(fileHandle, image->palette, 1024);
    ShiAssert(result == 1024);

    // Write the image data
    result = write(fileHandle, image->image, image->width * image->height);
    ShiAssert(result == image->width * image->height);

    return (GOOD_WRITE);
}
