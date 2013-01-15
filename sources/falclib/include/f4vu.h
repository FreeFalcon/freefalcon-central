#ifndef _VU_TEST_H_
#define _VU_TEST_H_

/** this file declares the vu application part. Parts of VU are left open, since its a generic library */

#include "../../vu2/include/VU2.h"
#include "vutypes.h"
#include "FalcEnt.h"

struct F4CSECTIONHANDLE;

// ==============================
// Vu Class heirarchy defines
// ==============================

#define	VU_DOMAIN		0
#define	VU_CLASS		1
#define	VU_TYPE			2
#define	VU_STYPE		3
#define	VU_SPTYPE		4
#define	VU_OWNER		5

// ========================
// These never change
// ========================

#define DOMAIN_ANY     255
#define CLASS_ANY      255
#define TYPE_ANY       255
#define STYPE_ANY      255
#define SPTYPE_ANY     255
#define RFU1_ANY       255
#define RFU2_ANY       255
#define RFU3_ANY       255
#define VU_ANY         255

// ========================
// Vis type defines
// ========================

#define VIS_NORMAL		0
#define VIS_REPAIRED	1
#define VIS_DAMAGED		2
#define VIS_DESTROYED	3
#define VIS_LEFT_DEST	4
#define VIS_RIGHT_DEST	5
#define VIS_BOTH_DEST	6

// ========================
// Filter Defines
// ========================

#define VU_FILTERANY	255
#define VU_FILTERSTOP	0

// ========================
// Default values
// ========================

#define F4_EVENT_QUEUE_SIZE				2000		// How many events we can have on the queue at one time

// ====================
// extern global defs
// ====================

extern VuMainThread *gMainThread;
extern int NumEntities;
extern VU_ID FalconNullId;
extern F4CSECTIONHANDLE* vuCritical;

// ==================================
// defines for backward compatibility
// ==================================

struct vector
{
	float		x;
	float		y;
	float		z;
};

struct euler
{
	float		yaw;
	float		pitch;
	float		roll;
};

// =========================
// Function Prototypes
// =========================

void InitVU (void);
void ExitVU (void);

#endif // _VU_TEST_H_
