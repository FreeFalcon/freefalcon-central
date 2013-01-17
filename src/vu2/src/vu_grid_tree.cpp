#include <float.h>
#include "vu_priv.h"
#include "vu2.h"

#if VU_ALL_FILTERED

VuGridTree::VuGridTree(VuBiKeyFilter* filter, unsigned int res) : 
	VuCollection(filter), res_(res), suspendUpdates_(FALSE), nextgrid_(0)
{
	table_        = new VuRedBlackTree*[res_];
	for (unsigned int i = 0; i < res_; ++i) {
		table_[i] = new VuRedBlackTree(filter);
	}
	vuCollectionManager->GridRegister(this);
}

VuGridTree::~VuGridTree(){
	Purge();
	delete [] table_;
	vuCollectionManager->GridDeRegister(this);
}

VU_ERRCODE VuGridTree::PrivateInsert(VuEntity* entity){
	VuBiKeyFilter *bkf = GetBiKeyFilter();
	VuRedBlackTree *row = table_[bkf->Key1(entity)];
	return row->ForcedInsert(entity);
}

VU_ERRCODE VuGridTree::PrivateRemove(VuEntity *entity){
	VuBiKeyFilter *bkf = GetBiKeyFilter();
	VuRedBlackTree* row = table_[bkf->Key1(entity)];
	VU_ERRCODE res = row->Remove(entity);
	return res;
}

bool VuGridTree::PrivateFind(VuEntity* entity) const {
	VuBiKeyFilter *bkf = GetBiKeyFilter();
	const VuRedBlackTree* row = table_[bkf->Key1(entity)];
	return row->Find(entity);
}

VU_ERRCODE VuGridTree::Move(VuEntity *ent, BIG_SCALAR coord1, BIG_SCALAR coord2){
	VuScopeLock l(GetMutex());
	VuBiKeyFilter *bkf = GetBiKeyFilter();
	if ((ent != NULL) && (ent->VuState() == VU_MEM_ACTIVE) && bkf->RemoveTest(ent)) {
		VuEntityBin safe(ent);
		VU_KEY ck1 = bkf->Key1(ent);
		VU_KEY nk1 = bkf->CoordToKey(coord1);
		VU_KEY ck2 = bkf->Key2(ent);
		VU_KEY nk2 = bkf->CoordToKey(coord2);
		if (ck1 != nk1 || ck2 != nk2){
			// keys changed... have to remove and insert again
			table_[ck1]->Remove(ent);
			table_[nk1]->Insert(ent);
		}
		return VU_SUCCESS;
	}
	return VU_NO_OP;
}

VuBiKeyFilter *VuGridTree::GetBiKeyFilter() const {
	return static_cast<VuBiKeyFilter*>(GetFilter());
}

#else

VuGridTree::VuGridTree(
	VuBiKeyFilter* filter, unsigned int numrows, BIG_SCALAR center, BIG_SCALAR radius
) : 
	VuCollection(), rowcount_(numrows), suspendUpdates_(FALSE), nextgrid_(0)
{
	filter_       = static_cast<VuBiKeyFilter*>(filter->Copy());
	ulong icenter = filter->CoordToKey1(center);
	ulong iradius = filter->Distance1(radius);
	bottom_       = icenter - iradius;
	top_          = icenter + iradius;
	rowheight_    = 1.0f;//(top_ - bottom_)/rowcount_;
	invrowheight_ = 1.0f;//1.0f/(float)((top_ - bottom_)/rowcount_);

	table_        = new VuRedBlackTree*[numrows];
	for (unsigned int i = 0; i < numrows; ++i) {
		table_[i] = new VuRedBlackTree(filter_);
	}
	
	vuCollectionManager->GridRegister(this);
}

VuGridTree::~VuGridTree(){
	Purge();
	delete [] table_;
	delete filter_;
	filter_ = 0;
	vuCollectionManager->GridDeRegister(this);
}

unsigned int VuGridTree::Row(VU_KEY key1) const {
	VuScopeLock l(GetMutex());
	// if this goes off - the FPU is rounding to nearest rather than down.
	_controlfp(_RC_CHOP, MCW_RC);

	unsigned int index = 0;

	// compute index 
	if (key1 > bottom_) {
		index = FTOL((key1 - bottom_)*invrowheight_);
	}
	// upper limit
	if (index >= rowcount_) {
		index = rowcount_-1;
	}
	
	return index;
}

VU_ERRCODE VuGridTree::ForcedInsert(VuEntity* entity){
	if (entity == NULL){ return VU_NO_OP; }
	VuScopeLock l(GetMutex());
	if (!filter_->RemoveTest(entity)) return VU_NO_OP;
	VuRedBlackTree *row = table_[Row(filter_->Key1(entity))];
	return row->ForcedInsert(entity);
}

VU_ERRCODE VuGridTree::Insert(VuEntity *entity){
	if (entity == NULL){ return VU_NO_OP; }
	VuScopeLock l(GetMutex());
	if (!filter_->Test(entity)) return VU_NO_OP;
	VuRedBlackTree *row = table_[Row(filter_->Key1(entity))];
	return row->Insert(entity);
}

VU_ERRCODE VuGridTree::Remove(VuEntity *entity){
	VuScopeLock l(GetMutex());
	if (filter_->RemoveTest(entity)) {
		VuRedBlackTree* row = table_[Row(filter_->Key1(entity))];
		VU_ERRCODE res = row->Remove(entity);
		return res;
	}
	return VU_NO_OP;
}

VU_ERRCODE VuGridTree::Remove(VU_ID entityId){
	// since filter is responsible for keying, we cannot use ID as key
	VuEntity *ent = vuDatabase->Find(entityId);
	if (ent) {
		return Remove(ent);
	}
	return VU_NO_OP;
}

VuEntity *VuGridTree::Find(VU_ID entityId) const {
	VuEntity* ent = vuDatabase->Find(entityId);
	return Find(ent);
}

VuEntity *VuGridTree::Find(VuEntity* ent) const {
	if (!ent){ return NULL; }

	VuScopeLock l(GetMutex());
	const VuRedBlackTree* row = table_[Row(filter_->Key1(ent))];
	return row->Find(ent);
}

VU_ERRCODE VuGridTree::Move(VuEntity *ent, BIG_SCALAR coord1, BIG_SCALAR coord2){
	VuScopeLock l(GetMutex());
	if ((ent != NULL) && (ent->VuState() == VU_MEM_ACTIVE) && filter_->RemoveTest(ent)) {
		VuEntityBin safe(ent);
		VU_KEY ck1 = filter_->Key1(ent);
		VU_KEY nk1 = filter_->CoordToKey1(coord1);
		VU_KEY ck2 = filter_->Key2(ent);
		VU_KEY nk2 = filter_->CoordToKey2(coord2);
		if (ck1 != nk1 || ck2 != nk2){
			// keys changed... have to remove and insert again
			table_[Row(ck1)]->Remove(ent);
			table_[Row(nk1)]->Insert(ent);
		}
		return VU_SUCCESS;
	}
	return VU_NO_OP;
}


#endif


unsigned int VuGridTree::Purge(VU_BOOL all){
	int retval = 0;
	VuScopeLock l(GetMutex());
	for (unsigned int i = 0; i < res_; i++) {
		retval += table_[i]->Purge(all);
	}
	return retval;
}

unsigned int VuGridTree::Count() const {
	VuScopeLock l(GetMutex());
	int count = 0;
	for (unsigned int i = 0; i < res_; i++) {
		count += table_[i]->Count();
	}
	return count;
}

VU_COLL_TYPE VuGridTree::Type() const {
	return VU_GRID_TREE_COLLECTION;
}
