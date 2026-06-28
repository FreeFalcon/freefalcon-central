//-----------------------------------------------------------------------------
// FFEmu.hlsl  -- fixed-function emulation shaders for the D3D7 -> D3D11 port.
//
// PHASE 2 (see project memory: render-port-direction).
//
// The legacy engine renders through two distinct vertex paths:
//
//   1. TLVERTEX (XYZRHW): vertices already transformed to SCREEN space on the
//      CPU (sx,sy in pixels, sz = depth 0..1, rhw = 1/w), with per-vertex color
//      pre-lit on the CPU. Used by 2D/HUD/terrain/particle paths. The shader
//      just converts screen->clip space and blends texture * vertex color.
//
//   2. Object/BSP path: untransformed model-space vertices fed through
//      SetTransform(WORLD/VIEW/PROJECTION) with optional fixed-function
//      lighting. The shader does world*view*proj and per-vertex lighting.
//
// VR / OpenXR note: cbView holds View and Projection as PER-PASS values (not a
// global). Stereo simply runs the object path twice with per-eye View/Proj.
// The XYZRHW path is screen-space and is for flat menus only (the 2D cockpit is
// being removed -- see openxr-feasibility memory).
//
// Pixel shading is shared: sample up to two textures, modulate, then apply
// chroma-key discard / alpha-test, then fog. Feature flags in cbRender select
// behaviour, replacing the legacy STATE_* / SetTextureStageState combinations.
//-----------------------------------------------------------------------------

//============================ Constant buffers ===============================

cbuffer cbViewport : register(b0)
{
    float2 gScreenSize;     // backbuffer width,height in pixels (for XYZRHW)
    float2 gViewportPad;
};

cbuffer cbView : register(b1)
{
    row_major float4x4 gView;   // per-pass (per-eye in VR)
    row_major float4x4 gProj;   // per-pass (per-eye in VR)
    float4 gCameraPos;          // world camera position (for specular: half-vector)
};

cbuffer cbObject : register(b2)
{
    row_major float4x4 gWorld;
};

// Feature flags -- replace the legacy STATE_* matrix.
#define FF_TEXTURE0     (1u << 0)   // sample texture stage 0
#define FF_TEXTURE1     (1u << 1)   // sample texture stage 1 (multitexture)
#define FF_VERTEXCOLOR  (1u << 2)   // modulate by per-vertex color (gouraud)
#define FF_LIGHTING     (1u << 3)   // per-vertex lighting (object path)
#define FF_CHROMAKEY    (1u << 4)   // discard texels matching chroma key
#define FF_ALPHATEST    (1u << 5)   // discard texels below gAlphaRef
#define FF_FOG          (1u << 6)   // apply fog
#define FF_MODULATE2X   (1u << 7)   // STATE_..._GOURAUD2 style 2x modulate
#define FF_TEXCOLORDIFFUSE (1u << 8) // D3D7 TexColorDiffuse: color=vertex, texture=mask (HUD/DED text)
#define FF_RTTSOFT      (1u << 9)   // #7 AA-RTT: soft composite of the RTT atlas (alpha=brightness=MSAA coverage)
#define FF_WATER        (1u << 10)  // #12: animated water tile (shimmer + tint over the base texture)
#define FF_EMISSIVE     (1u << 11)  // #49: self-illuminated surface (afterburner cone, nav/formation
                                    // lights) -- D3D7 SwEmissive. Skip scene-light darkening so it
                                    // glows at any time of day (dusk/night). Set per-surface in code.
#define FF_AFTERBURNER  (1u << 12)  // #49: afterburner cone (COMP_AB/COMP_AB2). Recolor to a warm
                                    // white-hot-core -> orange gradient (reference real_af.png),
                                    // independent of the model's vertex colors. Implies FF_EMISSIVE.

cbuffer cbRender : register(b3)
{
    uint   gFlags;
    float  gAlphaRef;       // 0..1 alpha-test threshold
    float  gFogStart;
    float  gFogEnd;

    float4 gFogColor;
    float4 gChromaKey;      // rgb to key out, a = tolerance
    float4 gMaterialColor;  // global modulate (default 1,1,1,1)
    float4 gSpecular;       // rgb = water glint color, w = power (0 -> no specular)
    float4 gWaterParams;    // #12: x = animation time (sec); y,z,w reserved
};

//============================ Lighting =======================================

#define MAX_LIGHTS 8

struct GpuLight
{
    float4 Position;    // xyz, w=1 for point / 0 for directional
    float4 Direction;   // xyz normalized
    float4 Color;       // rgb
    float4 Params;      // x=range, y=type(0=dir,1=point), z,w=atten
};

cbuffer cbLights : register(b4)
{
    float4   gAmbient;
    uint     gNumLights;
    float3   gLightPad;
    GpuLight gLights[MAX_LIGHTS];
};

//============================ Resources ======================================

Texture2D    gTex0 : register(t0);
Texture2D    gTex1 : register(t1);
SamplerState gSamp0 : register(s0);
SamplerState gSamp1 : register(s1);

//============================ IO structs =====================================

// XYZRHW input (matches TLVERTEX: screen pos + rhw, color, specular, 2x uv).
struct VSInScreen
{
    float4 PosRhw   : POSITION;   // x,y = pixels; z = depth; w = rhw (1/w)
    float4 Color    : COLOR0;
    float4 Specular : COLOR1;
    float2 Uv0      : TEXCOORD0;
    float2 Uv1      : TEXCOORD1;
};

// Object input: model-space position, normal, diffuse color, emissive color, uv.
// D3D7 object pass sets DIFFUSE/AMBIENT material source = COLOR1 (dwColour) and
// EMISSIVE material source = COLOR2 (dwSpecular), with global ambient = black.
// So the second vertex color (Emissive) is the self-illuminated base color that
// gives cockpit panels their gray tint independent of scene lighting.
struct VSInObject
{
    float3 Pos      : POSITION;
    float3 Normal   : NORMAL;
    float4 Color    : COLOR0;    // dwColour  -> diffuse + ambient material
    float4 Emissive : COLOR1;    // dwSpecular -> emissive material (D3DMCS_COLOR2)
    float2 Uv0      : TEXCOORD0;
};

struct VSOut
{
    float4 Pos    : SV_Position;
    float4 Color  : COLOR0;
    float2 Uv0    : TEXCOORD0;
    float2 Uv1    : TEXCOORD1;
    float  FogF   : TEXCOORD2;   // fog factor 1=clear .. 0=full fog
    float3 Spec   : TEXCOORD3;   // specular highlight (added in the PS on top of the texture)
};

//============================ Helpers ========================================

float ComputeFog(float viewDepth)
{
    // Linear fog, matching D3DFOG_LINEAR.
    float f = saturate((gFogEnd - viewDepth) / max(gFogEnd - gFogStart, 1e-4));
    return f;
}

//============================ Vertex shaders =================================

// Path 1: pre-transformed screen-space (XYZRHW emulation).
VSOut VS_Screen(VSInScreen i)
{
    VSOut o;

    // Convert pixel coords -> normalized device coords. D3D9-era XYZRHW assumed
    // a half-pixel offset; modern rasterization is top-left so we omit it.
    float2 ndc;
    ndc.x =  (i.PosRhw.x / gScreenSize.x) * 2.0f - 1.0f;
    ndc.y =  1.0f - (i.PosRhw.y / gScreenSize.y) * 2.0f;

    // rhw = 1/w. For PERSPECTIVE-CORRECT interpolation of attributes (UV) we must
    // recover the real w and return a clip position (ndc*w, w): the GPU divides by w
    // -> yields ndc, but UV interpolate with division by w (as needed). At w=1
    // (2D-UI, rhw=1) behavior is unchanged. Otherwise terrain textures 'swam' at the edges.
    float wv = (i.PosRhw.w != 0.0f) ? (1.0f / i.PosRhw.w) : 1.0f;
    o.Pos  = float4(ndc.x * wv, ndc.y * wv, i.PosRhw.z * wv, wv);
    o.Color = i.Color;
    o.Uv0  = i.Uv0;
    o.Uv1  = i.Uv1;
    // Terrain fog: fog by distance (wv = 1/rhw = view-distance) when FF_FOG.
    // 2D-UI/HUD without FF_FOG -> FogF=1 (no fog). Distant terrain dissolves into
    // haze (gFogColor), removing the 'steps' / 'too close'.
    o.FogF = (gFlags & FF_FOG) ? ComputeFog(wv) : 1.0f;
    o.Spec = float3(0, 0, 0);   // screen/2D path with no specular (otherwise the PS adds garbage)
    return o;
}

// Path 2: object/BSP -- full transform + optional per-vertex lighting.
VSOut VS_Object(VSInObject i)
{
    VSOut o;

    float4 worldPos = mul(float4(i.Pos, 1.0f), gWorld);
    float4 viewPos  = mul(worldPos, gView);
    o.Pos = mul(viewPos, gProj);

    // The D3D7 object path always has COLORVERTEX=TRUE (dxengine.cpp:2246) ->
    // the vertex color (dwColour) is applied ALWAYS, not by a flag. Otherwise the greenish-
    // gray cockpit-panel color was dropped -> col=white -> panels were blown out
    // to white by lighting (day=white, sunset=gray).
    float4 col = i.Color;
    o.Spec = float3(0, 0, 0);   // specular (filled when FF_LIGHTING + gSpecular.w>0)

    // #49 self-illuminated (D3D7 SwEmissive: afterburner cone, lights): use the EMISSIVE vertex
    // color (COLOR2 / dwSpecular -- the bright flame/light color in D3D7) and do NOT darken by
    // scene lighting. The diffuse (COLOR0) of the cone is dull, so taking COLOR0 left it unlit;
    // the D3D11 object path had dropped emissive entirely, so the plume was dark.
    if (gFlags & FF_AFTERBURNER)
    {
        col.rgb = float3(1.0f, 1.0f, 1.0f);   // neutral: PS maps texture brightness to a warm gradient
    }
    else if (gFlags & FF_EMISSIVE)
    {
        col.rgb = i.Emissive.rgb;   // self-illuminated flame/light color (keep diffuse alpha)
    }
    else if (gFlags & FF_LIGHTING)
    {
        // Per-vertex lighting: col = diffuse(dwColour) * saturate(ambient + lights).
        // NOTE: dwSpecular (COLOR2) is NOT added as emissive -- in cockpit models it is
        // near white (specular/fog encoding), and as emissive it blew the panels
        // white. In D3D7 this is a subtle specular, not self-illumination.
        float3 N = normalize(mul(i.Normal, (float3x3)gWorld));
        float3 lit = gAmbient.rgb;
        [loop] for (uint l = 0; l < gNumLights; ++l)
        {
            GpuLight L = gLights[l];
            float3 Ldir;
            float  atten = 1.0f;
            if (L.Params.y < 0.5f)              // directional (sun) -- no attenuation
                Ldir = -normalize(L.Direction.xyz);
            else                                // point (dynamic: muzzle flashes/explosions)
            {
                float3 toL = L.Position.xyz - worldPos.xyz;
                float  dist = length(toL);
                Ldir = toL / max(dist, 1e-3f);
                // linear range attenuation (Params.x=range) -> the lamp lights only
                // nearby, not the whole world. Otherwise every flash would light the entire scene.
                atten = saturate(1.0f - dist / max(L.Params.x, 1.0f));
            }
            lit += L.Color.rgb * max(dot(N, Ldir), 0.0f) * atten;
        }
        col.rgb *= saturate(lit);

        // Specular (Blinn-Phong, per-vertex) from the MAIN source (light 0 = sun/NVG).
        // gSpecular.rgb = highlight color (from the surface material), gSpecular.w = power.
        // Added in the PS ON TOP of the texture (like D3D7 SPECULAR), hence in o.Spec, not col.
        if (gSpecular.w > 0.0f && gNumLights > 0)
        {
            float3 V  = normalize(gCameraPos.xyz - worldPos.xyz);
            GpuLight L0 = gLights[0];
            float3 Ls = (L0.Params.y < 0.5f) ? -normalize(L0.Direction.xyz)
                                             : normalize(L0.Position.xyz - worldPos.xyz);
            float3 H  = normalize(Ls + V);
            float  s  = pow(max(dot(N, H), 0.0f), gSpecular.w);
            o.Spec = gSpecular.rgb * L0.Color.rgb * s;
        }
    }

    o.Color = col;
    o.Uv0 = i.Uv0;
    o.Uv1 = i.Uv0;
    // #29: fog distance = forward = clip.w (=view.x in this engine), NOT viewPos.z
    // (there it's the vertical/side axis). FF_FOG for objects is off for now (see BeginObjectPass).
    o.FogF = (gFlags & FF_FOG) ? ComputeFog(o.Pos.w) : 1.0f;
    return o;
}

//============================ Pixel shader ===================================

float4 PS_Main(VSOut i) : SV_Target
{
    float4 c = gMaterialColor;

    if (gFlags & FF_VERTEXCOLOR || true)
        c *= i.Color;       // vertex color always carried (1,1,1,1 if unused)

    float texA = 1.0f;          // texture alpha (chroma baked as a=0 at load time)

    if (gFlags & FF_TEXTURE0)
    {
        float4 t0 = gTex0.Sample(gSamp0, i.Uv0);
        texA = t0.a;

        if (gFlags & FF_TEXCOLORDIFFUSE)
        {
            // HUD/DED text (D3D7 TexColorDiffuse): the glyph is in the ALPHA (background baked to alpha=0,
            // but its RGB != black -> chroma-on-black won't cut the background -> a block). Cut by
            // ALPHA (like the working MFD via ALPHATEST). Color comes from the vertex (no c*=t0).
            if (texA < 0.5f)
                discard;
        }
        else if (gFlags & FF_RTTSOFT)
        {
            // #7 ADDITIVE EMISSIVE COMPOSITE of the RTT atlas (HUD/MFD/DED/RWR). A real HUD/BMS is
            // emissive glass: symbology is ADDED to the background, not chroma-cut. The black
            // atlas background adds 0 (no chroma needed), a line's AA gradient adds SMOOTHLY ->
            // no threshold/hard cut -> no 'rim'/bulk on lines or text.
            // BLEND_ADDITIVE (SRC_ALPHA,ONE): result = c.rgb*c.a + dst. c.rgb=atlas*tint, c.a=rttAlpha.
            // Brightness boost: chroma used to keep a dark-green border = visible THICKNESS; additive
            // removed it -> lines/text look 'thin'. The boost brightens the center + makes AA edges more visible = fatter,
            // but without a hard cut. Tune freely (runtime shader): 1.0 neutral, 2.0 ~as before.
            c.rgb *= t0.rgb * 1.5f;
            c.a    = i.Color.a;
        }
        else
        {
            // RGB chroma-key (background = chroma color) for non-text textures.
            if (gFlags & FF_CHROMAKEY)
            {
                float3 d = abs(t0.rgb - gChromaKey.rgb);
                if (max(max(d.r, d.g), d.b) <= gChromaKey.a)
                    discard;
            }
            c *= t0;
        }

        // Terrain multitexture (day/night): D3D7 STATE_MULTITEXTURE stage1 =
        // CURRENT ADD TEXTURE -> (day*diffuse) + night. Previously a multiply
        // (c *= night) -> day * dark_night = BLACK ground. Now an add.
        if (gFlags & FF_TEXTURE1)
            c.rgb += gTex1.Sample(gSamp1, i.Uv1).rgb;

        if (gFlags & FF_MODULATE2X)
            c.rgb *= 2.0f;
    }

    // #49 afterburner warm recolor (reference real_af.png): map the plume texture brightness to a
    // white-hot core -> orange/red body gradient. c.rgb is texture*white here (VS set col=white),
    // so its luminance tracks the texture's own density. Keeps c.a for the soft translucent edge.
    if (gFlags & FF_AFTERBURNER)
    {
        // Rendered ADDITIVE (Src=ONE,Dst=ONE): scale the color BY the texture brightness so dim
        // texels add little (soft edges) and the dense core adds a lot -> saturates to white.
        float b    = max(c.r, max(c.g, c.b));        // texture brightness (c = texture * white)
        float3 cool = float3(1.0f, 0.30f, 0.06f);    // orange/red outer plume
        float3 hot  = float3(1.0f, 0.95f, 0.80f);    // white-hot core near the nozzle
        float3 tint = lerp(cool, hot, saturate(b * 1.4f));

        // Day/night intensity: in daylight a real AB plume is nearly invisible (just nozzle flame +
        // heat haze); the bright glowing plume is a dusk/night thing. Scale by scene darkness
        // (gAmbient): subtle by day, bright at night. Tune the two endpoints freely (runtime shader).
        float amb     = saturate(max(gAmbient.r, max(gAmbient.g, gAmbient.b)));
        float night   = 1.0f - amb;                  // 0 = bright day .. 1 = night
        float intensity = lerp(0.45f, 2.5f, night);  // day endpoint .. night endpoint

        c.rgb = tint * b * intensity;
    }

    if (gFlags & FF_ALPHATEST)
    {
        // Object path: key on TEXTURE alpha (chroma is baked as a=0 in the
        // palette/RGBA loader), so per-vertex alpha cannot spuriously discard
        // opaque geometry. Screen/2D path (untextured) falls back to final alpha.
        float aTest = (gFlags & FF_TEXTURE0) ? texA : c.a;
        if (aTest < gAlphaRef)
            discard;
    }

    // The specular highlight is added ON TOP of the texture (like D3D7 specular), before fog.
    c.rgb += i.Spec;

    // #12: animated water. The terrain is drawn through the screen path (no world-space
    // normal/position), so this is a UV+time effect: a cool tint over the base water tile
    // plus interfering sine fields raised to a high power for sun-glitter sparkles. Tunable.
    if (gFlags & FF_WATER)
    {
        // Soft moving sheen, NOT sharp sparkles. The terrain UV repeats per tile (no continuous
        // world coord in the screen path), so a high-frequency / high-contrast pattern reads as
        // a regular dotted grid. Keep it low-frequency and low-amplitude -> gentle water sheen.
        float t  = gWaterParams.x;
        float2 p = i.Uv0 * 7.0f;
        float w  = sin(p.x + t * 0.6f)
                 + sin(p.y * 1.3f - t * 0.5f)
                 + sin((p.x + p.y) * 0.8f + t * 0.9f) * 0.5f;   // ~[-2.5..2.5]
        c.rgb *= float3(0.80f, 0.92f, 1.06f);              // blue-green tint
        c.rgb += w * 0.025f * float3(0.6f, 0.7f, 0.85f);   // subtle sheen ripple
    }

    if (gFlags & FF_FOG)
        c.rgb = lerp(gFogColor.rgb, c.rgb, i.FogF);

    return c;
}

//============================ Per-sample SSAA (display panels) ================
// #7: HUD/MFD/DED text 'swims' as the view moves -- the RTT atlas is projected onto the 3D glass and
// sampled once/pixel (sub-texel shift = shimmer). Here the PS runs per MSAA SAMPLE
// (SV_SampleIndex), and attributes with the `sample` modifier interpolate at the SAMPLE position -> UV
// differs per each of the 4 samples -> the RTT texture is sampled 4x -> MSAA resolve averages = 4x SSAA
// specifically on the displays (at normal size). Used only for DrawRttQuad (see the renderer).
struct VSOutSample
{
    float4        Pos   : SV_Position;
    sample float4 Color : COLOR0;
    sample float2 Uv0   : TEXCOORD0;
    sample float2 Uv1   : TEXCOORD1;
    sample float  FogF  : TEXCOORD2;
    sample float3 Spec  : TEXCOORD3;
};

float4 PS_PerSample(VSOutSample i, uint sIdx : SV_SampleIndex) : SV_Target
{
    VSOut o;
    o.Pos = i.Pos; o.Color = i.Color; o.Uv0 = i.Uv0; o.Uv1 = i.Uv1; o.FogF = i.FogF; o.Spec = i.Spec;
    return PS_Main(o);
}
