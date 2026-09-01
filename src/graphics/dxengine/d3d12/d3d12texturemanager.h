//-----------------------------------------------------------------------------
// D3D12TextureManager.h -- Artscout - 2026: #DX12 п.1. The D3D12 twin of
// D3D11TextureManager: given BCn/raw bytes, make a D3D12 texture + an SRV in a
// CPU (non-shader-visible) STAGING heap. The renderer copies that SRV into its
// per-frame shader-visible descriptor ring at draw time (CopyDescriptorsSimple).
//
// Textures load on the async LOADER thread, so uploads run on a DEDICATED direct
// queue + fence, serialized by a critical section (synchronous per-texture: create
// DEFAULT tex in COPY_DEST, fill an upload buffer with correctly re-pitched rows,
// CopyTextureRegion, barrier -> PIXEL_SHADER_RESOURCE, wait). Byte-identical DXT/BCn.
//-----------------------------------------------------------------------------
#ifndef _D3D12TEXTUREMANAGER_H_
#define _D3D12TEXTUREMANAGER_H_

#include <windows.h>
#include <vector>
#include <map>

// Artscout - 2026 (D3D11 purge): one source mip level for texture upload. Was in d3d11texturemanager.h;
// moved here (backend-agnostic) when the D3D11 texture manager was retired.
struct TexMipData
{
    const void* data;
    int rowPitch; // bytes per row (or per block-row for BCn)
};

struct ID3D12Device;
struct ID3D12CommandQueue;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;
struct ID3D12Fence;
struct ID3D12Resource;
struct ID3D12DescriptorHeap;
struct ID3D12Heap;

// A created D3D12 texture. TextureHandle::m_pDDS holds a POINTER to one of these
// (heap-allocated, persistent) under D3D12, reinterpreted by D3D12Renderer::SetTexture.
struct D3D12Texture
{
    ID3D12Resource* tex;
    unsigned __int64
        srvCpuPtr; // D3D12_CPU_DESCRIPTOR_HANDLE.ptr in the staging heap (copy source)
    int width, height, mips, dxgiFormat;
    int depth; // #13: >1 only for a TEXTURE3D (Create3D); 1 for every 2D texture

    // #DX12 п.3 RTT: for a render-target texture (FLAG_RENDERTARGET). rtvCpuPtr valid when isRT; rtState is
    // the CURRENT resource state (PIXEL_SHADER_RESOURCE at rest, RENDER_TARGET while displays draw into it).
    bool isRT;
    unsigned __int64 rtvCpuPtr;
    unsigned rtState; // D3D12_RESOURCE_STATES

    // #65 perf: pooled placed-resource region. poolHeapIdx >= 0 => `tex` is a PLACED resource living at
    // (poolHeapIdx, poolOffset) of size poolSize in the manager's shared texture heaps; Destroy returns that
    // region to the pool (frame-deferred). poolHeapIdx == -1 => `tex` is a standalone committed resource.
    int poolHeapIdx;
    unsigned __int64 poolOffset;
    unsigned __int64 poolSize;

    // Artscout - 2026: slot in the renderer's resident bindless heap, assigned
    // on first use. 0xFFFFFFFF = not registered. Peer of the Vulkan side's
    // VulkanTexture::bindlessSlot.
    unsigned bindlessSlot;

    D3D12Texture()
        : tex(0), srvCpuPtr(0), width(0), height(0), mips(0), dxgiFormat(0),
          depth(1), isRT(false), rtvCpuPtr(0), rtState(0), poolHeapIdx(-1),
          poolOffset(0), poolSize(0), bindlessSlot(0xFFFFFFFFu)
    {
    }
};

class D3D12TextureManager
{
public:
    D3D12TextureManager();
    ~D3D12TextureManager();

    bool
    Init(); // grabs device from g_pD3D12Backend, builds the upload queue + staging heap
    void Release();
    bool IsValid() const
    {
        return m_pDevice != 0;
    }

    // Same format mapping as D3D11 (identical DXGI values).
    static int DxgiFormatFromMPR(unsigned long mprTexInfoFlags);
    static bool IsBlockCompressed(int dxgiFormat);
    static int BlockBytes(int dxgiFormat);
    // Artscout - 2026 (D3D11 purge): offline BCn .dds authoring via NVTT 3 (was D3D11TextureManager::SaveBCnDDS).
    // Backend-agnostic (pure NVTT); the terrain/fartex DDS cache uses it. Win32 fallback returns false.
    static bool SaveBCnDDS(const char* fileName, unsigned long mprTexInfoFlags,
                           const void* bgra, int width, int height);

    // Create an immutable texture from mip levels already in the target format. Fills out.
    bool Create(D3D12Texture& out, int width, int height, int dxgiFormat,
                const TexMipData* mips, int mipCount);
    // Single-mip BCn (DXT) blob with auto row pitch.
    bool CreateBCn(D3D12Texture& out, int width, int height, int dxgiFormat,
                   const void* blockData, int blockDataBytes);

    // #DX12 п.3 RTT: create a render-target texture (RGBA8, RTV + SRV). Rest state = PIXEL_SHADER_RESOURCE.
    bool CreateRenderTarget(D3D12Texture& out, int width, int height);

    // Artscout - 2026 (#13): volume texture (TEXTURE3D). Baked 3D noise for the volumetric clouds -- the
    // procedural fBm cost ~320 ALU per sample (4 octaves x 8 hashes) and was the reason a cloud fly-through
    // dropped to 7 fps; a volume fetch is one (cached) sample. mips[i] is tightly packed: rowPitch = w*bpp,
    // slices follow each other (slicePitch = rowPitch*h). Mip i halves w, h AND d (a 3D mip is not an array).
    bool Create3D(D3D12Texture& out, int width, int height, int depth,
                  int dxgiFormat, const TexMipData* mips, int mipCount);

    // #DX12 п.4: create an immutable DEFAULT-heap vertex buffer with initial data (uploaded via the manager's
    // serialized queue -- loader-thread safe). Final state = VERTEX_AND_CONSTANT_BUFFER. Caller owns/Releases it.
    struct ID3D12Resource* CreateVertexBufferGPU(const void* data,
                                                 unsigned bytes);

    void Destroy(D3D12Texture& t);

    // Artscout - 2026 (#65 perf): make the render queue GPU-wait for every upload submitted so far.
    // Called once per frame from backend Present/EndEyeFrame, BEFORE the frame's draws execute, so
    // async-uploaded textures/VBs referenced by this frame are guaranteed resident when the GPU reads them.
    // Replaces the old per-upload CPU WaitForSingleObject(INFINITE) that caused the 3D-entry hitch.
    void SyncRenderQueue(ID3D12CommandQueue* renderQueue);

    // Handle-alloc helpers (TextureHandle stores a persistent D3D12Texture*): allocate a zeroed
    // D3D12Texture on the heap, or free one. Create*/Destroy operate on the pointed-to struct.
    D3D12Texture* Alloc();
    void Free(D3D12Texture* h);

    unsigned SrvIncrement() const
    {
        return m_srvInc;
    }

private:
    bool UploadAndBarrier(ID3D12Resource* dst, bool placed,
                          const TexMipData* mips, int mipCount,
                          const void* footprints, const unsigned* numRows,
                          const unsigned __int64* rowSize,
                          unsigned __int64 total);

    // Artscout - 2026 (#65 perf): async upload helpers (m_cs held). BeginUploadList acquires a ring
    // allocator (waiting only for the (i-N)th submit, not the current one) and resets m_pList onto it;
    // EnsureUploadRing hands back that slot's REUSABLE upload buffer (grown if too small) -- the slot's
    // prior submit already completed (BeginUploadList waited), so it is free. EndUploadList closes+submits
    // WITHOUT a CPU wait. Correctness across the upload/render queues comes from SyncRenderQueue.
    int BeginUploadList(); // returns the ring slot index in use
    struct ID3D12Resource* EnsureUploadRing(
        int idx,
        unsigned __int64 size); // slot idx's upload buffer, grown to >= size
    void EndUploadList(int idx); // close + submit m_pList (no CPU wait)
    // #DX12: staging-SRV / RTV slot allocation with a FREE-LIST. Every texture (and every Reload!) used to bump
    // m_srvHead permanently -> the 8192-slot heap exhausted ("staging heap full") -> new textures/RTTs failed to
    // load -> objects invisible, RTT atlas blank. Free() now returns the slot; these reuse it. Locked by m_cs.
    int AllocSrvSlot(); // -1 if genuinely full (all live)
    int AllocRtvSlot();
    void ReclaimSlots(
        const D3D12Texture&
            t); // push t's SRV (+ RTV if isRT) slot indices back onto the free-lists

    // #65 perf: placed-resource texture pool. Suballocate a COPY_DEST texture from shared DEFAULT heaps (records
    // its region in out); falls back to a committed resource (out.poolHeapIdx = -1) for oversized textures or on
    // pool failure. DeferTextureFree (m_cs held) defers resource+region; TickFrame reclaims them after
    // reuse free-list after TEX_POOL_SAFE_FRAMES so the GPU is guaranteed done with the old texture.
    ID3D12Resource* AllocPlacedTexture(const void* resourceDesc,
                                       D3D12Texture& out);
    // Defer RELEASING the texture resource AND reclaiming its pooled region for TEX_POOL_SAFE_FRAMES frames.
    // Async uploads mean the texture's own upload copy (its command allocator) and any render that drew it may
    // still be in flight; releasing now trips the debug layer's VerifyNotInUse / frees GPU-live memory. heapIdx<0
    // = committed (res released, no region). res may be NULL (region-only, never happens now but harmless).
    // bindlessSlot: 0xFFFFFFFF = none; otherwise returned with the resource.
    void DeferTextureFree(ID3D12Resource* res, int heapIdx,
                          unsigned __int64 offset, unsigned __int64 size,
                          unsigned bindlessSlot = 0xFFFFFFFFu);

public:
    void TickFrame(
        unsigned
            renderEpoch); // called from backend BeginFrame: advance epoch + reclaim regions
private:
    ID3D12Device* m_pDevice; // borrowed from g_pD3D12Backend
    ID3D12CommandQueue* m_pQueue; // dedicated upload queue
    ID3D12GraphicsCommandList*
        m_pList; // single list; reset per submit onto a ring allocator
    ID3D12Fence* m_pFence;
    unsigned __int64 m_fenceVal;
    HANDLE m_fenceEvent;

    // Artscout - 2026 (#65 perf): async upload pipeline. The old path did submit + CPU
    // WaitForSingleObject(INFINITE) per texture/VB -> 4259 serialized GPU round-trips on 3D-entry
    // (~850ms hitch) + streaming micro-stutters. Now each upload submits WITHOUT a CPU wait; the render
    // queue instead does one GPU-side Wait(m_pFence) per frame (SyncRenderQueue, from backend Present)
    // before it draws, so every texture the frame references is resident. A ring of allocators lets up to
    // N uploads be in flight -- the (i-N)th allocator is reused only after its submit's fence completes
    // (bounded backpressure, not a per-object stall). Each slot also owns a REUSABLE upload buffer, so the
    // per-texture CreateCommittedResource(UPLOAD) is gone (only ~N buffers ever exist, grown to the max size).
    enum
    {
        UPLOAD_ALLOC_RING = 16
    };
    ID3D12CommandAllocator* m_pAllocRing[UPLOAD_ALLOC_RING];
    unsigned __int64 m_ringFenceVal
        [UPLOAD_ALLOC_RING]; // fence value signaled at each allocator's last use
    unsigned m_ringHead; // next allocator index (taken mod ring size)
    ID3D12Resource* m_pUploadRing
        [UPLOAD_ALLOC_RING]; // reusable staging buffer per slot (grown on demand)
    unsigned __int64 m_uploadRingCap
        [UPLOAD_ALLOC_RING]; // current byte capacity of each slot's buffer

    ID3D12DescriptorHeap*
        m_pSrvStaging; // CPU (non-shader-visible) SRV heap; copy source for the renderer
    unsigned m_srvHead; // bump index into the staging heap
    unsigned m_srvCount; // capacity
    unsigned m_srvInc; // descriptor increment size

    ID3D12DescriptorHeap*
        m_pRtvHeap; // #DX12 п.3 RTT: RTV descriptors for render-target textures
    unsigned m_rtvHead;
    unsigned m_rtvCount;
    unsigned m_rtvInc;

    std::vector<unsigned>
        m_srvFree; // #DX12: reclaimed staging-SRV slots (reused before bumping m_srvHead)
    std::vector<unsigned> m_rtvFree; // #DX12: reclaimed RTV slots

    // #65 perf: placed-resource texture pool (see AllocPlacedTexture). Textures are suballocated from shared
    // DEFAULT heaps instead of one committed heap each (CreateCommittedResource was the dominant upload CPU cost).
    // Regions are reused by EXACT byte size -- textures of identical dimensions/format match perfectly, so the
    // heavy reload/streaming churn recycles regions with zero fragmentation; new sizes bump a heap (grow on demand).
    enum
    {
        TEX_HEAP_BYTES = 64u << 20
    }; // 64 MB per pool heap
    enum
    {
        TEX_POOL_SAFE_FRAMES = 8
    }; // frames a freed region waits before reuse (>> swapchain depth)
    struct TexHeap
    {
        ID3D12Heap* heap;
        unsigned __int64 used;
    }; // bump watermark within the heap
    std::vector<TexHeap> m_texHeaps;
    struct FreeRegion
    {
        int heapIdx;
        unsigned __int64 offset;
    };
    std::map<unsigned __int64, std::vector<FreeRegion> >
        m_texFreeBySize; // size -> GPU-safe reusable regions
    struct PendingFree
    {
        ID3D12Resource* res;
        unsigned __int64 size;
        unsigned __int64 offset;
        int heapIdx;
        unsigned freeEpoch;
        // #107: the bindless slot rides along so it comes back only once the
        // GPU is done -- an in-flight frame must not read a recycled slot.
        unsigned bindlessSlot;
    };
    std::vector<PendingFree>
        m_texPendingFree; // resources/regions waiting out SAFE_FRAMES
    unsigned m_frameEpoch; // advanced by TickFrame

    CRITICAL_SECTION m_cs; // serializes uploads from the loader + main threads
    bool m_csInit;
};

extern D3D12TextureManager* g_pD3D12TextureManager;

#endif // _D3D12TEXTUREMANAGER_H_
