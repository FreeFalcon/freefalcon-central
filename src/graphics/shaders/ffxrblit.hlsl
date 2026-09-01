// ffxrblit.hlsl -- Artscout - 2026: scene -> OpenXR swapchain image.
// The XR image is sRGB and encodes on write, while the scene holds already
// encoded bytes; decoding here cancels that second encode (a blit cannot).

[[vk::combinedImageSampler]][[vk::binding(0, 0)]] Texture2D gSrc;
[[vk::combinedImageSampler]][[vk::binding(0, 0)]] SamplerState gSrcSamp;

// The eye renders into the top-left sub-rect of a reused, max-sized target.
struct XrBlitPush
{
    float2 uvScale;
    float2 pad;
};

[[vk::push_constant]] XrBlitPush gPush;

struct VSOut
{
    float4 pos : SV_Position;
    [[vk::location(0)]] float2 uv : TEXCOORD0;
};

VSOut VS_XrBlit(uint vid : SV_VertexID)
{
    VSOut o;
    // 0 -> (-1,-1), 1 -> (3,-1), 2 -> (-1,3): covers the whole viewport.
    const float2 uv = float2((vid << 1) & 2, vid & 2);
    o.uv = uv;
    o.pos = float4(uv * 2.0f - 1.0f, 0.0f, 1.0f);
    return o;
}

float3 SrgbToLinear(float3 s)
{
    return select(s <= 0.04045f, s / 12.92f,
                  pow((s + 0.055f) / 1.055f, 2.4f));
}

float4 PS_XrBlit(VSOut i) : SV_Target
{
    const float4 c = gSrc.Sample(gSrcSamp, i.uv * gPush.uvScale);
    return float4(SrgbToLinear(c.rgb), c.a);
}
