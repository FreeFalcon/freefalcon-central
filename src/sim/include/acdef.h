#ifndef _SIMACDEFINITION_H
#define _SIMACDEFINITION_H

#include "mvrdef.h"

class SimACDefinition : public SimMoverDefinition
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gReadInMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
   public:
      // NOTE!!!: This matches the list in digi.h for Maneuver class
      enum CombatClass {F4, F5, F14, F15, F16, Mig25, Mig27, A10, Bomber};
      CombatClass combatClass;

      SimACDefinition (char*);
      ~SimACDefinition (void);
      int  airframeIndex;
      int  numPlayerSensors;
      int* playerSensorData;
};

#endif
