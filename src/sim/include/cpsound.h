#ifndef _CPSOUND_H
#define _CPSOUND_H

#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif


typedef struct {
	int	soundId;
	int	F4SoundEntry;
} CPSoundIndex;


class CPSoundList {
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif

	int				mNumSounds;
	int				mSoundTally;
	CPSoundIndex*	mpSoundArray;

public:

	int	GetSoundIndex(int);
	void	AddSound(int, int);

	CPSoundList(int);
	~CPSoundList();
};

#endif
