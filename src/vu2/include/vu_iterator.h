#ifndef VU_ITERATOR_H
#define VU_ITERATOR_H

#include <list>
#include "vucoll.h"

/** @file vu_iterator.h vu iterators declaration. */


class VuIterator {
public:
	virtual VuEntity *CurrEnt() = 0;
	virtual VU_ERRCODE Cleanup();

protected:
	/** may LOCK collection if collection is thread safe */
	VuIterator(VuCollection *coll);
	/** unlocks collection if locked */
	virtual ~VuIterator();
	VuCollection *collection_;
};

class VuListIterator : public VuIterator {
public:
	/** creates the iterator, may LOCK the list */
	VuListIterator(VuLinkedList *coll);
	/** destroys the iterator, unlocking if locked */
	virtual ~VuListIterator();

	/** removes current element (if not end) and goto next. */
	void RemoveCurrent();
	VuEntity *GetFirst();
	VuEntity *GetNext();
	VuEntity *GetFirst(VuFilter *filter);
	VuEntity *GetNext(VuFilter *filter);
	virtual VuEntity *CurrEnt();

	virtual VU_ERRCODE Cleanup();

protected:
	std::list<VuEntityBin>::iterator curr_;
};

class VuRBIterator : public VuIterator {
	friend class VuGridIterator;
public:
	/** creates iterator, may lock the tree */
	VuRBIterator(VuRedBlackTree *coll);
	virtual ~VuRBIterator();

	VuEntity *GetFirst();
	VuEntity *GetFirst(VU_KEY low);
	VuEntity *GetNext();
	VuEntity *GetFirst(VuFilter *filter);
	VuEntity *GetNext(VuFilter *filter);
	virtual VuEntity *CurrEnt();

	virtual VU_ERRCODE Cleanup();

protected:
	VuRBIterator(VuCollection *coll);

protected:
	VuRedBlackTree::RBMap::iterator curr_;
};

class VuGridIterator : public VuIterator {
public:
	VuGridIterator(VuGridTree *coll, BIG_SCALAR xPos, BIG_SCALAR yPos, BIG_SCALAR radius);
	virtual ~VuGridIterator();

	// note: these implementations HIDE the RBIterator methods, which
	//       is intended, but some compilers will flag this as a warning
	VuEntity *GetFirst();
	VuEntity *GetNext();
	VuEntity *GetFirst(VuFilter *filter);
	VuEntity *GetNext(VuFilter *filter);
	virtual VuEntity *CurrEnt();

protected:
	/** tree row interval, open at end. */
	unsigned int  rowlow_, rowhi_, rowcur_;
	/** tree column intervals, open at end. */
	VU_KEY collow_, colhi_, colcur_;
	/** origin, radius and radius squared of search in real units. */
	BIG_SCALAR p1_, p2_, radius_, radius_2_;
	/** iterator on trees. */
	VuRBIterator it_;
};


class VuHashIterator : public VuIterator {
public:
	/** creates hash iterator, may lock hash */
	VuHashIterator(VuHashTable *coll);
	virtual ~VuHashIterator();

	VuEntity *GetFirst();
	VuEntity *GetNext();
	VuEntity *GetFirst(VuFilter *filter);
	VuEntity *GetNext(VuFilter *filter);
	virtual VuEntity *CurrEnt();

	virtual VU_ERRCODE Cleanup();

protected:
	unsigned int idx_;
	VuListIterator curr_;
};

class VuDatabaseIterator : public VuHashIterator {
public:
	VuDatabaseIterator();
	virtual ~VuDatabaseIterator();
};

class VuSessionsIterator : public VuListIterator {
public:
	/** creates iterator, may lock the group mutex */
	VuSessionsIterator(VuGroupEntity *group=0);
	virtual ~VuSessionsIterator();

	VuSessionEntity *GetFirst();
	VuSessionEntity *GetNext();
	virtual VuEntity *CurrEnt();

	virtual VU_ERRCODE Cleanup();
};

#endif

