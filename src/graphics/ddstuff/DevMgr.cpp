/***************************************************************************\
    DevMgr.cpp
    Scott Randolph
    November 8, 1996

    This class provides management of the drawing devices in the system.
\***************************************************************************/
#include "stdafx.h"
#include "DevMgr.h"
#include "FalcLib/include/playerop.h"

#include "FalcLib/include/dispopts.h" //JAM 04Oct03

#include <math.h>
#include "polylib.h"
#include "Graphics/DXEngine/DXEngine.h"
#include "Graphics/DXEngine/D3D11Backend.h"	// PHASE 1
#include "Graphics/DXEngine/d3d11/D3D11Renderer.h"	// PHASE 4
#include "Graphics/DXEngine/d3d11/D3D11TextureManager.h"	// PHASE 3
#include "Graphics/DXEngine/DXVBManager.h"	// PHASE 4: TheVbManager.Setup
extern bool g_bUseD3D11;
int g_d3d11ReqWidth=0;
int g_d3d11ReqHeight=0;
int g_d3d11ReqDepth=32;
extern bool g_bUse_DX_Engine;

typedef std::vector<DDPIXELFORMAT> PIXELFMT_ARRAY;

int HighResolutionHackFlag = FALSE; // Used in WinMain.CPP
extern bool g_bForceDXMultiThreadedCoopLevel;
extern char g_CardDetails[]; // JB 010215

#define INT3 __debugbreak()   // Artscout - 2026 (x64): int 3 intrinsic, builds on x86+x64

// Cobra - Hack to get VC6 to link
#if _MSC_VER < 1300

void __cdecl std::_Xlen()
{
}
void __cdecl std::_Xran()
{
}

#endif

typedef HRESULT(WINAPI *LPDIRECTDRAWCREATEEX)(GUID *lpGUID, LPVOID *lplpDD, REFIID iid, IUnknown *pUnkOuter);
LPDIRECTDRAWENUMERATEEX pfnDirectDrawEnumerateEx = NULL;
LPDIRECTDRAWCREATEEX pfnDirectDrawCreateEx = NULL;

// Device GUIDs
struct __declspec(uuid("D7B71CFA-4342-11CF-CE67-0120A6C2C935")) DEVGUID_3DFX_VOODOO2_a; // DX7 Beta Driver
struct __declspec(uuid("472BEA00-40DF-11D1-A9DF-006097C2EDB2")) DEVGUID_3DFX_VOODOO2_b; // DX7

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
    if (ready) D3DXUninitialize();

    ready = FALSE;
}


const char * DeviceManager::GetDriverName(int driverNum)
{
    if (driverNum < 0 or driverNum >= (int) m_arrDDDrivers.size())
        return NULL;

    return m_arrDDDrivers[driverNum].GetName();
}


const char * DeviceManager::GetDeviceName(int driverNum, int devNum)
{
    if (driverNum < 0 or driverNum >= (int) m_arrDDDrivers.size())
        return NULL;

    return m_arrDDDrivers[driverNum].GetDeviceName(devNum);
}

int DeviceManager::FindPrimaryDisplayDriver()
{
    for (int i = 0; i < (int) m_arrDDDrivers.size(); i++)
        if (IsEqualGUID(m_arrDDDrivers[i].m_guid, GUID_NULL)) return i;

    return -1;
}

const char *DeviceManager::GetModeName(int driverNum, int devNum, int modeNum)
{
    static char buffer[80];
    int i = 0;

    if (driverNum < 0 or driverNum >= (int) m_arrDDDrivers.size())
        return NULL;

    DDDriverInfo &DI = m_arrDDDrivers[driverNum];
    LPDDSURFACEDESC2 pddsd = NULL;

    // Find the nth (legal) display mode
    while (pddsd = DI.GetDisplayMode(i))
    {
        // For now we only allow 640x480, 800x600, 1280x960, 1600x1200
        // (MPR already does the 4:3 aspect ratio check for us)
        if (pddsd->ddpfPixelFormat.dwRGBBitCount >= 16 and (pddsd->dwWidth == 640 or pddsd->dwWidth == 800 or pddsd->dwWidth == 1024 or
                (pddsd->dwWidth == 1280 and pddsd->dwHeight == 960) or pddsd->dwWidth == 1600 or HighResolutionHackFlag))
        {
            if (modeNum == 0)
            {
                // This is the one we want.  Return it.
                // OW
                // sprintf( buffer, "%0dx%0d", pddsd->dwWidth, pddsd->dwHeight);
                sprintf(buffer, "%0dx%0d - %d Bit", pddsd->dwWidth, pddsd->dwHeight, pddsd->ddpfPixelFormat.dwRGBBitCount);
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
struct D3D11ModeEntry { UINT w, h; };
static const D3D11ModeEntry g_d3d11Modes[] =
{
    {  640,  480 },   // 4:3 legacy
    {  800,  600 },   // 4:3 legacy
    { 1024,  768 },   // 4:3 legacy
    { 1280, 1024 },   // 5:4
    { 1280,  720 },   // 720p  (16:9)
    { 1600,  900 },   // 16:9
    { 1920, 1080 },   // 1080p (16:9) -- 3D DEFAULT
    { 2560, 1440 },   // 2K / QHD (16:9)
    { 3840, 2160 },   // 4K / UHD (16:9)
};
static const int g_nD3D11Modes = (int)(sizeof(g_d3d11Modes) / sizeof(g_d3d11Modes[0]));

bool DeviceManager::GetMode(int driverNum, int devNum, int modeNum, UINT *pWidth, UINT *pHeight, UINT *pDepth)
{
    static char buffer[80];
    int i = 0;

    if (g_bUseD3D11)
    {
        if (modeNum < 0 or modeNum >= g_nD3D11Modes) return false;
        *pWidth  = g_d3d11Modes[modeNum].w;
        *pHeight = g_d3d11Modes[modeNum].h;
        *pDepth  = 32;
        return true;
    }

    if (driverNum < 0 or driverNum >= (int) m_arrDDDrivers.size())
        return false;

    DDDriverInfo &DI = m_arrDDDrivers[driverNum];
    LPDDSURFACEDESC2 pddsd = DI.GetDisplayMode(modeNum);

    if ( not pddsd) return false;

    *pWidth = pddsd->dwWidth;
    *pHeight = pddsd->dwHeight;
    *pDepth = pddsd->ddpfPixelFormat.dwRGBBitCount; // OW

    return true;
}

// Present the user with a dialog box listing the available devices and pick one
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
    listWin = CreateWindow(
                  "LISTBOX", /* class */
                  "Choose Display Device",/* caption */
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

    if ( not listWin)
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
    devNum  = (packedNum >> 24) bitand 0xFF;
    drvNum  = (packedNum >> 8) bitand 0xFFFF;
    modeNum = (packedNum >> 0) bitand 0xFF;

    modeName = GetModeName(drvNum, devNum, modeNum);
    ShiAssert(modeName);
    sscanf(modeName, "%d x %d", &width, &height);
    ShiAssert(width * 3 / 4 == height);

    *usrDevNum  = devNum;
    *usrDrvNum  = drvNum;
    *usrWidth   = width;

    // Get rid of the list box now that we're done with it
    DestroyWindow(listWin);
    listWin = NULL;

    // return their choice
    return TRUE;
}

// OW

DXContext *DeviceManager::CreateContext(int driverNum, int devNum, int resNum, BOOL bFullscreen, HWND hWnd)
{
    try
    {
        // PHASE 1: bypassing the DDraw enum (crashes on modern Windows), Init() brings up D3D11Backend
        if (g_bUseD3D11)
        {
            DXContext *pCtx = new DXContext;
            if (pCtx == NULL) return NULL;
            pCtx->Init(hWnd, g_d3d11ReqWidth, g_d3d11ReqHeight, g_d3d11ReqDepth, bFullscreen ? true : false);
            return pCtx;
        }
        DDDriverInfo *pDDI = GetDriver(driverNum);

        if ( not pDDI) return NULL;

        DDDriverInfo::D3DDeviceInfo *pD3DDI = pDDI->GetDevice(devNum);

        if ( not pD3DDI) return NULL;

        LPDDSURFACEDESC2 pddsd = pDDI->GetDisplayMode(resNum);

        if ( not pddsd) return NULL;

        DXContext *pCtx = new DXContext;

        if (pCtx == NULL) return NULL;

        pCtx->m_guidDD = *pDDI->GetGuid();
        pCtx->m_guidD3D = *pD3DDI->GetGuid();

        MonoPrint("DeviceManager::CreateContext - %s\n", pD3DDI->GetName());

#ifdef _DEBUG

        if ( not bFullscreen) ShiAssert(pDDI->CanRenderWindowed());

#endif

        pCtx->Init(hWnd, pddsd->dwWidth, pddsd->dwHeight, pddsd->ddpfPixelFormat.dwRGBBitCount, bFullscreen ? true : false);

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
    HINSTANCE h = LoadLibrary("ddraw.dll");

    if ( not h) return;

    m_arrDDDrivers.clear();

    // Note that you must know which version of the function to retrieve (see the following text). For this example, we use the ANSI version.
    LPDIRECTDRAWENUMERATEEX lpDDEnumEx;
    lpDDEnumEx = (LPDIRECTDRAWENUMERATEEX) GetProcAddress(h, "DirectDrawEnumerateExA");

    // If the function is there, call it to enumerate all display devices attached to the desktop, and any non-display DirectDraw devices.
    if (lpDDEnumEx) lpDDEnumEx(EnumDDCallbackEx, pThis, DDENUM_ATTACHEDSECONDARYDEVICES |
                                   DDENUM_NONDISPLAYDEVICES);
    else DirectDrawEnumerate(EnumDDCallback, pThis);

    FreeLibrary(h);
}

BOOL WINAPI DeviceManager::EnumDDCallback(GUID FAR *lpGUID, LPSTR lpDriverDescription,
        LPSTR lpDriverName, LPVOID lpContext)
{
    return EnumDDCallbackEx(lpGUID, lpDriverDescription, lpDriverName, lpContext, NULL);
}

BOOL WINAPI DeviceManager::EnumDDCallbackEx(GUID FAR *lpGUID, LPSTR lpDriverDescription,
        LPSTR lpDriverName, LPVOID lpContext, HMONITOR hm)
{
    DeviceManager *pThis = (DeviceManager *) lpContext;
    pThis->m_arrDDDrivers.push_back(DDDriverInfo(lpGUID ? *lpGUID : GUID_NULL, lpDriverName, lpDriverDescription));
    return TRUE;
}

DeviceManager::DDDriverInfo *DeviceManager::GetDriver(int driverNum)
{
    if (driverNum < 0 or driverNum >= (int) m_arrDDDrivers.size())
        return false;

    return &m_arrDDDrivers[driverNum];
}

// DeviceManager::DDDriverInfo
/////////////////////////////////////////////////////////////////////////////

DeviceManager::DDDriverInfo::DDDriverInfo(GUID guid, LPCTSTR Name, LPCTSTR Description)
{
    m_guid = guid;
    m_strName = Name;
    m_strDescription = Description;

    EnumD3DDrivers();
}

void DeviceManager::DDDriverInfo::EnumD3DDrivers()
{
    try
    {
        IDirectDrawPtr pDD;
        IDirectDraw7Ptr pDD7;
        IDirect3D7Ptr pD3D;

        // Create DDRAW object
        CheckHR(DirectDrawCreateEx(&m_guid, (void **) &pDD, IID_IDirectDraw7, NULL));

        pD3D = pDD;
        pDD7 = pDD;
        pDD7->GetDeviceIdentifier(&devID, NULL);

        m_arrD3DDevices.clear();
        pD3D->EnumDevices(EnumD3DDriversCallback, this);

        pDD7->EnumDisplayModes(NULL, NULL, this, EnumModesCallback);

        ZeroMemory(&m_caps, sizeof(m_caps));
        m_caps.dwSize = sizeof(m_caps);
        pDD7->GetCaps(&m_caps, NULL);
    }

    catch (const _com_error &e)
    {
        MonoPrint("DeviceManager::DDDriverInfo::EnumD3DDrivers - Error 0x%X\n", e.Error());
    }
}

HRESULT CALLBACK DeviceManager::DDDriverInfo::EnumD3DDriversCallback(LPSTR lpDeviceDescription,
        LPSTR lpDeviceName, LPD3DDEVICEDESC7 lpD3DHWDeviceDesc, LPVOID lpContext)
{
    DeviceManager::DDDriverInfo *pThis = (DeviceManager::DDDriverInfo *) lpContext;

    if (lpD3DHWDeviceDesc)
    {
        // COBRA - DX - Consider only Drivers making HW T&L
        // sfr: this causes notebooks to stop working
        //if (lpD3DHWDeviceDesc->dwDevCaps bitand D3DDEVCAPS_HWTRANSFORMANDLIGHT ){
        if (lpD3DHWDeviceDesc->dwDevCaps bitand DisplayOptionsClass::GetDevCaps())
        {
            pThis->m_arrD3DDevices.push_back(D3DDeviceInfo(*lpD3DHWDeviceDesc, lpDeviceName, lpDeviceDescription));
        }
    }

    return D3DENUMRET_OK;
}

HRESULT WINAPI DeviceManager::DDDriverInfo::EnumModesCallback(LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext)
{
    DeviceManager::DDDriverInfo *pThis = (DeviceManager::DDDriverInfo *) lpContext;
    pThis->m_arrModes.push_back(*lpDDSurfaceDesc);

    return DDENUMRET_OK;
}

const char *DeviceManager::DDDriverInfo::GetDeviceName(int n)
{
    if (n < 0 or n >= (int) m_arrD3DDevices.size())
        return NULL;

    return m_arrD3DDevices[n].GetName();
}

int DeviceManager::DDDriverInfo::FindDisplayMode(int nWidth, int nHeight, int nBPP)
{
    for (int i = 0; i < (int) m_arrModes.size(); i++)
    {
        if (m_arrModes[i].dwWidth == nWidth and m_arrModes[i].dwHeight == nHeight and 
            m_arrModes[i].ddpfPixelFormat.dwRGBBitCount == nBPP)
            return i;
    }

    return -1; // not found
}

LPDDSURFACEDESC2 DeviceManager::DDDriverInfo::GetDisplayMode(int n)
{
    if (n < 0 or n >= (int) m_arrModes.size())
        return NULL;

    return &m_arrModes[n];
}

bool DeviceManager::DDDriverInfo::CanRenderWindowed()
{
    return m_caps.dwCaps2 bitand DDCAPS2_CANRENDERWINDOWED ? true : false;
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

DeviceManager::DDDriverInfo::D3DDeviceInfo *DeviceManager::DDDriverInfo::GetDevice(int n)
{
    if (n < 0 or n >= (int) m_arrD3DDevices.size())
        return NULL;

    return &m_arrD3DDevices[n];
}

int DeviceManager::DDDriverInfo::FindRGBRenderer()
{
    for (int i = 0; i < (int) m_arrD3DDevices.size(); i++)
        if (IsEqualGUID(m_arrD3DDevices[i].m_devDesc.deviceGUID, IID_IDirect3DRGBDevice)) return i;

    return -1;
}

// DeviceManager::DDDriverInfo::D3DDeviceInfo
/////////////////////////////////////////////////////////////////////////////

DeviceManager::DDDriverInfo::D3DDeviceInfo::D3DDeviceInfo(D3DDEVICEDESC7 &devDesc, LPSTR lpDeviceName, LPSTR lpDeviceDescription)
{
    m_devDesc = devDesc;
    m_strName = lpDeviceName;
    m_strDescription = lpDeviceDescription;
}

bool DeviceManager::DDDriverInfo::D3DDeviceInfo::IsHardware()
{
    if (IsEqualIID(m_devDesc.deviceGUID, IID_IDirect3DRGBDevice) or IsEqualIID(m_devDesc.deviceGUID, IID_IDirect3DRefDevice) or
        IsEqualIID(m_devDesc.deviceGUID, IID_IDirect3DRampDevice) or IsEqualIID(m_devDesc.deviceGUID, IID_IDirect3DMMXDevice))
        return false;
    else if (IsEqualIID(m_devDesc.deviceGUID, IID_IDirect3DHALDevice))
        return true;
    else if (IsEqualIID(m_devDesc.deviceGUID, IID_IDirect3DTnLHalDevice))
        return true;
    else
    {
        ShiAssert(false); // check this
        return true;
    }
}

bool DeviceManager::DDDriverInfo::D3DDeviceInfo::CanFilterAnisotropic()
{
    bool bCanDoAnisotropic = (m_devDesc.dpcTriCaps.dwTextureFilterCaps bitand D3DPTFILTERCAPS_MAGFANISOTROPIC) and 
                             (m_devDesc.dpcTriCaps.dwTextureFilterCaps bitand D3DPTFILTERCAPS_MINFANISOTROPIC);
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
    if (m_pcapsDD) delete m_pcapsDD;

    if (m_pD3DHWDeviceDesc) delete m_pD3DHWDeviceDesc;

    if (m_pDevID) delete m_pDevID;
}

void DXContext::Shutdown()
{
    // MonoPrint("DXContext::Shutdown()\n");

    DWORD dwRefCnt;

    // release DX Engine stuff
    TheDXEngine.Release();

    if (m_pDD)
        CheckHR(m_pDD->SetCooperativeLevel(m_hWnd, DDSCL_NORMAL));

    if (m_pD3DD)
    {
        // free all textures
        m_pD3DD->SetTexture(0, NULL);
        m_pD3DD->SetTexture(1, NULL);
        m_pD3DD->SetTexture(3, NULL);

        // release
        dwRefCnt = m_pD3DD->Release();
        // ShiAssert(dwRefCnt == 0);
        m_pD3DD = NULL;
    }

    if (m_pD3D)
    {
        dwRefCnt = m_pD3D->Release();
        // ShiAssert(dwRefCnt == 0);
        m_pD3D = NULL;
    }

    if (m_pDD)
    {
        if (m_bFullscreen)
        {
            m_pDD->RestoreDisplayMode();
            m_pDD->FlipToGDISurface();
        }

        dwRefCnt = m_pDD->Release();
        // ShiAssert(dwRefCnt == 0);
        m_pDD = NULL;
    }

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

bool DXContext::Init(HWND hWnd, int nWidth, int nHeight, int nDepth, bool bFullscreen)
{
    MonoPrint("DXContext::Init(0x%X, %d, %d, %d, %d)\n", hWnd, nWidth, nHeight, nDepth, bFullscreen);

    try
    {
        ShiAssert(::GetCurrentThreadId() == GetWindowThreadProcessId(hWnd, NULL)); // Make sure this gets called by the main thread

        m_bFullscreen = bFullscreen;
        m_nWidth = nWidth;
        m_nHeight = nHeight;
        m_hWnd = hWnd;

        // PHASE 1: bring up D3D11Backend and return; DDraw/D3D7 below is bypassed
        if (g_bUseD3D11)
        {
            if (g_pD3D11Backend == NULL) g_pD3D11Backend = new D3D11Backend();
            if (g_pD3D11Backend->Init(hWnd, nWidth, nHeight, nDepth, bFullscreen))
            {
                // PHASE 4: under D3D11 the D3D7 GetCaps() is not called -- fill D3DDEVICEDESC7
                // with sane caps, else dwMaxTextureWidth/Height=0 breaks creation
                // of cockpit textures (empty m_arrTex -> crash), and CheckCaps fails features.
                if (m_pD3DHWDeviceDesc)
                {
                    ZeroMemory(m_pD3DHWDeviceDesc, sizeof(*m_pD3DHWDeviceDesc));
                    m_pD3DHWDeviceDesc->dwMaxTextureWidth  = 16384;
                    m_pD3DHWDeviceDesc->dwMaxTextureHeight = 16384;
                    m_pD3DHWDeviceDesc->dwMaxAnisotropy    = 16;
                    m_pD3DHWDeviceDesc->dwDevCaps          = 0xFFFFFFFF;
                    m_pD3DHWDeviceDesc->dwTextureOpCaps    = 0xFFFFFFFF;
                    m_pD3DHWDeviceDesc->dpcTriCaps.dwAlphaCmpCaps  = 0xFFFFFFFF;
                    m_pD3DHWDeviceDesc->dpcTriCaps.dwDestBlendCaps = 0xFFFFFFFF;
                    m_pD3DHWDeviceDesc->dpcTriCaps.dwSrcBlendCaps  = 0xFFFFFFFF;
                    m_pD3DHWDeviceDesc->dpcTriCaps.dwRasterCaps    = 0xFFFFFFFF;
                    m_pD3DHWDeviceDesc->dpcTriCaps.dwShadeCaps     = 0xFFFFFFFF;
                    m_pD3DHWDeviceDesc->dpcTriCaps.dwTextureCaps   = 0xFFFFFFFF;
                }

                // Artscout - 2026: bring up the 3D renderer (FFEmu shaders) once. Shader source
                // is loaded at runtime from "<game dir>\shaders\" (FalconDataDirectory), so it
                // ships with the install instead of a hard-coded dev path.
                if (!g_pD3D11Renderer)
                {
                    g_pD3D11Renderer = new D3D11Renderer();
                    extern char FalconDataDirectory[];
                    char shaderDir[_MAX_PATH];
                    sprintf(shaderDir, "%s\\shaders\\", FalconDataDirectory);
                    if (!g_pD3D11Renderer->Init(shaderDir))
                        MonoPrint("D3D11Renderer::Init FAILED (shader compile? dir=%s)\n", shaderDir);
                    else
                        MonoPrint("D3D11Renderer: 3D renderer up (shaders: %s)\n", shaderDir);
                }

                // PHASE 3: the texture manager (TextureHandle::Load creates D3D11 textures).
                if (!g_pD3D11TextureManager)
                {
                    g_pD3D11TextureManager = new D3D11TextureManager();
                    if (!g_pD3D11TextureManager->Init())
                        MonoPrint("D3D11TextureManager::Init FAILED\n");
                    else
                        MonoPrint("D3D11TextureManager: up\n");
                }

                // PHASE 4: under D3D11 the D3D7 device-creation path (where normally
                // TheDXEngine.Setup/TheVbManager.Setup are called) is bypassed -- initialize here.
                // #55 LEAK: like g_pD3D11Renderer/TextureManager above -- the engine/VB are global,
                // Setup ONCE. DXContext::Init is called on EVERY _EnterMode (3D entry); without
                // a guard, Setup re-malloc'd buffers (m_AlphaStack/m_SolidStack=SurfaceItemType,
                // Dyn2DVertexBuffer=D3DDYNVERTEX, SimpleBuffer=D3DSIMPLEVERTEX) WITHOUT freeing the previous
                // -> ~20MB/entry -> std::bad_alloc. The buffers are fixed-size and don't depend on device/resolution
                // -> no reinitialization needed.
                static bool s_dxEngineSetupDone = false;

                if (g_bUse_DX_Engine and not s_dxEngineSetupDone)
                {
                    TheDXEngine.Setup();	// #34 C1: statics/materials/lighting/2D
                    TheVbManager.Setup();	// base VBs as D3D11 mirrors
                    s_dxEngineSetupDone = true;
                    MonoPrint("D3D11: DXEngine + VbManager initialized (once)\n");
                }
                // PHASE 1: windowed D3D11 -- a window with frame/title/controls, client = nWidth x nHeight.
                SetWindowLong(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
                RECT rcW = { 0, 0, nWidth, nHeight };
                AdjustWindowRect(&rcW, WS_OVERLAPPEDWINDOW, FALSE);
                SetWindowPos(hWnd, NULL, 0, 0, rcW.right - rcW.left, rcW.bottom - rcW.top,
                             SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                return true;
            }
            MonoPrint("DXContext::Init - D3D11 init failed, fallback D3D7\n");
            delete g_pD3D11Backend; g_pD3D11Backend = NULL; g_bUseD3D11 = false;
        }

        // Create DDRAW object
        CheckHR(DirectDrawCreateEx(&m_guidDD, (void **) &m_pDD, IID_IDirectDraw7, NULL));

        m_pcapsDD->dwSize = sizeof(*m_pcapsDD);
        CheckHR(m_pDD->GetCaps(m_pcapsDD, NULL));
        CheckHR(m_pDD->GetDeviceIdentifier(m_pDevID, NULL));

        sprintf(g_CardDetails, "DXContext::Init - DriverInfo - \"%s\" - \"%s\", Vendor: %d, Device: %d, SubSys: %d, Rev: %d, Product: %d, Version: %d, SubVersion: %d, Build: %d\n",
                m_pDevID->szDriver, m_pDevID->szDescription,
                m_pDevID->dwVendorId, m_pDevID->dwDeviceId, m_pDevID->dwSubSysId, m_pDevID->dwRevision,
                HIWORD(m_pDevID->liDriverVersion.HighPart), LOWORD(m_pDevID->liDriverVersion.HighPart),
                HIWORD(m_pDevID->liDriverVersion.LowPart), LOWORD(m_pDevID->liDriverVersion.LowPart)); // JB 010215
        MonoPrint("%s", g_CardDetails);  // JB 010215

        DWORD m_dwCoopFlags = NULL;
        m_dwCoopFlags or_eq DDSCL_FPUPRESERVE; // OW FIXME: check if this can be eliminated by eliminating ALL controlfp calls in all files

        if (g_bForceDXMultiThreadedCoopLevel) m_dwCoopFlags or_eq DDSCL_MULTITHREADED;

        if (bFullscreen) m_dwCoopFlags or_eq DDSCL_EXCLUSIVE bitor DDSCL_FULLSCREEN bitor DDSCL_ALLOWREBOOT;
        else m_dwCoopFlags or_eq DDSCL_NORMAL;

        CheckHR(m_pDD->SetCooperativeLevel(m_hWnd, m_dwCoopFlags));

        if (bFullscreen) CheckHR(m_pDD->SetDisplayMode(nWidth, nHeight, nDepth, 0, NULL));

        /*
         // Vendor specific workarounds
         if(IsEqualGUID(m_pDevID->guidDeviceIdentifier, __uuidof(DEVGUID_3DFX_VOODOO2)) and not bFlip)
         {
         // The V2 (Beta 1.0 DX Driver) cannot render to offscreen plain surfaces only to flipping primary surfaces
         m_guidD3D = IID_IDirect3DRGBDevice; // force software renderer
         }
        */

        //JAM 25Oct03 - Let's avoid user error and disable these.
        // if(IsEqualIID(m_guidD3D, IID_IDirect3DRGBDevice) or IsEqualIID(m_guidD3D, IID_IDirect3DRefDevice) or
        // IsEqualIID(m_guidD3D, IID_IDirect3DRampDevice) or IsEqualIID(m_guidD3D, IID_IDirect3DMMXDevice))
        // m_eDeviceCategory = D3DDeviceCategory_Software;
        // if(IsEqualIID(m_guidD3D, IID_IDirect3DHALDevice))
        m_eDeviceCategory = D3DDeviceCategory_Hardware;
        // else if(IsEqualIID(m_guidD3D, IID_IDirect3DTnLHalDevice))
        //FIXME: TnL
        // m_eDeviceCategory = D3DDeviceCategory_Hardware_TNL;
        // else
        // {
        // m_eDeviceCategory = D3DDeviceCategory_Software; // assume its software
        // ShiAssert(false); // check this
        // }
        //JAM

        return true;
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
    ((PIXELFMT_ARRAY *) parr)->clear();
    m_pD3D->EnumZBufferFormats(m_guidD3D, EnumZBufferFormatsCallback, parr);
}

HRESULT CALLBACK DXContext::EnumZBufferFormatsCallback(LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext)
{
    if (lpDDPixFmt->dwFlags bitand DDPF_ZBUFFER)
    {
        PIXELFMT_ARRAY *pThis = (PIXELFMT_ARRAY *)lpContext;
        pThis->push_back(*lpDDPixFmt);
    }

    return D3DENUMRET_OK;
}

void DXContext::AttachDepthBuffer(IDirectDrawSurface7 *p)
{
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
}

void DXContext::CheckCaps()
{
#ifdef _DEBUG
    MonoPrint("-- DXContext - Start of Caps report\n");

    if (m_pD3DHWDeviceDesc->dwDevCaps bitand D3DDEVCAPS_SEPARATETEXTUREMEMORIES)
        MonoPrint(" Device has separate texture memories per stage. \n");

    if (m_pD3DHWDeviceDesc->dwDevCaps bitand D3DDEVCAPS_TEXTURENONLOCALVIDMEM)
        MonoPrint(" Device supports AGP texturing\n");

    if ( not (m_pD3DHWDeviceDesc->dwDevCaps bitand D3DDEVCAPS_FLOATTLVERTEX))
        MonoPrint(" Device does not accepts floating point for post-transform vertex data. \n");

    if ( not (m_pD3DHWDeviceDesc->dwDevCaps bitand D3DDEVCAPS_TLVERTEXSYSTEMMEMORY))
        MonoPrint(" Device does not accept TL VBs in system memory.\n");

    if ( not (m_pD3DHWDeviceDesc->dwDevCaps bitand D3DDEVCAPS_TLVERTEXVIDEOMEMORY))
        MonoPrint(" Device does not accept TL VBs in video memory.\n");

    if ( not (m_pD3DHWDeviceDesc->dpcTriCaps.dwRasterCaps bitand D3DPRASTERCAPS_DITHER))
        MonoPrint(" No dithering\n");

    if ( not (m_pD3DHWDeviceDesc->dpcTriCaps.dwRasterCaps bitand D3DPRASTERCAPS_FOGRANGE))
        MonoPrint(" No range based fog\n");

    if ( not (m_pD3DHWDeviceDesc->dpcTriCaps.dwRasterCaps bitand D3DPRASTERCAPS_FOGVERTEX))
        MonoPrint(" No vertex fog\n");

    if ( not (m_pD3DHWDeviceDesc->dpcTriCaps.dwRasterCaps bitand D3DPRASTERCAPS_ZTEST))
        MonoPrint(" No Z Test support\n");

    if (m_pD3DHWDeviceDesc->dpcTriCaps.dwAlphaCmpCaps == D3DPCMPCAPS_ALWAYS or
        m_pD3DHWDeviceDesc->dpcTriCaps.dwAlphaCmpCaps == D3DPCMPCAPS_NEVER)
        MonoPrint(" No Alpha Test support\n");

    if ( not (m_pD3DHWDeviceDesc->dpcTriCaps.dwSrcBlendCaps bitand D3DPBLENDCAPS_SRCALPHA))
        MonoPrint(" SrcBlend SRCALPHA not supported\n");

    if ( not (m_pD3DHWDeviceDesc->dpcTriCaps.dwDestBlendCaps bitand D3DPBLENDCAPS_INVSRCALPHA))
        MonoPrint(" DestBlend INVSRCALPHA not supported\n");

    if ( not (m_pcapsDD->dwCaps bitand DDCAPS_COLORKEY and 
          m_pcapsDD->dwCKeyCaps bitand DDCKEYCAPS_DESTBLT and 
          m_pD3DHWDeviceDesc->dwDevCaps bitand D3DDEVCAPS_DRAWPRIMTLVERTEX))
        MonoPrint(" Insufficient color key support\n");

    if ( not (m_pD3DHWDeviceDesc->dpcTriCaps.dwShadeCaps bitand D3DPSHADECAPS_ALPHAFLATBLEND))
        MonoPrint(" No alpha blending with flat shading\n");

    if ( not (m_pD3DHWDeviceDesc->dpcTriCaps.dwShadeCaps bitand D3DPSHADECAPS_COLORGOURAUDRGB))
        MonoPrint(" No gouraud shading\n");

    if ( not (m_pD3DHWDeviceDesc->dpcTriCaps.dwShadeCaps bitand D3DPSHADECAPS_SPECULARFLATRGB))
        MonoPrint(" No specular flat shading\n");

    if ( not (m_pD3DHWDeviceDesc->dpcTriCaps.dwShadeCaps bitand D3DPSHADECAPS_SPECULARGOURAUDRGB))
        MonoPrint(" No specular gouraud shading\n");

    if ( not (m_pD3DHWDeviceDesc->dpcTriCaps.dwShadeCaps bitand D3DPSHADECAPS_FOGGOURAUD))
        MonoPrint(" No gouraud fog\n");

    if ( not (m_pD3DHWDeviceDesc->dpcTriCaps.dwTextureCaps bitand D3DPTEXTURECAPS_ALPHA))
        MonoPrint(" No alpha textures\n");

    if ( not (m_pD3DHWDeviceDesc->dpcTriCaps.dwTextureCaps bitand D3DPTEXTURECAPS_ALPHAPALETTE))
        MonoPrint(" No palettized alpha textures\n");

    if ( not (m_pD3DHWDeviceDesc->dpcTriCaps.dwTextureCaps bitand D3DPTEXTURECAPS_COLORKEYBLEND))
        MonoPrint(" No color key blending support\n");

    if (m_pD3DHWDeviceDesc->dpcTriCaps.dwTextureCaps bitand D3DPTEXTURECAPS_POW2)
        MonoPrint(" Textures must be power of 2\n");

    if (m_pD3DHWDeviceDesc->dpcTriCaps.dwTextureCaps bitand D3DPTEXTURECAPS_SQUAREONLY)
        MonoPrint(" Textures must be square\n");

    if ( not (m_pD3DHWDeviceDesc->dpcTriCaps.dwTextureCaps bitand D3DPTEXTURECAPS_TRANSPARENCY))
        MonoPrint(" No texture transparency\n");

    if ( not (m_pD3DHWDeviceDesc->dwTextureOpCaps bitand D3DTEXOPCAPS_BLENDDIFFUSEALPHA)) // required for MPR_TF_ALPHA
        MonoPrint(" No D3DTOP_BLENDDIFFUSEALPHA (MPR_TF_ALPHA wont work ie. smoke trails)\n");

    MonoPrint("-- DXContext - End of Caps report\n");
#endif
}

bool DXContext::ValidateD3DDevice()
{
#ifdef _DEBUG
    DWORD dwPasses = 0;
    HRESULT hr = m_pD3DD->ValidateDevice(&dwPasses);

    if (FAILED(hr))
    {
        char *strError = "Unknown error";

        switch (hr)
        {
            case DDERR_INVALIDOBJECT:
                strError = "DDERR_INVALIDOBJECT";
                break;

            case DDERR_INVALIDPARAMS:
                strError = "DDERR_INVALIDPARAMS";
                break;

            case D3DERR_CONFLICTINGTEXTUREFILTER:
                strError = "D3DERR_CONFLICTINGTEXTUREFILTER";
                break;

            case D3DERR_CONFLICTINGTEXTUREPALETTE:
                strError = "D3DERR_CONFLICTINGTEXTUREPALETTE";
                break;

            case D3DERR_TOOMANYOPERATIONS:
                strError = "D3DERR_TOOMANYOPERATIONS";
                break;

            case D3DERR_UNSUPPORTEDALPHAARG:
                strError = "D3DERR_UNSUPPORTEDALPHAARG";
                break;

            case D3DERR_UNSUPPORTEDALPHAOPERATION:
                strError = "D3DERR_UNSUPPORTEDALPHAOPERATION";
                break;

            case D3DERR_UNSUPPORTEDCOLORARG:
                strError = "D3DERR_UNSUPPORTEDCOLORARG";
                break;

            case D3DERR_UNSUPPORTEDCOLOROPERATION:
                strError = "D3DERR_UNSUPPORTEDCOLOROPERATION";
                break;

            case D3DERR_UNSUPPORTEDFACTORVALUE:
                strError = "D3DERR_UNSUPPORTEDFACTORVALUE";
                break;

            case D3DERR_UNSUPPORTEDTEXTUREFILTER:
                strError = "D3DERR_UNSUPPORTEDTEXTUREFILTER";
                break;

            case D3DERR_WRONGTEXTUREFORMAT:
                strError = "D3DERR_WRONGTEXTUREFORMAT";
                break;
        }

        MonoPrint(">>> DXContext::ValidateD3DDevice: ValidateDevice failed with 0x%X (%s) - %d passes required\n",
                  hr, strError, dwPasses);
    }

    return SUCCEEDED(hr);
#else
    return true;
#endif
}

DWORD DXContext::TestCooperativeLevel()
{
    if (g_bUseD3D11) return DD_OK;	// PHASE 1: no DDraw coop under D3D11
    HRESULT hr = m_pDD->TestCooperativeLevel();

    if (hr not_eq DD_OK)
    {
        do
        {
            Sleep(100);
            hr = m_pDD->TestCooperativeLevel();
        }
        while (hr not_eq DD_OK);

        return S_FALSE; // surface were lost
    }

    return DD_OK; // no change
}
