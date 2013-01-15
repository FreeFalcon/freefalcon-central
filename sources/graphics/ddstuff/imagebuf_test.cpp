/***************************************************************************\
    ImageBuf.cpp
    Scott Randolph
    December 29, 1995

    This class provides management for drawing target buffers and sources
	for blit operations.
\***************************************************************************/

#include "stdafx.h"
#include "Rotate.h"
#include "Device.h"
#include "ImageBuf.h"
#include "FalcLib\include\debuggr.h"
#include "Falclib\Include\IsBad.h"

//#define _IMAGEBUFFER_PROTECT_SURF_LOCK
extern bool g_bCheckBltStatusBeforeFlip;
#include "RedProfiler.h"

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

#ifdef _IMAGEBUFFER_PROTECT_SURF_LOCK
	InitializeCriticalSection(&m_cs);
#endif
}

ImageBuffer::~ImageBuffer()
{
	ShiAssert(!IsReady());
	Cleanup();	// OW

#ifdef _IMAGEBUFFER_PROTECT_SURF_LOCK
	DeleteCriticalSection(&m_cs);
#endif
}

BOOL ImageBuffer::IsReady()
{
	return m_bReady;
}

BOOL ImageBuffer::Setup( DisplayDevice *dev, int w, int h, MPRSurfaceType front, MPRSurfaceType back, HWND targetWin, BOOL clip, BOOL fullScreen, BOOL bWillCallSwapBuffer )
{
	ZeroMemory(&m_rcFront, sizeof(m_rcFront));

	try
	{
		ShiAssert( !IsReady() );
		ShiAssert( dev );


		// Record the properties of the buffer(s) we're creating
		device = dev;
		width = w;
		height = h;

		IDirectDraw7Ptr pDD(dev->GetMPRdevice());

		DDSURFACEDESC2 ddsd;
		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwFlags = DDSD_CAPS;
		ddsd.dwSize = sizeof(ddsd);

		switch(front)
		{
			case Primary:
			{
				switch(back)
				{
					case SystemMem:
					{
						ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE; 
						CheckHR(pDD->CreateSurface(&ddsd, &m_pDDSFront, NULL));

						ddsd.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;
						ddsd.dwWidth  = w;
						ddsd.dwHeight = h; 
						ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE; 
						CheckHR(pDD->CreateSurface(&ddsd, &m_pDDSBack, NULL));

						break;
					}

					case VideoMem:
					{
						ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE; 
						CheckHR(pDD->CreateSurface(&ddsd, &m_pDDSFront, NULL));

						ddsd.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;
						ddsd.dwWidth  = w;
						ddsd.dwHeight = h; 
						ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE; 
						CheckHR(pDD->CreateSurface(&ddsd, &m_pDDSBack, NULL));

						break;
					}

					case Flip:
					{
						if(!fullScreen)
						{
							ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE; 
							CheckHR(pDD->CreateSurface(&ddsd, &m_pDDSFront, NULL));

							ddsd.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;
							ddsd.dwWidth  = w;
							ddsd.dwHeight = h; 
							ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE; 
							CheckHR(pDD->CreateSurface(&ddsd, &m_pDDSBack, NULL));
						}

						else
						{
							// Create the primary surface.
							ddsd.dwFlags |= DDSD_BACKBUFFERCOUNT;
							ddsd.ddsCaps.dwCaps |= DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE | DDSCAPS_COMPLEX | DDSCAPS_FLIP; 
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
						ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE; 
						CheckHR(pDD->CreateSurface(&ddsd, &m_pDDSFront, NULL));

						CheckHR(m_pDDSFront->QueryInterface(IID_IDirectDrawSurface7, (void **) &m_pDDSBack));

						break;
					}

					default:
					{
						ShiAssert(false);	// illegal combination
						return FALSE;
					}
				}

				// auto adjust front rect when creating the primary surface in windowed mode
				if(!fullScreen)
				{
					GetClientRect(targetWin, &m_rcFront);
					ClientToScreen(targetWin, (LPPOINT)&m_rcFront);
					ClientToScreen(targetWin, (LPPOINT)&m_rcFront+1);
					m_bFrontRectValid = true;
				}

				break;
			}

			case SystemMem:
			{
				switch(back)
				{
					case SystemMem:
					{
						ShiAssert(false);	// not implemented yet
						break;
					}

					case VideoMem:
					{
						ShiAssert(false);	// not implemented yet
						break;
					}

					case Flip:
					{
						ShiAssert(false);	// not implemented yet
						break;
					}

					case None:
					{
						ddsd.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;
						ddsd.dwWidth  = w;
						ddsd.dwHeight = h; 
						ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE; 
						CheckHR(pDD->CreateSurface(&ddsd, &m_pDDSFront, NULL));

						CheckHR(m_pDDSFront->QueryInterface(IID_IDirectDrawSurface7, (void **) &m_pDDSBack));

						break;
					}

					default:
					{
						ShiAssert(false);	// illegal combination
						return FALSE;
					}
				}

				break;
			}

			case VideoMem:
			{
				switch(back)
				{
					case SystemMem:
					{
						ShiAssert(false);	// not implemented yet
						break;
					}

					case VideoMem:
					{
						ShiAssert(false);	// not implemented yet
						break;
					}

					case Flip:
					{
						ShiAssert(false);	// not implemented yet
						break;
					}

					case None:
					{
						ddsd.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;
						ddsd.dwWidth  = w;
						ddsd.dwHeight = h; 
						ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE;
						HRESULT hr = pDD->CreateSurface(&ddsd, &m_pDDSFront, NULL);

						if(FAILED(hr))
						{
							if(hr == DDERR_OUTOFVIDEOMEMORY)
							{
								MonoPrint("ImageBuffer::Setup - EVICTING MANAGED TEXTURES !!\n");

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
						ShiAssert(false);	// illegal combination
						return FALSE;
					}
				}

				break;
			}

			case LocalVideoMem:
			case LocalVideoMem3D:
			{
				switch(back)
				{
					case SystemMem:
					{
						ShiAssert(false);	// not implemented yet
						break;
					}

					case VideoMem:
					{
						ShiAssert(false);	// not implemented yet
						break;
					}

					case Flip:
					{
						ShiAssert(false);	// not implemented yet
						break;
					}

					case None:
					{
						ddsd.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;
						ddsd.dwWidth  = w;
						ddsd.dwHeight = h; 
						ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;

						if(front == LocalVideoMem3D)
							ddsd.ddsCaps.dwCaps |= DDSCAPS_3DDEVICE;
						else
							ddsd.ddsCaps.dwCaps |= DDSCAPS_OFFSCREENPLAIN;

						HRESULT hr = pDD->CreateSurface(&ddsd, &m_pDDSFront, NULL);

						if(FAILED(hr))
						{
							if(hr == DDERR_OUTOFVIDEOMEMORY)
							{
								MonoPrint("ImageBuffer::Setup - EVICTING MANAGED TEXTURES !!\n");

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
						ShiAssert(false);	// illegal combination
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
						ShiAssert(false);	// illegal combination
						return FALSE;
					}
				}

				break;
			}
*/

			case None:		// Note: This doesnt create a 3D surface and it lets the driver decide where to put it)
			{
				switch(back)
				{
					case SystemMem:
					{
						ShiAssert(false);	// not implemented yet
						break;
					}

					case VideoMem:
					{
						ShiAssert(false);	// not implemented yet
						break;
					}

					case Flip:
					{
						ShiAssert(false);	// not implemented yet
						break;
					}

					case None:
					{
						ddsd.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;
						ddsd.dwWidth  = w;
						ddsd.dwHeight = h; 
						ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN; 
						CheckHR(pDD->CreateSurface(&ddsd, &m_pDDSFront, NULL));

						CheckHR(m_pDDSFront->QueryInterface(IID_IDirectDrawSurface7, (void **) &m_pDDSBack));

						break;
					}

					default:
					{
						ShiAssert(false);	// illegal combination
						return FALSE;
					}
				}

				break;
			}

			default:
			{
				ShiAssert(false);	// illegal combination
				return FALSE;
			}
		}

		if(clip && m_pDDSFront)
		{
			ShiAssert(!fullScreen);
			IDirectDrawClipperPtr pDDCLP;
			CheckHR(pDD->CreateClipper(0, &pDDCLP, NULL));
			CheckHR(pDDCLP->SetHWnd( 0, targetWin));
			CheckHR(m_pDDSFront->SetClipper(pDDCLP));
		}
	
		// Compute the color format conversion parameters
		m_ddsdFront.dwSize = sizeof(DDSURFACEDESC2);
		CheckHR(m_pDDSFront->GetSurfaceDesc(&m_ddsdFront));
		m_ddsdBack.dwSize = sizeof(DDSURFACEDESC2);
		CheckHR(m_pDDSBack->GetSurfaceDesc(&m_ddsdBack));

/*		#ifdef _DEBUG
		if(back == None)
//			MonoPrint("ImageBuffer::Setup - %dx%d front (%s) created in %s memory\n",
//				m_ddsdFront.dwWidth, m_ddsdFront.dwHeight, arrType2String[front],
//				m_ddsdFront.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY  ? "SYSTEM" :
//				(m_ddsdFront.ddsCaps.dwCaps & DDSCAPS_LOCALVIDMEM ? "VIDEO" : "AGP"));
		else
/*			MonoPrint("ImageBuffer::Setup - %dx%d front (%s) created in %s memory, back (%s) created in %s memory\n",
				m_ddsdFront.dwWidth, m_ddsdFront.dwHeight,
				arrType2String[front],
				m_ddsdFront.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY  ? "SYSTEM" :
				(m_ddsdFront.ddsCaps.dwCaps & DDSCAPS_LOCALVIDMEM ? "VIDEO" : "AGP"),
				 arrType2String[back],
				m_ddsdBack.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY  ? "SYSTEM" :
				(m_ddsdBack.ddsCaps.dwCaps & DDSCAPS_LOCALVIDMEM ? "VIDEO" : "AGP"));
		#endif
*/
		// Set blt target surface depending on wether we will page flip or not
		if(bWillCallSwapBuffer)
			m_pBltTarget = m_pDDSBack;
		else
			m_pBltTarget = m_pDDSFront;

		ComputeColorShifts();

		// Everything worked, so finish up and return
		m_bReady = TRUE;

		return TRUE;
	}

	catch(_com_error e)
	{
		MonoPrint("ImageBuffer::Setup - Error 0x%X\n", e.Error());
		return FALSE;
	}
}

// Alternative setup method
void ImageBuffer::AttachSurfaces(DisplayDevice *pDev, IDirectDrawSurface7 *pDDSFront, IDirectDrawSurface7 *pDDSBack)
{
	if(!pDev || !pDDSFront)
		return;

	if(!pDDSBack)
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

		// Set blt target surface depending on wether we will page flip or not
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

	catch(_com_error e)
	{
		MonoPrint("ImageBuffer::AttachSurfaces - Error 0x%X\n", e.Error());
	}
}


// Release the DDraw surfaces associated with this object
void ImageBuffer::Cleanup( void )
{
	m_bReady = FALSE;

	if(m_pDDSBack)	// MUST be released before releasing the front buffer
	{
		m_pDDSBack->Release();
		m_pDDSBack = NULL;
	}

	// Destroy our surface (including attached back buffer)
	if(m_pDDSFront)
	{
		m_pDDSFront->Release();
		m_pDDSFront = NULL;
	}

	m_pBltTarget = NULL;
}


// Compute the right shifts required to get from 24 bit RGB to this pixel format
void ImageBuffer::ComputeColorShifts( void )
{
	UInt32	mask;

	// RED
	mask = m_ddsdFront.ddpfPixelFormat.dwRBitMask;
	redShift = 8;
	ShiAssert( mask );
	while( !(mask & 1) ) {
		mask >>= 1;
		redShift--;
	}
	while( mask & 1 ) {
		mask >>= 1;
		redShift--;
	}

	// GREEN
	mask = m_ddsdFront.ddpfPixelFormat.dwGBitMask;
	greenShift = 16;
	ShiAssert( mask );
	while( !(mask & 1) ) {
		mask >>= 1;
		greenShift--;
	}
	while( mask & 1 ) {
		mask >>= 1;
		greenShift--;
	}

	// BLUE
	mask = m_ddsdFront.ddpfPixelFormat.dwBBitMask;
	ShiAssert( mask );
	blueShift = 24;
	while( !(mask & 1) ) {
		mask >>= 1;
		blueShift--;
	}
	while( mask & 1 ) {
		mask >>= 1;
		blueShift--;
	}
}

void ImageBuffer::GetColorMasks( UInt32 *r, UInt32 *g, UInt32* b )
{
	*r = m_ddsdFront.ddpfPixelFormat.dwRBitMask;
	*g = m_ddsdFront.ddpfPixelFormat.dwGBitMask;
	*b = m_ddsdFront.ddpfPixelFormat.dwBBitMask;
}

// Adjust the offset into the primary surface if we are windowed
void ImageBuffer::UpdateFrontWindowRect( RECT *rect )
{
	// ShiAssert( frontType == Primary );	// This is only useful for the primary surface
	if(rect) m_rcFront = *rect;
	m_bFrontRectValid = rect && (m_rcFront.left || m_rcFront.right);
}

// Fix in memory and return and pointer to the memory associated with our back buffer
void *ImageBuffer::Lock(bool bLockMutexOnly, bool bWriteOnly)
{
#ifdef _IMAGEBUFFER_PROTECT_SURF_LOCK
	EnterCriticalSection(&m_cs);
	if(bLockMutexOnly)
		return NULL;
#endif

	ShiAssert(IsReady());

	HRESULT hr;

	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

//	DWORD dwFlags = DDLOCK_NOSYSLOCK | DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR;
	DWORD dwFlags = DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR;
	if(bWriteOnly) dwFlags |= DDLOCK_WRITEONLY;

	int nRetries = 1;

	Retry:
	hr = m_pDDSBack->Lock(NULL, &ddsd, dwFlags, NULL);
	m_bBitsLocked = SUCCEEDED(hr);

	if(FAILED(hr))
	{
		MonoPrint("ImageBuffer::Lock - Lock failed with 0x%X\n", hr);

		if(hr == DDERR_SURFACELOST && nRetries)
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
	ShiAssert(IsReady());

	if(m_bBitsLocked)
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
void ImageBuffer::SetChromaKey( UInt32 colorKey )
{
	if (!m_pDDSFront) // JB 010404 CTD
		return;

	ShiAssert(IsReady());

	// Convert the key color from 32 bit RGB to the current pixel format

	// RED
	if (redShift >= 0)
		m_dwColorKey = (colorKey >>  redShift) & m_ddsdFront.ddpfPixelFormat.dwRBitMask;
	else
		m_dwColorKey = (colorKey << -redShift) & m_ddsdFront.ddpfPixelFormat.dwRBitMask;

	// GREEN
	if (greenShift >= 0)
		m_dwColorKey |= (colorKey >>  greenShift) & m_ddsdFront.ddpfPixelFormat.dwGBitMask;
	else
		m_dwColorKey |= (colorKey << -greenShift) & m_ddsdFront.ddpfPixelFormat.dwGBitMask;

	// BLUE
	if(blueShift >= 0)
		m_dwColorKey |= (colorKey >>  blueShift) & m_ddsdFront.ddpfPixelFormat.dwBBitMask;
	else
		m_dwColorKey |= (colorKey << -blueShift) & m_ddsdFront.ddpfPixelFormat.dwBBitMask;

	DDCOLORKEY	key = { m_dwColorKey, m_dwColorKey};

	// Submit the request to DirectDraw
	HRESULT hr = m_pDDSFront->SetColorKey(DDCKEY_SRCBLT, &key); 
}

// Convert a 32 bit alpha, blue, green, red color to a 16 bit pixel
// NOTE:  At present alpha is ignored by this call, but that could change...
WORD ImageBuffer::Pixel32toPixel16( UInt32 ABGR )
{
	UInt32		color;
	
// OW FIXME
//	ShiAssert( PixelSize() == 2 );	// Only returns 16 bit values

	// RED
	if (redShift >= 0) {
		color = (ABGR >>  redShift) & m_ddsdFront.ddpfPixelFormat.dwRBitMask;
	} else {
		color = (ABGR << -redShift) & m_ddsdFront.ddpfPixelFormat.dwRBitMask;
	}

	// GREEN
	if (greenShift >= 0) {
		color |= (ABGR >>  greenShift) & m_ddsdFront.ddpfPixelFormat.dwGBitMask;
	} else {
		color |= (ABGR << -greenShift) & m_ddsdFront.ddpfPixelFormat.dwGBitMask;
	}

	// BLUE
	if (blueShift >= 0) {
		color |= (ABGR >>  blueShift) & m_ddsdFront.ddpfPixelFormat.dwBBitMask;
	} else {
		color |= (ABGR << -blueShift) & m_ddsdFront.ddpfPixelFormat.dwBBitMask;
	}

	return (WORD)color;
}

DWORD ImageBuffer::Pixel32toPixel32( UInt32 ABGR )
{
	UInt32		color;
	
// OW FIXME
//	ShiAssert( PixelSize() == 2 );	// Only returns 16 bit values

	// RED
	if (redShift >= 0) {
		color = (ABGR >>  redShift) & m_ddsdFront.ddpfPixelFormat.dwRBitMask;
	} else {
		color = (ABGR << -redShift) & m_ddsdFront.ddpfPixelFormat.dwRBitMask;
	}

	// GREEN
	if (greenShift >= 0) {
		color |= (ABGR >>  greenShift) & m_ddsdFront.ddpfPixelFormat.dwGBitMask;
	} else {
		color |= (ABGR << -greenShift) & m_ddsdFront.ddpfPixelFormat.dwGBitMask;
	}

	// BLUE
	if (blueShift >= 0) {
		color |= (ABGR >>  blueShift) & m_ddsdFront.ddpfPixelFormat.dwBBitMask;
	} else {
		color |= (ABGR << -blueShift) & m_ddsdFront.ddpfPixelFormat.dwBBitMask;
	}

	return color;
}


// Convert a 16 bit pixel to a 32 bit alpha, blue, green, red color
// NOTE:  At present alpha is always set to 0 by this call, but that could change...
UInt32 ImageBuffer::Pixel16toPixel32( WORD pixel )
{
	UInt32		color;
	
	ShiAssert( PixelSize() == 2 );	// Only returns 16 bit values

	// RED
	if (redShift >= 0) {
		color = (pixel & m_ddsdFront.ddpfPixelFormat.dwRBitMask) <<  redShift;
	} else {
		color = (pixel & m_ddsdFront.ddpfPixelFormat.dwRBitMask) >> -redShift;
	}

	// GREEN
	if (greenShift >= 0) {
		color |= (pixel & m_ddsdFront.ddpfPixelFormat.dwGBitMask) <<  greenShift;
	} else {
		color |= (pixel & m_ddsdFront.ddpfPixelFormat.dwGBitMask) >> -greenShift;
	}

	// BLUE
	if (blueShift >= 0) {
		color |= (pixel & m_ddsdFront.ddpfPixelFormat.dwBBitMask) <<  blueShift;
	} else {
		color |= (pixel & m_ddsdFront.ddpfPixelFormat.dwBBitMask) >> -blueShift;
	}

	return color;
}

// Copy a retangular area from the source image's front buffer to the this images's
// back buffer.  Both rectangles must be entirely inside their respective buffers.
void ImageBuffer::Compose( ImageBuffer *srcBuffer, RECT *dstRect, RECT *srcRect)
{
	ShiAssert(IsReady());
	ShiAssert(FALSE == F4IsBadReadPtr(srcRect, sizeof *srcRect));
	ShiAssert(FALSE == F4IsBadReadPtr(dstRect, sizeof *dstRect));
	ShiAssert(FALSE == F4IsBadReadPtr(srcBuffer, sizeof *srcBuffer));


	bool bStretch = ((srcRect->right - srcRect->left) != (dstRect->right - dstRect->left)) || ((srcRect->bottom - srcRect->top) != (dstRect->bottom - dstRect->top));
	HRESULT hr;

	if(!m_bFrontRectValid && !bStretch)
	{
		if(srcRect && m_pBltTarget != m_pDDSBack)
		{
			RECT rcSrc = *srcRect;
			rcSrc.left += m_rcFront.left;
			rcSrc.top += m_rcFront.top;

			hr = m_pBltTarget->BltFast(rcSrc.left, rcSrc.top, srcBuffer->m_pDDSBack, dstRect, DDBLTFAST_WAIT | DDBLTFAST_NOCOLORKEY);
		}

		else hr = m_pBltTarget->BltFast(srcRect->left, srcRect->top, srcBuffer->m_pDDSBack, dstRect, DDBLTFAST_WAIT | DDBLTFAST_NOCOLORKEY);
	}

	else
	{
		if(m_bFrontRectValid && srcRect && m_pBltTarget != m_pDDSBack)
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

	ShiAssert(SUCCEEDED(hr));

	if(!SUCCEEDED(hr))
		MonoPrint("ImageBuffer::Compose - Error 0x%X\n", hr);
}

// Copy a retangular area from the source image's front buffer to the this images's
// back buffer.  Don't write pixels from the source whose color matches the provided
// color key value.  Both rectangles must be entirely inside their respective buffers.
void ImageBuffer::ComposeTransparent( ImageBuffer *srcBuffer, RECT *dstRect, RECT *srcRect)
{
	ShiAssert(IsReady());
	ShiAssert(FALSE == F4IsBadReadPtr(srcRect, sizeof *srcRect));
	ShiAssert(FALSE == F4IsBadReadPtr(dstRect, sizeof *dstRect));
	ShiAssert(FALSE == F4IsBadReadPtr(srcBuffer, sizeof *srcBuffer));

	bool bStretch = ((srcRect->right - srcRect->left) != (dstRect->right - dstRect->left)) || ((srcRect->bottom - srcRect->top) != (dstRect->bottom - dstRect->top));
	HRESULT hr;

	if(!m_bFrontRectValid && !bStretch)
	{
		if(srcRect && m_pBltTarget != m_pDDSBack)
		{
			RECT rcSrc = *srcRect;
			rcSrc.left += m_rcFront.left;
			rcSrc.top += m_rcFront.top;

			hr = m_pBltTarget->BltFast(rcSrc.left, rcSrc.top, srcBuffer->m_pDDSBack, dstRect, DDBLTFAST_WAIT | DDBLTFAST_SRCCOLORKEY);
		}

		else hr = m_pBltTarget->BltFast(srcRect->left, srcRect->top, srcBuffer->m_pDDSBack, dstRect, DDBLTFAST_WAIT | DDBLTFAST_SRCCOLORKEY );
	}

	else
	{
		if(m_bFrontRectValid && srcRect && m_pBltTarget != m_pDDSBack)
		{
			RECT rcSrc = *srcRect;
			rcSrc.left += m_rcFront.left;
			rcSrc.right += m_rcFront.left;
			rcSrc.top += m_rcFront.top;
			rcSrc.bottom += m_rcFront.top;

			hr = m_pBltTarget->Blt(&rcSrc, srcBuffer->m_pDDSBack, dstRect, DDBLT_WAIT | DDBLT_KEYSRC, NULL);
		}

		else hr = m_pBltTarget->Blt(srcRect, srcBuffer->m_pDDSBack, dstRect, DDBLT_WAIT | DDBLT_KEYSRC, NULL);
	}

	ShiAssert(SUCCEEDED(hr));

	if(!SUCCEEDED(hr))
		MonoPrint("ImageBuffer::ComposeTransparent - Error 0x%X\n", hr);
}

// Copy a retangular area from the source image's BACK buffer to the this images's
// BACK buffer while rotating the image "angle" radians clockwise.  No clipping is provided
// (Note: we really should use the source's front buffer, but this was easier)
void ImageBuffer::ComposeRot( ImageBuffer *srcBuffer, RECT *srcRect, RECT *dstRect, float angle )
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
		ShiAssert((srcRect->bottom - srcRect->top) == (dstRect->bottom - dstRect->top));
		ShiAssert((srcRect->right - srcRect->left) == (dstRect->right - dstRect->left));

		RotateBitmap (	srcBuffer, this, 
						(int)(angle *  2607.594587618f),// 180.0f / PI * 4096 / 90.0f (convertion from radians to Erick's)
						srcRect, dstRect );
	}

	else
	{
		ShiAssert(2*(srcRect->bottom - srcRect->top) == (dstRect->bottom - dstRect->top));
		ShiAssert(2*(srcRect->right - srcRect->left) == (dstRect->right - dstRect->left));

		RotateBitmapDouble (srcBuffer, this, 
							(int)(angle *  2607.594587618f),// 180.0f / PI * 4096 / 90.0f (convertion from radians to Erick's)
							srcRect, dstRect );
	}
}

// Copy a round area from the source image's BACK buffer to the this images's
// BACK buffer while rotating the image "angle" radians clockwise.  The startStopArray
// must be an array of an even number of integers.  The first int is the offset along the
// first line in the target buffer at which to start writing.  The second int is the offset
// at which to stop writing on the first line.  There must be as many integer pairs as there
// are lines in the destination rectangle.
// (Note: we really should use the source's front buffer, but this was easier)
void ImageBuffer::ComposeRoundRot( ImageBuffer *srcBuffer, RECT *srcRect, RECT *dstRect, float angle, int *startStopArray )
{
	ShiAssert(IsReady());
	ShiAssert(FALSE == F4IsBadReadPtr(srcRect, sizeof *srcRect));
	ShiAssert(FALSE == F4IsBadReadPtr(dstRect, sizeof *dstRect));
	ShiAssert(FALSE == F4IsBadReadPtr(srcBuffer, sizeof *srcBuffer));


	// Probably this will break if it is ever used to target a Primary surface
	// in a window with no back buffer.  We'd need to account for the window
	// offset in screen space.
	if (srcRect->right - srcRect->left == dstRect->right - dstRect->left) {
		ShiAssert((srcRect->bottom - srcRect->top) == (dstRect->bottom - dstRect->top));
		ShiAssert((srcRect->right - srcRect->left) == (dstRect->right - dstRect->left));

		RotateBitmapMask (	srcBuffer, this, 
							(int)(angle *  2607.594587618f),// 180.0f / PI * 4096 / 90.0f (convertion from radians to Erick's)
							srcRect, dstRect,
							startStopArray );
	} else {
		ShiAssert(2*(srcRect->bottom - srcRect->top) == (dstRect->bottom - dstRect->top));
		ShiAssert(2*(srcRect->right - srcRect->left) == (dstRect->right - dstRect->left));


		RotateBitmapMaskDouble (srcBuffer, this, 
								(int)(angle *  2607.594587618f),// 180.0f / PI * 4096 / 90.0f (convertion from radians to Erick's)
								srcRect, dstRect,
								startStopArray );
	}
}

// Move this image's back buffer contents into its front buffer, possibly making it visible.
void ImageBuffer::SwapBuffers(bool bDontFlip)
{
	ShiAssert(IsReady());

	// Return right away is there is nothing to do
	if(m_pDDSBack == NULL)
		return;

	HRESULT hr;
	START_PROFILE("FLIP WAITS");

	// Make sure the drivers isnt buffering any data
	if(g_bCheckBltStatusBeforeFlip)
	{
		while(true)
		{
			HRESULT hres = m_pDDSBack->GetFlipStatus(DDGFS_ISFLIPDONE);
			if(hres != DDERR_WASSTILLDRAWING)
			{
					break;
			}

			// Let all the other threads have some CPU.
			Sleep(0);
			
		}
	}


	// OW
	if(!bDontFlip && (m_ddsdFront.ddsCaps.dwCaps & DDSCAPS_FLIP))
	{
		hr = m_pDDSFront->Flip(NULL, DDFLIP_WAIT);
		// hr = m_pDDSFront->Flip(NULL, DDFLIP_NOVSYNC);
		ShiAssert(SUCCEEDED(hr));
/*		while(true)
		{
			HRESULT hres = m_pDDSFront->GetBltStatus(DDGBS_ISBLTDONE);
			if(hres != DDERR_WASSTILLDRAWING)
			{
					break;
			}

			// Let all the other threads have some CPU.
			Sleep(0);
		}*/
		STOP_PROFILE("FLIP WAITS");
		return;
	}

	RECT backRect = { 0, 0, m_ddsdBack.dwWidth, m_ddsdBack.dwHeight };	

	if(!m_bFrontRectValid)	// assumes no clipper is attached (fullscreen) !!
		hr = m_pDDSFront->BltFast(m_rcFront.left, m_rcFront.top, m_pDDSBack, &backRect, DDBLTFAST_WAIT | DDBLTFAST_NOCOLORKEY);
	else
		hr = m_pDDSFront->Blt(&m_rcFront, m_pDDSBack, &backRect, DDBLT_WAIT, NULL);

	STOP_PROFILE("FLIP WAITS");

	ShiAssert(SUCCEEDED(hr));

	if(!SUCCEEDED(hr))
		MonoPrint("ImageBuffer::SwapBuffers - Error 0x%X\n", hr);

}

// Helpful function to drop a screen capture to disk (BACK buffer to 24 bit RAW file)
void ImageBuffer::BackBufferToRAW( char *filename )
{
	char	fullname[MAX_PATH];
	HANDLE	fileID;
	int		r, c;
	void	*imagePtr;
	BYTE	*buffer;
	BYTE	*p;
	UInt32	bufferSize;
	DWORD	bytes;
	UInt32	color;
	RECT	rect;

	// Probably this will break if it is ever used on a Primary surface
	// in a window with no back buffer.  We'd need to account for the window
	// offset in screen space.
	rect.top = 0;
	rect.left = 0;
	rect.bottom = height;
	rect.right = width;

	// Create a new RAW file
	sprintf( fullname, "%s%s", filename, ".bmp" );
    fileID = CreateFile( fullname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if(fileID == INVALID_HANDLE_VALUE )
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
	bih.biSizeImage = ((((bih.biWidth * bih.biBitCount) + 31) & ~31) >> 3) * bih.biHeight;

    bfh.bfType = 0x4d42;
    bfh.bfReserved1 = 0;
    bfh.bfReserved2 = 0;
    bfh.bfOffBits = sizeof( BITMAPFILEHEADER ) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = bfh.bfOffBits + bih.biSizeImage;

    // Write the header
    if((!WriteFile(fileID, &bfh, sizeof(BITMAPFILEHEADER), &dwBytes, NULL)) || ( dwBytes != sizeof(BITMAPFILEHEADER)))
    {
		char string[256];
		PutErrorString( string );
		strcat( string, "Failed to write screen dump file." );
		ShiError( string );
    }

    // Write the bitmap info header
    if((!WriteFile(fileID, &bih, sizeof(BITMAPINFOHEADER), &dwBytes, NULL)) || ( dwBytes != sizeof(BITMAPINFOHEADER)))
    {
		char string[256];
		PutErrorString( string );
		strcat( string, "Failed to write screen dump file." );
		ShiError( string );
    }

	// Create the scanline output buffer
	bufferSize = 3 * (rect.right - rect.left);
	buffer = new BYTE[ bufferSize ];
	ShiAssert( buffer );

	// Lock the back buffer surface
	imagePtr = Lock();
	ShiAssert( imagePtr );

	switch(PixelSize())
	{
		case 2:
		{
			WORD	*pixel;

			// Step through each scanline
			for (r=rect.bottom-1; r>=0; r--)
			{
				// Start a new line
				p=buffer;

				// Step accross the scanline converting each pixel to RGB
				for (c=rect.left; c<rect.right; c++)
				{
					// Get the 16 bit color from the surface and convert to 32 bit ABGR
					pixel = (WORD*)Pixel( imagePtr, r, c );
					color = Pixel16toPixel32( *pixel );

					*p = (BYTE)((color & 0x00FF0000) >> 16);	p++;	// Blue
					*p = (BYTE)((color & 0x0000FF00) >>  8);	p++;	// Green
					*p = (BYTE)((color & 0x000000FF) >>  0);	p++;	// Red
				}

				// Write the scanline to disk
				if ( !WriteFile( fileID, buffer, bufferSize, &bytes, NULL ) )  bytes=0xFFFFFFFF;
				if ( bytes != bufferSize ) {
					char string[256];
					PutErrorString( string );
					strcat( string, "Couldn't write screen dump file." );
					ShiError( string );
				}
			}

			break;
		}

		case 4:
		{
			DWORD	*pixel;

			// Step through each scanline
			for (r=rect.bottom-1; r>=0; r--)
			{
				// Start a new line
				p=buffer;

				// Step accross the scanline converting each pixel to RGB
				for (c=rect.left; c<rect.right; c++){
					
					// Get the 16 bit color from the surface and convert to 32 bit ABGR
					pixel = (DWORD*)Pixel( imagePtr, r, c );
					color = Pixel32toPixel32( *pixel );

					*p = (BYTE)((color & 0x00FF0000) >> 16);	p++;	// Blue
					*p = (BYTE)((color & 0x0000FF00) >>  8);	p++;	// Green
					*p = (BYTE)((color & 0x000000FF) >>  0);	p++;	// Red
				}

				// Write the scanline to disk
				if ( !WriteFile( fileID, buffer, bufferSize, &bytes, NULL ) )  bytes=0xFFFFFFFF;
				if ( bytes != bufferSize ) {
					char string[256];
					PutErrorString( string );
					strcat( string, "Couldn't write screen dump file." );
					ShiError( string );
				}
			}

			break;
		}
	}
	// Unlock the back buffer
	Unlock();

	// Close the output RAW file and free the output buffer
	CloseHandle( fileID );
	delete[] buffer;
}

// OW
void ImageBuffer::RestoreAll()
{
	ShiAssert(IsReady());

	HRESULT hr;
	hr = m_pDDSFront->IsLost();

	if(hr == DDERR_SURFACELOST)
	{
		hr = m_pDDSFront->Restore();

		if(SUCCEEDED(hr))
		{
			MonoPrint("ImageBuffer::RestoreAll - Front restored\n");

			if(m_pDDSFront != m_pDDSBack)
			{
				hr = m_pDDSBack->IsLost();

				if(hr == DDERR_SURFACELOST)
				{
					hr = m_pDDSBack->Restore();

					if(SUCCEEDED(hr))
						MonoPrint("ImageBuffer::RestoreAll - Back restored\n");
					else MonoPrint("ImageBuffer::RestoreAll - Failed to restore back (0x%X)\n", hr);
				}
			}
		}

		else MonoPrint("ImageBuffer::RestoreAll - Failed to restore front (0x%X)\n", hr);
	}
}


//Wombat778 10-06-2003 Added to allow clearing of an imagebuffer with the specified color

void ImageBuffer::Clear( UInt32 color )
{
	void *imageptr;
	WORD *pixel;

	int width=targetXres();
	int height=targetYres();

	imageptr=Lock();
	ShiAssert(imageptr);
		
	for (int r=0;r<height;r++) {
		for (int c=0;c<width;c++) {
			pixel = (WORD*)Pixel( imageptr, r, c );
			*pixel=Pixel32toPixel16(color);
		}
	}

	Unlock();

}
