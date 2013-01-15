#ifndef VUSESS_H_
#define VUSESS_H_

#include <list>
// sfr: uses list instead of map... may be faster than map
#define SESSION_USES_LIST_FOR_PU 1
#if !SESSION_USES_LIST_FOR_PU
#include <map>
#endif
#include <vector>
#include "vuentity.h"
#include "vuevent.h"
#include "vucoll.h"

#define VU_SESSION_NULL_CONNECTION	vuNullId
#define VU_SESSION_NULL_GROUP		vuNullId

#define VU_DEFAULT_PLAYER_NAME 		"anonymous"
#define VU_GAME_GROUP_NAME    	    "Vu2 Game"
#define VU_PLAYER_POOL_GROUP_NAME	"Player Pool"

#define PACKET_HDR_SIZE (vuKnownConnectionId ? 0 : sizeof(VU_SESSION_ID) + sizeof(VU_ID_NUMBER))
#define MIN_MSG_HDR_SIZE (2)     // 1 for type, 1 for min length
#define MAX_MSG_HDR_SIZE (MIN_MSG_HDR_SIZE + 1)
#define MSG_HDR_SIZE (MAX_MSG_HDR_SIZE) // assume larger
#define MIN_HDR_SIZE (PACKET_HDR_SIZE + MAX_MSG_HDR_SIZE)
#define TWO_PLAYER_HDR_SIZE (MSG_HDR_SIZE)

class VuGroupEntity;
class VuGameEntity;
typedef VuBin<VuGameEntity> VuGameBin;

enum VuSessionSync {
	VU_NO_SYNC,
	VU_SLAVE_SYNC,
	VU_MASTER_SYNC,
};

//----------------------------------------------------------------------
// VuCommsContext -- struct containing comms data
//----------------------------------------------------------------------
enum VuCommsConnectStatus {
	VU_CONN_INACTIVE,
	VU_CONN_PENDING,
	VU_CONN_ACTIVE,
	VU_CONN_ERROR,
};

struct VuCommsContext {
	ComAPIHandle          handle_;
	VuCommsConnectStatus  status_;
	VU_BOOL               reliable_;
	int                   maxMsgSize_;
	int					maxPackSize_;
	// outgoing data
	VU_BYTE               *normalSendPacket_;
	VU_BYTE               *normalSendPacketPtr_;
	VU_ID                 normalPendingSenderId_;
	VU_ID                 normalPendingSendTargetId_;
	// outgoing data
	VU_BYTE               *lowSendPacket_;
	VU_BYTE               *lowSendPacketPtr_;
	VU_ID                 lowPendingSenderId_;
	VU_ID                 lowPendingSendTargetId_;
	// incoming data
	VU_BYTE               *recBuffer_;
	// incoming message portion
	VU_MSG_TYPE           type_;
	int                   length_;
	ushort                msgid_;
	VU_ID                 targetId_;
	VU_TIME               timestamp_;
	// ping data
	int					ping;
};


//----------------------------------------------------------------------
// VuTargetEntity -- base class -- target for all messages
//----------------------------------------------------------------------
class VuTargetEntity : public VuEntity {
public:
	virtual ~VuTargetEntity();

	// virtual function interface
	virtual int SaveSize();
	virtual int Save(VU_BYTE **stream);
	virtual int Save(FILE *file);

	// Special VU type getters
	virtual VU_BOOL IsTarget();	// returns TRUE

	virtual VU_BOOL HasTarget(VU_ID id)=0; // TRUE --> id contains (or is) ent
	virtual VU_BOOL InTarget(VU_ID id)=0;  // TRUE --> ent contained by (or is) id

	virtual VuTargetEntity *ForwardingTarget(VuMessage *msg = 0);

	int FlushOutboundMessageBuffer();
	int SendMessage(VuMessage *msg);
	int GetMessages();
	int SendQueuedMessage();

	void SetDirty (void);
	void ClearDirty (void);
	int IsDirty(void);
	  
	// normal (best effort) comms handle
	VuCommsConnectStatus GetCommsStatus() { return bestEffortComms_.status_; }
	ComAPIHandle GetCommsHandle() { return bestEffortComms_.handle_; }
	void SetCommsStatus(VuCommsConnectStatus cs) { bestEffortComms_.status_ = cs;}
	void SetCommsHandle(ComAPIHandle ch, int bufSize=0, int packSize=0);

	// reliable comms handle
	VuCommsConnectStatus GetReliableCommsStatus() {return reliableComms_.status_;}
	ComAPIHandle GetReliableCommsHandle() { return reliableComms_.handle_; }
	void SetReliableCommsStatus(VuCommsConnectStatus cs) { reliableComms_.status_ = cs; }
	void SetReliableCommsHandle(ComAPIHandle ch, int bufSize=0, int packSize=0);

	int BytesPending();
	int MaxPacketSize();
	int MaxMessageSize();
	int MaxReliablePacketSize();
	int MaxReliableMessageSize();

protected:
	void CloseComms();
	int Flush(VuCommsContext *ctxt);
	int FlushLow(VuCommsContext *ctxt);
	int SendOutOfBand(VuCommsContext *ctxt, VuMessage *msg);
	int SendNormalPriority(VuCommsContext *ctxt, VuMessage *msg);
	int SendLowPriority(VuCommsContext *ctxt, VuMessage *msg);


	VuTargetEntity(ushort type, VU_ID_NUMBER eid);
	VuTargetEntity(VU_BYTE **stream, long *rem);
	VuTargetEntity(FILE *file);

private:
	int LocalSize();                      // returns local bytes written

//data
protected:
	VuCommsContext bestEffortComms_;
	VuCommsContext reliableComms_;
	int dirty;
};

//-----------------------------------------
struct VuGroupNode {
	VU_ID gid_;
	VuGroupNode *next_;
};
enum VU_GAME_ACTION {
	VU_NO_GAME_ACTION,
	VU_JOIN_GAME_ACTION,
	VU_CHANGE_GAME_ACTION,
	VU_LEAVE_GAME_ACTION
};
//-----------------------------------------
class VuSessionEntity : public VuTargetEntity {
friend class VuMainThread;
public:
	// constructors & destructor
	VuSessionEntity(ulong domainMask, const char *callsign);
	VuSessionEntity(VU_BYTE **stream, long *rem);
	VuSessionEntity(FILE *file);
	virtual ~VuSessionEntity();

	// virtual function interface
	virtual int SaveSize();
	virtual int Save(VU_BYTE **stream);
	virtual int Save(FILE *file);

	// accessors
	ulong DomainMask()		{ return domainMask_; }
	VU_SESSION_ID SessionId()	{ return sessionId_; }
	//sfr: converts
	// ip port info
	VU_ADDRESS GetAddress()   { return address_; }
	char *Callsign()		{ return callsign_; }
	VU_BYTE LoadMetric()		{ return loadMetric_; }
	VU_ID GameId();
	VuGameEntity *Game();
	int LastMessageReceived()	{ return lastMsgRecvd_; }
	VU_TIME KeepaliveTime();

	// setters
	//sfr: converts
	//ip port info
	void SetAddress(VU_ADDRESS add){address_ = add;}
	void SetCallsign(const char *callsign);
	void SetLoadMetric(VU_BYTE newMetric) { loadMetric_ = newMetric; }
	VU_ERRCODE JoinGroup(VuGroupEntity *newgroup);  //  < 0 retval ==> failure
	VU_ERRCODE LeaveGroup(VuGroupEntity *group);
	VU_ERRCODE LeaveAllGroups();
	VU_ERRCODE JoinGame(VuGameEntity *newgame);  //  < 0 retval ==> failure
	VU_GAME_ACTION GameAction() { return action_; }
	void SetLastMessageReceived(int id)	{ lastMsgRecvd_ = id; }
	void SetKeepaliveTime(VU_TIME ts);


#ifdef VU_SIMPLE_LATENCY
	int TimeDelta()		{ return timeDelta_; }
	void SetTimeDelta(int delta) { timeDelta_ = delta; }
	int Latency()		{ return latency_; }
	void SetLatency(int latency) { latency_ = latency; }
#endif //VU_SIMPLE_LATENCY
#ifdef VU_TRACK_LATENCY
	VU_BYTE TimeSyncState()	{ return timeSyncState_; }
	VU_TIME Latency()		{ return latency_; }
	void SetTimeSync(VU_BYTE newstate);
	void SetLatency(VU_TIME latency);
#endif //VU_TRACK_LATENCY

	// Special VU type getters
	virtual VU_BOOL IsSession();	// returns TRUE

	virtual VU_BOOL HasTarget(VU_ID id);  // TRUE --> id contains (or is) ent
	virtual VU_BOOL InTarget(VU_ID id);   // TRUE --> ent contained by (or is) id

	VU_ERRCODE AddGroup(VU_ID gid);
	VU_ERRCODE RemoveGroup(VU_ID gid);
	VU_ERRCODE PurgeGroups();

	virtual VuTargetEntity *ForwardingTarget(VuMessage *msg = 0);

	// camera stuff
	int CameraCount() const { return cameras_.size(); }
	VuEntity *GetCameraEntity(unsigned char index) const;
	// silent means session wont be marked as dirty
	VU_ERRCODE AttachCamera(VuEntity *entity, bool silent = false);
	VU_ERRCODE RemoveCamera(VuEntity *entity, bool silent = false);
	void ClearCameras(bool silent = false);

	// position update stuff sfr: TODO this stuff
	/** enqeue an entity for sending position update. Smaller the distance, bigger the chance to send */
	void EnqueuePositionUpdate(SM_SCALAR distance, VuEntity *entity);

	/** Enqueue an entity to have its position update sent next update, regardless of bw. */
	void EnqueueOobPositionUpdate(VuEntity *entity);

	/** send best enqeued position updates and discards the rest*/
	void SendBestEnqueuedPositionUpdatesAndClear(unsigned int qty, VU_TIME timestamp);

	// event Handlers
	virtual VU_ERRCODE Handle(VuEvent *event);
	virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);
	virtual VU_ERRCODE Handle(VuSessionEvent *event);

protected:
	VuSessionEntity(ushort typeindex, ulong domainMask, char *callsign);
	VU_SESSION_ID OpenSession();	// returns session id
	void CloseSession();

	/** called when session is inserted into DB. */
	virtual VU_ERRCODE InsertionCallback();

	/** called prior to GC entity. */
	virtual VU_ERRCODE RemovalCallback();

private:
	int LocalSize();                      // returns local bytes written

// DATA
protected:
	// shared data
	VU_SESSION_ID sessionId_;
	// IP / port information
	VU_ADDRESS address_;
	ulong domainMask_;
	char *callsign_;
	VU_BYTE loadMetric_;
	VU_ID gameId_;
	VU_BYTE groupCount_;
	VuGroupNode *groupHead_;
#ifdef VU_SIMPLE_LATENCY
	int timeDelta_;
	int latency_;
#endif //VU_SIMPLE_LATENCY;
#ifdef VU_TRACK_LATENCY
	VU_BYTE timeSyncState_;
	VU_TIME latency_;
	VU_TIME masterTime_;		// time from master
	VU_TIME masterTimePostTime_;	// local time of net msg post
	VU_TIME responseTime_;		// local time local msg post
	VU_SESSION_ID masterTimeOwner_;	// sender of master msg 
	// local data
	// time synchro statistics
	VU_TIME lagTotal_;
	int lagPackets_;
	int lagUpdate_;	// when packets > update, change latency value
#endif //VU_TRACK_LATENCY
	// msg tracking
	int lastMsgRecvd_;
	typedef std::vector< VuBin<VuEntity> > VuEntityVector;
	typedef VuEntityVector::iterator VuEntityVectorIterator;
	VuEntityVector cameras_;

#if SESSION_USES_LIST_FOR_PU
	typedef std::pair<SM_SCALAR, VuEntity*> ScoreEntityPair; 
	class ScoreEntityPairSort {
	public:
		bool operator ()(ScoreEntityPair &s1, ScoreEntityPair &s2) const {
			return s1.first < s2.first;
		}
	};
	typedef std::list< ScoreEntityPair > VuPositionUpdateQ;
#else
	typedef std::multimap< SM_SCALAR, VuEntity* > VuPositionUpdateQ;;
#endif
	typedef std::list< VuEntity * > VuOobPositionUpdateQ;
	// sfr: position update queue to be sent for this session
	VuPositionUpdateQ positionUpdateQ_;
	VuOobPositionUpdateQ oobPositionUpdateQ_;
	// local data
	VuGameBin game_;
	VU_GAME_ACTION action_;
};

//-----------------------------------------
class VuGroupEntity : public VuTargetEntity {

friend class VuSessionsIterator;
friend class VuSessionEntity;

public:
	// constructors & destructor
	VuGroupEntity(char *groupname);
	//sfr: added rem
	VuGroupEntity(VU_BYTE **stream, long *rem);
	VuGroupEntity(FILE *file);
	virtual ~VuGroupEntity();

	// virtual function interface
	virtual int SaveSize();
	virtual int Save(VU_BYTE **stream);
	virtual int Save(FILE *file);

	char *GroupName()     { return groupName_; }
	ushort MaxSessions()  { return sessionMax_; }
	int SessionCount()    { return sessionCollection_->Count(); }

	// setters
	void SetGroupName(char *groupname);
	void SetMaxSessions(ushort max) { sessionMax_ = max; }

	virtual VU_BOOL HasTarget(VU_ID id);
	virtual VU_BOOL InTarget(VU_ID id);
	VU_BOOL SessionInGroup(VuSessionEntity *session);
	virtual VU_ERRCODE AddSession(VuSessionEntity *session);
	VU_ERRCODE AddSession(VU_ID sessionId);
	virtual VU_ERRCODE RemoveSession(VuSessionEntity *session);
	VU_ERRCODE RemoveSession(VU_ID sessionId);

	virtual VU_BOOL IsGroup();	// returns TRUE

	// event Handlers
	virtual VU_ERRCODE Handle(VuSessionEvent *event);
	virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);

protected:
	VuGroupEntity(int type, char *groupname, VuFilter *filter=0);
	virtual VU_ERRCODE Distribute(VuSessionEntity *ent);
	virtual VU_ERRCODE InsertionCallback();
	virtual VU_ERRCODE RemovalCallback();

private:
	int LocalSize();                      // returns local bytes written

// DATA
protected:
	char *groupName_;
	ushort sessionMax_;
	VuOrderedList *sessionCollection_;

	// scratch data
	int selfIndex_; 
};

//-----------------------------------------
class VuGameEntity : public VuGroupEntity {

friend class VuSessionsIterator;
friend class VuSessionEntity;

public:
	// constructors & destructor
	VuGameEntity(ulong domainMask, char *gamename);
	VuGameEntity(VU_BYTE **stream, long *rem);
	VuGameEntity(FILE *file);
	virtual ~VuGameEntity();

	// virtual function interface
	virtual int SaveSize();
	virtual int Save(VU_BYTE **stream);
	virtual int Save(FILE *file);

	// accessors
	ulong DomainMask()		{ return domainMask_; }
	char *GameName()		{ return gameName_; }
	ushort MaxSessions() { return sessionMax_; }
	// sfr: duplicated from group
	//int SessionCount() { return sessionCollection_->Count(); }

	// setters
	void SetGameName(char *groupname);
	void SetMaxSessions(ushort max) { sessionMax_ = max; }

	virtual VU_ERRCODE AddSession(VuSessionEntity *session);
	virtual VU_ERRCODE RemoveSession(VuSessionEntity *session);

	virtual VU_BOOL IsGame();	// returns TRUE

	// event Handlers
	virtual VU_ERRCODE Handle(VuSessionEvent *event);
	virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);

protected:
	VuGameEntity(int type, ulong domainMask, char *gamename, char *groupname);
	virtual VU_ERRCODE Distribute(VuSessionEntity *ent);
	virtual VU_ERRCODE RemovalCallback();

private:
	int LocalSize();                      // returns local bytes written

// DATA
protected:
	// shared data
	ulong domainMask_;
	char *gameName_;
};

//-----------------------------------------
class VuPlayerPoolGame : public VuGameEntity {
public:
	// constructors & destructor
	VuPlayerPoolGame(ulong domainMask);

	virtual ~VuPlayerPoolGame();

	// virtual function interface	-- stubbed out here
	virtual int SaveSize();
	virtual int Save(VU_BYTE **stream);
	virtual int Save(FILE *file);

	// do nothing...
	virtual VU_ERRCODE Distribute(VuSessionEntity *ent);

private:
	//sfr: added rem
	VuPlayerPoolGame(VU_BYTE **stream, long *rem);
	VuPlayerPoolGame(FILE *file);

// DATA
protected:
  // none
};
//-----------------------------------------
class VuGlobalGroup : public VuGroupEntity {
public:
	// constructors & destructor
	VuGlobalGroup();

	virtual ~VuGlobalGroup();

	// virtual function interface	-- stubbed out here
	virtual int SaveSize();
	virtual int Save(VU_BYTE **stream);
	virtual int Save(FILE *file);

	virtual VU_BOOL HasTarget(VU_ID id);          // always returns TRUE
	VU_BOOL Connected() { return connected_; }
	void SetConnected(VU_BOOL conn) { connected_ = conn; }

private:
	//sfr: added rem
	VuGlobalGroup(VU_BYTE **stream, long *rem);
	VuGlobalGroup(FILE *file);

// DATA
protected:
	VU_BOOL connected_;
};

#endif // _VUSESSION_H_
