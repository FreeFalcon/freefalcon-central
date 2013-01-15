#include "vu2.h"
#include "vu_priv.h"

//-----------------------------------------------------------------------------
// VuDatabaseIterator
//-----------------------------------------------------------------------------

#if VU_ALL_FILTERED
VuDatabaseIterator::VuDatabaseIterator() : VuHashIterator(vuDatabase->dbHash_){
}
#else
VuDatabaseIterator::VuDatabaseIterator() : VuHashIterator(vuDatabase){
}
#endif

VuDatabaseIterator::~VuDatabaseIterator(){
}

