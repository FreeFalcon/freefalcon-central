#include "stdhdr.h"
#include "fcc.h"
#include "sms.h"
#include "object.h"
#include "otwdrive.h"
#include "simveh.h"
#include "radar.h"
#include "campwp.h"
#include "laserpod.h"
#include "Graphics/Include/drawbsp.h"
#include "camp2sim.h"
#include "simdrive.h"
#include "playerop.h"
#include "flightData.h"
/* S.G. FOR HAVING GROUND TARGET FROM DIGITALS */ #include "digi.h"
//MI
#include "aircrft.h"
#include "otwdrive.h"
#include "cpmanager.h"
#include "icp.h"

#include "simio.h"  // MD -- 20040111: added for analog cursor support

#include "airframe.h"

static const float rangeFOV = (float)tan(1.0F * DTR);
static const float LASER_SLEW_RATE  = 0.05F;
extern float g_fCursorSpeed;

extern SensorClass* FindLaserPod(SimMoverClass* theObject);

extern bool g_bLgbFixes; // a.s.
extern bool g_bAGRadarFixes;

void FireControlComputer::TargetingPodMode(void)
{
    float dx, dy, dz;
    float rx, ry, rz;
    //float pitch, roll, yaw; //moved to the class
    float tmpX, tmpY, tmpZ;
    int isLimited = FALSE;
    WayPointClass* curWaypoint = platform->curWaypoint;
    LaserPodClass* targetingPod = (LaserPodClass*) FindLaserPod(platform);
    SimObjectType *curTarget = targetPtr;
    mlTrig trig;
    int onGround = TRUE;
    SimObjectType *systemTarget;
    float minDist;

    if ( not targetingPod) return; // MLR 4/10/2004 - we're going to enable the TGP for non GBU types

    if ( not targetingPod->IsOn()) return; // can't do anything if off

    // Get our platforms radar (if any)
    RadarClass* theRadar = (RadarClass*) FindSensor(platform, SensorClass::Radar);

    if (theRadar)
        systemTarget = theRadar->CurrentTarget();
    else
        systemTarget = targetPtr;

    // 2001-11-01 ADDED BY M.N. IT MAY BE THAT theRadar HAS LOST THE TARGET, BUT OUR TARGETING POD
    // STILL HAS IT. SO SEE IF WE HAVE ONE LOCKED WITH THE POD
    if ( not systemTarget and not F4IsBadReadPtr(targetingPod, sizeof(targetingPod))) // M.N. CTD fix
    {
        systemTarget = targetingPod->CurrentTarget();

    }



    // Is the currently aimed target dead or exploding ? If so, delete the pod targetpointer
    if (systemTarget and (systemTarget->BaseData()->IsDead() or systemTarget->BaseData()->IsExploding())
       and not F4IsBadWritePtr(targetingPod, sizeof(targetingPod)))  // CTD fix
    {
        systemTarget = NULL;
        targetingPod->SetDesiredTarget(NULL);
    }

    // END of added section
#if 0 // MLR - erm. Why?

    // Make sure it's on the ground
    if (systemTarget)
    {
        /* JB 010624 Why? Setting the position like this screws up multiplayer and entitys' movement
        if (systemTarget->BaseData()->IsSim() and 
        systemTarget->BaseData()->OnGround() and 
        ((SimBaseClass*)systemTarget->BaseData())->IsAwake())
        {
        ((SimBaseClass*)systemTarget->BaseData())->drawPointer->GetPosition (&pos);
        }
        else
         */
        {
            pos.x = systemTarget->BaseData()->XPos();
            pos.y = systemTarget->BaseData()->YPos();
            pos.z = OTWDriver.GetGroundLevel(pos.x, pos.y);
        }
        systemTarget->BaseData()->SetPosition(pos.x, pos.y, pos.z);
    }

#endif

    if (targetingPod) // we already know we have a TGP
    {
        // Simple radars, look at locked target if any, otherwise look at cursors
        // 2001-04-16 MODIFIED BY S.G. CombatAP GETS TO USE THIS STUFF AS WELL.
        //    if  (playerFCC and (PlayerOptions.GetAvionicsType() not_eq ATRealistic) and (subMode == SLAVE))
        // M.N. added full realism mode
        if ((playerFCC and (PlayerOptions.GetAvionicsType() not_eq ATRealistic and PlayerOptions.GetAvionicsType() not_eq ATRealisticAV)
            and (subMode == SLAVE)) or ((AircraftClass *)platform)->AutopilotType() == AircraftClass::CombatAP)
        {
            if (systemTarget and systemTarget->BaseData()->OnGround())
            {
                /*  JB 010624 Why? Setting the position like this screws up multiplayer and entitys' movement
                if (systemTarget->BaseData()->IsSim() and ((SimBaseClass*)systemTarget->BaseData())->IsAwake())
                {
                ((SimBaseClass*)systemTarget->BaseData())->drawPointer->GetPosition( &pos );
                systemTarget->BaseData()->SetPosition( systemTarget->BaseData()->XPos(),
                systemTarget->BaseData()->YPos(), pos.z);
                }
                 */
                groundDesignateX = systemTarget->BaseData()->XPos();
                groundDesignateY = systemTarget->BaseData()->YPos();
                groundDesignateZ = systemTarget->BaseData()->ZPos();
            }
            else
            {
                // Find the ground Point
                ShiAssert(theRadar);
                theRadar->GetAGCenter(&groundDesignateX, &groundDesignateY);
                groundDesignateZ = OTWDriver.GetGroundLevel(groundDesignateX, groundDesignateY);
            }

            targetingPod->SetTargetPosition(groundDesignateX, groundDesignateY, groundDesignateZ);

            dx = groundDesignateX - platform->XPos();
            dy = groundDesignateY - platform->YPos();
            dz = groundDesignateZ - platform->ZPos();

            rx = platform->dmx[0][0] * dx + platform->dmx[0][1] * dy + platform->dmx[0][2] * dz;
            ry = platform->dmx[1][0] * dx + platform->dmx[1][1] * dy + platform->dmx[1][2] * dz;
            rz = platform->dmx[2][0] * dx + platform->dmx[2][1] * dy + platform->dmx[2][2] * dz;

            yaw   = groundDesignateAz    = (float)atan2(ry, rx);
            pitch = groundDesignateEl    = (float)atan(-rz / (float)sqrt(rx * rx + ry * ry + .1f));
            groundDesignateDroll = (float)atan2(ry, -rz);
            isLimited = targetingPod->SetDesiredSeekerPos(&yaw, &pitch);

            yaw   = (float)atan2(dy, dx);
            pitch = (float)atan(-dz / (float)sqrt(dx * dx + dy * dy + .1f));
            roll  = platform->Roll();
            targetingPod->SetYPR(yaw, pitch, roll);
            inRange = TRUE;

            // 2001-04-16 MODIFIED BY S.G. ONLY SET THE TARGET IF OUR POD CAN SEE IT
            //  targetingPod->SetDesiredTarget(systemTarget);
            if (systemTarget)
            {
                CalcRelGeom(platform, systemTarget, NULL, 1.0F / SimLibMajorFrameTime);

                if (targetingPod->CanSeeObject(systemTarget) and targetingPod->CanDetectObject(systemTarget))
                    targetingPod->SetDesiredTarget(systemTarget);
                else
                    targetingPod->SetDesiredTarget(NULL);
            }
            else
                targetingPod->SetDesiredTarget(NULL);

            // END OF MODIFIED SECTION

            if (targetingPod->CurrentTarget())
                targetingPod->LockTarget();

            // 2001-04-16 ADDED BY S.G. Set the LGB current TARGET IF WE ARE LOCKED
            if (targetingPod->IsLocked())
                SetTarget(targetingPod->CurrentTarget());

            // END OF ADDED SECTION

            platform->SOIManager(SimVehicleClass::SOI_WEAPON);
        }
        else
        {
            switch (masterMode)
            {
                case Missile:
                case Dogfight:
                case MissileOverride:
                    /* MLR - very early stages of implementing AA TGP mode */
                {
                    enum AATGPMode { OWNTGT, FCRLOS, BORESIGHT };

                    AATGPMode tgpmode = BORESIGHT;

                    if (systemTarget)
                    {
                        tgpmode = FCRLOS;
                    }

                    /*
                    if(targetingPod->IsSOI())
                    {
                     if(targetingPod->CurrentTarget())
                     {
                     tgpmode=OWNTGT;
                     }
                    }
                    */

                    /* In A-A the TGP is initially commanded to the FCR LOS if the FCR
                    is tracking an A-A target.
                    */

                    switch (tgpmode)
                    {
                        case FCRLOS:
                        {
                            float x, y, z;
                            x = systemTarget->BaseData()->XPos();
                            y = systemTarget->BaseData()->YPos();
                            z = systemTarget->BaseData()->ZPos();

                            float dx, dy, dz, rx, ry, rz, yaw, pitch, roll;

                            dx = x - platform->XPos();
                            dy = y - platform->YPos();
                            dz = z - platform->ZPos();

                            rx = platform->dmx[0][0] * dx + platform->dmx[0][1] * dy + platform->dmx[0][2] * dz;
                            ry = platform->dmx[1][0] * dx + platform->dmx[1][1] * dy + platform->dmx[1][2] * dz;
                            rz = platform->dmx[2][0] * dx + platform->dmx[2][1] * dy + platform->dmx[2][2] * dz;

                            yaw    = (float)atan2(ry, rx);
                            pitch  = (float)atan(-rz / (float)sqrt(rx * rx + ry * ry + .1f));
                            roll   = (float)atan2(ry, -rz);

                            isLimited = targetingPod->SetDesiredSeekerPos(&yaw, &pitch);

                            yaw   = (float)atan2(dy, dx);
                            pitch = (float)atan(-dz / (float)sqrt(dx * dx + dy * dy + .1f));
                            roll  = platform->Roll();

                            targetingPod->SetYPR(yaw, pitch, roll);

                            /*
                            if(designateCmd)
                            {
                             if (targetingPod->CanSeeObject(systemTarget) and targetingPod->CanDetectObject(systemTarget))
                             {
                             targetingPod->SetDesiredTarget(systemTarget);
                             }
                            }
                            */
                        }
                        break;

                        case BORESIGHT:
                        {
                            /* When the TGP is not the SOI and the FCR is not tracking a target,
                            the TGP LOS is positioned to 0 degrees azimuth and 3 degrees elevation.
                             */
                            float dx, dy, dz, rx, ry, rz, yaw, pitch, roll;

                            // -3 degrees down
                            dx = 0.998629f;
                            dy = 0;
                            dz = 0.052335f;

                            rx = platform->dmx[0][0] * dx + platform->dmx[1][0] * dy + platform->dmx[2][0] * dz;
                            ry = platform->dmx[0][1] * dx + platform->dmx[1][1] * dy + platform->dmx[2][1] * dz;
                            rz = platform->dmx[0][2] * dx + platform->dmx[1][2] * dy + platform->dmx[2][2] * dz;

                            yaw    = (float)atan2(ry, rx);
                            pitch  = (float)atan2(-rz, (float)sqrt(rx * rx + ry * ry + .1f));
                            //roll   = (float)atan2 (ry,-rz);
                            roll = platform->Roll();

                            MonoPrint("TGP r%.4f %.4f %.4f   ypr%.4f %.4f %.4f", rx, ry, rz, yaw * 180 / 3.14159, pitch * 180 / 3.14159, roll * 180 / 3.14159);
                            targetingPod->SetYPR(yaw, pitch, roll);
                        }
                        break;
                    }
                }

                return;

                case AirGroundLaser:
                    break;

                default:
                    break;
                    return;
            }

            if (preDesignate) //not ground stabilized
            {
                if (subMode == SLAVE)
                {
                    if (targetPtr and targetPtr->BaseData()->OnGround())
                    {
                        groundDesignateX = targetPtr->BaseData()->XPos();
                        groundDesignateY = targetPtr->BaseData()->YPos();
                        groundDesignateZ = targetPtr->BaseData()->ZPos();
                    }
                    else if (theRadar and theRadar->IsAG())
                    {
                        theRadar->GetAGCenter(&groundDesignateX, &groundDesignateY);
                        groundDesignateZ = OTWDriver.GetGroundLevel(groundDesignateX, groundDesignateY);
                    }
                    else if (curWaypoint)
                    {
                        curWaypoint->GetLocation(&groundDesignateX, &groundDesignateY, &groundDesignateZ);
                        groundDesignateZ = OTWDriver.GetGroundLevel(groundDesignateX, groundDesignateY);
                    }
                    else
                    {
                        ShiWarning("Warning:  Junk data for slave mode");
                    }

                    targetingPod->SetTargetPosition(groundDesignateX, groundDesignateY, groundDesignateZ);

                    dx = groundDesignateX - platform->XPos();
                    dy = groundDesignateY - platform->YPos();
                    dz = groundDesignateZ - platform->ZPos();

                    rx = platform->dmx[0][0] * dx + platform->dmx[0][1] * dy + platform->dmx[0][2] * dz;
                    ry = platform->dmx[1][0] * dx + platform->dmx[1][1] * dy + platform->dmx[1][2] * dz;
                    rz = platform->dmx[2][0] * dx + platform->dmx[2][1] * dy + platform->dmx[2][2] * dz;

                    groundDesignateAz    = (float)atan2(ry, rx);
                    groundDesignateEl    = (float)atan(-rz / (float)sqrt(rx * rx + ry * ry + .1f));
                    groundDesignateDroll = (float)atan2(ry, -rz);
                    yaw = groundDesignateAz;
                    pitch = groundDesignateEl;
                    isLimited = targetingPod->SetDesiredSeekerPos(&yaw, &pitch);

                    yaw   = (float)atan2(dy, dx);
                    pitch = (float)atan(-dz / (float)sqrt(dx * dx + dy * dy + .1f));
                    roll  = platform->Roll();

                    targetingPod->SetYPR(yaw, pitch, roll);
                    inRange = FALSE;

                    if (designateCmd and not lastDesignate)
                    {
                        preDesignate = FALSE;
                        inRange = TRUE;
                        platform->SOIManager(SimVehicleClass::SOI_WEAPON);
                        targetingPod->SetTargetPosition(groundDesignateX, groundDesignateY, groundDesignateZ);
                        //MI fix
                        targetingPod->SetDesiredSeekerPos(&yaw, &pitch);
                    }
                }
                else
                {
                    yaw   = 0.0F;
                    pitch = 0.0F;
                    isLimited = targetingPod->SetDesiredSeekerPos(&yaw, &pitch);
                    yaw   = platform->Yaw();
                    pitch = platform->Pitch();
                    roll  = platform->Roll();
                    mlSinCos(&trig, roll);
                    groundDesignateAz = -cockpitFlightData.beta * DTR + cockpitFlightData.windOffset * platform->platformAngles.cosphi;
                    groundDesignateEl = -cockpitFlightData.alpha * DTR + cockpitFlightData.windOffset * platform->platformAngles.sinphi;
                    pitch += groundDesignateEl * trig.cos - groundDesignateAz * trig.sin;
                    yaw += groundDesignateEl * trig.sin + groundDesignateAz * trig.cos;

                    targetingPod->SetYPR(yaw, pitch, roll);
                    inRange = FALSE;

                    if (FindGroundIntersection(pitch, yaw, &tmpX, &tmpY, &tmpZ))
                    {
                        groundDesignateX = tmpX;
                        groundDesignateY = tmpY;
                        groundDesignateZ = tmpZ;

                        // Must be looking at the ground to designate
                        if (designateCmd and not lastDesignate)
                        {
                            preDesignate = FALSE;
                            inRange = TRUE;
                            platform->SOIManager(SimVehicleClass::SOI_WEAPON);
                            targetingPod->SetTargetPosition(groundDesignateX, groundDesignateY, groundDesignateZ);
                        }
                    }
                    else
                    {
                        onGround = FALSE;
                    }
                }
            }
            else //predesignate (ground stabilized)
            {
                float xMove = 0.0F, yMove = 0.0F;

                if ((cursorXCmd not_eq 0) or (cursorYCmd not_eq 0))
                {
                    if ((IO.AnalogIsUsed(AXIS_CURSOR_X) == true) and (IO.AnalogIsUsed(AXIS_CURSOR_Y) == true))
                    {
                        yMove = (float)cursorYCmd / 10000.0F;
                        xMove = (float)cursorXCmd / 10000.0F;
                    }
                    else
                    {
                        yMove = (float)cursorYCmd;
                        xMove = (float)cursorXCmd;
                    }
                }

                if (((cursorXCmd not_eq 0) or (cursorYCmd not_eq 0)) and not targetingPod->IsLocked())
                {
                    if (g_bLgbFixes) // a.s. 26.Febr.2002. begin: New Code for slewing LGBs. With this code, not the angles are altered, but
                        // directly the designated point. A non-orthogonal rotation of the co-ordinate system is necessary
                    {

                        float deltaX = 0.0F, deltaY = 0.0F;
                        float theta = 0.0F, costheta = 0.0F, sintheta = 0.0F;
                        float phi = 0.0F, cosphi = 0.0F, sinphi = 0.0F;
                        float alpha = 0, cosalpha, sensoryaw = 0.0F;
                        float groundrange, range;
                        float pi = 3.1415926535898F;


                        dx = groundDesignateX - platform->XPos();
                        dy = groundDesignateY - platform->YPos();
                        dz = groundDesignateZ - platform->ZPos();

                        groundrange = (float) sqrt(dx * dx + dy * dy + 0.01F);
                        range = (float) sqrt(dx * dx + dy * dy + dz * dz + 0.01F);

                        theta = -(platform->Yaw());  // platform yaw  - rotation angle of y-axis

                        sensoryaw = -(float)atan2(dy, dx);  // sensor yaw

                        phi =   pi / 2.0F + sensoryaw; // rotation angle of x-axis
                        alpha = pi / 2.0F + ((float)atan(-dz / groundrange)); // (90° - sensor pitch)

                        costheta = (float) cos(theta);
                        sintheta = (float) sin(theta);
                        cosphi = (float) cos(phi);
                        sinphi = (float) sin(phi);
                        cosalpha = max((float) cos(alpha),  0.0001F);  // we need this for adjusting slew-rate

                        if (targetingPod->CurFOV() < 3.5F * DTR)
                        {
                            //MI only move if we are SOI
                            if (g_bRealisticAvionics)
                            {
                                if (targetingPod->IsSOI())
                                {
                                    deltaX = yMove * 50000.0F * dz / (cosalpha * cosalpha) * (0.03F / 4000) * g_fCursorSpeed * (LASER_SLEW_RATE / 8.0F) * SimLibMajorFrameTime; // calibrated for 4000 ft high and 10° pitch, (cos^2 10) = 0.03; "dx = z/(cos^2 alpha)*dalpha"
                                    deltaY = xMove * 12000.0F * range / 21000.0F * g_fCursorSpeed * (LASER_SLEW_RATE / 8.0F) * SimLibMajorFrameTime; // calibrated for 21000 ft range
                                }
                            }
                            else
                            {
                                deltaX = yMove * 50000.0F * dz / (cosalpha * cosalpha) * (0.03F / 4000) * g_fCursorSpeed * (LASER_SLEW_RATE / 8.0F) * SimLibMajorFrameTime; // calibrated for 4000 ft high and 10° pitch, (cos^2 10) = 0.03; "dx = z/(cos^2 alpha)*dalpha"
                                deltaY = xMove * 12000.0F * range / 21000.0F * g_fCursorSpeed * (LASER_SLEW_RATE / 8.0F) * SimLibMajorFrameTime; // calibrated for 21000 ft range
                            }
                        }
                        else
                        {
                            //MI only move if we are SOI
                            if (g_bRealisticAvionics)
                            {
                                if (targetingPod->IsSOI())
                                {
                                    deltaX = yMove * 50000.0F * dz / (cosalpha * cosalpha) * (0.03F / 4000) * g_fCursorSpeed * LASER_SLEW_RATE  * SimLibMajorFrameTime; // calibrated for 4000 ft high and 10° pitch, (cos^2 10) = 0.03
                                    deltaY = xMove * 12000.0F * range / 21000.0F * g_fCursorSpeed * LASER_SLEW_RATE  * SimLibMajorFrameTime; // calibrated for 21000 ft range
                                }
                            }
                            else
                            {

                                deltaX = yMove * 50000.0F * dz / (cosalpha * cosalpha) * (0.03F / 4000) * g_fCursorSpeed * LASER_SLEW_RATE * SimLibMajorFrameTime; // calibrated for 4000 ft high and 10° pitch, (cos^2 10) = 0.03; "dx = z/(cos^2 alpha)*dalpha"
                                deltaY = xMove * 12000.0F * range / 21000.0F * g_fCursorSpeed * LASER_SLEW_RATE * SimLibMajorFrameTime; // calibrated for 21000 ft range
                            }
                        }


                        ry = (costheta * deltaY + cosphi * deltaX);    // non-orthogonal rotation of euklidian base
                        rx = (sintheta * deltaY + sinphi * deltaX);
                        rz = 0.0F;


                        //ry =  costheta * deltaY - sintheta * deltaX; // orthogonal rotation of euklidian base
                        //rx =  sintheta * deltaY + costheta * deltaX;
                        //rz = 0.0F;


                        groundDesignateX += rx;   // adjusting the designated point
                        groundDesignateY += ry;
                        groundDesignateZ += 0.0F;


                        if (range > 210000.0F)  // no more than 40 Miles
                        {
                            preDesignate = TRUE;
                            groundPipperAz = 0.0F;
                            groundPipperEl = 0.0F;
                            yaw = 0.0F;
                            pitch = 0.0F;
                            platform->SOIManager(SimVehicleClass::SOI_RADAR);
                        }
                        else
                        {
                            targetingPod->SetTargetPosition(groundDesignateX, groundDesignateY, groundDesignateZ);
                        }

                        targetingPod->SetDesiredTarget(NULL);



                    }  // a.s. end new code
                    else // old code
                    {
                        targetingPod->GetYPR(&yaw, &pitch, &roll);

                        if (targetingPod->CurFOV() > 3.5F * DTR)
                        {
                            //MI only move if we are SOI
                            if (g_bRealisticAvionics)
                            {
                                if (targetingPod->IsSOI())
                                {
                                    pitch += yMove * g_fCursorSpeed * LASER_SLEW_RATE * SimLibMajorFrameTime;
                                    yaw += xMove * g_fCursorSpeed * LASER_SLEW_RATE * SimLibMajorFrameTime;
                                }
                            }
                            else
                            {
                                pitch += yMove * g_fCursorSpeed * LASER_SLEW_RATE * SimLibMajorFrameTime;
                                yaw += xMove * g_fCursorSpeed * LASER_SLEW_RATE * SimLibMajorFrameTime;
                            }
                        }
                        else
                        {
                            //MI only move if we are SOI
                            if (g_bRealisticAvionics)
                            {
                                if (targetingPod->IsSOI())
                                {
                                    pitch += yMove * g_fCursorSpeed * LASER_SLEW_RATE * SimLibMajorFrameTime * 0.25F;
                                    yaw += xMove * g_fCursorSpeed * LASER_SLEW_RATE * SimLibMajorFrameTime * 0.25F;
                                }
                            }
                            else
                            {
                                pitch += yMove * g_fCursorSpeed * LASER_SLEW_RATE * SimLibMajorFrameTime * 0.25F;
                                yaw += xMove * g_fCursorSpeed * LASER_SLEW_RATE * SimLibMajorFrameTime * 0.25F;
                            }
                        }

                        if ( not FindGroundIntersection(pitch, yaw, &tmpX, &tmpY, &tmpZ))
                        {
                            preDesignate = TRUE;
                            groundPipperAz = 0.0F;
                            groundPipperEl = 0.0F;
                            yaw = 0.0F;
                            pitch = 0.0F;
                            targetingPod->SetDesiredSeekerPos(&yaw, &pitch);
                            platform->SOIManager(SimVehicleClass::SOI_RADAR);
                        }
                        else
                        {
                            targetingPod->SetYPR(yaw, pitch, roll);
                            groundDesignateX = tmpX;
                            groundDesignateY = tmpY;
                            groundDesignateZ = tmpZ;
                            targetingPod->SetTargetPosition(groundDesignateX, groundDesignateY, groundDesignateZ);
                        }

                        targetingPod->SetDesiredTarget(NULL);
                    } // end old code

                }


                targetingPod->SetTargetPosition(groundDesignateX, groundDesignateY, groundDesignateZ);
                inRange = TRUE;

                dx = groundDesignateX - platform->XPos();
                dy = groundDesignateY - platform->YPos();
                dz = groundDesignateZ - platform->ZPos();

                pitch = (float)atan(-dz / (float)sqrt(dx * dx + dy * dy + .1f));
                yaw = (float)atan2(dy, dx);
                roll = 0.0F;

                targetingPod->SetYPR(yaw, pitch, roll);

                rx = platform->dmx[0][0] * dx + platform->dmx[0][1] * dy + platform->dmx[0][2] * dz;
                ry = platform->dmx[1][0] * dx + platform->dmx[1][1] * dy + platform->dmx[1][2] * dz;
                rz = platform->dmx[2][0] * dx + platform->dmx[2][1] * dy + platform->dmx[2][2] * dz;

                groundDesignateAz    = (float)atan2(ry, rx);
                groundDesignateEl    = (float)atan(-rz / (float)sqrt(rx * rx + ry * ry + .1f));
                groundDesignateDroll = (float)atan2(ry, -rz);

                pitch = groundDesignateEl;
                yaw   = groundDesignateAz;
                isLimited = targetingPod->SetDesiredSeekerPos(&yaw, &pitch);
            }

            if ( not targetingPod->IsLocked())
            {
                if ( not isLimited and onGround)
                {
                    // 2000-09-30 ADDED BY S.G. SO WE LOOK AT OUR CURRENTLY LOCKED GROUND RADAR TARGET IF WE DON'T ALREADY HAVE A curTarget
                    if ( not curTarget and ((DigitalBrain *)platform->Brain())->GetGroundTarget())
                    {
                        if (targetingPod->CanSeeObject(((DigitalBrain *)platform->Brain())->GetGroundTarget()) and targetingPod->CanDetectObject(((DigitalBrain *)platform->Brain())->GetGroundTarget()))
                            curTarget = ((DigitalBrain *)platform->Brain())->GetGroundTarget();
                    }

                    // Here we will redo our test for CanSeeObject and CanDetectObject. That's because I don't want
                    // to change the source code too much because of exe editing. In source, this will be another story
                    // END OF ADDED SECTION
                    if ( not curTarget)
                        curTarget = targetList;

                    // 2000-10-04 ADDED BY S.G. DON'T DO THE TARGET LIST IF ARE IN GM MODE
                    //MI CTD Fix
                    if ( not theRadar)
                        curTarget = NULL;
                    // 2002-04-12 MN Changed as we now also have ground units on GM radar when they are standing still
                    //    else if(theRadar->IsAG() == RadarClass::GM)
                    else if ( not g_bAGRadarFixes and theRadar->IsAG() == RadarClass::GM)
                        curTarget = NULL;

                    // 2000-09-30 MODIFIED BY S.G. WHY ONLY LOOK ONE DEGREE?? WE CALL CanSeeObject ANYHOW
                    // 2000-10-05 WE'LL LIMIT THE PLAYER'S TARGETING POD TO ONE DEGREE SO IT DOESN'T WANDER TOO FAR OFF
                    //             minDist = 1.0F * DTR;
                    if (playerFCC and SimDriver.GetPlayerAircraft() and ((AircraftClass *)platform)->AutopilotType() not_eq AircraftClass::CombatAP)
                        minDist = 0.2F * DTR;   // a.s. statt 1.0F 0.2F
                    else
                        minDist = 10.0f; // Above 2*pi

                    // END OF MODIFIED SECTION
                    while (curTarget)
                    {
                        // 2000-10-04 MODIFIED BY S.G. DON'T TARGET AIR VEHICLE
                        //                if (fabs(curTarget->localData->az - yaw) < minDist and 
                        if (curTarget->BaseData()->OnGround() and fabs(curTarget->localData->az - yaw) < minDist and 
                            fabs(curTarget->localData->el - pitch) < minDist and 
                            curTarget->BaseData()->IsSim() and 
 not curTarget->BaseData()->IsWeapon())
                        {
                            if (targetingPod->CanSeeObject(curTarget) and targetingPod->CanDetectObject(curTarget))
                            {
                                targetingPod->SetDesiredTarget(curTarget);
                                minDist = (float)min(fabs(curTarget->localData->az - yaw), fabs(curTarget->localData->el - pitch));
                            }
                        }

                        curTarget = curTarget->next;
                    }

                    // Check Features?
                    if ( not targetingPod->CurrentTarget() and not isLimited)
                    {
                        CheckFeatures(targetingPod);
                        curTarget = targetingPod->CurrentTarget();
                    }
                }
                else
                {
                    curTarget = NULL;
                    targetingPod->SetDesiredTarget(NULL);
                    preDesignate = TRUE;
                }
            }
            else
            {
                SetTarget(targetingPod->CurrentTarget());
            }

            if (targetingPod->CurrentTarget() and not preDesignate and designateCmd and not lastDesignate)
            {
                targetingPod->LockTarget();
            }

            if (targetPtr)
            {
                if (isLimited)
                {
                    dropTrackCmd = TRUE;
                }
                else
                {
                    if (targetPtr->BaseData()->IsSimObjective())
                    {
                        UpdateGroundObjectRelativeGeometry();
                    }

                    groundDesignateX = targetPtr->BaseData()->XPos();
                    groundDesignateY = targetPtr->BaseData()->YPos();
                    groundDesignateZ = targetPtr->BaseData()->ZPos();
                    targetingPod->SetTargetPosition(groundDesignateX, groundDesignateY, groundDesignateZ);
                }
            }
            else
            {
                if (isLimited)
                {
                    dropTrackCmd = TRUE;
                }
            }

            if (dropTrackCmd)
            {
                if (isLimited or not targetPtr)
                {
                    preDesignate = TRUE;

                    if (subMode == SLAVE)
                        platform->SOIManager(SimVehicleClass::SOI_RADAR);
                    else
                        platform->SOIManager(SimVehicleClass::SOI_HUD);
                }

                if (targetPtr)
                {
                    ClearCurrentTarget();
                }

                targetingPod->SetDesiredTarget(NULL);
                dropTrackCmd = FALSE;
            }
        }

        // Where will it hit?
        CalculateImpactPoint();
        FindRelativeImpactPoint();
        CalculateReleaseRange();
    }
    else
    {
        inRange = FALSE;
    }

    if ( not releaseConsent)
    {
        postDrop = FALSE;
    }

    //MI no Laser Above 25k ft
    if (g_bRealisticAvionics and playerFCC and ((AircraftClass *)platform)->AutopilotType() not_eq AircraftClass::CombatAP)
    {
        // RV - Biker - New systems (e.g. LANTIRN on F-14D) can do laser above 25k ft
        //if(platform->ZPos() > -25000.0F)
        if (platform->ZPos() > -1.0f * ((AircraftClass *)Sms->Ownship())->af->GetMaxLasingAlt())
        {
            //Firing the laser manually with the trigger
            if (ManualFire)
            {
                LaserFire = TRUE;
                LaserWasFired = TRUE;
            }
            //auto lasing, if check needed, not inhibited, and hasn't been fired manually between the drop
            else if (CheckForLaserFire and not InhibitFire and not LaserWasFired)
            {
                //Get the time before impact at which we want to lase
                if (OTWDriver.pCockpitManager and OTWDriver.pCockpitManager->mpIcp)
                    time = OTWDriver.pCockpitManager->mpIcp->LaserTime;
                else
                    time = 8;

                //Bomb still in air
                if (ImpactTime > 0.5F)
                {
                    Timer = 0.0F;

                    if ((ImpactTime <= time) and LaserArm)
                        LaserFire = TRUE;
                    else
                        LaserFire = FALSE;
                }
            }
            else
                LaserFire = FALSE;

            // MN only start the timer when we fired the laser and the bomb impacted
            if (LaserFire and ImpactTime <= 0.5F)
            {
                Timer += SimLibMajorFrameTime;

                if (Timer >= 4.5F) //lases 4 seconds after impact + 0.5 seconds from above
                {
                    CheckForLaserFire = FALSE;
                    LaserFire = FALSE;
                    Timer = 0.0F;
                }
            }

            if (InhibitFire)
                LaserFire = FALSE;

            //Get laser ranging
            float dx = platform->XPos() - groundDesignateX;
            float dy = platform->YPos() - groundDesignateY;
            float dz = platform->ZPos() - groundDesignateZ;
            LaserRange = (float)sqrt(dx * dx + dy * dy + dz * dz);
        }
        else
        {
            LaserFire = FALSE;
            //LaserWasFired = TRUE;
        }
    }
}

void FireControlComputer::CheckFeatures(LaserPodClass* targetingPod)
{
    FalconEntity* testObject = NULL;
    FalconEntity* closestObj = NULL;
    SimObjectType* tmpTarget = NULL;
    FeatureClassDataType *fc = NULL;
    SimBaseClass *simTarg = NULL;
    float groundRange;
    float curMin, dx, dy;

    if ( not targetPtr)
    {
        // Actually slant range
        groundRange = (float)sqrt((groundDesignateX - platform->XPos()) * (groundDesignateX - platform->XPos()) +
                                  (groundDesignateY - platform->YPos()) * (groundDesignateY - platform->YPos()) +
                                  (groundDesignateZ - platform->ZPos()) * (groundDesignateZ - platform->ZPos()));
        curMin = rangeFOV * groundRange;

        if (g_bRealisticAvionics)
        {
            curMin *= 2; //double the range
        }

        {
            VuListIterator featureWalker(SimDriver.featureList);
            testObject = (FalconEntity*)featureWalker.GetFirst();

            while (testObject)
            {
                dx = (float)fabs(testObject->XPos() - groundDesignateX);
                dy = (float)fabs(testObject->YPos() - groundDesignateY);
                //MI fix for jumping cursors
                float CurRange = (float)sqrt(dx * dx + dy * dy);

                // 2001-11-01 Added IsDead or
                //IsExploding check by M.N. - we don't want to bomb something that is already destroyed
                if ((CurRange < curMin) and not (testObject->IsDead() or testObject->IsExploding()))
                {
                    //simTarg = (SimBaseClass*)testObject;
                    //if (simTarg->IsStatic())
                    //fc = GetFeatureClassData(((Objective)simTarg)->GetFeatureID(0));
                    //if (fc and not F4IsBadReadPtr(fc, sizeof (fc)) and fc->Priority > 2)
                    // higher priority number = lower priority
                    closestObj = testObject;
                    curMin = CurRange;
                }

                testObject = (FalconEntity*)featureWalker.GetNext();
            }
        }

        if (closestObj)
        {
            Tpoint pos;
#ifdef DEBUG
            //tmpTarget = new SimObjectType(OBJ_TAG, platform, closestObj);
#else
            tmpTarget = new SimObjectType(closestObj);
#endif
            tmpTarget->Reference();

            //if ((SimBaseClass*)tmpTarget->BaseData()-> )
            /*  JB 010624 Why? Setting the position like this screws up multiplayer and entitys' movement
              if (tmpTarget->BaseData()->IsSim())
              // We ASSUMED that testObject IsSim above, so why check here???
             {
             ((SimBaseClass*)tmpTarget->BaseData())->drawPointer->GetPosition (&pos);
             } else
            */
            {
                pos.x = tmpTarget->BaseData()->XPos();
                pos.y = tmpTarget->BaseData()->YPos();
                pos.z = OTWDriver.GetGroundLevel(pos.x, pos.y);
            }
            tmpTarget->BaseData()->SetPosition(pos.x, pos.y, pos.z);

            tmpTarget->localData->az = groundDesignateAz;
            tmpTarget->localData->el = groundDesignateEl;
            tmpTarget->localData->droll = groundDesignateDroll =
                                              (float)atan2(sin(groundDesignateAz), sin(groundDesignateEl));
            tmpTarget->localData->ata = (float)acos(cos(groundDesignateAz) * cos(groundDesignateEl));

            tmpTarget->localData->range =
                (float)sqrt(
                    ((tmpTarget->BaseData()->XPos() - platform->XPos()) *
                     (tmpTarget->BaseData()->XPos() - platform->XPos())) +
                    ((tmpTarget->BaseData()->YPos() - platform->YPos()) *
                     (tmpTarget->BaseData()->YPos() - platform->YPos())) +
                    ((tmpTarget->BaseData()->ZPos() - platform->ZPos()) *
                     (tmpTarget->BaseData()->ZPos() - platform->ZPos()))
                )
                ;

            targetingPod->SetTargetPosition(groundDesignateX, groundDesignateY, groundDesignateZ);
            targetingPod->SetDesiredTarget(tmpTarget);
            tmpTarget->Release();
        }
        else if (targetingPod->CurrentTarget())
        {
            targetingPod->SetDesiredTarget(NULL);
        }
    }
}

//MI
void FireControlComputer::ToggleLaserArm(void)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (playerAC and playerAC->Sms->MasterArm() not_eq SMSBaseClass::Arm)
        return;

    LaserArm = not LaserArm;
    LaserFire = FALSE;
}
void FireControlComputer::RecalcPos(void)
{
    float dx, dy, dz;
    float rx, ry, rz;

    LaserPodClass* targetingPod = (LaserPodClass*) FindLaserPod(platform);
    RadarClass* theRadar = (RadarClass*) FindSensor(platform, SensorClass::Radar);
    WayPointClass* curWaypoint = platform->curWaypoint;

    if (targetingPod)
    {
        if (theRadar and theRadar->IsAG())
        {
            theRadar->GetAGCenter(&groundDesignateX, &groundDesignateY);
            groundDesignateZ = OTWDriver.GetGroundLevel(groundDesignateX, groundDesignateY);
        }
        else if (curWaypoint)
        {
            curWaypoint->GetLocation(&groundDesignateX, &groundDesignateY, &groundDesignateZ);
            groundDesignateZ = OTWDriver.GetGroundLevel(groundDesignateX, groundDesignateY);
        }
        else
        {
            ShiWarning("Warning:  Junk data for slave mode");
        }

        targetingPod->SetTargetPosition(groundDesignateX, groundDesignateY, groundDesignateZ);

        dx = groundDesignateX - platform->XPos();
        dy = groundDesignateY - platform->YPos();
        dz = groundDesignateZ - platform->ZPos();

        rx = platform->dmx[0][0] * dx + platform->dmx[0][1] * dy + platform->dmx[0][2] * dz;
        ry = platform->dmx[1][0] * dx + platform->dmx[1][1] * dy + platform->dmx[1][2] * dz;
        rz = platform->dmx[2][0] * dx + platform->dmx[2][1] * dy + platform->dmx[2][2] * dz;

        groundDesignateAz    = (float)atan2(ry, rx);
        groundDesignateEl    = (float)atan(-rz / (float)sqrt(rx * rx + ry * ry + .1f));
        groundDesignateDroll = (float)atan2(ry, -rz);
        yaw = groundDesignateAz;
        pitch = groundDesignateEl;
        targetingPod->SetDesiredSeekerPos(&yaw, &pitch);

        yaw   = (float)atan2(dy, dx);
        pitch = (float)atan(-dz / (float)sqrt(dx * dx + dy * dy + .1f));
        roll  = platform->Roll();
        targetingPod->SetYPR(yaw, pitch, roll);
    }
}
