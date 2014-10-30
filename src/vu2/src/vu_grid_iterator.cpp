#include <cISO646>
#include "vu_iterator.h"
#include "vu_filter.h"

VuGridIterator::VuGridIterator(VuGridTree* coll, BIG_SCALAR p1, BIG_SCALAR p2, BIG_SCALAR radius) :
    p1_(p1), p2_(p2), radius_(radius), it_(static_cast<VuRedBlackTree*>(NULL)), VuIterator(coll)
{
    // sfr: temp test
    //static int temp = 1;
    //radius *= temp;

    // low and up row (inclusive)
#if VU_ALL_FILTERED
    VuBiKeyFilter *bkf = coll->GetBiKeyFilter();
#else
    VuBiKeyFilter *bkf = coll->filter_;
#endif
    rowlow_ = bkf->CoordToKey(p1 - radius);
    rowhi_  = bkf->CoordToKey(p1 + radius);
    // low and up coll
    collow_ = bkf->CoordToKey(p2 - radius);
    colhi_  = bkf->CoordToKey(p2 + radius);
}

VuGridIterator::~VuGridIterator()
{
}

VuEntity *VuGridIterator::GetFirst()
{

    if ( not collection_)
    {
        return NULL;
    }

    VuGridTree *g = static_cast<VuGridTree*>(collection_);
#if VU_ALL_FILTERED
    VuBiKeyFilter *bkf = g->GetBiKeyFilter();
#else
    VuBiKeyFilter *bkf = g->filter_;
#endif
    rowcur_ = rowlow_;
    it_ = VuRBIterator(g->table_[rowcur_]);
    VuEntity *ret = it_.GetFirst(collow_);

    if ((ret == NULL) or (bkf->Key2(ret) > colhi_))
    {
        ret =  GetNext();
    }

    return ret;
}

VuEntity *VuGridIterator::GetNext()
{
    VuGridTree *g = static_cast<VuGridTree*>(collection_);
#if VU_ALL_FILTERED
    VuBiKeyFilter *bkf = g->GetBiKeyFilter();
#else
    VuBiKeyFilter *bkf = g->filter_;
#endif
    VuEntity *ret = it_.GetNext();;

    while ((ret == NULL) or (bkf->Key2(ret) > colhi_))
    {
        // end of column
        if (rowcur_ + 1 > rowhi_)
        {
            // last row... end
            return NULL;
        }
        else
        {
            // next row
            it_ = VuRBIterator(g->table_[++rowcur_]);
            ret = it_.GetFirst(collow_);
        }
    }

    return ret;
}

VuEntity *VuGridIterator::GetFirst(VuFilter* filter)
{
    VuEntity* retval = GetFirst();

    if (retval == 0 or filter->Test(retval))
    {
        return retval;
    }

    return GetNext(filter);
}

VuEntity *VuGridIterator::GetNext(VuFilter *filter)
{
    VuEntity* retval = 0;

    while ((retval = GetNext()) not_eq 0)
    {
        if (filter->Test(retval))
        {
            break;
        }
    }

    return retval;
}

VuEntity *VuGridIterator::CurrEnt()
{
    return it_.CurrEnt();
}
