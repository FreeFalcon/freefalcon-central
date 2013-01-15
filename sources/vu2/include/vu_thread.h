#ifndef VU_THREAD_H
#define VU_THREAD_H

/** @file vu_thread.h 
* vu threads structures. Every thread which wants to receive messages should have a VuBaseThread
* unless its the main thread, which is responsible for collecting messages for sending and 
* distributing received messages among all other threads.
*/

#include "vu_fwd.h"
#include "vutypes.h"
#include "../../comms/capi.h"

#define VU_DEFAULT_QUEUE_SIZE		100

/** base class for vu threads data stuff. */
class VuBaseThread {
protected:
	/** base class constructor with the message queue */
	VuBaseThread(VuMessageQueue *messageQueue_);
	/** flushes and delete the queue. */
	virtual ~VuBaseThread();
public:
	// sfr: removed not used
	//VuMessageQueue *Queue() const { return const_cast<VuBaseThread*>(this)->(messageQueue_); }

#define CAP_DISPATCH 1
	/** dispatches messages from queue. Spend at most maxTime ms in it (-1 for none). */
#if CAP_DISPATCH
	virtual void Update(int maxTime);
#else
	virtual void Update();
#endif

protected:
	/** the message queue for this thread. */
	VuMessageQueue *messageQueue_;
};

/** threads use this */
class VuThread : public VuBaseThread {
public:
	/** creates a VuMessageQueue with the given filter and queue size. */
	VuThread(VuMessageFilter *filter, int queueSize = VU_DEFAULT_QUEUE_SIZE);
	// sfr: removed not used
	//VuThread(int queueSize = VU_DEFAULT_QUEUE_SIZE);
	virtual ~VuThread();
};

/** main thread uses this. This is falcon core. */
class VuMainThread : public VuBaseThread {
public:
	/** initializes vu databases. */
	VuMainThread(
		int dbSize, 
		VuMessageFilter *filter = NULL,
		int queueSize = VU_DEFAULT_QUEUE_SIZE,
		VuSessionEntity *(*sessionCtorFunc)(void) = 0
	);
	/** finalizes vu database. */
	virtual ~VuMainThread();

#if CAP_DISPATCH
	/** sends and receives messages for everyone, placing in correct queues. 
	* @param mxTime maximum time to spend dispatching messages.
	*/
	virtual void Update(int mxTime);
#else
	virtual void Update();
#endif

	/** called when local player joins game. */
	VU_ERRCODE JoinGame(VuGameEntity *ent);

	/** called when local player leaves game. */
	VU_ERRCODE LeaveGame();

	/** initializes network comms. */
	VU_ERRCODE InitComms(
		ComAPIHandle handle, int bufSize=0, int packSize=0, 
		ComAPIHandle reliablehandle = NULL, int relBufSize=0, int relPackSize=0,
		int resendQueueSize = VU_DEFAULT_QUEUE_SIZE 
	);

	/** finalizes network comms. */
	VU_ERRCODE DeinitComms();

	/** flushes the outgoing message queues. */
	void FlushOutboundMessages();

	/** returns the send message queue. */
	VuPendingSendQueue *SendQueue() const { return const_cast<VuMainThread*>(this)->sendQueue_; }

private:
	/** initialization function, called by constructors. */
	void Init(int dbSize, VuSessionEntity *(*sessionCtorFunc)(void));

	/** updates group data. */
	void UpdateGroupData(VuGroupEntity *group);

	/** gets enqueued remote messages from remote sessions comms. */
	int GetMessages();

	/** sends enqueued comms messages to remote sessions. */
	int SendQueuedMessages();

	/** outgoing queue (for failed sent messages) */
	VuPendingSendQueue *sendQueue_;
};



#endif
