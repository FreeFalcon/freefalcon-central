// smart.h : Smart pointer definitions

#pragma once

// #define USE_ATL_SMART_POINTERS

#ifndef USE_ATL_SMART_POINTERS
	#define COM_SMARTPTR_TYPEDEF _COM_SMARTPTR_TYPEDEF
#else
	// Emulates VC smart pointers
	#define COM_SMARTPTR_TYPEDEF(a, b) typedef CComQIPtr<a, &b> a##Ptr
#endif	// USE_ATL_SMART_POINTERS

COM_SMARTPTR_TYPEDEF(IDirectDraw, IID_IDirectDraw);
COM_SMARTPTR_TYPEDEF(IDirectDrawPalette, IID_IDirectDrawPalette);
COM_SMARTPTR_TYPEDEF(IDirectDraw7, IID_IDirectDraw7);
COM_SMARTPTR_TYPEDEF(IDirect3D, IID_IDirect3D);
COM_SMARTPTR_TYPEDEF(IDirect3D7, IID_IDirect3D7);
COM_SMARTPTR_TYPEDEF(IDirectDrawSurface7, IID_IDirectDraw7);
COM_SMARTPTR_TYPEDEF(IDirectDrawClipper, IID_IDirectDrawClipper);
COM_SMARTPTR_TYPEDEF(IDirect3DDevice7, IID_IDirect3DDevice7);
COM_SMARTPTR_TYPEDEF(IDirect3DVertexBuffer7, IID_IDirect3DVertexBuffer7);
COM_SMARTPTR_TYPEDEF(IDirectDrawGammaControl, IID_IDirectDrawGammaControl);

// Helper stuff
inline void CheckHR(HRESULT hr)
{
	if(FAILED(hr))
	{
		IErrorInfo *pEI = NULL;
		::GetErrorInfo(NULL, &pEI);
		throw _com_error(hr, pEI);
	}
}
