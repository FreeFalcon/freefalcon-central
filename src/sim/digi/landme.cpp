#include <float.h>
#include "stdhdr.h"
#include "digi.h"
#include "simdrive.h"
#include "simveh.h"
#include "Find.h"
#include "aircrft.h"
#include "atcbrain.h"
#include "simfeat.h"
#include "MsgInc/RadioChatterMsg.h"
#include "MsgInc/ATCMsg.h"
#include "falcsnd/conv.h"
#include "falcsess.h"
#include "airframe.h"
#include "ptdata.h"
#include "airunit.h"
#include "otwdrive.h"
#include "camplist.h"
#include "Graphics/Include/drawBSP.h"
#include "simmath.h"
#include "phyconst.h"
#include "classtbl.h"
#include "Graphics/Include/tmap.h"
#include "Team.h"
#include "playerop.h"
#include "radar.h"
#include "sms.h"
#include "cpmanager.h"
#include "hud.h"
#include "Graphics/Include/tod.h"
#include "Fakerand.h"
#include "wingorder.h"

extern int gameCompressionRatio;
extern int g_nReagTimer;
extern int g_nShowDebugLabels;
extern float g_fTaxiEarly; //RAS - 10Oct04 - Change early taxi time
extern int g_nTaxiLaunchTime; //RAS - 14Oct04 - need Boosters value to do quick preflight earlier if
//user decides to start taxi earlier than 3 min (default quickpreflight)

#define MoveAlong 3.0f  // 05JAN04 - FRB - was 5 secs to travel between taxi points 
// RAS - 13Oct04 - changed from 2.0 to 3.0 to help make T/O times
#define HurryUp 1.0f  // 05JAN04 - FRB - was 3 secs to travel between taxi points

/** sfr: test for parking bug fix. This will only clear the parking spot in 2 situations:
* when we choose a new one or when we regroup
*/

#if 0
// sfr: debug to file stuff
#ifdef AI_MESSAGE
#undef AI_MESSAGE
#endif
#include <iostream>
#include <fstream>
using namespace std;
namespace
{
    string last("");
#define AI_MESSAGE(slot, str)\
do {\
 if (self->vehicleInUnit == 2){\
 string cur(str);\
 if (cur not_eq last){\
 logFile << str << endl; \
 last = cur;\
 }\
 }\
} while (0)
    ofstream logFile("c:\\temp\\log.txt");
}
#endif

void DigitalBrain::ResetATC()
{
    SetATCStatus(noATC);

    if ( not (moreFlags bitand NewHomebase))
    {
        // we set a new airbase to head to (for example because of fumes fuel -> Actions.cpp)
        airbase = self->HomeAirbase();
    }

    rwIndex = 0;
    rwtime = 0;
    waittimer = 0;
    SetTaxiPoint(0);
    desiredSpeed = 0;
    turnDist = 0;
}

void DigitalBrain::ResetTaxiState(void)
{
    // sfr: this is causing a big mess,
    // so Im renaming rwindex to rwLocalIndex (avoid confusion with member variable)
    // I just renamed here and the usages, nothing else... this code is confusing
    int takeoffpt, runwaypt, rwLocalIndex;
    float x1, y1, x2, y2;
    float dx, dy, relx;
    //float deltaHdg;
    //ObjectiveClass *Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);

    // sfr: fixing xy order
    GridIndex gx, gy;
    //gx = SimToGrid(self->YPos());
    //gy = SimToGrid(self->XPos());
    ::vector pos = { self->XPos(), self->YPos()};
    ConvertSimToGrid(&pos, &gx, &gy);
    Objective Airbase = FindNearbyAirbase(gx, gy);

    if ( not self->OnGround() or not Airbase)
    {
        if (atcstatus >= tReqTaxi)
        {
            ResetATC();
        }

        return;
    }

    // sfr: no runway index means it doesnt have a runway yet
    // get one now
    ATCBrain *atcb = Airbase->brain;

    if (atcb == NULL)
    {
        return;
    }

    if (rwIndex == 0)
    {
        UnitClass *u = static_cast<UnitClass*>(self->GetCampaignObject());
        WayPoint w = u->GetCurrentUnitWP();
        int action = w->GetWPAction();

        if (action == WP_TAKEOFF)
        {
            rwIndex = atcb->FindBestTakeoffRunway(TRUE);
        }
        else if (action == WP_LAND)
        {
            rwIndex = atcb->FindBestLandingRunway(self, TRUE);
        }
    }

    // this should never happen, but it does
    if (rwIndex == 0)
    {
        return;
    }

    float dist, closestDist =  4000000.0F;
    int taxiPoint, closest = curTaxiPoint;


    taxiPoint = GetFirstPt(rwIndex) + 1;

    while (taxiPoint)
    {
        TranslatePointData(Airbase, taxiPoint, &x1, &y1);
        dx = x1 - af->x;
        dy = y1 - af->y;
        dist = dx * dx + dy * dy;

        if (dist < closestDist)
        {
            closestDist = dist;
            closest = taxiPoint;
        }

        taxiPoint = GetNextPt(taxiPoint);
    }

    if (closest not_eq curTaxiPoint)
    {
        SetTaxiPoint(closest);
        self->spawnpoint = closest;
        float tx, ty;
        TranslatePointData(Airbase, curTaxiPoint, &tx, &ty);
        SetTrackPoint(tx, ty);
    }

    if (self->AutopilotType() == AircraftClass::APOff)
    {
        return;
    }

    if ((atcstatus == tTakeRunway or atcstatus == tTakeoff) and 
        (PtDataTable[curTaxiPoint].type == TakeoffPt or PtDataTable[curTaxiPoint].type == RunwayPt))
    {
        rwLocalIndex = Airbase->brain->IsOnRunway(self);

        takeoffpt =
            Airbase->brain->FindTakeoffPt(
                (Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &x1, &y1
            );
        runwaypt =
            Airbase->brain->FindRunwayPt(
                (Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &x2, &y2
            );

        float cosAngle = self->platformAngles.sinsig * PtHeaderDataTable[rwIndex].sinHeading +
                            self->platformAngles.cossig * PtHeaderDataTable[rwIndex].cosHeading;

        runwayStatsStruct *runwayStats = Airbase->brain->GetRunwayStats();
        dx = runwayStats[PtHeaderDataTable[rwIndex].runwayNum].centerX - self->XPos();
        dy = runwayStats[PtHeaderDataTable[rwIndex].runwayNum].centerY - self->YPos();

        relx = PtHeaderDataTable[rwIndex].cosHeading * dx + PtHeaderDataTable[rwIndex].sinHeading * dy;

        if (
            cosAngle > 0.99619F and 
            PtHeaderDataTable[rwIndex].runwayNum == PtHeaderDataTable[rwLocalIndex].runwayNum and 
            runwayStats[PtHeaderDataTable[rwIndex].runwayNum].halfheight + relx > 3000.0F
        )
        {
            SetTrackPoint(x2, y2);
            SetTaxiPoint(runwaypt);
        }
        else if (relx > 0.0F)
        {
            SetTrackPoint(
                x1 - relx * PtHeaderDataTable[rwIndex].cosHeading,
                y1 - relx * PtHeaderDataTable[rwIndex].sinHeading
            );
            SetTaxiPoint(takeoffpt);
        }
        else
        {
            SetTrackPoint(x1, y1);
            SetTaxiPoint(takeoffpt);
        }

        atcstatus = tTakeRunway;
    }
    else
    {
        if (PtDataTable[curTaxiPoint].type == TakeoffPt or PtDataTable[curTaxiPoint].type == RunwayPt)
        {
            float tx, ty;
            takeoffpt = Airbase->brain->FindTakeoffPt(
                            (Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &tx, &ty
                        );
            SetTrackPoint(tx, ty);
        }
        else
        {
            TranslatePointData(Airbase, curTaxiPoint, &x1, &y1);
            dx = x1 - af->x;
            dy = y1 - af->y;
            relx = self->platformAngles.cospsi * dx + self->platformAngles.sinpsi * dy;

            if (relx < 0.0F)
            {
                ChooseNextPoint(Airbase);
            }
            else
            {
                SetTrackPoint(x1, y1);
            }
        }
    }

    // Make sure ground weapon list is up to date
    SelectGroundWeapon();
}

float DigitalBrain::CalculateNextTurnDistance(void)
{
    float curHeading = 0.0F, newHeading = 0.0F, cosAngle = 1.0F, deltaHdg = 0.0F;
    float baseX = 0.0F, baseY = 0.0F, finalX = 0.0F, finalY = 0.0F, dx = 0.0F, dy = 0.0F, vt = 0.0F;
    ObjectiveClass *Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);

    turnDist = 500.0F;

    if (Airbase and Airbase->brain)
    {
        // sfr: get a rwindex if we still dont have one
        if (rwIndex == 0)
        {
            rwIndex = Airbase->brain->FindBestLandingRunway(self);
        }

        //vt = sqrt(self->XDelta() * self->XDelta() + self->YDelta() * self->YDelta());
        vt = af->MinVcas() * KNOTS_TO_FTPSEC;

        cosAngle = Airbase->brain->DetermineAngle(self, rwIndex, atcstatus);

        switch (atcstatus)
        {
            case lFirstLeg:
                dx = self->XPos() - trackX;
                dy = self->YPos() - trackY;
                curHeading = (float)atan2(dy, dx);

                if (curHeading < 0.0F)
                    curHeading += PI * 2.0F;

                Airbase->brain->FindFinalPt(self, rwIndex, &finalX, &finalY);

                if (cosAngle < 0.0F)
                {
                    Airbase->brain->FindBasePt(self, rwIndex, finalX, finalY, &baseX, &baseY);
                    dx = trackX - baseX;
                    dy = trackY - baseY;
                }
                else
                {
                    dx = trackX - finalX;
                    dy = trackY - finalY;
                }

                newHeading = (float)atan2(dy, dx);

                if (newHeading < 0.0F)
                    newHeading += PI * 2.0F;

                deltaHdg = newHeading - curHeading;

                if (deltaHdg > PI)
                    deltaHdg = -(deltaHdg - PI);
                else if (deltaHdg < -PI)
                    deltaHdg = -(deltaHdg + PI);

                turnDist = (float)fabs(deltaHdg * 12.15854203708F * vt);
                break;

            case lToBase:
                dx = self->XPos() - trackX;
                dy = self->YPos() - trackY;
                curHeading = (float)atan2(dy, dx);

                if (curHeading < 0.0F)
                    curHeading += PI * 2.0F;

                Airbase->brain->FindFinalPt(self, rwIndex, &finalX, &finalY);
                dx = trackX - finalX;
                dy = trackY - finalY;
                newHeading = (float)atan2(dy, dx);

                if (newHeading < 0.0F)
                    newHeading += PI * 2.0F;

                deltaHdg = newHeading - curHeading;

                if (deltaHdg > PI)
                    deltaHdg = -(deltaHdg - PI);
                else if (deltaHdg < -PI)
                    deltaHdg = -(deltaHdg + PI);

                turnDist = (float)fabs(deltaHdg * 12.15854203708F * vt);
                break;

            case lToFinal:
            case lOnFinal:
                dx = self->XPos() - trackX;
                dy = self->YPos() - trackY;
                curHeading = (float)atan2(dy, dx);

                if (curHeading < 0.0F)
                    curHeading += PI * 2.0F;

                newHeading = PtHeaderDataTable[rwIndex].data * DTR;

                deltaHdg = newHeading - curHeading;

                if (deltaHdg > PI)
                    deltaHdg = -(deltaHdg - PI);
                else if (deltaHdg < -PI)
                    deltaHdg = -(deltaHdg + PI);

                turnDist = (float)fabs(deltaHdg * 12.15854203708F * vt);

                if (turnDist < 4000.0F)
                    turnDist = 4000.0F;

                break;
        }

        turnDist += (0.5F * vt);
    }


    if (self->IsPlayer())
    {
        if (turnDist < 400.0F)
        {
            turnDist = 400.0F;
        }
    }
    else if (turnDist < 300.0F)
    {
        turnDist = 300.0F;
    }

    return turnDist;
}

void DigitalBrain::Land(void)
{
    SimBaseClass *inTheWay = NULL;
    ObjectiveClass *abObj = (ObjectiveClass *)vuDatabase->Find(airbase);
    AircraftClass *leader = NULL;
    AircraftClass *component = NULL;
    float cosAngle, heading, deltaTime, testDist, distland;
    ulong mini, maxi;
    float baseX, baseY, finalX, finalY, finalZ, x, y, z, dx,
          dy, dist, speed, minSpeed, relx, rely, cosHdg, sinHdg;
    mlTrig Trig;

    if ( not abObj)
    {
        //need to find something or we don't know where to go
        if (self->af->GetSimpleMode())
        {
            SimpleGoToCurrentWaypoint();
        }
        else
        {
            GoToCurrentWaypoint();
        }

        return;
    }

    // RV - Biker - If we're about to land on carrier set new track point
    else if ( not abObj->IsObjective() or not abObj->brain)
    {
        if ((self->curWaypoint->GetWPAction() == WP_LAND) and not self->IsPlayer())
        {
            if (self->curWaypoint->GetPrevWP() not_eq NULL)
            {
                GridIndex abXg, abYg;
                float abXs, abYs;
                abObj->GetLocation(&abXg, &abYg);
                abXs = GridToSim(abXg);
                abYs = GridToSim(abYg);

                SetTrackPoint(abXs, abYs, self->ZPos());
            }

            dx = self->XPos() - trackX;
            dy = self->YPos() - trackY;

            if (dx * dx + dy * dy < 3000.0F * 3000.0F)
            {
                //for carriers we just disappear when we get close enough
                //do carrier landings for F-18
                RegroupAircraft(self);
                return;
            }
        }

        if (self->af->GetSimpleMode())
        {
            SimpleGoToCurrentWaypoint();
        }
        else
        {
            GoToCurrentWaypoint();
        }

        return;
    }

    if (self->IsSetFlag(ON_GROUND) and (af->Fuel() <= 0.0F) and (self->GetVt() < 5.0F))
    {
        if ( not self->IsPlayer())
        {
            RegroupAircraft(self); //no gas get him out of the way
            // sfr: isnt this enough to return
            // added return
            return;
        }
    }

    SetDebugLabel(abObj);

    dx = self->XPos() - trackX;
    dy = self->YPos() - trackY;
    speed = af->MinVcas() * KNOTS_TO_FTPSEC;

    if (rwIndex > 0)
    {
        // jpo - only valid if we have a runway.
        cosAngle = abObj->brain->DetermineAngle(self, rwIndex, atcstatus);
        abObj->brain->CalculateMinMaxTime(self, rwIndex, atcstatus, &mini, &maxi, cosAngle);
    }
    else
    {
        cosAngle = 0;
        mini = maxi = 0;
    }

    // edg: project out 1 sec to get alt for possible ground avoid
    float gAvoidZ = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
                    self->YPos() + self->YDelta());
    float minZ = abObj->brain->GetAltitude(self, atcstatus);

    switch (atcstatus)
    {
        case noATC:
            // sfr: massive change here, we only process if we have a valid runway
            rwIndex = abObj->brain->FindBestLandingRunway(self, TRUE);

            if (rwIndex == 0)
            {
                // no runway (rwIndex == 0), try divert
                airbase = self->DivertAirbase();
                abObj = static_cast<ObjectiveClass*>(vuDatabase->Find(airbase));
                rwIndex = (abObj == NULL) ? 0 : abObj->brain->FindBestLandingRunway(self, TRUE);

                if (rwIndex == 0)
                {
                    // divert too was out of order, try a nearby one
                    // sfr: fixing xy order
                    GridIndex gx, gy;
                    //gx = SimToGrid(self->YPos());
                    //gy = SimToGrid(self->XPos());
                    ::vector pos = { self->XPos(), self->YPos() };
                    ConvertSimToGrid(&pos, &gx, &gy);
                    abObj = FindNearestFriendlyRunway(self->GetTeam(), gx, gy);

                    if (abObj)
                    {
                        airbase = abObj->Id();
                        rwIndex = abObj->brain->FindBestLandingRunway(self, TRUE);
                    }

                    if (rwIndex == 0)
                    {
                        // even then failed... navigate to waypoint then...
                        if (af->GetSimpleMode())
                        {
                            SimpleGoToCurrentWaypoint();
                        }
                        else
                        {
                            GoToCurrentWaypoint();
                        }

                        return;
                    }
                }
            }

            // if got here, rwIndex must be valid
            float tx, ty;
            abObj->brain->FindFinalPt(self, rwIndex, &tx, &ty);
            SetTrackPoint(tx, ty);
            trackZ = abObj->brain->GetAltitude(self, atcstatus);
            CalculateNextTurnDistance();
            waittimer = SimLibElapsedTime + 2 * TAKEOFF_TIME_DELTA;
            TrackPointLanding(af->CalcTASfromCAS(af->MinVcas() * 1.2F) * KNOTS_TO_FTPSEC);
            dx = self->XPos() - abObj->XPos();
            dy = self->YPos() - abObj->YPos();

            //me123
            if (
                curMode not_eq LandingMode and 
                curMode not_eq TakeoffMode and 
                curMode not_eq WaypointMode and 
                curMode not_eq RTBMode
            )
            {
                break;
            }

            if (dx * dx + dy * dy < APPROACH_RANGE * NM_TO_FT * NM_TO_FT * 0.95F)
            {
                atcstatus = lReqClearance;

                if ( not isWing)
                    SendATCMsg(lReqClearance);
            }
            else
            {
                atcstatus = lIngressing;
                SendATCMsg(lIngressing);
            }

            if (self->ZPos() - gAvoidZ > minZ)
            {
                trackZ = gAvoidZ + minZ - (self->ZPos() - gAvoidZ - minZ) * 2.0f;
            }

            break;

            //////////////////////////////////////////////////////////////////////////////////////
        case lIngressing:
            dx = self->XPos() - abObj->XPos();
            dy = self->YPos() - abObj->YPos();
            distland = sqrtf(dx * dx + dy * dy);

            if (distland < 30.0f * NM_TO_FT)
            {
                float tx, ty, tz;
                rwIndex = abObj->brain->FindBestLandingRunway(self, TRUE);
                abObj->brain->FindFinalPt(self, rwIndex, &tx, &ty);
                tz = abObj->brain->GetAltitude(self, atcstatus);
                SetTrackPoint(tx, ty, tz);
                CalculateNextTurnDistance();
                atcstatus = lReqClearance;

                if ( not isWing)
                    SendATCMsg(lReqClearance);

                waittimer = SimLibElapsedTime + LAND_TIME_DELTA;
            }

            if (self->ZPos() - gAvoidZ > minZ)
            {
                trackZ = gAvoidZ + minZ - (self->ZPos() - gAvoidZ - minZ) * 2.0f;
            }

            //Cobra
            if (distland * FT_TO_NM < 15.0F)
            {
                TrackPointLanding(af->CalcTASfromCAS(af->MinVcas() * 1.2F)*KNOTS_TO_FTPSEC);
            }
            else
            {
                TrackPointLanding(af->CalcTASfromCAS(af->CornerVcas())*KNOTS_TO_FTPSEC);
            }


            break;

            //////////////////////////////////////////////////////////////////////////////////////
        case lTakingPosition:
            //need to drag out formation
            //the atc will clear us when we get within range
            dx = self->XPos() - abObj->XPos();
            dy = self->YPos() - abObj->YPos();

            if (dx * dx + dy * dy < TOWER_RANGE * NM_TO_FT * NM_TO_FT * 0.5F and waittimer < SimLibElapsedTime)
            {
                atcstatus = lReqClearance;

                if ( not isWing)
                    SendATCMsg(lReqClearance);

                waittimer = SimLibElapsedTime + LAND_TIME_DELTA;
            }

            speed = af->CalcTASfromCAS(af->MinVcas() * 1.2F) * KNOTS_TO_FTPSEC;

            if (((Unit)self->GetCampaignObject())->GetTotalVehicles() > 1)
            {
                {
                    VuListIterator cit(self->GetCampaignObject()->GetComponents());
                    component = (AircraftClass*)cit.GetFirst();

                    while (component and component->vehicleInUnit not_eq self->vehicleInUnit)
                    {
                        leader = component;
                        component = (AircraftClass*)cit.GetNext();
                    }
                }

                if (leader and not mpActionFlags[AI_RTB]) // JB 010527 (from MN)
                {
                    dx = self->XPos() - leader->XPos();
                    dy = self->YPos() - leader->YPos();
                    dist = dx * dx + dy * dy;

                    if (dist < NM_TO_FT * NM_TO_FT)
                        speed = af->CalcTASfromCAS(af->MinVcas()) * KNOTS_TO_FTPSEC;

                    SetTrackPoint(leader->XPos(), leader->YPos(), leader->ZPos());
                }
                else
                {
                    float tx, ty, tz;
                    abObj->brain->FindFinalPt(self, rwIndex, &tx, &ty);
                    tz = abObj->brain->GetAltitude(self, lTakingPosition);
                    SetTrackPoint(tx, ty, tz);
                }
            }

            if (self->ZPos() - gAvoidZ > minZ)
            {
                trackZ = gAvoidZ + minZ - (self->ZPos() - gAvoidZ - minZ) * 2.0f;
            }

            TrackPointLanding(speed);
            break;

            //////////////////////////////////////////////////////////////////////////////////////
        case lReqClearance:
        case lReqEmerClearance:
            if (self->ZPos() - gAvoidZ > minZ)
            {
                trackZ = gAvoidZ + minZ - (self->ZPos() - gAvoidZ - minZ) * 2.0f;
            }

            if (SimLibElapsedTime > waittimer)
            {
                //we've been waiting too long, call again
                float tx, ty, tz;
                rwIndex = abObj->brain->FindBestLandingRunway(self, TRUE);
                abObj->brain->FindFinalPt(self, rwIndex, &tx, &ty);
                tz = abObj->brain->GetAltitude(self, atcstatus);

                if (self->ZPos() - gAvoidZ > minZ)
                {
                    tz = gAvoidZ + minZ - (self->ZPos() - gAvoidZ - minZ) * 2.0f;
                }

                SetTrackPoint(tx, ty, tz);
                CalculateNextTurnDistance();

                // JB 010802 RTBing AI aircraft won't land.
                dx = self->XPos() - abObj->XPos();
                dy = self->YPos() - abObj->YPos();

                if (dx * dx + dy * dy < (TOWER_RANGE + 100) * NM_TO_FT * NM_TO_FT * 0.95F)
                {
                    waittimer = SimLibElapsedTime + LAND_TIME_DELTA;
                    SendATCMsg(lReqClearance);
                }
                else
                {
                    waittimer = SimLibElapsedTime + LAND_TIME_DELTA / 2;
                }
            }

            //we're waiting to get a response back
            TrackPointLanding(af->CalcTASfromCAS(af->MinVcas() * 1.2F)*KNOTS_TO_FTPSEC);
            af->gearHandle = -1.0F;
            break;

            //////////////////////////////////////////////////////////////////////////////////////
        case lAborted:
            if (self->ZPos() - gAvoidZ > minZ)
            {
                trackZ = gAvoidZ + minZ - (self->ZPos() - gAvoidZ - minZ) * 2.0f;
            }

            if (dx * dx + dy * dy < 0.25F * NM_TO_FT * NM_TO_FT)
            {
                float tx, ty, tz;
                waittimer = SimLibElapsedTime + LAND_TIME_DELTA;
                rwIndex = abObj->brain->FindBestLandingRunway(self, TRUE);
                abObj->brain->FindFinalPt(self, rwIndex, &tx, &ty);
                tz = abObj->brain->GetAltitude(self, lReqClearance);
                z = TheMap.GetMEA(trackX, trackY);

                if (self->ZPos() - gAvoidZ > minZ)
                {
                    tz = gAvoidZ + minZ - (self->ZPos() - gAvoidZ - minZ) * 2.0f;
                }

                SetTrackPoint(tx, ty, tz);
                atcstatus = lReqClearance;
                SendATCMsg(lReqClearance);
            }

            TrackPoint(0.0F, af->CalcTASfromCAS(af->MinVcas() * 1.2F)*KNOTS_TO_FTPSEC);
            af->gearHandle = -1.0F;
            break;

            //////////////////////////////////////////////////////////////////////////////////////
        case lEmerHold:
            if (self->ZPos() - gAvoidZ > minZ)
            {
                trackZ = gAvoidZ + minZ - (self->ZPos() - gAvoidZ - minZ) * 2.0f;
            }

            if (waittimer < SimLibElapsedTime)
            {
                atcstatus = lEmerHold;
                SendATCMsg(lEmerHold);
                waittimer = SimLibElapsedTime + LAND_TIME_DELTA;
            }

            Loiter();
            af->gearHandle = -1.0F;
            break;

            //////////////////////////////////////////////////////////////////////////////////////
        case lHolding:
            if (self->ZPos() - gAvoidZ > minZ)
            {
                trackZ = gAvoidZ + minZ - (self->ZPos() - gAvoidZ - minZ) * 2.0f;
            }

            if (rwtime < SimLibElapsedTime + maxi - CampaignSeconds * 5)
            {
                abObj->brain->FindFinalPt(self, rwIndex, &finalX, &finalY);
                waittimer = rwtime + CampaignSeconds * 15;

                if (cosAngle < 0.0F)
                {
                    abObj->brain->FindBasePt(self, rwIndex, finalX, finalY, &baseX, &baseY);
                    float tx, ty, tz;
                    atcstatus = abObj->brain->FindFirstLegPt(self, rwIndex, rwtime, baseX, baseY, TRUE, &tx, &ty);
                    tz = abObj->brain->GetAltitude(self, atcstatus);
                    SetTrackPoint(tx, ty, tz);

                    if (atcstatus not_eq lHolding)
                    {
                        SendATCMsg(atcstatus);
                    }

                    CalculateNextTurnDistance();
                }
                else
                {
                    float tx, ty, tz;
                    atcstatus = abObj->brain->FindFirstLegPt(
                                    self, rwIndex, rwtime, finalX, finalY, FALSE, &tx, &ty
                                );
                    tz = abObj->brain->GetAltitude(self, atcstatus);
                    SetTrackPoint(tx, ty, tz);

                    if (atcstatus not_eq lHolding)
                    {
                        SendATCMsg(atcstatus);
                    }

                    CalculateNextTurnDistance();
                }

                if (self->ZPos() - gAvoidZ > minZ)
                {
                    trackZ = gAvoidZ + minZ - (self->ZPos() - gAvoidZ - minZ) * 2.0f;
                }

                if (atcstatus == lHolding)
                {
                    Loiter();
                }
            }
            else
            {
                Loiter();
            }

            af->gearHandle = -1.0F;
            break;

            //////////////////////////////////////////////////////////////////////////////////////
        case lFirstLeg:
            if (self->pctStrength < 0.4F)
            {
                abObj->brain->RequestEmerClearance(self);
            }

            if (self->ZPos() - gAvoidZ > minZ)
            {
                trackZ = gAvoidZ + minZ - (self->ZPos() - gAvoidZ - minZ) * 2.0f;
            }

            cosHdg = self->platformAngles.cossig;
            sinHdg = self->platformAngles.sinsig;

            relx = (cosHdg * dx + sinHdg * dy);
            rely = (-sinHdg * dx + cosHdg * dy);

            if (fabs(relx) < turnDist and fabs(rely) < turnDist * 3.0F)
            {
                abObj->brain->FindFinalPt(self, rwIndex, &finalX, &finalY);

                if (cosAngle < 0.0F)
                {
                    atcstatus = lToBase;
                    abObj->brain->FindBasePt(self, rwIndex, finalX, finalY, &baseX, &baseY);
                    SetTrackPoint(baseX, baseY, abObj->brain->GetAltitude(self, atcstatus));
                    SendATCMsg(atcstatus);
                    CalculateNextTurnDistance();
                }
                else
                {
                    atcstatus = lToFinal;
                    SetTrackPoint(finalX, finalY, abObj->brain->GetAltitude(self, atcstatus));
                    SendATCMsg(atcstatus);
                    CalculateNextTurnDistance();
                }

                if (self->ZPos() - gAvoidZ > minZ)
                {
                    trackZ = gAvoidZ + minZ - (self->ZPos() - gAvoidZ - minZ) * 2.0f;
                }
            }

            TrackPointLanding(af->CalcTASfromCAS(af->MinVcas())*KNOTS_TO_FTPSEC);
            break;

            //////////////////////////////////////////////////////////////////////////////////////
        case lToBase:
            if (self->pctStrength < 0.4F)
            {
                abObj->brain->RequestEmerClearance(self);
            }

        case lEmergencyToBase:
            if (self->ZPos() - gAvoidZ > minZ)
            {
                trackZ = gAvoidZ + minZ - (self->ZPos() - gAvoidZ - minZ) * 2.0f;
            }

            //if(dx*dx + dy*dy < turnDist*turnDist)
            cosHdg = self->platformAngles.cossig;
            sinHdg = self->platformAngles.sinsig;

            relx = (cosHdg * dx + sinHdg * dy);
            rely = (-sinHdg * dx + cosHdg * dy);

            if (fabs(relx) < turnDist and fabs(rely) < turnDist * 3.0F)
            {
                abObj->brain->FindFinalPt(self, rwIndex, &finalX, &finalY);

                if (atcstatus == lEmergencyToBase)
                {
                    atcstatus = lEmergencyToFinal;
                }
                else
                {
                    atcstatus = lToFinal;
                }

                SetTrackPoint(finalX, finalY, abObj->brain->GetAltitude(self, atcstatus));

                if (self->ZPos() - gAvoidZ > minZ)
                {
                    trackZ = gAvoidZ + minZ - (self->ZPos() - gAvoidZ - minZ) * 2.0f;
                }

                SendATCMsg(atcstatus);
                CalculateNextTurnDistance();
            }

            if (rwtime > SimLibElapsedTime + FINAL_TIME + BASE_TIME)
            {
                deltaTime = (rwtime - SimLibElapsedTime - FINAL_TIME - BASE_TIME) / (float)CampaignSeconds;
                speed = (float)sqrt(dx * dx + dy * dy) / deltaTime;
                speed = min(af->CalcTASfromCAS(af->MaxVcas() * 0.8F) * KNOTS_TO_FTPSEC,
                            max(speed, af->CalcTASfromCAS(af->MinVcas() * 0.8F) * KNOTS_TO_FTPSEC));
            }
            else
            {
                speed = af->CalcTASfromCAS(af->MaxVcas() * 0.8F) * KNOTS_TO_FTPSEC;
            }

            TrackPointLanding(speed);
            break;

            //////////////////////////////////////////////////////////////////////////////////////
        case lToFinal:
            if (self->pctStrength < 0.4F)
            {
                abObj->brain->RequestEmerClearance(self);
            }

        case lEmergencyToFinal:
            if (self->ZPos() - gAvoidZ > minZ)
            {
                trackZ = gAvoidZ + minZ - (self->ZPos() - gAvoidZ - minZ) * 2.0f;
            }

            cosHdg = PtHeaderDataTable[rwIndex].cosHeading;
            sinHdg = PtHeaderDataTable[rwIndex].sinHeading;
            relx = (cosHdg * dx + sinHdg * dy);
            rely = (-sinHdg * dx + cosHdg * dy);

            if (relx < 3.0F * NM_TO_FT and relx > -1.0F * NM_TO_FT and fabs(rely) < turnDist)
            {
                SetTaxiPoint(GetFirstPt(rwIndex));
                float tx, ty, tz;
                TranslatePointData(abObj, curTaxiPoint, &tx, &ty);
                atcstatus = lOnFinal;

                if (atcstatus == lEmergencyToFinal)
                {
                    atcstatus = lEmergencyOnFinal;
                }
                else
                {
                    atcstatus = lOnFinal;
                }

                tz = abObj->brain->GetAltitude(self, atcstatus);
                SetTrackPoint(tx, ty, tz);
                SendATCMsg(atcstatus);
                af->gearHandle = 1.0F;
            }

            if (rwtime > SimLibElapsedTime + FINAL_TIME)
            {
                deltaTime = (rwtime - SimLibElapsedTime - FINAL_TIME) / (float)CampaignSeconds;
                speed = (float)sqrt(dx * dx + dy * dy) / deltaTime;
                speed = min(af->CalcTASfromCAS(af->MaxVcas() * 0.8F) * KNOTS_TO_FTPSEC,
                            max(speed, af->CalcTASfromCAS(af->MinVcas() * 0.8F) * KNOTS_TO_FTPSEC));
            }
            else
            {
                speed = af->CalcTASfromCAS(af->MaxVcas() * 0.8F) * KNOTS_TO_FTPSEC;
            }

            TrackPointLanding(speed);
            break;

            //////////////////////////////////////////////////////////////////////////////////////
        case lEmergencyOnFinal:
        case lOnFinal:

            // now we just wait until touchdown
            if (self->IsSetFlag(ON_GROUND))
            {
                atcstatus = lLanded;
                SendATCMsg(atcstatus);
                ClearATCFlag(RequestTakeoff);
                SetATCFlag(Landed);
                int fp = GetFirstPt(rwIndex);
                SetTaxiPoint(GetNextPtLoop(fp));
                float tx, ty, tz;
                TranslatePointData(abObj, curTaxiPoint, &tx, &ty);
                tz = af->groundZ;
                SetTrackPoint(tx, ty, tz);
                SimpleGroundTrack(100.0F * KNOTS_TO_FTPSEC);
                break;
            }

            //don't do ground avoidance
            groundAvoidNeeded = FALSE;

            //TJL 02/21/04
            /*if(af->vt < af->CalcTASfromCAS(af->MinVcas() + 30.0F)*KNOTS_TO_FTPSEC or
             (af->gearPos > 0.0F and af->vt < af->CalcTASfromCAS(af->MinVcas() + 10.0F)*KNOTS_TO_FTPSEC))*/
            if (af->vcas < 270.0F)
            {
                af->gearHandle = 1.0F;
            }

            if (cosAngle > 0.0F and af->groundZ - self->ZPos() > 50.0F)
            {
                SetTaxiPoint(GetFirstPt(rwIndex));
                float tx, ty;
                TranslatePointData(abObj, curTaxiPoint, &tx, &ty);
                SetTrackPoint(tx, ty);

                //until chris moves the landing points
                //trackX -= 500.0F * PtHeaderDataTable[rwIndex].cosHeading;
                //trackY -= 500.0F * PtHeaderDataTable[rwIndex].sinHeading;

                dx = trackX - self->XPos();
                dy = trackY - self->YPos();
                dist = (float)sqrt(dx * dx + dy * dy);

                //decelerate as we approach
                minSpeed = af->CalcTASfromCAS(af->MinVcas()) * KNOTS_TO_FTPSEC;

                deltaTime = ((float)rwtime - (float)SimLibElapsedTime) / CampaignSeconds;
                testDist = (minSpeed * 0.2F / (FINAL_TIME / CampaignSeconds) * deltaTime + minSpeed * 0.6F) * deltaTime;
                desiredSpeed = (minSpeed * 0.4F / (FINAL_TIME / CampaignSeconds) * deltaTime + minSpeed * 0.6F);

                if (dist > minSpeed * 19.5F)
                {
                    desiredSpeed += (dist - testDist) / (testDist * 0.8F) * desiredSpeed;
                }
                else if (desiredSpeed > self->GetVt())
                {
                    desiredSpeed = self->GetVt();
                }

                desiredSpeed = max(min(minSpeed * 1.2F, desiredSpeed), minSpeed * 0.6F);
                finalZ = abObj->brain->GetAltitude(self, lOnFinal);
                trackZ = finalZ - dist * TAN_THREE_DEG_GLIDE;

                //recalculate track point to help line up better
                trackX += dist * 0.8F * PtHeaderDataTable[rwIndex].cosHeading;
                trackY += dist * 0.8F * PtHeaderDataTable[rwIndex].sinHeading;

                testDist = 0;
                x = self->XPos();
                y = self->YPos();

                while (testDist < dist * 0.2F and dist > 3000.0F)
                {
                    x -= 200.0F * PtHeaderDataTable[rwIndex].cosHeading;
                    y -= 200.0F * PtHeaderDataTable[rwIndex].sinHeading;

                    z = OTWDriver.GetGroundLevel(x, y);

                    if (dist < 6000.0F)
                    {
                        if (z - 100.0F < trackZ)
                        {
                            trackZ = z - 100.0F;
                        }
                    }
                    else
                    {
                        if (z - 200.0F < trackZ)
                        {
                            trackZ = z - 200.0F;
                        }
                    }

                    testDist += 200.0F;
                }

                if (af->groundZ - self->ZPos() > 200.0F)
                {
                    TrackPointLanding(desiredSpeed);
                }
                else
                {
                    TrackPointLanding(af->GetLandingAoASpd());
                }
            }
            else
            {
                float tx, ty, tz;
                //flare at the last minute so we hit softly
                int fp = GetFirstPt(rwIndex);
                SetTaxiPoint(GetNextPt(fp));
                TranslatePointData(abObj, curTaxiPoint, &tx, &ty);
                tz = abObj->brain->GetAltitude(self, atcstatus);
                SetTrackPoint(tx, ty, tz);
                TrackPointLanding(af->GetLandingAoASpd());
                pStick = - 0.01685393258427F;
            }

            break;

            //////////////////////////////////////////////////////////////////////////////////////
        case lLanded:

            // edg: If we're not on the ground, we're fucked (which has
            // been seen).  We can't stay in this state.  Go to aborted.
            if ( not self->OnGround())
            {
                ShiWarning("Please show this to Dave P (x4373)");

                float rx = self->dmx[0][0] * dx + self->dmx[0][1] * dy;

                if (rx > 3000.0F and af->IsSet(AirframeClass::OverRunway))
                {
                    atcstatus = lOnFinal;
                    SendATCMsg(atcstatus);
                    ClearATCFlag(Landed);
                    TrackPointLanding(af->MinVcas()* KNOTS_TO_FTPSEC * 0.6F);
                    pStick = - 0.01685393258427F;
                }
                else
                {
                    atcstatus = lAborted;
                    SendATCMsg(atcstatus);
                    float tx, ty, tz;
                    abObj->brain->FindAbortPt(self, &tx, &ty, &tz);
                    SetTrackPoint(tx, ty, tz);
                    TrackPoint(0.0F, af->MinVcas() * 1.2F * KNOTS_TO_FTPSEC);
                    af->gearHandle = -1.0F;
                }

                break;
            }

            trackZ = af->groundZ;

            if (CloseToTrackPoint())
            {
                // Are we there yet?
                switch (PtDataTable[GetNextPtLoop(curTaxiPoint)].type)
                {
                    case TaxiPt:
                        SetTaxiPoint(GetNextPtLoop(curTaxiPoint));
                        atcstatus = lTaxiOff;
                        waittimer = 0;
                        SendATCMsg(atcstatus);
                        break;

                    case TakeRunwayPt:
                    case TakeoffPt:
                        SetTaxiPoint(GetNextPtLoop(curTaxiPoint));
                        break;

                    case RunwayPt:
                        //we shouldn't be here
                        SetTaxiPoint(GetNextPtLoop(curTaxiPoint));
                        break;
                }

                float tx, ty, tz = af->groundZ;
                TranslatePointData(abObj, curTaxiPoint, &tx, &ty);
                SetTrackPoint(tx, ty, tz);
            }

            inTheWay = CheckTaxiTrackPoint();

            if (inTheWay and inTheWay->GetVt() < 10.0F)
            {
                heading = (float)atan2(trackX - self->XPos(), trackY - self->YPos());
                mlSinCos(&Trig, heading);

                trackX += TAXI_CHECK_DIST * Trig.cos;
                trackY += TAXI_CHECK_DIST * Trig.sin;
            }

            // JPO pop the chute
            if (af->HasDragChute() and 
                af->dragChute == AirframeClass::DRAGC_STOWED and 
                af->vcas < af->DragChuteMaxSpeed())
            {
                af->dragChute = AirframeClass::DRAGC_DEPLOYED;
            }

            // Thrust reverser
            if ((af->HasThrRev()) and (af->thrustReverse < 2))
                af->thrustReverse = 2;

            // Retract reverse thruster
            if ((af->HasThrRev()) and (self->GetVt() < af->MinVcas() * 0.3F))
                af->thrustReverse = 0;

            if (self->GetVt() < af->MinVcas() * 0.4F)  // was 0.6f
            {
                if (af->dragChute == AirframeClass::DRAGC_DEPLOYED)
                    af->dragChute = AirframeClass::DRAGC_JETTISONNED;
            }

            if (self->GetVt() < af->MinVcas() * 0.55F)  // clean up
            {
                if (af->speedBrake > -1.0F)
                    af->speedBrake = -1.0F;
            }

            if (self->GetVt() < af->MinVcas() * 0.5F)  // THW 2003-11-23 more clean up (this one a bit later)
            {
                if (af->tefPos > 0)
                    af->TEFClose();

                if (af->lefPos > 0)
                    af->LEFClose();
            }

            dx = trackX - af->x;
            dy = trackY - af->y;

            if (dx * dx + dy * dy > 1500.0F * 1500.0F)
            {
                SimpleGroundTrack(min(60.0F * KNOTS_TO_FTPSEC, af->MinVcas() * 0.4F)); //THW 2003-11-23 No hurry...no need to re-accelerate
            }
            else if (dx * dx + dy * dy > 500.0F * 500.0F) //THW 2003-11-23 Slow down
            {
                SimpleGroundTrack(min(50.0F * KNOTS_TO_FTPSEC, af->MinVcas() * 0.3F));
            }
            else if (dx * dx + dy * dy > 200.0F * 200.0F) //THW 2003-11-23 Slow down
            {
                SimpleGroundTrack(35.0F * KNOTS_TO_FTPSEC);
            }
            else
            {
                SimpleGroundTrack(20.0F * KNOTS_TO_FTPSEC);
            }

            if (g_nShowDebugLabels) //THW 2003-11-23 Some debug stuff to observe landing deccelaration behaviour
            {
                char label [96];
                sprintf(label, "Speed: %d kts, PointDist: %d ft", FloatToInt32(self->GetVt() / KNOTS_TO_FTPSEC), FloatToInt32(sqrt(dx * dx + dy * dy)));

                if (self->drawPointer)
                    ((DrawableBSP*)self->drawPointer)->SetLabel(label, ((DrawableBSP*)self->drawPointer)->LabelColor());
            }

            break;

            //////////////////////////////////////////////////////////////////////////////////////
        case lTaxiOff:

            // JPO - only halt when we really get there, so the above condition continues to fire.
            if (AtFinalTaxiPoint())
            {
                if (waittimer == 0)
                {
                    waittimer = SimLibElapsedTime + g_nReagTimer * CampaignMinutes;
                }

                dx = trackX - af->x;
                dy = trackY - af->y;

                if (dx * dx + dy * dy < 10 * 10)
                {
                    desiredSpeed = 0.0F;
                }
                else
                {
                    desiredSpeed = 20.0F * KNOTS_TO_FTPSEC * (float)sqrt(dx * dx + dy * dy) / TAXI_CHECK_DIST;
                }

                desiredSpeed = min(20.0F * KNOTS_TO_FTPSEC, desiredSpeed);

                if (waittimer < SimLibElapsedTime or (desiredSpeed == 0 and self->GetVt() < 0.1f))
                {
                    //TJL 03/01/04 Reorder this. Canopy up later.
                    // then clean up
                    if ( not af->IsSet(AirframeClass::EngineStopped))
                    {
                        af->SetFlag(AirframeClass::EngineStopped);
                    }
                    //TJL 03/01/04 Dual Engine off
                    else if (af->GetNumberEngines() == 2 and not af->IsEngineFlag(AirframeClass::EngineStopped2))
                    {
                        af->SetEngineFlag(AirframeClass::EngineStopped2);
                    }
                    else if ( not af->canopyState)
                    {
                        af->CanopyToggle();
                    }
                    else if (
                        af->rpm < 0.05f and 
                        self->MainPower() not_eq AircraftClass::MainPowerOff
                    )
                    {
                        self->DecMainPower();
                    }
                    else if (
                        self not_eq SimDriver.GetPlayerEntity() and 
                        (g_nReagTimer <= 0 or waittimer < SimLibElapsedTime)
                    )
                    {
                        // 02JAN04 - FRB - Make parking spot available for others
                        PtDataTable[curTaxiPoint].flags and_eq compl PT_OCCUPIED;
                        RegroupAircraft(self); //end of the line, time to pull you
                    }
                }
            }
            else if (CloseToTrackPoint()) // time to step along
            {
                switch (PtDataTable[GetNextPtLoop(curTaxiPoint)].type)
                {
                    case TaxiPt: // nothing special
                    default:
                        SetTaxiPoint(GetNextPtLoop(curTaxiPoint));
                        break;

                    case SmallParkPt:
                    case LargeParkPt: // possible parking spot
                        FindParkingSpot(abObj);
                        break;
                }

                // Are we there yet?
                switch (PtDataTable[curTaxiPoint].flags)
                {
                        //this taxi point is in middle of list
                    case 0:
                        break;

                        //this should be the runway pt, we shouldn't be here
                    case PT_FIRST:
                        break;

                    case PT_LAST:  // if at last PD, a/c should reagg...no parking spots available

                        //....looks bad to have bunch of a/c out in the fields
                        if (self not_eq SimDriver.GetPlayerEntity())
                            RegroupAircraft(self);  //end of the line, time to pull you

                        break;
                }

                float tx, ty, tz = af->groundZ;
                TranslatePointData(abObj, curTaxiPoint, &tx, &ty);
                SetTrackPoint(tx, ty, tz);
                desiredSpeed = 20.0F * KNOTS_TO_FTPSEC;
            }
            else
            {
                desiredSpeed = 20.0F * KNOTS_TO_FTPSEC;
            }

            inTheWay = CheckTaxiTrackPoint();

            if (inTheWay and inTheWay->GetVt() < 10.0F)
            {
                switch (PtDataTable[curTaxiPoint].type)
                {
                    case SmallParkPt: //was a possible parking spot, alas no more.
                    case LargeParkPt: // someone beat us to it.
                        if (PtDataTable[curTaxiPoint].flags not_eq PT_LAST)
                        {
                            FindParkingSpot(abObj); // try again
                            float tx, ty;
                            TranslatePointData(abObj, curTaxiPoint, &tx, &ty);
                            SetTrackPoint(tx, ty);
                            break;
                        }

                        // else fall
                    default:

                        // Cobra - Skip a/c on parking spots
                        if (
                            (((AircraftClass*)inTheWay)->DBrain()) and 
                            ((PtDataTable[((AircraftClass*)inTheWay)->DBrain()->GetTaxiPoint()].type == LargeParkPt or
                              (PtDataTable[((AircraftClass*)inTheWay)->DBrain()->GetTaxiPoint()].type == SmallParkPt)))
                        )
                        {
                            float tx, ty;
                            TranslatePointData(abObj, curTaxiPoint, &tx, &ty);
                            SetTrackPoint(tx, ty);
                        }
                        else
                        {
                            OffsetTrackPoint(TAXI_CHECK_DIST, offRight); // JPO from offRight
                        }

                        break;
                }
            }

            SimpleGroundTrack(desiredSpeed);
            break;

        default:
            break;
    }

    // COBRA - RED - Apply few randomnes to AI
    RandomStuff(NULL);
}

int DigitalBrain::BestParkSpot(ObjectiveClass *Airbase)  // FRB - Landing-->Park last-to-first or first-to-last???
{
    float x, y;
    int pktype = af->GetParkType();
    int npt, pt = curTaxiPoint;
#if 1  // 17JAN04 - FRB - Find the *closest* available parking spot.
    pt = GetNextParkTypePt(curTaxiPoint, pktype); // find first available parking spot
    npt = pt;

    while (PtDataTable[npt].flags not_eq PT_LAST)
    {
        pt = npt;
        TranslatePointData(Airbase, pt, &x, &y);

        if (CheckPoint(x, y) == NULL)
        {
            // this is good?
            return pt;
        }

        npt = GetNextParkTypePt(pt, pktype);
    }

#else

    while ((npt = GetNextParkTypePt(pt, pktype)) not_eq 0) // find last possible parking spot
        pt = npt;

    if (PtDataTable[pt].type not_eq pktype) // just in case we found none  // FRB - isn't pt == 0 here???
        return 0;

    while (pt > curTaxiPoint)
    {
        TranslatePointData(Airbase, pt, &x, &y);

        if (CheckPoint(x, y) == NULL)
        {
            // this is good?
            return pt;
        }

        pt = GetPrevParkTypePt(pt, pktype); // ok try another  // FRB - shouldn't it be GetNextParkTypePt
    }

#endif
    return pt;
}

void DigitalBrain::FindParkingSpot(ObjectiveClass *Airbase) // FRB - Landing-->Park (repeated calls - checks next pt for available parking spot)
{
#if 1
    int bestpt = BestParkSpot(Airbase);
    int pt = GetNextPtLoop(curTaxiPoint);

    while (PtDataTable[pt].flags not_eq PT_LAST)
    {
        if (pt == bestpt)
        {
            // next taxi point is our favoured parking spot
            SetTaxiPoint(pt);
            return;
        }
        else if (
            PtDataTable[pt].type == SmallParkPt or // already used (not bestpt), skip
            PtDataTable[pt].type == LargeParkPt
        )
        {
            pt = GetNextPtLoop(pt);
        }
        else
        {
            SetTaxiPoint(pt); // keep on trucking
            return;
        }
    }

    SetTaxiPoint(pt);
#else
    float x, y;
    int pt = GetNextPtLoop(curTaxiPoint);
    int pktype = af->GetParkType();

    while (PtDataTable[pt].flags not_eq PT_LAST)
    {
        if (PtDataTable[pt].type == pktype)
        {
            TranslatePointData(Airbase, pt, &x, &y);

            if (CheckPoint(x, y) == NULL)
            {
                SetTaxiPoint(pt);
                return;
            }

            pt ++; // try next
        }
        else if (PtDataTable[pt].type == SmallParkPt or
                 PtDataTable[pt].type == LargeParkPt)
        {
            pt = GetNextPtLoop(pt);
        }
        else
        {
            // not a parking spot, so carry on
            SetTaxiPoint(pt);
            return;
        }
    }

    SetTaxiPoint(pt);
#endif
}

// JPO - true if we're at last point, or at a suitable parking place
bool DigitalBrain::AtFinalTaxiPoint()
{
    // final point is one of these
    if (PtDataTable[curTaxiPoint].type == SmallParkPt or
        PtDataTable[curTaxiPoint].type == LargeParkPt or
        PtDataTable[curTaxiPoint].flags == PT_LAST)
    {
        return CloseToTrackPoint();
    }

    return false;
}

bool DigitalBrain::CloseToTrackPoint()
{
    if (fabs(trackX - af->x) < TAXI_CHECK_DIST and 
        fabs(trackY - af->y) < TAXI_CHECK_DIST)
        return true;

    return false;
}

#ifdef DAVE_DBG
void DigitalBrain::SetDebugLabel(ObjectiveClass *Airbase)
{
    // Update the label in debug
    char tmpStr[40];
    runwayQueueStruct *info = NULL;
    runwayQueueStruct *testInfo = NULL;
    int pos = 0;

    if (atcstatus == noATC)
        sprintf(tmpStr, "noATC");
    else
    {
        float xft, yft, rx;
        int status;
        //int delta = waittimer - SimLibElapsedTime;
        int rwdelta = rwtime - SimLibElapsedTime;

        xft = trackX - self->XPos();
        yft = trackY - self->YPos();
        // get relative position and az
        rx = self->dmx[0][0] * xft + self->dmx[0][1] * yft;

        info = Airbase->brain->InList(self->Id());

        if (info)
        {
            status = info->status;

            if (info and info->rwindex)
            {
                runwayQueueStruct* testInfo = info;

                while (testInfo->prev)
                {
                    testInfo = testInfo->prev;
                }

                while (testInfo)
                {
                    if (info->aircraftID == testInfo->aircraftID)
                        break;

                    pos++;
                    testInfo = testInfo->next;
                }
            }

            //sprintf (tmpStr,"%d %d %d %d %4.2f %4.2f", pos,  curTaxiPoint, atcstatus, status, delta/1000.0F, rwdelta/1000.0F);
            sprintf(tmpStr, "%d %d %d %d %3.1f %3.1f %3.1f", pos,  curTaxiPoint, atcstatus, status, desiredSpeed, rwdelta / 1000.0F, SimLibElapsedTime / 60000.0F);
        }
        else
            sprintf(tmpStr, "%d %d %3.1f %3.1f %3.1f",  curTaxiPoint, atcstatus, desiredSpeed, rwdelta / 1000.0F, SimLibElapsedTime / 60000.0F);

        //sprintf (tmpStr,"%d %d %4.2f %4.2f",  curTaxiPoint, atcstatus, delta/1000.0F, rwdelta/1000.0F);

    }

    if (self->drawPointer)
        ((DrawableBSP*)self->drawPointer)->SetLabel(tmpStr, ((DrawableBSP*)self->drawPointer)->LabelColor());
}
#endif



// FEW Randomic stuff done by AI
void DigitalBrain::RandomStuff(SimBaseClass *inTheWay)
{
    // will be used below as needed
    bool setTaxiLight;

    if (TheTimeOfDay.GetLightLevel() >= 0.65f)
    {
        setTaxiLight = false;
        //self->ClearAcStatusBits(AircraftClass::ACSTATUS_EXT_LANDINGLIGHT);
    }
    else
    {
        setTaxiLight = true;
        //self->SetAcStatusBits(AircraftClass::ACSTATUS_EXT_LANDINGLIGHT);
    }

    switch (atcstatus)
    {
        case noATC:
        case lReqClearance:
        case lReqEmerClearance:
        case lIngressing:
        case lTakingPosition:
        case lAborted:
        case lEmerHold:
        case lHolding:
        case lFirstLeg:
        case lToBase:
        case lToFinal:
        case lEmergencyToBase:
        case lEmergencyToFinal:
        case lClearToLand:
        case lCrashed:
            break;

        case lEmergencyOnFinal:
        case lOnFinal:
            self->SetAcStatusBits(AircraftClass::ACSTATUS_EXT_LANDINGLIGHT);
            break;

        case lLanded:
        {
            if (setTaxiLight and PRANDFloatPos() > 0.998f)
            {
                self->SetAcStatusBits(AircraftClass::ACSTATUS_EXT_LANDINGLIGHT);
                self->SetSwitch(COMP_3DPIT_INTERIOR_LIGHTS, TRUE);
            }
        }
        break;

        case lTaxiOff:
        {
            if (AtFinalTaxiPoint() and desiredSpeed == 0)
            {
                self->ClearAcStatusBits(AircraftClass::ACSTATUS_EXT_LANDINGLIGHT);
            }

            if (setTaxiLight)
            {
                self->SetSwitch(COMP_3DPIT_INTERIOR_LIGHTS, TRUE);
            }
        }
        break;

        case tReqTaxi:
        case tReqTakeoff:
        case tEmerStop:
        case tTaxi:
        {
            //if (PRANDFloatPos()>0.998f or inTheWay){ // FRB
            // af->canopyState = false;
            //}
            if (setTaxiLight and (PRANDFloatPos() > 0.998f or inTheWay))
            {
                self->SetAcStatusBits(AircraftClass::ACSTATUS_EXT_LANDINGLIGHT);
            }

            if (PRANDFloatPos() > 0.998f)
            {
                self->SetSwitch(COMP_3DPIT_INTERIOR_LIGHTS, FALSE);
            }
        }
        break;

        case tWait:
        case tHoldShort:
        case tPrepToTakeRunway:
        case tTakeRunway:
        case tTakeoff:
        {
            if (setTaxiLight)
            {
                self->SetAcStatusBits(AircraftClass::ACSTATUS_EXT_LANDINGLIGHT);
            }

            self->SetSwitch(COMP_3DPIT_INTERIOR_LIGHTS, FALSE);
            af->canopyState = false;
        }
        break;

        case tFlyOut:
        case tTaxiBack:
            break;
    }

    // if (af->LLON){ self->SetAcStatusBits(AircraftClass::ACSTATUS_EXT_LANDINGLIGHT); }
    // else { self->ClearAcStatusBits(AircraftClass::ACSTATUS_EXT_LANDINGLIGHT); }

    if (self->GetSwitch(COMP_3DPIT_INTERIOR_LIGHTS))
    {
        self->SetAcStatusBits(AircraftClass::ACSTATUS_PITLIGHT);
    }
    else
    {
        self->ClearAcStatusBits(AircraftClass::ACSTATUS_PITLIGHT);
    }
}



void DigitalBrain::TakeOff()
{
    SimBaseClass *inTheWay = NULL;
    ObjectiveClass *Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);
    float xft, yft, rx, distSq;
    AircraftClass *flight_leader = NULL; //RAS - 12Nov04 - used to check for human movement in RAMP start


    flight_leader = (AircraftClass*)self->GetCampaignObject()->GetComponentNumber(0); //RAS

    if ( not Airbase or not Airbase->IsObjective() or not Airbase->brain)
    {
        //need to find the airbase or we don't know where to go
        // JB carrier ShiWarning("Show this to Dave P. (no airbase)");
        //Cobra Appears aircraft were getting stuck in takeoff mode
        //and looping in waypoint/takeoff mode.  This cycles them to next point
        //since they don't have normal takeoff routines when doing carrier stuff

        // RV - Biker - Only switch to next WP if we are at takeoff
        if (self->curWaypoint->GetWPAction() == WP_TAKEOFF)
            self->curWaypoint = self->curWaypoint->GetNextWP();

        return;
    }

    if (self->IsSetFlag(ON_GROUND) and af->Fuel() <= 0.0F and self->GetVt() < 5.0F)
    {
        if (self not_eq SimDriver.GetPlayerEntity())
            RegroupAircraft(self);  //no gas get him out of the way
    }

    if (af->IsSet(AirframeClass::EngineOff))
        af->ClearFlag(AirframeClass::EngineOff);

    //TJL 01/22/04 multi-engine
    if (af->IsSet(AirframeClass::EngineOff2))
        af->ClearFlag(AirframeClass::EngineOff2);

    if (af->IsSet(AirframeClass::ThrottleCheck))
        af->ClearFlag(AirframeClass::ThrottleCheck);

    SetDebugLabel(Airbase);

    //Cobra This was causing CAP to switch off Fred ;)
    //me123 make sure ap if off for player in multiplay
    if (self == SimDriver.GetPlayerEntity() and IsSetATC(StopPlane) and SimDriver.GetPlayerEntity()->IsLocal())
    {
        ClearATCFlag(StopPlane);
        af->ClearFlag(AirframeClass::WheelBrakes);
        /*if(self->AutopilotType()==AircraftClass::CombatAP)
         self->SetAutopilot(AircraftClass::APOff);*/ //Offensive code ;)
        // Cobra - still trying to stop creep forward
        // sfr: WTF is this???????????????? @todo remove this shit
        af->vt = 0.0F;
        return;
    }

    // JPO - should we run preflight checks...
    if ( not IsSetATC(DonePreflight) and curTaxiPoint)
    {
        VU_TIME t2t; // time to takeoff

        if (rwtime > 0) // value given by ATC
            t2t = rwtime;
        else
            t2t = self->curWaypoint->GetWPDepartureTime(); // else original scheduled time

        // Cobra - Start with canopy open if on parking spot
        if ((PlayerOptions.GetStartFlag() not_eq PlayerOptionsClass::START_RUNWAY) and 
            ((af->GetParkType() not_eq LargeParkPt)))
        {
            af->canopyState = true;
            self->ClearAcStatusBits(AircraftClass::ACSTATUS_EXT_LANDINGLIGHT);

            if (TheTimeOfDay.GetLightLevel() < 0.65f)
            {
                self->SetSwitch(COMP_3DPIT_INTERIOR_LIGHTS, TRUE);  // Cobra - Light up cockpit
                self->SetAcStatusBits(AircraftClass::ACSTATUS_PITLIGHT);
            }
            else
            {
                self->SetSwitch(COMP_3DPIT_INTERIOR_LIGHTS, FALSE);  // Cobra - Light up cockpit
                self->ClearAcStatusBits(AircraftClass::ACSTATUS_PITLIGHT);
            }
        }


        //RAS - 14Oct04 - Orig = 3, take max of g_nTaxiLaunchTime or 3 min
        if (SimLibElapsedTime > t2t - max(g_nTaxiLaunchTime * CampaignMinutes, 3 * CampaignMinutes))
        {
            // emergency pre-flight
            QuickPreFlight();
        }
        else if (SimLibElapsedTime < t2t - PlayerOptionsClass::RAMP_MINUTES * CampaignMinutes)
        {
            return; // not time for startup yet
        }
        else if (
            PtDataTable[curTaxiPoint].flags == PT_LAST or
            PtDataTable[curTaxiPoint].type == SmallParkPt or
            PtDataTable[curTaxiPoint].type == LargeParkPt
        )
        {
            //RAS-12Nov04-If flight leader is Human and does not have combat autopilot on and
            //has left parking, then quick preflight wingman so they can follow human lead
            if (
                isWing and flight_leader and flight_leader->AutopilotType() not_eq AircraftClass::CombatAP
               and flight_leader->DBrain()->curTaxiPoint not_eq flight_leader->spawnpoint
            )
            {
                QuickPreFlight();
                SetATCFlag(DonePreflight);
                return;
            }
            else if ( not PreFlight())
            {
                // slow preflight
                return;
            }
        }
        else
        {
            QuickPreFlight();
        }

        SetATCFlag(DonePreflight);

        // COBRA - RED - Wingmans always confirm to humans when have checked in
        // sfr: added NULL check
        if (isWing and flight_leader and flight_leader->IsPlayer())
        {
            //short edata[10];
            FalconRadioChatterMessage* radioMessage = new FalconRadioChatterMessage(
                self->Id(), FalconLocalSession
            );
            radioMessage->dataBlock.from = self->Id();
            radioMessage->dataBlock.to = MESSAGE_FOR_FLIGHT;
            radioMessage->dataBlock.voice_id =
                ((Flight)(self->GetCampaignObject()))->GetPilotVoiceID(self->vehicleInUnit);
            radioMessage->dataBlock.message = rcREADYFORDERARTURE;
            radioMessage->dataBlock.edata[0] = -1;
            radioMessage->dataBlock.edata[1] = -1;
            radioMessage->dataBlock.edata[2] =
                ConvertWingNumberToCallNumber(self->GetCampaignObject()->GetComponentIndex(self));;
            radioMessage->dataBlock.time_to_play = 3000;
            FalconSendMessage(radioMessage, FALSE);
        }
    }

    // if we're damaged taxi back
    if (self->pctStrength < 0.7f and self->IsSetFlag(ON_GROUND))
    {
        TaxiBack(Airbase);
        return;
    }

    xft = trackX - af->x;
    yft = trackY - af->y;
    rx = self->dmx[0][0] * xft + self->dmx[0][1] * yft;
    distSq = xft * xft + yft * yft;

    groundAvoidNeeded = FALSE;

    //RAS - This doesn't run on every airbase spawning aircraft, should it?  <== No
    if ( not curTaxiPoint)
    {
        // Spawning
        Flight flight = (Flight)self->GetCampaignObject();

        if (flight)
        {
            WayPoint w;

            w = flight->GetFirstUnitWP();

            if (w)
            {
                // FRB - spawn point - tp on correct parking spot type (Small/Large)
                SetTaxiPoint(FindDesiredTaxiPoint(w->GetWPDepartureTime()));

                if (curTaxiPoint)
                {
                    self->spawnpoint = curTaxiPoint;
                    float tx, ty;
                    TranslatePointData(Airbase, curTaxiPoint, &tx, &ty);
                    SetTrackPoint(tx, ty);
                    CalcWaitTime(Airbase->brain);
                    int npt = GetPrevTaxiPt(curTaxiPoint); // this is next place to go to
                    float x, y;
                    TranslatePointData(Airbase, npt, &x, &y);
                    self->af->initialPsi = self->af->psi = self->af->sigma = (float)atan2((y - self->af->y), (x - self->af->x));
                    self->af->initialX = self->af->x;
                    self->af->initialY = self->af->y;
                    af->canopyState = true;
                }
            }
        }
    }

    if (g_nShowDebugLabels bitand 0x80000)
    {
        char label [32];
        sprintf(label, "TaxiPt %d, type: ", curTaxiPoint);

        switch (PtDataTable[curTaxiPoint].type)
        {
            case SmallParkPt:
                strcat(label, "SmallParkPt");
                break;

            case LargeParkPt:
                strcat(label, "LargeParkPt");
                break;

            case TakeoffPt:
                strcat(label, "TakeoffPt");
                break;

            case RunwayPt:
                strcat(label, "RunwayPt");
                break;

            case TaxiPt:
                strcat(label, "TaxiPt");
                break;

            case CritTaxiPt:
                strcat(label, "CritTaxiPt");
                break;

            case TakeRunwayPt:
                strcat(label, "TakeRunwayPt");
                break;
        }

        if (self->drawPointer)
            ((DrawableBSP*)self->drawPointer)->SetLabel(label, ((DrawableBSP*)self->drawPointer)->LabelColor());
    }


    switch (atcstatus)
    {
        case noATC:

            /*AI_MESSAGE(0,"AI - NO_ATC");*/
            if ( not self->IsSetFlag(ON_GROUND))
            {
                // ShiAssert( not "Show this to Dave P. (not on ground)");
                if (isWing)
                    self->curWaypoint = self->curWaypoint->GetNextWP();
                else
                    SelectNextWaypoint();

                break;
            }

            trackZ = af->groundZ;
            ClearATCFlag(RequestTakeoff);
            ClearATCFlag(PermitRunway);
            ClearATCFlag(PermitTakeoff);

            switch (PtDataTable[curTaxiPoint].type)
            {
                case SmallParkPt:
                case LargeParkPt:
                {
                    atcstatus = tReqTakeoff;
                    waittimer = SimLibElapsedTime + TAKEOFF_TIME_DELTA;
                    float tx, ty;
                    TranslatePointData(Airbase, curTaxiPoint, &tx, &ty);
                    SetTrackPoint(tx, ty);

                    if ( not isWing)
                    {
                        SendATCMsg(tReqTakeoff);
                    }

                    break;
                }

                case TakeoffPt:
                {
                    atcstatus = tReqTakeoff;
                    rwIndex = Airbase->brain->IsOnRunway(self);
                    float tx, ty;

                    if (GetFirstPt(rwIndex) == curTaxiPoint - 1)
                    {
                        SetTaxiPoint(
                            Airbase->brain->FindTakeoffPt(
                                (Flight)self->GetCampaignObject(),
                                self->vehicleInUnit,
                                rwIndex,
                                &tx, &ty
                            )
                        );
                    }
                    else
                    {
                        rwIndex = Airbase->brain->GetOppositeRunway(rwIndex);
                        SetTaxiPoint(
                            Airbase->brain->FindTakeoffPt(
                                (Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &tx, &ty
                            )
                        );
                    }

                    SetTrackPoint(tx, ty);
                    waittimer = SimLibElapsedTime;

                    if ( not isWing)
                    {
                        SendATCMsg(atcstatus);
                    }
                }
                break;

                case RunwayPt:
                {
                    atcstatus = tReqTakeoff;
                    rwIndex = Airbase->brain->IsOnRunway(self);
                    float tx, ty;

                    if (GetFirstPt(rwIndex) == curTaxiPoint)
                    {
                        SetTaxiPoint(
                            Airbase->brain->FindRunwayPt(
                                (Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &tx, &ty
                            )
                        );
                    }
                    else
                    {
                        rwIndex = Airbase->brain->GetOppositeRunway(rwIndex);
                        SetTaxiPoint(
                            Airbase->brain->FindRunwayPt(
                                (Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &tx, &ty
                            )
                        );
                    }

                    SetTrackPoint(tx, ty);

                    waittimer = SimLibElapsedTime;

                    if ( not isWing)
                    {
                        SendATCMsg(tReqTakeoff);
                    }

                    break;
                }

                case TaxiPt:
                case CritTaxiPt:
                case TakeRunwayPt:
                default:
                {
                    float tx, ty;
                    atcstatus = tReqTakeoff;
                    TranslatePointData(Airbase, curTaxiPoint, &tx, &ty);
                    SetTrackPoint(tx, ty);
                    // waittimer = SimLibElapsedTime + TAKEOFF_TIME_DELTA;
                    waittimer = SimLibElapsedTime + (VU_TIME)(HurryUp * CampaignSeconds); // FRB - speed up moving

                    if ( not isWing)
                    {
                        SendATCMsg(tReqTakeoff);
                    }

                    break;
                }
            }

            SimpleGroundTrack(0.0F);
            break;

            //////////////////////////////////////////////////////////////////////////////////////
        case tReqTakeoff:
        case tReqTaxi:

            //AI_MESSAGE(0,"AI - REQ TAXI");
            // 17JAN04 - FRB - clear occupied parking spot flag
            if (
                (PtDataTable[curTaxiPoint].type == SmallParkPt) or
                (PtDataTable[curTaxiPoint].type == LargeParkPt)
            )
            {
                PtDataTable[curTaxiPoint].flags and_eq compl PT_OCCUPIED;
            }

            if (SimLibElapsedTime > waittimer + TAKEOFF_TIME_DELTA)
            {
                //we've been waiting too long, call again
                SendATCMsg(atcstatus);
                waittimer = SimLibElapsedTime;
            }

            //we're waiting to get a response back
            SimpleGroundTrack(0.0F);
            break;

            //////////////////////////////////////////////////////////////////////////////////////
        case tWait:
            //AI_MESSAGE(0,"AI - WAITING");
            desiredSpeed = 0.0F;

            if (
                (distSq < TAXI_CHECK_DIST * TAXI_CHECK_DIST) or
                (rx < 1.0F and distSq < TAXI_CHECK_DIST * TAXI_CHECK_DIST * 4.0F)
            )
            {
                ChooseNextPoint(Airbase);
            }
            else
            {
                inTheWay = CheckTaxiTrackPoint();

                if (inTheWay)
                {
                    if (isWing)
                    {
                        DealWithBlocker(inTheWay, Airbase);
                    }
                }
                else
                {
                    //default speed
                    if (SimLibElapsedTime > waittimer + TAKEOFF_TIME_DELTA)
                    {
                        CalculateTaxiSpeed(HurryUp);
                    }
                    else
                    {
                        CalculateTaxiSpeed(MoveAlong);
                    }
                }

                trackZ = af->groundZ;
            }

            SimpleGroundTrack(desiredSpeed);
            break;

            //////////////////////////////////////////////////////////////////////////////////////
        case tTaxi:

            //AI_MESSAGE(0,"AI - TAXING");
            // 17JAN04 - FRB - clear occupied parking spot flag
            if ((PtDataTable[curTaxiPoint].type == SmallParkPt) or (PtDataTable[curTaxiPoint].type == LargeParkPt))
            {
                PtDataTable[curTaxiPoint].flags and_eq compl PT_OCCUPIED;
            }

            desiredSpeed = 0.0F;

            //if we haven't reached our desired taxi point, we need to move
            if (
                (distSq < TAXI_CHECK_DIST * TAXI_CHECK_DIST) or
                (rx < 1.0F and distSq < TAXI_CHECK_DIST * TAXI_CHECK_DIST * 4.0F)
            )
            {
                ChooseNextPoint(Airbase);
            }
            else
            {
                inTheWay = CheckTaxiTrackPoint();

                if (inTheWay)
                {
                    //someone is in the way
                    DealWithBlocker(inTheWay, Airbase);
                }
                else
                {
                    //default speed
                    if (SimLibElapsedTime > waittimer + TAKEOFF_TIME_DELTA)
                    {
                        CalculateTaxiSpeed(HurryUp);
                    }
                    else
                    {
                        CalculateTaxiSpeed(MoveAlong);
                    }
                }

                trackZ = af->groundZ;
            }

            SimpleGroundTrack(desiredSpeed);
            break;

            //////////////////////////////////////////////////////////////////////////////////////
        case tHoldShort:
            //AI_MESSAGE(0,"AI - HOLDSHORT");

            desiredSpeed = 0.0F;

            if (rwtime < (MoveAlong * CampaignSeconds + SimLibElapsedTime) and waittimer < SimLibElapsedTime)
            {
                SendATCMsg(atcstatus);
                waittimer = CalcWaitTime(Airbase->brain);
            }

            ChooseNextPoint(Airbase);

            if (desiredSpeed == 0.0F and Airbase->brain->IsOnRunway(trackX, trackY))
            {
                OffsetTrackPoint(50.0F, rightRunway);
                //CalculateTaxiSpeed(MoveAlong);
                CalculateTaxiSpeed(HurryUp);  // 17JAN04 - FRB - Expedite takeoff
            }

            SimpleGroundTrack(desiredSpeed);
            break;

            //////////////////////////////////////////////////////////////////////////////////////
        case tTakeRunway:
            /* AI_MESSAGE(0,"AI - TAKE RUNWAY");*/
            desiredSpeed = 0.0F;

            //if we haven't reached our desired taxi point, we need to move
            if (
                (distSq < TAXI_CHECK_DIST * TAXI_CHECK_DIST) or
                (rx < 0.0F and distSq < TAXI_CHECK_DIST * TAXI_CHECK_DIST * 4.0F)
            )
            {
                if (PtDataTable[curTaxiPoint].type not_eq RunwayPt)
                {
                    ChooseNextPoint(Airbase);
                }
            }
            else
            {
                inTheWay = CheckTaxiTrackPoint();

                if (inTheWay)
                {
                    DealWithBlocker(inTheWay, Airbase);
                }
                else
                {
                    // CalculateTaxiSpeed(MoveAlong);
                    CalculateTaxiSpeed(HurryUp);  // 17JAN04 - FRB - Expedite takeoff
                }

                trackZ = af->groundZ;
            }

            if (PtDataTable[curTaxiPoint].type == RunwayPt)
            {
                if (isWing and self->af->IsSet(AirframeClass::OverRunway) and not WingmanTakeRunway(Airbase))
                {
                    float tx, ty;
                    SetTaxiPoint(
                        Airbase->brain->FindTakeoffPt(
                            (Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &tx, &ty
                        )
                    );
                    OffsetTrackPoint(50.0F, rightRunway);
                    // CalculateTaxiSpeed(MoveAlong);
                    CalculateTaxiSpeed(HurryUp);  // 17JAN04 - FRB - Expedite takeoff
                    curTaxiPoint++;
                }
                else if ( not self->af->IsSet(AirframeClass::OverRunway))
                {
                    OffsetTrackPoint(0.0F, centerRunway);
                    curTaxiPoint++;
                }
                else
                {
                    float cosAngle = self->platformAngles.sinsig * PtHeaderDataTable[rwIndex].sinHeading +
                                        self->platformAngles.cossig * PtHeaderDataTable[rwIndex].cosHeading;

                    if (cosAngle >  0.99619F)
                    {
                        if (ReadyToGo())
                        {
                            waittimer = 0;
                            atcstatus = tTakeoff;
                            SendATCMsg(atcstatus);
                            trackZ = af->groundZ - 500.0F;
                        }
                        else
                        {
                            desiredSpeed = 0.0F;
                        }
                    }
                }
            }

            SimpleGroundTrack(desiredSpeed);

            // edg: test for fuckupedness -- I've seen planes taking the runway
            // which are already in the air (bad trackX and Y?).  They never
            // get out of this take runway cycle.   If we find ourselves in
            // this state go to take off since we're off already....
            if ( not self->OnGround())
            {
                ShiWarning("Show this to Dave P. (not on ground)");
                waittimer = 0;
                atcstatus = tTakeoff;
                SendATCMsg(atcstatus);
            }

            break;

            //////////////////////////////////////////////////////////////////////////////////////
        case tTakeoff:

            //AI_MESSAGE(0,"AI - TAKEOFF");
            if (self->OnGround() and not self->af->IsSet(AirframeClass::OverRunway))
            {
                float tx, ty;
                SetTaxiPoint(
                    Airbase->brain->FindTakeoffPt(
                        (Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &tx, &ty
                    )
                );
                SetTrackPoint(tx, ty);
                atcstatus = tTakeRunway;
                SendATCMsg(atcstatus);
                // CalculateTaxiSpeed(MoveAlong);
                CalculateTaxiSpeed(HurryUp);  // 17JAN04 - FRB - Expedite takeoff
                SimpleGroundTrack(desiredSpeed);
                return;
            }

            if (self->OnGround())
                SimpleGroundTrack(af->MinVcas() * KNOTS_TO_FTPSEC);
            else
                TrackPoint(0.0F, (af->MinVcas() + 50.0F) * KNOTS_TO_FTPSEC);

            if (af->z - af->groundZ < -20.0F and af->gearHandle > -1.0F)
                af->gearHandle = -1.0F;

            if (af->z - af->groundZ < -50.0F)
            {
                if (af->gearHandle > -1.0F)
                    af->gearHandle = -1.0F;

                atcstatus = tFlyOut;
                SendATCMsg(atcstatus);
            }

            break;

            //////////////////////////////////////////////////////////////////////////////////////
        case tFlyOut:
            //AI_MESSAGE(0,"AI - FLYOUT");
            int elements;
            trackZ = af->groundZ - 500.0F;

            if (af->gearHandle > -1.0F)
                af->gearHandle = -1.0F;

            // 2001-10-16 added by M.N. #1 performs a 90° base leg to waypoint 2 at takeoff
            // Needed so that lead that will perform the leg will first fly out before it starts the leg
            if (af->z - af->groundZ > -200.0F or (fabs(xft) < 200.0F and fabs(yft) < 200.0F))
            {
                break;
            }

            elements = self->GetCampaignObject()->NumberOfComponents();

            // 2001-10-16 M.N. added elemleader check -> perform a 90° base leg until element lead has taken off
            if ( not isWing and elements > 1) // Code for the flightlead
            {
                // wingy or lead
                AircraftClass *elemleader =
                    (AircraftClass*)self->GetCampaignObject()->GetComponentNumber(elements == 2 ? 1 : 2);

                if (elemleader) // is #3 in a 4- and 3-ship flight, #2 in a 2-ship flight
                {
                    airbase = self->LandingAirbase(); // JPO - now we set to go home
                    ObjectiveClass *Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);

                    if ( not Airbase or elemleader->af->z - elemleader->af->groundZ < -50.0F) // #3 has taken off -> lead continue to next WP
                    {
                        onStation = NotThereYet;
                        SelectNextWaypoint();
                        atcstatus = noATC;
                        SendATCMsg(atcstatus);

                    }
                    else
                    {
                        // #1 and #2 do a takeoff leg - find direction to next waypoint

                        int dir;
                        float tx, ty, dx, dy, dz, dist;
                        float deltaHdg, hdgToPt, acHeading, legHeading;

                        acHeading = self->Yaw(); // fix, use aircrafts real heading instead of runway heading

                        WayPointClass* tmpWaypoint = self->curWaypoint;

                        if (tmpWaypoint)
                        {
                            ShiAssert(tmpWaypoint->GetWPAction() == WP_TAKEOFF);
                            // add this if we have if (tmpWaypoint->GetWPAction() not_eq WP_TAKEOFF and tmpWaypoint->GetPrevWP())
                            // a failed assertion tmpWaypoint = tmpWaypoint->GetPrevWP();

                            tmpWaypoint = tmpWaypoint->GetNextWP();
                            tmpWaypoint->GetLocation(&dx, &dy, &dz);

                            tx = dx - Airbase->XPos();
                            ty = dy - Airbase->YPos();
                            hdgToPt = (float)atan2(ty, tx);

                            if (hdgToPt < 0.0F)
                                hdgToPt += PI * 2.0F;


                            if (acHeading < 0.0F)
                                acHeading += PI * 2.0F;

                            deltaHdg = hdgToPt - acHeading;

                            if (deltaHdg > PI)
                                deltaHdg -= (2.0F * PI);
                            else if (deltaHdg < -PI)
                                deltaHdg += (2.0F * PI);

                            if (deltaHdg < -PI)
                                dir = 1;
                            else if (deltaHdg > PI)
                                dir = 0;
                            else if (deltaHdg < 0.0F)
                                dir = 0;//left
                            else
                                dir = 1;//right

                            legHeading = hdgToPt;

                            // MN CTD fix #2
                            AircraftClass *wingman = NULL;
                            FlightClass *flight = NULL;
                            flight = (FlightClass*)self->GetCampaignObject();
                            ShiAssert( not F4IsBadReadPtr(flight, sizeof(FlightClass)));
                            float factor = 0.0F;

                            if (flight)
                            {
                                wingman = (AircraftClass*)flight->GetComponentNumber(1); // my wingy = 1 in each case
                                ShiAssert( not F4IsBadReadPtr(wingman, sizeof(AircraftClass)));

                                if (wingman and wingman->af and (wingman->af->z - wingman->af->groundZ < -200.0F)) // My wingman has flown out
                                    factor = 45.0F * DTR;

                                ShiAssert(Airbase->brain);

                                if (Airbase->brain and Airbase->brain->UseSectionTakeoff(flight, rwIndex)) // If our wingman took off with us, stay on a 90° leg
                                    factor = 0.0F;
                            }

                            if (dir)
                                legHeading = legHeading - (90.0F * DTR - factor);
                            else
                                legHeading = legHeading + (90.0F * DTR - factor);

                            if (legHeading >= 360.0F * DTR)
                                legHeading -= 360.0F * DTR;

                            if (legHeading < 0.0F)
                                legHeading += 360.0F * DTR;

                            dist = 10.0F * NM_TO_FT;

                            // Set up a new trackpoint

                            dx = Airbase->XPos() + dist * cos(legHeading);
                            dy = Airbase->YPos() + dist * sin(legHeading);

                            SetTrackPoint(dx, dy, -2000.0F + af->groundZ);

                            SetMaxRollDelta(75.0F); // don't roll too much
                            SimpleTrack(SimpleTrackSpd, (af->MinVcas() * 1.2f)); // fly as slow as possible ~ 178 kts
                            SetMaxRollDelta(100.0F);
                            break;
                        }

                    }
                }
            }
            else

                // In the air and ready to go
                if (af->z - af->groundZ < -200.0F or (fabs(xft) < 200.0F and fabs(yft) < 200.0F))
                {
                    onStation = NotThereYet;

                    if (isWing)
                        self->curWaypoint = self->curWaypoint->GetNextWP();
                    else
                        SelectNextWaypoint();

                    atcstatus = noATC;
                    SendATCMsg(atcstatus);
#ifdef DAVE_DBG
                    SetLabel(self);
#endif

                    if (isWing)
                        AiRejoin(NULL, AI_TAKEOFF); // JPO actually a takeoff signal

                    airbase = self->LandingAirbase(); // JPO - now we set to go home
                }

            TrackPoint(0.0F, (af->MinVcas() + 50.0F) * KNOTS_TO_FTPSEC);
            break;

            //////////////////////////////////////////////////////////////////////////////////////
        case tEmerStop:
            desiredSpeed = 0.0F;

            if (waittimer < SimLibElapsedTime)
            {
                SendATCMsg(atcstatus);
                waittimer = CalcWaitTime(Airbase->brain);
            }

            if ( not isWing or (flightLead and flightLead->OnGround()))
            {
                while (Airbase->brain->IsOnRunway(trackX, trackY))
                {
                    OffsetTrackPoint(20.0F, rightRunway);
                    desiredSpeed = 20.0F;
                }

                if (Airbase->brain->IsOnRunway(self) and self->GetVt() < 5.0F)
                {
                    OffsetTrackPoint(20.0F, rightRunway);
                    desiredSpeed = 20.0F;
                }
            }

            if (fabs(trackX - af->x) > TAXI_CHECK_DIST or fabs(trackY - af->y) > TAXI_CHECK_DIST)
            {
                inTheWay = CheckTaxiTrackPoint();

                if (inTheWay)
                {
                    if (isWing)
                        DealWithBlocker(inTheWay, Airbase);
                    else
                        desiredSpeed = 0.0F;
                }
                else
                {
                    //default speed
                    CalculateTaxiSpeed(MoveAlong);
                }
            }
            else
            {
                ChooseNextPoint(Airbase);
            }

            SimpleGroundTrack(desiredSpeed);
            break;

            //////////////////////////////////////////////////////////////////////////////////////
        case tTaxiBack:
            //this will cause them to taxi back, will only occur if ordered by ATC
            TaxiBack(Airbase);
            break;

        default:
            break;
    }

    // COBRA - RED - Apply few randomnes to AI
    RandomStuff(inTheWay);
}

int DigitalBrain::WingmanTakeRunway(ObjectiveClass *Airbase)
{
    AircraftClass *leader = (AircraftClass*)self->GetCampaignObject()->GetComponentNumber(self->vehicleInUnit - 1);

    return WingmanTakeRunway(Airbase, (AircraftClass *)flightLead, leader);
}

int DigitalBrain::WingmanTakeRunway(ObjectiveClass *Airbase, AircraftClass *FlightLead, AircraftClass *leader)
{
    int pt;
    float tempX, tempY;
    //when this function is called, I already know that the point will not move me past any wingmen in front
    //of me unless they are gone or off the ground

    switch (self->vehicleInUnit)
    {
        case 0:
            ShiWarning("This should never happen");
            return TRUE;
            break;

        case 1:
            pt = GetPrevPtLoop(curTaxiPoint);

            TranslatePointData(Airbase, pt, &tempX, &tempY);

            if ( not Airbase->brain->IsOnRunway(tempX, tempY))
                return TRUE;
            else if ( not FlightLead)
                return TRUE;
            else if ( not FlightLead->OnGround())
                return TRUE;
            else if (Airbase->brain->IsOnRunway(FlightLead) and Airbase->brain->UseSectionTakeoff((Flight)self->GetCampaignObject(), rwIndex))
                return TRUE;
            //RAS-28Oct04-Speed up #2 taking Rwy
            else if (Airbase->brain->IsOnRunway(FlightLead) and not Airbase->brain->UseSectionTakeoff((Flight)self->GetCampaignObject(), rwIndex)
                    and FlightLead->GetVt() > 40.0 * KNOTS_TO_FTPSEC)
                return TRUE;

            break;

        case 2: // Element Leader
            pt = GetPrevPtLoop(curTaxiPoint);

            TranslatePointData(Airbase, pt, &tempX, &tempY);

            if ( not Airbase->brain->IsOnRunway(tempX, tempY))
                return TRUE;

            //RAS - 16Oct04 - Get aircraft on runway and airborne sooner by checking to see if aircraft in
            //front of them on t/o roll are greater than 80 kts, if so, take the runway
            if (FlightLead and not FlightLead->OnGround()) // Cobra - if Flight leader is in the air
            {
                if (
                    (FlightLead->GetVt() > 40 * KNOTS_TO_FTPSEC) and 
                    leader and 
                    (leader->GetVt() > 40.0 * KNOTS_TO_FTPSEC)
                )
                {
                    return TRUE;
                }
            }

            //RAS - 16Oct04 - Get aircraft on runway and airborne sooner by checking to see if aircraft in
            //front of them on t/o roll are greater than 50 kts, if so, take the runway

            else if (leader and leader->OnGround() and leader->GetVt() > 40.0 * KNOTS_TO_FTPSEC)
                return TRUE;
            else if (leader and not leader->OnGround())
                return TRUE;
            else
                return FALSE;

            break;

        default:
        case 3: // Element Wingman
            pt = GetPrevPtLoop(curTaxiPoint);

            TranslatePointData(Airbase, pt, &tempX, &tempY);

            if ( not Airbase->brain->IsOnRunway(tempX, tempY))
                return TRUE;
            else if (leader and Airbase->brain->IsOnRunway(leader) and Airbase->brain->UseSectionTakeoff((Flight)self->GetCampaignObject(), rwIndex))
                return TRUE;
            //RAS-28Oct04-Speed up #4 taking Rwy
            else if (leader and Airbase->brain->IsOnRunway(leader) and not Airbase->brain->UseSectionTakeoff((Flight)self->GetCampaignObject(), rwIndex)
                    and leader->GetVt() > 40.0 * KNOTS_TO_FTPSEC)
                return TRUE;

            //RAS - 28Oct04 -speed up #4 taking rwy
            if (FlightLead and not FlightLead->OnGround())
            {
                if (FlightLead->GetVt() > 50 * KNOTS_TO_FTPSEC and leader and leader->GetVt() > 40.0 * KNOTS_TO_FTPSEC)
                    return TRUE;
            }

            FlightLead = (AircraftClass*)self->GetCampaignObject()->GetComponentNumber(1);

            //RAS - 28Oct04 -speed up #4 taking rwy
            if (FlightLead and not FlightLead->OnGround())
            {
                if (FlightLead->GetVt() > 50 * KNOTS_TO_FTPSEC and leader and leader->GetVt() > 40.0 * KNOTS_TO_FTPSEC)
                    return TRUE;
            }
            else if ( not leader)
                return TRUE;
            else if ( not leader->OnGround())
                return TRUE;
            else if (Airbase->brain->IsOnRunway(leader) and Airbase->brain->UseSectionTakeoff((Flight)self->GetCampaignObject(), rwIndex))
                return TRUE;

            break;
    }

    return FALSE;
}

void DigitalBrain::DealWithBlocker(SimBaseClass *inTheWay, ObjectiveClass *Airbase)
{
    float tmpX = 0.0F, tmpY = 0.0F, ry = 0.0F;
    int extraWait = 0;
    bool BigBoy = false;

    SimBaseClass *inTheWay2 = NULL;

    // Cobra - "large" aircraft?
    if (self->af->GetParkType() == LargeParkPt)
        BigBoy = true;

    desiredSpeed = 0.0F;

    if (inTheWay->GetCampaignObject() == self->GetCampaignObject() and self->vehicleInUnit > ((AircraftClass*)inTheWay)->vehicleInUnit)
    {
        return;//we never taxi around fellow flight members
    }

    switch (PtDataTable[curTaxiPoint].type)
    {
        case TakeoffPt:
            SetTaxiPoint(
                Airbase->brain->FindTakeoffPt(
                    (Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &tmpX, &tmpY
                )
            );
            break;

        case RunwayPt:
        {
            float tx, ty;
            SetTaxiPoint(
                Airbase->brain->FindRunwayPt(
                    (Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &tx, &ty
                )
            );
            SetTrackPoint(tx, ty);
        }
        break;

        default:
        case TaxiPt:
            TranslatePointData(Airbase, curTaxiPoint, &tmpX, &tmpY);
            break;
    }

    if (tmpX not_eq trackX or tmpY not_eq trackY)
        inTheWay2 = CheckPoint(tmpX, tmpY);
    else
        inTheWay2 = inTheWay;

    if (atcstatus not_eq tTakeRunway)
    {
        extraWait = FalconLocalGame->rules.AiPatience; // FRB - not *that* patient

        if (rwtime > SimLibElapsedTime)
            extraWait += (rwtime - SimLibElapsedTime) / 20; // FRB - Could be a long extra wait (was 20)
    }

    if ( not inTheWay2 and ( not inTheWay->IsAirplane() or ((AircraftClass*)inTheWay)->DBrain()->RwTime() > rwtime))
    {
        SetTrackPoint(tmpX, tmpY);
        waittimer = CalcWaitTime(Airbase->brain);

        if (SimLibElapsedTime > waittimer + TAKEOFF_TIME_DELTA)
        {
            CalculateTaxiSpeed(HurryUp);
        }
        else
        {
            CalculateTaxiSpeed(MoveAlong);
        }
    }
    else if ((isWing and ( not inTheWay2 or inTheWay2->GetCampaignObject() not_eq self->GetCampaignObject() or
                         self->vehicleInUnit < ((AircraftClass*)inTheWay2)->vehicleInUnit)) or
             (inTheWay->GetVt() < 5.0F and SimLibElapsedTime > waittimer + extraWait) or
             (inTheWay->IsAirplane() and ((AircraftClass*)inTheWay)->DBrain()->RwTime() > rwtime))
    {
        tmpX = inTheWay->XPos() - self->XPos();
        tmpY = inTheWay->YPos() - self->YPos();
        ry = self->dmx[1][0] * tmpX + self->dmx[1][1] * tmpY;

        switch (PtDataTable[GetPrevPtLoop(curTaxiPoint)].type)
        {
            default:
            case CritTaxiPt:
            case TaxiPt:
                if ( not BigBoy) // Skip the dance for big boys
                {
                    while (CheckTaxiTrackPoint() == inTheWay)
                    {
                        if (ry > 0.0F)
                            OffsetTrackPoint(10.0F, offLeft);
                        else
                            OffsetTrackPoint(10.0F, offRight);
                    }
                }

                CalculateTaxiSpeed(HurryUp);
                break;

            case TakeRunwayPt:

                //take runway if we have permission, else holdshort
                if (isWing or IsSetATC(PermitRunway) or IsSetATC(PermitTakeRunway))
                {
                    while (CheckTaxiTrackPoint() == inTheWay)
                    {
                        if (ry > 0.0F)
                            OffsetTrackPoint(10.0F, offLeft);
                        else
                            OffsetTrackPoint(10.0F, offRight);
                    }

                    CalculateTaxiSpeed(HurryUp);
                }
                else if (PtDataTable[curTaxiPoint].type == TakeRunwayPt)
                {
                    while (CheckTaxiTrackPoint() == inTheWay)
                    {
                        if (ry > 0.0F)
                            OffsetTrackPoint(10.0F, offLeft);
                        else
                            OffsetTrackPoint(10.0F, offRight);
                    }

                    CalculateTaxiSpeed(HurryUp);
                }

                break;

            case RunwayPt:
            case TakeoffPt:

                //take runway if we have permission, else holdshort
                if (isWing or IsSetATC(PermitRunway))
                {
                    while (CheckTaxiTrackPoint() == inTheWay)
                        OffsetTrackPoint(20.0F, downRunway);

                    CalculateTaxiSpeed(HurryUp);
                }

                break;
        }
    }
}

void DigitalBrain::ChooseNextPoint(ObjectiveClass *Airbase)  // to Runway  Takeoff() bitand ResetTaxiState()
{
    int pt = 0;
    BOOL HP_Is_Leader = FALSE;
    bool HP_Moving = false; // Cobra
    desiredSpeed = 0.0F;
    AircraftClass *leader = NULL;
    int minPoint = GetFirstPt(rwIndex);
    //RAS - 15Oct04 - used to determine if human lead has Combat AP on
    AircraftClass *flight_leader = NULL;

    //RAS - add AircraftClass for testing for Combat Autopilot
    flight_leader = (AircraftClass*)self->GetCampaignObject()->GetComponentNumber(0);

    // Cobra - Is the Human Pilot (HP) on the move?
    if (flight_leader and flight_leader->GetVt() > 10.0f * KNOTS_TO_FTPSEC)
    {
        HP_Moving = true;
    }

    // Cobra - If Human Pilot in control, then clear his parking spot and tell the AI to taxi to the runway
    if (flight_leader and flight_leader->IsPlayer() and flight_leader->AutopilotType() not_eq AircraftClass::CombatAP)
    {
        HP_Is_Leader = TRUE;

        if (HP_Moving)
        {
            // Cobra - Make HP's parking spot available
            PtDataTable[flight_leader->spawnpoint].flags and_eq compl PT_OCCUPIED;
            ((AircraftClass*)flight_leader)->DBrain()->SetTaxiPoint(GetFirstPt(rwIndex));
        }
    }

    //RAS - if human(i.e. CombatAP not on) then skip this and taxi
    if (
        flight_leader and 
        (
            (flight_leader->IsPlayer() and 
             (flight_leader->AutopilotType() == AircraftClass::CombatAP)) or
 not flight_leader->IsPlayer()
        )
    )
    {
        if (SimLibElapsedTime < waittimer and not IsSetATC(PermitRunway) and not IsSetATC(PermitTakeRunway))
        {
            return;
        }
    }

    if (isWing)
    {
        leader = (AircraftClass*)self->GetCampaignObject()->GetComponentNumber(self->vehicleInUnit - 1);

        if (HP_Is_Leader and HP_Moving and leader and (leader == flight_leader or leader->IsPlayer()))
        {
            // Cobra
            minPoint = GetFirstPt(rwIndex);
        }
        else if (
            leader and 
            leader->IsPlayer() and 
            HP_Moving and leader->AutopilotType() not_eq AircraftClass::CombatAP
        )
        {
            // Cobra
            minPoint = GetFirstPt(rwIndex);
        }
        else if (leader and leader->OnGround() and leader->DBrain()->ATCStatus() not_eq tTaxiBack)
        {
            minPoint = leader->DBrain()->GetTaxiPoint();
        }

        /*
         if (flightLead and flightLead->IsPlayer() and flightlead->AutopilotType() not_eq AircraftClass::CombatAP) // Cobra
         minPoint =0;
         else
         if(flightLead and flightLead->OnGround() and 
         ((AircraftClass*)flightLead)->DBrain()->GetTaxiPoint() > minPoint and 
         ((AircraftClass*)flightLead)->DBrain()->ATCStatus() not_eq tTaxiBack )
         minPoint = ((AircraftClass*)flightLead)->DBrain()->GetTaxiPoint();
        */
    }

    // else if( SimLibElapsedTime < waittimer and not IsSetATC(PermitRunway) and not IsSetATC(PermitTakeRunway))
    // return;


    switch (PtDataTable[GetPrevPtLoop(curTaxiPoint)].type)
    {
        case LargeParkPt:
        case SmallParkPt: // skip these on taxi
            if (isWing and GetPrevPtLoop(curTaxiPoint) <= minPoint)
            {
                return;
            }
            else
            {
                //just taxi along
                pt = GetPrevTaxiPt(curTaxiPoint);

                if ((pt == 0) or (isWing and pt <= minPoint))
                {
                    return;
                }

                SetTaxiPoint(pt);
                float tx, ty;
                TranslatePointData(Airbase, curTaxiPoint, &tx, &ty);
                SetTrackPoint(tx, ty);
                CalculateTaxiSpeed(MoveAlong);
                waittimer = CalcWaitTime(Airbase->brain);
            }

            break;

        default:
        case CritTaxiPt:
        case TaxiPt:
            if (isWing and GetPrevTaxiPt(curTaxiPoint) <= minPoint)
            {
                // 17JAN04 - FRB - Get taxi pt
                return;
            }

            // RAS-11Nov04-check to see if we are in parking and if our leader is in parking or in next
            // spot after parking, if so, don't move.
            // This allows aircraft to file out of parking versus all move at once.
            // sfr: added leader check and concatenated 2 checks in one if
            if (isWing and leader and (
                    PtDataTable[curTaxiPoint].type == SmallParkPt or
                    PtDataTable[curTaxiPoint].type == LargeParkPt
                ) and (
                    (
                        PtDataTable[leader->DBrain()->curTaxiPoint].type == SmallParkPt or
                        PtDataTable[leader->DBrain()->curTaxiPoint].type == LargeParkPt
                    ) or (
                        leader->DBrain()->curTaxiPoint >= (leader->spawnpoint) - 1 and 
                        leader->GetVt() < 0.1f * KNOTS_TO_FTPSEC
                    )
                )
               )
            {
                return;
            }

            // Cobra - Close the canopy
            if (af->canopyState)
                af->canopyState = false; // OK

            // Cobra - Clear parking spot occupied flag
            PtDataTable[self->spawnpoint].flags and_eq compl PT_OCCUPIED; // Cobra - Make HP's parking spot available

            //just taxi along
            pt = GetPrevTaxiPt(curTaxiPoint);    // 17JAN04 - FRB - Get taxi pt (prev pt may be parking pt)

            if ( not pt)
            {
                // no more TaxiPt's, must be at entrance to runway
                SetTaxiPoint(GetPrevPtLoop(curTaxiPoint));
            }
            else
            {
                SetTaxiPoint(pt);
            }

            float tx, ty;
            TranslatePointData(Airbase, curTaxiPoint, &tx, &ty);
            SetTrackPoint(tx, ty);
            CalculateTaxiSpeed(MoveAlong);

            if (isWing)
            {
                // FRB - let the leader get out first
                waittimer = CalcWaitTime(Airbase->brain);
            }
            else
            {
                waittimer = 0; // FRB - try no wait time
            }

            break;

        case TakeRunwayPt:

            //take runway if we have permission, else holdshort
            if (isWing)
            {
                if (GetPrevPtLoop(curTaxiPoint) == minPoint and not IsSetATC(PermitRunway))
                {
                    return;
                }

                if (GetPrevPtLoop(curTaxiPoint) < minPoint)
                {
                    return;
                }

                if (WingmanTakeRunway(Airbase, (AircraftClass*)flightLead, leader))
                {
                    SetTaxiPoint(GetPrevPtLoop(curTaxiPoint));
                    float tx, ty;
                    TranslatePointData(Airbase, curTaxiPoint, &tx, &ty);
                    SetTrackPoint(tx, ty);
                    CalculateTaxiSpeed(MoveAlong);
                    // waittimer = CalcWaitTime(Airbase->brain);
                    waittimer = 0;

                    //RAS - 17Oct04
                    if (atcstatus not_eq tTakeRunway)
                    {
                        atcstatus = tTakeRunway;
                        SendATCMsg(atcstatus);
                    }

                    //end RAS
                }
                else if ( not Airbase->brain->IsOnRunway(GetPrevPtLoop(GetPrevPtLoop(curTaxiPoint))))
                {
                    SetTaxiPoint(GetPrevPtLoop(GetPrevPtLoop(curTaxiPoint)));
                    float tx, ty;
                    TranslatePointData(Airbase, curTaxiPoint, &tx, &ty);
                    SetTrackPoint(tx, ty);
                    CalculateTaxiSpeed(HurryUp);  // FRB - he is already at least 2 taxipts behind
                    // waittimer = CalcWaitTime(Airbase->brain);  // <<-- not long
                    waittimer = 0;
                }
                else if (self->af->IsSet(AirframeClass::OverRunway))
                {
                    OffsetTrackPoint(50.0F, rightRunway);
                    CalculateTaxiSpeed(MoveAlong);
                }
            }
            else if (IsSetATC(PermitRunway) and not self->IsSetFalcFlag(FEC_HOLDSHORT))
            {
                float tx, ty;
                atcstatus = tTakeRunway;
                SetTaxiPoint(GetPrevPtLoop(curTaxiPoint));
                TranslatePointData(Airbase, curTaxiPoint, &tx, &ty);
                SetTrackPoint(tx, ty);
                CalculateTaxiSpeed(MoveAlong);
                waittimer = CalcWaitTime(Airbase->brain);
                // waittimer = 0;
            }
            else if (IsSetATC(PermitTakeRunway) and not self->IsSetFalcFlag(FEC_HOLDSHORT))
            {
                pt = GetPrevPtLoop(curTaxiPoint);
                float tempX, tempY;
                TranslatePointData(Airbase, pt, &tempX, &tempY);

                if ( not Airbase->brain->IsOnRunway(tempX, tempY))
                {
                    SetTaxiPoint(pt);
                    SetTrackPoint(tempX, tempY);
                    //CalculateTaxiSpeed(MoveAlong);
                    CalculateTaxiSpeed(HurryUp);    // 17JAN04 - FRB - expedite takeoff
                    //waittimer = CalcWaitTime(Airbase->brain);  // <<-- not long
                    waittimer = 0;
                }
            }
            else if (PtDataTable[curTaxiPoint].type == TakeRunwayPt and not self->IsSetFalcFlag(FEC_HOLDSHORT))
            {
                SetATCFlag(PermitTakeRunway);
                SetTaxiPoint(GetPrevPtLoop(curTaxiPoint));
                float tx, ty;
                TranslatePointData(Airbase, curTaxiPoint, &tx, &ty);
                SetTrackPoint(tx, ty);
                //CalculateTaxiSpeed(MoveAlong);
                CalculateTaxiSpeed(HurryUp);    // 17JAN04 - FRB - expedite takeoff

                if (atcstatus not_eq tTakeRunway)
                {
                    SendATCMsg(tPrepToTakeRunway);
                    atcstatus = tTaxi;
                }

                //waittimer = CalcWaitTime(Airbase->brain);
                waittimer = 0;
            }
            else if ( not Airbase->brain->IsOnRunway(GetPrevPtLoop(GetPrevPtLoop(curTaxiPoint))))
            {
                SetTaxiPoint(GetPrevPtLoop(GetPrevPtLoop(curTaxiPoint)));
                float tx, ty;
                TranslatePointData(Airbase, curTaxiPoint, &tx, &ty);
                SetTrackPoint(tx, ty);
                CalculateTaxiSpeed(HurryUp);
                //waittimer = CalcWaitTime(Airbase->brain);  // <<-- not long
                waittimer = 0;
            }
            else if (self->af->IsSet(AirframeClass::OverRunway))
            {
                OffsetTrackPoint(50.0F, rightRunway);
                CalculateTaxiSpeed(MoveAlong);
                //CalculateTaxiSpeed(HurryUp);    // 17JAN04 - FRB - expedite takeoff
            }
            else
            {
                if (atcstatus not_eq tTakeRunway and not self->IsSetFalcFlag(FEC_HOLDSHORT))
                {
                    atcstatus = tHoldShort;
                    SendATCMsg(atcstatus);
                }
            }

            break;

        case TakeoffPt:

            //take runway if we have permission, else holdshort
            if (isWing)
            {
                if (WingmanTakeRunway(Airbase, (AircraftClass*)flightLead, leader))
                {
                    float tx, ty;
                    SetTaxiPoint(
                        Airbase->brain->FindTakeoffPt(
                            (Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &tx, &ty
                        )
                    );
                    SetTrackPoint(tx, ty);
                    //CalculateTaxiSpeed(MoveAlong);
                    CalculateTaxiSpeed(HurryUp);    // 17JAN04 - FRB - expedite takeoff

                    if (atcstatus not_eq tTakeRunway)
                    {
                        atcstatus = tTakeRunway;
                        SendATCMsg(atcstatus);
                    }

                    //waittimer = CalcWaitTime(Airbase->brain);  // FRB - do we have to wait?
                    waittimer = 0;
                }
                else if (self->af->IsSet(AirframeClass::OverRunway))
                {
                    OffsetTrackPoint(50.0F, rightRunway);
                    //CalculateTaxiSpeed(MoveAlong);
                    CalculateTaxiSpeed(HurryUp);    // 17JAN04 - FRB - expedite takeoff
                }
            }
            else if (IsSetATC(PermitRunway) and not self->IsSetFalcFlag(FEC_HOLDSHORT))
            {
                SetATCFlag(PermitRunway);
                float tx, ty;
                SetTaxiPoint(
                    Airbase->brain->FindTakeoffPt(
                        (Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &tx, &ty
                    )
                );
                SetTrackPoint(tx, ty);
                CalculateTaxiSpeed(MoveAlong);

                //CalculateTaxiSpeed(HurryUp);    // 17JAN04 - FRB - expedite takeoff
                if (atcstatus not_eq tTakeRunway)
                {
                    atcstatus = tTakeRunway;
                    SendATCMsg(atcstatus);
                }

                waittimer = CalcWaitTime(Airbase->brain);  // FRB - do we have to wait?
                //waittimer = 0;
            }
            else if (self->af->IsSet(AirframeClass::OverRunway))
            {
                OffsetTrackPoint(50.0F, rightRunway);
                CalculateTaxiSpeed(MoveAlong);
                //CalculateTaxiSpeed(HurryUp);    // 17JAN04 - FRB - expedite takeoff
                waittimer = 0;
            }
            else if (
                PtDataTable[curTaxiPoint].type not_eq TakeRunwayPt and 
 not IsSetATC(PermitTakeRunway) and 
 not self->IsSetFalcFlag(FEC_HOLDSHORT)
            )
            {
                atcstatus = tHoldShort;
                SendATCMsg(atcstatus);
            }

            break;

        case RunwayPt:
        {
            if (
                isWing and 
 not WingmanTakeRunway(Airbase, (AircraftClass*)flightLead, leader) and 
                self->af->IsSet(AirframeClass::OverRunway)
            )
            {
                OffsetTrackPoint(50.0F, rightRunway);
                //CalculateTaxiSpeed(MoveAlong);
                CalculateTaxiSpeed(HurryUp);    // 17JAN04 - FRB - expedite takeoff
                waittimer = 0;
                break;
            }
            else if ( not isWing and not IsSetATC(PermitRunway))
            {
                if (self->af->IsSet(AirframeClass::OverRunway))
                {
                    OffsetTrackPoint(50.0F, rightRunway);
                    CalculateTaxiSpeed(MoveAlong);
                    //CalculateTaxiSpeed(HurryUp);    // 17JAN04 - FRB - expedite takeoff
                    waittimer = 0;
                }

                break;
            }

            //takeoff and get out of the way
            SetATCFlag(PermitRunway);

            if (atcstatus not_eq tTakeRunway and atcstatus not_eq tTakeoff)
            {
                atcstatus = tTakeRunway;
            }

            float tx, ty;
            SetTaxiPoint(
                Airbase->brain->FindRunwayPt(
                    (Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &tx, &ty
                )
            );
            SetTrackPoint(tx, ty);
            desiredSpeed = 30.0F * KNOTS_TO_FTPSEC;
            //waittimer = CalcWaitTime(Airbase->brain);  // FRB - do we have to wait?
            waittimer = 0;
        }
        break;
    }
}

void DigitalBrain::TaxiBack(ObjectiveClass *Airbase)
{
    if (atcstatus not_eq lTaxiOff)
    {
        atcstatus = lTaxiOff;
        SendATCMsg(noATC);
    }

    switch (PtDataTable[curTaxiPoint].type)
    {
        case TakeRunwayPt:
        case TakeoffPt:
        case RunwayPt:
            if (curTaxiPoint)
            {
                SetTaxiPoint(GetNextTaxiPt(curTaxiPoint));
                float tx, ty;
                TranslatePointData(Airbase, curTaxiPoint, &tx, &ty);
                SetTrackPoint(tx, ty);
                OffsetTrackPoint(120.0F, offRight);
            }

            break;
    }

    if (fabs(trackX - af->x) < TAXI_CHECK_DIST and fabs(trackY - af->y) < TAXI_CHECK_DIST)
    {
        SetTaxiPoint(GetNextTaxiPt(curTaxiPoint));

        if (curTaxiPoint)
        {
            float tx, ty;
            TranslatePointData(Airbase, curTaxiPoint, &tx, &ty);
            SetTrackPoint(tx, ty);
            OffsetTrackPoint(120.0F, offRight);
        }
    }

    if (self not_eq SimDriver.GetPlayerEntity())
    {
        if ( not curTaxiPoint or PtDataTable[curTaxiPoint].flags bitand PT_LAST)
        {
            // 02JAN04 - FRB - Make parking spot available for others
            PtDataTable[curTaxiPoint].flags and_eq compl PT_OCCUPIED;
            RegroupAircraft(self);
        }
    }

    if (CheckTaxiTrackPoint())
    {
        OffsetTrackPoint(80.0F, offRight);
    }

    CalculateTaxiSpeed(10.0F);
    SimpleGroundTrack(desiredSpeed);
}

extern bool g_bMPFix;


void DigitalBrain::SendATCMsg(AtcStatusEnum msg)
{
    //atcstatus = msg;
    //hack so we don't send atc messages to taskforces
    CampBaseClass *atc = (CampBaseClass*)vuDatabase->Find(airbase);

    if ( not atc or not atc->IsObjective())
    {
        return;
    }

    FalconATCMessage* ATCMessage;

    if (g_bMPFix)
    {
        ATCMessage = new FalconATCMessage(
            airbase, (VuTargetEntity*) vuDatabase->Find(vuLocalSessionEntity->Game()->OwnerId())
        );
    }
    else
    {
        ATCMessage = new FalconATCMessage(airbase, FalconLocalGame);
    }

    ATCMessage->dataBlock.from = self->Id();
    ATCMessage->dataBlock.status = (short)msg;

    switch (msg)
    {
        case lReqClearance:
            ATCMessage->dataBlock.type = FalconATCMessage::RequestClearance;
            break;

        case lTakingPosition:
            ATCMessage->dataBlock.type = FalconATCMessage::ContactApproach;
            break;

        case lReqEmerClearance:
            ATCMessage->dataBlock.type = FalconATCMessage::RequestEmerClearance;
            break;

        case tReqTaxi:
            ATCMessage->dataBlock.type = FalconATCMessage::RequestTaxi;
            break;

        case tReqTakeoff:
            ATCMessage->dataBlock.type = FalconATCMessage::RequestTakeoff;
            break;

        case tEmerStop:
        case lAborted:
        case lIngressing:
        case lHolding:
        case lFirstLeg:
        case lToBase:
        case lToFinal:
        case lOnFinal:
        case lLanded:
        case lTaxiOff:
        case lEmerHold:
        case lEmergencyToBase:
        case lEmergencyToFinal:
        case lEmergencyOnFinal:
        case lCrashed:
        case tTaxi:
        case tHoldShort:
        case tPrepToTakeRunway:
        case tTakeRunway:
        case tTakeoff:
        case tFlyOut:
        case noATC:
        case tTaxiBack:
            ATCMessage->dataBlock.type = FalconATCMessage::UpdateStatus;
            break;

        default:
            //we shouldn't get here
            ShiWarning("Sending unknown ATC message type");
    }

    FalconSendMessage(ATCMessage, TRUE);
}

float DigitalBrain::CalculateTaxiSpeed(float time)  // to T/O  Need to fix TaxiPt intervals in AB PDs
{
    ObjectiveClass *Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);

    ShiAssert(Airbase);

    float prevX, prevY, dx, dy;
    //float nextX, nextY;
    int point;
    // point = GetNextPt(curTaxiPoint);
    point = GetNextTaxiPt(curTaxiPoint); // 07JAN04 - FRB - Get *taxi* point we are at now

    if (point and time)
    {
        //TranslatePointData(Airbase, curTaxiPoint, &nextX, &nextY);
        TranslatePointData(Airbase, point, &prevX, &prevY);
        //dx = prevX - nextX;
        //dy = prevY - nextY;
        dx = prevX - trackX;
        dy = prevY - trackY;

        //how fast do we go to cover the distance in (time) seconds?
        desiredSpeed = (float)sqrt(dx * dx + dy * dy) / time;

        ShiAssert( not _isnan(desiredSpeed))
    }
    else
        desiredSpeed = 5.0F * KNOTS_TO_FTPSEC;

    //no matter how late, we don't taxi at more than 30 knots or less than 5
    desiredSpeed = max(5.0F * KNOTS_TO_FTPSEC, min(desiredSpeed, 24.0F * KNOTS_TO_FTPSEC)); //RAS - 10Oct04 - changed from 30 to 24 max based on Booster's input
    // desiredSpeed = max(10.0F*KNOTS_TO_FTPSEC, min(desiredSpeed,30.0F*KNOTS_TO_FTPSEC));  // 24JAN04 - FRB - was 5 min

    return desiredSpeed;
}

void DigitalBrain::OffsetTrackPoint(float offDist, int dir)
{
    float dx = 0.0F, dy = 0.0F, dist = 0.0F, relx = 0.0F, x1 = 0.0F, y1 = 0.0F;
    float cosHdg = 1.0F, sinHdg = 0.0F;
    int point = 0;
    float tmpX = 0.0F, tmpY = 0.0F;
    ObjectiveClass *Airbase = NULL;
    runwayStatsStruct *runwayStats = NULL;

    if (dir == centerRunway)
    {
        Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);

        if (Airbase)
        {
            int queue = GetQueue(rwIndex);
            runwayStats = Airbase->brain->GetRunwayStats();
            float length = runwayStats[queue].halfheight;
            //TranslatePointData(Airbase, pt, &x1, &y1);
            x1 = runwayStats[queue].centerX;
            y1 = runwayStats[queue].centerY;

            dx = x1 - self->XPos();
            dy = y1 - self->YPos();

            relx = (PtHeaderDataTable[rwIndex].cosHeading * dx +
                    PtHeaderDataTable[rwIndex].sinHeading * dy);

            relx = max(min(relx, length - TAXI_CHECK_DIST), 0.0F);

            SetTrackPoint(
                x1 - relx * PtHeaderDataTable[rwIndex].cosHeading,
                y1 - relx * PtHeaderDataTable[rwIndex].sinHeading
            );
        }

        return;
    }

    dx = trackX - self->XPos();
    dy = trackY - self->YPos();
    dist = (float)sqrt(dx * dx + dy * dy);

    //these are cos and sin of hdg to offset point along
    switch (dir)
    {
        case offForward: //forward
            cosHdg = dx / dist;
            sinHdg = dy / dist;
            break;

        case offRight: //right
            cosHdg = -dy / dist;
            sinHdg = dx / dist;
            break;

        case offBack: //back
            cosHdg = -dx / dist;
            sinHdg = -dy / dist;
            break;

        case offLeft: //left
            cosHdg = dy / dist;
            sinHdg = -dx / dist;
            break;

        case downRunway:
            cosHdg = PtHeaderDataTable[rwIndex].cosHeading;
            sinHdg = PtHeaderDataTable[rwIndex].sinHeading;
            break;

        case upRunway:
            cosHdg = -PtHeaderDataTable[rwIndex].cosHeading;
            sinHdg = -PtHeaderDataTable[rwIndex].sinHeading;
            break;

        case rightRunway:
            cosHdg = -PtHeaderDataTable[rwIndex].sinHeading;
            sinHdg = PtHeaderDataTable[rwIndex].cosHeading;
            break;

        case leftRunway:
            cosHdg = PtHeaderDataTable[rwIndex].sinHeading;
            sinHdg = -PtHeaderDataTable[rwIndex].cosHeading;
            break;

        case taxiLeft:
            Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);

            if (Airbase)
            {
                point = GetPrevPtLoop(curTaxiPoint);
                TranslatePointData(Airbase, point, &tmpX, &tmpY);
                dx = tmpX - trackX;
                dy = tmpY - trackY;
                dist = (float)sqrt(dx * dx + dy * dy);
                cosHdg = dy / dist;
                sinHdg = -dx / dist;
            }

            break;

        case taxiRight:
            Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);

            if (Airbase)
            {
                if (PtDataTable[curTaxiPoint].type == RunwayPt)
                    point = GetNextPtLoop(curTaxiPoint);
                else
                    point = GetPrevPtLoop(curTaxiPoint);

                TranslatePointData(Airbase, point, &tmpX, &tmpY);
                dx = tmpX - trackX;
                dy = tmpY - trackY;
                dist = (float)sqrt(dx * dx + dy * dy);
                cosHdg = -dy / dist;
                sinHdg = dx / dist;
            }

            break;
    }

    SetTrackPoint(trackX + cosHdg * offDist, trackY + sinHdg * offDist);
}

int DigitalBrain::FindDesiredTaxiPoint(ulong takeoffTime, int rwIndx)
{
    rwIndex = rwIndx;
    return FindDesiredTaxiPoint(takeoffTime);
}

// FRB - Spawn point  Assumes the PDs are equidistant (not always true)
int DigitalBrain::FindDesiredTaxiPoint(ulong takeoffTime)
{
    // sfr: check for no runway
    if (rwIndex == 0)
    {
        return -1;
    }

    // in TAKEOFF_TIME_DELTA (10) seconds blocks
    int time_til_takeoff = 0, tp, prevPt, parkPt;

    if (takeoffTime > SimLibElapsedTime)
    {
        time_til_takeoff = (takeoffTime - Camp_GetCurrentTime()) / (TAKEOFF_TIME_DELTA);
    }

    if (time_til_takeoff < 0)
    {
        time_til_takeoff = 0;
    }

    tp = GetFirstPt(rwIndex);

    // Cobra - Hack to get the fighter a/c leaders on the front row of parking area
    if ((af->GetParkType() == SmallParkPt) and (tp = GetNextParkTypePt(tp, af->GetParkType())))
    {
        PtDataTable[tp].flags or_eq PT_OCCUPIED; // 02JAN04 - FRB - Block use of first parking spot
    }

    tp = GetFirstPt(rwIndex);
    prevPt = tp + 1;
    parkPt = -1; // FRB
    tp = GetNextPt(tp);

    // while (tp and time_til_takeoff) // Cobra - Taxi Mode puts a/c in parking and taxiway spots
    while (tp)
    {
        if (PtDataTable[tp].type == CritTaxiPt)
        {
            break;
        }

        prevPt = tp;
        tp = GetNextPt(tp); // FRB - Look at all of them

        // 17JAN04 - FRB - Locate a suitable parking spot
        if ((PtDataTable[tp].type == SmallParkPt) or (PtDataTable[tp].type == LargeParkPt))
        {
            if (PtDataTable[tp].flags bitand PT_OCCUPIED)
            {
                time_til_takeoff--;
                continue;  // Taken
            }
            else if (af->GetParkType() == PtDataTable[tp].type)
            {
                parkPt = tp;
                time_til_takeoff--;
                break;
            }
            else
            {
                time_til_takeoff--;
                continue;  // Taken
            }
        }

        time_til_takeoff--;
    }

    // Cobra - Adjust taxi time
    // if (time_til_takeoff > (takeoffTime-Camp_GetCurrentTime())/(TAKEOFF_TIME_DELTA))
    // 17JAN04 - FRB - Use nearest Parking spot
    if (parkPt >= 0)
    {
        tp = parkPt;
        PtDataTable[tp].flags or_eq PT_OCCUPIED; // 02JAN04 - FRB - Reserve parking spot
        SetTaxiPoint(self->spawnpoint = tp);
        return tp;
    }
    else if (tp)
    {
        SetTaxiPoint(self->spawnpoint = tp);
        return tp;
    }
    else
    {
        self->spawnpoint = -1;
    }

    return -1;
}

int DigitalBrain::CalcWaitTime(ATCBrain *Atc)
{
    VU_TIME count = 0;
    VU_TIME time = rwtime;

    switch (atcstatus)
    {
        case tPrepToTakeRunway:
        case tTakeRunway:
        case tTakeoff:
            time = SimLibElapsedTime + static_cast<VU_TIME>(MoveAlong * CampaignSeconds);  // FRB - 5 ==> MoveAlong
            break;

        case tEmerStop:
            time = SimLibElapsedTime + 2 * TAKEOFF_TIME_DELTA;
            break;

        case tHoldShort:
        case tReqTaxi:
        case tReqTakeoff:
            // time = SimLibElapsedTime + TAKEOFF_TIME_DELTA;  // FRB - elapse time 10 sec???
            time = SimLibElapsedTime + static_cast<VU_TIME>(MoveAlong * CampaignSeconds);  // FRB - 2
            break;

        case tTaxi:
        default:

            count = GetTaxiPosition(curTaxiPoint, rwIndex);

            if (rwtime > count * (MoveAlong * CampaignSeconds) + SimLibElapsedTime)
            {
                time = ((rwtime - (count * (VU_TIME)(MoveAlong * CampaignSeconds) + SimLibElapsedTime)) + SimLibElapsedTime);   // FRB - (count * TAKEOFF_TIME_DELTA) is delta time
                time -= (VU_TIME)(g_fTaxiEarly * CampaignSeconds); //RAS - 10Oct04 - Added hold short delay to start taxi sooner

                if (time < SimLibElapsedTime)
                    time = SimLibElapsedTime;
            }

            else if (PtDataTable[curTaxiPoint].type <= TakeoffPt) //        rwtime is Sim clock time

                time = SimLibElapsedTime + (VU_TIME)(MoveAlong * CampaignSeconds);
            else
                time = SimLibElapsedTime + (VU_TIME)(HurryUp * CampaignSeconds);
    }

    return time;
}

int DigitalBrain::ReadyToGo(void)
{
    int retval = FALSE, runway;
    FlightClass *flight = (FlightClass*)self->GetCampaignObject();
    ObjectiveClass *Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);
    AircraftClass *wingman = NULL;

    if ( not Airbase)
        return TRUE;

    if ( not isWing and not IsSetATC(PermitTakeoff))
        return FALSE;

    if (( not isWing or self->vehicleInUnit == 2) and 
        rwtime + WINGMAN_WAIT_TIME < SimLibElapsedTime and // FRB - check WINGMAN_WAIT_TIME value (is 30 secs)
        waittimer <= SimLibElapsedTime)
    {
        retval = TRUE;
    }
    else if (self->GetVt() < 2.0F and waittimer <= SimLibElapsedTime)
    {
        if (Airbase->brain->UseSectionTakeoff(flight, rwIndex))
        {
            wingman = (AircraftClass*)MyWingman();

            if (wingman and Airbase->brain)
            {
                runway = Airbase->brain->IsOnRunway(wingman);

                if ( not wingman->OnGround())
                    retval = TRUE;
                else if (wingman->GetVt() > 40.0F * KNOTS_TO_FTPSEC and (runway == rwIndex or Airbase->brain->GetOppositeRunway(runway) == rwIndex))
                    retval = TRUE;

                if (isWing == 0 or self->vehicleInUnit == 2)
                {
                    if (wingman->GetVt() < 2.0F and (runway == rwIndex or Airbase->brain->GetOppositeRunway(runway) == rwIndex))
                        retval = TRUE;
                }
            }
            else
                retval = TRUE;
        }
        else
            retval = TRUE;
    }

    if (wingman and retval and not IsSetATC(WingmanReady))
    {
        SetATCFlag(WingmanReady);
        waittimer = CampaignSeconds + SimLibElapsedTime;

        retval = FALSE;
    }

    return retval;
}


/*
** Name: CheckTaxiTrackPoint
** Description:
** Looks to see if another object is occupying our next tracking point
** and returns TRUE if so.
*/
SimBaseClass* DigitalBrain::CheckTaxiTrackPoint(void)
{
    return CheckPoint(trackX, trackY);
}

SimBaseClass* DigitalBrain::CheckPoint(float x, float y)
{
    return CheckTaxiPointGlobal(self, x, y);
}



/*
** Name: CheckTaxiCollision  // Function not used
** Description:
** Looks out from our current heading to see if we're going to collide
** with anything.  If so returns TRUE.
*/
BOOL DigitalBrain::CheckTaxiCollision(void)
{
    Tpoint org, pos, vec;
    float rangeSquare;
    SimBaseClass* testObject;
    CampBaseClass* campBase;
#ifdef VU_GRID_TREE_Y_MAJOR
    VuGridIterator gridIt(RealUnitProxList, af->y, af->x, NM_TO_FT * 3.0F);
#else
    VuGridIterator gridIt(RealUnitProxList, af->x, af->y, NM_TO_FT * 3.0F);
#endif


    // Cobra - No on-ground collisions for "large" aircraft.
    // if (self->af->GetParkType() == LargeParkPt)
    // return FALSE;

    // get the 1st objective that contains the bomb
    campBase = (CampBaseClass*) gridIt.GetFirst();

    // main loop through campaign units
    while (campBase)
    {
        // skip campaign unit if no sim components
        if ( not campBase->GetComponents())
        {
            campBase = (CampBaseClass*) gridIt.GetNext();
            continue;
        }

        // loop thru each element in the objective
        VuListIterator unitWalker(campBase->GetComponents());
        testObject = (SimBaseClass*) unitWalker.GetFirst();

        while (testObject)
        {
            // ignore objects under these conditions:
            // Ourself
            // Not on ground
            // no drawpointer
            if ( not testObject->OnGround() or
                testObject == self or
 not testObject->drawPointer)
            {
                testObject = (SimBaseClass*) unitWalker.GetNext();
                continue;
            }

            // range from tracking point to object
            pos.x = testObject->XPos() - af->x;
            pos.y = testObject->YPos() - af->y;
            rangeSquare = pos.x * pos.x + pos.y * pos.y;

            // if object is greater than given range don't check
            // also, perhaps a degenerate case, if too close and overlapping
            if (rangeSquare > 80.0f * 80.0f or rangeSquare < 10.0f * 10.0f)
            {
                testObject = (SimBaseClass*) unitWalker.GetNext();
                continue;
            }

            // origin of ray
            rangeSquare = self->drawPointer->Radius();
            org.x = af->x + self->dmx[0][0] * rangeSquare * 1.1f;
            org.y = af->y + self->dmx[0][1] * rangeSquare * 1.1f;
            org.z = af->z + self->dmx[0][2] * rangeSquare * 1.1f;

            // vector of ray
            vec.x = self->dmx[0][0] * 80.0f;
            vec.y = self->dmx[0][1] * 80.0f;
            vec.z = self->dmx[0][2] * 80.0f;

            // do ray, box intersection test
            if (testObject->drawPointer->GetRayHit(&org, &vec, &pos, 1.0f))
            {
                return TRUE;
            }

            testObject = (SimBaseClass*) unitWalker.GetNext();
        }

        // get the next objective that contains the bomb
        campBase = (CampBaseClass*) gridIt.GetNext();

    } // end objective loop

    return FALSE;
}


/*
** Name: SimpleGroundTrack
** Description:
** Wraps Simple Track Speed.  Then it checks objects around it and
** uses a potential field to modify the steering to (hopefully)
** avoid other objects.
*/
BOOL DigitalBrain::SimpleGroundTrack(float speed)
{
    float tmpX, tmpY;
    float rx, ry;
    float az, azErr;
    float stickError;
    SimBaseClass* testObject;
    int numOnLeft, numOnRight;
    float myRad, testRad, xft, yft, dist;
    float minAz = 1000.0f;
    bool BigBoy = false;

    if (speed > 0.0f) //RAS - 14Oct04 - Keep AI from rolling if they aren't supposed to, if Player can't move, then change this
        af->ClearFlag(AirframeClass::WheelBrakes);  //RAS - may need to add this line to playerEntity section below

    xft = trackX - af->x;
    yft = trackY - af->y;
    // get relative position and az
    rx = self->dmx[0][0] * xft + self->dmx[0][1] * yft;
    ry = self->dmx[1][0] * xft + self->dmx[1][1] * yft;

    if (self == SimDriver.GetPlayerEntity() and IsSetATC(StopPlane))
    {
        // call simple track to set the stick
        if (rx < 10.0F)
        {
            // int taxiPoint = GetPrevPtLoop(curTaxiPoint);
            int taxiPoint = GetPrevTaxiPt(curTaxiPoint); // 03JAN04 - FRB - Skip Parking spots

            if ( not taxiPoint)
                taxiPoint = GetPrevPtLoop(curTaxiPoint);

            ObjectiveClass *Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);

            TranslatePointData(Airbase, taxiPoint, &tmpX, &tmpY);
            xft = tmpX - af->x;
            yft = tmpY - af->y;
            // get relative position and az
            rx = self->dmx[0][0] * xft + self->dmx[0][1] * yft;
            ry = self->dmx[1][0] * xft + self->dmx[1][1] * yft;

            if (gameCompressionRatio)
                // JB 020315 Why divide by gameCompressionRation?  It screws up taxing for one thing.
                rStick = SimpleTrackAzimuth(rx , ry, self->GetVt());///gameCompressionRatio;
            else
                rStick = 0.0f;

            pStick = 0.0F;
            throtl = 0.0F;
            af->vt = 0.0f;
            af->SetFlag(AirframeClass::WheelBrakes);
        }
        else
            TrackPoint(0.0f, 0.0F);

        return FALSE;
    }

    if (atcstatus == tTakeoff)
    {
        // once we're taking off just do it....
        //aim for a five degree climb
        af->LEFTakeoff();
        af->TEFTakeoff();
        rStick = SimpleTrackAzimuth(rx + 1000.0F, ry, self->GetVt());

        pStick = 5 * DTR;
        throtl = 1.5f;
    }
    else
    {
        // cheat a bit so we don't chase around in a circle
        // if we're getting close to our track point, slow and
        // rotate towards it

        dist = xft * xft + yft * yft;

        if (dist < TAXI_CHECK_DIST * TAXI_CHECK_DIST)  // * 4.0F) // Cobra - 120' may be not close enough
        {
            az = (float) atan2(ry, rx);

            if (fabs(az) > 30.0f * DTR)
                speed *= 0.5f;
        }

        // call simple track to set the stick
        TrackPoint(0.0f, speed);
    }

    if (speed == 0.0f)
    {
        // if no speed we're done
        if (rx < 10.0F)
        {
            // int taxiPoint = GetPrevPtLoop(curTaxiPoint);
            int taxiPoint = GetPrevTaxiPt(curTaxiPoint); // 03JAN04 - FRB - Skip Parking spots

            if ( not taxiPoint)
                taxiPoint = GetPrevPtLoop(curTaxiPoint);

            ObjectiveClass *Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);

            TranslatePointData(Airbase, taxiPoint, &tmpX, &tmpY);
            xft = tmpX - af->x;
            yft = tmpY - af->y;
            // get relative position and az
            rx = self->dmx[0][0] * xft + self->dmx[0][1] * yft;
            ry = self->dmx[1][0] * xft + self->dmx[1][1] * yft;

            if (gameCompressionRatio)
                // JB 020315 Why divide by gameCompressionRation?  It screws up taxing for one thing.
                rStick = SimpleTrackAzimuth(rx , ry, self->GetVt());///gameCompressionRatio;
            else
                rStick = 0.0f;

            pStick = 0.0F;
        }

        return FALSE;
    }

    // Cobra - "large" aircraft?
    if (self->af->GetParkType() == LargeParkPt)
        BigBoy = true;

    if ( not self->OnGround())
        return FALSE;

    // init the stick error
    stickError = 0.0f;
    numOnLeft = 0;
    numOnRight = 0;

    if (self->drawPointer)
        myRad = self->drawPointer->Radius();
    else
        myRad = 40.0f;

    // Cobra - Tell the big boys they are really not that big
    if (BigBoy)
        myRad = 10.f;

    // loop thru all sim objects
    {
        VuListIterator unitWalker(SimDriver.objectList);
        testObject = (SimBaseClass*) unitWalker.GetFirst();

        while (testObject)
        {
            // ignore objects under these conditions:
            // Ourself
            // Not on ground
            if ( not testObject->OnGround() or
                testObject == self)
            {
                testObject = (SimBaseClass*) unitWalker.GetNext();
                continue;
            }

            // range from us to object
            tmpX = testObject->XPos() - af->x;
            tmpY = testObject->YPos() - af->y;

            // Cobra - if too far to be in the way, skip it
            if ((tmpX * tmpX + tmpY * tmpY) > (200.f * 200.f))
            {
                testObject = (SimBaseClass*) unitWalker.GetNext();
                continue;
            }

            if (testObject->drawPointer)
                testRad = testObject->drawPointer->Radius() + myRad;
            else
                testRad = 40.0f + myRad;

            dist = (float)sqrt(tmpX * tmpX + tmpY * tmpY);
            float range = dist - testRad - MAX_RANGE_COLL;

            //rangeSquare = tmpX*tmpX + tmpY*tmpY - testRad * testRad - MAX_RANGE_SQ;

            // if object is greater than 2 x max range continue to next
            //if ( rangeSquare > MAX_RANGE_SQ )
            if (range > MAX_RANGE_COLL) // MAX_RANGE_COLL = 20'
            {
                testObject = (SimBaseClass*) unitWalker.GetNext();
                continue;
            }

            // get relative position and az
            rx = self->dmx[0][0] * tmpX + self->dmx[0][1] * tmpY;
            ry = self->dmx[1][0] * tmpX + self->dmx[1][1] * tmpY;

            az = (float) atan2(ry, rx);

            // reject anything more than MAX_AZ deg off our nose
            // Cobra - Restrict large a/c to narrow view
            if ((BigBoy and (fabs(az) > 10.f * DTR)) or
                (fabs(az) > MAX_AZ))
            {
                testObject = (SimBaseClass*) unitWalker.GetNext();
                continue;
            }

            /*
             // Cobra - Skip a/c on parking spots
             if (((AircraftClass*)testObject)->DBrain() and 
             (((AircraftClass*)testObject)->DBrain()->GetTaxiPoint() == LargeParkPt or
             ((AircraftClass*)testObject)->DBrain()->GetTaxiPoint() == SmallParkPt))
             {
             testObject = (SimBaseClass*) unitWalker.GetNext();
             continue;
             }
            */
            if (rx > 0.0F and fabs(ry) > testRad and range < 0.0F and 
                testObject->GetCampaignObject() == self->GetCampaignObject() and 
                self->vehicleInUnit > ((AircraftClass*)testObject)->vehicleInUnit)
            {
                xft = trackX - af->x;
                yft = trackY - af->y;
                // get relative position and az
                rx = self->dmx[0][0] * xft + self->dmx[0][1] * yft;
                ry = self->dmx[1][0] * xft + self->dmx[1][1] * yft;

                if (rx < 10.0F)
                {
                    // int taxiPoint = GetPrevPtLoop(curTaxiPoint);
                    int taxiPoint = GetPrevTaxiPt(curTaxiPoint); // 03JAN04 - FRB - Skip Parking spots

                    if ( not taxiPoint)
                        taxiPoint = GetPrevPtLoop(curTaxiPoint);

                    ObjectiveClass *Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);

                    TranslatePointData(Airbase, taxiPoint, &tmpX, &tmpY);
                    xft = tmpX - af->x;
                    yft = tmpY - af->y;
                    // get relative position and az
                    rx = self->dmx[0][0] * xft + self->dmx[0][1] * yft;
                    ry = self->dmx[1][0] * xft + self->dmx[1][1] * yft;

                    if (gameCompressionRatio)
                        // JB 020315 Why divide by gameCompressionRation?  It screws up taxing for one thing.
                        rStick = SimpleTrackAzimuth(rx , ry, self->GetVt());///gameCompressionRatio;
                    else
                        rStick = 0.0f;

                    pStick = 0.0F;
                    throtl = SimpleGroundTrackSpeed(0.0F);
                }
                else
                    TrackPoint(0.0f, speed);

                return TRUE;
            }

            if (fabs(az) < fabs(minAz))
            {
                minAz = az;
            }

            // have we reached a situation where it's impossible to
            // move forward without colliding?
            if (fabs(az) < 80.0F * DTR and rx > 5.0f and rx < testRad * 1.5F and fabs(ry) < testRad * 1.25F)
            {
                // count the number of blocks to our left and right
                // to be used later
                if (ry > 0.0f)
                    numOnRight++;
                else
                    numOnLeft++;

                testObject = (SimBaseClass*) unitWalker.GetNext();
                continue;
            }

            // OK, we've got an object in front of our 3-6 line and
            // within range.  The potential field will work by deflecting
            // our nose (rstick).  The closer the object and the more towards
            // our nose, the stronger is the deflection


            if (az > 0.0f)
                azErr = -1.0F + ry / dist;
            else if (az < 0.0f)
                azErr = 1.0F + ry / dist;
            else if (rStick > 0.0f)
                azErr = 1.0f;
            else
                azErr = -1.0f;

            // this deflection is now modulated by the proximity -- the closer
            // the stronger the force
            // weight the range higher
            azErr *= (MAX_RANGE_COLL  - range) / (MAX_RANGE_COLL * 0.4F);

            // now accumulate the stick error
            stickError += azErr;


            testObject = (SimBaseClass*) unitWalker.GetNext();
        }
    }
    //===================================

    // test blockages directly in front
    // cheat: if we get stuck just rotate in place
    if ((numOnLeft or numOnRight) and fabs(minAz) > 70.0f * DTR)
    {
        rStick = 0.0f;
        pStick = 0.0f;
        throtl *= 0.5f;
        tiebreaker = 0;
    }
    else if (tiebreaker > 10)
    {
        af->sigma -= 10.0f * DTR * SimLibMajorFrameTime;
        af->SetFlag(AirframeClass::WheelBrakes);
        throtl = 0.0f;

        if (tiebreaker ++ - 10 > rand() % 20)
            tiebreaker = 0;

        return TRUE;
    }
    else if (numOnLeft and numOnRight)
    {
        if (fabs(minAz) > 50.0f * DTR)
        {
            rStick = 0.0f;
            pStick = 0.0f;
            throtl *= 0.5f;
        }
        else
        {
            af->sigma -= 10.0f * DTR * SimLibMajorFrameTime;
            af->SetFlag(AirframeClass::WheelBrakes);
            throtl = 0.0f;
        }

        tiebreaker ++;
        return TRUE;
    }
    else if (numOnRight)
    {
        af->sigma -= 10.0f * DTR * SimLibMajorFrameTime;
        af->SetFlag(AirframeClass::WheelBrakes);
        throtl = 0.0f;
        tiebreaker ++;
        return TRUE;
    }
    else if (numOnLeft)
    {
        af->sigma += 10.0f * DTR * SimLibMajorFrameTime;
        af->SetFlag(AirframeClass::WheelBrakes);
        throtl = 0.0f;
        tiebreaker ++;
        return TRUE;
    }

    tiebreaker = 0;
    // we now apply the stick error to rstick
    // make sure we clamp rstick
    rStick += stickError;

    if (rStick > 1.0f)
        rStick = 1.0f;
    else if (rStick < -1.0f)
        rStick = -1.0f;

    // readjust throttle if our stick error is large
    if (fabs(stickError) > 0.5f)
    {
        throtl = SimpleGroundTrackSpeed(speed * 0.75f); // speed = desired speed (ft/sec)
    }

    return FALSE;
}

void DigitalBrain::UpdateTaxipoint(void)  // Called by Airframe.cpp, AircraftInputs.cpp
{
    // Not used by Landme.cpp

    // if(( not self->OnGround()) or (atcstatus == tTakeRunway))  // 07FEB04 - FRB - added atcstatus check
    //RAS-12Nov04-If your a player then if we're on the ground, we need to update our taxi point
    if (( not self->OnGround()) or not self->IsPlayer() and (atcstatus == tTakeRunway))
        return;

    ObjectiveClass *Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);
    float tmpX, tmpY, dx, dy, dist, closestDist =  4000000.0F;
    int taxiPoint, i, closest = curTaxiPoint;

    // SetDebugLabel(Airbase);

    taxiPoint = GetPrevPtLoop(curTaxiPoint);

    // Cobra - Move 2 Pt's forward to make sure we are pass an adjacent parking pt.
    // taxiPoint = GetPrevPtLoop(taxiPoint);
    // for(i = 0; i < 5; i++)
    for (i = 0; i < 3; i++)
    {
        TranslatePointData(Airbase, taxiPoint, &tmpX, &tmpY);
        dx = tmpX - af->x;
        dy = tmpY - af->y;
        dist = dx * dx + dy * dy;

        if (dist < closestDist)
        {
            closestDist = dist;
            closest = taxiPoint;
        }

        taxiPoint = GetNextPtLoop(taxiPoint);
    }

    if (closest not_eq curTaxiPoint)
    {
        // Cobra - Skip looking at parking pts.
        // if(closest == GetNextPtLoop(curTaxiPoint) and self->AutopilotType() == AircraftClass::APOff)
        if (closest == GetNextTaxiPt(curTaxiPoint) and self->AutopilotType() == AircraftClass::APOff)
        {
            if (IsSetATC(CheckTaxiBack) and atcstatus not_eq tTaxiBack)
            {
                atcstatus = tTaxiBack;
                SendATCMsg(atcstatus);
            }
            else
                SetATCFlag(CheckTaxiBack);
        }
        // Cobra - Skip looking at parking pts.
        // else if(closest == GetPrevPtLoop(curTaxiPoint) and atcstatus == tTaxiBack)
        else if (closest == GetPrevTaxiPt(curTaxiPoint) and atcstatus == tTaxiBack)
        {
            if (IsSetATC(PermitRunway))
                atcstatus = tTakeRunway;
            else
                atcstatus = tTaxi;

            SendATCMsg(atcstatus);
            ClearATCFlag(CheckTaxiBack);
        }

        SetTaxiPoint(closest);
        float tx, ty;
        TranslatePointData(Airbase, curTaxiPoint, &tx, &ty);
        SetTrackPoint(tx, ty);
        CalculateTaxiSpeed(MoveAlong);
        waittimer = CalcWaitTime(Airbase->brain);
    }
}


AircraftClass* DigitalBrain::GetLeader(void)
{
    VuEntity *cur;
    VuEntity *prev = NULL;

    VuListIterator cit(self->GetCampaignObject()->GetComponents());
    cur = cit.GetFirst();

    while (cur)
    {
        if (cur == self)
        {
            if (prev)
                return ((AircraftClass*)prev);
            else
                return NULL;
        }

        prev = cur;

        cur = cit.GetNext();
    }

    return NULL;
}

// JPO - whole bunch of preflight stuff here.
typedef int (*PreFlightFnx)(AircraftClass *self);
//Cobra 10/31/04 TJL copied over whole enginestart function
static int EngineStart(AircraftClass *self)
{
    if (self->af->IsSet(AirframeClass::EngineStopped))
    {
        /* RAS - not sure if this has any side effects so commenting it out for now
         //RAS-5Nov04-Set Parking Brake when Starting Engines
         if( not self->af->IsSet((AirframeClass::WheelBrakes)))
        */
        self->af->SetFlag(AirframeClass::WheelBrakes);

        if /*(self->af->rpm > 0.2f)*/ (self->af->IsSet(AirframeClass::JfsStart))   // Jfs is runnning time to light
        {
            self->af->SetThrotl(0.1f); // advance the throttle to light.
            self->af->ClearFlag(AirframeClass::EngineStopped);
            self->af->engine1Throttle = 0.3F;

        }
    }

    //TJL 01/22/04 multi-engine
    if (self->af->IsEngineFlag(AirframeClass::EngineStopped2))
    {
        if /*(self->af->rpm2 > 0.2f)*/(self->af->IsSet(AirframeClass::JfsStart))
        {
            self->af->SetThrotl(0.1f);
            self->af->ClearEngineFlag(AirframeClass::EngineStopped2);
            self->af->engine2Throttle = 0.3F;
        }
    }

    /* //TJL 08/15/04 Trying to fix this
     if (self->af->IsSet(AirframeClass::JfsStart))
     {
     // nothing much happening
     }
     else { // start the JFS
         self->af->JfsEngineStart();
     return 0;
    */

    //RAS-5Nov04-Fix for Ramp Start Preflight (All items would not get accomplished because the engine
    //start routine was looping in the JfsEngineStart() function
    if (self->af->IsSet(AirframeClass::EngineStopped))
    {
        self->af->JfsEngineStart();
        return 0;
    }


    if (self->af->rpm > 0.69f)   // at idle
    {
        self->af->SetThrotl(0.0F);
        self->af->engine1Throttle = 0.0F;
        self->af->engine2Throttle = 0.0F;
        self->DBrain()->throtl = 0.0F;
        return 1; // finished engine start up
    }
    else if (self->af->rpm > 0.5f)   // engine now running
    {
        self->af->SetThrotl(0.0F);
        self->af->engine1Throttle = 0.0F;
        self->af->engine2Throttle = 0.0F;
    }

    return 0;
    /*

    }
    else {
    */
}
static struct PreflightActions
{
    enum { CANOPY, FUEL1, FUEL2, FNX, RALTON, POWERON, AFFLAGS, RADAR, SEAT, MAINPOWER, EWSPGM,
           MASTERARM, EXTLON, INS, VOLUME, FLAPS
         };
    int action; // what to do,
    int data; // any data
    int timedelay; // how many seconds til next one
    PreFlightFnx fnx;
} PreFlightTable[] =
{
    { PreflightActions::CANOPY, 0, 1, NULL},
    { PreflightActions::FUEL1, 0, 1, NULL},
    { PreflightActions::FUEL2, 0, 1, NULL},
    { PreflightActions::MAINPOWER, 0, 1, NULL},
    { PreflightActions::FNX, 0, 0, EngineStart},
    { PreflightActions::RALTON, 0, 1, NULL },
    { PreflightActions::POWERON, AircraftClass::HUDPower, 6, NULL},
    { PreflightActions::POWERON, AircraftClass::MFDPower, 2, NULL},
    { PreflightActions::POWERON, AircraftClass::FCCPower, 1, NULL},
    { PreflightActions::POWERON, AircraftClass::SMSPower, 1, NULL},
    { PreflightActions::POWERON, AircraftClass::UFCPower, 1, NULL},
    { PreflightActions::POWERON, AircraftClass::FCRPower, 3, NULL},
    { PreflightActions::POWERON, AircraftClass::TISLPower, 1, NULL},
    { PreflightActions::POWERON, AircraftClass::LeftHptPower, 1, NULL},
    { PreflightActions::POWERON, AircraftClass::RightHptPower, 1, NULL},
    { PreflightActions::POWERON, AircraftClass::GPSPower, 1, NULL},
    { PreflightActions::POWERON, AircraftClass::DLPower, 1, NULL},
    { PreflightActions::POWERON, AircraftClass::MAPPower, 1, NULL},
    { PreflightActions::POWERON, AircraftClass::RwrPower, 1, NULL},
    { PreflightActions::RADAR, 0, 1, NULL},
    { PreflightActions::POWERON, AircraftClass::EWSRWRPower, 2, NULL},
    { PreflightActions::POWERON, AircraftClass::EWSJammerPower, 2, NULL},
    { PreflightActions::POWERON, AircraftClass::EWSChaffPower, 2, NULL},
    { PreflightActions::POWERON, AircraftClass::EWSFlarePower, 2, NULL},
    { PreflightActions::POWERON, AircraftClass::IFFPower, 2, NULL},
    { PreflightActions::EWSPGM, 0, 1, NULL},
    { PreflightActions::EXTLON, AircraftClass::Extl_Main_Power, 1, NULL}, //exterior lighting
    { PreflightActions::EXTLON, AircraftClass::Extl_Anti_Coll, 1, NULL}, //exterior lighting
    { PreflightActions::EXTLON, AircraftClass::Extl_Wing_Tail, 1, NULL}, //exterior lighting
    { PreflightActions::EXTLON, AircraftClass::Extl_Flash, 1, NULL}, //exterior lighting
    { PreflightActions::INS, AircraftClass::INS_AlignNorm, 485, NULL}, //time to align
    { PreflightActions::INS, AircraftClass::INS_Nav, 0, NULL},
    { PreflightActions::VOLUME, 1, 1, NULL},
    { PreflightActions::VOLUME, 2, 1, NULL},
    { PreflightActions::AFFLAGS, AirframeClass::NoseSteerOn, 3, NULL }, //shortly before taxi
    { PreflightActions::MASTERARM, 0, 1, NULL}, //MasterArm
    { PreflightActions::SEAT, 0, 1, NULL}, //this comes all at the end
    { PreflightActions::FLAPS, 0, 1, NULL},
};
static const int MAX_PF_ACTIONS = sizeof(PreFlightTable) / sizeof(PreFlightTable[0]);

// JPO - go through start up steps.
int DigitalBrain::PreFlight()
{
    ShiAssert(af not_eq NULL);

    if (SimLibElapsedTime < mNextPreflightAction) return 0;

    switch (PreFlightTable[mActionIndex].action)
    {
        case PreflightActions::FNX:
            if ((*PreFlightTable[mActionIndex].fnx)(self) == 0)
                return 0;

            mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
            break;

        case PreflightActions::CANOPY:
            if ( not af->canopyState)
            {
                af->canopyState = true;
                mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
            }

            break;

        case PreflightActions::FUEL1:
            if (af->IsEngineFlag(AirframeClass::MasterFuelOff))
            {
                af->ClearEngineFlag(AirframeClass::MasterFuelOff);
                mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
            }

            break;

        case PreflightActions::MAINPOWER:
            if (self->MainPower() not_eq AircraftClass::MainPowerMain)
            {
                self->IncMainPower();
                mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
                return 0; //keep clicking til we get to the right state
            }

            break;

        case PreflightActions::FUEL2:
            af->SetFuelPump(AirframeClass::FP_NORM);
            af->SetFuelSwitch(AirframeClass::FS_NORM);
            af->ClearEngineFlag(AirframeClass::WingFirst);
            af->SetAirSource(AirframeClass::AS_NORM);
            mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
            break;

        case PreflightActions::RALTON:
            if (self->RALTStatus == AircraftClass::ROFF)
            {
                self->RaltOn();
                mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
            }

            break;

        case PreflightActions::AFFLAGS:
            if ( not af->IsSet(PreFlightTable[mActionIndex].data))
            {
                af->SetFlag(PreFlightTable[mActionIndex].data);
                mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
            }

            break;

        case PreflightActions::POWERON:
            if ( not self->PowerSwitchOn((AircraftClass::AvionicsPowerFlags)PreFlightTable[mActionIndex].data))
            {
                self->PowerOn((AircraftClass::AvionicsPowerFlags)PreFlightTable[mActionIndex].data);

                //MI additional check for HUD, now that the dial indicates the status
                if (PreFlightTable[mActionIndex].data == AircraftClass::HUDPower)
                {
                    if (TheHud and self == SimDriver.GetPlayerEntity())
                        TheHud->SymWheelPos = 1.0F;
                }

                mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
            }

            break;

        case PreflightActions::RADAR:
        {
            RadarClass* theRadar = (RadarClass*) FindSensor(self, SensorClass::Radar);

            if (theRadar)
                theRadar->SetMode(RadarClass::AA);

            mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
        }
        break;

        case PreflightActions::SEAT:
        {
            self->SeatArmed = TRUE;
            mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
        }
        break;

        case PreflightActions::EWSPGM:
        {
            self->SetPGM(AircraftClass::Man);
            mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
        }
        break;

        case PreflightActions::MASTERARM:
        {
            self->Sms->SetMasterArm(SMSBaseClass::Arm);
            mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
        }
        break;

        case PreflightActions::EXTLON:
            if ( not self->ExtlState((AircraftClass::ExtlLightFlags)PreFlightTable[mActionIndex].data))
            {
                self->ExtlOn((AircraftClass::ExtlLightFlags)PreFlightTable[mActionIndex].data);
                mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;

            }

            break;

        case PreflightActions::INS:

            //turn on aligning flag
            if (PreFlightTable[mActionIndex].data == AircraftClass::INS_AlignNorm)
            {
                self->INSOff(AircraftClass::INS_PowerOff);

                if (self == SimDriver.GetPlayerEntity() and OTWDriver.pCockpitManager)
                {
                    self->SwitchINSToAlign();
                    self->INSAlignmentTimer = 0.0F;
                    self->HasAligned = FALSE;
                    //Set the UFC
                    OTWDriver.pCockpitManager->mpIcp->ClearStrings();
                    OTWDriver.pCockpitManager->mpIcp->LeaveCNI();
                    OTWDriver.pCockpitManager->mpIcp->SetICPFlag(ICPClass::MODE_LIST);
                    OTWDriver.pCockpitManager->mpIcp->SetICPSecondaryMode(23); //SIX Button, INS Page
                }
            }

            self->INSOn((AircraftClass::INSAlignFlags)PreFlightTable[mActionIndex].data);
            mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;

            if (self->INSState(AircraftClass::INS_Aligned) and self->INSState(AircraftClass::INS_AlignNorm))
                self->INSOff(AircraftClass::INS_AlignNorm);

            if (self == SimDriver.GetPlayerEntity() and OTWDriver.pCockpitManager and 
                PreFlightTable[mActionIndex].data == AircraftClass::INS_Nav)
            {
                //CNI page
                OTWDriver.pCockpitManager->mpIcp->ChangeToCNI();
                //Mark us as aligned
                self->SwitchINSToNav();
            }

            break;

        case PreflightActions::VOLUME:
            if (PreFlightTable[mActionIndex].data == 1)
                self->MissileVolume = 0;
            else if (PreFlightTable[mActionIndex].data == 2)
                self->ThreatVolume = 0;

            break;

        case PreflightActions::FLAPS:
            af->TEFTakeoff();
            af->LEFTakeoff();
            break;

        default:
            ShiWarning("Bad Preflight Table");
            break;
    }

    // force switch positions.
    if (self == SimDriver.GetPlayerEntity() and OTWDriver.pCockpitManager)
        OTWDriver.pCockpitManager->InitialiseInstruments();

    mActionIndex ++;

    if (mActionIndex < MAX_PF_ACTIONS)
        return 0;

    mActionIndex = 0;
    mNextPreflightAction = 0;
    return 1;
}

void DigitalBrain::QuickPreFlight()
{
    ShiAssert(af not_eq NULL and self not_eq NULL);
    self->PreFlight();
}

// JPO - work out if we are further along the time line.
int DigitalBrain::IsAtFirstTaxipoint()
{
    ShiAssert(self not_eq NULL);
    Flight flight = (Flight)self->GetCampaignObject();
    ShiAssert(flight not_eq NULL);
    WayPoint w = flight->GetFirstUnitWP(); // this is takeoff time
    ObjectiveClass *Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);
    ShiAssert(Airbase);

    // so takeoff time, - deag time (taxi point time) - if we're past that - we're ready.
    if (SimLibElapsedTime > w->GetWPDepartureTime() - Airbase->brain->MinDeagTime())
    {
        return 0;
    }
    else
        return 1;

}

