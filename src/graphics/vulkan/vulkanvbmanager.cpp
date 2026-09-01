//-----------------------------------------------------------------------------
// VulkanVbManager.cpp -- Artscout - 2026 (#104, Linux port -- subsystem 2).
//-----------------------------------------------------------------------------
#include <ciso646> // not/and/or tokens under MSVC
#include "vulkanvbmanager.h"
#include "vulkanvma.h"
#include "vulkanbackend.h"

#include <vulkan/vulkan.h>
#include <cstring>
#include <mutex>
#include <vector>
// #104: shared VkQueue lock (defined in VulkanBackend.cpp). VB uploads run on loader threads and submit to the SAME
// queue the render thread presents on -- every queue op must hold this or the driver deadlocks (concurrent queue use).
extern std::mutex g_vkQueueMutex;
// #104: guards THIS manager's private command pool against its own callers -- several loader threads can upload at
// once, and a command pool is externally synchronized (recording included, not just allocate/free).
static std::mutex g_vbUploadMutex;

struct VulkanVbManager::Impl
{
    VulkanBackend* backend = nullptr;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    VkCommandPool pool = VK_NULL_HANDLE;

    // #104: deferred destruction, the SAME reason the texture manager defers. A model's VB is freed the instant the
    // engine drops it (an aircraft explodes, an LOD swaps out while the camera turns) -- and that can land while the
    // render thread is STILL RECORDING the scene command buffer with this very buffer bound. A vkQueueWaitIdle does
    // NOT cover that: it drains already-submitted frames, but the buffer is bound into the frame being recorded right
    // now, not a submitted one. Freeing it there invalidates the recording command buffer and loses the device (the
    // "freeze in a turn / on an explosion" that survived the WaitIdle attempt). Defer past every frame instead.
    struct Retired
    {
        uint64_t buffer;
        uint64_t memory;
        int framesLeft;
    };
    std::vector<Retired> retired;
    std::mutex retireMutex;
    static const int kRetireFrames = 3; // 2 in flight + 1 being recorded
    void FreeNow(uint64_t buffer, uint64_t memory)   // memory = VMA allocation handle
    {
        if (buffer)
            FF_VmaBufferDestroy((VkBuffer)buffer, (void*)memory);
    }
};

// Artscout - 2026 (#104): active Vulkan per-model VB manager (peer of the VbD3D12 path).
VulkanVbManager* g_pVulkanVbManager = nullptr;

VulkanVbManager::VulkanVbManager(VulkanBackend* backend) : m(new Impl())
{
    m->backend = backend;
}
VulkanVbManager::~VulkanVbManager()
{
    Release();
    delete m;
    m = nullptr;
}

bool VulkanVbManager::IsValid() const
{
    return m->device != VK_NULL_HANDLE;
}

bool VulkanVbManager::Init()
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

// Buffers are owned per-model and freed via Destroy by the loader; the private command pool is ours to drop.
void VulkanVbManager::Release()
{
    // Free every deferred buffer now -- Release runs after the device is idle, and no TickRetire will follow.
    if (m->device)
    {
        std::lock_guard<std::mutex> lk(m->retireMutex);
        for (auto& r : m->retired)
            m->FreeNow(r.buffer, r.memory);
        m->retired.clear();
    }
    if (m->device && m->pool)
    {
        vkDestroyCommandPool(m->device, m->pool, nullptr);
        m->pool = VK_NULL_HANDLE;
    }
}

VulkanVb* VulkanVbManager::Create(const void* data, unsigned bytes)
{
    if (!m->device || !data || bytes == 0)
        return nullptr;

    // staging (host visible)
    VkBuffer stage = VK_NULL_HANDLE;
    void* stageAlloc = nullptr;
    VkBufferCreateInfo sbi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    sbi.size = bytes;
    sbi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    sbi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    void* map = nullptr;
    if (not FF_VmaBufferCreate(&sbi, true, &stage, &stageAlloc, &map) or
        not map)
        return nullptr;
    memcpy(map, data, bytes);
    FF_VmaFlush(stageAlloc, 0, bytes);

    // device-local vertex buffer
    VulkanVb* vb = new VulkanVb();
    vb->bytes = bytes;
    VkBuffer buf = VK_NULL_HANDLE;
    void* bufAlloc = nullptr;
    VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bi.size = bytes;
    bi.usage =
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (not FF_VmaBufferCreate(&bi, false, &buf, &bufAlloc, nullptr))
    {
        FF_VmaBufferDestroy(stage, stageAlloc);
        delete vb;
        return nullptr;
    }

    // copy staging -> device-local (one-time submit). The lock spans allocate..free: all of it touches m->pool.
    std::lock_guard<std::mutex> _ulk(g_vbUploadMutex);
    VkCommandBufferAllocateInfo cai{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cai.commandPool = m->pool;
    cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cai.commandBufferCount = 1;
    VkCommandBuffer c = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(m->device, &cai, &c);
    VkCommandBufferBeginInfo cbi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    cbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(c, &cbi);
    VkBufferCopy cp{};
    cp.size = bytes;
    vkCmdCopyBuffer(c, stage, buf, 1, &cp);
    vkEndCommandBuffer(c);
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &c;
    {
        std::lock_guard<std::mutex> _qlk(
            g_vkQueueMutex); // serialize queue use with the render thread's submit/present
        vkQueueSubmit(m->queue, 1, &si, VK_NULL_HANDLE);
        vkQueueWaitIdle(m->queue);
    }
    vkFreeCommandBuffers(m->device, m->pool, 1, &c);
    FF_VmaBufferDestroy(stage, stageAlloc);

    vb->buffer = (uint64_t)buf;
    vb->memory = (uint64_t)bufAlloc; // VMA allocation handle
    // CPU shadow for the RTT non-indexed expansion (see VulkanVb::shadow).
    vb->shadow.assign((const unsigned char*)data,
                      (const unsigned char*)data + bytes);
    return vb;
}

void VulkanVbManager::Destroy(VulkanVb* vb)
{
    if (!vb)
        return;
    if (!m->device)
    {
        delete vb;
        return;
    }
    // Defer the buffer free past every in-flight AND currently-recording frame (see the note on Impl::retired).
    // The VulkanVb wrapper itself is done with now -- only the GPU handles need to outlive the frame.
    {
        std::lock_guard<std::mutex> lk(m->retireMutex);
        m->retired.push_back({vb->buffer, vb->memory, Impl::kRetireFrames});
    }
    delete vb;
}

void VulkanVbManager::TickRetire()
{
    if (!m->device)
        return;
    std::lock_guard<std::mutex> lk(m->retireMutex);
    for (size_t i = 0; i < m->retired.size();)
    {
        if (--m->retired[i].framesLeft <= 0)
        {
            m->FreeNow(m->retired[i].buffer, m->retired[i].memory);
            m->retired[i] = m->retired.back();
            m->retired.pop_back();
        }
        else
            ++i;
    }
}
