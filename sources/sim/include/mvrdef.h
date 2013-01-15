#ifndef _SIMMOVERDEF_H
#define _SIMMOVERDEF_H

#ifdef USE_SH_POOLS
extern MEM_POOL gReadInMemPool;
#endif

class SimMoverDefinition
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gReadInMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
   public:
      enum MoverType {Aircraft, Ground, Helicopter, Weapon, Sea};
      SimMoverDefinition (void);
      virtual ~SimMoverDefinition (void);
      static void ReadSimMoverDefinitionData (void);
      static void FreeSimMoverDefinitionData (void);
      int  numSensors;
      int* sensorData;
};

extern SimMoverDefinition** moverDefinitionData;
extern int NumSimMoverDefinitions;

#endif
