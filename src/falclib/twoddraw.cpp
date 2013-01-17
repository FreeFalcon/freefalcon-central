#include <ctype.h>
#include <windows.h>
#include "falclib.h"
#include "sim\include\phyconst.h"
#include "twoddraw.h"

#define MAX_STYLE    1
#define CHAR_WIDTH 6

TWODDRAW_CLASS::TWODDRAW_CLASS (void)
{
   portOffsetX = 0;
   portOffsetY = 0;
   inUse = FALSE;
   lineStyle = 0;
   color = 0x0;
}

void TWODDRAW_CLASS::Lock (void)
{
   while (inUse)
      Sleep(0);
   inUse = TRUE;
}

void TWODDRAW_CLASS::UnLock (void)
{
   inUse = FALSE;
}

void TWODDRAW_CLASS::SetColor(unsigned long newColor)
{
	color = newColor;
}

void TWODDRAW_CLASS::SetLineStyle (int style)
{
    lineStyle = max ( min (style, MAX_STYLE), 0);
}

void TWODDRAW_CLASS::ResetRotate (void)
{
    matrix00 = 1.0F;
    matrix01 = 0.0F;
    matrix10 = 0.0F;
    matrix11 = 1.0F;
}

void TWODDRAW_CLASS::Rotate (float angle)
{
float tmp00, tmp01, tmp10, tmp11;
float a, b, c, d;

    tmp00 = (float)cos(angle * DTR);
    tmp01 = (float)sin(angle * DTR);
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


void TWODDRAW_CLASS::ResetTranslate (void)
{
   x_offset = 0.0F;
   y_offset = 0.0F;
}

void TWODDRAW_CLASS::Translate (float x, float y)
{
   x_offset += x * aspectRatio;
   y_offset += y;
}

void TWODDRAW_CLASS::DrawString (float x, float y, char *str)
{
short row, col;
int len;
float x1, y1;

   x *= aspectRatio;

   x1 = x * matrix00 + y * matrix01;
   y1 = x * matrix10 + y * matrix11;
          
   len = strlen (str);
        
   x = x1 + x_offset;
   y = y1 + y_offset;

   col = (short)(xRes / 2 * (1.0F + x));
   row = (short)(yRes / 2 * (1.0F - y));
   DisplayString (row, col - len * CHAR_WIDTH / 2, str);
}

void TWODDRAW_CLASS::DrawStringRight (float x, float y, char *str)
{
short row, col;
int len;
float x1, y1;

   x *= aspectRatio;

   x1 = x * matrix00 + y * matrix01;
   y1 = x * matrix10 + y * matrix11;

   len = strlen (str);

   x = x1 + x_offset;
   y = y1 + y_offset;

   col = (short)(xRes / 2 * (1.0F + x));
   row = (short)(yRes / 2 * (1.0F - y));
   DisplayString (row , col - len * CHAR_WIDTH, str);
}

void TWODDRAW_CLASS::DrawStringLeft (float x, float y, char *str)
{
short row, col;
float x1, y1;

   x *= aspectRatio;

   x1 = x * matrix00 + y * matrix01;
   y1 = x * matrix10 + y * matrix11;

   x = x1 + x_offset;
   y = y1 + y_offset;

   col = (short)(xRes / 2 * (1.0F + x));
   row = (short)(yRes / 2 * (1.0F - y));
   DisplayString (row , col, str);
}


void TWODDRAW_CLASS::DrawLine (float x1in, float y1in, float x2in, float y2in)
{
int x0, y0, x1, y1;
float x2, y2;

   x1in *= aspectRatio;
   x2in *= aspectRatio;

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
      x0 = (short)(xRes / 2 * (1.0F + x1in));
      y0 = (short)(yRes / 2 * (1.0F - y1in));
      x1 = (short)(xRes / 2 * (1.0F + x2in));
      y1 = (short)(yRes / 2 * (1.0F - y2in));
   }
   else
   {        
      x0 = (short)(xRes / 2 * (1.0F + x2in));
      y0 = (short)(yRes / 2 * (1.0F - y2in));
      x1 = (short)(xRes / 2 * (1.0F + x1in));
      y1 = (short)(yRes / 2 * (1.0F - y1in));
   }

   DisplayLine (x0, y0 , x1, y1 );
}


void TWODDRAW_CLASS::DrawCircle (float x, float y, float radius)
{
float x1in, y1in;
int x_center, y_center;

   x *= aspectRatio;

   x1in = x * matrix00 + y * matrix01;
   y1in = x * matrix10 + y * matrix11;

   x1in = x1in + x_offset;
   y1in = y1in + y_offset;

   x_center = (int)(xRes * 0.5F * (1.0F + x1in));
   y_center = (int)(yRes * 0.5F * (1.0F - y1in));

   DisplayCircle (x_center, y_center , (int)(yRes * 0.5F * radius));
}

void TWODDRAW_CLASS::DrawArc (float x, float y, float radius, float start, float end)
{
float x1in, y1in;
int x_center, y_center;

   x *= aspectRatio;

   x1in = x * matrix00 + y * matrix01;
   y1in = x * matrix10 + y * matrix11;

   x1in = x1in + x_offset;
   y1in = y1in + y_offset;

   x_center = (int)(xRes * 0.5F * (1.0F + x1in));
   y_center = (int)(yRes * 0.5F * (1.0F - y1in));

   DisplayArc(x_center, y_center , (int)(yRes * 0.5F * radius), start, end);
}

void TWODDRAW_CLASS::DisplayString (int row, int col, char *str)
{
   while (*str)
   {
        DisplayCharacter ((short)(toupper(*str++) - ','), col, row );
        col += CHAR_WIDTH;
   }
}

void TWODDRAW_CLASS::Viewport (int left, int right, int top, int bottom)
{

   if (bottom - top > winYres || right - left > winXres)
      return;

   portOffsetX = left;
   portOffsetY = top;
   xRes = right - left;
   yRes = bottom - top;
}
