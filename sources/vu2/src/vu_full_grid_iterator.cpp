#if 0
#include "vu2.h"
#include "vu_priv.h"

//-----------------------------------------------------------------------------
// VuFullGridIterator
//-----------------------------------------------------------------------------

VuFullGridIterator::VuFullGridIterator(VuGridTree* coll) : VuIterator(coll), crow_(0), rbit_(&coll->table_[0]){
}

VuFullGridIterator::~VuFullGridIterator(){
}

VuEntity *VuFullGridIterator::GetFirst(){
	if (!collection_){
		return NULL;
	}
	VuGridTree *g = static_cast<VuGridTree*>(collection_);
	crow_ = 0;
	rbit_ = VuRBIterator(&g->table_[crow_]);
	VuEntity *ret = rbit_.GetFirst();
	if (ret == NULL){
		ret = GetNext();
	}
	return ret;
}

VuEntity *VuFullGridIterator::GetNext(){
	VuGridTree *g = static_cast<VuGridTree*>(collection_);
	VuEntity *ret = rbit_.GetNext();
	do {
		if (ret != NULL){
			// found it
			return ret;
		}
		else {
			if (crow_ + 1 == g->rowcount_){
				// last row
				return NULL;
			}
			else {
				// next row
				++crow_;
				rbit_ = VuRBIterator(&g->table_[crow_]);
				ret = rbit_.GetFirst();
			}
		}
	} while (1);
}

VuEntity *VuFullGridIterator::GetFirst(VuFilter* filter){
	VuEntity* retval = GetFirst();
	if (retval == 0 || filter->Test(retval)){
		return retval;
	}
	return GetNext(filter);
}

VuEntity *VuFullGridIterator::GetNext(VuFilter* filter){
	VuEntity* retval = 0;
	while ((retval = GetNext()) != 0){
		if (filter->Test(retval)){
			return retval;
		}
	}
	return retval;
}
#endif

#if 0
VuFullGridIterator::VuFullGridIterator (VuGridTree* coll) : VuRBIterator(coll)
{
	curRB_ = 0;
	currow_ = 0;
	// empty
}

VuFullGridIterator::~VuFullGridIterator()
{
	// empty
}

VuEntity *VuFullGridIterator::GetFirst()
{
	if (collection_)
	{
		currow_  = 0;
		curlink_ = vuTailNode;
		curRB_   = ((VuGridTree*)collection_)->table_;
		curnode_ = curRB_->root_;

		if (curnode_)
		{
			curnode_ = curnode_->TreeMinimum();
			curlink_ = curnode_->head_;

			// sfr: smartpointer
			return curlink_->entity_.get();
		}

		return GetNext();
	}

	return 0;
}

VuEntity *VuFullGridIterator::GetNext()
{
	// sfr: smartpointer
	while ((curnode_ == 0) && (++currow_ < ((VuGridTree *)collection_)->rowcount_))
	{
		curRB_++;
		curnode_ = curRB_->root_;

		if (curnode_)
		{
			curnode_ = curnode_->TreeMinimum();
			curlink_ = curnode_->head_;

			return curlink_->entity_.get();
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

		if (curnode_ == 0)
		{
			// skip to next row
			curlink_ = vuTailNode;
			curnode_ = 0;

			return GetNext();
		}

		curlink_ = curnode_->head_;
	}

	return curlink_->entity_.get();
}

VuEntity *VuFullGridIterator::GetFirst(VuFilter* filter)
{
	VuEntity* retval = GetFirst();

	if (retval == 0 || filter->Test(retval))
	{
		return retval;
	}

	return GetNext(filter);
}

VuEntity *VuFullGridIterator::GetNext(VuFilter* filter)
{
	VuEntity* retval = 0;

	while ((retval = GetNext()) != 0)
	{
		if (filter->Test(retval))
		{
			return retval;
		}
	}

	return retval;
}
#endif