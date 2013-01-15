#ifndef FALCLIST_H
#define FALCLIST_H

extern int SimCompare (VuEntity* ent1, VuEntity*ent2);

/** sfr: I think priate here means they are not changed by VU insertions, since the Insert functions
* are all stubs which does nothing. So even if an entity pass the filter test, it wont be inserted 
* at all. The result is that these collections only insert elements when you explicity call ForcedInsert.
* Removal works as it usual.
*/

// =================================
// Falcon's Private Filters
// =================================

#if 0
class FalconAllFilterType : public VuFilter {
public:
	FalconAllFilterType(void)							{}
	virtual ~FalconAllFilterType(void)					{}

	virtual VU_BOOL Test(VuEntity *)					{ return TRUE; }
	virtual VU_BOOL RemoveTest(VuEntity *)			{ return TRUE; }
	virtual int Compare(VuEntity *ent1, VuEntity *ent2)	{ return (SimCompare (ent1, ent2)); }
	virtual VuFilter *Copy()							{ return new FalconAllFilterType(); }
};
#endif

/** accepts no entities. */
class FalconNothingFilterType : public VuFilter {
public:
	FalconNothingFilterType(void)						{}
	virtual ~FalconNothingFilterType(void)				{}

	virtual VU_BOOL Test(VuEntity *)					{ return FALSE; }
	virtual VU_BOOL RemoveTest(VuEntity *)				{ return TRUE; }
	virtual int Compare(VuEntity *ent1, VuEntity *ent2)	{ return (SimCompare (ent1, ent2)); }
	virtual VuFilter *Copy()							{ return new FalconNothingFilterType(); }
};

//extern FalconAllFilterType		FalconAllFilter;
extern FalconNothingFilterType	FalconNothingFilter;

// =================================
// Falcon's Private lists
// =================================

#if VU_ALL_FILTERED

/** TailInsertList is a Falcon-Private list structure which will add entries to the end of the list and
* remove from the begining.
*/
class TailInsertList : public VuLinkedList {
public:
    TailInsertList(VuFilter *filter = &FalconNothingFilter);
    virtual ~TailInsertList(void);

protected:
	/** does nothing, so collection manager doesnt mess with us. */
    virtual int PrivateInsert(VuEntity *entity);

public:
	/** public insertion. */
    int ForcedInsert(VuEntity *entity);

	/** removes first element. */
	VuEntity *PopHead();
};

// HeadInsertList is a Falcon-Private list structure which will add entries to the beginning of the list
class HeadInsertList : public VuLinkedList {
public:
    HeadInsertList(VuFilter *filter = &FalconNothingFilter);
    virtual ~HeadInsertList(void);

protected:
	/** does nothing, so vu collection manager doesnt mess with us */
    virtual int PrivateInsert(VuEntity *entity);

public:
	/** public insertion. */
    int ForcedInsert(VuEntity *entity);
};

// Falcon FalconPrivateList is simply a Falcon-Private vu entity storage list
class FalconPrivateList : public VuLinkedList {
public:
    FalconPrivateList(VuFilter *filter = &FalconNothingFilter);
    virtual ~FalconPrivateList();

protected:
	/** does nothing, so vu collection manager doesnt mess with us */
    virtual int PrivateInsert(VuEntity *entity);

public:
	/** public insertion. */
    int ForcedInsert(VuEntity *entity);
};

// Falcon PrivateFilteredList is identical to above but sorts entries
class FalconPrivateOrderedList : public VuLinkedList {
public:
    FalconPrivateOrderedList(VuFilter *filter = &FalconNothingFilter);
    virtual ~FalconPrivateOrderedList();

protected:
	/** does nothing, so vu collection manager doesnt mess with us */
    virtual int PrivateInsert(VuEntity *entity);

public:
	/** public insertion. */
    int ForcedInsert(VuEntity *entity);
};

#else
/** TailInsertList is a Falcon-Private list structure which will add entries to the end of the list and
* remove from the begining.
*/
class TailInsertList :public VuFilteredList {
public:
    TailInsertList(VuFilter *filter = &FalconNothingFilter);
    virtual ~TailInsertList(void);

	/** does nothing, so collection manager doesnt mess with us. */
    int Insert(VuEntity *entity);				// WARNING: DO NOT CALL THIS INSERT FUNCTION
	/** real insertion. */
    int ForcedInsert(VuEntity *entity);		// Call ForcedInsert instead.
	/** removes first element. */
	VuEntity *PopHead();
};

// HeadInsertList is a Falcon-Private list structure which will add entries to the beginning of the list
class HeadInsertList : public VuFilteredList {
public:
    HeadInsertList(VuFilter *filter = &FalconNothingFilter);
    virtual ~HeadInsertList(void);

    int Insert(VuEntity *entity);				// WARNING: DO NOT CALL THIS INSERT FUNCTION
    int ForcedInsert(VuEntity *entity);		// Call ForcedInsert instead.
};

// Falcon FalconPrivateList is simply a Falcon-Private vu entity storage list
class FalconPrivateList : public VuFilteredList {
public:
    FalconPrivateList(VuFilter *filter = &FalconNothingFilter);
    virtual ~FalconPrivateList();

    int Insert(VuEntity *entity);				// WARNING: DO NOT CALL THIS INSERT FUNCTION
    int ForcedInsert(VuEntity *entity);		// Call ForcedInsert instead.
};

// Falcon PrivateFilteredList is identical to above but sorts entries
class FalconPrivateOrderedList : public VuFilteredList {
public:
    FalconPrivateOrderedList(VuFilter *filter = &FalconNothingFilter);
    virtual ~FalconPrivateOrderedList();

    int Insert(VuEntity *entity);				// WARNING: DO NOT CALL THIS INSERT FUNCTION
    int ForcedInsert(VuEntity *entity);		// Call ForcedInsert instead.
};
#endif

#endif