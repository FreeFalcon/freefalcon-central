#include "vu2.h"
#include "vu_priv.h"

int foo_x3 = 0; // only for solving warnings

#if not VU_ALL_FILTERED

VuFilteredHashTable::VuFilteredHashTable(VuFilter *filter, unsigned int tableSize, uint key) :
    VuHashTable(tableSize, key), filter_(filter->Copy())
{
}


VuFilteredHashTable::~VuFilteredHashTable()
{
    delete filter_;
}

VU_ERRCODE VuFilteredHashTable::ForcedInsert(VuEntity *entity)
{
    if ( not filter_->RemoveTest(entity))
    {
        return VU_NO_OP;
    }

    return VuHashTable::Insert(entity);
}


VU_ERRCODE VuFilteredHashTable::Insert(VuEntity *entity)
{
    if ( not filter_->Test(entity))
    {
        return VU_NO_OP;
    }

    return VuHashTable::Insert(entity);
}

VU_ERRCODE VuFilteredHashTable::Handle(VuMessage *msg)
{
    if (filter_->Notice(msg))
    {
        VuEntity *ent = msg->Entity();

        if (ent and filter_->RemoveTest(ent))
        {
            if (Find(ent->Id()))
            {
                if ( not filter_->Test(ent))
                {
                    // ent is in table, but doesn't belong there...
                    Remove(ent);
                }
            }
            else if (filter_->Test(ent))
            {
                // ent is not in table, but does belong there...
                Insert(ent);
            }

            return VU_SUCCESS;
        }
    }

    return VU_NO_OP;
}

VU_COLL_TYPE VuFilteredHashTable::Type() const
{
    return VU_FILTERED_HASH_TABLE_COLLECTION;
}
#endif
