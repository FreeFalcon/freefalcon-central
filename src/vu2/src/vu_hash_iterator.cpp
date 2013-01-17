#include "vu2.h"
#include "vu_priv.h"

//-----------------------------------------------------------------------------
// VuHashIterator
//-----------------------------------------------------------------------------

VuHashIterator::VuHashIterator(VuHashTable* coll) : 
	VuIterator(coll), 
	idx_(coll->capacity_),
	curr_(NULL)
{
#if 0
	curr_ = vuTailNode;
	entry_ = 0;
#endif

//	vuCollectionManager->Register(this);
}

VuHashIterator::~VuHashIterator()
{
//	vuCollectionManager->DeRegister(this);
}


VuEntity *VuHashIterator::GetFirst(){
	VuHashTable *h = static_cast<VuHashTable*>(collection_);
	if (!h || h->capacity_ <= 0){ return NULL; }
	idx_ = 0;
	VuEntity *ret;
	do {
		curr_ = VuListIterator(&h->table_[idx_]);
		ret = curr_.GetFirst();
	} while (ret == NULL && ++idx_ < h->capacity_);
	return ret;
	
#if 0
	if (collection_)
	{
		entry_ = ((VuHashTable*)collection_)->table_;
		
		while (*entry_ == vuTailNode)
		{
			entry_++;
		}

		if (*entry_ == 0)
		{
			curr_ = vuTailNode;
		}
		else
		{
			curr_ = *entry_;
		}

		// sfr: smartpointer
		return curr_->entity_.get();
	}

	return 0;
#endif
}

VuEntity *VuHashIterator::GetNext(){
	VuHashTable *h = static_cast<VuHashTable*>(collection_);
	VuEntity *ret;
	do {
		// try next
		ret = curr_.GetNext();
		while (ret == NULL && ++idx_ < h->capacity_){
			// here we couldnt find a valid next, so try next entry until we find a valid one
			curr_ = VuListIterator(&h->table_[idx_]);
			ret = curr_.GetFirst();
		}
	} while (ret == NULL && idx_ < h->capacity_);
	return ret;

#if 0
	curr_ = curr_->next_;

	if (curr_ != vuTailNode)
	{
		// sfr: smartpointer
		return curr_->entity_.get();
	}

	entry_++;

	while (*entry_ == vuTailNode)
	{
		entry_++;
	}

	if (*entry_ == 0)
	{
		curr_ = vuTailNode;
	}
	else
	{
		curr_ = *entry_;
	}

	// sfr: smartpointer
	return curr_->entity_.get();
#endif
}

VuEntity *VuHashIterator::GetFirst(VuFilter* filter){
	if (!filter){ return GetFirst(); }
	VuEntity *ret = GetFirst();
	if (ret == NULL){ return NULL; }
	if (filter->Test(ret)){
		return ret;
	}
	else {
		return GetNext(filter);
	}
	
#if 0
	// sfr: smartpointer
	GetFirst();
	
	if (filter)
	{
		if (curr_->entity_.get() == 0)
		{
			return curr_->entity_.get();
		}

		if (filter->Test(curr_->entity_.get()))
		{
			return curr_->entity_.get();
		}

		return GetNext(filter);
	}

	return curr_->entity_.get();
#endif
}

VuEntity *VuHashIterator::GetNext(VuFilter* filter){
	if (!filter){ return GetNext(); }
	VuEntity *ret;
	do {
		ret = GetNext();
		if (ret == NULL || filter->Test(ret)){
			return ret;
		}
	} while (1);

#if 0
	// sfr: smartpointer
	GetNext();
	
	if (filter)
	{
		if (curr_->entity_.get() == 0)
		{
			return curr_->entity_.get();
		}

		if (filter->Test(curr_->entity_.get()))
		{
			return curr_->entity_.get();
		}

		return GetNext(filter);
	}
	
	return curr_->entity_.get();
#endif
}

VuEntity *VuHashIterator::CurrEnt(){
	return curr_.CurrEnt();
#if 0
	// sfr: smartpointer
	return curr_->entity_.get();
#endif
}


/*VU_BOOL VuHashIterator::IsReferenced (VuEntity* ent)
{
// 2002-02-04 MODIFIED BY S.G. If ent is false, then it can't be a valid entity, right? That's what I think too :-)
//	if (curr_->entity_ == ent)
	// sfr: smartpointer
	if (ent && curr_->entity_.get() == ent)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}*/


VU_ERRCODE VuHashIterator::Cleanup(){
#if 0
	curr_ = vuTailNode;
#endif
	return VU_SUCCESS;
}

