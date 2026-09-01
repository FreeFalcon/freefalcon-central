/***************************************************************************
    DrawBrdg.cpp    Scott Randolph
    July 29, 1997

    Derived class to do special position and containment processing for
 bridges (platforms upon which ground vehicles can drive).
***************************************************************************/

#include "rviewpnt.h"
#include "renderow.h"
#include "drawgrnd.h"
#include "drawbrdg.h"
#include "falclib/include/isbad.h" // #41 F4IsBadReadPtr to guard a dangling neighbor

#ifdef USE_SH_POOLS
MEM_POOL DrawableBridge::pool;
#endif


/***************************************************************************
    Initialize a container for a BSP object to be drawn
***************************************************************************/
DrawableBridge::DrawableBridge(float s) : DrawableObject(s)
{
    drawClassID = Bridge;

    // Set this object's height to sea level to ensure we're in the lowest object list
    position.z = 0.0f;
    previousLOD = -1;

    // Start with no included area -- area will be added as segments are added.
    radius = 0.0f;
    InclusionRadiusSquared = 0.0f;
    maxX = maxY = -1e24f;
    minX = minY = 1e24f;

    // Fill in our callback request structures for when we get added to a parent list
    updateCBstruct.fn = UpdateMetrics;
    updateCBstruct.self = this;
    updateCBstruct.next = NULL;
    updateCBstruct.prev = NULL;
    sortCBstruct.fn = SortForViewpoint;
    sortCBstruct.self = this;
    sortCBstruct.next = NULL;
    sortCBstruct.prev = NULL;

    // Setup the contained object lists
    roadbedObjects.Setup();
    dynamicObjects.Setup();
}


/***************************************************************************
    Remove an instance of a brige object.
 All the children must be removed before this happens.  The dynamic
 children are handled when SetParentList( NULL ) happens upon removal
 of this object from the display list.  The static objects must be
 manually removed before this point.
***************************************************************************/
DrawableBridge::~DrawableBridge(void)
{
    // #41 UAF ROOT (stack: SafeCallUpdateCB->DrawableBridge::UpdateMetrics, reading 0xDDDD..):
    // the bridge was deleted while left LINKED in the parent's prev/next -> neighbors held a dangling
    // pointer to the freed bridge -> AV (prev->distance, offset 0x28). The old SetParentList(NULL)
    // only removed callbacks and promoted children, but did NOT unlink the bridge from prev/next.
    // RemoveObject does BOTH the prev/next unlink (under ObjListLock, with F4 guards) AND calls
    // the virtual SetParentList(NULL) (callbacks + child promotion) -- full removal in one call.
    if (parentList)
        parentList->RemoveObject(this);

    ShiAssert(parentList == NULL);

    // Mark this object as finished (no area contained)
    InclusionRadiusSquared = -1.0f;

    // Clean up our list (must be empty at this point)
    roadbedObjects.Cleanup();
    dynamicObjects.Cleanup();
}


/***************************************************************************
    Add a segment (both under and over parts) to the bridge.
***************************************************************************/
void DrawableBridge::AddSegment(DrawableRoadbed *piece)
{
    Tpoint max, min;

    ShiAssert(piece);

    min.x = min.z = -(max.x = max.z = piece->Radius());

    // Update our center point and inclusion radius
    maxX = max(maxX, max.z + piece->position.x);
    minX = min(minX, min.z + piece->position.x);
    maxY = max(maxY, max.x + piece->position.y);
    minY = min(minY, min.x + piece->position.y);

    position.x = (maxX + minX) * 0.5f;
    position.y = (maxY + minY) * 0.5f;

    InclusionRadiusSquared = (maxX - position.x) * (maxX - position.x) +
                             (maxY - position.y) * (maxY - position.y);
    radius = (float)sqrt(InclusionRadiusSquared);


    // Add the two drawable pieces of this new segment
    roadbedObjects.InsertObject(piece);
}


/***************************************************************************
    Replace a piece of a bridge (used for damage, etc.)
 Note that this will, in fact, replace ANY drawable object in ANY list.
***************************************************************************/
void DrawableBridge::ReplacePiece(DrawableRoadbed *oldPiece,
                                  DrawableRoadbed *newPiece)
{
    ShiAssert(oldPiece);
    ShiAssert(oldPiece->InDisplayList());

    if (newPiece)
    {
        ShiAssert(not newPiece->InDisplayList());
        oldPiece->parentList->InsertObject(newPiece);
    }

    oldPiece->parentList->RemoveObject(oldPiece);
}


/***************************************************************************\
    Return the altitude of (and optionally, the normal to) the surface of
 the bridge at the provided location.
\***************************************************************************/
float DrawableBridge::GetGroundLevel(float x, float y, Tpoint *normal)
{
    DrawableRoadbed *roadbed;
    Tpoint pos;

    pos.x = x;
    pos.y = y;

    // Find the right segment to govern this point
    // (Note:  The cast below is safe because only roadbeds get added to this list)
    roadbedObjects.ResetTraversal();
    roadbed = (DrawableRoadbed *)roadbedObjects.GetNextAndAdvance();

    while (roadbed)
    {
        ShiAssert(roadbed->GetClass() == Roadbed);

        if (roadbed->OnRoadbed(&pos, normal))
        {
            return pos.z;
        }

        roadbed = (DrawableRoadbed *)roadbedObjects.GetNextAndAdvance();
    }

    // We didn't find a containing segment
    // We used to assert here, but in instant action, we let tanks go on water right now...
    // So, we use the position of the container object (presumably ground level)
    if (normal)
    {
        normal->x = 0.0f;
        normal->y = 0.0f;
        normal->z = -1.0f;
    }

    return position.z;
}


/***************************************************************************\
    Make sure the object is placed on the ground then draw it.
\***************************************************************************/
void DrawableBridge::Draw(class RenderOTW *renderer, int LOD)
{
    DrawableObject *obj;

    // See if we need to update our ground position
    if (LOD not_eq previousLOD)
    {

        // Update our position to reflect the terrain beneath us
        position.z =
            renderer->viewpoint->GetGroundLevel(position.x, position.y);
        previousLOD = LOD;

        // Stick all our children at the same height
        roadbedObjects.ResetTraversal();
        obj = roadbedObjects.GetNextAndAdvance();

        while (obj)
        {
            ShiAssert(obj->GetClass() == Roadbed);
            ((DrawableRoadbed *)obj)->ForceZ(position.z);
            obj = roadbedObjects.GetNextAndAdvance();
        }
    }

    // First draw the raodbed (should all be DrawableRoadbed)
    roadbedObjects.ResetTraversal();
    obj = roadbedObjects.GetNextAndAdvance();

    while (obj)
    {
        obj->Draw(renderer, LOD);
        obj = roadbedObjects.GetNextAndAdvance();
    }

    // Then draw the dynamic objects (should all be DrawableGroundVehicle)
    dynamicObjects.ResetTraversal();
    obj = dynamicObjects.GetNextAndAdvance();

    while (obj)
    {
        obj->Draw(renderer, LOD);
        obj = dynamicObjects.GetNextAndAdvance();
    }

    // Finally draw the superstructure
    roadbedObjects.ResetTraversal();
    obj = roadbedObjects.GetNextAndAdvance();

    while (obj)
    {
        ShiAssert(obj->GetClass() == Roadbed);
        ((DrawableRoadbed *)obj)->DrawSuperstructure(renderer, LOD);
        obj = roadbedObjects.GetNextAndAdvance();
    }
}


/***************************************************************************\
    Draw the children of this object (no ground leveling is done).
\***************************************************************************/
void DrawableBridge::Draw(class Render3D *renderer)
{
    DrawableObject *obj;

    // First draw the raodbed (should all be DrawableRoadbed)
    roadbedObjects.ResetTraversal();
    obj = roadbedObjects.GetNextAndAdvance();

    while (obj)
    {
        obj->Draw(renderer);
        obj = roadbedObjects.GetNextAndAdvance();
    }

    // Then draw the dynamic objects (should all be DrawableGroundVehicle)
    dynamicObjects.ResetTraversal();
    obj = dynamicObjects.GetNextAndAdvance();

    while (obj)
    {
        obj->Draw(renderer);
        obj = dynamicObjects.GetNextAndAdvance();
    }

    // Finally draw the superstructure
    roadbedObjects.ResetTraversal();
    obj = roadbedObjects.GetNextAndAdvance();

    while (obj)
    {
        ShiAssert(obj->GetClass() == Roadbed);
        ((DrawableRoadbed *)obj)->DrawSuperstructure(renderer);
        obj = roadbedObjects.GetNextAndAdvance();
    }
}


/***************************************************************************
    Handle the UpdateMetrics callback from the parent ObjectDisplayList
***************************************************************************/
void DrawableBridge::UpdateMetrics(void *self, long listNo, const Tpoint *pos,
                                   TransportStr *transList)
{
    ((DrawableBridge *)self)->UpdateMetrics(listNo, pos, transList);
}
void DrawableBridge::UpdateMetrics(long listNo, const Tpoint *pos,
                                   TransportStr *transList)
{
    DrawableObject *obj;
    DrawableObject *objNext;
    float checkDistance;
    long
        _g; // #41 guard against a looped list (hang in DrawableBridge::UpdateMetrics)


    // Have the object lists update their sort metrics.
    // (This could be sped up alot by presorting the bridge and reversing the order only when
    //  the viewer crosses into the opposite hemisphere with respect to the bridges yaw.)
    roadbedObjects.UpdateMetrics(pos);
    dynamicObjects.UpdateMetrics(listNo, pos, transList);

    // Check each dynamic object for continued membership in this container
    // (could hold this out of the parent list until after we check for
    //  new members, but this difference should normally be very small)
    dynamicObjects.ResetTraversal();
    obj = dynamicObjects.GetNextAndAdvance();

    _g = 0;
    while (obj)
    {
        if (++_g > 100000)
            break; // #41 guard: looped dynamicObjects -> break instead of hanging
        // Push the object back up to our parent list if it has moved beyond our area
        if (not ObjectInside(obj))
        {
            ShiAssert(obj->GetClass() == GroundVehicle);
            dynamicObjects.RemoveObject(obj);
            parentList->InsertObject(obj);
        }

        obj = dynamicObjects.GetNextAndAdvance();
    }


    // Check behind us in our parent list for new dynamic objects in our area
    checkDistance = distance + 2.0f * Radius();
    obj = prev;

    _g = 0;
    while (obj)
    {
        if (++_g > 100000)
            break; // #41 guard: looped parent list (prev) -> break
        // #41 UAF ROOT: the neighbor may have been freed CONCURRENTLY (campaign thread) via a path not
        // taking ObjListLock (TOCTOU, see objlist.cpp SafeCallUpdateCB). The freed one is filled
        // with 0xDD: memory is committed (F4IsBadReadPtr passes), BUT parentList != our list. The same
        // robust check as in RemoveObject (objlist.cpp:140). A dead neighbor -> the whole chain
        // (objNext = obj->prev) is garbage -> abort the traversal (previously it crashed reading obj->distance,
        // swallowed by __except -> first-chance every frame = HANG under a debugger).
        if (F4IsBadReadPtr(obj, sizeof(DrawableObject)) or
            obj->parentList not_eq parentList)
            break;
        if (not(obj->distance < checkDistance))
            break;
        // Pull object down from our parent list if it's inside our area
        objNext = obj->prev;

        if (ObjectInside(obj) and (obj->GetClass() == GroundVehicle))
        {
            parentList->RemoveObject(obj);
            dynamicObjects.InsertObject(obj);
            ((DrawableGroundVehicle *)obj)->SetUpon(this);
        }

        obj = objNext;
    }


    // Check in front of us in our parent list for new dynamic objects in our area
    checkDistance = distance;
    obj = next;

    _g = 0;
    while (obj)
    {
        if (++_g > 100000)
            break; // #41 guard: looped parent list (next) -> break
        // #41 UAF: the same dangling-neighbor guard as for the prev traversal above.
        if (F4IsBadReadPtr(obj, sizeof(DrawableObject)) or
            obj->parentList not_eq parentList)
            break;
        if (not(obj->distance > checkDistance))
            break;
        // Pull object down from our parent list if it's inside our area
        objNext = obj->next;

        if (ObjectInside(obj) and (obj->GetClass() == GroundVehicle))
        {
            parentList->RemoveObject(obj);
            dynamicObjects.InsertObject(obj);
            ((DrawableGroundVehicle *)obj)->SetUpon(this);
        }

        obj = objNext;
    }
}


/***************************************************************************\
    Handle the SortForViewpoint callback from the parent ObjectDisplayList
\***************************************************************************/
void DrawableBridge::SortForViewpoint(void *self)
{
    ((DrawableBridge *)self)->SortForViewpoint();
}
void DrawableBridge::SortForViewpoint(void)
{
    // Have the object lists reorder themselves.
    // (This could be sped up alot by presorting the bridge and reversing the order only when
    //  the viewer crosses the into the opposite hemisphere with respect to the bridges yaw.)
    roadbedObjects.SortForViewpoint();
    dynamicObjects.SortForViewpoint();
}


/***************************************************************************\
    See if an object is within our area, and if so, grab it from it's
 parent.
\***************************************************************************/
BOOL DrawableBridge::ObjectInside(DrawableObject *obj)
{
    Tpoint objPos;

    ShiAssert(obj);

    // Get the position of the object under consideration
    obj->GetPosition(&objPos);

    // return our conclusion
    return ((objPos.x >= minX) and (objPos.x <= maxX) and (objPos.y >= minY) and
            (objPos.y <= maxY));
}


/***************************************************************************\
    Add ourselves to our parent list and request callbacks
\***************************************************************************/
void DrawableBridge::SetParentList(ObjectDisplayList *list)
{
    DrawableObject *obj;

    // Remove ourselves from our old parent's call chain
    if (parentList)
    {
        parentList->RemoveUpdateCallbacks(&updateCBstruct, &sortCBstruct, this);

        // Promote all dynamnic children back up into our old parent's list
        dynamicObjects.ResetTraversal();
        obj = dynamicObjects.GetNextAndAdvance();

        while (obj)
        {
            dynamicObjects.RemoveObject(obj);
            parentList->InsertObject(obj);
            obj = dynamicObjects.GetNextAndAdvance();
        }
    }

    // Record our new parent
    parentList = list;

    // Add ourselves to our new parent's call chain
    if (parentList)
    {
        parentList->InsertUpdateCallbacks(&updateCBstruct, &sortCBstruct, this);
    }
}
