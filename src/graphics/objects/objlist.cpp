/***************************************************************************\
    ObjList.cpp
    Scott Randolph
    April 22, 1996

 Manage the list of active objects to be drawn each frame for a given
 renderer.
\***************************************************************************/
#include <math.h>
#include <windows.h>	// SEH (__try/__except, EXCEPTION_EXECUTE_HANDLER) to guard the update-callback
#include "grTypes.h"
#include "Matrix.h"
#include "ObjList.h"
#include "RenderOW.h"
#include "Falclib/Include/IsBad.h"
#include "FalcLib/include/dispopts.h" //JAM 04Oct03
#include "Graphics/DXEngine/DXEngine.h"
#include "Graphics/DXEngine/DXVBManager.h"
#include "DrawBsp.h"

#define _USE_OLD_SORT_ 1 // turn this off to make the sort 10 times slower...

/***************************************************************************\
 Clean up the object display list
\***************************************************************************/
ObjectDisplayList::ObjectDisplayList()
{
    head = NULL;
    tail = NULL;
    nextToDraw = NULL;
    updateCBlist = NULL;
    sortCBlist = NULL;
}


/***************************************************************************\
 Clean up the object display list
\***************************************************************************/
ObjectDisplayList::~ObjectDisplayList()
{
    // Commented out because it doesn't crash and we're desperate
    // ShiAssert ( not head );

    // KCK: This is kept around for shits and grins (and release, I guess)
    // #LEAK/EXIT HANG (stack: RViewPoint::Cleanup -> ~ObjectDisplayList:47 -> RemoveObject:141):
    // RemoveObject(head) for a node with parentList!=this (orphaned/bad, guard:140) RETURNS without
    // unlinking head -> head doesn't change -> while(head) is INFINITE -> hang on leaving 3D (worse
    // the more accumulated). Guarantee PROGRESS: if RemoveObject didn't unlink head, break the
    // chain manually (don't delete the node - it's already orphaned, owner frees/leaks it, but no hang).
    long _g = 0;

    while (head)
    {
        DrawableObject *h = head;
        RemoveObject(h);

        if (head == h)   // not unlinked -> force progress
        {
            if (F4IsBadReadPtr(h, sizeof(DrawableObject)))
            {
                head = NULL;   // head is garbage, can't walk the chain further
                break;
            }

            head = h->next;

            if (head and not F4IsBadWritePtr(head, sizeof(DrawableObject)))
                head->prev = NULL;

            h->prev = h->next = NULL;
        }

        if (++_g > 500000) break;   // absolute safety stop
    }

    tail = NULL;

    // Commented out because it doesn't crash and we're desperate
    // ShiAssert ( not head ); ShiAssert( not head );
    // ShiAssert( not tail );
}


// #9 ROOT FIX: a recursive critical section serializes access to the object lists between the
// RENDER thread (UpdateMetrics walk, incl. DrawableBridge prev/next walk) and the SIM/spawn
// thread (Insert/Remove as entities enter/leave view). Without it concurrent modification tears
// the prev/next chain -> cycle -> hang (previously only masked by a guard cap in drawbrdg.cpp).
// The CRITICAL_SECTION is RECURSIVE: a bridge-walk inside UpdateMetrics re-enters via
// parentList->RemoveObject / dynamicObjects.InsertObject on the SAME thread. Leaf lock (takes no
// other locks inside) -> no inversion/deadlock with the VU lock. DrawBeyond is not locked (it has
// its own F4IsBad guards + risk of inversion with VU during obj->Draw).
static CRITICAL_SECTION g_objListCS;
static struct ObjListCSInit { ObjListCSInit() { InitializeCriticalSection(&g_objListCS); } } s_objListCSInit;
struct ObjListLock { ObjListLock() { EnterCriticalSection(&g_objListCS); } ~ObjListLock() { LeaveCriticalSection(&g_objListCS); } };

/***************************************************************************\
 Add an instance of an object to the active display list
\***************************************************************************/
void ObjectDisplayList::InsertObject(DrawableObject *object)
{
    ObjListLock _lk;	// #9
    ShiAssert(object);
    ShiAssert( not object->InDisplayList());

    // #41/#54 ROOT of the DrawBeyond/OOM cycle: InsertObject was NOT idempotent - the re-insert
    // guard sat under a dead _SANITY_CHECK_ (defined nowhere). Re-inserting an already-linked node
    // (especially ==head) makes object->next=head=object -> a SELF-CYCLE in the list -> endless
    // DrawBeyond walk -> runaway new CDrawItem() -> hang+OOM. The "a node is in the list at most
    // once" invariant is now enforced UNCONDITIONALLY (like InsertUpdateCallbacks): already in some
    // list -> return (contract: unlink before inserting; the assert catches the culprit in Debug).
    if (object->InDisplayList())
        return;

    // Set up the links in the object
    object->prev = NULL;
    object->next = head;
    object->SetParentList(this);

    // Add the new entry at the head of this list
    if (head)
    {
        head->prev = object;
    }
    else
    {
        tail = object;
    }

    head = object;
#ifdef _SANITY_CHECK_

    if (head->prev not_eq NULL)
        head->prev = NULL;

#endif
}


/***************************************************************************\
 Remove an instance of an object from this display list
\***************************************************************************/
// #41/UAF ROOT (out-of-line destructor of base DrawableObject, declared in drawobj.h):
// an object deleted while LINKED (parentList != NULL) left dangling prev/next on its neighbours
// -> crashes walking the list (UpdateMetrics) and drawing (SafeDrawObject in DrawBeyond: reading
// null+0x14 / 0xDDDD..). The original only ASSERTed the intent "unlink before delete"; in Release
// it didn't unlink. We unlink ourselves: RemoveObject detaches prev/next under ObjListLock and
// calls SetParentList(NULL). Containers (Bridge/Platform) already remove themselves in their
// destructors (virtual SetParentList with callbacks + child promotion) -> arrive here with parentList==NULL.
DrawableObject::~DrawableObject()
{
    if (parentList)
        parentList->RemoveObject(this);
}

void ObjectDisplayList::RemoveObject(DrawableObject* object)
{
    ObjListLock _lk;	// #9
    // sfr: @todo remove JB checks
    if (F4IsBadReadPtr(this, sizeof(ObjectDisplayList))) // JB 010307 CTD
        return; // JB 010307 CTD

    if (F4IsBadReadPtr(object, sizeof(DrawableObject))) // JB 010221 CTD
        return; // JB 010221 CTD

    // PHASE 5: ENABLED unconditionally. A freed object (committed but filled with 0xDD) passes
    // F4IsBadReadPtr (memory is committed), but parentList=0xDDDDDDDD != this. This reliably
    // filters use-after-free (crash in SetParentList via a corrupt vtable).
    if ( not object->parentList or object->parentList not_eq this)
        return;

    // If we're removing the "nextToDraw" object, step to the next one
    if (object == nextToDraw)
    {
        nextToDraw = object->next;
    }

    // Take the given object out of the active list
    if (object->prev)
    {
        if ( not F4IsBadWritePtr(object->prev, sizeof(DrawableObject)))  // JB 010221 CTD
            object->prev->next = object->next;
    }
    else
    {
        ShiAssert(head == object);
        head = object->next;
    }

    if (object->next)
    {
        if ( not F4IsBadWritePtr(object->next, sizeof(DrawableObject)))  // JB 010221 CTD
            object->next->prev = object->prev;
    }
    else
    {
        ShiAssert(tail == object);
        tail = object->prev;
    }

    // Remove this objects links into the display list
    object->prev = object->next = NULL;
    object->SetParentList(NULL);
}


/*****************************************************************************\
 Compute the distance metrics for each entry in the display list
 given the new view point.  Entries below the bottom Z value given will
 be removed from the list and placed into the "lowList" chain.  Entries
 which are too high will go into the "highList" chain.

    this version ONLY calculates the distance for the objects in it's list
\*****************************************************************************/
void ObjectDisplayList::UpdateMetrics(const Tpoint *pos)
{
    ObjListLock _lk;	// #9
    register float x = pos->x;
    register float y = pos->y;
    // register float z = pos->z;

    DrawableObject *p;

    // Quit now if we don't have at least one list entry
    if ( not head) return;

    // Run through the whole list and compute the sorting metrics for each entry
    p = head;

    //while ( p )
    while (p and not F4IsBadReadPtr(p, sizeof(DrawableObject)))  // JB 010318 CTD
    {
        // #41 UAF: a node freed by the POOL without being unlinked stays COMMITTED and filled with
        // 0xDD -> F4IsBadReadPtr passes, but virtual p->Radius() goes through a corrupt vtable (AV).
        // A live node of this list has parentList == this; a freed one has 0xDDDDDDDD -> stop.
        if (p->parentList not_eq this) break;
        // Update the distance metric (not less than 0)
        p->distance = max((float)fabs(x - p->position.x), (float)fabs(y - p->position.y));
        ShiAssert( not _isnan(p->distance));

        if (_isnan(p->distance))
        {
            p->distance = 0.0f;
        }
        else if (p->distance > p->Radius())
        {
            p->distance = p->distance - p->Radius();
        }
        else
        {
            p->distance = 0.0f;
        }

        p = p->next;
    }
}

/*****************************************************************************\
 Compute the distance metrics for each entry in the display list
 given the new view point.  Entries below the bottom Z value given will
 be removed from the list and placed into the "lowList" chain.  Entries
 which are too high will go into the "highList" chain.
\*****************************************************************************/

// SEH wrapper around the update-callback call: self may become dangling (cross-thread UAF) - the
// campaign thread frees the DrawableBridge BETWEEN our check and the call (TOCTOU race), which
// F4IsBadReadPtr can't close. __try/__except catches the access violation regardless of the race
// -> the frame continues. Separate function with no C++ objects (otherwise C2712 on __try).
static void SafeCallUpdateCB(UpdateCallBack *nc, long listNo, const Tpoint *pos, TransportStr *transList)
{
    __try { nc->fn(nc->self, listNo, pos, transList); }
    __except (EXCEPTION_EXECUTE_HANDLER) { /* dangling self -> skip this callback for the frame */ }
}

// SEH wrapper around drawing an object: the object itself is intact, but a member pointer may be
// corrupt (overrun, e.g. 0xFFFD0016) -> Draw faults on the dereference. F4IsBad on the object
// pointer doesn't catch it (the object is valid). __try/__except swallows the AV -> frame
// continues, object skipped.
static bool SafeDrawObject(DrawableObject *obj, class RenderOTW *renderer, int lod)
{
    __try { obj->Draw(renderer, lod); return true; }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

void ObjectDisplayList::UpdateMetrics(long listNo, const Tpoint *pos, TransportStr *transList)
{
    ObjListLock _lk;	// #9 (recursive: a bridge-callback inside re-enters Remove/Insert/UpdateMetrics)
    register float x = pos->x;
    register float y = pos->y;
    // register float z = pos->z;

    DrawableObject *p;
    DrawableObject *q;

    long i;

    // Quit now if we don't have at least one list entry
    if ( not head) return;


#ifdef _SANITY_CHECK_

    if (head)
    {
        DrawableObject *_cur_;
        long count = 0;

        // Sanity checks
        if (head->parentList not_eq this)
            return;

        if (head->prev)
        {

            head->prev = NULL;
        }

        _cur_ = head;

        while (_cur_ and count < 10000)
        {
            if (_cur_->parentList not_eq this)
                return;

            _cur_ = _cur_->next;
            count++;
        }

        if (_cur_)
        {
            head->prev = NULL; // painless breakpoint
        }
    }

#endif

    // Run through the whole list and compute the sorting metrics for each entry
    q = head;

    long _iterGuard = 0;	// #41
    while (q)
    {
        // PHASE 5: guard against a dangling/freed node (heap corruption -> 0xDDDDDDDD).
        // F4IsBad catches unmapped/marker, parentList!=this catches freed (committed 0xDD).
        if (F4IsBadReadPtr(q, sizeof(DrawableObject)) or q->parentList not_eq this)
            break;

        // #41 HANG (caught via Parallel Stacks: SimLoopWrapper stuck here in
        // DrawableObject::Radius): a CYCLIC list (corrupt next -> q never NULL).
        // The checks above catch a bad/freed node, but NOT a cycle of VALID nodes. Iteration-count
        // guard: the real drawable count << 200000 -> exceeding it = cycle, break out
        // (instead of hanging the sim/render thread forever).
        if (++_iterGuard > 200000)
        {
            // no log available here (MonoPrint not linked); breaking the cycle = the hang fix
            OutputDebugStringA("ObjectDisplayList::UpdateMetrics: CYCLE in object list (>200k) -> break\n");
            break;
        }

        p = q;
        q = q->next;

        // Update the distance metric (not less than 0)
        p->distance = max((float)fabs(x - p->position.x), (float)fabs(y - p->position.y));
        ShiAssert( not _isnan(p->distance));

        if (_isnan(p->distance))
        {
            p->distance = 0.0f;
        }
        else if (p->distance > p->Radius())
        {
            p->distance = p->distance - p->Radius();
        }
        else
        {
            p->distance = 0.0f;
        }

        if (transList)
        {
            if (p->position.z >= transList->bottom[listNo] and listNo)
            {
                i = listNo - 1;

                while (i > 0 and p->position.z >= transList->bottom[i])
                    i--;

                // remove object from objectList
                RemoveObject(p);
                // head insert object into transport list
                p->next = transList->list[i];
                transList->list[i] = p;
            }
            else if (p->position.z < transList->top[listNo] and listNo < (_NUM_OBJECT_LISTS_ - 1))
            {
                i = listNo + 1;

                while (i < (_NUM_OBJECT_LISTS_ - 1) and p->position.z < transList->top[i])
                    i++;

                // remove object from objectList
                RemoveObject(p);
                // head insert object into transport list
                p->next = transList->list[i];
                transList->list[i] = p;
            }
        }
    }

#ifdef _SANITY_CHECK_

    if (head)
    {
        DrawableObject *_cur_;
        long count = 0;

        // Sanity checks
        if (head->prev)
        {
            head->prev = NULL;
        }

        _cur_ = head;

        while (_cur_ and count < 10000)
        {
            _cur_ = _cur_->next;
            count++;
        }

        if (_cur_)
        {
            head->prev = NULL; // painless breakpoint
        }
    }

#endif

    // Now call anyone who has registered for a callback
    long _cbGuard = 0;	// #41 HANG (caught in 22.png: SimLoopWrapper forever in PtrLooksBad): if
                        // updateCBlist is CYCLIC (a node ->next points back, e.g. double
                        // registration in InsertUpdateCallbacks), this for loop is infinite. F4IsBad
                        // catches a bad node, but NOT a cycle of VALID nodes. Iteration-count guard.
    for (UpdateCallBack *nextCall = updateCBlist; nextCall; nextCall = nextCall->next)
    {
        if (++_cbGuard > 100000) break;   // #41 cyclic callback list -> break instead of hanging
        if (F4IsBadReadPtr(nextCall, sizeof(UpdateCallBack))) break;   // bad node -> stop
        // #41 HANG ROOT (stack: SafeCallUpdateCB->DrawableBridge::UpdateMetrics, reading 0xDDDD..):
        // self (bridge/platform) may be freed by the POOL (MEM_POOL DrawableBridge::pool, bulk-free
        // WITHOUT a destructor -> callback NOT unregistered) or by the campaign thread. The memory
        // stays COMMITTED and filled with 0xDD, so the old F4IsBadReadPtr(self,4) MISSED it and the
        // call faulted on 0xDDDD.. inside -> AV swallowed by __except -> first-chance EVERY frame =
        // HANG under the debugger. Reliable check like in RemoveObject (objlist.cpp:140): a live
        // registered self is a DrawableObject whose parentList points AT THIS list. A freed one has
        // parentList == 0xDDDDDDDD != this -> the callback is orphaned -> skip (no call -> no AV -> no hang).
        if (F4IsBadReadPtr(nextCall->self, sizeof(DrawableObject))) continue;   // bad self -> skip
        if (((DrawableObject *)nextCall->self)->parentList not_eq this) continue; // orphaned/freed self
        SafeCallUpdateCB(nextCall, listNo, pos, transList);            // + SEH safety against the race
    }
}

void ObjectDisplayList::InsertionSortLink(DrawableObject **listhead, DrawableObject *listend)
{
    DrawableObject *newlist;
    DrawableObject *walk, *save;

    newlist = listend;
    walk = *listhead;

    for (; walk not_eq listend; walk = save)
    {
        DrawableObject **pnewlink;

        for (pnewlink = &newlist; *pnewlink not_eq listend and walk->distance <= (*pnewlink)->distance; pnewlink = &((*pnewlink)->next));

        save = walk->next;
        walk->next = *pnewlink;
        *pnewlink = walk;
    }

    *listhead = newlist;
}

void ObjectDisplayList::QuickSortLink(DrawableObject **head, DrawableObject *end)
{
    int left_count, right_count, count;
    DrawableObject **left_walk, *pivot, *old;
    DrawableObject **right_walk, *right;

    if (*head not_eq end)
    {
        do
        {
            pivot = *head;
            left_walk = head;
            right_walk = &right;
            left_count = right_count = 0;

            for (old = (*head)->next; old not_eq end; old = old->next)
            {
                if (old->distance > pivot->distance)
                {
                    left_count++;
                    *left_walk = old;
                    left_walk = &(old->next);
                }
                else
                {
                    right_count++;
                    *right_walk = old;
                    right_walk = &(old->next);
                }
            }

            *right_walk = end;
            *left_walk = pivot;
            pivot->next = right;

            if (left_count > right_count)
            {
                if (right_count >= 9)
                    QuickSortLink(&(pivot->next), end);
                else
                    InsertionSortLink(&(pivot->next), end);

                end = pivot;
                count = left_count;
            }
            else
            {
                if (left_count >= 9)
                    QuickSortLink(head, pivot);
                else
                    InsertionSortLink(head, pivot);

                head = &(pivot->next);
                count = right_count;
            }
        }
        while (count > 1);
    }
}

#ifndef _USE_OLD_SORT_
/*****************************************************************************\
 Sort the display list in far to near order.  It is assumed that
 the distances have already been computed through a call to
 UpdateMetrics.
\*****************************************************************************/
void ObjectDisplayList::SortForViewpoint(void)
{
    ObjListLock _lk;	// #41 CORRUPTION ROOT: this sort edits head/next/prev and walks sortCBlist
                        // from the RENDER thread WITHOUT the lock while the campaign thread under the
                        // lock does RemoveObject/Insert -> torn pointers (cycle -> hang #41 +
                        // dangling callbacks -> UAF). Same recursive CRITICAL_SECTION.
    DrawableObject *_cur_;
#ifdef _SANITY_CHECK_
    DrawableObject *_prev_;
    long count = 0;
#endif

    // Quit now if we don't have at least one list entry
    if ( not head) return;

#ifdef _SANITY_CHECK_

    // Sanity checks
    if (head->prev)
    {
        head->prev = NULL;
    }

    _cur_ = head;

    while (_cur_ and count < 10000)
    {
        _cur_ = _cur_->next;
        count++;
    }

    if (_cur_)
    {
        head->prev = NULL; // painless breakpoint
    }

#endif

    QuickSortLink(&head, NULL);

    if (head)
    {
        _cur_ = head->next;
        tail = head;
        head->prev = NULL;

        while (_cur_)
        {
            _cur_->prev = tail;
            tail = _cur_;
            _cur_ = _cur_->next;
        }

#ifdef _SANITY_CHECK_

        if (_cur_)
        {
            head->prev = NULL; // painless breakpoint
        }

#endif
    }

#ifdef _SANITY_CHECK_

    // Sanity checks
    if (head->prev)
    {
        head->prev = NULL;
    }

    _cur_ = head;
    count = 0;

    while (_cur_ and count < 10000)
    {
        _cur_ = _cur_->next;
        count++;
    }

    if (_cur_)
    {
        head->prev = NULL; // painless breakpoint
    }

#endif

    // Now call anyone who has registered for a callback
    for (SortCallBack *nextCall = sortCBlist; nextCall; nextCall = nextCall->next)
    {
        nextCall->fn(nextCall->self);
    }
}
#endif

#ifdef _USE_OLD_SORT_ // replacing this routine
/*****************************************************************************\
 Sort the display list in far to near order.  It is assumed that
 the distances have already been computed through a call to
 UpdateMetrics.
\*****************************************************************************/
void ObjectDisplayList::SortForViewpoint(void)
{
    DrawableObject *p;
    DrawableObject *q;


    // Quit now if we don't have at least one list entry
    if ( not head) return;

    // Now sort the list laterally from far to near
    for (p = head->next; p not_eq NULL; p = p->next)
    {

        // Decide where to place this element in the list
        q = p;

        while ((q->prev) and (q->prev->distance < p->distance)) // JB 010306 CTD
            //while ((q->prev) and ( not F4IsBadReadPtr(q->prev, sizeof(DrawableObject))) and (q->prev->distance < p->distance)) // JB 010306 CTD (too much CPU)
        {
            q = q->prev;
        }

        // Only adjust the list if we need to
        if (q not_eq p)
        {
            // Remove the element under consideration (p) from its current location
            if (p->prev)
            {
                p->prev->next = p->next;
            }
            else
            {
                head = p->next;
            }

            if (p->next)
            {
                p->next->prev = p->prev;
            }
            else
            {
                tail = p->prev;
            }

            // Insert the element under consideration in front of the identified element
            p->next = q;
            p->prev = q->prev;

            if (q->prev)
            {
                q->prev->next = p;
            }
            else
            {
                head = p;
            }

            q->prev = p;
        }
    }

    // Now call anyone who has registered for a callback
    for (SortCallBack *nextCall = sortCBlist; nextCall; nextCall = nextCall->next)
    {
        nextCall->fn(nextCall->self);
    }
}
#endif

// This function just preloads all objects in the List withing a range
// it exits when the list of ojects is fully loaded
void ObjectDisplayList::PreLoad(class RenderOTW *renderer)
{
    // for each object in list
    while (nextToDraw)
    {
        // PHASE 5: guard against a dangling/freed node. F4IsBad catches unmapped, parentList!=this
        // catches freed memory (committed, 0xDD). Can't continue (next is garbage too) -> return.
        if (F4IsBadReadPtr(nextToDraw, sizeof(DrawableObject)) or nextToDraw->parentList not_eq this)
        { nextToDraw = NULL; break; }

        // do the object (SEH: an object member may be corrupted by an overrun -> AV in Draw)
        SafeDrawObject(nextToDraw, renderer, -1);
        nextToDraw = nextToDraw->next;
    }
}



/*****************************************************************************\
 Draw all the objects in the display list which lie beyond the given ring
 Returns TRUE if any objects were actually drawn, FALSE otherwise.
\*****************************************************************************/
void ObjectDisplayList::DrawBeyond(float ringDistance, int LOD, class RenderOTW *renderer)
{
    //START_PROFILE("-->DRAW LIST");
    // #41/#54: this loop was the ONLY one without a back-link guard (cf. PreLoad:773). The
    // self-cycle root is closed in InsertObject (idempotency). Here we keep the same back-link guard
    // as in PreLoad - it catches a freed/foreign node (UAF 0xDD: committed memory, F4IsBadReadPtr
    // misses it, but parentList!=this rejects it). Can't continue - next is garbage too.
    while (nextToDraw and not F4IsBadReadPtr(nextToDraw, sizeof(DrawableObject)) and (nextToDraw->distance >= ringDistance))
    {
        if (nextToDraw->parentList not_eq this)
        { nextToDraw = NULL; break; }

        //COUNT_PROFILE("DRAW OBJECTS");
        // setup the object remove as false
        KillTheObject = RemoveTheObject = false;
        // do the object (SEH: an object member may be corrupted by an overrun -> AV in Draw)
        SafeDrawObject(nextToDraw, renderer, LOD);

        // List self management... if an object requests to be killed
        if (RemoveTheObject)
        {
            // keep the new item to draw
            DrawableObject *LastDrawn = nextToDraw->next;
            // remove the actual from list
            RemoveObject(nextToDraw);

            // if requests a deallocation, do it
            if (KillTheObject) delete nextToDraw;

            // get the pointer back
            nextToDraw = LastDrawn;
        }
        else
        {
            nextToDraw = nextToDraw->next;
        }
    }

    //STOP_PROFILE("-->DRAW LIST");
}

/*****************************************************************************\
 Draw all the objects in the display list which lie beyond the given ring
 Returns TRUE if any objects were actually drawn, FALSE otherwise.
\*****************************************************************************/
void ObjectDisplayList::DrawBeyond(float ringDistance, class Render3D *renderer)
{
    while (nextToDraw and not F4IsBadReadPtr(nextToDraw, sizeof(DrawableObject)) and (nextToDraw->distance >= ringDistance))
    {
        nextToDraw->Draw(renderer);
        nextToDraw = nextToDraw->next;
    }
}

/*****************************************************************************\
 Add a pair of functions to be called at UpdateMetrics and SortForViewpoint
 time.
\*****************************************************************************/
void ObjectDisplayList::InsertUpdateCallbacks(UpdateCallBack *up, SortCallBack *sort, void *self)
{
    ObjListLock _lk;	// #41 UAF ROOT: edits to updateCBlist/sortCBlist ran WITHOUT the lock while
                        // UpdateMetrics (under the lock) iterates these lists -> the campaign thread,
                        // destroying a DrawableBridge (SetParentList(NULL)->Remove), tore the list
                        // under the render thread -> dangling callback. Same CRITICAL_SECTION
                        // (recursive: a callback from UpdateMetrics re-enters here on the SAME thread -> no deadlock).
    ShiAssert(up);
    ShiAssert(sort);
    ShiAssert(self);

    // #41 HANG ROOT (22.png: SimLoopWrapper forever in PtrLooksBad): guard against DOUBLE insert.
    // If up is already in updateCBlist (a repeated SetParentList without unlink), a second prepend
    // makes up->next point back -> the list becomes CYCLIC -> UpdateMetrics hangs. If present -> return.
    {
        long g = 0;
        for (UpdateCallBack *p = updateCBlist; p; p = p->next)
        {
            if (p == up) return;            // already registered -> don't duplicate
            if (++g > 100000) break;        // list is already cyclic -> don't make it worse
        }
    }

    up->prev = NULL;
    up->next = updateCBlist;
    updateCBlist = up;

    sort->prev = NULL;
    sort->next = sortCBlist;
    sortCBlist = sort;

    if (updateCBlist->next)
    {
        ShiAssert(sortCBlist->next);
        updateCBlist->next->prev = updateCBlist;
        sortCBlist->next->prev  = sortCBlist;
    }
}


/*****************************************************************************\
 Remove a pair of functions to be called at UpdateMetrics and
 SortForViewpoint time.
\*****************************************************************************/
void ObjectDisplayList::RemoveUpdateCallbacks(UpdateCallBack *up, SortCallBack *sort, void *self)
{
    ObjListLock _lk;	// #41 UAF ROOT (see InsertUpdateCallbacks): unregistering a callback under the
                        // same lock as the walk in UpdateMetrics -> the campaign thread waits for the
                        // end of the render frame before removing the callback and destroying the bridge. Race closed.
    ShiAssert(up);
    ShiAssert(sort);
    ShiAssert(self);

    if (up->prev)
    {
        ShiAssert(sort->prev);
        up->prev->next = up->next;
        sort->prev->next = sort->next;
    }
    else
    {
        ShiAssert(up == updateCBlist);
        ShiAssert(sort == sortCBlist);
        updateCBlist = up->next;
        sortCBlist = sort->next;
    }

    if (up->next)
    {
        ShiAssert(sort->next);
        up->next->prev = up->prev;
        sort->next->prev = sort->prev;
    }
}

