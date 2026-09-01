/***************************************************************************\
    ImageBuf.cpp
    Scott Randolph
    December 29, 1995

    This class provides management for drawing target buffers and sources
 for blit operations.
\***************************************************************************/

#include <ciso646>
#include "stdafx.h"
#include "rotate.h"
#include "device.h"
#include "imagebuf.h"
#include "graphics/dxengine/d3d12backend.h" // Artscout - 2026: #DX12 Phase 1
#include "graphics/vulkan/vulkanbackend.h" // Artscout - 2026 (#104): g_pVulkanBackend 565 present peer
#include "graphics/vulkan/vulkantexturemanager.h" // Artscout - 2026 (GM radar): Vulkan off-screen RTT twin
#include "graphics/dxengine/d3d12/d3d12texturemanager.h" // Artscout - 2026: #DX12 A5 -- off-screen RTT (D3D12Texture)
#include "graphics/dxengine/openxrbackend.h" // VR (OpenXR)
#include "../../sim/include/ivibedata.h" // VR: g_intellivibeData.In3D (menu vs sim)
#include "graphics/dxengine/common/irenderer.h" // composite the 2D UI over 3D (renderer abstraction)
#include "falclib/include/debuggr.h"
#include "falclib/include/isbad.h"
//#define _IMAGEBUFFER_PROTECT_SURF_LOCK
extern bool g_bCheckBltStatusBeforeFlip;

#ifdef _DEBUG
static char *arrType2String[] = {
    "SystemMem", "VideoMem",      "Primary",         "Flip",
    "None",      "LocalVideoMem", "LocalVideoMem3D",
};
#endif

ImageBuffer::ImageBuffer()
{
    m_bReady = FALSE;
    m_bFrontRectValid = false;
    m_bBitsLocked = false;

    m_pDDSFront = NULL;
    m_pDDSBack = NULL;
    ZeroMemory(&m_rcFront, sizeof(m_rcFront));
    m_pBltTarget = NULL;
    m_bIsScreenBuffer = false;
    m_pSysMem = NULL;

    m_pD3D12RTT =
        NULL; // Artscout - 2026: #DX12 A5 -- off-screen RTT (D3D12Texture*)
    m_pVulkanRTT =
        NULL; // Artscout - 2026 (GM radar): Vulkan twin (VulkanTexture*)
    m_bVulkanRttVirgin = true;

#ifdef _IMAGEBUFFER_PROTECT_SURF_LOCK
    InitializeCriticalSection(&m_cs);
#endif
}

ImageBuffer::~ImageBuffer()
{
    ShiAssert(not IsReady());
    Cleanup(); // OW

#ifdef _IMAGEBUFFER_PROTECT_SURF_LOCK
    DeleteCriticalSection(&m_cs);
#endif
}

BOOL ImageBuffer::IsReady()
{
    return m_bReady;
}

BOOL ImageBuffer::Setup(DisplayDevice *dev, int w, int h, MPRSurfaceType front,
                        MPRSurfaceType back, HWND targetWin, BOOL clip,
                        bool fullScreen, BOOL bWillCallSwapBuffer)
{
    ZeroMemory(&m_rcFront, sizeof(m_rcFront));

    try
    {
        ShiAssert(not IsReady());
        ShiAssert(dev);


        // Record the properties of the buffer(s) we're creating
        device = dev;
        width = w;
        height = h;

        // PHASE 1 (D3D7->D3D11): no DDraw surfaces. CPU buffer 16-bit RGB565, the UI composites
        // via Lock; the screen buffer (front==Primary) drives Present.
        extern bool g_bUseGpu;
        if (g_bUseGpu) // #DX12/#104: any GPU backend (D3D12/Vulkan) uses the CPU RGB565 buffer (no DDraw surfaces)
        {
            m_bIsScreenBuffer = (front == Primary);
            ZeroMemory(&m_ddsdFront, sizeof(m_ddsdFront));
            ZeroMemory(&m_ddsdBack, sizeof(m_ddsdBack));
            m_ddsdFront.ddpfPixelFormat.dwRGBBitCount = 16;
            m_ddsdFront.ddpfPixelFormat.dwRBitMask = 0xF800;
            m_ddsdFront.ddpfPixelFormat.dwGBitMask = 0x07E0;
            m_ddsdFront.ddpfPixelFormat.dwBBitMask = 0x001F;
            m_ddsdBack.lPitch = width * 2;
            ComputeColorShifts();
            if (m_pSysMem)
            {
                free(m_pSysMem);
                m_pSysMem = NULL;
            } // #55 don't leak on a repeated Setup without Cleanup
            m_pSysMem = (BYTE *)malloc((size_t)width * height * 2);
            if (m_pSysMem)
                memset(m_pSysMem, 0, (size_t)width * height * 2);
            m_bReady = TRUE;
            return TRUE;
        }


        return TRUE;
    }

    catch (const _com_error &e)
    {
        MonoPrint("ImageBuffer::Setup - Error 0x%X\n", e.Error());
        return FALSE;
    }
}

// Alternative setup method
void ImageBuffer::AttachSurfaces(DisplayDevice *pDev,
                                 IDirectDrawSurface7 *pDDSFront,
                                 IDirectDrawSurface7 *pDDSBack)
{
    if (not pDev or not pDDSFront)
        return;

    // Artscout - 2026: [DX7-PURGE] DDraw surface attach removed. The GPU (D3D11/D3D12)
    // path never attaches DirectDraw surfaces -- it composes into the CPU RGB565 buffer.
    (void)pDev;
    (void)pDDSFront;
    (void)pDDSBack;
}


// Release the DDraw surfaces associated with this object
void ImageBuffer::Cleanup(void)
{
    m_bReady = FALSE;

    // Artscout - 2026: [DX7-PURGE] DDraw surface Release() removed (no DDraw surfaces under GPU).
    m_pDDSBack = NULL;
    m_pDDSFront = NULL;
    m_pBltTarget = NULL;

    // Artscout - 2026: #DX12 A5 -- free the D3D12 off-screen RTT + drop its readback slot in the backend.
#ifdef _WIN32 // D3D12 is Windows-only; Linux uses the Vulkan branch
    if (m_pD3D12RTT)
    {
        if (g_pD3D12Backend)
            g_pD3D12Backend->ReleaseReadbackFor(m_pD3D12RTT);
        if (g_pD3D12TextureManager)
        {
            D3D12Texture *t = (D3D12Texture *)m_pD3D12RTT;
            g_pD3D12TextureManager->Destroy(*t);
            g_pD3D12TextureManager->Free(t);
        }
        m_pD3D12RTT = NULL;
    }
#endif // _WIN32

    // Artscout - 2026 (GM radar): free the Vulkan off-screen RTT. Destroy() is deferred-retire (an in-flight
    // command buffer may still reference the view), so this is safe mid-frame.
    if (m_pVulkanRTT)
    {
        if (g_pVulkanTextureManager)
            g_pVulkanTextureManager->Destroy((VulkanTexture *)m_pVulkanRTT);
        m_pVulkanRTT = NULL;
        m_bVulkanRttVirgin = true;
    }

    // #55 MEMORY-LEAK ROOT on 3D enter/exit: the D3D11 surface CPU buffer (565), malloc'd in
    // Setup() line 101, was NEVER freed -> every ImageBuffer (cockpit 2x35MB, display 7MB,
    // MFD...) leaked its backing buffer on EVERY entry -> ~70+MB/cycle -> std::bad_alloc.
    // Cleanup is called from ~ImageBuffer -> free it here.
    if (m_pSysMem)
    {
        free(m_pSysMem);
        m_pSysMem = NULL;
    }
}


// Compute the right shifts required to get from 24 bit RGB to this pixel format
void ImageBuffer::ComputeColorShifts(void)
{
    UInt32 mask;

    // RED
    mask = m_ddsdFront.ddpfPixelFormat.dwRBitMask;
    redShift = 8;
    ShiAssert(mask);

    while (not(mask bitand 1))
    {
        mask >>= 1;
        redShift--;
    }

    while (mask bitand 1)
    {
        mask >>= 1;
        redShift--;
    }

    // GREEN
    mask = m_ddsdFront.ddpfPixelFormat.dwGBitMask;
    greenShift = 16;
    ShiAssert(mask);

    while (not(mask bitand 1))
    {
        mask >>= 1;
        greenShift--;
    }

    while (mask bitand 1)
    {
        mask >>= 1;
        greenShift--;
    }

    // BLUE
    mask = m_ddsdFront.ddpfPixelFormat.dwBBitMask;
    ShiAssert(mask);
    blueShift = 24;

    while (not(mask bitand 1))
    {
        mask >>= 1;
        blueShift--;
    }

    while (mask bitand 1)
    {
        mask >>= 1;
        blueShift--;
    }
}

void ImageBuffer::GetColorMasks(UInt32 *r, UInt32 *g, UInt32 *b)
{
    *r = m_ddsdFront.ddpfPixelFormat.dwRBitMask;
    *g = m_ddsdFront.ddpfPixelFormat.dwGBitMask;
    *b = m_ddsdFront.ddpfPixelFormat.dwBBitMask;
}

// Adjust the offset into the primary surface if we are windowed
void ImageBuffer::UpdateFrontWindowRect(RECT *rect)
{
    // ShiAssert( frontType == Primary ); // This is only useful for the primary surface
    if (rect)
        m_rcFront = *rect;

    m_bFrontRectValid = rect and (m_rcFront.left or m_rcFront.right);
}

// Fix in memory and return and pointer to the memory associated with our back buffer
void *ImageBuffer::Lock(bool bLockMutexOnly, bool bWriteOnly)
{
    // Artscout - 2026: [DX7-PURGE] GPU (D3D11/D3D12) is the only path -- always the CPU RGB565 buffer.
    (void)bLockMutexOnly;
    (void)bWriteOnly;
    return m_pSysMem;
}

// Reliquish our exclusive pointer to the memory associated with our back buffer
void ImageBuffer::Unlock()
{
    // Artscout - 2026: [DX7-PURGE] GPU path has no DDraw surface to unlock -- nothing to do.
    m_bBitsLocked = false;
}

// Set the color key for this surface to be used when it is transparently
// composed into another buffer.
void ImageBuffer::SetChromaKey(UInt32 colorKey)
{
    if (not m_pDDSFront) // JB 010404 CTD
        return;

    ShiAssert(IsReady());

    // Convert the key color from 32 bit RGB to the current pixel format

    // RED
    if (redShift >= 0)
        m_dwColorKey = (colorKey >> redShift) bitand
                       m_ddsdFront.ddpfPixelFormat.dwRBitMask;
    else
        m_dwColorKey = (colorKey << -redShift) bitand
                       m_ddsdFront.ddpfPixelFormat.dwRBitMask;

    // GREEN
    if (greenShift >= 0)
        m_dwColorKey or_eq (colorKey >> greenShift) bitand
                           m_ddsdFront.ddpfPixelFormat.dwGBitMask;
    else
        m_dwColorKey or_eq (colorKey << -greenShift) bitand
                           m_ddsdFront.ddpfPixelFormat.dwGBitMask;

    // BLUE
    if (blueShift >= 0)
        m_dwColorKey or_eq (colorKey >> blueShift) bitand
                           m_ddsdFront.ddpfPixelFormat.dwBBitMask;
    else
        m_dwColorKey or_eq (colorKey << -blueShift) bitand
                           m_ddsdFront.ddpfPixelFormat.dwBBitMask;

    // Artscout - 2026: [DX7-PURGE] m_dwColorKey is consumed by the CPU composite path;
    // the DDraw SetColorKey() submission is gone (no DDraw surface under GPU).
}

// Convert a 32 bit alpha, blue, green, red color to a 16 bit pixel
// NOTE:  At present alpha is ignored by this call, but that could change...
WORD ImageBuffer::Pixel32toPixel16(UInt32 ABGR)
{
    UInt32 color;

    // OW FIXME
    // ShiAssert( PixelSize() == 2 ); // Only returns 16 bit values

    // RED
    if (redShift >= 0)
    {
        color =
            (ABGR >> redShift) bitand m_ddsdFront.ddpfPixelFormat.dwRBitMask;
    }
    else
    {
        color =
            (ABGR << -redShift) bitand m_ddsdFront.ddpfPixelFormat.dwRBitMask;
    }

    // GREEN
    if (greenShift >= 0)
    {
        color or_eq
            (ABGR >> greenShift) bitand m_ddsdFront.ddpfPixelFormat.dwGBitMask;
    }
    else
    {
        color or_eq
            (ABGR << -greenShift) bitand m_ddsdFront.ddpfPixelFormat.dwGBitMask;
    }

    // BLUE
    if (blueShift >= 0)
    {
        color or_eq
            (ABGR >> blueShift) bitand m_ddsdFront.ddpfPixelFormat.dwBBitMask;
    }
    else
    {
        color or_eq
            (ABGR << -blueShift) bitand m_ddsdFront.ddpfPixelFormat.dwBBitMask;
    }

    return (WORD)color;
}

DWORD ImageBuffer::Pixel32toPixel32(UInt32 ABGR)
{
    UInt32 color;

    // OW FIXME
    // ShiAssert( PixelSize() == 2 ); // Only returns 16 bit values

    // RED
    if (redShift >= 0)
    {
        color =
            (ABGR >> redShift) bitand m_ddsdFront.ddpfPixelFormat.dwRBitMask;
    }
    else
    {
        color =
            (ABGR << -redShift) bitand m_ddsdFront.ddpfPixelFormat.dwRBitMask;
    }

    // GREEN
    if (greenShift >= 0)
    {
        color or_eq
            (ABGR >> greenShift) bitand m_ddsdFront.ddpfPixelFormat.dwGBitMask;
    }
    else
    {
        color or_eq
            (ABGR << -greenShift) bitand m_ddsdFront.ddpfPixelFormat.dwGBitMask;
    }

    // BLUE
    if (blueShift >= 0)
    {
        color or_eq
            (ABGR >> blueShift) bitand m_ddsdFront.ddpfPixelFormat.dwBBitMask;
    }
    else
    {
        color or_eq
            (ABGR << -blueShift) bitand m_ddsdFront.ddpfPixelFormat.dwBBitMask;
    }

    return color;
}


// Convert a 16 bit pixel to a 32 bit alpha, blue, green, red color
// NOTE:  At present alpha is always set to 0 by this call, but that could change...
UInt32 ImageBuffer::Pixel16toPixel32(WORD pixel)
{
    UInt32 color;

    ShiAssert(PixelSize() == 2); // Only returns 16 bit values

    // RED
    if (redShift >= 0)
    {
        color = (pixel bitand m_ddsdFront.ddpfPixelFormat.dwRBitMask)
                << redShift;
    }
    else
    {
        color =
            (pixel bitand m_ddsdFront.ddpfPixelFormat.dwRBitMask) >> -redShift;
    }

    // GREEN
    if (greenShift >= 0)
    {
        color or_eq (pixel bitand m_ddsdFront.ddpfPixelFormat.dwGBitMask)
                    << greenShift;
    }
    else
    {
        color or_eq (pixel bitand m_ddsdFront.ddpfPixelFormat.dwGBitMask) >>
                    -greenShift;
    }

    // BLUE
    if (blueShift >= 0)
    {
        color or_eq (pixel bitand m_ddsdFront.ddpfPixelFormat.dwBBitMask)
                    << blueShift;
    }
    else
    {
        color or_eq
            (pixel bitand m_ddsdFront.ddpfPixelFormat.dwBBitMask) >> -blueShift;
    }

    return color;
}

// Copy a retangular area from the source image's front buffer to the this images's
// back buffer.  Both rectangles must be entirely inside their respective buffers.
void ImageBuffer::Compose(ImageBuffer *srcBuffer, RECT *dstRect, RECT *srcRect)
{
    ShiAssert(IsReady());
    ShiAssert(FALSE == F4IsBadReadPtr(srcRect, sizeof *srcRect));
    ShiAssert(FALSE == F4IsBadReadPtr(dstRect, sizeof *dstRect));
    ShiAssert(FALSE == F4IsBadReadPtr(srcBuffer, sizeof *srcBuffer));


    // Artscout - 2026: [DX7-PURGE] DDraw surface->Blt composite removed (GPU path composes via renderer).
}

// Copy a retangular area from the source image's front buffer to the this images's
// back buffer.  Don't write pixels from the source whose color matches the provided
// color key value.  Both rectangles must be entirely inside their respective buffers.
void ImageBuffer::ComposeTransparent(ImageBuffer *srcBuffer, RECT *dstRect,
                                     RECT *srcRect)
{
    ShiAssert(IsReady());
    ShiAssert(FALSE == F4IsBadReadPtr(srcRect, sizeof *srcRect));
    ShiAssert(FALSE == F4IsBadReadPtr(dstRect, sizeof *dstRect));
    ShiAssert(FALSE == F4IsBadReadPtr(srcBuffer, sizeof *srcBuffer));

    // Artscout - 2026: [DX7-PURGE] DDraw surface->Blt transparent composite removed (GPU path composes via renderer).
}

// Copy a retangular area from the source image's BACK buffer to the this images's
// BACK buffer while rotating the image "angle" radians clockwise.  No clipping is provided
// (Note: we really should use the source's front buffer, but this was easier)
void ImageBuffer::ComposeRot(ImageBuffer *srcBuffer, RECT *srcRect,
                             RECT *dstRect, float angle)
{
    ShiAssert(IsReady());
    ShiAssert(FALSE == F4IsBadReadPtr(srcRect, sizeof *srcRect));
    ShiAssert(FALSE == F4IsBadReadPtr(dstRect, sizeof *dstRect));
    ShiAssert(FALSE == F4IsBadReadPtr(srcBuffer, sizeof *srcBuffer));


    // Probably this will break if it is ever used to target a Primary surface
    // in a window without a back buffer.  We'd need to account for the window
    // offset in screen space.
    if (srcRect->right - srcRect->left == dstRect->right - dstRect->left)
    {
        ShiAssert((srcRect->bottom - srcRect->top) ==
                  (dstRect->bottom - dstRect->top));
        ShiAssert((srcRect->right - srcRect->left) ==
                  (dstRect->right - dstRect->left));

        RotateBitmap(
            srcBuffer, this,
            (int)(angle *
                  2607.594587618f), // 180.0f / PI * 4096 / 90.0f (convertion from radians to Erick's)
            srcRect, dstRect);
    }

    else
    {
        ShiAssert(2 * (srcRect->bottom - srcRect->top) ==
                  (dstRect->bottom - dstRect->top));
        ShiAssert(2 * (srcRect->right - srcRect->left) ==
                  (dstRect->right - dstRect->left));

        RotateBitmapDouble(
            srcBuffer, this,
            (int)(angle *
                  2607.594587618f), // 180.0f / PI * 4096 / 90.0f (convertion from radians to Erick's)
            srcRect, dstRect);
    }
}

// Copy a round area from the source image's BACK buffer to the this images's
// BACK buffer while rotating the image "angle" radians clockwise.  The startStopArray
// must be an array of an even number of integers.  The first int is the offset along the
// first line in the target buffer at which to start writing.  The second int is the offset
// at which to stop writing on the first line.  There must be as many integer pairs as there
// are lines in the destination rectangle.
// (Note: we really should use the source's front buffer, but this was easier)
void ImageBuffer::ComposeRoundRot(ImageBuffer *srcBuffer, RECT *srcRect,
                                  RECT *dstRect, float angle,
                                  int *startStopArray)
{
    ShiAssert(IsReady());
    ShiAssert(FALSE == F4IsBadReadPtr(srcRect, sizeof *srcRect));
    ShiAssert(FALSE == F4IsBadReadPtr(dstRect, sizeof *dstRect));
    ShiAssert(FALSE == F4IsBadReadPtr(srcBuffer, sizeof *srcBuffer));


    // Probably this will break if it is ever used to target a Primary surface
    // in a window with no back buffer.  We'd need to account for the window
    // offset in screen space.
    if (srcRect->right - srcRect->left == dstRect->right - dstRect->left)
    {
        ShiAssert((srcRect->bottom - srcRect->top) ==
                  (dstRect->bottom - dstRect->top));
        ShiAssert((srcRect->right - srcRect->left) ==
                  (dstRect->right - dstRect->left));

        RotateBitmapMask(
            srcBuffer, this,
            (int)(angle *
                  2607.594587618f), // 180.0f / PI * 4096 / 90.0f (convertion from radians to Erick's)
            srcRect, dstRect, startStopArray);
    }
    else
    {
        ShiAssert(2 * (srcRect->bottom - srcRect->top) ==
                  (dstRect->bottom - dstRect->top));
        ShiAssert(2 * (srcRect->right - srcRect->left) ==
                  (dstRect->right - dstRect->left));


        RotateBitmapMaskDouble(
            srcBuffer, this,
            (int)(angle *
                  2607.594587618f), // 180.0f / PI * 4096 / 90.0f (convertion from radians to Erick's)
            srcRect, dstRect, startStopArray);
    }
}

// Move this image's back buffer contents into its front buffer, possibly making it visible.
// PHASE 2 (2D UI): blits THIS CPU buffer to the D3D11 backbuffer and presents (regardless of
// m_bIsScreenBuffer). CopyToPrimary calls on Front_ (the composited UI frame).
// Bind as render target (MFD/HUD/radar content is drawn here).
void ImageBuffer::BindRttTarget(bool clear)
{
    // Artscout - 2026: #DX12 A5 -- under D3D12 the off-screen RTT is a D3D12Texture bound via BindSceneRtt.
    // ContextMPR::StartFrame calls THIS unconditionally for an off-screen IB, so the delegation keeps the
    // call site backend-agnostic (the flat D3D11 path below is untouched).
    // Artscout - 2026 (D3D11 purge): D3D12 is the sole backend -- always delegate to the D3D12 RTT bind.
    // Artscout - 2026 (GM radar): Vulkan gets its own twin (the GM sweep buffer was never bound under Vulkan,
    // so the sweep leaked onto whatever target was current and StartScene's clear wiped the cockpit atlas).
    extern bool g_bUseVulkan;
    if (g_bUseVulkan)
    {
        BindVulkanRenderTarget(clear);
        return;
    }
    BindD3D12RenderTarget(clear);
}

// Artscout - 2026 (GM radar): Vulkan twin of EnsureD3D12RenderTarget -- lazily create the off-screen
// render-target (scFormat color, RTV+SRV) sized to this buffer, via the Vulkan texture manager.
bool ImageBuffer::EnsureVulkanRenderTarget()
{
    extern bool g_bUseVulkan;
    if (!g_bUseVulkan || !g_pVulkanTextureManager)
        return false;
    if (m_bIsScreenBuffer)
        return false; // screen buffer = back buffer / eye image
    if (m_pVulkanRTT)
        return true; // already created
    if (width <= 0 || height <= 0)
        return false;
    m_pVulkanRTT = g_pVulkanTextureManager->CreateRenderTarget(width, height);
    m_bVulkanRttVirgin = true;
    return m_pVulkanRTT != NULL;
}

// Vulkan twin of BindD3D12RenderTarget: bind the off-screen RTT as the current target (displays / GM sweep
// draw into it). BindSceneRtt honors `clear` (CLEAR vs LOAD pass) -- but a virgin image is still in layout
// UNDEFINED, which only the CLEAR pass accepts, so the first bind always clears.
void ImageBuffer::BindVulkanRenderTarget(bool clear)
{
    extern VulkanBackend *g_pVulkanBackend;
    if (!g_pVulkanBackend || !EnsureVulkanRenderTarget())
        return;
    if (m_bVulkanRttVirgin)
        clear = true;
    g_pVulkanBackend->BindSceneRtt(m_pVulkanRTT, width, height, clear);
    m_bVulkanRttVirgin = false;
    extern IRenderer *g_pRenderer;
    if (g_pRenderer)
        g_pRenderer->SetViewportSize(width, height);
}

// Backend-neutral unbind (ContextMPR::FinishFrame): close the off-screen RTT pass and restore the scene
// viewport metric, whichever GPU backend owns the buffer.
void ImageBuffer::UnbindGpuRenderTarget()
{
    extern bool g_bUseVulkan;
    if (g_bUseVulkan)
    {
        extern VulkanBackend *g_pVulkanBackend;
        if (!g_pVulkanBackend || !m_pVulkanRTT)
            return;
        g_pVulkanBackend->UnbindSceneRtt(
            m_pVulkanRTT); // ends the RTT pass -> texture SHADER_READ
        // Restore gScreenSize to the scene target size, same as the D3D12 branch: any 2D screen draw after the
        // sensor/GM pass must not be mapped by the small RTT size (that mis-scale was "the whole atlas shrunk
        // into the MFD" under Vulkan).
        extern IRenderer *g_pRenderer;
        if (g_pRenderer)
            g_pRenderer->SetViewportSize(g_pVulkanBackend->SceneW(),
                                         g_pVulkanBackend->SceneH());
        return;
    }
    UnbindD3D12RenderTarget();
}

// Artscout - 2026: #DX12 A5 -- lazily create the D3D12 off-screen render-target (RGBA8, RTV+SRV) sized to
// this buffer, from the texture manager. Rest state = PIXEL_SHADER_RESOURCE (so it can be sampled/copied).
bool ImageBuffer::EnsureD3D12RenderTarget()
{
#ifdef _WIN32 // D3D12 is Windows-only; Linux uses the Vulkan path
    extern bool g_bUseD3D12;
    if (!g_bUseD3D12 || !g_pD3D12Backend || !g_pD3D12TextureManager)
        return false;
    if (m_bIsScreenBuffer)
        return false; // screen buffer = back buffer / eye image
    if (m_pD3D12RTT)
        return true; // already created
    if (width <= 0 || height <= 0)
        return false;

    D3D12Texture *t = g_pD3D12TextureManager->Alloc();
    if (!t)
        return false;
    if (!g_pD3D12TextureManager->CreateRenderTarget(*t, width, height))
    {
        g_pD3D12TextureManager->Free(t);
        return false;
    }
    m_pD3D12RTT = t;
    return true;
#else
    return false;
#endif // _WIN32
}

// #DX12 A5: bind the D3D12 off-screen RTT as the scene target (displays/sensor scene draw into it). No depth
// (the sensor path renders z-less, like the D3D11 RTT). gScreenSize = the RTT size (VS_Screen pixel->NDC).
void ImageBuffer::BindD3D12RenderTarget(bool clear)
{
#ifdef _WIN32 // D3D12 is Windows-only; Linux uses the Vulkan path
    if (!EnsureD3D12RenderTarget())
        return;
    // #DX12 A5: only bind within an already-open frame. BindSceneRtt would otherwise EnsureFrameStarted and open
    // an orphan backbuffer frame if called off the render loop (sim-update sensor/GM render) -> #527 barrier
    // mismatch + VR xrEndFrame failure (black headset). Off-frame RTT binds are simply skipped.
    if (!g_pD3D12Backend->IsRecording())
        return;
    g_pD3D12Backend->BindSceneRtt(m_pD3D12RTT, width, height, clear);
    extern IRenderer *g_pRenderer;
    if (g_pRenderer)
        g_pRenderer->SetViewportSize(width, height);
#endif // _WIN32
}

// #DX12 A5: transition the RTT back to PIXEL_SHADER_RESOURCE and rebind the scene target (back buffer / eye).
// Called by ContextMPR::FinishFrame for an off-screen IB so the main scene render can continue afterwards.
void ImageBuffer::UnbindD3D12RenderTarget()
{
#ifdef _WIN32 // D3D12 is Windows-only; Linux uses the Vulkan path
    extern bool g_bUseD3D12;
    if (!g_bUseD3D12 || !g_pD3D12Backend || !m_pD3D12RTT)
        return;
    g_pD3D12Backend->UnbindSceneRtt(
        m_pD3D12RTT); // -> PIXEL_SHADER_RESOURCE + BindBackBufferRTV
    // Restore gScreenSize to the scene target size (BindBackBufferRTV rebinds the RTV/viewport but not the
    // renderer's cbViewport) so any 2D screen draw after the sensor pass isn't mapped by the small RTT size.
    extern IRenderer *g_pRenderer;
    if (g_pRenderer)
        g_pRenderer->SetViewportSize(g_pD3D12Backend->SceneW(),
                                     g_pD3D12Backend->SceneH());
#endif // _WIN32
}

// Artscout - 2026 (#34 menu 3D-viewer): copy a [x,y,w,h] rect out of this off-screen RTT into a
// 565 CPU buffer (the on-screen UI surface). The C_3dViewer renders a model into a screen-sized
// off-screen RTT; we read its viewport rect back and stamp it into the menu's 2D surface, so the
// model appears in the normal full 2D blit instead of the present-mode chroma path (black-out).
void ImageBuffer::BlitRttTo565(unsigned short *dst, int dstStridePix,
                               int dstHeightPix, int x, int y, int w, int h)
{
    extern bool g_bUseD3D12;
    // Artscout - 2026: #DX12 A5 -- read back the D3D12 off-screen RTT (deferred: this call converts the
    // PREVIOUS frame's copy into dst and records THIS frame's copy; 1-frame latent, no mid-frame GPU stall).
    // Artscout - 2026 (D3D11 purge): D3D12 is the sole backend -- read back the D3D12 RTT (1-frame latent).
#ifdef _WIN32 // D3D12 is Windows-only; Linux uses the Vulkan path
    if (g_pD3D12Backend && dst && m_pD3D12RTT)
        g_pD3D12Backend->ReadbackRttTo565(m_pD3D12RTT, dst, dstStridePix,
                                          dstHeightPix, x, y, w, h);
#endif // _WIN32
}

// Artscout - 2026: GPU-copy this RTT's texture into another texture (same size/format). Used by the GM
// radar to snapshot a completed sweep into a persistent panel texture (CopyResource = no CPU readback).
void ImageBuffer::CopyRttTo(void *destTex2D)
{
    extern bool g_bUseD3D12;
    // Artscout - 2026: #DX12 A5 -- GM radar snapshot. Under D3D12 destTex2D is the panel handle's D3D12Texture*
    // (targetHandle->m_pDDS). GPU copy on the main list (no readback): src RTT -> dst panel texture.
    // Artscout - 2026 (D3D11 purge): D3D12 is the sole backend -- GM radar snapshot: src RTT -> dst D3D12 texture.
#ifdef _WIN32 // D3D12 is Windows-only; Linux uses the Vulkan path
    if (g_pD3D12Backend && destTex2D && m_pD3D12RTT)
        g_pD3D12Backend->CopyRtt(m_pD3D12RTT, destTex2D);
#endif // _WIN32
    // Artscout - 2026 (GM radar): Vulkan snapshot -- blit the sweep RTT into the panel's VulkanTexture.
    extern bool g_bUseVulkan;
    extern VulkanBackend *g_pVulkanBackend;
    if (g_bUseVulkan && g_pVulkanBackend && destTex2D && m_pVulkanRTT)
    {
        VulkanTexture *dst = (VulkanTexture *)destTex2D;
        g_pVulkanBackend->CopyRtt(m_pVulkanRTT, dst, width, height, dst->width,
                                  dst->height);
    }
}

void ImageBuffer::PresentGpu()
{
    // Artscout - 2026: #DX12 -- present through D3D12. A GPU frame (3D scene recorded into the command list
    // by D3D12Renderer, which set g_bGpuDraw + opened the frame via EnsureFrameStarted) is just presented;
    // a 2D frame (menu) opens a fresh frame and blits the RGB565 UI surface. NOTE: the UI composite OVER the 3D
    // (Phase-3 CompositeUISurface, black=transparent) is the next increment -- for now the 2D overlay is skipped
    // on GPU frames so the terrain shows on its own.
    if (g_bUseD3D12)
    {
#ifdef _WIN32 // D3D12 is Windows-only; Linux uses the Vulkan branch below
        if (g_pD3D12Backend)
        {
            extern bool g_bGpuDraw;
            if (g_bGpuDraw)
            {
                // #DX12 п.2: composite the 2D UI (black=transparent) over the 3D, then present. Clear the CPU
                // layer afterwards so stale overlays don't ghost (overlays are redrawn each frame).
                if (g_pD3D12Backend)
                    g_pD3D12Backend
                        ->ResolveMsaaToBackBuffer(); // MSAA: resolve 3D into backbuffer BEFORE the UI composite
                if (m_pSysMem && g_pRenderer)
                    g_pRenderer->CompositeUISurface(m_pSysMem, width, height);
                if (m_pSysMem)
                    memset(m_pSysMem, 0, (size_t)width * height * 2);
                g_pD3D12Backend->Present(
                    true); // 3D already recorded -> close/execute/present
            }
            else
            {
                g_pD3D12Backend->BeginFrame(0xFF000000);
                if (m_pSysMem)
                    g_pD3D12Backend->BlitBitmap565(m_pSysMem, width, height);
                g_pD3D12Backend->Present(true);
                // #DX12 п.5 (VR menu): feed the 2D UI surface to the XR pump (RunMenuFrame quad panel).
                extern bool g_bUseOpenXR;
                if (g_bUseOpenXR && m_pSysMem)
                {
                    extern void OpenXR_CacheMenuSurface(const void *src565,
                                                        int w, int h);
                    OpenXR_CacheMenuSurface(m_pSysMem, width, height);
                }
            }
            g_bGpuDraw = false;
        }
        return;
#endif // _WIN32
    }
    else if (g_bUseVulkan)
    {
        // Artscout - 2026 (#104): present under Vulkan. A 3D frame (g_bGpuDraw -- the renderer opened the multiview
        // scene) blits the scene to the swapchain; a 2D frame (menu) blits the RGB565 UI. Both are self-contained
        // (acquire+blit+present).
        extern bool g_bGpuDraw;
        if (g_pVulkanBackend)
        {
            // Three frame shapes, and they must be told apart -- the earlier two-way split is why the load splash was
            // invisible. A 3D frame opens the multiview scene. A splash/load frame opens NO scene but DOES record GPU
            // draws into the flat pass (DrawBitmap2D's quad); it has to be PRESENTED, because BlitBitmap565 would
            // overwrite the whole swapchain image and erase exactly what was just drawn. A menu frame has neither and
            // is pure CPU 565. D3D12 makes the same distinction with g_bGpuDraw; this branch had collapsed it to
            // "scene or 565", which sent the splash down the path that wipes it.
            if (g_pVulkanBackend->IsSceneRecording())
            {
                // #104: composite the 2D UI (HUD, black=transparent) into the scene, THEN end/blit/present it.
                extern IRenderer *g_pRenderer;
                if (m_pSysMem && g_pRenderer)
                    g_pRenderer->CompositeUISurface(m_pSysMem, width, height);
                // Clear the CPU 2D layer after compositing (parity with the D3D12 branch above). Without this the
                // surface is never zeroed, so anything drawn into it once (e.g. a full-screen 2D element at 3D entry)
                // persists every frame as an OPAQUE overlay -- it covered the 3D cockpit panel with black while the
                // per-frame HUD redrew on top. Zeroing makes undrawn regions black (transparent) again next frame.
                if (m_pSysMem)
                    memset(m_pSysMem, 0, (size_t)width * height * 2);
                g_pVulkanBackend->PresentScene();
            }
            else if (g_bGpuDraw && g_pVulkanBackend->IsRecording())
            {
                // Splash/load: GPU draws already sit in the open flat pass -- present it as-is.
                g_pVulkanBackend->Present(true);
            }
            else if (m_pSysMem)
            {
                g_pVulkanBackend->BlitBitmap565(m_pSysMem, width, height);
                // #107 VR-Vulkan: cache the 2D UI 565 for the XR menu pump (peer of the D3D12 branch). NESTED in the
                // pure-565 menu frame ONLY (mirror D3D12 lines ~622): a GPU-draw splash frame must NOT re-cache with
                // the stale m_pSysMem, else it clobbers the splash surface splash.cpp already cached -> splash invisible.
                extern bool g_bUseOpenXR;
                if (g_bUseOpenXR)
                {
                    extern void OpenXR_CacheMenuSurface(const void *src565,
                                                        int w, int h);
                    OpenXR_CacheMenuSurface(m_pSysMem, width, height);
                }
            }
        }
        g_bGpuDraw = false;
        return;
    }
    // Artscout - 2026 (D3D11 purge): D3D12 is the sole backend; the D3D11 present tail was removed.
}

void ImageBuffer::SwapBuffers(bool bDontFlip)
{
    ShiAssert(IsReady());

    // Artscout - 2026: #DX12 -- screen buffer present through D3D12 (same GPU-frame vs 2D-frame split as
    // PresentGpu above). GPU frame (3D recorded, g_bGpuDraw set) -> present as-is; 2D frame (menu) ->
    // fresh frame + blit the RGB565 UI. UI-over-3D composite = next increment.
    if (g_bUseD3D12)
    {
#ifdef _WIN32 // D3D12 is Windows-only; Linux uses the Vulkan branch below
        if (m_bIsScreenBuffer && g_pD3D12Backend)
        {
            extern bool g_bGpuDraw;
            if (g_bGpuDraw)
            {
                // #DX12 п.2: composite the 2D UI (black=transparent) over the 3D scene, then present.
                if (g_pD3D12Backend)
                    g_pD3D12Backend
                        ->ResolveMsaaToBackBuffer(); // MSAA: resolve 3D into backbuffer BEFORE the UI composite
                if (m_pSysMem && g_pRenderer)
                    g_pRenderer->CompositeUISurface(m_pSysMem, width, height);
                if (m_pSysMem)
                    memset(m_pSysMem, 0, (size_t)width * height * 2);
                g_pD3D12Backend->Present(true);
            }
            else
            {
                g_pD3D12Backend->BeginFrame(0xFF000000);
                if (m_pSysMem)
                    g_pD3D12Backend->BlitBitmap565(m_pSysMem, width, height);
                g_pD3D12Backend->Present(true);
                // #DX12 п.5 (VR menu/splash): feed the 2D UI + load-splash surface to the XR pump so the
                // headset shows the menu/splash panel instead of a blue void (mirrors the D3D11 branch below).
                extern bool g_bUseOpenXR;
                if (g_bUseOpenXR && m_pSysMem)
                {
                    extern void OpenXR_CacheMenuSurface(const void *src565,
                                                        int w, int h);
                    OpenXR_CacheMenuSurface(m_pSysMem, width, height);
                }
            }
            g_bGpuDraw = false;
        }
        return;
#endif // _WIN32
    }
    else if (g_bUseVulkan)
    {
        // Artscout - 2026 (#104): screen-buffer present under Vulkan -- the SAME three frame shapes CopyToPrimary
        // tells apart above, and for the same reason. This is the path the LOAD SPLASH takes (splash.cpp ends with
        // OTWImage->SwapBuffers, not CopyToPrimary), and it had only the two-way split: a splash frame sets
        // g_bGpuDraw but opens no scene, so it went to PresentScene() -- which has no scene to present. Fixing the
        // split in CopyToPrimary alone left the splash invisible because the splash never goes through it.
        extern bool g_bGpuDraw;
        if (m_bIsScreenBuffer && g_pVulkanBackend)
        {
            if (g_pVulkanBackend->IsSceneRecording())
            {
                extern IRenderer *g_pRenderer;
                if (m_pSysMem && g_pRenderer)
                    g_pRenderer->CompositeUISurface(m_pSysMem, width, height);
                if (m_pSysMem)
                    memset(m_pSysMem, 0, (size_t)width * height * 2);
                g_pVulkanBackend->PresentScene();
            }
            else if (g_bGpuDraw && g_pVulkanBackend->IsRecording())
                g_pVulkanBackend->Present(
                    true); // splash/load: GPU draws sit in the open flat pass -- present it
            else if (m_pSysMem)
            {
                g_pVulkanBackend->BlitBitmap565(m_pSysMem, width, height);
                // #107 VR-Vulkan: feed the 2D UI 565 surface to the XR pump so the headset shows the menu panel
                // (peer of the D3D12 branch's OpenXR_CacheMenuSurface). Without this g_pXrMenuSurface565 stays NULL and
                // RunVulkanMenuFrame submits an empty frame -> black headset in menus.
                // NESTED in the pure-565 menu frame ONLY (mirror D3D12 lines ~707): a GPU-draw splash frame (the
                // Present(true) branch above) must NOT re-cache -- its m_pSysMem is the stale/empty 565 surface (the
                // splash is drawn on the GPU via DrawBitmap2D), and re-caching CLOBBERED the good splash surface that
                // splash.cpp already cached via OpenXR_CacheMenuSurface -> splash invisible in the headset.
                extern bool g_bUseOpenXR;
                if (g_bUseOpenXR)
                {
                    extern void OpenXR_CacheMenuSurface(const void *src565,
                                                        int w, int h);
                    OpenXR_CacheMenuSurface(m_pSysMem, width, height);
                }
            }
        }
        g_bGpuDraw = false;
        return;
    }

    // Artscout - 2026 (D3D11 purge): D3D12 is the sole backend; the D3D11 screen-present tail was removed.

    // Artscout - 2026: [DX7-PURGE] DDraw Flip/Blt present removed -- the D3D11/D3D12 paths above present and return.
}

// Helpful function to drop a screen capture to disk (BACK buffer to 24 bit RAW file)
void ImageBuffer::BackBufferToRAW(char *filename)
{
    char fullname[MAX_PATH];
    HANDLE fileID;
    int r, c;
    void *imagePtr;
    BYTE *buffer;
    BYTE *p;
    UInt32 bufferSize;
    DWORD bytes;
    UInt32 color;
    RECT rect;

    // Probably this will break if it is ever used on a Primary surface
    // in a window with no back buffer.  We'd need to account for the window
    // offset in screen space.
    rect.top = 0;
    rect.left = 0;
    rect.bottom = height;
    rect.right = width;

    // Create a new RAW file
    sprintf(fullname, "%s%s", filename, ".bmp");
    fileID = CreateFile(fullname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, NULL);

    if (fileID == INVALID_HANDLE_VALUE)
    {
        // JB 010806 Don't error out.
        return;
        //char string[256];
        //PutErrorString( string );
        //strcat( string, "Failed to open screen dump file." );
        //ShiError( string );
    }

    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;
    DWORD dwBytes;

    ZeroMemory(&bih, sizeof(bih));
    bih.biSize = sizeof(bih);
    bih.biWidth = rect.right - rect.left;
    bih.biHeight = rect.bottom - rect.top;
    bih.biPlanes = 1;
    bih.biBitCount = 24;
    bih.biCompression = BI_RGB;
    bih.biSizeImage =
        ((((bih.biWidth * bih.biBitCount) + 31) bitand compl 31) >> 3) *
        bih.biHeight;

    bfh.bfType = 0x4d42;
    bfh.bfReserved1 = 0;
    bfh.bfReserved2 = 0;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = bfh.bfOffBits + bih.biSizeImage;

    // Write the header
    if ((not WriteFile(fileID, &bfh, sizeof(BITMAPFILEHEADER), &dwBytes,
                       NULL)) or
        (dwBytes not_eq sizeof(BITMAPFILEHEADER)))
    {
        char string[256];
        PutErrorString(string);
        strcat(string, "Failed to write screen dump file.");
        ShiError(string);
    }

    // Write the bitmap info header
    if ((not WriteFile(fileID, &bih, sizeof(BITMAPINFOHEADER), &dwBytes,
                       NULL)) or
        (dwBytes not_eq sizeof(BITMAPINFOHEADER)))
    {
        char string[256];
        PutErrorString(string);
        strcat(string, "Failed to write screen dump file.");
        ShiError(string);
    }

    // Create the scanline output buffer
    bufferSize = 3 * (rect.right - rect.left);
    buffer = new BYTE[bufferSize];
    ShiAssert(buffer);

    // Lock the back buffer surface
    imagePtr = Lock();
    ShiAssert(imagePtr);

    switch (PixelSize())
    {
    case 2:
    {
        WORD *pixel;

        // Step through each scanline
        for (r = rect.bottom - 1; r >= 0; r--)
        {
            // Start a new line
            p = buffer;

            // Step accross the scanline converting each pixel to RGB
            for (c = rect.left; c < rect.right; c++)
            {
                // Get the 16 bit color from the surface and convert to 32 bit ABGR
                pixel = (WORD *)Pixel(imagePtr, r, c);
                color = Pixel16toPixel32(*pixel);

                *p = (BYTE)((color bitand 0x00FF0000) >> 16);
                p++; // Blue
                *p = (BYTE)((color bitand 0x0000FF00) >> 8);
                p++; // Green
                *p = (BYTE)((color bitand 0x000000FF) >> 0);
                p++; // Red
            }

            // Write the scanline to disk
            if (not WriteFile(fileID, buffer, bufferSize, &bytes, NULL))
                bytes = 0xFFFFFFFF;

            if (bytes not_eq bufferSize)
            {
                char string[256];
                PutErrorString(string);
                strcat(string, "Couldn't write screen dump file.");
                ShiError(string);
            }
        }

        break;
    }

    case 4:
    {
        DWORD *pixel;

        // Step through each scanline
        for (r = rect.bottom - 1; r >= 0; r--)
        {
            // Start a new line
            p = buffer;

            // Step accross the scanline converting each pixel to RGB
            for (c = rect.left; c < rect.right; c++)
            {

                // Get the 16 bit color from the surface and convert to 32 bit ABGR
                pixel = (DWORD *)Pixel(imagePtr, r, c);
                color = Pixel32toPixel32(*pixel);

                *p = (BYTE)((color bitand 0x00FF0000) >> 16);
                p++; // Blue
                *p = (BYTE)((color bitand 0x0000FF00) >> 8);
                p++; // Green
                *p = (BYTE)((color bitand 0x000000FF) >> 0);
                p++; // Red
            }

            // Write the scanline to disk
            if (not WriteFile(fileID, buffer, bufferSize, &bytes, NULL))
                bytes = 0xFFFFFFFF;

            if (bytes not_eq bufferSize)
            {
                char string[256];
                PutErrorString(string);
                strcat(string, "Couldn't write screen dump file.");
                ShiError(string);
            }
        }

        break;
    }
    }

    // Unlock the back buffer
    Unlock();

    // Close the output RAW file and free the output buffer
    CloseHandle(fileID);
    delete[] buffer;
}

// OW
void ImageBuffer::RestoreAll()
{
    ShiAssert(IsReady());

    // Artscout - 2026: [DX7-PURGE] DDraw surface IsLost/Restore removed (no DDraw surfaces under GPU).
}


//Wombat778 10-06-2003 Added to allow clearing of an imagebuffer with the specified color

void ImageBuffer::Clear(UInt32 color)
{
    void *imageptr;
    WORD *pixel;

    int width = targetXres();
    int height = targetYres();

    imageptr = Lock();
    ShiAssert(imageptr);

    for (int r = 0; r < height; r++)
    {
        for (int c = 0; c < width; c++)
        {
            pixel = (WORD *)Pixel(imageptr, r, c);
            *pixel = Pixel32toPixel16(color);
        }
    }

    Unlock();
}
