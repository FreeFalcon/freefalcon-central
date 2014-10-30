/**********************
 *
 *
 * BSPList Manager for the UI
 *
 *
 **********************/

#include "graphics/include/matrix.h"
#include "graphics/include/drawobj.h"
#include "graphics/include/drawbrdg.h"
#include "graphics/include/drawbsp.h"
#include "graphics/include/drawbldg.h"
#include "graphics/include/drawgrnd.h"
#include "graphics/include/drawbrdg.h"
#include "graphics/include/drawplat.h"
#include "graphics/include/drawguys.h"
#include "vu2.h"
#include "F4vu.h"
#include "team.h"
#include "sim/include/simbase.h"
//#include "simlib.h"
//#include "initdata.h"
//#include "simfiltr.h"
//#include "simdrive.h"
//#include "simfeat.h"
#include "vehicle.h"
#include "objectiv.h"
#include "feature.h"
#include "find.h"
#include "dispcfg.h"
#include "cbsplist.h"
#include "classtbl.h"

void PositandOrientSetData(float x, float y, float z, float pitch, float roll, float yaw,
                           Tpoint* simView, Trotation* viewRotation);

long GetFeatureFlags(Objective obj, short featno)
{
    FeatureClassDataType* fc;
    long classID;

    if (featno < 0 or featno >= obj->GetObjectiveClassData()->Features)
        return(0);

    classID = obj->GetFeatureID(featno);

    if (classID)
    {
        fc = GetFeatureClassData(classID);

        if (fc)
            return(fc->Flags);
    }

    return(0);
}

void C_BSPList::Setup()
{
    Root_ = NULL;
    LockCount_ = 0;
}

void C_BSPList::Cleanup()
{
    RemoveList(&Root_);

    RemoveLockList();
}

void C_BSPList::Add(BSPLIST **list, BSPLIST *obj)
{
    BSPLIST *cur;

    if (obj == NULL) return;

    if (*list == NULL)
        *list = obj;
    else
    {
        cur = *list;

        while (cur->Next not_eq NULL)
            cur = cur->Next;

        cur->Next = obj;
    }
}

void C_BSPList::RemoveList(BSPLIST **list)
{
    BSPLIST *cur, *prev;

    // Remove Children from their parents
    cur = *list;

    while (cur not_eq NULL)
    {
        if (cur->owner)
        {
            delete cur->object;
            cur->object = NULL;
            // if(cur->owner->type == FEAT_ELEV_CONTAINER)
            // {
            // ((DrawableBridge*)cur->owner->object)->Remove(cur->object);
            // cur->owner=NULL;
            // }
            // else if(cur->owner->type == FEAT_FLAT_CONTAINER)
            // {
            // ((DrawablePlatform*)cur->owner->object)->Remove(cur->object);
            // cur->owner=NULL;
            // }
        }

        cur = cur->Next;
    }

    // free up the allocated shit
    cur = *list;

    while (cur not_eq NULL)
    {
        prev = cur;
        cur = cur->Next;

        if (prev->object)
            delete prev->object;

        delete prev;
    }

    *list = NULL;
}

void C_BSPList::Remove(long ID, BSPLIST **top)
{
    BSPLIST *cur, *delme;

    if (*top == NULL) 
        return;

    cur = *top;

    if ((*top)->ID == ID)
    {
        *top = cur->Next;
        delete cur->object;
        delete cur;
    }
    else
    {
        while (cur->Next not_eq NULL)
        {
            if (cur->Next->ID == ID)
            {
                delme = cur->Next;
                cur->Next = cur->Next->Next;
                delete delme->object;
                delete delme;
                return;
            }

            cur = cur->Next;
        }
    }
}

BOOL C_BSPList::FindLock(long ID)
{
    int i;

    for (i = 0; i < LockCount_; i++)
        if (LockList_[i] == ID)
            return(TRUE);

    return(FALSE);
}

void C_BSPList::Lock(long ID)
{
    // RED - Load is deferred to DX Engine that manages this automatically
    //DrawableBSP::LockAndLoad(ID);
    LockList_[LockCount_++] = ID;
}

void C_BSPList::RemoveLockList()
{
    int i;

    for (i = 0; i < LockCount_; i++)
        DrawableBSP::Unlock(LockList_[i]);

    LockCount_ = 0;
}

BSPLIST *C_BSPList::Find(long ID, BSPLIST *list)
{
    while (list not_eq NULL)
    {
        if (list->ID == ID)
            return(list);

        list = list->Next;
    }

    return(NULL);
}

BSPLIST *C_BSPList::Load(long ID, long objID)
{
    BSPLIST *obj;
    Tpoint objPos;
    Trotation objRot;

    obj = new BSPLIST;

    if (obj == NULL)
        return(NULL);

    if ( not FindLock(objID))
        Lock(objID);

    objPos.x = 0;
    objPos.y = 0;
    objPos.z = 0;
    objRot = IMatrix;
    obj->ID = ID;
    obj->type = BSP_DRAWBSP;
    obj->owner = NULL;
    obj->object = new DrawableBSP(objID, &objPos, &objRot, 1.0f);

    if (obj->object == NULL)
    {
        delete obj;
        return(NULL);
    }

    obj->Next = NULL;
    return(obj);
}

BSPLIST *C_BSPList::LoadBSP(long ID, long objID)
{
    BSPLIST *obj;
    Tpoint objPos;
    Trotation objRot;

    obj = new BSPLIST;

    if (obj == NULL)
        return(NULL);

    if ( not FindLock(objID))
        Lock(objID);

    objPos.x = 0;
    objPos.y = 0;
    objPos.z = 0;
    objRot = IMatrix;
    obj->ID = ID;
    obj->owner = NULL;
    obj->type = BSP_DRAWOBJECT;
    obj->object = new DrawableBSP(objID, &objPos, &objRot, 1.0f);

    if (obj->object == NULL)
    {
        delete obj;
        return(NULL);
    }

    obj->Next = NULL;
    return(obj);
}

BSPLIST *C_BSPList::LoadBridge(long, long)
{
    // BSPLIST *obj;
    // Tpoint objPos;
    // Trotation objRot;

    // obj=new BSPLIST;
    // if(obj == NULL)
    return(NULL);

    // if( not FindLock(objID))
    // Lock(objID);

    // obj->ID=ID;
    // obj->type=BSP_DRAWBRIDGE;
    // obj->owner=NULL;
    // obj->object=new DrawableBridge(objID,&objPos,&objRot,1.0f);
    // if(obj->object == NULL)
    // {
    // delete obj;
    // return(NULL);
    // }
    // obj->Next=NULL;
    // return(obj);
}

BSPLIST *C_BSPList::LoadBuilding(long ID, long objID, Tpoint *pos, float heading)
{
    BSPLIST *obj;

    obj = new BSPLIST;

    if (obj == NULL)
        return(NULL);

    if ( not FindLock(objID))
        Lock(objID);

    obj->ID = ID;
    obj->type = BSP_DRAWBUILDING;
    obj->owner = NULL;
    obj->object = new DrawableBuilding(objID, pos, heading, 1.0f);

    if (obj->object == NULL)
    {
        delete obj;
        return(NULL);
    }

    obj->Next = NULL;
    return(obj);
}

BSPLIST *C_BSPList::CreateContainer(long ID, Objective obj, short f, short fid, Falcon4EntityClassType *classPtr, FeatureClassDataType* fc)
{
    short    visType = -1;
    long prevFlags, nextFlags;
    BSPLIST *bspobj = NULL;

    visType = classPtr->visType[obj->GetFeatureStatus(f) bitand VIS_TYPE_MASK];

    if (visType >= 0)
    {
        // In many cases, our visType should be modified by our neighbors.
        if ((obj->GetFeatureStatus(f) bitand VIS_TYPE_MASK) not_eq VIS_DESTROYED and (FeatureEntryDataTable[fid].Flags bitand (FEAT_PREV_NORM bitor FEAT_NEXT_NORM)))
        {
            prevFlags = GetFeatureFlags(obj, static_cast<short>(f - 1));
            nextFlags = GetFeatureFlags(obj, static_cast<short>(f + 1));

            if (prevFlags and (prevFlags bitand FEAT_PREV_NORM) and (obj->GetFeatureStatus(f - 1) bitand VIS_TYPE_MASK) == VIS_DESTROYED)
            {
                if (nextFlags and (nextFlags bitand FEAT_NEXT_NORM) and (obj->GetFeatureStatus(f + 1) bitand VIS_TYPE_MASK) == VIS_DESTROYED)
                    visType = classPtr->visType[VIS_BOTH_DEST];
                else
                    visType = classPtr->visType[VIS_LEFT_DEST];
            }
            else if (nextFlags and (nextFlags bitand FEAT_NEXT_NORM) and (obj->GetFeatureStatus(f + 1) bitand VIS_TYPE_MASK) == VIS_DESTROYED)
                visType = classPtr->visType[VIS_RIGHT_DEST];
        }

        // Some things require Base Objects (like bridges and airbases)
        // if we have a parent...but NO drawable
        if (fc->Flags bitand FEAT_ELEV_CONTAINER)
        {
            // baseObject is the "container" object for all parts of the bridge
            // There is only one container for the entire bridge, stored in the lead element

            bspobj = new BSPLIST;

            if ( not bspobj)
                return(NULL);

            bspobj->ID = ID bitor 0x8000;

            bspobj->object = new DrawableBridge(1.0F);
            bspobj->type = FEAT_ELEV_CONTAINER;
            bspobj->Next = NULL;
            bspobj->owner = NULL;
        }
        // Is this a big flat thing with things on it (like an airbase?)
        else if (fc->Flags bitand FEAT_FLAT_CONTAINER)
        {
            // baseObject is the "container" object for all parts of the platform
            // There is only one container for the entire platform, stored in the
            // lead element.
            bspobj = new BSPLIST;

            if ( not bspobj)
                return(NULL);

            bspobj->ID = ID bitor 0x8000;

            bspobj->object = new DrawablePlatform(1.0F);
            bspobj->type = FEAT_FLAT_CONTAINER;
            bspobj->owner = NULL;
            bspobj->Next = NULL;
        }
    }

    return(bspobj);
}

BSPLIST *C_BSPList::LoadDrawableFeature(long ID, Objective obj, short f, short fid, Falcon4EntityClassType *classPtr, FeatureClassDataType* fc, Tpoint *objPos, BSPLIST *Parent)
{
    short    visType = -1;
    long prevFlags, nextFlags;
    BSPLIST *bspobj = NULL;
    float Yaw;

    Yaw = (float)FeatureEntryDataTable[fid].Facing * DEG_TO_RADIANS;

    visType = classPtr->visType[obj->GetFeatureStatus(f) bitand VIS_TYPE_MASK];

    if (visType >= 0)
    {
        // In many cases, our visType should be modified by our neighbors.
        if ((obj->GetFeatureStatus(f) bitand VIS_TYPE_MASK) not_eq VIS_DESTROYED and (FeatureEntryDataTable[fid].Flags bitand (FEAT_PREV_NORM bitor FEAT_NEXT_NORM)))
        {
            prevFlags = GetFeatureFlags(obj, static_cast<short>(f - 1));
            nextFlags = GetFeatureFlags(obj, static_cast<short>(f + 1));

            if (prevFlags and (prevFlags bitand FEAT_PREV_NORM) and (obj->GetFeatureStatus(f - 1) bitand VIS_TYPE_MASK) == VIS_DESTROYED)
            {
                if (nextFlags and (nextFlags bitand FEAT_NEXT_NORM) and (obj->GetFeatureStatus(f + 1) bitand VIS_TYPE_MASK) == VIS_DESTROYED)
                    visType = classPtr->visType[VIS_BOTH_DEST];
                else
                    visType = classPtr->visType[VIS_LEFT_DEST];
            }
            else if (nextFlags and (nextFlags bitand FEAT_NEXT_NORM) and (obj->GetFeatureStatus(f + 1) bitand VIS_TYPE_MASK) == VIS_DESTROYED)
                visType = classPtr->visType[VIS_RIGHT_DEST];
        }

        // Add another building to this grouping of buildings, or replace the drawable
        // of one which is here.
        // Is the container a bridge?
        if (Parent and Parent->type == FEAT_ELEV_CONTAINER)
        {
            // Make the new BRIDGE object
            if (visType)
            {
                bspobj = new BSPLIST;

                if ( not bspobj)
                    return(NULL);

                if ( not FindLock(visType))
                    Lock(visType);

                bspobj->ID = ID;
                bspobj->type = -1;
                bspobj->owner = Parent;
                bspobj->Next = NULL;

                if ((fc->Flags bitand FEAT_NEXT_IS_TOP) and (obj->GetFeatureStatus(f) bitand VIS_TYPE_MASK) not_eq VIS_DESTROYED)
                    bspobj->object = new DrawableRoadbed(visType, visType + 1, objPos, Yaw, 10.0f, static_cast<float>(atan(20.0f / 280.0f)));
                else
                    bspobj->object = new DrawableRoadbed(visType, -1, objPos, Yaw, 10.0f, static_cast<float>(atan(20.0f / 280.0f)));
            }

            if (bspobj->object)
            {
                ShiAssert(bspobj->object->GetClass() == DrawableObject::Roadbed);
                ((DrawableBridge*)Parent->object)->AddSegment((DrawableRoadbed*)bspobj->object);
            }
        }
        // Is the container a big flat thing (airbase)?
        else if (Parent and Parent->type == FEAT_FLAT_CONTAINER)
        {
            // Everything on a platform is a Building
            // That means it sticks straight up the -Z axis
            bspobj = new BSPLIST;

            if ( not bspobj)
                return(NULL);

            if ( not FindLock(visType))
                Lock(visType);

            bspobj->ID = ID;
            bspobj->type = -1;
            bspobj->owner = Parent;
            bspobj->Next = NULL;
            bspobj->object = new DrawableBuilding(visType, objPos, Yaw, 1.0F);

            // Am I Flat (can things drive across it)?
            if (fc->Flags bitand (FEAT_FLAT_CONTAINER bitor FEAT_ELEV_CONTAINER))
                ((DrawablePlatform*)Parent->object)->InsertStaticSurface((DrawableBuilding*)bspobj->object);
            else
                ((DrawablePlatform*)Parent->object)->InsertStaticObject(bspobj->object);
        }
        else
        {
            // if we get here then this is just a loose collection of buildings, like a
            // village or city, with no big flat objects between them

            bspobj = new BSPLIST;

            if ( not bspobj)
                return(NULL);

            if ( not FindLock(visType))
                Lock(visType);

            bspobj->ID = ID;
            bspobj->type = 0;
            bspobj->owner = NULL;
            bspobj->object = new DrawableBuilding(visType, objPos, Yaw, 1.0F);
            bspobj->Next = NULL;
        }
    }

    return(bspobj);
}

BSPLIST *C_BSPList::LoadDrawableUnit(long ID, long visType, Tpoint *objPos, float facing, uchar domain, uchar type, uchar stype)
{
    BSPLIST *bspobj;
    Trotation objRot;

    bspobj = new BSPLIST;

    if ( not bspobj)
        return(NULL);

    if ( not FindLock(visType))
        Lock(visType);

    bspobj->ID = ID;
    bspobj->type = 0;
    bspobj->owner = NULL;
    bspobj->Next = NULL;

    if (domain == DOMAIN_AIR and objPos->z < -10.0f)
    {
        Tpoint tmpPos;
        PositandOrientSetData(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, facing, &tmpPos, &objRot);
        // objRot->heading=facing;
        bspobj->object = new DrawableBSP(visType, objPos, &objRot, 1.0f);
        ((DrawableBSP*)bspobj->object)->SetSwitchMask(5, TRUE);

        if (visType == MapVisId(VIS_F16C))
            ((DrawableBSP*)bspobj->object)->SetSwitchMask(10, 1); // Afterburner
    }
    else
    {
        if (type == TYPE_FOOT and stype == STYPE_FOOT_SQUAD)
        {
            // Make the ground personel as desired
            bspobj->object = new DrawableGuys(visType, objPos, facing, 1, 1.0f);
        }
        else
        {
            // Make the ground vehicle as desired
            bspobj->object = new DrawableGroundVehicle(visType, objPos, facing, 1.0f);
        }
    }

    return(bspobj);
}
