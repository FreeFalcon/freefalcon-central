#ifndef _CHAFF_H
#define _CHAFF_H

#include "bomb.h"

class ChaffClass : public BombClass {
#ifdef USE_SH_POOLS
public:
	// Overload new/delete to use a SmartHeap fixed size pool
	void *operator new(size_t size) { ShiAssert( size == sizeof(ChaffClass) ); return MemAllocFS(pool);	};
	void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
	static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(ChaffClass), 200, 0 ); };
	static void ReleaseStorage()	{ MemPoolFree( pool ); };
	static MEM_POOL	pool;
#endif

private:
	virtual void UpdateTrail();
	virtual void RemoveTrail();
	virtual void InitTrail();
	virtual void ExtraGraphics();
	virtual void DoExplosion();
	virtual void SpecialGraphics();

public:
	ChaffClass(VU_BYTE** stream, long *rem);
	ChaffClass(FILE* filePtr);
	ChaffClass(int type);
	virtual ~ChaffClass();
	virtual void InitData();
	virtual void CleanupData();
private:
	void InitLocalData();
	 void CleanupLocalData();
public:

	virtual int SaveSize();
	virtual int Save(VU_BYTE **stream);	// returns bytes written
	virtual int Save(FILE *file);		// returns bytes written

	virtual int Wake (void);
	virtual int Sleep (void);
	virtual void Init (SimInitDataClass* initData);
	virtual void Init (void);
	virtual int Exec (void);
	virtual void Start(vector* pos, vector* rate, float cd);

	virtual void CreateGfx();
	virtual void DestroyGfx();
  DrawableBSP*	drawPointer;
};

#endif
