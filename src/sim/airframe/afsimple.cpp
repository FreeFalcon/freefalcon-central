/*
** Name: AFSIMPLE.CPP
** Description:
** Contains a very simple flight model for AI and autopilot aircraft
** control.
*/
#include "stdhdr.h"
#include "airframe.h"
#include "aircrft.h"
#include "otwdrive.h"
#include "simdrive.h"
#include "camp2sim.h"
#include "Graphics/Include/tmap.h"
#include "Graphics/Include/rviewpnt.h"  // to get ground type
#include "MsgInc/DamageMsg.h"
#include "Graphics/Include/terrtex.h"
#include "fack.h"
#include "fsound.h"
#include "soundfx.h"
#include "find.h"
#include "atcbrain.h"
#include "playerop.h"
#include "arfrmdat.h"
#include "tankbrn.h"

#include "digi.h"

extern float gSpeedyGonzales;
// hack for making sure simple model STAYS SET
extern BOOL playerFlightModelHack;

extern bool g_bSimpleFMUpdates; // JB 010805
extern bool g_bTankerFMFix;

extern AeroDataSet *aeroDataset;

// for SIMPLE model
#define MAX_AF_PITCH ( DTR * 89.0f )
#define MAX_AF_ROLL ( DTR * 89.0f )
#define MAX_AF_YAWRATE ( DTR * 20.0f )
#define MAX_AF_FPS ( 450.0f * KNOTS_TO_FTPSEC )

#define A1   (1.5F) // integration constant
#define A2   (0.5F) // integration constant
#define B1   (1.0F - 1.5F) // integration constant
#define B2   (1.0F - 0.5F) // integration constant

/*
** SetSimpleMode
** Description:
** Tell af to use simple mode
*/
void
AirframeClass::SetSimpleMode(int mode)
{
    if (mode == simpleMode)
        return;

    if (simpleMode not_eq SIMPLE_MODE_OFF and mode == SIMPLE_MODE_OFF)
    {
        Reinit();
        // MonoPrint("Reinit() Called \n");
    }

    simpleMode = mode;
}

/*
** SimpleModel
** Description:
** A very simple flight model for aircraft that the AI and autopilot
** will fly
*/
void
AirframeClass::SimpleModel(void)
{
    float tmp;

    // "springy" constants for pitch and roll
    float kPitch = 0.75f;
    float kRoll = 1.0f;

    float ctlpitch;
    float ctlroll;

    float dT;
    float newVt;
    float maxBank;
    float maxTheta;
    float liftO_alp = (0.5F * rho * vt * vt * area * clalph0);
    float gmmaDes;
    float rotate = 0.0f;

    // get inputs
    ctlpitch = pstick;
    ctlroll = rstick;



    dT = SimLibMajorFrameTime;


    // NOTE:
    // p     = roll rate (around X axis)
    // q     = pitch rate (around Y axis)
    // r     = yaw rate (around Z axis)
    // phi   = roll (around X axis)
    // theta = pitch (around Y axis)
    // psi   = yaw (around Z axis)

    // handle body rates -- flying
    if (IsSet(InAir))
    {
        // we're not nose planted
        ClearFlag(Planted);
        ClearFlag(NoseSteerOn);

        // pitch rate
        // Modify pitch rate if going really slow
        if (qsom * cnalpha < 1.5F and not (playerFlightModelHack and 
                                       platform == SimDriver.GetPlayerEntity() and 
                                       platform->AutopilotType() == AircraftClass::APOff))
        {
            gmmaDes = (1.0F - qsom * cnalpha / 1.5F) * -45.0F * DTR;
        }
        else
        {
            gmmaDes = 0.0F;
        }

        if (ctlpitch)
        {
            // pitch where we want to be
            maxTheta = min(MAX_AF_PITCH, aeroDataset[vehicleIndex].inputData[AeroDataSet::ThetaMax]);
            tmp = ctlpitch * MAX_AF_PITCH;
            q = (tmp - gmma + gmmaDes) * kPitch;
        }
        else
        {
            // snap back to level flight when no input
            q = (-gmma + gmmaDes) * kPitch;
        }

        // roll rate and yawrate are tied together
        // NOTE: Roll is only +/- 90, if we want to fly inverted we'll likely
        // want to have a specific call to set inverted
        if (ctlroll)
        {
            //it looks like we're sliding around the turns, need to be rolled more
            maxBank = min(MAX_AF_ROLL, aeroDataset[vehicleIndex].inputData[AeroDataSet::MaxRoll]);

            //me123 dont bank more then you can keep the nose up with your max gs availeble
            if (g_bSimpleFMUpdates and gearPos < 0.7F and platform->DBrain()->GetCurrentMode() not_eq DigitalBrain::LandingMode and platform->DBrain()->GetCurrentMode() not_eq DigitalBrain::RefuelingMode)
                maxBank = min(maxBank, acos(1 / (max(min(curMaxGs, gsAvail), 1.5f))));

            tmp = ctlroll * maxBank * 1.2F;
            p = (tmp - mu) * kRoll;

            // maximum yaw rate will occur at +/-90 deg or when the ctrl is
            //ME123 this eliminates the skiddign in the turns
            float turnangle = phi;

            if (turnangle < -maxBank)
                turnangle = -maxBank;

            if (turnangle > maxBank)
                turnangle = maxBank;

            //me123 if the tanker is refuling a player then use FMupdate
            // 2002-04-02 MN only use that when tanker enters the track pattern (reached first track point), not when heading for the first trackpoint
            // JPG 15 Jan 04 - Fixed major crackhead tanker turns...what a big pile of goo
            // We want tankers to turn smooth ALL the time (except when landing) so in MP we aren't chasing them when they turn back like an F-15 to the first
            // track point.
            if ((g_bTankerFMFix and platform->DBrain() and platform->DBrain()->IsTanker() and 
                 platform->TBrain() /* and platform->TBrain()->IsSet(TankerBrain::IsRefueling)  and 
 platform->TBrain()->ReachedFirstTrackPoint() */) or
                (g_bSimpleFMUpdates and vt > 1 and gearPos < 0.7F and 
                 platform->DBrain()->GetCurrentMode() not_eq DigitalBrain::LandingMode and 
                 platform->DBrain()->GetCurrentMode() not_eq DigitalBrain::RefuelingMode))
                r = (360.0f * GRAVITY * tan(turnangle) / (2 * PI * vt)) * DTR;
            else
                r = (ctlroll * MAX_AF_YAWRATE);
        }
        else
        {
            // snap roll back to level flight
            p = (-mu) * kRoll;

            // slow and stop yawing
            r = -r;

            if (r < 2.0F * DTR)
                r = 0.0F;
        }

        if ( not (playerFlightModelHack and 
              platform == SimDriver.GetPlayerEntity() and 
              platform->AutopilotType() == AircraftClass::APOff))
        {

            q -= max(1.0F - (qsom * cnalpha / 0.8F), 0.0F) * (float)atan(platform->platformAngles.cosmu * platform->platformAngles.cosgam * GRAVITY / max(vt, 4.0F));
        }
    }
    // handle body rates -- on ground with nose planted
    else
    {
        // snap any roll back to level
        p = -mu;

        // yaw rate coupled to lateral stick and speed
        ctlroll = max(min(2.0F * ctlroll, 1.0F), -1.0F);
        r = ctlroll * min(vt / 25.0F, 1.0F);

        // pitch rate
        // only can pull back on ground when we can achieve 1G with 10 degrees AOA
        //note: we want to make sure the plane takes off, so even if we couldn't
        //get off the ground, we lift off
        //if ( ctlpitch > 0.0f and (-zaero > GRAVITY or oldp03[2] == 13.0F) ) Cobra old rotation code
        float cltakeoff = Math.TwodInterp(mach, 12.0f, aeroData->mach, aeroData->alpha,
                                          aeroData->clift, aeroData->numMach,
                                          aeroData->numAlpha, &curMachBreak, &curAlphaBreak) *
                          aeroData->clFactor;
        cltakeoff *= (1 + tefFactor * auxaeroData->CLtefFactor);
        rotate = 17.16f * sqrt((weight / area) / fabs(cltakeoff)); //cobra
        ctlpitch += 0.1f;
        int tstatusf = platform->DBrain()->ATCStatus();

        if (vcas > rotate and tstatusf > tTaxi)  //cobra
        {
            // pitch where we want to be
            tmp = ctlpitch * MAX_AF_PITCH;
            q = (tmp - gmma) * kPitch;
        }
        else if (vcas > 125.0f)
        {
            q = 0.005f;
        }
        else
        {
            // snap back to level when no input
            q = (-gmma);
        }
    }

    // integrate angular velocities to roll, pitch and yaw
    mu = mu + dT * p;
    sigma = sigma + dT * r;
    gmma = gmma + dT * q;

    if (mu > MAX_AF_ROLL)
    {
        mu = MAX_AF_ROLL;
        p = 0.0f;
    }
    else if (mu < -MAX_AF_ROLL)
    {
        mu = -MAX_AF_ROLL;
        p = 0.0f;
    }

    if (gmma > MAX_AF_PITCH)
    {
        gmma = MAX_AF_PITCH;
        q = 0.0f;
    }
    else if (gmma < -MAX_AF_PITCH)
    {
        gmma = -MAX_AF_PITCH;
        q = 0.0f;
    }

    if (sigma > DTR * 180.0f)
        sigma -= 360.0f * DTR;
    else if (sigma < -180.0f)
        sigma += 360.0f * DTR;

    // Given gs, we can get alpha from the standard lift equation and Cl-alpha
    // Assume 1 G for wings level, max at 7 gs at +/- 90 degrees roll
    float desiredGs = 1.0F;

    if ( not IsSet(InAir))
    {
        //nzcgs = nzcgb = max(0.0F, (15.0F - weight/liftO_alp)*0.25F);
        if (ctlpitch > 0.0F)
            desiredGs = max(0.0F, (15.0F - weight / liftO_alp) * 0.25F);
        else
            desiredGs = 0.0F;
    }
    else
    {
        //nzcgb = 1.0F + 6.0F * fabs (mu / (MAX_AF_ROLL)) * liftO_alp/weight * 5.0F;
        if (platform->platformAngles.cosmu)
            desiredGs = 1.0F / platform->platformAngles.cosmu;
        else
            desiredGs = maxGs;

        desiredGs = min(desiredGs, maxGs);
        //nzcgs = nzcgb = min (nzcgb, maxGs);
    }

    //this needs to always be calculated, otherwise it is impossible to taxi or takeoff
    // Find new velocity
    vtDot = xwaero + xwprop - GRAVITY * platform->platformAngles.singam;

    float netAccel = CalculateVt(dT);

    // edg: my mr steen mode to allow hovering
    // speed is directly based on throtl
    // speed up yawrate
    if (playerFlightModelHack and 
        platform == SimDriver.GetPlayerEntity() and 
        platform->AutopilotType() == AircraftClass::APOff)
    {
        float oldvt;

        oldvt = vt;

        vt = throtl * 3000.0f;
        vtDot = vt - oldvt;

        // double the yawrate
        sigma = sigma - dT * r;
        r *= 2.0f;
        sigma = sigma + dT * r;

        if (sigma > DTR * 180.0f)
            sigma -= 360.0f * DTR;
        else if (sigma < -180.0f)
            sigma += 360.0f * DTR;

        // if in air we don't want alpha effects
        nzcgb = 0.0f;

        newVt = vt + vtDot * dT;

        if (newVt > 0.0F)
            vt = newVt;
        else
            vt = 0.00001F;
    }



    if (vt and not (playerFlightModelHack and 
                platform == SimDriver.GetPlayerEntity() and 
                platform->AutopilotType() == AircraftClass::APOff))
    {
        Gains();

        if ( not IsSet(InAir))
        {
            if (desiredGs)
                tmp = CalcDesAlpha(desiredGs);
            else
                tmp = 0.0F;

            tmp = min(max(tmp, 0.0F), 13.0F);
        }
        else
        {
            tmp = CalcDesAlpha(desiredGs);
            tmp = min(max(tmp, -5.0F), 13.0F);
        }

        PitchIt(tmp, dT);
    }
    else
    {
        alpha = 0.0F;
        oldp03[1] = alpha;
        oldp03[2] = alpha;
        oldp03[4] = alpha;
        oldp03[5] = alpha;
    }


    beta = 0.0f;

    // Find angles and sin/cos
    Trigenometry();

    // get delta x y and z entirely based on the direction we're pointing
    if (vt > 0.00001F)
    {
        xdot =  vt * platform->platformAngles.cosgam *
                platform->platformAngles.cossig;
        ydot =   vt * platform->platformAngles.cosgam *
                 platform->platformAngles.sinsig;
        zdot =  -vt * platform->platformAngles.singam;
        ShiAssert( not _isnan(xdot));
        ShiAssert( not _isnan(ydot));
        ShiAssert( not _isnan(zdot));
    }
    else
    {
        xdot = 0.0F;
        ydot = 0.0F;
        zdot = 0.0F;
        vt = 0.0001f;
    }

    //RunLandingGear(); // MLR 2003-10-15

    if ( not IsSet(InAir))
    {
        groundDeltaX += dT * xdot * gSpeedyGonzales;
        groundDeltaY += dT * ydot * gSpeedyGonzales;
        x = groundAnchorX + groundDeltaX;
        y = groundAnchorY + groundDeltaY;
        z = z + dT * zdot * gSpeedyGonzales;


    }
    else
    {
        x = x + dT * xdot * gSpeedyGonzales;
        y = y + dT * ydot * gSpeedyGonzales;
        z = z + dT * zdot * gSpeedyGonzales;
        groundAnchorX = x;
        groundAnchorY = y;
        groundDeltaX = 0.0f;
        groundDeltaY = 0.0f;
    }

    ShiAssert( not _isnan(x));
    ShiAssert( not _isnan(y));
    ShiAssert( not _isnan(z));

    groundZ = OTWDriver.GetGroundLevel(x, y, &gndNormal);
    // Normalize terrain normal
    tmp = (float)sqrt(gndNormal.x * gndNormal.x + gndNormal.y * gndNormal.y + gndNormal.z * gndNormal.z);
    gndNormal.x /= tmp;
    gndNormal.y /= tmp;
    gndNormal.z /= tmp;

    /*----------------------*/
    /* set flight status
    /*----------------------*/
    if ( not IsSet(InAir))
    {
        float gndGmma, relMu;

        CalculateGroundPlane(&gndGmma, &relMu);

        if (qsom * cnalpha < 0.5F)
        {
            SetFlag(Planted);
            SetGroundPosition(dT, netAccel, gndGmma, relMu);
        }
        else if (qsom * cnalpha > 0.55F)
        {
            ClearFlag(Planted);
            //Cobra test
            //if ((-zaero > GRAVITY or oldp03[2] == 13.0F) and gmma - gndGmma > 0.0F and ctlpitch > 0.0f)
            int tstatus = platform->DBrain()->ATCStatus();

            if (vcas > rotate and tstatus > tTaxi) //Cobra
            {
#ifdef DEBUG
                ShiAssert(platform->DBrain()->ATCStatus() not_eq lLanded);
#endif
                SetFlag(InAir);
                platform->UnSetFlag(ON_GROUND);
            }
            else
                SetGroundPosition(dT, netAccel, gndGmma, relMu);
        }
        else
            SetGroundPosition(dT, netAccel, gndGmma, relMu);
    }
    else
    {
        CheckGroundImpact(dT);
    }
}

