#ifndef SQUADUI_H
#define SQUADUI_H

#include <tchar.h>
#include "F4Vu.h"


// =====================
// Defines
// =====================

#define		CAMP_FLY_ANY_AIRCRAFT	8888						// Cheat to allow flying any aircraft

// =====================
// SquadUI Class
// =====================

// This stores all data we need to know about squadrons in preload
class SquadUIInfoClass {
	private:
	public:
		float				x;									// Sim coordinates of squadron
		float				y;
		VU_ID				id;									// VU_ID (not valid til Campaign Loads)
		short				dIndex;								// Description Index
		short				nameId;								// The UI's id into name and patch data
		short				airbaseIcon;
		short				squadronPatch;
		uchar				specialty;
		uchar				currentStrength;					// # of current active aircraft
		uchar				country;
		_TCHAR				airbaseName[40];					// Name of airbase (string)
	public:
		SquadUIInfoClass (void);
	};

// =====================
// Other functions
// =====================

void ReadValidAircraftTypes (char *typefile, short *vat);

#endif
