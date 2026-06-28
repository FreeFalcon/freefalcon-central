/***************************************************************************\
    ImageBuf.cpp
    Scott Randolph
    December 29, 1995

    This class provides management for drawing target buffers and sources
 for blit operations.
\***************************************************************************/

#include <cISO646>
#include "stdafx.h"
#include "Rotate.h"
#include "Device.h"
#include "ImageBuf.h"
#include "Graphics/DXEngine/D3D11Backend.h"	// PHASE 1: D3D11 backend
#include "Graphics/DXEngine/OpenXRBackend.h"	// VR (OpenXR)
#include "../../sim/INCLUDE/ivibedata.h"	// VR: g_intellivibeData.In3D (menu vs sim)
#include "Graphics/DXEngine/d3d11/D3D11Renderer.h"	// PHASE 5: composite the 2D UI over 3D
#include <d3d11.h>	// PHASE 5 (RTT): off-screen render target
#include "FalcLib/include/debuggr.h"
#include "Falclib/Include/IsBad.h"
//#define _IMAGEBUFFER_PROTECT_SURF_LOCK
extern bool g_bCheckBltStatusBeforeFlip;

#ifdef _DEBUG
static char *arrType2String[] =
{
    "SystemMem", "VideoMem",
    "Primary",
    "Flip", "None",
    "LocalVideoMem",
    "LocalVideoMem3D",
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

    // PHASE 5 (RTT)
    m_pD3D11RTTex = NULL;
    m_pD3D11RTV = NULL;
    m_pD3D11SRV = NULL;
    m_pD3D11Staging = NULL;

#ifdef _IMAGEBUFFER_PROTECT_SURF_LOCK
    InitializeCriticalSection(&m_cs);
#endif
}

ImageBuffer::~ImageBuffer()
{
    ShiAssert( not IsReady());
    Cleanup(); // OW

#ifdef _IMAGEBUFFER_PROTECT_SURF_LOCK
    DeleteCriticalSection(&m_cs);
#endif
}

BOOL ImageBuffer::IsReady()
{
    return m_bReady;
}

BOOL ImageBuffer::Setup(DisplayDevice *dev, int w, int h, MPRSurfaceType front, MPRSurfaceType back, HWND targetWin, BOOL clip, bool fullScreen, BOOL bWillCallSwapBuffer)
{
    ZeroMemory(&m_rcFront, sizeof(m_rcFront));

    try
    {
        ShiAssert( not IsReady());
        ShiAssert(dev);


        // Record the properties of the buffer(s) we're creating
        device = dev;
        width = w;
        height = h;

        // PHASE 1 (D3D7->D3D11): no DDraw surfaces. CPU buffer 16-bit RGB565, the UI composites
        // via Lock; the screen buffer (front==Primary) drives Present.
        extern bool g_bUseD3D11;
        if (g_bUseD3D11)
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
            if (m_pSysMem) { free(m_pSysMem); m_pSysMem = NULL; }   // #55 don't leak on a repeated Setup without Cleanup
            m_pSysMem = (BYTE*)malloc((size_t)width * height * 2);
            if (m_pSysMem) memset(m_pSysMem, 0, (size_t)width * height * 2);
            m_bReady = TRUE;
            return TRUE;
        }


        IDirectDraw7Ptr pDD(dev->GetMPRdevice());

        DDSURFACEDESC2 ddsd;
        ZeroMemory(&ddsd, sizeof(ddsd));
        ddsd.dwFlags = DDSD_CAPS;
        ddsd.dwSize = sizeof(ddsd);

        switch (front)
        {
            case Primary:
            {
                switch (back)
                {
                    case SystemMem:
                    {
                        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
                        CheckHR(pDD->CreateSurface(&ddsd, &m_pDDSFront, NULL));

                        ddsd.dwFlags or_eq DDSD_WIDTH bitor DDSD_HEIGHT;
                        ddsd.dwWidth  = w;
                        ddsd.dwHeight = h;
                        ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY bitor DDSCAPS_3DDEVICE;
                        CheckHR(pDD->CreateSurface(&ddsd, &m_pDDSBack, NULL));

                        break;
                    }

                    case VideoMem:
                    {
                        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
                        CheckHR(pDD->CreateSurface(&ddsd, &m_pDDSFront, NULL));

                        ddsd.dwFlags or_eq DDSD_WIDTH bitor DDSD_HEIGHT;
                        ddsd.dwWidth  = w;
                        ddsd.dwHeight = h;
                        ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY bitor DDSCAPS_3DDEVICE;
                        CheckHR(pDD->CreateSurface(&ddsd, &m_pDDSBack, NULL));

                        break;
                    }

                    case Flip:
                    {
                        if ( not fullScreen)
                        {
                            ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
                            CheckHR(pDD->CreateSurface(&ddsd, &m_pDDSFront, NULL));

                            ddsd.dwFlags or_eq DDSD_WIDTH bitor DDSD_HEIGHT;
                            ddsd.dwWidth  = w;
                            ddsd.dwHeight = h;
                            ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY bitor DDSCAPS_3DDEVICE;
                            CheckHR(pDD->CreateSurface(&ddsd, &m_pDDSBack, NULL));
                        }

                        else
                        {
                            // Create the primary surface.
                            ddsd.dwFlags or_eq DDSD_BACKBUFFERCOUNT;
                            ddsd.ddsCaps.dwCaps or_eq DDSCAPS_PRIMARYSURFACE bitor DDSCAPS_3DDEVICE bitor DDSCAPS_COMPLEX bitor DDSCAPS_FLIP;
                            ddsd.dwBackBufferCount = 1;
                            CheckHR(pDD->CreateSurface(&ddsd, &m_pDDSFront, NULL));

                            // Get the attached backbuffer surface
                            DDSCAPS2 ddscaps;
                            ZeroMemory(&ddscaps, sizeof(ddscaps));
                            ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
                            CheckHR(m_pDDSFront->GetAttachedSurface(&ddscaps, &m_pDDSBack));
                        }

                        break;
                    }

                    case None:
                    {
                        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE bitor DDSCAPS_3DDEVICE;
                        CheckHR(pDD->CreateSurface(&ddsd, &m_pDDSFront, NULL));

                        CheckHR(m_pDDSFront->QueryInterface(IID_IDirectDrawSurface7, (void **) &m_pDDSBack));

                        break;
                    }

                    default:
                    {
                        ShiAssert(false); // illegal combination
                        return FALSE;
                    }
                }

                // auto adjust front rect when creating the primary surface in windowed mode
                if ( not fullScreen)
                {
                    GetClientRect(targetWin, &m_rcFront);
                    ClientToScreen(targetWin, (LPPOINT)&m_rcFront);
                    ClientToScreen(targetWin, (LPPOINT)&m_rcFront + 1);
                    m_bFrontRectValid = true;
                }

                break;
            }

            case SystemMem:
            {
                switch (back)
                {
                    case SystemMem:
                    {
                        ShiAssert(false); // not implemented yet
                        break;
                    }

                    case VideoMem:
                    {
                        ShiAssert(false); // not implemented yet
                        break;
                    }

                    case Flip:
                    {
                        ShiAssert(false); // not implemented yet
                        break;
                    }

                    case None:
                    {
                        ddsd.dwFlags or_eq DDSD_WIDTH bitor DDSD_HEIGHT;
                        ddsd.dwWidth  = w;
                        ddsd.dwHeight = h;
                        ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY bitor DDSCAPS_3DDEVICE;
                        CheckHR(pDD->CreateSurface(&ddsd, &m_pDDSFront, NULL));

                        CheckHR(m_pDDSFront->QueryInterface(IID_IDirectDrawSurface7, (void **) &m_pDDSBack));

                        break;
                    }

                    default:
                    {
                        ShiAssert(false); // illegal combination
                        return FALSE;
                    }
                }

                break;
            }

            case VideoMem:
            {
                switch (back)
                {
                    case SystemMem:
                    {
                        ShiAssert(false); // not implemented yet
                        break;
                    }

                    case VideoMem:
                    {
                        ShiAssert(false); // not implemented yet
                        break;
                    }

                    case Flip:
                    {
                        ShiAssert(false); // not implemented yet
                        break;
                    }

                    case None:
                    {
                        ddsd.dwFlags or_eq DDSD_WIDTH bitor DDSD_HEIGHT;
                        ddsd.dwWidth  = w;
                        ddsd.dwHeight = h;
                        ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY bitor DDSCAPS_3DDEVICE;
                        HRESULT hr = pDD->CreateSurface(&ddsd, &m_pDDSFront, NULL);

                        if (FAILED(hr))
                        {
                            if (hr == DDERR_OUTOFVIDEOMEMORY)
                            {
                                MonoPrint("ImageBuffer::Setup - EVICTING MANAGED TEXTURES \n");

                                // if we are out of video memory, evict all managed textures and retry
                                CheckHR(dev->GetDefaultRC()->m_pD3D->EvictManagedTextures());
                                CheckHR(pDD->CreateSurface(&ddsd, &m_pDDSFront, NULL));
                            }

                            else throw _com_error(hr);
                        }

                        CheckHR(m_pDDSFront->QueryInterface(IID_IDirectDrawSurface7, (void **) &m_pDDSBack));

                        break;
                    }

                    default:
                    {
                        ShiAssert(false); // illegal combination
                        return FALSE;
                    }
                }

                break;
            }

            case LocalVideoMem:
            case LocalVideoMem3D:
            {
                switch (back)
                {
                    case SystemMem:
                    {
                        ShiAssert(false); // not implemented yet
                        break;
                    }

                    case VideoMem:
                    {
                        ShiAssert(false); // not implemented yet
                        break;
                    }

                    case Flip:
                    {
                        ShiAssert(false); // not implemented yet
                        break;
                    }

                    case None:
                    {
                        ddsd.dwFlags or_eq DDSD_WIDTH bitor DDSD_HEIGHT;
                        ddsd.dwWidth  = w;
                        ddsd.dwHeight = h;
                        ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY bitor DDSCAPS_LOCALVIDMEM;

                        if (front == LocalVideoMem3D)
                            ddsd.ddsCaps.dwCaps or_eq DDSCAPS_3DDEVICE;
                        else
                            ddsd.ddsCaps.dwCaps or_eq DDSCAPS_OFFSCREENPLAIN;

                        HRESULT hr = pDD->CreateSurface(&ddsd, &m_pDDSFront, NULL);

                        if (FAILED(hr))
                        {
                            if (hr == DDERR_OUTOFVIDEOMEMORY)
                            {
                                MonoPrint("ImageBuffer::Setup - EVICTING MANAGED TEXTURES \n");

                                // if we are out of video memory, evict all managed textures and retry
                                CheckHR(dev->GetDefaultRC()->m_pD3D->EvictManagedTextures());
                                CheckHR(pDD->CreateSurface(&ddsd, &m_pDDSFront, NULL));
                            }

                            else throw _com_error(hr);
                        }

                        CheckHR(m_pDDSFront->QueryInterface(IID_IDirectDrawSurface7, (void **) &m_pDDSBack));

                        break;
                    }

                    default:
                    {
                        ShiAssert(false); // illegal combination
                        return FALSE;
                    }
                }

                break;
            }

            /*
             case Flip:
             {
             switch(back)
             {
             case SystemMem:
             {
             break;
             }

             case VideoMem:
             {
             break;
             }

             case Flip:
             {
             break;
             }

             case None:
             {
             break;
             }

             default:
             {
             ShiAssert(false); // illegal combination
             return FALSE;
             }
             }

             break;
             }
            */

            case None: // Note: This doesnt create a 3D surface and it lets the driver decide where to put it)
            {
                switch (back)
                {
                    case SystemMem:
                    {
                        ShiAssert(false); // not implemented yet
                        break;
                    }

                    case VideoMem:
                    {
                        ShiAssert(false); // not implemented yet
                        break;
                    }

                    case Flip:
                    {
                        ShiAssert(false); // not implemented yet
                        break;
                    }

                    case None:
                    {
                        ddsd.dwFlags or_eq DDSD_WIDTH bitor DDSD_HEIGHT;
                        ddsd.dwWidth  = w;
                        ddsd.dwHeight = h;
                        ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
                        CheckHR(pDD->CreateSurface(&ddsd, &m_pDDSFront, NULL));

                        CheckHR(m_pDDSFront->QueryInterface(IID_IDirectDrawSurface7, (void **) &m_pDDSBack));

                        break;
                    }

                    default:
                    {
                        ShiAssert(false); // illegal combination
                        return FALSE;
                    }
                }

                break;
            }

            default:
            {
                ShiAssert(false); // illegal combination
                return FALSE;
            }
        }

        if (clip and m_pDDSFront)
        {
            ShiAssert( not fullScreen);
            IDirectDrawClipperPtr pDDCLP;
            CheckHR(pDD->CreateClipper(0, &pDDCLP, NULL));
            CheckHR(pDDCLP->SetHWnd(0, targetWin));
            CheckHR(m_pDDSFront->SetClipper(pDDCLP));
        }

        // Compute the color format conversion parameters
        m_ddsdFront.dwSize = sizeof(DDSURFACEDESC2);
        CheckHR(m_pDDSFront->GetSurfaceDesc(&m_ddsdFront));
        m_ddsdBack.dwSize = sizeof(DDSURFACEDESC2);
        CheckHR(m_pDDSBack->GetSurfaceDesc(&m_ddsdBack));

        /* #ifdef _DEBUG
         if(back == None)
        // MonoPrint("ImageBuffer::Setup - %dx%d front (%s) created in %s memory\n",
        // m_ddsdFront.dwWidth, m_ddsdFront.dwHeight, arrType2String[front],
        // m_ddsdFront.ddsCaps.dwCaps bitand DDSCAPS_SYSTEMMEMORY  ? "SYSTEM" :
        // (m_ddsdFront.ddsCaps.dwCaps bitand DDSCAPS_LOCALVIDMEM ? "VIDEO" : "AGP"));
         else
        /* MonoPrint("ImageBuffer::Setup - %dx%d front (%s) created in %s memory, back (%s) created in %s memory\n",
         m_ddsdFront.dwWidth, m_ddsdFront.dwHeight,
         arrType2String[front],
         m_ddsdFront.ddsCaps.dwCaps bitand DDSCAPS_SYSTEMMEMORY  ? "SYSTEM" :
         (m_ddsdFront.ddsCaps.dwCaps bitand DDSCAPS_LOCALVIDMEM ? "VIDEO" : "AGP"),
          arrType2String[back],
         m_ddsdBack.ddsCaps.dwCaps bitand DDSCAPS_SYSTEMMEMORY  ? "SYSTEM" :
         (m_ddsdBack.ddsCaps.dwCaps bitand DDSCAPS_LOCALVIDMEM ? "VIDEO" : "AGP"));
         #endif
        */
        // Set blt target surface depending on whether we will page flip or not
        if (bWillCallSwapBuffer)
            m_pBltTarget = m_pDDSBack;
        else
            m_pBltTarget = m_pDDSFront;

        ComputeColorShifts();

        // Everything worked, so finish up and return
        m_bReady = TRUE;

        return TRUE;
    }

    catch (const _com_error &e)
    {
        MonoPrint("ImageBuffer::Setup - Error 0x%X\n", e.Error());
        return FALSE;
    }
}

// Alternative setup method
void ImageBuffer::AttachSurfaces(DisplayDevice *pDev, IDirectDrawSurface7 *pDDSFront, IDirectDrawSurface7 *pDDSBack)
{
    if ( not pDev or not pDDSFront)
        return;

    if ( not pDDSBack)
        pDDSBack = pDDSFront;

    try
    {
        CheckHR(pDDSFront->QueryInterface(IID_IDirectDrawSurface7, (void **) &m_pDDSFront));
        CheckHR(pDDSBack->QueryInterface(IID_IDirectDrawSurface7, (void **) &m_pDDSBack));

        // Compute the color format conversion parameters
        m_ddsdFront.dwSize = sizeof(DDSURFACEDESC2);
        CheckHR(m_pDDSFront->GetSurfaceDesc(&m_ddsdFront));
        m_ddsdBack.dwSize = sizeof(DDSURFACEDESC2);
        CheckHR(m_pDDSBack->GetSurfaceDesc(&m_ddsdBack));

        // Set blt target surface depending on whether we will page flip or not
        m_pBltTarget = m_pDDSFront;

        // Record the properties of the buffer(s) we're creating
        device = pDev;
        width = m_ddsdFront.dwWidth;
        height = m_ddsdFront.dwHeight;

        m_bFrontRectValid = false;
        ZeroMemory(&m_rcFront, sizeof(m_rcFront));

        ComputeColorShifts();

        // Everything worked, so finish up and return
        m_bReady = TRUE;
    }

    catch (const _com_error &e)
    {
        MonoPrint("ImageBuffer::AttachSurfaces - Error 0x%X\n", e.Error());
    }
}


// Release the DDraw surfaces associated with this object
void ImageBuffer::Cleanup(void)
{
    m_bReady = FALSE;

    if (m_pDDSBack) // MUST be released before releasing the front buffer
    {
        m_pDDSBack->Release();
        m_pDDSBack = NULL;
    }

    // Destroy our surface (including attached back buffer)
    if (m_pDDSFront)
    {
        m_pDDSFront->Release();
        m_pDDSFront = NULL;
    }

    m_pBltTarget = NULL;

    // PHASE 5 (RTT): release the off-screen render target
    if (m_pD3D11SRV)     { m_pD3D11SRV->Release();     m_pD3D11SRV = NULL; }
    if (m_pD3D11RTV)     { m_pD3D11RTV->Release();     m_pD3D11RTV = NULL; }
    if (m_pD3D11RTTex)   { m_pD3D11RTTex->Release();   m_pD3D11RTTex = NULL; }
    if (m_pD3D11Staging) { m_pD3D11Staging->Release(); m_pD3D11Staging = NULL; }

    // #55 MEMORY-LEAK ROOT on 3D enter/exit: the D3D11 surface CPU buffer (565), malloc'd in
    // Setup() line 101, was NEVER freed -> every ImageBuffer (cockpit 2x35MB, display 7MB,
    // MFD...) leaked its backing buffer on EVERY entry -> ~70+MB/cycle -> std::bad_alloc.
    // Cleanup is called from ~ImageBuffer -> free it here.
    if (m_pSysMem) { free(m_pSysMem); m_pSysMem = NULL; }
}


// Compute the right shifts required to get from 24 bit RGB to this pixel format
void ImageBuffer::ComputeColorShifts(void)
{
    UInt32 mask;

    // RED
    mask = m_ddsdFront.ddpfPixelFormat.dwRBitMask;
    redShift = 8;
    ShiAssert(mask);

    while ( not (mask bitand 1))
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

    while ( not (mask bitand 1))
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

    while ( not (mask bitand 1))
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

void ImageBuffer::GetColorMasks(UInt32 *r, UInt32 *g, UInt32* b)
{
    *r = m_ddsdFront.ddpfPixelFormat.dwRBitMask;
    *g = m_ddsdFront.ddpfPixelFormat.dwGBitMask;
    *b = m_ddsdFront.ddpfPixelFormat.dwBBitMask;
}

// Adjust the offset into the primary surface if we are windowed
void ImageBuffer::UpdateFrontWindowRect(RECT *rect)
{
    // ShiAssert( frontType == Primary ); // This is only useful for the primary surface
    if (rect) m_rcFront = *rect;

    m_bFrontRectValid = rect and (m_rcFront.left or m_rcFront.right);
}

// Fix in memory and return and pointer to the memory associated with our back buffer
void *ImageBuffer::Lock(bool bLockMutexOnly, bool bWriteOnly)
{
    extern bool g_bUseD3D11;
    if (g_bUseD3D11) return m_pSysMem;	// PHASE 1/2: CPU buffer; no DDraw
#ifdef _IMAGEBUFFER_PROTECT_SURF_LOCK
    EnterCriticalSection(&m_cs);

    if (bLockMutexOnly)
        return NULL;

#endif

    ShiAssert(IsReady());

    HRESULT hr;

    DDSURFACEDESC2 ddsd;
    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);

    // DWORD dwFlags = DDLOCK_NOSYSLOCK bitor DDLOCK_WAIT bitor DDLOCK_SURFACEMEMORYPTR;
    DWORD dwFlags = DDLOCK_WAIT bitor DDLOCK_SURFACEMEMORYPTR;

    if (bWriteOnly) dwFlags or_eq DDLOCK_WRITEONLY;

    int nRetries = 1;

Retry:
    hr = m_pDDSBack->Lock(NULL, &ddsd, dwFlags, NULL);
    m_bBitsLocked = SUCCEEDED(hr);

    if (FAILED(hr))
    {
        MonoPrint("ImageBuffer::Lock - Lock failed with 0x%X\n", hr);

        if (hr == DDERR_SURFACELOST and nRetries)
        {
            RestoreAll();
            nRetries--;
            goto Retry;
        }
    }

    return ddsd.lpSurface;
}

// Reliquish our exclusive pointer to the memory associated with our back buffer
void ImageBuffer::Unlock()
{
    extern bool g_bUseD3D11;
    if (g_bUseD3D11) return;	// PHASE 1/2: no DDraw Unlock
    ShiAssert(IsReady());

    if (m_bBitsLocked)
    {
        HRESULT hr = m_pDDSBack->Unlock(NULL);
        ShiAssert(SUCCEEDED(hr));
        m_bBitsLocked = false;
    }

#ifdef _IMAGEBUFFER_PROTECT_SURF_LOCK
    LeaveCriticalSection(&m_cs);
#endif
}

// Set the color key for this surface to be used when it is transparently
// composed into another buffer.
void ImageBuffer::SetChromaKey(UInt32 colorKey)
{
    if ( not m_pDDSFront) // JB 010404 CTD
        return;

    ShiAssert(IsReady());

    // Convert the key color from 32 bit RGB to the current pixel format

    // RED
    if (redShift >= 0)
        m_dwColorKey = (colorKey >>  redShift) bitand m_ddsdFront.ddpfPixelFormat.dwRBitMask;
    else
        m_dwColorKey = (colorKey << -redShift) bitand m_ddsdFront.ddpfPixelFormat.dwRBitMask;

    // GREEN
    if (greenShift >= 0)
        m_dwColorKey or_eq (colorKey >>  greenShift) bitand m_ddsdFront.ddpfPixelFormat.dwGBitMask;
    else
        m_dwColorKey or_eq (colorKey << -greenShift) bitand m_ddsdFront.ddpfPixelFormat.dwGBitMask;

    // BLUE
    if (blueShift >= 0)
        m_dwColorKey or_eq (colorKey >>  blueShift) bitand m_ddsdFront.ddpfPixelFormat.dwBBitMask;
    else
        m_dwColorKey or_eq (colorKey << -blueShift) bitand m_ddsdFront.ddpfPixelFormat.dwBBitMask;

    DDCOLORKEY key = { m_dwColorKey, m_dwColorKey};

    // Submit the request to DirectDraw
    HRESULT hr = m_pDDSFront->SetColorKey(DDCKEY_SRCBLT, &key);
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
        color = (ABGR >>  redShift) bitand m_ddsdFront.ddpfPixelFormat.dwRBitMask;
    }
    else
    {
        color = (ABGR << -redShift) bitand m_ddsdFront.ddpfPixelFormat.dwRBitMask;
    }

    // GREEN
    if (greenShift >= 0)
    {
        color or_eq (ABGR >>  greenShift) bitand m_ddsdFront.ddpfPixelFormat.dwGBitMask;
    }
    else
    {
        color or_eq (ABGR << -greenShift) bitand m_ddsdFront.ddpfPixelFormat.dwGBitMask;
    }

    // BLUE
    if (blueShift >= 0)
    {
        color or_eq (ABGR >>  blueShift) bitand m_ddsdFront.ddpfPixelFormat.dwBBitMask;
    }
    else
    {
        color or_eq (ABGR << -blueShift) bitand m_ddsdFront.ddpfPixelFormat.dwBBitMask;
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
        color = (ABGR >>  redShift) bitand m_ddsdFront.ddpfPixelFormat.dwRBitMask;
    }
    else
    {
        color = (ABGR << -redShift) bitand m_ddsdFront.ddpfPixelFormat.dwRBitMask;
    }

    // GREEN
    if (greenShift >= 0)
    {
        color or_eq (ABGR >>  greenShift) bitand m_ddsdFront.ddpfPixelFormat.dwGBitMask;
    }
    else
    {
        color or_eq (ABGR << -greenShift) bitand m_ddsdFront.ddpfPixelFormat.dwGBitMask;
    }

    // BLUE
    if (blueShift >= 0)
    {
        color or_eq (ABGR >>  blueShift) bitand m_ddsdFront.ddpfPixelFormat.dwBBitMask;
    }
    else
    {
        color or_eq (ABGR << -blueShift) bitand m_ddsdFront.ddpfPixelFormat.dwBBitMask;
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
        color = (pixel bitand m_ddsdFront.ddpfPixelFormat.dwRBitMask) <<  redShift;
    }
    else
    {
        color = (pixel bitand m_ddsdFront.ddpfPixelFormat.dwRBitMask) >> -redShift;
    }

    // GREEN
    if (greenShift >= 0)
    {
        color or_eq (pixel bitand m_ddsdFront.ddpfPixelFormat.dwGBitMask) <<  greenShift;
    }
    else
    {
        color or_eq (pixel bitand m_ddsdFront.ddpfPixelFormat.dwGBitMask) >> -greenShift;
    }

    // BLUE
    if (blueShift >= 0)
    {
        color or_eq (pixel bitand m_ddsdFront.ddpfPixelFormat.dwBBitMask) <<  blueShift;
    }
    else
    {
        color or_eq (pixel bitand m_ddsdFront.ddpfPixelFormat.dwBBitMask) >> -blueShift;
    }

    return color;
}

// Copy a retangular area from the source image's front buffer to the this images's
// back buffer.  Both rectangles must be entirely inside their respective buffers.
void ImageBuffer::Compose(ImageBuffer *srcBuffer, RECT *dstRect, RECT *srcRect)
{
    ShiAssert(IsReady());
    ShiAssert(FALSE == F4IsBadReadPtr(srcRect, sizeof * srcRect));
    ShiAssert(FALSE == F4IsBadReadPtr(dstRect, sizeof * dstRect));
    ShiAssert(FALSE == F4IsBadReadPtr(srcBuffer, sizeof * srcBuffer));


    bool bStretch = ((srcRect->right - srcRect->left) not_eq (dstRect->right - dstRect->left)) or ((srcRect->bottom - srcRect->top) not_eq (dstRect->bottom - dstRect->top));
    HRESULT hr;

    if ( not m_bFrontRectValid and not bStretch)
    {
        if (srcRect and m_pBltTarget not_eq m_pDDSBack)
        {
            RECT rcSrc = *srcRect;
            rcSrc.left += m_rcFront.left;
            rcSrc.top += m_rcFront.top;

            hr = m_pBltTarget->BltFast(rcSrc.left, rcSrc.top, srcBuffer->m_pDDSBack, dstRect, DDBLTFAST_WAIT bitor DDBLTFAST_NOCOLORKEY);
        }

        else hr = m_pBltTarget->BltFast(srcRect->left, srcRect->top, srcBuffer->m_pDDSBack, dstRect, DDBLTFAST_WAIT bitor DDBLTFAST_NOCOLORKEY);
    }

    else
    {
        if (m_bFrontRectValid and srcRect and m_pBltTarget not_eq m_pDDSBack)
        {
            RECT rcSrc = *srcRect;
            rcSrc.left += m_rcFront.left;
            rcSrc.right += m_rcFront.left;
            rcSrc.top += m_rcFront.top;
            rcSrc.bottom += m_rcFront.top;

            hr = m_pBltTarget->Blt(&rcSrc, srcBuffer->m_pDDSBack, dstRect, DDBLT_WAIT, NULL);
        }

        else hr = m_pBltTarget->Blt(srcRect, srcBuffer->m_pDDSBack, dstRect, DDBLT_WAIT, NULL);
    }

    if ( not SUCCEEDED(hr))
        MonoPrint("ImageBuffer::Compose - Error 0x%X\n", hr);
}

// Copy a retangular area from the source image's front buffer to the this images's
// back buffer.  Don't write pixels from the source whose color matches the provided
// color key value.  Both rectangles must be entirely inside their respective buffers.
void ImageBuffer::ComposeTransparent(ImageBuffer *srcBuffer, RECT *dstRect, RECT *srcRect)
{
    ShiAssert(IsReady());
    ShiAssert(FALSE == F4IsBadReadPtr(srcRect, sizeof * srcRect));
    ShiAssert(FALSE == F4IsBadReadPtr(dstRect, sizeof * dstRect));
    ShiAssert(FALSE == F4IsBadReadPtr(srcBuffer, sizeof * srcBuffer));

    bool bStretch = ((srcRect->right - srcRect->left) not_eq (dstRect->right - dstRect->left)) or ((srcRect->bottom - srcRect->top) not_eq (dstRect->bottom - dstRect->top));
    HRESULT hr;

    if ( not m_bFrontRectValid and not bStretch)
    {
        if (srcRect and m_pBltTarget not_eq m_pDDSBack)
        {
            RECT rcSrc = *srcRect;
            rcSrc.left += m_rcFront.left;
            rcSrc.top += m_rcFront.top;

            hr = m_pBltTarget->BltFast(rcSrc.left, rcSrc.top, srcBuffer->m_pDDSBack, dstRect, DDBLTFAST_WAIT bitor DDBLTFAST_SRCCOLORKEY);
        }

        else hr = m_pBltTarget->BltFast(srcRect->left, srcRect->top, srcBuffer->m_pDDSBack, dstRect, DDBLTFAST_WAIT bitor DDBLTFAST_SRCCOLORKEY);
    }

    else
    {
        if (m_bFrontRectValid and srcRect and m_pBltTarget not_eq m_pDDSBack)
        {
            RECT rcSrc = *srcRect;
            rcSrc.left += m_rcFront.left;
            rcSrc.right += m_rcFront.left;
            rcSrc.top += m_rcFront.top;
            rcSrc.bottom += m_rcFront.top;

            hr = m_pBltTarget->Blt(&rcSrc, srcBuffer->m_pDDSBack, dstRect, DDBLT_WAIT bitor DDBLT_KEYSRC, NULL);
        }

        else hr = m_pBltTarget->Blt(srcRect, srcBuffer->m_pDDSBack, dstRect, DDBLT_WAIT bitor DDBLT_KEYSRC, NULL);
    }

    ShiAssert(SUCCEEDED(hr));

    if ( not SUCCEEDED(hr))
        MonoPrint("ImageBuffer::ComposeTransparent - Error 0x%X\n", hr);
}

// Copy a retangular area from the source image's BACK buffer to the this images's
// BACK buffer while rotating the image "angle" radians clockwise.  No clipping is provided
// (Note: we really should use the source's front buffer, but this was easier)
void ImageBuffer::ComposeRot(ImageBuffer *srcBuffer, RECT *srcRect, RECT *dstRect, float angle)
{
    ShiAssert(IsReady());
    ShiAssert(FALSE == F4IsBadReadPtr(srcRect, sizeof * srcRect));
    ShiAssert(FALSE == F4IsBadReadPtr(dstRect, sizeof * dstRect));
    ShiAssert(FALSE == F4IsBadReadPtr(srcBuffer, sizeof * srcBuffer));


    // Probably this will break if it is ever used to target a Primary surface
    // in a window without a back buffer.  We'd need to account for the window
    // offset in screen space.
    if (srcRect->right - srcRect->left == dstRect->right - dstRect->left)
    {
        ShiAssert((srcRect->bottom - srcRect->top) == (dstRect->bottom - dstRect->top));
        ShiAssert((srcRect->right - srcRect->left) == (dstRect->right - dstRect->left));

        RotateBitmap(srcBuffer, this,
                     (int)(angle *  2607.594587618f),// 180.0f / PI * 4096 / 90.0f (convertion from radians to Erick's)
                     srcRect, dstRect);
    }

    else
    {
        ShiAssert(2 * (srcRect->bottom - srcRect->top) == (dstRect->bottom - dstRect->top));
        ShiAssert(2 * (srcRect->right - srcRect->left) == (dstRect->right - dstRect->left));

        RotateBitmapDouble(srcBuffer, this,
                           (int)(angle *  2607.594587618f),// 180.0f / PI * 4096 / 90.0f (convertion from radians to Erick's)
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
void ImageBuffer::ComposeRoundRot(ImageBuffer *srcBuffer, RECT *srcRect, RECT *dstRect, float angle, int *startStopArray)
{
    ShiAssert(IsReady());
    ShiAssert(FALSE == F4IsBadReadPtr(srcRect, sizeof * srcRect));
    ShiAssert(FALSE == F4IsBadReadPtr(dstRect, sizeof * dstRect));
    ShiAssert(FALSE == F4IsBadReadPtr(srcBuffer, sizeof * srcBuffer));


    // Probably this will break if it is ever used to target a Primary surface
    // in a window with no back buffer.  We'd need to account for the window
    // offset in screen space.
    if (srcRect->right - srcRect->left == dstRect->right - dstRect->left)
    {
        ShiAssert((srcRect->bottom - srcRect->top) == (dstRect->bottom - dstRect->top));
        ShiAssert((srcRect->right - srcRect->left) == (dstRect->right - dstRect->left));

        RotateBitmapMask(srcBuffer, this,
                         (int)(angle *  2607.594587618f),// 180.0f / PI * 4096 / 90.0f (convertion from radians to Erick's)
                         srcRect, dstRect,
                         startStopArray);
    }
    else
    {
        ShiAssert(2 * (srcRect->bottom - srcRect->top) == (dstRect->bottom - dstRect->top));
        ShiAssert(2 * (srcRect->right - srcRect->left) == (dstRect->right - dstRect->left));


        RotateBitmapMaskDouble(srcBuffer, this,
                               (int)(angle *  2607.594587618f),// 180.0f / PI * 4096 / 90.0f (convertion from radians to Erick's)
                               srcRect, dstRect,
                               startStopArray);
    }
}

// Move this image's back buffer contents into its front buffer, possibly making it visible.
// PHASE 2 (2D UI): blits THIS CPU buffer to the D3D11 backbuffer and presents (regardless of
// m_bIsScreenBuffer). CopyToPrimary calls on Front_ (the composited UI frame).
// PHASE 5 (RTT): create an off-screen render target (RTV+SRV) sized to the buffer.
bool ImageBuffer::EnsureD3D11RenderTarget()
{
    extern bool g_bUseD3D11;
    if (!g_bUseD3D11 || !g_pD3D11Backend) return false;
    if (m_bIsScreenBuffer) return false;          // screen buffer = backbuffer
    if (m_pD3D11RTV && m_pD3D11SRV) return true;  // already created
    if (width <= 0 || height <= 0) return false;

    ID3D11Device* dev = g_pD3D11Backend->GetDevice();
    if (!dev) return false;

    D3D11_TEXTURE2D_DESC td; ZeroMemory(&td, sizeof(td));
    td.Width = width; td.Height = height; td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;       // = backbuffer format (the screen path draws the same)
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(dev->CreateTexture2D(&td, NULL, &m_pD3D11RTTex))) return false;
    if (FAILED(dev->CreateRenderTargetView(m_pD3D11RTTex, NULL, &m_pD3D11RTV)))
    { m_pD3D11RTTex->Release(); m_pD3D11RTTex = NULL; return false; }
    if (FAILED(dev->CreateShaderResourceView(m_pD3D11RTTex, NULL, &m_pD3D11SRV)))
    { m_pD3D11RTV->Release(); m_pD3D11RTV = NULL; m_pD3D11RTTex->Release(); m_pD3D11RTTex = NULL; return false; }
    return true;
}

// PHASE 5 (RTT): bind as render target (MFD/HUD/radar content is drawn here).
void ImageBuffer::BindD3D11RenderTarget(bool clear)
{
    if (!EnsureD3D11RenderTarget()) return;
    ID3D11DeviceContext* ctx = g_pD3D11Backend->GetContext();
    if (!ctx) return;

    ctx->OMSetRenderTargets(1, &m_pD3D11RTV, NULL);   // depth not needed for 2D content
    D3D11_VIEWPORT vp;
    vp.TopLeftX = 0; vp.TopLeftY = 0;
    vp.Width = (FLOAT)width; vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f; vp.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &vp);

    if (clear) { const FLOAT z[4] = { 0, 0, 0, 0 }; ctx->ClearRenderTargetView(m_pD3D11RTV, z); }

    // the renderer's gScreenSize = the RTT size (VS_Screen: pixel->NDC by this size)
    if (g_pD3D11Renderer) g_pD3D11Renderer->SetViewportSize(width, height);
}

// Artscout - 2026 (#34 menu 3D-viewer): copy a [x,y,w,h] rect out of this off-screen RTT into a
// 565 CPU buffer (the on-screen UI surface). The C_3dViewer renders a model into a screen-sized
// off-screen RTT; we read its viewport rect back and stamp it into the menu's 2D surface, so the
// model appears in the normal full 2D blit instead of the present-mode chroma path (black-out).
void ImageBuffer::BlitD3D11RTTTo565(unsigned short* dst, int dstStridePix, int dstHeightPix, int x, int y, int w, int h)
{
    extern bool g_bUseD3D11;
    if (!g_bUseD3D11 || !g_pD3D11Backend || !dst) return;
    if (!m_pD3D11RTTex) return;                 // nothing was rendered into the RTT

    ID3D11Device* dev = g_pD3D11Backend->GetDevice();
    ID3D11DeviceContext* ctx = g_pD3D11Backend->GetContext();
    if (!dev || !ctx) return;

    // Lazily create the STAGING copy (same size/format as the RTT, CPU-readable).
    if (!m_pD3D11Staging)
    {
        D3D11_TEXTURE2D_DESC td; ZeroMemory(&td, sizeof(td));
        td.Width = width; td.Height = height; td.MipLevels = 1; td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_STAGING;
        td.BindFlags = 0;
        td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        if (FAILED(dev->CreateTexture2D(&td, NULL, &m_pD3D11Staging))) return;
    }

    ctx->CopyResource(m_pD3D11Staging, m_pD3D11RTTex);

    D3D11_MAPPED_SUBRESOURCE map;
    if (FAILED(ctx->Map(m_pD3D11Staging, 0, D3D11_MAP_READ, 0, &map))) return;

    // Clip the requested rect to both the RTT (source) and the destination buffer.
    int x0 = x, y0 = y;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    int x1 = x + w, y1 = y + h;
    if (x1 > width)  x1 = width;
    if (y1 > height) y1 = height;
    if (x1 > dstStridePix) x1 = dstStridePix;
    if (y1 > dstHeightPix) y1 = dstHeightPix;

    const BYTE* srcBase = (const BYTE*)map.pData;
    for (int row = y0; row < y1; ++row)
    {
        const BYTE* src = srcBase + (size_t)row * map.RowPitch + (size_t)x0 * 4;
        unsigned short* d = dst + (size_t)row * dstStridePix + x0;
        for (int col = x0; col < x1; ++col)
        {
            BYTE r = src[0], g = src[1], b = src[2];   // R8G8B8A8
            *d++ = (unsigned short)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
            src += 4;
        }
    }

    ctx->Unmap(m_pD3D11Staging, 0);
}

// Artscout - 2026: GPU-copy this RTT's texture into another texture (same size/format). Used by the GM
// radar to snapshot a completed sweep into a persistent panel texture (CopyResource = no CPU readback).
void ImageBuffer::CopyD3D11RTTo(void* destTex2D)
{
    extern bool g_bUseD3D11;
    if (!g_bUseD3D11 || !g_pD3D11Backend || !destTex2D || !m_pD3D11RTTex) return;
    ID3D11DeviceContext* ctx = g_pD3D11Backend->GetContext();
    if (!ctx) return;
    ctx->CopyResource((ID3D11Resource*)destTex2D, (ID3D11Resource*)m_pD3D11RTTex);
}

void ImageBuffer::PresentD3D11()
{
    extern bool g_bUseD3D11;
    extern bool g_bD3D11GPUDraw;
    if (!g_bUseD3D11 || !g_pD3D11Backend) return;
    // PHASE 5: 2D UI. On a GPU frame (3D scene in the RTV) composite the UI OVER 3D with
    // black chroma-key (in-sim overlays: text/cursor/dialogs). On a pure 2D frame
    // (menu) -- a full blit from m_pSysMem.
    // #7 MSAA: on a 3D frame resolve the multisample target into the swapchain backbuffer BEFORE any UI
    // (and before present), regardless of m_pSysMem presence/render validity. Without MSAA -- no-op.
    if (g_bD3D11GPUDraw)
        g_pD3D11Backend->ResolveMsaaToBackBuffer();
    if (m_pSysMem)
    {
        if (g_bD3D11GPUDraw && g_pD3D11Renderer && g_pD3D11Renderer->IsValid())
        {
            g_pD3D11Renderer->CompositeUISurface(m_pSysMem, width, height);
            // On a 3D frame clear the CPU layer to black (=transparent): overlays are drawn
            // anew each frame, else stale content (old menu) ghosts over the 3D.
            memset(m_pSysMem, 0, (size_t)width * height * 2);
        }
        else if (!g_bD3D11GPUDraw)
        {
            g_pD3D11Backend->BlitBitmap565(m_pSysMem, width, height);
            // VR: this is the real (menu) 2D present path (runs on the ui95 OutputLoop
            // thread). Cache the UI surface so the XR pump can show it as a quad panel.
            extern bool g_bUseOpenXR;
            if (g_bUseOpenXR)
            {
                // Artscout - 2026 (VR menu): COPY the surface into a lock-protected stable buffer (m_pSysMem
                // is valid on THIS thread now) so the pump never reads a freed/resized ImageBuffer.
                extern void OpenXR_CacheMenuSurface(const void* src565, int w, int h);
                OpenXR_CacheMenuSurface(m_pSysMem, width, height);
            }
        }
    }
    g_pD3D11Backend->Present(true);
    g_bD3D11GPUDraw = false;
}

void ImageBuffer::SwapBuffers(bool bDontFlip)
{
    ShiAssert(IsReady());

    extern bool g_bUseD3D11;
    extern bool g_bD3D11GPUDraw;
    if (g_bUseD3D11)
    {
        if (m_bIsScreenBuffer && g_pD3D11Backend)
        {
            // Artscout - 2026 (VR mirror): in a VR 3D frame the scene was rendered into the XR eye
            // images (NOT the MSAA backbuffer target), and RenderFrame already copied the eye(s) onto
            // the back buffer for the desktop mirror. Resolving the (stale) MSAA target and compositing
            // the (stale splash) UI surface here would ERASE that mirror -> skip both in VR. The headset
            // gets its frame from xrEndFrame regardless; this only controls the desktop window.
            // NOTE: gate on g_bUseOpenXR, NOT g_bVrFrameActive. This present runs on the ui95 OutputLoop
            // thread, while g_bVrFrameActive is written on the render thread inside otwloop -- reading it here
            // is a cross-thread race: a stale 'false' flips vrMirror off, runs an extra Resolve/Composite in
            // the middle of a live XR frame, and desyncs the OpenXR frame loop (xrBeginFrame -> CALL_ORDER_
            // INVALID -> _com_error flood -> black screen). g_bUseOpenXR is stable, so VR stays consistent.
            extern bool g_bUseOpenXR, g_bXrMirror;
            const bool vrMirror = g_bUseOpenXR && g_bXrMirror && g_bD3D11GPUDraw;

            // PHASE 5: GPU frame (3D) -> composite UI over 3D; 2D (menu) -> blit.
            // #7 MSAA: 3D frame -> resolve the multisample target into the backbuffer before UI/present (no-op without MSAA).
            if (g_bD3D11GPUDraw && !vrMirror)
                g_pD3D11Backend->ResolveMsaaToBackBuffer();
            if (m_pSysMem && !vrMirror)
            {
                if (g_bD3D11GPUDraw && g_pD3D11Renderer && g_pD3D11Renderer->IsValid())
                {
                    g_pD3D11Renderer->CompositeUISurface(m_pSysMem, width, height);
                    memset(m_pSysMem, 0, (size_t)width * height * 2);
                }
                else if (!g_bD3D11GPUDraw)
                {
                    g_pD3D11Backend->BlitBitmap565(m_pSysMem, width, height);
                    // VR: this 2D path also carries the 3D-load splash (OTWImage 2D blit).
                    // Cache it so the XR pump shows the splash on the panel instead of black.
                    extern bool g_bUseOpenXR;
                    if (g_bUseOpenXR)
                    {
                        extern void OpenXR_CacheMenuSurface(const void* src565, int w, int h);
                        OpenXR_CacheMenuSurface(m_pSysMem, width, height);   // copy under lock (no UAF)
                    }
                }
            }
            g_pD3D11Backend->Present(true);
            g_bD3D11GPUDraw = false;
            // VR: in the 3D world the headset frame is driven by the per-eye STEREO loop in
            // OTWDriverClass::RenderFrame (the scene is rendered once per eye into the XR eye
            // images there). Menus are driven by the main-loop pump (OpenXR_PumpFrame). So
            // SwapBuffers no longer drives XR for the 3D path.
        }
        return;
    }


    // Return right away is there is nothing to do
    if (m_pDDSBack == NULL)
        return;

    HRESULT hr;

    //START_PROFILE("SWAP WAIT : ");
    // Make sure the drivers isnt buffering any data
    if (g_bCheckBltStatusBeforeFlip)
    {
        while (true)
        {
            HRESULT hres = m_pDDSBack->GetFlipStatus(DDGFS_CANFLIP);

            if (hres not_eq DDERR_WASSTILLDRAWING)
            {
                break;
            }

            // Let all the other threads have some CPU.
            Sleep(0);
        }
    }

    //STOP_PROFILE("SWAP WAIT : ");
    // OW
    if ( not bDontFlip and (m_ddsdFront.ddsCaps.dwCaps bitand DDSCAPS_FLIP))
    {
        hr = m_pDDSFront->Flip(NULL, DDFLIP_WAIT);
        // hr = m_pDDSFront->Flip(NULL, DDFLIP_NOVSYNC);
        ShiAssert(SUCCEEDED(hr));

        return;
    }

    RECT backRect = { 0, 0, m_ddsdBack.dwWidth, m_ddsdBack.dwHeight };

    if ( not m_bFrontRectValid) // assumes no clipper is attached (fullscreen) 
        hr = m_pDDSFront->BltFast(m_rcFront.left, m_rcFront.top, m_pDDSBack, &backRect, DDBLTFAST_WAIT bitor DDBLTFAST_NOCOLORKEY);
    else
        hr = m_pDDSFront->Blt(&m_rcFront, m_pDDSBack, &backRect, DDBLT_WAIT, NULL);

    ShiAssert(SUCCEEDED(hr));

    if ( not SUCCEEDED(hr))
        MonoPrint("ImageBuffer::SwapBuffers - Error 0x%X\n", hr);
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
    fileID = CreateFile(fullname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

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
    bih.biSizeImage = ((((bih.biWidth * bih.biBitCount) + 31) bitand compl 31) >> 3) * bih.biHeight;

    bfh.bfType = 0x4d42;
    bfh.bfReserved1 = 0;
    bfh.bfReserved2 = 0;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = bfh.bfOffBits + bih.biSizeImage;

    // Write the header
    if (( not WriteFile(fileID, &bfh, sizeof(BITMAPFILEHEADER), &dwBytes, NULL)) or (dwBytes not_eq sizeof(BITMAPFILEHEADER)))
    {
        char string[256];
        PutErrorString(string);
        strcat(string, "Failed to write screen dump file.");
        ShiError(string);
    }

    // Write the bitmap info header
    if (( not WriteFile(fileID, &bih, sizeof(BITMAPINFOHEADER), &dwBytes, NULL)) or (dwBytes not_eq sizeof(BITMAPINFOHEADER)))
    {
        char string[256];
        PutErrorString(string);
        strcat(string, "Failed to write screen dump file.");
        ShiError(string);
    }

    // Create the scanline output buffer
    bufferSize = 3 * (rect.right - rect.left);
    buffer = new BYTE[ bufferSize ];
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
                    pixel = (WORD*)Pixel(imagePtr, r, c);
                    color = Pixel16toPixel32(*pixel);

                    *p = (BYTE)((color bitand 0x00FF0000) >> 16);
                    p++; // Blue
                    *p = (BYTE)((color bitand 0x0000FF00) >>  8);
                    p++; // Green
                    *p = (BYTE)((color bitand 0x000000FF) >>  0);
                    p++; // Red
                }

                // Write the scanline to disk
                if ( not WriteFile(fileID, buffer, bufferSize, &bytes, NULL))  bytes = 0xFFFFFFFF;

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
                    pixel = (DWORD*)Pixel(imagePtr, r, c);
                    color = Pixel32toPixel32(*pixel);

                    *p = (BYTE)((color bitand 0x00FF0000) >> 16);
                    p++; // Blue
                    *p = (BYTE)((color bitand 0x0000FF00) >>  8);
                    p++; // Green
                    *p = (BYTE)((color bitand 0x000000FF) >>  0);
                    p++; // Red
                }

                // Write the scanline to disk
                if ( not WriteFile(fileID, buffer, bufferSize, &bytes, NULL))  bytes = 0xFFFFFFFF;

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

    HRESULT hr;
    hr = m_pDDSFront->IsLost();

    if (hr == DDERR_SURFACELOST)
    {
        hr = m_pDDSFront->Restore();

        if (SUCCEEDED(hr))
        {
            MonoPrint("ImageBuffer::RestoreAll - Front restored\n");

            if (m_pDDSFront not_eq m_pDDSBack)
            {
                hr = m_pDDSBack->IsLost();

                if (hr == DDERR_SURFACELOST)
                {
                    hr = m_pDDSBack->Restore();

                    if (SUCCEEDED(hr))
                        MonoPrint("ImageBuffer::RestoreAll - Back restored\n");
                    else MonoPrint("ImageBuffer::RestoreAll - Failed to restore back (0x%X)\n", hr);
                }
            }
        }

        else MonoPrint("ImageBuffer::RestoreAll - Failed to restore front (0x%X)\n", hr);
    }
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
            pixel = (WORD*)Pixel(imageptr, r, c);
            *pixel = Pixel32toPixel16(color);
        }
    }

    Unlock();

}
