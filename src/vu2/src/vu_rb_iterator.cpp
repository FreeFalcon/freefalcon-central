#include "vu2.h"
#include "vu_priv.h"

VuRBIterator::VuRBIterator(VuRedBlackTree* coll) : VuIterator(coll)
{
    if (coll == NULL)
    {
        return;
    }

    curr_ = coll->map_.end();
}

VuRBIterator::~VuRBIterator()
{
}

VuEntity *VuRBIterator::GetFirst()
{
    if ( not collection_)
    {
        return NULL;
    }

    VuRedBlackTree *rbt = static_cast<VuRedBlackTree *>(collection_);
    curr_ = rbt->map_.begin();
    VuEntity *ret = CurrEnt();

    if (ret->VuState() not_eq VU_MEM_ACTIVE)
    {
        return GetNext();
    }
    else
    {
        return ret;
    }
}

VuEntity *VuRBIterator::GetFirst(VU_KEY low)
{
    if ( not collection_)
    {
        return NULL;
    }

    VuRedBlackTree *rbt = static_cast<VuRedBlackTree *>(collection_);
    curr_ = rbt->map_.lower_bound(low);

    if (curr_ == rbt->map_.end())
    {
        return NULL;
    }

    VuEntity *ret = CurrEnt();

    if (ret->VuState() not_eq VU_MEM_ACTIVE)
    {
        return GetNext();
    }
    else
    {
        return ret;
    }
}

VuEntity *VuRBIterator::GetNext()
{
    VuEntity *ret = NULL;
    if ( not collection_)
    {
        return NULL;
    }

    VuRedBlackTree *rbt = static_cast<VuRedBlackTree *>(collection_);
    if (curr_ not_eq rbt->map_.end())
    {
        ++curr_;
    }
    else
    {
        return ret;
    }
    ret = CurrEnt();

    if (ret not_eq NULL and ret->VuState() not_eq VU_MEM_ACTIVE)
    {
        return GetNext();
    }

    return ret;
}

VuEntity *VuRBIterator::CurrEnt()
{
    VuRedBlackTree *rbt = static_cast<VuRedBlackTree *>(collection_);

    if (curr_ == rbt->map_.end())
    {
        return NULL;
    }

    return curr_->second.get();
}

VU_ERRCODE VuRBIterator::Cleanup()
{
    VuRedBlackTree *rbt = static_cast<VuRedBlackTree *>(collection_);
    curr_ = rbt->map_.end();
    return VU_SUCCESS;
}
