#include "vu2.h"
#include "vu_priv.h"

int foo_x2 = 0;// only for solving warnings

#if not VU_ALL_FILTERED


VuFilteredList::VuFilteredList(VuFilter* filter) : VuLinkedList(),  filter_(filter->Copy())
{
}

VuFilteredList::~VuFilteredList()
{
    delete filter_;
}

VU_ERRCODE VuFilteredList::Handle(VuMessage* msg)
{
    VuScopeLock l(GetMutex());

    if (filter_->Notice(msg))
    {
        // list has to do with msg
        VuEntity* ent = msg->Entity();

        if (ent and filter_->RemoveTest(ent))
        {
            if (Find(ent->Id()))
            {
                if ( not filter_->Test(ent))
                {
                    // ent is in table, but doesn't belong there...
                    VuLinkedList::Remove(ent);
                }
            }
            else if (filter_->Test(ent))
            {
                // ent is not in table, but does belong there...
                VuLinkedList::Insert(ent);
            }

            return VU_SUCCESS;
        }
    }

    return VU_NO_OP;
}

VU_ERRCODE VuFilteredList::ForcedInsert(VuEntity *entity)
{
    if (entity == NULL)
    {
        return VU_NO_OP;
    }

    VuScopeLock l(GetMutex());

    if (filter_->RemoveTest(entity))
    {
        return VuLinkedList::Insert(entity);
    }

    return VU_NO_OP;
}

VU_ERRCODE VuFilteredList::Insert(VuEntity *entity)
{
    if (entity == NULL)
    {
        return VU_NO_OP;
    }

    VuScopeLock l(GetMutex());

    if (filter_->Test(entity))
    {
        return VuLinkedList::Insert(entity);
    }

    return VU_NO_OP;
}

VU_ERRCODE VuFilteredList::Remove(VuEntity *entity)
{
    if (entity == NULL)
    {
        return VU_NO_OP;
    }

    VuScopeLock l(GetMutex());

    if (filter_->RemoveTest(entity))
    {
        return VuLinkedList::Remove(entity);
    }

    return VU_NO_OP;
}

VU_ERRCODE VuFilteredList::Remove(VU_ID entityId)
{
    VuEntity* ent = vuDatabase->Find(entityId);

    if (ent)
    {
        return VuFilteredList::Remove(ent);
    }

    return VU_NO_OP;
}


VU_COLL_TYPE VuFilteredList::Type() const
{
    return VU_FILTERED_LIST_COLLECTION;
}

#endif
