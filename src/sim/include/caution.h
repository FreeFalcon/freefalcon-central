#ifndef _CAUTION_H
#define _CAUTION_H

#ifndef _WINDOWS_
#include <windows.h>
#endif

//-------------------------------------------------
// A whole slew of caution lights
//-------------------------------------------------

typedef enum type_CSubSystem
{
    tf_fail,
    obs_wrn,
    alt_low,
    eng_fire,
    engine,
    hyd,
    oil_press,
    dual_fc,
    canopy,
    to_ldg_config,
    flt_cont_fault,
    le_flaps_fault,
    overheat_fault,
    fuel_low_fault,
    avionics_fault,
    radar_alt_fault,
    iff_fault,
    ecm_fault,
    hook_fault,
    nws_fault,
    cabin_press_fault,
    oxy_low_fault,
    fwd_fuel_low_fault,
    aft_fuel_low_fault,
    fuel_trapped,
    fuel_home,
    sec_fault,
    probeheat_fault,
    stores_config_fault,
    buc_fault,
    fueloil_hot_fault,
    anti_skid_fault,
    seat_notarmed_fault,
    equip_host_fault,
    elec_fault,
    lef_fault, //MI
    eng2_fire, //TJL 01/24/04 multi-engine

    lastFault
};

//-------------------------------------------------
// A whole slew of threat warning lights
//-------------------------------------------------

typedef enum type_TWSubSystem
{
    handoff, missile_launch,
    pri_mode, sys_test,
    tgt_t, unk,
    search, activate_power,
    system_power, low_altitude
};

#define BITS_PER_VECTOR    32

const int NumVectors = (lastFault / BITS_PER_VECTOR) + 1;


//-------------------------------------------------
// Class Defintion
//-------------------------------------------------

class CautionClass
{


    unsigned int mpBitVector[NumVectors];

    void SetCaution(int);
    void ClearCaution(int);
    BOOL GetCaution(int);

public:

    BOOL IsFlagSet();
    void ClearFlag();

    void SetCaution(type_CSubSystem);
    void SetCaution(type_TWSubSystem);

    void ClearCaution(type_CSubSystem);
    void ClearCaution(type_TWSubSystem);

    BOOL GetCaution(type_CSubSystem);
    BOOL GetCaution(type_TWSubSystem);

    CautionClass();
};

#endif
