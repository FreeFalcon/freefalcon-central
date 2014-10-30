#include "icp.h"
#include "campwp.h"
#include "phyconst.h"
#include "vu2.h"
#include "airframe.h"
#include "camplib.h"
#include "fcc.h"
#include "aircrft.h"
#include "find.h"
#include "simdrive.h"
#include "otwdrive.h"
#include "cpmanager.h"
#include "navsystem.h"
#include "fsound.h"
#include "falcsnd/voicemanager.h"
#include "weather.h"

//MI for ICP stuff
extern bool g_bRealisticAvionics;

//---------------------------------------------------------
// ICPClass::ExecCNIMode
//---------------------------------------------------------

void ICPClass::ExecCNIMode()
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if ( not playerAC) return;//me123 ctd fix

    if ( not g_bRealisticAvionics)
    {
        int type;
        static int wpflags;
        static int frame = 0;
        char timeStr[16] = "";
        char band;
        TacanList::StationSet set;
        int channel;
        VU_ID vuId;





        {

            // Clear the update flag

            mUpdateFlags and_eq not CNI_UPDATE;

            // Calculate Some Stuff
            if (playerAC->curWaypoint)
            {
                wpflags = playerAC->curWaypoint->GetWPFlags();
            }
            else
            {
                wpflags = 0;
            }

            if (wpflags bitand WPF_TARGET)
            {
                type = Way_TGT;
            }
            else if (wpflags bitand WPF_IP)
            {
                type = Way_IP;
            }
            else
            {
                type = Way_STPT;
            }

            //Original code
            // Format Line 1: Waypoint Num, Waypoint Type
            if (playerAC->FCC->GetStptMode() == FireControlComputer::FCCWaypoint)
            {
                sprintf(mpLine1, "COMM1 : %-12s STPT %-3d", (VM ? RadioStrings[VM->GetRadioFreq(0)] : "XXXX"), mWPIndex + 1);
            }
            else if (playerAC->FCC->GetStptMode() == FireControlComputer::FCCDLinkpoint)
            {
                sprintf(mpLine1, "COMM1 : %-12s LINK %-3d", (VM ? RadioStrings[VM->GetRadioFreq(0)] : "XXXX"), gNavigationSys->GetDLinkIndex() + 1);
            }
            else if (playerAC->FCC->GetStptMode() == FireControlComputer::FCCMarkpoint)
            {
                sprintf(mpLine1, "COMM1 : %-12s MARK %-3d", (VM ? RadioStrings[VM->GetRadioFreq(0)] : "XXXX"), gNavigationSys->GetMarkIndex() + 1);
            }


            // Format Line 2: Current Time
            FormatTime(vuxGameTime / 1000, timeStr); // Get game time and convert to secs
            sprintf(mpLine2, "COMM2 : %-12s %8s", (VM ? RadioStrings[VM->GetRadioFreq(1)] : "XXXX"), timeStr);

            if (mICPTertiaryMode == COMM1_MODE)
            {
                mpLine1[5] = '*';
            }
            else
            {
                mpLine2[5] = '*';

            }

            // Format Line 3: Bogus IFF info, Tacan Channel
            if (gNavigationSys)
            {
                gNavigationSys->GetTacanVUID(NavigationSystem::ICP, &vuId);

                if (vuId)
                {
                    gNavigationSys->GetTacanChannel(NavigationSystem::ICP, &channel, &set);

                    if (set == TacanList::X)
                    {
                        band = 'X';
                    }
                    else
                    {
                        band = 'Y';
                    }

                    sprintf(mpLine3, "                    T%3d%c", channel, band);
                }
                else
                {
                    sprintf(mpLine3, "");
                }
            }
        }
#if 0
        else if (frame == 9)
        {

            FormatTime(vuxGameTime * 0.001F, timeStr); // Get game time and convert to secs
            sprintf(mpLine2, "COMM2 : %-12s %8s", (VM ? RadioStrings[VM->GetRadioFreq(1)] : "XXXX"), timeStr);

            frame = 0;

            if (mICPTertiaryMode == COMM2_MODE)
            {
                mpLine2[5] = '*';
            }
        }
        else
        {

            frame++;
        }

#endif
    }
    else
    {
        char TacanC[6] = ""; //JAM 27Sep03 - From 3, prevents stack corruption?
        static int wpflags;

        if (gNavigationSys->GetControlSrc() == NavigationSystem::AUXCOMM)
        {
            CNIBackup();
            return;
        }

        //BEGIN Line1
        //COMM STUFF
        // MD -- 20040202: changed so that UHF is always printed above VHF per the real jet
        // if(WhichRadio == 0) //COMM1 is active
        // {
        if (VM)
        {
            if (VM->radiofilter[0] == rcfFromPackage)
                sprintf(tempstr, "11");
            else if (VM->radiofilter[0] == rcfProx)
                sprintf(tempstr, "12");
            else if (VM->radiofilter[0] == rcfAll)
                sprintf(tempstr, "14");
            else
                sprintf(tempstr, "%s", VM ? RadioStrings[VM->GetRadioFreq(0)] : "XXXX");
        }
        else
            sprintf(tempstr, "%s", VM ? RadioStrings[VM->GetRadioFreq(0)] : "XXXX");

        if (transmitingvoicecom1)
            FillDEDMatrix(0, 1, "UHF", 2);
        else
            FillDEDMatrix(0, 1, "UHF");

        // }
        // else //COMM2 is active
        // {
        // if(VM)
        // {
        // if(VM->radiofilter[1] == rcfFromPackage)
        // sprintf(tempstr,"From Pac");
        // else if(VM->radiofilter[1] == rcfProx)
        // sprintf(tempstr,"Prox");
        // else if(VM->radiofilter[1] == rcfAll)
        // sprintf(tempstr,"Broadc");
        // else
        // sprintf(tempstr, "%s", VM ? RadioStrings[VM->GetRadioFreq(1)] : "XXXX");
        // }
        // else
        // sprintf(tempstr, "%s", VM ? RadioStrings[VM->GetRadioFreq(1)] : "XXXX");
        // // FillDEDMatrix(0,1,"VHF"); MD -- 20031121: should show active when transmitting
        // if (transmitingvoicecom2)
        // FillDEDMatrix(0,1,"VHF",2);
        // else
        // FillDEDMatrix(0,1,"VHF");
        // }
        FillDEDMatrix(0, 6, tempstr);

        //END COMM STUFF
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

        //Fill in what this is
        if (playerAC->curWaypoint)
            wpflags = playerAC->curWaypoint->GetWPFlags();
        else
            wpflags = 0;

        if (wpflags bitand WPF_TARGET)
            sprintf(tempstr, "TGT");
        else if (wpflags bitand WPF_IP)
            sprintf(tempstr, "IP");
        else
            sprintf(tempstr, "STPT");

        FillDEDMatrix(0, 19 - strlen(tempstr), tempstr);

        //END WAYPOINT INFO
        //Up
        // MD -- 20040204: UHF should always appear in line 1, VHF in line 3
        //if(IsICPSet(ICPClass::EDIT_VHF) and WhichRadio == 1)
        // FillDEDMatrix(0,5,"\x01");
        //else
        if (IsICPSet(ICPClass::EDIT_VHF))
            FillDEDMatrix(2, 5, "\x01");
        else if (IsICPSet(ICPClass::EDIT_UHF)) // and WhichRadio == 0)
            FillDEDMatrix(0, 5, "\x01");

        //else if(IsICPSet(ICPClass::EDIT_UHF))
        // FillDEDMatrix(2,5,"\x01");
        //END LINE 1

        //BEGIN LINE 2
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

        //END LINE 2

        //BEGIN LINE 3
        // MD -- 20040202: changed so that UHF is always printed above VHF per the real jet
        // if(WhichRadio == 0) //COMM1 is active
        // {
        if (VM)
        {
            if (VM->radiofilter[1] == rcfFromPackage)
                sprintf(tempstr, "11");
            else if (VM->radiofilter[1] == rcfProx)
                sprintf(tempstr, "12");
            else if (VM->radiofilter[1] == rcfAll)
                sprintf(tempstr, "14");
            else
                sprintf(tempstr, "%s", VM ? RadioStrings[VM->GetRadioFreq(1)] : "XXXX");
        }
        else
            sprintf(tempstr, "%s", VM ? RadioStrings[VM->GetRadioFreq(1)] : "XXXX");

        if (transmitingvoicecom2)
            FillDEDMatrix(2, 1, "VHF", 2);
        else
            FillDEDMatrix(2, 1, "VHF");

        FillDEDMatrix(2, 6, tempstr);
        // }
        // else //COMM2 is active
        // {
        // if(VM)
        // {
        // if(VM->radiofilter[0] == rcfFromPackage)
        // sprintf(tempstr,"From Pac");
        // else if(VM->radiofilter[0] == rcfProx)
        // sprintf(tempstr,"Prox");
        // else if(VM->radiofilter[0] == rcfAll)
        // sprintf(tempstr,"Broadc");
        // else
        // sprintf(tempstr, "%s", VM ? RadioStrings[VM->GetRadioFreq(0)] : "XXXX");
        // }
        // else
        // sprintf(tempstr, "%s", VM ? RadioStrings[VM->GetRadioFreq(0)] : "XXXX");
        // // FillDEDMatrix(2,1, "UHF"); MD -- 20031121: should show active when transmitting
        // if (transmitingvoicecom1)
        // FillDEDMatrix(2,1,"UHF",2);
        // else
        // FillDEDMatrix(2,1,"UHF");
        // FillDEDMatrix(2,6, tempstr);
        // }
        //Get the time
        FormatTime(vuxGameTime / 1000, timeStr);
        FillDEDMatrix(2, (24 - strlen(timeStr)), timeStr);
        //END LINE 3

        //BEGIN LINE 4
        //HACK time if running
        if (running)
        {
            Difference = (vuxGameTime - Start);
            FormatTime(Difference / 1000, tempstr);
            int Lenght = strlen(tempstr);
            int j = 0;
            int k = 0;

            for (int i = 0; i < Lenght; i++)
            {
                j = i + 1;
                k = j + 1;

                if (tempstr[i] == ':')
                {
                    //now we gotta look ahead
                    if (tempstr[j] not_eq '0' or tempstr[k] not_eq '0')
                    {
                        if (tempstr[j] not_eq '0')
                        {
                            tempstr[i] = ' ';
                            break;
                        }
                        else if (tempstr[k] not_eq '0')
                        {
                            tempstr[j] = ' ';
                            tempstr[i] = ' ';
                            break;
                        }
                        else
                            break;
                    }
                    else
                        tempstr[i] = ' ';
                }
                else
                    tempstr[i] = ' ';
            }

            FillDEDMatrix(3, (24 - strlen(tempstr)), tempstr);
        }

        //END LINE 4

        //BEGIN LINE 5
        Digit1 = gNavigationSys->GetTacanChannel(NavigationSystem::ICP, 2);
        Digit2 = gNavigationSys->GetTacanChannel(NavigationSystem::ICP, 1);
        Digit3 = gNavigationSys->GetTacanChannel(NavigationSystem::ICP, 0);

        if (Digit1 == 0)
        {
            sprintf(TacanC, " %d%d", Digit2, Digit3);

            if (Digit2 == 0)
                sprintf(TacanC, "  %d", Digit3);
        }
        else
            sprintf(TacanC, "%d%d%d", Digit1, Digit2, Digit3);

        if (gNavigationSys->GetTacanBand(NavigationSystem::ICP) == TacanList::X)
            TacanBand = 'X';
        else
            TacanBand = 'Y';

        FillDEDMatrix(4, 1, "M");

        if (IsIFFSet(ICPClass::MODE_1))
            FillDEDMatrix(4, 2, "1");

        if (IsIFFSet(ICPClass::MODE_2))
            FillDEDMatrix(4, 3, "2");

        if (IsIFFSet(ICPClass::MODE_3))
            FillDEDMatrix(4, 4, "3");

        if (IsIFFSet(ICPClass::MODE_4))
            FillDEDMatrix(4, 5, "4");

        if (IsIFFSet(ICPClass::MODE_C))
            FillDEDMatrix(4, 6, "C");

        sprintf(tempstr, "%d", Mode3Code);
        FillDEDMatrix(4, 9, tempstr);
        FillDEDMatrix(4, 15, "MAN");
        sprintf(tempstr, "T%s%c", TacanC, TacanBand);
        FillDEDMatrix(4, (25 - strlen(tempstr)), tempstr);
        //END LINE 5
    }
}
