#include "stdhdr.h"
#include "hud.h"
#include "fcc.h"
#include "Graphics/Include/display.h"
#include "aircrft.h"
#include "simmover.h"
#include "flightData.h"
#include "sms.h"// status ok.
#include "laserpod.h" //MI
#include "simdrive.h" //MI

#include "simWeapn.h" //Wombat778 3-09-04
#include "classtbl.h" //Wombat778 3-09-04

#include "harmpod.h" // RV - I-Hawk

extern SensorClass* FindLaserPod(SimMoverClass* theObject); //MI

static const float PIPPER_SIZE = 0.05f;
//MI
static const float OUTER_RETICLE_SIZE = 0.18F;
static const float INNER_RETICLE_SIZE = 0.1F;
static const float TICK_LEN = 0.04F;
static const float RET_MIN = hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F;
static const float RET_MAX = -0.55F;
static const float MAX_STEPS = 12; //how many steps that we can move the reticle

extern bool g_bRealisticAvionics;
extern float g_fReconCameraOffset;
//Cobra 10/31/04 TJL Copied over entire file..too many differences to manually merge.

void HudClass::DrawAirGroundRocket(void) // MLR 4/3/2004 -
{
    DrawRCKT();
    /*
    if(g_bRealisticAvionics)
    {
     DrawMANReticle();
    }
    else
    {
     DrawRCKT();
    }
    */
}


void HudClass::DrawAirGroundGravity(void)
{
    switch (FCC->GetSubMode())
    {
        case FireControlComputer::CCIP:
            DrawCCIP();
            break;

            // MLR 6/5/2004 - Shouldn't get in here...
        case FireControlComputer::OBSOLETERCKT: // MLR 4/3/2004 -
            DrawRCKT();
            break;

        case FireControlComputer::CCRP:
            DrawCCRP();
            break;

        case FireControlComputer::DTOSS:
            DrawDTOSS();
            break;

        case FireControlComputer::LADD:
            DrawLADD();
            break;

        case FireControlComputer::STRAF:
            DrawStrafe();
            break;

        case FireControlComputer::MAN:
            DrawMANReticle();
            break;
    }
}


void HudClass::DrawCCIP(void)
{
    float vOffset;
    float pipperAz, pipperEl;
    float x, y;
    float len, droll;
    float puacY;//me123 status ok. insert.
    mlTrig azTrig, elTrig, drollTrig;
    //me123 tofextra is the extra tof for the last bomb(this is the bomb the puac symbolice) becourse it has to wait for the preciding bombs to drop
    //float  TofExtra = ( (FCC->Sms->RippleInterval())* ((FCC->Sms->RippleCount()+1))/ // MLR 4/3/2004 -
    float  TofExtra = ((FCC->Sms->GetAGBRippleInterval()) * ((FCC->Sms->GetAGBRippleCount() + 1)) /
                       ((float) sqrt(ownship->XDelta() * ownship->XDelta() + ownship->YDelta() * ownship->YDelta())));

    //if (FCC->Sms->Pair() == TRUE) // MLR 4/3/2004 -
    if (FCC->Sms->GetAGBPair() == TRUE)
    {
        TofExtra *= 0.5f;
    }

    // Draw a TD box if we have a locked target
    // NOTE:  In reality there would never be a locked target since the radar would be in AGR, but
    // if we allow a radar lock, we might as well allow a TD box...
    //MI
    if ( not g_bRealisticAvionics)
        DrawTDBox();

    // Compute and set the viewport offset to get 0,0 at the boresight cross
    vOffset = hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F;
    display->AdjustOriginInViewport(0.0F, vOffset);


    // If we've got a designated target, provide steering
    if (FCC->airGroundDelayTime > 0.0F)
    {
        // Draw the impact point marker
        DrawDesignateMarker(Circle, FCC->groundDesignateAz, FCC->groundDesignateEl, FCC->groundDesignateDroll);

        // Clear the viewport offset
        display->AdjustOriginInViewport(0.0F, -vOffset);

        // provide the steering to the release point
        DrawSteeringToRelease();
        return;
    }


    // Get on with the job of getting a designated target (or just dropping)
    pipperAz = FCC->groundPipperAz;
    pipperEl = FCC->groundPipperEl;

    // Draw the pipper

    // FRB - Try not compensating for screen aspect.
    //x = RadToHudUnits(pipperAz);
    //y = RadToHudUnits(pipperEl);
    x = RadToHudUnitsX(pipperAz);
    y = RadToHudUnitsY(pipperEl);

    display->Circle(x, y, PIPPER_SIZE);
    display->Point(x, y);

    // Draw the bomb fall line connecting the FPM with the exterior of the pipper
    mlSinCos(&azTrig, pipperAz);
    mlSinCos(&elTrig, pipperEl);
    droll = (float)atan2(azTrig.sin, elTrig.sin);
    mlSinCos(&drollTrig, droll);

    len = RadToHudUnits((float)acos(azTrig.cos * elTrig.cos)) - PIPPER_SIZE;
    x = len * drollTrig.sin;
    y = len * drollTrig.cos;

    display->Line(x, y, betaHudUnits, -alphaHudUnits);


    // See if the computed impact point is visible on the HUD
    if ( not FCC->groundPipperOnHud)
    {
        // Draw the Delay Cue Tick half way along the fall line
        // me123 status ok. x and y definitions are inset in the formula below instead so the original x/y's can be used below
        display->Line(((x + betaHudUnits)  / 2.0f - PIPPER_SIZE), (y - alphaHudUnits) / 2.0f, ((x + betaHudUnits)  / 2.0f) + PIPPER_SIZE, (y - alphaHudUnits) / 2.0f);
    }

    ///me123 status ok. insert PUAC in ccip

    //if we are above the hight where the bombs will have time to arm before impact
    //then draw the puac so it hit's the fpm when min release higt is reached.

    // no need to go though this if we are way above mra
    if (FCC->groundImpactTime < 25)
    {
        if ((FCC->groundImpactTime - TofExtra) > (FCC->Sms->armingdelay / 100))
        {
            puacY = ((FCC->groundImpactTime - TofExtra) / (FCC->Sms->armingdelay / 100)); // 1 at minimum release alt


            // draw the puac on the bomb fall line
            //MI vids show it stays below the FPM
            if ( not g_bRealisticAvionics)
                x = ((puacY - 1) * drollTrig.sin  + betaHudUnits);
            else
                x = betaHudUnits;

            //draw the puac
            puacY = - alphaHudUnits - ((puacY / 3) - (0.33F)) ; //this is fpm pos when at minimum release alt.

            display->Line(x - 0.075F, puacY, x + 0.075F, puacY);
            display->Line(x - 0.075F, puacY, x - 0.075F, puacY + 0.025F);
            display->Line(x + 0.075F, puacY, x + 0.075F, puacY + 0.025F);

            // Now we are below arming time so we show the "low" and change the PUAC function
            // so it hit's the fmp when the ground is reached (when we crash)
        }
        else
        {
            // now we use the PUAC to "count down" to ground impact
            puacY =  FCC->groundImpactTime / 4;

            // draw the puac on the bomb fall line
            //  x = (x  + betaHudUnits) * -((puacY-1)/y ) ;//me123 this might crash when the bombline length (y) is zero.
            //MI vids show it stays below the FPM
            if ( not g_bRealisticAvionics)
                x = ((puacY - 1) * drollTrig.sin  + betaHudUnits);
            else
                x = betaHudUnits;

            // draw the Puac
            puacY =  - alphaHudUnits - puacY  ; // fpm pos at ground impact
            display->Line(x - 0.075F, puacY, x + 0.075F, puacY);
            display->Line(x - 0.075F, puacY, x - 0.075F, puacY + 0.025F);
            display->Line(x + 0.075F, puacY, x + 0.075F, puacY + 0.025F);

            display->TextLeft(betaHudUnits + 0.1F, -alphaHudUnits, "LOW");
        }
    }

    ////insert end me123

    // Clear the viewport offset
    display->AdjustOriginInViewport(0.0F, -vOffset);

    //MI
    if (g_bRealisticAvionics)
    {
        TimeToSteerpoint();
        RangeToSteerpoint();
    }
}


void HudClass::DrawRCKT(void)
{
    float vOffset, diam, tic, stic;
    float RngToGo, yOffset;
    float x, y, puacY;

    puacY = 0;

    float droll;
    mlTrig azTrig, elTrig, drollTrig;
    float pipperAz, pipperEl;
    char tmpStr[32];

    // Cobra test
    static FILE *fp = NULL;
    //if (fp == NULL)
    //fp = fopen("G:\\RocketTest.txt", "w");

    // Draw a TD box if we have a locked target
    // NOTE:  In reality there would never be a locked target since the radar would be in AGR, but
    // if we allow a radar lock, we might as well allow a TD box...
    //MI
    if ( not g_bRealisticAvionics)
        DrawTDBox();

    // FRB - Try not compensating for screen aspect.
    //pipperAz = RadToHudUnitsX(FCC->groundPipperAz);
    //pipperEl = RadToHudUnitsY(FCC->groundPipperEl);
    pipperAz = RadToHudUnits(FCC->groundPipperAz);
    pipperEl = RadToHudUnits(FCC->groundPipperEl);
    mlSinCos(&azTrig, pipperAz);
    mlSinCos(&elTrig, pipperEl);
    droll = (float)atan2(azTrig.sin, elTrig.sin);
    mlSinCos(&drollTrig, droll);

    // draw the pipper
    //display->AdjustOriginInViewport(
    // 0.0F, hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F
    //);

    vOffset = hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F;

    if (fp)
    {
        float rng = FCC->airGroundRange;
        fprintf(fp, "**--** Pip Az %f Pip El %f Gnd Az %f Gnd El %f Range %f \n",
                pipperAz, pipperEl, FCC->groundPipperAz * RTD, FCC->groundPipperEl * RTD, rng);
        fflush(fp);
    }

    // FPM position
    float dx = betaHudUnits;
    float dy = vOffset - alphaHudUnits;

    yOffset = -0.40f;
    pipperEl += yOffset;

    // draw the pipper
    // Well, really it is centered on the flight path marker*/
    display->AdjustOriginInViewport(betaHudUnits, dy);

    diam = 0.15f;
    tic = 0.04f;
    stic = 0.02f;
    float maxRng = 36500.0f;
    float minRng = 18500.0F;

    if (fabs(pipperEl) < 0.90F and fabs(pipperAz + vOffset) < 0.90F and FCC->airGroundRange < maxRng and not FCC->noSolution)
    {
        if (FCC->airGroundRange < minRng)
        {
            display->Line(pipperAz - diam, pipperEl + diam - 0.01F, pipperAz, pipperEl + diam + 0.02F);
            display->Line(pipperAz, pipperEl + diam + 0.02F, pipperAz + diam, pipperEl + diam - 0.01F);
        }

        display->Circle(pipperAz, pipperEl, diam);
        display->Line(pipperAz, pipperEl + diam, pipperAz, pipperEl + diam + tic);
        display->Line(pipperAz, pipperEl - diam, pipperAz, pipperEl - diam - tic);
        display->Line(pipperAz + diam, pipperEl, pipperAz + diam + tic, pipperEl);
        display->Line(pipperAz - diam, pipperEl, pipperAz - diam - tic, pipperEl);

        float sx = sin(45 * DTR) * diam;
        float sy = cos(45 * DTR) * diam;
        float sticx = sin(45 * DTR) * (diam + stic);
        float sticy = cos(45 * DTR) * (diam + stic);
        display->Line(pipperAz + sx, pipperEl + sy, pipperAz + sticx, pipperEl + sticy);
        display->Line(pipperAz + sx, pipperEl - sy, pipperAz + sticx, pipperEl - sticy);
        display->Line(pipperAz - sx, pipperEl + sy, pipperAz - sticx, pipperEl + sticy);
        display->Line(pipperAz - sx, pipperEl - sy, pipperAz - sticx, pipperEl - sticy);

        if (FCC->airGroundRange <= minRng)
            RngToGo = 270.0f;
        else if (FCC->airGroundRange >= maxRng)
            RngToGo = 270.9f;
        else
            RngToGo = (((FCC->airGroundRange - minRng) / (maxRng - minRng)) * 360.0f) - 90.0f;

        if (RngToGo < 0.0f)
            RngToGo += 360.0f;

        display->Arc(pipperAz, pipperEl, diam - 0.02f, 270.0f * DTR, (RngToGo)*DTR);

        //TJL 11/20/03
        x = pipperAz;
        y = pipperEl;
        display->Point(x, y);//TJL 11/20/03 Hopefully this puts a point in the middle.
        display->Line(x, y + diam, 0.0f, 0.0f);
        //display->Line (x, y, 0.0f, -0.25f);
    }
    else
    {
        display->Circle(0.0f, yOffset, diam);
        display->Line(0.0f, yOffset + diam, 0.0f, yOffset + diam + tic);
        display->Line(0.0f, yOffset - diam, 0.0f, yOffset - diam - tic);
        display->Line(diam, yOffset, diam + tic, yOffset);
        display->Line(-diam, yOffset, -diam - tic, yOffset);
        display->Circle(0.0f, yOffset, diam - 0.02f);
        display->Line(0.0f, yOffset + diam, 0.0f, 0.0f);
        float sx = sin(45 * DTR) * diam;
        float sy = cos(45 * DTR) * diam + yOffset;
        float sticx = sin(45 * DTR) * (diam + stic);
        float sticy = cos(45 * DTR) * (diam + stic) + yOffset;
        display->Line(sx, sy, sticx, sticy);
        display->Line(sx, -sy,   sticx, -sticy);
        display->Line(-sx,  sy, - sticx,  sticy);
        display->Line(-sx, -sy, - sticx, -sticy);
    }

    display->AdjustOriginInViewport(-betaHudUnits, -dy);
    vOffset = hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F;
    display->AdjustOriginInViewport(0.0F, vOffset);

#if 0 // FRB - Removed PUAC for rockets.

    //PUAC for max Range
    if (FCC->airGroundRange < maxRng and FCC->airGroundRange >= minRng)
    {
        // now we use the PUAC to "count down" to in-range
        puacY = ((FCC->airGroundRange - minRng) / (maxRng - minRng));

        // draw the puac on the bomb fall line
        //MI vids show it stays below the FPM
        if ( not g_bRealisticAvionics)
            x = ((puacY - 1) * drollTrig.sin  + betaHudUnits);
        else
            x = betaHudUnits;

        // draw the Puac
        puacY =  - alphaHudUnits - puacY  ; // fpm pos at ground impact
        display->Line(x - 0.075F, puacY, x + 0.075F, puacY);
        display->Line(x - 0.075F, puacY, x - 0.075F, puacY + 0.025F);
        display->Line(x + 0.075F, puacY, x + 0.075F, puacY + 0.025F);

    }

#endif

    display->AdjustOriginInViewport(0.0F, -vOffset);

    // Add slant range
    if (FCC->airGroundRange > 1.0F * NM_TO_FT)
        sprintf(tmpStr, "F %4.1f", max(min(100.0F, FCC->airGroundRange * FT_TO_NM), 0.0F));
    else
        sprintf(tmpStr, "F %03.0f", max(min(10000.0F, FCC->airGroundRange * 0.01F), 0.0F));

    ShiAssert(strlen(tmpStr) < sizeof(tmpStr));
    DrawWindowString(10, tmpStr);
}


void HudClass::DrawStrafe(void)
{
    float vOffset;
    float droll;
    mlTrig azTrig, elTrig, drollTrig;
    float puacY;
    float pipperAz, pipperEl;
    //TJL 11/20/03
    float x = 0.0f;
    float y = 0.0f;
    //char tmpStr[32];

    // Draw a TD box if we have a locked target
    DrawTDBox();

    // FRB - Try not compensating for screen aspect.
    //pipperAz = RadToHudUnitsX(FCC->groundPipperAz);
    //pipperEl = RadToHudUnitsY(FCC->groundPipperEl);
    pipperAz = RadToHudUnits(FCC->groundPipperAz);
    pipperEl = RadToHudUnits(FCC->groundPipperEl);
    mlSinCos(&azTrig, pipperAz);
    mlSinCos(&elTrig, pipperEl);
    droll = (float)atan2(azTrig.sin, elTrig.sin);
    mlSinCos(&drollTrig, droll);


    display->AdjustOriginInViewport(0.0F, hudWinY[BORESIGHT_CROSS_WINDOW] +
                                    hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F);

    if (fabs(pipperEl) < 0.90F and fabs(pipperAz + hudWinY[BORESIGHT_CROSS_WINDOW] +
                                       hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F) < 0.90F and not FCC->noSolution)
    {
        if (FCC->airGroundRange < 8000.0F)// me123 status ok. changed from 8000. TJL 11/20/03 Back to 8000 per MIRV
        {
            display->Line(pipperAz - 0.05F, pipperEl + 0.05F, //me123 status test. changed the last number from 0.05 to 0.15 to make strafcircle bigger
                          pipperAz + 0.05F, pipperEl + 0.05F);//me123 status test. changed the last number from 0.05 to 0.15 to make strafcircle bigger
        }

        display->Circle(pipperAz, pipperEl, 0.05F); //me123 status test. changed the last number from 0.05 to 0.15 to make strafcircle bigger
        //TJL 11/20/03
        x = pipperAz;
        y = pipperEl;
        display->Point(x, y);//TJL 11/20/03 Hopefully this puts a point in the middle.
    }
    else
    {
        display->Circle(0.0F, 0.0F, 0.05F); //me123 status test, changed the last number from 0.05 to 0.15 to make strafcircle bigger
    }

    display->AdjustOriginInViewport(0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +

                                            hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));

    // Compute and set the viewport offset to get 0,0 at the boresight cross
    vOffset = hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F;
    display->AdjustOriginInViewport(0.0F, vOffset);

    //PUAC for strafe
    if (FCC->groundImpactTime < 25)
    {
        // now we use the PUAC to "count down" to ground impact
        puacY =  FCC->groundImpactTime / 4;

        // draw the puac on the bomb fall line
        //  x = (x  + betaHudUnits) * -((puacY-1)/y ) ;//me123 this might crash when the bombline length (y) is zero.
        //MI vids show it stays below the FPM
        if ( not g_bRealisticAvionics)
            x = ((puacY - 1) * drollTrig.sin  + betaHudUnits);
        else
            x = betaHudUnits;

        // draw the Puac
        puacY =  - alphaHudUnits - puacY  ; // fpm pos at ground impact
        display->Line(x - 0.075F, puacY, x + 0.075F, puacY);
        display->Line(x - 0.075F, puacY, x - 0.075F, puacY + 0.025F);
        display->Line(x + 0.075F, puacY, x + 0.075F, puacY + 0.025F);

    }

    display->AdjustOriginInViewport(0.0F, -vOffset);

    // Add slant range
    if (FCC->airGroundRange > 1.0F * NM_TO_FT)
        sprintf(tmpStr, "F %4.1f", max(min(100.0F, FCC->airGroundRange * FT_TO_NM), 0.0F));
    else
        sprintf(tmpStr, "F %03.0f", max(min(10000.0F, FCC->airGroundRange * 0.01F), 0.0F));

    ShiAssert(strlen(tmpStr) < sizeof(tmpStr));

    //MI
    if ( not g_bRealisticAvionics) //done in the routines below
        DrawWindowString(10, tmpStr);

    //else
    //   display->TextLeft(0.40F, -0.36F, tmpStr);

    //MI
    if (g_bRealisticAvionics)
    {
        TimeToSteerpoint();
        RangeToSteerpoint();
    }
}


void HudClass::DrawCCRP(void)
{
    // Draw the TD box
    display->AdjustOriginInViewport(0.0F, (hudWinY[BORESIGHT_CROSS_WINDOW] +
                                           hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
    DrawDesignateMarker(Square, FCC->groundDesignateAz, FCC->groundDesignateEl, FCC->groundDesignateDroll);
    display->AdjustOriginInViewport(0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
                                            hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));

    // Provide steering to the release point
    DrawSteeringToRelease();
}


void HudClass::DrawDTOSS(void)
{
    // Draw a TD box if we have a locked target
    // NOTE:  In reality there would never be a locked target since the radar would be in AGR, but
    // if we allow a radar lock, we might as well allow a TD box...

    //MI changed
    if ( not g_bRealisticAvionics)
        DrawTDBox();
    else
        DrawDTOSSBox();

    // Draw the impact point marker
    display->AdjustOriginInViewport(0.0F, (hudWinY[BORESIGHT_CROSS_WINDOW] +
                                           hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));

    if ( not g_bRealisticAvionics)
        DrawDesignateMarker(Square, FCC->groundDesignateAz, FCC->groundDesignateEl, FCC->groundDesignateDroll);
    else //JPG 13 Aug 04 - the "other" DTOS/TDMarker box doesn't yield the proper TLL (since it's method is really for AA, so let's use the DrawDesignateMarker
        /*DrawTDMarker*/DrawDesignateMarker(Square, FCC->groundDesignateAz, FCC->groundDesignateEl, FCC->groundDesignateDroll /*0.03F*/);

    display->AdjustOriginInViewport(0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
                                            hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));

    // If we have designated and are waiting for release, provide steering to the release point
    if (FCC->airGroundDelayTime > 0.0F)
    {
        // If we've designated a target, provide steering
        DrawSteeringToRelease();
    }
    else
    {
        if (g_bRealisticAvionics)
        {
            TimeToSteerpoint();
            RangeToSteerpoint();
        }
    }
}


void HudClass::DrawLADD(void)
{

    //Draw our heading at the bottom of the HUD
    headingPos = High;

    // Draw a TD box if we have a locked target
    // NOTE:  In reality there would never be a locked target since the radar would be in AGR, but
    // if we allow a radar lock, we might as well allow a TD box...
    DrawTDBox();

    // Draw the impact point marker
    display->AdjustOriginInViewport(0.0F, (hudWinY[BORESIGHT_CROSS_WINDOW] +
                                           hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
    DrawDesignateMarker(Square, FCC->groundDesignateAz, FCC->groundDesignateEl, FCC->groundDesignateDroll);
    display->AdjustOriginInViewport(0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
                                            hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));

    // If we have designated and are waiting for release, provide steering to the release point
    if (FCC->airGroundDelayTime > 0.0F)
        DrawSteeringToReleaseLADD();
}


void HudClass::DrawTargetingPod(void)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    // Draw the TD box
    display->AdjustOriginInViewport(0.0F, (hudWinY[BORESIGHT_CROSS_WINDOW] +
                                           hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
    DrawDesignateMarker(Square, FCC->groundDesignateAz, FCC->groundDesignateEl, FCC->groundDesignateDroll);
    display->AdjustOriginInViewport(0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
                                            hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));

    // Provide steering to the release point
    DrawSteeringToRelease();

    //MI Laser Indicator
    if (g_bRealisticAvionics)
    {
        LaserPodClass* laserPod = (LaserPodClass*)FindLaserPod(playerAC);

        if (laserPod and FCC->LaserArm)
        {
            //armed and fired
            if (FCC->LaserFire)
            {
                if (flash)
                    display->TextLeft(-0.35F, -0.12F, "L");
            }
            //armed, put it there steady
            else if (FCC->LaserArm)
                display->TextLeft(-0.35F, -0.12F, "L");
        }
    }
}


void HudClass::DrawSteeringToRelease(void)
{
    //mlTrig drollTrig;
    float vOffset;
    float x;
    char tmpStr[32];
    float steeringLineX, solutionCueY;
    float puacY, fpmY;
    int min, sec;
    float slantRange;
    //TJL 12/04/03 Added latRange
    float latRange = 0.0f;
    //me123 tofextra is the extra tof for the last bomb(this is the bomb the puac symbolice) becourse it has to wait for the preciding bombs to drop
    //float  TofExtra = ( (FCC->Sms->RippleInterval())* ((FCC->Sms->RippleCount()+1))/ // MLR 4/3/2004 -
    float  TofExtra = ((FCC->Sms->GetAGBRippleInterval()) * ((FCC->Sms->GetAGBRippleCount() + 1)) /
                       ((float) sqrt(ownship->XDelta() * ownship->XDelta() + ownship->YDelta() * ownship->YDelta())));

    //if (FCC->Sms->Pair() == TRUE)
    if (FCC->Sms->GetAGBPair() == TRUE) // MLR 4/3/2004 -
    {
        TofExtra *= 0.5f;
    }

    // RV - Biker - Check if we're a JSOW
    bool isJSOW = false;

    if (FCC->Sms->CurHardpoint() > 0)
    {
        BombClass* theBomb = (BombClass *)FCC->Sms->hardPoint[FCC->Sms->CurHardpoint()]->weaponPointer.get();

        if (theBomb and theBomb->EntityType()->classInfo_[VU_STYPE] == STYPE_BOMB_JSOW)
            isJSOW = true;
    }

    // Steering Line
    if (FCC->inRange and not isJSOW)
    {
        steeringLineX = FCC->airGroundBearing / (20.0F * DTR);
        steeringLineX += betaHudUnits;
        steeringLineX = min(max(steeringLineX , -1.0F), 1.0F);
        display->Line(steeringLineX, 1.0F, steeringLineX, -1.0F);

        // Flight path marker position
        fpmY = (hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F -
                alphaHudUnits);

        // Solution Cue
        if (FCC->tossAnticipationCue not_eq FireControlComputer::NoCue)
        {
            if (FCC->tossAnticipationCue == FireControlComputer::PullUp or
                FCC->tossAnticipationCue == FireControlComputer::AwaitingRelease)
                solutionCueY = min(FCC->airGroundDelayTime / 60.0F, 1.0F);
            else
                solutionCueY = min(FCC->airGroundDelayTime / 10.0F, 1.0F);

            solutionCueY = fpmY + (1.0F - fpmY) * solutionCueY;
            solutionCueY = min(max(solutionCueY , -1.0F), 1.0F);
            display->Line(steeringLineX - 0.05F, solutionCueY,
                          steeringLineX + 0.05F, solutionCueY);
        }

        // Toss Anticipation Cue
        if (FCC->tossAnticipationCue == FireControlComputer::PreToss or
            (FCC->tossAnticipationCue == FireControlComputer::PullUp and flash))
        {
            display->Circle(0.0F, RadToHudUnits(-3.0F * DTR) +
                            hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F,
                            MRToHudUnits(60.0F));
        }

        // PUAC goes here
        // me123 status ok. lots of changes in this PUAC rutine.
        vOffset = hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F;
        display->AdjustOriginInViewport(0.0F, vOffset);

        if (FCC->groundImpactTime < 25)
        {
            if ((FCC->groundImpactTime - TofExtra) > (FCC->Sms->armingdelay / 100))
            {
                puacY = (FCC->groundImpactTime - TofExtra) / (FCC->Sms->armingdelay / 100); // 1 at minimum release alt


                // draw the puac on the bomb fall line
                //MI vids show it stays below the FPM
                /*if( not g_bRealisticAvionics)
                 x = ((puacY-1) * drollTrig.sin  + betaHudUnits);
                else*/ //Cobra removed (droll not initialized and why do this for non-realistic?)
                x = betaHudUnits;

                //draw the puac
                puacY = - alphaHudUnits - ((puacY / 3) - (0.33F)) ; //this is fpm pos when at minimum release alt.

                display->Line(x - 0.075F, puacY, x + 0.075F, puacY);
                display->Line(x - 0.075F, puacY, x - 0.075F, puacY + 0.025F);
                display->Line(x + 0.075F, puacY, x + 0.075F, puacY + 0.025F);

                // Now we are below arming time so we show the "low" and change the PUAC function
                // so it hit's the fmp when the ground is reached (when we crash)
            }
            else
            {
                // now we use the PUAC to "count down" to ground impact
                puacY =  FCC->groundImpactTime / 4;

                // draw the puac on the bomb fall line
                //  x = (x  + betaHudUnits) * -((puacY-1)/y ) ;//me123 this might crash when the bombline length (y) is zero.
                //MI vids show it stays below the FPM
                /*if( not g_bRealisticAvionics)
                 x = ((puacY-1) * drollTrig.sin  + betaHudUnits);
                else*/ //Cobra removed (droll not initialized and why do this for non-realistic?)
                x = betaHudUnits;
                // draw the Puac
                puacY =  - alphaHudUnits - puacY  ; // fpm pos at ground impact
                display->Line(x - 0.075F, puacY, x + 0.075F, puacY);
                display->Line(x - 0.075F, puacY, x - 0.075F, puacY + 0.025F);
                display->Line(x + 0.075F, puacY, x + 0.075F, puacY + 0.025F);

                display->TextLeft(betaHudUnits + 0.1F, -alphaHudUnits, "LOW");
            }
        }

        // Clear the viewport offset
        display->AdjustOriginInViewport(0.0F, -vOffset);

        // Add Release Angle Scale if we need one
        if (FCC->airGroundMaxRange > 0.0F)
        {
            float lateralRange = (float)sqrt(
                                     (FCC->groundDesignateX - ownship->XPos()) * (FCC->groundDesignateX - ownship->XPos()) +
                                     (FCC->groundDesignateY - ownship->YPos()) * (FCC->groundDesignateY - ownship->YPos()))
                                 * FT_TO_NM;

            sprintf(tmpStr, "%.0f", lateralRange);
            DrawDLZSymbol(lateralRange / (FCC->missileWEZDisplayRange * FT_TO_NM), tmpStr,
                          FCC->airGroundMinRange / (FCC->missileWEZDisplayRange),
                          FCC->airGroundMaxRange / (FCC->missileWEZDisplayRange),
                          0.0f,
                          0.0f, FALSE, "");
        }

        //TJL 12/04/03 We need lateral range here as well. //JAM 07Dec03 - Tom, sqrtf is better
        float latRange = sqrtf(
                             (FCC->groundDesignateX - ownship->XPos()) * (FCC->groundDesignateX - ownship->XPos()) +
                             (FCC->groundDesignateY - ownship->YPos()) * (FCC->groundDesignateY - ownship->YPos()))
                         * FT_TO_NM;

        // Slant range;
        slantRange = sqrtf(
                         (FCC->groundDesignateX - ownship->XPos()) * (FCC->groundDesignateX - ownship->XPos()) +
                         (FCC->groundDesignateY - ownship->YPos()) * (FCC->groundDesignateY - ownship->YPos()) +
                         (FCC->groundDesignateZ - ownship->ZPos()) * (FCC->groundDesignateZ - ownship->ZPos()));

        // RV - I-Hawk - Do not display target range if in HARM HAS mode
        bool displayRange = true;

        if (FCC->GetSubMode() == FireControlComputer::HARM)
        {
            HarmTargetingPod* harmPod = (HarmTargetingPod*)FindSensor(ownship, SensorClass::HTS);

            if (harmPod and harmPod->GetSubMode() == HarmTargetingPod::HAS or
                harmPod and harmPod->GetSubMode() == HarmTargetingPod::Handoff or
                harmPod and harmPod->GetSubMode() == HarmTargetingPod::FilterMode)
            {
                displayRange = false;
            }
        }

        if (displayRange)
        {
            if (slantRange > 1.0F * NM_TO_FT)
            {
                sprintf(tmpStr, "F%4.1f", max(min(100.0F, slantRange * FT_TO_NM), 0.0F));
            }

            else
            {
                sprintf(tmpStr, "F%03.0f", max(min(10000.0F, slantRange * 0.01F), 0.0F));
            }
        }

        ShiAssert(strlen(tmpStr) < sizeof(tmpStr));

        //MI
        if ( not g_bRealisticAvionics)
        {
            DrawWindowString(10, tmpStr);
        }

        else
        {
            if (displayRange)
            {
                display->TextLeft(0.45F, -0.36F, tmpStr);
            }
        }

        // Text Data
        min = (int)(FCC->airGroundDelayTime / 60.0F);
        sec = (int)(FCC->airGroundDelayTime - min * 60.0F);

        // Time to release
        sprintf(tmpStr, "%02d:%02d", min, sec);
        ShiAssert(strlen(tmpStr) < sizeof(tmpStr));

        //MI
        if ( not g_bRealisticAvionics)
        {
            DrawWindowString(13, tmpStr);
        }

        else
        {
            if (displayRange)
            {
                display->TextLeft(0.45F, -0.43F, tmpStr);
            }
        }

        // Range and bearing to tgt
        sec = (int)(FCC->airGroundBearing * RTD * 0.1F);

        if (sec < 0)
            sec = -sec;

        //TJL 12/04/03 Not sure why it's GroundRange here...? Changing to lateralRange defined above
        //sprintf(tmpStr, "%02d %04.1f", sec, FCC->airGroundRange * FT_TO_NM);
        sprintf(tmpStr, "%02d %04.1f", sec, latRange);

        ShiAssert(strlen(tmpStr) < sizeof(tmpStr));

        //MI
        if ( not g_bRealisticAvionics)
            DrawWindowString(14, tmpStr);
        else
            display->TextLeft(0.45F, -0.50F, tmpStr);
    }
    // RV - Biker - Some diff HUD symboligy for JSOWs
    else if (FCC->Sms->JDAMPowered and FCC->Sms->JDAMInitTimer <= 4.0f)
    {
        steeringLineX = FCC->airGroundBearing / (20.0F * DTR);
        steeringLineX += betaHudUnits;
        steeringLineX = min(max(steeringLineX , -1.0F), 1.0F);
        display->Line(steeringLineX, 1.0F, steeringLineX, -1.0F);

        float lateralRange = (float)sqrt(
                                 (FCC->groundDesignateX - ownship->XPos()) * (FCC->groundDesignateX - ownship->XPos()) +
                                 (FCC->groundDesignateY - ownship->YPos()) * (FCC->groundDesignateY - ownship->YPos()))
                             * FT_TO_NM;

        sprintf(tmpStr, "%.0f", lateralRange);
        DrawDLZSymbol(lateralRange / (FCC->missileWEZDisplayRange * FT_TO_NM), tmpStr,
                      FCC->airGroundMinRange / (FCC->missileWEZDisplayRange),
                      FCC->airGroundMaxRange / (FCC->missileWEZDisplayRange),
                      0.0f,
                      0.0f, FALSE, "");
    }
}

void HudClass::DrawRPod(void)
{
    float vOffset;

    // Compute and set the viewport offset to get 0,0 at the boresight cross
    vOffset = hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F;
    //MI
    //display->AdjustOriginInViewport (0.0F, vOffset - 8.0F * degreesForScreen);
    display->AdjustOriginInViewport(0.0F, vOffset + g_fReconCameraOffset * degreesForScreen);


    // Draw the pipper
    display->Circle(0.0F, 0.0F, 2.0F * PIPPER_SIZE);
    display->Point(0.0F, 0.0F);
    display->Line(0.0F,  2.0F * PIPPER_SIZE, 0.0F,  3.0F * PIPPER_SIZE);
    display->Line(0.0F, -2.0F * PIPPER_SIZE, 0.0F, -3.0F * PIPPER_SIZE);
    display->Line(2.0F * PIPPER_SIZE, 0.0F,  3.0F * PIPPER_SIZE, 0.0F);
    display->Line(-2.0F * PIPPER_SIZE, 0.0F, -3.0F * PIPPER_SIZE, 0.0F);

    //MI
    //display->AdjustOriginInViewport (0.0F, -(vOffset - 8.0F * degreesForScreen));
    display->AdjustOriginInViewport(0.0F, -(vOffset + g_fReconCameraOffset * degreesForScreen));

    // Save the current location of the boresight cross (in pixels)
    pixelXCenter = display->viewportXtoPixel(0.0F);
    //MI
    //pixelYCenter = display->viewportYtoPixel (vOffset - 8.0F * degreesForScreen);
    pixelYCenter = display->viewportYtoPixel(-(vOffset + g_fReconCameraOffset * degreesForScreen));
    sightRadius  = display->viewportXtoPixel(2.0F * PIPPER_SIZE) - pixelXCenter;

    //MI
    if (g_bRealisticAvionics)
    {
        // Add waypoint info
        if (waypointValid)
        {
            TimeToSteerpoint();
            RangeToSteerpoint();
        }
    }
}

#include "SimIO.h" // Retro 3Jan2004

void HudClass::DrawMANReticle(void)
{
    //Draw our heading at the bottom of the HUD
    headingPos = High;

    // Retro 3Jan2004 (moved up a bit)
    //How much increase per step?
    float Diff = 140 / MAX_STEPS;
    float CurPos;

    if (IO.AnalogIsUsed(AXIS_RET_DEPR)) // Retro 3Jan2004
    {
        float axisVal = (float)IO.GetAxisValue(AXIS_RET_DEPR) / 15000.F;
        CurPos = axisVal * (-12.F) * Diff;
        RET_CENTER = RET_MIN + axisVal * (RET_MAX - RET_MIN);
    }
    else // another part of that 'else' to this is in MoveRetCenter()
    {
        CurPos = ReticlePosition * Diff;
    } // Retro 3Jan2004


    if (WhichMode == 1)
    {
        //Normal mode
        //Draw the outer circle

        display->Circle(0.0F, RET_CENTER, 2.0F * OUTER_RETICLE_SIZE);
        //Draw the inner circle
        display->Arc(0.0F, RET_CENTER, 2.0F * INNER_RETICLE_SIZE, 330 * DTR, 30 * DTR);
        display->Arc(0.0F, RET_CENTER, 2.0F * INNER_RETICLE_SIZE, 60 * DTR, 120 * DTR);
        display->Arc(0.0F, RET_CENTER, 2.0F * INNER_RETICLE_SIZE, 150 * DTR, 210 * DTR);
        display->Arc(0.0F, RET_CENTER, 2.0F * INNER_RETICLE_SIZE, 240 * DTR, 300 * DTR);
        //Draw the dot
        display->Point(0.0F, RET_CENTER);
    }
    else if (WhichMode == 2)
    {
        static float angles[] = {1 * DTR, 5 * DTR, 10 * DTR, 15 * DTR, 20 * DTR, 25 * DTR, 30 * DTR, 35 * DTR, 40 * DTR, 45 * DTR, 50 * DTR, 55 * DTR, 60 * DTR, 65 * DTR, 70 * DTR, 75 * DTR, 80 * DTR, 85 * DTR, 90 * DTR};
        static float angles1[] = { -30 * DTR, -25 * DTR, -20 * DTR, -15 * DTR, -10 * DTR, -5 * DTR, 1 * DTR, 5 * DTR, 10 * DTR, 15 * DTR, 20 * DTR, 25 * DTR, 30 * DTR};
        static const int nangles = sizeof(angles) / sizeof(angles[0]);
        static const int mangles = sizeof(angles1) / sizeof(angles1[0]);

        //for reference
        display->Point(0.0F, RET_CENTER + OUTER_RETICLE_SIZE * 2);
        display->Point(0.0F, RET_CENTER - OUTER_RETICLE_SIZE * 2);
        display->Point(OUTER_RETICLE_SIZE * 2, RET_CENTER);
        display->Point(-OUTER_RETICLE_SIZE * 2, RET_CENTER);
        mlTrig trig;

        //outer reticle
        for (int i = 0; i <= nangles; i++)
        {
            mlSinCos(&trig, angles[i]);
            display->Point(0.0F + (OUTER_RETICLE_SIZE * 2 * trig.cos), RET_CENTER + (OUTER_RETICLE_SIZE * 2 * trig.sin));
            display->Point(0.0F - (OUTER_RETICLE_SIZE * 2 * trig.cos), RET_CENTER - (OUTER_RETICLE_SIZE * 2 * trig.sin));
            display->Point(0.0F + (OUTER_RETICLE_SIZE * 2 * trig.cos), RET_CENTER - (OUTER_RETICLE_SIZE * 2 * trig.sin));
            display->Point(0.0F - (OUTER_RETICLE_SIZE * 2 * trig.cos), RET_CENTER + (OUTER_RETICLE_SIZE * 2 * trig.sin));
        }

        //inner reticle
        for (int j = 0; j <= mangles; j++)
        {
            mlSinCos(&trig, angles1[j]);
            display->Point(0.0F + (INNER_RETICLE_SIZE * 2 * trig.cos), RET_CENTER + (INNER_RETICLE_SIZE * 2 * trig.sin));
            display->Point(0.0F - (INNER_RETICLE_SIZE * 2 * trig.cos), RET_CENTER - (INNER_RETICLE_SIZE * 2 * trig.sin));
            display->Point(0.0F + (INNER_RETICLE_SIZE * 2 * trig.sin), RET_CENTER - (INNER_RETICLE_SIZE * 2 * trig.cos));
            display->Point(0.0F - (INNER_RETICLE_SIZE * 2 * trig.sin), RET_CENTER + (INNER_RETICLE_SIZE * 2 * trig.cos));
        }

        //Draw the cross
        //display->Point(0.0F, RETICLE_CENTER);
        display->Line(-0.02F, RET_CENTER, 0.02F, RET_CENTER);
        display->Line(0.0F, RET_CENTER + 0.02F, 0.0F, RET_CENTER - 0.02F);
        //draw the lines
        display->Line(0.0F, RET_CENTER + OUTER_RETICLE_SIZE * 2, 0.0F, RET_CENTER + OUTER_RETICLE_SIZE * 2 + TICK_LEN);
        display->Line(0.0F, RET_CENTER - OUTER_RETICLE_SIZE * 2, 0.0F, RET_CENTER - OUTER_RETICLE_SIZE * 2 - TICK_LEN);
        display->Line(0.0F + OUTER_RETICLE_SIZE * 2, RET_CENTER , 0.0F + OUTER_RETICLE_SIZE * 2 + TICK_LEN, RET_CENTER);
        display->Line(0.0F - OUTER_RETICLE_SIZE * 2, RET_CENTER , 0.0F - OUTER_RETICLE_SIZE * 2 - TICK_LEN, RET_CENTER);
    }

    CurPos *= -1;
    char tempstr[10];

    if (CurPos < 10)
        sprintf(tempstr, "00%1.0f", CurPos);
    else if (CurPos < 100)
        sprintf(tempstr, "0%2.0f", CurPos);
    else
        sprintf(tempstr, "%3.0f", CurPos);

    display->TextLeft(-0.7F, -0.05F, tempstr);
}
void HudClass::DrawSteeringToReleaseLADD(void)
{
    char tmpStr[32];
    float steeringLineX, solutionCueY;
    float puacY, fpmY;
    int min, sec;
    float slantRange;
    //me123 tofextra is the extra tof for the last bomb(this is the bomb the puac symbolice) becourse it has to wait for the preciding bombs to drop
    //float  TofExtra = ( (FCC->Sms->RippleInterval())* ((FCC->Sms->RippleCount()+1))/
    float  TofExtra = ((FCC->Sms->GetAGBRippleInterval()) * ((FCC->Sms->GetAGBRippleCount() + 1)) /

                       ((float) sqrt(ownship->XDelta() * ownship->XDelta() + ownship->YDelta() * ownship->YDelta())));

    //if (FCC->Sms->Pair() == TRUE) // MLR 4/3/2004 -
    if (FCC->Sms->GetAGBPair() == TRUE)
    {
        TofExtra *= 0.5f;
    }

    // Steering Line
    if (FCC->inRange)
    {
        steeringLineX = FCC->airGroundBearing / (20.0F * DTR);
        steeringLineX += betaHudUnits;
        steeringLineX = min(max(steeringLineX , -1.0F), 1.0F);
        display->Line(steeringLineX, 1.0F, steeringLineX, -1.0F);

        // Flight path marker position
        fpmY = (hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F -
                alphaHudUnits);

        // Solution Cue
        if (FCC->laddAnticipationCue not_eq FireControlComputer::NoLADDCue)
        {
            if (FCC->laddAnticipationCue == FireControlComputer::LADDPullUp or
                FCC->laddAnticipationCue == FireControlComputer::LADDAwaitingRelease)
                solutionCueY = min(FCC->airGroundDelayTime / 60.0F, 1.0F);
            else
                solutionCueY = min(FCC->airGroundDelayTime / 10.0F, 1.0F);

            solutionCueY = fpmY + (1.0F - fpmY) * solutionCueY;
            solutionCueY = min(max(solutionCueY , -1.0F), 1.0F);
            display->Line(steeringLineX - 0.05F, solutionCueY,
                          steeringLineX + 0.05F, solutionCueY);
        }


        // PUAC goes here
        // me123 status ok. lots of changes in this PUAC rutine.
        if (FCC->groundImpactTime < 25)
        {
            if ((FCC->groundImpactTime - TofExtra) > (FCC->Sms->armingdelay / 100))
            {
                puacY = (FCC->groundImpactTime - TofExtra) / (FCC->Sms->armingdelay / 100); // 1 at minimum release alt

                // Position between the FPM and the bottom of the HUD
                puacY = fpmY - puacY + 1; //this is fpm pos when at minimum release alt.
                display->Line(steeringLineX - 0.075F, puacY, steeringLineX + 0.075F, puacY);
                display->Line(steeringLineX - 0.075F, puacY, steeringLineX - 0.075F, puacY + 0.025F);
                display->Line(steeringLineX + 0.075F, puacY, steeringLineX + 0.075F, puacY + 0.025F);

            }
            else
            {
                // Now we are below arming time so we show the "low" and change the PUAC function
                // so it hit's the fmp when the ground is reached (when we crash)

                // now we use the PUAC to "count down" to ground impact
                puacY =  FCC->groundImpactTime / 3 ;

                puacY = fpmY - puacY;
                display->Line(steeringLineX - 0.075F, puacY, steeringLineX + 0.075F, puacY);
                display->Line(steeringLineX - 0.075F, puacY, steeringLineX - 0.075F, puacY + 0.025F);
                display->Line(steeringLineX + 0.075F, puacY, steeringLineX + 0.075F, puacY + 0.025F);

                display->TextLeft(betaHudUnits + 0.1F, fpmY, "LOW");
            }
        }

        // Slant range;
        slantRange = (float)sqrt(
                         (FCC->groundDesignateX - ownship->XPos()) * (FCC->groundDesignateX - ownship->XPos()) +
                         (FCC->groundDesignateY - ownship->YPos()) * (FCC->groundDesignateY - ownship->YPos()) +
                         (FCC->groundDesignateZ - ownship->ZPos()) * (FCC->groundDesignateZ - ownship->ZPos()));

        if (slantRange > 1.0F * NM_TO_FT)
            sprintf(tmpStr, "F %4.1f", max(min(100.0F, slantRange * FT_TO_NM), 0.0F));
        else
            sprintf(tmpStr, "F %03.0f", max(min(10000.0F, slantRange * 0.01F), 0.0F));

        ShiAssert(strlen(tmpStr) < sizeof(tmpStr));
        DrawWindowString(10, tmpStr);

        // Text Data
        min = (int)(FCC->airGroundDelayTime / 60.0F);
        sec = (int)(FCC->airGroundDelayTime - min * 60.0F);

        // Time to pull up
        sprintf(tmpStr, "%02d:%02d", min, sec);
        ShiAssert(strlen(tmpStr) < sizeof(tmpStr));
        DrawWindowString(13, tmpStr);

        // Range and bearing to tgt
        sec = (int)(FCC->airGroundBearing * RTD * 0.1F);

        if (sec < 0)
            sec = -sec;

        //sec += 36;
        sprintf(tmpStr, "%04.1f %02d", FCC->airGroundRange * FT_TO_NM, sec);
        ShiAssert(strlen(tmpStr) < sizeof(tmpStr));
        DrawWindowString(14, tmpStr);
    }
}
void HudClass::MoveRetCenter(void)
{
    if (IO.AnalogIsUsed(AXIS_RET_DEPR) == true) // Retro 3Jan2004, doing this in another place (DrawMANReticle()) an analogue
        return;

    RET_CENTER = RET_CENTER + (RetPos * 0.1F);
    RetPos = 0;
    RET_CENTER = max(min(RET_CENTER, RET_MIN), RET_MAX);
}
