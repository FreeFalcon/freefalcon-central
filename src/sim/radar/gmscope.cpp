#include "stdhdr.h"
#include "Graphics/Include/gmComposit.h"
#include "Graphics/Include/Drawbsp.h"
#include "geometry.h"
#include "debuggr.h"
#include "object.h"
#include "radarDoppler.h"
#include "simbase.h"
#include "otwdrive.h"
#include "simdrive.h"
#include "simfiltr.h"
#include "simveh.h"
#include "campwp.h"
#include "sms.h"
#include "smsdraw.h"
#include "aircrft.h"
#include "fack.h" //MI
#include "classtbl.h" // JB carrier
#include "fcc.h" //MI
#include "laserpod.h" //MI
#include "mavdisp.h" //MI
#include "sms.h" //MI
#include "missile.h" //MI
#include "profiler.h" // MLR 5/21/2004 - 
#include "FastMath.h"

#include "Campwp.h"  // MD -- 20040214: added for SP ground stabilization pseudo waypoint
#include "Hud.h"  // MD -- 20040306: added for TTG display in GMscope

// MD -- 20040108: added for analog RNG knob support
#include "simio.h"
extern SIMLIB_IO_CLASS IO;

#define  DEFAULT_OBJECT_RADIUS        50.0F

SensorClass* FindLaserPod(SimMoverClass* theObject); //MI
extern float g_fCursorSpeed;
extern bool g_bRealisticAvionics;
extern bool g_bAGRadarFixes;
extern float g_fGMTMaxSpeed;
extern float g_fGMTMinSpeed;
extern float g_fEXPfactor;
extern float g_fDBS1factor;
extern float g_fDBS2factor;
//TJL 11/25/03
extern bool g_bnoRadStutter;


// NOTE:  These being global mean that there can be only one RadarDopplerClass instance.
// They could easily be members of the class if this is a problem...
static Tpoint GMat;
static Tpoint viewCenter;
static Tpoint viewOffsetRel = {0.0F, 0.0F, 0.0F};
static Tpoint viewOffsetInertial = {0.0F, 0.0F, 0.0F};
static Tpoint viewFrom = {0.0F, 0.0F, 0.0F};
static float headingForDisplay = 0.0F;

static const float DisplayAreaViewTop    =  0.75F;
static const float DisplayAreaViewBottom = -0.68F;
static const float DisplayAreaViewLeft   = -0.80F;
static const float DisplayAreaViewRight  =  0.72F;
static const float RADAR_CONE_ANGLE      = 0.32f * PI;
static const float SIN_RADAR_CONE_ANGLE  = (float)sin(RADAR_CONE_ANGLE);
static const float COS_RADAR_CONE_ANGLE  = (float)cos(RADAR_CONE_ANGLE);
static const float TAN_RADAR_CONE_ANGLE  = (float)tan(RADAR_CONE_ANGLE);

void CalcRelGeom(SimBaseClass* ownObject, SimObjectType* targetList, TransformMatrix vmat, float elapsedTimeInverse);


float cRangeSquaredGMScope;




int RadarDopplerClass::InitialGroundContactTest(
    float &ownX, float &ownY, float &ownZ,
    float &radarHorizonSq, FalconEntity *contact,
    mlTrig &trig,
    // returned values
    float &range, float &radius, float &canSee)
{
    //Tpoint pos;
    float x, y, dx, dy;

    range = (contact->XPos() - ownX) * (contact->XPos() - ownX) +
            (contact->YPos() - ownY) * (contact->YPos() - ownY) +
            (contact->ZPos() - ownZ) * (contact->ZPos() - ownZ);

    if (range < radarHorizonSq)
    {
        range = sqrt(range);

        if (contact->IsSim())
        {
            if (((SimBaseClass*)contact)->IsAwake())
            {
                radius = ((SimBaseClass*)contact)->drawPointer->Radius();
                radius = radius * radius * radius * radius;
                canSee = radius / range * tdisplayRange / groundMapRange;
                // MLR why???
                //((SimBaseClass*)contact)->drawPointer->GetPosition(&pos);
                //contact->SetPosition(pos.x, pos.y, pos.z);
            }
            else
            {
                canSee = 0.0F;
            }
        }
        else
        {
            radius = DEFAULT_OBJECT_RADIUS;
            radius = radius * radius * radius * radius;
            canSee = radius / range * tdisplayRange / groundMapRange;
        }

        // Check LOS
        if (canSee > 0.8F)
        {
            x = contact->XPos() - ownX;
            y = contact->YPos() - ownY;

            // Rotate for normalization
            dx = trig.cos * x - trig.sin * y;
            dy = trig.sin * x + trig.cos * y;

            // Check Angle off nose
            if ((dy > 0.0F and dx > 0.5F *  dy) or  // Right side of nose
                (dy < 0.0F and dx > 0.5F * -dy)   // Left side of nose
               )
            {
                // Actual LOS
                if ( not OTWDriver.CheckLOS(platform, contact))
                {
                    canSee = 0.0F;  // LOS is blocked
                }
            }
            else
            {
                canSee = 0.0F;   // Outside of cone
            }
        }
    }

    if (canSee > .8f)
        return 1;

    return 0;
}

int RadarDopplerClass::GMTObjectContactTest(FalconEntity *contact)
{
    // Begine GMT test
    if (g_bRealisticAvionics and g_bAGRadarFixes)
    {
        if (contact->IsSim())
        {
            // never show pedestrians
            if (((SimBaseClass *)contact)->drawPointer and 
                ((SimBaseClass *)contact)->drawPointer->GetClass() == DrawableObject::Guys
               )
            {
                return 0;
            }

            // speed filter
            if (contact->GetVt() > g_fGMTMinSpeed and 
                contact->GetVt() < g_fGMTMaxSpeed)
            {
                return 1;
            }

            return 0;
        }
        else
        {
            //if(contact->GetVt() > 1.0f)
            // return 1;
            return 0;
        }
    }
    else
    {
        if (contact->GetVt())
            return 1;
    }

    return 0;
}



// for testing Entities from the Object list.
// Do not test Features with this code, just accept them
int RadarDopplerClass::GMObjectContactTest(FalconEntity *contact)
{
    // objects can move
    if (g_bRealisticAvionics and g_bAGRadarFixes)
    {
        // maybe at some point, we might want to have a Vt threshold other than 0
        // if so, we'll have to
        if (contact->GetVt() > g_fGMTMinSpeed)
            return 0;

        if (contact->IsSim())
        {

            //if(contact->GetVt() > 1.0F)
            // return 0.0f;
            if (((SimBaseClass*)contact)->drawPointer and 
                ((SimBaseClass*)contact)->drawPointer->GetClass() == DrawableObject::Guys)
            {
                return 0;
            }
        }

        /*
        else // is camp object
        {
         // 2002-04-03 MN contact is a CAMPAIGN object now  We can't do SimBaseClass stuff here.
         // Speed test however is valid, as it checks U_MOVING flag of unit
         // As there are no campaign units that consist only of soldiers, no need to check for them here
         if(contact->GetVt() > 1.0F )
         return 0.0f;
        }
        */
    }

    return 1;
}

/*
 Concept: Iterate the vu lists, determine if a contact should be
 on/off the contact list.  If the contact should be on the list, check to see
 if it is already marked as 'on the list', if not, create a new GMList
 node, and add it to the contact list.  If the contact should be 'off the list'
 clear the flag.

    After iterating both vu lists, purge the contact lists of entities where
 the 'on the list' flag has been cleared.
*/

#ifdef USE_HASH_TABLES

void RadarDopplerClass::GMMode(void)
{
    VuListIterator *walker = NULL; //MI

    FalconEntity* testFeature = NULL;

    GMList* curNode = NULL;

    float range = 0.0F;
    float radius = 0.0F;
    float canSee = 0.0F;
    float x = 0.0F;
    float y = 0.0F;
    float dx = 0.0F;
    float dy = 0.0F;
    float radarHorizonSq;
    float ownX;
    float ownY;
    float ownZ;

    mlTrig trig;

    mlSinCos(&trig, -platform->Yaw());

    // Find out where the beam hits the edge of the earth
    radius = EARTH_RADIUS_FT - platform->ZPos();
    radarHorizonSq = (float)/*sqrt*/ (radius * radius - EARTH_RADIUS_FT * EARTH_RADIUS_FT);
    ownX = platform->XPos();
    ownY = platform->YPos();
    ownZ = platform->ZPos();


    // flag all of the objects in our lists
    curNode = GMFeatureListRoot;

    while (curNode)
    {
        curNode->Object()->SetFELocalFlag(FELF_ON_PLAYERS_GM_CONTACT_LIST);
        curNode = curNode->next;
    }

    curNode = GMMoverListRoot;

    while (curNode)
    {
        curNode->Object()->SetFELocalFlag(FELF_ON_PLAYERS_GMT_CONTACT_LIST);
        curNode = curNode->next;
    }

    if (SimLibElapsedTime - lastFeatureUpdate > 500)
    {
        lastFeatureUpdate = SimLibElapsedTime;

        // walk features first, features only show on GM display
        {
            VuListIterator featureWalker(SimDriver.combinedFeatureList);
            walker = &featureWalker;
            testFeature = (FalconEntity*)walker->GetFirst();

            while (testFeature)
            {
                canSee = 0;

                if (isEmitting)
                {
                    // this sets canSee
                    InitialGroundContactTest(
                        ownX, ownY, ownZ,
                        radarHorizonSq, testFeature,
                        trig,
                        // returned values
                        range, radius, canSee) ;
                }

                // update entity flags
                if (canSee < .8f)
                {
                    // clear the flags
                    testFeature->UnSetFELocalFlag(FELF_ON_PLAYERS_GM_CONTACT_LIST);
                }
                else
                {
                    if (canSee > 1.0f)
                    {
                        if ( not testFeature->IsSetFELocalFlag(FELF_ON_PLAYERS_GM_CONTACT_LIST))
                        {
                            // only add new nodes if we're not in the list already
                            GMList *newNode = new GMList(testFeature);
                            newNode->next = GMFeatureListRoot;
                            GMFeatureListRoot = newNode;

                            testFeature->SetFELocalFlag(FELF_ON_PLAYERS_GM_CONTACT_LIST);
                        }
                    }
                }

                testFeature = (FalconEntity*)walker->GetNext();
            }
        }

        {
            VuListIterator objectWalker(SimDriver.combinedList);
            walker = &objectWalker;
            testFeature = (FalconEntity*)walker->GetFirst();

            // add features to target list that aren't already in the target list
            while (testFeature)
            {
                int gmCanSee  = 0;
                int gmtCanSee = 0;

                if (isEmitting)
                {
                    if (InitialGroundContactTest(
                            ownX, ownY, ownZ,
                            radarHorizonSq, testFeature,
                            trig,
                            // returned values
                            range, radius, canSee))
                    {
                        gmCanSee  = GMObjectContactTest(testFeature);
                        gmtCanSee = GMTObjectContactTest(testFeature);
                    }
                }

                // update entity flags
                if (canSee < .8f)
                {
                    // clear both flags
                    testFeature->UnSetFELocalFlag((FalconEntityLocalFlags)(FELF_ON_PLAYERS_GM_CONTACT_LIST bitor FELF_ON_PLAYERS_GMT_CONTACT_LIST));
                }
                else
                {
                    if (canSee > 1.0f)
                    {
                        if (gmCanSee)
                        {
                            if ( not testFeature->IsSetFELocalFlag(FELF_ON_PLAYERS_GM_CONTACT_LIST))
                            {
                                GMList *newNode = new GMList(testFeature);
                                newNode->next = GMFeatureListRoot;
                                GMFeatureListRoot = newNode;

                                testFeature->SetFELocalFlag(FELF_ON_PLAYERS_GM_CONTACT_LIST);
                            }
                        }
                        else
                            testFeature->UnSetFELocalFlag(FELF_ON_PLAYERS_GM_CONTACT_LIST);

                        if (gmtCanSee)
                        {
                            if ( not testFeature->IsSetFELocalFlag(FELF_ON_PLAYERS_GMT_CONTACT_LIST))
                            {
                                GMList *newNode = new GMList(testFeature);
                                newNode->next = GMMoverListRoot;
                                GMMoverListRoot = newNode;

                                testFeature->SetFELocalFlag(FELF_ON_PLAYERS_GMT_CONTACT_LIST);
                            }
                        }
                        else
                            testFeature->UnSetFELocalFlag(FELF_ON_PLAYERS_GMT_CONTACT_LIST);
                    }
                }

                testFeature = (FalconEntity*)walker->GetNext();
            } // end while(testFeature);
        }

        // prune the contact lists and clear the flags
        GMList **next, *test;

        next = &GMFeatureListRoot;

        while (*next)
        {
            test = *next;

            if ( not test->Object()->IsSetFELocalFlag(FELF_ON_PLAYERS_GM_CONTACT_LIST))
            {
                *next = test->next; // removes the node from the list
                test->Release(); // self deleting
            }
            else
            {
                test->Object()->UnSetFELocalFlag(FELF_ON_PLAYERS_GM_CONTACT_LIST);
                next = &((*next)->next);
            }
        }

        next = &GMMoverListRoot;

        while (*next)
        {
            test = *next;

            if ( not test->Object()->IsSetFELocalFlag(FELF_ON_PLAYERS_GMT_CONTACT_LIST))
            {
                *next = test->next; // removes the node from the list
                test->Release(); // self deleting
            }
            else
            {
                test->Object()->UnSetFELocalFlag(FELF_ON_PLAYERS_GMT_CONTACT_LIST);
                next = &((*next)->next);
            }
        }
    } // end if elapsed time


    if (IsSOI() and dropTrackCmd)
    {
        DropGMTrack();
    }

    //MI
    if (g_bRealisticAvionics and g_bAGRadarFixes)
    {
        if (lockedTarget and lockedTarget->BaseData())
        {
            if (mode == GMT)
            {
                if (lockedTarget->BaseData()->IsSim() and (lockedTarget->BaseData()->GetVt() < g_fGMTMinSpeed or
                        lockedTarget->BaseData()->GetVt() > g_fGMTMaxSpeed))
                    DropGMTrack();
                else if (lockedTarget->BaseData()->IsCampaign() and lockedTarget->BaseData()->GetVt() <= 0.0F)
                    DropGMTrack();
            }
            else if (mode == GM)
            {
                if (lockedTarget->BaseData()->IsSim() and lockedTarget->BaseData()->GetVt() > g_fGMTMinSpeed)
                    DropGMTrack();
                else if (lockedTarget->BaseData()->IsCampaign() and lockedTarget->BaseData()->GetVt() > 0.0F)
                    DropGMTrack();
            }
        }
    }

    //Build Track List
    if (lockedTarget)
    {
        // WARNING:  This might do ALOT more work than you want.  CalcRelGeom
        // will walk all children of lockedTarget (through the next pointer).
        // If you don't want this, set is next pointer to NULL before calling
        // and set it back upon return.
        CalcRelGeom(platform, lockedTarget, NULL, 1.0F / SimLibMajorFrameTime);
    }

    if (designateCmd)
    {
        if (mode == GM)
        {
            DoGMDesignate(GMFeatureListRoot);
        }
        else if (mode == GMT or mode == SEA)
        {
            DoGMDesignate(GMMoverListRoot);
        }
    }

    // MD -- 20040216: update the Pseudo waypoint after there was a slew operation
    if (IsSet(SP) and IsSet(SP_STAB))
    {
        //  only update if cursor is in the field of MFD view
        if ((F_ABS(cursorX) < 0.95F) and (F_ABS(cursorY) < 0.95F))
        {

            float x = 0.0F, y = 0.0F, z = 0.0F;

            if ( not GMSPPseudoWaypt)
            {
                GMSPPseudoWaypt = new WayPointClass();
            }

            // calculate x/y/z's
            GetGMCursorPosition(&x, &y);

            z = OTWDriver.GetGroundLevel(x, y);

            GMSPPseudoWaypt->SetLocation(x, y, z);
            GMSPPseudoWaypt->SetWPArrive(SimLibElapsedTime);

            SetGroundPoint(x, y, z);
            ToggleAGcursorZero();

            SetGMScan();
        }
    }

    // Update groundSpot position
    GetAGCenter(&x, &y);
    ((AircraftClass*)platform)->Sms->drawable->SetGroundSpotPos(x, y, 0.0F);
}

#endif

void RadarDopplerClass::DropGMTrack(void)
{
    float cosAz, sinAz;
    mlTrig trig;

    if (lockedTarget)
    {
        if (g_bRealisticAvionics and g_bAGRadarFixes)
        {
            mlSinCos(&trig, headingForDisplay);
            cosAz = trig.cos;
            sinAz = trig.sin;

            viewOffsetInertial.x = lockedTarget->BaseData()->XPos() -
                                   (platform->XPos() + tdisplayRange * cosAz * 0.5F);
            viewOffsetInertial.y = lockedTarget->BaseData()->YPos() -
                                   (platform->YPos() + tdisplayRange * sinAz * 0.5F);

            viewOffsetRel.x =  cosAz * viewOffsetInertial.x + sinAz * viewOffsetInertial.y;
            viewOffsetRel.y = -sinAz * viewOffsetInertial.x + cosAz * viewOffsetInertial.y;

            viewOffsetRel.x /= tdisplayRange * 0.5F;
            viewOffsetRel.y /= tdisplayRange * 0.5F;
        }
        else
        {
            if (flags bitand NORM)
            {
                mlSinCos(&trig, headingForDisplay);
                cosAz = trig.cos;
                sinAz = trig.sin;

                if ( not (flags bitand SP))
                {
                    GMat.x = platform->XPos() + tdisplayRange * cosAz * 0.5F;
                    GMat.y = platform->YPos() + tdisplayRange * sinAz * 0.5F;
                }
                else
                {
                    viewOffsetInertial.x = lockedTarget->BaseData()->XPos() -
                                           (platform->XPos() + tdisplayRange * cosAz * 0.5F);
                    viewOffsetInertial.y = lockedTarget->BaseData()->YPos() -
                                           (platform->YPos() + tdisplayRange * sinAz * 0.5F);
                }

                viewOffsetRel.x =  cosAz * viewOffsetInertial.x + sinAz * viewOffsetInertial.y;
                viewOffsetRel.y = -sinAz * viewOffsetInertial.x + cosAz * viewOffsetInertial.y;

                viewOffsetRel.x /= tdisplayRange * 0.5F;
                viewOffsetRel.y /= tdisplayRange * 0.5F;
            }
        }

        ClearSensorTarget();

        //MI add image noise again
        if (g_bRealisticAvionics and g_bAGRadarFixes)
        {
            scanDir = ScanFwd;
        }
    }
    else if (IsSet(SP) and IsSet(SP_STAB) and dropTrackCmd)
    {
        // MD -- 20040115: clear the ground stabilization and return FCC to steerpoint mode when
        // we are explicitly dropping a pseudo waypoint.
        if (SimDriver.GetPlayerAircraft())
            SimDriver.GetPlayerAircraft()->FCC->SetStptMode(FireControlComputer::FCCWaypoint);

        SetGMSPWaypt(NULL);
        ClearFlagBit(SP_STAB);
        ToggleAGcursorZero();
    }
}

void RadarDopplerClass::SetAimPoint(float xCmd, float yCmd)
{
    float sinAz, cosAz;
    mlTrig trig;
    float halfRange = tdisplayRange * 0.5F;

    MaverickDisplayClass *mavDisplay = NULL;

    //MI we also want this happening when in TGP mode, not ground stabilized and SOI
    LaserPodClass* laserPod = (LaserPodClass*)FindLaserPod(SimDriver.GetPlayerAircraft());
    //MI same for MAV's
    AircraftClass *pac = SimDriver.GetPlayerAircraft();

    if (pac and pac->Sms and pac ->Sms->curWeaponType == wtAgm65 and pac->Sms->curWeapon)
    {
        mavDisplay = (MaverickDisplayClass*)((MissileClass*)pac->Sms->GetCurrentWeapon())->display;
    }

    if (
        (IsSOI() and not lockedTarget) or
        (
            ((laserPod and laserPod->IsSOI()) or (mavDisplay and mavDisplay->IsSOI())) and 
            pac and pac->FCC and pac->FCC->preDesignate
        ) or
        (pac->FCC->GetSubMode() == FireControlComputer::CCRP)
    )
    {
        //MI better cursor control
        // MD -- 20040215: the cursor doesn't move in SP until you are ground stabilized
        if ((xCmd not_eq 0.0F or yCmd not_eq 0.0F) and (( not IsSet(SP)) or (IsSet(SP) and IsSet(SP_STAB))))
        {
            float CursorSpeed = g_fCursorSpeed;

            if (flags bitand DBS2)
                CursorSpeed *= g_fDBS2factor;
            else if (flags bitand DBS1)
                CursorSpeed *= g_fDBS1factor;
            else if (flags bitand EXP)
                CursorSpeed *= g_fEXPfactor;

            if ((IO.AnalogIsUsed(AXIS_CURSOR_X) == true) and (IO.AnalogIsUsed(AXIS_CURSOR_Y) == true))
            {
                viewOffsetRel.x += (yCmd / 20000.0F) * CursorSpeed * (6.5F * CursorRate) * SimLibMajorFrameTime;
                viewOffsetRel.y += (xCmd / 20000.0F) * CursorSpeed * (6.5F * CursorRate) * SimLibMajorFrameTime;
            }
            else
            {
                viewOffsetRel.x += yCmd * 0.5F * CursorSpeed * curCursorRate * SimLibMajorFrameTime;
                viewOffsetRel.y += xCmd * 0.5F * CursorSpeed * curCursorRate * SimLibMajorFrameTime;
                static float test = 0.0f;
                static float testa = 0.0f;
                curCursorRate = min(curCursorRate + CursorRate * SimLibMajorFrameTime * (4.0F + test), (6.5F + testa) * CursorRate);
            }
        }
        else
        {
            if ((IO.AnalogIsUsed(AXIS_CURSOR_X) == false) or (IO.AnalogIsUsed(AXIS_CURSOR_Y) == false))
                curCursorRate = CursorRate;

            //TJL 11/19/03
            //GetCursorXYFromWorldXY (&CursorX, &CursorY, WorldX, WorldY);
        }

        //viewOffsetRel.x += yCmd * CursorRate*3 * SimLibMajorFrameTime *
        //     groundMapRange / halfRange;
        //viewOffsetRel.y += xCmd * CursorRate*3 * SimLibMajorFrameTime *
        //    groundMapRange / halfRange;

        // 2002-04-04 MN fix for cursor movement on the radar cone borders, only restrict to SnowPlow
        if (((flags bitand SP) and (float)fabs(viewOffsetRel.y) > (viewOffsetRel.x + 1.0F)*TAN_RADAR_CONE_ANGLE))
        {
            // set to middle axis when really close to it
            if (viewOffsetRel.y > -0.05f and viewOffsetRel.y < 0.05f)
            {
                viewOffsetRel.y = 0.0f;
            }
            else if (xCmd and yCmd)  // let the cursor stay at its position on the gimbal border
            {
                if (viewOffsetRel.y >= 0.0f)
                {
                    viewOffsetRel.y = (float)(viewOffsetRel.x + 1.0F) * TAN_RADAR_CONE_ANGLE;
                    viewOffsetRel.x = (float)(viewOffsetRel.y  / TAN_RADAR_CONE_ANGLE) - 1.0F;
                }
                else
                {
                    viewOffsetRel.y = -((float)(viewOffsetRel.x + 1.0F) * TAN_RADAR_CONE_ANGLE);
                    viewOffsetRel.x = -((float)(viewOffsetRel.y / TAN_RADAR_CONE_ANGLE) + 1.0F);
                }
            }
            else if (yCmd)
            {
                if (viewOffsetRel.y >= 0.0f)
                    // Tangens works a bit different ;-)
                    //viewOffsetRel.y = (float)fabs(viewOffsetRel.x + 1.0F) / TAN_RADAR_CONE_ANGLE - 1.0F;
                    viewOffsetRel.y = (float)(viewOffsetRel.x + 1.0F) * TAN_RADAR_CONE_ANGLE;
                else
                    viewOffsetRel.y = -((float)(viewOffsetRel.x + 1.0F) * TAN_RADAR_CONE_ANGLE);
            }
            else if (xCmd)
            {
                if (viewOffsetRel.y >= 0.0F)
                    viewOffsetRel.x = (float)(viewOffsetRel.y  / TAN_RADAR_CONE_ANGLE) - 1.0F;
                else
                    viewOffsetRel.x = -(float)((viewOffsetRel.y / TAN_RADAR_CONE_ANGLE) + 1.0F);
            }
        }

        viewOffsetRel.x = min(max(viewOffsetRel.x, -0.975F), 0.975F);
        viewOffsetRel.y = min(max(viewOffsetRel.y, -0.975F), 0.975F);

    }

    mlSinCos(&trig, headingForDisplay);
    cosAz = trig.cos;
    sinAz = trig.sin;

    viewOffsetInertial.x = cosAz * viewOffsetRel.x - sinAz * viewOffsetRel.y;
    viewOffsetInertial.y = sinAz * viewOffsetRel.x + cosAz * viewOffsetRel.y;

    viewOffsetInertial.x *= halfRange;
    viewOffsetInertial.y *= halfRange;

}

int RadarDopplerClass::CheckGMBump(void)
{
    int rangeChangeCmd = 0;
    int maxIdx;
    float cRangeSQ;
    float tmpX = (viewOffsetRel.x + 1.0F) * 0.5F;   // Correct for 0.0 being the center of the scope
    float topfactor = 0.0F;
    float bottomfactor = 0.0F;

#if 0
    // MD -- 20040229: comment this lot out -- it's not clear what this is coded to do but it doesn't appear to be
    // correct according to docs Mirv provided and behavioral testing.
    int range = (int)rangeScales[curRangeIdx];

    switch (range)
    {
        case 10:
            if (flags bitand SP) //SnowPlow
            {
                topfactor = 1.6F;
                bottomfactor = 0.0F;
            }
            else
            {
                topfactor = 0.48F;
                bottomfactor = 0.0F;
            }

            break;

        case 20:
            if (flags bitand SP) //SnowPlow
            {
                topfactor = 1.7F;
                bottomfactor = 0.0078F;
            }
            else
            {
                topfactor = 1.797F;
                bottomfactor = 0.05F;
            }

            break;

        case 40:
            if (flags bitand SP) //SnowPlow
            {
                topfactor = 1.7F;
                bottomfactor = 0.0088F;
            }
            else
            {
                topfactor = 1.80280F;
                bottomfactor = 0.236F;
            }

            break;

        case 80:
            if (flags bitand SP) //SnowPlow
            {
                topfactor = 2.0F;
                bottomfactor = 0.014497F;
            }
            else
            {
                topfactor = 2.0F;
                bottomfactor = 0.4F;
            }

            break;

        default:
            ShiWarning("Should not get here")
            break;
    }


    //   cRangeSQ =  tmpX*tmpX + viewOffsetRel.y*viewOffsetRel.y;
    cRangeSQ = tmpX * tmpX + tmpX * tmpX; // only change range when we exceed limits in y direction (viewOffsetRel.x = y direction)

    cRangeSquaredGMScope = cRangeSQ;
    GetDOFValue(ComplexGearDOF[1]) == (af->GetAeroData(AeroDataSet::NosGearRng + 4) * DTR)

    //MI
    if (g_bRealisticAvionics and g_bAGRadarFixes)
    {
        if (IsSet(AutoAGRange))
        {
            if (cRangeSQ > topfactor)
            {
                rangeChangeCmd = 1;
            }
            else if (cRangeSQ < bottomfactor)
            {
                rangeChangeCmd = -1;
            }
        }
    }
    else
    {
        if (cRangeSQ > 0.95F * 0.95F)
        {
            rangeChangeCmd = 1;
        }
        else if (cRangeSQ < 0.425F * tmpX)
        {
            rangeChangeCmd = -1;
        }
    }

#endif

    // MD -- 20040229: it appears that the bump ranges are set as 95% of display for max and 42.5% of
    // display for for min.  These numbers are modified by the cosine of the azimuth of the cursor
    // position as an additional multiplier.  Also, the bump does not happen if the radar is frozen
    // or if the cursors are in motion or of course if the MAN range function is set.

    if (IsSet(FZ) or IsSet(WasMoving) or ( not IsSet(AutoAGRange)))
        return rangeChangeCmd;

    float x = (GMat.x + viewOffsetInertial.x) - platform->XPos();
    float y = (GMat.y + viewOffsetInertial.y) - platform->YPos();

    float noseCursorAngle = platform->Yaw();

    noseCursorAngle -= static_cast<float>(atan2(y, x));

    if (noseCursorAngle > 180.0F * DTR)
        noseCursorAngle -= 360.0F * DTR;
    else if (noseCursorAngle < -180.0F * DTR)
        noseCursorAngle += 360.0F * DTR;

    if (fabs(noseCursorAngle) > (90.0F * DTR)) // he's behind you
        return rangeChangeCmd;  // no bump unless the cursor is on the display in front of your jet

    cRangeSQ = (x * x) + (y * y);

    float fromNose = (float)sqrt(cRangeSQ);

    // since display cursor position is capped at +/-0.95F, apply the percentages to
    // that base but scale for correct range anyway.
    if (fromNose >= (0.95F * 0.95F * tdisplayRange))
    {
        rangeChangeCmd = 1;
    }
    else if (fromNose < (0.425F * 0.95F * tdisplayRange))
    {
        rangeChangeCmd = -1;
    }

    // Max range available
    if (flags bitand (DBS1 bitor DBS2) or mode == GMT or mode == SEA)
    {
        maxIdx = NUM_RANGES - 2;
    }
    else
    {
        maxIdx = NUM_RANGES - 1;
    }

    if (curRangeIdx + rangeChangeCmd >= maxIdx)
        rangeChangeCmd = 0;
    else if (curRangeIdx + rangeChangeCmd < 0)
        rangeChangeCmd = 0;

    return rangeChangeCmd;
}

void RadarDopplerClass::AdjustGMOffset(int rangeChangeCmd)
{
    if (rangeChangeCmd > 0)
    {
        //MI
        if (g_bRealisticAvionics and g_bAGRadarFixes)
        {
            if (mode == GM or mode == SEA)
            {
                if (gmRangeIdx >= 4)
                    return;
            }
            else if (mode == GMT)
            {
                if (gmRangeIdx >= 3)
                    return;
            }
        }

        viewOffsetRel.y *= 0.5F;
        viewOffsetRel.x = ((viewOffsetRel.x + 1.0F) * 0.5F) - 1.0F;
    }
    else if (rangeChangeCmd < 0)
    {
        //MI
        if (g_bRealisticAvionics and g_bAGRadarFixes)
        {
            if (gmRangeIdx == 0)
                return;
        }

        viewOffsetRel.y *= 2.0F;
        viewOffsetRel.x = ((viewOffsetRel.x + 1.0F) * 2.0F) - 1.0F;
    }

    SetAimPoint(0.0F, 0.0F);
}


void RadarDopplerClass::SetGroundPoint(float xPos, float yPos, float zPos)
{
    viewCenter.x = xPos;
    viewCenter.y = yPos;
    viewCenter.z = zPos;
}

// 2002-04-04 MN this fixes old AG radar bug where when you were in STP mode,
void RadarDopplerClass::RestoreAGCursor()
{
    viewOffsetInertial.x = 0.0f;
    viewOffsetInertial.y = 0.0f;
    viewOffsetInertial.z = 0.0f;

    viewOffsetRel.x = 0.0f;
    viewOffsetRel.y = 0.0f;
    viewOffsetRel.z = 0.0f;

    // now check if steerpoint position is off of the current radar range, and adjust it appropriately
    // only in NORM mode and STP mode
    // MD -- 20040229: and make this adjustment if we are ground stabilized in SP mode as well
    if ((flags bitand NORM) and (( not (flags bitand SP)) or (IsSet(SP) and IsSet(SP_STAB))))
    {
        // Distance to GMat, only x and y
        float dx, dy, dist, dispRange;
        dx = platform->XPos() - viewCenter.x;
        dy = platform->YPos() - viewCenter.y;

        dist = dx * dx + dy * dy;

        // find correct radar scan distance
        int i = 0;

        for (i = 0; i < NUM_RANGES; i++)
        {
            dispRange = rangeScales[i] * NM_TO_FT;
            dispRange *= dispRange;

            if (dist < dispRange)
                break;
        }

        int maxIdx;

        // Reuse current range, within limits of course
        if (flags bitand (DBS1 bitor DBS2) or mode == GMT or mode == SEA)
        {
            maxIdx = NUM_RANGES - 3;
        }
        else
        {
            maxIdx = NUM_RANGES - 2;
        }

        if (mode not_eq GM)
        {
            ClearFlagBit(DBS1);
            ClearFlagBit(DBS2);
        }

        curRangeIdx = i;

        if (curRangeIdx > maxIdx)
            curRangeIdx = maxIdx;
        else if (curRangeIdx < 0)
            curRangeIdx = 0;

        gmRangeIdx = curRangeIdx;
        displayRange = rangeScales[curRangeIdx];
        tdisplayRange = displayRange * NM_TO_FT;
        SetGMScan();
    }
}

void RadarDopplerClass::SetGMScan(void)
{
    float scanBottom, scanTop;
    float nearEdge, farEdge;
    static const float TwoRootTwo = 2.0f * (float)sqrt(2.0f);


    if (flags bitand EXP)
    {
        groundMapRange = tdisplayRange * 0.125F;

        if (displayRange <= 20.1f)
        {
            groundMapLOD = 2;
        }
        else
        {
            groundMapLOD = 3;
        }

        //azScan = 60.0F * DTR; // Radar still scans full volume, but only displays a subset...
        //MI az is set thru the OSB now
        if ( not g_bRealisticAvionics or not g_bAGRadarFixes)
            azScan = 60.0F * DTR;
        else if (g_bRealisticAvionics and g_bAGRadarFixes)
        {
            if (mode == GM or mode == SEA)
                curAzIdx = gmAzIdx;
            else
                curAzIdx = gmtAzIdx;

            azScan = rwsAzs[curAzIdx];
        }
    }
    else if (flags bitand DBS1)
    {
        groundMapRange = tdisplayRange * 0.125F;

        if (displayRange <= 20.1f)
        {
            groundMapLOD = 1;
        }
        else
        {
            groundMapLOD = 2;
        }

        //   azScan = atan2( TwoRootTwo*groundMapRange, distance from platform to GMat );
        //azScan = 15.0F * DTR;
        //MI az is set thru the OSB now
        if ( not g_bRealisticAvionics or not g_bAGRadarFixes)
            azScan = 15.0F * DTR;
        else if (g_bRealisticAvionics and g_bAGRadarFixes)
        {
            if (mode == GM or mode == SEA)
                curAzIdx = gmAzIdx;
            else
                curAzIdx = gmtAzIdx;

            azScan = rwsAzs[curAzIdx];
        }
    }
    else if (flags bitand DBS2)
    {
        //TJL 11/17/03 Fixed DBS2 to display what is within DBS1 tick marks
        //      groundMapRange = tdisplayRange * 0.0625F;
        groundMapRange = tdisplayRange * 0.03F;

        //TJL 11/17/03 Deleted the min range test to get the new DBS2 range to work.
        //groundMapRange = min ( max (groundMapRange, 1.0F * NM_TO_FT), 7.0F * NM_TO_FT);

        if (displayRange <= 20.1f)
        {
            groundMapLOD = 0;
        }
        else
        {
            groundMapLOD = 1;
        }

        //   azScan = atan2( TwoRootTwo*groundMapRange, distance from platform to GMat );
        //azScan = 5.0F * DTR;
        //MI az is set thru the OSB now
        if ( not g_bRealisticAvionics or not g_bAGRadarFixes)
            azScan = 5.0F * DTR;
        else if (g_bRealisticAvionics and g_bAGRadarFixes)
        {
            if (mode == GM or mode == SEA)
                curAzIdx = gmAzIdx;
            else
                curAzIdx = gmtAzIdx;

            azScan = rwsAzs[curAzIdx];
        }
    }
    else // NORM
    {
        ShiAssert(flags bitand NORM); // If not, then what mode is this???
        groundMapRange = tdisplayRange * 0.5F;

        if (displayRange <= 20.1f)
        {
            groundMapLOD = 3;
        }
        else
        {
            groundMapLOD = 4;
        }

        //MI az is set thru the OSB now
        if ( not g_bRealisticAvionics or not g_bAGRadarFixes)
            azScan = MAX_ANT_EL;
        else if (g_bRealisticAvionics and g_bAGRadarFixes)
        {
            if (mode == GM or mode == SEA)
                curAzIdx = gmAzIdx;
            else
                curAzIdx = gmtAzIdx;

            azScan = rwsAzs[curAzIdx];
        }
    }

    nearEdge = displayRange * NM_TO_FT - groundMapRange;
    farEdge = displayRange * NM_TO_FT + groundMapRange;
    scanTop = -(float)atan(-platform->ZPos() / farEdge);
    scanBottom = max(-MAX_ANT_EL, -(float)atan(-platform->ZPos() / nearEdge));
    // JB 010707
    //bars = (int)((scanTop - scanBottom) / (barWidth * 2.0F) + 0.5F);
    bars = max(1, (int)((scanTop - scanBottom) / (barWidth * 2.0F) + 0.5F));

    // For expanded modes, you'll need to set "at" to the center
    // of the expanded region and reduce range to the size of the footprint
    // to be expanded.  "center" is when the MFD center should fall in world space.
    //
    // SetGimbalLimit should also be done only when the value changes (typically at setup only)
    // It allows you to provide a max ATA beyond which the image is clipped (approximatly)
    //
    // Just use privateDisplay since that is the one and only GM type renderer
    // and we want it in the right mode when it is used again even if it isn't current.
    if (privateDisplay)
    {
        ((RenderGMComposite*)privateDisplay)->SetRange(groundMapRange, groundMapLOD);

        // ((RenderGMComposite*)privateDisplay)->SetGimbalLimit( azScan );
        //MI take the azimuth we've selected thru the OSB
        if ( not g_bRealisticAvionics or not g_bAGRadarFixes)
            ((RenderGMComposite*)privateDisplay)->SetGimbalLimit(MAX_ANT_EL);
        else
            ((RenderGMComposite*)privateDisplay)->SetGimbalLimit(azScan);
    }
}


void RadarDopplerClass::GMDisplay(void)
{
    //START_PROFILE("GMDISPLAY");
    Tpoint center;
    int i = 0;
    float len = 0.0F;
    int curFov = 0;
    float dx = 0.0F, dy = 0.0F, dz = 0.0F, cosAz = 0.0F, sinAz = 0.0F, rx = 0.0F, ry = 0.0F;
    float groundLookEl = 0.0F, baseAz = 0.0F, baseEl = 0.0F;
    int beamPercent = 0;
    mlTrig trig = {0.0F};
    float vpLeft = 0.0F, vpTop = 0.0F, vpRight = 0.0F, vpBottom = 0.0F;
    int tmpColor = display->Color();

    // Mode Step ?
    if (fovStepCmd)
    {
        fovStepCmd = 0;
        curFov = flags bitand 0x0f;
        flags -= curFov;
        curFov = curFov << 1;

        if (mode == GM and displayRange <= 40.0F)
        {
            if (curFov > DBS2)
                curFov = NORM;
        }
        else
        {
            if (curFov > EXP)
                curFov = NORM;
        }

        flags += curFov;
        SetGMScan();
    }

    // Find lookat point
    if ( not (flags bitand FZ))
    {
        viewFrom.x = platform->XPos();
        viewFrom.y = platform->YPos();
        viewFrom.z = platform->ZPos();

        headingForDisplay = platform->Yaw();
        mlSinCos(&trig, headingForDisplay);

        if ( not lockedTarget)
        {
            if (IsSet(SP) and ( not IsSet(SP_STAB)))  // (flags bitand SP) // MD -- 20040215: make sure we don't snow plow after ground stabilizing
            {
                // We're in snowplow, so look out in front of the aircraft
                GMat.x = viewFrom.x + tdisplayRange * 0.5F * trig.cos;
                GMat.y = viewFrom.y + tdisplayRange * 0.5F * trig.sin;
            }
            else
            {
                // We're in steer point mode, so look there
                GMat.x = viewCenter.x;
                GMat.y = viewCenter.y;
            }

            if ( not (flags bitand NORM))
            {
                // We're zoomed in, so track the cursors
                GMat.x += viewOffsetInertial.x;
                GMat.y += viewOffsetInertial.y;
            }
        }
        else
        {
            if ( not (flags bitand NORM))
            {
                // We're zoomed in, so look at the target
                GMat.x = lockedTarget->BaseData()->XPos();
                GMat.y = lockedTarget->BaseData()->YPos();
            }
            else if (IsSet(SP) and ( not IsSet(SP_STAB)))  // (flags bitand SP) // MD -- 20040215: if we aren't SP ground stabilized
            {
                // We're in snowplow, so look out in front of the aircraft
                GMat.x = viewFrom.x + tdisplayRange * 0.5F * trig.cos;
                GMat.y = viewFrom.y + tdisplayRange * 0.5F * trig.sin;
                viewOffsetInertial.x = lockedTarget->BaseData()->XPos() - GMat.x;
                viewOffsetInertial.y = lockedTarget->BaseData()->YPos() - GMat.y;
            }
            else
            {
                // We're in steer point NORM mode, so look there
                GMat.x = viewCenter.x;
                GMat.y = viewCenter.y;
            }
        }

        GMat.z  = OTWDriver.GetGroundLevel(GMat.x, GMat.y);

        groundDesignateX = GMat.x;
        groundDesignateY = GMat.y;
        groundDesignateZ = GMat.z;
    }
    else
    {
        mlSinCos(&trig, headingForDisplay);
    }

    // We now now where the radar is looking.  Now decide where the display is centered
    if (flags bitand NORM)
    {
        center.x = viewFrom.x + tdisplayRange * 0.5F * trig.cos;
        center.y = viewFrom.y + tdisplayRange * 0.5F * trig.sin;
    }
    else
    {
        center.x = GMat.x;
        center.y = GMat.y;
    }

    // Find inertial display center
    cosAz = trig.cos;
    sinAz = trig.sin;
    dx = GMat.x - viewFrom.x;
    dy = GMat.y - viewFrom.y;
    dz = GMat.z - viewFrom.z;

    groundLookAz = (float)atan2(dy, dx);
    groundLookEl = (float)atan(-dz / (float)sqrt(dy * dy + dx * dx + .1f));

    // Update the deltas if the cursor is off center
    if (flags bitand NORM)
    {
        dx += viewOffsetInertial.x;
        dy += viewOffsetInertial.y;
        dz  = OTWDriver.GetGroundLevel(GMat.x + viewOffsetInertial.x, GMat.y + viewOffsetInertial.y) - viewFrom.z;

        // position the seeker volume center
        baseAz = (float)atan2(dy, dx);
        baseEl = (float)atan(-dz / (float)sqrt(dy * dy + dx * dx + .1f));
    }
    else
    {
        // position the seeker volume center
        baseAz = groundLookAz;
        baseEl = groundLookEl;
    }


    // Position the cursor
    cursorY = (cosAz * dx + sinAz * dy) / (tdisplayRange * 0.5F) - 1.0F;
    cursorX = (-sinAz * dx + cosAz * dy) / (tdisplayRange * 0.5F);


    cursorX = max(min(cursorX, 0.95F), -0.95F);
    cursorY = max(min(cursorY, 0.95F), -0.95F);


    // remove body rotations
    seekerAzCenter =  baseAz - platform->Yaw();

    if (seekerAzCenter > 180.0F * DTR)
        seekerAzCenter -= 360.0F * DTR;
    else if (seekerAzCenter < -180.0F * DTR)
        seekerAzCenter += 360.0F * DTR;

    seekerElCenter = baseEl - platform->Pitch();

    // Blown antenna limit?
    if (fabs(seekerAzCenter) > MAX_ANT_EL)
    {
        //MI why would we want to do this?
        if ( not g_bRealisticAvionics or not g_bAGRadarFixes)
        {
            viewOffsetRel.x = 0.0F;
            viewOffsetRel.y = 0.0F;
            SetAimPoint(0.0F, 0.0F);
            seekerAzCenter = 0.0F;
        }
        else if (g_bRealisticAvionics and g_bAGRadarFixes)
        {
            if (seekerAzCenter > 0.0F)
                seekerAzCenter = MAX_ANT_EL;
            else
                seekerAzCenter = -MAX_ANT_EL;
        }

        curFov = flags bitand 0x0f;
        flags -= curFov;
        flags += NORM;
        SetGMScan();
        DropGMTrack();
    }

    //STOP_PROFILE("GMDISPLAY");
    // Draw the GM Display
    if (display)
    {
        //START_PROFILE("GMDISPLAY DRAW1");
        // Draw the actual image in the center
        display->GetViewport(&vpLeft, &vpTop, &vpRight, &vpBottom);
        display->SetViewportRelative(DisplayAreaViewLeft, DisplayAreaViewTop, DisplayAreaViewRight, DisplayAreaViewBottom);

        // edg: DON'T DRAW when the display is a CANVAS
        {
            if (gainCmd not_eq 0.0F)
            {
                if (gainCmd < 1)
                    GainPos -= 0.5F;
                else
                    GainPos += 0.5F;

                if (GainPos < -5)
                    GainPos = -5;

                if (GainPos > 20)
                    GainPos = 20;

                if (GainPos < 0)
                    curgain = 1 * pow(0.8f, - GainPos); //JAM 27Sep03 - These are floats
                else if (GainPos > 0)
                    curgain = 1 * pow(1.25f, GainPos); //JAM 27Sep03 - These are floats
                else
                    curgain = 1;

                //MI
                if ( not g_bRealisticAvionics or not g_bAGRadarFixes)
                    ((RenderGMComposite*)display)->SetGain(((RenderGMComposite*)display)->GetGain()*gainCmd);
                else
                    ((RenderGMComposite*)display)->SetGain(curgain);

                gainCmd = 0.0F;
            }

            // MD -- 20040108: if the RNG knob is mapped to an analog axis, then the key commands are
            // equivalent to the GAIN rocker switch on the top corner of the MFD -- the rocker sets the
            // baseline value and then the change in value of the RNG knob can add or subtract 20% of the
            // set baseline depending on direction of rotation and absolute magnitude of the movement.  Note:
            // this means that if you enter GM modes with the knob positioned all the way at one end of its
            // travel, there won't be a way to change the gain in that direction of travel using the knob.
            // Strange but true: that's how it works in the real thing apparently.
            // At the limits of the gain range, the knob should activate "synthetic enhancement" of the radar
            // picture.  Since the current radar model doesn't support that, For now this code merely
            // overdrives the gain for want of something better to do

            if (IO.AnalogIsUsed(AXIS_RANGE_KNOB))
            {
                int CurrentPos = IO.GetAxisValue(AXIS_RANGE_KNOB);

                if (CurrentPos not_eq lastRngKnobPos)
                {
                    float diff = ((float)abs(CurrentPos - lastRngKnobPos) / 10000.0F) * (0.4F * curgain); // +/-20% -> 40% range total variation(?)...looks better so assume yes ;)
                    float newgain = ((RenderGMComposite*)display)->GetGain();

                    if (CurrentPos > lastRngKnobPos)
                        newgain += diff;
                    else
                        newgain -= diff;

                    ((RenderGMComposite*)display)->SetGain(newgain);
                    lastRngKnobPos = CurrentPos;
                }
            }

            if (InitGain and g_bRealisticAvionics and g_bAGRadarFixes)
            {
                curgain = 1 * pow(1.25f, GainPos); //JAM 27Sep03 - These are floats
                ((RenderGMComposite*)display)->SetGain(curgain);

                // MD -- 20040108: save knob position on INIT if analog axis is mapped to RNG
                if (IO.AnalogIsUsed(AXIS_RANGE_KNOB))
                    lastRngKnobPos = IO.GetAxisValue(AXIS_RANGE_KNOB);

                InitGain = FALSE;
            }

            if ( not (flags bitand FZ))
            {
                // Decide how far along the beam scan is
                if (beamAz >  azScan)
                    beamAz =  azScan;

                if (beamAz < -azScan)
                    beamAz = -azScan;

                float tempres = beamAz / azScan;
                beamPercent = FloatToInt32((1.0f + beamAz / azScan) * 50.0f);

                if (scanDir == ScanRev)
                {
                    beamPercent = 100 - beamPercent;
                }

                // OW
                //((RenderGMComposite*)display)->EndDraw();

                // Advance the beam
                ShiAssert(beamPercent <= 100);
                // From, At, Center == Ownship, Look Point, Center of MFD in world

                // 2002-04-03 MN send the seekers current center to SetBeam to modify the gimbal borders
                float cursorAngle = 0.0F;

                if (g_bRealisticAvionics and g_bAGRadarFixes)
                {
                    float value = 60.0F * DTR;

                    if (azScan < value)
                        cursorAngle = seekerAzCenter;
                }

                //((RenderGMComposite*)display)->StartDraw();
                ((RenderGMComposite*)display)->SetBeam(&viewFrom, &GMat, &center, headingForDisplay, baseAz + beamAz, beamPercent, cursorAngle, (scanDir == ScanFwd), (flags bitand (DBS1 bitor DBS2)) ? true : false);
            }

            // OW - restore render target and start new scene
            ((RenderGMComposite*)display)->StartDraw();

            // COBRA - RED - Started a New Frame, assert again view port
            display->SetViewport(vpLeft, vpTop, vpRight, vpBottom);
            display->SetViewportRelative(DisplayAreaViewLeft, DisplayAreaViewTop, DisplayAreaViewRight, DisplayAreaViewBottom);

            // Generate the radar imagery
            //MI
            if (g_bRealisticAvionics and g_bAGRadarFixes)
            {
                if ( not lockedTarget)
                    ((RenderGMComposite*)display)->DrawComposite(&center, headingForDisplay);
            }
            else
                ((RenderGMComposite*)display)->DrawComposite(&center, headingForDisplay);


            // ((RenderGMComposite*)display)->FinishFrame();
            // ((RenderGMComposite*)display)->DebugDrawRadarImage( OTWDriver.OTWImage );
            // ((RenderGMComposite*)display)->StartFrame();
            // ((RenderGMComposite*)display)->DebugDrawLeftTexture( OTWDriver.renderer );

#if 0
            {
                float dx, dy;
                char string[80];

                dx = GMat.x - viewFrom.x;
                dy = GMat.y - viewFrom.y;

                sprintf(string, "beamAz %0f, beamDir %0d, beamPercent %0d", beamAz * 180.0f / PI, (scanDir == ScanFwd) ? 1 : -1, beamPercent);
                display->ScreenText(320.0f, 10.0f, string);

                sprintf(string, "from <%0f,%0f>  at <%0f,%0f>", viewFrom.x, viewFrom.y, GMat.x, GMat.y);
                display->ScreenText(320.0f, 18.0f, string);

                sprintf(string, "dx %0f, dy%0f, lookAngle %0f", dx, dy, atan2(dx, dy) * 180.0f / PI);
                display->ScreenText(320.0f, 26.0f, string);
            }
#endif

        }


        //STOP_PROFILE("GMDISPLAY DRAW1");
        //START_PROFILE("GMDISPLAY DRAW2");

        // Make sure to draw in green
        display->SetColor(tmpColor);

        // Add the Airplane if in freeze mode
        if (flags bitand FZ)
        {
            // Note the axis switch from NED to screen
            dx = platform->XPos() - GMXCenter;
            dy = platform->YPos() - GMYCenter;

            ry = trig.cos * dx + trig.sin * dy;//me123 from - to +
            rx = -trig.sin * dx + trig.cos * dy;//me123 from + to -

            rx /= groundMapRange;
            ry /= groundMapRange;

            display->AdjustOriginInViewport(rx, ry);
            display->AdjustRotationAboutOrigin(platform->Yaw() - headingForDisplay);
            //display->Line (0.1F, 0.0F, -0.1F, 0.0F);  //JPG 4 Mar 03  Make a circle with a line stickin' out
            display->Circle(0.0F, 0.0F, 0.05F);
            display->Line(0.0F, 0.13F, 0.0F, -0.01F);
            display->ZeroRotationAboutOrigin();
            display->AdjustOriginInViewport(-rx, -ry);
        }

        if (flags bitand NORM)
        {
            // Add FTT Diamond if needed
            if (lockedTarget)
            {
                //MI
                if (g_bRealisticAvionics and g_bAGRadarFixes)
                {
                    static const float size = 0.065F;
                    display->Tri(cursorX, cursorY, cursorX + size, cursorY, cursorX, cursorY + size);
                    display->Tri(cursorX, cursorY, cursorX - size, cursorY, cursorX, cursorY - size);
                    display->Tri(cursorX, cursorY, cursorX + size, cursorY, cursorX, cursorY - size);
                    display->Tri(cursorX, cursorY, cursorX - size, cursorY, cursorX, cursorY + size);
                }
                else
                {
                    display->Line(cursorX + 0.1F, cursorY, cursorX, cursorY + 0.1F);
                    display->Line(cursorX + 0.1F, cursorY, cursorX, cursorY - 0.1F);
                    display->Line(cursorX - 0.1F, cursorY, cursorX, cursorY + 0.1F);
                    display->Line(cursorX - 0.1F, cursorY, cursorX, cursorY - 0.1F);
                }
            }

            // RV - I-Hawk
            display->SetColor(GetMfdColor(MFD_GMSCOPE_CURSOR));

            // Add Cursor
            display->Line(-1.0F, cursorY, 1.0F, cursorY);
            display->Line(cursorX, -1.0F, cursorX, 1.0F);


            // Expansion Cues
            if (g_bRealisticAvionics and g_bAGRadarFixes)
            {
                float len = 0.065F;
                display->Line(cursorX + 0.25F, cursorY + len, cursorX + 0.25F, cursorY - len);
                display->Line(cursorX - 0.25F, cursorY + len, cursorX - 0.25F, cursorY - len);
                display->Line(cursorX + len, cursorY + 0.25F, cursorX - len, cursorY + 0.25F);
                display->Line(cursorX + len, cursorY - 0.25F, cursorX - len, cursorY - 0.25F);
            }
            else
            {
                display->Line(cursorX + 0.25F, cursorY + 0.1F, cursorX + 0.25F, cursorY - 0.1F);
                display->Line(cursorX - 0.25F, cursorY + 0.1F, cursorX - 0.25F, cursorY - 0.1F);
                display->Line(cursorX + 0.1F, cursorY + 0.25F, cursorX - 0.1F, cursorY + 0.25F);
                display->Line(cursorX + 0.1F, cursorY - 0.25F, cursorX - 0.1F, cursorY - 0.25F);
            }

            display->SetColor(tmpColor);

            // Add the Arcs
            //MI

            // RV - I-Hawk
            display->SetColor(GetMfdColor(MFD_GMSCOPE_ARCS));

            if (g_bRealisticAvionics and g_bAGRadarFixes)
            {
                if ( not lockedTarget)
                {
                    if (displayRange > 10.0F)
                    {
                        for (i = 0; i < 3; i++)
                        {
                            display->Arc(0.0F, -1.0F, (i + 1) * 0.5F, 213.0F * DTR, 333.0F * DTR);
                        }
                    }
                    else
                    {
                        display->Arc(0.0F, -1.0F, 1.0F, 213.0F * DTR, 333.0F * DTR);
                    }
                }
            }
            else
            {
                if (displayRange > 10.0F)
                {
                    for (i = 0; i < 3; i++)
                    {
                        display->Arc(0.0F, -1.0F, (i + 1) * 0.5F, 213.0F * DTR, 333.0F * DTR);
                    }
                }
                else
                {
                    display->Arc(0.0F, -1.0F, 1.0F, 213.0F * DTR, 333.0F * DTR);
                }

                display->SetColor(tmpColor);   // RV - I-Hawk - Return to green

                // RV - I-Hawk
                display->SetColor(GetMfdColor(MFD_GMSCOPE_CURSOR));

                // Lines are at 60 degrees
                display->Line(0.0F, -1.0F,  1.0F, -0.5F);
                display->Line(0.0F, -1.0F, -1.0F, -0.5F);
            }

        }
        else
        {
            // RV - I-Hawk
            display->SetColor(GetMfdColor(MFD_GMSCOPE_CURSOR));

            // Add True Cursor // JPG 9 Dec 03 - This is the SA cue - made it half size
            display->Line(cursorX - 0.05F, cursorY, cursorX + 0.05F, cursorY);
            display->Line(cursorX, cursorY + 0.05F, cursorX, cursorY - 0.05F);

            // Add FTT Diamond if needed
            if (lockedTarget)
            {
                //MI
                if (g_bRealisticAvionics and g_bAGRadarFixes)
                {
                    static const float size = 0.065F;
                    display->Tri(0.0F, 0.0F, size, 0.0F, 0.0F, size);
                    display->Tri(0.0F, 0.0F, -size, 0.0F, 0.0F, size);
                    display->Tri(0.0F, 0.0F, size, 0.0F, 0.0F, -size);
                    display->Tri(0.0F, 0.0F, -size, 0.0F, 0.0F, -size);
                }
                else
                {
                    display->Line(0.1F, 0.0F, 0.0F,  0.1F);
                    display->Line(0.1F, 0.0F, 0.0F, -0.1F);
                    display->Line(-0.1F, 0.0F, 0.0F,  0.1F);
                    display->Line(-0.1F, 0.0F, 0.0F, -0.1F);
                }
            }

            display->Line(-1.0F, 0.0F, 1.0F, 0.0F);
            display->Line(0.0F, -1.0F, 0.0F, 1.0F);

            // Add scale reference
            len = 1500.0F / groundMapRange;
            display->Line(-0.75F, 0.75F, -0.75F, 0.8F);
            display->Line(-0.75F, 0.8F, -0.75F + len, 0.8F);
            display->Line(-0.75F + len, 0.8F, -0.75F + len, 0.75F);

            if (g_bRealisticAvionics and g_bAGRadarFixes)
            {
                if (flags bitand DBS1)
                {
                    float len = 0.065F;
                    display->Line(0.25F, -len, 0.25F, len);
                    display->Line(-0.25F, -len, -0.25F, len);
                    display->Line(len, 0.25F, -len, 0.25F);
                    display->Line(len, -0.25F, -len, -0.25F);
                }
            }
            else
            {
                if (flags bitand DBS1)
                {
                    // Expansion Cues
                    display->Line(0.25F,  0.1F,  0.25F, -0.1F);
                    display->Line(-0.25F,  0.1F, -0.25F, -0.1F);
                    display->Line(0.1F,  0.25F, -0.1F,  0.25F);
                    display->Line(0.1F, -0.25F, -0.1F, -0.25F);
                }
            }
        }

        display->SetColor(tmpColor);   // RV - I-Hawk - Return to green

        //STOP_PROFILE("GMDISPLAY DRAW2");
        //START_PROFILE("GMDISPLAY DRAW3");

        display->SetViewport(vpLeft, vpTop, vpRight, vpBottom);

        //MI
        if (g_bRealisticAvionics and g_bAGRadarFixes)
        {
            static float MAX_GAIN = 25.0F;
            float x, y = 0;
            GetButtonPos(19, &x, &y);
            x += 0.02F;
            y = 0.95F;
            float y1 = 0.2F;
            float diff = y - y1;
            float step = y1 / MAX_GAIN;
            float pos = GainPos * step + (5 * step);
            //add the gain range
            display->Line(x, y, x + 0.07F, y);
            display->Line(x, y, x, y - y1);
            display->Line(x, y - y1, x + 0.07F, y - y1);
            //Gain
            mlTrig trig;
            float Angle = 45.0F * DTR;
            float lenght = 0.08F;
            mlSinCos(&trig, Angle);
            float pos1 = trig.sin * lenght;
            display->AdjustOriginInViewport(x, diff + pos);
            display->Line(0.0F, 0.0F, pos1, pos1);
            display->Line(0.0F, 0.0F, pos1, -pos1);
            display->CenterOriginInViewport();
        }

        //STOP_PROFILE("GMDISPLAY DRAW3");
        //START_PROFILE("GMDISPLAY DRAW4");
        // Common Radar Stuff // ASSOCIATOR 3/12/03: Reversed drawing order
        DrawAzElTicks();
        DrawScanMarkers();

        // Draw Range arrows
        if (IsAGDclt(Arrows) == FALSE)
            DrawRangeArrows();

        if (IsAADclt(Rng) == FALSE)
            DrawRange();

        display->SetColor(tmpColor);

        if (IsAGDclt(MajorMode) == FALSE)
        {
            // Add Buttons
            switch (mode)
            {
                case GM:
                    LabelButton(0, "GM");
                    break;

                case GMT:
                    LabelButton(0, "GMT");
                    break;

                case SEA:
                    LabelButton(0, "SEA");
                    break;
            }
        }

        if (IsAGDclt(SubMode) == FALSE)
        {
            if (g_bRealisticAvionics and g_bAGRadarFixes)
            {
                if (IsSet(AutoAGRange))
                    LabelButton(1, "AUTO");
                else
                    LabelButton(1, "MAN");
            }
            else
                LabelButton(1, "MAN");
        }

        if (IsAGDclt(Fov) == FALSE)
        {
            if (flags bitand NORM)
            {
                //MI
                if ( not g_bRealisticAvionics or not g_bAGRadarFixes)
                    LabelButton(2, "NRM");
                else
                    LabelButton(2, "NORM");
            }
            else
            {
                if (flags bitand EXP)
                    LabelButton(2, "EXP");
                else if (flags bitand DBS1)
                    LabelButton(2, "DBS1");
                else if (flags bitand DBS2)
                    LabelButton(2, "DBS2");
            }
        }

        if (IsAGDclt(Ovrd) == FALSE)
            LabelButton(3, "OVRD", NULL, not IsEmitting());

        if (IsAGDclt(Cntl) == FALSE)
            LabelButton(4, "CNTL", NULL, IsSet(CtlMode));

        if (IsSet(MenuMode bitor CtlMode))
            MENUDisplay();
        else
        {
            if (IsAGDclt(BupSen) == FALSE)
                LabelButton(5, "BARO");

            if (IsAGDclt(FzSp) == FALSE)
            {
                LabelButton(6, "FZ", NULL, IsSet(FZ));
                LabelButton(7, "SP", NULL, IsSet(SP));
                LabelButton(9, "STP", NULL, not IsSet(SP));
            }

            if (IsAGDclt(Cz) == FALSE)
                LabelButton(8, "CZ");

            AGBottomRow();

            if (IsAGDclt(AzBar) == FALSE)
            {
                //MI
                if (g_bRealisticAvionics and g_bAGRadarFixes)
                {
                    char str[10] = "";
                    sprintf(str, "%.0f", displayAzScan * 0.1F * RTD);
                    ShiAssert(strlen(str) < sizeof(str));
                    LabelButton(17, "A", str);
                }
                else
                    LabelButton(17, "A", "6");
            }
        }

        //STOP_PROFILE("GMDISPLAY DRAW4");
    }

    //STOP_PROFILE("GMDISPLAY DRAW");
    //START_PROFILE("GMDISPLAY");

    // MD --20040306: Adding the TTG display for when you have GM STP mode or SP mode with a ground
    // stabilized cursor position

    if (SimDriver.GetPlayerAircraft() and ( not IsSet(SP) or (IsSet(SP) and IsSet(SP_STAB))))
    {
        char tmpStr[24];
        float ttg = 0.0F;
        int hr = 0, minute = 0, sec = 0;

        // MD -- 20040515: watch out  Until MARKs are fixed properly, curWaypoint may not point to a real waypoint
        // so check the pointer to avoid a CTD here.
        if (SimDriver.GetPlayerAircraft() and 
 not F4IsBadReadPtr(SimDriver.GetPlayerAircraft()->curWaypoint, sizeof(WayPointClass))
           )
        {
            float x, y, z, dx, dy;

            if (IsSet(SP) and IsSet(SP_STAB) and GMSPWaypt())
                GMSPWaypt()->GetLocation(&x, &y, &z);
            else
                SimDriver.GetPlayerAircraft()->curWaypoint->GetLocation(&x, &y, &z);

            dx = x - SimDriver.GetPlayerAircraft()->XPos();
            dy = y - SimDriver.GetPlayerAircraft()->YPos();

            ttg = ((float)sqrt(dx * dx + dy * dy)) / SimDriver.GetPlayerAircraft()->GetVt();
        }

        if ((ttg > 0.0F) and (FloatToInt32(ttg) > 0.0F))
        {
            // burn any days in the number
            hr = FloatToInt32(ttg / (3600.0F * 24.0F));
            ttg -= hr * 3600.0F * 24.0F;
            hr  = FloatToInt32(ttg / 3600.0F);
            hr  = max(hr, 0);
            ttg -= hr * 3600.0F;
            ttg = max(ttg, 0.0F);
            minute = FloatToInt32(ttg / 60.0F);
            ttg -= minute * 60.0F;
            sec = FloatToInt32(ttg);

            minute = max(min(minute, 999), 0);
            sec = max(min(sec, 59), 0);

            if (hr not_eq 0)
                sprintf(tmpStr, "%03d:%02d", abs(minute), sec);   //JPG 5 Feb 04
            else if (sec >= 0)
            {
                if ( not g_bRealisticAvionics)
                    sprintf(tmpStr, "   %02d:%02d", abs(minute), sec);
                else
                    sprintf(tmpStr, "%03d:%02d", abs(minute), sec);   //JPG "%02d:%02d"
            }
            else
            {
                if ( not g_bRealisticAvionics)
                    sprintf(tmpStr, "  -%02d:%02d", abs(minute), abs(sec));
                else
                    sprintf(tmpStr, "-%02d:%02d", abs(minute), abs(sec));
            }
        }
        else
        {
            strcpy(tmpStr, "XX:XX");
        }

        display->TextRight(0.68F, -0.63F, tmpStr);
    }

    /*------------------*/
    /* Auto Range Scale */
    /*------------------*/
    if (lockedTarget)
    {
        //MI
        if (g_bRealisticAvionics and g_bAGRadarFixes)
        {
            if (IsSet(AutoAGRange))
            {
                if (lockedTarget->localData->range > 0.9F * tdisplayRange and curRangeIdx < NUM_RANGES - 1)
                    rangeChangeCmd = 1;
                else if (lockedTarget->localData->range < 0.4F * tdisplayRange and curRangeIdx > 0)
                    rangeChangeCmd = -1;
            }
        }
        else
        {
            if (lockedTarget->localData->range > 0.9F * tdisplayRange and curRangeIdx < NUM_RANGES - 1)
                rangeChangeCmd = 1;
            else if (lockedTarget->localData->range < 0.4F * tdisplayRange and curRangeIdx > 0)
                rangeChangeCmd = -1;
        }

        //MI
        if (g_bRealisticAvionics and g_bAGRadarFixes)
        {
            scanDir = ScanNone;
            beamAz = lockedTarget->localData->az;
            beamEl = lockedTarget->localData->el;
        }
    }

    //STOP_PROFILE("GMDISPLAY");
}


void RadarDopplerClass::AddTargetReturnCallback(void* self, RenderGMRadar* renderer, bool Shaping)
{
    ((RadarDopplerClass*)self)->AddTargetReturns(renderer, Shaping);
}


void RadarDopplerClass::AddTargetReturns(RenderGMRadar* renderer, bool Shaping)
{
    float dx, dy;
    float rx, ry, GainScale = 1.0f;
    float cosAz, sinAz;
    mlTrig trig;
    GMList *curNode;
    float       minDist = 0.05F;

    mlSinCos(&trig, headingForDisplay);
    cosAz =  trig.cos;
    sinAz = -trig.sin;

    // Offset the spots correctly
    if ( not (flags bitand FZ))
    {
        if (flags bitand NORM)
        {
            // Find center of scope
            GMXCenter = platform->XPos() + tdisplayRange * trig.cos * 0.5F;
            GMYCenter = platform->YPos() + tdisplayRange * trig.sin * 0.5F;
        }
        else
        {
            GMXCenter = GMat.x;
            GMYCenter = GMat.y;
        }
    }

    // Draw the appropriate type of targets
    // TODO:  We should select _any_ large targets for GM, and only moving targets for GMT/SEA
    if (mode == GM)
    {
        curNode = GMFeatureListRoot;
        GainScale = 1.0f;
    }
    else   // GMT or SEA
    {
        curNode = GMMoverListRoot;
        GainScale = 4.0f;
    }

    // Clear target under cursor;
    targetUnderCursor = FalconNullId;

    while (curNode)
    {
        dx = curNode->Object()->XPos() - GMXCenter;
        dy = curNode->Object()->YPos() - GMYCenter;

        // Note the axis switch from NED to screen
        ry = cosAz * dx - sinAz * dy;
        rx = sinAz * dx + cosAz * dy;

        if (F_ABS(rx) > groundMapRange and 
            F_ABS(ry) > groundMapRange)
        {
            curNode = curNode->next;
            continue;
        }

        rx /= groundMapRange;
        ry /= groundMapRange;

        // Check for scan width NOTE 0.57 = tan(30) (90.0 - the azimuth limit)
        if ((ry + 1.0F) / (F_ABS(rx) + 0.001F) > 0.57F or F_ABS(rx) > 1.0F)
        {
            if (curNode->Object()->IsSim() and ((SimBaseClass*)curNode->Object())->IsAwake())
            {
                DrawableObject *drawable = ((SimBaseClass*)curNode->Object())->drawPointer;

                //MI
                if (g_bRealisticAvionics and g_bAGRadarFixes)
                {
                    if ( not lockedTarget)
                        renderer->DrawBlip(drawable, GainScale, Shaping);
                }
                else
                    renderer->DrawBlip(drawable, GainScale, Shaping);
            }
            else
            {
                //MI
                if (g_bRealisticAvionics and g_bAGRadarFixes)
                {
                    if ( not lockedTarget)
                        renderer->DrawBlip(curNode->Object()->XPos(), curNode->Object()->YPos());
                }
                else
                    renderer->DrawBlip(curNode->Object()->XPos(), curNode->Object()->YPos());
            }
        }
        else
        {
            if (lockedTarget and curNode->Object() == lockedTarget->BaseData())
            {
                DropGMTrack();
            }
        }

        if (flags bitand NORM)
        {
            ry -= cursorY;
            rx -= cursorX;
        }

        if (F_ABS(rx) < minDist and F_ABS(ry) < minDist)
        {
            minDist = min(min(minDist, (float)F_ABS(rx)), (float)F_ABS(ry));
            targetUnderCursor = curNode->Object()->Id();
        }

        curNode = curNode->next;
    }

}


// Only used by virtual cockpit now...
void RadarDopplerClass::AddTargetReturnsOldStyle(GMList* curNode)
{
    float dx = 0.0F, dy = 0.0F;
    float rx = 0.0F, ry = 0.0F;
    float cosAz = 0.0F, sinAz = 0.0F;
    mlTrig trig = {0.0F};
    float minDist = groundMapRange;

    mlSinCos(&trig, headingForDisplay);
    cosAz =  trig.cos;
    sinAz = -trig.sin;

    // Offset the spots correctly
    if ( not (flags bitand FZ))
    {
        if (flags bitand NORM)
        {
            // Find center of scope
            GMXCenter = platform->XPos() + tdisplayRange * trig.cos * 0.5F;
            GMYCenter = platform->YPos() + tdisplayRange * trig.sin * 0.5F;
        }
        else
        {
            GMXCenter = GMat.x;
            GMYCenter = GMat.y;
        }
    }

    // Draw the spots
    while (curNode)
    {
        // Check distance from radar patch center
        dx = curNode->Object()->XPos() - GMat.x;
        dy = curNode->Object()->YPos() - GMat.y;

        if (fabs(dx) > groundMapRange or
            fabs(dy) > groundMapRange)
        {
            curNode = curNode->next;
            continue;
        }

        // Now check position on screen
        dx = curNode->Object()->XPos() - GMXCenter;
        dy = curNode->Object()->YPos() - GMYCenter;

        // Note the axis switch from NED to screen
        ry = cosAz * dx - sinAz * dy;
        rx = sinAz * dx + cosAz * dy;

        if (F_ABS(rx) > groundMapRange or
            F_ABS(ry) > groundMapRange)
        {
            curNode = curNode->next;
            continue;
        }

        rx /= groundMapRange;
        ry /= groundMapRange;

        // Check for scan width NOTE 0.57 = tan(30) (90.0 - the azimuth limit)
        if ((ry + 1.0F) / (F_ABS(rx) + 0.001F) > 0.57F or F_ABS(rx) > 1.0F)
        {
            //MI
            if (g_bRealisticAvionics and g_bAGRadarFixes)
            {
                if ( not lockedTarget)
                {
                    display->AdjustOriginInViewport(rx, ry);
                    DrawSymbol(Solid, 0.0F, 0);
                    display->AdjustOriginInViewport(-rx, -ry);
                }
            }
            else
            {
                display->AdjustOriginInViewport(rx, ry);
                DrawSymbol(Solid, 0.0F, 0);
                display->AdjustOriginInViewport(-rx, -ry);
            }
        }
        else
        {
            if (lockedTarget and curNode->Object() == lockedTarget->BaseData())
            {
                DropGMTrack();
            }
        }

        if (flags bitand NORM)
        {
            ry -= cursorY;
            rx -= cursorX;
        }

        if (F_ABS(rx) < minDist and F_ABS(ry) < minDist)
        {
            minDist = (float)min(min(minDist, F_ABS(rx)), F_ABS(ry));
            targetUnderCursor = curNode->Object()->Id();
        }

        curNode = curNode->next;
    }
}

void RadarDopplerClass::DoGMDesignate(GMList* curNode)
{
    float dx, dy;
    float rx, ry;
    float cosAz, sinAz;
    mlTrig trig;
    float minDist = 0.05F;

    // MD -- 20040215: adding a step in designation to the snow plow mode;  When you are in
    // SP and you first designate, that ground stabilizes the radar aim point and keys the
    // navigation systems to look at the pseudo waypoint defined by the cursor position as
    // the current steerpoint to use.
    if (IsSet(SP) and ( not IsSet(SP_STAB)))  // SP mode but not yet ground stabilized
    {
        float x = 0.0F, y = 0.0F, z = 0.0F;

        if ( not GMSPPseudoWaypt)
        {
            GMSPPseudoWaypt = new WayPointClass();
        }

        // calculate x/y/z's
        GetGMCursorPosition(&x, &y);

        z = OTWDriver.GetGroundLevel(x, y);

        GMSPPseudoWaypt->SetLocation(x, y, z);
        GMSPPseudoWaypt->SetWPArrive(SimLibElapsedTime);

        if (SimDriver.GetPlayerAircraft())
        {
            SimDriver.GetPlayerAircraft()->FCC->SetStptMode(FireControlComputer::FCCGMPseudoPoint);
            SimDriver.GetPlayerAircraft()->FCC->waypointStepCmd = 127;
            // shouldn't need this here but make sure that first designate command doesn't also lock a target
            SimDriver.GetPlayerAircraft()->FCC->designateCmd = FALSE;  // shouldn't need this here but make
        }

        SetFlagBit(SP_STAB);  // we are now in SP mode with a ground stabilized cursor aim point
        return;
    }
    else
    {
        mlSinCos(&trig, headingForDisplay);
        cosAz =  trig.cos;
        sinAz = -trig.sin;

        // Offset the spots correctly
        if ( not (flags bitand FZ))
        {
            if (flags bitand NORM)
            {
                // Find center of scope
                GMXCenter = platform->XPos() + tdisplayRange * trig.cos * 0.5F;
                GMYCenter = platform->YPos() + tdisplayRange * trig.sin * 0.5F;
            }
            else
            {
                GMXCenter = GMat.x;
                GMYCenter = GMat.y;
            }
        }

        // Draw the spots
        while (curNode)
        {
            dx = curNode->Object()->XPos() - GMXCenter;
            dy = curNode->Object()->YPos() - GMYCenter;

            // Note the axis switch from NED to screen
            ry = cosAz * dx - sinAz * dy;
            rx = sinAz * dx + cosAz * dy;

            if (F_ABS(rx) > groundMapRange or
                F_ABS(ry) > groundMapRange)
            {
                curNode = curNode->next;
                continue;
            }

            rx /= groundMapRange;
            ry /= groundMapRange;

            if (flags bitand NORM)
            {
                ry -= cursorY;
                rx -= cursorX;
            }

            if (F_ABS(rx) < minDist and F_ABS(ry) < minDist)
            {
                minDist = min(min(minDist, (float)F_ABS(rx)), (float)F_ABS(ry));

                if (designateCmd)
                {
                    SetGroundTarget(curNode->Object());
                }

                targetUnderCursor = curNode->Object()->Id();
            }

            curNode = curNode->next;
        }
    }
}

void RadarDopplerClass::FreeGMList(GMList* theList)
{
    GMList* tmp;

    while (theList)
    {
        tmp = theList;
        theList = theList->next;
        tmp->Release();
    }
}

RadarDopplerClass::GMList::GMList(FalconEntity* obj)
{
    F4Assert(obj);
    next = NULL;
    prev = NULL;
    object = obj;
    VuReferenceEntity(object);
    count = 1;
}

void RadarDopplerClass::GMList::Release(void)
{
    VuDeReferenceEntity(object);
    count --;

    if (count == 0)
        delete(this);
}


void RadarDopplerClass::SetGroundTarget(FalconEntity* newTarget)
{
    float cosAz, sinAz;
    mlTrig trig;

    if (lockedTarget and newTarget == lockedTarget->BaseData())
        return;

    SetSensorTargetHack(newTarget);

    if (newTarget)
    {
        if (flags bitand NORM)
        {
            mlSinCos(&trig, groundLookAz);
            cosAz = trig.cos;
            sinAz = trig.sin;

            viewOffsetInertial.x = lockedTarget->BaseData()->XPos() - GMat.x;
            viewOffsetInertial.y = lockedTarget->BaseData()->YPos() - GMat.y;

            viewOffsetRel.x =  cosAz * viewOffsetInertial.x + sinAz * viewOffsetInertial.y;
            viewOffsetRel.y = -sinAz * viewOffsetInertial.x + cosAz * viewOffsetInertial.y;

            viewOffsetRel.x /= tdisplayRange * 0.5F;
            viewOffsetRel.y /= tdisplayRange * 0.5F;
        }
        else
        {
            GMat.x = lockedTarget->BaseData()->XPos();
            GMat.y = lockedTarget->BaseData()->YPos();
            viewOffsetInertial.x = viewOffsetRel.x = 0.0F;
            viewOffsetInertial.y = viewOffsetRel.y = 0.0F;
        }
    }
}

void RadarDopplerClass::SetAGFreeze(int val)
{
    if (val)
        SetFlagBit(FZ);
    else
        ClearFlagBit(FZ);
}

void RadarDopplerClass::SetAGSnowPlow(int val)
{
    if (val)
        SetFlagBit(SP);
    else
        ClearFlagBit(SP);
}

void RadarDopplerClass::SetAGSteerpoint(int val)
{
    // MD -- 20040216: adding logic to support GM SP ground stabilization.
    // Here we need to ensure that the FCC is put back into standard waypoint
    // steering mode.
    if (IsSet(SP) and IsSet(SP_STAB))
    {
        if (SimDriver.GetPlayerAircraft())
            SimDriver.GetPlayerAircraft()->FCC->SetStptMode(FireControlComputer::FCCWaypoint);

        SimDriver.GetPlayerAircraft()->FCC->waypointStepCmd = 127;
        ClearFlagBit(SP_STAB);

        if (GMSPPseudoWaypt)
        {
            SetGMSPWaypt(NULL);
        }
    }

    if ( not val)
        SetFlagBit(SP);
    else
        ClearFlagBit(SP);
}

void RadarDopplerClass::ToggleAGfreeze()
{
    LastAGModes = 1; // ASSOCIATOR: moved to here so that the key command works the same as the MFD command

    if (flags bitand FZ)
        ClearFlagBit(FZ);
    else
        SetFlagBit(FZ);
}

// ASSOCIATOR: emulates MFD pushbutton so it remembers
void RadarDopplerClass::ToggleAGsnowPlow()
{
    if (flags bitand SP)
    {
        SetAGSteerpoint(TRUE);

        if (g_bRealisticAvionics and g_bAGRadarFixes)
            RestoreAGCursor();

        LastAGModes = 3;
    }
    else
    {
        SetAGSnowPlow(TRUE);

        if (g_bRealisticAvionics and g_bAGRadarFixes)
            RestoreAGCursor();

        LastAGModes = 2;
    }
}

/* // ASSOCIATOR: this is the old snowplow toggle that doesn't remember
void RadarDopplerClass::ToggleAGsnowPlow()
{
   if (flags bitand SP)
      ClearFlagBit (SP);
   else
      SetFlagBit(SP);
}
*/

void RadarDopplerClass::ToggleAGcursorZero()
{
    viewOffsetInertial.x = viewOffsetInertial.y = 0.0F;
    viewOffsetRel.x = viewOffsetRel.y = 0.0F;
}

int RadarDopplerClass::IsAG(void)
{
    int retval;

    if (mode == GM or mode == GMT or mode == SEA)
        // 2000-10-04 MODIFIED BY S.G. NEED TO KNOW WHICH MODE WE ARE IN
        //    retval = TRUE;
        retval = mode;
    else
        retval = FALSE;

    return retval;
}

void RadarDopplerClass::GetAGCenter(float* x, float* y)
{
    *x = GMat.x;
    *y = GMat.y;

    if ((flags bitand NORM))
    {
        *x += viewOffsetInertial.x;
        *y += viewOffsetInertial.y;
    }
}

// JPO - bottom row of MFD buttons.
void RadarDopplerClass::AGBottomRow()
{
    if (g_bRealisticAvionics)
    {
        if (IsAGDclt(Dclt) == FALSE) LabelButton(10, "DCLT", NULL, IsSet(AGDecluttered));

        if (IsAGDclt(Fmt1) == FALSE) DefaultLabel(11);

        //if (IsAGDclt(Fmt2) == FALSE) DefaultLabel(12); MI moved downwards
        if (IsAGDclt(Fmt3) == FALSE) DefaultLabel(13);

        if (IsAGDclt(Swap) == FALSE) DefaultLabel(14);

        //MI RF Switch info
        if (SimDriver.GetPlayerAircraft() and (SimDriver.GetPlayerAircraft()->RFState == 1 or
                                              SimDriver.GetPlayerAircraft()->RFState == 2))
        {
            FackClass* mFaults = ((AircraftClass*)(SimDriver.GetPlayerAircraft()))->mFaults;

            if (mFaults and not (mFaults->GetFault(FaultClass::fcc_fault) == FaultClass::xmtr))
            {
                if (SimDriver.GetPlayerAircraft()->RFState == 1)
                    LabelButton(12, "RDY", "QUIET");
                else
                    LabelButton(12, "RDY", "SILENT");
            }
        }
        else if (IsAGDclt(Fmt2) == FALSE) DefaultLabel(12);
    }
    else
    {
        LabelButton(10, "DCLT", NULL, IsSet(AGDecluttered));
        LabelButton(11, "SMS");
        LabelButton(13, "MENU", NULL, 1); //me123
        LabelButton(14, "SWAP");
    }
}

//TJL 11/19/03 Get the cursor to stick in SP mode
//void GetCursorXYFromWorldXY:: (CursorX, CursorY, WorldX, WorldY)
//{
//Code goes here;
//}

void RadarDopplerClass::GetGMCursorPosition(float* xLoc, float* yLoc)
{
    float xpos, ypos, cosAz, sinAz;
    mlTrig trig;

    mlSinCos(&trig, headingForDisplay);
    cosAz = trig.cos;
    sinAz = trig.sin;

    xpos = cursorX * 0.5F * tdisplayRange;
    ypos = (cursorY + 1.0F) * 0.5F * tdisplayRange;

    *yLoc = ypos * sinAz + xpos * cosAz;
    *xLoc = ypos * cosAz - xpos * sinAz;


    *xLoc += viewFrom.x;
    *yLoc += viewFrom.y;
}




#ifndef USE_HASH_TABLES

void RadarDopplerClass::GMMode(void)
{
    float ownX = 0.0F;
    float ownY = 0.0F;
    float ownZ = 0.0F;
    FalconEntity* testFeature = NULL;
    SimBaseClass* testObject = NULL;
    VuListIterator *walker = NULL; //MI
    GMList* curNode = GMFeatureListRoot;
    GMList* tmpList = NULL;
    GMList* tmp2 = NULL;
    GMList* lastList = NULL;
    float range = 0.0F;
    float radius = 0.0F;
    float canSee = 0.0F;
    float x = 0.0F;
    float y = 0.0F;
    float dx = 0.0F;
    float dy = 0.0F;
    Tpoint pos;
    float radarHorizon = 0.0F;
    mlTrig trig;

    mlSinCos(&trig, -platform->Yaw());

    // Find out where the beam hits the edge of the earth
    radius = EARTH_RADIUS_FT - platform->ZPos();
    radarHorizon = (float)sqrt(radius * radius - EARTH_RADIUS_FT * EARTH_RADIUS_FT);
    ownX = platform->XPos();
    ownY = platform->YPos();
    ownZ = platform->ZPos();

    VuListIterator featureWalker(SimDriver.combinedFeatureList);
    VuListIterator objectWalker(SimDriver.combinedList);

    // Parallel walk through the list of existing objects and the platforms
    // target list. Only draw the visible objects on the target list that
    // still exist. This test is to handle the case were an object is deleted
    // between the start of a refresh and it's visibility check.
    if (SimLibElapsedTime - lastFeatureUpdate > 500)
    {
        lastFeatureUpdate = SimLibElapsedTime;


        // Clear the head of the list of removed entities
        testFeature = (FalconEntity*)featureWalker.GetFirst();
        int c = 0;

        if (testFeature and curNode)
        {
            // MLR-NOTE SimCompare is like strcmp but with entity Ids
            while (curNode and SimCompare(curNode->Object(), testFeature) < 0)
            {
                // curNode and testFeature's id is higher than curNode->Object()'s is

                tmpList = curNode;
                curNode = curNode->next;
                tmpList->Release();
            }
        }

        GMFeatureListRoot = curNode;
        walker = &featureWalker;

        if (GMFeatureListRoot)
            GMFeatureListRoot->prev = NULL;

        //MI
        if ( not testFeature and g_bAGRadarFixes and g_bRealisticAvionics)
        {
            testFeature = (SimBaseClass*)objectWalker.GetFirst();
            walker = &objectWalker;
        }

        lastList = NULL;
        //while (testFeature)


        //ADDING SECTION
        //TJL 11/25/03 Fixes the 0.5 second stutter on Aircraft when in A/G mode
        // not g_bnoRadStutter turns off the fix; this section is original code. Fix is after this.
        while (testFeature and not g_bnoRadStutter)
        {

            if (isEmitting)

            {
                range = (float)sqrt(
                            (testFeature->XPos() - ownX) * (testFeature->XPos() - ownX) +
                            (testFeature->YPos() - ownY) * (testFeature->YPos() - ownY) +
                            (testFeature->ZPos() - ownZ) * (testFeature->ZPos() - ownZ));

                if (range < radarHorizon)
                {
                    if (testFeature->IsSim())
                    {
                        // Check for visibility
                        if (((SimBaseClass*)testFeature)->IsAwake())
                        {
                            radius = ((SimBaseClass*)testFeature)->drawPointer->Radius();
                            radius = radius * radius * radius * radius;
                            canSee = radius / range * tdisplayRange / groundMapRange;
                            ((SimBaseClass*)testFeature)->drawPointer->GetPosition(&pos);
                            testFeature->SetPosition(pos.x, pos.y, pos.z);

                            if (g_bRealisticAvionics and g_bAGRadarFixes)
                            {
                                if (walker == &objectWalker)
                                {
                                    // 2002-04-03 MN removed IsBattalion check, added Drawable::Guys here
                                    if (testFeature->GetVt() > 1.0F or /*testFeature->IsBattalion()*/
                                        ((SimBaseClass*)testFeature)->drawPointer and 
                                        ((SimBaseClass*)testFeature)->drawPointer->GetClass() == DrawableObject::Guys)
                                    {
                                        radius = 0.0F;
                                        //radius = radius*radius*radius*radius;
                                        canSee = 0.0F; //radius/range * tdisplayRange/groundMapRange;
                                    }
                                }
                            }
                        }
                        else
                        {
                            canSee = 0.0F;
                        }
                    }
                    else
                    {
                        //MI
                        if (g_bRealisticAvionics and g_bAGRadarFixes)
                        {
                            if (walker == &objectWalker)
                            {
                                // 2002-04-03 MN testFeature is a CAMPAIGN object now  We can't do SimBaseClass stuff here.
                                // Speed test however is valid, as it checks U_MOVING flag of unit
                                // As there are no campaign units that consist only of soldiers, no need to check for them here
                                if (testFeature->GetVt() > g_fGMTMinSpeed /*or
 ((SimBaseClass*)testFeature)->drawPointer and 
 ((SimBaseClass*)testFeature)->drawPointer->GetClass() == DrawableObject::Guys*/
                                   )
                                {
                                    radius = 0.0F;
                                    //radius = radius*radius*radius*radius;
                                    canSee = 0.0F;//radius/range * tdisplayRange/groundMapRange;
                                }
                                // 2002-04-03 MN a campaign unit only has two speed states - 0.0f and 40.0f for not moving/moving.
                                //else if(testFeature->GetVt() < -1.0F) //should never happen really.
                                else if ( not testFeature->GetVt())
                                {
                                    radius = DEFAULT_OBJECT_RADIUS;
                                    radius = radius * radius * radius * radius;
                                    canSee = radius / range * tdisplayRange / groundMapRange;
                                }
                            }
                            else
                            {
                                radius = DEFAULT_OBJECT_RADIUS;
                                radius = radius * radius * radius * radius;
                                canSee = radius / range * tdisplayRange / groundMapRange;
                            }
                        }
                        else
                        {
                            radius = DEFAULT_OBJECT_RADIUS;
                            radius = radius * radius * radius * radius;
                            canSee = radius / range * tdisplayRange / groundMapRange;
                        }
                    }
                }
                else
                {
                    canSee = 0.0F;
                }

                // Check LOS
                if (canSee > 0.8F)
                {
                    x = testFeature->XPos() - ownX;
                    y = testFeature->YPos() - ownY;

                    // Rotate for normalization
                    dx = trig.cos * x - trig.sin * y;
                    dy = trig.sin * x + trig.cos * y;

                    // Check Angle off nose
                    if (
                        (dy > 0.0F and dx > 0.5F * dy) or // Right side of nose
                        (dy < 0.0F and dx > 0.5F * -dy)   // Left side of nose
                    )
                    {
                        // Actual LOS
                        if ( not OTWDriver.CheckLOS(platform, testFeature))
                        {
                            canSee = 0.0F;  // LOS is blocked
                        }
                    }
                    else
                    {
                        canSee = 0.0F;   // Outside of cone
                    }
                }
            }
            else
            {
                canSee = 0.0F;
            }

            if (curNode)
            {
                switch (SimCompare(curNode->Object(), testFeature))
                {
                    case 0:
                        if (canSee > 0.8F)
                        {
                            //Update
                            lastList = curNode;
                            curNode = curNode->next;
                        }
                        else
                        {
                            // Object can't be seen, remove
                            if (curNode->prev)
                                curNode->prev->next = curNode->next;
                            else
                            {
                                GMFeatureListRoot = curNode->next;

                                if (GMFeatureListRoot)
                                    GMFeatureListRoot->prev = NULL;
                            }

                            if (curNode->next)
                                curNode->next->prev = curNode->prev;

                            tmpList = curNode;
                            curNode = curNode->next;
                            tmpList->Release();
                        }

                        //MI
                        if (g_bRealisticAvionics and g_bAGRadarFixes)
                        {
                            if (walker == &featureWalker)
                                testFeature = (SimBaseClass*)featureWalker.GetNext();
                            else
                                testFeature = (SimBaseClass*)objectWalker.GetNext();

                            if ( not testFeature and walker == &featureWalker)
                            {
                                testFeature = (SimBaseClass*)objectWalker.GetFirst();
                                walker = &objectWalker;
                            }
                        }
                        else
                            testFeature = (SimBaseClass*)featureWalker.GetNext();

                        break;

                    case 1: // testFeature > visObj -- Means the current allready deleted
                        if (curNode->prev)
                            curNode->prev->next = curNode->next;
                        else
                        {
                            GMFeatureListRoot = curNode->next;

                            if (GMFeatureListRoot)
                                GMFeatureListRoot->prev = NULL;
                        }

                        if (curNode->next)
                            curNode->next->prev = curNode->prev;

                        tmpList = curNode;
                        curNode = curNode->next;
                        tmpList->Release();
                        break;

                    case -1: // testFeature < visObj -- Means the current not added yet
                        bool filterthis = FALSE;

                        if (g_bRealisticAvionics and g_bAGRadarFixes)
                        {
                            if (walker == &objectWalker)
                            {
                                //bool here = true;
                                //float speed = testFeature->GetVt();
                                // 2002-04-03 MN removed IsBattalion check, added Drawable::Guys here and IsSim() check - don't do simbase stuff on campaign objects
                                if (testFeature->GetVt() > g_fGMTMinSpeed or /*testFeature->IsBattalion()*/
                                    testFeature->IsSim() and 
                                    ((SimBaseClass*)testFeature)->drawPointer and 
                                    ((SimBaseClass*)testFeature)->drawPointer->GetClass() == DrawableObject::Guys)
                                {
                                    filterthis = TRUE;
                                }
                            }
                        }

                        if (canSee > 1.0F and not filterthis)
                        {
                            tmpList = new GMList(testFeature);
                            tmpList->next = curNode;
                            tmpList->prev = lastList;

                            if (tmpList->next)
                                tmpList->next->prev = tmpList;

                            if (tmpList->prev)
                                tmpList->prev->next = tmpList;

                            if (curNode == GMFeatureListRoot)
                                GMFeatureListRoot = tmpList;

                            lastList = tmpList;
                        }

                        //MI
                        if (g_bRealisticAvionics and g_bAGRadarFixes)
                        {
                            if (walker == &featureWalker)
                                testFeature = (SimBaseClass*)featureWalker.GetNext();
                            else
                                testFeature = (SimBaseClass*)objectWalker.GetNext();

                            if ( not testFeature and walker == &featureWalker)
                            {
                                testFeature = (SimBaseClass*)objectWalker.GetFirst();
                                walker = &objectWalker;
                            }
                        }
                        else
                            testFeature = (SimBaseClass*)featureWalker.GetNext();

                        break;
                }
            } // curNode
            else
            {
                if (canSee > 1.0F)
                {
                    curNode = new GMList(testFeature);
                    curNode->prev = lastList;

                    if (lastList)
                        lastList->next = curNode;
                    else
                    {
                        GMFeatureListRoot = curNode;

                        if (GMFeatureListRoot)
                            GMFeatureListRoot->prev = NULL;
                    }

                    lastList = curNode;
                    curNode = NULL;
                }

                //MI
                if (g_bRealisticAvionics and g_bAGRadarFixes)
                {
                    if (walker == &featureWalker)
                        testFeature = (SimBaseClass*)featureWalker.GetNext();
                    else
                        testFeature = (SimBaseClass*)objectWalker.GetNext();

                    if ( not testFeature and walker == &featureWalker)
                    {
                        testFeature = (SimBaseClass*)objectWalker.GetFirst();
                        walker = &objectWalker;
                    }
                }
                else
                    testFeature = (SimBaseClass*)featureWalker.GetNext();
            }
        }


        //ENDING SECTION



        //TJL 11/25/03 This is the no stutter fix section.

        while (testFeature and g_bnoRadStutter)
        {

            if (isEmitting and not testFeature->IsAirplane())

            {
                range = (float)sqrt(
                            (testFeature->XPos() - ownX) * (testFeature->XPos() - ownX) +
                            (testFeature->YPos() - ownY) * (testFeature->YPos() - ownY) +
                            (testFeature->ZPos() - ownZ) * (testFeature->ZPos() - ownZ));

                if (range < radarHorizon)
                {
                    if (testFeature->IsSim())
                    {
                        // Check for visibility
                        //I-Hawk - added a check for GFX as chaff is now awake but has no GFX created
                        //so here it'll CTD if not checking GFX existence
                        if (((SimBaseClass*)testFeature)->IsAwake() and 
                            ((SimBaseClass*)testFeature)->drawPointer)
                        {
                            radius = ((SimBaseClass*)testFeature)->drawPointer->Radius();
                            radius = radius * radius * radius * radius;
                            canSee = radius / range * tdisplayRange / groundMapRange;
                            ((SimBaseClass*)testFeature)->drawPointer->GetPosition(&pos);
                            testFeature->SetPosition(pos.x, pos.y, pos.z);

                            if (g_bRealisticAvionics and g_bAGRadarFixes)
                            {
                                if (walker == &objectWalker)
                                {
                                    // 2002-04-03 MN removed IsBattalion check, added Drawable::Guys here
                                    if (testFeature->GetVt() > 1.0F or /*testFeature->IsBattalion()*/
                                        ((SimBaseClass*)testFeature)->drawPointer and 
                                        ((SimBaseClass*)testFeature)->drawPointer->GetClass() == DrawableObject::Guys)
                                    {
                                        radius = 0.0F;
                                        //radius = radius*radius*radius*radius;
                                        canSee = 0.0F; //radius/range * tdisplayRange/groundMapRange;
                                    }
                                }
                            }
                        }
                        else
                        {
                            canSee = 0.0F;
                        }
                    }
                    else
                    {
                        //MI
                        if (g_bRealisticAvionics and g_bAGRadarFixes)
                        {
                            if (walker == &objectWalker)
                            {
                                // 2002-04-03 MN testFeature is a CAMPAIGN object now  We can't do SimBaseClass stuff here.
                                // Speed test however is valid, as it checks U_MOVING flag of unit
                                // As there are no campaign units that consist only of soldiers, no need to check for them here
                                if (testFeature->GetVt() > g_fGMTMinSpeed /*or
   ((SimBaseClass*)testFeature)->drawPointer and 
   ((SimBaseClass*)testFeature)->drawPointer->GetClass() == DrawableObject::Guys*/
                                   )
                                {
                                    radius = 0.0F;
                                    //radius = radius*radius*radius*radius;
                                    canSee = 0.0F;//radius/range * tdisplayRange/groundMapRange;
                                }
                                // 2002-04-03 MN a campaign unit only has two speed states - 0.0f and 40.0f for not moving/moving.
                                //else if(testFeature->GetVt() < -1.0F) //should never happen really.
                                else if ( not testFeature->GetVt())
                                {
                                    radius = DEFAULT_OBJECT_RADIUS;
                                    radius = radius * radius * radius * radius;
                                    canSee = radius / range * tdisplayRange / groundMapRange;
                                }
                            }
                            else
                            {
                                radius = DEFAULT_OBJECT_RADIUS;
                                radius = radius * radius * radius * radius;
                                canSee = radius / range * tdisplayRange / groundMapRange;
                            }
                        }
                        else
                        {
                            radius = DEFAULT_OBJECT_RADIUS;
                            radius = radius * radius * radius * radius;
                            canSee = radius / range * tdisplayRange / groundMapRange;
                        }
                    }
                }
                else
                {
                    canSee = 0.0F;
                }

                // Check LOS
                if (canSee > 0.8F)
                {
                    x = testFeature->XPos() - ownX;
                    y = testFeature->YPos() - ownY;

                    // Rotate for normalization
                    dx = trig.cos * x - trig.sin * y;
                    dy = trig.sin * x + trig.cos * y;

                    // Check Angle off nose
                    if (
                        (dy > 0.0F and dx > 0.5F * dy) or // Right side of nose
                        (dy < 0.0F and dx > 0.5F * -dy)   // Left side of nose
                    )
                    {
                        // Actual LOS
                        if ( not OTWDriver.CheckLOS(platform, testFeature))
                        {
                            canSee = 0.0F;  // LOS is blocked
                        }
                    }
                    else
                    {
                        canSee = 0.0F;   // Outside of cone
                    }
                }
            }
            else
            {
                canSee = 0.0F;
            }

            if (curNode)
            {
                switch (SimCompare(curNode->Object(), testFeature))
                {
                    case 0:
                        if (canSee > 0.8F)
                        {
                            //Update
                            lastList = curNode;
                            curNode = curNode->next;
                        }
                        else
                        {
                            // Object can't be seen, remove
                            if (curNode->prev)
                                curNode->prev->next = curNode->next;
                            else
                            {
                                GMFeatureListRoot = curNode->next;

                                if (GMFeatureListRoot)
                                    GMFeatureListRoot->prev = NULL;
                            }

                            if (curNode->next)
                                curNode->next->prev = curNode->prev;

                            tmpList = curNode;
                            curNode = curNode->next;
                            tmpList->Release();
                        }

                        //MI
                        if (g_bRealisticAvionics and g_bAGRadarFixes)
                        {
                            if (walker == &featureWalker)
                                testFeature = (SimBaseClass*)featureWalker.GetNext();
                            else
                                testFeature = (SimBaseClass*)objectWalker.GetNext();

                            if ( not testFeature and walker == &featureWalker)
                            {
                                testFeature = (SimBaseClass*)objectWalker.GetFirst();
                                walker = &objectWalker;
                            }
                        }
                        else
                            testFeature = (SimBaseClass*)featureWalker.GetNext();

                        break;

                    case 1: // testFeature > visObj -- Means the current allready deleted
                        if (curNode->prev)
                            curNode->prev->next = curNode->next;
                        else
                        {
                            GMFeatureListRoot = curNode->next;

                            if (GMFeatureListRoot)
                                GMFeatureListRoot->prev = NULL;
                        }

                        if (curNode->next)
                            curNode->next->prev = curNode->prev;

                        tmpList = curNode;
                        curNode = curNode->next;
                        tmpList->Release();
                        break;

                    case -1: // testFeature < visObj -- Means the current not added yet
                        bool filterthis = FALSE;

                        if (g_bRealisticAvionics and g_bAGRadarFixes)
                        {
                            if (walker == &objectWalker)
                            {
                                //bool here = true;
                                //float speed = testFeature->GetVt();
                                // 2002-04-03 MN removed IsBattalion check, added Drawable::Guys here and IsSim() check - don't do simbase stuff on campaign objects
                                if (testFeature->GetVt() > g_fGMTMinSpeed or /*testFeature->IsBattalion()*/
                                    testFeature->IsSim() and 
                                    ((SimBaseClass*)testFeature)->drawPointer and 
                                    ((SimBaseClass*)testFeature)->drawPointer->GetClass() == DrawableObject::Guys)
                                {
                                    filterthis = TRUE;
                                }
                            }
                        }

                        if (canSee > 1.0F and not filterthis)
                        {
                            tmpList = new GMList(testFeature);
                            tmpList->next = curNode;
                            tmpList->prev = lastList;

                            if (tmpList->next)
                                tmpList->next->prev = tmpList;

                            if (tmpList->prev)
                                tmpList->prev->next = tmpList;

                            if (curNode == GMFeatureListRoot)
                                GMFeatureListRoot = tmpList;

                            lastList = tmpList;
                        }

                        //MI
                        if (g_bRealisticAvionics and g_bAGRadarFixes)
                        {
                            if (walker == &featureWalker)
                                testFeature = (SimBaseClass*)featureWalker.GetNext();
                            else
                                testFeature = (SimBaseClass*)objectWalker.GetNext();

                            if ( not testFeature and walker == &featureWalker)
                            {
                                testFeature = (SimBaseClass*)objectWalker.GetFirst();
                                walker = &objectWalker;
                            }
                        }
                        else
                            testFeature = (SimBaseClass*)featureWalker.GetNext();

                        break;
                }
            } // curNode
            else
            {
                if (canSee > 1.0F)
                {
                    curNode = new GMList(testFeature);
                    curNode->prev = lastList;

                    if (lastList)
                        lastList->next = curNode;
                    else
                    {
                        GMFeatureListRoot = curNode;

                        if (GMFeatureListRoot)
                            GMFeatureListRoot->prev = NULL;
                    }

                    lastList = curNode;
                    curNode = NULL;
                }

                //MI
                if (g_bRealisticAvionics and g_bAGRadarFixes)
                {
                    if (walker == &featureWalker)
                        testFeature = (SimBaseClass*)featureWalker.GetNext();
                    else
                        testFeature = (SimBaseClass*)objectWalker.GetNext();

                    if ( not testFeature and walker == &featureWalker)
                    {
                        testFeature = (SimBaseClass*)objectWalker.GetFirst();
                        walker = &objectWalker;
                    }
                }
                else
                    testFeature = (SimBaseClass*)featureWalker.GetNext();
            }
        }

        //TJL 11/25/03 No stutter fix section ends


        // Delete anthing after curNode
        tmpList = curNode;

        if (tmpList and tmpList->prev)
            tmpList->prev->next = NULL;

        if (tmpList == GMFeatureListRoot)
            GMFeatureListRoot = NULL;

        while (tmpList)
        {
            tmp2 = tmpList;
            tmpList = tmpList->next;
            tmp2->Release();
        }

#ifdef DEBUG
        // Verify order and singleness of list
        tmpList = GMFeatureListRoot;

        while (tmpList)
        {
            if (tmpList->next)
            {
                //MI changed to get movers on the list
                //F4Assert( SimCompare( tmpList->Object(), tmpList->next->Object() ) == 1 );
                F4Assert((SimCompare(tmpList->Object(), tmpList->next->Object()) == 1) or
                         (SimCompare(tmpList->Object(), tmpList->next->Object()) == -1));
            }

            tmpList = tmpList->next;
        }

#endif
    }

    // Now Do the movers
    curNode = GMMoverListRoot;
    lastList = NULL;
    testObject = (SimBaseClass*)objectWalker.GetFirst();

    if (testObject and curNode)
    {
        while (curNode and SimCompare(curNode->Object(), testObject) < 0)
        {
            tmpList = curNode;
            curNode = curNode->next;
            tmpList->Release();
        }
    }

    GMMoverListRoot = curNode;

    if (GMMoverListRoot)
        GMMoverListRoot->prev = NULL;

    while (testObject)
    {
        if (testObject->OnGround())
        {
            if (isEmitting)
            {
                // Check for visibility
                range = (float)sqrt(
                            (testObject->XPos() - ownX) * (testObject->XPos() - ownX) +
                            (testObject->YPos() - ownY) * (testObject->YPos() - ownY) +
                            (testObject->ZPos() - ownZ) * (testObject->ZPos() - ownZ));

                if (range < radarHorizon)
                {
                    //MI only show objects that are really moving
                    if (g_bRealisticAvionics and g_bAGRadarFixes)
                    {
                        bool FilterThis = FALSE;

                        if (testObject and testObject->IsSim() and testObject->drawPointer and 
                            testObject->drawPointer->GetClass() == DrawableObject::Guys)
                            FilterThis = TRUE;

                        if (testObject->IsSim() and not FilterThis and 
                            testObject->GetVt() > g_fGMTMinSpeed and 
                            testObject->GetVt() < g_fGMTMaxSpeed)
                        {
                            if (testObject->IsAwake())
                            {
                                radius = 2.0F * testObject->drawPointer->Radius();
                                /*  JB 010624 Why? Setting the position like this screws up multiplayer and entitys' movement
                                if (testObject->GetDomain() not_eq DOMAIN_SEA) // JB carrier (otherwise ships stop when you turn on your GM radar)
                                {
                                ((SimBaseClass*)testObject)->drawPointer->GetPosition(&pos);
                                testObject->SetPosition(pos.x, pos.y, pos.z);
                                }*/
                            }
                            else
                            {
                                radius = 0.0F;
                            }
                        }
                        // 2002-04-03 MN added check for moving campaign objects
                        else if (testObject->IsCampaign() and testObject->GetVt()) // campaign units only return 40 or 0 knots, depending on U_MOVING flag
                        {
                            radius = DEFAULT_OBJECT_RADIUS;
                        }
                        else
                        {
                            radius = 0.0F;
                        }
                    }
                    else
                    {
                        if (testObject->IsSim()) // NOTE this is for actually moving and testObject->GetVt() > 10.0F * KNOTS_TO_FTPSEC and 
                            //testObject->GetVt() < 100.0F * KNOTS_TO_FTPSEC)
                        {
                            if (testObject->IsAwake())
                            {
                                radius = 2.0F * testObject->drawPointer->Radius();
                                /*  JB 010624 Why? Setting the position like this screws up multiplayer and entitys' movement
                                if (testObject->GetDomain() not_eq DOMAIN_SEA) // JB carrier (otherwise ships stop when you turn on your GM radar)
                                {
                                ((SimBaseClass*)testObject)->drawPointer->GetPosition(&pos);
                                testObject->SetPosition(pos.x, pos.y, pos.z);
                                }*/
                            }
                            else
                            {
                                radius = 0.0F;
                            }
                        }
                        else
                        {
                            radius = DEFAULT_OBJECT_RADIUS;
                        }
                    }

                    radius = radius * radius * radius * radius * radius;
                    canSee = radius / range * tdisplayRange / groundMapRange;

                    // Check LOS
                    if (canSee > 0.8F)
                    {
                        x = testObject->XPos() - ownX;
                        y = testObject->YPos() - ownY;

                        // Rotate for normalization
                        dx = trig.cos * x - trig.sin * y;
                        dy = trig.sin * x + trig.cos * y;

                        // Check Angle off nose
                        if ((dy > 0.0F and dx > 0.5F * dy) or // Right side of nose
                            (dy < 0.0F and dx > 0.5F * -dy))   // Left side of nose
                        {
                            // Actual LOS
                            if (testObject->IsSim() and not OTWDriver.CheckLOS(platform, testObject))
                            {
                                canSee = 0.0F;  // LOS is blocked
                            }
                        }
                        else
                        {
                            canSee = 0.0F;   // Outside of cone
                        }
                    }
                }
                else
                {
                    canSee = 0.0F;  // Beyond radar horizon
                }
            }
            else
            {
                canSee = 0.0F;  // Our radar is off
            }

            if (curNode)
            {
                if (testObject == curNode->Object())
                {
                    if (canSee > 0.8F)
                    {
                        //Update
                        lastList = curNode;
                        curNode = curNode->next;
                    }
                    else
                    {
                        // Object can't be seen, remove
                        if (curNode->prev)
                            curNode->prev->next = curNode->next;
                        else
                        {
                            GMMoverListRoot = curNode->next;

                            if (GMMoverListRoot)
                                GMMoverListRoot->prev = NULL;
                        }

                        if (curNode->next)
                            curNode->next->prev = curNode->prev;

                        tmpList = curNode;
                        curNode = curNode->next;
                        tmpList->Release();
                    }

                    testObject = (SimBaseClass*)objectWalker.GetNext();
                }
                else
                {
                    switch (SimCompare(curNode->Object(), testObject))
                    {
                        case 0:
                        case 1: // testObject >= visObj -- Means the current allready deleted
                            if (curNode->prev)
                                curNode->prev->next = curNode->next;
                            else
                            {
                                GMMoverListRoot = curNode->next;

                                if (GMMoverListRoot) // Don't point the thing before the end of the list to nothing if there isn't anything after this list - RH
                                {
                                    GMMoverListRoot->prev = NULL;
                                }
                            }

                            if (curNode->next)
                                curNode->next->prev = curNode->prev;

                            tmpList = curNode;

                            curNode = curNode->next;
                            tmpList->Release();
                            break;

                        case -1: // testObject < visObj -- Means the current not added yet
                            if (canSee > 1.0F)
                            {
                                tmpList = new GMList(testObject);
                                tmpList->next = curNode;
                                tmpList->prev = lastList;

                                if (tmpList->next)
                                    tmpList->next->prev = tmpList;

                                if (tmpList->prev)
                                    tmpList->prev->next = tmpList;

                                if (curNode == GMMoverListRoot)
                                    GMMoverListRoot = tmpList;

                                lastList = tmpList;
                            }

                            testObject = (SimBaseClass*)objectWalker.GetNext();
                            break;
                    }
                } // inUse not_eq testObject
            } // inUse
            else
            {
                if (canSee > 1.0F)
                {
                    curNode = new GMList(testObject);
                    curNode->prev = lastList;

                    if (lastList)
                        lastList->next = curNode;
                    else
                    {
                        GMMoverListRoot = curNode;

                        if (GMMoverListRoot)
                            GMMoverListRoot->prev = NULL;
                    }

                    lastList = curNode;
                    curNode = NULL;
                }

                testObject = (SimBaseClass*)objectWalker.GetNext();
            }
        }
        else
        {
            testObject = (SimBaseClass*)objectWalker.GetNext();
        }
    }

    // Delete anthing after curNode
    tmpList = curNode;

    if (tmpList and tmpList->prev)
        tmpList->prev->next = NULL;

    if (tmpList == GMMoverListRoot)
        GMMoverListRoot = NULL;

    while (tmpList)
    {
        tmp2 = tmpList;
        tmpList = tmpList->next;
        tmp2->Release();
    }

#ifdef DEBUG
    // Verify order and singleness of list
    tmpList = GMMoverListRoot;

    while (tmpList)
    {
        if (tmpList->next)
        {
            F4Assert(SimCompare(tmpList->Object(), tmpList->next->Object()) == 1);
        }

        tmpList = tmpList->next;
    }

#endif

    if (IsSOI() and dropTrackCmd)
    {
        DropGMTrack();
    }

    //MI
    if (g_bRealisticAvionics and g_bAGRadarFixes)
    {
        if (lockedTarget and lockedTarget->BaseData())
        {
            if (mode == GMT)
            {
                if (lockedTarget->BaseData()->IsSim() and (lockedTarget->BaseData()->GetVt() < g_fGMTMinSpeed or
                        lockedTarget->BaseData()->GetVt() > g_fGMTMaxSpeed))
                    DropGMTrack();
                else if (lockedTarget->BaseData()->IsCampaign() and lockedTarget->BaseData()->GetVt() <= 0.0F)
                    DropGMTrack();
            }
            else if (mode == GM)
            {
                if (lockedTarget->BaseData()->IsSim() and lockedTarget->BaseData()->GetVt() > g_fGMTMinSpeed)
                    DropGMTrack();
                else if (lockedTarget->BaseData()->IsCampaign() and lockedTarget->BaseData()->GetVt() > 0.0F)
                    DropGMTrack();
            }
        }
    }

    //Build Track List
    if (lockedTarget)
    {
        // WARNING:  This might do ALOT more work than you want.  CalcRelGeom
        // will walk all children of lockedTarget (through the next pointer).
        // If you don't want this, set is next pointer to NULL before calling
        // and set it back upon return.
        CalcRelGeom(platform, lockedTarget, NULL, 1.0F / SimLibMajorFrameTime);
    }

    if (designateCmd)
    {
        if (mode == GM)
        {
            DoGMDesignate(GMFeatureListRoot);
        }
        else if (mode == GMT or mode == SEA)
        {
            DoGMDesignate(GMMoverListRoot);
        }
    }

    // MD -- 20040216: update the Pseudo waypoint after there was a slew operation
    if (IsSet(SP) and IsSet(SP_STAB))
    {
        //  only update if cursor is in the field of MFD view
        if ((F_ABS(cursorX) < 0.95F) and (F_ABS(cursorY) < 0.95F))
        {

            float x = 0.0F, y = 0.0F, z = 0.0F;

            if ( not GMSPPseudoWaypt)
            {
                GMSPPseudoWaypt = new WayPointClass();
            }

            // calculate x/y/z's
            GetGMCursorPosition(&x, &y);

            z = OTWDriver.GetGroundLevel(x, y);

            GMSPPseudoWaypt->SetLocation(x, y, z);
            GMSPPseudoWaypt->SetWPArrive(SimLibElapsedTime);

            SetGroundPoint(x, y, z);
            ToggleAGcursorZero();

            SetGMScan();
        }
    }

    // Update groundSpot position
    GetAGCenter(&x, &y);
    ((AircraftClass*)platform)->Sms->drawable->SetGroundSpotPos(x, y, 0.0F);
}



#endif
