#include "vu2.h"
#include "vu_priv.h"

using namespace std;

/** vu collection manager global definition. */
VuCollectionManager* vuCollectionManager = 0;

VuCollectionManager::VuCollectionManager()
    : collcoll_(0), gridcoll_(0),
      gcMutex_(VuxCreateMutex("garbage collector mutex")),
      birthMutex_(VuxCreateMutex("birthlist mutex"))
{
    collsMutex_   = NULL;//VuxCreateMutex("colls mutex");
    gridsMutex_   = NULL;//VuxCreateMutex("grids mutex");;
}


VuCollectionManager::~VuCollectionManager()
{
    gclist_.clear();

    VuxDestroyMutex(gridsMutex_);
    VuxDestroyMutex(collsMutex_);
    VuxDestroyMutex(birthMutex_);
    VuxDestroyMutex(gcMutex_);
}

void VuCollectionManager::Register(VuCollection* coll)
{
    VuScopeLock l(collsMutex_);
#if VU_ALL_FILTERED
    collcoll_.push_back(coll);
#else

    if (coll not_eq vuDatabase)
    {
        collcoll_.push_back(coll);
    }

#endif
}


void VuCollectionManager::DeRegister(VuCollection* coll)
{
    if ( not this)
        return;

    VuScopeLock l(collsMutex_);
    collcoll_.remove(coll);
}


void VuCollectionManager::GridRegister(VuGridTree* grid)
{
    VuScopeLock l(gridsMutex_);
    gridcoll_.push_back(grid);
}


void VuCollectionManager::GridDeRegister(VuGridTree *grid)
{
    VuScopeLock l(gridsMutex_);
    gridcoll_.remove(grid);
}

void VuCollectionManager::Add(VuEntity* ent)
{
    VuScopeLock l(collsMutex_);

    for (list<VuCollection*>::iterator it = collcoll_.begin(); it not_eq collcoll_.end(); ++it)
    {
        VuCollection *c = *it;
#if VU_ALL_FILTERED
        c->Insert(ent);
#else

        if (c not_eq vuDatabase)
        {
            c->Insert(ent);
        }

#endif
    }
}


void VuCollectionManager::Remove(VuEntity* ent)
{
    VuScopeLock l(collsMutex_);
    // sfr: just to ensure it lives through all deletions
    VuBin<VuEntity> e(ent);

    for (list<VuCollection*>::iterator it = collcoll_.begin(); it not_eq collcoll_.end(); ++it)
    {
        VuCollection *c = *it;
#if VU_ALL_FILTERED
        c->Remove(ent);
#else

        if (c not_eq vuDatabase)
        {
            c->Remove(ent);
        }

#endif
    }
}


int VuCollectionManager::HandleMove(VuEntity*  ent, BIG_SCALAR coord1, BIG_SCALAR coord2)
{
    VuScopeLock l(gridsMutex_);
    int retval = 0;

    for (list<VuGridTree*>::iterator it = gridcoll_.begin(); it not_eq gridcoll_.end(); ++it)
    {
        VuGridTree *g = *it;

        if ( not g->suspendUpdates_)
        {
            g->Move(ent, coord1, coord2);
        }
    }

    return retval;
}


#define VU_NO_HANDLE 1

int VuCollectionManager::Handle(VuMessage* msg)
{
#if VU_NO_HANDLE
    return VU_NO_OP;
#else
    VuScopeLock l(collsMutex_);
    int retval = VU_NO_OP;

    for (list<VuCollection*>::iterator it = collcoll_.begin(); it not_eq collcoll_.end(); ++it)
    {
        VuCollection *c = *it;
#if VU_ALL_FILTERED

        if (c->Handle(msg) == VU_SUCCESS)
        {
            retval = VU_SUCCESS;
        }

#else

        if (c not_eq vuDatabase)
        {
            if (c->Handle(msg) == VU_SUCCESS)
            {
                retval = VU_SUCCESS;
            }
        }

#endif
    }

    return retval;
#endif // NO_HANDLE
}


int VuCollectionManager::FindEnt(VuEntity* ent)
{
    VuScopeLock l(collsMutex_);
    int retval = 0;

    for (list<VuCollection*>::iterator it = collcoll_.begin(); it not_eq collcoll_.end(); ++it)
    {
        VuCollection *c = *it;
#if VU_ALL_FILTERED

        if (c->Find(ent))
        {
            retval++;
        }

#else
        VuEntity *ent2 = c->Find(ent);

        if (ent2 and ent2 == ent)
        {
            retval++;
        }

#endif
    }

    return retval;
}

void VuCollectionManager::AddToBirthList(VuEntity *e)
{
    VuScopeLock l(birthMutex_);
    birthlist_.push_back(VuEntityBin(e));
}

void VuCollectionManager::AddToGc(VuEntity *e)
{
    VuScopeLock l(gcMutex_);
    gclist_.push_back(VuEntityBin(e));
}


void VuCollectionManager::CreateEntitiesAndRunGc()
{
    // create entities
    // at most max bper cycle
    const unsigned int bmax = 5;
    unsigned int bcount = 0;

    while ( not birthlist_.empty() and (bcount < bmax))
    {
        ++bcount;
        VuEntityBin &eb = birthlist_.front();
        vuDatabase->ReallyInsert(eb.get());
        birthlist_.pop_front();
    }

    // remove list nodes, allow at most dmax per cycle
    const unsigned int dmax = 5;
    unsigned int dcount = 0;

    while ( not gclist_.empty() and (dcount < dmax))
    {
        ++dcount;
        VuEntityBin &eb = gclist_.front();

        // some entities are removed and re-inserted in DB because of ID change.
        // when they are removed, they are collected, but they shouldnt be removed...
        if (eb->VuState() not_eq VU_MEM_ACTIVE)
        {
            vuDatabase->ReallyRemove(eb.get());
        }

        gclist_.pop_front();
    }

    //REPORT_VALUE("collected", count);
}

void VuCollectionManager::Shutdown(VU_BOOL all)
{
    // make last...
    //birthlist_.clear();
    //gclist_.clear();
    //vuDatabase->Suspend(all);
    VuScopeLock l(collsMutex_);
    // copy the list since the purges can change the registered collection structure
    list<VuCollection*> collcollCopy(collcoll_);

    for (list<VuCollection*>::iterator it = collcollCopy.begin(); it not_eq collcollCopy.end(); ++it)
    {
        VuCollection *c = *it;
#if VU_ALL_FILTERED
        c->Purge(all);
#else

        if (c not_eq vuDatabase)
        {
            c->Purge(all);
        }

#endif
    }

    collcollCopy.clear();

    // sfr: do at the end to avoid self destructions up there
    birthlist_.clear();
    gclist_.clear();
    vuDatabase->Suspend(all);

    // clear the collections? Dont, some exist during whole game time.
    //collcoll_.clear();
    //gridcoll_.clear();
}


