#include "campwp.h"
#include "phyconst.h"
#include "vu2.h"
#include "airframe.h"
#include "camplib.h"
#include "fcc.h"
#include "aircrft.h"
#include "icp.h"
#include "find.h"
#include "simdrive.h"

//MI for ICP stuff
extern bool g_bRealisticAvionics;
#include "hud.h"
#include "navsystem.h"

#define S_IN_M 60
#define S_IN_H 3600
#define S_IN_D 86400

char *ICPWayPtNames[NUM_WAY_TYPES] =
{
    "STPT",
    "IP",
    "TGT"
};
char *ICPWayPtActionTable[NUM_ACTION_TYPES] =
{
    "NAV",
    "TAKEOFF",
    "ASSEMBLE",
    "NAV",
    "REFUEL",
    "REARM",
    "NAV",
    "LAND",
    "NAV",
    "NAV",
    "ESCORT", // Engage engaging fighters
    "SWEEP", // Engage all enemy aircraft
    "CAP", // Patrol area for enemy aircraft
    "INTERCEPT",    // Engage specific enemy aircraft
    "CAS", // Engage enemy units at target
    "ANTISHIP", // Engage enemy shits at target
    "S&D", // Engage any enemy at target
    "STRIKE", // Destroy enemy installation at target
    "BOMB", // Strategic bomb enemy installation at target
    "SEAD", // Suppress enemy air defense at target
    "ELINT", // Electronic intellicence (AWACS, JSTAR, ECM)
    "RECON", // Photograph target location
    "RESCUE", // Rescue a pilot at location
    "ASW",
    "FUEL", // Respond to tanker requests
    "AIRDROP",
    "ECM",
    "NAV",
    "NAV",
    "NAV",
    "FAC"
};

//---------------------------------------------------------
// ICPClass::FormatTime
//---------------------------------------------------------

void ICPClass::FormatTime(long hours, char* timeStr)
{
    long minutes, secs;
    char hoursStr[3] = "";
    char minutesStr[3] = "";
    char secsStr[3] = "";

    // Format the time fields
    hours %= S_IN_D; // Lop off any time in execess of a day

    minutes = hours % S_IN_H; // generate hours column
    hours = hours  / S_IN_H;

    secs = minutes % S_IN_M; // generate secs column
    minutes = minutes / S_IN_M; // generate minutes column

    sprintf(hoursStr, "%2d", hours);

    if (hours < 10)
    {
        *hoursStr = 0x30;
    }

    sprintf(minutesStr, "%2d", minutes);

    if (minutes < 10)
    {
        *minutesStr = 0x30;
    }

    sprintf(secsStr, "%2d", secs);

    if (secs < 10)
    {
        *secsStr = 0x30;
    }

    sprintf(timeStr, "%2s:%2s:%2s", hoursStr, minutesStr, secsStr);

}
//---------------------------------------------------------
// ICPClass::ExecSTPTMode
//---------------------------------------------------------

void ICPClass::ExecSTPTMode()
{

    int altitude;
    int altFirst;
    int altSecond;
    char altFirstStr[8] = "";
    char altSecondStr[8] = "";

    static float xCurr, yCurr, zCurr;
    float xPrev, yPrev, zPrev;
    float xOwn,  yOwn;
    float vtOwn;

    VU_TIME ETA;
    VU_TIME arrive;
    VU_TIME depart;
    static int plannedSpeed;
    char timeStr[16] = "";

    char scratchStr[64] = "";

    static WayPointClass *previous;
    int type;
    static int wpflags;
    int action = WP_NOTHING;
    static int frame = 0;
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if ( not g_bRealisticAvionics)
    {
        //MI Original code
        if (playerAC and playerAC->curWaypoint)  // and not playerAC->FCC->InTransistion()
        {
            // and mUpdateFlags bitand STPT_UPDATE and not ((AircraftClass*)(mpOwnship))->FCC->waypointStepCmd) {

            // Clear the update flag

            mUpdateFlags and_eq not STPT_UPDATE;

            // Get info from the aircraft

            xOwn = playerAC->XPos();
            yOwn = playerAC->YPos();
            vtOwn = playerAC->af->vt;
            previous = playerAC->curWaypoint->GetPrevWP();
            altitude = playerAC->curWaypoint->GetWPAltitude();
            arrive = playerAC->curWaypoint->GetWPArrivalTime() / SEC_TO_MSEC;
            action = playerAC->curWaypoint->GetWPAction();
            wpflags = playerAC->curWaypoint->GetWPFlags();
            playerAC->curWaypoint->GetLocation(&xCurr, &yCurr, &zCurr);

            // Calculate Some Stuff

            ETA = SimLibElapsedTime / SEC_TO_MSEC + FloatToInt32(Distance(xOwn, yOwn, xCurr, yCurr) / vtOwn);

            if (previous)
            {
                previous->GetLocation(&xPrev, &yPrev, &zPrev);
                depart = previous->GetWPDepartureTime() / SEC_TO_MSEC;
                plannedSpeed = abs(FloatToInt32((Distance(xCurr, yCurr, xPrev, yPrev) / (arrive - depart) * FTPSEC_TO_KNOTS)));
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

            // Format Line 1: Waypoint Num, Waypoint Type

            sprintf(scratchStr, "%s %d", ICPWayPtNames[type], mWPIndex + 1);
            sprintf(mpLine1, "%-9s %s", scratchStr, ICPWayPtActionTable[action]);

            // Format Line 2: TOS, Altitude

            FormatTime(arrive, timeStr);

            altFirst = altitude / 1000;
            altSecond = altitude % 1000;

            sprintf(altFirstStr, "%d", altFirst);
            sprintf(altSecondStr, "%3d", altSecond);

            if (altSecond < 10)
            {
                altSecondStr[0] = 0x30;
                altSecondStr[1] = 0x30;
            }
            else if (altSecond < 100)
            {
                altSecondStr[0] = 0x30;
            }

            if (altitude < 1000)
            {
                sprintf(mpLine2, "TOS %8s %6sFT", timeStr, altSecondStr);
            }
            else
            {
                sprintf(mpLine2, "TOS %8s %3s,%3sFT", timeStr, altFirstStr, altSecondStr);
            }

            // Format Line 3: ETA, Planned Speed

            FormatTime(ETA, timeStr);

            if ( not previous or  wpflags bitand WPF_ALTERNATE)
            {
                sprintf(mpLine3, "", timeStr);
            }
            else if (action == WP_LAND or action == WP_TAKEOFF or plannedSpeed == 0)
            {
                sprintf(mpLine3, "ETA %8s", timeStr);
            }
            else
            {
                sprintf(mpLine3, "ETA %8s  %3dKTS", timeStr, plannedSpeed);
            }
        }

#if 0
        else if (frame == 9)
        {

            if ( not previous or  wpflags bitand WPF_ALTERNATE)
            {
                sprintf(mpLine3, "", timeStr);
            }
            else
            {

                xOwn = mpOwnship->XPos();
                yOwn = mpOwnship->YPos();
                vtOwn = mpOwnship->af->vt;

                ETA = (long)(Distance(xOwn, yOwn, xCurr, yCurr) / vtOwn);

                FormatTime(ETA, timeStr);

                if (action == WP_LAND or action == WP_TAKEOFF or plannedSpeed == 0)
                {
                    sprintf(mpLine3, "ETA %8s", timeStr);
                }
                else
                {
                    sprintf(mpLine3, "ETA %8s  %3dKTS", timeStr, plannedSpeed);
                }
            }

            frame = 0;
        }
        else
        {

            frame++;
        }

#endif
    }
    else
    {
        //Line1
        FillDEDMatrix(0, 9, "STPT");
        AddSTPT(0, 15);

        if ( not MAN)
            FillDEDMatrix(0, 21, "AUTO");
        else
            FillDEDMatrix(0, 21, "MAN");

        //Line2
        //Get the current waypoint location
        if (playerAC and playerAC->curWaypoint)
            playerAC->curWaypoint->GetLocation(&xCurr, &yCurr, &zCurr);

        latitude = (FALCON_ORIGIN_LAT * FT_PER_DEGREE + xCurr) / EARTH_RADIUS_FT;
        cosLatitude = (float)cos(latitude);
        longitude = ((FALCON_ORIGIN_LONG * DTR * EARTH_RADIUS_FT * cosLatitude) + yCurr) / (EARTH_RADIUS_FT * cosLatitude);

        latitude *= RTD;
        longitude *= RTD;

        longDeg = FloatToInt32(longitude);
        longMin = (float)fabs(longitude - longDeg) * DEG_TO_MIN;

        latDeg = FloatToInt32(latitude);
        latMin = (float)fabs(latitude - latDeg) * DEG_TO_MIN;

        // format lat/long here
        if (latMin < 10.0F)
            sprintf(latStr, "N %3d* 0%2.2f\'", latDeg, latMin);
        else
            sprintf(latStr, "N %3d* %2.2f\'", latDeg, latMin);

        if (longMin < 10.0F)
            sprintf(longStr, "E %3d* 0%2.2f\'", longDeg, longMin);
        else
            sprintf(longStr, "E %3d* %2.2f\'", longDeg, longMin);

        FillDEDMatrix(1, 6, "LAT");
        FillDEDMatrix(1, 11, latStr);
        //Line3
        FillDEDMatrix(2, 6, "LNG");
        FillDEDMatrix(2, 11, longStr);
        //Line4
        FillDEDMatrix(3, 5, "ELEV");
        sprintf(tempstr, "%4.0fFT", -zCurr);
        FillDEDMatrix(3, 11, tempstr);
        //Line5
        FillDEDMatrix(4, 6, "TOS");

        if (playerAC)
            ETA = SimLibElapsedTime / SEC_TO_MSEC + FloatToInt32(Distance(
                        playerAC->XPos(), playerAC->YPos(), xCurr, yCurr)
                    / playerAC->af->vt);

        FormatTime(ETA, timeStr);
        FillDEDMatrix(4, 11, timeStr);
    }
}

//---------------------------------------------------------
// ICPClass::PNUpdateSTPTMode
//---------------------------------------------------------

void ICPClass::PNUpdateSTPTMode(int button, int)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if ( not playerAC)
    {
        return;
    }

    if (mNumWayPts == 0)
    {
        return;
    }

    if (button == PREV_BUTTON)
    {
        ((AircraftClass*)(playerAC))->FCC->waypointStepCmd = -1;
    }
    else
    {
        ((AircraftClass*)(playerAC))->FCC->waypointStepCmd = 1;
    }

    mUpdateFlags or_eq STPT_UPDATE;
    mUpdateFlags or_eq CNI_UPDATE;
}
