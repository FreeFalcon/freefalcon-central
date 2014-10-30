#include "Graphics/Include/drawbsp.h"
#include "stdhdr.h"
#include "fcc.h"
#include "missile.h"
#include "otwdrive.h"
#include "simdrive.h"
#include "sms.h"
#include "mavdisp.h"
#include "object.h"
#include "simveh.h"
#include "hardpnt.h"
#include "geometry.h"
#include "radar.h"
#include "camp2sim.h"
#include "playerop.h"
#include "flightData.h"
#include "hud.h" //MI
#include "aircrft.h" // 2002-03-01 MN

#include "simio.h"  // MD -- 20040111: added for analog cursor support

static const float rangeFOV = (float)tan(1.0F * DTR);
static const float MAVERICK_SLEW_RATE  = 0.05F;
extern float g_fCursorSpeed;

extern bool g_bRealisticAvionics; //MI

extern bool g_bMavFixes; // a.s.
extern bool g_bMavFix2; // MN

void FireControlComputer::AirGroundMissileMode(void)
{
    switch (Sms->curWeaponType)
    {
        case wtAgm88:
            HarmMode();
            break;

        case wtAgm65:
            MaverickMode();
            break;

        default:
            if ( not releaseConsent)
            {
                // Check for regeneration of weapon
                if (postDrop and Sms->curWeapon == NULL)
                {
                    Sms->ResetCurrentWeapon();
                    Sms->WeaponStep();

                    if (Sms->curWeapon)
                    {
                        platform->SOIManager(SimVehicleClass::SOI_WEAPON);
                    }

                    ClearCurrentTarget();
                }

                postDrop = FALSE;
            }

            missileTOF = 0.0F;
            missileActiveRange = 0.0F;
            missileActiveTime = -1.0F;
            missileSeekerAz = 0.0F;
            missileSeekerEl = 0.0F;
            //LRKLUDGE
            missileRMax   = 10000.0F;
            missileRMin   = 0.075F * missileRMax;
            missileRneMax = 0.8F * missileRMax;
            missileRneMin = 0.2F * missileRMax;
            break;
    }
}

void FireControlComputer::MaverickMode(void)
{
    SimObjectType *curTarget;
    MissileClass* theMissile;
    MaverickDisplayClass* theDisplay = NULL;
    float yaw, pitch, roll;
    float dx, dy, dz;
    float rx, ry, rz;
    float tmpX, tmpY, tmpZ;
    int isLimited;
    mlTrig trig;
    //Tpoint pos;
    SimObjectType *systemTarget;

    // MD -- 20040110: adding for analog cursor support
    float xMove = 0.0F, yMove = 0.0F;

    if ((cursorXCmd not_eq 0) or (cursorYCmd not_eq 0))
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


    // See if we have a radar target
    RadarClass* theRadar = (RadarClass*) FindSensor(platform, SensorClass::Radar);

    if (theRadar)
        systemTarget = theRadar->CurrentTarget();
    else
        systemTarget = targetPtr;

    // COBRA - RED - FIXING CTDs - Make the display to be available before using it...
    theMissile = (MissileClass *)(Sms->GetCurrentWeapon());

    if (theMissile and (MaverickDisplayClass*)(theMissile->display) and Sms->CurHardpoint() >= 0)
    {
        theDisplay = (MaverickDisplayClass*)(theMissile->display);
        // RV - Biker - Get WEZ max/min and convert to ft
        missileWEZmax = theMissile->GetWEZmax() * NM_TO_FT;
        missileWEZmin = theMissile->GetWEZmin() * NM_TO_FT;
    }

    //sfr: added display check
    if ((theDisplay not_eq NULL) and systemTarget and systemTarget->BaseData()->IsAirplane()) // Cobra - Target only ground targets
    {
        if (theDisplay) theDisplay->DropTarget();

        ClearCurrentTarget();
        missileTarget = FALSE;
        missileTOF    = 0.0f;
        return;
    }

    // Make sure relative geometry is updated for the system target
    if (systemTarget and systemTarget->BaseData()->IsStatic())
    {
        SimObjectType* tmpPtr = systemTarget->next;
        systemTarget->next = NULL;
        CalcRelGeom(platform, systemTarget, NULL, 1.0F / SimLibMajorFrameTime);
        systemTarget->next = tmpPtr;
    }

    // edg: I noted that sms->curHardpoint was -1 and caused a crash.  ergo the last
    // test in this if stmt

    if (theDisplay)
    {
        Sms->hardPoint[Sms->CurHardpoint()]->GetSubPosition(Sms->curWpnNum, &rx, &ry, &rz);
        rx += 5.0F;
        dx = rx * platform->dmx[0][0] + ry * platform->dmx[1][0] + rz * platform->dmx[2][0];
        dy = rx * platform->dmx[0][1] + ry * platform->dmx[1][1] + rz * platform->dmx[2][1];
        dz = rx * platform->dmx[0][2] + ry * platform->dmx[1][2] + rz * platform->dmx[2][2];

        theDisplay->SetXYZ(platform->XPos() + dx, platform->YPos() + dy, platform->ZPos() + dz);

        // Simple radars, look at locked target if any, otherwise look at cursors
        // M.N. added full realism mode
        AircraftClass *pa = (AircraftClass *)platform;

        if ( not playerFCC or (pa->IsPlayer() and pa->AutopilotType() == AircraftClass::CombatAP) or ((PlayerOptions.GetAvionicsType() not_eq ATRealistic and PlayerOptions.GetAvionicsType() not_eq ATRealisticAV) and (subMode == SLAVE)))
        {
            if (systemTarget and systemTarget->BaseData()->OnGround())
            {
                /* JB 010624 Why? Setting the position like this screws up multiplayer and entitys' movement
                 if (systemTarget->BaseData()->IsSim() and ((SimBaseClass*)systemTarget->BaseData())->IsAwake())
                {
                   ((SimBaseClass*)systemTarget->BaseData())->drawPointer->GetPosition (&pos);
                   systemTarget->BaseData()->SetPosition (systemTarget->BaseData()->XPos(),
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
                RadarClass* theRadar = (RadarClass*) FindSensor(platform, SensorClass::Radar);

                if (theRadar)
                {
                    theRadar->GetAGCenter(&groundDesignateX, &groundDesignateY);
                    groundDesignateZ = OTWDriver.GetGroundLevel(groundDesignateX, groundDesignateY);
                }
            }

            // Missile is stabalized, OK to have DLZ
            missileTarget = TRUE;

            // Point at the designated spot
            dx = groundDesignateX - platform->XPos();
            dy = groundDesignateY - platform->YPos();
            dz = groundDesignateZ - platform->ZPos();
            theMissile->SetTargetPosition(groundDesignateX, groundDesignateY, groundDesignateZ);

            rx = platform->dmx[0][0] * dx + platform->dmx[0][1] * dy + platform->dmx[0][2] * dz;
            ry = platform->dmx[1][0] * dx + platform->dmx[1][1] * dy + platform->dmx[1][2] * dz;
            rz = platform->dmx[2][0] * dx + platform->dmx[2][1] * dy + platform->dmx[2][2] * dz;

            groundDesignateAz = (float)atan2(ry, rx);
            groundDesignateEl = (float)atan(-rz / (float)sqrt(rx * rx + ry * ry + .1f));
            groundDesignateDroll = (float)atan2(ry, -rz);

            // Can the missile see the target?
            theMissile->SetTarget(systemTarget);

            // 2001-05-25 ADDED BY S.G. SO RELATIVE GEOMETRY IS CALCULATED FOR THE TARGET
            // 2001-05-25 MODIFIED BY S.G. DONE IN 'SetTarget' INSTEAD.
            // 2001-10-20 MODIFIED BY S.G. THE CODE IN 'SetTarget' SLOWS DOWN AIM-9 TARGET HUNTING. TRYING IT HERE AGAIN...
            if (theMissile->targetPtr)
                CalcRelGeom(platform, theMissile->targetPtr, NULL, 1.0F / SimLibMajorFrameTime);

            // END OF ADDED SECTION

            pitch = groundDesignateEl;
            yaw   = groundDesignateAz;
            isLimited = theMissile->SetSeekerPos(&yaw, &pitch);
            theMissile->RunSeeker();

            if (theMissile->targetPtr)
            {
                theDisplay->LockTarget();
                SetTarget(theMissile->targetPtr);
                missileTarget = TRUE;
                missileTOF    = theMissile->GetTOF(-platform->ZPos(), platform->GetVt(), 0.0f, 0.0f, targetPtr->localData->range);
            }
            else
            {
                theDisplay->DropTarget();
                ClearCurrentTarget();
                missileTarget = FALSE;
                missileTOF    = 0.0f;
            }

            platform->SOIManager(SimVehicleClass::SOI_WEAPON);
        }
        else
        {
            if (dropTrackCmd)
            {
                if (theDisplay->IsLocked())
                {
                    theDisplay->DetectTarget();
                    ClearCurrentTarget();
                    dropTrackCmd = FALSE;
                }
                else
                {
                    theDisplay->DropTarget();
                    preDesignate = TRUE;

                    if (subMode not_eq SLAVE)
                        platform->SOIManager(SimVehicleClass::SOI_HUD);
                    else
                        platform->SOIManager(SimVehicleClass::SOI_RADAR);

                    groundPipperAz = 0.0F;
                    groundPipperEl = 0.0F;
                    groundDesignateX = 0.0F;
                    groundDesignateY = 0.0F;
                    groundDesignateZ = 0.0F;
                    groundDesignateAz = 0.0F;
                    groundDesignateEl = 0.0F;
                    yaw = 0.0F;
                    pitch = 0.0F;
                    theMissile->SetSeekerPos(&yaw, &pitch);
                    theDisplay->SetYPR(platform->Yaw(), platform->Pitch(), platform->Roll());
                    dropTrackCmd = FALSE;
                    ClearCurrentTarget();
                    theDisplay->DropTarget();

                    // Do back to unstabalized, NO DLZ
                    missileTarget = FALSE;
                }
            }
            else if (preDesignate)
            {
                if (designateCmd)
                {
                    yaw   = platform->Yaw();
                    pitch = platform->Pitch();
                    mlSinCos(&trig, platform->Roll());
                    pitch += groundDesignateEl * trig.cos - groundDesignateAz * trig.sin;
                    yaw   += groundDesignateEl * trig.sin + groundDesignateAz * trig.cos;

                    if (FindGroundIntersection(pitch, yaw, &tmpX, &tmpY, &tmpZ))
                    {
                        // Point at the designated spot
                        dx = groundDesignateX - platform->XPos();
                        dy = groundDesignateY - platform->YPos();
                        dz = groundDesignateZ - platform->ZPos();
                        theMissile->SetTargetPosition(groundDesignateX, groundDesignateY, groundDesignateZ);

                        rx = platform->dmx[0][0] * dx + platform->dmx[0][1] * dy + platform->dmx[0][2] * dz;
                        ry = platform->dmx[1][0] * dx + platform->dmx[1][1] * dy + platform->dmx[1][2] * dz;
                        rz = platform->dmx[2][0] * dx + platform->dmx[2][1] * dy + platform->dmx[2][2] * dz;

                        groundDesignateAz = (float)atan2(ry, rx);
                        groundDesignateEl = (float)atan(-rz / (float)sqrt(rx * rx + ry * ry + .1f));
                        groundDesignateDroll = (float)atan2(ry, -rz);
                        theMissile->SetTargetPosition(tmpX, tmpY, tmpZ);
                        preDesignate = FALSE;
                        platform->SOIManager(SimVehicleClass::SOI_WEAPON);
                        missileTarget = TRUE;
                    }
                    else
                    {
                        missileTarget = FALSE;
                    }
                }
                else
                {
                    ClearCurrentTarget();
                    theDisplay->DropTarget();
                    missileTarget = TRUE;

                    if (subMode == SLAVE)
                    {
                        if (systemTarget)
                        {
                            yaw   = systemTarget->localData->az;
                            pitch = systemTarget->localData->el;
                            groundDesignateX = systemTarget->BaseData()->XPos();
                            groundDesignateY = systemTarget->BaseData()->YPos();
                            groundDesignateZ = systemTarget->BaseData()->ZPos();
                        }
                        else
                        {
                            RadarClass* theRadar = (RadarClass*) FindSensor(platform, SensorClass::Radar);

                            if (theRadar)
                            {
                                pitch = theRadar->SeekerEl();
                                yaw   = theRadar->SeekerAz();
                                theRadar->GetAGCenter(&groundDesignateX, &groundDesignateY);
                                groundDesignateZ = OTWDriver.GetGroundLevel(groundDesignateX, groundDesignateY);
                            }
                            else
                            {
                                pitch = 0.0f;
                                yaw   = 0.0f;
                                groundDesignateX = 0.0F;
                                groundDesignateY = 0.0F;
                                groundDesignateZ = 0.0F;
                                missileTarget = FALSE;
                            }
                        }

                        mlSinCos(&trig, platform->Roll());
                        groundDesignateEl =  pitch * trig.cos + yaw * trig.sin;
                        groundDesignateAz = -pitch * trig.sin + yaw * trig.cos;
                        groundDesignateDroll = (float)atan2(sin(groundDesignateAz), sin(groundDesignateEl));
                        groundPipperEl = 0.0F;
                        groundPipperAz = 0.0F;

                        pitch += platform->Pitch();
                        yaw   += platform->Yaw();
                        roll  = 0.0F;
                    }
                    else
                    {
                        groundDesignateAz = -cockpitFlightData.beta * DTR + cockpitFlightData.windOffset * platform->platformAngles.cosphi;
                        groundDesignateEl = -cockpitFlightData.alpha * DTR + cockpitFlightData.windOffset * platform->platformAngles.sinphi;
                        groundDesignateAz += groundPipperAz;
                        groundDesignateEl += groundPipperEl;
                        groundDesignateDroll = (float)atan2(sin(groundDesignateAz), sin(groundDesignateEl));

                        groundPipperEl += yMove * g_fCursorSpeed * MAVERICK_SLEW_RATE * SimLibMajorFrameTime;
                        groundPipperAz += xMove * g_fCursorSpeed * MAVERICK_SLEW_RATE * SimLibMajorFrameTime;

                        yaw   = platform->Yaw();
                        pitch = platform->Pitch();
                        roll  = platform->Roll();
                        mlSinCos(&trig, platform->Roll());
                        pitch += groundDesignateEl * trig.cos - groundDesignateAz * trig.sin;
                        yaw += groundDesignateEl * trig.sin + groundDesignateAz * trig.cos;

                        if ( not FindGroundIntersection(pitch, yaw, &tmpX, &tmpY, &tmpZ))
                        {
                            groundDesignateX = 0.0F;
                            groundDesignateY = 0.0F;
                            groundDesignateZ = 0.0F;
                            missileTarget = FALSE;
                        }
                        else
                        {
                            groundDesignateX = tmpX;
                            groundDesignateY = tmpY;
                            groundDesignateZ = tmpZ;
                            missileTarget = TRUE;
                        }
                    }

                    theDisplay->SetYPR(yaw, pitch, roll);
                    dx = groundDesignateAz;
                    dy = groundDesignateEl;
                    isLimited = theMissile->SetSeekerPos(&dx, &dy);

                    if (missileTarget)
                    {
                        theMissile->SetTargetPosition(groundDesignateX, groundDesignateY, groundDesignateZ);
                        MavCheckLock(theMissile);
                    }
                }
            }
            else if ( not preDesignate)
            {
                if (theDisplay->IsLocked())
                {
                    if (targetPtr)
                    {
                        if (targetPtr->BaseData()->IsSimObjective())
                        {
                            UpdateGroundObjectRelativeGeometry();
                        }

                        groundDesignateX = targetPtr->BaseData()->XPos();
                        groundDesignateY = targetPtr->BaseData()->YPos();
                        groundDesignateZ = targetPtr->BaseData()->ZPos();
                    }

                    // Point at the designated spot
                    dx = groundDesignateX - platform->XPos();
                    dy = groundDesignateY - platform->YPos();
                    dz = groundDesignateZ - platform->ZPos();
                    theMissile->SetTargetPosition(groundDesignateX, groundDesignateY, groundDesignateZ);

                    rx = platform->dmx[0][0] * dx + platform->dmx[0][1] * dy + platform->dmx[0][2] * dz;
                    ry = platform->dmx[1][0] * dx + platform->dmx[1][1] * dy + platform->dmx[1][2] * dz;
                    rz = platform->dmx[2][0] * dx + platform->dmx[2][1] * dy + platform->dmx[2][2] * dz;

                    groundDesignateAz = (float)atan2(ry, rx);
                    groundDesignateEl = (float)atan(-rz / (float)sqrt(rx * rx + ry * ry + .1f));
                    groundDesignateDroll = (float)atan2(ry, -rz);

                    if (targetPtr)
                    {
                        theMissile->SetTarget(targetPtr);

                        // 2001-10-20 ADDED BY S.G. THE CODE IN 'SetTarget' BETTER BE SAFE THAN SORRY. FOR PLAYER'S FCC, ALSO SET THE Target Geometry
                        if (theMissile->targetPtr)
                            CalcRelGeom(platform, theMissile->targetPtr, NULL, 1.0F / SimLibMajorFrameTime);

                        // END OF ADDED SECTION
                        pitch = groundDesignateEl;
                        yaw   = groundDesignateAz;
                        isLimited = theMissile->SetSeekerPos(&yaw, &pitch);
                        theMissile->RunSeeker();

                        if ( not theMissile->targetPtr)
                        {
                            if (subMode not_eq SLAVE)
                                platform->SOIManager(SimVehicleClass::SOI_HUD);
                            else
                                platform->SOIManager(SimVehicleClass::SOI_RADAR);

                            ClearCurrentTarget();
                            theDisplay->DropTarget();
                        }
                    }
                    else
                    {
                        if (subMode not_eq SLAVE)
                            platform->SOIManager(SimVehicleClass::SOI_HUD);
                        else
                            platform->SOIManager(SimVehicleClass::SOI_RADAR);

                        theDisplay->DropTarget();
                    }
                }
                else
                {
                    if ((cursorXCmd not_eq 0) or (cursorYCmd not_eq 0))
                    {

                        if (g_bMavFixes) // a.s. 20.Febr.2002. begin: New Code for slewing MAVs. With this code, not the angles are altered, but
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

                            theta = -(platform->Yaw());  // platform yaw  - rotation angle of y-achse

                            sensoryaw = -(float)atan2(dy, dx);  // sensor yaw

                            phi =   pi / 2.0F + sensoryaw; // rotation angle of x-achse

                            alpha = pi / 2.0F + ((float)atan(-dz / groundrange)); // (90° - sensor pitch)

                            costheta = (float) cos(theta);
                            sintheta = (float) sin(theta);
                            cosphi = (float) cos(phi);
                            sinphi = (float) sin(phi);
                            cosalpha = max((float) cos(alpha),  0.0001F);  // we need this for adjusting slew-rate


                            if (theDisplay->CurFOV() < (3.5F * DTR))
                            {
                                deltaX = yMove * 50000.0F * dz / (cosalpha * cosalpha) * (0.03F / 4000) * g_fCursorSpeed * (MAVERICK_SLEW_RATE / 2.0F) * SimLibMajorFrameTime; // calibrated for 4000 ft high and 10° pitch, (cos^2 10) = 0.03; "dx = z/(cos^2 alpha)*dalpha"
                                deltaY = xMove * 12000.0F * range / 21000.0F * g_fCursorSpeed * (MAVERICK_SLEW_RATE / 2.0F) * SimLibMajorFrameTime; // calibrated for 21000 ft range
                            }
                            else
                            {
                                deltaX = yMove * 50000.0F * dz / (cosalpha * cosalpha) * (0.03F / 4000) * g_fCursorSpeed * MAVERICK_SLEW_RATE * SimLibMajorFrameTime; // calibrated for 4000 ft high and 10° pitch, (cos^2 10) = 0.03
                                deltaY = xMove * 12000.0F * range / 21000.0F * g_fCursorSpeed * MAVERICK_SLEW_RATE * SimLibMajorFrameTime; // calibrated for 21000 ft range
                            }


                            ry = (costheta * deltaY + cosphi * deltaX);    // non-orthogonal rotation of euklidian base
                            rx = (sintheta * deltaY + sinphi * deltaX);
                            rz = 0.0F;

                            /*
                            ry =  costheta * deltaY - sintheta * deltaX; // orthogonal rotation of euklidian base
                            rx =  sintheta * deltaY + costheta * deltaX;
                            rz = 0.0F;
                            */

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
                                platform->SOIManager(SimVehicleClass::SOI_HUD);
                                missileTarget = FALSE;

                                // 2002-04-22 MN when setting back to SOI_HUD, set designateCmd to false. This way slewing continues at the last position of the MAV SOI
                                if (g_bMavFix2)
                                    designateCmd = false;
                            }
                            else
                            {
                                theMissile->SetTargetPosition(groundDesignateX, groundDesignateY, groundDesignateZ);
                                missileTarget = TRUE;
                            }

                            theMissile->SetTarget(NULL);
                        }
                        // a.s. end of new slew code

                        else  // old SLEW Code
                        {
                            theDisplay->GetYPR(&yaw, &pitch, &roll);

                            //MI
                            if (theDisplay->CurFOV() < (3.5F * DTR))
                            {
                                pitch += yMove * g_fCursorSpeed * (MAVERICK_SLEW_RATE / 2) * SimLibMajorFrameTime;
                                yaw += xMove * g_fCursorSpeed * (MAVERICK_SLEW_RATE / 2) * SimLibMajorFrameTime;
                            }
                            else
                            {
                                pitch += yMove * g_fCursorSpeed * MAVERICK_SLEW_RATE * SimLibMajorFrameTime;
                                yaw += xMove * g_fCursorSpeed * MAVERICK_SLEW_RATE * SimLibMajorFrameTime;
                            }

                            if ( not FindGroundIntersection(pitch, yaw, &tmpX, &tmpY, &tmpZ))
                            {
                                preDesignate = TRUE;
                                groundPipperAz = 0.0F;
                                groundPipperEl = 0.0F;
                                yaw = 0.0F;
                                pitch = 0.0F;
                                platform->SOIManager(SimVehicleClass::SOI_HUD);
                                missileTarget = FALSE;
                            }
                            else
                            {
                                theDisplay->SetYPR(yaw, pitch, roll);
                                groundDesignateX = tmpX;
                                groundDesignateY = tmpY;
                                groundDesignateZ = tmpZ;
                                theMissile->SetTargetPosition(tmpX, tmpY, tmpZ);
                                missileTarget = TRUE;
                            }

                            theMissile->SetTarget(NULL);
                        } // end old SLEW Code

                    }

                    // Point at the designated spot
                    dx = groundDesignateX - platform->XPos();
                    dy = groundDesignateY - platform->YPos();
                    dz = groundDesignateZ - platform->ZPos();
                    theMissile->SetTargetPosition(groundDesignateX, groundDesignateY, groundDesignateZ);

                    rx = platform->dmx[0][0] * dx + platform->dmx[0][1] * dy + platform->dmx[0][2] * dz;
                    ry = platform->dmx[1][0] * dx + platform->dmx[1][1] * dy + platform->dmx[1][2] * dz;
                    rz = platform->dmx[2][0] * dx + platform->dmx[2][1] * dy + platform->dmx[2][2] * dz;

                    groundDesignateAz = (float)atan2(ry, rx);
                    groundDesignateEl = (float)atan(-rz / (float)sqrt(rx * rx + ry * ry + .1f));
                    groundDesignateDroll = (float)atan2(ry, -rz);

                    pitch = groundDesignateEl;
                    yaw   = groundDesignateAz;
                    isLimited = theMissile->SetSeekerPos(&yaw, &pitch);

                    if ( not isLimited)
                    {
                        curTarget = MavCheckLock(theMissile);

                        mlSinCos(&trig, platform->Roll());
                        pitch = groundDesignateEl * trig.cos - groundDesignateAz * trig.sin;
                        yaw = groundDesignateEl * trig.sin + groundDesignateAz * trig.cos;
                        theDisplay->SetYPR(yaw + platform->Yaw(), pitch + platform->Pitch(), 0.0F);
                        theDisplay->DetectTarget();
                    }
                    else
                    {
                        curTarget = NULL;
                        preDesignate = TRUE;

                        if (subMode not_eq SLAVE)
                            platform->SOIManager(SimVehicleClass::SOI_HUD);
                        else
                            platform->SOIManager(SimVehicleClass::SOI_RADAR);
                    }

                    if (theMissile->targetPtr and designateCmd and not lastDesignate)
                    {
                        SetTarget(curTarget);
                        theDisplay->LockTarget();
                        platform->SOIManager(SimVehicleClass::SOI_WEAPON);
                    }

                    if (theDisplay->IsLocked())
                    {
                        if (targetPtr)
                        {
                            if (targetPtr->BaseData()->IsSimObjective())
                            {
                                UpdateGroundObjectRelativeGeometry();
                            }

                            groundDesignateX = targetPtr->BaseData()->XPos();
                            groundDesignateY = targetPtr->BaseData()->YPos();
                            groundDesignateZ = targetPtr->BaseData()->ZPos();
                            theMissile->SetTargetPosition(groundDesignateX, groundDesignateY, groundDesignateZ);
                        }
                    }
                }

                if (targetPtr)
                {
                    pitch = (float)atan(-dz / (float)sqrt(dx * dx + dy * dy + .1f));
                    yaw = (float)atan2(dy, dx);
                    roll = 0.0F;

                    theDisplay->SetYPR(yaw, pitch, roll);
                    pitch = groundDesignateEl;
                    yaw   = groundDesignateAz;
                    theMissile->SetSeekerPos(&yaw, &pitch);
                }
            }
            else
            {
                ShiWarning("MavFCC, How did I get here?");
            }

            // RV - Biker - New WEZ calculation for Mavs
            // if (targetPtr)
            //{
            // missileWEZDisplayRange = 10.0F * NM_TO_FT;
            // theDisplay->SetTarget(TRUE);
            // missileTOF    = theMissile->GetTOF(-platform->ZPos(), platform->GetVt(), 0.0f, 0.0f, targetPtr->localData->range);
            //}

            if (targetPtr)
            {
                if (theRadar)
                {
                    // We are in range
                    if (targetPtr->localData->range < missileRMax)
                    {
                        if (missileRMax > 80 * NM_TO_FT)
                            missileWEZDisplayRange = 160.0F * NM_TO_FT;
                        else if (missileRMax > 40 * NM_TO_FT)
                            missileWEZDisplayRange = 80.0F * NM_TO_FT;
                        else if (missileRMax > 20 * NM_TO_FT)
                            missileWEZDisplayRange = 40.0F * NM_TO_FT;
                        else if (missileRMax > 10 * NM_TO_FT)
                            missileWEZDisplayRange = 20.0F * NM_TO_FT;
                        else
                            missileWEZDisplayRange = 10.0F * NM_TO_FT;
                    }
                    else
                    {
                        missileWEZDisplayRange = theRadar->GetRange() * NM_TO_FT;
                        missileWEZDisplayRange = min(missileWEZDisplayRange, missileWEZmax);
                    }

                    missileWEZDisplayRange = max(missileWEZDisplayRange, missileWEZmin);
                }
                else
                    missileWEZDisplayRange = 20.0F * NM_TO_FT;

                theDisplay->SetTarget(TRUE);
                missileTOF = theMissile->GetTOF(-platform->ZPos(), platform->GetVt(), 0.0f, 0.0f, targetPtr->localData->range);
            }

            else
            {
                float range = (float)sqrt(groundDesignateX * groundDesignateX + groundDesignateY * groundDesignateY);

                theDisplay->SetTarget(FALSE);
                missileTOF    = theMissile->GetTOF(-platform->ZPos(), platform->GetVt(), 0.0f, 0.0f, range);
            }
        }

    }
    else
    {
        if (subMode not_eq SLAVE)
            platform->SOIManager(SimVehicleClass::SOI_HUD);
        else
            platform->SOIManager(SimVehicleClass::SOI_RADAR);

        ClearCurrentTarget();
        missileTarget = FALSE;
    }

    if ( not releaseConsent)
    {
        postDrop = FALSE;
    }

    // edg: sigh.  The data is ALL WRONG for AGM missiles.  Just stuff
    // some sane data for now
    // RV - Biker - Why all data is wrong??? Data can be fixed easier
    //missileRMax = 8.0f * NM_TO_FT;
    if (targetPtr)
        missileRMax   = theMissile->GetRMax(-platform->ZPos(), platform->GetVt(), targetPtr->localData->az, 0.0f, 0.0f);
    else
        missileRMax = 8.0f * NM_TO_FT;

    missileRMin   = 0.075F * missileRMax;
    missileRneMax = 0.8F * missileRMax;
    missileRneMin = 0.2F * missileRMax;

    // Stuff some dummy data...
    missileActiveRange = 0.0F;
    missileActiveTime = -1.0F;
    missileSeekerAz = 0.0F;
    missileSeekerEl = 0.0F;

    //MI
    MissileClass *curWeapon = NULL;
    curWeapon = ((MissileClass*)Sms->GetCurrentWeapon());

    if (g_bRealisticAvionics and Sms and playerFCC)
    {
        AircraftClass *pa = (AircraftClass *)curWeapon->parent.get();

        if (
            curWeapon and 
            (curWeapon->parent and 
             pa->IsPlayer() and 
 not (pa->AutopilotType() == AircraftClass::CombatAP) and 
             (curWeapon->Covered or not Sms->Powered))
        )
        {
            missileTarget = FALSE;
            ClearCurrentTarget();

            if (theDisplay)
            {
                theDisplay->DropTarget();
            }

            theMissile->SetTarget(NULL);
        }
    }
}

int FireControlComputer::FindGroundIntersection(float el, float az, float* x, float* y, float* z)
{
    euler dir;
    vector pos;
    int retval;

    dir.yaw = az;
    dir.pitch = el;
    dir.roll  = platform->Roll();

    if (OTWDriver.GetGroundIntersection(&dir, &pos))
    {
        *x = pos.x;
        *y = pos.y;
        *z = pos.z;
        retval = TRUE;
    }
    else
    {
        retval = FALSE;
    }

    return retval;
}

void FireControlComputer::CheckFeatures(MissileClass* theMissile)
{
    VuListIterator featureWalker(SimDriver.featureList);
    FalconEntity* testObject = NULL;
    FalconEntity* closestObj = NULL;
    SimObjectType* tmpTarget;
    float groundRange;
    float curMin, dx, dy;

    if ( not targetPtr)
    {
        groundRange = (float)sqrt((groundDesignateX - platform->XPos()) * (groundDesignateX - platform->XPos()) +
                                  (groundDesignateY - platform->YPos()) * (groundDesignateY - platform->YPos()) +
                                  (groundDesignateZ - platform->ZPos()) * (groundDesignateZ - platform->ZPos()));
        curMin = rangeFOV * groundRange;

        {
            testObject = (FalconEntity*)featureWalker.GetFirst();

            while (testObject)
            {
                dx = (float)fabs(testObject->XPos() - groundDesignateX);
                dy = (float)fabs(testObject->YPos() - groundDesignateY);
                //MI I don't know who did the original code below, but something must have been screwed
                //in his head when he wrote this Fix for the jumping cursors
                float CurRange = (float)sqrt(dx * dx + dy * dy);

                if ((CurRange < curMin) and not (testObject->IsDead() or testObject->IsExploding()))
                {
                    closestObj = testObject;
                    curMin = CurRange;
                }

                testObject = (FalconEntity*)featureWalker.GetNext();
            }
        }

        if (closestObj)
        {
            Tpoint pos;

            tmpTarget = new SimObjectType(closestObj);
            tmpTarget->Reference();

            /* JB 010624 Why? Setting the position like this screws up multiplayer and entitys' movement
            if (tmpTarget->BaseData()->IsSim()) // We ASSUMED that testObject IsSim above, so why check here???
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
            groundDesignateDroll = (float)atan2(sin(groundDesignateAz), sin(groundDesignateEl));

            tmpTarget->localData->ata = (float)acos(cos(groundDesignateAz) * cos(groundDesignateEl));

            tmpTarget->localData->range = (float) sqrt(
                                              ((tmpTarget->BaseData()->XPos() - platform->XPos()) * (tmpTarget->BaseData()->XPos() - platform->XPos())) +
                                              ((tmpTarget->BaseData()->YPos() - platform->YPos()) * (tmpTarget->BaseData()->YPos() - platform->YPos())) +
                                              ((tmpTarget->BaseData()->ZPos() - platform->ZPos()) * (tmpTarget->BaseData()->ZPos() - platform->ZPos())));

            theMissile->SetTarget(tmpTarget);
            theMissile->RunSeeker();
            tmpTarget->Release();
        }
        else if (theMissile->targetPtr)
        {
            theMissile->SetTarget(NULL);
        }
    }
}

void FireControlComputer::UpdateGroundObjectRelativeGeometry(void)
{
    //Tpoint pos;
    float tmp;

    /* JB 010624 Why? Setting the position like this screws up multiplayer and entitys' movement
     if (targetPtr->BaseData()->IsSim() and ((SimBaseClass*)targetPtr->BaseData())->IsAwake())
    {
       ((SimBaseClass*)targetPtr->BaseData())->drawPointer->GetPosition (&pos);
       targetPtr->BaseData()->SetPosition (pos.x, pos.y, pos.z);
    }
     */

    targetPtr->localData->az = groundDesignateAz;
    targetPtr->localData->el = groundDesignateEl;
    targetPtr->localData->droll = groundDesignateDroll = -(float)atan2(sin(groundDesignateAz), sin(groundDesignateEl));

    tmp = (float)(cos(groundDesignateAz) * cos(groundDesignateEl));
    targetPtr->localData->ata = (float)atan2(sqrt(1 - tmp * tmp), tmp);

    targetPtr->localData->range = (float) sqrt(
                                      ((groundDesignateX - platform->XPos()) * (groundDesignateX - platform->XPos())) +
                                      ((groundDesignateY - platform->YPos()) * (groundDesignateY - platform->YPos())) +
                                      ((groundDesignateZ - platform->ZPos()) * (groundDesignateZ - platform->ZPos())));
}

SimObjectType* FireControlComputer::MavCheckLock(MissileClass* theMissile)
{
    SimObjectType* curTarget = targetList;
    float minDist;

    float range = 0.0F; // a.s.

    //MI fix for better Maverick target selection
    AircraftClass *pa = (AircraftClass *)platform;

    if ( not playerFCC or (pa->IsPlayer() and pa->AutopilotType() == AircraftClass::CombatAP))
        minDist = 1.0F * DTR;
    else
    {
        if (g_bMavFixes)  // a.s. 20.Ferb.2002 begin
        {
            range = (float) sqrt(
                        ((groundDesignateX - platform->XPos()) * (groundDesignateX - platform->XPos())) +
                        ((groundDesignateY - platform->YPos()) * (groundDesignateY - platform->YPos())) +
                        ((groundDesignateZ - platform->ZPos()) * (groundDesignateZ - platform->ZPos())));
            // minDist is now between 0.2 and 0.6, depending on the dinstance to the rarget. The closer to the target the bigger minDist.
            // Min (0.2) is reached at 45000 ft (apprx 9 miles
            minDist = min(0.6F , max(0.2F, 0.6F - 0.000009F * range)) * DTR;
        }  // a.s. end
        else
        {
            minDist = 0.2F * DTR;  // old value
        }
    }

    float yaw, pitch;

    // Find out where the missile is looking
    theMissile->GetSeekerPos(&yaw, &pitch);
    theMissile->SetTarget(NULL);

    // Look for a target
    while (curTarget)
    {
        if (fabs(curTarget->localData->az - yaw) < minDist and 
            fabs(curTarget->localData->el - pitch) < minDist and 
            curTarget->BaseData()->IsSim() and 
 not curTarget->BaseData()->IsWeapon() and 
            curTarget->BaseData()->GetVt() <= 60 * KNOTS_TO_FTPSEC) //MI Maverik lockup fix
        {
            theMissile->SetTarget(curTarget);
            theMissile->RunSeeker();

            if (theMissile->targetPtr)
            {
                minDist = (float)min(fabs(curTarget->localData->az - yaw), fabs(curTarget->localData->el - pitch));
            }
        }

        curTarget = curTarget->next;
    }

    curTarget = theMissile->targetPtr;

    if ( not theMissile->targetPtr)
    {
        CheckFeatures(theMissile);
        curTarget = theMissile->targetPtr;
    }

    return curTarget;
}
