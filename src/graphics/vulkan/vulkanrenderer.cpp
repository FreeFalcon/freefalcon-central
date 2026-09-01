//-----------------------------------------------------------------------------
// VulkanRenderer.cpp -- Artscout - 2026 (#104, Linux port -- subsystem 2).
// IRenderer implementation on Vulkan. Foundation: shares VulkanBackend's device/queue/command buffers, keeps the
// legacy render-state block, loads SPIR-V pipelines, and implements the 2D screen path (BeginScreenPass + DrawTL/
// DrawTLIndexed + textured tris) end to end. The object/terrain/cloud/particle paths are built up incrementally on
// this spine (they carry the same state block + pipeline pattern); until each lands its draw records nothing, but
// the state it needs is already tracked -- this is staged real work, not throwaway stubs.
//-----------------------------------------------------------------------------
#include <ciso646> // not/and/or tokens under MSVC
#include "vulkanrenderer.h"
#include "vulkanvma.h"
#include "../include/frameprof.h" // #107 PERF: FrameProf_CountDraw
#include "vulkanbackend.h"
#include "vulkantexturemanager.h"
#include "vulkanvbmanager.h"
#include "../shaders/ffshaderblobs.h" // #78: SPIR-V for the mesh-shader terrain

#include <vulkan/vulkan.h>

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex> // #107: the bindless free-list is touched from the loader thread
#include <chrono>

// Artscout - 2026 (#104): the ONE legacy STATE_* -> {FF_* flags, blend, filter, addressing, depth} table, shared with
// D3D12Renderer. Dependency-free since the STATE_* enum moved to ffstates.h, so it compiles into the Linux lib too.
// Path is relative to THIS file, the same way VulkanRenderer.h reaches ../dxengine/common/irenderer.h: the Windows
// project and the Linux CMake lib have different include roots, and only a file-relative path resolves under both.
// Lowercase is the real filename -- MSVC would not care, a case-sensitive Linux filesystem would.
#include "../dxengine/common/ffstatemap.h"
#ifdef _WIN32
#include <wincodec.h> // WIC image decode for LoadTextureFile (controller-hand / cursor PNGs etc.)
#else
// Artscout - 2026 (#104): Linux has no WIC -- decode LoadTextureFile's PNG/JPG/TGA/BMP assets with stb_image (vendored,
// v2.30). Implementation lives ONLY in this TU. Restrict to the formats the game actually ships to keep the build lean.
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_TGA
#define STBI_ONLY_BMP
#include "stb_image.h"
// The engine's fopen shim (win32compat.cpp) case-folds paths against the (lowercase) data tree; stb_image goes straight
// to the real fopen, so resolve the case ourselves before handing it a path (assets ship mixed-case names the .mtl/.obj
// references disagree with, e.g. hands_baseColor.png on disk as hands_basecolor.png).
extern "C" int FF_CIResolvePath(const char* want, char* out, unsigned long cap);
#endif

#undef min
#undef max

namespace
{
constexpr int kDynVbBytes =
    16 * 1024 * 1024; // per-frame dynamic vertex ring for 2D/screen draws.
// Artscout - 2026 (VR hands): was 4 MB and overflowed by ~5% every frame in VR (the ODS log showed 3000+
// frames dropping 5304 verts each) -- the whole cockpit 2D plus the sensor terrain spans plus the two
// painter-sorted skinned gloves (drawn late in the VR tail) share this one ring, and the gloves, being
// last, are exactly what got dropped -> partial/flickering hands. 16 MB (x kFrames host-visible) clears it.
constexpr int kFrames = 2;
// Artscout - 2026 (#104): object draws per FRAME -- it sizes both the per-draw UBO ring and the descriptor pool.
// It was 4096 while the counters were rewound by every BeginObjectPass, i.e. it was really a per-PASS budget. Moving
// the rewind to the true frame boundary (SyncFrame) turned the same number into a whole-frame budget without
// resizing it, and a real cockpit frame (terrain + world objects + the pit's BSP + the RTT quads) goes past it --
// at which point the later draws were dropped in silence. A capture of a frame with no cockpit already showed ~1230.
// 16384 leaves room; the cost is uboStride (~1.3 KB) x this x kFrames of host-visible memory, about 42 MB.
// Artscout - 2026 (sensor video): 16384 is NOT enough for VR quad-views with a sensor page up. The GPU terrain
// alone is ~13k draws, the pit BSP + world objects ride on top, and the TGP bracket adds the sensor's world
// objects PER VIEW (x4) -- a far look across an airbase pushed the frame over the ceiling, and everything
// recorded after the overflow silently vanished: the HUD/DED went BLANK exactly while the pod aimed far, and
// "floated back in" as the cursor slewed near (smaller footprint -> fewer draws -> back under budget). 32768
// doubles the headroom (~84 MB host-visible with kFrames=2) and the exhaustion warning is now ODS-visible.
constexpr uint32_t kMaxDrawsPerFrame = 32768;
} // namespace

VulkanRenderer* g_pVulkanRenderer =
    nullptr; // active renderer, mirrored into g_pRenderer by DXContext::Init

struct VulkanRenderer::Impl
{
    VulkanBackend* backend = nullptr;
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice phys = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    bool valid = false;
    std::string shaderDir;

    int viewportW = 0, viewportH = 0;

    // ---- legacy render-state block (mirrors what D3D12Renderer tracks; consumed by pipeline selection + UBOs) ----
    float view[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    float proj[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    float world[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    // Artscout - 2026 (#107 VR-Vulkan multiview): view-instancing params, the peer of D3D12Renderer's cbViewStereo.
    // When viActive, the UBO's view[gl_ViewIndex]/proj[gl_ViewIndex] are built from the LIVE base view/proj (viWorldOff
    // rotated into view space, shifted into the translation row) + per-view viProj -- exactly like D3D12. 0 views off.
    bool viActive = false;
    int viCount = 0;
    float viWorldOff[4][3] = {};
    float viProj[4][16] = {};
    bool viHasProj = false;
    float camPos[3] = {0, 0, 0};
    int legacyState = 0;
    // Artscout - 2026 (#104): the shadow of the legacy STATE_* bundle, filled by SetState through FFMapState (the same
    // table D3D12Renderer uses). stateFlags are the FF_* bits the state itself implies; the per-draw word ORs the
    // sticky/dynamic bits on top (see RecordObjectDraw). blend/depthWrite/depthTest/filter/addr pick the pipeline and
    // sampler at draw time -- in Vulkan they are baked into the PSO/sampler, so they cannot be set as loose state.
    unsigned stateFlags = 0;
    // Depth-bias bucket, mirroring D3D12Renderer::m_bias: 0 = none, 1 = objects (pulled toward the camera, #16),
    // 2 = terrain (pushed away so it sinks under coplanar runway/objects instead of z-fighting, #78).
    int biasMode = 0;
    FFBlendMode blend = BLEND_OPAQUE;
    FFFilterMode filter = FILTER_LINEAR;
    FFAddrMode addr = ADDR_WRAP;
    bool depthWrite = true, depthTest = true;
    // FF_TEXCOLORDIFFUSE is armed by SetTexColorDiffuse and CONSUMED by the next SetState (one-shot, as in D3D12):
    // stateTexColorDiffuse is what the draw actually reads.
    bool stateTexColorDiffuse = false;
    unsigned long chromaArgb = 0;
    float chromaTol = 0;
    unsigned long fogArgb = 0;
    float fogStart = 0, fogEnd = 1;
    float alphaRef = 0;
    float matColor[4] = {1, 1, 1, 1};
    float matSpec[4] = {0, 0, 0, 0};
    bool alphaTest = false;
    bool emissive = false, afterburner = false, cockpitPass = false,
         irGrey = false, nvg = false, fullBright = false;
    bool texColorDiffuse = false, forcePerSample = false;
    bool skyNoFog =
        false; // #15: BeginSkyPass sets this to suppress FF_FOG for the dome WITHOUT zeroing the fog range
    bool glocActive = false;
    float glocParams[4] = {0, 0, 0,
                           0}; // FF_GLOC vignette (intensity, innerR, outerR)
    int stencilMode = 0;
    unsigned stencilRef = 0;
    int hudStencil = 0;
    VulkanTexture* boundTex[4] =
        {}; // opaque texture handles bound per slot (VulkanTexture*)

    // ---- UI-over-3D composite (CompositeUISurface): a persistent HOST-VISIBLE linear RGBA image updated by memcpy
    // each frame from the 565 UI (black -> alpha 0), then drawn as a full-screen alpha quad into the scene. ----
    uint64_t uiImg = 0, uiMem = 0,
             uiView = 0; // VkImage / VkDeviceMemory / VkImageView
    void* uiMapped = nullptr;
    int uiW = 0, uiH = 0;
    VulkanTexture
        uiTex{}; // wraps uiImg/uiView so SetTexture/DescriptorFor bind it

    // ---- textures + descriptors ----
    VulkanTextureManager* texMgr = nullptr;
    // Artscout - 2026 (#104): textures created for ONE draw (DrawBitmap2D's splash/cursor blit) cannot be destroyed
    // when that call returns -- the GPU has not read them yet, and VulkanTextureManager::Destroy is immediate. Retire
    // them with a frame countdown instead and free them once no in-flight frame can still reference them.
    struct RetiredTex
    {
        VulkanTexture* tex;
        int framesLeft;
    };
    std::vector<RetiredTex> retiredTex;
    VulkanTexture* whiteTex = nullptr;
    VkDescriptorPool descPool =
        VK_NULL_HANDLE; // sets live in tex->descriptor (see DescriptorFor), not a map here

    // ---- screen (2D) pipeline ----
    VkDescriptorSetLayout dsLayout = VK_NULL_HANDLE;
    VkPipelineLayout screenLayout = VK_NULL_HANDLE;
    // One screen pipeline per (topology, render pass) -- see GetScreenPipe. The render pass is part of the key
    // because the 2D path draws into TWO different passes: the flat swapchain pass (menus, and the RTT atlas, whose
    // pass is deliberately swapchain-compatible) and the multiview SCENE pass (the cockpit's RTT composite quad,
    // which must land on the panels rather than the swapchain). Those passes differ in colour format, so a pipeline
    // built for one is not bindable in the other. The shader modules stay alive to build more variants on demand.
    std::unordered_map<uint64_t, VkPipeline> screenPipes;
    VkShaderModule screenVs = VK_NULL_HANDLE, screenFs = VK_NULL_HANDLE;
    VkRenderPass screenRp =
        VK_NULL_HANDLE; // the pass the current screen draws are inside (BeginScreenPass)
    // Per-path RTT viewport assert cache (see AssertRttViewport): keyed on the backend's viewport serial, NOT the
    // command buffer pointer -- the RTT ring recycles pointers, so a reused buffer would false-hit a pointer key.
    unsigned rttVpSerial = ~0u;
    int rttVpKind = 0; // 1 = full extent (2D), 2 = sensor zone (objects)
    VkSampler samplers[2][2] =
        {}; // [FFFilterMode][FFAddrMode] -- selected per draw from the state
    VkSampler linearSampler =
        VK_NULL_HANDLE; // alias of samplers[LINEAR][CLAMP] (2D/composite paths)

    VkDescriptorSet DescriptorFor(VulkanTexture* tex);

    // per-frame dynamic vertex ring
    VkBuffer dynVb[kFrames] = {};
    VkDeviceMemory dynVbMem[kFrames] = {};
    void* dynVbMap[kFrames] = {};
    uint32_t dynVbUsed = 0;
    uint32_t frame = 0;

    // per-frame dynamic index ring (DrawTLIndexed / terrain / dynamic-2D)
    VkBuffer idxBuf[kFrames] = {};
    VkDeviceMemory idxMem[kFrames] = {};
    void* idxMap[kFrames] = {};
    uint32_t idxUsed = 0;

    // Artscout - 2026 (#78): mesh-shader terrain clipmap -- post/info array
    // images plus a per-frame staging ring for the strips the camera uncovers.
    VkImage clipPostImg = VK_NULL_HANDLE;
    VkImage clipInfoImg = VK_NULL_HANDLE;
    void* clipPostAlloc = nullptr;
    void* clipInfoAlloc = nullptr;
    VkImageView clipPostView = VK_NULL_HANDLE;
    VkImageView clipInfoView = VK_NULL_HANDLE;
    VkBuffer clipBounds = VK_NULL_HANDLE;
    void* clipBoundsAlloc = nullptr;
    void* clipBoundsMap = nullptr;
    VkBuffer clipStage[kFrames] = {};
    void* clipStageAlloc[kFrames] = {};
    void* clipStageMap[kFrames] = {};
    uint32_t clipStageSize = 0;
    uint32_t clipStageUsed = 0;
    unsigned char clipLayerReady[8] = {}; // layer left UNDEFINED until first fill
    int clipTexels = 0;

    // #78 mesh pipeline: set0 = clipmap (UBO + 2 images + bounds + sampler),
    // set1 = the tiles as SAMPLED_IMAGE (HLSL splits image and sampler).
    VkDescriptorSetLayout meshSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout meshTileLayout = VK_NULL_HANDLE;
    VkDescriptorPool meshPool = VK_NULL_HANDLE;
    VkDescriptorPool meshTilePool = VK_NULL_HANDLE;
    VkDescriptorSet meshSet[kFrames] = {};
    VkDescriptorSet meshTileSet = VK_NULL_HANDLE;
    VkPipelineLayout meshLayout = VK_NULL_HANDLE;
    // One pipeline per render pass: the sensor RTT pass and the multiview scene
    // pass alternate within a frame, and a single slot rebuilt it every time.
    struct MeshPipeEntry
    {
        VkRenderPass rp;
        VkPipeline pipe;
    };
    std::vector<MeshPipeEntry> meshPipes;
    VkBuffer meshUbo[kFrames] = {};
    void* meshUboAlloc[kFrames] = {};
    void* meshUboMap[kFrames] = {};

    // #78: clipmap uploads are recorded OUTSIDE the render pass, so they queue
    // up here and flush when the scene pass is about to open.
    struct ClipCopy
    {
        uint32_t slot, offPost, offInfo;
        int level, x, y, w, h;
    };
    std::vector<ClipCopy> clipPending;
    int clipLevels = 0;
    int clipTiles = 0;

    // ---- object/terrain pipeline (3D, multiview scene render pass) ----
    // One light of the object path. Byte-identical to GpuLightCPU (irenderer.h) and to GpuLight in ffemu.hlsl, so
    // the engine's array copies straight in: 4x float4 = 64 bytes, which is also its std140 size AND alignment.
    struct GpuLightUbo
    {
        float position[4]; // xyz, w = 1 point / 0 directional
        float direction[4]; // xyz normalized
        float color[4]; // rgb
        float params[4]; // x = range, y = type (0 = directional/sun, 1 = point)
    };
    static_assert(sizeof(GpuLightUbo) == 64,
                  "GpuLightUbo must stay 64 bytes (std140; the engine light "
                  "array copies in raw)");
    static const int kMaxLights = 8; // == MAX_LIGHTS in ffemu.hlsl

    struct ObjUbo
    { // std140; mirrors ffobject.vert/.frag UBO
        float world[16];
        float view[4][16];
        float proj[4][16];
        float materialColor[4];
        float ambient[4];
        float camPos
            [4]; // xyz = camera world position (specular view vector); w unused
        float params[4]; // x = lit
        uint32_t flags[4]; // x = gFlags bitmask (FF_*), rest reserved
        uint32_t numLights[4]; // x = active entries in lights[], rest reserved
        float fogColor[4]; // rgb = fog colour
        float fogParams
            [4]; // x = fog start, y = fog end, z = alphaRef, w = time (sec)
        float chromaKey[4]; // rgb = chroma colour, w = tolerance
        float spec[4]; // material specular rgb + w = power (0 = no highlight)
        float gloc
            [4]; // FF_GLOC vignette: x = intensity, y = innerR, z = outerR, w unused
        GpuLightUbo lights
            [kMaxLights]; // the scene's real lights (sun + dynamic lamps), from SetLights
    };
    VkDescriptorSetLayout objDsLayout =
        VK_NULL_HANDLE; // binding0 = UBO, binding1 = gTex0, binding2 = gTex1
    VkPipelineLayout objLayout = VK_NULL_HANDLE;
    // #107 PERF bindless terrain: an unbounded sampler2D[] descriptor (set=1). The whole terrain -- ~13k distinct tile
    // textures -- becomes ONE draw whose vertices carry a texture INDEX into this array, instead of ~13k per-tile draws
    // each rebinding a descriptor set. Peer of D3D12's SRV heap. Populated on demand: BindlessIndexFor(tex) assigns the
    // next slot and writes the array once; the index is stable for the texture's lifetime.
    static const uint32_t kBindlessMax = 16384;
    bool bindlessReady = false;
    VkDescriptorSetLayout bindlessLayout =
        VK_NULL_HANDLE; // set=1, binding0 = sampler2D bindlessTex[kBindlessMax]
    VkDescriptorPool bindlessPool = VK_NULL_HANDLE;
    VkDescriptorSet bindlessSet = VK_NULL_HANDLE;
    uint32_t bindlessNext =
        0; // next free array slot (slots live in VulkanTexture::bindlessSlot)
    // Slots of destroyed textures, reused before bindlessNext advances.
    // Textures die on the LOADER thread while the render thread hands slots
    // out, so the list needs its own lock.
    std::vector<uint32_t> bindlessFree;
    std::mutex bindlessFreeMutex;
    bool bindlessDraw =
        false; // set by DrawTerrainMeshBindless: this draw sets FF_BINDLESS + binds set 1
    uint32_t BindlessIndexFor(
        VulkanTexture*
            tex); // #107 PERF: get/assign a bindless slot, write the descriptor once
    // Artscout - 2026 (#104): blend / depth-write / depth-test / topology are immutable pipeline state in Vulkan, so
    // every combination the legacy states ask for needs its own pipeline. There used to be four fixed ones, which
    // could not express the combos that matter most -- e.g. RTT_SOFT wants additive with the depth TEST off, and the
    // old additive pipeline tested depth, so the HUD/MFD composite would hide behind the panels it belongs on.
    // Built lazily and cached (a handful in practice); the shader modules are kept alive to build more on demand.
    std::unordered_map<uint32_t, VkPipeline> objPipes;
    VkShaderModule objVs = VK_NULL_HANDLE, objFs = VK_NULL_HANDLE;
    // The scene render pass the cached pipelines were built against. EnsureSceneTarget DESTROYS and rebuilds that
    // pass every time the scene target changes size -- i.e. on every 3D entry -- and a VkPipeline is permanently
    // bound to its render pass. So a cache that outlives the pass hands back pipelines pointing at a freed object.
    // Comparing the handle is enough to NOTICE the swap only if we purge on change: Vulkan is free to hand out the
    // same handle value again, so a stale entry could otherwise be found by key and silently reused.
    VkRenderPass cachedSceneRp = VK_NULL_HANDLE;
    // ---- instanced particle pipeline (billboards: unit quad slot0 + per-instance stream slot1) ----
    VkPipeline partPipeAlpha = VK_NULL_HANDLE; // alpha blend
    VkPipeline partPipeAdd = VK_NULL_HANDLE; // additive blend
    VkBuffer quadVB =
        VK_NULL_HANDLE; // static unit quad (4 verts, triangle strip)
    VkDeviceMemory quadVBMem = VK_NULL_HANDLE;
    bool drawLit = true; // false for unlit 2D effects
    bool dyn2dAdditive = false; // BeginDynamic2D blend mode
    const void* dyn2DVerts =
        nullptr; // staged by UploadDynamic2D; gathered by DrawDynamic2DIndexed
    int dyn2DVcount = 0;
    VkDescriptorPool objPool =
        VK_NULL_HANDLE; // #107 PERF: pre-allocated sets (NOT reset per frame)
    // #107 PERF: PRE-ALLOCATED object descriptor sets, kMaxDrawsPerFrame per frame-in-flight. The old code did
    // vkAllocateDescriptorSets in RecordObjectDraw ON EVERY DRAW (~15k/frame over dense terrain) -- that pool churn was
    // the whole 26x gap vs D3D12 (which never allocates per draw). Now the sets exist once; a draw only UPDATES its
    // slot's set and binds it. Double-buffered by m->frame so a set is never updated while the previous frame using it
    // is still on the GPU (kFrames in flight, gated by the frame fence).
    std::vector<VkDescriptorSet> objSets[kFrames];
    // #107 PERF: per-command-buffer bind cache -- skip re-binding the pipeline / re-setting depth bias when unchanged
    // (terrain is ~13k draws sharing one pipeline+bias). Keyed by curCmd so a new pass (new cmd) forces a re-bind.
    VkPipeline boundObjPipe = VK_NULL_HANDLE;
    VkCommandBuffer boundObjPipeCmd = VK_NULL_HANDLE;
    float boundBiasConst = 1e30f, boundBiasSlope = 1e30f;
    VkCommandBuffer boundBiasCmd = VK_NULL_HANDLE;
    VkCommandBuffer boundBindlessCmd =
        VK_NULL_HANDLE; // #107: set-1 (tile array) bound once per command buffer
    VkBuffer uboRing[kFrames] = {};
    VkDeviceMemory uboRingMem[kFrames] = {};
    void* uboRingMap[kFrames] = {};
    uint32_t uboSlot = 0; // draws this frame
    uint32_t frameCounter =
        0; // monotonic; only used to rate-limit per-frame warnings
    uint32_t uboStride = 0; // aligned sizeof(ObjUbo)
    float ambient[4] = {0.35f, 0.35f, 0.40f, 1.0f};
    int numLights = 0;
    GpuLightUbo lights[kMaxLights] =
        {}; // filled by SetLights; uploaded per draw by RecordObjectDraw
    bool inObjectPass = false;

    // current pass command buffer (flat swapchain pass for the 2D path)
    VkCommandBuffer curCmd = VK_NULL_HANDLE;

    VkShaderModule LoadShader(FFShaderId id) const;
    bool CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags props, VkBuffer& buf,
                      VkDeviceMemory& mem, void** mapped) const;
};

// Artscout - 2026 (#104): ObjUbo is memcpy'd straight into a std140 uniform block, so C++ and GLSL must agree on every
// offset. Nothing checks this at runtime -- a mismatch does not fail, it silently feeds the shader the wrong field
// (the light array read as fog, say), which surfaces as an inexplicable lighting bug. These are the offsets
// glslangValidator reflects out of ffobject.vert; when adding a member, update BOTH sides and re-check with
// `glslangValidator -V -q shaders/ffobject.vert`.
static_assert(offsetof(VulkanRenderer::Impl::ObjUbo, world) == 0,
              "ObjUbo/std140 drift: world");
static_assert(offsetof(VulkanRenderer::Impl::ObjUbo, view) == 64,
              "ObjUbo/std140 drift: view");
static_assert(offsetof(VulkanRenderer::Impl::ObjUbo, proj) == 320,
              "ObjUbo/std140 drift: proj");
static_assert(offsetof(VulkanRenderer::Impl::ObjUbo, materialColor) == 576,
              "ObjUbo/std140 drift: materialColor");
static_assert(offsetof(VulkanRenderer::Impl::ObjUbo, ambient) == 592,
              "ObjUbo/std140 drift: ambient");
static_assert(offsetof(VulkanRenderer::Impl::ObjUbo, camPos) == 608,
              "ObjUbo/std140 drift: camPos");
static_assert(offsetof(VulkanRenderer::Impl::ObjUbo, params) == 624,
              "ObjUbo/std140 drift: params");
static_assert(offsetof(VulkanRenderer::Impl::ObjUbo, flags) == 640,
              "ObjUbo/std140 drift: flags");
static_assert(offsetof(VulkanRenderer::Impl::ObjUbo, numLights) == 656,
              "ObjUbo/std140 drift: numLights");
static_assert(offsetof(VulkanRenderer::Impl::ObjUbo, fogColor) == 672,
              "ObjUbo/std140 drift: fogColor");
static_assert(offsetof(VulkanRenderer::Impl::ObjUbo, fogParams) == 688,
              "ObjUbo/std140 drift: fogParams");
static_assert(offsetof(VulkanRenderer::Impl::ObjUbo, chromaKey) == 704,
              "ObjUbo/std140 drift: chromaKey");
static_assert(offsetof(VulkanRenderer::Impl::ObjUbo, spec) == 720,
              "ObjUbo/std140 drift: spec");
static_assert(offsetof(VulkanRenderer::Impl::ObjUbo, gloc) == 736,
              "ObjUbo/std140 drift: gloc");
static_assert(offsetof(VulkanRenderer::Impl::ObjUbo, lights) == 752,
              "ObjUbo/std140 drift: lights");
static_assert(sizeof(VulkanRenderer::Impl::GpuLightUbo) == 64,
              "GpuLightUbo must match GpuLightCPU and the std140 array stride");
static_assert(sizeof(VulkanRenderer::Impl::ObjUbo) == 1264,
              "ObjUbo/std140 drift: total size");

// ---------------------------------------------------------------------------------------------- helpers
// Artscout - 2026 (#78): shaders come from the bytecode compiled into the
// binary (one HLSL source per pass, DXC'd at build time), not from .spv files.
VkShaderModule VulkanRenderer::Impl::LoadShader(FFShaderId id) const
{
    unsigned int size = 0;
    const void* code = FFGetShaderBlob(id, FFSHADER_SPIRV, &size);
    if (!code || !size)
    {
        fprintf(stderr, "[Vulkan] shader blob missing: %s\n",
                FFGetShaderName(id));
        return VK_NULL_HANDLE;
    }
    VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    ci.codeSize = (size_t)size;
    ci.pCode = (const uint32_t*)code;
    VkShaderModule mod = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device, &ci, nullptr, &mod) != VK_SUCCESS)
    {
        fprintf(stderr, "[Vulkan] vkCreateShaderModule failed: %s\n",
                FFGetShaderName(id));
        return VK_NULL_HANDLE;
    }
    return mod;
}

// Lazily allocate a combined-image-sampler descriptor set for this texture's view. The set is stored IN the
// texture (tex->descriptor), NOT in an external map keyed by the VulkanTexture* pointer. That distinction is the
// whole fix for VUID-vkCmdDraw-None-08114 ("gTex0 uses imageView 0x0 that was destroyed"): a map keyed by pointer
// hands back a stale set when the allocator reuses a freed texture's address for a new texture. A field on the
// object cannot alias -- a freshly constructed VulkanTexture always starts with descriptor == 0, so it gets its own.
VkDescriptorSet VulkanRenderer::Impl::DescriptorFor(VulkanTexture* tex)
{
    if (!tex || !tex->view)
        return VK_NULL_HANDLE;
    if (tex->descriptor)
        return (VkDescriptorSet)tex->descriptor;

    VkDescriptorSetAllocateInfo ai{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    ai.descriptorPool = descPool;
    ai.descriptorSetCount = 1;
    ai.pSetLayouts = &dsLayout;
    VkDescriptorSet set = VK_NULL_HANDLE;
    VkResult ar = vkAllocateDescriptorSets(device, &ai, &set);
    if (ar != VK_SUCCESS)
    {
        // VK_ERROR_OUT_OF_POOL_MEMORY (-1000069000) is an ORDINARY return code -- no validation layer reports it, so
        // a pool running dry looks like a clean log while textures silently vanish (every draw whose texture could
        // not get a set falls back to the previously-bound one / white). This is exactly the "textures disappear
        // when the camera turns" symptom: a fast pan streams more unique terrain/object textures than the pool holds.
        static uint32_t lastWarnFrame = 0xFFFFFFFFu;
        if (lastWarnFrame != frameCounter)
        {
            lastWarnFrame = frameCounter;
            fprintf(stderr,
                    "[Vulkan] DescriptorFor: vkAllocateDescriptorSets = %d "
                    "(pool exhausted?) -- textures will "
                    "drop this frame. Raise the sampler descriptor pool.\n",
                    (int)ar);
        }
        return VK_NULL_HANDLE;
    }

    VkDescriptorImageInfo di{};
    di.sampler = linearSampler;
    di.imageView = (VkImageView)tex->view;
    di.imageLayout = tex->sampleGeneral ?
                         VK_IMAGE_LAYOUT_GENERAL :
                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    w.dstSet = set;
    w.dstBinding = 0;
    w.descriptorCount = 1;
    w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w.pImageInfo = &di;
    vkUpdateDescriptorSets(device, 1, &w, 0, nullptr);

    tex->descriptor = (uint64_t)set;
    return set;
}

// #107 PERF bindless: get (or assign) this texture's slot in the bindless array. First use writes the descriptor once;
// the index is then stable. Returns 0xFFFFFFFF when bindless is unavailable or full -> the caller draws that tile the
// old per-tile way. The terrain sampler (LINEAR/WRAP) is baked in -- terrain is the only bindless user for now.
uint32_t VulkanRenderer::Impl::BindlessIndexFor(VulkanTexture* tex)
{
    if (!bindlessReady || !tex || !tex->view)
        return 0xFFFFFFFFu;
    // Slot stored IN the texture (+1 bias so 0 == unassigned). A freshly constructed VulkanTexture starts at 0, so a
    // recycled pointer/handle can never inherit a previous texture's slot (the anti-aliasing rule DescriptorFor uses).
    if (tex->bindlessSlot)
        return tex->bindlessSlot - 1;
    // Reuse a destroyed texture's slot first: leaving 3D and coming back
    // recreates every tile, and without reuse the array runs out.
    uint32_t idx = 0;
    bool reused = false;

    {
        std::lock_guard<std::mutex> lk(bindlessFreeMutex);

        if (!bindlessFree.empty())
        {
            idx = bindlessFree.back();
            bindlessFree.pop_back();
            reused = true;
        }
    }

    if (reused)
    {
        // recycled slot -- fall through
    }
    else if (bindlessNext >= kBindlessMax)
    {
        return 0xFFFFFFFFu; // array full -> caller falls back to gTex0
    }
    else
    {
        idx = bindlessNext++;
    }
    tex->bindlessSlot = idx + 1;
    VkDescriptorImageInfo ii{};
    ii.sampler = samplers[FILTER_LINEAR][ADDR_WRAP];
    ii.imageView = (VkImageView)tex->view;
    ii.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w.dstSet = bindlessSet;
    w.dstBinding = 0;
    w.dstArrayElement = idx;
    w.descriptorCount = 1;
    w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w.pImageInfo = &ii;
    vkUpdateDescriptorSets(device, 1, &w, 0, nullptr);

    // Artscout - 2026 (#78): the same tile, mirrored into the SAMPLED_IMAGE
    // array. HLSL compiles to separate image + sampler, which cannot bind to the
    // combined array above; the slot numbers stay identical in both.
    if (meshTileSet != VK_NULL_HANDLE)
    {
        VkDescriptorImageInfo si{};
        si.imageView = (VkImageView)tex->view;
        si.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkWriteDescriptorSet sw{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        sw.dstSet = meshTileSet;
        sw.dstBinding = 0;
        sw.dstArrayElement = idx;
        sw.descriptorCount = 1;
        sw.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        sw.pImageInfo = &si;
        vkUpdateDescriptorSets(device, 1, &sw, 0, nullptr);
    }
    return idx;
}

bool VulkanRenderer::Impl::CreateBuffer(VkDeviceSize size,
                                        VkBufferUsageFlags usage,
                                        VkMemoryPropertyFlags props,
                                        VkBuffer& buf, VkDeviceMemory& mem,
                                        void** mapped) const
{
    // VMA-backed. `mem` (the old VkDeviceMemory slot) now carries the opaque VmaAllocation; every
    // caller frees through FF_VmaBufferDestroy. Host-visible requests come back persistently mapped
    // and COHERENT (the rings write through the map with no per-draw flush).
    VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bi.size = size;
    bi.usage = usage;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    const bool hostMapped = (props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;
    void* alloc = nullptr;
    if (not FF_VmaBufferCreate(&bi, hostMapped, &buf, &alloc, mapped))
        return false;
    mem = (VkDeviceMemory)alloc;
    return true;
}

// ---------------------------------------------------------------------------------------------- ctor/dtor
VulkanRenderer::VulkanRenderer(VulkanBackend* backend) : m(new Impl())
{
    m->backend = backend;
}
VulkanRenderer::~VulkanRenderer()
{
    Release();
    delete m;
    m = nullptr;
}

// ---------------------------------------------------------------------------------------------- lifecycle
bool VulkanRenderer::Init(const char* shaderDir)
{
    if (!m->backend || !m->backend->IsValid())
        return false;
    m->device = (VkDevice)m->backend->VkDeviceHandle();
    m->phys = (VkPhysicalDevice)m->backend->VkPhysicalDeviceHandle();
    m->queue = (VkQueue)m->backend->VkQueueHandle();
    m->shaderDir = shaderDir ? shaderDir : "";

    // Artscout - 2026 (#104): one sampler per (filter, addressing) combination FFMapState can select. Both axes are
    // load-bearing, not cosmetic: FILTER_POINT keeps a font glyph from bilinearly bleeding the next atlas row, and
    // ADDR_CLAMP stops a cell-edge glyph UV from wrapping and pulling that row in anyway (#7, "HUD text floats").
    for (int f = 0; f < 2; ++f)
        for (int a = 0; a < 2; ++a)
        {
            VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
            si.magFilter = si.minFilter =
                (f == FILTER_POINT) ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
            si.mipmapMode = (f == FILTER_POINT) ?
                                VK_SAMPLER_MIPMAP_MODE_NEAREST :
                                VK_SAMPLER_MIPMAP_MODE_LINEAR;
            si.addressModeU = si.addressModeV = si.addressModeW =
                (a == ADDR_CLAMP) ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE :
                                    VK_SAMPLER_ADDRESS_MODE_REPEAT;
            si.maxLod = VK_LOD_CLAMP_NONE;
            if (vkCreateSampler(m->device, &si, nullptr, &m->samplers[f][a]) !=
                VK_SUCCESS)
                return false;
        }
    m->linearSampler =
        m->samplers[FILTER_LINEAR]
                   [ADDR_CLAMP]; // the 2D/composite paths' fixed choice

    // descriptor set layout: 1 combined image sampler at binding 0 (gTex0)
    VkDescriptorSetLayoutBinding b{};
    b.binding = 0;
    b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    b.descriptorCount = 1;
    b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutCreateInfo dl{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    dl.bindingCount = 1;
    dl.pBindings = &b;
    if (vkCreateDescriptorSetLayout(m->device, &dl, nullptr, &m->dsLayout) !=
        VK_SUCCESS)
        return false;

    // pipeline layout: push constant = viewport size (2 floats) for screen->NDC in the VS
    // VERTEX|FRAGMENT: the vert reads viewport/flipY, the frag reads the RTT-soft flag (emissive composite). See
    // ffscreen.frag -- without the flag the composite multiplies the blend alpha by the atlas alpha, so any symbology
    // written with a zero-alpha colour (the radar lines/contour/waterline) contributes nothing and vanishes on the MFD.
    VkPushConstantRange pc{VK_SHADER_STAGE_VERTEX_BIT |
                               VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(float) * 4};
    VkPipelineLayoutCreateInfo pl{
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pl.setLayoutCount = 1;
    pl.pSetLayouts = &m->dsLayout;
    pl.pushConstantRangeCount = 1;
    pl.pPushConstantRanges = &pc;
    if (vkCreatePipelineLayout(m->device, &pl, nullptr, &m->screenLayout) !=
        VK_SUCCESS)
        return false;

    // per-frame dynamic vertex ring (host-visible)
    for (int i = 0; i < kFrames; ++i)
        if (!m->CreateBuffer(kDynVbBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             m->dynVb[i], m->dynVbMem[i], &m->dynVbMap[i]))
            return false;

    // Descriptor pool for per-texture combined-image-sampler sets -- ONE set per live texture (see DescriptorFor).
    // 4096 was too small: the engine caches terrain tiles + world-object + cockpit textures, and a fast camera pan
    // brings far more than 4096 unique textures live at once. Past the cap vkAllocateDescriptorSets returns
    // OUT_OF_POOL (a silent ordinary code -- no validation error), DescriptorFor hands back null, and every such
    // texture falls back to the 1x1 white default -> "all textures turn white when the camera turns". 64k sets are
    // a few MB of driver bookkeeping -- cheap next to the textures themselves.
    const uint32_t kSamplerSets = 65536;
    VkDescriptorPoolSize ps{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            kSamplerSets};
    VkDescriptorPoolCreateInfo dp{
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    dp.maxSets = kSamplerSets;
    dp.poolSizeCount = 1;
    dp.pPoolSizes = &ps;
    dp.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    if (vkCreateDescriptorPool(m->device, &dp, nullptr, &m->descPool) !=
        VK_SUCCESS)
        return false;

    // texture manager + a 1x1 white default (bound when a draw has no texture, so gTex0 is always valid)
    m->texMgr = new VulkanTextureManager(m->backend);
    if (!m->texMgr->Init())
        return false;
    m->texMgr->BindDescriptorPool(
        (uint64_t)m
            ->descPool); // so a freed texture returns its descriptor set to the pool
    m->whiteTex = m->texMgr->White();

    // ---- object/terrain path: index ring, UBO ring, descriptor layout/pipeline-layout/pool ----
    for (int i = 0; i < kFrames; ++i)
        if (!m->CreateBuffer(kDynVbBytes, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             m->idxBuf[i], m->idxMem[i], &m->idxMap[i]))
            return false;

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(m->phys, &props);
    VkDeviceSize uboAlign = props.limits.minUniformBufferOffsetAlignment;
    if (uboAlign == 0)
        uboAlign = 256;
    m->uboStride =
        (uint32_t)(((sizeof(Impl::ObjUbo) + uboAlign - 1) / uboAlign) *
                   uboAlign);

    for (int i = 0; i < kFrames; ++i)
        if (!m->CreateBuffer((VkDeviceSize)m->uboStride * kMaxDrawsPerFrame,
                             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             m->uboRing[i], m->uboRingMem[i],
                             &m->uboRingMap[i]))
            return false;

    // binding 0 = the per-draw UBO, 1 = gTex0, 2 = gTex1 (FF_TEXTURE1: the terrain's day/night second stage, which
    // the shader ADDs onto stage 0 -- a multiply is what used to make the ground black).
    // Artscout - 2026 (#78): binding 3 is the sampler the HLSL shader pairs with
    // the bindless tile ARRAY -- an array cannot be a combined descriptor.
    VkDescriptorSetLayoutBinding ob[4] = {};
    ob[0].binding = 0;
    ob[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ob[0].descriptorCount = 1;
    ob[0].stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    ob[1].binding = 1;
    ob[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ob[1].descriptorCount = 1;
    ob[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    ob[2].binding = 2;
    ob[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ob[2].descriptorCount = 1;
    ob[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    ob[3].binding = 3;
    ob[3].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    ob[3].descriptorCount = 1;
    ob[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutCreateInfo odl{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    odl.bindingCount = 4;
    odl.pBindings = ob;
    if (vkCreateDescriptorSetLayout(m->device, &odl, nullptr,
                                    &m->objDsLayout) != VK_SUCCESS)
        return false;

    // #107 PERF bindless terrain (set=1): an UPDATE_AFTER_BIND, PARTIALLY_BOUND unbounded sampler2D[] array. Built only
    // when the device enabled descriptor-indexing; the object pipeline layout then has 2 sets and the shader can sample
    // bindlessTex[index]. When unsupported, m->bindlessReady stays false and TerrainGpu falls back to per-tile draws.
    // Artscout - 2026: and only when asked for -- same knob as the D3D12 side.
    extern bool g_bEnableBindless;

    m->bindlessReady =
        g_bEnableBindless && m->backend && m->backend->BindlessSupported();
    if (m->bindlessReady)
    {
        VkDescriptorSetLayoutBinding bb{};
        bb.binding = 0;
        bb.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bb.descriptorCount = Impl::kBindlessMax;
        bb.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        VkDescriptorBindingFlags bf =
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
            VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
            VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
        VkDescriptorSetLayoutBindingFlagsCreateInfo bfi{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
        bfi.bindingCount = 1;
        bfi.pBindingFlags = &bf;
        VkDescriptorSetLayoutCreateInfo bl{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        bl.pNext = &bfi;
        bl.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        bl.bindingCount = 1;
        bl.pBindings = &bb;
        VkDescriptorPoolSize bps{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                 Impl::kBindlessMax};
        VkDescriptorPoolCreateInfo bdp{
            VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        bdp.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        bdp.maxSets = 1;
        bdp.poolSizeCount = 1;
        bdp.pPoolSizes = &bps;
        if (vkCreateDescriptorSetLayout(m->device, &bl, nullptr,
                                        &m->bindlessLayout) != VK_SUCCESS ||
            vkCreateDescriptorPool(m->device, &bdp, nullptr,
                                   &m->bindlessPool) != VK_SUCCESS)
        {
            m->bindlessReady = false;
        }
        else
        {
            VkDescriptorSetAllocateInfo bsi{
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            bsi.descriptorPool = m->bindlessPool;
            bsi.descriptorSetCount = 1;
            bsi.pSetLayouts = &m->bindlessLayout;
            if (vkAllocateDescriptorSets(m->device, &bsi, &m->bindlessSet) !=
                VK_SUCCESS)
                m->bindlessReady = false;
        }
    }

    // Artscout - 2026 (#78): the same tiles as SAMPLED_IMAGE, for the HLSL mesh
    // pipeline. Slots match the combined array above, filled in BindlessIndexFor.
    if (m->bindlessReady)
    {
        VkDescriptorSetLayoutBinding tb{};
        tb.binding = 0;
        tb.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        tb.descriptorCount = Impl::kBindlessMax;
        tb.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorBindingFlags tf =
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
            VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
            VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
        VkDescriptorSetLayoutBindingFlagsCreateInfo tfi{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
        tfi.bindingCount = 1;
        tfi.pBindingFlags = &tf;
        VkDescriptorSetLayoutCreateInfo tl{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        tl.pNext = &tfi;
        tl.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        tl.bindingCount = 1;
        tl.pBindings = &tb;

        VkDescriptorPoolSize tps{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                                 Impl::kBindlessMax};
        VkDescriptorPoolCreateInfo tdp{
            VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        tdp.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        tdp.maxSets = 1;
        tdp.poolSizeCount = 1;
        tdp.pPoolSizes = &tps;

        if (vkCreateDescriptorSetLayout(m->device, &tl, nullptr,
                                        &m->meshTileLayout) == VK_SUCCESS &&
            vkCreateDescriptorPool(m->device, &tdp, nullptr,
                                   &m->meshTilePool) == VK_SUCCESS)
        {
            VkDescriptorSetAllocateInfo tsi{
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            tsi.descriptorPool = m->meshTilePool;
            tsi.descriptorSetCount = 1;
            tsi.pSetLayouts = &m->meshTileLayout;
            if (vkAllocateDescriptorSets(m->device, &tsi, &m->meshTileSet) !=
                VK_SUCCESS)
            {
                m->meshTileSet = VK_NULL_HANDLE;
            }
        }
    }

    // Object pipeline layout: set0 = objDsLayout (UBO + 2 tex + sampler), set1 =
    // the bindless tiles. #78: the SAMPLED_IMAGE set, since HLSL splits image
    // and sampler; the combined set stays for anything still on the old path.
    VkDescriptorSetLayout objSetLayouts[2] = {m->objDsLayout,
                                              m->meshTileLayout};
    VkPipelineLayoutCreateInfo opl{
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    opl.setLayoutCount =
        (m->bindlessReady && m->meshTileLayout != VK_NULL_HANDLE) ? 2u : 1u;
    opl.pSetLayouts = objSetLayouts;
    if (vkCreatePipelineLayout(m->device, &opl, nullptr, &m->objLayout) !=
        VK_SUCCESS)
        return false;

    // #107 PERF: sized for ALL frames-in-flight worth of pre-allocated sets (kMaxDrawsPerFrame * kFrames). No per-frame
    // reset -> the sets persist; a draw only updates+binds its slot.
    VkDescriptorPoolSize ops[3] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, kMaxDrawsPerFrame * kFrames},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         kMaxDrawsPerFrame * kFrames * 2}, // gTex0 + gTex1
        // #78: binding 3, the bindless array's sampler.
        {VK_DESCRIPTOR_TYPE_SAMPLER, kMaxDrawsPerFrame * kFrames}};
    VkDescriptorPoolCreateInfo odp{
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    odp.maxSets = kMaxDrawsPerFrame * kFrames;
    odp.poolSizeCount = 3;
    odp.pPoolSizes = ops;
    if (vkCreateDescriptorPool(m->device, &odp, nullptr, &m->objPool) !=
        VK_SUCCESS)
        return false;

    // #107 PERF: pre-allocate every object descriptor set ONCE (kMaxDrawsPerFrame per frame-in-flight). Batched allocate
    // -- one vkAllocateDescriptorSets per frame with kMaxDrawsPerFrame identical layouts. RecordObjectDraw then only
    // updates+binds its slot's set, never allocates.
    {
        std::vector<VkDescriptorSetLayout> layouts(kMaxDrawsPerFrame,
                                                   m->objDsLayout);
        for (int f = 0; f < kFrames; ++f)
        {
            m->objSets[f].resize(kMaxDrawsPerFrame);
            VkDescriptorSetAllocateInfo bai{
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            bai.descriptorPool = m->objPool;
            bai.descriptorSetCount = kMaxDrawsPerFrame;
            bai.pSetLayouts = layouts.data();
            if (vkAllocateDescriptorSets(m->device, &bai,
                                         m->objSets[f].data()) != VK_SUCCESS)
            {
                fprintf(stderr,
                        "[Vulkan] pre-allocate object descriptor sets FAILED "
                        "(frame %d)\n",
                        f);
                return false;
            }
        }
        // #107 PERF: the UBO binding (binding 0) of set[f][slot] is FIXED -- always uboRing[f] at offset slot*stride.
        // Write it ONCE here so RecordObjectDraw never re-writes binding 0 (it only updates the 2 texture bindings that
        // actually change per draw). Batched: one vkUpdateDescriptorSets per frame with kMaxDrawsPerFrame writes.
        for (int f = 0; f < kFrames; ++f)
        {
            std::vector<VkDescriptorBufferInfo> binfo(kMaxDrawsPerFrame);
            std::vector<VkWriteDescriptorSet> uw(kMaxDrawsPerFrame);
            for (uint32_t s = 0; s < kMaxDrawsPerFrame; ++s)
            {
                binfo[s].buffer = m->uboRing[f];
                binfo[s].offset = (VkDeviceSize)s * m->uboStride;
                binfo[s].range = sizeof(Impl::ObjUbo);
                uw[s] = {};
                uw[s].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                uw[s].dstSet = m->objSets[f][s];
                uw[s].dstBinding = 0;
                uw[s].descriptorCount = 1;
                uw[s].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                uw[s].pBufferInfo = &binfo[s];
            }
            vkUpdateDescriptorSets(m->device, kMaxDrawsPerFrame, uw.data(), 0,
                                   nullptr);
        }
    }

    // The screen pipeline is still built lazily on the first BeginScreenPass;
    // its shaders are embedded, so nothing can be missing at Init any more.
    m->valid = true;
    fprintf(stderr, "[Vulkan] VulkanRenderer::Init OK (embedded shaders)\n");
    return true;
}

// Load the screen shader modules once; the topology variants are built on demand (see GetScreenPipe).
static bool EnsureScreenPipe(VulkanRenderer::Impl* m)
{
    if (m->screenVs && m->screenFs)
        return true;
    m->screenVs = m->LoadShader(FFSHADER_SCREEN_VS);
    m->screenFs = m->LoadShader(FFSHADER_SCREEN_PS);
    if (!m->screenVs || !m->screenFs)
    {
        if (m->screenVs)
        {
            vkDestroyShaderModule(m->device, m->screenVs, nullptr);
            m->screenVs = VK_NULL_HANDLE;
        }
        if (m->screenFs)
        {
            vkDestroyShaderModule(m->device, m->screenFs, nullptr);
            m->screenFs = VK_NULL_HANDLE;
        }
        return false;
    }
    return true;
}

// Artscout - 2026 (#104): the 2D screen pipeline for one topology, built against the render pass the draw will
// actually be inside (m->screenRp -- see BeginScreenPass). Two reasons this is keyed the way it is:
//   topology: there used to be a single TRIANGLE_LIST pipeline and DrawTL ignored primType, so every line/point list
//             the 2D engine submitted rasterized as triangles. D3D12 picks the topology per draw (TopoOf).
//   render pass: the same 2D path feeds both the flat swapchain pass and the multiview SCENE pass, and a pipeline is
//             only bindable inside a COMPATIBLE pass (same colour format) -- the two differ.
static VkPipeline GetScreenPipe(VulkanRenderer::Impl* m,
                                VkPrimitiveTopology topo, FFBlendMode blend)
{
    VkRenderPass rp = m->screenRp ?
                          m->screenRp :
                          (VkRenderPass)m->backend->SwapchainRenderPass();
    if (!rp)
        return VK_NULL_HANDLE;
    // #76 HUD aperture stencil lives on THIS 2D/TL screen path too: the collimated HUD glass-plate MARK and the
    // RTT-quad TEST are transformed-lit draws that route through DrawTL -> RecordScreenDraw -> here, NOT GetObjPipe.
    // Without baking + keying the stencil here, MARK never stamps and TEST never clips, so the HUD spills past the
    // combiner aperture. Gated on DepthHasStencil() -- hudStencil is only non-zero while drawing into the scene/RTT pass
    // (which carries a D32S8 plane); the flat swapchain variant keeps hudStencil==0 and stays stencil-free.
    const int hudStencil = m->backend->DepthHasStencil() ? m->hudStencil : 0;
    const uint64_t key = ((uint64_t)(uintptr_t)rp) ^ ((uint64_t)topo << 1) ^
                         ((uint64_t)blend << 4) ^ ((uint64_t)hudStencil << 6);
    auto it = m->screenPipes.find(key);
    if (it != m->screenPipes.end())
        return it->second;
    if (!EnsureScreenPipe(m))
        return VK_NULL_HANDLE;

    VkPipelineShaderStageCreateInfo stages[2] = {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
         VK_SHADER_STAGE_VERTEX_BIT, m->screenVs, "VS_Screen", nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
         VK_SHADER_STAGE_FRAGMENT_BIT, m->screenFs, "PS_Screen", nullptr},
    };
    // ScreenVertex: float4 pos(0), uint color(16), uint specular(20), float2 uv0(24), float2 uv1(32) -> stride 40
    VkVertexInputBindingDescription bind{0, 40, VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription attrs[3] = {
        {0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0}, // pos (screen xyz + rhw)
        {1, 0, VK_FORMAT_B8G8R8A8_UNORM,
         16}, // color: D3DCOLOR ARGB -> BGRA bytes -> vec4(R,G,B,A) in shader
        {2, 0, VK_FORMAT_R32G32_SFLOAT, 24}, // uv0
    };
    VkPipelineVertexInputStateCreateInfo vi{
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vi.vertexBindingDescriptionCount = 1;
    vi.pVertexBindingDescriptions = &bind;
    vi.vertexAttributeDescriptionCount = 3;
    vi.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo ia{
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ia.topology = topo;

    VkPipelineViewportStateCreateInfo vp{
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vp.viewportCount = 1;
    vp.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rs{
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo ds{
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    ds.depthTestEnable = VK_FALSE;
    ds.depthWriteEnable = VK_FALSE; // 2D screen pass: no depth
    // #76 HUD aperture stencil, mirror of GetObjPipe. MARK (1) stamps 0x80 where the cockpit bit (0x40) is clear;
    // TEST (2) draws only where 0x80 is set (writes nothing). Independent of the disabled depth test.
    if (hudStencil != 0)
    {
        VkStencilOpState so{};
        so.compareOp = VK_COMPARE_OP_EQUAL;
        so.compareMask = (hudStencil == 1) ? 0x40u : 0xC0u;
        so.writeMask = (hudStencil == 1) ? 0x80u : 0x00u;
        so.reference = 0x80u;
        so.passOp =
            (hudStencil == 1) ? VK_STENCIL_OP_REPLACE : VK_STENCIL_OP_KEEP;
        so.failOp = VK_STENCIL_OP_KEEP;
        so.depthFailOp = VK_STENCIL_OP_KEEP;
        ds.stencilTestEnable = VK_TRUE;
        ds.front = so;
        ds.back = so;
    }

    // Honour the blend state, exactly as the object pipeline does. This used to be hardwired to alpha blend, which
    // silently killed any SOLID-state symbology whose vertex alpha was 0 -- the radar page draws its blips and scan
    // symbology under STATE_SOLID with a foreground colour that carries no alpha byte (0x00RRGGBB), so alpha=0, and
    // under a forced SRC_ALPHA blend that is fully transparent. RenderDoc showed it exactly: correct geometry in VS
    // Output, vColor.w = 0, nothing on the atlas. D3D12 gets it right because GetPSO keys on m_blend (SOLID -> opaque,
    // so vertex alpha is ignored). Now Vulkan matches: SOLID is opaque, and the alpha states blend as before.
    VkPipelineColorBlendAttachmentState cba{};
    cba.colorBlendOp = VK_BLEND_OP_ADD;
    cba.alphaBlendOp = VK_BLEND_OP_ADD;
    cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    switch (blend)
    {
    case BLEND_ADDITIVE:
        cba.blendEnable = VK_TRUE;
        cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        break;
    case BLEND_PREMUL:
        cba.blendEnable = VK_TRUE;
        cba.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        break;
    case BLEND_OPAQUE:
        cba.blendEnable =
            VK_FALSE; // SOLID: write the colour straight, vertex alpha irrelevant (parity with D3D12)
        break;
    case BLEND_ALPHA:
    default:
        cba.blendEnable = VK_TRUE;
        cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        break;
    }
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo cb{
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cb.attachmentCount = 1;
    cb.pAttachments = &cba;

    VkDynamicState dyn[2] = {VK_DYNAMIC_STATE_VIEWPORT,
                             VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo ds2{
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    ds2.dynamicStateCount = 2;
    ds2.pDynamicStates = dyn;

    VkGraphicsPipelineCreateInfo gp{
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    gp.stageCount = 2;
    gp.pStages = stages;
    gp.pVertexInputState = &vi;
    gp.pInputAssemblyState = &ia;
    gp.pViewportState = &vp;
    gp.pRasterizationState = &rs;
    gp.pMultisampleState = &ms;
    gp.pDepthStencilState = &ds;
    gp.pColorBlendState = &cb;
    gp.pDynamicState = &ds2;
    gp.layout = m->screenLayout;
    gp.renderPass =
        rp; // the pass this variant is for -- swapchain/RTT or the multiview scene
    gp.subpass = 0;
    VkPipeline pipe = VK_NULL_HANDLE;
    if (vkCreateGraphicsPipelines(m->device, VK_NULL_HANDLE, 1, &gp, nullptr,
                                  &pipe) != VK_SUCCESS)
        return VK_NULL_HANDLE;
    m->screenPipes[key] = pipe;
    return pipe;
}

// The legacy primitive type -> Vulkan topology, mirroring D3D12Renderer's TopoOf(). 4 and 6 (emulated) are triangles.
static VkPrimitiveTopology TopoOfPrimType(int primType)
{
    switch (primType)
    {
    case 1:
        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case 2:
        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case 3:
        return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    case 5:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    default:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}
static bool PrimTypeIsLineOrPoint(int primType)
{
    return primType == 1 || primType == 2 || primType == 3;
}

// Artscout - 2026 (#104): key one object pipeline variant -- everything FFMapState (or the #76 HUD stencil) can ask
// for that Vulkan bakes into the pipeline: topology | depthWrite | depthTest | blend | hudStencil(0..2).
// The topology used to be a single `strip` bool, which is why a BSP surface's dwPrimType was thrown away and every
// object draw came out as a triangle list -- see DrawObjectIndexed.
static uint32_t ObjPipeKey(VkPrimitiveTopology topo, FFBlendMode blend,
                           bool depthWrite, bool depthTest, int hudStencil,
                           bool menu, bool tail, bool bindless,
                           bool rtt = false)
{
    return (rtt ? (1u << 12) : 0u) | (bindless ? (1u << 11) : 0u) |
           (tail ? (1u << 10) : 0u) |
           (menu ? (1u << 9) : 0u) | ((uint32_t)(hudStencil & 3) << 7) |
           ((uint32_t)(blend & 3) << 5) | (depthWrite ? 16u : 0u) |
           (depthTest ? 8u : 0u) | ((uint32_t)topo & 7u);
}

// Load the object shader modules once; the variants are built on demand from them (see GetObjPipe).
static bool EnsureObjectPipe(VulkanRenderer::Impl* m)
{
    if (m->objVs && m->objFs)
        return true;
    if (!m->backend->GetSceneRenderPass())
        return false; // scene target not created yet (EnsureSceneTarget)
    m->objVs = m->LoadShader(FFSHADER_OBJECT_VS);
    m->objFs = m->LoadShader(FFSHADER_OBJECT_PS);
    if (!m->objVs || !m->objFs)
    {
        if (m->objVs)
        {
            vkDestroyShaderModule(m->device, m->objVs, nullptr);
            m->objVs = VK_NULL_HANDLE;
        }
        if (m->objFs)
        {
            vkDestroyShaderModule(m->device, m->objFs, nullptr);
            m->objFs = VK_NULL_HANDLE;
        }
        return false;
    }
    return true;
}

// Build-or-fetch the object pipeline for this state bundle.
static VkPipeline GetObjPipe(VulkanRenderer::Impl* m, VkPrimitiveTopology topo,
                             FFBlendMode blend, bool depthWrite, bool depthTest,
                             bool bindless = false)
{
    // #76: the HUD aperture stencil is pipeline state too, and only meaningful when the depth buffer has a stencil
    // plane -- otherwise it silently disables itself rather than producing an invalid pipeline.
    const int hudStencil = m->backend->DepthHasStencil() ? m->hudStencil : 0;
    // #107 VR-Vulkan: the in-3D menu renders the exit-dialog BSP through this object path into its OWN sceneFormat
    // single-view pass (menuPass) -- a distinct pipeline variant, keyed apart so it never collides with the eye's.
    const bool menu = m->backend->IsMenuActive();
    // #107 Option 2: the per-eye tail pass (single-view, cockpit BSP) is a DISTINCT pipeline variant -- keyed apart and
    // built against TailRenderPass -- so the 2-view multiview objPipe is never (incompatibly) reused in the 1-view tail.
    const bool tail = m->backend->IsTailActive();
    // Sensor scene (TGP/MAV/FLIR video): the object path draws into the OPEN display-RTT pass -- a
    // distinct 1-view scFormat pipeline variant, same precedent as the in-3D menu variant above.
    // RTT outranks tail: the sensor flush runs INSIDE the tail/cockpit phase with the display RTT
    // open, and its object draws belong to the RTT (this mirrors the branch order in VkBeginScenePass).
    const bool rtt = (not menu) and m->backend->IsRttActive();
    const uint32_t key = ObjPipeKey(topo, blend, depthWrite, depthTest,
                                    hudStencil, menu, tail, bindless, rtt);
    auto it = m->objPipes.find(key);
    if (it != m->objPipes.end())
        return it->second;
    if (!EnsureObjectPipe(m))
        return VK_NULL_HANDLE;
VkRenderPass sceneRp =
        rtt  ? (VkRenderPass)m->backend->RttRenderPass() :
        tail ? (VkRenderPass)m->backend->TailRenderPass() :
        menu ? (VkRenderPass)m->backend->MenuRenderPass() :
               (VkRenderPass)m->backend->GetSceneRenderPass();
    if (!sceneRp)
        return VK_NULL_HANDLE;

    VkPipelineShaderStageCreateInfo stages[2] = {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
         VK_SHADER_STAGE_VERTEX_BIT, m->objVs, "VS_Object", nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
         VK_SHADER_STAGE_FRAGMENT_BIT, m->objFs, "PS_Object", nullptr},
    };
    // ObjV: pos(0) normal(12) col(24) spec(28) uv(32) -> stride 40. The #107 bindless terrain variant appends a
    // per-vertex tile slot (uint @ 40) -> stride 44; both variants declare location 5 so the shader always reads it,
    // but non-bindless points it in-bounds at offset 0 (a dummy uint the frag ignores when FF_BINDLESS is clear -- no
    // out-of-bounds fetch and no undefined value that could confuse validation).
    VkVertexInputBindingDescription bind{0, bindless ? 44u : 40u,
                                         VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription attrs[6] = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12},
        {2, 0, VK_FORMAT_B8G8R8A8_UNORM, 24},
        {3, 0, VK_FORMAT_B8G8R8A8_UNORM, 28},
        {4, 0, VK_FORMAT_R32G32_SFLOAT, 32},
        {5, 0, VK_FORMAT_R32_UINT, bindless ? 40u : 0u},
    };
    VkPipelineVertexInputStateCreateInfo vi{
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vi.vertexBindingDescriptionCount = 1;
    vi.pVertexBindingDescriptions = &bind;
    vi.vertexAttributeDescriptionCount = 6;
    vi.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo ia{
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ia.topology = topo;
    VkPipelineViewportStateCreateInfo vp{
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vp.viewportCount = 1;
    vp.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo rs{
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE; // no caller sets cull; D3D12 is NONE too
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;
    // Depth bias is DYNAMIC (vkCmdSetDepthBias per draw), not baked: the object pass pulls toward the camera and the
    // terrain pass pushes away, and keying a pipeline per bias value would multiply the cache for one raster field.
    rs.depthBiasEnable = VK_TRUE;
    VkPipelineMultisampleStateCreateInfo ms{
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo ds{
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    ds.depthTestEnable = depthTest ? VK_TRUE : VK_FALSE;
    ds.depthWriteEnable = depthWrite ? VK_TRUE : VK_FALSE;
    ds.depthCompareOp = VK_COMPARE_OP_GREATER; // reversed-Z (near = 1, far = 0)
    // #76 HUD aperture stencil, ported from D3D12Renderer::GetPSO. The reference is 0x80 and bit 0x40 is the cockpit
    // bit, so with compareOp EQUAL:
    //   MARK (1) -- compare 0x80 & 0x40 == buf & 0x40, i.e. pass where the cockpit bit is CLEAR, and stamp 0x80.
    //   TEST (2) -- compare 0x80 & 0xC0 == buf & 0xC0, i.e. draw only where 0x80 is set and 0x40 clear; write nothing.
    if (hudStencil != 0)
    {
        VkStencilOpState so{};
        so.compareOp = VK_COMPARE_OP_EQUAL;
        so.compareMask = (hudStencil == 1) ? 0x40u : 0xC0u;
        so.writeMask = (hudStencil == 1) ? 0x80u : 0x00u;
        so.reference = 0x80u;
        so.passOp =
            (hudStencil == 1) ? VK_STENCIL_OP_REPLACE : VK_STENCIL_OP_KEEP;
        so.failOp = VK_STENCIL_OP_KEEP;
        so.depthFailOp = VK_STENCIL_OP_KEEP;
        ds.stencilTestEnable = VK_TRUE;
        ds.front = so;
        ds.back = so;
    }
    VkPipelineColorBlendAttachmentState cba{};
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    cba.colorBlendOp = VK_BLEND_OP_ADD;
    cba.alphaBlendOp = VK_BLEND_OP_ADD;
    cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    switch (blend)
    {
    case BLEND_ALPHA:
        cba.blendEnable = VK_TRUE;
        cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        break;
    case BLEND_ADDITIVE: // SRC_ALPHA, ONE -- the emissive composite (FF_RTTSOFT) and tracers/flashes
        cba.blendEnable = VK_TRUE;
        cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        break;
    case BLEND_PREMUL: // ONE, INV_SRC_ALPHA -- rgb already weighted by its own coverage
        cba.blendEnable = VK_TRUE;
        cba.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        break;
    case BLEND_OPAQUE:
    default:
        cba.blendEnable = VK_FALSE;
        break;
    }
    VkPipelineColorBlendStateCreateInfo cb{
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cb.attachmentCount = 1;
    cb.pAttachments = &cba;
    VkDynamicState dyn[3] = {VK_DYNAMIC_STATE_VIEWPORT,
                             VK_DYNAMIC_STATE_SCISSOR,
                             VK_DYNAMIC_STATE_DEPTH_BIAS};
    VkPipelineDynamicStateCreateInfo ds2{
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    ds2.dynamicStateCount = 3;
    ds2.pDynamicStates = dyn;

    VkGraphicsPipelineCreateInfo gp{
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    gp.stageCount = 2;
    gp.pStages = stages;
    gp.pVertexInputState = &vi;
    gp.pInputAssemblyState = &ia;
    gp.pViewportState = &vp;
    gp.pRasterizationState = &rs;
    gp.pMultisampleState = &ms;
    gp.pDepthStencilState = &ds;
    gp.pColorBlendState = &cb;
    gp.pDynamicState = &ds2;
    gp.layout = m->objLayout;
    gp.renderPass = sceneRp;
    gp.subpass = 0;
    VkPipeline pipe = VK_NULL_HANDLE;
    if (vkCreateGraphicsPipelines(m->device, VK_NULL_HANDLE, 1, &gp, nullptr,
                                  &pipe) != VK_SUCCESS)
        return VK_NULL_HANDLE;
    m->objPipes[key] = pipe;
    return pipe;
}

// The pipeline for the CURRENT legacy state (SetState / SetObjectAlphaBlend shadow) at this topology.
static VkPipeline StateObjPipe(VulkanRenderer::Impl* m,
                               VkPrimitiveTopology topo)
{
    return GetObjPipe(m, topo, m->blend, m->depthWrite, m->depthTest);
}

// Artscout - 2026 (#104): instanced particle billboard pipeline (peer of D3D12 GetParticlePSO). Reuses objDsLayout
// (UBO + atlas sampler) and objLayout, built against the multiview scene render pass. Two vertex bindings: slot 0 =
// static unit quad (corner.xy, uv.xy, per-vertex), slot 1 = per-instance stream (D3D12ParticleInstance, 44 bytes).
// depth test GREATER (reversed-Z), depth write OFF; alpha + additive variants. Also builds the static unit-quad VB.
static bool EnsureParticlePipe(VulkanRenderer::Impl* m)
{
    if (m->partPipeAlpha)
        return true;
    VkRenderPass sceneRp = (VkRenderPass)m->backend->GetSceneRenderPass();
    if (!sceneRp || !m->objLayout)
        return false;

    // static unit quad (triangle strip: TL, TR, BL, BR) -- corner in [-0.5,0.5], uv in [0,1] (uv.y=1 at world-up top).
    if (!m->quadVB)
    {
        struct QuadV
        {
            float cx, cy, u, v;
        };
        const QuadV q[4] = {{-0.5f, 0.5f, 0.0f, 1.0f},
                            {0.5f, 0.5f, 1.0f, 1.0f},
                            {-0.5f, -0.5f, 0.0f, 0.0f},
                            {0.5f, -0.5f, 1.0f, 0.0f}};
        void* map = nullptr;
        if (!m->CreateBuffer(sizeof(q), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             m->quadVB, m->quadVBMem, &map) ||
            !map)
            return false;
        memcpy(map, q, sizeof(q));
    }

    VkShaderModule vs = m->LoadShader(FFSHADER_PARTICLE_VS);
    VkShaderModule fs = m->LoadShader(FFSHADER_PARTICLE_PS);
    if (!vs || !fs)
    {
        if (vs)
            vkDestroyShaderModule(m->device, vs, nullptr);
        if (fs)
            vkDestroyShaderModule(m->device, fs, nullptr);
        return false;
    }
    VkPipelineShaderStageCreateInfo stages[2] = {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
         VK_SHADER_STAGE_VERTEX_BIT, vs, "VS_Particle", nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
         VK_SHADER_STAGE_FRAGMENT_BIT, fs, "PS_Particle", nullptr},
    };
    VkVertexInputBindingDescription binds[2] = {
        {0, 16, VK_VERTEX_INPUT_RATE_VERTEX}, // unit quad: corner(8) + uv(8)
        {1, 44, VK_VERTEX_INPUT_RATE_INSTANCE}, // D3D12ParticleInstance
    };
    VkVertexInputAttributeDescription attrs[7] = {
        {0, 0, VK_FORMAT_R32G32_SFLOAT, 0}, // corner
        {1, 0, VK_FORMAT_R32G32_SFLOAT, 8}, // quad uv
        {2, 1, VK_FORMAT_R32G32B32_SFLOAT, 0}, // center
        {3, 1, VK_FORMAT_R32G32_SFLOAT, 12}, // size
        {4, 1, VK_FORMAT_R32_SFLOAT, 20}, // rot
        {5, 1, VK_FORMAT_B8G8R8A8_UNORM, 24}, // color (D3DCOLOR)
        {6, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 28}, // uvRect
    };
    VkPipelineVertexInputStateCreateInfo vi{
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vi.vertexBindingDescriptionCount = 2;
    vi.pVertexBindingDescriptions = binds;
    vi.vertexAttributeDescriptionCount = 7;
    vi.pVertexAttributeDescriptions = attrs;
    VkPipelineInputAssemblyStateCreateInfo ia{
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    VkPipelineViewportStateCreateInfo vp{
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vp.viewportCount = 1;
    vp.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo rs{
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;
    VkPipelineMultisampleStateCreateInfo ms{
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo ds{
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_FALSE;
    ds.depthCompareOp = VK_COMPARE_OP_GREATER; // reversed-Z, no write
    VkPipelineColorBlendAttachmentState cba{};
    cba.blendEnable = VK_TRUE;
    cba.colorBlendOp = VK_BLEND_OP_ADD;
    cba.alphaBlendOp = VK_BLEND_OP_ADD;
    cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo cb{
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cb.attachmentCount = 1;
    cb.pAttachments = &cba;
    VkDynamicState dyn[2] = {VK_DYNAMIC_STATE_VIEWPORT,
                             VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo ds2{
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    ds2.dynamicStateCount = 2;
    ds2.pDynamicStates = dyn;
    VkGraphicsPipelineCreateInfo gp{
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    gp.stageCount = 2;
    gp.pStages = stages;
    gp.pVertexInputState = &vi;
    gp.pInputAssemblyState = &ia;
    gp.pViewportState = &vp;
    gp.pRasterizationState = &rs;
    gp.pMultisampleState = &ms;
    gp.pDepthStencilState = &ds;
    gp.pColorBlendState = &cb;
    gp.pDynamicState = &ds2;
    gp.layout = m->objLayout;
    gp.renderPass = sceneRp;
    gp.subpass = 0;
    VkResult r = vkCreateGraphicsPipelines(m->device, VK_NULL_HANDLE, 1, &gp,
                                           nullptr, &m->partPipeAlpha);
    if (r == VK_SUCCESS)
    {
        cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE; // additive
        r = vkCreateGraphicsPipelines(m->device, VK_NULL_HANDLE, 1, &gp,
                                      nullptr, &m->partPipeAdd);
    }
    vkDestroyShaderModule(m->device, vs, nullptr);
    vkDestroyShaderModule(m->device, fs, nullptr);
    return r == VK_SUCCESS;
}

void VulkanRenderer::Release()
{
    if (!m || m->device == VK_NULL_HANDLE)
        return;
    vkDeviceWaitIdle(m->device);
    // descriptor sets live in each texture (tex->descriptor) and die with the pool below -- nothing to clear here.
    if (m->uiView)
        vkDestroyImageView(m->device, (VkImageView)m->uiView, nullptr);
    if (m->uiImg)
        FF_VmaImageDestroy((VkImage)m->uiImg, (void*)m->uiMem);
    m->uiImg = m->uiMem = m->uiView = 0;
    m->uiMapped = nullptr;
    // Hand any still-counting-down DrawBitmap2D temp textures to the manager BEFORE releasing it -- once it is
    // deleted below the `if (m->texMgr)` guards go false and these would leak. Its own retire queue is drained in
    // Release() (the device is idle by now, so it frees immediately).
    for (auto& r : m->retiredTex)
        if (m->texMgr)
            m->texMgr->Destroy(r.tex);
    m->retiredTex.clear();
    if (m->texMgr)
    {
        m->texMgr->Release();
        delete m->texMgr;
        m->texMgr = nullptr;
        m->whiteTex = nullptr;
    }
    if (m->descPool)
        vkDestroyDescriptorPool(m->device, m->descPool, nullptr);
    m->descPool = VK_NULL_HANDLE;
    // object path
    for (auto& kv : m->objPipes)
        if (kv.second)
            vkDestroyPipeline(m->device, kv.second, nullptr);
    m->objPipes.clear();
    if (m->objVs)
    {
        vkDestroyShaderModule(m->device, m->objVs, nullptr);
        m->objVs = VK_NULL_HANDLE;
    }
    if (m->objFs)
    {
        vkDestroyShaderModule(m->device, m->objFs, nullptr);
        m->objFs = VK_NULL_HANDLE;
    }
    if (m->partPipeAlpha)
        vkDestroyPipeline(m->device, m->partPipeAlpha, nullptr);
    if (m->partPipeAdd)
        vkDestroyPipeline(m->device, m->partPipeAdd, nullptr);
    if (m->quadVB)
        FF_VmaBufferDestroy(m->quadVB, (void*)m->quadVBMem);
    m->partPipeAlpha = VK_NULL_HANDLE;
    m->partPipeAdd = VK_NULL_HANDLE;
    m->quadVB = VK_NULL_HANDLE;
    m->quadVBMem = VK_NULL_HANDLE;
    if (m->objLayout)
        vkDestroyPipelineLayout(m->device, m->objLayout, nullptr);
    if (m->objDsLayout)
        vkDestroyDescriptorSetLayout(m->device, m->objDsLayout, nullptr);
    if (m->objPool)
        vkDestroyDescriptorPool(m->device, m->objPool,
                                nullptr); // frees the pre-allocated sets too
    for (int f = 0; f < kFrames; ++f)
        m->objSets[f]
            .clear(); // #107 PERF: handles owned by the pool, just drop them
    // #107 PERF bindless cleanup
    if (m->bindlessPool)
    {
        vkDestroyDescriptorPool(m->device, m->bindlessPool, nullptr);
        m->bindlessPool = VK_NULL_HANDLE;
    }
    if (m->bindlessLayout)
    {
        vkDestroyDescriptorSetLayout(m->device, m->bindlessLayout, nullptr);
        m->bindlessLayout = VK_NULL_HANDLE;
    }
    m->bindlessSet = VK_NULL_HANDLE;
    m->bindlessNext = 0;
    m->bindlessReady = false;

    // Artscout - 2026 (#78): mesh-terrain clipmap + pipeline.
    for (size_t i = 0; i < m->meshPipes.size(); ++i)
    {
        if (m->meshPipes[i].pipe)
            vkDestroyPipeline(m->device, m->meshPipes[i].pipe, nullptr);
    }
    m->meshPipes.clear();
    if (m->meshLayout)
    {
        vkDestroyPipelineLayout(m->device, m->meshLayout, nullptr);
        m->meshLayout = VK_NULL_HANDLE;
    }
    if (m->meshPool)
    {
        vkDestroyDescriptorPool(m->device, m->meshPool, nullptr);
        m->meshPool = VK_NULL_HANDLE;
    }
    if (m->meshTilePool)
    {
        vkDestroyDescriptorPool(m->device, m->meshTilePool, nullptr);
        m->meshTilePool = VK_NULL_HANDLE;
    }
    if (m->meshSetLayout)
    {
        vkDestroyDescriptorSetLayout(m->device, m->meshSetLayout, nullptr);
        m->meshSetLayout = VK_NULL_HANDLE;
    }
    if (m->meshTileLayout)
    {
        vkDestroyDescriptorSetLayout(m->device, m->meshTileLayout, nullptr);
        m->meshTileLayout = VK_NULL_HANDLE;
    }
    m->meshTileSet = VK_NULL_HANDLE;
    for (int f = 0; f < kFrames; ++f)
    {
        m->meshSet[f] = VK_NULL_HANDLE;
        if (m->meshUbo[f])
        {
            FF_VmaBufferDestroy(m->meshUbo[f], m->meshUboAlloc[f]);
            m->meshUbo[f] = VK_NULL_HANDLE;
            m->meshUboAlloc[f] = nullptr;
            m->meshUboMap[f] = nullptr;
        }
        if (m->clipStage[f])
        {
            FF_VmaBufferDestroy(m->clipStage[f], m->clipStageAlloc[f]);
            m->clipStage[f] = VK_NULL_HANDLE;
            m->clipStageAlloc[f] = nullptr;
            m->clipStageMap[f] = nullptr;
        }
    }
    if (m->clipPostView)
    {
        vkDestroyImageView(m->device, m->clipPostView, nullptr);
        m->clipPostView = VK_NULL_HANDLE;
    }
    if (m->clipInfoView)
    {
        vkDestroyImageView(m->device, m->clipInfoView, nullptr);
        m->clipInfoView = VK_NULL_HANDLE;
    }
    if (m->clipPostImg)
    {
        FF_VmaImageDestroy(m->clipPostImg, m->clipPostAlloc);
        m->clipPostImg = VK_NULL_HANDLE;
        m->clipPostAlloc = nullptr;
    }
    if (m->clipInfoImg)
    {
        FF_VmaImageDestroy(m->clipInfoImg, m->clipInfoAlloc);
        m->clipInfoImg = VK_NULL_HANDLE;
        m->clipInfoAlloc = nullptr;
    }
    if (m->clipBounds)
    {
        FF_VmaBufferDestroy(m->clipBounds, m->clipBoundsAlloc);
        m->clipBounds = VK_NULL_HANDLE;
        m->clipBoundsAlloc = nullptr;
        m->clipBoundsMap = nullptr;
    }
    m->clipPending.clear();
    m->clipStageUsed = 0;
    m->clipTexels = m->clipLevels = m->clipTiles = 0;
    for (int i = 0; i < 8; ++i)
        m->clipLayerReady[i] = 0;
    m->objLayout = VK_NULL_HANDLE;
    m->objDsLayout = VK_NULL_HANDLE;
    m->objPool = VK_NULL_HANDLE;
    for (int i = 0; i < kFrames; ++i)
    {
        if (m->idxMem[i])
            FF_VmaBufferDestroy(m->idxBuf[i], (void*)m->idxMem[i]);
        if (m->uboRingMem[i])
            FF_VmaBufferDestroy(m->uboRing[i], (void*)m->uboRingMem[i]);
        m->idxBuf[i] = VK_NULL_HANDLE;
        m->idxMem[i] = VK_NULL_HANDLE;
        m->uboRing[i] = VK_NULL_HANDLE;
        m->uboRingMem[i] = VK_NULL_HANDLE;
    }
    for (auto& kv : m->screenPipes)
        if (kv.second)
            vkDestroyPipeline(m->device, kv.second, nullptr);
    m->screenPipes.clear();
    if (m->screenVs)
    {
        vkDestroyShaderModule(m->device, m->screenVs, nullptr);
        m->screenVs = VK_NULL_HANDLE;
    }
    if (m->screenFs)
    {
        vkDestroyShaderModule(m->device, m->screenFs, nullptr);
        m->screenFs = VK_NULL_HANDLE;
    }
    if (m->screenLayout)
        vkDestroyPipelineLayout(m->device, m->screenLayout, nullptr);
    if (m->dsLayout)
        vkDestroyDescriptorSetLayout(m->device, m->dsLayout, nullptr);
    if (m->linearSampler)
        vkDestroySampler(m->device, m->linearSampler, nullptr);
    for (int i = 0; i < kFrames; ++i)
    {
        if (m->dynVb[i])
            FF_VmaBufferDestroy(m->dynVb[i], (void*)m->dynVbMem[i]);
    }
    m->screenLayout = VK_NULL_HANDLE;
    m->dsLayout = VK_NULL_HANDLE;
    m->linearSampler = VK_NULL_HANDLE;
    for (int i = 0; i < kFrames; ++i)
    {
        m->dynVb[i] = VK_NULL_HANDLE;
        m->dynVbMem[i] = VK_NULL_HANDLE;
        m->dynVbMap[i] = nullptr;
    }
    // the device is BORROWED from the backend (we do not destroy it); null it so a second Release()/dtor after the
    // backend has torn the device down does not vkDeviceWaitIdle on a dead handle.
    m->device = VK_NULL_HANDLE;
    m->valid = false;
}

bool VulkanRenderer::IsValid() const
{
    return m && m->valid;
}

// ---------------------------------------------------------------------------------------------- state
void VulkanRenderer::SetViewportSize(int w, int h)
{
    m->viewportW = w;
    m->viewportH = h;
}
void VulkanRenderer::SetView(const float* v)
{
    if (v)
        memcpy(m->view, v, sizeof(m->view));
}
void VulkanRenderer::SetProj(const float* v)
{
    if (v)
        memcpy(m->proj, v, sizeof(m->proj));
}

// Artscout - 2026 (#107 VR-Vulkan multiview): peer of D3D12Renderer::SetViewInstancingParams. worldOff = per-view eye
// position offset in WORLD space (xyz*nViews); projs = per-view projection (row-major 4x4*nViews) or NULL for the base.
void VulkanRenderer::SetViewInstancingParams(int nViews, const float* worldOff,
                                             const float* projs)
{
    if (nViews < 1)
        nViews = 1;
    if (nViews > 4)
        nViews = 4;
    m->viCount = nViews;
    if (worldOff)
        for (int v = 0; v < nViews; ++v)
        {
            m->viWorldOff[v][0] = worldOff[v * 3 + 0];
            m->viWorldOff[v][1] = worldOff[v * 3 + 1];
            m->viWorldOff[v][2] = worldOff[v * 3 + 2];
        }
    m->viHasProj = (projs != 0);
    if (projs)
        for (int v = 0; v < nViews; ++v)
            memcpy(m->viProj[v], projs + v * 16, 64);
}
void VulkanRenderer::SetViewInstancing(bool on)
{
    m->viActive = on;
}

// Artscout - 2026 (#107 VR-Vulkan): rewind the per-frame draw rings for a NEW eye. VR renders up to 4 eyes (quad)
// within ONE real frame, but the UBO ring / object descriptor pool / dynamic VB+index rings are sized per FRAME
// (kMaxDrawsPerFrame). 4 eyes overflow them, so the later (focus) eyes' draws are DROPPED -> their terrain textures /
// RTT displays / cockpit vanish while the periphery (rendered first) is fine. Each eye's GPU work completes before the
// next (EndEye waits sceneFence), so rewinding per eye is safe and gives each eye the full budget. Flat path unaffected.
void VulkanRenderer::ResetFrameRing()
{
    m->dynVbUsed = 0;
    m->idxUsed = 0;
    m->uboSlot = 0;
    // #107 PERF: do NOT reset the object descriptor pool -- the sets are PRE-ALLOCATED and reused every frame (a draw
    // updates its slot's set). Resetting would destroy them. uboSlot=0 above already rewinds the per-frame slot index.
}
void VulkanRenderer::SetWorld(const float* v)
{
    if (v)
        memcpy(m->world, v, sizeof(m->world));
}
void VulkanRenderer::SetCameraPos(float x, float y, float z)
{
    m->camPos[0] = x;
    m->camPos[1] = y;
    m->camPos[2] = z;
}
// Artscout - 2026 (#104): translate one legacy STATE_* through the shared FFMapState table -- a faithful port of
// D3D12Renderer::SetState. This used to store the state number and nothing else, so the ENTIRE legacy state model
// (~41 bundles: which texture stages, chroma vs alpha vs additive, depth on/off, filtering) was silently dropped on
// Vulkan. FF_RTTSOFT is the loudest casualty: without it the HUD/MFD atlas quad falls through to a plain modulate and
// its black background paints an opaque rectangle over the instrument panel.
void VulkanRenderer::SetState(int s)
{
    m->legacyState = s;
    FFStateDesc d;
    FFMapState(
        s,
        d); // returns false for an unknown state, having already filled the SOLID fallback

    // FF_TEXCOLORDIFFUSE is armed by the caller and consumed HERE, and only by the two states that mean "the texture
    // is a glyph mask, the colour comes from the vertex" -- otherwise it would leak onto unrelated draws.
    m->stateTexColorDiffuse = false;
    if (m->texColorDiffuse)
    {
        if (s == STATE_TEXTURE_TEXT || s == STATE_CHROMA_TEXTURE_GOURAUD2)
            m->stateTexColorDiffuse = true;
        m->texColorDiffuse = false;
    }

    m->stateFlags = d.flags;
    // No real texture bound -> draw by vertex colour rather than sampling the white default (mirrors D3D12's
    // !m_hasTex0 mask). SetTexture re-adds the bit if a texture is bound afterwards -- the screen path does exactly
    // that (context.cpp FlushVB calls SetState first, then SetTexture), so last-writer-wins is the intended order.
    if (!m->boundTex[0])
        m->stateFlags &= ~FF_TEXTURE0;
    if (!m->boundTex[1])
        m->stateFlags &= ~FF_TEXTURE1;
    m->blend = d.blend;
    m->filter = d.filter;
    m->addr = d.addr;
    m->depthWrite = d.depthWrite;
    m->depthTest = d.depthTest;
}

// Artscout - 2026 (#104): binding a texture is what SETS FF_TEXTURE0 -- exactly as D3D12Renderer::SetTexture does.
// This matters because the OBJECT path (the cockpit/aircraft BSP) never calls SetState: only the screen path does
// (context.cpp FlushVB). So the object pass's base flag word carries no FF_TEXTURE0, and if binding a texture does not
// add it, every textured panel samples nothing and renders as blown-out white -- which is precisely what happened when
// the flag word started coming from the legacy state instead of being inferred from the binding.
void VulkanRenderer::SetTexture(unsigned slot, ID3D11ShaderResourceView* srv)
{
    if (slot >= 4)
        return;
    m->boundTex[slot] = reinterpret_cast<VulkanTexture*>(
        srv); // opaque handle IS a VulkanTexture*
    if (slot == 0)
    {
        if (srv)
            m->stateFlags |= FF_TEXTURE0;
        else
            m->stateFlags &= ~FF_TEXTURE0;
    }
    else if (slot == 1)
    {
        if (srv)
            m->stateFlags |= FF_TEXTURE1;
        else
            m->stateFlags &= ~FF_TEXTURE1;
    }
}
void VulkanRenderer::SetChromaKey(unsigned long argb, float tol)
{
    m->chromaArgb = argb;
    m->chromaTol = tol;
}
void VulkanRenderer::SetFog(unsigned long argb, float s, float e)
{
    m->fogArgb = argb;
    m->fogStart = s;
    m->fogEnd = e;
}
void VulkanRenderer::SetAlphaRef(float r)
{
    m->alphaRef = r;
}
void VulkanRenderer::SetMaterialColor(float r, float g, float b, float a)
{
    m->matColor[0] = r;
    m->matColor[1] = g;
    m->matColor[2] = b;
    m->matColor[3] = a;
}
void VulkanRenderer::SetMaterialSpecular(float r, float g, float b, float p)
{
    m->matSpec[0] = r;
    m->matSpec[1] = g;
    m->matSpec[2] = b;
    m->matSpec[3] = p;
}
// These override the blend the state selected, for the object path's hand-sorted translucent pass (mirrors D3D12).
void VulkanRenderer::SetObjectAlphaBlend(bool on)
{
    m->blend = on ? BLEND_ALPHA : BLEND_OPAQUE;
    m->depthWrite = !on;
    m->depthTest = true;
}
void VulkanRenderer::SetObjectAdditiveBlend(bool on)
{
    m->blend = on ? BLEND_ADDITIVE : BLEND_ALPHA;
}
void VulkanRenderer::SetAlphaTestEnabled(bool on)
{
    m->alphaTest = on;
}
void VulkanRenderer::SetEmissive(bool on)
{
    m->emissive = on;
}
void VulkanRenderer::SetAfterburner(bool on)
{
    m->afterburner = on;
}
void VulkanRenderer::SetCockpitPass(bool on)
{
    m->cockpitPass = on;
}
void VulkanRenderer::SetIRGrey(bool on)
{
    m->irGrey = on;
}
void VulkanRenderer::SetNvgMode(bool on)
{
    m->nvg = on;
}
void VulkanRenderer::SetFullBright(bool on)
{
    m->fullBright = on;
}
void VulkanRenderer::SetTexColorDiffuse(bool on)
{
    m->texColorDiffuse = on;
}
void VulkanRenderer::SetForcePerSample(bool on)
{
    m->forcePerSample = on;
}
void VulkanRenderer::SetStencil(int mode, unsigned ref)
{
    m->stencilMode = mode;
    m->stencilRef = ref;
}
void VulkanRenderer::SetHudStencil(int mode)
{
    m->hudStencil = mode;
}

// ---------------------------------------------------------------------------------------------- passes
static void
SyncFrame(VulkanRenderer::Impl* m); // defined below (with the passes it serves)

// Artscout - 2026 (#104): drop every cached pipeline when the SCENE render pass is replaced. EnsureSceneTarget
// destroys and rebuilds that pass whenever the scene target changes size, which is every 3D entry -- and a pipeline
// is bound to its render pass for life. Without this the second 3D entry draws with pipelines whose pass was freed
// on the first exit: the cockpit and the models disappear while the terrain (drawn before the stale cache is first
// consulted) still shows. Exactly the shape of the old hero-squares bug -- a cache that outlived a teardown.
// Safe to run here: EnsureSceneTarget does vkDeviceWaitIdle before it destroys anything, so nothing is in flight.
static void PurgePipelinesIfScenePassChanged(VulkanRenderer::Impl* m)
{
    VkRenderPass sceneRp = (VkRenderPass)m->backend->GetSceneRenderPass();
    if (!sceneRp || sceneRp == m->cachedSceneRp)
        return;
    for (auto& kv : m->objPipes)
        if (kv.second)
            vkDestroyPipeline(m->device, kv.second, nullptr);
    for (auto& kv : m->screenPipes)
        if (kv.second)
            vkDestroyPipeline(m->device, kv.second, nullptr);
    m->objPipes.clear();
    m->screenPipes
        .clear(); // includes the swapchain-pass variants; they are cheap to rebuild and this keeps the
    // rule simple: no pipeline outlives a render-pass generation.
    m->cachedSceneRp = sceneRp;
}

// Artscout - 2026 (#104): the 2D path draws into whichever pass is OPEN -- and during a 3D frame that is the SCENE,
// not the swapchain. This used to always take the flat command buffer, which is why the cockpit displays and the
// mouse cursor were invisible: both are 2D-path draws issued while the scene pass is recording (the RTT composite
// quad comes from VirtualDisplay::DrawRttQuad -> context FlushVB -> BeginScreenPass). They landed in the swapchain
// pass, and PresentScene then BLITS the finished scene over the whole swapchain image -- erasing them. RenderDoc
// showed it plainly: the atlas pass rendered its 118 symbology draws, and nothing sampled the result onto a panel.
// D3D12 does not have the problem because it has ONE command list and simply keeps the scene RTV bound.
void VulkanRenderer::BeginScreenPass()
{
    SyncFrame(
        m); // recycles the rings only if the backend actually moved to a new frame
    PurgePipelinesIfScenePassChanged(m);
    // ORDER MATTERS, and getting it wrong is what put the whole atlas on screen. A bound RTT wins: the displays
    // render their symbology mid-cockpit, i.e. while the SCENE is also recording, and it must go into the RTT they
    // just bound. Testing the scene first sent every glyph and line straight onto the scene at its ATLAS pixel
    // coordinates -- which is exactly what "the entire atlas, laid out as in the atlas, stuck to the camera" was.
    // #107 VR-Vulkan: the in-3D menu wins over everything -- its 2D comms/AWACS text (DisplayDraw) and cursor must land
    // in the menu pass on menuCmd, alongside the 3D exit dialog the object path already routes there. Checked first so
    // an incidentally-open display RTT or the eye scene never captures the menu's 2D. GetScreenPipe keys on the pass.
    if (m->backend->IsMenuActive())
    {
        m->curCmd = (VkCommandBuffer)m->backend->MenuCommandBuffer();
        m->screenRp = (VkRenderPass)m->backend->MenuRenderPass();
    }
    else if (m->backend->IsRttActive())
    {
        m->curCmd =
            (VkCommandBuffer)m->backend
                ->FlatCommandBuffer(); // returns rttCmd while an RTT is bound
        m->screenRp = (VkRenderPass)m->backend->RttRenderPass();
    }
    else if (m->backend->IsTailActive())
    {
        // #107 Option 2 VR: the per-eye RTT/2D tail (VCock_Exec/DrawRttQuad composite, cursor) draws into ONE scene-
        // array layer via the single-view tail pass -- checked before IsSceneRecording (the scene pass has already
        // ended). Its screen pipelines key on (topo, tailRenderPass) so they coexist with the scene-pass pipelines.
        m->curCmd = (VkCommandBuffer)m->backend->TailCommandBuffer();
        m->screenRp = (VkRenderPass)m->backend->TailRenderPass();
    }
    else if (m->backend->IsSceneRecording())
    {
        // No RTT: this is 2D over the 3D frame (the RTT composite quad, the cursor) -- it belongs in the scene,
        // because PresentScene blits the scene over the whole swapchain and would erase anything left behind.
        m->curCmd = (VkCommandBuffer)m->backend->SceneCommandBuffer();
        m->screenRp = (VkRenderPass)m->backend->GetSceneRenderPass();
    }
    else
    {
        // Neither is open. This is either the load splash (a genuine flat frame -- no scene this frame) or a stray
        // 2D call issued inside a 3D frame BEFORE the scene opened. We must NOT acquire a swapchain image here for
        // the latter: a 3D frame presents through PresentScene, which does its OWN acquire, so an EnsureFrameStarted
        // here opens a flat frame that PresentScene never presents -- an orphaned acquire that leaves the image
        // semaphore permanently pending (VUID-...-semaphore-01779) and loses the device. Only open the flat frame
        // when the backend already has one recording (the splash path opened it); otherwise leave curCmd null and
        // let the draw no-op, exactly as before. Splash is handled by its own present branch.
        m->curCmd = (VkCommandBuffer)m->backend->FlatCommandBuffer();
        m->screenRp = (VkRenderPass)m->backend->SwapchainRenderPass();
    }
    EnsureScreenPipe(m); // lazy build once SPIR-V is available
}
// Artscout - 2026 (#104): recycle the per-frame rings when -- and only when -- the frame actually changes.
// The renderer used to advance its own ring in BeginScreenPass and reset the offsets in BeginObjectPass, but neither
// is a frame boundary: the engine calls BeginScreenPass several times per frame (context.cpp FlushVB x2,
// display.cpp) and BeginObjectPass from FlushBuffers. So every batch rewound dynVbUsed to 0 and rewrote vertices the
// PREVIOUS batch's queued draws still point at, and flipped to the other ring buffer mid-frame -- data the GPU had
// not read yet. The backend's frameSlot advances exactly once per BeginFrame, so key everything off that instead.
static void SyncFrame(VulkanRenderer::Impl* m)
{
    const uint32_t slot = m->backend->FrameSlot() % kFrames;
    if (slot == m->frame)
        return; // same frame -- keep accumulating, never rewind
    m->frame = slot;
    ++m->frameCounter;
    m->dynVbUsed = 0;
    m->idxUsed = 0;
    m->uboSlot = 0;
    // #78: the clipmap staging ring is per FRAME, not per eye -- rewinding it in
    // ResetFrameRing would overwrite strips the previous eye still reads.
    m->clipStageUsed = 0;
    // #107 PERF: do NOT reset the object descriptor pool -- the sets are PRE-ALLOCATED and reused every frame (a draw
    // updates its slot's set). Resetting would destroy them. uboSlot=0 above already rewinds the per-frame slot index.
    // One-shot DrawBitmap2D textures: tick their countdown once per FRAME (it used to tick per BeginScreenPass, which
    // would have retired them early -- i.e. destroyed an image the GPU was still reading).
    for (size_t i = 0; i < m->retiredTex.size();)
    {
        if (--m->retiredTex[i].framesLeft <= 0)
        {
            if (m->texMgr)
                m->texMgr->Destroy(
                    m->retiredTex[i]
                        .tex); // texMgr defers the actual free by kRetireFrames
            m->retiredTex[i] = m->retiredTex.back();
            m->retiredTex.pop_back();
        }
        else
            ++i;
    }
    // Free textures the ENGINE dropped (terrain tiles, exploded objects) once no in-flight frame can reference them.
    if (m->texMgr)
        m->texMgr->TickRetire();
    // Same for per-model vertex buffers (an exploded/LOD-swapped model's VB) -- via the global manager.
    if (g_pVulkanVbManager)
        g_pVulkanVbManager->TickRetire();
}

// Artscout - 2026 (#104): open the multiview scene lazily (desktop = 1 view) so object/terrain/sky draws have a
// live render pass, and mark the frame as a GPU 3D frame so present blits the scene. Gated by g_bVulkanScene: while
// OFF (default) curCmd is null so all scene draws no-op -> entering 3D is stable (no GPU hang) while the scene frame
// is debugged headless.
static void VkBeginScenePass(VulkanRenderer::Impl* m)
{
    extern bool g_bVulkanScene;
    if (g_bVulkanScene)
    {
        // Artscout - 2026 (#104): the per-frame object resources are recycled HERE, on the frame's FIRST scene pass,
        // not in BeginObjectPass. Validation caught why: BeginObjectPass can run more than once per frame (the engine
        // calls it from FlushBuffers), and each call was doing vkResetDescriptorPool -- which destroys sets that the
        // scene command buffer had ALREADY bound from earlier draws. That drops the whole command buffer into an
        // invalid state ("VkDescriptorSet was destroyed or updated" -> "vkCmdBindPipeline was called in a
        // VkCommandBuffer which is now in an invalid state"), so every later draw in that frame is silently discarded.
        // Resetting uboSlot per pass had the same shape of bug: the second pass would overwrite the UBO slots the
        // first pass's queued draws still point at.
        SyncFrame(m);
        // #107 VR-Vulkan: the in-3D menu owns its own pass + command buffer (BindMenuRtt already opened it). The exit
        // dialog is a 3D BSP that arrives here via BeginObjectPass -- route it to menuCmd, NOT the eye's sceneCmd, and
        // do NOT EnsureSceneStarted (that would open the eye scene mid-menu). GetObjPipe picks MenuRenderPass to match.
        if (m->backend->IsMenuActive())
        {
            m->curCmd = (VkCommandBuffer)m->backend->MenuCommandBuffer();
            return;
        }
        // #107 Option 2: the per-eye tail pass owns its own cmd + pass (BeginTailView opened it). The cockpit BSP arrives
        // here via BeginObjectPass -> route to tailCmd, and do NOT EnsureSceneStarted (that opened a FLAT 1920x1080 1-view
        // scene mid-tail -> nViews 2<->1 pass thrash -> pipeline purge every frame -> crash). GetObjPipe picks TailRenderPass.
        // Sensor scene (TGP/MAV/FLIR): a display RTT pass is open -- object/terrain draws belong to IT,
        // not to the eye. FlatCommandBuffer returns rttCmd while IsRttActive; GetObjPipe picks the
        // matching RttRenderPass variant. This is what makes the sensor video land in the MFD.
        if (m->backend->IsRttActive())
        {
            m->curCmd = (VkCommandBuffer)m->backend->FlatCommandBuffer();
            return;
        }
        if (m->backend->IsTailActive())
        {
            m->curCmd = (VkCommandBuffer)m->backend->TailCommandBuffer();
            return;
        }
        m->backend
            ->EnsureSceneStarted(); // may rebuild the scene target -- and with it the render pass
        PurgePipelinesIfScenePassChanged(
            m); // ...so check AFTER it, before anything is bound
        {
            extern bool g_bGpuDraw;
            g_bGpuDraw = true;
        }
        m->curCmd = (VkCommandBuffer)m->backend->SceneCommandBuffer();
    }
    else
    {
        m->curCmd = VK_NULL_HANDLE; // scene path off -> draws no-op
    }
}
// Artscout - 2026 (#104): ported from D3D12Renderer::BeginObjectPass -- the pass states its base FF_* word plus
// blend/depth, which the per-surface SetState calls then refine. It used to open the scene pass and set drawLit only.
void VulkanRenderer::BeginObjectPass()
{
    VkBeginScenePass(m);
    m->inObjectPass = true;
    m->drawLit = true;
    m->blend = BLEND_OPAQUE;
    m->depthWrite = true;
    m->depthTest = true;
    m->biasMode = 1; // #16 pull objects toward the camera
    m->filter = FILTER_LINEAR;
    m->addr = ADDR_WRAP;
    // Clear the glyph-mask flag: the object/terrain path NEVER renders text, and it does not call SetState (only the
    // screen path does), so without this it inherits stateTexColorDiffuse from the last MFD/HUD text draw
    // (STATE_TEXTURE_TEXT). That leaked FF_TEXCOLORDIFFUSE onto terrain/objects -> the shader took the "colour from
    // the vertex, ignore the texture" branch and every textured surface came out flat grey the moment symbology had
    // drawn that frame (e.g. right after the MFD renders while the camera pans). D3D12 avoids it by resetting its
    // whole flag word in BeginObjectPass; here the bit lives in a separate bool, so reset it here too.
    m->stateTexColorDiffuse = false;
    m->skyNoFog =
        false; // #15: only the sky dome suppresses fog; objects/terrain (BeginTerrainPass calls this) fog normally
    m->stateFlags = FF_VERTEXCOLOR | FF_LIGHTING | FF_ALPHATEST;
    if (m->nvg)
        m->stateFlags |=
            FF_NVG; // #97 green the cockpit / aircraft / world objects
    EnsureObjectPipe(
        m); // lazy: needs the scene render pass (EnsureSceneTarget) to exist
}
// Ported from D3D12Renderer::BeginTerrainPass. The terrain posts are CAMERA-RELATIVE (the object view matrix is
// rotation-only -- see TerrainGpu.cpp), hence the identity world. No lighting: tiles arrive pre-lit in the vertex
// colour. Distance fog dissolves the far terrain into haze and masks the residual LOD-seam shimmer.
void VulkanRenderer::BeginTerrainPass()
{
    BeginObjectPass();
    m->biasMode = 2; // #78 sink terrain under coplanar runway/objects
    static const float I[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    memcpy(m->world, I, sizeof(I));
    m->boundTex[0] = nullptr; // textured tiles add FF_TEXTURE0 via SetTexture
    m->drawLit = false;
    m->stateFlags =
        FF_VERTEXCOLOR | FF_FOG; // BeginObjectPass reset the word above
    if (m->nvg)
        m->stateFlags |= FF_NVG;
}
// Stubs on D3D12 as well ("per-LOD depth-bias variants = later increment"; PSOs are built lazily, so there is nothing
// to rebuild). Parity, not debt -- the per-LOD bias belongs with #78.
void VulkanRenderer::SetTerrainRasterForLod(int)
{
}
void VulkanRenderer::RebuildTerrainRasters()
{
}
// Artscout - 2026 (#104): #96 skydome -- the object path's background pass, drawn FIRST: depth OFF (no test, no
// write) so it fills the frame and terrain/objects then paint over it (occlusion by draw order, like the legacy 2D
// sky). Pure vertex colour, no lighting, no fog; a stars/sun texture adds FF_TEXTURE0 via SetTexture. Ported from
// D3D12Renderer::BeginSkyPass -- this used to just open the scene pass, leaving the dome to write depth and be lit.
void VulkanRenderer::BeginSkyPass(bool blend)
{
    VkBeginScenePass(m);
    m->blend =
        blend ?
            BLEND_ALPHA :
            BLEND_OPAQUE; // sun/moon discs blend over the gradient; the dome is opaque
    m->depthWrite = false;
    m->depthTest = false;
    // #15: WRAP, not CLAMP -- the equirect STARMAP wraps 360deg in azimuth, so its u leaves [0,1] over half the dome
    // (the dome build even notes "the sampler's WRAP addressing handles u outside 0..1"). CLAMP pinned that half to the
    // texture edge, so stars only appeared on the u-in-[0,1] hemisphere. D3D12's BeginSkyPass leaves the object default
    // (WRAP) untouched, which is why the Windows sky is full; match it. Sun/moon disc UVs are in [0,1], so WRAP is a no-op for them.
    m->filter = FILTER_LINEAR;
    m->addr = ADDR_WRAP;
    m->stateFlags = FF_VERTEXCOLOR;
    m->drawLit = false;
    // Artscout - 2026 (#15): the D3D12 peer draws the dome with "no fog" (see the comment above), but on Vulkan the
    // per-draw flag word ORs in FF_FOG whenever the sticky fog RANGE (m->fogStart/End, set by the terrain pass) is
    // valid -- and the dome sits at the far radius (~200000), so the fog factor saturates and the WHOLE dome renders as
    // the (grey, daytime-ish) fog colour: a flat grey night sky with the real dark gradient fogged away. Suppress FF_FOG
    // for the sky via a dedicated flag -- do NOT touch the fog RANGE (zeroing it made the terrain pass, which keeps
    // FF_FOG, divide by (fogEnd-fogStart)==0 -> NaN -> the whole terrain vanished). Also reset the material tint.
    m->skyNoFog = true;
    m->matColor[0] = m->matColor[1] = m->matColor[2] = m->matColor[3] = 1.0f;
    static const float I[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    memcpy(
        m->world, I,
        sizeof(I)); // dome verts are camera-relative world directions * radius
    m->boundTex[0] = nullptr;
}

// ---------------------------------------------------------------------------------------------- 2D screen draws
// Upload verts into the per-frame dynamic ring and issue a non-indexed triangle draw of the screen pipeline.
// Artscout - 2026 (#104): bind the screen pipeline for `primType` plus the texture/push constants, and stream the
// vertices into the per-frame ring. Shared by DrawTL and DrawTLIndexed; returns the vertex-buffer offset (or ~0 on
// failure). ffscreen.frag always samples gTex0, so lines/points bind the 1x1 white texture -- the peer of D3D12
// clearing FF_TEXTURE0 for them, which stops a leftover font texture chroma-cutting the line.
// Re-assert the viewport the current path needs before recording into an RTT command buffer. The zone rect set by
// ConfineObjectViewportToZone is fire-and-forget dynamic state: any mid-batch BindSceneRtt opens a NEW ring buffer
// whose viewport is reset to the full atlas, so the sensor OBJECT flush drew at atlas scale (512/zoneW ~ 3.4x,
// centred) -- "objects move faster than the cursor" -- while the zone scissor still clipped them into the MFD. Each
// path now states its own requirement at record time: objects take the pending zone (GetRttZone), 2D takes the full
// extent (its vertices are already atlas-zoned; zoning the viewport too would double-transform them). Cached on the
// backend's viewport serial so it costs one compare per draw.
static void AssertRttViewport(VulkanRenderer::Impl* m, bool objectPath)
{
    int zx = 0, zy = 0, zw = 0, zh = 0;
    const bool zone = m->backend->GetRttZone(&zx, &zy, &zw, &zh);
    int fw = 0, fh = 0;
    m->backend->GetRttExtent(&fw, &fh);
    if (fw <= 0 || fh <= 0 || !m->curCmd)
        return;
    const int kind = objectPath ? 2 : 1;
    const unsigned serial = m->backend->RttViewportSerial();
    if (m->rttVpSerial == serial && m->rttVpKind == kind)
        return;
    m->rttVpSerial = serial;
    m->rttVpKind = kind;
    VkViewport vp;
    VkRect2D sc;
    if (objectPath)
    {
        // Object draws carry D3D-convention matrices (clip +Y up) and Vulkan NDC +Y is DOWN -- every other object
        // pass (scene, tail) compensates with a NEGATIVE-height viewport, and the RTT object path must too, or the
        // object layer renders vertically MIRRORED against the CPU terrain spans (opposite vertical motion on slew;
        // D3D12 needs no flip, which is why it was straight there). Zone when pending, else full extent.
        const int x = zone ? zx : 0, y = zone ? zy : 0;
        const int w = zone ? zw : fw, h = zone ? zh : fh;
        vp = VkViewport{(float)x,  (float)(y + h), (float)w,
                        -(float)h, 0.0f,           1.0f};
        sc = VkRect2D{{x, y}, {(uint32_t)w, (uint32_t)h}};
    }
    else
    {
        // 2D keeps the FULL-extent viewport (its vertices are already atlas-zoned; a zone viewport would
        // double-transform them) but takes the ZONE as SCISSOR while one is pending: the sensor's CPU terrain
        // spans regenerate every slew and stray fragments past the zone edge -- unclipped, they splat IR fill
        // across the neighboring HUD/DED zones ("HUD/DED перекрываются движением курсора TGP"). D3D12 is clean
        // because its pre-DrawScene Confine leaves a zone scissor armed for the span draws; this is that
        // clipping, done per draw. No zone pending (symbology, GM) -> full-extent scissor as before.
        vp = VkViewport{0.0f, 0.0f, (float)fw, (float)fh, 0.0f, 1.0f};
        sc = zone ? VkRect2D{{zx, zy}, {(uint32_t)zw, (uint32_t)zh}} :
                    VkRect2D{{0, 0}, {(uint32_t)fw, (uint32_t)fh}};
    }
    vkCmdSetViewport(m->curCmd, 0, 1, &vp);
    vkCmdSetScissor(m->curCmd, 0, 1, &sc);
}

static VkDeviceSize RecordScreenDraw(VulkanRenderer::Impl* m, int primType,
                                     const ScreenVertex* verts, int count)
{
    const uint32_t stride = 40;
    uint32_t bytes = (uint32_t)count * stride;
    if (m->dynVbUsed + bytes > (uint32_t)kDynVbBytes)
    {
        // Dropping a draw here is silent data loss (big fan expansions are the usual victims) -- say so, once per
        // frame, so an "invisible geometry" report can be tied to ring exhaustion instead of a state bug.
        static uint32_t s_warnFrame = 0xFFFFFFFFu;
        if (s_warnFrame != m->frameCounter)
        {
            s_warnFrame = m->frameCounter;
            char buf[128];
            snprintf(buf, sizeof(buf),
                     "[Vulkan] dyn VB ring full: drop %u verts (%u/%u bytes)\n",
                     (unsigned)count, (unsigned)(m->dynVbUsed + bytes),
                     (unsigned)kDynVbBytes);
            fputs(buf, stderr);
#ifdef _WIN32
            OutputDebugStringA(buf);
#endif
        }
        return (VkDeviceSize)~0ull;
    }
    VkPipeline pipe =
        GetScreenPipe(m, TopoOfPrimType(primType),
                      m->blend); // SetState set m->blend from the state
    if (!pipe)
        return (VkDeviceSize)~0ull;
    memcpy((uint8_t*)m->dynVbMap[m->frame] + m->dynVbUsed, verts, bytes);
    VkDeviceSize offset = m->dynVbUsed;
    m->dynVbUsed += bytes;
    // A draw is now certain, and if it lands in the flat swapchain pass this frame is a GPU frame: present must
    // PRESENT it rather than blit the 565 UI over it (which would erase exactly what we drew). Mirrors D3D12, which
    // sets g_bGpuDraw from each draw entry. The scene path sets it in VkBeginScenePass; an RTT draw is part of a
    // 3D frame that already did.
    if (m->screenRp &&
        m->screenRp == (VkRenderPass)m->backend->SwapchainRenderPass())
    {
        extern bool g_bGpuDraw;
        g_bGpuDraw = true;
    }
    if (m->screenRp == (VkRenderPass)m->backend->RttRenderPass())
        AssertRttViewport(m, false); // 2D into the RTT: full extent

    vkCmdBindPipeline(m->curCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
    m->boundObjPipe = VK_NULL_HANDLE;
    m->boundBiasCmd =
        VK_NULL_HANDLE; // #107 PERF: screen pipeline bound -> invalidate object bind cache
    m->boundBindlessCmd =
        VK_NULL_HANDLE; // #107: the screen pipeline's layout disturbs set 1 -> force a re-bind next object draw
    // Honor FF_TEXTURE0 like D3D12's FFEmu PS does: untextured states (e.g. STATE_GOURAUD -- the GM radar terrain
    // fans) must NOT be multiplied by whatever texture happens to be left bound; ffscreen.frag always samples gTex0,
    // so the peer of "flag off" here is binding the 1x1 white texture. Without this the GM ground fans (uv 0,0)
    // were chroma-killed by texel (0,0) of a stale font/atlas page while the point blips (forced white) survived.
    VulkanTexture* tex = (PrimTypeIsLineOrPoint(primType) ||
                          !(m->stateFlags & FF_TEXTURE0)) ?
                             m->whiteTex :
                             (m->boundTex[0] ? m->boundTex[0] : m->whiteTex);
    VkDescriptorSet set = m->DescriptorFor(tex);
    // #104 robustness: a screen draw MUST have a descriptor bound -- issuing vkCmdDraw with the fragment sampler
    // unbound makes the driver dereference a null internal pointer (0xC0000005 write inside the GPU driver, seen from
    // the VR controller-model draw whose texture failed to resolve). If the bound texture yields no set (its view is
    // null/dangling, or the pool is dry), fall back to white; if even that fails, skip the draw rather than crash.
    if (!set && tex != m->whiteTex)
    {
        static uint32_t s_warn = 0xFFFFFFFFu;
        if (s_warn != m->frameCounter)
        {
            s_warn = m->frameCounter;
            char buf[128];
            snprintf(buf, sizeof(buf),
                     "[Vulkan] screen draw: texture %p gave no descriptor -> "
                     "white fallback\n",
                     (void*)tex);
            fputs(buf, stderr);
            fflush(stderr);
#ifdef _WIN32
            OutputDebugStringA(buf); // stderr is invisible on Windows
#endif
        }
        set = m->DescriptorFor(m->whiteTex);
    }
    if (!set)
        return (VkDeviceSize)~0ull;
    vkCmdBindDescriptorSets(m->curCmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m->screenLayout, 0, 1, &set, 0, nullptr);
    // push.z = flipY: 1 inside the SCENE pass, whose viewport has NEGATIVE height (it must, so the object path's
    // D3D-convention matrices come out right-side up). ffscreen.vert maps screen Y straight to Vulkan NDC Y, which is
    // correct under the flat/RTT passes' normal viewport and mirrored under the scene's -- so it cancels the flip
    // itself when told. Flipping the scene's viewport instead would mirror the 3D world; this only moves the 2D.
    // #107: the menu pass also uses a NEGATIVE-height viewport (its object dialog needs the D3D-convention flip), so it
    // flips its 2D exactly like the scene does.
    const float flipY =
        (m->screenRp &&
         (m->screenRp == (VkRenderPass)m->backend->GetSceneRenderPass() ||
          m->screenRp == (VkRenderPass)m->backend->MenuRenderPass())) ?
            1.0f :
            0.0f;
    // push.w = FF_RTTSOFT: the emissive RTT-atlas composite (DrawRttQuad -> DrawSquare -> here). The frag then adds
    // the atlas colour with a CONSTANT alpha (the quad's rttAlpha) instead of tex*vColor (whose alpha would be the
    // atlas's own) -- so zero-alpha symbology (radar lines/contour/waterline) survives. Peer of PS_Main's FF_RTTSOFT.
    const float rttSoft = (m->stateFlags & FF_RTTSOFT) ? 1.0f : 0.0f;
    float push[4] = {
        (float)(m->viewportW ? m->viewportW : m->backend->Width()),
        (float)(m->viewportH ? m->viewportH : m->backend->Height()), flipY,
        rttSoft};
    vkCmdPushConstants(m->curCmd, m->screenLayout,
                       VK_SHADER_STAGE_VERTEX_BIT |
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(push), push);
    vkCmdBindVertexBuffers(m->curCmd, 0, 1, &m->dynVb[m->frame], &offset);
    return offset;
}

// Artscout - 2026 (#104): TRIANGLEFAN (primType 6) has to be EMULATED, exactly as D3D12Renderer::DrawTL does -- and
// this is why the FCR MFD had neither blips nor symbology while the HUD/DED/RWR were fine. TopoOfPrimType maps 6 to
// TRIANGLE_LIST (there is no fan topology to rely on), so a fan of N vertices was drawn as N/3 disjoint triangles
// between unrelated corners: arcs and filled shapes came out as stray slivers or nothing at all. The engine emits
// fans for exactly the round/wedge symbology the radar page is made of, which is why only that page lost content.
// Same root as the black wedges on the exterior aircraft -- the object path threw dwPrimType away too.
void VulkanRenderer::DrawTL(int primType, const ScreenVertex* verts, int count)
{
    if (!m->curCmd || !verts || count <= 0)
        return;

    if (primType == 6)
    { // fan -> triangle list, EXPANDED to non-indexed (0, i+1, i+2) per triangle
        const int tris = count - 2;
        if (tris <= 0)
            return;
        const int nVert = tris * 3;
        // Non-indexed, on purpose -- see the note in DrawTLIndexed. Gather the fan into a flat vertex list.
        std::vector<ScreenVertex> flat((size_t)nVert);
        for (int i = 0; i < tris; ++i)
        {
            flat[(size_t)i * 3 + 0] = verts[0];
            flat[(size_t)i * 3 + 1] = verts[i + 1];
            flat[(size_t)i * 3 + 2] = verts[i + 2];
        }
        if (RecordScreenDraw(m, 4, flat.data(), nVert) == (VkDeviceSize)~0ull)
            return;
        vkCmdDraw(m->curCmd, (uint32_t)nVert, 1, 0, 0);
        return;
    }

    if (RecordScreenDraw(m, primType, verts, count) == (VkDeviceSize)~0ull)
        return;
    vkCmdDraw(m->curCmd, (uint32_t)count, 1, 0, 0);
}

// Artscout - 2026 (#104): a REAL indexed draw. This used to drop the indices on the floor and draw the vertex list --
// silently wrong rather than absent: any caller that reuses vertices via indices got mangled geometry.
// Artscout - 2026 (#104): draw NON-INDEXED by gathering the referenced vertices into a flat list. The indexed
// screen path (vkCmdDrawIndexed into the RTT command buffer) writes the atlas correctly but its result never
// reaches the composite -- a RenderDoc session pinned it: on the MFD atlas, every vkCmdDraw survives to the final
// image, every vkCmdDrawIndexed does not (the radar contour, scan bars, waterline -- all the LINE_LIST/TRI_LIST
// batches that arrive with indices). Non-indexed is proven to work into the RTT, so expand to it. D3D12's
// DrawDynamic2DIndexed does the same gather. The index list already encodes list order, so a straight gather + a
// non-indexed draw of `icount` vertices reproduces the exact primitives.
void VulkanRenderer::DrawTLIndexed(int primType, const ScreenVertex* verts,
                                   int vcount, const unsigned short* indices,
                                   int icount)
{
    if (!m->curCmd || !verts || vcount <= 0 || !indices || icount <= 0)
        return;
    std::vector<ScreenVertex> flat((size_t)icount);
    for (int i = 0; i < icount; ++i)
    {
        int idx = indices[i];
        flat[(size_t)i] = (idx >= 0 && idx < vcount) ?
                              verts[idx] :
                              verts[0]; // guard a stray index
    }
    if (RecordScreenDraw(m, primType, flat.data(), icount) ==
        (VkDeviceSize)~0ull)
        return;
    vkCmdDraw(m->curCmd, (uint32_t)icount, 1, 0, 0);
}

void VulkanRenderer::DrawColorTrisScreen(const ScreenVertex* verts, int count,
                                         ID3D11ShaderResourceView* tex,
                                         int opaque, int /*cull*/)
{
    // Honor `opaque` exactly as D3D12Renderer does. Without this the draw inherited whatever m->blend the
    // previous screen draw left set (the cockpit-emissive / HUD / effects path leaves BLEND_ADDITIVE), so
    // the VR hands -- though their vertex + texture alpha are both fully opaque -- were ADDED onto the scene
    // and read as translucent "ghost" gloves in BOTH modes. BLEND_OPAQUE disables blending -> solid hands.
    // (cull is unused: the screen pipeline does not consume it; the hand paths painter-sort + draw two-sided.)
    m->blend = opaque ? BLEND_OPAQUE : BLEND_ALPHA;
    SetTexture(0, tex);
    DrawTL(0, verts, count);
}

// ---- the remaining draw/UI/object surface: staged. State is tracked above; each records its geometry as the
//      screen path is generalized (index ring, descriptor sets per texture, object/terrain pipelines built against
//      backend->GetSceneRenderPass() for multiview). Bodies present so the class is concrete + linkable now. ----
// Bind the object pipeline + a UBO/texture descriptor built from the current state, then draw from the given
// vertex buffer: indexed (index buffer at iboOff, `count` indices) or a non-indexed run (`count` verts from
// firstVertex). `pipe` selects list vs strip topology.
namespace
{
// Scene animation time in seconds, wrapping every 1e6 s so the float keeps its precision. Feeds the afterburner
// turbulence, the water sheen and the NVG grain -- ffemu.hlsl's gWaterParams.x, which D3D12Renderer fills from
// GetTickCount(); steady_clock is the portable equivalent (this TU also builds on Linux).
float SceneTimeSec()
{
    static const auto t0 = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::steady_clock::now() - t0)
                  .count();
    return (float)(ms % 1000000) * 0.001f;
}
}

// #107 PERF: FF_BINDLESS now lives in ffstatemap.h -- D3D12 sets the same bit
// for its own bindless terrain. Still must match ffobject.frag's value.

static void RecordObjectDraw(VulkanRenderer::Impl* m, VkPipeline pipe,
                             VkBuffer vb, VkDeviceSize vboOff, bool indexed,
                             VkDeviceSize iboOff, int count, int firstVertex)
{
    if (!m->curCmd || !pipe || !vb)
        return;
    // Sensor objects into the RTT: re-assert the display's zone viewport (a mid-batch re-bind reset it to the full
    // atlas -> objects at ~3.4x cursor scale). See AssertRttViewport.
    if (!m->backend->IsMenuActive() && m->backend->IsRttActive() &&
        m->curCmd == (VkCommandBuffer)m->backend->FlatCommandBuffer())
        AssertRttViewport(m, true);
    // Artscout - 2026 (#104): the per-frame UBO ring and the descriptor pool are both kMaxDrawsPerFrame entries, and
    // this had NO bound on either -- it wrote past the mapped ring and then let vkAllocateDescriptorSets fail, which
    // returns an error code rather than tripping validation. So an overflowing frame lost its later draws SILENTLY,
    // with a clean log: the terrain (recorded first) survived, the cockpit and the models (recorded after) vanished.
    // The budget used to be per-PASS, because BeginObjectPass rewound uboSlot; moving the rewind to the real frame
    // boundary (correct -- see SyncFrame) made the same number a whole-frame budget without resizing it.
    // Refuse the draw loudly instead of corrupting memory, and say so once per frame so this can never be silent.
    if (m->uboSlot >= kMaxDrawsPerFrame)
    {
        static uint32_t lastWarnFrame = 0xFFFFFFFFu;
        if (lastWarnFrame != m->frameCounter)
        {
            lastWarnFrame = m->frameCounter;
            char buf[160];
            snprintf(buf, sizeof(buf),
                     "[Vulkan] object draw budget exhausted: %u draws this "
                     "frame (max %u) -- later draws are "
                     "being dropped. Raise kMaxDrawsPerFrame.\n",
                     m->uboSlot, kMaxDrawsPerFrame);
            fputs(buf, stderr);
#ifdef _WIN32
            OutputDebugStringA(buf); // stderr is invisible on Windows
#endif
        }
        return;
    }
    uint8_t* uboBase = (uint8_t*)m->uboRingMap[m->frame] +
                       (VkDeviceSize)m->uboSlot * m->uboStride;
    VulkanRenderer::Impl::ObjUbo* ubo = (VulkanRenderer::Impl::ObjUbo*)uboBase;
    memcpy(ubo->world, m->world, sizeof(ubo->world));
    // #107 VR-Vulkan: view-instancing per-view UBO (peer of D3D12 cbViewStereo). Base rotation view + the per-view
    // world eye offset rotated into view space and shifted into the translation row; per-view proj or the base. Off ->
    // replicate the single view/proj to all layers (flat/mono).
    for (int v = 0; v < 4; ++v)
    {
        if (m->viActive)
        {
            const int s = (v < m->viCount) ? v : 0;
            memcpy(ubo->view[v], m->view, 64);
            const float* w = m->viWorldOff[s];
            float ovx =
                w[0] * m->view[0] + w[1] * m->view[4] + w[2] * m->view[8];
            float ovy =
                w[0] * m->view[1] + w[1] * m->view[5] + w[2] * m->view[9];
            float ovz =
                w[0] * m->view[2] + w[1] * m->view[6] + w[2] * m->view[10];
            ubo->view[v][12] -= ovx;
            ubo->view[v][13] -= ovy;
            ubo->view[v][14] -= ovz;
            memcpy(ubo->proj[v], m->viHasProj ? m->viProj[s] : m->proj, 64);
        }
        else
        {
            memcpy(ubo->view[v], m->view, 64);
            memcpy(ubo->proj[v], m->proj, 64);
        }
    }
    // Artscout - 2026 (sensor video): NO projection flip for RTT-bound object draws. It was added
    // while the sensor objects were still mis-routed into the tail (where they showed upside-down);
    // with the routing fixed they land in the real RTT pass and the flip DOUBLE-inverted them
    // (upside-down picture, vertically inverted motion). The positive-height RTT viewport plus the
    // sensor projection compose correctly on their own.
    memcpy(ubo->materialColor, m->matColor, sizeof(ubo->materialColor));
    memcpy(ubo->ambient, m->ambient, sizeof(ubo->ambient));
    // The scene's REAL lights (sun + dynamic lamps) as handed over by SetLights. This used to be a hardcoded fake
    // sun direction, which is why the Vulkan cockpit never matched the D3D12 one at any time of day.
    ubo->numLights[0] = (uint32_t)m->numLights;
    ubo->numLights[1] = ubo->numLights[2] = ubo->numLights[3] = 0;
    memcpy(ubo->lights, m->lights, sizeof(ubo->lights));
    ubo->camPos[0] = m->camPos[0];
    ubo->camPos[1] = m->camPos[1];
    ubo->camPos[2] = m->camPos[2];
    ubo->camPos[3] = 0.0f;
    ubo->params[0] = m->drawLit ? 1.0f : 0.0f;
    ubo->params[1] = (float)m->viewportW;
    ubo->params[2] = (float)m->viewportH; // NVG tube vignette needs it
    ubo->params[3] = 0.0f;

    // Artscout - 2026 (#104): the draw's FF_* word = what the legacy STATE_* bundle implies (via FFMapState, shared
    // with D3D12) OR the sticky/dynamic bits the renderer methods set. It used to be invented from a handful of bools
    // with the state ignored entirely, which is how FF_RTTSOFT / FF_MODULATE2X / FF_TEXTURE1 / FF_WATER -- bits ONLY
    // the state can imply -- never reached the shader at all.
    uint32_t gf = m->stateFlags;
    if (!m->boundTex[0])
        gf &=
            ~FF_TEXTURE0; // no real texture -> draw by vertex colour, don't sample white
    if (!m->boundTex[1])
        gf &= ~FF_TEXTURE1;
    if (!m->drawLit)
        gf &=
            ~FF_LIGHTING; // 2D/effect paths force-unlit regardless of the state
    if (m->stateTexColorDiffuse)
        gf |= FF_TEXCOLORDIFFUSE;
    if (m->alphaTest)
        gf |= FF_ALPHATEST;
    if (m->chromaTol > 0.0f)
        gf |= FF_CHROMAKEY;
    if (m->emissive)
        gf |= FF_EMISSIVE;
    if (m->afterburner)
        gf |= FF_AFTERBURNER;
    if (m->cockpitPass)
        gf |= FF_COCKPIT;
    if (m->irGrey)
        gf |= FF_IRGREY;
    if (m->nvg)
        gf |= FF_NVG;
    if (m->fullBright)
        gf |= FF_FULLBRIGHT;
    if (m->fogEnd > m->fogStart && !m->cockpitPass && !m->skyNoFog)
        gf |=
            FF_FOG; // fog world/terrain, never the cockpit interior or the sky dome
    if (m->glocActive)
        gf |= FF_GLOC;
    if (m->bindlessDraw)
        gf |=
            FF_BINDLESS; // #107: sample the tile array by per-vertex slot (terrain only)
    // Artscout - 2026 (#15): decisive log of what the SKY draws actually send to the shader. The first two skyNoFog
    // draws are the base dome (opaque, no tex) then the additive starmap (tex bound). Tells us: is FF_FOG still on? is
    // the material tint neutral? is the starmap texture actually bound for the additive draw? Stop guessing.
    if (m->skyNoFog)
    {
        static int s_skyDrawN = 0;
        if (s_skyDrawN < 2)
        {
            fprintf(stderr,
                    "[SKYDRAW] #%d gf=0x%X FF_FOG=%d FF_TEXTURE0=%d "
                    "fog=[%.0f..%.0f] mat=(%.2f,%.2f,%.2f,%.2f) blend=%d "
                    "tex0=%p lit=%d\n",
                    s_skyDrawN, gf, (gf & FF_FOG) ? 1 : 0,
                    (gf & FF_TEXTURE0) ? 1 : 0, (double)m->fogStart,
                    (double)m->fogEnd, (double)m->matColor[0],
                    (double)m->matColor[1], (double)m->matColor[2],
                    (double)m->matColor[3], (int)m->blend,
                    (void*)m->boundTex[0], (int)(m->drawLit ? 1 : 0));
            fflush(stderr);
            s_skyDrawN++;
        }
    }
    ubo->flags[0] = gf;
    ubo->flags[1] = ubo->flags[2] = ubo->flags[3] = 0;
    memcpy(ubo->gloc, m->glocParams, sizeof(ubo->gloc));
    auto argb2rgb = [](unsigned long a, float* o)
    {
        o[0] = ((a >> 16) & 0xFF) / 255.0f;
        o[1] = ((a >> 8) & 0xFF) / 255.0f;
        o[2] = (a & 0xFF) / 255.0f;
        o[3] = 1.0f;
    };
    argb2rgb(m->fogArgb, ubo->fogColor);
    argb2rgb(m->chromaArgb, ubo->chromaKey);
    ubo->chromaKey[3] = m->chromaTol;
    // fogParams.w = scene animation time (sec) -- drives the afterburner turbulence/flicker, the water sheen and the
    // NVG photon grain, all of which read it as ffemu.hlsl's gWaterParams.x.
    ubo->fogParams[0] = m->fogStart;
    ubo->fogParams[1] = m->fogEnd;
    ubo->fogParams[2] = m->alphaRef;
    ubo->fogParams[3] = SceneTimeSec();
    memcpy(ubo->spec, m->matSpec, sizeof(ubo->spec));

    // #107 PERF: take this slot's PRE-ALLOCATED set (uboSlot < kMaxDrawsPerFrame, budget-checked above) -- no
    // vkAllocateDescriptorSets in the hot path; just update + bind. This is the whole fix for the 26x terrain gap.
    VkDescriptorSet set = m->objSets[m->frame][m->uboSlot];
    // #107 PERF: binding 0 (the UBO) is written ONCE at init (set[slot] -> uboRing offset slot*stride is fixed). Only
    // the 2 texture bindings change per draw -> update just those (2 writes, not 3).
    VkSampler samp = m->samplers[m->filter][m->addr];
    VulkanTexture* tex0 = m->boundTex[0] ? m->boundTex[0] : m->whiteTex;
    VulkanTexture* tex1 = m->boundTex[1] ? m->boundTex[1] : m->whiteTex;
    VkDescriptorImageInfo ii0{};
    ii0.sampler = samp;
    ii0.imageView = (VkImageView)tex0->view;
    ii0.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkDescriptorImageInfo ii1{};
    ii1.sampler = samp;
    ii1.imageView = (VkImageView)tex1->view;
    ii1.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // #78: binding 3 is the standalone sampler for the bindless tile array.
    VkDescriptorImageInfo iiSamp{};
    iiSamp.sampler = m->samplers[FILTER_LINEAR][ADDR_WRAP];
    VkWriteDescriptorSet w[3] = {};
    w[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = set;
    w[0].dstBinding = 1;
    w[0].descriptorCount = 1;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[0].pImageInfo = &ii0;
    w[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[1].dstSet = set;
    w[1].dstBinding = 2;
    w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[1].pImageInfo = &ii1;
    w[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[2].dstSet = set;
    w[2].dstBinding = 3;
    w[2].descriptorCount = 1;
    w[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    w[2].pImageInfo = &iiSamp;
    vkUpdateDescriptorSets(m->device, 3, w, 0, nullptr);

    // #107 PERF: skip the pipeline re-bind when it's already current in THIS command buffer (terrain = ~13k draws all on
    // one pipeline -> ~13k redundant vkCmdBindPipeline). Keyed by (cmd, pipe) since each command buffer has its own state.
    if (pipe != m->boundObjPipe || m->curCmd != m->boundObjPipeCmd)
    {
        vkCmdBindPipeline(m->curCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
        m->boundObjPipe = pipe;
        m->boundObjPipeCmd = m->curCmd;
        m->boundBiasCmd =
            VK_NULL_HANDLE; // a fresh pipeline resets dynamic state -> force the depth-bias re-set below
    }
    // Depth bias for the current pass (dynamic state; D3D12 bakes the same buckets into the PSO). Reversed-Z, so a
    // POSITIVE constant pulls toward the camera and a negative one sinks the surface away from it.
    float biasConst = 0.0f, biasSlope = 0.0f;
    if (m->biasMode == 1)
    {
        biasConst =
            100.0f; // #16: pull objects toward the camera off coplanar terrain
    }
    else if (m->biasMode == 2)
    {
        extern float g_fGpuTerrainSlopeBias,
            g_fGpuTerrainDepthBias; // #78; both 0 by default -> no push
        biasConst = -g_fGpuTerrainDepthBias;
        biasSlope = -g_fGpuTerrainSlopeBias;
    }
    // #107 PERF: skip the depth-bias re-set when unchanged in THIS command buffer (terrain draws all share one bias).
    if (biasConst != m->boundBiasConst || biasSlope != m->boundBiasSlope ||
        m->curCmd != m->boundBiasCmd)
    {
        vkCmdSetDepthBias(m->curCmd, biasConst, 0.0f, biasSlope);
        m->boundBiasConst = biasConst;
        m->boundBiasSlope = biasSlope;
        m->boundBiasCmd = m->curCmd;
    }
    vkCmdBindDescriptorSets(m->curCmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m->objLayout, 0, 1, &set, 0, nullptr);
    // #107 PERF bindless terrain: the object frag STATICALLY references gBindless (set 1) -- so when the layout carries
    // set 1, EVERY draw must have it bound, not just the terrain one (else validation flags an unbound statically-used
    // set). The array is a single constant set and binding set 0 doesn't disturb it (same objLayout), so bind it ONCE
    // per command buffer. It's UPDATE_AFTER_BIND, so slots assigned later this frame (terrain accumulation) stay valid.
    // #78: the SAMPLED_IMAGE twin, which is what the HLSL shader declares.
    if (m->bindlessReady && m->meshTileSet && m->curCmd != m->boundBindlessCmd)
    {
        vkCmdBindDescriptorSets(m->curCmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m->objLayout, 1, 1, &m->meshTileSet, 0,
                                nullptr);
        m->boundBindlessCmd = m->curCmd;
    }
    vkCmdBindVertexBuffers(m->curCmd, 0, 1, &vb, &vboOff);
    FrameProf_CountDraw(); // #107 PERF: count object draws/frame
    if (indexed)
    {
        vkCmdBindIndexBuffer(m->curCmd, m->idxBuf[m->frame], iboOff,
                             VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(m->curCmd, (uint32_t)count, 1, 0, firstVertex, 0);
    }
    else
    {
        vkCmdDraw(m->curCmd, (uint32_t)count, 1, firstVertex, 0);
    }
    m->uboSlot++;
}

// Artscout - 2026 (#104): CPU bitmap (splash / cursor / mirror) -> a temporary texture + a screen-space quad. Ported
// from D3D12Renderer::DrawBitmap2D; this was an empty stub, so Render2DBitmap drew nothing under Vulkan.
void VulkanRenderer::DrawBitmap2D(int dX, int dY, int w, int h, int totalWidth,
                                  int sX, int sY, const unsigned* pSrc,
                                  bool fit, int screenW, int screenH)
{
    if (!pSrc || w <= 0 || h <= 0 || !m->texMgr)
        return;

    // CreateRGBA wants tightly-packed rows, and the caller hands us a SUB-REGION of a wider bitmap -- repack it.
    std::vector<unsigned> packed((size_t)w * h);
    for (int y = 0; y < h; ++y)
        memcpy(&packed[(size_t)y * w],
               pSrc + (size_t)(sY + y) * totalWidth + sX, (size_t)w * 4);
    VulkanTexture* tmp = m->texMgr->CreateRGBA(packed.data(), w, h);
    if (!tmp)
        return;

    // Artscout - 2026 (#104): the load splash is a flat frame -- no scene, no RTT this frame -- and BeginScreenPass
    // deliberately does NOT open a swapchain frame (that would orphan the acquire inside a 3D frame that presents via
    // PresentScene). But this CPU-bitmap path is different: it is ONLY the splash / cursor / mirror, and when neither
    // the scene nor an RTT is recording, we ARE the splash, so open the flat frame here. It is safe precisely because
    // a splash frame never opens a scene afterwards, so nothing else acquires the image -- SwapBuffers presents it.
    if (!m->backend->IsSceneRecording() && !m->backend->IsRttActive())
        m->backend->EnsureFrameStarted();
    BeginScreenPass();
    if (!m->curCmd)
    {
        m->texMgr->Destroy(tmp);
        return;
    }
    SetTexture(0, (ID3D11ShaderResourceView*)tmp);

    const float x0 = (float)dX, y0 = (float)dY;
    const float dw = fit ? (float)screenW : (float)w;
    const float dh = fit ? (float)screenH : (float)h;
    ScreenVertex v[4];
    memset(v, 0, sizeof(v));
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
        v[i].rhw = 1.0f; // the screen pass has no depth, so sz is irrelevant
        v[i].color = 0xFFFFFFFF;
        v[i].specular = 0;
        v[i].tu1 = v[i].tu0;
        v[i].tv1 = v[i].tv0;
    }
    DrawTL(5, v, 4); // 5 = triangle strip

    // Do NOT leave the doomed texture bound: the next draw would sample a destroyed image. Same trap D3D12 hit.
    SetTexture(0, nullptr);
    SetTexture(1, nullptr);
    // kFrames + 1: the backend fences each frame, so after that many BeginScreenPass calls no in-flight frame can
    // still be reading this image.
    m->retiredTex.push_back({tmp, kFrames + 1});
}
// Artscout - 2026 (#104): G-force / end-of-flight vignette (blackout=black tint, redout=red tint). Fullscreen alpha
// quad into the scene; the frag short-circuits on FF_GLOC to a radial alpha ramp (edge -> centre). Peer of
// D3D12Renderer::DrawGlocOverlay. Modelled on CompositeUISurface (identity matrices, quad UV 0..1, alpha blend).
void VulkanRenderer::DrawGlocOverlay(float intensity, float innerR,
                                     float outerR, float tintR, float tintG,
                                     float tintB)
{
    if (intensity <= 0.0f || !m->backend->IsSceneRecording())
        return;
    m->curCmd = (VkCommandBuffer)m->backend->SceneCommandBuffer();
    if (!m->curCmd)
        return;
    const float I[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    memcpy(m->world, I, 64);
    memcpy(m->view, I, 64);
    memcpy(m->proj, I, 64);
    m->drawLit = false;
    m->matColor[0] = tintR;
    m->matColor[1] = tintG;
    m->matColor[2] = tintB;
    m->matColor[3] = 1.0f;
    m->glocActive = true;
    m->glocParams[0] = intensity;
    m->glocParams[1] = innerR;
    m->glocParams[2] = outerR;
    m->stateFlags =
        0; // explicit intent: FF_GLOC (from glocActive) is the whole shader path here
    m->boundTex[0] =
        nullptr; // FF_GLOC short-circuits before any texture sample
    struct ObjV
    {
        float p[3];
        float nrm[3];
        unsigned col, spec;
        float tu, tv;
    };
    auto V = [](float x, float y, float u, float v)
    {
        ObjV o;
        o.p[0] = x;
        o.p[1] = y;
        o.p[2] = 1.0f;
        o.nrm[0] = o.nrm[1] = 0;
        o.nrm[2] = -1;
        o.col = 0xFFFFFFFFu;
        o.spec = 0;
        o.tu = u;
        o.tv = v;
        return o;
    };
    ObjV q[6] = {V(-1, -1, 0, 0), V(1, -1, 1, 0), V(1, 1, 1, 1),
                 V(-1, -1, 0, 0), V(1, 1, 1, 1),  V(-1, 1, 0, 1)};
    uint32_t vbytes = sizeof(q);
    if (m->dynVbUsed + vbytes <= (uint32_t)kDynVbBytes)
    {
        VkDeviceSize vboOff = m->dynVbUsed;
        memcpy((uint8_t*)m->dynVbMap[m->frame] + m->dynVbUsed, q, vbytes);
        m->dynVbUsed += vbytes;
        // Explicit intent, not the legacy state: a fullscreen overlay blended over the finished frame, no depth.
        RecordObjectDraw(m,
                         GetObjPipe(m, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                                    BLEND_ALPHA, false, false),
                         m->dynVb[m->frame], vboOff, false, 0, 6, 0);
    }
    m->glocActive = false; // one-shot; don't leak FF_GLOC onto later draws
    m->matColor[0] = m->matColor[1] = m->matColor[2] = m->matColor[3] = 1.0f;
}
void VulkanRenderer::DrawTerrainMesh(const void* verts, int vcount,
                                     const unsigned short* indices, int icount)
{
    if (!m->curCmd || !verts || !indices || vcount <= 0 || icount <= 0)
        return;
    const uint32_t stride = 40; // ObjV
    uint32_t vbytes = (uint32_t)vcount * stride, ibytes = (uint32_t)icount * 2;
    if (m->dynVbUsed + vbytes > (uint32_t)kDynVbBytes ||
        m->idxUsed + ibytes > (uint32_t)kDynVbBytes)
        return;
    VkDeviceSize vboOff = m->dynVbUsed, iboOff = m->idxUsed;
    memcpy((uint8_t*)m->dynVbMap[m->frame] + m->dynVbUsed, verts, vbytes);
    m->dynVbUsed += vbytes;
    memcpy((uint8_t*)m->idxMap[m->frame] + m->idxUsed, indices, ibytes);
    m->idxUsed += ibytes;
    // #78: the terrain mesh is always an indexed triangle list -- state the topology, don't inherit a default.
    RecordObjectDraw(m, StateObjPipe(m, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
                     m->dynVb[m->frame], vboOff, true, iboOff, icount, 0);
}
// Artscout - 2026 (#107 PERF): bindless terrain draw. Same as DrawTerrainMesh, but the vertex is 44 bytes (ObjV + a
// uint tile slot at offset 40) and the draw sets FF_BINDLESS + binds the tile array (set 1) so ONE (or a few) draws
// replace the ~13k per-tile draws. TerrainGpu merges every tile bucket into these, tagging each vertex with its slot.
void VulkanRenderer::DrawTerrainMeshBindless(const void* verts, int vcount,
                                             const unsigned short* indices,
                                             int icount)
{
    if (!m->curCmd || !verts || !indices || vcount <= 0 || icount <= 0)
        return;
    if (!m->bindlessReady)
        return; // caller checks BindlessTerrainActive(), but guard the raw entry too
    const uint32_t stride = 44; // ObjV (40) + uint texIndex
    uint32_t vbytes = (uint32_t)vcount * stride, ibytes = (uint32_t)icount * 2;
    if (m->dynVbUsed + vbytes > (uint32_t)kDynVbBytes ||
        m->idxUsed + ibytes > (uint32_t)kDynVbBytes)
        return;
    VkDeviceSize vboOff = m->dynVbUsed, iboOff = m->idxUsed;
    memcpy((uint8_t*)m->dynVbMap[m->frame] + m->dynVbUsed, verts, vbytes);
    m->dynVbUsed += vbytes;
    memcpy((uint8_t*)m->idxMap[m->frame] + m->idxUsed, indices, ibytes);
    m->idxUsed += ibytes;
    // FF_TEXTURE0 gates the WHOLE texture block in the frag (and the real sample comes from gBindless). It lives in
    // stateFlags -- set ONLY by SetTexture -- and this path never calls SetTexture, so it must be forced on here, else
    // the frag skips texturing and draws flat vertex colour (all-white ground, geometry intact). Bind white to slot 0
    // too so RecordObjectDraw's `!boundTex[0] -> clear FF_TEXTURE0` guard doesn't undo it; white is only sampled for the
    // 0xFFFFFFFF fallback (non-resident tile). Save/restore both so the next real draw isn't clobbered.
    VulkanTexture* savedTex0 = m->boundTex[0];
    uint32_t savedFlags = m->stateFlags;
    m->boundTex[0] = m->whiteTex;
    m->stateFlags |= FF_TEXTURE0;
    m->bindlessDraw = true;
    m->boundObjPipe =
        VK_NULL_HANDLE; // the bindless pipeline is a distinct variant; force a fresh bind
    RecordObjectDraw(m,
                     GetObjPipe(m, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                                m->blend, m->depthWrite, m->depthTest, true),
                     m->dynVb[m->frame], vboOff, true, iboOff, icount, 0);
    m->bindlessDraw =
        false; // one-shot: never leak FF_BINDLESS / the set-1 bind onto the cockpit or objects
    m->boundTex[0] = savedTex0;
    m->stateFlags = savedFlags;
    m->boundObjPipe =
        VK_NULL_HANDLE; // next non-bindless draw must re-bind its own pipeline variant
}
bool VulkanRenderer::BindlessTerrainActive() const
{
    return m->bindlessReady;
}

bool VulkanRenderer::IsIRGrey() const
{
    return m->irGrey;
}

bool VulkanRenderer::IsNvgMode() const
{
    return m->nvg;
}

// #78: the sun exactly as SetLights delivered it -- the same lamp lighting the
// cockpit, not the leftover CDXEngine::TheSun.
bool VulkanRenderer::GetSunLight(float dir[3], float color[3],
                                 float ambient[3]) const
{
    ambient[0] = m->ambient[0];
    ambient[1] = m->ambient[1];
    ambient[2] = m->ambient[2];

    for (int l = 0; l < m->numLights && l < Impl::kMaxLights; ++l)
    {
        if (m->lights[l].params[1] >= 0.5f)
            continue; // point lamp, not the sun

        dir[0] = m->lights[l].direction[0];
        dir[1] = m->lights[l].direction[1];
        dir[2] = m->lights[l].direction[2];
        color[0] = m->lights[l].color[0];
        color[1] = m->lights[l].color[1];
        color[2] = m->lights[l].color[2];
        return true;
    }

    return false;
}

bool VulkanRenderer::GetFogParams(float& start, float& end, float rgb[3]) const
{
    if (m->fogEnd <= m->fogStart)
        return false;

    start = m->fogStart;
    end = m->fogEnd;
    rgb[0] = (float)((m->fogArgb >> 16) & 0xFF) / 255.0f;
    rgb[1] = (float)((m->fogArgb >> 8) & 0xFF) / 255.0f;
    rgb[2] = (float)(m->fogArgb & 0xFF) / 255.0f;
    return true;
}

// #107: a destroyed texture hands its slot back. The descriptor stays stale
// until the slot is handed out again, and only that texture referenced it.
void VulkanRenderer::ReleaseBindlessSlot(unsigned int slot)
{
    if (!m->bindlessReady || slot >= Impl::kBindlessMax)
        return;

    // Called from the loader thread when a texture is finally freed.
    std::lock_guard<std::mutex> lk(m->bindlessFreeMutex);
    m->bindlessFree.push_back((uint32_t)slot);
}

// Artscout - 2026 (#78): the same per-view derivation RecordObjectDraw does for
// the object UBO -- base rotation view shifted by the eye offset, per-view proj.
int VulkanRenderer::GetPerViewMatrices(float view[4][16], float proj[4][16])
{
    if (!m->viActive)
        return 0; // flat / monitor view: keep the base matrices

    const int n = (m->viCount >= 1 && m->viCount <= 4) ? m->viCount : 2;
    for (int v = 0; v < 4; ++v)
    {
        const int s = (v < n) ? v : 0; // unused slots mirror view 0
        memcpy(view[v], m->view, 64);
        const float* w = m->viWorldOff[s];
        const float ovx = w[0] * m->view[0] + w[1] * m->view[4] + w[2] * m->view[8];
        const float ovy = w[0] * m->view[1] + w[1] * m->view[5] + w[2] * m->view[9];
        const float ovz = w[0] * m->view[2] + w[1] * m->view[6] + w[2] * m->view[10];
        view[v][12] -= ovx;
        view[v][13] -= ovy;
        view[v][14] -= ovz;
        memcpy(proj[v], m->viHasProj ? m->viProj[s] : m->proj, 64);
    }
    return n;
}

// Artscout - 2026 (#78): the mesh-shader terrain needs the extension AND the
// bindless tile array the pixel shader samples through.
bool VulkanRenderer::MeshTerrainAvailable() const
{
    extern bool g_bTerrainMeshShader;
    return g_bTerrainMeshShader && m->bindlessReady && m->backend &&
           m->backend->MeshShaderSupported();
}

bool VulkanRenderer::CreateTerrainClipmap(int texels, int levels,
                                          int chunksPerSide)
{
    if (!m->device || texels <= 0 || levels <= 0 || levels > 8)
        return false;
    if (m->clipPostImg != VK_NULL_HANDLE && m->clipTexels == texels &&
        m->clipLevels == levels)
    {
        return true;
    }

    // Anything still queued names the OLD images and staging buffers.
    m->clipPending.clear();
    m->clipStageUsed = 0;

    auto makeImage = [&](VkFormat fmt, VkImage& img, void*& alloc,
                         VkImageView& view) -> bool
    {
        VkImageCreateInfo ii{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        ii.imageType = VK_IMAGE_TYPE_2D;
        ii.format = fmt;
        ii.extent = {(uint32_t)texels, (uint32_t)texels, 1};
        ii.mipLevels = 1;
        ii.arrayLayers = (uint32_t)levels;
        ii.samples = VK_SAMPLE_COUNT_1_BIT;
        ii.tiling = VK_IMAGE_TILING_OPTIMAL;
        ii.usage =
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (not FF_VmaImageCreate(&ii, &img, &alloc))
            return false;

        VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        vi.image = img;
        vi.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        vi.format = fmt;
        vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0,
                               (uint32_t)levels};
        return vkCreateImageView(m->device, &vi, nullptr, &view) == VK_SUCCESS;
    };

    if (not makeImage(VK_FORMAT_R32G32B32A32_SFLOAT, m->clipPostImg,
                      m->clipPostAlloc, m->clipPostView))
    {
        return false;
    }
    if (not makeImage(VK_FORMAT_R32_UINT, m->clipInfoImg, m->clipInfoAlloc,
                      m->clipInfoView))
    {
        return false;
    }

    const int tiles = (chunksPerSide > 0) ? chunksPerSide : 1;
    VkBufferCreateInfo bb{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bb.size = (VkDeviceSize)levels * tiles * tiles * 2 * sizeof(float);
    if (bb.size < 256)
        bb.size = 256;
    bb.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (not FF_VmaBufferCreate(&bb, true, &m->clipBounds, &m->clipBoundsAlloc,
                               &m->clipBoundsMap))
    {
        return false;
    }

    // One whole slice, twice over, so a refill plus the other levels' strips
    // fit in a single frame's ring.
    const uint32_t sliceBytes =
        (uint32_t)texels * (uint32_t)texels * (16u + 4u);
    m->clipStageSize = sliceBytes * 2u + (1u << 20);
    for (int f = 0; f < kFrames; ++f)
    {
        VkBufferCreateInfo sb{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        sb.size = m->clipStageSize;
        sb.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        if (not FF_VmaBufferCreate(&sb, true, &m->clipStage[f],
                                   &m->clipStageAlloc[f], &m->clipStageMap[f]))
        {
            return false;
        }
    }

    m->clipTexels = texels;
    m->clipLevels = levels;
    m->clipTiles = tiles;
    m->clipStageUsed = 0;
    return true;
}

// The posts go straight into this frame's staging ring; the copy itself is
// queued, because a transfer cannot be recorded inside a render pass.
void VulkanRenderer::UpdateTerrainClipmap(int level, int x, int y, int w, int h,
                                          const void* postRgba32f,
                                          const void* infoR32u)
{
    if (m->clipPostImg == VK_NULL_HANDLE || !postRgba32f || !infoR32u)
        return;
    if (w <= 0 || h <= 0 || level < 0 || level >= m->clipLevels)
        return;

    const uint32_t frame = m->frame;
    if (frame >= (uint32_t)kFrames || !m->clipStageMap[frame])
        return;

    const uint32_t postBytes = (uint32_t)w * h * 16u;
    const uint32_t infoBytes = (uint32_t)w * h * 4u;
    const uint32_t offP = (m->clipStageUsed + 15u) & ~15u;
    const uint32_t offI = (offP + postBytes + 15u) & ~15u;
    if (offI + infoBytes > m->clipStageSize)
        return; // ring full this frame; the rolling re-scan picks it up later

    memcpy((uint8_t*)m->clipStageMap[frame] + offP, postRgba32f, postBytes);
    memcpy((uint8_t*)m->clipStageMap[frame] + offI, infoR32u, infoBytes);
    m->clipStageUsed = offI + infoBytes;

    Impl::ClipCopy cc;
    cc.slot = frame;
    cc.offPost = offP;
    cc.offInfo = offI;
    cc.level = level;
    cc.x = x;
    cc.y = y;
    cc.w = w;
    cc.h = h;
    m->clipPending.push_back(cc);
}

// Artscout - 2026 (#78): record the queued clipmap copies. MUST be called with
// no render pass open -- VulkanBackend does so right before it begins the scene.
void VulkanRenderer::FlushTerrainClipmapUploads(void* cmd)
{
    // The caller's buffer is the one in the RECORDING state; m->curCmd belongs
    // to another pass and using it aborted inside the driver.
    VkCommandBuffer cb = cmd ? (VkCommandBuffer)cmd : m->curCmd;

    if (m->clipPending.empty() || !cb || m->clipPostImg == VK_NULL_HANDLE ||
        m->clipInfoImg == VK_NULL_HANDLE)
    {
        m->clipPending.clear();
        return;
    }

    const VkPipelineStageFlags meshStages =
        VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT |
        VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT;

    auto barrier = [&](VkImage img, uint32_t layer, VkImageLayout oldL,
                       VkImageLayout newL, VkAccessFlags sa, VkAccessFlags da,
                       VkPipelineStageFlags ss, VkPipelineStageFlags ds)
    {
        VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        b.oldLayout = oldL;
        b.newLayout = newL;
        b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image = img;
        b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, layer, 1};
        b.srcAccessMask = sa;
        b.dstAccessMask = da;
        vkCmdPipelineBarrier(cb, ss, ds, 0, 0, nullptr, 0, nullptr, 1,
                             &b);
    };

    for (size_t i = 0; i < m->clipPending.size(); ++i)
    {
        const Impl::ClipCopy& c = m->clipPending[i];

        // A queued copy must never outlive the buffers it names.
        if (c.slot >= (uint32_t)kFrames || !m->clipStage[c.slot] ||
            c.level < 0 || c.level >= m->clipLevels)
        {
            continue;
        }

        const uint32_t layer = (uint32_t)c.level;
        // A layer is UNDEFINED until its first upload, SHADER_READ_ONLY after.
        const bool first = (m->clipLayerReady[c.level] == 0);
        const VkImageLayout oldL =
            first ? VK_IMAGE_LAYOUT_UNDEFINED :
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        const VkPipelineStageFlags srcStage =
            first ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : meshStages;

        barrier(m->clipPostImg, layer, oldL,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
                VK_ACCESS_TRANSFER_WRITE_BIT, srcStage,
                VK_PIPELINE_STAGE_TRANSFER_BIT);
        barrier(m->clipInfoImg, layer, oldL,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
                VK_ACCESS_TRANSFER_WRITE_BIT, srcStage,
                VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkBufferImageCopy cp{};
        cp.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, layer, 1};
        cp.imageOffset = {c.x, c.y, 0};
        cp.imageExtent = {(uint32_t)c.w, (uint32_t)c.h, 1};

        cp.bufferOffset = c.offPost;
        vkCmdCopyBufferToImage(cb, m->clipStage[c.slot], m->clipPostImg,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &cp);
        cp.bufferOffset = c.offInfo;
        vkCmdCopyBufferToImage(cb, m->clipStage[c.slot], m->clipInfoImg,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &cp);

        barrier(m->clipPostImg, layer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, meshStages);
        barrier(m->clipInfoImg, layer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, meshStages);

        m->clipLayerReady[c.level] = 1;
    }

    m->clipPending.clear();
}

// Artscout - 2026 (#78): set0 layout, UBO ring, pipeline layout and the
// task/mesh/fragment pipeline. Rebuilt if the scene render pass changes.
static VkPipeline EnsureMeshPipeline(VulkanRenderer::Impl* m, int cbBytes)
{
    if (!m->device || !m->backend)
        return VK_NULL_HANDLE;

    VkRenderPass rp = (VkRenderPass)m->backend->GetSceneRenderPass();
    if (rp == VK_NULL_HANDLE)
        return VK_NULL_HANDLE;

    // Cached per pass: rebuilding on every switch cost a pipeline creation
    // twice a frame and handed the draw the wrong view count.
    for (size_t i = 0; i < m->meshPipes.size(); ++i)
    {
        if (m->meshPipes[i].rp == rp)
            return m->meshPipes[i].pipe;
    }

    if (m->meshSetLayout == VK_NULL_HANDLE)
    {
        VkDescriptorSetLayoutBinding b[5]{};
        b[0].binding = 0;
        b[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        b[0].descriptorCount = 1;
        b[1].binding = 1;
        b[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        b[1].descriptorCount = 1;
        b[2].binding = 2;
        b[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        b[2].descriptorCount = 1;
        b[3].binding = 3;
        b[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        b[3].descriptorCount = 1;
        b[4].binding = 4;
        b[4].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        b[4].descriptorCount = 1;
        for (int i = 0; i < 5; ++i)
        {
            b[i].stageFlags = VK_SHADER_STAGE_TASK_BIT_EXT |
                              VK_SHADER_STAGE_MESH_BIT_EXT |
                              VK_SHADER_STAGE_FRAGMENT_BIT;
        }

        VkDescriptorSetLayoutCreateInfo li{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        li.bindingCount = 5;
        li.pBindings = b;
        if (vkCreateDescriptorSetLayout(m->device, &li, nullptr,
                                        &m->meshSetLayout) != VK_SUCCESS)
        {
            return VK_NULL_HANDLE;
        }

        VkDescriptorPoolSize ps[4] = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, kFrames},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, kFrames * 2},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, kFrames},
            {VK_DESCRIPTOR_TYPE_SAMPLER, kFrames}};
        VkDescriptorPoolCreateInfo pi{
            VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pi.maxSets = kFrames;
        pi.poolSizeCount = 4;
        pi.pPoolSizes = ps;
        if (vkCreateDescriptorPool(m->device, &pi, nullptr, &m->meshPool) !=
            VK_SUCCESS)
        {
            return VK_NULL_HANDLE;
        }
        for (int f = 0; f < kFrames; ++f)
        {
            VkDescriptorSetAllocateInfo ai{
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            ai.descriptorPool = m->meshPool;
            ai.descriptorSetCount = 1;
            ai.pSetLayouts = &m->meshSetLayout;
            if (vkAllocateDescriptorSets(m->device, &ai, &m->meshSet[f]) !=
                VK_SUCCESS)
            {
                return VK_NULL_HANDLE;
            }

            VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
            bi.size = (VkDeviceSize)cbBytes;
            bi.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            if (not FF_VmaBufferCreate(&bi, true, &m->meshUbo[f],
                                       &m->meshUboAlloc[f], &m->meshUboMap[f]))
            {
                return VK_NULL_HANDLE;
            }
        }
    }

    if (m->meshLayout == VK_NULL_HANDLE)
    {
        VkDescriptorSetLayout sets[2] = {m->meshSetLayout, m->meshTileLayout};
        VkPipelineLayoutCreateInfo pl{
            VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pl.setLayoutCount =
            (m->meshTileLayout != VK_NULL_HANDLE) ? 2u : 1u;
        pl.pSetLayouts = sets;
        if (vkCreatePipelineLayout(m->device, &pl, nullptr, &m->meshLayout) !=
            VK_SUCCESS)
        {
            return VK_NULL_HANDLE;
        }
    }

    auto makeModule = [&](FFShaderId id, VkShaderModule& out) -> bool
    {
        unsigned int size = 0;
        const void* code = FFGetShaderBlob(id, FFSHADER_SPIRV, &size);
        if (!code || !size)
            return false;
        VkShaderModuleCreateInfo mi{
            VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        mi.codeSize = size;
        mi.pCode = (const uint32_t*)code;
        return vkCreateShaderModule(m->device, &mi, nullptr, &out) ==
               VK_SUCCESS;
    };

    VkShaderModule ts = VK_NULL_HANDLE, ms = VK_NULL_HANDLE,
                   fs = VK_NULL_HANDLE;
    if (not makeModule(FFSHADER_TERRAIN_AS, ts) or
        not makeModule(FFSHADER_TERRAIN_MS, ms) or
        not makeModule(FFSHADER_TERRAIN_PS, fs))
    {
        if (ts)
            vkDestroyShaderModule(m->device, ts, nullptr);
        if (ms)
            vkDestroyShaderModule(m->device, ms, nullptr);
        if (fs)
            vkDestroyShaderModule(m->device, fs, nullptr);
        return VK_NULL_HANDLE;
    }

    VkPipelineShaderStageCreateInfo st[3]{};
    const VkShaderStageFlagBits stages[3] = {VK_SHADER_STAGE_TASK_BIT_EXT,
                                             VK_SHADER_STAGE_MESH_BIT_EXT,
                                             VK_SHADER_STAGE_FRAGMENT_BIT};
    const VkShaderModule mods[3] = {ts, ms, fs};
    const char* names[3] = {"AS_Terrain", "MS_Terrain", "PS_Terrain"};
    for (int i = 0; i < 3; ++i)
    {
        st[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        st[i].stage = stages[i];
        st[i].module = mods[i];
        st[i].pName = names[i];
    }

    VkPipelineViewportStateCreateInfo vp{
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vp.viewportCount = 1;
    vp.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rs{
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    {
        // Off by default: a wrong winding guess hides the whole ground. The
        // scene uses a negative-height viewport, which flips the face order.
        extern bool g_bTerrainMeshCull;
        rs.cullMode =
            g_bTerrainMeshCull ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
    }
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo msaa{
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    // The Vulkan scene target is single-sampled (no MSAA path here yet).
    msaa.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo dss{
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    dss.depthTestEnable = VK_TRUE;
    dss.depthWriteEnable = VK_TRUE;
    // Reversed-Z, matching the rest of the scene.
    dss.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;

    VkPipelineColorBlendAttachmentState cba{};
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo cb{
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cb.attachmentCount = 1;
    cb.pAttachments = &cba;

    const VkDynamicState dyn[2] = {VK_DYNAMIC_STATE_VIEWPORT,
                                   VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dy{
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dy.dynamicStateCount = 2;
    dy.pDynamicStates = dyn;

    // No vertex input / input assembly: a mesh pipeline has no IA stage.
    VkGraphicsPipelineCreateInfo gp{
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    gp.stageCount = 3;
    gp.pStages = st;
    gp.pViewportState = &vp;
    gp.pRasterizationState = &rs;
    gp.pMultisampleState = &msaa;
    gp.pDepthStencilState = &dss;
    gp.pColorBlendState = &cb;
    gp.pDynamicState = &dy;
    gp.layout = m->meshLayout;
    gp.renderPass = rp;
    gp.subpass = 0;

    VkPipeline pipe = VK_NULL_HANDLE;
    const VkResult vr = vkCreateGraphicsPipelines(m->device, VK_NULL_HANDLE, 1,
                                                  &gp, nullptr, &pipe);
    vkDestroyShaderModule(m->device, ts, nullptr);
    vkDestroyShaderModule(m->device, ms, nullptr);
    vkDestroyShaderModule(m->device, fs, nullptr);

    if (vr != VK_SUCCESS)
        return VK_NULL_HANDLE;

    VulkanRenderer::Impl::MeshPipeEntry e;
    e.rp = rp;
    e.pipe = pipe;
    m->meshPipes.push_back(e);
    return pipe;
}

void VulkanRenderer::DrawTerrainMeshShader(const void* constants,
                                           int constantBytes, int chunkCount)
{
    if (!constants || constantBytes <= 0 || chunkCount <= 0)
        return;
    if (m->clipPostImg == VK_NULL_HANDLE || !m->curCmd)
        return;
    VkPipeline meshPipe = EnsureMeshPipeline(m, constantBytes);

    if (meshPipe == VK_NULL_HANDLE)
        return;

    const uint32_t f = m->frame % kFrames;
    if (!m->meshUboMap[f])
        return;

    memcpy(m->meshUboMap[f], constants, (size_t)constantBytes);
    FF_VmaFlush(m->meshUboAlloc[f], 0, (VkDeviceSize)constantBytes);

    VkDescriptorBufferInfo ubo{m->meshUbo[f], 0, (VkDeviceSize)constantBytes};
    VkDescriptorBufferInfo bnd{m->clipBounds, 0, VK_WHOLE_SIZE};
    VkDescriptorImageInfo post{VK_NULL_HANDLE, m->clipPostView,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkDescriptorImageInfo info{VK_NULL_HANDLE, m->clipInfoView,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkDescriptorImageInfo samp{};
    samp.sampler = m->samplers[FILTER_LINEAR][ADDR_WRAP];

    VkWriteDescriptorSet w[5]{};
    for (int i = 0; i < 5; ++i)
    {
        w[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w[i].dstSet = m->meshSet[f];
        w[i].dstBinding = (uint32_t)i;
        w[i].descriptorCount = 1;
    }
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    w[0].pBufferInfo = &ubo;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    w[1].pImageInfo = &post;
    w[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    w[2].pImageInfo = &info;
    w[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    w[3].pBufferInfo = &bnd;
    w[4].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    w[4].pImageInfo = &samp;
    vkUpdateDescriptorSets(m->device, 5, w, 0, nullptr);

    vkCmdBindPipeline(m->curCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipe);
    vkCmdBindDescriptorSets(m->curCmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m->meshLayout, 0, 1, &m->meshSet[f], 0, nullptr);
    if (m->meshTileSet != VK_NULL_HANDLE)
    {
        vkCmdBindDescriptorSets(m->curCmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m->meshLayout, 1, 1, &m->meshTileSet, 0,
                                nullptr);
    }
    // One amplification thread per chunk (AS_GROUP = 32 in ffterrain.hlsl).
    m->backend->CmdDrawMeshTasks(m->curCmd, (unsigned)((chunkCount + 31) / 32));

    // The engine pipelines are not bound any more -- force the next draw to.
    m->boundObjPipe = VK_NULL_HANDLE;
}

void VulkanRenderer::UpdateTerrainChunkBounds(const void* minMaxFloat2,
                                              int count)
{
    if (!m->clipBoundsMap || !minMaxFloat2 || count <= 0)
        return;
    const int cap = m->clipLevels * m->clipTiles * m->clipTiles;
    if (count > cap)
        count = cap;
    memcpy(m->clipBoundsMap, minMaxFloat2, (size_t)count * 2 * sizeof(float));
    FF_VmaFlush(m->clipBoundsAlloc, 0, (VkDeviceSize)count * 2 * sizeof(float));
}
// Artscout - 2026 (VMA): free a texture handed out by LoadTexture* (deferred-retire, frame-safe).
// The manager tracks only retired textures, so anything a caller creates it must also hand back.
void VulkanRenderer::DestroyTexture(struct ID3D11ShaderResourceView* srv)
{
    if (srv and m->texMgr)
        m->texMgr->Destroy(reinterpret_cast<VulkanTexture*>(srv));
}

unsigned int
VulkanRenderer::BindlessTexIndex(struct ID3D11ShaderResourceView* srv)
{
    return m->BindlessIndexFor(reinterpret_cast<VulkanTexture*>(srv));
}
// DynV (28 bytes) -> ObjV (40 bytes): the 2D-effect vertex carries no normal; effects are unlit (params.lit=0).
namespace
{
struct DynV
{
    float p[3];
    unsigned col, spec;
    float tu, tv;
};
}
static uint32_t ConvertDynToObj(const void* dynVerts, int vcount, uint8_t* dst)
{
    const DynV* s = (const DynV*)dynVerts;
    struct ObjV
    {
        float p[3];
        float n[3];
        unsigned col, spec;
        float tu, tv;
    };
    ObjV* d = (ObjV*)dst;
    for (int i = 0; i < vcount; ++i)
    {
        d[i].p[0] = s[i].p[0];
        d[i].p[1] = s[i].p[1];
        d[i].p[2] = s[i].p[2];
        d[i].n[0] = 0;
        d[i].n[1] = 0;
        d[i].n[2] = 1;
        d[i].col = s[i].col;
        d[i].spec = s[i].spec;
        d[i].tu = s[i].tu;
        d[i].tv = s[i].tv;
    }
    return (uint32_t)vcount * 40;
}

// Artscout - 2026 (#104): the 2D-effect pass (tracers / particles / blips) does not go through SetState, so it states
// its own FF_* word -- exactly as D3D12Renderer's Begin*Pass funcs do. Without this the draw would inherit whatever
// state the previous caller happened to leave behind, and lose FF_TEXTURE0 entirely on a cold frame.
void VulkanRenderer::BeginDynamic2D(bool additive)
{
    m->dyn2dAdditive = additive;
    m->drawLit = false;
    m->stateFlags =
        FF_TEXTURE0 |
        FF_VERTEXCOLOR; // FF_TEXTURE0 self-clears when no texture is bound
    m->filter = FILTER_LINEAR;
    m->addr = ADDR_WRAP;
    // Reset world to identity, exactly as D3D12Renderer::BeginDynamic2D does. The DynV vertices are already in WORLD
    // space (the engine builds the smoke-trail / cloud billboards camera-facing on the CPU), so the object shader must
    // NOT multiply them by a leftover model matrix. Without this they inherit the world matrix of the last object
    // drawn -- the aircraft -- so the trail/clouds render in the aircraft's frame: rotated 90deg, "perpendicular to
    // the aircraft axis". RenderDoc showed it cleanly: VS Input vertical, VS Output rotated a clean 90 degrees.
    static const float I[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    memcpy(m->world, I, sizeof(I));
}
// Artscout - 2026 (#104): stage the whole DynV vertex buffer for the frame. DrawDynamic2DIndexed gathers its own
// vertices out of it by index. Peer of D3D12Renderer::UploadDynamic2D. (DrawDynamic2D still streams directly.)
void VulkanRenderer::UploadDynamic2D(const void* dynVerts, int vcount)
{
    m->dyn2DVerts = dynVerts;
    m->dyn2DVcount = vcount;
}
// Artscout - 2026 (#104): primType honoured here too -- the legacy 2D-in-3D effects are not all triangles (tracers and
// blips come through as LINE/POINT lists), and forcing a triangle list drew them as arbitrary triangles.
void VulkanRenderer::DrawDynamic2D(const void* dynVerts, int vcount,
                                   ID3D11ShaderResourceView* srv, int primType)
{
    if (!m->curCmd || !dynVerts || vcount <= 0)
        return;
    // #107: depth-TEST on, depth-WRITE off -- matches D3D12 BeginDynamic2D (m_depthTest=true). World-space effects
    // (smoke trails) are then occluded by closer cockpit/geometry instead of drawing over it; screen-space effects
    // arrive at the near plane (reversed-Z z~1) so they still pass the GREATER test. Was hard-coded depthTest=false.
    VkPipeline pipe = GetObjPipe(
        m, TopoOfPrimType(primType),
        m->dyn2dAdditive ? BLEND_ADDITIVE : BLEND_ALPHA, false, true);
    if (!pipe)
        return;
    uint32_t vbytes = (uint32_t)vcount * 40;
    if (m->dynVbUsed + vbytes > (uint32_t)kDynVbBytes)
        return;
    VkDeviceSize vboOff = m->dynVbUsed;
    ConvertDynToObj(dynVerts, vcount,
                    (uint8_t*)m->dynVbMap[m->frame] + m->dynVbUsed);
    m->dynVbUsed += vbytes;
    if (srv)
        SetTexture(0, srv);
    RecordObjectDraw(m, pipe, m->dynVb[m->frame], vboOff, false, 0, vcount, 0);
}
// Artscout - 2026 (#104): the path DX2D_Flush2DObjects uses for EVERY legacy 2D-in-3D primitive -- smoke TRAILS
// (missile/flare/chaff), tape/ribbon effects, blips. It was a no-op, so only the hand-routed billboards drew and
// smoke trails were absent. Gather this draw's indexed vertices out of the staged DynV buffer into a flat ObjV list
// and draw non-indexed (the index list already encodes tri-list/line-list order), reusing DrawDynamic2D's pipeline.
// Ported from D3D12Renderer::DrawDynamic2DIndexed.
void VulkanRenderer::DrawDynamic2DIndexed(const unsigned short* indices,
                                          int icount,
                                          ID3D11ShaderResourceView* srv,
                                          int primType)
{
    if (!m->curCmd || !indices || icount <= 0 || !m->dyn2DVerts ||
        m->dyn2DVcount <= 0)
        return;
    // #107: depth-test on, write off (matches D3D12) -- smoke trails are world-space and must be occluded by the cockpit.
    VkPipeline pipe = GetObjPipe(
        m, TopoOfPrimType(primType),
        m->dyn2dAdditive ? BLEND_ADDITIVE : BLEND_ALPHA, false, true);
    if (!pipe)
        return;
    const uint32_t stride = 40; // ObjV
    uint32_t vbytes = (uint32_t)icount * stride;
    if (m->dynVbUsed + vbytes > (uint32_t)kDynVbBytes)
        return;
    VkDeviceSize vboOff = m->dynVbUsed;
    // Gather DynV[index] -> ObjV directly into the per-frame ring (no intermediate buffer).
    const DynV* src = (const DynV*)m->dyn2DVerts;
    struct ObjV
    {
        float p[3];
        float n[3];
        unsigned col, spec;
        float tu, tv;
    };
    ObjV* d = (ObjV*)((uint8_t*)m->dynVbMap[m->frame] + m->dynVbUsed);
    for (int i = 0; i < icount; ++i)
    {
        int idx = indices[i];
        if (idx < 0 || idx >= m->dyn2DVcount)
            idx = 0; // guard a stray index (never read past the staged buffer)
        const DynV& s = src[idx];
        d[i].p[0] = s.p[0];
        d[i].p[1] = s.p[1];
        d[i].p[2] = s.p[2];
        d[i].n[0] = 0;
        d[i].n[1] = 0;
        d[i].n[2] = 1;
        d[i].col = s.col;
        d[i].spec = s.spec;
        d[i].tu = s.tu;
        d[i].tv = s.tv;
    }
    m->dynVbUsed += vbytes;
    if (srv)
        SetTexture(0, srv);
    RecordObjectDraw(m, pipe, m->dynVb[m->frame], vboOff, false, 0, icount, 0);
}
// Artscout - 2026 (#104): ensure the persistent HOST-VISIBLE linear RGBA UI image is w x h (recreated on size
// change), mapped for per-frame memcpy, and in GENERAL layout so a scene draw can sample it.
static bool EnsureUiImage(VulkanRenderer::Impl* m, int w, int h)
{
    if (m->uiImg && m->uiW == w && m->uiH == h)
        return true;
    if (m->uiView)
        vkDestroyImageView(m->device, (VkImageView)m->uiView, nullptr);
    if (m->uiImg)
        FF_VmaImageDestroy((VkImage)m->uiImg, (void*)m->uiMem);
    m->uiImg = m->uiMem = m->uiView = 0;
    m->uiMapped = nullptr;
    m->uiW = w;
    m->uiH = h;

    VkImageCreateInfo ii{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ii.imageType = VK_IMAGE_TYPE_2D;
    ii.format = VK_FORMAT_R8G8B8A8_UNORM;
    ii.extent = {(uint32_t)w, (uint32_t)h, 1};
    ii.mipLevels = 1;
    ii.arrayLayers = 1;
    ii.samples = VK_SAMPLE_COUNT_1_BIT;
    ii.tiling = VK_IMAGE_TILING_LINEAR;
    ii.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    ii.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    VkImage img = VK_NULL_HANDLE;
    void* imgAlloc = nullptr;
    if (not FF_VmaImageCreateHost(&ii, &img, &imgAlloc, &m->uiMapped) or
        not m->uiMapped)
        return false;
    VkDeviceMemory mem = (VkDeviceMemory)imgAlloc; // opaque VMA handle in the old slot

    // PREINITIALIZED -> GENERAL (host writes + shader reads live in GENERAL for a linear sampled image)
    VkCommandBufferAllocateInfo cai{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cai.commandPool = (VkCommandPool)m->backend->VkCommandPoolHandle();
    cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cai.commandBufferCount = 1;
    VkCommandBuffer c = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(m->device, &cai, &c);
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(c, &bi);
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    b.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    b.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = img;
    b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    b.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(c, VK_PIPELINE_STAGE_HOST_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                         0, nullptr, 1, &b);
    vkEndCommandBuffer(c);
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &c;
    vkQueueSubmit((VkQueue)m->backend->VkQueueHandle(), 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle((VkQueue)m->backend->VkQueueHandle());
    vkFreeCommandBuffers(
        m->device, (VkCommandPool)m->backend->VkCommandPoolHandle(), 1, &c);

    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vi.image = img;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = VK_FORMAT_R8G8B8A8_UNORM;
    vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    VkImageView view = VK_NULL_HANDLE;
    vkCreateImageView(m->device, &vi, nullptr, &view);
    m->uiImg = (uint64_t)img;
    m->uiMem = (uint64_t)mem;
    m->uiView = (uint64_t)view;
    m->uiTex.image = m->uiImg;
    m->uiTex.view = m->uiView;
    m->uiTex.descriptor = 0;
    m->uiTex.width = w;
    m->uiTex.height = h;
    m->uiTex.sampleGeneral =
        1; // stays in GENERAL (host rewrites it each frame) -> its descriptor must declare GENERAL
    return true;
}

void VulkanRenderer::CompositeUISurface(const void* src565, int w, int h)
{
    if (!src565 || w <= 0 || h <= 0)
        return;
    if (!m->backend->IsSceneRecording())
        return; // no open scene to draw the UI quad into
    if (!EnsureUiImage(m, w, h) || !m->uiMapped)
        return;
    // 565 -> RGBA8: black -> alpha 0 (the 3D behind shows), symbology -> alpha 1. R in the low byte (R8G8B8A8_UNORM).
    const unsigned short* s = (const unsigned short*)src565;
    unsigned* d = (unsigned*)m->uiMapped;
    const int n = w * h;
    for (int i = 0; i < n; ++i)
    {
        unsigned short v = s[i];
        unsigned a = v ? 0xFF000000u : 0u;
        unsigned r = (v >> 11) & 0x1F;
        r = (r << 3) | (r >> 2);
        unsigned g = (v >> 5) & 0x3F;
        g = (g << 2) | (g >> 4);
        unsigned b = v & 0x1F;
        b = (b << 3) | (b >> 2);
        d[i] = a | (b << 16) | (g << 8) | r;
    }
    // Draw a full-screen alpha quad into the scene (last, on top). Identity matrices -> clip-space verts pass
    // through; z = 1 (reversed-Z near) so the GREATER test lays it over everything; alpha blend keys out black.
    m->curCmd = (VkCommandBuffer)m->backend->SceneCommandBuffer();
    if (!m->curCmd)
        return;
    const float I[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    memcpy(m->world, I, 64);
    memcpy(m->view, I, 64);
    memcpy(m->proj, I, 64);
    m->drawLit = false;
    m->stateFlags =
        FF_TEXTURE0 |
        FF_VERTEXCOLOR; // explicit intent: a textured overlay, no legacy state involved
    m->filter = FILTER_LINEAR;
    m->addr =
        ADDR_CLAMP; // smooth upscale, and never wrap the edge of the UI surface
    m->matColor[0] = m->matColor[1] = m->matColor[2] = m->matColor[3] = 1.0f;
    struct ObjV
    {
        float p[3];
        float nrm[3];
        unsigned col, spec;
        float tu, tv;
    };
    auto V = [](float x, float y, float u, float v)
    {
        ObjV o;
        o.p[0] = x;
        o.p[1] = y;
        o.p[2] = 1.0f;
        o.nrm[0] = o.nrm[1] = 0;
        o.nrm[2] = -1;
        o.col = 0xFFFFFFFFu;
        o.spec = 0;
        o.tu = u;
        o.tv = v;
        return o;
    };
    ObjV q[6] = {V(-1, -1, 0, 0), V(1, -1, 1, 0), V(1, 1, 1, 1),
                 V(-1, -1, 0, 0), V(1, 1, 1, 1),  V(-1, 1, 0, 1)};
    uint32_t vbytes = sizeof(q);
    if (m->dynVbUsed + vbytes > (uint32_t)kDynVbBytes)
        return;
    VkDeviceSize vboOff = m->dynVbUsed;
    memcpy((uint8_t*)m->dynVbMap[m->frame] + m->dynVbUsed, q, vbytes);
    m->dynVbUsed += vbytes;
    SetTexture(0, reinterpret_cast<ID3D11ShaderResourceView*>(&m->uiTex));
    RecordObjectDraw(m,
                     GetObjPipe(m, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                                BLEND_ALPHA, false, false),
                     m->dynVb[m->frame], vboOff, false, 0, 6, 0);
}
#ifndef _WIN32
// Artscout - 2026 (#15): box-halve an RGBA buffer (2x2 average -> 1). Used to cap oversized decodes: the 8192x4096
// night starmap (134 MB RGBA) washed the sky flat grey on Vulkan -- the huge single-mip upload did not sample its
// content, and 8k with no mip chain aliases badly on the coarse sky dome regardless. Halving down to a sane cap gives
// a reliable upload AND far less minification aliasing. Caller owns the returned malloc'd buffer.
static unsigned char* FF_BoxHalveRGBA(const unsigned char* s, int w, int h,
                                      int* ow, int* oh)
{
    const int nw = (w > 1) ? (w >> 1) : 1, nh = (h > 1) ? (h >> 1) : 1;
    unsigned char* d = (unsigned char*)malloc((size_t)nw * nh * 4);
    if (!d)
        return nullptr;
    for (int y = 0; y < nh; ++y)
    {
        const int sy0 = y * 2, sy1 = (sy0 + 1 < h) ? sy0 + 1 : sy0;
        for (int x = 0; x < nw; ++x)
        {
            const int sx0 = x * 2, sx1 = (sx0 + 1 < w) ? sx0 + 1 : sx0;
            const unsigned char* a = s + ((size_t)sy0 * w + sx0) * 4;
            const unsigned char* b = s + ((size_t)sy0 * w + sx1) * 4;
            const unsigned char* c = s + ((size_t)sy1 * w + sx0) * 4;
            const unsigned char* e = s + ((size_t)sy1 * w + sx1) * 4;
            unsigned char* o = d + ((size_t)y * nw + x) * 4;
            for (int k = 0; k < 4; ++k)
                o[k] = (unsigned char)(((int)a[k] + b[k] + c[k] + e[k]) >> 2);
        }
    }
    *ow = nw;
    *oh = nh;
    return d;
}
#endif
// Artscout - 2026 (#104): decode an image file to RGBA and upload it (peer of D3D12Renderer::LoadTextureFile).
// Windows: WIC (32bpp RGBA). Linux: stb_image (+ box-halve cap for oversized). The opaque handle IS a VulkanTexture*.
ID3D11ShaderResourceView* VulkanRenderer::LoadTextureFile(const char* path)
{
#ifdef _WIN32
    if (!path || !path[0] || !m->texMgr)
        return nullptr;
    wchar_t wpath[512];
    if (MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, 512) == 0)
        return nullptr;
    CoInitializeEx(NULL,
                   COINIT_APARTMENTTHREADED); // harmless if COM already up
    IWICImagingFactory* fac = NULL;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, NULL,
                                CLSCTX_INPROC_SERVER,
                                __uuidof(IWICImagingFactory), (void**)&fac)) ||
        !fac)
        return nullptr;
    VulkanTexture* out = nullptr;
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
            out = m->texMgr->CreateRGBA(buf, (int)w, (int)h);
        delete[] buf;
    } while (0);
    if (conv)
        conv->Release();
    if (frame)
        frame->Release();
    if (dec)
        dec->Release();
    fac->Release();
    return reinterpret_cast<ID3D11ShaderResourceView*>(out);
#else
    // Artscout - 2026 (#104): Linux decode via stb_image. Case-resolve first (see the include note above), then load as
    // forced 4-channel RGBA -- byte order (R,G,B,A) and the R8G8B8A8_UNORM target match the WIC path exactly, so the
    // controller/hand models texture identically on both backends.
    if (!path || !path[0] || !m->texMgr)
        return nullptr;
    char resolved[4096];
    const char* p =
        (FF_CIResolvePath(path, resolved, sizeof(resolved)) ? resolved : path);
    int w = 0, h = 0, comp = 0;
    unsigned char* buf =
        stbi_load(p, &w, &h, &comp,
                  4); // req_comp=4 -> RGBA regardless of source channel count
    if (!buf)
        return nullptr;
    // Artscout - 2026 (#15): cap enormous decodes (the 8192x4096 starmap). Box-halve until <= TEX_CAP. buf transfers
    // from stbi ownership (stbi_image_free) to malloc ownership (free) on the first halve -- track it so the right
    // free runs. Peer note: the Windows/WIC path does not need this (WIC + D3D upload the 8k fine); it is a Vulkan
    // upload/sampling robustness cap that also cuts minification aliasing on the sky dome.
    const int TEX_CAP =
        8192; // #15: raised so the 8192x4096 starmap loads at FULL res (rule out box-downscale dimming stars)
    bool stbiOwned = true;
    if (w > TEX_CAP || h > TEX_CAP)
    {
        const int ow = w, oh = h;
        while (w > TEX_CAP || h > TEX_CAP)
        {
            int nw = 0, nh = 0;
            unsigned char* d = FF_BoxHalveRGBA(buf, w, h, &nw, &nh);
            if (!d)
                break;
            if (stbiOwned)
                stbi_image_free(buf);
            else
                free(buf);
            buf = d;
            w = nw;
            h = nh;
            stbiOwned = false;
        }
        fprintf(stderr, "[TEXCAP] %s: %dx%d -> %dx%d\n", path, ow, oh, w, h);
    }
    // Artscout - 2026 (#15): for the starmap, dump a few texels + average luminance of the buffer that reaches the GPU.
    // If this is colourful/varied the decode+downscale are fine and the grey is a bind/sample bug; if it is flat grey
    // the data itself is wrong.
    if (strstr(path, "starmap"))
    {
        long lum = 0;
        const long N = (long)w * h;
        for (long i = 0; i < N; i += (N / 4096 > 0 ? N / 4096 : 1))
            lum += buf[i * 4] + buf[i * 4 + 1] + buf[i * 4 + 2];
        const long cnt = (N / (N / 4096 > 0 ? N / 4096 : 1)) + 1;
        const unsigned char* ctr = buf + ((size_t)(h / 2) * w + w / 2) * 4;
        fprintf(
            stderr,
            "[STARBUF] %dx%d center=(%u,%u,%u,%u) tl=(%u,%u,%u) avgLum=%.1f\n",
            w, h, ctr[0], ctr[1], ctr[2], ctr[3], buf[0], buf[1], buf[2],
            (double)lum / (double)(cnt * 3));
        fflush(stderr);
    }
    VulkanTexture* out = m->texMgr->CreateRGBA(buf, w, h);
    if (stbiOwned)
        stbi_image_free(buf);
    else
        free(buf);
    return reinterpret_cast<ID3D11ShaderResourceView*>(out);
#endif
}
ID3D11ShaderResourceView* VulkanRenderer::LoadTextureRGBA(const void* rgba,
                                                          int w, int h)
{
    if (!m->texMgr)
        return nullptr;
    VulkanTexture* t = m->texMgr->CreateRGBA(rgba, w, h);
    return reinterpret_cast<ID3D11ShaderResourceView*>(
        t); // opaque handle round-trips as VulkanTexture*
}
// Artscout - 2026 (#104): expose the renderer's texture manager so devmgr publishes it as g_pVulkanTextureManager.
VulkanTextureManager* VulkanRenderer::TextureManager() const
{
    return m->texMgr;
}
// Artscout - 2026 (#104): the scene's lights for the object path -- light 0 is the sun (directional), the rest are
// dynamic point lamps (muzzle flashes / explosions) that dxlightengine picks per object. This used to keep only the
// ambient and DROP the array, while RecordObjectDraw made up a fixed sun direction -- so the cockpit was lit by a
// constant that had nothing to do with the sky, and dynamic lamps did not exist. Ported to match D3D12Renderer.
void VulkanRenderer::SetLights(const float ambient[4], int numLights,
                               const void* lights, int lightStride)
{
    if (ambient)
        memcpy(m->ambient, ambient, sizeof(m->ambient));
    m->numLights =
        (numLights < 0) ?
            0 :
            (numLights > Impl::kMaxLights ? Impl::kMaxLights : numLights);
    if (!lights || m->numLights <= 0)
        return;
    // The caller hands over GpuLightCPU[] -- the same 4x float4 layout as GpuLightUbo -- but honour its stride
    // rather than assuming contiguity (dxlightengine passes sizeof(lights[0]); a future caller may pass wider).
    const char* src = (const char*)lights;
    const size_t stride =
        (lightStride > 0) ? (size_t)lightStride : sizeof(Impl::GpuLightUbo);
    for (int i = 0; i < m->numLights; ++i)
        memcpy(&m->lights[i], src + (size_t)i * stride,
               sizeof(Impl::GpuLightUbo));
}
// Per-model VB (from VulkanVbManager) -> vbHandle is a VulkanVb*. Indices are streamed into the per-frame ring.
// Artscout - 2026 (#104): primType is the BSP surface's dwPrimType and it MUST be honoured. It used to be discarded,
// so every object surface was drawn as a triangle LIST whatever it really was. Most surfaces are lists, so the model
// looked right and the bug hid -- until a TRIANGLEFAN surface (e.g. the 22-index one on the F-16 exterior) came
// through: read as a list it becomes a handful of triangles between unrelated vertices, i.e. the black wedges across
// the aircraft. Vulkan has no fan topology worth relying on (VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN is not supported by
// portability implementations), so fans are expanded to a list exactly as D3D12Renderer::DrawObjectIndexed does.
void VulkanRenderer::DrawObjectIndexed(int primType, void* vbHandle,
                                       int /*stride*/, int baseVertex,
                                       const unsigned short* indices,
                                       int indexCount)
{
    if (!m->curCmd || !vbHandle || !indices || indexCount <= 0)
        return;
    // Artscout - 2026 (#104): honour the blend state -- the canopy glass (and other translucent surfaces) set
    // SetObjectAlphaBlend, so pick the alpha/additive pipeline (depth-write OFF) instead of always the opaque one.
    // Blend/depth come from the current legacy state (SetState), possibly overridden by SetObjectAlphaBlend for the
    // hand-sorted translucent pass -- which is what keeps the canopy from drawing opaque over the outside view.
    const bool fan = (primType == 6);
    VkPipeline pipe =
        StateObjPipe(m, fan ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST :
                              TopoOfPrimType(primType));
    if (!pipe)
        return;
    VkBuffer vb = (VkBuffer)((VulkanVb*)vbHandle)->buffer;

    // Artscout - 2026 (sensor video): the display-RTT pass silently drops vkCmdDrawIndexed (the same
    // backend quirk DrawTLIndexed works around for the 2D path) -- every indexed BSP object recorded
    // into a sensor MFD never rasterized. Expand to a NON-indexed run from the model's CPU shadow,
    // exactly like the screen path does. Applies only while an RTT is bound; the eye scene keeps the
    // fast indexed path.
    if (m->backend->IsRttActive() && !m->backend->IsMenuActive() &&
        !m->backend->IsTailActive())
    {
        const VulkanVb* vbo = (const VulkanVb*)vbHandle;
        if (!vbo->shadow.empty())
        {
            const uint32_t stride = 40; // ObjV
            static std::vector<unsigned short> flat;
            flat.clear();
            if (fan)
            {
                for (int i = 0; i + 2 < indexCount; ++i)
                {
                    flat.push_back(indices[0]);
                    flat.push_back(indices[i + 1]);
                    flat.push_back(indices[i + 2]);
                }
            }
            else
                flat.assign(indices, indices + indexCount);
            const uint32_t n = (uint32_t)flat.size();
            const uint32_t vbytes = n * stride;
            if (n == 0 || m->dynVbUsed + vbytes > (uint32_t)kDynVbBytes)
                return;
            const size_t maxVert = vbo->shadow.size() / stride;
            uint8_t* dst = (uint8_t*)m->dynVbMap[m->frame] + m->dynVbUsed;
            for (uint32_t i = 0; i < n; ++i)
            {
                const size_t src = (size_t)baseVertex + flat[i];
                if (src >= maxVert)
                    return; // corrupt index -- refuse the whole draw
                memcpy(dst + (size_t)i * stride,
                       vbo->shadow.data() + src * stride, stride);
            }
            VkDeviceSize vboOff = m->dynVbUsed;
            m->dynVbUsed += vbytes;
            RecordObjectDraw(m, pipe, m->dynVb[m->frame], vboOff, false, 0,
                             (int)n, 0);
            return;
        }
    }

    if (fan)
    { // TRIANGLEFAN -> triangle list: (i[0], i[k+1], i[k+2]) for each of the count-2 triangles
        const int tris = indexCount - 2;
        if (tris <= 0)
            return;
        const int nIdx = tris * 3;
        const uint32_t ibytes = (uint32_t)nIdx * 2;
        if (m->idxUsed + ibytes > (uint32_t)kDynVbBytes)
            return;
        VkDeviceSize iboOff = m->idxUsed;
        unsigned short* dst =
            (unsigned short*)((uint8_t*)m->idxMap[m->frame] + m->idxUsed);
        for (int i = 0; i < tris; ++i)
        {
            dst[i * 3 + 0] = indices[0];
            dst[i * 3 + 1] = indices[i + 1];
            dst[i * 3 + 2] = indices[i + 2];
        }
        m->idxUsed += ibytes;
        RecordObjectDraw(m, pipe, vb, 0, true, iboOff, nIdx, baseVertex);
        return;
    }

    uint32_t ibytes = (uint32_t)indexCount * 2;
    if (m->idxUsed + ibytes > (uint32_t)kDynVbBytes)
        return;
    VkDeviceSize iboOff = m->idxUsed;
    memcpy((uint8_t*)m->idxMap[m->frame] + m->idxUsed, indices, ibytes);
    m->idxUsed += ibytes;
    RecordObjectDraw(m, pipe, vb, 0, true, iboOff, indexCount, baseVertex);
}
// The non-indexed object draw. Despite the name this is NOT always a strip: dxengine routes D3D3PT_POINTLIST here
// (see dxengine.cpp ":822"), and it used to be hardwired to TRIANGLE_STRIP -- so a point cloud came out as triangles.
void VulkanRenderer::DrawObjectStrip(int primType, void* vbHandle,
                                     int /*stride*/, int startVertex,
                                     int vertexCount)
{
    if (!m->curCmd || !vbHandle || vertexCount <= 0)
        return;
    VkBuffer vb = (VkBuffer)((VulkanVb*)vbHandle)->buffer;
    RecordObjectDraw(m, StateObjPipe(m, TopoOfPrimType(primType)), vb, 0, false,
                     0, vertexCount, startVertex);
}
// Artscout - 2026 (#104): GPU-instanced particle billboards (smoke/fire/explosions/tracers). Peer of
// D3D12Renderer::DrawParticlesInstanced. `inst` = array of D3D12ParticleInstance (44 bytes each); atlasSrv = the atlas
// (a VulkanTexture*); blendMode 0=additive(glow), 1=alpha, 2=premul(->alpha here). One instanced draw of the unit quad.
void VulkanRenderer::DrawParticlesInstanced(const void* inst, int count,
                                            void* atlasSrv, int blendMode)
{
    if (!inst || count <= 0 || !atlasSrv)
        return;
    // Sensor RTT / menu / tail: the particle pipeline exists ONLY for the multiview scene pass, and the old
    // unconditional EnsureSceneStarted() below opened a FLAT 1-view scene MID-TAIL whenever the sensor scene drew
    // its clouds/haze -> scene-pass change -> PurgePipelinesIfScenePassChanged destroyed pipelines the already-
    // recorded RTT/tail command buffers still reference -> undefined behavior: random garbage across the atlas
    // (HUD/DED zones wiped in sync with TGP slew) and driver-level crashes. Skip particles for the sensor view
    // (symbology parity; an RTT-pass particle variant is a later increment) and never touch the scene mid-tail.
    if (m->backend->IsMenuActive() || m->backend->IsRttActive() ||
        m->backend->IsTailActive())
        return;
    m->backend->EnsureSceneStarted(); // particles belong to the 3D scene frame
    {
        extern bool g_bGpuDraw;
        g_bGpuDraw = true;
    }
    m->curCmd = (VkCommandBuffer)m->backend->SceneCommandBuffer();
    if (!m->curCmd || !EnsureParticlePipe(m) || m->uboSlot >= kMaxDrawsPerFrame)
        return; // #104: the shared budget

    // stream the instance array into the per-frame dynamic VB (slot 1 of the particle pipeline).
    const uint32_t ibytes = (uint32_t)count * 44;
    if (m->dynVbUsed + ibytes > (uint32_t)kDynVbBytes)
        return;
    VkDeviceSize instOff = m->dynVbUsed;
    memcpy((uint8_t*)m->dynVbMap[m->frame] + m->dynVbUsed, inst, ibytes);
    m->dynVbUsed += ibytes;

    // UBO: only view/proj matter for the billboard VS (identity world, neutral material).
    uint8_t* uboBase = (uint8_t*)m->uboRingMap[m->frame] +
                       (VkDeviceSize)m->uboSlot * m->uboStride;
    VulkanRenderer::Impl::ObjUbo* ubo = (VulkanRenderer::Impl::ObjUbo*)uboBase;
    memset(ubo, 0, sizeof(*ubo));
    const float I[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    memcpy(ubo->world, I, 64);
    // #107 VR-Vulkan: view-instancing per-view UBO (peer of D3D12 cbViewStereo). Base rotation view + the per-view
    // world eye offset rotated into view space and shifted into the translation row; per-view proj or the base. Off ->
    // replicate the single view/proj to all layers (flat/mono).
    for (int v = 0; v < 4; ++v)
    {
        if (m->viActive)
        {
            const int s = (v < m->viCount) ? v : 0;
            memcpy(ubo->view[v], m->view, 64);
            const float* w = m->viWorldOff[s];
            float ovx =
                w[0] * m->view[0] + w[1] * m->view[4] + w[2] * m->view[8];
            float ovy =
                w[0] * m->view[1] + w[1] * m->view[5] + w[2] * m->view[9];
            float ovz =
                w[0] * m->view[2] + w[1] * m->view[6] + w[2] * m->view[10];
            ubo->view[v][12] -= ovx;
            ubo->view[v][13] -= ovy;
            ubo->view[v][14] -= ovz;
            memcpy(ubo->proj[v], m->viHasProj ? m->viProj[s] : m->proj, 64);
        }
        else
        {
            memcpy(ubo->view[v], m->view, 64);
            memcpy(ubo->proj[v], m->proj, 64);
        }
    }
    ubo->materialColor[0] = ubo->materialColor[1] = ubo->materialColor[2] =
        ubo->materialColor[3] = 1.0f;

    // descriptor set: binding0 = this UBO slot, binding1 = the atlas (mirrors RecordObjectDraw).
    VulkanTexture* atlas = reinterpret_cast<VulkanTexture*>(atlasSrv);
    if (!atlas || !atlas->view)
        return;
    // #107 PERF: take this slot's PRE-ALLOCATED set (uboSlot < kMaxDrawsPerFrame, budget-checked above) -- no
    // vkAllocateDescriptorSets in the hot path; just update + bind. This is the whole fix for the 26x terrain gap.
    VkDescriptorSet set = m->objSets[m->frame][m->uboSlot];
    VkDescriptorBufferInfo bi{};
    bi.buffer = m->uboRing[m->frame];
    bi.offset = (VkDeviceSize)m->uboSlot * m->uboStride;
    bi.range = sizeof(VulkanRenderer::Impl::ObjUbo);
    VkDescriptorImageInfo ii{};
    ii.sampler = m->linearSampler;
    ii.imageView = (VkImageView)atlas->view;
    ii.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkWriteDescriptorSet w[2] = {};
    w[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = set;
    w[0].dstBinding = 0;
    w[0].descriptorCount = 1;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    w[0].pBufferInfo = &bi;
    w[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[1].dstSet = set;
    w[1].dstBinding = 1;
    w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[1].pImageInfo = &ii;
    vkUpdateDescriptorSets(m->device, 2, w, 0, nullptr);

    VkPipeline pipe =
        (blendMode == 0) ?
            m->partPipeAdd :
            m->partPipeAlpha; // 0 = additive glow, else straight alpha
    vkCmdBindPipeline(m->curCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
    m->boundObjPipe = VK_NULL_HANDLE;
    m->boundBiasCmd =
        VK_NULL_HANDLE; // #107 PERF: particle pipeline bound -> invalidate object bind cache
    vkCmdBindDescriptorSets(m->curCmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m->objLayout, 0, 1, &set, 0, nullptr);
    VkBuffer vbs[2] = {m->quadVB, m->dynVb[m->frame]};
    VkDeviceSize offs[2] = {0, instOff};
    vkCmdBindVertexBuffers(m->curCmd, 0, 2, vbs, offs);
    vkCmdDraw(m->curCmd, 4, (uint32_t)count, 0,
              0); // unit quad (strip) x count instances
    m->uboSlot++;
}
