#include "stdhdr.h"
#include "f4vu.h"
#include "simfiltr.h"
#include "classtbl.h"
#include "persist.h"
#include "simfeat.h"
#include "feature.h"
#include "flight.h"

extern int F4FlyingEyeType;

// =========================================
// Usefull functions. Not sure why it's here
// =========================================

int SimCompare (VuEntity *ent1, VuEntity *ent2){
	int retval = 0;
	if (ent1 && ent2 && ent1->Id() != ent2->Id()){
		retval = (ent2->Id() > ent1->Id() ? 1 : -1);
	}
	return (retval);
}

// ==========================================
// AllSimFilter (All Vehicles and Features)
// ==========================================

AllSimFilter::AllSimFilter(void){
}

AllSimFilter::~AllSimFilter(void)
{
}

VU_BOOL AllSimFilter::Test (VuEntity* ent1)
{
	VuEntityType* classPtr;
	VU_BOOL retval = FALSE;

	if (ent1->Type() > VU_LAST_ENTITY_TYPE){
		classPtr = ent1->EntityType();
		if (
			classPtr->classInfo_[VU_DOMAIN] > DOMAIN_ABSTRACT &&
			(	
				classPtr->classInfo_[VU_CLASS] == CLASS_VEHICLE || 
				classPtr->classInfo_[VU_CLASS] == CLASS_FEATURE
			) &&
			(((FalconEntity*)ent1)->IsSim()) &&
			!(((FalconEntity*)ent1)->IsPersistant())
		){
			retval = TRUE;
		}
	}
	return (retval);
}

VU_BOOL AllSimFilter::RemoveTest (VuEntity* ent1)
{
   return Test(ent1);
}

int AllSimFilter::Compare(VuEntity *ent1, VuEntity *ent2)
{
   return (SimCompare (ent1, ent2));
}

VuFilter* AllSimFilter::Copy (void)
{
   return new AllSimFilter;
}

// ==========================================
// CombinedSimFilter (All Vehicles and Features)
// ==========================================

CombinedSimFilter::CombinedSimFilter(void)
{
}

CombinedSimFilter::~CombinedSimFilter(void)
{
}

VU_BOOL CombinedSimFilter::Test (VuEntity* ent1)
{
VU_BOOL retval = FALSE;

   if (ent1->Type() > VU_LAST_ENTITY_TYPE)
   {
      retval = TRUE;
   }
   return (retval);
}

VU_BOOL CombinedSimFilter::RemoveTest (VuEntity* ent1)
{
   return Test(ent1);
}

int CombinedSimFilter::Compare(VuEntity *ent1, VuEntity *ent2)
{
   return (SimCompare (ent1, ent2));
}

VuFilter* CombinedSimFilter::Copy (void)
{
   return new CombinedSimFilter;
}

// ==========================================
// SimFeatureFilter (All Features)
// ==========================================

SimFeatureFilter::SimFeatureFilter(void)
{
}

SimFeatureFilter::~SimFeatureFilter(void)
{
}

VU_BOOL SimFeatureFilter::Test (VuEntity* ent1)
{
VuEntityType* classPtr;
VU_BOOL retval = FALSE;

   if (ent1->Type() > VU_LAST_ENTITY_TYPE)
   {
      classPtr = ent1->EntityType();
/*  This was removed by Kevin 10/14/97 because it appeared to block required notifications (SCR)
      if (classPtr->classInfo_[VU_CLASS] == CLASS_FEATURE &&
           (((FalconEntity*)ent1)->IsSim()) &&
          !(((FalconEntity*)ent1)->IsPersistant()) &&
          !(((SimFeatureClass*)ent1)->IsSetCampaignFlag(FEAT_CONTAINER_TOP)))
*/
      if (classPtr->classInfo_[VU_DOMAIN] && classPtr->classInfo_[VU_CLASS] == CLASS_FEATURE)
      {
         retval = TRUE;
      }
   }

   return (retval);
}

VU_BOOL SimFeatureFilter::RemoveTest (VuEntity* ent1)
{
   return Test(ent1);
}

int SimFeatureFilter::Compare(VuEntity *ent1, VuEntity *ent2)
{
   return (SimCompare (ent1, ent2));
}

VuFilter* SimFeatureFilter::Copy (void)
{
   return new SimFeatureFilter;
}

// ==========================================
// SimLocalFilter (All Local sim entities)
// ==========================================

SimLocalFilter::SimLocalFilter(void)
{
}

SimLocalFilter::~SimLocalFilter(void)
{
}

VU_BOOL SimLocalFilter::Test (VuEntity* ent1)
{
	VuEntityType* 
		classPtr;

	VU_BOOL 
		retval = FALSE;

	classPtr = ent1->EntityType();

   if (!classPtr->classInfo_[VU_DOMAIN])
	   return retval;

   // edg: I'm not sure if this is the right way to test this, but
   // it was crashing in the next statement.   Look for flying eye
   // camera type
   if ( ent1->IsLocal() &&
   		ent1->Type() == VU_LAST_ENTITY_TYPE + F4FlyingEyeType )
   {
	   return TRUE;
   }

   if
	(
		ent1->Type() > VU_LAST_ENTITY_TYPE &&
		ent1->IsLocal() &&
		((FalconEntity*)ent1)->IsSim()
	)
	{
		if
		(
			(
            classPtr->classInfo_[VU_CLASS]  == CLASS_VEHICLE &&
            classPtr->classInfo_[VU_DOMAIN] == DOMAIN_AIR
         ) ||
			(
				classPtr->classInfo_[VU_DOMAIN] == DOMAIN_AIR &&
				classPtr->classInfo_[VU_CLASS] == CLASS_SFX &&
				classPtr->classInfo_[VU_TYPE] == TYPE_EJECT
			)
		)
		{
		   retval = TRUE;
		}
	}

   return (retval);
}

//VU_BOOL SimLocalFilter::RemoveTest (VuEntity* ent1)
VU_BOOL SimLocalFilter::RemoveTest (VuEntity*)
{
return TRUE;		// KCK NOTE: Until we can remove on ownership transfer
}

int SimLocalFilter::Compare(VuEntity *ent1, VuEntity *ent2)
{
   return (SimCompare (ent1, ent2));
}

VuFilter* SimLocalFilter::Copy (void)
{
   return new SimLocalFilter;
}

// ==============================================
// SimObjectFilter (All air sim entities)
// ==============================================

SimObjectFilter::SimObjectFilter(void)
{
}

SimObjectFilter::~SimObjectFilter(void)
{
}

VU_BOOL SimObjectFilter::Test (VuEntity* ent1)
{
	VuEntityType* classPtr;

	classPtr = ent1->EntityType();

	if (ent1->Type() > VU_LAST_ENTITY_TYPE && classPtr->classInfo_[VU_DOMAIN] > DOMAIN_ABSTRACT)
	{
		if ((classPtr->classInfo_[VU_CLASS] == CLASS_VEHICLE || classPtr->classInfo_[VU_CLASS] == CLASS_WEAPON) && ((FalconEntity*)ent1)->IsSim())
			return TRUE;;
	}

   return FALSE;
}

VU_BOOL SimObjectFilter::RemoveTest (VuEntity* ent1)
{
   return Test(ent1);
}

int SimObjectFilter::Compare(VuEntity *ent1, VuEntity *ent2)
{
   return (SimCompare (ent1, ent2));
}

VuFilter* SimObjectFilter::Copy (void)
{
   return new SimObjectFilter;
}

// ==============================================
// SimSurfaceFilter (All no-feature surface entities) 
// ==============================================
/* KCK Not being used now that we tossed LM
SimSurfaceFilter::SimSurfaceFilter(uchar domain_to_filter)
{
	domain = domain_to_filter;
}

SimSurfaceFilter::~SimSurfaceFilter(void)
{
}

VU_BOOL SimSurfaceFilter::Test (VuEntity* ent1)
{
	if (ent1->Type() > VU_LAST_ENTITY_TYPE && ((FalconEntity*)ent1)->IsSim())
		{
		VuEntityType*	classPtr = ent1->EntityType();
		if (classPtr->classInfo_[VU_CLASS] == CLASS_VEHICLE)
			{
			if (domain != DOMAIN_ANY)
				{
				if (classPtr->classInfo_[VU_DOMAIN] == domain)
					return TRUE;
				}
			else if (classPtr->classInfo_[VU_DOMAIN] != DOMAIN_AIR)
				return TRUE;
			}
		}

	return FALSE;
}

VU_BOOL SimSurfaceFilter::RemoveTest (VuEntity* ent1)
{
   return Test(ent1);
}

int SimSurfaceFilter::Compare(VuEntity *ent1, VuEntity *ent2)
{
   return (SimCompare (ent1, ent2));
}

VuFilter* SimSurfaceFilter::Copy (void)
{
   return new SimSurfaceFilter(domain);
}
*/

// ==============================================
// SimAirfieldFilter
// ==============================================

SimAirfieldFilter::SimAirfieldFilter(void)
{
}

SimAirfieldFilter::~SimAirfieldFilter(void)
{
}

VU_BOOL SimAirfieldFilter::Test (VuEntity* e)
{
	if (!(e->EntityType())->classInfo_[VU_DOMAIN] || (e->EntityType())->classInfo_[VU_CLASS] != CLASS_OBJECTIVE)
		return FALSE;

	if ( (e->EntityType())->classInfo_[VU_TYPE] != TYPE_AIRBASE)
		return FALSE;

	return TRUE;
}

VU_BOOL SimAirfieldFilter::RemoveTest (VuEntity* ent1)
{
   return Test(ent1);
}

int SimAirfieldFilter::Compare(VuEntity *ent1, VuEntity *ent2)
{
   return (SimCompare (ent1, ent2));
}

VuFilter* SimAirfieldFilter::Copy (void)
{
   return new SimAirfieldFilter;
}



// ==============================================
// SimDynamicTacanFilter
// ==============================================

SimDynamicTacanFilter::SimDynamicTacanFilter(void)
{
}

SimDynamicTacanFilter::~SimDynamicTacanFilter(void)
{
}

VU_BOOL SimDynamicTacanFilter::Test (VuEntity* e)
{
	VU_BOOL returnVal = FALSE;
	VuEntityType* type = e->EntityType();

	if(type->classInfo_[VU_DOMAIN] == DOMAIN_AIR && 
		type->classInfo_[VU_CLASS] == CLASS_UNIT && 
		type->classInfo_[VU_TYPE] == TYPE_FLIGHT &&
		type->classInfo_[VU_STYPE] == STYPE_UNIT_TANKER) 
	{
		returnVal = TRUE;
	}

	return returnVal;
}

VU_BOOL SimDynamicTacanFilter::RemoveTest (VuEntity* e)
{
	VU_BOOL returnVal = FALSE;
	VuEntityType* type = e->EntityType();

	if(type->classInfo_[VU_DOMAIN] == DOMAIN_AIR && 
		type->classInfo_[VU_CLASS] == CLASS_UNIT && 
		type->classInfo_[VU_TYPE] == TYPE_FLIGHT &&
		type->classInfo_[VU_STYPE] == STYPE_UNIT_TANKER) 
	{
		returnVal = TRUE;
	}

	return returnVal;
}

int SimDynamicTacanFilter::Compare(VuEntity *ent1, VuEntity *ent2)
{
   return (SimCompare (ent1, ent2));
}

VuFilter* SimDynamicTacanFilter::Copy (void)
{
   return new SimDynamicTacanFilter;
}



/*
SimPersistantFilter::SimPersistantFilter(void)
{
}

SimPersistantFilter::~SimPersistantFilter(void)
{
}

VU_BOOL SimPersistantFilter::Test (VuEntity* ent1)
{
VU_BOOL retval = FALSE;

   if (ent1->Type() > VU_LAST_ENTITY_TYPE)
   {
   if (((FalconEntity*)ent1)->IsSim() &&
       ((FalconEntity*)ent1)->IsPersistant())
      {
         retval = TRUE;
      }
   }

   return (retval);
}

int SimPersistantFilter::Compare(VuEntity *ent1, VuEntity *ent2)
{
int retval = 0;
double t1, t2;

   t1 = ((SimPersistantClass*)ent1)->RemovalTime();
   t2 = ((SimPersistantClass*)ent2)->RemovalTime();

   if (t1 != t2)
   {
      retval = (t2 > t1 ? 1 : -1);
   }

   return (retval);
}

VuFilter* SimPersistantFilter::Copy (void)
{
   return new SimPersistantFilter;
}
*/
