/***************************************************************************\
    DrawBSP.h
    Scott Randolph
    May 3, 1996

    Derived class to handle making drawing requests to the BSP object library.
\***************************************************************************/
#ifndef _DRAWBSP_H_
#define _DRAWBSP_H_

#include "ObjectInstance.h"
#include "DrawObj.h"

#ifdef USE_SH_POOLS
#include "SmartHeap/Include/smrtheap.h"
#endif

#include "context.h"


class DrawableBSP : public DrawableObject
{
public:
    DrawableBSP(int type, const Tpoint *pos, const Trotation *rot, float scale = 1.0f);
    virtual ~DrawableBSP();
protected:
    // This constructor is used only by derived classes who do their own setup
    DrawableBSP(float s, int ID): DrawableObject(s), instance(ID)
    {
        inhibitDraw = FALSE;
        labelLen = 0;
        id = ID;
        radius = instance.Radius();
        RadarSign = instance.RadarSign();
    };

public:
    void Update(const Tpoint *pos, const Trotation *rot);
    void SetRadius(float r)
    {
        instance.ParentObject->radius = r;
    }

    BOOL IsLegalEmptySlot(int slotNumber)
    {
        return (slotNumber < instance.ParentObject->nSlots) and (instance.SlotChildren) and (instance.SlotChildren[slotNumber] == NULL);
    };

    int GetNumSlots(void)
    {
        return instance.ParentObject->nSlots;
    };
    int GetNumDOFs(void)
    {
        return instance.ParentObject->nDOFs;
    };
    int GetNumSwitches(void)
    {
        return instance.ParentObject->nSwitches;
    };
    int GetNumDynamicVertices(void)
    {
        return instance.ParentObject->nDynamicCoords;
    };

    void AttachChild(DrawableBSP *child, int slotNumber);
    void DetachChild(DrawableBSP *child, int slotNumber);
    void GetChildOffset(int slotNumber, Tpoint *offset);

    bool SetupVisibility(RenderOTW *renderer);
    void SetDOFangle(int DOF, float radians);
    void SetDOFoffset(int DOF, float offset);
    void SetDynamicVertex(int vertID, float dx, float dy, float dz);
    void SetSwitchMask(int switchNumber, UInt32 mask);
    void SetTextureSet(UInt32 set)
    {
        instance.SetTextureSet(set);
    };
    int GetNTextureSet()
    {
        return instance.GetNTextureSet();
    };

    float GetDOFangle(int DOF);
    float GetDOFoffset(int DOF);
    void GetDynamicVertex(int vertID, float *dx, float *dy, float *dz);

    // RV - Biker
    void GetDynamicCoords(int vertID, float *dx, float *dy, float *dz);

    UInt32 GetSwitchMask(int switchNumber);
    UInt32 GetTextureSet(void)
    {
        return instance.TextureSet;
    };

    char *Label()
    {
        return label;
    }
    DWORD LabelColor()
    {
        return labelColor;
    }
    virtual void SetLabel(char *labelString, DWORD color);
    virtual void SetInhibitFlag(BOOL state)
    {
        inhibitDraw = state;
    };

    virtual BOOL GetRayHit(const Tpoint *from, const Tpoint *vector, Tpoint *collide, float boxScale = 1.0f);

    virtual void Draw(class RenderOTW *renderer, int LOD);
    virtual void Draw(class Render3D *renderer);

    int  GetID(void)
    {
        return id;
    };

    // These two functions are used to handle preloading BSP objects for quick drawing later
    static void LockAndLoad(int id)
    {
        TheObjectList[id].ReferenceWithFetch();
    };
    static void Unlock(int id)
    {
        TheObjectList[id].Release();
    };

    // get object's bounding box
    void GetBoundingBox(Tpoint *minB, Tpoint *maxB);

    // This one is for internal use only.  Don't use it or you'll break things...
    void ForceZ(float z)
    {
        position.z = z;
    };

public:
    Trotation orientation;
    static BOOL drawLabels; // Shared by ALL drawable BSPs

    ObjectInstance instance;
    // RED - Object volume, used for Radar stuff
    float GetRadarSign(void)
    {
        return RadarSign;
    }

protected:
    int id; // TODO: With the new BSP lib, this id could go...
    //ObjectInstance instance;

    BOOL inhibitDraw; // When TRUE, the Draw function just returns

    char label[32];
    int labelLen;
    DWORD labelColor;
    float RadarSign;

    // Handle time of day notifications
    static void TimeUpdateCallback(void *unused);

public:
    static void SetupTexturesOnDevice(DXContext *rc);
    static void ReleaseTexturesOnDevice(DXContext *rc);


protected:
    void DrawBoundingBox(class Render3D *renderer);

#ifdef USE_SH_POOLS
public:
    // Overload new/delete to use a SmartHeap fixed size pool
    void *operator new(size_t size)
    {
        ShiAssert(size == sizeof(DrawableBSP));
        return MemAllocFS(pool);
    };
    void operator delete(void *mem)
    {
        if (mem) MemFreeFS(mem);
    };
    static void InitializeStorage()
    {
        pool = MemPoolInitFS(sizeof(DrawableBSP), 50, 0);
    };
    static void ReleaseStorage()
    {
        MemPoolFree(pool);
    };
    static MEM_POOL pool;
#endif
};

#endif // _DRAWBSP_H_
