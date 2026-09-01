# Artscout - 2026: the shared HLSL shader set. DXC compiles each manifest entry
# into a C array header (-Fh/-Vn); ffshaderblobs.cpp includes them, so the
# bytecode ships inside the binary and no .spv is loaded from disk.
cmake_minimum_required(VERSION 3.16)
project(ffshaders LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FFSH_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(FFSH_GEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/generated)
set(FFSH_MANIFEST ${FFSH_SRC_DIR}/ffshaders.manifest)

# The Vulkan SDK ships a DXC with the SPIR-V backend; a distro dxc usually has it
# too. A plain Windows SDK dxc.exe does NOT -- the .bat script picks per target.
find_program(FFSH_DXC NAMES dxc HINTS $ENV{VULKAN_SDK}/bin $ENV{VULKAN_SDK}/Bin)

# Every blob the manifest names, so the custom command has real outputs and a
# shader edit forces a recompile (a stale .h silently shipping the old shader is
# the classic trap).
set(FFSH_OUTPUTS "")
file(STRINGS ${FFSH_MANIFEST} FFSH_LINES)
foreach(line ${FFSH_LINES})
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    string(REPLACE "|" ";" fields "${line}")
    list(LENGTH fields nfields)
    if(nfields LESS 6)
        continue()
    endif()
    list(GET fields 5 var)
    string(STRIP "${var}" var)
    list(APPEND FFSH_OUTPUTS ${FFSH_GEN_DIR}/${var}.h)
endforeach()

if(FFSH_DXC)
    add_custom_command(OUTPUT ${FFSH_OUTPUTS}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${FFSH_GEN_DIR}
        COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/../../../tools/build_shaders.sh
                ${FFSH_SRC_DIR} ${FFSH_GEN_DIR} ${FFSH_DXC}
        DEPENDS ${FFSH_MANIFEST} ${FFSH_SRC_DIR}/ffterrain.hlsl
                ${FFSH_SRC_DIR}/ffobject.hlsl ${FFSH_SRC_DIR}/ffscreen.hlsl
                ${FFSH_SRC_DIR}/ffparticle.hlsl
                ${FFSH_SRC_DIR}/ffcomposite.hlsl
                ${FFSH_SRC_DIR}/ffxrblit.hlsl ${FFSH_SRC_DIR}/ffcommon.hlsli
        VERBATIM COMMENT "dxc: ffshaders.manifest -> generated/")
    add_custom_target(ffshaders_gen DEPENDS ${FFSH_OUTPUTS})
else()
    message(FATAL_ERROR
        "ffshaders: dxc not found. Install the Vulkan SDK (or directx-shader-compiler) "
        "-- the mesh-shader terrain needs SPIR-V from HLSL.")
endif()

add_library(ffshaders STATIC ffshaderblobs.cpp)
add_dependencies(ffshaders ffshaders_gen)
target_include_directories(ffshaders PUBLIC ${FFSH_SRC_DIR})
target_link_libraries(ffshaders PUBLIC ff_shim)
