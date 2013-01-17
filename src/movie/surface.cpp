/*
   SURFACE.C

   Exported functions:

      surfaceGetPointer
      surfaceReleasePointer
      surfaceGetDescription
      surfaceCreate
      surfaceRelease

   Programmed by Kuswara Pranawahadi               September 5, 1996
*/

#include "surface.h"

// OW
#include <ddraw.h>
#include "IsBad.h"

/****************************************************************************

   surfaceGetPointer

   Purpose:    To get surface information, including pointer to the
               surface.

   Arguments:  Surface object.
               Surface pointer.

   Returns:    None.

****************************************************************************/

void surfaceGetPointer( LPVOID surface,
                        SURFACEACCESS *sa )
{
// OW 
#if 0
    MPRSurfaceDescription   sd;

    MPRGetSurfaceDescription( ( UInt ) surface, &sd );
    sa->surfacePtr = ( LPVOID ) MPRLockSurface( ( UInt ) surface );
    if( sa->surfacePtr )
        sa->lockStatus = SURFACE_IS_LOCKED;
    else
        sa->lockStatus = SURFACE_IS_UNLOCKED;

    sa->lPitch = sd.pitch;
#else
	IDirectDrawSurface7 *pDS = (IDirectDrawSurface7 *) surface;

	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	HRESULT hr = pDS->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_SURFACEMEMORYPTR, NULL);

	if(SUCCEEDED(hr))
	{
		sa->surfacePtr = ddsd.lpSurface;
        sa->lockStatus = SURFACE_IS_LOCKED;
	    sa->lPitch = ddsd.lPitch;
	}

	else
	{
		sa->surfacePtr = NULL;
		sa->lockStatus = SURFACE_IS_UNLOCKED;
	    sa->lPitch = 0;
	}
#endif
}

/****************************************************************************

   surfaceReleasePointer

   Purpose:    To unlock previously locked surface.

   Arguments:  Surface object.
               Surface pointer.

   Returns:    None.

****************************************************************************/

void surfaceReleasePointer( LPVOID surface,
                              SURFACEACCESS *sa )
{
// OW 
#if 0
    MPRUnlockSurface( ( UInt ) surface );
    sa->lockStatus = SURFACE_IS_UNLOCKED;
#else
	IDirectDrawSurface7 *pDS = (IDirectDrawSurface7 *) surface;

	HRESULT hr = pDS->Unlock(NULL);
	if(SUCCEEDED(hr))
	    sa->lockStatus = SURFACE_IS_UNLOCKED;
#endif
}

/****************************************************************************

   surfaceGetDescription

   Purpose:    To get surface description.

   Arguments:  Surface object.
               Surface description.

   Returns:    None.

****************************************************************************/

void surfaceGetDescription(LPVOID surface, SURFACEDESCRIPTION *sd)
{
#if 0
    MPRSurfaceDescription   mprsd;

    MPRGetSurfaceDescription( ( UInt ) surface, &mprsd );

    sd->dwWidth     = mprsd.width;
    sd->dwHeight    = mprsd.height;
    sd->lPitch      = mprsd.pitch;
    sd->bitCount    = mprsd.RGBBitCount;
    sd->redMask     = mprsd.RBitMask;
    sd->greenMask   = mprsd.GBitMask;
    sd->blueMask    = mprsd.BBitMask;
#else
	IDirectDrawSurface7 *pDS = (IDirectDrawSurface7 *) surface;

	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	HRESULT hr = -1; // JB 010220 CTD
	if (pDS && !F4IsBadReadPtr(pDS, sizeof(IDirectDrawSurface7))) // JB 010220 CTD
		hr = pDS->GetSurfaceDesc(&ddsd);

	if(SUCCEEDED(hr))
	{
		sd->dwWidth     = ddsd.dwWidth;
		sd->dwHeight    = ddsd.dwHeight;
		sd->lPitch      = ddsd.lPitch;
		sd->bitCount    = ddsd.ddpfPixelFormat.dwRGBBitCount;
		sd->redMask     = ddsd.ddpfPixelFormat.dwRBitMask;
		sd->greenMask   = ddsd.ddpfPixelFormat.dwGBitMask;
		sd->blueMask    = ddsd.ddpfPixelFormat.dwBBitMask;
	}
#endif
}

/****************************************************************************

   surfaceCreate

   Purpose:    To create a surface in system memory using the same
               color depth as the current display mode.

   Arguments:  DirectDraw object.
               Surface width.
               Surface height.

   Returns:    None.

****************************************************************************/

LPVOID surfaceCreate( LPVOID ddPointer, int dibWidth, int dibHeight )
{
// OW 
#if 0
    return ( LPVOID ) MPRCreateSurface( ( UInt ) ddPointer, dibWidth, dibHeight,
                                        SystemMem, None, NULL, 0, 0 );
#else
	IDirectDraw7 *pDD = (IDirectDraw7 *) ddPointer;
	IDirectDrawSurface7 *pDS = NULL;

	// Get mode caps
	DDSURFACEDESC2 ddsdMode;
	ZeroMemory(&ddsdMode, sizeof(ddsdMode));
	ddsdMode.dwSize = sizeof(ddsdMode);
	HRESULT hr = pDD->GetDisplayMode(&ddsdMode);

	if(SUCCEEDED(hr))
	{
		DDSURFACEDESC2 ddsd;
		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT;
		ddsd.dwSize = sizeof(ddsd);
		ddsd.ddpfPixelFormat = ddsdMode.ddpfPixelFormat;
		
		ddsd.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;
		ddsd.dwWidth  = dibWidth;
		ddsd.dwHeight = dibHeight; 
		ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN; 
		hr = pDD->CreateSurface(&ddsd, &pDS, NULL);
	}

	return pDS;
#endif
}

/****************************************************************************

   surfaceRelease

   Purpose:    To release a surface.

   Arguments:  Surface object to release.

   Returns:    None.

****************************************************************************/

void surfaceRelease( LPVOID surface )
{
// OW
#if 0
   MPRReleaseSurface( ( UInt ) surface );
#else
	IDirectDrawSurface7 *pDS = (IDirectDrawSurface7 *) surface;
	if(pDS) pDS->Release();
#endif
}

/****************************************************************************

   surfaceBlit

   Purpose:    To blit from one surface to another surface.

   Arguments:  Destination surface object.
               Starting x-coordinate on the destination.
               Starting y-coordinate on the destination.
               Source surface object.
               Source width.
               Source height.
               Mode: straight copy or stretch in y direction.

   Returns:    None.

****************************************************************************/

int surfaceBlit( LPVOID dstSurface, int x, int y, LPVOID srcSurface,
            int srcWidth, int srcHeight, int mode )
{
    int     dstHeight;
    RECT    srcRectangle, dstRectangle;

    if ( mode == BLIT_MODE_NORMAL )
        dstHeight = srcHeight;
    else
        dstHeight = srcHeight * 2;

    srcRectangle.left = 0;
    srcRectangle.top = 0;
    srcRectangle.right = srcWidth;
    srcRectangle.bottom = srcHeight;

    dstRectangle.left = x;
    dstRectangle.top = y;
    dstRectangle.right = x + srcWidth;
    dstRectangle.bottom = y + dstHeight;

// OW
#if 0
    if( MPRBlt( ( UInt ) srcSurface, ( UInt ) dstSurface, &srcRectangle, &dstRectangle ) )
        return 0;
    else
        return -1;
#else
	IDirectDrawSurface7 *pDSSrc = (IDirectDrawSurface7 *) srcSurface;
	IDirectDrawSurface7 *pDSDst = (IDirectDrawSurface7 *) dstSurface;
	HRESULT hr = pDSDst->Blt(&dstRectangle, pDSSrc, &srcRectangle, DDBLT_WAIT, NULL);
	return SUCCEEDED(hr) ? 0 : -1;
#endif

}
