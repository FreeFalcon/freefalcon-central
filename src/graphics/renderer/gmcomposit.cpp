/***************************************************************************\
    GMComposite.cpp
    Scott Randolph
    August 5, 1998

    This class provides the ground mapping radar image composition and
 beam sweep.
\***************************************************************************/
#include <math.h>
#include "Edge.h"
#include "GMRadar.h"
#include "GMComposit.h"
#include "Falclib/include/dispcfg.h"
#include "FalcLib/include/playerop.h"
#include "FalcLib/include/dispopts.h"

//MI
extern bool g_bRealisticAvionics;
extern bool g_bAGRadarFixes;

static const int MAX_CLIP_EDGES = 3;
static const int MAX_CONSTRUCTION_VERTS = 2 * MAX_CLIP_EDGES; // Create 2 per edge
static const int MAX_VERTEX_LIST = 4 + MAX_CLIP_EDGES; // Add 2 remove at least 1 per edge

// These are global to save some parameter passing, etc.
// This implies that only one thread can call DrawOutput at a time.
// (which is true anyway because MPR is single threaded)
static Edge beam;
static Edge leftLimit;
static Edge rightLimit;
static TwoDVertex v0, v1, v2, v3;
static TwoDVertex *vertArray[MAX_VERTEX_LIST];
static TwoDVertex c[MAX_CONSTRUCTION_VERTS];
static int NumCverts;


// This list controls the distribution of work to generate each new
// frame of radar return imagery.
enum OpName { Start, Xform, Ground, Features, Targets, Finish, Replace };
struct OpRecord
{
    enum OpName operation;
    int percent;
};

static const struct OpRecord OperationList[] =
{
    {Start, 15},
    {Xform, 30},
    {Ground, 45},
    {Features, 50},
    {Targets, 75},
    {Finish, 90},
    {Replace, 101}
};


// Helper function defined at the bottom of this file
static inline void Intersect(TwoDVertex *v1, TwoDVertex *v2, TwoDVertex *c, float t);
static int ClipToBeamAndLimits(void);

// Used to add random noise to the radar images
static int random;
static inline BYTE Noise(int input)
{
    // return input;

    random = random * 214013L + 2531011L; // Stolen from C Runtime RAND() function

    input = input + ((random >> 16) bitand 0x3F);

    // Gee, I sure whould like to get rid of this if...
    return (BYTE)min(input, 0xFF);
}


RenderGMComposite::RenderGMComposite()
{
    lTexHandle = rTexHandle = nTexHandle = NULL;
    m_pRenderTarget = NULL;
    m_bRenderTargetOwned = false;
    m_pBackupBuffer = NULL;
    m_pRenderBuffer = NULL;
    nextOperation = NULL;
};

void RenderGMComposite::Setup(ImageBuffer *output, void(*tgtDrawCallback)(void*, RenderGMRadar*, bool), void *tgtDrawParam)
{
    // Call our parents Setup code
    Render2D::Setup(output);

    // Set some default values
    range = 0.0f;
    gimbalLimit = 60.0f * PI / 180.0f;
    prevBeamPercent = 0;
    prevBeamRight = FALSE;
    lOriginX = lOriginY = 0;
    rOriginX = rOriginY = 0;
    rAngle = lAngle = 0;
    DrawChanged = true;


    // Store the callback funcion pointer for drawing targets
    ShiAssert(tgtDrawCallback);
    tgtDrawCB = tgtDrawCallback;
    tgtDrawCBparam = tgtDrawParam;

    if (DisplayOptions.bRender2Texture)
    {
        // Set up our private rendering target image
        MPRSurfaceType front = FalconDisplay.theDisplayDevice.IsHardware() ? VideoMem : SystemMem;
        m_pRenderTarget = new ImageBuffer;
        m_pRenderTarget->Setup(output->GetDisplayDevice(), GM_TEXTURE_SIZE, GM_TEXTURE_SIZE, front, None);
        m_bRenderTargetOwned = true;

        // Set up our child RenderGMRadar
        radar.Setup(m_pRenderTarget);
    }

    else
    {
        // Welcome to the heart of darkness
        // The whole (ugly) SetRenderTarget workaround is necessary because 1) the Voodoo 1 and 2 completely
        // ignore SetRenderTarget and 2) the SetRenderTarget implementation in many drivers (including some
        // versions of the detonator drivers) is broken ;(

        // Use primary
        // m_pRenderTarget = new ImageBuffer;
        // m_pRenderTarget->AttachSurfaces(output->GetDisplayDevice(), output->targetSurface(), NULL);
        m_pRenderTarget = output;
        m_bRenderTargetOwned = false;

        // Setup backup buffer (let the driver decide where to put it - Note: on the V2 we are counting on the driver choosing system memory)
        m_pBackupBuffer = new ImageBuffer;
        m_pBackupBuffer->Setup(output->GetDisplayDevice(), GM_TEXTURE_SIZE, GM_TEXTURE_SIZE, None, None);

        // Setup render buffer (contains a backup of ther rendered image)
        m_pRenderBuffer = new ImageBuffer;
        m_pRenderBuffer->Setup(output->GetDisplayDevice(), GM_TEXTURE_SIZE, GM_TEXTURE_SIZE, None, None);

        // Here we fool the radar and make it thing it renders to a 128x128 surface
        radar.Setup(m_pRenderBuffer);
        // and now we are using a back door to make it actually use the primary surface (geez, ugly shit)
        IDirectDrawSurface7 *lpDDSBack = m_pRenderTarget->targetSurface();
        radar.context.NewImageBuffer((UInt) lpDDSBack);
    }

    paletteHandle = new PaletteHandle(context.m_pCtxDX->m_pDD, 32, 256);
    ShiAssert(paletteHandle);

    lTexHandle = new TextureHandle;
    ShiAssert(lTexHandle);
    lTexHandle->Create("GM Radar Left", 0, 0,
                       GM_TEXTURE_SIZE, GM_TEXTURE_SIZE, TextureHandle::FLAG_HINT_DYNAMIC bitor TextureHandle::FLAG_MATCHPRIMARY |
                       TextureHandle::FLAG_NOTMANAGED);

    rTexHandle = new TextureHandle;
    ShiAssert(rTexHandle);
    rTexHandle->Create("GM Radar Right", 0, 0,
                       GM_TEXTURE_SIZE, GM_TEXTURE_SIZE, TextureHandle::FLAG_HINT_DYNAMIC bitor TextureHandle::FLAG_MATCHPRIMARY |
                       TextureHandle::FLAG_NOTMANAGED);

    // Noise texture is twice as wide as the other textures to allow for random u (avoids time consuming re-generation)
    nTexHandle = new TextureHandle;
    ShiAssert(nTexHandle);
    paletteHandle->AttachToTexture(nTexHandle);
    nTexHandle->Create("GM Radar Noise", MPR_TI_PALETTE bitor MPR_TI_CHROMAKEY, 8,
                       GM_TEXTURE_SIZE * 2, GM_TEXTURE_SIZE, TextureHandle::FLAG_HINT_STATIC);

    // Generate the green palette for the noise texture
    DWORD paletteData[256];

    for (int i = 0; i < 256; i++)
        paletteData[i] = 0xFF000000 bitor (i << 8);

    paletteData[0] = 0;

    // Generate noise image
    BYTE *pBuf = new BYTE[GM_TEXTURE_SIZE * 2 * GM_TEXTURE_SIZE];
    BYTE *pDst = pBuf;
    ShiAssert(pDst);

    if ( not pDst) return;

    const int nNoiseBase = 0x5;

    for (int y = 0; y < GM_TEXTURE_SIZE; y++)
    {
        for (int x = 0; x < GM_TEXTURE_SIZE * 2; x++)
            *pDst++ = Noise(nNoiseBase);
    }

    // initialize the texture but do not load bits
    nTexHandle->Load(0, 0, pBuf, true); // Chromakey 0 = black

    // this will eventually load (indirectly) the texture for the first and last time
    paletteHandle->Load(MPR_TI_PALETTE, 32, 0, 256, (BYTE*) paletteData);

    // texture owns a copy of the image data
    delete[] pBuf;
    nextOperation = NULL;
}


void RenderGMComposite::Cleanup(void)
{
    if (paletteHandle)
    {
        delete paletteHandle;
        paletteHandle = NULL;
    }

    if (lTexHandle)
    {
        delete lTexHandle;
        lTexHandle = NULL;
    }

    if (rTexHandle)
    {
        delete rTexHandle;
        rTexHandle = NULL;
    }

    if (nTexHandle)
    {
        delete nTexHandle;
        nTexHandle = NULL;
    }

    // Clean up our child RenderGMRadar
    radar.Cleanup();

    // Call our parent's cleanup
    Render2D::Cleanup();

    // Clean up our private rendering target image
    if (m_pRenderTarget)
    {
        if (m_bRenderTargetOwned)
        {
            m_pRenderTarget->Cleanup();
            delete m_pRenderTarget;
        }

        m_pRenderTarget = NULL;
        m_bRenderTargetOwned = false;
    }

    if (m_pBackupBuffer)
    {
        m_pBackupBuffer->Cleanup();
        delete m_pBackupBuffer;
        m_pBackupBuffer = NULL;
    }

    if (m_pRenderBuffer)
    {
        m_pRenderBuffer->Cleanup();
        delete m_pRenderBuffer;
        m_pRenderBuffer = NULL;
    }
}


void RenderGMComposite::SetBeam(Tpoint *from, Tpoint *at, Tpoint *center, float platformHdg, float beamAngle, int beamPercent, float cursorAngle, BOOL movingRight, bool Shaped)
{
    float Px, Py;
    float dx, dy;
    float hdg;

    ShiAssert(rTexHandle);
    ShiAssert(lTexHandle);

    // Convert our location to use as the origion of the radar beam
    Px = (from->x - center->x) * worldToUnitScale; // Normalize for display range
    Py = (from->y - center->y) * worldToUnitScale; // Normalize for display range

    // Set up the edge with separates the old and new parts of the sweep
    hdg = platformHdg + beamAngle;
    dx = (float)cos(hdg);
    dy = (float)sin(hdg);
    beam.SetupWithVector(Px, Py, dx, dy);

    // Set up the left gimbal limit edge
    hdg = platformHdg - gimbalLimit + cursorAngle; // 2002-04-03 MN add in current cursor position angle for azimuth limitations
    dx = (float)cos(hdg);
    dy = (float)sin(hdg);
    leftLimit.SetupWithVector(Px, Py, dx, dy);

    // Set up the right gimbal limit edge (direction reversed since clip is "edge right of pt == OUT")
    hdg = platformHdg + gimbalLimit + cursorAngle;
    dx = -(float)cos(hdg);
    dy = -(float)sin(hdg);
    rightLimit.SetupWithVector(Px, Py, dx, dy);

    // OW
    static RECT rcBlit = { 0, 0, GM_TEXTURE_SIZE, GM_TEXTURE_SIZE };

    if ( not DisplayOptions.bRender2Texture)
    {
        // This whole blitting crap is slooow

        // Save scene
        HRESULT hr = m_pBackupBuffer->targetSurface()->Blt(&rcBlit, m_pRenderTarget->targetSurface(), &rcBlit, DDBLT_WAIT, NULL);
        ShiAssert(SUCCEEDED(hr));

        // restore image
        hr = m_pRenderTarget->targetSurface()->Blt(&rcBlit, m_pRenderBuffer->targetSurface(), &rcBlit, DDBLT_WAIT, NULL);
        ShiAssert(SUCCEEDED(hr));

        radar.context.SetViewportAbs(0, 0, GM_TEXTURE_SIZE, GM_TEXTURE_SIZE);
        radar.context.LockViewport();
    }

    // Do necessary background processing to prepare the next image
    ShiAssert(beamPercent <= 100);
    bool bRender = BackgroundGeneration(from, at, platformHdg, beamPercent, movingRight, Shaped);

    if ( not DisplayOptions.bRender2Texture)
    {
        HRESULT hr;

        // Save image
        hr = m_pRenderBuffer->targetSurface()->Blt(&rcBlit, m_pRenderTarget->targetSurface(), &rcBlit, DDBLT_WAIT, NULL);
        ShiAssert(SUCCEEDED(hr));

        // restore scene
        hr = m_pRenderTarget->targetSurface()->Blt(&rcBlit, m_pBackupBuffer->targetSurface(), &rcBlit, DDBLT_WAIT, NULL);
        ShiAssert(SUCCEEDED(hr));

        radar.context.UnlockViewport();
        radar.context.SetViewportAbs(0, 0, m_pRenderTarget->targetXres(), m_pRenderTarget->targetYres());
    }
}


void RenderGMComposite::DrawComposite(Tpoint *center, float platformHdg)
{
    float Px, Py, x, y, dx, dy;
    float sinRot, cosRot;
    int num = 0;
    int i;

    ShiAssert(rTexHandle);
    ShiAssert(lTexHandle);

    if ( not DisplayOptions.bRender2Texture)
    {
        radar.context.UnlockViewport();
        radar.context.SetViewportAbs(0, 0, m_pRenderTarget->targetXres(), m_pRenderTarget->targetYres());
    }

    // Normalize the current center of attention position for display range
    Px = center->x * worldToUnitScale;
    Py = center->y * worldToUnitScale;

    // Determine translation and rotation parameters for the right side
    dx = rOriginX - Px;
    dy = rOriginY - Py;
    sinRot = (float)sin(rAngle);
    cosRot = (float)cos(rAngle);

    // Set up a normalized quad for the right patch in X north, Y east
    // We're translating and rotating to account for drift since the texture
    // was rendererd.
    x = -1.0f;
    y = -GM_OVERSCAN_H;
    v0.x = x * cosRot - y * sinRot + dx;
    v0.y = x * sinRot + y * cosRot + dy;
    v0.u = 0.0f;
    v0.v = 1.0f; // Lower left

    x = -1.0f;
    y =  GM_OVERSCAN_H;
    v1.x = x * cosRot - y * sinRot + dx;
    v1.y = x * sinRot + y * cosRot + dy;
    v1.u = 1.0f;
    v1.v = 1.0f; // Lower right

    x =  GM_OVERSCAN_V;
    y =  GM_OVERSCAN_H;
    v2.x = x * cosRot - y * sinRot + dx;
    v2.y = x * sinRot + y * cosRot + dy;
    v2.u = 1.0f;
    v2.v = 0.0f; // Upper right

    x =  GM_OVERSCAN_V;
    y = -GM_OVERSCAN_H;
    v3.x = x * cosRot - y * sinRot + dx;
    v3.y = x * sinRot + y * cosRot + dy;
    v3.u = 0.0f;
    v3.v = 0.0f; // Upper left

    num = ClipToBeamAndLimits();

    // Draw the polygon if it isn't totally clipped
    if (num >= 3)
    {
        sinRot = (float)sin(-platformHdg);
        cosRot = (float)cos(-platformHdg);

        for (i = 0; i < num; i++)
        {
            // Rotate for platform heading
            x = vertArray[i]->x * cosRot - vertArray[i]->y * sinRot;
            y = vertArray[i]->x * sinRot + vertArray[i]->y * cosRot;

            // Scale to pixel space
            vertArray[i]->x = viewportXtoPixel(y);
            vertArray[i]->y = viewportYtoPixel(-x);
            SetClipFlags(vertArray[i]);
        }

        context.RestoreState(STATE_TEXTURE);
        context.SelectTexture1((UInt) rTexHandle);
        ClipAndDraw2DFan(vertArray, num);
    }


    // Reverse the direction of the beam edge to keep the clipping code consistent
    beam.Reverse();

    // Determine translation and rotation parameters for the left side
    dx = lOriginX - Px;
    dy = lOriginY - Py;
    sinRot = (float)sin(lAngle);
    cosRot = (float)cos(lAngle);

    // Set up a normalized quad for the left patch in X north, Y east
    // We're translating and rotating to account for drift since the texture
    // was rendererd.
    x = -1.0f;
    y = -GM_OVERSCAN_H;
    v0.x = x * cosRot - y * sinRot + dx;
    v0.y = x * sinRot + y * cosRot + dy;
    v0.u = 0.0f;
    v0.v = 1.0f; // Lower left

    x = -1.0f;
    y =  GM_OVERSCAN_H;
    v1.x = x * cosRot - y * sinRot + dx;
    v1.y = x * sinRot + y * cosRot + dy;
    v1.u = 1.0f;
    v1.v = 1.0f; // Lower right

    x =  GM_OVERSCAN_V;
    y =  GM_OVERSCAN_H;
    v2.x = x * cosRot - y * sinRot + dx;
    v2.y = x * sinRot + y * cosRot + dy;
    v2.u = 1.0f;
    v2.v = 0.0f; // Upper right

    x =  GM_OVERSCAN_V;
    y = -GM_OVERSCAN_H;
    v3.x = x * cosRot - y * sinRot + dx;
    v3.y = x * sinRot + y * cosRot + dy;
    v3.u = 0.0f;
    v3.v = 0.0f; // Upper left

    num = ClipToBeamAndLimits();

    // Draw the polygon if it isn't totally clipped
    if (num >= 3)
    {
        sinRot = (float)sin(-platformHdg);
        cosRot = (float)cos(-platformHdg);

        for (i = 0; i < num; i++)
        {
            // Rotate for platform heading
            x = vertArray[i]->x * cosRot - vertArray[i]->y * sinRot;
            y = vertArray[i]->x * sinRot + vertArray[i]->y * cosRot;

            // Scale to pixel space
            vertArray[i]->x = viewportXtoPixel(y);
            vertArray[i]->y = viewportYtoPixel(-x);
            SetClipFlags(vertArray[i]);
        }

        context.RestoreState(STATE_TEXTURE);
        context.SelectTexture1((UInt) lTexHandle);
        ClipAndDraw2DFan(vertArray, num);
    }

    // Put the beam back the way it was in case we need it again next time
    beam.Reverse();

    if ( not DisplayOptions.bRender2Texture)
    {
        radar.context.UnlockViewport();
    }
}


// Always assumes beam starts from left after reset
void RenderGMComposite::SetRange(float newRange, int newLOD)
{
    ShiAssert(lTexHandle);
    ShiAssert(rTexHandle);

    // Update our child class
    if ( not radar.SetRange(newRange * GM_OVERSCAN_RNG, newLOD))
    {
        // If it required no changes, stop here
        return;
    }

    // Store our scale factor
    range = newRange;
    worldToUnitScale = 1.0f / newRange;

    // We need to force a full cycle of the GMRadar redraw loop
    // to generate the first sweeps data all at once.
    nextOperation = &OperationList[0];
    prevBeamRight = FALSE;
    prevBeamPercent = 101;
    DrawChanged = true;

    rTexHandle->Clear();
}


bool RenderGMComposite::BackgroundGeneration(Tpoint *from, Tpoint *at, float platformHdg, int beamPercent, BOOL movingRight, bool Shaped)
{
    OpName operation;
    float dx, dy;
    Tpoint center;

    ShiAssert(prevBeamPercent <= 101);
    ShiAssert(beamPercent <= 101);

    //MI CTD
    if (nextOperation == NULL)
        return FALSE;

    // Detect the beam reversal at the end of a sweep or a sweep restart
    if ((prevBeamRight not_eq movingRight) /*or (beamPercent < prevBeamPercent)*/ or DrawChanged)
    {
        //BackgroundGeneration( from, at, platformHdg, 101, prevBeamRight, Shaped );
        dx = (float)cos(radar.GetHdg());
        dy = (float)sin(radar.GetHdg());
        center.x = radar.GetAt()->x - dx * GM_OVERSCAN * range;
        center.y = radar.GetAt()->y - dy * GM_OVERSCAN * range;
        center.z = radar.GetAt()->z;
        NewImage(&center, radar.GetHdg(), movingRight, Shaped);
        prevBeamRight = movingRight;
        nextOperation = &OperationList[0];
        DrawChanged = false;
    }

    prevBeamPercent = beamPercent;
    bool bRender = false;

    // Do all necessary processing
    while (beamPercent >= nextOperation->percent)
    {
        operation = nextOperation->operation;
        nextOperation++;
        bRender = true;

        switch (operation)
        {

            case Start:
                radar.StartDraw();
                dx = (float)cos(platformHdg);
                dy = (float)sin(platformHdg);
                center.x = at->x + dx * GM_OVERSCAN * range;
                center.y = at->y + dy * GM_OVERSCAN * range;
                center.z = at->z;
                radar.StartScene(from, &center, platformHdg);
                radar.EndDraw();
                break;

            case Xform:
                radar.StartDraw();
                radar.TransformScene();
                radar.EndDraw();
                break;

            case Ground:
                radar.StartDraw();
                radar.DrawScene();
                radar.EndDraw();
                break;

            case Features:
                if ( not Shaped) break;

                radar.StartDraw();
                radar.DrawFeatures();
                radar.EndDraw();
                break;

            case Targets:
                radar.StartDraw();
                radar.PrepareToDrawTargets();
                tgtDrawCB(tgtDrawCBparam, &radar, Shaped);
                radar.FlushDrawnTargets();
                radar.EndDraw();
                break;

            case Finish:
                radar.StartDraw();
                radar.FinishScene();
                radar.EndDraw();
                break;

            case Replace:
                beamPercent = 0;
                break;

            default:
                ShiWarning("Bad GM Radar generation sequence");
                break;
        }

    }

    return bRender;
}

void RenderGMComposite::NewImage(Tpoint *at, float platformHdg, BOOL replaceRight, bool Shaped)
{
    TextureHandle *targetHandle;
    ShiAssert(lTexHandle);
    ShiAssert(rTexHandle);

    // nTexHandle->PreLoad(); // it will be used soon

    // Change target textures each time we get called
    if (replaceRight)
    {
        rOriginX = at->x * worldToUnitScale; // Normalize for display range
        rOriginY = at->y * worldToUnitScale; // Normalize for display range
        rAngle = platformHdg;
        targetHandle = rTexHandle;
    }

    else
    {
        lOriginX = at->x * worldToUnitScale; // Normalize for display range
        lOriginY = at->y * worldToUnitScale; // Normalize for display range
        lAngle = platformHdg;
        targetHandle = lTexHandle;
    }

    // Render Noise overlay
    if ( not DisplayOptions.bRender2Texture)
    {
        radar.context.UnlockViewport();
        radar.context.SetViewportAbs(0, 0, m_pRenderTarget->targetXres(), m_pRenderTarget->targetYres());
    }

    float Alpha = 0.3f;

    if (Shaped) Alpha = 0.7f;

    radar.StartDraw();

    // calc random U
    int nRand = MulDiv(rand(), 128, RAND_MAX) - 1;
    const float fUStart = nRand * (1.0f / (GM_TEXTURE_SIZE * 2));
    const float fUStop = fUStart + 0.5f;
    ShiAssert(fUStop <= 1.0f);

    // setup vertices
    TwoDVertex pVtx[4];
    ZeroMemory(pVtx, sizeof(pVtx));
    pVtx[0].x = 0.0f;
    pVtx[0].y = 0.0f;
    pVtx[0].u = fUStart;
    pVtx[0].v = 0.0f;
    pVtx[0].r = pVtx[0].g = pVtx[0].b = pVtx[0].a = Alpha;

    pVtx[1].x = GM_TEXTURE_SIZE;
    pVtx[1].y = 0.0f;
    pVtx[1].u = fUStop;
    pVtx[1].v = 0.0f;
    pVtx[1].r = pVtx[1].g = pVtx[1].b = pVtx[1].a = Alpha;

    pVtx[2].x = GM_TEXTURE_SIZE;
    pVtx[2].y = GM_TEXTURE_SIZE;
    pVtx[2].u = fUStop;
    pVtx[2].v = 1.0f;
    pVtx[2].r = pVtx[2].g = pVtx[2].b = pVtx[2].a = Alpha;

    pVtx[3].x = 0.0f;
    pVtx[3].y = GM_TEXTURE_SIZE;
    pVtx[3].u = fUStart;
    pVtx[3].v = 1.0f;
    pVtx[3].r = pVtx[3].g = pVtx[3].b = pVtx[3].a = Alpha;

    // and action
    radar.context.RestoreState(STATE_ALPHA_TEXTURE); //JAM 18Oct03
    radar.context.SetState(MPR_STA_DST_BLEND_FUNCTION, MPR_BF_ONE);
    //MI TEST
    radar.context.SelectTexture1((GLint) nTexHandle);
    radar.context.DrawPrimitive(MPR_PRM_TRIFAN, MPR_VI_COLOR bitor MPR_VI_TEXTURE, 4, pVtx, sizeof(pVtx[0]));
    // radar.context.InvalidateState();

    radar.EndDraw();

    static RECT rcBlit = { 0, 0, GM_TEXTURE_SIZE, GM_TEXTURE_SIZE };

    if ( not DisplayOptions.bRender2Texture)
    {
        // Save image
        HRESULT hr = m_pRenderBuffer->targetSurface()->Blt(&rcBlit, m_pRenderTarget->targetSurface(), &rcBlit, DDBLT_WAIT, NULL);
        ShiAssert(SUCCEEDED(hr));
    }

    // Now blit the final viewport image to the texture
    ImageBuffer *pSrcBuffer = DisplayOptions.bRender2Texture ? m_pRenderTarget : m_pRenderBuffer;
    HRESULT hr = targetHandle->m_pDDS->Blt(NULL, pSrcBuffer->targetSurface(), NULL, DDBLT_WAIT, NULL);
    ShiAssert(SUCCEEDED(hr));

    if ( not DisplayOptions.bRender2Texture)
    {
        radar.context.SetViewportAbs(0, 0, GM_TEXTURE_SIZE, GM_TEXTURE_SIZE);
        radar.context.LockViewport();
    }
}


// Helper function to compute x, y, u, v between two points
static inline void Intersect(TwoDVertex *v1, TwoDVertex *v2, TwoDVertex *c, float t)
{
    c->x = v1->x + t * (v2->x - v1->x);
    c->y = v1->y + t * (v2->y - v1->y);
    c->u = v1->u + t * (v2->u - v1->u);
    c->v = v1->v + t * (v2->v - v1->v);
}


static int ClipToEdge(TwoDVertex **inArray, TwoDVertex **outArray, Edge *edge, int inCount)
{
    int i;
    int num;
    BOOL amOut, wasOut, startedOut;
    float d1, d2, t;
    TwoDVertex *p;
    TwoDVertex *v;


    // Stop now if we don't have enough verts to continue
    if (inCount < 3)
    {
        return 0;
    }

    num = 0;
    v = inArray[0];

    // Check the first vert
    wasOut = startedOut = edge->RightOf(v->x, v->y);

    if ( not startedOut)
    {
        outArray[num] = v;
        num++;
    }

    // Run through the interior verts
    i = 1;

    while (i < inCount)
    {
        p = v;
        v = inArray[i];
        i++;

        amOut = edge->RightOf(v->x, v->y);

        if (amOut not_eq wasOut)
        {
            // Compute the intesection
            d1 = edge->DistanceFrom(p->x, p->y);
            d2 = edge->DistanceFrom(v->x, v->y);
            t = d1 / (d1 - d2);
            Intersect(p, v, &c[NumCverts], t);
            outArray[num] = &c[NumCverts];
            num++;
            NumCverts++;
        }

        if ( not amOut)
        {
            outArray[num] = v;
            num++;
        }

        wasOut = amOut;
    }

    // Finally, clip the edge between the first and last verts
    if (wasOut not_eq startedOut)
    {
        p = v;
        v = inArray[0];

        // Compute the intesection
        d1 = edge->DistanceFrom(p->x, p->y);
        d2 = edge->DistanceFrom(v->x, v->y);
        t = d1 / (d1 - d2);
        Intersect(p, v, &c[NumCverts], t);
        outArray[num] = &c[NumCverts];
        num++;
        NumCverts++;
    }

    ShiAssert(num <= MAX_VERTEX_LIST);
    ShiAssert(NumCverts <= MAX_CONSTRUCTION_VERTS);

    return num;
}


static int ClipToBeamAndLimits(void)
{
    int num = 4;
    TwoDVertex *inArray[MAX_VERTEX_LIST];

    inArray[0] = &v0;
    inArray[1] = &v1;
    inArray[2] = &v2;
    inArray[3] = &v3;

    // Start with no construction verts in use
    NumCverts = 0;

    // Clip the incomming quad to the beam and gimbal limits
    num = ClipToEdge(inArray,   vertArray, &leftLimit,  num);
    num = ClipToEdge(vertArray, inArray,   &rightLimit, num);
    num = ClipToEdge(inArray,   vertArray, &beam,       num);

    return num;
}


void RenderGMComposite::DebugDrawLeftTexture(Render2D *renderer)
{
    char string[80];
    int num = 0;

    ShiAssert(lTexHandle);


    renderer->context.RestoreState(STATE_TEXTURE);
    renderer->context.SelectTexture1((UInt) lTexHandle);

    v0.x = 1.0f;
    v0.y = 1.0f;
    v0.u = 0.0f;
    v0.v = 0.0f; // UL
    v1.x = GM_TEXTURE_SIZE;
    v1.y = 1.0f;
    v1.u = 1.0f;
    v1.v = 0.0f; // UR
    v2.x = GM_TEXTURE_SIZE;
    v2.y = GM_TEXTURE_SIZE;
    v2.u = 1.0f;
    v2.v = 1.0f; // BR
    v3.x = 1.0f;
    v3.y = GM_TEXTURE_SIZE;
    v3.u = 0.0f;
    v3.v = 1.0f; // BL

    vertArray[num++] = &v0;
    SetClipFlags(&v0);
    vertArray[num++] = &v1;
    SetClipFlags(&v1);
    vertArray[num++] = &v2;
    SetClipFlags(&v2);
    vertArray[num++] = &v3;
    SetClipFlags(&v3);

    renderer->ClipAndDraw2DFan(vertArray, num);

    renderer->SetColor(0x00000000);

    sprintf(string, "L ANGLE: %0.0f deg", lAngle * 180.0f / PI);
    renderer->ScreenText(2.0f, 10.0f, string);
    sprintf(string, "GM HDG:  %0.0f deg", radar.GetHdg() * 180.0f / PI);
    renderer->ScreenText(2.0f, 18.0f, string);
}


void RenderGMComposite::DebugDrawRadarImage(ImageBuffer *target)
{
    RECT srcRect;
    RECT dstRect;

    srcRect.top = 0;
    srcRect.left = 0;
    srcRect.bottom = GM_TEXTURE_SIZE;
    srcRect.right = GM_TEXTURE_SIZE;

    dstRect.top = 0;
    dstRect.left = GM_TEXTURE_SIZE + 2;
    dstRect.bottom = dstRect.top + GM_TEXTURE_SIZE;
    dstRect.right = dstRect.left + GM_TEXTURE_SIZE;

    target->Compose(m_pRenderTarget, &srcRect, &dstRect);

    v0.b = 0.0f;
    v0.g = 0.0f;
    v0.r = 1.0f;
    v1.b = 0.0f;
    v1.g = 1.0f;
    v1.r = 0.0f;
    v2.b = 1.0f;
    v2.g = 0.0f;
    v2.r = 0.0f;
    v3.b = 0.0f;
    v3.g = 1.0f;
    v3.r = 1.0f;
    c[0].b = 1.0f;
    c[0].g = 1.0f;
    c[0].r = 1.0f;
    c[1].b = 1.0f;
    c[1].g = 1.0f;
    c[1].r = 1.0f;
}
