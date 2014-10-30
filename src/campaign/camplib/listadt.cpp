#include <stddef.h>
#include "cmpglobl.h"
#include "vutypes.h"
#include "listadt.h"

#ifdef USE_SH_POOLS
MEM_POOL ListElementClass::pool;
MEM_POOL ListClass::pool;
#endif

// =============================
// List class functions
// =============================

ListElementClass::ListElementClass(short newKey, void *newData, uchar newFlags)
{
    key = newKey;
    user_data = newData;
    flags = newFlags;
    prev = next = NULL;
}

ListElementClass::~ListElementClass(void)
{
    if (flags bitand LADT_FREE_USER_DATA)
        delete user_data;
}

void ListElementClass::SetUserData(void *newData)
{
    if (user_data and (flags bitand LADT_FREE_USER_DATA))
        delete user_data;

    user_data = newData;
}

ListClass::ListClass(uchar newFlags)
{
    flags = newFlags;
    front = end = NULL;
}

ListClass::~ListClass(void)
{
    Purge();
}

void ListClass::Insert(ListElementClass *newElement)
{
    if ( not newElement)
        return;

    if ( not front)
    {
        ShiAssert( not end);
        front = newElement;
        end = newElement;
        ShiAssert(SanityCheck());
        return;
    }

    if ( not flags bitand LADT_SORTED_LIST)
    {
        newElement->next = front;
        front->prev = newElement;
        front = newElement;
        ShiAssert(SanityCheck());
        return;
    }
    else
    {
        // Check for end of list
        if (end->key <= newElement->key)
        {
            // Inserting at the end
            newElement->prev = end;

            if (end)
                end->next = newElement;

            end = newElement;
        }
        // Check for front of list
        else if (front->key >= newElement->key)
        {
            newElement->next = front;
            newElement->prev = NULL;
            front->prev = newElement;
            front = newElement;
        }
        // Insert somewhere in the middle
        else
        {
            ListNode current = front;

            while (current and current->key < newElement->key)
                current = current->next;

            ShiAssert(current not_eq front);
            newElement->prev = current->prev;
            newElement->next = current;

            if (current->prev)
                current->prev->next = newElement;

            current->prev = newElement;
        }
    }

    ShiAssert(SanityCheck());
}

void ListClass::InsertAtEnd(ListElementClass *newElement)
{
    if ( not newElement)
        return;

    if (flags bitand LADT_SORTED_LIST)
    {
        Insert(newElement);
        return;
    }

    newElement->prev = end;

    if (end)
        end->next = newElement;

    end = newElement;

    if ( not front)
        front = newElement;

    ShiAssert(SanityCheck());
}

void ListClass::Remove(ListElementClass *oldElement)
{
    if ( not oldElement)
        return;

    Detach(oldElement);
    delete oldElement;
    ShiAssert(SanityCheck());
}

void ListClass::Detach(ListElementClass *oldElement)
{
    if ( not oldElement)
        return;

    if (front == oldElement)
        front = oldElement->next;

    if (end == oldElement)
        end = oldElement->prev;

    if (oldElement->prev)
        oldElement->prev->next = oldElement->next;

    if (oldElement->next)
        oldElement->next->prev = oldElement->prev;

    ShiAssert(SanityCheck());
    oldElement->prev = NULL;
    oldElement->next = NULL;
}

void ListClass::InsertNewElement(short newKey, void *newData, uchar newFlags)
{
    ListNode newNode = new ListElementClass(newKey, newData, newFlags);

    Insert(newNode);
}

void ListClass::InsertNewElementAtEnd(short newKey, void *newData, uchar newFlags)
{
    ListNode newNode = new ListElementClass(newKey, newData, newFlags);

    InsertAtEnd(newNode);
}

void ListClass::Purge(void)
{
    ListNode current = front;

    ShiAssert(SanityCheck());

    while (current)
    {
        front = current->next;
        delete current;
        current = front;
    }

    front = end = NULL;
}

int ListClass::SanityCheck(void)
{
    ListElementClass *cur, *next;

    if ( not front and not end)
        return 1;

    if (front == end and (front->prev or front->next))
        return 0;

    if ((front and not end) or ( not front and end))
        return 0;

    if (front == (void*)0xdddddddd or front == (void*)0xfcfcfcfc)
        return 0;

    if (end == (void*)0xdddddddd or end == (void*)0xfcfcfcfc)
        return 0;

    cur = front;
    next = cur->next;

    while (cur and next)
    {
        if (next->prev not_eq cur)
            return 0;

        cur = next;
        next = cur->next;
    }

    cur = end;
    next = cur->prev;

    while (cur and next)
    {
        if (next->next not_eq cur)
            return 0;

        cur = next;
        next = cur->prev;
    }

    return 1;
}

