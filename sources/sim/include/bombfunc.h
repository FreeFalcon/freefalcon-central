#ifndef _BOMBLIST_H
#define _BOMBLIST_H

class SimBaseClass;

SimWeaponClass** InitBomb (FalconEntity* parent, ushort type, int num, int side);
SimWeaponClass* InitABomb (FalconEntity* parent, ushort type, int slot);
void FreeRackBomb (SimBaseClass *racker);

#endif
