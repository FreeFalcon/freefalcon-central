#ifndef _MAVDISP_H
#define _MAVDISP_H

#include "misldisp.h"
class SimBaseClass;
class SimMoverClass;

class MaverickDisplayClass : public MissileDisplayClass
{
   private:
      void DrawTerrain (void);
      int onMainScreen;
      int seesTarget;
      int toggleFOV;
      float seekerAz, seekerEl;
      float curFOV;
      void DrawDisplay (void);
      enum {NoTarget, TargetDetected, TargetLocked};
      int trackStatus;

   public:
      MaverickDisplayClass(SimMoverClass* newPlatform);
      virtual ~MaverickDisplayClass()	{};

      virtual void DisplayInit (ImageBuffer*);
      virtual void Display (VirtualDisplay *newDisplay);

      int SeesTarget (void) { return seesTarget; };
      void SetTarget (int flag) { seesTarget = flag; };
      void GetSeekerPos (float* az, float* el) { *az = seekerAz; *el = seekerEl; };
      void SetSeekerPos (float az, float el) { seekerAz = az; seekerEl = el; };
      float CurFOV (void) {return curFOV;};
      void ToggleFOV (void) {toggleFOV = TRUE;};
      int IsDetected (void) {return (trackStatus == TargetDetected);};
      int IsLocked (void) {return (trackStatus == TargetLocked);};
      void LockTarget (void);
      void DetectTarget (void);
      void DropTarget (void);
      int IsCentered (SimBaseClass* testObj);

	  //MI
	  void DrawFOV(void);
};

#endif
