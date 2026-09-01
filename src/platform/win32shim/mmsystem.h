// Artscout - 2026 (#104, Linux Ф1): minimal <mmsystem.h> stub. The real behaviour (waveOut audio, joyGetPos) is
// replaced later -- audio by OpenAL (Ф4), joystick by SDL3 (Ф3). This just gives the TYPES + no-op timing so the
// headers that pull mmsystem (fsound.h, ...) compile. timeGetTime comes from windows.h (win32_time.h). Linux-only.
#ifndef FF_WIN32SHIM_MMSYSTEM_H
#define FF_WIN32SHIM_MMSYSTEM_H
#ifdef _WIN32
#error "win32shim/mmsystem.h is the Linux shim."
#endif
#include <windows.h>

typedef UINT MMRESULT;
#define MMSYSERR_NOERROR 0
#define MMSYSERR_ERROR 1
#define MMSYSERR_BADDEVICEID 2
#define MMSYSERR_NOMEM 7
#define MMSYSERR_INVALHANDLE 5
#define WAVERR_STILLPLAYING 33
#define TIMERR_NOERROR 0
#define WAVE_FORMAT_PCM 1
#define WAVE_MAPPER ((UINT) - 1)
#define CALLBACK_NULL 0x00000000
#define CALLBACK_FUNCTION 0x00030000
#define CALLBACK_EVENT 0x00050000
#define WHDR_DONE 0x00000001
#define WHDR_PREPARED 0x00000002
#define WHDR_INQUEUE 0x00000010

// timeBeginPeriod/timeEndPeriod: scheduler granularity -- irrelevant on Linux, no-op.
static inline MMRESULT timeBeginPeriod(UINT)
{
    return TIMERR_NOERROR;
}
static inline MMRESULT timeEndPeriod(UINT)
{
    return TIMERR_NOERROR;
}

// WAVEFORMATEX / WAVEFORMAT / PCMWAVEFORMAT come from mmreg.h -- as on Windows, where mmsystem.h does not define
// them either. This header used to carry its own copy, which collided whenever both were included (4 files) and,
// worse, disagreed with the real thing: it was unpacked (sizeof 20, not the SDK's 18) and its "PCMWAVEFORMAT" was
// missing wBitsPerSample entirely -- it was a WAVEFORMAT under the wrong name. These structs are read straight out
// of .wav headers, so a layout that drifts from the SDK's mis-parses real files rather than merely failing to build.
#include "mmreg.h"

FF_DECLARE_HANDLE(HWAVEOUT);
typedef HWAVEOUT* LPHWAVEOUT;
typedef struct wavehdr_tag
{
    LPSTR lpData;
    DWORD dwBufferLength, dwBytesRecorded;
    DWORD_PTR dwUser;
    DWORD dwFlags, dwLoops;
    struct wavehdr_tag* lpNext;
    DWORD_PTR reserved;
} WAVEHDR, *LPWAVEHDR;

typedef struct
{
    WORD wMid, wPid;
    unsigned int vDriverVersion;
    char szPname[32];
    DWORD dwFormats;
    WORD wChannels, wReserved1;
    DWORD dwSupport;
} WAVEOUTCAPSA, *LPWAVEOUTCAPSA;
#define WAVEOUTCAPS WAVEOUTCAPSA

// waveOut* : declared for whatever still references them; the audio layer is rewritten to OpenAL (Ф4). Provide
// weak no-op stubs so a header-only include links even before that rewrite lands.
static inline MMRESULT waveOutGetNumDevs(void)
{
    return 0;
}
static inline UINT joyGetNumDevs(void)
{
    return 0;
}

// timing struct used by some code paths
typedef struct mmtime_tag
{
    UINT wType;
    union
    {
        DWORD ms;
        DWORD sample;
        DWORD cb;
        DWORD ticks;
    } u;
} MMTIME, *LPMMTIME;
#define TIME_MS 0x0001
#define TIME_SAMPLES 0x0002

#endif // FF_WIN32SHIM_MMSYSTEM_H
