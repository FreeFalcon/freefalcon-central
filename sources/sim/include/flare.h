#ifndef _FLARE_H
#define _FLARE_H

#include "bomb.h"

class FlareClass : public BombClass
{
#ifdef USE_SH_POOLS
public:
	// Overload new/delete to use a SmartHeap fixed size pool
	void *operator new(size_t size) { ShiAssert( size == sizeof(FlareClass) ); return MemAllocFS(pool);	};
	void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
	static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(FlareClass), 200, 0 ); };
	static void ReleaseStorage()	{ MemPoolFree( pool ); };
	static MEM_POOL	pool;
#endif

private:
	DrawableTrail *trail;
	Drawable2D *trailGlow;
	Drawable2D *trailSphere;
	virtual void UpdateTrail (void);
	virtual void RemoveTrail (void);
	virtual void InitTrail (void);
	virtual void ExtraGraphics (void);
	virtual void DoExplosion (void);
	virtual void SpecialGraphics (void) {};

public:
	FlareClass (VU_BYTE** stream, long *rem);
	FlareClass (FILE* filePtr);
	FlareClass (int type);
	virtual ~FlareClass (void);
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
