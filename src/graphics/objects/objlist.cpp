/***************************************************************************\
    ObjList.cpp
    Scott Randolph
    April 22, 1996

 Manage the list of active objects to be drawn each frame for a given
 renderer.
\***************************************************************************/
#include <math.h>
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
    while (head)
    {
        RemoveObject(head);
    }

    // Commented out because it doesn't crash and we're desperate
    // ShiAssert ( not head ); ShiAssert( not head );
    // ShiAssert( not tail );
}


/***************************************************************************\
 Add an instance of an object to the active display list
\***************************************************************************/
void ObjectDisplayList::InsertObject(DrawableObject *object)
{
    ShiAssert(object);
    ShiAssert( not object->InDisplayList());

#ifdef _SANITY_CHECK_

    if (object->InDisplayList())
        return;

    if (object == head)
        return;

#endif
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
void ObjectDisplayList::RemoveObject(DrawableObject* object)
{
    // sfr: @todo remove JB checks
    if (F4IsBadReadPtr(this, sizeof(ObjectDisplayList))) // JB 010307 CTD
        return; // JB 010307 CTD

    if (F4IsBadReadPtr(object, sizeof(DrawableObject))) // JB 010221 CTD
        return; // JB 010221 CTD

#ifdef _SANITY_CHECK_

    if ( not object->parentList)
        return;

    if (object->parentList not_eq this)
        return;

#endif

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
void ObjectDisplayList::UpdateMetrics(long listNo, const Tpoint *pos, TransportStr *transList)
{
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

    while (q)
    {

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
    for (UpdateCallBack *nextCall = updateCBlist; nextCall; nextCall = nextCall->next)
    {
        nextCall->fn(nextCall->self, listNo, pos, transList);
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
        // do the object
        nextToDraw->Draw(renderer, -1);
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
    while (nextToDraw and (nextToDraw->distance >= ringDistance))
    {
        //COUNT_PROFILE("DRAW OBJECTS");
        // setup the object remove as false
        KillTheObject = RemoveTheObject = false;
        // do the object
        nextToDraw->Draw(renderer, LOD);

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
    while (nextToDraw and (nextToDraw->distance >= ringDistance))
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
    ShiAssert(up);
    ShiAssert(sort);
    ShiAssert(self);

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

