#ifndef _GEOMETRY_H
#define _GEOMETRY_H

//#include "falclib/include/f4vu.h"

class SimObjectType;
class SimBaseClass;
class FalconEntity;

typedef float TransformMatrix[3][3];

class ObjectGeometry
{
#ifdef USE_SH_POOLS
public:
    // Overload new/delete to use a SmartHeap fixed size pool
    void *operator new(size_t size)
    {
        ShiAssert(size == sizeof(ObjectGeometry));
        return MemAllocFS(pool);
    };
    void operator delete(void *mem)
    {
        if (mem) MemFreeFS(mem);
    };
    static void InitializeStorage()
    {
        pool = MemPoolInitFS(sizeof(ObjectGeometry), 100, 0);
    };
    static void ReleaseStorage()
    {
        MemPoolFree(pool);
    };
    static MEM_POOL pool;
#endif

public:
    ObjectGeometry() {}
    ~ObjectGeometry() {}

    float sinalp, cosalp, tanalp;
    float sinbet, cosbet, tanbet;
    float singam, cosgam;
    float sinsig, cossig;
    float sinmu,  cosmu;
    float sinthe, costhe;
    float sinphi, cosphi;
    float sinpsi, cospsi;
};

float TargetAz(FalconEntity* af1, float x, float y);
float TargetAz(FalconEntity* af1, FalconEntity* af2);
float TargetAz(FalconEntity* af1, SimObjectType* af2);
float TargetEl(FalconEntity* af1, float x, float y, float z);
float TargetEl(FalconEntity* af1, FalconEntity* af2);
float TargetEl(FalconEntity* af1, SimObjectType* af2);
void TargetAzEl(FalconEntity* af1, float x, float y, float z, float &az, float &el);  // MLR 6/28/2004 -
void GetXYZ(SimBaseClass* platform, float az, float el, float range,
            float *x, float *y, float *z);
void CalcRelAzEl(SimBaseClass*, float, float, float, float*, float*); // added by VWF 7/1/98
void CalcRelValues(SimBaseClass*, FalconEntity*, float*, float*, float*, float*, float*);  // added by VWF 7/6/98
void CalcRelAzElRangeAta(SimBaseClass* ownObject, SimObjectType* targetPtr);  // KCK 8/6

void CalcRelGeom(SimBaseClass* obj, SimObjectType* objList, TransformMatrix vmat, float elapsedTime);
struct vector;
int FindCollisionPoint(SimBaseClass* obj, SimBaseClass* ownShip, vector* collPoint, float speedBoost = 0.0F);

#endif

