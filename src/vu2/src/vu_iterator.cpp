#include "vu2.h"
#include "vu_priv.h"

VuIterator::VuIterator(VuCollection *coll) : collection_(coll)
{
    if (collection_ not_eq NULL)
    {
        VuMutex m = collection_->GetMutex();

        if (m not_eq NULL)
        {
            VuxLockMutex(m);
        }
    }
}

VuIterator::~VuIterator()
{
    if (collection_ not_eq NULL)
    {
        VuMutex m = collection_->GetMutex();

        if (m not_eq NULL)
        {
            VuxUnlockMutex(m);
        }
    }
}

VU_ERRCODE VuIterator::Cleanup()
{
    // by default, do nothing
    return VU_SUCCESS;
}
