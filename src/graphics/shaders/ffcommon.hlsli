// ffcommon.hlsli -- Artscout - 2026: the object-pass constant buffer and the
// FF_* flag set, shared by every shader that binds set 0. One declaration, so
// the layout cannot drift between shaders (or from the C++ ObjUbo mirror).
#ifndef FF_COMMON_HLSLI
#define FF_COMMON_HLSLI

// MUST match ffstatemap.h / VulkanRenderer.cpp / ffemu.hlsl.
#define FF_TEXTURE0        (1u << 0)
#define FF_TEXTURE1        (1u << 1)
#define FF_VERTEXCOLOR     (1u << 2)
#define FF_LIGHTING        (1u << 3)
#define FF_CHROMAKEY       (1u << 4)
#define FF_ALPHATEST       (1u << 5)
#define FF_FOG             (1u << 6)
#define FF_MODULATE2X      (1u << 7)
#define FF_TEXCOLORDIFFUSE (1u << 8)
#define FF_RTTSOFT         (1u << 9)
#define FF_WATER           (1u << 10)
#define FF_EMISSIVE        (1u << 11)
#define FF_AFTERBURNER     (1u << 12)
#define FF_COCKPIT         (1u << 13)
#define FF_IRGREY          (1u << 14)
#define FF_GLOC            (1u << 15)
#define FF_NVG             (1u << 16)
#define FF_FULLBRIGHT      (1u << 17)
#define FF_BINDLESS        (1u << 19)

struct Light
{
    float4 position;
    float4 direction;
    float4 color;
    float4 params; // x = range, y = type (0 = directional, 1 = point)
};

// Field order is load-bearing: VulkanRenderer memcpy's its ObjUbo straight in
// and static_asserts the offsets. Do not reorder without updating both.
[[vk::binding(0, 0)]]
cbuffer ObjUbo : register(b0)
{
    row_major float4x4 gWorld;
    row_major float4x4 gView[4];  // per view (multiview: stereo / quad)
    row_major float4x4 gProj[4];
    float4 gMaterialColor;
    float4 gAmbient;
    float4 gCamPos;
    float4 gParams;    // x = lit, yz = screen size (px)
    uint4  gFlags;     // x = FF_* bitmask
    uint4  gNumLights;
    float4 gFogColor;
    float4 gFogParams; // x = start, y = end, z = alphaRef, w = time (sec)
    float4 gChromaKey; // rgb = chroma, w = tolerance
    float4 gSpec;      // rgb = specular colour, w = power (0 = none)
    float4 gGloc;      // x = intensity, y = innerR, z = outerR
    Light  gLights[8];
};

bool Has(uint f)
{
    return (gFlags.x & f) != 0u;
}

#endif // FF_COMMON_HLSLI
