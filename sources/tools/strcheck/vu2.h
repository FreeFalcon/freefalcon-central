// vu2.h - Copyright (c) Mon July 15 15:01:01 1996,  Spectrum HoloByte, Inc.  All Rights Reserved

#ifndef _VU_2_H_
#define _VU_2_H_

/* #define VU_USE_COMMS		0	*/
/* #define VU_AUTO_UPDATE		0	*/
/* #define VU_AUTO_COLLISION	1 */
/* #define VU_THREAD_SAFE	1 */

#define VU_DEFAULT_GROUP_SIZE	6

#ifdef VU_USE_COMMS
#include "comms\capi.h"
#endif // VU_USE_COMMS

#include "apitypes.h"
#include "vuentity.h"
#include "vucoll.h"
#include "vuevent.h"
#include "vudriver.h"
#include "vumath.h"
#include "vusessn.h"
#include "vu.h"

// Stubs
extern char * ComAPIRecvBufferGet(int a);
extern int	ComAPIGet(int a);
extern int ComAPIHostIDLen(int a);
extern int ComAPIHostIDGet(int a, char* b);
extern void ComAPIGroupSet(int a, VU_ID b);
extern VuMessage* VuxCreateMessage(unsigned short);

#endif // _VU_2_H_
