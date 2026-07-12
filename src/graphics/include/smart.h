// smart.h : Smart pointer definitions

#pragma once

// #define USE_ATL_SMART_POINTERS

#ifndef USE_ATL_SMART_POINTERS
#define COM_SMARTPTR_TYPEDEF _COM_SMARTPTR_TYPEDEF
#else
// Emulates VC smart pointers
#define COM_SMARTPTR_TYPEDEF(a, b) typedef CComQIPtr<a, &b> a##Ptr
#endif // USE_ATL_SMART_POINTERS

// Artscout - 2026: [DX7-PURGE] The DirectDraw/Direct3D7 COM smart-pointer typedefs
// (IDirectDraw7Ptr, IDirectDrawSurface7Ptr, IDirect3DDevice7Ptr, ...) were removed.
// They needed the real COM interfaces (__uuidof/IID_*) which no longer exist under
// the D3D11/D3D12 path. Their only users were the now-dead DDraw surface/device
// bring-up code in devmgr/imagebuf/tex/context, which is being excised alongside.

// Helper stuff
// Artscout - 2026: shared CHECKHR_DEFINED guard with comsup.h -- a TU that includes both must see exactly
// one definition (see comsup.h for the Release LNK2019 this prevents).
#ifndef CHECKHR_DEFINED
#define CHECKHR_DEFINED
inline void CheckHR(HRESULT hr)
{
    if (FAILED(hr))
    {
        IErrorInfo *pEI = NULL;
        ::GetErrorInfo(NULL, &pEI);
        throw _com_error(hr, pEI);
    }
}
#endif // CHECKHR_DEFINED
