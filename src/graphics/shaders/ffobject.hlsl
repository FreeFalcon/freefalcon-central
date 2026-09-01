// ffobject.hlsl -- Artscout - 2026: object/terrain pass, HLSL for both backends.
// Ported from the GLSL pair it replaces: bindings, UBO layout and behaviour are
// unchanged. Lighting is PER-VERTEX (legacy D3D7 gouraud), as the PS expects.
#include "ffcommon.hlsli"
//============================== Resources ====================================

// Paired image+sampler on one binding: DXC folds them into a combined
// descriptor, which is what the existing sets already hold.
[[vk::combinedImageSampler]][[vk::binding(1, 0)]] Texture2D gTex0;
[[vk::combinedImageSampler]][[vk::binding(1, 0)]] SamplerState gTex0Samp;
[[vk::combinedImageSampler]][[vk::binding(2, 0)]] Texture2D gTex1;
[[vk::combinedImageSampler]][[vk::binding(2, 0)]] SamplerState gTex1Samp;

// #107 bindless terrain: the tile atlas as one unbounded array. An array cannot
// be a combined descriptor, so image and sampler are separate here.
[[vk::binding(0, 1)]] Texture2D gBindless[];
[[vk::binding(3, 0)]] SamplerState gBindlessSamp;

//============================ Stage plumbing =================================

struct VSIn
{
    [[vk::location(0)]] float3 pos      : POSITION;
    [[vk::location(1)]] float3 normal   : NORMAL;
    [[vk::location(2)]] float4 color    : COLOR0;    // diffuse + ambient material
    [[vk::location(3)]] float4 emissive : COLOR1;    // EMISSIVE material, not a highlight
    [[vk::location(4)]] float2 uv       : TEXCOORD0;
    [[vk::location(5)]] uint   texIndex : TEXCOORD1; // bindless tile slot
};

struct VSOut
{
    float4 pos : SV_Position;
    [[vk::location(0)]] float4 color : COLOR0;
    [[vk::location(1)]] float2 uv    : TEXCOORD0;
    [[vk::location(2)]] float2 uv1   : TEXCOORD1;
    [[vk::location(3)]] float3 spec  : TEXCOORD2;
    [[vk::location(4)]] float  fogF  : TEXCOORD3;
    [[vk::location(5)]] nointerpolation uint texIndex : TEXCOORD4;
};

// The VS writes one field more than the PS reads: PointSize is a builtin, takes
// no location, and must be written or a POINT_LIST pipeline is invalid.
struct VSOutVs
{
    float4 pos : SV_Position;
    [[vk::builtin("PointSize")]] float psize : PSIZE;
    [[vk::location(0)]] float4 color : COLOR0;
    [[vk::location(1)]] float2 uv    : TEXCOORD0;
    [[vk::location(2)]] float2 uv1   : TEXCOORD1;
    [[vk::location(3)]] float3 spec  : TEXCOORD2;
    [[vk::location(4)]] float  fogF  : TEXCOORD3;
    [[vk::location(5)]] nointerpolation uint texIndex : TEXCOORD4;
};

//============================== Vertex =======================================

// PointSize is required for a POINT_LIST pipeline (BSP nav-lights): without it
// the driver rejects the pipeline and the device is lost on the first such frame.
VSOutVs VS_Object(VSIn i, uint viewId : SV_ViewID)
{
    VSOutVs o;
    o.psize = 1.0f;

    const float4 wp = mul(float4(i.pos, 1.0f), gWorld);
    o.pos = mul(mul(wp, gView[viewId]), gProj[viewId]);

    // COLORVERTEX is always on in the D3D7 object path, so the vertex colour
    // applies unconditionally -- dropping it blows the cockpit panels white.
    float4 col = i.color;
    float3 spec = float3(0.0f, 0.0f, 0.0f);

    if (Has(FF_AFTERBURNER))
    {
        col.rgb = float3(1.0f, 1.0f, 1.0f); // the PS maps brightness to a gradient
    }
    else if (Has(FF_EMISSIVE))
    {
        col.rgb = i.emissive.rgb; // self-lit flame/light colour, diffuse alpha kept
    }
    else if (Has(FF_LIGHTING))
    {
        const float3 N = normalize(mul(i.normal, (float3x3)gWorld));

        // #72: build cockpit depth by BRIGHTENING sun-lit faces, never by
        // cutting ambient (uniform darkening just muddies an already dark pit).
        float ambScale = 1.0f;
        const float sunScale = Has(FF_COCKPIT) ? 1.25f : 1.0f;
        if (Has(FF_COCKPIT))
        {
            // #97: dim ambient by the SUN level so the pit goes dark after dusk.
            const float sunLvl =
                (gNumLights.x > 0u) ?
                    dot(gLights[0].color.rgb, float3(0.299f, 0.587f, 0.114f)) :
                    1.0f;
            ambScale = clamp(sunLvl, 0.06f, 1.0f);
        }

        float3 lit = gAmbient.rgb * ambScale;
        for (uint l = 0u; l < gNumLights.x && l < 8u; ++l)
        {
            float3 Ldir;
            float atten = 1.0f;
            if (gLights[l].params.y < 0.5f) // directional (sun)
            {
                Ldir = -normalize(gLights[l].direction.xyz);
            }
            else // point (muzzle flashes / explosions)
            {
                const float3 toL = gLights[l].position.xyz - wp.xyz;
                const float dist = length(toL);
                Ldir = toL / max(dist, 1e-3f);
                // Linear range falloff: a lamp lights its neighbourhood only.
                atten = saturate(1.0f - dist / max(gLights[l].params.x, 1.0f));
            }
            // #72: boost only the sun -- a muzzle flash must not blow out the pit.
            const float lScale = (gLights[l].params.y < 0.5f) ? sunScale : 1.0f;
            lit += gLights[l].color.rgb * max(dot(N, Ldir), 0.0f) * atten * lScale;
        }
        if (Has(FF_FULLBRIGHT))
            lit = float3(1.0f, 1.0f, 1.0f); // #97 unlit (exit menu)
        col.rgb *= saturate(lit);

        // Blinn-Phong from light 0, added in the PS on top of the texture.
        float specPow = gSpec.w;
        float3 specCol = gSpec.rgb;
        if (Has(FF_COCKPIT) && specPow <= 0.0f)
        {
            specPow = 20.0f;
            specCol = float3(0.10f, 0.10f, 0.10f);
        }
        if (specPow > 0.0f && gNumLights.x > 0u)
        {
            const float3 V = normalize(gCamPos.xyz - wp.xyz);
            const float3 Ls =
                (gLights[0].params.y < 0.5f) ?
                    -normalize(gLights[0].direction.xyz) :
                    normalize(gLights[0].position.xyz - wp.xyz);
            const float3 H = normalize(Ls + V);
            spec = specCol * gLights[0].color.rgb *
                   pow(max(dot(N, H), 0.0f), specPow);
        }
    }

    o.color = col;
    o.uv = i.uv;
    o.uv1 = i.uv; // stage 1 shares coordinates (terrain day/night)
    o.spec = spec;
    o.texIndex = i.texIndex;
    // Fog distance is clip.w: this engine's view forward axis is X, not view.z.
    o.fogF = Has(FF_FOG) ?
                 saturate((gFogParams.y - o.pos.w) /
                          max(gFogParams.y - gFogParams.x, 1e-4f)) :
                 1.0f;
    return o;
}

//============================== Pixel ========================================

float4 PS_Object(VSOut i) : SV_Target
{
    // FF_GLOC: G-force vignette on a fullscreen quad -- before any texture work.
    if (Has(FF_GLOC))
    {
        const float r = length(i.uv - float2(0.5f, 0.5f)) * 2.0f;
        const float v = smoothstep(gGloc.y, gGloc.z, r) * gGloc.x;
        return float4(gMaterialColor.rgb, saturate(v));
    }

    float4 c = gMaterialColor * i.color;
    float texA = 1.0f; // chroma is baked to a=0 at load time

    if (Has(FF_TEXTURE0))
    {
        // FF_BINDLESS routes the base sample through the array by per-vertex
        // slot; ~0 means "not resident" and falls back to gTex0 (white here).
        const bool bindless = Has(FF_BINDLESS) && i.texIndex != 0xFFFFFFFFu;
        const float4 t0 =
            bindless ?
                gBindless[NonUniformResourceIndex(i.texIndex)].Sample(
                    gBindlessSamp, i.uv) :
                gTex0.Sample(gTex0Samp, i.uv);
        texA = t0.a;

        if (Has(FF_TEXCOLORDIFFUSE))
        {
            // D3D7 TexColorDiffuse (HUD/DED text): the glyph is in the ALPHA and
            // its RGB is not black, so cut by alpha and keep the vertex colour.
            if (texA < 0.5f)
                discard;
        }
        else if (Has(FF_RTTSOFT))
        {
            // #7 additive emissive composite of the RTT atlas: symbology is
            // ADDED, so no chroma cut and no rim on the text.
            c.rgb *= t0.rgb * 1.5f;
            c.a = i.color.a;
        }
        else
        {
            if (Has(FF_CHROMAKEY))
            {
                const float3 d = abs(t0.rgb - gChromaKey.rgb);
                if (max(max(d.r, d.g), d.b) <= gChromaKey.a)
                    discard;
            }
            c *= t0;
        }

        // Terrain day/night: the D3D7 stage 1 op is CURRENT ADD TEXTURE -- it
        // ADDS. A multiply here is what used to give a black ground.
        if (Has(FF_TEXTURE1))
            c.rgb += gTex1.Sample(gTex1Samp, i.uv1).rgb;

        if (Has(FF_MODULATE2X))
            c.rgb *= 2.0f;
    }

    // #49 afterburner: scale colour BY texture brightness, with time-animated
    // turbulence and flicker so the plume licks instead of sitting frozen.
    if (Has(FF_AFTERBURNER))
    {
        const float tAB = gFogParams.w;
        const float2 uvAB = i.uv;
        float turb = 0.5f
                   + 0.30f * sin(uvAB.y * 15.0f - tAB * 11.0f + uvAB.x * 6.0f)
                   + 0.16f * sin(uvAB.y * 29.0f - tAB * 19.0f - uvAB.x * 10.0f + 1.7f)
                   + 0.08f * sin(uvAB.x * 22.0f + tAB * 7.0f);
        turb = saturate(turb);

        const float b = max(c.r, max(c.g, c.b));
        const float bMod = b * lerp(0.60f, 1.30f, turb);

        const float3 cool = float3(1.0f, 0.30f, 0.06f);
        const float3 hot = float3(1.0f, 0.95f, 0.82f);
        const float3 core = float3(0.70f, 0.82f, 1.00f);
        float3 tint = lerp(cool, hot, saturate(bMod * 1.4f));
        tint = lerp(tint, core, saturate((bMod - 0.85f) * 3.0f) * 0.45f);

        // In daylight a real plume is nearly invisible -- scale by darkness.
        const float amb = saturate(max(gAmbient.r, max(gAmbient.g, gAmbient.b)));
        const float intensity = lerp(0.45f, 2.6f, 1.0f - amb);
        const float flick = 0.85f + 0.15f * sin(tAB * 42.0f) * (0.6f + 0.4f * turb);
        c.rgb = tint * bMod * intensity * flick;
    }

    if (Has(FF_ALPHATEST))
    {
        // Key on TEXTURE alpha so per-vertex alpha cannot discard opaque geometry.
        const float aTest = Has(FF_TEXTURE0) ? texA : c.a;
        if (aTest < gFogParams.z)
            discard;
    }

    c.rgb += i.spec; // D3D7-style specular, on top of the texture, before fog

    // #12 water: a UV+time effect (the terrain has no world normal here).
    if (Has(FF_WATER))
    {
        const float t = gFogParams.w;
        const float2 p = i.uv * 7.0f;
        const float w = sin(p.x + t * 0.6f)
                      + sin(p.y * 1.3f - t * 0.5f)
                      + sin((p.x + p.y) * 0.8f + t * 0.9f) * 0.5f;
        c.rgb *= float3(0.80f, 0.92f, 1.06f);
        c.rgb += w * 0.025f * float3(0.6f, 0.7f, 0.85f);
    }

    if (Has(FF_FOG))
        c.rgb = lerp(gFogColor.rgb, c.rgb, i.fogF);

    // #72 cockpit: brighten gently and let the clamp take the top end.
    if (Has(FF_COCKPIT))
        c.rgb = saturate(c.rgb * 1.08f);

    // #A5 sensor pass (TGP/Maverick/FLIR) -> Rec.601 luma.
    if (Has(FF_IRGREY))
        c.rgb = dot(c.rgb, float3(0.299f, 0.587f, 0.114f)).xxx;

    // #97 NVG: green phosphor with tube gain, scanlines, grain and vignette.
    if (Has(FF_NVG))
    {
        const float kNvgGain = 4.0f;
        float lum = dot(c.rgb, float3(0.30f, 0.59f, 0.11f));
        lum = 1.0f - exp(-lum * kNvgGain);

        const float scan = 0.92f + 0.08f * sin(i.pos.y * 3.14159f);
        const float2 np = i.pos.xy + gFogParams.w * 37.0f;
        const float grain =
            frac(sin(dot(np, float2(12.9898f, 78.233f))) * 43758.5453f);
        const float noise = 0.91f + 0.09f * grain;
        const float2 vc =
            i.pos.xy / max(gParams.yz, float2(1.0f, 1.0f)) - 0.5f;
        const float vig = saturate(1.0f - dot(vc, vc) * 1.35f);

        lum *= scan * noise * vig;
        c.rgb = float3(0.10f, 1.0f, 0.28f) * lum;
    }

    return c;
}
