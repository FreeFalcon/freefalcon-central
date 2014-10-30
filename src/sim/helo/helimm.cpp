/*
** Name: HELIMM.CPP
** DEscription:
** Helicopter Math Model functions
**  Most of this stuff was taken originally from the NASA Technical
** Paper: "A Minimum-Complexity Helicopter Simulation Math Model"
** History:
** 20-jun-97 (edg)
** We go dancing in......
*/

#if _MSC_VER >= 1300
#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>
#else
#include <iostream.h>
#include <iomanip.h>
#include <string.h>
#include <fstream.h>
#endif //_MSC_VER

#include <math.h>
#include <stdlib.h>
#include "stdhdr.h"
#include "simbase.h"
#include "otwdrive.h"
#include "helimm.h"


float A1 = 1.5F; // integration constant
float A2 = 0.5F; // integration constant
float B1 = 1.0F - 1.5F; // integration constant
float B2 = 1.0F - 0.5F; // integration constant

// This cheat MM cheat is necessary to run the model in small
// enough time increments -- essentially dividing the frame time
// by a factor
#define TIME_CHEAT  (1.0F)

// constant which determines number of times to iterate on
// solution for thrust and induced velocity for both tail
// and main rotors.   5 is pretty accurate.  fewer, of course
// is faster.
// the STABLE model doesn't use this fro tail rotor, does for main rotor
const int numRotorIterations = 4;

// global data
/*
HELI_MODEL_DATA gModelData[NUM_MODELS] =
{
 // This model is for the Agusta A102
 {
 A109,
 0, // has_wing
 4.0, // roll damp
 // Fuselage Data
 {
 132.7, // fs cg
 38.5, // wl cg
 5401.0, // weight
 {
   1590, // IX
   6760, // IY
   6407, // IZ
 },
 800.0, // IXZ
 132.4, // fs cp
 38.2, // wl cp
 {
   -10.8, // fe x
   -167, // fe y
   -85, // fe z
 },
 },
 // Main rotor Data
 {
 132.4, // fs
 98.2, // wl
 0.11, // is
 0.5, // hinge offset
 212.0, // moment of flap inertia
 18.0, // radius
 5.7, // lift slope
 385, // rpm
 0.010, // cd0
 4, // n blades
 1.10, // chord
 -.105, // twist
 0.0, // k1

 },
 // Tail rotor Data
 {
 391.0, // fs
 70.0, // wl
 3.1, // radius
 5.0, // lift slope
 2080, // rpm
 -.137, // twist
 0.134, // solidity
 2, // # blades
 0.6 // chord
 },
 // Horiz Wing Data
 {
 0, // fs
 0, // wl
 0, // zuu
 0, // zuw
 0, // zmax
 1, // span
 },
 // Horiz Tail Data
 {
 330, // fs
 54, // wl
 0.0, // zuu
 -34.0, // zuw
 -22.0, // zmax
 1, // span
 },
 // Vert Tail Data
 {
 380, // fs
 80, // wl
 0.0, // zuu
 -47.0, // zuw
 -17.0, // zmax
 },
 // trim data
 {
 -1.9, // cyc_roll center
 12.0, // cyc_roll max
  -0.2, // cyc_pitch center
  6.5, // cyc_pitch max
  0.0, // coll_pitch center
 10.0, // coll_pitch max
 8.0, // tr_pitch center
 18.0, // tr_pitch max
 },
 },


 // This model is for the Cobra
 {
 COBRA,
 0, // has_wing
 4.0, // roll damp
 // Fuselage Data
 {
 196.0, // fs cg
 75.0, // wl cg
 9000.0, // weight
 {
   2593, // IX
   14320, // IY
   12330, // IZ
 },
 0, // IXZ
 200.0, // fs cp
 65.0, // wl cp
 {
   -30.0, // fe x
   -275, // fe y
   -41, // fe z
 },
 },
 // Main rotor Data
 {
 200.0, // fs
 153.0, // wl
 0.0, // is
 0.00, // hinge offset
 1382.0, // moment of flap inertia
 22.0, // radius
 6.0, // lift slope
 324, // rpm
 0.010, // cd0
 2, // n blades
 2.25, // chord
 -.175, // twist
 0.0, // k1

 },
 // Tail rotor Data
 {
 521.5, // fs
 119.0, // wl
 // 75.0, // wl
 4.25, // radius
 6.0, // lift slope
 1660, // rpm
 0.0, // twist
 0.105, // solidity
 2, // # blades
 0.5 // chord
 },
 // Horiz Wing Data
 {
 200, // fs
 65, // wl
 -39, // zuu
 -161, // zuw
 -65, // zmax
 10.75, // span
 },
 // Horiz Tail Data
 {
 400, // fs
 65, // wl
 0.0, // zuu
 -80.0, // zuw
 -32.0, // zmax
 1, // span
 },
 // Vert Tail Data
 {
 490, // fs
 80, // wl
 // 75, // wl
 0.0, // zuu
 -62.0, // zuw
 -50.0, // zmax
 },
 // trim data
 {
 -0.3, // cyc_roll center
 10.0, // cyc_roll max
 -0.3, // cyc_pitch center
 10.0, // cyc_pitch max
  0.0, // coll_pitch center
 14.0, // coll_pitch max
  5.9, // tr_pitch center
 15.0, // tr_pitch max
 },
 },

 // This model is for the MD500
 {
 MD500,
 0, // has_wing
 4.0, // roll damp
 // Fuselage Data
 {
 87.6, // fs cg
 66.0, // wl cg
 2200.0, // weight
 {
   263, // IX
   1101, // IY
   1000, // IZ
 },
 200.0, // IXZ
 87.8, // fs cp
 66.2, // wl cp
 {
   -5.8, // fe x
   -100, // fe y
   -45, // fe z
 },
 },
 // Main rotor Data
 {
 87.6, // fs
 90.0, // wl
 0.0, // is
 0.42, // hinge offset
 150.0, // moment of flap inertia
 13.5, // radius
 6.0, // lift slope
 481, // rpm
 0.010, // cd0
 5, // n blades
 0.56, // chord
 -.157, // twist
 0.0, // k1

 },
 // Tail rotor Data
 {
 271.2, // fs
 66.0, // wl
 2.3, // radius
 3.0, // lift slope
 2924, // rpm
 -.150, // twist
 0.119, // solidity
 2, // # blades
 0.44 // chord
 },
 // Horiz Wing Data
 {
 0, // fs
 0, // wl
 0, // zuu
 0, // zuw
 0, // zmax
 1, // span
 },
 // Horiz Tail Data
 {
 278.0, // fs
 116, // wl
 0.0, // zuu
 -20.0, // zuw
 -12.0, // zmax
 1, // span
 },
 // Vert Tail Data
 {
 265.0, // fs
 70, // wl
 0.0, // zuu
 -33.0, // zuw
 -10.0, // zmax
 },
 // trim data
 {
  0.0, // cyc_roll center
 10.0, // cyc_roll max
 -0.5, // cyc_pitch center
  7.5, // cyc_pitch max
  0.0, // coll_pitch center
 11.3, // coll_pitch max
 9.0, // tr_pitch center
 15.0, // tr_pitch max
 },
 },

 // This model is for the STABLE
 {
 STABLE,
 0, // has_wing
 8.0, // roll damp
 // Fuselage Data
 {
 87.6, // fs cg
 66.0, // wl cg
 2200.0, // weight
 {
   263, // IX
   1101, // IY
   1000, // IZ
 },
 200.0, // IXZ
 87.6, // fs cp
 66.0, // wl cp
 {
   -5.8, // fe x
   -100, // fe y
   -45, // fe z
 },
 },
 // Main rotor Data
 {
 87.6, // fs
 90.0, // wl
 0.0, // is
 0.42, // hinge offset
 150.0, // moment of flap inertia
 13.5, // radius
 6.0, // lift slope
 481, // rpm
 0.010, // cd0
 5, // n blades
 0.56, // chord
 -.157, // twist
 0.0, // k1

 },
 // Tail rotor Data
 {
 271.2, // fs
 66.0, // wl
 2.3, // radius
 3.0, // lift slope
 2924, // rpm
 0.0, // twist
 0.119, // solidity
 2, // # blades
 0.44 // chord
 },
 // Horiz Wing Data
 {
 0, // fs
 0, // wl
 0, // zuu
 0, // zuw
 0, // zmax
 1, // span
 },
 // Horiz Tail Data
 {
 278.0, // fs
 116, // wl
 0.0, // zuu
 -20.0, // zuw
 -12.0, // zmax
 1, // span
 },
 // Vert Tail Data
 {
 265.0, // fs
 66.0, // wl
 0.0, // zuu
 -33.0, // zuw
 -50.0, // zmax
 },
 // trim data
 {
  0.0, // cyc_roll center
  8.0, // cyc_roll max
  0.0, // cyc_pitch center
  16.5, // cyc_pitch max
  0.0, // coll_pitch center
 17.3, // coll_pitch max
 0.0, // tr_pitch center
 10.0, // tr_pitch max
 },
 },
 // This model is for the SIMPLE
 {
 SIMPLE,
 0, // has_wing
 8.0, // roll damp
 // Fuselage Data
 {
 87.6, // fs cg
 66.0, // wl cg
 2200.0, // weight
 {
   263, // IX
   1101, // IY
   1000, // IZ
 },
 200.0, // IXZ
 87.6, // fs cp
 66.0, // wl cp
 {
   -5.8, // fe x
   -100, // fe y
   -45, // fe z
 },
 },
 // Main rotor Data
 {
 87.6, // fs
 90.0, // wl
 0.0, // is
 0.42, // hinge offset
 150.0, // moment of flap inertia
 13.5, // radius
 6.0, // lift slope
 481, // rpm
 0.010, // cd0
 5, // n blades
 0.56, // chord
 -.157, // twist
 0.0, // k1

 },
 // Tail rotor Data
 {
 271.2, // fs
 66.0, // wl
 2.3, // radius
 3.0, // lift slope
 2924, // rpm
 0.0, // twist
 0.119, // solidity
 2, // # blades
 0.44 // chord
 },
 // Horiz Wing Data
 {
 0, // fs
 0, // wl
 0, // zuu
 0, // zuw
 0, // zmax
 1, // span
 },
 // Horiz Tail Data
 {
 278.0, // fs
 116, // wl
 0.0, // zuu
 -20.0, // zuw
 -12.0, // zmax
 1, // span
 },
 // Vert Tail Data
 {
 265.0, // fs
 66.0, // wl
 0.0, // zuu
 -33.0, // zuw
 -50.0, // zmax
 },
 // trim data
 {
  0.0, // cyc_roll center
  8.0, // cyc_roll max
  0.0, // cyc_pitch center
  16.5, // cyc_pitch max
  0.0, // coll_pitch center
 17.3, // coll_pitch max
 0.0, // tr_pitch center
 10.0, // tr_pitch max
 },
 },
};
*/
HELI_MODEL_DATA gModelData[NUM_MODELS] =
{
    // This model is for the Agusta A102
    {
        A109,
        0, // has_wing
        4.0F, // roll damp
        // Fuselage Data
        {
            132.7F, // fs cg
            38.5F, // wl cg
            5401.0F, // weight
            {
                1590.0F, // IX
                6760.0F, // IY
                6407.0F, // IZ
            },
            800.0F, // IXZ
            132.4F, // fs cp
            38.2F, // wl cp
            {
                -10.8F, // fe x
                -167.0F, // fe y
                -85.0F, // fe z
            },
        },
        // Main rotor Data
        {
            132.4F, // fs
            98.2F, // wl
            0.11F, // is
            0.5F, // hinge offset
            212.0F, // moment of flap inertia
            18.0F, // radius
            5.7F, // lift slope
            385.0F, // rpm
            0.010F, // cd0
            4.0F, // n blades
            1.10F, // chord
            -.105F, // twist
            0.0F, // k1

        },
        // Tail rotor Data
        {
            391.0F, // fs
            70.0F, // wl
            3.1F, // radius
            5.0F, // lift slope
            2080.0F, // rpm
            0.0f, // twist
            0.134F, // solidity
            2.0F, // # blades
            0.6F // chord
        },
        // Horiz Wing Data
        {
            0.0F, // fs
            0.0F, // wl
            0.0F, // zuu
            0.0F, // zuw
            0.0F, // zmax
            1.0F, // span
        },
        // Horiz Tail Data
        {
            330.0F, // fs
            54.0F, // wl
            0.0F, // zuu
            -34.0F, // zuw
            -22.0F, // zmax
            1.0F, // span
        },
        // Vert Tail Data
        {
            380.0F, // fs
            80.0F, // wl
            0.0F, // zuu
            -47.0F, // zuw
            -17.0F, // zmax
        },
        // trim data
        {
            -1.9F, // cyc_roll center
            12.0F, // cyc_roll max
            -0.2F, // cyc_pitch center
            6.5F, // cyc_pitch max
            0.0F, // coll_pitch center
            10.0F, // coll_pitch max
            0.0F, // tr_pitch center
            18.0F, // tr_pitch max
        },
    },


    // This model is for the Cobra
    {
        COBRA,
        0, // has_wing
        4.0F, // roll damp
        // Fuselage Data
        {
            196.0F, // fs cg
            75.0F, // wl cg
            9000.0F, // weight
            {
                2593.0F, // IX
                14320.0F, // IY
                12330.0F, // IZ
            },
            0.0F, // IXZ
            200.0F, // fs cp
            65.0F, // wl cp
            {
                -30.0F, // fe x
                -275.0F, // fe y
                -41.0F, // fe z
            },
        },
        // Main rotor Data
        {
            200.0F, // fs
            153.0F, // wl
            0.0F, // is
            0.00F, // hinge offset
            1382.0F, // moment of flap inertia
            22.0F, // radius
            6.0F, // lift slope
            324.0F, // rpm
            0.010F, // cd0
            2.0F, // n blades
            2.25F, // chord
            -.175F, // twist
            0.0F, // k1

        },
        // Tail rotor Data
        {
            521.5F, // fs
            119.0F, // wl
            // 75.0, // wl
            4.25F, // radius
            6.0F, // lift slope
            1660.0F, // rpm
            0.0F, // twist
            0.105F, // solidity
            2.0F, // # blades
            0.5F // chord
        },
        // Horiz Wing Data
        {
            200.0F, // fs
            65.0F, // wl
            -39.0F, // zuu
            -161.0F, // zuw
            -65.0F, // zmax
            10.75F, // span
        },
        // Horiz Tail Data
        {
            400.0F, // fs
            65.0F, // wl
            0.0F, // zuu
            -80.0F, // zuw
            -32.0F, // zmax
            1.0F, // span
        },
        // Vert Tail Data
        {
            490.0F, // fs
            80.0F, // wl
            // 75, // wl
            0.0F, // zuu
            -62.0F, // zuw
            -50.0F, // zmax
        },
        // trim data
        {
            -0.3F, // cyc_roll center
            10.0F, // cyc_roll max
            -0.3F, // cyc_pitch center
            10.0F, // cyc_pitch max
            0.0F, // coll_pitch center
            14.0F, // coll_pitch max
            0.0F, // tr_pitch center
            15.0F, // tr_pitch max
        },
    },

    // This model is for the MD500
    {
        MD500,
        0, // has_wing
        4.0F, // roll damp
        // Fuselage Data
        {
            87.6F, // fs cg
            66.0F, // wl cg
            2200.0F, // weight
            {
                263.0F, // IX
                1101.0F, // IY
                1000.0F, // IZ
            },
            200.0F, // IXZ
            87.8F, // fs cp
            66.2F, // wl cp
            {
                -5.8F, // fe x
                -100.0F, // fe y
                -45.0F, // fe z
            },
        },
        // Main rotor Data
        {
            87.6F, // fs
            90.0F, // wl
            0.0F, // is
            0.42F, // hinge offset
            150.0F, // moment of flap inertia
            13.5F, // radius
            6.0F, // lift slope
            481.0F, // rpm
            0.010F, // cd0
            5.0F, // n blades
            0.56F, // chord
            -.157F, // twist
            0.0F, // k1

        },
        // Tail rotor Data
        {
            271.2F, // fs
            66.0F, // wl
            2.3F, // radius
            4.0F, // lift slope
            2924.0F, // rpm
            0.0F, // twist
            0.119F, // solidity
            2.0F, // # blades
            0.44F // chord
        },
        // Horiz Wing Data
        {
            0.0F, // fs
            0.0F, // wl
            0.0F, // zuu
            0.0F, // zuw
            0.0F, // zmax
            1.0F, // span
        },
        // Horiz Tail Data
        {
            278.0F, // fs
            116.0F, // wl
            0.0F, // zuu
            -20.0F, // zuw
            -12.0F, // zmax
            1.0F, // span
        },
        // Vert Tail Data
        {
            265.0F, // fs
            70.0F, // wl
            0.0F, // zuu
            -33.0F, // zuw
            -10.0F, // zmax
        },
        // trim data
        {
            0.0F, // cyc_roll center
            10.0F, // cyc_roll max
            -0.5F, // cyc_pitch center
            7.5F, // cyc_pitch max
            0.0F, // coll_pitch center
            11.3F, // coll_pitch max
            0.0F, // tr_pitch center
            15.0F, // tr_pitch max
        },
    },

    // This model is for the STABLE
    {
        STABLE,
        0, // has_wing
        8.0F, // roll damp
        // Fuselage Data
        {
            87.6F, // fs cg
            66.0F, // wl cg
            2200.0F, // weight
            {
                263.0F, // IX
                1101.0F, // IY
                1000.0F, // IZ
            },
            200.0F, // IXZ
            87.6F, // fs cp
            66.0F, // wl cp
            {
                -5.8F, // fe x
                -100.0F, // fe y
                -45.0F, // fe z
            },
        },
        // Main rotor Data
        {
            87.6F, // fs
            90.0F, // wl
            0.0F, // is
            0.42F, // hinge offset
            150.0F, // moment of flap inertia
            13.5F, // radius
            6.0F, // lift slope
            481.0F, // rpm
            0.010F, // cd0
            5.0F, // n blades
            0.56F, // chord
            -.157F, // twist
            0.0F, // k1

        },
        // Tail rotor Data
        {
            271.2F, // fs
            66.0F, // wl
            2.3F, // radius
            3.0F, // lift slope
            2924.0F, // rpm
            0.0F, // twist
            0.119F, // solidity
            2.0F, // # blades
            0.44F // chord
        },
        // Horiz Wing Data
        {
            0.0F, // fs
            0.0F, // wl
            0.0F, // zuu
            0.0F, // zuw
            0.0F, // zmax
            1.0F, // span
        },
        // Horiz Tail Data
        {
            278.0F, // fs
            116.0F, // wl
            0.0F, // zuu
            -20.0F, // zuw
            -12.0F, // zmax
            1.0F, // span
        },
        // Vert Tail Data
        {
            265.0F, // fs
            66.0F, // wl
            0.0F, // zuu
            -33.0F, // zuw
            -50.0F, // zmax
        },
        // trim data
        {
            0.0F, // cyc_roll center
            8.0F, // cyc_roll max
            0.0F, // cyc_pitch center
            16.5F, // cyc_pitch max
            0.0F, // coll_pitch center
            17.3F, // coll_pitch max
            0.0F, // tr_pitch center
            10.0F, // tr_pitch max
        },
    },
    // This model is for the SIMPLE
    {
        SIMPLE,
        0, // has_wing
        8.0F, // roll damp
        // Fuselage Data
        {
            87.6F, // fs cg
            66.0F, // wl cg
            2200.0F, // weight
            {
                263.0F, // IX
                1101.0F, // IY
                1000.0F, // IZ
            },
            200.0F, // IXZ
            87.6F, // fs cp
            66.0F, // wl cp
            {
                -5.8F, // fe x
                -100.0F, // fe y
                -45.0F, // fe z
            },
        },
        // Main rotor Data
        {
            87.6F, // fs
            90.0F, // wl
            0.0F, // is
            0.42F, // hinge offset
            150.0F, // moment of flap inertia
            13.5F, // radius
            6.0F, // lift slope
            481.0F, // rpm
            0.010F, // cd0
            5.0F, // n blades
            0.56F, // chord
            -.157F, // twist
            0.0F, // k1

        },
        // Tail rotor Data
        {
            271.2F, // fs
            66.0F, // wl
            2.3F, // radius
            3.0F, // lift slope
            2924.0F, // rpm
            0.0F, // twist
            0.119F, // solidity
            2.0F, // # blades
            0.44F // chord
        },
        // Horiz Wing Data
        {
            0.0F, // fs
            0.0F, // wl
            0.0F, // zuu
            0.0F, // zuw
            0.0F, // zmax
            1.0F, // span
        },
        // Horiz Tail Data
        {
            278.0F, // fs
            116.0F, // wl
            0.0F, // zuu
            -20.0F, // zuw
            -12.0F, // zmax
            1.0F, // span
        },
        // Vert Tail Data
        {
            265.0F, // fs
            66.0F, // wl
            0.0F, // zuu
            -33.0F, // zuw
            -50.0F, // zmax
        },
        // trim data
        {
            0.0F, // cyc_roll center
            8.0F, // cyc_roll max
            0.0F, // cyc_pitch center
            16.5F, // cyc_pitch max
            0.0F, // coll_pitch center
            17.3F, // coll_pitch max
            0.0F, // tr_pitch center
            10.0F, // tr_pitch max
        },
    },
};



inline float
FABS(float a)
{
    return ((a >= 0.0) ? a : -a);
}


/*
** Name: HeliMMClass
** Description:
** Constructor for helicopter math model.
** Helicopter type must be passed in in order to set the model's
** basis data.
*/
HeliMMClass::HeliMMClass(SimBaseClass *self, int helitype)
{
    mlTrig trig;

    // sanity check type, use default if invlaid
    if (helitype >= NUM_MODELS)
        helitype = MD500;

    platform = self;

    // set pointer to model data
    md = &gModelData[ helitype ];

    // reset variables
    ResetForceVars();

    p_ind = 0; // induced power
    p_climb = 0; // climb power
    p_par = 0; // parasite power
    p_prof = 0; // profile power
    p_tot = 0; // total power
    p_mr = 0; // main rotor power
    p_tr = 0; // tail rotor power
    torque_mr = 0; // main rotor torque
    vr_tr  = 0; // air vel relative to tail rotor disk
    vb_tr  = 0; // air vel relative to tail rotor blade
    thrust_tr = 0; // thrust tail rotor
    vi_tr = 0; // induced air tail rotor
    dw_ht_pos = 0; // downwash on horizontal tail pos
    wa_ht = 0; // airflow on ht
    eps_ht = 0; // downwash factor on ht
    vta_ht = 0; // total airspeed at ht
    va_vt = 0; // airflow on vt
    vta_vt = 0; // total airspeed at vt
    coll_pitch = 0; // collective setting in rads (at root)
    cyc_pitch = 0; // cyclic pitch in rads
    cyc_roll = 0; // cyclic roll in rads
    tr_pitch = 0; // tail rotor pitch
    a_sum = 0; // tpp temp?
    b_sum = 0; // tpp temp?
    wr = 0; // Z axis air velocity relative to rotor plane
    wb = 0; // Z axis air velocity relative to rotor blade
    thrust_mr = 0; // main rotor thrust
    vi_mr = 0; // main rotor induced velocity = 0;
    wa_fus = 0; // downwash on fuselage
    wa_fus_pos = 0; // position of downwash on fuselage
    va_x_sq = 0; // square of VA.x
    va_y_sq = 0; // square of VA.y
    isDigital = FALSE;


    // get sines and cosines
    mlSinCos(&trig, XE.ax);
    eucos.x = trig.cos;
    eusin.x = trig.sin;
    mlSinCos(&trig, XE.ay);
    eucos.y = trig.cos;
    eusin.y = trig.sin;
    mlSinCos(&trig, XE.az);
    eucos.z = trig.cos;
    eusin.z = trig.sin;

    // set delta time resolution for model



    // run some precalcs to set variables based on the model data
    PreCalc();
}

/*
** Name: ~HeliMMClass
** Description:
** DEstructor for helicopter math model.
*/

HeliMMClass::~HeliMMClass(void)
{
}

/*
** Name: SetControls
** Description:
** Sets up cyclic, collective and tail rotor pitches based on
** control inputs
*/
void
HeliMMClass::SetControls(float pstick, float rstick, float throttle, float pedals)
{

    ctlcpitch = throttle;

    // for pedals bitand stick, put in a centered dead zone
    // when human controlled
    if ( not isDigital)
    {
        if (pedals > 0.30F)
            ctltpitch = (pedals - 0.30F) / 0.70F  ;
        else if (pedals < -0.30F)
            ctltpitch = (pedals + 0.30F) / 0.70F;
        else
            ctltpitch = 0.0F;

        if (pstick > 0.05F)
            ctlroll = -(pstick - 0.05F) / 0.95F;
        else if (pstick < -0.05F)
            ctlroll = -(pstick + 0.05F) / 0.95F;
        else
            ctlroll = 0.0F;

        if (rstick > 0.15F)
            ctlpitch = (rstick - 0.15F) / 0.85F;
        else if (rstick < -0.15F)
            ctlpitch = (rstick + 0.15F) / 0.85F;
        else
            ctlpitch = 0.0;
    }
    else
    {
        ctltpitch = pedals;
        ctlroll = -pstick;
        ctlpitch = rstick;
    }

    // get math model control inputs and convert to radians
    cyc_roll = md->td.cyc_roll_center *
               (PI / 180.0F) + ctlroll *
               md->td.cyc_roll_max * (PI / 180.0F);
    cyc_pitch = md->td.cyc_pitch_center *
                (PI / 180.0F) + ctlpitch *
                md->td.cyc_pitch_max * (PI / 180.0F);
    coll_pitch = md->td.coll_pitch_center *
                 (PI / 180.0F) + ctlcpitch *
                 md->td.coll_pitch_max * (PI / 180.0F);
    tr_pitch = md->td.tr_pitch_center *
               (PI / 180.0F) + ctltpitch *
               md->td.tr_pitch_max * (PI / 180.0F);
}


/*
** Name: HeliMMPreCalc
** Description:
** Precalculates some variables to be used by the flight dynamics
** functions.
*/
void
HeliMMClass::PreCalc(void)
{
    // get mass based on weight and gravity
    mass = md->fus.weight / GRAVITY;

    // tail and main rotor omega's (angular speed)
    omega_mr = md->mr.rpm * 2.0F * PI / 60.0F;
    omega_tr = md->tr.rpm * 2.0F * PI / 60.0F;

    // tip speed
    vtip = omega_mr * md->mr.radius;

    // effective frontal areas of rotors
    fr_mr = md->mr.cD * md->mr.radius * md->mr.nb * md->mr.chord;
    fr_tr = md->mr.cD * md->tr.radius * md->tr.nb * md->tr.chord;

    hp_loss = 90.0F;

    // here we could calc based on an altitude, for now just use std
    // sea level density
    rh0 = (float)STD_DENSITY_0;
    rh02 = rh0 / 2.0F;

    // I've got no idea on this one yet -- used in other calcs
    gam_om_16 =
        rh0 * md->mr.lslope * md->mr.chord * (float)pow(md->mr.radius, 4) /
        md->mr.b_mi * omega_mr /
        16.0F * (1.0F + 8.0F / 3.0F * md->mr.h_offset / md->mr.radius);

    // kC is flapping aerodynamic couple
    kC = (0.75F * omega_mr * md->mr.h_offset / md->mr.radius /
          gam_om_16) + md->mr.k1;

    // flapping x-couple coeff
    itb2_om = omega_mr / (1.0F + (float)pow(omega_mr / gam_om_16, 2));

    // flapping primary response
    itb = itb2_om * omega_mr / gam_om_16;



    // primary flapping stiffness
    dl_db1 = md->mr.nb / 2.0F *
             (1.5F * md->mr.b_mi * md->mr.h_offset / md->mr.radius *
              omega_mr * omega_mr);

    // cross flapping stiffness
    dl_da1 = (rh02) * md->mr.lslope * md->mr.nb * md->mr.chord *
             md->mr.radius * vtip * vtip * md->mr.h_offset / 6.0F;

    // empirical hack for the Agusta A102
    if (md->type == A109)
    {
        itb2_om = 0;
        itb = gam_om_16;
        // zeroing this out (or lowering) effects roll-oscillations -- it makes
        // them smaller to nonexistent for much smoother feel
        dl_da1 /= 8.0;
    }


    // thrust coefficient
    cT = md->fus.weight /
         (rh0 * PI * md->mr.radius * md->mr.radius *
          vtip * vtip);

    // a * sigma
    a_sigma = md->mr.lslope * md->mr.nb * md->mr.chord / md->mr.radius / PI;

    // tip plane path dihedral effect
    db1dv = 2.0F / omega_mr / md->mr.radius *
            (8.0F * cT / a_sigma + (float)sqrt(cT / 2.0F));

    // tip plane path pitchup with speed
    da1du = -db1dv;

    // empirical hack for the STABLE model
    if (md->type == STABLE)
    {
        // zeroing this out (or lowering) effects roll-oscillations -- it makes
        // them smaller to nonexistent for much smoother feel
        dl_da1 *= 0.8F;
    }

    // pre-calc some values for main rotor
    mr_tmp1 = omega_mr * md->mr.radius * rh0 * md->mr.lslope *
              md->mr.nb * md->mr.chord * md->mr.radius / 4.0F;
    mr_tmp2 = md->mr.radius * rh0 * md->mr.radius * PI;

    // calculate the moment arms
    // these are basically the relation of components to center of grav
    // also conerts inches to ft
    ma_hub.y = (md->mr.wl_hub - md->fus.wl_cg) / 12.0F;
    ma_hub.x = (md->mr.fs_hub - md->fus.fs_cg) / 12.0F;

    ma_fus.y = (md->fus.wl_cp - md->fus.wl_cg) / 12.0F;
    ma_fus.x = (md->fus.fs_cp - md->fus.fs_cg) / 12.0F;

    ma_wn.y = (md->wn.wl - md->fus.wl_cg) / 12.0F;
    ma_wn.x = (md->wn.fs - md->fus.fs_cg) / 12.0F;

    ma_ht.y = (md->ht.wl - md->fus.wl_cg) / 12.0F;
    ma_ht.x = (md->ht.fs - md->fus.fs_cg) / 12.0F;

    ma_vt.y = (md->vt.wl - md->fus.wl_cg) / 12.0F;
    ma_vt.x = (md->vt.fs - md->fus.fs_cg) / 12.0F;

    ma_tr.y = (md->tr.wl_hub - md->fus.wl_cg) / 12.0F;
    ma_tr.x = (md->tr.fs_hub - md->fus.fs_cg) / 12.0F;
}

/*
** Name: HeliMMDynamics
** Description:
** Runs the functions calculating the dynamic math model functions
** for helicopter flight.
*/
void
HeliMMClass::Exec(void)
{
    int i;


    // NOTE: slower time for frame time. the current val is too
    // slow for the math.  Theoretically we should also 2 iterations
    // of the math model for every sim loop

    if (md->type == SIMPLE)
    {
        dT = SimLibMajorFrameTime;
        SimpleModel();
    }
    else
    {
        dT = SimLibMajorFrameTime / 5.0f;

        for (i = 0; i < 5; i++)
        {
            Setup();
            TipPlanePath();
            MainRotor();
            Fuselage();
            TailRotor();
            HorizTail();
            Wing();
            VertTail();
            ForceCalc();
        }

        SetPlatformData();
    }
}

/*
** Name: HeliMMDynamicsSetup
** Description:
** Caluclate setup variables for the frame
*/
void
HeliMMClass::Setup(void)
{

    // set rel airmass velocity
    // just assign VB to VA
    // however, we could also do the VG (gust) assignment here too
    VA = VB;

    // total relative airspeed
    vta = (float)sqrt(
              VA.x * VA.x +
              VA.y * VA.y +
              VA.z * VA.z);
}


/*
** Name: HeliMMDynamicsTipPlanePath
** Description:
** Caluclate setup variables for the frame
*/
void
HeliMMClass::TipPlanePath(void)
{
    float wake_effect;

    // if velocity in X is within a range, use a wake effect
    if (VA.x < vtrans)
        wake_effect = 1;
    else
        wake_effect = 0;

    // empirical hack not in cobra
    // don't use this for either now -- it's fucked up
    wake_effect = 0;


    // these 2 statements read the cyclic control and
    // dihedral effect based slip and aoa
    a_sum = GV.y - cyc_pitch +
            kC * GV.x +
            db1dv * VA.y * (1.0F + wake_effect);

    b_sum = GV.x + cyc_roll -
            kC * GV.y +
            da1du * VA.x * (1.0F + 2.0F * wake_effect);

    GR.x = -itb * b_sum -
           itb2_om * a_sum -
           VA.ay;
    GR.y = -itb * a_sum +
           itb2_om * b_sum -
           VA.ax;

    GV.x = GV.x +
           dT * (A2 * GR.x +
                 B2 * ABprev.a1);
    GV.y = GV.y +
           dT * (A2 * GR.y +
                 B2 * ABprev.b1);

    // save past values
    ABprev.a1 = GR.x;
    ABprev.b1 = GR.y;
}


/*
** Name: HeliMMDynamicsMainRotor
** Description:
** Calculates main rotor thrust and induced velocity
*/
void
HeliMMClass::MainRotor(void)
{
    int i;
    float tmp3;
    float vhat_sq;
    float vi_sq;

    // STABLE model: no induced flow effects.  Just calculate thrust
    // as a percent of throttle setting based on weight of fuselage
    if (md->type == SIMPLE)
    {
        // no induced flow
        vi_mr = 0.0F;
        thrust_mr = (md->fus.weight * 1.5F) * ctlcpitch;
    }
    else
    {
        // get velocities relative to rotor plane and blade
        // collective setting gets factored in here
        wr = VA.z +
             (GV.x - md->mr.hub_is) * VA.x -
             GV.y * VA.y;
        wb = wr +
             (2.0F / 3.0F) * omega_mr * md->mr.radius *
             (coll_pitch + 0.75F * md->mr.twist);

        va_x_sq = VA.x * VA.x;
        va_y_sq = VA.y * VA.y;

        // iterative solution for thrust and induced velocity
        for (i = 0; i < numRotorIterations; i++)
        {
            thrust_mr = (wb - vi_mr) * mr_tmp1;
            vhat_sq = va_x_sq + va_y_sq + wr * (wr - 2 * vi_mr);
            tmp3 = thrust_mr / 2.0F / mr_tmp2;
            tmp3 *= tmp3;
            tmp3 += (vhat_sq / 2.0F) * (vhat_sq / 2.0F);
            vi_sq = (float)sqrt(tmp3) - vhat_sq / 2.0F;
            vi_mr = (float)sqrt(FABS(vi_sq));
        }
    }

    // hack for upsidedown flight
    // for some reason the main rotor thrust isn't working right when
    // inverted.  Check when we are inverted and reduce thrust proprtional
    // to our inverted amount
    /*
    if ( eucos.x < 0.0f )
     thrust_mr *= (1.0f + eucos.x) * 0.50f;
    */



    // if ( thrust_mr < 0.0f )
    // thrust_mr = -thrust_mr;
}

/*
** Name: HeliMMDynamicsFuselage
** Description:
** Calculates forces acting on fuselage
*/
void
HeliMMClass::Fuselage(void)
{
    float om_radius;

    if (md->type == STABLE)
    {
        // STABLE: no download effect on fuselage
        wa_fus = 0.0F;
        wa_fus_pos = 0.0F;
    }
    else
    {
        // calc the downwash on the fuselage
        wa_fus = VA.z - vi_mr;

        // calc position on fuselage
        if (wa_fus not_eq 0)
            wa_fus_pos =
                (VA.x / (-wa_fus) * (ma_hub.y - ma_fus.y)) -
                (ma_fus.x - ma_hub.x);
        else
            wa_fus_pos = 0.0F;
    }

    // empirical hack
    // don't use for now
    // wa_fus_pos *= 3.0;

    // compute the forces and moments on the fuselage
    fus6d.x = (rh02) * md->fus.fe.x * va_x_sq;
    fus6d.y = (rh02) * md->fus.fe.y * va_y_sq;
    fus6d.z = (rh02) * md->fus.fe.z * wa_fus * wa_fus;
    fus6d.ax = fus6d.y * ma_fus.y;
    fus6d.ay = fus6d.z * wa_fus_pos -
               fus6d.x * ma_fus.y;

    // drag of fuselage based on velocity^2 of rotation and mass
    fus6d.az = -(float)fabs(VB.az) * VB.az * 0.004f * mass;


    // compute forces and moments of the main rotor
    mr6d.x = -thrust_mr * (GV.x - md->mr.hub_is);
    mr6d.y = thrust_mr * (GV.y);
    mr6d.z = -thrust_mr;
    mr6d.ax = mr6d.y * ma_hub.y +
              dl_db1 * GV.y +
              dl_da1 * (GV.x + cyc_roll - md->mr.k1 * GV.y);
    mr6d.ay = mr6d.z * ma_hub.x -
              mr6d.x * ma_hub.y +
              dl_db1 * GV.x +
              dl_da1 * (-GV.y + cyc_pitch - md->mr.k1 * GV.x);

    // power calcs
    p_ind = thrust_mr * vi_mr;

    // not sure about this one.  Paper calcs as WT*HDOT but HDOT is
    // never defined anywhere.  I assume it's the rate of climb which
    // should be VE.z
    p_climb = md->fus.weight * VE.z;

    p_par = - fus6d.x * VA.x -
            fus6d.y * VA.y -
            fus6d.z * wa_fus;

    om_radius = omega_mr * md->mr.radius;

    p_prof = (rh02) *
             (fr_mr / 4.0F) *
             om_radius *
             (om_radius * om_radius + 4.6F *
              (va_x_sq + va_y_sq));

    p_tot = p_ind + p_climb + p_par + p_prof;

    p_mr = p_ind + p_prof;

    // main rotor torque
    torque_mr = p_tot / omega_mr;

    // hack -- not sure what's coming out, but torque should always
    // have same sign
    // if ( torque_mr > 0.0f )
    //  torque_mr = -torque_mr;


    mr6d.az = torque_mr;

    // all models reduce torque -- it's just too much
    // for STABLE type, no/lessen rotor torque
    if (md->type == STABLE)
    {
        mr6d.az *= 0.20f;
    }
    else if (md->type == COBRA)
    {
        mr6d.az *= 0.53f;
    }
    else
    {
        mr6d.az *= 0.53f;
    }

}


/*
** Name: HeliMMDynamicsTailRotor
** Description:
** Calculates Tail rotor thrust and induced velocity
*/
void
HeliMMClass::TailRotor(void)
{
    int i;
    float tmp1;
    float tmp2;
    float tmp3;
    float tmp4;
    float vhat_sq;
    float vi_sq;

    // get velocities relative to rotor plane and blade
    // collective setting gets factored in here
    if (md->type == SIMPLE)
    {
        // STABLE model just calculates thrust based on some constant
        // force times the +/- % of pedal settings

        // no tail induced flow
        vi_tr = 0.0f;

        // thrust
        thrust_tr = 130.0F * ctltpitch;

        // no sideslip effect of tail
        tr6d.y = 0.0F;

        // no roll effect of tail
        tr6d.ax = 0.0F;

        // yaw
        tr6d.az = -thrust_tr * ma_tr.x;
    }
    else
    {
        // relative wind on tail rotor
        vr_tr = -(VA.y -
                  VA.az * ma_tr.x +
                  VA.ax * ma_tr.y);
        vb_tr = vr_tr +
                2.0F / 3.0F * omega_tr * md->tr.radius *
                (tr_pitch + 0.75F * md->tr.twist);


        // pre-calc some values
        tmp1 = omega_tr * md->tr.radius * rh0 * md->tr.lslope *
               md->tr.solidity * PI * md->tr.radius * md->tr.radius / 4;
        tmp2 = md->tr.radius * rh0 * md->tr.radius * PI;
        tmp4 = VA.z + VA.ay * ma_tr.x;
        tmp4 *= tmp4;

        // iterative solution for thrust and induced velocity
        for (i = 0; i < numRotorIterations; i++)
        {
            thrust_tr = (vb_tr - vi_tr) * tmp1;
            vhat_sq = tmp4 + va_x_sq + vr_tr * (vr_tr - 2 * vi_tr);
            tmp3 = thrust_tr / 2.0F / tmp2;
            tmp3 *= tmp3;
            tmp3 += (vhat_sq / 2.0F) * (vhat_sq / 2.0F);
            vi_sq = (float)sqrt(tmp3) - vhat_sq / 2.0F;
            vi_tr = (float)sqrt(FABS(vi_sq));
        }

        // power calc
        p_tr = thrust_tr * vi_tr;

        // force and moment calc
        tr6d.y = thrust_tr;
        tr6d.ax = tr6d.y * ma_tr.y;
        tr6d.az = -tr6d.y * ma_tr.x;
    }

}

/*
** Name: HeliMMDynamicsWing
** Description:
** Calculates Tail rotor thrust and induced velocity
*/
void
HeliMMClass::Wing(void)
{
    float wa_wn;
    float vta_wn;

    if (md->has_wing == 0)
    {
        wn6d.z = 0;
        wn6d.x = 0;
        return;
    }

    // airflow (Z) velocity on ht
    if (md->type == STABLE)
    {
        wa_wn = VA.z;
        vta_wn = (float)sqrt(va_x_sq + wa_wn * wa_wn);
        wn6d.x = 0;
    }
    else
    {
        wa_wn = VA.z - vi_mr;
        vta_wn = (float)sqrt(va_x_sq + wa_wn * wa_wn);

        // induced drag
        if (vta_wn not_eq 0)
            wn6d.x =
                -(rh02) / PI / vta_wn / vta_wn *
                (md->wn.zuu * va_x_sq +
                 md->wn.zuw * (VA.x) * wa_wn) *
                (md->wn.zuu * va_x_sq +
                 md->wn.zuw * (VA.x) * wa_wn) ;
        else
            wn6d.x = 0;
    }


    // surface stalled?
    if (FABS(wa_wn) > 0.3F * FABS(VA.x))
        wn6d.z =
            (rh02) *
            md->wn.zmax * FABS(vta_wn) * wa_wn;
    else
        // circulation lift on HT
        wn6d.z =
            (rh02) *
            (md->wn.zuu * va_x_sq +
             md->wn.zuw * (VA.x) * wa_wn);


}

/*
** Name: HeliMMDynamicsHorizTail
** Description:
** Calculates Tail rotor thrust and induced velocity
*/
void
HeliMMClass::HorizTail(void)
{
    float tmp1;


    // calc position of downwash on tail
    if (md->type == STABLE)
    {
        // airflow (Z) velocity on ht
        // STABLE -- consider only rotational velocity
        wa_ht = ma_ht.x * VA.ay;
    }
    else
    {
        tmp1 = vi_mr - VA.z;

        if (tmp1 == 0)
        {
            dw_ht_pos = 0;
        }
        else
        {
            dw_ht_pos =
                (VA.x / (tmp1) * (ma_hub.y - ma_ht.y)) -
                (ma_ht.x - ma_hub.x - md->mr.radius);

            // empirical hack
            if (md->type == A109)
                dw_ht_pos += 1;
        }

        // trianglur downwash field
        if (dw_ht_pos > 0 and dw_ht_pos < md->mr.radius)
            eps_ht = 2.0F * (1 - dw_ht_pos / md->mr.radius);
        else
            eps_ht = 0.0F;

        // airflow (Z) velocity on ht
        wa_ht = VA.z -
                eps_ht * vi_mr +
                ma_ht.x * VA.ay;

    }

    // total tail rel air velocity
    vta_ht = (float)sqrt(va_x_sq + va_y_sq + wa_ht * wa_ht);


    // surface stalled?
    if (FABS(wa_ht) > 0.3F * FABS(VA.x))
        ht6d.z = (rh02) * md->ht.zmax * FABS(vta_ht) * wa_ht;
    else
        // circulation lift on HT
        ht6d.z =
            (rh02) *
            (md->ht.zuu * va_x_sq +
             md->ht.zuw * FABS(VA.x) * wa_ht);

    // pitching moment
    ht6d.ay = ht6d.z * ma_ht.x;

}

/*
** Name: HeliMMDynamicsVertTail
** Description:
** Calculates Tail rotor thrust and induced velocity
*/
void
HeliMMClass::VertTail(void)
{
    // airflow (Z) velocity on vt
    if (md->type == STABLE)
    {
        // no tail rotor induced velocity effects on tail
        va_vt = VA.y - ma_vt.x * VA.az;
    }
    else
    {
        va_vt = VA.y + vi_tr - ma_vt.x * VA.az;
    }

    // vta_vt = sqrt( va_x_sq + va_y_sq + va_vt * va_vt );
    vta_vt = (float)sqrt(va_x_sq + va_vt * va_vt);


    // surface stalled?
    if (FABS(va_vt) > 0.3F * FABS(VA.x))
        vt6d.y =
            (rh02) *
            md->vt.ymax * FABS(vta_vt) * va_vt;
    else
        // circulation lift on VT
        vt6d.y =
            (rh02) *
            (md->vt.yuu * va_x_sq +
             md->vt.yuv * FABS(VA.x) * va_vt);

    // rolling and yawing moment
    vt6d.ax = vt6d.y * ma_vt.y;
    vt6d.az = -vt6d.y * ma_vt.x;

}


/*
** Name: HeliMMDynamicsForceCalc
** Description:
** Calculates final forces, acclerations and velocities
*/
void
HeliMMClass::ForceCalc(void)
{
    mlTrig trig;

    // gravity force
    grav.x = -mass * GRAVITY * eusin.y;
    grav.y = mass * GRAVITY * eusin.x * eucos.y;
    grav.z = mass * GRAVITY * eucos.x * eucos.y;


    // Calculate Translational Forces
    F.x =
        grav.x +
        mr6d.x +
        wn6d.x +
        fus6d.x;
    F.y =
        grav.y +
        mr6d.y +
        fus6d.y +
        tr6d.y +
        vt6d.y;
    F.z =
        grav.z +
        mr6d.z +
        wn6d.z +
        fus6d.z +
        ht6d.z;

    // calculate torques
    F.ax =
        mr6d.ax +
        fus6d.ax +
        tr6d.ax / md->roll_damp +
        vt6d.ax / md->roll_damp;
    F.ay =
        mr6d.ay +
        fus6d.ay +
        ht6d.ay;
    F.az =
        mr6d.az +
        tr6d.az +
        vt6d.az +
        fus6d.az;

    // pitch and roll flap
    F.a1 = GR.x / itb;
    F.b1 = GR.y / itb;

    // body accelerations
    AB.x = -(VB.ay * VB.z - VB.az * VB.y) + F.x / mass;
    AB.y = (VB.ax * VB.z - VB.az * VB.x) + F.y / mass;
    AB.z = (VB.ay * VB.x - VB.ax * VB.y) + F.z / mass;
    AB.ax = F.ax / md->fus.mi.x;
    AB.ay = F.ay / md->fus.mi.y -
            VB.ax * VB.az * (md->fus.mi.x - md->fus.mi.z) / md->fus.mi.y +
            (VB.az * VB.az - VB.ax * VB.ax) * md->fus.mi_xz / md->fus.mi.y;
    AB.az = F.az / md->fus.mi.z +
            md->fus.mi_xz * AB.ax / md->fus.mi.z;


    // integrate body accelerations
    VB.x = VB.x + dT * (A1 * AB.x + B1 * ABprev.x);
    VB.y = VB.y + dT * (A1 * AB.y + B1 * ABprev.y);
    VB.z = VB.z + dT * (A1 * AB.z + B1 * ABprev.z);

    VB.ax = VB.ax + dT * (A1 * AB.ax + B1 * ABprev.ax);
    VB.ay = VB.ay + dT * (A1 * AB.ay + B1 * ABprev.ay);
    VB.az = VB.az + dT * (A1 * AB.az + B1 * ABprev.az);

    // save previous
    ABprev = AB;
    ABprev.a1 = GR.x;
    ABprev.b1 = GR.y;


    VE.x = (VB.x * eucos.y + VB.z * eusin.y) *
           eucos.x * (float)cos(XE.az);
    mlSinCos(&trig, XE.az);
    VE.y = VB.y * trig.cos + VB.x * trig.sin;
    VE.z = -(VB.x * eusin.y - VB.z * eucos.y) * eucos.x;

    CalcBodyOrientation();

    XE.x = XE.x + dT * TIME_CHEAT * (A2 * VE.x + B2 * VEprev.x);
    XE.y = XE.y + dT * TIME_CHEAT * (A2 * VE.y + B2 * VEprev.y);
    XE.z = XE.z + dT * TIME_CHEAT * (A2 * VE.z + B2 * VEprev.z);


    // save previous
    VEprev = VE;

    // get sines and cosines
    mlSinCos(&trig, XE.ax);  // Roll
    eucos.x = trig.cos;
    eusin.x = trig.sin;
    mlSinCos(&trig, XE.ay);  // Pitch
    eucos.y = trig.cos;
    eusin.y = trig.sin;
    mlSinCos(&trig, XE.az);  // Yaw
    eucos.z = trig.cos;
    eusin.z = trig.sin;

    // get alpha and beta
    alpha = (float)atan2(VB.z, VB.x) * RTD;
    beta = (float)atan2(VB.y, VB.x) * RTD;

}

/*
** Name: HeliMMDynamicsResetForceVars
** Description:
** Resets the dynamic force variables
*/
void
HeliMMClass::ResetForceVars(void)
{
    memset(&AB, 0, sizeof(D6DOF));
    memset(&ABprev, 0, sizeof(D6DOF));
    memset(&F, 0, sizeof(D6DOF));
    memset(&VA, 0, sizeof(D6DOF));
    memset(&VB, 0, sizeof(D6DOF));
    memset(&VE, 0, sizeof(D6DOF));
    memset(&VEprev, 0, sizeof(D6DOF));
    memset(&VG, 0, sizeof(D6DOF));
    memset(&GR, 0, sizeof(D6DOF));
    memset(&GV, 0, sizeof(D6DOF));
    memset(&XE, 0, sizeof(D6DOF));
}

/*
** Name: SetPlatformData
** Description:
** Sets values for the platform
*/
void
HeliMMClass::SetPlatformData(void)
{
    float t1, t2;
    float alpharad, betarad;
    mlTrig trig;

    alpharad = alpha * DTR;
    betarad  = beta  * DTR;

    platform->platformAngles.cospsi = (float)eucos.z;
    platform->platformAngles.costhe = (float)eucos.y;
    platform->platformAngles.cosphi = (float)eucos.x;

    platform->platformAngles.sinpsi = (float)eusin.z;
    platform->platformAngles.sinthe = (float)eusin.y;
    platform->platformAngles.sinphi = (float)eusin.x;
    mlSinCos(&trig, alpharad);
    platform->platformAngles.sinalp = trig.sin;
    platform->platformAngles.cosalp = trig.cos;
    mlSinCos(&trig, betarad);
    platform->platformAngles.sinbet = trig.sin;
    platform->platformAngles.cosbet = trig.cos;

    platform->platformAngles.tanbet = (float)tan(betarad);

    /*-----------------------------*/
    /* velocity vector orientation */
    /*-----------------------------*/

    /*-------*/
    /* gamma */
    /*-------*/
    platform->platformAngles.singam = (platform->platformAngles.sinthe *
                                       platform->platformAngles.cosalp - platform->platformAngles.costhe *
                                       platform->platformAngles.cosphi * platform->platformAngles.sinalp) *
                                      platform->platformAngles.cosbet - platform->platformAngles.costhe *
                                      platform->platformAngles.sinphi * platform->platformAngles.sinbet;

    platform->platformAngles.cosgam = (float)sqrt(1.0f -
                                      platform->platformAngles.singam * platform->platformAngles.singam);

    gmma = (float)atan2(platform->platformAngles.singam, platform->platformAngles.cosgam);

    /*----*/
    /* mu */
    /*----*/
    t1 = platform->platformAngles.costhe * platform->platformAngles.sinphi *
         platform->platformAngles.cosbet + (platform->platformAngles.sinthe *
                                            platform->platformAngles.cosalp - platform->platformAngles.costhe *
                                            platform->platformAngles.cosphi * platform->platformAngles.sinalp) *
         platform->platformAngles.sinbet;
    t2 = platform->platformAngles.costhe * platform->platformAngles.cosphi *
         platform->platformAngles.cosalp + platform->platformAngles.sinthe *
         platform->platformAngles.sinalp;

    mu     = (float)atan2(t1, t2);
    platform->platformAngles.sinmu  = t1 * mu;
    platform->platformAngles.cosmu  = t2 * mu;


    /*-------*/
    /* sigma */
    /*-------*/
    t1 = (-platform->platformAngles.sinphi *
          platform->platformAngles.sinalp * platform->platformAngles.cosbet +
          platform->platformAngles.cosphi * platform->platformAngles.sinbet) *
         platform->platformAngles.cospsi + ((platform->platformAngles.costhe *
                                            platform->platformAngles.cosalp + platform->platformAngles.sinthe *
                                            platform->platformAngles.cosphi * platform->platformAngles.sinalp) *
                                            platform->platformAngles.cosbet + platform->platformAngles.sinthe *
                                            platform->platformAngles.sinphi * platform->platformAngles.sinbet) *
         platform->platformAngles.sinpsi;
    t2 = ((platform->platformAngles.costhe *
           platform->platformAngles.cosalp + platform->platformAngles.sinthe *
           platform->platformAngles.cosphi * platform->platformAngles.sinalp) *
          platform->platformAngles.cosbet + platform->platformAngles.sinthe *
          platform->platformAngles.sinphi * platform->platformAngles.sinbet) *
         platform->platformAngles.cospsi + (platform->platformAngles.sinphi *
                                            platform->platformAngles.sinalp * platform->platformAngles.cosbet -
                                            platform->platformAngles.cosphi * platform->platformAngles.sinbet) *
         platform->platformAngles.sinpsi;

    sigma  = (float)atan2(t1, t2);
    platform->platformAngles.sinsig = sigma * t1;
    platform->platformAngles.cossig = sigma * t2;

    /*
    platform->dmx[0][0] = platform->platformAngles.cospsi*platform->platformAngles.costhe;
    platform->dmx[0][1] = platform->platformAngles.sinpsi*platform->platformAngles.costhe;
    platform->dmx[0][2] = -platform->platformAngles.sinthe;

    platform->dmx[1][0] = -platform->platformAngles.sinpsi*platform->platformAngles.cosphi + platform->platformAngles.cospsi*platform->platformAngles.sinthe*platform->platformAngles.sinphi;
    platform->dmx[1][1] = platform->platformAngles.cospsi*platform->platformAngles.cosphi + platform->platformAngles.sinpsi*platform->platformAngles.sinthe*platform->platformAngles.sinphi;
    platform->dmx[1][2] = platform->platformAngles.costhe*platform->platformAngles.sinphi;

    platform->dmx[2][0] = platform->platformAngles.sinpsi*platform->platformAngles.sinphi + platform->platformAngles.cospsi*platform->platformAngles.sinthe*platform->platformAngles.cosphi;
    platform->dmx[2][1] = -platform->platformAngles.cospsi*platform->platformAngles.sinphi + platform->platformAngles.sinpsi*platform->platformAngles.sinthe*platform->platformAngles.cosphi;
    platform->dmx[2][2] = platform->platformAngles.costhe*platform->platformAngles.cosphi;
    */

    // indicated air speed in knots -- don't include rate of climb/descent
    GetKias = (float)sqrt(VB.x * VB.x + VB.y * VB.y) * FTPSEC_TO_KNOTS;

}

/*
** Name: Init
** Description:
** Set initial position and velocity
*/
void
HeliMMClass::Init(float x, float y, float z)
{
    XE.x = x;
    XE.y = y;
    XE.z = z;
    VB.z = -10.0F;

    // init quaternion
    InitQuat();
}

/*
** Name: InitQuat
** Description:
** Set initial quaternion vals and init integration arrays
*/
void
HeliMMClass::InitQuat(void)
{
    float e10, e20, e30, e40;
    mlTrig trigAZ, trigAY, trigAX;

    mlSinCos(&trigAX, XE.ax * 0.5F);
    mlSinCos(&trigAY, XE.ay * 0.5F);
    mlSinCos(&trigAZ, XE.az * 0.5F);

    /*------------------------*/
    /* initialize quaternions */
    /*------------------------*/
    e10 = trigAZ.cos * trigAY.cos * trigAX.cos +
          trigAZ.sin * trigAY.sin * trigAX.sin;

    e20 = trigAZ.sin * trigAY.cos * trigAX.cos -
          trigAZ.cos * trigAY.sin * trigAX.sin;

    e30 = trigAZ.cos * trigAY.sin * trigAX.cos +
          trigAZ.sin * trigAY.cos * trigAX.sin;

    e40 = trigAZ.cos * trigAY.cos * trigAX.sin -
          trigAZ.sin * trigAY.sin * trigAX.cos;

    e1 = e10;
    e2 = e20;
    e3 = e30;
    e4 = e40;

    olde1[0] = e10;
    olde1[1] = e10;
    olde1[2] = 0.0;
    olde1[3] = 0.0;

    olde2[0] = e20;
    olde2[1] = e20;
    olde2[2] = 0.0;
    olde2[3] = 0.0;

    olde3[0] = e30;
    olde3[1] = e30;
    olde3[2] = 0.0;
    olde3[3] = 0.0;

    olde4[0] = e40;
    olde4[1] = e40;
    olde4[2] = 0.0;
    olde4[3] = 0.0;

    /*---------------------------*/
    /* initial direction cosines */
    /*---------------------------*/
    platform->dmx[0][0] = e10 * e10 - e20 * e20 - e30 * e30 + e40 * e40;
    platform->dmx[0][1] = 2 * (e30 * e40 + e10 * e20);
    platform->dmx[0][2] = 2 * (e20 * e40 - e10 * e30);

    platform->dmx[1][0] = 2 * (e30 * e40 - e10 * e20);
    platform->dmx[1][1] = e10 * e10 - e20 * e20 + e30 * e30 - e40 * e40;
    platform->dmx[1][2] = 2 * (e20 * e30 + e40 * e10);

    platform->dmx[2][0] = 2 * (e10 * e30 + e20 * e40);
    platform->dmx[2][1] = 2 * (e20 * e30 - e10 * e40);
    platform->dmx[2][2] = e10 * e10 + e20 * e20 - e30 * e30 - e40 * e40;

    oldGRx[0] = 0.0F;
    oldGRx[1] = 0.0F;
    oldGRx[2] = 0.0F;
    oldGRx[3] = 0.0F;

    oldGRy[0] = 0.0F;
    oldGRy[1] = 0.0F;
    oldGRy[2] = 0.0F;
    oldGRy[3] = 0.0F;

    oldABax[0] = 0.0F;
    oldABax[1] = 0.0F;
    oldABax[2] = 0.0F;
    oldABax[3] = 0.0F;

    oldABay[0] = 0.0F;
    oldABay[1] = 0.0F;
    oldABay[2] = 0.0F;
    oldABay[3] = 0.0F;

    oldABaz[0] = 0.0F;
    oldABaz[1] = 0.0F;
    oldABaz[2] = 0.0F;
    oldABaz[3] = 0.0F;

}


/*
** CalcBodyOrientation
** Description:
** Based on the angular velocities relative to body, calc the
** euler angles using quaternion integration
*/
void
HeliMMClass::CalcBodyOrientation(void)
{
    float e1dot, e2dot, e3dot, e4dot;
    float enorm;
    float e1temp, e2temp, e3temp, e4temp;

    /*-----------------------------------*/
    /* quaternion differential equations */
    /*-----------------------------------*/
    e1dot = (-e4 * VB.ax - e3 * VB.ay - e2 * VB.az) * 0.5F;
    e2dot = (-e3 * VB.ax + e4 * VB.ay + e1 * VB.az) * 0.5F;
    e3dot = (e2 * VB.ax + e1 * VB.ay - e4 * VB.az) * 0.5F;
    e4dot = (e1 * VB.ax - e2 * VB.ay + e3 * VB.az) * 0.5F;

    /*-----------------------*/
    /* integrate quaternions */
    /*-----------------------*/
    e1temp = Math.FITust(e1dot, dT, olde1);
    e2temp = Math.FITust(e2dot, dT, olde2);
    e3temp = Math.FITust(e3dot, dT, olde3);
    e4temp = Math.FITust(e4dot, dT, olde4);

    /*--------------------------*/
    /* quaternion normalization */
    /*--------------------------*/
    enorm = (float)sqrt(e1temp * e1temp + e2temp * e2temp +
                        e3temp * e3temp + e4temp * e4temp);
    e1    = e1temp / enorm;
    e2    = e2temp / enorm;
    e3    = e3temp / enorm;
    e4    = e4temp / enorm;

    /*------------------------------*/
    /* reset quaternion integrators */
    /*------------------------------*/
    olde1[0] = e1;
    olde2[0] = e2;
    olde3[0] = e3;
    olde4[0] = e4;

    /*-------------------*/
    /* direction cosines */
    /*-------------------*/
    platform->dmx[0][0] = e1 * e1 - e2 * e2 -
                          e3 * e3 + e4 * e4;
    platform->dmx[0][1] = 2.0F * (e3 * e4 + e1 * e2);
    platform->dmx[0][2] = 2.0F * (e2 * e4 - e1 * e3);

    platform->dmx[1][0] = 2.0F * (e3 * e4 - e1 * e2);
    platform->dmx[1][1] = e1 * e1 - e2 * e2 +
                          e3 * e3 - e4 * e4;
    platform->dmx[1][2] = 2.0F * (e2 * e3 + e4 * e1);

    platform->dmx[2][0] = 2.0F * (e1 * e3 + e2 * e4);
    platform->dmx[2][1] = 2.0F * (e2 * e3 - e1 * e4);
    platform->dmx[2][2] = e1 * e1 + e2 * e2 -
                          e3 * e3 - e4 * e4;

    /*--------------*/
    /* euler angles */
    /*--------------*/

    // pitch (ay) constrained to +/- 90 deg, roll(ax) +/- 180 deg
    /*
    XE.az   =  (float)atan2(platform->dmx[0][1],platform->dmx[0][0]);
    XE.ay = -(float)asin(platform->dmx[0][2]);
    XE.ax =  (float)atan2(platform->dmx[1][2],platform->dmx[2][2]);
    */

    // roll (ax) constrained to +/- 90 deg, pitch(ay) +/- 180 deg
    XE.az   = (float)atan2(platform->dmx[0][1], platform->dmx[0][0]);
    XE.ax = (float)asin(platform->dmx[1][2]);
    XE.ay = -(float)atan2(platform->dmx[0][2], platform->dmx[2][2]);

}


/*
** SimpleModel
** Description:
** Based on the angular velocities relative to body, calc the
** euler angles using quaternion integration
*/
void
HeliMMClass::SimpleModel(void)
{
    float  totspeed;
    float dx, dy;
    float len;
    float tmp;
    mlTrig trig;


    ctlroll = -ctlroll;

    // pitch rate
    if (ctlroll)
    {
        // pitch where we want to be
        tmp = ctlroll * MAX_HELI_PITCH;
        VE.ay = tmp - XE.ay;
    }
    else
        VE.ay = -XE.ay;

    // roll rate and yawrate are tied together
    if (ctlpitch)
    {
        tmp = ctlpitch * MAX_HELI_ROLL;
        VE.ax = tmp - XE.ax;
        VE.az = (ctlpitch * MAX_HELI_YAWRATE);
    }
    else
    {
        VE.ax = -XE.ax;
        VE.az = -VE.az;
    }

    // climb rate
    VE.z = -(ctlcpitch - 0.5f) * MAX_HELI_CLIMBRATE;

    XE.ax = XE.ax + dT * TIME_CHEAT * (A2 * VE.ax + B2 * VEprev.ax);
    XE.ay = XE.ay + dT * TIME_CHEAT * (A2 * VE.ay + B2 * VEprev.ay);
    XE.az = XE.az + dT * TIME_CHEAT * (A2 * VE.az + B2 * VEprev.az);

    if (XE.ax > MAX_HELI_ROLL)
    {
        XE.ax = MAX_HELI_ROLL;
        VE.ax = 0.0f;
    }
    else if (XE.ax < -MAX_HELI_ROLL)
    {
        XE.ax = -MAX_HELI_ROLL;
        VE.ax = 0.0f;
    }

    if (XE.ay > MAX_HELI_PITCH)
    {
        XE.ay = MAX_HELI_PITCH;
        VE.ay = 0.0f;
    }
    else if (XE.ay < -MAX_HELI_PITCH)
    {
        XE.ay = -MAX_HELI_PITCH;
        VE.ay = 0.0f;
    }

    if (XE.az > DTR * 360.0f)
        XE.az -= 360.0f * DTR;
    else if (XE.az < 0.0f)
        XE.az += 360.0f * DTR;

    // get sines and cosines
    mlSinCos(&trig, XE.ax);  // Roll
    eucos.x = trig.cos;
    eusin.x = trig.sin;
    mlSinCos(&trig, XE.ay);  // Pitch
    eucos.y = trig.cos;
    eusin.y = trig.sin;
    mlSinCos(&trig, XE.az);  // Yaw
    eucos.z = trig.cos;
    eusin.z = trig.sin;

    platform->platformAngles.cospsi = (float)eucos.z;
    platform->platformAngles.costhe = (float)eucos.y;
    platform->platformAngles.cosphi = (float)eucos.x;
    platform->platformAngles.sinpsi = (float)eusin.z;
    platform->platformAngles.sinthe = (float)eusin.y;
    platform->platformAngles.sinphi = (float)eusin.x;

    // build matrix
    platform->dmx[0][0] = platform->platformAngles.cospsi * platform->platformAngles.costhe;
    platform->dmx[0][1] = platform->platformAngles.sinpsi * platform->platformAngles.costhe;
    platform->dmx[0][2] = -platform->platformAngles.sinthe;

    platform->dmx[1][0] = -platform->platformAngles.sinpsi * platform->platformAngles.cosphi + platform->platformAngles.cospsi * platform->platformAngles.sinthe * platform->platformAngles.sinphi;
    platform->dmx[1][1] = platform->platformAngles.cospsi * platform->platformAngles.cosphi + platform->platformAngles.sinpsi * platform->platformAngles.sinthe * platform->platformAngles.sinphi;
    platform->dmx[1][2] = platform->platformAngles.costhe * platform->platformAngles.sinphi;

    platform->dmx[2][0] = platform->platformAngles.sinpsi * platform->platformAngles.sinphi + platform->platformAngles.cospsi * platform->platformAngles.sinthe * platform->platformAngles.cosphi;
    platform->dmx[2][1] = -platform->platformAngles.cospsi * platform->platformAngles.sinphi + platform->platformAngles.sinpsi * platform->platformAngles.sinthe * platform->platformAngles.cosphi;
    platform->dmx[2][2] = platform->platformAngles.costhe * platform->platformAngles.cosphi;


    // speed is based on pitch
    totspeed = -(XE.ay / MAX_HELI_PITCH) * MAX_HELI_FPS;

    // get the x,y plane forward pointing vector components and normalize
    dx = platform->dmx[0][0];
    dy = platform->dmx[0][1];
    len = (float)sqrt(dx * dx + dy * dy);
    dx = dx / len;
    dy = dy / len;

    VE.x = dx * totspeed;
    VE.y = dy * totspeed;
    XE.x = XE.x + dT * TIME_CHEAT * (A2 * VE.x + B2 * VEprev.x);
    XE.y = XE.y + dT * TIME_CHEAT * (A2 * VE.y + B2 * VEprev.y);
    XE.z = XE.z + dT * TIME_CHEAT * (A2 * VE.z + B2 * VEprev.z);

    // save prev
    VEprev = VE;
    VB = VE;

    // get alpha and beta
    alpha = (float)atan2(VB.z, VB.x) * RTD;
    beta = (float)atan2(VB.y, VB.x) * RTD;

    GetKias = totspeed * FTPSEC_TO_KNOTS;

    // set other platform data -- may be unnecessary
    float t1, t2;
    float alpharad, betarad;

    alpharad = alpha * DTR;
    betarad  = beta  * DTR;

    mlSinCos(&trig, alpharad);
    platform->platformAngles.sinalp = trig.sin;
    platform->platformAngles.cosalp = trig.cos;
    mlSinCos(&trig, betarad);
    platform->platformAngles.sinbet = trig.sin;
    platform->platformAngles.cosbet = trig.cos;

    platform->platformAngles.tanbet = (float)tan(betarad);

    /*-----------------------------*/
    /* velocity vector orientation */
    /*-----------------------------*/

    /*-------*/
    /* gamma */
    /*-------*/
    platform->platformAngles.singam = (platform->platformAngles.sinthe *
                                       platform->platformAngles.cosalp - platform->platformAngles.costhe *
                                       platform->platformAngles.cosphi * platform->platformAngles.sinalp) *
                                      platform->platformAngles.cosbet - platform->platformAngles.costhe *
                                      platform->platformAngles.sinphi * platform->platformAngles.sinbet;

    platform->platformAngles.cosgam = (float)sqrt(1.0f -
                                      platform->platformAngles.singam * platform->platformAngles.singam);

    gmma  = (float)asin(platform->platformAngles.singam);


    /*----*/
    /* mu */
    /*----*/
    t1 = platform->platformAngles.costhe * platform->platformAngles.sinphi *
         platform->platformAngles.cosbet + (platform->platformAngles.sinthe *
                                            platform->platformAngles.cosalp - platform->platformAngles.costhe *
                                            platform->platformAngles.cosphi * platform->platformAngles.sinalp) *
         platform->platformAngles.sinbet;
    t2 = platform->platformAngles.costhe * platform->platformAngles.cosphi *
         platform->platformAngles.cosalp + platform->platformAngles.sinthe *
         platform->platformAngles.sinalp;

    mu     = (float)atan2(t1, t2);
    platform->platformAngles.sinmu  = mu * t1;
    platform->platformAngles.cosmu  = mu * t2;

    /*-------*/
    /* sigma */
    /*-------*/
    t1 = (-platform->platformAngles.sinphi *
          platform->platformAngles.sinalp * platform->platformAngles.cosbet +
          platform->platformAngles.cosphi * platform->platformAngles.sinbet) *
         platform->platformAngles.cospsi + ((platform->platformAngles.costhe *
                                            platform->platformAngles.cosalp + platform->platformAngles.sinthe *
                                            platform->platformAngles.cosphi * platform->platformAngles.sinalp) *
                                            platform->platformAngles.cosbet + platform->platformAngles.sinthe *
                                            platform->platformAngles.sinphi * platform->platformAngles.sinbet) *
         platform->platformAngles.sinpsi;
    t2 = ((platform->platformAngles.costhe *
           platform->platformAngles.cosalp + platform->platformAngles.sinthe *
           platform->platformAngles.cosphi * platform->platformAngles.sinalp) *
          platform->platformAngles.cosbet + platform->platformAngles.sinthe *
          platform->platformAngles.sinphi * platform->platformAngles.sinbet) *
         platform->platformAngles.cospsi + (platform->platformAngles.sinphi *
                                            platform->platformAngles.sinalp * platform->platformAngles.cosbet -
                                            platform->platformAngles.cosphi * platform->platformAngles.sinbet) *
         platform->platformAngles.sinpsi;

    sigma  = (float)atan2(t1, t2);
    platform->platformAngles.sinsig = sigma * t1;
    platform->platformAngles.cossig = sigma * t2;

    vta = (float)sqrt(
              VB.x * VB.x +
              VB.y * VB.y +
              VB.z * VB.z);

}
