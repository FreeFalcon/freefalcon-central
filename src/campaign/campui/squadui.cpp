
#include <stdio.h>
#include <string.h>
#include "cmpglobl.h"
#include "f4vu.h"
#include "camplist.h"
#include "squadron.h"
#include "squadui.h"
#include "campaign.h"
#include "objectiv.h"
#include "find.h"

// =====================
// Globals
// =====================

// =====================
// SquadUIInfo Class
// =====================

SquadUIInfoClass::SquadUIInfoClass(void)
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
    sprintf(airbaseName, "None");
}

// =====================
// Other functions
// =====================
