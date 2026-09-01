/***************************************************************************
    Tex.cpp
    Miro "Jammer" Torrielli
    10Oct03

 - Begin Major Rewrite
***************************************************************************/
#include "stdafx.h"
#include "image.h"
#include "tex.h"
#include "palbank.h"
#include "graphics/dxengine/d3d12/d3d12texturemanager.h" // #DX12 п.1 (was duplicated + a PHASE-3 line)
#include "graphics/vulkan/vulkantexturemanager.h" // Artscout - 2026 (#104): Vulkan engine-texture peer (g_pVulkanTextureManager)
// Artscout - 2026 (Linux Ф0): <d3d11.h> removed -- its ID3D11Texture2D use here was only an opaque handle for
// m_pGpuTex (a void* holding a D3D12Texture*), so the cast is gone. BUT the BC1/2/3 block decoder below (D3D12
// only) names DXGI_FORMAT_BC1_UNORM etc., which came from dxgiformat.h (pulled in by the old d3d11.h). That
// lightweight enum-only header is restored on Windows; Linux never compiles this D3D12 decoder path (it uses the
// Vulkan texture manager with native VkFormat_BC*), so no Windows-SDK header reaches the Linux build.
#ifdef _WIN32
#include <dxgiformat.h> // DXGI_FORMAT_* (enum only, no COM) for the BC decoder
#endif
#include "falclib/include/playerop.h"
#include "falclib/include/dispopts.h"

#ifdef USE_SH_POOLS
MEM_POOL Palette::pool;
#endif

static char TexturePath[256] = {'\0'};
static DXContext* rc = NULL;

extern bool g_bEnableNonPersistentTextures;
extern bool g_bShowMipUsage;

#define ARGB_TEXEL_SIZE 4
#define ARGB_TEXEL_BITS 32

static HRESULT WINAPI MipLoadCallback(LPDIRECTDRAWSURFACE7 lpDDSurface,
                                      LPDDSURFACEDESC2 lpDDSurfaceDesc,
                                      LPVOID lpContext);

struct MipLoadContext
{
    int nLevel;
    LPDIRECTDRAWSURFACE7 lpDDSurface;
};

void SetMipLevelColor(MipLoadContext* pCtx);

static char* arrSurfFmt2String[] = {
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
DWORD Texture::m_dwTotalBytes =
    0; // Total number of bytes allocated (including bitmap copies and object size)
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
    InterlockedIncrement((long*)&m_dwNumHandles); // Number of instances
    InterlockedExchangeAdd((long*)&m_dwTotalBytes, sizeof(*this));
#endif
};

Texture::~Texture()
{
#ifdef _DEBUG
    //InterlockedIncrement((long *)&m_dwNumHandles); // Number of instances
    //InterlockedExchangeAdd((long *)&m_dwTotalBytes,-sizeof(*this));
#endif
    if ((texHandle not_eq NULL) or (imageData not_eq NULL))
    {
        FreeAll();
    }
};

/* Store some useful global information.  The path is used for all
   texture loads through this interface and the RC is used for loading.
   This means that at present, only one device at a time can load textures
   through this interface.*/
void Texture::SetupForDevice(DXContext* texRC, char* path)
{
    // Store the texture path for future reference
    if (strlen(path) + 1 >= sizeof(TexturePath))
    {
        ShiError("Texture path name overflow");
    }

    strcpy(TexturePath, path);

    if (TexturePath[strlen(TexturePath) - 1] not_eq '\\' and
        TexturePath[strlen(TexturePath) - 1] not_eq '/')
    {
        strcat(TexturePath, "/");
    }

    rc = texRC;
    Palette::SetupForDevice(texRC);

    TextureHandle::StaticInit(texRC->m_pD3DD);
}

// This is called when we're done working with a given device (as represented by an RC).
void Texture::CleanupForDevice(DXContext* texRC)
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
BOOL Texture::LoadImage(char* filename, DWORD newFlags, BOOL addDefaultPath)
{
    char fullname[MAX_PATH];
    CImageFileMemory texFile;
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

    if (result not_eq
        1) // #104: file missing/unreadable -- skip cleanly (asserts are off in release, and falling
    { // through fed a NULL palette to Palette::Setup32 -> memcpy(dst, NULL, 1024) crash)
        fprintf(stderr, "[FF] Texture::LoadImage: OPEN FAILED: %s\n", fullname);
        return FALSE;
    }

    ShiAssert(result == 1)

        // Note that ReadTextureImage will close texFile for us
        texFile.glReadFileMem();
    result = ReadTextureImage(&texFile);

    if (result not_eq
        GOOD_READ) // #104: bad decode -> skip (DDS legitimately has a NULL palette; Setup32 is gated
    { // by the MPR_TI_DDS flag below and also NULL-guards itself, so don't reject on that)
        fprintf(stderr, "[FF] Texture::LoadImage: DECODE FAILED (%d): %s\n",
                (int)result, fullname);
        return FALSE;
    }

    ShiAssert(result == GOOD_READ)

        // We only support square textures
        ShiAssert(texFile.image.width == texFile.image.height) dimensions =
            texFile.image.width;
    // Artscout - 2026: the 2048 cap was a DX7 limit; the modern D3D11/12 backends handle 4096/8192.
    ShiAssert(dimensions <= 8192);

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
            palette->Setup32((DWORD*)texFile.image.palette);
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

        // Artscout - 2026: lifted DX7 2048 cap -- modern backends handle these.
        case 4096:
            flags or_eq MPR_TI_4096;
            break;

        case 8192:
            flags or_eq MPR_TI_8192;
            break;

        default:
            ShiAssert(false);
        }

        palette = NULL;
        dimensions = ddsd.dwLinearSize;
    }

#ifdef _DEBUG
    InterlockedExchangeAdd((long*)&m_dwTotalBytes, dimensions * dimensions);
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
        InterlockedExchangeAdd((long*)&m_dwTotalBytes,
                               -(dimensions * dimensions));
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
bool Texture::CreateTexture(char* strName)
{
    ShiAssert(rc not_eq NULL);
    ShiAssert(imageData);
    ShiAssert(texHandle == NULL);

    // JB 010318 CTD
    if (/* not F4IsBadReadPtr(palette,sizeof(Palette)) and */ (flags bitand
                                                               MPR_TI_PALETTE))
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
        texHandle->Create(strName, (WORD)flags, 8,
                          static_cast<UInt16>(dimensions),
                          static_cast<UInt16>(dimensions));

        // OW: Prevent a crash
        if (imageData not_eq NULL)
        {
            if (not texHandle->Load(0, chromaKey, (BYTE*)imageData))
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
        else if (flags bitand
                 MPR_TI_4096) // Artscout - 2026: lifted DX7 2048 cap
            width = 4096;
        else if (flags bitand MPR_TI_8192)
            width = 8192;

        texHandle = new TextureHandle();
        texHandle->Create(strName, flags, 32, static_cast<UInt16>(width),
                          static_cast<UInt16>(width));
        return texHandle->Load(0, 0, (BYTE*)imageData, false, false,
                               dimensions);
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

BOOL Texture::LoadAndCreate(char* filename, DWORD newFlags)
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
        if ((palette < &ThePaletteBank.PalettePool[0]) or
            (palette >= &ThePaletteBank.PalettePool[ThePaletteBank.nPalettes]))
        {
            // we cannot delete palettes that come from bank
            delete palette;
        }
    }

    palette = NULL;
}

// Reload the MPR texels with the ones we have stored locally.
bool Texture::UpdateMPR(char* strName)
{
    ShiAssert(rc not_eq NULL);
    ShiAssert(imageData);
    ShiAssert(texHandle);

    if (not texHandle or not imageData)
    {
        return false;
    }

    return texHandle->Load(0, chromaKey, (BYTE*)imageData);
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
IDirect3DDevice7* TextureHandle::m_pD3DD = NULL; // Warning: Not addref'd
struct _D3DDeviceDesc7* TextureHandle::m_pD3DHWDeviceDesc = NULL;

#ifdef _DEBUG
DWORD TextureHandle::m_dwNumHandles = 0; // Number of instances
DWORD TextureHandle::m_dwBitmapBytes = 0; // Bytes allocated for bitmap copies
DWORD TextureHandle::m_dwTotalBytes =
    0; // Total number of bytes allocated (including bitmap copies and object size)
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
    m_pGpuTex = NULL;

#ifdef _DEBUG
    InterlockedIncrement((long*)&m_dwNumHandles); // Number of instances
    InterlockedExchangeAdd((long*)&m_dwTotalBytes, sizeof(*this));
#endif
}

TextureHandle::~TextureHandle()
{
#ifdef _DEBUG
    InterlockedDecrement((long*)&m_dwNumHandles); // Number of instances
    //InterlockedExchangeAdd((long *)&m_dwTotalBytes,-sizeof(*this));
    //InterlockedExchangeAdd((long *)&m_dwTotalBytes,-m_strName.size());

    // Artscout - 2026: [DX7-PURGE] the DDraw GetSurfaceDesc byte-accounting is gone
    // (under a GPU backend m_pDDS is an SRV handle, not a DirectDraw surface).

    if (m_pImageData and m_bImageDataOwned)
    {
        DWORD dwSize = m_nImageDataStride * m_nHeight;
        /* InterlockedExchangeAdd((long *)&m_dwTotalBytes,-dwSize);
         InterlockedExchangeAdd((long *)&m_dwBitmapBytes,-dwSize); */
    }

#endif

    // #DX12: under D3D12 m_pDDS is a persistent D3D12Texture* (NOT a COM object) -> free it via the manager,
    // never ->Release() (that would call a garbage vtable). D3D11: the SRV/tex are IUnknown, release below.
    {
        extern bool g_bUseD3D12;
        extern bool g_bUseVulkan;
        if (g_bUseD3D12)
        {
#ifdef _WIN32 // D3D12 is Windows-only; Linux uses the Vulkan branch below
            if (m_pDDS and g_pD3D12TextureManager)
                g_pD3D12TextureManager->Free((D3D12Texture*)m_pDDS);
            m_pDDS = NULL;
            m_pGpuTex = NULL;
#endif // _WIN32
        }
        else if (g_bUseVulkan)
        {
            // Artscout - 2026 (#104): under Vulkan m_pDDS is a persistent VulkanTexture* -> destroy via the manager.
            if (m_pDDS and g_pVulkanTextureManager)
                g_pVulkanTextureManager->Destroy((VulkanTexture*)m_pDDS);
            m_pDDS = NULL;
            m_pGpuTex = NULL;
        }
        else
        {
#ifdef _WIN32
            // Artscout - 2026: [DX7-PURGE] under D3D11 m_pDDS holds the SRV (IUnknown) -- release it as such.
            if (m_pDDS)
                ((IUnknown*)m_pDDS)->Release();
            m_pDDS = NULL;
            // PHASE 3: release the D3D11 texture (m_pDDS already released the SRV above -- it's IUnknown)
            if (m_pGpuTex)
            {
                ((IUnknown*)m_pGpuTex)->Release();
                m_pGpuTex = NULL;
            }
#else
            // Linux has no COM/D3D11. At shutdown g_bUseVulkan gets cleared (DXContext::Shutdown) BEFORE the
            // last textures (weather etc.) are freed, so we land here even though m_pDDS is a VulkanTexture* --
            // calling ->Release() on it jumps a garbage vtable (crash on exit). Destroy it via the Vulkan
            // manager if it is still alive; NEVER ->Release().
            if (m_pDDS and g_pVulkanTextureManager)
                g_pVulkanTextureManager->Destroy((VulkanTexture*)m_pDDS);
            m_pDDS = NULL;
            m_pGpuTex = NULL;
#endif
        }
    }

    if (m_pPalAttach)
        m_pPalAttach->DetachFromTexture(this);

    if (m_pImageData and m_bImageDataOwned)
        delete[] m_pImageData;
}

bool TextureHandle::Create(char* strName, UInt32 info, UInt16 bits,
                           UInt16 width, UInt16 height, DWORD dwFlags)
{

    if (not rc) // FRB CTD
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

    // PHASE 3 (D3D7->D3D11): engine textures are stubbed (m_pDDS=NULL), startup proceeds; the real
    // load into a D3D11 texture is later. UI menus composite on the CPU.
    extern bool g_bUseGpu;
    extern bool g_bUseD3D12;
    extern bool g_bUseVulkan;
    if (g_bUseGpu) // #DX12/#104: stub engine textures on any GPU backend; the real load happens later in Load()
    {
        m_pDDS = NULL;
        m_pGpuTex = NULL;
        if (info & MPR_TI_DXT1)
            m_eSurfFmt = D3DX_SF_DXT1;
        else if (info & MPR_TI_DXT3)
            m_eSurfFmt = D3DX_SF_DXT3;
        else if (info & MPR_TI_DXT5)
            m_eSurfFmt = D3DX_SF_DXT5;
        else
            m_eSurfFmt = D3DX_SF_A8R8G8B8;

        // #DX12 п.3 (RTT): a render-target texture on D3D12 (MFD/HUD draw into it; the panel samples it via
        // DrawRttQuad). m_pDDS holds the D3D12Texture* (RTV+SRV); the backend binds it via BindSceneRtt.
#ifdef _WIN32 // D3D12 is Windows-only; Linux uses the Vulkan RTT branch below
        if ((dwFlags bitand FLAG_RENDERTARGET) and g_bUseD3D12 and
            g_pD3D12TextureManager and width > 0 and height > 0)
        {
            D3D12Texture* hh = g_pD3D12TextureManager->Alloc();
            if (hh and
                g_pD3D12TextureManager->CreateRenderTarget(*hh, width, height))
            {
                m_pDDS = (IDirectDrawSurface7*)hh;
                m_nActualWidth = width;
                m_nActualHeight = height;
            }
            else if (hh)
                g_pD3D12TextureManager->Free(hh);
            return true;
        }
#endif // _WIN32

        // Artscout - 2026 (#104): Vulkan RTT twin -- a color-attachment texture the displays draw into and the
        // panel quad samples. m_pDDS holds a VulkanTexture* (VkImage RTV+SRV); the backend binds it via BindSceneRtt.
        if ((dwFlags bitand FLAG_RENDERTARGET) and g_bUseVulkan and
            g_pVulkanTextureManager and width > 0 and height > 0)
        {
            VulkanTexture* vh =
                g_pVulkanTextureManager->CreateRenderTarget(width, height);
            if (vh)
            {
                m_pDDS = (IDirectDrawSurface7*)vh;
                m_nActualWidth = width;
                m_nActualHeight = height;
            }
            return true;
        }

        // Artscout - 2026 (D3D11 purge): the D3D11 render-target-texture creation (FLAG_RENDERTARGET) was
        // removed. Under D3D12 the RTT texture is created by the D3D12 texture manager earlier in this path
        // (m_pDDS = D3D12Texture*), so RTT displays keep working.
        return true;
    }

    // Artscout - 2026: [DX7-PURGE] DDraw CreateSurface texture path removed; GPU textures are
    // created above (D3D11/D3D12 texture manager). Non-GPU is unreachable.
    return false;
}

// PHASE 5: resolves an 8-bit palettized source (indices) into a D3D11 RGBA texture (tex+srv).
// Factored out of Load so Reload() goes the same path -- rebaking on a palette change
// (Translate3D). In D3D7 indices lived on the GPU and a palette change went via the hardware
// SetEntries; in D3D11 there is no hardware palette, so on every real palette change
// we must rebake RGBA from the saved source indices.
// #DX12 п.1: create an engine texture on the ACTIVE GPU backend. On success sets *outHandle to the opaque
// handle stored in TextureHandle::m_pDDS -- a D3D11 SRV under D3D11, a persistent D3D12Texture* under D3D12 --
// and *outTex to the D3D11 texture (NULL under D3D12). SelectTexture/SetTexture reinterpret the handle per API.
static bool EngineTexCreate(void** outHandle, void** outTex, int w, int h,
                            int fmt, const TexMipData* mips, int mipCount)
{
    extern bool g_bUseD3D12;
    if (g_bUseD3D12)
    {
#ifdef _WIN32 // D3D12 is Windows-only; Linux uses the Vulkan branch below
        if (not g_pD3D12TextureManager or not g_pD3D12TextureManager->IsValid())
            return false;
        D3D12Texture* hh = g_pD3D12TextureManager->Alloc();
        if (not hh)
            return false;
        if (not g_pD3D12TextureManager->Create(*hh, w, h, fmt, mips, mipCount))
        {
            g_pD3D12TextureManager->Free(hh);
            return false;
        }
        *outHandle = hh;
        *outTex = NULL;
        return true;
#endif // _WIN32
    }
    // Artscout - 2026 (#104): Vulkan twin -- upload mip 0 (the Vulkan manager is single-mip for now). fmt is the
    // same DXGI value; CreateFromDxgi maps it to a VkFormat. The opaque handle IS a VulkanTexture* (renderer casts it).
    extern bool g_bUseVulkan;
    if (g_bUseVulkan)
    {
        if (not g_pVulkanTextureManager or
            not g_pVulkanTextureManager->IsValid() or not mips or mipCount < 1)
            return false;
        unsigned bytes = (unsigned)mips[0].rowPitch * h;
        VulkanTexture* vh = g_pVulkanTextureManager->CreateFromDxgi(
            mips[0].data, bytes, w, h, fmt);
        if (not vh)
            return false;
        *outHandle = vh;
        *outTex = NULL;
        return true;
    }
    return false; // Artscout - 2026 (D3D11 purge): D3D12 is the sole GPU texture manager
}
// ===========================================================================================
// Artscout - 2026: #78 terrain -- BC1/BC3 -> RGBA8 decode + box mip-chain generation. The terrain
// tiles ship as SINGLE-MIP DXT1; with no mip chain the minified far ground aliases ("boils"/moire,
// distant roads flicker) and anisotropic filtering alone can't fully kill it. Decode the top BC mip,
// box-downsample a full RGBA mip chain, and upload RGBA8 + mips. Gated to SMALL textures (terrain
// tiles) so large cockpit/object atlases stay compressed (VRAM). D3D12 only. No BC decoder existed
// in the tree (NVTT only encodes), so this is a compact self-contained one (BC1/BC2/BC3).
// #WIN32-only: this CPU BC decoder feeds the D3D12 texture manager via DXGI_FORMAT_* tokens (dxgiformat.h,
// itself _WIN32-gated above). On Linux the Vulkan path uploads BC blocks natively (CreateFromDxgi), so the
// CPU decoder is not built there.
#ifdef _WIN32
static inline void TexBc565(unsigned c, int& r, int& g, int& b)
{
    r = (c >> 11) & 0x1F;
    r = (r << 3) | (r >> 2);
    g = (c >> 5) & 0x3F;
    g = (g << 2) | (g >> 4);
    b = c & 0x1F;
    b = (b << 3) | (b >> 2);
}

// decode one 4x4 BC block -> 16 RGBA8 pixels (row-major; R in the low byte for DXGI_FORMAT_R8G8B8A8_UNORM).
static void TexDecodeBCBlock(const unsigned char* blk, int dxgiFmt,
                             unsigned out[16])
{
    const bool bc1 = (dxgiFmt == DXGI_FORMAT_BC1_UNORM);
    const unsigned char* col =
        bc1 ?
            blk :
            (blk +
             8); // BC2/BC3 put alpha first (8 bytes), then the BC1 color block
    unsigned c0 = col[0] | (col[1] << 8), c1 = col[2] | (col[3] << 8);
    int r[4], g[4], b[4];
    TexBc565(c0, r[0], g[0], b[0]);
    TexBc565(c1, r[1], g[1], b[1]);
    const bool threeCol = bc1 && (c0 <= c1);
    if (!threeCol)
    {
        r[2] = (2 * r[0] + r[1]) / 3;
        g[2] = (2 * g[0] + g[1]) / 3;
        b[2] = (2 * b[0] + b[1]) / 3;
        r[3] = (r[0] + 2 * r[1]) / 3;
        g[3] = (g[0] + 2 * g[1]) / 3;
        b[3] = (b[0] + 2 * b[1]) / 3;
    }
    else
    {
        r[2] = (r[0] + r[1]) / 2;
        g[2] = (g[0] + g[1]) / 2;
        b[2] = (b[0] + b[1]) / 2;
        r[3] = g[3] = b[3] = 0;
    }
    const unsigned idx =
        col[4] | (col[5] << 8) | (col[6] << 16) | ((unsigned)col[7] << 24);
    int av[16];
    if (dxgiFmt == DXGI_FORMAT_BC3_UNORM)
    {
        int a0 = blk[0], a1 = blk[1], at[8];
        at[0] = a0;
        at[1] = a1;
        if (a0 > a1)
        {
            for (int i = 2; i < 8; i++)
                at[i] = ((8 - i) * a0 + (i - 1) * a1) / 7;
        }
        else
        {
            for (int i = 2; i < 6; i++)
                at[i] = ((6 - i) * a0 + (i - 1) * a1) / 5;
            at[6] = 0;
            at[7] = 255;
        }
        unsigned long long ab = 0;
        for (int i = 0; i < 6; i++)
            ab |= ((unsigned long long)blk[2 + i]) << (8 * i);
        for (int i = 0; i < 16; i++)
            av[i] = at[(ab >> (3 * i)) & 7];
    }
    else if (dxgiFmt == DXGI_FORMAT_BC2_UNORM)
    {
        for (int i = 0; i < 16; i++)
        {
            int nib = (blk[i / 2] >> ((i & 1) * 4)) & 0xF;
            av[i] = nib * 17;
        }
    }
    else
    {
        for (int i = 0; i < 16; i++)
            av[i] = (threeCol && (((idx >> (2 * i)) & 3) == 3)) ? 0 : 255;
    }
    for (int i = 0; i < 16; i++)
    {
        int ci = (idx >> (2 * i)) & 3;
        out[i] = (unsigned)r[ci] | ((unsigned)g[ci] << 8) |
                 ((unsigned)b[ci] << 16) | ((unsigned)av[i] << 24);
    }
}

static void TexDecodeBCImage(const unsigned char* bc, int w, int h, int dxgiFmt,
                             unsigned* rgba)
{
    const int bw = (w + 3) / 4, bh = (h + 3) / 4,
              bb = (dxgiFmt == DXGI_FORMAT_BC1_UNORM) ? 8 : 16;
    for (int by = 0; by < bh; ++by)
        for (int bx = 0; bx < bw; ++bx)
        {
            unsigned blk[16];
            TexDecodeBCBlock(bc + (size_t)(by * bw + bx) * bb, dxgiFmt, blk);
            for (int py = 0; py < 4; ++py)
                for (int px = 0; px < 4; ++px)
                {
                    int x = bx * 4 + px, y = by * 4 + py;
                    if (x < w && y < h)
                        rgba[(size_t)y * w + x] = blk[py * 4 + px];
                }
        }
}

#endif // _WIN32 (BC decoder above; TexBoxDown below is a backend-neutral RGBA mip helper)
static void TexBoxDown(const unsigned* s, int sw, int sh, unsigned* d, int dw,
                       int dh)
{
    for (int y = 0; y < dh; ++y)
        for (int x = 0; x < dw; ++x)
        {
            int x0 = x * 2, y0 = y * 2, x1 = (x0 + 1 < sw) ? x0 + 1 : sw - 1,
                y1 = (y0 + 1 < sh) ? y0 + 1 : sh - 1;
            unsigned a = s[(size_t)y0 * sw + x0], b = s[(size_t)y0 * sw + x1],
                     c = s[(size_t)y1 * sw + x0], e = s[(size_t)y1 * sw + x1],
                     o = 0;
            for (int ch = 0; ch < 4; ++ch)
            {
                int m = ((a >> (8 * ch)) & 0xFF) + ((b >> (8 * ch)) & 0xFF) +
                        ((c >> (8 * ch)) & 0xFF) + ((e >> (8 * ch)) & 0xFF);
                o |= (unsigned)(m >> 2) << (8 * ch);
            }
            d[(size_t)y * dw + x] = o;
        }
}

#ifdef _WIN32
static bool EngineTexCreateBCnMipped(void** outHandle, void** outTex, int w,
                                     int h, int dxgiFmt, const void* blob)
{
    extern bool g_bUseD3D12;
    if (!g_bUseD3D12 || !g_pD3D12TextureManager ||
        !g_pD3D12TextureManager->IsValid())
        return false;
    if (w < 1 || h < 1)
        return false;

    int mipCount = 1;
    {
        int mw = w, mh = h;
        while (mw > 1 || mh > 1)
        {
            mw = (mw > 1) ? mw >> 1 : 1;
            mh = (mh > 1) ? mh >> 1 : 1;
            ++mipCount;
        }
    }
    if (mipCount > 15)
        mipCount = 15;

    unsigned** lv = (unsigned**)malloc((size_t)mipCount * sizeof(unsigned*));
    int* lw = (int*)malloc((size_t)mipCount * sizeof(int));
    int* lh = (int*)malloc((size_t)mipCount * sizeof(int));
    TexMipData* mips =
        (TexMipData*)malloc((size_t)mipCount * sizeof(TexMipData));
    if (!lv || !lw || !lh || !mips)
    {
        free(lv);
        free(lw);
        free(lh);
        free(mips);
        return false;
    }

    lw[0] = w;
    lh[0] = h;
    lv[0] = (unsigned*)malloc((size_t)w * h * 4);
    if (!lv[0])
    {
        free(lv);
        free(lw);
        free(lh);
        free(mips);
        return false;
    }
    TexDecodeBCImage((const unsigned char*)blob, w, h, dxgiFmt, lv[0]);
    for (int i = 1; i < mipCount; ++i)
    {
        lw[i] = (lw[i - 1] > 1) ? lw[i - 1] >> 1 : 1;
        lh[i] = (lh[i - 1] > 1) ? lh[i - 1] >> 1 : 1;
        lv[i] = (unsigned*)malloc((size_t)lw[i] * lh[i] * 4);
        if (!lv[i])
        {
            mipCount = i;
            break;
        } // out of memory -> upload what we have
        TexBoxDown(lv[i - 1], lw[i - 1], lh[i - 1], lv[i], lw[i], lh[i]);
    }
    for (int i = 0; i < mipCount; ++i)
    {
        mips[i].data = lv[i];
        mips[i].rowPitch = lw[i] * 4;
    }

    D3D12Texture* hh = g_pD3D12TextureManager->Alloc();
    bool ok = hh && g_pD3D12TextureManager->Create(
                        *hh, w, h, DXGI_FORMAT_R8G8B8A8_UNORM, mips, mipCount);
    if (hh && !ok)
        g_pD3D12TextureManager->Free(hh);

    for (int i = 0; i < mipCount; ++i)
        free(lv[i]);
    free(lv);
    free(lw);
    free(lh);
    free(mips);
    if (!ok)
        return false;
    *outHandle = hh;
    *outTex = NULL;
    return true;
}
#endif // _WIN32 (CPU BC decoder for the D3D12 texture path)

static bool EngineTexCreateBCn(void** outHandle, void** outTex, int w, int h,
                               int fmt, const void* blob, int bytes)
{
    extern bool g_bUseD3D12;
    if (g_bUseD3D12)
    {
#ifdef _WIN32 // D3D12 is Windows-only; Linux uses the Vulkan branch below
        if (not g_pD3D12TextureManager or not g_pD3D12TextureManager->IsValid())
            return false;
        D3D12Texture* hh = g_pD3D12TextureManager->Alloc();
        if (not hh)
            return false;
        if (not g_pD3D12TextureManager->CreateBCn(*hh, w, h, fmt, blob, bytes))
        {
            g_pD3D12TextureManager->Free(hh);
            return false;
        }
        *outHandle = hh;
        *outTex = NULL;
        return true;
#endif // _WIN32
    }
    // Artscout - 2026 (#104): Vulkan twin -- native BC block upload (fmt = DXGI BC value -> VkFormat block).
    extern bool g_bUseVulkan;
    if (g_bUseVulkan)
    {
        if (not g_pVulkanTextureManager or
            not g_pVulkanTextureManager->IsValid())
            return false;
        VulkanTexture* vh = g_pVulkanTextureManager->CreateFromDxgi(
            blob, (unsigned)bytes, w, h, fmt);
        if (not vh)
            return false;
        *outHandle = vh;
        *outTex = NULL;
        return true;
    }
    return false; // Artscout - 2026 (D3D11 purge): D3D12 is the sole GPU texture manager
}

static bool ResolvePaletteToGpu(void** outHandle, void** outTex, int w, int h,
                                int stride, const UInt8* src, const DWORD* pal,
                                int nEnt, DWORD flags, DWORD chromaKey)
{
    if (w <= 0 or h <= 0 or not src)
        return false;
    if (stride <= 0)
        stride = w;

#ifdef _WIN32
    const int fmt = D3D12TextureManager::DxgiFormatFromMPR(flags);
#else
    // D3D12TextureManager is Windows-only; DxgiFormatFromMPR is a pure MPR->DXGI-number map the Vulkan
    // CreateFromDxgi path also consumes. Mirror its result as raw DXGI numeric values (BC1=71, BC2=74,
    // BC3=77, B8G8R8A8=87, B5G6R5=85) so the Vulkan branch gets an identical format code.
    const int fmt = (flags & MPR_TI_DXT1)  ? 71 :
                    (flags & MPR_TI_DXT3)  ? 74 :
                    (flags & MPR_TI_DXT5)  ? 77 :
                    (flags & MPR_TI_RGB16) ? 85 :
                                             87;
#endif
    const bool useChroma = (flags bitand MPR_TI_CHROMAKEY) != 0;
    const bool useAlpha = (flags bitand MPR_TI_ALPHA) != 0;
    const DWORD chromaRGB = chromaKey & 0x00FFFFFF;

    // Alpha rules mirror the D3D7 path: chroma entry -> alpha 0 (alpha-test discards),
    // MPR_TI_ALPHA -> alpha from the palette, else opaque. pal[i] is already swizzled to 0xAARRGGBB.
    DWORD resolved[256];
    for (int i = 0; i < 256; ++i)
    {
        DWORD c = (pal and i < nEnt) ? pal[i] : 0xFF000000;
        if (useChroma and (c & 0x00FFFFFF) == chromaRGB)
            c &= 0x00FFFFFF; // transparent
        else if (useAlpha)
            c = c; // alpha from the palette
        else
            c |= 0xFF000000; // opaque
        resolved[i] = c;
    }

    DWORD* rgba = (DWORD*)malloc((size_t)w * h * 4);
    if (not rgba)
        return false;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            rgba[y * w + x] = resolved[src[y * stride + x] & 0xFF];

    // Artscout - 2026: #78 -- the theater terrain tiles are PALETTE (this path), NOT DXT, so the DXT-only
    // mip path never reached them and the far ground kept aliasing/shimmering. The RGBA is already resolved
    // here, so just box-downsample a mip chain and upload RGBA + mips. Cap at 2048: terrain H-tiles are
    // 512/1024/2048 (terrtex getDDSWidth) -- the earlier 512 cap left the 1024/2048 near/mid tiles single-mip,
    // so they kept boiling under motion and MipLODBias had only mip 0 to clamp to. UI/HUD atlases are sampled
    // 1:1 (mip 0), so giving them a chain too is harmless (only +33% memory). D3D12 only.
    extern bool g_bUseD3D12;
    bool ok;
    if (g_bUseD3D12 and w >= 2 and h >= 2 and w <= 2048 and h <= 2048)
    {
        int mc = 1;
        {
            int mw = w, mh = h;
            while (mw > 1 || mh > 1)
            {
                mw = (mw > 1) ? mw >> 1 : 1;
                mh = (mh > 1) ? mh >> 1 : 1;
                ++mc;
            }
        }
        if (mc > 15)
            mc = 15;
        unsigned** lv = (unsigned**)malloc((size_t)mc * sizeof(unsigned*));
        int* lw = (int*)malloc((size_t)mc * sizeof(int));
        int* lh = (int*)malloc((size_t)mc * sizeof(int));
        TexMipData* mips = (TexMipData*)malloc((size_t)mc * sizeof(TexMipData));
        if (lv && lw && lh && mips)
        {
            lv[0] = (unsigned*)rgba;
            lw[0] = w;
            lh[0] = h;
            for (int i = 1; i < mc; ++i)
            {
                lw[i] = (lw[i - 1] > 1) ? lw[i - 1] >> 1 : 1;
                lh[i] = (lh[i - 1] > 1) ? lh[i - 1] >> 1 : 1;
                lv[i] = (unsigned*)malloc((size_t)lw[i] * lh[i] * 4);
                if (!lv[i])
                {
                    mc = i;
                    break;
                }
                TexBoxDown(lv[i - 1], lw[i - 1], lh[i - 1], lv[i], lw[i],
                           lh[i]);
            }
            for (int i = 0; i < mc; ++i)
            {
                mips[i].data = lv[i];
                mips[i].rowPitch = lw[i] * 4;
            }
            ok = EngineTexCreate(outHandle, outTex, w, h, fmt, mips, mc);
            for (int i = 1; i < mc; ++i)
                free(lv[i]); // lv[0] == rgba, freed below
        }
        else
            ok = false;
        free(lv);
        free(lw);
        free(lh);
        free(mips);
    }
    else
    {
        TexMipData mip = {rgba, w * 4};
        ok = EngineTexCreate(outHandle, outTex, w, h, fmt, &mip, 1);
    }
    free(rgba);
    return ok;
}

bool TextureHandle::Load(UInt16 mip, UInt chroma, UInt8* TexBuffer,
                         bool bDoNotLoadBits, bool bDoNotCopyBits,
                         int nImageDataStride)
{
    ShiAssert(TexBuffer);

    // MLR 2003-10-10 - Prevent CTD.
    if (not TexBuffer)
    {
        return false;
    }

    extern bool g_bUseGpu;
    extern bool g_bUseD3D12;
    extern bool g_bUseVulkan;
    if (g_bUseGpu)
    {
        // PHASE 3/#DX12/#104: real load into a GPU texture on the ACTIVE backend (D3D12 or Vulkan). Immutable +
        // thread-safe on the loader thread (the texture manager uploads via its own serialized queue). No active
        // manager -> no-op (menu CPU-composited).
#ifdef _WIN32 // D3D12 is Windows-only; the Vulkan guard below is the Linux path
        if (g_bUseD3D12 and (not g_pD3D12TextureManager or
                             not g_pD3D12TextureManager->IsValid()))
            return true;
#endif // _WIN32
        if (g_bUseVulkan and (not g_pVulkanTextureManager or
                              not g_pVulkanTextureManager->IsValid()))
            return true;

        const int w = m_nWidth, h = m_nHeight;
        if (w <= 0 or h <= 0)
            return true;

        // reload -- release the old one (free the persistent manager handle, NEVER ->Release: it is not a COM object)
        if (g_bUseD3D12)
        {
#ifdef _WIN32 // D3D12 is Windows-only; Linux uses the Vulkan branch below
            if (m_pDDS and g_pD3D12TextureManager)
                g_pD3D12TextureManager->Free((D3D12Texture*)m_pDDS);
            m_pDDS = NULL;
            m_pGpuTex = NULL;
#endif // _WIN32
        }
        else if (g_bUseVulkan)
        {
            if (m_pDDS and g_pVulkanTextureManager)
                g_pVulkanTextureManager->Destroy((VulkanTexture*)m_pDDS);
            m_pDDS = NULL;
            m_pGpuTex = NULL;
        }
        else
        {
            if (m_pDDS)
            {
                ((IUnknown*)m_pDDS)->Release();
                m_pDDS = NULL;
            }
            if (m_pGpuTex)
            {
                ((IUnknown*)m_pGpuTex)->Release();
                m_pGpuTex = NULL;
            }
        }

        m_nActualWidth = w;
        m_nActualHeight = h;
        m_dwChromaKey = RGBA_MAKE(RGBA_GETBLUE(chroma), RGBA_GETGREEN(chroma),
                                  RGBA_GETRED(chroma), RGBA_GETALPHA(chroma));

#ifdef _WIN32
        const int fmt = D3D12TextureManager::DxgiFormatFromMPR(m_dwFlags);
#else
        // D3D12TextureManager is Windows-only; DxgiFormatFromMPR is a pure MPR->DXGI-number map the Vulkan
        // CreateFromDxgi path also consumes. Mirror its result as raw DXGI numeric values (see tex.cpp above).
        const int fmt = (m_dwFlags & MPR_TI_DXT1)  ? 71 :
                        (m_dwFlags & MPR_TI_DXT3)  ? 74 :
                        (m_dwFlags & MPR_TI_DXT5)  ? 77 :
                        (m_dwFlags & MPR_TI_RGB16) ? 85 :
                                                     87;
#endif
        const bool isDXT =
            (m_eSurfFmt == D3DX_SF_DXT1 or m_eSurfFmt == D3DX_SF_DXT3 or
             m_eSurfFmt == D3DX_SF_DXT5);
        void* hdl = NULL;
        void* texptr =
            NULL; // #DX12: opaque handle (D3D11 SRV or D3D12Texture*)

        bool ok = false;

        if (isDXT)
        {
            // D3D12TextureManager::BlockBytes is Windows-only; it is a pure BC1(71)=8 / else=16 map. Mirror it
            // inline so the Vulkan block-size math below is identical (fmt 71 == DXGI BC1).
            int bb = (fmt == 71) ? 8 : 16;
            int bytes = ((w + 3) / 4) * ((h + 3) / 4) * bb;
            // Artscout - 2026: #78 -- SMALL DXT (terrain tiles) get a decoded RGBA mip chain so the far
            // ground stops aliasing/shimmering; large atlases stay compressed single-mip (VRAM). Falls
            // back to plain BCn if the mip path is unavailable/fails.
            // <=512 covers the theater terrain tiles (getDDSWidth tops out at 512; some tiles ship ONLY
            // the H/512 version and are used at range). Larger cockpit/object atlases (>512) stay BC.
#ifdef _WIN32
            if (g_bUseD3D12 and w <= 512 and h <= 512)
                ok = EngineTexCreateBCnMipped(&hdl, &texptr, w, h, fmt,
                                              TexBuffer); // D3D12 CPU mip-chain
#endif
            if (not ok)
                ok = EngineTexCreateBCn(&hdl, &texptr, w, h, fmt, TexBuffer,
                                        bytes);
        }
        else if (m_dwFlags bitand MPR_TI_PALETTE)
        {
            int stride = (nImageDataStride > 0) ? nImageDataStride : w;
            // Save the source indices: on a palette change (Translate3D) Reload() rebakes
            // RGBA. The owner (CPSurface/CPObject mpSourceBuffer) keeps the buffer alive, so
            // we keep only the pointer (bDoNotCopyBits == true on these paths).
            if (not bDoNotCopyBits)
            {
                if (m_pImageData and m_bImageDataOwned)
                    delete[] m_pImageData;
                m_pImageData = new BYTE[(size_t)stride * h];
                if (m_pImageData)
                    memcpy(m_pImageData, TexBuffer, (size_t)stride * h);
                m_bImageDataOwned = true;
            }
            else
            {
                if (m_pImageData and m_bImageDataOwned)
                    delete[] m_pImageData;
                m_pImageData = TexBuffer;
                m_bImageDataOwned = false;
            }
            m_nImageDataStride = stride;

            DWORD* pal = (m_pPalAttach and m_pPalAttach->m_pPalData) ?
                             m_pPalAttach->m_pPalData :
                             NULL;
            int nEnt = m_pPalAttach ? m_pPalAttach->m_nNumEntries : 0;
            ok = ResolvePaletteToGpu(&hdl, &texptr, w, h, stride, TexBuffer,
                                     pal, nEnt, m_dwFlags, m_dwChromaKey);
        }
        else if (m_dwFlags bitand MPR_TI_RGB16)
        {
            // PHASE 5: 16-bit B5G6R5 -- stride w*2 (previously else sent w*4 -> broken/white)
            TexMipData mip = {TexBuffer, w * 2};
            ok = EngineTexCreate(&hdl, &texptr, w, h, fmt, &mip, 1);
        }
        else if (m_dwFlags bitand MPR_TI_RGB24)
        {
            // PHASE 5: 24-bit BGR -> 32-bit BGRA (fmt=B8G8R8A8), source 3 bytes/pixel
            DWORD* rgba = (DWORD*)malloc((size_t)w * h * 4);
            if (rgba)
            {
                const BYTE* s = TexBuffer;
                for (int p = 0; p < w * h; ++p)
                {
                    BYTE b = s[0], g = s[1], r = s[2];
                    s += 3;
                    rgba[p] = (DWORD)b | ((DWORD)g << 8) | ((DWORD)r << 16) |
                              0xFF000000;
                }
                TexMipData mip = {rgba, w * 4};
                ok = EngineTexCreate(&hdl, &texptr, w, h, fmt, &mip, 1);
                free(rgba);
            }
        }
        else
        {
            // 32-bit ARGB source directly (B8G8R8A8). Artscout - 2026 (#78): in TEX_MODE_DDS the terrain
            // color/night tiles (terrtex.cpp:988) are created 32-bit and land HERE -> they were single-mip,
            // so the far/mid ground kept boiling under motion and MipLODBias had only mip 0 to clamp to.
            // Box-downsample a mip chain (mirrors the palette path). D3D12 + <=2048; else single mip as before.
            extern bool g_bUseD3D12;
            bool mipAttempted = false;
            if (g_bUseD3D12 and w >= 2 and h >= 2 and w <= 2048 and h <= 2048)
            {
                int mc = 1;
                {
                    int mw = w, mh = h;
                    while (mw > 1 || mh > 1)
                    {
                        mw = (mw > 1) ? mw >> 1 : 1;
                        mh = (mh > 1) ? mh >> 1 : 1;
                        ++mc;
                    }
                }
                if (mc > 15)
                    mc = 15;
                unsigned** lv =
                    (unsigned**)malloc((size_t)mc * sizeof(unsigned*));
                int* lw = (int*)malloc((size_t)mc * sizeof(int));
                int* lh = (int*)malloc((size_t)mc * sizeof(int));
                TexMipData* mips =
                    (TexMipData*)malloc((size_t)mc * sizeof(TexMipData));
                if (lv && lw && lh && mips)
                {
                    lv[0] = (unsigned*)TexBuffer;
                    lw[0] = w;
                    lh[0] = h; // mip 0 = caller's buffer (not freed)
                    for (int i = 1; i < mc; ++i)
                    {
                        lw[i] = (lw[i - 1] > 1) ? lw[i - 1] >> 1 : 1;
                        lh[i] = (lh[i - 1] > 1) ? lh[i - 1] >> 1 : 1;
                        lv[i] = (unsigned*)malloc((size_t)lw[i] * lh[i] * 4);
                        if (!lv[i])
                        {
                            mc = i;
                            break;
                        }
                        TexBoxDown(lv[i - 1], lw[i - 1], lh[i - 1], lv[i],
                                   lw[i], lh[i]);
                    }
                    for (int i = 0; i < mc; ++i)
                    {
                        mips[i].data = lv[i];
                        mips[i].rowPitch = lw[i] * 4;
                    }
                    ok = EngineTexCreate(&hdl, &texptr, w, h, fmt, mips, mc);
                    for (int i = 1; i < mc; ++i)
                        free(lv[i]); // lv[0] == TexBuffer, owned by caller
                    mipAttempted = true;
                }
                free(lv);
                free(lw);
                free(lh);
                free(mips);
            }
            if (not mipAttempted)
            {
                TexMipData mip = {
                    TexBuffer,
                    w * 4}; // fallback: single mip (non-D3D12 / >2048 / OOM)
                ok = EngineTexCreate(&hdl, &texptr, w, h, fmt, &mip, 1);
            }
        }

        if (ok)
        {
            m_pDDS = (IDirectDrawSurface7*)
                hdl; // SelectTexture casts back (D3D11 SRV, or D3D12Texture* under D3D12)
            m_pGpuTex =
                texptr; // Artscout - 2026 (Linux Ф0): void* handle; EngineTexCreate's outTex is already void*
        }

        return true;
    }

#ifdef DEBUG

    if (m_dwFlags bitand MPR_TI_PALETTE)
        ShiAssert(m_pPalAttach);

#endif

    // Convert chroma key
    m_dwChromaKey = RGBA_MAKE(RGBA_GETBLUE(chroma), RGBA_GETGREEN(chroma),
                              RGBA_GETRED(chroma), RGBA_GETALPHA(chroma));

    switch (m_eSurfFmt)
    {
    case D3DX_SF_A8R8G8B8:
    case D3DX_SF_X8R8G8B8:
    case D3DX_SF_PALETTE8:
        break;

    case D3DX_SF_A1R5G5B5:
    {
        PALETTEENTRY* pal = (PALETTEENTRY*)&m_dwChromaKey;
        m_dwChromaKey =
            (pal[0].peRed >> 3) bitor ((pal[0].peGreen >> 3) << 5) bitor
            ((pal[0].peBlue >> 3) << 10) bitor ((pal[0].peFlags >> 7) << 15);
        break;
    }

    case D3DX_SF_A4R4G4B4:
    {
        PALETTEENTRY* pal = (PALETTEENTRY*)&m_dwChromaKey;
        m_dwChromaKey =
            (pal[0].peRed >> 4) bitor ((pal[0].peGreen >> 4) << 4) bitor
            ((pal[0].peBlue >> 4) << 8) bitor ((pal[0].peFlags >> 4) << 12);
        break;
    }

    case D3DX_SF_R5G6B5:
    {
        PALETTEENTRY* pal = (PALETTEENTRY*)&m_dwChromaKey;
        m_dwChromaKey = (pal[0].peRed >> 3) bitor
                        ((pal[0].peGreen >> 2) << 5) bitor
                        ((pal[0].peBlue >> 3) << 11);
        break;
    }

    case D3DX_SF_R5G5B5:
    {
        PALETTEENTRY* pal = (PALETTEENTRY*)&m_dwChromaKey;
        m_dwChromaKey = (pal[0].peRed >> 3) bitor
                        ((pal[0].peGreen >> 3) << 5) bitor
                        ((pal[0].peBlue >> 3) << 10);
        break;
    }

    case D3DX_SF_DXT1:
    case D3DX_SF_DXT3:
    case D3DX_SF_DXT5:
        break;

    default:;
    }

    m_nImageDataStride =
        nImageDataStride not_eq -1 ? nImageDataStride : m_nWidth;

    if ((m_dwFlags bitand MPR_TI_PALETTE) and
        m_eSurfFmt not_eq D3DX_SF_PALETTE8)
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

        if (not bDoNotCopyBits)
        {
            m_pImageData = new BYTE[dwSize];

            if (not m_pImageData)
            {
                ReportTextureLoadError(E_OUTOFMEMORY, true);
                return false;
            }

            memcpy(m_pImageData, TexBuffer, dwSize);
            m_bImageDataOwned = true;

#ifdef _DEBUG
            InterlockedExchangeAdd((long*)&m_dwTotalBytes, dwSize);
            InterlockedExchangeAdd((long*)&m_dwBitmapBytes, dwSize);
#endif
        }
        else
        {
            // Owner promises not to delete it while we are using it
            m_pImageData = TexBuffer;
            m_bImageDataOwned = false;
        }

        // Load it
        if (not bDoNotLoadBits)
        {
            if (not Reload())
            {
                // If loading failed free memory
                if (m_pImageData and m_bImageDataOwned)
                    delete[] m_pImageData;

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
            InterlockedExchangeAdd((long*)&m_dwTotalBytes,
                                   -(m_nImageDataStride * m_nHeight));
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
    extern bool g_bUseGpu;
    extern bool g_bUseD3D12;
    extern bool g_bUseVulkan;
    if (g_bUseGpu)
    {
        // PHASE 5/#DX12/#104: rebake the palettized texture for the updated palette (Translate3D / TOD).
        // Non-palettized textures load once -- nothing to do here.
        if (not(m_dwFlags bitand MPR_TI_PALETTE))
            return true;
        if (not m_pImageData)
            return false;

        DWORD* pal = (m_pPalAttach and m_pPalAttach->m_pPalData) ?
                         m_pPalAttach->m_pPalData :
                         NULL;
        int nEnt = m_pPalAttach ? m_pPalAttach->m_nNumEntries : 0;

        void* hdl = NULL;
        void* texptr = NULL;
        if (not ResolvePaletteToGpu(&hdl, &texptr, m_nWidth, m_nHeight,
                                    m_nImageDataStride, m_pImageData, pal, nEnt,
                                    m_dwFlags, m_dwChromaKey))
            return false;

        if (g_bUseD3D12)
        {
#ifdef _WIN32 // D3D12 is Windows-only; Linux uses the Vulkan branch below
            if (m_pDDS and g_pD3D12TextureManager)
                g_pD3D12TextureManager->Free((D3D12Texture*)m_pDDS);
#endif // _WIN32
        }
        else if (g_bUseVulkan)
        {
            if (m_pDDS and g_pVulkanTextureManager)
                g_pVulkanTextureManager->Destroy((VulkanTexture*)m_pDDS);
        }
        else
        {
            if (m_pDDS)
                ((IUnknown*)m_pDDS)->Release();
            if (m_pGpuTex)
                ((IUnknown*)m_pGpuTex)->Release();
        }
        m_pDDS = (IDirectDrawSurface7*)hdl;
        m_pGpuTex =
            texptr; // Artscout - 2026 (Linux Ф0): void* handle; EngineTexCreate's outTex is already void*
        m_nActualWidth = m_nWidth;
        m_nActualHeight = m_nHeight;
        return true;
    }

    // Artscout - 2026: [DX7-PURGE] DDraw surface Lock/upload path removed; GPU rebakes above.
    return false;
}

void TextureHandle::RestoreAll()
{
    // Artscout - 2026: [DX7-PURGE] DDraw surface IsLost/Restore removed (no DDraw surfaces under GPU).
}

//FIXME
void TextureHandle::Clear()
{
    // Artscout - 2026: this is the legacy D3D7 DDraw Lock/clear path. Under D3D11 m_pDDS is either
    // NULL or actually an SRV (cast), so m_pDDS->Lock() would crash. It fired entering AG radar mode
    // (RenderGMComposite::SetRange -> rTexHandle->Clear() with m_pDDS==NULL). The GM radar composite
    // is still a DDraw subsystem (Blt/Lock) not yet ported to D3D11 -- no-op here to avoid the crash.
    // #DX12 A5: under D3D12 m_pDDS is a D3D12Texture* (now non-NULL because the GM panel is a render
    // target) -- the DDraw Lock below would dereference it as a surface and crash (0x1). No-op too.
    // Artscout - 2026: [DX7-PURGE] the legacy DDraw surface Lock/clear path is gone. Under GPU
    // m_pDDS is NULL or an SRV; clearing a GPU render target is the backend's job.
    return;
}

bool TextureHandle::SetPriority(DWORD dwPrio)
{
    // Artscout - 2026: [DX7-PURGE] DDraw texture-management priority removed.
    (void)dwPrio;
    return false;
}

void TextureHandle::PreLoad()
{
    // Artscout - 2026: [DX7-PURGE] D3D7 device PreLoad removed.
}

void TextureHandle::ReportTextureLoadError(HRESULT hr, bool bDuringLoad)
{
#ifdef _DEBUG
    MonoPrint("Texture: Failed to %s texture %s (Code: %X)\n",
              bDuringLoad ? "load" : "create", m_strName.c_str(), hr);
#endif
}

void TextureHandle::ReportTextureLoadError(char* strReason)
{
#ifdef _DEBUG
    MonoPrint("Texture: %s failed to load (Reason: %X)\n", m_strName.c_str(),
              strReason);
#endif
}

void TextureHandle::PaletteAttach(PaletteHandle* p)
{
    m_pPalAttach = p;
}

void TextureHandle::PaletteDetach(PaletteHandle* p)
{
    ShiAssert(p == m_pPalAttach);
    m_pPalAttach = NULL;
}

void TextureHandle::StaticInit(IDirect3DDevice7* pD3DD)
{
    // Artscout - 2026: [DX7-PURGE] the D3D7 device caps probe (GetCaps) and
    // EnumTextureFormats pixel-format search are gone. Under D3D11/D3D12 texture
    // formats are chosen by the texture manager; m_pD3DD is a dead opaque handle.
    m_pD3DD = pD3DD;
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

HRESULT CALLBACK TextureHandle::TextureSearchCallback(DDPIXELFORMAT* pddpf,
                                                      VOID* param)
{
    if (NULL == pddpf or NULL == param)
    {
        return DDENUMRET_OK;
    }

    TEXTURESEARCHINFO* ptsi = (TEXTURESEARCHINFO*)param;

    // Skip any funky modes
    if (pddpf->dwFlags bitand
        (DDPF_LUMINANCE bitor DDPF_BUMPLUMINANCE bitor DDPF_BUMPDUDV))
    {
        return DDENUMRET_OK;
    }

    // Retired
    if (ptsi->bUsePalette)
    {
        if (not(pddpf->dwFlags bitand DDPF_PALETTEINDEXED8))
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
    if ((ptsi->dwDesiredAlphaBPP) and
            not(pddpf->dwFlags bitand DDPF_ALPHAPIXELS) or
        wAlphaBits < ptsi->dwDesiredAlphaBPP)
    {
        return DDENUMRET_OK;
    }

    if ((not ptsi->dwDesiredAlphaBPP) and
        (pddpf->dwFlags bitand DDPF_ALPHAPIXELS))
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

DWORD RGB32ToSurfaceColor(DWORD col, LPDIRECTDRAWSURFACE7 lpDDSurface,
                          DDSURFACEDESC2* pddsd = NULL)
{
    // Artscout - 2026: [DX7-PURGE] DDraw surface colour-format probe removed (unused).
    (void)lpDDSurface;
    (void)pddsd;
    return col;
}

static void SetMipLevelColor(MipLoadContext* pCtx)
{
    // Artscout - 2026: [DX7-PURGE] DDraw mip-debug fill removed.
    (void)pCtx;
}

static HRESULT WINAPI MipLoadCallback(LPDIRECTDRAWSURFACE7 lpDDSurface,
                                      LPDDSURFACEDESC2 lpDDSurfaceDesc,
                                      LPVOID lpContext)
{
    // Artscout - 2026: [DX7-PURGE] DDraw mip Blt/enum callback removed (unused; GPU manages mips).
    (void)lpDDSurface;
    (void)lpDDSurfaceDesc;
    (void)lpContext;
    return DDENUMRET_OK;
}

bool Texture::SaveDDS_DXTn(const char* szFileName, BYTE* pDst, int dimensions,
                           DWORD flags)
{
    // Do not overwrite an existing .dds (matches the legacy behaviour).
    FILE* fp = fopen(szFileName, "rb");

    if (fp)
    {
        fclose(fp);
        return false;
    }

    // Compress the BGRA source to a DXT .dds via modern NVTT 3 (x64). The block
    // format (DXT1 / DXT1a / DXT3) is derived from the MPR_TI_* flags inside
    // SaveBCnDDS, exactly as the old nvDXTcompress path did.
#ifdef _WIN32 // D3D12TextureManager (NVTT DDS writer) is Windows-only; Linux does not bake .dds
    return D3D12TextureManager::SaveBCnDDS(szFileName, flags, pDst, dimensions,
                                           dimensions);
#else
    (void)szFileName;
    (void)pDst;
    (void)dimensions;
    (void)flags;
    return false;
#endif // _WIN32
}

bool Texture::DumpImageToFile(char* szFile, int palID)
{
    BYTE *pSrc, *pDst;
    char szFileName[256];
    BOOL /*bSave,*/ bChroma = FALSE;
    DWORD *pal, dwSize, dwTmp, i, n;

    ShiAssert(this->imageData);
    ShiAssert(this->palette);
    ShiAssert(this->palette->paletteData);

    if (not this->imageData)
        return false;

    sprintf(szFileName, "%s.dds", szFile);

    pSrc = (BYTE*)this->imageData;
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
        BYTE* p = (BYTE*)(&dwTmp);

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
    if (not bChroma)
    {
        this->flags and_eq compl MPR_TI_CHROMAKEY;
    }

    return true;
}
