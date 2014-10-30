/***************************************************************************\
    Context.h
    Miro "Jammer" Torrielli
    10Oct03

 - Begin Major Rewrite
\***************************************************************************/
#include <cISO646>
#include "stdafx.h"
#include <io.h>
#include <fcntl.h>
#include "Utils/lzss.h"
#include "Loader.h"
#include "grinline.h"
#include "StateStack.h"
#include "ObjectLOD.h"
#include "PalBank.h"
#include "TexBank.h"
#include "Image.h"
#include "TerrTex.h"
#include "FalcLib/include/playerop.h"
#include "FalcLib/include/dispopts.h"

// Static data members (used to avoid requiring "this" to be passed to every access function)
int TextureBankClass::nTextures = 0;
TexBankEntry* TextureBankClass::TexturePool = NULL;
TempTexBankEntry* TextureBankClass::TempTexturePool = NULL;
BYTE* TextureBankClass::CompressedBuffer = NULL;
int TextureBankClass::deferredLoadState = 0;
char TextureBankClass::baseName[256];
FileMemMap TextureBankClass::TexFileMap;
#ifdef _DEBUG
int TextureBankClass::textureCount = 0;
#endif
TexFlagsType *TextureBankClass::TexFlags;
BYTE* TextureBankClass::TexBuffer;
DWORD TextureBankClass::TexBufferSize;
bool TextureBankClass::RatedLoad;
short *TextureBankClass::CacheLoad, *TextureBankClass::CacheRelease;
short TextureBankClass::LoadIn, TextureBankClass::LoadOut, TextureBankClass::ReleaseIn, TextureBankClass::ReleaseOut;

DWORD gDebugTextureID;

#ifdef USE_SH_POOLS
extern MEM_POOL gBSPLibMemPool;
#endif

extern bool g_bUseMappedFiles;
int nVer; // Header version


void TextureBankClass::Setup(int nEntries)
{
    // Create our array of texture headers
    nTextures = nEntries;

    if (nEntries)
    {
#ifdef USE_SH_POOLS
        TexturePool = (TexBankEntry *)MemAllocPtr(gBSPLibMemPool, sizeof(TexBankEntry) * nEntries, 0);
        TempTexturePool = (TempTexBankEntry *)MemAllocPtr(gBSPLibMemPool, sizeof(TempTexBankEntry) * nEntries, 0);
#else
        TexturePool = new TexBankEntry[nEntries];
        TempTexturePool = new TempTexBankEntry[nEntries];
#endif

        TexFlags = (TexFlagsType*) malloc(nTextures * sizeof(TexFlagsType));
        memset(TexFlags, 0, nTextures *  sizeof(TexFlagsType));
        // Allocte acche with a little safety margin
        CacheLoad = (short*) malloc(sizeof(short) * (nTextures + CACHE_MARGIN));
        CacheRelease = (short*) malloc(sizeof(short) * (nTextures + CACHE_MARGIN));
    }
    else
    {
        TexturePool = NULL;
        TempTexturePool = NULL;
        TexFlags = NULL;
        TexBuffer = NULL;
        CacheLoad = NULL;
        CacheRelease = NULL;
    }

    RatedLoad = true;
    LoadIn = LoadOut = ReleaseIn = ReleaseOut = 0;
}

void TextureBankClass::Cleanup(void)
{
    // wait for any update on textures
    WaitUpdates();
    // Clean up our array of texture headers
#ifdef USE_SH_POOLS
    MemFreePtr(TexturePool);
    MemFreePtr(TempTexturePool);
#else
    delete[] TexturePool;
    delete[] TempTexturePool;
#endif
    TexturePool = NULL;
    TempTexturePool = NULL;
    nTextures = 0;

    if (TexFlags) free(TexFlags), TexFlags = NULL;

    // Clean up the decompression buffer
#ifdef USE_SH_POOLS
    MemFreePtr(CompressedBuffer);
#else
    delete[] CompressedBuffer;
#endif
    CompressedBuffer = NULL;

    RatedLoad = false;

    if (CacheLoad)  free(CacheLoad), CacheLoad = NULL;

    if (CacheRelease)  free(CacheRelease), CacheRelease = NULL;

    LoadIn = LoadOut = ReleaseIn = ReleaseOut = 0;

    // Close our texture resource file
    if (TexFileMap.IsReady())
    {
        CloseTextureFile();
    }
}

void TextureBankClass::ReadPool(int file, char *basename)
{
    int result;
    int maxCompressedSize;

    ZeroMemory(baseName, sizeof(baseName));
    sprintf(baseName, "%s", basename);

    // Read the number of textures in the pool
    result = read(file, &nTextures, sizeof(nTextures));

    if (nTextures == 0) return;

    // Read the size of the biggest compressed texture in the pool
    result = read(file, &maxCompressedSize, sizeof(maxCompressedSize));

    // HACK - KO.dxh version - Note maxCompressedSize is used for the old BMP
    // textures, not DDS textures.
    nVer = maxCompressedSize;

#ifdef USE_SH_POOLS
    CompressedBuffer = (BYTE *)MemAllocPtr(gBSPLibMemPool, sizeof(BYTE) * maxCompressedSize, 0);
#else
    CompressedBuffer = new BYTE[maxCompressedSize];
#endif
    ShiAssert(CompressedBuffer);

    if (CompressedBuffer)
    {
        ZeroMemory(CompressedBuffer, maxCompressedSize);
    }

    // Setup the pool
    Setup(nTextures);

    // sfr: Ok some ????? did this and now we cannot change Texture class a bit
    // since this stop working
    result = read(file, TempTexturePool, sizeof(*TempTexturePool) * nTextures);

    if (result < 0)
    {
        char message[256];
        sprintf(message, "Reading object texture bank: %s", strerror(errno));
        ShiError(message);
    }

    for (int i = 0; i < nTextures; i++)
    {
        TexturePool[i].fileOffset = TempTexturePool[i].fileOffset;
        TexturePool[i].fileSize = TempTexturePool[i].fileSize;

        TexturePool[i].tex = TempTexturePool[i].tex;
        TexturePool[i].tex.flags or_eq MPR_TI_INVALID;
        TexturePool[i].texN = TempTexturePool[i].tex;
        TexturePool[i].texN.flags or_eq MPR_TI_INVALID;

        if (DisplayOptions.bMipmapping)
        {
            TexturePool[i].tex.flags or_eq MPR_TI_MIPMAP;
            TexturePool[i].texN.flags or_eq MPR_TI_MIPMAP;
        }

        TexturePool[i].palID = 0;//TempTexturePool[i].palID;
        TexturePool[i].refCount = 0;//TempTexturePool[i].refCount;
    }



    OpenTextureFile();
}

void TextureBankClass::FreeCompressedBuffer()
{
#ifdef USE_SH_POOLS
    MemFreePtr(CompressedBuffer);
#else
    delete[] CompressedBuffer;
#endif

    CompressedBuffer = NULL;
}

void TextureBankClass::AllocCompressedBuffer(int maxCompressedSize)
{
#ifdef USE_SH_POOLS
    CompressedBuffer = (BYTE *)MemAllocPtr(gBSPLibMemPool, sizeof(BYTE) * maxCompressedSize, 0);
#else
    CompressedBuffer = new BYTE[maxCompressedSize];
#endif

    ShiAssert(CompressedBuffer);

    if (CompressedBuffer)
    {
        ZeroMemory(CompressedBuffer, maxCompressedSize);
    }
}

void TextureBankClass::OpenTextureFile()
{
    char filename[_MAX_PATH];

    ShiAssert( not TexFileMap.IsReady());

    strcpy(filename, baseName);
    strcat(filename, ".TEX");

    if ( not TexFileMap.Open(filename, FALSE, not g_bUseMappedFiles))
    {
        char message[256];
        sprintf(message, "Failed to open object texture file %s\n", filename);
        ShiError(message);
    }
}

void TextureBankClass::CloseTextureFile(void)
{
    TexFileMap.Close();
}

void TextureBankClass::Reference(int id)
{
    int  isLoaded;

    gDebugTextureID = id;

    ShiAssert(IsValidIndex(id));

    // Get our reference to this texture recorded to ensure it doesn't disappear out from under us
    //EnterCriticalSection(&ObjectLOD::cs_ObjectLOD);

    isLoaded = TexturePool[id].refCount;
    ShiAssert(isLoaded >= 0);
    TexturePool[id].refCount++;

    // If we already have the data, just verify that fact. Otherwise, load it.
    if (isLoaded)
    {
        ;
    }
    else
    {
        ShiAssert(TexFileMap.IsReady());
        ShiAssert(CompressedBuffer);
        if(TexturePool[id].tex.imageData not_eq NULL)
            return;
        ShiAssert(TexturePool[id].tex.TexHandle() == NULL);

        // Get the palette pointer
        // would be great if we could set a flag saying this palette comes from bank...
        // but since we cannot add anything to texture structure (because Jammer read them
        // directly from file instead of from a method) I make the check when releasing
        // the palette.
        TexturePool[id].tex.SetPalette(&ThePaletteBank.PalettePool[TexturePool[id].palID]);
        ShiAssert(TexturePool[id].tex.GetPalette());
        TexturePool[id].tex.GetPalette()->Reference();

        // Mark for the request if not already marked
        if ( not TexFlags[id].OnOrder)
        {
            TexFlags[id].OnOrder = true;
            // put into load cache
            CacheLoad[LoadIn++] = id;

            // Ring the pointer
            if (LoadIn >= (nTextures + CACHE_MARGIN)) LoadIn = 0;

            // Kick the Loader
            TheLoader.WakeUp();

        }

    }

    gDebugTextureID = -1;

}

// Calls to this func are enclosed in the critical section cs_ObjectLOD by ObjectLOD::Unload()
void TextureBankClass::Release(int id)
{
    ShiAssert(IsValidIndex(id));
    ShiAssert(TexturePool[id].refCount > 0);

    // RED - no reference, no party... 
    if ( not TexturePool[id].refCount) 
        return;

    TexturePool[id].refCount--;

    if (TexturePool[id].refCount == 0)
    {
        if ( not TexFlags[id].OnRelease)
        {
            TexFlags[id].OnRelease = true;
            // put into load cache
            CacheRelease[ReleaseIn++] = id;

            // Ring the pointer
            if (ReleaseIn >= (nTextures + CACHE_MARGIN)) ReleaseIn = 0;

            // Kick the Loader
            TheLoader.WakeUp();
        }
    }
}

void TextureBankClass::ReadImageData(int id, bool forceNoDDS)
{
    int retval;
    int size;
    BYTE *cdata;
    //sfr: added for more control
    int cdataSize;


    ShiAssert(TexturePool[id].refCount);

    if ( not forceNoDDS and DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS)
    {
        ReadImageDDS(id);
        ReadImageDDSN(id);
        return;
    }

    if (g_bUseMappedFiles)
    {
        cdata = TexFileMap.GetData(TexturePool[id].fileOffset, TexturePool[id].fileSize);
        cdataSize = TexturePool[id].fileSize - TexturePool[id].fileOffset;
        ShiAssert(cdata);
    }
    else
    {
        if ( not TexFileMap.ReadDataAt(TexturePool[id].fileOffset, CompressedBuffer, TexturePool[id].fileSize))
        {
            char message[120];
            sprintf(message, "%s: Bad object texture seek (%0d)", strerror(errno), TexturePool[id].fileOffset);
            ShiError(message);
        }

        cdata = CompressedBuffer;
        //sfr: in this case, im not sure size is this
        // FRB - fileOffset - fileOffset ????
        //cdataSize = TexturePool[id].fileOffset - TexturePool[id].fileOffset;
        cdataSize = TexturePool[id].fileSize - TexturePool[id].fileOffset;
    }

    // Allocate memory for the new texture
    size = TexturePool[id].tex.dimensions;
    size = size * size;

    TexturePool[id].tex.imageData = glAllocateMemory(size, FALSE);
    ShiAssert(TexturePool[id].tex.imageData);

    // Uncompress the data into the texture structure
    //sfr: using new cdataSize for control
    retval = LZSS_Expand(cdata, cdataSize, (BYTE*)TexturePool[id].tex.imageData, size);
    ShiAssert(retval == TexturePool[id].fileSize);

#ifdef _DEBUG
    textureCount++;
#endif
}

void TextureBankClass::SetDeferredLoad(BOOL state)
{
    LoaderQ *request;

    // Allocate space for the async request
    request = new LoaderQ;

    if ( not request)
        ShiError("Failed to allocate memory for a object texture load state change request");

    // Build the data transfer request to get the required object data
    request->filename = NULL;
    request->fileoffset = 0;
    request->callback = LoaderCallBack;
    request->parameter = (void*)state;

    // Submit the request to the asynchronous loader
    TheLoader.EnqueueRequest(request);
}

void TextureBankClass::LoaderCallBack(LoaderQ* request)
{
    BOOL state = (int)request->parameter;

    //EnterCriticalSection(&ObjectLOD::cs_ObjectLOD);

    // If we're turning deferred loads off, go back and do all the loads we held up
    if (deferredLoadState and not state)
    {
        DWORD Count = 5;

        // Check each texture
        for (int id = 0; id < nTextures; id++)
        {
            // See if it is in use
            if (TexturePool[id].refCount)

                // This one is in use. Is it already loaded?
                if (/* not TexturePool[id].tex.imageData and */ not TexturePool[id].tex.TexHandle())
                {

                    // Nope, go get it.
                    if ( not TexturePool[id].tex.imageData) ReadImageData(id);

                    TexturePool[id].tex.CreateTexture();
                    Count--;
                    //TexturePool[id].tex.FreeImage();
                }

            if ( not Count) break;
        }
    }

    // Now store the new state
    deferredLoadState = state;

    //LeaveCriticalSection(&ObjectLOD::cs_ObjectLOD);

    // Free the request queue entry
    delete request;
}

void TextureBankClass::FlushHandles(void)
{
    int id;

    for (id = 0; id < nTextures; id++)
    {
        ShiAssert(TexturePool[id].refCount == 0);

        while (TexturePool[id].refCount > 0)
        {
            Release(id);
        }
    }

    WaitUpdates();
}

void TextureBankClass::Select(int id)
{
}


void TextureBankClass::SelectHandle(DWORD TexHandle)
{
    TheStateStack.context->SelectTexture1(TexHandle);
}


BOOL TextureBankClass::IsValidIndex(int id)
{
    return((id >= 0) and (id < nTextures));
}

void TextureBankClass::RestoreAll()
{
}

void TextureBankClass::SyncDDSTextures(bool bForce)
{
    char szFile[256];
    FILE *fp;

    ShiAssert(TexturePool);

    CreateDirectory(baseName, NULL);

    for (DWORD id = 0; id < (DWORD)nTextures; id++)
    {
        sprintf(szFile, "%s\\%d.dds", baseName, id);
        fp = fopen(szFile, "rb");

        if ( not fp or bForce)
        {
            if (fp)
                fclose(fp);

            UnpackPalettizedTexture(id);
        }
        else
            fclose(fp);

        TexturePool[id].tex.flags or_eq MPR_TI_DDS;
        TexturePool[id].tex.flags and_eq compl MPR_TI_PALETTE;
        TexturePool[id].texN.flags or_eq MPR_TI_DDS;
        TexturePool[id].texN.flags and_eq compl MPR_TI_PALETTE;
    }
}

void TextureBankClass::UnpackPalettizedTexture(DWORD id)
{
    char szFile[256];

    CreateDirectory(baseName, NULL);

    if (TexturePool[id].tex.dimensions > 0)
    {
        //sfr: (see my comment regarding palette origin above)
        TexturePool[id].tex.SetPalette(&ThePaletteBank.PalettePool[TexturePool[id].palID]);
        ShiAssert(TexturePool[id].tex.GetPalette());
        TexturePool[id].tex.GetPalette()->Reference();

        ReadImageData(id, true);
        sprintf(szFile, "%s\\%d", baseName, id);
        TexturePool[id].tex.DumpImageToFile(szFile, TexturePool[id].palID);
        Release(id);
    }
    else
    {
        sprintf(szFile, "%s\\%d.dds", baseName, id);
        FILE* fp = fopen(szFile, "wb");
        fclose(fp);
    }
}



void TextureBankClass::ReadImageDDS(DWORD id)
{
    DDSURFACEDESC2 ddsd;
    DWORD dwSize, dwMagic;
    char szFile[256];
    FILE *fp;

    TexturePool[id].tex.flags = MPR_TI_DDS;
    TexturePool[id].tex.flags and_eq compl MPR_TI_PALETTE;

    sprintf(szFile, "%s\\%d.dds", baseName, id);
    fp = fopen(szFile, "rb");

    // RV - RED - Avoid CTD if a missing texture
    if ( not fp) return;

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

    switch (ddsd.ddpfPixelFormat.dwFourCC)
    {
        case MAKEFOURCC('D', 'X', 'T', '1'):
            TexturePool[id].tex.flags or_eq MPR_TI_DXT1;
            break;

        case MAKEFOURCC('D', 'X', 'T', '3'):
            TexturePool[id].tex.flags or_eq MPR_TI_DXT3;
            break;

        case MAKEFOURCC('D', 'X', 'T', '5'):
            TexturePool[id].tex.flags or_eq MPR_TI_DXT5;
            break;

        default:
            ShiAssert(false);
    }

    switch (ddsd.dwWidth)
    {
        case 16:
            TexturePool[id].tex.flags or_eq MPR_TI_16;
            break;

        case 32:
            TexturePool[id].tex.flags or_eq MPR_TI_32;
            break;

        case 64:
            TexturePool[id].tex.flags or_eq MPR_TI_64;
            break;

        case 128:
            TexturePool[id].tex.flags or_eq MPR_TI_128;
            break;

        case 256:
            TexturePool[id].tex.flags or_eq MPR_TI_256;
            break;

        case 512:
            TexturePool[id].tex.flags or_eq MPR_TI_512;
            break;

        case 1024:
            TexturePool[id].tex.flags or_eq MPR_TI_1024;
            break;

        case 2048:
            TexturePool[id].tex.flags or_eq MPR_TI_2048;
            break;

        default:
            ShiAssert(false);
    }

    dwSize = ddsd.dwLinearSize;
    TexturePool[id].tex.imageData = (BYTE *)glAllocateMemory(dwSize, FALSE);
    fread(TexturePool[id].tex.imageData, 1, dwSize, fp);

    TexturePool[id].tex.dimensions = dwSize;

    fclose(fp);

#ifdef _DEBUG
    textureCount++;
#endif
}

void TextureBankClass::ReadImageDDSN(DWORD id)
{
    DDSURFACEDESC2 ddsd;
    DWORD dwSize, dwMagic;
    char szFile[256];
    FILE *fp;

    sprintf(szFile, "%s\\%dN.dds", baseName, id);
    fp = fopen(szFile, "rb");

    if ( not fp)
    {
        return;
    }

    TexturePool[id].texN.flags or_eq MPR_TI_DDS;
    TexturePool[id].texN.flags and_eq compl MPR_TI_PALETTE;
    TexturePool[id].texN.flags and_eq compl MPR_TI_INVALID;

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

    switch (ddsd.ddpfPixelFormat.dwFourCC)
    {
        case MAKEFOURCC('D', 'X', 'T', '1'):
            TexturePool[id].texN.flags or_eq MPR_TI_DXT1;
            break;

        case MAKEFOURCC('D', 'X', 'T', '3'):
            TexturePool[id].tex.flags or_eq MPR_TI_DXT3;
            break;

        case MAKEFOURCC('D', 'X', 'T', '5'):
            TexturePool[id].texN.flags or_eq MPR_TI_DXT5;
            break;

        default:
            ShiAssert(false);
    }

    switch (ddsd.dwWidth)
    {
        case 16:
            TexturePool[id].texN.flags or_eq MPR_TI_16;
            break;

        case 32:
            TexturePool[id].texN.flags or_eq MPR_TI_32;
            break;

        case 64:
            TexturePool[id].texN.flags or_eq MPR_TI_64;
            break;

        case 128:
            TexturePool[id].texN.flags or_eq MPR_TI_128;
            break;

        case 256:
            TexturePool[id].texN.flags or_eq MPR_TI_256;
            break;

        case 512:
            TexturePool[id].texN.flags or_eq MPR_TI_512;
            break;

        case 1024:
            TexturePool[id].texN.flags or_eq MPR_TI_1024;
            break;

        case 2048:
            TexturePool[id].texN.flags or_eq MPR_TI_2048;
            break;

        default:
            ShiAssert(false);
    }

    dwSize = ddsd.dwLinearSize;
    TexturePool[id].texN.imageData = (BYTE *)glAllocateMemory(dwSize, FALSE);
    fread(TexturePool[id].texN.imageData, 1, dwSize, fp);

    TexturePool[id].texN.dimensions = dwSize;

    fclose(fp);

#ifdef _DEBUG
    textureCount++;
#endif
}

void TextureBankClass::RestoreTexturePool()
{
    for (int i = 0; i < nTextures; i++)
    {
        TexturePool[i].fileOffset = TempTexturePool[i].fileOffset;
        TexturePool[i].fileSize = TempTexturePool[i].fileSize;
        TexturePool[i].tex = TempTexturePool[i].tex;
        TexturePool[i].texN = TempTexturePool[i].tex;
        TexturePool[i].palID = TempTexturePool[i].palID;
        TexturePool[i].refCount = TempTexturePool[i].refCount;
    }
}



DWORD TextureBankClass::GetHandle(DWORD id)
{
    // if already on release, avoid using or requesting it
    if (TexFlags[id].OnRelease) return NULL;

    // if the Handle is prsent, return it
    if (IsValidIndex(id) and TexturePool[id].tex.TexHandle()) return TexturePool[id].tex.TexHandle();

    // return  a null pointer that means BLANK SURFACE
    return NULL;
}



// RED - This function manages to load and create requested textures
bool TextureBankClass::UpdateBank(void)
{
    DWORD id;

    // till when data to update into caches
    while (LoadIn not_eq LoadOut or ReleaseIn not_eq ReleaseOut)
    {

        // check for textures to be released
        if (ReleaseIn not_eq ReleaseOut)
        {
            // get the 1st texture Id from cache
            id = CacheRelease[ReleaseOut++];

            // if not an order again, and no Referenced, release it
            if ( not TexFlags[id].OnOrder and not TexturePool[id].refCount and TexFlags[id].OnRelease) TexturePool[id].tex.FreeAll();

            // clear flag, in any case
            TexFlags[id].OnRelease = false;

            // ring the pointer
            if (ReleaseOut >= (nTextures + CACHE_MARGIN)) ReleaseOut = 0;

            // if any action, terminate here
            if (RatedLoad) return true;
        }

        // check for textures to be released
        if (LoadIn not_eq LoadOut)
        {
            // get the 1st texture Id from cache
            id = CacheLoad[LoadOut++];

            // if Texture not yet loaded, load it
            if ( not TexturePool[id].tex.imageData) ReadImageData(id);

            // if Texture not yet crated, crate it
            if ( not TexturePool[id].tex.TexHandle()) TexturePool[id].tex.CreateTexture();

            // clear flag, in any case
            TexFlags[id].OnOrder = false;

            // ring the pointer
            if (LoadOut >= (nTextures + CACHE_MARGIN)) LoadOut = 0;

            // if any action, terminate here
            if (RatedLoad) return true;
        }

    }

    // if here, nothing done, back is up to date
    return false;
}


void TextureBankClass::WaitUpdates(void)
{
    // if no data to wait, exit here
    if (LoadIn == LoadOut and ReleaseIn == ReleaseOut) return;

    // Pause the Loader...
    TheLoader.SetPause(true);

    while ( not TheLoader.Paused());

    // Not slow loading
    RatedLoad = false;

    // Parse all objects till any opration to do
    while (UpdateBank());

    // Restore rated loading
    RatedLoad = true;
    // Run the Loader again
    TheLoader.SetPause(false);
}


void TextureBankClass::CreateCallBack(LoaderQ* request)
{
}


void TextureBankClass::ReferenceTexSet(DWORD *TexList, DWORD Nr)
{
    while (Nr--) 
    {
        Reference(*TexList);
        TexList++;
    }
}

void TextureBankClass::ReleaseTexSet(DWORD *TexList, DWORD Nr)
{
    while (Nr--) 
    {
        Release(*TexList);
        TexList++;
    }
}
