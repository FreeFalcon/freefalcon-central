// Artscout - 2026 (#104, Linux Ф1): <mmreg.h> -- the multimedia registry: WAVEFORMATEX and the format tags. These
// are PODs the sound code fills in and parses out of .wav headers, so they are needed regardless of which audio
// backend plays the result (DirectSound on Windows; the Linux audio path is a separate piece of work).
#ifndef FF_WIN32SHIM_MMREG_H
#define FF_WIN32SHIM_MMREG_H
#include <windows.h>

#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM 0x0001
#endif
#define WAVE_FORMAT_ADPCM 0x0002
#define WAVE_FORMAT_IEEE_FLOAT 0x0003
#define WAVE_FORMAT_ALAW 0x0006
#define WAVE_FORMAT_MULAW 0x0007
#define WAVE_FORMAT_IMA_ADPCM 0x0011
#define WAVE_FORMAT_MPEGLAYER3 0x0055
#define WAVE_FORMAT_EXTENSIBLE 0xFFFE

#ifndef _WAVEFORMATEX_DEFINED
#define _WAVEFORMATEX_DEFINED
#pragma pack(push, 1)
typedef struct tWAVEFORMATEX
{
    WORD wFormatTag;
    WORD nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD nBlockAlign;
    WORD wBitsPerSample;
    WORD cbSize;
} WAVEFORMATEX, *PWAVEFORMATEX, *LPWAVEFORMATEX;
typedef const WAVEFORMATEX* LPCWAVEFORMATEX;
typedef struct waveformat_tag
{
    WORD wFormatTag;
    WORD nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD nBlockAlign;
} WAVEFORMAT, *PWAVEFORMAT, *LPWAVEFORMAT;
typedef struct pcmwaveformat_tag
{
    WAVEFORMAT wf;
    WORD wBitsPerSample;
} PCMWAVEFORMAT, *PPCMWAVEFORMAT, *LPPCMWAVEFORMAT;
#pragma pack(pop)
#endif // _WAVEFORMATEX_DEFINED
#endif
