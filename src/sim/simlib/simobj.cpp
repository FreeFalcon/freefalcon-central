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
#include "acmi/src/include/acmirec.h"
#include "CampBase.h"
#include "Pilot.h"
#include "object.h"
#include "radar.h"
#include "airunit.h"

/* S.G. NEED TO KEEP RWR CONTACTS IN TARGET LIST */
#include "vehrwr.h"

ACMIFeaturePositionRecord featPos;

static SimBaseClass* AddFeatureToSim(SimInitDataClass *initData);
static SimBaseClass* AddVehicleToSim(SimInitDataClass *initData, int motionType);
static int CheckForConcern(FalconEntity* curUpdate, SimMoverClass* self);
void CalcTransformMatrix(SimBaseClass* theObject);

#ifdef USE_SH_POOLS
MEM_POOL SimObjectType::pool;
MEM_POOL SimObjectLocalData::pool;
#endif

//sfr: test
#define COUNT_SIMOBJECTTYPE 0
#if COUNT_SIMOBJECTTYPE
DWORD SimObjects = 0;
F4CSECTIONHANDLE *som = F4CreateCriticalSection("sim object count");
#endif

SimObjectType::SimObjectType(FalconEntity* baseObj) :
    baseData(baseObj), mutex(F4CreateCriticalSection("simobj")), refCount(0)
{
#if COUNT_SIMOBJECTTYPE
    {
        F4ScopeLock l(som);
        ++SimObjects;
    }
#endif
    //REPORT_VALUE("SIM OBJECTS",SimObjects);
    localData = new SimObjectLocalData;
    memset(localData, 0, sizeof(SimObjectLocalData));
    next = NULL;
    prev = NULL;
}


SimObjectType::~SimObjectType()
{
#if COUNT_SIMOBJECTTYPE
    {
        F4ScopeLock l(som);
        --SimObjects;
    }
#endif
    //REPORT_VALUE("SIM OBJECTS",SimObjects);
    baseData.reset();
    delete localData;
    F4DestroyCriticalSection(mutex);
}


void SimObjectType::Reference(void)
{
    F4ScopeLock l(mutex);
    ++refCount;
}


void SimObjectType::Release(void)
{
    {
        F4ScopeLock l(mutex);
        --refCount;
    }

    if (refCount == 0)
    {
        delete this;
    }
}

SimObjectType* SimObjectType::Copy(void)
{
    SimObjectType *theCopy = new SimObjectType(baseData.get());
    *theCopy->localData = *localData;
    // sfr: temp test for leak
    //theCopy->next = NULL;
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


SimBaseClass* AddObjectToSim(SimInitDataClass *initData, int motionType)
{
    SimBaseClass* retval, *leadObject;

    // LRKLUDGE
    if (initData->flags < 0)
    {
        initData->flags += 65536;
    }

    if (initData->campBase->IsObjective())
    {
        retval = AddFeatureToSim(initData);

        if (gACMIRec.IsRecording() and retval)
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
            leadObject = initData->campBase->GetComponentLead();

            if (leadObject and leadObject->Id().num_ not_eq retval->Id().num_)
            {
                featPos.data.leadUniqueID = (leadObject->Id());//.num_;
            }
            else
            {
                featPos.data.leadUniqueID = -1;
            }

            gACMIRec.FeaturePositionRecord(&featPos);
        }
    }
    else if (initData->campBase->IsUnit())
    {
        retval = AddVehicleToSim(initData, motionType);
    }
    else
    {
        retval = NULL;
    }

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

        if (initData->createFlags bitand SIDC_REMOTE_OWNER)
        {
            retval->ChangeOwner(initData->owner->Id());
        }

        // Inherit certain attributes from campaign parent
        // HACK HACK HACK HACK HACK
        if (FalconLocalGame->GetGameType() == game_Dogfight and initData->campBase->IsUnit())
            //    if ((initData->campUnit and initData->campUnit->IsSetFalcFlag(FEC_REGENERATING)) or
            //    (initData->campObj and initData->campObj->IsSetFalcFlag(FEC_REGENERATING)))
            // END HACK
        {
            retval->SetFalcFlag(FEC_REGENERATING);
            retval->reinitData = new SimInitDataClass;
            *retval->reinitData = *initData;
        }

        if (
            initData->playerSlot not_eq NO_PILOT and 
            initData->campBase->IsUnit() and 
            initData->campBase->IsSetFalcFlag(FEC_PLAYERONLY)
        )
        {
            retval->SetFalcFlag(FEC_PLAYERONLY);
        }

        if (
            initData->playerSlot not_eq NO_PILOT and 
            initData->campBase->IsUnit() and 
            initData->campBase->IsSetFalcFlag(FEC_HOLDSHORT)
        )
        {
            retval->SetFalcFlag(FEC_HOLDSHORT);
        }

        /*
        if (initData->createFlags bitand SIDC_SILENT_INSERT){
         vuDatabase->SilentInsert (retval);
        }
        else if ( not (initData->createFlags bitand SIDC_NO_INSERT)){
         vuDatabase->QuickInsert (retval);
        }*/
        // sfr: flags now take care of sending creation events
        vuDatabase->Insert(retval);

        retval->SetTransmissionTime(
            vuxRealTime + (unsigned long)((float)rand() / RAND_MAX * retval->UpdateRate())
        );
    }

    return (retval);
}


// sfr: temp test
void *debugPtr = NULL;

SimBaseClass* AddVehicleToSim(SimInitDataClass *initData, int motionType)
{
    SimBaseClass* theVehicle = NULL;
    Falcon4EntityClassType* classPtr = &Falcon4ClassTable[initData->descriptionIndex - VU_LAST_ENTITY_TYPE];

    if (classPtr->vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_LAND)
    {
        motionType = MOTION_GND_AI;
        theVehicle = new GroundClass(initData->descriptionIndex);
        // sfr: temp test
        //static bool first = true;
        //if (first){
        // first = false;
        // debugPtr = theVehicle;
        //}
    }
    else if (classPtr->vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_AIR)
    {
        if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_HELICOPTER)
        {
            motionType = MOTION_HELO_AI;
            theVehicle = new HelicopterClass(initData->descriptionIndex);
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
        theVehicle = new GroundClass(initData->descriptionIndex);
    }

    // RV - Biker - Don't create subs
    if (theVehicle)
    {
        theVehicle->SetFlag(motionType);
        theVehicle->Init(initData);

        return (theVehicle);
    }
    else
        return NULL;
}


SimBaseClass* AddFeatureToSim(SimInitDataClass *initData)
{
    SimFeatureClass* theFeature;

    /*------------------------*/
    /* Add it to the database */
    /*------------------------*/
    theFeature = new SimFeatureClass(initData->descriptionIndex);
    CalcTransformMatrix(theFeature);
    theFeature->Init(initData);
    return (theFeature);
}

/*=================================================================
/* Update targe list is a little out of place here, but here goes..
/*=================================================================*/

SimObjectType* UpdateTargetList(SimObjectType* inUseList, SimMoverClass* self, FalconPrivateOrderedList* thisObjectList)
{
    VuListIterator updateWalker(thisObjectList);
    // sfr: how can you be so sure this is a SimBaseClass???
    //SimBaseClass* curUpdate;
    FalconEntity *curUpdate;
    SimObjectType* curInUse;
    SimObjectType* tmpInUse;
    SimObjectType* lastInUse = NULL;
    SimObjectType* headOfNewList = inUseList;

    curInUse = inUseList;
    //curUpdate = (SimBaseClass*)updateWalker.GetFirst();
    curUpdate = (FalconEntity*)updateWalker.GetFirst();

    while (curUpdate)
    {
        if (curUpdate == self)
        {
            //curUpdate = (SimBaseClass*)updateWalker.GetNext();
            curUpdate = (FalconEntity*)updateWalker.GetNext();
            continue;
        }

        if (curInUse)
        {
            switch (SimCompare(curInUse->BaseData(), curUpdate))
            {
                case 0: // curUpdate == curInUse -- Means the current entry is still active
                    if ( not curUpdate->IsExploding() and CheckForConcern(curUpdate, self))
                    {
                        lastInUse = curInUse;
                        curInUse = curInUse->next;
                    }
                    else
                    {
                        //Remove from In Use List
                        if (curInUse->prev == NULL)
                        {
                            headOfNewList = curInUse->next;
                        }
                        else
                        {
                            curInUse->prev->next = curInUse->next;
                        }

                        if (curInUse->next)
                        {
                            curInUse->next->prev = curInUse->prev;
                        }

                        //edg: Fix bAAAAADDDDD bug
                        lastInUse = curInUse->prev;

                        tmpInUse = curInUse;
                        curInUse = curInUse->next;
                        // This node will either die or be pointed to by radar->lockedTarget;
                        tmpInUse->prev = NULL;
                        tmpInUse->next = NULL;
                        tmpInUse->Release();
                        tmpInUse = NULL;
                    }

                    //curUpdate = (SimBaseClass*)updateWalker.GetNext();
                    curUpdate = (FalconEntity*)updateWalker.GetNext();
                    break;

                case 1: // curUpdate > curInUse -- Means the current entry should be removed

                    //Remove from In Use List
                    if (curInUse->prev == NULL)
                    {
                        headOfNewList = curInUse->next;
                    }
                    else
                    {
                        curInUse->prev->next = curInUse->next;
                    }

                    if (curInUse->next)
                    {
                        curInUse->next->prev = curInUse->prev;
                    }

                    // edg: fix baaaad bug
                    lastInUse = curInUse->prev;

                    tmpInUse = curInUse;
                    curInUse = curInUse->next;
                    // This node will either die or be pointed to by radar->lockedTarget;
                    tmpInUse->prev = NULL;
                    tmpInUse->next = NULL;
                    tmpInUse->Release();
                    tmpInUse = NULL;
                    break;

                case -1: // curUpdate < curInUse -- Insert into the list
                    if ( not curUpdate->IsDead() and CheckForConcern(curUpdate, self))
                    {
                        // Add before curInUse
                        tmpInUse = new SimObjectType(curUpdate);
                        tmpInUse->Reference();
                        memset(tmpInUse->localData->sensorLoopCount, 0, SensorClass::NumSensorTypes * sizeof(int));
                        tmpInUse->localData->range = 0.0F;
                        tmpInUse->localData->ataFrom = 180.0F * DTR;
                        tmpInUse->localData->aspect = 0.0F;
                        CalcRelGeom(self, tmpInUse, NULL, 1.0F);
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

                    //curUpdate = (SimBaseClass*)updateWalker.GetNext();
                    curUpdate = (FalconEntity*)updateWalker.GetNext();
                    break;
            }
        } // inUse
        else
        {
            // Check and add to the end of the list if needed
            // sfr: removed JB hack
            if (
                // not F4IsBadReadPtr(curUpdate, sizeof(SimBaseClass)) and 
 not curUpdate->IsDead() and CheckForConcern(curUpdate, self)
            )
            {
                // Add after lastInUse
                tmpInUse = new SimObjectType(curUpdate);
                tmpInUse->Reference();
                tmpInUse->localData->range = 0.0F;
                tmpInUse->localData->ataFrom = 180.0F * DTR;
                memset(tmpInUse->localData->sensorLoopCount, 0, SensorClass::NumSensorTypes * sizeof(int));
                tmpInUse->localData->aspect = 0.0F;
                CalcRelGeom(self, tmpInUse, NULL, 1.0F);
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

            //curUpdate = (SimBaseClass*)updateWalker.GetNext();
            curUpdate = (FalconEntity*)updateWalker.GetNext();
        } // No Objects
    } // while curUpdate

    // Ensure the last valid target has a NULL for its "next" pointer
    if (curInUse and curInUse->prev)
    {
        curInUse->prev->next = NULL;
    }

    // Read as "If remainder of target list == entire new target list"
    if (curInUse == headOfNewList)
    {
        headOfNewList = NULL;
    }

    // Delete the rest of the target list (which doesn't have matches in the global list)
    while (curInUse)
    {
        tmpInUse = curInUse;
        curInUse = curInUse->next;
        // This node will either be deleted or is locked by some radar
        tmpInUse->prev = NULL;
        tmpInUse->next = NULL;
        tmpInUse->Release();
        tmpInUse = NULL;
    }

    return (headOfNewList);
}


// COBRA - RED - Function created to release Allocated Target Lists * USE WITH CARE *
void ReleaseTargetList(SimObjectType* InUseList)
{
    SimObjectType* Next;

    while (InUseList)
    {
        Next = InUseList->next;
        InUseList->Release();
        InUseList = Next;
    }
}



int CheckForConcern(FalconEntity* curUpdate, SimMoverClass* self)
{
    float rangeSqr, airRange, gndRange;
    int retval = FALSE;

    if (curUpdate == self)
    {
        return FALSE;
    }

    // Make sure we keep anything currently locked up in the target list
    if (self->targetPtr and self->targetPtr->BaseData() == curUpdate)
    {
        return TRUE;
    }

    // SCR:  Lets not bother keeping bombs (and chaff and flares) in the target lists
    // NOTE:  The missile will eventually have their own target list maintenance polices
    // that don't go through this routine.  For now they don't have a target list at all.
    //if (curUpdate->IsBomb()){
    //ME123 CHAFF FLARE BOMBS SHOULD NOT BE REJECTED IMO    return FALSE;
    //}

    // edg: I'm going to try this out -- don't put anything into target
    // lists that are hidden (should only affect helos and grnd vehicles)
    if (curUpdate->IsSim() and ((SimBaseClass*)curUpdate)->IsSetLocalFlag(IS_HIDDEN))
    {
        return FALSE;
    }

    // KCK: Don't target ejected entities
    // edg: We HAVE to put ejected pilots into the target list so they
    // can be shot and collided with  Missile logic may have to be changed...
    // to reduce possible crashes and other anomalies I'm doing this only
    // for player vehicle
    if (curUpdate->EntityType()->classInfo_[VU_TYPE] == TYPE_EJECT and self not_eq SimDriver.GetPlayerEntity())
    {
        return FALSE;
    }

    if (self->IsSetFlag(MOTION_OWNSHIP))
    {
        airRange = 50.0F * NM_TO_FT * 50.0F * NM_TO_FT;
        gndRange = 20.0F;

        if (self->IsAirplane())
        {
            RadarClass *radar = ((RadarClass*)FindSensor(self, SensorClass::Radar));

            if (radar)
            {
                float tmpRng = radar->GetRange();

                if (radar->IsAG())
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
            VehRwrClass *rwr = ((VehRwrClass *)FindSensor(self, SensorClass::RWR));

            if (rwr)
            {
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
    else if (self->IsAirplane() and (self->OnGround() /*or curUpdate->IsHelicopter()*/)) // 2002-03-05 MODIFIED BY S.G. Choppers are fare game now under some condition so don't screen them out
    {
        return FALSE;
    }
    else if (self->IsHelicopter() and curUpdate->IsAirplane())
    {
        return FALSE;
    }
    else if (self->IsAirplane() and (curUpdate->IsAirplane() or curUpdate->IsFlight() or curUpdate->IsHelicopter())) // 2002-03-05 MODIFIED BY S.G. Choppers are fare game now under some condition so don't screen them out
    {
        if (self->GetTeam() == curUpdate->GetTeam())
            airRange = 100.0F * 100.0F;
        else if (curUpdate->IsSim() and 
                 (((AircraftClass*)curUpdate)->GetSType() == STYPE_AIR_FIGHTER or
                  ((AircraftClass*)curUpdate)->GetSType() == STYPE_AIR_FIGHTER_BOMBER or
                  // 2002-03-05 MODIFIED BY S.G. Duh, it's missionClass, not missionType that holds AAMission
                  //   ((AircraftClass*)self)->DBrain()->MissionType() == DigitalBrain::AAMission) )
                  ((AircraftClass*)self)->DBrain()->MissionClass() == DigitalBrain::AAMission))
        {
            airRange = 20.0F * NM_TO_FT * 20.0F * NM_TO_FT;
        }
        else if (curUpdate->IsCampaign() and 
                 (((AirUnitClass*)curUpdate)->GetSType() == STYPE_UNIT_FIGHTER or
                  ((AirUnitClass*)curUpdate)->GetSType() == STYPE_UNIT_FIGHTER_BOMBER or
                  // 2002-03-05 MODIFIED BY S.G. Duh, it's missionClass, not missionType that holds AAMission
                  //   ((AircraftClass*)self)->DBrain()->MissionType() == DigitalBrain::AAMission) )
                  ((AircraftClass*)self)->DBrain()->MissionClass() == DigitalBrain::AAMission))
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
        else if (self->IsGroundVehicle() and ((GroundClass*)self)->isAirCapable)
            airRange = 20.0F * NM_TO_FT * 20.0F * NM_TO_FT;
        else
            // MODIFIED BY S.G. FOR THE MISSILE TO FIND A LOCK WITH THIS SET TO 10 NM INSTEAD OF 8 NM
            //  airRange = 8.0F * NM_TO_FT * 8.0F * NM_TO_FT;
            airRange = 10.0F * NM_TO_FT * 10.0F * NM_TO_FT;

        // END OF MODIFIED SECTION
        // LRKLUDGE ?
        // edg note: Looks like leon only had ground units putting other
        // ground untis into their target list and at a maximum of 1 NM.
        // I extended this for to include helicopters and made the range
        // go out to 5NM.  Also: Aren't aircraft AI going to need to detect
        // ground units?
        if (self->IsGroundVehicle() or self->IsHelicopter())
            gndRange = 15.0f * NM_TO_FT * 15.0f * NM_TO_FT;
        else
            gndRange = 0.0F;
    }


    rangeSqr = (curUpdate->XPos() - self->XPos()) * (curUpdate->XPos() - self->XPos()) +
               (curUpdate->YPos() - self->YPos()) * (curUpdate->YPos() - self->YPos());

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
        rsigma = (float)pow(ttheta, 4.256F);
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
        qc = ((float)pow((1.0F + 0.2F * mach * mach), 3.5F) - 1.0F) * pa;
    else
        qc = ((166.9F * mach * mach) / (float)(pow((7.0F - 1.0F / (mach * mach)), 2.5F)) - 1.0F) * pa;

    qpasl1 = qc / PASL + 1.0F;
    vcas = 1479.12F * (float)sqrt(pow(qpasl1, 0.285714F) - 1.0F);

    if (qc > 1889.64F)
    {
        oper = qpasl1 * (float)pow((7.0F - AASLK * AASLK / (vcas * vcas)), 2.5F);

        if (oper < 0.0F) oper = 0.1F;

        vcas = 51.1987F * (float)sqrt(oper);
    }

    return (vcas);
}
