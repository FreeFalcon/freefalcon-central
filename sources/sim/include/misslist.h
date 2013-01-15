#ifndef _MISSFUNC_H
#define _MISSFUNC_H

class SimBaseClass;
class SimWeaponClass;
class FalconEntity;

// Helper Functions
SimWeaponClass** InitMissile (FalconEntity* parent, ushort type, int num, int side);
SimWeaponClass* InitAMissile (FalconEntity* parent, ushort type, int slot);
void FreeFlyingMissile (SimBaseClass* flier);
void FreeRailMissile (SimBaseClass* railer);

#endif

