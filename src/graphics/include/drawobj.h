/***************************************************************************
    DrawObj.h
    Scott Randolph
    May 3, 1996

    Abstract base class used to interact with drawable objects which need
 to be drawn sorted with terrain in an out the window view.
***************************************************************************/
#ifndef _DRAWOBJ_H_
#define _DRAWOBJ_H_

#include <cISO646>
#include <math.h>
#include "grTypes.h"

class DrawableObject
{
public:
    DrawableObject(float s)
    {
        drawClassID = Default, scale = s, parentList = NULL, prev = next = NULL;
    };
    // sfr: update parent list before destroying...
    virtual ~DrawableObject()
    {
        ShiAssert(parentList == NULL)
    };

    // RED- Object volume, used for Radar stuff
    virtual float GetRadarSign(void)
    {
        return 0.0f;
    }

    enum DrawClass { Default, BSP, GroundVehicle, Guys, Building, Platform, Bridge, Roadbed, Overcast, Puffy, Trail, realClouds }; //JAM 09Nov03

    DrawClass GetClass() const
    {
        return drawClassID;
    }
    float Radius() const
    {
        return radius;
    }
    void GetPosition(Tpoint* pos)
    {
        *pos = position;
    }
    float X() const
    {
        return position.x;
    }
    float Y() const
    {
        return position.y;
    }
    float Z() const
    {
        return position.z;
    }

    void SetScale(float s)
    {
        radius *= s / scale;
        scale = s;
    };
    float GetScale(void)
    {
        return scale;
    };

    virtual void SetLabel(char*, DWORD) {};

    virtual void SetInhibitFlag(BOOL) {};

    virtual void Draw(class RenderOTW *renderer, int LOD) = 0;
    virtual void Draw(class Render3D*) {};


    // ray hit not implemented yet for object
    virtual BOOL GetRayHit(const Tpoint*, const Tpoint*, Tpoint*, float = 1.0f)
    {
        return FALSE;
    } ;

    BOOL InDisplayList(void)
    {
        return (parentList not_eq NULL);
    };


protected:
    Tpoint position;
    float radius;
    float scale;
    DrawClass drawClassID;

    // NOTE:  Each instance can be managed by only ONE OBJECT LIST
    class ObjectDisplayList *parentList;
    DrawableObject *prev;
    DrawableObject *next;

    // NOTE:  This field is set by our parent list during UpdateMetrics
    float distance;

protected:
    virtual void SetParentList(ObjectDisplayList *list)
    {
        parentList = list;
    };
#define NEW_DRAWABLE_DESTRUCTION 1
#if NEW_DRAWABLE_DESTRUCTION
    friend class OTWDriverClass;
#endif
    friend class ObjectDisplayList;
    friend class RViewPoint;
    friend class DrawablePlatform;  // This one is weird -- DrawablePlatform isa DrawableObject,
    friend class DrawableBridge; // but the compiler complains.  Apparently, a second order
    // inheritance doesn't get to see "protected" members of its
    // grand parent.
public:
    void SetPosition(Tpoint *pos)
    {
        position = *pos;
    };
};

#endif // _DRAWOBJ_H_
