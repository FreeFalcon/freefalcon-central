#include "stdhdr.h"
#include "digi.h"
#include "simveh.h"
#include "aircrft.h"
#include "airframe.h"
#include "simdrive.h"
#include "tankbrn.h" // 2002-03-15 MN

extern float SimLibLastMajorFrameTime;
extern int gameCompressionRatio;

#include "Arfrmdat.h" // 2002-01-30 S.G.
#define MAX_AF_PITCH ( DTR * 89.0f )// 2002-01-30 S.G.
extern AeroDataSet *aeroDataset; // 2002-01-30 S.G.
extern bool g_bPitchLimiterForAI; // 2002-01-30 S.G.
extern AuxAeroData *auxaeroData; // 2002-01-30 S.G.
extern float g_fTankerRStick; // 2002-03-13 MN
extern float g_fTankerPStick; // 2002-03-13 MN

//--------------------------------------------------
//
// DigitalBrain::SimplePullUp
//
//--------------------------------------------------

void DigitalBrain::SimplePullUp(void)
{

    pStick = 0.1F;
    throtl = 1.0F;
    rStick = 0.0F;
    yPedal = 0.0F;
}


//--------------------------------------------------
//
// DigitalBrain::SimpleTrack
//
//--------------------------------------------------

void DigitalBrain::SimpleTrack(SimpleTrackMode mode, float value)
{
    float xft;
    float yft;
    float zft;
    float rx;
    float ry;
    float rz;

    if ( not self->OnGround())
    {
        if (mode == SimpleTrackDist)
        {
            CalculateRelativePos(&xft, &yft, &zft, &rx, &ry, &rz);

            if (flightLead)
            {
                if (flightLead->LastUpdateTime() == vuxGameTime)
                {
                    // Correct for different frame rates
                    xft -= flightLead->XDelta() * SimLibMajorFrameTime;
                    yft -= flightLead->YDelta() * SimLibMajorFrameTime;
                    //            xft += flightLead->XDelta()*SimLibLastMajorFrameTime;
                    //            yft += flightLead->YDelta()*SimLibLastMajorFrameTime;
                }

                SimpleTrackDistance(flightLead->GetVt(), (float)sqrt(xft * xft + yft * yft)); // Get Leader's speed, relative position
            }

            rStick = SimpleTrackAzimuth(rx, ry, self->GetVt());
            pStick = SimpleTrackElevation(zft, 5000.0F);
        }
        else if (mode == SimpleTrackSpd)
        {
            CalculateRelativePos(&xft, &yft, &zft, &rx, &ry, &rz);

            SimpleTrackSpeed(value); // value = desired speed (ft/sec)

            rStick = SimpleTrackAzimuth(rx + 1000.0F, ry, self->GetVt());
            pStick = SimpleTrackElevation(zft, 5000.0F);

            // Capture heading first, then pitch
            /*
            ** edg: I don't know where this comes from but its totally porked
            if (fabs(rStick) < 10.0F * DTR)
               pStick = SimpleTrackElevation(zft, 10000.0F);
            else
               pStick = 0.0F;
            */
        }
        else if (mode == SimpleTrackTanker)
        {

            bool sticklimitation = false;

            if (self->TBrain() and self->TBrain()->ReachedFirstTrackPoint())
                sticklimitation = true; // tanker is entering the track pattern

            SimpleTrackSpeed(value); // value = desired speed (ft/sec)

            CalculateRelativePos(&xft, &yft, &zft, &rx, &ry, &rz);

            rStick = SimpleTrackAzimuth(rx + 1000.0F, ry, self->GetVt());

            if (sticklimitation)
                rStick = min(g_fTankerRStick, max(rStick, -g_fTankerRStick));
            else
                rStick = min(0.2f, max(rStick, -0.2f));

            //rStick = 0.0F;

            //it's better if the tanker just goes to the desired altitude and stays there
            //pStick = SimpleTrackElevation(trackZ - self->ZPos(), 100000.0F);
            pStick = SimpleTrackElevation(trackZ - self->ZPos(), 10000.0F);

            if (sticklimitation)
                pStick = min(g_fTankerPStick, max(pStick, -g_fTankerPStick));
            else
                pStick = min(0.02f, max(pStick, -0.02f));
        }
    }
    else
    {
        CalculateRelativePos(&xft, &yft, &zft, &rx, &ry, &rz);

        if (gameCompressionRatio)
            // JB 020315 Why divide by gameCompressionRation?  It screws up taxing for one thing.
            rStick = SimpleTrackAzimuth(rx , ry, self->GetVt());///gameCompressionRatio;
        else
            rStick = 0.0f;

        pStick = SimpleTrackElevation(zft, 5000.0F);
        throtl = SimpleGroundTrackSpeed(value);// value = desired speed (ft/sec)
    }

    yPedal = 0.0f;

}

//--------------------------------------------------
//
// DigitalBrain::CalculateRelativePos
//
//--------------------------------------------------
void DigitalBrain::CalculateRelativePos(float* xft, float* yft, float* zft, float* rx, float* ry, float* rz)
{

    // Calculate relative position and range to track point
    *xft = trackX - self->XPos();
    *yft = trackY - self->YPos();
    *zft = trackZ - self->ZPos();

    *rx = self->dmx[0][0] * *xft + self->dmx[0][1] * *yft + self->dmx[0][2] * *zft;
    *ry = self->dmx[1][0] * *xft + self->dmx[1][1] * *yft + self->dmx[1][2] * *zft;
    *rz = self->dmx[2][0] * *xft + self->dmx[2][1] * *yft + self->dmx[2][2] * *zft;
}

//--------------------------------------------------
//
// DigitalBrain::SimpleScaleThrottle
//
//--------------------------------------------------

float DigitalBrain::SimpleScaleThrottle(float v)
{
    return  1.0F + (v - (450.0F * KNOTS_TO_FTPSEC)) / (450.0F * KNOTS_TO_FTPSEC);
}



//--------------------------------------------------
//
// DigitalBrain::SimpleTrackDistance
//
//--------------------------------------------------
float DigitalBrain::SimpleTrackDistance(float, float rx)
{
    float desiredClosure, actualClosure;

    //desiredClosure = 200.0F * rx/(1.0F*NM_TO_FT) - 200.0F;
    desiredClosure = 350.0F * rx / (1.0F * NM_TO_FT) - 350.0F; // JB 011016 Increase closure rates to adjust for the vt/GetKias bug fix below

    // get actual closure
    actualClosure = -(rx - velocitySlope) / SimLibLastMajorFrameTime;
    //me123 machhold needs knots  chenged af->vt() to self->GetKias
    MachHold(self->GetKias() + desiredClosure - actualClosure, self->GetKias(), FALSE);
    velocitySlope = rx;
    return throtl;
}




//--------------------------------------------------
//
// DigitalBrain::SimpleTrackSpeed
//
//--------------------------------------------------
//TJL 02/20/04 Severe confusion in the code with v (in KNOTS) and vt (FPS)
float DigitalBrain::SimpleTrackSpeed(float v)
{
    //TJL 02/20/04 They had v here in knots comparing to vt in FPS. I have commented this out
    //if(af->Qsom()*af->Cnalpha() < 1.55F and v < af->vt + 5.0F)
    // v = af->vt /* * */ + 5.0F; // 2001-10-27 M.N. removed "*", caused af->vt * 5
    // Lets Try MachHold on velocity (TJL) Again, more confusion with v in knots being converted to knots
    //MachHold (v * FTPSEC_TO_KNOTS , af->vt * FTPSEC_TO_KNOTS , FALSE);
    MachHold(v, af->vt * FTPSEC_TO_KNOTS , FALSE);

    return throtl;
}

float  DigitalBrain::SimpleGroundTrackSpeed(float v)
{
    if (af->vt > v + 2.0F)
        af->SetFlag(AirframeClass::WheelBrakes);
    else
        af->ClearFlag(AirframeClass::WheelBrakes);

    if (af->vt > 20.0F and v > 20.0F)
    {
        float eProp = v - af->vt;

        if (eProp >= 50.0F)
        {
            autoThrottle = 1.5F;
            throtl = 1.5F;                        /* burner     */
        }
        else if (eProp < -50.0F)
        {
            autoThrottle = 0.0F;
            throtl = 0.0F;                        /* idle and boards */
        }

        autoThrottle += eProp * 0.005F * SimLibMajorFrameTime;
        autoThrottle = max(0.0F, min(1.5F, autoThrottle));
        throtl = eProp * 0.005F + autoThrottle - af->vtDot * SimLibMajorFrameTime * 0.005F;
    }
    else
    {
        autoThrottle = throtl = af->CalcThrotlPos(v); // v = desired speed (ft/sec)
    }

    return throtl;
}

//--------------------------------------------------
//
// DigitalBrain::SimpleTrackAzimuth
//
//--------------------------------------------------

float DigitalBrain::SimpleTrackAzimuth(float rx, float ry, float)
{
    float azErr;


    // Clamp/limit for in air
    if ( not self->OnGround())
    {
        // Calculate azimuth error
        azErr = (float) atan2(ry, rx);

        if (rx < 0.0F and (fabs(rx) < 3.0F * NM_TO_FT))
        {

            // If our track point is to the right and behind us
            if ((azErr > 0.0F) and (azErr > 90 * DTR))
            {

                // Change the azErr to be infront so that we don't backtrack
                azErr = (180 * DTR) - azErr;
            }
            else if ((azErr < 0.0F) and (azErr < -90 * DTR))   // else to the left and behind
            {

                // Change the azErr to be infrom so that we don't backtrack
                azErr = (-180 * DTR) - azErr;
            }
        }

        // azErr *= 0.75F;  // smooth it out a little
        // edg: the azimuth error should really be "normalized" to some arc of
        // rotation.  Let's make this 180 deg

        // 2002-01-31 ADDED BY S.G. Lets limit the roll of an AI controlled plane when going from one waypoint to the next
        if (g_bPitchLimiterForAI and // We are asking to limit AI's turn agressiveness when flying to waypoints
 not groundAvoidNeeded and // We're not trying to avoid the ground
            (curMode == WingyMode or curMode == WaypointMode or curMode == RTBMode) and // Following waypoint or lead
            agDoctrine == AGD_NONE and // Not doing any A2G attack (since it's in FollowWaypoints during that time)
            ( not flightLead or not flightLead->IsSetFlag(MOTION_OWNSHIP)))   // The lead isn't the player (we must follow him whatever he does)
        {

            azErr /= ((180.0f) * DTR);

            float maxRoll = self->af->GetRollLimitForAiInWP() * DTR;
            float curRoll = (float)fabs(self->Roll()) * 0.85f; // Current Roll with some leadway
            float scale;

            // scale the role based on on the difference of roll from max
            if (curRoll > maxRoll)
                scale = 0.0;
            else
                scale = (float)sqrt((maxRoll - curRoll) / maxRoll); // Give more weights being toward zero

            azErr *= scale;
        }
        else
        {
            // END OF ADDED SECTION 2002-01-31
            // 2001-10-25 CHANGED BACK by M.N. 40° caused planes to jink around when changing to another trackpoint
            azErr /= ((180.0f) * DTR);  //ME123 HOW ABOUT 40
        }
    }
    else
    {
        // Calculate azimuth error
        azErr = (float) atan2(ry, rx);
    }


    return max(-1.0F, min(1.0F, azErr)); // return the roll command
}



//--------------------------------------------------
//
// DigitalBrain::SimpleTrackElevation
//
//--------------------------------------------------

float DigitalBrain::SimpleTrackElevation(float zft, float scale)
{
    float altErr;

    // JPO - don't mess with stuff if we're taking avoiding action
    if (groundAvoidNeeded or pullupTimer)
        return pStick;

    // Scale and limit the altitude error
    altErr = -zft / scale;  // Use 5000ft for error slope

    if (fabs(zft) > 2000.0f)
        altErr *= 0.5f;

    // limit climb based on airspeed
    if (-zft > 0.0f and af->vt < 600.0f * KNOTS_TO_FTPSEC)
    {
        altErr *= af->vt / (600.0f * KNOTS_TO_FTPSEC);
    }

    // 2002-01-30 ADDED BY S.G. Lets limit the pitch when we're at max climb angle
    if (g_bPitchLimiterForAI and not groundAvoidNeeded and 
        (curMode == WingyMode or curMode == WaypointMode or curMode == RTBMode) and 
        /*agDoctrine == AGD_NONE and *///Cobra removed this // Not doing any A2G attack (since it's in FollowWaypoints during that time)
        ( not flightLead or not flightLead->IsSetFlag(MOTION_OWNSHIP) or ((AircraftClass *)flightLead)->autopilotType == AircraftClass::CombatAP) and // The lead isn't the player (we must follow him whatever he does)
        altErr > 0.0f and self->Pitch() > 0.0f)
    {
        float maxPitch = min(MAX_AF_PITCH, aeroDataset[self->af->VehicleIndex()].inputData[AeroDataSet::ThetaMax]);
        float curPitch = self->Pitch() * 0.85f; // Current pitch with some leadway
        float scale;

        // scale the pitch based on on the difference of pitch from max
        if (curPitch > maxPitch)
            scale = 0.0f;
        else
            scale = (float)sqrt((maxPitch - curPitch) / maxPitch); // Give more weights being toward zero

        altErr *= scale;

        // scale the pitch based on on the difference of speed
        float minVcas = aeroDataset[self->af->VehicleIndex()].inputData[AeroDataSet::MinVcas];
        float curKias = self->GetKias(); // Current speed

        // If we're way above our best climb speed, lets pitch up a bit more to drain some speed and get some altitude
        if (minVcas * 1.9f < curKias and minVcas not_eq 0.0f)
            altErr *=  curKias / (minVcas * 1.9f);
        else
        {
            // If we are below the stall speed, wait until you get their before pitching up
            if (minVcas > curKias)
                scale = 0.0f;
            // If we're above or best climb speed, adjust the pitch
            else if (minVcas * 1.5f > curKias)
                scale = (float)sqrt((curKias - minVcas) / (minVcas * 0.5f)); // Give more weights being toward zero
            else
                scale = 1.0f; // You're fine, for now

            altErr *= scale;
        }
    }

    // END OF ADDED SECTION 2002-01-30

    return max(-0.5F, min(0.5F, altErr)); // Return the pitch command
}



