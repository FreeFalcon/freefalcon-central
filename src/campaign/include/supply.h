#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include "cmpglobl.h"
#include "listadt.h"
#include "f4vu.h"
#include "vutypes.h"
#include "objectiv.h"
#include "strategy.h"
#include "unit.h"
#include "find.h"
#include "path.h"
#include "campaign.h"
#include "update.h"
#include "f4vu.h"
#include "camplist.h"
#include "gtm.h"
#include "team.h"

#ifndef SUPPLY_H
#define SUPPLY_H

#define SUPPLY_PT_FUEL                                                         \
    10000 // How many lbs of fuel each point of supply fuel is worth

// ==================
// Supply functions
// ==================

extern int ProduceSupplies(CampaignTime delta);

extern int SupplyUnits(Team who, CampaignTime delta);

#endif
