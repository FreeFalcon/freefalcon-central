/***************************************************************************\
    ObjectInstance.cpp
    Scott Randolph
    February 9, 1998

    Provides structures and definitions for 3D objects.
\***************************************************************************/
#include <cISO646>
#include "stdafx.h"
#include "StateStack.h"
#include "ObjectInstance.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gBSPLibMemPool;
#endif

ObjectInstance::ObjectInstance(int tid)
{
    id = tid;

    ShiAssert(TheObjectList);
    ShiAssert(id < TheObjectListLength); // Make sure the requested object ID is in
    ShiAssert(TheObjectList[tid].nLODs); // the current object set (IDS.TXT, etc).

    ParentObject = &TheObjectList[tid];

    /***** BEGIN HACK HACK HACK HACK - Billy forced me to do it :-) -RH *****/
    if
    ( // F16c
        (tid == 1052) or
        (tid == 564) or
        (tid == 563) or
        (tid == 562) or
        (tid == 5)
    )
    {
        ParentObject->radius = 40.0;
    }

    if (tid == 1329) // B1
    {
        ParentObject->radius = 100.0;
    }

    /***** END HACK HACK HACK HACK - Billy forced me to do it :-) -RH *****/

    ParentObject->Reference();

    if (ParentObject->nSwitches == 0)
    {
        SwitchValues = NULL;
    }
    else
    {
#ifdef USE_SH_POOLS
        SwitchValues = (DWORD *)MemAllocPtr(gBSPLibMemPool, sizeof(DWORD) * (ParentObject->nSwitches), 0);
#else
        SwitchValues = new DWORD[ParentObject->nSwitches];
#endif
        memset(SwitchValues, 0, sizeof(*SwitchValues)*ParentObject->nSwitches);
    }

    if (ParentObject->nDOFs == 0)
    {
        DOFValues = NULL;
    }
    else
    {
#ifdef USE_SH_POOLS
        DOFValues = (DOFvalue *)MemAllocPtr(gBSPLibMemPool, sizeof(DOFvalue) * (ParentObject->nDOFs), 0);
#else
        DOFValues = new DOFvalue[ParentObject->nDOFs];
#endif
        memset(DOFValues, 0, sizeof(*DOFValues)*ParentObject->nDOFs);
    }

    if (ParentObject->nSlots == 0)
    {
        SlotChildren = NULL;
    }
    else
    {
#ifdef USE_SH_POOLS
        SlotChildren = (ObjectInstance **)MemAllocPtr(gBSPLibMemPool, sizeof(ObjectInstance *) * (ParentObject->nSlots), 0);
#else
        SlotChildren = new ObjectInstance*[ParentObject->nSlots];
#endif
        memset(SlotChildren, 0, sizeof(*SlotChildren)*ParentObject->nSlots);
    }

    if ((ParentObject->nDynamicCoords == 0) or (ParentObject->nDynamicCoords > 10))
    {
        DynamicCoords = NULL;
    }
    else
    {
#ifdef USE_SH_POOLS
        DynamicCoords = (Ppoint *)MemAllocPtr(gBSPLibMemPool, sizeof(Ppoint) * (ParentObject->nDynamicCoords), 0);
#else
        DynamicCoords = new Ppoint[ParentObject->nDynamicCoords];
#endif
        memcpy(DynamicCoords,
               ParentObject->pSlotAndDynamicPositions + ParentObject->nSlots,
               sizeof(*DynamicCoords)*ParentObject->nDynamicCoords);
    }

    TextureSet = 0;
    TexSetReferenced = false;

    /*// Setup RadarSign
    RadarSignal = BoxTop() - BoxBottom();
    if(abs(RadarSignal) <= 1.0f) RadarSignal = 200.0f;
    else {
     RadarSignal *= BoxFront() - BoxBack();
     RadarSignal *= BoxRight() - BoxLeft();
    }
    RadarSignal = sqrtf(fabs(RadarSignal));*/

    RadarSignal = ParentObject->RadarSign;



}


ObjectInstance::~ObjectInstance()
{
    if (TexSetReferenced) 
        ParentObject->ReleaseTexSet(TextureSet);

    ParentObject->Release();
    ParentObject = NULL;
#ifdef USE_SH_POOLS

    if (SwitchValues)
        MemFreePtr(SwitchValues);

    if (DOFValues)
        MemFreePtr(DOFValues);

    if (SlotChildren)
        MemFreePtr(SlotChildren);

    if (DynamicCoords)
        MemFreePtr(DynamicCoords);

#else

    // sfr: checking for NULL values first
    if (SwitchValues not_eq NULL)
    {
        delete[] SwitchValues;
    }

    if (DOFValues not_eq NULL)
    {
        delete[] DOFValues;
    }

    if (SlotChildren not_eq NULL)
    {
        delete[] SlotChildren;
    }

    if (DynamicCoords not_eq NULL)
    {
        delete[] DynamicCoords;
    }

#endif
}


void ObjectInstance::SetDynamicVertex(int id, float dx, float dy, float dz)
{
    Ppoint *original;

    ShiAssert(id < ParentObject->nDynamicCoords);

    original = ParentObject->pSlotAndDynamicPositions + ParentObject->nSlots + id;

#ifdef _DEBUG
    float r1 = Radius() * Radius();
#endif

    DynamicCoords[id].x = original->x + dx;
    DynamicCoords[id].y = original->y + dy;
    DynamicCoords[id].z = original->z + dz;

#ifdef _DEBUG
    float r2 = DynamicCoords[id].x * DynamicCoords[id].x + DynamicCoords[id].y * DynamicCoords[id].y + DynamicCoords[id].z * DynamicCoords[id].z;

    //Need to take a further look why it should be this way.
    //ShiAssert(r2 <= r1 + 0.00001f); // Illegal for dynamic verts to exceed object bounding volume
    // so we require movement to be only toward the origin.
#endif
}


void ObjectInstance::GetDynamicVertex(int id, float *dx, float *dy, float *dz)
{
    Ppoint *original;

    ShiAssert(id < ParentObject->nDynamicCoords);

    original = ParentObject->pSlotAndDynamicPositions + ParentObject->nSlots + id;

    *dx = DynamicCoords[id].x - original->x;
    *dy = DynamicCoords[id].y - original->y;
    *dz = DynamicCoords[id].z - original->z;
}

// RV - Biker - New function to read dynamic data
void ObjectInstance::GetDynamicCoords(int id, float *dx, float *dy, float *dz)
{
    *dx = DynamicCoords[id].x;
    *dy = DynamicCoords[id].y;
    *dz = DynamicCoords[id].z;
}



void ObjectInstance::SetTextureSet(int id)
{
    // edg: sanity check texture setting
    if (id >= ParentObject->nTextureSets) id = 0;

    // check if we already hold a texture set
    if (TexSetReferenced)
    {
        // if same texture set, exit
        if (id == TextureSet) return;

        // else release the owned one
        ParentObject->ReleaseTexSet(TextureSet);
    }

    //New texture set
    TextureSet = id;
    // get it, and mark as owning a Tex Set
    ParentObject->ReferenceTexSet(TextureSet);
    TexSetReferenced = true;
};
