#ifndef _CPVBOUNDS_H
#define _CPVBOUNDS_H

#include "windows.h"

#define BOUNDS_HUD		0
#define BOUNDS_RWR		1
#define BOUNDS_MFDLEFT	2
#define BOUNDS_MFDRIGHT	3	
#define BOUNDS_MFD3		4			//Wombat778 4-12-04
#define BOUNDS_MFD4		5			//Wombat778 4-12-04
#define BOUNDS_TOTAL		6		//Wombat778 4-12-04 changed from 4 to 6

typedef struct {
			float			top;
			float			left;
			float			bottom;
			float			right;
			} ViewportBounds;

// sfr: 2 scale factors
void ConvertRecttoVBounds(RECT*, ViewportBounds*, int, int, float hScale, float vScale);  //Wombat778 10-06-2003	Changed scale from int to float

#endif
