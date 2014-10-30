#include "stdhdr.h"
#include "aircrft.h"
#include "fack.h"
#include "airframe.h"
#include "soundfx.h"
#include "fsound.h"
#include "fcc.h"
#include "radar.h"
#include "falcmesg.h"
#include "simdrive.h"
#include "flightData.h"
#include "otwdrive.h"
#include "cpmanager.h"
#include "sms.h"
#include "playerop.h"
#include "limiters.h"
#include "IvibeData.h"
#include "falcsess.h"

// MD -- 20031011: added to make the RWR related SetLightBits2() calls work
#include "PlayerRWR.h"

// MD -- 20031207: adding for TFR STBY lamp check
#include "lantirn.h"

/* not GetFault(obs_wrn) and //never get's set currently*/

//extern bool g_bEnableAircraftLimits; //me123 MI replaced with g_bRealisticAvionics
extern bool g_bRealisticAvionics;

//MI
bool Warned = FALSE;
//extern bool g_bEnableCATIIIExtension; //MI replaced with g_bRealisticAvionics
void AircraftClass::CautionCheck(void)
{
    if ( not isDigital)
    {
        // Check fuel
        if (af->Fuel() + af->ExternalFuel() < bingoFuel and not mFaults->GetFault(FaultClass::fms_fault))
        {
            if (g_bRealisticAvionics)
            {
                //MI added for ICP stuff.
                //rewritten 04/21/01
#if 0
                bingoFuel = bingoFuel * 0.5F;

                //Update our ICP readout
                if (OTWDriver.pCockpitManager->mpIcp->IsICPSet(ICPClass::BINGO_MODE))
                    OTWDriver.pCockpitManager->mpIcp->ExecBingo();

                if (bingoFuel < 100.0F)
                    bingoFuel = -1.0F;

                cockpitFlightData.SetLightBit(FlightData::FuelLow);
                mFaults->SetFault(fuel_low_fault);
                mFaults->SetMasterCaution();
                F4SoundFXSetDist(af->auxaeroData->sndBBBingo, TRUE, 0.0f, 1.0f);
#else

                //Only warn us if we've not already been warned.
                if ( not mFaults->GetFault(fuel_low_fault))
                {
                    cockpitFlightData.SetLightBit(FlightData::FuelLow);
                    //mFaults->SetFault(fuel_low_fault);
                    mFaults->SetWarning(fuel_low_fault);

                    //Smeg 27-Oct-2003 - added not to play Bingo when not alrady being played. Probably a typo.
                    //if( not F4SoundFXPlaying( af->auxaeroData->sndBBBingo)) // MLR 5/16/2004 -
                    //   F4SoundFXSetDist( af->auxaeroData->sndBBBingo, TRUE, 0.0f, 1.0f );
                    if ( not SoundPos.IsPlaying(af->auxaeroData->sndBBBingo)) // MLR 5/16/2004 -
                        SoundPos.Sfx(af->auxaeroData->sndBBBingo);
                }

#endif
            }
            else
            {
                //me123 let's set a bingo manualy
                bingoFuel =   100.0f;

                if (af->Fuel() <=  100.0F)
                    bingoFuel = -10.0F;

                cockpitFlightData.SetLightBit(FlightData::FuelLow);
                mFaults->SetFault(fuel_low_fault);
                mFaults->SetMasterCaution();

                //if( not F4SoundFXPlaying(af->auxaeroData->sndBBBingo)) // MLR 5/16/2004 -
                //   F4SoundFXSetDist( af->auxaeroData->sndBBBingo, TRUE, 0.0f, 1.0f );
                if ( not SoundPos.IsPlaying(af->auxaeroData->sndBBBingo)) // MLR 5/16/2004 -
                    SoundPos.Sfx(af->auxaeroData->sndBBBingo);

            }

        }
        //MI reset our fuel low fault if we set a bingo value below our current level
        else if (g_bRealisticAvionics and mFaults->GetFault(fuel_low_fault))
        {
            if (bingoFuel < af->Fuel() + af->ExternalFuel())
            {
                cockpitFlightData.ClearLightBit(FlightData::FuelLow);
                mFaults->ClearFault(fuel_low_fault);
            }
        }

        // Caution TO/LDG Config
        //MI
        //if(IsF16())
        //TJL 10/20/03 Allowing TO/LDG Config warning on all aircraft
        if ( not isDigital)
        {
            //RV - I-Hawk - changed altitude value from 10000 to 5000 according to Dannycoh
            if (ZPos() > -5000.0F and GetKias() < 190.0F and ZDelta() * 60.0F >= 250.0F and af->gearPos not_eq 1.0F)
            {
                if ( not mFaults->GetFault(to_ldg_config))
                {
                    if ( not g_bRealisticAvionics)
                        mFaults->SetFault(to_ldg_config);
                    else
                        mFaults->SetWarning(to_ldg_config);
                }
            }
            else
                mFaults->ClearFault(to_ldg_config);

            // JPO check for trapped fuel
            if ( not mFaults->GetFault(FaultClass::fms_fault) and af->CheckTrapped())
            {
                if ( not mFaults->GetFault(fuel_trapped))
                {
                    if ( not g_bRealisticAvionics)
                        mFaults->SetFault(fuel_trapped);
                    else
                        mFaults->SetWarning(fuel_trapped);
                }
            }
            else
                mFaults->ClearFault(fuel_trapped);

            //MI Fuel HOME warning
            if ( not mFaults->GetFault(FaultClass::fms_fault) and af->CheckHome())
            {
                if ( not mFaults->GetFault(fuel_home))
                {
                    if ( not g_bRealisticAvionics)
                        mFaults->SetFault(fuel_home);
                    else
                        mFaults->SetWarning(fuel_home);

                    //Make noise
                    //   if( not F4SoundFXPlaying(af->auxaeroData->sndBBBingo))
                    //   F4SoundFXSetDist( af->auxaeroData->sndBBBingo, TRUE, 0.0f, 1.0f );
                    if ( not SoundPos.IsPlaying(af->auxaeroData->sndBBBingo)) // MLR 5/16/2004 -
                        SoundPos.Sfx(af->auxaeroData->sndBBBingo);

                }
            }
            else
                mFaults->ClearFault(fuel_home);
        }

        /////////////me123 let's brake something if we fly too fast
        //me123 OWLOOK switch here to enable aircraft limits (overg and max speed)
        //if (g_bEnableAircraftLimits) { MI
        if (g_bRealisticAvionics)
        {
            // Marco Edit - OverG DOES NOT affect 
            // (at least not before the aircraft falls apart)
            //MI put back in after discussing with Marco
            CheckForOverG();
            CheckForOverSpeed();
        }

        // save for later.
        int savewarn = mFaults->WarnReset();
        int savemc = mFaults->MasterCaution();

        //// JPO - check hydraulics too.
        ///////////
        if ((af->rpm * 37.0F) < 15.0F or mFaults->GetFault(FaultClass::eng_fault))
        {
            if ( not mFaults->GetFault(oil_press))
            {
                if ( not g_bRealisticAvionics)
                    // less than 15 psi
                    mFaults->SetFault(oil_press);
                else
                    mFaults->SetWarning(oil_press);
            }
        }
        else
            mFaults->ClearFault(oil_press);

        if ( not af->HydraulicOK())
        {
            if ( not mFaults->GetFault(hyd))
            {
                if ( not g_bRealisticAvionics)
                    mFaults->SetFault(hyd);
                else
                    mFaults->SetWarning(hyd);
            }
        }
        else
            mFaults->ClearFault(hyd);

        // JPO Sec is active below 20% rpm
        if (af->rpm < 0.20F)
        {
            if ( not mFaults->GetFault(sec_fault))
            {
                if ( not g_bRealisticAvionics)
                    mFaults->SetFault(sec_fault);
                else
                    mFaults->SetCaution(sec_fault);
            }
        }
        else
            mFaults->ClearFault(sec_fault);

        // this is a hack JPO
        // when starting up we don't want to set the warn/caution lights,
        // but we do want the indicator lights.
        // so we clear the cautions if nothing else had them set.
        if (af->rpm < 1e-2 and OnGround())
        {
            if (savewarn == 0) mFaults->ClearWarnReset();

            if (savemc == 0) mFaults->ClearMasterCaution();
        }

#if 0 // JPO: I don't think this makes any sense to me... me123????

        ///////////me123 changed 27000 to -27000
        if (ZPos() < -27000.0F and mFaults->GetFault(FaultClass::eng_fault))
        {
            if ( not mFaults->GetFault(to_ldg_config))
                mFaults->SetFault(cabin_press_fault);
        }
        else
            mFaults->ClearFault(cabin_press_fault);

#endif

        // JPO - dump dumps cabin pressure, off means its not there anyway.
        // 10000 is a guess - thats where you requirte oxygen
        // MD -- 20031105: the dash one says this caution kicks in at 27k MSL
        if (ZPos() < -27000 and (af->GetAirSource() == AirframeClass::AS_DUMP or
                                af->GetAirSource() == AirframeClass::AS_OFF))
        {
            if ( not mFaults->GetFault(cabin_press_fault))
            {
                if ( not g_bRealisticAvionics)
                    mFaults->SetFault(cabin_press_fault);
                else
                    mFaults->SetCaution(cabin_press_fault);
            }
        }
        else
            mFaults->ClearFault(cabin_press_fault);

        if (mFaults->GetFault(FaultClass::hud_fault) and mFaults->GetFault(FaultClass::fcc_fault))
        {
            if ( not mFaults->GetFault(canopy))
            {
                if ( not g_bRealisticAvionics)
                    mFaults->SetFault(canopy);
                else
                    mFaults->SetWarning(canopy);
            }
        }
        else
            mFaults->ClearFault(canopy);

        ///////////
        if (mFaults->GetFault(FaultClass::fcc_fault))
        {
            if ( not mFaults->GetFault(dual_fc))
            {
                if ( not g_bRealisticAvionics)
                    mFaults->SetFault(dual_fc);
                else
                    mFaults->SetWarning(dual_fc);
            }
        }
        else
            mFaults->ClearFault(dual_fc);

        ///////////
        if (mFaults->GetFault(FaultClass::amux_fault) or mFaults->GetFault(FaultClass::bmux_fault))
        {
            if ( not g_bRealisticAvionics)
                mFaults->SetFault(avionics_fault);
            else
                mFaults->SetCaution(avionics_fault);
        }
        else
        {
            mFaults->ClearFault(avionics_fault);
        }

        ////////////
        if (mFaults->GetFault(FaultClass::ralt_fault))
        {
            if ( not mFaults->GetFault(radar_alt_fault))
            {
                if ( not g_bRealisticAvionics)
                    mFaults->SetFault(radar_alt_fault);
                else
                    mFaults->SetCaution(radar_alt_fault);
            }
        }
        else
            mFaults->ClearFault(radar_alt_fault);

        ///////////////
        if (mFaults->GetFault(FaultClass::iff_fault))
        {
            if ( not mFaults->GetFault(iff_fault))
            {
                if ( not g_bRealisticAvionics)
                    mFaults->SetFault(iff_fault);
                else
                    mFaults->SetCaution(iff_fault);
            }
        }
        else
            mFaults->ClearFault(iff_fault);

        ///////////////
        if (mFaults->GetFault(FaultClass::rwr_fault))
        {
            if ( not mFaults->GetFault(ecm_fault))
            {
                if ( not g_bRealisticAvionics)
                    mFaults->SetFault(ecm_fault);
                else
                    mFaults->SetCaution(ecm_fault);
            }
        }
        else
            mFaults->ClearFault(ecm_fault);

        ///////////////
        if (mFaults->GetFault(FaultClass::rwr_fault))
        {
            if ( not mFaults->GetFault(nws_fault))
            {
                if ( not g_bRealisticAvionics)
                    mFaults->SetFault(nws_fault);
                else
                    mFaults->SetCaution(nws_fault);
            }
        }
        else
            mFaults->ClearFault(nws_fault);

        /////////////
        //MI
        // Overheat Fault
        if (mFaults->GetFault(FaultClass::eng_fault) and af->rpm <= 0.75)
        {
            if ( not mFaults->GetFault(overheat_fault))
            {
                if ( not g_bRealisticAvionics)
                    mFaults->SetFault(overheat_fault);
                else
                    mFaults->SetCaution(overheat_fault);
            }
        }
        else
            mFaults->ClearFault(overheat_fault);


        // if lg up and aoa and speed
        // if airbrakes on
        //MI what kind of bullshit is this anyway?????
        if ( not g_bRealisticAvionics)
        {
            if (mFaults->GetFault(FaultClass::rwr_fault))
            {
                mFaults->SetFault(to_ldg_config);
            }
            else
            {
                mFaults->ClearFault(to_ldg_config);
            }
        }

        //////////////
        // Set external data if this is Ownship and player
        if (this == SimDriver.GetPlayerEntity())
            SetExternalData();

        // AMUX and BMUX combined failure forces FCC into NAV
        if (mFaults->GetFault(FaultClass::amux_fault) and mFaults->GetFault(FaultClass::bmux_fault))
        {
            FCC->SetMasterMode(FireControlComputer::Nav);
        }

        // If blanker broken, no ECM
        if (mFaults->GetFault(FaultClass::blkr_fault))
        {
            SensorClass* theRwr = FindSensor(this, SensorClass::RWR);

            if (theRwr)
                theRwr->SetPower(FALSE);

            UnSetFlag(ECM_ON);
        }

        // Shut down radar when broken
        if (mFaults->GetFault(FaultClass::fcc_fault) == FaultClass::xmtr)
        {
            RadarClass* theRadar = (RadarClass*)FindSensor(this, SensorClass::Radar);

            if (theRadar)
                theRadar->SetEmitting(FALSE);
        }

        if (mFaults->GetFault(FaultClass::fcc_fault) == FaultClass::bus)
        {
            RadarClass* theRadar = (RadarClass*)FindSensor(this, SensorClass::Radar);

            if (theRadar)
                theRadar->SetPower(FALSE);
        }

        // Shut down rwr when broken
        if (mFaults->GetFault(FaultClass::rwr_fault))
        {
            SensorClass* theRwr = FindSensor(this, SensorClass::RWR);

            if (theRwr)
                theRwr->SetPower(FALSE);
        }

        // Shut down HTS when broken
        if (mFaults->GetFault(FaultClass::harm_fault))
        {
            SensorClass* theHTS = FindSensor(this, SensorClass::HTS);

            if (theHTS)
                theHTS->SetPower(FALSE);
        }
    }

    //MI new home of the wrong/correct CAT stuff
    //if(af->platform->IsPlayer() and g_bEnableCATIIIExtension) MI
    if (IsPlayer() and g_bRealisticAvionics)
    {
        float MaxG = af->curMaxGs;
        float limitGs = 6.5f;
        Limiter *limiter = NULL; // JPO - use dynamic figure , not 6.5


        if (limiter = gLimiterMgr->GetLimiter(CatIIIMaxGs, af->VehicleIndex()))
            limitGs = limiter->Limit(0);

        if (MaxG <= limitGs)
        {
            //we need CATIII
            if ( not af->IsSet(AirframeClass::CATLimiterIII))
                WrongCAT();
            else
                CorrectCAT();
        }
        else
        {
            //we don't need CATIII
            if (af->IsSet(AirframeClass::CATLimiterIII))
                WrongCAT();
            else
                CorrectCAT();
        }
    }




    //MI Seat Arm switch
    if (IsPlayer() and g_bRealisticAvionics)
    {
        if ( not SeatArmed)
        {
            if ( not mFaults->GetFault(seat_notarmed_fault))
                mFaults->SetCaution(seat_notarmed_fault);
        }
        else
            mFaults->ClearFault(seat_notarmed_fault);
    }

    if (g_bRealisticAvionics)
    {
        //MI WARN Reset stuff
        //me123 loopign warnign sound is just T_LCFG i think
        if (cockpitFlightData.IsSet(FlightData::T_L_CFG)) //this one gives continous warning
        {
            //sound
            if (mFaults->WarnReset())
            {
                if (vuxGameTime >= WhenToPlayWarning)
                {
                    if ( not SoundPos.IsPlaying(SFX_BB_WARNING)) // MLR 5/16/2004 -
                        SoundPos.Sfx(SFX_BB_WARNING);

                    //    F4SoundFXSetDist(SFX_BB_WARNING, FALSE, 0.0f, 1.0f );
                }
            }
        }
        else
        {
            if (mFaults->DidManWarn())
            {
                mFaults->ClearManWarnReset();
                mFaults->ClearWarnReset();
            }
        }

        //MI Caution sound
        if (NeedsToPlayCaution)
        {
            if (vuxGameTime >= WhenToPlayCaution)
            {
                if (mFaults->MasterCaution())
                {
                    //    if( not F4SoundFXPlaying( af->auxaeroData->sndBBCaution))
                    //        F4SoundFXSetDist( af->auxaeroData->sndBBCaution, TRUE, 0.0f, 1.0f );
                    if ( not SoundPos.IsPlaying(af->auxaeroData->sndBBCaution)) // MLR 5/16/2004 -
                        SoundPos.Sfx(af->auxaeroData->sndBBCaution);


                }

                NeedsToPlayCaution = FALSE;
            }
        }

        if (NeedsToPlayWarning)
        {
            if (vuxGameTime >= WhenToPlayWarning)
            {
                if (mFaults->WarnReset())
                {
                    //    if( not F4SoundFXPlaying(af->auxaeroData->sndBBWarning))
                    //    F4SoundFXSetDist(af->auxaeroData->sndBBWarning, TRUE, 0.0f, 1.0f );
                    if ( not SoundPos.IsPlaying(af->auxaeroData->sndBBWarning)) // MLR 5/16/2004 -
                        SoundPos.Sfx(af->auxaeroData->sndBBWarning);

                }

                NeedsToPlayWarning = FALSE;
            }
        }

        //MI RF In SILENT gives TF FAIL
        if (RFState == 2)
        {
            if ( not mFaults->GetFault(tf_fail))
                mFaults->SetWarning(tf_fail);
        }
        else
            mFaults->ClearFault(tf_fail);
    }
}

//MI make some noise when overstressing
void AircraftClass::DamageSounds(void)
{
    int sound = rand() % 5;
    sound++;

    switch (sound)
    {
        case 1:
            SoundPos.Sfx(SFX_HIT_5);
            break;

        case 2:
            SoundPos.Sfx(SFX_HIT_4);
            break;

        case 3:
            SoundPos.Sfx(SFX_HIT_3);
            break;

        case 4:
            SoundPos.Sfx(SFX_HIT_2);
            break;

        case 5:
            SoundPos.Sfx(SFX_HIT_1);
            break;

        default:
            break;
    }
}

//MI warn us when in wrong config
void AircraftClass::WrongCAT(void)
{
    if (Warned)
        return;

    //set our fault
    mFaults->SetCaution(stores_config_fault);
    //mark that we've been here
    Warned = TRUE;
}

//MI we've switched to the correct CAT
void AircraftClass::CorrectCAT(void)
{
    if ( not Warned)
        return;

    //clear our fault
    mFaults->ClearFault(stores_config_fault);
    //mark that we've been here
    Warned = FALSE;
}

void AircraftClass::CheckForOverG(void)
{
    //check for bombs etc
    if (GetNz() > af->curMaxGs)
    {
        if (this == FalconLocalSession->GetPlayerEntity())
        {
            g_intellivibeData.IsOverG = true;
        }

        //let us know we're approaching overG
        GSounds();

        if (af->curMaxGs == 5.5 and GetNz() > af->curMaxGs + GToleranceBombs)
        {
            StoreToDamage(wcRocketWpn);
            StoreToDamage(wcBombWpn);
            StoreToDamage(wcAgmWpn);
            StoreToDamage(wcHARMWpn);
            StoreToDamage(wcSamWpn);
            StoreToDamage(wcGbuWpn);
        }

        //Tanks have 7G
        if ((af->curMaxGs == 7.0 or af->curMaxGs == 6.5) and GetNz() > af->curMaxGs + GToleranceTanks)
            if (GetNz() > af->curMaxGs + GToleranceTanks)
                StoreToDamage(wcTank);
    }
    else
    {
        if (this == FalconLocalSession->GetPlayerEntity())
            g_intellivibeData.IsOverG = false;
    }
}

void AircraftClass::CheckForOverSpeed(void)
{
    if (af->curMaxGs == 5.5 and GetKias() > af->curMaxStoreSpeed)
    {
        SSounds();

        if (GetKias() > af->curMaxStoreSpeed + SpeedToleranceBombs)
        {
            StoreToDamage(wcRocketWpn);
            StoreToDamage(wcBombWpn);
            StoreToDamage(wcAgmWpn);
            StoreToDamage(wcHARMWpn);
            StoreToDamage(wcSamWpn);
            StoreToDamage(wcGbuWpn);
        }
    }

    if ((af->curMaxGs == 7.0 or af->curMaxGs == 6.5) and GetKias() > af->curMaxStoreSpeed)
    {
        GSounds();

        if (GetKias() > af->curMaxStoreSpeed + SpeedToleranceTanks)
            StoreToDamage(wcTank);
    }

    //TJL 11/03/03 Adding VNE violation
    if (af->vcas > af->maxVcas)
    {
        //F4SoundFXSetDist(af->auxaeroData->sndOverSpeed2, TRUE, 0.0f, (GetKias() - af->maxVcas) / 10);
        //SoundPos.Sfx(af->auxaeroData->sndOverSpeed2, 0, (GetKias() - af->maxVcas) / 10, 0); // MLR 5/16/2004 -
        if (af->vcas > (af->maxVcas + (af->maxVcas * 0.05)))
        {
            DamageSounds();

            //TJL 05/30/04 added more damage modeling
            if ( not mFaults->GetFault(FaultClass::flcs_fault))
            {
                mFaults->SetFault(FaultClass::flcs_fault, FaultClass::dual, FaultClass::fail, FALSE);
            }

            if (pctStrength > 0.0f)
            {
                pctStrength -= 0.25f;
            }

            if (rand() % 100 < 20)
            {
                CanopyDamaged = TRUE;
            }

            if (rand() % 100 < 25 and not LEFState(RT_LEF_OUT))
            {
                if (LEFState(RT_LEF_DAMAGED))
                {
                    LEFOn(RT_LEF_OUT);
                    RTLEFAOA = -20.0f;
                }
                else
                {
                    LEFOn(RT_LEF_DAMAGED);
                    RTLEFAOA = -20.0f;
                    LEFOn(RT_LEF_OUT);
                    LEFOn(LEFSASYNCH);
                }
            }

        }
    }

    //end VNE violation


}

void AircraftClass::DoOverGSpeedDamage(int station)
{
    if ( not mFaults)
        return;

    int damage = rand() % 100;
    damage++;

    switch (station)
    {
        case 1:
            if (damage < 95 and not GetStationFailed(Station1_Degr) and not GetStationFailed(Station1_Fail))
            {
                mFaults->SetFault(FaultClass::sms_fault, FaultClass::sta1, FaultClass::degr, FALSE);
                StationFailed(Station1_Degr);
            }
            else
            {
                if ( not GetStationFailed(Station1_Fail) and (GSoundsNFuel == 2 or GSoundsWFuel == 3))
                    mFaults->SetFault(FaultClass::sms_fault, FaultClass::sta1, FaultClass::fail, FALSE);

                StationFailed(Station1_Fail);
            }

            break;

        case 2:
            damage = rand() % 100;

            if (damage < 95 and not GetStationFailed(Station2_Degr) and not GetStationFailed(Station2_Fail))
            {
                mFaults->SetFault(FaultClass::sms_fault, FaultClass::sta2, FaultClass::degr, FALSE);
                StationFailed(Station2_Degr);
            }
            else
            {
                if ( not GetStationFailed(Station2_Fail) and (GSoundsNFuel == 2 or GSoundsWFuel == 3))
                    mFaults->SetFault(FaultClass::sms_fault, FaultClass::sta2, FaultClass::fail, FALSE);

                StationFailed(Station2_Fail);
            }

            break;

        case 3:
            damage = rand() % 100;

            if (damage < 95 and not GetStationFailed(Station3_Degr) and not GetStationFailed(Station3_Fail))
            {
                mFaults->SetFault(FaultClass::sms_fault, FaultClass::sta3, FaultClass::degr, FALSE);
                StationFailed(Station3_Degr);
            }
            else
            {
                if ( not GetStationFailed(Station3_Fail) and (GSoundsNFuel == 2 or GSoundsWFuel == 3))
                    mFaults->SetFault(FaultClass::sms_fault, FaultClass::sta3, FaultClass::fail, FALSE);

                StationFailed(Station3_Fail);
            }

            break;

        case 4:
            damage = rand() % 100;

            if (damage < 95 and not GetStationFailed(Station4_Degr) and not GetStationFailed(Station4_Fail))
            {
                mFaults->SetFault(FaultClass::sms_fault, FaultClass::sta4, FaultClass::degr, FALSE);
                StationFailed(Station4_Degr);
            }
            else
            {
                if ( not GetStationFailed(Station4_Fail) and (GSoundsNFuel == 2 or GSoundsWFuel == 3))
                    mFaults->SetFault(FaultClass::sms_fault, FaultClass::sta4, FaultClass::fail, FALSE);

                StationFailed(Station4_Fail);
            }

            break;

        case 5:
            damage = rand() % 100;

            if (damage < 95 and not GetStationFailed(Station5_Degr) and not GetStationFailed(Station5_Fail))
            {
                mFaults->SetFault(FaultClass::sms_fault, FaultClass::sta5, FaultClass::degr, FALSE);
                StationFailed(Station5_Degr);
            }
            else
            {
                if ( not GetStationFailed(Station5_Fail) and (GSoundsNFuel == 2 or GSoundsWFuel == 3))
                    mFaults->SetFault(FaultClass::sms_fault, FaultClass::sta5, FaultClass::fail, FALSE);

                StationFailed(Station5_Fail);
            }

            break;

        case 6:
            damage = rand() % 100;

            if (damage < 95 and not GetStationFailed(Station6_Degr) and not GetStationFailed(Station6_Fail))
            {
                mFaults->SetFault(FaultClass::sms_fault, FaultClass::sta6, FaultClass::degr, FALSE);
                StationFailed(Station6_Degr);
            }
            else
            {
                if ( not GetStationFailed(Station6_Fail) and (GSoundsNFuel == 2 or GSoundsWFuel == 3))
                    mFaults->SetFault(FaultClass::sms_fault, FaultClass::sta6, FaultClass::fail, FALSE);

                StationFailed(Station6_Fail);
            }

            break;

        case 7:
            damage = rand() % 100;

            if (damage < 95 and not GetStationFailed(Station7_Degr) and not GetStationFailed(Station7_Fail))
            {
                mFaults->SetFault(FaultClass::sms_fault, FaultClass::sta7, FaultClass::degr, FALSE);
                StationFailed(Station7_Degr);
            }
            else
            {
                if ( not GetStationFailed(Station7_Fail) and (GSoundsNFuel == 2 or GSoundsWFuel == 3))
                    mFaults->SetFault(FaultClass::sms_fault, FaultClass::sta7, FaultClass::fail, FALSE);

                StationFailed(Station7_Fail);
            }

            break;

        case 8:
            damage = rand() % 100;

            if (damage < 95 and not GetStationFailed(Station8_Degr) and not GetStationFailed(Station8_Fail))
            {
                mFaults->SetFault(FaultClass::sms_fault, FaultClass::sta8, FaultClass::degr, FALSE);
                StationFailed(Station8_Degr);
            }
            else
            {
                if ( not GetStationFailed(Station8_Fail) and (GSoundsNFuel == 2 or GSoundsWFuel == 3))
                    mFaults->SetFault(FaultClass::sms_fault, FaultClass::sta8, FaultClass::fail, FALSE);

                StationFailed(Station8_Fail);
            }

            break;

        case 9:
            damage = rand() % 100;

            if (damage < 95 and not GetStationFailed(Station9_Degr) and not GetStationFailed(Station9_Fail))
            {
                mFaults->SetFault(FaultClass::sms_fault, FaultClass::sta9, FaultClass::degr, FALSE);
                StationFailed(Station9_Degr);
            }
            else
            {
                if ( not GetStationFailed(Station9_Fail) and (GSoundsNFuel == 2 or GSoundsWFuel == 3))
                    mFaults->SetFault(FaultClass::sms_fault, FaultClass::sta9, FaultClass::fail, FALSE);

                StationFailed(Station9_Fail);
            }

            break;

        default:
            break;
    }
}

void AircraftClass::StoreToDamage(WeaponClass thing)
{
    if ( not g_bRealisticAvionics or (PlayerOptions.Realism < 0.76 and not isDigital))
        return;

    if ( not Sms or not mFaults or not af)
        return;

    //Check which station to fail
    int center = (Sms->NumHardpoints() - 1) / 2 + 1;

    for (int i = 0; i < Sms->NumHardpoints(); i++)
    {
        // if its a tank - try and guess which one.
        if (Sms->hardPoint[i] and 
            Sms->hardPoint[i]->GetWeaponClass() == thing)
        {
            //tanks cause our Fuel Management System to fail.
            if (thing == wcTank)
            {
                if (GetNz() > af->curMaxGs + GToleranceTanks)
                {
                    AdjustTankG(1);
                    DoOverGSpeedDamage(i);

                    if ( not mFaults->GetFault(FaultClass::fms_fault))
                        mFaults->SetFault(FaultClass::fms_fault, FaultClass::bus, FaultClass::degr, FALSE);

                    if (GSoundsWFuel == 0)
                    {
                        DamageSounds();
                        GSoundsWFuel = 2;
                    }
                }

                if (GetNz() > af->curMaxGs + GToleranceTanks)
                {
                    AdjustTankG(2);
                    DoOverGSpeedDamage(i);

                    if ( not mFaults->GetFault(FaultClass::fms_fault))
                        mFaults->SetFault(FaultClass::fms_fault, FaultClass::bus, FaultClass::fail, FALSE);

                    if (GSoundsWFuel == 2)
                    {
                        DamageSounds();
                        GSoundsWFuel++;
                    }
                }

                if (GetNz() >= af->curMaxGs + GToleranceTanks)
                {
                    AdjustTankG(3);
                    DoOverGSpeedDamage(i);

                    if ( not mFaults->GetFault(FaultClass::flcs_fault))
                        mFaults->SetFault(FaultClass::flcs_fault, FaultClass::sngl, FaultClass::fail, FALSE);

                    if (GSoundsWFuel == 3)
                    {
                        DamageSounds();
                        GSoundsWFuel++;
                    }
                }

                if (GetKias() > af->curMaxStoreSpeed + SpeedToleranceTanks)
                {
                    AdjustTankSpeed(1);
                    DoOverGSpeedDamage(i);

                    if ( not mFaults->GetFault(FaultClass::fms_fault))
                        mFaults->SetFault(FaultClass::fms_fault, FaultClass::bus, FaultClass::fail, FALSE);

                    if (SpeedSoundsWFuel == 0)
                    {
                        DamageSounds();
                        SpeedSoundsWFuel++;
                    }
                }

                if (GetKias() > af->curMaxStoreSpeed + SpeedToleranceTanks)
                {
                    AdjustTankSpeed(2);
                    DoOverGSpeedDamage(i);

                    if ( not mFaults->GetFault(FaultClass::fms_fault))
                        mFaults->SetFault(FaultClass::fms_fault, FaultClass::bus, FaultClass::fail, FALSE);

                    if (SpeedSoundsWFuel == 1)
                    {
                        DamageSounds();
                        SpeedSoundsWFuel++;
                    }
                }

                if (af->mach >= 2.05f or GetKias() > af->curMaxStoreSpeed + SpeedToleranceTanks)
                {
                    AdjustTankSpeed(3);
                    DoOverGSpeedDamage(i);

                    if ( not mFaults->GetFault(FaultClass::flcs_fault))
                        mFaults->SetFault(FaultClass::flcs_fault, FaultClass::sngl, FaultClass::fail, FALSE);

                    if ( not mFaults->GetFault(FaultClass::cadc_fault))
                        mFaults->SetFault(FaultClass::cadc_fault, FaultClass::bus, FaultClass::degr, FALSE);

                    if (SpeedSoundsWFuel == 2)
                    {
                        DamageSounds();
                        SpeedSoundsWFuel++;
                    }
                }
            }

            if (thing == wcBombWpn or thing == wcRocketWpn or thing == wcAgmWpn or
                thing == wcHARMWpn or thing == wcSamWpn or thing == wcGbuWpn)
            {
                if (GetNz() > af->curMaxGs + GToleranceBombs)
                {
                    AdjustBombG(1);
                    DoOverGSpeedDamage(i);

                    if ( not mFaults->GetFault(FaultClass::isa_fault))
                        mFaults->SetFault(FaultClass::isa_fault, FaultClass::sngl, FaultClass::fail, FALSE);

                    if (GSoundsNFuel == 0)
                    {
                        DamageSounds();
                        GSoundsNFuel = 2;
                    }
                }

                if (GetNz() >= af->curMaxGs + GToleranceBombs)
                {
                    AdjustBombG(2);
                    DoOverGSpeedDamage(i);

                    if ( not mFaults->GetFault(FaultClass::sms_fault))
                        mFaults->SetFault(FaultClass::sms_fault, FaultClass::bus, FaultClass::degr, FALSE);

                    if (GSoundsNFuel == 2)
                    {
                        DamageSounds();
                        GSoundsNFuel++;
                    }
                }

                if (GetNz() >= af->curMaxGs + GToleranceBombs)
                {
                    AdjustBombG(3);
                    DoOverGSpeedDamage(i);

                    if ( not mFaults->GetFault(FaultClass::flcs_fault))
                        mFaults->SetFault(FaultClass::flcs_fault, FaultClass::dual, FaultClass::fail, FALSE);

                    if ( not mFaults->GetFault(FaultClass::sms_fault) bitand FaultClass::bus bitand FaultClass::fail)
                        mFaults->SetFault(FaultClass::sms_fault, FaultClass::bus, FaultClass::fail, FALSE);

                    if (GSoundsNFuel == 3)
                    {
                        DamageSounds();
                        GSoundsNFuel++;
                    }
                }

                if (GetKias() > af->curMaxStoreSpeed + SpeedToleranceBombs)
                {
                    AdjustBombSpeed(1);
                    DoOverGSpeedDamage(i);

                    if (SpeedSoundsNFuel == 0)
                    {
                        DamageSounds();
                        SpeedSoundsNFuel++;
                    }
                }

                if (GetKias() > af->curMaxStoreSpeed + SpeedToleranceBombs)
                {
                    AdjustBombSpeed(2);
                    DoOverGSpeedDamage(i);

                    if ( not mFaults->GetFault(FaultClass::fms_fault))
                        mFaults->SetFault(FaultClass::fms_fault, FaultClass::bus, FaultClass::fail, FALSE);

                    if ( not mFaults->GetFault(FaultClass::isa_fault))
                        mFaults->SetFault(FaultClass::isa_fault, FaultClass::sngl, FaultClass::fail, FALSE);

                    if (SpeedSoundsNFuel == 1)
                    {
                        DamageSounds();
                        SpeedSoundsNFuel++;
                    }
                }

                if (af->mach >= 2.05f or GetKias() > af->curMaxStoreSpeed + SpeedToleranceBombs)
                {
                    AdjustBombSpeed(3);
                    DoOverGSpeedDamage(i);

                    if ( not mFaults->GetFault(FaultClass::flcs_fault))
                        mFaults->SetFault(FaultClass::flcs_fault, FaultClass::sngl, FaultClass::fail, FALSE);

                    if ( not mFaults->GetFault(FaultClass::cadc_fault))
                        mFaults->SetFault(FaultClass::cadc_fault, FaultClass::bus, FaultClass::degr, FALSE);

                    if ( not mFaults->GetFault(FaultClass::amux_fault))
                        mFaults->SetFault(FaultClass::amux_fault, FaultClass::bus, FaultClass::degr, FALSE);

                    if ( not mFaults->GetFault(FaultClass::fcc_fault))
                        mFaults->SetFault(FaultClass::fcc_fault, FaultClass::bus, FaultClass::degr, FALSE);

                    if (SpeedSoundsNFuel == 2)
                    {
                        DamageSounds();
                        SpeedSoundsNFuel++;
                    }
                }
            }
        }
    }
}

void AircraftClass::SetExternalData(void)
{

    // MD -- 20031113: Adding support for the MAL IND button the test panel to update the
    // shared memory bit preperly.  Also play the VMS in accordance with the dash one; technically
    // this should mean saying each of the words once each while the button is held in the test
    // position but this doesn't seem to be possible without new sounds for this purpose.
    // MD -- 20031216: oops forgot the AVTR lamp in HsiBits
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (playerAC == NULL)
    {
        // sfr: NULL check
        return;
    }

    if (playerAC->TestLights not_eq NULL)
    {
        cockpitFlightData.lightBits  = FlightData::AllLampBitsOn;   // All current lamp related lightBits  to ON
        cockpitFlightData.lightBits2 = FlightData::AllLampBits2On;  // All current lamp related lightBits2 to ON
        cockpitFlightData.lightBits3 = FlightData::AllLampBits3On;  // All current lamp related lightBits3 to ON
        cockpitFlightData.hsiBits = FlightData::AllLampHsiBitsOn;   // All current lamp related HsiBits to ON
        //if( not F4SoundFXPlaying(SFX_BB_ALLWORDS))
        // F4SoundFXSetDist(SFX_BB_ALLWORDS, TRUE, 0.0f, 1.0f );
        SoundPos.Sfx(SFX_BB_ALLWORDS); // MLR 5/16/2004 -
        return;
    }

    /* // MLR 5/16/2004 -
    else {
     if (F4SoundFXPlaying(SFX_BB_ALLWORDS))  // stop the littany if player lets up on the MAL/IND switch
     F4StopSound(SFX_DEF[SFX_BB_ALLWORDS].handle);
    }
    */

    // Master Caution
    //if (OTWDriver.pCockpitManager->mMiscStates.GetMasterCautionLight())
    // MD -- 20031007: don't look at the cockpit art light, look at the fault state to
    // determine whether to set the sh.mem value.  This makes sure that it goes on and
    // off even if the lamp is not in the current field of view.
    if (playerAC->mFaults->MasterCaution())
    {
        cockpitFlightData.SetLightBit(FlightData::MasterCaution);
    }
    else
    {
        cockpitFlightData.ClearLightBit(FlightData::MasterCaution);
    }

    // Oil Pressure
    if ((af->rpm * 37.0F) < 15.0F or mFaults->GetFault(FaultClass::eng_fault))
        cockpitFlightData.SetLightBit(FlightData::OIL);
    else
        cockpitFlightData.ClearLightBit(FlightData::OIL);

    if ( not af->HydraulicOK() and ( not (((AircraftClass*)(playerAC))->MainPower() == AircraftClass::MainPowerOff)))
        cockpitFlightData.SetLightBit(FlightData::HYD);
    else
        cockpitFlightData.ClearLightBit(FlightData::HYD);

    // Cabin Pressure
    if (ZPos() < 27000.0F and mFaults->GetFault(FaultClass::eng_fault))
        cockpitFlightData.SetLightBit(FlightData::CabinPress);
    else
        cockpitFlightData.ClearLightBit(FlightData::CabinPress);

    // Canopy Light
    if (mFaults->GetFault(FaultClass::hud_fault) and mFaults->GetFault(FaultClass::fcc_fault))
        cockpitFlightData.SetLightBit(FlightData::CAN);
    else
        cockpitFlightData.ClearLightBit(FlightData::CAN);

    // FLCS
    if (mFaults->GetFault(FaultClass::fcc_fault))
        cockpitFlightData.SetLightBit(FlightData::DUAL);
    else
        cockpitFlightData.ClearLightBit(FlightData::DUAL);

    // Avioncs Caution
    if ( not g_bRealisticAvionics)
    {
        if (mFaults->GetFault(FaultClass::amux_fault) or mFaults->GetFault(FaultClass::bmux_fault))
            cockpitFlightData.SetLightBit(FlightData::Avionics);
        else
            cockpitFlightData.ClearLightBit(FlightData::Avionics);
    }
    else
    {
        if (mFaults->NeedAckAvioncFault)
            cockpitFlightData.SetLightBit(FlightData::Avionics);
        else
            cockpitFlightData.ClearLightBit(FlightData::Avionics);
    }

    // Radar altimeter
    if (mFaults->GetFault(radar_alt_fault))
        cockpitFlightData.SetLightBit(FlightData::RadarAlt);
    else
        cockpitFlightData.ClearLightBit(FlightData::RadarAlt);

    // IFF Fault
    if (mFaults->GetFault(FaultClass::iff_fault))
        cockpitFlightData.SetLightBit(FlightData::IFF);
    else
        cockpitFlightData.ClearLightBit(FlightData::IFF);

    // AOA Indicator lights
    if (cockpitFlightData.alpha >= 14.0F)
    {
        cockpitFlightData.SetLightBit(FlightData::AOAAbove);
        cockpitFlightData.ClearLightBit(FlightData::AOAOn);
        cockpitFlightData.ClearLightBit(FlightData::AOABelow);
    }
    else if ((cockpitFlightData.alpha < 14.0F) and (cockpitFlightData.alpha >= 11.5F))
    {
        cockpitFlightData.ClearLightBit(FlightData::AOAAbove);
        cockpitFlightData.SetLightBit(FlightData::AOAOn);
        cockpitFlightData.ClearLightBit(FlightData::AOABelow);
    }
    else
    {
        cockpitFlightData.ClearLightBit(FlightData::AOAAbove);
        cockpitFlightData.ClearLightBit(FlightData::AOAOn);
        cockpitFlightData.SetLightBit(FlightData::AOABelow);
    }

    //Commented out by JPG - 1/1/04 - The AoA indexer is operational regardless of the gear being up
    // or down - per the -1
    //MI only operational with gear down
    /*if(g_bRealisticAvionics and af and af->gearPos < 0.8F)
    {
     cockpitFlightData.ClearLightBit(FlightData::AOAOn);
     cockpitFlightData.ClearLightBit(FlightData::AOABelow);
     cockpitFlightData.ClearLightBit(FlightData::AOAAbove);
    }*/

    // Nose Wheel Steering
    // MD -- 20031215: this was pretty broken in that it only recorded what happens to
    // the NWS function.  Adding refueling status indications now.

    // For 3 seconds after disconnect the DISC light comes on
    // cross reference to cblights.cpp where this check used to be
    // This is probably not a great place for this but it should at
    // least work properly.  Leaving this over in cblights would mean that
    // the timecheck would only be preformed if the lamp is on the screen.

    if (OTWDriver.pCockpitManager->mMiscStates.mRefuelState == 3 and (vuxGameTime > (OTWDriver.pCockpitManager->mMiscStates.mRefuelTimer + 3000)))
    {
        OTWDriver.pCockpitManager->mMiscStates.SetRefuelState(0);
    }

    if (playerAC->af->IsSet(AirframeClass::NoseSteerOn) or
        (OTWDriver.pCockpitManager->mMiscStates.mRefuelState == 2))  // contact
        cockpitFlightData.SetLightBit(FlightData::RefuelAR);
    else
        cockpitFlightData.ClearLightBit(FlightData::RefuelAR);

    if (playerAC->af->IsEngineFlag(AirframeClass::FuelDoorOpen) and 
        (OTWDriver.pCockpitManager->mMiscStates.mRefuelState <= 1))  // ready to refuel
        cockpitFlightData.SetLightBit(FlightData::RefuelRDY);
    else
        cockpitFlightData.ClearLightBit(FlightData::RefuelRDY);

    if (OTWDriver.pCockpitManager->mMiscStates.mRefuelState == 3)
        cockpitFlightData.SetLightBit(FlightData::RefuelDSC);
    else
        cockpitFlightData.ClearLightBit(FlightData::RefuelDSC);


    // FLCS
    if (mFaults->GetFault(FaultClass::flcs_fault))
        cockpitFlightData.SetLightBit(FlightData::FltControlSys);
    else
        cockpitFlightData.ClearLightBit(FlightData::FltControlSys);

    // Engine Fault
    if (mFaults->GetFault(FaultClass::eng_fault))
        cockpitFlightData.SetLightBit(FlightData::EngineFault);
    else
        cockpitFlightData.ClearLightBit(FlightData::EngineFault);

    // Overheat Fault
    if (mFaults->GetFault(FaultClass::eng_fault) and af->rpm <= 0.75)
        cockpitFlightData.SetLightBit(FlightData::Overheat);
    else
        cockpitFlightData.ClearLightBit(FlightData::Overheat);

    // These are not faults, they are cautions
    if (mFaults->GetFault(FaultClass::eng_fault) bitand FaultClass::efire)
        cockpitFlightData.SetLightBit(FlightData::ENG_FIRE);
    else
        cockpitFlightData.ClearLightBit(FlightData::ENG_FIRE);

    // TJL 01/24/04 multi-engine
    // These are not faults, they are cautions
    if (mFaults->GetFault(FaultClass::eng2_fault) bitand FaultClass::efire)
        cockpitFlightData.SetLightBit3(FlightData::Eng2_Fire);
    else
        cockpitFlightData.ClearLightBit3(FlightData::Eng2_Fire);

    //MI
    if (mFaults->GetFault(stores_config_fault))
        cockpitFlightData.SetLightBit(FlightData::CONFIG);
    else
        cockpitFlightData.ClearLightBit(FlightData::CONFIG);

    // Caution TO/LDG Config
    // MD -- 20031120: replace test with fault check
    if (mFaults->GetFault(to_ldg_config))
        cockpitFlightData.SetLightBit(FlightData::T_L_CFG);
    else
        cockpitFlightData.ClearLightBit(FlightData::T_L_CFG);

    if (mFaults->GetFault(nws_fault))
        cockpitFlightData.SetLightBit(FlightData::NWSFail);
    else
        cockpitFlightData.ClearLightBit(FlightData::NWSFail);

    if (mFaults->GetFault(cabin_press_fault))
        cockpitFlightData.SetLightBit(FlightData::CabinPress);
    else
        cockpitFlightData.ClearLightBit(FlightData::CabinPress);

    // MD -- 20031101: Adding a check for Weight on Wheels.  If the a/c is on the ground
    // and the gear is down and there are no gear faults WoW switch is "on"
    // this is not used elsewhere in the game but several places do use the OnGround() routine
    // to implement correct WoW behavior.
    if (playerAC->OnGround() and (af->gearPos == 1.0F) and 
        ( not playerAC->mFaults->GetFault(FaultClass::gear_fault)))
        cockpitFlightData.SetLightBit(FlightData::WOW);
    else
        cockpitFlightData.ClearLightBit(FlightData::WOW);

    // MD -- 20031125: adding a bit to show if the autopilot is engaged or not.  This will be
    // useful for cockpit builders that implement the rightmost AP switch with the same kind of
    // switch that is used in the F-16: a three place momentary that has the off-center positions
    // magnetically captured to hold the swithc out of the off position while the AP is functioning
    // within its operating limits.  Use this bit to turn on the solenoid if your switch has one

    if ((playerAC->IsOn(AircraftClass::AttHold)) or (playerAC->IsOn(AircraftClass::AltHold)))
        cockpitFlightData.SetLightBit(FlightData::AutoPilotOn);
    else
        cockpitFlightData.ClearLightBit(FlightData::AutoPilotOn);

    if (mFaults->GetFault(oxy_low_fault))
        cockpitFlightData.SetLightBit2(FlightData::OXY_LOW);
    else
        cockpitFlightData.ClearLightBit2(FlightData::OXY_LOW);

    if (mFaults->GetFault(fwd_fuel_low_fault))
        cockpitFlightData.SetLightBit2(FlightData::FwdFuelLow);
    else
        cockpitFlightData.ClearLightBit2(FlightData::FwdFuelLow);

    if (mFaults->GetFault(aft_fuel_low_fault))
        cockpitFlightData.SetLightBit2(FlightData::AftFuelLow);
    else
        cockpitFlightData.ClearLightBit2(FlightData::AftFuelLow);

    if (mFaults->GetFault(sec_fault))
        cockpitFlightData.SetLightBit2(FlightData::SEC);
    else
        cockpitFlightData.ClearLightBit2(FlightData::SEC);

    if (mFaults->GetFault(probeheat_fault))
        cockpitFlightData.SetLightBit2(FlightData::PROBEHEAT);
    else
        cockpitFlightData.ClearLightBit2(FlightData::PROBEHEAT);

    if (mFaults->GetFault(buc_fault))
        cockpitFlightData.SetLightBit2(FlightData::BUC);
    else
        cockpitFlightData.ClearLightBit2(FlightData::BUC);

    if (mFaults->GetFault(fueloil_hot_fault))
        cockpitFlightData.SetLightBit2(FlightData::FUEL_OIL_HOT);
    else
        cockpitFlightData.ClearLightBit2(FlightData::FUEL_OIL_HOT);

    if (mFaults->GetFault(anti_skid_fault))
        cockpitFlightData.SetLightBit2(FlightData::ANTI_SKID);
    else
        cockpitFlightData.ClearLightBit2(FlightData::ANTI_SKID);

    if (mFaults->GetFault(seat_notarmed_fault))
        cockpitFlightData.SetLightBit2(FlightData::SEAT_ARM);
    else
        cockpitFlightData.ClearLightBit2(FlightData::SEAT_ARM);

    // MD -- 20031011: Moved all the SetLightBit2() calls from cblights.cpp to here
    // which will ensure the bits in the flightData structure are consistent with the
    // state of the jet regardless of the OTW view state.

    if (playerAC->IsSetFlag(ECM_ON))
        cockpitFlightData.SetLightBit2(FlightData::EcmPwr);
    else
        cockpitFlightData.ClearLightBit2(FlightData::EcmPwr);

    if (playerAC->mFaults->GetFault(FaultClass::epod_fault) or
        playerAC->mFaults->GetFault(FaultClass::blkr_fault))
        cockpitFlightData.SetLightBit2(FlightData::EcmFail);
    else
        cockpitFlightData.ClearLightBit2(FlightData::EcmFail);

    PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(playerAC, SensorClass::RWR);

    if (theRwr) // MLR Somewhere inside here crashed.
    {
        if (theRwr->HasActivity())
            cockpitFlightData.SetLightBit2(FlightData::AuxAct);
        else
            cockpitFlightData.ClearLightBit2(FlightData::AuxAct);

        if (theRwr->LaunchIndication())
            cockpitFlightData.SetLightBit2(FlightData::Launch);
        else
            cockpitFlightData.ClearLightBit2(FlightData::Launch);

        if (theRwr->ManualSelect())
            cockpitFlightData.SetLightBit2(FlightData::HandOff);
        else
            cockpitFlightData.ClearLightBit2(FlightData::HandOff);

        if (theRwr->IsPriority())
            cockpitFlightData.SetLightBit2(FlightData::PriMode);
        else
            cockpitFlightData.ClearLightBit2(FlightData::PriMode);

        if (theRwr->LightUnknowns())
            cockpitFlightData.SetLightBit2(FlightData::Unk);
        else
            cockpitFlightData.ClearLightBit2(FlightData::Unk);

        if (theRwr->ShowNaval())
            cockpitFlightData.SetLightBit2(FlightData::Naval);
        else
            cockpitFlightData.ClearLightBit2(FlightData::Naval);

        if (theRwr->TargetSep())
            cockpitFlightData.SetLightBit2(FlightData::TgtSep);
        else
            cockpitFlightData.ClearLightBit2(FlightData::TgtSep);

        if (theRwr->LightSearch())
            cockpitFlightData.SetLightBit2(FlightData::AuxSrch);
        else
            cockpitFlightData.ClearLightBit2(FlightData::AuxSrch);

        if (theRwr->LowAltPriority())
            cockpitFlightData.SetLightBit2(FlightData::AuxLow);
        else
            cockpitFlightData.ClearLightBit2(FlightData::AuxLow);

        if (theRwr->IsOn())
            cockpitFlightData.SetLightBit2(FlightData::AuxPwr);
        else
            cockpitFlightData.ClearLightBit2(FlightData::AuxPwr);
    }

    if (((AircraftClass*)(playerAC))->AutopilotType() == AircraftClass::LantirnAP)
        cockpitFlightData.SetLightBit2(FlightData::TFR_ENGAGED);
    else
        cockpitFlightData.ClearLightBit2(FlightData::TFR_ENGAGED);

    // MD -- 20031207: This isn't exactly how the STBY light works in the real thing but given that the
    // LANTIRN implementation isn't complete and blended TF/AP pitch mode isn't implemented, this will
    // have to do for now.
    // Lamp should be on if the TFR mode is standby or if the AP Override is being held during TFR operation.

    if ((theLantirn and (theLantirn->GetTFRMode() == LantirnClass::TFR_STBY)) or
        ((playerAC->lastapType == AircraftClass::LantirnAP) and playerAC->IsOn(AircraftClass::Override)))
        cockpitFlightData.SetLightBit(FlightData::TFR_STBY);
    else
        cockpitFlightData.ClearLightBit(FlightData::TFR_STBY);

    // MD -- 20031011: this is a different logic flow that is used when looking at the cockpit
    // because here we only care about the lamp state not where the handle is (up/down)
    if (playerAC->mFaults->GetFault(FaultClass::gear_fault) or
        (playerAC->mFaults->GetFault(to_ldg_config) and playerAC->af->gearPos == 0.0F) or
        ((playerAC->af->gearPos not_eq 0.0F) and (playerAC->af->gearPos not_eq 1.0F)))
        cockpitFlightData.SetLightBit2(FlightData::GEARHANDLE);
    else
        cockpitFlightData.ClearLightBit2(FlightData::GEARHANDLE);

    // MD -- 20021011: end of lightBits2 fixes

    if (af->EpuIsAir())
        cockpitFlightData.SetLightBit3(FlightData::Air);
    else
        cockpitFlightData.ClearLightBit3(FlightData::Air);

    if (af->EpuIsHydrazine())
        cockpitFlightData.SetLightBit3(FlightData::Hydrazine);
    else
        cockpitFlightData.ClearLightBit3(FlightData::Hydrazine);

    if (ElecIsSet(AircraftClass::ElecFlcsPmg))
        cockpitFlightData.SetLightBit3(FlightData::FlcsPmg);
    else
        cockpitFlightData.ClearLightBit3(FlightData::FlcsPmg);

    if (ElecIsSet(AircraftClass::ElecEpuGen))
        cockpitFlightData.SetLightBit3(FlightData::EpuGen);
    else
        cockpitFlightData.ClearLightBit3(FlightData::EpuGen);

    if (ElecIsSet(AircraftClass::ElecEpuPmg))
        cockpitFlightData.SetLightBit3(FlightData::EpuPmg);
    else
        cockpitFlightData.ClearLightBit3(FlightData::EpuPmg);

    if (ElecIsSet(AircraftClass::ElecToFlcs))
        cockpitFlightData.SetLightBit3(FlightData::ToFlcs);
    else
        cockpitFlightData.ClearLightBit3(FlightData::ToFlcs);

    if (ElecIsSet(AircraftClass::ElecFlcsRly))
        cockpitFlightData.SetLightBit3(FlightData::FlcsRly);
    else
        cockpitFlightData.ClearLightBit3(FlightData::FlcsRly);

    if (ElecIsSet(AircraftClass::ElecBatteryFail))
        cockpitFlightData.SetLightBit3(FlightData::BatFail);
    else
        cockpitFlightData.ClearLightBit3(FlightData::BatFail);

    if (af->IsSet(AirframeClass::JfsStart))
        cockpitFlightData.SetLightBit2(FlightData::JFSOn);
    else
        cockpitFlightData.ClearLightBit2(FlightData::JFSOn);

    // JPO Is EPU running.
    if (af->GeneratorRunning(AirframeClass::GenEpu))
        cockpitFlightData.SetLightBit2(FlightData::EPUOn);
    else
        cockpitFlightData.ClearLightBit2(FlightData::EPUOn);

    // MD -- 20031221: This one is for Mirv -- separate light for the lower half of the ENG FIRE/ENGINE split face
    // lamp on the right side of the glareshield.
    if (((af->rpm <= 0.6F) and (((AircraftClass*)(playerAC))->MainPower() == AircraftClass::MainPowerMain)) or
        (cockpitFlightData.ftit > 1100.0F))
        cockpitFlightData.SetLightBit2(FlightData::ENGINE);
    else
        cockpitFlightData.ClearLightBit2(FlightData::ENGINE);

    // MD -- 20031011: this lights come on if the generator is *not* running
    // but only set this to on if the main power switch is not in the off position
    if ( not af->GeneratorRunning(AirframeClass::GenMain) and 
        ( not (((AircraftClass*)(playerAC))->MainPower() == AircraftClass::MainPowerOff)))
        cockpitFlightData.SetLightBit3(FlightData::MainGen);
    else
        cockpitFlightData.ClearLightBit3(FlightData::MainGen);

    // MD -- 20031011: this lights come on if the generator is *not* running
    // but only set this to on if the main power switch is not in the off position
    if ( not af->GeneratorRunning(AirframeClass::GenStdby) and 
        ( not (((AircraftClass*)(playerAC))->MainPower() == AircraftClass::MainPowerOff)))
        cockpitFlightData.SetLightBit3(FlightData::StbyGen);
    else
        cockpitFlightData.ClearLightBit3(FlightData::StbyGen);

    //MI RF In SILENT gives TF FAIL
    // MD -- 20031011: make the pattern consistent and ensure that if tf_fail is ever set anywhere else
    // in future, the caution light will get lit properly.
    //if(RFState == 2)
    if (mFaults->GetFault(tf_fail))
        cockpitFlightData.SetLightBit(FlightData::TF);
    else
        cockpitFlightData.ClearLightBit(FlightData::TF);

    if (mFaults->GetFault(elec_fault))
        cockpitFlightData.SetLightBit3(FlightData::Elec_Fault);
    else
        cockpitFlightData.ClearLightBit3(FlightData::Elec_Fault);

    if (mFaults->GetFault(alt_low))
        cockpitFlightData.SetLightBit(FlightData::ALT);
    else
        cockpitFlightData.ClearLightBit(FlightData::ALT);

    if (mFaults->GetFault(le_flaps_fault))
        cockpitFlightData.SetLightBit(FlightData::LEFlaps);
    else
        cockpitFlightData.ClearLightBit(FlightData::LEFlaps);

    if (mFaults->GetFault(ecm_fault))
        cockpitFlightData.SetLightBit(FlightData::ECM);
    else
        cockpitFlightData.ClearLightBit(FlightData::ECM);

    if (mFaults->GetFault(hook_fault))
        cockpitFlightData.SetLightBit(FlightData::Hook);
    else
        cockpitFlightData.ClearLightBit(FlightData::Hook);

    if (mFaults->GetFault(lef_fault))
        cockpitFlightData.SetLightBit3(FlightData::Lef_Fault);
    else
        cockpitFlightData.ClearLightBit3(FlightData::Lef_Fault);

    // MD -- 20031208: adding a bit for power off -- set if there is no electrical power

    if (((AircraftClass*)(playerAC))->MainPower() == AircraftClass::MainPowerOff)
        cockpitFlightData.SetLightBit3(FlightData::Power_Off);
    else
        cockpitFlightData.ClearLightBit3(FlightData::Power_Off);

    // MD -- 20040301: adding bits for the gear down and locked so you can see for sure
    // what the individual strut state looks like.
    if (GetDOFValue(ComplexGearDOF[0]) == (af->GetAeroData(AeroDataSet::NosGearRng) * DTR))
        cockpitFlightData.SetLightBit3(FlightData::NoseGearDown);
    else
        cockpitFlightData.ClearLightBit3(FlightData::NoseGearDown);

    if (GetDOFValue(ComplexGearDOF[1]) == (af->GetAeroData(AeroDataSet::NosGearRng + 4) * DTR))
        cockpitFlightData.SetLightBit3(FlightData::LeftGearDown);
    else
        cockpitFlightData.ClearLightBit3(FlightData::LeftGearDown);

    if (GetDOFValue(ComplexGearDOF[2]) == (af->GetAeroData(AeroDataSet::NosGearRng + 8) * DTR))
        cockpitFlightData.SetLightBit3(FlightData::RightGearDown);
    else
        cockpitFlightData.ClearLightBit3(FlightData::RightGearDown);

}

void AircraftClass::GSounds(void)
{
    //not if we're going down
    if ( not IsExploding() and not IsDead())
    {
        //F4SoundFXSetDist(af->auxaeroData->sndOverSpeed1, TRUE, 0.0f, 1.0f);
        //RV - I-Hawk - Added 0, 2, -1000 parameters to lower volume for such overG sound
        SoundPos.Sfx(af->auxaeroData->sndOverSpeed1, 0, 2, -1000); // MLR 5/16/2004 -
    }
}

void AircraftClass::SSounds(void)
{
    if ( not IsExploding() and not IsDead())
    {
        //F4SoundFXSetDist(af->auxaeroData->sndOverSpeed2, TRUE, 0.0f, (GetKias() - af->curMaxStoreSpeed) / 25);
        if (af->curMaxStoreSpeed)
        {
            float v;

            //v = k - l / (h-l)
            /*
            v = (GetKias() - af->curMaxStoreSpeed) / 50 ;
            if(v<0)
             v=0;
            else
             if(v>1)
             v=1;

            v=(1-v) * -2000;
            */

            v = -2000;
            v += ((GetKias() - af->curMaxStoreSpeed) / 50) * 250 ;

            //F4SoundFXSetDist(af->auxaeroData->sndOverSpeed2, TRUE, v, ( (GetKias() - af->curMaxStoreSpeed)) / 25);
            SoundPos.Sfx(af->auxaeroData->sndOverSpeed2, 0, 2 , v); // MLR 5/16/2004 -
        }
    }
}

void AircraftClass::AdjustTankSpeed(int level)
{
    //adjust for OverG/Speed
    switch (level)
    {
        case 1:
            SpeedToleranceTanks = OverSpeedToleranceTanks[1];
            break;

        case 2:
            SpeedToleranceTanks = OverSpeedToleranceTanks[2];
            break;

        case 3:
            SpeedToleranceTanks = 100; //No more damage
            break;

        default:
            break;
    }
}

void AircraftClass::AdjustBombSpeed(int level)
{
    //adjust for OverG/Speed
    switch (level)
    {
        case 1:
            SpeedToleranceBombs = OverSpeedToleranceBombs[1];
            break;

        case 2:
            SpeedToleranceBombs = OverSpeedToleranceBombs[2];
            break;

        case 3:
            SpeedToleranceBombs = 100; //No more damage
            break;

        default:
            break;
    }
}

void AircraftClass::AdjustTankG(int level)
{
    //adjust for OverG/Speed
    switch (level)
    {
        case 1:
            GToleranceTanks = float(OverGToleranceTanks[1]) / 10.0f;
            break;

        case 2:
            GToleranceTanks = float(OverGToleranceTanks[2]) / 10.0f;
            break;

        case 3:
            GToleranceTanks = 100; //No more damage
            break;

        default:
            break;
    }
}

void AircraftClass::AdjustBombG(int level)
{
    //adjust for OverG/Speed
    switch (level)
    {
        case 1:
            GToleranceBombs = float(OverGToleranceBombs[1]) / 10.0f;
            break;

        case 2:
            GToleranceBombs = float(OverGToleranceBombs[2]) / 10.0f;
            break;

        case 3:
            GToleranceBombs = 100; //No more damage
            break;

        default:
            break;
    }
}
