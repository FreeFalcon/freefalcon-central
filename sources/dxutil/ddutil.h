#ifndef _DDUTIL_H_

#define _DDUTIL_H_

#include <ddraw.h>

/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation. All Rights Reserved.
 *
 *  File:       ddutil.cpp
 *  $Header: /home/cvsroot/RedCobra/dxutil/ddutil.h,v 1.1.1.1 2003/09/26 20:20:46 Red Exp $
 *
 *  Content:    Routines for loading bitmap and palettes from resources
 *
 ***************************************************************************/

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

typedef struct tagST_RECT {
  LONG x, y, wid, ht;
} ST_RECT;

extern IDirectDrawPalette * DDLoadPalette(IDirectDraw *pdd, LPCSTR szBitmap);
extern IDirectDrawSurface * DDLoadBitmap(IDirectDraw *pdd, LPCSTR szBitmap, int dx, int dy);
extern IDirectDrawSurface * DDLoadBitmapEx(IDirectDraw *pdd, LPCSTR szBitmap,
	 int dx, int dy, ST_RECT *bmRect);
extern HRESULT              DDReLoadBitmap(IDirectDrawSurface *pdds, LPCSTR szBitmap);
extern HRESULT              DDReLoadBitmapEx(IDirectDrawSurface *pdds,
	 LPCSTR szBitmap, ST_RECT *bmRect);
// also returns HBITMAP and doesn't delete it
extern HRESULT              DDReLoadBitmap1(IDirectDrawSurface *pdds,
	 LPCSTR szBitmap, HBITMAP *hbm);
// also returns HBITMAP and doesn't delete it
extern HRESULT              DDReLoadBitmapEx1(IDirectDrawSurface *pdds,
	 LPCSTR szBitmap, ST_RECT *bmRect, HBITMAP *hbm);
extern HRESULT              DDCopyBitmap(IDirectDrawSurface *pdds, HBITMAP hbm, int x, int y, int dx, int dy);
extern DWORD                DDColorMatch(IDirectDrawSurface *pdds, COLORREF rgb);
extern HRESULT              DDSetColorKey(IDirectDrawSurface *pdds, COLORREF rgb);

#ifdef __cplusplus
}
#endif  /* __cplusplus */


HRESULT DDCopyBitmap(IDirectDrawSurface *pdds, char *image, 
	BITMAPINFO *bmi, int x, int y, int dx, int dy, int src_y );

HRESULT DDStretchBitmap(IDirectDrawSurface *pdds, char *image, 
	BITMAPINFO *bmi, int x, int y, int w, int h, int dest_w, int dest_h, int src_y );

#endif		// end _DDUTIL_H_
