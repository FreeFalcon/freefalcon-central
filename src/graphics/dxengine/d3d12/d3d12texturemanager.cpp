//-----------------------------------------------------------------------------
// D3D12TextureManager.cpp -- Artscout - 2026: #DX12 п.1 (see header).
//-----------------------------------------------------------------------------
#include <windows.h>
#include <d3d12.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "d3d12texturemanager.h"
#include "d3d12renderer.h" // #107: hand bindless slots back on Destroy
#include "graphics/dxengine/d3d12backend.h"
#include "context.h"   // MPR_TI_* flags

#pragma comment(lib, "d3d12.lib")

D3D12TextureManager* g_pD3D12TextureManager = NULL;

static void T12Log(const char* fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    _vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);
    buf[sizeof(buf) - 1] = 0;
    OutputDebugStringA(buf);
}
#define T12_RELEASE(p)                                                         \
    do                                                                         \
    {                                                                          \
        if (p)                                                                 \
        {                                                                      \
            (p)->Release();                                                    \
            (p) = 0;                                                           \
        }                                                                      \
    } while (0)

// #DX12: a full campaign scene has FAR more than 8192 live textures (terrain + every object type + effects);
// the old 8192 exhausted mid-mission ("staging heap full") -> textures/RTTs failed -> objects invisible, RTT
// blank. This is a NON-shader-visible CPU heap (just system memory, ~32B/descriptor), so it can be huge. The
// free-list (Destroy->ReclaimSlots) still reuses slots so this ceiling is generous headroom, not a leak sink.
enum
{
    STAGING_SRV_COUNT = 262144
};

D3D12TextureManager::D3D12TextureManager()
    : m_pDevice(0), m_pQueue(0), m_pList(0), m_pFence(0), m_fenceVal(0),
      m_fenceEvent(0), m_ringHead(0), m_pSrvStaging(0), m_srvHead(0),
      m_srvCount(0), m_srvInc(0), m_pRtvHeap(0), m_rtvHead(0), m_rtvCount(0),
      m_rtvInc(0), m_frameEpoch(0), m_csInit(false)
{
    for (int i = 0; i < UPLOAD_ALLOC_RING; ++i)
    {
        m_pAllocRing[i] = 0;
        m_ringFenceVal[i] = 0;
        m_pUploadRing[i] = 0;
        m_uploadRingCap[i] = 0;
    }
}
D3D12TextureManager::~D3D12TextureManager()
{
    Release();
}

bool D3D12TextureManager::Init()
{
    if (!g_pD3D12Backend || !g_pD3D12Backend->IsValid())
    {
        T12Log("[D3D12Tex] no backend\n");
        return false;
    }
    m_pDevice = g_pD3D12Backend->GetDevice();
    if (!m_pDevice)
        return false;

    D3D12_COMMAND_QUEUE_DESC qd;
    ZeroMemory(&qd, sizeof(qd));
    qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    if (FAILED(m_pDevice->CreateCommandQueue(&qd, IID_PPV_ARGS(&m_pQueue))))
    {
        T12Log("[D3D12Tex] queue failed\n");
        return false;
    }
    for (int i = 0; i < UPLOAD_ALLOC_RING; ++i)
        if (FAILED(m_pDevice->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&m_pAllocRing[i]))))
            return false;
    if (FAILED(m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                            m_pAllocRing[0], NULL,
                                            IID_PPV_ARGS(&m_pList))))
        return false;
    m_pList->Close();
    m_ringHead = 0;
    for (int i = 0; i < UPLOAD_ALLOC_RING; ++i)
    {
        m_ringFenceVal[i] = 0;
        m_pUploadRing[i] = 0;
        m_uploadRingCap[i] = 0;
    }
    if (FAILED(m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                      IID_PPV_ARGS(&m_pFence))))
        return false;
    m_fenceVal = 0;
    m_fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!m_fenceEvent)
        return false;

    D3D12_DESCRIPTOR_HEAP_DESC hd;
    ZeroMemory(&hd, sizeof(hd));
    hd.NumDescriptors = STAGING_SRV_COUNT;
    hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    hd.Flags =
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // CPU staging (copy source for the renderer's shader-visible ring)
    if (FAILED(
            m_pDevice->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&m_pSrvStaging))))
    {
        T12Log("[D3D12Tex] staging heap failed\n");
        return false;
    }
    m_srvCount = STAGING_SRV_COUNT;
    m_srvHead = 0;
    m_srvInc = m_pDevice->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // #DX12 п.3 RTT: a small RTV heap for render-target textures (the display atlas etc.).
    D3D12_DESCRIPTOR_HEAP_DESC rd;
    ZeroMemory(&rd, sizeof(rd));
    rd.NumDescriptors = 256;
    rd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (FAILED(m_pDevice->CreateDescriptorHeap(&rd, IID_PPV_ARGS(&m_pRtvHeap))))
    {
        T12Log("[D3D12Tex] RTV heap failed\n");
        return false;
    }
    m_rtvCount = 256;
    m_rtvHead = 0;
    m_rtvInc = m_pDevice->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    InitializeCriticalSection(&m_cs);
    m_csInit = true;
    T12Log("D3D12TextureManager: up (staging heap %u)\n",
           (unsigned)STAGING_SRV_COUNT);
    return true;
}

void D3D12TextureManager::Release()
{
    if (m_pQueue && m_pFence) // drain any in-flight upload
    {
        m_fenceVal++;
        m_pQueue->Signal(m_pFence, m_fenceVal);
        if (m_pFence->GetCompletedValue() < m_fenceVal)
        {
            m_pFence->SetEventOnCompletion(m_fenceVal, m_fenceEvent);
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
    }
    if (m_fenceEvent)
    {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = 0;
    }
    T12_RELEASE(m_pSrvStaging);
    T12_RELEASE(m_pRtvHeap);
    T12_RELEASE(m_pFence);
    T12_RELEASE(m_pList);
    // #65 perf: reusable per-slot upload buffers -- all uploads drained above, so they are idle; release them.
    for (int i = 0; i < UPLOAD_ALLOC_RING; ++i)
    {
        T12_RELEASE(m_pUploadRing[i]);
        m_uploadRingCap[i] = 0;
    }
    for (int i = 0; i < UPLOAD_ALLOC_RING; ++i)
        T12_RELEASE(m_pAllocRing[i]);
    // #65 perf: release deferred texture resources BEFORE their heaps (placed resources must outlive-release into a
    // live heap). GPU already drained above. Then the pool heaps themselves.
    for (size_t i = 0; i < m_texPendingFree.size(); ++i)
        if (m_texPendingFree[i].res)
            m_texPendingFree[i].res->Release();
    m_texPendingFree.clear();
    m_texFreeBySize.clear();
    for (size_t i = 0; i < m_texHeaps.size(); ++i)
        T12_RELEASE(m_texHeaps[i].heap);
    m_texHeaps.clear();
    T12_RELEASE(m_pQueue);
    m_pDevice = 0;
    if (m_csInit)
    {
        DeleteCriticalSection(&m_cs);
        m_csInit = false;
    }
}

//================================ format mapping =============================
int D3D12TextureManager::DxgiFormatFromMPR(unsigned long f)
{
    if (f & MPR_TI_DXT1)
        return DXGI_FORMAT_BC1_UNORM;
    if (f & MPR_TI_DXT3)
        return DXGI_FORMAT_BC2_UNORM;
    if (f & MPR_TI_DXT5)
        return DXGI_FORMAT_BC3_UNORM;
    if (f & MPR_TI_ARGB32)
        return DXGI_FORMAT_B8G8R8A8_UNORM;
    if (f & MPR_TI_RGB24)
        return DXGI_FORMAT_B8G8R8A8_UNORM;
    if (f & MPR_TI_RGB16)
        return DXGI_FORMAT_B5G6R5_UNORM;
    return DXGI_FORMAT_B8G8R8A8_UNORM;
}
bool D3D12TextureManager::IsBlockCompressed(int fmt)
{
    return fmt == DXGI_FORMAT_BC1_UNORM || fmt == DXGI_FORMAT_BC2_UNORM ||
           fmt == DXGI_FORMAT_BC3_UNORM;
}
int D3D12TextureManager::BlockBytes(int fmt)
{
    return (fmt == DXGI_FORMAT_BC1_UNORM) ? 8 : 16;
}

//================================ async upload pipeline (locked) =============
// Artscout - 2026 (#65 perf): acquire the next ring allocator + reset m_pList onto it. We wait ONLY for the
// submit that last used this allocator (UPLOAD_ALLOC_RING submits ago) -- its command memory must be free
// before Reset. The CURRENT submit is never CPU-waited; that is the whole point. Allows N uploads in flight.
int D3D12TextureManager::BeginUploadList()
{
    int idx = (int)(m_ringHead % UPLOAD_ALLOC_RING);
    if (m_pFence->GetCompletedValue() < m_ringFenceVal[idx])
    {
        m_pFence->SetEventOnCompletion(m_ringFenceVal[idx], m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
    m_pAllocRing[idx]->Reset();
    m_pList->Reset(m_pAllocRing[idx], NULL);
    return idx;
}

// Close + submit the recorded copies (NO CPU wait). The upload buffer stays owned by its ring slot and is
// reused UPLOAD_ALLOC_RING submits later (by then BeginUploadList has waited for this submit's fence).
void D3D12TextureManager::EndUploadList(int idx)
{
    m_pList->Close();
    ID3D12CommandList* lists[] = {(ID3D12CommandList*)m_pList};
    m_pQueue->ExecuteCommandLists(1, lists);
    m_fenceVal++;
    m_pQueue->Signal(m_pFence, m_fenceVal);
    m_ringFenceVal[idx] = m_fenceVal;
    m_ringHead++;
}

// Return ring slot idx's reusable upload buffer, (re)creating it only if smaller than `size`. Releasing the old
// one here is safe: BeginUploadList already waited for slot idx's previous submit, so the GPU is done with it.
// Grows monotonically to the largest texture the slot ever staged, then stops reallocating. m_cs held.
ID3D12Resource* D3D12TextureManager::EnsureUploadRing(int idx,
                                                      unsigned __int64 size)
{
    if (size < 1)
        size = 1;
    if (m_pUploadRing[idx] && m_uploadRingCap[idx] >= size)
        return m_pUploadRing[idx];
    T12_RELEASE(m_pUploadRing[idx]);
    m_uploadRingCap[idx] = 0;
    D3D12_HEAP_PROPERTIES hpUp;
    ZeroMemory(&hpUp, sizeof(hpUp));
    hpUp.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bd.Width = size;
    bd.Height = 1;
    bd.DepthOrArraySize = 1;
    bd.MipLevels = 1;
    bd.Format = DXGI_FORMAT_UNKNOWN;
    bd.SampleDesc.Count = 1;
    bd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ID3D12Resource* buf = 0;
    {
        if (FAILED(m_pDevice->CreateCommittedResource(
                &hpUp, D3D12_HEAP_FLAG_NONE, &bd,
                D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&buf))))
            return 0;
    }
    m_pUploadRing[idx] = buf;
    m_uploadRingCap[idx] = size;
    return buf;
}

// #65 perf: make the render queue GPU-wait for every upload submitted so far, once per frame, before the
// frame's draws execute. Cross-queue correctness: m_pQueue signals m_pFence; renderQueue waits on the same
// fence. Any texture/VB the frame references was created (hence submitted, fence <= m_fenceVal) before the
// render list was recorded -> waiting for m_fenceVal guarantees it is resident when the GPU reads it.
void D3D12TextureManager::SyncRenderQueue(ID3D12CommandQueue* renderQueue)
{
    if (!renderQueue || !m_pFence)
        return;
    // Read m_fenceVal WITHOUT the lock: an aligned 64-bit read is atomic on x64 and the value only grows. A
    // slightly-stale (smaller) value is still safe -- any texture this frame's already-recorded render list
    // references was submitted (fence advanced) before Present. Avoids stalling the main thread on m_cs while
    // the loader holds it during a memcpy.
    unsigned __int64 v = m_fenceVal;
    if (v > 0)
        renderQueue->Wait(m_pFence, v);
}

bool D3D12TextureManager::UploadAndBarrier(ID3D12Resource* dst, bool placed,
                                           const TexMipData* mips, int mipCount,
                                           const void* footprints,
                                           const unsigned* numRows,
                                           const unsigned __int64* rowSize,
                                           unsigned __int64 total)
{
    const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* fp =
        (const D3D12_PLACED_SUBRESOURCE_FOOTPRINT*)footprints;
    EnterCriticalSection(&m_cs);
    int idx = BeginUploadList();
    ID3D12Resource* upload = EnsureUploadRing(idx, total);
    // On failure the list was already opened by BeginUploadList -- Close it (leaving it open would make the next
    // BeginUploadList's Reset fail). We DON'T advance m_ringHead/fence, so the same slot is reset+reused next time.
    if (!upload)
    {
        m_pList->Close();
        LeaveCriticalSection(&m_cs);
        return false;
    }

    // #65 perf: a placed texture may reuse heap memory a previous (now-freed) texture held. D3D12 requires an
    // aliasing barrier before its first use; without it the debug layer warns and this build BREAKs on warnings
    // (the same InfoQueue break that once hung the device). pResourceBefore=NULL is the conservative "any prior
    // resource" form. The following CopyTextureRegion fully initializes every subresource, so contents are defined.
    if (placed)
    {
        D3D12_RESOURCE_BARRIER ab;
        ZeroMemory(&ab, sizeof(ab));
        ab.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
        ab.Aliasing.pResourceBefore = NULL;
        ab.Aliasing.pResourceAfter = dst;
        m_pList->ResourceBarrier(1, &ab);
    }

    // Fill the slot's upload buffer, re-pitching each row (source is tightly packed; D3D12 wants 256-aligned rows).
    unsigned char* mapped = 0;
    D3D12_RANGE noRead;
    noRead.Begin = 0;
    noRead.End = 0;
    if (FAILED(upload->Map(0, &noRead, (void**)&mapped)) || !mapped)
    {
        m_pList->Close();
        LeaveCriticalSection(&m_cs);
        return false;
    }
    for (int i = 0; i < mipCount; ++i)
    {
        unsigned char* dstMip = mapped + fp[i].Offset;
        const unsigned char* srcMip = (const unsigned char*)mips[i].data;
        const UINT dstPitch = fp[i].Footprint.RowPitch;
        const UINT srcPitch = (UINT)mips[i].rowPitch;
        const UINT rows = numRows[i];
        UINT copyBytes =
            (srcPitch < (UINT)rowSize[i]) ? srcPitch : (UINT)rowSize[i];
        // Artscout - 2026 (#13): a 3D texture's subresource is Depth SLICES of `rows` rows each -- numRows is the
        // rows in ONE slice, so a rows-only loop would fill slice 0 and leave the rest of the volume UNDEFINED.
        // Slice stride is RowPitch*numRows on both sides (source is tightly packed). Depth == 1 for every 2D
        // texture, so this degenerates to the original single-slice loop -- no behaviour change for 2D.
        const UINT depth = fp[i].Footprint.Depth ? fp[i].Footprint.Depth : 1;
        const size_t dstSlice = (size_t)dstPitch * rows;
        const size_t srcSlice = (size_t)srcPitch * rows;
        for (UINT z = 0; z < depth; ++z)
            for (UINT r = 0; r < rows; ++r)
                memcpy(dstMip + z * dstSlice + (size_t)r * dstPitch,
                       srcMip + z * srcSlice + (size_t)r * srcPitch, copyBytes);
    }
    upload->Unmap(0, NULL);

    for (int i = 0; i < mipCount; ++i)
    {
        D3D12_TEXTURE_COPY_LOCATION dstL;
        ZeroMemory(&dstL, sizeof(dstL));
        dstL.pResource = dst;
        dstL.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstL.SubresourceIndex = (UINT)i;
        D3D12_TEXTURE_COPY_LOCATION srcL;
        ZeroMemory(&srcL, sizeof(srcL));
        srcL.pResource = upload;
        srcL.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcL.PlacedFootprint = fp[i];
        m_pList->CopyTextureRegion(&dstL, 0, 0, 0, &srcL, NULL);
    }
    D3D12_RESOURCE_BARRIER b;
    ZeroMemory(&b, sizeof(b));
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource = dst;
    b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    b.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    m_pList->ResourceBarrier(1, &b);
    EndUploadList(idx);
    LeaveCriticalSection(&m_cs);
    return true;
}

//================================ placed-resource texture pool ===============
// Suballocate a COPY_DEST texture from the shared DEFAULT heaps and record its region in `out` (out.poolHeapIdx>=0).
// Falls back to a standalone committed resource (out.poolHeapIdx=-1) for oversized textures or on pool failure.
// m_cs is taken only around the heap bookkeeping. Reused regions come from m_texFreeBySize, which TickFrame fills
// only after TEX_POOL_SAFE_FRAMES -> the GPU is guaranteed done with whatever texture last held the region.
ID3D12Resource*
D3D12TextureManager::AllocPlacedTexture(const void* resourceDesc,
                                        D3D12Texture& out)
{
    const D3D12_RESOURCE_DESC& td = *(const D3D12_RESOURCE_DESC*)resourceDesc;
    out.poolHeapIdx = -1;
    out.poolOffset = 0;
    out.poolSize = 0;
    D3D12_HEAP_PROPERTIES hpDef;
    ZeroMemory(&hpDef, sizeof(hpDef));
    hpDef.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_ALLOCATION_INFO ai =
        m_pDevice->GetResourceAllocationInfo(0, 1, &td);
    unsigned __int64 size = ai.SizeInBytes;
    unsigned __int64 align =
        ai.Alignment ? ai.Alignment : (unsigned __int64)(64u << 10);
    if (size == 0 || size == (unsigned __int64)-1 ||
        size > (unsigned __int64)TEX_HEAP_BYTES)
    {
        ID3D12Resource* tex = 0; // oversized / bad info -> committed
        if (FAILED(m_pDevice->CreateCommittedResource(
                &hpDef, D3D12_HEAP_FLAG_NONE, &td,
                D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&tex))))
            return 0;
        return tex;
    }

    EnterCriticalSection(&m_cs);
    int heapIdx = -1;
    unsigned __int64 offset = 0;
    std::map<unsigned __int64, std::vector<FreeRegion> >::iterator it =
        m_texFreeBySize.find(size);
    if (it != m_texFreeBySize.end() && !it->second.empty())
    {
        FreeRegion fr = it->second.back();
        it->second.pop_back();
        heapIdx = fr.heapIdx;
        offset = fr.offset;
    }
    else
    {
        for (size_t h = 0; h < m_texHeaps.size() && heapIdx < 0;
             ++h) // bump within an existing heap
        {
            unsigned __int64 base =
                (m_texHeaps[h].used + (align - 1)) & ~(align - 1);
            if (base + size <= (unsigned __int64)TEX_HEAP_BYTES)
            {
                heapIdx = (int)h;
                offset = base;
                m_texHeaps[h].used = base + size;
            }
        }
        if (heapIdx < 0) // grow: new heap
        {
            D3D12_HEAP_DESC hd;
            ZeroMemory(&hd, sizeof(hd));
            hd.SizeInBytes = (unsigned __int64)TEX_HEAP_BYTES;
            hd.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
            hd.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
            ID3D12Heap* heap = 0;
            if (SUCCEEDED(m_pDevice->CreateHeap(&hd, IID_PPV_ARGS(&heap))) &&
                heap)
            {
                TexHeap th;
                th.heap = heap;
                th.used = size;
                m_texHeaps.push_back(th);
                heapIdx = (int)m_texHeaps.size() - 1;
                offset = 0;
            }
        }
    }
    ID3D12Heap* heap = (heapIdx >= 0) ? m_texHeaps[heapIdx].heap : 0;
    LeaveCriticalSection(&m_cs);

    if (heap)
    {
        ID3D12Resource* tex = 0;
        if (SUCCEEDED(m_pDevice->CreatePlacedResource(
                heap, offset, &td, D3D12_RESOURCE_STATE_COPY_DEST, NULL,
                IID_PPV_ARGS(&tex))) &&
            tex)
        {
            out.poolHeapIdx = heapIdx;
            out.poolOffset = offset;
            out.poolSize = size;
            return tex;
        }
        EnterCriticalSection(
            &m_cs); // placed create failed -> return the region, fall back to committed
        FreeRegion fr;
        fr.heapIdx = heapIdx;
        fr.offset = offset;
        m_texFreeBySize[size].push_back(fr);
        LeaveCriticalSection(&m_cs);
    }

    ID3D12Resource* tex = 0;
    if (FAILED(m_pDevice->CreateCommittedResource(
            &hpDef, D3D12_HEAP_FLAG_NONE, &td, D3D12_RESOURCE_STATE_COPY_DEST,
            NULL, IID_PPV_ARGS(&tex))))
        return 0;
    return tex;
}

// Defer releasing a texture resource (and reclaiming its pooled region) by TEX_POOL_SAFE_FRAMES frames. Takes m_cs.
void D3D12TextureManager::DeferTextureFree(ID3D12Resource* res, int heapIdx,
                                           unsigned __int64 offset,
                                           unsigned __int64 size,
                                           unsigned bindlessSlot)
{
    if (!res && heapIdx < 0 && bindlessSlot == 0xFFFFFFFFu)
        return;
    if (m_csInit)
        EnterCriticalSection(&m_cs);
    PendingFree pf;
    pf.res = res;
    pf.size = size;
    pf.offset = offset;
    pf.heapIdx = heapIdx;
    pf.bindlessSlot = bindlessSlot;
    pf.freeEpoch = m_frameEpoch;
    m_texPendingFree.push_back(pf);
    if (m_csInit)
        LeaveCriticalSection(&m_cs);
}

// Called from backend BeginFrame. Advance the frame epoch and, for anything freed >= TEX_POOL_SAFE_FRAMES frames
// ago, RELEASE the resource and return its pooled region to the reuse free-list -- by then the GPU has finished
// the texture's async upload AND every frame that drew it (swapchain depth << SAFE_FRAMES, backend fences each frame).
void D3D12TextureManager::TickFrame(unsigned renderEpoch)
{
    if (!m_csInit)
    {
        m_frameEpoch = renderEpoch;
        return;
    }
    EnterCriticalSection(&m_cs);
    m_frameEpoch = renderEpoch;
    size_t w = 0;
    for (size_t i = 0; i < m_texPendingFree.size(); ++i)
    {
        PendingFree& pf = m_texPendingFree[i];
        if (renderEpoch - pf.freeEpoch >= (unsigned)TEX_POOL_SAFE_FRAMES)
        {
            if (pf.res)
                pf.res
                    ->Release(); // GPU done -> safe to release the resource object
            if (pf.heapIdx >= 0) // pooled -> region is now reusable
            {
                FreeRegion fr;
                fr.heapIdx = pf.heapIdx;
                fr.offset = pf.offset;
                m_texFreeBySize[pf.size].push_back(fr);
            }

            // #107: only now is the slot's descriptor no longer read by a frame
            // still in flight -- hand it back here, not at Destroy time.
            if (pf.bindlessSlot != 0xFFFFFFFFu && g_pD3D12Renderer)
                g_pD3D12Renderer->ReleaseBindlessSlot(pf.bindlessSlot);
        }
        else
            m_texPendingFree[w++] = pf;
    }
    m_texPendingFree.resize(w);
    LeaveCriticalSection(&m_cs);
}

void D3D12TexMgr_TickFrame(unsigned renderEpoch)
{
    if (g_pD3D12TextureManager)
        g_pD3D12TextureManager->TickFrame(renderEpoch);
}

//================================ creation ==================================
bool D3D12TextureManager::Create(D3D12Texture& out, int width, int height,
                                 int dxgiFormat, const TexMipData* mips,
                                 int mipCount)
{
    if (!m_pDevice || mipCount <= 0 || mipCount > 15 || width < 1 || height < 1)
        return false;

    // 1. DEFAULT-heap texture in COPY_DEST -- suballocated from a shared pool heap (placed resource) instead of a
    //    committed heap each; falls back to committed for oversized textures. Records out.poolHeapIdx/Offset/Size.
    D3D12_RESOURCE_DESC td;
    ZeroMemory(&td, sizeof(td));
    td.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    td.Width = (UINT64)width;
    td.Height = (UINT)height;
    td.DepthOrArraySize = 1;
    td.MipLevels = (UINT16)mipCount;
    td.Format = (DXGI_FORMAT)dxgiFormat;
    td.SampleDesc.Count = 1;
    td.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    ID3D12Resource* tex = AllocPlacedTexture(&td, out);
    if (!tex)
    {
        T12Log("[D3D12Tex] texture alloc failed %dx%d fmt=%d\n", width, height,
               dxgiFormat);
        return false;
    }

    // 2. copyable footprints.
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp[16];
    UINT numRows[16];
    UINT64 rowSize[16];
    UINT64 total = 0;
    m_pDevice->GetCopyableFootprints(&td, 0, (UINT)mipCount, 0, fp, numRows,
                                     rowSize, &total);

    // 3. fill a ring-owned upload buffer + copy + barrier (locked; loader-thread safe). Async: submits without a
    //    CPU wait; the upload buffer is REUSED per ring slot (no per-texture allocation), and the render queue
    //    GPU-waits for the copy in Present (SyncRenderQueue).
    if (!UploadAndBarrier(tex, out.poolHeapIdx >= 0, mips, mipCount, fp,
                          numRows, rowSize, total))
    {
        DeferTextureFree(tex, out.poolHeapIdx, out.poolOffset,
                         out.poolSize); // deferred (safe even w/o GPU work)
        out.poolHeapIdx = -1;
        out.poolOffset = 0;
        out.poolSize = 0;
        return false;
    }

    // 6. SRV in the staging heap (reuse a reclaimed slot before bumping -- fixes the reload leak).
    int srvSlot = AllocSrvSlot();
    if (srvSlot < 0)
    {
        T12Log("[D3D12Tex] staging heap full\n");
        tex->Release();
        return false;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE h =
        m_pSrvStaging->GetCPUDescriptorHandleForHeapStart();
    h.ptr += (SIZE_T)srvSlot * m_srvInc;
    D3D12_SHADER_RESOURCE_VIEW_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.Format = (DXGI_FORMAT)dxgiFormat;
    sd.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    sd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    sd.Texture2D.MipLevels = (UINT)mipCount;
    m_pDevice->CreateShaderResourceView(tex, &sd, h);

    out.tex = tex;
    out.srvCpuPtr = (unsigned __int64)h.ptr;
    out.width = width;
    out.height = height;
    out.mips = mipCount;
    out.dxgiFormat = dxgiFormat;
    return true;
}

// Artscout - 2026 (#13): volume texture. Same shape as Create() -- placed resource + copyable footprints +
// async upload -- but TEXTURE3D, where DepthOrArraySize is the DEPTH and every mip halves it too. The upload
// itself needed no new path: CopyTextureRegion already honours fp[i].Footprint.Depth; only the CPU-side memcpy
// had to learn about slices (see UploadAndBarrier).
bool D3D12TextureManager::Create3D(D3D12Texture& out, int width, int height,
                                   int depth, int dxgiFormat,
                                   const TexMipData* mips, int mipCount)
{
    if (!m_pDevice || mipCount <= 0 || mipCount > 15)
        return false;
    if (width < 1 || height < 1 || depth < 1)
        return false;
    if (width > 2048 || height > 2048 || depth > 2048)
        return false; // D3D12 TEXTURE3D dimension cap

    D3D12_RESOURCE_DESC td;
    ZeroMemory(&td, sizeof(td));
    td.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    td.Width = (UINT64)width;
    td.Height = (UINT)height;
    td.DepthOrArraySize = (UINT16)
        depth; // 3D: DEPTH (halves per mip -- unlike an array's slice count)
    td.MipLevels = (UINT16)mipCount;
    td.Format = (DXGI_FORMAT)dxgiFormat;
    td.SampleDesc.Count = 1;
    td.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    ID3D12Resource* tex = AllocPlacedTexture(&td, out);
    if (!tex)
    {
        T12Log("[D3D12Tex] 3D texture alloc failed %dx%dx%d fmt=%d\n", width,
               height, depth, dxgiFormat);
        return false;
    }

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp[16];
    UINT numRows[16];
    UINT64 rowSize[16];
    UINT64 total = 0;
    m_pDevice->GetCopyableFootprints(&td, 0, (UINT)mipCount, 0, fp, numRows,
                                     rowSize, &total);

    if (!UploadAndBarrier(tex, out.poolHeapIdx >= 0, mips, mipCount, fp,
                          numRows, rowSize, total))
    {
        DeferTextureFree(tex, out.poolHeapIdx, out.poolOffset, out.poolSize);
        out.poolHeapIdx = -1;
        out.poolOffset = 0;
        out.poolSize = 0;
        return false;
    }

    int srvSlot = AllocSrvSlot();
    if (srvSlot < 0)
    {
        T12Log("[D3D12Tex] staging heap full (3D)\n");
        tex->Release();
        return false;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE h =
        m_pSrvStaging->GetCPUDescriptorHandleForHeapStart();
    h.ptr += (SIZE_T)srvSlot * m_srvInc;
    D3D12_SHADER_RESOURCE_VIEW_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.Format = (DXGI_FORMAT)dxgiFormat;
    sd.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
    sd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    sd.Texture3D.MipLevels = (UINT)mipCount;
    m_pDevice->CreateShaderResourceView(tex, &sd, h);

    out.tex = tex;
    out.srvCpuPtr = (unsigned __int64)h.ptr;
    out.width = width;
    out.height = height;
    out.depth = depth;
    out.mips = mipCount;
    out.dxgiFormat = dxgiFormat;
    T12Log("[D3D12Tex] 3D %dx%dx%d mips=%d fmt=%d bytes=%llu\n", width, height,
           depth, mipCount, dxgiFormat, (unsigned long long)total);
    return true;
}

bool D3D12TextureManager::CreateBCn(D3D12Texture& out, int width, int height,
                                    int dxgiFormat, const void* blockData,
                                    int /*bytes*/)
{
    int blocksW = (width + 3) / 4;
    int rowPitch = blocksW * BlockBytes(dxgiFormat);
    TexMipData mip;
    mip.data = blockData;
    mip.rowPitch = rowPitch;
    return Create(out, width, height, dxgiFormat, &mip, 1);
}

bool D3D12TextureManager::CreateRenderTarget(D3D12Texture& out, int width,
                                             int height)
{
    if (!m_pDevice || width < 1 || height < 1)
        return false;

    D3D12_HEAP_PROPERTIES hp;
    ZeroMemory(&hp, sizeof(hp));
    hp.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC td;
    ZeroMemory(&td, sizeof(td));
    td.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    td.Width = (UINT64)width;
    td.Height = (UINT)height;
    td.DepthOrArraySize = 1;
    td.MipLevels = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    td.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE cv;
    ZeroMemory(&cv, sizeof(cv));
    cv.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // black
    ID3D12Resource* tex = 0;
    if (FAILED(m_pDevice->CreateCommittedResource(
            &hp, D3D12_HEAP_FLAG_NONE, &td,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &cv,
            IID_PPV_ARGS(&tex))))
    {
        T12Log("[D3D12Tex] RTT create failed %dx%d\n", width, height);
        return false;
    }

    // SRV (sampled by DrawRttQuad) + RTV in the staging/RTV heaps -- reuse reclaimed slots (free-list).
    int srvSlot = AllocSrvSlot(), rtvSlot = AllocRtvSlot();
    if (srvSlot < 0 || rtvSlot < 0)
    {
        T12Log("[D3D12Tex] RTT heap full\n");
        if (m_csInit)
            EnterCriticalSection(&m_cs);
        if (srvSlot >= 0)
            m_srvFree.push_back((unsigned)srvSlot);
        if (rtvSlot >= 0)
            m_rtvFree.push_back((unsigned)rtvSlot);
        if (m_csInit)
            LeaveCriticalSection(&m_cs);
        tex->Release();
        return false;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE sh =
        m_pSrvStaging->GetCPUDescriptorHandleForHeapStart();
    sh.ptr += (SIZE_T)srvSlot * m_srvInc;
    D3D12_SHADER_RESOURCE_VIEW_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    sd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    sd.Texture2D.MipLevels = 1;
    m_pDevice->CreateShaderResourceView(tex, &sd, sh);

    // RTV (bound while displays draw into it).
    D3D12_CPU_DESCRIPTOR_HANDLE rh =
        m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();
    rh.ptr += (SIZE_T)rtvSlot * m_rtvInc;
    m_pDevice->CreateRenderTargetView(tex, NULL, rh);

    out.tex = tex;
    out.srvCpuPtr = (unsigned __int64)sh.ptr;
    out.rtvCpuPtr = (unsigned __int64)rh.ptr;
    out.width = width;
    out.height = height;
    out.mips = 1;
    out.dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    out.isRT = true;
    out.rtState = (unsigned)D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    return true;
}

// #DX12: SRV/RTV slot free-list. Reuse a reclaimed slot before bumping the head -- without this every texture
// AND every Reload (palette/TOD/LOD, very frequent) leaked a staging slot forever -> the 8192-slot heap
// exhausted ("staging heap full") mid-mission -> new textures/RTTs failed -> objects invisible, RTT blank.
int D3D12TextureManager::AllocSrvSlot()
{
    int slot = -1;
    if (m_csInit)
        EnterCriticalSection(&m_cs);
    if (!m_srvFree.empty())
    {
        slot = (int)m_srvFree.back();
        m_srvFree.pop_back();
    }
    else if (m_srvHead < m_srvCount)
    {
        slot = (int)m_srvHead++;
    }
    if (m_csInit)
        LeaveCriticalSection(&m_cs);
    return slot;
}
int D3D12TextureManager::AllocRtvSlot()
{
    int slot = -1;
    if (m_csInit)
        EnterCriticalSection(&m_cs);
    if (!m_rtvFree.empty())
    {
        slot = (int)m_rtvFree.back();
        m_rtvFree.pop_back();
    }
    else if (m_rtvHead < m_rtvCount)
    {
        slot = (int)m_rtvHead++;
    }
    if (m_csInit)
        LeaveCriticalSection(&m_cs);
    return slot;
}
void D3D12TextureManager::ReclaimSlots(const D3D12Texture& t)
{
    if (m_csInit)
        EnterCriticalSection(&m_cs);
    if (m_pSrvStaging && t.srvCpuPtr && m_srvInc)
    {
        unsigned __int64 base = (unsigned __int64)m_pSrvStaging
                                    ->GetCPUDescriptorHandleForHeapStart()
                                    .ptr;
        unsigned slot = (unsigned)((t.srvCpuPtr - base) / m_srvInc);
        if (slot < m_srvCount)
            m_srvFree.push_back(slot);
    }
    if (t.isRT && m_pRtvHeap && t.rtvCpuPtr && m_rtvInc)
    {
        unsigned __int64 base =
            (unsigned __int64)m_pRtvHeap->GetCPUDescriptorHandleForHeapStart()
                .ptr;
        unsigned slot = (unsigned)((t.rtvCpuPtr - base) / m_rtvInc);
        if (slot < m_rtvCount)
            m_rtvFree.push_back(slot);
    }
    if (m_csInit)
        LeaveCriticalSection(&m_cs);
}

void D3D12TextureManager::Destroy(D3D12Texture& t)
{
    // #DX12: return the SRV (+ RTV) descriptor slot(s) to the free-list so the next Create/CreateRenderTarget
    // reuses them -- bounds the staging heap to LIVE textures instead of leaking on every reload.
    ReclaimSlots(t);

    // #107: the bindless slot goes back with the resource, once SAFE_FRAMES have
    // passed -- see DeferTextureFree below. Returning it here would let a frame
    // still in flight sample through a slot already given to another texture.
    const unsigned bindlessSlot = t.bindlessSlot;
    t.bindlessSlot = 0xFFFFFFFFu;

    // #65 perf/lifetime: DEFER releasing the resource AND (if pooled) reclaiming its region by TEX_POOL_SAFE_FRAMES
    // frames. Async uploads mean the texture's own upload copy (its ring command allocator) and any render that
    // drew it may still be in flight; releasing now trips the debug layer's VerifyNotInUse and can hang. TickFrame
    // releases + reclaims once the GPU is guaranteed done. (This replaced the old immediate T12_RELEASE(t.tex).)
    DeferTextureFree(t.tex, t.poolHeapIdx, t.poolOffset, t.poolSize,
                     bindlessSlot);
    t.tex = 0;
    t.srvCpuPtr = 0;
    t.rtvCpuPtr = 0;
    t.isRT = false;
    t.width = t.height = t.mips = 0;
    t.poolHeapIdx = -1;
    t.poolOffset = 0;
    t.poolSize = 0;
}

ID3D12Resource* D3D12TextureManager::CreateVertexBufferGPU(const void* data,
                                                           unsigned bytes)
{
    if (!m_pDevice || !data || bytes < 1)
        return 0;

    D3D12_HEAP_PROPERTIES hpDef;
    ZeroMemory(&hpDef, sizeof(hpDef));
    hpDef.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bd.Width = bytes;
    bd.Height = 1;
    bd.DepthOrArraySize = 1;
    bd.MipLevels = 1;
    bd.Format = DXGI_FORMAT_UNKNOWN;
    bd.SampleDesc.Count = 1;
    bd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ID3D12Resource* vb = 0;
    // #DX12: create in COMMON, not COPY_DEST. A BUFFER cannot be created in COPY_DEST -- D3D12 ignores it and
    // creates it in COMMON, emitting warning #1328 CREATERESOURCE_STATE_IGNORED on EVERY object VB. In a Debug
    // build the InfoQueue BREAKs on that warning -> a _com_error per VB -> a stall per VB. 245 VBs at scene load
    // = 245 breaks -> the GPU falls far enough behind that the TDR watchdog removes the device (DEVICE_HUNG).
    // The CopyBufferRegion below implicitly promotes the buffer COMMON->COPY_DEST, so the copy + barrier are fine.
    HRESULT hrVb;
    {
        hrVb = m_pDevice->CreateCommittedResource(
            &hpDef, D3D12_HEAP_FLAG_NONE, &bd, D3D12_RESOURCE_STATE_COMMON,
            NULL, IID_PPV_ARGS(&vb));
    }
    if (FAILED(hrVb))
    {
        T12Log("[D3D12Tex] VB create failed %u\n", bytes);
        return 0;
    }

    // Stage into the ring slot's reusable upload buffer (no per-VB allocation), fill + copy + submit under the lock.
    EnterCriticalSection(&m_cs);
    int idx = BeginUploadList();
    ID3D12Resource* upload = EnsureUploadRing(idx, bytes);
    if (!upload)
    {
        LeaveCriticalSection(&m_cs);
        vb->Release();
        return 0;
    }
    void* mapped = 0;
    D3D12_RANGE noRead;
    noRead.Begin = 0;
    noRead.End = 0;
    if (FAILED(upload->Map(0, &noRead, &mapped)) || !mapped)
    {
        LeaveCriticalSection(&m_cs);
        vb->Release();
        return 0;
    }
    memcpy(mapped, data, bytes);
    upload->Unmap(0, NULL);

    m_pList->CopyBufferRegion(vb, 0, upload, 0, bytes);
    D3D12_RESOURCE_BARRIER b;
    ZeroMemory(&b, sizeof(b));
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource = vb;
    b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    b.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    m_pList->ResourceBarrier(1, &b);
    EndUploadList(idx);
    LeaveCriticalSection(&m_cs);

    return vb;
}

D3D12Texture* D3D12TextureManager::Alloc()
{
    return new D3D12Texture();
}
void D3D12TextureManager::Free(D3D12Texture* h)
{
    if (!h)
        return;
    // #DX12: drop any renderer binding to this texture BEFORE deleting it (else m_pTex0/m_pTex1 dangle -> the
    // next FlushConstants reads a freed struct's srvCpuPtr -> debug-layer break / crash on 3D re-entry).
    extern void D3D12Renderer_NotifyTextureFreed(const void* tex);
    D3D12Renderer_NotifyTextureFreed(h);
    Destroy(*h);
    delete h;
}

// #65 perf: free-function shim so d3d12backend (Present/EndEyeFrame) can insert the render queue's GPU-side
// wait on the upload fence without including the manager header.
void D3D12TexMgr_SyncRenderQueue(ID3D12CommandQueue* renderQueue)
{
    if (g_pD3D12TextureManager)
        g_pD3D12TextureManager->SyncRenderQueue(renderQueue);
}

//================================ NVTT 3 offline DDS authoring ================
// Artscout - 2026 (D3D11 purge): moved from d3d11texturemanager.cpp when the D3D11 texture manager was retired.
// Pure NVTT (backend-agnostic); the terrain/fartex DDS cache calls SaveBCnDDS to bake a .dds. NVTT 3.x is
// x64-only here -> the real path compiles only under _WIN64 (or when forced); Win32 degrades to a no-op stub
// (the runtime never authors DDS -- it loads ready ones).
#if defined(_WIN64) || defined(REDVIPER_USE_NVTT3)
#include <nvtt/nvtt.h>

// nvtt30205.lib is linked via the Falcon4 x64 AdditionalDependencies (not a #pragma here -- /NODEFAULTLIB
// suppresses defaultlib pragmas). Drop nvtt30205.dll next to the exe at runtime.
namespace
{
// Pick the NVTT block format from the legacy MPR_TI_* flags (alpha -> BC2, chroma-key -> BC1a, else BC1).
nvtt::Format NvttFormatFromMPR(unsigned long f)
{
    if (f & MPR_TI_ALPHA)
        return nvtt::Format_BC2;
    if (f & MPR_TI_CHROMAKEY)
        return nvtt::Format_BC1a;
    return nvtt::Format_BC1;
}
} // namespace

bool D3D12TextureManager::SaveBCnDDS(const char* fileName,
                                     unsigned long mprTexInfoFlags,
                                     const void* bgra, int width, int height)
{
    if (!fileName || !bgra || width <= 0 || height <= 0)
        return false;

    nvtt::Surface image;
    // Legacy DumpImageToFile produces tightly-packed BGRA8 (B,G,R,A bytes).
    if (!image.setImage(nvtt::InputFormat_BGRA_8UB, width, height, 1, bgra))
        return false;

    nvtt::CompressionOptions co;
    co.setFormat(NvttFormatFromMPR(mprTexInfoFlags));

    nvtt::OutputOptions oo;
    oo.setFileName(fileName);
    oo.setContainer(
        nvtt::Container_DDS); // legacy DX9 .dds header (no DX10 ext)
    oo.setOutputHeader(true);

    nvtt::Context ctx(true); // CUDA if available, else CPU fallback
    if (!ctx.outputHeader(image, 1, co,
                          oo)) // single mip (old path used dNoMipMaps)
        return false;
    return ctx.compress(image, 0, 0, co, oo);
}

#else // !(_WIN64 || REDVIPER_USE_NVTT3)
// Win32 / NVTT-unavailable: offline DDS authoring is not supported.
bool D3D12TextureManager::SaveBCnDDS(const char*, unsigned long, const void*,
                                     int, int)
{
    return false;
}
#endif // _WIN64 || REDVIPER_USE_NVTT3
