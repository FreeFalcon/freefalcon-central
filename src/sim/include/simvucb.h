#ifndef _SIMVUCB_H
#define _SIMVUCB_H

#include "f4vu.h"

//sfr: added rem
VuEntity* SimVUCreateVehicle (ushort type, ushort size, VU_BYTE* data);
VuEntity* SimVUCreateFeature (ushort type, ushort size, VU_BYTE* data);

#endif
