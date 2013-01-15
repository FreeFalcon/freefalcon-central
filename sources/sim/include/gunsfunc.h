#ifndef _GUNSLIST_H
#define _GUNSLIST_H

class SimBaseClass;

SimWeaponClass** InitGun (SimBaseClass* parent, ushort type, int num, int side);
SimWeaponClass* InitAGun (SimBaseClass* parent, ushort type, int rounds);

#endif
