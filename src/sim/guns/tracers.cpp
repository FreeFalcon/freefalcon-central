#include "stdhdr.h"
#include "otwdrive.h"
#include "guns.h"
#include "Graphics/Include/DrawTrcr.h"
#include "Graphics/Include/Draw2d.h"
#include "Graphics/Include/drawsgmt.h"
#include "fakerand.h"
#include "playerop.h"
#include "DrawParticleSys.h" // RV - I-Hawk - added to support RV new trails code

extern bool g_bUse_DX_Engine;

void GunClass::InitTracers()
{
    int i;
    float rgbScale;
    // Tpoint pos;

    if ( not (typeOfGun == GUN_TRACER or typeOfGun == GUN_TRACER_BALL))
        return;

    // Tracers
    tracers = new DrawableTracer*[numTracers];
    trailState = new int[numTracers];

    for (i = 0; i < numTracers; i++)
    {
        rgbScale = 1.0f - (float)((float)i / (float)numTracers);
        // Artscout - 2026: radius reverted to the original. The #31 widening (1.6+i*0.25) was 3-13x the original,
        // and since DetailLevel = GetDetailLevel()/radius, that pushed DetailLevel tiny for almost every tracer ->
        // they all took the thick 5-line "fan" draw path = angular "laser" look along the whole trajectory (on all
        // aircraft), only clearing once the detail/LOD state settled (e.g. after a radar lock changed the focus).
        // Visibility is carried by the additive ONE/ONE blend (also #31), not by a fat radius -> thin bright tracers.
        tracers[i] = new DrawableTracer(0.5f + (float)((float)i * 0.15f));

        if (i == 0)
            tracers[i]->SetAlpha(0.8f);
        else
            tracers[i]->SetAlpha(1.0f);

        // Artscout - 2026: warm AMBER tracer (DCS F-16 look) instead of the old yellow-white. The flying
        // trail and the muzzle flash (firstTracer below) now share one warm family so they read consistently;
        // the muzzle flash is just a touch redder/brighter. rgbScale (1 at the lead tracer -> 0 down the
        // stream) keeps the head warmest and the tail a deeper orange.
        tracers[i]->SetRGB(0.90f + rgbScale * 0.10f,   // R: ~0.90-1.00 (always warm)
                           0.40f + rgbScale * 0.25f,   // G: ~0.40-0.65 (amber)
                           0.0f + rgbScale * 0.05f);   // B: ~0 (no cold cast)

        // just test
        if (typeOfGun == GUN_TRACER_BALL)
            tracers[i]->SetType(TRACER_TYPE_BALL);

        trailState[i] = 0;
    }

    firstTracer =  new DrawableTracer*[numFirstTracers];

    for (i = 0; i < numFirstTracers; i++)
    {
        firstTracer[i] = new DrawableTracer(0.5f + (float)((float)i * 0.15f)); // Artscout - 2026: reverted #31 over-wide radius (see above)
        firstTracer[i]->SetAlpha(0.7f + (float)((float)i * 0.1f));
    }

    muzzleLoc = new Tpoint[ numFirstTracers ];
    muzzleEnd = new Tpoint[ numFirstTracers ];
    muzzleWidth = new float[ numFirstTracers ];
    muzzleAlpha = new float[ numFirstTracers ];
}

void GunClass::UpdateTracers(int firing)
{
    int i;
    Tpoint pos, end;
    GunTracerType *bulptr;

    // JB 010108 Update is being called without init
    if (tracers == NULL or trailState == NULL)
        return;

    // JB 010108

    // do the 1st muzzle tracers -- only if alpha blending is ON
    //   if ( PlayerOptions.AlphaOn() )
    //   {
    if (firing)
    {
        for (i = 0; i < numFirstTracers; i++)
        {
            if ( not firstTracer[i]->InDisplayList())
            {
                OTWDriver.InsertObject(firstTracer[i]);
                firstTracer[i]->SetAlpha(0.0f);
            }

            firstTracer[i]->SetAlpha(max(0.05f, muzzleAlpha[i] * 1.0f));

            // Artscout - 2026: muzzle flash kept in the same warm amber family as the flying trail (above),
            // just brighter/redder so it still reads as a flash rather than the old red(1,0.2,0)+yellow(1,1,0.2)
            // mismatch that looked disconnected from the normalized tracers.
            if (i bitand 1)
                firstTracer[i]->SetRGB(1.0f, 0.35f, 0.0f);   // hotter core of the flash (orange-red)
            else
                firstTracer[i]->SetRGB(1.0f, 0.55f, 0.05f);  // warm amber, matches the trail

            firstTracer[i]->SetWidth(max(0.10f, muzzleWidth[i] * 0.6f));
            firstTracer[i]->Update(&muzzleEnd[i], &muzzleLoc[i]);
        }

        /*    }
            else
            {
            if ( firstTracer[0]->InDisplayList() )
            {
         for ( i = 0; i < numFirstTracers; i++ )
         {
         OTWDriver.RemoveObject(firstTracer[i]);
         }
            }
            }*/
    }
    else
    {
        // Artscout - 2026: when the trigger is released the muzzle-flash tracers used to linger FROZEN in
        // world space until the 150 ms staleness cull -- as the jet flies forward the frozen flash slid back
        // and up over the canopy ("the burst source rises above the cockpit at the end of a burst"). Remove
        // them from the display list the instant firing stops -- the SAME proven path the flying tracers use
        // below (OTWDriver.RemoveObject(tracers[i])), so it is guaranteed gone regardless of the (additive)
        // blend state, not relying on alpha to hide it.
        for (i = 0; i < numFirstTracers; i++)
            if (firstTracer[i]->InDisplayList())
                OTWDriver.RemoveObject(firstTracer[i]);
    }

    for (i = 0; i < numTracers; i++)
    {
        bulptr = &bullet[i];

        if (bulptr->flying)
        {
            pos.x = bulptr->x;
            pos.y = bulptr->y;
            pos.z = bulptr->z;

            // float rtmp = max( 0.5f, PRANDFloatPos() ) * SimLibMajorFrameTime;
            // Artscout - 2026 (VR): the long #31 streak reads as a giant laser in VR's wide FOV + stereo depth -> shorter in headset.
            // Streak length = velocity * rtmp. The original used the LIVE SimLibMajorFrameTime, so the length tracked
            // frame rate: during load and master-mode (MRM<->DF) transitions the FPS drops, frameTime grows, and whole
            // bursts come out as giant lasers until the FPS settles. Decouple it -> a FIXED nominal frame time, so the
            // streak is a constant length regardless of FPS / mode switches (matches the old 60 FPS look on the flat path).
            extern bool g_bUseOpenXR; extern float g_fTracerStreak; extern float g_fVrTracerStreak;
            const float kNominalFrameTime = 0.0166f;   // ~60 FPS reference (FPS-independent streak length)
            float rtmp = kNominalFrameTime * (g_bUseOpenXR ? g_fVrTracerStreak : g_fTracerStreak);   // #31 long trail (flat); VR shortens
            end.x = bulptr->x - bulptr->xdot * rtmp;
            end.y = bulptr->y - bulptr->ydot * rtmp;
            end.z = bulptr->z - bulptr->zdot * rtmp;
            tracers[i]->Update(&pos, &end);

            // bullets[i]->SetPosition( &pos );

            if (trailState[i] == 0)
            {
                trailState[i] = 1;
                // OTWDriver.InsertObject(bullets[i]);
                OTWDriver.InsertObject(tracers[i]);
            }
        }
        else
        {
            if (trailState[i] not_eq 0)
            {
                trailState[i] = 0;
                OTWDriver.RemoveObject(tracers[i]);
                // OTWDriver.RemoveObject(bullets[i]);
            }
        }
    }
}

void GunClass::CleanupTracers()
{
    int i;

    if ( not (typeOfGun == GUN_TRACER or typeOfGun == GUN_TRACER_BALL))
    {
        FireShell(NULL);
        return;
    }

    if (tracers)
    {
        for (i = 0; i < numTracers; i++)
        {
            if (tracers[i])
            {
                OTWDriver.RemoveObject(tracers[i], TRUE);
            }

            // OTWDriver.RemoveObject(bullets[i], TRUE);
            tracers[i] = NULL;
            // bullets[i] = NULL;
        }
    }

    // delete [] bullets;
    if (tracers)
    {
        delete [] tracers;
    }

    if (trailState)
    {
        delete [] trailState;
    }

    tracers = NULL;
    // bullets = NULL;
    trailState = NULL;

    // KCK: This was in destructor - but must be cleaned up when graphics are still running
    // RV - I-Hawk - RV new trails call changes
    //if (smokeTrail)
    if (TrailIdNew)
    {
        //OTWDriver.RemoveObject(smokeTrail);
        DrawableParticleSys::PS_KillTrail(Trail);
        //smokeTrail = NULL
        TrailIdNew = Trail = NULL;
    }

    if (muzzleLoc)
    {
        delete [] muzzleLoc;
    }

    if (muzzleEnd)
    {
        delete [] muzzleEnd;
    }

    if (muzzleWidth)
    {
        delete [] muzzleWidth;
    }

    if (muzzleAlpha)
    {
        delete [] muzzleAlpha;
    }

    muzzleLoc = NULL;
    muzzleEnd = NULL;
    muzzleWidth = NULL;
    muzzleAlpha = NULL;

    if (firstTracer)
    {
        for (i = 0; i < numFirstTracers; i++)
        {
            if (firstTracer[i])
            {
                OTWDriver.RemoveObject(firstTracer[i], TRUE);
            }

            firstTracer[i] = NULL;
        }

        delete [] firstTracer;
        firstTracer = NULL;
    }
}
