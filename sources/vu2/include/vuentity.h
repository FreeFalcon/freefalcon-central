#ifndef _VUENTITY_H_
#define _VUENTITY_H_

#include <stdio.h>

#include "vu_fwd.h"
#include "vu_templates.h"
#include "vutypes.h"
#include "vumath.h"

#define MAX_VU_STR_LEN                      255

#define VU_UNKNOWN_ENTITY_TYPE              0
#define VU_SESSION_ENTITY_TYPE              1
#define VU_GROUP_ENTITY_TYPE                2
#define VU_GLOBAL_GROUP_ENTITY_TYPE	        3
#define VU_GAME_ENTITY_TYPE                 4
#define VU_PLAYER_POOL_GROUP_ENTITY_TYPE    5
#define VU_LAST_ENTITY_TYPE                 100

#define VU_CREATE_PRIORITY_BASE             100

#define CLASS_NUM_BYTES                     8

// domains
#define VU_GLOBAL_DOMAIN                    0

// Predefined entity ids
#define VU_NULL_ENTITY_ID                   0
#define VU_GLOBAL_GROUP_ENTITY_ID           1
#define VU_PLAYER_POOL_ENTITY_ID            2
#define VU_SESSION_ENTITY_ID                3
#define VU_FIRST_ENTITY_ID                  4       // first generated

// Standard Collision type
#define VU_NON_COLLIDABLE                   0
#define VU_DEFAULT_COLLIDABLE               1

struct VuEntityType {
	ushort id_;
	ushort collisionType_;
	SM_SCALAR collisionRadius_;
	VU_BYTE classInfo_[CLASS_NUM_BYTES];
	VU_TIME updateRate_;
	VU_TIME updateTolerance_;
	SM_SCALAR bubbleRange_;	// max distance to send position updates
	SM_SCALAR fineUpdateForceRange_; // distance to force position updates, sfr: seems this is not used at all...
	SM_SCALAR fineUpdateMultiplier_; // multiplier for noticing position updates
	VU_DAMAGE damageSeed_;
	int hitpoints_;
	ushort majorRevisionNumber_;
	ushort minorRevisionNumber_;
	ushort createPriority_;
	VU_BYTE managementDomain_;
	VU_BOOL transferable_;
	VU_BOOL private_;
	VU_BOOL tangible_;
	VU_BOOL collidable_;
	VU_BOOL global_;
	VU_BOOL persistent_;
};

// keep this 16 bit
struct VuFlagBits {
	uint private_       : 1;  // 1 --> not public
	uint transfer_      : 1;  // 1 --> can be transferred
	uint tangible_      : 1;  // 1 --> can be seen/touched with
	uint collidable_    : 1;  // 1 --> put in auto collision table
	uint global_        : 1;  // 1 --> visible to all groups
	uint persistent_    : 1;  // 1 --> keep ent local across group joins
	uint sendCreate_    : 2;  // 0 --> dont send, 1 send, 2 send immediately (oob)
	uint pad_           : 8;  // unused
};

// function declarations
/** refs entity, thread safe */
int VuReferenceEntity(VuEntity *ent);
/** unrefs entity, thread safe. If count reaches 0, deletes it */
int VuDeReferenceEntity(VuEntity *ent);


/** memory state enumeration */
enum VU_MEM {
	VU_MEM_CREATED        = 0x01, ///< entity has been created (constructor called)
	VU_MEM_TO_BE_INSERTED = 0x02, ///< entity was added to birth list, will be inserted soon
	VU_MEM_ACTIVE         = 0x03, ///< entity inserted into vuDB
	VU_MEM_INACTIVE	      = 0x04, ///< entity marked for collection
	VU_MEM_REMOVED        = 0x05, ///< entity garbage collected or purged from collection
	VU_MEM_DELETED        = 0xDD  ///< happens just before entity deletion by unreferencing
};

class VuEntity {

friend int VuReferenceEntity(VuEntity *ent);
friend int VuDeReferenceEntity(VuEntity *ent);	// note: may free ent
friend class VuDatabase;
friend class VuCollectionManager;
friend class VuCreateEvent;
//friend class VuReleaseEvent;

public:
	// constructors
	/** creates the entity of a given type with the given id */
	VuEntity(ushort typeindex, VU_ID_NUMBER eid);
	/** creates entity from stream */
	VuEntity(VU_BYTE **stream, long *rem);
	/** creates entity from file */
	VuEntity(FILE *file);

	// setters
	void SetPosition(BIG_SCALAR x, BIG_SCALAR y, BIG_SCALAR z);
	void SetDelta(SM_SCALAR dx, SM_SCALAR dy, SM_SCALAR dz);
	void SetYPR(SM_SCALAR yaw, SM_SCALAR pitch, SM_SCALAR roll){ 
		orient_.yaw_ = yaw; orient_.pitch_ = pitch; orient_.roll_ = roll; 
	}
	void SetYPRDelta(SM_SCALAR dyaw, SM_SCALAR dpitch,SM_SCALAR droll){ 
		orient_.dyaw_ = dyaw; orient_.dpitch_ = dpitch; orient_.droll_=droll; 
	}
	void SetUpdateTime(VU_TIME currentTime){ lastUpdateTime_ = currentTime; }
	void SetTransmissionTime(VU_TIME currentTime){ lastTransmissionTime_ = currentTime; }
	void SetCollisionCheckTime(VU_TIME currentTime){ lastCollisionCheckTime_ = currentTime; }
	void SetOwnerId(VU_ID ownerId);
	void SetEntityType(ushort entityType);
	void SetAssociation(VU_ID assoc) { share_.assoc_ = assoc; }

	//void AlignTimeAdd(VU_TIME timeDelta);
	//void AlignTimeSubtract(VU_TIME timeDelta);

	// getters
	// FRB - CTD's here
	VU_ID Id() const	{ return share_.id_; }
	VU_BYTE Domain() const { return domain_; }
	// check VuFlagBits for descriptions of each flag bit
	VU_BOOL IsPrivate() const       { return (VU_BOOL)share_.flags_.breakdown_.private_; }
	VU_BOOL IsTransferrable() const {return (VU_BOOL)share_.flags_.breakdown_.transfer_; }
	VU_BOOL IsTangible() const      { return (VU_BOOL)share_.flags_.breakdown_.tangible_; }
	VU_BOOL IsCollidable() const    { return (VU_BOOL)share_.flags_.breakdown_.collidable_; }
	VU_BOOL IsGlobal() const        { return (VU_BOOL)share_.flags_.breakdown_.global_; }
	VU_BOOL IsPersistent() const    { return (VU_BOOL)share_.flags_.breakdown_.persistent_; }
	/** action to take for a created local unit inserted into DB */
	typedef enum {
		VU_SC_DONT_SEND =0,  ///< dont send creation, since each session reads its own from files (non volatiles)
		VU_SC_SEND      =1,  ///< send creation, reliable but no hurry
		VU_SC_SEND_OOB  =2   ///< send creation, hurry (OOB)
	} VU_SEND_TYPE;
	/** if true, entity is sent immediately (OOB) to others upon creation */
	VU_SEND_TYPE SendCreate() const { return static_cast<VU_SEND_TYPE>(share_.flags_.breakdown_.sendCreate_); } 
	void SetSendCreate(VU_SEND_TYPE sc) { share_.flags_.breakdown_.sendCreate_ = sc; }

	VuFlagBits Flags() const  { return share_.flags_.breakdown_; }
	ushort FlagValue() const  { return share_.flags_.value_; }
	VU_ID OwnerId() const     { return share_.ownerId_; }
	VU_MEM VuState() const    { return vuState_; }
	ushort Type() const       { return share_.entityType_; }
	VU_BOOL IsLocal() const;
	VU_ID Association()	const { return share_.assoc_; }

	BIG_SCALAR XPos() const { return pos_.x_; }
	BIG_SCALAR YPos()	const { return pos_.y_; }
	BIG_SCALAR ZPos()	const { return pos_.z_; }
	SM_SCALAR XDelta() const { return pos_.dx_; }
	SM_SCALAR YDelta() const { return pos_.dy_; }
	SM_SCALAR ZDelta() const { return pos_.dz_; }
	SM_SCALAR Yaw() const { return orient_.yaw_; }
	SM_SCALAR Pitch() const { return orient_.pitch_; }
	SM_SCALAR Roll() const { return orient_.roll_; }
	SM_SCALAR YawDelta() const { return orient_.dyaw_; }
	SM_SCALAR PitchDelta() const { return orient_.dpitch_; }
	SM_SCALAR RollDelta() const { return orient_.droll_; }

	VU_TIME UpdateRate() const { return entityTypePtr_->updateRate_; }
	VU_TIME LastUpdateTime() const { return lastUpdateTime_; }
	VU_TIME LastTransmissionTime() const { return lastTransmissionTime_; }
	VU_TIME LastCollisionCheckTime() const { return lastCollisionCheckTime_; }

	VuEntityType *EntityType() const { return const_cast<VuEntityType*>(entityTypePtr_); }

	// entity driver
	VU_TIME DriveEntity(VU_TIME currentTime);	// returns TIME of next update
	VuDriver *EntityDriver()	{ return driver_; }
	VuDriver *SetDriver(VuDriver *newdriver);	// returns old driver (for del)
	// sfr: these 2 functions avoid position update duplication
	/** returns if unit is enquened for a positional update. */
	bool EnqueuedForPositionUpdate() const { return enqueuedForPositionUpdate; }
	/** sets if unit is enqueued for a positional update. */
	void SetEnqueuedForPositionUpdate(bool val){ enqueuedForPositionUpdate = val; }

	VU_BOOL CollisionCheck(VuEntity *other, SM_SCALAR deltatime);	// uses built-in
	virtual VU_BOOL CustomCollisionCheck(VuEntity *other, SM_SCALAR deltatime);
	virtual VU_BOOL TerrainCollisionCheck();		// default returns false
	VU_BOOL LineCollisionCheck(
		BIG_SCALAR x1, BIG_SCALAR y1, BIG_SCALAR z1,
		BIG_SCALAR x2, BIG_SCALAR y2, BIG_SCALAR z2,
		SM_SCALAR timeDelta, SM_SCALAR sizeFactor 
	);
	// virtual VU type getters
	virtual VU_BOOL IsTarget();	// returns FALSE
	virtual VU_BOOL IsSession();	// returns FALSE
	virtual VU_BOOL IsGroup();	// returns FALSE
	virtual VU_BOOL IsGame();	// returns FALSE
	// not really a type, but a utility nonetheless
	virtual VU_BOOL IsCamera() const;	// returns FALSE


	// virtual function interface
	// serialization functions
	virtual int SaveSize();
	virtual int Save(VU_BYTE **stream);	// returns bytes written
	virtual int Save(FILE *file);		// returns bytes written

	// event handlers
	virtual VU_ERRCODE Handle(VuErrorMessage *error);
	virtual VU_ERRCODE Handle(VuPushRequest *msg);
	virtual VU_ERRCODE Handle(VuPullRequest *msg);
	virtual VU_ERRCODE Handle(VuEvent *event);
	virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);
	virtual VU_ERRCODE Handle(VuPositionUpdateEvent *event);
	virtual VU_ERRCODE Handle(VuEntityCollisionEvent *event);
	virtual VU_ERRCODE Handle(VuTransferEvent *event);
	virtual VU_ERRCODE Handle(VuSessionEvent *event);

	int RefCount() { return refcount_; };
protected:
	// destructor
	virtual ~VuEntity();
	//void ChangeId(VuEntity *other);
	// making this private, so that only friends can access it. too dangerous to make public
	void SetId(VU_ID_NUMBER newId){ share_.id_.num_ = newId; }
	void SetVuState(VU_MEM newState){ vuState_ = newState; }
	virtual VU_ERRCODE InsertionCallback();
	virtual VU_ERRCODE RemovalCallback();

private:
	int LocalSize();                      // returns local bytes written

// DATA
protected:
	// shared data
	struct ShareData {
		ushort entityType_;	// id (table index)
		union {
			ushort value_;
			VuFlagBits breakdown_;
		} flags_;
		VU_ID id_;
		VU_ID ownerId_;	// owning session
		VU_ID assoc_;	// id of ent which must be local to this ent
	} share_;
	struct PositionData {
	BIG_SCALAR x_, y_, z_;
	SM_SCALAR dx_, dy_, dz_;
	} pos_;
	struct OrientationData {
		SM_SCALAR yaw_, pitch_, roll_;
		SM_SCALAR dyaw_, dpitch_, droll_;
	} orient_;

  // local data
private:
	/** reference count */
	ushort refcount_;
	/** entity mutex */
	VuMutex eMutex_;
	/** increases entity refcount. thread safe. Returns refcount. Can only be used by VuReferenceEntity. */
	int Ref();
	/** decreases entity reference count. thread safe. Returns refcount. Can only be used by VuDeReferenceEntity */
	int UnRef();

protected:
	VU_MEM vuState_;		
	VU_BYTE domain_;		
	VU_TIME lastUpdateTime_; 
	VU_TIME lastCollisionCheckTime_; 
	VU_TIME lastTransmissionTime_; 
	VuEntityType *entityTypePtr_;
	VuDriver *driver_;
	/** indicates entity is enqueued to be checked for position update. */
	bool enqueuedForPositionUpdate;
};
/** container for VU entities. */
typedef VuBin<VuEntity> VuEntityBin;


#endif // _VUENTITY_H_
