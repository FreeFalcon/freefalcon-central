#include "assert.h"

// ===============
// Linked List ADT
// ===============


#ifndef LISTADT_H
#define LISTADT_H

// =================================
// List Flags
// =================================

#define		LADT_SORTED_LIST		0x01			// Keep this list sorted
#define		LADT_FREE_USER_DATA		0x02			// Free user data on delete

// =================================
// List ADT - Private Implementation
// =================================

class ListClass;

class ListElementClass
	{
	friend class ListClass;
	private:
		void				*user_data;
		short				key;
		uchar				flags;
		ListElementClass	*prev;
		ListElementClass	*next;

	private:
		ListElementClass(short newKey=0, void *newData=NULL, uchar newFlags=0);
		~ListElementClass(void);

	public:
		void* GetUserData(void)						{ return user_data; }
		short GetKey(void)							{ return key; }

		void SetUserData(void *newData);
		void SetKey(short newKey)					{ key = newKey; }

		ListElementClass* GetNext(void)				{ return next; }
		ListElementClass* GetPrev(void)				{ return prev; }
#ifdef USE_SH_POOLS
   public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { assert( size == sizeof(ListElementClass) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(ListElementClass), 20, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
	};
typedef ListElementClass* ListNode;

class ListClass
	{
	private:
		uchar				flags;
		ListElementClass	*front;
		ListElementClass	*end;

	public:
		ListClass(uchar newFlags=0);
		~ListClass(void);

		void Insert(ListElementClass *newElement);
		void InsertAtEnd(ListElementClass *newElement);
		void Remove(ListElementClass *oldElement);
		void Detach(ListElementClass *oldElement);

		void InsertNewElement(short newKey, void *newData, uchar newFlags=0);
		void InsertNewElementAtEnd(short newKey, void *newData, uchar newFlags=0);

		void Purge(void);

		int SanityCheck (void);

		ListElementClass* GetFirstElement(void)		{ return front; }
		ListElementClass* GetLastElement(void)		{ return end; }
#ifdef USE_SH_POOLS
   public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { assert( size == sizeof(ListClass) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(ListClass), 20, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
	};
typedef ListClass* List;

#endif
