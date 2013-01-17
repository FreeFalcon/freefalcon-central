#ifndef _UI_SOUNDBITE_CAT_H_
#define _UI_SOUNDBITE_CAT_H_

class SOUNDCAT
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		long ID;
		long SoundID;
		long Used;
		SOUNDCAT *Next;
};

class CATLIST
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		long CatID;
		long Count;
		SOUNDCAT *Root;
		CATLIST *Next;
};

class C_SoundBite
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		CATLIST *Root;

	public:
		C_SoundBite();
		C_SoundBite(char **)	{ ; }
		C_SoundBite(FILE *)		{ ; }
		~C_SoundBite();
		long Size();
		void Save(char **)		{ ; }
		void Save(FILE *)		{ ; }

		void Setup();
		void Cleanup();

		CATLIST *Find(long CatID);

		void Add(long ID,long SoundID);
		long Pick(long Cat);
		long PickAlways(long Cat);
};

#endif