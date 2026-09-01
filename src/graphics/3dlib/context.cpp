/***************************************************************************\
    Context.cpp
    Scott Randolph
 April 29, 1996

    //JAM 06Oct03 - Begin Major Rewrite
\***************************************************************************/
#include "stdafx.h"
#include "imagebuf.h"
#include "context.h"
#include "polylib.h"
#include "statestack.h"
#include "render3d.h"
#include "alloc.h"
#include "radix.h"
#include "graphics/include/texbank.h"
#include "graphics/include/fartex.h"
#include "graphics/include/terrtex.h"
#include "falclib/include/playerop.h"
#include "falclib/include/dispopts.h"
#include "graphics/include/tod.h"
#include "sim/include/otwdrive.h"
#include "realweather.h"


extern DWORD p3DpitHilite; // Cobra - 3D pit high night lighting color
extern DWORD p3DpitLolite; // Cobra - 3D pit low night lighting color


#include "graphics/dxengine/dxengine.h"
#include "graphics/dxengine/dxvbmanager.h"
#include "graphics/dxengine/common/irenderer.h" // PHASE 4: D3D11 screen-path
#include "graphics/dxengine/d3d12backend.h" // #DX12 п.5: per-eye gScreenSize (SceneW/H) for the 2D sky/terrain
extern bool g_bUse_DX_Engine;
extern bool
    g_bUseD3D12; // Artscout - 2026: #DX12 -- the (sole) GPU backend selector
extern bool
    g_bUseGpu; // Artscout - 2026: #DX12 -- GPU render mode (D3D11 || D3D12), not dead DDraw7

extern bool g_bSlowButSafe;
extern float g_fMipLodBias;

#define INT3                                                                   \
    __debugbreak() // Artscout - 2026 (x64): int 3 intrinsic, builds on x86+x64

#ifdef _DEBUG

#define _CONTEXT_ENABLE_STATS
//#define _CONTEXT_RECORD_USED_STATES
//#define _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
//#define _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT_REPLACE
//define _CONTEXT_FLUSH_EVERY_PRIMITIVE
//#define _CONTEXT_TRACE_ALL
#define _CONTEXT_USE_MANAGED_TEXTURES
//#define _VALIDATE_DEVICE

static int m_nInstCount = 0;

#ifdef _CONTEXT_RECORD_USED_STATES
#include <set>
static std::set<int> m_setStatesUsed;
#endif

#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT_REPLACE
static bool bEnableRenderStateHighlightReplace = false;
static int bRenderStateHighlightReplaceTargetState = 0;
#endif

#endif //_DEBUG

UInt ContextMPR::StateTable[MAXIMUM_MPR_STATE];
ContextMPR::State ContextMPR::StateTableInternal[MAXIMUM_MPR_STATE];
int ContextMPR::StateSetupCounter = 0;

bool bInBeginScene = false; // ASSO: BeginScene/EndScene check


// COBRA - RED - This function is to calculate some CXs used in Drawing functions
// They are calculated on change of gZBias by 'ContextMPR::setGlobalZBias()'
inline void ContextMPR::ZCX_Calculate(void)
{
    zNear = ZNEAR + gZBias;
    szCX1 = ZFAR / (ZFAR - zNear);
    szCX2 = ZFAR * zNear / (zNear - ZFAR);
}

// Macro to use CXes
#define SCALE_SZ(x) (szCX1 + szCX2 / x)

// COBRA - RED - End


ContextMPR::ContextMPR()
{
#ifdef _DEBUG
    m_nInstCount++;
#endif

    m_pCtxDX = NULL;
    m_pDD = NULL;
    m_pD3DD = NULL;
    m_pVB = NULL;
    m_pVBH = NULL;
    m_pVBB = NULL;
    m_dwVBSize = 0;
    m_pIdx = NULL;
    m_dwNumVtx = 0;
    m_dwNumIdx = 0;
    m_dwStartVtx = 0;
    m_nCurPrimType = 0;
    m_pRenderTarget = NULL;
    m_bEnableScissors = false;
    m_pDDSP = NULL;
    m_bNoD3DStatsAvail = false;
    m_bUseSetStateInternal = false;
    m_pIB = NULL;
    m_nFrameDepth = 0;
    m_pTLVtx = NULL;
    m_pVBCpu = NULL;
    m_bRenderTargetHasZBuffer = false;
    m_bViewportLocked = false;
    m_colFG = m_colBG = 0;
    m_colFG_Raw = m_colBG_Raw = 0;
    bZBuffering = false;
    gZBias = 0.f;
    m_2DPrimZ =
        1.0f; // #48: 2D screen primitives default to the near plane (reversed-Z: near = 1.0)
    ZFAR = 280000.f;
    // COBRA - RED - TEST
    //ZNEAR = 1.f;
    ZNEAR = 0.2f;
    NVGmode = 0;
    TVmode = 0;
    IRmode = 0;
    palID = 0;
    texID = 0;

    ZCX_Calculate(); // COBRA - RED - Drawing CXs update

#ifdef _DEBUG
    m_pVtxEnd = NULL;
#endif
}

ContextMPR::~ContextMPR()
{
#ifdef _DEBUG
    m_nInstCount--;
#endif
}

BOOL ContextMPR::Setup(ImageBuffer *pIB, DXContext *c)
{
    BOOL bRetval = FALSE;

#ifdef _CONTEXT_TRACE_ALL
    MonoPrint("ContextMPR::Setup(0x%X,0x%X)\n", pIB, c);
#endif

    try
    {
        m_pCtxDX = c;

        if (not m_pCtxDX)
        {
            ShiWarning("Failed to create device");
            return FALSE;
        }

        ShiAssert(m_pTLVtx == NULL);

        ShiAssert(pIB);

        if (not pIB)
            return FALSE;

        m_pIB = pIB;

        // PHASE 4: ContextMPR on D3D11. We don't create a D3D7 device (m_pDD/m_pD3DD) - the
        // screen path (TLVERTEX/XYZRHW) funnels into g_pRenderer->DrawTL. The VB lives in the
        // CPU array m_pVBCpu (instead of m_pVB->Lock); RestoreState -> SetState;
        // SetState(MPR_STA_*) goes through SetStateInternal (without m_pD3DD).
        // Artscout - 2026: #DX12 -- D3D12 uses the SAME CPU-VB screen context as D3D11 (TLVERTEX funnels into
        // g_pRenderer->DrawTL; no D3D7 device). Without this the D3D12 path fell through to the dead DDraw
        // branch below -> device create failed -> ShiError "Failed to setup rendering context" on 3D entry.
        if (g_bUseGpu)
        {
            m_pDD = NULL;
            m_pD3DD = NULL;
            m_pVB = m_pVBB = NULL;

            m_dwVBSize = 32768;
            m_pVBCpu = new TLVERTEX[m_dwVBSize];
            if (not m_pVBCpu)
                throw _com_error(E_OUTOFMEMORY);

            m_pIdx = new WORD
                [m_dwVBSize * 3 +
                 64]; // +slack: fan triangulation is right up against 3*N
            if (not m_pIdx)
                throw _com_error(E_OUTOFMEMORY);

            m_bUseSetStateInternal =
                true; // SetState(MPR_STA_*) -> SetStateInternal (m_pD3DD-free)

            // Initialize the buckets/states (like the D3D7 branch below, but without D3D7 calls)
            mIdx = 0;
            plainPolys = texturedPolys = translucentPolys = NULL;
            plainPolyVCnt = texturedPolyVCnt = translucentPolyVCnt = 0;
            currentState = lastState = currentTexture1 = currentTexture2 =
                lastTexture1 = lastTexture2 = -1;
            m_dwStartVtx = m_dwNumVtx = m_dwNumIdx = 0;
            m_nCurPrimType = 0;
            memPool = AllocInit();
            RadixReset();

            InvalidateState();
            RestoreState(STATE_SOLID);
            ZeroMemory(&m_rcVP, sizeof(m_rcVP));
            m_bViewportLocked = false;

            // Artscout - 2026 (D3D11 purge): the D3D11 viewport-size init was removed; under D3D12 gScreenSize
            // is set per-frame in StartFrame (SceneW/H) / per RTT.
            return TRUE;
        }

        // #34 D3D7 device/state setup removed (D3D11 returns TRUE above).
    }

    catch (const _com_error &e)
    {
        MonoPrint("ContextMPR::Setup - Error 0x%X\n", e.Error());
    }

    return bRetval;
}

void ContextMPR::Cleanup()
{
#ifdef _CONTEXT_TRACE_ALL
    MonoPrint("ContextMPR::Cleanup()\n");
#endif

#ifdef _DEBUG
#ifdef _CONTEXT_ENABLE_STATS
    m_stats.Report();
#endif
#endif

    if (StateSetupCounter)
        CleanupMPRState(CHECK_PREVIOUS_STATE);

    // Warning: The SIM code uses a shared DXContext which might be already toast when this function gets called
    // Under no circumstances access m_pCtxDX here
    // Btw: this was causing the infamous LGB CTD

    m_pCtxDX = NULL;

    // Artscout - 2026: [DX7-PURGE] no DDraw7 vertex buffers to Release under GPU.
    m_pVB = NULL;
    m_pVBB = NULL;

    if (m_pIdx)
    {
        delete[] m_pIdx;
        m_pIdx = NULL;
    }

    if (m_pVBCpu) // PHASE 4
    {
        delete[] m_pVBCpu;
        m_pVBCpu = NULL;
    }

    m_pIdx = NULL;
    m_dwNumVtx = 0;
    m_dwNumIdx = 0;
    m_dwStartVtx = 0;
    m_nCurPrimType = 0;
    m_pIB = NULL;
    m_nFrameDepth = 0;
    m_pTLVtx = NULL;
    m_bRenderTargetHasZBuffer = false;

#ifdef _DEBUG
    m_pVtxEnd = NULL;
#endif

#ifdef _CONTEXT_RECORD_USED_STATES
    MonoPrint("ContextMPR::Cleanup - Report of used states follows\n ");
    std::set<int>::iterator it;

    for (it = m_setStatesUsed.begin(); it not_eq m_setStatesUsed.end(); it++)
        MonoPrint("%d,", *it);

    m_setStatesUsed.clear();
    MonoPrint("\nContextMPR::Cleanup - End of report\n ");
#endif
}

void ContextMPR::NewImageBuffer(UInt lpDDSBack)
{
#ifdef _CONTEXT_TRACE_ALL
    MonoPrint("ContextMPR::NewImageBuffer(0x%X)\n", lpDDSBack);
#endif

    if (m_pRenderTarget)
        m_pRenderTarget = NULL;

    m_pRenderTarget = (IDirectDrawSurface7 *)lpDDSBack;

    // Artscout - 2026: [DX7-PURGE] the DDraw GetAttachedSurface Z-buffer probe is gone
    // (GPU depth is managed by the D3D11/D3D12 backend).
    m_bRenderTargetHasZBuffer = false;
}

void ContextMPR::ClearBuffers(WORD ClearInfo)
{
    // PHASE: back-buffer clear is on the D3D11Backend side. We clear the RTT atlas HERE (once at
    // the start of the batch via ClearDraw), because StartRtt no longer clears (a repeated StartRtt
    // from an MFD would wipe HUD/RWR/DED). Clear only during the RTT batch (g_rttBatchActive).
    // #34: dead D3D7 m_pD3DD->Clear removed.
    // #DX12: clear the RTT atlas on the ACTIVE backend (D3D11 or D3D12). Under D3D12 this was g_bUseD3D11-gated
    // -> never ran -> HUD/MFD symbology accumulated frame to frame. The neutral ClearCurrentRTV clears the
    // currently-bound RTV (the RTT atlas, since g_rttBatchActive means StartRtt bound it).
    if (g_bUseGpu)
    {
        extern bool g_rttBatchActive;
        extern IRenderBackend *g_pRenderBackend;
        if (g_rttBatchActive and (ClearInfo bitand MPR_CI_DRAW_BUFFER) and
            g_pRenderBackend)
            g_pRenderBackend->ClearCurrentRTV(0.0f, 0.0f, 0.0f, 0.0f);
    }
}


void ContextMPR::StartDraw(void)
{
    // PHASE 5 (RTT): the display's render target is switched by StartRtt/AdjustRttViewport.
    // #34: dead D3D7 SetRenderTarget/Clear/UpdateViewport path removed.
    InvalidateState();
}

void ContextMPR::EndDraw(void)
{
    FlushVB(); // flush the display content into the current RTV (its RTT)

    // PHASE 5 (RTT): after rendering the display into its RTT, restore the scene target + its gScreenSize.
    // Artscout - 2026 (#104): asks the ACTIVE backend, not g_pD3D12Backend. Under Vulkan this restore was skipped
    // entirely, so gScreenSize stayed at the display ATLAS size for everything drawn afterwards -- including the RTT
    // composite quad, whose pixel->NDC mapping then had nothing to do with the screen. The displays ended up glued
    // to the camera instead of to their panels.
    if (g_bUseGpu && g_pRenderBackend && m_pIB && not m_pIB->IsScreenBuffer())
    {
        // The RTT display just set gScreenSize to its atlas size; restore it to the SCENE size (eye in VR, back
        // buffer flat) so the next scene draws map correctly. The scene RTV itself is rebound by
        // UnbindSceneRtt/BindBackBufferRTV in the display path -- here we only fix gScreenSize.
        if (g_pRenderer)
            g_pRenderer->SetViewportSize(g_pRenderBackend->SceneW(),
                                         g_pRenderBackend->SceneH());
    }
}


void ContextMPR::StartFrame(void)
{
    // PHASE 4/5: no D3D7 BeginScene/Clear; the screen pass goes through DrawTL. If this context has
    // an off-screen target (MFD/HUD/radar) bind its RTT texture, else the back buffer.
    // #34: dead D3D7 BeginScene/Clear/surface-lost tail removed.
    if (m_pIB && not m_pIB->IsScreenBuffer())
    {
        m_pIB->BindRttTarget(true);
    }
    else if (g_bUseGpu && g_pRenderBackend)
    {
        // #DX12 п.5: the scene target (eye in VR, back buffer flat) is already bound + cleared by
        // BeginEyeFrame/BeginFrame. Only set gScreenSize here -- to the SCENE size, so VS_Screen's pixel->NDC
        // for the CPU-projected 2D sky/terrain matches the eye-sized pixels (VR_SetRes -> scaleX/scaleY). This
        // is the D3D12 analogue of the D3D11 XrEyeActive() branch below; it was missing (SetGScreenSize was a
        // no-op stub + this block gated on g_pD3D11Backend), so the eye-sized sky was mapped by the back-buffer
        // size -> horizon mis-scaled/inverted ("dark blue, sky only when inverted"). Terrain/objects survive it
        // (world matProj is unaffected) which is why only the pure-2D sky visibly broke.
        // Artscout - 2026 (#104): via the neutral backend, so Vulkan gets the same treatment -- it was named
        // g_pD3D12Backend, which made this a D3D12-only fix and left the Vulkan 2D path mapped by the wrong size.
        if (g_pRenderer)
            g_pRenderer->SetViewportSize(g_pRenderBackend->SceneW(),
                                         g_pRenderBackend->SceneH());
    }

    InvalidateState();
}


void ContextMPR::BindD3D11RttNoClear(void)
{
    // Artscout - 2026: bind this context's off-screen RTT but do NOT clear it. Used by the GM radar,
    // which renders its sweep incrementally across frames into m_pRenderTarget; clearing every
    // StartDraw would wipe the accumulated image (StartScene/ClearDraw clears when a scene restarts).
    // Without this the radar sweep leaks onto the screen (no RTT bound -> draws to the back buffer).
    // #DX12 A5: BindRttTarget delegates to the D3D12 RTT under g_bUseD3D12.
    // Artscout - 2026 (GM radar): Vulkan too -- BindRttTarget routes by backend. Un-gated, the Vulkan GM sweep
    // was never bound to its private buffer, so it leaked onto the current target and StartScene's clear wiped
    // the cockpit atlas mid-frame (dark MFDs).
    extern bool g_bUseVulkan;
    if ((g_bUseD3D12 || g_bUseVulkan) && m_pIB && not m_pIB->IsScreenBuffer())
        m_pIB->BindRttTarget(false);
}

void ContextMPR::ClearBoundD3D11Rtt(void)
{
    // Artscout - 2026: clear the currently-bound off-screen RTV NOW. ClearBuffers() is gated to the RTT
    // batch (g_rttBatchActive) and no-ops for the GM radar's private buffer, so the sweep never cleared
    // and accumulated green to a full-field white. The GM calls this once per sweep (StartScene), after
    // StartDraw has bound its buffer; the per-beam-op accumulation within the sweep is unaffected.
    // #DX12 A5: same for a GPU backend (ClearCurrentRTV clears the currently-bound off-screen RTV).
    // Artscout - 2026 (#104): via the ACTIVE backend -- ClearCurrentRTV is already an IRenderBackend method, so
    // naming D3D12 here only meant Vulkan never cleared. That is the very bug the comment above describes (a sweep
    // that never clears accumulates to a full-field white), just re-armed for the other backend.
    if (g_bUseGpu && m_pIB && not m_pIB->IsScreenBuffer() && g_pRenderBackend)
        g_pRenderBackend->ClearCurrentRTV(0.0f, 0.0f, 0.0f, 0.0f);
}

void ContextMPR::FinishFrame(void *lpFnPtr)
{
    FlushVB();
    // #34 D3D11: present is done by ImageBuffer::PresentGpu; dead D3D7 EndScene/surface-lost
    // tail removed.

    // Artscout - 2026: #DX12 A5 -- for an off-screen IB (TGP/FLIR/Munitions/GM) transition its D3D12 RTT
    // back to PIXEL_SHADER_RESOURCE and rebind the scene target, so the sensor render is finished and the
    // main pass (or the readback copy) can continue. Under D3D11 the RTV stays bound (next BindBackBuffer
    // restores it); under D3D12 explicit unbind is required.
    {
        extern bool
            g_bUseVulkan; // Artscout - 2026 (GM radar): Vulkan unbinds through the same neutral call
        if ((g_bUseD3D12 || g_bUseVulkan) && m_pIB &&
            not m_pIB->IsScreenBuffer())
            m_pIB->UnbindGpuRenderTarget();
    }
}

// DX - COBRA - Red
// This function forces the use of the Vertex Colours as Texture Color source
// Used for HUD Text
void ContextMPR::TexColorDiffuse(void)
{
    // PHASE 4/6: emulate COLORARG1=DIFFUSE in the FFEmu shader (FF_TEXCOLORDIFFUSE): text color
    // from the vertex, the font texture is only a mask. Cleared on the next RestoreState.
    // #34: dead D3D7 SetTextureStageState removed.
    if (g_pRenderer)
        g_pRenderer->SetTexColorDiffuse(true);
}


void ContextMPR::SetState(WORD State, DWORD Value)
{
    // #34 D3D11: render state is applied via SetStateInternal / the FFEmu shader (FFStateMap).
    // Under D3D11 m_bUseSetStateInternal is always true (set in Setup); the legacy D3D7
    // fixed-function state machine (m_pD3DD switch) has been removed.
    if (m_bUseSetStateInternal)
    {
        SetStateInternal(State, Value);
        return;
    }
}

void ContextMPR::SetStateInternal(WORD State, DWORD Value)
{
    switch (State)
    {
    case MPR_STA_NONE:
    {
        break;
    }

    case MPR_STA_DISABLES:
    case MPR_STA_ENABLES:
    {
        bool bNewVal = (State == MPR_STA_ENABLES) ? true : false;

        // Artscout - 2026: currentState is -1 right after InvalidateState() (the rendered-
        // cursor path runs StartDraw -> SetViewport(SCISSORING) before any RestoreState).
        // Writing StateTableInternal[-1] is an out-of-bounds store that corrupted adjacent
        // memory -> Release-only crash on mouse move (garbage m_dwNumVtx / nulled m_pIdx,
        // lost sky/HUD/MFD). Only touch the per-slot table when the slot index is valid;
        // the scissor enable still updates the global m_bEnableScissors below.
        bool bValidSlot =
            (currentState >= 0 and currentState < MAXIMUM_MPR_STATE);

        if (Value bitand MPR_SE_SCISSORING)
        {
            if (bValidSlot)
                StateTableInternal[currentState].SE_SCISSORING = bNewVal;
            // PHASE 5: apply the per-display viewport (MFD/HUD are positioned by it)
            if (bNewVal not_eq (m_bEnableScissors ? true : false))
            {
                FlushVB();
                m_bEnableScissors = bNewVal;
                UpdateViewport();
            }
        }

        if (bValidSlot)
        {
            if (Value bitand MPR_SE_MODULATION)
                StateTableInternal[currentState].SE_MODULATION = bNewVal;

            if (Value bitand MPR_SE_TEXTURING)
                StateTableInternal[currentState].SE_TEXTURING = bNewVal;

            if (Value bitand MPR_SE_SHADING)
                StateTableInternal[currentState].SE_SHADING = bNewVal;

            if (Value bitand MPR_SE_Z_BUFFERING)
                StateTableInternal[currentState].SE_Z_BUFFERING = bNewVal;

            if (Value bitand MPR_SE_Z_WRITE)
                StateTableInternal[currentState].SE_Z_WRITE = bNewVal;

            if (Value bitand MPR_SE_FILTERING)
                StateTableInternal[currentState].SE_FILTERING = bNewVal;

            if (Value bitand MPR_SE_ALPHA)
                StateTableInternal[currentState].SE_ALPHA = bNewVal;

            if (Value bitand MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE)
                StateTableInternal[currentState]
                    .SE_NON_PERSPECTIVE_CORRECTION_MODE = bNewVal;
        }

        break;
    }

    // PHASE 5: the display's scissor rect (MFD/HUD). In D3D11 it wasn't handled before ->
    // all displays were fullscreen. Now -> per-display viewport.
    case MPR_STA_SCISSOR_LEFT:
        if (Value not_eq (DWORD) m_rcVP.left)
        {
            FlushVB();
            m_rcVP.left = Value;
            UpdateViewport();
        }
        break;
    case MPR_STA_SCISSOR_TOP:
        if (Value not_eq (DWORD) m_rcVP.top)
        {
            FlushVB();
            m_rcVP.top = Value;
            UpdateViewport();
        }
        break;
    case MPR_STA_SCISSOR_RIGHT:
        if (Value not_eq (DWORD) m_rcVP.right)
        {
            FlushVB();
            m_rcVP.right = Value;
            UpdateViewport();
        }
        break;
    case MPR_STA_SCISSOR_BOTTOM:
        if (Value not_eq (DWORD) m_rcVP.bottom)
        {
            FlushVB();
            m_rcVP.bottom = Value;
            UpdateViewport();
        }
        break;
    }
}

// flag bitand 0x01  --> skip StateSetupCount checking --> reset/set state
void ContextMPR::SetCurrentState(GLint state, GLint flag)
{
    // #34 D3D11: dead -- this was the D3D7 state-block recorder (m_pD3DD texture-stage/render
    // states), only reached via SetStateTable<-SetupMPRState, which never runs under D3D11
    // (StateSetupCounter stays 0; m_pD3DD is NULL). State is applied via SetStateInternal/FFStateMap.
}

void ContextMPR::Render2DBitmap(int sX, int sY, int dX, int dY, int w, int h,
                                int totalWidth, DWORD *pSrc, bool Fit)
{
    // #30/#34 D3D11: CPU bitmap (splash/cursor/mirror) via a temporary texture + screen quad.
    // Dead D3D7 D3DXCreateTexture/SetTexture path removed.
    if (g_pRenderer)
        g_pRenderer->DrawBitmap2D(dX, dY, w, h, totalWidth, sX, sY,
                                  (const unsigned *)pSrc, Fit,
                                  m_pCtxDX->m_nWidth, m_pCtxDX->m_nHeight);
}

inline void ContextMPR::SetStateTable(GLint state, GLint flag)
{
    // #34 D3D11: dead -- recorded a D3D7 state block (m_pD3DD Begin/EndStateBlock). Not used.
}

inline void ContextMPR::ClearStateTable(GLint state)
{
    // #34 D3D11: dead -- released a D3D7 state block (m_pD3DD->DeleteStateBlock). Not used.
}


void ContextMPR::SetupMPRState(GLint flag)
{
    if (flag bitand CHECK_PREVIOUS_STATE)
    {
        StateSetupCounter++;

        if (StateSetupCounter > 1)
            return;
    }
    else if (StateSetupCounter)
        CleanupMPRState();

    // Record one stateblock per poly type
    MonoPrint("ContextMPR - Setting up state table\n");

    for (currentState = STATE_SOLID; currentState < MAXIMUM_MPR_STATE;
         currentState++)
        SetStateTable(currentState, flag);

    InvalidateState();
}

void ContextMPR::CleanupMPRState(GLint flag)
{
    if (not StateSetupCounter)
    {
        ShiWarning("MPR not initialized");
        return;
    }

    if (flag bitand CHECK_PREVIOUS_STATE)
    {
        StateSetupCounter--;

        if (StateSetupCounter > 0)
            return;
    }

    MonoPrint("ContextMPR - Clearing state table\n");

    for (int i = STATE_SOLID; i < MAXIMUM_MPR_STATE; i++)
        ClearStateTable(i);
}

void ContextMPR::SetTexture1(
    DWORD_PTR texID) // Artscout - 2026 (x64): pointer-sized handle/SRV
{
    if (texID not_eq lastTexture1)
    {
        HRESULT hr;

        lastTexture1 = texID;

        if (g_bUseGpu) // PHASE 4/#DX12
        {
            if (g_pRenderer)
            {
                g_pRenderer->SetTexture(
                    0, (texID == -1) ?
                           NULL :
                           (struct ID3D11ShaderResourceView *)texID);
                g_pRenderer->SetTexture(1, NULL);
            }
            return;
        }
        // #34 dead D3D7 SetTexture removed (D3D11 returns above)
    }
}

void ContextMPR::SetTexture2(
    DWORD_PTR texID) // Artscout - 2026 (x64): pointer-sized handle/SRV
{
    if (texID not_eq lastTexture2)
    {
        HRESULT hr;

        lastTexture2 = texID;

        if (g_bUseGpu) // PHASE 4/#DX12
        {
            if (g_pRenderer)
                g_pRenderer->SetTexture(
                    1, (texID == -1) ?
                           NULL :
                           (struct ID3D11ShaderResourceView *)texID);
            return;
        }
        // #34 dead D3D7 SetTexture removed (D3D11 returns above)
    }
}

void ContextMPR::SelectTexture1(
    DWORD_PTR texID) // Artscout - 2026 (x64): pointer-sized handle/SRV
{
#ifdef _CONTEXT_TRACE_ALL
    MonoPrint("ContextMPR::ApplyTexture1(0x%X)\n", texID);
#endif

    if (texID)
        texID = (DWORD_PTR)((TextureHandle *)texID)
                    ->m_pDDS; // Artscout - 2026 (x64): no pointer truncation

    if (texID not_eq currentTexture1)
    {
        currentTexture1 = texID;

#ifdef _CONTEXT_ENABLE_STATS
        m_stats.PutTexture(false);
#endif

        // PHASE 5: in D3D11 bind the texture/font REGARDLESS of bZBuffering. The
        // "if(not bZBuffering)" gate is a D3D7 quirk; because of it the MFD/HUD font wasn't bound
        // in the cockpit (bZBuffering=true) -> empty gTex0 -> "little squares".
        if (not bZBuffering or g_bUseGpu)
        {
            // JB 010326 CTD (too much CPU)
            if (not g_bUseGpu and g_bSlowButSafe and
                F4IsBadReadPtr((TextureHandle *)texID, sizeof(TextureHandle)))
                return;

            FlushVB();

            // #34 D3D11: m_pDDS holds the D3D11 SRV (dead D3D7 else removed)
            if (g_pRenderer)
                g_pRenderer->SetTexture(
                    0, (struct ID3D11ShaderResourceView *)texID);
        }
    }
    else if (g_bUseGpu and g_pRenderer)
    {
        // CACHE HIT (texID == currentTexture1): same texture, but the ACTUAL slot-0 binding and
        // m_hasTex0 may have desynced (SetTexture1(-1)/StartRtt unbound the slot while the cached
        // currentTexture1 stayed) -> HUD text drew without a texture (blocks). Re-sync the binding
        // (same texture, no FlushVB).
        g_pRenderer->SetTexture(0, (struct ID3D11ShaderResourceView *)texID);
    }

#ifdef _CONTEXT_ENABLE_STATS
    else
        m_stats.PutTexture(true);

#endif

    currentTexture2 = -1;
}

void ContextMPR::SelectTexture2(
    DWORD_PTR texID) // Artscout - 2026 (x64): pointer-sized handle/SRV
{
#ifdef _CONTEXT_TRACE_ALL
    MonoPrint("ContextMPR::ApplyTexture2(0x%X)\n", texID);
#endif

    if (texID)
        texID = (DWORD_PTR)((TextureHandle *)texID)
                    ->m_pDDS; // Artscout - 2026 (x64): no pointer truncation

    if (texID not_eq currentTexture2)
    {
        currentTexture2 = texID;

#ifdef _CONTEXT_ENABLE_STATS
        m_stats.PutTexture(false);
#endif

        if (not bZBuffering)
        {
            // JB 010326 CTD (too much CPU)
            if (g_bSlowButSafe and
                F4IsBadReadPtr((TextureHandle *)texID, sizeof(TextureHandle)))
                return;

            FlushVB();

            // #34 D3D11 (dead D3D7 else removed)
            if (g_pRenderer)
                g_pRenderer->SetTexture(
                    1, (struct ID3D11ShaderResourceView *)texID);
        }
    }

#ifdef _CONTEXT_ENABLE_STATS
    else
        m_stats.PutTexture(true);

#endif
}

void ContextMPR::SelectForegroundColor(GLint color)
{
    if (color not_eq m_colFG_Raw)
    {
        m_colFG_Raw = color;
        m_colFG = MPRColor2D3DRGBA(color);
    }
}

void ContextMPR::SelectBackgroundColor(GLint color)
{
    if (color not_eq m_colBG_Raw)
    {
        m_colBG_Raw = color;
        m_colBG = MPRColor2D3DRGBA(color);
    }
}

void ContextMPR::ApplyStateBlock(GLint state)
{
    if (state == -1)
        return;

    ShiAssert(state >= 0 and state < MAXIMUM_MPR_STATE);

    if (state not_eq lastState)
    {
        lastState = state;

        // #34 D3D11: state applied via FFMapState in FlushVB; dead D3D7 ApplyStateBlock removed
    }
}

void ContextMPR::RestoreState(GLint state)
{
    ShiAssert(state not_eq -1);
    ShiAssert(state >= 0 and state < MAXIMUM_MPR_STATE);

    // PHASE 4/#34: the actual state is set by g_pRenderer->SetState (FFMapState) in FlushVB.
    // Dead D3D7 ApplyStateBlock path removed.
    if (state not_eq currentState)
    {
        if (currentState == -1 or
            (StateTableInternal[currentState].SE_TEXTURING and
             not StateTableInternal[state].SE_TEXTURING))
            currentTexture1 = -1;

        if (not bZBuffering)
            FlushVB();

        currentState = state;
    }
}

// COBRA - RED - Comparing or a so short conditional action has no sense, do it always
void ContextMPR::UpdateSpecularFog(DWORD specular)
{
    /*if(specular not_eq m_colFOG)*/ m_colFOG = specular;
}

void ContextMPR::SetZBuffering(BOOL state)
{
    if (not bZBuffering and state)
    {
        FlushVB();
        bZBuffering = state;
    }
    else if (bZBuffering and not state)
    {
        bZBuffering = state;
    }
}

void ContextMPR::SetNVGmode(BOOL state)
{
    NVGmode = state;
}

void ContextMPR::SetTVmode(BOOL state)
{
    TVmode = state;
}

void ContextMPR::SetIRmode(BOOL state)
{
    IRmode = state;
}

// #DX12 A5: free helper to toggle the shader grey pass (FF_IRGREY) for the sensor 3D scene. NOT tied to
// SetTVmode/SetIRmode -- RenderOTW::ComputeVertexColor (otw.cpp:2044) resets those to FALSE mid-terrain, which
// clobbered the flag (scene drew colour, only the symbology stayed grey = inverted). Instead the sensor draw
// (laserpod/mavdisp/lantmfd) brackets its DrawScene with FF_SetIRGrey(true/false) explicitly. Desaturates the
// composed pixel to luma, so the terrain (whose vertex colours are cached from the main colour view) greys too.
void FF_SetIRGrey(bool on)
{
    extern IRenderer *g_pRenderer;
    if (g_pRenderer)
        g_pRenderer->SetIRGrey(on);
}

// Artscout - 2026 (D3D11 purge): re-homed from the deleted D3D11Renderer.cpp. Free shim so the object path
// (DXVbManager::GetDrawItem, via dxengine.h) can toggle the #72 cockpit-fidelity pass without pulling the
// renderer header. Sticky flag on the active renderer, cleared after the pit is drawn.
void FF_SetCockpitPass(bool on)
{
    extern IRenderer *g_pRenderer;
    if (g_pRenderer)
        g_pRenderer->SetCockpitPass(on);
}

// COBRA - RED - Comparing or a so short conditional action has no sense, do it always
void ContextMPR::SetPalID(int id)
{
    /*if(id not_eq palID)*/ palID = id;
}

void ContextMPR::SetTexID(int id)
{
    /*if(id not_eq texID)*/ texID = id;
}

DWORD ContextMPR::MPRColor2D3DRGBA(GLint color)
{
    return RGBA_MAKE(RGBA_GETBLUE(color), RGBA_GETGREEN(color),
                     RGBA_GETRED(color), RGBA_GETALPHA(color));
}

HRESULT WINAPI ContextMPR::EnumSurfacesCB2(
    IDirectDrawSurface7 *lpDDSurface, struct _DDSURFACEDESC2 *lpDDSurfaceDesc,
    LPVOID lpContext)
{
    ContextMPR *pThis = (ContextMPR *)lpContext;
    ShiAssert(FALSE == F4IsBadReadPtr(pThis, sizeof *pThis));
    ShiAssert(FALSE ==
              F4IsBadReadPtr(lpDDSurfaceDesc, sizeof *lpDDSurfaceDesc));

    if (lpDDSurfaceDesc->ddsCaps.dwCaps bitand DDSCAPS_PRIMARYSURFACE)
    {
        pThis->m_pDDSP = lpDDSurface;
        return DDENUMRET_CANCEL;
    }

    return DDENUMRET_OK;
}

void ContextMPR::UpdateViewport()
{
    if (m_bViewportLocked)
        return;
    // PHASE 5/#34 D3D11: per-display viewport is handled by BindRenderTargetView (full texture)
    // + absolute display coordinates; no GPU viewport narrowing here. Dead D3D7
    // GetViewport/SetViewport path removed.
}

void ContextMPR::SetViewportAbs(int nLeft, int nTop, int nRight, int nBottom)
{
    if (m_bViewportLocked)
        return;

    m_rcVP.left = nLeft;
    m_rcVP.right = nRight;
    m_rcVP.top = nTop;
    m_rcVP.bottom = nBottom;

    UpdateViewport();
}

void ContextMPR::LockViewport()
{
    m_bViewportLocked = true;
}

void ContextMPR::UnlockViewport()
{
    m_bViewportLocked = false;
}

void ContextMPR::GetViewport(RECT *prc)
{
    ShiAssert(FALSE == F4IsBadWritePtr(prc, sizeof *prc));
    *prc = m_rcVP;
}

void ContextMPR::Stats()
{
    // #34 D3D11: no D3D7 GetInfo stats.
}

void ContextMPR::TextOut(short x, short y, DWORD col, LPSTR str)
{
#ifdef _CONTEXT_TRACE_ALL
    MonoPrint("ContextMPR::TextOut(%d,%d,0x%X,%s)\n", x, y, col, str);
#endif

    if (not str)
        return;

    // Artscout - 2026: [DX7-PURGE] GDI-on-DDraw-surface text (GetDC/DrawText/ReleaseDC) removed;
    // the GPU path draws text through the renderer, not a DirectDraw surface DC.
    (void)col;
    (void)x;
    (void)y;
}

bool ContextMPR::LockVB(int nVtxCount, void **p)
{
#ifdef _CONTEXT_TRACE_ALL
    MonoPrint(
        "ContextMPR::LockVB(%d,0x%X) (m_dwStartVtx = %d,m_dwNumVtx = %d)\n",
        nVtxCount, p, m_dwStartVtx, m_dwNumVtx);
#endif

    HRESULT hr;
    DWORD dwSize = 0;

    // PHASE 4/#DX12: GPU mode (D3D11 or D3D12) - hand back a pointer into the CPU array (base); writes go
    // to m_pTLVtx[m_dwStartVtx + m_dwNumVtx]. No GPU/DDraw lock needed (the dead DDraw7 m_pVB is NULL).
    if (g_bUseGpu)
    {
        // Artscout - 2026: guard against corrupted context state. Long-standing heap
        // corruption (see known-issues) zeroes the context's m_pIdx pointer / garbages the
        // batch counters; Release exposes it as a null write in DrawPrimitive or an EnsureVB
        // size-overflow hang. If a buffer pointer was wiped, bail cleanly (drop this draw)
        // instead of crashing; the caller treats a false return as "skip primitive".
        if (not m_pVBCpu or not m_pIdx)
        {
            *p = NULL;
            m_pTLVtx = NULL;
            return false;
        }
        if (m_dwNumVtx >= m_dwVBSize || m_dwStartVtx >= m_dwVBSize ||
            m_dwNumIdx >= (DWORD)(m_dwVBSize * 3))
        {
            m_dwStartVtx = m_dwNumVtx = m_dwNumIdx = 0;
        }

        if ((m_dwStartVtx + m_dwNumVtx + (DWORD)nVtxCount) >= m_dwVBSize)
        {
            FlushVB();
            m_dwStartVtx = 0;
        }
        *p = m_pVBCpu;
        m_pTLVtx = m_pVBCpu;
        return true;
    }

    // Artscout - 2026: [DX7-PURGE] DDraw7 m_pVB Lock path removed (GPU returns above).
    return false;
}

void ContextMPR::UnlockVB()
{
#ifdef _CONTEXT_TRACE_ALL
    MonoPrint("ContextMPR::UnlockVB()\n");
#endif

    // PHASE 4/#DX12: GPU mode - data is already in the CPU array, no GPU/DDraw unlock needed
    if (g_bUseGpu)
    {
        m_pTLVtx = NULL;
        return;
    }

    // Artscout - 2026: [DX7-PURGE] DDraw7 m_pVB Unlock removed (GPU returns above).
    m_pTLVtx = NULL;
}

DWORD VCounter;

void ContextMPR::FlushPolyLists(bool clearDepthBeforeObjects)
{
    VCounter = 0;


    // START_PROFILE(BSP_ENGINE_PROF);

    SetState(MPR_STA_ENABLES, MPR_SE_Z_WRITE);
    SetState(MPR_STA_ENABLES, MPR_SE_Z_BUFFERING);

    if (plainPolys not_eq NULL)
        RenderPolyList(plainPolys);

    if (texturedPolys not_eq NULL)
        RenderPolyList(texturedPolys);

    // STOP_PROFILE(BSP_ENGINE_PROF);

    // COBRA - DX - Switching btw Old and New Engine - Flush of the objects
    if (g_bUse_DX_Engine)
    {
        //START_PROFILE(DX_ENGINE_PROF);
        bool k = bZBuffering ? true : false;
        bZBuffering = false;
        // #48: previously this ALWAYS cleared the depth buffer here so the object-path
        // flush (cockpit + world objects, batched together) drew on top of the screen-path
        // sky/terrain. That was a stale workaround from the era when the object projection
        // was inverted (Flip.m02 = -1) and the cockpit ended up behind the terrain. Now the
        // projection is correct and the screen path (sz = szCX1 + szCX2/z) and the object
        // path (PerspectiveFov with the same ZNEAR/ZFAR) produce IDENTICAL NDC depth, so a
        // single coherent depth buffer works: the pit at near-Z (z ~ 1 ft) beats the terrain
        // (z >> 100 ft) on its own, and world objects are correctly occluded by the ground.
        // The OTW world pass passes clearDepthBeforeObjects=false; mini-scene displays keep
        // the clear (default true).
        TheDXEngine.FlushBuffers();
        bZBuffering = k;
        InvalidateState();
        //STOP_PROFILE(DX_ENGINE_PROF);
    }

    // START_PROFILE(BSP_ENGINE_PROF);

    SetState(MPR_STA_DISABLES, MPR_SE_Z_WRITE);
    //TheDXEngine.SetStencilMode(STENCIL_CHECK);

    if (translucentPolys not_eq NULL)
        RenderPolyList(translucentPolys);

    TheDXEngine.SetStencilMode(STENCIL_OFF);

    mIdx = 0;
    plainPolys = texturedPolys = translucentPolys = NULL;
    plainPolyVCnt = texturedPolyVCnt = translucentPolyVCnt = 0;


    AllocResetPool();
    SetZBuffering(FALSE);
    SetState(MPR_STA_DISABLES, MPR_SE_Z_BUFFERING);

    // STOP_PROFILE(BSP_ENGINE_PROF);

    //REPORT_VALUE("Vertices", VCounter);
}

void ContextMPR::FlushVB()
{
    if (not m_dwNumVtx)
        return;

    // Artscout - 2026: FlushVB is reached directly from SelectTexture1/RestoreState/EndDraw
    // (bypassing the LockVB guard). If a batch counter is garbage (corruption), DrawTL/
    // DrawTLIndexed get a huge count / out-of-range start vertex and memcpy reads off the end
    // of m_pVBCpu -> AV. Drop a clearly-invalid batch here instead of crashing.
    if (g_bUseGpu and (not m_pVBCpu or not m_pIdx or m_dwNumVtx >= m_dwVBSize or
                       m_dwStartVtx >= m_dwVBSize or
                       (m_dwStartVtx + m_dwNumVtx) > m_dwVBSize or
                       m_dwNumIdx >= (DWORD)(m_dwVBSize * 3)))
    {
        // Cheap sanity guard: drop an out-of-range batch instead of memcpy'ing off the end of
        // m_pVBCpu. Kept as defensive hardening against any future counter desync.
        m_dwStartVtx = m_dwNumVtx = m_dwNumIdx = 0;
        return;
    }

    ShiAssert(m_nCurPrimType not_eq 0);

#ifdef _CONTEXT_TRACE_ALL
    MonoPrint("ContextMPR::FlushVB()\n");
#endif

    // PHASE 4: D3D11 screen path. m_nCurPrimType (D3DPT_*) == MPR_PKT_* (1..6). Multi-fan/
    // multi-line batches arrive with m_pIdx indices (as TRIANGLELIST/LINELIST) - draw
    // DrawTLIndexed; otherwise DrawTL by m_nCurPrimType.
    if (g_bUseGpu)
    {
        UnlockVB();

        if (g_pRenderer and g_pRenderer->IsValid())
        {
            g_pRenderer->BeginScreenPass();
            g_pRenderer->SetState(currentState);

            if (m_dwNumIdx)
            {
                int listType = (m_nCurPrimType == D3DPT_LINESTRIP or
                                m_nCurPrimType == D3DPT_LINELIST) ?
                                   2 :
                                   4;
                g_pRenderer->DrawTLIndexed(
                    listType, (ScreenVertex *)&m_pVBCpu[m_dwStartVtx],
                    (int)m_dwNumVtx, m_pIdx, (int)m_dwNumIdx);
            }
            else
            {
                g_pRenderer->DrawTL(m_nCurPrimType,
                                    (ScreenVertex *)&m_pVBCpu[m_dwStartVtx],
                                    (int)m_dwNumVtx);
            }
        }

        // PHASE 5 FIX (heap corruption): in D3D11 DrawTL/DrawTLIndexed already copied the vertices
        // into the GPU VB (Map/memcpy/Draw synchronously), so the CPU buffer can be reused FROM
        // ZERO. Accumulating m_dwStartVtx += m_dwNumVtx pushed writes toward the m_pVBCpu[32768]
        // boundary and clobbered the header of the neighbouring m_pIdx block (free crash in
        // Cleanup, value ~0.3f = a vertex UV/color).
        m_dwStartVtx = 0;
        m_dwNumVtx = 0;
        m_dwNumIdx = 0;
        return;
    }
    // #34 dead D3D7 tail removed (D3D11 branch above returns).
}

// ASSO:
void ContextMPR::ZeroViewport()
{
    //ZeroMemory(&m_rcVP,sizeof(m_rcVP));
    m_rcVP.right = m_rcVP.left;
    m_rcVP.bottom = m_rcVP.top;
}

void ContextMPR::SetPrimitiveType(int nType)
{
#ifdef _CONTEXT_TRACE_ALL
    MonoPrint("ContextMPR::SetPrimitiveType(%d)\n", nType);
#endif

    if (m_nCurPrimType not_eq nType)
    {
        // Flush on changed primitive type
        FlushVB();
        m_nCurPrimType = nType;
    }
}

void ContextMPR::SetView(LPD3DMATRIX l_pMV)
{
    memcpy(&mV, l_pMV, sizeof(D3DMATRIX));
}

void ContextMPR::SetWorld(LPD3DMATRIX l_pMW)
{
    ShiAssert(mIdx < 4096);

    memcpy(&mW[mIdx++], l_pMW, sizeof(D3DMATRIX));
}

void ContextMPR::SetProjection(LPD3DMATRIX l_pMP)
{
    memcpy(&mP, l_pMP, sizeof(D3DMATRIX));
}

void ContextMPR::setGlobalZBias(float zBias)
{
    if (gZBias not_eq zBias)
        gZBias = zBias;

    ZCX_Calculate(); // COBRA - RED - Drawing CXs update
}

inline TLVERTEX *SPolygon::CopyToVertexBuffer(TLVERTEX *bufferPos)
{
    if (not bufferPos)
        return NULL;

    // COBRA - RED - Using arrays of TLVERTEX it is possible to copy directly into DX Buffer
    memcpy(bufferPos, pVertexList, sizeof(TLVERTEX) * numVertices);
    return bufferPos + numVertices;
}


// COBRA - RED - PolyZ is caclulated step by step on each Vertex stuff, saving time, so the average is calculated
// on the passed value
inline void SPolygon::CalcPolyZ(float Avg)
{
    Avg /= float(numVertices);
    zBuffer = FloatToInt32(Avg * 16777215.f);
}

/*inline*/ void ContextMPR::AllocatePolygon(SPolygon *&curPoly,
                                            const DWORD numVertices)
{
    curPoly =
        (SPolygon *)Alloc(sizeof(SPolygon) + numVertices * sizeof(TLVERTEX));
    curPoly->numVertices = numVertices;
    // Artscout - 2026 (x64): was `(TLVERTEX*)(DWORD(curPoly) + sizeof(SPolygon))` -- DWORD() truncated the
    // 64-bit curPoly to 32 bits, so pVertexList pointed at the LOW 32 bits of the pointer (e.g. 0x00FEE040)
    // -> the vertex fill in DrawPrimitive wrote into unmapped low memory -> CTD during terrain draw. Use
    // proper byte-pointer arithmetic so the full 64-bit address is preserved.
    curPoly->pVertexList = (TLVERTEX *)((char *)curPoly + sizeof(SPolygon));
}

inline void ContextMPR::AddPolygon(SPolygon *&polyList, SPolygon *&curPoly)
{
    curPoly->pNext = polyList;
    polyList = curPoly;
}


void ContextMPR::RenderPolyList(SPolygon *&pHead)
{
    TLVERTEX *pIns;
    SPolygon *pStart, *pEnd, *pCur;
    DWORD offset, vertcnt = 0, verttot = 0;

    // PHASE 4/#DX12: GPU mode - copy the polygons into m_pVBCpu and draw them one by one as DrawTL(TRIFAN)
    // with state/texture through g_pRenderer.
    if (g_bUseGpu)
    {
        if ((pHead->renderState >= STATE_ALPHA_SOLID) and
            (pHead->renderState <= STATE_ALPHA_TEXTURE_PERSPECTIVE_CLAMP))
        {
            offset = DWORD(&pHead->zBuffer) - DWORD(pHead);
            pHead =
                (SPolygon *)RadixSortDescending((radix_sort_t *)pHead, offset);
        }

        if (g_pRenderer and g_pRenderer->IsValid())
        {
            g_pRenderer->BeginScreenPass();

            DWORD base = 0;
            for (pCur = pHead; pCur not_eq NULL; pCur = pCur->pNext)
            {
                if (base + pCur->numVertices >= m_dwVBSize)
                    base = 0;
                memcpy(&m_pVBCpu[base], pCur->pVertexList,
                       sizeof(TLVERTEX) * pCur->numVertices);

                g_pRenderer->SetState(pCur->renderState);

                if ((pCur->renderState > STATE_GOURAUD and
                     pCur->renderState < STATE_ALPHA_SOLID) or
                    pCur->renderState > STATE_ALPHA_GOURAUD)
                    SetTexture1(pCur->textureID0);
                else
                    SetTexture1(-1);

                if (pCur->renderState >= STATE_MULTITEXTURE)
                    SetTexture2(pCur->textureID1);

                g_pRenderer->DrawTL(D3DPT_TRIANGLEFAN,
                                    (ScreenVertex *)&m_pVBCpu[base],
                                    (int)pCur->numVertices);
                base += pCur->numVertices;
            }
        }
        return;
    }
    // #34 dead D3D7 tail removed (D3D11 branch above returns).
}


void ContextMPR::DrawPoly(DWORD opFlag, Poly *poly, int *xyzIdxPtr,
                          int *rgbaIdxPtr, int *IIdxPtr, Ptexcoord *uv,
                          bool bUseFGColor)
{
    float *I;
    Spoint *xyz;
    Pcolor *rgba;
    TLVERTEX *pVtx = NULL;
    TLVERTEX *sVertex = NULL;
    SPolygon *sPolygon = NULL;
    float PolyZAvg = 0;

    // Incoming type is always MPR_PRM_TRIFAN
    ShiAssert(FALSE == F4IsBadReadPtr(poly, sizeof *poly));
    ShiAssert(poly->nVerts >= 3);
    ShiAssert(xyzIdxPtr);
    ShiAssert(not bUseFGColor or (bUseFGColor and rgbaIdxPtr == NULL));

#ifdef _CONTEXT_TRACE_ALL
    MonoPrint("ContextMPR::DrawPoly(0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,%s)\n",
              opFlag, poly, xyzIdxPtr, rgbaIdxPtr, IIdxPtr, uv,
              bUseFGColor ? "true" : "false");
#endif

#ifdef _CONTEXT_ENABLE_STATS
    m_stats.Primitive(D3DPT_TRIANGLEFAN, poly->nVerts);
#endif

    if (not bZBuffering)
    {
        // Lock VB
        if (not LockVB(poly->nVerts, (void **)&m_pTLVtx))
        {
            m_colFOG = 0xFFFFFFFF;
            return;
        }

        ShiAssert(FALSE == F4IsBadWritePtr(m_pTLVtx, sizeof *m_pTLVtx));
        ShiAssert(m_dwStartVtx < m_dwVBSize);
        pVtx = &m_pTLVtx[m_dwStartVtx + m_dwNumVtx];
        ShiAssert(FALSE == F4IsBadWritePtr(pVtx, poly->nVerts * sizeof *pVtx));

        // JB 011124 CTD
        if (not pVtx)
        {
            m_colFOG = 0xFFFFFFFF;
            return;
        }

        SetPrimitiveType(D3DPT_TRIANGLEFAN);
    }
    else
    {
        AllocatePolygon(sPolygon, poly->nVerts);
        sPolygon->renderState = currentState;
        sPolygon->textureID0 = currentTexture1;
        sPolygon->pNext = NULL;
        sVertex = sPolygon->pVertexList;
    }

    // Artscout - 2026: #44 view-dependent canopy reflection. palID==2 is the static painted glass
    // "reflection" overlay (was a flat 0x26 alpha -> looked stuck to the glass and rode the canopy
    // when it opened). Modulate its alpha by the facet's grazing angle to the eye so the glint shifts
    // with the view/head (VR) and reads as a real reflection. poly->A,B,C is the facet normal and
    // TheStateStack.ObjSpaceEye the eye, BOTH in the same object space (the BSP back-face cull uses
    // exactly these, bspnodes.cpp). cos^2 form avoids sqrt/fabs (FastMath sqrt-macro). Grazing
    // (normal ~perpendicular to the eye direction) -> brighter; head-on -> dimmer. Toggle CanopyReflect.
    extern bool g_bCanopyReflect;
    DWORD reflAlpha = 0x26000000; // legacy flat alpha (used only when palID==2)
    if (palID == 2 and g_bCanopyReflect)
    {
        float ex = TheStateStack.ObjSpaceEye.x;
        float ey = TheStateStack.ObjSpaceEye.y;
        float ez = TheStateStack.ObjSpaceEye.z;
        float nn = poly->A * poly->A + poly->B * poly->B + poly->C * poly->C;
        float ee = ex * ex + ey * ey + ez * ez;
        float ne = poly->A * ex + poly->B * ey + poly->C * ez;
        float denom = nn * ee;

        if (denom > 1e-6f)
        {
            float cos2 =
                (ne * ne) / denom; // cos^2(normal, eye direction), 0..1
            float graze = 1.0f - cos2; // 0 head-on .. 1 grazing
            int ai = (int)((0.06f + 0.30f * graze) * 255.0f +
                           0.5f); // ~0x10 .. ~0x5C
            if (ai < 0)
                ai = 0;
            else if (ai > 255)
                ai = 255;
            reflAlpha = (DWORD)ai << 24;
        }
    }

    // Iterate for each vertex
    if (not bZBuffering)
    {
        for (int i = 0; i < poly->nVerts; i++)
        {
            // Check for overrun
            ShiAssert((BYTE *)pVtx < m_pVtxEnd);

            xyz = &TheStateStack.XformedPosPool[*xyzIdxPtr++];


            if (DisplayOptions.bScreenCoordinateBiasFix) //Wombat778 4-01-04
            {
                pVtx->sx = xyz->x - 0.5f;
                pVtx->sy = xyz->y - 0.5f;
            }
            else
            {
                pVtx->sx = xyz->x;
                pVtx->sy = xyz->y;
            }

            // NOTE: HACK
            if (xyz->z > 5)
                pVtx->sz =
                    SCALE_SZ(xyz->z); // COBRA - RED - Using precomputed CXs
            else
                pVtx->sz = 0.f;

            pVtx->rhw = 1.f / xyz->z;
            pVtx->specular = m_colFOG;

            // End Mission box
            if (texID > 25 and texID < 32)
                pVtx->color = 0xFFFFFFFF;
            else if (OTWDriver.GetOTWDisplayMode() ==
                     OTWDriverClass::Mode3DCockpit)
            {
                // Cobra - unshaded 3D cockpit nodes (verts) need full intensity at night
                // Cobra - Added adjustable instrument/interior lighting in 3D pit
                // AARRGGBB
                if (TheTimeOfDay.GetLightLevel() > 0.5f)
                {
                    if (TheColorBank.PitLightLevel == 0)
                        pVtx->color = TheColorBank.TODcolor;
                    else if (TheColorBank.PitLightLevel == 1)
                        pVtx->color = 0xFF808080;
                    else
                        pVtx->color = 0xFFFFFFFF;
                }
                else
                {
                    DWORD src;

                    if (TheColorBank.PitLightLevel == 0)
                        pVtx->color = TheColorBank.TODcolor;
                    else if (TheColorBank.PitLightLevel == 1)
                    {
                        src = p3DpitLolite;
                        pVtx->color = (src bitand 0xFF000000) +
                                      ((src bitand 0x00FF0000) >> 16) +
                                      (src bitand 0x0000FF00) +
                                      ((src bitand 0x000000FF) << 16);
                    }
                    else
                    {
                        src = p3DpitHilite;
                        pVtx->color = (src bitand 0xFF000000) +
                                      ((src bitand 0x00FF0000) >> 16) +
                                      (src bitand 0x0000FF00) +
                                      ((src bitand 0x000000FF) << 16);
                    }
                }
            }
            else
                pVtx->color = TheColorBank.TODcolor;


            if (opFlag bitand PRIM_COLOP_COLOR)
            {
                ShiAssert(rgbaIdxPtr);
                rgba = &TheColorBank.ColorPool[*rgbaIdxPtr++];

                ShiAssert(rgba);

                if (rgba)
                {
                    if (opFlag bitand PRIM_COLOP_INTENSITY)
                    {
                        ShiAssert(IIdxPtr);
                        I = &TheStateStack.IntensityPool[*IIdxPtr++];
                        pVtx->color = D3DRGBA(rgba->r * *I, rgba->g * *I,
                                              rgba->b * *I, rgba->a);
                    }

                    else
                    {
                        pVtx->color =
                            D3DRGBA(rgba->r, rgba->g, rgba->b, rgba->a);
                    }
                }
            }
            else if (opFlag bitand PRIM_COLOP_INTENSITY)
            {
                ShiAssert(IIdxPtr);

                I = &TheStateStack.IntensityPool[*IIdxPtr++];
                pVtx->color = D3DRGBA(*I, *I, *I, 1.f);
            }
            else if (bUseFGColor)
                pVtx->color = m_colFG;
            else
            {
                // Set the light level for the "special building lights"
                if (palID == 3)
                    pVtx->color = TheColorBank.TODcolor;
                // Set the light level with "special cockpit reflection alpha"
                // Artscout - 2026: #44 alpha is now view-dependent (reflAlpha), keep TOD RGB.
                else if (palID == 2)
                    pVtx->color =
                        (TheColorBank.TODcolor bitand 0x00FFFFFF) bitor
                        reflAlpha;
            }

            if (opFlag bitand PRIM_COLOP_TEXTURE)
            {
                // NVG_LIGHT_LEVEL = 0.703125f
                if (NVGmode or TVmode or IRmode)
                {
                    // Artscout - 2026: NVG (pilot goggles) = green phosphor. TV (TGP) / IR (Maverick, FLIR)
                    // sensors are GRAYSCALE, not green -- green was the legacy CRT look (same as the old green
                    // MFD labels that are really white). Grey = luma of the vertex color (Rec.601 weights).
                    if (NVGmode)
                    {
                        pVtx->color and_eq 0xFF00FF00;
                        pVtx->color or_eq 0x0000B400;
                    }
                    else
                    {
                        DWORD c = pVtx->color;
                        DWORD lum = ((((c >> 16) bitand 0xFF) * 77) +
                                     (((c >> 8) bitand 0xFF) * 150) +
                                     ((c bitand 0xFF) * 29)) >>
                                    8;
                        pVtx->color = (c bitand 0xFF000000) bitor
                                      (lum << 16) bitor (lum << 8) bitor lum;
                    }
                }

                ShiAssert(uv);

                pVtx->tu0 = pVtx->tu1 = uv->u;
                pVtx->tv0 = pVtx->tv1 = uv->v;

                uv++;
            }
            else
            {
                pVtx->tu0 = 0;
                pVtx->tv0 = 0;
            }

            pVtx++;
        }
    }
    else
    {
        for (int i = 0; i < poly->nVerts; i++)
        {

            //********************************************************************************************************************
            // COBRA - RED - The following part of code is a tranformed in a direcct COPY from Statestack to sVertex of x,y,z

            /* xyz  = &TheStateStack.XformedPosPool[*xyzIdxPtr++];

             sVertex->sx = xyz->x;
             sVertex->sy = xyz->y;

             // NOTE: HACK
             if(xyz->z > 5)
             sVertex->sz = SCALE_SZ(xyz->z);
             else
             sVertex->sz = 0.f;

             sVertex->rhw = 1.f/xyz->z;
            */
            // COBRA - RED - New Version

            *(Spoint *)&(sVertex->sx) =
                *(Spoint *)&TheStateStack.XformedPosPool[*xyzIdxPtr++];
            sVertex->rhw = 1.f / sVertex->sz;

            // NOTE: HACK
            if (sVertex->sz > 5)
                sVertex->sz = SCALE_SZ(
                    sVertex->sz); // COBRA - RED - Using precomputed CXs;
            else
                sVertex->sz = 0.f;

            // COBRA - RED - End
            //********************************************************************************************************************
            sVertex->specular = m_colFOG;

            // End Mission box
            if (texID > 25 and texID < 32)
                sVertex->color = 0xFFFFFFFF;
            else
                sVertex->color = TheColorBank.TODcolor;

            if (opFlag bitand PRIM_COLOP_COLOR)
            {
                ShiAssert(rgbaIdxPtr);
                rgba = &TheColorBank.ColorPool[*rgbaIdxPtr++];

                ShiAssert(rgba);

                if (rgba)
                {
                    if (opFlag bitand PRIM_COLOP_INTENSITY)
                    {
                        ShiAssert(IIdxPtr);
                        I = &TheStateStack.IntensityPool[*IIdxPtr++];
                        sVertex->color = D3DRGBA(rgba->r * *I, rgba->g * *I,
                                                 rgba->b * *I, rgba->a);
                    }

                    else
                    {
                        sVertex->color =
                            D3DRGBA(rgba->r, rgba->g, rgba->b, rgba->a);
                    }
                }
            }
            else if (opFlag bitand PRIM_COLOP_INTENSITY)
            {
                ShiAssert(IIdxPtr);

                I = &TheStateStack.IntensityPool[*IIdxPtr++];
                sVertex->color = D3DRGBA(*I, *I, *I, 1.f);
            }
            else if (bUseFGColor)
                sVertex->color = m_colFG;
            else
            {
                // Set the light level for the "special building lights"
                if (palID == 3)
                    sVertex->color = TheColorBank.TODcolor;
                // Set the light level with "special cockpit reflection alpha"
                // Artscout - 2026: #44 alpha is now view-dependent (reflAlpha), keep TOD RGB.
                else if (palID == 2)
                    sVertex->color =
                        (TheColorBank.TODcolor bitand 0x00FFFFFF) bitor
                        reflAlpha;
            }

            if (opFlag bitand PRIM_COLOP_TEXTURE)
            {
                // NVG_LIGHT_LEVEL = 0.703125f
                if (NVGmode or TVmode or IRmode)
                {
                    // Artscout - 2026: NVG = green; TV (TGP) / IR (Maverick, FLIR) = GRAYSCALE (luma). See the
                    // twin block above -- green is the legacy CRT look; real sensor video is monochrome grey.
                    if (NVGmode)
                    {
                        sVertex->color and_eq 0xFF00FF00;
                        sVertex->color or_eq 0x0000B400;
                    }
                    else
                    {
                        DWORD c = sVertex->color;
                        DWORD lum = ((((c >> 16) bitand 0xFF) * 77) +
                                     (((c >> 8) bitand 0xFF) * 150) +
                                     ((c bitand 0xFF) * 29)) >>
                                    8;
                        sVertex->color = (c bitand 0xFF000000) bitor
                                         (lum << 16) bitor (lum << 8) bitor lum;
                    }
                }

                ShiAssert(uv);

                sVertex->tu0 = sVertex->tu1 = uv->u;
                sVertex->tv0 = sVertex->tv1 = uv->v;

                uv++;
            }
            else
            {
                sVertex->tu0 = 0;
                sVertex->tv0 = 0;
            }

            PolyZAvg +=
                sVertex
                    ->sz; // COBRA - RED - Poly Z Sum is calculated on the fly

            // COBRA - RED - No More Linking of vertexes, as single ARRAYS of TLVERTEX structures
            sVertex++;
        }
    }

    // Generate Indices
    if (not bZBuffering)
    {
        WORD *pIdx = &m_pIdx[m_dwNumIdx];

        for (int x = 0; x < poly->nVerts - 2; x++)
        {
            pIdx[0] = (WORD)m_dwNumVtx;
            pIdx[1] = (WORD)(m_dwNumVtx + x + 1);
            pIdx[2] = (WORD)(m_dwNumVtx + x + 2);
            pIdx += 3;
        }

        m_dwNumIdx += pIdx - &m_pIdx[m_dwNumIdx];
        m_dwNumVtx += poly->nVerts;

#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
        FlushVB();
#endif
    }
    else
    {
        // COBRA - RED - Here calculates the Average Z
        sPolygon->CalcPolyZ(PolyZAvg);

        // Double-textured
        if (sPolygon->renderState >= STATE_MULTITEXTURE)
        {
            texturedPolyVCnt += poly->nVerts;
            AddPolygon(texturedPolys, sPolygon);
        }
        // Translucent
        else if (sPolygon->renderState >= STATE_ALPHA_SOLID)
        {
            translucentPolyVCnt += poly->nVerts;
            AddPolygon(translucentPolys, sPolygon);
        }
        // Textured
        else if (sPolygon->renderState >= STATE_TEXTURE)
        {
            texturedPolyVCnt += poly->nVerts;
            AddPolygon(texturedPolys, sPolygon);
        }
        // Plain
        else if (sPolygon->renderState >= STATE_SOLID)
        {
            plainPolyVCnt += poly->nVerts;
            AddPolygon(plainPolys, sPolygon);
        }
        else
            INT3;
    }

    m_colFOG = 0xFFFFFFFF;
}

void ContextMPR::Draw2DPoint(Tpoint *v0)
{
    ShiAssert(v0);

    // COUNT_PROFILE("BSP POINTS");

#ifdef _CONTEXT_TRACE_ALL
    MonoPrint("ContextMPR::Draw2DPoint(0x%X)\n", v0);
#endif

    SetPrimitiveType(D3DPT_POINTLIST);

#ifdef _CONTEXT_ENABLE_STATS
    m_stats.Primitive(m_nCurPrimType, 1);
#endif

    // Lock VB
    TLVERTEX *pVtx;

    if (not LockVB(1, (void **)&m_pTLVtx))
    {
        m_colFOG = 0xFFFFFFFF;
        return;
    }

    ShiAssert(FALSE == F4IsBadWritePtr(m_pTLVtx, sizeof *m_pTLVtx));
    ShiAssert(m_dwStartVtx < m_dwVBSize);
    pVtx = &m_pTLVtx[m_dwStartVtx + m_dwNumVtx];
    ShiAssert(FALSE == F4IsBadWritePtr(pVtx, sizeof *pVtx));

    // Check for overrun
    ShiAssert((BYTE *)pVtx < m_pVtxEnd);

    if (DisplayOptions.bScreenCoordinateBiasFix) //Wombat778 4-01-04
    {
        pVtx->sx = v0->x - 0.5f;
        pVtx->sy = v0->y - 0.5f;
    }
    else
    {
        pVtx->sx = v0->x;
        pVtx->sy = v0->y;
    }

    if (v0->z)
        pVtx->sz = SCALE_SZ(v0->z); // COBRA - RED - Using precomputed CXs
    else
        pVtx->sz = 0.f;

    pVtx->rhw = 1.0f;
    pVtx->color = m_colFG;
    pVtx->specular = m_colFOG;
    pVtx->tu0 = 0;
    pVtx->tv0 = 0;

#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
    pVtx->color =
        currentState not_eq -1 ?
            RGBA_MAKE((currentState << 1) + 50, (currentState << 1) + 50,
                      (currentState << 1) + 50, (currentState << 1) + 50) :
            D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
#endif

    m_dwNumVtx++;

#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
    FlushVB();
#endif

    m_colFOG = 0xFFFFFFFF;
}

void ContextMPR::Draw2DPoint(float x, float y)
{
#ifdef _CONTEXT_TRACE_ALL
    MonoPrint("ContextMPR::Draw2DPoint(%f,%f)\n", x, y);
#endif

    // COUNT_PROFILE("POINTS");

    SetPrimitiveType(D3DPT_POINTLIST);

#ifdef _CONTEXT_ENABLE_STATS
    m_stats.Primitive(m_nCurPrimType, 1);
#endif

    // Lock VB
    TLVERTEX *pVtx;

    if (not LockVB(1, (void **)&m_pTLVtx))
    {
        m_colFOG = 0xFFFFFFFF;
        return;
    }

    ShiAssert(FALSE == F4IsBadWritePtr(m_pTLVtx, sizeof *m_pTLVtx));
    ShiAssert(m_dwStartVtx < m_dwVBSize);
    pVtx = &m_pTLVtx[m_dwStartVtx + m_dwNumVtx];

    // Check for overrun
    ShiAssert((BYTE *)pVtx < m_pVtxEnd);
    ShiAssert(FALSE == F4IsBadWritePtr(pVtx, sizeof *pVtx));

    if (DisplayOptions.bScreenCoordinateBiasFix) //Wombat778 4-01-04
    {
        pVtx->sx = x - 0.5f;
        pVtx->sy = y - 0.5f;
    }
    else
    {
        pVtx->sx = x;
        pVtx->sy = y;
    }

    pVtx->sz =
        m_2DPrimZ; // #48: reversed-Z near (1) for UI, far (0) for the sky background (see m_2DPrimZ set-points)
    pVtx->rhw = 1.0f;
    pVtx->color = m_colFG;
    pVtx->specular = m_colFOG;
    pVtx->tu0 = 0;
    pVtx->tv0 = 0;

#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
    pVtx->color =
        currentState not_eq -1 ?
            RGBA_MAKE((currentState << 1) + 50, (currentState << 1) + 50,
                      (currentState << 1) + 50, (currentState << 1) + 50) :
            D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
#endif

    m_dwNumVtx++;

#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
    FlushVB();
#endif

    m_colFOG = 0xFFFFFFFF;
}

void ContextMPR::Draw2DLine(Tpoint *v0, Tpoint *v1)
{
    ShiAssert(v0 and v1);

#ifdef _CONTEXT_TRACE_ALL
    MonoPrint("ContextMPR::Draw2DLine(0x%X,0x%X)\n", v0, v1);
#endif

    SetPrimitiveType(D3DPT_LINESTRIP);

#ifdef _CONTEXT_ENABLE_STATS
    m_stats.Primitive(m_nCurPrimType, 2);
#endif

    // Lock VB
    TLVERTEX *pVtx;

    if (not LockVB(2, (void **)&m_pTLVtx))
    {
        m_colFOG = 0xFFFFFFFF;
        return;
    }

    ShiAssert(FALSE == F4IsBadWritePtr(m_pTLVtx, sizeof *m_pTLVtx));
    ShiAssert(m_dwStartVtx < m_dwVBSize);
    pVtx = &m_pTLVtx[m_dwStartVtx + m_dwNumVtx];

    // Check for overrun
    ShiAssert((BYTE *)pVtx < m_pVtxEnd);
    ShiAssert(FALSE == F4IsBadWritePtr(pVtx, 2 * sizeof *pVtx));

    if (DisplayOptions.bScreenCoordinateBiasFix) //Wombat778 4-01-04
    {
        pVtx->sx = v0->x - 0.5f;
        pVtx->sy = v0->y - 0.5f;
    }
    else
    {
        pVtx->sx = v0->x;
        pVtx->sy = v0->y;
    }

    if (v0->z)
        pVtx->sz = SCALE_SZ(v0->z); // COBRA - RED - Using precomputed CXs
    else
        pVtx->sz = 0.f;

    // JB 010220 CTD
    if (v0->z)
        pVtx->rhw = 1.0f / v0->z;

    pVtx->color = m_colFG;
    pVtx->specular = m_colFOG;
    pVtx->tu0 = 0;
    pVtx->tv0 = 0;
    pVtx++;

#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
    pVtx->color =
        currentState not_eq -1 ?
            RGBA_MAKE((currentState << 1) + 50, (currentState << 1) + 50,
                      (currentState << 1) + 50, (currentState << 1) + 50) :
            D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
#endif

    if (DisplayOptions.bScreenCoordinateBiasFix) //Wombat778 4-01-04
    {
        pVtx->sx = v1->x - 0.5f;
        pVtx->sy = v1->y - 0.5f;
    }
    else
    {
        pVtx->sx = v1->x;
        pVtx->sy = v1->y;
    }

    if (v1->z)
        pVtx->sz =
            (ZFAR / (ZFAR - ZNEAR)) + (ZFAR * ZNEAR / (ZNEAR - ZFAR)) / v1->z;
    else
        pVtx->sz = 0.f;

    pVtx->rhw = 1.0f / v1->z;
    pVtx->color = m_colFG;
    pVtx->specular = m_colFOG;
    pVtx->tu0 = 0;
    pVtx->tv0 = 0;

#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
    pVtx->color =
        currentState not_eq -1 ?
            RGBA_MAKE((currentState << 1) + 50, (currentState << 1) + 50,
                      (currentState << 1) + 50, (currentState << 1) + 50) :
            D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
#endif

    WORD *pIdx = &m_pIdx[m_dwNumIdx];
    *pIdx++ = (WORD)m_dwNumVtx;
    *pIdx++ = (WORD)(m_dwNumVtx + 1);

    m_dwNumIdx += 2;
    m_dwNumVtx += 2;

#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
    FlushVB();
#endif

    m_colFOG = 0xFFFFFFFF;
}


void ContextMPR::Draw2DLine(float x0, float y0, float x1, float y1)
{
#ifdef _CONTEXT_TRACE_ALL
    MonoPrint("ContextMPR::Draw2DLine(0x%X,0x%X)\n", x0, y0);
#endif

    SetPrimitiveType(D3DPT_LINESTRIP);

#ifdef _CONTEXT_ENABLE_STATS
    m_stats.Primitive(m_nCurPrimType, 2);
#endif

    // Lock VB
    TLVERTEX *pVtx;

    if (not LockVB(2, (void **)&m_pTLVtx))
    {
        m_colFOG = 0xFFFFFFFF;
        return;
    }

    ShiAssert(FALSE == F4IsBadWritePtr(m_pTLVtx, sizeof *m_pTLVtx));
    ShiAssert(m_dwStartVtx < m_dwVBSize);
    pVtx = &m_pTLVtx[m_dwStartVtx + m_dwNumVtx];
    ShiAssert(FALSE == F4IsBadWritePtr(pVtx, 2 * sizeof *pVtx));

    // Check for overrun
    ShiAssert((BYTE *)pVtx < m_pVtxEnd);

    if (DisplayOptions.bScreenCoordinateBiasFix) //Wombat778 4-01-04
    {
        pVtx->sx = x0 - 0.5f;
        pVtx->sy = y0 - 0.5f;
    }
    else
    {
        pVtx->sx = x0;
        pVtx->sy = y0;
    }

    pVtx->sz =
        m_2DPrimZ; // #48: reversed-Z near (1) for UI, far (0) for the sky background (see m_2DPrimZ set-points)
    pVtx->rhw = 1.0f;
    pVtx->color = m_colFG;
    pVtx->specular = m_colFOG;
    pVtx->tu0 = 0;
    pVtx->tv0 = 0;
    pVtx++;

#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
    pVtx->color =
        currentState not_eq -1 ?
            RGBA_MAKE((currentState << 1) + 50, (currentState << 1) + 50,
                      (currentState << 1) + 50, (currentState << 1) + 50) :
            D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
#endif

    if (DisplayOptions.bScreenCoordinateBiasFix) //Wombat778 4-01-04
    {
        pVtx->sx = x1 - 0.5f;
        pVtx->sy = y1 - 0.5f;
    }
    else
    {
        pVtx->sx = x1;
        pVtx->sy = y1;
    }

    pVtx->sz =
        m_2DPrimZ; // #48: reversed-Z near (1) for UI, far (0) for the sky background (see m_2DPrimZ set-points)
    pVtx->rhw = 1.0f;
    pVtx->color = m_colFG;
    pVtx->specular = m_colFOG;
    pVtx->tu0 = 0;
    pVtx->tv0 = 0;

#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
    pVtx->color =
        currentState not_eq -1 ?
            RGBA_MAKE((currentState << 1) + 50, (currentState << 1) + 50,
                      (currentState << 1) + 50, (currentState << 1) + 50) :
            D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
#endif

    WORD *pIdx = &m_pIdx[m_dwNumIdx];
    *pIdx++ = (WORD)m_dwNumVtx;
    *pIdx++ = (WORD)(m_dwNumVtx + 1);

    m_dwNumIdx += 2;
    m_dwNumVtx += 2;

#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
    FlushVB();
#endif

    m_colFOG = 0xFFFFFFFF;
}

void ContextMPR::DrawPrimitive2D(int type, int nVerts, int *xyzIdxPtr)
{
    ShiAssert(xyzIdxPtr);

#ifdef _CONTEXT_TRACE_ALL
    MonoPrint("ContextMPR::DrawPrimitive2D(%d,%d,0x%X)\n", type, nVerts,
              xyzIdxPtr);
#endif

    SetPrimitiveType(type == LineF ? D3DPT_LINESTRIP : D3DPT_POINTLIST);

#ifdef _CONTEXT_ENABLE_STATS
    m_stats.Primitive(m_nCurPrimType, nVerts);
#endif

    // Lock VB
    TLVERTEX *pVtx;

    if (not LockVB(nVerts, (void **)&m_pTLVtx))
    {
        m_colFOG = 0xFFFFFFFF;
        return;
    }

    ShiAssert(FALSE == F4IsBadWritePtr(m_pTLVtx, sizeof *m_pTLVtx));
    ShiAssert(m_dwStartVtx < m_dwVBSize);
    pVtx = &m_pTLVtx[m_dwStartVtx + m_dwNumVtx];
    ShiAssert(FALSE == F4IsBadWritePtr(pVtx, nVerts * sizeof *pVtx));

    Ppoint *xyz;

    // Iterate for each vertex
    for (int i = 0; i < nVerts; i++)
    {
        ShiAssert(*xyzIdxPtr < MAX_VERT_POOL_SIZE);
        xyz = &TheStateStack.XformedPosPool[*xyzIdxPtr++];

        // Check for overrun
        ShiAssert((BYTE *)pVtx < m_pVtxEnd);

        if (DisplayOptions.bScreenCoordinateBiasFix) //Wombat778 4-01-04
        {
            pVtx->sx = xyz->x - 0.5f;
            pVtx->sy = xyz->y - 0.5f;
        }
        else
        {
            pVtx->sx = xyz->x;
            pVtx->sy = xyz->y;
        }

        if (xyz->z)
            pVtx->sz = SCALE_SZ(xyz->z); // COBRA - RED - Using precomputed CXs;
        else
            pVtx->sz = 0.f;

        // JB 010305
        if (xyz->z)
            pVtx->rhw = 1.0f / xyz->z;

        pVtx->color = m_colFG;
        pVtx->specular = m_colFOG;
        pVtx->tu0 = 0;
        pVtx->tv0 = 0;

#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
        pVtx->color =
            currentState not_eq -1 ?
                RGBA_MAKE((currentState << 1) + 50, (currentState << 1) + 50,
                          (currentState << 1) + 50, (currentState << 1) + 50) :
                D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
#endif

        pVtx++;
    }

    // Generate Indices
    if (m_nCurPrimType == D3DPT_LINESTRIP)
    {
        WORD *pIdx = &m_pIdx[m_dwNumIdx];

        for (int x = 0; x < nVerts; x++)
            *pIdx++ = (WORD)(m_dwNumVtx + x);

        m_dwNumIdx += nVerts;
    }

    m_dwNumVtx += nVerts;

#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
    FlushVB();
#endif

    m_colFOG = 0xFFFFFFFF;
}

void ContextMPR::DrawPrimitive(int nPrimType, WORD VtxInfo, WORD nVerts,
                               MPRVtx_t *pData, WORD Stride)
{
    // Impossible
    ShiAssert(not(VtxInfo bitand MPR_VI_COLOR));

    // Ensure no degenerate nPrimTypeitives
    ShiAssert((nVerts >= 3) or (nPrimType == MPR_PRM_POINTS and nVerts >= 1) or
              (nPrimType <= MPR_PRM_POLYLINE and nVerts >= 2));

#ifdef _CONTEXT_TRACE_ALL
    MonoPrint("ContextMPR::DrawPrimitive(%d,0x%X,%d,0x%X,%d)\n", nPrimType,
              VtxInfo, nVerts, pData, Stride);
#endif

    SetPrimitiveType(nPrimType);

#ifdef _CONTEXT_ENABLE_STATS
    m_stats.Primitive(m_nCurPrimType, nVerts);
#endif

    // Lock VB
    TLVERTEX *pVtx;

    if (not LockVB(nVerts, (void **)&m_pTLVtx))
    {
        m_colFOG = 0xFFFFFFFF;
        return;
    }

    ShiAssert(FALSE == F4IsBadWritePtr(m_pTLVtx, sizeof *m_pTLVtx));
    ShiAssert(m_dwStartVtx < m_dwVBSize);
    pVtx = &m_pTLVtx[m_dwStartVtx + m_dwNumVtx];
    ShiAssert(FALSE == F4IsBadWritePtr(pVtx, nVerts * sizeof *pVtx));

    // Iterate for each vertex
    for (int i = 0; i < nVerts; i++)
    {
        // Check for overrun
        ShiAssert((BYTE *)pVtx < m_pVtxEnd);

        if (DisplayOptions.bScreenCoordinateBiasFix) //Wombat778 4-01-04
        {

            pVtx->sx = pData->x - 0.5f;
            pVtx->sy = pData->y - 0.5f;
        }
        else
        {
            pVtx->sx = pData->x;
            pVtx->sy = pData->y;
        }

        pVtx->sz =
            m_2DPrimZ; // #48: reversed-Z near (1) for UI, far (0) for the sky background (see m_2DPrimZ set-points)
        pVtx->rhw = 1.0f;
        pVtx->color = m_colFG;
        pVtx->specular = m_colFOG;
        pVtx->tu0 = 0;
        pVtx->tv0 = 0;

#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
        pVtx->color =
            currentState not_eq -1 ?
                RGBA_MAKE((currentState << 1) + 50, (currentState << 1) + 50,
                          (currentState << 1) + 50, (currentState << 1) + 50) :
                D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
#endif

        pVtx++;
        pData = (MPRVtx_t *)((BYTE *)pData + Stride);
    }

    // Generate Indices
    if (m_nCurPrimType == D3DPT_TRIANGLEFAN)
    {
        WORD *pIdx = &m_pIdx[m_dwNumIdx];

        for (int x = 0; x < nVerts - 2; x++)
        {
            pIdx[0] = (WORD)m_dwNumVtx;
            pIdx[1] = (WORD)(m_dwNumVtx + x + 1);
            pIdx[2] = (WORD)(m_dwNumVtx + x + 2);
            pIdx += 3;
        }

        m_dwNumIdx += pIdx - &m_pIdx[m_dwNumIdx];
    }

    else if (m_nCurPrimType == D3DPT_LINESTRIP)
    {
        WORD *pIdx = &m_pIdx[m_dwNumIdx];

        for (int x = 0; x < nVerts; x++)
            *pIdx++ = (WORD)(m_dwNumVtx + x);

        m_dwNumIdx += nVerts;
    }

    m_dwNumVtx += nVerts;

#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
    FlushVB();
#endif

    m_colFOG = 0xFFFFFFFF;
}

void ContextMPR::DrawPrimitive(int nPrimType, WORD VtxInfo, WORD nVerts,
                               MPRVtxTexClr_t *pData, WORD Stride)
{
    TLVERTEX *pVtx;

    // Ensure no degenerate nPrimTypeitives
    ShiAssert((nVerts >= 3) or (nPrimType == MPR_PRM_POINTS and nVerts >= 1) or
              (nPrimType <= MPR_PRM_POLYLINE and nVerts >= 2));

#ifdef _CONTEXT_TRACE_ALL
    MonoPrint("ContextMPR::DrawPrimitive2(%d,0x%X,%d,0x%X,%d)\n", nPrimType,
              VtxInfo, nVerts, pData, Stride);
#endif

#ifdef _CONTEXT_ENABLE_STATS
    m_stats.Primitive(m_nCurPrimType, nVerts);
#endif

    // Lock VB
    if (not LockVB(nVerts, (void **)&m_pTLVtx))
    {
        m_colFOG = 0xFFFFFFFF;
        return;
    }

    ShiAssert(FALSE == F4IsBadWritePtr(m_pTLVtx, sizeof *m_pTLVtx));
    ShiAssert(m_dwStartVtx < m_dwVBSize);
    pVtx = &m_pTLVtx[m_dwStartVtx + m_dwNumVtx];

    ShiAssert(FALSE == F4IsBadWritePtr(pVtx, nVerts * sizeof *pVtx));

    // JB 011124 CTD
    if (not pVtx)
    {
        m_colFOG = 0xFFFFFFFF;
        return;
    }

    SetPrimitiveType(nPrimType);

    // Iterate for each vertex
    for (int i = 0; i < nVerts; i++)
    {
        // Check for overrun
        ShiAssert((BYTE *)pVtx < m_pVtxEnd);

        if (DisplayOptions.bScreenCoordinateBiasFix) //Wombat778 4-01-04
        {
            pVtx->sx = pData->x - 0.5f;
            pVtx->sy = pData->y - 0.5f;
        }
        else
        {
            pVtx->sx = pData->x;
            pVtx->sy = pData->y;
        }

        pVtx->sz =
            m_2DPrimZ; // #48: reversed-Z near (1) for UI, far (0) for the sky background (see m_2DPrimZ set-points)

        // OW FIXME: this should be 1.0f / pData->z
        pVtx->rhw = 1.0f;

        if (VtxInfo == (MPR_VI_COLOR bitor MPR_VI_TEXTURE))
        {
            pVtx->color = D3DRGBA(pData->r, pData->g, pData->b, pData->a);
            pVtx->specular = m_colFOG;

            pVtx->tu0 = pData->u;
            pVtx->tv0 = pData->v;
        }
        else if (VtxInfo == MPR_VI_COLOR)
        {
            pVtx->color = D3DRGBA(pData->r, pData->g, pData->b, pData->a);
            pVtx->specular = m_colFOG;
        }
        else
        {
            pVtx->color = m_colFG;
            pVtx->specular = m_colFOG;
        }

#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
        pVtx->color =
            currentState not_eq -1 ?
                RGBA_MAKE((currentState << 1) + 50, (currentState << 1) + 50, ,
                          (currentState << 1) + 50, (currentState << 1) + 50) :
                D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
#endif

        pVtx++;
        pData = (MPRVtxTexClr_t *)((BYTE *)pData + Stride);
    }

    // Generate Indices (in advance)
    if (m_nCurPrimType == D3DPT_TRIANGLEFAN)
    {
        WORD *pIdx = &m_pIdx[m_dwNumIdx];

        for (int x = 0; x < nVerts - 2; x++)
        {
            pIdx[0] = (WORD)m_dwNumVtx;
            pIdx[1] = (WORD)(m_dwNumVtx + x + 1);
            pIdx[2] = (WORD)(m_dwNumVtx + x + 2);
            pIdx += 3;
        }

        m_dwNumIdx += pIdx - &m_pIdx[m_dwNumIdx];
    }
    else if (m_nCurPrimType == D3DPT_LINESTRIP)
    {
        WORD *pIdx = &m_pIdx[m_dwNumIdx];

        for (int x = 0; x < nVerts; x++)
            *pIdx++ = (WORD)(m_dwNumVtx + x);

        m_dwNumIdx += nVerts;
    }

    m_dwNumVtx += nVerts;

#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
    FlushVB();
#endif

    m_colFOG = 0xFFFFFFFF;
}

void ContextMPR::DrawPrimitive(int nPrimType, WORD VtxInfo, WORD nVerts,
                               MPRVtxTexClr_t **pData, bool terrain)
{
    TLVERTEX *pVtx = NULL;
    TLVERTEX *sVertex = NULL;
    SPolygon *sPolygon = NULL;
    float PolyZAvg = 0;

    // Ensure no degenerate nPrimTypeitives
    ShiAssert((nVerts >= 3) or (nPrimType == MPR_PRM_POINTS and nVerts >= 1) or
              (nPrimType <= MPR_PRM_POLYLINE and nVerts >= 2));

#ifdef _CONTEXT_TRACE_ALL
    MonoPrint("ContextMPR::DrawPrimitive3(%d,0x%X,%d,0x%X)\n", nPrimType,
              VtxInfo, nVerts, pData);
#endif

#ifdef _CONTEXT_ENABLE_STATS
    m_stats.Primitive(m_nCurPrimType, nVerts);
#endif

    if (not bZBuffering)
    {
        // Lock VB
        if (not LockVB(nVerts, (void **)&m_pTLVtx))
        {
            m_colFOG = 0xFFFFFFFF;
            return;
        }

        ShiAssert(FALSE == F4IsBadWritePtr(m_pTLVtx, sizeof *m_pTLVtx));
        ShiAssert(m_dwStartVtx < m_dwVBSize);
        pVtx = &m_pTLVtx[m_dwStartVtx + m_dwNumVtx];

        ShiAssert(FALSE == F4IsBadWritePtr(pVtx, nVerts * sizeof *pVtx));

        // JB 011124 CTD
        if (not pVtx)
        {
            m_colFOG = 0xFFFFFFFF;
            return;
        }

        SetPrimitiveType(nPrimType);
    }
    else
    {
        AllocatePolygon(sPolygon, nVerts);
        sPolygon->renderState = currentState;
        sPolygon->textureID0 = currentTexture1;

        if (currentState >= STATE_MULTITEXTURE)
            sPolygon->textureID1 = currentTexture2;
        else
            sPolygon->textureID1 = -1;

        sPolygon->pNext = NULL;
        sVertex = sPolygon->pVertexList;
    }

    if (not bZBuffering)
    {
        // Iterate for each vertex
        for (int i = 0; i < nVerts; i++)
        {
            // Check for overrun
            ShiAssert((BYTE *)pVtx < m_pVtxEnd);

            // JB 010712 CTD second try
            if (not pData[i])
                break;

            if (DisplayOptions.bScreenCoordinateBiasFix)
            {
                pVtx->sx = pData[i]->x - 0.5f;
                pVtx->sy = pData[i]->y - 0.5f;
            }
            else
            {
                pVtx->sx = pData[i]->x;
                pVtx->sy = pData[i]->y;
            }

            // NOTE: HACK -- reversed-Z: 0.0 = far plane (was 1.0 under standard Z). 2D screen prims that don't
            // depth-test ignore it; ones that do now sit at the far plane as intended.
            pVtx->sz = 0.0f;
            pVtx->rhw =
                pData[i]->q > 0.0f ? 1.0f / (pData[i]->q / Q_SCALE) : 1.0f;

            if (terrain)
            {
                if (VtxInfo bitand MPR_VI_COLOR)
                    pVtx->color =
                        D3DRGBA(pData[i]->r, pData[i]->g, pData[i]->b, 1.f);

                pVtx->specular =
                    (min(255, FloatToInt32(pData[i]->a * 255.f)) << 24) +
                    0xFFFFFF;
            }
            else
            {
                if (VtxInfo bitand MPR_VI_COLOR)
                    pVtx->color = D3DRGBA(pData[i]->r, pData[i]->g, pData[i]->b,
                                          pData[i]->a);

                pVtx->specular = m_colFOG;
            }

            if (VtxInfo bitand MPR_VI_TEXTURE)
            {
                if (terrain)
                {
                    if (DisplayOptions.m_texMode ==
                        DisplayOptionsClass::TEX_MODE_DDS)
                    {
                        // Tex coords for night texture
                        pVtx->tu1 = pData[i]->u;
                        pVtx->tv1 = pData[i]->v;
                    }
                }

                pVtx->tu0 = pData[i]->u;
                pVtx->tv0 = pData[i]->v;
            }

#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
            pVtx->color = currentState not_eq -1 ?
                              RGBA_MAKE((currentState << 1) + 50,
                                        (currentState << 1) + 50,
                                        (currentState << 1) + 50,
                                        (currentState << 1) + 50) :
                              D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
#endif

            pVtx++;
        }
    }
    else
    {
        // COBRA - RED - These are to be calculated ONCE for poly, not for Vertex
        float gzNear, gCX1, gCX2;

        if (terrain)
            gzNear = ZNEAR - .02f;
        else
            gzNear = ZNEAR;

        gCX1 = (ZFAR / (ZFAR - gzNear));
        gCX2 = (ZFAR * gzNear / (gzNear - ZFAR));
        // COBRA - RED -End2


        // Iterate for each vertex
        for (int i = 0; i < nVerts; i++)
        {
            // JB 010712 CTD
            if (not pData[i])
                break;

            sVertex->sx = pData[i]->x;
            sVertex->sy = pData[i]->y;

            // NOTE: HACK
            if (pData[i]->q)
                sVertex->sz = gCX1 + gCX2 / (pData[i]->q / Q_SCALE);
            else
                sVertex->sz = 0.f;

            sVertex->rhw =
                pData[i]->q > 0.0f ? 1.0f / (pData[i]->q / Q_SCALE) : 1.0f;

            if (terrain)
            {
                if (VtxInfo bitand MPR_VI_COLOR)
                    sVertex->color =
                        D3DRGBA(pData[i]->r, pData[i]->g, pData[i]->b, 1.f);

                sVertex->specular =
                    (min(255, FloatToInt32(pData[i]->a * 255.f)) << 24) +
                    0xFFFFFF;
            }
            else
            {
                if (VtxInfo bitand MPR_VI_COLOR)
                    sVertex->color = D3DRGBA(pData[i]->r, pData[i]->g,
                                             pData[i]->b, pData[i]->a);

                sVertex->specular = m_colFOG;
            }

            if (VtxInfo bitand MPR_VI_TEXTURE)
            {
                if (terrain)
                {
                    if (DisplayOptions.m_texMode ==
                        DisplayOptionsClass::TEX_MODE_DDS)
                    {
                        sVertex->tu1 = pData[i]->u;
                        sVertex->tv1 = pData[i]->v;
                    }
                }

                sVertex->tu0 = pData[i]->u;
                sVertex->tv0 = pData[i]->v;
            }

#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
            sVertex->color = currentState not_eq -1 ?
                                 RGBA_MAKE((currentState << 1) + 50,
                                           (currentState << 1) + 50,
                                           (currentState << 1) + 50,
                                           (currentState << 1) + 50) :
                                 D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
#endif
            PolyZAvg +=
                sVertex->sz; // COBRA - RED - Poly Z Sum is calculated onthe fly

            // COBRA - RED - No More Linking of vertexes, as single ARRAYS of TLVERTEX structures
            sVertex++;
        }
    }

    // Generate Indices
    if (not bZBuffering)
    {
        if (m_nCurPrimType == D3DPT_TRIANGLEFAN)
        {
            WORD *pIdx = &m_pIdx[m_dwNumIdx];

            for (int x = 0; x < nVerts - 2; x++)
            {
                pIdx[0] = (WORD)m_dwNumVtx;
                pIdx[1] = (WORD)(m_dwNumVtx + x + 1);
                pIdx[2] = (WORD)(m_dwNumVtx + x + 2);
                pIdx += 3;
            }

            m_dwNumIdx += pIdx - &m_pIdx[m_dwNumIdx];
        }
        else if (m_nCurPrimType == D3DPT_LINESTRIP)
        {
            WORD *pIdx = &m_pIdx[m_dwNumIdx];

            for (int x = 0; x < nVerts; x++)
                *pIdx++ = (WORD)(m_dwNumVtx + x);

            m_dwNumIdx += nVerts;
        }

        m_dwNumVtx += nVerts;

#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
        FlushVB();
#endif
    }
    else
    {
        // COBRA - RED - Here calculates the Average Z
        sPolygon->CalcPolyZ(PolyZAvg);

        // Double-textured
        if (sPolygon->renderState >= STATE_MULTITEXTURE)
        {
            texturedPolyVCnt += nVerts;
            AddPolygon(texturedPolys, sPolygon);
        }
        // Translucent
        else if (sPolygon->renderState >= STATE_ALPHA_SOLID)
        {
            translucentPolyVCnt += nVerts;
            AddPolygon(translucentPolys, sPolygon);
        }
        // Textured
        else if (sPolygon->renderState >= STATE_TEXTURE)
        {
            texturedPolyVCnt += nVerts;
            AddPolygon(texturedPolys, sPolygon);
        }
        // Plain
        else if (sPolygon->renderState >= STATE_SOLID)
        {
            plainPolyVCnt += nVerts;
            AddPolygon(plainPolys, sPolygon);
        }
        else
            INT3;
    }

    m_colFOG = 0xFFFFFFFF;
}

ContextMPR::Stats::Stats()
{
#ifdef _CONTEXT_ENABLE_STATS
    Init();
#endif
}

#ifdef _CONTEXT_ENABLE_STATS
void ContextMPR::Stats::Check()
{
    DWORD Ticks = ::GetTickCount();

    if (Ticks - dwTicks > 1000)
    {
        dwTicks = Ticks;
        dwLastFPS = dwCurrentFPS;
        dwCurrentFPS = 0;

        if (dwCurPrimCountPerSecond > dwMaxPrimCountPerSecond)
            dwMaxPrimCountPerSecond = dwCurPrimCountPerSecond;

        if (dwCurVtxCountPerSecond > dwMaxVtxCountPerSecond)
            dwMaxVtxCountPerSecond = dwCurVtxCountPerSecond;

        if (dwTotalSeconds)
        {
            dwAvgVtxCountPerSecond = (WORD)dwTotalVtxCount / dwTotalSeconds;
            dwAvgPrimCountPerSecond = (WORD)dwTotalPrimCount / dwTotalSeconds;
        }

        if (dwTotalBatches)
        {
            dwAvgVtxBatchSize = (WORD)dwTotalVtxBatchSize / dwTotalBatches;
            dwAvgPrimBatchSize = (WORD)dwTotalPrimBatchSize / dwTotalBatches;
        }

        if (dwLastFPS < dwMinFPS)
            dwMinFPS = dwLastFPS;
        else if (dwLastFPS > dwMaxFPS)
            dwMaxFPS = dwLastFPS;

        dwTotalFPS += dwLastFPS;
        dwTotalSeconds++;
        dwAverageFPS = dwTotalFPS / dwTotalSeconds;
    }
}

void ContextMPR::Stats::Init()
{
    ZeroMemory(this, sizeof(*this));
}

void ContextMPR::Stats::StartFrame()
{
    dwCurrentFPS++;
    Check();
}

void ContextMPR::Stats::StartBatch()
{
    dwTotalBatches++;

    if (dwCurVtxBatchSize > dwMaxVtxBatchSize)
        dwMaxVtxBatchSize = dwCurVtxBatchSize;

    dwTotalVtxBatchSize += dwCurVtxBatchSize;
    dwCurVtxBatchSize = 0;

    if (dwCurPrimBatchSize > dwMaxPrimBatchSize)
        dwMaxPrimBatchSize = dwCurPrimBatchSize;

    dwTotalPrimBatchSize += dwCurPrimBatchSize;
    dwCurPrimBatchSize = 0;
}

void ContextMPR::Stats::Primitive(DWORD dwType, DWORD dwNumVtx)
{
    // Artscout - 2026 (x64): bounds-guard. dwType is m_nCurPrimType, which is 0 until
    // BeginPrimitive sets it (1..6). On the text path (ScreenText) it can still be 0, so
    // dwType-1 underflows to 0xFFFFFFFF -> arrPrimitives[~16GB]. On x86 the index wrapped
    // mod 2^32 to base-4 (silent neighbour corruption); on x64 there is no wrap -> fault.
    if (dwType >= 1 and dwType <= 6)
        arrPrimitives[dwType - 1]++;
    dwTotalPrimitives++;
    dwCurPrimCountPerSecond++;
    dwCurVtxCountPerSecond += dwNumVtx;
    dwCurVtxBatchSize += dwNumVtx;
    dwCurPrimBatchSize++;
    dwTotalPrimCount++;
    dwTotalVtxCount += dwNumVtx;
}

void ContextMPR::Stats::PutTexture(bool bCached)
{
    dwPutTextureTotal++;

    if (bCached)
        dwPutTextureCached++;
}

void ContextMPR::Stats::Report()
{
    MonoPrint("Stats report follows\n");

    float fT = dwTotalPrimitives / 100.0f;

    MonoPrint(" MinFPS: %d\n", dwMinFPS);
    MonoPrint(" MaxFPS: %d\n", dwMaxFPS);
    MonoPrint(" AverageFPS: %d\n", dwAverageFPS);
    MonoPrint(" TotalPrimitives: %d\n", dwTotalPrimitives);
    MonoPrint(" Triangle Lists: %d (%.2f %%)\n", arrPrimitives[3],
              arrPrimitives[3] / fT);
    MonoPrint(" Triangle Strips: %d (%.2f %%)\n", arrPrimitives[4],
              arrPrimitives[4] / fT);
    MonoPrint(" Triangle Fans: %d (%.2f %%)\n", arrPrimitives[5],
              arrPrimitives[5] / fT);
    MonoPrint(" Point Lists: %d (%.2f %%)\n", arrPrimitives[0],
              arrPrimitives[0] / fT);
    MonoPrint(" Line Lists: %d (%.2f %%)\n", arrPrimitives[1],
              arrPrimitives[1] / fT);
    MonoPrint(" Line Strips: %d (%.2f %%)\n", arrPrimitives[2],
              arrPrimitives[2] / fT);
    MonoPrint(" AvgVtxBatchSize: %d\n", dwAvgVtxBatchSize);
    MonoPrint(" MaxVtxBatchSize: %d\n", dwMaxVtxBatchSize);
    MonoPrint(" AvgPrimBatchSize: %d\n", dwAvgPrimBatchSize);
    MonoPrint(" MaxPrimBatchSize: %d\n", dwMaxPrimBatchSize);
    MonoPrint(" AvgVtxCountPerSecond: %d\n", dwAvgVtxCountPerSecond);
    MonoPrint(" MaxVtxCountPerSecond: %d\n", dwMaxVtxCountPerSecond);
    MonoPrint(" AvgPrimCountPerSecond: %d\n", dwAvgPrimCountPerSecond);
    MonoPrint(" MaxPrimCountPerSecond: %d\n", dwMaxPrimCountPerSecond);
    MonoPrint(" TextureChangesTotal: %d\n", dwPutTextureTotal);
    MonoPrint(" TextureChangesCached: %d (%.2f %%)\n", dwPutTextureCached,
              (float)dwPutTextureCached / (dwPutTextureTotal / 100.0f));

    MonoPrint("End of stats report\n");
}

#endif
