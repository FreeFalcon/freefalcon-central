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
#include "SmartHeap/Include/smrtheap.hpp"
#endif
#include "../../codelib/include/shi/ShiError.h"

class DisplayDevice;
enum MPRSurfaceType;

#include <ddraw.h>
#include "context.h"

class ImageBuffer
{
	public:

	// Constructor/Destructor for this buffer
	ImageBuffer();
	virtual ~ImageBuffer();

	// Functions used to set up and manage this buffer
	BOOL Setup(DisplayDevice *dev, int width, int height, MPRSurfaceType front, MPRSurfaceType back, HWND targetWin = NULL, BOOL clip = FALSE, BOOL fullScreen = FALSE, BOOL bWillCallSwapBuffer = FALSE);
	void Cleanup();
	BOOL IsReady();
	void SetChromaKey(UInt32 RGBcolor);
	void UpdateFrontWindowRect( RECT *rect );
	void AttachSurfaces(DisplayDevice *pDev, IDirectDrawSurface7 *pDDSFront, IDirectDrawSurface7 *pDDSBack);

	// Used to get the properties of the under lying surfaces
	DisplayDevice *GetDisplayDevice()	{ return device; };
	IDirectDrawSurface7 *targetSurface()	{ return m_pDDSBack; };
	IDirectDrawSurface7 *frontSurface()	{ return m_pDDSFront; };

	int targetXres()	{ return width; };
	int	targetYres()	{ return height; };
	int	targetStride(){ return m_ddsdBack.lPitch; };

	void GetColorMasks( UInt32 *r, UInt32 *g, UInt32* b );
	UInt32 RedMask(void)	{ return m_ddsdFront.ddpfPixelFormat.dwRBitMask; };
	UInt32 GreenMask(void)	{ return m_ddsdFront.ddpfPixelFormat.dwGBitMask; };
	UInt32 BlueMask(void)	{ return m_ddsdFront.ddpfPixelFormat.dwBBitMask; }; 
	int RedShift(void)	{ return redShift; };
	int	GreenShift(void){ return greenShift; };
	int	BlueShift(void)	{ return blueShift; };
	int	PixelSize() { return m_ddsdFront.ddpfPixelFormat.dwRGBBitCount >>3; };

	void RestoreAll();	// OW

	// Used to allow direct pixel access to the back buffer
	void *Lock(bool bDontLockBits = false, bool bWriteOnly = true);
	void  Unlock();

	// Used to convert from a 32 bit color to a 16 bit pixel (assumes 16 bit display mode)
	UInt16  Pixel32toPixel16( UInt32 ABGR );
	UInt32  Pixel16toPixel32( UInt16 pixel );
	UInt32  Pixel32toPixel32( UInt32 pixel );

	// Used to write a pixel to the back buffer surface (requires a Lock)
	void* Pixel( void* ptr, int row, int col )	  { ShiAssert( ptr );
											return (BYTE*)ptr + 
											  row*m_ddsdBack.lPitch + 
											  col*PixelSize(); };

	// Used to control image compositing
	void Compose(ImageBuffer *srcBuffer, RECT *srcRect, RECT *dstRect );
	void ComposeTransparent( ImageBuffer *srcBuffer, RECT *srcRect, RECT *dstRect );

	// Used to control image compositing with a clockwise rotation (in radians)
	void ComposeRot(ImageBuffer *srcBuffer, RECT *srcRect, RECT *dstRect, float angle );
	void ComposeRoundRot( ImageBuffer *srcBuffer, RECT *srcRect, RECT *dstRect, float angle, int *startStopArray );

	// Swap rolls of front and back buffers (page flip, blt, or nothing depending on types)
	void SwapBuffers(bool bDontFlip);

	// Helpful function to drop a screen capture to disk (BACK buffer to 24 bit RAW file)
	void BackBufferToRAW( char *filename );

	//Wombat778 10-06-2003
	void Clear( UInt32 color);

	protected:
	void ComputeColorShifts();

	protected:
	BOOL m_bReady;
	DisplayDevice *device;
	IDirectDrawSurface7 *m_pDDSFront;
	DDSURFACEDESC2	m_ddsdFront;
	HWND m_hWnd;
	RECT m_rcFront;
	bool m_bFrontRectValid;
	IDirectDrawSurface7 *m_pDDSBack;
	DDSURFACEDESC2	m_ddsdBack;
	int width;
	int height;
	int redShift;
	int greenShift;
	int blueShift;
	DWORD m_dwColorKey;
	IDirectDrawSurface7 *m_pBltTarget;
	CRITICAL_SECTION m_cs;
	bool m_bBitsLocked;
};


#endif // _IMAGEBUF_H_
