#ifndef _VUEVENT_H_
#define _VUEVENT_H_

#include "vu_fwd.h"
#include "vutypes.h"
#include "vu.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gVuMsgMemPool;
#endif

// sfr: use enum instead of defines 
#define VU_USE_ENUM_FOR_TYPES 1
#if VU_USE_ENUM_FOR_TYPES
enum VuMessageTypes {
	VU_UNKNOWN_MESSAGE = 0,
	// error message
	VU_ERROR_MESSAGE,
	// request messages
	VU_GET_REQUEST_MESSAGE,
	VU_PUSH_REQUEST_MESSAGE,
	VU_PULL_REQUEST_MESSAGE,
	// internal events
	VU_TIMER_EVENT,
#if !NO_RELEASE_EVENT
	VU_RELEASE_EVENT,
#endif
	// event messages
	VU_DELETE_EVENT = 7,
	//VU_UNMANAGE_EVENT,
	VU_MANAGE_EVENT = 9,
	VU_CREATE_EVENT,
	VU_SESSION_EVENT,
	VU_TRANSFER_EVENT,
	VU_BROADCAST_GLOBAL_EVENT,
	VU_POSITION_UPDATE_EVENT,
	VU_FULL_UPDATE_EVENT,
	VU_RESERVED_UPDATE_EVENT,
	VU_ENTITY_COLLISION_EVENT,
	VU_GROUND_COLLISION_EVENT,
	VU_SHUTDOWN_EVENT,
	VU_TIMING_MESSAGE,
	VU_REQUEST_DUMMY_BLOCK_MESSAGE,
	VU_LAST_EVENT
};
#else
#define VU_UNKNOWN_MESSAGE		0		// 0x00000001
// error message
#define VU_ERROR_MESSAGE		1		// 0x00000002
// request messages
#define VU_GET_REQUEST_MESSAGE		2		// 0x00000004
#define VU_PUSH_REQUEST_MESSAGE		3		// 0x00000008
#define VU_PULL_REQUEST_MESSAGE		4		// 0x00000010
// internal events
#define VU_TIMER_EVENT			5		// 0x00000020
#if !NO_RELEASE_EVENT
#define VU_RELEASE_EVENT		6		// 0x00000040
#endif
// event messages
#define VU_DELETE_EVENT			            7		// 0x00000080
//#define VU_UNMANAGE_EVENT		            8		// 0x00000100
#define VU_MANAGE_EVENT			            9		// 0x00000200
#define VU_CREATE_EVENT			            10		// 0x00000400
#define VU_SESSION_EVENT                    11		// 0x00000800
#define VU_TRANSFER_EVENT		            12		// 0x00001000
#define VU_BROADCAST_GLOBAL_EVENT           13		// 0x00002000
#define VU_POSITION_UPDATE_EVENT	        14		// 0x00004000
#define VU_FULL_UPDATE_EVENT		        15		// 0x00008000
#define VU_RESERVED_UPDATE_EVENT	        16		// 0x00010000 ***
#define VU_ENTITY_COLLISION_EVENT	        17		// 0x00020000 ***
#define VU_GROUND_COLLISION_EVENT	        18		// 0x00040000 ***
#define VU_SHUTDOWN_EVENT	                19		// 0x00080000
#define VU_TIMING_MESSAGE			        20		// 0x00100000
#define VU_REQUEST_DUMMY_BLOCK_MESSAGE      21      // sfr: added for NAT stuff
#define VU_LAST_EVENT				        21		// 0x00100000
// handy-dandy bit combinations
#define VU_VU_MESSAGE_BITS		0x001ffffe
#define VU_REQUEST_MSG_BITS		0x0000001c
#define VU_VU_EVENT_BITS		0x000fffe0
#define VU_DELETE_EVENT_BITS		0x000800c0
#define VU_CREATE_EVENT_BITS		0x00000600
#define VU_TIMER_EVENT_BITS		0x00000020
#define VU_INTERNAL_EVENT_BITS		0x00080060
#define VU_EXTERNAL_EVENT_BITS		0x0017ff82
#define VU_USER_MESSAGE_BITS		0xffe00000
#endif // use enum

// error messages
#define VU_UNKNOWN_ERROR				0
#define VU_NO_SUCH_ENTITY_ERROR			1
#define VU_CANT_MANAGE_ENTITY_ERROR		2	// for push request denial
#define VU_DONT_MANAGE_ENTITY_ERROR		3	// for pull request denial
#define VU_CANT_TRANSFER_ENTITY_ERROR	4	// for non-transferrable ents
#define VU_TRANSFER_ASSOCIATION_ERROR	5	// for association errors
#define VU_NOT_AVAILABLE_ERROR			6	// session too busy or exiting

#define VU_LAST_ERROR			99

// timer types
#define VU_UNKNOWN_TIMER		0
#define VU_DELETE_TIMER			1
#define VU_DELAY_TIMER			1
// MD -- 20050102: make bandwidth deferred message go back to VuMessageQueue::PostVuMessage
#define VU_POSTVUMSGBW_TIMER	2
#define VU_LAST_TIMER			99

// message flags
#define VU_NORMAL_PRIORITY_MSG_FLAG 0x01	// send normal priority
#define VU_OUT_OF_BAND_MSG_FLAG     0x02	// send unbuffered
#define VU_KEEPALIVE_MSG_FLAG       0x04	// this is a keepalive msg
#define VU_RELIABLE_MSG_FLAG        0x08	// attempt to send reliably
#define VU_LOOPBACK_MSG_FLAG        0x10	// post msg to self as well
#define VU_REMOTE_MSG_FLAG          0x20	// msg came from outside
#define VU_SEND_FAILED_MSG_FLAG     0x40	// msg has been sent
#define VU_PROCESSED_MSG_FLAG       0x80	// msg has been processed

// session event subtypes
enum vuEventTypes {
	VU_SESSION_UNKNOWN_SUBTYPE	= 0,
	VU_SESSION_CLOSE,
	VU_SESSION_JOIN_GAME,
	VU_SESSION_CHANGE_GAME,
	VU_SESSION_JOIN_GROUP,
	VU_SESSION_LEAVE_GROUP,
	VU_SESSION_CHANGE_CALLSIGN,
	VU_SESSION_DISTRIBUTE_ENTITIES,
	VU_SESSION_TIME_SYNC,
	VU_SESSION_LATENCY_NOTICE
};

class VuEntity;
class VuMessage;
class VuTimerEvent;

//--------------------------------------------------
class VuMessage {
	// these classes need full access to message function
	friend int MessageReceive(VU_ID, VU_ADDRESS, VU_ID, VU_MSG_TYPE, VU_BYTE **, int, VU_TIME);
	friend class VuTargetEntity;
	friend class VuMessageQueue;
	friend class VuMainMessageQueue;
	friend class VuPendingSendQueue;
	friend class VuTimerEvent;
#ifdef USE_SH_POOLS
public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gVuMsgMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
public:
	virtual ~VuMessage();

	VU_MSG_TYPE Type() const { return type_; }
	VU_ID Sender() const { return sender_; }
	VU_ID Destination() const { return tgtid_; }
	VU_BOOL IsLocal() const;
	VU_ID EntityId() const { return entityId_; }
	VU_BYTE Flags() const { return flags_; }
	VuEntity *Entity() const { return ent_.get(); }
	VU_TIME PostTime() { return postTime_; }
	VuTargetEntity *Target() const { return target_.get(); }

	void SetPostTime(VU_TIME posttime) { postTime_ = posttime; }
	virtual int Size() const;
protected:
	// sfr: im protecting these since they are used only internally
	//sfr: changed to long*
	//int Read(VU_BYTE **buf, int length);
	/** reads message from buffer */
	int Read(VU_BYTE **buf, long *length);
	/** writes the message to buffer */
	int Write(VU_BYTE **buf);
	/** process the message */
	VU_ERRCODE Dispatch(VU_BOOL autod);
	/** sends message */
	int Send();
public:

	void RequestLoopback() { flags_ |= VU_LOOPBACK_MSG_FLAG; }
	void RequestReliableTransmit() { flags_ |= VU_RELIABLE_MSG_FLAG; }
	void RequestOutOfBandTransmit() { flags_ |= VU_OUT_OF_BAND_MSG_FLAG; }
	void RequestLowPriorityTransmit() { flags_ &= 0xf0; }

	// app needs to Ref & UnRef messages they keep around
	// 	most often this need not be done
	int Ref();
	int UnRef();

	// the following determines just prior to sending message whether or not
	// it goes out on the wire (default is TRUE, of course)
	virtual VU_BOOL DoSend();

protected:
	VuMessage(VU_MSG_TYPE type, VU_ID entityId, VuTargetEntity *target,
				VU_BOOL loopback);
	VuMessage(VU_MSG_TYPE type, VU_ID sender, VU_ID target);
	virtual VU_ERRCODE Activate(VuEntity *ent);
	virtual VU_ERRCODE Process(VU_BOOL autod) = 0;
	virtual int Encode(VU_BYTE **buf);
	//sfr: changed to long*
	//virtual int Decode(VU_BYTE **buf, int length);
	virtual int Decode(VU_BYTE **buf, long *rem);
	//VuEntity *SetEntity(VuEntity *ent){ ent_.reset(ent); }
	void SetEntity(VuEntity *ent){ ent_.reset(ent); }

private:
	/** returns the size of the message alone, not derived parts. CANNOT be virtual. */
	int LocalSize() const;

private:
	VU_BYTE refcnt_;		// vu references

protected:
	VU_MSG_TYPE type_;
	VU_BYTE flags_;		// misc flags
	VU_ID sender_;
	VU_ID tgtid_;
	VU_ID entityId_;
	// scratch variables (not networked)
	// sfr: smartpointer
	//VuTargetEntity *target_;
	VuBin<VuTargetEntity> target_;
	VU_TIME postTime_;
private:
	//VuEntity *ent_;
	VuBin<VuEntity> ent_;
};

// sfr: added dummy block message
class VuRequestDummyBlockMessage : public VuMessage {
public:
	// sender version
	VuRequestDummyBlockMessage(VU_ADDRESS address, VuTargetEntity *target);
	// receiver version
	VuRequestDummyBlockMessage(VU_ID sender, VU_ID target);

	// serialize functions
	virtual int Size() const;
	virtual int Decode(VU_BYTE **buf, long *length);
	virtual int Encode(VU_BYTE **buf);

	// process message
	VU_ERRCODE Process(VU_BOOL autod);

private:
	virtual int LocalSize() const;
	// address to send dummy to
	VU_ADDRESS address_;
};


//--------------------------------------------------
class VuErrorMessage : public VuMessage {
public:
	VuErrorMessage(int errorType, VU_ID senderid, VU_ID entityid, VuTargetEntity *target);
	VuErrorMessage(VU_ID senderid, VU_ID targetid);
	virtual ~VuErrorMessage();

	virtual int Size() const;
	//sfr: changed to long *
	//virtual int Decode(VU_BYTE **buf, int length);
	virtual int Decode(VU_BYTE **buf, long *length);
	virtual int Encode(VU_BYTE **buf);
	ushort ErrorType() { return etype_; }

protected:
	virtual VU_ERRCODE Process(VU_BOOL autod);

private:
	int LocalSize() const;

protected:
	VU_ID srcmsgid_;
	short etype_;
};

//--------------------------------------------------
class VuRequestMessage : public VuMessage {
public:
  virtual ~VuRequestMessage();

protected:
  VuRequestMessage(VU_MSG_TYPE type, VU_ID entityid, VuTargetEntity *target);
  VuRequestMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID entityid);
  virtual VU_ERRCODE Process(VU_BOOL autod) = 0;

// DATA
protected:
  // none
};

//--------------------------------------------------
enum VU_SPECIAL_GET_TYPE {
  VU_GET_GAME_ENTS,
  VU_GET_GLOBAL_ENTS
};

class VuGetRequest : public VuRequestMessage {
public:
  VuGetRequest(VU_SPECIAL_GET_TYPE gettype, VuSessionEntity *sess = 0);
  VuGetRequest(VU_ID entityId, VuTargetEntity *target);
  VuGetRequest(VU_ID senderid, VU_ID target);
  virtual ~VuGetRequest();

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);
};

//--------------------------------------------------
class VuPushRequest : public VuRequestMessage {
public:
  VuPushRequest(VU_ID entityId, VuTargetEntity *target);
  VuPushRequest(VU_ID senderid, VU_ID target);
  virtual ~VuPushRequest();

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);
};

//--------------------------------------------------
class VuPullRequest : public VuRequestMessage {
public:
  VuPullRequest(VU_ID entityId, VuTargetEntity *target);
  VuPullRequest(VU_ID senderid, VU_ID target);
  virtual ~VuPullRequest();

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);
};

//--------------------------------------------------
class VuEvent : public VuMessage {
public:
  virtual ~VuEvent();

  virtual int Size() const;
  //sfr: cchanged to long *
  //virtual int Decode(VU_BYTE **buf, int length);
  virtual int Decode(VU_BYTE **buf, long *length);
  virtual int Encode(VU_BYTE **buf);

protected:
  VuEvent(VU_MSG_TYPE type, VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback = FALSE);
  VuEvent(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
  virtual VU_ERRCODE Activate(VuEntity *ent);
  virtual VU_ERRCODE Process(VU_BOOL autod) = 0;

private:
  int LocalSize() const;

// DATA
public:
  // these fields are filled in on Activate()
  VU_TIME updateTime_; 
};

//--------------------------------------------------
class VuCreateEvent : public VuEvent {
public:
	VuCreateEvent(VuEntity *entity, VuTargetEntity *target, VU_BOOL loopback=FALSE);
	VuCreateEvent(VU_ADDRESS senderAddress,VU_ID senderid, VU_ID target);
	virtual ~VuCreateEvent();

	virtual int Size() const;
	//sfr: changed to long *
	//virtual int Decode(VU_BYTE **buf, int length);
	virtual int Decode(VU_BYTE **buf, long *length);
	virtual int Encode(VU_BYTE **buf);
	virtual VU_BOOL DoSend();     // returns TRUE if ent is in database

	VuEntity *EventData() { return expandedData_.get(); }

protected:
	VuCreateEvent(VU_MSG_TYPE type, VuEntity *entity, VuTargetEntity *target,
						VU_BOOL loopback=FALSE);
	//sfr: converts
	// added senderaddress
	VuCreateEvent(VU_MSG_TYPE type, VU_ADDRESS senderAddress, VU_ID senderid, VU_ID target);

	virtual VU_ERRCODE Activate(VuEntity *ent);
	virtual VU_ERRCODE Process(VU_BOOL autod);

private:
	int LocalSize() const;

// data
//protected:
public:
	// sfr: smartpointer
	VuBin<VuEntity> expandedData_;
#ifdef VU_USE_CLASS_INFO
	VU_BYTE classInfo_[CLASS_NUM_BYTES];	// entity class type
#endif
	ushort vutype_;			// entity type
	ushort size_;
	VU_BYTE *data_;
	//sfr: converts
	// sender of this message address, not sent over network
	VU_ADDRESS senderAddress;
};

//--------------------------------------------------
class VuManageEvent : public VuCreateEvent {
public:
  VuManageEvent(VuEntity *entity, VuTargetEntity *target, VU_BOOL loopback=FALSE);
  VuManageEvent(VU_ID senderid, VU_ID target);
  virtual ~VuManageEvent();

// data
protected:
  // none
};

//--------------------------------------------------
class VuDeleteEvent : public VuEvent {
public:
  VuDeleteEvent(VuEntity *entity);
  VuDeleteEvent(VU_ID senderid, VU_ID target);
  virtual ~VuDeleteEvent();

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);
  virtual VU_ERRCODE Activate(VuEntity *ent);

// data
protected:
  // none
};

//--------------------------------------------------
#if 0
class VuUnmanageEvent : public VuEvent {
public:
  VuUnmanageEvent(VuEntity *entity, VuTargetEntity *target,
                        VU_TIME mark, VU_BOOL loopback = FALSE);
  VuUnmanageEvent(VU_ID senderid, VU_ID target);
  virtual ~VuUnmanageEvent();

  virtual int Size() const;
  //sfr: changed to long *
  //virtual int Decode(VU_BYTE **buf, int length);
  virtual int Decode(VU_BYTE **buf, long *rem);
  virtual int Encode(VU_BYTE **buf);

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);

private:
  int LocalSize() const;

// data
public:
  // time of removal
  VU_TIME mark_;
};
#endif

//--------------------------------------------------
#define NO_RELEASE_EVENT 1
#if !NO_RELEASE_EVENT
class VuReleaseEvent : public VuEvent {
public:
	VuReleaseEvent(VuEntity *entity);
	virtual ~VuReleaseEvent();

	virtual int Size() const;
	// all these are stubbed out here, as this is not a net message
	virtual int Decode(VU_BYTE **buf, long *rem);
	virtual int Encode(VU_BYTE **buf);
	virtual VU_BOOL DoSend();     // returns FALSE

protected:
	virtual VU_ERRCODE Activate(VuEntity *ent);
	virtual VU_ERRCODE Process(VU_BOOL autod);

// data
protected:
	// none
};
#endif

//--------------------------------------------------
class VuTransferEvent : public VuEvent {
public:
  VuTransferEvent(VuEntity *entity, VuTargetEntity *target, VU_BOOL loopback=FALSE);
  VuTransferEvent(VU_ID senderid, VU_ID target);
  virtual ~VuTransferEvent();

  virtual int Size() const;
  //sfr: changed to long *
  //virtual int Decode(VU_BYTE **buf, int length);
  virtual int Decode(VU_BYTE **buf, long *length);
  virtual int Encode(VU_BYTE **buf);

protected:
  virtual VU_ERRCODE Activate(VuEntity *ent);
  virtual VU_ERRCODE Process(VU_BOOL autod);

private:
  int LocalSize() const;

// data
public:
  VU_ID newOwnerId_;
};

//--------------------------------------------------
// sfr: byte packing
#pragma pack(1)
class VuPositionUpdateEvent : public VuEvent {
public:

	VuPositionUpdateEvent(VuEntity *entity, VuTargetEntity *target, VU_BOOL loopback=FALSE);
	VuPositionUpdateEvent(VU_ID senderid, VU_ID target);
	virtual ~VuPositionUpdateEvent();

	virtual int Size() const;
	//sfr: changed to long *
	//virtual int Decode(VU_BYTE **buf, int length);
	virtual int Decode(VU_BYTE **buf, long *length);
	virtual int Encode(VU_BYTE **buf);

	virtual VU_BOOL DoSend();
	// these constants are used to turn from radians to byte and vice versa
	#define RAD2CHAR static_cast<SM_SCALAR>(128.0f / VU_PI)
	#define CHAR2RAD static_cast<SM_SCALAR>(VU_PI / 128.0f)
	// sfr: biggest speed I could see was 5000.0f
	#define MAX_SPEED (5000.0f)
	#define D2SHORT  static_cast<SM_SCALAR>(0xFFFF / MAX_SPEED)
	#define SHORT2D  static_cast<SM_SCALAR>(MAX_SPEED / 0xFFFF)

protected:
	virtual VU_ERRCODE Process(VU_BOOL autod);

private:
	int LocalSize() const;

	// data
public:
	//SM_SCALAR dyaw_, dpitch_, droll_;
	SM_SCALAR yaw_, pitch_, roll_; // sfr: does not go to network, using char instead
	char yc_, pc_, rc_;
	BIG_SCALAR x_, y_, z_;
	SM_SCALAR dx_, dy_, dz_;       // sfr: does not go to networt, using short instead
	short sdx_, sdy_, sdz_;
};
#pragma pack()

//--------------------------------------------------
class VuBroadcastGlobalEvent : public VuEvent {
public:
  VuBroadcastGlobalEvent(VuEntity *entity, VuTargetEntity *target, VU_BOOL loopback=FALSE);
  // sfr: converts
  // added address
  VuBroadcastGlobalEvent(VU_ADDRESS senderAddress, VU_ID senderId, VU_ID target);
  virtual ~VuBroadcastGlobalEvent();

  void MarkAsKeepalive() { flags_ |= VU_KEEPALIVE_MSG_FLAG; }

protected:
  virtual int Size() const;
  //sfr: changed to long *
  //virtual int Decode(VU_BYTE **buf, int length);
  virtual int Decode(VU_BYTE **buf, long *length);
  virtual int Encode(VU_BYTE **buf);

  virtual VU_ERRCODE Activate(VuEntity *ent);
  virtual VU_ERRCODE Process(VU_BOOL autod);

// data
protected:

#ifdef VU_USE_CLASS_INFO
  VU_BYTE classInfo_[CLASS_NUM_BYTES];	// entity class type
#endif
  ushort vutype_;			// entity type
  //sfr: converts
  // this tells the entity ID
  // its necessary because when a player joins session
  // server sends a VuBroadcastEvent to the joining client
  // with other clients ID. But there is no address info
  // this isnt the best place to do this, but placing under VuEvent 
  // wont be a good idea either
  VU_ADDRESS entityAddress; //< address of an entity
  // this is not sent over network, but rather infered
  VU_ADDRESS senderAddress; //< address of this message sender
};

//--------------------------------------------------
class VuFullUpdateEvent : public VuCreateEvent {
public:
  VuFullUpdateEvent(VuEntity *entity, VuTargetEntity *target, VU_BOOL loopback=FALSE);
  VuFullUpdateEvent(VU_ADDRESS add, VU_ID senderid, VU_ID target);
  virtual ~VuFullUpdateEvent();

  VU_BOOL WasCreated() { return (VU_BOOL)(Entity() == expandedData_ ? TRUE : FALSE); }
  void MarkAsKeepalive() { flags_ |= VU_KEEPALIVE_MSG_FLAG; }

protected:
  virtual VU_ERRCODE Activate(VuEntity *ent);

// data
protected:
  // none
};

//--------------------------------------------------
class VuEntityCollisionEvent : public VuEvent {
public:
  VuEntityCollisionEvent(VuEntity *entity, VU_ID otherId,
                         VU_DAMAGE hitLocation, int hitEffect,
                         VuTargetEntity *target, VU_BOOL loopback=FALSE);
  VuEntityCollisionEvent(VU_ID senderid, VU_ID target);

  virtual ~VuEntityCollisionEvent();

  virtual int Size() const;
  //sfr: changed to long *
  //virtual int Decode(VU_BYTE **buf, int length);
  virtual int Decode(VU_BYTE **buf, long *length);
  virtual int Encode(VU_BYTE **buf);

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);

private:
  int LocalSize() const;

// data
public:
  VU_ID otherId_;
  VU_DAMAGE hitLocation_;	// affects damage
  int hitEffect_;		// affects hitpoints/health
};

//--------------------------------------------------
class VuGroundCollisionEvent : public VuEvent {
public:
  VuGroundCollisionEvent(VuEntity *entity, VuTargetEntity *target, VU_BOOL loopback=FALSE);
  VuGroundCollisionEvent(VU_ID senderid, VU_ID target);
  virtual ~VuGroundCollisionEvent();

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);

// data
};

//--------------------------------------------------
class VuSessionEvent : public VuEvent {
public:
  VuSessionEvent(VuEntity *entity, ushort subtype, VuTargetEntity *target,
                 VU_BOOL loopback=FALSE);
  VuSessionEvent(VU_ID senderid, VU_ID target);
  virtual ~VuSessionEvent();

  virtual int Size() const;
  //sfr: changed to long *
  //virtual int Decode(VU_BYTE **buf, int length);
  virtual int Decode(VU_BYTE **buf, long *l);
  virtual int Encode(VU_BYTE **buf);

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);

private:
  int LocalSize() const;

// data
public:
  ushort subtype_;
  VU_ID group_;
  char *callsign_;
  VU_BYTE syncState_;
  VU_TIME gameTime_;
};

//--------------------------------------------------
/** used to resend messages in a given time frame. For example, a reliable message fails,
* we can create a timed event with a given delay so the message can be resent after delay
*/
class VuTimerEvent : public VuEvent {
friend class VuMainMessageQueue;
public:
  VuTimerEvent(VuEntity *entity, VU_TIME mark, ushort type, VuMessage *event=0);
  virtual ~VuTimerEvent();

  virtual int Size() const;
  // all these are stubbed out here, as this is not a net message
  //sfr: changed to long *
  //virtual int Decode(VU_BYTE **buf, int length);
  virtual int Decode(VU_BYTE **buf, long *l);
  virtual int Encode(VU_BYTE **buf);
  virtual VU_BOOL DoSend();     // returns FALSE

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);

// data
public:
  // time of firing
  VU_TIME mark_;
  ushort timertype_;
  // event to launch on firing
  VuMessage *event_;

private:
  VuTimerEvent *next_;
};

//--------------------------------------------------
class VuShutdownEvent : public VuEvent {
public:
  VuShutdownEvent(VU_BOOL all = FALSE);
  virtual ~VuShutdownEvent();

  virtual int Size() const;
  // all these are stubbed out here, as this is not a net message
  //sfr: changed to long*
  //virtual int Decode(VU_BYTE **buf, int length);
  virtual int Decode(VU_BYTE **buf, long *l);
  virtual int Encode(VU_BYTE **buf);
  virtual VU_BOOL DoSend();     // returns FALSE

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);

// data
public:
  VU_BOOL shutdownAll_;
  VU_BOOL done_;
};

#ifdef VU_SIMPLE_LATENCY
//--------------------------------------------------
class VuTimingMessage : public VuMessage {
public:
  VuTimingMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=FALSE);
  VuTimingMessage(VU_ID senderid, VU_ID target);
  virtual ~VuTimingMessage();

  virtual int Size() const;
  //sfr: changed to long *
  //virtual int Decode(VU_BYTE **buf, int length);
  virtual int Decode(VU_BYTE **buf, long *l);
  virtual int Encode(VU_BYTE **buf);

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);

// data
public:
	VU_TIME	sessionRealSendTime_;
	VU_TIME sessionGameSendTime_;
	VU_TIME remoteGameTime_;
};
#endif //VU_SIMPLE_LATENCY



//--------------------------------------------------
class VuUnknownMessage : public VuMessage {
public:
  VuUnknownMessage();
  virtual ~VuUnknownMessage();

  virtual int Size() const;
  // all these are stubbed out here, as this is not a net message
  //sfr: changed to long *
  //virtual int Decode(VU_BYTE **buf, int length);
  virtual int Decode(VU_BYTE **buf, long *l);
  virtual int Encode(VU_BYTE **buf);
  virtual VU_BOOL DoSend();     // returns FALSE

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);

// data
public:
  // none
};

#endif // _VUEVENT_H_
