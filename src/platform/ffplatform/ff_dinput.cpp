// Artscout - 2026 (#104, Linux port Ф3): DirectInput8 implemented ON TOP OF SDL3.
//
// The engine's input layer (sim/siminput/*, sim/simlib/Io.cpp, the control-binding UI) was written against the
// DirectInput8 API, and DI's data formats (DIK scancodes, DIJOYSTATE2, the calibration structs) are ALSO the on-disk
// wire format of the .key/.dat binding files. So the correct Linux port keeps that API and swaps the driver
// underneath -- every DI method here reads real state from SDL3 (SDL_Joystick / SDL_GetKeyboardState /
// SDL_GetMouseState / SDL_Haptic). The interface *declarations* live in the SDL-free win32 shim (dinput.h); this is
// the single bridge TU that is allowed to pull SDL, and it lives in the platform layer (ffplatform) that links SDL3.
//
// Windows keeps the real DirectInput: this whole file is a no-op there.
#ifndef _WIN32

#include <windows.h>
#include "dinput.h"

#include <SDL3/SDL.h>

#include <cstring>
#include <cstdlib>
#include <vector>
#include <deque>

// ====================================================================================================================
//  Well-known GUIDs / data formats (addresses must be unique; the engine identifies devices/axes/effects by these)
// ====================================================================================================================
// Distinct Data1 values so memcmp/identity works; the exact bytes are private to this build (they are not persisted
// as-is -- the persisted device id is SDL's stable joystick GUID, copied into DIDEVICEINSTANCE.guidInstance below).
#define FFGUID(n) {(n), 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}}
extern "C"
{
    const GUID GUID_SysMouse = FFGUID(0x01);
    const GUID GUID_SysKeyboard = FFGUID(0x02);
    const GUID GUID_XAxis = FFGUID(0x10);
    const GUID GUID_YAxis = FFGUID(0x11);
    const GUID GUID_ZAxis = FFGUID(0x12);
    const GUID GUID_RxAxis = FFGUID(0x13);
    const GUID GUID_RyAxis = FFGUID(0x14);
    const GUID GUID_RzAxis = FFGUID(0x15);
    const GUID GUID_Slider = FFGUID(0x16);
    const GUID GUID_POV = FFGUID(0x17);
    const GUID GUID_Button = FFGUID(0x18);
    const GUID GUID_ConstantForce = FFGUID(0x20);
    const GUID GUID_RampForce = FFGUID(0x21);
    const GUID GUID_Square = FFGUID(0x22);
    const GUID GUID_Sine = FFGUID(0x23);
    const GUID GUID_Triangle = FFGUID(0x24);
    const GUID GUID_SawtoothUp = FFGUID(0x25);
    const GUID GUID_SawtoothDown = FFGUID(0x26);
    const GUID GUID_Spring = FFGUID(0x27);
    const GUID GUID_Damper = FFGUID(0x28);
    const GUID GUID_Inertia = FFGUID(0x29);
    const GUID GUID_Friction = FFGUID(0x2A);
    const GUID GUID_CustomForce = FFGUID(0x2B);
    const IID IID_IDirectInput8 = FFGUID(0x80);
    const IID IID_IDirectInput7 = FFGUID(0x81);
    const IID IID_IDirectInputDevice8 = FFGUID(0x82);
    const IID IID_IDirectInputDevice7 = FFGUID(0x83);
#ifndef GUID_NULL_DEFINED_IMPL
    const GUID GUID_NULL = FFGUID(0x00);
#endif
    // The engine only takes the ADDRESS of these formats (to tell mouse/keyboard/joystick apart in SetDataFormat), so a
    // zeroed object per format is sufficient and honest -- we do not parse rgodf, we drive state through SDL by kind.
    DIDATAFORMAT c_dfDIJoystick2 = {0}, c_dfDIJoystick = {0}, c_dfDIMouse = {0},
                 c_dfDIMouse2 = {0}, c_dfDIKeyboard = {0};
}

// ====================================================================================================================
//  SDL <-> DirectInput keyboard scancode mapping
// ====================================================================================================================
// DIK_* are PS/2 set-1 scancodes; SDL_Scancode is USB-HID-usage based. This table is the wire bridge: the engine and
// the .key files speak DIK, SDL speaks HID. Only the keys the sim binds are mapped; unmapped keys report nothing.
static unsigned char g_sdlToDik[SDL_SCANCODE_COUNT];
static bool g_dikTableBuilt = false;

static void BuildDikTable()
{
    if (g_dikTableBuilt)
        return;
    memset(g_sdlToDik, 0, sizeof(g_sdlToDik));
#define M(sc, dik) g_sdlToDik[sc] = (unsigned char)(dik)
    M(SDL_SCANCODE_ESCAPE, DIK_ESCAPE);
    M(SDL_SCANCODE_1, DIK_1);
    M(SDL_SCANCODE_2, DIK_2);
    M(SDL_SCANCODE_3, DIK_3);
    M(SDL_SCANCODE_4, DIK_4);
    M(SDL_SCANCODE_5, DIK_5);
    M(SDL_SCANCODE_6, DIK_6);
    M(SDL_SCANCODE_7, DIK_7);
    M(SDL_SCANCODE_8, DIK_8);
    M(SDL_SCANCODE_9, DIK_9);
    M(SDL_SCANCODE_0, DIK_0);
    M(SDL_SCANCODE_MINUS, DIK_MINUS);
    M(SDL_SCANCODE_EQUALS, DIK_EQUALS);
    M(SDL_SCANCODE_BACKSPACE, DIK_BACK);
    M(SDL_SCANCODE_TAB, DIK_TAB);
    M(SDL_SCANCODE_Q, DIK_Q);
    M(SDL_SCANCODE_W, DIK_W);
    M(SDL_SCANCODE_E, DIK_E);
    M(SDL_SCANCODE_R, DIK_R);
    M(SDL_SCANCODE_T, DIK_T);
    M(SDL_SCANCODE_Y, DIK_Y);
    M(SDL_SCANCODE_U, DIK_U);
    M(SDL_SCANCODE_I, DIK_I);
    M(SDL_SCANCODE_O, DIK_O);
    M(SDL_SCANCODE_P, DIK_P);
    M(SDL_SCANCODE_LEFTBRACKET, DIK_LBRACKET);
    M(SDL_SCANCODE_RIGHTBRACKET, DIK_RBRACKET);
    M(SDL_SCANCODE_RETURN, DIK_RETURN);
    M(SDL_SCANCODE_LCTRL, DIK_LCONTROL);
    M(SDL_SCANCODE_A, DIK_A);
    M(SDL_SCANCODE_S, DIK_S);
    M(SDL_SCANCODE_D, DIK_D);
    M(SDL_SCANCODE_F, DIK_F);
    M(SDL_SCANCODE_G, DIK_G);
    M(SDL_SCANCODE_H, DIK_H);
    M(SDL_SCANCODE_J, DIK_J);
    M(SDL_SCANCODE_K, DIK_K);
    M(SDL_SCANCODE_L, DIK_L);
    M(SDL_SCANCODE_SEMICOLON, DIK_SEMICOLON);
    M(SDL_SCANCODE_APOSTROPHE, DIK_APOSTROPHE);
    M(SDL_SCANCODE_GRAVE, DIK_GRAVE);
    M(SDL_SCANCODE_LSHIFT, DIK_LSHIFT);
    M(SDL_SCANCODE_BACKSLASH, DIK_BACKSLASH);
    M(SDL_SCANCODE_Z, DIK_Z);
    M(SDL_SCANCODE_X, DIK_X);
    M(SDL_SCANCODE_C, DIK_C);
    M(SDL_SCANCODE_V, DIK_V);
    M(SDL_SCANCODE_B, DIK_B);
    M(SDL_SCANCODE_N, DIK_N);
    M(SDL_SCANCODE_M, DIK_M);
    M(SDL_SCANCODE_COMMA, DIK_COMMA);
    M(SDL_SCANCODE_PERIOD, DIK_PERIOD);
    M(SDL_SCANCODE_SLASH, DIK_SLASH);
    M(SDL_SCANCODE_RSHIFT, DIK_RSHIFT);
    M(SDL_SCANCODE_KP_MULTIPLY, DIK_MULTIPLY);
    M(SDL_SCANCODE_LALT, DIK_LMENU);
    M(SDL_SCANCODE_SPACE, DIK_SPACE);
    M(SDL_SCANCODE_CAPSLOCK, DIK_CAPITAL);
    M(SDL_SCANCODE_F1, DIK_F1);
    M(SDL_SCANCODE_F2, DIK_F2);
    M(SDL_SCANCODE_F3, DIK_F3);
    M(SDL_SCANCODE_F4, DIK_F4);
    M(SDL_SCANCODE_F5, DIK_F5);
    M(SDL_SCANCODE_F6, DIK_F6);
    M(SDL_SCANCODE_F7, DIK_F7);
    M(SDL_SCANCODE_F8, DIK_F8);
    M(SDL_SCANCODE_F9, DIK_F9);
    M(SDL_SCANCODE_F10, DIK_F10);
    M(SDL_SCANCODE_NUMLOCKCLEAR, DIK_NUMLOCK);
    M(SDL_SCANCODE_SCROLLLOCK, DIK_SCROLL);
    M(SDL_SCANCODE_KP_7, DIK_NUMPAD7);
    M(SDL_SCANCODE_KP_8, DIK_NUMPAD8);
    M(SDL_SCANCODE_KP_9, DIK_NUMPAD9);
    M(SDL_SCANCODE_KP_MINUS, DIK_SUBTRACT);
    M(SDL_SCANCODE_KP_4, DIK_NUMPAD4);
    M(SDL_SCANCODE_KP_5, DIK_NUMPAD5);
    M(SDL_SCANCODE_KP_6, DIK_NUMPAD6);
    M(SDL_SCANCODE_KP_PLUS, DIK_ADD);
    M(SDL_SCANCODE_KP_1, DIK_NUMPAD1);
    M(SDL_SCANCODE_KP_2, DIK_NUMPAD2);
    M(SDL_SCANCODE_KP_3, DIK_NUMPAD3);
    M(SDL_SCANCODE_KP_0, DIK_NUMPAD0);
    M(SDL_SCANCODE_KP_PERIOD, DIK_DECIMAL);
    M(SDL_SCANCODE_NONUSBACKSLASH, DIK_OEM_102);
    M(SDL_SCANCODE_F11, DIK_F11);
    M(SDL_SCANCODE_F12, DIK_F12);
    M(SDL_SCANCODE_F13, DIK_F13);
    M(SDL_SCANCODE_F14, DIK_F14);
    M(SDL_SCANCODE_F15, DIK_F15);
    M(SDL_SCANCODE_KP_ENTER, DIK_NUMPADENTER);
    M(SDL_SCANCODE_RCTRL, DIK_RCONTROL);
    M(SDL_SCANCODE_KP_DIVIDE, DIK_DIVIDE);
    M(SDL_SCANCODE_RALT, DIK_RMENU);
    M(SDL_SCANCODE_PAUSE, DIK_PAUSE);
    M(SDL_SCANCODE_HOME, DIK_HOME);
    M(SDL_SCANCODE_UP, DIK_UP);
    M(SDL_SCANCODE_PAGEUP, DIK_PRIOR);
    M(SDL_SCANCODE_LEFT, DIK_LEFT);
    M(SDL_SCANCODE_RIGHT, DIK_RIGHT);
    M(SDL_SCANCODE_END, DIK_END);
    M(SDL_SCANCODE_DOWN, DIK_DOWN);
    M(SDL_SCANCODE_PAGEDOWN, DIK_NEXT);
    M(SDL_SCANCODE_INSERT, DIK_INSERT);
    M(SDL_SCANCODE_DELETE, DIK_DELETE);
    M(SDL_SCANCODE_LGUI, DIK_LWIN);
    M(SDL_SCANCODE_RGUI, DIK_RWIN);
    M(SDL_SCANCODE_APPLICATION, DIK_APPS);
#undef M
    g_dikTableBuilt = true;
}

// Exposed for ff_events.cpp: translate an SDL scancode to its DIK / PS2 set-1 scan code -- the value the 2D UI
// decodes out of WM_KEYDOWN's lParam (bits 16-24) and feeds to AsciiChar()/CheckKeyboard(). Builds on first use.
unsigned char FF_SdlScancodeToDik(int sdlScancode)
{
    BuildDikTable();
    if (sdlScancode < 0 || sdlScancode >= (int)SDL_SCANCODE_COUNT)
        return 0;
    return g_sdlToDik[sdlScancode];
}

// ====================================================================================================================
//  Backend device state
// ====================================================================================================================
enum DIKind
{
    KIND_MOUSE,
    KIND_KEYBOARD,
    KIND_JOYSTICK
};

struct AxisCfg
{
    LONG lMin = 0, lMax = 65535;
    DWORD deadzone = 0, saturation = 10000;
};

struct DIDeviceImpl
{
    DIKind kind;
    int refcount = 1;
    bool acquired = false;
    DWORD coopFlags = 0;

    // joystick
    SDL_Joystick* joy = nullptr;
    SDL_Haptic* haptic = nullptr;
    SDL_JoystickID sdlId = 0;
    GUID instanceGuid = {0};
    char name[MAX_PATH] = {0};

    // per-axis config keyed by DIJOFS offset (X..RZ, sliders). AUTOCENTER toggled here for FF pass-through.
    AxisCfg axis[8];
    bool autocenter = true;

    // buffered-keyboard/mouse edge detection: last snapshot
    unsigned char prevKeyDik[256] = {0}; // 0x80 = down (indexed by DIK)
    Uint32 prevMouseButtons = 0;
    bool haveMousePrev = false;
};

struct DIObjectImpl
{
    int refcount = 1;
};

// One SDL init for the input subsystems (idempotent; the window path may have inited video already).
static void EnsureSdlInput()
{
    static bool done = false;
    if (done)
        return;
    SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD | SDL_INIT_HAPTIC);
    done = true;
}

// Map a DIJOFS_* byte offset to an index 0..7 in DIDeviceImpl::axis / to the SDL axis index. The engine's DIJOFS_X..
// are FIELD_OFFSETs into DIJOYSTATE, so recover the ordinal by dividing by sizeof(LONG); sliders follow the 6 axes.
static int OffsetToAxisIndex(DWORD dwOfs)
{
    if (dwOfs == (DWORD)DIJOFS_X)
        return 0;
    if (dwOfs == (DWORD)DIJOFS_Y)
        return 1;
    if (dwOfs == (DWORD)DIJOFS_Z)
        return 2;
    if (dwOfs == (DWORD)DIJOFS_RX)
        return 3;
    if (dwOfs == (DWORD)DIJOFS_RY)
        return 4;
    if (dwOfs == (DWORD)DIJOFS_RZ)
        return 5;
    if (dwOfs == (DWORD)DIJOFS_SLIDER(0))
        return 6;
    if (dwOfs == (DWORD)DIJOFS_SLIDER(1))
        return 7;
    return -1;
}

// Apply DirectInput range/deadzone/saturation to a raw SDL axis value (-32768..32767) -> the configured DI range.
static LONG ShapeAxis(const AxisCfg& c, Sint16 raw)
{
    double s = (double)raw / 32767.0; // -1..1
    if (s < -1.0)
        s = -1.0;
    else if (s > 1.0)
        s = 1.0;
    double dz = c.deadzone / 10000.0; // fractions
    double sat = c.saturation / 10000.0;
    double a = s < 0 ? -s : s;
    if (a <= dz)
        a = 0.0;
    else if (sat > dz)
    {
        a = (a - dz) / (sat - dz);
        if (a > 1.0)
            a = 1.0;
    }
    double shaped = (s < 0) ? -a : a; // -1..1 shaped
    double norm = shaped * 0.5 + 0.5; // 0..1
    return (LONG)(c.lMin + norm * (double)(c.lMax - c.lMin) + 0.5);
}

// ====================================================================================================================
//  IDirectInputDevice8A methods
// ====================================================================================================================
ULONG IDirectInputDevice8A::AddRef()
{
    DIDeviceImpl* d = (DIDeviceImpl*)impl;
    return d ? ++d->refcount : 1;
}
ULONG IDirectInputDevice8A::Release()
{
    DIDeviceImpl* d = (DIDeviceImpl*)impl;
    if (!d)
    {
        delete this;
        return 0;
    }
    if (--d->refcount > 0)
        return (ULONG)d->refcount;
    if (d->haptic)
        SDL_CloseHaptic(d->haptic);
    if (d->joy)
        SDL_CloseJoystick(d->joy);
    delete d;
    impl = nullptr;
    delete this;
    return 0;
}

HRESULT IDirectInputDevice8A::SetDataFormat(LPCDIDATAFORMAT)
{
    return DI_OK;
} // kind already fixed at create
HRESULT IDirectInputDevice8A::SetCooperativeLevel(HWND, DWORD f)
{
    ((DIDeviceImpl*)impl)->coopFlags = f;
    return DI_OK;
}
HRESULT IDirectInputDevice8A::SetEventNotification(HANDLE)
{
    return DI_OK;
} // no buffered win32 event needed

HRESULT IDirectInputDevice8A::Acquire()
{
    ((DIDeviceImpl*)impl)->acquired = true;
    return DI_OK;
}
HRESULT IDirectInputDevice8A::Unacquire()
{
    ((DIDeviceImpl*)impl)->acquired = false;
    return DI_OK;
}

HRESULT IDirectInputDevice8A::Poll()
{
    DIDeviceImpl* d = (DIDeviceImpl*)impl;
    if (d->kind == KIND_JOYSTICK)
        SDL_UpdateJoysticks();
    else
        SDL_PumpEvents();
    return DI_OK;
}

HRESULT IDirectInputDevice8A::SetProperty(REFGUID rguidProp,
                                          LPCDIPROPHEADER pdiph)
{
    DIDeviceImpl* d = (DIDeviceImpl*)impl;
    UINT_PTR prop =
        (UINT_PTR)&rguidProp; // MAKEDIPROP recovers the small-int id from the address
    if (prop == 9 /*DIPROP_AUTOCENTER*/)
    {
        d->autocenter = (((LPDIPROPDWORD)pdiph)->dwData != 0);
        return DI_OK;
    }
    if (prop == 1 /*DIPROP_BUFFERSIZE*/)
        return DI_OK; // our buffered read is diff-based; no fixed ring to size
    if (!pdiph)
        return DIERR_INVALIDPARAM;
    if (pdiph->dwHow == DIPH_BYOFFSET)
    {
        int ai = OffsetToAxisIndex(pdiph->dwObj);
        if (ai < 0)
            return DI_OK;
        if (prop == 4 /*DIPROP_RANGE*/)
        {
            LPDIPROPRANGE r = (LPDIPROPRANGE)pdiph;
            d->axis[ai].lMin = r->lMin;
            d->axis[ai].lMax = r->lMax;
        }
        else if (prop == 5 /*DIPROP_DEADZONE*/)
        {
            d->axis[ai].deadzone = ((LPDIPROPDWORD)pdiph)->dwData;
        }
        else if (prop == 6 /*DIPROP_SATURATION*/)
        {
            d->axis[ai].saturation = ((LPDIPROPDWORD)pdiph)->dwData;
        }
        // DIPROP_CPOINTS (20): custom axis shaping -- honoured by the engine's own post-processing, not the driver.
    }
    return DI_OK;
}

HRESULT IDirectInputDevice8A::GetProperty(REFGUID rguidProp,
                                          LPDIPROPHEADER pdiph)
{
    DIDeviceImpl* d = (DIDeviceImpl*)impl;
    UINT_PTR prop = (UINT_PTR)&rguidProp;
    if (prop == 4 /*DIPROP_RANGE*/ && pdiph && pdiph->dwHow == DIPH_BYOFFSET)
    {
        int ai = OffsetToAxisIndex(pdiph->dwObj);
        if (ai >= 0)
        {
            LPDIPROPRANGE r = (LPDIPROPRANGE)pdiph;
            r->lMin = d->axis[ai].lMin;
            r->lMax = d->axis[ai].lMax;
            return DI_OK;
        }
    }
    return DI_OK;
}

HRESULT IDirectInputDevice8A::GetDeviceState(DWORD cbData, LPVOID lpvData)
{
    DIDeviceImpl* d = (DIDeviceImpl*)impl;
    if (!d->acquired)
        return DIERR_NOTACQUIRED;
    if (!lpvData)
        return DIERR_INVALIDPARAM;

    if (d->kind == KIND_KEYBOARD)
    {
        BuildDikTable();
        SDL_PumpEvents();
        int n = 0;
        const bool* ks = SDL_GetKeyboardState(&n);
        unsigned char* out =
            (unsigned char*)lpvData; // 256 bytes, DIK-indexed, 0x80 = down
        memset(out, 0, cbData);
        for (int sc = 0; sc < n && sc < SDL_SCANCODE_COUNT; ++sc)
            if (ks[sc])
            {
                unsigned char dik = g_sdlToDik[sc];
                if (dik && dik < cbData)
                    out[dik] = 0x80;
            }
        return DI_OK;
    }

    if (d->kind == KIND_MOUSE)
    {
        SDL_PumpEvents();
        float rx = 0, ry = 0;
        SDL_MouseButtonFlags b = SDL_GetRelativeMouseState(&rx, &ry);
        if (cbData >= sizeof(DIMOUSESTATE))
        {
            DIMOUSESTATE* ms = (DIMOUSESTATE*)lpvData;
            memset(ms, 0, cbData);
            ms->lX = (LONG)rx;
            ms->lY = (LONG)ry;
            ms->lZ = 0;
            if (b & SDL_BUTTON_LMASK)
                ms->rgbButtons[0] = 0x80;
            if (b & SDL_BUTTON_RMASK)
                ms->rgbButtons[1] = 0x80;
            if (b & SDL_BUTTON_MMASK)
                ms->rgbButtons[2] = 0x80;
        }
        return DI_OK;
    }

    // joystick -> DIJOYSTATE2
    if (!d->joy)
        return DIERR_INPUTLOST;
    SDL_UpdateJoysticks();
    DIJOYSTATE2* js = (DIJOYSTATE2*)lpvData;
    memset(js, 0, cbData);
    int nAxes = SDL_GetNumJoystickAxes(d->joy);
    LONG* axisOut[8] = {&js->lX,           &js->lY,          &js->lZ,
                        &js->lRx,          &js->lRy,         &js->lRz,
                        &js->rglSlider[0], &js->rglSlider[1]};
    for (int i = 0; i < 8 && i < nAxes; ++i)
        *axisOut[i] = ShapeAxis(d->axis[i], SDL_GetJoystickAxis(d->joy, i));
    // POV hats
    int nHats = SDL_GetNumJoystickHats(d->joy);
    for (int i = 0; i < 4 && i < nHats; ++i)
    {
        Uint8 h = SDL_GetJoystickHat(d->joy, i);
        DWORD ang = 0xFFFFFFFF; // centered
        switch (h)
        {
        case SDL_HAT_UP:
            ang = 0;
            break;
        case SDL_HAT_RIGHTUP:
            ang = 4500;
            break;
        case SDL_HAT_RIGHT:
            ang = 9000;
            break;
        case SDL_HAT_RIGHTDOWN:
            ang = 13500;
            break;
        case SDL_HAT_DOWN:
            ang = 18000;
            break;
        case SDL_HAT_LEFTDOWN:
            ang = 22500;
            break;
        case SDL_HAT_LEFT:
            ang = 27000;
            break;
        case SDL_HAT_LEFTUP:
            ang = 31500;
            break;
        default:
            ang = 0xFFFFFFFF;
            break;
        }
        js->rgdwPOV[i] = ang;
    }
    int nBtn = SDL_GetNumJoystickButtons(d->joy);
    for (int i = 0; i < 128 && i < nBtn; ++i)
        js->rgbButtons[i] = SDL_GetJoystickButton(d->joy, i) ? 0x80 : 0;
    return DI_OK;
}

HRESULT IDirectInputDevice8A::GetDeviceData(DWORD, LPDIDEVICEOBJECTDATA rgdod,
                                            LPDWORD pdwInOut, DWORD dwFlags)
{
    DIDeviceImpl* d = (DIDeviceImpl*)impl;
    if (!pdwInOut)
        return DIERR_INVALIDPARAM;
    DWORD cap = *pdwInOut;
    *pdwInOut = 0;
    if (!d->acquired)
        return DIERR_NOTACQUIRED;
    // rgdod == NULL is the "how many are pending" / liveness peek (CheckDeviceAcquisition). We are diff-based and
    // never overflow, so report zero pending -- and, crucially, DI_OK, so the caller keeps the device acquired.
    if (!rgdod)
        return DI_OK;

    DWORD count = 0;
    const bool peek = (dwFlags & DIGDD_PEEK) != 0;

    if (d->kind == KIND_KEYBOARD)
    {
        BuildDikTable();
        SDL_PumpEvents();
        int n = 0;
        const bool* ks = SDL_GetKeyboardState(&n);
        unsigned char cur[256];
        memset(cur, 0, sizeof(cur));
        for (int sc = 0; sc < n && sc < SDL_SCANCODE_COUNT; ++sc)
            if (ks[sc])
            {
                unsigned char dik = g_sdlToDik[sc];
                if (dik)
                    cur[dik] = 0x80;
            }
        for (int dik = 1; dik < 256 && count < cap; ++dik)
            if (cur[dik] != d->prevKeyDik[dik])
            {
                rgdod[count].dwOfs = dik;
                rgdod[count].dwData = cur[dik]; // 0x80 down / 0 up
                rgdod[count].dwTimeStamp = SDL_GetTicks();
                rgdod[count].dwSequence = count;
                rgdod[count].uAppData = 0;
                ++count;
            }
        if (!peek)
            memcpy(d->prevKeyDik, cur, sizeof(cur));
        *pdwInOut = count;
        return DI_OK;
    }

    if (d->kind == KIND_MOUSE)
    {
        SDL_PumpEvents();
        float rx = 0, ry = 0;
        SDL_MouseButtonFlags b = SDL_GetRelativeMouseState(&rx, &ry);
        struct Ev
        {
            DWORD ofs;
            DWORD data;
        } evs[8];
        int ne = 0;
        if (rx != 0 && count < cap)
        {
            evs[ne++] = {(DWORD)DIMOFS_X, (DWORD)(LONG)rx};
        }
        if (ry != 0)
        {
            evs[ne++] = {(DWORD)DIMOFS_Y, (DWORD)(LONG)ry};
        }
        if (d->haveMousePrev)
        {
            struct
            {
                Uint32 mask;
                DWORD ofs;
            } btn[3] = {{SDL_BUTTON_LMASK, (DWORD)DIMOFS_BUTTON0},
                        {SDL_BUTTON_RMASK, (DWORD)DIMOFS_BUTTON1},
                        {SDL_BUTTON_MMASK, (DWORD)DIMOFS_BUTTON2}};
            for (int i = 0; i < 3; ++i)
            {
                bool now = (b & btn[i].mask) != 0,
                     was = (d->prevMouseButtons & btn[i].mask) != 0;
                if (now != was && ne < 8)
                    evs[ne++] = {btn[i].ofs, (DWORD)(now ? 0x80 : 0)};
            }
        }
        for (int i = 0; i < ne && count < cap; ++i)
        {
            rgdod[count].dwOfs = evs[i].ofs;
            rgdod[count].dwData = evs[i].data;
            rgdod[count].dwTimeStamp = SDL_GetTicks();
            rgdod[count].dwSequence = count;
            rgdod[count].uAppData = 0;
            ++count;
        }
        if (!peek)
        {
            d->prevMouseButtons = b;
            d->haveMousePrev = true;
        }
        *pdwInOut = count;
        return DI_OK;
    }

    // buffered joystick data is not consumed by the engine (it polls via GetDeviceState); report empty honestly.
    *pdwInOut = 0;
    return DI_OK;
}

HRESULT IDirectInputDevice8A::GetCapabilities(LPDIDEVCAPS caps)
{
    DIDeviceImpl* d = (DIDeviceImpl*)impl;
    if (!caps)
        return DIERR_INVALIDPARAM;
    DWORD keep = caps->dwSize;
    memset(caps, 0, sizeof(DIDEVCAPS));
    caps->dwSize = keep ? keep : sizeof(DIDEVCAPS);
    if (d->kind == KIND_JOYSTICK && d->joy)
    {
        caps->dwDevType = DIDEVTYPE_JOYSTICK;
        caps->dwAxes = (DWORD)SDL_GetNumJoystickAxes(d->joy);
        caps->dwButtons = (DWORD)SDL_GetNumJoystickButtons(d->joy);
        caps->dwPOVs = (DWORD)SDL_GetNumJoystickHats(d->joy);
        if (d->haptic)
            caps->dwFlags |= DIDC_FORCEFEEDBACK;
    }
    return DI_OK;
}

HRESULT IDirectInputDevice8A::GetObjectInfo(LPDIDEVICEOBJECTINSTANCEA pdidoi,
                                            DWORD dwObj, DWORD dwHow)
{
    DIDeviceImpl* d = (DIDeviceImpl*)impl;
    if (!pdidoi)
        return DIERR_INVALIDPARAM;
    if (dwHow == DIPH_BYOFFSET)
    {
        int ai = OffsetToAxisIndex(dwObj);
        if (ai < 0 || (d->joy && ai >= SDL_GetNumJoystickAxes(d->joy)))
            return DIERR_OBJECTNOTFOUND;
        static const GUID* axisGuid[8] = {
            &GUID_XAxis,  &GUID_YAxis,  &GUID_ZAxis,  &GUID_RxAxis,
            &GUID_RyAxis, &GUID_RzAxis, &GUID_Slider, &GUID_Slider};
        static const char* axisName[8] = {
            "X Axis",     "Y Axis",     "Z Axis",   "X Rotation",
            "Y Rotation", "Z Rotation", "Slider 0", "Slider 1"};
        pdidoi->guidType = *axisGuid[ai];
        pdidoi->dwOfs = dwObj;
        pdidoi->dwType = DIDFT_AXIS;
        strncpy(pdidoi->tszName, axisName[ai], MAX_PATH - 1);
        pdidoi->tszName[MAX_PATH - 1] = 0;
        return DI_OK;
    }
    return DIERR_OBJECTNOTFOUND;
}

HRESULT IDirectInputDevice8A::GetDeviceInfo(LPDIDEVICEINSTANCEA pdidi)
{
    DIDeviceImpl* d = (DIDeviceImpl*)impl;
    if (!pdidi)
        return DIERR_INVALIDPARAM;
    DWORD keep = pdidi->dwSize;
    memset(pdidi, 0, sizeof(DIDEVICEINSTANCEA));
    pdidi->dwSize = keep ? keep : sizeof(DIDEVICEINSTANCEA);
    pdidi->guidInstance = d->instanceGuid;
    pdidi->dwDevType = (d->kind == KIND_JOYSTICK) ? DIDEVTYPE_JOYSTICK : 0;
    strncpy(pdidi->tszInstanceName, d->name, MAX_PATH - 1);
    strncpy(pdidi->tszProductName, d->name, MAX_PATH - 1);
    return DI_OK;
}

HRESULT IDirectInputDevice8A::EnumObjects(LPDIENUMDEVICEOBJECTSCALLBACKA cb,
                                          LPVOID pv, DWORD dwFlags)
{
    DIDeviceImpl* d = (DIDeviceImpl*)impl;
    if (!cb || !d->joy)
        return DI_OK;
    if (dwFlags == DIDFT_ALL || (dwFlags & DIDFT_AXIS))
    {
        int nAxes = SDL_GetNumJoystickAxes(d->joy);
        static const DWORD ofs[8] = {
            (DWORD)DIJOFS_X,         (DWORD)DIJOFS_Y,        (DWORD)DIJOFS_Z,
            (DWORD)DIJOFS_RX,        (DWORD)DIJOFS_RY,       (DWORD)DIJOFS_RZ,
            (DWORD)DIJOFS_SLIDER(0), (DWORD)DIJOFS_SLIDER(1)};
        for (int i = 0; i < 8 && i < nAxes; ++i)
        {
            DIDEVICEOBJECTINSTANCEA oi;
            memset(&oi, 0, sizeof(oi));
            oi.dwSize = sizeof(oi);
            if (GetObjectInfo(&oi, ofs[i], DIPH_BYOFFSET) == DI_OK)
                if (cb(&oi, pv) == DIENUM_STOP)
                    break;
        }
    }
    return DI_OK;
}

// ---- force feedback (SDL_Haptic) -----------------------------------------------------------------------------------
HRESULT IDirectInputDevice8A::EnumEffects(LPDIENUMEFFECTSCALLBACKA cb,
                                          LPVOID pv, DWORD dwEffType)
{
    DIDeviceImpl* d = (DIDeviceImpl*)impl;
    if (!cb)
        return DI_OK;
    // Report the standard effect this device can play, if any, so the engine's selector (which stores pei->guid)
    // gets a valid GUID. We only advertise what the SDL_Haptic backing actually supports.
    if (!d->haptic)
        return DI_OK;
    unsigned features = SDL_GetHapticFeatures(d->haptic);
    DIEFFECTINFOA ei;
    memset(&ei, 0, sizeof(ei));
    ei.dwSize = sizeof(ei);
    DWORD t = DIEFT_GETTYPE(dwEffType);
    if ((t == DIEFT_CONSTANTFORCE || t == 0) &&
        (features & SDL_HAPTIC_CONSTANT))
    {
        ei.guid = GUID_ConstantForce;
        ei.dwEffType = DIEFT_CONSTANTFORCE;
        strncpy(ei.tszName, "Constant Force", MAX_PATH - 1);
        if (cb(&ei, pv) == DIENUM_STOP)
            return DI_OK;
    }
    if ((t == DIEFT_PERIODIC || t == 0) && (features & SDL_HAPTIC_SINE))
    {
        ei.guid = GUID_Sine;
        ei.dwEffType = DIEFT_PERIODIC;
        strncpy(ei.tszName, "Sine", MAX_PATH - 1);
        if (cb(&ei, pv) == DIENUM_STOP)
            return DI_OK;
    }
    return DI_OK;
}

// A live SDL haptic effect owned by an IDirectInputEffect.
struct DIEffectImpl
{
    SDL_Haptic* haptic = nullptr;
    int sdlEffectId = -1;
    SDL_HapticEffect eff;
    int refcount = 1;
};

static void DIEffectToSdl(LPCDIEFFECT pe, REFGUID rguid, SDL_HapticEffect& out)
{
    memset(&out, 0, sizeof(out));
    // direction: DI polar coordinates -> SDL polar
    Sint32 dir0 =
        (pe && pe->rglDirection && pe->cAxes >= 1) ? pe->rglDirection[0] : 0;
    // constant force
    if (&rguid == &GUID_ConstantForce)
    {
        out.type = SDL_HAPTIC_CONSTANT;
        out.constant.direction.type = SDL_HAPTIC_POLAR;
        out.constant.direction.dir[0] = dir0;
        out.constant.length = (pe && pe->dwDuration != INFINITE_FF) ?
                                  pe->dwDuration / 1000 :
                                  SDL_HAPTIC_INFINITY;
        if (pe && pe->lpvTypeSpecificParams &&
            pe->cbTypeSpecificParams >= sizeof(DICONSTANTFORCE))
            out.constant.level =
                (Sint16)(((DICONSTANTFORCE*)pe->lpvTypeSpecificParams)
                             ->lMagnitude);
    }
    else
    { // treat every periodic family as sine (the engine only maps sine parameters here)
        out.type = SDL_HAPTIC_SINE;
        out.periodic.direction.type = SDL_HAPTIC_POLAR;
        out.periodic.direction.dir[0] = dir0;
        out.periodic.length = (pe && pe->dwDuration != INFINITE_FF) ?
                                  pe->dwDuration / 1000 :
                                  SDL_HAPTIC_INFINITY;
        if (pe && pe->lpvTypeSpecificParams &&
            pe->cbTypeSpecificParams >= sizeof(DIPERIODIC))
        {
            DIPERIODIC* p = (DIPERIODIC*)pe->lpvTypeSpecificParams;
            out.periodic.magnitude = (Sint16)p->dwMagnitude;
            out.periodic.offset = (Sint16)p->lOffset;
            out.periodic.phase = (Uint16)p->dwPhase;
            out.periodic.period = (Uint16)(p->dwPeriod / 1000);
        }
    }
}

HRESULT IDirectInputDevice8A::CreateEffect(REFGUID rguid, LPCDIEFFECT lpeff,
                                           LPDIRECTINPUTEFFECT* ppdeff,
                                           LPUNKNOWN)
{
    DIDeviceImpl* d = (DIDeviceImpl*)impl;
    if (!ppdeff)
        return DIERR_INVALIDPARAM;
    *ppdeff = nullptr;
    if (!d->haptic)
        return DIERR_UNSUPPORTED; // honest: no FF hardware/driver -> engine skips FF
    DIEffectImpl* ei = new DIEffectImpl();
    ei->haptic = d->haptic;
    DIEffectToSdl(lpeff, rguid, ei->eff);
    ei->sdlEffectId = SDL_CreateHapticEffect(d->haptic, &ei->eff);
    if (ei->sdlEffectId < 0)
    {
        delete ei;
        return DIERR_UNSUPPORTED;
    }
    IDirectInputEffect* e = new IDirectInputEffect();
    e->impl = ei;
    *ppdeff = e;
    return DI_OK;
}

// ====================================================================================================================
//  IDirectInputEffect methods
// ====================================================================================================================
ULONG IDirectInputEffect::AddRef()
{
    DIEffectImpl* e = (DIEffectImpl*)impl;
    return e ? ++e->refcount : 1;
}
ULONG IDirectInputEffect::Release()
{
    DIEffectImpl* e = (DIEffectImpl*)impl;
    if (!e)
    {
        delete this;
        return 0;
    }
    if (--e->refcount > 0)
        return (ULONG)e->refcount;
    if (e->sdlEffectId >= 0 && e->haptic)
        SDL_DestroyHapticEffect(e->haptic, e->sdlEffectId);
    delete e;
    impl = nullptr;
    delete this;
    return 0;
}
HRESULT IDirectInputEffect::Start(DWORD iters, DWORD)
{
    DIEffectImpl* e = (DIEffectImpl*)impl;
    if (!e || !e->haptic || e->sdlEffectId < 0)
        return DIERR_NOTDOWNLOADED;
    return SDL_RunHapticEffect(e->haptic, e->sdlEffectId, iters ? iters : 1) ?
               DI_OK :
               DIERR_GENERIC;
}
HRESULT IDirectInputEffect::Stop()
{
    DIEffectImpl* e = (DIEffectImpl*)impl;
    if (!e || !e->haptic || e->sdlEffectId < 0)
        return DIERR_NOTDOWNLOADED;
    return SDL_StopHapticEffect(e->haptic, e->sdlEffectId) ? DI_OK :
                                                             DIERR_GENERIC;
}
HRESULT IDirectInputEffect::SetParameters(LPCDIEFFECT pe, DWORD)
{
    DIEffectImpl* e = (DIEffectImpl*)impl;
    if (!e || !e->haptic || e->sdlEffectId < 0)
        return DIERR_NOTDOWNLOADED;
    // Re-derive the SDL effect from the DI parameters and update it in place. rguid is unknown here, so keep the
    // effect's existing SDL type and only refresh the fields DI can change (level/magnitude/direction/duration).
    SDL_HapticEffect& s = e->eff;
    Sint32 dir0 =
        (pe && pe->rglDirection && pe->cAxes >= 1) ? pe->rglDirection[0] : 0;
    if (s.type == SDL_HAPTIC_CONSTANT)
    {
        s.constant.direction.dir[0] = dir0;
        if (pe && pe->lpvTypeSpecificParams &&
            pe->cbTypeSpecificParams >= sizeof(DICONSTANTFORCE))
            s.constant.level =
                (Sint16)((DICONSTANTFORCE*)pe->lpvTypeSpecificParams)
                    ->lMagnitude;
    }
    else
    {
        s.periodic.direction.dir[0] = dir0;
        if (pe && pe->lpvTypeSpecificParams &&
            pe->cbTypeSpecificParams >= sizeof(DIPERIODIC))
        {
            DIPERIODIC* p = (DIPERIODIC*)pe->lpvTypeSpecificParams;
            s.periodic.magnitude = (Sint16)p->dwMagnitude;
            s.periodic.period = (Uint16)(p->dwPeriod / 1000);
        }
    }
    return SDL_UpdateHapticEffect(e->haptic, e->sdlEffectId, &s) ?
               DI_OK :
               DIERR_GENERIC;
}
HRESULT IDirectInputEffect::Download()
{
    return DI_OK;
} // SDL downloads on create/update
HRESULT IDirectInputEffect::Unload()
{
    DIEffectImpl* e = (DIEffectImpl*)impl;
    if (e && e->sdlEffectId >= 0 && e->haptic)
    {
        SDL_DestroyHapticEffect(e->haptic, e->sdlEffectId);
        e->sdlEffectId = -1;
    }
    return DI_OK;
}

// ====================================================================================================================
//  IDirectInput8A methods
// ====================================================================================================================
ULONG IDirectInput8A::AddRef()
{
    DIObjectImpl* o = (DIObjectImpl*)impl;
    return o ? ++o->refcount : 1;
}
ULONG IDirectInput8A::Release()
{
    DIObjectImpl* o = (DIObjectImpl*)impl;
    if (o && --o->refcount > 0)
        return (ULONG)o->refcount;
    delete o;
    impl = nullptr;
    delete this;
    return 0;
}

static void SdlGuidToWin(const SDL_GUID& g, GUID& out)
{
    memcpy(&out, &g, sizeof(out));
} // both are 16 opaque bytes

HRESULT IDirectInput8A::EnumDevices(DWORD dwDevType,
                                    LPDIENUMDEVICESCALLBACKA cb, LPVOID pv,
                                    DWORD)
{
    EnsureSdlInput();
    if (!cb)
        return DIERR_INVALIDPARAM;
    if (dwDevType == DIDEVTYPE_JOYSTICK || dwDevType == DI8DEVCLASS_GAMECTRL)
    {
        int count = 0;
        SDL_JoystickID* ids = SDL_GetJoysticks(&count);
        for (int i = 0; ids && i < count; ++i)
        {
            DIDEVICEINSTANCEA di;
            memset(&di, 0, sizeof(di));
            di.dwSize = sizeof(di);
            SDL_GUID g = SDL_GetJoystickGUIDForID(ids[i]);
            SdlGuidToWin(g, di.guidInstance);
            di.dwDevType = DIDEVTYPE_JOYSTICK;
            const char* nm = SDL_GetJoystickNameForID(ids[i]);
            if (!nm)
                nm = "Joystick";
            strncpy(di.tszInstanceName, nm, MAX_PATH - 1);
            strncpy(di.tszProductName, nm, MAX_PATH - 1);
            if (cb(&di, pv) == DIENUM_STOP)
                break;
        }
        if (ids)
            SDL_free(ids);
    }
    return DI_OK;
}

HRESULT IDirectInput8A::CreateDevice(REFGUID rguid,
                                     LPDIRECTINPUTDEVICE8* lplpDevice,
                                     LPUNKNOWN)
{
    EnsureSdlInput();
    if (!lplpDevice)
        return DIERR_INVALIDPARAM;
    *lplpDevice = nullptr;
    DIDeviceImpl* d = new DIDeviceImpl();

    if (&rguid == &GUID_SysMouse ||
        memcmp(&rguid, &GUID_SysMouse, sizeof(GUID)) == 0)
    {
        d->kind = KIND_MOUSE;
        strncpy(d->name, "System Mouse", MAX_PATH - 1);
    }
    else if (&rguid == &GUID_SysKeyboard ||
             memcmp(&rguid, &GUID_SysKeyboard, sizeof(GUID)) == 0)
    {
        d->kind = KIND_KEYBOARD;
        strncpy(d->name, "System Keyboard", MAX_PATH - 1);
    }
    else
    {
        // joystick: find the SDL device whose stable GUID matches the requested instance GUID
        d->kind = KIND_JOYSTICK;
        int count = 0;
        SDL_JoystickID* ids = SDL_GetJoysticks(&count);
        for (int i = 0; ids && i < count; ++i)
        {
            GUID g;
            SDL_GUID sg = SDL_GetJoystickGUIDForID(ids[i]);
            SdlGuidToWin(sg, g);
            if (memcmp(&g, &rguid, sizeof(GUID)) == 0)
            {
                d->joy = SDL_OpenJoystick(ids[i]);
                d->sdlId = ids[i];
                d->instanceGuid = g;
                const char* nm = SDL_GetJoystickName(d->joy);
                if (nm)
                    strncpy(d->name, nm, MAX_PATH - 1);
                if (SDL_IsJoystickHaptic(d->joy))
                    d->haptic = SDL_OpenHapticFromJoystick(d->joy);
                break;
            }
        }
        if (ids)
            SDL_free(ids);
        if (!d->joy)
        {
            delete d;
            return DIERR_DEVICENOTREG;
        }
    }
    IDirectInputDevice8A* dev = new IDirectInputDevice8A();
    dev->impl = d;
    *lplpDevice = dev;
    return DI_OK;
}

HRESULT IDirectInput8A::CreateDeviceEx(REFGUID rguid, REFIID, void** ppvOut,
                                       LPUNKNOWN punk)
{
    return CreateDevice(rguid, (LPDIRECTINPUTDEVICE8*)ppvOut, punk);
}
HRESULT IDirectInput8A::GetDeviceStatus(REFGUID)
{
    return DI_OK;
}

// ====================================================================================================================
//  Factory + GetAsyncKeyState
// ====================================================================================================================
extern "C" HRESULT DirectInput8Create(HINSTANCE, DWORD, REFIID, void** ppvOut,
                                      LPUNKNOWN)
{
    EnsureSdlInput();
    if (!ppvOut)
        return DIERR_INVALIDPARAM;
    IDirectInput8A* obj = new IDirectInput8A();
    obj->impl = new DIObjectImpl();
    *ppvOut = obj;
    return DI_OK;
}

// GetAsyncKeyState over SDL keyboard state. Returns the Win32 encoding (0x8000 = currently down). The engine calls it
// only for the modifier VKs (Alt-Tab modifier re-sync) and VK_SHIFT (control-binding UI).
extern "C" SHORT GetAsyncKeyState(int vKey)
{
    SDL_PumpEvents();

    // #104: mouse buttons -- the UI polls GetAsyncKeyState(VK_LBUTTON) for clicks (chandler.cpp), which the
    // keyboard-only path below returned 0 for -> clicks were never seen. Report the live SDL button state.
    if (vKey == VK_LBUTTON || vKey == VK_RBUTTON || vKey == VK_MBUTTON)
    {
        SDL_MouseButtonFlags mb = SDL_GetMouseState(nullptr, nullptr);
        bool b = (vKey == VK_LBUTTON) ? ((mb & SDL_BUTTON_LMASK) != 0) :
                 (vKey == VK_RBUTTON) ? ((mb & SDL_BUTTON_RMASK) != 0) :
                                        ((mb & SDL_BUTTON_MMASK) != 0);
        return b ? (SHORT)0x8000 : 0;
    }

    int n = 0;
    const bool* ks = SDL_GetKeyboardState(&n);
    if (!ks)
        return 0;
    auto down = [&](SDL_Scancode sc) -> bool { return (sc < n) && ks[sc]; };
    bool pressed = false;
    switch (vKey)
    {
    case VK_LSHIFT:
        pressed = down(SDL_SCANCODE_LSHIFT);
        break;
    case VK_RSHIFT:
        pressed = down(SDL_SCANCODE_RSHIFT);
        break;
    case VK_SHIFT:
        pressed = down(SDL_SCANCODE_LSHIFT) || down(SDL_SCANCODE_RSHIFT);
        break;
    case VK_LCONTROL:
        pressed = down(SDL_SCANCODE_LCTRL);
        break;
    case VK_RCONTROL:
        pressed = down(SDL_SCANCODE_RCTRL);
        break;
    case VK_CONTROL:
        pressed = down(SDL_SCANCODE_LCTRL) || down(SDL_SCANCODE_RCTRL);
        break;
    case VK_LMENU:
        pressed = down(SDL_SCANCODE_LALT);
        break;
    case VK_RMENU:
        pressed = down(SDL_SCANCODE_RALT);
        break;
    case VK_MENU:
        pressed = down(SDL_SCANCODE_LALT) || down(SDL_SCANCODE_RALT);
        break;
    default:
        break;
    }
    return pressed ? (SHORT)0x8000 : 0;
}

#endif // !_WIN32
