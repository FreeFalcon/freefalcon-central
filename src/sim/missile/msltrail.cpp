#include "stdhdr.h"
#include "f4error.h"
#include "f4vu.h"
#include "DrawParticleSys.h"
#include "missile.h"
#include "Graphics/Include/drawsgmt.h"
#include "Graphics/Include/drawbsp.h"
#include "Graphics/Include/draw2d.h"
#include "otwdrive.h"
#include "classtbl.h"
#include "sfx.h"
#include "Entity.h"
#include "weather.h"
#include "FakeRand.h"


extern int g_nmissiletrial;
extern bool g_bDisableMissleEngGlow; // MLR 2003-10-11
void MissileClass::InitTrail(void)
{
#ifndef MISSILE_TEST_PROG
    Tpoint newPoint;
    Tpoint origPoint;
    Tpoint delta;
    float distSq;
    Trotation rot;
    Falcon4EntityClassType *classPtr;
    WeaponClassDataType *wc;

    newPoint.x = XPos();
    newPoint.y = YPos();
    newPoint.z = ZPos();

    origPoint = newPoint;

    rot.M11 = dmx[0][0];
    rot.M21 = dmx[0][1];
    rot.M31 = dmx[0][2];
    rot.M12 = dmx[1][0];
    rot.M22 = dmx[1][1];
    rot.M32 = dmx[1][2];
    rot.M13 = dmx[2][0];
    rot.M23 = dmx[2][1];
    rot.M33 = dmx[2][2];

    // placement a bit behind the missile
    newPoint.x += dmx[0][0] * -7.0f;
    newPoint.y += dmx[0][1] * -7.0f;
    newPoint.z += dmx[0][2] * -7.0f;

    // edg: Something's going wrong with position of head of missile trail
    // which results in crash in viewpoint.  On the theory that the matrix
    // may be fucked up, let's do some checking, asserts and cover ups...
    delta.x = newPoint.x - origPoint.x;
    delta.y = newPoint.y - origPoint.y;
    delta.z = newPoint.z - origPoint.z;

    distSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
    // check distance sq.  Should be 7 * 7, but we'll allow more

    if (distSq > 100.0f)
    {
        // the notification
        ShiWarning("Something wrong with missile  Contact Engineer");

        // the bandaid
        newPoint = origPoint;
    }




    classPtr = (Falcon4EntityClassType*)EntityType();
    // 2002-03-28 MN if we need the engine to model "lift", don't display trails or engine glows...
    wc = (WeaponClassDataType*)classPtr->dataPtr;

    if (wc and (wc->Flags bitand WEAP_NO_TRAIL))
        return;

    // 2002-03-28 MN externalised missile trail types
    int mistrail = 0, misengGlow = 0, misengGlowBSP = 0, misgroundGlow = 0;

    if (auxData)
    {
        mistrail = auxData->mistrail;
        misengGlow = auxData->misengGlow;
        misengGlowBSP = auxData->misengGlowBSP;
        misgroundGlow = auxData->misgroundGlow;
    }


    if (mistrail == 0 and misengGlow == 0 and misengGlowBSP == 0 and misgroundGlow == 0)
    {
        // differentiate trails and missile types
        if (g_nmissiletrial)
        {
            //trail = new DrawableTrail( g_nmissiletrial );//me123
            TrailId = g_nmissiletrial;

            engGlow = new Drawable2D(DRAW2D_MISSILE_GLOW, 5.0, &newPoint);
            engGlowBSP1 = new DrawableBSP(MapVisId(VIS_MFLAME_L), &newPoint, &rot, 1.0f);
        }
        else if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_ROCKET)
        {
            //trail = new DrawableTrail( TRAIL_ROCKET );
            TrailId = TRAIL_ROCKET;

            engGlow = new Drawable2D(DRAW2D_MISSILE_GLOW, 2.0, &newPoint);
            engGlowBSP1 = new DrawableBSP(MapVisId(VIS_MFLAME_S), &newPoint, &rot, 1.0f);
        }
        else if (classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_AIM9M or
                 classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_AIM9P or
                 classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_AIM9R)
        {
            // smallish trail
            //trail = new DrawableTrail( TRAIL_IR_MISSILE );
            TrailId = TRAIL_IR_MISSILE;
            engGlow = new Drawable2D(DRAW2D_MISSILE_GLOW, 5.0, &newPoint);
            engGlowBSP1 = new DrawableBSP(MapVisId(VIS_MFLAME_S), &newPoint, &rot, 1.0f);
        }
        else if (classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_AIM120 or
                 classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_AA12 or
                 classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_AA11 or
                 classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_AA10C
                )
        {

            //trail = new DrawableTrail( TRAIL_IR_MISSILE ); // MLR 12/14/2003 -
            TrailId = TRAIL_IR_MISSILE;
            engGlow = new Drawable2D(DRAW2D_MISSILE_GLOW, 5.0, &newPoint);
            engGlowBSP1 = new DrawableBSP(MapVisId(VIS_MFLAME_L), &newPoint, &rot, 1.0f);
        }
        else
        {
            // RED -WAS ALERADY REMMED BEFORE ADDING NEW PS TRAIL - trail = new DrawableTrail( TRAIL_SAM );
            //trail = new DrawableTrail( TRAIL_IR_MISSILE ); // MLR 12/14/2003 -
            TrailId = TRAIL_IR_MISSILE;
            engGlow = new Drawable2D(DRAW2D_MISSILE_GLOW, 5.0, &newPoint);
            engGlowBSP1 = new DrawableBSP(MapVisId(VIS_MFLAME_L), &newPoint, &rot, 1.0f);
        }

        //RV - I-Hawk - disabling old ground glow stuff...
        //groundGlow = new Drawable2D( DRAW2D_MISSILE_GROUND_GLOW, 1.0, &newPoint );
    }
    else
    {
        //trail = new DrawableTrail( mistrail );
        TrailId = mistrail;
        engGlow = new Drawable2D(misengGlow, 5.0, &newPoint);
        engGlowBSP1 = new DrawableBSP(MapVisId(misengGlowBSP), &newPoint, &rot, 1.0f);
        //RV - I-Hawk - disabling old ground glow stuff...
        /*
        if (misgroundGlow not_eq -1)
         groundGlow = new Drawable2D( misgroundGlow, 1.0, &newPoint );
         */
    }

    // add 1st point
    //OTWDriver.AddTrailHead (trail, newPoint.x, newPoint.y, newPoint.z);
#endif
}

void MissileClass::UpdateTrail(void)
{
#ifndef MISSILE_TEST_PROG
    Tpoint newPoint;
    Trotation rot = IMatrix;
    float radius;
    float agl;
    Falcon4EntityClassType *classPtr;

    if (drawPointer)
        radius = drawPointer->Radius();
    else
        radius = 5.0f;

    classPtr = (Falcon4EntityClassType*)EntityType();
    ShiAssert(classPtr);

    //   if (trail)
    if (TrailId)
    {
        if (PowerOutput() > 0.25F or g_nmissiletrial)
        {
            newPoint.x = XPos();
            newPoint.y = YPos();
            newPoint.z = ZPos();


            // engGlow->Update( &newPoint, &((DrawableBSP*)drawPointer)->orientation  );
            rot.M11 = dmx[0][0];
            rot.M21 = dmx[0][1];
            rot.M31 = dmx[0][2];
            rot.M12 = dmx[1][0];
            rot.M22 = dmx[1][1];
            rot.M32 = dmx[1][2];
            rot.M13 = dmx[2][0];
            rot.M23 = dmx[2][1];
            rot.M33 = dmx[2][2];

            // MLR 2003-10-11 Fix for poorly placed engine glows
            if (auxData->misengLocation.x) // if .x=0, then just use the old method
            {
                // COBRA - RED - Add a little jittering on X Axis for Burner
                float RndOffset = PRANDFloatPos() * 0.3f;
                newPoint.x += dmx[0][0] * (auxData->misengLocation.x - RndOffset) + dmx[1][0] * auxData->misengLocation.y + dmx[2][0] * auxData->misengLocation.z;
                newPoint.y += dmx[0][1] * (auxData->misengLocation.x - RndOffset) + dmx[1][1] * auxData->misengLocation.y + dmx[2][1] * auxData->misengLocation.z;
                newPoint.z += dmx[0][2] * (auxData->misengLocation.x - RndOffset) + dmx[1][2] * auxData->misengLocation.y + dmx[2][2] * auxData->misengLocation.z;
            }
            else
            {
                // placement a bit behind the missile
                newPoint.x += dmx[0][0] * -radius;
                newPoint.y += dmx[0][1] * -radius;
                newPoint.z += dmx[0][2] * -radius;
            }

            // COBRA - RED - Add a little Random Scaling to flame
            engGlow->Update(&newPoint, &rot);
            engGlowBSP1->Update(&newPoint, &rot);
            engGlowBSP1->SetScale(1.0f + 0.1f * PRANDFloat());
            //RV I-Hawk - changed value from 30 to 20, trail was too far from missiles
            newPoint.x += dmx[0][0] * -20.0f;
            newPoint.y += dmx[0][1] * -20.0f;
            newPoint.z += dmx[0][2] * -20.0f;

            // JPO - add contrails if appropriate
            float objAlt = -ZPos() * 0.001F;
            bool contrail = false;

            //JAM 24Nov03
            if (objAlt > ((WeatherClass*)realWeather)->contrailLow and 
                objAlt < ((WeatherClass*)realWeather)->contrailHigh)
                contrail = true;

            //RV - I-Hawk - AIM-120 had no trail, it should have a weak trail
            /*
            if (classPtr->vuClassData.classInfo_[VU_SPTYPE] not_eq SPTYPE_AIM120 or
            contrail or
             g_nmissiletrial)
            */
            // add new head to trail
            //OTWDriver.AddTrailHead (trail, newPoint.x, newPoint.y, newPoint.z);
            //
            //RV - I-Hawk added check so missiles trail alpha/size change with altitude
            // but never goes too low...
            float MissileTrailAlpha;
            float MissileTrailSize;

            if (-ZPos() < 25000.0f)
            {
                MissileTrailAlpha = ((15000.0f - (-ZPos())) / 15000.0f) * 0.25f + 1;
                MissileTrailSize = ((15000.0f - (-ZPos())) / 15000.0f) * 0.25f + 1;
            }
            else
            {
                MissileTrailAlpha = ((15000.0f - (25000.0f)) / 15000.0f) * 0.4f + 1;
                MissileTrailSize = ((15000.0f - (25000.0f)) / 15000.0f) * 0.25f + 1;
            }

            Trail = DrawableParticleSys::PS_EmitTrail(Trail, TrailId, newPoint.x, newPoint.y, newPoint.z, MissileTrailAlpha, MissileTrailSize);

            // do the ground glow if near to ground
            agl = max(0.0f, groundZ - ZPos());
            //RV - I-Hawk - disabling old ground glow stuff...
            /*
            if ( agl < 1000.0f )
            {
              if ( not groundGlow->InDisplayList() )
              {
              OTWDriver.InsertObject(groundGlow );
              }
             // alpha fades with height, radius increases with height
             groundGlow->SetAlpha( 0.3f * ( 1000.0f - agl ) / 1000.0f );
             groundGlow->SetRadius( 20.0f + 350.0f * ( agl ) / 1000.0f );
             newPoint.x = XPos();
             newPoint.y = YPos();
             newPoint.z = groundZ;
             groundGlow->SetPosition( &newPoint );
            }
            else if ( groundGlow->InDisplayList() )
            {
            OTWDriver.RemoveObject(groundGlow, FALSE );
            }
            */
        }
        else// if (trail->GetHead()) // MLR 12/11/2003 - commented out for new smoke trails
        {

            if (engGlow and engGlow->InDisplayList())
            {
                OTWDriver.RemoveObject(engGlow, FALSE);
            }

            if (engGlowBSP1 and engGlowBSP1->InDisplayList())
            {
                OTWDriver.RemoveObject(engGlowBSP1, FALSE);
            }

            if (groundGlow and groundGlow->InDisplayList())
            {
                OTWDriver.RemoveObject(groundGlow, FALSE);
            }
        }
    }
    else if ( not IsExploding())
    {
        InitTrail();

        //if (trail)
        //OTWDriver.InsertObject(trail);
        if (engGlow and not g_bDisableMissleEngGlow) // MLR 2003-10-11 Disble the star
            OTWDriver.InsertObject(engGlow);

        if (engGlowBSP1)
            OTWDriver.InsertObject(engGlowBSP1);
    }

#endif
}

void MissileClass::RemoveTrail(void)
{
#ifndef MISSILE_TEST_PROG

    //   if (trail)
    if (TrailId)
    {
        if (OTWDriver.IsActive())
        {
            // hand trail destruction over to sfx driver after a
            // time to live period
            //OTWDriver.AddSfxRequest( new SfxClass(20.0f, trail) );
            DrawableParticleSys::PS_KillTrail(Trail);
        }
        else
        {
            //OTWDriver.RemoveObject(trail, TRUE);
            DrawableParticleSys::PS_KillTrail(Trail);
        }

        //trail = NULL;
        TrailId = Trail = 0;
    }


    if (engGlow)
    {
        // MLR 2003-10-11 Note: It IS safe to call RemoveObject if the object is not it the list.
        OTWDriver.RemoveObject(engGlow, TRUE);
        engGlow = NULL;
    }

    if (groundGlow)
    {
        OTWDriver.RemoveObject(groundGlow, TRUE);
        groundGlow = NULL;
    }

    if (engGlowBSP1)
    {
        OTWDriver.RemoveObject(engGlowBSP1, TRUE);
        engGlowBSP1 = NULL;
    }

#endif
}
