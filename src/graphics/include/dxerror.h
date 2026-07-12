/***************************************************************************\
    DXerror.h
    Scott Randolph
    November 12, 1996

    This file provide utility functions to decode Direct Draw error codes.
\***************************************************************************/
#ifndef _DXERROR_H_
#define _DXERROR_H_

#include <windows.h>


// Artscout - 2026: [DX7-PURGE] DDErrorCheck/D3DErrorCheck removed (dead DDraw/D3D7
// decoders, 0 call-sites). Only the DirectSound decoder survives.
BOOL DSErrorCheck(HRESULT result);

#endif // _DDERROR_H_
