// ffscreen.hlsl -- Artscout - 2026: the 2D/UI screen pass, HLSL.
// Pre-transformed D3D XYZRHW geometry -> NDC, then gTex0 * vertex colour.
// Ported from the GLSL pair; the push-constant block is byte-identical.

// viewport = the pixel extent this geometry was projected against.
// flipY    = 1 in a pass with a NEGATIVE-height viewport (the scene pass).
// rttSoft  = FF_RTTSOFT, the additive RTT-atlas composite.
[[vk::push_constant]]
struct ScreenPush
{
    float2 viewport;
    float flipY;
    float rttSoft;
} pc;

[[vk::combinedImageSampler]][[vk::binding(0, 0)]] Texture2D gTex0;
[[vk::combinedImageSampler]][[vk::binding(0, 0)]] SamplerState gTex0Samp;

struct VSIn
{
    [[vk::location(0)]] float4 pos   : POSITION; // screen x,y,z,rhw
    [[vk::location(1)]] float4 color : COLOR0;
    [[vk::location(2)]] float2 uv    : TEXCOORD0;
};

struct VSOut
{
    float4 pos : SV_Position;
    [[vk::location(0)]] float4 color : COLOR0;
    [[vk::location(1)]] float2 uv    : TEXCOORD0;
};

struct VSOutVs
{
    float4 pos : SV_Position;
    [[vk::builtin("PointSize")]] float psize : PSIZE;
    [[vk::location(0)]] float4 color : COLOR0;
    [[vk::location(1)]] float2 uv    : TEXCOORD0;
};

VSOutVs VS_Screen(VSIn i)
{
    VSOutVs o;
    o.psize = 1.0f; // the 2D path builds a POINT_LIST pipeline for primType 1

    // screen (origin top-left, y down) -> Vulkan NDC (y down).
    float x = (i.pos.x / max(pc.viewport.x, 1.0f)) * 2.0f - 1.0f;
    float y = (i.pos.y / max(pc.viewport.y, 1.0f)) * 2.0f - 1.0f;

    // The scene pass uses a NEGATIVE-height viewport (its matrices are
    // D3D-convention), which would mirror 2D drawn there -- negate to cancel.
    if (pc.flipY > 0.5f)
        y = -y;

    // Perspective-correct UVs for XYZRHW geometry: pos.w is rhw = 1/w. Dropping
    // it gives affine interpolation, which stretches a quad seen at an angle
    // (the VR display panels). Reconstructing w keeps the same NDC.
    const float w = (i.pos.w > 1e-6f) ? (1.0f / i.pos.w) : 1.0f;
    o.pos = float4(x * w, y * w, i.pos.z * w, w);
    o.color = i.color;
    o.uv = i.uv;
    return o;
}

float4 PS_Screen(VSOut i) : SV_Target
{
    const float4 tex = gTex0.Sample(gTex0Samp, i.uv);

    // #7 additive emissive composite of the RTT atlas. It must NOT fall through
    // to tex*colour: the blend is additive, and lines drawn with zero alpha
    // would vanish while text survived. Scale by the constant quad alpha.
    if (pc.rttSoft > 0.5f)
        return float4(tex.rgb * i.color.rgb * 1.5f, i.color.a);

    return tex * i.color;
}
