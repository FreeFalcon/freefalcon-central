//-----------------------------------------------------------------------------
// D3D12Renderer.cpp -- Artscout - 2026: #DX12 Phase 3 (see D3D12Renderer.h).
//
// The D3D12 implementation of the neutral IRenderer. Mirrors D3D11Renderer, but
// where D3D11 uses an immediate context + driver-versioned UpdateSubresource /
// MAP_WRITE_DISCARD, D3D12 records into the backend's OPEN command list (between
// BeginFrame/Present) and manages its own resource versioning explicitly:
//
//   * a PSO cache -- D3D12 bakes {shaders, input layout, blend, depth, raster,
//     topology-type, RTV/DSV format} into one immutable object, so the ~states
//     the D3D11 path sets à la carte become a PSO selected per draw;
//   * per-frame UPLOAD rings (double-buffered by the backend frame index) for the
//     constants (b0..b4), the dynamic vertex data, and the trifan/index data --
//     an in-flight frame's data is never overwritten because the next frame uses
//     the other ring and the backend fences between reuse;
//   * root CBVs re-bound only when a CB actually changes (or when the list was
//     reset at frame start, which clears all root arguments).
//
// Textures are a later increment: SetTexture only toggles FF_TEXTURE0 for now and
// every draw samples a 1x1 white default (t0,t1) so untextured geometry is flat
// vertex colour (exactly the #78 GPU-terrain Phase-1 look). The object-VB path
// (aircraft/cockpit BSP: DrawObjectIndexed/Strip) stays D3D11-specific (it is not
// on IRenderer), so D3D12 3D is: terrain + screen symbology + 2D-in-3D effects.
//-----------------------------------------------------------------------------
#include <windows.h>
#include <d3d12.h>
#include <d3dcompiler.h>
// Artscout - 2026: #DX12 п.5 -- DXC/DXIL runtime compile for the SM6.1 view-instancing shaders (SV_ViewID needs
// SM6.1, which FXC/D3DCompile cannot emit). dxcompiler.dll is loaded at runtime (LoadLibrary + DxcCreateInstance),
// so no link-time dependency. dxcapi.h ships with modern Windows SDKs / the Microsoft.Direct3D.DXC NuGet; guard
// with __has_include so a toolchain WITHOUT it still builds (view instancing simply stays unavailable then).
#if defined(__has_include)
#if __has_include(<dxcapi.h>)
#include <dxcapi.h>
#define FF_HAVE_DXC 1
#endif
#endif
#include <wincodec.h> // Artscout - 2026: WIC image decode for LoadTextureFile (VR controller model textures)
#include <new>
// Artscout - 2026 (#104): this file spells the operators `and` / `not` / `bitand`. They are standard C++ tokens, but
// MSVC only accepts them via <iso646.h> (or /permissive-), and it used to arrive here by accident: FFStateMap.h
// included context.h, which includes it. FFStateMap.h no longer pulls context.h -- its STATE_* enum moved to
// ffstates.h so the Vulkan backend can share the one state table -- so declare the dependency where it is used.
#include <iso646.h>
#include <math.h> // Artscout - 2026 (#13): floorf, for the baked cloud-noise volume
#include <thread> // Artscout - 2026 (#13): the 256^3 bake is a parallel-for over z slices
#include <vector>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <map>
#include <vector>
#include <string>

#include "d3d12renderer.h"
#include "graphics/dxengine/d3d12backend.h" // g_pD3D12Backend (device + command list)
#include "graphics/dxengine/embeddedshader.h" // Artscout - 2026: FFEmu.hlsl from external file or embedded RCDATA
#include "graphics/shaders/ffshaderblobs.h" // #78: DXIL for the mesh-shader terrain
#include "graphics/dxengine/d3d12/d3d12texturemanager.h" // D3D12Texture (srvCpuPtr) for the SRV ring
#include "graphics/dxengine/common/irenderer.h" // full ScreenVertex POD (shared vertex) + FFStateMap
#include "graphics/dxengine/common/ffstatemap.h" // FFMapState / FFStateDesc / FF_* / STATE_* (shared with D3D11)

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

static void R12Log(const char* fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    _vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);
    buf[sizeof(buf) - 1] = 0;
    OutputDebugStringA(buf);
}
#define R12_RELEASE(p)                                                         \
    do                                                                         \
    {                                                                          \
        if (p)                                                                 \
        {                                                                      \
            (p)->Release();                                                    \
            (p) = 0;                                                           \
        }                                                                      \
    } while (0)

// Ring sizes (per frame, UPLOAD heap = system memory, not precious VRAM). Sized so a whole Falcon frame
// fits without a mid-frame resize; a draw that would overflow is dropped (logged once) rather than
// forcing a GPU stall to grow. Tune if a giant terrain frame ever overflows.
enum
{
    CB_RING_BYTES = 4 * 1024 * 1024,
    VB_RING_BYTES = 24 * 1024 * 1024,
    IB_RING_BYTES = 8 * 1024 * 1024
};

static inline unsigned AlignUp(unsigned x, unsigned a)
{
    return (x + (a - 1)) & ~(a - 1);
}

// CB mirrors -- MUST match FFEmu.hlsl (16-byte aligned), identical to D3D11Renderer.
struct CBViewport
{
    float screenW, screenH, pad0, pad1;
};
struct CBView
{
    float view[16];
    float proj[16];
    float camPos[4];
};
struct CBObject
{
    float world[16];
};
struct CBRender
{
    unsigned int flags;
    float alphaRef, fogStart, fogEnd;
    float fogColor[4];
    float chromaKey[4];
    float materialColor[4];
    float specular[4];
    float waterParams[4];
    float gloc
        [4]; // Artscout - 2026: FF_GLOC vignette (x=intensity, y=inner, z=outer)
    // Artscout - 2026: #13 volumetric clouds. MUST match cbRender in ffemu.hlsl field for field.
    float cloud0
        [4]; // x=zTop, y=zBot (both CAMERA-RELATIVE feet), z=coverage, w=density
    float cloud1
        [4]; // xy=noise anchor (camera.xy + wind), z=noise scale, w=step count
    float cloud2[4]; // xyz=sun direction (toward the sun), w=ambient blend
    float cloudSun[4]; // rgb=sun/moon colour at the layer, a=powder strength
    float cloud3
        [4]; // x=camera world z (absolute feet), y=profile, z/w=depth linearization A/B
    float cloudFwd
        [4]; // xyz=view forward axis (camera-relative world), w=1 when t2 has a usable depth
    float cloudDiag
        [4]; // #13: x=erosion, y=SUN GAIN, z=debug view, w=vertical noise scale
    float cloudDiag2
        [4]; // #13: x=patch freq, y=coverage scale, z=top vary, w=free (was ref-mode unit ft)
    float cloudLight
        [4]; // #13: x=SUN extinction/ft (NOT the view's), y=multiple-scattering strength
    float cloudLib
        [4]; // #13: x=model count (0=none), y/z=sdf encode range, w=cell size (noise units)
};
// Object/dynamic vertex layouts (match DXVbManager / the object input layout below).
struct DynV
{
    float p[3];
    unsigned col, spec;
    float tu, tv;
}; // 28 bytes (D3DDYNVERTEX)
struct ObjV
{
    float p[3];
    float n[3];
    unsigned col, spec;
    float tu, tv;
}; // 40 bytes
// LP64 guard: these must stay byte-exact against the GPU input layout on both platforms (a stray `long` = 8 bytes on
// Linux would shift them silently). col/spec are 32-bit D3DCOLOR -> `unsigned`, never `unsigned long`.
static_assert(sizeof(DynV) == 28, "DynV must stay 28 bytes (D3DDYNVERTEX)");
static_assert(sizeof(ObjV) == 40,
              "ObjV must stay 40 bytes (object input layout)");
// Artscout - 2026: #VFX Phase 1 -- the static unit-quad vertex (slot 0 of the particle draw). Corner in
// [-0.5,+0.5], uv in [0,1]. Matches particleIL slot-0 elements + VSInParticleVtx in FFEmu.hlsl.
struct ParticleQuadV
{
    float pos[2];
    float uv[2];
}; // 16 bytes

static inline void ArgbToRgba(unsigned long argb, float* out, float tol = -1.0f)
{
    out[0] = ((argb >> 16) & 0xFF) / 255.0f;
    out[1] = ((argb >> 8) & 0xFF) / 255.0f;
    out[2] = ((argb) & 0xFF) / 255.0f;
    out[3] = (tol >= 0.0f) ? tol : (((argb >> 24) & 0xFF) / 255.0f);
}

D3D12Renderer* g_pD3D12Renderer =
    NULL; // concrete instance (g_pRenderer points here under D3D12)

// Artscout - 2026 (D3D11 purge): these engine-wide globals were DEFINED in the now-deleted D3D11Renderer.cpp;
// re-homed here (D3D12 is the sole renderer). g_pRenderer = the active neutral renderer (set in DXContext::Init);
// g_bGpuDraw = "a GPU 3D draw happened this frame" (present path composites the 3D scene vs blitting the 2D
// UI); g_rttBatchActive = true during the RTT display batch (StartRtt..FinishRtt). Names kept (not renamed) so
// the ~dozen extern references across the engine stay unchanged.
IRenderer* g_pRenderer = NULL;
bool g_bGpuDraw = false;
bool g_rttBatchActive = false;

// #DX12: free-function hook so the texture manager can notify the renderer of a texture free WITHOUT including
// D3D12Renderer.h (avoids a header cycle). Nulls the renderer's current-texture pointer if it is being freed.
void D3D12Renderer_NotifyTextureFreed(const void* tex)
{
    if (g_pD3D12Renderer)
        g_pD3D12Renderer->OnTextureFreed(tex);
}

typedef std::map<unsigned, ID3D12PipelineState*> PsoMap;

//================================ ctor/dtor ==================================

D3D12Renderer::D3D12Renderer()
    : m_pDevice(0), m_pRootSig(0), m_pVSScreen(0), m_pVSObject(0), m_pPSMain(0),
      m_pVSParticle(0), m_pPSParticle(0), m_pQuadVB(0), m_pQuadIB(0),
      m_particleIbReady(false), m_pPsoCache(0), m_srvRingCount(0), m_srvInc(0),
      m_pWhiteStaging(0), m_whiteSrvCpu(0), m_whiteArraySrvCpu(0),
      m_pWhiteTex(0), m_pWhiteUpload(0), m_whiteUploaded(false), m_pTex0(0),
      m_pTex1(0), m_tableDirty(true), m_pWhite3DTex(0), m_pWhite3DUpload(0),
      m_white3DSrvCpu(0), m_pCloudNoise(0), m_pCloudDetail(0), m_libCount(0.0f),
      m_libSdfMin(0.0f), m_libSdfMax(0.0f), m_libDir{0}, m_pLightCache(0),
      m_pCsRootSig(0), m_pCsLightPso(0), m_pCsHeap(0), m_pCsSrvStage(0),
      m_pCSLight(0), m_lightCacheSrvCpu(0), m_lcValid(false), m_curFrame(0),
      m_curEpoch(0xFFFFFFFF), m_frameRebind(true), m_screenW(0), m_screenH(0),
      m_flags(0), m_curState(0), m_valid(false), m_pass(0),
      m_blend(BLEND_OPAQUE), m_depthWrite(false), m_depthTest(false),
      m_depthTargetBound(true), m_hudStencil(0), m_cull(0), m_bias(0),
      m_forcePerSample(false), m_alphaRef(0.5f), m_fogStart(0.0f),
      m_fogEnd(1.0e9f), m_fogColor(0xFF808080), m_chromaKey(0xFF000000),
      m_chromaTol(0.02f), m_texColorDiffuse(false), m_cockpitPass(false),
      m_hasTex0(false), m_irGrey(false), m_nvg(false), m_fullBright(false),
      m_dViewport(true), m_dView(true), m_dObject(true), m_dRender(true),
      m_dLights(true), m_dyn2DVerts(0), m_dyn2DVcount(0)
{
    for (int i = 0; i < 16; ++i)
    {
        m_view[i] = m_proj[i] = m_world[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
    m_camPos[0] = m_camPos[1] = m_camPos[2] = 0.0f;
    m_camPos[3] = 1.0f;
    // Artscout - 2026: #DX12 п.5 view-instancing state.
    for (int i = 0; i < VI_BLOB_COUNT; ++i)
    {
        m_viBlob[i] = 0;
        m_viBlobSize[i] = 0;
    }
    m_viAvailable = false;
    m_stereoActive = false;
    m_dStereo = true;
    m_stereoHasProj = false;
    m_stereoViewCount = 2;
    for (int e = 0; e < 4; ++e)
    {
        m_stereoWorldOff[e][0] = m_stereoWorldOff[e][1] =
            m_stereoWorldOff[e][2] = 0.0f;
        for (int i = 0; i < 16; ++i)
            m_stereoProj[e][i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
    m_materialColor[0] = m_materialColor[1] = m_materialColor[2] =
        m_materialColor[3] = 1.0f;
    m_specular[0] = m_specular[1] = m_specular[2] = m_specular[3] = 0.0f;
    m_gloc[0] = m_gloc[1] = m_gloc[2] = m_gloc[3] =
        0.0f; // Artscout - 2026: FF_GLOC off
    // Artscout - 2026: #13 clouds off until SetCloudParams runs (FF_CLOUD is only set by BeginCloudPass anyway).
    for (int c = 0; c < 4; ++c)
    {
        m_cloud0[c] = m_cloud1[c] = m_cloud2[c] = m_cloudSun[c] = m_cloud3[c] =
            m_cloudFwd[c] = 0.0f;
    }
    memset(m_lightsBuf, 0, sizeof(m_lightsBuf));
    for (int f = 0; f < kFrames; ++f)
    {
        m_pCbRing[f] = 0;
        m_pCbCpu[f] = 0;
        m_cbSize[f] = 0;
        m_cbOff[f] = 0;
        m_pVbRing[f] = 0;
        m_pVbCpu[f] = 0;
        m_vbSize[f] = 0;
        m_vbOff[f] = 0;
        m_pIbRing[f] = 0;
        m_pIbCpu[f] = 0;
        m_ibSize[f] = 0;
        m_ibOff[f] = 0;
        m_pSrvRing[f] = 0;
        m_srvRingOff[f] = 0;
        m_pClipUpload[f] = 0; // #78 clipmap
        m_pClipUploadCpu[f] = 0;
        m_clipUploadSize[f] = 0;
        m_clipUploadOff[f] = 0;
    }
    m_pClipPost = 0;
    m_pClipInfo = 0;
    m_pClipBounds = 0;
    m_pClipBoundsCpu = 0;
    m_clipTexels = m_clipLevels = m_clipTiles = 0;
    m_pMeshRootSig = 0;
    m_clipInCopyState = false;
}
D3D12Renderer::~D3D12Renderer()
{
    Release();
}

//================================ Init =======================================

bool D3D12Renderer::Init(const char* shaderDir)
{
    if (!g_pD3D12Backend || !g_pD3D12Backend->IsValid())
    {
        R12Log("[D3D12R] no backend\n");
        return false;
    }
    m_pDevice = g_pD3D12Backend->GetDevice();
    if (!m_pDevice)
        return false;

    m_pPsoCache = new PsoMap();

    if (!CompileShaders(shaderDir))
        return false;
    if (!CreateRootSignature())
        return false;
    if (!CreateConstantBuffers())
        return false;
    if (!CreateWhiteTexture())
        return false;

    m_screenW = g_pD3D12Backend->Width();
    m_screenH = g_pD3D12Backend->Height();

    m_valid = true;
    R12Log("D3D12Renderer: up (shaders + root sig + CB/VB/IB rings + PSO cache "
           "+ white default).\n");
    return true;
}

bool D3D12Renderer::CompileShaders(const char* shaderDir)
{
    // Artscout - 2026: prefer an external shaderDir\FFEmu.hlsl (runtime-tunable), else the embedded RCDATA copy.
    std::wstring wpathUnused;
    bool fromFile = false;
    std::string src = LoadFFEmuShaderSource(shaderDir, wpathUnused, fromFile);
    if (src.empty())
    {
        R12Log("[D3D12R] FFEmu.hlsl source not found (no external file, no "
               "embedded resource)\n");
        return false;
    }
    const char* srcName =
        fromFile ? "FFEmu.hlsl(file)" : "FFEmu.hlsl(embedded)";

    // Artscout - 2026: shader debug info is gated on g_bD3D12Debug, NOT on _DEBUG -- exactly like the D3D12
    // validation layer, and for the same reason: the builds that actually get flown are RELEASE, so under _DEBUG
    // the information was never there when it was needed. Without it RenderDoc's pixel debugger has no HLSL to
    // show: the optimiser has inlined and reordered everything, so you get raw disassembly and r0..r15 with no
    // way to find a named local. With it, Debug Pixel steps the real source and names every variable.
    // SKIP_OPTIMIZATION also keeps locals alive that the optimiser would otherwise fold away entirely.
    UINT flags = 0;
    {
        extern bool g_bD3D12Debug;
        if (g_bD3D12Debug)
            flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    }
    // Artscout - 2026: #VFX Phase 1 -- VS_Particle/PS_Particle are compiled here alongside the base
    // programs (same runtime D3DCompile mechanism; entry points added to the enumeration).
    // Artscout - 2026 (#13): CS_LightCache is compiled from the SAME source, with CLOUD_CS defined. Same file
    // on purpose -- CloudDensity must have exactly one definition, and the define keeps the u0 UAV out of the
    // ps_5_0 compiles (where u0 collides with render target 0). Optional like the particle shaders: without it
    // the clouds simply have no light cache, they do not fail Init.
    void** outs[6] = {&m_pVSScreen,   &m_pVSObject,   &m_pPSMain,
                      &m_pVSParticle, &m_pPSParticle, &m_pCSLight};
    const char* entries[6] = {"VS_Screen",   "VS_Object",   "PS_Main",
                              "VS_Particle", "PS_Particle", "CS_LightCache"};
    const char* targets[6] = {"vs_5_0", "vs_5_0", "ps_5_0",
                              "vs_5_0", "ps_5_0", "cs_5_0"};
    const D3D_SHADER_MACRO csDefs[2] = {{"CLOUD_CS", "1"}, {0, 0}};
    for (int i = 0; i < 6; ++i)
    {
        ID3DBlob* blob = 0;
        ID3DBlob* err = 0;
        HRESULT hr = D3DCompile(src.data(), src.size(), srcName,
                                (i == 5) ? csDefs : NULL,
                                D3D_COMPILE_STANDARD_FILE_INCLUDE, entries[i],
                                targets[i], flags, 0, &blob, &err);
        if (FAILED(hr))
        {
            R12Log("[D3D12R] compile %s failed: %s\n", entries[i],
                   err ? (const char*)err->GetBufferPointer() :
                         "(compile error)");
            if (err)
                err->Release();
            // #VFX Phase 1: the particle shaders (i>=3, VS_Particle/PS_Particle) are OPTIONAL -- a compile error
            // there must NOT fail whole-renderer Init (that skips CreateWhiteTexture -> null SRV rings/white tex
            // -> crash in FlushConstants). Log + leave the blob null (GetParticlePSO returns 0 -> particles just
            // don't draw). The base programs (Screen/Object/Main, i<3) remain fatal.
            if (i < 3)
                return false;
            *outs[i] = 0;
            continue;
        }
        if (err)
            err->Release();
        *outs[i] = blob; // ID3DBlob* -> void* member (kept for PSO creation)
    }

    // Artscout - 2026: #DX12 п.5 -- if the user asked for view-instanced stereo AND the device supports a
    // ViewInstancing tier, compile the DXIL/SM6.1 VI shader set (dxcompiler.dll). Failure is non-fatal: the
    // flat/per-eye path is untouched and the caller keeps the per-eye loop. Reuses the SAME `src` (so an
    // external tuned ffemu.hlsl feeds both the FXC and the DXC compile).
    extern bool g_bVrViewInstancing;
    bool tierOk = g_pD3D12Backend && g_pD3D12Backend->ViewInstancingSupported();
    if (g_bVrViewInstancing && tierOk)
    {
        m_viAvailable = CompileViShaders(shaderDir);
        R12Log("[D3D12R] view-instancing shaders: %s\n",
               m_viAvailable ? "READY (DXIL SM6.1)" :
                               "unavailable -> per-eye loop");
    }
    else if (g_bVrViewInstancing)
        R12Log("[D3D12R] view-instancing requested but device reports no "
               "ViewInstancing tier -> per-eye loop\n");

    return true;
}

// Artscout - 2026: #DX12 п.5 -- compile ONE FFEmu entry point to DXIL (SM6.1) via dxcompiler.dll (loaded once,
// process-lifetime). Copies the bytecode into `outBuf`/`outSize` (malloc). SV_ViewID (view instancing) needs
// SM6.1, which FXC/D3DCompile cannot emit -- hence DXC. Returns false (and leaves the buffer NULL) on any error.
static bool DxcCompileOne(const char* src, size_t srcLen, const wchar_t* entry,
                          const wchar_t* target, const wchar_t* define,
                          void** outBuf,
                          unsigned* outSize)
{
    *outBuf = 0;
    *outSize = 0;
#ifndef FF_HAVE_DXC
    (void)src;
    (void)srcLen;
    (void)entry;
    (void)target;
    R12Log("[D3D12R] built without dxcapi.h -> view instancing unavailable\n");
    return false;
#else
    // Load dxcompiler.dll on demand; keep the module handle for the process lifetime (no per-call load/unload).
    static HMODULE s_dxil =
        0; // dxil.dll (validator) -- optional; DXC signs the container if present
    static HMODULE s_dxc = 0;
    static DxcCreateInstanceProc s_create = 0;
    if (!s_dxc)
    {
        s_dxil = LoadLibraryA(
            "dxil.dll"); // optional signer; absence is fine on dev machines
        s_dxc = LoadLibraryA("dxcompiler.dll");
        if (!s_dxc)
        {
            R12Log(
                "[D3D12R] dxcompiler.dll not found -> view-instancing off\n");
            return false;
        }
        s_create =
            (DxcCreateInstanceProc)GetProcAddress(s_dxc, "DxcCreateInstance");
        if (!s_create)
        {
            R12Log("[D3D12R] DxcCreateInstance missing in dxcompiler.dll\n");
            return false;
        }
    }
    if (!s_create)
        return false;

    IDxcUtils* utils = 0;
    IDxcCompiler3* comp = 0;
    if (FAILED(s_create(CLSID_DxcUtils, IID_PPV_ARGS(&utils))) ||
        FAILED(s_create(CLSID_DxcCompiler, IID_PPV_ARGS(&comp))))
    {
        if (utils)
            utils->Release();
        if (comp)
            comp->Release();
        R12Log("[D3D12R] DxcCreateInstance failed\n");
        return false;
    }

    DxcBuffer srcBuf;
    srcBuf.Ptr = src;
    srcBuf.Size = srcLen;
    srcBuf.Encoding = DXC_CP_UTF8;

    const wchar_t* args[] = {
        L"-E",   entry,   L"-T", target, L"-Qstrip_debug", L"-Qstrip_reflect",
        L"-HV",  L"2018", // classic HLSL semantics (matches the FXC-compiled base)
        L"-Zpr", // Artscout - 2026: FORCE row-major cbuffer matrix packing. Without it
        // DXC read gView2/gProj2 column-major (transposed) even with the
        // row_major keyword -> the FF permuted view matrix came out axis-
        // swapped -> the whole world rendered rotated ~90deg vs the FXC base.
        // Artscout - 2026 (#107): switches on the ResourceDescriptorHeap path and the extra vertex slot.
        // Harmless in the 6_1 jobs -- they never take the bindless branch, and the define costs one bool.
        L"-D",   define ? define : L"FF_BINDLESS_OK=0",
    };
    IDxcResult* result = 0;
    HRESULT hr = comp->Compile(&srcBuf, args, _countof(args), NULL,
                               IID_PPV_ARGS(&result));
    bool ok = false;
    if (SUCCEEDED(hr) && result)
    {
        HRESULT status = E_FAIL;
        result->GetStatus(&status);
        if (FAILED(status))
        {
            IDxcBlobUtf8* errs = 0;
            result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errs), NULL);
            char ename[64];
            wcstombs(ename, entry, sizeof(ename));
            ename[63] = 0;
            R12Log("[D3D12R] DXC compile %s failed: %s\n", ename,
                   (errs && errs->GetStringLength()) ?
                       errs->GetStringPointer() :
                       "(dxc error)");
            if (errs)
                errs->Release();
        }
        else
        {
            IDxcBlob* obj = 0;
            result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&obj), NULL);
            if (obj && obj->GetBufferSize())
            {
                *outSize = (unsigned)obj->GetBufferSize();
                *outBuf = malloc(*outSize);
                if (*outBuf)
                {
                    memcpy(*outBuf, obj->GetBufferPointer(), *outSize);
                    ok = true;
                }
                else
                    *outSize = 0;
            }
            if (obj)
                obj->Release();
        }
    }
    if (result)
        result->Release();
    comp->Release();
    utils->Release();
    return ok;
#endif // FF_HAVE_DXC
}

bool D3D12Renderer::CompileViShaders(const char* shaderDir)
{
    std::wstring wpathUnused;
    bool fromFile = false;
    std::string src = LoadFFEmuShaderSource(shaderDir, wpathUnused, fromFile);
    if (src.empty())
    {
        R12Log("[D3D12R] VI: FFEmu.hlsl source not found\n");
        return false;
    }

    // DXIL for BOTH stages of the VI PSOs (a PSO cannot mix DXBC and DXIL). VS_Screen is reused unchanged for
    // the sky (identical both eyes); VS_ObjectVI/VS_ParticleVI read gView2[SV_ViewID]. PS_Main/PS_Particle are
    // the same pixel programs, just recompiled to DXIL so they can pair with the DXIL vertex shaders.
    struct
    {
        int slot;
        const wchar_t* entry;
        const wchar_t* target;
    } jobs[] = {
        {VI_OBJECT, L"VS_ObjectVI", L"vs_6_1"},
        {VI_SCREEN, L"VS_Screen", L"vs_6_1"},
        {VI_PARTICLE_VS, L"VS_ParticleVI", L"vs_6_1"},
        {VI_PSMAIN, L"PS_Main", L"ps_6_1"},
        {VI_PSPARTICLE, L"PS_Particle", L"ps_6_1"},
        // Artscout - 2026 (#107 bindless): its own pair at 6_6, where ResourceDescriptorHeap exists. Optional
        // like the particle ones -- if they fail, BindlessTerrainActive() stays false and the terrain draws
        // the old per-tile way.
        {VI_BINDLESS_VS, L"VS_Object", L"vs_6_6"},
        {VI_BINDLESS_PS, L"PS_Main", L"ps_6_6"},
    };
    for (int j = 0; j < (int)(sizeof(jobs) / sizeof(jobs[0])); ++j)
    {
        const bool bindlessJob = (jobs[j].slot == VI_BINDLESS_VS
                                  || jobs[j].slot == VI_BINDLESS_PS);

        if (bindlessJob && !BindlessWanted())
            continue;

        if (!DxcCompileOne(src.data(), src.size(), jobs[j].entry,
                           jobs[j].target,
                           bindlessJob ? L"FF_BINDLESS_OK=1" : NULL,
                           &m_viBlob[jobs[j].slot],
                           &m_viBlobSize[jobs[j].slot]))
        {
            // Object/Screen/PSMain are required for the world VI pass; particle and bindless are optional.
            if (jobs[j].slot == VI_PARTICLE_VS || jobs[j].slot == VI_PSPARTICLE
                || bindlessJob)
                continue;
            // Free anything already compiled and bail (per-eye loop is the fallback).
            for (int k = 0; k < VI_BLOB_COUNT; ++k)
            {
                if (m_viBlob[k])
                {
                    free(m_viBlob[k]);
                    m_viBlob[k] = 0;
                    m_viBlobSize[k] = 0;
                }
            }
            return false;
        }
    }
    return true;
}

bool D3D12Renderer::CreateRootSignature()
{
    // FFEmu layout: CBVs b0..b4 (root CBVs), SRVs t0..t1 (one table), static samplers s0,s1.
    // Artscout - 2026: #DX12 п.5 -- a 6th root CBV (b5 = cbViewStereo) is appended as param 6 (AFTER the SRV
    // table at param 5) so existing param indices are unchanged; the flat/per-eye path never binds it (harmless).
    D3D12_ROOT_PARAMETER params[7];
    ZeroMemory(params, sizeof(params));
    for (int b = 0; b < 5; ++b)
    {
        params[b].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        params[b].Descriptor.ShaderRegister = b;
        params[b].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    }
    D3D12_DESCRIPTOR_RANGE srvRange;
    ZeroMemory(&srvRange, sizeof(srvRange));
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    // Artscout - 2026: #13 -- t2 = the SCENE DEPTH, read by the FF_CLOUD raymarch to clamp itself against the
    // world (terrain/objects). It is bound for EVERY draw (a 1x1 dummy when there is no depth to read) because
    // the table is one contiguous range; only the cloud branch samples it.
    // #13: t3 = the NVDF, t4 = the up-rez DETAIL noise, t5 = the LIGHT CACHE (filled by CS_LightCache).
    srvRange.NumDescriptors = 6; // t0,t1,t2,t3,t4,t5
    srvRange.OffsetInDescriptorsFromTableStart =
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    params[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[5].DescriptorTable.NumDescriptorRanges = 1;
    params[5].DescriptorTable.pDescriptorRanges = &srvRange;
    params[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // #DX12 п.5: b5 = cbViewStereo (per-eye view/proj for the view-instanced VS). Vertex-only visibility.
    params[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[6].Descriptor.ShaderRegister = 5;
    params[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    // Artscout - 2026 (#78 terrain shimmer): the far ground boils under motion because its baked high-contrast
    // features (roads, coastlines) alias even with mips+aniso. A positive mip LOD bias nudges sampling toward
    // blurrier mips and kills that. Global (near cockpit is at mip 0 -> barely affected). Tune via FFViper.cfg
    // "MipLodBias" (0 = original). Clamp to a sane range so a fat-fingered cfg can't blur the world to mush.
    extern float g_fMipLodBias;
    float mipBias = g_fMipLodBias;
    if (mipBias < -4.0f)
        mipBias = -4.0f;
    else if (mipBias > 4.0f)
        mipBias = 4.0f;

    D3D12_STATIC_SAMPLER_DESC samp[2];
    ZeroMemory(samp, sizeof(samp));
    for (int s = 0; s < 2; ++s)
    {
        // Artscout - 2026: ANISOTROPIC-wrap. The far terrain is viewed at grazing angles where its tile
        // textures minify hard along the view direction; plain linear (even trilinear) can't gather that
        // elongated footprint -> "boiling"/moire shimmer over the whole distant ground (roads flicker).
        // Anisotropic filtering takes multiple taps along the major axis of the pixel footprint, which
        // reduces that aliasing MUCH. Terrain tiles now DO carry box-filtered mip chains (ResolvePaletteToGpu),
        // but the baked high-contrast roads/coastlines still boil under motion -> the MipLODBias below nudges
        // sampling toward blurrier mips to finish the job. MaxAnisotropy 8 = good quality vs cost (tunable;
        // VR perf #65). The D3D11 path swaps point/clamp per state; this is the D3D12 default.
        // Artscout - 2026: level/on-off from the graphics option (g_bAnisoEnable / g_nAnisoSamples, read at
        // root-signature create -> applied on entering 3D). OFF -> trilinear (linear mip). Clamped 1..16.
        extern bool g_bAnisoEnable;
        extern int g_nAnisoSamples;
        int aniso = g_nAnisoSamples;
        if (aniso < 1)
            aniso = 1;
        if (aniso > 16)
            aniso = 16;
        samp[s].Filter = (g_bAnisoEnable && aniso >= 2) ?
                             D3D12_FILTER_ANISOTROPIC :
                             D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samp[s].MaxAnisotropy = (g_bAnisoEnable && aniso >= 2) ? aniso : 1;
        samp[s].AddressU = samp[s].AddressV = samp[s].AddressW =
            D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samp[s].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        samp[s].MipLODBias =
            mipBias; // Artscout - 2026 (#78): >0 = blurrier mips = no far-terrain boil
        samp[s].MaxLOD = D3D12_FLOAT32_MAX;
        samp[s].ShaderRegister = s;
        samp[s].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    }

    // Artscout - 2026 (#13): s2 = plain trilinear WRAP for the cloud noise volume. s0/s1 are ANISOTROPIC (for
    // the grazing-angle terrain) and carry a MipLODBias tuned for it -- neither is wanted on a noise volume:
    // aniso multiplies taps for nothing, and the bias would blur the cloud detail by a terrain setting. WRAP is
    // mandatory here, not cosmetic: the volume TILES, and the march relies on it repeating seamlessly.
    D3D12_STATIC_SAMPLER_DESC sampNoise;
    ZeroMemory(&sampNoise, sizeof(sampNoise));
    sampNoise.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampNoise.AddressU = sampNoise.AddressV = sampNoise.AddressW =
        D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampNoise.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    sampNoise.MaxAnisotropy = 1;
    sampNoise.MipLODBias = 0.0f;
    sampNoise.MaxLOD = D3D12_FLOAT32_MAX;
    sampNoise.ShaderRegister = 2;
    sampNoise.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_STATIC_SAMPLER_DESC sampAll[3] = {samp[0], samp[1], sampNoise};

    D3D12_ROOT_SIGNATURE_DESC rsd;
    ZeroMemory(&rsd, sizeof(rsd));
    rsd.NumParameters = 7;
    rsd.pParameters = params;
    rsd.NumStaticSamplers = 3;
    rsd.pStaticSamplers = sampAll;
    rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // Artscout - 2026 (#107 bindless): lets the shader index the bound heap itself. The SRV table stays, so
    // every existing draw is unaffected -- only the terrain reads through the heap.
    if (BindlessWanted())
        rsd.Flags |= D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

    ID3DBlob* blob = 0;
    ID3DBlob* err = 0;
    if (FAILED(D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1,
                                           &blob, &err)))
    {
        R12Log("[D3D12R] serialize root sig failed: %s\n",
               err ? (const char*)err->GetBufferPointer() : "?");
        if (err)
            err->Release();
        return false;
    }
    HRESULT hr = m_pDevice->CreateRootSignature(0, blob->GetBufferPointer(),
                                                blob->GetBufferSize(),
                                                IID_PPV_ARGS(&m_pRootSig));
    blob->Release();
    if (err)
        err->Release();
    if (FAILED(hr))
    {
        R12Log("[D3D12R] CreateRootSignature failed 0x%08X\n", (unsigned)hr);
        return false;
    }
    return true;
}

// Create one UPLOAD ring per frame for constants / vertices / indices, persistently mapped.
static ID3D12Resource* MakeUploadRing(ID3D12Device* dev, unsigned bytes,
                                      unsigned char** outCpu)
{
    D3D12_HEAP_PROPERTIES hp;
    ZeroMemory(&hp, sizeof(hp));
    hp.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC rd;
    ZeroMemory(&rd, sizeof(rd));
    rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    rd.Width = bytes;
    rd.Height = 1;
    rd.DepthOrArraySize = 1;
    rd.MipLevels = 1;
    rd.Format = DXGI_FORMAT_UNKNOWN;
    rd.SampleDesc.Count = 1;
    rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ID3D12Resource* res = 0;
    if (FAILED(dev->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd,
                                            D3D12_RESOURCE_STATE_GENERIC_READ,
                                            NULL, IID_PPV_ARGS(&res))))
        return 0;
    D3D12_RANGE noRead;
    noRead.Begin = 0;
    noRead.End = 0;
    if (FAILED(res->Map(0, &noRead, (void**)outCpu)) || !*outCpu)
    {
        res->Release();
        return 0;
    }
    return res;
}

bool D3D12Renderer::CreateConstantBuffers()
{
    for (int f = 0; f < kFrames; ++f)
    {
        m_pCbRing[f] = MakeUploadRing(m_pDevice, CB_RING_BYTES, &m_pCbCpu[f]);
        m_cbSize[f] = CB_RING_BYTES;
        m_pVbRing[f] = MakeUploadRing(m_pDevice, VB_RING_BYTES, &m_pVbCpu[f]);
        m_vbSize[f] = VB_RING_BYTES;
        m_pIbRing[f] = MakeUploadRing(m_pDevice, IB_RING_BYTES, &m_pIbCpu[f]);
        m_ibSize[f] = IB_RING_BYTES;
        if (!m_pCbRing[f] || !m_pVbRing[f] || !m_pIbRing[f])
        {
            R12Log("[D3D12R] ring create failed\n");
            return false;
        }
    }
    return true;
}

bool D3D12Renderer::CreateWhiteTexture()
{
    m_srvInc = m_pDevice->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_HEAP_PROPERTIES hpDef;
    ZeroMemory(&hpDef, sizeof(hpDef));
    hpDef.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC td;
    ZeroMemory(&td, sizeof(td));
    td.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    td.Width = 1;
    td.Height = 1;
    td.DepthOrArraySize = 1;
    td.MipLevels = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    if (FAILED(m_pDevice->CreateCommittedResource(
            &hpDef, D3D12_HEAP_FLAG_NONE, &td, D3D12_RESOURCE_STATE_COPY_DEST,
            NULL, IID_PPV_ARGS(&m_pWhiteTex))))
    {
        R12Log("[D3D12R] white tex failed\n");
        return false;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srv;
    ZeroMemory(&srv, sizeof(srv));
    srv.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.Texture2D.MipLevels = 1;

    // White SRV in a CPU (non-shader-visible) staging heap -- the copy source for untextured slots.
    D3D12_DESCRIPTOR_HEAP_DESC hs;
    ZeroMemory(&hs, sizeof(hs));
    hs.NumDescriptors = 3;
    hs.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    hs.Flags =
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // 2D white, 2DArray white, 3D white
    if (FAILED(m_pDevice->CreateDescriptorHeap(&hs,
                                               IID_PPV_ARGS(&m_pWhiteStaging))))
    {
        R12Log("[D3D12R] white staging failed\n");
        return false;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE wh =
        m_pWhiteStaging->GetCPUDescriptorHandleForHeapStart();
    m_pDevice->CreateShaderResourceView(m_pWhiteTex, &srv, wh);
    m_whiteSrvCpu = (unsigned __int64)wh.ptr;
    // Artscout - 2026: #13 -- a second view of the SAME 1x1 white, declared as a TEXTURE2DARRAY. t2 is the cloud
    // pass's scene depth, which the shader declares as Texture2DArray<float>; the table is copied for EVERY draw,
    // so the slot needs a stand-in whenever no depth is readable. It must match the shader's declared dimension --
    // feeding the Texture2D white there is a descriptor TYPE mismatch (undefined reads, debug-layer error).
    D3D12_SHADER_RESOURCE_VIEW_DESC sa = srv;
    sa.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    sa.Texture2DArray.MipLevels = 1;
    sa.Texture2DArray.ArraySize = 1;
    sa.Texture2DArray.FirstArraySlice = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE wa = wh;
    wa.ptr += m_srvInc;
    m_pDevice->CreateShaderResourceView(m_pWhiteTex, &sa, wa);
    m_whiteArraySrvCpu = (unsigned __int64)wa.ptr;

    // Artscout - 2026 (#13): a 1x1x1 TEXTURE3D white for t3 (the cloud noise volume). Same reasoning as the
    // array view above: the range is copied for every draw, so the slot must always hold a descriptor of the
    // dimension the shader declares (Texture3D<float>), even for the 99% of draws that never sample it.
    D3D12_RESOURCE_DESC t3d;
    ZeroMemory(&t3d, sizeof(t3d));
    t3d.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    t3d.Width = 1;
    t3d.Height = 1;
    t3d.DepthOrArraySize = 1;
    t3d.MipLevels = 1;
    t3d.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    t3d.SampleDesc.Count = 1; // MUST match the noise volume's format
    t3d.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    // NOT fatal, and that matters: this is an optional slot-filler for a branch only the clouds take. It used to
    // `return false`, which skipped the SRV RING creation below -- so one failed 1x1x1 texture left m_pSrvRing
    // null and the first splash-screen draw dereferenced it. An optional feature must never be able to take the
    // renderer down with it; on failure m_white3DSrvCpu stays 0 and FlushConstants falls back (see there).
    if (FAILED(m_pDevice->CreateCommittedResource(
            &hpDef, D3D12_HEAP_FLAG_NONE, &t3d, D3D12_RESOURCE_STATE_COPY_DEST,
            NULL, IID_PPV_ARGS(&m_pWhite3DTex))))
    {
        R12Log("[D3D12R] white 3D tex failed -- clouds will bind the 2D white "
               "at t3\n");
        m_pWhite3DTex = 0;
        m_white3DSrvCpu = 0;
    }
    else
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC s3;
        ZeroMemory(&s3, sizeof(s3));
        s3.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        s3.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        s3.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        s3.Texture3D.MipLevels = 1;
        D3D12_CPU_DESCRIPTOR_HANDLE w3 = wh;
        w3.ptr += (SIZE_T)2 * m_srvInc;
        m_pDevice->CreateShaderResourceView(m_pWhite3DTex, &s3, w3);
        m_white3DSrvCpu = (unsigned __int64)w3.ptr;
    }

    // Per-frame shader-visible SRV rings (t0,t1 copied here per textured draw; root table points at the region).
    // MUST hold 2*(texture changes per frame): terrain draws hundreds of tile-texture groups across ~12 LODs
    // PLUS the screen path (2D/HUD/fonts) -- easily thousands. If this wraps mid-frame it overwrites slots a
    // not-yet-executed draw still references -> tiles sample each other's textures ("swapped tiles, random").
    m_srvRingCount =
        65536; // 32768 textured draws/frame; reset each frame (tier-1 heap cap is 1,000,000)

    // Artscout - 2026 (#107 bindless): a RESIDENT prefix in front of the ring, in EVERY frame's heap. Only one
    // CBV_SRV_UAV heap can be bound at a time, so the bindless array has to live inside the heap the draws
    // already use. The slot number is the same in each frame's heap, so a descriptor written to all of them
    // once stays valid whichever heap is bound.
    m_bindlessBase = BindlessWanted() ? kBindlessMax : 0;
    m_bindlessNext = 0;

    D3D12_DESCRIPTOR_HEAP_DESC hr;
    ZeroMemory(&hr, sizeof(hr));
    hr.NumDescriptors = m_srvRingCount + m_bindlessBase;
    hr.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    hr.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    for (int f = 0; f < kFrames; ++f)
        if (FAILED(m_pDevice->CreateDescriptorHeap(
                &hr, IID_PPV_ARGS(&m_pSrvRing[f]))))
        {
            R12Log("[D3D12R] SRV ring[%d] failed\n", f);
            return false;
        }

    // Upload buffer holding the single white texel (row pitch aligned to 256).
    D3D12_HEAP_PROPERTIES hpUp;
    ZeroMemory(&hpUp, sizeof(hpUp));
    hpUp.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bd.Width = 256;
    bd.Height = 1;
    bd.DepthOrArraySize = 1;
    bd.MipLevels = 1;
    bd.Format = DXGI_FORMAT_UNKNOWN;
    bd.SampleDesc.Count = 1;
    bd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    if (FAILED(m_pDevice->CreateCommittedResource(
            &hpUp, D3D12_HEAP_FLAG_NONE, &bd, D3D12_RESOURCE_STATE_GENERIC_READ,
            NULL, IID_PPV_ARGS(&m_pWhiteUpload))))
    {
        R12Log("[D3D12R] white upload failed\n");
        return false;
    }
    void* p = 0;
    D3D12_RANGE noRead;
    noRead.Begin = 0;
    noRead.End = 0;
    if (SUCCEEDED(m_pWhiteUpload->Map(0, &noRead, &p)) && p)
    {
        *(unsigned*)p = 0xFFFFFFFFu;
        m_pWhiteUpload->Unmap(0, NULL);
    }
    // #13: a second 256-byte upload holding one R8 white texel for the 1x1x1 3D stand-in.
    if (FAILED(m_pDevice->CreateCommittedResource(
            &hpUp, D3D12_HEAP_FLAG_NONE, &bd, D3D12_RESOURCE_STATE_GENERIC_READ,
            NULL, IID_PPV_ARGS(&m_pWhite3DUpload))))
    {
        R12Log("[D3D12R] white 3D upload failed (non-fatal)\n");
        m_pWhite3DUpload = 0;
    } // see above: never fatal
    void* p3 = 0;
    if (SUCCEEDED(m_pWhite3DUpload->Map(0, &noRead, &p3)) && p3)
    {
        *(unsigned*)p3 = 0xFFFFFFFFu;
        m_pWhite3DUpload->Unmap(0, NULL);
    }
    m_whiteUploaded =
        false; // the actual copy is recorded on the first frame (needs an open list)
    return true;
}

void D3D12Renderer::Release()
{
    ReleaseTerrainClipmap(); // #78
    if (m_pPsoCache)
    {
        PsoMap* m = (PsoMap*)m_pPsoCache;
        for (PsoMap::iterator it = m->begin(); it != m->end(); ++it)
            if (it->second)
                it->second->Release();
        delete m;
        m_pPsoCache = 0;
    }
    for (int f = 0; f < kFrames; ++f)
    {
        if (m_pCbRing[f])
        {
            m_pCbRing[f]->Unmap(0, NULL);
        }
        R12_RELEASE(m_pCbRing[f]);
        m_pCbCpu[f] = 0;
        if (m_pVbRing[f])
        {
            m_pVbRing[f]->Unmap(0, NULL);
        }
        R12_RELEASE(m_pVbRing[f]);
        m_pVbCpu[f] = 0;
        if (m_pIbRing[f])
        {
            m_pIbRing[f]->Unmap(0, NULL);
        }
        R12_RELEASE(m_pIbRing[f]);
        m_pIbCpu[f] = 0;
    }
    R12_RELEASE(m_pWhiteUpload);
    R12_RELEASE(m_pWhiteTex);
    R12_RELEASE(m_pWhite3DUpload);
    R12_RELEASE(m_pWhite3DTex);
    R12_RELEASE(m_pWhiteStaging);
    m_white3DSrvCpu = 0;
    // #13: destroy the noise volume and drop the pointer. Leaving it would both leak the pool region and let a
    // later Init hand the shader a pointer into a torn-down heap -- the exact class of bug that made the hero
    // explosions white squares (a texture pointer cached across a 3D exit). EnsureCloudNoise re-bakes on demand.
    if (m_pCloudNoise)
    {
        if (g_pD3D12TextureManager)
            g_pD3D12TextureManager->Destroy(*(D3D12Texture*)m_pCloudNoise);
        delete (D3D12Texture*)m_pCloudNoise;
        m_pCloudNoise = 0;
    }
    if (m_pCloudDetail)
    {
        if (g_pD3D12TextureManager)
            g_pD3D12TextureManager->Destroy(*(D3D12Texture*)m_pCloudDetail);
        delete (D3D12Texture*)m_pCloudDetail;
        m_pCloudDetail = 0;
    }
    R12_RELEASE(m_pLightCache);
    R12_RELEASE(m_pCsLightPso);
    R12_RELEASE(m_pCsRootSig);
    R12_RELEASE(m_pCsHeap);
    R12_RELEASE(m_pCsSrvStage);
    m_lightCacheSrvCpu = 0;
    m_lcValid = false;
    for (int f = 0; f < kFrames; ++f)
        R12_RELEASE(m_pSrvRing[f]);
    R12_RELEASE(m_pRootSig);
    // Artscout - 2026: #VFX Phase 1 -- release the static particle geometry + shader blobs.
    R12_RELEASE(m_pQuadVB);
    R12_RELEASE(m_pQuadIB);
    m_particleIbReady = false;
    if (m_pVSScreen)
    {
        ((IUnknown*)m_pVSScreen)->Release();
        m_pVSScreen = 0;
    }
    if (m_pVSObject)
    {
        ((IUnknown*)m_pVSObject)->Release();
        m_pVSObject = 0;
    }
    if (m_pPSMain)
    {
        ((IUnknown*)m_pPSMain)->Release();
        m_pPSMain = 0;
    }
    if (m_pVSParticle)
    {
        ((IUnknown*)m_pVSParticle)->Release();
        m_pVSParticle = 0;
    }
    if (m_pPSParticle)
    {
        ((IUnknown*)m_pPSParticle)->Release();
        m_pPSParticle = 0;
    }
    // #DX12 п.5: free the malloc'd DXIL VI blobs.
    for (int i = 0; i < VI_BLOB_COUNT; ++i)
    {
        if (m_viBlob[i])
        {
            free(m_viBlob[i]);
            m_viBlob[i] = 0;
            m_viBlobSize[i] = 0;
        }
    }
    m_viAvailable = false;
    m_pDevice = 0;
    m_valid = false;
}

//================================ state setters ==============================

void D3D12Renderer::SetViewportSize(int w, int h)
{
    m_screenW = w;
    m_screenH = h;
    m_dViewport = true;
}
void D3D12Renderer::SetView(const float* m)
{
    if (m)
    {
        memcpy(m_view, m, sizeof(m_view));
        m_dView = true;
        m_dStereo = true;
    }
} // #DX12 п.5: b5 derives from m_view
void D3D12Renderer::SetProj(const float* m)
{
    if (m)
    {
        memcpy(m_proj, m, sizeof(m_proj));
        m_dView = true;
        m_dStereo = true;
    }
} // #DX12 п.5: b5 derives from m_proj
void D3D12Renderer::SetWorld(const float* m)
{
    if (m)
    {
        memcpy(m_world, m, sizeof(m_world));
        m_dObject = true;
    }
}
void D3D12Renderer::SetCameraPos(float x, float y, float z)
{
    m_camPos[0] = x;
    m_camPos[1] = y;
    m_camPos[2] = z;
    m_dView = true;
}

// Artscout - 2026: #DX12 п.5 -- per-view deltas for the view-instanced pass (2 stereo / 4 quad). cbViewStereo(b5)
// is derived from these + the live base camera in FlushConstants. projs = per-view projections (quad off-axis),
// or NULL to use the live base m_proj for every view (symmetric stereo).
void D3D12Renderer::SetViewInstancingParams(int nViews, const float* worldOff,
                                            const float* projs)
{
    if (nViews < 1)
        nViews = 1;
    if (nViews > 4)
        nViews = 4;
    m_stereoViewCount = nViews;
    if (worldOff)
        for (int v = 0; v < nViews; ++v)
        {
            m_stereoWorldOff[v][0] = worldOff[v * 3 + 0];
            m_stereoWorldOff[v][1] = worldOff[v * 3 + 1];
            m_stereoWorldOff[v][2] = worldOff[v * 3 + 2];
        }
    m_stereoHasProj = (projs != 0);
    if (projs)
        for (int v = 0; v < nViews; ++v)
            memcpy(m_stereoProj[v], projs + v * 16, sizeof(m_stereoProj[v]));
    m_dStereo = true;
}

void D3D12Renderer::SetChromaKey(unsigned long argb, float tol)
{
    m_chromaKey = argb;
    m_chromaTol = tol;
    m_dRender = true;
}
void D3D12Renderer::SetFog(unsigned long argb, float s, float e)
{
    m_fogColor = argb;
    m_fogStart = s;
    m_fogEnd = e;
    m_dRender = true;
}
void D3D12Renderer::SetAlphaRef(float r)
{
    m_alphaRef = r;
    m_dRender = true;
}
void D3D12Renderer::SetMaterialColor(float r, float g, float b, float a)
{
    m_materialColor[0] = r;
    m_materialColor[1] = g;
    m_materialColor[2] = b;
    m_materialColor[3] = a;
    m_dRender = true;
}
void D3D12Renderer::SetMaterialSpecular(float r, float g, float b, float power)
{
    m_specular[0] = r;
    m_specular[1] = g;
    m_specular[2] = b;
    m_specular[3] = power;
    m_dRender = true;
}

void D3D12Renderer::SetTexture(unsigned slot, ID3D11ShaderResourceView* srv)
{
    // #DX12 п.1: under D3D12 'srv' is actually a D3D12Texture* (the opaque handle the texture manager stored
    // in TextureHandle::m_pDDS). Bind it to t0/t1; the descriptor copy into the shader-visible ring + the root
    // table happen at draw time (FlushConstants). NULL -> the 1x1 white default (untextured -> vertex colour).
    if (slot == 0)
    {
        if (m_pTex0 != (void*)srv)
        {
            m_pTex0 = (void*)srv;
            m_tableDirty = true;
        }
        unsigned nf = srv ? (m_flags | FF_TEXTURE0) : (m_flags & ~FF_TEXTURE0);
        if (nf != m_flags)
        {
            m_flags = nf;
            m_dRender = true;
        }
        m_hasTex0 = (srv != 0);
    }
    else if (slot == 1)
    {
        if (m_pTex1 != (void*)srv)
        {
            m_pTex1 = (void*)srv;
            m_tableDirty = true;
        }
    }
}

void D3D12Renderer::SetLights(const float ambient[4], int numLights,
                              const void* lights, int lightStride)
{
    // (Not on IRenderer -- terrain/dynamic2D don't light; kept for a future D3D12 object path. Fills the
    // cbLights shadow: ambient(16) + numLights(16) + up to 8 lights of 64 bytes.)
    const int MAXL = 8;
    if (numLights < 0)
        numLights = 0;
    if (numLights > MAXL)
        numLights = MAXL;
    memset(m_lightsBuf, 0, sizeof(m_lightsBuf));
    if (ambient)
        memcpy(m_lightsBuf, ambient, 16);
    *(unsigned*)(m_lightsBuf + 16) = (unsigned)numLights;
    if (numLights > 0 && lights)
        memcpy(m_lightsBuf + 32, lights,
               (size_t)numLights * (lightStride > 0 ? lightStride : 64));
    m_dLights = true;
}

void D3D12Renderer::SetState(int legacyState)
{
    FFStateDesc d;
    FFMapState(legacyState, d); // false -> SOLID fallback already filled

    unsigned nf = d.flags;
    if (m_texColorDiffuse)
    {
        if (legacyState == STATE_TEXTURE_TEXT ||
            legacyState == STATE_CHROMA_TEXTURE_GOURAUD2)
            nf |= FF_TEXCOLORDIFFUSE;
        m_texColorDiffuse = false;
    }
    if (!m_hasTex0)
        nf &=
            ~FF_TEXTURE0; // no real texture -> draw by vertex colour (see D3D11 SetState)
    if (m_cockpitPass)
        nf |= FF_COCKPIT;
    if (m_irGrey)
        nf |=
            FF_IRGREY; // #DX12 A5: sensor pass -> grey (sticky like m_cockpitPass)
    // Artscout - 2026: #97 NVG -- the RTT display composite (HUD/MFD/DED/RWR, FF_RTTSOFT) is not a world pass, so it
    // wouldn't otherwise green. Through the goggles the MFDs read green too -> OR FF_NVG onto the RTT composite.
    if (m_nvg and (nf & FF_RTTSOFT))
        nf |= FF_NVG;

    if (nf != m_flags)
    {
        m_flags = nf;
        m_dRender = true;
    }

    // D3D12: blend/depth/raster are baked into the PSO -> shadow them; the PSO is resolved at draw time.
    m_blend = d.blend;
    m_depthWrite = d.depthWrite;
    m_depthTest = d.depthTest;
    m_curState = legacyState;
}

// State toggles that only affect shader feature bits / the PSO shadow.
void D3D12Renderer::SetObjectAlphaBlend(bool on)
{
    m_blend = on ? BLEND_ALPHA : BLEND_OPAQUE;
    m_depthWrite = !on;
    m_depthTest = true;
}
void D3D12Renderer::SetObjectAdditiveBlend(bool on)
{
    m_blend = on ? BLEND_ADDITIVE : BLEND_ALPHA;
}
void D3D12Renderer::SetAlphaTestEnabled(bool on)
{
    unsigned nf = on ? (m_flags | FF_ALPHATEST) : (m_flags & ~FF_ALPHATEST);
    if (nf != m_flags)
    {
        m_flags = nf;
        m_dRender = true;
    }
}
void D3D12Renderer::SetEmissive(bool on)
{
    unsigned nf = on ? (m_flags | (1u << 11)) : (m_flags & ~(1u << 11));
    if (nf != m_flags)
    {
        m_flags = nf;
        m_dRender = true;
    }
}
void D3D12Renderer::SetAfterburner(bool on)
{
    unsigned nf = on ? (m_flags | (1u << 12)) : (m_flags & ~(1u << 12));
    if (nf != m_flags)
    {
        m_flags = nf;
        m_dRender = true;
    }
}
void D3D12Renderer::SetCockpitPass(bool on)
{
    m_cockpitPass = on;
    unsigned nf = on ? (m_flags | FF_COCKPIT) : (m_flags & ~FF_COCKPIT);
    if (nf != m_flags)
    {
        m_flags = nf;
        m_dRender = true;
    }
}
void D3D12Renderer::SetIRGrey(bool on)
{
    m_irGrey = on;
    unsigned nf = on ? (m_flags | FF_IRGREY) : (m_flags & ~FF_IRGREY);
    if (nf != m_flags)
    {
        m_flags = nf;
        m_dRender = true;
    }
}
// Artscout - 2026: #97 NVG. Sticky mode flag; the world Begin*Pass funcs OR FF_NVG into their base flags so the PS
// greens terrain/objects/cockpit/sky. 2D UI / RTT displays use other passes -> stay in their own colour.
void D3D12Renderer::SetNvgMode(bool on)
{
    m_nvg = on;
}
// Artscout - 2026: #97 unlit/full-bright. OR'd into cb.flags for every draw while set -> wrap only the exit-menu
// dialog draw so a lit 3D-BSP menu renders at full material colour instead of the (dark, at night) TOD lighting.
void D3D12Renderer::SetFullBright(bool on)
{
    m_fullBright = on;
}
void D3D12Renderer::SetTexColorDiffuse(bool on)
{
    m_texColorDiffuse = on;
}
void D3D12Renderer::SetForcePerSample(bool on)
{
    m_forcePerSample = on;
}
void D3D12Renderer::SetStencil(int, unsigned)
{ /* cockpit-bit (0x40) stencil path = later increment */
}
// #76 HUD aperture stencil: the glass plate marks bit 0x80 (MARK), the HUD symbology draws only where it is set
// (TEST) so the collimated HUD is clipped to the combiner aperture instead of spilling past the frame. GetPSO
// bakes the stencil state per m_hudStencil; the ref (0x80) is set at draw time (OMSetStencilRef). Needs a D24S8
// depth-stencil (DepthFormat) + the DSV bound -- true during the RTT composite (eye DSV).
void D3D12Renderer::SetHudStencil(int mode)
{
    m_hudStencil = mode;
}

//================================ frame / constants ==========================

ID3D12GraphicsCommandList* D3D12Renderer::Cmd()
{
    if (!g_pD3D12Backend || !g_pD3D12Backend->IsRecording())
        return 0;
    return g_pD3D12Backend->GetCommandList();
}

// Artscout - 2026 (#13): the ONE place that describes the frame -- and the cloud -- to the GPU. It was inline
// in FlushConstants; CS_LightCache needs the identical cbRender, because it shares CloudDensity with the
// pixel shader. Two copies of these numbers drifting apart would put the cache and the draw on different
// clouds, and "two sources of truth, one silently wrong" is the shape of most of this session's lost time.
// void* because CBRender is file-local to this .cpp and does not belong in the header.
void D3D12Renderer::FillRenderCB(void* pCb)
{
    CBRender& cb = *(CBRender*)pCb;
    ZeroMemory(&cb, sizeof(cb));
    // #DX12 A5: OR the sensor grey flag here so it reaches BOTH the object/screen path (SetState already ORs
    // it) AND the GPU-terrain path (BeginTerrainPass/DrawTerrainMesh set m_flags directly, bypassing SetState).
    cb.flags = m_flags | (m_irGrey ? FF_IRGREY : 0u) |
               (m_fullBright ? FF_FULLBRIGHT : 0u);
    cb.alphaRef = m_alphaRef;
    cb.fogStart = m_fogStart;
    cb.fogEnd = m_fogEnd;
    ArgbToRgba(m_fogColor, cb.fogColor);
    ArgbToRgba(m_chromaKey, cb.chromaKey, m_chromaTol);
    memcpy(cb.materialColor, m_materialColor, sizeof(cb.materialColor));
    memcpy(cb.specular, m_specular, sizeof(cb.specular));
    cb.waterParams[0] = (float)(GetTickCount() % 1000000) * 0.001f;
    memcpy(cb.gloc, m_gloc, sizeof(cb.gloc));
}

void D3D12Renderer::BeginFrameStateIfNeeded()
{
    // #DX12 п.5: reset on the backend RENDER EPOCH (bumped per BeginFrame AND per VR eye), not the swap-chain
    // index -- both eyes share one swap-chain index but are separate render passes (each fence-waited by
    // EndEyeFrame, so reusing the same ring slot is safe). The ring SLOT is still the swap-chain index (0/1)
    // so the desktop path stays double-buffered.
    unsigned epoch = g_pD3D12Backend->RenderEpoch();
    if (epoch == m_curEpoch)
        return;
    m_curEpoch = epoch;
    m_curFrame = g_pD3D12Backend->FrameIndex();
    unsigned fi = m_curFrame;
    if (fi < (unsigned)kFrames)
    {
        m_cbOff[fi] = 0;
        m_vbOff[fi] = 0;
        m_ibOff[fi] = 0;
        m_clipUploadOff[fi] = 0; // #78 clipmap staging ring
        m_srvRingOff[fi] = m_bindlessBase;
    }
    m_frameRebind = true;
}

void D3D12Renderer::EnsureWhiteTexture()
{
    if (m_whiteUploaded)
        return;
    if (!m_pWhiteTex || !m_pWhiteUpload)
        return; // defensive: never record a copy/barrier with a null resource
    ID3D12GraphicsCommandList* cl = Cmd();
    if (!cl)
        return;

    D3D12_TEXTURE_COPY_LOCATION dstL;
    ZeroMemory(&dstL, sizeof(dstL));
    dstL.pResource = m_pWhiteTex;
    dstL.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstL.SubresourceIndex = 0;
    D3D12_TEXTURE_COPY_LOCATION srcL;
    ZeroMemory(&srcL, sizeof(srcL));
    srcL.pResource = m_pWhiteUpload;
    srcL.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcL.PlacedFootprint.Offset = 0;
    srcL.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srcL.PlacedFootprint.Footprint.Width = 1;
    srcL.PlacedFootprint.Footprint.Height = 1;
    srcL.PlacedFootprint.Footprint.Depth = 1;
    srcL.PlacedFootprint.Footprint.RowPitch = 256;
    cl->CopyTextureRegion(&dstL, 0, 0, 0, &srcL, NULL);

    D3D12_RESOURCE_BARRIER b;
    ZeroMemory(&b, sizeof(b));
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource = m_pWhiteTex;
    b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    b.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    cl->ResourceBarrier(1, &b);

    // #13: the same one-shot for the 1x1x1 3D stand-in (t3 when no noise volume is baked yet).
    if (m_pWhite3DTex && m_pWhite3DUpload)
    {
        D3D12_TEXTURE_COPY_LOCATION d3;
        ZeroMemory(&d3, sizeof(d3));
        d3.pResource = m_pWhite3DTex;
        d3.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        d3.SubresourceIndex = 0;
        D3D12_TEXTURE_COPY_LOCATION u3;
        ZeroMemory(&u3, sizeof(u3));
        u3.pResource = m_pWhite3DUpload;
        u3.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        u3.PlacedFootprint.Offset = 0;
        u3.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        u3.PlacedFootprint.Footprint.Width = 1;
        u3.PlacedFootprint.Footprint.Height = 1;
        u3.PlacedFootprint.Footprint.Depth = 1;
        u3.PlacedFootprint.Footprint.RowPitch = 256;
        cl->CopyTextureRegion(&d3, 0, 0, 0, &u3, NULL);
        D3D12_RESOURCE_BARRIER b3 = b;
        b3.Transition.pResource = m_pWhite3DTex;
        cl->ResourceBarrier(1, &b3);
    }
    m_whiteUploaded = true;
}

unsigned __int64 D3D12Renderer::AllocCB(const void* src, unsigned bytes)
{
    unsigned f = m_curFrame;
    if (f >= (unsigned)kFrames)
        return 0;
    unsigned off =
        AlignUp(m_cbOff[f], 256); // root CBVs need 256-byte alignment
    unsigned need = AlignUp(bytes, 256);
    if (off + need > m_cbSize[f])
    {
        static bool once = false;
        if (!once)
        {
            once = true;
            R12Log("[D3D12R] CB ring overflow\n");
        }
        return 0;
    }
    memcpy(m_pCbCpu[f] + off, src, bytes);
    m_cbOff[f] = off + need;
    return m_pCbRing[f]->GetGPUVirtualAddress() + off;
}

bool D3D12Renderer::AllocVB(const void* src, unsigned bytes, unsigned stride,
                            void* outVbv)
{
    unsigned f = m_curFrame;
    if (f >= (unsigned)kFrames)
        return false;
    unsigned off = AlignUp(m_vbOff[f], 16);
    if (off + bytes > m_vbSize[f])
    {
        static bool once = false;
        if (!once)
        {
            once = true;
            R12Log("[D3D12R] VB ring overflow\n");
        }
        return false;
    }
    memcpy(m_pVbCpu[f] + off, src, bytes);
    m_vbOff[f] = off + bytes;
    D3D12_VERTEX_BUFFER_VIEW* v = (D3D12_VERTEX_BUFFER_VIEW*)outVbv;
    v->BufferLocation = m_pVbRing[f]->GetGPUVirtualAddress() + off;
    v->SizeInBytes = bytes;
    v->StrideInBytes = stride;
    return true;
}

// Artscout - 2026: #VFX Phase 1 -- suballocate the per-instance particle stream from the SAME per-frame
// VB ring as AllocVB (mirrors its map/upload/versioning). Stride = one D3D12ParticleInstance.
bool D3D12Renderer::AllocInstanceVB(const void* data, int bytes, void* outVbv)
{
    if (!data || bytes <= 0)
        return false;
    return AllocVB(data, (unsigned)bytes,
                   (unsigned)sizeof(D3D12ParticleInstance), outVbv);
}

bool D3D12Renderer::AllocIB(const unsigned short* src, unsigned icount,
                            void* outIbv)
{
    unsigned f = m_curFrame;
    if (f >= (unsigned)kFrames)
        return false;
    unsigned bytes = icount * (unsigned)sizeof(unsigned short);
    unsigned off = AlignUp(m_ibOff[f], 16);
    if (off + bytes > m_ibSize[f])
    {
        static bool once = false;
        if (!once)
        {
            once = true;
            R12Log("[D3D12R] IB ring overflow\n");
        }
        return false;
    }
    memcpy(m_pIbCpu[f] + off, src, bytes);
    m_ibOff[f] = off + bytes;
    D3D12_INDEX_BUFFER_VIEW* v = (D3D12_INDEX_BUFFER_VIEW*)outIbv;
    v->BufferLocation = m_pIbRing[f]->GetGPUVirtualAddress() + off;
    v->SizeInBytes = bytes;
    v->Format = DXGI_FORMAT_R16_UINT;
    return true;
}

void D3D12Renderer::FlushConstants()
{
    ID3D12GraphicsCommandList* cl = Cmd();
    if (!cl)
        return;
    BeginFrameStateIfNeeded();

    if (m_frameRebind)
    {
        cl->SetGraphicsRootSignature(m_pRootSig);
        ID3D12DescriptorHeap* heaps[] = {m_pSrvRing[m_curFrame]};
        cl->SetDescriptorHeaps(1,
                               heaps); // this frame's shader-visible SRV ring
        m_dViewport = m_dView = m_dObject = m_dRender = m_dLights =
            true; // root args cleared by list Reset
        m_dStereo =
            true; // #DX12 п.5: re-bind cbViewStereo(b5) after a list reset
        m_tableDirty =
            true; // must (re)bind the SRV table into the new frame's ring
        m_frameRebind = false;
    }
    EnsureWhiteTexture();

    // Copy t0/t1 (or the white default) into the ring and point the root table (param 5) at them.
    if (m_tableDirty)
    {
        unsigned f = m_curFrame;
        unsigned off = m_srvRingOff[f];
        // Artscout - 2026 (#13): SRV_PER_DRAW, not a literal. This said 4 while the table had grown to t0..t4 and
        // now t0..t5 -- so every draw wrote two descriptors PAST the slots it reserved, into the next draw's.
        // A silent heap overrun, and mine: I added t3, then t4, and never came back to the stride they are
        // allocated with. The root signature's NumDescriptors and this number are the same fact stated twice.
        const unsigned SRV_PER_DRAW =
            6; // t0,t1,t2,t3,t4,t5 -- MUST match srvRange.NumDescriptors
        // On overflow do NOT wrap to 0 (that would corrupt earlier, not-yet-executed draws -> "swapped tiles").
        // Clamp to the last slot: only the overflowing tail aliases (visually wrong there, but the bulk is correct).
        // The ring proper starts past the resident bindless prefix.
        if (off + SRV_PER_DRAW > m_bindlessBase + m_srvRingCount)
        {
            static bool once = false;
            if (!once)
            {
                once = true;
                R12Log("[D3D12R] SRV ring overflow (raise m_srvRingCount)\n");
            }
            off = m_bindlessBase + m_srvRingCount - SRV_PER_DRAW;
        }
        if (!m_pSrvRing[f])
            return; // renderer init failed short of the rings -- draw nothing, do not AV
        D3D12_CPU_DESCRIPTOR_HANDLE dst =
            m_pSrvRing[f]->GetCPUDescriptorHandleForHeapStart();
        dst.ptr += (SIZE_T)off * m_srvInc;
        // Artscout - 2026: guard against a stale t0/t1 -- m_pTex0/1 are raw pointers into the texture pool and
        // are NOT cleared when their D3D12Texture is freed/recycled (the pool sets srvCpuPtr to -1). A leftover
        // binding (e.g. particles bind only t0, so t1 is whatever the previous draw left) with a since-freed
        // texture would feed CopyDescriptorsSimple a -1 source handle -> AV inside the driver (#nvwgf2umx crash
        // in FlushConstants). Fall back to the white default for any invalid handle. (Proper fix: clear m_pTex*
        // on texture free.)
        SIZE_T p0 = m_pTex0 ? (SIZE_T)((D3D12Texture*)m_pTex0)->srvCpuPtr :
                              (SIZE_T)m_whiteSrvCpu;
        SIZE_T p1 = m_pTex1 ? (SIZE_T)((D3D12Texture*)m_pTex1)->srvCpuPtr :
                              (SIZE_T)m_whiteSrvCpu;
        if (p0 == 0 || p0 == (SIZE_T)-1)
            p0 = (SIZE_T)m_whiteSrvCpu;
        if (p1 == 0 || p1 == (SIZE_T)-1)
            p1 = (SIZE_T)m_whiteSrvCpu;
        D3D12_CPU_DESCRIPTOR_HANDLE s0;
        s0.ptr = p0;
        D3D12_CPU_DESCRIPTOR_HANDLE s1;
        s1.ptr = p1;
        m_pDevice->CopyDescriptorsSimple(
            1, dst, s0, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_CPU_DESCRIPTOR_HANDLE dst1 = dst;
        dst1.ptr += m_srvInc;
        m_pDevice->CopyDescriptorsSimple(
            1, dst1, s1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        // Artscout - 2026: #13 t2 = scene depth for the cloud raymarch. The backend hands back a depth SRV that
        // MATCHES the currently-bound scene depth (MS vs MS-array), or 0 when there is none / it is still bound
        // for writing -- then a dummy keeps the descriptor valid, since the range is copied for every draw.
        SIZE_T p2 =
            (SIZE_T)(g_pD3D12Backend ? g_pD3D12Backend->SceneDepthSrvCpu() : 0);
        if (p2 == 0 || p2 == (SIZE_T)-1)
            p2 = (SIZE_T)
                m_whiteArraySrvCpu; // MUST be the ARRAY view (see EnsureWhite*)
        D3D12_CPU_DESCRIPTOR_HANDLE s2;
        s2.ptr = p2;
        D3D12_CPU_DESCRIPTOR_HANDLE dst2 = dst;
        dst2.ptr += (SIZE_T)2 * m_srvInc;
        m_pDevice->CopyDescriptorsSimple(
            1, dst2, s2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        // Artscout - 2026: #13 t3 = the baked cloud noise volume; the 1x1x1 3D white stands in until it is baked
        // (and for every non-cloud draw, which never samples it). MUST be the TEXTURE3D view -- see CreateWhiteTexture.
        SIZE_T p3 = m_pCloudNoise ?
                        (SIZE_T)((D3D12Texture*)m_pCloudNoise)->srvCpuPtr :
                        (SIZE_T)m_white3DSrvCpu;
        if (p3 == 0 || p3 == (SIZE_T)-1)
            p3 = (SIZE_T)m_white3DSrvCpu;
        // The FALLBACK ITSELF can be 0 (the 3D stand-in is optional now) -- and CopyDescriptorsSimple from a null
        // handle is an access violation, which is exactly how this crashed. The 2D white is the wrong DIMENSION
        // for a Texture3D slot, so the debug layer will grumble and the cloud branch would read nonsense -- but
        // that branch is not running if we got here without a 3D white, and a validation warning beats a crash.
        if (p3 == 0)
            p3 = (SIZE_T)m_whiteSrvCpu;
        if (p3 == 0)
            return; // nothing valid to bind at all -- skip the draw rather than fault
        D3D12_CPU_DESCRIPTOR_HANDLE s3h;
        s3h.ptr = p3;
        D3D12_CPU_DESCRIPTOR_HANDLE dst3 = dst;
        dst3.ptr += (SIZE_T)3 * m_srvInc;
        m_pDevice->CopyDescriptorsSimple(
            1, dst3, s3h, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        // Artscout - 2026 (#13): t4 = the up-rez DETAIL noise volume. Same stand-in rules as t3.
        SIZE_T p4 = m_pCloudDetail ?
                        (SIZE_T)((D3D12Texture*)m_pCloudDetail)->srvCpuPtr :
                        (SIZE_T)m_white3DSrvCpu;
        if (p4 == 0 || p4 == (SIZE_T)-1)
            p4 = (SIZE_T)m_white3DSrvCpu;
        if (p4 == 0)
            p4 = (SIZE_T)m_whiteSrvCpu;
        if (p4 == 0)
            return;
        D3D12_CPU_DESCRIPTOR_HANDLE s4h;
        s4h.ptr = p4;
        D3D12_CPU_DESCRIPTOR_HANDLE dst4 = dst;
        dst4.ptr += (SIZE_T)4 * m_srvInc;
        m_pDevice->CopyDescriptorsSimple(
            1, dst4, s4h, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        // Artscout - 2026 (#13): t5 = the light cache. Its SRV is a Texture3D<float2>; the 3D white stands in
        // until the first dispatch (and for every non-cloud draw, which never samples it).
        SIZE_T p5 =
            m_lightCacheSrvCpu ? m_lightCacheSrvCpu : (SIZE_T)m_white3DSrvCpu;
        if (p5 == 0 || p5 == (SIZE_T)-1)
            p5 = (SIZE_T)m_white3DSrvCpu;
        if (p5 == 0)
            p5 = (SIZE_T)m_whiteSrvCpu;
        if (p5 == 0)
            return;
        D3D12_CPU_DESCRIPTOR_HANDLE s5h;
        s5h.ptr = p5;
        D3D12_CPU_DESCRIPTOR_HANDLE dst5 = dst;
        dst5.ptr += (SIZE_T)5 * m_srvInc;
        m_pDevice->CopyDescriptorsSimple(
            1, dst5, s5h, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_GPU_DESCRIPTOR_HANDLE gpu =
            m_pSrvRing[f]->GetGPUDescriptorHandleForHeapStart();
        gpu.ptr += (UINT64)off * m_srvInc;
        cl->SetGraphicsRootDescriptorTable(5, gpu);
        m_srvRingOff[f] = off + SRV_PER_DRAW;
        m_tableDirty = false;
    }

    if (m_dViewport)
    {
        CBViewport v = {(float)m_screenW, (float)m_screenH, 0, 0};
        unsigned __int64 va = AllocCB(&v, sizeof(v));
        if (va)
            cl->SetGraphicsRootConstantBufferView(0, va);
        m_dViewport = false;
    }
    if (m_dView)
    {
        CBView v;
        memcpy(v.view, m_view, sizeof(v.view));
        memcpy(v.proj, m_proj, sizeof(v.proj));
        v.camPos[0] = m_camPos[0];
        v.camPos[1] = m_camPos[1];
        v.camPos[2] = m_camPos[2];
        v.camPos[3] = 1.0f;
        unsigned __int64 va = AllocCB(&v, sizeof(v));
        if (va)
            cl->SetGraphicsRootConstantBufferView(1, va);
        m_dView = false;
    }
    if (m_dObject)
    {
        unsigned __int64 va = AllocCB(m_world, sizeof(m_world));
        if (va)
            cl->SetGraphicsRootConstantBufferView(2, va);
        m_dObject = false;
    }
    if (m_dRender)
    {
        CBRender cb;
        FillRenderCB(&cb);
        unsigned __int64 va = AllocCB(&cb, sizeof(cb));
        if (va)
            cl->SetGraphicsRootConstantBufferView(3, va);
        m_dRender = false;
    }
    if (m_dLights)
    {
        unsigned __int64 va = AllocCB(m_lightsBuf, sizeof(m_lightsBuf));
        if (va)
            cl->SetGraphicsRootConstantBufferView(4, va);
        m_dLights = false;
    }
    // #DX12 п.5: cbViewStereo(b5) -- only meaningful while a view-instanced pass is active (the VI PSOs read it).
    // Layout MUST match HLSL cbViewStereo { float4x4 gView2[4]; float4x4 gProj2[4]; float4 gCamPos2; }.
    // Each view = the LIVE base rotation view (m_view) with its translation row shifted by the per-view world eye
    // offset rotated into view space (permutation-agnostic). Projection = per-view m_stereoProj[v] (quad off-axis)
    // or the live base m_proj (symmetric stereo). nViews unused slots are filled with view 0 (harmless).
    if (m_stereoActive &&
        m_dStereo) // build b5 ONLY when changed (SetView/SetProj/params) -- per-draw overflowed CB ring
    {
        struct CBViewStereo
        {
            float view[4][16];
            float proj[4][16];
            float camPos[4];
        } vs;
        const int n = (m_stereoViewCount >= 1 && m_stereoViewCount <= 4) ?
                          m_stereoViewCount :
                          2;
        for (int e = 0; e < 4; ++e)
        {
            int s = (e < n) ? e : 0; // fill unused slots with view 0
            memcpy(vs.view[e], m_view,
                   sizeof(m_view)); // base rotation view (row-major)
            const float* w = m_stereoWorldOff[s];
            float ovx = w[0] * m_view[0] + w[1] * m_view[4] + w[2] * m_view[8];
            float ovy = w[0] * m_view[1] + w[1] * m_view[5] + w[2] * m_view[9];
            float ovz = w[0] * m_view[2] + w[1] * m_view[6] + w[2] * m_view[10];
            vs.view[e][12] -= ovx;
            vs.view[e][13] -= ovy;
            vs.view[e][14] -= ovz; // translation row -= view-space eye offset
            memcpy(vs.proj[e], m_stereoHasProj ? m_stereoProj[s] : m_proj,
                   sizeof(m_proj));
        }
        vs.camPos[0] = m_camPos[0];
        vs.camPos[1] = m_camPos[1];
        vs.camPos[2] = m_camPos[2];
        vs.camPos[3] = 1.0f;
        unsigned __int64 va = AllocCB(&vs, sizeof(vs));
        if (va)
            cl->SetGraphicsRootConstantBufferView(6, va);
        m_dStereo = false;
    }
}

//================================ PSO cache ==================================

// Artscout - 2026: #DX12 п.5 -- create a PSO WITH view instancing (2 views -> the rasterizer replicates each
// primitive to RT-array slices 0 and 1). CreateGraphicsPipelineState has no view-instancing field, so this
// rebuilds the same state as a D3D12_PIPELINE_STATE_STREAM (ID3D12Device2::CreatePipelineState) and appends a
// D3D12_VIEW_INSTANCING subobject. The classic desc `pd` (VS/PS already pointing at DXIL) supplies every other
// piece of state so the VI PSO matches its non-VI sibling exactly.
static ID3D12PipelineState*
CreateViPso(ID3D12Device* dev, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& pd,
            unsigned key, int nViews)
{
    ID3D12Device2* dev2 = 0;
    if (FAILED(dev->QueryInterface(IID_PPV_ARGS(&dev2))) || !dev2)
    {
        R12Log("[D3D12R] VI PSO: ID3D12Device2 unavailable (key 0x%X)\n", key);
        if (dev2)
            dev2->Release();
        return 0;
    }

    // nViews view-instance locations: view v -> RT slice v (no viewport array indexing). 2 = stereo, 4 = quad.
    if (nViews < 1)
        nViews = 1;
    if (nViews > 4)
        nViews = 4;
    D3D12_VIEW_INSTANCE_LOCATION loc[4];
    for (int v = 0; v < nViews; ++v)
    {
        loc[v].ViewportArrayIndex = 0;
        loc[v].RenderTargetArrayIndex = (UINT)v;
    }
    D3D12_VIEW_INSTANCING_DESC vi;
    vi.ViewInstanceCount = (UINT)nViews;
    vi.pViewInstanceLocations = loc;
    vi.Flags = D3D12_VIEW_INSTANCING_FLAG_ENABLE_VIEW_INSTANCE_MASKING;

    D3D12_RT_FORMAT_ARRAY rtf;
    ZeroMemory(&rtf, sizeof(rtf));
    rtf.NumRenderTargets = pd.NumRenderTargets;
    for (UINT i = 0; i < pd.NumRenderTargets && i < 8; ++i)
        rtf.RTFormats[i] = pd.RTVFormats[i];

    // Packed stream. `alignas(void*)` on each member gives the required per-subobject alignment.
    struct StreamT
    {
        struct alignas(void*)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE t;
            ID3D12RootSignature* v;
        } rootSig;
        struct alignas(void*)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE t;
            D3D12_INPUT_LAYOUT_DESC v;
        } il;
        struct alignas(void*)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE t;
            D3D12_SHADER_BYTECODE v;
        } vs;
        struct alignas(void*)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE t;
            D3D12_SHADER_BYTECODE v;
        } ps;
        struct alignas(void*)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE t;
            D3D12_BLEND_DESC v;
        } blend;
        struct alignas(void*)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE t;
            UINT v;
        } sampleMask;
        struct alignas(void*)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE t;
            D3D12_RASTERIZER_DESC v;
        } raster;
        struct alignas(void*)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE t;
            D3D12_DEPTH_STENCIL_DESC v;
        } ds;
        struct alignas(void*)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE t;
            DXGI_FORMAT v;
        } dsvFmt;
        struct alignas(void*)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE t;
            D3D12_PRIMITIVE_TOPOLOGY_TYPE v;
        } topo;
        struct alignas(void*)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE t;
            D3D12_RT_FORMAT_ARRAY v;
        } rtFmts;
        struct alignas(void*)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE t;
            DXGI_SAMPLE_DESC v;
        } sampleDesc;
        struct alignas(void*)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE t;
            D3D12_VIEW_INSTANCING_DESC v;
        } viewInst;
    } s;
    s.rootSig.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
    s.rootSig.v = pd.pRootSignature;
    s.il.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT;
    s.il.v = pd.InputLayout;
    s.vs.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS;
    s.vs.v = pd.VS;
    s.ps.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS;
    s.ps.v = pd.PS;
    s.blend.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND;
    s.blend.v = pd.BlendState;
    s.sampleMask.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK;
    s.sampleMask.v = pd.SampleMask;
    s.raster.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER;
    s.raster.v = pd.RasterizerState;
    s.ds.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL;
    s.ds.v = pd.DepthStencilState;
    s.dsvFmt.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT;
    s.dsvFmt.v = pd.DSVFormat;
    s.topo.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY;
    s.topo.v = pd.PrimitiveTopologyType;
    s.rtFmts.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS;
    s.rtFmts.v = rtf;
    s.sampleDesc.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC;
    s.sampleDesc.v = pd.SampleDesc;
    s.viewInst.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING;
    s.viewInst.v = vi;

    D3D12_PIPELINE_STATE_STREAM_DESC sd;
    sd.SizeInBytes = sizeof(s);
    sd.pPipelineStateSubobjectStream = &s;
    ID3D12PipelineState* pso = 0;
    HRESULT hr = dev2->CreatePipelineState(&sd, IID_PPV_ARGS(&pso));
    dev2->Release();
    if (FAILED(hr))
    {
        R12Log("[D3D12R] VI PSO create failed 0x%08X (key 0x%X)\n",
               (unsigned)hr, key);
        return 0;
    }
    return pso;
}

// D3D_PRIMITIVE_TOPOLOGY_TYPE from our coarse topoType (0 point, 1 line, 2 triangle).
ID3D12PipelineState* D3D12Renderer::GetPSO(int pass, int blend, bool dWrite,
                                           bool dTest, int topoType, int cull,
                                           int bias)
{
    // CRITICAL: when the current render target has NO depth-stencil bound (the RTT display atlas -- BindSceneRtt
    // binds a null DSV for the 2D HUD/MFD/RWR/DED symbology), a D32 PSO + null DSV trips #615 and the runtime
    // DROPS the draw -> the whole display disappears. Force depth OFF here so the PSO's DSVFormat is UNKNOWN
    // (see the depthEnable branch below) and a null DSV is legal. Collapses to the depth-off cache key.
    if (!m_depthTargetBound)
    {
        dWrite = false;
        dTest = false;
    }

    // Artscout - 2026: MSAA -- the PSO's SampleDesc MUST match the currently-bound render target. The MSAA
    // scene target has >1 samples; the single-sample backbuffer / RTT atlas / menu have 1. The backend reports
    // the current target's count. Fold it into the cache key so both variants coexist.
    int samples = g_pD3D12Backend ? g_pD3D12Backend->CurrentSampleCount() : 1;
    if (samples < 1)
        samples = 1;

    // #DX12 п.5: view-instanced variant is a DISTINCT PSO (DXIL shaders + view-instancing subobject). The view
    // count (2 stereo / 4 quad) is part of the PSO -> fold it into the key so both coexist.
    bool stereo = m_stereoActive && m_viAvailable;
    unsigned key =
        ((unsigned)(pass & 1)) | ((unsigned)(blend & 3) << 1) |
        ((unsigned)(dWrite ? 1 : 0) << 3) | ((unsigned)(dTest ? 1 : 0) << 4) |
        ((unsigned)(topoType & 3) << 5) | ((unsigned)(cull & 3) << 7) |
        ((unsigned)(m_hudStencil & 3)
         << 10) // #76 HUD aperture stencil variant (0/1/2)
        | ((unsigned)(bias & 1) << 9) |
        ((unsigned)((bias >> 1) & 1)
         << 16) // #78 bias==2 (terrain) high bit -- distinct from 0/1
        | ((unsigned)(samples & 0xF) << 12) // MSAA sample-count variant
        | ((unsigned)(stereo ? 1 : 0) << 17) // #DX12 п.5 view-instanced variant
        | ((unsigned)((stereo && m_stereoViewCount == 4) ? 1 : 0)
           << 18); // quad (4-view) VI variant

    PsoMap* cache = (PsoMap*)m_pPsoCache;
    PsoMap::iterator it = cache->find(key);
    if (it != cache->end())
        return it->second;

    // Input layouts (screen = TLVERTEX; object = 40-byte ObjV) -- match FFEmu.hlsl VSInScreen / VSInObject.
    static const D3D12_INPUT_ELEMENT_DESC screenIL[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, 16,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 1, DXGI_FORMAT_B8G8R8A8_UNORM, 0, 20,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 32,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    static const D3D12_INPUT_ELEMENT_DESC objectIL[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, 24,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 1, DXGI_FORMAT_B8G8R8A8_UNORM, 0, 28,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 32,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    // Artscout - 2026 (#107 bindless): objectIL plus the tile slot at offset 40 -> 44-byte vertex.
    static const D3D12_INPUT_ELEMENT_DESC terrainBindlessIL[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, 24,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 1, DXGI_FORMAT_B8G8R8A8_UNORM, 0, 28,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 32,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 1, DXGI_FORMAT_R32_UINT, 0, 40,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pd;
    ZeroMemory(&pd, sizeof(pd));
    pd.pRootSignature = m_pRootSig;
    if (topoType == 3)
    {
        // Artscout - 2026 (#107 bindless): its own DXIL pair at 6_6 -- the only build of these shaders that
        // has ResourceDescriptorHeap and the extra vertex slot.
        void* vb = m_viBlob[VI_BINDLESS_VS];
        void* pb = m_viBlob[VI_BINDLESS_PS];

        if (!vb || !pb)
        {
            (*cache)[key] = 0;
            return 0;
        }

        pd.VS.pShaderBytecode = vb;
        pd.VS.BytecodeLength = m_viBlobSize[VI_BINDLESS_VS];
        pd.PS.pShaderBytecode = pb;
        pd.PS.BytecodeLength = m_viBlobSize[VI_BINDLESS_PS];
    }
    else if (stereo)
    {
        // #DX12 п.5: DXIL VI shaders. pass 1 = VS_ObjectVI (per-eye matrices); pass 0 = VS_Screen (sky, identical
        // both eyes). PS_Main must ALSO be DXIL (a PSO cannot mix DXBC and DXIL).
        void* vb = m_viBlob[pass == 1 ? VI_OBJECT : VI_SCREEN];
        void* pb = m_viBlob[VI_PSMAIN];
        if (!vb || !pb)
        {
            (*cache)[key] = 0;
            return 0;
        } // shouldn't happen (m_viAvailable gated), but be safe
        pd.VS.pShaderBytecode = vb;
        pd.VS.BytecodeLength = m_viBlobSize[pass == 1 ? VI_OBJECT : VI_SCREEN];
        pd.PS.pShaderBytecode = pb;
        pd.PS.BytecodeLength = m_viBlobSize[VI_PSMAIN];
    }
    else
    {
        ID3DBlob* vs = (ID3DBlob*)(pass == 1 ? m_pVSObject : m_pVSScreen);
        ID3DBlob* ps = (ID3DBlob*)m_pPSMain;
        // Artscout - 2026: a null blob here means a base shader failed to COMPILE. CompileShaders returns false
        // for that and Init propagates it -- but somebody up the chain does not check, so the game carried on and
        // dereferenced null on the very first splash-screen draw. An HLSL typo should not be a wild-pointer crash
        // three call levels away from the mistake; it should be a log line and a black screen.
        if (!vs || !ps)
        {
            static bool once = false;
            if (!once)
            {
                once = true;
                R12Log(
                    "[D3D12R] GetPSO: base shader blob is NULL -- FFEmu.hlsl "
                    "failed to compile; see the compile error above\n");
            }
            (*cache)[key] = 0;
            return 0;
        }
        pd.VS.pShaderBytecode = vs->GetBufferPointer();
        pd.VS.BytecodeLength = vs->GetBufferSize();
        pd.PS.pShaderBytecode = ps->GetBufferPointer();
        pd.PS.BytecodeLength = ps->GetBufferSize();
    }
    if (topoType == 3)
    {
        // Artscout - 2026 (#107 bindless): the object layout plus a per-vertex tile slot at offset 40.
        // Same 44-byte vertex the Vulkan path uses (TGpuVertBindless).
        pd.InputLayout.pInputElementDescs = terrainBindlessIL;
        pd.InputLayout.NumElements = _countof(terrainBindlessIL);
    }
    else if (pass == 1)
    {
        pd.InputLayout.pInputElementDescs = objectIL;
        pd.InputLayout.NumElements = _countof(objectIL);
    }
    else
    {
        pd.InputLayout.pInputElementDescs = screenIL;
        pd.InputLayout.NumElements = _countof(screenIL);
    }

    // Blend (mirror D3D11 CreateStateObjects): opaque / alpha / pure-additive (tracers).
    D3D12_RENDER_TARGET_BLEND_DESC& rt = pd.BlendState.RenderTarget[0];
    rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    if (blend == BLEND_OPAQUE)
    {
        rt.BlendEnable = FALSE;
    }
    else
    {
        rt.BlendEnable = TRUE;
        // Artscout - 2026 (#13): BLEND_PREMUL = ONE / INV_SRC_ALPHA. Three modes now, and the third is the one
        // this engine has always been missing:
        //   ADDITIVE  ONE/ONE            -- src + dst. Cannot occlude ANYTHING, by arithmetic.
        //   ALPHA     SRC_ALPHA/INV      -- src*a + dst*(1-a). Needs rgb to be a straight COLOUR.
        //   PREMUL    ONE/INV_SRC_ALPHA  -- src + dst*(1-a). rgb is light ALREADY weighted by coverage.
        //   A volumetric march produces exactly the third thing: `scat` is accumulated as trans*a*lit, i.e.
        // premultiplied by construction. Pushing it through ALPHA multiplies by coverage a SECOND time, which is
        // why the cloud came out dark and only opaque at its core. Pushing it through ADDITIVE never occludes.
        // Neither mode was right and the engine had no third one, so I kept trading one wrong for the other.
        //   This is the same missing mode I flagged this morning for the sun's aureole and the moon halo -- they
        // need it for the same reason (their textures are premultiplied, and plain alpha squares the falloff).
        const bool premul = (blend == BLEND_PREMUL);
        rt.SrcBlend = (blend == BLEND_ADDITIVE || premul) ?
                          D3D12_BLEND_ONE :
                          D3D12_BLEND_SRC_ALPHA;
        rt.DestBlend = (blend == BLEND_ADDITIVE) ? D3D12_BLEND_ONE :
                                                   D3D12_BLEND_INV_SRC_ALPHA;
        rt.BlendOp = D3D12_BLEND_OP_ADD;
        rt.SrcBlendAlpha = D3D12_BLEND_ONE;
        rt.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    }

    // Depth. CRITICAL: a null DSV (the RTT display atlas + any 2D pass bind OMSetRenderTargets with NO depth)
    // is ONLY legal when the PSO's DSVFormat is UNKNOWN. Baking DSVFormat=D32 on a depth-OFF PSO trips
    // #615 DEPTH_STENCIL_FORMAT_MISMATCH on every such draw; on runtimes that DROP invalid draws this silently
    // erased the HUD/MFD RTT symbology + textured object passes ("RTT gone, missiles wireframe"). So the format
    // tracks depthEnable: D32 only when the pass actually tests/writes depth (then the eye/scene DSV is bound).
    bool depthEnable = (dTest || dWrite);
    pd.DepthStencilState.DepthEnable = depthEnable ? TRUE : FALSE;
    pd.DepthStencilState.DepthWriteMask =
        dWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    pd.DepthStencilState.DepthFunc =
        dTest ? D3D12_COMPARISON_FUNC_GREATER_EQUAL :
                D3D12_COMPARISON_FUNC_ALWAYS; // reversed-Z

    // #76 HUD aperture stencil (depth stays OFF; ref 0x80 set at draw time via OMSetStencilRef). MARK: EQUAL
    // with ReadMask 0x40 (draw where the cockpit bit is clear) -> REPLACE writes bit 0x80 (WriteMask 0x80).
    // TEST: EQUAL with ReadMask 0xC0 -> draw where (stencil & 0xC0) == 0x80 (aperture set, cockpit clear).
    if (m_hudStencil != 0)
    {
        pd.DepthStencilState.StencilEnable = TRUE;
        pd.DepthStencilState.StencilReadMask =
            (m_hudStencil == 1) ? 0x40 : 0xC0;
        pd.DepthStencilState.StencilWriteMask =
            (m_hudStencil == 1) ? 0x80 : 0x00;
        D3D12_DEPTH_STENCILOP_DESC so;
        so.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
        so.StencilPassOp = (m_hudStencil == 1) ? D3D12_STENCIL_OP_REPLACE :
                                                 D3D12_STENCIL_OP_KEEP;
        so.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        so.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        pd.DepthStencilState.FrontFace = so;
        pd.DepthStencilState.BackFace = so;
    }
    else
        pd.DepthStencilState.StencilEnable = FALSE;

    // The DSV format must be present when depth OR stencil is used (a null DSV is only legal with UNKNOWN).
    pd.DSVFormat = (depthEnable || m_hudStencil != 0) ?
                       (DXGI_FORMAT)D3D12Backend::DepthFormat() :
                       DXGI_FORMAT_UNKNOWN;

    // Raster.
    pd.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    pd.RasterizerState.CullMode = (cull == 1) ? D3D12_CULL_MODE_BACK :
                                  (cull == 2) ? D3D12_CULL_MODE_FRONT :
                                                D3D12_CULL_MODE_NONE;
    pd.RasterizerState.FrontCounterClockwise = FALSE;
    pd.RasterizerState.DepthClipEnable = TRUE;
    pd.RasterizerState.MultisampleEnable =
        (samples > 1) ? TRUE :
                        FALSE; // MSAA: edge AA on the multisample scene target
    if (bias == 1)
    {
        pd.RasterizerState.DepthBias = 100;
        pd.RasterizerState.SlopeScaledDepthBias = 0.0f;
    } // reversed-Z: +bias = toward camera   // #16 pull objects toward camera
    else if (
        bias ==
        2) // #78 terrain: reversed-Z NEGATIVE bias = AWAY from camera, so terrain sinks below coplanar
    { // objects/runway and stops the grazing-angle z-fight/see-through. From cfg (baked at PSO build).
        extern float g_fGpuTerrainSlopeBias, g_fGpuTerrainDepthBias;
        pd.RasterizerState.DepthBias = -(int)g_fGpuTerrainDepthBias;
        pd.RasterizerState.SlopeScaledDepthBias = -g_fGpuTerrainSlopeBias;
    }

    pd.PrimitiveTopologyType =
        (topoType == 0) ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT :
        (topoType == 1) ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE :
                          D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pd.SampleMask = 0xFFFFFFFFu;
    pd.NumRenderTargets = 1;
    pd.RTVFormats[0] = (DXGI_FORMAT)D3D12Backend::BackBufferFormat();
    pd.SampleDesc.Count =
        (UINT)samples; // MSAA: match the currently-bound target

    ID3D12PipelineState* pso = 0;
    if (stereo)
        pso = CreateViPso(m_pDevice, pd, key, m_stereoViewCount);
    else if (FAILED(m_pDevice->CreateGraphicsPipelineState(&pd,
                                                           IID_PPV_ARGS(&pso))))
        pso = 0;
    if (!pso)
    {
        R12Log("[D3D12R] PSO create failed (key 0x%X)\n", key);
        (*cache)[key] = 0;
        return 0;
    }
    (*cache)[key] = pso;
    return pso;
}

// Artscout - 2026: #VFX Phase 1 -- PSO for the GPU-instanced billboard particles. Distinct program
// (VS_Particle/PS_Particle) + a two-slot input layout (unit quad PER_VERTEX + instance PER_INSTANCE).
// Depth test yes / write no (glows sort behind opaque geometry but never occlude one another), cull none
// (either quad winding must draw), triangle-list. blendMode: 0 additive, 1 alpha. Cached in the shared
// PSO map on a disjoint key range (0x0200_0000+) so it can never collide with a GetPSO key.
ID3D12PipelineState* D3D12Renderer::GetParticlePSO(int blendMode)
{
    // Respect the null-DSV rule (see GetPSO): when no depth-stencil is bound, a depth-ON PSO + null DSV
    // trips #615 and the draw is dropped -> force depth OFF (DSVFormat UNKNOWN) in that case.
    bool depthOn = m_depthTargetBound;
    // blendMode: 0 = additive (glow), 1 = straight alpha, 2 = premultiplied alpha (EmberGen flipbooks -- one
    // blend does both the bright emissive fire AND the dark smoke).
    int bm = (blendMode >= 0 && blendMode <= 2) ? blendMode : 0;

    int samples = g_pD3D12Backend ? g_pD3D12Backend->CurrentSampleCount() :
                                    1; // MSAA: match the bound target
    if (samples < 1)
        samples = 1;
    // #DX12 п.5: view-instanced particle variant (DXIL + view-instancing). Requires the optional VI particle
    // blobs -- if they didn't compile, particles simply don't draw in the VI pass (safe degradation).
    bool stereo = m_stereoActive && m_viAvailable && m_viBlob[VI_PARTICLE_VS] &&
                  m_viBlob[VI_PSPARTICLE];
    // Artscout - 2026: #DX12 -- cache-key fields MUST NOT overlap; these did. samples was shifted <<5, so on a 4x
    // MSAA target (samples=4 -> 0x80) it set the VERY SAME bit as the quad-view flag, and 2x (0x40) aliased the
    // stereo flag. With MSAA enabled (display.xml msaa samples="4") the quad bit is therefore a no-op -- 0x80|0x80
    // == 0x80 -- so the 2-view and 4-view particle PSOs collapse onto ONE key and whichever is built first is handed
    // to the other. A pipeline whose ViewInstanceCount / sample count does not match the bound target renders
    // garbage: the hero explosion came out as flat squares. Which variant won depended on PSO caching order, which
    // is why unrelated extra draws (the sky glare) appeared to cause and "fix" it. GetPSO keys each field its own
    // bits and was never affected. Layout here: bm 0-1, depth 4, stereo 6, quad 7, samples 8-11, particle tag 25.
    unsigned key = 0x02000000u | ((unsigned)(bm & 3)) | (depthOn ? 0x10u : 0u) |
                   (stereo ? 0x40u : 0u) |
                   ((stereo && m_stereoViewCount == 4) ? 0x80u : 0u) |
                   ((unsigned)(samples & 0xF) << 8);
    if (m_stereoActive && !stereo)
        return 0; // VI pass but no VI particle PSO -> skip particles this frame
    PsoMap* cache = (PsoMap*)m_pPsoCache;
    PsoMap::iterator it = cache->find(key);
    if (it != cache->end())
        return it->second;

    // Slot 0 = static unit quad (PER_VERTEX); slot 1 = per-instance stream (PER_INSTANCE, step rate 1).
    // Semantics/formats MUST match VSInParticleVtx/VSInParticleInst (FFEmu.hlsl) and D3D12ParticleInstance.
    static const D3D12_INPUT_ELEMENT_DESC particleIL[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0,
         D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1}, // center
        {"TEXCOORD", 2, DXGI_FORMAT_R32G32_FLOAT, 1, 12,
         D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1}, // size
        {"TEXCOORD", 3, DXGI_FORMAT_R32_FLOAT, 1, 20,
         D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1}, // rot
        {"COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 1, 24,
         D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,
         1}, // color (D3DCOLOR ARGB)
        {"TEXCOORD", 4, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 28,
         D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1}, // uvRect
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pd;
    ZeroMemory(&pd, sizeof(pd));
    pd.pRootSignature = m_pRootSig;
    if (stereo)
    {
        pd.VS.pShaderBytecode = m_viBlob[VI_PARTICLE_VS];
        pd.VS.BytecodeLength = m_viBlobSize[VI_PARTICLE_VS];
        pd.PS.pShaderBytecode = m_viBlob[VI_PSPARTICLE];
        pd.PS.BytecodeLength = m_viBlobSize[VI_PSPARTICLE];
    }
    else
    {
        ID3DBlob* vs = (ID3DBlob*)m_pVSParticle;
        ID3DBlob* ps = (ID3DBlob*)m_pPSParticle;
        if (!vs || !ps)
        {
            (*cache)[key] = 0;
            return 0;
        }
        pd.VS.pShaderBytecode = vs->GetBufferPointer();
        pd.VS.BytecodeLength = vs->GetBufferSize();
        pd.PS.pShaderBytecode = ps->GetBufferPointer();
        pd.PS.BytecodeLength = ps->GetBufferSize();
    }
    pd.InputLayout.pInputElementDescs = particleIL;
    pd.InputLayout.NumElements = _countof(particleIL);

    // Blend: 0 additive = ONE,ONE (glow); 1 straight alpha = SRC_ALPHA,INV_SRC_ALPHA; 2 premultiplied =
    // ONE,INV_SRC_ALPHA (color already * alpha -- correct for EmberGen fire+smoke in one sheet).
    D3D12_RENDER_TARGET_BLEND_DESC& rt = pd.BlendState.RenderTarget[0];
    rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    rt.BlendEnable = TRUE;
    rt.SrcBlend = (bm == 1) ? D3D12_BLEND_SRC_ALPHA :
                              D3D12_BLEND_ONE; // additive & premult use ONE
    rt.DestBlend = (bm == 0) ? D3D12_BLEND_ONE :
                               D3D12_BLEND_INV_SRC_ALPHA; // additive uses ONE
    rt.BlendOp = D3D12_BLEND_OP_ADD;
    rt.SrcBlendAlpha = D3D12_BLEND_ONE;
    rt.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;

    // Depth: TEST yes, WRITE no (particles are transparent -> they read the scene depth but must not
    // occlude each other). DSVFormat present only when a DSV is bound (else null-DSV #615).
    pd.DepthStencilState.DepthEnable = depthOn ? TRUE : FALSE;
    pd.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    pd.DepthStencilState.DepthFunc =
        depthOn ? D3D12_COMPARISON_FUNC_GREATER_EQUAL :
                  D3D12_COMPARISON_FUNC_ALWAYS; // reversed-Z
    pd.DepthStencilState.StencilEnable = FALSE;
    pd.DSVFormat = depthOn ? (DXGI_FORMAT)D3D12Backend::DepthFormat() :
                             DXGI_FORMAT_UNKNOWN;

    pd.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    pd.RasterizerState.CullMode =
        D3D12_CULL_MODE_NONE; // camera-facing quad: draw regardless of winding
    pd.RasterizerState.FrontCounterClockwise = FALSE;
    pd.RasterizerState.DepthClipEnable = TRUE;
    pd.RasterizerState.MultisampleEnable =
        (samples > 1) ? TRUE : FALSE; // MSAA scene target

    pd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pd.SampleMask = 0xFFFFFFFFu;
    pd.NumRenderTargets = 1;
    pd.RTVFormats[0] = (DXGI_FORMAT)D3D12Backend::BackBufferFormat();
    pd.SampleDesc.Count = (UINT)samples;

    ID3D12PipelineState* pso = 0;
    if (stereo)
        pso = CreateViPso(m_pDevice, pd, key, m_stereoViewCount);
    else if (FAILED(m_pDevice->CreateGraphicsPipelineState(&pd,
                                                           IID_PPV_ARGS(&pso))))
        pso = 0;
    if (!pso)
    {
        R12Log("[D3D12R] particle PSO create failed (key 0x%X)\n", key);
        (*cache)[key] = 0;
        return 0;
    }
    (*cache)[key] = pso;
    return pso;
}

// Artscout - 2026: #VFX Phase 1 -- lazily build the static unit-quad VB + 6-index IB (once) and transition
// the IB to INDEX_BUFFER state (once, needs an open command list -- mirrors EnsureWhiteTexture's deferred
// barrier). Returns false until both the texture manager and an open list are available.
bool D3D12Renderer::EnsureParticleStatics()
{
    if (m_pQuadVB && m_particleIbReady)
        return true;
    if (!g_pD3D12TextureManager)
        return false;

    if (!m_pQuadVB || !m_pQuadIB)
    {
        // Corners CCW: (-.5,-.5)/(.5,-.5)/(.5,.5)/(-.5,.5), uv (0,0)/(1,0)/(1,1)/(0,1).
        static const ParticleQuadV quad[4] = {
            {{-0.5f, -0.5f}, {0.0f, 0.0f}},
            {{0.5f, -0.5f}, {1.0f, 0.0f}},
            {{0.5f, 0.5f}, {1.0f, 1.0f}},
            {{-0.5f, 0.5f}, {0.0f, 1.0f}},
        };
        static const unsigned short idx[6] = {0, 1, 2, 0, 2, 3};
        if (!m_pQuadVB)
            m_pQuadVB = g_pD3D12TextureManager->CreateVertexBufferGPU(
                quad, sizeof(quad));
        if (!m_pQuadIB)
            m_pQuadIB =
                g_pD3D12TextureManager->CreateVertexBufferGPU(idx, sizeof(idx));
        if (!m_pQuadVB || !m_pQuadIB)
            return false;
    }

    // CreateVertexBufferGPU leaves the resource in VERTEX_AND_CONSTANT_BUFFER; the IB needs INDEX_BUFFER.
    if (!m_particleIbReady)
    {
        ID3D12GraphicsCommandList* cl = Cmd();
        if (!cl)
            return false;
        D3D12_RESOURCE_BARRIER b;
        ZeroMemory(&b, sizeof(b));
        b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource = m_pQuadIB;
        b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        b.Transition.StateBefore =
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        b.Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;
        cl->ResourceBarrier(1, &b);
        m_particleIbReady = true;
    }
    return true;
}

// primType (MPR_PKT_*): 1 POINTS, 2 LINES, 3 POLYLINE, 4 TRIANGLES, 5 TRISTRIP, 6 TRIFAN.
static int TopoTypeOf(int primType)
{
    return (primType == 1) ? 0 : (primType == 2 || primType == 3) ? 1 : 2;
}
static D3D_PRIMITIVE_TOPOLOGY TopoOf(int primType)
{
    switch (primType)
    {
    case 1:
        return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case 2:
        return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case 3:
        return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case 5:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    default:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; // 4, 6(emulated)
    }
}

//================================ passes =====================================

void D3D12Renderer::BeginScreenPass()
{
    m_pass = 0;
    m_cull = 0;
    m_bias = 0;
    // blend/depth come from SetState before each DrawTL; default to opaque, depth off (2D overlay).
    m_blend = BLEND_OPAQUE;
    m_depthWrite = false;
    m_depthTest = false;
}

void D3D12Renderer::BeginObjectPass()
{
    m_pass = 1;
    m_cull = 0;
    m_bias = 1; // #16 object depth-bias toward camera
    m_blend = BLEND_OPAQUE;
    m_depthWrite = true;
    m_depthTest = true;
    m_flags = FF_VERTEXCOLOR | FF_LIGHTING |
              FF_ALPHATEST; // matches D3D11 BeginObjectPass bring-up flags
    if (m_nvg)
        m_flags |=
            FF_NVG; // #97 NVG: green the cockpit / aircraft / world objects
    // #A5: IR is sticky like NVG -- without this, objects drawn straight after
    // the pass begins (buildings) stayed in colour inside a grey sensor image.
    if (m_irGrey)
        m_flags |= FF_IRGREY;
    m_dRender = true;
}

void D3D12Renderer::BeginTerrainPass()
{
    BeginObjectPass();
    m_bias = 2; // #78 terrain bias key: reversed-Z slope-scaled bias from
    // g_fGpuTerrainSlopeBias/DepthBias (both 0 by default -> no
    // push, biased objects still win the seam). Tune SlopeBias
    // to sink grazing-angle terrain so it stops z-fighting/see-through.
    static const float I[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    SetWorld(I); // Artscout - 2026: terrain posts are CAMERA-RELATIVE (the
    // object view matrix is rotation-only -- see TerrainGpu.cpp
    // ":223  g.x = ... - cp.x"). An earlier comment here claimed
    // "absolute world coordinates", which is simply false.
    m_hasTex0 = false;
    // #78 fog: distance fog (haze set once/frame by otw.cpp SetFog) dissolves distant terrain into haze -> DX7
    // hazed horizon + masks the residual mid/far LOD-seam shimmer. Textured tiles add FF_TEXTURE0 via SetTexture.
    m_flags = FF_VERTEXCOLOR | FF_FOG;
    if (m_nvg)
        m_flags |=
            FF_NVG; // #97 NVG: green the terrain (BeginObjectPass reset m_flags above)
    m_dRender = true;
}

void D3D12Renderer::SetTerrainRasterForLod(int)
{ /* per-LOD depth-bias variants = later increment */
}
void D3D12Renderer::RebuildTerrainRasters()
{ /* PSOs are built lazily -> nothing to rebuild */
}

// Artscout - 2026: #96 3D skydome. Object-path pass for the sky, drawn FIRST as the background: depth OFF (no test,
// no write) so it fills the frame and terrain/objects then draw over it (occlusion by draw order, like the legacy
// 2D sky). No lighting, no fog, no cull (dome viewed from inside). Vertex-colour (gradient), or textured (stars/sun).
void D3D12Renderer::BeginSkyPass(bool blend)
{
    m_pass = 1;
    m_cull = 0;
    m_bias = 0;
    m_blend =
        blend ?
            BLEND_ALPHA :
            BLEND_OPAQUE; // sun/moon discs blend over the gradient; the dome itself is opaque
    m_depthWrite = false;
    m_depthTest =
        false; // background: no depth (drawn first; terrain/objects draw over)
    m_flags =
        FF_VERTEXCOLOR; // pure vertex colour (no lighting/fog); a texture (stars) adds FF_TEXTURE0 via SetTexture
    if (m_nvg)
        m_flags |= FF_NVG; // #97 NVG: green the sky/sun/moon dome too
    static const float I[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    SetWorld(I); // dome verts are camera-relative world directions * radius
    m_hasTex0 = false;
    m_dRender = true;
}

//================================ draws ======================================

void D3D12Renderer::DrawTL(int primType, const ScreenVertex* verts, int count)
{
    if (!verts || count <= 0)
        return;
    g_pD3D12Backend->EnsureFrameStarted();
    g_bGpuDraw =
        true; // this frame drew 3D on the GPU -> present composites the scene (see imagebuf)
    ID3D12GraphicsCommandList* cl = Cmd();
    if (!cl)
        return;

    // Lines aren't textured (else a leftover font gTex0 would chroma-cut them).
    if ((primType == 2 || primType == 3) && (m_flags & FF_TEXTURE0))
    {
        m_flags &= ~FF_TEXTURE0;
        m_dRender = true;
    }

    FlushConstants();

    int topoType = TopoTypeOf(primType);
    ID3D12PipelineState* pso = GetPSO(m_pass, m_blend, m_depthWrite,
                                      m_depthTest, topoType, m_cull, m_bias);
    if (!pso)
        return;
    cl->SetPipelineState(pso);
    if (m_hudStencil)
        cl->OMSetStencilRef(
            0x80); // #76 HUD aperture ref (0x80); no-op unless a stencil PSO is bound

    D3D12_VERTEX_BUFFER_VIEW vbv;
    if (!AllocVB(verts, (unsigned)count * sizeof(ScreenVertex),
                 sizeof(ScreenVertex), &vbv))
        return;
    cl->IASetVertexBuffers(0, 1, &vbv);

    if (primType ==
        6) // TRIFAN -> triangle-list via an index buffer (no native fan topology)
    {
        int tris = count - 2;
        if (tris <= 0)
            return;
        int nIdx = tris * 3;
        unsigned short stackIdx[3 * 256];
        unsigned short* idx = stackIdx;
        unsigned short* heap = 0;
        if (nIdx > (int)_countof(stackIdx))
        {
            heap = (unsigned short*)malloc(nIdx * sizeof(unsigned short));
            if (!heap)
                return;
            idx = heap;
        }
        for (int i = 0; i < tris; ++i)
        {
            idx[i * 3 + 0] = 0;
            idx[i * 3 + 1] = (unsigned short)(i + 1);
            idx[i * 3 + 2] = (unsigned short)(i + 2);
        }
        D3D12_INDEX_BUFFER_VIEW ibv;
        bool ok = AllocIB(idx, nIdx, &ibv);
        if (heap)
            free(heap);
        if (!ok)
            return;
        cl->IASetIndexBuffer(&ibv);
        cl->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cl->DrawIndexedInstanced(nIdx, 1, 0, 0, 0);
        return;
    }

    cl->IASetPrimitiveTopology(TopoOf(primType));
    cl->DrawInstanced(count, 1, 0, 0);
}

void D3D12Renderer::DrawTLIndexed(int primType, const ScreenVertex* verts,
                                  int vcount, const unsigned short* indices,
                                  int icount)
{
    if (!verts || vcount <= 0 || !indices || icount <= 0)
        return;
    g_pD3D12Backend->EnsureFrameStarted();
    g_bGpuDraw =
        true; // this frame drew 3D on the GPU -> present composites the scene (see imagebuf)
    ID3D12GraphicsCommandList* cl = Cmd();
    if (!cl)
        return;

    if (primType == 2 && (m_flags & FF_TEXTURE0))
    {
        m_flags &= ~FF_TEXTURE0;
        m_dRender = true;
    }
    FlushConstants();

    int topoType = TopoTypeOf(primType);
    ID3D12PipelineState* pso = GetPSO(m_pass, m_blend, m_depthWrite,
                                      m_depthTest, topoType, m_cull, m_bias);
    if (!pso)
        return;
    cl->SetPipelineState(pso);
    if (m_hudStencil)
        cl->OMSetStencilRef(
            0x80); // #76 HUD aperture ref (0x80); no-op unless a stencil PSO is bound

    D3D12_VERTEX_BUFFER_VIEW vbv;
    if (!AllocVB(verts, (unsigned)vcount * sizeof(ScreenVertex),
                 sizeof(ScreenVertex), &vbv))
        return;
    D3D12_INDEX_BUFFER_VIEW ibv;
    if (!AllocIB(indices, icount, &ibv))
        return;
    cl->IASetVertexBuffers(0, 1, &vbv);
    cl->IASetIndexBuffer(&ibv);
    cl->IASetPrimitiveTopology(primType == 2 ?
                                   D3D_PRIMITIVE_TOPOLOGY_LINELIST :
                                   D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cl->DrawIndexedInstanced(icount, 1, 0, 0, 0);
}

void D3D12Renderer::DrawColorTrisScreen(const ScreenVertex* verts, int count,
                                        ID3D11ShaderResourceView* tex,
                                        int opaque, int cull)
{
    if (!verts || count < 3)
        return;
    BeginScreenPass();
    m_cull = (cull == 1) ? 1 : (cull == 2) ? 2 : 0;
    m_blend = opaque ? BLEND_OPAQUE : BLEND_ALPHA;
    m_depthWrite = false;
    m_depthTest = false; // overlay
    m_flags = tex ? FF_TEXTURE0 : 0u;
    m_dRender = true;
    SetTexture(0, tex); // NULL under D3D12 -> vertex colour
    DrawTL(4, verts, count); // TRIANGLELIST
}

// Artscout - 2026 (#107 bindless): asked for, and the device can do it. SM 6.6 gives the shader
// ResourceDescriptorHeap[]; binding tier 3 lifts the table-size limits the resident prefix needs.
bool D3D12Renderer::BindlessWanted() const
{
    extern bool g_bEnableBindless;

    if (!g_bEnableBindless || !m_pDevice)
        return false;

    D3D12_FEATURE_DATA_SHADER_MODEL sm;
    sm.HighestShaderModel = D3D_SHADER_MODEL_6_6;

    if (FAILED(m_pDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &sm,
                                              sizeof(sm)))
        || sm.HighestShaderModel < D3D_SHADER_MODEL_6_6)
    {
        return false;
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS opt;
    ZeroMemory(&opt, sizeof(opt));

    if (FAILED(m_pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &opt,
                                              sizeof(opt)))
        || opt.ResourceBindingTier < D3D12_RESOURCE_BINDING_TIER_3)
    {
        return false;
    }

    return true;
}

// Artscout - 2026 (#78): mesh shaders need SM 6.5 and MeshShaderTier1; the tile
// PS samples through the SM 6.6 heap, so this implies BindlessWanted() too.
bool D3D12Renderer::MeshShaderSupported() const
{
    extern bool g_bTerrainMeshShader;

    if (!g_bTerrainMeshShader || !m_pDevice || !BindlessWanted())
        return false;

    D3D12_FEATURE_DATA_SHADER_MODEL sm;
    sm.HighestShaderModel = D3D_SHADER_MODEL_6_6;

    if (FAILED(m_pDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &sm,
                                              sizeof(sm)))
        || sm.HighestShaderModel < D3D_SHADER_MODEL_6_5)
    {
        return false;
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS7 opt7;
    ZeroMemory(&opt7, sizeof(opt7));

    if (FAILED(m_pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7,
                                              &opt7, sizeof(opt7)))
        || opt7.MeshShaderTier < D3D12_MESH_SHADER_TIER_1)
    {
        return false;
    }

    return true;
}

bool D3D12Renderer::MeshTerrainAvailable() const
{
    return MeshShaderSupported();
}

// #78: the sun exactly as SetLights delivered it -- CDXEngine::TheSun is a
// leftover D3D7 light and is NOT what lights the cockpit.
bool D3D12Renderer::GetSunLight(float dir[3], float color[3],
                                float ambient[3]) const
{
    const unsigned numLights = *(const unsigned*)(m_lightsBuf + 16);
    const float* amb = (const float*)m_lightsBuf;

    ambient[0] = amb[0];
    ambient[1] = amb[1];
    ambient[2] = amb[2];

    // Lights are GpuLightCPU (64 bytes): position, direction, colour, params.
    for (unsigned l = 0; l < numLights && l < 8; ++l)
    {
        const float* L = (const float*)(m_lightsBuf + 32 + l * 64);

        if (L[4 * 3 + 1] >= 0.5f)
            continue; // params.y: 1 = point lamp, not the sun

        dir[0] = L[4];
        dir[1] = L[5];
        dir[2] = L[6]; // direction.xyz
        color[0] = L[8];
        color[1] = L[9];
        color[2] = L[10]; // colour.rgb
        return true;
    }

    return false;
}

// Artscout - 2026 (#78): the same per-view derivation FlushConstants does for
// cbViewStereo -- base rotation view shifted by the eye offset, per-view proj.
int D3D12Renderer::GetPerViewMatrices(float view[4][16], float proj[4][16])
{
    if (!m_stereoActive)
        return 0; // flat / monitor view: the base matrices already fit

    const int n = (m_stereoViewCount >= 1 && m_stereoViewCount <= 4) ?
                      m_stereoViewCount :
                      2;
    for (int e = 0; e < 4; ++e)
    {
        const int s = (e < n) ? e : 0; // unused slots mirror view 0
        memcpy(view[e], m_view, sizeof(m_view));
        const float* w = m_stereoWorldOff[s];
        const float ovx = w[0] * m_view[0] + w[1] * m_view[4] + w[2] * m_view[8];
        const float ovy = w[0] * m_view[1] + w[1] * m_view[5] + w[2] * m_view[9];
        const float ovz =
            w[0] * m_view[2] + w[1] * m_view[6] + w[2] * m_view[10];
        view[e][12] -= ovx;
        view[e][13] -= ovy;
        view[e][14] -= ovz; // translation row -= view-space eye offset
        memcpy(proj[e], m_stereoHasProj ? m_stereoProj[s] : m_proj,
               sizeof(m_proj));
    }
    return n;
}

void D3D12Renderer::ReleaseTerrainClipmap()
{
    if (m_pClipPost)
    {
        m_pClipPost->Release();
        m_pClipPost = 0;
    }
    if (m_pClipInfo)
    {
        m_pClipInfo->Release();
        m_pClipInfo = 0;
    }
    if (m_pClipBounds)
    {
        if (m_pClipBoundsCpu)
            m_pClipBounds->Unmap(0, NULL);
        m_pClipBounds->Release();
        m_pClipBounds = 0;
    }
    m_pClipBoundsCpu = 0;
    for (int f = 0; f < kFrames; ++f)
    {
        if (m_pClipUpload[f])
        {
            if (m_pClipUploadCpu[f])
                m_pClipUpload[f]->Unmap(0, NULL);
            m_pClipUpload[f]->Release();
            m_pClipUpload[f] = 0;
        }
        m_pClipUploadCpu[f] = 0;
        m_clipUploadSize[f] = 0;
        m_clipUploadOff[f] = 0;
    }
    m_clipTexels = m_clipLevels = m_clipTiles = 0;
    m_clipInCopyState = false;
    for (size_t i = 0; i < m_meshPsos.size(); ++i)
    {
        if (m_meshPsos[i].pso)
            m_meshPsos[i].pso->Release();
    }
    m_meshPsos.clear();
    if (m_pMeshRootSig)
    {
        m_pMeshRootSig->Release();
        m_pMeshRootSig = 0;
    }
}

// Artscout - 2026 (#78): allocate the clipmap. Two array textures (one slice per
// LOD) plus a per-frame staging ring; the bounds buffer stays CPU-writable, it
// is a few KB rewritten whenever a slice moves.
bool D3D12Renderer::CreateTerrainClipmap(int texels, int levels,
                                         int chunksPerSide)
{
    if (!m_pDevice || texels <= 0 || levels <= 0 || levels > 8)
        return false;
    if (m_pClipPost && m_clipTexels == texels && m_clipLevels == levels)
        return true;

    ReleaseTerrainClipmap();

    D3D12_HEAP_PROPERTIES hpDef;
    ZeroMemory(&hpDef, sizeof(hpDef));
    hpDef.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC td;
    ZeroMemory(&td, sizeof(td));
    td.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    td.Width = (UINT64)texels;
    td.Height = (UINT)texels;
    td.DepthOrArraySize = (UINT16)levels;
    td.MipLevels = 1;
    td.SampleDesc.Count = 1;
    td.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    td.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    if (FAILED(m_pDevice->CreateCommittedResource(
            &hpDef, D3D12_HEAP_FLAG_NONE, &td,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, NULL,
            IID_PPV_ARGS(&m_pClipPost))))
    {
        R12Log("[D3D12R] clipmap post texture failed\n");
        return false;
    }

    td.Format = DXGI_FORMAT_R32_UINT;
    if (FAILED(m_pDevice->CreateCommittedResource(
            &hpDef, D3D12_HEAP_FLAG_NONE, &td,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, NULL,
            IID_PPV_ARGS(&m_pClipInfo))))
    {
        R12Log("[D3D12R] clipmap info texture failed\n");
        ReleaseTerrainClipmap();
        return false;
    }

    D3D12_HEAP_PROPERTIES hpUp;
    ZeroMemory(&hpUp, sizeof(hpUp));
    hpUp.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bd.Height = 1;
    bd.DepthOrArraySize = 1;
    bd.MipLevels = 1;
    bd.Format = DXGI_FORMAT_UNKNOWN;
    bd.SampleDesc.Count = 1;
    bd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    const int tiles = (chunksPerSide > 0) ? chunksPerSide : 1;
    bd.Width = (UINT64)levels * tiles * tiles * 2 * sizeof(float);
    if (bd.Width < 256)
        bd.Width = 256;
    if (FAILED(m_pDevice->CreateCommittedResource(
            &hpUp, D3D12_HEAP_FLAG_NONE, &bd,
            D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
            IID_PPV_ARGS(&m_pClipBounds))))
    {
        R12Log("[D3D12R] clipmap bounds buffer failed\n");
        ReleaseTerrainClipmap();
        return false;
    }
    D3D12_RANGE noRead;
    noRead.Begin = noRead.End = 0;
    if (FAILED(m_pClipBounds->Map(0, &noRead, (void**)&m_pClipBoundsCpu)))
    {
        R12Log("[D3D12R] clipmap bounds map failed\n");
        ReleaseTerrainClipmap();
        return false;
    }

    // One full slice (post + info, rows padded to 256) with room to spare for
    // the strips the other levels stream in the same frame.
    const unsigned fullPost =
        AlignUp((unsigned)texels * 16u, 256u) * (unsigned)texels;
    const unsigned fullInfo =
        AlignUp((unsigned)texels * 4u, 256u) * (unsigned)texels;
    const unsigned ringBytes = (fullPost + fullInfo) * 2u + (1u << 20);

    bd.Width = ringBytes;
    for (int f = 0; f < kFrames; ++f)
    {
        if (FAILED(m_pDevice->CreateCommittedResource(
                &hpUp, D3D12_HEAP_FLAG_NONE, &bd,
                D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
                IID_PPV_ARGS(&m_pClipUpload[f]))))
        {
            R12Log("[D3D12R] clipmap upload ring[%d] failed\n", f);
            ReleaseTerrainClipmap();
            return false;
        }
        if (FAILED(m_pClipUpload[f]->Map(0, &noRead,
                                         (void**)&m_pClipUploadCpu[f])))
        {
            R12Log("[D3D12R] clipmap upload ring[%d] map failed\n", f);
            ReleaseTerrainClipmap();
            return false;
        }
        m_clipUploadSize[f] = ringBytes;
        m_clipUploadOff[f] = 0;
    }

    m_clipTexels = texels;
    m_clipLevels = levels;
    m_clipTiles = tiles;
    R12Log("[D3D12R] clipmap %dx%d x%d slices, ring %u KB\n", texels, texels,
           levels, ringBytes >> 10);
    return true;
}

void D3D12Renderer::UpdateTerrainClipmap(int level, int x, int y, int w, int h,
                                         const void* postRgba32f,
                                         const void* infoR32u)
{
    if (!m_pClipPost || !postRgba32f || !infoR32u || w <= 0 || h <= 0)
        return;
    if (level < 0 || level >= m_clipLevels)
        return;

    g_pD3D12Backend->EnsureFrameStarted();
    ID3D12GraphicsCommandList* cl = Cmd();
    if (!cl)
        return;

    const unsigned f = m_curFrame;
    if (f >= (unsigned)kFrames || !m_pClipUploadCpu[f])
        return;

    const unsigned pitchP = AlignUp((unsigned)w * 16u, 256u);
    const unsigned pitchI = AlignUp((unsigned)w * 4u, 256u);
    // Footprint offsets must sit on a 512-byte boundary.
    const unsigned offP = AlignUp(m_clipUploadOff[f], 512u);
    const unsigned offI = AlignUp(offP + pitchP * (unsigned)h, 512u);
    const unsigned end = offI + pitchI * (unsigned)h;

    if (end > m_clipUploadSize[f])
    {
        static bool once = false;
        if (!once)
        {
            once = true;
            R12Log("[D3D12R] clipmap staging ring overflow\n");
        }
        return;
    }
    m_clipUploadOff[f] = end;

    for (int r = 0; r < h; ++r)
    {
        memcpy(m_pClipUploadCpu[f] + offP + (size_t)r * pitchP,
               (const unsigned char*)postRgba32f + (size_t)r * w * 16,
               (size_t)w * 16);
        memcpy(m_pClipUploadCpu[f] + offI + (size_t)r * pitchI,
               (const unsigned char*)infoR32u + (size_t)r * w * 4,
               (size_t)w * 4);
    }

    // One transition for the whole upload run, not a pair per region.
    if (!m_clipInCopyState)
    {
        D3D12_RESOURCE_BARRIER bar[2];
        ZeroMemory(bar, sizeof(bar));
        for (int i = 0; i < 2; ++i)
        {
            bar[i].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            bar[i].Transition.pResource = (i == 0) ? m_pClipPost : m_pClipInfo;
            bar[i].Transition.Subresource =
                D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            bar[i].Transition.StateBefore =
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            bar[i].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        }
        cl->ResourceBarrier(2, bar);
        m_clipInCopyState = true;
    }

    D3D12_TEXTURE_COPY_LOCATION dst, src;
    ZeroMemory(&dst, sizeof(dst));
    ZeroMemory(&src, sizeof(src));
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = (UINT)level;
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint.Footprint.Width = (UINT)w;
    src.PlacedFootprint.Footprint.Height = (UINT)h;
    src.PlacedFootprint.Footprint.Depth = 1;

    dst.pResource = m_pClipPost;
    src.pResource = m_pClipUpload[f];
    src.PlacedFootprint.Offset = offP;
    src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    src.PlacedFootprint.Footprint.RowPitch = pitchP;
    cl->CopyTextureRegion(&dst, (UINT)x, (UINT)y, 0, &src, NULL);

    dst.pResource = m_pClipInfo;
    src.PlacedFootprint.Offset = offI;
    src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R32_UINT;
    src.PlacedFootprint.Footprint.RowPitch = pitchI;
    cl->CopyTextureRegion(&dst, (UINT)x, (UINT)y, 0, &src, NULL);
    // The matching transition back is in DrawTerrainMeshShader.
}

void D3D12Renderer::UpdateTerrainChunkBounds(const void* minMaxFloat2,
                                             int count)
{
    if (!m_pClipBoundsCpu || !minMaxFloat2 || count <= 0)
        return;
    const int cap = m_clipLevels * m_clipTiles * m_clipTiles;
    if (count > cap)
        count = cap;
    memcpy(m_pClipBoundsCpu, minMaxFloat2, (size_t)count * 2 * sizeof(float));
}

// Artscout - 2026 (#78): the terrain's own root signature -- no input layout
// (illegal with mesh shaders) and heap-direct indexing for the tile PS.
ID3D12PipelineState* D3D12Renderer::GetMeshTerrainPso()
{
    const int samples =
        g_pD3D12Backend ? g_pD3D12Backend->CurrentSampleCount() : 1;
    // View-instanced when the scene is (2 = stereo, 4 = quad-views); 1 = flat.
    int nViews = 1;
    if (m_stereoActive)
    {
        nViews = (m_stereoViewCount >= 1 && m_stereoViewCount <= 4) ?
                     m_stereoViewCount :
                     2;
    }

    // A depth-enabled PSO against a NULL DSV (the RTT atlas) trips #615 and the
    // runtime DROPS the draw -- which is why the sensor showed objects but no
    // terrain. Same rule GetPSO follows for every other pass.
    const bool depthOn = m_depthTargetBound;

    // Cached per combination: the sensor RTT pass is mono and the eyes are
    // view-instanced, so one slot would rebuild the PSO twice EVERY frame -- and
    // whichever variant lost the race drew the wrong eye count.
    for (size_t i = 0; i < m_meshPsos.size(); ++i)
    {
        if (m_meshPsos[i].samples == samples && m_meshPsos[i].views == nViews &&
            m_meshPsos[i].depth == depthOn)
        {
            return m_meshPsos[i].pso;
        }
    }

    unsigned int asSize = 0, msSize = 0, psSize = 0;
    const void* asBlob = FFGetShaderBlob(FFSHADER_TERRAIN_AS, FFSHADER_DXIL,
                                         &asSize);
    const void* msBlob = FFGetShaderBlob(FFSHADER_TERRAIN_MS, FFSHADER_DXIL,
                                         &msSize);
    const void* psBlob = FFGetShaderBlob(FFSHADER_TERRAIN_PS, FFSHADER_DXIL,
                                         &psSize);
    if (!asBlob || !msBlob || !psBlob)
    {
        R12Log("[D3D12R] mesh terrain: shader blobs missing\n");
        return 0;
    }

    if (!m_pMeshRootSig)
    {
        D3D12_DESCRIPTOR_RANGE srvRange;
        ZeroMemory(&srvRange, sizeof(srvRange));
        srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srvRange.NumDescriptors = 3; // t0 post, t1 info, t2 chunk bounds
        srvRange.OffsetInDescriptorsFromTableStart =
            D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER params[2];
        ZeroMemory(params, sizeof(params));
        params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        params[0].Descriptor.ShaderRegister = 0;
        params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[1].DescriptorTable.NumDescriptorRanges = 1;
        params[1].DescriptorTable.pDescriptorRanges = &srvRange;
        params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        D3D12_STATIC_SAMPLER_DESC samp;
        ZeroMemory(&samp, sizeof(samp));
        samp.Filter = D3D12_FILTER_ANISOTROPIC;
        samp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samp.MaxAnisotropy = 8;
        samp.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        samp.MaxLOD = D3D12_FLOAT32_MAX;
        samp.ShaderRegister = 0;
        samp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC rsd;
        ZeroMemory(&rsd, sizeof(rsd));
        rsd.NumParameters = 2;
        rsd.pParameters = params;
        rsd.NumStaticSamplers = 1;
        rsd.pStaticSamplers = &samp;
        // No ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT: a mesh pipeline rejects it.
        rsd.Flags =
            D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

        ID3DBlob* blob = 0;
        ID3DBlob* err = 0;
        if (FAILED(D3D12SerializeRootSignature(
                &rsd, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &err)))
        {
            R12Log("[D3D12R] mesh terrain root sig serialize failed: %s\n",
                   err ? (const char*)err->GetBufferPointer() : "?");
            if (err)
                err->Release();
            if (blob)
                blob->Release();
            return 0;
        }
        if (err)
            err->Release();

        HRESULT hr = m_pDevice->CreateRootSignature(
            0, blob->GetBufferPointer(), blob->GetBufferSize(),
            IID_PPV_ARGS(&m_pMeshRootSig));
        blob->Release();
        if (FAILED(hr))
        {
            R12Log("[D3D12R] mesh terrain root sig failed 0x%08X\n",
                   (unsigned)hr);
            return 0;
        }
    }

    ID3D12Device2* dev2 = 0;
    if (FAILED(m_pDevice->QueryInterface(IID_PPV_ARGS(&dev2))) || !dev2)
    {
        R12Log("[D3D12R] mesh terrain: ID3D12Device2 unavailable\n");
        if (dev2)
            dev2->Release();
        return 0;
    }

    D3D12_RASTERIZER_DESC rs;
    ZeroMemory(&rs, sizeof(rs));
    rs.FillMode = D3D12_FILL_MODE_SOLID;
    {
        // Off by default: a wrong winding guess hides the whole ground.
        extern bool g_bTerrainMeshCull;
        rs.CullMode = g_bTerrainMeshCull ? D3D12_CULL_MODE_BACK :
                                           D3D12_CULL_MODE_NONE;
    }
    rs.DepthClipEnable = TRUE;
    rs.MultisampleEnable = (samples > 1) ? TRUE : FALSE;
    {
        extern float g_fGpuTerrainSlopeBias, g_fGpuTerrainDepthBias;
        rs.DepthBias = -(int)g_fGpuTerrainDepthBias;
        rs.SlopeScaledDepthBias = -g_fGpuTerrainSlopeBias;
        rs.DepthBiasClamp = 0.0f;
    }

    D3D12_DEPTH_STENCIL_DESC ds;
    ZeroMemory(&ds, sizeof(ds));
    ds.DepthEnable = depthOn ? TRUE : FALSE;
    ds.DepthWriteMask =
        depthOn ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    // Reversed-Z, same comparison the rest of the scene uses.
    ds.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

    D3D12_BLEND_DESC bl;
    ZeroMemory(&bl, sizeof(bl));
    bl.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_RT_FORMAT_ARRAY rtf;
    ZeroMemory(&rtf, sizeof(rtf));
    rtf.NumRenderTargets = 1;
    rtf.RTFormats[0] = (DXGI_FORMAT)D3D12Backend::BackBufferFormat();

    DXGI_SAMPLE_DESC sd;
    sd.Count = (UINT)samples;
    sd.Quality = 0;

// alignas(void*) per member gives each subobject the alignment the stream wants.
#define FF_SUBOBJ(name, valType)                                               \
    struct alignas(void*)                                                      \
    {                                                                          \
        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE t;                                 \
        valType v;                                                             \
    } name;

    struct StreamT
    {
        FF_SUBOBJ(rootSig, ID3D12RootSignature*)
        FF_SUBOBJ(as, D3D12_SHADER_BYTECODE)
        FF_SUBOBJ(ms, D3D12_SHADER_BYTECODE)
        FF_SUBOBJ(ps, D3D12_SHADER_BYTECODE)
        FF_SUBOBJ(raster, D3D12_RASTERIZER_DESC)
        FF_SUBOBJ(depth, D3D12_DEPTH_STENCIL_DESC)
        FF_SUBOBJ(blend, D3D12_BLEND_DESC)
        FF_SUBOBJ(dsvFmt, DXGI_FORMAT)
        FF_SUBOBJ(rtvFmts, D3D12_RT_FORMAT_ARRAY)
        FF_SUBOBJ(sample, DXGI_SAMPLE_DESC)
        FF_SUBOBJ(sampleMask, UINT)
        FF_SUBOBJ(viewInst, D3D12_VIEW_INSTANCING_DESC)
    } s;
#undef FF_SUBOBJ

    s.rootSig.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
    s.rootSig.v = m_pMeshRootSig;
    s.as.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS;
    s.as.v.pShaderBytecode = asBlob;
    s.as.v.BytecodeLength = asSize;
    s.ms.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS;
    s.ms.v.pShaderBytecode = msBlob;
    s.ms.v.BytecodeLength = msSize;
    s.ps.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS;
    s.ps.v.pShaderBytecode = psBlob;
    s.ps.v.BytecodeLength = psSize;
    s.raster.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER;
    s.raster.v = rs;
    s.depth.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL;
    s.depth.v = ds;
    s.blend.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND;
    s.blend.v = bl;
    s.dsvFmt.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT;
    // UNKNOWN when no DSV is bound, or the runtime rejects the draw.
    s.dsvFmt.v = depthOn ? (DXGI_FORMAT)D3D12Backend::DepthFormat() :
                           DXGI_FORMAT_UNKNOWN;
    s.rtvFmts.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS;
    s.rtvFmts.v = rtf;
    s.sample.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC;
    s.sample.v = sd;
    s.sampleMask.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK;
    s.sampleMask.v = 0xFFFFFFFFu;

    // View v -> RT array slice v, so one DispatchMesh fills every eye. MS reads
    // SV_ViewID to pick its matrices.
    D3D12_VIEW_INSTANCE_LOCATION loc[4];
    for (int v = 0; v < nViews; ++v)
    {
        loc[v].ViewportArrayIndex = 0;
        loc[v].RenderTargetArrayIndex = (UINT)v;
    }
    s.viewInst.t = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING;
    s.viewInst.v.ViewInstanceCount = (UINT)nViews;
    s.viewInst.v.pViewInstanceLocations = loc;
    s.viewInst.v.Flags =
        D3D12_VIEW_INSTANCING_FLAG_ENABLE_VIEW_INSTANCE_MASKING;

    D3D12_PIPELINE_STATE_STREAM_DESC sdesc;
    sdesc.SizeInBytes = sizeof(s);
    sdesc.pPipelineStateSubobjectStream = &s;
    ID3D12PipelineState* pso = 0;
    HRESULT hr = dev2->CreatePipelineState(&sdesc, IID_PPV_ARGS(&pso));
    dev2->Release();

    if (FAILED(hr) || !pso)
    {
        R12Log("[D3D12R] mesh terrain PSO failed 0x%08X\n", (unsigned)hr);
        return 0;
    }

    MeshPsoEntry e;
    e.samples = samples;
    e.views = nViews;
    e.depth = depthOn;
    e.pso = pso;
    m_meshPsos.push_back(e);

    // Blob sizes identify WHICH build of the shaders is in the exe -- a stale
    // generated header is otherwise indistinguishable from a shader bug.
    R12Log("[D3D12R] mesh terrain PSO ready (samples=%d, views=%d, depth=%d) "
           "blobs as=%u ms=%u ps=%u\n",
           samples, nViews, (int)depthOn, asSize, msSize, psSize);
    return pso;
}

void D3D12Renderer::DrawTerrainMeshShader(const void* constants,
                                          int constantBytes, int chunkCount)
{
    if (!constants || constantBytes <= 0 || chunkCount <= 0)
        return;
    if (!m_pClipPost || !m_pClipInfo || !m_pClipBounds)
        return;
    ID3D12PipelineState* const meshPso = GetMeshTerrainPso();

    if (!meshPso)
        return;

    g_pD3D12Backend->EnsureFrameStarted();
    ID3D12GraphicsCommandList* cl = Cmd();
    if (!cl)
        return;

    ID3D12GraphicsCommandList6* cl6 = 0;
    if (FAILED(cl->QueryInterface(IID_PPV_ARGS(&cl6))) || !cl6)
    {
        if (cl6)
            cl6->Release();
        return;
    }

    // Close the upload run this frame's UpdateTerrainClipmap calls opened.
    if (m_clipInCopyState)
    {
        D3D12_RESOURCE_BARRIER bar[2];
        ZeroMemory(bar, sizeof(bar));
        for (int i = 0; i < 2; ++i)
        {
            bar[i].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            bar[i].Transition.pResource = (i == 0) ? m_pClipPost : m_pClipInfo;
            bar[i].Transition.Subresource =
                D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            bar[i].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            bar[i].Transition.StateAfter =
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        }
        cl6->ResourceBarrier(2, bar);
        m_clipInCopyState = false;
    }

    const unsigned f = m_curFrame;
    if (f >= (unsigned)kFrames || !m_pSrvRing[f])
    {
        cl6->Release();
        return;
    }

    // Three descriptors in this frame's shader-visible ring.
    const unsigned slot = m_srvRingOff[f];
    if (slot + 3 > m_srvRingCount + m_bindlessBase)
    {
        cl6->Release();
        return;
    }
    m_srvRingOff[f] = slot + 3;

    D3D12_CPU_DESCRIPTOR_HANDLE cpu =
        m_pSrvRing[f]->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE gpu =
        m_pSrvRing[f]->GetGPUDescriptorHandleForHeapStart();
    cpu.ptr += (SIZE_T)slot * m_srvInc;
    gpu.ptr += (UINT64)slot * m_srvInc;

    D3D12_SHADER_RESOURCE_VIEW_DESC sv;
    ZeroMemory(&sv, sizeof(sv));
    sv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    sv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    sv.Texture2DArray.MipLevels = 1;
    sv.Texture2DArray.ArraySize = (UINT)m_clipLevels;

    sv.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    m_pDevice->CreateShaderResourceView(m_pClipPost, &sv, cpu);
    cpu.ptr += m_srvInc;

    sv.Format = DXGI_FORMAT_R32_UINT;
    m_pDevice->CreateShaderResourceView(m_pClipInfo, &sv, cpu);
    cpu.ptr += m_srvInc;

    ZeroMemory(&sv, sizeof(sv));
    sv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    sv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    sv.Format = DXGI_FORMAT_UNKNOWN;
    sv.Buffer.NumElements =
        (UINT)(m_clipLevels * m_clipTiles * m_clipTiles);
    sv.Buffer.StructureByteStride = 2 * sizeof(float);
    m_pDevice->CreateShaderResourceView(m_pClipBounds, &sv, cpu);

    const unsigned __int64 cbAddr = AllocCB(constants, (unsigned)constantBytes);
    if (!cbAddr)
    {
        cl6->Release();
        return;
    }

    // Bind the ring explicitly: the tile PS reaches it through
    // ResourceDescriptorHeap, and this draw may be the frame's first.
    ID3D12DescriptorHeap* heaps[] = {m_pSrvRing[f]};
    cl6->SetDescriptorHeaps(1, heaps);
    cl6->SetGraphicsRootSignature(m_pMeshRootSig);
    cl6->SetPipelineState(meshPso);
    cl6->SetGraphicsRootConstantBufferView(0, cbAddr);
    cl6->SetGraphicsRootDescriptorTable(1, gpu);
    // One amplification thread per chunk (AS_GROUP = 32 in ffterrain.hlsl).
    cl6->DispatchMesh((UINT)((chunkCount + 31) / 32), 1, 1);
    cl6->Release();

    // Which pass actually issued the dispatch: the sensor (no depth target) is
    // the one that showed no ground, so count the two apart.
    {
        static unsigned s_scene = 0, s_sensor = 0, s_lastChunks = 0;
        static unsigned long s_tick = 0;
        const unsigned long now = GetTickCount();

        if (m_depthTargetBound)
            ++s_scene;
        else
            ++s_sensor;

        s_lastChunks = (unsigned)chunkCount;

        if (!s_tick)
            s_tick = now;

        if (now - s_tick >= 5000)
        {
            R12Log("[D3D12R] mesh terrain dispatches/5s: scene=%u sensor=%u "
                   "(last chunks=%u)\n",
                   s_scene, s_sensor, s_lastChunks);
            s_tick = now;
            s_scene = s_sensor = 0;
        }
    }

    g_bGpuDraw = true;
    // The terrain root signature is not the engine's -- make the next ordinary
    // draw rebind its own state.
    m_frameRebind = true;
}

bool D3D12Renderer::BindlessTerrainActive() const
{
    return m_bindlessBase != 0;
}

// #107: a destroyed texture hands its slot back. The descriptor stays stale
// until the slot is handed out again, and only that texture referenced it.
void D3D12Renderer::ReleaseBindlessSlot(unsigned slot)
{
    if (!m_bindlessBase || slot >= kBindlessMax)
        return;

    // Point the slot at the white 1x1 before recycling it: the descriptor would
    // otherwise keep naming a destroyed resource, and anything still holding
    // this slot samples a dangling descriptor -- black ground under D3D12.
    if (m_pDevice && m_whiteSrvCpu)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE src;
        src.ptr = (SIZE_T)m_whiteSrvCpu;

        for (int f = 0; f < kFrames; ++f)
        {
            if (!m_pSrvRing[f])
                continue;

            D3D12_CPU_DESCRIPTOR_HANDLE dst =
                m_pSrvRing[f]->GetCPUDescriptorHandleForHeapStart();
            dst.ptr += (SIZE_T)slot * m_srvInc;
            m_pDevice->CopyDescriptorsSimple(
                1, dst, src, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }
    }

    // Called from the loader thread when a texture dies.
    std::lock_guard<std::mutex> lk(m_bindlessMutex);
    m_bindlessFree.push_back(slot);
}

// A tile SRV gets a slot on first use and keeps it. The descriptor is written into EVERY frame's heap, so the
// number means the same thing whichever one is bound. 0xFFFFFFFF -> the caller draws that tile the old way.
unsigned int D3D12Renderer::BindlessTexIndex(ID3D11ShaderResourceView* srv)
{
    D3D12Texture* const tex = (D3D12Texture*)srv;

    if (!m_bindlessBase || !tex || !m_pDevice)
        return 0xFFFFFFFFu;

    if (tex->bindlessSlot != 0xFFFFFFFFu)
        return tex->bindlessSlot;

    const SIZE_T src = (SIZE_T)tex->srvCpuPtr;

    if (src == 0 || src == (SIZE_T)-1)
        return 0xFFFFFFFFu;

    // Reuse a destroyed texture's slot first: leaving 3D and coming back
    // recreates every tile, and without reuse the 16384 slots run out.
    unsigned slot;
    bool reused = false;

    {
        std::lock_guard<std::mutex> lk(m_bindlessMutex);

        if (!m_bindlessFree.empty())
        {
            slot = m_bindlessFree.back();
            m_bindlessFree.pop_back();
            reused = true;
        }
    }

    if (reused)
    {
        // fall through with the recycled slot
    }
    else if (m_bindlessNext >= kBindlessMax)
    {
        static bool once = false;

        if (!once)
        {
            once = true;
            R12Log("[D3D12R] bindless slots exhausted at %u\n",
                   (unsigned)kBindlessMax);
        }

        return 0xFFFFFFFFu;
    }
    else
    {
        slot = m_bindlessNext++;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE s;
    s.ptr = src;

    for (int f = 0; f < kFrames; ++f)
    {
        if (!m_pSrvRing[f])
            continue;

        D3D12_CPU_DESCRIPTOR_HANDLE dst =
            m_pSrvRing[f]->GetCPUDescriptorHandleForHeapStart();
        dst.ptr += (SIZE_T)slot * m_srvInc;

        m_pDevice->CopyDescriptorsSimple(
            1, dst, s, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    tex->bindlessSlot = slot;
    return slot;
}

// The whole ground in one draw: 44-byte vertices (ObjV + a uint slot) and FF_BINDLESS, which routes the base
// sample through the heap instead of t0.
void D3D12Renderer::DrawTerrainMeshBindless(const void* verts, int vcount,
                                            const unsigned short* indices,
                                            int icount)
{
    if (!m_bindlessBase || !verts || vcount <= 0 || !indices || icount <= 0)
        return;

    g_pD3D12Backend->EnsureFrameStarted();
    g_bGpuDraw = true;

    ID3D12GraphicsCommandList* cl = Cmd();

    if (!cl)
        return;

    const unsigned savedFlags = m_flags;
    m_flags |= FF_BINDLESS;
    m_dRender = true;

    FlushConstants();

    ID3D12PipelineState* pso =
        GetPSO(m_pass, m_blend, m_depthWrite, m_depthTest, /*bindless terrain*/ 3,
               m_cull, m_bias);

    if (!pso)
    {
        m_flags = savedFlags;
        m_dRender = true;
        return;
    }

    cl->SetPipelineState(pso);

    D3D12_VERTEX_BUFFER_VIEW vbv;
    const unsigned stride = (unsigned)sizeof(ObjV) + 4u;

    if (AllocVB(verts, (unsigned)vcount * stride, stride, &vbv))
    {
        D3D12_INDEX_BUFFER_VIEW ibv;

        if (AllocIB(indices, icount, &ibv))
        {
            cl->IASetVertexBuffers(0, 1, &vbv);
            cl->IASetIndexBuffer(&ibv);
            cl->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            cl->DrawIndexedInstanced(icount, 1, 0, 0, 0);
        }
    }

    m_flags = savedFlags;
    m_dRender = true;
}

void D3D12Renderer::DrawTerrainMesh(const void* verts, int vcount,
                                    const unsigned short* indices, int icount)
{
    if (!verts || vcount <= 0 || !indices || icount <= 0)
        return;
    g_pD3D12Backend->EnsureFrameStarted();
    g_bGpuDraw =
        true; // this frame drew 3D on the GPU -> present composites the scene (see imagebuf)
    ID3D12GraphicsCommandList* cl = Cmd();
    if (!cl)
        return;

    FlushConstants();
    ID3D12PipelineState* pso =
        GetPSO(m_pass, m_blend, m_depthWrite, m_depthTest, /*triangle*/ 2,
               m_cull, m_bias);
    if (!pso)
        return;
    cl->SetPipelineState(pso);
    if (m_hudStencil)
        cl->OMSetStencilRef(
            0x80); // #76 HUD aperture ref (0x80); no-op unless a stencil PSO is bound

    D3D12_VERTEX_BUFFER_VIEW vbv;
    if (!AllocVB(verts, (unsigned)vcount * (unsigned)sizeof(ObjV),
                 (unsigned)sizeof(ObjV), &vbv))
        return;
    D3D12_INDEX_BUFFER_VIEW ibv;
    if (!AllocIB(indices, icount, &ibv))
        return;
    cl->IASetVertexBuffers(0, 1, &vbv);
    cl->IASetIndexBuffer(&ibv);
    cl->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cl->DrawIndexedInstanced(icount, 1, 0, 0, 0);
}

//---- 2D-in-3D (particles/tracers/blips): object VS, world=identity, alpha/additive, depth test no write ----

void D3D12Renderer::BeginDynamic2D(bool additive)
{
    m_pass = 1;
    m_cull = 0;
    m_bias = 0;
    m_blend = additive ? BLEND_ADDITIVE : BLEND_ALPHA;
    m_depthWrite = false;
    m_depthTest = true;
    static const float I[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    SetWorld(I);
    m_flags = FF_VERTEXCOLOR | FF_ALPHATEST;
    m_alphaRef = 0.02f;
    m_dRender = true;
}

// Convert a DynV batch to ObjV and draw it as one indexed/non-indexed primitive. (D3D11 splits upload from
// draw for multi-item flushes; here each call is self-contained -- fine for the small dynamic batches.)
static int ConvertDyn(const void* dynVerts, int vcount, ObjV* dst)
{
    const DynV* src = (const DynV*)dynVerts;
    for (int i = 0; i < vcount; ++i)
    {
        dst[i].p[0] = src[i].p[0];
        dst[i].p[1] = src[i].p[1];
        dst[i].p[2] = src[i].p[2];
        dst[i].n[0] = 0.0f;
        dst[i].n[1] = 0.0f;
        dst[i].n[2] = 1.0f;
        dst[i].col = src[i].col;
        dst[i].spec = src[i].spec;
        dst[i].tu = src[i].tu;
        dst[i].tv = src[i].tv;
    }
    return vcount;
}

// Artscout - 2026: #VFX -- stage the accumulated DynV buffer for the per-item DrawDynamic2DIndexed
// calls that follow in DX2D_Flush2DObjects. The pointer is the persistent CPU Dyn2DVertexBuffer
// (valid for the whole flush); we don't upload to the GPU here -- each indexed draw gathers only its
// own vertices (small batches) and streams them via AllocVB, mirroring DrawDynamic2D.
void D3D12Renderer::UploadDynamic2D(const void* dynVerts, int vcount)
{
    m_dyn2DVerts = dynVerts;
    m_dyn2DVcount = vcount;
}

void D3D12Renderer::DrawDynamic2D(const void* dynVerts, int vcount,
                                  ID3D11ShaderResourceView* srv, int primType)
{
    if (!dynVerts || vcount <= 0)
        return;
    g_pD3D12Backend->EnsureFrameStarted();
    g_bGpuDraw =
        true; // this frame drew 3D on the GPU -> present composites the scene (see imagebuf)
    ID3D12GraphicsCommandList* cl = Cmd();
    if (!cl)
        return;
    SetTexture(0, srv);
    FlushConstants();

    int topoType = TopoTypeOf(primType);
    ID3D12PipelineState* pso = GetPSO(m_pass, m_blend, m_depthWrite,
                                      m_depthTest, topoType, m_cull, m_bias);
    if (!pso)
        return;
    cl->SetPipelineState(pso);
    if (m_hudStencil)
        cl->OMSetStencilRef(
            0x80); // #76 HUD aperture ref (0x80); no-op unless a stencil PSO is bound

    ObjV stackV[512];
    ObjV* v = stackV;
    ObjV* heap = 0;
    if (vcount > (int)_countof(stackV))
    {
        heap = (ObjV*)malloc(vcount * sizeof(ObjV));
        if (!heap)
            return;
        v = heap;
    }
    ConvertDyn(dynVerts, vcount, v);
    D3D12_VERTEX_BUFFER_VIEW vbv;
    bool ok = AllocVB(v, (unsigned)vcount * (unsigned)sizeof(ObjV),
                      (unsigned)sizeof(ObjV), &vbv);
    if (heap)
        free(heap);
    if (!ok)
        return;
    cl->IASetVertexBuffers(0, 1, &vbv);
    cl->IASetPrimitiveTopology(TopoOf(primType));
    cl->DrawInstanced(vcount, 1, 0, 0);
}

// Artscout - 2026: #VFX -- indexed draw over the DynV buffer staged by UploadDynamic2D. This is the
// path DX2D_Flush2DObjects uses for EVERY legacy 2D-in-3D primitive (quads, tapes/ribbons, lines);
// under D3D12 it was a stub, so smoke TRAILS (missile & flare/chaff smoke), tape effects and blips
// were invisible -- only the hand-routed PS_PolyRun billboards drew. We gather this draw's indexed
// vertices from the staged DynV buffer into a flat ObjV list and draw non-indexed (the index list
// already encodes tri-list / line-list order), reusing the exact PSO/blend/depth of DrawDynamic2D.
void D3D12Renderer::DrawDynamic2DIndexed(const unsigned short* indices,
                                         int icount,
                                         ID3D11ShaderResourceView* srv,
                                         int primType)
{
    if (!indices || icount <= 0 || !m_dyn2DVerts || m_dyn2DVcount <= 0)
        return;
    g_pD3D12Backend->EnsureFrameStarted();
    g_bGpuDraw =
        true; // this frame drew 3D on the GPU -> present composites the scene (see imagebuf)
    ID3D12GraphicsCommandList* cl = Cmd();
    if (!cl)
        return;
    SetTexture(0, srv);
    FlushConstants();

    int topoType = TopoTypeOf(primType);
    ID3D12PipelineState* pso = GetPSO(m_pass, m_blend, m_depthWrite,
                                      m_depthTest, topoType, m_cull, m_bias);
    if (!pso)
        return;
    cl->SetPipelineState(pso);
    if (m_hudStencil)
        cl->OMSetStencilRef(
            0x80); // #76 HUD aperture ref; no-op unless a stencil PSO is bound

    const DynV* src = (const DynV*)m_dyn2DVerts;
    ObjV stackV[512];
    ObjV* v = stackV;
    ObjV* heap = 0;
    if (icount > (int)_countof(stackV))
    {
        heap = (ObjV*)malloc(icount * sizeof(ObjV));
        if (!heap)
            return;
        v = heap;
    }
    for (int i = 0; i < icount; ++i)
    {
        int idx = indices[i];
        if (idx < 0 || idx >= m_dyn2DVcount)
            idx =
                0; // guard a stray index (never read out of the staged buffer)
        const DynV& s = src[idx];
        ObjV& d = v[i];
        d.p[0] = s.p[0];
        d.p[1] = s.p[1];
        d.p[2] = s.p[2];
        d.n[0] = 0.0f;
        d.n[1] = 0.0f;
        d.n[2] = 1.0f;
        d.col = s.col;
        d.spec = s.spec;
        d.tu = s.tu;
        d.tv = s.tv;
    }
    D3D12_VERTEX_BUFFER_VIEW vbv;
    bool ok = AllocVB(v, (unsigned)icount * (unsigned)sizeof(ObjV),
                      (unsigned)sizeof(ObjV), &vbv);
    if (heap)
        free(heap);
    if (!ok)
        return;
    cl->IASetVertexBuffers(0, 1, &vbv);
    cl->IASetPrimitiveTopology(TopoOf(primType));
    cl->DrawInstanced(icount, 1, 0, 0);
}

//---- object/BSP path (aircraft/cockpit): per-model DEFAULT-heap VB (ID3D12Resource*), object shader (pass 1) ----

void D3D12Renderer::DrawObjectIndexed(int primType, void* vbHandle, int stride,
                                      int baseVertex,
                                      const unsigned short* indices,
                                      int indexCount)
{
    if (!vbHandle || !indices || indexCount <= 0)
        return;
    g_pD3D12Backend->EnsureFrameStarted();
    g_bGpuDraw = true;
    ID3D12GraphicsCommandList* cl = Cmd();
    if (!cl)
        return;

    FlushConstants();
    ID3D12PipelineState* pso =
        GetPSO(/*object*/ 1, m_blend, m_depthWrite, m_depthTest,
               (primType == 6) ? 2 : TopoTypeOf(primType), m_cull, m_bias);
    if (!pso)
        return;
    cl->SetPipelineState(pso);
    if (m_hudStencil)
        cl->OMSetStencilRef(
            0x80); // #76 HUD aperture ref (0x80); no-op unless a stencil PSO is bound

    ID3D12Resource* vb = (ID3D12Resource*)vbHandle;
    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = vb->GetGPUVirtualAddress();
    vbv.SizeInBytes = (UINT)vb->GetDesc().Width;
    vbv.StrideInBytes = (UINT)stride;
    cl->IASetVertexBuffers(0, 1, &vbv);

    if (primType == 6) // TRIANGLEFAN -> triangle-list via an index buffer
    {
        int tris = indexCount - 2;
        if (tris <= 0)
            return;
        int nIdx = tris * 3;
        unsigned short stackIdx[3 * 256];
        unsigned short* idx = stackIdx;
        unsigned short* heap = 0;
        if (nIdx > (int)_countof(stackIdx))
        {
            heap = (unsigned short*)malloc(nIdx * sizeof(unsigned short));
            if (!heap)
                return;
            idx = heap;
        }
        for (int i = 0; i < tris; ++i)
        {
            idx[i * 3 + 0] = indices[0];
            idx[i * 3 + 1] = indices[i + 1];
            idx[i * 3 + 2] = indices[i + 2];
        }
        D3D12_INDEX_BUFFER_VIEW ibv;
        bool ok = AllocIB(idx, nIdx, &ibv);
        if (heap)
            free(heap);
        if (!ok)
            return;
        cl->IASetIndexBuffer(&ibv);
        cl->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cl->DrawIndexedInstanced(nIdx, 1, 0, baseVertex, 0);
        return;
    }

    D3D12_INDEX_BUFFER_VIEW ibv;
    if (!AllocIB(indices, indexCount, &ibv))
        return;
    cl->IASetIndexBuffer(&ibv);
    cl->IASetPrimitiveTopology(TopoOf(primType));
    cl->DrawIndexedInstanced(indexCount, 1, 0, baseVertex, 0);
}

void D3D12Renderer::DrawObjectStrip(int primType, void* vbHandle, int stride,
                                    int startVertex, int vertexCount)
{
    if (!vbHandle || vertexCount <= 0)
        return;
    g_pD3D12Backend->EnsureFrameStarted();
    g_bGpuDraw = true;
    ID3D12GraphicsCommandList* cl = Cmd();
    if (!cl)
        return;

    FlushConstants();
    ID3D12PipelineState* pso = GetPSO(1, m_blend, m_depthWrite, m_depthTest,
                                      TopoTypeOf(primType), m_cull, m_bias);
    if (!pso)
        return;
    cl->SetPipelineState(pso);
    if (m_hudStencil)
        cl->OMSetStencilRef(
            0x80); // #76 HUD aperture ref (0x80); no-op unless a stencil PSO is bound

    ID3D12Resource* vb = (ID3D12Resource*)vbHandle;
    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = vb->GetGPUVirtualAddress();
    vbv.SizeInBytes = (UINT)vb->GetDesc().Width;
    vbv.StrideInBytes = (UINT)stride;
    cl->IASetVertexBuffers(0, 1, &vbv);
    cl->IASetPrimitiveTopology(TopoOf(primType));
    cl->DrawInstanced(vertexCount, 1, startVertex, 0);
}

//---- #VFX Phase 1: GPU-instanced billboard particles --------------------------------------------

// Artscout - 2026: #VFX Phase 1 -- draw `count` camera-facing billboards with ONE DrawIndexedInstanced.
// The static unit quad (slot 0) is expanded per instance in VS_Particle from the per-instance stream
// (slot 1) uploaded into the per-frame VB ring. `atlasSrv` (D3D12Texture*) is the sprite atlas -> t0;
// blendMode 0 = additive (glow), 1 = alpha. NOTE: this is a NEW, self-contained path -- the legacy
// DX2D_AddQuad particle emit (drawparticlesys.cpp) is NOT rewired here (that is Phase 2, by hand).
void D3D12Renderer::DrawParticlesInstanced(const void* inst, int count,
                                           void* atlasSrv, int blendMode)
{
    if (count <= 0 || !inst || !atlasSrv)
        return;
#ifdef _DEBUG
    // Artscout - 2026: #VFX Phase 2 TEMP diag (remove once confirmed). Log the first few real
    // draws: count, atlas ptr, first instance center/size (44-byte D3D12ParticleInstance: 3f center,
    // 2f size, ...) vs the camera pos -> confirms the draw fires AND that coords are sane.
    {
        static int s_n = 0;
        if (s_n < 12)
        {
            s_n++;
            const float* f = (const float*)inst; // center[0..2], size[0..1]
            R12Log("[VFXDIAG] DrawParticlesInstanced n=%d blend=%d atlas=%p "
                   "center=(%.1f %.1f %.1f) size=(%.1f %.1f)\n",
                   count, blendMode, atlasSrv, f[0], f[1], f[2], f[3], f[4]);
        }
    }
#endif
    g_pD3D12Backend->EnsureFrameStarted();
    g_bGpuDraw =
        true; // this frame drew 3D on the GPU -> present composites the scene (see imagebuf)
    ID3D12GraphicsCommandList* cl = Cmd();
    if (!cl)
        return;
    if (!EnsureParticleStatics())
        return;

    // Bind the atlas as t0 (FlushConstants copies it into the SRV ring + points the root table at it).
    SetTexture(0, (ID3D11ShaderResourceView*)atlasSrv);
    SetTexture(
        1,
        NULL); // Artscout - 2026: particles sample only t0 -- don't inherit a stale/freed t1 (crash guard)
    FlushConstants();

    ID3D12PipelineState* pso = GetParticlePSO(blendMode);
    if (!pso)
        return;
    cl->SetPipelineState(pso);

    // Per-instance stream into the per-frame VB ring.
    D3D12_VERTEX_BUFFER_VIEW ivbv;
    if (!AllocInstanceVB(inst, count * (int)sizeof(D3D12ParticleInstance),
                         &ivbv))
        return;

    // slot 0 = static unit quad, slot 1 = per-instance stream.
    D3D12_VERTEX_BUFFER_VIEW qvbv;
    qvbv.BufferLocation = m_pQuadVB->GetGPUVirtualAddress();
    qvbv.SizeInBytes = 4 * (UINT)sizeof(ParticleQuadV);
    qvbv.StrideInBytes = (UINT)sizeof(ParticleQuadV);
    D3D12_VERTEX_BUFFER_VIEW vbs[2] = {qvbv, ivbv};
    cl->IASetVertexBuffers(0, 2, vbs);

    D3D12_INDEX_BUFFER_VIEW ibv;
    ibv.BufferLocation = m_pQuadIB->GetGPUVirtualAddress();
    ibv.SizeInBytes = 6 * (UINT)sizeof(unsigned short);
    ibv.Format = DXGI_FORMAT_R16_UINT;
    cl->IASetIndexBuffer(&ibv);
    cl->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cl->DrawIndexedInstanced(6, count, 0, 0,
                             0); // 6 indices (one quad) x count instances
}

// Artscout - 2026 (#DX12 splash): draw a CPU RGBA bitmap (load splash / cursor / mirror) as a screen-space quad.
// Was a {} stub -> the 3D-load splash never showed under D3D12 (flat + the VR panel that mirrors this frame).
// Mirrors D3D11Renderer::DrawBitmap2D: upload the sub-region into a temp texture (via the manager's serialized
// upload, so it's ready before the draw executes), draw an alpha-blended depth-off screen TRISTRIP, release the
// temp (frame-deferred -> the GPU keeps it alive until this frame finishes). Infrequent path -> per-call create is fine.
void D3D12Renderer::DrawBitmap2D(int dX, int dY, int w, int h, int totalWidth,
                                 int sX, int sY, const unsigned* pSrc, bool fit,
                                 int screenW, int screenH)
{
    if (!pSrc || w <= 0 || h <= 0 || !g_pD3D12TextureManager)
        return;

    D3D12Texture tmp;
    TexMipData mip;
    mip.data = pSrc + (size_t)sY * totalWidth + sX;
    mip.rowPitch = totalWidth * 4; // sub-region
    if (!g_pD3D12TextureManager->Create(
            tmp, w, h, (int)DXGI_FORMAT_R8G8B8A8_UNORM, &mip, 1))
        return;

    BeginScreenPass();
    m_cull = 0;
    m_blend = BLEND_ALPHA;
    m_depthWrite = false;
    m_depthTest = false; // overlay
    m_flags = FF_TEXTURE0;
    m_dRender = true;
    SetTexture(
        0,
        (ID3D11ShaderResourceView*)&tmp); // D3D12: pointer IS a D3D12Texture* (SetTexture reads srvCpuPtr)

    const float x0 = (float)dX, y0 = (float)dY;
    const float dw = fit ? (float)screenW : (float)w;
    const float dh = fit ? (float)screenH : (float)h;
    ScreenVertex v[4];
    ZeroMemory(v, sizeof(v));
    v[0].sx = x0;
    v[0].sy = y0;
    v[0].tu0 = 0;
    v[0].tv0 = 0; // TL
    v[1].sx = x0 + dw;
    v[1].sy = y0;
    v[1].tu0 = 1;
    v[1].tv0 = 0; // TR
    v[2].sx = x0;
    v[2].sy = y0 + dh;
    v[2].tu0 = 0;
    v[2].tv0 = 1; // BL
    v[3].sx = x0 + dw;
    v[3].sy = y0 + dh;
    v[3].tu0 = 1;
    v[3].tv0 = 1; // BR
    for (int i = 0; i < 4; ++i)
    {
        v[i].sz = 0.0f;
        v[i].rhw = 1.0f; // depth off -> sz value irrelevant (reversed-Z safe)
        v[i].color = 0xFFFFFFFF;
        v[i].specular = 0;
        v[i].tu1 = v[i].tu0;
        v[i].tv1 = v[i].tv0;
    }
    DrawTL(
        5, v,
        4); // TRISTRIP -- FlushConstants (inside) copies tmp's descriptor into the ring here

    // CRITICAL: `tmp` is a STACK D3D12Texture and is Destroyed below. SetTexture stored &tmp in m_pTex0; if we
    // left it, the NEXT draw's FlushConstants would CopyDescriptorsSimple from that dead-stack srvCpuPtr -> the
    // D3D12 debug layer reports descriptor corruption (crash), and Release silently copies garbage. Clear the
    // slot to the 1x1 white default so no stale pointer survives this call. (Was the sky-draw crash after splash.)
    SetTexture(0, NULL);
    SetTexture(1, NULL);

    g_pD3D12TextureManager->Destroy(
        tmp); // frame-deferred: released only after this frame's GPU work completes
}

// Artscout - 2026: G-force / end-flight vignette (blackout / redout) -- see D3D11Renderer::DrawGlocOverlay.
// Fullscreen alpha-blended quad with FF_GLOC; the PS darkens/tints the periphery by radial UV distance.
// The quad spans the current viewport (m_screenW/H, set per-eye in VR), so it lands on the flat frame and
// in each HMD eye. Restores neutral state so following cursor/menu draws aren't tinted.
void D3D12Renderer::DrawGlocOverlay(float intensity, float innerR, float outerR,
                                    float tintR, float tintG, float tintB)
{
    if (intensity <= 0.0f || m_screenW <= 0 || m_screenH <= 0)
        return;

    BeginScreenPass();
    m_cull = 0;
    m_blend = BLEND_ALPHA;
    m_depthWrite = false;
    m_depthTest = false; // overlay
    m_gloc[0] = intensity;
    m_gloc[1] = innerR;
    m_gloc[2] = outerR;
    m_gloc[3] = 0.0f;
    m_materialColor[0] = tintR;
    m_materialColor[1] = tintG;
    m_materialColor[2] = tintB;
    m_materialColor[3] = 1.0f;
    m_flags = FF_GLOC; // PS short-circuits to the vignette; no texture sampled
    m_dRender = true;

    const float w = (float)m_screenW, h = (float)m_screenH;
    ScreenVertex v[4];
    ZeroMemory(v, sizeof(v));
    v[0].sx = 0;
    v[0].sy = 0;
    v[0].tu0 = 0;
    v[0].tv0 = 0; // TL
    v[1].sx = w;
    v[1].sy = 0;
    v[1].tu0 = 1;
    v[1].tv0 = 0; // TR
    v[2].sx = 0;
    v[2].sy = h;
    v[2].tu0 = 0;
    v[2].tv0 = 1; // BL
    v[3].sx = w;
    v[3].sy = h;
    v[3].tu0 = 1;
    v[3].tv0 = 1; // BR
    for (int i = 0; i < 4; ++i)
    {
        v[i].sz = 0.0f;
        v[i].rhw = 1.0f;
        v[i].color = 0xFFFFFFFF;
        v[i].specular = 0;
        v[i].tu1 = v[i].tu0;
        v[i].tv1 = v[i].tv0;
    }
    DrawTL(5, v, 4); // TRISTRIP

    // Restore neutral render state (flags off, material white, gloc off) for subsequent draws.
    m_flags = 0;
    m_gloc[0] = 0.0f;
    m_materialColor[0] = m_materialColor[1] = m_materialColor[2] =
        m_materialColor[3] = 1.0f;
    m_dRender = true;
}

// #DX12 п.2: composite the 2D-UI surface (RGB565, black=transparent) over the 3D scene, via the backend's
// alpha-blend fullscreen quad. Called by the present path on a GPU frame instead of skipping the overlay.
void D3D12Renderer::CompositeUISurface(const void* src565, int w, int h)
{
    if (g_pD3D12Backend)
        g_pD3D12Backend->CompositeBitmap565(src565, w, h);
}
// Artscout - 2026 (#90): load an image file (WIC decode -> RGBA) into a D3D12 texture and return the opaque
// handle SetTexture/FlushConstants expect (a D3D12Texture*, cast to ID3D11ShaderResourceView* like the rest of
// the D3D12 path). Was a { return 0; } stub -> the VR controller-hand .obj models (art/ckptart/controllers)
// drew untextured under D3D12 (D3D11 worked). Same WIC pipeline as D3D11Renderer::LoadTextureFile; the pixels
// go through the texture manager (serialized upload queue) instead of an immediate CreateTexture2D.
ID3D11ShaderResourceView* D3D12Renderer::LoadTextureFile(const char* path)
{
    if (!path || !path[0] || !g_pD3D12TextureManager)
        return 0;
    wchar_t wpath[512];
    if (MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, 512) == 0)
        return 0;

    CoInitializeEx(
        NULL,
        COINIT_APARTMENTTHREADED); // harmless if COM already up (RPC_E_CHANGED_MODE ignored)

    IWICImagingFactory* fac = NULL;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, NULL,
                                CLSCTX_INPROC_SERVER,
                                __uuidof(IWICImagingFactory), (void**)&fac)) ||
        !fac)
        return 0;

    D3D12Texture* out = 0;
    IWICBitmapDecoder* dec = NULL;
    IWICBitmapFrameDecode* frame = NULL;
    IWICFormatConverter* conv = NULL;
    do
    {
        if (FAILED(fac->CreateDecoderFromFilename(
                wpath, NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand,
                &dec)) ||
            !dec)
            break;
        if (FAILED(dec->GetFrame(0, &frame)) || !frame)
            break;
        if (FAILED(fac->CreateFormatConverter(&conv)) || !conv)
            break;
        if (FAILED(conv->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
                                    WICBitmapDitherTypeNone, NULL, 0.0,
                                    WICBitmapPaletteTypeCustom)))
            break;
        UINT w = 0, h = 0;
        conv->GetSize(&w, &h);
        if (!w || !h)
            break;
        unsigned char* buf =
            new (std::nothrow) unsigned char[(size_t)w * h * 4];
        if (!buf)
            break;
        if (SUCCEEDED(conv->CopyPixels(NULL, w * 4, w * h * 4, buf)))
        {
            D3D12Texture* t = new (std::nothrow) D3D12Texture();
            if (t)
            {
                TexMipData mip;
                mip.data = buf;
                mip.rowPitch = (int)(w * 4);
                if (g_pD3D12TextureManager->Create(
                        *t, (int)w, (int)h, (int)DXGI_FORMAT_R8G8B8A8_UNORM,
                        &mip, 1))
                    out = t;
                else
                    delete t;
            }
        }
        delete[] buf;
    } while (0);
    if (conv)
        conv->Release();
    if (frame)
        frame->Release();
    if (dec)
        dec->Release();
    fac->Release();
    R12Log("VR model tex (D3D12): %s -> %s\n", path, out ? "ok" : "FAILED");
    return (ID3D11ShaderResourceView*)
        out; // D3D12: really a D3D12Texture* -> SetTexture reads its srvCpuPtr
}

// Artscout - 2026: #96 -- static texture from a 32bpp RGBA buffer (for the baked palettized moon). Same create path
// as LoadTextureFile, minus WIC. Returns a D3D12Texture* (as ID3D11ShaderResourceView*); NULL on failure.
ID3D11ShaderResourceView* D3D12Renderer::LoadTextureRGBA(const void* rgba,
                                                         int w, int h)
{
    if (!rgba || w <= 0 || h <= 0 || !g_pD3D12TextureManager)
        return 0;
    D3D12Texture* t = new (std::nothrow) D3D12Texture();
    if (!t)
        return 0;
    TexMipData mip;
    mip.data = (void*)rgba;
    mip.rowPitch = w * 4;
    if (!g_pD3D12TextureManager->Create(
            *t, w, h, (int)DXGI_FORMAT_R8G8B8A8_UNORM, &mip, 1))
    {
        delete t;
        return 0;
    }
    return (ID3D11ShaderResourceView*)t;
}

