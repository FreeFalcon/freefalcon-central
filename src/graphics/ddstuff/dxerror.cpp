/***************************************************************************\
    DXerror.cpp
    Scott Randolph
    November 12, 1996

    This file provide utility functions to decode Direct Draw error codes.
\***************************************************************************/
// Artscout - 2026: [DX7-PURGE] DDErrorCheck (DirectDraw) and D3DErrorCheck
// (Direct3D7) were dead code -- 0 call-sites -- and were the only reason this
// file pulled <ddraw.h>/<d3d.h>. Removed them and both DX7 headers. Only
// DSErrorCheck (DirectSound) survives; it is still called from fsound.cpp.
// NB: <ddraw.h> used to pull <windows.h> in ahead of <mmsystem.h>; now that it
// is gone we must include <windows.h> ourselves so mmsystem/mciapi see WinAPI types.
#include <windows.h>

#pragma warning(push)
#pragma warning(disable : 4201)
#include <mmsystem.h>
#pragma warning(pop)

#include <dsound.h>
#include "dxerror.h"


// Convert a DirectSound return code into an error message
BOOL DSErrorCheck(HRESULT result)
{
    switch (result)
    {

    case DS_OK:
        return TRUE;

    case DSERR_ALLOCATED:
        MessageBox(NULL, "DSERR_ALLOCATED", "DSound Error", MB_OK);
        return FALSE;

    case DSERR_CONTROLUNAVAIL:
        MessageBox(NULL, "DSERR_CONTROLUNAVAIL", "DSound Error", MB_OK);
        return FALSE;

    case DSERR_INVALIDPARAM:
        MessageBox(NULL, "DSERR_INVALIDPARAM", "DSound Error", MB_OK);
        return FALSE;

    case DSERR_INVALIDCALL:
        MessageBox(NULL, "DSERR_INVALIDCALL", "DSound Error", MB_OK);
        return FALSE;

    case DSERR_GENERIC:
        MessageBox(NULL, "DSERR_GENERIC", "DSound Error", MB_OK);
        return FALSE;

    case DSERR_PRIOLEVELNEEDED:
        MessageBox(NULL, "DSERR_PRIOLEVELNEEDED", "DSound Error", MB_OK);
        return FALSE;

    case DSERR_OUTOFMEMORY:
        MessageBox(NULL, "DSERR_OUTOFMEMORY", "DSound Error", MB_OK);
        return FALSE;

    case DSERR_BADFORMAT:
        MessageBox(NULL, "DSERR_BADFORMAT", "DSound Error", MB_OK);
        return FALSE;

    case DSERR_UNSUPPORTED:
        MessageBox(NULL, "DSERR_UNSUPPORTED", "DSound Error", MB_OK);
        return FALSE;

    case DSERR_NODRIVER:
        MessageBox(NULL, "DSERR_NODRIVER", "DSound Error", MB_OK);
        return FALSE;

    case DSERR_ALREADYINITIALIZED:
        MessageBox(NULL, "DSERR_ALREADYINITIALIZED", "DSound Error", MB_OK);
        return FALSE;

    case DSERR_NOAGGREGATION:
        MessageBox(NULL, "DSERR_NOAGGREGATION", "DSound Error", MB_OK);
        return FALSE;

    case DSERR_BUFFERLOST:
        MessageBox(NULL, "DSERR_BUFFERLOST", "DSound Error", MB_OK);
        return FALSE;

    case DSERR_OTHERAPPHASPRIO:
        MessageBox(NULL, "DSERR_OTHERAPPHASPRIO", "DSound Error", MB_OK);
        return FALSE;

    case DSERR_UNINITIALIZED:
        MessageBox(NULL, "DSERR_UNINITIALIZED", "DSound Error", MB_OK);
        return FALSE;

    default:
        MessageBox(NULL, "UNKNOWN ERROR CODE", "DSound Error", MB_OK);
        return FALSE;
    }
}
