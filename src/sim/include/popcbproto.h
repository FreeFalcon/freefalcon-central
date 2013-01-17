#ifndef _POPCBPROTO_H
#define _POPCBPROTO_H

#include "falclib.h"

#define TOTAL_POPCALLBACK_SLOTS 10

typedef BOOL (*MenuCallback) (int, int, int, BOOL, VU_ID);
extern MenuCallback MenuCallbackArray[];

extern BOOL CBPopTestTrue(int, int, int, BOOL, VU_ID);
extern BOOL CBPopTestFalse(int, int, int, BOOL, VU_ID);

extern BOOL CBTestForTarget(int, int, int, BOOL, VU_ID);
extern BOOL CBCheckExtent(int, int, int, BOOL, VU_ID);
extern BOOL CBTestTwoShip(int, int, int, BOOL, VU_ID);
extern BOOL CBTestOnGround(int, int, int, BOOL, VU_ID);
extern BOOL CBTestAWACS(int, int, int, BOOL, VU_ID);
extern BOOL CBTestNotOnGround(int, int, int, BOOL, VU_ID);

#endif