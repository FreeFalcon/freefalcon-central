#include "stdhdr.h"
#include "simmath.h"
#include "digi.h"
#include "simveh.h"
#include "airframe.h"
#include "campwp.h"
#include "object.h"
#include "PilotInputs.h"
#include "aircrft.h"
#include "objectiv.h"
#include "simdrive.h"
#include "Graphics/Include/tmap.h"
#define MANEUVER_DEBUG
#ifdef MANEUVER_DEBUG
#include "Graphics/include/drawbsp.h"
extern int g_nShowDebugLabels;
#endif

extern float g_fFormationBurnerDistance;

extern float g_fFuelBaseProp; // 2002-03-14 S.G. For better fuel consumption tweaking
extern float g_fFuelMultProp; // 2002-03-14 S.G. For better fuel consumption tweaking
extern float g_fFuelTimeStep; // 2002-03-14 S.G. For better fuel consumption tweaking
extern bool g_bFuelUseVtDot; // 2002-03-14 S.G. For better fuel consumption tweaking
extern float g_fFuelVtClip; // 2002-03-14 S.G. For better fuel consumption tweaking
extern float g_fFuelVtDotMult; // 2002-03-14 S.G. For better fuel consumption tweaking
extern bool g_bFuelLimitBecauseVtDot; // 2002-03-14 S.G. For better fuel consumption tweaking
extern float g_fWaypointBurnerDelta; // 2002-03-27 MN BurnerDelta in WaypointMode bitand WingyMode
extern float g_fePropFactor; // 2002-04-05 MN ePr

int HoldCorner(int combatClass, SimObjectType* targetPtr);

float DigitalBrain::TrackPointLanding(float speed)
{
    float xft, yft, zft, rx, ry, rz, elErr;
    float eProp, minSpeed;

    CalculateRelativePos(&xft, &yft, &zft, &rx, &ry, &rz);

    rStick = SimpleTrackAzimuth(rx, ry, self->GetVt());

    //clamp yaw rate to 10 deg/s max
    //simple model uses a max yaw rate of 20 deg/s
    //TJL 02/21/04 Increase rstick to get proper bank angles for turn rate (was 0.15F)
    rStick = max(min(rStick, 0.6F), -0.6F);

    elErr = SimpleTrackElevation(trackZ - self->ZPos(), (float)sqrt(xft * xft + yft * yft));
    // keep stick at reasonable values.
    pStick = min(0.2f, max(elErr, -0.3F));

    eProp = speed - af->vt;

    if (af->z - af->groundZ < -200.0F)
    {
        minSpeed = af->CalcDesSpeed(10.0F);

        if (speed < minSpeed)
        {
            eProp = minSpeed - af->vt;
            speed = minSpeed;
        }
    }

    //if we're going to stall out, hit the gas a bit
    if (af->Qsom()*af->Cnalpha() < 1.55F and eProp < 20.0F)
    {
        eProp = 20.0F;
        speed = af->vt + 20.0F;
    }

    if (eProp >= 150.0F)
    {
        autoThrottle = 1.5F;
        throtl = 1.5F;                        /* burner     */
        af->speedBrake = -1.0F;
    }
    else if (eProp < -100.0F)
    {
        autoThrottle = 0.0F;
        throtl = 0.0F;                        /* idle and boards */
        af->speedBrake = 1.0F;
    }
    else
    {
        //if(atcstatus == lOnFinal or (throtl == 0.0F and af->vtDot > -5.0F and eProp < -10.0F))
        //TJL 02/20/04 Not just boards on final because of drag penalty.
        if (throtl == 0.0F and af->vtDot > -5.0F and eProp < -10.0F)
            //deploy speed brakes on final
            af->speedBrake = 1.0F;
        else
            af->speedBrake = -1.0F;

        autoThrottle += eProp * 0.01F * SimLibMajorFrameTime;
        autoThrottle = max(0.0F, min(1.5F, autoThrottle));
        throtl = eProp * 0.02F + autoThrottle - af->vtDot * SimLibMajorFrameTime * 0.005F;
    }

    af->SetFlaps(true);
    //MonoPrint("Eprop:  %6.3f  autoTh: %6.3f  vtDot: %6.3f  throtl: %6.3f\n", eProp, autoThrottle, af->vtDot, throtl);
    throtl = min(max(throtl, 0.0F), 1.5F);

    return speed;
}

float DigitalBrain::TrackPoint(float maxGs, float speed)
{
    float retval = 0.0F;

    //TJL 08/28/04 Other things besides ATC/LANDME use this.  A/A and A/G maneuvers too
    /*if ( not self->OnGround())
        af->SetFlaps(curMode == LandingMode);*/
    if (self->af->GetSimpleMode())
    {
        // do simple flight model
        SimpleTrack(SimpleTrackSpd, speed); // speed = desired speed (ft/sec)
    }
    else
    {
        retval = AutoTrack(maxGs);
        //TJL 02/21/04 Removed * FTPSEC. Speed is passed in knots
        MachHold(speed, self->GetKias(), TRUE);
    }

    return retval;
}

float DigitalBrain::VectorTrack(float maxMnvrGs, int fineTrack)
{
#if 0
    double xft, yft, zft, rx, ry, rz, range, xyRange;
    double azerr, elerr, ata, droll;

    /*-----------------------------*/
    /* calculate relative position */
    /*-----------------------------*/

    xft = trackX - af->x;
    yft = trackY - af->y;
    zft = trackZ - af->z;
    rx = self->vmat[0][0] * xft + self->vmat[0][1] * yft + self->vmat[0][2] * zft;
    ry = self->vmat[1][0] * xft + self->vmat[1][1] * yft + self->vmat[1][2] * zft;
    rz = self->vmat[2][0] * xft + self->vmat[2][1] * yft + self->vmat[2][2] * zft;


    /** MBR: If this code is turned back 'ON', it **/
    /** should be modified....                    **/

    range = sqrt(rx * rx + ry * ry + rz * rz);

    /*--------------*/
    /* Sanity Check */
    /*--------------*/
    if (range < 0.1F)
        return (0.0F);

    rx = max(min(rx, range), -range);
    ry = max(min(ry, range), -range);
    rz = max(min(rz, range), -range);

    /*-------------------*/
    /* relative geometry */
    /*-------------------*/
    if (rx not_eq 0.0F)
        ata      = (float)acos(rx / range) * RTD;
    else
        ata = 0.0F;

    droll    = atan2(ry, -rz);
    xyRange = sqrt(rx * rx + ry * ry);
    azerr    = atan2(ry, rx) * RTD;
    elerr    = atan(-rz / xyRange) * RTD;

    /*---------------*/
    /* roll and pull */
    /*---------------*/
    if (trackMode == 1)
    {
        /* alternative method for setting pstick allows for unloaded rolls...  */
        SetPstick(elerr * 0.75F, maxMnvrGs, AirframeClass::ErrorCommand);

        /* set pstick this way for better return from a out of plane maneuver
        SetPstick( (float)ata, maxGs, AirframeClass::ErrorCommand);
        */

        SetRstick(droll * RTD);
        SetYpedal(0.0F);
        SetMaxRollDelta(droll * RTD);

        if (ata < 5.0) trackMode = 2;
    }
    /*----------------------------*/
    /* pitch and yaw, wings level */
    /*----------------------------*/
    else
    {
        SetPstick((float)elerr, maxMnvrGs, AirframeClass::ErrorCommand);
        SetYpedal((float)azerr / 3.0F);

        if (ata > 8.0F) trackMode = 1;
    }

    /*-------------------------*/
    /* return nose angle error */
    /*-------------------------*/
    return ((float)ata);
#else
    return 0.0F;
#endif
}

float DigitalBrain::AutoTrack(float maxMnvrGs)
{
    float xft, yft, zft, rx, ry, rz;
    float elerr, ata, droll, azerr;

    /*-----------------------------*/
    /* calculate relative position */
    /*-----------------------------*/

    xft = trackX - self->XPos();
    yft = trackY - self->YPos();
    zft = trackZ - self->ZPos();
    rx = self->dmx[0][0] * xft + self->dmx[0][1] * yft + self->dmx[0][2] * zft;
    ry = self->dmx[1][0] * xft + self->dmx[1][1] * yft + self->dmx[1][2] * zft;
    rz = self->dmx[2][0] * xft + self->dmx[2][1] * yft + self->dmx[2][2] * zft;

    ata   = (float)atan2(sqrt(ry * ry + rz * rz), rx) * RTD;


    /*---------------*/
    /* roll and pull */
    /*---------------*/
    droll = (float)atan2(ry, -rz);
    elerr = (float)atan2(-rz, sqrt(rx * rx + ry * ry)) * RTD;
    azerr    = (float)atan2(ry, rx) * RTD;

    /* set pstick this way for better return from a out of plane maneuver
    SetPstick( (float)ata, maxMnvrGs, AirframeClass::ErrorCommand);
    */

    if (ata < 5.0F)
    {
        SetPstick(1.5F * elerr, maxMnvrGs, AirframeClass::ErrorCommand);
        SetYpedal(azerr / 4.0F);
        SetRstick(-self->Roll() * 5.0F);
    }
    else if (ata < 10.0f and curMode >= BVREngageMode)
    {
        // edg note: what this does is to roll in the opposite direction
        // and do a neg G push rather than roll all the way around and pull
        // if our roll error is large ( in this case beyond 150deg ).   This
        // will keep it from flip-flopping around as seen previously, plus
        // I think it's the way a pilot would likely do things.
        if (droll > 150.0f * DTR and rStick < 0.5F)
        {
            SetRstick(droll * RTD - 180.0f);
            SetPstick(-ata, maxMnvrGs, AirframeClass::ErrorCommand);
        }
        else if (droll < -150.0f * DTR and rStick > -0.5F)
        {
            SetRstick(droll * RTD + 180.0f);
            SetPstick(-ata, maxMnvrGs, AirframeClass::ErrorCommand);
        }
        else
        {
            SetRstick(droll * RTD);
            SetPstick(ata * RTD, maxMnvrGs, AirframeClass::ErrorCommand);
        }

        SetYpedal(0.0F);
    }
    else
    {
        // If we're stupid
        if (SkillLevel() < 2 and fabs(ata) > 90.0F and fabs(self->Pitch()) < 45.0F * DTR)
        {
            elerr = maxMnvrGs * 0.85F;
            droll = (float)acos(1.0F / elerr);
            SetMaxRoll(droll * RTD);
            droll -= self->Roll();
            SetRstick(droll * RTD);
            SetPstick(maxMnvrGs, maxMnvrGs, AirframeClass::GCommand);
        }
        else
        {
            SetRstick(droll * RTD);
            SetPstick(ata * min((30.0F * DTR) / (float)fabs(droll), 1.0F), maxMnvrGs, AirframeClass::AlphaCommand);  //me123 from errorcommand
            SetYpedal(0.0F);
            SetMaxRoll((float)fabs(self->Roll() + droll) * RTD);
            SetMaxRollDelta(droll * RTD);
        }
    }

    /*-------------------------*/
    /* return nose angle error */
    /*-------------------------*/
    return (ata);
}

float DigitalBrain::SetPstick(float pitchError , float gLimit, int commandType)
{
    float stickFact = 0.0F, stickCmd = 0.0F;

    af->ClearFlag(AirframeClass::GCommand bitor AirframeClass::AlphaCommand |
                  AirframeClass::AutoCommand bitor AirframeClass::ErrorCommand);

    if (commandType == AirframeClass::ErrorCommand)
    {
        if (pitchError > 30.0F)
            stickCmd = gLimit;
        else if (pitchError > 0.0F)
            stickCmd = gLimit * pitchError / 30.0F + 1.0F;
        else if (pitchError > -30.0F)
            stickCmd = gLimit * 0.5F * pitchError / 30.0F;
        else
            stickCmd = -gLimit * 0.5F;

        af->SetFlag(AirframeClass::GCommand);
    }
    else if (commandType == AirframeClass::GCommand)
    {
        /*
        if (pitchError <= 1.0F)
          stickCmd = -(float)sqrt((1.0F - pitchError) / (4.0F + self->platformAngles.costhe));
        else
          stickCmd = (float)sqrt((pitchError - 1.0F) / (gLimit - self->platformAngles.costhe));
         af->SetFlag(AirframeClass::GCommand);
         */
        stickCmd = max(min(pitchError, gLimit), -gLimit);
        af->SetFlag(AirframeClass::GCommand);
    }
    else if (commandType == AirframeClass::AlphaCommand)
    {
        stickCmd = pitchError * 0.75F;
        af->SetFlag(AirframeClass::AlphaCommand);
    }
    else
    {
        MonoPrint("digi.w: : BAD COMMAND MODE IN stickCmd\n");
        stickCmd = 0.0F;
    }

    if (stickCmd <= 1.0F)
    {
        stickCmd = -(float)sqrt((1.0F - stickCmd) / (4.0F + self->platformAngles.costhe));
    }
    else
    {
        stickCmd = (float)sqrt((stickCmd - 1.0F) / (af->MaxGs() - self->platformAngles.costhe));
    }

    // Limit stick for low airspeeds

    stickFact = min(150.0F, self->GetKias() - 150.0F);
    stickFact = 0.5F + stickFact / 300.0F;
    stickFact = max(0.0F, stickFact);
    stickCmd *= stickFact;

    // Smooth the command
    pStick = 0.2F * pStick + 0.8F * stickCmd;
    return pitchError;
}

float DigitalBrain::SetRstick(float rollError)
{
    float maxRoll = af->MaxRoll();
    float stickCmd;

    //TJL 09/26/04 Roll is in radians but maxRoll in degrees //Cobra 10/31/04 TJL
    //if (fabs(self->Roll()) > maxRoll)
    if (fabs(self->Roll()*RTD) > maxRoll)
    {
        if (self->Roll() > 0.0F)
        {
            //rollError = min (rollError, (maxRoll - self->Roll()) * RTD);
            rollError = min(rollError, (maxRoll - (self->Roll() * RTD)));
        }
        else
        {
            //rollError = max (rollError, (maxRoll + self->Roll()) * RTD);
            rollError = max(rollError, (maxRoll + (self->Roll() * RTD)));
        }
    }

    stickCmd = rollError * DTR * 0.75F / max((af->kr01 * af->tr01), 0.1F);

    rStick = 0.2F * rStick + 0.8F * stickCmd;

    return rollError;
}

float DigitalBrain::SetYpedal(float yawError)
{
    yPedal = 0.2F * yPedal - 0.8F * yawError * RTD * 0.0125F;

    return yawError;
}

void DigitalBrain::SetMaxRoll(float maxRoll)
{
    af->SetMaxRoll(maxRoll);
}

void DigitalBrain::SetMaxRollDelta(float maxRoll)
{
    af->SetMaxRollDelta(maxRoll);
}

void DigitalBrain::ResetMaxRoll(void)
{
    af->ReSetMaxRoll();
}

int DigitalBrain::MachHold(float m1, float m2, int pitchStick)
{
    float eProp = 0.0F, thr = 0.0F;
    float maxDelta = 0.0F, cornerDelta = 0.0F, burnerDelta = 0.0F;
    float dx = 0, dy = 0, dist = 0;

    eProp  = m1 - m2;

    /*-----------------*/
    /* knots indicated */
    /*-----------------*/

    // 2001-10-28 ADDED "* FTPSEC_TO_KNOTS" to MinVCas by M.N. this function needs knots 2002-03-14 REMOVED BY S.G. MinVcas is ALREADY in KNOTS and NOT in FTPSEC
    // Added it back in, Hehe... // JPG 2 Jan 04
    //TJL 02/20/04 Took it back out ;)
    if (m1 < (af->MinVcas()) and af->IsSet(AirframeClass::InAir))
    {
        //Cobra 10/31/04 TJL
        m1 = af->MinVcas(); // * TJL 08/28/04 FTPSEC_TO_KNOTS; m1 is in KNOTS and MinVcas is in KNOTS
        eProp = m1 - m2;
    }

    //me123 addet the max check. we don't wanna exceed the airframe max speed
    if (m1 > af->curMaxStoreSpeed - 20.0f and af->IsSet(AirframeClass::InAir))
    {
        m1 = af->curMaxStoreSpeed - 20.0f;
        eProp = m1 - m2 - g_fePropFactor;
    }

    af->SetFlaps(curMode == LandingMode);

    if (eProp < -100.0F)
    {
        thr = 0.0F;                        /* idle and boards */

        if (curMode > DefensiveModes and curMode < LoiterMode)
            af->speedBrake = 1.0F;
        else
            af->speedBrake = -1.0F;
    }
    else if (eProp < -50.0F)
    {
        thr = 0.0F;                        /* idle  */
        af->speedBrake = -1.0F;
    }
    else
    {
        // If in combat take vtDot into account
        // For waypoint stuff you need to be really slow to hit burner
        if (curMode < LoiterMode and curMode not_eq LandingMode) // 2002-02-12 MODIFIED BY S.G. And not in landing mode
        {
            eProp -= min(2.0F * af->VtDot(), 0.0F);
            burnerDelta = 100.0F;
        }
        else
        {
            burnerDelta = 500.0F;

            // 2002-03-26 MN make it harder to go into afterburner when in waypoint- or wingymode
            if (curMode == WingyMode or curMode == WaypointMode)
                burnerDelta = g_fWaypointBurnerDelta;
        }

        if (eProp >= burnerDelta)
        {
            thr = 1.5F;                        /* burner     */
            af->speedBrake = -1.0F;
        }
        else
        {
            // 2002-03-14 MODIFIED BY S.G. Lets fine tune this throttle thing
            //       thr = (eProp + 100.0F) * 0.008F;     /* linear in-between */
            //       autoThrottle += eProp * 0.001F * SimLibMajorFrameTime;
            //       autoThrottle += eProp * timeStep * SimLibMajorFrameTime;
            // Here we take tVtDot in consideration but clip it at +-5 so it doesn't affect too much
            float usedVtDot = af->VtDot();

            if (usedVtDot > g_fFuelVtClip)
                usedVtDot = g_fFuelVtClip;
            else if (usedVtDot < -g_fFuelVtClip)
                usedVtDot = -g_fFuelVtClip;

            thr = (eProp + g_fFuelBaseProp) * g_fFuelMultProp; /* linear in-between */
            autoThrottle += (eProp - usedVtDot * g_fFuelVtDotMult) * g_fFuelTimeStep * SimLibMajorFrameTime;

            // Now see if we're asking to increase/cut the throtle because of speed difference too much (don't go the other direction)
            if (g_bFuelLimitBecauseVtDot)
            {
                if (eProp > 0.0f and autoThrottle < 0.0f)
                    autoThrottle = 0.0f;
                else if (eProp < 0.0f and autoThrottle > 0.0f)
                    autoThrottle = 0.0f;
            }

            // END OF MODFIED SECTION 2002-03-14

            autoThrottle = max(min(autoThrottle, 1.5F), -1.5F);
            thr += autoThrottle;

            if (flightLead)
            {
                dx = ((AircraftClass*)flightLead)->af->x - af->x;
                dy = ((AircraftClass*)flightLead)->af->y - af->y;
                dist = (float)sqrt(dx * dx + dy * dy);
            }

            // no burner unless in combat
            if ((curMode >= LoiterMode or curMode == LandingMode) and // 2002-02-12 MODIFIED BY S.G. Don't go in AB if you're in landing mode either
                m2 > aeroDataset[self->af->VehicleIndex()].inputData[AeroDataSet::MinVcas] * 0.9f and // JB 011018 If we can't keep our speed up, use the buner 2002-02-12 MODIFIED BY S.G. Use a percentage of MinVcas instead.
                ( not flightLead or flightLead and ((AircraftClass*)flightLead)->af) and 
                ( not flightLead or (((AircraftClass*)flightLead)->af == af or ((((AircraftClass*)flightLead)->af->rpm < 1.0F)) and // JB 011025 If the lead is using his burner, we can use ours 2002-02-12 MODIFIED BY S.G. Don't look at lead's burner or g_fFormationBurnerDistance if we're RTBing...
                                 dist < g_fFormationBurnerDistance * NM_TO_FT) or curMode == RTBMode) or // allow usage of burner if lead is more than defined distance away
                self->OnGround()) // never use AB on ground
            {
                // Flight lead goes even slower so wingies can catch up
                /* 2002-02-12 MODIFIED BY S.G. Take the wings 'mInPositionFlag' flag before limiting ourself
                            if ( not isWing)
                               thr = min (thr, 0.9F);
                            else
                               thr = min (thr, 0.975F); */
                if ( not isWing)
                {
                    // The lead will look at everybody else's position and push faster if everyone is in position.
                    int size = self->GetCampaignObject()->NumberOfComponents();
                    int i;

                    for (i = 1; i < size; i++)
                    {
                        AircraftClass *flightMember = (AircraftClass *)self->GetCampaignObject()->GetComponentEntity(i);

                        // This code is assuming the lead and the AI are on the same PC... Should be no problem unless another player is in Combat AP...
                        if (flightMember and flightMember->DBrain() and not flightMember->DBrain()->mInPositionFlag)
                            break;
                    }

                    if (i == size)
                        thr = min(thr, 0.99F);
                    else
                        thr = min(thr, 0.9F);
                }
                else
                {
                    // While wingmen unlike look after themself...
                    // 2002-04-07 MN limit wingmen only not to use afterburner, but they may catch up with full MIL power
                    // if (mInPositionFlag)
                    thr = min(thr, 0.99F);
                    // else
                    // thr = min (thr, 0.975F);
                }

            }

            af->speedBrake = -1.0F;
        }

        // Scale pStick if way off
        if (targetPtr and HoldCorner(self->CombatClass(), targetPtr))
        {
            cornerDelta = cornerSpeed - m2;
            cornerDelta -= 2.0F * af->VtDot();

            switch (SkillLevel())
            {
                case 0:  // Recruit, pull till you drop
                    maxDelta = 1000.0F;
                    break;

                case 1:  // Rookie, pull almost till you drop
                    maxDelta = 250.0F;
                    break;

                case 2:  // Average, pull too far
                    maxDelta = 200.0F;
                    break;

                case 3:  // Good, pull a little to far
                    maxDelta = 100.0F;
                    break;

                case 4:  // Ace, back off and hold desired speed
                default:
                    maxDelta = 0.0F;
                    break;
            }

            if (pStick > 1.0f) pStick = 1.0f;//me123

            if ( not af->IsSet(AirframeClass::IsDigital)) maxDelta = 0.0F;

            if (pitchStick and cornerDelta > maxDelta and pStick > 0.0F)// and fabs(self->Roll()) < 110.0F * DTR)
            {
                pStick *= max(0.55F, (1.0F - (cornerDelta - maxDelta) / (cornerSpeed * 0.75F))); //me123 0.55 from 0.25
            }
        }
    }

    /*-----------------------------*/
    /* add pitch stick interaction */
    /*-----------------------------*/
    if (pitchStick)
        throtl = thr + ((float)fabs(pStick) / 15.0F);
    else
        throtl = thr;

    //me123 status test. IRCM STUFF.

    if (curMode == MissileEngageMode or curMode == GunsEngageMode or curMode == WVREngageMode)
    {
        //me123 status test. we are inside 6nm, somebody is pointing at us and we are head on.

        if (
            targetData and not F4IsBadReadPtr(targetData, sizeof(SimObjectLocalData)) and // JB 010318 CTD
            targetData->ataFrom < 40.0f * DTR and 
            targetData->ata < 40.0f * DTR and 
            targetData->range < 10.0f * NM_TO_FT and 
            targetData->range > 1.0 * NM_TO_FT)
        {
            if (SkillLevel() >= 1 and targetData->range > 8.0 * NM_TO_FT)
            {
                throtl = min(0.99f, throtl); // let's cancel burner inside 10nm
            }
            else if (SkillLevel() >= 3 and targetData->range > 5.0 * NM_TO_FT)
            {
                throtl = min(0.40f, throtl); // let's go midrange inside 8
            }
            else if (SkillLevel() >= 2)
                throtl = 0.0f;// let's go idle between 4 and 1nm
        }
    }

    /*-------------------*/
    /* limit 0.0 ... 1.5 */
    /*-------------------*/
    throtl = min(max(throtl, 0.0F), 1.5F);
#ifdef MANEUVER_DEBUG

    if (g_nShowDebugLabels bitand 0x10000)
    {
        char tmpchr[128];
        sprintf(tmpchr, "%.0f %.0f %.0f %.3f %.2f %.2f", m1, m2, eProp, thr, autoThrottle, throtl);

        if (self->drawPointer)
            ((DrawableBSP*)self->drawPointer)->SetLabel(tmpchr, ((DrawableBSP*)self->drawPointer)->LabelColor());
    }

#endif

    if (fabs(eProp) < 0.1F * m1)
        return (TRUE);
    else
        return (FALSE);
}

void DigitalBrain::Loiter(void)
{
    //we don't want to loiter just above the ground
    // trackZ = min(max(-20000.0F, trackZ), -5000.0F);

    // Cobra - Use local max elevation to try and keep AI from lawndarting
    trackZ = -TheMap.GetMEA(((AircraftClass*) self)->XPos(), ((AircraftClass*) self)->YPos()) - 2000.0F;

    if (((AircraftClass*) self)->af->GetSimpleMode())   // do simple flight model
    {

        throtl = SimpleScaleThrottle(af->MinVcas() * KNOTS_TO_FTPSEC);
        //pStick = 0.0F; // level
        pStick = SimpleTrackElevation(trackZ - self->ZPos(), 10000.0F);
        pStick = min(0.2f, max(pStick, -0.3F));
        rStick = 0.15F; // 13.5 degree bank turn
    }
    else
    {
        /*-----------*/
        /* MACH HOLD */
        /*-----------*/
        if (curMode not_eq lastMode)
        {
            onStation = Stabalizing;
            holdAlt = -self->ZPos();
        }


        if (onStation == Stabalizing)
        {
            if (MachHold(cornerSpeed, self->GetKias(), TRUE))
            {
                onStation = OnStation;
                LevelTurn(2.0F, 1.0F, TRUE);
            }

            AltitudeHold(holdAlt);
        }
        else
        {
            LevelTurn(2.0F, 1.0F, FALSE);
        }
    }
}

void DigitalBrain::LevelTurn(float load_factor, float turnDir, int newTurn)
{
    float  edroll, elerr, alterr;

    /*-------------------------------------------*/
    /* if your not flying level, level out first */
    /*-------------------------------------------*/
    if (newTurn)
    {
        gammaHoldIError = 0.0F;
        trackMode = 0;
    }

    if (trackMode not_eq 0)
    {
        edroll = (float)atan(sqrt(load_factor * load_factor - 1));
        ResetMaxRoll();
        SetMaxRollDelta(edroll * RTD);
        edroll *= turnDir - af->mu;

        SetRstick(edroll * RTD * 2.50F);

        if (fabs(edroll) < 5.0 * DTR or trackMode == 2)
        {
            alterr = (holdAlt + self->ZPos() - self->ZDelta()) * 0.015F;
            GammaHold(alterr);
            trackMode = 2;
        }
        else
            SetPstick(0.0F, 5.0F, AirframeClass::GCommand);
    }
    else
    {
        SetRstick(-self->Roll() * RTD);
        SetMaxRoll(0.0F);
        SetMaxRollDelta(5.0F * RTD);
        elerr = -af->gmma;
        SetPstick(elerr * RTD, 2.5F, AirframeClass::ErrorCommand);

        if (fabs(af->gmma) < 2.0 * DTR and fabs(self->Roll()) < 10.0 * DTR)
            trackMode = 1;
    }

    SetYpedal(0.0F);
}

int DigitalBrain::AltitudeHold(float desAlt)
{
    float alterr;
    int retval;

    SetYpedal(0.0F);
    SetRstick(-self->Roll() * 2.0F * RTD);
    SetMaxRoll(0.0F);


    alterr = desAlt + self->ZPos();

    if (fabs(alterr) < 25.0F)
    {
        retval = TRUE;
    }
    else
    {
        retval = FALSE;
    }

    alterr -= self->ZDelta();
    GammaHold(alterr * 0.015F);

    return (retval);
}

int DigitalBrain::HeadingAndAltitudeHold(float desPsi, float desAlt)
{
    float altErr, psiErr;
    int retval = FALSE;
    int newTurn;
    float turnDir;

    psiErr = desPsi - self->Yaw();

    if (psiErr > 180.0F * DTR)
        psiErr -= 360.0F * DTR;
    else if (psiErr < -180.0F * DTR)
        psiErr += 360.0F * DTR;

    SetYpedal(0.0F);

    if (fabs(psiErr) < 5.0F * DTR)
    {
        SetRstick(-self->Roll() * 2.0F * RTD);
        SetMaxRoll(0.0F);
        SetMaxRollDelta(-self->Roll() * 2.0F * RTD);


        altErr = desAlt + self->ZPos();

        if (fabs(altErr) < 25.0F)
        {
            retval = TRUE;
        }

        altErr -= self->ZDelta();
        GammaHold(altErr * 0.015F);
    }
    else
    {
        if (psiErr > 0.0F)
            turnDir = 1.0F;
        else
            turnDir = -1.0F;

        if (wvrCurrTactic == wvrPrevTactic)
            newTurn = FALSE;
        else
            newTurn = TRUE;

        LevelTurn(2.0F, turnDir, newTurn);
    }

    return (retval);
}

void DigitalBrain::GammaHold(float desGamma)
{
    float elevCmd;
    float gammaCmd;

    // MD -- 20031110: AP ATT HLD fixes.  Dash one says the pitch hold works for +/- 60 degrees not 30
    // desGamma = max ( min ( desGamma, 30.0F), -30.0F);
    desGamma = max(min(desGamma, 60.0F), -60.0F);
    elevCmd = desGamma - af->gmma * RTD;

    elevCmd *= 0.25F * self->GetKias() / 350.0F;

    if (fabs(af->gmma) < (45.0F * DTR))
        elevCmd /= self->platformAngles.cosphi;

    if (elevCmd > 0.0F)
        elevCmd *= elevCmd;
    else
        elevCmd *= -elevCmd;

    gammaHoldIError += 0.0025F * elevCmd;

    if (gammaHoldIError > 1.0F)
        gammaHoldIError = 1.0F;
    else if (gammaHoldIError < -1.0F)
        gammaHoldIError = -1.0F;

    gammaCmd = gammaHoldIError + elevCmd + (1.0F / self->platformAngles.cosphi);
    SetPstick(min(max(gammaCmd, -2.0F), 6.5F), maxGs, AirframeClass::GCommand);
}

void DigitalBrain::RollOutOfPlane(void)
{
    float eroll;

    /*-----------------------*/
    /* first pass, save roll */
    /*-----------------------*/
    if (lastMode not_eq RoopMode)
    {
        mnverTime = 1.0F;//me123 from 4

        /*----------------------------------------------------*/
        /* want to roll toward the vertical but limit to keep */
        /* droll < 45 degrees.                                */
        /*----------------------------------------------------*/
        if (self->Roll() >= 0.0)
        {
            newroll = self->Roll() - 30.0F * DTR; //me123 don't do a fucking quarterplane :-) from 45
        }
        else
        {
            newroll = self->Roll() + 30.0F * DTR; //me123 don't do a fucking quarterplane :-) from 45
        }
    }

    /*------------*/
    /* roll error */
    /*------------*/
    eroll = newroll - self->Roll();

    /*-----------------------------*/
    /* roll the shortest direction */
    /*-----------------------------*/
    if (eroll < -180.0F * DTR)
        eroll += 360.0F * DTR;
    else if (eroll > 180.0F * DTR)
        eroll -= 360.0F * DTR;

    SetPstick(af->GsAvail(), maxGs, AirframeClass::GCommand);
    SetRstick(eroll * RTD);

    /*-----------*/
    /* exit mode */
    /*-----------*/
    mnverTime -= SimLibMajorFrameTime;

    if (mnverTime > 0.0)
    {
        AddMode(RoopMode);
    }
}

void DigitalBrain::OverBank(float delta)
{
    float eroll = 0.0F;

    if (targetData == NULL)
        return;

    /*-------------------------*/
    /* not in a vertical fight */
    /*-------------------------*/
    if (fabs(self->Pitch()) < 45.0 * DTR) //me123 from 70
    {
        /*-----------------------*/
        /* Find a new roll angle */
        /*-----------------------*/
        if (lastMode not_eq OverBMode)
        {
            if (self->Roll() > 0.0F)
                newroll = targetData->droll + delta;
            else
                newroll = targetData->droll - delta;

            if (newroll > 180.0F * DTR)
                newroll -= 360.0F * DTR;
            else if (newroll < -180.0F * DTR)
                newroll += 360.0F * DTR;
        }

        eroll = newroll - self->Roll();
        SetRstick(eroll * RTD);
    }

    /*------*/
    /* exit */
    /*------*/
    if (fabs(eroll) > 1.0)
    {
        AddMode(OverBMode);
    }
}

int HoldCorner(int combatClass, SimObjectType* targetPtr)
{
    int i, hisCombatClass;
    DigitalBrain::ManeuverChoiceTable *theIntercept;
    int retval = TRUE; // Assume you can engage
    return true ;
    //me123 hack hack request from saint
    //always alow corner hold until we fix the disengage stuff

    // Only check for A/C
    // if (targetPtr->BaseData()->IsSim() and targetPtr->BaseData()->IsAirplane())
    if (targetPtr->BaseData()->IsAirplane() or targetPtr->BaseData()->IsFlight()) // 2002-02-26 MODIFIED BY S.G. airplane and fligth are ok in here
    {
        // Find the data table for these two types of A/C
        hisCombatClass = targetPtr->BaseData()->CombatClass(); // 2002-02-26 MODIFIED BY S.G. Removed the AircraftClass cast
        theIntercept = &(DigitalBrain::maneuverData[combatClass][hisCombatClass]);

        for (i = 0; i < theIntercept->numMerges; i++)
        {
            if (theIntercept->merge[0] == DigitalBrain::WvrMergeHitAndRun)
            {
                retval = FALSE;
                break;
            }
        }

        //me123 don't blow corner for now, i wanna test them
        //     if ( not retval)
        //     {
        // Chance of blowing corner speed is proportional to your number of choices
        //        if ((float)rand()/(float)RAND_MAX > 1.0F / theIntercept->numMerges)
        //           retval = TRUE;
        //     }
    }

    return retval;
}
