//-----------------------------------------------------------------------------
// VulkanTextureManager.cpp -- Artscout - 2026 (#104, Linux port -- subsystem 2).
//-----------------------------------------------------------------------------
#include <ciso646> // not/and/or tokens under MSVC
#include "vulkantexturemanager.h"
#include "vulkanrenderer.h" // #107: hand bindless slots back on free
#include "vulkanbackend.h"
#include "vulkanvma.h"

#include <vulkan/vulkan.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <mutex>
// #104: shared VkQueue lock (defined in VulkanBackend.cpp). Texture uploads run on loader threads and submit to the
// SAME queue the render thread presents on -- every queue op must hold this or the driver deadlocks.
extern std::mutex g_vkQueueMutex;
// #104: guards THIS manager's private command pool against its own callers (several loader threads upload at once).
static std::mutex g_texUploadMutex;

struct VulkanTextureManager::Impl
{
    VulkanBackend* backend = nullptr;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    VkCommandPool pool = VK_NULL_HANDLE;
    VulkanTexture* white = nullptr;

    // #104: deferred destruction (see Destroy/TickRetire). framesLeft starts at kRetireFrames so the texture
    // outlives every command buffer that could still reference it. Guarded because Destroy runs on the engine
    // thread while TickRetire runs on the render thread (they can be the same, but need not be).
    struct Retired
    {
        VulkanTexture* tex;
        int framesLeft;
    };
    std::vector<Retired> retired;
    std::mutex retireMutex;
    VkDescriptorPool descSetPool =
        VK_NULL_HANDLE; // renderer's set pool (see BindDescriptorPool), for freeing sets
    // kMaxFramesInFlight (2) frames can be in flight, plus the one being recorded -> 3 is the minimum safe delay.
    static const int kRetireFrames = 3;
    void FreeNow(VulkanTexture* tex)
    {
        // #107: hand the bindless slot back, or the array leaks one entry per
        // destroyed tile and runs dry after a couple of 3D entries.
        if (tex->bindlessSlot && g_pVulkanRenderer)
        {
            g_pVulkanRenderer->ReleaseBindlessSlot(tex->bindlessSlot - 1);
            tex->bindlessSlot = 0;
        }

        if (tex->descriptor && descSetPool)
        {
            VkDescriptorSet s = (VkDescriptorSet)tex->descriptor;
            vkFreeDescriptorSets(
                device, descSetPool, 1,
                &s); // pool is FREE_DESCRIPTOR_SET_BIT; frees on render thread
        }
        if (tex->view)
            vkDestroyImageView(device, (VkImageView)tex->view, nullptr);
        // tex->memory holds the VMA allocation handle (same 8-byte slot the
        // raw VkDeviceMemory used to occupy); image + memory free in one call.
        if (tex->image)
            FF_VmaImageDestroy((VkImage)tex->image, (void*)tex->memory);
        delete tex;
    }

    // One-time-submit command buffer for uploads. Submit() TAKES g_texUploadMutex and Flush() RELEASES it: everything
    // between them records into a buffer from our pool, and a command pool is externally synchronized -- so the whole
    // allocate..record..submit..free window must be one critical section against other loader threads. The pair is
    // always matched (callers do `if (Submit(cb)) { record...; Flush(cb); }`) and Submit unlocks on its failure path,
    // so the lock cannot leak. Hand-rolled rather than RAII because the section spans two functions.
    bool Submit(VkCommandBuffer& cb)
    {
        g_texUploadMutex.lock();
        VkCommandBufferAllocateInfo ai{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        ai.commandPool = pool;
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(device, &ai, &cb) != VK_SUCCESS)
        {
            g_texUploadMutex.unlock();
            return false;
        }
        VkCommandBufferBeginInfo bi{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cb, &bi);
        return true;
    }
    void Flush(VkCommandBuffer cb)
    {
        vkEndCommandBuffer(cb);
        VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        si.commandBufferCount = 1;
        si.pCommandBuffers = &cb;
        {
            std::lock_guard<std::mutex> _qlk(
                g_vkQueueMutex); // serialize queue use with the render thread
            vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE);
            vkQueueWaitIdle(queue);
        }
        vkFreeCommandBuffers(device, pool, 1, &cb);
        g_texUploadMutex.unlock(); // paired with Submit() -- see the note there
    }

    // Artscout - 2026 (#104): generic UNCOMPRESSED 2D upload (mip 0). Peer of CreateRGBA but format-parametric,
    // used by CreateFromDxgi for the B8G8R8A8 / R8G8B8A8 / B5G6R5 engine texture paths. `bytes` = w*h*bpp.
    VulkanTexture* Upload2D(const void* data, int w, int h, VkFormat fmt,
                            int bpp)
    {
        if (!device || !data || w <= 0 || h <= 0 || bpp <= 0)
            return nullptr;
        const VkDeviceSize bytes = (VkDeviceSize)w * h * bpp;

        VkBuffer stage = VK_NULL_HANDLE;
        void* stageAlloc = nullptr;
        VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bi.size = bytes;
        bi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        void* map = nullptr;
        if (not FF_VmaBufferCreate(&bi, true, &stage, &stageAlloc, &map) or not map)
            return nullptr;
        memcpy(map, data, (size_t)bytes);
        FF_VmaFlush(stageAlloc, 0, bytes);

        VulkanTexture* tex = new VulkanTexture();
        tex->width = w;
        tex->height = h;
        VkImage img = VK_NULL_HANDLE;
        void* imgAlloc = nullptr;
        VkImageView view = VK_NULL_HANDLE;
        VkImageCreateInfo ii{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        ii.imageType = VK_IMAGE_TYPE_2D;
        ii.format = fmt;
        ii.extent = {(uint32_t)w, (uint32_t)h, 1};
        ii.mipLevels = 1;
        ii.arrayLayers = 1;
        ii.samples = VK_SAMPLE_COUNT_1_BIT;
        ii.tiling = VK_IMAGE_TILING_OPTIMAL;
        ii.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (not FF_VmaImageCreate(&ii, &img, &imgAlloc))
        {
            delete tex;
            FF_VmaBufferDestroy(stage, stageAlloc);
            return nullptr;
        }

        VkCommandBuffer cb;
        if (Submit(cb))
        {
            auto barrier = [&](VkImageLayout oldL, VkImageLayout newL,
                               VkAccessFlags sa, VkAccessFlags da,
                               VkPipelineStageFlags ss, VkPipelineStageFlags ds)
            {
                VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
                b.oldLayout = oldL;
                b.newLayout = newL;
                b.srcQueueFamilyIndex = b.dstQueueFamilyIndex =
                    VK_QUEUE_FAMILY_IGNORED;
                b.image = img;
                b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
                b.srcAccessMask = sa;
                b.dstAccessMask = da;
                vkCmdPipelineBarrier(cb, ss, ds, 0, 0, nullptr, 0, nullptr, 1,
                                     &b);
            };
            barrier(VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
                    VK_ACCESS_TRANSFER_WRITE_BIT,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT);
            VkBufferImageCopy cp{};
            cp.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            cp.imageExtent = {(uint32_t)w, (uint32_t)h, 1};
            vkCmdCopyBufferToImage(
                cb, stage, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &cp);
            barrier(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            Flush(cb);
        }
        FF_VmaBufferDestroy(stage, stageAlloc);

        VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        vi.image = img;
        vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vi.format = fmt;
        vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCreateImageView(device, &vi, nullptr, &view);
        tex->image = (uint64_t)img;
        tex->memory = (uint64_t)imgAlloc; // VMA allocation handle
        tex->view = (uint64_t)view;
        return tex;
    }
};

// Artscout - 2026 (#104): active Vulkan texture manager (peer of g_pD3D12TextureManager).
VulkanTextureManager* g_pVulkanTextureManager = nullptr;

VulkanTextureManager::VulkanTextureManager(VulkanBackend* backend)
    : m(new Impl())
{
    m->backend = backend;
}
VulkanTextureManager::~VulkanTextureManager()
{
    Release();
    delete m;
    m = nullptr;
}

bool VulkanTextureManager::Init()
{
    if (!m->backend || !m->backend->IsValid())
        return false;
    m->device = (VkDevice)m->backend->VkDeviceHandle();
    m->queue = (VkQueue)m->backend->VkQueueHandle();
    // Artscout - 2026 (#104): our OWN VkCommandPool, not the backend's. A VkCommandPool is externally synchronized, and
    // that covers recording into ANY buffer allocated from it -- not just allocate/free. Uploads run on background loader
    // threads while the render thread records its frame, so sharing the backend's pool meant vkCmdCopyBufferToImage and
    // the render thread's vkCmdPipelineBarrier touching one pool at once (validation: "VkCommandPool is simultaneously
    // used in current thread X and thread Y"). A mutex on the shared pool would be the wrong fix -- it would serialize
    // every upload against the whole frame's recording. A private pool costs nothing and removes the sharing entirely.
    // uploadMutex then covers this manager against ITSELF, since several loader threads may upload at once.
    {
        VkCommandPoolCreateInfo pci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        pci.queueFamilyIndex = m->backend->VkGraphicsFamily();
        pci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        if (vkCreateCommandPool(m->device, &pci, nullptr, &m->pool) !=
            VK_SUCCESS)
            return false;
    }
    return m->device != VK_NULL_HANDLE;
}

void VulkanTextureManager::Release()
{
    // The device is going away, so every deferred texture must be freed NOW, not on a future TickRetire that will
    // never come. Callers Release only after the device is idle, so immediate free is safe here.
    if (m->device)
    {
        std::lock_guard<std::mutex> lk(m->retireMutex);
        for (auto& r : m->retired)
            m->FreeNow(r.tex);
        m->retired.clear();
    }
    if (m->white)
    {
        m->FreeNow(m->white);
        m->white = nullptr;
    } // direct free -- do not re-queue during teardown
    if (m->device && m->pool)
    {
        vkDestroyCommandPool(m->device, m->pool, nullptr);
        m->pool = VK_NULL_HANDLE;
    }
}

VulkanTexture* VulkanTextureManager::CreateRGBA(const void* rgba, int w, int h)
{
    if (!m->device || !rgba || w <= 0 || h <= 0)
        return nullptr;
    const VkDeviceSize bytes = (VkDeviceSize)w * h * 4;

    // staging buffer (host visible)
    VkBuffer stage = VK_NULL_HANDLE;
    void* stageAlloc = nullptr;
    VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bi.size = bytes;
    bi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    void* map = nullptr;
    if (not FF_VmaBufferCreate(&bi, true, &stage, &stageAlloc, &map) or not map)
        return nullptr;
    memcpy(map, rgba, (size_t)bytes);
    FF_VmaFlush(stageAlloc, 0, bytes);

    // device-local image
    VulkanTexture* tex = new VulkanTexture();
    tex->width = w;
    tex->height = h;
    VkImage img = VK_NULL_HANDLE;
    void* imgAlloc = nullptr;
    VkImageView view = VK_NULL_HANDLE;
    VkImageCreateInfo ii{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ii.imageType = VK_IMAGE_TYPE_2D;
    ii.format = VK_FORMAT_R8G8B8A8_UNORM;
    ii.extent = {(uint32_t)w, (uint32_t)h, 1};
    ii.mipLevels = 1;
    ii.arrayLayers = 1;
    ii.samples = VK_SAMPLE_COUNT_1_BIT;
    ii.tiling = VK_IMAGE_TILING_OPTIMAL;
    ii.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (not FF_VmaImageCreate(&ii, &img, &imgAlloc))
    {
        delete tex;
        return nullptr;
    }

    // upload: UNDEFINED -> TRANSFER_DST -> copy -> SHADER_READ
    VkCommandBuffer cb;
    if (m->Submit(cb))
    {
        auto barrier = [&](VkImageLayout oldL, VkImageLayout newL,
                           VkAccessFlags sa, VkAccessFlags da,
                           VkPipelineStageFlags ss, VkPipelineStageFlags ds)
        {
            VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            b.oldLayout = oldL;
            b.newLayout = newL;
            b.srcQueueFamilyIndex = b.dstQueueFamilyIndex =
                VK_QUEUE_FAMILY_IGNORED;
            b.image = img;
            b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            b.srcAccessMask = sa;
            b.dstAccessMask = da;
            vkCmdPipelineBarrier(cb, ss, ds, 0, 0, nullptr, 0, nullptr, 1, &b);
        };
        barrier(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                0, VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT);
        VkBufferImageCopy cp{};
        cp.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        cp.imageExtent = {(uint32_t)w, (uint32_t)h, 1};
        vkCmdCopyBufferToImage(cb, stage, img,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &cp);
        barrier(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        m->Flush(cb);
    }
    FF_VmaBufferDestroy(stage, stageAlloc);

    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vi.image = img;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = VK_FORMAT_R8G8B8A8_UNORM;
    vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCreateImageView(m->device, &vi, nullptr, &view);

    tex->image = (uint64_t)img;
    tex->memory = (uint64_t)imgAlloc; // VMA allocation handle
    tex->view = (uint64_t)view;
    return tex;
}

VulkanTexture* VulkanTextureManager::CreateBC(const void* data, unsigned bytes,
                                              int w, int h, int bcType)
{
    if (!m->device || !data || bytes == 0 || w <= 0 || h <= 0)
        return nullptr;
    VkFormat fmt = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    if (bcType == 2)
        fmt = VK_FORMAT_BC2_UNORM_BLOCK;
    else if (bcType == 3)
        fmt = VK_FORMAT_BC3_UNORM_BLOCK;

    // Guard: only proceed if the device actually samples this BC format (real GPUs do; a software ICD like
    // lavapipe may not -> return null instead of creating an unusable image).
    VkPhysicalDevice phys =
        (VkPhysicalDevice)m->backend->VkPhysicalDeviceHandle();
    VkFormatProperties fp{};
    vkGetPhysicalDeviceFormatProperties(phys, fmt, &fp);
    if (!(fp.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
    {
        fprintf(
            stderr,
            "[Vulkan] CreateBC: BC format %d not supported by this device\n",
            (int)fmt);
        return nullptr;
    }

    // staging buffer holds the whole block stream
    VkBuffer stage = VK_NULL_HANDLE;
    void* stageAlloc = nullptr;
    VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bi.size = bytes;
    bi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    void* map = nullptr;
    if (not FF_VmaBufferCreate(&bi, true, &stage, &stageAlloc, &map) or not map)
        return nullptr;
    memcpy(map, data, (size_t)bytes);
    FF_VmaFlush(stageAlloc, 0, bytes);

    VulkanTexture* tex = new VulkanTexture();
    tex->width = w;
    tex->height = h;
    VkImage img = VK_NULL_HANDLE;
    void* imgAlloc = nullptr;
    VkImageView view = VK_NULL_HANDLE;
    VkImageCreateInfo ii{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ii.imageType = VK_IMAGE_TYPE_2D;
    ii.format = fmt;
    ii.extent = {(uint32_t)w, (uint32_t)h, 1};
    ii.mipLevels = 1;
    ii.arrayLayers = 1;
    ii.samples = VK_SAMPLE_COUNT_1_BIT;
    ii.tiling = VK_IMAGE_TILING_OPTIMAL;
    ii.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (not FF_VmaImageCreate(&ii, &img, &imgAlloc))
    {
        delete tex;
        return nullptr;
    }

    VkCommandBuffer cb;
    if (m->Submit(cb))
    {
        auto barrier = [&](VkImageLayout oldL, VkImageLayout newL,
                           VkAccessFlags sa, VkAccessFlags da,
                           VkPipelineStageFlags ss, VkPipelineStageFlags ds)
        {
            VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            b.oldLayout = oldL;
            b.newLayout = newL;
            b.srcQueueFamilyIndex = b.dstQueueFamilyIndex =
                VK_QUEUE_FAMILY_IGNORED;
            b.image = img;
            b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            b.srcAccessMask = sa;
            b.dstAccessMask = da;
            vkCmdPipelineBarrier(cb, ss, ds, 0, 0, nullptr, 0, nullptr, 1, &b);
        };
        barrier(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                0, VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT);
        VkBufferImageCopy cp{};
        cp.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        cp.imageExtent = {(uint32_t)w, (uint32_t)h,
                          1}; // texels; the block size comes from the BC format
        vkCmdCopyBufferToImage(cb, stage, img,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &cp);
        barrier(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        m->Flush(cb);
    }
    FF_VmaBufferDestroy(stage, stageAlloc);

    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vi.image = img;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = fmt;
    vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCreateImageView(m->device, &vi, nullptr, &view);
    tex->image = (uint64_t)img;
    tex->memory = (uint64_t)imgAlloc; // VMA allocation handle
    tex->view = (uint64_t)view;
    return tex;
}

VulkanTexture* VulkanTextureManager::White()
{
    if (m->white)
        return m->white;
    const uint32_t px = 0xFFFFFFFFu; // opaque white RGBA
    m->white = CreateRGBA(&px, 1, 1);
    return m->white;
}

bool VulkanTextureManager::IsValid() const
{
    return m->device != VK_NULL_HANDLE;
}

VulkanTexture* VulkanTextureManager::CreateFromDxgi(const void* data,
                                                    unsigned bytes, int w,
                                                    int h, int dxgiFormat)
{
    if (!m->device || !data || w <= 0 || h <= 0)
        return nullptr;
    // Block-compressed (DXT) DXGI formats -> the tested block uploader. DXGI: 70/71 BC1, 73/74 BC2, 76/77 BC3
    // (both the typeless and _UNORM values map to the same Vulkan block format).
    switch (dxgiFormat)
    {
    case 70:
    case 71:
        return CreateBC(data, bytes, w, h, 1); // BC1 / DXT1
    case 73:
    case 74:
        return CreateBC(data, bytes, w, h, 2); // BC2 / DXT3
    case 76:
    case 77:
        return CreateBC(data, bytes, w, h, 3); // BC3 / DXT5
    default:
        break;
    }
    // Uncompressed: map the DXGI value to a VkFormat + bytes-per-pixel. (28 R8G8B8A8, 87 B8G8R8A8, 85 B5G6R5.)
    VkFormat fmt;
    int bpp;
    switch (dxgiFormat)
    {
    case 28:
        fmt = VK_FORMAT_R8G8B8A8_UNORM;
        bpp = 4;
        break;
    case 87:
        fmt = VK_FORMAT_B8G8R8A8_UNORM;
        bpp = 4;
        break;
    case 85:
        fmt = VK_FORMAT_R5G6B5_UNORM_PACK16;
        bpp = 2;
        break; // #104: DXGI B5G6R5 order; closest Vk packed
    default:
        fmt = VK_FORMAT_R8G8B8A8_UNORM;
        bpp = 4;
        break; // fallback: treat as RGBA8
    }
    return m->Upload2D(data, w, h, fmt, bpp);
}

VulkanTexture* VulkanTextureManager::CreateRenderTarget(int w, int h)
{
    if (!m->device || w <= 0 || h <= 0)
        return nullptr;
    // Artscout - 2026 (#104): the SWAPCHAIN format, not a hardcoded R8G8B8A8. Three things must agree or the displays
    // silently vanish: this image, the RTT render pass (VulkanBackend, which uses scFormat), and the screen pipelines
    // that draw the symbology (built against the swapchain render pass -- a pipeline is only bindable inside a
    // COMPATIBLE pass, i.e. same colour format). It was R8G8B8A8 while the other two were B8G8R8A8, so
    // vkCreateFramebuffer failed the format-match rule, the RTT pass never began, and the command buffer went invalid.
    // Sampling is unaffected by the choice: the format defines the swizzle, so the shader reads rgba either way.
    const VkFormat fmt = m->backend ?
                             (VkFormat)m->backend->SwapchainColorFormatVk() :
                             VK_FORMAT_B8G8R8A8_UNORM;

    VulkanTexture* tex = new VulkanTexture();
    tex->width = w;
    tex->height = h;
    VkImage img = VK_NULL_HANDLE;
    void* imgAlloc = nullptr;
    VkImageView view = VK_NULL_HANDLE;
    VkImageCreateInfo ii{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ii.imageType = VK_IMAGE_TYPE_2D;
    ii.format = fmt;
    ii.extent = {(uint32_t)w, (uint32_t)h, 1};
    ii.mipLevels = 1;
    ii.arrayLayers = 1;
    ii.samples = VK_SAMPLE_COUNT_1_BIT;
    ii.tiling = VK_IMAGE_TILING_OPTIMAL;
    // RTT: rendered into (COLOR_ATTACHMENT), then sampled by the panel quad (SAMPLED); TRANSFER_SRC for readback.
    // TRANSFER_DST too: the GM radar panel textures are the DESTINATION of a sweep-snapshot blit (CopyRtt).
    ii.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
               VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
               VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (not FF_VmaImageCreate(&ii, &img, &imgAlloc))
    {
        delete tex;
        return nullptr;
    }

    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vi.image = img;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = fmt;
    vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCreateImageView(m->device, &vi, nullptr, &view);
    tex->image = (uint64_t)img;
    tex->memory = (uint64_t)imgAlloc; // VMA allocation handle
    tex->view = (uint64_t)view;
    return tex;
}

void VulkanTextureManager::Destroy(VulkanTexture* tex)
{
    if (!tex)
        return;
    if (!m->device)
    {
        delete tex;
        return;
    }
    // Defer: an in-flight command buffer may still reference this image view. Freeing it now is the freeze. The
    // renderer's TickRetire frees it kRetireFrames later, once no in-flight frame can still be using it.
    std::lock_guard<std::mutex> lk(m->retireMutex);
    m->retired.push_back({tex, Impl::kRetireFrames});
}

void VulkanTextureManager::BindDescriptorPool(uint64_t descriptorPool)
{
    m->descSetPool = (VkDescriptorPool)descriptorPool;
}

void VulkanTextureManager::TickRetire()
{
    if (!m->device)
        return;
    std::lock_guard<std::mutex> lk(m->retireMutex);
    for (size_t i = 0; i < m->retired.size();)
    {
        if (--m->retired[i].framesLeft <= 0)
        {
            m->FreeNow(m->retired[i].tex);
            m->retired[i] = m->retired.back();
            m->retired.pop_back();
        }
        else
            ++i;
    }
}
