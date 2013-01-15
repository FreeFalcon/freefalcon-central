/***************************************************************************\
    TerrTex.h
    Scott Randolph
    October 2, 1995

	Hold information on all the terrain texture tiles.
\***************************************************************************/
#ifndef _TERRTEX_H_
#define _TERRTEX_H_

#include "grtypes.h"
#include "Context.h"
#include "Image.h"

// JPO - increased to 32 bits
typedef DWORD TextureID;

// Terrain and feature types defined in the the visual basic tile tool
const int COVERAGE_NODATA		= 0;
const int COVERAGE_WATER		= 1;
const int COVERAGE_RIVER		= 2;
const int COVERAGE_SWAMP		= 3;
const int COVERAGE_PLAINS		= 4;
const int COVERAGE_BRUSH		= 5;
const int COVERAGE_THINFOREST	= 6;
const int COVERAGE_THICKFOREST	= 7;
const int COVERAGE_ROCKY		= 8;
const int COVERAGE_URBAN		= 9;
const int COVERAGE_ROAD			= 10;
const int COVERAGE_RAIL			= 11;
const int COVERAGE_BRIDGE		= 12;
const int COVERAGE_RUNWAY		= 13;
const int COVERAGE_STATION		= 14;
const int COVERAGE_OBJECT		= 15; // JB carrier

extern class TextureDB TheTerrTextures;

enum { H=0,M,L,TEX_LEVELS };

typedef struct TexArea {
	int			type;
	float		radius;
	float		x;
	float		y;
} TexArea;

typedef struct TexPath {
	int			type;
	float		width;
	float		x1,y1;
	float		x2,y2;
} TexPath;

typedef struct TileEntry {
	char 		filename[20];			// Source filename of the bitmap (extension, but no path)

	int			nAreas;
	TexArea		*Areas;					// List of special areas (NULL if none)
	int			nPaths;
	TexPath		*Paths;					// List of paths (NULL if none)

	int			width[TEX_LEVELS];		// texture width in pixels
	int			height[TEX_LEVELS];		// texture height in pixels
	BYTE		*bits[TEX_LEVELS];		// Pixel data (NULL if not loaded)
	UInt		handle[TEX_LEVELS];		// Texture handle (NULL if not available)
	int			widthN[TEX_LEVELS];     // sfr: night texture width in pixels
	int			heightN[TEX_LEVELS];    // sfr: night texture height in pixels
	BYTE		*bitsN[TEX_LEVELS];		// Pixel data for Night tiles (NULL if not loaded)
	UInt		handleN[TEX_LEVELS];	// Texture handle for Night tiles (NULL if not available)
	int			refCount[TEX_LEVELS];	// Reference count
} TileEntry;

typedef struct SetEntry {
	int			refCount;				// Reference count

	DWORD		*palette;				// 32 bit palette entries (NULL if not loaded)
	UInt		palHandle;				// Rasterization engine palette handle (NULL if not available)

	BYTE		terrainType;			// Terrain coverage type represented by this texture set
	int			numTiles;				// How many tiles in this set?
	TileEntry	*tiles;					// Array of tiles in this set
} SetEntry;

// fwd declaration of csection pointer
struct F4CSECTIONHANDLE;

class TextureDB
{
public:
	TextureDB();// : cs_textureList(F4CreateCriticalSection("texturedb mutex")), TextureSets(NULL){}
	~TextureDB();//{ F4DestroyCriticalSection(cs_textureList); };

	BOOL Setup(DXContext *hrc, const char* texturePath);
	BOOL IsReady(void) { return (TextureSets != NULL); };
	void Cleanup(void);

	// Function to force a single texture to override all others (for ACMI wireframe)
	void SetOverrideTexture(UInt texHandle) { overrideHandle = texHandle; };

	// Functions to load and use textures at model load time and render time
	void Request(TextureID texID);
	void Release(TextureID texID);
	void Select(ContextMPR *localContext, TextureID texID);

	// Misc functions
	void RestoreAll();
	bool SyncDDSTextures(bool bForce = false);
	void FlushHandles();

	// Functions to interact with the texture database entries
	const char *GetTexturePath(void) { return texturePath; };
	TexPath* GetPath(TextureID id, int type, int offset);
	TexArea* GetArea(TextureID id, int type, int offset);
	BYTE GetTerrainType(TextureID id);

protected:
	char texturePath[MAX_PATH];
	char texturePathD[MAX_PATH];

	int	totalTiles;
	int	numSets;
	SetEntry *TextureSets;	// Array of texture set records

	UInt overrideHandle;	// If nonNull, use this handle for ALL texture selects

	Tcolor lightColor;		// Current light color

	DXContext *private_rc;

	//CRITICAL_SECTION cs_textureList; // sfr: initializing this in constructor and destroying in destructor
	F4CSECTIONHANDLE *cs_textureList;

protected:
	// These functions actually load and release the texture bitmap memory
	void Load(SetEntry* pSet, TileEntry* pTile, int res, bool forceNoDDS = false);
	void Activate(SetEntry* pSet, TileEntry* pTile, int res);
	void Deactivate(SetEntry* pSet, TileEntry* pTile, int res);
	void Free(SetEntry* pSet, TileEntry* pTile, int res);

	// Extract set, tile, and resolution from a texID
	int	ExtractSet(TextureID texID)	 { return (texID >> 4) & 0xFF; };
	int	ExtractTile(TextureID texID) { return texID & 0xF;		   };
	int	ExtractRes(TextureID texID)	 { return (texID >> 12) & 0xF; };

	// Handle time of day and lighting notifications
	static void TimeUpdateCallback(void *self);
	void SetLightLevel(void);

	// This function handles lighting and storing the MPR version of a specific palette
	void StoreMPRPalette(SetEntry *pSet);

	bool DumpImageToFile(TileEntry* pTile, DWORD *palette, int res, bool bForce = false);
	void ReadImageDDS(TileEntry* pTile, int res);
	bool SaveDDS_DXTn(const char *szFileName, BYTE* pDst, int dimensions);

	//THW Some helpers for season adjustement
	void HSVtoRGB( float *r, float *g, float *b, float h, float s, float v );
	void RGBtoHSV( float r, float g, float b, float *h, float *s, float *v );

public:
	// Current light level (0.0 to 1.0)
	float lightLevel;

#ifdef _DEBUG
  public:
	int	LoadedSetCount;
	int	LoadedTextureCount;
	int	ActiveTextureCount;
#endif
};

#endif // _TERRTEX_H_
