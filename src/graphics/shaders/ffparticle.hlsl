// ffparticle.hlsl -- Artscout - 2026: instanced particle billboards, HLSL.
// Spherical camera-facing quad with world-up constrained to NED (-Z), matching
// the legacy roll-free billboard matrix. Ported from the GLSL pair.
#include "ffcommon.hlsli"

[[vk::combinedImageSampler]][[vk::binding(1, 0)]] Texture2D gAtlas;
[[vk::combinedImageSampler]][[vk::binding(1, 0)]] SamplerState gAtlasSamp;

struct VSIn
{
    [[vk::location(0)]] float2 corner : POSITION;  // -0.5..0.5 unit quad (slot 0)
    [[vk::location(1)]] float2 quadUv : TEXCOORD0; // 0..1
    [[vk::location(2)]] float3 center : TEXCOORD1; // camera-relative world (slot 1)
    [[vk::location(3)]] float2 size   : TEXCOORD2; // world width, height
    [[vk::location(4)]] float  rot    : TEXCOORD3; // billboard-plane rotation (rad)
    [[vk::location(5)]] float4 color  : COLOR0;    // D3DCOLOR ARGB -> rgba
    [[vk::location(6)]] float4 uvRect : TEXCOORD4; // xy = atlas offset, zw = scale
};

struct VSOut
{
    float4 pos : SV_Position;
    [[vk::location(0)]] float4 color : COLOR0;
    [[vk::location(1)]] float2 uv    : TEXCOORD0;
};

VSOut VS_Particle(VSIn i, uint viewId : SV_ViewID)
{
    VSOut o;

    const float3 fwd = normalize(-i.center);
    const float3 wup = float3(0.0f, 0.0f, -1.0f);
    float3 right = cross(wup, fwd);
    const float rl = length(right);
    right = (rl > 1e-4f) ? (right / rl) : float3(1.0f, 0.0f, 0.0f);
    const float3 up = cross(fwd, right);

    const float s = sin(i.rot), c = cos(i.rot);
    const float2 rc = float2(i.corner.x * c - i.corner.y * s,
                             i.corner.x * s + i.corner.y * c);
    const float2 off = rc * i.size;
    const float3 worldPos = i.center + right * off.x + up * off.y;
    o.pos = mul(mul(float4(worldPos, 1.0f), gView[viewId]), gProj[viewId]);

    // Vertical gradient fakes self-shadow, so a cloud of sprites reads as volume.
    const float grad = lerp(0.68f, 1.0f, i.quadUv.y);
    o.color = float4(i.color.rgb * grad, i.color.a);
    o.uv = i.uvRect.xy + i.quadUv * i.uvRect.zw; // atlas / flipbook cell
    return o;
}

float4 PS_Particle(VSOut i) : SV_Target
{
    // Blend mode (alpha vs additive) is the pipeline's job.
    return gAtlas.Sample(gAtlasSamp, i.uv) * i.color;
}
