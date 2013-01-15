#ifndef _UI95_DD_LAYER_
#define _UI95_DD_LAYER_

//XX void UI95_SetScreenColorInfo(WORD r_mask,WORD g_mask,WORD b_mask);
//XX void UI95_GetScreenColorInfo(WORD &r_mask,WORD &r_shift,WORD &g_mask,WORD &g_shift,WORD &b_mask,WORD &b_shift);
void UI95_SetScreenColorInfo(DWORD r_mask,DWORD g_mask, DWORD b_mask);
void UI95_GetScreenColorInfo
( 
	DWORD &r_mask, WORD &r_shift, 
	DWORD &g_mask, WORD &g_shift, 
	DWORD &b_mask, WORD &b_shift
);

//!void UI95_GetScreenColorInfo(WORD *r_mask,WORD *r_shift,WORD *g_mask,WORD *g_shift,WORD *b_mask,WORD *b_shift);

WORD UI95_RGB24Bit(unsigned long rgb);
WORD UI95_RGB15Bit(WORD rgb);
WORD UI95_ScreenToTga(WORD color);
WORD UI95_ScreenToGrey(WORD color);

//IDirectDrawSurface *UI95_CreateDDSurface(IDirectDraw *DD,DWORD width,DWORD height);
//void UI95_GetScreenFormat(DDSURFACEDESC *desc)
//BOOL UI95_DDErrorCheck( HRESULT result );
//void *UI95_Lock(IDirectDrawSurface *ddsurface);

#endif
