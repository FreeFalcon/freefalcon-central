/***************************************************************************\
    Palette.h
    Scott Randolph
    April 1, 1997

	Provide a class to manage MPR palettes.
\***************************************************************************/
#ifndef _PALETTE_H_
#define _PALETTE_H_

#include "grtypes.h"
#include "Context.h"

struct IDirectDrawPalette;
class Texture;
class TextureHandle;
struct IDirectDraw7;

class Palette
{
#ifdef USE_SH_POOLS
	public:
	// Overload new/delete to use a SmartHeap fixed size pool
	void *operator new(size_t size) { ShiAssert( size == sizeof(Palette) ); return MemAllocFS(pool);	};
	void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
	static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(Palette), 100, 0 ); };
	static void ReleaseStorage()	{ MemPoolFree( pool ); };
	static MEM_POOL	pool;
#endif
	public:
		//sfr: sent to CPP for better debug
	Palette();//	{ refCount = 0; palHandle = NULL; memset(paletteData, 0, sizeof(paletteData)); };
	~Palette();//	{ ShiAssert( refCount == 0); };

	public:
	DWORD paletteData[256];
	PaletteHandle *palHandle;

	protected:
	int		refCount;

	public:
	static void SetupForDevice( DXContext *texRC );
	static void CleanupForDevice( DXContext *texRC );
	void Setup24( BYTE  *data24 );
	void Setup32( DWORD *data32 );
	void Cleanup();
	void Reference();
	int Release();
	void Activate()	{ if (!palHandle) UpdateMPR(); };
	void UpdateMPR( DWORD *pal );
	void UpdateMPR() { UpdateMPR( paletteData ); };
	void LightTexturePalette(Tcolor *light);
	void LightTexturePaletteRange(Tcolor *light, int start, int end);
	void LightTexturePaletteBuilding(Tcolor *light);
	void LightTexturePaletteReflection(Tcolor *light);
};

#endif // _PALETTE_H_
