#include "stdhdr.h"
#include "digi.h"
#include "mesg.h"
#include "simveh.h"
#include "flight.h"
#include "camp2sim.h"
#include "Aircrft.h"
#include "msginc/radiochattermsg.h"
#include "otwdrive.h"
#include "tankbrn.h"
#include "navsystem.h"
#include "msginc/tankermsg.h"
#include "airframe.h"
#include "playerop.h"
#include "PilotInputs.h"
#include "Graphics/Include/matrix.h"

// #define FOLLOW_RATE 10.0F //-> externalised for each aircraft (auxaerodata)
Objective FindNearestFriendlyRunway(Team who, GridIndex X, GridIndex Y);
float get_air_speed(float, int);

#include "Graphics/include/drawbsp.h"

extern bool g_bNewRefuelHelp;
extern int g_nShowDebugLabels;
extern bool g_bPutAIToBoom;

extern float SimLibLastMajorFrameTime;
// ----------------------------------------------------
// DigitalBrain::AiRefuel
// ----------------------------------------------------

void DigitalBrain::AiRefuel(void)
{
    AircraftClass *tanker = NULL;

    //for now
    if (tankerId not_eq FalconNullId)
    {
        tanker = (AircraftClass*)vuDatabase->Find(tankerId);

        if (tanker)
        {
            float xft;
            float yft;
            float zft;
            float rx, fx;
            float ry, fy;
            float rz, fz;
            float dx, dy, dz, dist;
            Tpoint targetPos;
            float rad;
            int ReadySet = 0;

            if (self->drawPointer)
                rad = self->drawPointer->Radius();
            else rad = 50;

            ((TankerBrain*)tanker->Brain())->OptTankingPosition(&targetPos);

            // JB 020311 Respond to "commands" from the tanker.
            SetTrackPoint(targetPos.x, targetPos.y, targetPos.z);

            switch (refuelstatus)
            {
                default:
                case refNoTanker:
                case refWaiting:
                    trackZ += rad * tnkposition;

                    if (tnkposition <= self->vehicleInUnit)
                    {
                        // Set track point 1.0NM ahead of desired location
                        trackX += (-0.05F * NM_TO_FT + rad * tnkposition) * tanker->platformAngles.cossig;
                        trackY += (-0.05F * NM_TO_FT + rad * tnkposition) * tanker->platformAngles.sinsig;
                    }
                    else
                    {
                        // Set track point 1.0NM ahead of desired location
                        trackX += (-0.25F * NM_TO_FT + rad * tnkposition) * tanker->platformAngles.cossig;
                        trackY += (-0.25F * NM_TO_FT + rad * tnkposition) * tanker->platformAngles.sinsig;
                    }

                    //offset trackpoint according to our position in line
                    trackX += 200.0F * -tanker->platformAngles.sinsig * tnkposition;
                    trackY += 200.0F * tanker->platformAngles.cossig * tnkposition;
                    break;

                case refRefueling:
                    break;

                case refDone:
                    trackZ += rad * tnkposition;

                    if (tnkposition >= 0)
                    {
                        // Set track point 1.0NM ahead of desired location
                        trackX += (-0.05F * NM_TO_FT + rad * tnkposition) * tanker->platformAngles.cossig;
                        trackY += (-0.05F * NM_TO_FT + rad * tnkposition) * tanker->platformAngles.sinsig;

                        //offset trackpoint according to our position in line
                        trackX -= 200.0F * -tanker->platformAngles.sinsig * (4 - tnkposition);
                        trackY -= 200.0F * tanker->platformAngles.cossig * (4 - tnkposition);
                    }
                    else
                    {
                        dx = trackX - af->x;
                        dy = trackY - af->y;
                        dz = trackZ - af->z;

                        if (dx * dx + dy * dy > 0.01 * NM_TO_FT * NM_TO_FT)
                        {
                            ClearATCFlag(NeedToRefuel);
                            tankerId = FalconNullId;

                            // 02DEC03 - FRB
                            if (self->IsDigital())
                            {
                                self->af->ResetFuel();

                                if (self->drawPointer)
                                {
                                    ((DrawableBSP*)self->drawPointer)->SetSwitchMask(13, 0);  // 29NOV03 - FRB - Close refueling door/ Hide probe
                                    ((DrawableBSP*)self->drawPointer)->SetDOFangle(41, 0);  // 29NOV03 - FRB - Close refueling door/ Retract probe
                                }
                            }

                            // end FRB
                            Package package;
                            Flight flight;

                            flight = (Flight)self->GetCampaignObject();

                            if (flight)
                            {
                                package = flight->GetUnitPackage();

                                if (package)
                                {
                                    tankerId = package->GetTanker();
                                }
                            }
                        }
                        else
                        {
                            trackX -= 0.5F * NM_TO_FT * tanker->platformAngles.cossig;
                            trackY -= 0.5F * NM_TO_FT * tanker->platformAngles.sinsig;

                            //offset trackpoint according to our position in line
                            trackX -= 200.0F * -tanker->platformAngles.sinsig * (4.0F - self->vehicleInUnit);
                            trackY -= 200.0F * tanker->platformAngles.cossig * (4.0F - self->vehicleInUnit);
                        }
                    }

                    break;
            }

            // Calculate relative positions
            dx = trackX - af->x;
            dy = trackY - af->y;
            dz = trackZ - af->z;

            // Distance
            dist = (float)sqrt(dx * dx + dy * dy + dz * dz);

            dist = max(dist, 0.001F);

            float RFR = af->GetRefuelFollowRate();

            fx = dx / dist * RFR * SimLibLastMajorFrameTime;
            fy = dy / dist * RFR * SimLibLastMajorFrameTime;
            fz = dz / dist * RFR * SimLibLastMajorFrameTime;

            bool tractor = false;

            if (fabs(dx) > fabs(fx))
                af->x = af->x + fx;
            else
            {
                tractor = true;
                fx = af->x - trackX;
                af->x = trackX;
            }

            if (fabs(dy) > fabs(fy))
                af->y = af->y + fy;
            else
            {
                tractor = true;
                fy = af->y - trackY;
                af->y = trackY;
            }

            if (fabs(dz) > fabs(fz))
                af->z = af->z + fz;
            else
            {
                tractor = true;
                fz = af->z - trackZ;
                af->z = trackZ;
            }

            ShiAssert( not _isnan(af->x));
            ShiAssert( not _isnan(af->y));
            ShiAssert( not _isnan(af->z));

            // 12DEC03 - FRB - update distance to refueling position
            dx = trackX - af->x;
            dy = trackY - af->y;
            dz = trackZ - af->z;

            dist = (float)sqrt(dx * dx + dy * dy + dz * dz);

            dist = max(dist, 0.001F);
            // - FRB

            // 2002-03-28 MN Hack to make AI refueling working in each and every situation: if they are really close, just put them on the boom. Period ;-)
            // this also helps them in a tanker turn, which really can only do a human ;-)
            if (dist < af->GetAIBoomDistance() and g_bPutAIToBoom and refuelstatus == refRefueling)
            {
                // 26NOV03 - FRB - Get a more recent position
                ((TankerBrain*)tanker->Brain())->OptTankingPosition(&targetPos);
                af->x = targetPos.x;
                af->y = targetPos.y;
                af->z = targetPos.z;
                self->SetYPR(tanker->Yaw(), 0.0F, 0.0F); // change current heading to that of the tanker

                if ( not ReadySet)
                {
                    ((TankerBrain*)tanker->Brain())->AIReady(); // 28NOV03 - FRB - tell th tanker I'm in position
                    ReadySet = 1;
                }
            }

            float a, oldrx, deceldistance;
            bool decelerating = false;

            deceldistance = af->GetDecelerateDistance();

            CalculateRelativePos(&xft, &yft, &zft, &rx, &ry, &rz);
            oldrx = rx;

            if (rx > deceldistance)
            {
                rStick = SimpleTrackAzimuth(rx * 0.5F, ry, self->GetVt()) * 0.5F;
                pStick = SimpleTrackElevation(zft, 5000.0F);
                // Set track point 1.0NM ahead of desired location
                trackX += 1.0F * NM_TO_FT * tanker->platformAngles.cossig;
                trackY += 1.0F * NM_TO_FT * tanker->platformAngles.sinsig;
                CalculateRelativePos(&xft, &yft, &zft, &rx, &ry, &rz);
            }
            else
            {
                decelerating = true;

                if ((self->drawPointer) and not ((DrawableBSP*)self->drawPointer)->GetDOFangle(41))
                {
                    ((DrawableBSP*)self->drawPointer)->SetSwitchMask(13, 1);  // 29NOV03 - FRB - Open refueling door/ Display probe
                    ((DrawableBSP*)self->drawPointer)->SetDOFangle(41, self->af->GetRefuelAngle()*DTR);  // 29NOV03 - FRB - Open refueling door/ Extend probe
                }

                // MN we want to close our trackpoint with the tankers location, so reduce trackpoint distance by distance to tanker
                a = rx / deceldistance + 0.25f;
                a = min(1.0f, max(0.45f, a)); // minimum distance of trackpoint = 0.45nm in front of the tanker
                trackX += a * NM_TO_FT * tanker->platformAngles.cossig;
                trackY += a * NM_TO_FT * tanker->platformAngles.sinsig;
                CalculateRelativePos(&xft, &yft, &zft, &rx, &ry, &rz);
                rStick = SimpleTrackAzimuth(rx, ry, self->GetVt());
                pStick = SimpleTrackElevation(zft, 5000.0F);

                /*
                 // JB 020311 Respond to "commands" from the tanker.
                 if ( not af->IsSet(AirframeClass::Refueling) and refuelstatus == refRefueling and 
                 (SimLibElapsedTime - lastBoomCommand) > 10000)
                 {
                 lastBoomCommand = SimLibElapsedTime;
                 Tpoint relPos;
                 ((TankerBrain*)tanker->DBrain())->ReceptorRelPosition(&relPos, (SimVehicleClass*) self);

                 Tpoint boompos;
                 boompos.x = boompos.y = boompos.z = 0;

                 af->GetRefuelPosition(&boompos);
                 if (boompos.x == 0 and boompos.y ==0 and boompos.z == 0)
                 boompos.x = -39.63939795321F;

                 relPos.x -= boompos.x;

                 if(relPos.x < 0.0f)
                 tankerRelPositioning.x += 2.0f;
                 else
                 tankerRelPositioning.x -= 2.0f;

                 if(relPos.y < 0.0f)
                 tankerRelPositioning.y += 2.0f;
                 else
                 tankerRelPositioning.y -= 2.0f;

                 if(relPos.z < 0.0f)
                 tankerRelPositioning.z += 2.0f;
                 else
                 tankerRelPositioning.z -= 2.0f;
                 }
                */
            }

            float desiredClosure, actualClosure;

            rx = (float)sqrt(xft * xft + yft * yft);

            Tpoint followVector, followWVector;
            followWVector.x = fx;
            followWVector.y = fy;
            followWVector.z = fz;

            MatrixMultTranspose(&((DrawableBSP*)self->drawPointer)->orientation, &followWVector, &followVector);

            desiredClosure = 200.0F * rx / (af->GetDesiredClosureFactor() * NM_TO_FT) - 200.0F;

            // get actual closure
            actualClosure = - (rx - velocitySlope + followVector.x) / SimLibLastMajorFrameTime;

            float eProp, thr;
            eProp = desiredClosure - actualClosure;

            // 27NOV03 - FRB - Slow down to get behind tanker
            if (oldrx < 0.0F) // we're in front of the tanker...
            {
                eProp -= 20.0F;
                thr = 0.0F;
                af->speedBrake = 1.0F;
            }
            else if (eProp < -30.0F) // FRB - was -50
            {
                throtl = 0.0F;
                af->speedBrake = 1.0F;
            }
            else if (eProp < -20.0F) // FRB - was -40
            {
                thr = 0.0F;
            }
            else if (eProp < -10.0F)
            {
                thr = 0.2F;
            }
            else if (eProp > 200.0F)
            {
                thr = 1.0F;
                af->speedBrake = -1.0F;
            }
            else if (eProp > 100.0F)
            {
                thr = 0.8F;
                af->speedBrake = -1.0F;
            }
            else if (eProp > 10.0F)
            {
                thr = 0.6F;
                af->speedBrake = -1.0F;
            }
            else
            {
                thr = 0.48F;
            }

            if (fabs(eProp) < 200.0F)
            {
                if (refuelstatus == refRefueling)//af->IsSet(AirframeClass::Refueling))
                {
                    SimpleTrack(SimpleTrackSpd, tanker->TBrain()->GetDesSpeed()); // track the exact tanker speed

                    if (tanker->af->vcas > af->vcas + 10.0f)
                        thr += 0.15f; // speed somewhat up, we're at least 10 knots behind tanker speed
                    else if (tanker->af->vcas > af->vcas + 5.0f)
                        thr += 0.1f; // speed a bit up, we're falling behind
                    else if (af->vcas > tanker->af->vcas + 10.0f)
                        thr -= 0.15f; // we're at least 10 knots too fast, go even slower
                    else if (af->vcas > tanker->af->vcas + 5.0f)
                        thr -= 0.1f; // we're a bit too fast, go slower
                }
                else
                {
                    if (tanker->af->vcas > af->vcas + 10.0f)
                        thr += 0.2f;
                    else if (tanker->af->vcas > af->vcas + 5.0f)
                        thr += 0.15f;
                    else if (tanker->af->vcas > af->vcas + 2.5f)
                        thr += 0.05f;
                    else if (af->vcas > tanker->af->vcas - 10.0f)
                        thr = thr - 0.05f;
                    else
                        thr += 0.1f;
                }
            }

            if (g_nShowDebugLabels bitand 0x800)
            {
                char tmpchr[32];

                if (decelerating)
                    sprintf(tmpchr, "D %3.2f %5.1f %6.1f", af->vcas, oldrx, af->Fuel() + af->ExternalFuel());
                else
                    sprintf(tmpchr, "%3.2f %5.1f %6.1f", af->vcas, oldrx, af->Fuel() + af->ExternalFuel());

                if (tractor)
                    strcat(tmpchr, " TRCT");

                if (self->drawPointer)
                    ((DrawableBSP*)self->drawPointer)->SetLabel(tmpchr, ((DrawableBSP*)self->drawPointer)->LabelColor());
            }

            if (g_nShowDebugLabels bitand 0x20000)
            {
                char tmpchr[32];
                float tX, tY, tZ;

                // JB 020311 Respond to commands from the tanker.
                tX = targetPos.x - tankerRelPositioning.x;
                tY = targetPos.y - tankerRelPositioning.y;
                tZ = targetPos.z - tankerRelPositioning.z;

                fx = tX - self->XPos();
                fy = tY - self->YPos();
                fz = tZ - self->ZPos();
                sprintf(tmpchr, "dX %3.4f dY %3.4f dZ %3.4f", fx, fy, fz);

                if (self->drawPointer)
                    ((DrawableBSP*)self->drawPointer)->SetLabel(tmpchr, ((DrawableBSP*)self->drawPointer)->LabelColor());
            }


            throtl = min(max(thr, 0.0F), 1.5F);

            velocitySlope = rx;
        }
        else
        {
            //tanker must have died
            tankerId = FalconNullId;
            ClearATCFlag(Refueling);
        }
    }
    else
    {
        FollowWaypoints();
    }
}

//////////////////////////////////////
//HelpRefuel

void DigitalBrain::HelpRefuel(AircraftClass *tanker)
{

    float desiredClosure, actualClosure;
    float eProp;
    float xft;
    float yft;
    float zft;
    float rx, fx, fxr;
    float ry, fy, fyr;
    float rz, fz, fzr;
    float dx, dy, dz, dist;
    Tpoint targetPos;
    int refuelMode;
    int ReadySet = 0;

    fx = fy = fz = 0.0F;
    fxr = fyr = fzr = 0.0F;

    ((TankerBrain*)tanker->Brain())->OptTankingPosition(&targetPos);

    SetTrackPoint(targetPos.x, targetPos.y, targetPos.z);

    dx = trackX - af->x;
    dy = trackY - af->y;
    dz = trackZ - af->z;

    dist = (float)sqrt(dx * dx + dy * dy + dz * dz);

    dist = max(dist, 0.001F);

    if (af->IsSet(AirframeClass::Refueling))
    {
        fxr = dx / dist * af->GetRefuelFollowRate() * SimLibLastMajorFrameTime;
        fyr = dy / dist * af->GetRefuelFollowRate() * SimLibLastMajorFrameTime;
        fzr = dz / dist * af->GetRefuelFollowRate() * SimLibLastMajorFrameTime;

        if (fabs(dx) > fabs(fxr))
            af->x = af->x + fxr;
        else
        {
            fxr = trackX - af->x;
            af->x = trackX;
        }

        if (fabs(dy) > fabs(fyr))
            af->y = af->y + fyr;
        else
        {
            fyr = trackY - af->y;
            af->y = trackY;
        }

        if (fabs(dz) > fabs(fzr))
            af->z = af->z + fzr;
        else
        {
            fzr = trackZ - af->z;
            af->z = trackZ;
        }
    }

    ShiAssert( not _isnan(af->x));
    ShiAssert( not _isnan(af->y));
    ShiAssert( not _isnan(af->z));

    refuelMode = PlayerOptions.GetRefuelingMode() - 1;

    // OW: sylvains refuelling fix
    // 2002-02-28 MN refuel fixes, help the player somewhat more. Simplistic = full AI control
#ifndef DEBUG

    if (dist < refuelMode * 100.0F and 
        fabs(tanker->Yaw() - self->Yaw()) < 3.0F * DTR * refuelMode and 
        fabs(tanker->GetVt() - self->GetVt())*FTPSEC_TO_KNOTS < (5.0F + 45.0F * (refuelMode /* S.G. NO refuelMode is ALREADY -1 - 1 */)) and 
        fabs(self->Pitch()*DTR) < 8.0F * refuelMode and 
        fabs(self->Roll()*DTR) < 8.0F * refuelMode)
#endif
    {
        //when helping we'll use simple mode, as it is the only practical way to fly formation
        af->SetSimpleMode(SIMPLE_MODE_AF);


        dx = trackX - af->x;
        dy = trackY - af->y;
        dz = trackZ - af->z;

        dist = (float)sqrt(dx * dx + dy * dy + dz * dz);

        dist = max(dist, 0.001F);

        fx = dx / dist * af->GetRefuelFollowRate() * SimLibLastMajorFrameTime * (refuelMode + 1);
        fy = dy / dist * af->GetRefuelFollowRate() * SimLibLastMajorFrameTime * (refuelMode + 1);
        fz = dz / dist * af->GetRefuelFollowRate() * SimLibLastMajorFrameTime * (refuelMode + 1);

        if (fabs(dx) > fabs(fx))
            af->x = af->x + fx;
        else
        {
            fx = trackX - af->x;
            af->x = trackX;
        }

        if (fabs(dy) > fabs(fy))
            af->y = af->y + fy;
        else
        {
            fy = trackY - af->y;
            af->y = trackY;
        }

        if (fabs(dz) > fabs(fz))
            af->z = af->z + fz;
        else
        {
            fz = trackZ - af->z;
            af->z = trackZ;
        }

        float a, oldrx, deceldistance;
        bool decelerating = false;

        ShiAssert( not _isnan(af->x));
        ShiAssert( not _isnan(af->y));
        ShiAssert( not _isnan(af->z));

        // 2002-03-28 MN Hack to make full AI refueling control working in each and every situation:
        // if we are really close, just put us on the boom. Period ;-)
        if (dist < af->GetAIBoomDistance() and g_bPutAIToBoom and refuelstatus == refRefueling and 
            (PlayerOptions.GetRefuelingMode() == ARSimplistic or PlayerOptions.GetRefuelingMode() == ARModerated and af->IsSet(AirframeClass::Refueling)))
        {
            // 26NOV03 - FRB - Get a more recent position
            ((TankerBrain*)tanker->Brain())->OptTankingPosition(&targetPos);
            af->x = targetPos.x;
            af->y = targetPos.y;
            af->z = targetPos.z;
            self->SetYPR(tanker->Yaw(), 0.0F, 0.0F); // change current heading to that of the tanker

            if ( not ReadySet)
            {
                ((TankerBrain*)tanker->Brain())->AIReady(); // 28NOV03 - FRB - tell th tanker I'm in position
                ReadySet = 1;
            }
        }


        deceldistance = af->GetDecelerateDistance();

        CalculateRelativePos(&xft, &yft, &zft, &rx, &ry, &rz);
        oldrx = rx;

        if (rx > deceldistance)
        {
            rStick = SimpleTrackAzimuth(rx * 0.5F, ry, self->GetVt()) * 0.5F;
            pStick = SimpleTrackElevation(zft, 5000.0F);
            // Set track point 1.0NM ahead of desired location
            trackX += 1.0F * NM_TO_FT * tanker->platformAngles.cossig;
            trackY += 1.0F * NM_TO_FT * tanker->platformAngles.sinsig;
            CalculateRelativePos(&xft, &yft, &zft, &rx, &ry, &rz);
        }
        else
        {
            decelerating = true;

            // MN we want to close our trackpoint with the tankers location, so reduce trackpoint distance by distance to tanker
            a = rx / deceldistance + 0.1f;
            a = min(1.0f, max(0.35f, a)); // minimum distance of trackpoint = 0.35nm in front of the tanker
            trackX += a * NM_TO_FT * tanker->platformAngles.cossig;
            trackY += a * NM_TO_FT * tanker->platformAngles.sinsig;
            CalculateRelativePos(&xft, &yft, &zft, &rx, &ry, &rz);
            rStick = SimpleTrackAzimuth(rx, ry, self->GetVt());
            pStick = SimpleTrackElevation(zft, 5000.0F);

            /*
             // JB 020311 Respond to "commands" from the tanker.
             if ( not af->IsSet(AirframeClass::Refueling) and refuelstatus == refRefueling and 
             (SimLibElapsedTime - lastBoomCommand) > 10000)
             {
             lastBoomCommand = SimLibElapsedTime;
             Tpoint relPos;
             ((TankerBrain*)tanker->DBrain())->ReceptorRelPosition(&relPos, (SimVehicleClass*) self);

             Tpoint boompos;
             boompos.x = boompos.y = boompos.z = 0;
             af->GetRefuelPosition(&boompos);
             if (boompos.x == 0 and boompos.y ==0 and boompos.z == 0)
             boompos.x = -39.63939795321F;

             relPos.x -= boompos.x;

             if(relPos.x < 0.0f)
             tankerRelPositioning.x += 2.0f;
             else
             tankerRelPositioning.x -= 2.0f;

             if(relPos.y < 0.0f)
             tankerRelPositioning.y += 2.0f;
             else
             tankerRelPositioning.y -= 2.0f;

             if(relPos.z < 0.0f)
             tankerRelPositioning.z += 2.0f;
             else
             tankerRelPositioning.z -= 2.0f;
             }
            */
        }

        rx = (float)sqrt(xft * xft + yft * yft);

        fx += fxr;
        fy += fyr;
        fz += fzr;

        Tpoint followVector, followWVector;
        followWVector.x = fx;
        followWVector.y = fy;
        followWVector.z = fz;

        MatrixMultTranspose(&((DrawableBSP*)self->drawPointer)->orientation, &followWVector, &followVector);

        desiredClosure = 200.0F * rx / (af->GetDesiredClosureFactor() * NM_TO_FT) - 200.0F;

        // get actual closure
        actualClosure = - (rx - velocitySlope + followVector.x) / SimLibLastMajorFrameTime;

        eProp  = desiredClosure - actualClosure;

        // 27NOV03 - FRB - Slow down to get behind tanker
        if (oldrx < 0.0F) // we're in front of the tanker...
        {
            eProp -= 20.0F;
            throtl = 0.0F;
            af->speedBrake = 1.0F;
        }
        else if (eProp < -30.0F) // FRB - was -50
        {
            throtl = 0.0F;
            af->speedBrake = 1.0F;
        }
        else if (eProp < -20.0F) // FRB - was -40
        {
            throtl = 0.0F;
        }
        else if (eProp < -10.0F)
        {
            throtl = 0.2F;
        }
        else if (eProp > 200.0F)
        {
            throtl = 1.0F;
            af->speedBrake = -1.0F;
        }
        else if (eProp > 100.0F)
        {
            throtl = 0.8F;
            af->speedBrake = -1.0F;
        }
        else if (eProp > 10.0F)
        {
            throtl = 0.6F;
            af->speedBrake = -1.0F;
        }
        else
        {
            throtl = 0.48F;
        }

        velocitySlope = rx;

        //allow the player to have some input

        // MN when connected, just try to keep about the same speed than the tanker has - this should keep us
        // in position, if not, try to be faster
        if (fabs(eProp) < 200.0F)
        {
            if (refuelstatus == refRefueling)//af->IsSet(AirframeClass::Refueling))
            {
                SimpleTrack(SimpleTrackSpd, tanker->TBrain()->GetDesSpeed()); // track the exact tanker speed

                if (tanker->af->vcas > af->vcas + 10.0f)
                    throtl += 0.15f; // speed somewhat up, we're at least 10 knots behind tanker speed
                else if (tanker->af->vcas > af->vcas + 5.0f)
                    throtl += 0.1f; // speed a bit up, we're falling behind
                else if (af->vcas > tanker->af->vcas + 10.0f)
                    throtl -= 0.15f; // we're at least 10 knots too fast, go even slower
                else if (af->vcas > tanker->af->vcas + 5.0f)
                    throtl -= 0.1f; // we're a bit too fast, go slower
            }
            else
            {
                if (tanker->af->vcas > af->vcas + 10.0f)
                    throtl += 0.2f;
                else if (tanker->af->vcas > af->vcas + 5.0f)
                    throtl += 0.15f;
                else if (tanker->af->vcas > af->vcas + 2.5f)
                    throtl += 0.05f;
                else if (af->vcas > tanker->af->vcas - 10.0f)
                    throtl = throtl - 0.05f;
                else
                    throtl += 0.1f;
            }
        }

        if (g_nShowDebugLabels bitand 0x800)
        {
            char tmpchr[32];

            if (decelerating)
                sprintf(tmpchr, "D %3.2f %5.1f %6.1", af->vcas, oldrx, af->Fuel() + af->ExternalFuel());
            else
                sprintf(tmpchr, "%3.2f %5.1f %6.1", af->vcas, oldrx, af->Fuel() + af->ExternalFuel());

            if (self->drawPointer)
                ((DrawableBSP*)self->drawPointer)->SetLabel(tmpchr, ((DrawableBSP*)self->drawPointer)->LabelColor());
        }


        if (g_bNewRefuelHelp)
        {
            // 2002-03-06 MN in moderated mode (Simplified), once we're stuck to the boom, no stick input needed anymore
            if (PlayerOptions.GetRefuelingMode() == ARModerated and not af->IsSet(AirframeClass::Refueling))
            {
                af->pstick = (UserStickInputs.pstick - pStick) * 0.3F + pStick;
                af->rstick = (UserStickInputs.rstick - rStick) * 0.3F + rStick;
                af->ypedal = (UserStickInputs.rudder);  // yPedal is not modified by this function
                af->throtl = (UserStickInputs.throttle - throtl) * 0.6F + throtl;
            }
            // Easy means no player input needed at all, also in Simplified when connected to the boom
            else if (PlayerOptions.GetRefuelingMode() == ARSimplistic or
                     PlayerOptions.GetRefuelingMode() == ARModerated and af->IsSet(AirframeClass::Refueling))
            {
                af->pstick =  pStick;
                af->rstick =  rStick;
                af->ypedal = (UserStickInputs.rudder);  // yPedal is not modified by this function
                af->throtl = (UserStickInputs.throttle - throtl) * 0.7F + throtl;
            }
            else
                // let's give realistic refueling also some help (realistic is just too hard in FF according to Keith Rosenkrantz)
            {
                af->pstick = (UserStickInputs.pstick - pStick) * 0.75F + pStick;
                af->rstick = (UserStickInputs.rstick - rStick) * 0.75F + rStick;
                af->ypedal = (UserStickInputs.rudder);
                af->throtl = (UserStickInputs.throttle - throtl) * 0.7F + throtl;
            }
        }
        else
        {
            if (PlayerOptions.GetRefuelingMode() == ARModerated)
            {
                af->pstick = (UserStickInputs.pstick - pStick) * 0.6F + pStick;
                af->rstick = (UserStickInputs.rstick - rStick) * 0.6F + rStick;
                af->ypedal = (UserStickInputs.rudder);
                af->throtl = (UserStickInputs.throttle - throtl) * 0.7F + throtl;
            }
            else if (PlayerOptions.GetRefuelingMode() == ARSimplistic)
            {
                af->pstick = (UserStickInputs.pstick - pStick) * 0.3F + pStick;
                af->rstick = (UserStickInputs.rstick - rStick) * 0.3F + rStick;
                af->ypedal = (UserStickInputs.rudder);
                af->throtl = (UserStickInputs.throttle - throtl) * 0.6F + throtl;
            }
        }

        af->throtl = max(0.0F, min(af->throtl, 1.5F));

    }
}

//////////////////////////////////////
//StartRefueling

void DigitalBrain::StartRefueling(void)
{
    VuEntity *theTanker = NULL;

    theTanker = vuDatabase->Find(tankerId);

    if (theTanker)
    {
        tankerId = theTanker->Id();
        refuelstatus = refWaiting;
        SetATCFlag(NeedToRefuel);
    }
}

///////////////////////////////////////
//DoneRefueling

void DigitalBrain::DoneRefueling(void)
{
    //this is so Russian planes will actually get gas when they try to refuel

    // 02DEC03 - FRB
    if (self->IsDigital())
    {
        self->af->ResetFuel();

        if (self->drawPointer)
        {
            ((DrawableBSP*)self->drawPointer)->SetSwitchMask(13, 0);  // 29NOV03 - FRB - Open refueling door/ Display probe
            ((DrawableBSP*)self->drawPointer)->SetDOFangle(41, 0);  // 29NOV03 - FRB - Close refueling door/ Retract probe
        }
    }

    // end FRB

    tnkposition = 0;
    //tankerId = FalconNullId;
    refuelstatus = refDone;
    //ClearATCFlag(NeedToRefuel);
    af->ClearFlag(AirframeClass::Refueling);
}

// Someone needs gas, what to do?
void DigitalBrain::FlightMemberWantsFuel(int state)
{
    float rangeAvail = 50.0F * NM_TO_FT;
    FlightClass* tankerFlight = NULL;
    float xPos = 0.0F, yPos = 0.0F, zPos = 0.0F;
    WayPointClass* tmpWaypoint = self->waypoint;
    WayPointClass* newWaypoint = NULL;
    int foundSomething = FALSE;
    int time = 0;

    // Only Combat AP or AI will respond to request for fuel
    if (self->AutopilotType() == AircraftClass::CombatAP)
    {
        switch (state)
        {
            case SaidJoker:
                rangeAvail = 100.0F * NM_TO_FT;
                break;

            case SaidBingo:
                rangeAvail =  65.0F * NM_TO_FT;
                break;

            case SaidFumes:
                rangeAvail =  25.0F * NM_TO_FT;
                break;

            case SaidFlameout:
                rangeAvail = 0.0F;
                break;

            default:
                rangeAvail = 50.0F * NM_TO_FT;
                break;
        }

        // Look for tanker w/in range
        tankerFlight = ((FlightClass*)self->GetCampaignObject())->GetTankerFlight();

        if (tankerFlight)
        {
            // Find the tanker waypoint, and make it the current one
            while (tmpWaypoint)
            {
                if (tmpWaypoint->GetWPAction() == WP_REFUEL)
                {
                    break;
                }

                tmpWaypoint = tmpWaypoint->GetNextWP();
            }

            // Close enough?
            if (tmpWaypoint)
            {
                xPos = tankerFlight->XPos();
                yPos = tankerFlight->YPos();
                zPos = tankerFlight->ZPos();

                if (
                    fabs(self->XPos() - tankerFlight->XPos()) < rangeAvail and 
                    fabs(self->YPos() - tankerFlight->YPos()) < rangeAvail
                )
                {
                    self->curWaypoint = tmpWaypoint;
                    SetWaypointSpecificStuff();
                    foundSomething = TRUE;
                    // MonoPrint ("Heading for tanker\n");
                }
            }
        }

        // Check alternate field
        if ( not foundSomething)
        {
            // Find the alternate field
            tmpWaypoint = self->waypoint;

            while (tmpWaypoint)
            {
                if (tmpWaypoint->GetWPFlags() == WPF_ALTERNATE)
                {
                    break;
                }

                tmpWaypoint = tmpWaypoint->GetNextWP();
            }

            // Close enough?
            if (tmpWaypoint)
            {
                tmpWaypoint->GetLocation(&xPos, &yPos, &zPos);

                if (fabs(self->XPos() - tankerFlight->XPos()) < rangeAvail and 
                    fabs(self->YPos() - tankerFlight->YPos()) < rangeAvail)
                {
                    self->curWaypoint = tmpWaypoint;
                    SetWaypointSpecificStuff();
                    foundSomething = TRUE;
                    MonoPrint("Heading for alternate\n");
                }
            }
        }

        // Find nearest ?
        if ( not foundSomething)
        {
            ObjectiveClass* nearest = FindNearestFriendlyRunway(
                                          self->GetTeam(), SimToGrid(self->XPos()), SimToGrid(self->YPos())
                                      );

            // Head for it in any case, it's our best bet
            if (nearest)
            {
                newWaypoint = new WayPointClass;
                newWaypoint->SetLocation(nearest->XPos(), nearest->YPos(), -20000);
                time = SimLibElapsedTime;
                time += FloatToInt32((Distance(self->XPos(), self->YPos(), nearest->XPos(), nearest->YPos())) /
                                     get_air_speed(300.0F, 20000) * 1000.0F);

                newWaypoint->SetWPArrive(time);
                newWaypoint->SetWPDepartTime(time);
                newWaypoint->SetWPAction(WP_LAND);

                // Find the last waypoint
                tmpWaypoint = self->waypoint;

                while (tmpWaypoint)
                {
                    // Is this waypoint close to one we have?
                    tmpWaypoint->GetLocation(&xPos, &yPos, &zPos);

                    if (tmpWaypoint->GetWPAction() == WP_LAND and 
                        fabs(nearest->XPos() - xPos) < 2.0F * NM_TO_FT and 
                        fabs(nearest->YPos() - yPos) < 2.0F * NM_TO_FT)
                    {
                        foundSomething = TRUE;
                        break;
                    }

                    if ( not tmpWaypoint->GetNextWP())
                    {
                        break;
                    }

                    tmpWaypoint = tmpWaypoint->GetNextWP();
                }

                if (foundSomething)
                {
                    delete newWaypoint;
                }
                else
                {
                    tmpWaypoint->SetNextWP(newWaypoint);
                    self->curWaypoint = newWaypoint;
                    SetWaypointSpecificStuff();
                }

                //            MonoPrint ("Heading for nearest\n");
            }
        }
    }
}
