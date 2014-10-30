#include "vu2.h"
//#include "vu_priv.h"

VuListIterator::VuListIterator(VuLinkedList* coll) : VuIterator(coll)
{
    if (coll not_eq NULL)
    {
        curr_ = coll->l_.end();
    }
}

VuListIterator::~VuListIterator()
{
}

void VuListIterator::RemoveCurrent()
{
    VuLinkedList::VuEntityBinList bl = static_cast<VuLinkedList*>(collection_)->l_;

    if (curr_ == bl.end())
    {
        return;
    }

    bl.erase(curr_);
}

VuEntity *VuListIterator::GetFirst()
{
    if ( not collection_)
    {
        return NULL;
    }

    VuLinkedList *vl = static_cast<VuLinkedList*>(collection_);

    if (vl->l_.empty())
    {
        return NULL;
    }

    curr_ = vl->l_.begin();
    VuEntityBin &eb = *curr_;

    if (eb->VuState() not_eq VU_MEM_ACTIVE)
    {
        return GetNext();
    }
    else
    {
        return *eb;
    }
}

VuEntity *VuListIterator::GetNext()
{
    VuLinkedList *vl = static_cast<VuLinkedList*>(collection_);

    do
    {
        if (++curr_ == vl->l_.end())
        {
            return NULL;
        }
        else if ((*curr_)->VuState() not_eq VU_MEM_ACTIVE)
        {
            continue;
        }
        else
        {
            return curr_->get();
        }
    }
    while (1);
}

VuEntity *VuListIterator::GetFirst(VuFilter* filter)
{
    if (collection_)
    {
        return NULL;
    }

    VuEntity *e = GetFirst();

    if (e == NULL)
    {
        return NULL;
    }
    else if (( not filter) or filter->Test(e))
    {
        return e;
    }
    else
    {
        return GetNext(filter);
    }
}

VuEntity *VuListIterator::GetNext(VuFilter *filter)
{
    VuEntity *e;

    do
    {
        e = GetNext();

        if ( not filter or filter->Test(e))
        {
            return e;
        }
    }
    while (e not_eq NULL);

    return NULL;
}

VuEntity *VuListIterator::CurrEnt()
{
    return curr_->get();
}

VU_ERRCODE VuListIterator::Cleanup()
{
    if (collection_)
    {
        curr_ = static_cast<VuLinkedList*>(collection_)->l_.end();
    }

    return VU_SUCCESS;
}


