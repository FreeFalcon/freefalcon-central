// Artscout - 2026 (#104, Linux port). The Linux game entry point. On Windows the process starts in WinMain()
// (ui/src/winmain.cpp), which sets up SEH and forwards to handle_WinMain(). Linux has no WinMain / SEH; the C
// runtime calls main() instead. This TU is the Linux-only twin of that WinMain shell: it assembles the command
// line from argv and forwards into the SAME real boot path, handle_WinMain(), so there is one game bring-up code
// path shared by both platforms. It is NOT a stub -- it calls the actual engine entry.
#ifndef _WIN32

#include <windows.h> // foundation shim: HINSTANCE / LPSTR / PASCAL vocabulary
#include <string>

// The real engine entry, defined in ui/src/winmain.cpp (lib ff_ui). Same signature the Windows WinMain calls.
extern signed int PASCAL handle_WinMain(HINSTANCE h_instance,
                                        HINSTANCE h_previous_instance,
                                        LPSTR command_line,
                                        signed int command_show);

int main(int argc, char** argv)
{
    // Rebuild a single Win32-style command-line string from argv[1..] (WinMain receives lpCmdLine without argv[0]).
    std::string cmdline;
    for (int i = 1; i < argc; ++i)
    {
        if (i > 1)
            cmdline += ' ';
        cmdline += argv[i];
    }

    // No HINSTANCE on Linux; handle_WinMain only feeds it to the ATL _Module.Init shim. Pass NULL (the SDL3
    // window token is owned by the ffplatform layer, created later during bring-up, not needed as an HINSTANCE).
    // command_show == 1 mirrors SW_SHOWNORMAL, matching the Windows launch.
    signed int rc =
        handle_WinMain(NULL, NULL, const_cast<LPSTR>(cmdline.c_str()), 1);
    return (int)rc;
}

#endif // !_WIN32
