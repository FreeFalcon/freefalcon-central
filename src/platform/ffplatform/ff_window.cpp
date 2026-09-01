// Artscout - 2026 (#104, Linux port -- subsystem 1). SDL3 implementation of ffplatform::Window. Compiled on both
// platforms and linked against SDL3. The only platform-specific bit is pulling the Win32 HWND out of SDL for the
// D3D12 swapchain -- that is genuine native-handle bridging (SDL itself is #ifdef'd internally for it), not a
// stub-behind-a-guard. Everything else is one code path for Windows and Linux.
#include "ff_window.h"

#include <SDL3/SDL.h>

namespace ffplatform
{

struct Window::Impl
{
    SDL_Window* sdl = nullptr;
};

bool Init()
{
    if (SDL_WasInit(SDL_INIT_VIDEO))
        return true;
    return SDL_Init(SDL_INIT_VIDEO); // SDL3: returns true on success
}

void Shutdown()
{
    SDL_Quit();
}

const char* LastError()
{
    const char* e = SDL_GetError();
    return e ? e : "";
}

Window* Window::Create(const WindowDesc& desc)
{
    if (!Init())
        return nullptr;

    SDL_WindowFlags flags = 0;
    if (desc.fullscreen)
        flags |= SDL_WINDOW_FULLSCREEN;
    if (desc.resizable)
        flags |= SDL_WINDOW_RESIZABLE;
    if (desc.borderless)
        flags |= SDL_WINDOW_BORDERLESS;
    flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
#ifndef _WIN32
    // Linux renders through Vulkan -- the window must be created Vulkan-capable so SDL_Vulkan_CreateSurface works.
    flags |= SDL_WINDOW_VULKAN;
#endif

    SDL_Window* w =
        SDL_CreateWindow(desc.title, desc.width, desc.height, flags);
    if (!w)
        return nullptr;

    Window* self = new Window();
    self->m_impl = new Impl();
    self->m_impl->sdl = w;
    return self;
}

Window::~Window()
{
    if (m_impl)
    {
        if (m_impl->sdl)
            SDL_DestroyWindow(m_impl->sdl);
        delete m_impl;
        m_impl = nullptr;
    }
}

void Window::Show()
{
    if (m_impl && m_impl->sdl)
        SDL_ShowWindow(m_impl->sdl);
}
void Window::Hide()
{
    if (m_impl && m_impl->sdl)
        SDL_HideWindow(m_impl->sdl);
}

void Window::SetTitle(const char* title)
{
    if (m_impl && m_impl->sdl && title)
        SDL_SetWindowTitle(m_impl->sdl, title);
}

void Window::GetSize(int* outW, int* outH) const
{
    int w = 0, h = 0;
    if (m_impl && m_impl->sdl)
        SDL_GetWindowSize(m_impl->sdl, &w, &h);
    if (outW)
        *outW = w;
    if (outH)
        *outH = h;
}

void Window::GetPixelSize(int* outW, int* outH) const
{
    int w = 0, h = 0;
    if (m_impl && m_impl->sdl)
        SDL_GetWindowSizeInPixels(m_impl->sdl, &w, &h);
    if (outW)
        *outW = w;
    if (outH)
        *outH = h;
}

void Window::SetSize(int w, int h)
{
    if (m_impl && m_impl->sdl)
    {
        SDL_SetWindowSize(m_impl->sdl, w, h);
        // Wait for the resize to actually take effect (Wayland applies window state asynchronously) so the Vulkan
        // surface's currentExtent reflects the new size before the caller (re)creates the swapchain against it.
        SDL_SyncWindow(m_impl->sdl);
    }
}

void Window::SetFullscreen(bool on)
{
    if (m_impl && m_impl->sdl)
        SDL_SetWindowFullscreen(m_impl->sdl, on);
}

bool Window::IsFullscreen() const
{
    if (!m_impl || !m_impl->sdl)
        return false;
    return (SDL_GetWindowFlags(m_impl->sdl) & SDL_WINDOW_FULLSCREEN) != 0;
}

void Window::SetPosition(int x, int y)
{
    if (m_impl && m_impl->sdl)
        SDL_SetWindowPosition(m_impl->sdl, x, y);
}

void Window::CenterOnDisplay()
{
    if (m_impl && m_impl->sdl)
        SDL_SetWindowPosition(m_impl->sdl, SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED);
}

void* Window::GetWin32Hwnd() const
{
#ifdef _WIN32
    if (!m_impl || !m_impl->sdl)
        return nullptr;
    SDL_PropertiesID props = SDL_GetWindowProperties(m_impl->sdl);
    return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER,
                                  nullptr);
#else
    return nullptr;
#endif
}

void* Window::GetSdlWindow() const
{
    return m_impl ? m_impl->sdl : nullptr;
}

} // namespace ffplatform
