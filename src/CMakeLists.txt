# Artscout - 2026 (#104, Linux port). Top-level Linux build of the FreeFalcon/RedViper engine: compiles the engine
# subsystems as static libs against the win32 foundation shim with clang, alongside the SDL3 platform layer and the
# Vulkan renderer. This is the real-build grind harness -- add libs as their files compile; the exe link comes once
# the platform subsystems (window/input/audio/net) are ported. clang-only (the shim uses -fms-compatibility).
cmake_minimum_required(VERSION 3.16)
project(ffviper_linux LANGUAGES CXX)

if(WIN32)
    message(FATAL_ERROR "This CMake is the LINUX build. Windows uses the MSVC .sln/.vcxproj.")
endif()
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "The Linux build needs clang (the win32 shim uses -fms-compatibility). "
                        "Use: cmake -DCMAKE_CXX_COMPILER=clang++ -S src -B build-linux")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(FF_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# ---- ff_shim: the common compile contract every engine lib links (INTERFACE = flags/includes/defines only) ----
add_library(ff_shim INTERFACE)
target_include_directories(ff_shim INTERFACE
    ${FF_ROOT}/platform/win32shim         # <windows.h> etc. -> the shim, FIRST
    ${FF_ROOT}
    ${FF_ROOT}/include
    ${FF_ROOT}/graphics/include ${FF_ROOT}/graphics
    ${FF_ROOT}/campaign/include ${FF_ROOT}/codelib/include ${FF_ROOT}/falclib/include
    ${FF_ROOT}/sim/include ${FF_ROOT}/ui/include
    ${FF_ROOT}/ui95 ${FF_ROOT}/ui95_ext ${FF_ROOT}/vu2/include ${FF_ROOT}/tools/lists
    ${FF_ROOT}/comms/include    # network subsystem headers (ComList.h, capibw*.h)
    ${FF_ROOT}/acmi/src/include  # ACMI recorder/tape headers (acmirec.h/acmitape.h/... pulled by sim + UI)
    ${FF_ROOT}/platform/ffplatform) # ff_events.h (message-pump port) + ff_window.h
    # NB: do NOT add -I mathlib -- the engine's mathlib/math.h is included by relative path (../../mathlib/math.h);
    # putting it on the include path makes bare `#include <math.h>` resolve to it instead of the C library header.
target_compile_options(ff_shim INTERFACE
    -fms-extensions -fms-compatibility -fms-compatibility-version=19.20 -w -Wno-register -Wno-writable-strings
    # Force-include the shim <windows.h> before every TU. On Windows a force-included PCH (stdafx.h) transitively
    # pulls windows.h into every file; many .cpp use DWORD/GetTickCount/etc. without including it directly. The shim
    # header is designed to be includable anywhere (it pre-includes the STL before its min/max macros), so this is
    # the Linux equivalent of that PCH and gives every file the Win32 vocabulary.
    -include windows.h)
# #104: the current git commit, stamped into the on-screen version line ("FreeFalcon: 7.0.1 Linux build <hash>").
# Resolved at configure time -- re-run cmake to refresh it after new commits.
execute_process(
    COMMAND git -C ${FF_ROOT} rev-parse --short HEAD
    OUTPUT_VARIABLE FF_GIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if(NOT FF_GIT_HASH)
    set(FF_GIT_HASH "unknown")
endif()
message(STATUS "ffviper build stamp: FreeFalcon: 7.0.1 Linux build ${FF_GIT_HASH}")

target_compile_definitions(ff_shim INTERFACE
    FF_GIT_HASH="${FF_GIT_HASH}"
    # re-inject the GCC atomic macros -fms-compatibility drops but libstdc++ <atomic> needs (all 2 except TAS=1)
    __GCC_ATOMIC_BOOL_LOCK_FREE=2 __GCC_ATOMIC_CHAR_LOCK_FREE=2 __GCC_ATOMIC_CHAR16_T_LOCK_FREE=2
    __GCC_ATOMIC_CHAR32_T_LOCK_FREE=2 __GCC_ATOMIC_WCHAR_T_LOCK_FREE=2 __GCC_ATOMIC_SHORT_LOCK_FREE=2
    __GCC_ATOMIC_INT_LOCK_FREE=2 __GCC_ATOMIC_LONG_LOCK_FREE=2 __GCC_ATOMIC_LLONG_LOCK_FREE=2
    __GCC_ATOMIC_POINTER_LOCK_FREE=2 __GCC_ATOMIC_TEST_AND_SET_TRUEVAL=1
    # ST80 is a Windows x86 .lib (st80w) chatter voice codec with no source and no x64/Linux build; disable it
    # (lhsp.cpp guards the ST80_Decode path on NO_ST80). Same decision as the x64 port.
    NO_ST80)

# ---- helper: add an engine static lib from a .vcxproj's ClCompile list ----
# The Windows project is the source of truth for WHICH files compile (globbing picks up dead/excluded files such as
# code behind #if 0). We read the .vcxproj, extract every <ClCompile Include="..."/>, normalise the path, and build
# those. So the Linux build tracks the Windows source list automatically.
# Usage: ff_engine_lib_from_vcxproj(<name> <vcxproj relative to FF_ROOT>)
# Artscout - 2026 (Linux port): Windows-only translation units that have no Linux meaning and are
# intentionally left out of the Linux build (their function lives in the SDL/POSIX platform layer or
# is a Windows-only tool/dependency). Matched as a substring of each source's path.
#   ehandler.cpp / crashhandler -> imagehlp/BugslayerUtil (Win32 crash dump)
#   dispcfg.cpp                 -> WNDCLASS/CreateWindow game window, replaced by ffplatform (SDL3)
#   F4Comms.cpp                 -> cguid.h (DirectPlay COM GUIDs)
#   camptool/Dialog.cpp,Unitdlg.cpp -> commdlg.h (Win32 common dialogs, campaign editor tool)
set(FF_LINUX_EXCLUDE
    "/falclib/ehandler.cpp"
    "/camptool/Dialog.cpp"
    "/camptool/Unitdlg.cpp"
    # audio voice periphery that is pure Win32 UI / Windows app integration:
    "/falcsnd/winampfrontend.cpp"  # Winamp window messaging (FindWindow/GetWindowText)
    # build-time Windows GDI tool wired into ui.vcxproj: bakes a font bitmap via EnumFonts/GetDC/SetPixel.
    # Its output is a data artifact, not game runtime -- has no place in the Linux executable.
    "/Tools/ui_tools/savefont.cpp"
    # Windows-only peripherals that belong to not-yet-ported subsystems (kept intact for the Windows build):
    "/common/printer.cpp"   # COM printer via #import "GMPrint.tlb" (MSVC-only) -- desktop printing, unported
    "/ddstuff/dxerror.cpp"  # DirectSound/DirectDraw HRESULT->string decoder -- dead on Linux (audio->OpenAL)
    "/comms/serverbrowser.cpp"  # GameSpy multiplayer server browser -- entirely GNETCORELib COM/#import, unported
    "/setup/simoptn.cpp")   # legacy Win32 USER32 sim-options DIALOG -- replaced by the ui95 tab UI (SimTab); dead (only a stray prototype in winmain)

# Artscout - 2026 (#104): resolve a path whose case may not match the case-sensitive Linux filesystem.
# The Windows .vcxproj files record source paths in the casing the authors happened to type (e.g.
# "Src\UI_Main.cpp"), which resolved fine on the old case-INSENSITIVE WSL/NTFS mount. On native ext4 the
# real files are "src/ui_main.cpp", so the literal path misses. This walks the path component by component
# from FF_ROOT, and at any level where the exact spelling is absent, matches the one on-disk entry whose
# name is case-insensitively equal. Keeps the .vcxproj (Windows source of truth) untouched -- the casing
# fix lives only in the Linux build glue. Returns the original path if no match (so the error stays visible).
function(ff_ci_resolve out_var abspath)
    if(EXISTS "${abspath}")
        set(${out_var} "${abspath}" PARENT_SCOPE)
        return()
    endif()
    file(RELATIVE_PATH _relp "${FF_ROOT}" "${abspath}")
    string(REPLACE "/" ";" _parts "${_relp}")
    set(_cur "${FF_ROOT}")
    foreach(_p ${_parts})
        if(EXISTS "${_cur}/${_p}")
            set(_cur "${_cur}/${_p}")
        else()
            string(TOLOWER "${_p}" _plow)
            file(GLOB _entries RELATIVE "${_cur}" "${_cur}/*")
            set(_hit "")
            foreach(_e ${_entries})
                string(TOLOWER "${_e}" _elow)
                if(_elow STREQUAL _plow)
                    set(_hit "${_e}")
                    break()
                endif()
            endforeach()
            if(_hit)
                set(_cur "${_cur}/${_hit}")
            else()
                set(${out_var} "${abspath}" PARENT_SCOPE)
                return()
            endif()
        endif()
    endforeach()
    set(${out_var} "${_cur}" PARENT_SCOPE)
endfunction()

function(ff_engine_lib_from_vcxproj name vcxproj)
    # Artscout - 2026: resolve the PROJECT path case-insensitively too. A miss
    # here read nothing and produced an EMPTY library -- 166 undefined symbols
    # at the exe link, with no word about the cause.
    ff_ci_resolve(_proj "${FF_ROOT}/${vcxproj}")
    if(NOT EXISTS "${_proj}")
        message(FATAL_ERROR "ff_engine_lib_from_vcxproj: ${vcxproj} not found")
    endif()
    get_filename_component(projdir ${_proj} DIRECTORY)
    file(READ ${_proj} content)
    string(REGEX MATCHALL "<ClCompile Include=\"[^\"]+\"" matches "${content}")
    set(srcs "")
    foreach(m ${matches})
        string(REGEX REPLACE "<ClCompile Include=\"([^\"]+)\"" "\\1" rel "${m}")
        string(REPLACE "\\" "/" rel "${rel}")
        set(_skip FALSE)
        # Match excludes case-INSENSITIVELY: the .vcxproj keeps the authors' casing (e.g. "SimOptn.cpp") which
        # need not match the lowercase exclude patterns, and this comparison runs before ff_ci_resolve.
        string(TOLOWER "${projdir}/${rel}" _rellow)
        foreach(ex ${FF_LINUX_EXCLUDE})
            string(TOLOWER "${ex}" _exlow)
            string(FIND "${_rellow}" "${_exlow}" _pos)
            if(NOT _pos EQUAL -1)
                set(_skip TRUE)
            endif()
        endforeach()
        if(NOT _skip)
            ff_ci_resolve(_realsrc "${projdir}/${rel}")
            list(APPEND srcs ${_realsrc})
        endif()
    endforeach()
    if(NOT srcs)
        message(WARNING "ff_engine_lib_from_vcxproj(${name}): no ClCompile sources in ${vcxproj}")
        return()
    endif()
    add_library(${name} STATIC ${srcs})
    target_link_libraries(${name} PUBLIC ff_shim)
endfunction()

# ---- shared HLSL shaders (DXC -> SPIR-V; -> DXIL as well on Windows) ----
# Declared before ffvulkan, which links it.
add_subdirectory(graphics/shaders)

# ---- platform layer + Vulkan renderer (own CMakeLists) ----
add_subdirectory(platform/ffplatform)
add_subdirectory(graphics/vulkan)

# Artscout - 2026 (#104): Linux definitions of the backend-neutral render globals + D3D12 link stubs. The Windows
# builds keep these in the (Windows-only) D3D12 sources; on Linux those TUs are absent, so shared code that names
# the symbols would fail to link. One tiny always-compiled TU supplies them (no-op on Windows via #ifndef _WIN32).
add_library(ff_backend_globals STATIC platform/win32shim/backend_globals_linux.cpp)
target_link_libraries(ff_backend_globals PUBLIC ff_shim)

# The headless Vulkan GPU test (defined in graphics/vulkan) links only ffvulkan, but VulkanRenderer references the
# render globals (g_bGpuDraw, g_fGpuTerrain*Bias) whose Linux definitions live in ff_backend_globals. Wire it here,
# after both targets exist (the test target is created earlier, in the vulkan subdirectory).
if(TARGET ffvk_gputest)
    target_link_libraries(ffvk_gputest PRIVATE ff_backend_globals)
endif()

# ---- audio backend: DirectSound interfaces implemented over OpenAL (Phase 1) ----
find_library(OPENAL_LIB NAMES openal)
add_library(ff_audio_openal STATIC platform/win32shim/dsound_openal.cpp)
target_link_libraries(ff_audio_openal PUBLIC ff_shim ${OPENAL_LIB})

# ---- engine subsystems: one static lib per Windows .vcxproj (game logic; platform subsystems come as ports) ----
ff_engine_lib_from_vcxproj(ff_vu2 vu2/vu2.vcxproj)

# auto-add the game-logic projects: falclib + campaign/* + sim/*. Build with `cmake --build ... -k 0` to compile
# every lib independently and see how much of the engine builds; the grind fixes per-lib blockers.
file(GLOB engine_projs
    ${FF_ROOT}/falclib/falclib.vcxproj
    ${FF_ROOT}/campaign/*/*.vcxproj
    ${FF_ROOT}/sim/*/*.vcxproj
    # platform subsystems being ported: audio (falcsnd -> OpenAL), network (comms -> BSD sockets),
    # UI toolkit (ui95/ui95_ext -> freetype/SDL_ttf font backend)
    ${FF_ROOT}/falcsnd/falcsnd.vcxproj
    ${FF_ROOT}/comms/comms.vcxproj
    ${FF_ROOT}/ui95/ui95.vcxproj
    ${FF_ROOT}/ui95_ext/ui95_ext.vcxproj
    # NB: codelib/tools/lists + codelib/resources/reslib (resmgr = zlib) are PURE-C libs and are deliberately NOT
    # wired here. The win32 foundation shim is C++-only (pre-includes the STL, defines COM classes / REFGUID
    # references), so those .c TUs need a C-mode shim path first (guard the STL/COM/reference sections under
    # __cplusplus and port win32_ui.h/win32_strings.h to compile as C). Tracked as a Linux-port blocker.
    # graphics geometry/render-support libs (NOT dxengine.vcxproj -- that is the Windows D3D backend; on Linux only
    # its ffstatemap.cpp + OpenXRBackend.cpp are pulled into ffvulkan). These reach the Vulkan renderer via the
    # abstracted Renderer interface. Wired into the grind so `make -k` shows their per-lib blockers toward the exe.
    ${FF_ROOT}/graphics/3dlib/3dlib.vcxproj
    ${FF_ROOT}/graphics/bsplib/bsplib.vcxproj
    ${FF_ROOT}/graphics/ddstuff/ddstuff.vcxproj
    ${FF_ROOT}/graphics/objects/objects.vcxproj
    ${FF_ROOT}/graphics/terrain/terrain.vcxproj
    ${FF_ROOT}/graphics/texture/texture.vcxproj
    ${FF_ROOT}/graphics/utils/utils.vcxproj
    ${FF_ROOT}/graphics/weather/weather.vcxproj
    ${FF_ROOT}/graphics/renderer/renderer.vcxproj
    # ACMI (air-combat maneuvering instrumentation) recorder/playback -- pure game logic, no D3D. Defines
    # ACMIRecorder / the gACMIRec tape used by the sim + UI.
    ${FF_ROOT}/acmi/acmi.vcxproj
    # main UI + the game entry (ui/src/winmain.cpp lives in ui.vcxproj) and its per-screen UI sub-libs.
    ${FF_ROOT}/ui/ui.vcxproj
    ${FF_ROOT}/ui/src/*/*.vcxproj)
foreach(p ${engine_projs})
    get_filename_component(pname ${p} NAME_WE)
    file(RELATIVE_PATH rel ${FF_ROOT} ${p})
    ff_engine_lib_from_vcxproj(ff_${pname} ${rel})
endforeach()

# radix.c (3dlib polygon depth radix-sort, RadixSortDescending) and magvar.c (icp magnetic-variation lookup,
# getVar) are pure C but lean on the C++-only foundation shim. A lowercase .c compiles as C and fails on the
# shim's STL; force them to build as C++ (both compile clean that way).
set_source_files_properties(${FF_ROOT}/graphics/3dlib/radix.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(${FF_ROOT}/sim/icp/magvar.c PROPERTIES LANGUAGE CXX)
# Artscout - 2026: ALLOC.C is C++ (MSVC compiles an uppercase .C as C++, and
# context.cpp calls Alloc/AllocInit with C++ linkage). Lowercased on Linux it
# would build as C and the mangled names would not resolve.
ff_ci_resolve(_ff_alloc_cxx "${FF_ROOT}/graphics/3dlib/ALLOC.C")
set_source_files_properties(${_ff_alloc_cxx} PROPERTIES LANGUAGE CXX)

# Artscout - 2026 (#104, Linux): the comms sockets layer is pure-C Winsock (capi/tcp/udp/rudp/ws2init). A lowercase .c
# compiles as C and never sees the C++-only foundation shim (windows.h/winsock.h), so on Linux these TUs were dropped
# -- leaving ComAPI*/com_API_*/force_ip_address undefined. Winsock IS the BSD API with a Microsoft accent, so the
# win32shim/winsock.h header maps the names 1:1; force these C sources to build as C++ so they pick the shim up.
set_source_files_properties(
    ${FF_ROOT}/comms/capi.c ${FF_ROOT}/comms/tcp.c ${FF_ROOT}/comms/udp.c
    ${FF_ROOT}/comms/rudp.c ${FF_ROOT}/comms/ws2init.c
    PROPERTIES LANGUAGE CXX)
# (sim/icp/magvar.c LANGUAGE CXX is set with radix.c above.)

# Artscout - 2026 (#104): CDXEngine -- the shared 2D/particle/state/light engine facade (the global TheDXEngine
# the whole engine draws through). It is backend-neutral (GPU work routes via g_pRenderer/g_pRenderBackend), so it
# MUST compile on Linux -- unlike the concrete D3D12/OpenXR backends that live in the same dxengine.vcxproj (those
# stay Windows-only; the Vulkan backend + ffstatemap + OpenXR interop are built into ffvulkan). Compile only the
# neutral CDXEngine sources here.
add_library(ff_dxengine STATIC
    graphics/dxengine/dxengine.cpp
    graphics/dxengine/dx2dengine.cpp
    graphics/dxengine/dxlightengine.cpp
    graphics/dxengine/dxtools.cpp
    graphics/dxengine/dxvbmanager.cpp)
target_link_libraries(ff_dxengine PUBLIC ff_shim)

# Artscout - 2026 (#104): the POSIX foundation shim implementation (CreateFile->open, CreateThread->pthread,
# events->mutex/condvar, GetPrivateProfile* .ini parser, _findfirst) that backs the win32 handle/file/thread API
# the engine calls throughout. Header-declared in win32_file.h/win32_sync.h; the definitions live here.
add_library(ff_win32compat STATIC platform/win32shim/win32compat.cpp)
target_link_libraries(ff_win32compat PUBLIC ff_shim)

# Mono debug console (MonoPrint) -- the engine's logging sink, referenced almost everywhere. monodraw.cpp (the
# secondary mono-monitor GRAPHICS display, DOS/Windows debug hardware) is not built on Linux.
add_library(ff_mono STATIC tools/mono/debuggr.cpp)
target_link_libraries(ff_mono PUBLIC ff_shim)

# Pointer-validity guards (F4IsBadReadPtr/WritePtr/CodePtr) -- an address-range check (portable, no Win32
# IsBadReadPtr), referenced ~130x across sim/graphics for UAF hardening. Its own tool TU, not in any lib.
add_library(ff_isbad STATIC tools/isbad/isbad.cpp)
target_link_libraries(ff_isbad PUBLIC ff_shim)

# Resource manager (ResFOpen/ResFClose/ResAddPath/... over zip archives) = codelib/resources/reslib/src/{resmgr.c,
# inflate.c, unzip.c} + codelib/tools/lists. Old-Watcom-C -> C++ port (#104): the .c files are compiled as C++
# (LANGUAGE CXX) because the foundation shim windows.h is C++-only.
add_library(ff_reslib STATIC
    codelib/resources/reslib/src/resmgr.c
    codelib/resources/reslib/src/inflate.c
    codelib/resources/reslib/src/unzip.c
    codelib/tools/lists/lists.c)
set_source_files_properties(
    codelib/resources/reslib/src/resmgr.c
    codelib/resources/reslib/src/inflate.c
    codelib/resources/reslib/src/unzip.c
    codelib/tools/lists/lists.c
    PROPERTIES LANGUAGE CXX)
target_include_directories(ff_reslib PRIVATE
    ${FF_ROOT}/codelib/tools/lists
    ${FF_ROOT}/codelib/resources/reslib/src)
target_link_libraries(ff_reslib PUBLIC ff_shim)

# ---- the game executable ------------------------------------------------------------------------------------
# Artscout - 2026 (#104): the ffviper Linux game target. It links every engine static lib plus the platform
# layer (SDL3), the Vulkan renderer, and the OpenAL audio backend. The engine libs have cyclic dependencies
# (e.g. sim <-> graphics <-> ui), so they are wrapped in --start-group/--end-group for repeated symbol
# resolution. The entry point is main() in ff_main_linux.cpp, which forwards into the real handle_WinMain().
# This target is expected to surface undefined symbols during the port grind; build it to measure progress:
#   cmake --build build-linux --target ffviper
# Collect every ff_* static lib target that exists (added above) so new libs are picked up automatically.
get_property(_all_targets DIRECTORY ${FF_ROOT} PROPERTY BUILDSYSTEM_TARGETS)
set(FF_ENGINE_LIBS "")
foreach(t ${_all_targets})
    if(t MATCHES "^ff_" AND NOT t STREQUAL "ff_shim")
        get_target_property(_ttype ${t} TYPE)
        if(_ttype STREQUAL "STATIC_LIBRARY")
            list(APPEND FF_ENGINE_LIBS ${t})
        endif()
    endif()
endforeach()

find_library(VULKAN_LIB NAMES vulkan)
find_library(OPENXR_LIB NAMES openxr_loader)
find_library(ZLIB_LIB NAMES z)

# The exe's own translation units come from the root Falcon4.vcxproj (they are NOT in any src/ sub-lib .vcxproj):
# winmain.cpp (handle_WinMain -- the shared boot path ff_main_linux.cpp forwards into; plus the Win32 WinMain +
# message pump, being ported to the SDL3/ffplatform event loop), f4config.cpp (the config console), and the
# RedProfiler sampler. On Linux the entry point is main() in ff_main_linux.cpp instead of WinMain.
add_executable(ffviper
    platform/ffplatform/ff_main_linux.cpp
    ui/src/winmain.cpp
    ui/src/f4config.cpp
    ${FF_ROOT}/../RedProfiler.cpp)
target_link_libraries(ffviper PRIVATE ff_shim)
# --start-group over the whole engine (cyclic deps). ffvulkan + ffplatform + audio + backend globals are in the
# group too so their cross-references resolve. System libs follow the group.
target_link_libraries(ffviper PRIVATE
    -Wl,--start-group
    ${FF_ENGINE_LIBS} ff_backend_globals ff_audio_openal ffvulkan ffplatform
    -Wl,--end-group
    ${OPENAL_LIB} ${VULKAN_LIB} ${OPENXR_LIB} ${ZLIB_LIB}
    SDL3 pthread dl m)
