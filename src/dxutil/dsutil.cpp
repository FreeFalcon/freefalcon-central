/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation. All Rights Reserved.
 *
 *  File:       dsutil.cpp
 *  Content:    Routines for dealing with sounds from resources
 *
 *
 ***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <dsound.h>

typedef struct
{
    BYTE *pbWaveData;               // pointer into wave resource (for restore)
    DWORD cbWaveSize;               // size of wave data (for restore)
    int iAlloc;                     // number of buffers.
    int iCurrent;                   // current buffer
    IDirectSoundBuffer* Buffers[1]; // list of buffers

} SNDOBJ, *HSNDOBJ;

#define _HSNDOBJ_DEFINED
#include "dsutil.h"

static const char c_szWAV[] = "WAV";

///////////////////////////////////////////////////////////////////////////////
//
// DSLoadSoundBuffer
//
///////////////////////////////////////////////////////////////////////////////

IDirectSoundBuffer *DSLoadSoundBuffer(IDirectSound *pDS, LPCTSTR lpName)
{
    IDirectSoundBuffer *pDSB = NULL;
    DSBUFFERDESC dsBD = {0};
    BYTE *pbWaveData;

    if (DSGetWaveResource(NULL, lpName, &dsBD.lpwfxFormat, &pbWaveData, &dsBD.dwBufferBytes))
    {
        dsBD.dwSize = sizeof(dsBD);
        dsBD.dwFlags = DSBCAPS_STATIC | DSBCAPS_CTRLDEFAULT;

        if (SUCCEEDED(pDS->CreateSoundBuffer(&dsBD, &pDSB, NULL)))
        {
            if (!DSFillSoundBuffer(pDSB, pbWaveData, dsBD.dwBufferBytes))
            {
               pDSB->Release();
                pDSB = NULL;
            }
        }
        else
        {
            pDSB = NULL;
        }
    }

    return pDSB;
}

///////////////////////////////////////////////////////////////////////////////
//
// DSReloadSoundBuffer
//
///////////////////////////////////////////////////////////////////////////////

BOOL DSReloadSoundBuffer(IDirectSoundBuffer *pDSB, LPCTSTR lpName)
{
    BOOL result=FALSE;
    BYTE *pbWaveData;
    DWORD cbWaveSize;

    if (DSGetWaveResource(NULL, lpName, NULL, &pbWaveData, &cbWaveSize))
    {
        if (SUCCEEDED(pDSB->Restore()) &&
            DSFillSoundBuffer(pDSB, pbWaveData, cbWaveSize))
        {
            result = TRUE;
        }
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
//
// DSGetWaveResource
//
///////////////////////////////////////////////////////////////////////////////

BOOL DSGetWaveResource(HMODULE hModule, LPCTSTR lpName,
    WAVEFORMATEX **ppWaveHeader, BYTE **ppbWaveData, DWORD *pcbWaveSize)
{
    HRSRC hResInfo;
    HGLOBAL hResData;
    void *pvRes;

    if (((hResInfo = FindResource(hModule, lpName, c_szWAV)) != NULL) &&
        ((hResData = LoadResource(hModule, hResInfo)) != NULL) &&
        ((pvRes = LockResource(hResData)) != NULL) &&
        DSParseWaveResource(pvRes, ppWaveHeader, ppbWaveData, pcbWaveSize))
    {
        return TRUE;
    }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// SndObj fns
///////////////////////////////////////////////////////////////////////////////

SNDOBJ *SndObjCreate(IDirectSound *pDS, LPCTSTR lpName, int iConcurrent)
{
    SNDOBJ *pSO = NULL;
    LPWAVEFORMATEX pWaveHeader;
    BYTE *pbData;
    UINT cbData;

    if (DSGetWaveResource(NULL, lpName, &pWaveHeader, &pbData, (unsigned long *)&cbData))
    {
        if (iConcurrent < 1)
            iConcurrent = 1;

        if ((pSO = (SNDOBJ *)LocalAlloc(LPTR, sizeof(SNDOBJ) +
            (iConcurrent-1) * sizeof(IDirectSoundBuffer *))) != NULL)
        {
            int i;

            pSO->iAlloc = iConcurrent;
            pSO->pbWaveData = pbData;
            pSO->cbWaveSize = cbData;
            pSO->Buffers[0] = DSLoadSoundBuffer(pDS, lpName);

            for (i=1; i<pSO->iAlloc; i++)
            {
                if (FAILED(pDS->DuplicateSoundBuffer(
                    pSO->Buffers[0], &pSO->Buffers[i])))
                {
                    pSO->Buffers[i] = DSLoadSoundBuffer(pDS, lpName);
                    if (!pSO->Buffers[i]) {
                        SndObjDestroy(pSO);
                        pSO = NULL;
                        break;
                    }
                }
            }
        }
    }

    return pSO;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SndObjDestroy(SNDOBJ *pSO)
{
    if (pSO)
    {
        int i;

        for (i=0; i<pSO->iAlloc; i++)
        {
            if (pSO->Buffers[i])
            {
                pSO->Buffers[i]->Release();
                pSO->Buffers[i] = NULL;
            }
        }
        LocalFree((HANDLE)pSO);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

IDirectSoundBuffer *SndObjGetFreeBuffer(SNDOBJ *pSO)
{
    IDirectSoundBuffer *pDSB;

    if (pSO == NULL)
        return NULL;

    if (pDSB = pSO->Buffers[pSO->iCurrent])
    {
        HRESULT hres;
        DWORD dwStatus;

        hres = pDSB->GetStatus(&dwStatus);

        if (FAILED(hres))
            dwStatus = 0;

        if ((dwStatus & DSBSTATUS_PLAYING) == DSBSTATUS_PLAYING)
        {
            if (pSO->iAlloc > 1)
            {
                if (++pSO->iCurrent >= pSO->iAlloc)
                    pSO->iCurrent = 0;

                pDSB = pSO->Buffers[pSO->iCurrent];
                hres = pDSB->GetStatus(&dwStatus);

                if (SUCCEEDED(hres) && (dwStatus & DSBSTATUS_PLAYING) == DSBSTATUS_PLAYING)
                {
                    pDSB->Stop();
                    pDSB->SetCurrentPosition(0);
                }
            }
            else
            {
                pDSB = NULL;
            }
        }

        if (pDSB && (dwStatus & DSBSTATUS_BUFFERLOST))
        {
            if (FAILED(pDSB->Restore()) ||
                !DSFillSoundBuffer(pDSB, pSO->pbWaveData, pSO->cbWaveSize))
            {
                pDSB = NULL;
            }
        }
    }

    return pDSB;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

BOOL SndObjPlay(SNDOBJ *pSO, DWORD dwPlayFlags)
{
    BOOL result = FALSE;

    if (pSO == NULL)
        return FALSE;

    if ((!(dwPlayFlags & DSBPLAY_LOOPING) || (pSO->iAlloc == 1)))
    {
        IDirectSoundBuffer *pDSB = SndObjGetFreeBuffer(pSO);
        if (pDSB != NULL) {
            result = SUCCEEDED(pDSB->Play(0, 0, dwPlayFlags));
        }
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

BOOL SndObjStop(SNDOBJ *pSO)
{
    int i;

    if (pSO == NULL)
        return FALSE;

    for (i=0; i<pSO->iAlloc; i++)
    {
        pSO->Buffers[i]->Stop();
        pSO->Buffers[i]->SetCurrentPosition(0);
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

BOOL DSFillSoundBuffer(IDirectSoundBuffer *pDSB, BYTE *pbWaveData, DWORD cbWaveSize)
{
    if (pDSB && pbWaveData && cbWaveSize)
    {
        LPVOID pMem1, pMem2;
        DWORD dwSize1, dwSize2;

        if (SUCCEEDED(pDSB->Lock(0, cbWaveSize,
            &pMem1, &dwSize1, &pMem2, &dwSize2, 0)))
        {
            CopyMemory(pMem1, pbWaveData, dwSize1);

            if ( 0 != dwSize2 )
                CopyMemory(pMem2, pbWaveData+dwSize1, dwSize2);

            pDSB->Unlock(pMem1, dwSize1, pMem2, dwSize2);
            return TRUE;
        }
    }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

BOOL DSParseWaveResource(void *pvRes, WAVEFORMATEX **ppWaveHeader, BYTE **ppbWaveData,DWORD *pcbWaveSize)
{
    DWORD *pdw;
    DWORD *pdwEnd;
    DWORD dwRiff;
    DWORD dwType;
    DWORD dwLength;

    if (ppWaveHeader)
        *ppWaveHeader = NULL;

    if (ppbWaveData)
        *ppbWaveData = NULL;

    if (pcbWaveSize)
        *pcbWaveSize = 0;

    pdw = (DWORD *)pvRes;
    dwRiff = *pdw++;
    dwLength = *pdw++;
    dwType = *pdw++;

    if (dwRiff != mmioFOURCC('R', 'I', 'F', 'F'))
        goto exit;      // not even RIFF

    if (dwType != mmioFOURCC('W', 'A', 'V', 'E'))
        goto exit;      // not a WAV

    pdwEnd = (DWORD *)((BYTE *)pdw + dwLength-4);

    while (pdw < pdwEnd)
    {
        dwType = *pdw++;
        dwLength = *pdw++;

        switch (dwType)
        {
        case mmioFOURCC('f', 'm', 't', ' '):
            if (ppWaveHeader && !*ppWaveHeader)
            {
                if (dwLength < sizeof(WAVEFORMAT))
                    goto exit;      // not a WAV

                *ppWaveHeader = (WAVEFORMATEX *)pdw;

                if ((!ppbWaveData || *ppbWaveData) &&
                    (!pcbWaveSize || *pcbWaveSize))
                {
                    return TRUE;
                }
            }
            break;

        case mmioFOURCC('d', 'a', 't', 'a'):
            if ((ppbWaveData && !*ppbWaveData) ||
                (pcbWaveSize && !*pcbWaveSize))
            {
                if (ppbWaveData)
                    *ppbWaveData = (LPBYTE)pdw;

                if (pcbWaveSize)
                    *pcbWaveSize = dwLength;

                if (!ppWaveHeader || *ppWaveHeader)
                    return TRUE;
            }
            break;
        }

        pdw = (DWORD *)((BYTE *)pdw + ((dwLength+1)&~1));
    }

exit:
    return FALSE;
}
