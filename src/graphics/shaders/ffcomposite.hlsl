// ffcomposite.hlsl -- Artscout - 2026: UI-over-3D composite, HLSL.
// No vertex buffer: SV_VertexID 0..2 makes one oversized triangle. The pipeline
// blend lays the UI (alpha 0 where black) over the already-blitted scene.

[[vk::combinedImageSampler]][[vk::binding(0, 0)]] Texture2D gUi;
[[vk::combinedImageSampler]][[vk::binding(0, 0)]] SamplerState gUiSamp;

struct VSOut
{
    float4 pos : SV_Position;
    [[vk::location(0)]] float2 uv : TEXCOORD0;
};

VSOut VS_Composite(uint vid : SV_VertexID)
{
    VSOut o;
    // 0 -> (-1,-1), 1 -> (3,-1), 2 -> (-1,3): covers the whole viewport.
    const float2 uv = float2((vid << 1) & 2, vid & 2);
    o.uv = uv;
    o.pos = float4(uv * 2.0f - 1.0f, 0.0f, 1.0f);
    return o;
}

float4 PS_Composite(VSOut i) : SV_Target
{
    // .a carries the black-key: 0 shows the 3D behind.
    return gUi.Sample(gUiSamp, i.uv);
}
