#include "stdhdr.h"
#include "fault.h"
#include "fack.h"
#include "debuggr.h"
#include "soundfx.h"
#include "fsound.h"
#include "simdrive.h"
#include "aircrft.h"
#include "cmpclass.h"
#include "flightData.h"

extern bool g_bRealisticAvionics;
//-------------------------------------------------
// FackClass::FackClass
//-------------------------------------------------

FackClass::FackClass()
{

    mMasterCaution = 0;
    NeedsWarnReset = 0; //MI Warn reset switch
    DidManWarnReset = 0; //MI Warn reset switch
    NeedAckAvioncFault = FALSE;
}

FackClass::~FackClass()
{
}

//-------------------------------------------------
// FackClass::IsFlagSet
//-------------------------------------------------

BOOL FackClass::IsFlagSet()
{

    return mCautions.IsFlagSet();
}

//-------------------------------------------------
// FackClass::ClearFlag
//-------------------------------------------------
void FackClass::ClearFlag()
{
    MonoPrint("remove call\n");
}

//-------------------------------------------------
// FackClass::SetFault
//-------------------------------------------------

void FackClass::SetFault(int systemBits, BOOL doWarningMsg)
{
    FaultClass::type_FSubSystem subSystem = mFaults.PickSubSystem(systemBits);
    FaultClass::type_FFunction function = mFaults.PickFunction(subSystem);


    //TJL 01/11/04 Added Additional ENUM list
    SetFault(subSystem, function, FaultClass::fail, doWarningMsg);
}

void FackClass::SetFault(
    FaultClass::type_FSubSystem subsystem,
    FaultClass::type_FFunction function,
    FaultClass::type_FSeverity severity,
    BOOL doWarningMsg)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (playerAC == NULL) return;

    FaultClass::str_FEntry entry;

    //ShiAssert(doWarningMsg == 0 or SimDriver.GetPlayerEntity()->mFaults == this); // should only apply to us
    GetFault(subsystem, &entry);

    // Set the fault
    mFaults.SetFault(subsystem, function, severity, doWarningMsg);

    // Adjust needed cautions
    if (entry.elFunction == FaultClass::nofault)
    {
        if (subsystem == FaultClass::eng_fault)
        {
            mCautions.SetCaution(engine);
        }
        //TJL 01/16/04 multi-engine
        else if (subsystem == FaultClass::eng2_fault)
        {
            mCautions.SetCaution(engine);
        }

        else if (subsystem == FaultClass::iff_fault)
        {
            mCautions.SetCaution(iff_fault);
        }
    }

    if ( not g_bRealisticAvionics)
    {
        mMasterCaution = TRUE;
        NeedsWarnReset = TRUE; //MI Warn Reset
    }
    else if (doWarningMsg)
    {
        //TJL 01/24/04 Added Eng2
        if (subsystem == FaultClass::amux_fault or
            subsystem == FaultClass::blkr_fault or
            subsystem == FaultClass::bmux_fault or
            subsystem == FaultClass::cadc_fault or
            subsystem == FaultClass::cmds_fault or
            subsystem == FaultClass::dlnk_fault or
            subsystem == FaultClass::dmux_fault or
            subsystem == FaultClass::dte_fault or
            subsystem == FaultClass::eng_fault or
            subsystem == FaultClass::eng2_fault or
            subsystem == FaultClass::epod_fault or
            subsystem == FaultClass::fcc_fault or
            subsystem == FaultClass::fcr_fault or
            subsystem == FaultClass::flcs_fault or
            subsystem == FaultClass::fms_fault or
            subsystem == FaultClass::gear_fault or
            subsystem == FaultClass::gps_fault or
            subsystem == FaultClass::harm_fault or
            subsystem == FaultClass::hud_fault or
            subsystem == FaultClass::iff_fault or
            subsystem == FaultClass::ins_fault or
            subsystem == FaultClass::isa_fault or
            subsystem == FaultClass::mfds_fault or
            subsystem == FaultClass::msl_fault or
            subsystem == FaultClass::ralt_fault or
            subsystem == FaultClass::rwr_fault or
            subsystem == FaultClass::sms_fault or
            subsystem == FaultClass::tcn_fault or
            subsystem == FaultClass::ufc_fault)
        {
            playerAC->NeedsToPlayCaution = TRUE;//caution
            SetMasterCaution(); //set our MasterCaution immediately
            playerAC->WhenToPlayCaution = vuxGameTime + 7 * CampaignSeconds;
            NeedAckAvioncFault = TRUE;
        }
        else
        {
            /*//these are warnings
            if(function == FaultClass::dual or
             function == FaultClass::efire or
             function == FaultClass::hydr)
            { */
            // sfr: this was inverted
            if ( not playerAC->NeedsToPlayWarning)
            {
                playerAC->WhenToPlayWarning = vuxGameTime + (unsigned long) 1.5 * CampaignSeconds;
            }

            SetWarnReset();
            playerAC->NeedsToPlayWarning = TRUE;// warning
        }
    }
}

//-------------------------------------------------
// FackClass::SetFault
//-------------------------------------------------

void FackClass::SetFault(type_CSubSystem subsystem)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if ( not playerAC)
    {
        return;
    }

    //ShiAssert(SimDriver.GetPlayerEntity()->mFaults == this); // should only apply to us
    if ( not mCautions.GetCaution(subsystem))
    {
        mCautions.SetCaution(subsystem);

        // No Master Caution for low_altitude warming - just bitchin' betty :-) - RH
        if (subsystem not_eq alt_low)
        {
            if ( not g_bRealisticAvionics)
            {
                mMasterCaution = TRUE;
                NeedsWarnReset = TRUE;
            }
            else
            {
                //these are warnings
                if (GetFault(tf_fail) or //never get's set currently
                    GetFault(obs_wrn) or //never get's set currently
                    GetFault(eng_fire) or
                    GetFault(eng2_fire) or //TJL 01/24/04 multi-engine
                    GetFault(hyd) or
                    GetFault(oil_press) or
                    GetFault(dual_fc) or
                    GetFault(to_ldg_config))
                {
                    if ( not playerAC->NeedsToPlayWarning)
                    {
                        playerAC->WhenToPlayWarning = vuxGameTime + (unsigned long) 1.5 * CampaignSeconds;
                    }

                    playerAC->NeedsToPlayWarning = TRUE;// warning
                    SetWarnReset();
                }

                //these are actually cautions
                if (subsystem not_eq fuel_low_fault)
                {
                    if (GetFault(stores_config_fault) or
                        GetFault(flt_cont_fault) or
                        GetFault(le_flaps_fault) or
                        GetFault(engine) or
                        GetFault(overheat_fault) or
                        GetFault(avionics_fault) or
                        GetFault(radar_alt_fault) or
                        GetFault(iff_fault) or
                        GetFault(ecm_fault) or
                        GetFault(hook_fault) or
                        GetFault(nws_fault) or
                        GetFault(cabin_press_fault) or
                        GetFault(fwd_fuel_low_fault) or
                        GetFault(aft_fuel_low_fault) or
                        GetFault(probeheat_fault) or
                        GetFault(seat_notarmed_fault) or
                        GetFault(buc_fault) or
                        GetFault(fueloil_hot_fault) or
                        GetFault(anti_skid_fault) or
                        GetFault(nws_fault) or
                        GetFault(oxy_low_fault) or
                        GetFault(sec_fault) or
                        GetFault(lef_fault))
                    {
                        if ( not playerAC->NeedsToPlayCaution and not cockpitFlightData.IsSet(FlightData::MasterCaution))
                        {
                            playerAC->WhenToPlayCaution = vuxGameTime + 7 * CampaignSeconds;
                        }

                        playerAC->NeedsToPlayCaution = TRUE;//caution
                        SetMasterCaution(); //set our MasterCaution immediately
                    }
                }

                if (subsystem == fuel_low_fault)
                {
                    //MI need flashing on HUD
                    SetWarnReset();
                }
            }
        }
    }
}

//-------------------------------------------------
// FackClass::ClearFault
//-------------------------------------------------

void FackClass::ClearFault(FaultClass::type_FSubSystem subsystem)
{

    mFaults.ClearFault(subsystem);

    if (subsystem == FaultClass::eng_fault)
    {
        mCautions.ClearCaution(engine);
    }
    //TJL 01/16/04 multi-engine
    else if (subsystem == FaultClass::eng2_fault)
    {
        mCautions.ClearCaution(engine);
    }
    else if (subsystem == FaultClass::iff_fault)
    {
        mCautions.ClearCaution(iff_fault);
    }
}

//-------------------------------------------------
// FackClass::ClearFault
//-------------------------------------------------
void FackClass::ClearFault(type_CSubSystem subsystem)
{

    if (g_bRealisticAvionics)
    {
        //warnings
        if ( not GetFault(tf_fail) and //never get's set currently
 not GetFault(obs_wrn) and //never get's set currently
 not GetFault(eng_fire) and 
 not GetFault(eng2_fire) and //TJL 01/24/04 multi-engine
 not GetFault(hyd) and 
 not GetFault(oil_press) and 
 not GetFault(dual_fc) and 
 not GetFault(to_ldg_config) and 
 not GetFault(fuel_low_fault) and 
 not GetFault(fuel_trapped) and 
 not GetFault(fuel_home))
        {
            ClearWarnReset();
        }

        //Cautions
        if ( not GetFault(stores_config_fault) and 
 not GetFault(flt_cont_fault) and 
 not GetFault(le_flaps_fault) and 
 not GetFault(engine) and 
 not GetFault(overheat_fault) and 
 not GetFault(avionics_fault) and 
 not GetFault(radar_alt_fault) and 
 not GetFault(iff_fault) and 
 not GetFault(ecm_fault) and 
 not GetFault(hook_fault) and 
 not GetFault(nws_fault) and 
 not GetFault(cabin_press_fault) and 
 not GetFault(fwd_fuel_low_fault) and 
 not GetFault(aft_fuel_low_fault) and 
 not GetFault(probeheat_fault) and 
 not GetFault(seat_notarmed_fault) and 
 not GetFault(buc_fault) and 
 not GetFault(fueloil_hot_fault) and 
 not GetFault(anti_skid_fault) and 
 not GetFault(nws_fault) and 
 not GetFault(oxy_low_fault) and 
 not GetFault(sec_fault) and 
 not GetFault(elec_fault) and 
 not GetFault(lef_fault) and 
 not NeedAckAvioncFault)
        {
            ClearMasterCaution();

        }
    }

    mCautions.ClearCaution(subsystem);
}

//-------------------------------------------------
// FackClass::GetFault
//-------------------------------------------------

void FackClass::GetFault(FaultClass::type_FSubSystem subsystem, FaultClass::str_FEntry* pentry)
{

    mFaults.GetFault(subsystem, pentry);
}

//-------------------------------------------------
// FackClass::GetFault
//-------------------------------------------------

BOOL FackClass::GetFault(FaultClass::type_FSubSystem subsystem)
{

    return mFaults.GetFault(subsystem);
}

//-------------------------------------------------
// FackClass::GetFault
//-------------------------------------------------

BOOL FackClass::GetFault(type_CSubSystem subsystem)
{

    return mCautions.GetCaution(subsystem);
}

//-------------------------------------------------
// FackClass::GetFaultNames
//-------------------------------------------------

void FackClass::GetFaultNames(FaultClass::type_FSubSystem subsystem, int funcNum, FaultClass::str_FNames* pnames)
{

    mFaults.GetFaultNames(subsystem, funcNum, pnames);
}

void FackClass::TotalPowerFailure()
{
    mFaults.TotalPowerFailure(); // JPO

    //MI need to route these thru the appropriate function
    //since we have electrics in non realistic mode, we have to go this way....
    if (g_bRealisticAvionics)
    {
        SetCaution(radar_alt_fault);
        SetCaution(le_flaps_fault);
        SetCaution(hook_fault);
        SetCaution(nws_fault);
        SetCaution(ecm_fault);
        SetCaution(iff_fault);
    }
    else
    {
        mCautions.SetCaution(radar_alt_fault);
        mCautions.SetCaution(le_flaps_fault);
        mCautions.SetCaution(hook_fault);
        mCautions.SetCaution(nws_fault);
        mCautions.SetCaution(ecm_fault);
        mCautions.SetCaution(iff_fault);
    }

    if ( not g_bRealisticAvionics)
        mMasterCaution = TRUE;

    /*if( not SimDriver.GetPlayerEntity()->NeedsToPlayCaution)
     SimDriver.GetPlayerEntity()->WhenToPlayCaution = vuxGameTime + 7*CampaignSeconds;
    SimDriver.GetPlayerEntity()->NeedsToPlayCaution = TRUE;*/
}

void FackClass::RandomFailure()
{
    mFaults.RandomFailure(); // THW Same as above, just for random failure

    //Wombat778 2-25-04 Removed because if the fault is random, surely we shouldnt be setting these cautions every time.

    /* if(g_bRealisticAvionics)
     {
     SetCaution(radar_alt_fault);
     SetCaution(le_flaps_fault);
     SetCaution(hook_fault);
     SetCaution(nws_fault);
     SetCaution(ecm_fault);
     SetCaution(iff_fault);
     }
     else
     {
     mCautions.SetCaution(radar_alt_fault);
     mCautions.SetCaution(le_flaps_fault);
     mCautions.SetCaution(hook_fault);
     mCautions.SetCaution(nws_fault);
     mCautions.SetCaution(ecm_fault);
     mCautions.SetCaution(iff_fault);
     }*/

    if ( not g_bRealisticAvionics)
        mMasterCaution = TRUE;
}

//MI
void FackClass::SetWarning(type_CSubSystem subsystem)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (playerAC == NULL)
    {
        return;
    }

    ShiAssert(playerAC->mFaults == this); // should only apply to us

    if ( not mCautions.GetCaution(subsystem))
    {
        mCautions.SetCaution(subsystem);

        if ( not playerAC->NeedsToPlayWarning)
            playerAC->WhenToPlayWarning = vuxGameTime + (unsigned long) 1.5 * CampaignSeconds;

        if ( not GetFault(fuel_low_fault) and 
 not GetFault(fuel_home))//no betty for bingo
            playerAC->NeedsToPlayWarning = TRUE;// warning

        SetWarnReset();
    }
}
void FackClass::SetCaution(type_CSubSystem subsystem)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (playerAC == NULL)
    {
        return;
    }

    ShiAssert(playerAC->mFaults == this); // should only apply to us

    if ( not mCautions.GetCaution(subsystem))
    {
        mCautions.SetCaution(subsystem);

        if ( not playerAC->NeedsToPlayCaution and not cockpitFlightData.IsSet(FlightData::MasterCaution))
        {
            playerAC->WhenToPlayCaution = vuxGameTime + 7 * CampaignSeconds;
        }

        playerAC->NeedsToPlayCaution = TRUE;//caution
        SetMasterCaution(); //set our MasterCaution immediately
    }
}
