#ifndef _FLAT_RSC_H_
#define _FLAT_RSC_H_

//
// This class is TIED to the C_Resmgr class (which holds the actual sound data)
//
// (So you need both to actually be able to play a sound)
//

// First item MUST be     short Type
class FlatHeader
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_GENERAL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		long  Type;
		char  ID[32];
		long  offset;
		long  size;
};

class FLAT_RSC
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_GENERAL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		long		ID;
		C_Resmgr	*Owner;
		FlatHeader	*Header;

		FLAT_RSC()
		{
			ID=0;
			Owner=NULL;
			Header=NULL;
		}
		~FLAT_RSC() {}
		void *GetData();
};

// Code for the 1 function is in soundrsc.cpp

#endif