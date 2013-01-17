#ifndef _FACBRAIN_H
#define _FACBRAIN_H

#include "digi.h"

class TailInsertList;
class CampBaseClass;

class FACBrain : public DigitalBrain
{
   private:
      TailInsertList *fighterQ;
      int lastTarget;
      int numInTarget;
      SimBaseClass* controlledFighter;
      CampBaseClass* campaignTarget;
      enum FACFlags {
         FlightInbound = 0x1};
      int flags;

   public:
      void RequestTarget(SimVehicleClass* theFighter);
      void RequestMark(void);
      void RequestLocation(void);
      void RequestTACAN(void);
      void RequestBDA(SimVehicleClass* theFighter);
      SimBaseClass* AssignTarget (void);
      void AddToQ (SimVehicleClass* theFighter);
      void RemoveFromQ (SimVehicleClass* theFighter);
      void FrameExec(SimObjectType*, SimObjectType*);
      FACBrain (AircraftClass *myPlatform, AirframeClass* myAf);
      ~FACBrain (void);
      virtual void PostInsert (void);

#ifdef USE_SH_POOLS
public:
	// Overload new/delete because our parent class does (and assumes a fixed size)
	void *operator new(size_t size) { return MemAllocPtr(pool, size, 0); };
	void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
};

#endif /* _FACBRAIN_H */
