#ifndef _CAMP2SIM_H
#define _CAMP2SIM_H

class SimBaseClass;
class SimInitDataClass;

#define MOTION_OWNSHIP       (0x1 << 16)
#define MOTION_AIR_AI        (0x2 << 16)
#define MOTION_GND_AI        (0x4 << 16)
#define MOTION_MSL_AI        (0x8 << 16)
#define MOTION_BMB_AI       (0x10 << 16)
#define MOTION_HELO_AI      (0x20 << 16)
#define MOTION_EJECT_PILOT  (0x40 << 16)
#define MOTION_IN_COCKPIT  (0x100 << 16)
#define MOTION_THREAT      (0x200 << 16)
#define MOTION_MASK         (0x1f << 16)

#define WORLD_X_CENTER   (-65.0F * 5280.0F)
#define WORLD_Y_CENTER   ( 80.0F * 5280.0F)

SimBaseClass* AddObjectToSim (SimInitDataClass* init_data, int motion_type);

#endif

