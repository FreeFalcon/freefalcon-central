#include <list>
#include <time.h>
#include "falclib.h"
#include "classtbl.h"
#include "vu2/src/vu_priv.h"
#include "F4Vu.h"
#include "FalcList.h"

using namespace std;

// =================================
// Falcon's Private lists
// =================================

// Private collection functions.  These collections provide a ForcedInsert that
// actually does the inserting. The Insert function does nothing and is provided
// to keep VU from adding things to the lists. The application must call ForcedInsert
// to actually add to the list.
// The optional filter in the constructor can add efficiency if it has a reasonable
// RemoveTest() function which will filter out entities which couldn't be in the list

#if VU_ALL_FILTERED

// Private List
FalconPrivateList::FalconPrivateList(VuFilter *filter) : VuLinkedList(filter){

}

FalconPrivateList::~FalconPrivateList(){
}

int FalconPrivateList::PrivateInsert(VuEntity *entity){
	return VU_NO_OP;
}

int FalconPrivateList::ForcedInsert(VuEntity *entity){
	return VuLinkedList::PrivateInsert(entity);
}

// Ordered List
FalconPrivateOrderedList::FalconPrivateOrderedList(VuFilter *filter) : VuLinkedList(filter){
}

FalconPrivateOrderedList::~FalconPrivateOrderedList(){
}

int FalconPrivateOrderedList::PrivateInsert(VuEntity *entity){
	return VU_NO_OP;
}

int FalconPrivateOrderedList::ForcedInsert(VuEntity *entity){
	for (list<VuEntityBin>::iterator it=l_.begin();it!=l_.end();++it){
		VuEntityBin &eb = *it;
		if (entity == eb.get()){
			// already in
			return VU_NO_OP;
		}
		else if (GetFilter()->Compare(entity, eb.get()) > 0){
			l_.insert(it, VuEntityBin(entity));
			return VU_SUCCESS;
		}
	}
	l_.push_back(VuEntityBin(entity));
	return VU_SUCCESS;
}

// Tail insert list

TailInsertList::TailInsertList(VuFilter *filter) : VuLinkedList(filter)
{
}

TailInsertList::~TailInsertList(void){
}

int TailInsertList::PrivateInsert(VuEntity *entity){
	return VU_NO_OP;
}

int TailInsertList::ForcedInsert(VuEntity *entity){
	VuScopeLock l(GetMutex());
	l_.push_back(VuEntityBin(entity));
	return VU_SUCCESS;
}

VuEntity *TailInsertList::PopHead(){
	VuScopeLock l(GetMutex());
	while (!l_.empty()){
		VuEntityBin eb = l_.front();
		l_.pop_front();
		if (eb->VuState() == VU_MEM_ACTIVE){
			// found a good one
			return eb.get();
		}
	}
	return NULL;
}

// Head insert list
HeadInsertList::HeadInsertList(VuFilter *filter) : VuLinkedList(filter)
{
}

HeadInsertList::~HeadInsertList(void){
}

int HeadInsertList::PrivateInsert(VuEntity *entity){
	return VU_NO_OP;
}

int HeadInsertList::ForcedInsert(VuEntity *entity){
	VuScopeLock l(GetMutex());
	l_.push_front(VuEntityBin(entity));
	return VU_SUCCESS;
}

#else

// Private List

FalconPrivateList::FalconPrivateList(VuFilter *filter) : VuFilteredList(filter)
{

}

FalconPrivateList::~FalconPrivateList(){
}

int FalconPrivateList::Insert(VuEntity *entity){
	return 0;
}

int FalconPrivateList::ForcedInsert(VuEntity *entity){
	return VuLinkedList::Insert(entity);
}

// Ordered List
FalconPrivateOrderedList::FalconPrivateOrderedList(VuFilter *filter) : VuFilteredList(filter)
{
}

FalconPrivateOrderedList::~FalconPrivateOrderedList(){
}

int FalconPrivateOrderedList::Insert(VuEntity *entity){
	return 0;
}

int FalconPrivateOrderedList::ForcedInsert(VuEntity *entity){
	for (list<VuEntityBin>::iterator it=l_.begin();it!=l_.end();++it){
		VuEntityBin &eb = *it;
		if (entity == eb.get()){
			// already in
			return 1;
		}
		// sfr: @todo shouldnt we use the filter instead?
		//else if (FalconAllFilter.Compare(entity, eb.get()) > 0)
		else if (SimCompare(entity, eb.get()) > 0)
		{
			l_.insert(it, VuEntityBin(entity));
			return 1;
		}
	}
	l_.push_back(VuEntityBin(entity));
	return 1;
}

// Tail insert list

TailInsertList::TailInsertList(VuFilter *filter) : VuFilteredList(filter)
{
}

TailInsertList::~TailInsertList(void){
}

int TailInsertList::Insert(VuEntity *entity){
	return 0;
}

int TailInsertList::ForcedInsert(VuEntity *entity){
	VuScopeLock l(GetMutex());
	l_.push_back(VuEntityBin(entity));
	return 1;
}

VuEntity *TailInsertList::PopHead(){
	VuScopeLock l(GetMutex());
	while (!l_.empty()){
		VuEntityBin eb = l_.front();
		l_.pop_front();
		if (eb->VuState() == VU_MEM_ACTIVE){
			// found a good one
			return eb.get();
		}
	}
	return NULL;
}

// Head insert list
HeadInsertList::HeadInsertList(VuFilter *filter) : VuFilteredList(filter)
{
}

HeadInsertList::~HeadInsertList(void){
}

int HeadInsertList::Insert(VuEntity *entity){
	return 0;
}

int HeadInsertList::ForcedInsert(VuEntity *entity){
	VuScopeLock l(GetMutex());
	l_.push_front(VuEntityBin(entity));
	return 1;
}

#endif