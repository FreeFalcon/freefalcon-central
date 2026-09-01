// ffshaderblobs.h -- Artscout - 2026: shader bytecode baked into the binary.
// One HLSL source per subject, compiled by DXC to DXIL (D3D12) and SPIR-V
// (Vulkan) at build time -- see ffshaders.manifest / tools/build_shaders.
#ifndef _FFSHADERBLOBS_H_
#define _FFSHADERBLOBS_H_

enum FFShaderId
{
    FFSHADER_TERRAIN_AS = 0, // per-chunk LOD pick + frustum cull
    FFSHADER_TERRAIN_MS,     // builds the post grid out of the clipmap
    FFSHADER_TERRAIN_PS,
    // The engine passes (Vulkan): SPIR-V only, D3D12 runs ffemu.hlsl.
    FFSHADER_OBJECT_VS,
    FFSHADER_OBJECT_PS,
    FFSHADER_SCREEN_VS,
    FFSHADER_SCREEN_PS,
    FFSHADER_PARTICLE_VS,
    FFSHADER_PARTICLE_PS,
    FFSHADER_COMPOSITE_VS,
    FFSHADER_COMPOSITE_PS,
    FFSHADER_XRBLIT_VS, // scene -> XR image, undoing the sRGB double-encode
    FFSHADER_XRBLIT_PS,
    FFSHADER_COUNT
};

enum FFShaderTarget
{
    FFSHADER_DXIL = 0, // D3D12
    FFSHADER_SPIRV     // Vulkan
};

// NULL when this build produced no such blob (e.g. DXIL on Linux).
const void* FFGetShaderBlob(FFShaderId id, FFShaderTarget target,
                            unsigned int* sizeBytes);
const char* FFGetShaderName(FFShaderId id);

#endif // _FFSHADERBLOBS_H_
