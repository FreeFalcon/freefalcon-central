// Artscout - 2026 (#104, Linux port -- subsystem 1: window/events -> SDL3).
// A single, cross-platform window built on SDL3, used by BOTH the Windows and the Linux build (no #ifdef at the
// call sites). On Windows SDL still owns the window and we hand its HWND to the D3D12 swapchain unchanged, so the
// DX12 renderer path is not disturbed -- only WHO creates the window changes (SDL3 instead of RegisterClass/
// CreateWindow). On Linux the same SDL window backs the Vulkan surface. This replaces the Win32 window creation in
// ui/src/winmain.cpp outright; the old RegisterClass/CreateWindow/message-pump goes away (not guarded, deleted).
//
// The header is deliberately free of SDL and <windows.h> so it is safe to include anywhere on either platform;
// the native handle is exposed as an opaque void* (an HWND on Windows, null elsewhere).
#ifndef FF_PLATFORM_FF_WINDOW_H
#define FF_PLATFORM_FF_WINDOW_H

namespace ffplatform
{

struct WindowDesc
{
    const char* title = "Falcon 4";
    int width = 1024;
    int height = 768;
    bool fullscreen = false;
    bool resizable = false;
    bool borderless = false;
};

// Owns one SDL_Window. Create() also brings up SDL's video subsystem on first use.
class Window
{
public:
    static Window*
    Create(const WindowDesc&
               desc); // null on failure (see ffplatform::LastError())
    ~Window();

    void Show();
    void Hide();
    void SetTitle(const char* title);
    void GetSize(int* outW, int* outH) const; // logical size
    void GetPixelSize(int* outW,
                      int* outH) const; // backing-store size (HiDPI aware)
    void
    SetSize(int w,
            int h); // resize the window (SDL_SetWindowSize + sync); the Vulkan
    // surface extent follows this -- used to apply the 3D resolution
    void SetFullscreen(bool on);
    bool IsFullscreen() const;
    void SetPosition(int x, int y);
    void CenterOnDisplay();

    // Native window handle for the D3D12 swapchain: an HWND on Windows, null on other platforms. Void* so this
    // header pulls in neither <windows.h> nor SDL. The Linux/Vulkan path uses GetSdlWindow() + SDL_Vulkan_*.
    void* GetWin32Hwnd() const;

    // Underlying SDL_Window* (as void*) for the event pump and Vulkan surface creation.
    void* GetSdlWindow() const;

private:
    Window() = default;
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    struct Impl;
    Impl* m_impl = nullptr;
};

// Global lifecycle. Init() is optional (Window::Create brings up video lazily); Shutdown() tears SDL down at exit.
bool Init();
void Shutdown();
const char* LastError(); // last SDL error string, never null

} // namespace ffplatform

#endif // FF_PLATFORM_FF_WINDOW_H
