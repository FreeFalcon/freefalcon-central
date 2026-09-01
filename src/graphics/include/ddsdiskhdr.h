// Artscout - 2026 (x64): canonical on-disk DDS header.
//
// The engine historically read DDS file headers straight into a DDSURFACEDESC2
// via fread(&ddsd, 1, sizeof(DDSURFACEDESC2), ...). That works on x86 because
// sizeof(DDSURFACEDESC2) == 124 == the on-disk DDS header size. On x64 the
// struct grows (it contains an LPVOID lpSurface, 4->8 bytes) so sizeof becomes
// 128 AND every field after lpSurface (notably ddpfPixelFormat.dwFourCC) shifts
// by 4 bytes -> the DXT FourCC is misread -> textures are treated as 32-bit and
// the GPU over-reads the (smaller) compressed buffer -> access violation.
//
// DDSDiskHeader is the fixed 124-byte on-disk layout (all DWORD, pack(1)) which
// is identical on x86 and x64. Read into it, then map the fields the engine uses
// into a DDSURFACEDESC2 with DDSDiskToDesc(). Use DDS_DISK_HEADER_SIZE wherever
// the on-disk header size is needed as a file offset.

#ifndef _DDSDISKHDR_H_
#define _DDSDISKHDR_H_

#include "d3d7compat.h"
#include <string.h>

#pragma pack(push, 1)
struct DDSDiskHeader
{
    DWORD dwSize; // 124
    DWORD dwFlags;
    DWORD dwHeight;
    DWORD dwWidth;
    DWORD dwPitchOrLinearSize;
    DWORD dwDepth;
    DWORD dwMipMapCount;
    DWORD dwReserved1[11];
    // DDS_PIXELFORMAT (32 bytes)
    DWORD pfSize; // 32
    DWORD pfFlags;
    DWORD pfFourCC;
    DWORD pfRGBBitCount;
    DWORD pfRBitMask;
    DWORD pfGBitMask;
    DWORD pfBBitMask;
    DWORD pfABitMask;
    // caps
    DWORD dwCaps;
    DWORD dwCaps2;
    DWORD dwCaps3;
    DWORD dwCaps4;
    DWORD dwReserved2;
};
#pragma pack(pop)

// On-disk DDS header size (matches sizeof(DDSURFACEDESC2) on x86).
#define DDS_DISK_HEADER_SIZE 124

inline void DDSDiskToDesc(const DDSDiskHeader &h, DDSURFACEDESC2 &d)
{
    ZeroMemory(&d, sizeof(d));
    d.dwSize = sizeof(DDSURFACEDESC2);
    d.dwFlags = h.dwFlags;
    d.dwHeight = h.dwHeight;
    d.dwWidth = h.dwWidth;
    d.dwLinearSize = h.dwPitchOrLinearSize; // union with lPitch
    d.dwMipMapCount = h.dwMipMapCount;

    d.ddpfPixelFormat.dwSize = h.pfSize;
    d.ddpfPixelFormat.dwFlags = h.pfFlags;
    d.ddpfPixelFormat.dwFourCC = h.pfFourCC;
    d.ddpfPixelFormat.dwRGBBitCount = h.pfRGBBitCount;
    d.ddpfPixelFormat.dwRBitMask = h.pfRBitMask;
    d.ddpfPixelFormat.dwGBitMask = h.pfGBitMask;
    d.ddpfPixelFormat.dwBBitMask = h.pfBBitMask;
    d.ddpfPixelFormat.dwRGBAlphaBitMask = h.pfABitMask;

    d.ddsCaps.dwCaps = h.dwCaps;
    d.ddsCaps.dwCaps2 = h.dwCaps2;
}

#endif // _DDSDISKHDR_H_
