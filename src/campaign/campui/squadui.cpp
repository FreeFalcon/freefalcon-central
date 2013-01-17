
#include <stdio.h>
#include <string.h>
#include "cmpglobl.h"
#include "F4Vu.h"
#include "CampList.h"
#include "Squadron.h"
#include "SquadUI.h"
#include "Campaign.h"
#include "Objectiv.h"
#include "Find.h"

// =====================
// Globals
// =====================

// =====================
// SquadUIInfo Class
// =====================

SquadUIInfoClass::SquadUIInfoClass (void)
	{
	x = y = 0.0F;
	id = FalconNullId;
	dIndex = 0;
	nameId = -1;
	specialty = 0;
	currentStrength = 0;
	country = 0;
	airbaseIcon = 0;
	squadronPatch = 0;
	sprintf(airbaseName,"None");
	}

// =====================
// Other functions
// =====================

