#include "stdhdr.h"
#include "digi.h"
#include "simveh.h"
#include "aircrft.h"
#include "airframe.h"
#include "campbase.h"
#include "radar.h"
#include "Find.h" // 2002-03-11 MN
#include "Falclib/include/MsgInc/RadioChatterMsg.h" // MN
#include "Flight.h" // MN

#define SHOW_MANEUVERLABELS
#ifdef SHOW_MANEUVERLABELS
#include "Graphics/include/drawbsp.h"
#include "SimDrive.h"
extern int g_nShowDebugLabels;
extern int g_nAirbaseCheck;
extern float g_fBingoReturnDistance;

static const char *ATCModes[] =
{
    "noATC",

    "lReqClearance",
    "lReqEmerClearance",
    "lIngressing",
    "lTakingPosition",
    "lAborted",
    "lEmerHold",
    "lHolding",
    "lFirstLeg",
    "lToBase",
    "lToFinal",
    "lOnFinal",
    "lClearToLand",
    "lLanded",
    "TaxiOff",
    "lEmergencyToBase",
    "lEmergencyToFinal",
    "lEmergencyOnFinal",
    "lCrashed",

    "tReqTaxi",
    "tReqTakeoff",
    "tEmerStop",
    "tTaxi",
    "tWait",
    "tHoldShort",
    "tPrepToTakeRunway",
    "tTakeRunway",
    "tTakeoff",
    "tFlyOut",
    "tTaxiBack",
};
static const int MAXATCSTATUS = sizeof(ATCModes) / sizeof(ATCModes[0]);
#endif


void DigitalBrain::Actions(void)
{
    float cur;

    RadarClass* theRadar = (RadarClass*) FindSensor(self, SensorClass::Radar);

#if 0 // test if each AGmission has a target waypoint

    if (missionClass == AGMission)
    {
        Flight flight = (Flight)self->GetCampaignObject();

        if (flight)
        {
            WayPoint w = flight->GetFirstUnitWP();
            bool found = false;

            while (w and not found)
            {
                if (w->GetWPFlags() bitand WPF_TARGET)
                    found = true;

                w = w->GetNextWP();
            }

            if ( not found)
            {
                char message[250];
                sprintf(message, "Mission: %d doesn't have WPF_TARGET", flight->GetUnitMission());
                F4Assert( not message);
            }
        }
    }

#endif

    AiMonitorTargets();
#ifdef SHOW_MANEUVERLABELS
    char label[100];//cobra change from 40
#endif

    // handle threat above all else
    if (threatPtr and curMode not_eq TakeoffMode)
    {
        if (curMode == MissileDefeatMode)
        {
            MissileDefeat();
#ifdef SHOW_MANEUVERLABELS
            sprintf(label, "ThreatMissileDefeatMode");
#endif
        }
        else
        {
            HandleThreat();
#ifdef SHOW_MANEUVERLABELS
            sprintf(label, "HandleThreatMode");
#endif
        }

        //tell atc we're going to ignore him
        if (atcstatus not_eq noATC)
        {
            ShiAssert(atcstatus < tReqTaxi);
            SendATCMsg(noATC);
            ResetATC();
        }
    }
    /*-----------------------------------------------*/
    /* stick/throttle commands based on current mode */
    /*-----------------------------------------------*/
    else
    {
        // Mode radar appropriately
        if (curMode not_eq WaypointMode and theRadar and theRadar->IsAG())
        {
            theRadar->SetMode(RadarClass::AA);
        }

        switch (curMode)
        {
            case FollowOrdersMode:

                // edg double check groundAvoidNeeded if set -- could be stuck there
                if (groundAvoidNeeded)
                    GroundCheck();

                AiPerformManeuver();
#ifdef SHOW_MANEUVERLABELS
                sprintf(label, "FollowOrdersMode");
#endif

                break;

            case WingyMode:

                // edg double check groundAvoidNeeded if set -- could be stuck there
                if (groundAvoidNeeded)
                    GroundCheck();

                AiFollowLead();
#ifdef SHOW_MANEUVERLABELS
                sprintf(label, "WingmanMode");
#endif
                break;

            case RefuelingMode:

                // edg double check groundAvoidNeeded if set -- could be stuck there
                if (groundAvoidNeeded)
                    GroundCheck();

                AiRefuel();
#ifdef SHOW_MANEUVERLABELS
                sprintf(label, "RefuelingMode");
#endif
                break;

                /*------------------*/
                /* follow waypoints */
                /*------------------*/
                // 2002-03-11 MN break up RTBMode and WaypointMode.
                // RTBCheck: If we have said Fumes, check for closest airbase and head there instead towards home base...
                // If not, continue to follow waypoints.
            case RTBMode:
                sprintf(label, "RTBMode");
                FollowWaypoints();
                break;

            case WaypointMode:
                sprintf(label, "WaypointMode");
                FollowWaypoints();
                break;

            case LandingMode:
                Land();
#ifdef SHOW_MANEUVERLABELS
                sprintf(label, "Landing %s",
                        atcstatus >= 0 and atcstatus < MAXATCSTATUS ? ATCModes[atcstatus] : "");
#endif
                break;

            case TakeoffMode:
                TakeOff();
#ifdef SHOW_MANEUVERLABELS
                sprintf(label, "TakeOff %s",
                        atcstatus >= 0 and atcstatus < MAXATCSTATUS ? ATCModes[atcstatus] : "");
#endif
                break;

                /*------------*/
                /* BVR engage */
                /*------------*/
            case BVREngageMode:
                BvrEngage();
#ifdef SHOW_MANEUVERLABELS
                sprintf(label, "BVREngagemode");
#endif
                break;

                /*------------*/
                /* WVR engage */
                /*------------*/
            case WVREngageMode:
                WvrEngage();
#ifdef SHOW_MANEUVERLABELS
                sprintf(label, "WVREngagemode");
#endif
                break;

            case GunsEngageMode:
                GunsEngage();
#ifdef SHOW_MANEUVERLABELS
                sprintf(label, "GunsEngageMode");
#endif
                break;

            case MergeMode:
                MergeManeuver();
#ifdef SHOW_MANEUVERLABELS
                sprintf(label, "MergeMode");
#endif
                break;

                /*-----------------------------------------*/
                /* Inside missile range, try to line it up */
                /*-----------------------------------------*/
            case MissileEngageMode:
                MissileEngage();
#ifdef SHOW_MANEUVERLABELS
                sprintf(label, "MissileEngageMode");
#endif
                break;

                /*----------------*/
                /* missile defeat */
                /*----------------*/
            case MissileDefeatMode:
                MissileDefeat();
#ifdef SHOW_MANEUVERLABELS
                sprintf(label, "MissileDefeatMode");
#endif
                break;

                /*-----------*/
                /* guns jink */
                /*-----------*/
            case GunsJinkMode:
                GunsJink();
#ifdef SHOW_MANEUVERLABELS
                sprintf(label, "GunsJinkMode");
#endif
                break;

                /*--------*/
                /* loiter */
                /*--------*/
            case LoiterMode:
                Loiter();
#ifdef SHOW_MANEUVERLABELS
                sprintf(label, "LoiterMode");
#endif
                break;

                /*-----------------*/
                /* collision avoid */
                /*-----------------*/
            case CollisionAvoidMode:
                CollisionAvoid();
#ifdef SHOW_MANEUVERLABELS
                sprintf(label, "CollisionAvoidMode");
#endif
                break;

                /*----------*/
                /* Separate */
                /*----------*/
            case SeparateMode:
            case BugoutMode:
                if (lastMode not_eq BugoutMode and lastMode not_eq SeparateMode and targetPtr)
                {
                    // 2001-10-28 CHANGED BACK M.N. holdAlt is used in AltitudeHold, which needs a positive value
                    // altitude error is calculated as holdAlt + self->ZPos() there 
                    //    holdAlt = min (-5000.0F, self->ZPos());
                    //TJL 11/08/03 Hold position altitude but bug out
                    //holdAlt = min (5000.0F, -self->ZPos());
                    holdAlt = -self->ZPos();

                    // Find a heading directly away from the target
                    cur = TargetAz(self, targetPtr);

                    if (cur > 0.0F)
                        cur = self->Yaw() - (180.0F * DTR - cur);
                    else
                        cur = self->Yaw() - (-180.0F * DTR - cur);

                    // Normalize
                    if (cur >  180.0F * DTR)
                        cur -= 360.0F * DTR;

                    holdPsi = cur;
                }

                WvrBugOut();
#ifdef SHOW_MANEUVERLABELS
                sprintf(label, "Separate/BugoutMode");
#endif
                break;

                /*-------------------*/
                /* Roll Out of Plane */
                /*-------------------*/
            case RoopMode:
                RollOutOfPlane();
#ifdef SHOW_MANEUVERLABELS
                sprintf(label, "RollOutOfPlaneMode");
#endif
                break;

                /*-----------*/
                /* Over Bank */
                /*-----------*/
            case OverBMode:
                OverBank(30.0F * DTR);
#ifdef SHOW_MANEUVERLABELS
                sprintf(label, "OverBankMode");
#endif
                break;

            case AccelMode:
                AccelManeuver();
#ifdef SHOW_MANEUVERLABELS
                sprintf(label, "AccelerateMode");
#endif
                break;

            default:
                SimLibPrintError("%s digi.w: Invalid digi mode %d\n",
                                 self->Id().num_, curMode);
                FollowWaypoints();
#ifdef SHOW_MANEUVERLABELS
                sprintf(label, "InvalidDigiMode");
#endif
                break;

        } /*switch*/
    }

#ifdef SHOW_MANEUVERLABELS

    if (g_nShowDebugLabels bitand 0x01)
    {
        char element[40];
        sprintf(element, " %d%d W%d R%d V%d", isWing + 1, SkillLevel(), detRWR, detRAD, detVIS);
        strcat(label, element);

        if (g_nShowDebugLabels bitand 0x40)
        {
            RadarClass* theRadar = (RadarClass*)FindSensor(self, SensorClass::Radar);

            if (theRadar)
            {
                if (theRadar->digiRadarMode = RadarClass::DigiSTT)
                    strcat(label, " STT");
                else if (theRadar->digiRadarMode = RadarClass::DigiSAM)
                    strcat(label, " SAM");
                else if (theRadar->digiRadarMode = RadarClass::DigiTWS)
                    strcat(label, " TWS");
                else if (theRadar->digiRadarMode = RadarClass::DigiRWS)
                    strcat(label, " RWS");
                else if (theRadar->digiRadarMode = RadarClass::DigiOFF)
                    strcat(label, " OFF");
                else strcat(label, " UNKNOWN");
            }
        }

        if (groundAvoidNeeded or pullupTimer)
            strcat(label, " Pullup");

        if (g_nShowDebugLabels bitand 0x8000)
        {
            if (((AircraftClass*) self)->af->GetSimpleMode())
                strcat(label, " SIMP");
            else
                strcat(label, " COMP");
        }

        if (g_nShowDebugLabels bitand 0x1000)
        {
            if (SimDriver.GetPlayerEntity() and self->GetCampaignObject() and self->GetCampaignObject()->GetIdentified(SimDriver.GetPlayerEntity()->GetTeam()))
                strcat(label, "IDed");
            else
                strcat(label, "Not IDed");
        }

        if (self->drawPointer)
            ((DrawableBSP*)self->drawPointer)->SetLabel(label, ((DrawableBSP*)self->drawPointer)->LabelColor());
    }

    if (g_nShowDebugLabels bitand 0x200000)
    {
        sprintf(label, "%.0f %.0f %.0f", trackX, trackY, trackZ);

        if (self->drawPointer)
            ((DrawableBSP*)self->drawPointer)->SetLabel(label, ((DrawableBSP*)self->drawPointer)->LabelColor());
    }

    if (g_nShowDebugLabels bitand 0x100000)
    {
        float yaw = self->Yaw();

        if (yaw < 0.0f)
            yaw += 360.0F * DTR;

        yaw *= RTD;
        sprintf(label, "%4.0f %3.0f %6.2f %3.3f", self->af->vcas, yaw, -self->ZPos(), self->pctStrength * 100.0F);

        if (self->drawPointer)
            ((DrawableBSP*)self->drawPointer)->SetLabel(label, ((DrawableBSP*)self->drawPointer)->LabelColor());
    }

    if (g_nShowDebugLabels bitand 0x800000)
    {
        sprintf(label, "0x%x leader: 0x%x", self, flightLead);

        if (self->drawPointer)
            ((DrawableBSP*)self->drawPointer)->SetLabel(label, ((DrawableBSP*)self->drawPointer)->LabelColor());
    }

#endif

    // have we got a surprise for you
    if ((targetPtr or threatPtr) and IsSetATC(HasTrainable) and self->HasPilot())
    {
        TrainableGunsEngage();
    }

    if (curMode > WVREngageMode)
        engagementTimer = 0;

    // if(isWing) {
    // PrtMode();
    // }

    /*-----------------------------------------------------------------*/
    /* Ground avoid now runs concurrently with whatever mode has been  */
    /* selected by the conflict resolver. Ground avoid modifies the    */
    /* current Pstick and Rstick commands.                             */
    /*-----------------------------------------------------------------*/

    // 2002-02-24 MN added pullupTimer - continue last calculated pullup for at least g_nPullupTime seconds
    if (groundAvoidNeeded or pullupTimer)
    {

        // 2001-10-21 MODIFIED by M.N.
        /* if(((AircraftClass*) self)->af->GetSimpleMode()) {
         SimplePullUp();
         }
         else { */

        GroundCheck();  // Cobra - make sure max elev data is current
        PullUp(); // let's forget SimplePullUp(); They just set pstick of 0.1f, which is WAY
        // too little, the AI must fly into the hills. No more low-alt crashes with PullUp();
    }
    else
    {
        ResetMaxRoll();
        SetMaxRollDelta(100.0F);
    }
}

void DigitalBrain::AirbaseCheck()
{
    bool nearestAirbase = false;
    bool returnHomebase = false;

    if (self->af->Fuel() <= 0.0F) // just float down and try to land - gear is out in that case
        return;

    GridIndex x, y;
    float dist;
    vector pos;
    Objective obj = NULL;

    if (airbasediverted == 1)
    {
        AddMode(RTBMode);
    }
    else if (airbasediverted == 2)
    {
        AddMode(LandingMode);
        return;
    }

    // Only check every g_nAirbaseCheck seconds
    if (nextFuelCheck > SimLibElapsedTime)
        return;

    nextFuelCheck = SimLibElapsedTime + g_nAirbaseCheck * CampaignSeconds;

    // when on Bingo, check distance to closest airbase when not having a target and not being threatened
    // return if distance is greater than g_fBingoReturnDistance
    if (IsSetATC(SaidBingo) and not IsSetATC(SaidFumes) and not targetPtr and not threatPtr and not airbasediverted)
    {
        pos.x = self->XPos();
        pos.y = self->YPos();

        ConvertSimToGrid(&pos, &x, &y);
        obj = FindNearestFriendlyAirbase(self->GetTeam(), x, y);

        if (obj)
        {
            dist = Distance(self->XPos(), self->YPos(), obj->XPos(), obj->YPos());

            if (dist > self->af->GetBingoReturnDistance() * NM_TO_FT)
            {
                returnHomebase = true;
                airbasediverted = 1;
            }
        }
    }

    // 2002-03-13 modified by MN works together with checks in Separate.cpp, if 49.9% damage, head to hearest airbase instead of home base
    if (IsSetATC(SaidFumes) or self->pctStrength < 0.50f) // when on fumes, force RTB to closest airbase
    {
        nearestAirbase = true;
        airbasediverted = 2;
    }

    char label[32];

    if (returnHomebase)
    {
        if ( not (moreFlags bitand SaidImADot))
        {
            moreFlags or_eq SaidImADot;
            int flightIdx = self->GetCampaignObject()->GetComponentIndex(self);
            FalconRadioChatterMessage* radioMessage = new FalconRadioChatterMessage(self->Id(), FalconLocalSession);
            radioMessage->dataBlock.from = self->Id();
            radioMessage->dataBlock.to = MESSAGE_FOR_FLIGHT;
            radioMessage->dataBlock.voice_id = ((Flight)(self->GetCampaignObject()))->GetPilotVoiceID(self->vehicleInUnit);
            radioMessage->dataBlock.message = rcIMADOT;
            radioMessage->dataBlock.edata[0] = (((FlightClass*)self->GetCampaignObject())->callsign_num);
            radioMessage->dataBlock.time_to_play = 500; // 0.5 seconds
            FalconSendMessage(radioMessage, FALSE);
        }

        AddMode(RTBMode); // changed from LandingMode to RTBMode

        if (g_nShowDebugLabels bitand 0x40000)
        {
            sprintf(label, "RTB Homebase");

            if (self->drawPointer)
                ((DrawableBSP*)self->drawPointer)->SetLabel(label, ((DrawableBSP*)self->drawPointer)->LabelColor());
        }

    }

    if (nearestAirbase)
    {
        pos.x = self->XPos();
        pos.y = self->YPos();
        ConvertSimToGrid(&pos, &x, &y);

        // 2002-04-02 ADDED BY S.G. Since it's not done above anymore, do it here only if not done within 'SaidBingo' if statement above
        if ( not obj)
            obj = FindNearestFriendlyAirbase(self->GetTeam(), x, y);

        // END OF ADDED SECTION 2002-04-02

        // change home base, of course only if it is another than our current one...
        if (obj and obj->Id() not_eq airbase)
        {
            airbase = obj->Id();
            moreFlags or_eq NewHomebase; // set this so that ResetATC doesn't reset our new airbase
            int flightIdx = self->GetCampaignObject()->GetComponentIndex(self);
            FalconRadioChatterMessage* radioMessage = new FalconRadioChatterMessage(self->Id(), FalconLocalSession);
            radioMessage->dataBlock.from = self->Id();
            radioMessage->dataBlock.to = MESSAGE_FOR_FLIGHT;
            radioMessage->dataBlock.voice_id = ((Flight)(self->GetCampaignObject()))->GetPilotVoiceID(self->vehicleInUnit);
            radioMessage->dataBlock.message = rcALTLANDING;
            radioMessage->dataBlock.edata[0] = ((FlightClass*)self->GetCampaignObject())->callsign_id;
            radioMessage->dataBlock.edata[1] = (((FlightClass*)self->GetCampaignObject())->callsign_num - 1) * 4 + flightIdx + 1;
            radioMessage->dataBlock.time_to_play = 500; // 0.5 seconds
            FalconSendMessage(radioMessage, FALSE);
            mpActionFlags[AI_FOLLOW_FORMATION] = FALSE;
        }

        AddMode(LandingMode);

        if (g_nShowDebugLabels bitand 0x40000)
        {
            sprintf(label, "RTB near ab, pct-Strgth: %1.2f", self->pctStrength);

            if (self->drawPointer)
                ((DrawableBSP*)self->drawPointer)->SetLabel(label, ((DrawableBSP*)self->drawPointer)->LabelColor());
        }
    }
}
