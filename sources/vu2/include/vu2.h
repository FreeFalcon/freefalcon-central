#ifndef _VU_2_H_
#define _VU_2_H_

//#pragma warning (disable : 4514)

#define VU_DEFAULT_GROUP_SIZE	6

// -----------------
//    VU options
// -----------------
#define VU_SIMPLE_LATENCY		1
//#define VU_TRACK_LATENCY  	1
#define VU_GRID_TREE_Y_MAJOR	1

// sfr: I think cameras can be viewed as areas of interest
#define VU_MAX_SESSION_CAMERAS	8 

// Set this if you don't want to auto-dispatch
#define REALLOC_QUEUES			1

// define this to send 8 byte class info with create msgs
// #define VU_SEND_CLASS_INFO 1

#include "../../mathlib/math.h"
#define FTOL FloatToInt32

#include "../../comms/capi.h"
#include "vutypes.h"
#include "vuentity.h"
#include "vucoll.h"
#include "vuevent.h"
#include "vudriver.h"
#include "vumath.h"
#include "vusessn.h"
#include "vu.h"
#include "vu_thread.h"
#include "vu_mq.h"
#include "vu_mf.h"
#include "vu_filter.h"
#include "vu_iterator.h"

// For Debug
#if 0
#include "debuggr.h"
#define VU_PRINT   MonoPrint
#else
#define VU_PRINT(a)
#endif

//#pragma warning (disable : 4514)

#endif // _VU_2_H_
