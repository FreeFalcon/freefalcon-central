/***************************************************************************\
    DevMgr.cpp
    Scott Randolph
    November 8, 1996

    This class provides management of the drawing devices in the system.
\***************************************************************************/
#include "stdafx.h"
#include "devmgr.h"
#include "falclib/include/playerop.h"

#include "falclib/include/dispopts.h" //JAM 04Oct03

#include <math.h>
#include "polylib.h"
#include "graphics/dxengine/dxengine.h"
#include "graphics/dxengine/d3d12backend.h" // Artscout - 2026: #DX12 Phase 1
#include "graphics/dxengine/d3d12/d3d12renderer.h" // Artscout - 2026: #DX12 Phase 3
#include "graphics/dxengine/d3d12/d3d12texturemanager.h" // Artscout - 2026: #DX12 п.1
#include "graphics/dxengine/common/irenderer.h" // PHASE 4
#include "graphics/dxengine/d3d12/d3d12texturemanager.h" // PHASE 3
#include "graphics/dxengine/dxvbmanager.h" // PHASE 4: TheVbManager.Setup
#include "graphics/dxengine/openxrbackend.h" // VR (OpenXR)
#include "graphics/vulkan/vulkanbackend.h" // Artscout - 2026 (#104): runtime-selectable Vulkan backend (Win+Linux)
#include "graphics/vulkan/vulkanrenderer.h"
#include "graphics/vulkan/vulkantexturemanager.h" // #104: g_pVulkanTextureManager (engine Tex.cpp peer)
#include "graphics/vulkan/vulkanvbmanager.h" // #104: g_pVulkanVbManager (per-model VB peer)
#ifdef _WIN32
#include <dxgi.h> // Artscout - 2026 (#89): DXGI adapter enumeration for the GPU selector
#endif
extern bool g_bUseOpenXR;
int g_d3d11ReqWidth = 0;
int g_d3d11ReqHeight = 0;
int g_d3d11ReqDepth = 32;
extern bool g_bUse_DX_Engine;

typedef std::vector<DDPIXELFORMAT> PIXELFMT_ARRAY;

int HighResolutionHackFlag = FALSE; // Used in WinMain.CPP
extern bool g_bForceDXMultiThreadedCoopLevel;
extern char g_CardDetails[]; // JB 010215

#define INT3                                                                   \
    __debugbreak() // Artscout - 2026 (x64): int 3 intrinsic, builds on x86+x64

// Cobra - Hack to get VC6 to link
#if defined(_MSC_VER) &&                                                       \
    _MSC_VER <                                                                 \
        1300 // guard against non-MSVC (_MSC_VER undefined == 0 would enable this)

void __cdecl std::_Xlen()
{
}
void __cdecl std::_Xran()
{
}

#endif

// Artscout - 2026: [DX7-PURGE] the DirectDraw create/enumerate function-pointer globals
// (LPDIRECTDRAWENUMERATEEX/LPDIRECTDRAWCREATEEX) were unused after the DDraw enum removal.

// Device GUIDs
struct __declspec(uuid("D7B71CFA-4342-11CF-CE67-0120A6C2C935"))
DEVGUID_3DFX_VOODOO2_a; // DX7 Beta Driver
struct __declspec(uuid(
    "472BEA00-40DF-11D1-A9DF-006097C2EDB2")) DEVGUID_3DFX_VOODOO2_b; // DX7

void DeviceManager::Setup(int languageNum)
{
    try
    {
        CheckHR(D3DXInitialize());
        EnumDDDrivers(this);

        ready = TRUE;
    }

    catch (const _com_error &e)
    {
        MonoPrint("DeviceManager::Setup - Error 0x%X\n", e.Error());
    }
}


void DeviceManager::Cleanup(void)
{
    if (ready)
        D3DXUninitialize();

    ready = FALSE;
}


const char *DeviceManager::GetDriverName(int driverNum)
{
    if (driverNum < 0 or driverNum >= (int)m_arrDDDrivers.size())
        return NULL;

    return m_arrDDDrivers[driverNum].GetName();
}


const char *DeviceManager::GetDeviceName(int driverNum, int devNum)
{
    if (driverNum < 0 or driverNum >= (int)m_arrDDDrivers.size())
        return NULL;

    return m_arrDDDrivers[driverNum].GetDeviceName(devNum);
}

int DeviceManager::FindPrimaryDisplayDriver()
{
#ifdef _WIN32
    for (int i = 0; i < (int)m_arrDDDrivers.size(); i++)
        if (IsEqualGUID(m_arrDDDrivers[i].m_guid, GUID_NULL))
            return i;
#endif
    return -1; // Linux: the DDraw driver list is always empty (no DirectDraw) -> no primary DDraw driver
}

const char *DeviceManager::GetModeName(int driverNum, int devNum, int modeNum)
{
    static char buffer[80];
    int i = 0;

    if (driverNum < 0 or driverNum >= (int)m_arrDDDrivers.size())
        return NULL;

    DDDriverInfo &DI = m_arrDDDrivers[driverNum];
    LPDDSURFACEDESC2 pddsd = NULL;

    // Find the nth (legal) display mode
    while (pddsd = DI.GetDisplayMode(i))
    {
        // For now we only allow 640x480, 800x600, 1280x960, 1600x1200
        // (MPR already does the 4:3 aspect ratio check for us)
        if (pddsd->ddpfPixelFormat.dwRGBBitCount >= 16 and
            (pddsd->dwWidth == 640 or pddsd->dwWidth == 800 or
             pddsd->dwWidth == 1024 or
             (pddsd->dwWidth == 1280 and pddsd->dwHeight == 960) or
             pddsd->dwWidth == 1600 or HighResolutionHackFlag))
        {
            if (modeNum == 0)
            {
                // This is the one we want.  Return it.
                // OW
                // sprintf( buffer, "%0dx%0d", pddsd->dwWidth, pddsd->dwHeight);
                sprintf(buffer, "%0dx%0d - %d Bit", pddsd->dwWidth,
                        pddsd->dwHeight, pddsd->ddpfPixelFormat.dwRGBBitCount);
                return buffer;
            }

            else
            {
                // One down, more to go...
                modeNum--;
            }
        }

        i++;
    }

    return NULL;
}

// PHASE 5: a curated resolution list for D3D11 (bypass the DDraw enum, provide our own
// list with widescreen modes). modeNum index = index into this table. Depth is 32-bit.
// Artscout - 2026 (#104): API-neutral display-mode table (was g_d3d11Modes). Not static -> accessible from other
// TUs via the extern declarations in devmgr.h (the resolution UI + backends share it, DX12 and Vulkan alike).
const DisplayModeEntry g_DisplayModes[] = {
    {640, 480}, // 4:3 legacy
    {800, 600}, // 4:3 legacy
    {1024, 768}, // 4:3 legacy
    {1280, 1024}, // 5:4
    {1280, 720}, // 720p  (16:9)
    {1600, 900}, // 16:9
    {1920, 1080}, // 1080p (16:9) -- 3D DEFAULT
    {2560, 1440}, // 2K / QHD (16:9)
    {3840, 2160}, // 4K / UHD (16:9)
};
const int g_nDisplayModes =
    (int)(sizeof(g_DisplayModes) / sizeof(g_DisplayModes[0]));

// Artscout - 2026 (#89): DXGI hardware-adapter enumeration for the GPU selector. HARDWARE adapters only
// (DXGI_ADAPTER_FLAG_SOFTWARE skipped) so the "video card" combo index maps 1:1 to the backend's adapter
// pick. Names cached on first call (adapters don't change during a session); GetDxgiAdapter re-enumerates
// to hand the backend a live IDXGIAdapter1* at device-create time (same skip/order -> same index).
#define DXGI_MAX_ADAPTERS 16
static bool s_dxgiAdaptersInit = false;
static int s_dxgiAdapterCount = 0;
static char s_dxgiAdapterNames[DXGI_MAX_ADAPTERS][256];

#ifdef _WIN32
static void EnsureDxgiAdapters()
{
    if (s_dxgiAdaptersInit)
        return;
    s_dxgiAdaptersInit = true;
    s_dxgiAdapterCount = 0;

    IDXGIFactory1 *pFactory = NULL;
    if (FAILED(
            CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void **)&pFactory)) or
        not pFactory)
        return;

    IDXGIAdapter1 *pAdapter = NULL;
    for (UINT i = 0;
         s_dxgiAdapterCount < DXGI_MAX_ADAPTERS and
         pFactory->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND;
         ++i)
    {
        DXGI_ADAPTER_DESC1 desc;
        if (SUCCEEDED(pAdapter->GetDesc1(&desc)) and
            not(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
        {
            WideCharToMultiByte(CP_ACP, 0, desc.Description, -1,
                                s_dxgiAdapterNames[s_dxgiAdapterCount], 256,
                                NULL, NULL);
            s_dxgiAdapterNames[s_dxgiAdapterCount][255] = 0;
            s_dxgiAdapterCount++;
        }
        pAdapter->Release();
        pAdapter = NULL;
    }
    pFactory->Release();
}
#else
// Linux: DXGI does not exist -> enumerate GPUs via Vulkan (vulkanbackend.cpp) so the "video card" selector
// shows real device names. Cached in the same static table; index maps 1:1 to a Vulkan physical device.
extern int VulkanEnumerateGpuNames(char (*out)[256], int maxN);
static void EnsureDxgiAdapters()
{
    if (s_dxgiAdaptersInit)
        return;
    s_dxgiAdaptersInit = true;
    s_dxgiAdapterCount =
        VulkanEnumerateGpuNames(s_dxgiAdapterNames, DXGI_MAX_ADAPTERS);
}
#endif

int DeviceManager::GetDxgiAdapterCount()
{
    EnsureDxgiAdapters();
    return s_dxgiAdapterCount;
}

bool DeviceManager::GetDxgiAdapterName(int index, char *buf, int bufLen)
{
    EnsureDxgiAdapters();
    if (not buf or bufLen <= 0 or index < 0 or index >= s_dxgiAdapterCount)
        return false;
    strncpy(buf, s_dxgiAdapterNames[index], bufLen - 1);
    buf[bufLen - 1] = 0;
    return true;
}

// Artscout - 2026 (#89): the chosen adapter index (DisplayOptions.DispVideoCard), set by DXContext::Init
// before the backend comes up. GetSelectedDxgiAdapter() is a free function so the backends can pull the
// chosen IDXGIAdapter1* WITHOUT pulling in devmgr.h/dispopts.h (just forward-declare + extern the fn).
int g_nDispVideoCard = 0;

#ifdef _WIN32
IDXGIAdapter1 *GetSelectedDxgiAdapter()
{
    return DeviceManager::GetDxgiAdapter(g_nDispVideoCard);
}

IDXGIAdapter1 *DeviceManager::GetDxgiAdapter(int index)
{
    if (index < 0)
        return NULL;

    IDXGIFactory1 *pFactory = NULL;
    if (FAILED(
            CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void **)&pFactory)) or
        not pFactory)
        return NULL;

    IDXGIAdapter1 *pAdapter = NULL;
    int hw = 0;
    for (UINT i = 0;
         pFactory->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC1 desc;
        if (SUCCEEDED(pAdapter->GetDesc1(&desc)) and
            not(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
        {
            if (hw == index)
            {
                pFactory->Release();
                return pAdapter;
            } // keep the ref for the caller
            hw++;
        }
        pAdapter->Release();
        pAdapter = NULL;
    }
    pFactory->Release();
    return NULL;
}
#endif // _WIN32 (DXGI adapter object for the D3D12 device pick; Vulkan selects its own physical device)

bool DeviceManager::GetMode(int driverNum, int devNum, int modeNum,
                            UINT *pWidth, UINT *pHeight, UINT *pDepth)
{
    static char buffer[80];
    int i = 0;

    // #DX12/#104: the resolution table is API-neutral -- return it for ANY modern GPU backend (D3D12 OR Vulkan).
    // Gating on g_bUseD3D12 broke Vulkan: DXContext::Init clears g_bUseD3D12 when Vulkan owns the frame, so this
    // fell through to the dead DDraw path (empty driver list) -> every mode failed -> "unavailable resolution".
    {
        extern bool g_bUseGpu;
        if (g_bUseGpu)
        {
            if (modeNum < 0 or modeNum >= g_nDisplayModes)
                return false;
            *pWidth = g_DisplayModes[modeNum].w;
            *pHeight = g_DisplayModes[modeNum].h;
            *pDepth = 32;
            return true;
        }
    }

    if (driverNum < 0 or driverNum >= (int)m_arrDDDrivers.size())
        return false;

    DDDriverInfo &DI = m_arrDDDrivers[driverNum];
    LPDDSURFACEDESC2 pddsd = DI.GetDisplayMode(modeNum);

    if (not pddsd)
        return false;

    *pWidth = pddsd->dwWidth;
    *pHeight = pddsd->dwHeight;
    *pDepth = pddsd->ddpfPixelFormat.dwRGBBitCount; // OW

    return true;
}

// Present the user with a dialog box listing the available devices and pick one
#ifdef _WIN32
BOOL DeviceManager::ChooseDevice(int *usrDrvNum, int *usrDevNum, int *usrWidth)
{
    RECT rect;
    HWND listWin;
    DWORD listSlot;
    const char *devName;
    const char *drvName;
    const char *modeName;
    char name[MAX_PATH];
    unsigned devNum;
    unsigned drvNum;
    unsigned modeNum;
    unsigned width;
    unsigned height;
    unsigned packedNum;

    // Build a window for this application
    rect.top = rect.left = 0;
    rect.right = 200;
    rect.bottom = 400;
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    listWin = CreateWindow("LISTBOX", /* class */
                           "Choose Display Device", /* caption */
                           WS_OVERLAPPEDWINDOW, /* style */
                           CW_USEDEFAULT, /* init. x pos */
                           CW_USEDEFAULT, /* init. y pos */
                           rect.right - rect.left, /* init. x size */
                           rect.bottom - rect.top, /* init. y size */
                           NULL, /* parent window */
                           NULL, /* menu handle */
                           NULL, /* program handle */
                           NULL /* create parms */
    );

    if (not listWin)
    {
        ShiError("Failed to construct list box window");
    }

    // Fill in the list box with the avaiable devices
    drvNum = 0;

    while (drvName = GetDriverName(drvNum))
    {

        devNum = 0;

        while (devName = GetDeviceName(drvNum, devNum))
        {

            modeNum = 0;

            while (modeName = GetModeName(drvNum, devNum, modeNum))
            {

                packedNum = (devNum << 24) bitor (drvNum << 8) bitor (modeNum);
                strcpy(name, drvName);
                strcat(name, ":  ");
                strcat(name, devName);

                strcat(name, "  ");
                strcat(name, modeName);

                listSlot = SendMessage(listWin, LB_ADDSTRING, 0, (LPARAM)name);

                if (listSlot == LB_ERR)
                {
                    ShiError("Failed to add device to selection list.");
                }

                SendMessage(listWin, LB_SETITEMDATA, listSlot, packedNum);

                modeNum++;
            }

            devNum++;
        }

        drvNum++;
    }

    // Mark the first entry as selected by default and show the window to the user
    SendMessage(listWin, LB_SETCURSEL, 0, 0);
    ShowWindow(listWin, SW_SHOW);

    // Stop here until we get a choice from the user
    MessageBox(NULL, "Click OK when you've made your device choice", "", MB_OK);

    listSlot = SendMessage(listWin, LB_GETCURSEL, 0, 0);
    ShiAssert(listSlot not_eq LB_ERR);
    packedNum = SendMessage(listWin, LB_GETITEMDATA, listSlot, 0);
    devNum = (packedNum >> 24) bitand 0xFF;
    drvNum = (packedNum >> 8) bitand 0xFFFF;
    modeNum = (packedNum >> 0) bitand 0xFF;

    modeName = GetModeName(drvNum, devNum, modeNum);
    ShiAssert(modeName);
    sscanf(modeName, "%d x %d", &width, &height);
    ShiAssert(width * 3 / 4 == height);

    *usrDevNum = devNum;
    *usrDrvNum = drvNum;
    *usrWidth = width;

    // Get rid of the list box now that we're done with it
    DestroyWindow(listWin);
    listWin = NULL;

    // return their choice
    return TRUE;
}
#else
BOOL DeviceManager::ChooseDevice(int *usrDrvNum, int *usrDevNum, int *usrWidth)
{
    // Legacy Win32 listbox device chooser -- unused on Linux (the backend/GPU is picked in the options UI).
    (void)usrDrvNum;
    (void)usrDevNum;
    (void)usrWidth;
    return FALSE;
}
#endif

// OW

DXContext *DeviceManager::CreateContext(int driverNum, int devNum, int resNum,
                                        BOOL bFullscreen, HWND hWnd)
{
    try
    {
        // PHASE 1: bypassing the DDraw enum (crashes on modern Windows), Init() brings up the GPU backend.
        // #DX12/#104: any modern GPU backend (D3D12 OR Vulkan) -- DXContext::Init picks the backend by flag. Not DDraw.
        extern bool g_bUseGpu;
        if (g_bUseGpu)
        {
            DXContext *pCtx = new DXContext;
            if (pCtx == NULL)
                return NULL;
            pCtx->Init(hWnd, g_d3d11ReqWidth, g_d3d11ReqHeight, g_d3d11ReqDepth,
                       bFullscreen ? true : false);
            return pCtx;
        }
        DDDriverInfo *pDDI = GetDriver(driverNum);

        if (not pDDI)
            return NULL;

        DDDriverInfo::D3DDeviceInfo *pD3DDI = pDDI->GetDevice(devNum);

        if (not pD3DDI)
            return NULL;

        LPDDSURFACEDESC2 pddsd = pDDI->GetDisplayMode(resNum);

        if (not pddsd)
            return NULL;

        DXContext *pCtx = new DXContext;

        if (pCtx == NULL)
            return NULL;

        pCtx->m_guidDD = *pDDI->GetGuid();
        pCtx->m_guidD3D = *pD3DDI->GetGuid();

        MonoPrint("DeviceManager::CreateContext - %s\n", pD3DDI->GetName());

#ifdef _DEBUG

        if (not bFullscreen)
            ShiAssert(pDDI->CanRenderWindowed());

#endif

        pCtx->Init(hWnd, pddsd->dwWidth, pddsd->dwHeight,
                   pddsd->ddpfPixelFormat.dwRGBBitCount,
                   bFullscreen ? true : false);

        return pCtx;
    }

    catch (const _com_error &e)
    {
        MonoPrint("DeviceManager::OpenDevice - Error 0x%X\n", e.Error());
        return NULL;
    }
}

void DeviceManager::EnumDDDrivers(DeviceManager *pThis)
{
    // Artscout - 2026: [DX7-PURGE] the DirectDraw driver enumeration is gone. Under
    // D3D11/D3D12 the backend is selected directly; this legacy driver list stays empty
    // (GetMode() serves a curated resolution table, GetDriverName tolerates an empty list).
    (void)pThis;
    m_arrDDDrivers.clear();
}

#ifdef _WIN32
BOOL WINAPI DeviceManager::EnumDDCallback(GUID FAR *lpGUID,
                                          LPSTR lpDriverDescription,
                                          LPSTR lpDriverName, LPVOID lpContext)
{
    return EnumDDCallbackEx(lpGUID, lpDriverDescription, lpDriverName,
                            lpContext, NULL);
}

BOOL WINAPI DeviceManager::EnumDDCallbackEx(GUID FAR *lpGUID,
                                            LPSTR lpDriverDescription,
                                            LPSTR lpDriverName,
                                            LPVOID lpContext, HMONITOR hm)
{
    DeviceManager *pThis = (DeviceManager *)lpContext;
    pThis->m_arrDDDrivers.push_back(DDDriverInfo(
        lpGUID ? *lpGUID : GUID_NULL, lpDriverName, lpDriverDescription));
    return TRUE;
}
#endif // _WIN32 (DirectDraw enumeration callbacks; DDraw is gone, the list stays empty on Linux)

DeviceManager::DDDriverInfo *DeviceManager::GetDriver(int driverNum)
{
    if (driverNum < 0 or driverNum >= (int)m_arrDDDrivers.size())
        return false;

    return &m_arrDDDrivers[driverNum];
}

// DeviceManager::DDDriverInfo
/////////////////////////////////////////////////////////////////////////////

DeviceManager::DDDriverInfo::DDDriverInfo(GUID guid, LPCTSTR Name,
                                          LPCTSTR Description)
{
    m_guid = guid;
    m_strName = Name;
    m_strDescription = Description;

    EnumD3DDrivers();
}

void DeviceManager::DDDriverInfo::EnumD3DDrivers()
{
    // Artscout - 2026: [DX7-PURGE] the DirectDraw/Direct3D7 driver+mode enumeration
    // (DirectDrawCreateEx / EnumDevices / EnumDisplayModes / GetCaps) is gone. Under
    // D3D11/D3D12 the backend is selected directly; this legacy driver list stays empty
    // (the graphics-settings UI already tolerates an empty D3D-device/mode list).
    m_arrD3DDevices.clear();
    m_arrModes.clear();
    ZeroMemory(&m_caps, sizeof(m_caps));
    m_caps.dwSize = sizeof(m_caps);
    ZeroMemory(&devID, sizeof(devID));
}

HRESULT CALLBACK DeviceManager::DDDriverInfo::EnumD3DDriversCallback(
    LPSTR lpDeviceDescription, LPSTR lpDeviceName,
    LPD3DDEVICEDESC7 lpD3DHWDeviceDesc, LPVOID lpContext)
{
    DeviceManager::DDDriverInfo *pThis =
        (DeviceManager::DDDriverInfo *)lpContext;

    if (lpD3DHWDeviceDesc)
    {
        // COBRA - DX - Consider only Drivers making HW T&L
        // sfr: this causes notebooks to stop working
        //if (lpD3DHWDeviceDesc->dwDevCaps bitand D3DDEVCAPS_HWTRANSFORMANDLIGHT ){
        if (lpD3DHWDeviceDesc->dwDevCaps bitand
            DisplayOptionsClass::GetDevCaps())
        {
            pThis->m_arrD3DDevices.push_back(D3DDeviceInfo(
                *lpD3DHWDeviceDesc, lpDeviceName, lpDeviceDescription));
        }
    }

    return D3DENUMRET_OK;
}

HRESULT WINAPI DeviceManager::DDDriverInfo::EnumModesCallback(
    LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext)
{
    DeviceManager::DDDriverInfo *pThis =
        (DeviceManager::DDDriverInfo *)lpContext;
    pThis->m_arrModes.push_back(*lpDDSurfaceDesc);

    return DDENUMRET_OK;
}

const char *DeviceManager::DDDriverInfo::GetDeviceName(int n)
{
    if (n < 0 or n >= (int)m_arrD3DDevices.size())
        return NULL;

    return m_arrD3DDevices[n].GetName();
}

int DeviceManager::DDDriverInfo::FindDisplayMode(int nWidth, int nHeight,
                                                 int nBPP)
{
    for (int i = 0; i < (int)m_arrModes.size(); i++)
    {
        if (m_arrModes[i].dwWidth == nWidth and
            m_arrModes[i].dwHeight == nHeight and
            m_arrModes[i].ddpfPixelFormat.dwRGBBitCount == nBPP)
            return i;
    }

    return -1; // not found
}

LPDDSURFACEDESC2 DeviceManager::DDDriverInfo::GetDisplayMode(int n)
{
    if (n < 0 or n >= (int)m_arrModes.size())
        return NULL;

    return &m_arrModes[n];
}

bool DeviceManager::DDDriverInfo::CanRenderWindowed()
{
    // Artscout - 2026: [DX7-PURGE] no DDraw caps -- the D3D11/D3D12 backend is always windowed-capable.
    return true;
}

bool DeviceManager::DDDriverInfo::Is3dfx()
{
    return devID.dwVendorId == 4634;
}

bool DeviceManager::DDDriverInfo::SupportsSRT()
{
    if (devID.dwVendorId == 4634) // 3dfx
    {
        if (devID.dwDeviceId == 1 or devID.dwDeviceId == 2) // Voodoo 1 bitand 2
            return false;
    }

    return true; // assume SetRenderTarget works for all other cards
}

DeviceManager::DDDriverInfo::D3DDeviceInfo *
DeviceManager::DDDriverInfo::GetDevice(int n)
{
    if (n < 0 or n >= (int)m_arrD3DDevices.size())
        return NULL;

    return &m_arrD3DDevices[n];
}

int DeviceManager::DDDriverInfo::FindRGBRenderer()
{
    // Artscout - 2026: [DX7-PURGE] no D3D7 RGB software renderer to find (device list is empty under GPU).
    return -1;
}

// DeviceManager::DDDriverInfo::D3DDeviceInfo
/////////////////////////////////////////////////////////////////////////////

DeviceManager::DDDriverInfo::D3DDeviceInfo::D3DDeviceInfo(
    D3DDEVICEDESC7 &devDesc, LPSTR lpDeviceName, LPSTR lpDeviceDescription)
{
    m_devDesc = devDesc;
    m_strName = lpDeviceName;
    m_strDescription = lpDeviceDescription;
}

bool DeviceManager::DDDriverInfo::D3DDeviceInfo::IsHardware()
{
    // Artscout - 2026: [DX7-PURGE] no D3D7 software/HAL device GUIDs -- the GPU backend is always hardware.
    return true;
}

bool DeviceManager::DDDriverInfo::D3DDeviceInfo::CanFilterAnisotropic()
{
    // Artscout - 2026: [DX7-PURGE] no D3D7 caps for anisotropic filtering.
    bool bCanDoAnisotropic = false;
    return bCanDoAnisotropic;
}

// DXContext
/////////////////////////////////////////////////////////////////////////////

DXContext::DXContext()
{
    m_pDD = NULL;
    m_pD3D = NULL;
    m_pD3DD = NULL;
    m_bFullscreen = false;
    m_hWnd = NULL;
    m_nWidth = m_nHeight = 0;
    ZeroMemory(&m_guidDD, sizeof(m_guidDD));
    ZeroMemory(&m_guidD3D, sizeof(m_guidD3D));

    m_pcapsDD = new DDCAPS;
    m_pD3DHWDeviceDesc = new D3DDEVICEDESC7;
    m_pDevID = new DDDEVICEIDENTIFIER2;
    refcount = 1; // start with 1
}

DXContext::~DXContext()
{

    Shutdown();


    // sfr: why are these not being NULLed??
    if (m_pcapsDD)
        delete m_pcapsDD;

    if (m_pD3DHWDeviceDesc)
        delete m_pD3DHWDeviceDesc;

    if (m_pDevID)
        delete m_pDevID;
}

void DXContext::Shutdown()
{
    // MonoPrint("DXContext::Shutdown()\n");

    DWORD dwRefCnt;

    // release DX Engine stuff
    TheDXEngine.Release();

    // Artscout - 2026: [DX7-PURGE] no DirectDraw/Direct3D7 device to tear down
    // (SetCooperativeLevel/SetTexture/RestoreDisplayMode/Release) -- opaque handles stay NULL.
    (void)dwRefCnt;
    m_pD3DD = NULL;
    m_pD3D = NULL;
    m_pDD = NULL;

    m_bFullscreen = false;
    m_hWnd = NULL;
}

/*
DXContext& DXContext::operator=(DXContext &ref)
{
 m_pDD = ref.m_pDD;
 if(m_pDD) m_pDD->AddRef();

 m_pD3D = ref.m_pD3D;
 if(m_pD3D) m_pD3D->AddRef();

 m_pD3DD = ref.m_pD3DD;
 if(m_pD3DD) m_pD3DD->AddRef();

 return *this;
}
*/

bool DXContext::Init(HWND hWnd, int nWidth, int nHeight, int nDepth,
                     bool bFullscreen)
{
    MonoPrint("DXContext::Init(0x%X, %d, %d, %d, %d)\n", hWnd, nWidth, nHeight,
              nDepth, bFullscreen);

    try
    {
        ShiAssert(
            ::GetCurrentThreadId() ==
            GetWindowThreadProcessId(
                hWnd, NULL)); // Make sure this gets called by the main thread

        m_bFullscreen = bFullscreen;
        m_nWidth = nWidth;
        m_nHeight = nHeight;
        m_hWnd = hWnd;

        // Artscout - 2026 (#89): publish the chosen GPU (video-card combo index) so the backend picks that
        // DXGI adapter at device-create time instead of the default. -1/OOR -> default adapter.
        g_nDispVideoCard = DisplayOptions.DispVideoCard;

        // Artscout - 2026 (D3D11 purge C0): D3D12 is the sole GPU backend. g_bUseD3D11 now defaults OFF, so
        // if a stale cfg set "UseD3D12 0" we'd have NO GPU backend -> fall through to the dead DDraw7 path -> crash.
        // Force D3D12 on whenever D3D11 is off (i.e. always, now) so a GPU backend always comes up.
        {
            extern bool g_bUseD3D12;
            g_bUseD3D12 = true; // D3D11 purge: D3D12 is the sole GPU backend
        }

        // Artscout - 2026 (#104): runtime Vulkan backend. On Windows g_bUseVulkan is the graphics-options driver
        // choice (DX12 vs Vulkan); on Linux it defaults true (Vulkan is the only backend). When on, bring up
        // VulkanBackend+Renderer and return; the D3D12 block below is skipped. The window is the same Win32 HWND
        // either way (VulkanBackend builds a VK_KHR_win32_surface from it). If Vulkan init fails on Windows we
        // clear the flag and fall through to the (unchanged) D3D12 path.
        if (g_bUseVulkan)
        {
            // Artscout - 2026 (#104): the window keeps its normal bordered style (WS_OVERLAPPEDWINDOW, as created). The
            // earlier BORDERLESS (WS_POPUP) force here was a workaround from before swapchain resize handling existed: a
            // bordered window at full resolution gets its client clamped smaller than requested, and the fixed-size
            // swapchain then mismatched -> VK_ERROR_OUT_OF_DATE_KHR / a DWM wedge. That is now handled properly -- a
            // present/acquire returning OUT_OF_DATE/SUBOPTIMAL sets resizeRequested and RecreateSwapchain rebuilds to the
            // real client extent next frame -- so the borderless force is no longer needed and is removed.
            if (g_pVulkanBackend == NULL)
                g_pVulkanBackend = new VulkanBackend();

            // Artscout - 2026 (#107 VR-Vulkan Phase 1.5): BEFORE the Vulkan device is created, bring up the OpenXR
            // instance+system and ask which Vulkan instance/device extensions the VR session needs (compositor image
            // sharing); inject them, else xrCreateSession fails with VALIDATION_FAILURE. PreInitVulkan creates the
            // SINGLE XR instance that Init reuses (the SteamVR loader forbids two simultaneous instances). Best-effort.
            if (g_bUseOpenXR && g_pOpenXRBackend == NULL)
            {
                g_pOpenXRBackend = new OpenXRBackend();
                std::string vrInstExts, vrDevExts;
                if (g_pOpenXRBackend->PreInitVulkan(vrInstExts, vrDevExts))
                    g_pVulkanBackend->SetExtraVulkanExtensions(
                        vrInstExts.c_str(), vrDevExts.c_str());
                else
                {
                    // #104: PreInitVulkan brings up the XR instance+system and asks the runtime for its extensions.
                    // Failure here means NO usable VR runtime/headset (e.g. no OpenXR runtime installed, headset off).
                    // Disable VR NOW -- otherwise the later OpenXRBackend::Init(NULL) "adopts" the Vulkan handles and
                    // reports success WITHOUT a session, leaving g_bUseOpenXR set. winmain then runs the VR frame loop
                    // (xrWaitFrame path) which never presents to the flat swapchain -> a blank on-screen window. Clear
                    // the flag so the flat GetMessage/render loop runs and the window shows the game.
                    MonoPrint("OpenXR: no VR runtime/headset -- VR disabled, "
                              "flat rendering path\n");
                    delete g_pOpenXRBackend;
                    g_pOpenXRBackend = NULL;
                    g_bUseOpenXR = false;
                }
            }

            if (g_pVulkanBackend->Init(hWnd, nWidth, nHeight, nDepth,
                                       bFullscreen))
            {
                {
                    extern bool g_bUseGpu;
                    g_bUseGpu = true;
                } // GPU render mode
                {
                    extern bool g_bUseD3D12;
                    g_bUseD3D12 = false;
                } // Vulkan owns the frame -> keep D3D12-concrete paths OUT
                g_pRenderBackend = g_pVulkanBackend;

                if (!g_pVulkanRenderer)
                {
                    g_pVulkanRenderer = new VulkanRenderer(g_pVulkanBackend);
                    extern char FalconDataDirectory[];
                    char shaderDirVk[_MAX_PATH];
                    sprintf(shaderDirVk, "%s/shaders/vulkan/",
                            FalconDataDirectory); // compiled .spv location
                    if (!g_pVulkanRenderer->Init(shaderDirVk))
                        MonoPrint("VulkanRenderer::Init FAILED (spv dir=%s)\n",
                                  shaderDirVk);
                    else
                        MonoPrint("VulkanRenderer: up (shaders=%s)\n",
                                  shaderDirVk);
                }
                g_pRenderer = g_pVulkanRenderer;

                // Artscout - 2026 (#104): publish the resource managers as the global peers the engine's Tex.cpp /
                // dxvbmanager create through (mirrors g_pD3D12TextureManager in the D3D12 branch below). The texture
                // manager IS the renderer's own (so it samples exactly what Tex.cpp uploads); the VB manager is a
                // dedicated instance sharing the backend's device/queue.
                g_pVulkanTextureManager = g_pVulkanRenderer->TextureManager();
                if (!g_pVulkanVbManager)
                {
                    g_pVulkanVbManager = new VulkanVbManager(g_pVulkanBackend);
                    if (!g_pVulkanVbManager->Init())
                        MonoPrint("VulkanVbManager::Init FAILED\n");
                }

                // Fill sane device caps (mirrors the D3D12 branch) so caps readers don't see zeros.
                if (m_pD3DHWDeviceDesc)
                {
                    ZeroMemory(m_pD3DHWDeviceDesc, sizeof(*m_pD3DHWDeviceDesc));
                    m_pD3DHWDeviceDesc->dwMaxTextureWidth = 16384;
                    m_pD3DHWDeviceDesc->dwMaxTextureHeight = 16384;
                    m_pD3DHWDeviceDesc->dwMaxAnisotropy = 16;
                    m_pD3DHWDeviceDesc->dwDevCaps = 0xFFFFFFFF;
                    m_pD3DHWDeviceDesc->dwTextureOpCaps = 0xFFFFFFFF;
                    m_pD3DHWDeviceDesc->dpcTriCaps.dwAlphaCmpCaps = 0xFFFFFFFF;
                    m_pD3DHWDeviceDesc->dpcTriCaps.dwDestBlendCaps = 0xFFFFFFFF;
                    m_pD3DHWDeviceDesc->dpcTriCaps.dwSrcBlendCaps = 0xFFFFFFFF;
                    m_pD3DHWDeviceDesc->dpcTriCaps.dwRasterCaps = 0xFFFFFFFF;
                    m_pD3DHWDeviceDesc->dpcTriCaps.dwShadeCaps = 0xFFFFFFFF;
                    m_pD3DHWDeviceDesc->dpcTriCaps.dwTextureCaps = 0xFFFFFFFF;
                }

                TheDXEngine
                    .Setup(); // shared engine (materials/lighting/DX2D) -- API-agnostic

                // Artscout - 2026 (#107): bring up the OpenXR session ONCE on the Vulkan device that now carries the
                // runtime's required extensions (Init pulls the handles from g_pVulkanBackend, device arg NULL). The
                // object was created above for the extension query. DXContext::Init re-runs on every 3D entry / resize,
                // but VulkanBackend::Init then only rebuilds the swapchain -- the VkDevice is KEPT -- so the session
                // stays valid; gate on !IsSessionCreated() so we do NOT create a second session (the runtime allows
                // only one -> XR_ERROR_LIMIT_REACHED). Best-effort: failure keeps the flat path.
                if (g_bUseOpenXR && g_pOpenXRBackend &&
                    !g_pOpenXRBackend->IsSessionCreated())
                {
                    if (g_pOpenXRBackend->Init(NULL))
                        MonoPrint("OpenXR: Vulkan backend up\n");
                    else
                    {
                        MonoPrint("OpenXR: Vulkan init failed/incomplete -- VR "
                                  "disabled, flat path continues\n");
                        delete g_pOpenXRBackend;
                        g_pOpenXRBackend = NULL;
                        g_bUseOpenXR = false;
                    }
                }

                // #104: window already sized to nWidth x nHeight BEFORE Init (above), so the swapchain matches it --
                // do NOT resize again here (a second resize would re-trigger OUT_OF_DATE and the frozen-present bug).
                MonoPrint("DXContext::Init - Vulkan backend up\n");
                return true;
            }
            MonoPrint("DXContext::Init - Vulkan init failed; falling back to "
                      "D3D12\n");
            // #107: drop the OpenXR object created above for the extension query -- the D3D12 branch recreates it.
            if (g_pOpenXRBackend)
            {
                delete g_pOpenXRBackend;
                g_pOpenXRBackend = NULL;
            }
            delete g_pVulkanBackend;
            g_pVulkanBackend = NULL;
            g_bUseVulkan =
                false; // fall through to the D3D12 path (g_bUseD3D12 is still true)
        }

        // Artscout - 2026: #DX12 Phase 1 -- bring up D3D12Backend and return. On success we clear g_bUseD3D11
        // so the (concrete) D3D11 render path stays OUT (it would call a NULL g_pD3D11Backend). Phase 1 renders
        // nothing but the per-frame clear (device+swapchain+fence+present milestone). D3D11/D3D7 untouched when
        // the flag is off. Ported passes (2D/object/terrain/RTT/OpenXR/view-instancing) come in later phases.
        {
#ifdef _WIN32 // D3D12 is Windows-only; Linux uses the Vulkan branch above
            extern bool g_bUseD3D12;
            if (g_bUseD3D12)
            {
                if (g_pD3D12Backend == NULL)
                    g_pD3D12Backend = new D3D12Backend();
                if (g_pD3D12Backend->Init(hWnd, nWidth, nHeight, nDepth,
                                          bFullscreen))
                {
                    // D3D11 purge: D3D12 owns the frame (g_bUseD3D11 symbol removed).
                    {
                        extern bool g_bUseGpu;
                        g_bUseGpu = true;
                    } // #DX12: GPU render mode (not dead DDraw7)
                    g_pRenderBackend =
                        g_pD3D12Backend; // #DX12: active neutral backend

                    // #DX12 Phase 3: bring up the D3D12 renderer (compiles FFEmu.hlsl -> DXBC + root sig + CBs).
                    // Draw passes are still stubbed, so nothing 3D renders yet AND nothing calls it (render
                    // sites are not migrated to g_pRenderer). Creating it here validates shader compilation.
                    if (!g_pD3D12Renderer)
                    {
                        g_pD3D12Renderer = new D3D12Renderer();
                        extern char FalconDataDirectory[];
                        char shaderDir12[_MAX_PATH];
                        sprintf(shaderDir12, "%s/shaders/",
                                FalconDataDirectory);
                        if (!g_pD3D12Renderer->Init(shaderDir12))
                            MonoPrint("D3D12Renderer::Init FAILED (shader "
                                      "compile? dir=%s)\n",
                                      shaderDir12);
                        else
                            MonoPrint("D3D12Renderer: up (shaders=%s)\n",
                                      shaderDir12);
                    }
                    g_pRenderer =
                        g_pD3D12Renderer; // #DX12: active neutral renderer (D3D12; passes = later Phase 3)

                    // #DX12 п.1: the texture manager (TextureHandle::Load creates D3D12 textures + staging SRVs).
                    if (!g_pD3D12TextureManager)
                    {
                        g_pD3D12TextureManager = new D3D12TextureManager();
                        if (!g_pD3D12TextureManager->Init())
                            MonoPrint("D3D12TextureManager::Init FAILED\n");
                        else
                            MonoPrint("D3D12TextureManager: up\n");
                    }

                    // #DX12 п.5: bring up OpenXR bound to the D3D12 device/queue (Init pulls them from
                    // g_pD3D12Backend internally, so the device arg is NULL). Best-effort -- failure keeps flat.
                    if (g_bUseOpenXR && g_pOpenXRBackend == NULL)
                    {
                        g_pOpenXRBackend = new OpenXRBackend();
                        if (g_pOpenXRBackend->Init(NULL))
                            MonoPrint("OpenXR: D3D12 backend up\n");
                        else
                        {
                            MonoPrint("OpenXR: D3D12 init failed -- VR "
                                      "disabled, flat path continues\n");
                            delete g_pOpenXRBackend;
                            g_pOpenXRBackend = NULL;
                            g_bUseOpenXR = false;
                        }
                    }

                    // Fill sane device caps (D3D7 GetCaps is not called) so anything that reads them
                    // doesn't see zeros -- mirrors the D3D11 branch below.
                    if (m_pD3DHWDeviceDesc)
                    {
                        ZeroMemory(m_pD3DHWDeviceDesc,
                                   sizeof(*m_pD3DHWDeviceDesc));
                        m_pD3DHWDeviceDesc->dwMaxTextureWidth = 16384;
                        m_pD3DHWDeviceDesc->dwMaxTextureHeight = 16384;
                        m_pD3DHWDeviceDesc->dwMaxAnisotropy = 16;
                        m_pD3DHWDeviceDesc->dwDevCaps = 0xFFFFFFFF;
                        m_pD3DHWDeviceDesc->dwTextureOpCaps = 0xFFFFFFFF;
                        m_pD3DHWDeviceDesc->dpcTriCaps.dwAlphaCmpCaps =
                            0xFFFFFFFF;
                        m_pD3DHWDeviceDesc->dpcTriCaps.dwDestBlendCaps =
                            0xFFFFFFFF;
                        m_pD3DHWDeviceDesc->dpcTriCaps.dwSrcBlendCaps =
                            0xFFFFFFFF;
                        m_pD3DHWDeviceDesc->dpcTriCaps.dwRasterCaps =
                            0xFFFFFFFF;
                        m_pD3DHWDeviceDesc->dpcTriCaps.dwShadeCaps = 0xFFFFFFFF;
                        m_pD3DHWDeviceDesc->dpcTriCaps.dwTextureCaps =
                            0xFFFFFFFF;
                    }

                    // #DX12: initialize the shared engine (materials, lighting, and the 2D/particle engine --
                    // DX2D_Init mallocs Dyn2DVertexBuffer[].VbPtr). The D3D11 branch below calls this at :828;
                    // the D3D12 branch returns before reaching it, so without this the particle trail path
                    // (DX2D_AddSingle) wrote into a NULL VbPtr -> crash. API-agnostic (no D3D7/D3D11 device use).
                    TheDXEngine.Setup();

#ifdef _WIN32
                    SetWindowLong(hWnd, GWL_STYLE,
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE);
                    RECT rcW = {0, 0, nWidth, nHeight};
                    AdjustWindowRect(&rcW, WS_OVERLAPPEDWINDOW, FALSE);
                    SetWindowPos(hWnd, NULL, 0, 0, rcW.right - rcW.left,
                                 rcW.bottom - rcW.top,
                                 SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED |
                                     SWP_SHOWWINDOW);
#endif // _WIN32: window styling/resize is the ffplatform/SDL3 job on Linux
                    MonoPrint("DXContext::Init - D3D12 backend up (Phase 1)\n");
                    return true;
                }
                MonoPrint("DXContext::Init - D3D12 init failed (no D3D11 "
                          "fallback -- D3D12 is the sole backend)\n");
                delete g_pD3D12Backend;
                g_pD3D12Backend = NULL;
                g_bUseD3D12 = false;
            }
#endif // _WIN32
        }


        // Artscout - 2026: [DX7-PURGE] no DirectDraw/Direct3D7 device fallback -- if the
        // D3D11/D3D12 backend failed to init above there is nothing else to try.
        return false;
    }

    catch (const _com_error &e)
    {
        MonoPrint("DXContext::DD_Init - Error 0x%X\n", e.Error());
        return false;
    }
}

extern bool bInBeginScene; // ASSO:

bool DXContext::SetRenderTarget(IDirectDrawSurface7 *pRenderTarget)
{
    // #34 D3D11: the render target (RTV) is owned by D3D11Backend; the legacy D3D7
    // device-creation / SetRenderTarget path has been removed.
    return true;
}


void DXContext::EnumZBufferFormats(void *parr)
{
    // Artscout - 2026: [DX7-PURGE] no DDraw Z-buffer format enumeration (depth is the backend's job).
    ((PIXELFMT_ARRAY *)parr)->clear();
}

HRESULT CALLBACK DXContext::EnumZBufferFormatsCallback(
    LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext)
{
    (void)lpDDPixFmt;
    (void)lpContext; // [DX7-PURGE] unused
    return 0;
}

void DXContext::AttachDepthBuffer(IDirectDrawSurface7 *p)
{
    // Artscout - 2026: [DX7-PURGE] DDraw depth-surface creation/attach removed (D3D11/D3D12 own depth).
    (void)p;
    return;
#if 0
    //JAM 25Jul03
    //return;

    // Check the display mode, and
    DDSURFACEDESC2 ddsd_disp;
    ZeroMemory(&ddsd_disp, sizeof(ddsd_disp));
    ddsd_disp.dwSize = sizeof(ddsd_disp);
    CheckHR(m_pDD->GetDisplayMode(&ddsd_disp));

    IDirectDrawSurface7Ptr pDDSZB;
    PIXELFMT_ARRAY arrZBFmts;

    EnumZBufferFormats(&arrZBFmts);

    if ( not arrZBFmts.empty())
    {
        // Match Z Buffer depth to the display depth
        DDPIXELFORMAT pixfmt;

        PIXELFMT_ARRAY::iterator it;

        for (it = arrZBFmts.begin(); it not_eq arrZBFmts.end(); it++)
        {
            // RV - RED - OK, Restored old original Code, seems the Stencil search causes a 25% FPS drop, dunno why
            // as we use the setncil on a surface not having it now
            // if(it->dwZBufferBitDepth >= ddsd_disp.ddpfPixelFormat.dwRGBBitCount and it->dwStencilBitDepth>=8)
            if (it->dwZBufferBitDepth == ddsd_disp.ddpfPixelFormat.dwRGBBitCount)
            {
                pixfmt = *it;
                break;
            }
        }

        DDSURFACEDESC2 ddsd;
        ZeroMemory(&ddsd, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS bitor DDSD_WIDTH bitor DDSD_HEIGHT bitor DDSD_PIXELFORMAT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
        ddsd.dwWidth = m_nWidth;
        ddsd.dwHeight = m_nHeight;
        ddsd.ddpfPixelFormat = pixfmt;

        // Software devices require system-memory depth buffers.
        if (m_eDeviceCategory == D3DDeviceCategory_Software)
            ddsd.ddsCaps.dwCaps or_eq DDSCAPS_SYSTEMMEMORY;

        CheckHR(m_pDD->CreateSurface(&ddsd, &pDDSZB, NULL));

        // Attach it to the render target
        CheckHR(p->AddAttachedSurface(pDDSZB));
    }

    else MonoPrint("DXContext::AttachDepthBuffer() - Warning: No Z-Buffer formats \n");
#endif
}

void DXContext::CheckCaps()
{
    // Artscout - 2026: [DX7-PURGE] D3D7 device-caps debug report removed (m_pD3DHWDeviceDesc is dead).
}

bool DXContext::ValidateD3DDevice()
{
    // Artscout - 2026: [DX7-PURGE] D3D7 ValidateDevice removed (no D3D7 device under GPU).
    return true;
}

DWORD DXContext::TestCooperativeLevel()
{
    // #DX12/#104: any GPU backend (D3D12/Vulkan) has no DDraw device -> no cooperative-level check.
    extern bool g_bUseGpu;
    if (g_bUseGpu)
        return DD_OK; // no DDraw coop under a GPU backend
    // Artscout - 2026: [DX7-PURGE] no DDraw TestCooperativeLevel under a GPU backend.
    return DD_OK;
}
