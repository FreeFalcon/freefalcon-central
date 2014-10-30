#include "Graphics/Include/drawsgmt.h"
#include "Graphics/Include/drawbsp.h"
#include "Graphics/Include/draw2d.h"
#include "stdhdr.h"
#include "otwdrive.h"
#include "initdata.h"
#include "object.h"
#include "simdrive.h"
#include "classtbl.h"
#include "entity.h"
#include "flare.h"
#include "terrtex.h"
#include "sfx.h" //RV - I-Hawk - added to support New PS flare effect
#include "Graphics/Include/drawparticlesys.h"


#ifdef USE_SH_POOLS
MEM_POOL FlareClass::pool;
#endif

extern bool g_bDisableMissleEngGlow; // MLR 2003-11-15

FlareClass::FlareClass(VU_BYTE** stream, long *rem) : BombClass(stream, rem)
{
    InitLocalData();
}

FlareClass::FlareClass(FILE* filePtr) : BombClass(filePtr)
{
    InitLocalData();
}

FlareClass::FlareClass(int type) : BombClass(type)
{
    InitLocalData();
}

FlareClass::~FlareClass()
{
    CleanupLocalData();
}

void FlareClass::CleanupLocalData()
{
}

void FlareClass::CleanupData()
{
    CleanupLocalData();
    BombClass::CleanupData();
}

void FlareClass::InitLocalData()
{
    bombType = Flare;
}

void FlareClass::InitData()
{
    BombClass::InitData();
    InitLocalData();
}

int FlareClass::SaveSize()
{
    return BombClass::SaveSize();
}

int FlareClass::Save(VU_BYTE **stream)
{
    int saveSize = BombClass::Save(stream);
    return saveSize;
}

int FlareClass::Save(FILE *file)
{
    int saveSize = SimWeaponClass::Save(file);
    return saveSize;
}

void FlareClass::Init(SimInitDataClass* initData)
{
    BombClass::Init(initData);
}

void FlareClass::Init(void)
{
    BombClass::Init();
}

int FlareClass::Wake(void)
{
    int retval = 0;

    if (IsAwake())
    {
        return retval;
    }

    BombClass::Wake();

    //OTWDriver.InsertObject(trail);

    if (parent)
    {
        x = parent->XPos();
        y = parent->YPos();
        z = parent->ZPos();
    }

    return retval;
}

int FlareClass::Sleep(void)
{
    // I-Hawk - Removed, flare's trail isn't managed here anymore but from PS
    //RemoveTrail();
    return BombClass::Sleep();
}

void FlareClass::Start(vector* pos, vector* rate, float cD)
{
    BombClass::Start(pos, rate, cD);
    dragCoeff = 0.4f;
}

void FlareClass::CreateGfx()
{
    // dont call base class since it create wrong gfx for some reason
    InitTrail();
    ExtraGraphics();
}

void FlareClass::DestroyGfx()
{
    BombClass::DestroyGfx();
}

void FlareClass::ExtraGraphics()
{
#if 0
    Falcon4EntityClassType* classPtr;

    OTWDriver.RemoveObject(drawPointer, TRUE);
    drawPointer = NULL;
    classPtr = &Falcon4ClassTable[displayIndex];
    OTWDriver.CreateVisualObject(this, classPtr->visType[0], OTWDriver.Scale());

    BombClass::ExtraGraphics();
    OTWDriver.RemoveObject(drawPointer, TRUE);
    drawPointer = NULL;
    classPtr = &Falcon4ClassTable[displayIndex];
    OTWDriver.CreateVisualObject(this, classPtr->visType[0], OTWDriver.Scale());
#else
    BombClass::ExtraGraphics();

    if (drawPointer)
    {
        OTWDriver.RemoveObject(drawPointer, TRUE);
        drawPointer = NULL;
    }

    Falcon4EntityClassType* classPtr = &Falcon4ClassTable[displayIndex];
    OTWDriver.CreateVisualObject(this, classPtr->visType[0], OTWDriver.Scale());
#endif

    Tpoint newPoint = { XPos(), YPos(), ZPos() };
    Tpoint vec = { XDelta(), YDelta(), ZDelta() }; //I-Hawk - Added vector to support new PS flare movement

    //RV - I-Hawk - PS flare effect call, to replace the old Flare effect
    /*

    OTWDriver.AddSfxRequest(new SfxClass( SFX_LIGHT_DEBRIS, // type
     SFX_MOVES,
     &newPoint, // world pos
     &vec,
     1.0f, // time to live
     1.0f) ); // scale
     */
    DrawableParticleSys::PS_AddParticleEx(
        (SFX_FLARE_GFX + 1), &newPoint, &vec
    );

    // I-Hawk - Removed, Not used anymore
    /*
    if( not g_bDisableMissleEngGlow) // MLR 2003-11-15 disable that shit-o star effect
    {
     OTWDriver.InsertObject(trailGlow);
     OTWDriver.InsertObject(trailSphere);
    }*/

    // I-Hawk - Removed, old flare graphics stuff

    //OTWDriver.RemoveObject(drawPointer, TRUE);
    //drawPointer = new Drawable2D( DRAW2D_FLARE, 2.0f, &newPoint );
    //OTWDriver.InsertObject(drawPointer);
    timeOfDeath = SimLibElapsedTime;
}

int FlareClass::Exec()
{
    if (trail)
    {
        // I-Hawk - Removed, flare's trail isn't managed here anymore but from PS
        //UpdateTrail();
        if (SimLibElapsedTime - timeOfDeath > 10 * SEC_TO_MSEC)
        {
            SetDead(TRUE);
        }
    }

    BombClass::Exec();

    if (IsExploding())
    {
        SetFlag(SHOW_EXPLOSION);
        SetDead(TRUE);
        return TRUE;
    }

    // MLR 2003-11-15 flares were going thru the ground
    {
        float z;

        z = OTWDriver.GetGroundLevel(XPos(), YPos());

        if (ZPos() > z)
        {
            int gtype = OTWDriver.GetGroundType(XPos(), YPos());

            switch (gtype)
            {
                case COVERAGE_WATER:
                case COVERAGE_RIVER:
                case COVERAGE_SWAMP:
                    // flare went into water.
                    SetDead(TRUE);
                    break;

                default:
                    // give em a slight bounce.
                    SetDelta((SM_SCALAR)(XDelta() * .25), (SM_SCALAR)(YDelta() * .25), (SM_SCALAR)(-ZDelta() * .25));
                    SetPosition(XPos(), YPos(), z);
                    break;
            }
        }
    }

    SetDelta(XDelta() * 0.99F, YDelta() * 0.99F, ZDelta() * 0.99F);
    return TRUE;
}

void FlareClass::DoExplosion(void)
{
    SetFlag(SHOW_EXPLOSION);
    SetDead(TRUE);
}

void FlareClass::InitTrail(void)
{
    //Falcon4EntityClassType* classPtr;

    //I-Hawk - Removed, old flare graphics stuff
    /*
     trailGlow = NULL;
     trailSphere = NULL;
    */

    flags or_eq IsFlare;
    displayIndex = GetClassID(DOMAIN_AIR, CLASS_SFX, TYPE_FLARE,
                              STYPE_FLARE1, SPTYPE_ANY, VU_ANY, VU_ANY, VU_ANY);

    if ( not drawPointer or drawPointer == (DrawableObject*)0xbaadf00d) // FRB
    {
        drawPointer = NULL;
    }

    if (drawPointer)
    {
        //OTWDriver.RemoveObject(drawPointer, TRUE); // FRB - causes CTD
        drawPointer = NULL;
    }

    // I-Hawk - Removed, old flare graphics stuff
    /*
     classPtr = &Falcon4ClassTable[displayIndex];

         trail = new DrawableTrail( TRAIL_FLARE );
         trail = (DrawableTrail*)DrawableParticleSys::PS_AddTrail( TRAIL_FLARE, XPos(), YPos(), ZPos());

     if (IsAwake())
     {
     OTWDriver.CreateVisualObject (this, classPtr->visType[0], OTWDriver.Scale());
     displayIndex = -1;
     }
     Tpoint newPoint = {0.0f, 0.0f, 0.0f};
     trailGlow = new Drawable2D( DRAW2D_MISSILE_GLOW, 6.0f, &newPoint );
     trailSphere = new Drawable2D( DRAW2D_EXPLCIRC_GLOW, 8.0f, &newPoint );
    */
}

void FlareClass::UpdateTrail(void)
{
    Tpoint newPoint;

    newPoint.x = XPos();
    newPoint.y = YPos();
    newPoint.z = ZPos();

    // I-Hawk - Removed, flare's trail isn't managed here anymore but from PS

    /* OTWDriver.AddTrailHead (trail, newPoint.x, newPoint.y, newPoint.z);
      // OTWDriver.TrimTrail(trail, 25); // MLR 12/14/2003 - removed
    // DrawableParticleSys::PS_UpdateTrail((PS_PTR)trail, XPos(), YPos(), ZPos());
     //trail = (DrawableTrail*)DrawableParticleSys::PS_EmitTrail((TRAIL_HANDLE)trail, TRAIL_FLARE, XPos(), YPos(), ZPos());

     trailGlow->SetPosition( &newPoint );
     trailSphere->SetPosition( &newPoint );
     ((Drawable2D *)drawPointer)->SetPosition( &newPoint );
    */
    if (SimLibElapsedTime - timeOfDeath > 10 * SEC_TO_MSEC)
    {
        SetDead(TRUE);
    }

    BombClass::UpdateTrail();

}

void FlareClass::RemoveTrail()
{
    if (trail)
    {
        //OTWDriver.RemoveObject(trail, TRUE);
        //DrawableParticleSys::PS_KillTrail((PS_PTR)trail);
        trail = NULL;
    }

    // I-Hawk - Removed, old flare graphics stuff

    /*
    if (trailGlow)
    {
     OTWDriver.RemoveObject(trailGlow, TRUE);
     trailGlow = NULL;
    }

    if (trailSphere)
    {
     OTWDriver.RemoveObject(trailSphere, TRUE);
     trailSphere = NULL;
    }
    */
}

