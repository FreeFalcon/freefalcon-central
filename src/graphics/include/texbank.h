/***************************************************************************\
    TexBank.h
    Miro "Jammer" Torrielli
    09Oct03

 - Begin Major Rewrite
\***************************************************************************/
#ifndef _TEXBANK_H_
#define _TEXBANK_H_

#include "Tex.h"
#include "PolyLib.h"
#include "../../Falclib/Include/FileMemMap.h"

extern class TextureBankClass TheTextureBank;

typedef struct TexBankEntry
{
    long   fileOffset; // How far into the .TEX file does the compressed data start?
    long   fileSize; // How big is the compressed data on disk?
    Texture tex; // The container class which manages the texture data
    Texture texN; // The container class which manages the texture data
    int palID; // The offset into ThePaletteBank to use with this texture
    int refCount; // How many objects want this texture right now
} TexBankEntry;

typedef struct TempTexBankEntry
{
    long   fileOffset; // How far into the .TEX file does the compressed data start?
    long   fileSize; // How big is the compressed data on disk?
    Texture tex; // The container class which manages the texture data
    int palID; // The offset into ThePaletteBank to use with this texture
    int refCount; // How many objects want this texture right now
} TempTexBankEntry;

typedef struct
{
    char OnOrder;
    char OnRelease;
} TexFlagsType ;

class TextureBankClass
{
public:
    TextureBankClass()
    {
        nTextures = 0;
        TexturePool = NULL;
    };
    ~TextureBankClass() {};

public:
    static int nTextures;
    static TexBankEntry *TexturePool;
    static TempTexBankEntry *TempTexturePool;

#ifdef _DEBUG
    static int textureCount;
#endif

protected:
    static char baseName[256];
    static int deferredLoadState;
    static FileMemMap TexFileMap;
    static BYTE *CompressedBuffer;
    static BYTE *TexBuffer;
    static DWORD TexBufferSize;


public:
    static void Setup(int nEntries);
    static void Cleanup(void);
    static void ReadPool(int file, char *basename);
    static void FlushHandles(void);
    static void Reference(int id);
    static void Release(int id);
    static void Select(int id);
    static void RestoreAll();
    static void SetDeferredLoad(BOOL state);
    static BOOL IsValidIndex(int id);
    static void SyncDDSTextures(bool bForce = false);
    static void RestoreTexturePool();
    static void SelectHandle(DWORD);
    static DWORD GetHandle(DWORD id);


protected:
    static void AllocCompressedBuffer(int size);
    static void FreeCompressedBuffer(void);
    static void OpenTextureFile();
    static void ReadImageData(int id, bool forceNoDDS = false);
    static void CloseTextureFile(void);
    static void LoaderCallBack(struct LoaderQ* request);
    static void UnpackPalettizedTexture(DWORD id);
    static void ReadImageDDS(DWORD id);
    static void ReadImageDDSN(DWORD id);

    // New texture management...
    static TexFlagsType *TexFlags;
    static void CreateCallBack(LoaderQ* request);
    static bool   RatedLoad; // This flag makes textures loaded once x loader frame
    static short  *CacheLoad, *CacheRelease, LoadIn, LoadOut, ReleaseIn, ReleaseOut;

public:
    static bool UpdateBank(void);
    static void WaitUpdates(void);
    static void ReferenceTexSet(DWORD *TexList, DWORD Nr);
    static void ReleaseTexSet(DWORD *TexList, DWORD Nr);
    static void SetRatedLoad(bool Status)
    {
        RatedLoad = false;
    }
};

#define TEX_ON_ORDER 0x00000001
#define TEX_ON_RELEASE 0x00000002
#define CACHE_MARGIN 32

#endif // _TEXBANK_H_
