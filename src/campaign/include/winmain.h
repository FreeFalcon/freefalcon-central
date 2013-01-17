#include "cmpglobl.h"
#include "unit.h"
#include "objectiv.h"

   extern   HANDLE 	hInst,  hMainWnd, hToolWnd;
   extern   HDC		hMainDC, hToolDC;
	extern   HMENU		hMainMenu;

   extern   HANDLE 	hCampaignThread;
   extern   DWORD  	CampaignThreadID;

   extern 	char     CampFile[80];
 	extern   char     CellSize;
   extern 	short    WULX,WULY,CULX,CULY;

	extern 	CdbList	GlobList;
	extern 	Unit		GlobUnit;
	extern	Objective GlobObj;

   // For testing
   extern   boolean  SHOWSTATS;


