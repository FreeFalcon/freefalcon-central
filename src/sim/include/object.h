#ifndef _OBJECT_H
#define _OBJECT_H

#include "f4vu.h"
#include "sensclas.h"

#define MAX_OBJECTS 100
#define NUM_RADAR_HISTORY 4

#define OBJ_TAG_STR( f, l )		( f " Line " #l )
#define OBJ_TAG		OBJ_TAG_STR( __FILE__, __LINE__ )

class SimBaseClass;
class FalconEntity;

class SimObjectLocalData
{
public:
	SimObjectLocalData (void) {
		rdrDetect = painted = TWSTrackFileOpen = extrapolateStart = detFlags = nextLOSCheck = interrogated = 0;
		range = ata = az = el = droll = 0.0F; 
	};//Cobra 11/21/04 Added Interrogated

	// Absolute true relative geometry for this target
	Float32 ata;				// "antenna train angle" is total angle from nose to target
	Float32 ataFrom;			// total angle from targets nose to our own position
	Float32 atadot;				// ata change rate (radians/sec)
	Float32 ataFromdot;			// ataFrom rate
	Float32 az;					// body relative angle to target in "yaw" plane
	Float32 azFrom;				// same for targets view of us.
	Float32 azFromdot;			// rate of change of azFrom (radians/sec)
	Float32 el;					// body relative angle to target in "pitch" plane
	Float32 elFrom, elFromdot;	// from target to use values for elevation
	Float32 droll;				// body relative roll to target (how far to roll to get lift vector on target)
	Float32 range, rangedot;	// range to target (feet & feet/sec)

	// Radar specific target data (move into Radar classes???)
	BOOL	painted;					// Was this target painted this frame
	BOOL	TWSTrackFileOpen;	// MD -- 20040116: added for revise TWS mode
	unsigned int lockmsgsend;					// 0=no, 1 = lock 2 = unlock
	int lastRadarMode;					// 2002-02-10 ADDED BY S.G. Need to know the last mode the radar was in
	Float32 aspect;						// Target aspect (= 180.0F*DTR - ataFrom)
	Int32   rdrSy[NUM_RADAR_HISTORY];	// radar symbol (assigned by exec in RadarDoppler)
	Float32 rdrX[NUM_RADAR_HISTORY];	// azmuth in radians?
	Float32 rdrY[NUM_RADAR_HISTORY];	// range in feet (radial)
	Float32 rdrHd[NUM_RADAR_HISTORY];	// our heading at target paint time (platform->Yaw())
	VU_TIME rdrLastHit;					// Last time this target was detected (SimLibElapsedTime)
	UInt32	rdrDetect;					// Bit field indicating when we did/didn't detect the target
	VU_TIME extrapolateStart;			// MD -- 20040121: start extrapolate timer.

	// Digi use only
	Float32 threatTime;					// How long for him to kill us (digi use only) 
	Float32 targetTime;					// How long to kill this target (digi use only)
	BOOL interrogated;					// Did we get interrogated? Cobra 11/21/04
	int threatScore;				// Cobra threatTime/targetTime ignored now. Going to score threats
										// in Sensorfusion()

	// Per sensor data
	Int32		sensorLoopCount[SensorClass::NumSensorTypes];	// Number of frames since the target was last seen
	SensorClass::TrackTypes	sensorState[SensorClass::NumSensorTypes];	// What kind of sensor lock do we have

	Float32		irSignature;									// Should go away - look it up when required

	UInt32		detFlags;				//flag field indicating which los's are true
	UInt32		nextLOSCheck;			//next time to check LOS again

	int		CloudLOS(void)				{return (detFlags & 0x01) && TRUE;}
	void	SetCloudLOS(int value)		{if(value)detFlags |= 0x01; else detFlags &= ~0x01;}
	int		TerrainLOS(void)			{return (detFlags & 0x02) && TRUE;}
	void	SetTerrainLOS(int value)	{if(value)detFlags |= 0x02; else detFlags &= ~0x02;}
#ifdef USE_SH_POOLS
   public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(SimObjectLocalData) ); return MemAllocFS(pool);	};
    void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
    static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(SimObjectLocalData), 1000, 0 ); };
    static void ReleaseStorage()	{ MemPoolFree( pool ); };
    static MEM_POOL	pool;
#endif
};


class SimObjectType
{
public:
	SimObjectType(FalconEntity* baseObj);
	SimObjectType* Copy(void);

	void Reference(void);
	void Release(void);

	FalconEntity* BaseData(void) { return baseData.get(); };
	BOOL IsReferenced(void);

private:
	~SimObjectType();
	// sfr: changed to smartpointer
	FalconEntityBin		baseData;

private:
	F4CSECTIONHANDLE *mutex;
	int					refCount;

public:
	SimObjectLocalData	*localData;

	SimObjectType		*next;
	SimObjectType		*prev;


#ifdef USE_SH_POOLS
   public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(SimObjectType) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(SimObjectType), 1000, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
};

typedef SimObjectType* SimObjectPtr;

#endif // _OBJECT_H
