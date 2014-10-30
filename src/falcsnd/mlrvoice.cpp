#include <windows.h>
#include <mmreg.h>
#include <process.h>
#include "PlayerOp.h"
#include "fsound.h"
#include "f4thread.h"
#include "falclib.h"
#include "dsound.h"
#include "psound.h"
#include "grTypes.h"
#include "matrix.h"
#include "SoundFX.h"
#include "sim/include/simdrive.h"
#include "sim/include/otwdrive.h"
#include "mlrVoice.h"

extern bool g_bUse3dSound;

mlrVoiceManager gVoiceManager;

extern bool g_bEnableDopplerSound, g_bSoundDistanceEffect;
extern float ExtAttenuation;
extern bool g_bSoundHearVMSExternal;
extern int g_nDynamicVoices;

//extern vector3 gVoiceManager.listenerPosition, CamVel;

mlrVoiceManager::mlrVoiceManager()
{
    mlrVoiceSection = F4CreateCriticalSection("mlrVoiceSection");
}

mlrVoiceManager::~mlrVoiceManager()
{
    F4DestroyCriticalSection(mlrVoiceSection);
}

void mlrVoiceManager::Lock()
{
    F4EnterCriticalSection(mlrVoiceSection);
}

void mlrVoiceManager::Unlock()
{
    F4LeaveCriticalSection(mlrVoiceSection);
}


void mlrVoiceManager::MovePlay2Hold()
{
    mlrVoice *n;

    while (n = (mlrVoice *)PlayList.RemHead())
    {
        HoldList.AddTail((ANode *)n);
    }
}

void mlrVoiceManager::StopAll()
{
    Lock();

    mlrVoice *n;
    AList temp;

    MovePlay2Hold();

    n = (mlrVoice *)HoldList.GetHead();

    while (n)
    {
        n->Stop();
        n->ReleaseBuffers();
        n = (mlrVoice *)n->GetSucc();
    }

    Unlock();
}

void mlrVoiceManager::Exec(Tpoint *campos, Trotation *camrot, Tpoint *camvel)
{
    static const Tpoint upv = { 0, 0, 1 };
    static const Tpoint fwd = { 1, 0, 0 };

    MatrixMult(camrot, &upv, &listenerUp);
    MatrixMult(camrot, &fwd, &listenerFront);
    listenerPosition = *campos;
    listenerVelocity = *camvel;

    Lock();

    mlrVoice *n;
    int channels;
    channels = g_nDynamicVoices;
    // sort the PlayList, store in temp
    AList temp;

    while (n = (mlrVoice *)PlayList.RemHead())
    {
        n->PreExec();
        temp.AddSorted((ANode *)n);
    }

    // filter out what will be played
    while (n = (mlrVoice *)temp.RemHead())
    {
        if ((n->status == mlrVoice::VSSTART or n->status == mlrVoice::VSPLAYING) and channels > 0)
        {
            PlayList.AddTail(n); // maintain sortedness
            channels--;
        }
        else
        {
            n->ReleaseBuffers();
            HoldList.AddTail(n);
        }
    }

    channels = g_nDynamicVoices;

    // make these allocate their sound buffers
    // then exec()
    if (SimDriver.MotionOn())
    {
        n = (mlrVoice *)PlayList.GetHead();

        while (n)
        {
            //COUNT_PROFILE("VOICEMAN_COUNT");
            n->AllocateBuffers();
            n->Exec();
            n = (mlrVoice *)n->GetSucc();
        }
    }
    else
    {
        n = (mlrVoice *)PlayList.GetHead();

        while (n)
        {
            n->Pause();
            n = (mlrVoice *)n->GetSucc();
        }

    }

    Unlock();
}

/************************************************/

mlrVoice::mlrVoice(mlrVoiceHandle *creator)
{
    owner = creator;
    sfx = &SFX_DEF[owner->sfxid];

    DSound3dBuffer = 0;
    DSoundBuffer   = 0;
    autodelete = 0;

    gVoiceManager.Lock();
    gVoiceManager.HoldList.AddTail(this);
    gVoiceManager.Unlock();
}

mlrVoice::~mlrVoice()
{
    gVoiceManager.Lock();
    ReleaseBuffers();
    ANode::Remove();
    gVoiceManager.Unlock();
}


void mlrVoice::Stop()
{
    if (DSoundBuffer)
    {
        status = VSSTOP;
        DSoundBuffer->Stop();
    }
}

void mlrVoice::Pause()
{
    if (DSoundBuffer)
    {
        status = VSPAUSED;
        DSoundBuffer->Stop();
    }
}


bool mlrVoice::IsPlaying()
{
    DWORD status;

    if (DSoundBuffer)
    {
        DSoundBuffer->GetStatus(&status);

        if (status bitand DSBSTATUS_PLAYING)
        {
            return true;
        }
    }

    return false;
}

void mlrVoice::Play(float PScale, float Vol, float X, float Y, float Z, float VX, float VY, float VZ)
{
    // Cobra - Treat thunder differently
    bool IsThunder = false;

    if ( not stricmp(sfx->fileName, "thunder.wav"))
    {
        IsThunder = true;
    }

    x = X;
    y = Y;
    z = Z;

    float dx = x - gVoiceManager.listenerPosition.x;
    float dy = y - gVoiceManager.listenerPosition.y;
    float dz = z - gVoiceManager.listenerPosition.z;

    distsq = dx * dx + dy * dy + dz * dz;

    // if(IsThunder)
    // if(distsq > 510000000000.f)
    // return;
    // else if(distsq > sfx->distSq)
    if ( not IsThunder and (distsq > sfx->distSq))
    {
        return;
    }

    startTime = vuxGameTime;

    pscale = PScale;
    initvol = Vol;

    vx = VX;
    vy = VY;
    vz = VZ;

    /*
    if( not (sfx->flags bitand SFX_POS_EXTERN))
     priority = 1;
    else
     priority = vol - (distsq / sfx->distSq) * 10000;
    */

    status = VSSTART;
    gVoiceManager.Lock();
    ANode::Remove();
    gVoiceManager.PlayList.AddHead(this);
    gVoiceManager.Unlock();
}

int mlrVoice::CompareWith(ANode *n)
{
    return(-((int)(priority - ((mlrVoice *)n)->priority)));
}

int gVoiceCount = 0;
int g3dVoiceCount = 0;

#if 0
bool mlrVoice::AllocateBuffers(void)
{
    SOUNDLIST *Sample;

    if (gVoiceCount > g_nDynamicVoices)
    {
        ReleaseBuffers();
        return(false);
    }

    // incase we already have what we need
    if ( not DSoundBuffer)
    {
        // going to get info from Sample already loaded.
        if (gSoundDriver)
        {
            Sample = gSoundDriver->FindSample(sfx->handle);

            if (Sample and Sample->Buf[0].DSoundBuffer)  // MLR 3/7/2004 - prevent CTDs
            {
                /////////////
                gSoundDriver->DSound->DuplicateSoundBuffer(Sample->Buf[0].DSoundBuffer, &DSoundBuffer);

                if (DSoundBuffer)
                {
                    gVoiceCount++;
                    freq = Sample->Frequency;
                }
            }
        }
    }

    if (DSoundBuffer)
    {
        if (is3d)
        {
            if ( not DSound3dBuffer)
            {
                if (/*sfx->flags bitand SFX_FLAGS_3D and */
                    g3dVoiceCount < 16
                )
                {
                    DSoundBuffer->QueryInterface(IID_IDirectSound3DBuffer,
                                                 (LPVOID *)&DSound3dBuffer);

                    if (DSound3dBuffer)  // and 
                        //  sfx and 
                        // (sfx->flags bitand SFX_POS_EXTERN) and 
                        // (sfx->flags bitand SFX_FLAGS_3D)) // only make external 3d sounds 3d
                    {
                        g3dVoiceCount++;
                        float maxdist = (float)  sqrt(sfx->maxDistSq);
                        float mindist = sfx->min3ddist;

                        if (mindist == 0)
                            mindist = maxdist / 20.0f;

                        DSound3dBuffer->SetMaxDistance(maxdist, DS3D_DEFERRED);
                        DSound3dBuffer->SetMinDistance(mindist, DS3D_DEFERRED);
                        DSound3dBuffer->SetMode(DS3DMODE_NORMAL, DS3D_DEFERRED);
                        return true;
                    }
                    else
                    {
                        ReleaseBuffers()
                        return false;
                    }
                }
            }
            else
            {
                if (DSound3dBuffer)
                {
                    g3dVoiceCount--;
                    DSound3dBuffer->Release();
                    DSound3dBuffer = 0;
                }
            }

            return true;
        }

        return false;
    }
}
#endif

#if 1
bool mlrVoice::AllocateBuffers(void)
{
    SOUNDLIST *Sample;

    // incase we already have what we need
    if (DSoundBuffer)
    {
        return(true);
    }

    // going to get info from Sample already loaded.
    if (gSoundDriver)
    {
        Sample = gSoundDriver->FindSample(sfx->handle);

        if (Sample and Sample->Buf[0].DSoundBuffer)
        {
            // MLR 3/7/2004 - prevent CTDs
            /////////////
            gSoundDriver->DSound->DuplicateSoundBuffer(Sample->Buf[0].DSoundBuffer, &DSoundBuffer);

            if (DSoundBuffer)
            {
                gVoiceCount++;
                freq = Sample->Frequency;

                if (sfx->flags bitand SFX_FLAGS_3D and g_bUse3dSound)
                {
                    DSoundBuffer->QueryInterface(IID_IDirectSound3DBuffer, (LPVOID *)&DSound3dBuffer);

                    if (DSound3dBuffer)  // and 
                        //  sfx and 
                        // (sfx->flags bitand SFX_POS_EXTERN) and 
                        // (sfx->flags bitand SFX_FLAGS_3D)) // only make external 3d sounds 3d
                    {
                        g3dVoiceCount++;
                        float maxdist = (float)  sqrt(sfx->maxDistSq);
                        float mindist = sfx->min3ddist;

                        if (mindist == 0)
                            mindist = maxdist / 20.0f;

                        DSound3dBuffer->SetMaxDistance(maxdist, DS3D_DEFERRED);
                        DSound3dBuffer->SetMinDistance(mindist, DS3D_DEFERRED);
                        DSound3dBuffer->SetMode(DS3DMODE_NORMAL, DS3D_DEFERRED);
                    }
                    else
                    {
                        // even if allocating a 3d voice fails, should we still play the sound???
                        ReleaseBuffers(); // MLR 5/15/2004 -
                        return false;
                    }
                }

                return(true);
            }
        }
    }

    return(false);
}
#endif

void mlrVoice::ReleaseBuffers(void)
{
    if (DSound3dBuffer)
    {
        g3dVoiceCount--;
        DSound3dBuffer->Release();
    }

    if (DSoundBuffer)
    {
        gVoiceCount--;
        DSoundBuffer->Stop();
        DSoundBuffer->Release();
    }

    DSoundBuffer = 0;
    DSound3dBuffer = 0;
}


// PreExec() - makes sure the sound needs to be played
// we shouldn't do anything in here that uses the camera/listner position
// because that position has not been updated since the last Exec()
void mlrVoice::PreExec()
{
    //MonoPrint("mlrVoice::PreExec() - sfxid:%3d uid:%7d dsb:%8x pscale:%2.4f vol:%f pri:%f",owner->sfxid,owner->userid,DSoundBuffer,pscale,vol,priority);

    // Cobra - Treat thunder differently
    bool IsThunder = false;

    if ( not stricmp(sfx->fileName, "thunder.wav"))
    {
        IsThunder = true;
    }

    int retval = 0;
    vol = initvol;

    if (sfx->flags bitand SFX_POS_EXTERN)
    {
        is3d = 1;
    }
    else
    {
        is3d = 0;
    }

    int inpit = OTWDriver.DisplayInCockpit() ;

    if (inpit)
    {
        // if this is an external only sound and we're in cockpit adjust
        bool isplayer = 0;

        if (owner->SPos->platform and 
            owner->SPos->platform == (SimBaseClass *)SimDriver.GetPlayerEntity())
            isplayer = 1; // the object that called this sound is the player


        if (sfx->flags bitand SFX_POS_EXTONLY)
            status = VSSTOP;

        if (sfx->flags bitand SFX_POS_EXTERN)
        {
            // don't cut the volume of sounds that are self originating from the player
            if ((sfx->flags bitand SFX_POS_SELF and isplayer))
            {
                is3d = 0;
            }
            else
            {
                if ( not IsThunder)
                    vol += ExtAttenuation;
            }
        }

        if (sfx->flags bitand (SFX_POS_INSIDE bitor SFX_FLAGS_VMS) and 
 not isplayer and 
            owner->SPos->platform)
        {
            // if we're inside the pit, but this "internal" sound is from another
            // object, get out.
            status = VSSTOP;
        }


    }
    else
    {
        // outside of pit
        if (sfx->flags bitand SFX_POS_EXTINT)
            status = VSSTOP;

        if (sfx->flags bitand SFX_POS_INSIDE)     // don't play internal sound
            status = VSSTOP;

        if (sfx->flags bitand SFX_FLAGS_VMS and not g_bSoundHearVMSExternal)
            status = VSSTOP;
    }

    vol += sfx->maxVol;

    // RV - Biker - Check if we are in array index limits
    if (sfx->soundGroup >= 0 and sfx->soundGroup < NUM_SOUND_GROUPS)
    {
        vol += PlayerOptions.GroupVol[ sfx->soundGroup ];
    }

    // if(vol>0) vol=0;

    if (vol < -10000)
    {
        vol = -10000;
        status = VSSTOP;
    }
    else
    {
        if (is3d and not IsThunder and owner->SPos->inMachShadow)
        {
            status = VSSTOP;
        }

        if (is3d and not IsThunder and (sfx->flags bitand SFX_FLAGS_CONE))
        {
            Tpoint delta = { x - gVoiceManager.listenerPosition.x,
                             y - gVoiceManager.listenerPosition.y,
                             z - gVoiceManager.listenerPosition.z
                           };

            // delta.Normalize();
            float l = sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);

            if (l > 0.0000001)
            {
                delta.x /= l;
                delta.y /= l;
                delta.z /= l;
            }

            float dot = delta.x * owner->SPos->orientation.M11 +
                        delta.y * owner->SPos->orientation.M21 +
                        delta.z * owner->SPos->orientation.M31;
            MonoPrint("dot %f  or(%f, %f, %f)  camvec(%f, %f, %f)", dot, owner->SPos->orientation.M11, owner->SPos->orientation.M21, owner->SPos->orientation.M31,
                      delta.x, delta.y, delta.z);

#define RESCALE(in,inmin,inmax,outmin,outmax) ( ((float)(in) - (inmin)) * ((outmax) - (outmin)) / ((inmax) - (inmin)) + (outmin))

            dot = RESCALE(dot, sfx->coneInsideAngle, sfx->coneOutsideAngle, 1.0f, 0.0f);

            if (dot < 0.0f) dot = 0.0f;

            if (dot > 1.0f) dot = 1.0f;

            vol += sfx->coneOutsideVol * dot;

            MonoPrint("  dot %f  vol %f", dot, vol);
        }
    }

    if (IsThunder or not is3d)
    {
        priority = 1;
    }
    else
    {
        priority = vol - (distsq / sfx->distSq) * 10000;

        if (owner->SPos->platform)
        {
            if (owner->SPos->platform == OTWDriver.GetTrackPlatform())
            {
                priority = .01f;
            }

            if (owner->SPos->platform == OTWDriver.GraphicsOwnship())
            {
                priority = 0.0f;
            }
        }

    }
}


// return 1 when the object has been executed, and can return to the HoldList
// like AssignSamples
void mlrVoice::Exec()
{
    float rx, ry, rz; // relative to the camera
    float dist, vel, Tdistsq = 0.0f, Tvol = 0.0f;

    if (DSoundBuffer == 0)
    {
        status = VSSTOP;
    }

    // Cobra - Treat thunder differently
    bool IsThunder = false;

    if ( not stricmp(sfx->fileName, "thunder.wav"))
    {
        float fsign = 1.0f;
        IsThunder = true;

        _copysign(fsign, (x - gVoiceManager.listenerPosition.x));
        x = gVoiceManager.listenerPosition.x + (float)(1100 * (rand() % 10));
        x *= fsign;
        fsign = 1.0f;
        _copysign(fsign, (y - gVoiceManager.listenerPosition.y));
        y = gVoiceManager.listenerPosition.y + (float)(1100 * (rand() % 10));
        y *= fsign;
        z = gVoiceManager.listenerPosition.z;

        rx = x - gVoiceManager.listenerPosition.x;
        ry = y - gVoiceManager.listenerPosition.y;
        rz = z - gVoiceManager.listenerPosition.z;
        dist = (float)sqrt(rx * rx + ry * ry + rz * rz);

        // Volume vs. dist attenuation
        float v = 1.0f - min(1.0f, dist / 15556.3f);

        if (dist < (15556.3f * 0.5f))
        {
            v = 1.0f;
        }

        v = 1.0f - log(v * 9.f + 1.f);
        Tvol = sfx->maxVol - (sfx->maxVol - sfx->minVol) * 0.5f * v;

        if (OTWDriver.DisplayInCockpit())
        {
            if ((Tvol - ExtAttenuation) < sfx->minVol)
            {
                Tvol = sfx->minVol * 0.8f;
            }
            else
            {
                Tvol += ExtAttenuation;
            }
        }

        dist *= 8.0f;
        Tdistsq = distsq = dist * dist;
        vol = Tvol;
    }
    else
    {
        rx = x - gVoiceManager.listenerPosition.x;
        ry = y - gVoiceManager.listenerPosition.y;
        rz = z - gVoiceManager.listenerPosition.z;
        // dist from camera
        dist = (float)sqrt(rx * rx + ry * ry + rz * rz);
        distsq = dist * dist; // distance from camera to sounds origin
    }

    /* -----------------------------*/
    // a lot of this can be optimized to use the owner->spos data
    if (is3d)
    {
        // object velocity (f/s)
        vel  = (float)sqrt(vx * vx + vy * vy + vz * vz);

        // velocity vector
        float vvx, vvy, vvz;

        if (vel)
        {
            vvx = vx / vel;
            vvy = vy / vel;
            vvz = vz / vel;
        }
        else
        {
            vvx = 1.0f;
            vvy = vvz = 0.0f;
        }

        /*
        dist = owner->SPos->distance;
        vel  = owner->SPos->velocity;
        vvx  = owner->SPos->vel.x;
        vvy  = owner->SPos->vel.y;
        vvz  = owner->SPos->vel.z;
        */

        if (g_bEnableDopplerSound and not IsThunder)
        {
            float d2, xx, yy, zz, m;

            // compute camera distance in 1 second (since velocity components are already in Feet/Sec)
            xx = (x + vx) - (gVoiceManager.listenerPosition.x + gVoiceManager.listenerVelocity.x);
            yy = (y + vy) - (gVoiceManager.listenerPosition.y + gVoiceManager.listenerVelocity.y);
            zz = (z + vz) - (gVoiceManager.listenerPosition.z + gVoiceManager.listenerVelocity.z);

            d2 = (float)sqrt(xx * xx + yy * yy + zz * zz);

            m = ((dist - d2) / (1100));  // * g_fSoundDopplerFactor;

            if (sfx->flags bitand SFX_FLAGS_REVDOP)
                m = -m;

            // constrain to +/- mach 1
            if (m < -1.f)
            {
                m = -1.f;
            }

            if (m > 1.f)
                m = 1.f;

            m += 1; // range is 0 to 2

            pscale *= m;

            // lower volume on sounds moving away
            if (m < 0)
                vol += m * 1000; // m is already negative

            // prevent the pscale from going below 0
            if (pscale < 0)
            {
                pscale = 0;
                vol = -10000; // MLR 12/3/2003 - Fixed, to -10000 not 0
            }
        } // done computing doppler effect


        // set position and/or attenuate volume
        if (DSound3dBuffer)
        {
            // MLR 2/23/2004 -
            // sound is 3d
            DSound3dBuffer->SetMode(DS3DMODE_NORMAL, DS3D_DEFERRED);
#define DISTEFF_THRESHOLD 100

            if (g_bSoundDistanceEffect and sfx->flags bitand SFX_POS_LOOPED)
            {
                // MLR 12/3/2003 - Only applied to looping sounds
                // sounds lag behind high speed objects
                // only applied to external sounds
                float s, v;

                // compute the seconds it takes for the sound to
                // get from the object, to the camera.
                s = (float)((dist/* - DISTEFF_THRESHOLD*/) / 1100);
                // this had to be fudged a little so that objects real close wouldn't have the effect applied.

                v = vel; // copy velocity for our own use.

                if (v > dist) // keeps sound on the correct side of the camera
                    v = dist;

                s *= v * .9f;
                x -= vvx * s;
                y -= vvy * s;
                z -= vvz * s;
            }

            DSound3dBuffer->SetPosition(x, y, z, DS3D_DEFERRED);
        }
        else
        {
            // non-external or non-3d sounds
            // no 3d buffer == no 3d sound
            // we have to do volume and panning by hand.

            if (DSound3dBuffer)
            {
                DSound3dBuffer->SetMode(DS3DMODE_DISABLE, DS3D_DEFERRED);
            }

            // MLR 12/4/2003 - We shouldn't be messing with internal sound volumes
            {
                // we're here because there's no 3d buffer allocated.
                // we need to compute the volume by hand to simulate
                // attenuation over distance.
                float v;

                if (IsThunder)
                {
                    vol = Tvol;
                }
                else
                {
                    // scale v from 0 to 1 between min bitand max dist
                    v = (distsq - sfx->min3ddist) / (sfx->maxDistSq - sfx->min3ddist);

                    // clamp result
                    if (v < 0) v = 0;
                    else if (v > 1) v = 1;

                    // this prolly ain't quite scientifically correct. :)
                    // log(1)  = 0
                    // log(10) = 1
                    v = (float)log(v * 9 + 1);
                    // map v to maxVol -> minVol
                    v = sfx->maxVol - (sfx->maxVol - sfx->minVol) * v;
                    vol += v;
                }
            }
        } // end position and/or attenuate volume
    }
    else
    {
        // not 3d sound
        // disable 3d buffer if we have one
        if (DSound3dBuffer)
        {
            DSound3dBuffer->SetMode(DS3DMODE_DISABLE, DS3D_DEFERRED);
        }
    }

    if (vol < -10000)
    {
        status = VSSTOP;
        //vol=-10000;
    }

    if (status == VSSTART)
    {
        // only concern ourselves with sounds that need to be started
        ////////////////////////////////////
        long Frequency = freq;

        if (sfx->flags bitand SFX_FLAGS_FREQ)
        {
            Frequency = (long)(Frequency * pscale);
            Frequency = min(Frequency, DSBFREQUENCY_MAX);
            Frequency = max(DSBFREQUENCY_MIN, Frequency);
        }

        ////////////////////////////////////

        // Play the sample
        if (sfx->flags bitand SFX_POS_LOOPED)
        {
            // loopy sounds
            static LARGE_INTEGER biggest = { 0 };
            LARGE_INTEGER freq, res;
            LARGE_INTEGER beg, end;
            QueryPerformanceCounter(&beg);
            DSoundBuffer->SetFrequency(Frequency); // MLR 12/7/2003 - The freq bitand vol code was moved here
            DSoundBuffer->SetVolume((long)vol);
            DSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);
            QueryPerformanceCounter(&end);
            QueryPerformanceFrequency(&freq);
            res.QuadPart = ((end.QuadPart - beg.QuadPart) * 1000000) / freq.QuadPart;

            if (res.QuadPart > biggest.QuadPart)
            {
                biggest.QuadPart = res.QuadPart;
            }

            //REPORT_VALUE("SOUND (us)", biggest.QuadPart);
            status = VSSTOP; // mark buffer as unused for the next go around.
        }
        else
        {
            // NON Looped sounds
            if (g_bSoundDistanceEffect and is3d)
            {
                //(sfx->flags bitand SFX_POS_EXTERN))
                // delay external sounds
                float time,     // Elapsed time since sound was created.
                      radiussq; // MLR 12/2/2003 - The radius from the sounds origin that the soundwave is currently at.

                // since sound radiates out in a sphere from the origin, we'll calculate the radius
                // that the sound has traveled, and see if the camera is inside the sphere.
                // if the camera is inside, the sound is played.
                // if not, the sound is delayed.
                // Speed of sound @60F ~= 1118 ft/sec

                // when time = 1 = 1sec
                time = (float)((vuxGameTime - startTime) / 1000.0 + 1.0);
                // MLR 12/2/2003 - The radius from the sounds origin that the soundwave is currently at.
                radiussq = (1100 * 1100 * time * time);

                // it would be more correct to only play the sound if the camera is very
                // close to the radius.
                if (IsThunder)
                    distsq = Tdistsq;

                if (distsq < radiussq)
                {
                    // note comparing squared values
                    DSoundBuffer->SetFrequency(Frequency);
                    DSoundBuffer->SetVolume((long)vol);
                    DSoundBuffer->SetCurrentPosition(0);
                    DSoundBuffer->Play(0, 0, 0);
                    status = VSPLAYING;
                }
                else
                {
                    if ((distsq > sfx->maxDistSq) and not IsThunder)
                    {
                        // MLR 12/2/2003 - Terminate sounds that can't be heard because they've traveled to far.
                        // the sound has outlasted it's lifespan and could not be heard anymore.
                        status = VSSTOP;
                    }
                    else
                    {
                        status = VSSTART; // Playing Delayed
                    }
                }
            }
            else
            {
                // Playing Once
                DSoundBuffer->SetFrequency(Frequency);
                DSoundBuffer->SetVolume((long)vol);
                DSoundBuffer->SetCurrentPosition(0);
                DSoundBuffer->Play(0, 0, 0);
                status = VSPLAYING;
            }
        }
    }
    else
    {
        if (sfx->flags bitand SFX_POS_LOOPED)
        {
            // don't stop non-looping sounds, let them finish on thier own.
            status = VSSTOP;
            DSoundBuffer->Stop();
        }
        else
        {
            // TODO: adjust the volume of these sounds while they are still playing is we're not using D3d
            if ( not IsPlaying())
            {
                // don't remove until the node has finished playing
                if (status == VSPAUSED)
                {
                    DSoundBuffer->Play(0, 0, 0);
                    status = VSPLAYING;
                }
                else
                {
                    status = VSSTOP;
                }
            }
            else
            {
                // set to VSPLAYING so we don't delete the buffers
                //DSoundBuffer->SetVolume((long)vol);
                status = VSPLAYING;
            }
        }
    }
}
