#ifndef _SIMWPNDEFINITION_H
#define _SIMWPNDEFINITION_H

#include "mvrdef.h"

class SimWpnDefinition : public SimMoverDefinition
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gReadInMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
   public:
      SimWpnDefinition (char*);
      ~SimWpnDefinition (void);
      int  flags;
      float cd;
      float weight;
      float area;
      float xEjection;
      float yEjection;
      float zEjection;
      char  mnemonic[8];
      int   weaponClass;
      int   domain;
      int   weaponType;
      int   dataIdx;
};

#endif
