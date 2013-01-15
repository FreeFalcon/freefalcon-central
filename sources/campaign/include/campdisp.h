#ifndef DISPLAY_H
#define DISPLAY_H

#include "CampCell.h"
#include "CampTerr.h"
#include "Objectiv.h"
#include "Strategy.h"
#include "Unit.h"

// --------------------------------------
// Public functions
// --------------------------------------

extern char ColTable[];
extern char SideColor[NUM_COUNS];
extern char TeamColor[NUM_TEAMS];
extern char Side[NUM_COUNS][3];

extern void DisplayCellData  (	HDC DC,
  								GridIndex 	x, 
   							 	GridIndex 	y,
                         		short    ScreenX,
                         		short    ScreenY,
                        		short    Size,
                         		char     DataMode,
                         		unsigned char Roads,
								unsigned char Rails);

extern void DisplayObjective (HDC DC, Objective O, short ScreenX, short ScreenY, short Size);

extern char* ObjTypStr (Objective O);

extern char* ObjStatusStr (Objective O);

extern void ObjFeatureStr (Objective O, char buffer[]);

extern void ObjOffsetStr (Objective O, char buffer[]);

extern void DisplayUnit (HDC DC, Unit U, short ScreenX, short ScreenY, short Size);

extern void ShowSubunitInfo(HDC DC, Unit U, short ULX, short ULY, short Set, short i, int asagg);

extern void ShowUnitSymbol (HDC DC, Unit U, short ULX, short ULY);

extern char* UnitTypeStr (Unit u, int stype, char buffer[]);

extern char* UnitSPTypeStr (Unit u, char buffer[]);

extern char* UnitOrdersStr (Unit u, char buffer[]);

extern void ShowUnitInfo (HDC DC, Unit U, int FLX, int FLY, int TLX, int TLY, int LX, int OLY, int CSLX, int CSLY, int ORLX, int ORLY);

extern void ShowStates(HDC DC, Team who);

extern void DisplayState (HDC DC, CampaignState s);

extern void DisplaySideRange(HDC DC, Control c, short x, short y, int range);

extern char* GetNumberName(int nameid, char *buffer);

#endif
