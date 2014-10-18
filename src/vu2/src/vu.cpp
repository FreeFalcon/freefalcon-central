#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vu2.h"
#include "vu_priv.h"
#include "vutypes.h"

///////////////////////////////////////////////////////////////////////////////
// statics & globals
// static VuListIterator* vuTargetListIter     = 0;
#if VU_ALL_FILTERED
VuLinkedList*        vuGameList           = 0;
VuLinkedList*        vuTargetList         = 0;
#else
VuFilteredList*        vuGameList           = 0;
VuFilteredList*        vuTargetList         = 0;
#endif
VuGlobalGroup*         vuGlobalGroup        = 0;
VuGameEntity*          vuPlayerPoolGroup    = 0;
VuBin<VuSessionEntity> vuLocalSessionEntity;
VuMainThread*          vuMainThread         = 0;
// sfr: now in vuMainThread
//VuPendingSendQueue*    vuNormalSendQueue    = 0;
//VuPendingSendQueue*    vuLowSendQueue       = 0;
VU_TIME                vuTransmitTime       = 0;
VU_ID vuLocalSession(0, 0);
//VU_ID_NUMBER vuAssignmentId   = VU_FIRST_ENTITY_ID;
//VU_ID_NUMBER vuLowWrapNumber  = VU_FIRST_ENTITY_ID;
//VU_ID_NUMBER vuHighWrapNumber = ~((VU_ID_NUMBER)0);
//int VuMainThread::bytesSent_ = 0;
extern VU_TIME VuRandomTime(VU_TIME max);

