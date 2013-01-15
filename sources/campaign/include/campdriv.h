#include "cmpglobl.h"
#include "unit.h"
#include "objectiv.h"
#include "F4Vu.h"

extern	HWND	hMainWnd, hToolWnd;
extern	HDC		hMainDC, hToolDC;
extern	HMENU	hMainMenu;

extern	MapData	MainMapData;
extern	short   CellSize;
extern 	short   WULX,WULY,CULX,CULY;

extern 	F4PFList	GlobList;
extern 	Unit		GlobUnit,WPUnit;
extern	Objective	GlobObj;
extern	WayPoint	GlobWP;

extern	F4PFList GetSquadsFlightList (int id);

extern	GridIndex CurX,CurY;

// For testing
extern	unsigned char  SHOWSTATS;
	

