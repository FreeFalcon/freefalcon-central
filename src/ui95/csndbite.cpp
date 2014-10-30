#include <windows.h>
#include <stdio.h>
#include "chandler.h"


C_SoundBite::C_SoundBite()
{
    Root = NULL;
}


C_SoundBite::~C_SoundBite()
{
    if (Root)
        Cleanup();
}

long C_SoundBite::Size()
{
    return(0);
}

void C_SoundBite::Setup()
{
    Root = NULL;
}

void C_SoundBite::Cleanup()
{
    CATLIST *cur, *last;
    SOUNDCAT *cursnd, *lastsnd;

    cur = Root;

    while (cur)
    {
        last = cur;
        cur = cur->Next;

        cursnd = last->Root;

        while (cursnd)
        {
            lastsnd = cursnd;
            cursnd = cursnd->Next;
            delete lastsnd;
        }

        delete last;
    }

    Root = NULL;
}

CATLIST *C_SoundBite::Find(long CatID)
{
    CATLIST *cur;

    cur = Root;

    while (cur)
    {
        if (cur->CatID == CatID)
            return(cur);

        cur = cur->Next;
    }

    return(NULL);
}

void C_SoundBite::Add(long CatID, long SoundID)
{
    CATLIST *cur;
    SOUNDCAT *sndlist, *newitem;

    newitem = new SOUNDCAT;
    newitem->Used = 0;
    newitem->SoundID = SoundID;
    newitem->Next = NULL;

    cur = Find(CatID);

    if (cur)
    {
        sndlist = cur->Root;

        while (sndlist->Next)
            sndlist = sndlist->Next;

        newitem->ID = sndlist->ID + 1;
        sndlist->Next = newitem;
        cur->Count++;
    }
    else
    {
        if ( not Root)
        {
            Root = new CATLIST;
            Root->CatID = CatID;
            Root->Count = 1;
            Root->Root = newitem;
            Root->Next = NULL;
            newitem->ID = 1;
        }
        else
        {
            cur = Root;

            while (cur->Next)
                cur = cur->Next;

            cur->Next = new CATLIST;
            cur = cur->Next;
            cur->CatID = CatID;
            cur->Count = 1;
            cur->Root = newitem;
            cur->Next = NULL;
            newitem->ID = 1;
        }
    }
}

long C_SoundBite::Pick(long CatID)
{
    CATLIST *cur;
    SOUNDCAT *snds;
    long num;

    cur = Find(CatID);

    if (cur)
    {
        snds = cur->Root;
        num = rand() % cur->Count;
        snds = cur->Root;

        while (snds and num > 0)
        {
            num--;
            snds = snds->Next;
        }

        if (snds)
        {
            if (snds->Used)
                return(0);

            snds->Used++;
            return(snds->SoundID);
        }
    }

    return(0);
}

long C_SoundBite::PickAlways(long CatID)
{
    CATLIST *cur;
    SOUNDCAT *snds;
    long num;

    cur = Find(CatID);

    if (cur)
    {
        snds = cur->Root;
        num = rand() % cur->Count;

        snds = cur->Root;

        while (snds and num > 0)
        {
            num--;
            snds = snds->Next;
        }

        if (snds)
        {
            snds->Used++;
            return(snds->SoundID);
        }
    }

    return(0);
}
