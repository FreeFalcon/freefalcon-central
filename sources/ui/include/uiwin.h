/********************************************************************************
/*
/* windows related UI header
/*
/*******************************************************************************/

#ifndef UIWIN_H
#define UIWIN_H

#define Black			0
#define Blue			1
#define Green			2
#define Yellow			3
#define Red				4
#define Magenta			5
#define Brown			6
#define Gray			7
#define Orange			8
#define LightBlue		9
#define LightGreen		10
#define LightBrown		11
#define LightRed		12
#define LightGray		13
#define DarkGray		14
#define White			15

struct MapWindowType {
		HWND			hMapWnd;				// Handle to the window
		HDC				hMapDC;
		short			FX,FY,LX,LY;			// GridIndex window bounds
		short			PFX,PFY,PLX,PLY;		// Pixel window bounds
		short			PMFX,PMFY,PMLX,PMLY;	// Pixelmap bounds
		short			CenX,CenY;				// GridIndex center
		short			WULX,WULY;				// Window's upper left screen coordinates
		short			CULX,CULY;				// Client's upper left offset from window
		short			CellSize;				// Pixel size per cell
		unsigned char	Emitters;
		unsigned char	SAMs;
		unsigned char	ShowObjectives;
		unsigned char	ShowLinks;
		unsigned char	ShowUnits;
		unsigned char	ShowWPs;
		};
typedef MapWindowType* MapData;

// Map sizing Info
#define 	MAX_XPIX	1536
#define 	MAX_YPIX	768

#define 	XSIDE 0
#define 	YSIDE 1

// ==============================================================
// Map drawing routines
// ==============================================================

void	SetRefresh(MapData md);

void RefreshMap(MapData md, HDC DC, RECT	*rect);

int CenPer(MapData md, GridIndex cen, int side);

void FindBorders(MapData md);

#endif