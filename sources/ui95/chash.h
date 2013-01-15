#ifndef _C_HASH_TABLE_H_
#define _C_HASH_TABLE_H_

class C_HASHNODE
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_GENERAL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		long ID;
		long Check;
		void *Record;
		C_HASHNODE *Next;
};

class C_HASHROOT
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_GENERAL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		C_HASHNODE *Root_;
};

class C_Hash
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_GENERAL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		long flags_;
		long TableSize_;
		C_HASHROOT *Table_;
		long Check_;

		void (*Callback_)(void *rec);

	public:

		C_Hash();
		~C_Hash();

		void Setup(long Size);
		void Cleanup();

		void SetFlags(long flags) { flags_=flags; }
		long GetFlags() { return(flags_); }

		void SetCallback(void (*cb)(void*)) { Callback_=cb; }

		void SetCheck(long check)	{ Check_=check; }
		long GetCheck()				{ return(Check_); }

		void *Find(long ID);
		_TCHAR *FindText(long ID);
		long FindTextID(long ID);
		long FindTextID(_TCHAR *txt);

		void Add(long ID,void *rec);
		long AddText(const _TCHAR *string);
		long AddTextID(long ID,_TCHAR *string);

		void Remove(long ID);

		void RemoveOld();

		void *GetFirstOld(C_HASHNODE **current,long *curidx);
		void *GetNextOld(C_HASHNODE **current,long *curidx);

		void *GetFirst(C_HASHNODE **cur,long *curidx);
		void *GetNext(C_HASHNODE **cur,long *curidx);
};
#endif
