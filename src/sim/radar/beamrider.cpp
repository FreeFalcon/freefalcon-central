#include "stdhdr.h"
#include "falcsess.h"
#include "entity.h"
#include "campbase.h"
#include "simmover.h"
#include "handoff.h"
#include "radar.h"
#include "Object.h"
#include "SimMath.h"
#include "MsgInc/TrackMsg.h"
#include "BeamRider.h"
#include "Battalion.h"
/* S.G. SO ARH DON'T SEND A LAUNCH SIGNAL WHEN COMMAND GUIDED */ #include "Missile.h"
#include "RadarDoppler.h" // 2002-03-13 S.G.


static const float CM_EFFECTIVE_ANGLE = 30.0f * DTR; // If used, should be in class table data...


BeamRiderClass::BeamRiderClass(int, SimMoverClass *body) : SensorClass(body)
{
    sensorType = RadarHoming;
    dataProvided = ExactPosition;
    radarPlatform = NULL;
    lastChaffID = FalconNullId;
    lastTargetLockSend = 0;
    chafftime = SimLibElapsedTime - 5000;
}


BeamRiderClass::~BeamRiderClass(void)
{
    SetGuidancePlatform(NULL);
}


SimObjectType* BeamRiderClass::Exec(SimObjectType*)
{
    SensorClass *radar;

    // Validate our locked target
    CheckLockedTarget();

    // Validate our radar platform
    SetGuidancePlatform(SimCampHandoff(radarPlatform, HANDOFF_RADAR));

    if ( not radarPlatform)
    {
        if (lockedTarget)
            SendTrackMsg(lockedTarget, Track_Unlock);

        ClearSensorTarget();
        return NULL;
    }

    if ( not lockedTarget)  //me123 allow reacusition
    {
        // 2002-03-13 MODIFIED BY S.G. Fair enough but don't assume it's a battalion, planes can fire SARH but they always return FEC_RADAR_SEARCH_100. Removed all cast to BattalionClass and other unrequired class casting. Let the class hierarchy sort it out
        /*RadarClass* platformradar = NULL;
        if (radarPlatform->IsSim())
        {
         platformradar = (RadarClass*)FindSensor( (SimMoverClass*) radarPlatform, SensorClass::Radar );
         int mode;
         if (((SimBaseClass*)radarPlatform)->IsSim() and ((BattalionClass*)((SimBaseClass*)radarPlatform)->GetCampaignObject()))
         mode = ((BattalionClass*)((SimBaseClass*)radarPlatform)->GetCampaignObject())->GetRadarMode();
         else
         mode = ((SimBaseClass*)radarPlatform)->GetRadarMode();

         if (mode == FEC_RADAR_GUIDE and platformradar)
         {
         SetSensorTarget( platformradar->CurrentTarget());
         CalcRelGeom ((MissileClass *)platform, lockedTarget, NULL, 1.0F);
         }
         else return NULL;
        }*/
        if (radarPlatform->IsSim() and radarPlatform->OnGround())
        {
            RadarClass* radarSensor = (RadarClass*)FindSensor((SimMoverClass*) radarPlatform, SensorClass::Radar);
            int mode = FEC_RADAR_OFF;

            if (((SimBaseClass*)radarPlatform)->GetCampaignObject())
                mode = ((SimBaseClass*)radarPlatform)->GetCampaignObject()->GetRadarMode();

            if (mode == FEC_RADAR_GUIDE and radarSensor)
            {
                SetSensorTarget(radarSensor->CurrentTarget());
                CalcRelGeom((MissileClass *)platform, lockedTarget, NULL, 1.0F);
            }
            else
                return NULL;
        }
    }

    if ( not lockedTarget)
    {
        return NULL;
    }

    // See if our guidance radar is still providing information for us
    if (radarPlatform->IsSim())
    {
        radar = FindSensor((SimMoverClass*)radarPlatform, SensorClass::Radar);
        ShiAssert(radar);

        // 20002-03-13 ADDED BY S.G. If the radarPlatform is the player and the missile is a SARH, make sure he's in STT otherwise break lock
        if (radarPlatform->IsPlayer())
        {
            if (((MissileClass *)platform)->GetSeekerType() == SensorClass::RadarHoming)
            {
                if ( not ((RadarDopplerClass *)radar)->IsSet(RadarDopplerClass::STTingTarget))
                {
                    // That's it, he's off the hook...
                    if (lockedTarget)
                        SendTrackMsg(lockedTarget, Track_Unlock);

                    ClearSensorTarget();
                    return NULL;
                }
            }
        }

        // END OF ADDED SECTION 2002-03-13

#if(0)
        // Cobra - AI SARH support check
        else if (lockedTarget and radarPlatform->IsAirplane())
        {
            if ((((MissileClass *)platform)->GetSeekerType() == SensorClass::RadarHoming) and 
                (((MissileClass *)platform)->GetRuntime() > 0.0f))
            {
                int stat = 0;

                //if (lockedTarget->BaseData()->IsSPJamming())
                //{
                //float ret = ((RadarClass *)platform)->ReturnStrength(lockedTarget);
                //if (ret < 1.0f)
                //{
                // Ok so it's too low, but is it jamming? If so, follow anyway...
                //if (ret == -1.0f or lockedTarget->BaseData()->IsSPJamming())
                //}
                //stat ++;
                //}
                if (radarPlatform->IsDead())
                    stat ++;

                if (radarPlatform->IsExploding())
                    stat ++;

                //if ( not radarPlatform->IsEmitting())
                //stat ++;


                if (stat)
                {
                    //if (lockedTarget->BaseData()->IsSPJamming() or
                    // radarPlatform->IsDead() or
                    // radarPlatform->IsExploding() or
                    // not radarPlatform->IsEmitting())
                    //{
                    if (lockedTarget)
                    {
                        SendTrackMsg(lockedTarget, Track_Unlock);
                        ((MissileClass *)platform)->SetFlag(MissileClass::SensorLostLock);
                        ((MissileClass *)platform)->SetFlag(MissileClass::ClosestApprch);
                        ((MissileClass *)platform)->SetFlag(OBJ_EXPLODING);
                        SetDesiredTarget(NULL);
                    }

                    ClearSensorTarget();
                    return NULL;
                }

                //}
                // Since flights don't currently update their radar state, we'll make this approximation
                float dx = lockedTarget->BaseData()->XPos() - radarPlatform->XPos();
                float dy = lockedTarget->BaseData()->YPos() - radarPlatform->YPos();
                float brg = (float)atan2(dy, dx);
                float angleOff = (float)fmod(fabs(fabs(brg) - fabs(radarPlatform->Yaw())), PI);     // Cobra fabs all vars

                if (angleOff > RadarDataTable[radarPlatform->GetRadarType()].ScanHalfAngle)
                {
                    if (lockedTarget)
                    {
                        SendTrackMsg(lockedTarget, Track_Unlock);
                        ((MissileClass *)platform)->SetFlag(MissileClass::SensorLostLock);
                        ((MissileClass *)platform)->SetFlag(MissileClass::ClosestApprch);
                        ((MissileClass *)platform)->SetFlag(OBJ_EXPLODING);
                        SetDesiredTarget(NULL);
                    }

                    ClearSensorTarget();
                    return NULL;
                }
            }
        }
        // end Cobra
        // Cobra - AI SARH ground support check
        else if ( not radarPlatform->IsAirplane() and radarPlatform->OnGround())
        {
            if ((((MissileClass *)platform)->GetSeekerType() == SensorClass::RadarHoming) and 
                (((MissileClass *)platform)->GetRuntime() > 0.0f))
            {
                //float ret = ((RadarClass *)platform)->ReturnStrength(lockedTarget);
                //if (ret < 1.0f)
                //{
                // Ok so it's too low, but is it jamming? If so, follow anyway...
                //if (ret == -1.0f or lockedTarget->BaseData()->IsSPJamming())
                int stat = 0;

                //if (lockedTarget->BaseData()->IsSPJamming())
                //stat ++;
                if (radarPlatform->IsDead())
                    stat ++;

                if (radarPlatform->IsExploding())
                    stat ++;

                //if ( not radarPlatform->IsEmitting())
                //stat ++;

                if (stat)
                {
                    //if (lockedTarget->BaseData()->IsSPJamming() or
                    // radarPlatform->IsDead() or
                    // radarPlatform->IsExploding() or
                    // not radarPlatform->IsEmitting())
                    //{
                    if (lockedTarget)
                    {
                        SendTrackMsg(lockedTarget, Track_Unlock);
                        ((MissileClass *)platform)->SetFlag(MissileClass::SensorLostLock);
                        ((MissileClass *)platform)->SetFlag(MissileClass::ClosestApprch);
                        ((MissileClass *)platform)->SetFlag(OBJ_EXPLODING);
                        SetDesiredTarget(NULL);
                    }

                    ClearSensorTarget();
                    return NULL;
                }

                //}
            }
        }

        // end Cobra
#endif

        if ( not radar->CurrentTarget() or radar->CurrentTarget()->BaseData() not_eq lockedTarget->BaseData())
        {
            if (lockedTarget)
                SendTrackMsg(lockedTarget, Track_Unlock);

            ClearSensorTarget();
            return NULL;
        }

        // ADDED BY S.G. TO MAKE SURE OUR RADAR IS STILL LOCKED ON THE TARGET AND NOT JAMMED (NEW: USES SensorTrack INSTEAD of noTrack)
        if (radar->CurrentTarget()->localData->sensorState[SensorClass::Radar] not_eq SensorClass::SensorTrack)
        {
            if (lockedTarget)
                SendTrackMsg(lockedTarget, Track_Unlock);

            ClearSensorTarget();
            return NULL;
        }

        // END OF ADDED SECTION
    }
    else // END (radarPlatform->IsSim())
    {
        if (radarPlatform->IsFlight())
        {
            // Since flights don't currently update their radar state, we'll make this approximation
            float dx = lockedTarget->BaseData()->XPos() - radarPlatform->XPos();
            float dy = lockedTarget->BaseData()->YPos() - radarPlatform->YPos();
            float brg = (float)atan2(dy, dx);
            float angleOff = (float)fmod(fabs(fabs(brg) - fabs(radarPlatform->Yaw())), PI);     // Cobra fabs all vars
            //float angleOff = (float)fmod( fabs( brg - radarPlatform->Yaw() ), PI );

            if (angleOff > RadarDataTable[radarPlatform->GetRadarType()].ScanHalfAngle)
            {
                if (lockedTarget)
                    SendTrackMsg(lockedTarget, Track_Unlock);

                ClearSensorTarget();
                return NULL;
            }
        }
        else
        {
            if (radarPlatform->GetRadarMode() not_eq FEC_RADAR_GUIDE)
            {
                if (lockedTarget)
                    SendTrackMsg(lockedTarget, Track_Unlock);

                ClearSensorTarget();
                return NULL;
            }
        }
    }


    // Consider taking one of our target's decoys
    ConsiderDecoy(lockedTarget);


    // Send a launch message to our intended victim (if he isn't a countermeasure)
    if (lockedTarget and not lockedTarget->BaseData()->IsWeapon())
    {
        // 2000-08-31 ADDED BY S.G. SO ARH DOESN'T SEND A LAUNCH WHEN THE MISSILE IS LAUNCHED (IT'S COMMAND GUIDED, NOT A REAL BEAM RIDER)
        if (((MissileClass *)platform)->GetSeekerType() not_eq SensorClass::Radar)
        {
            // END OF ADDED SECTION (EXCEPT FOR THE BLOCK INDENTATION)
            if (lockedTarget->localData->lockmsgsend == Track_Lock and SimLibElapsedTime - lastTargetLockSend > RadarClass::TrackUpdateTime)
            {
                SendTrackMsg(lockedTarget, Track_Launch);
                lastTargetLockSend = SimLibElapsedTime;
            }
        }
    }

    //me123 hardcoded gimbal limit for semiactive/beamrider missiles for now
    //there were no limit before 
    // it's on purpose that this is placed after the send trach msg 
    //ME123 I GUES THIS WAS A BIT TOO HACKY...IT BRAKES MP..FLOODING TRACK LOCK/UNLOCK/LAUNCH MESSAGES
    /* if (lockedTarget and lockedTarget->localData->ata > 60.0f*DTR)
     {
     if (lockedTarget)
     SendTrackMsg( lockedTarget, Track_Unlock );
     ClearSensorTarget();
     return NULL;
     }
    */
    return lockedTarget;
}

void BeamRiderClass::SetGuidancePlatform(FalconEntity* rdrPlat)
{
    if (rdrPlat == radarPlatform)
        return;

    // 2002-03-10 REMOVED BY S.G. Needs to be done AFTER we send a Track_Unlock otherwise it will NEVER send it because radarPlatform will be NULL
    // if (radarPlatform)
    // {
    // VuDeReferenceEntity( radarPlatform );
    // }

    // radarPlatform = rdrPlat;

    if (rdrPlat) // 2002-03-10 MODIFIED BY S.G. Used rdrPlat instead of radarPlatform since I move the line above
    {
        VuReferenceEntity(rdrPlat);   // 2002-03-10 MODIFIED BY S.G. Used rdrPlat instead of radarPlatform since I move the line above
    }
    else
    {
        if (lockedTarget)
            SendTrackMsg(lockedTarget, Track_Unlock);

        ClearSensorTarget();
    }

    // 2002-03-10 ADDED BY S.G. Was above, needs to be done here now
    if (radarPlatform)
    {
        VuDeReferenceEntity(radarPlatform);
    }

    radarPlatform = rdrPlat;

}


void BeamRiderClass::SendTrackMsg(SimObjectType* tgtptr , unsigned int trackType, unsigned int hardpoint)
{
    if ( not radarPlatform) return;

    VU_ID id = tgtptr->BaseData()->Id();
    static int count = 0;
    static int countb = 0;
    count ++;

    if (tgtptr->localData->lockmsgsend == trackType) return;

    if (tgtptr->localData->lockmsgsend == Track_None and trackType == 2) return;

    if ( not ((SimBaseClass*)tgtptr->BaseData())->IsAirplane()) return;

    tgtptr->localData->lockmsgsend = trackType;
    // Create and fill in the message structure
    VuGameEntity *game = vuLocalSessionEntity->Game();

    if ( not game) return;

    VuSessionsIterator Sessioniter(game);
    VuSessionEntity*   sess;
    sess = Sessioniter.GetFirst();
    int reliable = 1;

    while (sess)
    {
        if (
            (sess->CameraCount() > 0) and 
            (
                sess->GetCameraEntity(0)->Id() == platform->Id() or
                sess->GetCameraEntity(0)->Id() == id
            )
        )
        {
            reliable = 2;
            break;
        }

        sess = Sessioniter.GetNext();
    }

    countb ++;
    //  MonoPrint ("BeamriderClass::SendTrackMsg %d %d %d %08x %08x %08x%08x, %d\n",reliable,count,countb, this, platform, id, trackType);

    FalconTrackMessage* trackMsg = new FalconTrackMessage(reliable, radarPlatform->Id(), FalconLocalGame);

    ShiAssert(trackMsg);
    trackMsg->dataBlock.trackType = trackType;
    trackMsg->dataBlock.hardpoint = hardpoint;
    trackMsg->dataBlock.id = id;

    // Send our track list
    FalconSendMessage(trackMsg, TRUE);
}

//me123 changes to chaff effectivenes original 0.0 0.1 0.5 0.5 0.2 0.1
//me123 from 0.0F,  1500.0f,  3000.0f,  11250.0f,  18750.0f,  30000.0f
// This controls how effective countermeasures are as a function of seeker range from target
static const float cmRangeArray[] = {0.0F,  1500.0f,  3000.0f,  11250.0f,  18750.0f,  30000.0f};
static const float cmBiteChanceArray[] = {0.0F,     0.1F,     0.5F,      0.5F,      0.2F,      0.1F};
static const int cmArrayLength = sizeof(cmRangeArray) / sizeof(cmRangeArray[0]);


// TODO:  Consider how this affects and is affected by the radar platform...
void BeamRiderClass::ConsiderDecoy(SimObjectType *target)
{
    VU_ID id;
    FalconEntity *cm;
    float chance;
    int dummy = 0;

    // No counter measures deployed by campaign things
    if ( not target or not target->BaseData()->IsSim())
    {
        return;
    }

    // Get the ID of the most recently launched counter measure from our target
    // 2000-11-24 REMOVED BY S.G. TO BRING IT TO RP4 LEVEL
    // if (chafftime + 5000 <= SimLibElapsedTime) //me123 let's give the chaff that worked some effective time
    id = ((SimBaseClass*)target->BaseData())->NewestChaffID();

    // If we have a new chaff bundle to deal with
    if (id not_eq lastChaffID)
    {
        // Stop here if there isn't a counter measure in play
        if (id == FalconNullId)
        {
            lastChaffID = id;
            return;
        }

        // Try to find the counter measure entity in the database
        cm = (FalconEntity*)vuDatabase->Find(id);

        if ( not cm)
        {
            // We'll have to wait until next time
            // (probably because the create event hasn't been processed locally yet)
            return;
        }

        // Start with the suceptability of this seeker to counter measures
        chance = RadarDataTable[ radarPlatform->GetRadarType() ].ChaffChance;

        // Adjust with a range to target based chance of an individual countermeasure working
        chance *= Math.OnedInterp(target->localData->range, cmRangeArray, cmBiteChanceArray, cmArrayLength, &dummy);

        // Player countermeasures work better //no thins is crap
        // if (target->BaseData()->IsPlayer()) {
        // chance *= 1.15F;
        // }

        // Roll the dice
        if (chance > (float)rand() / RAND_MAX)
        {

            // Compute some relative geometry stuff
            const float atx = platform->dmx[0][0];
            const float aty = platform->dmx[0][1];
            const float atz = platform->dmx[0][2];
            const float dx = cm->XPos() - platform->XPos();
            const float dy = cm->YPos() - platform->YPos();
            const float dz = cm->ZPos() - platform->ZPos();
            const float range = (float)sqrt(dx * dx + dy * dy);
            const float cosATA = (atx * dx + aty * dy + atz * dz) / (float)sqrt(range * range + dz * dz);

            // Only take the bait if we can see the thing
            if (cosATA >= cos(CM_EFFECTIVE_ANGLE))
            {
#ifdef DEBUG
                /* SetSensorTarget( new SimObjectType(OBJ_TAG, platform, cm) );*/
#else

                SetSensorTarget(new SimObjectType(cm));
                // 2000-11-24 QUESTION BY S.G. me123, WHY DID YOU LIMIT IT TO 'RELEASE MODE'?
                chafftime = SimLibElapsedTime;
#endif
            }
        }

        // Note that we've considered this countermeasure
        lastChaffID = id;
    }
}
