#include "entity.h"
#include "fcc.h"
#include "aircrft.h"
#include "classtbl.h"
#include "navsystem.h"
#include "icp.h"

//MI for ICP stuff
extern bool g_bRealisticAvionics;

void ICPClass::ExecDLINKMode(void)
{

    FalconDLinkMessage::DLinkPointType  type;
    int pointNumber;
    char ptype[5];
    char ptarget[15];
    char pthreat[15];
    char pheading[4];
    char pdistance[5];

    if ( not g_bRealisticAvionics)
    {
        if (mUpdateFlags bitand DLINK_UPDATE)
        {
            mUpdateFlags and_eq not DLINK_UPDATE;

            //MI Changed for DLINK stuff
#if 0
            sprintf(mpLine1, "OPERATIONAL");
            sprintf(mpLine2, "");
            sprintf(mpLine3, "");
#endif

            gNavigationSys->GetDataLink(&type, &pointNumber, ptype, ptarget, pthreat, pheading, pdistance);

            if (type == FalconDLinkMessage::NODLINK)
            {
                sprintf(mpLine1, "DLINK %2d", pointNumber + 1);
                sprintf(mpLine2, "NO DLINK DATA");
                sprintf(mpLine3, "");
            }
            else if ((type == FalconDLinkMessage::IP or type == FalconDLinkMessage::TGT) and *pheading and *pdistance)
            {
                sprintf(mpLine1, "DLINK %2d  %-5s", pointNumber + 1, ptype);
                sprintf(mpLine2, "PRI: %-8s  THRT: %-8s", ptarget, pthreat);
                sprintf(mpLine3, "IP-TGT %3s *  %4s NM", pheading, pdistance);
            }
            else
            {
                sprintf(mpLine1, "DLINK %2d  %-5s", pointNumber + 1, ptype);
                sprintf(mpLine2, "PRI: %-8s  THRT: %-8s", ptarget, pthreat);
                sprintf(mpLine3, "");
            }
        }
    }
    else
    {
        if (gNavigationSys)
            gNavigationSys->GetDataLink(&type, &pointNumber, ptype, ptarget, pthreat, pheading, pdistance);

        if (type == FalconDLinkMessage::NODLINK or type == NULL)
        {
            sprintf(tempstr, "DLINK %2d", pointNumber + 1, ptype);
            FillDEDMatrix(0, 10, tempstr);
            FillDEDMatrix(1, 5, "NO DLINK DATA");
        }
        else if ((type == FalconDLinkMessage::IP or type == FalconDLinkMessage::TGT) and *pheading and *pdistance)
        {
            sprintf(tempstr, "DLINK %2d", pointNumber + 1, ptype);
            FillDEDMatrix(0, 10, tempstr);
            sprintf(tempstr, "PRI %-8s THR %-8s", ptarget, pthreat);
            FillDEDMatrix(1, ((25 - strlen(tempstr)) / 2), tempstr);
            sprintf(tempstr, "IP-TGT %3s* %4sNM", pheading, pdistance);
            FillDEDMatrix(2, ((25 - strlen(tempstr)) / 2), tempstr);
        }
        else
        {
            sprintf(tempstr, "DLINK %2d  %-5s", pointNumber + 1, ptype);
            FillDEDMatrix(0, 10, tempstr);
            sprintf(tempstr, "PRI %-8s  THR %-8s", ptarget, pthreat);
            FillDEDMatrix(2, ((25 - strlen(tempstr)) / 2), tempstr);
        }
    }
}


void ICPClass::PNUpdateDLINKMode(int button, int mode)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (button == PREV_BUTTON)
    {
        playerAC->FCC->waypointStepCmd = -1;
    }
    else if (button == NEXT_BUTTON)
    {
        playerAC->FCC->waypointStepCmd = 1;
    }

    mUpdateFlags or_eq DLINK_UPDATE;
    mUpdateFlags or_eq CNI_UPDATE;
}
