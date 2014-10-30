#include "stdhdr.h"
#include "simobj.h"
#include "DrawParticleSys.h"
#include "PilotInputs.h"
#include "fcc.h"
#include "digi.h"
#include "radarDigi.h"
#include "radar360.h"
#include "radarSuper.h"
#include "radarDoppler.h"
#include "alr56.h"
#include "easyHts.h"
#include "advancedHts.h"
#include "irst.h"
#include "sms.h"
#include "airframe.h"
#include "initdata.h"
#include "object.h"
#include "fsound.h"
#include "soundfx.h"
#include "otwdrive.h"
#include "hardpnt.h"
#include "campwp.h"
#include "dogfight.h"
#include "missile.h"
#include "misldisp.h"
#include "f4error.h"
#include "campbase.h"
#include "unit.h"
#include "playerop.h"
#include "camp2sim.h"
#include "sfx.h"
#include "acdef.h"
#include "simeject.h"
#include "fakerand.h"
#include "navsystem.h"
#include "falcsess.h"
#include "laserpod.h"
#include "classtbl.h"
#include "caution.h"
#include "hud.h"
#include "eyeball.h"
#include "iaction.h"
#include "mfd.h"
#include "commands.h"
#include "aircrft.h"
#include "flight.h"
#include "Graphics/Include/drawbsp.h"
#include "Graphics/Include/drawsgmt.h"
#include "fcc.h"
#include "icp.h"
#include "cpmanager.h"
#include "ACTurbulence.h"
#include "Graphics/Include/tod.h"
#include "Fakerand.h"
//TJL 01/02/04
extern int set3DTexture;
extern bool g_bWakeTurbulence;

#ifdef DEBUG
void SimObjCheckOwnership(FalconEntity *);
#endif


// hack for making sure simple model STAYS SET
BOOL playerFlightModelHack = 0;
void MakeDogfightTopTen(int mode);
WeaponType playerLastWeaponType = wtNone;
WeaponClass playerLastWeaponClass = wcNoWpn;
FireControlComputer::FCCMasterMode playerLastMasterMode = FireControlComputer::Nav;
//MI
//FireControlComputer::FCCSubMode playerLastSubMode = FireControlComputer::TimeToGo;
FireControlComputer::FCCSubMode playerLastSubMode = FireControlComputer::ETE;
extern int gPlayerExitMenuShown;
int gUnlimitedAmmo = 0;
extern bool g_bUnlimitedAmmo;//Cobra

int AircraftClass::Wake(void)
{
    int retval = 0;

    if (IsAwake())
    {
        return retval;
    }

    SimVehicleClass::Wake();


    if ( not (DrawableBSP*)drawPointer)
        return retval;

    if (g_bWakeTurbulence)
    {
        /*
        if( not turbulence)
        {
         turbulence = new AircraftTurbulence;
         turbulence->lifeSpan = 30; // seconds;
         turbulence->growthRate = 5; // feet/sec
         turbulence->startRadius = af->GetAeroData(AeroDataSet::Span) / 4;
        }
        */
        if ( not lVortex)
        {
            lVortex = new AircraftTurbulence;
            lVortex->lifeSpan = 30; // seconds;
            lVortex->growthRate = 5; // feet/sec
            lVortex->startRadius = 1;
            lVortex->type = AircraftTurbulence::LVORTEX;
        }

        if ( not rVortex)
        {
            rVortex = new AircraftTurbulence;
            rVortex->lifeSpan = 30; // seconds;
            rVortex->growthRate = 5; // feet/sec
            rVortex->startRadius = 1;
            rVortex->type = AircraftTurbulence::RVORTEX;
        }
    }

    //if it's not a tanker this does nothing
    theBrain->InitBoom();

    if (Sms)
        Sms->AddWeaponGraphics();

    // easter egg: need to check af NULL since we may be non-local
    if (0)//me123 af and af->GetSimpleMode() == SIMPLE_MODE_HF )
    {
        OTWDriver.RemoveObject((DrawableBSP*)drawPointer, TRUE);
        drawPointer = NULL;
        OTWDriver.CreateVisualObject(this, MapVisId(VIS_AH64), 1.0f);
    }

    InitCountermeasures();
    InitDamageStation();

    if (this == SimDriver.GetPlayerEntity())
        OTWDriver.SetGraphicsOwnship(this);

    // Set the fin flash for F16's
    // JPO - enhance this for all models.
    // If they have more than 1 texture set, then allow then to be selected
    if (SimDriver.RunningCampaignOrTactical())
    {
        //int squad = ((FlightClass*)GetCampaignObject())->GetUnitSquadron()->GetUnitNameID();
        int squad = 0;

        if (GetCampaignObject() and ((FlightClass*)GetCampaignObject())->GetUnitSquadron())
        {
            // JB 010806 CTD
            squad = ((FlightClass*)GetCampaignObject())->GetUnitSquadron()->GetUnitNameID();
        }

        int set = 0;
        int ntexset;

        if (IsF16())   // hardwired values
        {
            switch (squad)
            {
                case 149:  // Virginia ANG
                    set = 0;
                    break;

                case 35:   // Wolfpack in blue
                    set = 1;
                    break;

                case 80:   // Wolfpack in yellow
                    set = 2;
                    break;

                case 161:  // ROK squadron
                    set = 3;
                    break;

                case 36:   // Osan squadron
                    set = 4;
                    break;

                case 194:  // Californians
                    set = 5;
                    break;

                default:
                    // This happens in Tactical engagment, dogfight and probably IA too.
                    // use to be 0, but we'll vary it a little.
                    ntexset = ((DrawableBSP*)drawPointer)->GetNTextureSet();

                    if (ntexset > 0)
                    {
                        set = squad % ntexset;
                    }
                    else
                    {
                        set = 0;
                    }

                    break;
            }
        }
        else
        {
            ntexset = ((DrawableBSP*)drawPointer)->GetNTextureSet();

            if (ntexset > 0)
            {
                set = squad % ntexset;
            }
        }

        if (set3DTexture not_eq -1 and ((FlightClass*)GetCampaignObject())->IsPlayer())
        {
            ((DrawableBSP*)drawPointer)->SetTextureSet(set3DTexture);
        }
        else
        {
            ((DrawableBSP*)drawPointer)->SetTextureSet(set);
        }
    }//me123 fix for missing canopy, pilot, nozzle ect when a flight is sleept, but not agregated and then woken

    if (IsComplex())
    {
        // F16 switches/DOFS
        if (OnGround())
        {
            ((DrawableBSP*)drawPointer)->SetSwitchMask(1, 1); // Landing Gear stuff
            ((DrawableBSP*)drawPointer)->SetSwitchMask(2, 1); //
            ((DrawableBSP*)drawPointer)->SetSwitchMask(3, 1); //
            ((DrawableBSP*)drawPointer)->SetSwitchMask(4, 1); //
            ((DrawableBSP*)drawPointer)->SetSwitchMask(14, 1); //
            ((DrawableBSP*)drawPointer)->SetSwitchMask(15, 1); //
            ((DrawableBSP*)drawPointer)->SetSwitchMask(16, 1); //
            ((DrawableBSP*)drawPointer)->SetSwitchMask(17, 1); //
            ((DrawableBSP*)drawPointer)->SetSwitchMask(18, 1); //
            ((DrawableBSP*)drawPointer)->SetSwitchMask(19, 1); //
        }

        ((DrawableBSP*)drawPointer)->SetSwitchMask(5, TRUE);//canopy me123
        ((DrawableBSP*)drawPointer)->SetSwitchMask(10, TRUE);//nozzle
    }
    else
    {
        ((DrawableBSP*)drawPointer)->SetSwitchMask(5, TRUE);//canopy me123
        ((DrawableBSP*)drawPointer)->SetSwitchMask(10, TRUE);//nozzle

        if (OnGround())
        {
            ((DrawableBSP*)drawPointer)->SetSwitchMask(2, 1); //
        }
    }

    return retval;
}

int AircraftClass::Sleep(void)
{
    int retval = 0;

    if ( not IsAwake())
    {
        return retval;
    }

    if (turbulence)
    {
        turbulence->Release();
    }

    turbulence = 0;

    if (rVortex)
    {
        rVortex->Release();
    }

    rVortex = 0;

    if (lVortex)
    {
        lVortex->Release();
    }

    lVortex = 0;

    //RV - I-Hawk - RV new trails call changes
    CleanupVortex(); //I-Hawk

    // edg: hm.  I'm not sure this is the right place to do this -- perhaps
    // this should go in the destructor.  Anyway, for multiplayer, we want
    // to make sure the explosion runs if this aircraft was killed.
    // potentially we could get removed prior to exploding.   Check that
    // here for nonlocal entities

    if ( not IsLocal() and pctStrength <= 0.0f and not IsSetFlag(SHOW_EXPLOSION))
    {
        RunExplosion();
        SetFlag(SHOW_EXPLOSION);
    }

    // Clean up any smoke trails, etc. we were running
    CleanupDamage();

    //this does nothing if it's not a tanker
    theBrain->CleanupBoom();

    // Put away any weapon graphics and let go of references
    if (Sms)
    {
        Sms->FindWeaponClass(wcNoWpn);
        Sms->FreeWeaponGraphics();
    }

    if (FCC)
    {
        FCC->ClearCurrentTarget();
    }

    if (this == SimDriver.GetPlayerEntity())
    {
        OTWDriver.SetGraphicsOwnship(NULL);
    }

    SimVehicleClass::Sleep();

    CleanupLitePool();

    CleanupCountermeasures();
    CleanupDamageStation();

#ifdef DEBUG
    // See if we still own any SimObjectType things.
    // This function could hammer them to stop memory leaks, BUT
    // that might cause evil over dereferencing problems, so for now
    // it just Asserts so we can fix the problems instead of hiding them.
    //SimObjCheckOwnership( this );
#endif
    return retval;
}

void AircraftClass::MakePlayerVehicle(void)
{
    Falcon4EntityClassType* classPtr = (Falcon4EntityClassType*)EntityType();
    int i, s;
    int hasHarm, hasLGB;
    int sensorClass;
    int sensorType;
    VuEntity* curEntity;

    // Make Live
    // sfr: using function here
    SetIsDigital(0);
    af->ClearFlag(AirframeClass::IsDigital);
    af->SetFlag(AirframeClass::AutoCommand);
    playerLastWeaponType = wtNone;
    playerLastWeaponClass = wcNoWpn;

    if (OnGround())
    {
        af->SetFlag(AirframeClass::ThrottleCheck);
        af->SetFlag(AirframeClass::EngineOff);
        af->SetFlag(AirframeClass::EngineOff2);//TJL 01/14/04 Multi-engine
        af->pwrlev = af->throtl;
        af->pwrlevEngine1 = af->engine1Throttle;//TJL 01/14/04 Multi-engine
        af->pwrlevEngine2 = af->engine2Throttle;//TJL 01/14/04 Multi-engine
    }

    // Unlimited fuel?
    if (PlayerOptions.UnlimitedFuel())
    {
        VuListIterator updateWalker(GetCampaignObject()->GetComponents());
        curEntity = updateWalker.GetFirst();

        while (curEntity)
        {
            ((AircraftClass*)curEntity)->af->SetFlag(AirframeClass::NoFuelBurn);
            curEntity = updateWalker.GetNext();
        }
    }

    // Flight model type
    if (PlayerOptions.GetFlightModelType() == FMSimplified)
    {
        af->SetFlag(AirframeClass::Simplified);
    }

    SetFlag(MOTION_OWNSHIP);

    if ( not IsLocal())
    {
        return;
    }

    targetUpdateRate = (5 * SEC_TO_MSEC);/*targetUpdateRate * ((4 - DBrain()->SkillLevel()) * 2 + 1);*/

    if (FalconLocalSession->GetPlayerEntity() not_eq this)
    {
        MonoPrint("Local but not player\n");
        return;
    }

    gPlayerExitMenuShown = FALSE;
    geomCalcRate = 0;
    targetUpdateRate = 0;
    DBrain()->SetSkill(max(2, DBrain()->SkillLevel()));
    // DBrain()->SetSkill(4);

    DBrain()->SetBvrCurrProfile(DigitalBrain::Plevel1c); // 2002-03-15 ADDED BY S.G. Player will default to pursuit so if we ask someone to engage, that's what they'll will do...

    if (curWaypoint->GetWPAction() == WP_TAKEOFF)
    {
        //JPO check for preflighting the aircraft. Doit unless we are right at the begining
        //if ( not DBrain()->IsSetATC(DigitalBrain::DonePreflight) and not DBrain()->IsAtFirstTaxipoint()) {
        //RAS-11Nov04-Fix for Ramp Start: When entering Ramp Start, jet would already be running.  Removed line
        //above and replaced with this one
        if ( not DBrain()->IsSetATC(DigitalBrain::DonePreflight))
        {
            if (PlayerOptions.GetStartFlag() not_eq PlayerOptionsClass::START_RAMP)
            {
                PreFlight();
            }
            else
            {
                // Cobra - Start with canopy open
                af->canopyState = true;

                if (TheTimeOfDay.GetLightLevel() < 0.65f)
                {
                    SetSwitch(COMP_3DPIT_INTERIOR_LIGHTS, TRUE);
                    SetStatusBit(ACSTATUS_PITLIGHT);
                }
            }
        }
    }
    else   // JPO start up anyway.
    {
        PreFlight();
    }

    // just for testing simple flight model
    // if (af->IsSet(AirframeClass::Simplified))
    // af->SetSimpleMode( SIMPLE_MODE_AF );
    //     af->SetFlag(AirframeClass::Simplified);
    // else
    //  af->SetSimpleMode( FALSE );
    if (PlayerOptions.GetFlightModelType() == FMSimplified)
    {
        // EASTER EGG: use helicopter hifi model
        if (strnicmp(LogBook.Name(), "mrsteed", 7) == 0)
        {
            af->SetSimpleMode(SIMPLE_MODE_HF);
            af->Reinit();
            af->SetFlag(AirframeClass::Simplified);
            playerFlightModelHack = TRUE;

            if (drawPointer)
                OTWDriver.RemoveObject(drawPointer, TRUE);

            drawPointer = NULL;

            switch (PRANDInt5())
            {
                case 0:
                    OTWDriver.CreateVisualObject(this, MapVisId(VIS_MD500), 1.0f);
                    break;

                case 1:
                    OTWDriver.CreateVisualObject(this, MapVisId(VIS_MI24), 1.0f);
                    break;

                case 2:
                    OTWDriver.CreateVisualObject(this, MapVisId(VIS_KA50), 1.0f);
                    break;

                default:
                    OTWDriver.CreateVisualObject(this, MapVisId(VIS_AH64), 1.0f);
                    break;
            }
        }
        else if (strnicmp(LogBook.Name(), "mrsteen", 7) == 0)
        {
            af->SetSimpleMode(SIMPLE_MODE_AF);
            af->SetFlag(AirframeClass::Simplified);
            playerFlightModelHack = TRUE;
        }
        else
        {
            af->SetSimpleMode(SIMPLE_MODE_OFF);
        }
    }
    else
    {
        af->SetSimpleMode(SIMPLE_MODE_OFF);
    }

    if (PlayerOptions.UnlimitedFuel())
        af->SetFlag(AirframeClass::NoFuelBurn);
    else
        af->ClearFlag(AirframeClass::NoFuelBurn);

    if (gNavigationSys)
    {
        gNavigationSys->SetMissionTacans(this);
    }

    // Tell the FCC we are a player
    FCC->SetPlayerFCC(TRUE);

    if (PlayerOptions.AutoTargetingOn())
        FCC->autoTarget = TRUE;
    else
        FCC->autoTarget = FALSE;

    // Set Unlimited ammo appropriatly
    //Cobra we are doing this to test for now in TE  ADD BACK IN LATER
    if (SimDriver.RunningInstantAction() or gUnlimitedAmmo > 2 or g_bUnlimitedAmmo)
        Sms->SetUnlimitedAmmo(TRUE);

    // Get rid of old sensors
    //s = numSensors; // Set sensors to 0, so we can run while we're doing this

    // OW - fixes memory leak
    if (tempSensorArray)
    {
        //when we need to change the sensors for the player we just set tempSensorArray equal
        //to sensorarray to save them for deletion at this time, and then
        for (int i = 0; i < tempNumSensors; i++)
        {
            delete tempSensorArray[i];
            tempSensorArray[i] = NULL;
        }

        delete [] tempSensorArray;
        tempSensorArray = NULL;
        tempNumSensors = 0;
    }

    // OW

    tempNumSensors = numSensors;
    numSensors = 0;
    tempSensorArray = sensorArray;
    sensorArray = NULL;

    /*for (i=0; i<s; i++)
    {
       delete sensorArray[i];
       sensorArray[i] = NULL;
    }
    delete [] sensorArray;
    sensorArray = NULL;*/

    // Should we add a HARM pod?
    hasHarm = Sms->HasHarm();

    // Should we add a Targeting pod?
    hasLGB = Sms->HasLGB();

    // Create the new sensors
    // Quick, count the sensors
    s = 0;

    for (i = 0; i < 5; i++)
    {
        if (SimACDefTable[classPtr->vehicleDataIndex].sensorType[i] > 0)
        {
            s++;
        }
    }

    sensorArray = new SensorClass*[s + hasHarm + hasLGB];

    for (i = 0; i < s; i++)
    {
        sensorClass = SimACDefTable[classPtr->vehicleDataIndex].sensorType[i];
        sensorType = SimACDefTable[classPtr->vehicleDataIndex].sensorIdx[i];

        switch (sensorClass)
        {
            case SensorClass::Radar:
                switch (PlayerOptions.GetAvionicsType())
                {
                    case ATEasy:
                        sensorArray[i] = new Radar360Class(RDR_F16_360, this);

                        // JB 011213 If awacs set the radar range
                        if (af and af->platform and GetCampaignObject() and GetCampaignObject()->GetSType() == STYPE_UNIT_AWACS)
                        {
                            ((Radar360Class*) sensorArray[i])->SetAWACSMode(true);
                            ((Radar360Class*) sensorArray[i])->SetMaxRange(RadarDataTable[af->platform->GetRadarType()].NominalRange * FT_TO_NM);
                        }

                        break;

                    case ATSimplified:
                        sensorArray[i] = new RadarSuperClass(RDR_F16_SIMPLE, this);
                        break;

                    case ATRealistic:
                    case ATRealisticAV: // M.N.
                        sensorArray[i] = new RadarDopplerClass(GetRadarType(), this);
                        // SCR 11/28/98  I don't think this is a valid thing to assert -- the sensorIdx data is old and out of date.
                        //                  F4Assert (sensorType == GetRadarType());
                        break;
                }

                break;

            case SensorClass::RWR:
                switch (PlayerOptions.GetAvionicsType())
                {
                    case ATEasy:
                        sensorArray[i] = new PlayerRwrClass(sensorType, this);
                        break;

                    case ATSimplified:
                        sensorArray[i] = new ALR56Class(sensorType, this);
                        break;

                    case ATRealistic:
                    case ATRealisticAV: // M.N.
                        sensorArray[i] = new ALR56Class(sensorType, this);
                        break;
                }

                break;

            case SensorClass::IRST:
                sensorArray[i] = new IrstClass(sensorType, this);
                break;

            case SensorClass::Visual:
                sensorArray[i] = new EyeballClass(sensorType, this);
                break;
        }
    }

    if (hasHarm)
    {
        switch (PlayerOptions.GetAvionicsType())
        {
            case ATEasy:
                sensorArray[i] = new EasyHarmTargetingPod(5, this);
                // Cobra - new EasyHarmTargetingPod() does not set the sensorType.  New function added to set sensorType.
                sensorArray[s]->SetType(SensorClass::HTS);
                break;

            case ATSimplified:
            case ATRealistic:
            case ATRealisticAV: // M.N.
                sensorArray[i] = new AdvancedHarmTargetingPod(5, this);
                // Cobra - new AdvancedHarmTargetingPod() does not set the sensorType.  New function added to set sensorType.
                sensorArray[s]->SetType(SensorClass::HTS);
                break;
        }

        s ++;
    }

    if (hasLGB)
    {
        sensorArray[s] = new LaserPodClass(5, this);
        // Cobra - new LaserPodClass() does not set the sensorType.  New function added to set sensorType.
        sensorArray[s]->SetType(SensorClass::TargetingPod);
        s ++;
    }

    numSensors = s;
    SOIManager(SimVehicleClass::SOI_RADAR);
}



void AircraftClass::ConfigurePlayerAvionics(void)
{
    ShiAssert(this == SimDriver.GetPlayerEntity());

    // Configure the avionics appropriatly
    if (SimDriver.RunningDogfight() or
        (SimDriver.RunningInstantAction() and instant_action::is_fighter_sweep()))
    {
        CPButtonObject* pButton = OTWDriver.pCockpitManager->GetButtonPointer(ICP_AA_BUTTON_ID);
        SimICPAA(ICP_AA_BUTTON_ID, KEY_DOWN, pButton);
    }
    else if (SimDriver.RunningInstantAction() and instant_action::is_moving_mud())
    {
        CPButtonObject* pButton = OTWDriver.pCockpitManager->GetButtonPointer(ICP_AG_BUTTON_ID);
        SimICPAG(ICP_AG_BUTTON_ID, KEY_DOWN, pButton);
    }
    else
    {
        CPButtonObject* pButton = OTWDriver.pCockpitManager->GetButtonPointer(ICP_NAV_BUTTON_ID);
        SimICPNav(ICP_NAV_BUTTON_ID, KEY_DOWN, pButton);
    }

    // XXX JPO good place to init switches?
    if (OTWDriver.pCockpitManager)
        OTWDriver.pCockpitManager->InitialiseInstruments();
}


void AircraftClass::MakeNonPlayerVehicle()
{
    VuEntity* curEntity;

    // if there's no pilot ( ejected ), don't set airframe to simple
    // model.  NOTE: MakeNonPlayerVehicle is called when player is
    // detached from plane after ejecting.
    if (HasPilot())
    {
        af->SetSimpleMode(SIMPLE_MODE_AF);
    }

    if ((GetCampaignObject()->GetDeagOwner() not_eq OwnerId()) and ( not IsSetFlag(OBJ_DEAD)))
    {
        //MonoPrint(
        // "AircraftClass::Change owner to deag owner %08x %08x%08x\n", this, GetCampaignObject()->GetDeagOwner()
        //);
        ChangeOwner(GetCampaignObject()->GetDeagOwner());
    }

    geomCalcRate = (int)(1.0f + 4.0f * PRANDFloatPos()) * SEC_TO_MSEC;
    targetUpdateRate = (5 * SEC_TO_MSEC);/*(int)(5.0f + 15.0f * PRANDFloatPos()) * SEC_TO_MSEC;*/

    UnSetFlag(MOTION_OWNSHIP);
    // sfr: unset player flag, this is not a player anymore
    UnSetFalcFlag(FEC_HASPLAYERS);

    if (
 not HasPilot() or
        ((DBrain()->ATCStatus() < tReqTaxi) and OnGround() and not af->IsSet(AirframeClass::OnObject))
    )
    {
        if (this == FalconLocalSession->GetPlayerEntity())
        {
            SetAutopilot(APOff);
        }
        else
        {
            SetAutopilot(CombatAP);
        }

        if ((DBrain()->ATCStatus() not_eq lLanded) and (DBrain()->ATCStatus() not_eq lTaxiOff))
        {
            // JB 010811 Prevent aircraft KIA after landing.
            SetAcStatusBits(ACSTATUS_PILOT_EJECTED);
        }
    }
    else
    {
        SetAutopilot(CombatAP);
    }

    af->SetFlag(AirframeClass::IsDigital);

    // Unlimited fuel?
    if (PlayerOptions.UnlimitedFuel())
    {
        VuListIterator updateWalker(GetCampaignObject()->GetComponents());
        curEntity = updateWalker.GetFirst();

        while (curEntity)
        {
            if (SimDriver.RunningInstantAction())
                ((AircraftClass*)curEntity)->af->SetFlag(AirframeClass::NoFuelBurn);
            else
                ((AircraftClass*)curEntity)->af->ClearFlag(AirframeClass::NoFuelBurn);

            curEntity = updateWalker.GetNext();
        }
    }

    // sfr using function here
    SetIsDigital(1);
    Sms->SetUnlimitedAmmo(FALSE);
    FCC->SetMasterMode(FireControlComputer::Missile);
    FCC->SetSubMode(FireControlComputer::Aim9);
}

void AircraftClass::MakeLocal(void)
{
    SimVehicleClass::MakeLocal();
    af->RemoteUpdate();
    DBrain()->ResetTaxiState();
    af->Reinit();
}

void AircraftClass::MakeRemote(void)
{
    SimVehicleClass::MakeRemote();
}

#ifdef _DEBUG
void AircraftClass::SetDead(int flag)
{
    if (flag)
    {
        if (isDigital)
            MonoPrint("Aircraft %d dead at %8ld\n", Id().num_, SimLibElapsedTime);
        else
            MonoPrint("Ownship %d dead at %8ld\n", Id().num_, SimLibElapsedTime);
    }

    SimVehicleClass::SetDead(flag);
}
#endif // _DEBUG

void AircraftClass::JoinFlight(void)
{
    if (theBrain)
        theBrain->JoinFlight();
}

void AircraftClass::SetLead(int flag)
{
    if (theBrain)
        theBrain->SetLead(flag);
}

void AircraftClass::SetVuPosition(void)
{
    /*--------------------*/
    /* Update shared data */
    /*--------------------*/
    MonoPrint("AC %f,%f,%f\n", af->x, af->y, af->z);

    SetPosition(af->x, af->y, af->z);
    SetDelta(af->xdot, af->ydot, af->zdot);
    SetYPR(af->psi, af->theta, af->phi);
    SetYPRDelta(af->r, af->q, af->p);
    // sfr: no need for this anymore
    //SetVt (af->vt);
    //SetKias(af->vcas);
    // 2000-11-17 MODIFIED BY S.G. SO OUR SetPowerOutput WITH ENGINE TEMP IS USED INSTEAD OF THE SimBase CLASS ONE
    SetPowerOutput(af->rpm);
    //me123 changed back   SetPowerOutput(af->rpm, af->oldp01[0]);
    // END OF ADDED SECTION
}

/* sfr: commented out
void AircraftClass::SetVt(float newvt)
{
 af->vt = newvt;
 vt = newvt;
}

void AircraftClass::SetKias(float newkias)
{
 af->vcas = newkias;
 GetKias = newkias;
}

// OW: Jackals "scold-on-bounce" fix

float AircraftClass::GetKias()
{
 return GetKias;
}*/

//void AircraftClass::Regenerate (float newx, float newy, float newz, float newyaw)
void AircraftClass::Regenerate(float, float, float, float)
{
    int wasDigital = isDigital;
    int wasPlayer = 0;
    unsigned char oldPilotSlot = 0;
    int wasLocal = IsLocal();

    // KCK KLUDGE: To handle regen messages received after a reaggregate
    if ( not GetCampaignObject() or GetCampaignObject()->IsAggregate())
        return;

    /*----------------------*/
    /* Make us whole again  */
    /*----------------------*/

    wasPlayer = IsPlayer();

    if (wasPlayer)
    {
        oldPilotSlot = pilotSlot;
        MakeNonPlayerVehicle();

        // Reset the player
        if (IsLocal())
        {
            SimDriver.SetPlayerEntity(NULL);

            //SimDriver.SetPlayerEntity(this);
            // sfr: cleanupdata will cleanup FCC so the MFDs will get destroyed data
            // this cleans them up before it happens
            for (unsigned int i = 0; i < NUM_MFDS; ++i)
            {
                MfdDisplay[i]->SetMode(MFDClass::MfdOff);
            }

            TheHud->SetOwnship(NULL);
            OTWDriver.pCockpitManager->SetOwnship(NULL);
            OTWDriver.pPadlockCPManager->SetOwnship(NULL);

            for (int i = 0; i < NUM_MFDS; i++)
            {
                MfdDisplay[i]->SetOwnship(NULL);
            }
        }
    }

    // Sleep us to start
    if (IsAwake())
    {
        Sleep();
    }

    // Clear flags
    UnSetFlag(RADAR_ON);
    UnSetFlag(ECM_ON);
    UnSetFlag(OBJ_EXPLODING);
    UnSetFlag(OBJ_DEAD);
    UnSetFlag(OBJ_FIRING_GUN);
    UnSetFlag(SHOW_EXPLOSION);
    UnSetFlag(OBJ_DYING);
    ClearAcStatusBits(ACSTATUS_PILOT_EJECTED);
    UnSetFlag(RADAR_ON);
    UnSetFlag(ON_GROUND);

    // KCK: This is a little hokie, I realize. But I didn't want to virtualize cleanup for every
    // class. Maybe I'll do it when I verify that this method works.
    CleanupData();
    InitData();

    // sfr: init will reset all flags, set them again
    SetFalcFlag(FEC_REGENERATING);

    if (wasPlayer)
    {
        SetFalcFlag(FEC_HASPLAYERS);
    }

    // Now reinit
    Init(reinitData);

    if ( not wasLocal)
    {
        MakeRemote();
    }

    // Now wake it up again
    if (GetCampaignObject()->IsAwake())
    {
        Wake();
    }

    // Reset fuel when regenerating
    af->ResetFuel();

    // now configure back
    if (wasPlayer)
    {
        MakePlayerVehicle();
        pilotSlot = oldPilotSlot;

        if (IsLocal())
        {
            SimDriver.SetPlayerEntity(this);
            // edg: trying to fix a bug with this
            TheHud->SetOwnship(this);
            OTWDriver.pCockpitManager->SetOwnship(this);
            OTWDriver.pPadlockCPManager->SetOwnship(this);

            for (int i = 0; i < NUM_MFDS; i++)
            {
                MfdDisplay[i]->SetOwnship(this);
            }

            ConfigurePlayerAvionics();
            // sfr: I hate to do this, but for some reason regeneration is putting us in orbit view
            // does anyone knows why?? BTW: I didnt add this global.
            extern bool g_bStartIn3Dpit;
            OTWDriver.SetOTWDisplayMode(
                g_bStartIn3Dpit ? OTWDriverClass::Mode3DCockpit : OTWDriverClass::Mode2DCockpit
            );
        }

        SetAutopilot(AircraftClass::APOff);
    }

    //MI fix for radar mode
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    RadarClass* theRadar = (RadarClass*)FindSensor(playerAC, SensorClass::Radar);

    if (theRadar and playerAC->FCC)
    {
        FireControlComputer *fcc = playerAC->FCC;

        if (fcc->GetMasterMode() == FireControlComputer::MissileOverride)
        {
            theRadar->SetMRMOverride();
            theRadar->ClearModeDesiredCmd();
        }
        else if (fcc->GetMasterMode() == FireControlComputer::Dogfight)
        {
            theRadar->SetSRMOverride();
            theRadar->ClearModeDesiredCmd();
        }
        else if (fcc->IsAGMasterMode())
        {
            theRadar->DefaultAGMode();
        }
        else if (fcc->IsAAMasterMode())
        {
            theRadar->DefaultAAMode();
        }
    }
}

void AircraftClass::GetTransform(TransformMatrix tMat)
{
    memcpy(tMat, dmx, sizeof(TransformMatrix));
}

float AircraftClass:: GetP(void)
{
    return (af->p);
}

float AircraftClass:: GetQ(void)
{
    return (af->q);
}

float AircraftClass:: GetR(void)
{
    return (af->r);
}

float AircraftClass:: GetAlpha(void)
{
    return (af->alpha);
}

float AircraftClass:: GetBeta(void)
{
    return (af->beta);
}

float AircraftClass:: GetNx(void)
{
    return (af->nxcgb);
}

float AircraftClass:: GetNy(void)
{
    return (af->nycgb);
}

float AircraftClass:: GetNz(void)
{
    return (af->nzcgb);
}

float AircraftClass:: GetGamma(void)
{
    return (af->gmma);
}

float AircraftClass:: GetSigma(void)
{
    return (af->sigma);
}

float AircraftClass:: GetMu(void)
{
    return (af->mu);
}

void AircraftClass::ResetFuel(void)
{
    af->ResetFuel();
}

long AircraftClass::GetTotalFuel(void)
{
    return FloatToInt32(af->Fuel() + af->ExternalFuel());
}

int AircraftClass::HasAreaJamming(void)
{
    return FALSE;
}

int AircraftClass::HasSPJamming(void)
{
    return Sms->HasSPJammer();
}
