/** @file vu_gc.cpp garbage collector implementation. */

#include <list>
#include "vu.h"
#include "vu_priv.h"
#include "vu_gc.h"

#include "falclib/include/F4thread.h"

using namespace std;

/** garbage collector internal structure. */
struct VuGC::Data {
	F4CSECTIONHANDLE *gcmutex;
	// FIFOs
	list<VuEntityBin> es;          ///< list entities to be deleted
};

VuGC::VuGC() : d(new Data){
	d->gcmutex = F4CreateCriticalSection("gc mutex");
}
VuGC::~VuGC(){
	flush();
	F4DestroyCriticalSection(d->gcmutex);
}

void VuGC::insert(VuEntity *e){
	F4ScopeLock l(d->gcmutex);
	d->es.push_back(VuEntityBin(e));
}


void VuGC::flush(){
	// remove list nodes 
	while (!d->es.empty()){
		VuEntityBin &eb = d->es.back();
		d->es.pop_back();
		vuDatabase->ReallyRemove(eb.get());
	}	
}





