#include <cISO646>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <conio.h>
#include <string.h>
#include <ctype.h>
#include "debuggr.h"


#define DTR                     0.01745329F

#ifdef NDEBUG
//  #define DISABLE_MONO_DISPLAY
#endif
//#define WRITE_FILE

#ifdef WRITE_FILE
static FILE* debugFile = NULL;
#endif

// OW - redirect text mode output to different targets
#define _TEXT_TGT_CONSOLE
//#define _TEXT_TGT_TRACE
//#define _TEXT_TGT_FILE

#if defined _TEXT_TGT_CONSOLE
HANDLE hStdoutDbg = NULL;
CHAR_INFO charinfo[80 * 25 * 2];
#elif defined _TEXT_TGT_FILE
HANDLE hFileDbg = NULL;
#endif

#define MAX_STYLE    1
#define CHAR_WIDTH 6
#define CONTROL_REG   0x3B8
#define CONFIG_REG    0x3BF
#define STATUS_REG    0x3BA
#define DATA_REG      0x3B5
#define ADDRESS_REG   0x3B4
#define MONO_TEXT     0xB0000

#define OUT_BYTE(a, b) _outp((a), (b))
#define OUT_WORD(a, b) _outpw ((a), (b))

static float DebugScreenWidth = 719.0F;
static float DebugScreenHeight = 347.0F;
static char *screen_buffer[2];
static int page = 0;
static unsigned char *MonoNewline(void);
static unsigned char monoPenattribute = 0x07, monoPenX = 0, monoPenY = 0;

static char mono_memory[80 * 25 * 2];
static char mono_buffer[80 * 25 * 2];

void WriteDebugPixel(int, int);

static unsigned char Comma[] =
{
    0x00, 0x00, 0x00, 0x00, 0x20, 0x60, 0x40, 0x00
};
static unsigned char Minus[] =
{
    0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00
};
static unsigned char Period[] =
{
    0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00
};
static unsigned char Slash[] =
{
    0x08, 0x10, 0x20, 0x40, 0x80, 0x00, 0x00, 0x00
};
static unsigned char Number0[] =
{
    0xe0, 0xa0, 0xa0, 0xa0, 0xe0, 0x00, 0x00, 0x00
};
static unsigned char Number1[] =
{
    0x40, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00
};
static unsigned char Number2[] =
{
    0xe0, 0x20, 0xe0, 0x80, 0xe0, 0x00, 0x00, 0x00
};
static unsigned char Number3[] =
{
    0xe0, 0x20, 0x60, 0x20, 0xe0, 0x00, 0x00, 0x00
};
static unsigned char Number4[] =
{
    0x80, 0xa0, 0xe0, 0x20, 0x20, 0x00, 0x00, 0x00
};
static unsigned char Number5[] =
{
    0xe0, 0x80, 0xe0, 0x20, 0xe0, 0x00, 0x00, 0x00
};
static unsigned char Number6[] =
{
    0x80, 0x80, 0xe0, 0xa0, 0xe0, 0x00, 0x00, 0x00
};
static unsigned char Number7[] =
{
    0xe0, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00
};
static unsigned char Number8[] =
{
    0xe0, 0xa0, 0xe0, 0xa0, 0xe0, 0x00, 0x00, 0x00
};
static unsigned char Number9[] =
{
    0xe0, 0xa0, 0xe0, 0x20, 0x20, 0x00, 0x00, 0x00
};
static unsigned char Colon[] =
{
    0x00, 0x20, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00
};
static unsigned char SemiColon[] =
{
    0x00, 0x20, 0x00, 0x20, 0x60, 0x40, 0x00, 0x00
};
static unsigned char Less[] =
{
    0x20, 0x40, 0x80, 0x40, 0x20, 0x00, 0x00, 0x00
};
static unsigned char Equal[] =
{
    0x00, 0x70, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00
};
static unsigned char More[] =
{
    0x80, 0x40, 0x20, 0x40, 0x80, 0x00, 0x00, 0x00
};
static unsigned char Quest[] =
{
    0xc0, 0x20, 0x60, 0x40, 0x40, 0x00, 0x40, 0x00
};
static unsigned char Each[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static unsigned char LetterA[] =
{
    0x40, 0xa0, 0xa0, 0xe0, 0xa0, 0x00, 0x00, 0x00
};
static unsigned char LetterB[] =
{
    0xe0, 0xa0, 0xe0, 0xa0, 0xe0, 0x00, 0x00, 0x00
};
static unsigned char LetterC[] =
{
    0xe0, 0x80, 0x80, 0x80, 0xe0, 0x00, 0x00, 0x00
};
static unsigned char LetterD[] =
{
    0xc0, 0xa0, 0xa0, 0xa0, 0xc0, 0x00, 0x00, 0x00
};
static unsigned char LetterE[] =
{
    0xe0, 0x80, 0xc0, 0x80, 0xe0, 0x00, 0x00, 0x00
};
static unsigned char LetterF[] =
{
    0xe0, 0x80, 0xc0, 0x80, 0x80, 0x00, 0x00, 0x00
};
static unsigned char LetterG[] =
{
    0xf0, 0x80, 0xb0, 0x90, 0xf0, 0x00, 0x00, 0x00
};
static unsigned char LetterH[] =
{
    0xa0, 0xa0, 0xe0, 0xa0, 0xa0, 0x00, 0x00, 0x00
};
static unsigned char LetterI[] =
{
    0xe0, 0x40, 0x40, 0x40, 0xe0, 0x00, 0x00, 0x00
};
static unsigned char LetterJ[] =
{
    0x20, 0x20, 0x20, 0xa0, 0xe0, 0x00, 0x00, 0x00
};
static unsigned char LetterK[] =
{
    0xa0, 0xc0, 0xc0, 0xa0, 0x90, 0x00, 0x00, 0x00
};
static unsigned char LetterL[] =
{
    0x80, 0x80, 0x80, 0x80, 0xe0, 0x00, 0x00, 0x00
};
static unsigned char LetterM[] =
{
    0x88, 0xd8, 0xa8, 0x88, 0x88, 0x00, 0x00, 0x00
};
static unsigned char LetterN[] =
{
    0x88, 0xc8, 0xa8, 0x98, 0x88, 0x00, 0x00, 0x00
};
static unsigned char LetterO[] =
{
    0xf0, 0x90, 0x90, 0x90, 0xf0, 0x00, 0x00, 0x00
};
static unsigned char LetterP[] =
{
    0xe0, 0xa0, 0xe0, 0x80, 0x80, 0x00, 0x00, 0x00
};
static unsigned char LetterQ[] =
{
    0xf0, 0x90, 0x90, 0xb0, 0xf0, 0x08, 0x00, 0x00
};
static unsigned char LetterR[] =
{
    0xe0, 0xa0, 0xe0, 0xa0, 0x90, 0x00, 0x00, 0x00
};
static unsigned char LetterS[] =
{
    0xe0, 0x80, 0xe0, 0x20, 0xe0, 0x00, 0x00, 0x00
};
static unsigned char LetterT[] =
{
    0xe0, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00
};
static unsigned char LetterU[] =
{
    0xa0, 0xa0, 0xa0, 0xa0, 0xe0, 0x00, 0x00, 0x00
};
static unsigned char LetterV[] =
{
    0xa0, 0xa0, 0xa0, 0xe0, 0x40, 0x00, 0x00, 0x00
};
static unsigned char LetterW[] =
{
    0x88, 0x88, 0xa8, 0xd8, 0x88, 0x00, 0x00, 0x00
};
static unsigned char LetterX[] =
{
    0x88, 0x50, 0x20, 0x50, 0x88, 0x00, 0x00, 0x00
};
static unsigned char LetterY[] =
{
    0xa0, 0xa0, 0xe0, 0x40, 0x40, 0x00, 0x00, 0x00
};
static unsigned char LetterZ[] =
{
    0xe0, 0x20, 0x40, 0x80, 0xe0, 0x00, 0x00, 0x00
};

static unsigned char Space[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define MAX_CHAR_IDX      47
#define ASPECT_RATIO      0.775F

static int graphicsMode = -1;
static unsigned char *CharList[MAX_CHAR_IDX + 1] = {Comma, Minus, Period, Slash,
        Number0, Number1, Number2, Number3, Number4,
        Number5, Number6, Number7, Number8, Number9,
        Colon, SemiColon, Less, Equal, More, Quest, Each,
        LetterA, LetterB, LetterC, LetterD, LetterE, LetterF, LetterG,
        LetterH, LetterI, LetterJ, LetterK, LetterL, LetterM, LetterN,
        LetterO, LetterP, LetterQ, LetterR, LetterS, LetterT, LetterU,
        LetterV, LetterW, LetterX, LetterY, LetterZ, Space
                                                   };

static int styles[MAX_STYLE + 1] = {0xFFFF, 0xC0C0};
static int linestyle = 0;
static float matrix00, matrix01, matrix10, matrix11;
static float x_offset = 0.0F, y_offset = 0.0F;

void SetDebugLineStyle(int style)
{
    linestyle = max(min(style, MAX_STYLE), 0);
}

void ResetRotateDebug2D(void)
{
    matrix00 = 1.0F;
    matrix01 = 0.0F;
    matrix10 = 0.0F;
    matrix11 = 1.0F;
}

void ResetTranslateDebug2D(void)
{
    x_offset = y_offset = 0.0F;
}

void TranslateDebug2D(float x, float y)
{
    x_offset += x;
    y_offset += y;
}

void RotateDebug2D(float angle)
{
    float tmp00, tmp01, tmp10, tmp11;
    float a, b, c, d;

    tmp00 = (float)cos(angle * DTR);
    tmp01 = -(float)sin(angle * DTR);
    tmp10 = -tmp01;
    tmp11 = tmp00;

    a = matrix00 * tmp00 + matrix01 * tmp10;
    b = matrix00 * tmp01 + matrix01 * tmp11;

    c = matrix10 * tmp00 + matrix11 * tmp10;
    d = matrix10 * tmp01 + matrix11 * tmp11;

    matrix00 = a;
    matrix01 = b;
    matrix10 = c;
    matrix11 = d;
}

void DrawDebugString(float x, float y, char *str)
{
    int row, col;
    int len;
    float x1, y1;

    if (graphicsMode not_eq DEBUGGER_GRAPHICS_MODE)
        return;

    x *= ASPECT_RATIO;

    x1 = x * matrix00 + y * matrix01;
    y1 = x * matrix10 + y * matrix11;

    len = strlen(str);

    x = min(max(x1 + x_offset, -1.0F), 1.0F);
    y = min(max(y1 + y_offset, -1.0F), 1.0F);

    col = (int)(0.5 + DebugScreenWidth * 0.5F * (1.0F + x));
    row = (int)(0.5 + DebugScreenHeight * 0.5F * (1.0F - y));
    DisplayDebugString(row, col - len * CHAR_WIDTH / 2, str);
}

void DrawDebugStringRight(float x, float y, char *str)
{
    int row, col;
    int len;
    float x1, y1;

    if (graphicsMode not_eq DEBUGGER_GRAPHICS_MODE)
        return;

    x *= ASPECT_RATIO;

    x1 = x * matrix00 + y * matrix01;
    y1 = x * matrix10 + y * matrix11;

    len = strlen(str);

    x = min(max(x1 + x_offset, -1.0F), 1.0F);
    y = min(max(y1 + y_offset, -1.0F), 1.0F);

    col = (int)(0.5 + DebugScreenWidth * 0.5F * (1.0F + x));
    row = (int)(0.5 + DebugScreenHeight * 0.5F * (1.0F - y));
    DisplayDebugString(row, col - len * CHAR_WIDTH, str);
}

void DrawDebugStringLeft(float x, float y, char *str)
{
    int row, col;
    float x1, y1;

    if (graphicsMode not_eq DEBUGGER_GRAPHICS_MODE)
        return;

    x *= ASPECT_RATIO;

    x1 = x * matrix00 + y * matrix01;
    y1 = x * matrix10 + y * matrix11;

    x = min(max(x1 + x_offset, -1.0F), 1.0F);
    y = min(max(y1 + y_offset, -1.0F), 1.0F);

    col = (int)(0.5 + DebugScreenWidth * 0.5F * (1.0F + x));
    row = (int)(0.5 + DebugScreenHeight * 0.5F * (1.0F - y));
    DisplayDebugString(row, col, str);
}

void DisplayDebugString(int row, int col, char *str)
{
    while (*str)
    {
        DisplayDebugCharacter((int)(toupper(*str++) - ','), col, row);
        col += CHAR_WIDTH;
    }
}
void DrawDebugLine(float x1in, float y1in, float x2in, float y2in)
{
    int x0, y0, x1, y1;
    float x2, y2;

    x2 = x1in * matrix00 + y1in * matrix01;
    y2 = x1in * matrix10 + y1in * matrix11;
    x1in = x2 + x_offset;
    y1in = y2 + y_offset;

    x2 = x2in * matrix00 + y2in * matrix01;
    y2 = x2in * matrix10 + y2in * matrix11;
    x2in = x2 + x_offset;
    y2in = y2 + y_offset;

    if (x1in <= x2in)
    {
        x0 = (int)(DebugScreenWidth * 0.5F * (1.0F + x1in * ASPECT_RATIO));
        y0 = (int)(DebugScreenHeight * 0.5F * (1.0F - y1in));
        x1 = (int)(DebugScreenWidth * 0.5F * (1.0F + x2in * ASPECT_RATIO));
        y1 = (int)(DebugScreenHeight * 0.5F * (1.0F - y2in));
    }
    else
    {
        x0 = (int)(DebugScreenWidth * 0.5F * (1.0F + x2in * ASPECT_RATIO));
        y0 = (int)(DebugScreenHeight * 0.5F * (1.0F - y2in));
        x1 = (int)(DebugScreenWidth * 0.5F * (1.0F + x1in * ASPECT_RATIO));
        y1 = (int)(DebugScreenHeight * 0.5F * (1.0F - y1in));
    }

    DisplayDebugLine(x0, y0, x1, y1);
}

void DisplayDebugLine(int x0, int y0, int x1, int y1)
{
    int dx, dy, ince, incne;
    int d, x, y, pixcount;

    if (graphicsMode not_eq DEBUGGER_GRAPHICS_MODE)
        return;

    dx = x1 - x0;
    dy = y1 - y0;
    x = x0;
    y = y0;
    pixcount = 0;

    if (dy >= 0 and dy <= dx)
    {
        d = 2 * dy - dx;
        ince = 2 * dy;
        incne = 2 * (dy - dx);
        WriteDebugPixel(x, y);

        while (x < x1)
        {
            if (d <= 0)
            {
                d += ince;
                x ++;
            }
            else
            {
                d += incne;
                x ++;
                y ++;
            }

            if (styles[linestyle] bitand (1 << (pixcount % 16)))
                WriteDebugPixel(x, y);

            pixcount ++;
        }
    }
    else if (dy < 0 and -dy < dx)
    {
        d = -2 * dy - dx;
        ince = -2 * dy;
        incne = 2 * (-dy - dx);

        while (x < x1)
        {
            if (d <= 0)
            {
                d += ince;
                x ++;
            }
            else
            {
                d += incne;
                x ++;
                y --;
            }

            if (styles[linestyle] bitand (1 << (pixcount % 16)))
                WriteDebugPixel(x, y);

            pixcount ++;
        }
    }
    else if (dx >= 0 and dy >= 0)
    {
        d = 2 * dx - dy;
        ince = 2 * dx;
        incne = 2 * (dx - dy);
        WriteDebugPixel(x, y);

        while (y < y1)
        {
            if (d <= 0)
            {
                d += ince;
                y ++;
            }
            else
            {
                x ++;
                d += incne;
                y ++;
            }

            if (styles[linestyle] bitand (1 << (pixcount % 16)))
                WriteDebugPixel(x, y);

            pixcount ++;
        }
    }
    else
    {
        d = 2 * dx + dy;
        ince = 2 * dx;
        incne = 2 * (dx + dy);
        WriteDebugPixel(x, y);

        while (y > y1)
        {
            if (d <= 0)
            {
                d += ince;
                y --;
            }
            else
            {
                d += incne;
                y --;
                x ++;
            }

            if (styles[linestyle] bitand (1 << (pixcount % 16)))
                WriteDebugPixel(x, y);

            pixcount ++;
        }
    }
}

void DrawDebugCircle(float xin, float yin, float radius)
{
    float x1in, y1in;
    int x_center, y_center;

    xin *= ASPECT_RATIO;

    x1in = min(max(xin + x_offset, -1.0F), 1.0F);
    y1in = min(max(yin + y_offset, -1.0F), 1.0F);
    x_center = (int)(DebugScreenWidth * 0.5F * (1.0F + x1in));
    y_center = (int)(DebugScreenHeight * 0.5F * (1.0F - y1in));

    DisplayDebugCircle(x_center, y_center, (int)(DebugScreenHeight * 0.5F * radius));
}

void DisplayDebugCircle(int x_center, int y_center, int radius)
{
    int x, y, d, de , dse;
    int x2, y2;

    if (graphicsMode not_eq DEBUGGER_GRAPHICS_MODE)
        return;

    x = 0;
    y = radius;
    d = 1 - y;
    de = 3;
    dse = -2 * y + 5;
    x2 = (int)((float)x * DebugScreenWidth / DebugScreenHeight * ASPECT_RATIO);
    WriteDebugPixel(x2 + x_center, y + y_center);
    WriteDebugPixel(-x2 + x_center, y + y_center);
    WriteDebugPixel(-x2 + x_center, -y + y_center);
    WriteDebugPixel(x2 + x_center, -y + y_center);
    y2 = (int)((float)y * DebugScreenWidth / DebugScreenHeight * ASPECT_RATIO);
    WriteDebugPixel(y2 + x_center, x + y_center);
    WriteDebugPixel(-y2 + x_center, x + y_center);
    WriteDebugPixel(-y2 + x_center, -x + y_center);
    WriteDebugPixel(y2 + x_center, -x + y_center);

    while (y > x)
    {
        if (d < 0)
        {
            d += de;
            de += 2;
            dse += 2;
            x ++;
        }
        else
        {
            d += dse;
            de += 2;
            dse += 4;
            x++;
            y--;
        }

        x2 = (int)((float)x * DebugScreenWidth / DebugScreenHeight * ASPECT_RATIO);
        WriteDebugPixel(x2 + x_center, y + y_center);
        WriteDebugPixel(-x2 + x_center, y + y_center);
        WriteDebugPixel(-x2 + x_center, -y + y_center);
        WriteDebugPixel(x2 + x_center, -y + y_center);
        y2 = (int)((float)y * DebugScreenWidth / DebugScreenHeight * ASPECT_RATIO);
        WriteDebugPixel(y2 + x_center, x + y_center);
        WriteDebugPixel(-y2 + x_center, x + y_center);
        WriteDebugPixel(-y2 + x_center, -x + y_center);
        WriteDebugPixel(y2 + x_center, -x + y_center);
    }
}

void DisplayDebugCharacter(int num, int x, int y)
{
    int i, j;
    unsigned char *data;
    unsigned char c;

    if (graphicsMode not_eq DEBUGGER_GRAPHICS_MODE)
        return;

    if (num < -4 or num > MAX_CHAR_IDX)
        num = MAX_CHAR_IDX;

    data = CharList[num];


    //LRKLUDGE
    if (num == -1)
    {
        DisplayDebugLine(x - 2, y - 2, x - 2, y + 6);
        DisplayDebugLine(x - 2, y - 2, x + 5, y - 2);
        DisplayDebugLine(x + 5, y + 6, x + 5, y - 2);
        DisplayDebugLine(x - 2, y + 6, x + 5, y + 6);
    }
    else if (num == -2)
    {
        DisplayDebugLine(x - 2, y - 2, x + 5, y - 2);
        DisplayDebugLine(x - 2, y + 6, x + 5, y + 6);
    }
    else if (num == -3)
    {
        DisplayDebugLine(x - 2, y - 2, x + 5, y - 2);
        DisplayDebugLine(x - 2, y + 6, x + 5, y + 6);
        DisplayDebugLine(x + 5, y + 6, x + 5, y - 2);
    }
    else if (num == -4)
    {
        DisplayDebugLine(x - 2, y - 2, x + 5, y - 2);
        DisplayDebugLine(x - 2, y + 6, x + 5, y + 6);
        DisplayDebugLine(x - 2, y - 2, x - 2, y + 6);
    }
    else
    {
        for (i = 0; i < 7; i++)
        {
            c = *data;

            for (j = 0; j < 5; j++)
            {
                if (c bitand 0x80)
                    WriteDebugPixel(x + j, y + i);

                c = (char)(c << 1);
            }

            data ++;
        }
    }
}

static int
spinner = 0,
spinner1 = 0,
spinner2 = 0,
spinner3 = 0;

static char
spin[] = "|/-\\";

void set_spinner1(int s)
{
    if (graphicsMode not_eq DEBUGGER_TEXT_MODE)
        return;

    spinner1 = s;

#ifndef DISABLE_MONO_DISPLAY
#if defined _TEXT_TGT_CONSOLE
#elif defined _TEXT_TGT_TRACE
#elif defined _TEXT_TGT_FILE
#else
    char
    *dst;

    dst = (char *) MONO_TEXT;
    dst[156] = spin[spinner1 bitand 3];;
    dst[157] = 7;
#endif
#endif
}

void set_spinner2(int s)
{
    if (graphicsMode not_eq DEBUGGER_TEXT_MODE)
        return;

    spinner2 = s;
#ifndef DISABLE_MONO_DISPLAY
#if defined _TEXT_TGT_CONSOLE
#elif defined _TEXT_TGT_TRACE
#elif defined _TEXT_TGT_FILE
#else
    char
    *dst;

    dst = (char *) MONO_TEXT;
    dst[154] = spin[spinner2 bitand 3];;
    dst[155] = 7;
#endif
#endif
}

void set_spinner3(int s)
{
    if (graphicsMode not_eq DEBUGGER_TEXT_MODE)
        return;

    spinner3 = s;
#ifndef DISABLE_MONO_DISPLAY
#if defined _TEXT_TGT_CONSOLE
#elif defined _TEXT_TGT_TRACE
#elif defined _TEXT_TGT_FILE
#else
    char
    *dst;

    dst = (char *) MONO_TEXT;
    dst[154] = spin[spinner3 bitand 3];;
    dst[155] = 7;
#endif
#endif
}

unsigned long WINAPI update_mono(void *ptr)
{
#if defined _TEXT_TGT_CONSOLE
#if 1
    COORD dwBufferSize = { 80, 25 };
    COORD dwBufferCoord = { 0, 0 };

    char *src;
    CHAR_INFO *dst;

    while (ptr and hStdoutDbg)
    {
        SMALL_RECT rc = { 0, 0, 80, 25 };

        src = mono_memory;
        dst = charinfo;

        for (int loop = 0; loop < 80 * 25 * 2; loop ++)
        {
            dst->Char.AsciiChar = *src++;
            dst->Attributes = *src++ ? FOREGROUND_GREEN : FOREGROUND_BLUE;

            dst ++;
        }


        BOOL b = WriteConsoleOutput(hStdoutDbg, charinfo, dwBufferSize, dwBufferCoord, &rc);

        Sleep(25);
    }

#elif defined _TEXT_TGT_TRACE
#elif defined _TEXT_TGT_FILE
#else
    COORD dwBufferCoord = { 0, 0 };
    DWORD cb;

    while (ptr and hStdoutDbg)
    {
        if ( not SetConsoleCursorPosition(hStdoutDbg, dwBufferCoord))
            OutputDebugString("Warning: WriteConsoleOutputA failed\n");

        if ( not WriteConsole(hStdoutDbg, mono_memory, 80 * 25 * 2, &cb, NULL))
            OutputDebugString("Warning: WriteConsoleOutputA failed\n");

        Sleep(25);
    }

#endif
#else
    int
    loop;

    char
    *src,
    *cmp,
    *dst;


    while (ptr)
    {
        src = mono_memory;
        cmp = mono_buffer;
        dst = (char *) MONO_TEXT;

        for (loop = 0; loop < 80 * 25 * 2; loop ++)
        {
            if (*cmp not_eq *src)
            {
                *dst = *src;
                *cmp = *src;
            }

            src ++;
            dst ++;
            cmp ++;
        }

        dst = (char *) MONO_TEXT;
        dst[158] = spin[(spinner ++) bitand 3];;
        dst[159] = 7;

        dst[156] = spin[spinner1 bitand 3];;
        dst[157] = 7;

        dst[154] = spin[spinner2 bitand 3];;
        dst[155] = 7;

        dst[152] = spin[spinner3 bitand 3];;
        dst[153] = 7;

        Sleep(25);
    }

#endif
    return 0;
}

CRITICAL_SECTION
mono_critical;

void InitDebug(int mode)
{
    int graph_mode[] =
    {53, 45, 46, 7, 91, 2, 87, 87, 2, 3, 0, 0, 0, 0, 0, 0};
    int text_mode[] =
    {97, 80, 82, 15, 25, 6, 25, 25, 2, 13, 11, 12, 0, 0, 0, 0};

#ifdef NDEBUG
    //   if (mode == DEBUGGER_TEXT_MODE)
    //      return;
#endif

#ifdef WRITE_FILE

    if ( not debugFile)
        debugFile = fopen("c:\\temp\\debug.dat", "w");

#endif

    return;
}

void WriteDebugPixel(int x, int y)
{
    int the_byte;
    int the_bit;
    char *cur_val;

    if (x < 0 or y < 0 or x > (int)DebugScreenWidth or y > (int)DebugScreenHeight)
        return;

    the_byte = 0x2000 * (y % 4) + 90 * (y / 4) + x / 8;
    the_bit  = 7 - x % 8;

    cur_val = (char *)(screen_buffer[page] + the_byte);
#ifndef DISABLE_MONO_DISPLAY
    *cur_val or_eq (char)(1 << the_bit);
#endif
}

void DebugSwapbuffer()
{
    int i;

    if (graphicsMode not_eq DEBUGGER_GRAPHICS_MODE)
        return;

    for (i = 0; i < 0x8000; i++)
    {
        if (*((char *)(screen_buffer[page] + i)) not_eq 
            *((char *)(screen_buffer[1 - page] + i)))
        {
#ifndef DISABLE_MONO_DISPLAY
            *((char *)(MONO_TEXT + i)) = *((char *)(screen_buffer[page] + i));
#endif
        }
    }

    page = 1 - page;
}

void DebugClear(void)
{
    if (graphicsMode not_eq DEBUGGER_GRAPHICS_MODE)
        return;

#ifndef DISABLE_MONO_DISPLAY
    memset((void *)screen_buffer[page], 0, 0x8000);
#endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if 0
void FileOutput(char *_mono_buffer)
{
    FILE
    *fp;

    fp = fopen("d:\\debug.out", "a");

    if (fp)
    {
        fputs(_mono_buffer, fp);

        fclose(fp);
    }
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void MonoPrint(char *string, ...)
{
    va_list params;   /* watcom manual 'Library' p.470 */
    int idx = 0;
    int   check;
    static char  _mono_buffer[1000];

    va_start(params, string);

    if (graphicsMode not_eq DEBUGGER_TEXT_MODE)
        return;

    if ( not string)
        return;

    EnterCriticalSection(&mono_critical);

    memset(mono_buffer, 0, sizeof(mono_buffer));
    check = vsprintf(_mono_buffer, string, params);
    va_end(params);

    // FileOutput (_mono_buffer);

#if defined _TEXT_TGT_CONSOLE
    COORD dwCursorPosition = { monoPenX, monoPenY };
    BOOL b = SetConsoleCursorPosition(hStdoutDbg, dwCursorPosition);

    if (b)
    {
        DWORD cb;
        b = WriteConsole(hStdoutDbg, _mono_buffer, check, &cb, NULL);

        if (b)
        {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            b = GetConsoleScreenBufferInfo(hStdoutDbg, &csbi);

            if (b)
            {
                monoPenX = (unsigned char) csbi.dwCursorPosition.X;
                monoPenY = (unsigned char) csbi.dwCursorPosition.Y;
            }
        }
    }

#elif defined _TEXT_TGT_TRACE

    if (_mono_buffer[check - 1] not_eq '\n')
        strcat(_mono_buffer, "\n");

    OutputDebugString(_mono_buffer);
#elif defined _TEXT_TGT_FILE

    if (_mono_buffer[check - 1] not_eq '\n')
    {
        strcat(_mono_buffer, "\n");
        check++;
    }

    DWORD cb;
    WriteFile(hFileDbg, _mono_buffer, check, &cb, NULL);
#else
    unsigned char  *mem_loc;
    mem_loc = (unsigned char *)(monoPenY * 160 + monoPenX * 2 + mono_memory);

    while (_mono_buffer[idx])
    {
        if (_mono_buffer[idx] == '\n')
        {
            mem_loc = MonoNewline();
            ++idx;
        }
        else
        {
#ifndef DISABLE_MONO_DISPLAY
            *(mem_loc++) = _mono_buffer[idx];
            *(mem_loc++) = (unsigned char) monoPenattribute;
#endif
            idx++;

            if ((++monoPenX) > 79)
            {
                mem_loc = MonoNewline();
            }
        }
    }

#endif

    LeaveCriticalSection(&mono_critical);
}

static unsigned char * MonoNewline(void)
{
    unsigned char
    *ptr;

    if (graphicsMode not_eq DEBUGGER_TEXT_MODE)
        return (0);

    EnterCriticalSection(&mono_critical);

    monoPenX = 0;
    ++monoPenY;

    if (monoPenY > 24)
    {
        monoPenY = 24;
#if defined _TEXT_TGT_CONSOLE
        ptr = ((unsigned char *)(monoPenY * 160 + monoPenX * 2 + mono_memory));
#elif defined _TEXT_TGT_TRACE
        OutputDebugString("\n");
#elif defined _TEXT_TGT_FILE
#else
        memset(linebuf, 0, sizeof(linebuf));
        ptr = linebuf;
#endif

        LeaveCriticalSection(&mono_critical);
        MonoScroll();
    }
    else
    {
#if defined _TEXT_TGT_CONSOLE
        ptr = ((unsigned char *)(monoPenY * 160 + monoPenX * 2 + mono_memory));
#elif defined _TEXT_TGT_TRACE
        OutputDebugString("\n");
#elif defined _TEXT_TGT_FILE
#else
        memset(linebuf, 0, sizeof(linebuf));
        ptr = linebuf;
#endif

        LeaveCriticalSection(&mono_critical);
    }

    return ptr;
}

void MonoScroll(void)
{
    if (graphicsMode not_eq DEBUGGER_TEXT_MODE)
        return;

#ifndef DISABLE_MONO_DISPLAY
#if defined _TEXT_TGT_CONSOLE
    char c = '\n';
    BOOL fSuccess = WriteFile(hStdoutDbg, &c, 1, NULL, NULL);
#elif defined _TEXT_TGT_TRACE
#elif defined _TEXT_TGT_FILE
#else
    memmove((void *)mono_memory, (void *)(mono_memory + 160), (160 * 24));
    memset((void *)(mono_memory + (160 * 24)), 0, 160);
#endif
#endif
}

void MonoLocate(unsigned char x, unsigned char y)
{
    if (graphicsMode not_eq DEBUGGER_TEXT_MODE)
        return;

    EnterCriticalSection(&mono_critical);
    monoPenX = x;
    monoPenY = y;
    LeaveCriticalSection(&mono_critical);
}

void MonoGetLoc(int* x, int* y)
{
    *x = monoPenX;
    *y = monoPenY;
}

void MonoColor(unsigned char attribute)
{
    if (graphicsMode not_eq DEBUGGER_TEXT_MODE)
        return;

    monoPenattribute = attribute;

#if defined _TEXT_TGT_CONSOLE
#elif defined _TEXT_TGT_TRACE
#elif defined _TEXT_TGT_FILE
#endif
}

void MonoCls(void)
{
    if (graphicsMode not_eq DEBUGGER_TEXT_MODE)
        return;

#ifndef DISABLE_MONO_DISPLAY

#if defined _TEXT_TGT_CONSOLE
    COORD coord;
    coord.X = 0;            // start at first cell
    coord.Y = 0;            //   of first row
    char chFillChar = ' ';
    DWORD cWritten = 0;

    BOOL fSuccess = FillConsoleOutputCharacter(
                        hStdoutDbg,          // screen buffer handle
                        chFillChar,       // fill with spaces
                        80 * 25,          // number of cells to fill
                        coord,            // first cell to write to
                        &cWritten);       // actual number written
#elif defined _TEXT_TGT_TRACE
#elif defined _TEXT_TGT_FILE
#else
    memset((void *)mono_memory, 0, (160 * 25));
#endif
#endif
    monoPenX = 0;
    monoPenY = 0;
}

void SetMonoGraphicsMode(int newMode)
{
#ifdef NDEBUG
    //   if (newMode == DEBUGGER_TEXT_MODE)
    //      return;
#endif

    if (graphicsMode not_eq -1)
        graphicsMode = newMode;
}
