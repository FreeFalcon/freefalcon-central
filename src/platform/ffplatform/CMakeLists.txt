# Artscout - 2026 (#104, Linux port). The cross-platform platform layer (window + events on SDL3), built as a
# static lib the game links on both Windows and Linux. This is the first piece of the Linux (CMake) build; the
# Windows build keeps its MSVC .vcxproj and just adds these two sources + the SDL3 dependency. Guard-free: one
# implementation, both platforms.
cmake_minimum_required(VERSION 3.16)
project(ffplatform LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 QUIET)
if(NOT SDL3_FOUND)
    # Fall back to pkg-config (Ubuntu libsdl3-dev ships an sdl3.pc).
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)
    set(FFPLATFORM_SDL_TARGET PkgConfig::SDL3)
else()
    set(FFPLATFORM_SDL_TARGET SDL3::SDL3)
endif()

add_library(ffplatform STATIC
    ff_window.cpp
    ff_events.cpp
    ff_dinput.cpp   # DirectInput8 implemented over SDL3 (#104 input Ф3); no-op on Windows
)

target_include_directories(ffplatform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# On Linux the Win32 vocabulary (HWND/UINT/WPARAM/...) comes from the foundation shim; on Windows from the SDK.
if(NOT WIN32)
    # The foundation shim relies on clang's MSVC-compatibility mode (-fms-compatibility); GCC does not support it.
    # Configure the Linux build with clang, e.g.  cmake -DCMAKE_CXX_COMPILER=clang++ ...
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR
            "The Linux build needs clang (the win32 shim uses -fms-compatibility, unsupported by GCC). "
            "Re-run: cmake -DCMAKE_CXX_COMPILER=clang++ -S . -B build")
    endif()
    target_include_directories(ffplatform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../win32shim)
    # clang needs the MSVC-ism flags + the atomic macros the -fms-compatibility path drops (see linux-compile-check.sh).
    target_compile_options(ffplatform PRIVATE
        -fms-extensions -fms-compatibility -fms-compatibility-version=19.20 -Wno-register)
    target_compile_definitions(ffplatform PRIVATE
        __GCC_ATOMIC_BOOL_LOCK_FREE=2 __GCC_ATOMIC_CHAR_LOCK_FREE=2 __GCC_ATOMIC_CHAR16_T_LOCK_FREE=2
        __GCC_ATOMIC_CHAR32_T_LOCK_FREE=2 __GCC_ATOMIC_WCHAR_T_LOCK_FREE=2 __GCC_ATOMIC_SHORT_LOCK_FREE=2
        __GCC_ATOMIC_INT_LOCK_FREE=2 __GCC_ATOMIC_LONG_LOCK_FREE=2 __GCC_ATOMIC_LLONG_LOCK_FREE=2
        __GCC_ATOMIC_POINTER_LOCK_FREE=2 __GCC_ATOMIC_TEST_AND_SET_TRUEVAL=1)
endif()

target_link_libraries(ffplatform PUBLIC ${FFPLATFORM_SDL_TARGET})
