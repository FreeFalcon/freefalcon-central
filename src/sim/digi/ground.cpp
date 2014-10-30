#include "stdhdr.h"
#include "simveh.h"
#include "digi.h"
#include "otwdrive.h"
#include "simbase.h"
#include "airframe.h"
#include "Aircrft.h"
#include "Graphics/Include/tmap.h"

#include "limiters.h"

#define MIN_ALTT 1500.0F //me123 from 1500

extern float g_MaximumTheaterAltitude;
extern float g_fPullupTime;
extern bool g_bOtherGroundCheck; // = OldGroundCheck function
extern float g_fGALookAheadTime; // Cobra - How far to look ahead (times deltaX bitand deltaY) for lower elevations
extern float g_fAIMinAlt; // Cobra - minimum alt AI will fly
extern float g_fGApStickFac; // Cobra - Smooth out Ground Avoidance pitch (pStick * g_fGApStickFac)
extern int g_nCriticalPullup; // Cobra - <= g_fGALookAheadTime tick full pStick pullup

// JB 011023 Rewritten -- use complex ground checking for all AI
// JB 020313 Rewritten again
void DigitalBrain::GroundCheck(void)
{
    float turnRadius, num, alt;
    float groundAlt;
    float minAlt = 0;

    // 2001-10-21 Modified by M.N. When Leader is in WaypointMode, Wings are always in Wingymode.
    // so they do terrain based ground checks for the wingies, but not for leads - makes no sense.
    // Let it follow waypoints close to the ground
    if ( //( curMode == WaypointMode or // also perform GroundCheck for leaders
        //curMode == LandingMode or // airbases with hilly terrain around need GroundCheck
        /*(curMode == WaypointMode and agDoctrine not_eq AGD_NONE) or*/ // 2002-03-11 ADDED BY S.G. GroundAttackMode has its own ground avoidance code
        curMode == TakeoffMode //)
       and threatPtr == NULL)
    {
        // edg: do we really ever need to do ground avoidance if we're in
        // waypoint mode?  The waypoint code should be smart enough....
        // minAlt = 500.0F;
        groundAvoidNeeded = FALSE;
        ResetMaxRoll();
        SetMaxRollDelta(100.0F);
        return;
    }

    // 2002-03-11 ADDED BY S.G. If in WaypointMode or WingyMode, drop the min altitude before pullup to 500 feet AGL or -trackZ, whichever is smaller but never below 100.0f AGL
    if (curMode == WaypointMode or curMode == WingyMode)
        minAlt = (trackZ > -g_fAIMinAlt ? (trackZ > -100.0f ? 100.0f : -trackZ) : g_fAIMinAlt); // Cobra - externalized AI min alt
    else
        // END OF ADDED SECTION 2002-03-11
        minAlt = MIN_ALTT;

    // Allow full roll freedom
    ResetMaxRoll();
    SetMaxRollDelta(100.0F);

    /*----------------------------------------------------------------*/
    /* If gamma is positive ground avoidance is not needed regardless */
    /* of altitude.                                                   */
    /*----------------------------------------------------------------*/
    if (self->ZPos() < g_MaximumTheaterAltitude) // changed for theater compatibility
    {
        groundAvoidNeeded = FALSE;
        return;
    }

    // Calculate the time it takes to level the wings.
    float maxcurrentrollrate = af->GetMaxCurrentRollRate();
    float timetorolllevel = 1.0F;

    if (maxcurrentrollrate > 1.0F)
    {
        timetorolllevel = fabs(self->Roll() * RTD) / maxcurrentrollrate;
    }

    // Sanity check
    timetorolllevel = min(timetorolllevel, 60.0F);

    /*--------------------------------*/
    /* Find current state turn radius */
    /*--------------------------------*/
    gaGs = max(af->SustainedGs(TRUE), 2.0F);
    gaGs = min(maxGs, gaGs);

    // turn Radius, corrected for current gamma (i.e. max at -90.0, min at 0.0
    turnRadius = self->GetVt() * self->GetVt() / (GRAVITY * (gaGs - 1.75F));

    // turn Radius needs to take into account the time it takes to roll level so we can start pulling.
    turnRadius += timetorolllevel * self->GetVt();

    float lookahead = max(2.0F, turnRadius / (self->GetVt() + 1));

    if (lookahead < 0)
        lookahead = -lookahead;

    // Sanity check
    lookahead = min(lookahead, 120.0F);

    // Cobra - use external look ahead variable
    lookahead = max(g_fGALookAheadTime, lookahead);

    bool groundavoid = false;
    float pitchadjust;
    // At what angle are we headed to the ground.  Take that into account so we can try to project a recovery path.
    float pitchturnfactor = max(0.0f, sin(-self->Pitch()) * turnRadius);
    // If we need to guess at a recovery path, adjust for different delta leg sizes.
    float directionalfactor = fabs(self->XDelta() / self->YDelta());
    float xsign = (self->XDelta() > 0.0) ? 1.0f : -1.0f;
    float ysign = (self->YDelta() > 0.0) ? 1.0f : -1.0f;
    PullupNow = 0;

    // Cobra - Reworked to make AI mountain climbing smoother and more realistic (no jerky pitch)
    // =========================================================================================
    // Are we even close to hitting the ground?
    // Cobra - GetMEA() returns really gross elevations (8 km ground resolution)
    //   groundAlt = TheMap.GetMEA( self->XPos() + self->XDelta(), self->YPos() + self->YDelta());
    maxElevation = groundAlt = -OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(), self->YPos() + self->YDelta());

    if (-self->ZPos() < (groundAlt + max(turnRadius * 2, minAlt)))
    {
        groundAlt = -OTWDriver.GetGroundLevel(self->XPos(), self->YPos());

        // Test the terrain along the likely path we will take.  Take into account the deltas along the ground
        // and the fact that we may be headed straight down so the deltas could be very small.  Since we still
        // need to test the terrain in case we're diving into a valley, test the terrain along the recovery path.
        //  for (float i = 0.0; i < lookahead and not groundavoid; i += 0.5)
        for (float i = 0.0; i < lookahead; i += 0.5)
        {
            // Project a recovery path based on our pitch.
            pitchadjust = i / lookahead * pitchturnfactor;

            float dx = i * self->XDelta() + xsign * pitchadjust * directionalfactor;
            float dy = i * self->YDelta() + ysign * pitchadjust / directionalfactor;

            groundAlt = max(groundAlt, -OTWDriver.GetGroundLevel(self->XPos() + dx, self->YPos() + dy));

            // 2002-04-17 MN Start ground avoid check only if ground distances is below a threshold
            //if (-self->ZPos() - groundAlt < self->af->GetStartGroundAvoidCheck())
            // Cobra - default 3000' AGL is important for a/c in dogfight
            if (-self->ZPos() - groundAlt < 3000.0f)
            {
                if (groundAlt > maxElevation)
                    maxElevation = groundAlt;

                // Check to be sure we're not going to hit the ground in the next time frame and double check that our worst case
                // recovery path along the ground to recover from high pitch angles won't take us into the ground either.
                if (-self->ZPos() - (i * self->ZDelta()) < maxElevation + 100.0F or
                    -self->ZPos() - pitchturnfactor * 1.25 < maxElevation + 100.0F)
                {
                    //float z = self->ZPos();
                    //float dz = self->ZDelta();
                    //float dist = max(sqrtf(dx*dx + dy*dy),0.1f);
                    //float height = maxElevation + self->ZPos()+100.f;
                    //float slope = atan2(height, (g_fPullupTime * self->GetVt()));
                    //groundAvoidPStick = slope * RTD / 90.0f;
                    //float Time2Pull = dist / (self->GetVt()+1);
                    if ( not PullupNow)
                    {
                        PullupNow = (int)i;
                    }

                    groundavoid = true;
                }
            }
        }
    }

    // Cobra - end of reconstruction

    if (self->GetVt() < 0.1f or not groundavoid)
    {
        groundAvoidNeeded = FALSE;
        ResetMaxRoll();
        SetMaxRollDelta(100.0F);
        pullupTimer = 0;
    }
    else
    {
        // Keep the turn radius above the hard deck
        alt = (-self->ZPos()) - maxElevation - minAlt; // Cobra - using max elevation in path
        // Cobra - divide by zero check
        turnRadius = min(1.0f, turnRadius);
        num = alt / turnRadius;

        /*------------------------------------*/
        /* find roll limit and pitch altitude */
        /*------------------------------------*/
        if (alt > 0.0F)
            gaRoll = 120.0F - (float)atan2(sqrt(1 - num * num), num) * RTD;
        else
            gaRoll = 45.0F;

        if (gaRoll < 0.0f)
            gaRoll = 0.0f;

        groundAvoidNeeded = TRUE;
    }

    // 2002-02-24 added by MN
    if (groundAvoidNeeded)
    {
        pullupTimer = SimLibElapsedTime + ((unsigned long)(g_fPullupTime * CampaignSeconds)); // configureable for how long we pull at least once entered groundAvoidNeeded mode
    }
}

void DigitalBrain::PullUp(void)
{
    float fact;
    float lastPstick = pStick;
    float gaMoo = self->Roll() * RTD;
    float cushAlt;
    float myAlt;

    /*-----------*/
    /* MACH HOLD */
    /*-----------*/
    // We really want the smallest turn radius speed.  Try corner - 100.
    MachHold(cornerSpeed - 100, self->GetKias(), TRUE);

    /*-----------------------------------------*/
    /* negative gamma decreases the roll limit */
    /*-----------------------------------------*/
    fact = 1.0F - self->platformAngles.singam;
    fact = 1.0F; // ??

    /*---------------------------------------------------------*/
    /* Get roll error to the roll limit. Set RSTICK and PSTICK */
    /*---------------------------------------------------------*/
    if (gaMoo > gaRoll / fact)
    {
        SetMaxRollDelta(gaRoll / fact - gaMoo);
    }
    else if (gaMoo < -gaRoll / fact)
    {
        SetMaxRollDelta(-gaRoll / fact - gaMoo);
    }
    else
    {
        if (rStick > 0.0F)
            SetMaxRollDelta(100.0F);
        else
            SetMaxRollDelta(-100.0F);
    }

    // Cobra - using max elevation in path
    //   cushAlt = -OTWDriver.GetGroundLevel(self->XPos(), self->YPos()) + MIN_ALTT * 0.75;
    cushAlt = maxElevation + MIN_ALTT * 0.75f;
    myAlt = -self->ZPos();

    if (myAlt > cushAlt)
        SetMaxRoll(min(af->MaxRoll() * 0.99F, gaRoll / fact));
    else
        SetMaxRoll(0.0f);

    // The ground avoidance calculations tell us when we need to pull up NOW.  So pull back NOW.

    // 2002-04-17 MN make it aircraft dependent which method of PStick change we want to use
    // ( a C-130 doing full P-stick pulls looks ridiculous...)

    if (self->af->LimitPstick())
    {
        if (PullupNow <= g_nCriticalPullup)
            pStick = 1.0f;
        else if (myAlt > cushAlt)
        {
            // Pull the gs to avoid the ground
            SetPstick(max(gaGs, 6.0F), af->MaxGs(), AirframeClass::GCommand);
            pStick = max(lastPstick, pStick);
        }
        else
        {
            // cushAlt -= MIN_ALTT * 0.5f;
            // pStick = 1.0f - ( myAlt - cushAlt )/(MIN_ALTT * 0.5f);

            cushAlt = maxElevation + g_fAIMinAlt;

            if (myAlt <= cushAlt)
            {
                pStick = 1.0f - (cushAlt - myAlt) / (cushAlt);
                pStick *= g_fGApStickFac;

                if (pStick > 1.0f)
                    pStick = 1.0f;

                if (pStick < 0.0f)
                    pStick = 0.0f;
            }
            else
                pStick = 0.0f;
        }
    }
    else
        pStick = 1.0f;

    // if (pullupTimer < SimLibElapsedTime)
    // pullupTimer = 0; // This says us "stop pull up"
    // Cobra -
    if ((groundAvoidNeeded) and (pullupTimer <= SimLibElapsedTime))
        pullupTimer = SimLibElapsedTime + ((unsigned long)(g_fPullupTime * CampaignSeconds)); // configureable for how long we pull at least once entered groundAvoidNeeded mode
    else
        pullupTimer = 0; // This says us "stop pull up"
}
