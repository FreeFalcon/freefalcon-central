#include "stdhdr.h"
#include "mesg.h"
#include "otwdrive.h"
#include "initdata.h"
#include "waypoint.h"
#include "f4error.h"
#include "object.h"
#include "simobj.h"
#include "simdrive.h"
#include "Graphics/Include/drawsgmt.h"
#include "Graphics/Include/drawgrnd.h"
#include "Graphics/Include/drawguys.h"
#include "Graphics/Include/drawbsp.h"
#include "entity.h"
#include "classtbl.h"
#include "sms.h"
#include "fcc.h"
#include "PilotInputs.h"
#include "MsgInc/DamageMsg.h"
#include "guns.h"
#include "hardpnt.h"
#include "campwp.h"
#include "sfx.h"
#include "Unit.h"
#include "fsound.h"
#include "soundfx.h"
#include "fakerand.h"
#include "acmi/src/include/acmirec.h"
#include "ui/include/ui_ia.h"
#include "gndunit.h"
#include "radar.h"
#include "handoff.h"
#include "ground.h"
#include "Team.h"
#include "dofsnswitches.h"
#include "profiler.h"
#include "Graphics/Include/drawparticlesys.h"
/* 2001-03-21 S.G. */
#include "atm.h"

//JAM 24Nov03
#include "weather.h"
#include "AIInput.h"
#ifdef USE_SH_POOLS
MEM_POOL GroundClass::pool;
#endif

void CalcTransformMatrix(SimBaseClass* theObject);
void Trigenometry(SimMoverClass *platform);
void SetLabel(SimBaseClass* theObject);
GNDAIClass *NewGroundAI(GroundClass *us, int position, BOOL isFirst, int skill);

extern bool g_bSAM2D3DHandover;

#define MAX_NCTR_RANGE    (60.0F * NM_TO_FT) // 2002-02-12 S.G. See RadarDoppler.h

//// RV - Biker - Maybe better to have the tractors as define
//#define F4_GENERIC_US_TRUCK_TYPE_SMALL    534 // HMMWV
//#define F4_GENERIC_US_TRUCK_TYPE_LARGE    534 // 548 //M997
//#define F4_GENERIC_US_TRUCK_TYPE_TRAILER  534 // 548 //TBD
//
//// RV - Biker - Maybe better to have the tractors as define
//#define F4_GENERIC_OPFOR_TRUCK_TYPE_SMALL    707 //KrAz T 255B
//#define F4_GENERIC_OPFOR_TRUCK_TYPE_LARGE    835 //1394 //Ural
//#define F4_GENERIC_OPFOR_TRUCK_TYPE_TRAILER  835 //ZIL-135

// sfr: mem pool stuff
//VuMemPool<GroundClass> GroundClass::grdPool(1000);

// Re-written by KCK 2/11/98
void GroundClass::SetupGNDAI(SimInitDataClass *idata)
{
    BOOL isFirst = (idata->vehicleInUnit == 0);
    int position = idata->campSlot * 3 + idata->inSlot;
    int skill = idata->skill;

    gai = NewGroundAI(this, position, isFirst, skill);
}

GroundClass::GroundClass(VU_BYTE** stream, long *rem) : SimVehicleClass(stream, rem)
{
    InitLocalData();
}

GroundClass::GroundClass(FILE* filePtr) : SimVehicleClass(filePtr)
{
    InitLocalData();
}

GroundClass::GroundClass(int type) : SimVehicleClass(type)
{
    InitLocalData();
}

void GroundClass::InitData()
{
    SimVehicleClass::InitData();
    InitLocalData();
}

void GroundClass::InitLocalData()
{
    // KCK: These were getting reset to the leader's each frame, which is moronic.
    // Just set constantly here.
    // timer for target processing
    processRate = 2 * SEC_TO_MSEC;
    // timer for movement decisions and firing processing
    thoughtRate = 6 * SEC_TO_MSEC;

    // Set last values to now
    lastProcess = SimLibElapsedTime;
    lastThought = SimLibElapsedTime;
    nextSamFireTime = SimLibElapsedTime;
    // 2002-02-26 ADDED BY S.G. RadarDigi Exec interval (used to be per frame).
    nextTargetUpdate = SimLibElapsedTime;
    allowSamFire = TRUE;
    truckDrawable = NULL;
    Sms = NULL;
    gai = NULL;
    battalionFireControl = NULL;
}

GroundClass::~GroundClass()
{
    CleanupLocalData();
}

void GroundClass::CleanupLocalData()
{
    if (gai)
    {
        delete gai;
        gai = NULL;
    }

    if (Sms)
    {
        delete Sms;
        Sms = NULL;
    }
}

void GroundClass::CleanupData()
{
    CleanupLocalData();
    SimVehicleClass::CleanupData();
}

void GroundClass::Init(SimInitDataClass* initData)
{
    SimVehicleClass::Init(initData);

    float nextX, nextY;
    float range, velocity;
    float wp1X, wp1Y, wp1Z;
    float wp2X, wp2Y, wp2Z;
    int i;
    WayPointClass* atWaypoint;
    mlTrig trig;
    VehicleClassDataType* vc;

    vc = GetVehicleClassData(Type() - VU_LAST_ENTITY_TYPE);

    // dustTrail = new DrawableTrail(TRAIL_DUST);
    isFootSquad = FALSE;
    isEmitter = FALSE;
    needKeepAlive = FALSE;

    hasCrew = (vc->Flags bitand VEH_HAS_CREW) ? TRUE : FALSE;
    isTowed = (vc->Flags bitand VEH_IS_TOWED) ? TRUE : FALSE;
    isShip = (GetDomain() == DOMAIN_SEA) ? TRUE : FALSE;

    // RV - Biker
    radarDown = false;

    // check for radar emitter
    if (vc->RadarType not_eq RDR_NO_RADAR)
    {
        isEmitter = TRUE;
    }

    // 2002-01-20 ADDED BY S.G. At time of creation,
    // the radar will take the mode of the battalion instead of being
    // off until it finds a target by itself (and can it find it if its radar is off).
    // SimVehicleClass::Init created the radar so it's safe to do it here...
    if (isEmitter)
    {
        if (GetCampaignObject()->GetRadarMode() not_eq FEC_RADAR_OFF)
        {
            RadarClass *radar = NULL;
            radar = (RadarClass*)FindSensor(this, SensorClass::Radar);
            ShiAssert(radar);
            radar->SetEmitting(TRUE);


            // 2002-04-22 MN last fix for FalconSP3 -
            //this was a good intention to keep a 2D target targetted by a deaggregating unit,
            // however - it doesn't work this way.
            //The campaign air target derived falconentity does not correlate with the deaggregated aircraft.
            // The SAM's radars would stay stuck at TRACK S1 or
            //TRACK S3 and won't engage. Symptom was the not changing range to the target (.label 4)
            // Now with this code removed, SAMs should work correctly again.
            //As we have large SAM bubble sizes - it doesn't really matter if we
            // need to go through all search states in the SIM again -
            //because SAM's are faster in GUIDE mode than in maximum missile range.
            if (g_bSAM2D3DHandover)
            {
                // 2002-03-21 ADDED BY S.G. In addition, we need to set our radar's target RFN
                //(right f*cking now) and run a sensor sweep on it so it's valid by the
                //time TargetProcessing is called.
                FalconEntity *campTargetEntity = ((UnitClass *)GetCampaignObject())->GetAirTarget();

                if (campTargetEntity)
                {
                    SetTarget(new SimObjectType(campTargetEntity));
                    CalcRelAzElRangeAta(this, targetPtr);
                    radar->SetDesiredTarget(targetPtr);
                    radar->SetFlag(RadarClass::FirstSweep);
                    radar->Exec(targetList);
                }
            }
        }
    }

    SetFlag(ON_GROUND);
    SetPowerOutput(1.0F); // Assume our motor is running all the time

    SetPosition(initData->x, initData->y, OTWDriver.GetGroundLevel(initData->x, initData->y));
    SetYPR(initData->heading, 0.0F, 0.0F);

    SetupGNDAI(initData);

    if (initData->ptIndex)
    {
        // Don't move if we've got an assigned point
        gai->moveState = GNDAI_MOVE_HALTED;
        gai->moveFlags or_eq GNDAI_MOVE_FIXED_POSITIONS;
    }

    CalcTransformMatrix(this);

    strength        = 100.0F;

    // Check for Campaign mode
    // we don't follow waypoints here
    switch (gai->moveState)
    {
        case GNDAI_MOVE_GENERAL:
        {
            waypoint = curWaypoint = NULL;
            numWaypoints = 0;
            DeleteWPList(initData->waypointList);
            InitFromCampaignUnit();
        }
        break;

        case GNDAI_MOVE_WAYPOINT:
        {
            waypoint        = initData->waypointList;
            numWaypoints    = initData->numWaypoints;
            curWaypoint     = waypoint;

            if (curWaypoint)
            {
                // Corrent initial heading/velocity
                // Find the waypoint to go to.
                atWaypoint = curWaypoint;

                for (i = 0; i < initData->currentWaypoint; i++)
                {
                    atWaypoint = curWaypoint;
                    curWaypoint = curWaypoint->GetNextWP();
                }

                // If current is the on we're at, set for the next one.
                if (curWaypoint == atWaypoint)
                    curWaypoint = curWaypoint->GetNextWP();

                atWaypoint->GetLocation(&wp1X, &wp1Y, &wp1Z);

                if (curWaypoint == NULL)
                {
                    wp1X = initData->x;
                    wp1Y = initData->y;
                    curWaypoint = atWaypoint;
                    SetDelta(0.0F, 0.0F, 0.0F);
                    SetYPRDelta(0.0F, 0.0F, 0.0F);
                }
                else
                {
                    curWaypoint->GetLocation(&wp2X, &wp2Y, &wp2Z);

                    SetYPR((float)atan2(wp2Y - wp1Y, wp2X - wp1X), 0.0F, 0.0F);

                    nextX = wp2X;
                    nextY = wp2Y;

                    range = (float)sqrt((wp1X - nextX) * (wp1X - nextX) + (wp1Y - nextY) * (wp1Y - nextY));
                    velocity = range / ((curWaypoint->GetWPArrivalTime() - SimLibElapsedTime) / SEC_TO_MSEC);

                    if ((curWaypoint->GetWPArrivalTime() - SimLibElapsedTime) < 1 * SEC_TO_MSEC)
                        velocity = 0.0F;

                    // sfr: no need for this anymore
                    //SetVt(velocity);
                    //SetKias(velocity * FTPSEC_TO_KNOTS);
                    mlSinCos(&trig, Yaw());
                    SetDelta(velocity * trig.cos, velocity * trig.sin, 0.0F);
                    SetYPRDelta(0.0F, 0.0F, 0.0F);
                }
            }
        }
        break;

        default:
        {
            SetDelta(0.0F, 0.0F, 0.0F);
            // sfr: no need for this anymore
            //SetVt(0.0F);
            //SetKias(0.0F);
            SetYPRDelta(0.0F, 0.0F, 0.0F);
            gai->moveState = GNDAI_MOVE_HALTED;
            waypoint = curWaypoint = NULL;
            numWaypoints = 0;
            DeleteWPList(initData->waypointList);
            InitFromCampaignUnit();
        }
        break;
    }

    theInputs   = new PilotInputs;

    // Create our SMS
    Sms = new SMSBaseClass(this, initData->weapon, initData->weapons);

    uchar dam[10] = {100};

    for (i = 0; i < 10; i++)
    {
        dam[i] = 100;
    }

    Sms->SelectBestWeapon(dam, LowAir, -1);

    if (Sms->CurHardpoint() not_eq -1)
    {
        isAirCapable = TRUE;
    }
    else
    {
        isAirCapable = FALSE;
    }

    Sms->SelectBestWeapon(dam, NoMove, -1);

    if (Sms->CurHardpoint() not_eq -1)
    {
        isGroundCapable = TRUE;
    }
    else
    {
        isGroundCapable = FALSE;
    }

    Sms->SetCurHardpoint(-1);

    if ((GetType() == TYPE_WHEELED and GetSType() == STYPE_WHEELED_AIR_DEFENSE) or
        (GetType() == TYPE_WHEELED and GetSType() == STYPE_WHEELED_AAA) or
        (GetType() == TYPE_TRACKED and GetSType() == STYPE_TRACKED_AIR_DEFENSE) or
        (GetType() == TYPE_TRACKED and GetSType() == STYPE_TRACKED_AAA) or
        (GetType() == TYPE_TOWED and GetSType() == STYPE_TOWED_AAA))
    {
        isAirDefense = TRUE;
        // If we're an airdefense thingy, elevate our gun, and point in a random direction
        SetDOF(AIRDEF_ELEV, 60.0f * DTR);
        SetDOF(AIRDEF_ELEV2, 60.0f * DTR);
        SetDOF(AIRDEF_AZIMUTH, 180.0F * DTR - rand() / (float)RAND_MAX * 360.0F * DTR);
    }
    else
    {
        isAirDefense = FALSE;
    }
}

/*
** GroundClass Exec() function.
** NOTE: returns TRUE if we've processed this frame.  FALSE if we're to do
** dead reckoning (in VU)
*/
int GroundClass::Exec(void)
{
    //RV - I-Hawk - Added a 0 vector for RV new PS calls
    Tpoint PSvec;
    PSvec.x = 0;
    PSvec.y = 0;
    PSvec.z = 0;

    Tpoint pos;
    Tpoint vec;
    float speedScale;
    float groundZ;
    float labelLOD;
    float drawLOD;
    RadarClass *radar = NULL;

    SoundPos.UpdatePos((SimBaseClass *)this);

    //Cobra
    pos.x = 0.0f;
    pos.y = 0.0f;
    pos.z = 0.0f;

    // MLR 5/23/2004 -
    pos.x = XPos();
    pos.y = YPos();
    pos.z = OTWDriver.GetApproxGroundLevel(pos.x, pos.y);
    // pos.z = -10.0f;//Cobra trying to fix the stupid uninit CTD
    SetPosition(pos.x, pos.y, pos.z);

    // dead? -- we do nothing
    if (IsDead())
    {
        return FALSE;
    }

    // if damaged
    if (pctStrength < 0.5f)
    {
        if (sfxTimer > 1.5f - gai->distLOD * 1.3)
        {
            // reset the timer
            sfxTimer = 0.0f;
            pos.z -= 10.0f;

            // VP_changes this shoud be checked why have GetGroundLevel been subtracted by 10.0F
            // Sometimes the trails seem strange
            vec.x = PRANDFloat() * 20.0f;
            vec.y = PRANDFloat() * 20.0f;
            vec.z = PRANDFloat() * 20.0f;

            /*
            OTWDriver.AddSfxRequest(
             new SfxClass(
             SFX_TRAILSMOKE, // type
             SFX_MOVES bitor SFX_NO_GROUND_CHECK, // flags
             &pos, // world pos
             &vec, // vector
             3.5f, // time to live
             4.5f // scale
             )
            );
             */
            DrawableParticleSys::PS_AddParticleEx((SFX_TRAILSMOKE + 1),
                                                  &pos,
                                                  &vec);

        }
    }

    if (IsExploding())
    {
        // KCK: I've never seen this section of code executed. Maybe it gets hit, but I doubt
        // it.
        if ( not IsSetFlag(SHOW_EXPLOSION))
        {
            // Show the explosion
            Tpoint pos, vec;
            Falcon4EntityClassType *classPtr = (Falcon4EntityClassType *)EntityType();
            //DrawableGroundVehicle *destroyedPtr; // FRB

            //Cobra TJL 11/07/04 CTD point initialize here
            pos.x = 0.0f;
            pos.y = 0.0f;
            pos.z = 0.0f;

            // MLR 5/23/2004 - uncommented out the x, y
            pos.x = XPos();
            pos.y = YPos();
            pos.z = OTWDriver.GetApproxGroundLevel(pos.x, pos.y) - 10.0f;

            vec.x = 0.0f;
            vec.y = 0.0f;
            vec.z = 0.0f;

            // create a new drawable for destroyed vehicle
            // sometimes.....

            //RV - I-Hawk - Commenting all this if statement... not necessary

            /*
            if ( rand() bitand 1 ){
             destroyedPtr = new DrawableGroundVehicle(
             classPtr->visType[3],
             &pos,
             Yaw(),
             1.0f
             );

             groundZ = PRANDFloatPos() * 60.0f + 15.0f;

             /*
             OTWDriver.AddSfxRequest(
             new SfxClass (
             SFX_BURNING_PART, // type
             &pos, // world pos
             &vec, //
             (DrawableBSP *)destroyedPtr,
             groundZ, // time to live
             1.0f  // scale
             )
             );
             */
            /*
             DrawableParticleSys::PS_AddParticleEx((SFX_BURNING_PART + 1),
             &pos,
             &vec);


             pos.z += 10.0f;
             /*
             OTWDriver.AddSfxRequest(
             new SfxClass(
             SFX_FEATURE_EXPLOSION, // type
             &pos, // world pos
             groundZ, // time to live
             100.0f  // scale
             )
             );
             */
            /*
             DrawableParticleSys::PS_AddParticleEx((SFX_FEATURE_EXPLOSION + 1),
             &pos,
             &PSvec);

            }
            */
            //RV - I-Hawk - seperating explosion type for ground/sea domains. also
            //adding a check so soldiers will not explode like ground vehicles...

            if (GetDomain() == DOMAIN_LAND and GetType() not_eq TYPE_FOOT)
            {
                //pos.z -= 20.0f;
                /*
                OTWDriver.AddSfxRequest(
                 new SfxClass(
                 SFX_VEHICLE_EXPLOSION, // type
                 &pos, // world pos
                 1.5f, // time to live
                 100.0f  // scale
                 )
                );
                 */
                DrawableParticleSys::PS_AddParticleEx((SFX_VEHICLE_EXPLOSION + 1),
                                                      &pos,
                                                      &PSvec);
            }
            else if (GetDomain() == DOMAIN_SEA)
            {
                DrawableParticleSys::PS_AddParticleEx((SFX_WATER_FIREBALL + 1),
                                                      &pos,
                                                      &PSvec);
            }

            // make sure we don't do it again...
            SetFlag(SHOW_EXPLOSION);

            // we can now kill it immediately
            SetDead(TRUE);
        }

        return FALSE;
    }

    // exec any base functionality
    SimVehicleClass::Exec();

    // Sept 30, 2002
    // VP_changes: Frequently Z value is not in the correct place. It should follow the terrain.
    if (drawPointer)
    {
        drawPointer->GetPosition(&pos);
    }
    else
    {
        return FALSE;
    }

    //JAM 27Sep03 - Let's try this
    groundZ = pos.z; // - 0.7f; KCK: WTF is this?

    //VP_changes Sept 25
    groundZ = OTWDriver.GetGroundLevel(pos.x, pos.y);


    // Movement/Targeting for local entities
    if (IsLocal() and SimDriver.MotionOn())
    {
        //I commented this out, because it is done in gai->ProcessTargeting down below DSP 4/30/99
        // Refresh our target pointer (if any)
        //SetTarget( SimCampHandoff( targetPtr, targetList, HANDOFF_RANDOM ) );
        // Look for someone to do radar fire control for us
        FindBattalionFireControl();

        // RV - Biker - Switch on lights for ground/naval vehicles
        int isNight = TimeOfDayGeneral(TheCampaign.CurrentTime) < TOD_DAWNDUSK ? true : false;

        if (drawPointer and ((DrawableBSP *)drawPointer)->GetNumSwitches() >= AIRDEF_LIGHT_SWITCH)
        {
            if (isShip)
            {
                isNight = (TimeOfDayGeneral(TheCampaign.CurrentTime) <= TOD_DAWNDUSK or realWeather->weatherCondition == INCLEMENT) ? true : false;

                if (pctStrength > 0.50f)
                {
                    ((DrawableBSP *)drawPointer)->SetSwitchMask(0, isNight);
                    ((DrawableBSP *)drawPointer)->SetSwitchMask(AIRDEF_LIGHT_SWITCH, isNight);
                }
            }
            else if (GetVt() > 1.0f)
            {
                VuListIterator vehicleWalker(SimDriver.combinedList);
                FalconEntity* object = (FalconEntity*)vehicleWalker.GetFirst();
                bool hasThreat = false;
                float range = 999.9f * NM_TO_FT;

                // Consider each potential target in our environment
                while (object and not hasThreat)
                {
                    // Skip sleeping sim objects
                    if (object->IsSim())
                    {
                        if ( not ((SimBaseClass*)object)->IsAwake())
                        {
                            object = (FalconEntity*)vehicleWalker.GetNext();
                            continue;
                        }
                    }

                    // Fow now we skip missles -- might want to display them eventually...
                    if (object->IsMissile() or object->IsBomb())
                    {
                        object = (FalconEntity*)vehicleWalker.GetNext();
                        continue;
                    }

                    if (object->GetTeam() == GetTeam())
                    {
                        object = (FalconEntity*)vehicleWalker.GetNext();
                        continue;
                    }

                    float dx = object->XPos() - XPos();
                    float dy = object->YPos() - YPos();
                    float dz = object->ZPos() - ZPos();

                    range = (float)sqrt(dx * dx + dy * dy + dz * dz);

                    if (range < 5.0f * NM_TO_FT)
                        hasThreat = true;

                    object = (FalconEntity*)vehicleWalker.GetNext();
                }

                // If no enemy nearby and not heavy damaged switch on lights
                if ( not hasThreat and pctStrength > 0.75f)
                {
                    ((DrawableBSP *)drawPointer)->SetSwitchMask(AIRDEF_LIGHT_SWITCH, isNight);
                }
                else
                {
                    ((DrawableBSP *)drawPointer)->SetSwitchMask(AIRDEF_LIGHT_SWITCH, 0);
                }
            }
            else
            {
                ((DrawableBSP *)drawPointer)->SetSwitchMask(AIRDEF_LIGHT_SWITCH, 0);
            }
        }

        // RV - Biker - Do also switch on lights for tractor vehicles
        if (truckDrawable and truckDrawable->GetNumSwitches() >= AIRDEF_LIGHT_SWITCH)
        {
            if (GetVt() > 1.0f)
            {
                VuListIterator vehicleWalker(SimDriver.combinedList);
                FalconEntity* object = (FalconEntity*)vehicleWalker.GetFirst();
                bool hasThreat = false;
                float range = 999.9f * NM_TO_FT;

                // Consider each potential target in our environment
                while (object and not hasThreat)
                {
                    // Skip sleeping sim objects
                    if (object->IsSim())
                    {
                        if ( not ((SimBaseClass*)object)->IsAwake())
                        {
                            object = (FalconEntity*)vehicleWalker.GetNext();
                            continue;
                        }
                    }

                    // Fow now we skip missles -- might want to display them eventually...
                    if (object->IsMissile() or object->IsBomb())
                    {
                        object = (FalconEntity*)vehicleWalker.GetNext();
                        continue;
                    }

                    if (object->GetTeam() == GetTeam())
                    {
                        object = (FalconEntity*)vehicleWalker.GetNext();
                        continue;
                    }

                    float dx = object->XPos() - XPos();
                    float dy = object->YPos() - YPos();
                    float dz = object->ZPos() - ZPos();

                    range = (float)sqrt(dx * dx + dy * dy + dz * dz);

                    if (range < 5.0f * NM_TO_FT)
                        hasThreat = true;

                    object = (FalconEntity*)vehicleWalker.GetNext();
                }

                // If no enemy nearby and not heavy damaged switch on lights
                if ( not hasThreat and pctStrength > 0.75f)
                {
                    truckDrawable->SetSwitchMask(AIRDEF_LIGHT_SWITCH, isNight);
                }
                else
                {
                    truckDrawable->SetSwitchMask(AIRDEF_LIGHT_SWITCH, 0);
                }
            }
            else
            {
                truckDrawable->SetSwitchMask(AIRDEF_LIGHT_SWITCH, 0);
            }
        }

        // RV - Biker - Shut down ship radar if damaged
        if (isShip and radarDown == false and pctStrength < 0.9f and rand() % 50 > (pctStrength - 0.50f) * 100)
        {
            isEmitter = false;
            RadarClass *radar = (RadarClass*)FindSensor(this, SensorClass::Radar);

            if (radar)
            {
                radarDown = true;
                radar->SetDesiredTarget(NULL);
                radar->SetEmitting(FALSE);
            }

            if (targetPtr)
            {
                SelectWeapon(true);
            }
        }

        // 2001-03-26 ADDED BY S.G. NEED TO KNOW IF THE RADAR CALLED SetSpotted
        // RV - Biker - Rotate radars
        float deltaDOF;
        float curDOF = GetDOFValue(5);

        deltaDOF = 180.0f * DTR * SimLibMajorFrameTime;
        curDOF += deltaDOF;

        if (curDOF > 360.0f * DTR)
            curDOF -= 360.0f * DTR;

        SetDOF(5, curDOF);
        int spottedSet = FALSE;
        // END OF ADDED SECTION

        // 2002-03-21 ADDED BY S.G.
        // If localData only has zeros,
        // there is a good chance they are not valid (should not happen here though)...
        if (targetPtr)
        {
            SimObjectLocalData* localData = targetPtr->localData;

            if (
                localData->ataFrom == 0.0f and 
                localData->az == 0.0f  and 
                localData->el == 0.0f and 
                localData->range == 0.0f
            )
            {
                CalcRelAzElRangeAta(this, targetPtr);
            }
        }

        // END OF ADDED SECTION 2002-03-21

        // check for sending radar emmisions
        // 2002-02-26 MODIFIED BY S.G.
        // Added the nextTargetUpdate check to prevent the radar code to run on every frame
        if (isEmitter and nextTargetUpdate < SimLibElapsedTime)
        {
            // 2002-02-26 ADDED BY S.G. Next radar scan is 1 sec for aces, 2 for vets, etc ...
            nextTargetUpdate = SimLibElapsedTime + (5 - gai->skillLevel) * SEC_TO_MSEC;

            radar = (RadarClass*)FindSensor(this, SensorClass::Radar);
            ShiAssert(radar);

            if (radar)
            {
                radar->Exec(targetList);
            }

            // 2001-03-26 ADDED BY S.G.
            // IF WE CAN SEE THE RADAR'S TARGET AND WE ARE A AIR DEFENSE THINGY
            // NOT IN A BKOGEN MORAL STATE, MARK IT AS SPOTTED IF WE'RE BRIGHT ENOUGH
            if (
                radar and 
                radar->CurrentTarget() and 
                gai->skillLevel >= 3 and 
                ((UnitClass *)GetCampaignObject())->GetSType() == STYPE_UNIT_AIR_DEFENSE and 
 not ((UnitClass *)GetCampaignObject())->Broken()
            )
            {
                CampBaseClass *campBaseObj;

                if (radar->CurrentTarget()->BaseData()->IsSim())
                {
                    campBaseObj = ((SimBaseClass *)radar->CurrentTarget()->BaseData())->GetCampaignObject();
                }
                else
                {
                    campBaseObj = (CampBaseClass *)radar->CurrentTarget()->BaseData();
                }

                // JB 011002 If campBaseObj is NULL the target may be chaff
                if (campBaseObj and not (campBaseObj->GetSpotted(GetTeam())) and campBaseObj->IsFlight())
                {
                    RequestIntercept((FlightClass *)campBaseObj, GetTeam());
                }

                spottedSet = TRUE;

                if (campBaseObj and radar->GetRadarDatFile())
                {
                    campBaseObj->SetSpotted(
                        GetTeam(), TheCampaign.CurrentTime,
                        (radar->radarData->flag bitand RAD_NCTR) not_eq 0 and 
                        radar->CurrentTarget()->localData and 
                        radar->CurrentTarget()->localData->ataFrom < 45.0f * DTR and 
                        radar->CurrentTarget()->localData->range <
                        radar->GetRadarDatFile()->MaxNctrRange / (2.0f * (16.0f - (float)gai->skillLevel) / 16.0f)
                    );
                }

                // 2002-03-05 MODIFIED BY S.G. target's aspect and skill used in the equation
            }

            // END OF ADDED SECTION
        }

        // 2001-03-26 ADDED BY S.G.
        // IF THE BATTALION LEAD HAS LOS
        // ON IT AND WE ARE A AIR DEFENSE THINGY NOT IN A BKOGEN MORAL STATE,
        // MARK IT AS SPOTTED IF WE'RE BRIGHT ENOUGH
        // 2002-02-11 MODIFED BY S.G.
        // Since I only identify visually, need to perform this even if spotted by radar in case I can ID it.
        if (
            /* not spottedSet and gai->skillLevel >= 3 and */
            ((UnitClass *)GetCampaignObject())->GetSType() == STYPE_UNIT_AIR_DEFENSE and 
            gai == gai->battalionCommand and 
 not ((UnitClass *)GetCampaignObject())->Broken() and 
            gai->GetAirTargetPtr() and 
            CheckLOS(gai->GetAirTargetPtr())
        )
        {
            CampBaseClass *campBaseObj;

            if (gai->GetAirTargetPtr()->BaseData()->IsSim())
                campBaseObj = ((SimBaseClass *)gai->GetAirTargetPtr()->BaseData())->GetCampaignObject();
            else
                campBaseObj = (CampBaseClass *)gai->GetAirTargetPtr()->BaseData();

            // JB 011002 If campBaseObj is NULL the target may be chaff

            if ( not spottedSet and campBaseObj and not (campBaseObj->GetSpotted(GetTeam())) and campBaseObj->IsFlight())
                RequestIntercept((FlightClass *)campBaseObj, GetTeam());

            if (campBaseObj)
                campBaseObj->SetSpotted(GetTeam(), TheCampaign.CurrentTime, 1);

            // 2002-02-11 MODIFIED BY S.G. Visual detection means identified as well
        }

        // END OF ADDED SECTION

        // KCK: When should we run a target update cycle?
        if (SimLibElapsedTime > lastProcess)
        {
            gai->ProcessTargeting();
            lastProcess = SimLibElapsedTime + processRate;
        }

        // KCK: Check if it's ok to think
        if (SimLibElapsedTime > lastThought)
        {
            // do movement and (possibly) firing....
            gai->Process();
            lastThought = SimLibElapsedTime + thoughtRate;
        }

        // RV - Biker - Only allow SAM fire if radar still does work
        SimWeaponClass *theWeapon = Sms->GetCurrentWeapon();

        // FRB - This version seems to give SAMs a little more activity
        if (SimLibElapsedTime > nextSamFireTime and not allowSamFire)
        {
            allowSamFire = TRUE;
        }

        // Biker's version
        //if(SimLibElapsedTime > nextSamFireTime and not allowSamFire)
        //{
        // if (radarDown == false or (theWeapon and theWeapon->IsMissile() and theWeapon->sensorArray[0]->Type() == SensorClass::IRST))
        // allowSamFire = TRUE;
        //}

        // Move and update delta;
        gai->Move_Towards_Dest();

        // edg: always insure that our Z position is valid for the entity.
        // the draw pointer should have this value
        // KCK NOTE: The Z we have is actually LAST FRAME's Z. Probably not a big deal.
        SetPosition(
            XPos() + XDelta() * SimLibMajorFrameTime,
            YPos() + YDelta() * SimLibMajorFrameTime,
            groundZ
        );

        // do firing
        // this also does weapon keep alive
        if (Sms)
        {
            gai->Fire();
        }
    }

    // KCK: I simplified this some. This is now speed squared.
    speedScale = XDelta() * XDelta() + YDelta() * YDelta();

    // set our level of detail
    if (gai == gai->battalionCommand)
    {
        gai->SetDistLOD();
    }
    else
    {
        gai->distLOD = gai->battalionCommand->distLOD;
    }

    // do some extra LOD stuff: if the unit is not a lead veh amd the
    // distLOD is less than a certain value, remove it from the draw
    // list.
    if (drawPointer and gai->rank not_eq GNDAI_BATTALION_COMMANDER)
    {
        // distLOD cutoff by ranking (KCK: This is explicit for testing, could be a formula/table)
        if (gai->rank bitand GNDAI_COMPANY_LEADER)
        {
            labelLOD = .5F;
            drawLOD = .25F;
        }
        else if (gai->rank bitand GNDAI_PLATOON_LEADER)
        {
            labelLOD = .925F;
            drawLOD = .5F;
        }
        else
        {
            labelLOD = 1.1F;
            drawLOD = .75F;
        }

        // RV - Biker - Why do this maybe helpful knowing which vehicle has problems
        // Determine whether to draw label or not
        if (gai->distLOD < labelLOD)
        {
            if ( not IsSetLocalFlag(NOT_LABELED))
            {
                drawPointer->SetLabel("", 0xff00ff00); // Don't label
                SetLocalFlag(NOT_LABELED);
            }
        }
        else if (IsSetLocalFlag(NOT_LABELED))
        {
            SetLabel(this);
            UnSetLocalFlag(NOT_LABELED);
        }

        //if (IsSetLocalFlag(NOT_LABELED)) {
        // SetLabel(this);
        // UnSetLocalFlag(NOT_LABELED);
        //}
    }

    if ( not targetPtr)
    {
        //rotate turret to be pointing forward again
        float maxAz = TURRET_ROTATE_RATE * SimLibMajorFrameTime;
        float maxEl = TURRET_ELEVATE_RATE * SimLibMajorFrameTime;
        float newEl;

        if (isAirDefense)
        {
            newEl = 60.0F * DTR;
        }
        else
        {
            newEl = 0.0F;
        }

        float delta = newEl - GetDOFValue(AIRDEF_ELEV);

        if (delta > 180.0F * DTR)
        {
            delta -= 180.0F * DTR;
        }
        else if (delta < -180.0F * DTR)
        {
            delta += 180.0F * DTR;
        }

        // Do elevation adjustments
        if (delta > maxEl)
        {
            SetDOFInc(AIRDEF_ELEV, maxEl);
        }
        else if (delta < -maxEl)
        {
            SetDOFInc(AIRDEF_ELEV, -maxEl);
        }
        else
        {
            SetDOF(AIRDEF_ELEV, newEl);
        }

        SetDOF(AIRDEF_ELEV, min(85.0F * DTR, max(GetDOFValue(AIRDEF_ELEV), 0.0F)));
        SetDOF(AIRDEF_ELEV2, GetDOFValue(AIRDEF_ELEV));

        delta = 0.0F - GetDOFValue(AIRDEF_AZIMUTH);

        if (delta > 180.0F * DTR)
        {
            delta -= 180.0F * DTR;
        }
        else if (delta < -180.0F * DTR)
        {
            delta += 180.0F * DTR;
        }

        // Now do the azmuth adjustments
        if (delta > maxAz)
        {
            SetDOFInc(AIRDEF_AZIMUTH, maxAz);
        }
        else if (delta < -maxAz)
        {
            SetDOFInc(AIRDEF_AZIMUTH, -maxAz);
        }

        // RV - Biker - Don't do this
        //else
        // SetDOF(AIRDEF_AZIMUTH, 0.0F);
    }

    // Special shit by ground type
    if (isFootSquad)
    {
        if (speedScale > 0.0f)
        {
            // Couldn't this be done in the drawable class's update function???
            ((DrawableGuys*)drawPointer)->SetSquadMoving(TRUE);
        }
        else
        {
            // Couldn't this be done in the drawable class's update function???
            ((DrawableGuys*)drawPointer)->SetSquadMoving(FALSE);
        }

        // If we're less than 80% of the way from "FAR" toward the viewer, just draw one guy
        // otherwise, put 5 guys in a squad.
        if (gai->distLOD < 0.8f)
        {
            ((DrawableGuys*)drawPointer)->SetNumInSquad(1);
        }
        else
        {
            ((DrawableGuys*)drawPointer)->SetNumInSquad(5);
        }
    }
    // We're not a foot squad, so do the vehicle stuff
    else if ( not IsSetLocalFlag(IS_HIDDEN) and speedScale > 300.0f)
    {
        // speedScale /= ( 900.0f * KPH_TO_FPS * KPH_TO_FPS); // essentially 1.0F at 30 mph

        // JPO - for engine noise
        VehicleClassDataType *vc = GetVehicleClassData(Type() - VU_LAST_ENTITY_TYPE);
        ShiAssert(FALSE == F4IsBadReadPtr(vc, sizeof * vc));

        // (a) Make sound:
        // everything sounds like a tank right now
        if (GetCampaignObject()->IsBattalion())
        {
            //if (vc)
            if (vc and vc->EngineSound not_eq 34) // kludge prevent 34 from playing
            {
                SoundPos.Sfx(vc->EngineSound, 0, 1.0, 0);  // MLR 5/16/2004 -
            }
            else
            {
                SoundPos.Sfx(SFX_TANK, 0, 1.0, 0);  // MLR 5/16/2004 -
            }

            // (b) Make dust
            // dustTimer += SimLibMajorFrameTime;
            // if ( dustTimer > max( 0.2f,  4.5f - speedScale - gai->distLOD * 3.3f ) )
            if (((rand() bitand 7) == 7) and 
                gSfxCount[ SFX_GROUND_DUSTCLOUD ] < gSfxLODCutoff and 
                gTotSfx < gSfxLODTotCutoff
               )
            {
                // reset the timer
                // dustTimer = 0.0f;

                pos.x += PRANDFloat() * 5.0f;
                pos.y += PRANDFloat() * 5.0f;
                pos.z = groundZ;

                // RV - Biker - Move that smoke more behind the vehicle
                mlTrig trig;
                mlSinCos(&trig, Yaw());

                pos.x -= 15.0f * trig.cos;
                pos.y -= 15.0f * trig.sin;

                vec.x = PRANDFloat() * 5.0f;
                vec.y = PRANDFloat() * 5.0f;
                vec.z = -20.0f;

                //JAM 24Oct03 - No dust trails when it's raining.
                if (realWeather->weatherCondition < INCLEMENT)
                {
                    /*
                    OTWDriver.AddSfxRequest(
                     new SfxClass (SFX_VEHICLE_DUST, // type //JAM 03Oct03
                    // new SfxClass (SFX_GROUND_DUSTCLOUD, // type
                     SFX_USES_GRAVITY bitor SFX_NO_DOWN_VECTOR bitor SFX_MOVES bitor SFX_NO_GROUND_CHECK,
                     &pos, // world pos
                     &vec,
                     1.0f, // time to live
                     1.f)); //JAM 03Oct03 8.5f )); // scale
                     */
                    DrawableParticleSys::PS_AddParticleEx((SFX_VEHICLE_DUST + 1),
                                                          &pos,
                                                          &vec);
                }
            }

            // (c) Make sure we're using our 'Moving' model (i.e. Trucked artillery, APC, etc)
            if (truckDrawable)
            {
                // Keep truck 20 feet behind us (HACK HACK)
                Tpoint truckPos;
                mlTrig trig;
                mlSinCos(&trig, Yaw());
                truckPos.x = XPos() - 20.0F * trig.cos;
                truckPos.y = YPos() - 20.0F * trig.sin;
                truckPos.z = ZPos();
                truckDrawable->Update(&truckPos, Yaw() + PI);
            }

            if (isTowed or hasCrew)
            {
                SetSwitch(0, 0x2);
            }
        }
        else // itsa task force
        {
            if (vc)
            {
                SoundPos.Sfx(vc->EngineSound, 0, 1.0, 0);
            }
            else
            {
                SoundPos.Sfx(SFX_SHIP, 0, 1.0, 0);
            }

            //RV - I-Hawk - Do wakes only at some cases
            if ((rand() bitand 7) == 7)
            {
                //I-Hawk - not using all this anymore
                //
                // reset the timer
                // dustTimer = 0.0f;
                //float ttl;
                //static float trailspd = 5.0f;
                //static float bowfx = 0.92f;
                //static float sternfx = 0.75f;
                //float spdratio = GetVt() / ((UnitClass*)GetCampaignObject())->GetMaxSpeed();

                float radius;

                if (drawPointer)
                {
                    radius = drawPointer->Radius(); // JPO from 0.15 - now done inline
                }
                else
                {
                    radius = 90.0f;
                }

                //I-Hawk - Fixed position for ships wakes, effect "delay" in position is
                //handled by PS now. No more the "V shape" of water wakes.

                pos.x = XPos() + XDelta() * SimLibMajorFrameTime;
                pos.y = YPos() + YDelta() * SimLibMajorFrameTime;
                pos.z = groundZ;

                //// JPO - think this is sideways on.
                ///*
                //vec.x = dmx[1][0] * spdratio * PRANDFloat() * trailspd;
                //vec.y = dmx[1][1] * spdratio * PRANDFloat() * trailspd;
                //vec.z = 0.5f; // from -20 JPO
                //*/

                //I-Hawk - More correct vector for wakes
                vec.x = XDelta();
                vec.y = YDelta();
                vec.z = 0.0f;


                //I-Hawk - Separate wake effect for different ships size
                int theSFX;

                if (radius < 200.0f)
                {
                    theSFX = SFX_WATER_WAKE_SMALL;
                }

                else if (radius >= 200.0f and radius < 400.0f)
                {
                    theSFX = SFX_WATER_WAKE_MEDIUM;
                }

                else if (radius >= 400.0f)
                {
                    theSFX = SFX_WATER_WAKE_LARGE;
                }

                //I-Hawk - The PS
                DrawableParticleSys::PS_AddParticleEx((theSFX + 1),
                                                      &pos,
                                                      &vec);
            }
        }
    }
    // Otherwise, we're not moving or are hidden. Do some stuff
    else
    {
        // (b) Make sure we're using our 'Holding' model (i.e. Unlimbered artillery, troops prone, etc)
        if (truckDrawable)
        {
            // Once we stop, our truck doesn't move at all - but sits further away than when moving
            Tpoint truckPos;
            truckPos.x = XPos() + 40.0F;
            truckPos.y = YPos();
            truckPos.z = 0.0F;
            truckDrawable->Update(&truckPos, Yaw());
        }

        if (isTowed or hasCrew)
        {
            SetSwitch(0, 0x1);
        }
    }

    // ACMI Output
    if (gACMIRec.IsRecording() and (SimLibFrameCount bitand 0x0f) == 0)
    {
        ACMIGenPositionRecord genPos;
        genPos.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
        genPos.data.type = Type();
        genPos.data.uniqueID = ACMIIDTable->Add(Id(), NULL, TeamInfo[GetTeam()]->GetColor()); //.num_;
        genPos.data.x = XPos();
        genPos.data.y = YPos();
        genPos.data.z = ZPos();
        genPos.data.roll = Roll();
        genPos.data.pitch = Pitch();
        genPos.data.yaw = Yaw();
        // Remove genPos.data.teamColor = TeamInfo[GetTeam()]->GetColor();
        gACMIRec.GenPositionRecord(&genPos);
    }

    return IsLocal();
}

void GroundClass::ApplyDamage(FalconDamageMessage *damageMessage)
{
    if (IsDead())
    {
        return;
    }

    SimVehicleClass::ApplyDamage(damageMessage);
    lastDamageTime = SimLibElapsedTime;
}

void GroundClass::SetDead(int flag)
{
    SimVehicleClass::SetDead(flag);
}

int GroundClass::Wake(void)
{
    if (IsAwake())
    {
        return 0;
    }

    SimVehicleClass::Wake();

    Tpoint pos;
    int retval = 0;

    if (Sms)
    {
        Sms->AddWeaponGraphics();
    }

    // Pick a groupId. KCK: Is this even being used?
    groupId = GetCampaignObject()->Id().num_;

    if ( not gai->rank)
    {
        drawPointer->SetLabel("", 0xff00ff00);
    }

    // edg: I'm not sure if drawpointer has valid position yet?
    drawPointer->GetPosition(&pos);
    SetPosition(XPos(), YPos(), pos.z - 0.7f);
    ShiAssert(XPos() > 0.0F and YPos() > 0.0F)

    // determine if its a foot squad or not -- Real thinking done in addobj.cpp
    isFootSquad = (drawPointer->GetClass() == DrawableObject::Guys);

    // Determine if this vehicle has a truck and create it, if needed
    if (drawPointer and isTowed)
    {
        // Place truck 20 feet behind us
        Tpoint simView;
        int vistype;
        mlTrig trig;
        int tracktorType;

        // RV - Biker - Make the tracktor random
        tracktorType = (rand() % 3);
        bool teamUs = (
                          TeamInfo[GetCountry()] and (
                              TeamInfo[GetCountry()]->equipment == toe_us or TeamInfo[GetCountry()]->equipment == toe_rok
                          )
                      );
        int vtIdx;

        switch (tracktorType)
        {
            case 0:
                vtIdx =  teamUs ? F4_GENERIC_US_TRUCK_TYPE_SMALL   : F4_GENERIC_OPFOR_TRUCK_TYPE_SMALL;
                break;

            case 1:
                vtIdx =  teamUs ? F4_GENERIC_US_TRUCK_TYPE_LARGE   : F4_GENERIC_OPFOR_TRUCK_TYPE_LARGE;
                break;

            default:
                vtIdx = teamUs ? F4_GENERIC_US_TRUCK_TYPE_TRAILER : F4_GENERIC_OPFOR_TRUCK_TYPE_TRAILER;
                break;
        }

        vistype = Falcon4ClassTable[vtIdx].visType[Status() bitand VIS_TYPE_MASK];

        mlSinCos(&trig, Yaw());
        simView.x = XPos() - 20.0F * trig.cos;
        simView.y = YPos() - 20.0F * trig.sin;
        simView.z = ZPos();

        if (vistype > 0)
        {
            truckDrawable = new DrawableGroundVehicle(vistype, &simView, Yaw() + PI, drawPointer->GetScale());
        }
    }

    // when we wake the object, default it to not labeled unless
    // it's the main guy
    if (drawPointer and gai->rank not_eq GNDAI_BATTALION_COMMANDER)
    {
        drawPointer->SetLabel("", 0xff00ff00); // Don't label
        SetLocalFlag(NOT_LABELED);
    }

    return retval;
}

int GroundClass::Sleep(void)
{
    int retval = 0;

    if ( not IsAwake())
    {
        return retval;
    }

    // NULL any targets our ai might have
    if (gai)
    {
        gai->SetGroundTarget(NULL);
        gai->SetAirTarget(NULL);
    }

    if (battalionFireControl)
    {
        VuDeReferenceEntity(battalionFireControl);
        battalionFireControl = NULL;
    }

    // Put away any weapon graphics
    if (Sms)
    {
        Sms->FreeWeaponGraphics();
    }

    SimVehicleClass::Sleep();

    // Dispose of our truck, if we have one.
    if (truckDrawable)
    {
        OTWDriver.RemoveObject(truckDrawable, TRUE);
        truckDrawable = NULL;
    }

    return retval;
}

VU_ERRCODE GroundClass::InsertionCallback(void)
{
    return SimMoverClass::InsertionCallback();
}

VU_ERRCODE GroundClass::RemovalCallback(void)
{
    // Put this here, since it will get called for both single and multiplayer cases
    gai->PromoteSubordinates();
    return SimMoverClass::RemovalCallback();
}

