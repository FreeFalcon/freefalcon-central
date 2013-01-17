#ifndef _LIMITERS_H
#define _LIMITERS_H

#include "stdhdr.h"
#include "simfile.h"

typedef enum{
	ltLine		= 0,
	ltValue		= 1,
	ltPercent	= 2,
	ltThreePt	= 3,
	ltMinMax	= 4,
}LimiterType;

enum{
	NegGLimiter				= 0,
	PosGLimiter				= 1,
	RollRateLimiter			= 2,
	YawAlphaLimiter			= 3,
	YawRollRateLimiter		= 4,
	CatIIICommandType		= 5,
	CatIIIAOALimiter		= 6,
	CatIIIRollRateLimiter	= 7,
	CatIIIYawAlphaLimiter	= 8,
	CatIIIYawRollRateLimiter = 9,
	PitchYawControlDamper	= 10,
	RollControlDamper		= 11,
	CommandType				= 12,
	LowSpeedOmega			= 13,
	StoresDrag				= 14,
	LowSpeedPitchDown		= 15,
	CatIIIMaxGs				= 16,
	AOALimiter				= 17,
	NumLimiterTypes
};

class Limiter
{	
protected:
	LimiterType type;

public:
	
	Limiter(LimiterType ltype);
	LimiterType Type(void)				{return type;}

	virtual float Limit(float x);
	virtual void Setup(char *)	{;}
	
};

class LineLimiter: public Limiter
{
#ifdef USE_SH_POOLS
   public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(LineLimiter) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(LineLimiter), 20, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
protected:
	float upperX;
	float lowerX;
	float m;
	float b;
public:
	LineLimiter(void);
	virtual void Setup(char *string);
	void Setup(float x1,float y1, float x2, float y2);
	virtual float Limit(float x);
};

class ThreePointLimiter: public Limiter
{
#ifdef USE_SH_POOLS
   public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(ThreePointLimiter) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(ThreePointLimiter), 20, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
protected:
	float upperX;
	float middleX;
	float lowerX;
	float m1, m2;
	float b1, b2;
public:
	ThreePointLimiter(void);
	virtual void Setup(char *string);
	void Setup(float *x,float *y);
	virtual float Limit(float x);
};

class ValueLimiter: public Limiter
{
#ifdef USE_SH_POOLS
   public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(ValueLimiter) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(ValueLimiter), 20, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
protected:
	float value;
public:
	ValueLimiter(void);
	virtual void Setup(char *string);
	void Setup(float newValue);
	virtual float Limit(float x);
};

class MinMaxLimiter: public Limiter
{
#ifdef USE_SH_POOLS
   public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(MinMaxLimiter) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(MinMaxLimiter), 20, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
protected:
	float minimum;
	float maximum;
public:
	MinMaxLimiter(void);
	virtual void Setup(char *string);
	void Setup(float min, float max);
	virtual float Limit(float x);
};

class PercentLimiter: public Limiter
{
#ifdef USE_SH_POOLS
   public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(PercentLimiter) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(PercentLimiter), 20, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
protected:
	float percent;
public:
	PercentLimiter(void);
	virtual void Setup(char *string);
	void Setup(float newPercent);
	virtual float Limit(float x);
};

typedef struct LimiterLink
{
	Limiter			*limiter;
	int				key;
	LimiterLink		*next;
}LimiterLink;

class LimiterMgrClass
{
protected:
	Limiter		**limiterDatasets;
	int			numDatasets;
public:
	LimiterMgrClass(int numDatasets);
	~LimiterMgrClass(void);
	int		ReadLimiters(SimlibFileClass *file, int dataset);
	Limiter *GetLimiter(int key, int dataset);
	int		HasLimiter(int key, int dataset);
};

extern LimiterMgrClass *gLimiterMgr;

#endif