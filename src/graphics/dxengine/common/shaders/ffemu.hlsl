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

// Artscout - 2026: #DX12 п.5 view-instanced single-pass stereo. When g_bVrViewInstancing is on
// (D3D12, STEREO, ViewInstancingTier supported) the whole GPU-matProj scene (terrain/objects/
// cockpit BSP/particles) is drawn ONCE and the rasterizer replicates each primitive to the two
// eye RT-array slices via SV_ViewID. The VI vertex shaders (VS_ObjectVI/VS_ParticleVI) index the
// per-eye View/Proj here by SV_ViewID; the flat/per-eye path never binds this buffer (b5) and uses
// cbView(b1) as before. gCamPos2 is the shared head-centre camera position (specular half-vector).
cbuffer cbViewStereo : register(b5)
{
    row_major float4x4 gView2[4];   // per-VIEW view matrix (index by SV_ViewID). 2 = stereo, 4 = quad-views (Varjo:
    row_major float4x4 gProj2[4];   // 0/1 periphery, 2/3 focus). Each quad view has its OWN off-axis projection.
    float4             gCamPos2;    // shared world camera position (head centre)
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
#define FF_IRGREY       (1u << 14)  // #DX12 A5: sensor video (TGP=TV, Maverick/FLIR=IR) is monochrome grey.
                                    // Desaturate the FINAL colour to luma for the whole sensor pass (set by
                                    // SetIRGrey while IR/TV mode). Needed because the terrain's vertex colours
                                    // are cached from the main full-colour view -> the CPU green-out never grays them.
#define FF_EMISSIVE     (1u << 11)  // #49: self-illuminated surface (afterburner cone, nav/formation
                                    // lights) -- D3D7 SwEmissive. Skip scene-light darkening so it
                                    // glows at any time of day (dusk/night). Set per-surface in code.
#define FF_AFTERBURNER  (1u << 12)  // #49: afterburner cone (COMP_AB/COMP_AB2). Recolor to a warm
                                    // white-hot-core -> orange gradient (reference real_af.png),
                                    // independent of the model's vertex colors. Implies FF_EMISSIVE.
#define FF_COCKPIT      (1u << 13)  // Artscout - 2026: #72 cockpit-fidelity pass (set by SetCockpitPass).
                                    // The flat per-vertex cockpit looked "cartoonish" (ambient floods
                                    // every face equally -> no gradient, no crevice shade, no glints).
                                    // For cockpit surfaces only: dampen the ambient FLOOR so the sun
                                    // N.L gradient reads, add a subtle default specular (head-move
                                    // glints on knobs/glass in VR), and a mild contrast in the PS.
                                    // World geometry / flat path never set this bit -> untouched.
#define FF_CLOUD        (1u << 18)  // Artscout - 2026: #13 volumetric cloud layer -- raymarch the slab described
                                    // by gCloud0..3 along the view ray. Set by BeginCloudPass. The geometry is a
                                    // camera-centred sphere drawn BACK-FACES-ONLY (one march per pixel, in every
                                    // direction, from anywhere); occlusion by the world comes from SAMPLING the
                                    // scene depth (t2/gSceneDepth), NOT from the depth test -- a single depth
                                    // comparison per pixel cannot describe a volume the camera is inside.
#define FF_NVG          (1u << 16)  // Artscout - 2026: #97 night-vision goggles -- recolor the fully-composed world
                                    // colour to green phosphor + tube gain. Set on the WORLD passes (terrain/objects/
                                    // cockpit/sky) when NVG is on, so everything seen through the goggles goes green.
#define FF_FULLBRIGHT   (1u << 17)  // Artscout - 2026: #97 unlit -- force lit=1 (full material colour, no TOD darkening).
#define FF_GLOC         (1u << 15)  // Artscout - 2026: G-force / end-flight vignette (blackout/redout).
                                    // Fullscreen post-process quad (uv 0..1): the PS ignores the texture
                                    // and returns the tint (gMaterialColor.rgb) with alpha = radial vignette
                                    // computed from gGloc (x=intensity, y=inner radius, z=outer radius).
                                    // Replaces the legacy screen-space tunnel-ring -> works on D3D11/D3D12
                                    // and, drawn per-eye in the VR loop, lands in each HMD view.

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
    float4 gGloc;           // Artscout - 2026: FF_GLOC vignette -- x=intensity(0=off), y=inner radius,
                            // z=outer radius (normalized: 0=centre .. 1=screen edge), w reserved
    // Artscout - 2026: #13 volumetric clouds (FF_CLOUD). Units are Falcon FEET with z DOWN (altitude 10000 ft
    // is z = -10000), matching realWeather->stratusZ.
    // The slab bounds are CAMERA-RELATIVE (the CPU subtracts the camera z). That is deliberate: pass-1 geometry
    // is camera-relative and the object view matrix is rotation-only, so the camera IS the origin of WPos and the
    // march needs no camera position at all. It also sidesteps a real trap -- the flat path's camera lives in
    // gCameraPos (cbView b1) but the view-instanced path's lives in gCamPos2 (cbViewStereo b5), and the PS cannot
    // tell which is bound. World anchoring for the noise is folded into gCloud1/gCloud3 by the CPU instead.
    float4 gCloud0;         // x = zTop (layer top, MORE negative), y = zBot (layer bottom) -- both camera-relative;
                            // z = coverage 0..1, w = density (extinction per foot)
    float4 gCloud1;         // xy = noise anchor = camera.xy + wind scroll (feet), z = noise scale (1/feet),
                            // w = march step count
    float4 gCloud2;         // xyz = sun direction (world, normalized, points TOWARD the sun), w = ambient blend
    float4 gCloudSun;       // rgb = sun/moon light colour reaching the layer, a = powder/silver-lining strength
    float4 gCloud3;         // x = camera world z (absolute, feet) -- anchors the noise vertically so the deck's
                            // structure does not swim as the aircraft climbs;
                            // y = profile: 0 = stratus deck, 1 = cumulus;
                            // z,w = depth linearization (A,B): forward = B / (sampledDepth - A). See gCloudFwd.
    float4 gCloudFwd;       // xyz = the view's FORWARD axis in camera-relative world space; w = 1 when t2 holds a
                            // usable scene depth (0 -> no occlusion clamp; flat MSAA, see SceneDepthSrvCpu).
    // Artscout - 2026: #13 tuning. x = erosion strength (0 = off), z = DEBUG view (0 = normal cloud;
    // 1 = RGB(density, envelope, noise) in one shot, 2/3/4 = envelope / noise / height fraction in grey),
    // w = vertical noise scale. y is FREE (it was a diagnostic step cap, dead since the step formula was fixed).
    float4 gCloudDiag;
    // Artscout - 2026: #13 weather map -- x = its frequency relative to the cloud noise (small = broader
    // patches), y = fraction of sky carrying weather, z = how much cloud TOP HEIGHT varies between clouds
    // (0 = every cloud reaches the slab ceiling, i.e. a flat lid; 0.6 = tops differ a lot).
    float4 gCloudDiag2;
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
// Artscout - 2026: #13 -- the SCENE DEPTH, so the cloud raymarch can stop each ray where the world does.
// ONE view type covers every case: both VR paths are single-sample (m_curSampleCount = 1), so VR depth is a
// plain 2-or-4 slice array, and a non-array depth is viewable as an array of 1. Flat MSAA has no array view --
// there gCloudFwd.w is 0 and the clamp is skipped. Bound for every draw (dummy when unused); only FF_CLOUD reads it.
Texture2DArray<float> gSceneDepth : register(t2);
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
    // Artscout - 2026: #13 -- world position of the fragment RELATIVE TO THE CAMERA (pass-1 geometry is built
    // camera-relative and the object view matrix is rotation-only; see TerrainGpu.cpp). FF_CLOUD needs the view
    // ray per pixel, and normalize(WPos) IS that ray because the camera sits at this space's origin. Filled by
    // ObjectVSCore, so VS_Object and VS_ObjectVI both carry it; VS_Screen writes 0 (no cloud in the 2D path).
    float3 WPos   : TEXCOORD4;
    // #13: which view this replica is, for indexing the scene-depth array slice in the PS. nointerpolation --
    // it is a per-draw/per-view constant, not something to blend across the triangle.
    nointerpolation uint ViewId : TEXCOORD5;
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
    o.WPos = float3(0, 0, 0);   // #13: no world position in the 2D path (FF_CLOUD never runs here)
    o.ViewId = 0;
    return o;
}

// Path 2: object/BSP -- full transform + optional per-vertex lighting.
// Artscout - 2026: #DX12 п.5 -- the transform inputs (view/proj/camera) are parameters so the SAME
// body serves the flat/per-eye entry (VS_Object) and the view-instanced stereo entry (VS_ObjectVI,
// which picks gView2[SV_ViewID]/gProj2[SV_ViewID]). Pure refactor: VS_Object behaviour is unchanged.
VSOut ObjectVSCore(VSInObject i, float4x4 wmat, float4x4 vmat, float4x4 pmat, float3 camWorld)
{
    VSOut o;

    float4 worldPos = mul(float4(i.Pos, 1.0f), wmat);
    float4 viewPos  = mul(worldPos, vmat);
    o.Pos = mul(viewPos, pmat);
    o.WPos = worldPos.xyz;   // #13 camera-relative world pos -> the PS reconstructs the view ray for FF_CLOUD
    o.ViewId = 0;            // #13 overridden by VS_ObjectVI; the flat path renders one view into slice 0

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
        float3 N = normalize(mul(i.Normal, (float3x3)wmat));

        // #72 cockpit: the flat "cartoonish" look came from ambient lighting EVERY face to near-full
        // brightness (no crevice shade, no gradient). Dampen the ambient FLOOR and boost the sun term
        // for cockpit surfaces so the N.L gradient reads -- shadowed faces go darker, lit faces stay
        // bright -> depth. World geometry keeps 1.0/1.0 (unchanged). Tune freely (runtime shader).
        // #72 user feedback: cutting ambient made the (already dark) F-16 pit too dark -- uniform
        // darkening muddies it (true crevice depth needs SSAO). So DON'T cut ambient (1.0); build depth
        // ONLY by brightening the sun-lit faces (a gradient by adding light, never removing it).
        float  ambScale = 1.0f;
        float  sunScale = (gFlags & FF_COCKPIT) ? 1.25f : 1.0f;
        // Artscout - 2026: #97 NIGHT cockpit. The pit was full-bright at night (ambient floor didn't drop). For
        // cockpit surfaces, dim the AMBIENT by the SUN LEVEL (light 0 = sun -> dark at night) so the pit goes dark
        // after dusk, lit only by the self-illuminated instruments (RTT displays, FF_EMISSIVE) + console lamps.
        // Daytime is unchanged (sun ~ full -> ambScale ~ 1); a small floor keeps a hint of glow (moon/panel spill).
        if (gFlags & FF_COCKPIT)
        {
            float sunLvl = (gNumLights > 0) ? dot(gLights[0].Color.rgb, float3(0.299, 0.587, 0.114)) : 1.0f;
            ambScale = clamp(sunLvl, 0.06f, 1.0f);
        }
        float3 lit = gAmbient.rgb * ambScale;
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
            // #72 boost only the directional (sun) term for the cockpit -- point lights (muzzle
            // flashes/explosions) keep their own intensity so a flash doesn't over-blow the pit.
            float lScale = (L.Params.y < 0.5f) ? sunScale : 1.0f;
            lit += L.Color.rgb * max(dot(N, Ldir), 0.0f) * atten * lScale;
        }
        if (gFlags & FF_FULLBRIGHT) lit = float3(1.0f, 1.0f, 1.0f);   // #97 unlit: full material colour (exit menu)
        col.rgb *= saturate(lit);

        // Specular (Blinn-Phong, per-vertex) from the MAIN source (light 0 = sun/NVG).
        // gSpecular.rgb = highlight color (from the surface material), gSpecular.w = power.
        // Added in the PS ON TOP of the texture (like D3D7 SPECULAR), hence in o.Spec, not col.
        // #72 cockpit: most pit surfaces carry NO material specular (SpecularIndex=0) -> matte/dead.
        // Give them a subtle default highlight so knobs/glass glint as the head moves (VR). Per-vertex
        // (soft/blocky on flat panels) but adds life; the model's own specular still wins when present.
        float  specPow = gSpecular.w;
        float3 specCol = gSpecular.rgb;
        if ((gFlags & FF_COCKPIT) && specPow <= 0.0f)
        {
            specPow = 20.0f;                       // #72 default cockpit gloss (tune freely)
            specCol = float3(0.10f, 0.10f, 0.10f); // #72 dim grey highlight (tune freely)
        }
        if (specPow > 0.0f && gNumLights > 0)
        {
            float3 V  = normalize(camWorld - worldPos.xyz);
            GpuLight L0 = gLights[0];
            float3 Ls = (L0.Params.y < 0.5f) ? -normalize(L0.Direction.xyz)
                                             : normalize(L0.Position.xyz - worldPos.xyz);
            float3 H  = normalize(Ls + V);
            float  s  = pow(max(dot(N, H), 0.0f), specPow);
            o.Spec = specCol * L0.Color.rgb * s;
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

// Flat / per-eye entry: uses cbView(b1) exactly as before.
VSOut VS_Object(VSInObject i) { return ObjectVSCore(i, gWorld, gView, gProj, gCameraPos.xyz); }

// Artscout - 2026: #DX12 п.5 -- view-instanced entry. SV_ViewID is 0 for the left eye's replica and
// 1 for the right; picks that eye's View/Proj from cbViewStereo(b5). The RT-array slice is routed by
// the PSO's D3D12_VIEW_INSTANCE_LOCATION (RenderTargetArrayIndex = SV_ViewID), so no SV_RTArrayIndex
// output is needed here. Geometry is built camera-relative to the SHARED head-centre origin (IPD lives
// in the per-eye view matrices), so a single draw feeds both eyes.
VSOut VS_ObjectVI(VSInObject i, uint vid : SV_ViewID)
{
    VSOut o = ObjectVSCore(i, gWorld, gView2[vid], gProj2[vid], gCamPos2.xyz);
    o.ViewId = vid;   // #13: the scene depth is an array with one slice per view -- index it by the SAME id
    return o;
}

//====================== #13 Volumetric clouds ================================
// Artscout - 2026: a raymarched cloud LAYER. Falcon's weather model is a slab (realWeather->stratusZ +/-
// stratusDepth/2), not a cumulus field, so a slab is what this integrates -- honest about what the sim knows.
//
// The geometry backing this pass is a camera-relative disc at the layer altitude (see RenderOTW::
// DrawVolumetricClouds). That choice is what keeps the whole thing cheap and VR-correct:
//   * the rasterizer's own depth sorts clouds against terrain -- no depth SRV, no MSAA depth resolve, and
//     nothing to go wrong per-view under view-instancing (each view rasterizes the same world geometry);
//   * the march is bounded by the slab, so cost is a fixed step count, not a screen-space raymarch of the
//     whole frustum (which at the quad-view focus resolution, 2740x2706 x2, would not fit the frame budget).
//
// Coordinates are Falcon world FEET with z DOWN. The camera is at the origin of WPos, so the view ray is
// just normalize(WPos), and a fragment's absolute world z is gCameraPos.z + WPos.z.

// Cheap hash-based value noise. Deliberately arithmetic, not a texture lookup: a 3D noise volume would need
// new texture plumbing (the manager is a 2D DDS/WIC path) and the bandwidth hurts more than the ALU here.
float CloudHash(float3 p)
{
    p = frac(p * 0.3183099f + float3(0.1f, 0.2f, 0.3f));
    p *= 17.0f;
    return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float CloudValueNoise(float3 p)
{
    float3 ip = floor(p);
    float3 fp = frac(p);
    fp = fp * fp * (3.0f - 2.0f * fp);   // smoothstep interpolant
    return lerp(lerp(lerp(CloudHash(ip + float3(0, 0, 0)), CloudHash(ip + float3(1, 0, 0)), fp.x),
                     lerp(CloudHash(ip + float3(0, 1, 0)), CloudHash(ip + float3(1, 1, 0)), fp.x), fp.y),
                lerp(lerp(CloudHash(ip + float3(0, 0, 1)), CloudHash(ip + float3(1, 0, 1)), fp.x),
                     lerp(CloudHash(ip + float3(0, 1, 1)), CloudHash(ip + float3(1, 1, 1)), fp.x), fp.y), fp.z);
}

// 4-octave fBm, NORMALIZED to 0..1. The normalization is not cosmetic -- it is what makes `coverage` mean
// what it says. Raw, this is a weighted sum of smoothed value noise, so it clusters tightly around its mean
// and never reaches either end: measured over 60k samples it runs min 0.100 / max 0.819 / mean 0.469 /
// sd 0.106. Threshold it at (1 - coverage) as if it were uniform on 0..1 and coverage=0.26 asks for n > 0.74,
// i.e. +2.5 sigma == 0.32% of space -- a few starved wisps pinned to one altitude by the profile, which reads
// as a thin flat sheet rather than clouds. Remapping the occupied band [0.23, 0.71] (p1..p99) onto 0..1 gives
// mean 0.498 / sd 0.219, and then coverage 0.26/0.45/0.75 yields 14.5%/41%/86% of space as cloud -- scattered,
// broken, overcast. Re-measure these constants if the octave count or weights ever change.
// lod = how much fine detail to DROP (0 = all four octaves, 0.75 = base octave only). It exists because the
// march's step grows with distance (geometric), and sampling an octave more coarsely than its Nyquist does not
// merely lose it -- it ALIASES, and along a near-horizontal ray (where the step is largest and altitude changes
// slowest) that alias lands on screen as horizontal ripples across the distant deck. Fading the octave out
// instead leaves smooth cloud.
//   Two things here are measured, not guessed. The FINEST octave must fade first -- an earlier cut had the
// index reversed and killed the BASE octave, collapsing coverage from 14.3% to 1.0%. And only the MEAN the
// dropped octaves carried is given back: rescaling by the surviving weight sum instead inflates the variance
// (sd 0.107 -> 0.174 at lod 0.75) and coverage runs away to 27%. With this form, coverage holds at
// 14.3 / 14.3 / 13.7 / 11.4% across lod 0 / 0.25 / 0.5 / 0.75. lod must never reach 1.0 -- there the base
// octave dies too and the field goes constant (0% cloud), hence the 0.75 clamp in CloudLod.
float CloudFbm(float3 p, float lod)
{
    float f = 0.0f, a = 0.5f, wsum = 0.0f;
    [unroll] for (int o = 0; o < 4; ++o)
    {
        float w = saturate(1.0f - (lod * 4.0f - (float)(3 - o)));   // octave 3 (finest) fades first
        f += a * w * CloudValueNoise(p);
        wsum += a * w;
        p *= 2.02f; a *= 0.5f;
    }
    f += 0.5f * (0.9375f - wsum);   // hand back ONLY the mean the dropped octaves carried
    return saturate((f - 0.23f) * (1.0f / 0.48f));
}

// Pick the LOD for a march step of dt feet: an octave is safe while the step is under half its feature size.
float CloudLod(float dt)
{
    float stepN   = max(dt * gCloud1.z, 1e-6f);          // step in NOISE units
    float safeOct = log2(max(0.5f / stepN, 1e-6f));      // highest octave still above Nyquist
    return clamp((3.0f - safeOct) * 0.25f, 0.0f, 0.75f);
}

// Density at a point given CAMERA-RELATIVE coords (see the gCloud0 note). Returns 0..1.
// cov/topF come from the caller: they are per-COLUMN maps (weather + top height) and were being rebuilt inside
// EVERY call -- including the four sun taps, which multiplied the cost by five for nothing.
// lite = skip the erosion detail. The sun march only needs roughly how much cloud lies toward the sun, and its
// four coarse 220 ft taps cannot resolve erosion anyway.
float CloudDensity(float3 wp, float lod, float cov, float topF, bool lite)
{
    // Height fraction inside the slab: 0 at the bottom, 1 at the top. zTop is MORE negative than zBot.
    float h = saturate((gCloud0.y - wp.z) / max(gCloud0.y - gCloud0.x, 1.0f));
    // Vertical profile: flat-ish base, rounded top -- a stratus deck, not a sphere.
    float profile = saturate(h * 4.0f) * saturate((1.0f - h) * 2.5f);

    // Back to ABSOLUTE feet for the noise lookup, so the field is pinned to the world: xy via the CPU-folded
    // anchor (camera.xy + wind), z via the camera's absolute altitude.
    //
    // gCloudDiag.w scales z independently. NOTE it was added as a fix for horizontal layering and that diagnosis
    // was WRONG -- the layering came from the step formula (a constant G made dt0 collapse to 0 as steps rose).
    // Stretching z far (0.3) flattens the noise toward 2D and costs the clouds their vertical structure, so the
    // default is 1.0 (isotropic, as the reference uses). Kept only as a tuning handle.
    float3 sp = float3((wp.xy + gCloud1.xy) * gCloud1.z, (wp.z + gCloud3.x) * gCloud1.z * gCloudDiag.w);
    float  n  = CloudFbm(sp, lod);

    if (gCloud3.y > 0.5f)
    {
        // CUMULUS -- built the standard way (Schneider/HZD), because the obvious way is wrong in two ways I
        // had to see on screen to believe. It used to modulate the THRESHOLD by height:
        //     cov = coverage * baseR(h) * topR(h);   d = (n - (1-cov)) / cov;
        // which produced exactly the two artefacts you get from it:
        //  * FLAT, CUT-OFF TOPS. As h -> 1, cov -> 0, so the DIVISOR collapses and any n a hair over the
        //    threshold snaps to d = 1. The cloud does not fade out at altitude -- it holds FULL density through
        //    an ever-thinner sliver and then stops dead against the slab.
        //  * STACKED DISCS. A height-dependent threshold against fully 3D noise lets one column cross it
        //    up-down-up, which literally stacks separate blobs at different heights.
        // The right construction: keep the threshold CONSTANT and take the base shape from the noise with z
        // PINNED, so each column carries one connected cloud; let height multiply the DENSITY; and use the 3D
        // noise only to erode. Verified against a simulated vertical slice before it ever compiled.
        // Height profile: a PARABOLA (the reference form's gradient_top * gradient_bottom = h * (1-h)), skewed
        // low and normalised so its peak is exactly 1. The point is that it has NO PLATEAU anywhere.
        //   The old `saturate(h/0.08) * saturate((1-h)/0.50)` was flat 1.0 across the entire LOWER HALF, so from
        // 6000 to 8500 ft the envelope was CONSTANT and nothing shaped the cloud vertically: the noise only chose
        // where clouds stood in plan, and each came out an extruded column with a flat top where the plateau ended.
        //   pow(h, 0.6) skews the peak down to h=0.375 -- a cumulus is widest in its lower third, not at mid-height.
        // The 2.882 is 1/peak: without it the curve reaches 1.39, saturate() clips it, and the plateau is back --
        // which is exactly the trap the previous version fell into.
        //   TOP HEIGHT VARIES PER CLOUD. h alone is a fraction of the SLAB, so h=1 is one absolute altitude across
        // the whole sky and every cloud dies at exactly the same ceiling -- a flat lid, however smoothly the
        // envelope tapers into it. Real cumulus do not agree on a top: one towers, the next stays flat.
        // So divide h by a mid-frequency 2D map (z pinned -- a cloud's height is a property of its column, not of
        // altitude): where the map is low the cloud reaches env=0 early and stays squat, where it is high it grows
        // to the full slab. gCloudDiag2.z sets how much they differ.
        float hh   = saturate(h / max(topF, 0.15f));
        float env  = saturate(2.882f * pow(hh, 0.6f) * (1.0f - hh));

        // Base shape from the FULL 3D noise. Pinning z here (to make the shape 2D) was an over-correction:
        // the stacked-discs artefact came from the height-modulated THRESHOLD above, never from the noise being
        // 3D. Removing the threshold modulation alone fixes it -- flattening the base as well just traded discs
        // for the opposite artefact, an extruded PICKET FENCE, since every column then has an identical
        // cross-section from base to crown (obvious at a grazing angle along the horizon). Simulated all three
        // variants on a vertical slice: 3D base + constant threshold is the one that bulges and tapers.
        float nb = CloudFbm(sp, lod);
        // Threshold SUBTRACTED, not divided out -- the reference form: saturate(noise - (1 - coverage)).
        // Dividing by coverage (0.26) amplified the transition 3.8x and made the cloud boundary a razor, so
        // density lived on the tail of the noise distribution where an octave, the lod or a step flipped it across
        // the threshold. Measured: d sat between 0.001 and ~0.05 nearly everywhere -- the debug view came back with
        // essentially no red channel. Subtracting gives a ramp neighbouring samples blend instead of flicker across.
        // WEATHER MAP. Coverage is one number for the whole theatre, which sprinkles clouds evenly like semolina --
        // real sky clusters: a clump here, a clear stretch there. So modulate coverage by a very low-frequency 2D
        // noise (gCloudDiag2.x sets how low; ~0.12 = features ~8x wider than a cloud). Pinning z keeps the map
        // purely horizontal -- it says WHERE weather is, and has no business varying with altitude.
        // The remap is deliberately harsh (x1.8 - 0.45): it pushes most of the map toward zero coverage and lets
        // only the peaks open up, which is what produces "a few small ones here, two or three big ones there"
        // instead of a uniform field. gCloudDiag2.y scales the whole thing (fewer/more clouds overall).
        //   The map and the coverage have SEPARATE jobs and must not multiply: the map decides WHERE weather is,
        // coverage decides how much sky it fills THERE. Chaining them (cov * remap(wm) * amount) just starved the
        // threshold -- 0.26 * 0.45 * 0.35 = 0.041, i.e. it demanded nb > 0.959, which is a fraction of a percent
        // of the field. That is why it came out as scraps with no clustering: `amount` was thinning everything
        // evenly instead of choosing patches.
        //   So `amount` is now the FRACTION OF SKY that has weather: it sets where the map opens (wm above
        // 1-amount), ramping in over half that width so patch edges are soft. Inside a patch, coverage applies at
        // full strength -- which is what makes a patch hold real clouds rather than a haze of crumbs.
        float d  = saturate(nb - (1.0f - cov));
        d *= env;                                           // height shapes DENSITY, never the threshold

        // Erosion, at a CONSTANT strength that does not know about height -- and as a REMAP, so it can never
        // subtract more than there is.
        //   The previous form, `d - det * (0.12 + 0.40*(1-env))`, smuggled back the very mistake removed further
        // up: a height-modulated threshold. Subtracting an amount that depends only on h from a density capped
        // only by env(h) makes the EFFECTIVE coverage threshold a function of h -- which lays down horizontal
        // dead bands, i.e. the deck sliced into layers with clear air between.
        //   Measured in the pixel debugger, two adjacent pixels at h~0.7518, env~0.496, one inside a cloud and
        // one in a gap: nb = 0.81929 and 0.82324, BOTH well over the 0.74 threshold, and BOTH d = 0. The band
        // demanded nb > 0.823 there instead of 0.74, and killed everything either side of it regardless of the
        // noise. Higher up it was worse still: at env = 0.136 the erosion was 0.466 against a density that could
        // not exceed 0.136, so nothing survived at any nb.
        //   The cauliflower crown comes from the noise and from `env`, not from eroding harder where the cloud is
        // already weakest.
        if (lite) return d;                                       // sun taps: erosion is below their resolution
        float det = CloudFbm(sp * 3.0f, saturate(lod + 0.25f));   // 3x finer -> aliases a step sooner
        float e   = det * 0.10f * gCloudDiag.x;
        d = saturate((d - e) / max(1.0f - e, 1e-3f));   // remap, not a raw subtract
        return d;
    }

    // STRATUS deck. Coverage carves the field: 0 = clear, 1 = solid. Remap so coverage moves the noise threshold
    // rather than just scaling density (scaling alone thins the whole deck into haze instead of opening holes).
    float d = saturate((n - (1.0f - gCloud0.z)) / max(gCloud0.z, 1e-3f));
    return d * profile;
}

// The per-COLUMN maps: where weather is (x) and how tall clouds grow there (y). Both pin z -- they describe a
// COLUMN, not an altitude -- so the march evaluates them ONCE per step and hands them down, instead of every
// CloudDensity rebuilding them and the four sun taps redoing it all again.
float2 CloudColumnMaps(float3 wp, float lod)
{
    float2 spxy  = (wp.xy + gCloud1.xy) * gCloud1.z;
    float  wm    = CloudFbm(float3(spxy * gCloudDiag2.x, 0.0f), lod);
    float  amt   = saturate(gCloudDiag2.y);
    float  patch = saturate((wm - (1.0f - amt)) / max(amt * 0.5f, 1e-3f));
    float  hmap  = CloudFbm(float3(spxy * 0.35f, 0.0f), lod);
    return float2(gCloud0.z * patch, 1.0f - gCloudDiag2.z * (1.0f - hmap));
}

// Beer-Lambert transmittance toward the sun: a short march, since only the first few hundred feet of
// self-shadowing actually reads on screen.
float CloudSunTransmittance(float3 wp, float cov, float topF)
{
    const int   LSTEPS = 4;
    const float LSTEP  = 220.0f;   // feet
    float t = 0.0f;
    [unroll] for (int s = 0; s < LSTEPS; ++s)
    {
        float3 lp = wp + gCloud2.xyz * (LSTEP * (float)(s + 1));
        t += CloudDensity(lp, CloudLod(LSTEP), cov, topF, true);   // lite: 1 fbm instead of 4
    }
    return exp(-t * LSTEP * gCloud0.w);
}

// Integrate the slab along the view ray. The ray starts at the camera, i.e. the ORIGIN (camera-relative space),
// so there is no ray-origin parameter. Returns rgb = scattered light, a = opacity.
float4 CloudMarch(float3 rd, float tMax, float jitter)
{
    // Slab intersection. rd.z ~ 0 means the ray runs along the layer -- clamp the span instead of dividing by
    // zero, otherwise a horizon-grazing pixel marches to infinity.
    float t0, t1;
    if (abs(rd.z) < 1e-4f)
    {
        // Inside the slab and parallel to it -> take a long fixed span; outside -> nothing to integrate.
        // Camera-relative, so the camera's z is 0 and the slab straddles it exactly when zTop < 0 < zBot.
        if (0.0f > gCloud0.y || 0.0f < gCloud0.x) return float4(0, 0, 0, 0);
        t0 = 0.0f; t1 = 120000.0f;
    }
    else
    {
        float ta = gCloud0.x / rd.z;   // (zTop - 0) / rd.z
        float tb = gCloud0.y / rd.z;   // (zBot - 0) / rd.z
        t0 = min(ta, tb); t1 = max(ta, tb);
        t0 = max(t0, 0.0f);
        if (t1 <= t0) return float4(0, 0, 0, 0);
    }
    // Stop where the WORLD does. tMax is the scene's distance along this ray (from the depth buffer); the
    // fixed clamp is the fallback when there is no usable depth, and also keeps steps meaningful at the horizon.
    t1 = min(t1, min(tMax, 160000.0f));
    if (t1 <= t0) return float4(0, 0, 0, 0);   // the world hides this ray's whole cloud span

    // Step scheme. A UNIFORM step across the whole span was the flaw: the span is whatever the ray happens to
    // cut through the slab, so a near-horizontal ray gets t1-t0 ~ 160k ft and, over 24 samples, a 6.6k ft step --
    // while the noise's base feature is ~3.5k ft. Sampling twice as coarsely as the field varies puts the sample
    // PLANES on screen: stacked plates, dithered bands, ripples that swim as the span changes per pixel, and
    // "flying through them feels like flying through thin sheets". Raising the density only made it visible.
    //
    // Two fixes, both standard, both needed:
    //  * GEOMETRIC growth -- each step is G x the last, so the series still sums to the span but the samples
    //    crowd near the camera where detail is resolvable. A cloud 40 km out covers a few pixels and the haze
    //    has half-dissolved it; spending equal samples there was the waste that starved the near end.
    //  * JITTER -- push the first sample by a per-pixel fraction of a step, so the planes stop lining up
    //    between neighbouring pixels and the residual banding becomes noise.
    int   steps = (int)max(gCloud1.w, 2.0f);
    // G is DERIVED from the step count, never a constant. A fixed G=1.12 made this formula destroy itself as
    // steps rose: dt0 = span*(G-1)/(G^steps - 1), so G^steps explodes exponentially --
    //     steps  32 -> G^steps 3.8e1  -> first step 525 ft   (worked)
    //     steps 500 -> G^steps 4.1e24 -> first step 0.0000 ft (every sample crammed at the slab entry)
    //     steps 800 -> G^steps 2.4e39 -> OVERFLOWS float32 -> dt = 0 -> the march never advances, no clouds
    // (1.12^n passes the float32 max at exactly n = 783, which is precisely where the clouds vanished.)
    // So raising the step count made sampling WORSE, not better, while looking like proof that step count was
    // innocent -- every 200/500/600 test was measuring the same handful of coarse samples.
    // Pick G so the LAST step is a fixed multiple of the first instead: G = R^(1/(steps-1)). Sum still equals
    // the span, the ratio stays bounded, and more steps now means finer sampling, monotonically.
    const float R = 64.0f;                                        // last/first step ratio
    float  G  = pow(R, 1.0f / max((float)steps - 1.0f, 1.0f));
    float  gN = pow(G, (float)steps);
    float  dt = (t1 - t0) * (G - 1.0f) / max(gN - 1.0f, 1e-4f);   // first step; sum(dt*G^i) == span
    float  t  = t0 + dt * jitter;

    const float kSkip = 6.0f;   // coarse stride multiplier while the ray is in clear air
    int    miss  = 0;           // consecutive empty samples -- gates the stride (see the loop)
    float  trans = 1.0f;
    float3 scat  = float3(0, 0, 0);
    // Ambient = sky light from above; the layer's underside is darker than its top.
    float3 amb   = gFogColor.rgb * gCloud2.w;

    [loop] for (int s = 0; s < steps; ++s)
    {
        if (trans < 0.01f) break;   // saturated -- nothing behind this contributes
        if (t >= t1) break;
        float3 wp = rd * (t + dt * 0.5f);   // camera at the origin
        float  lodS = CloudLod(dt);                    // detail matched to THIS step's coarseness
        float2 cm   = CloudColumnMaps(wp, lodS);       // per-column maps: ONCE per step, not per call
        float  d    = CloudDensity(wp, lodS, cm.x, cm.y, false);

        // EMPTY-SPACE SKIPPING. The fine step exists to resolve cloud; spending it on air is pure waste, and air
        // is most of the slab -- a ray that finds nothing still paid for every sample. So run at a COARSE stride
        // while the field reads empty, and drop to the fine step only after something is found; on losing the
        // cloud again, wait a few samples before speeding back up (`miss`), otherwise a ray skims a cloud's edge,
        // accelerates, and steps straight over its body.
        //   Correctness is preserved by backing up one coarse stride when a hit is found: the boundary was
        // somewhere inside the stride we just jumped, so the fine march has to re-enter it, or cloud edges get
        // bitten off at exactly the coarse spacing (which would look like... layers).
        if (d <= 0.001f)
        {
            ++miss;
            if (miss > 4) { t += dt * kSkip; dt *= G; }   // confidently in clear air -> stride
            else          { t += dt; dt *= G; }
            continue;
        }
        if (miss > 4) { t -= dt * kSkip; miss = 0; continue; }   // re-enter the stride we skipped over
        miss = 0;

        // DEBUG views (gCloudDiag.z): show the raw quantity at this first sample that has density, flat and
        // unlit, and stop. See the gCloudDiag note in cbRender.
        if (gCloudDiag.z > 0.5f)
        {
            // MUST mirror CloudDensity exactly. It did not: this still used the old plateau envelope and
            // ignored topF, so the view reported an `env` the shader no longer computes -- and a debug view that
            // lies is worse than none. It already cost a wrong diagnosis once.
            float hRaw = saturate((gCloud0.y - wp.z) / max(gCloud0.y - gCloud0.x, 1.0f));
            float hh   = saturate(hRaw / max(cm.y, 0.15f));                  // cm.y = topF (per-column top height)
            float ev   = saturate(2.882f * pow(hh, 0.6f) * (1.0f - hh));
            float3 sp2 = float3((wp.xy + gCloud1.xy) * gCloud1.z, (wp.z + gCloud3.x) * gCloud1.z * gCloudDiag.w);
            float nbv  = CloudFbm(sp2, lodS);
            // Mode 1 packs the three fields into COLOUR CHANNELS -- R=density, G=envelope, B=noise -- so ONE
            // screenshot carries all of them with identical geometry. Switching modes between shots would need a
            // restart, and the view could never be reproduced exactly; separate channels sidestep that entirely.
            // Modes 2/3/4 stay as single-quantity greyscale for when one needs a closer look.
            if (gCloudDiag.z < 1.5f) return float4(d, ev, nbv, 1.0f);
            float v = (gCloudDiag.z < 2.5f) ? ev : (gCloudDiag.z < 3.5f) ? nbv : hRaw;
            return float4(v, v, v, 1.0f);
        }

        float sigma = d * gCloud0.w;
        float sun   = CloudSunTransmittance(wp, cm.x, cm.y);
        // Powder term: the "silver lining" -- edges facing the sun scatter forward and read brighter than
        // Beer alone predicts. gCloudSun.a scales it.
        float powder = 1.0f - exp(-d * 2.0f * gCloudSun.a);
        float3 lit   = gCloudSun.rgb * sun * powder + amb;
        // Aerial perspective, from THIS step's own distance. It has to happen here: the VS's FogF is computed
        // at the backing geometry's vertices, and that geometry is a camera-centred sphere of FIXED radius --
        // every vertex reports the same distance, which says nothing about where the cloud is. Fog the COLOUR
        // only: a distant cloud is not transparent, it merely takes the haze's colour.
        float fogF = saturate((gFogEnd - t) / max(gFogEnd - gFogStart, 1e-4f));
        lit = lerp(gFogColor.rgb, lit, fogF);

        // Energy-conserving integration over the step (analytic, so step count changes brightness far less
        // than a naive sum would -- important because the step count is a perf knob).
        float  a  = 1.0f - exp(-sigma * dt);
        scat  += trans * a * lit;
        trans *= (1.0f - a);
        t  += dt;
        dt *= G;      // geometric: coarser further out, where it cannot be resolved anyway
    }
    return float4(scat, saturate(1.0f - trans));
}

//============================ Pixel shader ===================================

float4 PS_Main(VSOut i) : SV_Target
{
    // Artscout - 2026: #13 volumetric cloud layer. Short-circuits before texture/lighting: the disc carries
    // no texture and no vertex lighting, only the ray.
    if (gFlags & FF_CLOUD)
    {
        // The camera is the origin of WPos, so the view ray is simply its direction.
        float3 rd = normalize(i.WPos);
        // How far along THIS ray does the world sit? Sample the scene depth at this pixel and turn it back
        // into a distance. Reversed-Z (near=1, far=0) is handled implicitly: A/B come straight from the
        // projection, so depth = A + B/forward  ->  forward = B / (depth - A), whatever the convention.
        // NOTE this engine's view FORWARD axis is X, not Z (see the fog comment in ObjectVSCore: the fog
        // distance is clip.w == view.x) -- so 'forward' here is a distance along the view's X axis, and
        // dividing by (rd . forwardAxis) converts it to a distance along the ray.
        float tMax = 1e30f;
        if (gCloudFwd.w > 0.5f)
        {
            float d = gSceneDepth.Load(int4((int2)i.Pos.xy, (int)i.ViewId, 0));
            // depth == the far value (0 under reversed-Z) means nothing was drawn: sky, so no clamp.
            if (d > 1e-6f)
            {
                float fwd   = gCloud3.w / (d - gCloud3.z);
                float rdFwd = dot(rd, gCloudFwd.xyz);
                if (fwd > 0.0f && rdFwd > 1e-3f) tMax = fwd / rdFwd;
            }
        }
        // Per-pixel jitter for the march's first sample (see CloudMarch). Any decorrelated 0..1 works;
        // reuse the cloud hash on the pixel coords.
        float jit = CloudHash(float3(i.Pos.xy, 0.5f));
        float4 c = CloudMarch(rd, tMax, jit);
        if (c.a < 0.004f) discard;   // fully clear pixel: keep the sky/terrain behind untouched
        // Deliberately NOT i.FogF: the march already applied aerial perspective per step, at each step's own
        // distance. i.FogF is meaningless for this pass -- the VS computes it at the backing geometry's
        // vertices, and that geometry is a camera-centred SPHERE of fixed radius, so every vertex reports the
        // same (huge) distance. Along the view axis that is past gFogEnd -> FogF 0 -> alpha 0; at grazing
        // angles the forward distance collapses -> FogF 1. i.e. a radial alpha gradient painted across the
        // screen by the helper geometry: clouds gauzy dead ahead, solid at the edges.
        return float4(c.rgb, c.a);
    }

    // Artscout - 2026: FF_GLOC -- G-force blackout / redout vignette. This is a fullscreen overlay quad
    // (uv 0..1) alpha-blended over the finished frame: output the tint (gMaterialColor.rgb) with alpha =
    // a radial ramp so the periphery darkens/tints while the centre stays clear. Radius 0 at the screen
    // centre, ~1 at the mid-edges (sqrt(2) at the corners); smoothstep feathers the ring and saturates
    // to full past the outer radius. Short-circuits before any texture/lighting work.
    if (gFlags & FF_GLOC)
    {
        float2 gd = i.Uv0 - 0.5f;
        float  gr = length(gd) * 2.0f;
        float  ga = smoothstep(gGloc.y, gGloc.z, gr) * gGloc.x;
        return float4(gMaterialColor.rgb, saturate(ga));
    }

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
        // #49 v2: a LIVING plume. Rendered ADDITIVE (Src=ONE,Dst=ONE): scale color BY the texture
        // brightness (dim texels add little -> soft edges; dense core saturates to white). On top of the
        // static warm recolor we add TIME-animated turbulence + flicker so the flame licks/throbs instead
        // of a frozen cone. Uses gWaterParams.x (scene animation time, sec). Runtime shader -- tune freely.
        float tAB = gWaterParams.x;

        // Flowing turbulence: sine layers scrolled along the plume UV -> ripples running down the flame.
        // Orientation-agnostic (works whichever way the cone is UV-mapped) -> still reads as a live burner.
        float2 uvAB = i.Uv0;
        float turb = 0.5f
                   + 0.30f * sin(uvAB.y * 15.0f - tAB * 11.0f + uvAB.x * 6.0f)
                   + 0.16f * sin(uvAB.y * 29.0f - tAB * 19.0f - uvAB.x * 10.0f + 1.7f)
                   + 0.08f * sin(uvAB.x * 22.0f + tAB * 7.0f);
        turb = saturate(turb);

        float b    = max(c.r, max(c.g, c.b));            // texture brightness (c = texture * white)
        float bMod = b * lerp(0.60f, 1.30f, turb);       // turbulence ripples the body, keeps the core

        float3 cool = float3(1.0f, 0.30f, 0.06f);        // orange/red outer plume
        float3 hot  = float3(1.0f, 0.95f, 0.82f);        // white-hot core near the nozzle
        float3 core = float3(0.70f, 0.82f, 1.00f);       // faint blue-white shock diamonds at the peak
        float3 tint = lerp(cool, hot, saturate(bMod * 1.4f));
        tint = lerp(tint, core, saturate((bMod - 0.85f) * 3.0f) * 0.45f);

        // Day/night intensity: in daylight a real AB plume is nearly invisible (nozzle flame + heat haze);
        // the bright glowing plume is a dusk/night thing. Scale by scene darkness (gAmbient).
        float amb     = saturate(max(gAmbient.r, max(gAmbient.g, gAmbient.b)));
        float night   = 1.0f - amb;                  // 0 = bright day .. 1 = night
        float intensity = lerp(0.45f, 2.6f, night);  // day endpoint .. night endpoint

        // Fast overall flicker so the whole burner throbs; modulated by the turbulence for irregularity.
        float flick = 0.85f + 0.15f * sin(tAB * 42.0f) * (0.6f + 0.4f * turb);

        c.rgb = tint * bMod * intensity * flick;
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

    // #72 cockpit: NON-darkening pop. A mid-pivot contrast darkened the (already dark) pit -> user found
    // it too dark. Instead do a gentle overall BRIGHTEN with a soft highlight clip (saturate) for a touch
    // of top-end pop. Depth comes from the sun gradient + specular glints, not from crushing shadows
    // (real crevice depth = SSAO, a separate step). kCockpitBright: 1.0 neutral, >1 brighter. Tune freely.
    if (gFlags & FF_COCKPIT)
    {
        const float kCockpitBright = 1.08f;
        c.rgb = saturate(c.rgb * kCockpitBright);
    }

    // #DX12 A5: sensor (TGP/Maverick/FLIR) pass -> monochrome grey. Desaturate the composed colour to Rec.601
    // luma. Applies to the whole 3D scene (terrain + objects) drawn while IR/TV mode is set; the MFD symbology
    // is drawn afterwards with the flag cleared, so it keeps its own colour.
    if (gFlags & FF_IRGREY)
        c.rgb = dot(c.rgb, float3(0.299f, 0.587f, 0.114f)).xxx;

    // Artscout - 2026: #97 NVG -- night-vision goggles. Convert the fully-composed colour to a green phosphor image
    // with tube gain, so EVERYTHING seen through the goggles (terrain, aircraft, cockpit, sky) goes green -- not just
    // the legacy 2D sky/far tiles. Set on the world passes when NVG is on; self-lit surfaces bloom green as well.
    if (gFlags & FF_NVG)
    {
        const float kNvgGain = 4.0f;                         // image-intensifier gain (tune: higher = brighter)
        float lum = dot(c.rgb, float3(0.30f, 0.59f, 0.11f));
        lum = 1.0f - exp(-lum * kNvgGain);                   // lift the low-light scene, saturates toward 1

        // Phase 2 tube artefacts (pixel-space, so per-eye in VR): horizontal scanlines, animated photon grain, and a
        // soft circular tube vignette. gWaterParams.x = scene time (sec) animates the grain. All subtle -- tune freely.
        float scan  = 0.92f + 0.08f * sin(i.Pos.y * 3.14159f);                 // 1px raster scanline
        float2 np   = i.Pos.xy + gWaterParams.x * 37.0f;                       // time-scrolled noise seed
        float  grain = frac(sin(dot(np, float2(12.9898f, 78.233f))) * 43758.5453f);
        float  noise = 0.91f + 0.09f * grain;                                  // photon shot noise (subtle)
        float2 vc   = i.Pos.xy / max(gScreenSize, 1.0f) - 0.5f;               // 0..1 -> centred
        float  vig  = saturate(1.0f - dot(vc, vc) * 1.35f);                    // circular tube edge falloff

        lum *= scan * noise * vig;
        c.rgb = float3(0.10f, 1.0f, 0.28f) * lum;            // green phosphor tint
    }

    return c;
}

//============================ GPU-instanced particles ========================
// Artscout - 2026: #VFX Phase 1 -- GPU-instanced billboard particles. One
// DrawIndexedInstanced(6, N) expands N camera-facing quads entirely on the GPU:
// a shared unit-quad (slot 0) is rotated/scaled/placed per instance from the
// per-instance stream (slot 1). Reuses cbView (gView/gProj/gCameraPos) and gTex0/
// gSamp0 -- no new constant buffers. Additive OR alpha blend is chosen by the PSO,
// so the pixel shader stays blend-agnostic (texture * instance colour).

// Slot 0 (PER_VERTEX): the static unit quad corner + its cell uv.
struct VSInParticleVtx
{
    float2 Corner : POSITION;    // quad corner in [-0.5,+0.5]
    float2 Uv     : TEXCOORD0;   // cell uv in [0,1]
};

// Slot 1 (PER_INSTANCE): one record per particle (matches D3D12ParticleInstance).
struct VSInParticleInst
{
    float3 Center : TEXCOORD1;   // world position of the particle centre
    float2 Size   : TEXCOORD2;   // world width,height of the billboard
    float  Rot    : TEXCOORD3;   // billboard-plane rotation (radians)
    float4 Color  : COLOR0;      // rgba modulate (NOT premultiplied)
    float4 UvRect : TEXCOORD4;   // xy = atlas uv offset, zw = atlas uv scale (flipbook cell)
};

struct VSOutParticle
{
    float4 Pos   : SV_Position;
    float4 Color : COLOR0;
    float2 Uv0   : TEXCOORD0;
};

// Artscout - 2026: #DX12 п.5 -- transform inputs parameterized so the same billboard body serves both
// the flat/per-eye entry (VS_Particle) and the view-instanced entry (VS_ParticleVI). inst.Center is
// camera-relative to the SHARED head-centre origin, so the billboard faces the shared eye centre; the
// per-eye parallax comes from the two view matrices. Pure refactor: VS_Particle behaviour unchanged.
VSOutParticle ParticleVSCore(VSInParticleVtx v, VSInParticleInst inst, float4x4 vmat, float4x4 pmat)
{
    VSOutParticle o;

    // Artscout - 2026: #VFX SPHERICAL billboard facing the camera POSITION. The old basis (gView
    // columns) baked in camera roll + the RH->LH Flip, so the quads sat in a FIXED plane -> flying
    // past an explosion showed them edge-on as several stacked flat sheets (NOT camera-facing). The
    // legacy DX2D path oriented billboards from a dedicated BB matrix (RotY(pitch)*RotZ(yaw), no roll)
    // that is never uploaded to the shader. Rebuild an equivalent here from geometry only: inst.Center
    // is camera-RELATIVE world, so -Center points at the eye; constrain 'up' to world up (NED = -Z),
    // like the roll-free BB matrix. This always faces the viewer -> no more stacked-plate look.
    float3 fwd   = normalize(-inst.Center);
    float3 wup   = float3(0.0f, 0.0f, -1.0f);
    float3 right = cross(wup, fwd);
    float  rl    = length(right);
    right = (rl > 1e-4f) ? (right / rl) : float3(1.0f, 0.0f, 0.0f);   // guard: gaze near-vertical
    float3 up    = cross(fwd, right);

    // Rotate the unit-quad corner in the billboard plane, then scale by the world size.
    float s = sin(inst.Rot);
    float c = cos(inst.Rot);
    float2 rc  = float2(v.Corner.x * c - v.Corner.y * s,
                        v.Corner.x * s + v.Corner.y * c);
    float2 off = rc * inst.Size;

    // Expand to a world-space, camera-facing position, then transform EXACTLY like VS_Object
    // (row_major, vector-on-the-left): viewPos = worldPos * gView; clip = viewPos * gProj.
    float3 worldPos = inst.Center + right * off.x + up * off.y;
    float4 viewPos  = mul(float4(worldPos, 1.0f), vmat);
    o.Pos = mul(viewPos, pmat);

    // Artscout - 2026: #VFX per-vertex VERTICAL GRADIENT -- DX7->D3D11 parity. The legacy DX2D
    // particle quad shaded the TOP verts at full brightness (HiColor) and the BOTTOM at 0.68x
    // (LoColor). That cheap fake self-shadow is what makes a CLOUD of overlapping smoke/fire sprites
    // read as a billowing VOLUME instead of flat uniform discs -- the single thing the instanced path
    // was missing vs the D3D11 (DX2D) render that looked volumetric in VR/QuadViews. Unit quad maps
    // uv = corner + 0.5, so v.Uv.y = 1 at the top (world up) and 0 at the bottom. RGB only (alpha
    // unchanged, exactly as HiColor/LoColor shared the same F_TO_UARGB alpha in the legacy quad).
    // #VFX per-vertex vertical gradient (DX7->D3D11 parity): top ×1.0, bottom ×0.68 (HiColor/LoColor).
    float grad = lerp(0.68f, 1.0f, v.Uv.y);
    o.Color = float4(inst.Color.rgb * grad, inst.Color.a);
    o.Uv0   = inst.UvRect.xy + v.Uv * inst.UvRect.zw;   // atlas / flipbook cell
    return o;
}

// Flat / per-eye entry: cbView(b1) as before.
VSOutParticle VS_Particle(VSInParticleVtx v, VSInParticleInst inst)
{
    return ParticleVSCore(v, inst, gView, gProj);
}

// Artscout - 2026: #DX12 п.5 -- view-instanced particle entry (per-eye View/Proj from cbViewStereo b5).
VSOutParticle VS_ParticleVI(VSInParticleVtx v, VSInParticleInst inst, uint vid : SV_ViewID)
{
    return ParticleVSCore(v, inst, gView2[vid], gProj2[vid]);
}

float4 PS_Particle(VSOutParticle i) : SV_Target
{
    float4 t = gTex0.Sample(gSamp0, i.Uv0);
    // #VFX Phase 3 TODO: soft-particle depth fade here -- needs a SAMPLEABLE scene-depth SRV
    // (the D3D12 backend depth is DSV-only today). Then: t.a *= saturate((sceneZ - particleZ)/k).
    return t * i.Color;
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
    o.WPos = float3(0, 0, 0); o.ViewId = 0;   // #13: unused here (FF_CLOUD never runs on the per-sample path),
                                              // but VSOut must be fully initialised before PS_Main reads it.
    return PS_Main(o);
}
