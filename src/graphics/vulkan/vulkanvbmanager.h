//-----------------------------------------------------------------------------
// VulkanVbManager.h -- Artscout - 2026 (#104, Linux port -- subsystem 2).
// Per-model vertex buffers for the object/BSP path (aircraft/cockpit/world models). The engine's model loader
// uploads each model's vertices once and holds an opaque handle; DrawObjectIndexed/DrawObjectStrip in IRenderer
// take that handle as `void* vbHandle`. On Vulkan the handle is a VulkanVb* (a device-local VkBuffer). Peer of the
// per-model VbD3D12 the Windows dxvbmanager creates.
//-----------------------------------------------------------------------------
#ifndef FF_VULKAN_VBMANAGER_H
#define FF_VULKAN_VBMANAGER_H

#include <cstdint>
#include <vector>

class VulkanBackend;

struct VulkanVb
{
    uint64_t buffer = 0; // VkBuffer
    uint64_t memory = 0; // VkDeviceMemory
    uint32_t bytes = 0;
    // Artscout - 2026 (sensor video): CPU copy of the vertex data. The display-RTT pass silently drops
    // vkCmdDrawIndexed (same backend quirk the screen path works around in DrawTLIndexed), so an object
    // drawn into a sensor MFD is expanded to a non-indexed run -- which needs the vertices CPU-side.
    // Costs one RAM copy of every model VB; drop once the indexed-into-RTT root cause is fixed.
    std::vector<unsigned char> shadow;
};

class VulkanVbManager
{
public:
    explicit VulkanVbManager(VulkanBackend* backend);
    ~VulkanVbManager();

    bool Init();
    void Release();

    // Upload immutable vertex data to a device-local buffer. Returns a handle (cast to/from void* vbHandle). Null on failure.
    VulkanVb* Create(const void* data, unsigned bytes);
    void Destroy(VulkanVb* vb);
    void
    TickRetire(); // renderer calls this once per real frame; frees buffers past every in-flight frame

    bool IsValid() const;

private:
    struct Impl;
    Impl* m;
};

// Artscout - 2026 (#104): active Vulkan per-model VB manager (peer of the VbD3D12 path). Set in devmgr when the
// Vulkan backend comes up; the engine's dxvbmanager creates per-model VBs through it. Null under D3D12.
extern VulkanVbManager* g_pVulkanVbManager;

#endif // FF_VULKAN_VBMANAGER_H
