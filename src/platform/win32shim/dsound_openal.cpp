// Artscout - 2026 (#104, Linux port, Phase 1 audio): OpenAL implementation of the DirectSound
// interfaces declared in dsound.h. One OpenAL source+buffer backs each DirectSound buffer.
//
// Mapping summary (DirectSound -> OpenAL):
//   CreateSoundBuffer          -> alGenSources + alGenBuffers, CPU staging buffer of dwBufferBytes
//   Lock / Unlock              -> hand out the staging buffer; on Unlock upload it via alBufferData
//   Play / Stop                -> alSourcePlay / alSourceStop (+ AL_LOOPING)
//   SetVolume (millibels)      -> alSourcef AL_GAIN, gain = 10^(mB/2000)
//   SetFrequency (Hz)          -> alSourcef AL_PITCH, pitch = Hz / originalHz
//   SetPan (-1e4..1e4)         -> head-relative source X offset (OpenAL has no direct pan)
//   3D SetPosition/SetVelocity -> alSource3f AL_POSITION / AL_VELOCITY
//   3D listener                -> alListener3f / alListenerf
//   SetNotificationPositions   -> stored; streaming refill is approximate (see TODO)
//
// Artscout - 2026: BOTH platforms now. Windows moved off real DirectSound onto this same OpenAL
// implementation (one audio path to maintain, and OpenAL Soft's mixer/resampler beats the Windows
// DirectSound emulation). The engine's sound TUs include the shim dsound.h explicitly, so the
// interfaces here are the ones they call; dsound.lib is no longer linked.
#include <ciso646> // not/and/or tokens under MSVC
#include <windows.h>
#include <mmsystem.h>
#include "platform/win32shim/dsound.h"
#ifdef _WIN32
#include "extlibs/openal/include/AL/al.h" // OpenAL Soft (extlibs; runtime = OpenAL32.dll aka soft_oal)
#include "extlibs/openal/include/AL/alc.h"
#else
#include <AL/al.h> // system OpenAL Soft
#include <AL/alc.h>
#endif
#ifndef ALC_ALL_DEVICES_SPECIFIER // some headers keep it in alext.h
#define ALC_ALL_DEVICES_SPECIFIER 0x1013
#endif

// f4config: substring of the audio output device to open ("" = system default). Declared OUTSIDE the
// anonymous namespace below, or the extern picks up internal linkage and never finds the definition.
extern char g_strSoundDevice[];
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cctype>
#include <cstring>
#include <vector>

// ---- process-wide OpenAL device/context (opened by DirectSoundCreate) ----
namespace
{
// TEMP sound-bringup diagnostics (Windows switch to OpenAL): visible in the VS Output window /
// DebugView AND on stderr. Remove once audio is confirmed on both platforms.
void DsLog(const char* fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    OutputDebugStringA(buf);
    fprintf(stderr, "%s", buf);
}

ALCdevice* g_alDevice = nullptr;
ALCcontext* g_alContext = nullptr;
int g_dsRefs = 0;

bool EnsureOpenAL()
{
    if (g_alContext)
        return true;
    // Device pick: the SoundDevice config knob holds a case-insensitive substring of the wanted
    // output (e.g. "Pimax" to pin the headset). Empty -> system default; OpenAL Soft's WASAPI
    // backend follows default-device changes, so the headset audio (which wakes up after launch)
    // is picked up automatically in that mode.
    const char* want = nullptr;
    char picked[256] = {0};
    if (g_strSoundDevice[0] and
        alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT"))
    {
        const char* all = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
        DsLog("[dsound/OpenAL] want device substring '%s'; available:\n",
              g_strSoundDevice);
        for (const char* d = all; d and *d; d += strlen(d) + 1)
        {
            DsLog("[dsound/OpenAL]   '%s'\n", d);
            if (!picked[0])
            {
                // case-insensitive substring match
                const char* h = d;
                for (; *h; ++h)
                {
                    const char* a = h;
                    const char* b = g_strSoundDevice;
                    while (*a and *b and
                           tolower((unsigned char)*a) ==
                               tolower((unsigned char)*b))
                    {
                        ++a;
                        ++b;
                    }
                    if (!*b)
                    {
                        strncpy(picked, d, sizeof(picked) - 1);
                        break;
                    }
                }
            }
        }
        if (picked[0])
            want = picked;
        else
            DsLog("[dsound/OpenAL] no device matches '%s' -> default\n",
                  g_strSoundDevice);
    }
    g_alDevice = alcOpenDevice(want); // nullptr = default output device
    if (!g_alDevice and want)
    {
        DsLog("[dsound/OpenAL] open '%s' failed -> default\n", want);
        g_alDevice = alcOpenDevice(nullptr);
    }
    if (!g_alDevice)
    {
        OutputDebugStringA("[dsound/OpenAL] alcOpenDevice failed\n");
        return false;
    }
    // #104: the engine pre-creates a DirectSound buffer (=> one OpenAL SOURCE) per sound effect -- hundreds of them.
    // OpenAL Soft's default is only ~256 mono / ~256 stereo sources, so alGenSources started returning 0 partway
    // through (buffer generated, source NOT) -> those buffers (incl. the streamed menu music) had source=0 and
    // Play() bailed -> silence. Ask the context for a large source pool up front.
    const ALCint attrs[] = {ALC_MONO_SOURCES, 1024, ALC_STEREO_SOURCES, 256, 0};
    g_alContext = alcCreateContext(g_alDevice, attrs);
    if (!g_alContext)
    {
        alcCloseDevice(g_alDevice);
        g_alDevice = nullptr;
        return false;
    }
    alcMakeContextCurrent(g_alContext);
    DsLog("[dsound/OpenAL] context up, device='%s'\n",
          alcGetString(g_alDevice, ALC_DEVICE_SPECIFIER));
#ifdef _WIN32
    // Which OpenAL32.dll actually loaded? 'Generic Software' means the legacy Creative router from
    // System32 grabbed the process (max 256 sources, no HMD endpoint) -- the exe needs OUR soft_oal
    // copy named OpenAL32.dll SITTING NEXT TO IT (the post-build step deploys it).
    {
        HMODULE hm = GetModuleHandleA("OpenAL32.dll");
        char dllPath[MAX_PATH] = {0};
        if (hm)
            GetModuleFileNameA(hm, dllPath, sizeof(dllPath));
        DsLog("[dsound/OpenAL] OpenAL32.dll = %s\n",
              dllPath[0] ? dllPath : "(not found?)");
    }
#endif
    return true;
}

ALenum AlFormat(int channels, int bits)
{
    if (channels >= 2)
        return bits == 8 ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
    return bits == 8 ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
}
float MilliBelToGain(long mb)
{
    if (mb <= DSBVOLUME_MIN)
        return 0.0f;
    if (mb >= 0)
        return 1.0f;
    return std::pow(10.0f, (float)mb / 2000.0f);
}
} // namespace

// ---- per-buffer backend state ----
struct IDirectSoundBuffer::Impl
{
    ALuint source = 0;
    ALuint buffer =
        0; // static-path buffer: one-shot SFX and looping-static (engine) sounds
    WAVEFORMATEX fmt{};
    std::vector<unsigned char> staging; // CPU-side bytes (Lock/Unlock target)
    DWORD origFreq = 22050;
    long volMb = 0;
    long panMb = 0;
    bool looping = false;
    bool primary = false;
    IDirectSound3DBuffer* facet3d = nullptr;
    IDirectSoundNotify* facetNotify = nullptr;
    ULONG refs = 1;

    // ---- streaming (music/voice file) path ----
    // A DirectSound stream is a LOOPING circular buffer that the engine refills half-at-a-time WHILE it
    // plays (ProcessStream Lock/Unlock's one half as the play cursor passes it). OpenAL cannot mutate a
    // buffer attached to a playing source, so the old static path stopped the source, re-uploaded the whole
    // buffer with alBufferData and restarted it on EVERY refill -- that stop/restart was the audible jerk.
    // Instead, the first write that arrives while the source is PLAYING switches this buffer to an OpenAL
    // queued-buffer chain: each refilled half is queued as its own buffer and processed buffers are recycled,
    // so playback is gapless. Static one-shot / looping-static SFX only ever write BEFORE Play, so they never
    // enter this path and keep the exact static behaviour (no regression to 3D sound).
    bool streaming = false;
    DWORD lockOffset =
        0; // region handed out by the last Lock (consumed by Unlock)
    DWORD lockBytes = 0;
    unsigned long long consumedBytes =
        0; // total bytes fully played -- drives the synthetic play cursor
    DWORD ringBytes =
        0; // circular buffer size (== staging.size()) captured at the switch
    DWORD halfBytes =
        0; // refill granularity (== Stream->HalfSize), inferred from lockBytes
    std::vector<ALuint>
        freeBufs; // unqueued AL buffers, ready to be refilled and requeued

    ALsizei Rate() const
    {
        return (ALsizei)(fmt.nSamplesPerSec ? fmt.nSamplesPerSec : origFreq);
    }
    // Pull every finished buffer off the queue, tally its bytes as played, and recycle it.
    void DrainProcessed()
    {
        if (!source)
            return;
        ALint proc = 0;
        alGetSourcei(source, AL_BUFFERS_PROCESSED, &proc);
        while (proc-- > 0)
        {
            ALuint b = 0;
            alSourceUnqueueBuffers(source, 1, &b);
            if (!b)
                break;
            ALint sz = 0;
            alGetBufferi(b, AL_SIZE, &sz);
            consumedBytes += (unsigned)sz;
            freeBufs.push_back(b);
        }
    }
    ALuint TakeBuf()
    {
        if (!freeBufs.empty())
        {
            ALuint b = freeBufs.back();
            freeBufs.pop_back();
            return b;
        }
        ALuint b = 0;
        alGenBuffers(1, &b);
        return b;
    }
    // Tear the queue down (stop / re-start of a stream): unqueue+delete everything, back to static state.
    void ResetStream()
    {
        if (source)
        {
            alSourceStop(source);
            ALint q = 0;
            alGetSourcei(source, AL_BUFFERS_QUEUED, &q);
            while (q-- > 0)
            {
                ALuint b = 0;
                alSourceUnqueueBuffers(source, 1, &b);
                if (b)
                    freeBufs.push_back(b);
            }
            alSourcei(source, AL_BUFFER, 0);
        }
        for (ALuint b : freeBufs)
            if (b)
                alDeleteBuffers(1, &b);
        freeBufs.clear();
        streaming = false;
        consumedBytes = 0;
        ringBytes = 0;
        halfBytes = 0;
    }

    void ApplyPan()
    {
        // OpenAL has no pan; place a head-relative mono source left/right of the listener.
        if (!source)
            return;
        alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
        float x = (float)panMb / (float)DSBPAN_RIGHT; // -1 .. +1
        float fwd2 = 1.0f - x * x;
        if (fwd2 < 0.0f)
            fwd2 = 0.0f;
        alSource3f(source, AL_POSITION, x, 0.0f, -std::sqrt(fwd2));
    }
};

// ============================ IDirectSoundBuffer ============================
HRESULT IDirectSoundBuffer::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
        return DSERR_INVALIDPARAM;
    if (std::memcmp(&riid, &IID_IDirectSound3DBuffer, sizeof(IID)) == 0)
    {
        if (!d->facet3d)
        {
            d->facet3d = new IDirectSound3DBuffer();
            d->facet3d->owner = this;
        }
        *ppv = d->facet3d;
        return DS_OK;
    }
    if (std::memcmp(&riid, &IID_IDirectSoundNotify, sizeof(IID)) == 0)
    {
        if (!d->facetNotify)
        {
            d->facetNotify = new IDirectSoundNotify();
            d->facetNotify->owner = this;
        }
        *ppv = d->facetNotify;
        return DS_OK;
    }
    // The primary buffer answers the 3D listener query with a listener facet.
    if (std::memcmp(&riid, &IID_IDirectSound3DListener, sizeof(IID)) == 0)
    {
        *ppv = new IDirectSound3DListener();
        return DS_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}
ULONG IDirectSoundBuffer::AddRef(void)
{
    return ++d->refs;
}
ULONG IDirectSoundBuffer::Release(void)
{
    if (!d)
    {
        delete this;
        return 0;
    }
    if (--d->refs == 0)
    {
        if (d->source)
        {
            alSourceStop(d->source);
            alSourcei(d->source, AL_BUFFER, 0);
            alDeleteSources(1, &d->source);
        }
        for (ALuint b : d->freeBufs)
            if (b)
                alDeleteBuffers(1, &b); // streaming: recycled queue buffers
        if (d->buffer)
            alDeleteBuffers(1, &d->buffer);
        delete d->facet3d;
        delete d->facetNotify;
        delete d;
        d = nullptr;
        delete this;
        return 0;
    }
    return d->refs;
}
HRESULT IDirectSoundBuffer::Play(DWORD, DWORD, DWORD flags)
{
    if (!d || !d->source)
        return DSERR_GENERIC;
    d->looping = (flags & DSBPLAY_LOOPING) != 0;
    ALint st = AL_INITIAL;
    alGetSourcei(d->source, AL_SOURCE_STATE, &st);
    // DirectSound Play() on an ALREADY-PLAYING buffer is a no-op (it keeps playing); OpenAL alSourcePlay RESTARTS
    // from the start. The engine re-issues Play() every frame to keep continuous/looping sounds (the 3D engine hum,
    // ambience) alive -- restarting each frame made them "reset constantly". Only (re)start if not already playing;
    // a genuine replay of a one-shot goes through Stop()/SetCurrentPosition(0) first, which leaves it not-playing.
    if (st == AL_PLAYING)
        return DS_OK; // already sounding -> keep going, don't reset
    if (!d->streaming) // streams own AL_LOOPING=false via the queue
        alSourcei(d->source, AL_LOOPING, d->looping ? AL_TRUE : AL_FALSE);
    alSourcePlay(d->source);
    static int s_plays = 0;
    if (++s_plays <= 24)
    {
        ALint st2 = 0;
        ALfloat gain = -1.0f;
        alGetSourcei(d->source, AL_SOURCE_STATE, &st2);
        alGetSourcef(d->source, AL_GAIN, &gain);
        DsLog("[dsound/OpenAL] Play #%d src=%u state=%d gain=%.3f loop=%d "
              "alErr=%d\n",
              s_plays, (unsigned)d->source, (int)st2, (double)gain,
              (int)d->looping, (int)alGetError());
    }
    return DS_OK;
}
HRESULT IDirectSoundBuffer::Stop(void)
{
    if (d && d->source)
    {
        if (d->streaming)
            d->ResetStream(); // stream stopped -> drop the queue, back to static state
        else
            alSourceStop(d->source);
    }
    return DS_OK;
}
HRESULT IDirectSoundBuffer::Lock(DWORD offset, DWORD bytes, void** pp1,
                                 DWORD* pb1, void** pp2, DWORD* pb2,
                                 DWORD flags)
{
    if (!d)
        return DSERR_GENERIC;
    if (flags & DSBLOCK_ENTIREBUFFER)
    {
        offset = 0;
        bytes = (DWORD)d->staging.size();
    }
    if (offset > d->staging.size())
        return DSERR_INVALIDPARAM;
    if (bytes > d->staging.size() - offset)
        bytes = (DWORD)d->staging.size() - offset;
    if (pp1)
        *pp1 = d->staging.data() + offset;
    if (pb1)
        *pb1 = bytes;
    if (pp2)
        *pp2 = nullptr; // no wrap-around; single contiguous region
    if (pb2)
        *pb2 = 0;
    d->lockOffset =
        offset; // remembered for Unlock (streaming: the region to queue)
    d->lockBytes = bytes;
    return DS_OK;
}
HRESULT IDirectSoundBuffer::Unlock(void*, DWORD, void*, DWORD)
{
    if (!d || d->staging.empty())
        return DS_OK;

    ALint st = AL_STOPPED;
    if (d->source)
        alGetSourcei(d->source, AL_SOURCE_STATE, &st);
    const bool playing = (st == AL_PLAYING);

    // Static fill-before-play: one-shot SFX, looping-static engine sounds, AND a stream's initial fill
    // (SilenceStream + first ReadStream both run before Play). Upload the whole staging buffer once.
    // A buffer reused across sounds is still ATTACHED to its source from the previous play, and OpenAL
    // rejects alBufferData on an attached buffer (AL_INVALID_OPERATION) -- detach first, then reattach,
    // exactly as the original did. (Dropping this detach silently kept stale PCM -> broke reused SFX/voice.)
    if (!d->streaming && !playing)
    {
        if (!d->buffer)
            return DS_OK;
        if (d->source)
            alSourcei(d->source, AL_BUFFER, 0);
        alBufferData(d->buffer,
                     AlFormat(d->fmt.nChannels, d->fmt.wBitsPerSample),
                     d->staging.data(), (ALsizei)d->staging.size(), d->Rate());
        if (d->source)
            alSourcei(d->source, AL_BUFFER, (ALint)d->buffer);
        return DS_OK;
    }

    if (!d->source)
        return DS_OK;

    // Only a TRUE stream refills a partial region (one half) of a LOOPING buffer while it plays. A retriggered
    // one-shot SFX writes the whole buffer, usually while stopped -- keep those on the static path so they are
    // never hijacked into the queue (that is what broke 3D SFX). Fall back to the static re-upload otherwise.
    const bool partial =
        (d->lockOffset != 0) || (d->lockBytes < d->staging.size());
    if (!d->streaming && !(d->looping && partial))
    {
        if (!d->buffer)
            return DS_OK;
        alSourceStop(d->source);
        alSourcei(d->source, AL_BUFFER, 0);
        alBufferData(d->buffer,
                     AlFormat(d->fmt.nChannels, d->fmt.wBitsPerSample),
                     d->staging.data(), (ALsizei)d->staging.size(), d->Rate());
        alSourcei(d->source, AL_BUFFER, (ALint)d->buffer);
        alSourcePlay(d->source);
        return DS_OK;
    }

    // A partial write into a looping buffer while it plays == a streaming refill. On the first one, convert the
    // source from the static looping buffer to a queued chain; thereafter just queue each refilled region.
    if (!d->streaming)
    {
        d->streaming = true;
        d->ringBytes = (DWORD)d->staging.size();
        d->halfBytes =
            d->lockBytes ? d->lockBytes : (d->ringBytes ? d->ringBytes / 2 : 0);
        d->consumedBytes = 0;
        alSourceStop(d->source);
        alSourcei(
            d->source, AL_LOOPING,
            AL_FALSE); // the stream loops at the FILE level, not the buffer
        alSourcei(d->source, AL_BUFFER,
                  0); // detach static buffer -> source enters queue mode
    }

    d->DrainProcessed(); // recycle finished buffers, advance the play cursor

    DWORD off = d->lockOffset, len = d->lockBytes;
    if (off > d->staging.size())
        off = (DWORD)d->staging.size();
    if (len > d->staging.size() - off)
        len = (DWORD)d->staging.size() - off;
    if (len)
    {
        ALuint b = d->TakeBuf();
        alBufferData(b, AlFormat(d->fmt.nChannels, d->fmt.wBitsPerSample),
                     d->staging.data() + off, (ALsizei)len, d->Rate());
        alSourceQueueBuffers(d->source, 1, &b);
    }

    // (Re)start after the switch, or recover from an underrun where the queue drained and the source
    // auto-stopped -- a rare small gap instead of the per-refill jerk the static re-upload produced.
    ALint s2 = AL_STOPPED;
    alGetSourcei(d->source, AL_SOURCE_STATE, &s2);
    if (s2 != AL_PLAYING)
        alSourcePlay(d->source);
    return DS_OK;
}
HRESULT IDirectSoundBuffer::SetVolume(LONG mb)
{
    if (!d)
        return DSERR_GENERIC;
    d->volMb = mb;
    if (d->source)
        alSourcef(d->source, AL_GAIN, MilliBelToGain(mb));
    return DS_OK;
}
HRESULT IDirectSoundBuffer::GetVolume(LONG* pmb)
{
    if (pmb && d)
        *pmb = d->volMb;
    return DS_OK;
}
HRESULT IDirectSoundBuffer::SetPan(LONG mb)
{
    if (!d)
        return DSERR_GENERIC;
    d->panMb = mb;
    d->ApplyPan();
    return DS_OK;
}
HRESULT IDirectSoundBuffer::GetPan(LONG* pmb)
{
    if (pmb && d)
        *pmb = d->panMb;
    return DS_OK;
}
HRESULT IDirectSoundBuffer::SetFrequency(DWORD hz)
{
    if (!d || !d->source)
        return DSERR_GENERIC;
    float pitch =
        (hz == 0 || d->origFreq == 0) ? 1.0f : (float)hz / (float)d->origFreq;
    alSourcef(d->source, AL_PITCH, pitch > 0.0f ? pitch : 1.0f);
    return DS_OK;
}
HRESULT IDirectSoundBuffer::GetFrequency(DWORD* phz)
{
    if (phz && d)
        *phz = d->origFreq;
    return DS_OK;
}
HRESULT IDirectSoundBuffer::GetStatus(DWORD* pStatus)
{
    if (!pStatus)
        return DSERR_INVALIDPARAM;
    *pStatus = 0;
    if (d && d->streaming)
    {
        // Active stream: report PLAYING through transient queue underruns (which Unlock auto-recovers) so
        // the stream isn't mistaken for finished -- true end-of-stream is signalled via SND_STREAM_DONE.
        *pStatus |= DSBSTATUS_PLAYING | DSBSTATUS_LOOPING;
    }
    else if (d && d->source)
    {
        ALint st = 0;
        alGetSourcei(d->source, AL_SOURCE_STATE, &st);
        if (st == AL_PLAYING)
            *pStatus |= DSBSTATUS_PLAYING;
        if (d->looping)
            *pStatus |= DSBSTATUS_LOOPING;
    }
    return DS_OK;
}
HRESULT IDirectSoundBuffer::SetCurrentPosition(DWORD bytes)
{
    if (d && d->source)
        alSourcei(d->source, AL_BYTE_OFFSET, (ALint)bytes);
    return DS_OK;
}
HRESULT IDirectSoundBuffer::GetCurrentPosition(DWORD* pPlay, DWORD* pWrite)
{
    if (d && d->streaming && d->source && d->ringBytes)
    {
        // Synthesise the circular play cursor ProcessStream polls: which half is playing (parity of how
        // many HalfSize buffers have finished), plus the offset within the current buffer for smoothness.
        // Each queued buffer maps to alternating circular halves, so this reproduces the 0->Half->wrap
        // cursor that drives the half-at-a-time refill, without depending on notify events.
        d->DrainProcessed();
        const DWORD half = d->halfBytes ? d->halfBytes : d->ringBytes / 2;
        const unsigned long long halves = half ? (d->consumedBytes / half) : 0;
        DWORD pos = (DWORD)((halves & 1ull) ? half : 0);
        ALint off = 0;
        alGetSourcei(d->source, AL_BYTE_OFFSET, &off);
        if (off > 0 && (DWORD)off < half)
            pos += (DWORD)off;
        if (pos >= d->ringBytes)
            pos = d->ringBytes - 1;
        if (pPlay)
            *pPlay = pos;
        if (pWrite)
            *pWrite = pos;
        return DS_OK;
    }
    ALint off = 0;
    if (d && d->source)
        alGetSourcei(d->source, AL_BYTE_OFFSET, &off);
    if (pPlay)
        *pPlay = (DWORD)off;
    if (pWrite)
        *pWrite = (DWORD)off;
    return DS_OK;
}
HRESULT IDirectSoundBuffer::GetFormat(LPWAVEFORMATEX pwfx, DWORD size,
                                      DWORD* written)
{
    if (pwfx && size >= sizeof(WAVEFORMATEX) && d)
        std::memcpy(pwfx, &d->fmt, sizeof(WAVEFORMATEX));
    if (written)
        *written = sizeof(WAVEFORMATEX);
    return DS_OK;
}
HRESULT IDirectSoundBuffer::GetCaps(LPDSBCAPS pCaps)
{
    if (!pCaps || !d)
        return DSERR_INVALIDPARAM;
    pCaps->dwBufferBytes = (DWORD)d->staging.size();
    return DS_OK;
}
HRESULT IDirectSoundBuffer::Restore(void)
{
    return DS_OK;
} // OpenAL buffers are never "lost"
HRESULT IDirectSoundBuffer::SetFormat(LPCWAVEFORMATEX pwfx)
{
    if (pwfx && d)
    {
        std::memcpy(&d->fmt, pwfx, sizeof(WAVEFORMATEX));
        d->origFreq = d->fmt.nSamplesPerSec;
    }
    return DS_OK;
}

// ============================ IDirectSound3DBuffer ============================
HRESULT IDirectSound3DBuffer::QueryInterface(REFIID, void** ppv)
{
    if (ppv)
        *ppv = this;
    return DS_OK;
}
ULONG IDirectSound3DBuffer::AddRef(void)
{
    return 1;
}
ULONG IDirectSound3DBuffer::Release(void)
{
    return 0;
} // owned by the parent buffer
HRESULT IDirectSound3DBuffer::SetPosition(float x, float y, float z, DWORD)
{
    if (owner && owner->d && owner->d->source)
    {
        alSourcei(owner->d->source, AL_SOURCE_RELATIVE, AL_FALSE);
        alSource3f(owner->d->source, AL_POSITION, x, y, z);
    }
    return DS_OK;
}
HRESULT IDirectSound3DBuffer::SetVelocity(float x, float y, float z, DWORD)
{
    if (owner && owner->d && owner->d->source)
        alSource3f(owner->d->source, AL_VELOCITY, x, y, z);
    return DS_OK;
}
HRESULT IDirectSound3DBuffer::SetMinDistance(float dist, DWORD)
{
    if (owner && owner->d && owner->d->source)
        alSourcef(owner->d->source, AL_REFERENCE_DISTANCE, dist);
    return DS_OK;
}
HRESULT IDirectSound3DBuffer::SetMaxDistance(float dist, DWORD)
{
    if (owner && owner->d && owner->d->source)
        alSourcef(owner->d->source, AL_MAX_DISTANCE, dist);
    return DS_OK;
}
HRESULT IDirectSound3DBuffer::SetMode(DWORD mode, DWORD)
{
    // DS3DMODE_DISABLE -> treat as head-relative with no attenuation (2D). NORMAL -> world 3D.
    if (owner && owner->d && owner->d->source)
        alSourcei(owner->d->source, AL_SOURCE_RELATIVE,
                  mode == DS3DMODE_NORMAL ? AL_FALSE : AL_TRUE);
    return DS_OK;
}
HRESULT IDirectSound3DBuffer::SetConeAngles(DWORD inside, DWORD outside, DWORD)
{
    if (owner && owner->d && owner->d->source)
    {
        alSourcef(owner->d->source, AL_CONE_INNER_ANGLE, (float)inside);
        alSourcef(owner->d->source, AL_CONE_OUTER_ANGLE, (float)outside);
    }
    return DS_OK;
}

// ============================ IDirectSound3DListener ============================
HRESULT IDirectSound3DListener::QueryInterface(REFIID, void** ppv)
{
    if (ppv)
        *ppv = this;
    return DS_OK;
}
ULONG IDirectSound3DListener::AddRef(void)
{
    return 1;
}
ULONG IDirectSound3DListener::Release(void)
{
    delete this;
    return 0;
}
HRESULT IDirectSound3DListener::SetPosition(float x, float y, float z, DWORD)
{
    alListener3f(AL_POSITION, x, y, z);
    return DS_OK;
}
HRESULT IDirectSound3DListener::SetVelocity(float x, float y, float z, DWORD)
{
    alListener3f(AL_VELOCITY, x, y, z);
    return DS_OK;
}
HRESULT IDirectSound3DListener::SetOrientation(float fx, float fy, float fz,
                                               float tx, float ty, float tz,
                                               DWORD)
{
    float ori[6] = {fx, fy, fz, tx, ty, tz};
    alListenerfv(AL_ORIENTATION, ori);
    return DS_OK;
}
HRESULT IDirectSound3DListener::SetRolloffFactor(float f, DWORD)
{
    alListenerf(AL_GAIN, 1.0f);
    (void)f;
    return DS_OK;
}
HRESULT IDirectSound3DListener::SetDopplerFactor(float f, DWORD)
{
    alDopplerFactor(f);
    return DS_OK;
}
HRESULT IDirectSound3DListener::SetDistanceFactor(float f, DWORD)
{
    alSpeedOfSound(343.3f * (f > 0 ? f : 1.0f));
    return DS_OK;
}
HRESULT IDirectSound3DListener::CommitDeferredSettings(void)
{
    return DS_OK;
} // OpenAL applies immediately

// ============================ IDirectSoundNotify ============================
HRESULT IDirectSoundNotify::QueryInterface(REFIID, void** ppv)
{
    if (ppv)
        *ppv = this;
    return DS_OK;
}
ULONG IDirectSoundNotify::AddRef(void)
{
    return 1;
}
ULONG IDirectSoundNotify::Release(void)
{
    return 0;
}
HRESULT IDirectSoundNotify::SetNotificationPositions(DWORD,
                                                     LPCDSBPOSITIONNOTIFY)
{
    // TODO(streaming): OpenAL has no position-notify callback. Gapless streaming should instead poll
    // AL_BUFFERS_PROCESSED and requeue. Accepting the positions keeps the stream setup path working;
    // playback of a fully-uploaded buffer is unaffected.
    return DS_OK;
}

// ============================ IDirectSound ============================
HRESULT IDirectSound::QueryInterface(REFIID, void** ppv)
{
    if (ppv)
        *ppv = this;
    return DS_OK;
}
ULONG IDirectSound::AddRef(void)
{
    return 1;
}
ULONG IDirectSound::Release(void)
{
    if (--g_dsRefs <= 0 && g_alContext)
    {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(g_alContext);
        g_alContext = nullptr;
        alcCloseDevice(g_alDevice);
        g_alDevice = nullptr;
    }
    delete this;
    return 0;
}
HRESULT IDirectSound::CreateSoundBuffer(LPCDSBUFFERDESC desc,
                                        LPDIRECTSOUNDBUFFER* out, IUnknown*)
{
    if (!desc || !out)
        return DSERR_INVALIDPARAM;
    IDirectSoundBuffer* b = new IDirectSoundBuffer();
    b->d = new IDirectSoundBuffer::Impl();
    b->d->primary = (desc->dwFlags & DSBCAPS_PRIMARYBUFFER) != 0;
    if (desc->lpwfxFormat)
    {
        b->d->fmt = *desc->lpwfxFormat;
        b->d->origFreq = desc->lpwfxFormat->nSamplesPerSec;
    }
    if (!b->d->primary)
    {
        b->d->staging.resize(desc->dwBufferBytes ? desc->dwBufferBytes : 1);
        alGenBuffers(1, &b->d->buffer);
        alGenSources(1, &b->d->source);
        // A DirectSound buffer plays at full volume regardless of the 3D listener UNLESS it's a 3D buffer.
        // OpenAL sources default to AL_SOURCE_RELATIVE=false at world origin (0,0,0), so once the listener moves
        // to the aircraft (huge world coords) every 2D sound (voice/chatter, music, UI) gets distance-attenuated
        // to silence -- exactly why radio voice was inaudible in 3D (source PLAYING, gain 0.36, but ~0 after
        // distance rolloff). Default to HEAD-RELATIVE at the listener; a real 3D buffer's SetPosition() flips
        // AL_SOURCE_RELATIVE back to false and places it in the world.
        alSourcei(b->d->source, AL_SOURCE_RELATIVE, AL_TRUE);
        alSource3f(b->d->source, AL_POSITION, 0.0f, 0.0f, 0.0f);
        static int s_bufs = 0;
        ++s_bufs;
        if (s_bufs <= 4 || (s_bufs % 100) == 0 || !b->d->source)
            DsLog("[dsound/OpenAL] CreateSoundBuffer #%d bytes=%lu src=%u "
                  "buf=%u alErr=%d\n",
                  s_bufs, (unsigned long)desc->dwBufferBytes,
                  (unsigned)b->d->source, (unsigned)b->d->buffer,
                  (int)alGetError());
    }
    else
        DsLog("[dsound/OpenAL] CreateSoundBuffer PRIMARY\n");
    *out = b;
    return DS_OK;
}
HRESULT IDirectSound::DuplicateSoundBuffer(LPDIRECTSOUNDBUFFER original,
                                           LPDIRECTSOUNDBUFFER* dup)
{
    if (!original || !original->d || !dup)
        return DSERR_INVALIDPARAM;
    // Independent duplicate that shares the same PCM: its own OpenAL source+buffer with a copy of the
    // staging bytes, so it can play/stop and be released independently of the original.
    IDirectSoundBuffer* b = new IDirectSoundBuffer();
    b->d = new IDirectSoundBuffer::Impl();
    b->d->fmt = original->d->fmt;
    b->d->origFreq = original->d->origFreq;
    b->d->staging = original->d->staging;
    alGenBuffers(1, &b->d->buffer);
    alGenSources(1, &b->d->source);
    alSourcei(b->d->source, AL_SOURCE_RELATIVE,
              AL_TRUE); // head-relative by default (see CreateSoundBuffer)
    alSource3f(b->d->source, AL_POSITION, 0.0f, 0.0f, 0.0f);
    if (!b->d->staging.empty())
        alBufferData(b->d->buffer,
                     AlFormat(b->d->fmt.nChannels, b->d->fmt.wBitsPerSample),
                     b->d->staging.data(), (ALsizei)b->d->staging.size(),
                     (ALsizei)(b->d->fmt.nSamplesPerSec ?
                                   b->d->fmt.nSamplesPerSec :
                                   b->d->origFreq));
    alSourcei(b->d->source, AL_BUFFER, (ALint)b->d->buffer);
    *dup = b;
    return DS_OK;
}
HRESULT IDirectSound::GetCaps(LPDSCAPS caps)
{
    if (caps)
    {
        caps->dwMinSecondarySampleRate = 100;
        caps->dwMaxSecondarySampleRate = 200000;
        caps->dwPrimaryBuffers = 1;
    }
    return DS_OK;
}
HRESULT IDirectSound::SetCooperativeLevel(HWND, DWORD)
{
    return DS_OK;
} // no exclusive device on Linux
HRESULT IDirectSound::Compact(void)
{
    return DS_OK;
}

// ============================ IDirectSoundCapture ============================
// Minimal capture object: opens an OpenAL capture device so the voice path has a real input handle.
// Actual sample pumping is driven by the voice manager (SetupTalkIO); this provides the device.
HRESULT IDirectSoundCapture::QueryInterface(REFIID, void** ppv)
{
    if (ppv)
        *ppv = this;
    return DS_OK;
}
ULONG IDirectSoundCapture::AddRef(void)
{
    return 1;
}
ULONG IDirectSoundCapture::Release(void)
{
    delete this;
    return 0;
}
HRESULT IDirectSoundCapture::CreateCaptureBuffer(
    const void*, IDirectSoundCaptureBuffer** out, IUnknown*)
{
    // TODO(voice): back a capture buffer with alcCaptureOpenDevice/alcCaptureSamples.
    if (out)
        *out = nullptr;
    return DSERR_GENERIC;
}
HRESULT IDirectSoundCapture::GetCaps(void*)
{
    return DS_OK;
}

// ============================ factories + IIDs ============================
extern "C" HRESULT DirectSoundCreate(LPGUID, LPDIRECTSOUND* ppDS, IUnknown*)
{
    if (!ppDS)
        return DSERR_INVALIDPARAM;
    if (!EnsureOpenAL())
    {
        DsLog("[dsound/OpenAL] DirectSoundCreate: EnsureOpenAL FAILED\n");
        return DSERR_GENERIC;
    }
    ++g_dsRefs;
    *ppDS = new IDirectSound();
    DsLog("[dsound/OpenAL] DirectSoundCreate OK\n");
    return DS_OK;
}
extern "C" HRESULT DirectSoundCaptureCreate(LPGUID, LPDIRECTSOUNDCAPTURE* ppDSC,
                                            IUnknown*)
{
    if (!ppDSC)
        return DSERR_INVALIDPARAM;
    *ppDSC = new IDirectSoundCapture();
    return DS_OK;
}

// Interface IIDs. Real DirectSound values; only their identity (compared in QueryInterface) matters here.
extern "C" const IID IID_IDirectSound3DBuffer = {
    0x279AFA86,
    0x4981,
    0x11CE,
    {0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60}};
extern "C" const IID IID_IDirectSound3DListener = {
    0x279AFA84,
    0x4981,
    0x11CE,
    {0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60}};
extern "C" const IID IID_IDirectSoundNotify = {
    0xB0210783,
    0x89CD,
    0x11D0,
    {0xAF, 0x08, 0x00, 0xA0, 0xC9, 0x25, 0xCD, 0x16}};
