# Artscout - 2026 (#104, Linux port). The Vulkan render backend (Linux peer of D3D12Backend), built as a static
# lib. Depends on the ffplatform layer (for the SDL window handle) + Vulkan + SDL3. clang-only on Linux (win32 shim).
cmake_minimum_required(VERSION 3.16)
project(ffvulkan LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

# SDL3 (same resolution as ffplatform).
find_package(SDL3 QUIET)
if(NOT SDL3_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)
    set(FFVK_SDL_TARGET PkgConfig::SDL3)
else()
    set(FFVK_SDL_TARGET SDL3::SDL3)
endif()

add_library(ffvulkan STATIC
    vulkanbackend.cpp
    vulkanrenderer.cpp
    vulkantexturemanager.cpp
    vulkanvma.cpp
    vulkanvbmanager.cpp
    # Artscout - 2026 (#104): the shared legacy STATE_* -> {FF_* flags, blend, filter, addressing, depth} table. Pure
    # (depends on ffstates.h alone since the enum was split out), so both backends translate through the SAME source
    # instead of the Vulkan side re-inventing -- or, as it did until now, ignoring the legacy state model entirely.
    ../dxengine/common/ffstatemap.cpp
    # Artscout - 2026 (#108): the OpenXR VR backend. Windows compiles all three graphics bindings (D3D11/D3D12/Vulkan);
    # on Linux only the Vulkan binding path is active (the D3D paths are under #ifdef _WIN32). Lives here (not dxengine)
    # because the live Linux VR path is OpenXR<->Vulkan, right next to VulkanBackend (#109 enable2 hooks).
    ../dxengine/openxrbackend.cpp
)

# ---- shaders ----
# Artscout - 2026 (#78): the GLSL set is gone. Every shader is now HLSL under
# graphics/shaders, compiled by DXC into the binary (target `ffshaders`), so
# there is no .spv on disk and one source feeds both backends.

target_include_directories(ffvulkan PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../dxengine        # irenderbackend.h / irenderer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include         # ffstates.h (the STATE_* enum ffstatemap.h needs)
    ${CMAKE_CURRENT_SOURCE_DIR}/../../platform/ffplatform
    ${CMAKE_CURRENT_SOURCE_DIR}/../..)             # src root: extlibs/vma/vk_mem_alloc.h

# ---- OpenXR headers (#108 VR on Linux) ----
# Prefer the vendored 1.1.61 headers (vulkan_enable2 + quad_views + hand_tracking, matching the Windows NuGet); fall
# back to a system install. The loader (.so) is linked only for the final exe -- the static lib just needs the headers.
set(FFXR_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/../../../packages/OpenXR.Loader.1.1.61/include")
if(EXISTS "${FFXR_HEADERS}/openxr/openxr.h")
    target_include_directories(ffvulkan PUBLIC ${FFXR_HEADERS})
    message(STATUS "ffvulkan: OpenXR headers (vendored 1.1.61) -> ${FFXR_HEADERS}")
else()
    find_path(OPENXR_INCLUDE_DIR openxr/openxr.h)
    if(OPENXR_INCLUDE_DIR)
        target_include_directories(ffvulkan PUBLIC ${OPENXR_INCLUDE_DIR})
        message(STATUS "ffvulkan: OpenXR headers (system) -> ${OPENXR_INCLUDE_DIR}")
    else()
        message(WARNING "ffvulkan: OpenXR headers not found (vendored or system) -- OpenXRBackend will fail to compile.")
    endif()
endif()
# Loader link (best-effort; only needed at exe link time, TODO for the exe milestone).
find_library(OPENXR_LOADER_LIB NAMES openxr_loader)
if(OPENXR_LOADER_LIB)
    target_link_libraries(ffvulkan PUBLIC ${OPENXR_LOADER_LIB})
    message(STATUS "ffvulkan: OpenXR loader -> ${OPENXR_LOADER_LIB}")
else()
    message(STATUS "ffvulkan: libopenxr_loader not found -- link deferred to the exe milestone (#108 TODO).")
endif()

if(NOT WIN32)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "The Linux build needs clang (win32 shim uses -fms-compatibility). Use -DCMAKE_CXX_COMPILER=clang++")
    endif()
    target_include_directories(ffvulkan PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../platform/win32shim)
    target_compile_options(ffvulkan PRIVATE
        -fms-extensions -fms-compatibility -fms-compatibility-version=19.20 -Wno-register)
    target_compile_definitions(ffvulkan PRIVATE
        __GCC_ATOMIC_BOOL_LOCK_FREE=2 __GCC_ATOMIC_CHAR_LOCK_FREE=2 __GCC_ATOMIC_CHAR16_T_LOCK_FREE=2
        __GCC_ATOMIC_CHAR32_T_LOCK_FREE=2 __GCC_ATOMIC_WCHAR_T_LOCK_FREE=2 __GCC_ATOMIC_SHORT_LOCK_FREE=2
        __GCC_ATOMIC_INT_LOCK_FREE=2 __GCC_ATOMIC_LONG_LOCK_FREE=2 __GCC_ATOMIC_LLONG_LOCK_FREE=2
        __GCC_ATOMIC_POINTER_LOCK_FREE=2 __GCC_ATOMIC_TEST_AND_SET_TRUEVAL=1)
endif()

target_link_libraries(ffvulkan PUBLIC Vulkan::Vulkan ${FFVK_SDL_TARGET})

# Artscout - 2026 (#78): the embedded HLSL bytecode (mesh-shader terrain).
target_link_libraries(ffvulkan PUBLIC ffshaders)

# ---- headless GPU test (optional) ----
# Renders a textured quad offscreen and reads it back; run under lavapipe for a software check. Links the
# ffplatform window sources (VulkanBackend::Init pulls SDL_Vulkan_* even though the test uses InitHeadless).
option(FFVK_BUILD_TESTS "Build the headless Vulkan GPU test" ON)
if(FFVK_BUILD_TESTS AND FFVK_SPV)   # needs the compiled SPIR-V
    add_executable(ffvk_gputest
        tests/gputest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../platform/ffplatform/ff_window.cpp)
    target_link_libraries(ffvk_gputest PRIVATE ffvulkan)
    # vulkanrenderer.cpp calls the case-insensitive open helpers (FF_CIFopen/FF_CIResolvePath) from
    # win32compat; the main exe gets them via ff_win32compat, the standalone test must link them too.
    # (Plain target name: ff_win32compat is defined later in the parent scope; CMake resolves it at generate.)
    target_link_libraries(ffvk_gputest PRIVATE ff_win32compat)
    add_dependencies(ffvk_gputest ffvulkan_shaders)
    # copy the compiled shaders next to the test so it finds them with the default arg
    add_custom_command(TARGET ffvk_gputest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${FFVK_SHADER_OUT_DIR} $<TARGET_FILE_DIR:ffvk_gputest>/shaders)
endif()
