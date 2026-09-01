// Artscout - 2026 (#104, Linux port, Phase 1 audio): <dsound.h> backed by OpenAL.
//
// This is NOT a stub: the DirectSound interfaces below are concrete classes whose methods drive a
// real OpenAL context (one source + buffer per DirectSound buffer, 3D position/velocity/listener,
// gain, pitch, pan). The implementation lives in dsound_openal.cpp. DirectSound's millibel volume
// and its Lock/Unlock "fill a buffer then play" model are mapped onto OpenAL's linear gain and
// alBufferData upload. Streaming (position notifications) is functional but approximate -- see the
// TODOs in the .cpp; nothing reports success without doing the work.
//
// Windows keeps its real DirectSound; this header is Linux-only.
#ifndef FF_WIN32SHIM_DSOUND_H
#define FF_WIN32SHIM_DSOUND_H
#include <windows.h>
#include <mmsystem.h>
#ifdef _WIN32
// Artscout - 2026: with WIN32_LEAN_AND_MEAN the SDK's windows.h leaves the COM headers (and IUnknown)
// out. This header only passes IUnknown* through (the aggregation argument, always NULL), so a forward
// declaration is all it needs -- and it stays compatible if <unknwn.h> is included elsewhere.
struct IUnknown;
#endif

// ---- return codes ----
#define DS_OK S_OK
#define DSERR_BUFFERLOST ((HRESULT)0x88780096L)
#define DSERR_INVALIDPARAM ((HRESULT)0x80070057L)
#define DSERR_GENERIC E_FAIL
#define DSERR_OUTOFMEMORY ((HRESULT)0x8007000EL)
#define DSERR_NOAGGREGATION ((HRESULT)0x80040110L)

// ---- buffer capability / control flags ----
#define DSBCAPS_PRIMARYBUFFER 0x00000001
#define DSBCAPS_STATIC 0x00000002
#define DSBCAPS_LOCHARDWARE 0x00000004
#define DSBCAPS_LOCSOFTWARE 0x00000008
#define DSBCAPS_CTRL3D 0x00000010
#define DSBCAPS_CTRLFREQUENCY 0x00000020
#define DSBCAPS_CTRLPAN 0x00000040
#define DSBCAPS_CTRLVOLUME 0x00000080
#define DSBCAPS_CTRLPOSITIONNOTIFY 0x00000100
#define DSBCAPS_CTRLFX 0x00000200
#define DSBCAPS_STICKYFOCUS 0x00004000
#define DSBCAPS_GLOBALFOCUS 0x00008000
#define DSBCAPS_GETCURRENTPOSITION2 0x00010000
#define DSBCAPS_MUTE3DATMAXDISTANCE 0x00020000
#define DSBCAPS_LOCDEFER 0x00040000
// legacy aliases used by the engine
#define DSBCAPS_CTRL DSBCAPS_CTRLVOLUME
#define DSBCAPS_GETCURRENTPOSITION DSBCAPS_GETCURRENTPOSITION2
#define DSBCAPS_MUTE DSBCAPS_MUTE3DATMAXDISTANCE

// ---- play / status / lock ----
#define DSBPLAY_LOOPING 0x00000001
#define DSBSTATUS_PLAYING 0x00000001
#define DSBSTATUS_BUFFERLOST 0x00000002
#define DSBSTATUS_LOOPING 0x00000004
#define DSBLOCK_FROMWRITECURSOR 0x00000001
#define DSBLOCK_ENTIREBUFFER 0x00000002

// ---- cooperative levels ----
#define DSSCL_NORMAL 1
#define DSSCL_PRIORITY 2
#define DSSCL_EXCLUSIVE 3
#define DSSCL_WRITEPRIMARY 4

// ---- volume / pan / frequency ranges ----
#define DSBVOLUME_MIN (-10000)
#define DSBVOLUME_MAX 0
#define DSBPAN_LEFT (-10000)
#define DSBPAN_CENTER 0
#define DSBPAN_RIGHT 10000
#define DSBFREQUENCY_MIN 100
#define DSBFREQUENCY_MAX 200000

// ---- 3D mode / apply ----
#define DS3DMODE_NORMAL 0
#define DS3DMODE_HEADRELATIVE 1
#define DS3DMODE_DISABLE 2
#define DS3D_IMMEDIATE 0
#define DS3D_DEFERRED 1

// ---- device caps flags (queried, rarely acted on) ----
#define DSCAPS_PRIMARYMONO 0x00000001
#define DSCAPS_PRIMARYSTEREO 0x00000002
#define DSCAPS_PRIMARY16BIT 0x00000008
#define DSCAPS_CONTINUOUSRATE 0x00000010
#define DSCAPS_EMULDRIVER 0x00000020
#define DSCAPS_CERTIFIED 0x00000040

// ---- structs ----
typedef struct _DSBUFFERDESC
{
    DWORD dwSize, dwFlags, dwBufferBytes, dwReserved;
    LPWAVEFORMATEX lpwfxFormat;
    GUID guid3DAlgorithm;
} DSBUFFERDESC, *LPDSBUFFERDESC;
typedef const DSBUFFERDESC* LPCDSBUFFERDESC;

typedef struct _DSBCAPS
{
    DWORD dwSize, dwFlags, dwBufferBytes, dwUnlockTransferRate,
        dwPlayCpuOverhead;
} DSBCAPS, *LPDSBCAPS;

typedef struct _DSCAPS
{
    DWORD dwSize, dwFlags;
    DWORD dwMinSecondarySampleRate, dwMaxSecondarySampleRate;
    DWORD dwPrimaryBuffers;
    DWORD dwMaxHwMixingAllBuffers, dwMaxHwMixingStaticBuffers,
        dwMaxHwMixingStreamingBuffers;
    DWORD dwFreeHwMixingAllBuffers, dwFreeHwMixingStaticBuffers,
        dwFreeHwMixingStreamingBuffers;
    DWORD dwMaxHw3DAllBuffers, dwMaxHw3DStaticBuffers,
        dwMaxHw3DStreamingBuffers;
    DWORD dwFreeHw3DAllBuffers, dwFreeHw3DStaticBuffers,
        dwFreeHw3DStreamingBuffers;
    DWORD dwTotalHwMemBytes, dwFreeHwMemBytes, dwMaxContigFreeHwMemBytes;
    DWORD dwUnlockTransferRateHwBuffers, dwPlayCpuOverheadSwBuffers;
    DWORD dwReserved1, dwReserved2;
} DSCAPS, *LPDSCAPS;

typedef struct _DSBPOSITIONNOTIFY
{
    DWORD dwOffset;
    HANDLE hEventNotify;
} DSBPOSITIONNOTIFY, *LPDSBPOSITIONNOTIFY;
typedef const DSBPOSITIONNOTIFY* LPCDSBPOSITIONNOTIFY;

// ---- interface IIDs (defined in dsound_openal.cpp) ----
extern "C" const IID IID_IDirectSound3DBuffer;
extern "C" const IID IID_IDirectSound3DListener;
extern "C" const IID IID_IDirectSoundNotify;

// forward decls
class IDirectSoundBuffer;
class IDirectSound3DBuffer;
class IDirectSound3DListener;
class IDirectSoundNotify;
class IDirectSound;

// A DirectSound secondary buffer: owns one OpenAL source + buffer. The 3D and notify facets are
// created lazily by QueryInterface and share this object's OpenAL state.
class IDirectSoundBuffer
{
public:
    HRESULT QueryInterface(REFIID riid, void** ppv);
    ULONG AddRef(void);
    ULONG Release(void);
    HRESULT Play(DWORD reserved1, DWORD priority, DWORD flags);
    HRESULT Stop(void);
    HRESULT Lock(DWORD offset, DWORD bytes, void** pp1, DWORD* pb1, void** pp2,
                 DWORD* pb2, DWORD flags);
    HRESULT Unlock(void* p1, DWORD b1, void* p2, DWORD b2);
    HRESULT SetVolume(LONG mbVolume); // millibels (-10000..0)
    HRESULT GetVolume(LONG* pmb);
    HRESULT SetPan(LONG mbPan); // -10000..10000
    HRESULT GetPan(LONG* pmb);
    HRESULT SetFrequency(DWORD hz); // 0 == original
    HRESULT GetFrequency(DWORD* phz);
    HRESULT GetStatus(DWORD* pStatus);
    HRESULT SetCurrentPosition(DWORD bytes);
    HRESULT GetCurrentPosition(DWORD* pPlay, DWORD* pWrite);
    HRESULT GetFormat(LPWAVEFORMATEX pwfx, DWORD size, DWORD* written);
    HRESULT GetCaps(LPDSBCAPS pCaps);
    HRESULT Restore(void);
    HRESULT SetFormat(LPCWAVEFORMATEX pwfx);
    struct Impl;
    Impl* d = nullptr; // backend state
};
typedef IDirectSoundBuffer* LPDIRECTSOUNDBUFFER;

class IDirectSound3DBuffer
{
public:
    HRESULT QueryInterface(REFIID riid, void** ppv);
    ULONG AddRef(void);
    ULONG Release(void);
    HRESULT SetPosition(float x, float y, float z, DWORD apply);
    HRESULT SetVelocity(float x, float y, float z, DWORD apply);
    HRESULT SetMinDistance(float d, DWORD apply);
    HRESULT SetMaxDistance(float d, DWORD apply);
    HRESULT SetMode(DWORD mode, DWORD apply);
    HRESULT SetConeAngles(DWORD inside, DWORD outside, DWORD apply);
    IDirectSoundBuffer* owner = nullptr;
};
typedef IDirectSound3DBuffer* LPDIRECTSOUND3DBUFFER;

class IDirectSound3DListener
{
public:
    HRESULT QueryInterface(REFIID riid, void** ppv);
    ULONG AddRef(void);
    ULONG Release(void);
    HRESULT SetPosition(float x, float y, float z, DWORD apply);
    HRESULT SetVelocity(float x, float y, float z, DWORD apply);
    HRESULT SetOrientation(float fx, float fy, float fz, float tx, float ty,
                           float tz, DWORD apply);
    HRESULT SetRolloffFactor(float f, DWORD apply);
    HRESULT SetDopplerFactor(float f, DWORD apply);
    HRESULT SetDistanceFactor(float f, DWORD apply);
    HRESULT CommitDeferredSettings(void);
};
typedef IDirectSound3DListener* LPDIRECTSOUND3DLISTENER;

class IDirectSoundNotify
{
public:
    HRESULT QueryInterface(REFIID riid, void** ppv);
    ULONG AddRef(void);
    ULONG Release(void);
    HRESULT SetNotificationPositions(DWORD count,
                                     LPCDSBPOSITIONNOTIFY notifies);
    IDirectSoundBuffer* owner = nullptr;
};
typedef IDirectSoundNotify* LPDIRECTSOUNDNOTIFY;

class IDirectSound
{
public:
    HRESULT QueryInterface(REFIID riid, void** ppv);
    ULONG AddRef(void);
    ULONG Release(void);
    HRESULT CreateSoundBuffer(LPCDSBUFFERDESC desc, LPDIRECTSOUNDBUFFER* out,
                              IUnknown* outer);
    HRESULT DuplicateSoundBuffer(LPDIRECTSOUNDBUFFER original,
                                 LPDIRECTSOUNDBUFFER* duplicate);
    HRESULT GetCaps(LPDSCAPS caps);
    HRESULT SetCooperativeLevel(HWND hwnd, DWORD level);
    HRESULT Compact(void);
};
typedef IDirectSound* LPDIRECTSOUND;

// ---- capture (voice mic input) ----
class IDirectSoundCaptureBuffer;
class IDirectSoundCapture
{
public:
    HRESULT QueryInterface(REFIID riid, void** ppv);
    ULONG AddRef(void);
    ULONG Release(void);
    HRESULT CreateCaptureBuffer(const void* desc,
                                IDirectSoundCaptureBuffer** out,
                                IUnknown* outer);
    HRESULT GetCaps(void* caps);
};
typedef IDirectSoundCapture* LPDIRECTSOUNDCAPTURE;

extern "C" HRESULT DirectSoundCreate(LPGUID lpGuid, LPDIRECTSOUND* ppDS,
                                     IUnknown* pUnkOuter);
extern "C" HRESULT DirectSoundCaptureCreate(LPGUID lpGuid,
                                            LPDIRECTSOUNDCAPTURE* ppDSC,
                                            IUnknown* pUnkOuter);

#endif // FF_WIN32SHIM_DSOUND_H
