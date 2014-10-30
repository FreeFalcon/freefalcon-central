#include "icp.h"
#include "simdrive.h"
#include "aircrft.h"
#include "navsystem.h"
#include "fsound.h"
#include "falcsnd/voicemanager.h"
#include "weather.h"
#include "otwdrive.h"
#include "cpmanager.h"
#include "cphsi.h"

void ICPClass::CNIBackup(void)
{
    //Line1
    // MD -- 20040204: actually the UHF entry is always on line one
    //if(WhichRadio == 0) //COMM1 is active
    FillDEDMatrix(0, 0, "UHF");

    //else //COMM2 is active
    // FillDEDMatrix(0,0,"VHF");
    //WAYPOINT INFO
    if ( not MAN) //Auto Waypoint
    {
        if (IsICPSet(ICPClass::EDIT_STPT))
            sprintf(tempstr, "\x01%2dA", mWPIndex + 1);
        else
            sprintf(tempstr, "%2dA", mWPIndex + 1);
    }
    else
    {
        if (IsICPSet(ICPClass::EDIT_STPT))
            sprintf(tempstr, "\x01%2d ", mWPIndex + 1);
        else
            sprintf(tempstr, "%2d ", mWPIndex + 1);
    }

    FillDEDMatrix(0, (24 - strlen(tempstr)), tempstr);
    FillDEDMatrix(0, 15, "STPT");
    //END WAYPOINT INFO
    //Line2, Wind
    FillDEDMatrix(1, 0, "BUP");

    if (ShowWind)
    {
        GetWind();

        if (windSpeed > 1 and windSpeed < 9)
            sprintf(tempstr, "%d*00%d", heading, (int)windSpeed);
        else if (windSpeed > 9)
            sprintf(tempstr, "%d*0%d", heading, (int)windSpeed);
        else
            sprintf(tempstr, "%d*000", heading, (int)windSpeed);

        FillDEDMatrix(1, (24 - strlen(tempstr)), tempstr);
    }

    //Line3
    // MD -- 20040204: actually the UHF entry is always on line one
    //if(WhichRadio == 0) //COMM1 is active
    FillDEDMatrix(2, 0, "VHF");
    //else //COMM2 is active
    // FillDEDMatrix(2,0, "UHF");
    //Get the time
    FormatTime(vuxGameTime / 1000, timeStr);
    FillDEDMatrix(2, (24 - strlen(timeStr)), timeStr);
    //Line4, HACK time if running
    FillDEDMatrix(3, 0, "BUP");

    if (running)
    {
        Difference = (vuxGameTime - Start);
        FormatTime(Difference / 1000, tempstr);
        FillDEDMatrix(3, (24 - strlen(tempstr)), tempstr);
    }

    //Line5
    FillDEDMatrix(4, 0, "M");
    FillDEDMatrix(4, 15, "BUP");
    FillDEDMatrix(4, 20, "T");
}
void ICPClass::UHFBackup(void)
{
    //Line1
    FillDEDMatrix(0, 10, "UHF  ON"); // MD -- 20031121: oops, fixed UFH typo ;)
    //Line3
    FillDEDMatrix(2, 11, "BACKUP");
}
void ICPClass::VHFBackup(void)
{
    //Line1
    FillDEDMatrix(0, 10, "VHF  ON");
    //Line3
    FillDEDMatrix(2, 11, "BACKUP");
}
void ICPClass::IFFBackup(void)
{
    //Line1
    FillDEDMatrix(0, 10, "IFF  ON");
    //Line3
    FillDEDMatrix(2, 11, "BACKUP");
}
void ICPClass::ILSBackup(void)
{
    VU_ID id;

    if (gNavigationSys)
        gNavigationSys->GetTacanVUID(NavigationSystem::AUXCOMM, &id);

    //Line1
    FillDEDMatrix(0, 1, "TCN ON");

    if (gNavigationSys and gNavigationSys->GetTacanBand(NavigationSystem::AUXCOMM) == TacanList::X and 
        id not_eq FalconNullId)
        FillDEDMatrix(0, 18, "ILS ON");
    else
        FillDEDMatrix(0, 18, "ILS OFF");

    //Line3
    FillDEDMatrix(2, 17, "CMD STRG", 2);
    //Line4
    FillDEDMatrix(3, 2, "BACKUP");
    Digit1 = gNavigationSys->GetTacanChannel(NavigationSystem::AUXCOMM, 2);
    Digit2 = gNavigationSys->GetTacanChannel(NavigationSystem::AUXCOMM, 1);
    Digit3 = gNavigationSys->GetTacanChannel(NavigationSystem::AUXCOMM, 0);
    TacanChannel = (Digit1 * 100 + Digit2 * 10 + Digit3);

    FakeILSFreq();

    FillDEDMatrix(3, 14, "FREQ");
    FillDEDMatrix(3, 19, Freq);
    //Line5
    HSICourse = FloatToInt32(OTWDriver.pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_CRS));
    FillDEDMatrix(4, 14, "CRS");
    sprintf(tempstr, "%d*", HSICourse);
    FillDEDMatrix(4, 18, tempstr);

}
