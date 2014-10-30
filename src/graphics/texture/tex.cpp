/***************************************************************************
    Tex.cpp
    Miro "Jammer" Torrielli
    10Oct03

 - Begin Major Rewrite
***************************************************************************/
#include "stdafx.h"
#include "Image.h"
#include "Tex.h"
#include "dxtlib.h"
#include "PalBank.h"
#include "FalcLib/include/playerop.h"
#include "FalcLib/include/dispopts.h"

#ifdef USE_SH_POOLS
MEM_POOL Palette::pool;
#endif

static char TexturePath[256] = {'\0'};
static DXContext *rc = NULL;

extern bool g_bEnableNonPersistentTextures;
extern bool g_bShowMipUsage;
extern int fileout;
extern void ConvertToNormalMap(int kerneltype, int colorcnv, int alpha, float scale, int minz, bool wrap, bool bInvertX, bool bInvertY, int w, int h, int bits, void * data);
extern void ReadDTXnFile(unsigned long count, void * buffer);
extern void WriteDTXnFile(unsigned long count, void *buffer);

#define ARGB_TEXEL_SIZE 4
#define ARGB_TEXEL_BITS 32

static HRESULT WINAPI MipLoadCallback(LPDIRECTDRAWSURFACE7 lpDDSurface, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext);

struct MipLoadContext
{
    int nLevel;
    LPDIRECTDRAWSURFACE7 lpDDSurface;
};

void SetMipLevelColor(MipLoadContext *pCtx);

static char *arrSurfFmt2String[] =
{
    "UNKNOWN",
    "R8G8B8",
    "A8R8G8B8",
    "X8R8G8B8",
    "R5G6B5",
    "R5G5B5",
    "PALETTE4",
    "PALETTE8",
    "A1R5G5B5",
    "X4R4G4B4",
    "A4R4G4B4",
    "L8",
    "A8L8",
    "U8V8",
    "U5V5L6",
    "U8V8L8",
    "UYVY",
    "YUY2",
    "DXT1",
    "DXT3",
    "DXT5",
    "R3G3B2",
    "A8",
    "TEXTUREMAX",

    "Z16S0",
    "Z32S0",
    "Z15S1",
    "Z24S8",
    "S1Z15",
    "S8Z24",

    "<<<Index our of range>>>",
    "<<<Index our of range>>>",
    "<<<Index our of range>>>",
    "<<<Index our of range>>>",
    "<<<Index our of range>>>",
    "<<<Index our of range>>>",
    "<<<Index our of range>>>",
    "<<<Index our of range>>>",
    "<<<Index our of range>>>",
    "<<<Index our of range>>>",
    "<<<Index our of range>>>",
};

static int FindMsb(DWORD val)
{
    int i = 0;

    for (i = 0; i < 32; i++)
    {
        if (val bitand (1 << (31 - i)))
        {
            break;
        }
    }

    // pmvstrm - VS2010 fix
    int result = 31 + i;

    // pmvstrm original was
    //result = 31 +i;

    return result;
}

#ifdef _DEBUG
DWORD Texture::m_dwNumHandles = 0; // Number of instances
DWORD Texture::m_dwBitmapBytes = 0; // Bytes allocated for bitmap copies
DWORD Texture::m_dwTotalBytes = 0; // Total number of bytes allocated (including bitmap copies and object size)
#endif

Texture::Texture()
{
    texHandle = NULL;
    imageData = NULL;
    palette = NULL;
    flags = 0;
    //sfr: added palette control
    //paletteFromBank = thFromBank = false;

#ifdef _DEBUG
    InterlockedIncrement((long *)&m_dwNumHandles); // Number of instances
    InterlockedExchangeAdd((long *)&m_dwTotalBytes, sizeof(*this));
#endif
};

Texture::~Texture()
{
#ifdef _DEBUG
    //InterlockedIncrement((long *)&m_dwNumHandles); // Number of instances
    //InterlockedExchangeAdd((long *)&m_dwTotalBytes,-sizeof(*this));
#endif
    if((texHandle not_eq NULL) or (imageData not_eq NULL))
    {
        FreeAll();
    }
};

/* Store some useful global information.  The path is used for all
   texture loads through this interface and the RC is used for loading.
   This means that at present, only one device at a time can load textures
   through this interface.*/
void Texture::SetupForDevice(DXContext *texRC, char *path)
{
    // Store the texture path for future reference
    if (strlen(path) + 1 >= sizeof(TexturePath))
    {
        ShiError("Texture path name overflow");
    }

    strcpy(TexturePath, path);

    if (TexturePath[strlen(TexturePath) - 1] not_eq '\\')
    {
        strcat(TexturePath, "\\");
    }

    rc = texRC;
    Palette::SetupForDevice(texRC);

    TextureHandle::StaticInit(texRC->m_pD3DD);
}

// This is called when we're done working with a given device (as represented by an RC).
void Texture::CleanupForDevice(DXContext *texRC)
{
    Palette::CleanupForDevice(texRC);
    rc = NULL;

    TextureHandle::StaticCleanup();
}

// This is called to check whether the device is setup.
bool Texture::IsSetup()
{
    return rc not_eq NULL;
}

// Read a data file and store its information.
BOOL Texture::LoadImage(char *filename, DWORD newFlags, BOOL addDefaultPath)
{
    char fullname[MAX_PATH];
    CImageFileMemory  texFile;
    int result;


    ShiAssert(filename);
    ShiAssert(imageData == NULL);

    flags or_eq newFlags;

    if (addDefaultPath)
    {
        strcpy(fullname, TexturePath);
        strcat(fullname, filename);
    }
    else
    {
        strcpy(fullname, filename);
    }

    texFile.imageType = CheckImageType(fullname);
    ShiAssert(texFile.imageType not_eq IMAGE_TYPE_UNKNOWN);

    if (texFile.imageType == IMAGE_TYPE_APL)
    {
        flags or_eq MPR_TI_ALPHA;
    }

    result = texFile.glOpenFileMem(fullname);
    ShiAssert(result == 1)

    // Note that ReadTextureImage will close texFile for us
    texFile.glReadFileMem();
    result = ReadTextureImage(&texFile);
    ShiAssert(result == GOOD_READ)

    // We only support square textures
    ShiAssert(texFile.image.width == texFile.image.height)
    dimensions = texFile.image.width;
    ShiAssert(dimensions <= 2048);

    if (texFile.image.palette)
    {
        chromaKey = texFile.image.palette[0];
    }
    else
    {
        // Default to blue chroma key color
        chromaKey = 0xFFFF0000;
    }

    imageData = texFile.image.image;

    if ((flags bitand MPR_TI_DDS) == 0)
    {
        ShiAssert(texFile.image.palette);

        if (palette == NULL)
        {
            palette = new Palette();
            //paletteFromBank = false;
            palette->Setup32((DWORD *)texFile.image.palette);
        }

        else
        {
            palette->Reference();
        }

        // Release the image's palette data now that we've got our own copy
        glReleaseMemory(texFile.image.palette);
    }
    else
    {
        DDSURFACEDESC2 ddsd = texFile.image.ddsd;
        ShiAssert(ddsd.dwFlags bitand DDSD_LINEARSIZE);

        switch (ddsd.ddpfPixelFormat.dwFourCC)
        {
            case MAKEFOURCC('D', 'X', 'T', '1'):
                flags or_eq MPR_TI_DXT1;
                break;

            case MAKEFOURCC('D', 'X', 'T', '3'):
                flags or_eq MPR_TI_DXT3;
                break;

            case MAKEFOURCC('D', 'X', 'T', '5'):
                flags or_eq MPR_TI_DXT5;
                break;

            default:
                ShiAssert(false);
        }

        switch (ddsd.dwWidth)
        {
            case 16:
                flags or_eq MPR_TI_16;
                break;

            case 32:
                flags or_eq MPR_TI_32;
                break;

            case 64:
                flags or_eq MPR_TI_64;
                break;

            case 128:
                flags or_eq MPR_TI_128;
                break;

            case 256:
                flags or_eq MPR_TI_256;
                break;

            case 512:
                flags or_eq MPR_TI_512;
                break;

            case 1024:
                flags or_eq MPR_TI_1024;
                break;

            case 2048:
                flags or_eq MPR_TI_2048;
                break;

            default:
                ShiAssert(false);
        }

        palette = NULL;
        dimensions = ddsd.dwLinearSize;
    }

#ifdef _DEBUG
    InterlockedExchangeAdd((long *)&m_dwTotalBytes, dimensions * dimensions);
#endif

    return TRUE;
}

//sfr: moved here
void Texture::FreeAll()
{
    FreeTexture();
    FreeImage();
    //FreePalette();
};

// Free the image data (but NOT the texture or palette).
void Texture::FreeImage()
{
    if (imageData)
    {
#ifdef _DEBUG
        InterlockedExchangeAdd((long *)&m_dwTotalBytes, -(dimensions * dimensions));
#endif

        glReleaseMemory(imageData);
        imageData = NULL;
    }

    //FIXME - WTF is going on here?
    if (texHandle == NULL)
    {
        FreePalette();
    }
}

// Using image (and optional palette data) already loaded, create an MPR texture.
bool Texture::CreateTexture(char *strName)
{
    ShiAssert(rc not_eq NULL);
    ShiAssert(imageData);
    ShiAssert(texHandle == NULL);

    // JB 010318 CTD
    if (/* not F4IsBadReadPtr(palette,sizeof(Palette)) and */ (flags bitand MPR_TI_PALETTE))
    {
        palette->Activate();
        ShiAssert(palette->palHandle);

        // JB 010616
        if (palette->palHandle == NULL)
        {
            return false;
        }

        texHandle = new TextureHandle();
        ShiAssert(texHandle);

        palette->palHandle->AttachToTexture(texHandle);
        texHandle->Create(strName, (WORD)flags, 8, static_cast<UInt16>(dimensions), static_cast<UInt16>(dimensions));

        // OW: Prevent a crash
        if (imageData not_eq NULL)
        {
            if ( not texHandle->Load(0, chromaKey, (BYTE *)imageData))
            {
                return false;
            }
        }

        return true;
    }
    else
    {
        int width = 0;

        if (flags bitand MPR_TI_16)
            width = 16;
        else if (flags bitand MPR_TI_32)
            width = 32;
        else if (flags bitand MPR_TI_64)
            width = 64;
        else if (flags bitand MPR_TI_128)
            width = 128;
        else if (flags bitand MPR_TI_256)
            width = 256;
        else if (flags bitand MPR_TI_512)
            width = 512;
        else if (flags bitand MPR_TI_1024)
            width = 1024;
        else if (flags bitand MPR_TI_2048)
            width = 2048;

        texHandle = new TextureHandle();
        texHandle->Create(strName, flags, 32, static_cast<UInt16>(width), static_cast<UInt16>(width));
        return texHandle->Load(0, 0, (BYTE*)imageData, false, false, dimensions);
    }
}

// Release the MPR texture we're holding.
void Texture::FreeTexture()
{
    if (texHandle not_eq NULL)
    {
        delete texHandle;
        texHandle = NULL;
    }

    // We're totally gone, so get rid of our palette if we had one
    if (imageData == NULL)
    {
        FreePalette();
    }
}

BOOL Texture::LoadAndCreate(char *filename, DWORD newFlags)
{
    if (LoadImage(filename, newFlags))
    {
        CreateTexture(filename);
        return TRUE;
    }

    return FALSE;
}

// Release the MPR palette and palette data we're holding.
void Texture::FreePalette()
{
    if ((palette not_eq NULL) and (palette->Release() == 0))
    {
        // sfr: added palette check and corectedx the > to >=
        if (
            (palette < &ThePaletteBank.PalettePool[0]) or
            (palette >= &ThePaletteBank.PalettePool[ThePaletteBank.nPalettes])
        )
        {
            // we cannot delete palettes that come from bank
            delete palette;
        }
    }

    palette = NULL;
}

// Reload the MPR texels with the ones we have stored locally.
bool Texture::UpdateMPR(char *strName)
{
    ShiAssert(rc not_eq NULL);
    ShiAssert(imageData);
    ShiAssert(texHandle);

    if ( not texHandle or not imageData)
    {
        return false;
    }

    return texHandle->Load(0, chromaKey, (BYTE *)imageData);
}

// OW
void Texture::RestoreAll()
{
    if (texHandle)
    {
        texHandle->RestoreAll();
    }
}

#ifdef _DEBUG
void Texture::MemoryUsageReport()
{
}
#endif


// TextureHandle
struct _DDPIXELFORMAT TextureHandle::m_arrPF[TEX_CAT_MAX];
IDirect3DDevice7 *TextureHandle::m_pD3DD = NULL; // Warning: Not addref'd
struct _D3DDeviceDesc7 *TextureHandle::m_pD3DHWDeviceDesc = NULL;

#ifdef _DEBUG
DWORD TextureHandle::m_dwNumHandles = 0; // Number of instances
DWORD TextureHandle::m_dwBitmapBytes = 0; // Bytes allocated for bitmap copies
DWORD TextureHandle::m_dwTotalBytes = 0; // Total number of bytes allocated (including bitmap copies and object size)
#endif

TextureHandle::TextureHandle()
{
    m_pDDS = NULL;
    m_eSurfFmt = D3DX_SF_UNKNOWN;
    m_nWidth = 0;
    m_nHeight = 0;
    m_dwFlags = NULL;
    m_dwChromaKey = NULL;
    m_pPalAttach = NULL;
    m_pImageData = NULL;
    m_nImageDataStride = -1;

#ifdef _DEBUG
    InterlockedIncrement((long *)&m_dwNumHandles); // Number of instances
    InterlockedExchangeAdd((long *)&m_dwTotalBytes, sizeof(*this));
#endif
}

TextureHandle::~TextureHandle()
{
#ifdef _DEBUG
    InterlockedDecrement((long *)&m_dwNumHandles); // Number of instances
    //InterlockedExchangeAdd((long *)&m_dwTotalBytes,-sizeof(*this));
    //InterlockedExchangeAdd((long *)&m_dwTotalBytes,-m_strName.size());

    if (m_pDDS)
    {
        DDSURFACEDESC2 ddsd;
        ZeroMemory(&ddsd, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        HRESULT hr = m_pDDS->GetSurfaceDesc(&ddsd);
        ShiAssert(SUCCEEDED(hr));

        if (ddsd.ddsCaps.dwCaps bitand DDSCAPS_SYSTEMMEMORY)
        {
            //InterlockedExchangeAdd((long *)&m_dwTotalBytes,-(ddsd.lPitch * ddsd.dwHeight));
        }
    }

    if (m_pImageData and m_bImageDataOwned)
    {
        DWORD dwSize = m_nImageDataStride * m_nHeight;
        /* InterlockedExchangeAdd((long *)&m_dwTotalBytes,-dwSize);
         InterlockedExchangeAdd((long *)&m_dwBitmapBytes,-dwSize); */
    }

#endif

    // JB 010318 CTD
    if (m_pDDS and not F4IsBadReadPtr(m_pDDS, sizeof(IDirectDrawSurface7))) m_pDDS->Release();

    m_pDDS = NULL;

    if (m_pPalAttach) m_pPalAttach->DetachFromTexture(this);

    if (m_pImageData and m_bImageDataOwned) delete[] m_pImageData;
}

bool TextureHandle::Create(char *strName, UInt32 info, UInt16 bits, UInt16 width, UInt16 height, DWORD dwFlags)
{

    if ( not rc) // FRB CTD
        return false;

    m_dwFlags = info;

#ifdef _DEBUG

    if (strName)
    {
        m_strName = strName;
        InterlockedExchangeAdd((long*)&m_dwTotalBytes, m_strName.size());
    }

#endif

    m_nWidth = width;
    m_nHeight = height;

    try
    {
        DDSURFACEDESC2 ddsd;
        ZeroMemory(&ddsd, sizeof(ddsd));

        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS bitor DDSD_PIXELFORMAT bitor DDSD_WIDTH bitor DDSD_HEIGHT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
        ddsd.dwWidth  = m_nWidth;
        ddsd.dwHeight = m_nHeight;

        if (info bitand MPR_TI_MIPMAP)
        {
            ddsd.dwMipMapCount = 5;
            ddsd.ddsCaps.dwCaps or_eq DDSCAPS_MIPMAP bitor DDSCAPS_COMPLEX;
        }
        else
        {
            ddsd.dwMipMapCount = 1;
        }

        // JB 010326 CTD
        //if( F4IsBadReadPtr(m_pD3DHWDeviceDesc,sizeof(_D3DDeviceDesc7)) )
        //{
        // ReportTextureLoadError("Bad Read Pointer");
        // return false;
        //}

        // Force power of 2
        if ((info bitand MPR_TI_MIPMAP) or (m_pD3DHWDeviceDesc->dpcTriCaps.dwTextureCaps bitand D3DPTEXTURECAPS_POW2))
        {
            int nMsb;

            nMsb = FindMsb(ddsd.dwWidth);

            if (ddsd.dwWidth bitand compl (1 << nMsb)) ddsd.dwWidth = 1 << (nMsb + 1);

            nMsb = FindMsb(ddsd.dwHeight);

            if (ddsd.dwHeight bitand compl (1 << nMsb)) ddsd.dwHeight = 1 << (nMsb + 1);
        }

        // Force square
        if (ddsd.dwWidth not_eq ddsd.dwHeight and (m_pD3DHWDeviceDesc->dpcTriCaps.dwTextureCaps bitand D3DPTEXTURECAPS_SQUAREONLY))
        {
            ddsd.dwWidth = ddsd.dwHeight = max(ddsd.dwWidth, ddsd.dwHeight);
        }

        if (dwFlags bitand FLAG_RENDERTARGET)
        {
            // Can't render to managed surfaces
            dwFlags or_eq FLAG_NOTMANAGED bitor FLAG_MATCHPRIMARY;

            // HW devices cannot render to system memory surfaces
            if (rc and (rc->m_eDeviceCategory >= DXContext::D3DDeviceCategory_Hardware))
            {
                dwFlags or_eq FLAG_INLOCALVIDMEM;
            }

            ddsd.ddsCaps.dwCaps or_eq DDSCAPS_3DDEVICE;
        }

        // Turn on texture management for HW devices
        if (rc and (rc->m_eDeviceCategory >= DXContext::D3DDeviceCategory_Hardware))
        {
            if ( not (dwFlags bitand FLAG_NOTMANAGED))
            {
                ddsd.ddsCaps.dwCaps2 or_eq DDSCAPS2_TEXTUREMANAGE;

                // Do not create system memory copies
                if (g_bEnableNonPersistentTextures)
                    ddsd.ddsCaps.dwCaps2 or_eq DDSCAPS2_DONOTPERSIST;
            }
            // Note: mutually exclusive with texture management
            else if (dwFlags bitand FLAG_INLOCALVIDMEM)
            {
                ddsd.ddsCaps.dwCaps or_eq DDSCAPS_VIDEOMEMORY;
            }
        }
        else
        {
            ddsd.ddsCaps.dwCaps or_eq DDSCAPS_SYSTEMMEMORY;
        }

        if (dwFlags bitand FLAG_HINT_STATIC)
        {
            ddsd.ddsCaps.dwCaps2 or_eq DDSCAPS2_HINTSTATIC;
        }

        if (dwFlags bitand FLAG_HINT_DYNAMIC)
        {
            ddsd.ddsCaps.dwCaps2 or_eq DDSCAPS2_HINTDYNAMIC;
        }

        if (m_dwFlags bitand MPR_TI_PALETTE)
        {
            ShiAssert(m_pPalAttach);

            if (m_dwFlags bitand MPR_TI_ALPHA)
            {
                if (m_dwFlags bitand MPR_TI_CHROMAKEY)
                {
                    ddsd.ddpfPixelFormat = m_arrPF[TEX_CAT_CHROMA_ALPHA];
                }
                else
                {
                    ddsd.ddpfPixelFormat = m_arrPF[TEX_CAT_ALPHA];
                }
            }
            else if (m_dwFlags bitand MPR_TI_CHROMAKEY)
            {
                ddsd.ddpfPixelFormat = m_arrPF[TEX_CAT_CHROMA];
            }
            else
            {
                ddsd.ddpfPixelFormat = m_arrPF[TEX_CAT_DEFAULT];
            }
        }
        else
        {
            if (dwFlags bitand FLAG_MATCHPRIMARY)
            {
                IDirectDrawSurface7Ptr pDDS;
                CheckHR(m_pD3DD->GetRenderTarget(&pDDS));

                IDirectDraw7Ptr pDD;
                CheckHR(pDDS->GetDDInterface((void**)&pDD));

                DDSURFACEDESC2 ddsdMode;
                ZeroMemory(&ddsdMode, sizeof(ddsdMode));
                ddsdMode.dwSize = sizeof(ddsdMode);

                CheckHR(pDD->GetDisplayMode(&ddsdMode));
                ddsd.ddpfPixelFormat = ddsdMode.ddpfPixelFormat;
            }
            else if (m_dwFlags bitand MPR_TI_DDS)
            {
                ddsd.ddpfPixelFormat.dwSize = 32;
                ddsd.ddpfPixelFormat.dwFlags or_eq DDPF_FOURCC;

                if (m_dwFlags bitand MPR_TI_DXT1)
                {
                    ddsd.ddpfPixelFormat.dwFourCC = MAKEFOURCC('D', 'X', 'T', '1');
                }
                else if (m_dwFlags bitand MPR_TI_DXT3)
                {
                    ddsd.ddpfPixelFormat.dwFourCC = MAKEFOURCC('D', 'X', 'T', '3');
                }
                else if (m_dwFlags bitand MPR_TI_DXT5)
                {
                    ddsd.ddpfPixelFormat.dwFourCC = MAKEFOURCC('D', 'X', 'T', '5');
                }
            }
        }

        // Create the surface
        HRESULT hr = rc->m_pDD->CreateSurface(&ddsd, &m_pDDS, NULL);

        if (FAILED(hr))
        {
            if (hr == DDERR_OUTOFVIDEOMEMORY)
            {
                MonoPrint("TextureHandle::Create - EVICTING MANAGED TEXTURES \n");

                // If we are out of video memory, evict all managed textures and retry
                CheckHR(rc->m_pD3D->EvictManagedTextures());
                CheckHR(rc->m_pDD->CreateSurface(&ddsd, &m_pDDS, NULL));
            }
            else
            {
                throw _com_error(hr);
            }
        }

        m_eSurfFmt = D3DXMakeSurfaceFormat(&ddsd.ddpfPixelFormat);

        m_nActualWidth = ddsd.dwWidth;
        m_nActualHeight = ddsd.dwHeight;

        // Attach DirectDraw palette if real palettized texture format created
        switch (m_eSurfFmt)
        {
            case D3DX_SF_PALETTE8:
            {
                if (m_pDDS and m_pPalAttach)
                    m_pDDS->SetPalette(m_pPalAttach->m_pIDDP);

                break;
            }
        }

#ifdef _DEBUG

        if (m_pDDS)
        {
            DDSURFACEDESC2 ddsd;
            ZeroMemory(&ddsd, sizeof(ddsd));
            ddsd.dwSize = sizeof(ddsd);
            HRESULT hr = m_pDDS->GetSurfaceDesc(&ddsd);
            ShiAssert(SUCCEEDED(hr));

#ifdef DEBUG_TEXTURE
            MonoPrint("Texture: %s [%s] created in %s memory\n",
                      strName, arrSurfFmt2String[m_eSurfFmt],
                      ddsd.ddsCaps.dwCaps bitand DDSCAPS_SYSTEMMEMORY  ? "SYSTEM" :
                      (ddsd.ddsCaps.dwCaps bitand DDSCAPS_LOCALVIDMEM ? "VIDEO" : "AGP"));
#endif

            if (ddsd.ddsCaps.dwCaps bitand DDSCAPS_SYSTEMMEMORY)
                InterlockedExchangeAdd((long *)&m_dwTotalBytes, ddsd.lPitch * ddsd.dwHeight);
        }

#endif

        return true;
    }

    catch (_com_error e)
    {
        ReportTextureLoadError(e.Error());
        return false;
    }
}

bool TextureHandle::Load(UInt16 mip, UInt chroma, UInt8 *TexBuffer, bool bDoNotLoadBits, bool bDoNotCopyBits, int nImageDataStride)
{
    ShiAssert(TexBuffer);

    // MLR 2003-10-10 - Prevent CTD.
    if ( not TexBuffer)
    {
        return false;
    }

#ifdef DEBUG

    if (m_dwFlags bitand MPR_TI_PALETTE) ShiAssert(m_pPalAttach);

#endif

    // Convert chroma key
    m_dwChromaKey = RGBA_MAKE(RGBA_GETBLUE(chroma), RGBA_GETGREEN(chroma), RGBA_GETRED(chroma), RGBA_GETALPHA(chroma));

    switch (m_eSurfFmt)
    {
        case D3DX_SF_A8R8G8B8:
        case D3DX_SF_X8R8G8B8:
        case D3DX_SF_PALETTE8:
            break;

        case D3DX_SF_A1R5G5B5:
        {
            PALETTEENTRY *pal = (PALETTEENTRY *)&m_dwChromaKey;
            m_dwChromaKey = (pal[0].peRed >> 3) bitor ((pal[0].peGreen >> 3) << 5) bitor ((pal[0].peBlue >> 3) << 10) bitor ((pal[0].peFlags >> 7) << 15);
            break;
        }

        case D3DX_SF_A4R4G4B4:
        {
            PALETTEENTRY *pal = (PALETTEENTRY *) &m_dwChromaKey;
            m_dwChromaKey = (pal[0].peRed >> 4) bitor ((pal[0].peGreen >> 4) << 4) bitor ((pal[0].peBlue >> 4) << 8) bitor ((pal[0].peFlags >> 4) << 12);
            break;
        }

        case D3DX_SF_R5G6B5:
        {
            PALETTEENTRY *pal = (PALETTEENTRY *) &m_dwChromaKey;
            m_dwChromaKey = (pal[0].peRed >> 3) bitor ((pal[0].peGreen >> 2) << 5) bitor ((pal[0].peBlue >> 3) << 11);
            break;
        }

        case D3DX_SF_R5G5B5:
        {
            PALETTEENTRY *pal = (PALETTEENTRY *) &m_dwChromaKey;
            m_dwChromaKey = (pal[0].peRed >> 3) bitor ((pal[0].peGreen >> 3) << 5) bitor ((pal[0].peBlue >> 3) << 10);
            break;
        }

        case D3DX_SF_DXT1:
        case D3DX_SF_DXT3:
        case D3DX_SF_DXT5:
            break;

        default:
            ;
    }

    m_nImageDataStride = nImageDataStride not_eq -1 ? nImageDataStride : m_nWidth;

    if ((m_dwFlags bitand MPR_TI_PALETTE) and m_eSurfFmt not_eq D3DX_SF_PALETTE8)
    {
        DWORD dwSize = m_nImageDataStride * m_nHeight;

        // Free previously allocated surface memory copy
        if (m_pImageData and m_bImageDataOwned)
        {
#ifdef _DEBUG
            //InterlockedExchangeAdd((long *)&m_dwTotalBytes,-dwSize);
            //InterlockedExchangeAdd((long *)&m_dwBitmapBytes,-dwSize);
#endif

            delete[] m_pImageData;
        }

        if ( not bDoNotCopyBits)
        {
            m_pImageData = new BYTE[dwSize];

            if ( not m_pImageData)
            {
                ReportTextureLoadError(E_OUTOFMEMORY, true);
                return false;
            }

            memcpy(m_pImageData, TexBuffer, dwSize);
            m_bImageDataOwned = true;

#ifdef _DEBUG
            InterlockedExchangeAdd((long *)&m_dwTotalBytes, dwSize);
            InterlockedExchangeAdd((long *)&m_dwBitmapBytes, dwSize);
#endif
        }
        else
        {
            // Owner promises not to delete it while we are using it
            m_pImageData = TexBuffer;
            m_bImageDataOwned = false;
        }

        // Load it
        if ( not bDoNotLoadBits)
        {
            if ( not Reload())
            {
                // If loading failed free memory
                if (m_pImageData and m_bImageDataOwned) delete[] m_pImageData;

                m_pImageData = NULL;
                m_bImageDataOwned = false;

                return false;
            }
        }

        return true;
    }
    else
    {
        // Free previously allocated surface memory copy
        if (m_pImageData and m_bImageDataOwned)
        {
#ifdef _DEBUG
            InterlockedExchangeAdd((long *)&m_dwTotalBytes, -(m_nImageDataStride * m_nHeight));
#endif

            delete[] m_pImageData;
        }

        // Temporary value
        m_pImageData = TexBuffer;

        // Load it
        bool bResult = Reload();

        // Not copied make invalid
        m_pImageData = NULL;

        return bResult;
    }
}

inline WORD _RGB8toRGB565(DWORD sc)
{
    WORD r = (WORD)((sc >> 16) bitand 255);
    WORD g = (WORD)((sc >> 8) bitand 255);
    WORD b = (WORD)((sc bitand 255));
    r >>= 3;
    g >>= 2;
    b >>= 3;
    return static_cast<WORD>((r << 11) bitor (g << 5) bitor b);
}

inline WORD _RGB8toRGB555(DWORD sc)
{
    WORD r = (WORD)((sc >> 16) bitand 255);
    WORD g = (WORD)((sc >> 8) bitand 255);
    WORD b = (WORD)((sc bitand 255));
    r >>= 3;
    g >>= 3;
    b >>= 3;
    return static_cast<WORD>((r << 10) bitor (g << 5) bitor b);
}

inline WORD _RGB8toARGB1555(DWORD sc)
{
    WORD a = (WORD)((sc >> 24) bitand 255);
    WORD r = (WORD)((sc >> 16) bitand 255);
    WORD g = (WORD)((sc >> 8) bitand 255);
    WORD b = (WORD)((sc bitand 255));
    r >>= 3;
    g >>= 3;
    b >>= 3;
    a = (a > 0) ? 1 : 0;
    return ((a << 15) bitor (r << 10) bitor (g << 5) bitor b);
}

inline WORD _RGB8toARGB4444(DWORD sc)
{
    WORD a = (WORD)((sc >> 24) bitand 255);
    WORD r = (WORD)((sc >> 16) bitand 255);
    WORD g = (WORD)((sc >> 8) bitand 255);
    WORD b = (WORD)((sc bitand 255));
    r >>= 4;
    g >>= 4;
    b >>= 4;
    a >>= 4;
    return ((a << 12) bitor (r << 8) bitor (g << 4) bitor b);
}

bool TextureHandle::Reload()
{
    // No DX context
    if ( not m_pDDS) return false;

    if ( not (m_dwFlags bitand MPR_TI_DDS))
    {
        if ( not (m_dwFlags bitand MPR_TI_PALETTE))
        {
            ShiAssert(false);
            return true;
        }
    }

    if ( not m_pImageData) return false;

    DDSURFACEDESC2 ddsd;
    ZeroMemory(&ddsd, sizeof(ddsd));

    try
    {
        // Lock the surface
        ddsd.dwSize = sizeof(ddsd);

        // JB 010305 CTD
        //if(F4IsBadReadPtr(m_pDDS,sizeof(IDirectDrawSurface7))) return false;

        HRESULT hr = m_pDDS->Lock(NULL, &ddsd, DDLOCK_DONOTWAIT bitor DDLOCK_WRITEONLY bitor DDLOCK_SURFACEMEMORYPTR, NULL);

        if (FAILED(hr))
        {
            if (hr == DDERR_SURFACELOST)
            {
                // If the surface is lost, restore it and retry
                CheckHR(m_pDDS->Restore());

                CheckHR(m_pDDS->Lock(NULL, &ddsd, DDLOCK_WAIT bitor DDLOCK_WRITEONLY bitor DDLOCK_SURFACEMEMORYPTR, NULL));
            }
            else
            {
                throw _com_error(hr);
            }
        }

        // Can be larger but not smaller
        ShiAssert(m_nWidth <= (int) ddsd.dwWidth and m_nHeight <= (int) ddsd.dwHeight);

        // Reloading is a different story - because this is called VERY frequently we have to be very fast with whatever we are doing here

        if (m_dwFlags bitand MPR_TI_DDS)
        {
            BYTE *pDst = (BYTE *)ddsd.lpSurface;
            BYTE *pSrc = m_pImageData;

            memcpy(pDst, pSrc, m_nImageDataStride);
            /*for(int i = 0; i < m_nImageDataStride; i++){
             *pDst++ = *pSrc++;
            }*/
        }
        // sfr: weird.. added {} around switch
        else
        {
            switch (m_eSurfFmt)
            {
                case D3DX_SF_PALETTE8:
                {
                    BYTE *pSrc = m_pImageData;
                    BYTE *pDst = (BYTE *)ddsd.lpSurface;
                    DWORD dwPitch = ddsd.lPitch;

                    // If source and destination pitch match, use single loop
                    if (dwPitch == m_nWidth and m_nImageDataStride == m_nWidth)
                        memcpy(pDst, pSrc, m_nWidth * m_nHeight);
                    else
                    {
                        for (int y = 0; y < m_nHeight; y++)
                        {
                            memcpy(pDst, pSrc, m_nWidth);

                            pSrc += m_nImageDataStride;
                            pDst += dwPitch;
                        }
                    }

                    break;
                }

                case D3DX_SF_A8R8G8B8:
                case D3DX_SF_X8R8G8B8:
                {
                    DWORD dwTmp;

                    // Convert palette to 16bit
                    DWORD palette[256];
                    DWORD *pal = &m_pPalAttach->m_pPalData[0];

                    for (int i = 0; i < m_pPalAttach->m_nNumEntries; i++)
                    {
                        dwTmp = pal[i];

                        if (dwTmp not_eq m_dwChromaKey)
                            palette[i] = dwTmp;
                        else
                            // Zero alpha but preserve RGB for pre-alpha test filtering (0 == full transparent, 0xff == full opaque)
                            palette[i] = dwTmp bitand 0xffffff;
                    }

                    BYTE *pSrc = m_pImageData;
                    DWORD *pDst = (DWORD *) ddsd.lpSurface;
                    DWORD dwPitch = ddsd.lPitch >> 2;

                    if (dwPitch == m_nWidth and m_nImageDataStride == m_nWidth)
                    {
                        DWORD dwSize = m_nWidth * m_nHeight;

                        for (int i = 0; static_cast<unsigned int>(i) < dwSize; i++)
                        {
                            pDst[i] = palette[pSrc[i]];
                        }
                    }
                    else
                    {
                        for (int y = 0; y < m_nHeight; y++)
                        {
                            for (int x = 0; x < m_nWidth; x++)
                                pDst[x] = palette[pSrc[x]];

                            pSrc += m_nImageDataStride;
                            pDst += dwPitch;
                        }
                    }

                    break;
                }

                case D3DX_SF_A1R5G5B5:
                {
                    WORD dwTmp;

                    // Convert palette to 16bit
                    WORD palette[256];
                    PALETTEENTRY *pal = (PALETTEENTRY *)&m_pPalAttach->m_pPalData[0];

                    for (int i = 0; i < m_pPalAttach->m_nNumEntries; i++)
                    {
                        dwTmp = (pal[i].peRed >> 3) bitor ((pal[i].peGreen >> 3) << 5) bitor ((pal[i].peBlue >> 3) << 10) bitor ((pal[i].peFlags >> 7) << 15);

                        if (dwTmp not_eq (WORD)m_dwChromaKey)
                            palette[i] = dwTmp;
                        else
                            // Zero alpha but preserve RGB for pre-alpha test filtering (0 == full transparent, 0xff == full opaque)
                            palette[i] = dwTmp bitand 0x7fff;
                    }

                    BYTE *pSrc = m_pImageData;
                    WORD *pDst = (WORD *)ddsd.lpSurface;

                    // JB 010404 CTD
                    if (F4IsBadReadPtr(pSrc, sizeof(BYTE)) or F4IsBadReadPtr(pDst, sizeof(WORD))) break;

                    DWORD dwPitch = ddsd.lPitch >> 1;

                    if (dwPitch == m_nWidth and m_nImageDataStride == m_nWidth)
                    {
                        // If source and destination pitch match, use single loop
                        DWORD dwSize = m_nWidth * m_nHeight;

                        for (int i = 0; static_cast<unsigned int>(i) < dwSize; i++)
                            pDst[i] = palette[pSrc[i]];
                    }
                    else
                    {
                        for (int y = 0; y < m_nHeight; y++)
                        {
                            for (int x = 0; x < m_nWidth; x++)
                                pDst[x] = palette[pSrc[x]];

                            pSrc += m_nImageDataStride;
                            pDst += dwPitch;
                        }
                    }

                    break;
                }

                case D3DX_SF_A4R4G4B4:
                {
                    WORD dwTmp;

                    // Convert palette to 16bit
                    WORD palette[256];
                    PALETTEENTRY *pal = (PALETTEENTRY *)&m_pPalAttach->m_pPalData[0];

                    for (int i = 0; i < m_pPalAttach->m_nNumEntries; i++)
                    {
                        dwTmp = (pal[i].peRed >> 4) bitor ((pal[i].peGreen >> 4) << 4) bitor ((pal[i].peBlue >> 4) << 8) bitor ((pal[i].peFlags >> 4) << 12);

                        if (dwTmp not_eq (WORD) m_dwChromaKey)
                            palette[i] = dwTmp;
                        else
                            // Zero alpha but preserve RGB for pre-alpha test filtering (0 == full transparent, 0xff == full opaque)
                            palette[i] = dwTmp bitand 0xfff;
                    }

                    BYTE *pSrc = m_pImageData;
                    WORD *pDst = (WORD *)ddsd.lpSurface;
                    DWORD dwPitch = ddsd.lPitch >> 1;

                    if (dwPitch == m_nWidth and m_nImageDataStride == m_nWidth)
                    {
                        // If source and destination pitch match, use single loop
                        DWORD dwSize = m_nWidth * m_nHeight;

                        for (int i = 0; static_cast<unsigned int>(i) < dwSize; i++)
                            pDst[i] = palette[pSrc[i]];
                    }
                    else
                    {
                        for (int y = 0; y < m_nHeight; y++)
                        {
                            for (int x = 0; x < m_nWidth; x++)
                                pDst[x] = palette[pSrc[x]];

                            pSrc += m_nImageDataStride;
                            pDst += dwPitch;
                        }
                    }

                    break;
                }

                case D3DX_SF_R5G6B5:
                {
                    ShiAssert( not (m_dwFlags bitand MPR_TI_CHROMAKEY));

                    WORD dwTmp;

                    // Convert palette to 16bit
                    WORD palette[256];
                    PALETTEENTRY *pal = (PALETTEENTRY *)&m_pPalAttach->m_pPalData[0];

                    for (int i = 0; i < m_pPalAttach->m_nNumEntries; i++)
                    {
                        dwTmp = (pal[i].peRed >> 3) bitor ((pal[i].peGreen >> 2) << 5) bitor ((pal[i].peBlue >> 3) << 11);
                        palette[i] = (WORD) dwTmp;
                    }

                    BYTE *pSrc = m_pImageData;
                    WORD *pDst = (WORD *)ddsd.lpSurface;
                    DWORD dwPitch = ddsd.lPitch >> 1;

                    if (dwPitch == m_nWidth and m_nImageDataStride == m_nWidth)
                    {
                        // If source and destination pitch match, use single loop
                        DWORD dwSize = m_nWidth * m_nHeight;

                        for (int i = 0; static_cast<unsigned int>(i) < dwSize; i++)
                            pDst[i] = palette[pSrc[i]];
                    }
                    else
                    {
                        for (int y = 0; y < m_nHeight; y++)
                        {
                            for (int x = 0; x < m_nWidth; x++)
                                pDst[x] = palette[pSrc[x]];

                            pSrc += m_nImageDataStride;
                            pDst += dwPitch;
                        }
                    }

                    break;
                }

                case D3DX_SF_R5G5B5:
                {
                    ShiAssert( not (m_dwFlags bitand MPR_TI_CHROMAKEY));

                    WORD dwTmp;

                    // Convert palette to 16bit
                    WORD palette[256];
                    PALETTEENTRY *pal = (PALETTEENTRY *)&m_pPalAttach->m_pPalData[0];

                    for (int i = 0; i < m_pPalAttach->m_nNumEntries; i++)
                    {
                        dwTmp = (pal[i].peRed >> 3) bitor ((pal[i].peGreen >> 3) << 5) bitor ((pal[i].peBlue >> 3) << 10);
                        palette[i] = (WORD) dwTmp;
                    }

                    BYTE *pSrc = m_pImageData;
                    WORD *pDst = (WORD *)ddsd.lpSurface;
                    DWORD dwPitch = ddsd.lPitch >> 1;

                    if (dwPitch == m_nWidth and m_nImageDataStride == m_nWidth)
                    {
                        // If source and destination pitch match, use single loop
                        DWORD dwSize = m_nWidth * m_nHeight;

                        for (int i = 0; static_cast<unsigned int>(i) < dwSize; i++)
                            pDst[i] = palette[pSrc[i]];
                    }
                    else
                    {
                        for (int y = 0; y < m_nHeight; y++)
                        {
                            for (int x = 0; x < m_nWidth; x++)
                                pDst[x] = palette[pSrc[x]];

                            pSrc += m_nImageDataStride;
                            pDst += dwPitch;
                        }
                    }

                    break;
                }

                default:
                    ShiAssert(false);
            }

        }

        CheckHR(m_pDDS->Unlock(NULL));

        if (m_dwFlags bitand MPR_TI_MIPMAP)
        {
            MipLoadContext ctx = { 0, m_pDDS };

            if (g_bShowMipUsage)
                SetMipLevelColor(&ctx);

            CheckHR(m_pDDS->EnumAttachedSurfaces(&ctx, MipLoadCallback));
        }

        return true;
    }

    catch (_com_error e)
    {
        // Unlock if still locked
        if (ddsd.lpSurface) m_pDDS->Unlock(NULL);

        ReportTextureLoadError(e.Error(), true);
        return false;
    }
}

void TextureHandle::RestoreAll()
{
    if (m_pDDS and m_pDDS->IsLost() == DDERR_SURFACELOST)
    {
        HRESULT hr = m_pDDS->Restore();

        if (SUCCEEDED(hr))
        {
#ifdef _DEBUG
            MonoPrint("TextureHandle::RestoreAll - %s restored successfully\n", m_strName.c_str());
#endif

            Reload();
        }

#ifdef _DEBUG
        else MonoPrint("TextureHandle::RestoreAll - FAILED to restore %s \n", m_strName.c_str());

#endif
    }
}

//FIXME
void TextureHandle::Clear()
{
    DDSURFACEDESC2 ddsd;
    ZeroMemory(&ddsd, sizeof(ddsd));

    try
    {
        // Lock the surface
        ddsd.dwSize = sizeof(ddsd);

        CheckHR(m_pDDS->Lock(NULL, &ddsd, DDLOCK_WAIT bitor DDLOCK_WRITEONLY bitor DDLOCK_SURFACEMEMORYPTR, NULL));

        // Can be larger but not smaller
        ShiAssert(m_nWidth <= (int) ddsd.dwWidth and m_nHeight <= (int) ddsd.dwHeight);

        if (ddsd.lPitch == ddsd.dwWidth)
        {
            // If source and destination pitch match, use single loop
            DWORD dwSize = ddsd.dwWidth * ddsd.dwHeight * (ddsd.ddpfPixelFormat.dwRGBBitCount >> 3);
            memset(ddsd.lpSurface, 0, dwSize);
        }
        else
        {
            BYTE *pDst = (BYTE *)ddsd.lpSurface;
            DWORD dwSize = ddsd.dwWidth * (ddsd.ddpfPixelFormat.dwRGBBitCount >> 3);

            for (int y = 0; static_cast<unsigned int>(y) < ddsd.dwHeight; y++)
            {
                memset(pDst, 0, dwSize);
                pDst += ddsd.lPitch;
            }
        }

        CheckHR(m_pDDS->Unlock(NULL));

        if (m_dwFlags bitand MPR_TI_MIPMAP)
        {
            MipLoadContext ctx = { 0, m_pDDS };

            if (g_bShowMipUsage)
                SetMipLevelColor(&ctx);

            CheckHR(m_pDDS->EnumAttachedSurfaces(&ctx, MipLoadCallback));
        }
    }

    catch (_com_error e)
    {
        // Unlock if still locked
        if (ddsd.lpSurface)
            m_pDDS->Unlock(NULL);
    }
}

bool TextureHandle::SetPriority(DWORD dwPrio)
{
    if (m_pDDS) return SUCCEEDED(m_pDDS->SetPriority(dwPrio));

    return false;
}

void TextureHandle::PreLoad()
{
    if (m_pDDS and m_pD3DD)
    {
        m_pD3DD->PreLoad(m_pDDS);
    }
}

void TextureHandle::ReportTextureLoadError(HRESULT hr, bool bDuringLoad)
{
#ifdef _DEBUG
    MonoPrint("Texture: Failed to %s texture %s (Code: %X)\n", bDuringLoad ? "load" : "create", m_strName.c_str(), hr);
#endif
}

void TextureHandle::ReportTextureLoadError(char *strReason)
{
#ifdef _DEBUG
    MonoPrint("Texture: %s failed to load (Reason: %X)\n", m_strName.c_str(), strReason);
#endif
}

void TextureHandle::PaletteAttach(PaletteHandle *p)
{
    m_pPalAttach = p;
}

void TextureHandle::PaletteDetach(PaletteHandle *p)
{
    ShiAssert(p == m_pPalAttach);
    m_pPalAttach = NULL;
}

void TextureHandle::StaticInit(IDirect3DDevice7 *pD3DD)
{
    // Warning: Not addref'd
    ShiAssert(pD3DD);
    m_pD3DD = pD3DD;

    if ( not m_pD3DD)
        return;

    m_pD3DHWDeviceDesc = new D3DDEVICEDESC7;

    if ( not m_pD3DHWDeviceDesc) return;

    HRESULT hr = m_pD3DD->GetCaps(m_pD3DHWDeviceDesc);
    ShiAssert(SUCCEEDED(hr));

    //Note: DDS textures get their own format.If no hardware DXTn support, use one of these.
    TEXTURESEARCHINFO tsi_16[6] =
    {
        //bpp,alpha,pal
        { 16, 0, FALSE, FALSE, &m_arrPF[0] }, //DEFAULT (DXT1)
        { 16, 1, FALSE, FALSE, &m_arrPF[1] }, //CHROMA (DXT1)
        { 16, 4, FALSE, FALSE, &m_arrPF[2] }, //ALPHA (DXT3)
        { 16, 4, FALSE, FALSE, &m_arrPF[3] }, //CHROMA_ALPHA (DXT3)
    };

    TEXTURESEARCHINFO tsi_32[6] =
    {
        //bpp,alpha,pal
        { 32, 0, FALSE, FALSE, &m_arrPF[0] }, //DEFAULT (DXT1)
        { 32, 8, FALSE, FALSE, &m_arrPF[1] }, //CHROMA (DXT1)
        { 32, 8, FALSE, FALSE, &m_arrPF[2] }, //ALPHA (DXT3)
        { 32, 8, FALSE, FALSE, &m_arrPF[3] }, //CHROMA_ALPHA (DXT3)
    };

    TEXTURESEARCHINFO *ptsi;

    if (DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_16)
    {
        ptsi = tsi_16;
    }
    else
    {
        ptsi = tsi_32;
    }

    for (int i = 0; i < TEX_CAT_MAX; i++)
    {
        m_pD3DD->EnumTextureFormats(TextureSearchCallback, &ptsi[i]);
        ShiAssert(ptsi[i].bFoundGoodFormat)
    }
}

void TextureHandle::StaticCleanup()
{
    if (m_pD3DHWDeviceDesc)
    {
        delete m_pD3DHWDeviceDesc;
        m_pD3DHWDeviceDesc = NULL;
    }

    // sfr: doesnt this leak??
    if (m_pD3DD)
    {
        m_pD3DD = NULL;
    }
}

HRESULT CALLBACK TextureHandle::TextureSearchCallback(DDPIXELFORMAT* pddpf, VOID* param)
{
    if (NULL == pddpf or NULL == param)
    {
        return DDENUMRET_OK;
    }

    TEXTURESEARCHINFO* ptsi = (TEXTURESEARCHINFO *)param;

    // Skip any funky modes
    if (pddpf->dwFlags bitand (DDPF_LUMINANCE bitor DDPF_BUMPLUMINANCE bitor DDPF_BUMPDUDV))
    {
        return DDENUMRET_OK;
    }

    // Retired
    if (ptsi->bUsePalette)
    {
        if ( not (pddpf->dwFlags bitand DDPF_PALETTEINDEXED8))
        {
            return DDENUMRET_OK;
        }

        // Accept the first 8-bit palettized format we get
        memcpy(ptsi->pddpf, pddpf, sizeof(DDPIXELFORMAT));
        ptsi->bFoundGoodFormat = TRUE;
        return DDENUMRET_CANCEL;
    }

    // Else, skip any paletized formats (all modes under 16bpp)
    if (pddpf->dwRGBBitCount < 16)
    {
        return DDENUMRET_OK;
    }

    // Skip any FourCC formats
    if (pddpf->dwFourCC not_eq 0)
    {
        return DDENUMRET_OK;
    }

    // Calc alpha depth
    DWORD dwMask = pddpf->dwRGBAlphaBitMask;
    WORD wAlphaBits = 0;

    while (dwMask)
    {
        dwMask = dwMask bitand (dwMask - 1);
        wAlphaBits++;
    }

    // Make sure current alpha format agrees with requested format type
    if ((ptsi->dwDesiredAlphaBPP) and not (pddpf->dwFlags bitand DDPF_ALPHAPIXELS) or wAlphaBits < ptsi->dwDesiredAlphaBPP)
    {
        return DDENUMRET_OK;
    }

    if (( not ptsi->dwDesiredAlphaBPP) and (pddpf->dwFlags bitand DDPF_ALPHAPIXELS))
    {
        return DDENUMRET_OK;
    }

    // Check if we found a good match
    if (pddpf->dwRGBBitCount == ptsi->dwDesiredBPP)
    {
        memcpy(ptsi->pddpf, pddpf, sizeof(DDPIXELFORMAT));
        ptsi->bFoundGoodFormat = TRUE;
        return DDENUMRET_CANCEL;
    }

    return DDENUMRET_OK;
}

#ifdef _DEBUG
void TextureHandle::MemoryUsageReport()
{
}

#endif

DWORD RGB32ToSurfaceColor(DWORD col, LPDIRECTDRAWSURFACE7 lpDDSurface, DDSURFACEDESC2 *pddsd = NULL)
{
    // NOTE: This function is sloooooow

    // Is it a palette surface?
    IDirectDrawPalettePtr pPal;

    if (SUCCEEDED(lpDDSurface->GetPalette(&pPal)))
        return col;

    DDSURFACEDESC2 ddsd;

    if (pddsd == NULL)
    {
        ZeroMemory(&ddsd, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        HRESULT hr = lpDDSurface->GetSurfaceDesc(&ddsd);
        ShiAssert(SUCCEEDED(hr));

        pddsd = &ddsd;
    }

    // Compute the right shifts required to get from 24 bit RGB to this pixel format
    UInt32 mask;

    int redShift;
    int greenShift;
    int blueShift;

    // RED
    mask = pddsd->ddpfPixelFormat.dwRBitMask;
    redShift = 8;
    ShiAssert(mask);

    while ( not (mask bitand 1))
    {
        mask >>= 1;
        redShift--;
    }

    while (mask bitand 1)
    {
        mask >>= 1;
        redShift--;
    }

    // GREEN
    mask = pddsd->ddpfPixelFormat.dwGBitMask;
    greenShift = 16;
    ShiAssert(mask);

    while ( not (mask bitand 1))
    {
        mask >>= 1;
        greenShift--;
    }

    while (mask bitand 1)
    {
        mask >>= 1;
        greenShift--;
    }

    // BLUE
    mask = pddsd->ddpfPixelFormat.dwBBitMask;
    ShiAssert(mask);
    blueShift = 24;

    while ( not (mask bitand 1))
    {
        mask >>= 1;
        blueShift--;
    }

    while (mask bitand 1)
    {
        mask >>= 1;
        blueShift--;
    }

    // Convert the key color from 32 bit RGB to the current pixel format
    DWORD dwResult;

    // RED
    if (redShift >= 0)
    {
        dwResult = (col >>  redShift) bitand pddsd->ddpfPixelFormat.dwRBitMask;
    }
    else
    {
        dwResult = (col << -redShift) bitand pddsd->ddpfPixelFormat.dwRBitMask;
    }

    // GREEN
    if (greenShift >= 0)
    {
        dwResult or_eq (col >>  greenShift) bitand pddsd->ddpfPixelFormat.dwGBitMask;
    }
    else
    {
        dwResult or_eq (col << -greenShift) bitand pddsd->ddpfPixelFormat.dwGBitMask;
    }

    // BLUE
    if (blueShift >= 0)
    {
        dwResult or_eq (col >>  blueShift) bitand pddsd->ddpfPixelFormat.dwBBitMask;
    }
    else
    {
        dwResult or_eq (col << -blueShift) bitand pddsd->ddpfPixelFormat.dwBBitMask;
    }

    return dwResult;
}

static void SetMipLevelColor(MipLoadContext *pCtx)
{
    static DWORD arrMipColors[] = { 0xffffff, 0xff0000, 0xff00, 0xff, 0xffff00,
                                    0xffff, 0xff00ff, 0x808080, 0x800000, 0x8000,
                                    0x80, 0x808000, 0x8080, 0x800080,
                                  };

    _ASSERTE(pCtx->nLevel < sizeof(arrMipColors) / sizeof(arrMipColors[0]));

    DDSURFACEDESC2 ddsd;
    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    HRESULT hr = pCtx->lpDDSurface->GetSurfaceDesc(&ddsd);
    ShiAssert(SUCCEEDED(hr));

    if (FAILED(hr))
    {
        return;
    }

    // Fill surface with a unique color representing the mipmap level
    DDBLTFX bfx;
    ZeroMemory(&bfx, sizeof(bfx));
    bfx.dwSize = sizeof(bfx);
    bfx.dwFillColor = RGB32ToSurfaceColor(arrMipColors[pCtx->nLevel], pCtx->lpDDSurface, &ddsd);

    hr = pCtx->lpDDSurface->Blt(NULL, NULL, NULL, DDBLT_COLORFILL bitor DDBLT_WAIT, &bfx);
}

static HRESULT WINAPI MipLoadCallback(LPDIRECTDRAWSURFACE7 lpDDSurface, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext)
{
    MipLoadContext *pCtx = (MipLoadContext *)lpContext;
    HRESULT hr;

    if (lpDDSurfaceDesc->ddsCaps.dwCaps bitand DDSCAPS_MIPMAP)
    {
        if (g_bShowMipUsage)
        {
            SetMipLevelColor(pCtx);
        }
        else
        {
            // Perform a 2:1 stretch blit from the parent surface
            hr = lpDDSurface->Blt(NULL, pCtx->lpDDSurface, NULL, DDBLT_WAIT, NULL);

            if (FAILED(hr))
            {
                ShiAssert(false);
                return DDENUMRET_CANCEL;
            }
        }

        // Process next lower mipmap level recursively
        MipLoadContext ctx = { pCtx->nLevel + 1, lpDDSurface };
        hr = lpDDSurface->EnumAttachedSurfaces(&ctx, MipLoadCallback);

        if (FAILED(hr))
        {
            ShiAssert(false);
            return DDENUMRET_CANCEL;
        }
    }

    return DDENUMRET_OK;
}

bool Texture::SaveDDS_DXTn(const char *szFileName, BYTE* pDst, int dimensions, DWORD flags)
{
    FILE *fp;

    fp = fopen(szFileName, "rb");

    if (fp)
    {
        fclose(fp);
        return false;
    }

    CompressionOptions options;

#if _MSC_VER >= 1300

    fileout = _open(szFileName, O_WRONLY bitor O_BINARY bitor O_CREAT, S_IWRITE);

    options.MipMapType = dNoMipMaps;
    options.bBinaryAlpha = false;

    if (flags bitand MPR_TI_ALPHA)
        options.TextureFormat = dDXT3;
    else if (flags bitand MPR_TI_CHROMAKEY)
        options.TextureFormat = dDXT1a;
    else
        options.TextureFormat = dDXT1;

    //nvDXTcompress((BYTE *)pDst,dimensions,dimensions,dimensions*4,&options,4,0);

    _close(fileout);

#endif

    return true;
}

bool Texture::DumpImageToFile(char *szFile, int palID)
{
    BYTE *pSrc, *pDst;
    char szFileName[256];
    BOOL /*bSave,*/bChroma = FALSE;
    DWORD *pal, dwSize, dwTmp, i, n;

    ShiAssert(this->imageData);
    ShiAssert(this->palette);
    ShiAssert(this->palette->paletteData);

    if ( not this->imageData) return false;

    sprintf(szFileName, "%s.dds", szFile);

    pSrc = (BYTE *)this->imageData;
    dwSize = dimensions * dimensions;
    pDst = new BYTE[dwSize * ARGB_TEXEL_SIZE];

    pal = this->palette->paletteData;

    for (i = 0, n = 0; i < dwSize; i++, n += ARGB_TEXEL_SIZE)
    {
        dwTmp = pal[pSrc[i]];

        // Preserve RGB for pre-alpha test filtering
        if (dwTmp == chromaKey)
        {
            dwTmp and_eq 0x00ffffff;
            bChroma = TRUE;
        }

        //ABGR to ARGB, Lowendian
        BYTE *p = (BYTE *)(&dwTmp);

        pDst[n + 0] = p[2]; //B
        pDst[n + 1] = p[1]; //G
        pDst[n + 2] = p[0]; //R
        pDst[n + 3] = p[3]; //A
    }

    SaveDDS_DXTn(szFileName, pDst, this->dimensions, this->flags);

    delete[] pDst;

    // Night texture
    /* if(palID == 3)
     {
     BYTE *from;
     DWORD npal[256],*to,*stop;

     sprintf(szFileName,"%sN.dds",szFile);

     to = npal+1;
     from = (BYTE *)pal+1;
     stop = npal + 248;
     npal[0] = pal[0];

     //FIXME
     while(to < stop)
     {
     *to  =    (FloatToInt32(*(from)   * 0.f)) // Red
     bitor (FloatToInt32(*(from+1) * 0.f) << 8) // Green
     bitor (FloatToInt32(*(from+2) * 0.f) << 16) // Blue
     bitor ((*(from+3)) << 24); // Alpha
     from += 4;
     to++;
     }

     *to = 0xFF0000FF; to++;
     *to = 0xFF0F30BE; to++;
     *to = 0xFFFF0000; to++;
     *to = 0xFFAD0000; to++;
     *to = 0xFFABD34C; to++;
     *to = 0xFF9BB432; to++;
     *to = 0xFF87C5F0; to++;
     *to = 0xFF61B2EA; to++;

     pDst = new BYTE[dwSize * ARGB_TEXEL_SIZE];

     n = 0;

     for(i = 0, n = 0; i < dwSize; i++, n += ARGB_TEXEL_SIZE)
     {
     dwTmp = npal[pSrc[i]];

     if(dwTmp == chromaKey)
     {
     dwTmp = 0;
     }

     if(dwTmp bitand 0x00FFFFFF)
     {
     bSave = TRUE;
     dwTmp and_eq 0x00FFFFFF;
     dwTmp or_eq 0xFF000000;
     }
     else
     dwTmp = 0;

     //ABGR to ARGB, Lowendian
     BYTE *p = (BYTE *)(&dwTmp);

     pDst[n+0] = p[2];//B
     pDst[n+1] = p[1];//G
     pDst[n+2] = p[0];//R
     pDst[n+3] = p[3];//A
     }

     if(bSave)
     SaveDDS_DXTn(szFileName,pDst,this->dimensions,this->flags);

     delete[] pDst;
     }
    */
    // Filter out fake chroma textures
    if ( not bChroma)
    {
        this->flags and_eq compl MPR_TI_CHROMAKEY;
    }

    return true;
}
