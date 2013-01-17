#ifndef _VU_PRIVATE_H_
#define _VU_PRIVATE_H_

#include <list>
#include "vutypes.h"
#include "vucoll.h"

/** @file vu_priv.h VU private classes (not exported). */

/** as name says, responsible for managing all registered collection.
* It calls Handle for all collections when a message is processed. 
* It removes entities from all collections when database removes them.
* It can also insert entities in collections when entities are inserted into database.
* It can also clean all collections.
* Used by the VuDatabase class (insertion/removal) and also the VuMainThread::Update 
* (birth list and garbage collector iteration).
*/
class VuCollectionManager {
public:
	VuCollectionManager();
	~VuCollectionManager();

	// register/unregister collections and grids functions
	void Register(VuCollection *coll);
	void DeRegister(VuCollection *coll);
	void GridRegister(VuGridTree *grid);
	void GridDeRegister(VuGridTree *grid);

	// add/remove
	/** adds an entity to all registered collections (except vuDatabase) */
	void Add(VuEntity *ent);
	/** removes entity from all registered collections (except vuDatabase) */
	void Remove(VuEntity *ent);

	// handling
	/** handles a move in all registered grid collections (so they can be updated) */
	int HandleMove(VuEntity *ent, BIG_SCALAR coord1, BIG_SCALAR coord2);
	/** handles message in all collections */
	VU_ERRCODE Handle(VuMessage *msg);

	// shutdown
	void Shutdown(VU_BOOL all);

	/** adds an entity to garbage collector. It will be removed from registered collections
	* when flush gc is called. This is used by VuDatabase. Multiple AddToGc are thread safe with
	* each other.
	*/
	void AddToGc(VuEntity *e);

	/** adds an entity to the birth list. The entity will be inserted into database when 
	* CreateEntitiesAndFlushGc is called.
	* 
	*/
	void AddToBirthList(VuEntity *e);

	/** Creates the entities in birth list and destroys some entities in GC.
	* CALL THIS WHEN NO ONE IS USING VU DATABASE. 
	* This is not thread safe.
	*/
	void CreateEntitiesAndRunGc();

	// query
	/** returns the number of registered collections holding the entity */
	int FindEnt(VuEntity *ent);

private:
	// registered collections and mutexes
	std::list<VuCollection*> collcoll_;
	VuMutex collsMutex_;
	std::list<VuGridTree*> gridcoll_;
	VuMutex gridsMutex_;

	// garbage collector
	VuMutex gcMutex_;                        ///< garbage collector insertion mutex
	std::list<VuEntityBin> gclist_;          ///< list entities to be deleted
	VuMutex birthMutex_;                      ///< birth list mutex
	std::list<VuEntityBin> birthlist_;       ///< list entities to be created
};

// vu private globals
extern VuCollectionManager *vuCollectionManager;

#endif // _VU_PRIVATE_H_
