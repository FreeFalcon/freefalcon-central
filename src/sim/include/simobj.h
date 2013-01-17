#ifndef _SIM_OBJECT_H
#define _SIM_OBJECT_H

#include "f4vu.h"

class SimObjectType;
class SimMoverClass;
class FalconPrivateOrderedList;

SimObjectType* UpdateTargetList (SimObjectType* inUseList, SimMoverClass* self,
                       FalconPrivateOrderedList* thisObjectList);
void ReleaseTargetList (SimObjectType* InUseList);

#endif

