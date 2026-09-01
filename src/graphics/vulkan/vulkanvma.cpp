//-----------------------------------------------------------------------------
// vulkanvma.cpp -- Artscout - 2026: the single VMA implementation TU + the
// thin wrappers from vulkanvma.h. See the header for the design notes.
//-----------------------------------------------------------------------------
#include <ciso646> // not/and/or tokens under MSVC
#include "vulkanvma.h"

// The engine links the Vulkan loader statically on both platforms, so VMA can
// call the functions directly instead of fetching pointers at runtime.
#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4100 4127 4189 4324 4505)
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-private-field"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wnullability-completeness"
#endif
#include "extlibs/vma/vk_mem_alloc.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__clang__)
#pragma clang diagnostic pop
#endif

#include <cstdio>

static VmaAllocator s_vma = VK_NULL_HANDLE;

bool FF_VmaInit(VkInstance instance, VkPhysicalDevice phys, VkDevice device,
                unsigned apiVersion)
{
    if (s_vma)
        return true; // already up (a second device init reuses the process allocator only after shutdown)

    VmaAllocatorCreateInfo ci = {};
    ci.instance = instance;
    ci.physicalDevice = phys;
    ci.device = device;
    ci.vulkanApiVersion = apiVersion;
    if (vmaCreateAllocator(&ci, &s_vma) != VK_SUCCESS)
    {
        fprintf(stderr, "[VMA] vmaCreateAllocator FAILED\n");
        s_vma = VK_NULL_HANDLE;
        return false;
    }
    return true;
}

void FF_VmaShutdown(void)
{
    if (not s_vma)
        return;
    FF_VmaLogStats("shutdown");
    {
        // TEMP leak diag: if anything is still alive, dump the detailed JSON so the leaked
        // allocation's size/type identifies the owner. Remove once shutdown is clean.
        VmaTotalStatistics st;
        vmaCalculateStatistics(s_vma, &st);
        if (st.total.statistics.allocationCount > 0)
        {
            char* json = nullptr;
            vmaBuildStatsString(s_vma, &json, VK_TRUE);
            if (json)
            {
                fprintf(stderr, "[VMA-LEAK] %s\n", json);
                vmaFreeStatsString(s_vma, json);
            }
        }
    }
    vmaDestroyAllocator(s_vma);
    s_vma = VK_NULL_HANDLE;
}

bool FF_VmaReady(void)
{
    return s_vma != VK_NULL_HANDLE;
}

bool FF_VmaImageCreate(const VkImageCreateInfo* ii, VkImage* outImage,
                       void** outAlloc)
{
    *outImage = VK_NULL_HANDLE;
    *outAlloc = nullptr;
    if (not s_vma)
        return false;

    VmaAllocationCreateInfo ai = {};
    ai.usage = VMA_MEMORY_USAGE_AUTO;

    VmaAllocation alloc = VK_NULL_HANDLE;
    if (vmaCreateImage(s_vma, ii, &ai, outImage, &alloc, nullptr) !=
        VK_SUCCESS)
        return false;
    *outAlloc = (void*)alloc;
    return true;
}

void FF_VmaImageDestroy(VkImage image, void* alloc)
{
    if (not s_vma or not image)
        return;
    vmaDestroyImage(s_vma, image, (VmaAllocation)alloc);
}

bool FF_VmaBufferCreate(const VkBufferCreateInfo* bi, bool hostMapped,
                        VkBuffer* outBuffer, void** outAlloc, void** outMapped)
{
    *outBuffer = VK_NULL_HANDLE;
    *outAlloc = nullptr;
    if (outMapped)
        *outMapped = nullptr;
    if (not s_vma)
        return false;

    VmaAllocationCreateInfo ai = {};
    ai.usage = VMA_MEMORY_USAGE_AUTO;
    if (hostMapped)
    {
        ai.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                   VMA_ALLOCATION_CREATE_MAPPED_BIT;
        // COHERENT required: the per-frame dynamic rings write through the persistent map with no
        // per-draw flush (the pre-VMA code demanded the same combination). Universally present on
        // desktop GPUs; FF_VmaFlush calls on such memory are no-ops.
        ai.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }

    VmaAllocation alloc = VK_NULL_HANDLE;
    VmaAllocationInfo info = {};
    if (vmaCreateBuffer(s_vma, bi, &ai, outBuffer, &alloc, &info) !=
        VK_SUCCESS)
        return false;
    *outAlloc = (void*)alloc;
    if (outMapped)
        *outMapped = info.pMappedData;
    return true;
}

void FF_VmaBufferDestroy(VkBuffer buffer, void* alloc)
{
    if (not s_vma or not buffer)
        return;
    vmaDestroyBuffer(s_vma, buffer, (VmaAllocation)alloc);
}

bool FF_VmaReadbackCreate(const VkBufferCreateInfo* bi, VkBuffer* outBuffer,
                          void** outAlloc, void** outMapped)
{
    *outBuffer = VK_NULL_HANDLE;
    *outAlloc = nullptr;
    *outMapped = nullptr;
    if (not s_vma)
        return false;

    VmaAllocationCreateInfo ai = {};
    ai.usage = VMA_MEMORY_USAGE_AUTO;
    ai.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
               VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocation alloc = VK_NULL_HANDLE;
    VmaAllocationInfo info = {};
    if (vmaCreateBuffer(s_vma, bi, &ai, outBuffer, &alloc, &info) !=
        VK_SUCCESS)
        return false;
    *outAlloc = (void*)alloc;
    *outMapped = info.pMappedData;
    return true;
}

// Host-visible LINEAR image, persistently mapped + COHERENT (the 2D-UI composite surface: the CPU
// rewrites it every frame and the shader samples it in GENERAL layout).
bool FF_VmaImageCreateHost(const VkImageCreateInfo* ii, VkImage* outImage,
                           void** outAlloc, void** outMapped)
{
    *outImage = VK_NULL_HANDLE;
    *outAlloc = nullptr;
    *outMapped = nullptr;
    if (not s_vma)
        return false;

    VmaAllocationCreateInfo ai = {};
    ai.usage = VMA_MEMORY_USAGE_AUTO;
    ai.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
               VMA_ALLOCATION_CREATE_MAPPED_BIT;
    ai.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    VmaAllocation alloc = VK_NULL_HANDLE;
    VmaAllocationInfo info = {};
    if (vmaCreateImage(s_vma, ii, &ai, outImage, &alloc, &info) != VK_SUCCESS)
        return false;
    *outAlloc = (void*)alloc;
    *outMapped = info.pMappedData;
    return true;
}

// ---- bind-style helpers (VulkanBackend): the image/buffer is created by the caller as before; only
// the memory behind it comes from VMA. Freed with FF_VmaFree AFTER the caller destroys the handle.
bool FF_VmaAllocImageMemory(VkImage image, void** outAlloc)
{
    *outAlloc = nullptr;
    if (not s_vma or not image)
        return false;
    // AUTO is vmaCreateImage-only; for bind-style allocation state the properties explicitly.
    VmaAllocationCreateInfo ai = {};
    ai.usage = VMA_MEMORY_USAGE_UNKNOWN;
    ai.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VmaAllocation alloc = VK_NULL_HANDLE;
    if (vmaAllocateMemoryForImage(s_vma, image, &ai, &alloc, nullptr) !=
        VK_SUCCESS)
        return false;
    if (vmaBindImageMemory(s_vma, alloc, image) != VK_SUCCESS)
    {
        vmaFreeMemory(s_vma, alloc);
        return false;
    }
    *outAlloc = (void*)alloc;
    return true;
}

bool FF_VmaAllocBufferMemory(VkBuffer buffer, bool hostMapped, void** outAlloc,
                             void** outMapped)
{
    *outAlloc = nullptr;
    if (outMapped)
        *outMapped = nullptr;
    if (not s_vma or not buffer)
        return false;
    // AUTO (and the HOST_ACCESS_* flags) are vmaCreateBuffer-only; bind-style states properties explicitly.
    VmaAllocationCreateInfo ai = {};
    ai.usage = VMA_MEMORY_USAGE_UNKNOWN;
    if (hostMapped)
    {
        ai.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        ai.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }
    else
        ai.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VmaAllocation alloc = VK_NULL_HANDLE;
    VmaAllocationInfo info = {};
    if (vmaAllocateMemoryForBuffer(s_vma, buffer, &ai, &alloc, &info) !=
        VK_SUCCESS)
        return false;
    if (vmaBindBufferMemory(s_vma, alloc, buffer) != VK_SUCCESS)
    {
        vmaFreeMemory(s_vma, alloc);
        return false;
    }
    *outAlloc = (void*)alloc;
    if (outMapped)
        *outMapped = info.pMappedData;
    return true;
}

// Readback variant: RANDOM host access (CPU reads), persistently mapped, COHERENT.
bool FF_VmaAllocReadbackMemory(VkBuffer buffer, void** outAlloc,
                               void** outMapped)
{
    *outAlloc = nullptr;
    *outMapped = nullptr;
    if (not s_vma or not buffer)
        return false;
    VmaAllocationCreateInfo ai = {};
    ai.usage = VMA_MEMORY_USAGE_UNKNOWN;
    ai.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    ai.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    ai.preferredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    VmaAllocation alloc = VK_NULL_HANDLE;
    VmaAllocationInfo info = {};
    if (vmaAllocateMemoryForBuffer(s_vma, buffer, &ai, &alloc, &info) !=
        VK_SUCCESS)
        return false;
    if (vmaBindBufferMemory(s_vma, alloc, buffer) != VK_SUCCESS)
    {
        vmaFreeMemory(s_vma, alloc);
        return false;
    }
    *outAlloc = (void*)alloc;
    *outMapped = info.pMappedData;
    return true;
}

// Host-visible LINEAR staging image (bind-style): persistently mapped + COHERENT.
bool FF_VmaAllocImageMemoryHost(VkImage image, void** outAlloc,
                                void** outMapped)
{
    *outAlloc = nullptr;
    *outMapped = nullptr;
    if (not s_vma or not image)
        return false;
    VmaAllocationCreateInfo ai = {};
    ai.usage = VMA_MEMORY_USAGE_UNKNOWN;
    ai.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    ai.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VmaAllocation alloc = VK_NULL_HANDLE;
    VmaAllocationInfo info = {};
    if (vmaAllocateMemoryForImage(s_vma, image, &ai, &alloc, &info) !=
        VK_SUCCESS)
        return false;
    if (vmaBindImageMemory(s_vma, alloc, image) != VK_SUCCESS)
    {
        vmaFreeMemory(s_vma, alloc);
        return false;
    }
    *outAlloc = (void*)alloc;
    *outMapped = info.pMappedData;
    return true;
}

void FF_VmaFree(void* alloc)
{
    if (not s_vma or not alloc)
        return;
    vmaFreeMemory(s_vma, (VmaAllocation)alloc);
}

void FF_VmaFlush(void* alloc, unsigned long long offset,
                 unsigned long long size)
{
    if (not s_vma or not alloc)
        return;
    vmaFlushAllocation(s_vma, (VmaAllocation)alloc, offset, size);
}

void FF_VmaLogStats(const char* tag)
{
    if (not s_vma)
        return;
    VmaTotalStatistics st;
    vmaCalculateStatistics(s_vma, &st);
    fprintf(stderr,
            "[VMA] %s: blocks=%u allocations=%u used=%.1fMB reserved=%.1fMB\n",
            tag ? tag : "", st.total.statistics.blockCount,
            st.total.statistics.allocationCount,
            st.total.statistics.allocationBytes / (1024.0 * 1024.0),
            st.total.statistics.blockBytes / (1024.0 * 1024.0));
}
