#ifndef VU_MQ_H
#define VU_MQ_H

/** @file vu_mq.h header file for message queue files. */

#include "vutypes.h"

class VuMessage;
class VuMessageFilter;

/** base class for message queues. Works like a circular buffer. */
class VuMessageQueue {
	//friend class VuTargetEntity;
public:
	/** creates and register the mq. */
	VuMessageQueue(int queueSize, VuMessageFilter *filter = 0);

	/** unregister the mq and destroys it. @TODO why dont dispatch messages!! */
	~VuMessageQueue();

	/** returns next message to be read. */
	VuMessage *PeekVuMessage() { return *read_; }

	/** dispatch next to be read. Returns FALSE if none. */
	virtual VU_BOOL DispatchVuMessage(VU_BOOL autod = FALSE);

	/** dispatches messages, returns the number of dispatched messages.
	* @param max maximum number of messages dispatched. -1 for all.
	*/
	int DispatchMessages(int max, VU_BOOL autod = FALSE);

	/** calls eval function on all messages. If it returns true, replace message with an VuUnknownMessage. 
	* @todo sfr: seems not used. 
	*/
	int InvalidateQueueMessages(VU_BOOL (*evalFunc)(VuMessage*, void*), void *);

	// static functions

	/** post a message to the normal or low send queue. 
	* If its loopback, also place on local queues if their filters accept it.
	*/
	static int PostVuMessage(VuMessage *event);

	/** flushes all queues calling dispatch with auto destroy set (dont process messages). */
	static void FlushAllQueues();

	/** creates a timer event with the message, with given delay. 
	* Timer event is then posted. @TODO sfr: check the flags, seems bugged
	*/
	static void RepostMessage(VuMessage *event, int delay);	

protected:
	/** when queue gets full, reallocates and copies all messages. @TODO sfr: check this copy seems bugged. */
	virtual VU_BOOL ReallocQueue();

	/** adds a message to this queue. called only by PostVuMessage(). */
	virtual VU_BOOL AddMessage(VuMessage *event);

protected:
	VuMessage **head_;     ///< queue head
	VuMessage **read_;     ///< next to read
	VuMessage **write_;    ///< place to write
	VuMessage **tail_;     ///< last position for wrapping around
	VuMessageFilter *filter_;  ///< determines which types of messages are accepted in queue

private:
	/** registered queues. */
	static VuMessageQueue *queuecollhead_;
	/** next registed queue after this. */
	VuMessageQueue *nextqueue_;
};

/** the main message queue */
class VuMainMessageQueue : public VuMessageQueue {
public:
	/** creates the main queue with empty timer list. */
	VuMainMessageQueue(int queueSize, VuMessageFilter *filter = 0);
	/** null event list. */
	~VuMainMessageQueue();

	/** dispatches all expired timers and then one message from queue. */
	virtual VU_BOOL DispatchVuMessage(VU_BOOL autod = FALSE);

protected:
	/** if message is timer event, adds to time list (sorted from oldest mark to newest mark). 
	* Otherwise, adds normally. 
	*/
	virtual VU_BOOL AddMessage(VuMessage *event); // called only by PostVuMessage()

protected:
	/** list of VuTimeEvents. */
	VuTimerEvent *timerlisthead_;
};

/** message queue for sending messages. */
class VuPendingSendQueue : public VuMessageQueue {
friend class VuMessageQueue;
public:
	/** creates a pending send queue with 0 pending bytes. */
	VuPendingSendQueue(int queueSize);
	/** dispatches all messages before destroying. */
	~VuPendingSendQueue();

	/** dispatches a message, sending it. If it fails, message is added to pending queue. */
	virtual VU_BOOL DispatchVuMessage(VU_BOOL autod = FALSE);
	/** invalidates all messages from a given target. */
	void RemoveTarget(VuTargetEntity *target);

	/** return bytes pending. */
	int BytesPending() { return bytesPending_; }

protected:
	/** adds a message to the queue, updating pending bytes. called only by PostVuMessage(). */
	virtual VU_BOOL AddMessage(VuMessage *event);

protected:
	/** number of bytes in pending queue. */
	int bytesPending_;
};

#endif


