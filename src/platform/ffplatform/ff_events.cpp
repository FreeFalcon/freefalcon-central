// Artscout - 2026 (#104, Linux port -- subsystem 1: events). SDL3 + internal-queue implementation of the message
// system declared in ff_events.h. Compiled on both platforms, linked against SDL3.
#include "ff_events.h"

#include <SDL3/SDL.h>

#include <deque>
#include <mutex>
#include <unordered_map>
#include <cstdint>

// Defined in ff_dinput.cpp: SDL scancode -> DIK / PS2 set-1 scan code (what the 2D UI reads out of lParam).
extern unsigned char FF_SdlScancodeToDik(int sdlScancode);

namespace ffevents
{

namespace
{

struct WinEntry
{
    void* sdlWindow = nullptr;
    WndProcFn proc = nullptr;
};

std::mutex g_lock; // guards the queue + registry
std::deque<Msg>
    g_queue; // posted messages (internal FM_ bus + translated SDL events)
std::unordered_map<HWND, WinEntry> g_windows; // hwnd -> {sdl window, proc}
std::unordered_map<Uint32, HWND>
    g_byWindowId; // SDL window id -> hwnd (for event routing)
HWND g_mainWindow = nullptr;
bool g_quitPosted = false;

HWND HwndForWindowId(Uint32 id)
{
    auto it = g_byWindowId.find(id);
    return (it != g_byWindowId.end()) ? it->second : g_mainWindow;
}

WndProcFn ProcForHwnd(HWND hwnd)
{
    auto it = g_windows.find(hwnd);
    return (it != g_windows.end()) ? it->second.proc : nullptr;
}

LPARAM PackXY(int x, int y)
{
    return ((LPARAM)(int16_t)y << 16) | ((LPARAM)(int16_t)x & 0xffff);
}

WPARAM MouseButtonMask(Uint32 sdlState)
{
    WPARAM w = 0;
    if (sdlState & SDL_BUTTON_LMASK)
        w |= MK_LBUTTON;
    if (sdlState & SDL_BUTTON_RMASK)
        w |= MK_RBUTTON;
    if (sdlState & SDL_BUTTON_MMASK)
        w |= MK_MBUTTON;
    return w;
}

// Enqueue already holding g_lock.
void EnqueueLocked(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    Msg m;
    m.hwnd = hwnd;
    m.message = message;
    m.wParam = wParam;
    m.lParam = lParam;
    g_queue.push_back(m);
}

// Map an SDL keycode to a Win32 virtual-key code so WndProc key handlers (menu shortcuts, the controls-tab
// key filter, dialog navigation) see the VKs they expect -- previously we forwarded the raw SDL scancode,
// which those handlers can't match. Printable ASCII maps directly (letters -> uppercase VK); named keys go
// through the switch. Text entry (the "search by action" field etc.) arrives separately as WM_CHAR from
// SDL_EVENT_TEXT_INPUT. Returns 0 for keys with no VK equivalent.
WPARAM SdlKeyToVk(SDL_Keycode k)
{
    if (k >= 'a' && k <= 'z')
        return (WPARAM)(k - 'a' + 'A'); // VK_A..VK_Z (0x41..0x5A)
    if (k >= '0' && k <= '9')
        return (WPARAM)k; // VK_0..VK_9 (0x30..0x39)
    switch (k)
    {
    case SDLK_BACKSPACE:
        return 0x08; // VK_BACK
    case SDLK_TAB:
        return 0x09; // VK_TAB
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        return 0x0D; // VK_RETURN
    case SDLK_ESCAPE:
        return 0x1B; // VK_ESCAPE
    case SDLK_SPACE:
        return 0x20; // VK_SPACE
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
        return 0x10; // VK_SHIFT
    case SDLK_LCTRL:
    case SDLK_RCTRL:
        return 0x11; // VK_CONTROL
    case SDLK_LALT:
    case SDLK_RALT:
        return 0x12; // VK_MENU
    case SDLK_PAUSE:
        return 0x13;
    case SDLK_CAPSLOCK:
        return 0x14;
    case SDLK_PAGEUP:
        return 0x21;
    case SDLK_PAGEDOWN:
        return 0x22;
    case SDLK_END:
        return 0x23;
    case SDLK_HOME:
        return 0x24;
    case SDLK_LEFT:
        return 0x25;
    case SDLK_UP:
        return 0x26;
    case SDLK_RIGHT:
        return 0x27;
    case SDLK_DOWN:
        return 0x28;
    case SDLK_INSERT:
        return 0x2D;
    case SDLK_DELETE:
        return 0x2E;
    case SDLK_F1:
        return 0x70;
    case SDLK_F2:
        return 0x71;
    case SDLK_F3:
        return 0x72;
    case SDLK_F4:
        return 0x73;
    case SDLK_F5:
        return 0x74;
    case SDLK_F6:
        return 0x75;
    case SDLK_F7:
        return 0x76;
    case SDLK_F8:
        return 0x77;
    case SDLK_F9:
        return 0x78;
    case SDLK_F10:
        return 0x79;
    case SDLK_F11:
        return 0x7A;
    case SDLK_F12:
        return 0x7B;
    default:
        return 0;
    }
}

// Translate one SDL event into zero or more queued Msgs. Called holding g_lock.
void TranslateSdlEventLocked(const SDL_Event& e)
{
    switch (e.type)
    {
    case SDL_EVENT_QUIT:
        EnqueueLocked(g_mainWindow, WM_QUIT, 0, 0);
        g_quitPosted = true;
        break;

    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        EnqueueLocked(HwndForWindowId(e.window.windowID), WM_CLOSE, 0, 0);
        break;

    case SDL_EVENT_WINDOW_FOCUS_GAINED:
    {
        HWND h = HwndForWindowId(e.window.windowID);
        EnqueueLocked(h, WM_ACTIVATEAPP, 1, 0);
        EnqueueLocked(h, WM_ACTIVATE, WA_ACTIVE, 0);
        break;
    }
    case SDL_EVENT_WINDOW_FOCUS_LOST:
    {
        HWND h = HwndForWindowId(e.window.windowID);
        EnqueueLocked(h, WM_ACTIVATEAPP, 0, 0);
        EnqueueLocked(h, WM_ACTIVATE, WA_INACTIVE, 0);
        EnqueueLocked(h, WM_KILLFOCUS, 0, 0);
        break;
    }

    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        EnqueueLocked(HwndForWindowId(e.window.windowID), WM_SIZE, 0,
                      PackXY(e.window.data1, e.window.data2));
        break;

    case SDL_EVENT_MOUSE_MOTION:
        EnqueueLocked(HwndForWindowId(e.motion.windowID), WM_MOUSEMOVE,
                      MouseButtonMask(e.motion.state),
                      PackXY((int)e.motion.x, (int)e.motion.y));
        break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
    {
        const bool down = (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
        UINT msg = WM_MOUSEMOVE;
        if (e.button.button == SDL_BUTTON_LEFT)
            msg = down ? (e.button.clicks >= 2 ? WM_LBUTTONDBLCLK :
                                                 WM_LBUTTONDOWN) :
                         WM_LBUTTONUP;
        else if (e.button.button == SDL_BUTTON_RIGHT)
            msg = down ? (e.button.clicks >= 2 ? WM_RBUTTONDBLCLK :
                                                 WM_RBUTTONDOWN) :
                         WM_RBUTTONUP;
        else
            break;
        EnqueueLocked(HwndForWindowId(e.button.windowID), msg, 0,
                      PackXY((int)e.button.x, (int)e.button.y));
        break;
    }

    case SDL_EVENT_MOUSE_WHEEL:
        // Win32 packs the wheel delta (multiples of 120) in the HIWORD of wParam.
        EnqueueLocked(HwndForWindowId(e.wheel.windowID), WM_MOUSEWHEEL,
                      ((WPARAM)(int16_t)(e.wheel.y * 120) << 16), 0);
        break;

    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
    {
        // The 2D UI (ui95/chandler.cpp) decodes the pressed key from WM_KEYDOWN's lParam, NOT wParam:
        //   Key = (lParam>>16 & 0xff) | (lParam>>17 & 0x80)   -> a DIK / PS2 set-1 scan code, which it turns
        // into ASCII (AsciiChar) and hands to CheckKeyboard() for menu shortcuts, the controls-tab key filter
        // and edit fields (the "search by action" box). Previously we forwarded the raw SDL scancode in wParam
        // and left lParam=0, so the UI decoded Key=0 and no key ever registered. Pack the DIK scan code into
        // lParam's Win32 keyboard layout (repeat | scan<<16 | extended<<24); wParam carries the VK for the few
        // wParam==VK_* checks.
        const bool down = (e.type == SDL_EVENT_KEY_DOWN);
        unsigned dik = FF_SdlScancodeToDik((int)e.key.scancode);
        LPARAM lp =
            1 // repeat count = 1 (lParam bits 0-15)
            | ((LPARAM)(dik & 0x7f) << 16) // scan code    (lParam bits 16-23)
            | ((dik & 0x80) ? ((LPARAM)1 << 24) :
                              0); // extended flag (lParam bit 24)
        EnqueueLocked(HwndForWindowId(e.key.windowID),
                      down ? WM_KEYDOWN : WM_KEYUP, SdlKeyToVk(e.key.key), lp);
        break;
    }

    default:
        break;
    }
}

// Drain all pending SDL events into the queue. Called holding g_lock.
void PumpSdlLocked()
{
    SDL_Event e;
    int drained = 0;
    while (SDL_PollEvent(&e))
    {
        TranslateSdlEventLocked(e);
        ++drained;
    }
}

} // namespace

// Linux peer of Win32 ClipCursor: confine (enable!=0) or release the OS cursor to a window. SDL_SetWindowMouseGrab
// keeps the cursor inside the window WITHOUT switching to relative mode, so the absolute 2D-UI cursor still works.
// Looked up by HWND so winmain can drive it exactly where it drives ClipCursor on Windows.
extern "C" void FF_ClipCursorToWindow(HWND hwnd, int enable)
{
    std::lock_guard<std::mutex> g(g_lock);
    void* sw = nullptr;
    if (hwnd)
    {
        auto it = g_windows.find(hwnd);
        if (it != g_windows.end())
            sw = it->second.sdlWindow;
    }
    else if (!g_windows.empty())
        sw = g_windows.begin()->second.sdlWindow;
    if (sw)
        SDL_SetWindowMouseGrab((SDL_Window*)sw, enable ? true : false);
}

// 3D uses a delta-driven cursor (gxPos += mouse delta, in DispWidth space) and expects the OS cursor HIDDEN --
// on Windows the exclusive DirectInput mouse does both. SDL relative mode is the Linux peer: it hides the OS
// cursor, captures it (no window-edge pinning), and keeps feeding relative motion to SDL_GetRelativeMouseState
// (the ff_dinput mouse source). Driven from the ShowCursor shim's visibility counter so it follows the engine's
// own show/hide bracketing of 3D vs the 2D menu. Idempotent so the ShowCursor spin-loops don't thrash it.
extern "C" void FF_SetRelativeMouse(int enable)
{
    std::lock_guard<std::mutex> g(g_lock);
    if (g_windows.empty())
        return;
    SDL_Window* sw = (SDL_Window*)g_windows.begin()->second.sdlWindow;
    if (!sw)
        return;
    bool want = enable != 0;
    if (SDL_GetWindowRelativeMouseMode(sw) == want)
        return; // no-op if already in the wanted state
    SDL_SetWindowRelativeMouseMode(sw, want);
}

// #108 Linux VR menu cursor: map the SDL window-relative mouse position (the SAME coordinate space ui95 reads
// from WM_MOUSEMOVE via the synthesized SDL_EVENT_MOUSE_MOTION) into the VR menu panel's pixel grid
// (panelW x panelH). RunVulkanMenuFrame paints a crosshair there because the OS/SDL hardware cursor is not
// composited into the menu surface (m_pSysMem) that the quad shows -> otherwise the pointer is invisible in VR.
extern "C" bool FF_GetMenuCursorPx(int panelW, int panelH, int* outX, int* outY)
{
    if (panelW <= 0 || panelH <= 0 || !outX || !outY)
        return false;
    float mx = 0.0f, my = 0.0f;
    SDL_GetMouseState(&mx, &my); // window-relative logical coords
    SDL_Window* sw =
        SDL_GetMouseFocus(); // the window the cursor is over (matches the coords above)
    if (!sw)
    {
        std::lock_guard<std::mutex> g(g_lock);
        if (!g_windows.empty())
            sw = (SDL_Window*)g_windows.begin()->second.sdlWindow;
    }
    int ww = panelW, wh = panelH;
    if (sw)
        SDL_GetWindowSize(sw, &ww, &wh);
    if (ww <= 0 || wh <= 0)
        return false;
    *outX = (int)(mx * (float)panelW / (float)ww);
    *outY = (int)(my * (float)panelH / (float)wh);
    return true;
}

void RegisterWindow(void* sdlWindow, HWND hwnd, WndProcFn proc)
{
    std::lock_guard<std::mutex> g(g_lock);
    g_windows[hwnd] = WinEntry{sdlWindow, proc};
    if (sdlWindow)
        g_byWindowId[SDL_GetWindowID((SDL_Window*)sdlWindow)] = hwnd;
    if (!g_mainWindow)
        g_mainWindow = hwnd;
}

void UnregisterWindow(HWND hwnd)
{
    std::lock_guard<std::mutex> g(g_lock);
    auto it = g_windows.find(hwnd);
    if (it != g_windows.end())
    {
        if (it->second.sdlWindow)
            g_byWindowId.erase(
                SDL_GetWindowID((SDL_Window*)it->second.sdlWindow));
        g_windows.erase(it);
    }
    if (g_mainWindow == hwnd)
        g_mainWindow = nullptr;
}

void SetMainWindow(HWND hwnd)
{
    std::lock_guard<std::mutex> g(g_lock);
    g_mainWindow = hwnd;
}
HWND GetMainWindow()
{
    std::lock_guard<std::mutex> g(g_lock);
    return g_mainWindow;
}

void PostAppMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    std::lock_guard<std::mutex> g(g_lock);
    EnqueueLocked(g_mainWindow, msg, wParam, lParam);
}

LRESULT SendAppMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND hwnd = nullptr;
    WndProcFn proc = nullptr;
    {
        std::lock_guard<std::mutex> g(g_lock);
        hwnd = g_mainWindow;
        proc = ProcForHwnd(hwnd);
    }
    return proc ? proc(hwnd, msg, wParam, lParam) :
                  0; // synchronous: call outside the lock
}

void PostWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    std::lock_guard<std::mutex> g(g_lock);
    EnqueueLocked(hwnd, msg, wParam, lParam);
}

// ---- Win32 PostMessage/SendMessage bridge (win32shim <windows.h> forwards PostMessageA/SendMessageA here) ------
// The engine posts its FM_* command bus to appWin (registered via RegisterWindow). Route those; leave messages to
// UNREGISTERED handles (Win32 dialog controls from GetDlgItem) as no-ops, exactly as the old stub did. extern "C"
// so the symbol is global regardless of this enclosing namespace.
extern "C" BOOL FF_PostWindowMessage(HWND hwnd, UINT msg, WPARAM wParam,
                                     LPARAM lParam)
{
    std::lock_guard<std::mutex> g(g_lock);
    if (g_windows.find(hwnd) == g_windows.end())
        return 0; // unregistered (dialog control / null) -> no-op, message not delivered
    EnqueueLocked(hwnd, msg, wParam, lParam);
    return 1;
}

extern "C" LRESULT FF_SendWindowMessage(HWND hwnd, UINT msg, WPARAM wParam,
                                        LPARAM lParam)
{
    WndProcFn proc = nullptr;
    {
        std::lock_guard<std::mutex> g(g_lock);
        proc = ProcForHwnd(hwnd);
    }
    return proc ? proc(hwnd, msg, wParam, lParam) :
                  0; // synchronous; unregistered -> 0 (old no-op behaviour)
}

void PostQuit(int /*exitCode*/)
{
    std::lock_guard<std::mutex> g(g_lock);
    EnqueueLocked(g_mainWindow, WM_QUIT, 0, 0);
    g_quitPosted = true;
}

bool GetMessage(Msg* out)
{
    for (;;)
    {
        {
            std::lock_guard<std::mutex> g(g_lock);
            PumpSdlLocked();
            if (!g_queue.empty())
            {
                *out = g_queue.front();
                g_queue.pop_front();
                return out->message != WM_QUIT;
            }
        }
        // Nothing queued: block briefly on SDL so we don't busy-spin, then retry.
        SDL_Event e;
        if (SDL_WaitEventTimeout(&e, 10))
        {
            std::lock_guard<std::mutex> g(g_lock);
            TranslateSdlEventLocked(e);
        }
    }
}

bool PeekMessage(Msg* out, bool remove)
{
    std::lock_guard<std::mutex> g(g_lock);
    PumpSdlLocked();
    if (g_queue.empty())
        return false;
    *out = g_queue.front();
    if (remove)
        g_queue.pop_front();
    return true;
}

LRESULT DispatchMessage(const Msg* m)
{
    HWND hwnd = m->hwnd;
    WndProcFn proc = nullptr;
    {
        std::lock_guard<std::mutex> g(g_lock);
        proc = ProcForHwnd(hwnd);
        if (!proc && g_mainWindow)
        {
            hwnd = g_mainWindow;
            proc = ProcForHwnd(hwnd);
        }
    }
    return proc ? proc(hwnd, m->message, m->wParam, m->lParam) : 0;
}

void TranslateMessage(const Msg* /*m*/)
{
    // No-op: KEYDOWN->CHAR synthesis and text input belong to the SDL keyboard port (subsystem 3).
}

} // namespace ffevents
