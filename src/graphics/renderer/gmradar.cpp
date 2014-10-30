/***************************************************************************\
    GMRadar.cpp
    Scott Randolph
    November 15, 1996

    This class provides the ground mapping radar terrain display.
\***************************************************************************/
#include <cISO646>
#include <math.h>
#include "Tmap.h"
#include "TViewPnt.h"
#include "Tpost.h"
#include "DrawBSP.h"
#include "TerrTex.h"
#include "GMRadar.h"
#include "Graphics/DXEngine/DXEngine.h"
#include "Graphics/DXEngine/DXVBManager.h"

#pragma warning(disable : 4127)
#pragma warning(disable : 4706)

// The real data I have is:
// smooth water: -45.4 dB
// desert: -12.4 dB
// wooded area:  -7.4 dB
// cities:   0.6 dB
// I'm converting to a much more linear scale to make it "look right"
static const float reflectivity[] =
{
    0.0f, // UNUSED -- NO TYPE AVAILABLE
    0.1f, // COVERAGE_WATER
    0.1f, // COVERAGE_RIVER
    0.3f, // COVERAGE_SWAMP
    0.4f, // COVERAGE_PLAINS
    0.4f, // COVERAGE_BRUSH
    0.5f, // COVERAGE_THINFOREST
    0.6f, // COVERAGE_THICKFOREST
    0.7f, // COVERAGE_ROCKY
    1.0f, // COVERAGE_URBAN
    0.1f, // COVERAGE_ROAD
    1.0f, // COVERAGE_RAIL
    1.0f, // COVERAGE_BRIDGE
    0.1f, // COVERAGE_RUNWAY
    0.1f, // COVERAGE_STATION
};


// This establishes the 3D field of view used for over head views of targets
static const float FieldOfView = 4.0f * PI / 180.0f;
extern bool g_bGreyMFD;
bool bNVGmode = false;


void RenderGMRadar::Setup(ImageBuffer *imageBuffer)
{
    // Call our parents Setup code
    Render3D::Setup(imageBuffer);

    // Start our parameters marked uninitialized
    gain = 1.0f;
    LOD = -1;
    range = -1.0f;
    xformBuff = NULL;

    // Set our field of view for 3D renderings
    SetFOV(FieldOfView);
}


void RenderGMRadar::Cleanup(void)
{
    if (viewpoint)
    {
        viewpoint->Cleanup();
        delete viewpoint;
        viewpoint = NULL;
    }

    // Call our parent's cleanup
    Render3D::Cleanup();
}


/***************************************************************************\
 Do start of frame housekeeping
\***************************************************************************/
void RenderGMRadar::StartDraw(void)
{
    // DX - YELLOW BUG FIX - RED
    Render2D::StartDraw();
}


void RenderGMRadar::StartScene(Tpoint *from, Tpoint *at, float upHdg)
{
    float x, y, dx, dy;

    const float c = (float)cos(-upHdg);
    const float s = (float)sin(-upHdg);


    // Make sure we have our viewpoint setup appropriatly
    ShiAssert(viewpoint);

    // Clear the display
    ClearDraw();

    // Store our COA in units of level posts at the current LOD
    boxCenterRow = WORLD_TO_LEVEL_POST(at->x, LOD);
    boxCenterCol = WORLD_TO_LEVEL_POST(at->y, LOD);

    // Store our new parameters
    cameraPos = *from;
    centerPos = *at;
    SkipDraw = FALSE;


    // Update our viewpoint to be at the center of attention
    viewpoint->Update(at);

    // Compute the spherical light vector based on our direction of view
    ComputeLightAngles(from, at);

    // Compute the scale from world space to normalized window space
    worldToUnitScale = 1.0f / range;

    // Update the display rotation angle
    rotationAngle = -upHdg;

    // Compute where the COA will be drawn relative to display center
    dx = at->x - from->x; // This part just gets the vector
    dy = at->y - from->y; // from the viewer to the COA

    x = c * dx - s * dy; // Then we rotate the vector into
    y = s * dx + c * dy; // "heading up" space

    vOffset = 2.0f * worldToUnitScale * (x - range);
    hOffset = 2.0f * worldToUnitScale * y;
}


void RenderGMRadar::TransformScene(void)
{
    Tpost *post;
    int r, c;
    float scene_x;
    float scene_y;
    float x, y;
    GroundMapVertex *vert;
    int runLength;

    ShiAssert(viewpoint);

    // Compute the scaled rotation components for our platforms heading
    ScaledCOS = (float)cos(rotationAngle) * worldToUnitScale;
    ScaledSIN = (float)sin(rotationAngle) * worldToUnitScale;

    // Restrict our viewport to the region which will have visible radar imagery
    dCtrX = 0.0f;
    dCtrY = 0.0f;
    dScaleX = 1.0f;
    dScaleY = 1.0f;
    SetSubViewport(-1.0f, 1.0f, 1.0f, -1.0f);

    // Decide how big a patch about the COA we can draw right now
    drawRadius = min(boxSize, viewpoint->GetAvailablePostRange(LOD));

    if (drawRadius < 1)
    {
        SkipDraw = TRUE;
    }

    // Quit now if we don't have anything to draw
    if (SkipDraw)  return;


    // Figure out how long each row of points will be.
    runLength = drawRadius * 2 + 1;


    // If we still have an xformBuff laying around, free it.
    // This would happen if TransformScene we called twice without calling DrawScene
    if (xformBuff)
    {
        delete[] xformBuff;
        xformBuff = NULL;
    }


    // Get space to store the sample points
    xformBuff = new GroundMapVertex[ runLength * runLength ];
    ShiAssert(xformBuff);
    vert = xformBuff;


    // Traverse the scene data and rotate and light it
    for (r = boxCenterRow - drawRadius; r <= boxCenterRow + drawRadius; r++)
    {

        scene_x = LEVEL_POST_TO_WORLD(r, LOD) - centerPos.x;

        for (c = boxCenterCol - drawRadius; c <= boxCenterCol + drawRadius; c++)
        {

            scene_y = LEVEL_POST_TO_WORLD(c, LOD) - centerPos.y;

            // Get the post we want to draw now
            post = viewpoint->GetPost(r, c, LOD);
            ShiAssert(post);

            // Compute the reflected intensity based on the angel of incidence
            vert->r = vert->b = 0.0f;
            vert->a = 1;
            vert->g = ComputeReflectedIntensity(post);
            // FRB - grey display?
            //vert->r = vert->b = vert->g;

            // Compute the rotated and scaled location of the points
            // Note:  We're converting from FreeFalcon to normalized screen space
            // (positive y down screen in this case)
            y = -(scene_x * ScaledCOS - scene_y * ScaledSIN + dCtrY) * dScaleY;
            x = (scene_x * ScaledSIN + scene_y * ScaledCOS + dCtrX) * dScaleX;

            // Store this vertex's clip flags
            if (y < -1.0f)
            {
                vert->clipFlag = CLIP_TOP;
                ShiAssert(viewportYtoPixel(y) <= topPixel);
            }
            else if (y > 1.0f)
            {
                vert->clipFlag = CLIP_BOTTOM;
                ShiAssert(viewportYtoPixel(y) >= bottomPixel);
            }
            else
            {
                vert->clipFlag = ON_SCREEN;
                ShiAssert(viewportYtoPixel(y) <= bottomPixel);
                ShiAssert(viewportYtoPixel(y) >= topPixel);
            }

            if (x < -1.0f)
            {
                vert->clipFlag or_eq CLIP_LEFT;
                ShiAssert(viewportXtoPixel(x) <= leftPixel);
            }
            else if (x > 1.0f)
            {
                vert->clipFlag or_eq CLIP_RIGHT;
                ShiAssert(viewportXtoPixel(x) >= rightPixel);
            }
            else
            {
                ShiAssert(viewportXtoPixel(x) <= rightPixel);
                ShiAssert(viewportXtoPixel(x) >= leftPixel);
            }

            // Store the pixel location
            vert->x = viewportXtoPixel(x);
            vert->y = viewportYtoPixel(y);

            // Advance the vertex pointer
            vert++;
        }
    }
}


void RenderGMRadar::DrawScene(void)
{
    int r, c;
    GroundMapVertex *vTop, *vBot;
    int runLength;


    ShiAssert(viewpoint);


    // Clear the display
    ClearDraw();


    // Quit now if we don't have anything to draw
    if (SkipDraw)  return;

    // Setup the drawing state
    context.RestoreState(STATE_GOURAUD);

    // Figure out how long each row of points will be.
    runLength = drawRadius * 2 + 1;

    // We must already have the transformed data
    ShiAssert(xformBuff);
    vBot = xformBuff;
    vTop = vBot + runLength;


    // Traverse the scene data and draw it
    // Note:  We stop one row early since we're drawing each
    // triangle strip from the bottom up.
    for (r = boxCenterRow - drawRadius; r < boxCenterRow + drawRadius; r++)
    {

        for (c = boxCenterCol - drawRadius; c < boxCenterCol + drawRadius; c++, vBot++, vTop++)
        {

            // Draw each square separately for now
            // (could optimize to strips, but that makes clipping somewhat harder)
            DrawGMsquare(vBot, vTop, vTop + 1, vBot + 1);

        }

        // Skip the last vertex at the end of the row to get to the start of the next row
        vBot++;
        vTop++;
    }

    delete[] xformBuff;
    xformBuff = NULL;
}


// Called to draw roads and rivers into the radar scene.
// Can only be used when the LOD is at or better than the last near textured level.
// NOTE:  We trash this renderer's 2D transformation matrix
void RenderGMRadar::DrawFeatures(void)
{
    Tpost *post;
    int r, c;
    float scene_x;
    float scene_y;
    int drawRadius;
    int mask;
    int levelDifference;
    TextureID id;
    TexPath *path;


    ShiAssert(viewpoint);


    // Quit now if we don't have anything to draw
    if (SkipDraw)  return;

    // Quit now if we don't have detailed enough information
    levelDifference = TheMap.LastNearTexLOD() - LOD;

    if (levelDifference < 0)  return;

    // Use our entire available area when looking for features (parent posts may be off screen)
    drawRadius = viewpoint->GetAvailablePostRange(LOD);

    // Quit now if we don't have enough data to do anything reasonable
    if (drawRadius < 1)
    {
        return;
    }

    // Setup the drawing state
    SetColor(0xFF000000);

    // Construct the mask to test if a post falls on a texture boundry
    mask = compl ((compl 0 >> levelDifference) << levelDifference);

    // Load the display's 2D transformation matrix
    // Note that we're swapping x and y to get into the system VirtualDisplay expects
    dmatrix.rotation00 = ScaledSIN * dScaleX;
    dmatrix.rotation01 =  ScaledCOS * dScaleX;
    dmatrix.rotation10 = ScaledCOS * dScaleY;
    dmatrix.rotation11 = -ScaledSIN * dScaleY;


    // Step across all the posts which fall on texture boundries
    // Note:  we stop one row and one column early, since we're drawing
    // things as areas, not edges.
    for (r = boxCenterRow - drawRadius; r < boxCenterRow + drawRadius; r++)
    {

        if (r bitand mask)  continue; // Could take care of this in loop control

        scene_x = LEVEL_POST_TO_WORLD(r, LOD) - centerPos.x;

        for (c = boxCenterCol - drawRadius; c < boxCenterCol + drawRadius; c++)
        {

            if (c bitand mask)  continue; // Could take care of this in loop control

            scene_y = LEVEL_POST_TO_WORLD(c, LOD) - centerPos.y;

            // Load the display's 2D translation vector
            // Note that we're swapping x and y to get into the system VirtualDisplay expects
            dmatrix.translationX = (scene_x * ScaledSIN + scene_y * ScaledCOS + dCtrX) * dScaleX;
            dmatrix.translationY = (scene_x * ScaledCOS - scene_y * ScaledSIN + dCtrY) * dScaleY;

            // Get the texture id of the post we're dealing with
            post = viewpoint->GetPost(r, c, LOD);
            ShiAssert(post);
            id = post->texID;

            // Step through all the paths we want to drawn
            int pathType = COVERAGE_ROAD;
            int offset;

            while (TRUE)
            {

                // Step through the paths of this type
                offset = 0;
                path = TheTerrTextures.GetPath(id, pathType, offset++);

                while (path)
                {
                    Line(path->x1, path->y1, path->x2, path->y2);
                    path = TheTerrTextures.GetPath(id, pathType, offset++);
                }

                // Move on to the next path type of interest
                if (pathType == COVERAGE_ROAD)
                {
                    pathType = COVERAGE_RIVER;
                }
                else if (pathType == COVERAGE_RIVER)
                {
                    pathType = COVERAGE_RAIL;
                }
                else
                {
                    break;
                }
            }
        }
    }

    // Remove the feature specific rotation and translation settings
    ZeroRotationAboutOrigin();
    CenterOriginInViewport();
}


void RenderGMRadar::PrepareToDrawTargets(void)
{
    float dx, dy;
    Tpoint location;
    Trotation down;
    Tpoint up, right;

    // Compute where the COA will be drawn relative to display center
    dx = centerPos.x - cameraPos.x; // This part just gets the vector
    dy = centerPos.y - cameraPos.y; // from the viewer to the COA


    // Set the 3D camera for overhead images
    location.x = centerPos.x;
    location.y = centerPos.y;
    location.z = -range / (float)tan(FieldOfView / 2.0f);

    // Setup a downward looking matrix with the appropriate "roll"
    up.x = -ScaledCOS / worldToUnitScale;
    up.y =  ScaledSIN / worldToUnitScale;
    up.z =  0.0f;

    right.x =  up.y;
    right.y = -up.x;
    right.z =  0.0f;

    down.M11 = 0.0f;
    down.M12 = right.x;
    down.M13 = up.x;
    down.M21 = 0.0f;
    down.M22 = right.y;
    down.M23 = up.y;
    down.M31 = 1.0f;
    down.M32 = right.z;
    down.M33 = up.z;

    // SetFar( -2.0f * location.z );
    TheStateStack.SetContext(&context);
    TheStateStack.SetCameraProperties(oneOVERtanHFOV, oneOVERtanVFOV, scaleX, scaleY, shiftX, shiftY);
    TheStateStack.SetLODBias(resRelativeScaler);
    TheStateStack.SetTextureState(FALSE);
    TheColorBank.SetColorMode(ColorBankClass::UnlitGreenMode);
    SetFOV(FieldOfView);
    TheDXEngine.SaveState();
    TheDXEngine.SetState(DX_DBS);
    SetCamera(&location, &down);
}


void RenderGMRadar::FlushDrawnTargets(void)
{
    this->context.FlushPolyLists();
    TheDXEngine.RestoreState();
}


void RenderGMRadar::DrawBlip(float worldX, float worldY)
{
    float x,  y;
    float dx, dy;

    // Quit now if we don't have anything to draw
    if (SkipDraw)  return;

    // Compute the rotated and scaled location of the points
    // Note:  We're converting from FreeFalcon to normalized screen space
    // (positive y down screen in this case)
    dx = worldX - centerPos.x;
    dy = worldY - centerPos.y;

    y = -(dx * ScaledCOS - dy * ScaledSIN + dCtrY) * dScaleY;
    x = (dx * ScaledSIN + dy * ScaledCOS + dCtrX) * dScaleX;

    x = viewportXtoPixel(x);
    y = viewportYtoPixel(y);

    //Clip test
    if ((x + 1.0f <= rightPixel)  and 
        (x      >= leftPixel)   and 
        (y + 1.0f <= bottomPixel) and 
        (y      >= topPixel))
    {
        SetColor(0xFF00FF00);
        Render2DPoint(x,      y);
        Render2DPoint(x,      y + 1.0f);
        Render2DPoint(x + 1.0f, y);
        Render2DPoint(x + 1.0f, y + 1.0f);
    }
}


void RenderGMRadar::DrawBlip(DrawableObject* drawable, float GainScale, bool Shaped)
{
    float x,  y;
    float dx, dy;
    float r;

    // Quit now if we don't have anything to draw
    if (SkipDraw)  return;

    // This is the radius of the objects footprint in pixels.
    if (drawable->GetRadarSign() == 0.0f)
        r = drawable->Radius() * worldToUnitScale * scaleX;
    else
        r = drawable->GetRadarSign() * worldToUnitScale * scaleX; // +/-1 * scale to pixels

    // Decide if a spot will suffice or if we need to do a full render
    if ( not Shaped or r < 2.0f)
    {

        // Compute the rotated and scaled location of the points
        // Note:  We're converting from FreeFalcon to normalized screen space
        // (positive y down screen in this case)
        dx = drawable->X() - centerPos.x;
        dy = drawable->Y() - centerPos.y;

        y = -(dx * ScaledCOS - dy * ScaledSIN + dCtrY) * dScaleY;
        x = (dx * ScaledSIN + dy * ScaledCOS + dCtrX) * dScaleX;

        x = viewportXtoPixel(x);
        y = viewportYtoPixel(y);

        //Clip test
        if (
            (x + 1.0f >= rightPixel)  or
            (x      <= leftPixel)   or
            (y + 1.0f >= bottomPixel) or
            (y      <= topPixel)
        )
            return;

        float BlitColor = r * 32.0f * gain * GainScale;

        if (BlitColor > 255.0f) BlitColor = 255.0f;

        SetColor(0xFF000000 bitor (F_I32(BlitColor) << 8));
        Render2DPoint(x,      y);

        BlitColor /= 4.0f; //r * 64.0f * gain;
        SetColor(0xFF000000 bitor (F_I32(BlitColor) << 8));

        if (r > 1.0f)
        {
            Render2DPoint(x,      y + 1.0f);
            Render2DPoint(x + 1.0f, y);
            Render2DPoint(x + 1.0f, y + 1.0f);
        }
    }
    else
    {
        r *= gain * GainScale;

        if (r > 255.0f) r = 255.0f;

        TheDXEngine.SetBlipIntensity(r);
        drawable->Draw(this);
    }
}


void RenderGMRadar::FinishScene(void)
{
    // Remove the viewport restriction
    ClearSubViewport();

}


void RenderGMRadar::SetViewport(float l, float t, float r, float b)
{
    Render2D::SetViewport(l, t, r, b);

    prevLeft = left;
    prevRight = right;
    prevTop = top;
    prevBottom = bottom;
}


void RenderGMRadar::SetSubViewport(float l, float t, float r, float b)
{
    float newLeft, newRight, newTop, newBottom;
    const float halfXres = xRes * 0.5f;
    const float halfYres = yRes * 0.5f;

    newLeft = (l * scaleX + shiftX - halfXres) / halfXres;
    newRight = (r * scaleX + shiftX - halfXres) / halfXres;
    newTop = -(-t * scaleY + shiftY - halfYres) / halfYres;
    newBottom = -(-b * scaleY + shiftY - halfYres) / halfYres;

    Render2D::SetViewport(newLeft, newTop, newRight, newBottom);
}


void RenderGMRadar::ClearSubViewport(void)
{
    Render2D::SetViewport(prevLeft, prevTop, prevRight, prevBottom);
}


// Given a range and LOD, determine what if any changes we need to make to our TViewPoint
BOOL RenderGMRadar::SetRange(float newRange, int newLOD)
{
    TViewPoint *oldViewpoint;

    // Quit now if we don't need to make a change (really shouldn't happen)
    if ((LOD == newLOD) and (newRange == range))
    {
        // No change.  We didn't have to refresh our viewpoint
        return FALSE;
    }

    // We check this to make sure we're not doing too much work as a result
    // of a floating point miscompare
    ShiAssert((LOD not_eq newLOD) or (fabs(newRange - range) > 1.0f))


    // Get us to a known starting state (constructed but uninitialized TViewPoint)
    oldViewpoint = viewpoint;
    viewpoint = new TViewPoint;

    // Store our new parameters
    // We take the patch size (newRange) and divide by two to get radial distance.
    // Then we apply a correction (sqrt(2) = 1.414) to get maximal diagonal range.
    range = newRange;
    float diagonalRange = newRange * 1.414f;
    LOD = newLOD;
    boxSize = max(WORLD_TO_LEVEL_POST(diagonalRange, LOD), 1);

    // Build the range array needed to initialize our viewpoint
    // Note that we're asking for at least one near textured post in each direction
    float *fetchRanges;
    fetchRanges = new float[LOD + 1];
    ShiAssert(fetchRanges);
    fetchRanges[LOD] = max(diagonalRange, LEVEL_POST_TO_WORLD(1, TheMap.LastNearTexLOD()));

    // Setup the viewpoint for the requested detail level
    ShiAssert(viewpoint);
    viewpoint->Setup(LOD, LOD, fetchRanges);

    // Cleanup the old viewpoint (if we had one)
    if (oldViewpoint)
    {
        oldViewpoint->Cleanup();
        delete oldViewpoint;
    }

    delete[] fetchRanges;

    // We DID have to refresh our viewpoint
    return TRUE;
}


// NOTE:  To do lighting calculations, we will need to do a dot product
// between the light vector and the surface normal.  For this to work,
// light vector must be pointing from the surface TO the light source.  Thus,
// we are storing the angles for the light vector pointing TOWARD the "from"
// point.
void RenderGMRadar::ComputeLightAngles(Tpoint *from, Tpoint *at)
{
    float dx, dy, dz, r;

    dx = from->x - at->x;
    dy = from->y - at->y;
    dz = from->z - at->z;
    r  = (float)sqrt(dx * dx + dy * dy);

    lightTheta = (float)atan2(dy, dx);
    lightPhi = (float)atan2(-dz, r);

    ShiAssert(lightPhi <= PI * 0.5f);
}


float RenderGMRadar::ComputeReflectedIntensity(Tpost *post)
{
    float cosAngle;
    float I;

    if ( not post)
        return 0.0f;

    cosAngle = (float)sin(lightPhi) * (float)sin(post->phi);
    cosAngle *= (float)cos(lightPhi) * (float)cos(post->phi) * (float)cos(lightTheta - post->theta);


    // Use the type to lookup a reflectance value

    // FRB - pre-RV
    //if (LOD <= TheMap.LastNearTexLOD()) {
    // BYTE type = TheTerrTextures.GetTerrainType( post->texID );
    // I = cosAngle * reflectivity[type] * gain;
    //} else {
    // I = cosAngle * reflectivity[COVERAGE_WATER] * gain;
    //}


    // if (LOD <= TheMap.LastNearTexLOD()) {
    BYTE type = TheTerrTextures.GetTerrainType(post->texID);
    I = cosAngle * reflectivity[type] * gain;
    /* } else {
     I = cosAngle * reflectivity[COVERAGE_WATER] * gain;
     }
    */

    // Clamp the intensity to 0 to 1
    if (I <= 0.0f)
    {
        return 0.0f;
    }

    //if (I > 1.0f) {
    // return 1.0f;
    //}

    //Cobra 1.0f was too high and washed out the map
    if (I > 0.5)
        return 0.5f;

    //if (I > 0.25)
    // return 0.25f;

    return I;
}


void RenderGMRadar::DrawGMsquare(GroundMapVertex *v0, GroundMapVertex *v1, GroundMapVertex *v2, GroundMapVertex *v3)
{
    TwoDVertex vert[4];
    TwoDVertex* vertPointers[4];

    ShiAssert(v0);
    ShiAssert(v1);
    ShiAssert(v2);
    ShiAssert(v3);

    if (v0->clipFlag bitor v1->clipFlag bitor v2->clipFlag bitor v3->clipFlag)
    {

        // Convert the structure format
        vert[0].x = v0->x, vert[0].y = v0->y, vert[0].clipFlag = v0->clipFlag, vert[0].g = v0->g, vert[0].r = vert[0].b = 0.0f;
        vert[1].x = v1->x, vert[1].y = v1->y, vert[1].clipFlag = v1->clipFlag, vert[1].g = v1->g, vert[1].r = vert[1].b = 0.0f;
        vert[2].x = v2->x, vert[2].y = v2->y, vert[2].clipFlag = v2->clipFlag, vert[2].g = v2->g, vert[2].r = vert[2].b = 0.0f;
        vert[3].x = v3->x, vert[3].y = v3->y, vert[3].clipFlag = v3->clipFlag, vert[3].g = v3->g, vert[3].r = vert[3].b = 0.0f;

        // Setup the pointers
        vertPointers[0] = &vert[0];
        vertPointers[1] = &vert[1];
        vertPointers[2] = &vert[2];
        vertPointers[3] = &vert[3];

        ClipAndDraw2DFan(vertPointers, 4);

    }
    else
    {

        // OW
#if 0
        context.Primitive(MPR_PRM_TRIFAN, MPR_VI_COLOR, 4, sizeof(MPRVtxClr_t));

        // MPRVtxClr_t
        context.StorePrimitiveVertexData(v0);
        context.StorePrimitiveVertexData(v1);
        context.StorePrimitiveVertexData(v2);
        context.StorePrimitiveVertexData(v3);
#else
        context.DrawPrimitive(MPR_PRM_TRIFAN, MPR_VI_COLOR, 4, (MPRVtxTexClr_t **) &v0);
#endif
    }
}
