#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <conio.h>
#include <string.h>
#include <ctype.h>
#include "stdhdr.h"
#include "monodraw.h"

void Mono2D::Setup (void)
{
   Lock();

   InitDebug (GRAPHICS_MODE);
   ResetRotate();
   ResetTranslate();
   xRes = 720;
   yRes = 348;
   winXres = xRes;
   winYres = yRes;
   aspectRatio = 0.775F;

   UnLock();

}

void Mono2D::Cleanup (void)
{
   // Placeholder
}

void Mono2D::DisplayLine (int x0, int y0, int x1, int y1)
{
   DisplayDebugLine(x0, y0, x1, y1);
}

void Mono2D::DisplayCircle (int x_center, int y_center, int radius)
{
   DisplayDebugCircle (x_center, y_center, radius);
}

void Mono2D::DisplayArc (int xCenter, int yCenter, int radius, float start, float end)
{
int x, y, lastX, lastY;
float ang;

   xCenter += portOffsetX;
   yCenter += portOffsetY;

   lastX = lastY = (int)(2.0F * radius);

   for (ang = start; ang<end; ang += 0.5F)
   {
      x = (int)((float)(radius) * (float)cos(ang));
      y = (int)((float)(radius) * (float)sin(ang));
      if (x != lastX || y != lastY)
      {
      	WriteDebugPixel( x+xCenter,  y+yCenter);
      }
      lastX = x;
      lastY = y;
   }
}

void Mono2D::DisplayCharacter (short num, int x, int y)
{
   DisplayDebugCharacter (num, x, y);
}

void Mono2D::DisplayPixel (int x, int y)
{
   WriteDebugPixel (x, y);
}

void Mono2D::StartFrame (void)
{
   Lock();
}

void Mono2D::FinishFrame (void)
{
   UnLock();
}
