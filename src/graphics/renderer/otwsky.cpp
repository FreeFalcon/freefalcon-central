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

// Artscout - 2026: #96 3D skydome -- draw the sky as WORLD geometry through the object path (VI-aware, per-view
// gProj2), replacing the 2D-screen bands/sun/moon that couldn't be per-eye under quad view instancing.
#include "Graphics/DXEngine/DXEngine.h"              // CDXEngine::GetObjProjection/View
#include "Graphics/DXEngine/common/IRenderer.h"      // g_pRenderer, BeginSkyPass, DrawTerrainMesh
extern bool g_b3DSky;
// Artscout - 2026: #13 volumetric clouds (see RenderOTW::DrawVolumetricClouds).
extern bool  g_bVolumetricClouds;
extern float g_fCloudSteps, g_fCloudCoverage, g_fCloudDensity, g_fCloudScale;
extern float g_fCloudAmbient, g_fCloudPowder;
extern float g_fCumulusBase, g_fCumulusThick, g_fCumulusCoverage;
extern float g_fSunWarmth;   // #96 sun disc warmth (0 = physical white, 1 = strongly yellow)
extern float g_fSkyDomeRadius, g_fSkyDomeSunSize, g_fSkyDomeMoonSize, g_fSkyDomeStarSize;
extern float g_fSkyMapRotate;   // #96: equirect starmap azimuth rotation (0..1 = full turn) to place the Milky Way
extern float g_fSkyMapBright;   // #96: starmap brightness multiplier (additive over the night sky)
extern float g_fSkyMapTilt;     // #96: starmap tilt OFFSET (turns*PI) on top of the auto latitude tilt
extern float g_fLatitude;       // theater latitude (deg) -- drives the auto celestial-pole tilt (Polaris to the N)

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
// Artscout - 2026 (VR off-axis): shift the sky horizon-line positions to meet the off-axis-projected
// terrain & sky geometry. The bands are placed in screen pixels from tan(Pitch())*scale (a SYMMETRIC
// projection assumption); the geometry is shifted by the off-axis (T-fold), so without this the per-eye
// clear shows through as a coloured stripe at the horizon. Apply ONLY the component of the off-axis
// screen shift (oaX,oaY) PERPENDICULAR to the horizon line (along (sR,cR)). The sky is uniform ALONG
// the horizon, so the along-horizon component is pointless AND harmful: a large horizontal shift (gaze
// to the side -> big oaX) would slide the finite-width haze quad off the focus view, leaving a flat
// clear-sky block + seam (seen in 7.png). For a level horizon this reduces to a pure vertical shift.
static void ShiftHorizonOffAxis(HorizonRecord* h, float oaX, float oaY, float sR, float cR)
{
    const float perp = oaX * sR + oaY * cR;   // component perpendicular to the horizon line
    if (perp == 0.0f) return;
    const float dx = perp * sR, dy = perp * cR;
    h->vx   += dx; h->vy   += dy;
    h->vxUp += dx; h->vyUp += dy;
    h->vxDn += dx; h->vyDn += dy;
}

BOOL RenderOTW::DrawSky(void)
{
    // Update the sky color based on our current attitude and position
    AdjustSkyColor();

    // Artscout - 2026: #96 -- 3D world-space skydome path (VI-correct sky/sun/moon). Replaces the ENTIRE 2D sky
    // (bands/stars/sun/moon) in ALL cases when g_b3DSky. AdjustSkyColor (above) already filled sky_color/haze_sky_
    // color, which the dome reuses. Clouds are a SEPARATE system (DX2D layers) and still draw on top of the dome.
    // needTerrain preserves the 2D logic: FALSE only when above the cloud roof (no ground visible), else TRUE.
    if (g_b3DSky)
    {
        DrawSkyDome();
        if (skyRoof and viewpoint->Z() < -SKY_ROOF_HEIGHT) return FALSE;   // above the deck: ground hidden
        return TRUE;
    }

    // #48: the sky is drawn through the 2D screen-primitive path. Push the sky background to the FAR plane so the
    // pit and the world draw in front of it. reversed-Z: far = 0.0 (was 1.0 under standard Z). Restored before return.
    context.m_2DPrimZ = 0.0f;
    BOOL needTerrain;

    if ( not skyRoof)
    {
        DrawSkyNoRoof();
        needTerrain = TRUE; // Need to draw terrain
    }
    else if (viewpoint->Z() < -SKY_ROOF_HEIGHT)
    {
        DrawSkyAbove();
        needTerrain = FALSE; // Don't need to draw terrain
    }
    else
    {
        DrawSkyBelow();
        needTerrain = TRUE; // Need to draw terrain
    }

    context.m_2DPrimZ = 1.0f; // restore NEAR plane for UI/HUD 2D primitives (reversed-Z: near = 1.0)
    return needTerrain;
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

    // Artscout - 2026 (horizon): extend the filler band DOWN past the terrain end so the near/far
    // (fartiles) terrain seam shows GROUND HAZE through the gap (the sky is drawn behind the terrain),
    // instead of a black contour stripe. Terrain draws on top where it exists, so over-extending the
    // (behind-terrain) filler is safe -- it only shows in the gaps.
    extern float g_fHorizonFillerExtend;
    horizon.vxDn += (horizon.vxDn - horizon.vx) * g_fHorizonFillerExtend;
    horizon.vyDn += (horizon.vyDn - horizon.vy) * g_fHorizonFillerExtend;

    ShiftHorizonOffAxis(&horizon, -m_vrOffAxisX * scaleX, -m_vrOffAxisY * scaleY, sR, cR);

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

    // Artscout - 2026 (horizon): extend the filler band DOWN past the terrain end so the near/far
    // (fartiles) terrain seam shows GROUND HAZE through the gap (the sky is drawn behind the terrain),
    // instead of a black contour stripe. Terrain draws on top where it exists, so over-extending the
    // (behind-terrain) filler is safe -- it only shows in the gaps.
    extern float g_fHorizonFillerExtend;
    horizon.vxDn += (horizon.vxDn - horizon.vx) * g_fHorizonFillerExtend;
    horizon.vyDn += (horizon.vyDn - horizon.vy) * g_fHorizonFillerExtend;

    ShiftHorizonOffAxis(&horizon, -m_vrOffAxisX * scaleX, -m_vrOffAxisY * scaleY, sR, cR);


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

    // Artscout - 2026 (horizon): extend the filler band DOWN past the terrain end so the near/far
    // (fartiles) terrain seam shows GROUND HAZE through the gap (the sky is drawn behind the terrain),
    // instead of a black contour stripe. Terrain draws on top where it exists, so over-extending the
    // (behind-terrain) filler is safe -- it only shows in the gaps.
    extern float g_fHorizonFillerExtend;
    horizon.vxDn += (horizon.vxDn - horizon.vx) * g_fHorizonFillerExtend;
    horizon.vyDn += (horizon.vyDn - horizon.vy) * g_fHorizonFillerExtend;

    ShiftHorizonOffAxis(&horizon, -m_vrOffAxisX * scaleX, -m_vrOffAxisY * scaleY, sR, cR);


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


// Artscout - 2026: #DX12 п.5 -- when TRUE, the VI world pass skips the sun/moon so they can be re-drawn PER EYE in
// the tail (each with its own off-axis frustum). Drawn through the shared VS_Screen path they'd land at ONE NDC for
// both slices; under quad foveation the gaze-canted focus eyes then disagree -> the sun DOUBLES and rides the gaze.
bool g_bVrDeferCelestial = false;

/***************************************************************************\
 Draw the sun
\***************************************************************************/
void RenderOTW::DrawSun(void)
{
    if (g_bVrDeferCelestial) return;   // suppressed in the VI world pass; re-drawn per eye in the tail
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
        // Artscout - 2026 (#79): full dist -- the old dist/4 blew the sun up to 4x (billboard angular size ~ 1/dist).
        DrawCelestialBody(&center, dist, 1.f, 0.984375f, 0.9765625f, 0.87109375f);
        // Artscout - 2026 (#79): flush the billboard NOW, while SunTexture is still bound to slot 0. DrawSquare
        // batches into the VB; without this the sun verts flushed later (after the GPU terrain re-bound slot 0 via
        // a direct SetTexture(0)) and sampled the terrain tile / white -> the "square with ground/white" in VR.
        context.FlushPending();
    }
    else
    {
        context.RestoreState(STATE_ALPHA_TEXTURE);
        context.SelectTexture1(viewpoint->SunTexture.TexHandle());
        DrawCelestialBody(&center, dist, alpha);
        Draw2DSunGlowEffect(this, &center, dist, alpha);
        context.FlushPending();   // #79: submit while SunTexture is bound (see above)
    }

    //JAM
}


/***************************************************************************\
 Draw the moon
\***************************************************************************/
void RenderOTW::DrawMoon(void)
{
    extern bool g_bVrDeferCelestial;
    if (g_bVrDeferCelestial) return;   // suppressed in the VI world pass; re-drawn per eye in the tail

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
    context.FlushPending();   // Artscout - 2026 (#79): submit while MoonTexture is bound to slot 0 (see DrawSun)
}


// Artscout - 2026: #DX12 п.5 -- re-draw the sun/moon for ONE eye in the per-slice VI tail, with THIS eye's off-axis
// frustum already armed (SetVRFrustum in the tail). The VI world pass suppresses them (g_bVrDeferCelestial) because
// the shared VS_Screen path writes one NDC to both slices -> under quad foveation the gaze-canted focus eyes then
// disagree and the sun DOUBLES / rides the gaze. Enables Z-TEST (read, no write) so the VI terrain/objects occlude
// the celestial bodies -- normally the sky is drawn depth-less BEFORE terrain, but here it comes AFTER, so occlusion
// must come from the depth buffer, not draw order.
void RenderOTW::VrDrawCelestial(void)
{
    if (realWeather->InsideOvercast() or realWeather->UnderOvercast()) return;
    // Match the STOCK sky path's render state EXACTLY (otw.cpp: Z DISABLED around DrawSky). An earlier attempt with
    // Z-test ON produced a dark square backing the sun in one eye (the glare quad's blend disagreed with the depth
    // state). Draw celestial depth-less like normal, then restore Z for the cockpit that follows.
    context.SetZBuffering(FALSE);
    context.SetState(MPR_STA_DISABLES, MPR_SE_Z_WRITE);
    context.SetState(MPR_STA_DISABLES, MPR_SE_Z_BUFFERING);
    bool save = g_bVrDeferCelestial; g_bVrDeferCelestial = false;
    if (TheTimeOfDay.ThereIsASun())  DrawSun();
    if (TheTimeOfDay.ThereIsAMoon()) DrawMoon();
    g_bVrDeferCelestial = save;
    if (DisplayOptions.bZBuffering)
    {
        context.SetZBuffering(TRUE);
        context.SetState(MPR_STA_ENABLES, MPR_SE_Z_WRITE);
        context.SetState(MPR_STA_ENABLES, MPR_SE_Z_BUFFERING);
    }
}


//============================================================================
// Artscout - 2026: #96 -- 3D SKYDOME. Draw the sky as WORLD geometry through the object path (VS_ObjectVI ->
// per-view gProj2 -> VI-correct per slice: no doubling / gaze-follow / focus-periphery seam). A gradient dome
// (haze horizon -> zenith blue, reusing the engine's per-frame sky_color/haze_sky_color) + sun/moon as camera-
// facing world billboards. Depth OFF, drawn FIRST as the background (terrain/objects then occlude it by draw
// order, exactly like the legacy 2D sky). Stars = phase 2 (a star texture mapped on the dome interior at night).
// Gated by g_b3DSky (OFF by default; the legacy 2D DrawSky stays the shipping path until confirmed in-headset).
//============================================================================
struct SkyDomeVert { float p[3]; float n[3]; unsigned long col; unsigned long spec; float tu, tv; };

static inline unsigned long PackSkyColor(float r, float g, float b, float a)
{
    if (r < 0) r = 0; if (r > 1) r = 1;
    if (g < 0) g = 0; if (g > 1) g = 1;
    if (b < 0) b = 0; if (b > 1) b = 1;
    if (a < 0) a = 0; if (a > 1) a = 1;
    unsigned R = (unsigned)(r * 255.0f), G = (unsigned)(g * 255.0f), B = (unsigned)(b * 255.0f), A = (unsigned)(a * 255.0f);
    return (A << 24) | (R << 16) | (G << 8) | B;   // 0xAARRGGBB, matches TGpuVert / the object vertex colour packing
}

// Camera-facing disc at world direction 'dir' (unit) * R, radius 'sz' world units, colour (r,g,b). Opaque bright
// centre fading to a transparent rim (a radial alpha gradient) -> a soft disc. Drawn in the alpha sky pass.
// Artscout - 2026: #96 shared atmospheric GLARE sprite (soft radial falloff, WHITE -- the caller tints it via the
// vertex colour). Both the sun and the moon need one: the halo is scattering in the AIR around the body, so its
// size is set by the atmosphere, NOT by the body's disc. (The moon's halo used to be built inside the moon block
// and scaled by the moon's own size, which is why shrinking the moon to its true 0.52deg also shrank the halo and
// the moon "lost brightness" -- the glow is what carries the perceived light.) Baked once, 64x64 is plenty: it is
// a smooth gradient stretched over several degrees.
static void* SkyGlareTex()
{
    static void* s_srv = 0;
    static bool  s_tried = false;
    if (!s_tried && g_pRenderer)
    {
        s_tried = true;
        const int gd = 64; unsigned* gg = new unsigned[(size_t)gd * gd];
        const float gc = (gd - 1) * 0.5f;
        for (int y = 0; y < gd; ++y)
            for (int x = 0; x < gd; ++x)
            {
                const float dx = x - gc, dy = y - gc;
                float rr = (float)sqrt(dx * dx + dy * dy) / gc;   // 0 centre .. 1 at mid-edge
                float a = 1.0f - rr; if (a < 0.0f) a = 0.0f;
                a = a * a;                                        // soft radial falloff
                // PREMULTIPLIED: the falloff lives in the RGB, not only in the alpha. The additive blend is
                // ONE/ONE (D3D12Renderer: SrcBlend=ONE, DestBlend=ONE), so alpha NEVER enters the equation --
                // a white sprite that fades only in alpha gets added at FULL white right out to the quad's
                // corners, which is exactly the "white translucent square around the sun/moon" bug. Fading the
                // RGB makes the edges add zero, so the quad has no visible boundary.
                const unsigned C = (unsigned)(255.0f * a + 0.5f);
                const unsigned A = (unsigned)(255.0f * a + 0.5f);   // kept sane in case it is ever alpha-blended
                gg[y * gd + x] = (A << 24) | (C << 16) | (C << 8) | C;   // R8G8B8A8: white premultiplied by the falloff
            }
        s_srv = (void*)g_pRenderer->LoadTextureRGBA(gg, gd, gd);
        delete[] gg;
    }
    return s_srv;
}

void RenderOTW::DrawSkyBillboard(const void* dirv, float R, float sz, float r, float g, float b, void* srv, float a,
                                 float flatten, bool additive)
{
    const float szY = sz * flatten;   // refraction squashes the disc vertically near the horizon (never magnifies)
    const Tpoint* dir = (const Tpoint*)dirv;
    if (!g_pRenderer) return;
    // Quad faces the camera: it is perpendicular to 'dir' (the camera sits at the dome centre, so the line to the
    // body IS dir). right = worldUp x dir; up = dir x right. worldUp = -Z (Falcon z=down).
    Tpoint right;
    right.x =  (-1.0f) * dir->y - 0.0f;   // (0,0,-1) x dir
    right.y =  0.0f - (-1.0f) * dir->x;
    right.z =  0.0f;
    float rl = (float)sqrt(right.x*right.x + right.y*right.y + right.z*right.z);
    if (rl < 1e-6f) { right.x = 1; right.y = 0; right.z = 0; rl = 1; }
    right.x /= rl; right.y /= rl; right.z /= rl;
    Tpoint up;
    up.x = dir->y * right.z - dir->z * right.y;
    up.y = dir->z * right.x - dir->x * right.z;
    up.z = dir->x * right.y - dir->y * right.x;

    const float cx0 = dir->x * R, cy0 = dir->y * R, cz0 = dir->z * R;   // disc centre (camera-relative)

    // TEXTURED quad (the game's sun.dds / moon.gif -- both carry alpha for the round shape). White vertex tint (the
    // texture supplies colour); alpha blend keys out the transparent corners. srv = ((TextureHandle*)TexHandle())->
    // m_pDDS (the backend D3D12Texture* SetTexture wants under D3D12; same conversion as drawparticlesys).
    // BLENDING, once and for all (an older note here claimed the shader "premultiplies colour by the vertex alpha"
    // and blamed it for a black moon disc -- that was WRONG twice over: the black circle was a HOLE at the dome's
    // apex, and the PS simply does `c *= t0` with no premultiply anywhere):
    //   alpha path (default): SrcBlend=SRC_ALPHA, DestBlend=INV_SRC_ALPHA -> alpha shapes the disc, as expected.
    //   additive path:        SrcBlend=ONE,       DestBlend=ONE           -> ALPHA IS NEVER READ. Anything meant to
    // fade an additive sprite (its radial falloff, a day/night dim) MUST live in the RGB. Fading only the alpha
    // adds the sprite at full white all the way to the quad's corners == the "white translucent square" bug.
    if (srv)
    {
        SkyDomeVert q[4]; unsigned short qi[6];
        const unsigned long qc = PackSkyColor(r, g, b, a);   // a<1 -> the disc washes out (pale daytime moon)
        for (int k = 0; k < 4; ++k)
        {
            const float ex = ((k & 1) ? 1.0f : -1.0f) * sz;
            const float ey = ((k & 2) ? 1.0f : -1.0f) * szY;
            SkyDomeVert& o = q[k];
            o.p[0] = cx0 + right.x*ex + up.x*ey;
            o.p[1] = cy0 + right.y*ex + up.y*ey;
            o.p[2] = cz0 + right.z*ex + up.z*ey;
            o.n[0]=0; o.n[1]=0; o.n[2]=-1; o.col = qc; o.spec = 0;
            o.tu = (k & 1) ? 1.0f : 0.0f;
            o.tv = (k & 2) ? 1.0f : 0.0f;
        }
        qi[0]=0; qi[1]=1; qi[2]=2; qi[3]=1; qi[4]=3; qi[5]=2;
        g_pRenderer->BeginSkyPass(true);   // alpha blend (texture alpha = round shape)
        g_pRenderer->SetProj((const float*)&CDXEngine::GetObjProjection());
        g_pRenderer->SetView((const float*)&CDXEngine::GetObjView());
        g_pRenderer->SetTexture(0, (struct ID3D11ShaderResourceView*)srv);
        // ADDITIVE for the atmospheric glare: scattered light ADDS to the sky behind it, it does not replace it.
        // Alpha-blending a glare just greys the sky out; adding it is what makes a body read as a light SOURCE.
        if (additive) g_pRenderer->SetObjectAdditiveBlend(true);
        g_pRenderer->DrawTerrainMesh(q, 4, qi, 6);
        if (additive) g_pRenderer->SetObjectAdditiveBlend(false);
        g_pRenderer->SetTexture(0, 0);     // clear slot 0 so a later dome/draw doesn't inherit the sun/moon texture
        return;
    }

    // SOLID OPAQUE disc (16-gon) FALLBACK (no texture). NOTE: an earlier alpha-fade version (rim alpha=0) rendered the MOON as a black
    // circle -- the object shader premultiplies colour by the vertex alpha, so the alpha=0 rim became colour*0 =
    // BLACK; a bright sun hid it but the dim grey moon read as a black disc. Uniform alpha=1 everywhere avoids it.
    const int NS = 16;
    SkyDomeVert d[1 + 16]; unsigned short di[16 * 3];
    const unsigned long cCol = PackSkyColor(r, g, b, 1.0f);
    d[0].p[0]=cx0; d[0].p[1]=cy0; d[0].p[2]=cz0; d[0].n[0]=0; d[0].n[1]=0; d[0].n[2]=-1;
    d[0].col = cCol; d[0].spec = 0; d[0].tu = 0.5f; d[0].tv = 0.5f;
    for (int k = 0; k < NS; ++k)
    {
        const float a = (float)(2.0 * PI) * (float)k / (float)NS;
        const float ex = (float)cos(a) * sz, ey = (float)sin(a) * szY;
        SkyDomeVert& o = d[1 + k];
        o.p[0] = cx0 + right.x*ex + up.x*ey;
        o.p[1] = cy0 + right.y*ex + up.y*ey;
        o.p[2] = cz0 + right.z*ex + up.z*ey;
        o.n[0]=0; o.n[1]=0; o.n[2]=-1; o.col = cCol; o.spec = 0; o.tu = 0; o.tv = 0;
    }
    int n = 0;
    for (int k = 0; k < NS; ++k) { di[n++] = 0; di[n++] = (unsigned short)(1 + k); di[n++] = (unsigned short)(1 + ((k + 1) % NS)); }

    g_pRenderer->BeginSkyPass(false);   // OPAQUE (no alpha premultiply-to-black); drawn over the dome, depth off
    g_pRenderer->SetProj((const float*)&CDXEngine::GetObjProjection());
    g_pRenderer->SetView((const float*)&CDXEngine::GetObjView());
    g_pRenderer->SetTexture(0, 0);      // textureless disc; sun/moon texture = phase 2
    g_pRenderer->DrawTerrainMesh(d, 1 + NS, di, NS * 3);
}

//=============================================================================
// Artscout - 2026: #13 volumetric clouds -- the CPU half. Everything visual happens in the PS (FF_CLOUD in
// ffemu.hlsl); this only picks the layers, feeds each slab, and rasterizes the geometry the march lives on.
//
// WHY A DISC AND NOT A FULLSCREEN RAYMARCH: the disc sits at the layer's real altitude, so the depth buffer
// sorts clouds against terrain by itself -- a ridge in front of the deck occludes it, a deck under the aircraft
// covers the ground -- with no scene-depth SRV, no MSAA depth resolve, and no per-view depth plumbing under
// view instancing. It also bounds the fill: only the layer's screen area marches, not the whole frustum. At the
// quad-view focus resolution (2740x2706, twice) a fullscreen march is simply not affordable here.
//
// Layer geometry: a camera-relative disc at the slab's MID altitude, rings spaced quadratically (near rings
// carry the parallax the eye reads; far rings only need to reach the horizon).
struct CloudLayerDesc
{
    float zTopW, zBotW;    // world z (DOWN-positive, feet); zTop is the MORE negative
    float coverage;        // 0..1
    float profile;         // 0 = stratus deck, 1 = cumulus (see CloudDensity in ffemu.hlsl)
};

static void DrawOneCloudSlab(const Tpoint& cam, const CloudLayerDesc& L,
                             float anchorX, float anchorY, const float sunDir[3], const float sunColor[3],
                             float steps)
{
    if (L.zBotW - L.zTopW < 1.0f) return;   // degenerate slab

    // Camera-relative: the shader marches from the origin (see the gCloud0 note in ffemu.hlsl).
    const float zTop = L.zTopW - cam.z;
    const float zBot = L.zBotW - cam.z;

    // NOTE there is deliberately NO "camera is inside the layer" special case any more. The first version bailed
    // out there (the disc it marched on sat at the slab's mid altitude and degenerated into a plane through the
    // eye), which meant clouds vanished outright between the layer's base and tops and you could not fly through
    // them. The sphere below has no such degeneracy: the camera is always strictly inside it.
    const float coverage = L.coverage;
    if (coverage < 0.02f) return;

    g_pRenderer->SetCloudParams(zTop, zBot, coverage, g_fCloudDensity,
                                anchorX, anchorY, g_fCloudScale, steps,
                                sunDir, g_fCloudAmbient, sunColor, g_fCloudPowder, cam.z, L.profile);

    // ---- the marching surface: a camera-centred SPHERE, back faces only ----
    // Its only job is to give every pixel exactly one march. Drawn front-culled (BeginCloudPass sets cull=FRONT),
    // so each ray leaves through exactly one back face -> one march per pixel, in every direction, whether the
    // camera is under, inside or above the layer. Radius just has to exceed the march's far clamp; nothing about
    // the cloud's position depends on it (the slab is analytic, in the shader).
    const int RINGS = 16, SEGS = 32;
    static SkyDomeVert v[(RINGS + 1) * (SEGS + 1)];
    static unsigned short idx[RINGS * SEGS * 6];
    static bool s_built = false;
    static int  s_nIdx = 0;
    const unsigned long white = PackSkyColor(1.0f, 1.0f, 1.0f, 1.0f);   // PS supplies the colour; vertex stays neutral
    if (!s_built)
    {
        s_built = true;
        const float R = 200000.0f;   // > the shader's 160k far clamp
        for (int r = 0; r <= RINGS; ++r)
        {
            const float phi = (float)PI * (float)r / (float)RINGS;        // 0..PI (pole to pole)
            const float sp = (float)sin(phi), cp = (float)cos(phi);
            for (int sg = 0; sg <= SEGS; ++sg)
            {
                const float th = (float)(2.0 * PI) * (float)sg / (float)SEGS;
                SkyDomeVert& o = v[r * (SEGS + 1) + sg];
                o.p[0] = R * sp * (float)cos(th);
                o.p[1] = R * sp * (float)sin(th);
                o.p[2] = R * cp;
                o.n[0] = 0; o.n[1] = 0; o.n[2] = -1;
                o.col = white; o.spec = 0; o.tu = 0; o.tv = 0;
            }
        }
        int n = 0;
        for (int r = 0; r < RINGS; ++r)
            for (int sg = 0; sg < SEGS; ++sg)
            {
                const unsigned short a = (unsigned short)( r      * (SEGS + 1) + sg);
                const unsigned short b = (unsigned short)((r + 1) * (SEGS + 1) + sg);
                idx[n++] = a; idx[n++] = b;                        idx[n++] = (unsigned short)(a + 1);
                idx[n++] = b; idx[n++] = (unsigned short)(b + 1);   idx[n++] = (unsigned short)(a + 1);
            }
        s_nIdx = n;
    }

    g_pRenderer->BeginCloudPass();
    g_pRenderer->SetProj((const float*)&CDXEngine::GetObjProjection());
    g_pRenderer->SetView((const float*)&CDXEngine::GetObjView());
    g_pRenderer->SetCameraPos(cam.x, cam.y, cam.z);
    g_pRenderer->SetTexture(0, 0);
    g_pRenderer->DrawTerrainMesh(v, (RINGS + 1) * (SEGS + 1), idx, s_nIdx);
    g_pRenderer->EndCloudPass();   // put the scene depth back to DEPTH_WRITE for whatever draws next
}

void RenderOTW::DrawVolumetricClouds(void)
{
    if (!g_bVolumetricClouds) return;
    if (!g_pRenderer || !g_pRenderer->IsValid()) return;
    if (!realWeather) return;

    Tpoint cam; viewpoint->GetPos(&cam);
    const int cond = realWeather->weatherCondition;

    //---- pick the layers ----------------------------------------------------
    CloudLayerDesc layers[2];
    int nLayers = 0;

    // CUMULUS. The sim only fills cumulusZ meaningfully at FAIR: cumulusBase starts at 0 (weather.cpp:35) and
    // ONLY the FAIR branch sets it (=80 -> cumulusZ ~ -8000..-8400 ft). Under SUNNY it is never set, so
    // cumulusZ lands at ~0..-400 ft -- ground level, i.e. garbage; under POOR/INCLEMENT it is whatever a
    // previous FAIR left behind. So: trust it at FAIR, use the knob otherwise (a fair-weather cumulus base of
    // ~5-6k ft is the honest default, and it is what makes a SUNNY sky stop being empty).
    if (cond <= INCLEMENT)
    {
        float baseZ;
        if (cond == FAIR && realWeather->cumulusZ < -1000.0f) baseZ = realWeather->cumulusZ;
        else                                                  baseZ = -g_fCumulusBase;

        float cov = g_fCumulusCoverage;
        if (cov < 0.0f)
        {
            if      (cond == SUNNY) cov = 0.26f;   // scattered fair-weather cu
            else if (cond == FAIR)  cov = 0.45f;   // the sim's own cumulus day
            else                    cov = 0.32f;   // broken cu under the overcast
        }
        if (cov > 0.02f)
        {
            layers[nLayers].zBotW    = baseZ;                       // flat condensation base
            layers[nLayers].zTopW    = baseZ - g_fCumulusThick;     // tops (more negative = higher)
            layers[nLayers].coverage = cov;
            layers[nLayers].profile  = 1.0f;                        // cumulus
            ++nLayers;
        }
    }

    // STRATUS deck. Hi/Lo/MidOvercast are only maintained when weatherCondition > FAIR; under SUNNY/FAIR they
    // hold STALE values (RefreshWeather's else-branch clears only the three bools). Gate on the condition, not
    // on the numbers.
    if (cond > FAIR)
    {
        float cov = g_fCloudCoverage;
        if (cov < 0.0f)
        {
            // ShadingFactor is ~5..10 for POOR/INCLEMENT (RealWeather::UpdateCondition) -> a believable deck
            // rather than a pinned-solid one.
            float sf = realWeather->ShadingFactor * 0.1f; if (sf < 0.0f) sf = 0.0f; if (sf > 1.0f) sf = 1.0f;
            cov = (cond >= INCLEMENT) ? (0.75f + 0.20f * sf) : (0.45f + 0.40f * sf);
        }
        if (cov > 0.02f)
        {
            layers[nLayers].zTopW    = realWeather->HiOvercast;
            layers[nLayers].zBotW    = realWeather->LoOvercast;
            layers[nLayers].coverage = cov;
            layers[nLayers].profile  = 0.0f;                        // stratus deck
            ++nLayers;
        }
    }
    if (nLayers == 0) return;

    //---- shared per-frame inputs -------------------------------------------
    // Wind scroll: the deck drifts. GetWindVector gives ft/sec; integrate it into a scrolling anchor. Folding the
    // camera's xy in here is what keeps the noise pinned to the WORLD while the march stays camera-relative.
    // Clock: GetTickCount, as the renderer already uses for gWaterParams. (vuxRealTime is declared with two
    // DIFFERENT types -- VU_TIME in vu.h, unsigned long in timerthread.h -- and this file includes neither.)
    static float s_windX = 0.0f, s_windY = 0.0f;
    static float s_lastT = -1.0f;
    const float nowT = (float)(GetTickCount() % 100000000u) * 0.001f;
    if (s_lastT < 0.0f) s_lastT = nowT;
    float dt = nowT - s_lastT; s_lastT = nowT;
    if (dt < 0.0f) dt = 0.0f; if (dt > 0.25f) dt = 0.25f;   // clamp hitches/wrap so the deck never teleports
    {
        const Tpoint w = realWeather->GetWindVector();   // returns BY VALUE
        s_windX += w.x * dt; s_windY += w.y * dt;
    }
    const float anchorX = cam.x + s_windX;
    const float anchorY = cam.y + s_windY;

    // Sun direction (toward the sun) + the light colour reaching the layer. At night the moon stands in, dimly:
    // an unlit deck reads as a black hole in the sky, which is worse than a slightly-too-bright one.
    Tpoint sunD; TheTimeOfDay.CalculateSunMoonPos(&sunD, FALSE);
    float sl = (float)sqrt(sunD.x*sunD.x + sunD.y*sunD.y + sunD.z*sunD.z);
    if (sl < 1e-6f) sl = 1.0f;
    // Artscout - 2026: sunD from CalculateSunMoonPos already points TOWARD the sun -- DrawSkyDome relies on
    // exactly that ("sunUp = -sunD.z", i.e. sunD.z is negative while the sun is up, and z is DOWN here). Negating
    // it, as this did, aimed the light march AWAY from the sun and lit the shadowed side. Caught in a RenderDoc
    // capture: gCloud2 came back as sunDir.z = +0.92, i.e. pointing at the ground, with the sun 67 deg UP.
    const float sunDir[3] = { sunD.x / sl, sunD.y / sl, sunD.z / sl };
    const float sunUp = -sunD.z / sl;                                        // >0 = above the horizon

    // Light colour: warm and dim near the horizon, white at noon; a cold floor at night (moonlight/skyglow).
    float day = sunUp * 3.0f; if (day < 0.0f) day = 0.0f; if (day > 1.0f) day = 1.0f;
    float warm = 1.0f - day;
    const float sunColor[3] = {
        0.10f + day * (1.00f - 0.10f) + warm * day * 0.20f,
        0.11f + day * (0.97f - 0.11f),
        0.16f + day * (0.92f - 0.16f) - warm * day * 0.15f,
    };

    // Artscout - 2026: the ceiling was 64, which silently ate every attempt to test a finer march -- a
    // `set g_fCloudSteps 8000` came out as 64 and looked like proof that step count was innocent. 2048 is
    // still a guard against a typo pinning the GPU, not a budget: reference implementations run ~500.
    float steps = g_fCloudSteps; if (steps < 4.0f) steps = 4.0f; if (steps > 2048.0f) steps = 2048.0f;

    //---- draw FAR to NEAR ---------------------------------------------------
    // The layers alpha-blend and do NOT write depth, so their mutual order is ours to get right: the farther
    // slab must go down first or it paints over the nearer one.
    if (nLayers == 2)
    {
        const float d0 = (float)fabs((layers[0].zTopW + layers[0].zBotW) * 0.5f - cam.z);
        const float d1 = (float)fabs((layers[1].zTopW + layers[1].zBotW) * 0.5f - cam.z);
        if (d1 > d0) { const CloudLayerDesc t = layers[0]; layers[0] = layers[1]; layers[1] = t; }
    }
    for (int i = 0; i < nLayers; ++i)
        DrawOneCloudSlab(cam, layers[i], anchorX, anchorY, sunDir, sunColor, steps);
}

void RenderOTW::DrawSkyDome(void)
{
    if (!g_b3DSky) return;
    if (!g_pRenderer || !g_pRenderer->IsValid()) return;
    // NOTE: no overcast early-out -- the dome always renders (its colours go greyish under overcast via sky_color;
    // an early return would leave a BLACK background). Fog / the cloud layer covers the inside-cloud case.

    Tpoint cam; viewpoint->GetPos(&cam);
    const float R = (g_fSkyDomeRadius > 1000.0f) ? g_fSkyDomeRadius : 200000.0f;

    Tpoint sunD; TheTimeOfDay.CalculateSunMoonPos(&sunD, FALSE);
    // Sun height above the horizon: sunUp = -sunD.z (z=down). UNCLAMPED so it goes NEGATIVE below the horizon (night).
    const float sunUp = -sunD.z;
    float sunElev = sunUp; if (sunElev < 0.0f) sunElev = 0.0f; if (sunElev > 1.0f) sunElev = 1.0f;   // clamped, moon gate
    // Dawn/dusk glow WINDOW: strongest at the horizon (sunUp ~ 0), fading to 0 both UP (high day sun -> the glow was a
    // false "second sun") and DOWN (deep-below sun -> the glow used to blaze at MIDNIGHT because the clamp lost the
    // below-horizon sign). Band ~ +/-0.30 (~17 deg) around the horizon.
    float glowWin = 1.0f - (float)fabs(sunUp) / 0.30f; if (glowWin < 0.0f) glowWin = 0.0f;

    // Artscout - 2026: #96 the equirect starmap's own celestial pole lands INSIDE the visible sky (the tilt puts
    // it at altitude == latitude, ~38deg for Korea). u varies arbitrarily fast around that singularity, so a
    // coarse ring turns it into a visible swirl/vortex. Denser rings+segments shrink it to a small patch; the
    // dome is a single draw of ~3.2k verts, so the cost is nil. (Exact fix = per-pixel equirect lookup in the
    // shader -- only worth it if the residual patch still shows.)
    const int SEGS = 96, RINGS = 32;
    static SkyDomeVert v[(RINGS + 1) * (SEGS + 1)];
    static unsigned short idx[RINGS * SEGS * 6];
    static bool s_idxBuilt = false;
    static int  s_nIdx = 0;

    // Artscout - 2026: #96 -- over these first radians above the horizon the dome blends from the TERRAIN's fog
    // colour (GetFogColor) into haze_sky_color. Keep it LOW: the haze is a thin band hugging the horizon, not a
    // gradient climbing the sky.
    const float GROUND_HAZE_BLEND = 0.05f;  // rad (~3 deg)
    // Glare HALF-sizes (fraction of the dome radius ~= radians). Set by the ATMOSPHERE, not by the body: both discs
    // are only ~0.5deg across, but their scattered aureoles span many degrees. Independent of *Size for that reason.
    const float SUN_GLARE_HALF  = 0.10f;    // ~11 deg across -- the sun's glare is what you cannot look at
    const float MOON_GLARE_HALF = 0.045f;   // ~5 deg across -- keeps the pre-shrink halo the moon read well with
    // CLOSED (2026-07-14): the additive glare once made the HERO explosion render as flat squares. It was never
    // the glare -- GetParticlePSO's cache key packed the MSAA sample count over the quad-view bit, so with
    // msaa samples="4" the 2-view and 4-view particle pipelines shared one key and whichever built first was
    // handed to the other. The glare's two extra draws merely shifted the caching order. Fixed in the key.
    // The sunset/sunrise glow must start essentially AT the horizon -- that is where dawn physically happens; the
    // haze there merely takes its colour. But the dome's glow is azimuth-dependent (sun side only) while the
    // terrain's fog is ONE flat colour per frame that cannot know direction, so a glow at el=0 re-opens the seam.
    // Compromise: fade the glow in over well under a degree -- it reads as "from the horizon", and the mismatched
    // sliver is shorter than the terrain's own mesh edge, so nothing shows.
    const float GLOW_HORIZON_FADE = 0.012f; // rad (~0.7 deg)
    // Stars are extinguished by the atmosphere near the horizon (the same air mass that reddens the moon), so they
    // must NOT shine through the haze band. Fade them out below ~14 deg.
    const float STAR_HORIZON_FADE = 0.25f;  // rad (~14 deg)
    const float elLo = -0.12f;              // ~ -7 deg (skirt below the horizon so the dome overlaps the terrain edge)
    const float elHi = (float)(PI * 0.5);   // 90 deg -- CLOSE the dome at the zenith. Capping short of the pole left a
                                            // hole at the top -> the black background showed through as a "black circle
                                            // always overhead" (mistaken for the moon). At el=90 the whole top ring
                                            // collapses to the zenith point, so each top segment gets one valid triangle
                                            // to the pole (the paired degenerate triangle is zero-area, harmless).
    int nv = 0;
    for (int ri = 0; ri <= RINGS; ++ri)
    {
        const float tt = (float)ri / (float)RINGS;
        const float el = elLo + (elHi - elLo) * tt;
        const float ce = (float)cos(el), se = (float)sin(el);
        float tc = el / 1.5708f; if (tc < 0.0f) tc = 0.0f; if (tc > 1.0f) tc = 1.0f;
        tc = (float)sqrt(tc);       // fatten the horizon band
        // Artscout - 2026: #96 HORIZON SEAM. The terrain fogs to GetFogColor() == haze_ground_color and the fog
        // SATURATES at 0.6*far_clip (otw.cpp), so the outer edge of the loaded tiles is a FLAT slab of ground-haze
        // colour. The dome used a DIFFERENT variable for its horizon (haze_sky_color) -> two flat colours butted
        // together = a razor-sharp line. And because the loaded tiles form a SQUARE, that visible edge rides up
        // (side) and down (corner) with azimuth, which reads as "the horizon TILTS when I turn my head" -- it is
        // not a tilt, it is the mesh boundary. Legacy could keep the two hazes apart because the old 2D sky never
        // touched the terrain; the 3D dome butts right against it. Fix: ANCHOR the dome to the ground fog colour
        // at el=0 and blend into the sky haze over the first few degrees -> the seam is unseeable by construction
        // and the mesh edge disappears with it. (The blend keeps the old look everywhere above the band.)
        const Tcolor* fogc = GetFogColor();
        float gh = el * (1.0f / GROUND_HAZE_BLEND); if (gh < 0.0f) gh = 0.0f; if (gh > 1.0f) gh = 1.0f;
        float gf = el * (1.0f / GLOW_HORIZON_FADE); if (gf < 0.0f) gf = 0.0f; if (gf > 1.0f) gf = 1.0f;  // glow ramp
        const float hr = fogc->r + (haze_sky_color.r - fogc->r) * gh;   // el<=0 -> exactly the terrain's fog colour
        const float hg = fogc->g + (haze_sky_color.g - fogc->g) * gh;
        const float hb = fogc->b + (haze_sky_color.b - fogc->b) * gh;
        const float cr = hr + (sky_color.r - hr) * tc;
        const float cg = hg + (sky_color.g - hg) * tc;
        const float cb = hb + (sky_color.b - hb) * tc;
        // Artscout - 2026: #96 starmap seam. atan2 jumps +PI -> -PI once per ring, so u leaps by 1.0 and the
        // triangle straddling the jump interpolates u BACKWARDS across the whole map -> a smeared band running
        // out of the celestial pole. Unwrap u along the ring (accumulate +-1 at the jump) so it stays
        // continuous; the sampler's WRAP addressing handles u outside 0..1.
        float ringOfs = 0.0f, prevRawU = 0.0f;
        for (int si = 0; si <= SEGS; ++si)
        {
            const float az = (float)(2.0 * PI) * (float)si / (float)SEGS;
            const float ca = (float)cos(az), sa = (float)sin(az);
            const float dx = ce * ca, dy = ce * sa, dz = -se;   // world dir (x=N, y=E, z=down -> up=-z)
            SkyDomeVert& o = v[nv++];
            o.p[0] = dx * R; o.p[1] = dy * R; o.p[2] = dz * R;
            o.n[0] = 0; o.n[1] = 0; o.n[2] = -1;
            // Sunset glow: warm the horizon on the sun's side (dot to sun, tight lobe, faded to zenith).
            float sdot = dx * sunD.x + dy * sunD.y + dz * sunD.z;
            if (sdot < 0) sdot = 0;
            // ...ramped in over GLOW_HORIZON_FADE (a fraction of a degree) rather than the full haze blend: dawn
            // physically starts AT the horizon, so a glow that only appears degrees up looks wrong. The ramp exists
            // solely because the terrain's flat fog colour cannot follow an azimuth-dependent glow -- keeping it
            // sub-degree makes the mismatch shorter than the mesh edge itself.
            const float glow = sdot * sdot * sdot * (1.0f - tc) * gf * glowWin * 0.6f;   // glowWin: only near dawn/dusk
            const float rr = cr + glow * (1.0f - cr);
            const float gg = cg + glow * (0.85f - cg) * 0.7f;
            const float bb = cb - glow * cb * 0.5f;
            o.col = PackSkyColor(rr, gg, bb, 1.0f);
            o.spec = 0;
            // Artscout - 2026: #96 equirect UV for the optional starmap. TILT the sampling direction about the East
            // axis (g_fSkyMapTilt turns * PI) so the map's celestial pole lifts OFF the zenith -> the Milky Way arcs
            // ACROSS the sky instead of lying on the horizon (with tilt=0 the pole is at the zenith and the celestial
            // equator/Milky Way sits on the horizon). Then standard equirect: u = azimuth (+rotate), v = 0.5 - alt/PI
            // (geometrically correct, no vertical stretch; visible hemisphere = the map half the pole faces).
            {
                // Auto tilt from the theater LATITUDE: the celestial pole sits at altitude == latitude, in the NORTH
                // (this rotation about the East axis moves the pole along the N-S meridian toward north). So with the
                // azimuth rotate at 0, Polaris lands due north at the right height -- no manual tuning needed.
                // g_fSkyMapTilt is an extra manual OFFSET on top. (Sidereal time / date-accurate rotation not done.)
                const float tl = ((90.0f - g_fLatitude) * (1.0f / 180.0f) + g_fSkyMapTilt) * (float)PI;
                const float ct = (float)cos(tl), st = (float)sin(tl);
                const float tx = dx * ct + dz * st;     // rotate (dx=north, dz=down) about East (+y)
                const float tz = -dx * st + dz * ct;
                const float ty = dy;
                const float rawU = 0.5f + (float)atan2(ty, tx) / (float)(2.0 * PI);
                if (si > 0)
                {
                    const float du = rawU - prevRawU;                 // unwrap across the atan2 branch cut
                    if      (du < -0.5f) ringOfs += 1.0f;
                    else if (du >  0.5f) ringOfs -= 1.0f;
                }
                prevRawU = rawU;
                o.tu = rawU + ringOfs + g_fSkyMapRotate;
                float up2 = -tz; if (up2 > 1.0f) up2 = 1.0f; if (up2 < -1.0f) up2 = -1.0f;
                float sv = 0.5f - (float)asin(up2) / (float)PI; if (sv < 0.0f) sv = 0.0f; if (sv > 1.0f) sv = 1.0f;
                o.tv = sv;
            }
        }
    }
    if (!s_idxBuilt)
    {
        int n = 0;
        for (int ri = 0; ri < RINGS; ++ri)
            for (int si = 0; si < SEGS; ++si)
            {
                const unsigned short a = (unsigned short)(ri * (SEGS + 1) + si);
                const unsigned short bb = (unsigned short)(a + 1);
                const unsigned short c = (unsigned short)(a + (SEGS + 1));
                const unsigned short d = (unsigned short)(c + 1);
                idx[n++] = a; idx[n++] = c; idx[n++] = bb;
                idx[n++] = bb; idx[n++] = c; idx[n++] = d;
            }
        s_nIdx = n; s_idxBuilt = true;
    }

    g_pRenderer->BeginSkyPass(false);
    g_pRenderer->SetProj((const float*)&CDXEngine::GetObjProjection());
    g_pRenderer->SetView((const float*)&CDXEngine::GetObjView());
    g_pRenderer->SetCameraPos(cam.x, cam.y, cam.z);
    g_pRenderer->SetTexture(0, 0);
    g_pRenderer->DrawTerrainMesh(v, nv, idx, s_nIdx);

    // Artscout - 2026: #96 STARS. (1) equirect STARMAP texture (terrdata\misctex\starmap.png/.jpg, Solar System Scope
    // celestial 8k) drawn ADDITIVE over the dome -> Milky Way + natural variation; (2) if no map file, fall back to the
    // engine's REAL star catalog (GetStarData) as tiny quads. Both fade by GetStarIntensity() (gone by day), VI-correct.
    const float sb = TheTimeOfDay.GetStarIntensity();
    static void* s_starmapSrv = 0;
    static bool  s_starmapTried = false;
    if (!s_starmapTried and g_pRenderer)
    {
        s_starmapTried = true;
        if (void* sm = (void*)g_pRenderer->LoadTextureFile("terrdata\\misctex\\starmap.png")) s_starmapSrv = sm;
        else if (void* sj = (void*)g_pRenderer->LoadTextureFile("terrdata\\misctex\\starmap.jpg")) s_starmapSrv = sj;
        else if (void* sk = (void*)g_pRenderer->LoadTextureFile("terrdata\\misctex\\starmap.dds")) s_starmapSrv = sk;
    }
    if (s_starmapSrv and sb > 0.003f)
    {
        // Re-draw the dome geometry (positions + equirect UVs from the dome build) with the starmap, ADDITIVE and
        // scaled by the star intensity so the Milky Way / stars ADD over the gradient and fade out by day.
        static SkyDomeVert vs[(RINGS + 1) * (SEGS + 1)];
        const float mb = (g_fSkyMapBright > 0.0f ? g_fSkyMapBright : 0.85f) * sb;
        // Artscout - 2026: #96 -- fade the stars OUT toward the horizon. This pass is ADDITIVE over the whole dome,
        // so a flat brightness let stars shine straight THROUGH the haze band, which never happens: near the
        // horizon the light crosses far more air and atmospheric extinction kills it (the same air mass that
        // reddens the moon). Vertices are laid out ring-major, so the ring index recovers this vertex's elevation.
        for (int k = 0; k < nv; ++k)
        {
            vs[k] = v[k];
            const int   ri2 = k / (SEGS + 1);
            const float el2 = elLo + (elHi - elLo) * ((float)ri2 / (float)RINGS);
            float sf = el2 * (1.0f / STAR_HORIZON_FADE); if (sf < 0.0f) sf = 0.0f; if (sf > 1.0f) sf = 1.0f;
            sf = sf * sf;                                   // hug the horizon: stars stay clear of the haze band
            const float m = mb * sf;
            vs[k].col = PackSkyColor(m, m, m, 1.0f);
        }
        g_pRenderer->BeginSkyPass(true);
        g_pRenderer->SetProj((const float*)&CDXEngine::GetObjProjection());
        g_pRenderer->SetView((const float*)&CDXEngine::GetObjView());
        g_pRenderer->SetCameraPos(cam.x, cam.y, cam.z);
        g_pRenderer->SetTexture(0, (struct ID3D11ShaderResourceView*)s_starmapSrv);
        g_pRenderer->SetObjectAdditiveBlend(true);
        g_pRenderer->DrawTerrainMesh(vs, nv, idx, s_nIdx);
        g_pRenderer->SetObjectAdditiveBlend(false);
        g_pRenderer->SetTexture(0, 0);
    }
    // Catalog fallback -- only when no starmap file is present.
    if (!s_starmapSrv and sb > 0.003f)
    {
        StarData* sd = TheTimeOfDay.GetStarData();
        if (sd and sd->coord and sd->totalcoord > 0)
        {
            const int MAXST = 4000;                          // idx are u16 -> keep 4*MAXST < 65536
            static SkyDomeVert    sv[MAXST * 4];
            static unsigned short sidx[MAXST * 6];
            const float ssz = (g_fSkyDomeStarSize > 0.0f ? g_fSkyDomeStarSize : 0.0011f) * R;
            const StarCoord* c = sd->coord;
            const int total = (sd->totalcoord < MAXST) ? sd->totalcoord : MAXST;
            int q = 0;
            for (int i = 0; i < total; ++i, ++c)
            {
                if (c->flag) continue;                       // behind the sun / moon
                if (c->z > -0.02f) continue;                 // up = -z: skip at/below the horizon
                float bright = (float)c->color * (1.0f / 255.0f) * sb;
                if (bright < 0.05f) continue;                // cull the dimmest
                if (bright > 1.0f) bright = 1.0f;

                // tiny quad on the dome at dir*R (orientation irrelevant at ~1px). right = (0,0,-1) x dir; up = dir x right.
                const float dxs = c->x, dys = c->y, dzs = c->z;
                float rx = -dys, ry = dxs, rz = 0.0f;
                float rl = (float)sqrt(rx * rx + ry * ry); if (rl < 1e-6f) { rx = 1.0f; ry = 0.0f; rl = 1.0f; }
                rx /= rl; ry /= rl;
                const float ux = dys * rz - dzs * ry, uy = dzs * rx - dxs * rz, uz = dxs * ry - dys * rx;
                const float cx = dxs * R, cy = dys * R, cz = dzs * R;
                const float aa = (float)sqrt(bright);        // compensate the object shader's colour*alpha premultiply
                const unsigned long col = PackSkyColor(1.0f, 1.0f, 1.0f, aa);
                const float s2 = ssz * (0.7f + 0.6f * bright);   // brighter stars a touch larger
                for (int k = 0; k < 4; ++k)
                {
                    const float ex = ((k & 1) ? 1.0f : -1.0f) * s2, ey = ((k & 2) ? 1.0f : -1.0f) * s2;
                    SkyDomeVert& o = sv[q * 4 + k];
                    o.p[0] = cx + rx * ex + ux * ey; o.p[1] = cy + ry * ex + uy * ey; o.p[2] = cz + rz * ex + uz * ey;
                    o.n[0] = 0; o.n[1] = 0; o.n[2] = -1; o.col = col; o.spec = 0; o.tu = 0; o.tv = 0;
                }
                const unsigned short b0 = (unsigned short)(q * 4);
                unsigned short* ip = &sidx[q * 6];
                ip[0] = b0; ip[1] = (unsigned short)(b0 + 1); ip[2] = (unsigned short)(b0 + 2);
                ip[3] = (unsigned short)(b0 + 1); ip[4] = (unsigned short)(b0 + 3); ip[5] = (unsigned short)(b0 + 2);
                if (++q >= MAXST) break;
            }
            if (q > 0)
            {
                g_pRenderer->BeginSkyPass(true);             // alpha blend over the gradient dome; drawn before sun/moon
                g_pRenderer->SetProj((const float*)&CDXEngine::GetObjProjection());
                g_pRenderer->SetView((const float*)&CDXEngine::GetObjView());
                g_pRenderer->SetCameraPos(cam.x, cam.y, cam.z);
                g_pRenderer->SetTexture(0, 0);
                g_pRenderer->DrawTerrainMesh(sv, q * 4, sidx, q * 6);
            }
        }
    }

    // Reuse the game's existing sun/moon textures via the object path. TexHandle() = a TextureHandle*; its ->m_pDDS
    // is the backend D3D12Texture* SetTexture wants (same conversion as drawparticlesys / dx2dengine). White tint =
    // texture as-is. Falls back to a solid disc if the texture is missing.
    if (TheTimeOfDay.ThereIsASun() and sunElev > 0.0f)   // sun above the horizon
    {
        DWORD_PTR sh = viewpoint->SunTexture.TexHandle();
        void* sSrv = sh ? (void*)((TextureHandle*)sh)->m_pDDS : 0;
        // Artscout - 2026: #96 SUN GLARE. The bare disc read as "not bright -- you can just look at it", the
        // opposite of reality. What blinds you is NOT the disc (a mere ~0.5deg) but the ATMOSPHERIC GLARE around
        // it: sunlight scattered by air over many degrees. We cannot out-shout a monitor's white, so we sell it
        // the way the eye reads it -- a large ADDITIVE halo that blooms over everything near the sun. Its size is
        // set by the air, so it does NOT scale with the disc. Extinction reddens+dims it toward the horizon (same
        // air-mass model as the moon), which is exactly why a low sun stops hurting to look at.
        const float sunSz = (g_fSkyDomeSunSize > 0.0f ? g_fSkyDomeSunSize : 0.010f);
        float gRd = 1.0f, gGd = 1.0f, gBd = 1.0f;   // disc tint = atmospheric extinction (set in the glare block)
        {
            const float sh    = (sunUp > 1.0f) ? 1.0f : sunUp;                 // sin(elevation), >0 here
            const float shDeg = (float)(asin(sh) * 180.0 / PI);
            float Xs = 1.0f / (sh + 0.50572f * (float)pow(shDeg + 6.07995, -1.6364));
            if (Xs < 1.0f) Xs = 1.0f; if (Xs > 12.0f) Xs = 12.0f;
            const float gR = (float)pow(10.0, -0.4 * 0.09 * (Xs - 1.0f));      // per-channel extinction: blue dies first
            const float gG = (float)pow(10.0, -0.4 * 0.14 * (Xs - 1.0f));
            const float gB = (float)pow(10.0, -0.4 * 0.22 * (Xs - 1.0f));
            gRd = gR; gGd = gG; gBd = gB;   // hand the extinction to the DISC below -- it is why the sun is yellow
            // OPEN (2026-07-15): the daytime aureole rings CYAN, and ADDITIVE is why -- it is not a tint bug.
            // With the sun high the extinction above returns ~1/1/1, so this adds white over a blue sky, and
            // additive clamps PER CHANNEL: sky (0.45,0.62,0.88) + w saturates BLUE at w>0.12 while green is still
            // climbing, leaving (0.45+w, 0.62+w, 1.0) -- cyan -- until green catches up at w>0.38.
            //   Warming the glare does NOT fix it (checked numerically before writing it): for red to saturate
            // first you need cB/cR < 0.22, which is an orange glare, not sunlight.
            //   The real fix is that an aureole does not ADD to the sky, it REPLACES it: forward-scattered
            // sunlight overwhelms the Rayleigh blue, which is why the sky near the sun is white. That is a lerp
            // toward white = PREMULTIPLIED blending (ONE / INV_SRC_ALPHA), which this texture is already built
            // for. Plain alpha (SRC_ALPHA / INV_SRC_ALPHA) squares the premultiplied falloff -- that is what made
            // the halo DARK on the earlier attempt. So the sky pass needs a premultiplied blend mode; it has only
            // BLEND_ALPHA and BLEND_ADDITIVE today. Left additive until that exists.
            void* glare = SkyGlareTex();
            if (glare)
                DrawSkyBillboard(&sunD, R, SUN_GLARE_HALF * R, gR, gG, gB, glare, 1.0f, 1.0f, /*additive*/ true);
        }
        // Artscout - 2026: the disc is ALPHA-blended. Drawing it ADDITIVE is a mistake already made once here:
        // additive is ONE/ONE, so ALPHA IS NEVER READ, and SUN.DDS is not premultiplied -- its transparent
        // corners then add at full strength and the "white square around the sun" comes straight back.
        //   The tint is the LEGACY sun's, recovered from the pre-skydome DrawSun (still live below for the 2D
        // path): DrawCelestialBody(&center, dist, 1.f, 0.984375f, 0.9765625f, 0.87109375f) -- 8-bit (252,250,223),
        // a MILD warm white. Do not re-guess this by eye; it was tuned against SUN.DDS, which is already yellow.
        //   Note the legacy sun had no aureole texture at all: its glare was the BILLBOARD SWELLING, dist =
        // SUN_DIST - (SUN_DIST - MOST_SUN_GLARE_DIST) * SunGlareValue, i.e. up to 2.5x wide when you look near it
        // (size ~ 1/dist). The skydome draws a separate glare billboard instead, above.
        static const float kSunLegacy[3] = { 0.984375f, 0.9765625f, 0.87109375f };
        float wq = g_fSunWarmth;
        if (wq < 0.0f) wq = 0.0f;
        else if (wq > 1.0f) wq = 1.0f;
        // Extinction multiplies on top: 1,1,1 at high sun (so the default IS the legacy tint, exactly), reddening
        // toward the horizon on the air-mass model rather than on taste.
        DrawSkyBillboard(&sunD, R, sunSz * R,
                         gRd * (1.0f + (kSunLegacy[0] - 1.0f) * wq),
                         gGd * (1.0f + (kSunLegacy[1] - 1.0f) * wq),
                         gBd * (1.0f + (kSunLegacy[2] - 1.0f) * wq), sSrv);
    }
    // Moon (twilight/night) -- same dome-billboard path as the sun (NOT the legacy 2D DrawMoon, which is dead under
    // VI). moon.gif is palettized; the object path has no palette LUT so BAKE it to RGBA ONCE: index -> palette RGB,
    // alpha from the palette high byte / chroma key, AND a radial mask that forces the corners transparent so the
    // moon is a round disc regardless of whether the source carries usable transparency. A degenerate result (chroma
    // swallowed the whole disc -> invisible; or fully opaque -> black square) is rebuilt from the radial mask alone.
    {
        Tpoint moonD; TheTimeOfDay.CalculateSunMoonPos(&moonD, TRUE);
        const float moonUp   = -moonD.z;                                     // world z = down -> up component
        // Draw the moon whenever it is above the horizon -- it is visible in daylight too (a pale daytime moon), and
        // the old sunElev<0.25 gate hid it any time the sun was more than ~14 deg up (why it kept "not showing").
        const bool  moonGate = TheTimeOfDay.ThereIsAMoon() and moonUp > 0.0f;

        if (moonGate)
        {
            static void* s_moonSrv = 0;
            static bool  s_moonTried = false;
            static bool  s_moonHalo = false;   // baked canvas carries a glow ring (billboard must cover 2x the disc)
            if (!s_moonTried && g_pRenderer)
            {
                s_moonTried = true;

                // Artscout - 2026: #96 high-res moon override. A user-supplied terrdata\misctex\moon.png (or .dds) --
                // a full-moon disc on a TRANSPARENT background -- wins over the low-res palettized moon.gif. Drawn
                // as-is at the moon size (no bake/halo; the asset carries its own detail + alpha). Absent -> gif path.
                if (void* hi = (void*)g_pRenderer->LoadTextureFile("terrdata\\misctex\\moon.png")) { s_moonSrv = hi; s_moonHalo = false; }
                else if (void* hd = (void*)g_pRenderer->LoadTextureFile("terrdata\\misctex\\moon.dds")) { s_moonSrv = hd; s_moonHalo = false; }

                // (The moon's halo now comes from the SHARED SkyGlareTex() -- see the draw below.)

                Texture& mt = viewpoint->OriginalMoonTexture;
                const int dim = mt.dimensions;
                const unsigned char* src = (const unsigned char*)mt.imageData;
                Palette* pal = mt.GetPalette();
                if (!s_moonSrv and dim > 0 and src and pal)   // only bake the gif when no high-res moon.png/.dds was loaded
                {
                    // Bake into a 2x canvas: the moon disc fills the CENTRE, a soft atmospheric GLOW fades outward to
                    // the canvas edge. A hard-edged opaque disc read as a "pasted sticker"; the feathered rim + halo
                    // make it sit in the sky naturally. Single textured billboard (alpha blend) -- no additive pass.
                    const unsigned chroma = (unsigned)(mt.chromaKey & 0xFFFFFFu);
                    const int   HALO = 2;
                    const int   cdim = dim * HALO;
                    const float cc   = (cdim - 1) * 0.5f;         // canvas centre
                    const float off  = (cdim - dim) * 0.5f;       // where the source disc starts in the canvas
                    unsigned* rgba = new unsigned[(size_t)cdim * cdim];
                    int opaque = 0;
                    for (int Y = 0; Y < cdim; ++Y)
                        for (int X = 0; X < cdim; ++X)
                        {
                            const int p = Y * cdim + X;
                            const float dx = X - cc, dy = Y - cc;
                            const float d  = (float)sqrt(dx * dx + dy * dy);

                            // Moon disc: opaque where the SOURCE texel is the moon (not the chroma background). The
                            // moon in moon.gif is smaller than the texture (chroma padding), so its real limb is well
                            // inside -- we key on the texel, not on a fixed radius.
                            // Moon coverage: BILINEAR sample of the source (anti-aliases the blocky 64px edge that
                            // read as a "stepped" ring) with a per-texel alpha. chroma -> 0; near-black limb / AA ring
                            // -> smoothstep-faded to 0 (else those dark, non-chroma pixels stayed opaque = a black
                            // ring); moon body -> 1. Premultiplied accumulation so the AA doesn't darken the edge.
                            const float sx = (float)X - off, sy = (float)Y - off;
                            const int   x0 = (int)floorf(sx), y0 = (int)floorf(sy);
                            const float fx = sx - x0, fy = sy - y0;
                            float aAcc = 0.0f, rAcc = 0.0f, gAcc = 0.0f, bAcc = 0.0f;
                            for (int j = 0; j < 2; ++j)
                                for (int i = 0; i < 2; ++i)
                                {
                                    const int xi = x0 + i, yi = y0 + j;
                                    const float w = (i ? fx : 1.0f - fx) * (j ? fy : 1.0f - fy);
                                    float ta = 0.0f, R = 0.0f, G = 0.0f, B = 0.0f;
                                    if (xi >= 0 and xi < dim and yi >= 0 and yi < dim)
                                    {
                                        const unsigned rgb = pal->paletteData[src[yi * dim + xi]] & 0xFFFFFFu;
                                        R = (float)((rgb >> 16) & 0xFFu); G = (float)((rgb >> 8) & 0xFFu); B = (float)(rgb & 0xFFu);
                                        if (rgb != chroma)
                                        {
                                            const float ml = (0.299f * R + 0.587f * G + 0.114f * B) / 255.0f;
                                            float s = (ml - 0.05f) / 0.13f; if (s < 0.0f) s = 0.0f; if (s > 1.0f) s = 1.0f;
                                            ta = s * s * (3.0f - 2.0f * s);   // smoothstep -> fade the near-black limb
                                        }
                                    }
                                    aAcc += w * ta; rAcc += w * ta * R; gAcc += w * ta * G; bAcc += w * ta * B;
                                }
                            const float mA = aAcc;                        // anti-aliased moon coverage 0..1
                            float mr = 205.0f, mg = 214.0f, mb = 235.0f;  // where transparent -> glow colour
                            if (aAcc > 1e-4f) { mr = rAcc / aAcc; mg = gAcc / aAcc; mb = bAcc / aAcc; }

                            // Glow halo: smooth radial falloff from the CENTRE outward -> no transparent gap between the
                            // moon limb and the glow (that gap also read as a black ring).
                            const float t  = d / cc;
                            float hA = (t < 1.0f) ? (1.0f - t) : 0.0f;
                            hA = hA * hA * 0.35f;

                            // Composite: moon OVER glow (lerp by coverage -> the faded limb shows glow, never black).
                            const float A  = (mA > hA) ? mA : hA;
                            const float gr = 205.0f, gg = 214.0f, gb = 235.0f;
                            const unsigned R8 = (unsigned)(gr + (mr - gr) * mA + 0.5f);
                            const unsigned G8 = (unsigned)(gg + (mg - gg) * mA + 0.5f);
                            const unsigned B8 = (unsigned)(gb + (mb - gb) * mA + 0.5f);
                            if (mA >= 0.5f) ++opaque;
                            const unsigned A8 = (unsigned)(255.0f * (A < 1.0f ? A : 1.0f) + 0.5f);
                            rgba[p] = (A8 << 24) | (B8 << 16) | (G8 << 8) | R8;   // R8G8B8A8 (R,G,B,A)
                        }
                    if (opaque > 0)
                    {
                        s_moonSrv = (void*)g_pRenderer->LoadTextureRGBA(rgba, cdim, cdim);
                        s_moonHalo = (s_moonSrv != 0);
                    }
                    delete[] rgba;
                }
            }
            // Billboard covers moon + halo. With the halo canvas the moon disc is the CENTRE half -> draw at 2x so the
            // disc keeps g_fSkyDomeMoonSize; the fallback solid disc (no srv) uses the size directly.
            const float mSize = (g_fSkyDomeMoonSize > 0.0f ? g_fSkyDomeMoonSize : 0.030f);
            const float sz    = s_moonHalo ? mSize * 2.0f : mSize;
            // Artscout - 2026: #96 day/night dimming. A bright full-glow moon in a blue DAY sky looked like a "Death
            // Star". Fade with the SUN elevation: full disc + halo at night; a PALE disc and NO glow by day. sunUp =
            // -sunD.z (sun elevation); mNight 1 at/below the horizon -> 0 once the sun is ~11 deg up.
            float mNight = 1.0f - sunUp / 0.20f; if (mNight < 0.0f) mNight = 0.0f; if (mNight > 1.0f) mNight = 1.0f;
            // Fade the moon by OPACITY, not brightness: keep the RGB LIGHT and drop the alpha so the daytime disc
            // WASHES OUT into the bright sky (pale, low-contrast -- a real daytime moon), instead of going dark grey.
            const float mA = 0.20f + 0.80f * mNight;   // opacity: 0.20 (pale daytime) .. 1.0 (night)
            // Artscout - 2026: the HALO gets its own, much sharper curve -- mNight's is for the DISC and is far too
            // generous for scattered light. The aureole IS scattered moonlight, and against a daylit sky it sits
            // orders of magnitude below it: physically it is gone the moment the sun is up, not "once the sun is
            // 11 deg up". mNight still returns 1.0 with the sun sitting exactly ON the horizon, which is why a
            // full-strength halo survived around a moon in a lit sky. Fade it across civil twilight instead:
            // full while the sun is below -6 deg (sunUp = -0.105), zero at sunrise (sunUp = 0), nothing after.
            float mDark = 1.0f - (sunUp + 0.105f) / 0.105f;
            if (mDark < 0.0f) mDark = 0.0f; if (mDark > 1.0f) mDark = 1.0f;
            const float mG = mDark * mDark;            // glow opacity: gone by sunrise

            // Artscout - 2026: #96 REAL horizon physics. The moon does NOT grow near the horizon -- that is the Moon
            // Illusion (purely perceptual). Geometrically it is ~1.7% SMALLER there (the observer sits one Earth
            // radius further away than at the zenith), so the size stays constant. What DOES happen, all driven by
            // the elevation, is modelled here:
            //   1) EXTINCTION + REDDENING -- the light crosses more air, and Rayleigh scattering removes blue first,
            //      so the disc dims AND turns orange/red. One physical model gives both: per-channel transmission
            //      t = 10^(-0.4*k*(X-1)), X = air mass, k = extinction coefficient (mag/airmass, blue >> red).
            //   2) REFRACTION FLATTENING -- air is denser lower down, so the lower limb is lifted more than the
            //      upper one and the disc squashes VERTICALLY into an oval (~0.8 at the horizon). Never magnifies.
            const float mh    = (moonUp >  1.0f) ? 1.0f : moonUp;             // sin(elevation)
            const float hDeg  = (float)(asin(mh) * 180.0 / PI);
            // Kasten-Young air mass (1 at the zenith, ~38 at the true horizon). Clamped: at the real horizon the
            // extinction is so severe the moon would go black, which reads as a bug rather than as physics.
            float X = 1.0f / (mh + 0.50572f * (float)pow(hDeg + 6.07995, -1.6364));
            if (X < 1.0f) X = 1.0f; if (X > 12.0f) X = 12.0f;
            const float kR = 0.09f, kG = 0.14f, kB = 0.22f;                   // mag/airmass: blue is scattered most
            const float tR = (float)pow(10.0, -0.4 * kR * (X - 1.0f));
            const float tG = (float)pow(10.0, -0.4 * kG * (X - 1.0f));
            const float tB = (float)pow(10.0, -0.4 * kB * (X - 1.0f));
            // Bennett refraction R(h) in arcmin; flatten = 1 + (R(top) - R(bottom)) / diameter  (both in degrees).
            const float dDeg  = (float)(2.0 * mSize * 180.0 / PI);            // angular DIAMETER of the disc
            float hTop = hDeg + dDeg * 0.5f, hBot = hDeg - dDeg * 0.5f; if (hBot < 0.0f) hBot = 0.0f;
            #define BENNETT_DEG(H) ((float)(1.0 / tan(((H) + 7.31 / ((H) + 4.4)) * PI / 180.0) / 60.0))
            float flat = 1.0f + (BENNETT_DEG(hTop) - BENNETT_DEG(hBot)) / dDeg;
            #undef BENNETT_DEG
            if (flat < 0.55f) flat = 0.55f; if (flat > 1.0f) flat = 1.0f;

            // Soft glow BEHIND the moon (hi-res PNG path only -- the gif fallback already bakes its halo into the disc).
            // Halo at its OWN angular size (NOT mSize * 2.6): the aureole is scattering in the AIR, so the air sets
            // its size. Tying it to the disc meant that correcting the moon to its true 0.52deg shrank the halo
            // 4x with it -- and since the halo is what carries the perceived light, the moon "went dim". ADDITIVE:
            // scattered moonlight adds to the night sky. Tinted by the same extinction as the disc.
            // mG (the day/night fade) must scale the RGB, NOT the alpha: the additive blend is ONE/ONE, so alpha
            // is never read -- passing mG as alpha would leave the halo at full strength in daylight.
            void* mglare = SkyGlareTex();
            if (s_moonSrv and !s_moonHalo and mG > 0.02f and mglare)
                DrawSkyBillboard(&moonD, R, MOON_GLARE_HALF * R,
                                 tR * 0.80f * mG, tG * 0.84f * mG, tB * 0.92f * mG,
                                 mglare, 1.0f, flat, /*additive*/ true);
            DrawSkyBillboard(&moonD, R, sz * R, 0.98f * tR, 0.98f * tG, 1.0f * tB, s_moonSrv, mA, flat);
        }
    }
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

    // Artscout - 2026 (VR): feed the current sky colour to the per-eye clear (D3D11Backend) so any
    // residual off-axis sky-band gap blends with the sky rather than a fixed colour.
    { extern float g_vrClearColor[3];
      g_vrClearColor[0] = sky_color.r; g_vrClearColor[1] = sky_color.g; g_vrClearColor[2] = sky_color.b; }
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
