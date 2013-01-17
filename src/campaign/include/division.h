#ifndef _DIVISION_H
#define _DIVISION_H

// =======================
// Division data structure
// =======================

class DivisionClass {
#ifdef USE_SH_POOLS
   public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(DivisionClass) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(DivisionClass), 50, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
	public:
		GridIndex		x;									// Averaged Location
		GridIndex		y;
		short			nid;								// Division id
		short			owner;
		uchar			type;								// type type (armor/infanty/etc)
		uchar			elements;							// Number of child units
		uchar			c_element;							// Which one we're looking at
		VU_ID			*element;							// VU_IDs of elements
		DivisionClass	*next;

	public:
		DivisionClass (void);
		~DivisionClass ();
		
		int BuildDivision (int control, int div);
		void GetLocation (GridIndex *rx, GridIndex *ry)		{ *rx = x; *ry = y; }
		uchar GetDivisionType (void)						{ return type; }
		_TCHAR* GetName (_TCHAR* buffer, int size, int object);

		Unit GetFirstUnitElement (void);
		Unit GetNextUnitElement (void);
		Unit GetUnitElement (int e);
		Unit GetUnitElementByID (int eid);
		Unit GetPrevUnitElement (Unit e);

		void UpdateDivisionStats (void);
		
		void RemoveChildren (void);
		void RemoveChild (VU_ID eid);
	};

typedef DivisionClass	*Division;

// ========================
// Other functions
// ========================

extern void DumpDivisionData (void);

extern void BuildDivisionData (void);

extern Division GetFirstDivision (int team);

extern Division GetNextDivision (Division d);

extern Division GetFirstDivisionByCountry (int country);

extern Division GetNextDivisionByCountry (Division d, int country);

extern Division GetDivisionByUnit (Unit u);

extern Division FindDivisionByXY (GridIndex x, GridIndex y);

#endif
