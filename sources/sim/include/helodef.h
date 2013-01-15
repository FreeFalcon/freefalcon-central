#ifndef _SIMHELODEFINITION_H
#define _SIMHELODEFINITION_H

#include "mvrdef.h"

class SimHeloDefinition : public SimMoverDefinition
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gReadInMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
   public:
      SimHeloDefinition (char*);
      ~SimHeloDefinition (void);
      int  airframeIndex;
};

#endif
