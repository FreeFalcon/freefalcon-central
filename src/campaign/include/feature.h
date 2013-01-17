// ********************************************************
//
// feature.h
//
// ********************************************************

#ifndef FEATURE_H
#define FEATURE_H

#include "entity.h"

// Feature flags
#define	FEAT_EREPAIR				0x01				// Set if enemy can repair this building
#define FEAT_VIRTUAL				0x02				// Don't deaggregate as an entity

#define FEAT_HAS_LIGHT_SWITCH		0x04				// Tells sim to turn on the lights at night
#define FEAT_HAS_SMOKE_STACK		0x08				// Tells sim to add smoke

#define FEAT_FLAT_CONTAINER			0x100				// Vehicles typically sit on this
#define FEAT_ELEV_CONTAINER			0x200

#define FEAT_CAN_EXPLODE			0x400
#define FEAT_CAN_BURN				0x800
#define FEAT_CAN_SMOKE				0x1000
#define FEAT_CAN_COLAPSE			0x2000

#define FEAT_CONTAINER_TOP			0x4000
#define FEAT_NEXT_IS_TOP			0x8000

// 2002-02-06 MN this feature is not to be added to mission evaluation (trees...)
#define FEAT_NO_HITEVAL				0x10

// Feature Entry flags
#define FEAT_PREV_CRIT				0x01				// Feature associations
#define FEAT_NEXT_CRIT				0x02
#define FEAT_PREV_NORM				0x04
#define FEAT_NEXT_NORM				0x08

// ---------------------------------------
// External Function Declarations
// ---------------------------------------

extern FeatureClassDataType* GetFeatureClassData (int index);

extern int GetFeatureRepairTime (int index);

extern int GetFeatureHitChance (int id, int mt, int range, int hitflags);

extern int GetAproxFeatureHitChance (int id, int mt, int range);

extern int CalculateFeatureHitChance (int id, int mt);

extern int GetFeatureCombatStrength (int id, int mt, int range);

extern int GetAproxFeatureCombatStrength (int id, int mt, int range);

extern int CalculateFeatureCombatStrength (int id, int mt);

extern int GetAproxFeatureRange (int id, int mt);

extern int GetFeatureRange (int id, int mt);

extern int CalculateFeatureRange (int id, int mt);

extern int GetFeatureDetectionRange (int id, int mt);

extern int GetBestFeatureWeapon(int id, uchar* dam, MoveType m, int range);

#endif
