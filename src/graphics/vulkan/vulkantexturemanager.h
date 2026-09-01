//-----------------------------------------------------------------------------
// VulkanTextureManager.h -- Artscout - 2026 (#104, Linux port -- subsystem 2).
// Owns Vulkan textures for VulkanRenderer. The engine passes textures around as opaque ID3D11ShaderResourceView*
// handles (see IRenderer); on Vulkan each handle is really a VulkanTexture* (VkImage + memory + VkImageView), which
// the renderer binds via a combined-image-sampler descriptor. Peer of D3D12TextureManager. DXT/BC compressed
// uploads (the on-disk texture format) land as the object/terrain paths need them; RGBA is the first cut.
//-----------------------------------------------------------------------------
#ifndef FF_VULKAN_TEXTUREMANAGER_H
#define FF_VULKAN_TEXTUREMANAGER_H

#include <cstdint>

class VulkanBackend;

// One GPU texture. The opaque handle the engine holds (cast to/from ID3D11ShaderResourceView*) IS a VulkanTexture*.
struct VulkanTexture
{
    uint64_t image = 0; // VkImage
    uint64_t memory = 0; // VMA allocation handle (opaque; freed via FF_VmaImageDestroy)
    uint64_t view = 0; // VkImageView  (what the descriptor samples)
    uint64_t descriptor =
        0; // VkDescriptorSet (lazily built by the renderer; 0 until then)
    // #107 PERF bindless: this texture's slot in the renderer's tile array (+1 -- 0 means "unassigned", so a fresh
    // texture always re-registers). Stored IN the texture, NOT an external view->slot map: the same anti-aliasing
    // reason as `descriptor` above (a recycled VkImageView handle would otherwise return a stale slot). See BindlessIndexFor.
    uint32_t bindlessSlot = 0;
    int width = 0, height = 0;
    // Artscout - 2026 (#104): the layout the shader samples this in. Almost every texture ends in SHADER_READ_ONLY
    // (0). The UI-composite image is the exception: it is a mapped LINEAR image the host rewrites every frame, so it
    // must stay in GENERAL, and its descriptor has to declare GENERAL to match -- otherwise the sample is a
    // validation error (VUID-vkCmdDraw-None-09600) and a layout hazard on some drivers. 0 = SHADER_READ_ONLY, 1 = GENERAL.
    int sampleGeneral = 0;
};

class VulkanTextureManager
{
public:
    explicit VulkanTextureManager(VulkanBackend* backend);
    ~VulkanTextureManager();

    bool Init();
    void Release();

    // Create an immutable texture from a tightly-packed 32bpp RGBA buffer (rowPitch = w*4). Null on failure.
    VulkanTexture* CreateRGBA(const void* rgba, int w, int h);
    // Create an immutable texture from block-compressed data (the on-disk DXT format), sampled natively by the GPU.
    // bcType: 1 = BC1/DXT1, 2 = BC2/DXT3, 3 = BC3/DXT5. `data`/`bytes` = the whole mip-0 block stream. Null on fail.
    VulkanTexture* CreateBC(const void* data, unsigned bytes, int w, int h,
                            int bcType);
    // A 1x1 opaque-white texture used when a draw binds no texture (so the sampler is always valid).
    VulkanTexture* White();

    // Artscout - 2026 (#104): Vulkan twins of the D3D12TextureManager surface the engine's Tex.cpp expects.
    bool IsValid() const;
    // Immutable 2D texture from tightly-packed mip-0 pixels in a DXGI format. The engine computes the SAME DXGI
    // value it feeds the D3D12 path (DxgiFormatFromMPR); this maps it to a VkFormat. Handles BC1/2/3 (DXT) and
    // R8G8B8A8 / B8G8R8A8 / B5G6R5. `bytes` is the mip-0 byte count (block stream for BC). Null on failure.
    VulkanTexture* CreateFromDxgi(const void* data, unsigned bytes, int w,
                                  int h, int dxgiFormat);
    // #DX12 п.3 RTT peer: a render-target texture (VK_IMAGE_USAGE_COLOR_ATTACHMENT + SAMPLED), RGBA8. Null on fail.
    VulkanTexture* CreateRenderTarget(int w, int h);

    // Artscout - 2026 (#104): a texture the engine drops (a terrain tile scrolling out, an exploded object) is NOT
    // freed on the spot -- the scene command buffer of a frame still in flight may have its image view bound, and
    // freeing it there loses the device (VkImageView destroyed while a recorded draw references it -> the "freeze in
    // a turn / on an explosion" hang). Destroy hands it to a retire queue instead; TickRetire, called once per frame
    // by the renderer, frees the entries that have outlived every in-flight frame.
    void Destroy(VulkanTexture* tex);
    void
    TickRetire(); // renderer calls this once per real frame (see VulkanRenderer SyncFrame)
    // The renderer hands over its combined-image-sampler descriptor pool (as a VkDescriptorPool u64) so that when a
    // texture is finally freed its descriptor set is returned to the pool. Without this the pool leaks one set per
    // texture ever destroyed -- a slow bleed that exhausts it over a long flight as terrain tiles stream in and out.
    void BindDescriptorPool(uint64_t descriptorPool);

private:
    struct Impl;
    Impl* m;
};

// Artscout - 2026 (#104): active Vulkan texture manager (peer of g_pD3D12TextureManager). Set in devmgr when the
// Vulkan backend comes up (points at the renderer's manager). Tex.cpp creates engine textures through it.
extern VulkanTextureManager* g_pVulkanTextureManager;

#endif // FF_VULKAN_TEXTUREMANAGER_H
