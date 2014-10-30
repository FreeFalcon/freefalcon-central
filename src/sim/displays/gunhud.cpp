#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "stdhdr.h"
#include "aircrft.h"
#include "simmath.h"
#include "camp2sim.h"
#include "hud.h"
#include "fcc.h"
#include "simveh.h"
#include "guns.h"
#include "object.h"
#include "airframe.h"
#include "Graphics/Include/display.h"
#include "icp.h"
#include "otwdrive.h"
#include "cpmanager.h"

//static const float DefaultTargetSpan = 35.0f; MI now done in constructor
extern bool g_bRealisticAvionics;

void HudClass::DrawGuns(void)
{
    char tmpStr[24];
    int tmpMode;

    // If the HUD is drawing (ie: the player is here), we'd assume we have a gun on this airplane...
    ShiAssert(ownship);
    ShiAssert(ownship->Guns);

    // Don't do anything until we have some history
    //if (SimLibFrameCount < 2 * SimLibMajorFrameRate)
    // return;

    // 2001-04-09 ADDED  BY S.G. IF WE HAVE NO GUNS ONBOARD, DON'T DO GUNS DISPLAY STUFF
    // RV - Biker - AC without guns should also have a reticle and target location line
    if ( not ownship->Guns)
    {
        //return;
        tmpMode = FireControlComputer::EEGS;
    }
    else
    {
        FlyBullets();
        tmpMode = ownship->FCC->GetSubMode();
    }

    // Slant Range and closure
    if (targetPtr)
    {
        sprintf(tmpStr, "%.0f", max(min(10000.0F, -targetData->rangedot * FTPSEC_TO_KNOTS), -10000.0F));

        //ShiAssert (strlen(tmpStr) < 24);
        if ( not g_bRealisticAvionics)
            DrawWindowString(13, tmpStr);
        else
            display->TextLeft(0.45F, -0.43F, tmpStr);

        if (targetData->range > 1.0F * NM_TO_FT)
            sprintf(tmpStr, "F %4.1f", max(min(100.0F, targetData->range * FT_TO_NM), 0.0F));
        else
            sprintf(tmpStr, "F %03.0f", max(min(10000.0F, targetData->range * 0.01F), 0.0F));
    }
    else
        sprintf(tmpStr, "M  015");

    if ( not g_bRealisticAvionics)
        DrawWindowString(10, tmpStr);
    else
        display->TextLeft(0.45F, -0.36F, tmpStr);

    ShiAssert(strlen(tmpStr) < 24);

    //MI
    if (g_bRealisticAvionics)
        DefaultTargetSpan = OTWDriver.pCockpitManager->mpIcp->ManWSpan;
    else
        DefaultTargetSpan = 35.0F;

    // ASSOCIATOR 03/12/03: Added the combined SnapShot LCOS Gunmode SSLC
    // RV - Biker - switch for tmpMode now
    switch (tmpMode)
    {
        case FireControlComputer::EEGS:
            DrawEEGS();
            break;

        case FireControlComputer::SSLC:
            DrawSSLC();
            break;

        case FireControlComputer::LCOS:
            DrawLCOS();
            break;

        case FireControlComputer::Snapshot:
            DrawSnapshot();
            break;
    }

    //MI make the funnel dissapear after we pull the trigger
    if (g_bRealisticAvionics)
    {
        if (targetPtr)
        {
            if ((ownship->fireGun or ownship->GunFire) and ownship->Sms->MasterArm() not_eq SMSBaseClass::Safe)
            {
                if ( not HideFunnel and not SetHideTimer)
                {
                    HideFunnelTimer = SimLibElapsedTime + 150;
                    SetHideTimer = TRUE;
                }

                if ( not ownship->OnGround())
                {
                    if ( not HideFunnel)
                    {
                        if (SimLibElapsedTime >= HideFunnelTimer)
                        {
                            HideFunnel = TRUE;
                            SetShowTimer = FALSE;
                        }
                    }
                }
            }
            else
            {
                //Make it appear again 1 second after we released the trigger
                if (SetHideTimer and not SetShowTimer)
                {
                    SetHideTimer = FALSE;
                    SetShowTimer = TRUE;
                    ShowFunnelTimer = SimLibElapsedTime + 1000;
                }

                if (HideFunnel)
                {
                    if (SimLibElapsedTime >= ShowFunnelTimer)
                    {
                        HideFunnel = FALSE;
                        SetShowTimer = FALSE;
                    }
                }
            }
        }
    }
}

void HudClass::DrawEEGS(void)
{
    // RV - Biker - Only draw funnel if we have guns
    if (ownship->Guns)
    {
        DrawFunnel();
    }

    if (targetPtr == NULL)
    {
        //MI this IS there in reality...
        //if ( not g_bRealisticAvionics) // JPO not real I don't think.
        DrawMRGS();
    }
    else
    {
        if (targetData->range < 2.0F * NM_TO_FT)
        {
            DrawTSymbol();
        }

        DrawTDCircle();
    }
}



void HudClass::FlyBullets(void)
{
    int i;
    SIM_LONG dt;
    float tf;

    int before, after;
    float interp;

    float dx, dy, dz;
    float rx, ry, rz;

    // Fly out a bunch of bullets
    for (i = 0; i < NumEEGSSegments; i++)
    {
        dt = EEGSTimePerSegment * (i + 1); // ms
        tf = dt * 0.001F; // seconds

        // Get the interpolation parameters for the required time
        interp = EEGShistory(dt, &before, &after);

        // find bullet's relative position
        dx = EEGSvalueVX(interp, before, after) * tf + EEGSvalueX(interp, before, after) - ownship->XPos();
        dy = EEGSvalueVY(interp, before, after) * tf + EEGSvalueY(interp, before, after) - ownship->YPos();
        dz = EEGSvalueVZ(interp, before, after) * tf + EEGSvalueZ(interp, before, after) - ownship->ZPos();

        // Gravity Drop
        dz += GRAVITY * 0.5F * tf * tf;

        // Rotate the bullet's relative position into body space
        rx = ownship->dmx[0][0] * dx + ownship->dmx[0][1] * dy + ownship->dmx[0][2] * dz;
        ry = ownship->dmx[1][0] * dx + ownship->dmx[1][1] * dy + ownship->dmx[1][2] * dz;
        rz = ownship->dmx[2][0] * dx + ownship->dmx[2][1] * dy + ownship->dmx[2][2] * dz;

        // Store the HUD space projection of the bullet's position and it's range
        bulletH[i] = RadToHudUnitsX((float)atan2(ry, rx));
        bulletV[i] = RadToHudUnitsY((float)atan(-rz / (float)sqrt(rx * rx + ry * ry + .1f)));
        bulletRange[i] = (float)sqrt(rx * rx + ry * ry + rz * rz);
    }
}


void HudClass::DrawFunnel(void)
{
    int i, iprev;
    float radius;
    float dx, dy;

    // Calculate funnel shape
    iprev = 1;
    // ShiAssert( NumEEGSSegments > 1 );

    for (i = 0; i < NumEEGSSegments; i++)
    {
        // Decide how wide the funnel should be at this range
        // First in radians, then in HUD viewport space units.
        radius = (float)atan2(DefaultTargetSpan * 0.5f, bulletRange[i]);
        radius = RadToHudUnits(radius);

#if 1 // It turns out that the F16 doesn't really rotate the width vector, so we won't either
        dx = -radius;
        dy =  0.0f;
#else
        // Get a vector perpendicular to line connecting this bullet and its neighbor
        dx =  bulletV[i] - bulletV[iprev];
        dy = -bulletH[i] + bulletH[iprev];
        float scale = dx * dx + dy * dy;

        if (scale < 0.0001f)
        {
            dx = -radius;
            dy =  0.0f;
        }
        else
        {
            if (i not_eq 0)
            {
                scale = radius / sqrt(scale);
            }
            else
            {
                scale = -radius / sqrt(scale);
            }

            dx *= scale;
            dy *= scale;
        }

#endif

        iprev = i;

#if 0
        // Instantanious
        funnel1X[i] = bulletH[i] + dx;
        funnel1Y[i] = bulletV[i] + dy;
        funnel2X[i] = bulletH[i] - dx;
        funnel2Y[i] = bulletV[i] - dy;
#else
        // IIR filtered
        static const float TC = 0.5f; // Seconds...
        float m, im;

        if (SimLibMajorFrameTime < TC)
        {
            m  = SimLibMajorFrameTime / TC;
            im = 1.0f - m;
            funnel1X[i] = (bulletH[i] + dx) * m + funnel1X[i] * im;
            funnel1Y[i] = (bulletV[i] + dy) * m + funnel1Y[i] * im;
            funnel2X[i] = (bulletH[i] - dx) * m + funnel2X[i] * im;
            funnel2Y[i] = (bulletV[i] - dy) * m + funnel2Y[i] * im;
        }
        else
        {
            funnel1X[i] = bulletH[i] + dx;
            funnel1Y[i] = bulletV[i] + dy;
            funnel2X[i] = bulletH[i] - dx;
            funnel2Y[i] = bulletV[i] - dy;
        }

#endif
    }

    display->AdjustOriginInViewport(0.0F, hudWinY[BORESIGHT_CROSS_WINDOW] +
                                    hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F);

    // Draw the funnel (limited at 2 sec time of flight)
    static const int stopIdx = 2000 / EEGSTimePerSegment;

    // ShiAssert( stopIdx < NumEEGSSegments );
    for (i = stopIdx; i > 0; i--)
    {
        //MI make it dissapearn
        if (g_bRealisticAvionics)
        {
            if ( not HideFunnel)
            {
                display->Line(funnel1X[i], funnel1Y[i], funnel1X[i - 1], funnel1Y[i - 1]);
                display->Line(funnel2X[i], funnel2Y[i], funnel2X[i - 1], funnel2Y[i - 1]);
            }
        }
        else
        {
            display->Line(funnel1X[i], funnel1Y[i], funnel1X[i - 1], funnel1Y[i - 1]);
            display->Line(funnel2X[i], funnel2Y[i], funnel2X[i - 1], funnel2Y[i - 1]);
        }

        if (g_bRealisticAvionics)
        {
            if (HideFunnel and targetPtr)
            {
                DrawBATR();
            }
        }

        //MI in SIM we want FEDS, but only if no locked target
        if (g_bRealisticAvionics and not targetPtr)
        {
            //document shows that we get this even when the gun is fired for real.
            if ( not ownship->OnGround() /* and (ownship->Sms->MasterArm() == SMSBaseClass::Sim)*/)
            {
                if (ownship->fireGun and ownship->Sms->FEDS)
                    FlyFEDSBullets(TRUE);
                else if (ownship->Sms->FEDS)
                    FlyFEDSBullets(FALSE);
            }
        }
    }

    display->AdjustOriginInViewport(0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
                                            hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
}

void HudClass::DrawMRGS(void)
{
    int i;
    float angle, lineSpace;
    mlTrig trig;

    // Each line is supposed to represent a target which is flying in a
    // direction that, if it continued on path, would be hit by rounds fired now.
    // In other words, each line represents a lead angle of the same magnitude but for a
    // target moving in a different plane with respect to ours.  Since range is unknown
    // in level III (no locked target),  I _presume_ the lines "breath" to represent
    // the same information for different ranges.  SCR 9-16-98

    // Decide what percent of full spread to draw the angled lines
    // (For now we're just drawing arbitrary lines)
    // NOTE:  0xFFF = 4095 or 4.095 seconds since sim time is in milliseconds.
    lineSpace = (float)fabs(1.0f - (float)(SimLibElapsedTime bitand 0xFFF) / 0x7FF);
    angle = (2.0f - lineSpace) * DTR * 3.0F;

    for (i = 0; i < 4; i++)
    {
        mlSinCos(&trig, angle);
        display->Line(0.75F * trig.sin, -0.75F * trig.cos,  0.65F * trig.sin, -0.65F * trig.cos);
        display->Line(-0.75F * trig.sin, -0.75F * trig.cos, -0.65F * trig.sin, -0.65F * trig.cos);
        angle *= 2.0f;
    }

    // Draw in the center line in the series
    display->Line(0.0f, -0.75f,  0.0f, -0.65f);
}

void HudClass::DrawTDCircle(void)
{
    float xPos, yPos;
    float extent;
    float rangeTic1X, rangeTic1Y;
    float rangeTic2X, rangeTic2Y;
    mlTrig trig;
    float offset = 0.04F;

    ShiAssert(targetPtr);

    xPos = RadToHudUnitsX(targetData->az);
    yPos = RadToHudUnitsY(targetData->el);

    display->AdjustOriginInViewport(0.0F, (hudWinY[BORESIGHT_CROSS_WINDOW] +
                                           hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));

    if (fabs(targetData->az) > 825.0F * DTR or
        fabs(targetData->el) > 825.0F * DTR or
        fabs(xPos) > 0.90F or fabs(yPos + hudWinY[BORESIGHT_CROSS_WINDOW] +
                                   hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F) > 0.825F)
    {
        mlSinCos(&trig, 90.0F * DTR - targetData->droll);
        xPos = MRToHudUnits(45.0F) * trig.cos;
        yPos = MRToHudUnits(45.0F) * trig.sin;
        display->Line(0.0F, 0.0F, xPos, yPos);

        xPos = offset * trig.cos;
        yPos = offset * trig.sin;

        while (fabs(xPos) < 0.825F and fabs(yPos + hudWinY[BORESIGHT_CROSS_WINDOW] +
                                           hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F) < 0.825F)
        {
            offset += 0.02F;
            xPos = offset * trig.cos;
            yPos = offset * trig.sin;
        }

        offset -= 0.02F;
        xPos = offset * trig.cos;
        yPos = offset * trig.sin;
        display->Circle(xPos, yPos, 0.15F);
        display->Line(xPos + 0.05F, yPos + 0.05F, xPos - 0.05F, yPos - 0.05F);
        display->Line(xPos + 0.05F, yPos - 0.05F, xPos - 0.05F, yPos + 0.05F);

        //      sprintf (tmpStr, "%.0f", targetData->ata * RTD);
        //      ShiAssert (strlen(tmpStr) < 12);
        //      display->TextRight(-MRToHudUnits(60.0F), 0.0F, tmpStr);
    }
    else
    {
        extent = min(12000.0F, targetData->range) / 12000.0F * 360.0F * DTR;

        if (extent > 90.0F * DTR)
        {
            display->Arc(xPos, yPos, 0.15F, 0.0F, extent - 90.0F * DTR);
            display->Arc(xPos, yPos, 0.15F, 270.0F * DTR, 360.0F * DTR);
        }
        else
        {
            display->Arc(xPos, yPos, 0.15F, 270.0F * DTR, 270.0F * DTR + extent);
        }

        display->Circle(xPos + 0.175F, yPos, 0.01F);

        if (targetData->range < 12000.0F)
        {
            mlSinCos(&trig, extent);
            rangeTic1X = 0.15F * trig.sin;
            rangeTic1Y = 0.15F * trig.cos;
            rangeTic2X = 0.115F * trig.sin;
            rangeTic2Y = 0.115F * trig.cos;
            display->Line(xPos + rangeTic1X, yPos + rangeTic1Y,
                          xPos + rangeTic2X, yPos + rangeTic2Y);
        }
    }

    display->AdjustOriginInViewport(0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
                                            hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
}


// This symbol is supposed to draw two lines outside the funnel at 1G lead
// and one line inside the funnel at 9G lead for the locked target
void HudClass::DrawTSymbol(void)
{
    float rx, ry, offset, offsetX, offsetY;
    float tf, interp, xPosL, yPosL, xPosR, yPosR;
    float scale;
    int idx;
    int tfms;

    ShiAssert(targetPtr);
    ShiAssert(targetData);

    display->AdjustOriginInViewport(0.0F, hudWinY[BORESIGHT_CROSS_WINDOW] +
                                    hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F);


    // 1G lines outside funnel (assume target pulls 1G directly into us, where should he be to get hit?)
    // What we need to know to do this correctly is:
    // IF ownship continues in steady state, and target continues in steady state PULLING 1G,
    // THEN how long will it take for bullets fired now to reach the targets range.
    //
    // FOW NOW:  Approximate the 1G line position by putting it near targets _current_ range along the funnel

    // How long to fly to the target's range (neglecting gravity)?
    // JMB 010220 CTD
    // tf = targetData->range / ownship->Guns->initBulletVelocity; //-
    if (ownship->Guns) //+
        tf = targetData->range / ownship->Guns->initBulletVelocity; //+
    else//+
        tf = targetData->range / 3000.0F;//+

    // JMB 010220 CTD
    tfms = FloatToInt32(tf * 1000.0f);

    // Which bullet index to use
    idx = tfms / EEGSTimePerSegment - 1;

    if (idx > NumEEGSSegments - 2)
        idx = NumEEGSSegments - 2;

    if (idx < 0)
        idx = 0;

    //me123 status test. multible changes in the draw eegs rutine.
    // interp = (float)((tfms - (idx+1) * EEGSTimePerSegment)) / EEGSTimePerSegment;

    xPosL = funnel1X[1];// + (funnel1X[idx + 1] - funnel1X[idx]) * interp;
    yPosL = funnel1Y[1];// + (funnel1Y[idx + 1] - funnel1Y[idx]) * interp;
    xPosR = funnel2X[1];// + (funnel2X[idx + 1] - funnel2X[idx]) * interp;
    yPosR = funnel2Y[1];// + (funnel2Y[idx + 1] - funnel2Y[idx]) * interp;
    //me123 status test. insert stop

    // Vector across funnel
    //Cobra TJL 10/30/04
    rx = xPosR - xPosL;
    ry = yPosR - yPosL;
    scale = 1.0f / (float)sqrt(rx * rx + ry * ry);
    rx *= scale;
    ry *= scale;

    // 1G lines outside funnel
    if (targetPtr)
    {
        FalconEntity *tgt = targetPtr->BaseData();
        float d, xx, yy;


        float cx = (xPosL + xPosR) * 0.5F;
        float cy = (yPosL + yPosR) * 0.5F;

        float dx =  tgt->XPos() + tgt->XDelta() - Ownship()->XDelta();
        float dy =  tgt->YPos() + tgt->YDelta() - Ownship()->YDelta();
        float dz =  tgt->ZPos() + tgt->ZDelta() - Ownship()->ZDelta();

        float az, el;

        CalcRelAzEl(Ownship(), dx, dy, dz, &az, &el);

        xx = RadToHudUnitsX(az);
        yy = RadToHudUnitsY(el);

        xx -= cx;
        yy -= cy;

        float temp = xx;
        xx = -yy;
        yy = temp;

        d = sqrt(xx * xx + yy * yy);

        if (d)
        {
            xx /= d;
            yy /= d;
        }
        else
        {
            xx = 1.0f;
            yy = 0.0f;
        }

#define RESCALE(in,inmin,inmax,outmin,outmax) ( ((float)(in) - (inmin)) * ((outmax) - (outmin)) / ((inmax) - (inmin)) + (outmin))

        float extent = RESCALE(targetData->range , 1200, 3000, 0, 1);

        if (extent > 1) extent = 1;

        if (extent < 0) extent = 0;

        float insize = .05f;
        float outsize = insize + .2f * extent;

        display->Line(cx + xx * insize, cy + yy * insize, cx + xx * outsize, cy + yy * outsize);
        xx = -xx;
        yy = -yy;
        display->Line(cx + xx * insize, cy + yy * insize, cx + xx  * outsize, cy + yy * outsize);

    }
    else
    {
        //Cobra 10/30/04 TJL
        /*rx = xPosR - xPosL;
        ry = yPosR - yPosL;
        scale = 1.0f / (float)sqrt( rx*rx + ry*ry );
        rx *= scale;
        ry *= scale;*/

        display->Line(xPosR , yPosR , xPosR + rx * 0.15f, yPosR + ry * 0.15f);
        display->Line(xPosL , yPosL , xPosL - rx * 0.15f, yPosL - ry * 0.15f);
    }

    display->AdjustOriginInViewport((xPosL + xPosR) * 0.5F, (yPosL + yPosR) * 0.5F);

    // 1G plus sign inside funnel
    display->Line(0.0F, -0.025F, 0.0F, 0.025F);
    display->Line(-0.025F, 0.0F,  0.025F, 0.0F);

    display->AdjustOriginInViewport(-(xPosL + xPosR) * 0.5F, -(yPosL + yPosR) * 0.5F);

    // How to interpolate/extrapolate between the two bullet records
    interp = (float)((tfms - (idx + 1) * EEGSTimePerSegment)) / EEGSTimePerSegment;

    xPosL = funnel1X[idx] + (funnel1X[idx + 1] - funnel1X[idx]) * interp;
    yPosL = funnel1Y[idx] + (funnel1Y[idx + 1] - funnel1Y[idx]) * interp;
    xPosR = funnel2X[idx] + (funnel2X[idx + 1] - funnel2X[idx]) * interp;
    yPosR = funnel2Y[idx] + (funnel2Y[idx + 1] - funnel2Y[idx]) * interp;
    display->AdjustOriginInViewport((xPosL + xPosR) * 0.5F, (yPosL + yPosR) * 0.5F);

    display->Circle(0.0F, 0.0F, 0.012F);  //me123 status test. draw batr


    // Positioning for 9G minus sign (assume target pulls 9G directly into us, where should he be to get hit?)
    //
    // FOR NOW: This is a rough approximation of how aspect affects the relative position of the 9G line.
    offset = -(targetData->ataFrom - (90.0F * DTR)) / (90.0F * DTR);
    offsetX = -offset * ry;
    offsetY = offset * rx;

    // 9G minus sign inside funnel
    display->Line(offsetX - rx * 0.025F, offsetY - ry * 0.025F,
                  offsetX + rx * 0.025F, offsetY + ry * 0.025F);

    // Restore the original viewport origin
    display->AdjustOriginInViewport(-(xPosL + xPosR) * 0.5F, -(yPosL + yPosR) * 0.5F);
    display->AdjustOriginInViewport(0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
                                            hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
}


// ASSOCIATOR 03/12/03: Added the combined SnapShot LCOS Gunmode SSLC
void HudClass::DrawSSLC(void)
{
    float tf, range, interp;
    float xPos, yPos; // The HUD space location of the hypothetical bullet in flight
    int tfms, idx;
    float radius;

    if (targetPtr)
        DrawLCOSForSSLC();

    display->AdjustOriginInViewport(0.0F, (hudWinY[BORESIGHT_CROSS_WINDOW] +
                                           hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));

    // Continuously Computed Impact Line
    static const float tickWidth = MRToHudUnits(5.0F);

    static const int idx1 =  500 / EEGSTimePerSegment;
    static const int idx2 = 1000 / EEGSTimePerSegment;
    static const int idx3 = 1500 / EEGSTimePerSegment;
    // ShiAssert( idx3 < NumEEGSSegments );

    display->Line(0.0F, 0.0F, bulletH[idx1], bulletV[idx1]);
    display->Line(bulletH[idx1] - tickWidth, bulletV[idx1], bulletH[idx1] + tickWidth, bulletV[idx1]);

    display->Line(bulletH[idx1], bulletV[idx1], bulletH[idx2], bulletV[idx2]);
    display->Line(bulletH[idx2] - tickWidth, bulletV[idx2], bulletH[idx2] + tickWidth, bulletV[idx2]);

    display->Line(bulletH[idx2], bulletV[idx2], bulletH[idx3], bulletV[idx3]);
    display->Line(bulletH[idx3] - tickWidth, bulletV[idx3], bulletH[idx3] + tickWidth, bulletV[idx3]);

    // Pipper, 1 TOF in the future
    if (targetPtr)
        range = min(targetData->range, 9000.0f);
    else
        range = 1500.0F;

    // How long to fly to the chosen range (neglecting gravity)?
    ShiAssert(FALSE == F4IsBadReadPtr(ownship->Guns, sizeof * ownship->Guns)); // JPO
    tf = range / ownship->Guns->initBulletVelocity;
    tfms = FloatToInt32(tf * 1000.0f);

    // Which bullet index to use
    idx = tfms / EEGSTimePerSegment - 1;

    if (idx > NumEEGSSegments - 2)
        idx = NumEEGSSegments - 2;

    if (idx < 0)
        idx = 0;

    // How to interpolate/extrapolate between the two bullet records
    interp = (float)((tfms - (idx + 1) * EEGSTimePerSegment)) / EEGSTimePerSegment;

    // Draw the range pipper
    xPos = bulletH[idx] + (bulletH[idx + 1] - bulletH[idx]) * interp;
    yPos = bulletV[idx] + (bulletV[idx + 1] - bulletV[idx]) * interp;
    display->Circle(xPos, yPos, tickWidth);

    // If we DON'T have a locked target, draw a cirle showing default wing span at default range (1500.0f)
    if (targetPtr == NULL)
    {
        // Decide how big the default target would be at the default range
        // First in radians, then in HUD viewport space units.
        radius = (float)atan2(DefaultTargetSpan * 0.5f, 1500.0f);
        radius = RadToHudUnits(radius);
        display->Circle(xPos, yPos, radius);
    }


    // Put the viewport origin back where it was
    display->AdjustOriginInViewport(0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
                                            hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
}

// ASSOCIATOR 03/12/03: Helper function for DrawSSLC method
void HudClass::DrawLCOSForSSLC(void)
{
    float angle, rangeTic1X, rangeTic1Y, rangeTic2X, rangeTic2Y;
    mlTrig trig;
    float hPos, vPos; // The HUD space location of the hypothetical bullet after a 1 second flight


    static const SIM_LONG dt = 1000;
    hPos = bulletH[dt / EEGSTimePerSegment];
    vPos = bulletV[dt / EEGSTimePerSegment];

    // Should be smoothed to account for granularity of eegs data storage...
    display->AdjustOriginInViewport(hPos, (hudWinY[BORESIGHT_CROSS_WINDOW] +
                                           hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F + vPos));



    if (targetPtr)
    {
        // Data Circle
        display->Circle(0.0F, 0.0F, 0.2F);
        angle = (float)atan2(-vPos, -hPos);
        mlSinCos(&trig, angle);

        //display->Line (0.2F * trig.cos, 0.2F * trig.sin, -hPos, -vPos);
        //display->Line (-0.2F * trig.cos, -0.2F * trig.sin, -0.4F * trig.cos, -0.4F * trig.sin);
        if (targetData->range < 12000.0F)
        {
            // Range
            angle = 90.0F * DTR - targetData->range / 12000.0F * 360.0F * DTR;
            //MI
            display->Arc(0.0F, 0.0F, 0.2F, 0.0F, 360.0F * DTR - angle);
            mlSinCos(&trig, angle);
            rangeTic1X = 0.2F * trig.cos;
            rangeTic1Y = 0.2F * trig.sin;
            rangeTic2X = 0.175F * trig.cos;
            rangeTic2Y = 0.175F * trig.sin;
            display->Line(rangeTic1X, rangeTic1Y, rangeTic2X, rangeTic2Y);
        }

        // Closure
        angle = 90.0F * DTR - (max(min(-targetData->rangedot * 0.01F,
                                       5.0F), -5.0F) * 0.2F) * 150.0F * DTR;
        mlSinCos(&trig, angle);
        rangeTic1X = 0.2F * trig.cos;
        rangeTic1Y = 0.2F * trig.sin;
        mlSinCos(&trig, angle + 5.0F * DTR);
        rangeTic2X = 0.175F * trig.cos;
        rangeTic2Y = 0.175F * trig.sin;
        display->Line(rangeTic1X, rangeTic1Y, rangeTic2X, rangeTic2Y);
        mlSinCos(&trig, angle - 5.0F * DTR);
        rangeTic2X = 0.175F * trig.cos;
        rangeTic2Y = 0.175F * trig.sin;
        display->Line(rangeTic1X, rangeTic1Y, rangeTic2X, rangeTic2Y);
    }
    else
        display->Circle(0.0F, 0.0F, 0.2F);

    // Gun Pipper
    display->Circle(0.0F, 0.0F, MRToHudUnits(10.0F));

    // Settling Cue
    if (fabs(lastPipperX - hPos) > MRToHudUnits(10.0F) or
        fabs(lastPipperY - vPos) > MRToHudUnits(10.0F))
    {
        display->Line(-(lastPipperX - hPos) * 2.0F,
                      -(lastPipperY - vPos) * 2.0F,
                      0.0F, 0.0F);
    }

    lastPipperX = hPos;
    lastPipperY = vPos;

    display->AdjustOriginInViewport(-hPos,
                                    -(hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F + vPos)
                                   );
}


void HudClass::DrawLCOS(void)
{
    float angle, rangeTic1X, rangeTic1Y, rangeTic2X, rangeTic2Y;
    mlTrig trig;
    float hPos, vPos; // The HUD space location of the hypothetical bullet after a 1 second flight


    static const SIM_LONG dt = 1000;
    hPos = bulletH[dt / EEGSTimePerSegment];
    vPos = bulletV[dt / EEGSTimePerSegment];

    // Should be smoothed to account for granularity of eegs data storage...
    display->AdjustOriginInViewport(hPos, (hudWinY[BORESIGHT_CROSS_WINDOW] +
                                           hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F + vPos));

    // Data Circle
    //MI fix for LCOS... why would we want a full circle here?
    //ASSOCIATOR because the real F-16 has a full circle
    display->Circle(0.0F, 0.0F, 0.2F);
    angle = (float)atan2(-vPos, -hPos);
    mlSinCos(&trig, angle);
    display->Line(0.2F * trig.cos, 0.2F * trig.sin, -hPos, -vPos);
    display->Line(-0.2F * trig.cos, -0.2F * trig.sin, -0.4F * trig.cos, -0.4F * trig.sin);

    if (targetPtr)
    {
        if (targetData->range < 12000.0F)
        {
            // Range
            angle = 90.0F * DTR - targetData->range / 12000.0F * 360.0F * DTR;
            //MI
            display->Arc(0.0F, 0.0F, 0.2F, 0.0F, 360.0F * DTR - angle);
            mlSinCos(&trig, angle);
            rangeTic1X = 0.2F * trig.cos;
            rangeTic1Y = 0.2F * trig.sin;
            rangeTic2X = 0.175F * trig.cos;
            rangeTic2Y = 0.175F * trig.sin;
            display->Line(rangeTic1X, rangeTic1Y, rangeTic2X, rangeTic2Y);
        }

        // Closure
        angle = 90.0F * DTR - (max(min(-targetData->rangedot * 0.01F,
                                       5.0F), -5.0F) * 0.2F) * 150.0F * DTR;
        mlSinCos(&trig, angle);
        rangeTic1X = 0.2F * trig.cos;
        rangeTic1Y = 0.2F * trig.sin;
        mlSinCos(&trig, angle + 5.0F * DTR);
        rangeTic2X = 0.175F * trig.cos;
        rangeTic2Y = 0.175F * trig.sin;
        display->Line(rangeTic1X, rangeTic1Y, rangeTic2X, rangeTic2Y);
        mlSinCos(&trig, angle - 5.0F * DTR);
        rangeTic2X = 0.175F * trig.cos;
        rangeTic2Y = 0.175F * trig.sin;
        display->Line(rangeTic1X, rangeTic1Y, rangeTic2X, rangeTic2Y);
    }
    else
        display->Circle(0.0F, 0.0F, 0.2F);

    // Gun Pipper
    display->Circle(0.0F, 0.0F, MRToHudUnits(10.0F));

    // Settling Cue
    if (fabs(lastPipperX - hPos) > MRToHudUnits(10.0F) or
        fabs(lastPipperY - vPos) > MRToHudUnits(10.0F))
    {
        display->Line(-(lastPipperX - hPos) * 2.0F,
                      -(lastPipperY - vPos) * 2.0F,
                      0.0F, 0.0F);
    }

    lastPipperX = hPos;
    lastPipperY = vPos;

    display->AdjustOriginInViewport(-hPos,
                                    -(hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F + vPos)
                                   );
}

void HudClass::DrawSnapshot(void)
{
    float tf, range, interp;
    float xPos, yPos; // The HUD space location of the hypothetical bullet in flight
    int tfms, idx;
    float radius;

    display->AdjustOriginInViewport(0.0F, (hudWinY[BORESIGHT_CROSS_WINDOW] +
                                           hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));

    // Continuously Computed Impact Line
    static const float tickWidth = MRToHudUnits(5.0F);

    static const int idx1 =  500 / EEGSTimePerSegment;
    static const int idx2 = 1000 / EEGSTimePerSegment;
    static const int idx3 = 1500 / EEGSTimePerSegment;
    // ShiAssert( idx3 < NumEEGSSegments );

    display->Line(0.0F, 0.0F, bulletH[idx1], bulletV[idx1]);
    display->Line(bulletH[idx1] - tickWidth, bulletV[idx1], bulletH[idx1] + tickWidth, bulletV[idx1]);

    display->Line(bulletH[idx1], bulletV[idx1], bulletH[idx2], bulletV[idx2]);
    display->Line(bulletH[idx2] - tickWidth, bulletV[idx2], bulletH[idx2] + tickWidth, bulletV[idx2]);

    display->Line(bulletH[idx2], bulletV[idx2], bulletH[idx3], bulletV[idx3]);
    display->Line(bulletH[idx3] - tickWidth, bulletV[idx3], bulletH[idx3] + tickWidth, bulletV[idx3]);

    // Pipper, 1 TOF in the future
    if (targetPtr)
        range = min(targetData->range, 9000.0f);
    else
        range = 1500.0F;

    // How long to fly to the chosen range (neglecting gravity)?
    ShiAssert(FALSE == F4IsBadReadPtr(ownship->Guns, sizeof * ownship->Guns)); // JPO
    tf = range / ownship->Guns->initBulletVelocity;
    tfms = FloatToInt32(tf * 1000.0f);

    // Which bullet index to use
    idx = tfms / EEGSTimePerSegment - 1;

    if (idx > NumEEGSSegments - 2)
        idx = NumEEGSSegments - 2;

    if (idx < 0)
        idx = 0;

    // How to interpolate/extrapolate between the two bullet records
    interp = (float)((tfms - (idx + 1) * EEGSTimePerSegment)) / EEGSTimePerSegment;

    // Draw the range pipper
    xPos = bulletH[idx] + (bulletH[idx + 1] - bulletH[idx]) * interp;
    yPos = bulletV[idx] + (bulletV[idx + 1] - bulletV[idx]) * interp;
    display->Circle(xPos, yPos, tickWidth);

    // If we DON'T have a locked target, draw a cirle showing default wing span at default range (1500.0f)
    if (targetPtr == NULL)
    {
        // Decide how big the default target would be at the default range
        // First in radians, then in HUD viewport space units.
        radius = (float)atan2(DefaultTargetSpan * 0.5f, 1500.0f);
        radius = RadToHudUnits(radius);
        display->Circle(xPos, yPos, radius);
    }


    // Put the viewport origin back where it was
    display->AdjustOriginInViewport(0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
                                            hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
}


// Here we compute and store the initial position and velocity of a bullet
// fired with the provided parameters
void HudClass::SetEEGSData(float x, float y, float z,
                           float gamma, float sigma,
                           float theta, float psi, float vt)
{
    mlTrig trigGamma, trigSigma, trigTheta, trigPsi;
    float dx, dy, dz, initVel;

    // If the HUD is drawing (ie: the player is here), we'd assume we have a gun on this airplane...
    ShiAssert(ownship);
    ShiAssert(ownship->Guns);


    // See if we should update the same record or advance to a new one
    if (SimLibElapsedTime - lastEEGSstepTime >= EEGSUpdateTime)
    {
        // Advance to the next slot in our circular buffer
        if (eegsFrameNum < NumEEGSFrames - 1)
            eegsFrameNum++;
        else
            eegsFrameNum = 0;

        lastEEGSstepTime = SimLibElapsedTime;
    }

    /*-------------*/
    /* Sample Time */
    /*-------------*/
    eegsFrameArray[eegsFrameNum].time = SimLibElapsedTime;

    /*-----------------*/
    /* Firing Position */
    /*-----------------*/
    eegsFrameArray[eegsFrameNum].x = x;
    eegsFrameArray[eegsFrameNum].y = y;
    eegsFrameArray[eegsFrameNum].z = z;

    /*-------------*/
    /* Initial Vel */
    /*-------------*/
    mlSinCos(&trigGamma, gamma);
    mlSinCos(&trigSigma, sigma);
    mlSinCos(&trigTheta, theta);
    mlSinCos(&trigPsi,   psi);

    // Aircraft Vel
    dx =  vt * trigGamma.cos * trigSigma.cos;
    dy =  vt * trigGamma.cos * trigSigma.sin;
    dz = -vt * trigGamma.sin;

    // Muzzle Vel (cheaper to use the dmx matrix for this, but we'll leave it for now)
    if (ownship->Guns)
        initVel = ownship->Guns->initBulletVelocity;
    else
        initVel = 3000.0F;

    dx += initVel * trigTheta.cos * trigPsi.cos;
    dy += initVel * trigTheta.cos * trigPsi.sin;
    dz -= initVel * trigTheta.sin;

    eegsFrameArray[eegsFrameNum].vx = dx;
    eegsFrameArray[eegsFrameNum].vy = dy;
    eegsFrameArray[eegsFrameNum].vz = dz;
}


// Time interpolation from sampled EEGS data to the specified time ago
float HudClass::EEGShistory(SIM_LONG dt, int *beforeIndex, int *afterIndex)
{
    SIM_LONG time;
    float t;
    int i;
    int before = -1;
    int after;

    // Convert from ms ago to a sim time
    time = SimLibElapsedTime - dt;

    // Search from newest back to oldest
    for (i = eegsFrameNum; i >= 0; i--)
    {
        if (eegsFrameArray[i].time < time)
        {
            before = i;
            break;
        }
    }

    if (before == -1)
    {
        for (i = NumEEGSFrames - 1; i > eegsFrameNum; i--)
        {
            if (eegsFrameArray[i].time < time)
            {
                before = i;
                break;
            }
        }
    }

    // Handle no data case
    if (before == -1)
    {
        if (eegsFrameNum < NumEEGSFrames - 1)
            before = eegsFrameNum + 1;
        else
            before = 0;
    }

    // Find the time just after the "before" case
    if (before < NumEEGSFrames - 1)
        after = before + 1;
    else
        after = 0;

    *beforeIndex = before;
    *afterIndex = after;

    // Now compute the percentage from after toward before that "time" is
    t = (eegsFrameArray[after].time - time) /
        (float)(eegsFrameArray[after].time - eegsFrameArray[before].time);

    return t;
}
//MI
void HudClass::FlyFEDSBullets(bool NewBullets)
{
    int i;
    SIM_LONG dt;
    float tf;

    int before, after;
    float interp;

    float dx, dy, dz;
    float lastX, lastY, lastZ;
    float rx, ry, rz;

    // Fly out a bunch of bullets
    for (i = 0; i < NumEEGSSegments / 4; i++)
    {
        dt = EEGSTimePerSegment * (i + 1); // ms
        tf = dt * 0.001F; // seconds

        // Get the interpolation parameters for the required time
        interp = EEGShistory(dt, &before, &after);

        // find bullet's relative position
        if (NewBullets)
        {
            dx = EEGSvalueVX(interp, before, after) * tf + EEGSvalueX(interp, before, after) - ownship->XPos();
            dy = EEGSvalueVY(interp, before, after) * tf + EEGSvalueY(interp, before, after) - ownship->YPos();
            dz = EEGSvalueVZ(interp, before, after) * tf + EEGSvalueZ(interp, before, after) - ownship->ZPos();
            lastX = dx;
            lastY = dy;
            lastZ = dz;
        }

        // Gravity Drop
        dz += GRAVITY * 0.5F * tf * tf;

        if (NewBullets)
        {
            // Rotate the bullet's relative position into body space
            rx = ownship->dmx[0][0] * dx + ownship->dmx[0][1] * dy + ownship->dmx[0][2] * dz;
            ry = ownship->dmx[1][0] * dx + ownship->dmx[1][1] * dy + ownship->dmx[1][2] * dz;
            rz = ownship->dmx[2][0] * dx + ownship->dmx[2][1] * dy + ownship->dmx[2][2] * dz;
        }

        /*else
        {
         //TODO
         //Make this so the bullets fly based on their last position
         rx = ownship->dmx[0][0]*lastX + ownship->dmx[0][1]*lastY + ownship->dmx[0][2]*dz;
         ry = ownship->dmx[1][0]*lastX + ownship->dmx[1][1]*lastY + ownship->dmx[1][2]*dz;
         rz = ownship->dmx[2][0]*lastX + ownship->dmx[2][1]*lastY + ownship->dmx[2][2]*dz;
        }*/

        // Store the HUD space projection of the bullet's position and it's range
        bulletH[i] = RadToHudUnitsX((float)atan2(ry, rx));
        bulletV[i] = RadToHudUnitsY((float)atan(-rz / (float)sqrt(rx * rx + ry * ry + 0.1f)));

        if (NewBullets)
        {
            display->Circle(bulletH[i] + funnel1X[i], bulletV[i] + funnel1Y[i], 0.003F);
            display->Circle(bulletH[i] + funnel2X[i], bulletV[i] + funnel2Y[i], 0.003F);
        }

        //display->Circle(funnel1X[i], funnel1Y[i], 0.005F);
    }
}

void HudClass::DrawBATR(void)
{
    float tf, range, interp;
    float xPos, yPos; // The HUD space location of the hypothetical bullet in flight
    int tfms, idx;
    //float radius;

    //display->AdjustOriginInViewport (0.0F, (hudWinY[BORESIGHT_CROSS_WINDOW] +
    // hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));

    // Continuously Computed Impact Line
    static const float tickWidth = MRToHudUnits(5.0F);

    static const int idx1 =  500 / EEGSTimePerSegment;
    static const int idx2 = 1000 / EEGSTimePerSegment;
    static const int idx3 = 1500 / EEGSTimePerSegment;

    // ShiAssert( idx3 < NumEEGSSegments );
    /*
     display->Line( 0.0F, 0.0F, bulletH[idx1], bulletV[idx1]);
     display->Line( bulletH[idx1] - tickWidth, bulletV[idx1], bulletH[idx1] + tickWidth, bulletV[idx1]);

     display->Line( bulletH[idx1], bulletV[idx1], bulletH[idx2], bulletV[idx2]);
     display->Line( bulletH[idx2] - tickWidth, bulletV[idx2], bulletH[idx2] + tickWidth, bulletV[idx2]);

     display->Line( bulletH[idx2], bulletV[idx2], bulletH[idx3], bulletV[idx3]);
     display->Line( bulletH[idx3] - tickWidth, bulletV[idx3], bulletH[idx3] + tickWidth, bulletV[idx3]);
    */
    // Pipper, 1 TOF in the future
    if (targetPtr)
        range = min(targetData->range, 9000.0f);

    /*else
     range = 1500.0F; */

    // How long to fly to the chosen range (neglecting gravity)?
    ShiAssert(FALSE == F4IsBadReadPtr(ownship->Guns, sizeof * ownship->Guns)); // JPO
    tf = range / ownship->Guns->initBulletVelocity;
    tfms = FloatToInt32(tf * 1000.0f);

    // Which bullet index to use
    idx = tfms / EEGSTimePerSegment - 1;

    if (idx > NumEEGSSegments - 2)
        idx = NumEEGSSegments - 2;

    if (idx < 0)
        idx = 0;

    // How to interpolate/extrapolate between the two bullet records
    interp = (float)((tfms - (idx + 1) * EEGSTimePerSegment)) / EEGSTimePerSegment;

    // Draw the range pipper
    xPos = bulletH[idx] + (bulletH[idx + 1] - bulletH[idx]) * interp;
    yPos = bulletV[idx] + (bulletV[idx + 1] - bulletV[idx]) * interp;
    display->Circle(xPos, yPos, 0.002F);   // Inner dot
    display->Circle(xPos, yPos, 0.022F);   //0.012F

    // If we DON'T have a locked target, draw a cirle showing default wing span at default range (1500.0f)
    /* if( targetPtr == NULL )
    {
     // Decide how big the default target would be at the default range
     // First in radians, then in HUD viewport space units.
     radius = (float)atan2( DefaultTargetSpan*0.5f, 1500.0f );
     radius = RadToHudUnits(radius);
     display->Circle (xPos, yPos, radius);
    } */


    // Put the viewport origin back where it was
    //display->AdjustOriginInViewport (0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
    // hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
}
