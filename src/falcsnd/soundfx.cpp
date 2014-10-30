#include <cISO646>
#include "soundfx.h"
#include "soundgroups.h"

// JPO - added flags with names, and the VMS flag
// THESE MUST BE IN THE SAME ORDER AS THE ENUM IN SoundFX.h
SFX_DEF_ENTRY BuiltinSFX[] =
{
    {
        "none.wav",       0, 0, 0, 10.0F, -5000.0F, -10000.0F, 10.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, FX_SOUND_GROUP, 0,  0
    },
    {
        "biggun1.wav",    0, 0, 0, 10000000.0F, 0.0F, -10000.0F, 10000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "biggun2.wav",    0, 0, 0, 10000000.0F, 0.0F, -10000.0F, 10000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "bombdrop.wav",   0, 0, 0, 2000000.0F, 0.0F, -10000.0F, 2000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "engheli.wav",    0, 0, 0, 10000000.0F, 0.0F, -6000.0F, 10000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_POS_EXTERN bitor SFX_FLAGS_FREQ bitor SFX_FLAGS_3D bitor SFX_FLAGS_HIGH, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "eject.wav",      0, 0, 0, 100000.0F, 0.0F, -10000.0F, 100000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "growl.wav",      0, 0, 0, 100000.0F, 0.0F, -10000.0F, 1000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_FLAGS_FREQ bitor SFX_FLAGS_HIGH, 1.0F, 1.0F, SIDEWINDER_SOUND_GROUP, 0, 0
    },
    {
        "growlock.wav",   0, 0, 0, 4000000.0F, 0.0F, -10000.0F, 4000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_FLAGS_FREQ, 1.0F, 1.0F, SIDEWINDER_SOUND_GROUP, 0, 0
    },
    {
        "grndhit1.wav",   0, 0, 0, 4000000.0F, 0.0F, -10000.0F, 4000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "grndhit2.wav",   0, 0, 0, 4000000.0F, 0.0F, -10000.0F, 4000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "missile1.wav",   0, 0, 0, 4000000.0F, 0.0F, -6000.0F, 4000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "missile2.wav",   0, 0, 0, 4000000.0F, 0.0F, -6000.0F, 4000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "missile3.wav",   0, 0, 0, 4000000.0F, 0.0F, -6000.0F, 4000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "dirtdart.wav",   0, 0, 0, 40000000.0F, 0.0F, -10000.0F, 40000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "h2odart.wav",    0, 0, 0, 40000000.0F, 0.0F, -10000.0F, 40000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "lawndart.wav",   0, 0, 0, 40000000.0F, 0.0F, -10000.0F, 40000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "prop.wav",       0, 0, 0, 500000.0F, 0.0F, -10000.0F, 500000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_POS_EXTERN bitor SFX_FLAGS_FREQ bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "tank.wav",       0, 0, 0, 10000000.0F, 0.0F, -8000.0F, 10000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_POS_EXTERN bitor SFX_FLAGS_FREQ bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "truck1.wav",     0, 0, 0, 10000000.0F, 0.0F, -8000.0F, 10000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_POS_EXTERN bitor SFX_FLAGS_FREQ bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "truck2.wav",     0, 0, 0, 10000000.0F, 0.0F, -8000.0F, 10000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_POS_EXTERN bitor SFX_FLAGS_FREQ bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "ricco1.wav",     0, 0, 0, 4000000.0F, 0.0F, -3000.0F, 4000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "ricco2.wav",     0, 0, 0, 4000000.0F, 0.0F, -2000.0F, 4000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "ricco3.wav",     0, 0, 0, 4000000.0F, 0.0F, -3000.0F, 4000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "ricco4.wav",     0, 0, 0, 4000000.0F, 0.0F, -3000.0F, 4000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "ricco5.wav",     0, 0, 0, 4000000.0F, 0.0F, -3000.0F, 4000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "vulstart.wav",   0, 0, 0, 400000000.0F, 0.0F, -3500.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_FLAGS_3D, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "vulloop.wav",    0, 0, 0, 400000000.0F, 0.0F, -4000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_FLAGS_3D, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "vulend.wav",     0, 0, 0, 400000000.0F, 0.0F, -3500.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_FLAGS_3D, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "boomg1.wav",     0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "boomg2.wav",     0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "boomg3.wav",     0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "boomg4.wav",     0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "boomg5.wav",     0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "enginea.wav",    0, 0, 0, 2500000000.0F, 0.0F, -100.0F, 2500000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_POS_EXTERN bitor SFX_FLAGS_FREQ bitor SFX_FLAGS_3D bitor SFX_FLAGS_HIGH, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 500
    },
    {
        "engineb.wav",    0, 0, 0, 2500000000.0F, 0.0F, -100.0F, 2500000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_POS_EXTERN bitor SFX_FLAGS_FREQ bitor SFX_FLAGS_3D bitor SFX_FLAGS_HIGH, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 500
    },
    {
        "f16ext.wav",     0, 0, 0, 2500000000.0F, 0.0F, -100.0F, 2500000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_POS_EXTERN bitor SFX_FLAGS_FREQ bitor SFX_FLAGS_3D bitor SFX_FLAGS_HIGH, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 500
    },
    {
        "f16int.wav",     0, 0, 0, 2500000000.0F, 0.0F, -10000.0F, 2500000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_FLAGS_FREQ bitor SFX_FLAGS_3D bitor SFX_FLAGS_HIGH, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 500
    },
    {
        "twi\\mig21.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, RWR_SOUND_GROUP, 2, 3
    },
    {
        "geardn.wav",     0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "burnere.wav",    0, 0, 0, 2500000000.0F, 0.0F, -100.0F, 2500000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_POS_EXTERN bitor SFX_FLAGS_FREQ bitor SFX_FLAGS_3D bitor SFX_FLAGS_HIGH, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 500
    },
    {
        "burneri.wav",    0, 0, 0, 2500000000.0F, 0.0F, -10000.0F, 2500000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_FLAGS_FREQ bitor SFX_FLAGS_3D bitor SFX_FLAGS_HIGH, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 500
    },
    {
        "twi\\mig23.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, RWR_SOUND_GROUP, 2, 2
    },
    {
        "touchdn.wav",    0, 0, 0, 10000000.0F, 0.0F, -10000.0F, 10000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "booma1.wav",     0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "booma2.wav",     0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "booma3.wav",     0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "booma4.wav",     0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "booma5.wav",     0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "flaps.wav",      0, 0, 0, 500000.0F, 0.0F, -10000.0F, 500000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "mcgun.wav",      0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "jammer.wav",     0, 0, 0, 50000000.0F, 0.0F, -10000.0F, 50000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "vulstrte.wav",   0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "twi\\mig25.wav", 0, 0, 0, 4000000.0F, 0.0F, -10000.0F, 4000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, RWR_SOUND_GROUP, 2, 4
    },
    {
        "twi\\mig31.wav", 0, 0, 0, 4000000.0F, 0.0F, -10000.0F, 4000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, RWR_SOUND_GROUP, 2, 5
    },
    {
        "impacta1.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "impacta2.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "impacta3.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "impacta4.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "impacta5.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "twslock.wav",    0, 0, 0, 4000000.0F, 0.0F, -10000.0F, 4000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, RWR_SOUND_GROUP, 0, 0
    },
    {
        "twslnch.wav",    0, 0, 0, 4000000.0F, 0.0F, -10000.0F, 4000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, RWR_SOUND_GROUP, 0, 0
    },
    {
        "twslncir.wav",   0, 0, 0, 4000000.0F, 0.0F, -10000.0F, 4000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, RWR_SOUND_GROUP, 0, 0
    },
    {
        "twssrch.wav",    0, 0, 0, 4000000.0F, 0.0F, -10000.0F, 4000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, RWR_SOUND_GROUP, 0, 0
    },
    {
        "warning.wav",    0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_FLAGS_VMS, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "caution.wav",    0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_FLAGS_VMS, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "altitude.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_FLAGS_VMS, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "bingo.wav",      0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_FLAGS_VMS, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "lock.wav",       0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_FLAGS_VMS, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "pullup.wav",     0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_FLAGS_VMS, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "None.wav",       0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_FLAGS_VMS, 1.0F, 1.0F, RWR_SOUND_GROUP, 0, 0
    },
    {
        "iff.wav",        0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_FLAGS_VMS, 1.0F, 1.0F, RWR_SOUND_GROUP, 0, 0
    },
    {
        "vulloope.wav",   0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "vulende.wav",    0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "twi\\a50.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 2, 8
    },
    {
        "twi\\chaparal.wav",            0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 0
    },
    {
        "twi\\f5.wav",    0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 6, 2
    },
    {
        "twi\\f22.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 6, 6
    },
    {
        "twi\\2s6.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 1, 4
    },
    {
        "twi\\adats.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 6
    },
    {
        "twi\\ah66.wav",  0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 6, 7
    },
    {
        "twi\\av8b.wav",  0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 6, 8
    },
    {
        "twi\\e2c.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 1
    },
    {
        "twi\\e3.wav",    0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 2
    },
    {
        "twi\\f4.wav",    0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 6, 1
    },
    {
        "twi\\f14.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 6, 3
    },
    {
        "twi\\f15.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 6, 4
    },
    {
        "twi\\hawk.wav",  0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 0
    },
    {
        "twi\\hercules.wav",  0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 7
    },
    {
        "twi\\j5.wav",    0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 2, 7
    },
    {
        "twi\\j7.wav",    0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, RWR_SOUND_GROUP, 2, 6
    },
    {
        "twi\\launch.wav",              0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 0, 0
    },
    {
        "twi\\lav_adv.wav",             0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 9
    },
    {
        "twi\\patriot.wav",             0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 8
    },
    {
        "twi\\sa2.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 3, 2
    },
    {
        "twi\\sa3.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 3, 3
    },
    {
        "twi\\sa4.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 3, 4
    },
    {
        "twi\\sa5.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 3, 5
    },
    {
        "twi\\sa6.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 3, 6
    },
    {
        "twi\\sa8.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 3, 8
    },
    {
        "twi\\sa9.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 3, 9
    },
    {
        "twi\\sa10.wav",  0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 3, 1
    },
    {
        "twi\\sa13.wav",  0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 3, 7
    },
    {
        "twi\\slotback.wav",            0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 2, 1
    },
    {
        "twi\\su15.wav",  0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, RWR_SOUND_GROUP, 2, 9
    },
    {
        "diveloop.wav",   0, 0, 0, 50000000.0F, 0.0F, -10000.0F, 50000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "fireloop.wav",   0, 0, 0, 50000000.0F, -500.0F, -10000.0F, 50000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D bitor SFX_FLAGS_FREQ, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "chute.wav",      0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "splish.wav",     0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "rcktloop.wav",   0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "impactg1.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "impactg2.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "impactg3.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "impactg4.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "impactg5.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "impactg6.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "cockpit\\toggllil.wav",        0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "cockpit\\chngview.wav",        0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "cockpit\\ejectlvr.wav",        0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "cockpit\\geardwn.wav",         0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "cockpit\\gearup.wav",          0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "cockpit\\icp1.wav",            0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "cockpit\\ICP2.wav",            0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "cockpit\\icpmntry.wav",        0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "cockpit\\jettison.wav",        0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "cockpit\\knobbig.wav",         0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "cockpit\\knoblil.wav",         0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "cockpit\\mfd.wav",             0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "cockpit\\momntary.wav",        0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "cockpit\\navknob.wav",         0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "cockpit\\togglbig.wav",        0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "cockpit\\chaflare.wav",        0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "airbrake.wav",   0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "bind.wav",       0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "gearup.wav",     0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "jettison.wav",   0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "scream.wav",     0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "stall.wav",      0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "wind.wav",       0, 0, 0, 40000000.0F, 0.0F, -10000.0F, 40000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "flare.wav",      0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "brakend.wav",    0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "brakloop.wav",   0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "brakstrt.wav",   0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "brakwind.wav",   0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D bitor SFX_FLAGS_FREQ, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "flapend.wav",    0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "flaploop.wav",   0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "flapstrt.wav",   0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "gearcend.wav",   0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "gearcst.wav",    0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "gearloop.wav",   0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "gearost.wav",    0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "gearoend.wav",   0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "cockpit\\ugh.wav",             0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "twi\\Barlock.wav",             0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 1, 9
    },
    {
        "twi\\Firecan.wav",             0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 1, 3
    },
    {
        "twi\\flatface.wav",            0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 1, 1
    },
    {
        "twi\\longtrak.wav",            0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 1, 5
    },
    {
        "twi\\lowblow.wav",             0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 1, 6
    },
    {
        "twi\\MPQ54.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 0
    },
    {
        "twi\\MSQ48.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 3
    },
    {
        "twi\\MSQ50.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 4
    },
    {
        "twi\\spoonrst.wav",            0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 1, 7
    },
    {
        "twi\\Tps63.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 10
    },
    {
        "twi\\f16.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 6, 5
    },
    {
        "twi\\spy1a.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 5
    },
    {
        "twi\\gundish.wav",             0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 1, 8
    },
    {
        "twi\\amraam.wav",              0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 8, 1
    },
    {
        "twi\\phoenix.wav",             0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 8, 2
    },
    {
        "cockpit\\lspdtone.wav",        0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "tlscrape.wav",   0, 0, 0, 4000000.0F, 0.0F, -10000.0F, 4000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_FLAGS_3D bitor SFX_FLAGS_FREQ, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "rumble.wav",     0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_FLAGS_3D bitor SFX_FLAGS_FREQ, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "rumbshrt.wav",   0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_FLAGS_FREQ, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "thump2.wav",     0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_FLAGS_3D bitor SFX_FLAGS_FREQ, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "TireSqueal.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_FLAGS_3D bitor SFX_FLAGS_FREQ, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "hit1.wav",       0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "hit2.wav",       0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_FLAGS_3D bitor SFX_FLAGS_FREQ, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "hit3.wav",       0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_FLAGS_3D bitor SFX_FLAGS_FREQ, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "hit4.wav",       0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_FLAGS_3D bitor SFX_FLAGS_FREQ, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "hit5.wav",       0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_FLAGS_3D bitor SFX_FLAGS_FREQ, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "GroundCrunch.wav",             0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
    {
        "c130.wav",       0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_FREQ bitor SFX_FLAGS_3D bitor SFX_FLAGS_HIGH, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    //MI added for EWS stuff
    {
        "counter.wav",    0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_FLAGS_VMS, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    //JPO added for Ships
    {
        "ship.wav",        0, 0, 0, 10000000.0F, 0.0F, -8000.0F, 10000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    //MI uncaged AIM-9
    {
        "nocage.wav",      0, 0, 0, 1000000.0F, 0.0F, -10000.0F, 1000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_FLAGS_FREQ bitor SFX_FLAGS_3D bitor SFX_FLAGS_HIGH, 1.0F, 1.0F, SIDEWINDER_SOUND_GROUP, 0, 0
    },
    //MI EWS sounds
    {
        "Chaflare.wav",    0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_FLAGS_VMS, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "Chaflarelow.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_FLAGS_VMS, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "Chaflareout.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_FLAGS_VMS, 1.0F, 1.0F, COCKPIT_SOUND_GROUP, 0, 0
    },
    {
        "Aim9Envg.wav",      0, 0, 0, 1000000.0F, 0.0F, -10000.0F, 1000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_FLAGS_FREQ, 1.0F, 1.0F, SIDEWINDER_SOUND_GROUP, 0, 0
    },
    {
        "Aim9Envs.wav",      0, 0, 0, 1000000.0F, 0.0F, -10000.0F, 1000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_FLAGS_FREQ, 1.0F, 1.0F, SIDEWINDER_SOUND_GROUP, 0, 0
    },
    {
        "thunder.wav",    0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 10000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0, 10000
    },
    {
        "rainint.wav",    0, 0, 0, 40000.0F, 0.0F, -10000.0F, 10000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_FLAGS_FREQ bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0, 0
    },
    {
        "rainext.wav",    0, 0, 0, 40000.0F, 0.0F, -10000.0F, 10000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_FREQ bitor SFX_POS_LOOPED bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0, 0
    },
    {
        "groan1.wav",    0, 0, 0, 40000.0F, 0.0F, -10000.0F, 10000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_FLAGS_FREQ, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0, 0
    },
    {
        "groan2.wav",    0, 0, 0, 40000.0F, 0.0F, -10000.0F, 10000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_FLAGS_FREQ, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0, 0
    },
    // JB carrier
    {
        "hookend.wav",    0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "hookloop.wav",   0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    {
        "hookstrt.wav",   0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, ENGINE_SOUND_GROUP, 0, 0
    },
    //ME123 NEW RWR SOUNDS
    {
        "twi\\FLATLIB.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 4
    },
    {
        "twi\\TOMBSTONE.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 4
    },
    {
        "twi\\CLAMSHELL.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 4
    },
    {
        "twi\\BIGBIRD.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 4
    },
    {
        "twi\\SQUATEYE.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 4
    },
    {
        "twi\\TALLKING.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 4
    },
    {
        "twi\\SIDENET.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 4
    },
    {
        "twi\\BACKNET.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 4
    },
    {
        "twi\\KNIFEREST.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 4
    },
    {
        "twi\\GRILLPAN.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 4
    },
    {
        "twi\\BILLBOARD.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 4
    },
    {
        "twi\\HIGHSCREEN.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 4
    },
    {
        "twi\\ODDPAIR.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 4
    },
    {
        "twi\\PATHAND.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 4
    },
    {
        "twi\\THINSKIN.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 4
    },
    {
        "twi\\TINSHIELD.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 4
    },
    {
        "twi\\GINSLING.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 4
    },
    {
        "twi\\FLAPWHEEL.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 4
    },
    {
        "twi\\DOGEAR.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 4
    },
    {
        "twi\\BIGBACK.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 4
    },
    {
        "twi\\BACKTRAP.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_LOOPED, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 4
    },
    {
        "mikeclick.wav", 0, 0, 0, 400000000.0F, 0.0F, -10000.0F, 400000000.0F, 0, 0,
        SFX_POSITIONAL, 1.0F, 1.0F, RWR_SOUND_GROUP, 5, 4
    },
    {
        "dragchute.wav",   0, 0, 0, 5000000.0F, 0.0F, -10000.0F, 5000000.0F, 0, 0,
        SFX_POSITIONAL bitor SFX_POS_EXTERN bitor SFX_FLAGS_3D, 1.0F, 1.0F, FX_SOUND_GROUP, 0, 0
    },
};

const int BuiltinNSFX = sizeof(BuiltinSFX) / sizeof(SFX_DEF_ENTRY);
const char *FALCONSNDTABLE = "f4sndtbl.sfx";
const char *FALCONSNDTABLETXT = "f4sndtbl.txt";
SFX_DEF_ENTRY *SFX_DEF = 0;
int NumSFX = 0;
