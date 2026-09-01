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
#define FF_BINDLESS     (1u << 19)  // Artscout - 2026 (#107): sample the base texture from the resident heap by
                                    // the per-vertex slot instead of t0. Set only by DrawTerrainMeshBindless;
                                    // same bit as FF_BINDLESS in the Vulkan shader and in ffstatemap.h.
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
    // Artscout - 2026: #13 lighting. x = the SUN march's extinction per foot -- a SEPARATE coefficient from
    // gCloud0.w (the view march's), as in the reference. y = multiple-scattering strength.
    float4 gCloudLight;
    // Artscout - 2026: #13 the model library. x = model count (0 = no library, take the procedural bake),
    // y/z = the sdf encode range the CONVERTER measured across all models, w = cell size in noise units.
    //   Its OWN float4, and that is not tidiness. I first hung sdfMax on gCloudDiag2.w, which was then the
    // reference-mode switch (since removed), so writing 1.6 there sent the entire screen through a different
    // renderer and the sky came out empty while the library loaded perfectly. Same class as cloud3[2] being
    // overwritten by the projection, and as SunGain never reaching the shader: always from taking an occupied
    // slot without grepping it first. (gCloudDiag2.w is free again now that the reference port is gone.)
    float4 gCloudLib;
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
// Artscout - 2026 (#13): the BAKED cloud noise volume (R8, 128^3 + mips) and its own sampler. gSamp0/1 are
// anisotropic and carry the terrain's MipLODBias -- neither belongs on a noise volume, hence s2 (plain
// trilinear WRAP). WRAP is load-bearing, not cosmetic: the volume TILES and the march depends on it.
Texture3D<float4>     gCloudNoise : register(t3);   // #13 NVDF, theirs: R = encoded SDF, G = dimensional
                                                    // profile, B = detailType (billowy<->wispy), A = 1
// Artscout - 2026 (#13): t4 = the UP-REZ detail volume -- WORLD space, its own tile, separate from the NVDF
// exactly as theirs is. R = lowFreqWispy, G = highFreqWispy, B = lowFreqBillow, A = highFreqBillow.
Texture3D<float4>     gCloudDetail : register(t4);
// Artscout - 2026 (#13): t5 = the LIGHT CACHE -- r = the density INTEGRAL toward the sun, g = the same straight
// up, both in feet-density units. NOT tau: multiplying by the extinction happens at lookup, so CloudSunExt and
// CloudDensity stay live knobs instead of being frozen into a dispatch.
//   This is the thing that lets a 1 m light march exist at all. Theirs marches 128 steps toward the sun -- and
// the VIEW march never does: it reads ONE trilinear fetch from a volume a compute pass filled. Their light march
// is amortised over the cache's voxels; ours was re-run per sample per pixel, which is why our 12 steps cost
// more than their 128, and why "we cannot afford 1 m" was wrong.
Texture3D<float2>     gLightCache  : register(t5);
SamplerState          gSampNoise  : register(s2);
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
#if FF_BINDLESS_OK
    // Artscout - 2026 (#107): tile slot in the resident heap. Only the bindless terrain layout supplies it;
    // every other pipeline leaves FF_BINDLESS clear, so the value is ignored.
    uint   TexIndex : TEXCOORD1;
#endif
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
#if FF_BINDLESS_OK
    // Artscout - 2026 (#107): the tile slot, flat -- it is per vertex of a tile, not something to blend.
    nointerpolation uint TexIndex : TEXCOORD6;
#endif
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
#if FF_BINDLESS_OK
    o.TexIndex = 0xFFFFFFFFu;   // #107: the 2D path has no tile slot -- the PS falls back to t0
#endif
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
#if FF_BINDLESS_OK
    o.TexIndex = i.TexIndex; // #107 tile slot, carried through untouched
#endif

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

// Cheap hash. Only the march's per-pixel dither uses this now -- the cloud FIELD is a baked volume (gCloudNoise).
//   The comment that used to sit here claimed the arithmetic form was deliberate because "a 3D volume would need
// new texture plumbing and the bandwidth hurts more than the ALU". Both halves were wrong. The plumbing was a
// day's work (Create3D + t3), and the bandwidth argument was backwards: one cached fetch beats ~320 ALU, which
// is why the procedural field cost the cloud fly-through ~7 fps. It was a rationalisation of the easy path.
// Artscout - 2026 (#13): their Hash231/StaticStepJitter, verbatim. White noise over (pixel, stepIndex).
//   Ours was a 4x4 Bayer per pixel, advanced by the golden ratio per step, and I argued for it at length:
// blue noise is spatially uniform, white noise makes neighbours draw independently and speckles an edge. That
// reasoning is right for ONE sample per ray, which is what the march did when I wrote it. It is wrong now: with
// a jitter on EVERY step, a shared per-step advance keeps two pixels' offsets a fixed distance apart forever,
// so the 4x4 grid survives the whole ray and reads as a checkerboard -- the "рябь" on every fringe. Sixteen
// distinct values cannot dither a continuous field either way.
//   Decorrelating pixel AND step is what makes it grain instead of a lattice, and grain is what temporal
// accumulation (their uFrame%32, which we do not have yet) is able to average away.
float CloudHash231(uint2 p, uint stepIndex)
{
    uint n = p.x * 1973u ^ p.y * 9277u ^ stepIndex * 26699u ^ 0x68bc21ebu;
    n = (n << 13u) ^ n;
    return frac((n * (n * n * 15731u + 789221u) + 1376312589u) / 4294967296.0f);
}

float CloudHash(float3 p)
{
    p = frac(p * 0.3183099f + float3(0.1f, 0.2f, 0.3f));
    p *= 17.0f;
    return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}


// 4-octave fBm, NORMALIZED to 0..1 -- now a single fetch from a BAKED volume (see EnsureCloudNoise). It used
// to be computed here: 4 octaves x 8 hashes = ~320 ALU, called twice per march step by CloudDensity and once
// more per step by the sun march. That cost, not the step count, is why flying into a cloud fell to 7 fps.
//
// The normalization is baked in and is not cosmetic -- it is what makes `coverage` mean what it says. Raw, the
// field is a weighted sum of smoothed value noise, so it clusters tightly around its mean and never reaches
// either end: measured over 60k samples it runs min 0.100 / max 0.819 / mean 0.469 / sd 0.106. Threshold that
// at (1 - coverage) as if it were uniform on 0..1 and coverage=0.26 asks for n > 0.74, i.e. +2.5 sigma ==
// 0.32% of space -- a few starved wisps pinned to one altitude, reading as a thin flat sheet rather than
// clouds. Remapping the occupied band [0.23, 0.71] (p1..p99) onto 0..1 gives mean 0.498 / sd 0.219, and then
// coverage 0.26/0.45/0.75 yields 14.5%/41%/86% of space as cloud -- scattered, broken, overcast. THESE
// CONSTANTS ARE MEASURED: re-measure them (in EnsureCloudNoise, where the remap now lives) if the octave count
// or weights ever change. The same warning is why the volume is a single tap and not two mixed taps -- mixing
// would shrink sd and silently move coverage.
//
// lod = how much fine detail to DROP (0 = full field, 0.75 = base octave only), and it is now just a MIP. It
// exists because the march's step grows with distance (geometric), and sampling detail below its Nyquist does
// not merely lose it -- it ALIASES, and along a near-horizontal ray (largest step, slowest altitude change)
// that alias lands on screen as horizontal ripples across the distant deck. A prefiltered mip both removes the
// detail and preserves the mean, which is exactly the property the old hand-rolled octave fade was built for
// (rescaling by the surviving weight sum instead inflated the variance and ran coverage away to 27%).
//
// The volume TILES every CLOUD_NOISE_PERIOD noise units (~7 km at the default g_fCloudScale). That is the one
// real regression against procedural noise, which never repeated; see EnsureCloudNoise for why it is not free
// to fix.
#define CLOUD_NOISE_PERIOD 16.0f
#define CLOUD_DETAIL_U     0.8f    // must match CN_DETAIL_U: one detail tile spans this many noise units

// Artscout - 2026 (#13): NO uvw warp. There was one -- CloudSmoothUvw -- that pushed the fractional texel
// coordinate through a smoothstep to make the hardware's C0 trilinear into C1, on the theory that the gradient
// break at each texel boundary was cutting flat facets. Its own comment offered N=256 as the alternative it was
// standing in for. We have N=256 now, and the warp turned out to be the "staircase through the whole volume"
// Albert reported. Measured, resampling a LINEAR RAMP:
//
//     slope of the reconstruction (ideal: constant 1.0)
//       plain trilinear   1.000 .. 1.000     <- reproduces a straight line exactly
//       CloudSmoothUvw    0.003 .. 1.500     <- ZERO at every texel centre
//
// A zero slope is a plateau; a lattice of plateaus joined by steep ramps is a staircase, at exactly the texel
// frequency. And the joke is that the R channel stores the SDF *because* a distance field is near-linear and
// trilinear reconstructs it almost exactly -- then this warp destroyed that property on the way out. Making a
// field C1 by flattening it at every lattice point is not smoothing, it is quantizing.
//   Neither reference has anything like it: both sample the volume with a plain trilinear SampleLevel. Mine.
//======================= The cloud MODEL LIBRARY, instanced =====================
// An NVDF is a cloud MODEL -- made once, then scattered. Guerrilla ships a library and instances it; the Cumulus
// demo stretches one asset over its whole 4 km box, which is why it never shows this half.
//
// Measured, this is the whole point: a tiling volume spends 256^3 on a 16.3 km cube that is ~99% empty air, so a
// cloud gets 46 texels. A per-cloud cube spends all 128^3 on ONE cloud -> 115, from the same memory, and the
// 16 km repeat is gone because there is no tile.
//
// One cell of a lattice = one instance. Cells are sized so a cloud NEVER crosses into its neighbour, which is
// what keeps this to ONE fetch per sample instead of eight -- and our clouds were already deliberately
// separated, so nothing is lost. Coverage finally means what it says: the fraction of cells holding a cloud.
struct CloudInst
{
    float3 centre;   // noise units
    float  hext;     // half-extent, noise units. The model cube [-1,1] maps onto centre +- hext.
                 // NOT `half`: that is an HLSL keyword (the 16-bit float type).
    float  yaw;      // radians. One library, every instance turned differently -- 12 models stop reading as 12.
    float  rot;      // 0..3: the yaw QUANTIZED to cardinal turns. yaw is derived from it, never the reverse.
    float  model;    // index into the atlas
    bool   valid;
};

CloudInst CloudInstanceAt(float3 sp)
{
    CloudInst it;
    const float cell = gCloudLib.w;                         // noise units per cell
    float2 cid = floor(sp.xy / max(cell, 1.0e-3f));
    float  h0  = CloudHash(float3(cid, 11.0f));
    float  h1  = CloudHash(float3(cid, 23.0f));
    float  h2  = CloudHash(float3(cid, 37.0f));
    float  h3  = CloudHash(float3(cid, 53.0f));
    float  h4  = CloudHash(float3(cid, 71.0f));   // its OWN hash: h2 already jitters the centre's x, and reusing
                                                  // it would tie every instance's turn to which way it stepped.

    // Coverage = the fraction of cells that hold a cloud at all. Not a threshold on a noise field -- THIS is
    // what "where the weather is" means when clouds are objects, and it is why every coverage knob before now
    // behaved so strangely: it was cutting a percolating field, which gives crumbs or an overcast and nothing
    // in between.
    it.valid = (h0 < saturate(gCloud0.z));

    // Half-extent: at most half the cell, so neighbours cannot overlap and one fetch is enough.
    it.hext  = cell * 0.5f * (0.55f + 0.40f * h1);
    // Jitter the centre inside what is left, so the lattice does not read as a grid.
    float2 room = (cell * 0.5f - it.hext);
    // sp.z is WORLD-anchored: sp.z = (wp.z + gCloud3.x) * scale * vertScale, where gCloud3.x is the camera's
    // world z. gCloud0.x/y (zTop/zBot) are CAMERA-RELATIVE feet, so the slab's world centre needs gCloud3.x
    // added back before it can be compared with sp.z at all. Without it the instances sat camZ*scale = ~3 noise
    // units off -- two whole cloud-heights -- while the march is bounded to the slab, so it met nothing and the
    // sky came out empty with the library loading perfectly.
    it.centre = float3((cid + 0.5f) * cell + float2(h2 * 2.0f - 1.0f, h3 * 2.0f - 1.0f) * room,
                       ((gCloud0.x + gCloud0.y) * 0.5f + gCloud3.x) * gCloud1.z * gCloudDiag.w);
    // Artscout - 2026 (#13): the turn is QUANTIZED to 4 cardinal steps, and it is not a simplification -- it is
    // what makes the light cache possible at all.
    //   A cache in MODEL space stores tau for ONE sun direction expressed in that model's frame. Two instances of
    // the same model turned differently see the sun from different sides, so they cannot share an entry: the
    // cache is per (model, turn), and a continuous yaw means an unbounded number of them. Four turns = 4 entries
    // per model = 48 for the library, which fits in one atlas. The variety that costs is paid back elsewhere --
    // hext already spans 1.7x and the centres are jittered, so 12 models do not read as 12.
    //   yaw is DERIVED from rot on purpose. If the two were computed independently the shape would be turned by
    // one angle and lit as though turned by another, and nothing about the picture would say so.
    it.rot   = floor(h4 * 4.0f);
    it.yaw   = it.rot * 1.5707963f;
    // Artscout - 2026 (#13): the MODEL is placed on a lattice, not by hash -- deliberately. A hash gives ~1/count
    // (~2.4%) chance that two ADJACENT cells draw the same model, which reads as "two identical clouds side by
    // side". You cannot get both "random" AND "never adjacent-identical" from a stateless function: a hard
    // guarantee is necessarily periodic. So make it periodic with a period LARGER than the view: model = (a*cx +
    // b*cy) mod count. For count = 41 (prime), a = 9, b = 19 give every one of the 8 neighbours a DIFFERENT model
    // (a, b, a+b, a-b are all non-zero mod 41), the nearest repeat 6.08 cells (~15 miles) away, and a neighbour
    // index that jumps >= 9 -- so neighbours are usually a different cloud, not the next one in the family. Size
    // (hext), turn (rot) and whether a cell holds a cloud at all stay hash-random, so the sky still reads organic;
    // only the TYPE follows the hidden lattice, and its 41-cell period (~100 miles) never repeats within one view.
    // (a, b tuned for count = 41; if the library is re-baked to a different count, pick a, b coprime to it and to
    // a +- b, else adjacent twins come back.)
    float cnt = max(gCloudLib.x, 1.0f);
    it.model = fmod(9.0f * cid.x + 19.0f * cid.y + cnt * 65536.0f, cnt);   // +cnt*K keeps fmod >= 0 for -cid
    return it;
}

// Artscout - 2026 (#13): local [-1,1]^3 + (model, turn) -> the light atlas.
// Layout: the 4 turns are a 2x2 tile in XY, the models run down Z --> 128 x 128 x (64*count).
// Stacking all 48 down Z alone (the model atlas's layout) would need 64*4*12 = 3072, and D3D12 caps a Texture3D
// dimension at 2048. That cap is also why the model atlas at 128^3 tops out at 16 models; worth knowing before
// the library grows.
//   Trilinear bleeds half a texel across the tile seams, and unlike the model atlas the value AT a seam is not
// zero (tau on the cube's face is a full path through the cloud). It is still safe, and by measurement not by
// hope: the converter fits each cloud to FILL 0.92, so the cloud stops 2.56 texels short of the face at 64^3,
// and we only ever look tau up where there IS cloud. 2.56 >> 0.5.
// Artscout - 2026 (#13): both atlases (the model library and the light cache) tile their MODELS across XY, not
// down Z, because Z alone cannot hold them: a Texture3D dimension caps at 2048, and at 41 models the old Z stack
// wanted 128*41 = 5248 (library) and 96*41 = 3936 (cache). The tiling is a pure function of the model COUNT --
// tx = ceil(sqrt(count)) columns, ty = ceil(count/tx) rows -- so the C++ loader, this shader and the CS all derive
// the SAME grid from gCloudLib.x with no extra cbuffer field to drift. For 41: 7x6, so the library atlas is
// 896x768x128 and the cache is 1344x1152x96, both inside the cap. Perfect squares give an exact float sqrt, and
// non-squares are far from any ceil boundary, so C++ (double) and HLSL (float) never disagree for count <= 64.
void CloudLibTiles(float count, out float tx, out float ty)
{
    float c = max(count, 1.0f);
    tx = ceil(sqrt(c));
    ty = ceil(c / max(tx, 1.0f));
}

float3 CloudLightUvw(float3 local, float model, float rot)
{
    float tx, ty; CloudLibTiles(gCloudLib.x, tx, ty);
    float3 t  = saturate(local * 0.5f + 0.5f);
    float2 rc = float2(fmod(rot, 2.0f), floor(rot * 0.5f));       // turn -> its 2x2 sub-tile
    // Each MODEL owns a 2x2 block of turns; models tile across XY. Z holds one model's local depth, no stacking.
    float2 tile = float2(fmod(model, tx), floor(model / tx)) * 2.0f + rc;
    return float3((tile + t.xy) / float2(tx * 2.0f, ty * 2.0f), t.z);
}

// Distance from sp to the wall of its own cell, in noise units. This is what an EMPTY cell reports: there is
// nothing in here, so the trace may run to the wall and no further -- the NEXT cell gets to decide for itself.
// Stepping a fixed fraction of the cell instead (which is what I first wrote) OVER-steps whenever the sample
// sits near a wall, and an over-stepping sphere trace tears holes in whatever it jumped over.
float CloudCellWallDist(float3 sp, float cell)
{
    float2 f = frac(sp.xy / max(cell, 1.0e-3f)) * cell;
    float2 d = min(f, cell - f);
    return max(min(d.x, d.y), 0.0f);
}

// Artscout - 2026 (#13): the same question, asked of a RAY instead of a point: how far along rd until this
// sample leaves its cell? Returns FEET, and it is exact -- a DDA, not a bound.
//
// CloudCellWallDist above answers for a point, so it must report the nearest wall in ANY direction. That is
// correct and useless: a ray running nearly PARALLEL to a wall sits a few feet from it for its whole length, so
// the radius stays ~0, the step collapses to the DT floor, and the 400-step budget dies in the near field.
// Measured, marching empty air only:
//     azimuth off the wall     reach with the point radius       reach with this
//        0.0 deg               27,778 ft  (all 400 steps)        173,330 ft (13 steps)
//        0.2 deg               29,489 ft  (all 400 steps)        173,331 ft (13 steps)
//        1.0 deg              160,020 ft  (323 steps)            160,021 ft (12 steps)
// A cell wall is a VERTICAL plane (cells are 2D columns, unbounded in z), and the rays that graze one form a
// vertical wedge -- so the budget runs out along a thin VERTICAL strip of pixels and everything past ~8 km in it
// vanishes. That is the "transparent vertical rectangle" seen both in a roll and on approach, and why every one
// of these artifacts is vertical and never horizontal.
//   Over-stepping is the other failure and this does not trade one for the other: 40k random rays, counting how
// often the trace jumped clean over a cloud --  own-box only: 5439/40000, worst 12,666 ft (the tear this
// replaced);  point radius: 195, worst 66 ft;  this: 3073, worst 66 ft. Both clamps bottom out at the SAME 66 ft,
// which is the DT floor landing just inside a box -- not a skip (the box is the bound; the cloud is deeper in),
// and unavoidable for any minimum step.
float CloudCellExitFt(float3 sp, float3 rd, float cell)
{
    float2 v   = rd.xy * gCloud1.z;                    // d(sp.xy) per foot travelled along rd
    float2 cid = floor(sp.xy / max(cell, 1.0e-3f));
    float2 tw  = float2(1.0e30f, 1.0e30f);
    // step(0,v) picks the wall we are HEADING for: the far one going +, the near one going -.
    float2 wall = (cid + step(0.0f, v)) * cell;
    if (abs(v.x) > 1.0e-12f) tw.x = (wall.x - sp.x) / v.x;
    if (abs(v.y) > 1.0e-12f) tw.y = (wall.y - sp.y) / v.y;
    return max(min(tw.x, tw.y), 0.0f);                 // parallel to an axis' walls -> never crossed -> 1e30
}

// Distance to the model's box, in MODEL units. Outside the box this is exact and conservative -- the box
// contains the cloud, so the distance to it can never overshoot the distance to the cloud. That is what keeps
// the sphere trace valid in the empty air between instances, where there is no baked SDF to read at all.
float CloudSdBox(float3 p)
{
    float3 q = abs(p) - 1.0f;
    return length(max(q, 0.0f)) + min(max(q.x, max(q.y, q.z)), 0.0f);
}

// sp -> the instance's model space. Returns false when there is no cloud here.
bool CloudToModel(float3 sp, CloudInst it, out float3 local)
{
    float3 rel = sp - it.centre;
    float  cs = cos(-it.yaw), sn = sin(-it.yaw);
    rel.xy = float2(rel.x * cs - rel.y * sn, rel.x * sn + rel.y * cs);
    local = rel / max(it.hext, 1.0e-4f);
    return it.valid;
}

// Sample the atlas. Models tile across XY (see CloudLibTiles), so the model index picks a tile and z is the
// model's own local depth. Trilinear bleeds one texel across a tile seam; harmless -- the converter's uniform fit
// (FILL 0.92) leaves margin, so both sides of every seam are empty air, in XY now exactly as it was in Z.
float4 CloudModelSample(float3 local, float model)
{
    float tx, ty; CloudLibTiles(gCloudLib.x, tx, ty);
    float3 t = local * 0.5f + 0.5f;
    float2 tile = float2(fmod(model, tx), floor(model / tx));
    return gCloudNoise.SampleLevel(gSampNoise, float3((tile + t.xy) / float2(tx, ty), t.z), 0.0f);
}

// The ONE place the light cache is read. Returns the density integral in FEET-density in both modes --
// r = toward the sun, g = straight up -- so every consumer applies the same extinction and there is one meaning.
//   It is a function and not two call sites because this file has twice grown a debug view that computed a
// quantity the shader no longer had, and a debug view that lies is worse than none. Mode 6 shows exactly what
// the lighting eats, by construction.
float2 CloudLightFetch(float3 spM)
{
    if (gCloudLib.x > 0.5f)
    {
        CloudInst it = CloudInstanceAt(spM);
        float3 local;
        if (!CloudToModel(spM, it, local)) return float2(0.0f, 0.0f);
        // The cache holds the integral per LOCAL unit; instances differ in size, so scale by this one's.
        return gLightCache.SampleLevel(gSampNoise, CloudLightUvw(local, it.model, it.rot), 0.0f)
               * (it.hext / max(gCloud1.z, 1.0e-9f));
    }
    // No library: the procedural bake, whose field really does tile with period 16 in every axis -- so the
    // world box the CS bakes IS the whole field and this lookup is exact.
    return gLightCache.SampleLevel(gSampNoise, spM * (1.0f / CLOUD_NOISE_PERIOD), 0.0f);
}

float4 CloudNoise4(float3 p, float lod)
{
    // lod 0..0.75 maps to mip 0..3: one mip == one octave dropped, which is what lod*4 counted before.
    return gCloudNoise.SampleLevel(gSampNoise, p * (1.0f / CLOUD_NOISE_PERIOD), lod * 4.0f);
}

float CloudFbm(float3 p, float lod) { return CloudNoise4(p, lod).r; }

// Artscout - 2026 (#13): the erosion detail, from the volume's OWN high-frequency channels. It used to be
// CloudFbm(sp * 3.0), i.e. the SHAPE channel re-sampled at 3x -- which lands 2.7 texels per Worley cell, below
// Nyquist, so it returned ALIASING and not detail. Turning CloudErode up only amplified the alias: measured, the
// clouds thinned and grew noisier and no structure appeared. G/B/A are Worley baked at 2x/4x/8x, each at its own
// resolution, weighted the standard way. Comes free with the shape fetch -- one sample where there were two.
float CloudDetailFbm(float4 n)
{
    return saturate(n.g * 0.625f + n.b * 0.25f + n.a * 0.125f);
}

// Artscout - 2026 (#13): THERE IS NO LOD. This returns 0 -- always mip 0 -- and that is not a stopgap, it is
// what both reference implementations do. Neither Mukherjee (the C port we followed) nor Heckel selects a mip at
// all; both march at a CONSTANT step and let the sample rate resolve the field. Heckel's own words: "no explicit
// LOD/mip handling".
//   The LOD machinery here was MINE, invented to patch a geometric step that outran the noise, and it caused
// every unexplained cloud fault of the day:
//     * "clouds constantly change shape"     -- the mip came from the RAY'S SPAN, so it differed per pixel and
//                                               per frame for the same point in the world
//     * "appear from nowhere / vanish"       -- once the march was anchored to distance the mip became a
//                                               function of range, and a box-filtered mip crushes VARIANCE,
//                                               which is exactly what coverage lives on: far = blurred = no
//                                               cloud, approach = sharp = it materializes
//     * two attempts to fix it (equalize every mip, cap the ceiling) each emptied the sky, because I was tuning
//       a mechanism that should not exist -- my model described a construction the references do not have.
//   Aliasing is handled the way they handle it: sample finely enough (constant step ~1/12 of a noise feature)
// and dither the start per pixel. The remaining grain is what temporal reconstruction is for -- their blue noise
// varies per FRAME (uFrame%32), which averages the error away instead of destroying the field to hide it.
//   Kept as a function so the call sites stay put and the diff stays readable.
float CloudLod(float dt) { return 0.0f; }

// Density at a point given CAMERA-RELATIVE coords (see the gCloud0 note). Returns 0..1.
// cov/topF come from the caller: they are per-COLUMN maps (weather + top height) and were being rebuilt inside
// EVERY call -- including the four sun taps, which multiplied the cost by five for nothing.
// lite = skip the erosion detail. The sun march only needs roughly how much cloud lies toward the sun, and its
// four coarse 220 ft taps cannot resolve erosion anyway.
// Artscout - 2026 (#13): aux returns what the LIGHTING needs and the density alone cannot say --
//   aux.x = the decoded SDF at this sample, in noise units (negative = inside a cloud, and how deep);
//   aux.y = the raw dimensional profile, before erosion and sharpening.
// Both are free here (the volume is already sampled) and both are what their ComputeMultipleScattering takes.
// This is also the first use of the R channel since the profile moved to G: we have been baking a distance
// field every launch and reading nothing out of it.
float CloudDensity(float3 wp, float lod, float cov, float topF, bool lite, out float2 aux)
{
    aux = float2(6.0f, 0.0f);   // far outside any cloud, no profile -- HLSL: every path must write an out param
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
    // Artscout - 2026 (#13): the DETAIL volume gets an UNDISTORTED position. gCloudDiag.w (CloudVertScale,
    // 0.30) squashes the NVDF's z lookup, so the volume stretches 3.3x vertically -- a leftover of the days when
    // the shape was a threshold on noise and clouds had to be pulled wide by hand. It costs the NVDF a 212 m
    // vertical texel against its 63.5 m horizontal one, which is why the staircase is worst on a cloud's BASE.
    //   And I fed that same squashed sp to the up-rez volume last commit, which was simply a bug: that volume is
    // WORLD-space by definition, and stretching it made its z cells 83 m -- COARSER than the horizontal texel it
    // exists to break up. The erosion meant to cut the step was bigger than the step.
    //   Fixing the sampling, not the squash: the squash also sets the cloud's aspect, and changing that is a
    // shape decision to take with a screenshot, not folded into a bug fix.
    float3 spd = float3((wp.xy + gCloud1.xy) * gCloud1.z, (wp.z + gCloud3.x) * gCloud1.z);
    float  n  = CloudFbm(sp, lod);

    if (gCloud3.y > 0.5f)
    {
        // CUMULUS -- a DIRECT PORT of the reference implementation (Jaysmito Mukherjee 2023, MIT; the
        // Lague/Schneider formulation), ported whole rather than a piece at a time. Every earlier attempt here
        // was a home-grown variant, and each one cost a round of tuning to disprove. Its sampleDensity is:
        //
        //     heightGrad = saturate(remap01(h, 0, gMin)) * saturate(remap01(h, 1, gMax));  // gMin .4, gMax .6
        //     heightGrad = pow(heightGrad, heightFactor);                                   // 2.0
        //     density    = dot(shapeNoise, normalizedWeights) * heightGrad;                 // weights (1,1,1,1)
        //     density    = max(0, density - threshold);                                     // 0.65
        //     if (density > 0) density *= (1 - detail)^3;
        //     return density * densityFactor;
        //
        // THREE structural differences from what we had, and they are why ours read as a sheet, not cumulus:
        //
        //  1. HEIGHT MULTIPLIES THE NOISE, BEFORE THE THRESHOLD. We thresholded first and scaled by height
        //     after -- so a cloud EXISTED wherever the noise passed, through the WHOLE slab, and height only
        //     dimmed it: every column ran floor to ceiling, i.e. a lumpy sheet. Multiplying first means
        //     noise*grad > thr <=> noise > thr/grad: near the top and bottom the effective threshold runs away
        //     and there is NO CLOUD there at all. The shape gets carved by height, which is where the domes and
        //     the flat bases come from.
        //     (The old comment here forbade "height-modulated threshold" as the cause of stacked discs. That is
        //     mathematically this same thing -- and the reference does it and works. Given two measurements
        //     already turned out to be contaminated today, I trust the working reference over that note.)
        //  2. EROSION IS A CUBED MULTIPLY, not a 10% subtract. detail 0.5 -> density x0.125. Ours could remove
        //     at most 10% and could never carve cauliflower.
        //  3. SHAPE IS ALL FOUR CHANNELS, dot(rgba, weights) -- we read only .r while baking the other three.
        //
        // Trade taken knowingly: `cov` is a THRESHOLD again (as in the reference), not the exact sky fraction
        // the equalizer guaranteed for the R channel alone -- dot()ing four channels re-mixes the distribution.
        float h  = saturate((gCloud0.y - wp.z) / max(gCloud0.y - gCloud0.x, 1.0f));
        float hh = saturate(h / max(topF, 0.15f));                 // per-column top (our weather map)
        const float gMin = 0.4f, gMax = 0.6f;
        float heightGrad = saturate(hh / gMin) * saturate((1.0f - hh) / (1.0f - gMax));
        heightGrad = heightGrad * heightGrad;                       // == pow(grad, heightFactor=2)

        // The LIBRARY path: sp -> cell -> instance -> model space -> one atlas fetch. gCloudLib.x is the model
        // count; 0 means no library is installed and the procedural tiling bake is bound instead, so both live
        // side by side and the fallback is exact.
        //   This said gCloudLight.z, which is where the count lived before it got its own float4 -- and .z is the
        // tonemap EXPOSURE now. A comment naming the wrong register is how the next reader takes an occupied
        // slot, which is a mistake this file has already paid for three times.
        float4 n4; float kSdfMin, kSdfMax;
        if (gCloudLib.x > 0.5f)
        {
            CloudInst it = CloudInstanceAt(sp);
            float3 local;
            if (!CloudToModel(sp, it, local) || CloudSdBox(local) > 0.0f)
            {
                // No cloud here, or outside this instance's box. NOTHING STEPS BY THIS any more -- CloudMarch
                // runs its own sphere trace (it has the ray, so it can use the exact DDA cell exit; see
                // CloudCellExitFt) and only calls this once it is already INSIDE, and the CS light march steps a
                // fixed LC_STEP. So aux here is written purely because HLSL requires every path to write an out
                // param; its one live consumer is CloudMultiScatter, which by construction never sees this branch.
                //   Kept as the honest point-query answer rather than a sentinel, so that if something does start
                // reading it, it reads a conservative distance and not a lie.
                float dW = CloudCellWallDist(sp, gCloudLib.w);
                aux = float2(it.valid ? min(CloudSdBox(local) * it.hext, dW) : dW, 0.0f);
                return 0.0f;
            }
            n4 = CloudModelSample(local, it.model);
            // The library carries its own encode range (the converter measured it across all 12 models), and the
            // sdf it stores is in MODEL units -- scale to noise units, or the sphere trace steps by the wrong
            // ruler and tears holes.
            kSdfMin = gCloudLib.y; kSdfMax = gCloudLib.z;
            aux = float2((n4.r * (kSdfMax - kSdfMin) + kSdfMin) * it.hext, n4.g);
        }
        else
        {
            n4 = CloudNoise4(sp, lod);                              // R = encoded SDF, G = profile, B/A = detail
            kSdfMin = -2.0f; kSdfMax = 6.0f;                        // must match CLOUD_SDF_MIN/MAX in the bake
            aux = float2(n4.r * (kSdfMax - kSdfMin) + kSdfMin, n4.g);
        }
        const float4 kW = float4(0.25f, 0.25f, 0.25f, 0.25f);       // detail weights (1,1,1,1), normalized
        // R already IS the reference's dot(shapeNoise, weights) -- combined AND equalized at bake time. Doing
        // that dot here (as the reference does, over its own texture) averaged four channels and crushed the
        // variance: shape sat near 0.5, shape*heightGrad peaked ~0.5, and nothing cleared the 0.65 threshold --
        // no clouds at all. Baked and equalized, R is uniform, so `1 - cov` is a threshold that means what it says.
        // G is the DIMENSIONAL PROFILE, baked as theirs: a 12-octave billow fbm thresholded by the SDF distance
        // (see the bake). Deriving it from the SDF here -- which is what I did -- gives a smooth blob, because an
        // SDF of spheres IS smooth. The cauliflower is a NOISE isosurface; the SDF only says roughly where.
        // R still holds the encoded SDF, for the sphere-trace that comes next.
        float shape = n4.g;
        // Artscout - 2026 (#13): SIGNED noise ADDED to a smooth field -- Heckel's construction, and the reason
        // his clouds read as cloud while ours read as blobs:
        //     float scene(vec3 p) { return -sdSphere(p, 1.2) + fbm(p); }   // fbm is SIGNED, -1..1. No threshold.
        // His base is a smooth gradient (1.2 at the core, 0 at the rim) and the noise DISPLACES ITS ISOSURFACE.
        // That is what a soft, billowy, gradually-fading edge is made of.
        //   Ours thresholded noise instead -- max(0, shape*grad - thr) -- which yields islands with a hard-ish
        // boundary, and then multiplied by (1-detail)^3, which only eats density INWARD from wherever d was
        // already positive. A multiply can carve a surface; it cannot move one. That is the difference between
        // "cloud" and "lump", and no amount of coverage/erode/scale tuning crosses it.
        //   `base` here is our smooth field: shape*heightGrad against the coverage threshold, kept SIGNED (no
        // max(0)) so the detail can push the surface both ways. Amplitude ratio follows his: his fbm reaches
        // ~0.885 against a base range of 1.2 (~0.74); ours is ~0.74 of cov.
        //   And I was WRONG about the noise itself: I declared value noise "fog, never a cloud" and rebuilt the
        // bake on Worley to prove it. Heckel's cloud -- the one we are chasing -- is PURE value noise. The
        // billows never came from Worley; they come from this construction.
        // BACK to the form that produced the good clouds (18:24): threshold, then CUBED erosion. I replaced it
        // with Heckel's signed-add because his cloud is built that way -- but his base is an SDF SPHERE, a smooth
        // radial gradient with a dense core, and ours is a noise field times a height ramp. The signed add
        // displaces a surface that HAS a well-defined core; against our base it just drowns the shape, which is
        // the flat slabs. Their construction needs their base, and we do not have one.
        // NO THRESHOLD. R is the dimensional profile of a PLACED cloud -- it IS the shape, with a core and a
        // soft rim, exactly as their NVDF's .g is. Thresholding it would be re-deriving a shape from a shape.
        //   Coverage is now a weather MASK on whole clouds (does this column have one at all), not a knife
        // through a noise field, which is what it was pretending to be and why it never behaved.
        // NO heightGrad. The SDF seeds already carry the cloud's vertical extent -- they are 3D primitives, not
        // a 2D field needing a profile stretched over it. Multiplying by heightGrad shaped the cloud VERTICALLY A
        // SECOND TIME, from a band that knows nothing about where the seeds actually are: the slab is 2.4 noise
        // units at Thick 8000 / Scale 0.0003, the seed layers sit at z~2..6 and ~10..14, and wherever those two
        // independent vertical shapes failed to overlap the answer was zero. That is the empty sky, with the bake
        // itself reporting a healthy 7.1% occupancy.
        //   Their NVDF volume IS the cloud layer; there is no separate height ramp in Nubis at all.
        // cov is NOT applied here any more. CloudColumnMaps builds it from CloudFbm(...).r -- and R is no longer
        // a uniform noise field (mean 0.5, which the weather map was written against) but the SDF PROFILE, mean
        // 0.023 and zero almost everywhere. So wm ~ 0 -> patch = 0 -> cov = 0 -> d = shape*0 = 0 over the entire
        // sky. The map was starving on a channel I replaced underneath it, which is why the bake reported a
        // healthy 7% occupancy while nothing rendered.
        //   Same lesson as heightGrad an hour ago: both are leftovers of the threshold scheme. In Nubis the SEEDS
        // are the weather -- where a cloud is, is decided by placing one, not by masking a noise field. Coverage
        // will come back as a seed-density control on the CPU side, which is what it should always have been.
        float d = shape;
        if (lite) return d;                                        // sun taps: erosion is below their resolution
        if (d <= 0.0f) return 0.0f;

        // Detail from the DETAIL channels only, on the reference's own weights (worleyFBM: .625/.25/.125).
        //   It was dot(nd, 0.25) -- an equal-weight average of all FOUR channels. That is the same
        // variance-crushing mistake I found an hour ago on `shape` and wrote a commit about: averaging four
        // channels flattens the result toward its mean, so `det` came out nearly constant 0, displaced nothing,
        // and since the (1-detail)^3 carve was removed in the same change we lost the relief and gained nothing.
        // That is the "lost volume". And nd.r is the SHAPE channel -- it never belonged in a detail sum at all.
        // NUBIS 3 (Cumulus, rubenaryo -- a DX12 engine, our API and our scale, unlike the two browser demos).
        // Their GetUprezzedVoxelCloudDensity, in order:
        //     uprezzed = ValueErosion(dimensionalProfile, noise_composite);   // (base-erosion)/(1-erosion)
        //     uprezzed *= pow(densityScale, 4);
        //     uprezzed  = pow(uprezzed, lerp(0.3, 0.6, powered_density_scale));   // <-- SHARPEN
        //
        // The last line is the "fill" that has been missing all day, and it is not cosmetic: pow(0.2, 0.3) = 0.62,
        // pow(0.05, 0.3) = 0.42. Our density runs 0.03..0.35 and we fed it RAW to sigma. Theirs lifts the same
        // 0.2 to 0.62 -- a thin cloud becomes a full one on that one line. No coverage/thickness/gain value
        // substitutes for it, which is why none of them ever worked.
        //   ValueErosion is also exactly the remap we HAD and I threw away this afternoon for Heckel's signed
        // add. It was right. Two references now agree on it.
        // THE UP-REZ. Their GetUprezzedVoxelCloudDensity, verbatim:
        //     wispy_noise           = lerp(noise.r, noise.g, dimensionalProfile);
        //     billowy_type_gradient = pow(dimensionalProfile, 0.25);
        //     billowy_noise         = lerp(noise.b * 0.3, noise.a * 0.3, billowy_type_gradient);
        //     noise_composite       = lerp(wispy_noise, billowy_noise, detailType);
        //     uprezzed              = ValueErosion(dimensionalProfile, noise_composite);
        //
        // This is what makes the surface, and until now we did not have it. Their NVDF is a Minecraft blob --
        // 85 m cloud at a 7.8 m voxel, one voxel a ninth of the frame when it fills the screen -- and cumulus.png
        // is smooth because EVERY silhouette pixel is decided right here by metre-scale noise from a separate
        // world-space volume. Ours was eroded by two Worley bands out of the NVDF's own texture, at the NVDF's
        // own tiling: an erosion the size of the voxel, which can move a step but never break one. That is the
        // staircase, and no amount of shape or lighting work was going to touch it.
        //   Note the noise is sampled at the world position on its OWN tile (CLOUD_DETAIL_U), not on the NVDF's:
        // that is the whole point of it being a second volume.
        //   NB their mip is `0.0f // TODO: plug in distance-based mip` -- they have no LOD either, and get 270
        // fps. So mip 0, and the dither carries the undersampling, exactly as it does for them.
        float4 nd  = gCloudDetail.SampleLevel(gSampNoise, spd * (1.0f / CLOUD_DETAIL_U), 0.0f);
        float  prof = aux.y;
        float  dtyp = saturate(n4.b);                                     // detailType: billowy <-> wispy
        // Theirs verbatim: NO x0.3 on the wispy branch, only on billowy. I put one here and justified it with
        // "my wispy channels clip at 0 and 1, so det is bimodal" -- then measured the baked channel and it clips
        // 0.0% of the volume (range [0.030, 1.001]). I invented the mechanism to fit the symptom without
        // checking it, and the x0.3 only starved the erosion until the naked SDF showed through.
        //   The "popcorn" it was meant to cure was also judged against sun == 1 (the light cache was dispatching
        // a zeroed cbuffer at the time), i.e. against a cloud pinned at maximum brightness where no density
        // variation can show at all. Both the observation and the fix were built on nothing.
        float  wispy = lerp(nd.r, nd.g, prof);
        float  btg   = pow(max(prof, 1.0e-5f), 0.25f);
        float  billowy = lerp(nd.b * 0.3f, nd.a * 0.3f, btg);
        float  det = lerp(wispy, billowy, dtyp) * saturate(gCloudDiag.x);
        float  denom = max(1.0e-4f, 1.0f - det);
        d = saturate((d - det) / denom);                       // ValueErosion, theirs verbatim
        // densityScale is per-voxel authored data in their NVDF; we have no such channel, so it is 1 (their
        // pow(1,4) = 1) and the sharpen exponent lands at their 0.6 end. Stated rather than silently dropped.
        return pow(max(d, 1.0e-5f), 0.6f);
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
    // Artscout - 2026 (#13): patch is a MASK, not a multiplier -- ramp at the EDGE, full strength inside.
    // CloudDensity's own comment has said since the weather map went in that the map and coverage "must not
    // multiply" (the map picks WHERE weather is, coverage says how much sky it fills THERE), but the code kept
    // multiplying anyway: the ramp spread over HALF the amount band, so patch only reached 1 where
    // wm >= 1-amt+amt*0.5 -- at amt=0.30 that is wm >= 0.85, and wm runs mean 0.498 / sd 0.219, so ~6% of sky.
    // Over the other ~94% coverage was silently scaled DOWN, starving saturate(nb - (1-cov)) into crumbs.
    //   This is why NO combination of CloudScale / CumulusCoverage / CumulusThick could produce proper clouds:
    // every one of them was fighting a coverage the patch had already divided away. Lowering coverage made it
    // worse (crumbs of crumbs) and enlarging the grain changed nothing, because the starvation is multiplicative
    // and grain-independent. Two dead-end tuning rounds came from this one line.
    //   A fixed narrow edge (in wm units, NOT scaled by amt) keeps the soft patch boundary the ramp was for,
    // while the interior gets coverage at full strength -- which is what "a patch holds real clouds rather than
    // a haze of crumbs" was supposed to mean.
    const float kPatchEdge = 0.06f;
    float  patch = smoothstep(1.0f - amt, 1.0f - amt + kPatchEdge, wm);
    float  hmap  = CloudFbm(float3(spxy * 0.35f, 0.0f), lod);
    return float2(gCloud0.z * patch, 1.0f - gCloudDiag2.z * (1.0f - hmap));
}

// Artscout - 2026 (#13): their ValueRemap -- saturate into [0,1] over the input range, then lerp the output.
float CloudValueRemap(float x, float inMin, float inMax, float outMin, float outMax)
{
    return lerp(outMin, outMax, saturate((x - inMin) / (inMax - inMin)));
}

// Artscout - 2026 (#13): MULTIPLE SCATTERING, their ComputeMultipleScattering:
//     depthTerm = ValueRemap(sdfDistance, -128, 0, 0.05, 0.25);
//     factor    = ValueRemap(sunDot,        0, 0.9, 0.25, depthTerm);
//     ms_volume = dimensionalProfile * exp(-tauSun * factor);
//     return secondaryColor * ms_volume * secondaryStrength;
//
// This is the term the whole layer has been missing, and this shader's own comments have been apologising for
// its absence all along ("real clouds are bright white from MULTIPLE scattering, which we do not model at all --
// so hg is normalized to the side lobe to compensate, which is a fudge, mine"). SunGain 45 was the dial on that
// fudge. Here is the thing it was standing in for.
//   The point is `factor`: it DIVIDES the optical depth by 4..20 before the exponential, so where direct light
// has died (exp(-tau) -> 0) this term still carries 60-90% of its value. That is what puts a GRADIENT in the
// shadowed side instead of a flat fill. And it is deepest-first: far inside the cloud (sdf very negative) the
// factor drops to 0.05, because light that gets that far has scattered many times and no longer cares about
// direction. Near the surface it is 0.25.
//   sdf is in NOISE UNITS here, not their metres: their -128 sits ~3 cloud radii deep against an 85 m cloud, and
// our SDF is encoded down to CLOUD_SDF_MIN = -2 against a 2.86-unit cloud. Same ratio, our scale.
// Artscout - 2026 (#13): multiply-scattered sunlight IS SUNLIGHT, so it carries the sun's scale -- gCloudDiag.y,
// the same SunGain the direct term uses -- and gCloudLight.y (g_fCloudMsStrength) is a FRACTION of it.
//
// It was not, and that was the last of the three unit mismatches in this shader (after LDR haze lerped into HDR
// radiance, and the fog inverse still inverting Reinhard). Direct carried 255 and MS carried 2.0: not a balance
// to tune, two different units. The consequence was measured on a pair of frames --
//     looking AT the sun   : cloud luminance 236..249
//     sun BEHIND the camera: cloud luminance 106..130
// -- which is backwards. A cumulus is bright from EVERY angle; the backlit one gets a blazing rim and a slightly
// deeper body, the frontlit one is evenly white. Ours hung entirely on the phase, whose range is 51:1 (hg 1.0 at
// the sun, 0.0195 away from it), because the only term that does NOT depend on the phase was 128x too small to
// matter. Same root as "the unlit side goes too dark": nothing holds the floor up.
//
// Cumulus ships the same 255/2.0 pair, and it works there for a reason we must NOT copy: their direct term
// carries alpha TWICE -- segmentScatter = 1-exp(-sigma*stepSize) inside ComputeDirectLighting, and then
// `lighting * alpha * transmittance` outside, while ambient and MS are multiplied once. At their step that is a
// ~11x suppression of direct alone, which accidentally lands 255 next to 2.0. The transfer equation says the
// factor belongs there exactly once (scat += trans * alpha * (direct + ms + amb)), which is what we do. Their
// number is only meaningful with their double-count; porting one without the other is what produced this.
float3 CloudMultiScatter(float3 sunColor, float profile, float tauSun, float cosVS, float sdf)
{
    float depthTerm = CloudValueRemap(sdf,   -2.0f, 0.0f, 0.05f, 0.25f);
    float factor    = CloudValueRemap(cosVS,  0.0f, 0.9f, 0.25f, depthTerm);
    // exp(-tauSun * factor) with factor 0.05..0.25 is the whole point: MS reaches deep into a cloud where the
    // direct term (factor 1.0) is long dead. That is what makes a thick cloud glow instead of going black.
    return sunColor * (profile * exp(-tauSun * factor)) * gCloudLight.y * gCloudDiag.y;
}

// Artscout - 2026 (#13): the per-pixel sun march is GONE -- CS_LightCache does it once per cache voxel and the
// view march reads one texel. It had climbed to 12 geometric steps of CloudDensity, i.e. ~24 texture fetches per
// view sample, and it still could not see a 25 m crevice. Theirs is 128 steps at 1 m and costs the view march a
// single trilinear fetch, because it is amortised. That is the whole answer to "why can't we afford 1 m".

// Artscout - 2026 (#13): the tonemap and its inverse, ADJACENT, because they are one decision and were briefly
// two. The march must put the haze into radiance before mixing it into `lit` (see the aerial-perspective note),
// which needs the inverse; then the accumulated total is tonemapped. If the two ever disagree the haze stops
// landing on the haze colour, and the failure is not a level error but a HUE shift -- each channel comes out
// wrong by a different factor, so every cloud tints toward the sky. That is exactly what happened when the curve
// moved from Reinhard to exponential and the inverse, written two commits earlier and nine hundred lines away,
// silently kept inverting the old one.
//   Keep them together. Change one, the other is right there.
float3 CloudTonemap(float3 L)     { return 1.0f - exp(-L * max(gCloudLight.z, 1.0e-6f)); }
float3 CloudTonemapInv(float3 c)  { return -log(max(1.0f - min(c, 0.98f), 1.0e-3f)) / max(gCloudLight.z, 1.0e-6f); }

float4 CloudMarch(float3 rd, float tMax)
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
    int steps = (int)max(gCloud1.w, 2.0f);
    // CONSTANT step, as both references march. dt is 1/12 of a noise feature (a feature is 1/gCloud1.z feet), so
    // the field is sampled ~12x per feature and simply cannot alias -- which is why neither of them needs a mip.
    // Previous schemes here derived dt from the ray's SPAN (so the same world point sampled differently per pixel
    // -> shape swam) and then from DISTANCE (so the mip fell as you approached -> clouds materialized). A
    // constant step has neither property: a point is sampled identically by every ray, in every frame, forever.
    //   `steps` is a BUDGET. A ray that exhausts it stops; that only bites near-horizontal rays, which are the
    // ones the haze has already dissolved. Empty-space skipping (below) stretches the budget without changing
    // WHAT gets sampled -- the references have no need for it at their scene scale; ours is 160 km.
    // 48 samples per noise feature -- the references' own rate. Heckel marches MARCH_SIZE 0.08 through a box
    // ~10 units tall (125 steps) holding 2-3 billows, i.e. ~40-60 samples across one. We ran 12, then 24; a
    // fly-through costs nothing since the constant step stopped oversampling the near field 11x, so there is no
    // reason to sit short of them. Bayer SPREADS the sampling error, it does not reduce it -- only rate does.
    //   This is the ONLY number of theirs that transfers, and that is the lesson of the whole port: their scene
    // is a box "(6,5,6)" at scale 1.0 -- six of WHAT? Their units are not feet or metres, so threshold / scale /
    // bounds are unmappable, and every attempt to copy one today missed. Samples-per-feature is DIMENSIONLESS,
    // so it carries. Structure transfers; constants do not.
    // Artscout - 2026 (#13): DT is the MINIMUM step now -- the floor of a sphere trace, not the schedule.
    //
    // 48 -> 125 samples per noise feature, i.e. DT 69.4 -> 26.7 ft. NOT a taste adjustment: it is the one ratio
    // that governs the fringe ripple, and it was measured against the reference rather than guessed.
    //
    //   Cumulus, read out of its source:  NVDF 512x512x64 over 4km x 4km x 0.5km  -> model voxel 7.81 m
    //                                     detail noise 128^3 over NOISE_DOMAIN_SIDE_LENGTH 100 m -> texel 0.781 m
    //                                     minStepSize = AUTHORING_TO_WORLD_SCALE (= 1.0)         -> step  1.00 m
    //   Ours (defaults):                  model 128^3 over a ~10,000 ft cloud      -> model voxel 78.1 ft
    //                                     detail 128^3 over CN_DETAIL_U 0.8 units  -> texel      20.8 ft
    //                                     DT at 48/feature                         -> step       69.4 ft
    //
    //          step / detail texel:   theirs 1.28      ours 3.33      <- we sampled the up-rez 2.6x too coarsely
    //
    // The up-rez decides every silhouette pixel (see GetUprezzedVoxelCloudDensity), so the detail texel is the
    // finest thing in the field and IT sets Nyquist -- not the model, not the noise feature. Marching at 3.3
    // texels cannot resolve what the erosion carves, and the per-step white noise then spreads that error over
    // the fringe instead of removing it: exactly the grain along every cloud edge. 1.28 texels = 26.7 ft.
    //   Coarsening the DETAIL to meet the old step was the other way to close the same ratio, and it is wrong:
    // it would put the erosion back at the model's own scale, which "can move a step but never break one" -- the
    // staircase this whole up-rez exists to kill.
    // Artscout - 2026 (#13): 125 -> 150 (~26.7 -> 22.2 ft). Finer than the detail-texel Nyquist that 125 matched,
    // on purpose: the extra samples do not reveal new detail (there is none past Nyquist) but they lower the
    // per-step dither's undersampling, so the residual VR grain drops. Ripple cannot return -- it was an UNDER-
    // sampling artifact, and this only over-samples. Costs ~20% more steps; the fps headroom (120-162 in VR) pays.
    const float DT = 1.0f / max(gCloud1.z * 150.0f, 1.0e-6f);   // ~22.2 ft at CloudScale 0.0003
    float dt = DT;
    float t  = t0;

    // The SDF is in NOISE units and the volume is ANISOTROPIC in z (gCloudDiag.w), so a distance there is not a
    // Euclidean world distance. Divide by the LARGEST of the two rates and the trace stays conservative -- it
    // may under-step, never over-step, which is the only property a sphere trace has to have.
    const float sdfToFt = 1.0f / max(gCloud1.z * max(1.0f, gCloudDiag.w), 1.0e-9f);
    float  trans = 1.0f;
    float3 scat  = float3(0, 0, 0);
    // Ambient = sky light from above; the layer's underside is darker than its top.
    float3 amb   = gFogColor.rgb * gCloud2.w;

    // Artscout - 2026 (#13): STEREO-COHERENT jitter key. The per-step dither below (CloudHash231) breaks the SDF
    // sphere-trace's isosurfaces into grain that a temporal filter is meant to average out (their uFrame%32, which
    // we do not have). It was keyed on the SCREEN PIXEL (i.Pos.xy, passed in), and there is the VR bug: the same
    // world feature lands on DIFFERENT pixels in each eye, so each eye drew an UNCORRELATED white-noise offset ->
    // two different sampled densities -> the stereo brain cannot fuse them and reads the disagreement as a bumpy,
    // unstable surface. That is the "bumps" seen ONLY in VR and invisible in a mono capture -- measured: the flat
    // mirror's local bump amplitude is 1.2/255, i.e. the dither itself, with no dark crevices (contrast 31/255) and
    // no plateaus (the cache-march bench showed 3.5-texel vs 1-texel steps equally smooth). Not the up-rez (erode 0
    // still bumped), not the storage grid (64->96 unmoved), not the atlas seams (measured: 5-texel air collar).
    //   Keying on the WORLD ray direction makes both eyes draw the SAME offset for a feature: rd differs between
    // eyes only by the parallax of a 6.5 cm baseline, which past ~900 ft is under one jitter cell, so the eyes
    // agree and the grain FUSES (and is then what a future TAA/DLSS pass removes cleanly). It still decorrelates
    // neighbouring pixels -- rd sweeps ~2.5 cells per pixel across the screen -- so the mono dither is unchanged.
    int3   jq3    = (int3)floor(rd * 4096.0f);
    uint   jhash  = (uint)(jq3.x * 73856093) ^ (uint)(jq3.y * 19349663) ^ (uint)(jq3.z * 83492791);
    uint2  jitKey = uint2(jhash & 0xFFFFu, (jhash >> 16) & 0xFFFFu);

    [loop] for (int s = 0; s < steps; ++s)
    {
        if (trans < 0.01f) break;   // saturated -- nothing behind this contributes
        if (t >= t1) break;
        // SPHERE TRACE. This is the reference's loop, and it is the last structural piece of it we did not
        // have: sample the SDF, step by the distance it reports, and do NOTHING until it says we are inside.
        //     float sdfDistance = DecodeSdf(sdfTex.r) * AUTHORING_TO_WORLD_SCALE;
        //     march.stepSize = max(sdfDistance, AUTHORING_TO_WORLD_SCALE);
        //     if (sdfDistance < 0.0) { ...density, lighting, accumulate... }
        // Ours ran a CONSTANT step everywhere and guessed at empty space with a `miss` counter and a x6 stride --
        // a heuristic standing in for the exact answer we bake into the R channel every launch and never read.
        // Worse, it evaluated CloudDensity (two volume fetches plus the up-rez) at every sample INCLUDING clear
        // air, just to discover the air was clear.
        float3 wp  = rd * t;
        float3 spS = float3((wp.xy + gCloud1.xy) * gCloud1.z, (wp.z + gCloud3.x) * gCloud1.z * gCloudDiag.w);
        // Artscout - 2026 (#13): their ComputeAdaptiveStepSize -- "take smaller steps near the camera". Theirs:
        //     adaptiveNvdf = max(1.0, max(sqrt(distanceNvdf), EPSILON) * 0.08)   // NVDF units
        // AUTHORING_TO_WORLD_SCALE is 1 in their scene, which makes one NVDF unit exactly their MINIMUM step --
        // so the statement is dimensionless in (step/u, t/u) with u = the minimum step, and 0.08 ports as a pure
        // number in that pairing (it is NOT a length and does not port on its own: sqrt(t) is not dimensionless).
        //   The comment above defends a CONSTANT step at length. That argument was about the MIP -- a mip from
        // distance crushes variance and makes clouds materialize as you close -- and it is still right; CloudLod
        // still returns 0. It does not carry to the STEP: a step schedule changes only how finely the integral is
        // taken, not what the field is, and 1-exp(-sigma*dt) is the step-size-consistent integrator, so it
        // converges rather than drifting. Their live code does this, and it is what makes 26.7 ft affordable:
        // finer than the old flat 69.4 out to 28k ft (where the ripple is), coarser past it (where a cloud is a
        // few pixels and the haze has half-eaten it anyway).
        float  adapt = DT * max(1.0f, 0.08f * sqrt(max(t, 0.0f) / DT));
        float  capFt = 1.0e30f;   // the DDA's exact cell exit, in feet -- only the library path has cells
        float sdfN;
        if (gCloudLib.x > 0.5f)                      // library: the box distance carries the trace between clouds
        {
            CloudInst it = CloudInstanceAt(spS);
            float3 local;
            // +1 ft so we land just PAST the wall: landing exactly on it lets floor() keep us in the same cell
            // for one wasted iteration. It cannot over-step -- the neighbour gets to answer for itself.
            capFt = CloudCellExitFt(spS, rd, gCloudLib.w) + 1.0f;
            if (!CloudToModel(spS, it, local))         { sdfN = 1.0e6f; }   // empty cell: the cap alone carries it
            else if (CloudSdBox(local) > 0.0f)         { sdfN = CloudSdBox(local) * it.hext; }
            else
            {
                float4 m = CloudModelSample(local, it.model);
                sdfN = (m.r * (gCloudLib.z - gCloudLib.y) + gCloudLib.y) * it.hext;
            }
        }
        else
        {
            const float kSdfMin = -2.0f, kSdfMax = 6.0f;   // must match CLOUD_SDF_MIN/MAX in the bake
            sdfN = gCloudNoise.SampleLevel(gSampNoise, spS * (1.0f / CLOUD_NOISE_PERIOD), 0.0f).r
                   * (kSdfMax - kSdfMin) + kSdfMin;
        }
        // Sphere trace, floored by the adaptive step and capped by the cell exit. Inside a cloud sdfN < 0, so the
        // min/max collapse to `adapt` -- which is the whole schedule in one line.
        dt = max(min(sdfN * sdfToFt, capFt), adapt);
        if (sdfN >= 0.0f) { t += dt; continue; }       // outside every cloud: nothing here, and the SDF says how far

        // Per-STEP jitter, theirs (StaticStepJitter(pixel, stepIndex) in [-0.5, 0.5]). Ours jittered only the
        // march's FIRST sample, so every sample after it sat on the same lattice for the whole ray and the dither
        // decorrelated nothing past the entry point. The golden-ratio advance keeps it spread per step as the
        // Bayer keeps it spread per pixel.
        float  jitS = CloudHash231(jitKey, (uint)s) - 0.5f;   // [-0.5, 0.5], per WORLD-RAY (stereo-coherent) AND step
        wp += rd * (jitS * dt);
        float  lodS = CloudLod(dt);                    // detail matched to THIS step's coarseness
        float2 cm   = CloudColumnMaps(wp, lodS);       // per-column maps: ONCE per step, not per call
        float2 aux;
        float  d    = CloudDensity(wp, lodS, cm.x, cm.y, false, aux);

        if (d <= 0.001f) { t += dt; continue; }   // inside the SDF but eroded away

        // DEBUG views (gCloudDiag.z): show the raw quantity at this first sample that has density, flat and
        // unlit, and stop. See the gCloudDiag note in cbRender.
        // Artscout - 2026 (#13): modes 1..6 report a quantity at THIS sample and stop, so they live in the loop.
        // Mode 7 is the accumulated HDR and only exists after the march, so it must NOT be caught here -- it was,
        // by `> 0.5f`, and then swallowed by mode 6's `> 5.5f` below. That is why "debug 7" showed the light cache
        // and not the HDR, and it never once ran the code written for it.
        if (gCloudDiag.z > 0.5f && gCloudDiag.z < 6.5f)
        {
            // MUST mirror CloudDensity exactly. It did not: this still used the old plateau envelope and
            // ignored topF, so the view reported an `env` the shader no longer computes -- and a debug view that
            // lies is worse than none. It already cost a wrong diagnosis once.
            // MUST mirror CloudDensity exactly -- it did not, twice: it still computed the OLD plateau envelope
            // after that was replaced, so it reported a quantity the shader no longer had. A debug view that
            // lies is worse than none, and this one already bought a wrong diagnosis. Mirrors the ported
            // reference algorithm now (heightGrad, baked shape).
            float hRaw = saturate((gCloud0.y - wp.z) / max(gCloud0.y - gCloud0.x, 1.0f));
            float hh   = saturate(hRaw / max(cm.y, 0.15f));                  // cm.y = topF (per-column top height)
            float ev   = saturate(hh / 0.4f) * saturate((1.0f - hh) / 0.4f);
            ev = ev * ev;                                                    // heightGrad, as CloudDensity has it
            float3 sp2 = float3((wp.xy + gCloud1.xy) * gCloud1.z, (wp.z + gCloud3.x) * gCloud1.z * gCloudDiag.w);
            // Artscout - 2026 (#13): the SHAPE, as CloudDensity actually reads it -- `shape = n4.g`, the
            // dimensional profile, BEFORE the up-rez erodes it. This is the instrument for "are the facets in the
            // model or in the light", so it must be the model's own field and nothing else.
            //   It was CloudNoise4(sp2, lod).r, and that was stale twice over: .r is the SDF, not the shape (the
            // comment still said "the BAKED, equalized shape" from a scheme two rewrites back), and under the
            // library gCloudNoise IS the atlas while sp2/CLOUD_NOISE_PERIOD is a tiling coordinate -- so it
            // indexed a stack of models with a number that means nothing there. Exactly the fault mode 6 had, and
            // a debug view that lies is worse than none: I would have measured it and believed the answer.
            float nbv;
            if (gCloudLib.x > 0.5f)
            {
                CloudInst itD = CloudInstanceAt(sp2);
                float3 locD;
                nbv = CloudToModel(sp2, itD, locD) ? CloudModelSample(locD, itD.model).g : 0.0f;
            }
            else
            {
                nbv = CloudNoise4(sp2, lodS).g;
            }
            // Mode 1 packs the three fields into COLOUR CHANNELS -- R=density, G=envelope, B=noise -- so ONE
            // screenshot carries all of them with identical geometry. Switching modes between shots would need a
            // restart, and the view could never be reproduced exactly; separate channels sidestep that entirely.
            // Modes 2/3/4 stay as single-quantity greyscale for when one needs a closer look.
            // R = density, G = heightGrad, B = shape. Mode 5 = the per-column COVERAGE (cm.x) -- the weather
            // patch times CumulusCoverage. If B is healthy and R is black, the threshold is eating everything;
            // if G is a thin band, heightGrad is; if mode 5 is black, the weather map never opened here and no
            // amount of coverage or density can help. That is the one thing no other channel can tell apart.
            if (gCloudDiag.z < 1.5f) return float4(d, ev, nbv, 1.0f);
            // Artscout - 2026 (#13): mode 6 = THE LIGHT CACHE, straight out, at this sample.
            //   R = tauSun (0..5 mapped to 0..1), G = tauVertical, B = the density integral toward the sun / 5000.
            // Black everywhere means the cache is empty -- which is the question I have now guessed at three
            // times running (t5 not bound? dispatch not firing? cbuffer zeroed? UAV format?) instead of simply
            // looking at it. A value that decides every lit pixel and cannot be seen is not debuggable.
            // Through CloudLightFetch, the SAME call the lighting makes -- it cannot report a cache the shader
            // does not read. It reported one for a whole session: this view sampled a world-space volume long
            // after the library had made that lookup meaningless, so "the cache is empty" and "the cache is not
            // where you are looking" were indistinguishable, and I guessed between them four times.
            if (gCloudDiag.z > 5.5f)
            {
                float3 spD = float3((wp.xy + gCloud1.xy) * gCloud1.z, (wp.z + gCloud3.x) * gCloud1.z * gCloudDiag.w);
                float2 lcD = CloudLightFetch(spD);
                return float4(saturate(lcD.r * gCloudLight.x / 5.0f),
                              saturate(lcD.g * gCloudLight.x / 5.0f),
                              saturate(lcD.r / 5000.0f), 1.0f);
            }
            float v = (gCloudDiag.z < 2.5f) ? ev : (gCloudDiag.z < 3.5f) ? nbv
                    : (gCloudDiag.z < 4.5f) ? hRaw : cm.x;
            return float4(v, v, v, 1.0f);
        }

        float sigma = d * gCloud0.w;
        // ONE fetch, where twelve density marches used to be -- theirs does exactly this. gLightCache holds the
        // density INTEGRAL (feet-density), so the extinction is applied here and CloudSunExt stays a live knob.
        // `sp` is a local of CloudDensity, NOT of this function -- rebuild it here from wp, with the same
        // mapping CloudDensity uses (gCloudDiag.w squashes z; see the spd note there).
        float3 spM    = float3((wp.xy + gCloud1.xy) * gCloud1.z, (wp.z + gCloud3.x) * gCloud1.z * gCloudDiag.w);
        // Artscout - 2026 (#13): the cache is keyed on the INSTANCE now, not on a world box.
        //   The world-space cache was a periodic volume: the CS baked sp in [0,16)^3 literally, and the PS read it
        // through a WRAP sampler, which is exactly right for the procedural bake -- that field really does repeat
        // every 16 units in all three axes, so the literal box IS the whole field.
        //   Instances repeat in NOTHING. In XY the cell is 13333*0.0003 = exactly 4.0 units, so the box spanned 4
        // cells while floor(sp.xy/cell) runs unbounded -- a cloud at cell 37 read the entry baked for cell 1. And
        // in Z it was worse than aliased: instance centre.z is one absolute altitude, ~-4.8 units at 16k ft, while
        // the CS baked [0,16) -- so the baked box did not contain a single cloud voxel and the cache came out
        // IDENTICALLY ZERO. tau 0 -> sun = exp(0) = 1 -> nothing in the lighting varies across a cloud. Measured
        // on the frame: p5..p95 spread of 14 levels out of 255, against ~40-60% for a real cumulus, and the flat
        // value matched the prediction for tau=0 at that view-sun angle to within 3%.
        //   So: per (model, turn), in MODEL space -- which is what Cumulus does (their lightCacheTex is read
        // through WorldToNvdfUV, the asset's own frame; their scene is one NVDF, so per-model IS per-scene there).
        //   The cache stores the integral per LOCAL unit, because instances differ in size: scaling by hext here
        // turns it into the same feet-density integral the world path produces, so everything below is unchanged
        // and there is ONE meaning of `lc` rather than two.
        float2 lc     = CloudLightFetch(spM);
        float  tauSun = lc.r * gCloudLight.x;    // direct wants exp(-tau); multiple scattering wants exp(-tau*factor)
        float  sun    = exp(-tauSun);
        // Powder term: the "silver lining" -- edges facing the sun scatter forward and read brighter than
        // Beer alone predicts. gCloudSun.a scales it.
        // Artscout - 2026 (#13): thin cloud must be BRIGHT, not dark. Two things were missing and together they
        // inverted the physics at every cloud edge, outlining it in shadow:
        //   * NO PHASE FUNCTION at all. Cloud droplets scatter strongly FORWARD, which is what makes a thin edge
        //     between you and the sun glow (the silver lining). Henyey-Greenstein is the standard cheap model.
        //     g > 0 = forward-biased.
        //   * POWDER applied unconditionally. It approximates the darkening of thin cloud from multiple
        //     scattering, and it is real -- but only when looking AWAY from the sun (Schneider's "powder sugar"),
        //     and it goes to 0 as d -> 0, so ungated it killed the sun term outright wherever the cloud thinned:
        //     lit collapsed to ambient and every edge got a hard dark rim. Gate it by the view-sun angle so the
        //     lit side keeps its glow and the away side keeps the effect it was written for.
        float  cosVS = dot(rd, gCloud2.xyz);                      // gCloud2.xyz = direction TO the sun
        // g = the anisotropy of the droplets. 0.75 is the published DCS figure (they run 0.7-0.8); it is droplet
        // physics that the reference states outright, not a value to dial by eye. The higher g, the NARROWER the
        // forward lobe -- the silver lining tightens to the rays that nearly graze the sun, instead of washing
        // the whole sunward half of every cloud.
        // Artscout - 2026 (#13): g = 0.75 with saturate(HG * 3) -- their ComputeDirectLighting, verbatim. It was
        // g = 0.30 and the RAW phase, taken from Heckel, and the note below argues at length for preferring his
        // "checkable source" to an article's claim about DCS. That argument is still true and still beside the
        // point: his model has NO multiple-scattering term, so his phase has to carry the whole cloud on its own.
        // Ours now has one. Lifting a constant out of a model that lacks the term we just added is the exact
        // mixing this file keeps warning about -- and Cumulus is the reference that has the term.
        //   The saturate() matters as much as the 0.75: it makes the phase a 0..1 REDISTRIBUTION (0.05 away from
        // the sun, 1.0 into it) instead of an unbounded spike. That spike is why SunGain had to be 45.
        float  g     = 0.75f;
        float  hgRaw = (1.0f - g * g) / (4.0f * 3.14159265f * pow(max(1.0f + g * g - 2.0f * g * cosVS, 1.0e-4f), 1.5f));
        hgRaw = saturate(hgRaw * 3.0f);
        // Artscout - 2026 (#13): NORMALIZE to the side lobe (HG at cos=0), so the phase REDISTRIBUTES light
        // instead of adding it: 1.0 looking across the sun, <1 away from it, >1 toward it. The previous form was
        // `1 + hg*6`, and the 6 was a number I invented -- it peaked at 5.78x toward the sun and simply blew an
        // already-white cloud out. Forward scatter really is ~125x at g=0.75, and LDR has nowhere to put that, so
        // clamp it and say so rather than pretend: the sky is the exposure reference here, not the cloud.
        // RAW phase, as the reference uses it -- no normalization, no clamp. hgRaw at g=0.3 runs 0.033 (away
        // from the sun) to 0.211 (into it), mean 1/4pi. Dividing by the side lobe was MY invention and it lifted
        // everything ~16x, which is where the clipping came from and why I kept re-guessing a coefficient.
        //   g was 0.75, taken from an ARTICLE'S CLAIM about DCS. Heckel's working shader -- the one whose picture
        // we are chasing -- says SCATTERING_ANISO 0.3. I preferred a quote to a checkable source; the source wins.
        //   Note our formulation differs from his and the constants do NOT interchange: his luminance is
        // 0.025 + density*phase where density reaches ~2 (an SDF sphere plus SIGNED noise, no threshold), while
        // our d is 0.03..0.35 after threshold and cubed erosion, and our phase multiplies RADIANCE, not density.
        // Mixing his constants into Mukherjee's density is exactly the trap that cost today. g transfers because
        // it is dimensionless droplet physics; the brightness it loses is paid back by SunGain, below.
        float  hg = hgRaw;
        // Artscout - 2026 (#13): powder as a LERP from 1, so gCloudSun.a (g_fCloudPowder) is a real strength dial.
        // It was `1 - exp(-d * 2 * a)`, which is inverted at the bottom end: a=0 gives powder=0, i.e. the knob's
        // "off" position KILLED the sun term entirely and greyed every cloud out. There was no way to switch the
        // effect off to test it, which is why the dark rims went unexplained for so long.
        //   The rims are this term: at a cloud edge d -> 0, so powder -> 0 and lit collapsed to amb (= fogColor
        // * 0.55, a dark grey) exactly where the cloud is thinnest and should be brightest. The view-sun gate
        // below only saves the side facing the sun; looking away from it the gate is wide open and the rim is
        // back. Floor it so a thin edge keeps most of its light.
        float  powder = lerp(1.0f, 1.0f - exp(-d * 2.0f), saturate(gCloudSun.a));
        powder = lerp(1.0f, powder, saturate(0.5f - cosVS * 0.5f) * 0.6f);   // gate by view-sun angle, capped
        // Artscout - 2026 (#13): gCloud3.z = g_fCloudSunGain -- the sun term had NO gain control at all, while
        // ambient had one, so every overexposure round was me guessing a coefficient instead of handing it over.
        //   It needs one because this term is not physical and cannot be: single-scatter Beer-Lambert makes a
        // DARK cloud (the raw HG phase averages 1/4pi ~ 0.08, since light leaves in every direction and only a
        // sliver reaches the eye). Real clouds are bright white from MULTIPLE scattering, which we do not model
        // at all -- so hg is normalized to the side lobe to compensate, which is a fudge, mine, and about 12x
        // the reference's raw phase. Fudges need a dial, not a hard-coded constant I re-guess every hour.
        // Artscout - 2026 (#13): their three terms -- lighting = direct + ambient + multiple scattering.
        // We had one and a half: direct, plus a CONSTANT ambient. So once exp(-tau) died there was nothing left
        // but that constant, and every shadowed pixel in the frame was literally the same colour. That is the
        // flat dark slabs, and no amount of shape work was ever going to touch them.
        //   Theirs is ambientColor * exp(-tauVertical); ours is still constant. That needs a second optical
        // depth, which they precompute in a light cache. Stated, rather than quietly skipped.
        // Artscout - 2026 (#13): this is HDR. It runs to ~180 looking into the sun and ~0.5 in deep shadow, and
        // the march tonemaps the total at the end -- see CloudMarch. That is not an addition to their model, it
        // is the half of it we did not have: their pipeline is HDR + tonemap (sunIntensity = 255, secondary =
        // 2.0), ours wrote straight to an LDR backbuffer. Which is exactly why "their constants do not transfer"
        // kept being true here and nowhere else: they transfer fine, once there is somewhere to put the range.
        // Ambient is theirs at last: ambientColor * exp(-tauVertical) * strength. It was a CONSTANT, and the
        // note above said so -- "needs a second optical depth, which they precompute in a light cache". It does,
        // and now we have one: tauVertical is channel 2 of the fetch we already did, so it costs nothing. A cloud
        // base is dark because there is a kilometre of cloud between it and the sky, not because of a constant.
        float3 ambL  = amb * exp(-lc.g * gCloudLight.x * 0.25f);
        float3 lit   = gCloudSun.rgb * sun * powder * hg * gCloudDiag.y                 // direct
                     + CloudMultiScatter(gCloudSun.rgb, aux.y, tauSun, cosVS, aux.x)    // multiple scattering
                     + ambL;                                                            // ambient x exp(-tauVert)
        // Aerial perspective, from THIS step's own distance. It has to happen here: the VS's FogF is computed
        // at the backing geometry's vertices, and that geometry is a camera-centred sphere of FIXED radius --
        // every vertex reports the same distance, which says nothing about where the cloud is. Fog the COLOUR
        // only: a distant cloud is not transparent, it merely takes the haze's colour.
        // Artscout - 2026 (#13): the haze must enter as RADIANCE, not as a screen colour. `lit` is HDR -- gCloudSun
        // times SunGain 255, so 17..47 through a sunlit face -- while gFogColor is the LDR 0..1 value the terrain
        // writes straight to the target. Lerping one into the other mixes two different units, and the tonemap at
        // the end then reads the result as radiance. Measured (haze 0.55, a 40.0 face):
        //     fogF   on screen today      correct
        //     1.00      0.976              0.976
        //     0.20      0.894              0.900
        //     0.00      0.355              0.550      <- the haze itself is 0.550
        // So a fully hazed cloud came out DARKER than the air it is dissolving into, and because Reinhard is flat
        // until the very bottom the whole error lands in the last ~10% of the range as a sudden fall-off. That is
        // "the farther, the darker" -- not aerial perspective, a unit mix.
        //   Invert the TONEMAP to put the haze into radiance, and it is exact by construction: the whole point is
        // tonemap(fogRad) == gFogColor, so at fogF 0 the cloud lands ON the haze colour, which is what "dissolved
        // into the haze" means.
        //   This MUST be the inverse of whatever CloudMarch tonemaps with, and for one build it was not: it stayed
        // c/(1-c), the inverse of Reinhard, after the tonemap became exponential -- written two commits apart, and
        // nothing in either place named the other. Measured on the frame, with the haze at (0.40, 0.58, 0.74):
        //     fogRad c/(1-c)      = (0.67, 1.36, 2.79)  -> tonemaps to (0.046, 0.091, 0.177), not the haze
        //     fogRad -ln(1-c)/E   = (7.32, 12.3, 19.0)  -> tonemaps to (0.401, 0.577, 0.736), exactly the haze
        // and c/(1-c) is superlinear, so each channel was wrong by a DIFFERENT factor (R 0.091, G 0.111, B 0.147
        // of what it should be). That is a hue shift, not just a level error: every cloud in the frame came out
        // with R < G < B, tracking the sky, and the range collapsed too -- a fogRad near 1 drags a cloud whose
        // radiance is 8..50 down to nothing.
        float3 fogRad = CloudTonemapInv(gFogColor.rgb);
        float fogF = saturate((gFogEnd - t) / max(gFogEnd - gFogStart, 1e-4f));
        lit = lerp(fogRad, lit, fogF);

        // Energy-conserving integration over the step (analytic, so step count changes brightness far less
        // than a naive sum would -- important because the step count is a perf knob).
        float  a  = 1.0f - exp(-sigma * dt);
        scat  += trans * a * lit;
        trans *= (1.0f - a);
        t += dt;   // the sphere trace set dt above: DT inside a cloud, the SDF distance outside
    }
    // Artscout - 2026 (#13): TONEMAP. scat is premultiplied HDR radiance, so un-premultiply, compress, and
    // re-premultiply -- tonemapping the premultiplied value would darken the thin edges (where alpha is small)
    // by their own coverage a second time.
    //   Reinhard, on the whole accumulated radiance rather than per step: what reaches the eye is the sum, and
    // that sum is the thing that has to land in [0,1). With it, their numbers finally mean what they say --
    // a sunlit face comes out ~0.91, a shadowed one ~0.36, and the silver lining clips at ~0.99 exactly where a
    // photograph of a backlit cumulus does.
    float  alpha = saturate(1.0f - trans);
    float3 col   = (alpha > 1.0e-4f) ? (scat / alpha) : scat;
    // Artscout - 2026 (#13): mode 7 = the HDR the tonemap is about to eat, the ONE number that decides every lit
    // pixel and that I have been modelling instead of measuring. R = hdr/8 (so 1.0 == the Reinhard knee's
    // useful top), G = hdr/50, B = alpha. Measured off a screenshot the clouds sit at hdr 17..47 -- 20-50x past
    // the knee, where x/(1+x) is flat by construction -- but that was inferred from output pixels, and a 2.8x
    // input range cannot produce the 0.35 spread a real cumulus has whatever the exposure is. So the question is
    // whether the HDR really only spans 2.8x, and this answers it directly instead of me arguing with the picture.
    if (gCloudDiag.z > 6.5f)
    {
        // Artscout - 2026 (#13): the LAST component is the blend alpha, and returning a hard 1.0 here painted the
        // whole frame black -- every CLEAR pixel reaches this line with scat = 0, and under BLEND_PREMUL an
        // opaque black replaces whatever the sky and the terrain had put there. The debug view has to stay
        // transparent exactly where the march found nothing, same as the real return below.
        //   B still carries alpha as a VALUE to look at; only the blend weight changes.
        // G was lum/50 and 70% of a sun-facing cloud sat on that ceiling, so the top of the range was invisible.
        // The measurement it was built for is done -- 8.4..50+, and the two-point fit that came out of it is in
        // the tonemap note below -- so widen G to see what R cannot: R = lum/8 resolves the shadowed end, G =
        // lum/250 the silver lining, B = alpha.
        float lum = dot(col, float3(0.299f, 0.587f, 0.114f));
        return float4(saturate(lum / 8.0f), saturate(lum / 250.0f), alpha, (alpha > 1.0e-4f) ? 1.0f : 0.0f);
    }
    // Artscout - 2026 (#13): THEIR curve, not Reinhard, and the choice is measured rather than aesthetic.
    //   Cumulus tonemaps  pow(1 - exp(-radiance / white_point * exposure), 1/2.2)  with exposure = 10*1e-5 = 1e-4
    // and white_point ~ 1. So their sunIntensity = 255 is a radiance that gets divided by 10,000 -- it never
    // reaches a curve knee at 1. The note that used to sit here said "255 transfers, it only looked untransferable
    // because we had no tonemap". Half right, and the wrong half: we added A tonemap, not THEIRS, and Reinhard's
    // white point is at INFINITY. 255 transfers only together with an exposure; alone it is meaningless.
    //   Measured off CloudDebug 7: the cloud's HDR runs 8.4 (shadowed) to 50+ (sunlit), a real 6:1 -- which is
    // exactly what a photographed cumulus has (base ~110/255, face ~250/255). So the RANGE was right all along;
    // the physics underneath it -- light cache, multiple scattering, ambient -- is producing what it should.
    //   Then ask each curve whether ONE constant can put both ends where the photograph puts them:
    //       1 - exp(-L*E) :  E = 0.067 from the shadowed end, 0.079 from the sunlit end   -- agree to 17%
    //       Reinhard      :  k = 0.090 from the shadowed end, 1.000 from the sunlit end   -- differ by 11x
    // Reinhard's shoulder is simply the wrong shape for this range: it CANNOT fit both, which is why every gain I
    // tried moved the whole cloud without ever opening it up. The 6:1 landed in 22 levels of 255; this puts it in
    // ~134. The exposure falls out of the fit -- it is not a number I picked.
    //   NO gamma, deliberately, though theirs has pow(1/2.2): their radiance is linear (Bruneton) and the gamma is
    // their display transform. Our sky, terrain and cockpit are written straight to the target with no transform
    // at all, so the cloud must match THEM. Lifting their gamma without their pipeline is the exact mistake this
    // file keeps recording.
    col = CloudTonemap(col);
    return float4(col * alpha, alpha);
}

//======================= Light-cache compute pass =============================
// Artscout - 2026 (#13): the light march, done ONCE per cache voxel instead of once per sample per pixel.
// Guarded by a define so the PS/VS compiles never see a u0 UAV (which in ps_5_0 collides with render target 0);
// D3D12Renderer passes CLOUD_CS=1 only for the cs_5_0 target. Same FILE on purpose: CloudDensity must have
// exactly one definition. Two copies of that function drifting apart is the failure mode this session has been
// made of.
#ifdef CLOUD_CS
RWTexture3D<float2> gLightCacheOut : register(u0);

// The cache spans the noise volume's own tiling domain, so it tiles with the field and only the SUN direction
// invalidates it. Wind and camera motion do not: gCloud1.xy/gCloud3.x fold them into sp, which is world-anchored.
#define LC_N        128.0f
// 0.05, not 0.03. At 0.03 the march reached 6400 ft -- 67% of a 9533 ft cloud -- so the cache never saw the far
// side of anything and every deep sample under-reported its own shadow. 0.05 reaches 10,667 ft: 112%, one cloud.
#define LC_STEP     0.05f    // sp units per step (~50 m at CloudScale 0.0003)
#define LC_STEPS    64
#define LC_UPREZ    8        // theirs: full up-rez for the first 8 steps, the raw profile after

// Artscout - 2026 (#13): the per-MODEL cache -- 64^3 per (model, turn), turns tiled 2x2 in XY, models down Z.
// See CloudLightUvw for the layout and why Z alone cannot hold all 48.
// Artscout - 2026 (#13): 64 -> 96, and this is a MEASUREMENT, not a quality setting.
//   The clouds read stepped in VR (where they would: finer angular resolution AND a bigger subtended angle, so
// any world-space grid surfaces there first). Two suspects, the model's 128^3 profile and this cache. Debug 3 --
// the profile, flat and unlit -- measures SMOOTH: 189 distinct levels of 256, spread ~1.5% each, max gap 2. A
// posterised field piles up on a few; this does not. So the model is not the source, and the steps are
// downstream, in the light.
//   Debug 6 then measures the cache: tauSun holds 53% of the cloud in its top 8 levels against the profile's
// ~12%. Suggestive, NOT proof -- that pile-up could equally be the depth distribution (a lot of cloud at similar
// tau) rather than a 64^3 grid, and no histogram separates those two.
//   THIS separates them. 64 -> 96 makes the texel 156 -> 104 ft. If the steps are the grid, their spacing shrinks
// by exactly 1.5x. If they are the distribution, nothing moves. Either answer is worth the build.
//   Cost while it stands: the atlas goes 128x128x768 (50 MB) -> 192x192x1152 (170 MB). Within the 2048/axis cap
// (see CloudLightUvw), and the seam margin improves too -- FILL 0.92 leaves 3.84 texels against trilinear's 0.5,
// up from 2.56. If 96 wins, 170 MB next to the model atlas's 100 is a real decision to make; if it changes
// nothing, this goes back to 64 and the hunt moves on.
#define MLC_N       96.0f
#define MLC_STEPS   64
// The cube's space diagonal is 2*sqrt(3) = 3.4641 local units, so a march of MLC_STEPS x this ALWAYS exits the
// box from any point in any direction -- no ray can end early and under-report its own shadow.
#define MLC_DS      (3.4641016f / MLC_STEPS)

// One (model, turn) entry: march the model's own profile toward the sun, in the model's own frame.
//   No up-rez, and that is forced rather than skipped: the up-rez reads a WORLD-space detail volume, and a model
// has no world position -- it is at a hundred of them. It costs little. tau is an integral over thousands of
// feet and the erosion's features are tens, so they average out of it; their own march already drops the up-rez
// after LC_UPREZ steps for the same reason.
//   EXACT at vertScale 1, which is the default and is also the reference's own value (their
// AUTHORING_TO_WORLD_SCALE is 1, i.e. isotropic). vertScale squashes sp in z, so a local unit stops meaning the
// same number of feet along z as along xy, and both this march's path length and the PS's hext/scale conversion
// would pick up that shear. Stated rather than silently wrong: if that knob is ever used in anger, this is one
// of the places that has to learn about it.
float2 CloudModelLight(float3 local, uint model, uint rot)
{
    // World direction -> sp -> model. CloudToModel maps a POSITION by R(-yaw) after subtracting the centre; a
    // DIRECTION takes the same rotation with no translation, so the two cannot disagree about which way is which.
    float yaw = (float)rot * 1.5707963f;
    float cs = cos(-yaw), sn = sin(-yaw);
    float3 s = normalize(float3(gCloud2.xy, gCloud2.z * gCloudDiag.w));   // sp is squashed in z by vertScale
    float3 dirs[2];
    dirs[0] = float3(s.x * cs - s.y * sn, s.x * sn + s.y * cs, s.z);
    dirs[1] = float3(0.0f, 0.0f, -1.0f);   // straight up: Falcon z is DOWN, and a turn about z never touches it

    float2 acc = float2(0.0f, 0.0f);
    [unroll] for (int k = 0; k < 2; ++k)
    {
        // MIDPOINT, as the world march has it ((i + 0.5) * step): sample the middle of each step, so the voxel
        // contributes half of its own cell and the quadrature is second-order rather than first.
        float3 p = local + dirs[k] * (MLC_DS * 0.5f);
        float  t = 0.0f;
        [loop] for (int i = 0; i < MLC_STEPS; ++i)
        {
            if (any(abs(p) > 1.0f)) break;             // left the model's cube: nothing further belongs to it
            t += CloudModelSample(p, (float)model).g * MLC_DS;   // .g = the dimensional profile, as `lite` reads it
            p += dirs[k] * MLC_DS;
        }
        acc[k] = t;                                     // the integral per LOCAL unit -- the PS scales it by hext
    }
    return acc;
}

[numthreads(8, 8, 4)]
void CS_LightCache(uint3 id : SV_DispatchThreadID)
{
    // Artscout - 2026 (#13): the library bakes per MODEL; the procedural fallback bakes the world box below.
    // Both live here because both are real: instances cannot make an overcast (hext <= 0.5*cell is what buys the
    // single fetch per sample), so the tiling bake stays the vehicle for a solid layer.
    if (gCloudLib.x > 0.5f)
    {
        const uint N = (uint)MLC_N, count = (uint)max(gCloudLib.x, 1.0f);
        float txf, tyf; CloudLibTiles(gCloudLib.x, txf, tyf);
        const uint TX = (uint)txf, TY = (uint)tyf;
        if (id.x >= N * 2u * TX || id.y >= N * 2u * TY || id.z >= N) return;
        // XY tile = (model column*2 + turnX, model row*2 + turnY); z is the model's local depth, no stacking.
        const uint txi = id.x / N, tyi = id.y / N;
        const uint rot   = (tyi % 2u) * 2u + (txi % 2u);          // the 2x2 turn sub-tile
        const uint model = (tyi / 2u) * TX + (txi / 2u);
        if (model >= count) { gLightCacheOut[id] = float2(0.0f, 0.0f); return; }   // padding tile in the grid tail
        float3 local = (float3(id.x % N, id.y % N, id.z) + 0.5f) * (2.0f / MLC_N) - 1.0f;
        // Skip empty voxels. Nothing is ever shaded where there is no cloud (the march bails on d <= 0.001), and
        // the converter's fit leaves most of the cube as air -- so this is the difference between a dispatch that
        // hitches and one that does not.
        if (CloudModelSample(local, (float)model).g < 0.001f) { gLightCacheOut[id] = float2(0.0f, 0.0f); return; }
        gLightCacheOut[id] = CloudModelLight(local, model, rot);
        return;
    }

    if (id.x >= (uint)LC_N || id.y >= (uint)LC_N || id.z >= (uint)LC_N) return;

    // Voxel centre -> sp (the noise volume's coordinates).
    float3 sp0 = (float3(id) + 0.5f) * (CLOUD_NOISE_PERIOD / LC_N);

    // sp -> wp, exactly inverting CloudDensity's own mapping, so the CS and the PS cannot disagree about where
    // a sample is. (sp.xy = (wp.xy + gCloud1.xy) * gCloud1.z; sp.z = (wp.z + gCloud3.x) * gCloud1.z * gCloudDiag.w)
    const float invS  = 1.0f / max(gCloud1.z, 1.0e-9f);
    const float invSZ = 1.0f / max(gCloud1.z * gCloudDiag.w, 1.0e-9f);

    float2 acc = float2(0.0f, 0.0f);
    // r: toward the sun. g: straight up -- their tauVertical, which is what an honest ambient term needs and
    // what ours has been faking with a constant. Falcon z is DOWN, so "up" is -z.
    float3 dirs[2] = { gCloud2.xyz, float3(0.0f, 0.0f, -1.0f) };

    [unroll] for (int k = 0; k < 2; ++k)
    {
        float t = 0.0f;
        // Step in WORLD feet along the direction, then convert back to sp -- the direction is a world vector and
        // sp is anisotropic (gCloudDiag.w squashes z), so stepping in sp would bend the ray.
        const float stepFt = LC_STEP * invS;
        [loop] for (int i = 0; i < LC_STEPS; ++i)
        {
            float3 wpOff = dirs[k] * (stepFt * ((float)i + 0.5f));
            float3 sp = sp0 + float3(wpOff.xy * gCloud1.z, wpOff.z * gCloud1.z * gCloudDiag.w);
            float3 wp = float3(sp.xy * invS - gCloud1.xy, sp.z * invSZ - gCloud3.x);
            float2 aux;
            // lite = the raw profile (no up-rez) past the first LC_UPREZ steps, theirs verbatim: the erosion
            // only matters near the sample, and this is what makes 64 steps affordable at all.
            float d = CloudDensity(wp, 0.0f, 1.0f, 1.0f, i >= LC_UPREZ, aux);
            t += d * stepFt;
            if (t * gCloudLight.x >= 5.0f) break;   // their cut-off: exp(-5) = 0.0067, nothing past it is visible
        }
        acc[k] = t;
    }
    gLightCacheOut[id] = acc;
}
#endif // CLOUD_CS

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
        // The march jitters EVERY step from CloudHash231(worldRayKey, stepIndex) -- see its note. The key is
        // derived from rd INSIDE CloudMarch (stereo-coherent), so nothing screen-space is passed in any more; the
        // 4x4 Bayer that used to be built here is long gone, and with it the checkerboard it printed on every fringe.
        float4 c = CloudMarch(rd, tMax);
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
        // Artscout - 2026 (#107 bindless): the terrain reaches its tile through the resident heap by the
        // per-vertex slot, so the whole ground is one draw. Every other pass leaves FF_BINDLESS clear.
#if FF_BINDLESS_OK
        Texture2D bindlessTex = ResourceDescriptorHeap[NonUniformResourceIndex(i.TexIndex)];
        float4 t0 = ((gFlags & FF_BINDLESS) && i.TexIndex != 0xFFFFFFFFu)
                        ? bindlessTex.Sample(gSamp0, i.Uv0)
                        : gTex0.Sample(gSamp0, i.Uv0);
#else
        float4 t0 = gTex0.Sample(gSamp0, i.Uv0);
#endif
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
