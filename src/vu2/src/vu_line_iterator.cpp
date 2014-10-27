#include "vu2.h"
#include "vu_priv.h"

//-----------------------------------------------------------------------------
// VuLineIterator
//-----------------------------------------------------------------------------

int foo_x1 = 0; // only to solve warnings

#if 0
VuLineIterator::VuLineIterator
(
    VuGridTree* coll,
    VuEntity*   origin,
    VuEntity*,
    BIG_SCALAR  radius
) : VuRBIterator(coll)
{
    curRB_ = 0;
    key1min_ = 0;
    key1max_ = static_cast<VU_KEY>(compl 0);
    key2min_ = 0;
    key2max_ = static_cast<VU_KEY>(compl 0);
    key1cur_ = 0;

    VU_KEY key1origin      = coll->filter_->Key1(origin);
    VU_KEY key2origin      = coll->filter_->Key2(origin);
    // VU_KEY key1destination = coll->filter_->Key1(destination);
    // VU_KEY key2destination = coll->filter_->Key2(destination);
    VU_KEY key1radius      = coll->filter_->Distance1(radius);
    VU_KEY key2radius      = coll->filter_->Distance2(radius);
    lineA_                 = 1;
    lineB_                 = 1;
    lineC_                 = 1;

    if (key1origin > key1radius)
    {
        key1min_ = key1origin - key1radius;
    }

    if (key2origin > key2radius)
    {
        key2min_ = key2origin - key2radius;
    }

    key1max_ = key1origin + key1radius;
    key2max_ = key2origin + key2radius;

    VuGridTree *gt = (VuGridTree *)collection_;

    if ( not gt->wrap_)
    {
        if (key1min_ < gt->bottom_)
        {
            key1min_ = gt->bottom_ + ((gt->bottom_ - key1min_) % gt->rowheight_);
        }

        if (key1max_ > gt->top_)
        {
            key1max_ = gt->top_;
        }
    }
}

VuLineIterator::VuLineIterator(
    VuGridTree* coll,
    BIG_SCALAR xPos0,
    BIG_SCALAR yPos0,
    BIG_SCALAR,
    BIG_SCALAR,
    BIG_SCALAR radius
) : VuRBIterator(coll)
{
    curRB_ = 0;
    key1min_ = 0;
    key1max_ = static_cast<VU_KEY>(compl 0);
    key2min_ = 0;
    key2max_ = static_cast<VU_KEY>(compl 0);
    key1cur_ = 0;
    VU_KEY key1origin      = coll->filter_->CoordToKey1(xPos0);
    VU_KEY key2origin      = coll->filter_->CoordToKey2(yPos0);
    // VU_KEY key1destination = coll->filter_->CoordToKey1(xPos1);
    // VU_KEY key2destination = coll->filter_->CoordToKey2(yPos1);
    VU_KEY key1radius      = coll->filter_->Distance1(radius);
    VU_KEY key2radius      = coll->filter_->Distance2(radius);
    lineA_ = 1;
    lineB_ = 1;
    lineC_ = 1;

    if (key1origin > key1radius)
    {
        key1min_ = key1origin - key1radius;
    }

    if (key2origin > key2radius)
    {
        key2min_ = key2origin - key2radius;
    }

    key1max_ = key1origin + key1radius;
    key2max_ = key2origin + key2radius;

    VuGridTree *gt = (VuGridTree *)collection_;

    if ( not gt->wrap_)
    {
        if (key1min_ < gt->bottom_)
        {
            key1min_ = gt->bottom_ + ((gt->bottom_ - key1min_) % gt->rowheight_);
        }

        if (key1max_ > gt->top_)
        {
            key1max_ = gt->top_;
        }
    }
}

VuLineIterator::~VuLineIterator()
{
    // empty
}


VuEntity *VuLineIterator::GetFirst()
{
    if (collection_)
    {
        curlink_ = vuTailNode;
        key1cur_ = key1min_;
        curRB_   = ((VuGridTree *)collection_)->Row(key1cur_);
        curnode_ = curRB_->root_;

        if (curnode_)
        {
            curnode_ = curnode_->LowerBound(key2min_);

            if (curnode_ and curnode_->head_ and curnode_->key_ < key2max_)
            {
                curlink_ = curnode_->head_;

                // sfr: smartpointer
                return curlink_->entity_.get();
            }
        }

        return GetNext();
    }

    return 0;
}

VuEntity *VuLineIterator::GetNext()
{
    // sfr: smartpointer
    while (curnode_ == 0 and key1cur_ < key1max_)
    {
        // danm_TBD: what about non-wrapping edges?
        key1cur_ += ((VuGridTree *)collection_)->rowheight_;
        curRB_ = ((VuGridTree *)collection_)->Row(key1cur_);
        curnode_ = curRB_->root_;

        if (curnode_)
        {
            curnode_ = curnode_->LowerBound(key2min_);

            if (curnode_ and curnode_->head_ and curnode_->key_ < key2max_)
            {
                curlink_ = curnode_->head_;

                return curlink_->entity_.get();
            }
        }
    }

    if (curnode_ == 0)
    {
        return 0;
    }

    curlink_ = curlink_->next_;

    if (curlink_ == vuTailNode)
    {
        curnode_ = curnode_->next_;

        if (curnode_ == 0 or curnode_->key_ > key2max_)
        {
            // skip to next row
            curlink_ = vuTailNode;
            curnode_ = 0;

            return GetNext();
        }

        curlink_ = curnode_->head_;
    }

    // sfr: smartpointer
    return curlink_->entity_.get();
}


VuEntity *VuLineIterator::GetFirst(VuFilter* filter)
{
    VuEntity* retval = GetFirst();

    if (retval == 0 or filter->Test(retval))
    {
        return retval;
    }

    return GetNext(filter);
}


VuEntity *VuLineIterator::GetNext(VuFilter* filter)
{
    VuEntity* retval = 0;

    while ((retval = GetNext()) not_eq 0)
    {
        if (filter->Test(retval))
        {
            return retval;
        }
    }

    return retval;
}

#endif
