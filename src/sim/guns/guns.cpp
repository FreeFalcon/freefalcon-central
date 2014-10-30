#include "stdhdr.h"
#include "falcmesg.h"
#include "object.h"
#include "guns.h"
#include "object.h"
#include "simdrive.h"
#include "simmover.h"
#include "MsgInc/DamageMsg.h"
#include "campbase.h"
#include "otwdrive.h"
#include "sfx.h"
#include "fsound.h"
#include "soundfx.h"
#include "fakerand.h"
#include "Graphics/Include/drawsgmt.h"
#include "Graphics/Include/RViewPnt.h"
#include "feature.h"
#include "acmi/src/include/acmirec.h"
#include "playerop.h"
#include "classtbl.h"
#include "entity.h"
#include "camplib.h"
#include "campweap.h"
#include "camp2sim.h"
#include "Graphics/Include/terrtex.h"
#include "camplist.h"
#include "aircrft.h"
#include "IvibeData.h"
#include "DrawParticleSys.h" // RV - I-Hawk - added to support RV new trails code


#ifdef USE_SH_POOLS
MEM_POOL GunClass::pool;
#endif

float BulletSphereHit(vector *sp, vector *ep, vector *tc, float r, vector *impact);
#define NUM_BULLETS 11

ACMITracerStartRecord acmiTracer;


#define NODRAG

GunClass::GunClass(int type) : SimWeaponClass(type)
{
    InitLocalData(type);
}

GunClass::~GunClass()
{
    CleanupLocalData();
}

void GunClass::Init(float muzzleVel, int numRounds)
{
    int i;
    // 2000-10-18 ADDED BY S.G. SO BULLETS HAVE THEIR OWN VELOCITY
    // ANY REFERENCE TO initBulletVelocity IN THIS FUNCTION HAS BEEN COMMENTED OUT AS WELL
    // We'll use an unused field in the WCD file.
    //The LAST 5 bits will be used and the value will range from 256 to 8192 ft/sec.
    initBulletVelocity = (float)(((((unsigned char *)wcPtr)[45] >> 3) + 1) * 256);

    if (typeOfGun == GUN_TRACER or typeOfGun == GUN_TRACER_BALL)
    {
        if (parent->IsGroundVehicle())
        {
            unlimitedAmmo = TRUE;
        }

        bullet = new GunTracerType[NUM_BULLETS];
        numTracers = (int)(NUM_BULLETS);

        // This really should be player only, but not sure how to decide if this will be a player AC
        if (parent->IsAirplane())
            numFirstTracers = (int)(max(4.0f, gSfxLOD * 12.0f));
        else
            numFirstTracers = 1;

        ShiAssert(bullet);
        // Marco Edit
        // Externalized Gun Fire rate - last byte of MaxAlt
        // Original setting - remmed out
        // roundsPerSecond = 100.0F;
        //roundsPerSecond = (float) (wcPtr->MaxAlt)[2] ;
        roundsPerSecond = (float)((((unsigned char *)wcPtr)[59]));

        if (roundsPerSecond == 0.0)
            roundsPerSecond = 100.0;


        // End Marco Edit
        numRoundsRemaining = numRounds * 10;
        initialRounds = numRoundsRemaining;

        for (i = 0; i < numTracers; i++)
        {
            bullet[i].flying = FALSE;
        }

        tracerMode = COLLIDE_CHECK_ALL;
    }
    else // SHELLS
    {
        numRoundsRemaining = numRounds;
        roundsPerSecond = 1.0F;
        //    initBulletVelocity = muzzleVel * 0.75F;
        numFirstTracers = 0;
        numTracers = 0;
    }

    dragFactor  = 0.5F * RHOASL * 0.0055F * 0.15F / 0.03F;
    status = Ready;
    //RV - I-Hawk - RV new trails call changes
    //smokeTrail = NULL;
    TrailIdNew = 0;
    trailID = TRAIL_GUN;

    //MI to add a timer to AAA
    CheckAltitude = TRUE;
    AdjustForAlt = SimLibElapsedTime;
    TargetAlt = 0.0f;
}

void GunClass::InitData()
{
    SimMoverClass::InitData();
    InitLocalData(Type());
}

void GunClass::InitLocalData(int type)
{
    float baseRange;
    int domain = 0;

    // type is the description index into the class table
    // get the weapon class data
    type -= VU_LAST_ENTITY_TYPE;

    wcPtr = (WeaponClassDataType *)Falcon4ClassTable[type].dataPtr;

    // hack at the moment for class tbl problem with flak
    if (strcmp("30mm AAA HE", wcPtr->Name) == 0)
    {
        typeOfGun = GUN_SHELL;
    }
    else if (wcPtr->Flags bitand WEAP_TRACER)
    {
        Falcon4EntityClassType* classPtr = (Falcon4EntityClassType*)EntityType();

        if (classPtr->vuClassData.classInfo_[VU_STYPE] == STYPE_AAA_GUN)
        {
            typeOfGun = GUN_TRACER_BALL;
        }
        else
        {
            typeOfGun = GUN_TRACER;
        }
    }
    else
    {
        typeOfGun = GUN_SHELL;
    }

    unlimitedAmmo = FALSE;
    bullet = NULL;
    numRoundsRemaining = 0;
    fractionalRoundsRemaining = 0.0f;
    fireCount = 0;
    tracers = NULL;
    trailState = NULL;
    smokeTrail = NULL;
    muzzleStart = 0;
    qTimer = 0.0f;
    numFlying = 0;
    FiremsgsendTime = 0;
    bursts = 0;
    shellTargetPtr = NULL;
    shellDetonateTime = 0;
    gunDomain = wdNoDomain; // air, land, both

    // ok, try and figure out what the domain is from the
    // weapon hit chance table
    if (
        wcPtr->HitChance[ NoMove ] > 0 or wcPtr->HitChance[ Foot ] > 0 or wcPtr->HitChance[ Wheeled ] > 0 or
        wcPtr->HitChance[ Tracked ] > 0 or wcPtr->HitChance[ Naval ] > 0 or wcPtr->HitChance[ Rail ] > 0
    )
    {
        domain = (int)wdGround;
    }

    if (wcPtr->HitChance[ Air ] > 0 or wcPtr->HitChance[ LowAir ] > 0)
    {
        domain or_eq (int)wdAir;
    }

    gunDomain = (WeaponDomain)domain;

    // get a base range for the weapon
    baseRange = wcPtr->Range * 6000.0f;

    if (wcPtr->Range == 1)
    {
        minShellRange = 0.0f;
    }
    else
    {
        minShellRange = baseRange * 0.3f;
    }

    maxShellRange = baseRange * 1.7f;
    xPos = yPos = zPos = 0;
    pitch = yaw = 0;
}

void GunClass::CleanupLocalData()
{
    if (typeOfGun == GUN_TRACER or typeOfGun == GUN_TRACER_BALL)
    {
        delete [] bullet;
        bullet = NULL;

        if (smokeTrail)
        {
            OTWDriver.RemoveObject(smokeTrail, TRUE);
        }
    }
    else
    {
        FireShell(NULL);
    }
}

void GunClass::CleanupData()
{
    CleanupLocalData();
    SimWeaponClass::CleanupData();
}


void GunClass::SetPosition(float xOffset, float yOffset, float zOffset, float p, float y)
{
    xPos = xOffset;
    // edg NOTE: the cockpit (virtual anyways) is 2x scale.  In order that
    // it looks like the bullets are flying out of the correct position,
    // scale the yoffset further out (to the left).  This won't look
    // quite right from external views.  What we should do is check in the
    // exec loop to see of we're: 1) ownship, and 2) in internal/external
    // view and scale dynamically
    yPos = yOffset;
    zPos = zOffset;
    pitch = p;
    yaw   = y;
}

int GunClass::Exec(
    int* fire, TransformMatrix dmx, ObjectGeometry *geomData, SimObjectType* targetList, BOOL isOwnship
)
{

    int i;
    float timeToImpact, vt;
    vector p1, p2, p3;
    FalconDamageMessage* message;
    SimBaseClass* testFeature;
    SimObjectType* testObject;
    Tpoint org, vec, pos, fpos;
    float groundZ = 0.0;
    BOOL gotGround = FALSE;
    BOOL hitGround = FALSE;
    BOOL advanceQueue = FALSE;
    GunTracerType *bulptr;
    CampBaseClass* objective;
    int whatWasHit = TRACER_HIT_NOTHING;
    float yOffset;
    VuGridIterator* gridIt = NULL;

    //RV - I-Hawk - Added a 0 vector for RV new PS calls
    Tpoint PSvec;
    PSvec.x = 0;
    PSvec.y = 0;
    PSvec.z = 0;

#ifndef NODRAG
    float vt1, dragDelta, cosgam, singam, cosmu, sinmu, alpha;
#endif

    // Handle reloading if we have unlimited ammo
    if (unlimitedAmmo)
    {
        // Could make numRoundsRemaining a float, but this changed less code...
        fractionalRoundsRemaining += 0.1f * roundsPerSecond * SimLibMajorFrameTime;

        if (fractionalRoundsRemaining > 1.0f)
        {
            int inc = FloatToInt32(fractionalRoundsRemaining);
            numRoundsRemaining = min(initialRounds, numRoundsRemaining + inc);
            fractionalRoundsRemaining -= (float)inc;

            if (*fire and this->parent.get() == FalconLocalSession->GetPlayerEntity())
            {
                g_intellivibeData.BulletsFired++;
            }
        }
    }


    // if we're a shell type just update and return
    if (typeOfGun == GUN_SHELL)
    {
        if (shellTargetPtr)
        {
            UpdateShell();
        }

        return whatWasHit;
    }

    // TOTAL HACK HERE  We observe that sometimes bullet is NULL here (why?)
    // SCR 5-9-97
    if ( not bullet)
    {
        return whatWasHit;
    }

    // END OF HACK

    // edg: why go thru this exec stuff every time if there's no bullets
    // flying or the fire button isn't pressed?   Added numFlying counter
    // to determine bullets in air
    if ( not (*fire) and not numFlying)
        return whatWasHit;


    // scale the y start position if we're in the cockpit and ownship
    if (isOwnship and OTWDriver.DisplayInCockpit())
    {
        yOffset = yPos * 2.0f;
    }
    else
    {
        yOffset = yPos;
    }

    // see if we should advance the queue
    // this is determined by the # of bullets we're tracking.
    // we want to run them about 2 secs out from the muzzle.
    // so, if we have # bullets = half the frame rate, we want
    // to advance the q 4 * frame time to get about 2 secs
    qTimer += SimLibMajorFrameTime;

    // 2000-10-17 MODIFIED BY S.G. SO BULLETS STAY IN THE AIR LONGER (WE'LL USE AN UNUSED FIELD IN THE WCD FILE)
    // This var will take the first 3 bits of that field and use it for 1 to 8 seconds.
    // if ( qTimer >= SimLibMajorFrameTime * 4.0f )
    if (qTimer >= SimLibMajorFrameTime * (float)(((((unsigned char *)wcPtr)[45] bitand 7) + 1) * 2))
    {
        advanceQueue = TRUE;
        qTimer = 0.0f;
    }

    //START_PROFILE("GUNS EXEC");

    for (i = numTracers - 1; i >= 0; i--)
    {
        // see if we should move the bullets up towards end
        if (advanceQueue)
        {
            if (i == numTracers - 1 and bullet[i].flying)
                numFlying--;

            if (i > 0)
                bullet[i] = bullet[i - 1];
            else
                break;
        }

        // point to the bullet entry
        bulptr = &bullet[i];

        /*---------------------------*/
        /* Only check flying bullets */
        /*---------------------------*/
        if (bulptr->flying)
        {
            // 1st point is where bullet will end up
            vec.x = bulptr->xdot * SimLibMajorFrameTime;
            vec.y = bulptr->ydot * SimLibMajorFrameTime;
            vec.z = bulptr->zdot * SimLibMajorFrameTime;


            p1.x = bulptr->x + vec.x;
            p1.y = bulptr->y + vec.y;
            p1.z = bulptr->z + vec.z;

            // edg: hunh?  I think I wrote this.  I musta been on LSD...  However,
            // I think it's for collision detection purposes....
            // Since we run a SimLibFrameTime sec frame and there should be roundPerSecond
            // a bullet comes out very we need to extend the vector by roundsPerSecond*SimLibFrameTime
            // 2000-10-11 NOT TOUCHED BY S.G. BUT MAY BE HE WAS ON LSD, THIS MAKES NO SENSE
            vec.x *= roundsPerSecond * SimLibMajorFrameTime * 2.0F;
            vec.y *= roundsPerSecond * SimLibMajorFrameTime * 2.0F;
            vec.z *= roundsPerSecond * SimLibMajorFrameTime * 2.0F;

            // get ground level of furthest bullet
            if (gotGround == FALSE)
            {
                // if it's the player's plane that's firing, test ground
                // on every bullet by never setting gotGround to TRUE
                if ( not isOwnship)
                    gotGround = TRUE;

                groundZ = OTWDriver.GetApproxGroundLevel(p1.x, p1.y);

                if (p1.z - groundZ  > -1000.0f)
                    groundZ = OTWDriver.GetGroundLevel(p1.x, p1.y);
            }

            // get objective
            if (gridIt == NULL)
            {
#ifdef VU_GRID_TREE_Y_MAJOR
                gridIt = new VuGridIterator(ObjProxList, parent->YPos(), parent->XPos(), 2.5F * NM_TO_FT);
#else
                gridIt = new VuGridIterator(ObjProxList, parent->XPos(), parent->YPos(), 2.5F * NM_TO_FT);
#endif
            }

            // 2nd point is where bullet starts
            if (i == 0)
            {
                p2.x = parent->XPos();
                p2.y = parent->YPos();
                p2.z = parent->ZPos();

                p2.x += dmx[0][0] * xPos + dmx[1][0] * yPos + dmx[2][0] * zPos;
                p2.y += dmx[0][1] * xPos + dmx[1][1] * yPos + dmx[2][1] * zPos;
                p2.z += dmx[0][2] * xPos + dmx[1][2] * yPos + dmx[2][2] * zPos;
            }
            else
            {
                p2.x = bulptr->x;
                p2.y = bulptr->y;
                p2.z = bulptr->z;
            }

            p3.x = (float)fabs(vec.x);
            p3.y = (float)fabs(vec.y);
            p3.z = (float)fabs(vec.z);


            // Check this tracer against all objectives
            objective = (CampBaseClass*)gridIt->GetFirst();

            while (objective)
            {
                if ( not objective->GetComponents())
                {
                    objective = (CampBaseClass*)gridIt->GetNext();
                    continue;
                }

                // loop thru each element in the objective
                VuListIterator featureWalker(objective->GetComponents());
                testFeature = (SimBaseClass*) featureWalker.GetFirst();

                while (testFeature)
                {
                    if ( not testFeature->drawPointer)
                    {
                        testFeature = (SimBaseClass*) featureWalker.GetNext();
                        continue;
                    }

                    if (testFeature->IsSetCampaignFlag(FEAT_FLAT_CONTAINER))
                    {
                        testFeature = (SimBaseClass*) featureWalker.GetNext();
                        continue;
                    }

                    // get feature's radius -- store in vt to save another stack var
                    // 2000-11-17 MODIFIED BY S.G. NEED TO ADD THE SIZE OF THE 'blastRadius' TO THIS
                    //vt = testFeature->drawPointer->Radius();
                    vt = testFeature->drawPointer->Radius() + wcPtr->BlastRadius;
                    // END OF MODIFIED SECTION
                    testFeature->drawPointer->GetPosition(&fpos);

                    if (fabs(p1.x - fpos.x) < vt + p3.x and 
                        fabs(p1.y - fpos.y) < vt + p3.y and 
                        fabs(p1.z - fpos.z) < vt + p3.z)
                    {
                        org.x = p2.x;
                        org.y = p2.y;
                        org.z = p2.z;

                        if (testFeature->drawPointer->GetRayHit(&org, &vec, &pos, 1.0f))
                        {

                            // "back up" the hit point on features.  We want to
                            // be able to see them, otherwise they'll sort
                            // badly in draw
                            pos.x -= bulptr->xdot * 0.10f;
                            pos.y -= bulptr->ydot * 0.10f;
                            pos.z -= bulptr->zdot * 0.10f;

                            vec.x = PRANDFloat() * 60.0f;
                            vec.y = PRANDFloat() * 60.0f;
                            vec.z = -40.0f;

                            //RV - I-Hawk - PSFX_GUN_HIT_OBJECT is always defined, no need for this
                            //old stuff anymore... only a PS call

                            AddParticleEffect(PSFX_GUN_HIT_OBJECT, &pos, &vec);

                            //{
                            // // add bullet effect...
                            // switch( PRANDInt5() )
                            // {
                            // case 0:
                            // /*
                            // OTWDriver.AddSfxRequest(
                            // new SfxClass(SFX_SPARKS, // type
                            // &pos, // world pos
                            // 0.75f, // time to live
                            // 21.0f)); // scale
                            // */
                            // DrawableParticleSys::PS_AddParticleEx((SFX_SPARKS + 1),
                            // &pos,
                            // &PSvec);
                            // break;
                            // case 1:
                            // /*
                            // OTWDriver.AddSfxRequest(
                            // new SfxClass( SFX_FIRE4, // type
                            // SFX_MOVES bitor SFX_USES_GRAVITY bitor SFX_NO_DOWN_VECTOR,
                            // &pos, // world pos
                            // &vec, // vel vector
                            // 1.5, // time to live
                            // 15.0f ) ); // scale
                            // */
                            // DrawableParticleSys::PS_AddParticleEx((SFX_FIRE4 + 1),
                            // &pos,
                            // &vec);
                            // break;
                            // case 2:
                            // /*
                            // OTWDriver.AddSfxRequest(
                            // new SfxClass( SFX_FIRE5, // type
                            // SFX_MOVES bitor SFX_USES_GRAVITY bitor SFX_NO_DOWN_VECTOR,
                            // &pos, // world pos
                            // &vec, // vel vector
                            // 1.5, // time to live
                            // 15.0f ) ); // scale
                            // */
                            // DrawableParticleSys::PS_AddParticleEx((SFX_FIRE5 + 1),
                            // &pos,
                            // &vec);
                            // break;
                            // case 3:
                            // /*
                            // OTWDriver.AddSfxRequest(
                            // new SfxClass( SFX_DEBRISTRAIL, // type
                            // SFX_MOVES bitor SFX_USES_GRAVITY bitor SFX_BOUNCES,
                            // &pos, // world pos
                            // &vec, // vel vector
                            // 1.5, // time to live
                            // 15.0f ) ); // scale
                            // */
                            // DrawableParticleSys::PS_AddParticleEx((SFX_DEBRISTRAIL + 1),
                            // &pos,
                            // &vec);
                            // break;
                            // case 4:
                            // default:
                            // pos.x += bulptr->xdot * 0.10f;
                            // pos.y += bulptr->ydot * 0.10f;
                            // pos.z += bulptr->zdot * 0.10f;
                            // /*
                            // OTWDriver.AddSfxRequest(
                            // new SfxClass( SFX_BILLOWING_SMOKE, // type
                            // &pos, // world pos
                            // 10 + PRANDInt3() * 5, // time to live
                            // 0.3f ) ); // scale
                            // */
                            // DrawableParticleSys::PS_AddParticleEx((SFX_BILLOWING_SMOKE + 1),
                            // &pos,
                            // &PSvec);
                            // break;
                            // }

                            // F4SoundFXSetPos( (PRANDInt3() == 1 ) ? SFX_GRNDHIT2 : SFX_GRNDHIT1,
                            // TRUE,
                            // bulptr->x, bulptr->y, bulptr->z, 1.0f);
                            //}

                            // VuTargetEntity* target= (VuTargetEntity*) vuDatabase->Find(testFeature->OwnerId());
                            message = new FalconDamageMessage(testFeature->Id(), FalconLocalGame);
                            message->dataBlock.fEntityID  = parent->Id();
                            message->dataBlock.fCampID    = parent->GetCampID();
                            message->dataBlock.fSide      = parent->GetCountry();
                            message->dataBlock.fPilotID   = ((SimMoverClass*)parent.get())->pilotSlot;
                            message->dataBlock.fIndex     = parent->Type();
                            message->dataBlock.fWeaponID  = Type();
                            message->dataBlock.fWeaponUID.num_ = GetCurrentBurst();

                            message->dataBlock.dEntityID  = testFeature->Id();
                            message->dataBlock.dCampID    = testFeature->GetCampID();
                            message->dataBlock.dSide      = testFeature->GetCountry();
                            message->dataBlock.dPilotID   = 255;
                            message->dataBlock.dIndex     = testFeature->Type();
                            message->dataBlock.damageType = FalconDamageType::BulletDamage;
                            message->dataBlock.damageStrength = 0;
                            message->dataBlock.damageRandomFact = PRANDFloatPos();
                            message->RequestOutOfBandTransmit();
                            FalconSendMessage(message, TRUE);

                            bulptr->flying = FALSE;
                            // kinda yucky, but efficient
                            goto FinishedFeatureTest;
                        }
                    }

                    testFeature = (SimBaseClass*) featureWalker.GetNext();
                }

                objective = (CampBaseClass*)gridIt->GetNext();
            }

        FinishedFeatureTest:

            if ( not bulptr->flying)
            {
                whatWasHit or_eq TRACER_HIT_FEATURE;
                numFlying--;
                continue;
            }

            // Objects
            testObject = targetList;

            while (testObject)
            {
                if
                (
                    testObject->BaseData() and testObject->BaseData()->IsSim() and 
                    ( not testObject->BaseData()->IsWeapon() or testObject->BaseData()->IsEject()) and 
 not ((SimBaseClass*)testObject->BaseData())->IsExploding() and 
                    testObject->localData and testObject->localData->range < initBulletVelocity * (2.5F) and 
                    ((SimBaseClass*)testObject->BaseData())->drawPointer not_eq NULL
                )
                {
                    // get feature's radius -- store in vt to save another stack var
                    // 2000-11-17 MODIFIED BY S.G. NEED TO ADD THE SIZE OF THE 'blastRadius' TO THIS
                    //    vt = ((SimBaseClass*)testObject->BaseData())->drawPointer->Radius();
                    vt = ((SimBaseClass*)testObject->BaseData())->drawPointer->Radius() + wcPtr->BlastRadius;
                    // END OF MODIFIED SECTION
                    ((SimBaseClass*)testObject->BaseData())->drawPointer->GetPosition(&fpos);

                    if (fabs(p1.x - fpos.x) < vt + p3.x and fabs(p1.y - fpos.y) < vt + p3.y and fabs(p1.z - fpos.z) < vt + p3.z)
                    {
                        // Back up 1/2 of the vector traveled.
                        org.x = p2.x - 0.5F * vec.x;
                        org.y = p2.y - 0.5F * vec.y;
                        org.z = p2.z - 0.5F * vec.z;

                        // init pos to something, not really necessary
                        pos = fpos;

                        // scale the bounding box by how close the object is -- the
                        // closer the smaller the box
                        vt = testObject->localData->range / (initBulletVelocity * 1.5f);
                        vt = max(1.0f, vt);

                        if (((SimBaseClass*)testObject->BaseData())->drawPointer->GetRayHit(&org, &vec, &pos, vt))
                        {
                            // add bullet effect...
                            // pos.x = p4.x;
                            // pos.y = p4.y;
                            // pos.z = p4.z;
                            vec.x = 0.0f;
                            vec.y = 0.0f;
                            vec.z = -15.0f;

                            //RV - I-Hawk - PSFX_GUN_HIT_OBJECT is always defined, no need for this
                            //old stuff anymore... only a PS call

                            AddParticleEffect(PSFX_GUN_HIT_OBJECT, &pos, &vec);

                            ///*
                            // OTWDriver.AddSfxRequest(
                            //  new SfxClass(SFX_TRAILSMOKE, // type
                            //  SFX_MOVES, // flags
                            //  &pos, // world pos
                            //  &vec, // vector
                            //  1.5f, // time to live
                            //  1.0f)); // scale
                            //  */
                            // DrawableParticleSys::PS_AddParticleEx((SFX_TRAILSMOKE + 1),
                            // &pos,
                            // &vec);
                            //
                            // /*
                            // OTWDriver.AddSfxRequest(
                            //  new SfxClass(SFX_SPARKS, // type
                            //  &pos, // world pos
                            //  0.75f, // time to live
                            //  11.0f)); // scale
                            //  */
                            // DrawableParticleSys::PS_AddParticleEx((SFX_SPARKS + 1),
                            // &pos,
                            // &PSvec);


                            // VuTargetEntity* target= (VuTargetEntity*) vuDatabase->Find(testObject->BaseData()->OwnerId());
                            message = new FalconDamageMessage(testObject->BaseData()->Id(), FalconLocalGame);
                            message->dataBlock.fEntityID  = parent->Id();
                            message->dataBlock.fCampID    = parent->GetCampID();
                            message->dataBlock.fSide      = parent->GetCountry();
                            message->dataBlock.fPilotID   = ((SimMoverClass*)parent.get())->pilotSlot;
                            message->dataBlock.fIndex     = parent->Type();
                            message->dataBlock.fWeaponID  = Type();
                            message->dataBlock.fWeaponUID.num_ = GetCurrentBurst();
                            message->dataBlock.dEntityID  = testObject->BaseData()->Id();
                            message->dataBlock.dCampID    = testObject->BaseData()->GetCampID();
                            message->dataBlock.dSide      = testObject->BaseData()->GetCountry();
                            message->dataBlock.dPilotID   = ((SimMoverClass*)testObject->BaseData())->pilotSlot;
                            message->dataBlock.dIndex     = testObject->BaseData()->Type();
                            message->dataBlock.damageType = FalconDamageType::BulletDamage;
                            message->dataBlock.damageRandomFact = PRANDFloatPos();
                            message->dataBlock.damageStrength = 0;
                            message->RequestOutOfBandTransmit();
                            FalconSendMessage(message, TRUE);
                            bulptr->flying = FALSE;
                        }
                    }
                }

                testObject = testObject->next;
            }

            if ( not bulptr->flying)
            {
                whatWasHit or_eq TRACER_HIT_UNIT;
                numFlying--;
                continue;
            }

            // Apply Drag;
#ifndef NODRAG
            vt     = (float)sqrt(bulptr->xdot * bulptr->xdot +
                                 bulptr->ydot * bulptr->ydot +
                                 bulptr->zdot * bulptr->zdot);
            vt1    = (float)sqrt(bulptr->xdot * bulptr->xdot +
                                 bulptr->ydot * bulptr->ydot);
            singam = -bulptr->zdot / vt;
            cosgam = vt1 / vt;
            cosmu  = bulptr->xdot / vt1;
            sinmu  = bulptr->ydot / vt1;
            alpha  = dragFactor * vt * vt;
            dragDelta = alpha * cosgam * SimLibMajorFrameTime;

            bulptr->xdot -= dragDelta * cosmu;
            bulptr->ydot -= dragDelta * sinmu;
            bulptr->zdot -= (alpha * singam) * SimLibMajorFrameTime;
#endif
            bulptr->zdot += GRAVITY * SimLibMajorFrameTime;

            //------------------------------------------------------//
            // see if bullet is within 1 time step of impacting the //
            // ground and linearly extrapolate to get impact point. //
            //------------------------------------------------------//
            if (parent->OnGround() and bulptr->z + bulptr->zdot * SimLibMajorFrameTime >= groundZ + 1500.0f)
            {
                // in this case the firing object is a ground vehicle.  We don't
                // really do any ground detect for them since they're too close
                // to the ground, just detect when the bullet has gone too far below
                // ground and stop it.
                bulptr->flying = FALSE;
                numFlying--;
            }
            else if ( not parent->OnGround() and bulptr->z + bulptr->zdot * SimLibMajorFrameTime >= groundZ)
            {
                // check to see if bullet already below ground
                if (bulptr->z > groundZ)
                {
                    // back up the bullet 1/2 frame as a guess
                    bulptr->x = bulptr->x - bulptr->xdot * SimLibMajorFrameTime * 0.5f;
                    bulptr->y = bulptr->y - bulptr->ydot * SimLibMajorFrameTime * 0.5f;
                }
                else
                {
                    timeToImpact = (bulptr->z - groundZ) / bulptr->zdot;
                    bulptr->x = bulptr->x + bulptr->xdot * timeToImpact;
                    bulptr->y = bulptr->y + bulptr->ydot * timeToImpact;
                }

                bulptr->z = groundZ;
                bulptr->flying = FALSE;
                numFlying--;
                whatWasHit or_eq TRACER_HIT_GROUND;

                // only do sound and smoke effect for lead bullet....
                // hack: seems like ground vehicles shoot into ground
                // for some reason -- don't place craters
                if (hitGround == FALSE and not parent->OnGround())
                {
                    if ( not isOwnship)
                        hitGround = TRUE;

                    pos.x = bulptr->x + PRANDFloat() * 8.0f;
                    pos.y = bulptr->y + PRANDFloat() * 8.0f;
                    pos.z = OTWDriver.GetGroundLevel(pos.x, pos.y) - 15.1f;

                    vec.x = 0.0f;
                    vec.y = 0.0f;
                    vec.z = -20.0f;

                    int groundType = OTWDriver.GetGroundType(pos.x, pos.y);

                    if ( not (groundType == COVERAGE_WATER or
                          groundType == COVERAGE_RIVER))
                    {
                        // MLR this effect creates the other effects by default
                        // but allows easy replacement with a particle object

                        //RV - I-Hawk - PSFX_GUN_HIT_OBJECT is always defined, no need for this
                        //old stuff anymore... only a PS cal

                        AddParticleEffect(PSFX_GUN_HIT_GROUND, &pos, &vec);

                        //{
                        // F4SoundFXSetPos( (PRANDInt3() == 1 ) ? SFX_GRNDHIT2 : SFX_GRNDHIT1,
                        // TRUE,
                        // bulptr->x, bulptr->y, bulptr->z, 1.0f);
                        //
                        // if ( not PRANDInt3() )
                        // {
                        // /*
                        // OTWDriver.AddSfxRequest(
                        // new SfxClass (SFX_GROUND_DUSTCLOUD, // type
                        // SFX_USES_GRAVITY bitor SFX_NO_DOWN_VECTOR bitor SFX_MOVES bitor SFX_NO_GROUND_CHECK,
                        // &pos, // world pos
                        // &vec,
                        // 1.0f, // time to live
                        // 8.5f )); // scale
                        // */
                        // DrawableParticleSys::PS_AddParticleEx((SFX_GROUND_DUSTCLOUD + 1),
                        // &pos,
                        // &vec);
                        //
                        // }
                        // else if ( not PRANDInt3() )
                        // {
                        // /*
                        // OTWDriver.AddSfxRequest(
                        // new SfxClass( SFX_FIRE4, // type
                        // SFX_MOVES bitor SFX_USES_GRAVITY bitor SFX_NO_DOWN_VECTOR,
                        // &pos, // world pos
                        // &vec, // vel vector
                        // 1.5, // time to live
                        // 15.0f ) ); // scale
                        // */
                        // DrawableParticleSys::PS_AddParticleEx((SFX_FIRE4 + 1),
                        // &pos,
                        // &vec);
                        // }
                        // else
                        // {
                        // /*
                        // OTWDriver.AddSfxRequest(
                        // new SfxClass(SFX_SPARKS, // type
                        // &pos, // world pos
                        // 0.75f, // time to live
                        // 21.0f)); // scale
                        // */
                        // DrawableParticleSys::PS_AddParticleEx((SFX_SPARKS + 1),
                        // &pos,
                        // &PSvec);
                        // }
                        //}

                    }
                    else // water
                    {
                        // MLR this effect creates the other effects by default
                        // but allows easy replacement with a particle object

                        //RV - I-Hawk - PSFX_GUN_HIT_WATER is always defined, no need for this
                        //old stuff anymore... only a PS call.

                        AddParticleEffect(PSFX_GUN_HIT_WATER, &pos, &vec);

                        //if( not AddParticleEffect(PSFX_GUN_HIT_WATER,&pos,&vec))
                        //{
                        // F4SoundFXSetPos( SFX_SPLASH,
                        // TRUE,
                        // bulptr->x, bulptr->y, bulptr->z, 1.0f);
                        //
                        // /*
                        // OTWDriver.AddSfxRequest(
                        // new SfxClass(SFX_WATER_STRIKE, // type
                        // &pos, // world pos
                        // 1.2f, // time to live
                        // 20.0f) ); // scale
                        // */
                        // DrawableParticleSys::PS_AddParticleEx((SFX_WATER_STRIKE + 1),
                        // &pos,
                        // &PSvec);
                        //}
                    }

                } // if hit ground
            } // if ground hit test passed
            else
            {
                // continue moving the bullet
                bulptr->x += bulptr->xdot * SimLibMajorFrameTime;
                bulptr->y += bulptr->ydot * SimLibMajorFrameTime;
                bulptr->z += bulptr->zdot * SimLibMajorFrameTime;
            }
        } // if bullet flying
    } // bullet loop

    if (numRoundsRemaining == 0)
        *fire = FALSE;

    // set pointer to 1st bullet
    bulptr = &bullet[0];

    // set position of muzzle tracers if we're firing -- only when alpha
    // blending is on
    if (*fire)  // and PlayerOptions.AlphaOn() )
    {
        float stagger;
        float ystagger;
        float zstagger;
        float xsize;
        float tmp;


        for (i = 0; i < numFirstTracers; i++)
        {

            if (i == 0)
            {
                ystagger = 0.0f;
                zstagger = 0.0f;
                muzzleAlpha[i] = 0.5f;
                muzzleStart++;
                muzzleStart and_eq 0x03;
                xsize = 0.1f;
                stagger = muzzleStart * 6.2f;
            }
            else
            {
                stagger =  NRANDPOS * 25.0f;
                xsize =  0.08f + NRANDPOS * 0.08f;

                ystagger = NRAND * 0.40f;

                if (ystagger <= 0.0f and ystagger > -0.20f)
                    ystagger = -0.20f;
                else if (ystagger >= 0.0f and ystagger < 0.20f)
                    ystagger = 0.20f;

                zstagger = NRAND * 0.40f;

                if (zstagger <= 0.0f and zstagger > -0.20f)
                    zstagger = -0.20f;
                else if (zstagger >= 0.0f and zstagger < 0.20f)
                    zstagger = 0.20f;

                // alpha falls off the further out the tracer gets
                // and also the further off from center
                muzzleAlpha[i] = 1.0f - stagger / 25.0f;
                muzzleAlpha[i] *= muzzleAlpha[i];
                muzzleAlpha[i] *= muzzleAlpha[i];
                tmp = 1.0f - (float)fabs(ystagger / 0.40f);
                tmp *= tmp;
                tmp *= tmp;

                if (tmp < muzzleAlpha[i])
                    muzzleAlpha[i] = tmp;

                tmp = 1.0f - (float)fabs(zstagger / 0.40f);
                tmp *= tmp;
                tmp *= tmp;

                if (tmp < muzzleAlpha[i])
                    muzzleAlpha[i] = tmp;
            }


            // width falls off the closer to center
            tmp = (float)fabs(ystagger / 0.40f);
            tmp *= tmp;
            tmp *= tmp;
            muzzleWidth[i] = tmp;
            tmp = (float)fabs(zstagger / 0.40f);
            tmp *= tmp;
            tmp *= tmp;

            if (tmp > muzzleWidth[i])
                muzzleWidth[i] = tmp;

            ystagger += yOffset;
            zstagger += zPos;

            muzzleLoc[i].x = parent->XPos();
            muzzleLoc[i].y = parent->YPos();
            muzzleLoc[i].z = parent->ZPos();

            muzzleLoc[i].x += dmx[0][0] * xPos * stagger + dmx[1][0] * ystagger + dmx[2][0] * zstagger;
            muzzleLoc[i].y += dmx[0][1] * xPos * stagger + dmx[1][1] * ystagger + dmx[2][1] * zstagger;
            muzzleLoc[i].z += dmx[0][2] * xPos * stagger + dmx[1][2] * ystagger + dmx[2][2] * zstagger;

            muzzleEnd[i].x = dmx[0][0] * initBulletVelocity + dmx[1][0] * ystagger + dmx[2][0] * zstagger;
            muzzleEnd[i].y = dmx[0][1] * initBulletVelocity + dmx[1][1] * ystagger + dmx[2][1] * zstagger;
            muzzleEnd[i].z = dmx[0][2] * initBulletVelocity + dmx[1][2] * ystagger + dmx[2][2] * zstagger;

            muzzleEnd[i].x = muzzleLoc[i].x + muzzleEnd[i].x * SimLibMajorFrameTime * xsize;
            muzzleEnd[i].y = muzzleLoc[i].y + muzzleEnd[i].y * SimLibMajorFrameTime * xsize;
            muzzleEnd[i].z = muzzleLoc[i].z + muzzleEnd[i].z * SimLibMajorFrameTime * xsize;
        }
    }

    // if firing, insert an new round into the queue at 1st position
    if (*fire and ( not bulptr->flying or advanceQueue))
    {
        bulptr->x = parent->XPos();
        bulptr->y = parent->YPos();
        bulptr->z = parent->ZPos();


        // add gunsmoke effect
        pos.x = bulptr->x;
        pos.y = bulptr->y;
        pos.z = bulptr->z;
        vec.x = 0.0f;
        vec.y = 0.0f;
        vec.z = -50.0f;

        if (parent->IsAirplane())
        {
            //RV - I-Hawk - RV new trails call changes
            //if ( not smokeTrail )
            if ( not TrailIdNew)
            {
                //smokeTrail = new DrawableTrail(trailID);
                TrailIdNew = trailID;
                //OTWDriver.InsertObject( smokeTrail );
            }

            pos.x += dmx[0][0] * xPos + dmx[1][0] * yOffset + dmx[2][0] * zPos;
            pos.y += dmx[0][1] * xPos + dmx[1][1] * yOffset + dmx[2][1] * zPos;
            pos.z += dmx[0][2] * xPos + dmx[1][2] * yOffset + dmx[2][2] * zPos;
            //OTWDriver.AddTrailHead( smokeTrail, pos.x, pos.y, pos.z );
            Trail = DrawableParticleSys::PS_EmitTrail(Trail, TrailIdNew, pos.x, pos.y, pos.z);

            //RV - I-Hawk - Add a gunfire PS at the gun location
            Tpoint gunPSvec;

            gunPSvec.x = parent->XDelta();
            gunPSvec.y = parent->YDelta();
            gunPSvec.z = parent->ZDelta();

            DrawableParticleSys::PS_AddParticleEx((SFX_GUNFIRE + 1),
                                                  &pos,
                                                  &gunPSvec);


        }

        if ( not bullet[1].flying)
        {
            fireCount ++;
            muzzleStart = 0;
        }

        bulptr->flying = fireCount;
        numFlying++;


        bulptr->x += dmx[0][0] * xPos * 30.0f + dmx[1][0] * yOffset + dmx[2][0] * zPos;
        bulptr->y += dmx[0][1] * xPos * 30.0f + dmx[1][1] * yOffset + dmx[2][1] * zPos;
        bulptr->z += dmx[0][2] * xPos * 30.0f + dmx[1][2] * yOffset + dmx[2][2] * zPos;

        // muzzleLoc.x = bulptr->x;
        // muzzleLoc.y = bulptr->y;
        // muzzleLoc.z = bulptr->z;

        /*--------------------------------------*/
        /* Dot terms are a combination of where */
        /* I'm going and where I'm pointing     */
        /*--------------------------------------*/

        // edg: it is observed that geomData for ground vehicles is (sometimes?)
        // invalid.  Since they don't move all that fast anyway, just use
        // bullet velocity for dot vals
        if ( not parent->OnGround() and parent->IsAirplane())
        {
            vt = parent->GetVt();
            bulptr->xdot = vt * geomData->cosgam * geomData->cossig + initBulletVelocity * dmx[0][0];
            bulptr->ydot = vt * geomData->cosgam * geomData->sinsig + initBulletVelocity * dmx[0][1];
            bulptr->zdot = -vt * geomData->singam + initBulletVelocity * dmx[0][2];
        }
        else
        {
            bulptr->xdot = initBulletVelocity * dmx[0][0];
            bulptr->ydot = initBulletVelocity * dmx[0][1];
            bulptr->zdot = initBulletVelocity * dmx[0][2];
        }

        bulptr->x += bulptr->xdot * SimLibMajorFrameTime * 0.5f;
        bulptr->y += bulptr->ydot * SimLibMajorFrameTime * 0.5f;
        bulptr->z += bulptr->zdot * SimLibMajorFrameTime * 0.5f;

        if (gACMIRec.IsRecording())
        {
            acmiTracer.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
            acmiTracer.data.x = bulptr->x;
            acmiTracer.data.y = bulptr->y;
            acmiTracer.data.z = bulptr->z;
            acmiTracer.data.dx = bulptr->xdot;
            acmiTracer.data.dy = bulptr->ydot;
            acmiTracer.data.dz = bulptr->zdot;
            gACMIRec.TracerRecord(&acmiTracer);

        }

        // just testing
        /*
        pos.x = parent->XPos();
        pos.y = parent->YPos();
        pos.z = parent->ZPos();
        vec.x = parent->XDelta();
        vec.y = parent->YDelta();
        vec.z = parent->ZDelta();

        // for the moment (at least), bullets only go in direction
        // object is pointing
        vec.x += dmx[0][0]*3000.0f;
        vec.y += dmx[0][1]*3000.0f;
        vec.z += dmx[0][2]*3000.0f;
        pos.x += vec.x*SimLibMajorFrameTime;
        pos.y += vec.y*SimLibMajorFrameTime;
        pos.z += vec.z*SimLibMajorFrameTime;

        OTWDriver.AddSfxRequest(
         new SfxClass(SFX_GUN_TRACER, // type
         SFX_MOVES bitor SFX_USES_GRAVITY, // flags
         &pos, // world pos
         &vec, // vector
         3.0f, // time to live
         1.0f)); // scale
        */

        // Could make numRoundsRemaining a float, but this changed less code...
        fractionalRoundsRemaining -= roundsPerSecond * SimLibMajorFrameTime;

        if (fractionalRoundsRemaining < 1.0f)
        {
            int inc = FloatToInt32(fractionalRoundsRemaining);
            numRoundsRemaining = max(0, numRoundsRemaining + inc);
            fractionalRoundsRemaining -= (float)inc;

            if (*fire and this->parent.get() == FalconLocalSession->GetPlayerEntity())
            {
                g_intellivibeData.BulletsFired++;
            }
        }
    }
    else if (*fire and not advanceQueue and bulptr->flying)
    {
        bulptr->x += bulptr->xdot * SimLibMajorFrameTime;
        bulptr->y += bulptr->ydot * SimLibMajorFrameTime;
        bulptr->z += bulptr->zdot * SimLibMajorFrameTime;

        // float rtmp = SimLibMajorFrameTime * 0.5f;
        // muzzleLoc.x = bulptr->x - bulptr->xdot * rtmp;
        // muzzleLoc.y = bulptr->y - bulptr->ydot * rtmp;
        // muzzleLoc.z = bulptr->z - bulptr->zdot * rtmp;

        if ( not unlimitedAmmo)
        {
            numRoundsRemaining =
                (int)(max(numRoundsRemaining - roundsPerSecond * SimLibMajorFrameTime, 0.0F));
        }

        //RV - I-Hawk - RV new trails call changes
        //if (smokeTrail)
        if (TrailIdNew)
        {
            pos.x = parent->XPos() + dmx[0][0] * xPos + dmx[1][0] * yOffset + dmx[2][0] * zPos;
            pos.y = parent->YPos() + dmx[0][1] * xPos + dmx[1][1] * yOffset + dmx[2][1] * zPos;
            pos.z = parent->ZPos() + dmx[0][2] * xPos + dmx[1][2] * yOffset + dmx[2][2] * zPos;
            /*OTWDriver.AddTrailHead( smokeTrail,
               pos.x,
               pos.y,
               pos.z );*/
            Trail = DrawableParticleSys::PS_EmitTrail(Trail, TrailIdNew, pos.x, pos.y, pos.z);
        }
    }
    else if ( not (*fire))
    {
        if (advanceQueue)
            bulptr->flying = FALSE;

        if (bulptr->flying)
        {
            bulptr->x += bulptr->xdot * SimLibMajorFrameTime;
            bulptr->y += bulptr->ydot * SimLibMajorFrameTime;
            bulptr->z += bulptr->zdot * SimLibMajorFrameTime;

            // float rtmp = SimLibMajorFrameTime * 0.5f;
            // muzzleLoc.x = bulptr->x - bulptr->xdot * rtmp;
            // muzzleLoc.y = bulptr->y - bulptr->ydot * rtmp;
            // muzzleLoc.z = bulptr->z - bulptr->zdot * rtmp;
        }

        //if (smokeTrail)
        //RV - I-Hawk - RV new trails call changes
        if (TrailIdNew)
        {
            /*OTWDriver.AddSfxRequest(
            new SfxClass ( 2.0f, // time to live
            smokeTrail ) );*/
            DrawableParticleSys::PS_KillTrail(Trail);
            //smokeTrail = NULL
            TrailIdNew = Trail = NULL;
        }
    }

    UpdateTracers(*fire);

    // Clean up the iterator if used
    if (gridIt)
        delete gridIt;

    //STOP_PROFILE("GUNS EXEC");


    return whatWasHit;
}

/*
** Name: BulletSphereHit
** Description:
** Check to see if a ray  from source's initial
** position (sp) to end position (ep) intersects target's bounding
** sphere (center at tc, radius r).  Returns distance to target if
**  ray hits sphere, less than 0 if not.
** The formula was gotten from Graphics Gems Vol1 - ray-sphere interset
*/
float
BulletSphereHit(vector *sp, vector *ep, vector *tc, float r, vector *impact)
{
    float dotp1, dotp2, d_squared, newdist;
    vector s_to_t;
    vector ray;
    float length;
    float ratio;


    /*
    ** Get vector from source to target
    */
    s_to_t.x = (tc->x - sp->x);
    s_to_t.y = (tc->y - sp->y);
    s_to_t.z = (tc->z - sp->z);

    // get vector of ray from source to end point
    ray.x = (ep->x - sp->x);
    ray.y = (ep->y - sp->y);
    ray.z = (ep->z - sp->z);

    /*
    ** dot the 2 vectors
    */
    dotp1 = (ray.x * s_to_t.x) + (ray.y * s_to_t.y) +
            (ray.z * s_to_t.z);

    /*
    ** If the dot product of the source to target vector with the
    ** source dir unit vector is less than 0, then the vectors are pointing
    ** in opposite directions.  We can quit right here.
    */
    if (dotp1 < 0.0)
        return(-1.0f);

    // normalize dotp1 with length of ray
    length = (float)sqrt(ray.x * ray.x + ray.y * ray.y + ray.z * ray.z);
    dotp1 /= length;

    dotp2 = (s_to_t.x * s_to_t.x) + (s_to_t.y * s_to_t.y) +
            (s_to_t.z * s_to_t.z);

    d_squared = (r * r) - (dotp2 - (dotp1 * dotp1));

    if (d_squared < 0.0)
        return(-1.0f);

    /*
    ** We should use the square root of d squared for the below calculation,
    ** but instead we'll just approximate with the radius.  The dotp1 minus
    ** radius should approximate the distance along the ray from the source
    ** point to the point of intersection.  If this value is > the dist
    ** traveled in the frame, the collision has fallen short
    */
    newdist = dotp1 - r;

    if (newdist < 0.0)
        newdist = 0.5;

    if (length < newdist)
        return(-1.0f);

    /*
    ** That's it, a hit
    ** set impact point and return dist
    */
    ratio = newdist / length;
    impact->x = sp->x + ray.x * ratio;
    impact->y = sp->y + ray.y * ratio;
    impact->z = sp->z + ray.z * ratio;

    return newdist;
}

/*
** Name: BulletBoxHit
** Description:
** Check to see if a ray  from source's initial
** position (sp) to end position (ep) intersects target's bounding
** sphere (center at tc, radius r).  Returns collision point and
**  TRUE or FALSE if hit .
** The formula was gotten from Graphics Gems Vol1 - ray-box interset
*/

