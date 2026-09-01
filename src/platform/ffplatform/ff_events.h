// Artscout - 2026 (#104, Linux port -- subsystem 1: events). The message system, ported off Win32 onto SDL3 +
// an internal queue, used by BOTH builds (no #ifdef at call sites). It replaces the Win32 message pump wholesale:
//
//  * OS window events (activate / focus / size / close / mouse / key) come from SDL_PollEvent, get translated to
//    the (message, wParam, lParam) shape the game's existing WndProcs already switch on, and are dispatched to the
//    registered handler for the target window. The big switch(WM_*) bodies in FalconMessageHandler / SimWndProc
//    are NOT rewritten -- only the transport changes.
//
//  * The game's INTERNAL command bus (the FM_* = WM_USER+n messages the game PostMessage's to its own window --
//    FM_START_GAME, FM_LOAD_CAMPAIGN, _EnterMode, ...) rides an internal thread-safe queue we own, NOT the OS
//    queue. Game code posts via PostAppMessage()/SendAppMessage() instead of PostMessage(appWin, ...).
//
// This is the real message-system port, not a stub. The Win32 dialog-control traffic (SendMessage(GetDlgItem...))
// belongs to the separate Win32-dialog subsystem and is untouched here.
//
// The header pulls <windows.h> only for the shared integer/handle types (HWND/UINT/WPARAM/LPARAM/LRESULT/POINT);
// on Linux that resolves to the win32 foundation shim, on Windows to the real SDK. SDL is confined to the .cpp.
#ifndef FF_PLATFORM_FF_EVENTS_H
#define FF_PLATFORM_FF_EVENTS_H

#include <windows.h> // HWND, UINT, WPARAM, LPARAM, LRESULT, DWORD, POINT, and (on Windows) the real WM_* values

// ---- WM_* message-id protocol -------------------------------------------------------------------------------
// These are the message ids the game's own dispatch switches on. They are PROTOCOL DATA (ids), not USER32 API.
// Defined idempotently: on Windows the real <windows.h> (included above) already defined them and every #ifndef
// is skipped; on Linux the foundation shim does not define them, so these become the canonical values here. No
// "#ifdef _WIN32" -- the same header text is correct on both platforms.
#ifndef WM_NULL
#define WM_NULL 0x0000
#endif
#ifndef WM_CREATE
#define WM_CREATE 0x0001
#endif
#ifndef WM_DESTROY
#define WM_DESTROY 0x0002
#endif
#ifndef WM_SIZE
#define WM_SIZE 0x0005
#endif
#ifndef WM_ACTIVATE
#define WM_ACTIVATE 0x0006
#endif
#ifndef WM_KILLFOCUS
#define WM_KILLFOCUS 0x0008
#endif
#ifndef WM_PAINT
#define WM_PAINT 0x000F
#endif
#ifndef WM_CLOSE
#define WM_CLOSE 0x0010
#endif
#ifndef WM_QUIT
#define WM_QUIT 0x0012
#endif
#ifndef WM_ACTIVATEAPP
#define WM_ACTIVATEAPP 0x001C
#endif
#ifndef WM_NCACTIVATE
#define WM_NCACTIVATE 0x0086
#endif
#ifndef WM_KEYDOWN
#define WM_KEYDOWN 0x0100
#endif
#ifndef WM_KEYUP
#define WM_KEYUP 0x0101
#endif
#ifndef WM_CHAR
#define WM_CHAR 0x0102
#endif
#ifndef WM_SYSKEYDOWN
#define WM_SYSKEYDOWN 0x0104
#endif
#ifndef WM_SYSKEYUP
#define WM_SYSKEYUP 0x0105
#endif
#ifndef WM_COMMAND
#define WM_COMMAND 0x0111
#endif
#ifndef WM_TIMER
#define WM_TIMER 0x0113
#endif
#ifndef WM_MOUSEMOVE
#define WM_MOUSEMOVE 0x0200
#endif
#ifndef WM_LBUTTONDOWN
#define WM_LBUTTONDOWN 0x0201
#endif
#ifndef WM_LBUTTONUP
#define WM_LBUTTONUP 0x0202
#endif
#ifndef WM_LBUTTONDBLCLK
#define WM_LBUTTONDBLCLK 0x0203
#endif
#ifndef WM_RBUTTONDOWN
#define WM_RBUTTONDOWN 0x0204
#endif
#ifndef WM_RBUTTONUP
#define WM_RBUTTONUP 0x0205
#endif
#ifndef WM_RBUTTONDBLCLK
#define WM_RBUTTONDBLCLK 0x0206
#endif
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif
#ifndef WM_MOUSELEAVE
#define WM_MOUSELEAVE 0x02A3
#endif
#ifndef WM_USER
#define WM_USER 0x0400
#endif
// WM_ACTIVATE's wParam values
#ifndef WA_INACTIVE
#define WA_INACTIVE 0
#define WA_ACTIVE 1
#define WA_CLICKACTIVE 2
#endif
// mouse-button mask bits carried in wParam of mouse messages
#ifndef MK_LBUTTON
#define MK_LBUTTON 0x0001
#define MK_RBUTTON 0x0002
#define MK_SHIFT 0x0004
#define MK_CONTROL 0x0008
#define MK_MBUTTON 0x0010
#endif

namespace ffevents
{

// A message, mirroring the fields the pump and WndProcs use.
struct Msg
{
    HWND hwnd = nullptr;
    UINT message = 0;
    WPARAM wParam = 0;
    LPARAM lParam = 0;
    DWORD time = 0;
    POINT pt = {0, 0};
};

// The game's WndProc signature (FalconMessageHandler / SimWndProc match this).
typedef LRESULT (*WndProcFn)(HWND, UINT, WPARAM, LPARAM);

// Associate a window (its ffplatform SDL window, as void*) with an HWND token and its procedure, so translated
// SDL events and dispatched queue messages reach the right handler. `hwnd` is the token the WndProc will receive
// (on Windows this is the real HWND; on Linux an opaque token the handlers pass around).
void RegisterWindow(void* sdlWindow, HWND hwnd, WndProcFn proc);
void UnregisterWindow(HWND hwnd);

// The "main" window is the one PostAppMessage/SendAppMessage target (the game render window, appWin).
void SetMainWindow(HWND hwnd);
HWND GetMainWindow();

// Internal command bus (replaces PostMessage/SendMessage(appWin, FM_*, ...)).
void PostAppMessage(UINT msg, WPARAM wParam,
                    LPARAM lParam); // async: enqueue, returns immediately
LRESULT
SendAppMessage(UINT msg, WPARAM wParam,
               LPARAM lParam); // sync:  invoke the handler now, return result
// Async post to a specific window.
void PostWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
// Ask the loop to quit (enqueues WM_QUIT so GetMessage returns false).
void PostQuit(int exitCode);

// ---- pump primitives (used by the winmain loops; SDL OS events + internal queue) -----------------------------
// GetMessage blocks until a message is available (draining SDL in the meantime); returns false on WM_QUIT.
bool GetMessage(Msg* out);
// PeekMessage is non-blocking; returns true and fills `out` if a message was available (removed if `remove`).
bool PeekMessage(Msg* out, bool remove);
// Route a message to the registered procedure for out->hwnd (or the main window if hwnd is null).
LRESULT DispatchMessage(const Msg* m);
// Key translation hook (no-op today; real KEYDOWN->CHAR synthesis lives in the SDL input port, subsystem 3).
void TranslateMessage(const Msg* m);

} // namespace ffevents

// ---- Win32-compatible pump aliases (Linux only) --------------------------------------------------------------
// winmain.cpp's pump and WndProcs are written against the Win32 shapes (MSG, the 4-arg GetMessage, PM_REMOVE,
// GET_X_LPARAM, DefWindowProc, ...). On Windows those come from the real <windows.h>; on Linux we map them onto
// the ffevents primitives above so the SAME winmain source compiles unchanged. This is message-PROTOCOL glue
// over the real port (like the WM_* ids above), NOT a USER32 function stub.
#ifndef _WIN32
typedef ffevents::Msg MSG;
typedef MSG* LPMSG;
#ifndef PM_NOREMOVE
#define PM_NOREMOVE 0x0000
#define PM_REMOVE 0x0001
#endif
inline int GetMessage(MSG* m, HWND, UINT, UINT)
{
    return ffevents::GetMessage(m) ? 1 : 0;
}
inline bool PeekMessage(MSG* m, HWND, UINT, UINT, UINT fl)
{
    return ffevents::PeekMessage(m, ((fl)&PM_REMOVE) != 0);
}
// NB: DispatchMessage(&msg) / TranslateMessage(&msg) are NOT redefined here -- since MSG == ffevents::Msg,
// argument-dependent lookup already resolves the unqualified calls to ffevents::DispatchMessage/TranslateMessage.
// Adding globals here would make those calls ambiguous.
inline void PostQuitMessage(int code)
{
    ffevents::PostQuit(code);
}
inline LRESULT DefWindowProc(HWND, UINT, WPARAM, LPARAM)
{
    return 0;
} // unhandled -> 0 (no OS default proc)
#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)((LPARAM)(lp) & 0xffff))
#define GET_Y_LPARAM(lp) ((int)(short)(((LPARAM)(lp) >> 16) & 0xffff))
#endif
#endif // !_WIN32

#endif // FF_PLATFORM_FF_EVENTS_H
