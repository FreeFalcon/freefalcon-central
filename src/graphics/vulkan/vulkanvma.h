//-----------------------------------------------------------------------------
// vulkanvma.h -- Artscout - 2026: central GPU memory allocator (AMD VMA).
//
// Every VkImage/VkBuffer in the Vulkan backend allocates through these helpers
// instead of raw vkAllocateMemory. One vkAllocateMemory per resource does not
// scale (drivers cap the total allocation count at 4096, and the terrain
// streams thousands of tile textures per sortie); VMA sub-allocates from big
// blocks, is thread-safe (loader threads upload textures concurrently), and
// handles dedicated allocations for large render targets automatically.
//
// The allocation handle is an opaque void* (a VmaAllocation) so this header
// leaks no VMA types into the rest of the tree. It travels wherever the old
// VkDeviceMemory handle used to live (same 8-byte slot in VulkanTexture).
//-----------------------------------------------------------------------------
#ifndef FF_VULKAN_VMA_H
#define FF_VULKAN_VMA_H

#include <vulkan/vulkan.h>

// Create/destroy the process-wide allocator. Init right after vkCreateDevice,
// shutdown right before vkDestroyDevice. Init failure is fatal for the
// backend (there is no raw-allocation fallback path).
bool FF_VmaInit(VkInstance instance, VkPhysicalDevice phys, VkDevice device,
                unsigned apiVersion);
void FF_VmaShutdown(void);
bool FF_VmaReady(void);

// Device-local image. Returns the image and the opaque allocation handle.
bool FF_VmaImageCreate(const VkImageCreateInfo* ii, VkImage* outImage,
                       void** outAlloc);
void FF_VmaImageDestroy(VkImage image, void* alloc);

// Buffer. hostMapped=false -> device-local; hostMapped=true -> host-visible,
// persistently mapped (sequential-write) with the pointer in *outMapped.
bool FF_VmaBufferCreate(const VkBufferCreateInfo* bi, bool hostMapped,
                        VkBuffer* outBuffer, void** outAlloc,
                        void** outMapped);
void FF_VmaBufferDestroy(VkBuffer buffer, void* alloc);

// Readback buffer: host-visible, persistently mapped, RANDOM host access
// (the CPU reads it back), HOST_CACHED preferred.
bool FF_VmaReadbackCreate(const VkBufferCreateInfo* bi, VkBuffer* outBuffer,
                          void** outAlloc, void** outMapped);

// Host-visible LINEAR image, persistently mapped + COHERENT (the 2D-UI
// composite surface). Layout semantics unchanged from the raw path.
bool FF_VmaImageCreateHost(const VkImageCreateInfo* ii, VkImage* outImage,
                           void** outAlloc, void** outMapped);

// Bind-style helpers (VulkanBackend): the caller creates the VkImage/VkBuffer
// itself; these allocate + bind the memory behind it. The handle is destroyed
// by the caller as before, then the allocation freed with FF_VmaFree.
bool FF_VmaAllocImageMemory(VkImage image, void** outAlloc);
bool FF_VmaAllocBufferMemory(VkBuffer buffer, bool hostMapped, void** outAlloc,
                             void** outMapped);
bool FF_VmaAllocReadbackMemory(VkBuffer buffer, void** outAlloc,
                               void** outMapped);
bool FF_VmaAllocImageMemoryHost(VkImage image, void** outAlloc,
                                void** outMapped);
void FF_VmaFree(void* alloc);

// Flush a host-visible allocation after CPU writes (no-op on coherent memory).
void FF_VmaFlush(void* alloc, unsigned long long offset,
                 unsigned long long size);

// One-line usage statistics for diagnostics ([VMA] blocks/allocations/bytes).
void FF_VmaLogStats(const char* tag);

#endif // FF_VULKAN_VMA_H
