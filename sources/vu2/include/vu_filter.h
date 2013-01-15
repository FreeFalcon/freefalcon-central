#ifndef VU_FILTER_H
#define VU_FILTER_H

/** @file vu_filter.h vu filters declaration. */
#include "vutypes.h"
#include "vuentity.h"

/** interface for filters */
class VuFilter {
public:
	/** destructor */
	virtual ~VuFilter();

	/** used for ordered containers mostly 
	* @return < 0 --> ent1  < ent2, == 0 --> ent1 == ent2, > 0 --> ent1  > ent2
	*/
	virtual int Compare(VuEntity *ent1, VuEntity *ent2) = 0;
	
	/** test if ent can be inserted into a filtered collection 
	* @return TRUE --> ent in sub-set and can be inserted,  FALSE --> ent not in sub-set and wont be inserted
	*/
	virtual VU_BOOL Test(VuEntity *ent) = 0;
	
	/** called before removing an entity from collection
	* @return TRUE --> ent might be in sub-set and may be removed, FALSE --> ent could never have been in sub-set
	*/
	virtual VU_BOOL RemoveTest(VuEntity *ent);

	/** called to check if a message can affect the collection 	
	* @return TRUE --> event may cause a change to result of Test(),  FALSE --> event will never cause a change to result of Test()
	*/
	virtual VU_BOOL Notice(VuMessage *event);
	
	/** allocates and returns a copy */
	virtual VuFilter *Copy() = 0;
	
protected:
	// constructors, all protected since this is a base virtual class
	/** default */
	VuFilter() {}
	/** copy constructor */
	VuFilter(VuFilter *) {}

};



class VuStandardFilter : public VuFilter {
public:
	VuStandardFilter(VuFlagBits mask, VU_TRI_STATE localSession = DONT_CARE);
	VuStandardFilter(ushort mask, VU_TRI_STATE localSession = DONT_CARE);
	virtual ~VuStandardFilter();

	virtual VU_BOOL Test(VuEntity *ent);
	virtual int Compare(VuEntity *ent1, VuEntity *ent2);
  		// returns ent2->Id() - ent1->Id()
	virtual VuFilter *Copy();

	virtual VU_BOOL Notice(VuMessage *event);

protected:
	VuStandardFilter(VuStandardFilter *other);
 
protected:
	union {
		VuFlagBits breakdown_;
		ushort val_;
	} idmask_;
	VU_TRI_STATE localSession_;
};

class VuKeyFilter : public VuFilter {
public:
	virtual ~VuKeyFilter();

	virtual VU_BOOL Test(VuEntity *ent) = 0;

	/** uses Key()...
  	* < 0 --> ent1  < ent2
  	* == 0 --> ent1 == ent2
  	* > 0 --> ent1  > ent2
	*/
	virtual int Compare(VuEntity *ent1, VuEntity *ent2);

	virtual VuFilter *Copy() = 0;

  	/** translates ent into a VU_KEY... used in Compare (above)
	* default implemetation returns id coerced to VU_KEY
	*/
	virtual VU_KEY Key(const VuEntity *ent) const = 0;

protected:
	VuKeyFilter() {}
	VuKeyFilter(VuKeyFilter *) {}

};

/** a filter that converts coordinates to grid keys. Which coordinate to use (X/Y) depends on how
* the map is organized (y major, or X major). So this class abstracts the order and correct usage
* is responsability of higher classes.
*/
class VuBiKeyFilter : public VuKeyFilter {
protected:
	VuBiKeyFilter(unsigned int res, BIG_SCALAR max) : res_(res), factor_(static_cast<BIG_SCALAR>(res) / max){}
	VuBiKeyFilter(const VuBiKeyFilter &rhs) { res_ = rhs.res_; factor_ = rhs.factor_; }
	virtual VuBiKeyFilter::~VuBiKeyFilter(){}

public:
	// virtual interface
	virtual VU_BOOL Test(VuEntity *ent) = 0;
	virtual VuFilter *Copy() = 0;

	/** translates ent into a VU_KEY... used in Compare (above)
	* default implementation calls Key2(ent). This is useful for grids, since the RB trees
	* share the same filter and then just call it to get their coordinates correctly. 
	*/
	virtual VU_KEY Key(const VuEntity *ent) const ;

	/** returns key1 for given entity. */
	VU_KEY Key1(const VuEntity *ent) const;
	/** returns key2 for given entity. */
	VU_KEY Key2(const VuEntity *ent) const;
	/** converts a coordinate to key. */
	VU_KEY CoordToKey(BIG_SCALAR coord) const;

private:
	/** grid resolution. */
	unsigned int res_;
	/** multiply by a real coordinate to get key. */
	float factor_;
};


class VuAssociationFilter : public VuFilter {
public:
	VuAssociationFilter(VU_ID association);
	virtual ~VuAssociationFilter();

	virtual VU_BOOL Test(VuEntity *ent);
	virtual int Compare(VuEntity *ent1, VuEntity *ent2);
  		// returns ent2->Id() - ent1->Id()
	virtual VuFilter *Copy();

protected:
	VuAssociationFilter(VuAssociationFilter *other);

	VU_ID association_;
};

class VuTypeFilter : public VuFilter {
public:
	VuTypeFilter(ushort type);
	virtual ~VuTypeFilter();

	virtual VU_BOOL Test(VuEntity *ent);
	virtual VU_BOOL RemoveTest(VuEntity *ent);
	virtual int Compare(VuEntity *ent1, VuEntity *ent2);
  		// returns ent2->Id() - ent1->Id()
	virtual VuFilter *Copy();

protected:
	VuTypeFilter(VuTypeFilter *other);

	ushort type_;
};

class VuOpaqueFilter : public VuFilter {
public:
	VuOpaqueFilter();
	virtual ~VuOpaqueFilter();

	virtual VU_BOOL Test(VuEntity *ent);		// returns FALSE
	virtual int Compare(VuEntity *ent1, VuEntity *ent2);
  		// returns ent2->Id() - ent1->Id()
	virtual VuFilter *Copy();

protected:
	VuOpaqueFilter(VuOpaqueFilter *other);
};


class VuTransmissionFilter : public VuKeyFilter {
public:
	VuTransmissionFilter();
	VuTransmissionFilter(VuTransmissionFilter *other);
	virtual ~VuTransmissionFilter();

	virtual VU_BOOL Test(VuEntity *ent);
  		// TRUE --> ent in sub-set
  		// FALSE --> ent not in sub-set
	virtual int Compare(VuEntity *ent1, VuEntity *ent2);
  		//  < 0 --> ent1  < ent2
  		// == 0 --> ent1 == ent2
  		//  > 0 --> ent1  > ent2
	virtual VuFilter *Copy();

	virtual VU_BOOL Notice(VuMessage *event);

	virtual VU_KEY Key(const VuEntity *ent) const;

protected:
};

class VuSessionFilter : public VuFilter {
public:
	VuSessionFilter(VU_ID groupId);
	virtual ~VuSessionFilter();

	virtual VU_BOOL Test(VuEntity *ent);
	virtual VU_BOOL RemoveTest(VuEntity *ent);
	virtual int Compare(VuEntity *ent1, VuEntity *ent2);
	virtual VU_BOOL Notice(VuMessage *event);
	virtual VuFilter *Copy();

protected:
	VuSessionFilter(VuSessionFilter *other);

// DATA
protected:
	VU_ID groupId_;
};



#endif