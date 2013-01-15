#ifndef __DXTLIB_H__
#define __DXTLIB_H__

#include <nvdxt_options.h>
#include "nvdxtdll.h"

/*********************************************************************NVMH2****
Path:  C:\Dev\devrel\Nv_sdk_4\Dx8_private\PhotoShop\dxtlib
File:  dxtlib.h

Copyright (C) 1999, 2000 NVIDIA Corporation
This file is provided without support, instruction, or implied warranty of any
kind.  NVIDIA makes no guarantee of its fitness for a particular purpose and is
not liable under any circumstances for any damages or loss whatsoever arising
from the use or inability to use this file or items derived from it.

Comments:


******************************************************************************/







typedef void (*MIPFiltercallback)(int miplevel, int TotalMIPs);
typedef HRESULT (*MIPcallback)(void * data, int miplevel, DWORD size);
void set_mip_filter_callback(MIPFiltercallback callback);

// call back
// pointer to data
// mip level
// size of chunk

     




#ifndef TRGBA
#define TRGBA
typedef	struct	
{
	BYTE	rgba[4];
} rgba_t;
#endif

#ifndef TPIXEL
#define TPIXEL
union tPixel
{
  unsigned long u;
  rgba_t c;
};
#endif

#ifndef ISPOWER2
inline bool IsPower2(unsigned int x)
{              
    if ( x < 1 )
        return false;

    if (x == 1)
        return true;

    if ( x & (x-1) )        
        return false;

    return true;
}
#define ISPOWER2
#endif


/*
   Compresses an image with a user supplied callback with the data for each MIP level created
   Only supports input of RGB 24 or ARGB 32 bpp
*/

#ifdef NVDXTC
extern "C" {
#endif

NVDXTDLL_API
HRESULT nvDXTcompress(unsigned char * raw_data, // pointer to data (24 or 32 bit)
                unsigned long w, // width in texels
                unsigned long h, // height in texels
                DWORD byte_pitch,
                CompressionOptions * options,
                DWORD planes, // 3 or 4
                MIPcallback callback = 0);   // callback for generated levels

#ifdef  NVDXTC
}
#endif

// if callback is == 0 (or not specified), then WriteDTXnFile is called with all file info
//
// You must write the routines (or provide stubs)
// void WriteDTXnFile(count, buffer);
// void ReadDTXnFile(count, buffer);
// 
//
#ifdef  NVDXTDLL

typedef void (*DXTDataTransfer)(DWORD count, void *buffer);

#ifdef  NVDXTC
extern "C" {
#endif

NVDXTDLL_API void SetReadDTXnFile(DXTDataTransfer UserReadDTXnFile);
NVDXTDLL_API void SetWriteDTXnFile(DXTDataTransfer UserWriteDTXnFile);


#ifdef  NVDXTC
}
#endif

#else

void WriteDTXnFile(DWORD count, void * buffer);
void ReadDTXnFile(DWORD count, void * buffer);


#if _MSC_VER >=1300

 #ifdef _MT
  #ifdef _DLL
   #pragma message("Note: including lib: nvDXTlibMTDLL7.lib") 
   #pragma comment(lib, "nvDXTlibMTDLL7.lib")
  #else
   #pragma message("Note: including lib: nvDXTlibMT7.lib") 
   #pragma comment(lib, "nvDXTlibMT7.lib")
  #endif //_DLL
 #else
  #pragma message("Note: including lib: nvDXTlib7.lib") 
  #pragma comment(lib, "nvDXTlib7.lib")
 #endif

#else

 #ifdef _MT
  #ifdef _DLL                         
   #pragma message("Note: including lib: nvDXTlibMTDLL6.lib") 
   #pragma comment(lib, "nvDXTlibMTDLL6.lib")
  #else
   #pragma message("Note: including lib: nvDXTlibMT6.lib") 
   #pragma comment(lib, "nvDXTlibMT6.lib")
  #endif //_DLL
 #else
  #pragma message("Note: including lib: nvDXTlib6.lib") 
  #pragma comment(lib, "nvDXTlib6.lib")
 #endif

#endif





#endif // NVDXTC




#define DXTERR_INPUT_POINTER_ZERO -1
#define DXTERR_DEPTH_IS_NOT_3_OR_4 -2
#define DXTERR_NON_POWER_2 -3


/* example

LPDIRECT3DTEXTURE8 pCurrentTexture = 0; 

HRESULT LoadAllMipSurfaces(void * data, int iLevel)
{
    HRESULT hr;
    LPDIRECT3DSURFACE8 psurf;
    D3DSURFACE_DESC sd;
    D3DLOCKED_RECT lr;
       
    hr = pCurrentTexture->GetSurfaceLevel(iLevel, &psurf);
    
    if (FAILED(hr))
        return hr;
    psurf->GetDesc(&sd);
    
    
    hr = pCurrentTexture->LockRect(iLevel, &lr, NULL, 0);
    if (FAILED(hr))
        return hr;
    
    memcpy(lr.pBits, data, sd.Size);
    
    hr = pCurrentTexture->UnlockRect(iLevel);
    
    ReleasePpo(&psurf);
    
    return 0;
}
       

    hr = D3DXCreateTexture(m_pd3dDevice, Width, Height, nMips,  0,   D3DFMT_DXT3,  D3DPOOL_MANAGED, &pCurrentTexture);
    nvDXTcompress(raw_data, Width, Height, DXT3, true, 4, LoadAllMipSurfaces);

*/


	/*
    src_format
    dDXT1 
	dDXT1a  // DXT1 with one bit alpha
	dDXT3    // explicit alpha
	dDXT5    // interpolated alpha

	d4444   // a4 r4 g4 b4
	d1555   // a1 r5 g5 b5
	d565    // a0 r5 g6 b5
	d8888   // a8 r8 g8 b8
	d888    // a0 r8 g8 b8
	d555    // a0 r5 g5 b5
    d8      // paletted
    dV8U8   // DuDv
    dCxV8U8   // normal map
    dA8       // A8

      */

#ifdef NVDXTC
extern "C" {
#endif

NVDXTDLL_API
unsigned char * nvDXTdecompress(int & w, int & h, int & depth, int & total_width, int & rowBytes, int & src_format);


#ifdef NVDXTC
}
#endif

enum ColorFormat
{
	COLOR_RGB,
	COLOR_ARGB,
	COLOR_BGR,
	COLOR_BGRA,
	COLOR_RGBA,
	COLOR_ABGR,
};

#endif