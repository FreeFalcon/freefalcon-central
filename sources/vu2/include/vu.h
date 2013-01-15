#ifndef _VU_H_
#define _VU_H_

//#pragma warning(disable : 4514)

#include "vu2.h"

//-------------------------------------------------------------------------
// The following three #defines are used to identify the Vu release number.
// VU_VERSION is defined as 2 for this architecture of Vu.
// VU_REVISION will be incremented each time a significant change is made
// 	(such as changing composition or organization of class data)
// VU_PATCH will be begin at 0 for each Revision, and be incremented by 1
// 	each time any change is made to Vu2
// VU_REVISION_DATE is a string which indicates the date of the latest revision
// VU_PATCH_DATE is a string which indicates the date of the latest patch
//-------------------------------------------------------------------------
#define VU_VERSION           3
#define VU_REVISION          1
#define VU_PATCH             0
#define VU_REVISION_DATE     "28/01/2007"
#define VU_PATCH_DATE        "28/01/2007"
//-------------------------------------------------------------------------

struct VuEntityType;

template <class E> class VuBin;

// globals
// VU provided
//extern VU_ID_NUMBER vuAssignmentId;
//extern VU_ID_NUMBER vuLowWrapNumber;
//extern VU_ID_NUMBER vuHighWrapNumber;
extern VuBin<VuSessionEntity> vuLocalSessionEntity;
extern VuGlobalGroup *vuGlobalGroup;
extern VuGameEntity *vuPlayerPoolGroup;
#if VU_ALL_FILTERED
extern VuLinkedList *vuGameList;
extern VuLinkedList *vuTargetList;
#else
extern VuFilteredList *vuGameList;
extern VuFilteredList *vuTargetList;
#endif

// sfr: now in vuMainThread
//extern VuPendingSendQueue *vuNormalSendQueue;
//extern VuPendingSendQueue *vuLowSendQueue;

extern VU_SESSION_ID vuKnownConnectionId;       // 0 --> not known (usual case)
extern VU_SESSION_ID vuNullSession;
extern VU_ID vuLocalSession;
//#define vuLocalSession (vuLocalSessionEntity.get() == NULL ? vuNullId : vuLocalSessionEntity->Id())
#define vuLocalGame (vuLocalSessionEntity == NULL ? NULL : vuLocalSessionEntity->Game())
extern VU_ID vuNullId;
extern VU_TIME vuTransmitTime;

// app provided globals
extern char *vuxWorldName;
extern ulong vuxLocalDomain;
extern VU_TIME vuxGameTime;
extern VU_TIME vuxRealTime;

// functions defined by application, used by VU
// these are like pure virtual functions which the app must define to use vu
extern VuEntityType *VuxType(ushort id);
extern VuMutex VuxCreateMutex(const char *name);
extern void VuxDestroyMutex(VuMutex);
extern void VuxLockMutex(VuMutex);
extern void VuxUnlockMutex(VuMutex);
extern bool VuxAddDanglingSession (VU_ID owner, VU_ADDRESS address);
extern bool VuxAddDanglingSession (VU_ID owner, VU_ADDRESS add);
extern int VuxSessionConnect(VuSessionEntity *session);
extern void VuxSessionDisconnect(VuSessionEntity *session);
extern int VuxGroupConnect(VuGroupEntity *group);
extern void VuxGroupDisconnect(VuGroupEntity *group);
extern int VuxGroupAddSession(VuGroupEntity *group, VuSessionEntity *session);
extern int VuxGroupRemoveSession(VuGroupEntity *group,VuSessionEntity *session);
extern void VuxAdjustLatency(VU_TIME, VU_TIME);
extern VU_ID_NUMBER VuxGetId();

// sfr: start removing locks
#define NO_VU_LOCK 1

extern void VuEnterCriticalSection();
extern void VuExitCriticalSection();
extern bool VuHasCriticalSection();

/** class used to lock a given scope with a mutex */
class VuScopeLock {
public:
	explicit VuScopeLock(VuMutex m) : m(m){ VuxLockMutex(m); }
	~VuScopeLock(){ VuxUnlockMutex(m); }
private:
	VuMutex m;
};

// virtual constructors (factories)
// these invoke appropriate NEW func with VU_BYTE stream, & return pointer
extern VuEntity *VuxCreateEntity(ushort type, ushort size, VU_BYTE *data);
extern VuMessage *VuxCreateMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID targetid);
extern void VuxRetireEntity(VuEntity *ent);
#ifdef VU_TRACK_LATENCY
extern void VuxAdjustLatency(VU_TIME newlatency, VU_TIME oldlatency);
#endif //VU_TRACK_LATENCY


#endif // _VU_H_
