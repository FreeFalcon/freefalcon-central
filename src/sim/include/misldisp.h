#ifndef _MISLDISP_H
#define _MISLDISP_H

#include "drawable.h"
#include "f4vu.h"
#include "simmover.h"

class MissileDisplayClass : public DrawableClass
{
	protected:
      enum FlagData {DisplayReady = 0x2};
      int displayType;
      SimMoverClass* platform;
      int flags;

   public:
      enum DisplayType {AGM65_IR, AGM65_TV, AGM88_HTS, NoDisplay};
      void GetXYZ (float* x, float* y, float* z) { *x = platform->XPos();
          *y = platform->YPos();  *z = platform->ZPos(); } ;
      void SetXYZ (float x, float y, float z) { platform->SetPosition (x, y, z); } ;
      void GetYPR (float* y, float* p, float* r) { *y = platform->Yaw();
          *p = platform->Pitch(); *r = platform->Roll(); } ;
      void SetYPR (float y, float p, float r) { platform->SetYPR(y, p, r); };
      MissileDisplayClass(SimMoverClass* newPlatform);
      int IsReady (void) {return flags & DisplayReady; };
      void SetReady (int flag) {if (flag) flags |= DisplayReady; else flags &= ~DisplayReady; };
      virtual void DisplayInit (ImageBuffer* newImage);
      void Display (VirtualDisplay*);
      int DisplayType (void) {return displayType;};
};

#endif
