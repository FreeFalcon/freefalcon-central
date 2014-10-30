#include "stdhdr.h"
#include "fcc.h"
#include "sms.h"
#include "object.h"
#include "otwdrive.h"
#include "simveh.h"
#include "radar.h"
#include "campwp.h"
#include "simdrive.h"
#include "bomb.h"
#include "missile.h"
#include "misslist.h"
#include "flightData.h"
#include "weather.h"
#include "hud.h" //MI

#include "classtbl.h" //Wombat778 3-12-04

/* 2001-04-12 S.G. BOMBING INACURACY */#include "aircrft.h"
/* 2001-04-12 S.G. BOMBING INACURACY */#include "simbrain.h"

#include "simio.h"  // MD -- 20040111: added for analog cursor support

#include "laserpod.h"

#define NODRAG

static const float DTOS_SLEW_RATE  = 0.05F;
extern float g_fCursorSpeed;
extern bool g_bEnableWindsAloft;
extern float g_fGroundImpactMod;
extern float g_fBombTimeStep;
extern bool g_bBombNumLoopOnly;
extern float g_fHighDragGravFactor;
extern float g_fJDAMLift; //Wombat778 3-12-04
extern float g_fAIJSOWmaxRange; // Cobra
//MI check for pickle
bool Released = FALSE;

//float atanh(double x);

extern SensorClass* FindLaserPod(SimMoverClass* theObject);


BombClass *FireControlComputer::GetTheBomb()
{

    if (Sms and 
        Sms->CurHardpoint() >= 0 and 
        Sms->hardPoint[Sms->CurHardpoint()] and 
        Sms->hardPoint[Sms->CurHardpoint()]->weaponPointer and 
        Sms->hardPoint[Sms->CurHardpoint()]->weaponPointer->IsBomb()) //be EXTRA careful
        return (BombClass *)Sms->hardPoint[Sms->CurHardpoint()]->weaponPointer.get();
    else
        return NULL;
}


void FireControlComputer::AirGroundMode(void)
{
    float dx, dy, dz;

    switch (subMode)
    {
        case CCIP:
            if (masterMode == AirGroundRocket)
                CalculateRocketImpactPoint();
            else
                CalculateImpactPoint();

            FindRelativeImpactPoint();

            if (preDesignate)
            {

                DelayModePipperCorrection();

                if (releaseConsent)
                {
                    DesignateGroundTarget();
                }
                else
                {
                    airGroundDelayTime = 0.0F;
                }
            }
            else if ( not releaseConsent)
            {
                preDesignate = TRUE;
            }

            if ( not preDesignate and not postDrop)
            {
                FindTargetError();
                CheckForBombRelease();
            }

            break;

        case OBSOLETERCKT:
            CalculateRocketImpactPoint();
            FindRelativeImpactPoint();
            dx = platform->XPos() - groundImpactX;
            dy = platform->YPos() - groundImpactY;
            dz = platform->ZPos() - groundImpactZ;
            airGroundRange = (float)sqrt(dx * dx + dy * dy);

            if ( not releaseConsent)
            {
                preDesignate = TRUE;
            }
            else if ( not postDrop and Sms->curWeapon)
            {
                bombPickle = TRUE;
            }

            break;

        case STRAF:
            CalculateImpactPoint();
            FindRelativeImpactPoint();
            dx = platform->XPos() - groundImpactX;
            dy = platform->YPos() - groundImpactY;
            dz = platform->ZPos() - groundImpactZ;
            airGroundRange = (float)sqrt(dx * dx + dy * dy + dz * dz);
            airGroundDelayTime = 0.0F;
            inRange = (airGroundRange < 8000.0F) ? TRUE : FALSE;
            break;

        case CCRP:
            SetDesignatedTarget();

            // Where will it hit?
            if (masterMode == AirGroundRocket)
                CalculateRocketImpactPoint();
            else
                CalculateImpactPoint();

            FindRelativeImpactPoint();
            CalculateReleaseRange();
            FindTargetError(); // Cobra - Get some A/G stats
            break;

        case DTOSS:
            // Put the TD box in the right place
            DTOSMode();
            break;

        case LADD:
            SetDesignatedTarget();
            LADDMode();
            break;

        case MAN:
            if (releaseConsent and not Released)
            {
                // Cobra - Check for JDAM/JSOW ready to be dropped
                BombClass *theBomb;
                theBomb = GetTheBomb();

                if (theBomb and ((AircraftClass*)platform->IsPlayer() and ((AircraftClass *)platform)->AutopilotType() not_eq AircraftClass::CombatAP) and 
                    ((theBomb->EntityType()->classInfo_[VU_STYPE] == STYPE_BOMB_GPS) or
                     (theBomb->EntityType()->classInfo_[VU_STYPE] == STYPE_BOMB_JSOW)) and 
                    ( not Sms->JDAMPowered))
                {
                    releaseConsent = FALSE;
                    Released = TRUE;
                    bombPickle = FALSE;
                }
                else
                {
                    Released = TRUE;
                    bombPickle = TRUE;
                }
            }
            else if ( not releaseConsent and Released)
                Released = FALSE;

            break;
    }

    if ( not releaseConsent)
    {
        postDrop = FALSE;
    }
}

void FireControlComputer::CalculateRocketImpactPoint(void)
{
    float ImpactX = 0.0f;
    float ImpactY = 0.0f;
    float ImpactZ = 0.0f;
    float ImpactTime = 0.0f;

    noSolution = TRUE;

    if (fccWeaponPtr)
    {
        if (fccWeaponPtr->IsMissile())
        {
            noSolution = not ((MissileClass *)fccWeaponPtr.get())->FindRocketGroundImpact(
                             &ImpactX,
                             &ImpactY,
                             &ImpactZ,
                             &ImpactTime);

            groundImpactX = ImpactX;
            groundImpactY = ImpactY;
            groundImpactZ = ImpactZ;
            groundImpactTime = ImpactTime;
        }
        else
        {
            if (fccWeaponPtr->IsLauncher())
            {
                BombClass *lau = (BombClass *)fccWeaponPtr.get();

                // the rocketPointer is used to retain a missile object
                // to be used to compute the impact prediction
                // this object is freed/allocated when the Lau's munition's
                // WeaponId does not match rocketWeaponId

                /* MLR 5/29/2004 - Handled in UpdateWeaponPointer()
                if(lau->LauGetWeaponId() not_eq rocketWeaponId)
                {

                 if(rocketPointer)
                 delete rocketPointer;

                 if(lau->LauGetWeaponId())
                 {
                 rocketPointer = (MissileClass *)InitAMissile(Sms->Ownship(), lau->LauGetWeaponId(), 0);
                 }
                 else
                 {
                 rocketPointer = 0;
                 }
                 rocketWeaponId = lau->LauGetWeaponId();
                }
                */

                if (rocketPointer)
                {
                    noSolution = not rocketPointer->FindRocketGroundImpact(
                                     &ImpactX,
                                     &ImpactY,
                                     &ImpactZ,
                                     &ImpactTime);

                    groundImpactX = ImpactX;
                    groundImpactY = ImpactY;
                    groundImpactZ = ImpactZ;
                    groundImpactTime = ImpactTime;
                }
            }
        }
    }
}

//Wombat778 3-12-04 Check to see if the effective gravity should be modified because of the properties of the bomb.
//Wombat778 3-13-04 Changed to FCC from Sms, and added PlayerFCC check so that AI doesnt drop JDAM's short
float calcgrav(FireControlComputer *FCC)
{
    BombClass *theBomb;

    if (FCC and FCC->PlayerFCC() and FCC->Sms and FCC->Sms->CurHardpoint() >= 0 and 
        FCC->Sms->hardPoint[FCC->Sms->CurHardpoint()] and 
        FCC->Sms->hardPoint[FCC->Sms->CurHardpoint()]->weaponPointer and 
        FCC->Sms->hardPoint[FCC->Sms->CurHardpoint()]->weaponPointer->IsBomb()) //be EXTRA careful
    {
        theBomb = (BombClass *)FCC->Sms->hardPoint[FCC->Sms->CurHardpoint()]->weaponPointer.get();

        if (
            theBomb and not (F4IsBadReadPtr(theBomb, sizeof(BombClass))) and //be EXTRA EXTRA careful
            (theBomb->EntityType()->classInfo_[VU_STYPE] == STYPE_BOMB_GPS) or
            (theBomb->EntityType()->classInfo_[VU_STYPE] == STYPE_BOMB_JSOW)
        )
        {
            // RV - Biker - Never give neg. gravity
            return max(GRAVITY - theBomb->GetJDAMLift(), 0.0f); //The 0.6 value was derived from testing to prevent the CCRP from dropping when the bomb cant actually get to the target
        }
    }

    return GRAVITY;
}


void FireControlComputer::CalculateImpactPoint(void)
{
    float maxTime, timeStep;
#ifndef NODRAG
    float vx, vy;
#endif
    float dx, dy, dz;
    float rx, ry, rz;
    float area;
    float mass;
    float lastX, lastY, lastZ;
    float delta, dragCoeff;
    float xDot, yDot, zDot;
    float a, b, c, t, xt, yt;
    float vdragx, vdragy;
    BombClass *theBomb;
    bool isJSOW = false;

    float grav = calcgrav(this); //Wombat778 3-12-04

    // For now, assume a perfect world, with zero drag, and no wind
    // height = h0 + v0*t + 0.5*a*t*t
    // Solve for t when height = ground height
    // Always use the later t
    groundImpactZ = 0.0F;
    groundImpactTime = 0.0F;

    xDot = platform->XDelta();
    yDot = platform->YDelta();
    zDot = platform->ZDelta();
    //MI externalised
    timeStep = g_fBombTimeStep;

    // COBRA - RED - Rewritten JSOW check
    maxTime = 120.0F;
    theBomb = GetTheBomb();

    if (theBomb and theBomb->EntityType()->classInfo_[VU_STYPE] == STYPE_BOMB_JSOW)
    {
        isJSOW = true;
        maxTime = ((theBomb->GetJSOWmaxRange() * NM_TO_FT) / platform->GetVt()) * 2.2f; // 1200.0F;
    }


    if ( not preDesignate and GetMasterMode() not_eq AirGroundLaser) //MI added check for Laser
    {

        a = 0.5F * grav;
        b = zDot;
        c = platform->ZPos() - groundDesignateZ;

        t = (-b + (float)sqrt(b * b - 4 * a * c)) / (2.0F * a);
        groundImpactX = platform->XPos() + xDot * t;
        groundImpactY = platform->YPos() + yDot * t;
        groundImpactZ = groundDesignateZ;
        groundImpactTime = t;
    }

    // M.N. no "else if" here, or dumb bombs will drop short
    if (Sms->CurHardpoint() >= 0)
    {
        rx = Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponData()->xEjection;
        ry = Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponData()->yEjection;
        rz = Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponData()->zEjection;
        //LRKLUDGE  Replace this with Terrain LOS check when its ready
        xDot += platform->dmx[0][0] * rx + platform->dmx[1][0] * ry + platform->dmx[2][0] * rz;
        yDot += platform->dmx[0][1] * rx + platform->dmx[1][1] * ry + platform->dmx[2][1] * rz;
        zDot += platform->dmx[0][2] * rx + platform->dmx[1][2] * ry + platform->dmx[2][2] * rz;

        area  = Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponData()->area;
        mass  = Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponData()->weight / grav;
        dragCoeff  = Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponData()->cd;

        // edg kludge: drag coeff >= 1.0 is a durandal (w/chute)
        if (dragCoeff >= 1.0f)
            //MI
            //grav *= 0.65f;
            grav *= g_fHighDragGravFactor;


        // dragCoeff = 0.5F * RHOASL * area * dragCoeff;
        // v0Factor = (float)atanh(sqrt(dragCoeff / grav) * zDot);

        float ratx, raty, horiz, tfloor, tceil;

        horiz = (float)sqrt(xDot * xDot + yDot * yDot + 0.1f);
        ratx = (float)fabs(xDot) / horiz;
        raty = (float)fabs(yDot) / horiz;

        // Start position
        dx = platform->XPos();
        dy = platform->YPos();
        dz = platform->ZPos();

        // for 1st sec, there's no drag only gravity
        dx += xDot;
        dy += yDot;

        //dz += zDot + 0.5f * GRAVITY;
        //zDot += GRAVITY;
        dz += zDot + 0.5f * calcgrav(this); //Wombat778 3-12-04
        zDot += calcgrav(this);



        // first we're going to use analytic methods to get us most of
        // the way thru the calculation, and then use numerical integration
        // to get us to the impact point.  First, calc the time it will take
        // for the bomb to drop some pct of the dist to the ground.   Then
        // calc the times where x and y vel vectors will drop to zero due
        // to drag.  Use the smallest of these times for the analytic part to
        // calc the starting position for numerical integration.
        OTWDriver.GetAreaFloorAndCeiling(&tfloor, &tceil);
        a = 0.5F * grav;
        b = zDot;
        // c = (platform->ZPos() - OTWDriver.GetApproxGroundLevel( platform->XPos(), platform->YPos() ) ) * 0.8f;
        c = dz - tceil;

        //MI
        if (g_bBombNumLoopOnly)
        {
            c = 0.0F;
        }
        else if (c > 0.0f)
        {
            c = 0.0f;
        }

        // time to drop some pct of the way down
        t = (-b + (float)sqrt(b * b - 4 * a * c)) / (2.0F * a);

        // now calc the x and y times
        vdragx = dragCoeff * BombClass::dragConstant * ratx;

        if (vdragx not_eq 0.0f)
        {
            xt = (float)fabs(xDot) / (vdragx);

            if (xt < t)
                t = xt;
        }

        vdragy = dragCoeff * BombClass::dragConstant * raty;

        if (vdragy not_eq 0.0f)
        {
            yt = (float)fabs(yDot) / (vdragy);

            if (yt < t)
                t = yt;
        }

        // change x and y drags to appropriate sign (opposite vel)
        if (xDot > 0.0f)
            vdragx = -vdragx;

        if (yDot > 0.0f)
            vdragy = -vdragy;


        dz += zDot * t + 0.5f * grav * t * t;
        dx += xDot * t + 0.5f * vdragx * t * t;
        dy += yDot * t + 0.5f * vdragy * t * t;

        zDot += grav * t;
        yDot += vdragy * t;
        xDot += vdragx * t;

        // numerical integration loop
        vdragx *= timeStep;
        vdragy *= timeStep;
        // add back in the 1st sec
        groundImpactTime = t + 1.0f;

        do
        {
            lastX = dx;
            lastY = dy;
            lastZ = dz;

            dx += xDot * timeStep;
            dy += yDot * timeStep;
            dz += zDot * timeStep;

            if (fabs(xDot) < fabs(vdragx))
            {
                xDot = 0.0f;
            }
            else
            {
                xDot += vdragx;
            }

            if (fabs(yDot) < fabs(vdragy))
            {
                yDot = 0.0f;
            }
            else
            {
                yDot += vdragy;
            }

            zDot += grav * timeStep;
            groundImpactTime += timeStep;

        }
        while (dz <= OTWDriver.GetGroundLevel(dx, dy) and groundImpactTime < maxTime);

        if (groundImpactTime >= maxTime)
            noSolution = TRUE;
        else
            noSolution = FALSE;

        groundImpactZ = OTWDriver.GetGroundLevel(dx, dy);

        // 2002-03-28 MI make this configurable for better adjustments
        // 2001-06-17 ADDED BY S.G. ONLY IF IT'S NOT A GUN
        if (Sms->curWeaponClass not_eq wcGunWpn)
            // 2001-04-14 ADDED BY S.G. KLUDGE TO MAKE THE BOMBS HIT WHERE THEY ARE SUPPOSED TO HIT
            groundImpactZ -= g_fGroundImpactMod;

        // END OF ADDED SECTION

        // Interpolate for the time
        delta = (groundImpactZ - lastZ) / (dz - lastZ);
        groundImpactTime = (groundImpactTime - timeStep) + delta * timeStep;
        groundImpactX = lastX + delta * (dx - lastX);
        groundImpactY = lastY + delta * (dy - lastY);

        // Cobra - test
        dx = groundImpactX - platform->XPos();
        dy = groundImpactY - platform->YPos();
        dz = groundImpactZ - platform->ZPos();
        float GroundRange = (float)SqrtF(dx * dx + dy * dy);
        float GndAngle = (float)(atan2(dy, dx)) * RTD;

        dx = groundDesignateX - platform->XPos();
        dy = groundDesignateY - platform->YPos();
        dz = groundDesignateZ - platform->ZPos();
        float TargetRange = (float)SqrtF(dx * dx + dy * dy);
        float TgtAngle = (float)(atan2(dy, dx)) * RTD;

        dx = groundDesignateX - groundImpactX;
        dy = groundDesignateY - groundImpactY;
        dz = groundDesignateZ - groundImpactZ;
        float TgtGrndRange = (float)SqrtF(dx * dx + dy * dy);

        //me123 lets add the wind change effect to the bomb fall
        //MI
        if (g_bEnableWindsAloft)
        {
            mlTrig trigWind;
            float wind;
            Tpoint pos;
            pos.x = platform->XPos();
            pos.y = platform->YPos();
            pos.z = platform->ZPos();

            // current wind
            mlSinCos(&trigWind, ((WeatherClass*)realWeather)->WindHeadingAt(&pos));
            wind = ((WeatherClass*)realWeather)->WindSpeedInFeetPerSecond(&pos);
            float winddx = trigWind.cos * wind;
            float winddy = trigWind.sin * wind;



            /* //wind at 1/3 the altitude
            pos.z = platform->ZPos()/3f;
            mlSinCos(&trigWind, ((WeatherClass*)realWeather)->WindHeadingAt(&pos));
            wind =  ((WeatherClass*)realWeather)->WindSpeedInFeetPerSecond(&pos);
            float nextwinddx = trigWind.cos * wind;
            float nextwinddy = trigWind.sin * wind;
            */
            float nextwinddx = winddx * 0.5f;
            float nextwinddy = winddy * 0.5f;

            //factor in the change
            static float test = 1.0f;
            groundImpactX += (nextwinddx - winddx) * groundImpactTime * test * 0.5f;
            groundImpactY += (nextwinddy - winddy) * groundImpactTime * test * 0.5f;
            groundImpactZ = OTWDriver.GetGroundLevel(groundImpactX, groundImpactY);
        }
    }
}

void FireControlComputer::FindRelativeImpactPoint(void)
{
    float dx, dy, dz;
    float rx, ry, rz;
    float a, b, c, t;
    //float grav = GRAVITY;
    float grav = calcgrav(this); //Wombat778 3-12-04
    float zDot = platform->ZDelta();
    float winddx = 0.0F;
    float winddy = 0.0F;

    //MI
    // POGOLOOK I think this is already calculated in above and doesn't need to be added again here...
    // with this code, the pipper jumps on the HUD as soon as you designate
    if (g_bEnableWindsAloft)
    {
        mlTrig trigWind;
        float wind;

        // factor in current wind me123
        a = 0.5F * grav;
        b = zDot;
        c = platform->ZPos() - groundDesignateZ;
        t = (-b + (float)sqrt(b * b - 4 * a * c)) / (2.0F * a);


        Tpoint pos;
        pos.x = platform->XPos();
        pos.y = platform->YPos();
        pos.z = platform->ZPos();
        mlSinCos(&trigWind, ((WeatherClass*)realWeather)->WindHeadingAt(&pos));
        wind = ((WeatherClass*)realWeather)->WindSpeedInFeetPerSecond(&pos);
        winddx = trigWind.cos * wind * t;
        winddy = trigWind.sin * wind * t;
    }

    if (preDesignate)
    {
        dx = groundImpactX - platform->XPos() + winddx;
        dy = groundImpactY - platform->YPos() + winddy;
        dz = groundImpactZ - platform->ZPos();

        // Cobra - Rocket calcs are screwed up (dx/dy instead of dy/dx)
        if (subMode == OBSOLETERCKT or subMode == STRAF)
        {
            //----------------------------------
            groundPipperAz = ((float)atan2(dx, dy) - platform->Yaw());
            groundPipperEl = (float)atan(-dz / (float)sqrt(dx * dx + dy * dy + .01F)) - platform->Pitch();
            //----------------------------------
        }
        else
        {
            rx = platform->dmx[0][0] * dx + platform->dmx[0][1] * dy + platform->dmx[0][2] * dz;
            ry = platform->dmx[1][0] * dx + platform->dmx[1][1] * dy + platform->dmx[1][2] * dz;
            rz = platform->dmx[2][0] * dx + platform->dmx[2][1] * dy + platform->dmx[2][2] * dz;
            groundPipperAz = (float)atan2(ry, rx);
            groundPipperEl = (float)atan(-rz / (float)sqrt(rx * rx + ry * ry + .1F));
        }

        //MI
        if (g_bRealisticAvionics and TheHud)
        {
            TheHud->SlantRange = (float)sqrt(dx * dx + dy * dy + dz * dz);
        }
    }
    else
    {
        dx = groundDesignateX - platform->XPos() + winddx;
        dy = groundDesignateY - platform->YPos() + winddy;
        dz = groundDesignateZ - platform->ZPos();

        rx = platform->dmx[0][0] * dx + platform->dmx[0][1] * dy + platform->dmx[0][2] * dz;
        ry = platform->dmx[1][0] * dx + platform->dmx[1][1] * dy + platform->dmx[1][2] * dz;
        rz = platform->dmx[2][0] * dx + platform->dmx[2][1] * dy + platform->dmx[2][2] * dz;

        groundDesignateDroll = (float)atan2(ry, -rz);
        groundDesignateAz = (float)atan2(ry, rx);
        groundDesignateEl = (float)atan(-rz / (float)sqrt(rx * rx + ry * ry + .1F));
        groundPipperAz = groundDesignateAz;
        groundPipperEl = groundDesignateEl;

        //MI
        if (g_bRealisticAvionics and TheHud)
        {
            TheHud->SlantRange = (float)sqrt(dx * dx + dy * dy + dz * dz);
        }
    }

    // groundPipperAz = max ( min ( groundPipperAz, 10.0F * DTR), -10.0F * DTR);
}


// Used only by CCIP when in the predesignate state
void FireControlComputer::DelayModePipperCorrection(void)
{
    float dRoll;
    mlTrig rollTrig;
    static const float LIMIT_PIPPER_EL_MAX =   3.0f * DTR;
    static const float LIMIT_PIPPER_EL_MIN = -13.0f * DTR;
    static const float LIMIT_PIPPER_AZ =   180.0f * DTR;//me123 from 9

    // Compute the angle to the pipper from the boresight cross
    dRoll = (float)atan2(sin(groundPipperAz), sin(groundPipperEl));
    mlSinCos(&rollTrig, dRoll);
    groundPipperOnHud = TRUE;

    // See if the pipper falls out of bounds in elevation
    if (groundPipperEl > LIMIT_PIPPER_EL_MAX)
    {
        // groundPipperAz = atan2( tan(LIMIT_PIPPER_EL_MAX) * tan(groundPipperAz), tan(groundPipperEl) );
        groundPipperEl = LIMIT_PIPPER_EL_MAX;
        groundPipperOnHud = FALSE;
    }
    else if (groundPipperEl < LIMIT_PIPPER_EL_MIN)
    {
        // groundPipperAz = atan2( tan(-LIMIT_PIPPER_EL_MIN) * tan(groundPipperAz), tan(-groundPipperEl) );
        groundPipperEl = LIMIT_PIPPER_EL_MIN;
        groundPipperOnHud = FALSE;
    }

    // See if the pipper falls out of bounds in azmuth
    if (fabs(groundPipperAz) > LIMIT_PIPPER_AZ)
    {
        if (groundPipperAz > LIMIT_PIPPER_AZ)
        {
            // groundPipperEl = atan2( tan(LIMIT_PIPPER_AZ) * tan(groundPipperEl), tan(groundPipperAz) );
            groundPipperAz = LIMIT_PIPPER_AZ;
        }
        else
        {
            // groundPipperEl = atan2( tan(LIMIT_PIPPER_AZ) * tan(groundPipperEl), tan(-groundPipperAz) );
            groundPipperAz = -LIMIT_PIPPER_AZ;
        }

        groundPipperOnHud = FALSE;
    }

#if 0

    // See if the pipper falls off screen
    if (groundPipperEl >  5.0F * DTR or groundPipperEl < -13.0F * DTR or
        groundPipperAz > 10.0F * DTR or groundPipperAz < -10.0F * DTR)
    {
        // This computes a new az and el in the same direction with a 13 degree solid angle
        groundPipperEl = atan(tan(13.0f * DTR) * rollTrig.cos);
        groundPipperAz = atan(tan(13.0f * DTR) * rollTrig.sin);

        groundPipperOnHud = FALSE;
    }

#endif
}


void FireControlComputer::DesignateGroundTarget(void)
{
    float dx, dy, dz;
    float rx, ry, rz;
    euler dir;
    vector pos;

    if (groundPipperOnHud)
    {
        // Just the use impact point we already got
        groundDesignateX = groundImpactX;
        groundDesignateY = groundImpactY;
        groundDesignateZ = groundImpactZ;
        preDesignate = FALSE;
    }
    else
    {
        // Compute where the delay pipper falls on the ground and designate there
        dir.pitch =
            groundPipperEl * platform->platformAngles.cosphi +
            -groundPipperAz * platform->platformAngles.sinphi +
            platform->Pitch()
            ;
        dir.yaw =
            groundPipperEl * platform->platformAngles.sinphi +
            groundPipperAz * platform->platformAngles.cosphi +
            platform->Yaw()
            ;
        dir.roll = platform->Roll();

        if (OTWDriver.GetGroundIntersection(&dir, &pos))
        {
            groundDesignateX = pos.x;
            groundDesignateY = pos.y;
            groundDesignateZ = pos.z;
            preDesignate = FALSE;
        }
    }

    // If we got a valid designation above, compute some useful geometry
    if (preDesignate == FALSE)
    {
        dx = groundDesignateX - platform->XPos();
        dy = groundDesignateY - platform->YPos();
        dz = groundDesignateZ - platform->ZPos();

        rx = platform->dmx[0][0] * dx + platform->dmx[0][1] * dy + platform->dmx[0][2] * dz;
        ry = platform->dmx[1][0] * dx + platform->dmx[1][1] * dy + platform->dmx[1][2] * dz;
        rz = platform->dmx[2][0] * dx + platform->dmx[2][1] * dy + platform->dmx[2][2] * dz;

        groundDesignateAz    = (float)atan2(ry, rx);
        groundDesignateEl    = (float)atan(-rz / (float)sqrt(rx * rx + ry * ry + 0.1f));
        groundDesignateDroll = (float)atan2(ry, -rz);

        dx = groundDesignateX - groundImpactX;
        dy = groundDesignateY - groundImpactY;
        airGroundRange = (float)sqrt(dx * dx + dy * dy);
    }
}

void FireControlComputer::FindTargetError(void)
{
    float dx, dy;
    float rx, ry;
    mlTrig trig;
    float hdg, vel;

    Tpoint pos;

    pos.x = platform->XPos();
    pos.y = platform->YPos();
    pos.z = platform->ZPos();

    hdg = ((WeatherClass*)realWeather)->WindHeadingAt(&pos);
    vel = ((WeatherClass*)realWeather)->WindSpeedInFeetPerSecond(&pos);
    mlSinCos(&trig, hdg);


    dx = groundDesignateX - (platform->XPos() + trig.cos * vel * groundImpactTime);
    dy = groundDesignateY - (platform->YPos() + trig.sin * vel * groundImpactTime);
    //MI What do these lines do here?????????
    //dx = groundDesignateX - groundImpactX;
    //dy = groundDesignateY - groundImpactY;

    rx =  platform->platformAngles.cospsi * dx + platform->platformAngles.sinpsi * dy;
    ry = -platform->platformAngles.sinpsi * dx + platform->platformAngles.cospsi * dy;

    airGroundBearing = (float)atan2(ry, rx);

    airGroundDelayTime = airGroundRange / platform->GetVt();

    dx = groundDesignateX - groundImpactX;
    dy = groundDesignateY - groundImpactY;
    airGroundRange = (float)sqrt(dx * dx + dy * dy);
}

void FireControlComputer::CheckForBombRelease(void)
{
    float tmpTime;

    // Set some flags to make us look like CCRP
    tossAnticipationCue = AwaitingRelease;
    airGroundMaxRange = -1.0F;

    // How long to null the impact point error assuming level flight toward the target?
    tmpTime = airGroundRange / platform->GetVt();

    if ((tmpTime < 0.1F or (airGroundDelayTime > 0.0 and 
                            airGroundDelayTime < 0.4F and tmpTime > airGroundDelayTime)) and 
        Sms->curWeapon)
    {
        bombPickle = TRUE;
        airGroundDelayTime = 0.0F;
    }
    else
        airGroundDelayTime = tmpTime;
}

void FireControlComputer::SetDesignatedTarget(void)
{
    float dx, dy, dz;
    float rx, ry, rz;
    RadarClass* theRadar = (RadarClass*) FindSensor(platform, SensorClass::Radar);
    WayPointClass* curWaypoint = platform->curWaypoint;

    if (targetPtr and targetPtr->BaseData()->OnGround())
    {
        groundDesignateX = targetPtr->BaseData()->XPos();
        groundDesignateY = targetPtr->BaseData()->YPos();
        // edg: sigh.  ground targets can't be trusted to have a valid Z
        // groundDesignateZ = targetPtr->BaseData()->ZPos();
        groundDesignateZ = OTWDriver.GetGroundLevel(groundDesignateX, groundDesignateY);
    }
    else if (theRadar and theRadar->CurrentTarget())
    {
        groundDesignateX = theRadar->CurrentTarget()->BaseData()->XPos();
        groundDesignateY = theRadar->CurrentTarget()->BaseData()->YPos();
        // edg: sigh.  ground targets can't be trusted to have a valid Z
        // groundDesignateZ = targetPtr->BaseData()->ZPos();
        groundDesignateZ = OTWDriver.GetGroundLevel(groundDesignateX, groundDesignateY);
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

    //
    // What happens if none of the above conditions are met?  Could/should that ever happen?
    // Can one of the conditions be removed and used as a default case?
    //
    // COBRA - RED - Rewritten GPS guided bombs check
    int bGPS = FALSE;
    BombClass *theBomb = GetTheBomb();

    if (theBomb and (theBomb->EntityType()->classInfo_[VU_STYPE] == STYPE_BOMB_GPS or
                    theBomb->EntityType()->classInfo_[VU_STYPE] == STYPE_BOMB_JSOW))
        bGPS = TRUE;

    // 2001-04-12 ADDED BY S.G. NEED TO MAKE A VARIABLE DESIGNATE BASED ON SKILL AND ALTITUDE
    if (platform->IsAirplane() and not bGPS and 
        (((AircraftClass *)platform)->IsDigital() or
         ((AircraftClass *)platform)->AutopilotType() == AircraftClass::CombatAP))
    {
        // Intentionnaly, I didn't play with the sign of ZPos because it will be 'below' groundDesignateZ and therefore - - will make it + and the end value will be positive
        groundDesignateX += (float)((5 - platform->Brain()->SkillLevel()) * xBombAccuracy) * ((groundDesignateZ - platform->ZPos()) / 10000.0f);
        groundDesignateY += (float)((5 - platform->Brain()->SkillLevel()) * yBombAccuracy) * ((groundDesignateZ - platform->ZPos()) / 10000.0f);
    }

    // END OF ADDED SECTION

    // Cobra - Make sure the current JSOW target is a target WP target.
    if (theBomb and theBomb->EntityType()->classInfo_[VU_STYPE] == STYPE_BOMB_JSOW)
    {
        curWaypoint->GetLocation(&groundDesignateX, &groundDesignateY, &groundDesignateZ);
        groundDesignateZ = OTWDriver.GetGroundLevel(groundDesignateX, groundDesignateY);
    }

    dx = groundDesignateX - platform->XPos();
    dy = groundDesignateY - platform->YPos();
    dz = groundDesignateZ - platform->ZPos();

    rx = platform->dmx[0][0] * dx + platform->dmx[0][1] * dy + platform->dmx[0][2] * dz;
    ry = platform->dmx[1][0] * dx + platform->dmx[1][1] * dy + platform->dmx[1][2] * dz;
    rz = platform->dmx[2][0] * dx + platform->dmx[2][1] * dy + platform->dmx[2][2] * dz;

    groundDesignateAz    = (float)atan2(ry, rx);
    groundDesignateEl    = (float)atan(-rz / (float)sqrt(rx * rx + ry * ry + 0.1f));
    groundDesignateDroll = (float)atan2(ry, -rz);
}

void FireControlComputer::CalculateReleaseRange(void)
{
    float xDot, yDot, zDot;
    float tossAngle, curRange;
    float dx, dy, rx, ry, radius;
    float minRange = 100000.0F;
    float tmpTime;
    float maxDelay;
    int wayTooFar = FALSE;
    bool isJSOW = false;
    bool isJDAM = false;
    TossAnticipation lastCue = tossAnticipationCue;
#ifdef NODRAG
    float a, b, c, t;
#endif
    mlTrig trig;
    float hdg, vel;
    BombClass *theBomb;

    theBomb = GetTheBomb();

    if (theBomb and theBomb->EntityType()->classInfo_[VU_STYPE] == STYPE_BOMB_JSOW)
    {
        isJSOW = true;
    }

    if (theBomb and theBomb->EntityType()->classInfo_[VU_STYPE] == STYPE_BOMB_GPS)
    {
        isJDAM = true;
    }

    if (isJSOW or isJDAM)
    {
        // RV - Biker - If we have a locked target on laser pod go to TOO
        LaserPodClass* laserPod = (LaserPodClass*)FindLaserPod(platform);

        if (laserPod and laserPod->IsLocked() and Sms->JDAMtargeting == SMSBaseClass::TOO
           and ((AircraftClass *)platform)->IsPlayer()
           and (((AircraftClass *)platform)->AutopilotType() not_eq AircraftClass::CombatAP))
        {
            ((AircraftClass*)platform)->JDAMAllowAutoStep = false;
            float lpdX;
            float lpdY;
            float lpdZ;
            laserPod->GetTargetPosition(&lpdX, &lpdY, &lpdZ);
            ((AircraftClass*)platform)->JDAMtgtPos.x = lpdX;
            ((AircraftClass*)platform)->JDAMtgtPos.y = lpdY;
            ((AircraftClass*)platform)->JDAMtgtPos.z = lpdZ;
        }
    }

    Tpoint pos;
    pos.x = platform->XPos();
    pos.y = platform->YPos();
    pos.z = platform->ZPos();

    if (g_bEnableWindsAloft)
    {
        hdg = ((WeatherClass*)realWeather)->WindHeadingAt(&pos);
        vel = ((WeatherClass*)realWeather)->WindSpeedInFeetPerSecond(&pos);
        mlSinCos(&trig, hdg);

        dx = groundDesignateX - (platform->XPos() + trig.cos * vel * groundImpactTime);
        dy = groundDesignateY - (platform->YPos() + trig.sin * vel * groundImpactTime);
    }
    else
    {
        dx = groundDesignateX - platform->XPos();
        dy = groundDesignateY - platform->YPos();
    }

    predictedClimbAngle = 10.0F;
    predictedReleaseAltitude = -platform->ZPos();

    rx =  platform->platformAngles.cospsi * dx + platform->platformAngles.sinpsi * dy;
    ry = -platform->platformAngles.sinpsi * dx + platform->platformAngles.cospsi * dy;

    airGroundBearing = (float)atan2(ry, rx);
    dx = groundDesignateX - groundImpactX;
    dy = groundDesignateY - groundImpactY;
    airGroundRange = (float)sqrt(dx * dx + dy * dy);

    if (airGroundRange < minRange)
        minRange = airGroundRange;

    curRange = (float)sqrt(
                   (groundDesignateX - platform->XPos()) * (groundDesignateX - platform->XPos()) +
                   (groundDesignateY - platform->YPos()) * (groundDesignateY - platform->YPos()));

    // Bomb Range w/ loft  Assume 45 Degree Toss.
    if ((Sms->CurHardpoint() >= 0) and (Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponData()->flags bitand SMSClass::Loftable))
    {
        //MI
        if ( not g_bRealisticAvionics or not playerFCC or (((AircraftClass *)platform)->AutopilotType() == AircraftClass::CombatAP))
            tossAngle = 45.0F * DTR;
        else
        {
            if (Sms)
                //tossAngle = Sms->angle * DTR;
                tossAngle = Sms->GetAGBReleaseAngle() * DTR;
            else
                tossAngle = 45.0F * DTR;
        }
    }
    else
    {
        tossAngle = 0.0F;
    }

    tossAnticipationCue = NoCue;

    if ((Sms->CurHardpoint() >= 0) and (Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponData()->flags bitand SMSClass::Loftable))
    {
        // 4 G pull-up to release

        //radius = platform->GetVt() * platform->GetVt() / (4.0F * GRAVITY);
        radius = platform->GetVt() * platform->GetVt() / (4.0F * calcgrav(this)); //Wombat778 3-12-04


#ifdef NODRAG
        mlSinCos(&trig, tossAngle);
        xDot = platform->GetVt() * trig.cos * platform->platformAngles.cospsi;
        yDot = platform->GetVt() * trig.cos * platform->platformAngles.sinpsi;
        zDot = -platform->GetVt() * trig.sin;

        //a = 0.5F * GRAVITY;
        a = 0.5F * calcgrav(this); //Wombat778 3-12-04


        // Calculate level release range
        c = platform->ZPos() - groundDesignateZ;
        // Can't throw bomb up, so clamp
        c = min(c, 0.0F);

        t = (float)SqrtF(FabsF(- 4 * a * c)) / (2.0F * a);
        airGroundMinRange = (float)SqrtF(xDot * t * xDot * t + yDot * t * yDot * t);

        if (theBomb and isJSOW)
            missileWEZDisplayRange = theBomb->GetJSOWmaxRange() * NM_TO_FT;
        else
            missileWEZDisplayRange = 10.0F * NM_TO_FT;

        // Calc max range
        b = zDot;
        c = platform->ZPos() - groundDesignateZ - radius * trig.sin;

        t = (-b + (float)SqrtF(b * b - 4 * a * c)) / (2.0F * a);

        airGroundMaxRange = (float)SqrtF(xDot * t * xDot * t + yDot * t * yDot * t);
        airGroundMaxRange += radius * trig.cos;

        // RV - Biker
        if (isJSOW)
        {
            float radical = (float)sqrt(2.0F * GRAVITY * (groundDesignateZ - platform->ZPos()));
            float altFactor = (groundDesignateZ - platform->ZPos() - 10000.0f) / 25000.0f + 0.50f;
            altFactor = min(altFactor, 1.0f) / 11.0f;
            float speedFactor = platform->GetVt() * radical / max(calcgrav(this), 0.5f);
            float angleFactor = cos(min(abs(groundDesignateAz), 90.0f * DTR) * 2.0f);

            airGroundMaxRange = max(speedFactor * altFactor * angleFactor, 2.0f * NM_TO_FT);
            airGroundMinRange = 2.0f * NM_TO_FT;
        }

        // 2001-05-23 ADDED BY S.G. FUDGE THE airGroundMinRange AND airGroundMaxRange TO ACCOUNT FOR PSEUDO DRAG :-(
        static float minFudgeValue = 2.0f;
        static float maxFudgeValue1 = -7000.0f;
        static float maxFudgeValue2 = 0.1f;
        static float maxFudgeValue3 = 910.0f;
        static float maxFudgeValue4 = 30.0f;

        airGroundMinRange -= platform->GetVt() * minFudgeValue;
        //    float tmp1AirGroundMaxRange = airGroundMaxRange - (platform->GetVt() * platform->GetVt() * (float)sqrt(platform->GetVt()) / maxFudgeValue1);
        // This was hard to come up with. This 'formula' brings low level tossing in par with the Cher Min's Low level tossing chart.
        airGroundMaxRange += (maxFudgeValue3 - platform->GetVt()) * maxFudgeValue4;

        // END OF ADDED SECTION
#else
#error "Not implemented"
#endif

        t = (curRange - airGroundMaxRange) / platform->GetVt();

        if ( not isJSOW)
        {
            if (t >= 10.0F)
            {
                tmpTime = (curRange - airGroundMaxRange) / platform->GetVt();
                wayTooFar = TRUE;
            }
            else if (t >= 2.0F)
            {
                tmpTime = (curRange - airGroundMaxRange) / platform->GetVt();
                tossAnticipationCue = EarlyPreToss;
            }
            else if (t >= 0.0F)
            {
                tmpTime = (curRange - airGroundMaxRange) / platform->GetVt();
                tossAnticipationCue = PreToss;
            }
            else if (t >= -2.0F)
            {
                tmpTime = airGroundRange / platform->GetVt();
                tossAnticipationCue = PullUp;
            }
            else
            {
                tmpTime = airGroundRange / platform->GetVt();
                tossAnticipationCue = AwaitingRelease;
            }
        }
        else
        {
            // Cobra - JSOW
            tmpTime = airGroundRange / platform->GetVt();
            tossAnticipationCue = AwaitingRelease;
        }
    }
    else
    {
        tmpTime = airGroundRange / platform->GetVt();
        airGroundMaxRange = -1.0F;
    }

    // Let LGB's be dropped up to 10 closer than max TOF
    if (masterMode == AirGroundLaser)
    {
        maxDelay = 10.0F;
    }
    else
    {
        maxDelay = 0.5F;
    }

    if (tossAnticipationCue == NoCue or
        (tossAnticipationCue == AwaitingRelease and tossAnticipationCue == lastCue)) // AwaitingRelease = lastCue
    {
        if ( not wayTooFar and releaseConsent and not postDrop and ((tmpTime < 0.1F or
                (airGroundDelayTime > 0.0 and airGroundDelayTime < maxDelay and tmpTime > airGroundDelayTime)) or
                bombReleaseOverride) and Sms->curWeapon)
        {
            bombPickle = TRUE;
        }
    }

    airGroundDelayTime = tmpTime;

    // RV - Biker - Launch JSOW
    if (isJSOW and releaseConsent and not postDrop and Sms->curWeapon and Sms->JDAMPowered and Sms->JDAMInitTimer <= 4.0f and abs(groundDesignateAz) < 45.0f * DTR)
        bombPickle = TRUE;
}

//float atanh(double x)
//{
//    return (float)(0.5F * log((1 + x) / (1 - x)));
//}

void FireControlComputer::DTOSMode(void)
{
    float pitch, yaw, tmpX, tmpY, tmpZ;
    float dx, dy, dz, rx, ry, rz;
    float xMove = 0.0F, yMove = 0.0F;
    mlTrig trig;

    // Drop Track on command
    if (dropTrackCmd)
    {
        preDesignate = TRUE;
        groundPipperAz = 0.0F;
        groundPipperEl = 0.0F;
    }

    // Move the cursors as needed
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

    groundPipperEl += yMove * g_fCursorSpeed * DTOS_SLEW_RATE * SimLibMajorFrameTime;
    groundPipperAz += xMove * g_fCursorSpeed * DTOS_SLEW_RATE * SimLibMajorFrameTime;

    if (preDesignate or (cursorXCmd not_eq 0) or (cursorYCmd not_eq 0))
    {
        groundDesignateAz = -cockpitFlightData.beta * DTR + cockpitFlightData.windOffset * platform->platformAngles.cosphi;
        groundDesignateAz += groundPipperAz;
        groundDesignateEl = -cockpitFlightData.alpha * DTR + cockpitFlightData.windOffset * platform->platformAngles.sinphi;
        groundDesignateEl += groundPipperEl;
        groundDesignateDroll = (float)atan2(sin(groundDesignateAz), sin(groundDesignateEl));

        if (releaseConsent or designateCmd or not preDesignate)
        {
            // Convert from the body relative flight path marker angles to world space pitch/yaw
            mlSinCos(&trig, platform->Roll());
            yaw   = platform->Yaw();
            pitch = platform->Pitch();
            pitch += groundDesignateEl * trig.cos - groundDesignateAz * trig.sin;
            yaw   += groundDesignateEl * trig.sin + groundDesignateAz * trig.cos;

            if (FindGroundIntersection(pitch, yaw, &tmpX, &tmpY, &tmpZ))
            {
                groundDesignateX = tmpX;
                groundDesignateY = tmpY;
                groundDesignateZ = tmpZ;
                preDesignate = FALSE;
            }
        }
    }

    // Do post designate work
    if ( not preDesignate)
    {
        if (masterMode == AirGroundRocket)
            CalculateRocketImpactPoint();
        else
            CalculateImpactPoint();

        dx = groundDesignateX - platform->XPos();
        dy = groundDesignateY - platform->YPos();
        dz = groundDesignateZ - platform->ZPos();

        rx = platform->dmx[0][0] * dx + platform->dmx[0][1] * dy + platform->dmx[0][2] * dz;
        ry = platform->dmx[1][0] * dx + platform->dmx[1][1] * dy + platform->dmx[1][2] * dz;
        rz = platform->dmx[2][0] * dx + platform->dmx[2][1] * dy + platform->dmx[2][2] * dz;

        groundDesignateDroll = (float)atan2(ry, -rz);
        groundDesignateAz = (float)atan2(ry, rx);
        groundDesignateEl = (float)atan(-rz / (float)sqrt(rx * rx + ry * ry + .1F));

        CalculateReleaseRange();
    }
    else
    {
        airGroundDelayTime = 0.0F;
    }
}
void FireControlComputer::LADDMode(void)
{
#if 0
    float pitch, yaw, tmpX, tmpY, tmpZ;
    float dx, dy, dz, rx, ry, rz;
    float xMove = 0.0F, yMove = 0.0F;
    mlTrig trig;

    // Drop Track on command
    if (dropTrackCmd)
    {
        preDesignate = TRUE;
        groundPipperAz = 0.0F;
        groundPipperEl = 0.0F;
    }

    // Move the cursors as needed
    if (cursorXCmd not_eq 0) or (cursorYCmd not_eq 0)
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

    groundPipperEl += yMove * g_fCursorSpeed * DTOS_SLEW_RATE * SimLibMajorFrameTime;
    groundPipperAz += xMove * g_fCursorSpeed * DTOS_SLEW_RATE * SimLibMajorFrameTime;

    if (preDesignate or (cursorXCmd not_eq 0) or (cursorYCmd not_eq 0))
    {
        groundDesignateAz = -cockpitFlightData.beta * DTR + cockpitFlightData.windOffset * platform->platformAngles.cosphi;
        groundDesignateAz += groundPipperAz;
        groundDesignateEl = -cockpitFlightData.alpha * DTR + cockpitFlightData.windOffset * platform->platformAngles.sinphi;
        groundDesignateEl += groundPipperEl;
        groundDesignateDroll = (float)atan2(sin(groundDesignateAz), sin(groundDesignateEl));

        if (releaseConsent or designateCmd or not preDesignate)
        {
            // Convert from the body relative flight path marker angles to world space pitch/yaw
            mlSinCos(&trig, platform->Roll());
            yaw   = platform->Yaw();
            pitch = platform->Pitch();
            pitch += groundDesignateEl * trig.cos - groundDesignateAz * trig.sin;
            yaw   += groundDesignateEl * trig.sin + groundDesignateAz * trig.cos;

            if (FindGroundIntersection(pitch, yaw, &tmpX, &tmpY, &tmpZ))
            {
                groundDesignateX = tmpX;
                groundDesignateY = tmpY;
                groundDesignateZ = tmpZ;
                preDesignate = FALSE;
            }
        }
    }

    // Do post designate work
    if ( not preDesignate)
    {
        CalculateImpactPoint();

        dx = groundDesignateX - platform->XPos();
        dy = groundDesignateY - platform->YPos();
        dz = groundDesignateZ - platform->ZPos();

        rx = platform->dmx[0][0] * dx + platform->dmx[0][1] * dy + platform->dmx[0][2] * dz;
        ry = platform->dmx[1][0] * dx + platform->dmx[1][1] * dy + platform->dmx[1][2] * dz;
        rz = platform->dmx[2][0] * dx + platform->dmx[2][1] * dy + platform->dmx[2][2] * dz;

        groundDesignateDroll = (float)atan2(ry, -rz);
        groundDesignateAz = (float)atan2(ry, rx);
        groundDesignateEl = (float)atan(-rz / (float)sqrt(rx * rx + ry * ry + .1F));

        CalculateLADDReleaseRange();
    }
    else
    {
        airGroundDelayTime = 0.0F;
    }

#endif
}
void FireControlComputer::CalculateLADDReleaseRange(void)
{
#if 0
    float xDot, yDot, zDot;
    float laddAngle = 0.0F;
    float curRange;
    float dx, dy, rx, ry, radius;
    float minRange = 100000.0F;
    float tmpTime;
    float maxDelay;
    int wayTooFar = FALSE;
    LADDAnticipation lastCue = laddAnticipationCue;

    float a, b, c, t;

    mlTrig trig;
    float hdg, vel;

    hdg = ((WeatherClass*)realWeather)->WindHeadingAt(&pos);
    vel = ((WeatherClass*)realWeather)->WindSpeedInFeetPerSecond(&pos);
    mlSinCos(&trig, hdg);

    dx = groundDesignateX - (platform->XPos() + trig.cos * vel * groundImpactTime);
    dy = groundDesignateY - (platform->YPos() + trig.sin * vel * groundImpactTime);

    rx =  platform->platformAngles.cospsi * dx + platform->platformAngles.sinpsi * dy;
    ry = -platform->platformAngles.sinpsi * dx + platform->platformAngles.cospsi * dy;

    airGroundBearing = (float)atan2(ry, rx);
    dx = groundDesignateX - groundImpactX;
    dy = groundDesignateY - groundImpactY;
    airGroundRange = (float)sqrt(dx * dx + dy * dy);

    if (airGroundRange < minRange)
        minRange = airGroundRange;

    //Distance from target to us
    curRange = (float)sqrt(
                   (groundDesignateX - platform->XPos()) * (groundDesignateX - platform->XPos()) +
                   (groundDesignateY - platform->YPos()) * (groundDesignateY - platform->YPos()));

    laddAnticipationCue = NoLADDCue;

    if ((Sms->CurHardpoint() >= 0) and (Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponData()->flags bitand SMSClass::Loftable))
    {

        //OK, so it's loftable. Since we want to set a distance from the TGT where we want to make our
        //pull up, the angle at which we're going to toss is the thing to find.
        //we know:
        //--range from designated point to us.
        //--the bearing from us to the TGT (will be used by HUD)
        //--the range from us in the air to the TGT down there.
        //NEEDED: pitch to release the weapon

        //laddAngle = 45.0F * DTR;

        //Let's find out big our radius is
        // 4 G pull-up to release
        //radius = platform->GetVt() * platform->GetVt() / (4.0F * GRAVITY);
        radius = platform->GetVt() * platform->GetVt() / (4.0F * calcgrav(this)); //Wombat778 3-12-04

        //Let's get the distance from the TGT to where we should release out weapons
        float ReleaseRange = curRange - SafetyDistance; //SafetyDistance should be settable thru the MFD

        //add the distance of our radius. Now we know at what distance we should start our pull up
        //ReleaseRange += radius * trig.cos;

        /*mlSinCos (&trig, laddAngle);
        xDot = platform->GetVt() * trig.cos * platform->platformAngles.cospsi;
        yDot = platform->GetVt() * trig.cos * platform->platformAngles.sinpsi;
        zDot = -platform->GetVt() * trig.sin;

        a = 0.5F * GRAVITY;*/

        // Calculate level release range
        //c = platform->ZPos() - groundDesignateZ;
        // Can't throw bomb up, so clamp
        //c = min (c, 0.0F);

        //NOT USED
        /*t = (float)sqrt(- 4 * a * c) / (2.0F * a);
        airGroundMinRange = (float)sqrt(xDot*t * xDot*t + yDot*t * yDot*t);*/

        // Calc max range
        //b = zDot;
        //c = platform->ZPos() - groundDesignateZ - radius * trig.sin;

        //t = (-b + (float)sqrt(b * b - 4 * a * c)) / (2.0F * a);


        //airGroundMaxRange = (float)sqrt(xDot*t * xDot*t + yDot*t * yDot*t);
        //airGroundMaxRange += radius * trig.cos;

        t = (curRange - ReleaseRange) / platform->GetVt();

        if (t >= 10.0F)
        {
            tmpTime = (curRange - ReleaseRange) / platform->GetVt();
            wayTooFar = TRUE;
        }
        else if (t >= 2.0F)
        {
            tmpTime = (curRange - ReleaseRange) / platform->GetVt();
            laddAnticipationCue = EarlyPreLADD;
        }
        else if (t >= 0.0F)
        {
            tmpTime = (curRange - ReleaseRange) / platform->GetVt();
            laddAnticipationCue = PreLADD;
        }
        else if (t >= -2.0F)
        {
            tmpTime = airGroundRange / platform->GetVt();
            laddAnticipationCue = LADDPullUp;
        }
        else
        {
            tmpTime = airGroundRange / platform->GetVt();
            laddAnticipationCue = LADDAwaitingRelease;
        }
    }
    else
    {
        tmpTime = airGroundRange / platform->GetVt();
        airGroundMaxRange = -1.0F;
    }

    maxDelay = 0.5F;

    if (laddAnticipationCue == NoCue or
        (laddAnticipationCue == AwaitingRelease and laddAnticipationCue == lastCue))
    {
        if ( not wayTooFar and releaseConsent and not postDrop and 
            ((tmpTime < 0.1F or (airGroundDelayTime > 0.0 and airGroundDelayTime < maxDelay and 
                                 tmpTime > airGroundDelayTime)) or bombReleaseOverride) and Sms->curWeapon)
        {
            bombPickle = TRUE;
        }
    }

    airGroundDelayTime = tmpTime;
#endif
}

