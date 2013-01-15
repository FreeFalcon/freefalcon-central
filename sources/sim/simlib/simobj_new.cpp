#include "stdhdr.h"
#include "f4vu.h"
#include "simobj.h"
#include "ClassTbl.h"
#include "camp2sim.h"
#include "otwdrive.h"
#include "aircrft.h"
#include "missile.h"
#include "helo.h"
#include "digi.h"
#include "ground.h"
#include "initdata.h"
#include "waypoint.h"
#include "simdrive.h"
#include "simfeat.h"
#include "f4error.h"
#include "simfiltr.h"
#include "entity.h"
#include "FalcSess.h"
#include "PlayerOp.h"
#include "acmi\src\include\acmirec.h"
#include "CampBase.h"
#include "Pilot.h"
#include "object.h"
#include "radar.h"
#include "airunit.h"

#include "RedProfiler.h"

/* S.G. NEED TO KEEP RWR CONTACTS IN TARGET LIST */#include "vehrwr.h"

ACMIFeaturePositionRecord featPos;

static SimBaseClass* AddFeatureToSim (SimInitDataClass *initData);
static SimBaseClass* AddVehicleToSim (SimInitDataClass *initData, int motionType);
static int CheckForConcern (FalconEntity* curUpdate, SimMoverClass* self);
void CalcTransformMatrix (SimBaseClass* theObject);


DWORD	SimObjects=0;

#ifdef DEBUG
int ObjectNodes = 0;
int ObjectReferences = 0;

// use this for debugging only
typedef struct
{
	char *from;
	FalconEntity *owner;
	SimObjectType *obj;
} DBG_ENTRIES;

DBG_ENTRIES gDbgEntries[ 2000 ];
int HighestUsed = 0;
#endif

#ifdef USE_SH_POOLS
MEM_POOL	SimObjectType::pool;
MEM_POOL	SimObjectLocalData::pool;
#endif

#ifdef DEBUG
SimObjectType::SimObjectType(char *from, FalconEntity *owner, FalconEntity* baseObj)
#else
SimObjectType::SimObjectType(FalconEntity* baseObj)
#endif
{

   ShiAssert ( baseObj );

   SimObjects++;
   REPORT_VALUE("SIM OBJECTS",SimObjects);


   baseData = baseObj;
   localData = new SimObjectLocalData;
/* 2000-11-17 REMOVED BY S.G. INSTEAD OF JUST CLEARING HALF THE STUFF THROUGH A LOOP, I'LL 'memset' THE WHOLE STRUCTURE. I NEED TO DO THIS BECAUSE localData NEEDS TO BE CLEARED WHEN INITIALIZED WITH MY PATCHES
   for (i=0; i<NUM_RADAR_HISTORY; i++)
   {
	   localData->rdrSy[i] = 0;
	   localData->rdrX[i] = 0.0F;
	   localData->rdrY[i] = 0.0F;
	   localData->rdrHd[i] = 0.0F;
   }
   for (i=0; i<SensorClass::NumSensorTypes; i++)
	   localData->sensorLoopCount[i] = 0;
*/ memset(localData, 0, sizeof(SimObjectLocalData));

   next = NULL;
   prev = NULL;
   refCount = 0;

   

#ifdef SIMOBJ_REF_COUNT_DEBUG
	refOps = NULL;
#endif
#ifdef DEBUG
	int i; // JB 010118 variable only referenced here

	for ( i = 0; i < 2000; i++ )
	{
		if ( gDbgEntries[i].obj == NULL )
		{
			dbgIndex = i;
			gDbgEntries[i].obj = this;
			gDbgEntries[i].owner = owner;
			gDbgEntries[i].from = from;
			break;
		}
	}

	if ( i == 2000 )
		dbgIndex = -1;
	else if ( i > HighestUsed )
		HighestUsed = i;

	ObjectNodes++;
#endif
}


SimObjectType::~SimObjectType()
{
	ShiAssert( refCount == 0 );
	ShiAssert( !BaseData() );
	SimObjects--;
   REPORT_VALUE("SIM OBJECTS",SimObjects);
	if (localData)
		delete localData;

#ifdef DEBUG
	if ( dbgIndex >= 0 )
	{
		gDbgEntries[dbgIndex].obj = NULL;
		gDbgEntries[dbgIndex].owner = NULL;
		gDbgEntries[dbgIndex].from = NULL;
	}
	ObjectNodes--;
#endif
}


#ifdef SIMOBJ_REF_COUNT_DEBUG
void SimObjectType::Reference( int line, char *file )
#else
void SimObjectType::Reference(void)
#endif
{
	ShiAssert( refCount < 100 );	// Arbitrary limit, but this should be reasonable
	ShiAssert( refCount >= 0 );
	ShiAssert( refCount != 0xDDDDDDDD );
	ShiAssert( BaseData() );

	if (!refCount)
		{
		// Reference the entity on our first reference
		VuReferenceEntity( BaseData() );
		}

	refCount++;

#ifdef SIMOBJ_REF_COUNT_DEBUG
	// Keep a complete history of ref/releases for this object
	// SCR 10/12/98
	struct DebugRecord	*saveOps = new struct DebugRecord;
	saveOps->line	= line;
	strcpy( saveOps->file, file );
	saveOps->refInc	= +1;
	saveOps->prev	= refOps;
	refOps			= saveOps;		// *** Danger Will Robinson! ***
#endif

#ifdef DEBUG
	ObjectReferences++;
#endif
}


#ifdef SIMOBJ_REF_COUNT_DEBUG
void SimObjectType::Release( int line, char *file )
#else
void SimObjectType::Release(void)
#endif
{
#ifdef DEBUG
	ObjectReferences--;
#endif

	ShiAssert( refCount <= 100 );	// Arbitrary limit, but this should be reasonable
	ShiAssert( refCount > 0 );
	ShiAssert( refCount != 0xDDDDDDDD );
	ShiAssert( BaseData() );

	refCount--;

#ifdef SIMOBJ_REF_COUNT_DEBUG
	struct DebugRecord	*saveOps = new struct DebugRecord;
	saveOps->line	= line;
	strcpy( saveOps->file, file );
	saveOps->refInc	= -1;
	saveOps->prev	= refOps;
#endif

	if (!refCount) 
	{
//LRKLUDGE to keep testing going
//		ShiAssert ( !next );
		ShiAssert ( !prev );
		VuDeReferenceEntity( BaseData() );
		baseData = NULL;
		delete this;
	}

#ifdef SIMOBJ_REF_COUNT_DEBUG
	// THIS IS NASTY (and technically illegal) since we're writing to memory
	// which may already be freed, but it'll help with debugging
	// SCR 10/12/98
	refOps			= saveOps;		// *** Danger Will Robinson! ***
#endif
}


#ifdef DEBUG
SimObjectType* SimObjectType::Copy( char *from, FalconEntity *owner)
#else
SimObjectType* SimObjectType::Copy(void)
#endif
{
#ifdef DEBUG
	SimObjectType *theCopy = new SimObjectType(from, owner, baseData);
#else
	SimObjectType *theCopy = new SimObjectType(baseData);
#endif
	*theCopy->localData = *localData;
	return theCopy;
}


BOOL SimObjectType::IsReferenced(void)
{
	/*
	if (refCount)
		return TRUE;
	return FALSE;
	*/
	return refCount;
}


SimBaseClass* AddObjectToSim (SimInitDataClass *initData, int motionType)
{
SimBaseClass* retval, *leadObject;

	ShiAssert (motionType != MOTION_OWNSHIP);

   // LRKLUDGE
   if (initData->flags < 0)
      initData->flags += 65536;

   if (initData->campObj)
   {
      retval = AddFeatureToSim(initData);
      if (gACMIRec.IsRecording() && retval )
      {
			featPos.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
		   	featPos.data.type = retval->Type();
		   	featPos.data.uniqueID = (retval->Id());//.num_;
		   	featPos.data.x = retval->XPos();
		   	featPos.data.y = retval->YPos();
		   	featPos.data.z = retval->ZPos();
		   	featPos.data.roll = retval->Roll();
		   	featPos.data.pitch = retval->Pitch();
		   	featPos.data.yaw = retval->Yaw();
		   	featPos.data.slot = retval->GetSlot();
		   	featPos.data.specialFlags = initData->specialFlags;
			leadObject = initData->campObj->GetComponentLead();
			if ( leadObject && leadObject->Id().num_ != retval->Id().num_ )
				featPos.data.leadUniqueID = (leadObject->Id());//.num_;
			else
				featPos.data.leadUniqueID = -1;
			gACMIRec.FeaturePositionRecord( &featPos );
      }
   }
   else if (initData->campUnit){
      retval = AddVehicleToSim(initData, motionType);
// MonoPrint("x = %f, y = %f, z = %f\n", initData->x, initData->y, initData->z);
	}
   else
      retval = NULL;

   if (retval)
	   {
	   switch (initData->createType)
		   {
		   case SimInitDataClass::CampaignVehicle:
		   case SimInitDataClass::CampaignFeature:
			   retval->SetTypeFlag(FalconEntity::FalconSimEntity);
			   break;
			   
		   default:
			   ShiWarning("Unknown sim object initiater.\n");
		   }
	   
	   if (initData->createFlags & SIDC_REMOTE_OWNER)
		   retval->ChangeOwner(initData->owner->Id());
	   
	   // Inherit certain attributes from campaign parent
// HACK HACK HACK HACK HACK!!!
	   if (FalconLocalGame->GetGameType() == game_Dogfight && initData->campUnit)
//	   if ((initData->campUnit && initData->campUnit->IsSetFalcFlag(FEC_REGENERATING)) ||
//		   (initData->campObj && initData->campObj->IsSetFalcFlag(FEC_REGENERATING)))
// END HACK
		   {
		   retval->SetFalcFlag(FEC_REGENERATING);
		   retval->reinitData = new SimInitDataClass;
		   *retval->reinitData = *initData;
		   }
	   if (initData->playerSlot != NO_PILOT && initData->campUnit && initData->campUnit->IsSetFalcFlag(FEC_PLAYERONLY))
		   retval->SetFalcFlag(FEC_PLAYERONLY);
	   if (initData->playerSlot != NO_PILOT && initData->campUnit && initData->campUnit->IsSetFalcFlag(FEC_HOLDSHORT))
		   retval->SetFalcFlag(FEC_HOLDSHORT);

	   if (initData->createFlags & SIDC_SILENT_INSERT)
		   vuDatabase->SilentInsert (retval);
	   else if (!(initData->createFlags & SIDC_NO_INSERT))
		   vuDatabase->Insert (retval);
	   
	   retval->SetTransmissionTime(vuxRealTime +
		   (unsigned long)((float)rand()/RAND_MAX * retval->UpdateRate()));
	   
	   }
   return (retval);
}

SimBaseClass* AddVehicleToSim (SimInitDataClass *initData, int motionType)
{
SimBaseClass* theVehicle = NULL;
Falcon4EntityClassType* classPtr = &Falcon4ClassTable[initData->descriptionIndex - VU_LAST_ENTITY_TYPE];

   if (classPtr->vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_LAND)
   {
      motionType = MOTION_GND_AI;
      theVehicle = new GroundClass (initData->descriptionIndex);
   }
   else if (classPtr->vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_AIR)
   {
      if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_HELICOPTER)
      {
         motionType = MOTION_HELO_AI;
         theVehicle = new HelicopterClass (initData->descriptionIndex);
      }
      else
      {
         motionType = MOTION_AIR_AI;
		 //aircraft are assumed to be digital until made into player vehicles
         //theVehicle = new AircraftClass(FALSE, initData->descriptionIndex);
		 theVehicle = new AircraftClass(TRUE, initData->descriptionIndex);
      }
   }
   else if (classPtr->vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_SEA)
   {
      motionType = MOTION_GND_AI;
      theVehicle = new GroundClass (initData->descriptionIndex);
   }

   F4Assert(theVehicle);

   theVehicle->SetFlag(motionType);
   theVehicle->Init(initData);

   return (theVehicle);
}


SimBaseClass* AddFeatureToSim (SimInitDataClass *initData)
{
SimFeatureClass* theFeature;

   /*------------------------*/
   /* Add it to the database */
   /*------------------------*/
   theFeature = new SimFeatureClass (initData->descriptionIndex);
   CalcTransformMatrix (theFeature);
   theFeature->Init(initData);

   return (theFeature);
}

/*=================================================================
/* Update targe list is a little out of place here, but here goes..
/*=================================================================*/

SimObjectType* UpdateTargetList (SimObjectType* inUseList, SimMoverClass* self, FalconPrivateOrderedList* thisObjectList)
{
VuListIterator updateWalker (thisObjectList);
SimBaseClass* curUpdate;
SimObjectType* curInUse;
SimObjectType* tmpInUse;
SimObjectType* lastInUse = NULL;
SimObjectType* headOfNewList = inUseList;

   // edg: If we're running in Instant action, doesn't anything other than
   // the player just need to target the player?  I think so ....

/* // JB 010625 Commented out this section. Aim-120 bore mode didn't work in Instant Action.
   if ( SimDriver.RunningInstantAction() && !self->IsSetFlag( MOTION_OWNSHIP ) )
   {
	   // already got player?
	   // we need to make sure the player still exists though otherwise CalcRelGeom is unhappy :(
	   if ( inUseList != NULL || !SimDriver.playerEntity)
	   		return inUseList;

	   // create player target and return it
#ifdef DEBUG
       tmpInUse = new SimObjectType(OBJ_TAG, self, SimDriver.playerEntity);
#else
       tmpInUse = new SimObjectType(SimDriver.playerEntity);
#endif
       tmpInUse->Reference( SIM_OBJ_REF_ARGS );
       tmpInUse->prev = NULL;
       tmpInUse->next = NULL;
       CalcRelGeom (self, tmpInUse, NULL, 1.0F);

	   return tmpInUse;
   }
*/

#ifdef DEBUG
	// Debug testing: SCR 5/25/98
   /*
	curInUse = inUseList;
	while (curInUse) {
		if (curInUse->next) {
			ShiAssert( SimCompare( curInUse->BaseData(), curInUse->next->BaseData() ) == 1 );
		}
		curInUse = curInUse->next;
	}

	SimBaseClass *previous = (SimBaseClass*)updateWalker.GetFirst();
	curUpdate = (SimBaseClass*)updateWalker.GetNext();
	while (curUpdate) {
		ShiAssert( SimCompare( previous, curUpdate ) == 1 );
		previous = curUpdate;
		curUpdate = (SimBaseClass*)updateWalker.GetNext();
	}
	*/
#endif



   curInUse = inUseList;
   curUpdate = (SimBaseClass*)updateWalker.GetFirst();
#ifndef DAVE_DBG
   //MonoPrint ("---- Update Target List ----\n");
#endif
   while (curUpdate)
   {
#ifndef DAVE_DBG
	   //MonoPrint ("Target %08x\n", curUpdate);
#endif

      if (curUpdate == self)
      {
         curUpdate = (SimBaseClass*)updateWalker.GetNext();
         continue;
      }

      if (curInUse)
      {
            switch (SimCompare (curInUse->BaseData(), curUpdate))
            {
               case 0:	// curUpdate == curInUse -- Means the current entry is still active
					if (!curUpdate->IsExploding() && CheckForConcern (curUpdate, self))
					{
					   lastInUse = curInUse;
					   curInUse = curInUse->next;
					}
					else
					{
					   //Remove from In Use List
					   if (curInUse->prev == NULL)
						  headOfNewList = curInUse->next;
					   else
						  curInUse->prev->next = curInUse->next;
					   if (curInUse->next)
						  curInUse->next->prev = curInUse->prev;

					   //edg: Fix bAAAAADDDDD bug
					   lastInUse = curInUse->prev;

					   tmpInUse = curInUse;
					   curInUse = curInUse->next;
					   // This node will either die or be pointed to by radar->lockedTarget;
					   tmpInUse->prev = NULL;
					   tmpInUse->next = NULL;
					   tmpInUse->Release( SIM_OBJ_REF_ARGS );
					   tmpInUse = NULL;
					}
					curUpdate = (SimBaseClass*)updateWalker.GetNext();
				break;

               case 1: // curUpdate > curInUse -- Means the current entry should be removed
                  //Remove from In Use List
                  if (curInUse->prev == NULL)
                     headOfNewList = curInUse->next;
                  else
                     curInUse->prev->next = curInUse->next;
                  if (curInUse->next)
                     curInUse->next->prev = curInUse->prev;

				  // edg: fix baaaad bug
				  lastInUse = curInUse->prev;

                  tmpInUse = curInUse;
                  curInUse = curInUse->next;
                  // This node will either die or be pointed to by radar->lockedTarget;
                  tmpInUse->prev = NULL;
                  tmpInUse->next = NULL;
                  tmpInUse->Release( SIM_OBJ_REF_ARGS );
				  tmpInUse = NULL;
               break;

               case -1: // curUpdate < curInUse -- Insert into the list
                  if (!curUpdate->IsDead() && CheckForConcern (curUpdate, self))
                  {
                     // Add before curInUse
					 #ifdef DEBUG
					 tmpInUse = new SimObjectType(OBJ_TAG, self, curUpdate);
					 #else
					 tmpInUse = new SimObjectType(curUpdate);
					 #endif
                     tmpInUse->Reference( SIM_OBJ_REF_ARGS );
                     memset (tmpInUse->localData->sensorLoopCount, 0, SensorClass::NumSensorTypes*sizeof(int));
                     tmpInUse->localData->range = 0.0F;
                     tmpInUse->localData->ataFrom = 180.0F * DTR;
                     tmpInUse->localData->aspect = 0.0F;
                     CalcRelGeom (self, tmpInUse, NULL, 1.0F);
                     tmpInUse->next = curInUse;
                     tmpInUse->prev = curInUse->prev;
                     if (curInUse->prev == NULL)
                     {
                        headOfNewList = tmpInUse;
                     }
                     else
                     {
                        curInUse->prev->next = tmpInUse;
                     }
                     curInUse->prev = tmpInUse;
                     lastInUse = curInUse;
                  }
                  curUpdate = (SimBaseClass*)updateWalker.GetNext();
               break;
            }
      } // inUse
      else
      {
         // Check and add to the end of the list if needed
         //if (!curUpdate->IsDead() && CheckForConcern (curUpdate, self))
				 if (!F4IsBadReadPtr(curUpdate, sizeof(SimBaseClass)) && !curUpdate->IsDead() && CheckForConcern (curUpdate, self)) // JB 010405 CTD
         {
            // Add after lastInUse
			#ifdef DEBUG
			tmpInUse = new SimObjectType(OBJ_TAG, self, curUpdate);
			#else
			tmpInUse = new SimObjectType(curUpdate);
			#endif
            tmpInUse->Reference( SIM_OBJ_REF_ARGS );
            tmpInUse->localData->range = 0.0F;
            tmpInUse->localData->ataFrom = 180.0F * DTR;
            memset (tmpInUse->localData->sensorLoopCount, 0, SensorClass::NumSensorTypes*sizeof(int));
            tmpInUse->localData->aspect = 0.0F;
            CalcRelGeom (self, tmpInUse, NULL, 1.0F);
            tmpInUse->prev = lastInUse;
            tmpInUse->next = NULL;

			   if (lastInUse)
            {
				   lastInUse->next = tmpInUse;
			   }
            else
            {
				   headOfNewList = tmpInUse;
			   }
			   lastInUse = tmpInUse;
         }
         curUpdate = (SimBaseClass*)updateWalker.GetNext();
      } // No Objects
   } // while curUpdate

   // Ensure the last valid target has a NULL for its "next" pointer
   if (curInUse && curInUse->prev)
   {
      curInUse->prev->next = NULL;
   }

   // Read as "If remainder of target list == entire new target list"
   if (curInUse == headOfNewList)
      headOfNewList = NULL;

   // Delete the rest of the target list (which doesn't have matches in the global list)
   while (curInUse)
   {
      tmpInUse = curInUse;
      curInUse = curInUse->next;
	  // This node will either be deleted or is locked by some radar
	  tmpInUse->prev = NULL;
	  tmpInUse->next = NULL;
      tmpInUse->Release( SIM_OBJ_REF_ARGS );
	  tmpInUse = NULL;
   }
   return (headOfNewList);
}


// COBRA - RED - Function created to release Allocated Target Lists * USE WITH CARE *
void ReleaseTargetList (SimObjectType* InUseList)
{
	SimObjectType* Next;
	while(InUseList){
		Next=InUseList->next;
		InUseList->Release( SIM_OBJ_REF_ARGS );
		InUseList=Next;
	}
}



int CheckForConcern (FalconEntity* curUpdate, SimMoverClass* self)
{
float rangeSqr, airRange, gndRange;
int retval = FALSE;

	if (curUpdate == self)
    {
		return FALSE;
	}
	// Make sure we keep anything currently locked up in the target list
   if (self->targetPtr && self->targetPtr->BaseData() == curUpdate)
   {
      return TRUE;
   }

   // SCR:  Lets not bother keeping bombs (and chaff and flares) in the target lists
   // NOTE:  The missile will eventually have their own target list maintenance polices
   // that don't go through this routine.  For now they don't have a target list at all.
   if (curUpdate->IsBomb())
   {
//ME123 CHAFF FLARE BOMBS SHOULD NOT BE REJECTED IMO	   return FALSE;
   }

   // edg: I'm going to try this out -- don't put anything into target
   // lists that are hidden (should only affect helos and grnd vehicles)
   if ( curUpdate->IsSim() && ((SimBaseClass*)curUpdate)->IsSetLocalFlag( IS_HIDDEN ) )
   		return FALSE;

   // KCK: Don't target ejected entities
   // edg: We HAVE to put ejected pilots into the target list so they
   // can be shot and collided with!  Missile logic may have to be changed...
   // to reduce possible crashes and other anomalies I'm doing this only
   // for player vehicle
   if ( curUpdate->EntityType()->classInfo_[VU_TYPE] == TYPE_EJECT && self != SimDriver.playerEntity )
	   return FALSE;

   if (self->IsSetFlag(MOTION_OWNSHIP))
   {
	   airRange = 50.0F * NM_TO_FT * 50.0F * NM_TO_FT;
	   gndRange = 20.0F;

	   if(self->IsAirplane())
	   {
		   RadarClass *radar = ((RadarClass*)FindSensor (self, SensorClass::Radar));
		   if(radar)
		   {
			   float tmpRng = radar->GetRange();
			   if(radar->IsAG())
			   {
				   gndRange = max(20.0F * NM_TO_FT * 20.0F * NM_TO_FT, tmpRng * NM_TO_FT * tmpRng * NM_TO_FT);
				   airRange = 20.0F * NM_TO_FT * 20.0F * NM_TO_FT;
			   }
			   else
			   {
				   gndRange = 20.0F * NM_TO_FT * 20.0F * NM_TO_FT;
				   airRange = max(20.0F * NM_TO_FT * 20.0F * NM_TO_FT, tmpRng * NM_TO_FT * tmpRng * NM_TO_FT);
			   }
		   }
// 2001-03-02 ADDED BY S.G. SO RWR CONTACTS ARE MAINTAINED IN THE *PLAYERS* TARGET LIST, OTHERWISE, RWR WILL DO FUNNY STUFF WITH THESE
		   VehRwrClass *rwr = ((VehRwrClass *)FindSensor (self, SensorClass::RWR));
		   if(rwr) {
			   if (rwr->IsTracked(curUpdate))
				   return TRUE;
		   }
// END OF ADDED SECTION
	   }
   }
   else if (curUpdate->IsMissile())
   {
      return FALSE;
   }
   else if (self->IsAirplane() && (self->OnGround() /*|| curUpdate->IsHelicopter()*/)) // 2002-03-05 MODIFIED BY S.G. Choppers are fare game now under some condition so don't screen them out
   {
      return FALSE;
   }
   else if (self->IsHelicopter() && curUpdate->IsAirplane())
   {
      return FALSE;
   }
   else if (self->IsAirplane() && (curUpdate->IsAirplane() || curUpdate->IsFlight() || curUpdate->IsHelicopter())) // 2002-03-05 MODIFIED BY S.G. Choppers are fare game now under some condition so don't screen them out
   {
	   if (self->GetTeam() == curUpdate->GetTeam())
         airRange = 100.0F * 100.0F;
       else if( curUpdate->IsSim() &&
				( ((AircraftClass*)curUpdate)->GetSType() == STYPE_AIR_FIGHTER			|| 
				  ((AircraftClass*)curUpdate)->GetSType() == STYPE_AIR_FIGHTER_BOMBER	||
// 2002-03-05 MODIFIED BY S.G. Duh, it's missionClass, not missionType that holds AAMission
//				  ((AircraftClass*)self)->DBrain()->MissionType() == DigitalBrain::AAMission) )
				  ((AircraftClass*)self)->DBrain()->MissionClass() == DigitalBrain::AAMission) )	   {
			 airRange = 20.0F * NM_TO_FT * 20.0F * NM_TO_FT;
	   }
	   else if( curUpdate->IsCampaign() &&
				( ((AirUnitClass*)curUpdate)->GetSType() == STYPE_UNIT_FIGHTER			|| 
				  ((AirUnitClass*)curUpdate)->GetSType() == STYPE_UNIT_FIGHTER_BOMBER	||
// 2002-03-05 MODIFIED BY S.G. Duh, it's missionClass, not missionType that holds AAMission
//				  ((AircraftClass*)self)->DBrain()->MissionType() == DigitalBrain::AAMission) )
				  ((AircraftClass*)self)->DBrain()->MissionClass() == DigitalBrain::AAMission) )
	   {
			 airRange = 20.0F * NM_TO_FT * 20.0F * NM_TO_FT;
	   }
	   else
	   {
			airRange = 5.0F * NM_TO_FT * 5.0F * NM_TO_FT;
	   }
	   gndRange = 0.0F;
   }
   else
   {
      if (self->GetTeam() == curUpdate->GetTeam())
         airRange = 100.0F * 100.0F;
      else if (self->IsGroundVehicle() && ((GroundClass*)self)->isAirCapable)
         airRange = 20.0F * NM_TO_FT * 20.0F * NM_TO_FT;
	  else 
// MODIFIED BY S.G. FOR THE MISSILE TO FIND A LOCK WITH THIS SET TO 10 NM INSTEAD OF 8 NM
//		 airRange = 8.0F * NM_TO_FT * 8.0F * NM_TO_FT;
		 airRange = 10.0F * NM_TO_FT * 10.0F * NM_TO_FT;
// END OF MODIFIED SECTION
      // LRKLUDGE ?
	  // edg note: Looks like leon only had ground units putting other
	  // ground untis into their target list and at a maximum of 1 NM.
	  // I extended this for to include helicopters and made the range
	  // go out to 5NM.  Also: Aren't aircraft AI going to need to detect
	  // ground units?!
      if (self->IsGroundVehicle() || self->IsHelicopter())
         gndRange = 15.0f * NM_TO_FT * 15.0f * NM_TO_FT;
      else
         gndRange = 0.0F;
   }

   
  rangeSqr = (curUpdate->XPos()-self->XPos())*(curUpdate->XPos()-self->XPos()) +
			  (curUpdate->YPos()-self->YPos())*(curUpdate->YPos()-self->YPos());

  if (curUpdate->OnGround())
  {
	 if (rangeSqr < gndRange)
		retval = TRUE;
  }
  else if (rangeSqr < airRange)
  {
	 retval = TRUE;
  }
   return (retval);
}

float CalcKIAS(float vt, float alt)
{
float ttheta, rsigma;
float mach, vcas, pa;
float qpasl1, oper, qc;

   /*-----------------------------------------------*/
   /* calculate temperature ratio and density ratio */
   /*-----------------------------------------------*/
   if (alt <= 36089.0F)
   {
      ttheta = 1.0F - 0.000006875F * alt;
      rsigma = (float)pow (ttheta, 4.256F);
   }
   else
   {
      ttheta = 0.7519F;
      rsigma = 0.2971F * (float)pow(2.718, 0.00004806 * (36089.0 - alt));
   }

   mach = vt / ((float)sqrt(ttheta) * AASL);
   pa   = ttheta * rsigma * PASL;

   /*-------------------------------*/
   /* calculate calibrated airspeed */
   /*-------------------------------*/
   if (mach <= 1.0F)
      qc = ((float)pow((1.0F + 0.2F*mach*mach), 3.5F) - 1.0F)*pa;
   else
      qc = ((166.9F*mach*mach)/(float)(pow((7.0F - 1.0F/(mach*mach)), 2.5F)) - 1.0F)*pa;

   qpasl1 = qc/PASL + 1.0F;
   vcas = 1479.12F*(float)sqrt(pow(qpasl1, 0.285714F) - 1.0F);

   if (qc > 1889.64F)
   {
      oper = qpasl1 * (float)pow((7.0F - AASLK*AASLK/(vcas*vcas)), 2.5F);
      if (oper < 0.0F) oper = 0.1F;
      vcas = 51.1987F*(float)sqrt(oper);
   }

   return (vcas);
}


#ifdef DEBUG
void SimObjCheckOwnership( FalconEntity *owner )
{
	int i;
	int j;

	for ( i = 0; i <= HighestUsed; i++ )
	{
		if ( gDbgEntries[i].owner == owner )
		{
			if ( gDbgEntries[i].obj->IsReferenced() )
			{
// SCR 11/8/98: Often this indicates a problem, but is IS technically legal, so lets shut up about it for now...
				//ShiAssert( !"Why do we still have a SimObject referenced?" );
				j = 0;
			}
		}
	}
}
#endif
