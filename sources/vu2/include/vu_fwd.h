#ifndef VU_FWD_H
#define VU_FWD_H

/** @file vu_fwd.h
* forward declaration of all classes declared in vu
*/

// forward decls
// derived classes are tabbed

struct VuCommsContext;

//========
// Threads
//========
class VuBaseThread;
	class VuThread;
	class VuMainThread;

//==========
// Messages 
//==========
// messages and derived classes
class VuMessage;
	class VuTimingMessage;
	class VuUnknownMessage;
	class VuErrorMessage;
	class VuRequestMessage;
		class VuGetRequest;
		class VuPushRequest;
		class VuPullRequest;
	class VuEvent;
		class VuDeleteEvent;
		//class VuUnmanageEvent;
		//class VuReleaseEvent;
		class VuTransferEvent;
		class VuPositionUpdateEvent;
		class VuBroadcastGlobalEvent;
		class VuEntityCollisionEvent;
		class VuGroundCollisionEvent;
		class VuSessionEvent;
		class VuTimerEvent;
		class VuShutdownEvent;
		class VuCreateEvent;
			class VuManageEvent;
			class VuFullUpdateEvent;

// message filters
class VuMessageFilter;
	class VuNullMessageFilter;
	//class VuMessageTypeFilter;
	class VuStandardMsgFilter;
	class VuResendMsgFilter;

// message queues
class VuMessageQueue;
	class VuMainMessageQueue;
	class VuPendingSendQueue;

//==========
// Entities 
//==========
class VuEntity;
	class VuTargetEntity;
		class VuSessionEntity;
		class VuGroupEntity;
			class VuGlobalGroup;
			class VuGameEntity;
				class VuPlayerPoolGame;

//==========
// VuDriver	
//==========	
class VuDriver;
	class VuDeadReckon;
		class VuMaster;
		class VuDelaySlave;

//==========
// database
//==========
// DBs
class VuCollection;
	class VuRedBlackTree;
	class VuGridTree;
	class VuLinkedList;
#if VU_ALL_FILTERED
		class VuFilteredList;
			class VuOrderedList;
#else
		class VuOrderedList;
#endif
	class VuHashTable;
#if !VU_ALL_FILTERED
		class VuFilteredHashTable;
#endif
		class VuDatabase;
		
// db filters
class VuFilter;
	class VuSessionFilter;
	class VuStandardFilter;
	class VuAssociationFilter;
	class VuTypeFilter;
	class VuOpaqueFilter;
	class VuKeyFilter;
		class VuBiKeyFilter;
		class VuTransmissionFilter;
	

// iterator	
class VuIterator;
	class VuSessionsIterator;
	class VuListIterator;
	class VuHashIterator;
		class VuDatabaseIterator;
	class VuRBIterator;
		class VuGridIterator;
		class VuFullGridIterator;
		class VuLineIterator;
		

#endif

