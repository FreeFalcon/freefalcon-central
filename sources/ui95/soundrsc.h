#ifndef _SOUND_RSC_H_
#define _SOUND_RSC_H_

//
// This class is TIED to the C_Resmgr class (which holds the actual sound data)
//
// (So you need both to actually be able to play a sound)
//

// First item MUST be     short Type
class SoundHeader
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_SOUND_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		long  Type;
		char  ID[32];
		long  flags;
		short Channels;
		short SoundType;
		long  offset;
		long  headersize;
};

class SOUND_RSC
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_SOUND_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		long		ID;
		C_Resmgr	*Owner;
		SoundHeader	*Header;

		BOOL Play(int Stream);
		BOOL Loop(int Stream);
		BOOL Stream(int Stream);
};

#endif