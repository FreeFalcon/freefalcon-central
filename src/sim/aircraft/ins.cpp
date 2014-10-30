#include "stdhdr.h"
#include "aircrft.h"
#include "fack.h"
#include "airframe.h"
#include "otwdrive.h"
#include "cpmanager.h"
#include "phyconst.h"
#include "flightData.h"
#include "navsystem.h"

void AircraftClass::RunINS(void)
{
    if (INSAlign and INSState(INS_AlignNorm) or INSState(INS_AlignFlight))
    {
        if ((OnGround() and GetKias() <= 2.0F and not INS60kts) or
            INSState(INS_AlignFlight))
            DoINSAlign();
        else
            INSAlign = FALSE;


    }
    else if (INSState(INS_AlignNorm) and not INS60kts)
    {
        INSAlign = TRUE;
    }

    if (INSStatus <= 90)
    {
        //ADI OFF Flag goes away
        INSOn(AircraftClass::INS_ADI_OFF_IN);
        INSOn(AircraftClass::INS_HUD_STUFF);
        INSOn(AircraftClass::INS_HSI_OFF_IN);
        INSOn(AircraftClass::INS_HSD_STUFF);
    }
    else if ( not HasAligned)
    {
        //ADI OFF Flag goes away
        INSOff(AircraftClass::INS_ADI_OFF_IN);
        INSOff(AircraftClass::INS_HUD_STUFF);
        INSOff(AircraftClass::INS_HSI_OFF_IN);
        INSOff(AircraftClass::INS_HSD_STUFF);
    }

    if (INSStatus <= 79)
    {
        //ADI AUX Flag goes away
        INSOn(AircraftClass::INS_ADI_AUX_IN);
    }
    else
        INSOff(AircraftClass::INS_ADI_AUX_IN);

    if (INSStatus <= 70)
        HasAligned = TRUE;
    else
        HasAligned = FALSE;

    if (INSStatus <= 10)
        INSOn(INS_Aligned);
    else
        INSOff(INS_Aligned);

    CheckINSStatus();

    if (INSState(INS_Nav))
        CalcINSDrift();
    else
        INSLatDrift = 0.0F;

    if (GetKias() >= 60 and OnGround() and INSState(INS_AlignNorm))
        INS60kts = TRUE; //needs to be turned off

    //Check for power
    if (currentPower == PowerNone) //Emergency bus
    {
        //SwitchINSToOff();
        INSAlignmentTimer = 0.0F;
        INSAlignmentStart = vuxGameTime;
        INSAlign = FALSE;
        HasAligned = FALSE;
        INSStatus = 99;
        INSTimeDiff = 0.0F;
        INS60kts = FALSE;
        HasAligned = FALSE;
        CheckUFC = TRUE;
    }
}
void AircraftClass::DoINSAlign(void)
{
    //if they enter the coords after 2 mins of alignment, we start from the beginning
    if (CheckUFC)
    {
        if (OTWDriver.pCockpitManager and OTWDriver.pCockpitManager->mpIcp and 
            OTWDriver.pCockpitManager->mpIcp->INSEnterPush())
        {
            CheckUFC = FALSE;
            OTWDriver.pCockpitManager->mpIcp->ClearINSEnter();

            if (INSAlignmentTimer >= 120)
                INSAlignmentTimer = 0;
        }
    }

    if (INSAlignmentTimer >= 120)
        CheckUFC = TRUE;

    //Dont align if the UFC isn't powered
    if ( not HasPower(UFCPower))
        return;

    if (INSState(INS_AlignFlight))
        INSAlignmentTimer += 8 * SimLibMajorFrameTime; //reach 480 in 1 min
    else
        INSAlignmentTimer += SimLibMajorFrameTime;

    if (INSAlignmentTimer >= 12) //12 seconds
        INSStatus = 90;

    if (INSAlignmentTimer >= 60) //60 Seconds
        INSStatus = 79;

    if (INSAlignmentTimer >= 90) //90 seconds
        INSStatus = 70;

    if (INSAlignmentTimer >= 155) //every 65 seconds one step less
        INSStatus = 60;

    if (INSAlignmentTimer >= 220)
        INSStatus = 50;

    if (INSAlignmentTimer >= 285)
        INSStatus = 40;

    if (INSAlignmentTimer >= 350)
        INSStatus = 30;

    if (INSAlignmentTimer >= 415)
        INSStatus = 20;

    if (INSAlignmentTimer >= 480) //8 minutes
        INSStatus = 10;
}
void AircraftClass::SwitchINSToOff(void)
{
    //it all goes down here
    INSFlags = 0;
    INSOn(INS_PowerOff);
    INSAlignmentTimer = 0.0F;
    INSAlignmentStart = vuxGameTime;
    INSAlign = FALSE;
    HasAligned = FALSE;
    INSStatus = 99;
    INSTimeDiff = 0.0F;
    INS60kts = FALSE;
    HasAligned = FALSE;
    CheckUFC = TRUE;

    OTWDriver.pCockpitManager->mpIcp->ClearStrings();
}
void AircraftClass::SwitchINSToAlign(void)
{
    if (INSState(INS_PowerOff))
    {
        INSAlignmentTimer = 0.0F;
        HasAligned = FALSE;
    }

    INSOn(INS_AlignNorm);
    INSOff(INS_PowerOff);
    INSOff(INS_Nav);
    INSOff(INS_AlignFlight);
    INSAlignmentStart = vuxGameTime;
    INSAlign = TRUE;

    //Set the UFC
    if (OTWDriver.pCockpitManager and OTWDriver.pCockpitManager->mpIcp)
    {
        OTWDriver.pCockpitManager->mpIcp->ClearStrings();
        OTWDriver.pCockpitManager->mpIcp->LeaveCNI();
        OTWDriver.pCockpitManager->mpIcp->SetICPFlag(ICPClass::MODE_LIST);
        OTWDriver.pCockpitManager->mpIcp->SetICPSecondaryMode(23); //SIX Button, INS Page
        OTWDriver.pCockpitManager->mpIcp->INSLine = 0;
    }
}
void AircraftClass::SwitchINSToNav(void)
{
    INSOn(INS_Nav);
    INSOff(INS_AlignNorm);
    INSOff(INS_PowerOff);
    INSOff(INS_AlignFlight);
    CalcINSOffset(); //entered wrong INS coords?
    INSAlign = FALSE;
    INSAlignmentStart = vuxGameTime;
}
void AircraftClass::SwitchINSToInFLT(void)
{
    /*if(INSState(INS_PowerOff))
    {
     INSAlignmentTimer = 0.0F;
     HasAligned = FALSE;
    }*/

    if (OnGround())
        return;

    INSOn(INS_AlignFlight);
    INSOff(INS_AlignNorm);
    INSOff(INS_PowerOff);
    INSOff(INS_Nav);
    INSAlignmentStart = vuxGameTime;
    INSAlign = TRUE;
    INSAlignmentTimer = 0.0F;
    HasAligned = FALSE;
    INSStatus = 99;
    INSTimeDiff = 0.0F;

    //Set the UFC
    if (OTWDriver.pCockpitManager and OTWDriver.pCockpitManager->mpIcp)
    {
        OTWDriver.pCockpitManager->mpIcp->ClearStrings();
        OTWDriver.pCockpitManager->mpIcp->LeaveCNI();
        OTWDriver.pCockpitManager->mpIcp->SetICPFlag(ICPClass::MODE_LIST);
        OTWDriver.pCockpitManager->mpIcp->SetICPSecondaryMode(23); //SIX Button, INS Page
        OTWDriver.pCockpitManager->mpIcp->INSLine = 3;
    }
}
void AircraftClass::CheckINSStatus(void)
{
    if (INSState(AircraftClass::INS_PowerOff) or
        (INSState(AircraftClass::INS_Nav) and not HasAligned))
    {
        INSOff(AircraftClass::INS_ADI_OFF_IN);
        INSOff(AircraftClass::INS_ADI_AUX_IN);
        INSOff(AircraftClass::INS_HSI_OFF_IN);
        INSOff(AircraftClass::INS_HUD_FPM);
        INSOff(AircraftClass::INS_HUD_STUFF);
        INSOff(AircraftClass::INS_HSD_STUFF);
    }
    else if (INSState(AircraftClass::INS_Nav) and HasAligned)
    {
        INSOn(AircraftClass::INS_HUD_FPM);
        INSOn(AircraftClass::INS_HUD_STUFF);
        INSOn(AircraftClass::INS_HSD_STUFF);
    }

    if (INSState(AircraftClass::INS_PowerOff) or INSState(AircraftClass::INS_AlignNorm) or
 not HasAligned)
        INSOff(AircraftClass::INS_HUD_FPM);
    else
        INSOn(AircraftClass::INS_HUD_FPM);
}
void AircraftClass::CalcINSDrift(void)
{
    /*ok do this.
    after 6min make it drift with 0.1 nm pr hour in a randon direction.
    after 12 min - 0.2
    18 min - 0.3
    ........
    60min 1.0
    after 8min...if they take it after 2 min then add 10% to the drift speeds
    ie 6min - 0.11 nm/hour
    */
    //check for changing direction
    INSDriftDirectionTimer -= SimLibMajorFrameTime;

    if (INSDriftDirectionTimer <= 0.0F)
    {
        if (rand() % 11 > 5)
            INSDriftLatDirection = 1;
        else
            INSDriftLatDirection = -1;

        if (rand() % 11 > 5)
            INSDriftLongDirection = 1;
        else
            INSDriftLongDirection = -1;

        INSDriftDirectionTimer = 300.0F;
    }

    //difference from alignment till now
    INSTimeDiff = (float)vuxGameTime - INSAlignmentStart;
    INSTimeDiff /= 1000; //to get seconds
    INSTimeDiff /= 60; //minutes
#ifndef _DEBUG

    //no drift when GPS is powered
    if (HasPower(GPSPower))
        INSAlignmentStart = vuxGameTime;

#endif

    if (INSStatus <= 10)
    {
        //per 6 minutes 0.1NM drift -> 0.01666666666NM per minute
        INSLatDrift = 0.01666666666F * INSTimeDiff;
        //get it in feet
        INSLatDrift *= NM_TO_FT;
        INSLongDrift = INSLatDrift;

        //drift in random direction
        INSLatDrift *= INSDriftLatDirection;
        INSLongDrift *= INSDriftLongDirection;
    }
    else
    {
        //10% more drift per hour
        float Factor = 10;
        //find our alignment status and calculate how much more drift we have
        Factor = (float)(10 - ((100 - INSStatus) / 10));
        //"precise drift"
        INSLatDrift = 0.01666666666F * INSTimeDiff;
        //add the additional drift factor, in percent, 10% max
        //60 equals 100% (70-10)
        INSLatDrift += (INSLatDrift / 60) * Factor;
        //get it in feet
        INSLatDrift *= NM_TO_FT;
        INSLongDrift = INSLatDrift; //drifts the same in both directions

        //drift in random direction
        INSLatDrift *= INSDriftLatDirection;
        INSLongDrift *= INSDriftLongDirection;
    }
}
static const unsigned long const1 = 6370000;
static const float const2 = PI;
static const float const3 = const1 / 90;
void AircraftClass::CalcINSOffset(void)
{
    if (OTWDriver.pCockpitManager and OTWDriver.pCockpitManager->mpIcp)
    {
        float Curlatitude = (FALCON_ORIGIN_LAT * FT_PER_DEGREE + cockpitFlightData.x) / EARTH_RADIUS_FT;
        float CosCurlat = (float)cos(Curlatitude);
        float Curlongitude = ((FALCON_ORIGIN_LONG * DTR * EARTH_RADIUS_FT * CosCurlat) + cockpitFlightData.y) / (EARTH_RADIUS_FT * CosCurlat);

        Curlatitude *= RTD;
        Curlongitude *= RTD;

        //from our initial alignment position, to where we are now
        float DiffLat = fabs(OTWDriver.pCockpitManager->mpIcp->StartLat - Curlatitude);
        float DiffLong = fabs(OTWDriver.pCockpitManager->mpIcp->StartLong - Curlongitude);

        OTWDriver.pCockpitManager->mpIcp->INSLATDiff += DiffLat;
        OTWDriver.pCockpitManager->mpIcp->INSLONGDiff += DiffLong;

        Curlatitude = 90 - Curlatitude; //to be formula "compatible", we need the opposing value

        //find how many feet our degree is at the current lat
        //Lat is N/S and one degree is about 60 NM
        INSLatOffset = (OTWDriver.pCockpitManager->mpIcp->INSLATDiff * 60) * NM_TO_FT;

        //find how many feet a degree is in longitude, at our current latitude
        float radius = Curlatitude * (const1 / 90);
        float circumfence = 2 * const2 * radius;
        float feetperdeg = circumfence * 3.281f / 360.0f;

        //in Longitude
        INSLongOffset = OTWDriver.pCockpitManager->mpIcp->INSLONGDiff * feetperdeg;

        INSAltOffset = OTWDriver.pCockpitManager->mpIcp->INSALTDiff;

        //not used yet
        //INSHDGOffset = OTWDriver.pCockpitManager->mpIcp->INSHDGDiff;
    }
}
