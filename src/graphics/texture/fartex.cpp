/***************************************************************************\
    FarTex.cpp
    Miro "Jammer" Torrielli
    16Oct03

 - Begin Major Rewrite
\***************************************************************************/
#include "stdafx.h"
#include <stdio.h>
#include "TimeMgr.h"
#include "TOD.h"
#include "Image.h"
#include "FarTex.h"
#include "dxtlib.h"
#include "Falclib/Include/IsBad.h"
#include "FalcLib/include/playerop.h"
#include "FalcLib/include/dispopts.h"

extern bool g_bEnableStaticTerrainTextures;
extern bool g_bUseMappedFiles;
extern int fileout;
extern void ConvertToNormalMap(int kerneltype, int colorcnv, int alpha, float scale, int minz, bool wrap, bool bInvertX, bool bInvertY, int w, int h, int bits, void * data);
extern void ReadDTXnFile(unsigned long count, void * buffer);
extern void WriteDTXnFile(unsigned long count, void *buffer);

#include "FalcLib/include/PlayerOp.h"

#ifdef USE_SH_POOLS
MEM_POOL gFartexMemPool;
#endif

// OW avoid having the TextureHandle maintaining a copy of the texture bitmaps
#define _DONOT_COPY_BITS

#define ARGB_TEXEL_SIZE 4
#define ARGB_TEXEL_BITS 32

#define MAX(a,b)            ((a>b)?a:b)
#define MIN(a,b)            ((a<b)?a:b)

//#define MAXIMUM(a,b,c)      ((a>b)?MAX(a,c):MAX(b,c))
//#define MINIMUM(a,b,c)      ((a<b)?MIN(a,c):MIN(b,c))


FarTexDB TheFarTextures;
static const DWORD INVALID_TEXID = 0xFFFFFFFF;

// Setup the texture database
BOOL FarTexDB::Setup(DXContext *hrc, const char* path)
{
    char filename[MAX_PATH];
    HANDLE listFile;
    BOOL result;
    DWORD bytesRead;
    int i;
    DWORD tilesAtLOD;


    ShiAssert(hrc);
    ShiAssert(path);

#ifdef USE_SH_POOLS
    gFartexMemPool = MemPoolInitFS(sizeof(BYTE) * IMAGE_SIZE * IMAGE_SIZE, 24, MEM_POOL_SERIALIZE);
#endif

    // Store the rendering context to be used just for managing our textures
    private_rc = hrc;
    ShiAssert(private_rc);

    // Initialize data members to default values
    texCount = 0;

#ifdef _DEBUG
    LoadedTextureCount = 0;
    ActiveTextureCount = 0;
#endif

    // Create the synchronization objects we'll need
    InitializeCriticalSection(&cs_textureList);

    // Open the texture database description file
    sprintf(filename, "%s%s", path, "FarTiles.PAL");
    listFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (listFile == INVALID_HANDLE_VALUE)
    {
        char string[80];
        char message[120];
        PutErrorString(string);
        sprintf(message, "%s: Couldn't open far texture list %s", string, filename);
        ShiError(message);
    }

    // Read the palette data shared by all the distant textures
    result = ReadFile(listFile, &palette, sizeof(palette), &bytesRead, NULL);

    if ( not result)
    {
        char string[80];
        char message[120];
        PutErrorString(string);
        sprintf(message, "%s: Couldn'd read far texture palette.", string);
        ShiError(message);
    }

    // Read the number of textures in the database
    texCount = 0;
    tilesAtLOD = 0;

    do
    {
        texCount += tilesAtLOD;
        result = ReadFile(listFile, &tilesAtLOD, sizeof(tilesAtLOD), &bytesRead, NULL);
    }
    while (bytesRead == sizeof(tilesAtLOD));

    CloseHandle(listFile);

    // Create the MPR palette
    palHandle = new PaletteHandle(private_rc->m_pDD, 32, 256);
    ShiAssert(palHandle);

    // Allocate memory for the texture records
    texArray = new FarTexEntry[texCount];

    if ( not texArray)
        ShiError("Failed to allocate memory for the distant texture array.");

    // Set up the texture array
    for (i = 0; i < texCount; i++)
    {
        // For now just initialize the headers.
        texArray[i].bits = NULL;
        texArray[i].handle = NULL;
        texArray[i].refCount = 0;
    }

    // Save the path for later use
    sprintf(texturePath, "%sfartiles", path);

    // Open and hang onto the distant texture composite image file
    sprintf(filename, "%s%s", path, "FarTiles.RAW");
    fartexFile.Open(filename, FALSE, not g_bUseMappedFiles);

    // Initialize the lighting conditions and register for future time of day updates
    TimeUpdateCallback(this);
    TheTimeManager.RegisterTimeUpdateCB(TimeUpdateCallback, this);

    if (DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS)
    {
        DDSURFACEDESC2 ddsd;
        char szRawName[256];
        FILE *fp;

        // Fetch and save linear size first
        sprintf(szRawName, "%s.dds", texturePath);
        fp = fopen(szRawName, "rb");

        if ( not fp) return TRUE;

        fread(&ddsd, 1, sizeof(DDSURFACEDESC2), fp);
        ShiAssert(ddsd.dwFlags bitand DDSD_LINEARSIZE)

        linearSize = ddsd.dwLinearSize;

        fclose(fp);

        // Open and hang onto the distant texture DDS image file
        fartexDDSFile.Open(szRawName, FALSE, not g_bUseMappedFiles);
    }

    return TRUE;
}

void FarTexDB::Cleanup(void)
{
    ShiAssert(IsReady());

    // Stop receiving time updates
    TheTimeManager.ReleaseTimeUpdateCB(TimeUpdateCallback, this);

    // sfr: changed casting here
    // Free the entire texture list
    for (DWORD i = 0; (texCount > 0) and (i < ((DWORD)texCount)); ++i)
    {
        Deactivate(i);
        Free(i);
    }

    delete[] texArray;
    texArray = NULL;

    // Free the MPR palette
    if (palHandle)
    {
        delete palHandle;
        palHandle = NULL;
    }

    // We no longer need our texture managment RC
    private_rc = NULL;

    // Close the distant texture image file
    fartexFile.Close();

    if (DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS)
        fartexDDSFile.Close();

    ShiAssert(ActiveTextureCount == 0);
    ShiAssert(LoadedTextureCount == 0);

    // Release the sychronization objects we've been using
    DeleteCriticalSection(&cs_textureList);

#ifdef USE_SH_POOLS
    MemPoolFree(gFartexMemPool);
#endif
}

// Set the light level applied to the terrain textures.
void FarTexDB::TimeUpdateCallback(void *self)
{
    ((FarTexDB *)self)->SetLightLevel();
}

void FarTexDB::SetLightLevel(void)
{
    Tcolor lightColor; // Current light color
    DWORD scratchPal[256]; // Scratch palette for doing lighting calculations
    BYTE *to, *from, *stop; // Pointers used to walk through the palette
    FLOAT tmpR, tmpG, tmpB, h, s, v;

    ShiAssert(palette);
    ShiAssert(palHandle);

    // Decide what color to use for lighting
    if (TheTimeOfDay.GetNVGmode())
    {
        lightLevel  = NVG_LIGHT_LEVEL;
        lightColor.r = 0.0f;
        lightColor.g = NVG_LIGHT_LEVEL;
        lightColor.b = 0.0f;
    }
    else
    {
        lightLevel = TheTimeOfDay.GetLightLevel();
        TheTimeOfDay.GetTextureLightingColor(&lightColor);
    }

    if (DisplayOptions.m_texMode not_eq DisplayOptionsClass::TEX_MODE_DDS)
    {
        // Apply the current lighting
        from = (BYTE *)palette;
        to  = (BYTE *)scratchPal;
        stop = to + 256 * 4;

        while (to < stop)
        {
            //*to = static_cast<BYTE>(FloatToInt32(*from * lightColor.r)); to++, from++; // Red
            //*to = static_cast<BYTE>(FloatToInt32(*from * lightColor.g)); to++, from++; // Green
            //*to = static_cast<BYTE>(FloatToInt32(*from * lightColor.b)); to++, from++; // Blue
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
            to = (BYTE *) bitand (scratchPal[252]);

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

        // Update MPR's palette
        palHandle->Load(
            MPR_TI_PALETTE, // Palette info
            32, // Bits per entry
            0, // Start index
            256, // Number of entries
            (BYTE *)&scratchPal);
    }
}

// This function is called by anyone wishing the use of a particular texture.
void FarTexDB::Request(TextureID texID)
{
    BOOL needToLoad;

    ShiAssert(IsReady());

    if (texID == INVALID_TEXID) return;

    ShiAssert(texID >= (DWORD) 0);
    ShiAssert(texID < (DWORD) texCount);

    EnterCriticalSection(&cs_textureList);

    ShiAssert(texArray);

    // 2002-04-13 MN CTD fix
    if ( not texArray) return;

    // If this is the first reference, we need to load the data
    needToLoad = (texArray[texID].refCount == 0);

    // Increment our reference count.
    texArray[texID].refCount++;

    LeaveCriticalSection(&cs_textureList);

    if (needToLoad)
    {
        ShiAssert(texArray[texID].bits == NULL);
        ShiAssert(texArray[texID].handle == NULL);

        Load(texID);
        ShiAssert(texArray[texID].bits);
    }
}

// This function must eventually be called by anyone who calls the Request function above.
void FarTexDB::Release(TextureID texID)
{
    ShiAssert(IsReady());

    if (texID == INVALID_TEXID) return;

    ShiAssert(texID >= (WORD) 0);
    ShiAssert(texID < (WORD) texCount);

    EnterCriticalSection(&cs_textureList);

    // Release our hold on this texture
    texArray[texID].refCount--;

    if (texArray[texID].refCount == 0)
    {
        Deactivate(texID);
        Free(texID);
    }

    LeaveCriticalSection(&cs_textureList);
}

// This function reads texel data from disk. Only the requested resolution level.
void FarTexDB::Load(DWORD offset, bool forceNoDDS)
{
    ShiAssert(IsReady());
    ShiAssert(offset >= 0);
    ShiAssert(offset < (DWORD) texCount);
    ShiAssert(texArray[offset].bits == NULL);

    if ( not forceNoDDS and DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS)
    {
#ifdef USE_SH_POOLS
        texArray[offset].bits = (BYTE *)MemAllocFS(gFartexMemPool);
#else
        texArray[offset].bits = new BYTE[linearSize];
#endif
        ShiAssert(texArray[offset].bits);

        // Read the image data
        if ( not fartexDDSFile.ReadDataAt(sizeof(DDSURFACEDESC2) + (offset * linearSize), texArray[offset].bits, linearSize))
        {
            char string[80];
            char message[120];
            PutErrorString(string);
            sprintf(message, "%s: Couldn'd read far texture image %0d.", string, offset);
            ShiError(message);
        }
    }
    else
    {
        if (g_bUseMappedFiles)
        {
            texArray[offset].bits = fartexFile.GetFarTex(offset);
            ShiAssert(texArray[offset].bits);
        }
        else
        {
            // Allocate space for the bitmap
#ifdef USE_SH_POOLS
            texArray[offset].bits = (BYTE *)MemAllocFS(gFartexMemPool);
#else
            texArray[offset].bits = new BYTE[IMAGE_SIZE * IMAGE_SIZE];
#endif
            ShiAssert(texArray[offset].bits);

            // Read the image data
            if ( not fartexFile.ReadDataAt(offset * IMAGE_SIZE * IMAGE_SIZE, texArray[offset].bits, IMAGE_SIZE * IMAGE_SIZE))
            {
                char string[80];
                char message[120];
                PutErrorString(string);
                sprintf(message, "%s: Couldn'd read far texture image %0d.", string, offset);
                ShiError(message);
            }
        }
    }

#ifdef _DEBUG
    LoadedTextureCount++;
#endif
}

// This function sends texture data to MPR
void FarTexDB::Activate(DWORD offset)
{
    ShiAssert(IsReady());
    ShiAssert(offset >= 0);
    ShiAssert(offset < (DWORD) texCount);
    ShiAssert(texArray[offset].bits not_eq NULL);
    ShiAssert(texArray[offset].handle == NULL);

    if (DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS)
    {
        texArray[offset].handle = (UInt)new TextureHandle;
        ShiAssert(texArray[offset].handle);

        DWORD info = MPR_TI_DDS;
        info or_eq MPR_TI_DXT1;
        info or_eq MPR_TI_32;

        ((TextureHandle *)texArray[offset].handle)->Create("FarTexDB", info, 32, IMAGE_SIZE, IMAGE_SIZE);
        ((TextureHandle *)texArray[offset].handle)->Load(0, 0, texArray[offset].bits, false, true, linearSize);
    }
    else
    {
        texArray[offset].handle = (UInt)new TextureHandle;
        ShiAssert(texArray[offset].handle);
        palHandle->AttachToTexture((TextureHandle *)texArray[offset].handle);

        DWORD dwFlags = NULL;
        WORD info = MPR_TI_PALETTE;

        if (g_bEnableStaticTerrainTextures)
            dwFlags or_eq TextureHandle::FLAG_HINT_STATIC;

        ((TextureHandle *)texArray[offset].handle)->Create("FarTexDB", info, 8, IMAGE_SIZE, IMAGE_SIZE, dwFlags);

#ifdef _DONOT_COPY_BITS
        ((TextureHandle *)texArray[offset].handle)->Load(0, 0, texArray[offset].bits, false, true);
#else
        ((TextureHandle *)texArray[offset].handle)->Load(0, 0, texArray[offset].bits);
#endif
    }

    // Now that we don't need the local copy of the image, drop it
    Free(offset);

#ifdef _DEBUG
    ActiveTextureCount++;
#endif
}

// This function will release the MPR handle for the specified texture.
void FarTexDB::Deactivate(DWORD offset)
{
    ShiAssert(IsReady());
    ShiAssert(offset >= 0);
    ShiAssert(offset < (DWORD) texCount);
    ShiAssert(texArray[offset].refCount == 0);

    if (texArray == 0) return;

    // Quit now if we've got nothing to do
    if (texArray[offset].handle)
    {
        // Release the texture from the MPR context
        delete(TextureHandle *)texArray[offset].handle;
        texArray[offset].handle = NULL;

#ifdef _DEBUG
        ActiveTextureCount--;
#endif
    }

#ifdef _DONOT_COPY_BITS

    if (texArray[offset].bits)
    {
        if ( not g_bUseMappedFiles)
#ifdef USE_SH_POOLS
            MemFreeFS(texArray[offset].bits);

#else
            delete[] texArray[offset].bits;
#endif
        texArray[offset].bits = NULL;

#ifdef _DEBUG
        LoadedTextureCount--;
#endif
    }

#endif
}

// This function will release the memory image of a texture.
// It is called when the reference count of a texture reaches zero OR
// when the texture is moved into MPR and a local copy is no longer required.
void FarTexDB::Free(DWORD offset)
{
    ShiAssert(IsReady());
    ShiAssert(offset >= 0);
    ShiAssert(offset < (DWORD) texCount);

    // Quit now if we've got nothing to do
    if (texArray[offset].bits == NULL) return;

    // Release the image memory
#ifndef _DONOT_COPY_BITS

    if ( not g_bUseMappedFiles)
#ifdef USE_SH_POOLS
        MemFreeFS(texArray[offset].bits);

#else
        delete[] texArray[offset].bits;
#endif
}
texArray[offset].bits = NULL;

#ifdef _DEBUG
LoadedTextureCount--;
#endif
#endif
}

// Select a "Load"ed texture into an RC for immediate use by the rasterizer
void FarTexDB::Select(ContextMPR *localContext, TextureID texID)
{
    ShiAssert(IsReady());
    ShiAssert(localContext);

    if (texID == INVALID_TEXID) return;

    ShiAssert(texID >= 0);
    ShiAssert(texID < (DWORD) texCount);

    // Make sure the texture we're trying to use is local to MPR
    if (texArray[texID].handle == NULL)
    {
        ShiAssert(texArray[texID].bits);
        Activate(texID);
    }

    ShiAssert(texArray[texID].handle);
    localContext->SelectTexture1(texArray[texID].handle);
}

void FarTexDB::RestoreAll()
{
    EnterCriticalSection(&cs_textureList);

    // Free the entire texture list
    for (int i = 0; i < texCount; i++)
    {
        if (texArray[i].handle)
            ((TextureHandle *)texArray[i].handle)->RestoreAll();
    }

    LeaveCriticalSection(&cs_textureList);
}

void FarTexDB::FlushHandles()
{
    if (fartexDDSFile.IsReady()) fartexDDSFile.Close();

    if (DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS)
    {
        DDSURFACEDESC2 ddsd;
        char szRawName[256];
        FILE *fp;

        // Fetch and save linear size first
        sprintf(szRawName, "%s.dds", texturePath);
        fp = fopen(szRawName, "rb");

        if ( not fp) return;

        fread(&ddsd, 1, sizeof(DDSURFACEDESC2), fp);
        ShiAssert(ddsd.dwFlags bitand DDSD_LINEARSIZE)

        linearSize = ddsd.dwLinearSize;

        fclose(fp);

        // Open and hang onto the distant texture DDS image file
        fartexDDSFile.Open(szRawName, FALSE, not g_bUseMappedFiles);
    }
}

bool FarTexDB::SyncDDSTextures(bool bForce)
{
    DDSURFACEDESC2 ddsd;
    DWORD dwMagic;
    BYTE *pBuf;
    char szRawName[256], szDDSName[256];
    bool bOnce = false;
    FILE *fpRaw, *fpDDS;

    fartexDDSFile.Close();

    sprintf(szRawName, "%s.dds", texturePath);
    fpRaw = fopen(szRawName, "rb");

    // If fartiles.dds exists, quit now
    if (fpRaw)
    {
        fclose(fpRaw);

        if ( not bForce)
            return false;
    }

    DeleteFile(szRawName);

    fpRaw = fopen(szRawName, "wb");

    CreateDirectory(texturePath, NULL);

    // Dump the entire texture list
    for (int i = 0; i < texCount; i++)
    {
        texArray[i].refCount++;
        Load(i, true);
        DumpImageToFile(i);

        sprintf(szDDSName, "%s\\%d.dds", texturePath, i);
        fpDDS = fopen(szDDSName, "rb");
        fread(&dwMagic, 1, sizeof(DWORD), fpDDS);
        fread(&ddsd, 1, sizeof(DDSURFACEDESC2), fpDDS);

        pBuf = new BYTE[ddsd.dwLinearSize];
        fread(pBuf, 1, ddsd.dwLinearSize, fpDDS);

        // One header only, all textures are the same linear size
        if ( not bOnce)
        {
            fwrite(&ddsd, 1, sizeof(ddsd), fpRaw);
            bOnce = true;
        }

        fwrite(pBuf, 1, ddsd.dwLinearSize, fpRaw);

        delete pBuf;
        fclose(fpDDS);
        DeleteFile(szDDSName);
    }

    LeaveCriticalSection(&cs_textureList);

    fclose(fpRaw);
    RemoveDirectory(texturePath);

    fartexDDSFile.Open(szRawName, FALSE, not g_bUseMappedFiles);

    return true;
}

bool FarTexDB::DumpImageToFile(DWORD offset)
{
    DWORD dwSize, *pal, dwTmp, n, i;
    BYTE *pSrc, *pDst;
    char szFileName[256];
    FILE *fp;


    ShiAssert(IsReady());
    ShiAssert(offset >= 0);
    ShiAssert(offset < (DWORD) texCount);
    ShiAssert(texArray[offset].bits);

    if ( not texArray[offset].bits) return false;

    sprintf(szFileName, "%s\\%d.dds", texturePath, offset);
    fp = fopen(szFileName, "rb");

    if ( not fp)
    {
        dwSize = IMAGE_SIZE * IMAGE_SIZE;

        pSrc = (BYTE *)texArray[offset].bits;
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

        SaveDDS_DXTn(szFileName, pDst, IMAGE_SIZE);

        delete[] pDst;
    }
    else
        fclose(fp);

    return true;
}

bool FarTexDB::SaveDDS_DXTn(const char *szFileName, BYTE* pDst, int dimensions)
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
void FarTexDB::RGBtoHSV(float r, float g, float b, float *h, float *s, float *v)
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

void FarTexDB::HSVtoRGB(float *r, float *g, float *b, float h, float s, float v)
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
    i = (int)floor(h);
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
