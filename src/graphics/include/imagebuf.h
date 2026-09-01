/***************************************************************************\
    ImageBuf.h
    Scott Randolph
    December 29, 1995

    This class provides management for drawing target buffers and sources
 for blit operations.
\***************************************************************************/
#ifndef _IMAGEBUF_H_
#define _IMAGEBUF_H_

#ifdef USE_SMART_HEAP
#include <stdlib.h>
#include "smartheap/include/smrtheap.hpp"
#endif
#include "../../codelib/include/shi/shierror.h"

class DisplayDevice;
enum MPRSurfaceType;

#include "d3d7compat.h"
#include "context.h"

class ImageBuffer
{
public:
    // Constructor/Destructor for this buffer
    ImageBuffer();
    virtual ~ImageBuffer();

    // Functions used to set up and manage this buffer
    BOOL Setup(DisplayDevice *dev, int width, int height, MPRSurfaceType front,
               MPRSurfaceType back, HWND targetWin = NULL, BOOL clip = FALSE,
               bool fullScreen = false, BOOL bWillCallSwapBuffer = FALSE);
    void Cleanup();
    BOOL IsReady();
    void SetChromaKey(UInt32 RGBcolor);
    void UpdateFrontWindowRect(RECT *rect);
    void AttachSurfaces(DisplayDevice *pDev, IDirectDrawSurface7 *pDDSFront,
                        IDirectDrawSurface7 *pDDSBack);

    // Used to get the properties of the under lying surfaces
    DisplayDevice *GetDisplayDevice()
    {
        return device;
    };
    IDirectDrawSurface7 *targetSurface()
    {
        return m_pDDSBack;
    };
    IDirectDrawSurface7 *frontSurface()
    {
        return m_pDDSFront;
    };

    int targetXres()
    {
        return width;
    };
    int targetYres()
    {
        return height;
    };
    int targetStride()
    {
        return m_ddsdBack.lPitch;
    };

    void GetColorMasks(UInt32 *r, UInt32 *g, UInt32 *b);
    UInt32 RedMask(void)
    {
        return m_ddsdFront.ddpfPixelFormat.dwRBitMask;
    };
    UInt32 GreenMask(void)
    {
        return m_ddsdFront.ddpfPixelFormat.dwGBitMask;
    };
    UInt32 BlueMask(void)
    {
        return m_ddsdFront.ddpfPixelFormat.dwBBitMask;
    };
    int RedShift(void)
    {
        return redShift;
    };
    int GreenShift(void)
    {
        return greenShift;
    };
    int BlueShift(void)
    {
        return blueShift;
    };
    int PixelSize()
    {
        return m_ddsdFront.ddpfPixelFormat.dwRGBBitCount >> 3;
    };

    void RestoreAll(); // OW

    // Used to allow direct pixel access to the back buffer
    void *Lock(bool bDontLockBits = false, bool bWriteOnly = true);
    void Unlock();

    // Used to convert from a 32 bit color to a 16 bit pixel (assumes 16 bit display mode)
    UInt16 Pixel32toPixel16(UInt32 ABGR);
    UInt32 Pixel16toPixel32(UInt16 pixel);
    UInt32 Pixel32toPixel32(UInt32 pixel);

    // Used to write a pixel to the back buffer surface (requires a Lock)
    void *Pixel(void *ptr, int row, int col)
    {
        ShiAssert(ptr);
        return (BYTE *)ptr + row * m_ddsdBack.lPitch + col * PixelSize();
    };

    // Used to control image compositing
    void Compose(ImageBuffer *srcBuffer, RECT *srcRect, RECT *dstRect);
    void ComposeTransparent(ImageBuffer *srcBuffer, RECT *srcRect,
                            RECT *dstRect);

    // Used to control image compositing with a clockwise rotation (in radians)
    void ComposeRot(ImageBuffer *srcBuffer, RECT *srcRect, RECT *dstRect,
                    float angle);
    void ComposeRoundRot(ImageBuffer *srcBuffer, RECT *srcRect, RECT *dstRect,
                         float angle, int *startStopArray);

    // Swap rolls of front and back buffers (page flip, blt, or nothing depending on types)
    void SwapBuffers(bool bDontFlip);
    void PresentGpu(); // blit the CPU RGB565 buffer into the GPU backbuffer

    // Off-screen render target for MFD/HUD/radar (backed by the D3D12 RTT below).
    bool IsScreenBuffer() const
    {
        return m_bIsScreenBuffer;
    }
    // Binds the RTT as render target (+viewport over the whole buffer) and optionally clears.
    void BindRttTarget(bool clear);
    // Artscout - 2026: GPU-copy this off-screen RTT's texture into another backend texture (same
    // size/format). Used by the GM radar to SNAPSHOT a completed sweep into a persistent panel texture.
    // #DX12 A5: under D3D12 destTex2D is a D3D12Texture* (targetHandle->m_pDDS); branches internally.
    void CopyRttTo(void *destTex2D);
    // Artscout - 2026: menu 3D-viewer (#34). Read a rect out of this off-screen RTT and convert
    // it into a 565 CPU buffer (the screen UI surface), so a 3D model preview becomes part of the
    // normal 2D blit instead of going through the present-mode chroma path (which blacks it out).
    // #DX12 A5: under D3D12 this reads back the D3D12 off-screen RTT (deferred, 1-frame latent).
    void BlitRttTo565(unsigned short *dst, int dstStridePix, int dstHeightPix,
                      int x, int y, int w, int h);

    // Artscout - 2026: #DX12 A5 -- D3D12 twin of the D3D11 off-screen RTT (TGP/FLIR/Munitions viewer +
    // GM radar). Create a D3D12 render-target texture (via the texture manager), bind it as the scene
    // target (BindSceneRtt), and on FinishFrame transition it back to PIXEL_SHADER_RESOURCE + rebind the
    // scene. BindRttTarget delegates here when g_bUseD3D12, so ContextMPR::StartFrame is untouched.
    bool EnsureD3D12RenderTarget();
    void BindD3D12RenderTarget(bool clear);
    void UnbindD3D12RenderTarget();
    void *GetD3D12RTT() const
    {
        return m_pD3D12RTT;
    } // D3D12Texture* (NULL if not created)

    // Artscout - 2026 (GM radar): Vulkan twins + a backend-neutral unbind (ContextMPR::FinishFrame calls it
    // for any GPU backend; it routes to the D3D12 or Vulkan implementation).
    bool EnsureVulkanRenderTarget();
    void BindVulkanRenderTarget(bool clear);
    void UnbindGpuRenderTarget();

    // Helpful function to drop a screen capture to disk (BACK buffer to 24 bit RAW file)
    void BackBufferToRAW(char *filename);

    //Wombat778 10-06-2003
    void Clear(UInt32 color);

protected:
    void ComputeColorShifts();

protected:
    BOOL m_bReady;
    DisplayDevice *device;
    IDirectDrawSurface7 *m_pDDSFront;
    DDSURFACEDESC2 m_ddsdFront;
    HWND m_hWnd;
    RECT m_rcFront;
    bool m_bFrontRectValid;
    IDirectDrawSurface7 *m_pDDSBack;
    DDSURFACEDESC2 m_ddsdBack;
    int width;
    int height;
    bool m_bIsScreenBuffer; // PHASE 1: screen buffer (drives Present)
    BYTE *
        m_pSysMem; // PHASE 1/2: RGB565 CPU buffer (GPU composites/presents from it)
    int redShift;
    int greenShift;
    int blueShift;
    DWORD m_dwColorKey;
    IDirectDrawSurface7 *m_pBltTarget;
    CRITICAL_SECTION m_cs;
    bool m_bBitsLocked;

    // Artscout - 2026: #DX12 A5 -- D3D12 off-screen RTT (a D3D12Texture*, kept opaque as void* so this
    // widely-included header stays free of d3d12 headers). Created lazily by EnsureD3D12RenderTarget.
    void *m_pD3D12RTT;
    // Artscout - 2026 (GM radar): Vulkan off-screen RTT twin (a VulkanTexture*, opaque for the same reason).
    // m_bVulkanRttVirgin: image never rendered to yet -> the first bind must CLEAR (layout UNDEFINED; the
    // LOAD render pass requires SHADER_READ, which only exists after one completed pass).
    void *m_pVulkanRTT;
    bool m_bVulkanRttVirgin;
};


#endif // _IMAGEBUF_H_
