/***************************************************************************\
    OTWsky.cpp
    Scott Randolph
    January 2, 1995

 Erick Jap
 October 30, 1996

    This class provides 3D drawing functions specific to rendering out the
 window views including terrain.

 This file contains the implementations of the sky drawing functions
\***************************************************************************/
//JAM 30Sep03 - Begin Major Rewrite
#include <cISO646>
#include <math.h>
#include "grmath.h"
#include "grinline.h"
#include "StateStack.h"
#include "TimeMgr.h"
#include "TMap.h"
#include "Tpost.h"
#include "TOD.h"
#include "Tex.h"
#include "draw2d.h"
#include "RViewPnt.h"
#include "RenderOW.h"
#include "FalcLib/include/dispopts.h" //JAM 04Oct03
#include "RenderTV.h"

//JAM 18Nov03
#include "RealWeather.h"

#define FLAT_FILLER

// Distances are used in place of sizes -- bigger distance gives smaller apparent size
const float RenderOTW::MOON_DIST = 40.0f;
const float RenderOTW::SUN_DIST = 30.0f;
const float RenderOTW::MOST_SUN_GLARE_DIST = 12.0f;
const float RenderOTW::MIN_SUN_GLARE = 0.0f;
const float RenderOTW::ROOF_REPEAT_COUNT = 6.0f;
const float RenderOTW::HAZE_ALTITUDE_FACTOR = (1.0f / (SKY_MAX_HEIGHT - SKY_ROOF_HEIGHT));
const float RenderOTW::GLARE_FACTOR = (4096.0f / SKY_MAX_HEIGHT);

Texture RenderOTW::texRoofTop;
Texture RenderOTW::texRoofBottom;

extern int g_nGfxFix;


/***************************************************************************\
    Draw the sky  ( Assumes square pixels )
\***************************************************************************/
void RenderOTW::SetRoofMode(BOOL state)
{
    // Don't bother if the roof state isn't changing
    if (state == skyRoof)
    {
        return;
    }

    // Get the textures we'll need if the roof is being turned on
    if (state and not texRoofTop.TexHandle())
    {
        Tcolor light;

        texRoofTop.LoadAndCreate("OVClayerT.gif", MPR_TI_PALETTE);
        texRoofBottom.LoadAndCreate("OVClayerB.gif", MPR_TI_PALETTE);

        TheTimeOfDay.GetTextureLightingColor(&light);
        Palette *palette;
        palette = texRoofTop.GetPalette();
        palette->LightTexturePalette(&light);
        //texRoofTop.palette->LightTexturePalette( &light );
        palette = texRoofBottom.GetPalette();
        palette->LightTexturePalette(&light);
        //texRoofBottom.palette->LightTexturePalette( &light );
    }

    // Store the new state
    skyRoof = state;
}

BOOL RenderOTW::GetRoofMode()
{
    return skyRoof;
}

/***************************************************************************\
    Draw the sky  ( Assumes square pixels )
\***************************************************************************/
BOOL RenderOTW::DrawSky(void)
{
    // Update the sky color based on our current attitude and position
    AdjustSkyColor();


    if ( not skyRoof)
    {
        DrawSkyNoRoof();
        return TRUE; // Need to draw terrain
    }


    if (viewpoint->Z() < -SKY_ROOF_HEIGHT)
    {
        DrawSkyAbove();
        return FALSE; // Don't need to draw terrain
    }
    else
    {
        DrawSkyBelow();
        return TRUE; // Need to draw terrain
    }
}


/***************************************************************************\
    Draw the sky  ( Assumes square pixels )
\***************************************************************************/
void RenderOTW::DrawSkyNoRoof(void)
{
    HorizonRecord horizon;

    double angleOfDepression, percentHalfXscale;
    float pixelWidth, pixelDistance;
    float vpZ = -viewpoint->Z();

    float bandAngleUp = min(PI / 18.0f, (PI / 48.0f) * (SKY_MAX_HEIGHT / vpZ));

    float top = Pitch() + diagonal_half_angle;
    float bottom = Pitch() - diagonal_half_angle;



#ifdef TWO_D_MAP_AVAILABLE

    if (twoDmode)
    {
        ClearFrame();
        return;
    }

#endif


    // Figure out how for away from the center of the display the edge of the
    // terrain data is.
    angleOfDepression = atan2(vpZ, viewpoint->GetDrawingRange());

    pixelWidth = (float)sqrt(scaleX * scaleX + scaleY * scaleY);


    // Decide which portions of the sky can possibly be seen by the viewer in this orientation
    BOOL canSeeAboveTop = top > bandAngleUp;
    BOOL canSeeAboveHorizon = top > 0.0f;
    BOOL canSeeAboveTerrain = top > -angleOfDepression;
    BOOL canSeeBelowClear = bottom < bandAngleUp;
    BOOL canSeeBelowHorizon = bottom < 0.0f;

    BOOL drawFiller = canSeeAboveTerrain and canSeeBelowHorizon;
    BOOL drawTop = canSeeAboveHorizon and canSeeBelowClear;
    BOOL drawClear = canSeeAboveTop;


    // Compute two points on the horizon which are sure to be off opposite edges of the screen
    float cR = (float)cos(Roll());
    float sR = (float)sin(Roll());
    horizon.hx = pixelWidth *  cR;
    horizon.hy = pixelWidth * -sR;

    // Compute the position of the real horizon line
    // NOTE:  tan becomes infinite at pitch = +/- 90 degrees.
    //        We'll ignore the issue for now since it is a rare occurence.
    percentHalfXscale = tan(Pitch()) * oneOVERtanHFOV;
    pixelDistance = scaleX * (float)percentHalfXscale;
    horizon.vx = pixelDistance * sR;
    horizon.vy = pixelDistance * cR;


    // Compute the position of the top of the horizon/sky blending band
    // (the band extends "bandAngleUp" radians above the horizon)
    percentHalfXscale = tan(Pitch() - bandAngleUp) * oneOVERtanHFOV;
    pixelDistance = scaleX * (float)percentHalfXscale;
    horizon.vxUp = pixelDistance * sR;
    horizon.vyUp = pixelDistance * cR;
    horizon.bandAngleUp = bandAngleUp;


    // Compute the position of the bottom of the terrain to horizon filler band
    percentHalfXscale = Pitch() + angleOfDepression;

    if (percentHalfXscale < PI_OVER_2)
    {
        percentHalfXscale = tan(Pitch() + angleOfDepression) * oneOVERtanHFOV;
    }
    else
    {
        percentHalfXscale = 10.0f; // any big number should do...
    }

    pixelDistance = scaleX * (float)percentHalfXscale;
    horizon.vxDn = pixelDistance * sR;
    horizon.vyDn = pixelDistance * cR;

    // Do sunrise/sunset horizon calculations
    ComputeHorizonEffect(&horizon);

    // Clear that part of the screen which will not be covered by sky or terrain
    if (drawClear)
    {
        DrawClearSky(&horizon);
    }

    // Draw the blended poly from the haze color to the sky color
    // sfr: this is between horizon and roof
    if (drawTop)
    {
        DrawSkyHazeBand(&horizon);
    }

    // Draw the poly of low intensity haze color to fill from the terrain to the horizon
    // sfr: this is between sky and terrain
    if (drawFiller)
    {
        DrawFillerToHorizon(&horizon);
    }

    // Draw the celestial objects
    DrawStars();

    if (TheTimeOfDay.ThereIsASun())
    {
        DrawSun();
    }

    if (TheTimeOfDay.ThereIsAMoon())
    {
        DrawMoon();
    }
}



/***************************************************************************\
    Draw the sky  ( Assumes square pixels )
\***************************************************************************/
void RenderOTW::DrawSkyBelow(void)
{
    ThreeDVertex v0, v1, v2, v3;
    TwoDVertex *vertPointers[4] = { &v0, &v1, &v2, &v3 };

    double angleOfInclination, angleOfDepression;
    double percentHalfXscale;
    float pixelWidth, pixelDistance;
    float vpAlt = -viewpoint->Z();

    float top = Pitch() + diagonal_half_angle;
    float bottom = Pitch() - diagonal_half_angle;

    float u, v;

    HorizonRecord horizon;

#ifdef TWO_D_MAP_AVAILABLE

    if (twoDmode)
    {
        ClearFrame();
        return;
    }

#endif


    // Figure out how for away from the center of the display the edge of the
    // terrain data is.
    angleOfInclination = atan2(SKY_ROOF_HEIGHT - vpAlt, SKY_ROOF_RANGE);
    angleOfDepression = atan2(vpAlt, viewpoint->GetDrawingRange());

    pixelWidth = (float)sqrt(scaleX * scaleX + scaleY * scaleY);


    // Decide which portions of the sky can possibly be seen by the viewer in this orientation
    BOOL canSeeAboveTop = top > angleOfInclination;
    BOOL canSeeAboveHorizon = top > 0.0f;
    BOOL canSeeAboveTerrain = top > -angleOfDepression;
    BOOL canSeeBelowClear = bottom < angleOfInclination;
    BOOL canSeeBelowHorizon = bottom < 0.0f;

    BOOL drawFiller = canSeeAboveTerrain and canSeeBelowHorizon;
    BOOL drawTop = canSeeAboveHorizon and canSeeBelowClear;
    BOOL drawClear = canSeeAboveTop;


    // Compute two points on the horizon which are sure to be off opposite edges of the screen
    float cR = (float)cos(Roll());
    float sR = (float)sin(Roll());
    horizon.hx = pixelWidth *  cR;
    horizon.hy = pixelWidth * -sR;

    // Compute the position of the real horizon line
    // NOTE:  tan becomes infinite at pitch = +/- 90 degrees.
    //        We'll ignore the issue for now since it is a rare occurence.
    percentHalfXscale = tan(Pitch()) * oneOVERtanHFOV;
    pixelDistance = scaleX * (float)percentHalfXscale;
    horizon.vx = pixelDistance * sR;
    horizon.vy = pixelDistance * cR;


    // Compute the position of the top of the horizon/sky blending band
    // (the band extends "angleOfInclination" radians above the horizon)
    percentHalfXscale = tan(Pitch() - angleOfInclination) * oneOVERtanHFOV;
    pixelDistance = scaleX * (float)percentHalfXscale;
    horizon.vxUp = pixelDistance * sR;
    horizon.vyUp = pixelDistance * cR;


    // Compute the position of the bottom of the terrain to horizon filler band
    percentHalfXscale = tan(Pitch() + angleOfDepression) * oneOVERtanHFOV;
    pixelDistance = scaleX * (float)percentHalfXscale;
    horizon.vxDn = pixelDistance * sR;
    horizon.vyDn = pixelDistance * cR;


    // Clear that part of the screen which will not be covered by sky or terrain
    if (drawClear)
    {
        Tpoint worldSpace;

        worldSpace.z = -SKY_ROOF_HEIGHT;
        u = (float)fmod(viewpoint->Y() * 0.5f * ROOF_REPEAT_COUNT / SKY_ROOF_RANGE, 1.0f);
        v = 1.0f - (float)fmod(viewpoint->X() * 0.5f * ROOF_REPEAT_COUNT / SKY_ROOF_RANGE, 1.0f);

        // South West
        worldSpace.x = viewpoint->X() - SKY_ROOF_RANGE,  worldSpace.y = viewpoint->Y() - SKY_ROOF_RANGE;
        TransformPoint(&worldSpace, &v0);
        v0.u = u, v0.v = v + ROOF_REPEAT_COUNT, v0.q = v0.csZ * Q_SCALE;

        // North West
        worldSpace.x = viewpoint->X() + SKY_ROOF_RANGE,  worldSpace.y = viewpoint->Y() - SKY_ROOF_RANGE;
        TransformPoint(&worldSpace, &v1);
        v1.u = u, v1.v = v, v1.q = v1.csZ * Q_SCALE;

        // North East
        worldSpace.x = viewpoint->X() + SKY_ROOF_RANGE,  worldSpace.y = viewpoint->Y() + SKY_ROOF_RANGE;
        TransformPoint(&worldSpace, &v2);
        v2.u = u + ROOF_REPEAT_COUNT, v2.v = v, v2.q = v2.csZ * Q_SCALE;

        // South East
        worldSpace.x = viewpoint->X() - SKY_ROOF_RANGE,  worldSpace.y = viewpoint->Y() + SKY_ROOF_RANGE;
        TransformPoint(&worldSpace, &v3);
        v3.u = u + ROOF_REPEAT_COUNT, v3.v = v + ROOF_REPEAT_COUNT, v3.q = v3.csZ * Q_SCALE;

        v0.r = v1.r = v2.r = v3.r = 0.5f;
        v0.g = v1.g = v2.g = v3.g = 0.6f;
        v0.b = v1.b = v2.b = v3.b = 0.7f;

        // Setup the drawing state for these polygons
        context.RestoreState(STATE_TEXTURE_PERSPECTIVE);
        context.SelectTexture1(texRoofBottom.TexHandle());
#if 0

        if (GetFilteringMode())
        {
            context.SetState(MPR_STA_ENABLES, MPR_SE_FILTERING);
            context.SetState(MPR_STA_TEX_FILTER, MPR_TX_BILINEAR);
            context.InvalidateState();
        }

#endif

        DrawSquare(&v0, &v1, &v2, &v3, CULL_ALLOW_ALL);
    }


    // Draw the hazey sky band
    if (drawTop)
    {
        v0.x = shiftX + horizon.hx + horizon.vx; // horizon right
        v0.y = shiftY + horizon.hy + horizon.vy;
        v1.x = shiftX - horizon.hx + horizon.vx; // horizon left
        v1.y = shiftY - horizon.hy + horizon.vy;
        v2.x = shiftX - horizon.hx + horizon.vxUp; // upper left
        v2.y = shiftY - horizon.hy + horizon.vyUp;
        v3.x = shiftX + horizon.hx + horizon.vxUp; // upper right
        v3.y = shiftY + horizon.hy + horizon.vyUp;

        v0.r = v1.r = haze_sky_color.r;
        v0.g = v1.g = haze_sky_color.g;
        v0.b = v1.b = haze_sky_color.b;
        v0.a = v1.a = 1.0f;

        v2.r = v3.r = sky_color.r;
        v2.g = v3.g = sky_color.g;
        v2.b = v3.b = sky_color.b;
        v2.a = v3.a = 1.0f;

        // Set the clip flags on the constructed verts
        SetClipFlags(&v0);
        SetClipFlags(&v1);
        SetClipFlags(&v2);
        SetClipFlags(&v3);

        // Clip and draw the smooth shaded horizon polygon
        context.RestoreState(STATE_GOURAUD);
        /* if (dithered) {
         context.SetState( MPR_STA_ENABLES, MPR_SE_DITHERING );
         context.InvalidateState();
         }*/
        ClipAndDraw2DFan(&vertPointers[0], 4);
    }


    // Draw the celestial objects
    if (TheTimeOfDay.ThereIsASun()) DrawSun();

    if (TheTimeOfDay.ThereIsAMoon()) DrawMoon();


    // Draw the poly of low intensity haze color to fill from the terrain to the horizon
    if (drawFiller)
    {

        v0.x = shiftX + horizon.hx + horizon.vxDn; // lower right
        v0.y = shiftY + horizon.hy + horizon.vyDn;
        v0.r = haze_ground_color.r;
        v0.g = haze_ground_color.g;
        v0.b = haze_ground_color.b;
        v1.x = shiftX - horizon.hx + horizon.vxDn; // lower left
        v1.y = shiftY - horizon.hy + horizon.vyDn;
        v1.r = haze_ground_color.r;
        v1.g = haze_ground_color.g;
        v1.b = haze_ground_color.b;

        v2.x = shiftX - horizon.hx + horizon.vx; // horizon left
        v2.y = shiftY - horizon.hy + horizon.vy;
        v3.x = shiftX + horizon.hx + horizon.vx; // horizon right
        v3.y = shiftY + horizon.hy + horizon.vy;

        v2.r = earth_end_color.r;
        v2.g = earth_end_color.g;
        v2.b = earth_end_color.b;
        v3.r = earth_end_color.r;
        v3.g = earth_end_color.g;
        v3.b = earth_end_color.b;

        // Set the clip flags on the constructed verts
        SetClipFlags(&v0);
        SetClipFlags(&v1);
        SetClipFlags(&v2);
        SetClipFlags(&v3);

        // Clip and draw the smooth shaded horizon polygon
        context.RestoreState(STATE_GOURAUD);
        /* if (dithered) {
         context.SetState( MPR_STA_ENABLES, MPR_SE_DITHERING );
         context.InvalidateState();
         }*/
        ClipAndDraw2DFan(&vertPointers[0], 4);
    }
}


/***************************************************************************\
    Draw the sky  ( Assumes square pixels )
\***************************************************************************/
void RenderOTW::DrawSkyAbove(void)
{
    double angleOfDepression, percentHalfXscale;
    float pixelWidth, pixelDistance;
    float vpAlt = -viewpoint->Z();

    float bandAngleUp = min(PI / 18.0f, (PI / 48.0f) * (SKY_MAX_HEIGHT / vpAlt));

    float top = Pitch() + diagonal_half_angle;
    float bottom = Pitch() - diagonal_half_angle;

    float u, v;

    HorizonRecord horizon;

#ifdef TWO_D_MAP_AVAILABLE

    if (twoDmode)
    {
        ClearFrame();
        return;
    }

#endif


    // Figure out how for away from the center of the display the edge of the
    // roof polygon is.
    angleOfDepression = atan2(vpAlt - SKY_ROOF_HEIGHT, SKY_ROOF_RANGE);
    pixelWidth = (float)sqrt(scaleX * scaleX + scaleY * scaleY);


    // Decide which portions of the sky can possibly be seen by the viewer in this orientation
    BOOL canSeeAboveTop = top > bandAngleUp;
    BOOL canSeeAboveHorizon = top > 0.0f;
    BOOL canSeeAboveClouds = top > -angleOfDepression;
    BOOL canSeeBelowClear = bottom < bandAngleUp;
    BOOL canSeeBelowHorizon = bottom < 0.0f;
    BOOL canSeeCloudLayer = bottom < -angleOfDepression;

    BOOL drawFiller = canSeeAboveClouds and canSeeBelowHorizon;
    BOOL drawTop = canSeeAboveHorizon and canSeeBelowClear;
    BOOL drawClear = canSeeAboveTop;
    BOOL drawClouds = canSeeCloudLayer;


    // Compute two points on the horizon which are sure to be off opposite edges of the screen
    float cR = (float)cos(Roll());
    float sR = (float)sin(Roll());
    horizon.hx = pixelWidth *  cR;
    horizon.hy = pixelWidth * -sR;

    // Compute the position of the real horizon line
    // NOTE:  tan becomes infinite at pitch = +/- 90 degrees.
    //        We'll ignore the issue for now since it is a rare occurence.
    percentHalfXscale = tan(Pitch()) * oneOVERtanHFOV;
    pixelDistance = scaleX * (float)percentHalfXscale;
    horizon.vx = pixelDistance * sR;
    horizon.vy = pixelDistance * cR;


    // Compute the position of the top of the horizon/sky blending band
    // (the band extends "bandAngleUp" radians above the horizon)
    percentHalfXscale = tan(Pitch() - bandAngleUp) * oneOVERtanHFOV;
    pixelDistance = scaleX * (float)percentHalfXscale;
    horizon.vxUp = pixelDistance * sR;
    horizon.vyUp = pixelDistance * cR;
    horizon.bandAngleUp = bandAngleUp;


    // Compute the position of the bottom of the terrain to horizon filler band
    percentHalfXscale = tan(Pitch() + angleOfDepression) * oneOVERtanHFOV;
    pixelDistance = scaleX * (float)percentHalfXscale;
    horizon.vxDn = pixelDistance * sR;
    horizon.vyDn = pixelDistance * cR;


    if (drawClear)
    {
        DrawClearSky(&horizon);
    }

    // Do sunrise/sunset horizon calculations
    ComputeHorizonEffect(&horizon);

    // Draw the blended poly from the haze color to the sky color
    if (drawTop)
    {
        DrawSkyHazeBand(&horizon);
    }

    // Draw the celestial objects
    DrawStars();

    if (TheTimeOfDay.ThereIsASun()) DrawSun();

    if (TheTimeOfDay.ThereIsAMoon()) DrawMoon();

    // Draw the poly of low intensity haze color to fill from the terrain to the horizon
    if (drawFiller)
    {
        DrawFillerToHorizon(&horizon);
    }


    // Draw the overcast layer below us (covers all terrain)
    if (drawClouds)
    {
        Tpoint worldSpace;
        ThreeDVertex v0, v1, v2, v3;

        worldSpace.z = -SKY_ROOF_HEIGHT;
        u = (float)fmod(viewpoint->Y() * 0.5f * ROOF_REPEAT_COUNT / SKY_ROOF_RANGE, 1.0f);
        v = 1.0f - (float)fmod(viewpoint->X() * 0.5f * ROOF_REPEAT_COUNT / SKY_ROOF_RANGE, 1.0f);

        // South West
        worldSpace.x = viewpoint->X() - SKY_ROOF_RANGE,  worldSpace.y = viewpoint->Y() - SKY_ROOF_RANGE;
        TransformPoint(&worldSpace, &v0);
        v0.u = u, v0.v = v + ROOF_REPEAT_COUNT, v0.q = v0.csZ * Q_SCALE;

        // North West
        worldSpace.x = viewpoint->X() + SKY_ROOF_RANGE,  worldSpace.y = viewpoint->Y() - SKY_ROOF_RANGE;
        TransformPoint(&worldSpace, &v1);
        v1.u = u, v1.v = v, v1.q = v1.csZ * Q_SCALE;

        // South East
        worldSpace.x = viewpoint->X() - SKY_ROOF_RANGE,  worldSpace.y = viewpoint->Y() + SKY_ROOF_RANGE;
        TransformPoint(&worldSpace, &v2);
        v2.u = u + ROOF_REPEAT_COUNT, v2.v = v + ROOF_REPEAT_COUNT, v2.q = v2.csZ * Q_SCALE;

        // North East
        worldSpace.x = viewpoint->X() + SKY_ROOF_RANGE,  worldSpace.y = viewpoint->Y() + SKY_ROOF_RANGE;
        TransformPoint(&worldSpace, &v3);
        v3.u = u + ROOF_REPEAT_COUNT, v3.v = v, v3.q = v3.csZ * Q_SCALE;

        v0.r = v1.r = v2.r = v3.r = 0.5f;
        v0.g = v1.g = v2.g = v3.g = 0.6f;
        v0.b = v1.b = v2.b = v3.b = 0.7f;

        // Setup the drawing state for these polygons
        context.RestoreState(STATE_TEXTURE_PERSPECTIVE);

        if (GetFilteringMode())
        {
            // context.SetState( MPR_STA_ENABLES, MPR_SE_FILTERING );
            context.SetState(MPR_STA_TEX_FILTER, MPR_TX_BILINEAR);
            // context.InvalidateState();
        }

        context.SelectTexture1(texRoofTop.TexHandle());

        DrawSquare(&v0, &v1, &v3, &v2, CULL_ALLOW_ALL);
    }
}



/***************************************************************************\
    Draw the completly clear portion of the sky
\***************************************************************************/
void RenderOTW::DrawClearSky(HorizonRecord *pHorizon)
{
    Edge horizonLine;
    BOOL amOut, wasOut, startedOut;
    MPRVtx_t vert[6];
    unsigned short num;

    // Setup a line equation for the horizon in pixel space
    horizonLine.SetupWithVector(shiftX + pHorizon->vxUp, shiftY + pHorizon->vyUp, pHorizon->hx, pHorizon->hy);

    // Now clip the screen rectangle against the line to build the corners of a polygon
    num = 0;

    // First check the upper left corner
    wasOut = startedOut = horizonLine.LeftOf(leftPixel, topPixel);

    if ( not startedOut)
    {
        vert[num].x = leftPixel;
        vert[num].y = topPixel;
        num++;
    }

    // Now check the upper right corner
    amOut = horizonLine.LeftOf(rightPixel, topPixel);

    if (amOut not_eq wasOut)
    {
        // Compute the intesection of the top edge with the horizon and insert it
        vert[num].x = horizonLine.X(topPixel);
        vert[num].y = topPixel;
        num++;
    }

    if ( not amOut)
    {
        vert[num].x = rightPixel;
        vert[num].y = topPixel;
        num++;
    }

    wasOut = amOut;

    // Now check the lower right corner
    amOut = horizonLine.LeftOf(rightPixel, bottomPixel);

    if (amOut not_eq wasOut)
    {
        // Compute the intesection of the right edge with the horizon and insert it
        vert[num].x = rightPixel;
        vert[num].y = horizonLine.Y(rightPixel);
        num++;
    }

    if ( not amOut)
    {
        vert[num].x = rightPixel;
        vert[num].y = bottomPixel;
        num++;
    }

    wasOut = amOut;

    // Now check the lower left corner
    amOut = horizonLine.LeftOf(leftPixel, bottomPixel);

    if (amOut not_eq wasOut)
    {
        // Compute the intesection of the bottom edge with the horizon and insert it
        vert[num].x = horizonLine.X(bottomPixel);
        vert[num].y = bottomPixel;
        num++;
    }

    if ( not amOut)
    {
        vert[num].x = leftPixel;
        vert[num].y = bottomPixel;
        num++;
    }

    wasOut = amOut;

    // Finally, clip the left edge if it crosses the horizon line
    if (wasOut not_eq startedOut)
    {
        // Compute the intesection of the left edge with the horizon and insert it
        vert[num].x = (float)leftPixel;
        vert[num].y = horizonLine.Y((float)leftPixel);
        num++;
    }

    ShiAssert(num <= 5);

    // Draw the polygon if it isn't totally clipped
    if (num >= 3)
    {
        // Setup for flat shaded drawing for the sky clearing polygon
        context.RestoreState(STATE_SOLID);

        // Draw the sky filling polygon
        context.SelectForegroundColor(
            ((FloatToInt32(sky_color.r * 255.9f)) +
             (FloatToInt32(sky_color.g * 255.9f) <<  8) +
             (FloatToInt32(sky_color.b * 255.9f) << 16)) + 0xff000000);

        context.DrawPrimitive(MPR_PRM_TRIFAN, 0, num, &vert[0], sizeof(vert[0]));
    }
}



/***************************************************************************\
    Draw the sky haze from the horizon up to the clear blue
\***************************************************************************/

#define NEW_SKY_HORIZON 1
#if NEW_SKY_HORIZON
void RenderOTW::DrawSkyHazeBand(struct HorizonRecord *pHorizon)
{
    float dr = 0.0F, dg = 0.0F, db = 0.0F;
    const int num = 4;
    TwoDVertex v0, v1, v2, v3;
    TwoDVertex *vertPointers[4];
    // find out if sun is left or right
    bool sunLeft = pHorizon->sunEffectPos.x <= scaleX;

    //REPORT_VALUE("sun X", pHorizon->sunEffectPos.x);

    if (pHorizon->horeffect)
    {
        dr = pHorizon->sunEffectColor.r - haze_sky_color.r;
        dg = pHorizon->sunEffectColor.g - haze_sky_color.g;
        db = pHorizon->sunEffectColor.b - haze_sky_color.b;
    }

    // Build the corners of our horizon polygon
    v0.x = shiftX + pHorizon->hx + pHorizon->vx; // horizon right
    v0.y = shiftY + pHorizon->hy + pHorizon->vy;

    v1.x = shiftX - pHorizon->hx + pHorizon->vx; // horizon left
    v1.y = shiftY - pHorizon->hy + pHorizon->vy;

    v2.x = shiftX - pHorizon->hx + pHorizon->vxUp; // upper left
    v2.y = shiftY - pHorizon->hy + pHorizon->vyUp;

    v3.x = shiftX + pHorizon->hx + pHorizon->vxUp; // upper right
    v3.y = shiftY + pHorizon->hy + pHorizon->vyUp;

    if (pHorizon->horeffect)
    {
        v0.r = haze_sky_color.r + dr * pHorizon->rhazescale;
        v0.g = /*sunLeft ? 0.0f : 1.0f;*/haze_sky_color.g + dg * pHorizon->rhazescale;
        v0.b = haze_sky_color.b + db * pHorizon->rhazescale;

        v1.r = haze_sky_color.r + dr * pHorizon->lhazescale;
        v1.g = /*sunLeft ? 1.0f : 0.0f;*/haze_sky_color.g + dg * pHorizon->lhazescale;
        v1.b = haze_sky_color.b + db * pHorizon->lhazescale;
    }
    else
    {
        v0.r = haze_sky_color.r;
        v0.g = haze_sky_color.g;
        v0.b = haze_sky_color.b;

        v1.r = haze_sky_color.r;
        v1.g = haze_sky_color.g;
        v1.b = haze_sky_color.b;
    }

    v2.r = sky_color.r;
    v2.g = sky_color.g;
    v2.b = sky_color.b;

    v3.r = sky_color.r;
    v3.g = sky_color.g;
    v3.b = sky_color.b;

    // Set the clip flags on the constructed verts
    SetClipFlags(&v0);
    SetClipFlags(&v1);
    SetClipFlags(&v2);
    SetClipFlags(&v3);

    // Clip and draw the smooth shaded horizon polygon
    context.RestoreState(STATE_GOURAUD);

    // here we change the order of the fan for diagonal matching the side of sun
    if (sunLeft)
    {
        vertPointers[0] = &v0;
        vertPointers[1] = &v1;
        vertPointers[2] = &v2;
        vertPointers[3] = &v3;
    }
    else
    {
        vertPointers[0] = &v3;
        vertPointers[1] = &v2;
        vertPointers[2] = &v1;
        vertPointers[3] = &v0;
    }

    ClipAndDraw2DFan(vertPointers, num);

}

/**************************************************************************
    Draw the filler from the end of the terrain data out to the horizon
***************************************************************************/
void RenderOTW::DrawFillerToHorizon(HorizonRecord *pHorizon)
{
    // find out if sun is left or right
    bool sunLeft = pHorizon->sunEffectPos.x <= scaleX;

    float dr = 0.0F, dg = 0.0F, db = 0.0F;
    const int num = 4;
    float /*hazescale, */lhazescale, rhazescale;
    TwoDVertex v0, v1, v2, v3;
    TwoDVertex *vertPointers[4];

    v0.x = shiftX + pHorizon->hx + pHorizon->vxDn; // lower right
    v0.y = shiftY + pHorizon->hy + pHorizon->vyDn;

    v1.x = shiftX - pHorizon->hx + pHorizon->vxDn; // lower left
    v1.y = shiftY - pHorizon->hy + pHorizon->vyDn;

    v2.x = shiftX - pHorizon->hx + pHorizon->vx; // horizon left
    v2.y = shiftY - pHorizon->hy + pHorizon->vy;

    v3.x = shiftX + pHorizon->hx + pHorizon->vx; // horizon right
    v3.y = shiftY + pHorizon->hy + pHorizon->vy;


    // lower verts use terrain haze color
    v0.r = haze_ground_color.r;
    v0.g = haze_ground_color.g;
    v0.b = haze_ground_color.b;

    v1.r = haze_ground_color.r;
    v1.g = haze_ground_color.g;
    v1.b = haze_ground_color.b;

    if (pHorizon->horeffect)
    {
        // horizon light effect
        dr = pHorizon->sunEffectColor.r - haze_sky_color.r;
        dg = pHorizon->sunEffectColor.g - haze_sky_color.g;
        db = pHorizon->sunEffectColor.b - haze_sky_color.b;

        //hazescale = pHorizon->hazescale;//  * 0.4f;
        rhazescale = pHorizon->rhazescale;// * 0.5f;
        lhazescale = pHorizon->lhazescale;// * 0.5f;

        v2.r = haze_sky_color.r + dr * lhazescale;
        v2.g = haze_sky_color.g + dg * lhazescale;
        v2.b = haze_sky_color.b + db * lhazescale;

        v3.r = haze_sky_color.r + dr * rhazescale;
        v3.g = haze_sky_color.g + dg * rhazescale;
        v3.b = haze_sky_color.b + db * rhazescale;
    }
    else
    {
        // no effect use sky color
        v2.r = haze_sky_color.r;
        v2.g = haze_sky_color.g;
        v2.b = haze_sky_color.b;

        v3.r = haze_sky_color.r;
        v3.g = haze_sky_color.g;
        v3.b = haze_sky_color.b;
    }


    // Set the clip flags on the constructed verts
    SetClipFlags(&v0);
    SetClipFlags(&v1);
    SetClipFlags(&v2);
    SetClipFlags(&v3);

    // here we change the order of the fan for diagonal matching the side of sun
    if (sunLeft)
    {
        vertPointers[0] = &v0;
        vertPointers[1] = &v1;
        vertPointers[2] = &v2;
        vertPointers[3] = &v3;
    }
    else
    {
        vertPointers[0] = &v3;
        vertPointers[1] = &v2;
        vertPointers[2] = &v1;
        vertPointers[3] = &v0;
    }

    context.RestoreState(STATE_GOURAUD);
    ClipAndDraw2DFan(vertPointers, num);
}

#else

void RenderOTW::DrawSkyHazeBand(struct HorizonRecord *pHorizon)
{
    float dr = 0.0F, dg = 0.0F, db = 0.0F;
    int num = 0;
    TwoDVertex v0, v1, v2, v3, v4;
    TwoDVertex *vertPointers[5] = { &v0, &v1, &v2, &v3, &v4 };


    num = 4;

    if (pHorizon->horeffect)
    {
        dr = pHorizon->sunEffectColor.r - haze_sky_color.r;
        dg = pHorizon->sunEffectColor.g - haze_sky_color.g;
        db = pHorizon->sunEffectColor.b - haze_sky_color.b;

        if (pHorizon->horeffect bitand 2) num = 5;
    }

    // Build the corners of our horizon polygon
    if (num > 4)
    {

        v0.r = haze_sky_color.r + dr * pHorizon->rhazescale;
        v0.g = haze_sky_color.g + dg * pHorizon->rhazescale;
        v0.b = haze_sky_color.b + db * pHorizon->rhazescale;

        v1.r = haze_sky_color.r + dr * pHorizon->hazescale;
        v1.g = haze_sky_color.g + dg * pHorizon->hazescale;
        v1.b = haze_sky_color.b + db * pHorizon->hazescale;

        v2.r = haze_sky_color.r + dr * pHorizon->lhazescale;
        v2.g = haze_sky_color.g + dg * pHorizon->lhazescale;
        v2.b = haze_sky_color.b + db * pHorizon->lhazescale;

        v0.x = shiftX + pHorizon->hx + pHorizon->vx; // horizon right
        v0.y = shiftY + pHorizon->hy + pHorizon->vy;
        v1.x = pHorizon->sunEffectPos.x;
        v1.y = pHorizon->sunEffectPos.y;
        v2.x = shiftX - pHorizon->hx + pHorizon->vx; // horizon left
        v2.y = shiftY - pHorizon->hy + pHorizon->vy;

        v3.x = shiftX - pHorizon->hx + pHorizon->vxUp; // upper left
        v3.y = shiftY - pHorizon->hy + pHorizon->vyUp;
        v3.r = sky_color.r;
        v3.g = sky_color.g;
        v3.b = sky_color.b;

        v4.x = shiftX + pHorizon->hx + pHorizon->vxUp; // upper right
        v4.y = shiftY + pHorizon->hy + pHorizon->vyUp;
        v4.r = sky_color.r;
        v4.g = sky_color.g;
        v4.b = sky_color.b;
    }
    else
    {
        v0.x = shiftX + pHorizon->hx + pHorizon->vx; // horizon right
        v0.y = shiftY + pHorizon->hy + pHorizon->vy;
        v1.x = shiftX - pHorizon->hx + pHorizon->vx; // horizon left
        v1.y = shiftY - pHorizon->hy + pHorizon->vy;
        v2.x = shiftX - pHorizon->hx + pHorizon->vxUp; // upper left
        v2.y = shiftY - pHorizon->hy + pHorizon->vyUp;
        v3.x = shiftX + pHorizon->hx + pHorizon->vxUp; // upper right
        v3.y = shiftY + pHorizon->hy + pHorizon->vyUp;

        if (pHorizon->horeffect)
        {
            v0.r = haze_sky_color.r + dr * pHorizon->rhazescale;
            v0.g = haze_sky_color.g + dg * pHorizon->rhazescale;
            v0.b = haze_sky_color.b + db * pHorizon->rhazescale;

            v1.r = haze_sky_color.r + dr * pHorizon->lhazescale;
            v1.g = haze_sky_color.g + dg * pHorizon->lhazescale;
            v1.b = haze_sky_color.b + db * pHorizon->lhazescale;
        }
        else
        {
            v0.r = haze_sky_color.r;
            v0.g = haze_sky_color.g;
            v0.b = haze_sky_color.b;
            v1.r = haze_sky_color.r;
            v1.g = haze_sky_color.g;
            v1.b = haze_sky_color.b;
        }

        v2.r = sky_color.r;
        v2.g = sky_color.g;
        v2.b = sky_color.b;
        v3.r = sky_color.r;
        v3.g = sky_color.g;
        v3.b = sky_color.b;
    }


    // Set the clip flags on the constructed verts
    SetClipFlags(&v0);
    SetClipFlags(&v1);
    SetClipFlags(&v2);
    SetClipFlags(&v3);

    if (num > 4)
    {
        SetClipFlags(&v4);
    }


    // Clip and draw the smooth shaded horizon polygon
    context.RestoreState(STATE_GOURAUD);
    /*
    if (dithered){
     context.SetState( MPR_STA_ENABLES, MPR_SE_DITHERING );
     context.InvalidateState();
    }
    */
    ClipAndDraw2DFan(&vertPointers[0], num);
}

void RenderOTW::DrawFillerToHorizon(HorizonRecord *pHorizon)
{
    float dr = 0.0F, dg = 0.0F, db = 0.0F;
    int num = 0;
    float hazescale, lhazescale, rhazescale;
    TwoDVertex v0, v1, v2, v3, v4;
    TwoDVertex *vertPointers[5] = { &v0, &v1, &v2, &v3, &v4 };

    v0.x = shiftX + pHorizon->hx + pHorizon->vxDn; // lower right
    v0.y = shiftY + pHorizon->hy + pHorizon->vyDn;
    v0.r = haze_ground_color.r;
    v0.g = haze_ground_color.g;
    v0.b = haze_ground_color.b;
    v1.x = shiftX - pHorizon->hx + pHorizon->vxDn; // lower left
    v1.y = shiftY - pHorizon->hy + pHorizon->vyDn;
    v1.r = haze_ground_color.r;
    v1.g = haze_ground_color.g;
    v1.b = haze_ground_color.b;

    v2.x = shiftX - pHorizon->hx + pHorizon->vx; // horizon left
    v2.y = shiftY - pHorizon->hy + pHorizon->vy;

    if (pHorizon->horeffect)
    {
        dr = pHorizon->sunEffectColor.r - earth_end_color.r;
        dg = pHorizon->sunEffectColor.g - earth_end_color.g;
        db = pHorizon->sunEffectColor.b - earth_end_color.b;

        // scale down the scale factor for ground
        hazescale = pHorizon->hazescale  * 0.4f;
        lhazescale = pHorizon->rhazescale * 0.5f;
        rhazescale = pHorizon->lhazescale * 0.5f;

        v2.r = earth_end_color.r + dr * lhazescale;
        v2.g = earth_end_color.g + dg * lhazescale;
        v2.b = earth_end_color.b + db * lhazescale;

        if (pHorizon->horeffect bitand 2)
        {
            num = 5;

            v3.x = pHorizon->sunEffectPos.x;
            v3.y = pHorizon->sunEffectPos.y;
            v4.x = shiftX + pHorizon->hx + pHorizon->vx; // horizon right
            v4.y = shiftY + pHorizon->hy + pHorizon->vy;

            v3.r = earth_end_color.r + dr * hazescale;
            v3.g = earth_end_color.g + dg * hazescale;
            v3.b = earth_end_color.b + db * hazescale;

            v4.r = earth_end_color.r + dr * rhazescale;
            v4.g = earth_end_color.g + dg * rhazescale;
            v4.b = earth_end_color.b + db * rhazescale;
        }
        else
        {
            num = 4;

            v3.x = shiftX + pHorizon->hx + pHorizon->vx; // horizon right
            v3.y = shiftY + pHorizon->hy + pHorizon->vy;

            v3.r = earth_end_color.r + dr * rhazescale;
            v3.g = earth_end_color.g + dg * rhazescale;
            v3.b = earth_end_color.b + db * rhazescale;
        }
    }
    else
    {
        num = 4;

        v3.x = shiftX + pHorizon->hx + pHorizon->vx; // horizon right
        v3.y = shiftY + pHorizon->hy + pHorizon->vy;

        v2.r = earth_end_color.r;
        v2.g = earth_end_color.g;
        v2.b = earth_end_color.b;
        v3.r = earth_end_color.r;
        v3.g = earth_end_color.g;
        v3.b = earth_end_color.b;
    }


    // Set the clip flags on the constructed verts
    SetClipFlags(&v0);
    SetClipFlags(&v1);
    SetClipFlags(&v2);
    SetClipFlags(&v3);

    if (num > 4)
    {
        SetClipFlags(&v4);
    }


    // Clip and draw the terrain data to horizon filler
#ifdef FLAT_FILLER
    context.RestoreState(STATE_SOLID);
    v4.r = v3.r = v2.r = v1.r = v0.r;
    v4.g = v3.g = v2.g = v1.g = v0.g;
    v4.b = v3.b = v2.b = v1.b = v0.b;
#else
    context.RestoreState(STATE_GOURAUD);

    if (dithered)
    {
        context.SetState(MPR_STA_ENABLES, MPR_SE_DITHERING);
        // context.InvalidateState();
    }

#endif
    ClipAndDraw2DFan(&vertPointers[0], num);
}
#endif

void RenderOTW::DrawStars(void)
{
    // RED - Do not draw if inside a layer
    if (realWeather->InsideOvercast() or realWeather->UnderOvercast()) return;

    float starblend = TheTimeOfDay.GetStarIntensity();
    float vpAlt = -viewpoint->Z();


    if (vpAlt > SKY_ROOF_HEIGHT)
    {
        float althazefactor;

        if (vpAlt > SKY_MAX_HEIGHT)
        {
            althazefactor = 0.2f;
        }
        else
        {
            althazefactor = (SKY_MAX_HEIGHT - vpAlt) * HAZE_ALTITUDE_FACTOR;

            if (althazefactor < 0.2f) althazefactor = 0.2f;
        }

        starblend = min(1.0f, starblend + 1.0f - althazefactor);
    }


    if (starblend > 0.000001f)
    {
        Tcolor star_color;
        Tcolor sky_part;
        DWORD draw_color;
        MPRVtx_t vert;
        register float scratch_x;
        register float scratch_y;
        register float scratch_z;


        // Compute the sky color portion of the star colors
        float blend = 255.0f * (1.0f - starblend);
        sky_part.r = sky_color.r * blend;
        sky_part.g = sky_color.g * blend;
        sky_part.b = sky_color.b * blend;

        context.RestoreState(STATE_SOLID);

        StarData *stardata = TheTimeOfDay.GetStarData();
        StarCoord *coord = stardata -> coord;
        int lastcolor = -1;
        int i;

        for (i = 0; i < stardata -> totalcoord; i++, coord++)
        {
            if (coord -> flag) continue;

            if (lastcolor not_eq coord -> color)
            {
                lastcolor = coord -> color;
                float curcolor = lastcolor * starblend;
                star_color.r = curcolor + sky_part.r;
                star_color.g = curcolor + sky_part.g;
                star_color.b = curcolor + sky_part.b;

                if (star_color.r > 255.0f) star_color.r = 255.0f;

                if (star_color.g > 255.0f) star_color.g = 255.0f;

                if (star_color.b > 255.0f) star_color.b = 255.0f;

                if (star_color.r < 64.0f) star_color.r = 64.0f;

                if (star_color.g < 64.0f) star_color.g = 64.0f;

                if (star_color.b < 64.0f) star_color.b = 64.0f;

                ProcessColor(&star_color);
                draw_color = (DWORD)star_color.r |
                             (DWORD)star_color.g << 8 |
                             (DWORD)star_color.g << 16;
                context.SelectForegroundColor(draw_color);
            }

            // This part does rotation, translation, and scaling
            // Note, we're swapping the x and z axes here to get from z up/down to z far/near
            // then we're swapping the x and y axes to get into conventional screen pixel coordinates
            scratch_z = T.M11 * coord->x + T.M12 * coord->y + T.M13 * coord->z;
            scratch_x = T.M21 * coord->x + T.M22 * coord->y + T.M23 * coord->z;
            scratch_y = T.M31 * coord->x + T.M32 * coord->y + T.M33 * coord->z;

            // Now determine if the point is out behind us or to the sides
            if (scratch_z < 0.000001f) continue;

            if (GetHorizontalClipFlags(scratch_x, scratch_z) not_eq ON_SCREEN) continue;

            if (GetVerticalClipFlags(scratch_y, scratch_z) not_eq ON_SCREEN) continue;

            // Finally, do the perspective divide and scale and shift into screen space
            register float OneOverZ = 1.0f / scratch_z;
            vert.x = viewportXtoPixel(scratch_x * OneOverZ);
            vert.y = viewportYtoPixel(scratch_y * OneOverZ);

            // Draw the point (we _REALLY_ should do several (or all) points at once)
            context.DrawPrimitive(MPR_PRM_POINTS, 0, 1, &vert, sizeof(vert));
        }
    }
}


/***************************************************************************\
 Draw the sun
\***************************************************************************/
void RenderOTW::DrawSun(void)
{
    Tpoint center;
    float alpha;
    float dist;


    // RED - Do not draw if inside a layer
    if (realWeather->InsideOvercast() or realWeather->UnderOvercast()) return;

    // ShiAssert( TheTimeOfDay.ThereIsASun() );

    // Get the center point of the body on a unit sphere in world space
    TheTimeOfDay.CalculateSunMoonPos(&center, FALSE);

    // Draw the sun and its glare as one object sun
    alpha = max(SunGlareValue, MIN_SUN_GLARE);
    ShiAssert(alpha >= 0.0f);
    ShiAssert(alpha <= 1.0f);

    dist = SUN_DIST;
    float maxdist = MOST_SUN_GLARE_DIST;
    int sunpitch = TheTimeOfDay.GetSunPitch();

    if (sunpitch < 256)
    {
        sunpitch = 16 - (sunpitch >> 4);
        dist -= sunpitch;
    }

    // Compute the (inverse of the) size of the glare polygon
    dist += (alpha) * (maxdist - dist);

    // Draw the object

    //JAM 04Oct03
    if (DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS)
    {
        context.RestoreState(STATE_ALPHA_TEXTURE_GOURAUD);
        context.SelectTexture1(viewpoint->SunTexture.TexHandle());
        DrawCelestialBody(&center, dist / 4.f, 1.f, 0.984375f, 0.9765625f, 0.87109375f);
    }
    else
    {
        context.RestoreState(STATE_ALPHA_TEXTURE);
        context.SelectTexture1(viewpoint->SunTexture.TexHandle());
        DrawCelestialBody(&center, dist, alpha);
        Draw2DSunGlowEffect(this, &center, dist, alpha);
    }

    //JAM
}


/***************************************************************************\
 Draw the moon
\***************************************************************************/
void RenderOTW::DrawMoon(void)
{
    // RED - Do not draw if inside a layer
    if (realWeather->InsideOvercast() or realWeather->UnderOvercast()) return;

    Tpoint center;

    ShiAssert(TheTimeOfDay.ThereIsAMoon());

    // Get the center point of the body on a unit sphere in world space
    TheTimeOfDay.CalculateSunMoonPos(&center, TRUE);

    // Draw the object
    context.RestoreState(STATE_ALPHA_TEXTURE_GOURAUD);

    if (TheTimeOfDay.GetNVGmode())
    {
        context.SelectTexture1(viewpoint->GreenMoonTexture.TexHandle());
    }
    else
    {
        context.SelectTexture1(viewpoint->MoonTexture.TexHandle());
    }


    float dist = MOON_DIST;
#if 0 // I think this looks a little silly.  Let try without it...
    int moonpitch = TheTimeOfDay.GetMoonPitch();

    if (moonpitch < 512)
    {
        dist -= 0.25f * (8 - (moonpitch >> 6));
    }

#endif

    float glare = 0.0f;

    if (TheTimeOfDay.ThereIsASun()) glare = SunGlareValue;

    float moonblend = TheTimeOfDay.CalculateMoonBlend(glare);

    if (moonblend < 1.0f)
    {
        float vpAlt = -Z();

        if (vpAlt > SKY_MAX_HEIGHT) moonblend = 1.0f;
        else if (vpAlt > SKY_MAX_HEIGHT - 16384.0f)
        {
            vpAlt = (SKY_MAX_HEIGHT - vpAlt);
            moonblend += (float) glGetSine(FloatToInt32(vpAlt) >> 2);

            if (moonblend > 1.0f) moonblend = 1.0f;
        }
    }

    DrawCelestialBody(&center, dist, moonblend);
}


/***************************************************************************\
 Do the setup for the billboard of a celestial object (sun/moon)
\***************************************************************************/
int RenderOTW::DrawCelestialBody(Tpoint *cntr, float dist, float alpha, float r, float g, float b)
{
    ThreeDVertex v0, v1, v2, v3;
    Tpoint eastSide;
    Tpoint corner;
    Tpoint center = *cntr;

    // Cross the vector toward the center with North (1,0,0) to get the side vector

    // eastSide.x = center.y*n.z - center.z*n.y;
    // eastSide.y = center.z*n.x - center.x*n.z;
    // eastSide.z = center.x*n.y - center.y*n.x;
    eastSide.x =  0.0f;
    eastSide.y =  center.z;
    eastSide.z = -center.y;


    // Now push the center point outward to shrink the object
    center.x *= dist;
    center.y *= dist;
    center.z *= dist;

    // North West corner
    corner.x = center.x + 1.0f;
    corner.y = center.y - eastSide.y;
    corner.z = center.z - eastSide.z;
    TransformCameraCentricPoint(&corner, &v0);
    v0.u = 0.0f, v0.v = 0.0f;
    v0.q = 1.0f;

    //JAM 04Oct03
    if (DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS)
    {
        v0.r = r, v0.g = g;
        v0.b = b;
        v0.a = alpha;
    }
    else
    {
        v0.r = 1.0f, v0.g = 1.0f;
        v0.b = 1.0f;
        v0.a = alpha;
    }

    // North East corner
    corner.y = center.y + eastSide.y;
    corner.z = center.z + eastSide.z;
    TransformCameraCentricPoint(&corner, &v1);
    v1.u = 1.0f, v1.v = 0.0f;
    v1.q = 1.0f;

    //JAM 04Oct03
    if (DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS)
    {
        v1.r = r, v1.g = g;
        v1.b = b;
        v1.a = alpha;
    }
    else
    {
        v1.r = 1.0f, v1.g = 1.0f;
        v1.b = 1.0f;
        v1.a = alpha;
    }

    // South East corner
    corner.x = center.x - 1.0f;
    TransformCameraCentricPoint(&corner, &v2);
    v2.u = 1.0f, v2.v = 1.0f;
    v2.q = 1.0f;

    //JAM 04Oct03
    if (DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS)
    {
        v2.r = r, v2.g = g;
        v2.b = b;
        v2.a = alpha;
    }
    else
    {
        v2.r = 1.0f, v2.g = 1.0f;
        v2.b = 1.0f;
        v2.a = alpha;
    }

    // South West corner
    corner.y = center.y - eastSide.y;
    corner.z = center.z - eastSide.z;
    TransformCameraCentricPoint(&corner, &v3);
    v3.u = 0.0f, v3.v = 1.0f;
    v3.q = 1.0f;

    //JAM 04Oct03
    if (DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS)
    {
        v3.r = r, v3.g = g;
        v3.b = b;
        v3.a = alpha;
    }
    else
    {
        v3.r = 1.0f, v3.g = 1.0f;
        v3.b = 1.0f;
        v3.a = alpha;
    }

    // Render the polygon
    bool gif = false;

    if (g_nGfxFix bitand 0x04)
        gif = true;

    if (v0.clipFlag bitand v1.clipFlag bitand v2.clipFlag bitand v3.clipFlag) return 0; // not visible

    DrawSquare(&v0, &v1, &v2, &v3, CULL_ALLOW_ALL, gif);

    return 1;

}


/***************************************************************************\
    Compute the effect of the sun on the horizon during sunset/sunrise.
\***************************************************************************/
void RenderOTW::ComputeHorizonEffect(HorizonRecord *pHorizon)
{
    Tpoint sunEffectWorldSpace;
    ThreeDVertex sunEffectScreenSpace;

    //JAM 09Dec03
    if (realWeather->UnderOvercast() or realWeather->InsideOvercast())
    {
        pHorizon->horeffect = 0;
        return;
    }

    // Return now if the sun isn't up now
    if ( not TheTimeOfDay.ThereIsASun())
    {
        // No horizon effect
        pHorizon->horeffect = 0;
        return;
    }

    int bandangle = glConvertFromRadian(pHorizon->bandAngleUp);
    int sunpitch = TheTimeOfDay.GetSunPitch();
    int deltapitch = sunpitch - bandangle;

    // Return now if the sun isn't near the horizon
    if (deltapitch >= 784)
    {
        // No horizon effect
        pHorizon->horeffect = 0;
        return;
    }

    // Note that there is going to be a horizon effect
    pHorizon->horeffect = 1;

    // Figure out where the sun is...
    if (deltapitch > 0)
    {
        // Sun is above the horizon band
        pHorizon->hazescale = 0.6f - 0.6f * (float) deltapitch / 784.0f;
    }
    else
    {
        // Sun is comming up through the horizon
        if (sunpitch < 0)
        {
            // calculate scale factor when sun is below horizon to prevent color popup
            if (sunpitch < -256)
            {
                pHorizon->hazescale = 0.0f;
            }
            else
            {
                pHorizon->hazescale = 0.8f - (float) - sunpitch / 256.0f;

                if (pHorizon->hazescale < 0.0f)
                {
                    pHorizon->hazescale = 0.0f;
                }
            }
        }
        else
        {
            // calculate scale factor when sun is inside the band
            pHorizon->hazescale = 0.6f + 0.4f * (float) - deltapitch / (float) bandangle;

            if (pHorizon->hazescale > 1.0f)
            {
                pHorizon->hazescale = 1.0f;
            }
        }
    }

    // calculate point on the horizon line to represent sun position
    TheTimeOfDay.CalculateSunGroundPos(&sunEffectWorldSpace);
    TransformCameraCentricPoint(&sunEffectWorldSpace, &sunEffectScreenSpace);

    if (1/* not (sunEffectScreenSpace.clipFlag bitand CLIP_NEAR)*/)
    {
        pHorizon->sunEffectPos.x = sunEffectScreenSpace.x;
        pHorizon->sunEffectPos.y = sunEffectScreenSpace.y;
        pHorizon->sunEffectPos.z = 1.0f;
        Edge horizonLine;
        horizonLine.SetupWithVector(shiftX + pHorizon->vx, shiftY + pHorizon->vy, pHorizon->hx, pHorizon->hy);
        pHorizon->sunEffectPos.y = horizonLine.Y(pHorizon->sunEffectPos.x);
        pHorizon->horeffect or_eq 2;
    }

    // calculate scale factor on the left and right side based on the yaw
    pHorizon->lhazescale = pHorizon->rhazescale = 0.0f;
    int anglesize = glConvertFromRadian(diagonal_half_angle);
    int yaw = glConvertFromRadian(Yaw());
    int leftyaw = (yaw - anglesize) bitand 0x3fff;
    int rightyaw = (yaw + anglesize) bitand 0x3fff;
    anglesize <<= 1;
    float sizeperangle = pHorizon->hazescale / (float)(anglesize);
    int sunyaw = TheTimeOfDay.GetSunYaw();

    int i, j;
    i = (sunyaw - leftyaw) bitand 0x3fff;
    j = (leftyaw - sunyaw) bitand 0x3fff;

    if (i > j)
    {
        i = j;
    }

    if (i < anglesize)
    {
        pHorizon->lhazescale = pHorizon->hazescale - (i * sizeperangle);
    }

    i = (sunyaw - rightyaw) bitand 0x3fff;
    j = (rightyaw - sunyaw) bitand 0x3fff;

    if (i > j)
    {
        i = j;
    }

    if (i < anglesize)
    {
        pHorizon->rhazescale = pHorizon->hazescale - (i * sizeperangle);
    }

    // Get the effect of the sun on the horizon color
    TheTimeOfDay.GetHazeSunHorizonColor(&pHorizon->sunEffectColor);
    ProcessColor(&pHorizon->sunEffectColor);
}

/***************************************************************************\
    Establish the lighting parameters for this renderer as the time of
 day changes.
\***************************************************************************/
void RenderOTW::SetTimeOfDayColor(void)
{

    // if NVG Mode
    if (TheTimeOfDay.GetNVGmode())
    {
        lightAmbient = NVG_LIGHT_LEVEL;
        lightDiffuse = 0.f;
        lightSpecular = 0.f;
        TheTimeOfDay.GetLightDirection(&lightVector);

        sky_color.r = 0.f;
        sky_color.g = NVG_SKY_LEVEL;
        sky_color.b = 0.f;
        haze_sky_color.r = 0.f;
        haze_sky_color.g = NVG_SKY_LEVEL;
        haze_sky_color.b = 0.f;
        earth_end_color.r = 0.f;
        earth_end_color.g = NVG_SKY_LEVEL;
        earth_end_color.b = 0.f;
        haze_ground_color.r = 0.f;
        haze_ground_color.g = NVG_SKY_LEVEL;
        haze_ground_color.b = 0.f;

        DWORD ground_haze = (FloatToInt32(haze_ground_color.g * 255.9f) << 8) + 0xff000000;
        context.SetState(MPR_STA_FOG_COLOR, ground_haze);
    }
    else
    {

        Tcolor light;

        // Set 3D object lighting environment
        lightAmbient = TheTimeOfDay.GetAmbientValue();
        lightDiffuse = TheTimeOfDay.GetDiffuseValue();
        lightSpecular = TheTimeOfDay.GetSpecularValue();
        TheTimeOfDay.GetLightDirection(&lightVector);

        // Store terrain lighting environment (not used at present)
        lightTheta = (float)atan2(lightVector.y, lightVector.x);
        lightPhi = (float)atan2(-lightVector.z, sqrt(lightVector.x * lightVector.x + lightVector.y * lightVector.y));
        ShiAssert(lightPhi <= PI * 0.5f);

        // Get the new colors for this time of day
        TheTimeOfDay.GetSkyColor(&sky_color);
        TheTimeOfDay.GetHazeSkyColor(&haze_sky_color);
        TheTimeOfDay.GetHazeGroundColor(&earth_end_color);
        TheTimeOfDay.GetGroundColor(&haze_ground_color);
        ProcessColor(&sky_color);
        ProcessColor(&haze_sky_color);
        ProcessColor(&earth_end_color);
        ProcessColor(&haze_ground_color);

        // Set the fog color for the terrain
        DWORD ground_haze = (FloatToInt32(haze_ground_color.r * 255.9f)) +
                            (FloatToInt32(haze_ground_color.g * 255.9f) <<  8) +
                            (FloatToInt32(haze_ground_color.b * 255.9f) << 16) + 0xff000000;

        context.SetState(MPR_STA_FOG_COLOR, ground_haze);

        //JAM 03Dec03
        TheTimeOfDay.GetTextureLightingColor(&ground_color);

        // TODO:  Set the fog color for the objects
        // TheStateStack.SetDepthCueColor( haze_ground_color.r, haze_ground_color.g, haze_ground_color.b );

        // Adjust the color of the roof textures if they're loaded
        if (texRoofTop.TexHandle())
        {
            TheTimeOfDay.GetTextureLightingColor(&light);
            Palette *palette;
            palette = texRoofTop.GetPalette();
            palette->LightTexturePalette(&light);
            //texRoofTop.palette->LightTexturePalette( &light );
            palette = texRoofBottom.GetPalette();
            palette->LightTexturePalette(&light);
            //texRoofBottom.palette->LightTexturePalette( &light );
        }

    }
}


/***************************************************************************\
    Adjust the target color as necessary for display.  This is here
 just to allow derived classes (like RenderTV) to convert colors as
 necessary.
\***************************************************************************/
//void RenderOTW::ProcessColor( Tcolor *color )
void RenderOTW::ProcessColor(Tcolor * color)
{
    if (TheTimeOfDay.GetNVGmode())
    {
        color->r  = 0.f;
        color->g *= NVG_LIGHT_LEVEL;
        color->b  = 0.f;
    }
}


/***************************************************************************\
    Adjust the sky color based on angle from sun and altitude.  This is
 updated each frame.
\***************************************************************************/
void RenderOTW::AdjustSkyColor(void)
{
    //JAM 09Dec03
    if (realWeather->InsideOvercast() or realWeather->UnderOvercast()) return;

    TheTimeOfDay.GetSkyColor(&sky_color);
    ProcessColor(&sky_color);

    // Start with the default sky color for this time of day

    // darken color at high altitude
    float vpAlt = -viewpoint->Z();

    if (vpAlt > SKY_ROOF_HEIGHT)
    {
        float althazefactor, althazefactorblue;

        if (vpAlt > SKY_MAX_HEIGHT)
        {
            althazefactor = 0.2f;
            althazefactorblue = 0.4f;
        }
        else
        {
            althazefactor = (SKY_MAX_HEIGHT - vpAlt) * HAZE_ALTITUDE_FACTOR;
            althazefactorblue = 0.4f + (althazefactor * 0.6f);

            if (althazefactor < 0.2f) althazefactor = 0.2f;
        }

        sky_color.r *= althazefactor;
        sky_color.g *= althazefactor;
        sky_color.b *= althazefactorblue;
    }


    // calculate sun glare effect
    // sfr sun glare effect
    if (TheTimeOfDay.ThereIsASun())
    {
        int pitch = glConvertFromRadian(Pitch());
        int yaw = glConvertFromRadian(Yaw());
        SunGlareValue = TheTimeOfDay.GetSunGlare(yaw, pitch);

        if (SunGlareValue)
        {
            float vpAlt = -viewpoint->Z();

            if (vpAlt < SKY_MAX_HEIGHT)
            {
                vpAlt = (SKY_MAX_HEIGHT - vpAlt) * GLARE_FACTOR;
                float intensity = (float)glGetSine(FloatToInt32(vpAlt));
                intensity *= SunGlareValue;
                intensity *= 0.25f; // scale it down

                if (intensity > 0.05f)
                {
                    sky_color.r += intensity;
                    sky_color.g += intensity;
                    sky_color.b += intensity;

                    if (sky_color.r > 1.0f) sky_color.r = 1.0f;

                    if (sky_color.g > 1.0f) sky_color.g = 1.0f;

                    if (sky_color.b > 1.0f) sky_color.b = 1.0f;
                }
            }
        }
    }

    TheTimeOfDay.SetCurrentSkyColor(&sky_color);
}


/***************************************************************************\
 This function is called from the miscellanious texture loader function.
 It must be hardwired into that function.
\***************************************************************************/
//void RenderOTW::SetupTexturesOnDevice( DWORD rc )
void RenderOTW::SetupTexturesOnDevice(DXContext *rc)
{
}


/***************************************************************************\
    This function is called from the miscellanious texture clean up function.
 It must be hardwired into that function.
\***************************************************************************/
//void RenderOTW::ReleaseTexturesOnDevice( DWORD rc )
void RenderOTW::ReleaseTexturesOnDevice(DXContext *rc)
{
    // Free our texture resources
    if (texRoofTop.TexHandle())
    {
        texRoofTop.FreeAll();
        texRoofBottom.FreeAll();
    }
}


/***************************************************************************\
 Update the lighting properties based on the time of day
\***************************************************************************/
void RenderOTW::TimeUpdateCallback(void *self)
{
    // RED - Passed into the OTW Loop before any rendering
    //((RenderOTW*)self)->SetTimeOfDayColor();
}
