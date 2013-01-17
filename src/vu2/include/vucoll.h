#ifndef VUCOLL_H
#define VUCOLL_H

#include <list>
#include <map>

#include "vu_templates.h"
#include "vutypes.h"
#include "vuentity.h"
#include "vumath.h"

#define VU_DEFAULT_HASH_KEY	59

/** database types */
typedef enum {
	VU_UNKNOWN_COLLECTION             = 0x000,
	VU_HASH_TABLE_COLLECTION          =	0x101,
	VU_FILTERED_HASH_TABLE_COLLECTION = 0x102,
	VU_DATABASE_COLLECTION            = 0x103,
	VU_ANTI_DATABASE_COLLECTION       = 0x104,
	VU_LINKED_LIST_COLLECTION         = 0x201,
	VU_FILTERED_LIST_COLLECTION       = 0x202,
	VU_ORDERED_LIST_COLLECTION        = 0x203,
	VU_LIFO_QUEUE_COLLECTION          = 0x204,
	VU_FIFO_QUEUE_COLLECTION          = 0x205,
	VU_RED_BLACK_TREE_COLLECTION      = 0x401,
	VU_GRID_TREE_COLLECTION           = 0x801
} VU_COLL_TYPE;

// fwd declarations
class VuEntity;
class VuMessage;

// vuDatabase global
extern class VuDatabase *vuDatabase;


/** makes all collections filtered. Simplifies implementation. */
#define VU_ALL_FILTERED 1
/** base class for collections */
#if VU_ALL_FILTERED

class VuCollection {

friend class VuCollectionManager;

public:
	/** register collection, so that it will be updated by vuCollection manager.
	* For example: when an entity is removed from vuDatabase, it will also be removed 
	* from all registered collections. Registered collections also handle messages received.
	* This is thread safe.
	*/
	void Register();

	/** unregister collection, so that it stops receiving updates and handling messages.
	* Thread safe.
	*/
	void Unregister();

	/** handles message for collection. 
	* Default implementation does nothing if not filtered. Otherwise, if entity in message
	* should be in collection but is not, its inserted. If its in but should not, its removed.
	* This function is dangerous, not thread safe and its usage should be eliminated if possible.
	*/
	VU_ERRCODE Handle(VuMessage *msg);

	/** inserts an entity into collection. If collection has filter, entity must pass 
	* filter->Test() to be inserted. This function is called on all registered collection
	* by collection manager when a new entity is added to vuDatabase.
	*/
	VU_ERRCODE Insert(VuEntity *entity);

	/** inserts entity, regardless of filter->Test(). */
	VU_ERRCODE ForcedInsert(VuEntity *entity);

	/** removes an entity from collection.
	* If collection is filtered, entity must pass filter->RemoveTest() to run collection
	* looking for entity. This function is called by collection manager on all registered collections
	* when an entity is removed from vuDatabase.
	*/
	VU_ERRCODE Remove(VuEntity *entity);

	/** returns if entity is in collection or not. */
	bool Find(VuEntity *entity) const;

	// virtual public interface 

	/** removes entities from collection. If !all, local persistant and global
	* entities are not removed. 
	*/
	virtual unsigned int Purge(VU_BOOL all = TRUE) = 0;

	/** returns number of active entities in collection. */
	virtual unsigned int Count() const = 0;

	/** return collection type. */
	virtual VU_COLL_TYPE Type() const = 0;

	/** returns collection mutex. */
	inline VuMutex GetMutex() const { return const_cast<VuMutex>(mutex_); } 

protected:
	/** creates the collection. If threadSage, all iterators will lock it while running.
	* This doesnt work yet. If filtered, most operations will rely on filter before actually
	* doing it.
	*/
	VuCollection(VuFilter *filter, bool threadSafe = false);
	/** unregisters collection if registered, purge all elements and releases memory. */
	virtual ~VuCollection();

	// virtual private interface: all entities here are valid and passed filter test.

	/** private insertion, called by Insert and ForcedInsert. Do the actual insertion. */
	virtual VU_ERRCODE PrivateInsert(VuEntity *entity) = 0;

	/** private remove, do the actual removal from collection. */
	virtual VU_ERRCODE PrivateRemove(VuEntity *entity) = 0;

	/** does the actual look up for collection find. */
	virtual bool PrivateFind(VuEntity *entity) const = 0;

	/** gets the collection filter. */
	VuFilter *GetFilter() const;

// DATA
private:
	/** collection mutex */
	VuMutex mutex_;
	/** collection filter (if any). */
	VuFilter *filter_;
	/** tells if collection is registered. */
	bool registered; 
};

#else

class VuCollection {

friend class VuCollectionManager;

public:
	/** register collection, so that it will be updated by vuCollection manager.
	* For example: when an entity is removed from vuDatabase, it will also be removed 
	* from all registered collections. Registered collections also handle messages received.
	* This is thread safe.
	*/
	virtual void Register();

	/** unregister collection, so that it stops receiving updates and handling messages.
	* Thread safe.
	*/
	virtual void Unregister();

	/** handles message for collection. 
	* Default implementation does nothing. All registered collections handle messages 
	*/
	virtual VU_ERRCODE Handle(VuMessage *msg);

	/** inserts an entity into collection. If collection has filter, entity must pass 
	* filter->Test() to be inserted.
	*/
	virtual VU_ERRCODE Insert(VuEntity *entity)= 0;

	/** removes an entity from collection.
	* If collection is filtered, entity must pass filter->RemoveTest() to run collection
	* looking for entity.
	*/
	virtual VU_ERRCODE Remove(VuEntity *entity) = 0;

	/** for associative containers, removes all entities with the given ID. */
	virtual VU_ERRCODE Remove(VU_ID eid) = 0;

	/** removes entities from collection. If !all, local persistant and global
	* entities are not removed. 
	*/
	virtual unsigned int Purge(VU_BOOL all = TRUE) = 0;

	/** returns number of active entities in collection. */
	virtual unsigned int Count() const = 0;

	/** find an entity in collection, whether its ative or not. */
	virtual VuEntity *Find(VuEntity *entity) const = 0;

	/** for associative containers, finds an active entity using its ID. */
	virtual VuEntity *Find(VU_ID eid) const = 0;

	/** return collection type. */
	virtual VU_COLL_TYPE Type() const = 0;

	/** returns collection mutex */
	inline VuMutex GetMutex() const { return const_cast<VuMutex>(mutex_); } 

protected:
	VuCollection(bool threadSafe = false);
	virtual ~VuCollection();


// DATA
private:
	/** collection mutex */
	VuMutex mutex_;
	/** tells if collection is registered. */
	bool registered; 
};
#endif

/** a linked list. */
#if VU_ALL_FILTERED
class VuLinkedList : public VuCollection {
friend class VuListIterator;
public:
	VuLinkedList(VuFilter *filter = NULL);
	virtual ~VuLinkedList();

protected:
	// virtual private interface
	virtual VU_ERRCODE PrivateInsert(VuEntity *entity);
	virtual VU_ERRCODE PrivateRemove(VuEntity *entity);
	virtual bool PrivateFind(VuEntity *entity) const;

public:
	// virtual public interface
	virtual unsigned int Purge(VU_BOOL all = TRUE);
	virtual unsigned int Count() const;
	virtual VU_COLL_TYPE Type() const;

protected:
	typedef std::list< VuEntityBin > VuEntityBinList;
	typedef VuEntityBinList::iterator iterator;
	/** the real list. */
	VuEntityBinList l_;
};
typedef VuLinkedList VuFilteredList ;
#else
class VuLinkedList : public VuCollection {
friend class VuListIterator;
public:
	VuLinkedList();
	virtual ~VuLinkedList();

	virtual VU_ERRCODE Insert(VuEntity *entity);
	virtual VU_ERRCODE Remove(VuEntity *entity);
	virtual VU_ERRCODE Remove(VU_ID entityId);
	virtual unsigned int Purge(VU_BOOL all = TRUE);
	virtual unsigned int Count() const;
	virtual VuEntity *Find(VU_ID entityId) const;
	virtual VuEntity *Find(VuEntity *ent) const;
	virtual VU_COLL_TYPE Type() const;

protected:
	typedef std::list< VuEntityBin > VuEntityBinList;
	typedef VuEntityBinList::iterator iterator;
	/** the real list. */
	VuEntityBinList l_;
};
#endif

#if !VU_ALL_FILTERED
/** a list with a filter. Filters make list much more faster for search and message handling, 
* since they can discard entities immediately.
*/
class VuFilteredList : public VuLinkedList {
public:
	VuFilteredList(VuFilter *filter);
	virtual ~VuFilteredList();

	// virtual interface implementation
	/** if msg->Entity() is in but shouldnt, remove. If its out and should be in, insert. */
	virtual VU_ERRCODE Handle(VuMessage *msg);
	/** inserts only if filter->Test(entity) */
	virtual VU_ERRCODE Insert(VuEntity *entity);
	/** if filter->RemoveTest(entity), removes. */
	virtual VU_ERRCODE Remove(VuEntity *entity);
	/** same as above, gets entity from database. */
	virtual VU_ERRCODE Remove(VU_ID entityId);
	virtual VU_COLL_TYPE Type() const;

	// new virtual
	/** inserts if filter->RemoveTest(entity) */
	virtual VU_ERRCODE ForcedInsert(VuEntity *entity);


protected:
	VuFilter *filter_;
};
#endif

/** an ordered list, ordered by filter->Compare function. */
#if VU_ALL_FILTERED
class VuOrderedList : public VuLinkedList {
public:
	VuOrderedList(VuFilter *filter);
	virtual ~VuOrderedList();

protected:
	// virtual interface implementation 
	virtual VU_ERRCODE PrivateInsert(VuEntity *entity);
	virtual VU_ERRCODE PrivateRemove(VuEntity *entity);
	virtual bool PrivateFind(VuEntity *entity);
	virtual VU_COLL_TYPE Type() const;
};
#else
class VuOrderedList : public VuFilteredList {
public:
	VuOrderedList(VuFilter *filter);
	virtual ~VuOrderedList();

	// virtual interface implementation 
	virtual VU_ERRCODE Insert(VuEntity *entity);
	/** inserts regardless of filter->Test(). Calls filter->Compare for ordering. */
	virtual VU_ERRCODE ForcedInsert(VuEntity *entity);
	virtual VU_COLL_TYPE Type() const;
};
#endif


/** an associative hash table. The key is the entity ID.i
*	Adds insertion and query functions using Id to vucoll public interface.
 */
#if VU_ALL_FILTERED
class VuHashTable : public VuCollection {
friend class VuHashIterator;
friend class VuDatabase;
public:
	VuHashTable(VuFilter *filter, unsigned int tableSize, unsigned int key = VU_DEFAULT_HASH_KEY);
	virtual ~VuHashTable();

protected:
	// virtual private interface
	virtual VU_ERRCODE PrivateInsert(VuEntity *entity);
	virtual VU_ERRCODE PrivateRemove(VuEntity *entity);
	virtual bool PrivateFind(VuEntity *ent) const;

public:
	/** returns an active entity from hash. */
	VuEntity *Find(VU_ID entityId) const;
	using VuCollection::Remove;
	/** removes all entities sharing the same key. */
	VU_ERRCODE Remove(VU_ID entityId);

	// virtual public interface
	virtual unsigned int Purge(VU_BOOL all = TRUE);
	virtual unsigned int Count() const;
	virtual VU_COLL_TYPE Type() const;

protected:
	/** returns the index for an ID. */
	unsigned int getIndex(VU_ID) const;

// DATA
protected:
	unsigned int capacity_;
	unsigned int key_;
	VuLinkedList *table_;
};

#else
class VuHashTable : public VuCollection {
friend class VuHashIterator;
public:
	VuHashTable(unsigned int tableSize, unsigned int key = VU_DEFAULT_HASH_KEY);
	virtual ~VuHashTable();

	// virtual interface
	/** inserts into hash using Id as key. */
	virtual VU_ERRCODE Insert(VuEntity *entity);
	/** removes entity from hash. */
	virtual VU_ERRCODE Remove(VuEntity *entity);
	/** removes all entities sharing the same key. */
	virtual VU_ERRCODE Remove(VU_ID entityId);
	/** checks if an entity is present. */
	virtual VuEntity *Find(VuEntity *ent) const;
	/** returns an active entity from hash. */
	virtual VuEntity *Find(VU_ID entityId) const;
	virtual unsigned int Purge(VU_BOOL all = TRUE);
	virtual unsigned int Count() const;
	virtual VU_COLL_TYPE Type() const;

protected:
	/** returns the index for an ID. */
	unsigned int getIndex(VU_ID) const;

// DATA
protected:
	unsigned int capacity_;
	unsigned int key_;
	VuLinkedList *table_;
};
#endif

/** a hash table with a filter. */
class VuFilteredHashTable : public VuHashTable {
public:
	VuFilteredHashTable(VuFilter *filter, uint tableSize, uint key = VU_DEFAULT_HASH_KEY);
	virtual ~VuFilteredHashTable();
	virtual VU_ERRCODE Handle(VuMessage *msg);
	virtual VU_ERRCODE ForcedInsert(VuEntity *entity);
	virtual VU_ERRCODE Insert(VuEntity *entity);
	virtual VU_COLL_TYPE Type() const;

protected:
	VuFilter *filter_;
};


/** VU main database interface. Every VuEntity in VU should be present here. 
* Once inside, an entity is considered active (VU_MEM_ACTIVE). 
* When removed, its marked as to be collected (VU_MEM_INACTIVE).
* After garbage collection or removal, its marked as colected (VU_MEM_REMOVED).
* Each active entity has a different ID, no active duplicates allowed.
* VuDatabase uses vuCollectionManager to update registered collections.
*/
#if VU_ALL_FILTERED
class VuDatabase {
	
friend class VuCollectionManager;
friend class VuMainThread;
friend class VuDatabaseIterator;

public:
	/** creates the database of the given size and key. */
	VuDatabase(uint tableSize, uint key = VU_DEFAULT_HASH_KEY);
	/** destroys database, purging */
	virtual ~VuDatabase();

	/** handles a message, passing it to collectionManager so that all other databases handle it */
	VU_ERRCODE Handle(VuMessage *msg);
	
	/** handles a move, passing it to collectionManager so that all grids get updated. */
	void HandleMove(VuEntity *ent, BIG_SCALAR coord1, BIG_SCALAR coord2);

	/** inserts the entity. Depending on the entity->SendCreate() flags of entity, 
	* its insertion will be sent remotely immediately (VU_SC_SEND_OOB), normally (VU_SC_SEND) 
	* or wont be sent at all (VU_SC_SEND_DONT_SEND).
	*/
	VU_ERRCODE Insert(VuEntity *entity);

	/** calls RemovalCallback, sends remotely and marks entity as VU_MEM_INACTIVE. */
	VU_ERRCODE Remove(VuEntity *entity);

	/** like above, on active entitiy. */
	VU_ERRCODE Remove(VU_ID entityId);

	/** returns an active entity with the given ID. */
	VuEntity *Find(VU_ID entityId) const;

	/** like above, but dont send remove over network. */
	VU_ERRCODE SilentRemove(VuEntity *entity);

#if !NO_RELEASE_EVENT
	/** called by DeleteEvent */
	VU_ERRCODE DeleteRemove(VuEntity *entity);
#endif

	/** marks all entities as VU_MEM_INACTIVE before removing from DB. */
	unsigned int Purge(VU_BOOL all = TRUE);

private:

#define BIRTH_LIST 1
#if BIRTH_LIST
	/** inserts entity into all registered collections which accept it (@see VuFilter::Test). 
	* Used by collection manager birth list.
	*/
	void ReallyInsert(VuEntity *e);
#endif

	/** removes entity from all registered collections. Possibly destroying it. 
	* Used by collection manager garbage collector. 
	*/
	void ReallyRemove(VuEntity *e);

	/** common part of all removes in DB (except the real removal).	*/
	VU_ERRCODE CommonRemove(VuEntity *entity);

	/** just like purge, but calls removalCallback for entities removed. */
	virtual int Suspend(VU_BOOL all = TRUE);

	/** the database container. */
	VuHashTable *dbHash_;
};
#else
class VuDatabase : public VuHashTable {
	
friend class VuCollectionManager;
friend class VuMainThread;

public:
	/** creates the database of the given size and key. */
	VuDatabase(uint tableSize, uint key = VU_DEFAULT_HASH_KEY);
	/** destroys database, purging */
	virtual ~VuDatabase();

	/** handles a message, passing it to collectionManager so that all other databases handle it */
	virtual VU_ERRCODE Handle(VuMessage *msg);
	/** handles a move, passing it to collectionManager so that all grids get updated. */
	void HandleMove(VuEntity *ent, BIG_SCALAR coord1, BIG_SCALAR coord2);

	/** inserts the entity. Depending on the entity->SendCreate() flags of entity, 
	* its insertion will be sent remotely immediately (VU_SC_SEND_OOB), normally (VU_SC_SEND) 
	* or wont be sent at all (VU_SC_SEND_DONT_SEND).
	*/
	virtual VU_ERRCODE Insert(VuEntity *entity);

	/** calls RemovalCallback, sends remotely and marks entity as VU_MEM_INACTIVE. */
	virtual VU_ERRCODE Remove(VuEntity *entity);

	/** like above, on active entitiy. */
	virtual VU_ERRCODE Remove(VU_ID entityId);

	/** like above, but dont send remove over network. */
	virtual VU_ERRCODE SilentRemove(VuEntity *entity);

	/** called by DeleteEvent */
	VU_ERRCODE DeleteRemove(VuEntity *entity);

	/** marks all entities as VU_MEM_INACTIVE before removing from DB. */
	virtual unsigned int Purge(VU_BOOL all = TRUE);

	/** database type */
	virtual VU_COLL_TYPE Type(){ return VU_DATABASE_COLLECTION; }

private:

#define BIRTH_LIST 1
#if BIRTH_LIST
	/** inserts entity into all registered collections which accept it (@see VuFilter::Test). 
	* Used by collection manager birth list.
	*/
	void ReallyInsert(VuEntity *e);
#endif

	/** removes entity from all registered collections. Possibly destroying it. 
	* Used by collection manager garbage collector. 
	*/
	void ReallyRemove(VuEntity *e);

	/** common part of all removes in DB (except the real removal).	*/
	VU_ERRCODE CommonRemove(VuEntity *entity);

	/** just like purge, but calls removalCallback for entities removed. */
	virtual int Suspend(VU_BOOL all = TRUE);
};
#endif



/** a balanced map. Associates a key to an entity through a filter.
* Since filter is responsible for keying the entity, all id functions call
* database to get entity first and then call the function with same name using entity.
* This tree accepts multiple entries with same id (multimap).
*/
#if VU_ALL_FILTERED
class VuRedBlackTree : public VuCollection {

friend class VuRBIterator;

public:
	explicit VuRedBlackTree(VuKeyFilter *filter = 0);
	virtual ~VuRedBlackTree();

protected:
	// protected virtual interface implementation
	virtual VU_ERRCODE PrivateInsert(VuEntity *entity);
	virtual VU_ERRCODE PrivateRemove(VuEntity *entity);
	virtual bool PrivateFind(VuEntity *ent) const;

public:
	// public virtual interface implementation
	virtual unsigned int Purge(VU_BOOL all = TRUE);
	virtual unsigned int Count() const;
	virtual VU_COLL_TYPE Type() const;

	/** applies filter key function to entity, returning key. */
	//virtual VU_KEY Key(VuEntity *ent) const;

private:
	/** returns the filter. */
	virtual VuKeyFilter *GetKeyFilter() const;

	typedef std::multimap<VU_KEY, VuEntityBin> RBMap;
	RBMap map_;

};
#else
class VuRedBlackTree : public VuCollection {

friend class VuRBIterator;

public:
	explicit VuRedBlackTree(VuKeyFilter *filter = 0);
	virtual ~VuRedBlackTree();

	// virtual interface implementation

	/** handles message. */
	virtual VU_ERRCODE Handle(VuMessage *msg);

	/** inserts entity into tree. */
	virtual VU_ERRCODE Insert(VuEntity *entity);

	/** removes entity from tree. */
	virtual VU_ERRCODE Remove(VuEntity *entity);

	/** calls database to get entity and remove it. */
	virtual VU_ERRCODE Remove(VU_ID entityId);

	/** purges tree. */
	virtual unsigned int Purge(VU_BOOL all = TRUE);

	/** return number of active entities. */
	virtual unsigned int Count() const;

	/** returns an active entity with the given id. */
	virtual VuEntity *Find(VU_ID entityId) const;

	/** checks if an entity is in tree. */
	virtual VuEntity *Find(VuEntity *ent) const;

	virtual VU_COLL_TYPE Type() const;

	/** applies filter key function to entity, returning key. */
	VU_KEY Key(VuEntity *ent) const;

	/** bypass filter->Test. */
	VU_ERRCODE ForcedInsert(VuEntity *entity);


private:
	typedef std::multimap<VU_KEY, VuEntityBin> RBMap;
	RBMap map_;
	VuKeyFilter *filter_;
};
#endif

/** divides an area into several rows, each one a RB tree.
* The collection filter will be responsible for passing the key values.
* Key1 is mapped to row, key2 is mapped to column.
*/
#if VU_ALL_FILTERED

class VuGridTree : public VuCollection {

friend class VuGridIterator;
friend class VuCollectionManager;

public:
	/** builds a grid res x res */
	VuGridTree(VuBiKeyFilter *filter, uint res);
	virtual ~VuGridTree();

private:
	/** called by collection manager to handle an entity move. */
	VU_ERRCODE Move(VuEntity *entity, BIG_SCALAR c1, BIG_SCALAR c2);
	/** stops updating tree when entities moves. */
	void SuspendUpdates() { suspendUpdates_ = TRUE; }
	/** resumes updating tree when entities move. */
	void ResumeUpdates() { suspendUpdates_ = FALSE; }
	/** returns the filter casted correctly. */
	VuBiKeyFilter *GetBiKeyFilter() const;

protected:
	// protected interface 
	virtual VU_ERRCODE PrivateInsert(VuEntity *entity);
	virtual VU_ERRCODE PrivateRemove(VuEntity *entity);
	virtual bool PrivateFind(VuEntity *entity) const;
	
public:
	// public interface
	virtual unsigned int Purge(VU_BOOL all = TRUE);
	virtual unsigned int Count() const;
	virtual VU_COLL_TYPE Type() const;

// DATA
protected:
	VuRedBlackTree **table_;
	unsigned int res_; ///< grid resolution (res x res)
	VU_BOOL suspendUpdates_;

private:
	VuGridTree *nextgrid_;
};


#else
class VuGridTree : public VuCollection {

friend class VuGridIterator;
friend class VuCollectionManager;

public:
	VuGridTree(VuBiKeyFilter *filter, uint numrows, BIG_SCALAR center, BIG_SCALAR radius);
	virtual ~VuGridTree();

	virtual VU_ERRCODE ForcedInsert(VuEntity *entity);
	virtual VU_ERRCODE Insert(VuEntity *entity);
	virtual VU_ERRCODE Remove(VuEntity *entity);
	virtual VU_ERRCODE Remove(VU_ID entityId);
	virtual VU_ERRCODE Move(VuEntity *entity,BIG_SCALAR coord1,BIG_SCALAR coord2);
	virtual unsigned int Purge(VU_BOOL all = TRUE);
	virtual unsigned int Count() const;
	virtual VuEntity *Find(VU_ID entityId) const;
	virtual VuEntity *Find(VuEntity *ent) const;

	void SuspendUpdates() { suspendUpdates_ = TRUE; }
	void ResumeUpdates() { suspendUpdates_ = FALSE; }

	//void DebugString(char *buf);
	virtual VU_COLL_TYPE Type() const;

protected:
	unsigned int Row(VU_KEY key1) const;

// DATA
protected:
	VuRedBlackTree **table_;
	VuBiKeyFilter *filter_;
	unsigned int rowcount_;
	VU_KEY bottom_;
	VU_KEY top_;
	VU_KEY rowheight_;
	SM_SCALAR invrowheight_;
	VU_BOOL suspendUpdates_;

private:
	VuGridTree *nextgrid_;
};
#endif


#if 0
///////////////////
// sfr: NOT USED //
///////////////////
/** last in, first out (stack). */
class VuLifoQueue : public VuFilteredList {
public:
	VuLifoQueue(VuFilter *filter);
	virtual ~VuLifoQueue();

	virtual VU_COLL_TYPE Type() const;

	VU_ERRCODE Push(VuEntity *entity) { return ForcedInsert(entity); }
	VuEntity *Peek();
	VuEntity *Pop();
};

/** first in first out. */
class VuFifoQueue : public VuFilteredList {
public:
	VuFifoQueue(VuFilter *filter);
	virtual ~VuFifoQueue();

	// virtual interface implementation
	virtual VU_ERRCODE Insert(VuEntity *entity);
	virtual VU_ERRCODE Remove(VuEntity *entity);
	virtual VU_ERRCODE Remove(VU_ID entityId);
	virtual unsigned int Purge(VU_BOOL all = TRUE);
	virtual VU_COLL_TYPE Type() const;

	// new function
	virtual VU_ERRCODE ForcedInsert(VuEntity *entity);
	VU_ERRCODE Push(VuEntity *entity) { return ForcedInsert(entity); }
	VuEntity *Peek();
	VuEntity *Pop();

protected:
};

class VuAntiDatabase : public VuHashTable {
public:
  VuAntiDatabase(uint tableSize, uint key = VU_DEFAULT_HASH_KEY);
  virtual ~VuAntiDatabase();

  virtual VU_ERRCODE Insert(VuEntity *entity);
  virtual VU_ERRCODE Remove(VuEntity *entity);
  virtual VU_ERRCODE Remove(VU_ID entityId);

  virtual VU_COLL_TYPE Type() const;
  virtual int Purge(VU_BOOL all = TRUE);  // purges all from database
};

#endif


#endif // VU_COLLECTION_H
