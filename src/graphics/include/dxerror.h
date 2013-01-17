/***************************************************************************\
    DXerror.h
    Scott Randolph
    November 12, 1996

    This file provide utility functions to decode Direct Draw error codes.
\***************************************************************************/
#ifndef _DXERROR_H_
#define _DXERROR_H_

#include <windows.h>


// Convert a Direct Draw return code into an error message
BOOL DDErrorCheck( HRESULT result );
BOOL D3DErrorCheck( HRESULT result );
BOOL DSErrorCheck( HRESULT result );

#endif // _DDERROR_H_