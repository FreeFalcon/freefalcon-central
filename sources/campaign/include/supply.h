#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include "CmpGlobl.h"
#include "ListADT.h"
#include "F4Vu.h"
#include "vutypes.h"
#include "Objectiv.h"
#include "Strategy.h"
#include "Unit.h"
#include "Find.h"
#include "Path.h"
#include "Campaign.h"
#include "Update.h"
#include "f4vu.h"
#include "CampList.h"
#include "gtm.h"
#include "team.h"

#ifndef SUPPLY_H
#define SUPPLY_H

#define SUPPLY_PT_FUEL		10000			// How many lbs of fuel each point of supply fuel is worth

// ==================
// Supply functions
// ==================

extern int ProduceSupplies (CampaignTime delta);

extern int SupplyUnits (Team who, CampaignTime delta);

#endif