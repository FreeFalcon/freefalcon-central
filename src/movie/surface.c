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
   HRESULT        ddVal;
   DDSURFACEDESC  ddsd;

   ddsd.dwSize = sizeof( DDSURFACEDESC );

   while ( TRUE )
   {
         ddVal = IDirectDrawSurface_Lock ( ( LPDIRECTDRAWSURFACE ) surface,
                        NULL,
                        &ddsd, DDLOCK_SURFACEMEMORYPTR, NULL );

         if ( ddVal == DDERR_SURFACELOST )
         {
            IDirectDrawSurface_Restore ( ( LPDIRECTDRAWSURFACE ) surface );
         }
         else
            if ( ddVal != DDERR_WASSTILLDRAWING )
               break;
   }

   sa->surfacePtr = ddsd.lpSurface;
   sa->lPitch = ddsd.lPitch;
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
   IDirectDrawSurface_Unlock ( ( LPDIRECTDRAWSURFACE ) surface,
                                 sa->surfacePtr );
}

/****************************************************************************

   surfaceGetDescription

   Purpose:    To get surface description.

   Arguments:  Surface object.
               Surface description.

   Returns:    None.

****************************************************************************/

void surfaceGetDescription( LPVOID surface,
                                SURFACEDESCRIPTION *sd )
{
   DDSURFACEDESC  ddsd;

   ddsd.dwSize = sizeof( ddsd );
   IDirectDrawSurface_GetSurfaceDesc( ( LPDIRECTDRAWSURFACE ) surface,
                                       &ddsd );

   sd->dwWidth = ddsd.dwWidth;
   sd->dwHeight = ddsd.dwHeight;
   sd->lPitch = ddsd.lPitch;
   sd->bitCount = ddsd.ddpfPixelFormat.dwRGBBitCount;
   sd->redMask = ddsd.ddpfPixelFormat.dwRBitMask;
   sd->greenMask = ddsd.ddpfPixelFormat.dwGBitMask;
   sd->blueMask = ddsd.ddpfPixelFormat.dwBBitMask;
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
   DDSURFACEDESC  ddsd;
   LPVOID         surface;
   HRESULT        ddVal;

	memset ( &ddsd, 0, sizeof ( ddsd ) );
	ddsd.dwSize = sizeof ( ddsd );
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
   ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN |
							DDSCAPS_SYSTEMMEMORY;
	ddsd.dwWidth = dibWidth;
	ddsd.dwHeight = dibHeight;

	ddVal = IDirectDraw_CreateSurface ( ( LPDIRECTDRAW ) ddPointer, &ddsd,
							( LPDIRECTDRAWSURFACE * ) &surface, NULL );

   if ( ddVal == DD_OK )
      return surface;
   else
      return NULL;
}

/****************************************************************************

   surfaceRelease

   Purpose:    To release a surface.

   Arguments:  Surface object to release.

   Returns:    None.

****************************************************************************/

void surfaceRelease( LPVOID surface )
{
   IDirectDrawSurface_Release ( ( LPDIRECTDRAWSURFACE ) surface );
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
   int      dstHeight;
   RECT     srcRectangle, dstRectangle;
   DDBLTFX  ddbltfx;
   HRESULT  ddVal;

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

   ddbltfx.dwSize = sizeof( ddbltfx );
   ddbltfx.dwROP = SRCCOPY;

   ddVal = IDirectDrawSurface_Blt( ( LPDIRECTDRAWSURFACE ) dstSurface,
                  &dstRectangle, ( LPDIRECTDRAWSURFACE ) srcSurface,
                  &srcRectangle, DDBLT_ROP, &ddbltfx );

   if ( ddVal == DD_OK )
      return 0;
   else
      return -1;
}
