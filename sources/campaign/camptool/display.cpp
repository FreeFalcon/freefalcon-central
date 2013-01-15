
#ifdef CAMPTOOL

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <conio.h>
#include <math.h>
#include "CmpGlobl.h" 
#include "ErrorLog.h"
#include "Entity.h"
#include "Campdisp.h"
#include "WinGraph.h"
#include "CampMap.h"
#include "CmpClass.h"
#include "classtbl.h"

// Campaign Specific Includes
#include "Campaign.h"
#include "Weather.h"

#define 	Cyan 			LightBrown
#define 	LightCyan 		Orange
#define 	LightMagenta	LightGray

WORD TerrainBMap[COVER_TYPES][RELIEF_TYPES] =
{ { 0,		0,      0,		0, },       // Water
  { 12,		13,     14,     15,},    	// Bog/Swamp
  { 32,		33,     34,     35,}, 		// Barren/Desert
  { 8,		9,		10,		11,}, 		// Plain/Farmland
  { 8,		9,      10,     11,},    	// Grass/Brush
  { 16,  	17,     18,    	19,},		// LightForest
  { 20,		21,     22,     23,},    	// HvyForest/Jungle
  { 28,		29,     30,     31,} }; 	// Urban

WORD ReliefBMap[RELIEF_TYPES] = { 7, 6, 5, 4 };

WORD CloudCoverBMap[8] = { 40, 41, 42, 43, 44, 45, 46, 47 };

WORD CloudLevelBMap[8] = { 40, 48, 49, 50, 51, 52, 53, 54 };

WORD SamCoverBMap[4] = { 55, 7, 31, 5 };

COLORREF SamCol[4] = { RGB_BLACK, RGB_GREEN, RGB_YELLOW, RGB_RED };

COLORREF CovCol[COVER_TYPES] = { RGB_BLUE,RGB_CYAN,RGB_LIGHTGRAY,RGB_BROWNGREEN,RGB_LIGHTGREEN,RGB_LIGHTGREEN,RGB_GREEN,RGB_YELLOW };

COLORREF RelCol[RELIEF_TYPES] = { RGB_LIGHTGREEN, RGB_GREEN, RGB_BROWN, RGB_WHITE };

COLORREF SideColRGB[NUM_COUNS] = { RGB_WHITE, RGB_GREEN, RGB_BLUE, RGB_LIGHTGRAY, RGB_CYAN, RGB_YELLOW, RGB_RED };

COLORREF GradCol[8][2] = {	{ RGB_BLUE, RGB_BLUE },					{ RGB_WHITE, RGB_WHITE },
							{ RGB_WHITE, RGB_LIGHTGRAY },			{ RGB_LIGHTGRAY, RGB_LIGHTGRAY },
							{ RGB_LIGHTGRAY, RGB_GRAY },			{ RGB_GRAY, RGB_GRAY },
							{ RGB_GRAY, RGB_BLACK },				{ RGB_BLACK, RGB_BLACK } };

COLORREF CloudCol[6][4] = {	{ RGB_BLUE, RGB_BLUE, RGB_BLUE, RGB_BLUE },
							{ RGB_BLUE, RGB_BLUE, RGB_BLUE, RGB_WHITE },
							{ RGB_BLUE, RGB_WHITE, RGB_WHITE, RGB_BLUE },
							{ RGB_LIGHTGRAY, RGB_LIGHTGRAY, RGB_LIGHTGRAY, RGB_LIGHTGRAY },
							{ RGB_GRAY, RGB_GRAY, RGB_GRAY, RGB_GRAY },
							{ RGB_BLACK, RGB_GRAY, RGB_GRAY, RGB_BLACK } };

WORD AltitudeBMap[10] = {0,0,0,0,0,0,0,0,0,0};  // Set later

int LightCover    [8] = {0x40, 0xE0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00};
int ModerateCover [8] = {0x40, 0xE0, 0x40, 0x00, 0x04, 0x0E, 0x04, 0x00};
int HeavyCover    [8] = {0x55, 0xEE, 0x55, 0xBB, 0x55, 0xEE, 0x55, 0xBB};

char SideColor[NUM_COUNS] = { White, Green, LightBlue, Magenta, Red, Red, Red };
char TeamColor[NUM_TEAMS] = { White, Green, LightBlue, Magenta, Red, Red, Red };
char TypeColor[35] = {	White, Red, Red, Red, Magenta, Magenta, LightGray, Orange, Black, Red,
						Orange, Orange, Magenta, Red, Magenta, LightGray, LightGray, Orange, Magenta, Red,
						Orange, Red, LightGray, LightGray, LightGray, Orange, LightGray, Magenta, Black, Yellow,
						Red, Red, White, White };

char Relstr[6][5] =  { "None", "Ally", "Frnd", "Neut", "Host", "War" };
char Stastr[4][5] =  { "None", "Oper", "Dam", "Dest" };

char  Num[18][3] = {"0 ", "1 ", "2 ", "3 ", "4 ", "5 ",
                   "6 ", "7 ", "8 ", "9 ", "10", "11",
                   "12", "13", "14", "15", "16", "17"};
extern unsigned char Saved;
extern unsigned char StateEdit;
extern CampaignState StateToEdit;
extern char ObjMode;
extern Team ThisTeam;
extern int ShowReal;

char* GetFilename (short x, short y);

// --------------------------------------
// Campaign Cell Display & Edit Functions
// --------------------------------------

void drawRoad(HDC DC, short ScreenX, short ScreenY, short Size, int i)
	{
	short	sx=0,sy=0,tx=0,ty=0;

	ShiAssert(i>= 0 && i <= 7);
	i = max(0, min(i, 7));
	switch (i)
		{
		case	0:
			sx = ScreenX+Size/2;
			sy = ScreenY+Size/2;
			tx = ScreenX+Size/2;
			ty = ScreenY-1;
			break;
		case	1:
			sx = ScreenX+Size/2;
			sy = ScreenY-1;
			tx = ScreenX+Size;
			ty = ScreenY+Size/2;
//			tx = ScreenX+Size+3;
//			ty = ScreenY-3;
			break;
		case	2:
			sx = ScreenX+Size/2;
			sy = ScreenY+Size/2;
			tx = ScreenX+Size;
			ty = ScreenY+Size/2;
			break;
		case	3:
			sx = ScreenX+Size/2;
			sy = ScreenY+Size;
			tx = ScreenX+Size;
			ty = ScreenY+Size/2;
//			tx = ScreenX+Size+1;
//			ty = ScreenY+Size+1;
			break;
		case	4:
			sx = ScreenX+Size/2;
			sy = ScreenY+Size/2;
			tx = ScreenX+Size/2;
			ty = ScreenY+Size;
			break;
		case	5:
			sx = ScreenX-1;
			sy = ScreenY+Size/2;
			tx = ScreenX+Size/2;
			ty = ScreenY+Size;
//			tx = ScreenX-3;
//			ty = ScreenY+Size+3;
			break;
		case	6:
			sx = ScreenX+Size/2;
			sy = ScreenY+Size/2;
			tx = ScreenX-1;
			ty = ScreenY+Size/2;
			break;
		case	7:
			sx = ScreenX-1;
			sy = ScreenY+Size/2;
			tx = ScreenX+Size/2;
			ty = ScreenY-1;
//			tx = ScreenX-2;
//			ty = ScreenY-2;
			break;
		}
	_moveto(DC,sx,sy);
	_lineto(DC,tx,ty);
	}

void DisplayCellData (HDC DC, GridIndex x, GridIndex y, 
					  short ScreenX, short ScreenY, short Size, char DataMode,
                      unsigned char Roads, unsigned char Rails)
	{
	ReliefType  r;
	CoverType   c;
	CellData	TheCell;
	int			w,sides;
	int			i,ofx,ofy;
	uchar		side[8];
	int			mx,my;

	// BEGIN
	TheCell = GetCell(x,y);
	r = GetReliefType(TheCell);
	c = GetGroundCover(TheCell);
	if (c==Water && DataMode < 5)
		DataMode=0;
	switch (Size)
		{
		case 1:
			// Special case for largest map
			switch (DataMode)
				{
				case 2:
					if (c != Water)
						SetPixel(DC,ScreenX,ScreenY,RelCol[r]);
					else
						SetPixel(DC,ScreenX,ScreenY,CovCol[c]);
					break;
				case 5:
					w = ((WeatherClass*)realWeather)->GetCloudCover(x,y);
					//JAM - FIXME
//					if (w >= FIRST_OVC_TYPE)
//						w = FIRST_OVC_TYPE;
					i = (x & 0x01) + 2*(y & 0x01);
					SetPixel(DC,ScreenX,ScreenY,CloudCol[w][i]);
					break;
				case 6:
					w = ((WeatherClass*)realWeather)->GetCloudLevel(x,y) / 32;
					i = (x & 0x01) & (y & 0x01);
					SetPixel(DC,ScreenX,ScreenY,GradCol[w][i]);
					break;
				case 10:
				case 11:
					mx = x / MAP_RATIO;
					my = y / MAP_RATIO;
					i = my*MRX + mx;
					w = (TheCampaign.SamMapData[i] >> (4+2*(DataMode-10))) & 0x03;
					SetPixel(DC,ScreenX,ScreenY,SamCol[w]);
					break;
				default:
					SetPixel(DC,ScreenX,ScreenY,CovCol[c]);
					break;
				}
			return;					
		case 8:
			ofx = 8*(x & 1);
			ofy = 8*(y & 1);
			break;
		default:
			ofx = ofy = 0;
			break;
		}
	switch (DataMode)
		{
		case 0:	
			_drawbmap(DC,TerrainBMap[c][r],ScreenX,ScreenY,Size,ofx,ofy);
			break;
		case 1:
			_drawbmap(DC,TerrainBMap[c][0],ScreenX,ScreenY,Size,ofx,ofy);
			break;
		case 2:
			_drawbmap(DC,ReliefBMap[r],ScreenX,ScreenY,Size,ofx,ofy);
			break;
//		case 3:
//			_drawsbmap(AltitudeBMap[GetAltitudeCode(TheCell)],ScreenX,ScreenY,Size);
//			break;
		case 5:
			w = ((WeatherClass*)realWeather)->GetCloudCover(x,y);
			//JAM - FIXME
//			if (w >= FIRST_OVC_TYPE)
//				w = FIRST_OVC_TYPE;
			_drawbmap(DC,CloudCoverBMap[w],ScreenX,ScreenY,Size,ofx,ofy);
			return;
			break;
		case 6:
			w = ((WeatherClass*)realWeather)->GetCloudLevel(x,y) / 32;
			_drawbmap(DC,CloudLevelBMap[w],ScreenX,ScreenY,Size,ofx,ofy);
			return;
		case 9:
			{
			char	*fn;

			// Texture set
			fn = GetFilename(x,y);
			if (strncmp(fn,"HBCIT",5)==0)
				_drawbmap(DC,CloudLevelBMap[0],ScreenX,ScreenY,Size,ofx,ofy);
			if (strncmp(fn,"HCITY",5)==0)
				_drawbmap(DC,CloudLevelBMap[1],ScreenX,ScreenY,Size,ofx,ofy);
			if (strncmp(fn,"HFARM",5)==0)
				_drawbmap(DC,CloudLevelBMap[2],ScreenX,ScreenY,Size,ofx,ofy);
			if (strncmp(fn,"HFORR",5)==0)
				_drawbmap(DC,CloudLevelBMap[3],ScreenX,ScreenY,Size,ofx,ofy);
			if (strncmp(fn,"HFTOP",5)==0)
				_drawbmap(DC,CloudLevelBMap[4],ScreenX,ScreenY,Size,ofx,ofy);
			if (strncmp(fn,"HFLAT",5)==0)
				_drawbmap(DC,CloudLevelBMap[5],ScreenX,ScreenY,Size,ofx,ofy);
			if (strncmp(fn,"HCOST",5)==0)
				_drawbmap(DC,CloudLevelBMap[6],ScreenX,ScreenY,Size,ofx,ofy);
			if (strncmp(fn,"HBARE",5)==0)
				_drawbmap(DC,CloudLevelBMap[7],ScreenX,ScreenY,Size,ofx,ofy);
			if (strncmp(fn,"HBHIL",5)==0)
				_drawbmap(DC,CloudLevelBMap[0],ScreenX,ScreenY,Size,ofx,ofy);
			}
			break;
		case 10:
		case 11:
			mx = x / MAP_RATIO;
			my = y / MAP_RATIO;
			i = my*MRX + mx;
			w = (TheCampaign.SamMapData[i] >> (4+2*(DataMode-10))) & 0x03;
			_drawbmap(DC,SamCoverBMap[w],ScreenX,ScreenY,Size,ofx,ofy);
			break;
		}
	if (Roads)
		if (GetRoadCell(TheCell))
			{
			_setcolor(DC,Brown);
			memset(side,0,8);
			sides = 0;
			// Find sides which have roads
			for (i=0;i<8;i+=2)
				{
				if (GetRoadCell(GetCell((GridIndex)(x+dx[i]),(GridIndex)(y+dy[i]))))
					{
					side[i] = 1;
					sides++;
					}
				}
			// Cut corners
			if (sides == 2)
				{
				for (i=0;i<8;i+=2)
					{
					if (side[i] && side[(i+2)%8])
						{
						side[i] = side[(i+2)%8] = 0;
						side[i+1] = 1;
						}
					}
				}
			// draw them
			for (i=0;i<8;i++)
				{
				if (side[i])
					{
					drawRoad(DC,ScreenX,ScreenY,Size,i);
					drawRoad(DC,ScreenX,(short)(ScreenY-1),Size,i);
					drawRoad(DC,(short)(ScreenX-1),ScreenY,Size,i);
					}
				}
			}
	if (Rails)
		if (GetRailCell(TheCell))
			{
			_setcolor(DC,Black);
			memset(side,0,8);
			sides = 0;
			// Find sides which have roads
			for (i=0;i<8;i+=2)
				{
				if (GetRoadCell(GetCell((GridIndex)(x+dx[i]),(GridIndex)(y+dy[i]))))
					{
					side[i] = 1;
					sides++;
					}
				}
			// Cut corners
			if (sides == 2)
				{
				for (i=0;i<8;i+=2)
					{
					if (side[i] && side[(i+2)%8])
						{
						side[i] = side[(i+2)%8] = 0;
						side[i+1] = 1;
						}
					}
				}
			// draw them
			for (i=0;i<8;i++)
				{
				if (side[i])
					{
					drawRoad(DC,ScreenX,ScreenY,Size,i);
					drawRoad(DC,ScreenX,(short)(ScreenY-1),Size,i);
					drawRoad(DC,(short)(ScreenX-1),ScreenY,Size,i);
					}
				}
			}
	// END
	}

// ----------------------------------
// Objective Display & Edit Functions
// ----------------------------------

void DisplayObjective (HDC DC, Objective O, short ScreenX, short ScreenY, short Size)
	{
	short   ULX;
	short   ULY;
	short   LRX;
	short   LRY;
	short	off=0;
	char  	C;

	if (Size > 7)
		off = 1;
	ULX = ScreenX+off;
	ULY = ScreenY+off;
	LRX = ScreenX+Size-off;
	LRY = ScreenY+Size-off;

	C = O->GetOwner();
	if (StateEdit)
		{
		switch (0) // GetStateIDPriority(StateToEdit,O->GetObjectiveID()))
			{
			case SPRI_NONE:
				_setcolor(DC,White);
				break;
			case SPRI_NEEDED:
				_setcolor(DC,LightGreen);
				break;
			case SPRI_SECONDARY:
				_setcolor(DC,Yellow);
				break;
			case SPRI_PRIMARY:
				_setcolor(DC,Red);
			}
		}
	else
		{
		switch (ObjMode)
			{
			case 1:
				if (O->IsPrimary())
					_setcolor(DC,Red);
				else if (O->IsSecondary())
					_setcolor(DC,LightBlue);
				else
					_setcolor(DC,Black);
				break;
			case 2:
				_setcolor(DC,TypeColor[O->GetType()]);
				break;
			default:
				_setcolor(DC,SideColor[C]);
				break;
			}
		}
	_ellipse(DC,_GBORDER,ULX,ULY,LRX,LRY);
	if (LRX-ULX > 2)
		_ellipse(DC,_GBORDER,ULX+1,ULY+1,LRX-1,LRY-1);
	if (LRX-ULX > 15)
		_ellipse(DC,_GBORDER,ULX+2,ULY+2,LRX-2,LRY-2);
	}

char* ObjTypStr (Objective O)
	{
	return (O->GetObjectiveClassData())->Name;
	}

char* ObjStatusStr (Objective O)
	{
	return Stastr[O->GetObjectiveStatus()];
	}

void ObjFeatureStr (Objective O, char buffer[])
	{
	int		i;

	for (i=0;i<O->GetTotalFeatures();i++)
		buffer[i] = '0' + O->GetFeatureStatus(i);

	buffer[i] = 0;
	}

// ---------------------------------
// Unit Display & Edit Functions
// ---------------------------------

void DisplayUnit (HDC DC, Unit U, short ScreenX, short ScreenY,	short Size)
	{
	short ULX;
	short ULY;
	short LRX;
	short LRY;
	short ssmall,xsmall;
	short centerx,centery;
	short HQ=0,Type,SType,color;

//	BEGIN
	ULX = ScreenX;
	ULY = ScreenY;
	LRX = ScreenX+Size+(Size>>1);
	LRY = ScreenY+Size;
	centerx = (ULX+LRX)/2;
	centery = (ULY+LRY)/2;
	xsmall = (Size/4);

	color = SideColor[U->GetOwner()];
	_setcolor(DC,color);
	_rectangle(DC,_GFILLINTERIOR,ULX,ULY,LRX,LRY);
	_setcolor(DC,Black);
	_rectangle(DC,_GBORDER,ULX,ULY,LRX,LRY);
	Type = U->GetType();
	SType = U->GetSType();
	// Draw the Unit size markings (if land unit)
	if (U->GetDomain() == DOMAIN_LAND)
		{
		if (ShowReal == 3)
			{
			_moveto(DC,centerx-xsmall, ULY+1);
			_lineto(DC,centerx, ULY+xsmall);
			_lineto(DC,centerx+xsmall, ULY+1);
			_moveto(DC,centerx-xsmall, ULY+xsmall);
			_lineto(DC,centerx, ULY+1);
			_lineto(DC,centerx+xsmall, ULY+xsmall);
			}
		else if (Type == TYPE_BRIGADE)
			{
			_moveto(DC,centerx-Size/8, ULY+1);
			_lineto(DC,centerx+Size/8, ULY+xsmall);
			_moveto(DC,centerx-Size/8, ULY+xsmall);
			_lineto(DC,centerx+Size/8, ULY+1);
			}
		else if (Type == TYPE_BATTALION)
			{
			_moveto(DC,(LRX+ULX)/2-Size/6, ULY+1);
			_lineto(DC,(LRX+ULX)/2-Size/6, ULY+Size/4);
			_moveto(DC,(LRX+ULX)/2+Size/6, ULY+1);
			_lineto(DC,(LRX+ULX)/2+Size/6, ULY+Size/4);
			}
		}
	ULX += Size/5;       // Build the smaller box
	ULY += Size/4;
	LRX -= Size/5;
	LRY -= Size/8;
	ssmall = (LRY-ULY)/4;
	xsmall = Size/8;
	// Draw Cheesy Graphical Symbol here
	HQ = U->Parent();
	if (U->GetDomain() == DOMAIN_LAND)
		{
		_rectangle(DC,_GBORDER,ULX,ULY,LRX,LRY);
		if (SType == STYPE_UNIT_INFANTRY || SType == STYPE_UNIT_MECHANIZED || SType == STYPE_UNIT_MARINE)
			{
			_moveto(DC,ULX,ULY);
			_lineto(DC,LRX,LRY);
			_moveto(DC,LRX,ULY);
			_lineto(DC,ULX,LRY);
			}
		if (SType == STYPE_UNIT_ARMOR || SType == STYPE_UNIT_MECHANIZED || SType == STYPE_UNIT_ARMORED_CAV || SType == STYPE_UNIT_SP_ARTILLERY)
			{
			_ellipse(DC,_GBORDER,ULX+ssmall,ULY+ssmall,LRX-ssmall,LRY-ssmall);
			}
		if (SType == STYPE_UNIT_ARMORED_CAV)
			{
			_moveto(DC,LRX,ULY);
			_lineto(DC,ULX,LRY);
			}
		if (SType == STYPE_UNIT_AIRBORNE)
			{
			_moveto(DC,ULX+(int)(ssmall*1.8), ULY);
			_lineto(DC,centerx, (ULY+LRY)/2);
			_lineto(DC,LRX-(int)(ssmall*1.8), ULY);
			}
/*
		if (Type == Airborne)
            {
            _rectangle(DC,_GBORDER,ULX+(int)(ssmall*1.8),LRY-ssmall, centerx-1, LRY);
            _rectangle(DC,_GBORDER,centerx+1, LRY-ssmall, LRX-(int)(ssmall*1.8), LRY);
            }
*/
		if (SType == STYPE_UNIT_MARINE)
			{
            _moveto(DC,centerx-ssmall,ULY+(int)(xsmall/1.5));
            _lineto(DC,centerx+ssmall,ULY+(int)(xsmall/1.5));
            _moveto(DC,centerx, ULY+(int)(xsmall/1.5));
            _lineto(DC,centerx, (ULY+LRY)/2-(int)(xsmall/1.5));
            _lineto(DC,centerx+ssmall,(ULY+LRY)/2-xsmall);
            _moveto(DC,centerx-ssmall,(ULY+LRY)/2-xsmall);
            _lineto(DC,centerx, (ULY+LRY)/2-(int)(xsmall/1.5));
            }
		if (SType == STYPE_UNIT_AIR_DEFENSE)
            {
            _ellipse(DC,_GBORDER,ULX+ssmall,ULY+ssmall,LRX-ssmall,LRY-1);
            _setcolor(DC,color);
            _rectangle(DC,_GFILLINTERIOR,ULX+ssmall,(ULY+LRY)/2+1,LRX-ssmall,LRY-1);
            }																		 
		if (SType == STYPE_UNIT_SP_ARTILLERY  || SType == STYPE_UNIT_TOWED_ARTILLERY)
            {
            _ellipse(DC,_GFILLINTERIOR, (ULX+LRX)/2 - ssmall, ULY+ssmall, (ULX+LRX)/2 + ssmall, LRY-ssmall);
            }
		if (SType == STYPE_UNIT_SS_MISSILE || SType == STYPE_UNIT_ROCKET)
			{
            _moveto(DC,centerx, LRY-ssmall);
            _lineto(DC,centerx, ULY+ssmall);
			_lineto(DC,centerx-xsmall,centery);
            _moveto(DC,centerx, ULY+ssmall);
				_lineto(DC,centerx+xsmall,centery);
			}
		if (SType == STYPE_UNIT_ENGINEER)
            {
            _moveto(DC,ULX+ssmall, LRY-ssmall);
            _lineto(DC,ULX+ssmall, ULY+ssmall);
            _lineto(DC,LRX-ssmall, ULY+ssmall);
            _lineto(DC,LRX-ssmall, LRY-ssmall);
            _moveto(DC,(ULX+LRX)/2, ULY+ssmall);
            _lineto(DC,(ULX+LRX)/2, LRY-ssmall);
            }
		if (SType == STYPE_UNIT_HQ)
            {
            _moveto(DC,ULX+ssmall,ULY+ssmall);
            _lineto(DC,ULX+ssmall,LRY-ssmall);
            _moveto(DC,centerx-xsmall,ULY+ssmall);
            _lineto(DC,centerx-xsmall,LRY-ssmall);
            _moveto(DC,ULX+ssmall,(LRY+ULY)/2);
            _lineto(DC,centerx-xsmall,(LRY+ULY)/2);
            _rectangle(DC,_GBORDER,centerx+xsmall,ULY+ssmall,LRX-ssmall,LRY-ssmall);
            _moveto(DC,LRX-ssmall,LRY-ssmall);
            _lineto(DC,LRX-ssmall-xsmall,LRY-ssmall-xsmall);
            }
		}
	else if (U->GetDomain() == DOMAIN_AIR)
		{
		if (SType == STYPE_UNIT_ATTACK_HELO || SType == STYPE_UNIT_RECON_HELO || SType == STYPE_UNIT_TRANSPORT_HELO)
            {
            _ellipse(DC,_GFILLINTERIOR,ULX+ssmall,ULY+ssmall,centerx-1,ULY+ssmall*2);
            _ellipse(DC,_GFILLINTERIOR,centerx+1,ULY+ssmall,LRX-ssmall,ULY+ssmall*2);
            _moveto(DC,centerx,ULY+ssmall);
            _lineto(DC,centerx,LRY-xsmall);
            }
		else
            {
			if (Type == TYPE_FLIGHT)
				{
				_moveto(DC,centerx+ssmall,ULY+ssmall-1);
				_lineto(DC,centerx-ssmall,ULY+ssmall-1);
				_lineto(DC,centerx-ssmall,LRY-ssmall);
				_moveto(DC,centerx-ssmall,(ULY+LRY)/2);
				_lineto(DC,centerx+xsmall,(ULY+LRY)/2);
				}
			else if (Type == TYPE_PACKAGE)
				{
				_moveto(DC,centerx+ssmall,ULY+ssmall-1);
				_lineto(DC,centerx-ssmall,ULY+ssmall-1);
				_lineto(DC,centerx-ssmall,LRY-ssmall);
				_moveto(DC,centerx-ssmall,(ULY+LRY)/2);
				_lineto(DC,centerx+ssmall,(ULY+LRY)/2);
				_lineto(DC,centerx+ssmall,ULY+ssmall-1);
				}
			else if (Type == TYPE_SQUADRON)
				{
				_moveto(DC,centerx+ssmall,ULY+ssmall-1);
				_lineto(DC,centerx-ssmall,ULY+ssmall-1);
				_lineto(DC,centerx-ssmall,(ULY+LRY)/2);
				_lineto(DC,centerx+ssmall,(ULY+LRY)/2);
				_lineto(DC,centerx+ssmall,LRY-ssmall);
				_lineto(DC,centerx-ssmall,LRY-ssmall);
				}
            }
		}
	else
		{
		if (SType >= 0)
            {
            _moveto(DC,centerx-ssmall,LRY-ssmall);
            _lineto(DC,centerx-ssmall,ULY+ssmall-1);
            _lineto(DC,centerx+ssmall,LRY-ssmall);
            _lineto(DC,centerx+ssmall,ULY+ssmall-1);
            }
		}
	}

void ShowUnitSymbol (HDC DC, Unit U, short ULX, short ULY)
	{
	DisplayUnit(DC, U, (short)(ULX+60), (short)(ULY+10), 48);
	_setcolor(DC,Black);
	_moveto(DC,ULX+150, ULY+20);
	_outgtext(DC,Side[U->GetOwner()]);
	_moveto(DC,ULX+20, ULY+20);
	_outgtext(DC,Num[U->GetCampID()/100]);
	_moveto(DC,ULX+28, ULY+20);
	_outgtext(DC,Num[(U->GetCampID()%100)/10]);
	_moveto(DC,ULX+36, ULY+20);
	_outgtext(DC,Num[U->GetCampID()%10]);
	_moveto(DC,ULX+10, ULY+40);
	}

char* UnitTypeStr (Unit u, int stype, char buffer[])
	{
	if (u->GetDomain() == DOMAIN_LAND)
		{
		if (stype == STYPE_UNIT_INFANTRY)
			sprintf(buffer,"Infantry\0");
		else if (stype == STYPE_UNIT_MECHANIZED)
            sprintf(buffer,"Mechanized\0");
		else if (stype == STYPE_UNIT_ARMOR)
            sprintf(buffer,"Armored\0");
		else if (stype == STYPE_UNIT_ARMORED_CAV)
			sprintf(buffer,"Armored Cav\0");
		else if (stype == STYPE_UNIT_MARINE)
			sprintf(buffer,"Marine\0");
		else if (stype == STYPE_UNIT_AIR_DEFENSE)
			sprintf(buffer,"Air Defense\0");
		else if (stype == STYPE_UNIT_HQ)
			sprintf(buffer,"HQ\0");
		else if (stype == STYPE_UNIT_SP_ARTILLERY)
			sprintf(buffer,"SP Artiller\0");
		else if (stype == STYPE_UNIT_TOWED_ARTILLERY)
			sprintf(buffer,"Towed Artil\0");
		else if (stype == STYPE_UNIT_ENGINEER)
			sprintf(buffer,"Engineer\0");
		else if (stype == STYPE_UNIT_AIRMOBILE)
			sprintf(buffer,"Airmobile\0");
//		else if (stype == STYPE_UNIT_GROUND_TRANSPORT)
//			sprintf(buffer,"Transport\0");
		else
			sprintf(buffer,"Other\0");
		}
	else if (u->GetDomain() == DOMAIN_AIR)
		{
		if (stype == STYPE_UNIT_ATTACK_HELO)
            sprintf(buffer,"Attack Helo\0");
		else if (stype == STYPE_UNIT_TRANSPORT_HELO)
			sprintf(buffer,"Army Aviat\0");
		else if (stype == STYPE_UNIT_FIGHTER)
			sprintf(buffer,"Fighter\0");
		else if (stype == STYPE_UNIT_FIGHTER_BOMBER)
			sprintf(buffer,"Fight/Bomb\0");
		else if (stype == STYPE_UNIT_ATTACK)
			sprintf(buffer,"Attack\0");
		else if (stype == STYPE_UNIT_BOMBER)
			sprintf(buffer,"Bomber\0");
		else if (stype == STYPE_UNIT_AIR_TRANSPORT)
			sprintf(buffer,"Air Transp\0");
		else if (stype == STYPE_UNIT_RECON)
			sprintf(buffer,"Recon\0");
		else if (stype == STYPE_UNIT_ECM)
			sprintf(buffer,"ECM\0");
		else if (stype == STYPE_UNIT_AWACS)
			sprintf(buffer,"AWACS\0");
		else if (stype == STYPE_UNIT_JSTAR)
			sprintf(buffer,"JSTAR\0");
		else if (stype == STYPE_UNIT_TANKER)
			sprintf(buffer,"Tanker\0");
		else
			sprintf(buffer,"Other\0");
		}
	else 
		{
		if (stype == STYPE_UNIT_CARRIER)
            sprintf(buffer,"Carrier\0");
		else if (stype == STYPE_UNIT_BATTLESHIP)
			sprintf(buffer,"Battleship\0");
		else if (stype == STYPE_UNIT_CRUISER)
			sprintf(buffer,"Cruiser\0");
		else if (stype == STYPE_UNIT_FRIGATE)
			sprintf(buffer,"Frigate\0");
		else if (stype == STYPE_UNIT_SEA_SUPPLY)
			sprintf(buffer,"Supply\0");
		else if (stype == STYPE_UNIT_SEA_TRANSPORT)
			sprintf(buffer,"Transport\0");
		else if (stype == STYPE_UNIT_AMPHIBIOUS)
			sprintf(buffer,"Amphibious\0");
//		else if (stype == STYPE_UNIT_SSN)
//			sprintf(buffer,"Attk Sub\0");
		else
			sprintf(buffer,"Other\0");
		}
	return buffer;
	}
	
// =============================================================
// Relations display routines
// =============================================================

void ShowWithTeam (HDC DC, Control who, Control with, short ULX, short ULY)
	{
	_setcolor(DC,White);
	_rectangle(DC,_GFILLINTERIOR,ULX+80+35*with,ULY+10+10*who,ULX+114+35*with,ULY+19+10*who);
	_setcolor(DC,Black);
	_moveto(DC,ULX+80+35*with,ULY+10+10*who);
	switch (GetCTRelations(who, with))
		{
		case  Neutral:
			_outgtext(DC,"Neut\0");
			break;
		case  Hostile:
			_outgtext(DC,"Host\0");
			break;
		case  War:
			_outgtext(DC,"War\0");
			break;
		case  Friendly:
			_outgtext(DC,"Frnd\0");
			break;
		case  Allied:
			_outgtext(DC,"Ally\0");
			break;
		}
	}

// ==========================================
// Misc functions
// ==========================================

void DisplaySideRange(HDC DC, Control c, short x, short y, int range)
	{
	_setcolor(DC,SideColor[c]);
	_ellipse(DC,_GBORDER,x-range,y-range,x+range,y+range);
	}

#endif CAMPTOOL