/***************************************************************************\
    TerrTex.cpp
    Miro "Jammer" Torrielli
    15Oct03

 - Begin Major Rewrite
\***************************************************************************/
#include "stdafx.h"
#include <stdio.h>
#include <math.h>
#include "TimeMgr.h"
#include "TOD.h"
#include "Image.h"
#include "TerrTex.h"
#include "dxtlib.h"
#include "Falclib/Include/IsBad.h"
#include "FalcLib/include/dispopts.h"
#include "FalcLib/include/f4thread.h"

extern bool g_bEnableStaticTerrainTextures;
extern int fileout;
extern void ConvertToNormalMap(int kerneltype, int colorcnv, int alpha, float scale, int minz, bool wrap, bool bInvertX, bool bInvertY, int w, int h, int bits, void * data);
extern void ReadDTXnFile(unsigned long count, void * buffer);
extern void WriteDTXnFile(unsigned long count, void *buffer);

#include "FalcLib/include/PlayerOp.h"

#ifdef USE_SH_POOLS
MEM_POOL gTexDBMemPool = NULL;
#endif

#define ARGB_TEXEL_SIZE 4
#define ARGB_TEXEL_BITS 32

#define MAX(a,b)            ((a>b)?a:b)
#define MIN(a,b)            ((a<b)?a:b)

//#define MAXIMUM(a,b,c)      ((a>b)?MAX(a,c):MAX(b,c))
//#define MINIMUM(a,b,c)      ((a<b)?MIN(a,c):MIN(b,c))


TextureDB TheTerrTextures;

// Kludge for 094
static bool bIs092;

// sfr: constructor and destructor here
TextureDB::TextureDB() : cs_textureList(F4CreateCriticalSection("texturedb mutex")), TextureSets(NULL)
{

}
TextureDB::~TextureDB()
{
    F4DestroyCriticalSection(cs_textureList);
}

// Setup the texture database
BOOL TextureDB::Setup(DXContext *hrc, const char* path)
{
    char filename[MAX_PATH];
    HANDLE listFile;
    BOOL result;
    DWORD bytesRead;
    int dataSize;
    int i, j;


    ShiAssert(hrc);
    ShiAssert(path);

    // sfr: lock texture for setup
    F4ScopeLock sl(cs_textureList);

#ifdef USE_SH_POOLS

    if (gTexDBMemPool == NULL)
        gTexDBMemPool = MemPoolInit(0);

#endif

    // Store the texture path for future reference
    if (strlen(path) + 1 >= sizeof(texturePath))
        ShiError("Texture path name overflow");

    strcpy(texturePath, path);

    if (texturePath[strlen(texturePath) - 1] not_eq '\\')
        strcat(texturePath, "\\");

    sprintf(texturePathD, "%stexture\\", texturePath);

    // Store the rendering context to be used just for managing our textures
    private_rc = hrc;
    ShiAssert(private_rc not_eq NULL);

    // Initialize data members to default values
    overrideHandle = NULL;
    numSets = 0;

#ifdef _DEBUG
    LoadedSetCount = 0;
    LoadedTextureCount = 0;
    ActiveTextureCount = 0;
#endif

    // Initialize the lighting conditions and register for future time of day updates
    TimeUpdateCallback(this);
    TheTimeManager.RegisterTimeUpdateCB(TimeUpdateCallback, this);

    // Create the synchronization objects we'll need
    //InitializeCriticalSection(&cs_textureList);

    // Open the texture database description file
    strcpy(filename, texturePath);
    strcat(filename, "Texture.BIN");
    listFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    // Read the number of texture sets and tiles in the database.
    result = ReadFile(listFile, &numSets, sizeof(numSets), &bytesRead, NULL);

    if (result)
        result = ReadFile(listFile, &totalTiles, sizeof(totalTiles), &bytesRead, NULL);

    if ( not result)
    {
        char string[80];
        char message[120];
        PutErrorString(string);
        sprintf(message, "%s: Couldn'd read texture list.", string);
        MessageBox(NULL, message, "Proceeding with Empty List", MB_OK);
        numSets = 0;
    }

    // Allocate memory for the texture set records.
#ifdef USE_SH_POOLS
    TextureSets = (SetEntry *)MemAllocPtr(gTexDBMemPool, sizeof(SetEntry) * numSets, 0) ;
#else
    TextureSets = new SetEntry[numSets];
#endif

    if ( not TextureSets)
        ShiError("Failed to allocate memory for the texture database.");

    // Read the descriptions for the sets.
    for (i = 0; i < numSets; i++)
    {
        // Read the description
        result = ReadFile(listFile, &TextureSets[i].numTiles, sizeof(TextureSets[i].numTiles), &bytesRead, NULL);

        if (result)
            result = ReadFile(listFile, &TextureSets[i].terrainType, sizeof(TextureSets[i].terrainType), &bytesRead, NULL);

        if ( not result)
        {
            char string[80];
            char message[120];
            PutErrorString(string);
            sprintf(message, "%s: Couldn't read set description - disk error?", string);
            ShiError(message);
        }

        // Mark the set as unused
        TextureSets[i].refCount  = 0;
        TextureSets[i].palette   = NULL;
        TextureSets[i].palHandle = NULL;

        // Allocate memory for the tile headers in this set
        if (TextureSets[i].numTiles > 0)
        {
#ifdef USE_SH_POOLS
            TextureSets[i].tiles = (TileEntry *)MemAllocPtr(gTexDBMemPool, sizeof(TileEntry) * TextureSets[i].numTiles, 0);
#else
            TextureSets[i].tiles = new TileEntry[TextureSets[i].numTiles];
#endif
            ShiAssert(TextureSets[i].tiles);
        }
        else
            TextureSets[i].tiles = NULL;

        // Read the descriptions for the tiles within the sets.
        for (j = 0; j < TextureSets[i].numTiles; j++)
        {
            // Read the tile name and area and path counts
            result = ReadFile(listFile, &TextureSets[i].tiles[j].filename, sizeof(TextureSets[i].tiles[j].filename), &bytesRead, NULL);

            if (result)
                result = ReadFile(listFile, &TextureSets[i].tiles[j].nAreas, sizeof(TextureSets[i].tiles[j].nAreas), &bytesRead, NULL);

            if (result)
                result = ReadFile(listFile, &TextureSets[i].tiles[j].nPaths, sizeof(TextureSets[i].tiles[j].nPaths), &bytesRead, NULL);

            if ( not result)
            {
                char string[80];
                char message[120];
                PutErrorString(string);
                sprintf(message, "%s: Couldn't read tile header - disk error?", string);
                ShiError(message);
            }

            // Start with this tile unused
            for (int k = 0; k < TEX_LEVELS; k++)
            {
                TextureSets[i].tiles[j].refCount[k] = 0;
                TextureSets[i].tiles[j].bits[k] = NULL;
                TextureSets[i].tiles[j].bitsN[k] = NULL;
                TextureSets[i].tiles[j].handle[k] = NULL;
                TextureSets[i].tiles[j].handleN[k] = NULL;
            }

            // Now read all the areas
            if (TextureSets[i].tiles[j].nAreas == 0)
                TextureSets[i].tiles[j].Areas = NULL;
            else
            {
                dataSize = TextureSets[i].tiles[j].nAreas * sizeof(TexArea);

#ifdef USE_SH_POOLS
                TextureSets[i].tiles[j].Areas = (TexArea *)MemAllocPtr(gTexDBMemPool, sizeof(char) * dataSize, 0);
#else
                TextureSets[i].tiles[j].Areas = (TexArea *)new char[dataSize];
#endif
                ShiAssert(TextureSets[i].tiles[j].Areas);

                result = ReadFile(listFile, TextureSets[i].tiles[j].Areas, dataSize, &bytesRead, NULL);

                if ( not result)
                {
                    char string[80];
                    char message[120];
                    PutErrorString(string);
                    sprintf(message, "%s: Couldn't read tile areas - disk error?", string);
                    ShiError(message);
                }
            }

            // Now read all the paths
            if (TextureSets[i].tiles[j].nPaths == 0)
                TextureSets[i].tiles[j].Paths = NULL;
            else
            {
                dataSize = TextureSets[i].tiles[j].nPaths * sizeof(TexPath);

#ifdef USE_SH_POOLS
                TextureSets[i].tiles[j].Paths = (TexPath *)MemAllocPtr(gTexDBMemPool, sizeof(char) * dataSize, 0);
#else
                TextureSets[i].tiles[j].Paths = (TexPath *)new char[dataSize];
#endif
                ShiAssert(TextureSets[i].tiles[j].Paths);

                result = ReadFile(listFile, TextureSets[i].tiles[j].Paths, dataSize, &bytesRead, NULL);

                if ( not result)
                {
                    char string[80];
                    char message[120];
                    PutErrorString(string);
                    sprintf(message, "%s: Couldn't read tile paths - disk error?", string);
                    ShiError(message);
                }
            }

        }
    }

    CloseHandle(listFile);

    return TRUE;
}

void TextureDB::Cleanup(void)
{
    int i, j;

    ShiAssert(IsReady());
    ShiAssert(TextureSets);

    // sfr: lock texturedb for cleanup
    F4ScopeLock sl(cs_textureList);


    if ( not TextureSets) return;


    // Stop receiving time updates
    TheTimeManager.ReleaseTimeUpdateCB(TimeUpdateCallback, this);


    // Free the entire texture list (palettes will be freed as textures go away)
    for (i = 0; i < numSets; i++)
    {
        for (j = 0; j < TextureSets[i].numTiles; j++)
        {
            // Free the area descriptions
#ifdef USE_SH_POOLS
            if (TextureSets[i].tiles[j].Areas) MemFreePtr(TextureSets[i].tiles[j].Areas);

            TextureSets[i].tiles[j].Areas = NULL;

            if (TextureSets[i].tiles[j].Paths) MemFreePtr(TextureSets[i].tiles[j].Paths);

            TextureSets[i].tiles[j].Paths = NULL;
#else
            delete[] TextureSets[i].tiles[j].Areas;
            TextureSets[i].tiles[j].Areas = NULL;
            delete[] TextureSets[i].tiles[j].Paths;
            TextureSets[i].tiles[j].Paths = NULL;
#endif

            // Free the texture data
            for (int r = TEX_LEVELS - 1; r >= 0; r--)
            {
                if (TextureSets[i].tiles[j].handle[r])
                    Deactivate(&TextureSets[i], &TextureSets[i].tiles[j], r);

                if (TextureSets[i].tiles[j].bits[r])
                    Free(&TextureSets[i], &TextureSets[i].tiles[j], r);
            }
        }

#ifdef USE_SH_POOLS

        if (TextureSets[i].tiles) MemFreePtr(TextureSets[i].tiles);

#else
        delete[] TextureSets[i].tiles;
#endif
        TextureSets[i].tiles = NULL;
    }

#ifdef USE_SH_POOLS

    if (TextureSets) MemFreePtr(TextureSets);

#else
    delete[] TextureSets;
#endif
    TextureSets = NULL;

    // We no longer need our texture managment RC
    private_rc = NULL;

    ShiAssert(ActiveTextureCount == 0);
    ShiAssert(LoadedTextureCount == 0);
    ShiAssert(LoadedSetCount == 0);

#ifdef USE_SH_POOLS

    if (gTexDBMemPool not_eq NULL)
    {
        MemPoolFree(gTexDBMemPool);
        gTexDBMemPool = NULL;
    }

#endif

    // Release the sychronization objects we've been using
    // DeleteCriticalSection(&cs_textureList);
}

// Set the light level applied to the terrain textures.
void TextureDB::TimeUpdateCallback(void *self)
{
    ((TextureDB*)self)->SetLightLevel();
}

void TextureDB::SetLightLevel(void)
{
    // Store the new light level

    // Decide what color to use for lighting
    if (TheTimeOfDay.GetNVGmode())
    {
        lightLevel = NVG_LIGHT_LEVEL;
        lightColor.r = 0.0f;
        lightColor.g = NVG_LIGHT_LEVEL;
        lightColor.b = 0.0f;
    }
    else
    {
        lightLevel = TheTimeOfDay.GetLightLevel();
        TheTimeOfDay.GetTextureLightingColor(&lightColor);
    }

    // Update all the currently loaded textures
    if (DisplayOptions.m_texMode not_eq DisplayOptionsClass::TEX_MODE_DDS)
    {
        for (int i = 0; i < numSets; i++)
            if (TextureSets[i].palHandle)
                StoreMPRPalette(&TextureSets[i]);
    }
}

// Lite and store a texture palette in MPR.
void TextureDB::StoreMPRPalette(SetEntry *pSet)
{
    DWORD palette[256];
    BYTE  *to, *from, *stop;
    FLOAT tmpR, tmpG, tmpB, h, s, v;

    ShiAssert(pSet->palette);
    ShiAssert(pSet->palHandle);

    // Apply the current lighting
    from = (BYTE *)pSet->palette;

    // JB 010408 CTD
    if (F4IsBadReadPtr(from, sizeof(BYTE))) return;

    to  = (BYTE *)palette;
    stop = to + 256 * 4;

    while (to < stop)
    {
        // *to = static_cast<BYTE>(FloatToInt32(*from * lightColor.r)); to++, from++; // Red
        // *to = static_cast<BYTE>(FloatToInt32(*from * lightColor.g)); to++, from++; // Green
        // *to = static_cast<BYTE>(FloatToInt32(*from * lightColor.b)); to++, from++; // Blue
        //  to++, from++; // Alpha
        // tmpR = static_cast<BYTE>(FloatToInt32(*from * lightColor.r)); from++; // Red
        // tmpG = static_cast<BYTE>(FloatToInt32(*from * lightColor.g)); from++; // Green
        // tmpB = static_cast<BYTE>(FloatToInt32(*from * lightColor.b)); from++; // Blue
        // from++; // Alpha

        tmpR = *from * 1.0f;
        from++; // Red
        tmpG = *from * 1.0f;
        from++; // Green
        tmpB = *from * 1.0f;
        from++; // Blue
        from++; // Alpha

        // 0:Summer, 1:Fall, 2:Winter, 3:Spring

        if (PlayerOptions.Season == 1) //Autumn
        {
            if ( not ((tmpR == tmpG and tmpG == tmpB) or tmpG < 60 or (tmpR + tmpG + tmpB) / 3 > 225)) //Not Greyscale / green / not very bright
            {
                RGBtoHSV(tmpR, tmpG, tmpB, &h, &s, &v);

                if (h >= 30 and h <= 165)  //Green
                {
                    //h *= 0.6f; // min27 (yellow/orange/terracota/brown)
                    h = h * 0.33f + 15; //Shift to brown
                    s *= 1.2f; //more saturated (intenser brown, just mudy green otherwise
                    v *= 0.9f; //darker
                }
                else if ( not (v > 0.9 and s > 0.9)) //Not a strong green, but neither very bright
                {
                    s *= 0.9f; //less saturated
                    v *= 0.85f; //darken a bit
                }

                if (s > 255) s = 255;

                if (h > 255) h = 255;

                HSVtoRGB(&tmpR, &tmpG, &tmpB, h, s, v);
            }
        }
        else if (PlayerOptions.Season == 2) //Winter
        {
            if ( not (tmpR == tmpG and tmpR == tmpB) or tmpG < 60) //((tmpR+tmpG+tmpB)/3)>225) //or (tmpR == 255 and tmpG == 255))) //Greyscale //or pure color
            {
                RGBtoHSV(tmpR, tmpG, tmpB, &h, &s, &v);

                if ( not (s <= 0.2 or h == -1))  //If Not Greyscale
                {
                    if (h >= 45 and h <= 150) //If Green
                    {
                        s = 0;
                        v = 255; //Make white
                    }
                }
                else
                {
                    v *= 1.3f; //Make brighter
                }

                //else if (v<=200) v *= 0.9f; //Greyscale, but not white: darken a bit (to increase contrast)
                //else if (v>=200) v *= 1.2f; //bright...make even brighter

                //if (s==0 and v < 240) v *= 0.85f; //Greyscale, but not white: darken a bit (to increase contrast)
                //if (s>230) s = 255; //bright...make even brighter
                if (v > 255) v = 255;

                HSVtoRGB(&tmpR, &tmpG, &tmpB, h, s, v);
            }
        }
        else if (PlayerOptions.Season == 3) //Spring
        {
            RGBtoHSV(tmpR, tmpG, tmpB, &h, &s, &v);

            if ( not (s <= 0.1 or h == -1))  //Not Greyscale
            {
                if (h >= 45 and h <= 160) //Green
                {
                    s *= 0.8f;
                    v *= 1.2f;
                }

                if (s > 255) s = 255;

                if (v > 255) v = 255;

                HSVtoRGB(&tmpR, &tmpG, &tmpB, h, s, v);
            }
        }

        *to = static_cast<BYTE>(FloatToInt32(tmpR * lightColor.r));
        to++;
        *to = static_cast<BYTE>(FloatToInt32(tmpG * lightColor.g));
        to++;
        *to = static_cast<BYTE>(FloatToInt32(tmpB * lightColor.b));
        to++;
        to++; // Alpha
    }

    // Turn on the lights if it is dark enough
    if (lightLevel < 0.5f)
    {
        to = (BYTE *) bitand (palette[252]);

        if (TheTimeOfDay.GetNVGmode())
        {
            *to = 0;
            to++; // Red
            *to = 255;
            to++; // Green
            *to = 0;
            to++; // Blue
            *to = 255;
            to++; // Alpha

            *to = 0;
            to++; // Red
            *to = 255;
            to++; // Green
            *to = 0;
            to++; // Blue
            *to = 255;
            to++; // Alpha

            *to = 0;
            to++; // Red
            *to = 255;
            to++; // Green
            *to = 0;
            to++; // Blue
            *to = 255;
            to++; // Alpha

            *to = 0;
            to++; // Red
            *to = 255;
            to++; // Green
            *to = 0;
            to++; // Blue
            *to = 255;
            to++; // Alpha
        }
        else
        {
            *to = 115;
            to++; // Red
            *to = 171;
            to++; // Green
            *to = 155;
            to++; // Blue
            *to = 255;
            to++; // Alpha

            *to = 183;
            to++; // Red
            *to = 127;
            to++; // Green
            *to = 83;
            to++; // Blue
            *to = 255;
            to++; // Alpha

            *to = 171;
            to++; // Red
            *to = 179;
            to++; // Green
            *to = 139;
            to++; // Blue
            *to = 255;
            to++; // Alpha

            *to = 171;
            to++; // Red
            *to = 171;
            to++; // Green
            *to = 171;
            to++; // Blue
            *to = 255;
            to++; // Alpha
        }
    }

    ((PaletteHandle *)pSet->palHandle)->Load(
        MPR_TI_PALETTE, // Palette info
        32, // Bits per entry
        0, // Start index
        256, // Number of entries
        (BYTE *)&palette);
}

// This function is called by anyone wishing the use of a particular texture.
void TextureDB::Request(TextureID texID)
{
    int set = ExtractSet(texID);
    int tile = ExtractTile(texID);
    int res = ExtractRes(texID);
    BOOL needToLoad[TEX_LEVELS];

    ShiAssert(IsReady());
    ShiAssert(set >= 0);
    ShiAssert(set < numSets);
    ShiAssert(tile < TextureSets[set].numTiles);
    ShiAssert(res < TEX_LEVELS);

    {
        F4ScopeLock sl(cs_textureList);
        //EnterCriticalSection(&cs_textureList);

        // Decide if anyone needs to be loaded and increment our reference counts.
        // sfr: @todo it seems we are reading one texture more than we should
        for (int r = res; r >= 0; r--)
        {
            needToLoad[r] = (TextureSets[set].tiles[tile].refCount[r] == 0);
            TextureSets[set].tiles[tile].refCount[r]++;
        }

        //LeaveCriticalSection( &cs_textureList );
    }

    // Get the data for this bitmap and any lower res bitmaps required
    for (int r = res; r >= 0; r--)
    {
        if (needToLoad[r])
        {
            ShiAssert(TextureSets[set].tiles[tile].bits[r] == NULL);
            ShiAssert(TextureSets[set].tiles[tile].handle[r] == NULL);

            if (overrideHandle == NULL)
                Load(&TextureSets[set], &TextureSets[set].tiles[tile], r);
            else
            {
                // When using an override texture, we just assume its handle.
                TextureSets[set].tiles[tile].handle[res] = overrideHandle;
                TextureSets[set].tiles[tile].bits[res] = NULL;
            }
        }
    }
}

// This function must eventually be called by anyone who calls the Request function above.
void TextureDB::Release(TextureID texID)
{
    int set = ExtractSet(texID);
    int tile = ExtractTile(texID);
    int res = ExtractRes(texID);

    ShiAssert(IsReady());
    ShiAssert(set >= 0);
    ShiAssert(set < numSets);
    ShiAssert(tile < TextureSets[set].numTiles);
    ShiAssert(res < TEX_LEVELS);


    //EnterCriticalSection(&cs_textureList);
    F4ScopeLock sl(cs_textureList);

    // Release our hold on this texture and all mips
    while (res >= 0)
    {
        TextureSets[set].tiles[tile].refCount[res]--;

        if (TextureSets[set].tiles[tile].refCount[res] == 0)
        {
            if (overrideHandle == NULL)
            {
                Deactivate(&TextureSets[set], &TextureSets[set].tiles[tile], res);
                Free(&TextureSets[set], &TextureSets[set].tiles[tile], res);
            }
            else
            {
                // When using an override texture, we don't clean up just drop our handle.
                TextureSets[set].tiles[tile].handle[res] = NULL;
                TextureSets[set].tiles[tile].bits[res] = NULL;
            }
        }

        res--;
    }

    //LeaveCriticalSection(&cs_textureList);
}

// Return a pointer to the Nth path of type TYPE, where N is from the offset parameter. TYPE 0 = any type.
TexPath* TextureDB::GetPath(TextureID texID, int type, int offset)
{
    TexPath *a;
    TexPath *stop;

    int set = ExtractSet(texID);
    int tile = ExtractTile(texID);

    ShiAssert(set >= 0);
    ShiAssert(set < numSets);

    a = TextureSets[set].tiles[tile].Paths;
    stop = a + TextureSets[set].tiles[tile].nPaths;

    // Find the first entry of the required type
    if (type)
        while ((a < stop) and (a->type not_eq type))
            a++;

    // Step to the requested offset
    a += offset;

    // We didn't find enough (or any) matching types
    if ((a >= stop) or ((type) and (a->type not_eq type)))
        return NULL;

    // We found a match
    return a;
}

// Return a pointer to the Nth area of type TYPE, where N is from the offset parameter. TYPE 0 = any type.
TexArea* TextureDB::GetArea(TextureID texID, int type, int offset)
{
    TexArea *a;
    TexArea *stop;

    int set = ExtractSet(texID);
    int tile = ExtractTile(texID);

    ShiAssert(set >= 0);
    ShiAssert(set < numSets);

    a = TextureSets[set].tiles[tile].Areas;
    stop = a + TextureSets[set].tiles[tile].nAreas;

    // Find the first entry of the required type
    if (type)
        while ((a < stop) and (a->type not_eq type))
            a++;

    // Step to the requested offset
    a += offset;

    // We didn't find enough (or any) matching types
    if ((a >= stop) or ((type) and (a->type not_eq type)))
        return NULL;

    // We found a match
    return a;
}

BYTE TextureDB::GetTerrainType(TextureID texID)
{
    int set = ExtractSet(texID);

    if ((set < 0) or (set >= numSets))
        return 0;

    ShiAssert(set >= 0);
    ShiAssert(set < numSets);

    return TextureSets[set].terrainType;
}

// This function reads texel data from disk. Only the requested resolution level.
void TextureDB::Load(SetEntry* pSet, TileEntry* pTile, int res, bool forceNoDDS)
{
    char filename[MAX_PATH];
    int result;
    CImageFileMemory  texFile;


    ShiAssert(IsReady());
    ShiAssert(pSet);
    ShiAssert(pTile);
    ShiAssert( not pTile->handle[res]);
    ShiAssert( not pTile->handle[res]);

    if ( not forceNoDDS and DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS)
    {
        ReadImageDDS(pTile, res);
        pSet->palette = NULL;
    }
    else
    {
        // Construct the full texture file name including path
        strcpy(filename, texturePath);
        strcat(filename, pTile->filename);

        if (res == 1)
            filename[strlen(texturePath)] = 'M';
        else if (res == 0)
            filename[strlen(texturePath)] = 'L';

        // Make sure we recognize this file type
        texFile.imageType = CheckImageType(filename);
        ShiAssert(texFile.imageType not_eq IMAGE_TYPE_UNKNOWN);

        // Open the input file
        result = texFile.glOpenFileMem(filename);

        if (result not_eq 1)
        {
            char message[256];
            sprintf(message, "Failed to open %s", filename);
            ShiError(message);
        }

        // Read the image data (note that ReadTextureImage will close texFile for us)
        texFile.glReadFileMem();
        result = ReadTextureImage(&texFile);

        if (result not_eq GOOD_READ)
            ShiError("Failed to read terrain texture. CD Error?");

        // Store pointer to the image data
        pTile->bits[res] = (BYTE*)texFile.image.image;
        ShiAssert(pTile->bits[res]);

        // Store the width and height of the texture for use when loading the texture
        ShiAssert(texFile.image.width == texFile.image.height);
        ShiAssert(texFile.image.width < 2048);
        pTile->width[res] = texFile.image.width;
        pTile->height[res] = texFile.image.height;

        // Make sure this texture is palettized
        ShiAssert(texFile.image.palette);

        // If we already have this palette, we don't need it again
        if (pSet->palette)
            glReleaseMemory(texFile.image.palette);
        else
        {
            // This must be the first reference to this tile since we don't have a palette
            ShiAssert(pSet->refCount == 0);
            ShiAssert(pSet->palHandle == 0);

            // Save the palette from this image for future use
            pSet->palette = (DWORD*)texFile.image.palette;
        }

        ShiAssert(pSet->palette);
    }

#ifdef _DEBUG
    LoadedSetCount++;
#endif

    // Note that this tile is referencing it's parent set
    pSet->refCount++;

#ifdef _DEBUG
    LoadedTextureCount++;
#endif
}

// sfr: function to return DDS width
namespace
{
    int getDDSWidth(int flags)
    {
        if (flags bitand MPR_TI_16)
        {
            return 16;
        }
        else if (flags bitand MPR_TI_32)
        {
            return 32;
        }
        else if (flags bitand MPR_TI_64)
        {
            return 64;
        }
        else if (flags bitand MPR_TI_128)
        {
            return 128;
        }
        else if (flags bitand MPR_TI_256)
        {
            return 256;
        }
        else if (flags bitand MPR_TI_512)
        {
            return 512;
        }
        else if (flags bitand MPR_TI_1024)
        {
            return 1024;
        }
        else if (flags bitand MPR_TI_2048)
        {
            return 2048;
        }
        else
        {
            // BUG
            return 4096;
        }
    }
}

// This function sends texture data to MPR
void TextureDB::Activate(SetEntry* pSet, TileEntry* pTile, int res)
{
    ShiAssert(IsReady());
    ShiAssert(private_rc);
    ShiAssert(pSet);
    ShiAssert(pTile);
    ShiAssert(res < TEX_LEVELS);
    ShiAssert( not pTile->handle[res]);
    ShiAssert(pTile->bits[res]);

    if (DisplayOptions.m_texMode not_eq DisplayOptionsClass::TEX_MODE_DDS)
    {
        // Pass the palette to MPR if it isn't already there
        if (pSet->palHandle == 0)
        {
            pSet->palHandle = (UInt)new PaletteHandle(private_rc->m_pDD, 32, 256);
            ShiAssert(pSet->palHandle);
            StoreMPRPalette(pSet);
        }

        pTile->handle[res] = (UInt)new TextureHandle;
        ShiAssert(pTile->handle[res]);

        // Attach the palette
        ((PaletteHandle *)pSet->palHandle)->AttachToTexture((TextureHandle *)pTile->handle[res]);

        DWORD dwFlags = NULL;
        WORD info = MPR_TI_PALETTE;

        if (g_bEnableStaticTerrainTextures)
            dwFlags or_eq TextureHandle::FLAG_HINT_STATIC;

        ((TextureHandle *)pTile->handle[res])->Create("TextureDB", info, 8, static_cast<UInt16>(pTile->width[res]), static_cast<UInt16>(pTile->height[res]), dwFlags);

        ((TextureHandle *)pTile->handle[res])->Load(0, 0, (BYTE*)pTile->bits[res]);

        // Now that we don't need the local copy of the image, drop it
        glReleaseMemory((char*)pTile->bits[res]);
        pTile->bits[res] = NULL;

#ifdef _DEBUG
        LoadedTextureCount--;
#endif
    }
    else
    {
        // sfr: guess, hackers placed the flags in height member variable... sigh
        int width = getDDSWidth(pTile->height[res]);
        int widthN = getDDSWidth(pTile->heightN[res]);

#if 0

        if (pTile->height[res]&MPR_TI_16)
            width = 16;
        else if (pTile->height[res]&MPR_TI_32)
            width = 32;
        else if (pTile->height[res]&MPR_TI_64)
            width = 64;
        else if (pTile->height[res]&MPR_TI_128)
            width = 128;
        else if (pTile->height[res]&MPR_TI_256)
            width = 256;
        else if (pTile->height[res]&MPR_TI_512)
            width = 512;
        else if (pTile->height[res]&MPR_TI_1024)
            width = 1024;
        else if (pTile->height[res]&MPR_TI_2048)
            width = 2048;

#endif



        // Day texture
        pTile->handle[res] = (UInt)new TextureHandle;
        ShiAssert(pTile->handle[res]);

        ((TextureHandle *)pTile->handle[res])->Create(
            "TextureDB", (DWORD)pTile->height[res], 32, static_cast<UInt16>(width), static_cast<UInt16>(width)
        );
        ((TextureHandle *)pTile->handle[res])->Load(
            0, 0, (BYTE*)pTile->bits[res], false, false, pTile->width[res]
        );

        // Night texture
        pTile->handleN[res] = (UInt)new TextureHandle;
        ShiAssert(pTile->handleN[res]);

        ((TextureHandle *)pTile->handleN[res])->Create(
            "TextureDB", (DWORD)pTile->heightN[res], 32, static_cast<UInt16>(widthN), static_cast<UInt16>(widthN)
        );
        ((TextureHandle *)pTile->handleN[res])->Load(
            0, 0, (BYTE*)pTile->bitsN[res], false, false, pTile->widthN[res]
        );
    }

#ifdef _DEBUG
    ActiveTextureCount++;
#endif
}

// This function will release the MPR handle for the specified texture.
void TextureDB::Deactivate(SetEntry* pSet, TileEntry* pTile, int res)
{
    ShiAssert(IsReady());
    ShiAssert(pSet);
    ShiAssert(pTile);

    // Make sure we're not freeing a tile that is in use
    ShiAssert(pTile->refCount[res] == 0);

    // Day texture handle
    if (pTile->handle[res])
    {
        delete(TextureHandle *)pTile->handle[res];
        pTile->handle[res] = NULL;

        // Night texture handle
        if (pTile->handleN[res])
        {
            delete(TextureHandle *)pTile->handleN[res];
            pTile->handleN[res] = NULL;
        }

#ifdef _DEBUG
        ActiveTextureCount--;
#endif
    }

    // Day pixels
    if ((char *)pTile->bits[res])
    {
        glReleaseMemory((char *)pTile->bits[res]);
        pTile->bits[res] = NULL;

        // Night pixels
        if ((char *)pTile->bitsN[res])
        {
            glReleaseMemory((char *)pTile->bitsN[res]);
            pTile->bitsN[res] = NULL;
        }

#ifdef _DEBUG
        LoadedTextureCount--;
#endif
    }
}

// This function will release the memory image of a texture. It is called when the reference count of a texture reaches zero.
void TextureDB::Free(SetEntry* pSet, TileEntry* pTile, int res)
{
    ShiAssert(IsReady());
    ShiAssert(pSet);
    ShiAssert(pTile);

    // make sure we're not freeing a tile that is in use
    ShiAssert(pTile->refCount[res] == 0);
    ShiAssert(pTile->handle[res] == NULL);

    // KLUDGE to prevent release runtime crash
    if ( not pTile or not pSet) return;

    // Release the image memory if it isn't already gone
    if ((char*)pTile->bits[res])
    {
        glReleaseMemory((char *)pTile->bits[res]);
        pTile->bits[res] = NULL;

        // Night pixels
        if ((char*)pTile->bitsN[res])
        {
            glReleaseMemory((char *)pTile->bitsN[res]);
            pTile->bitsN[res] = NULL;
        }

#ifdef _DEBUG
        LoadedTextureCount--;
#endif
    }

    pSet->refCount--;

    // Free the set palette if no tiles are in use
    if (pSet->refCount == 0)
    {
        if (DisplayOptions.m_texMode not_eq DisplayOptionsClass::TEX_MODE_DDS)
        {
            if (pSet->palHandle)
            {
                delete(PaletteHandle *)pSet->palHandle;
                pSet->palHandle = NULL;
            }

            glReleaseMemory(pSet->palette);
            pSet->palette = NULL;
        }

#ifdef _DEBUG
        LoadedSetCount--;
#endif
    }
}

// Select a "Load"ed texture into an RC for immediate use by the rasterizer
void TextureDB::Select(ContextMPR *localContext, TextureID texID)
{
    ShiAssert(IsReady());
    ShiAssert(localContext);

    int set = ExtractSet(texID);
    int tile = ExtractTile(texID);
    int res = ExtractRes(texID);

    ShiAssert(set >= 0);
    ShiAssert(set < numSets);
    ShiAssert(tile >= 0);
    ShiAssert(tile < TextureSets[set].numTiles);

    // JB 010318 CTD
    if ( not (set >= 0 and set < numSets and tile >= 0 and tile < TextureSets[set].numTiles))
        return;

    // Make sure the texture we're trying to use is local to MPR
    if (TextureSets[set].tiles[tile].handle[res] == NULL)
    {
        ShiAssert(TextureSets[set].tiles[tile].bits[res]);
        Activate(&TextureSets[set], &TextureSets[set].tiles[tile], res);
    }

    // Day texture
    ShiAssert(TextureSets[set].tiles[tile].handle[res]);
    localContext->SelectTexture1(TextureSets[set].tiles[tile].handle[res]);

    // Night texture
    if (DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS and lightLevel < 0.5f)
    {
        ShiAssert(TextureSets[set].tiles[tile].handleN[res]);
        localContext->SelectTexture2(TextureSets[set].tiles[tile].handleN[res]);
    }
}

void TextureDB::RestoreAll()
{
    ShiAssert(IsReady());

    if ( not IsReady()) return;

    //   EnterCriticalSection(&cs_textureList);
    F4ScopeLock sl(cs_textureList);

    // Restore the entire texture list
    for (int i = 0; i < numSets; i++)
    {
        for (int j = 0; j < TextureSets[i].numTiles; j++)
        {
            for (int r = TEX_LEVELS - 1; r >= 0; r--)
            {
                if (TextureSets[i].tiles[j].handle[r])
                {
                    ((TextureHandle *)TextureSets[i].tiles[j].handle[r])->RestoreAll();

                    if (DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS)
                    {
                        ((TextureHandle *)TextureSets[i].tiles[j].handleN[r])->RestoreAll();
                    }
                }
            }
        }
    }

    // LeaveCriticalSection(&cs_textureList);
}

void TextureDB::FlushHandles()
{
    for (int i = 0; i < numSets; i++)
    {
        for (int j = 0; j < TextureSets[i].numTiles; j++)
        {
            for (int r = TEX_LEVELS - 1; r >= 0; r--)
            {
                // Free the texture data
                if (TextureSets[i].tiles[j].handle[r])
                    Deactivate(&TextureSets[i], &TextureSets[i].tiles[j], r);

                if (TextureSets[i].tiles[j].bits[r])
                    Free(&TextureSets[i], &TextureSets[i].tiles[j], r);

                if (TextureSets[i].tiles[j].refCount[r] > 0) TextureSets[i].tiles[j].refCount[r]--;
            }
        }
    }
}

bool TextureDB::SyncDDSTextures(bool bForce)
{
    ShiAssert(IsReady());

    if ( not IsReady())
        return false;

    CreateDirectory(texturePathD, NULL);

    //EnterCriticalSection(&cs_textureList);
    F4ScopeLock sl(cs_textureList);

    // Dump the entire texture list
    for (int i = 0; i < numSets; i++)
    {
        for (int j = 0; j < TextureSets[i].numTiles; j++)
        {
            for (int r = TEX_LEVELS - 1; r >= 0; r--)
            {
                Load(&TextureSets[i], &TextureSets[i].tiles[j], r, true);
                TextureSets[i].tiles[j].refCount[r]++;
                DumpImageToFile(&TextureSets[i].tiles[j], TextureSets[i].palette, r, bForce);
                Free(&TextureSets[i], &TextureSets[i].tiles[j], r);
                TextureSets[i].tiles[j].refCount[r]--;
            }
        }
    }

    //LeaveCriticalSection(&cs_textureList);

    return true;
}

bool TextureDB::DumpImageToFile(TileEntry* pTile, DWORD *palette, int res, bool bForce)
{
    DWORD dwSize, dwTmp, n, i;
	DWORD *pal = NULL;
	BYTE *pSrc = NULL;
	BYTE *pDst;
    char szFileName[256], szTemp[256], szKludge[256];
    char sep[] = ".";
    char *token;
    FILE *fp;

    ShiAssert(pTile->bits[res]);
    ShiAssert(palette);

    if ( not pTile->bits[res]) return false;

    strcpy(szTemp, (char *)pTile->filename);
    token = strtok(szTemp, sep);
    sprintf(szFileName, "%s%s.dds", texturePathD, token);

    if (res == 1)
        szFileName[strlen(texturePathD)] = 'M';
    else if (res == 0)
        szFileName[strlen(texturePathD)] = 'L';

    // Kludge for 094
    strcpy(szKludge, szFileName);
    szKludge[strlen(texturePathD)] = 'N';
    fp = fopen(szKludge, "rb");

    if (fp)
    {
        fclose(fp);
        DeleteFile(szKludge);
        bIs092 = true;
    }

    fp = fopen(szFileName, "rb");

    if ( not fp or bForce or bIs092)
    {
        if (fp)
            fclose(fp);

        dwSize = pTile->width[res] * pTile->height[res];

        pSrc = (BYTE *)pTile->bits[res];
        pDst = new BYTE[dwSize * ARGB_TEXEL_SIZE];

        pal = palette;

        // Day texture
        for (i = 0, n = 0; i < dwSize; i++, n += ARGB_TEXEL_SIZE)
        {
            dwTmp = pal[pSrc[i]];

            //ABGR to ARGB, Lowendian
            BYTE *p = (BYTE *)(&dwTmp);

            pDst[n + 0] = p[2]; //B
            pDst[n + 1] = p[1]; //G
            pDst[n + 2] = p[0]; //R
            pDst[n + 3] = p[3]; //A
        }

        SaveDDS_DXTn(szFileName, pDst, pTile->width[res]);

        delete[] pDst;
    }
    else
        fclose(fp);

    // Night texture
    sprintf(szFileName, "%s%sN.dds", texturePathD, token);

    if (res == 1)
        szFileName[strlen(texturePathD)] = 'M';
    else if (res == 0)
        szFileName[strlen(texturePathD)] = 'L';

    fp = fopen(szFileName, "rb");

    if ( not fp or bForce or bIs092)
    {
        if (fp)
            fclose(fp);

        BYTE  *to, *from, *stop;
        DWORD npal[256];

        from = (BYTE *)pal;
        to  = (BYTE *)npal;
        stop = to + 256 * 4;

        //FIXME
        while (to < stop)
        {
            *to = static_cast<BYTE>(FloatToInt32(*from * 0.f));
            to++, from++; // Red
            *to = static_cast<BYTE>(FloatToInt32(*from * 0.f));
            to++, from++; // Green
            *to = static_cast<BYTE>(FloatToInt32(*from * 0.f));
            to++, from++; // Blue
            to++, from++; // Alpha
        }

        to = (BYTE *) bitand (npal[252]);

        *to = 115;
        to++; // Red
        *to = 171;
        to++; // Green
        *to = 155;
        to++; // Blue
        *to = 255;
        to++; // Alpha

        *to = 183;
        to++; // Red
        *to = 127;
        to++; // Green
        *to = 83;
        to++; // Blue
        *to = 255;
        to++; // Alpha

        *to = 171;
        to++; // Red
        *to = 179;
        to++; // Green
        *to = 139;
        to++; // Blue
        *to = 255;
        to++; // Alpha

        *to = 171;
        to++; // Red
        *to = 171;
        to++; // Green
        *to = 171;
        to++; // Blue
        *to = 255;
        to++; // Alpha

        pDst = new BYTE[dwSize * ARGB_TEXEL_SIZE];

        for (i = 0, n = 0; i < dwSize; i++, n += ARGB_TEXEL_SIZE)
        {
            dwTmp = npal[pSrc[i]];

            //ABGR to ARGB, Lowendian
            BYTE *p = (BYTE *)(&dwTmp);

            pDst[n + 0] = p[2]; //B
            pDst[n + 1] = p[1]; //G
            pDst[n + 2] = p[0]; //R
            pDst[n + 3] = p[3]; //A
        }

        SaveDDS_DXTn(szFileName, pDst, pTile->width[res]);

        delete[] pDst;
    }
    else
        fclose(fp);

    return true;
}

void TextureDB::ReadImageDDS(TileEntry* pTile, int res)
{
    DDSURFACEDESC2 ddsd;
    DWORD dwMagic;
    char szFileName[256], szTemp[256], *token;
    char sep[] = ".";
    FILE *fp;


    strcpy(szTemp, (char *)pTile->filename);
    token = strtok(szTemp, sep);
    sprintf(szFileName, "%s%s.dds", texturePathD, token);

    if (res == 1)
    {
        szFileName[strlen(texturePathD)] = 'M';
    }
    else if (res == 0)
    {
        szFileName[strlen(texturePathD)] = 'L';
    }

    fp = fopen(szFileName, "rb");

    // FRB - bad dds file name
    if ( not fp)
        return;

    fread(&dwMagic, 1, sizeof(DWORD), fp);
    ShiAssert(dwMagic == MAKEFOURCC('D', 'D', 'S', ' '));

    // Read first compressed mipmap
    fread(&ddsd, 1, sizeof(DDSURFACEDESC2), fp);

    // MLR 1/25/2004 - Little kludge so FF can read DDS files made by dxtex
    if (ddsd.dwLinearSize == 0)
    {
        if (ddsd.ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', 'T', '3') or
            ddsd.ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', 'T', '5'))
        {
            ddsd.dwLinearSize = ddsd.dwWidth * ddsd.dwWidth;
            ddsd.dwFlags or_eq DDSD_LINEARSIZE;
        }

        if (ddsd.ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', 'T', '1'))
        {
            ddsd.dwLinearSize = ddsd.dwWidth * ddsd.dwWidth / 2;
            ddsd.dwFlags or_eq DDSD_LINEARSIZE;
        }
    }


    ShiAssert(ddsd.dwFlags bitand DDSD_LINEARSIZE)

    // Note: HACK (using height for flags)
    pTile->height[res] = MPR_TI_DDS;
    // Note: MUST BE DXT1
    pTile->height[res] or_eq MPR_TI_DXT1;

    // Note: 1024x1024 Max
    switch (ddsd.dwWidth)
    {
        case 16:
            pTile->height[res] or_eq MPR_TI_16;
            break;

        case 32:
            pTile->height[res] or_eq MPR_TI_32;
            break;

        case 64:
            pTile->height[res] or_eq MPR_TI_64;
            break;

        case 128:
            pTile->height[res] or_eq MPR_TI_128;
            break;

        case 256:
            pTile->height[res] or_eq MPR_TI_256;
            break;

        case 512:
            pTile->height[res] or_eq MPR_TI_512;
            break;

        case 1024:
            pTile->height[res] or_eq MPR_TI_1024;
            break;

        case 2048:
            pTile->height[res] or_eq MPR_TI_2048;
            break;

        default:
            ShiAssert(false);
    }

    // Note: HACK (using width for linear size)
    pTile->width[res] = ddsd.dwLinearSize;
    pTile->bits[res] = (BYTE *)glAllocateMemory(pTile->width[res], FALSE);
    fread(pTile->bits[res], 1, pTile->width[res], fp);
    fclose(fp);

    // Night tile, use same flags
    sprintf(szFileName, "%s%sN.dds", texturePathD, token);

    if (res == 1)
    {
        szFileName[strlen(texturePathD)] = 'M';
    }
    else if (res == 0)
    {
        szFileName[strlen(texturePathD)] = 'L';
    }

    fp = fopen(szFileName, "rb");
    fread(&dwMagic, 1, sizeof(DWORD), fp);
    ShiAssert(dwMagic == MAKEFOURCC('D', 'D', 'S', ' '));

    fread(&ddsd, 1, sizeof(DDSURFACEDESC2), fp);

    // MLR 1/25/2004 - Little kludge so FF can read DDS files made by dxtex
    if (ddsd.dwLinearSize == 0)
    {
        if (ddsd.ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', 'T', '3') or
            ddsd.ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', 'T', '5'))
        {
            ddsd.dwLinearSize = ddsd.dwWidth * ddsd.dwWidth;
            ddsd.dwFlags or_eq DDSD_LINEARSIZE;
        }

        if (ddsd.ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', 'T', '1'))
        {
            ddsd.dwLinearSize = ddsd.dwWidth * ddsd.dwWidth / 2;
            ddsd.dwFlags or_eq DDSD_LINEARSIZE;
        }
    }

    // Note: HACK (using height for flags)
    pTile->heightN[res] = MPR_TI_DDS;
    // Note: MUST BE DXT1
    pTile->heightN[res] or_eq MPR_TI_DXT1;

    // Note: 1024x1024 Max
    switch (ddsd.dwWidth)
    {
        case 16:
            pTile->heightN[res] or_eq MPR_TI_16;
            break;

        case 32:
            pTile->heightN[res] or_eq MPR_TI_32;
            break;

        case 64:
            pTile->heightN[res] or_eq MPR_TI_64;
            break;

        case 128:
            pTile->heightN[res] or_eq MPR_TI_128;
            break;

        case 256:
            pTile->heightN[res] or_eq MPR_TI_256;
            break;

        case 512:
            pTile->heightN[res] or_eq MPR_TI_512;
            break;

        case 1024:
            pTile->heightN[res] or_eq MPR_TI_1024;
            break;

        case 2048:
            pTile->heightN[res] or_eq MPR_TI_2048;
            break;

        default:
            ShiAssert(false);
    }

    ShiAssert(ddsd.dwFlags bitand DDSD_LINEARSIZE);
    pTile->widthN[res] = ddsd.dwLinearSize;
    pTile->bitsN[res] = (BYTE *)glAllocateMemory(pTile->widthN[res], FALSE);
    fread(pTile->bitsN[res], 1, pTile->widthN[res], fp);
    fclose(fp);
}

bool TextureDB::SaveDDS_DXTn(const char *szFileName, BYTE* pDst, int dimensions)
{
    CompressionOptions options;

#if _MSC_VER >= 1300

    fileout = _open(szFileName, O_WRONLY bitor O_BINARY bitor O_CREAT, S_IWRITE);

    options.MipMapType = dNoMipMaps;
    options.bBinaryAlpha = false;
    options.TextureFormat = dDXT1;

    //nvDXTcompress((BYTE *)pDst,dimensions,dimensions,dimensions*4,&options,4,0);

    _close(fileout);

#endif

    return true;
}


// r,g,b values are from 0 to 1
// h = [0,360], s = [0,1], v = [0,1]
// if s == 0, then h = -1 (undefined)
void TextureDB::RGBtoHSV(float r, float g, float b, float *h, float *s, float *v)
{
    float delta;

    //Value
    *v = MAXIMUM(r, g, b);
    //max = *v

    //Saturation
    delta = *v - MINIMUM(r, g, b);

    if (*v == 0.0f)
    {
        *s = 0;
        *h = -1;
        return;
    }
    else
        *s = delta / *v;

    if (*s == 0.0f)
    {
        *h = -1;
        return;
    }
    else
    {
        if (r == *v)
            *h = (g - b) / delta; // between yellow bitand magenta
        else if (g == *v)
            *h = 2 + (b - r) / delta; // between cyan bitand yellow
        else
            *h = 4 + (r - g) / delta; // between magenta bitand cyan
    }

    *h *= 60; // degrees

    if (*h < 0) *h += 360;
}

void TextureDB::HSVtoRGB(float *r, float *g, float *b, float h, float s, float v)
{
    int i;
    float f, p, q, t;

    if (s == 0 or h == -1)
    {
        // achromatic (grey)
        *r = *g = *b = v;
        return;
    }

    h /= 60.0f; // sector 0 to 5
    i = static_cast<int>(floor(h));
    f = h - i; // factorial part of h
    p = v * (1 - s);
    q = v * (1 - s * f);
    t = v * (1 - s * (1 - f));

    switch (i)
    {
        case 0:
            *r = v;
            *g = t;
            *b = p;
            break;

        case 1:
            *r = q;
            *g = v;
            *b = p;
            break;

        case 2:
            *r = p;
            *g = v;
            *b = t;
            break;

        case 3:
            *r = p;
            *g = q;
            *b = v;
            break;

        case 4:
            *r = t;
            *g = p;
            *b = v;
            break;

        default: // case 5:
            *r = v;
            *g = p;
            *b = q;
            //break;
    }
}
