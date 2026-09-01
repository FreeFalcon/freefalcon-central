//-----------------------------------------------------------------------------
// VulkanRenderer.h -- Artscout - 2026 (#104, Linux port -- subsystem 2: renderer).
//
// The Linux implementation of IRenderer (the API-neutral draw/state/pass surface), peer of the Windows
// D3D12Renderer. The engine draws through the single global g_pRenderer, so there is NO call-site churn -- this is
// a proper backend split, instantiated where g_pRenderer is created (devmgr.cpp). The Windows D3D12 path is
// untouched. VulkanRenderer draws into the swapchain (flat) or the VulkanBackend multiview array target (VR/quad),
// sharing that backend's device / queue / render passes.
//
// Shaders: the shared FFEmu.hlsl set is compiled HLSL->SPIR-V (DXC/glslang) at build time; scene pipelines are
// built against VulkanBackend::GetSceneRenderPass() so gl_ViewIndex multiview works. Built incrementally
// (screen -> 2D -> object -> terrain -> clouds); methods land real, not as throwaway stubs.
//
// All Vulkan detail is behind a pimpl so this header pulls neither <vulkan/vulkan.h> nor SDL.
//-----------------------------------------------------------------------------
#ifndef FF_VULKAN_RENDERER_H
#define FF_VULKAN_RENDERER_H

#include "../dxengine/common/irenderer.h" // the neutral draw surface (+ ScreenVertex/GpuLightCPU/VrTriVtx)

class VulkanBackend;

class VulkanRenderer : public IRenderer
{
public:
    explicit VulkanRenderer(VulkanBackend* backend);
    ~VulkanRenderer() override;

    // ---- lifecycle ----
    bool Init(const char* shaderDir) override;
    void Release() override;
    bool IsValid() const override;

    // ---- per-frame / per-pass state ----
    void SetViewportSize(int w, int h) override;
    void SetView(const float* m4x4_rowmajor) override;
    void SetProj(const float* m4x4_rowmajor) override;
    // Artscout - 2026 (#107 VR-Vulkan multiview): the peer of D3D12Renderer's view-instancing API. worldOff = per-view
    // eye position offset in WORLD space (xyz per view); projs = per-view projection (row-major 4x4, e.g. quad off-axis)
    // or NULL for the shared base projection (symmetric stereo). The per-view UBO (view[gl_ViewIndex]/proj[gl_ViewIndex])
    // is then built from the LIVE base view (its translation row shifted by the view-space eye offset) + per-view proj,
    // exactly like D3D12's cbViewStereo. SetViewInstancing(true) activates it; false returns to the flat/mono fill.
    void SetViewInstancingParams(int nViews, const float* worldOff,
                                 const float* projs);
    void SetViewInstancing(bool on);
    // #107 VR-Vulkan: rewind the per-frame draw rings for a new eye (VR renders up to 4 eyes per real frame; the rings
    // are sized per frame, so without this the later eyes' draws overflow and are dropped). Safe: each eye's GPU work
    // finishes before the next (EndEye waits sceneFence).
    void ResetFrameRing();
    void SetWorld(const float* m4x4_rowmajor) override;
    void SetCameraPos(float x, float y, float z) override;
    void SetState(int legacyState) override;

    // ---- render parameters ----
    void SetTexture(unsigned slot, ID3D11ShaderResourceView* srv) override;
    void SetChromaKey(unsigned long argb, float tolerance) override;
    void SetFog(unsigned long argb, float start, float end) override;
    void SetAlphaRef(float ref01) override;
    void SetMaterialColor(float r, float g, float b, float a) override;
    void SetMaterialSpecular(float r, float g, float b, float power) override;
    void SetObjectAlphaBlend(bool on) override;
    void SetObjectAdditiveBlend(bool on) override;
    void SetAlphaTestEnabled(bool on) override;
    void SetEmissive(bool on) override;
    void SetAfterburner(bool on) override;
    void SetCockpitPass(bool on) override;
    void SetIRGrey(bool on) override;
    void SetNvgMode(bool on) override;
    void SetFullBright(bool on) override;
    void SetTexColorDiffuse(bool on) override;
    void SetForcePerSample(bool on) override;
    void SetStencil(int mode, unsigned ref) override;
    void SetHudStencil(int mode) override;

    // ---- passes ----
    void BeginScreenPass() override;
    void BeginObjectPass() override;
    void BeginTerrainPass() override;
    void SetTerrainRasterForLod(int level) override;
    void RebuildTerrainRasters() override;
    void BeginSkyPass(bool blend) override;

    // ---- draws ----
    void DrawTL(int primType, const ScreenVertex* verts, int count) override;
    void DrawTLIndexed(int primType, const ScreenVertex* verts, int vcount,
                       const unsigned short* indices, int icount) override;
    void DrawColorTrisScreen(const ScreenVertex* verts, int count,
                             ID3D11ShaderResourceView* tex, int opaque,
                             int cull) override;
    void DrawBitmap2D(int dX, int dY, int w, int h, int totalWidth, int sX,
                      int sY, const unsigned* pSrc, bool fit, int screenW,
                      int screenH) override;
    void DrawGlocOverlay(float intensity, float innerR, float outerR,
                         float tintR, float tintG, float tintB) override;
    void DrawTerrainMesh(const void* verts, int vcount,
                         const unsigned short* indices, int icount) override;
    // Artscout - 2026 (#107 PERF): bindless terrain -- see IRenderer.
    bool BindlessTerrainActive() const override;
    unsigned int
    BindlessTexIndex(struct ID3D11ShaderResourceView* srv) override;
    void DrawTerrainMeshBindless(const void* verts, int vcount,
                                 const unsigned short* indices,
                                 int icount) override;
    // Artscout - 2026 (#78): mesh-shader terrain clipmap -- see IRenderer.
    bool MeshTerrainAvailable() const override;
    bool CreateTerrainClipmap(int texels, int levels,
                              int chunksPerSide) override;
    void UpdateTerrainClipmap(int level, int x, int y, int w, int h,
                              const void* postRgba32f,
                              const void* infoR32u) override;
    void UpdateTerrainChunkBounds(const void* minMaxFloat2,
                                  int count) override;
    void DrawTerrainMeshShader(const void* constants, int constantBytes,
                               int chunkCount) override;
    int GetPerViewMatrices(float view[4][16], float proj[4][16]) override;
    bool IsIRGrey() const override;
    bool IsNvgMode() const override;
    bool GetFogParams(float& start, float& end, float rgb[3]) const override;
    bool GetSunLight(float dir[3], float color[3],
                     float ambient[3]) const override;
    // #107: hand a destroyed texture's bindless slot back for reuse.
    void ReleaseBindlessSlot(unsigned int slot);
    // Records the queued clipmap copies into `cmd` (a RECORDING buffer), with no
    // render pass open. NULL falls back to the renderer's current buffer.
    void FlushTerrainClipmapUploads(void* cmd = 0);
    void BeginDynamic2D(bool additive) override;
    void UploadDynamic2D(const void* dynVerts, int vcount) override;
    void DrawDynamic2D(const void* dynVerts, int vcount,
                       ID3D11ShaderResourceView* srv, int primType) override;
    void DrawDynamic2DIndexed(const unsigned short* indices, int icount,
                              ID3D11ShaderResourceView* srv,
                              int primType) override;

    // ---- UI + textures ----
    void CompositeUISurface(const void* src565, int w, int h) override;
    ID3D11ShaderResourceView* LoadTextureFile(const char* path) override;
    ID3D11ShaderResourceView* LoadTextureRGBA(const void* rgba, int w,
                                              int h) override;
    void DestroyTexture(struct ID3D11ShaderResourceView* srv); // frees a LoadTexture* result (deferred)

    // ---- object / BSP path ----
    void SetLights(const float ambient[4], int numLights, const void* lights,
                   int lightStride) override;
    void DrawObjectIndexed(int primType, void* vbHandle, int stride,
                           int baseVertex, const unsigned short* indices,
                           int indexCount) override;
    void DrawObjectStrip(int primType, void* vbHandle, int stride,
                         int startVertex, int vertexCount) override;
    void DrawParticlesInstanced(const void* inst, int count, void* atlasSrv,
                                int blendMode) override;

    // Artscout - 2026 (#104): the renderer's texture manager, exposed so devmgr can publish it as the global
    // g_pVulkanTextureManager (the engine's Tex.cpp creates engine textures through the same manager the renderer
    // samples). Valid after Init().
    class VulkanTextureManager* TextureManager() const;

    // Pimpl forward-decl public so the .cpp's file-local pipeline helpers can name it; defined in the .cpp.
    struct Impl;

private:
    Impl* m;
};

// The active Vulkan renderer, mirrored into the neutral g_pRenderer by DXContext::Init when g_bUseVulkan.
extern VulkanRenderer* g_pVulkanRenderer;

#endif // FF_VULKAN_RENDERER_H
