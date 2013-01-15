/***************************************************************************\
    FarTex.h
    Miro "Jammer" Torrielli
    16Oct03

	- Begin Major Rewrite
\***************************************************************************/
#ifndef _FARRTEX_H_
#define _FARRTEX_H_

#include "TerrTex.h"
#include "Falclib\Include\FileMemMap.h"

struct IDirectDrawPalette;

extern class FarTexDB TheFarTextures;

#define IMAGE_SIZE 32

typedef struct FarTexEntry {
	BYTE		*bits;		// 8 bit pixel data (NULL if not loaded)
	UInt		handle;		// Rasterization engine texture handle (NULL if not available)
	int			refCount;	// Reference count
} FarTexEntry;

class FarTexFile : public FileMemMap
{
public:
    BYTE *GetFarTex(DWORD offset) { 
	return (BYTE *)GetData(offset*IMAGE_SIZE*IMAGE_SIZE,sizeof(BYTE)*IMAGE_SIZE*IMAGE_SIZE);
    };
};

class FarTexDB
{
  public:
    FarTexDB() { texArray = NULL; };
    ~FarTexDB() { ShiAssert( !IsReady() ); };

	BOOL Setup(DXContext *hrc, const char* texturePath);
	BOOL IsReady(void) { return (texArray != NULL); };
	void Cleanup(void);

	// Functions to load and use textures at model load time and render time
	void Request(TextureID texID);
	void Release(TextureID texID);
	void Select(ContextMPR *localContext, TextureID texID);
	void RestoreAll();
	void FlushHandles();
	bool SyncDDSTextures(bool bForce = false);

  protected:
	FarTexFile fartexFile;
	FileMemMap fartexDDSFile;

	int	texCount;
	FarTexEntry	*texArray;		// Array of texture records

	DWORD palette[256];			// Original, unlit palette data
	PaletteHandle *palHandle;	// Rasterization engine palette handle (NULL if not available)

	int	linearSize;				// Linear DDS size for fartiles.dds

	DXContext *private_rc;

	CRITICAL_SECTION cs_textureList;

	char texturePath[256];

  protected:
	// These functions actually load and release the texture bitmap memory
	void Load(DWORD offset, bool forceNoDDS = false);
	void Activate(DWORD offset);
	void Deactivate(DWORD offset);
	void Free(DWORD offset);

	// Handle time of day and lighting notifications
	static void TimeUpdateCallback(void *self);
	void SetLightLevel(void);

	bool DumpImageToFile(DWORD offset);
	bool SaveDDS_DXTn(const char *szFileName, BYTE* pDst, int dimensions);
	//THW Some helpers for season adjustement

	void HSVtoRGB( float *r, float *g, float *b, float h, float s, float v );
	void RGBtoHSV( float r, float g, float b, float *h, float *s, float *v );

  public:
  	// Current light level (0.0 to 1.0)
	float lightLevel;

#ifdef _DEBUG
  public:
	int	LoadedTextureCount;
	int	ActiveTextureCount;
#endif
};

#endif // _FARRTEX_H_
