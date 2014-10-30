#include "Graphics/Include/drawbsp.h"
#include "stdhdr.h"
#include "falcmesg.h"
#include "simfeat.h"
#include "otwdrive.h"
#include "MsgInc/DamageMsg.h"
#include "MsgInc/DeathMessage.h"
#include "campbase.h"
#include "simmover.h"
#include "Simdrive.h"
#include "feature.h"
#include "fsound.h"
#include "soundfx.h"
#include "sfx.h"
#include "fakerand.h"
#include "falcsess.h"
#include "acmi/src/include/acmirec.h"
#include "camplist.h"
#include "objectiv.h"
#include "Graphics/Include/drawparticlesys.h"
#include "Classtbl.h"

extern void UpdateDrawableObject(SimFeatureClass *theFeature);

void SimFeatureClass::ApplyDamage(FalconDamageMessage* damageMessage)
{
    FalconDeathMessage* deathMessage = NULL;
    Falcon4EntityClassType* classPtr = NULL;
    FeatureClassDataType *fc = NULL;
    WeaponClassDataType *wc = NULL;
    float hitPoints = 0.0F;
    float pctDamage = 0.0F;
    Tpoint minB = {0.0F}, maxB = {0.0F}, pos = {0.0F};
    float startPctStrength = 0.0F;
    Tpoint ppos = {0.0F}, vec = {0.0F};
    int i = 0;
    int numSfx = 0;
    float x1 = 0.0F, x2 = 0.0F, y1 = 0.0F, y2 = 0.0F;
    ACMIFeatureStatusRecord featStat;
    float fireScale = 0.0F;
    float timeToLive = 0.0F;

    //RV - I-Hawk - Added a 0 vector for RV new PS calls
    Tpoint PSvec;
    PSvec.x = 0;
    PSvec.y = 0;
    PSvec.z = 0;

    if (IsExploding() or IsDead() or pctStrength < 0.0F or (Status() bitand VIS_TYPE_MASK) == VIS_DESTROYED)
    {
        // Just double check the drawable object and return
        UpdateDrawableObject(this);
        return;
    }

    // call base class damage function
    SimBaseClass::ApplyDamage(damageMessage);

    // get classtbl entry for feature
    classPtr = &Falcon4ClassTable[ Type() - VU_LAST_ENTITY_TYPE];
    // get feature data
    fc = (FeatureClassDataType *)classPtr->dataPtr;

    switch (damageMessage->dataBlock.damageType)
    {
            // This means an actual weapon hit the thingy.. Apply the weapon's damage type to the feature
        case FalconDamageType::BulletDamage:
        case FalconDamageType::MissileDamage:
        case FalconDamageType::BombDamage:
            // get weapon data
            wc = (WeaponClassDataType *)Falcon4ClassTable[ damageMessage->dataBlock.fWeaponID - VU_LAST_ENTITY_TYPE].dataPtr;
            // get strength of weapon and calc damage based on vehicle mod
            hitPoints = (float)wc->Strength * ((float)fc->DamageMod[ wc->DamageType ]) / 100.0f;

            // debug
            // hitPoints = maxStrength;

            break;

            // KCK: This means some exploded nearby this thingy.. Apply high explosive damage
        case FalconDamageType::ProximityDamage:
            hitPoints = (float)damageMessage->dataBlock.damageStrength * ((float)fc->DamageMod[HighExplosiveDam]) / 100.0f;
            break;

            // for these types, sender passes in damage strength
        case FalconDamageType::FeatureCollisionDamage:
        case FalconDamageType::ObjectCollisionDamage:
        case FalconDamageType::GroundCollisionDamage:
        case FalconDamageType::CollisionDamage:
        case FalconDamageType::DebrisDamage:
            hitPoints = damageMessage->dataBlock.damageStrength;
            break;

        case FalconDamageType::FODDamage:
            hitPoints = maxStrength;
            break;

        default:
            hitPoints = 0;
            break;
    }

    if ( not hitPoints)
        return;

    // apply randomness to hit points (and double for features)
    hitPoints += /*hitPoints +*/ hitPoints * 0.25f * damageMessage->dataBlock.damageRandomFact;

    // what percent strength is left?
    strength -= hitPoints;
    startPctStrength = pctStrength;
    pctStrength = strength / maxStrength;
    pctDamage = min(1.0f, hitPoints / maxStrength);

    // MonoPrint ("Feature %d took %d hits at %8ld (%d%% damaged)\n", Id().num_,
    // (int)hitPoints, SimLibElapsedTime,(int)(100.0F * (1.0F - pctStrength)));

    // get bounding box for sfx
    // do it now, prior to any potential destruction and replacement of
    // damaged drawpointer
    if (IsAwake() and drawPointer)
    {
        ((DrawableBSP *)drawPointer)->GetBoundingBox(&minB, &maxB);
        drawPointer->GetPosition(&pos);
    }

    // Check for death..
    if (pctStrength <= 0.0F)
    {
        // Kill us
        if ((Status() bitand VIS_TYPE_MASK) not_eq VIS_DESTROYED)
        {

            if (gACMIRec.IsRecording())
            {
                featStat.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
                featStat.data.uniqueID = ACMIIDTable->Add(Id(), NULL, 0); //.num_;
                featStat.data.newStatus = VIS_DESTROYED;
                featStat.data.prevStatus = (Status() bitand VIS_TYPE_MASK);
                gACMIRec.FeatureStatusRecord(&featStat);
            }

            ClearStatusBit(VIS_TYPE_MASK);
            SetStatusBit(VIS_DESTROYED);

            // 2001-03-07 ADDED BY S.G. SO RADAR FEATURE HAS A ZERO RADAR RANGE ONCE DESTOYED (USED BY RWR CanDetectObject FUNCTION
            if (((FeatureClassDataType *)Falcon4ClassTable[ Type() - VU_LAST_ENTITY_TYPE ].dataPtr)->RadarType)
                SetRdrRng(0);

            // END OF ADDED FUNCTION

            // Is this something we were managing in our time of day list?
            if (IsSetCampaignFlag(FEAT_HAS_LIGHT_SWITCH) and drawPointer)
            {
                OTWDriver.RemoveFromLitList((DrawableBSP *)drawPointer);
            }

            if (IsLocal())
            {
                // send out death message if it's owned by us.
                deathMessage = new FalconDeathMessage(Id(), FalconLocalGame);
                deathMessage->dataBlock.damageType = damageMessage->dataBlock.damageType;

                deathMessage->dataBlock.dEntityID  = Id();

                ShiAssert(GetCampaignObject())
                deathMessage->dataBlock.dCampID = ((CampBaseClass*)GetCampaignObject())->GetCampID();
                deathMessage->dataBlock.dSide   = ((CampBaseClass*)GetCampaignObject())->GetOwner();
                deathMessage->dataBlock.dPilotID   = 255;   // There is no dead pilot
                deathMessage->dataBlock.dIndex     = Type();

                deathMessage->dataBlock.fEntityID  = damageMessage->dataBlock.fEntityID;
                deathMessage->dataBlock.fCampID    = damageMessage->dataBlock.fCampID;
                deathMessage->dataBlock.fSide      = damageMessage->dataBlock.fSide;
                deathMessage->dataBlock.fPilotID   = damageMessage->dataBlock.fPilotID;
                deathMessage->dataBlock.fIndex     = damageMessage->dataBlock.fIndex;
                deathMessage->dataBlock.fWeaponID  = damageMessage->dataBlock.fWeaponID;
                deathMessage->dataBlock.fWeaponUID = damageMessage->dataBlock.fWeaponUID;
                deathMessage->dataBlock.deathPctStrength = pctStrength;

                FalconSendMessage(deathMessage, TRUE);

                ((Objective)GetCampaignObject())->SetFeatureStatus(slotNumber, VIS_DESTROYED);
            }

            // MonoPrint ("Feature %d DEAD at %8ld\n", Id().num_, SimLibElapsedTime);

            // Kill off any critical links
            damageMessage->dataBlock.damageType = FalconDamageType::FODDamage;
            damageMessage->dataBlock.damageRandomFact = 0.0F;

            if ((featureFlags bitand FEAT_PREV_CRIT or featureFlags bitand FEAT_PREV_NORM))
            {
                SimFeatureClass *prevObj = (SimFeatureClass*) GetCampaignObject()->GetComponentEntity(GetCampaignObject()->GetComponentIndex(this) - 1);

                if (prevObj and featureFlags bitand FEAT_PREV_CRIT and (prevObj->Status() bitand VIS_TYPE_MASK) not_eq VIS_DESTROYED)
                {
                    MonoPrint("ID %d taking previous neighbor with it\n", GetCampaignObject()->GetComponentIndex(this));
                    prevObj->ApplyDamage(damageMessage);
                }
                else if (prevObj)
                    UpdateDrawableObject(prevObj);
            }

            if ((featureFlags bitand FEAT_NEXT_CRIT or featureFlags bitand FEAT_NEXT_NORM))
            {
                SimFeatureClass *nextObj = (SimFeatureClass*) GetCampaignObject()->GetComponentEntity(GetCampaignObject()->GetComponentIndex(this) + 1);

                if (nextObj and featureFlags bitand FEAT_NEXT_CRIT and (nextObj->Status() bitand VIS_TYPE_MASK) not_eq VIS_DESTROYED)
                {
                    MonoPrint("ID %d taking next neighbor with it\n", GetCampaignObject()->GetComponentIndex(this));
                    nextObj->ApplyDamage(damageMessage);
                }
                else if (nextObj)
                    UpdateDrawableObject(nextObj);
            }
        }

        // Update the drawable, to reflect our new state
        UpdateDrawableObject(this);
    }
    else if (pctStrength <= 0.75F and (Status() bitand VIS_TYPE_MASK) not_eq VIS_DAMAGED)
    {
        // MonoPrint ("Feature %d DAMAGED at %8ld\n", Id().num_, SimLibElapsedTime);

        ((Objective)GetCampaignObject())->SetFeatureStatus(slotNumber, VIS_DAMAGED);

        if (gACMIRec.IsRecording())
        {
            featStat.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
            featStat.data.uniqueID = ACMIIDTable->Add(Id(), NULL, 0); //.num_;
            featStat.data.newStatus = VIS_DAMAGED;
            featStat.data.prevStatus = (Status() bitand VIS_TYPE_MASK);
            gACMIRec.FeatureStatusRecord(&featStat);
        }

        ClearStatusBit(VIS_TYPE_MASK);
        SetStatusBit(VIS_DAMAGED);

        // Is this something we were managing in our time of day list?
        if (IsSetCampaignFlag(FEAT_HAS_LIGHT_SWITCH) and drawPointer)
        {
            OTWDriver.RemoveFromLitList((DrawableBSP *)drawPointer);
        }

        // Update the drawable, to reflect our new state
        UpdateDrawableObject(this);
    }


    // show damage and destruction effects here....
    if (pctDamage > 0.0f and IsAwake() and drawPointer)
    {
        // get x and y bbox vals depending on objs rotation
        x1 =
            ((DrawableBSP *)drawPointer)->orientation.M11 * minB.x +
            ((DrawableBSP *)drawPointer)->orientation.M21 * minB.y +
            ((DrawableBSP *)drawPointer)->orientation.M31 * minB.z;
        x2 =
            ((DrawableBSP *)drawPointer)->orientation.M11 * maxB.x +
            ((DrawableBSP *)drawPointer)->orientation.M21 * maxB.y +
            ((DrawableBSP *)drawPointer)->orientation.M31 * maxB.z;
        y1 =
            ((DrawableBSP *)drawPointer)->orientation.M12 * minB.x +
            ((DrawableBSP *)drawPointer)->orientation.M22 * minB.y +
            ((DrawableBSP *)drawPointer)->orientation.M32 * minB.z;
        y2 =
            ((DrawableBSP *)drawPointer)->orientation.M12 * maxB.x +
            ((DrawableBSP *)drawPointer)->orientation.M22 * maxB.y +
            ((DrawableBSP *)drawPointer)->orientation.M32 * maxB.z;

        // set x and y diff
        x1 = max(20.0f, (float)fabs(x1 - x2) * 0.5f);
        y1 = max(20.0f, (float)fabs(y1 - y2) * 0.5f);

        // fire or smoke
        if (pctStrength > 0.0f)
        {
            // edg: test dynamic vertices here
            for (i = 0; i < ((DrawableBSP *)drawPointer)->GetNumDynamicVertices(); i++)
            {
                // this should lower the verts a maximum of 20ft
                ((DrawableBSP*)drawPointer)->SetDynamicVertex(i, 0.0f, 0.0F, 20.0F * pctStrength);
            }

            if (pctDamage * PRANDFloatPos() > 0.02f)
            {
                if ((fc->Flags bitand FEAT_CAN_BURN))
                {
                    fireScale = max(x1, y1) * 0.30f + max(x1, y1) * 0.70f * PRANDFloatPos();
                    fireScale = min((float)fabs(minB.z) * 2.0f, fireScale);
                    ppos.z = pos.z - fireScale * 0.5f + PRANDFloatPos() * minB.z * 0.3f;
                    ppos.x = pos.x + PRANDFloat() * x1 * 0.7f;
                    ppos.y = pos.y + PRANDFloat() * y1 * 0.7f;
                    timeToLive = 20.0f + PRANDFloatPos() * 90.0f;

                    //RV- I-Hawk- Type changed to FIRE6 instead of FIRE or FIRE_NOSMOKE
                    //for Damaged features to produce different effect than destroyed features
                    /*
                    OTWDriver.AddSfxRequest(
                     new SfxClass (SFX_FIRE6, // type
                     &ppos, // world pos
                     timeToLive,
                     fireScale) ); // scale
                    */
                    DrawableParticleSys::PS_AddParticleEx((SFX_FIRE6 + 1),
                                                          &ppos,
                                                          &PSvec);
                    ppos.z = pos.z;
                    /*
                    OTWDriver.AddSfxRequest(
                     new SfxClass (SFX_FEATURE_EXPLOSION, // type
                     &ppos, // world pos
                     timeToLive,
                     fireScale * 2.0f) ); // scale
                     */
                    DrawableParticleSys::PS_AddParticleEx((SFX_FEATURE_EXPLOSION + 1),
                                                          &ppos,
                                                          &PSvec);
                }

                else if ((fc->Flags bitand FEAT_CAN_SMOKE))
                {
                    ppos.z = pos.z + PRANDFloatPos() * minB.z * 0.5f + minB.z * 0.5f;;
                    ppos.x = pos.x + PRANDFloat() * x1;
                    ppos.y = pos.y + PRANDFloat() * y1;

                    /*
                    OTWDriver.AddSfxRequest(
                     new SfxClass (SFX_BILLOWING_SMOKE, // type
                     &ppos, // world pos
                     10 + PRANDInt3() * 10, // count
                     0.5f ) ); // interval
                     */
                    DrawableParticleSys::PS_AddParticleEx((SFX_BILLOWING_SMOKE + 1),
                                                          &ppos,
                                                          &PSvec);
                }
            }
        }

        // possible explosion
        if ((fc->Flags bitand FEAT_CAN_EXPLODE) and 
            (pctDamage > 0.1f or pctStrength < 0.3f) and 
 not (rand() bitand 0x03))
        {
            ppos.z = pos.z + PRANDFloatPos() * minB.z * 0.5f + minB.z * 0.5f;
            ppos.x = pos.x + PRANDFloat() * x1;
            ppos.y = pos.y + PRANDFloat() * y1;
            /*
            OTWDriver.AddSfxRequest(
             new SfxClass (SFX_VEHICLE_EXPLOSION, // type
             &ppos, // world pos
             3.0f, // time to live
             0.3f ) ); // scale
             */

            //I-Hawk - Removing this call to prevent features with "CAN EXPLODE" flags to overload
            // the PS engine... they have enough other PS generated from other calls.
            /*
            DrawableParticleSys::PS_AddParticleEx((SFX_VEHICLE_EXPLOSION + 1),
             &ppos,
             &PSvec);
            */

            //F4SoundFXSetPos( SFX_BOOMG1 + PRANDInt5(), TRUE, pos.x, pos.y, pos.z, 1.0f );
            SoundPos.Sfx(SFX_BOOMG1 + PRANDInt5());  // MLR 5/16/2004 -
        }

        if (pctStrength <= 0.0f and not IsSetFlag(SHOW_EXPLOSION))
        {
            SetFlag(SHOW_EXPLOSION);

            // grande finale
            // edg note: explode them all for now
            // if ( fc->Flags bitand FEAT_CAN_EXPLODE )
            if (fc->Flags bitand (FEAT_CAN_BURN bitor FEAT_CAN_EXPLODE))
            {
                ppos = pos;
                ppos.z += minB.z * 0.15f;

                // vec.z = delta explosion chain rises
                // vec.x and y = dimensions within which to randomize
                vec.z = minB.z * 0.1f;
                vec.x = x1 * 0.8f;
                vec.y = y1 * 0.8f;

                // MonoPrint( "Feature Exploding:  x = %f, y = %f, z = %f \n",
                // vec.x,
                // vec.y,
                // vec.z  );

                // number to do is based on LOD
                if (fc->Flags bitand (FEAT_CAN_EXPLODE))
                    i = (int)(8.0f * (0.5f + gSfxLOD));
                else
                    i = 1;

                // test explosion
                /*
                OTWDriver.AddSfxRequest(
                 new SfxClass (SFX_RISING_GROUNDHIT_EXPLOSION_DEBR, // type
                 &ppos, // world pos
                 &vec,
                 i, // number to generate
                 0.8f ) ); // interval
                 */

                DrawableParticleSys::PS_AddParticleEx((SFX_RISING_GROUNDHIT_EXPLOSION_DEBR + 1),
                                                      &pos,
                                                      &vec);


                // apply chain reaction if feature can explode
                if (fc->Flags bitand (FEAT_CAN_EXPLODE))
                {
                    // vec.x is actually our explosion radius
                    vec.x = max(x1, y1) * 8.0f;

                    /*
                    OTWDriver.AddSfxRequest(
                     new SfxClass (SFX_FEATURE_CHAIN_REACTION, // type
                     &pos, // world pos
                     &vec,
                     1, // number to generate
                     2.8f + PRANDFloatPos() * 4.0f ) ); // interval
                     */
                    DrawableParticleSys::PS_AddParticleEx((SFX_FEATURE_CHAIN_REACTION + 1),
                                                          &pos,
                                                          &vec);
                }

                numSfx = PRANDInt5() + 3;

                for (i = 0; i < numSfx; i++)
                {
                    fireScale = max(x1, y1) * 0.30f + max(x1, y1) * 0.70f * PRANDFloatPos();
                    fireScale = min((float)fabs(minB.z) * 2.0f, fireScale);
                    ppos.z = pos.z - fireScale * 0.5f + PRANDFloatPos() * minB.z * 0.3f;
                    ppos.x = pos.x + PRANDFloat() * x1;
                    ppos.y = pos.y + PRANDFloat() * y1;
                    timeToLive = 30.0f + PRANDFloatPos() * 120.0f;

                    if (rand() bitand 1)
                    {
                        //RV I-Hawk Added a check to seperate CAN_EXPLODE and CAN_BURN features, burn type
                        if ( not (fc->Flags bitand (FEAT_CAN_EXPLODE)))
                            /*
                            OTWDriver.AddSfxRequest(
                             new SfxClass (SFX_FIRE, // type
                             &ppos, // world pos
                             timeToLive,
                             fireScale ) ); // scale
                             */
                            DrawableParticleSys::PS_AddParticleEx((SFX_FIRE + 1),
                                                                  &ppos,
                                                                  &PSvec);
                        else
                            /*
                            OTWDriver.AddSfxRequest(
                             new SfxClass (SFX_FIRE_HOT, // type
                             &ppos, // world pos
                             timeToLive,
                             fireScale ) ); // scale
                             */
                            DrawableParticleSys::PS_AddParticleEx((SFX_FIRE_HOT + 1),
                                                                  &ppos,
                                                                  &PSvec);

                    }
                    else
                    {
                        /*
                        OTWDriver.AddSfxRequest(
                         new SfxClass (SFX_FIRE_NOSMOKE, // type
                         &ppos, // world pos
                         timeToLive,
                         fireScale ) ); // scale
                         */
                        DrawableParticleSys::PS_AddParticleEx((SFX_FIRE_NOSMOKE + 1),
                                                              &ppos,
                                                              &PSvec);
                    }

                    ppos.z = pos.z;
                    /*
                    OTWDriver.AddSfxRequest(
                     new SfxClass (SFX_FEATURE_EXPLOSION, // type
                     &ppos, // world pos
                     timeToLive,
                     fireScale * 2.0f) ); // scale
                     */
                    DrawableParticleSys::PS_AddParticleEx((SFX_FEATURE_EXPLOSION + 1),
                                                          &ppos,
                                                          &PSvec);
                } // end for # firesfx
            } // end if can burn or explode
            else if (fc->Flags bitand (FEAT_CAN_SMOKE))
            {
                ppos.z = pos.z - 4.0f;
                vec.z = -70.0f;

                for (i = 0; i < 5; i++)
                {
                    ppos.x = pos.x + PRANDFloat() * x1;
                    ppos.y = pos.y + PRANDFloat() * y1;
                    vec.x =  PRANDFloat() * 30.0f;
                    vec.y =  PRANDFloat() * 30.0f;

                    /*
                    OTWDriver.AddSfxRequest(
                     new SfxClass( SFX_FIRE5, // type
                     SFX_MOVES bitor SFX_USES_GRAVITY bitor SFX_NO_DOWN_VECTOR,
                     &ppos, // world pos
                     &vec, // vel vector
                     3.0, // time to live
                     5.0f ) ); // scale
                     */
                    DrawableParticleSys::PS_AddParticleEx((SFX_FIRE5 + 1),
                                                          &ppos,
                                                          &vec);
                }
            }
        }
    }
}


// Update the drawable object of this feature to reflect current damage state-
// Note: CreateVisualObject will do nothing if the visType hasn't actually changed
void UpdateDrawableObject(SimFeatureClass *theFeature)
{
    // Update the drawable object for this feature
    if (theFeature->IsAwake())
        OTWDriver.CreateVisualObject(theFeature, OTWDriver.Scale()); // FRB

    // if we're damaged,, use the damaged texture set
    if ((theFeature->Status() bitand VIS_TYPE_MASK) == VIS_DAMAGED or (theFeature->Status() bitand VIS_TYPE_MASK) == VIS_DESTROYED)
    {
        if (theFeature->drawPointer)
        {
            // RV - Biker - Because we have more than 2 texture sets for number type runways use last texture set available
            int numTextures = ((DrawableBSP*)(theFeature->drawPointer))->GetNTextureSet();
            ((DrawableBSP*)(theFeature->drawPointer))->SetTextureSet(max(0, numTextures - 1));
        }
    }
}


/*
** Name: ApplyChainReaction
** Description:
** Used when a feature is exploding and damages other features around
** it.   This is called by the special effects since we want to
** delay the times on the chain reaction.
** Cycles thru objectList check for proximity.
** Cycles thru all objectives, and checks vs individual features
**        if it's within the objective's bounds.
**
** Returns FALSE when nothing found to destroy
*/
BOOL
SimFeatureClass::ApplyChainReaction(Tpoint *pos, float radius)
{
    float tmpX, tmpY;
    float rangeSquare;
    SimBaseClass* testObject;
    SimBaseClass* nearObject;
    CampBaseClass* objective;
    FalconDamageMessage *message;
    int numFound;
    float nearRange;
#ifdef VU_GRID_TREE_Y_MAJOR
    VuGridIterator gridIt(ObjProxList, pos->y, pos->x, NM_TO_FT * 1.5F);
#else
    VuGridIterator gridIt(ObjProxList, pos->x, pos->y, NM_TO_FT * 1.5F);
#endif

    // square the radius for comparision purposes
    radius *= radius;

    // initialize
    nearObject = NULL;
    numFound = 0;
    nearRange = radius * 2.0f;

    // get the 1st objective that contains the bomb
    objective = (CampBaseClass*)gridIt.GetFirst();

    // main loop through objectives
    while (objective)
    {
        if (objective->GetComponents())
        {
            // loop thru each element in the objective
            VuListIterator featureWalker(objective->GetComponents());
            testObject = (SimBaseClass*) featureWalker.GetFirst();

            while (testObject)
            {
                // we don't want to apply more damage to stuff that's
                // already exploded
                if (testObject->IsSetFlag(SHOW_EXPLOSION))
                {
                    testObject = (SimBaseClass*) featureWalker.GetNext();
                    continue;
                }

                // we don't check container tops
                if (testObject->IsSetCampaignFlag(FEAT_CONTAINER_TOP))
                {
                    testObject = (SimBaseClass*) featureWalker.GetNext();
                    continue;
                }

                tmpX = testObject->XPos() - pos->x;
                tmpY = testObject->YPos() - pos->y;

                rangeSquare = tmpX * tmpX + tmpY * tmpY;; // + tmpZ*tmpZ;

                // is object within lethal explosion radius?
                if (rangeSquare < radius)
                {
                    numFound++;

                    // is it the closest so far?
                    if (rangeSquare < nearRange)
                    {
                        nearObject = testObject;
                        nearRange = rangeSquare;
                    }

                } // end if within lethal radius

                // next object
                testObject = (SimBaseClass*) featureWalker.GetNext();
            }
        }

        // get the next objective that contains the bomb
        objective = (CampBaseClass*)gridIt.GetNext();
    } // end objective loop

    if (nearObject)
    {
        // we've got a hit, send damage message to feature
        message = new FalconDamageMessage(nearObject->Id(), FalconLocalGame);
        message->dataBlock.fEntityID  = nearObject->Id();
        message->dataBlock.fCampID = 0;
        message->dataBlock.fSide   = nearObject->GetCountry();
        message->dataBlock.fPilotID   = 255;
        message->dataBlock.fIndex     = nearObject->Type();
        message->dataBlock.fWeaponID  = 0;
        message->dataBlock.fWeaponUID = nearObject->Id();
        message->dataBlock.dEntityID  = nearObject->Id();
        message->dataBlock.dCampID = 0;
        message->dataBlock.dSide   = nearObject->GetCountry();
        message->dataBlock.dPilotID   = 255;
        message->dataBlock.dIndex     = nearObject->Type();
        message->dataBlock.damageRandomFact = 1.0f;
        message->dataBlock.damageType = FalconDamageType::DebrisDamage;
        message->dataBlock.damageStrength = 1.0f;
        //me123 message->RequestOutOfBandTransmit ();
        FalconSendMessage(message, TRUE);
    }

    // if there's more than 1 found, return true
    if (numFound > 1)
        return TRUE;

    return FALSE;
}
