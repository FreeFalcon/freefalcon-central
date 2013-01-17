#ifndef _MONODRAW_H
#define _MONODRAW_H

#include "twoddraw.h"
#include "Ttypes.h"

class Mono2D : public TWODDRAW_CLASS
{
   private:
      virtual void DisplayLine (int x1, int y1, int x2, int y2);
      virtual void DisplayPixel (int x, int y);
      virtual void DisplayCircle (int x, int y, int radius);
      virtual void DisplayArc (int x, int y, int radius, float, float);
      virtual void DisplayCharacter (short num, int x, int y);
      char *screen_buffer[2];
      int page;

   public:

      void Setup(void);
      void Cleanup(void);
      virtual void StartFrame(void);
      virtual void FinishFrame(void);
};

#endif
