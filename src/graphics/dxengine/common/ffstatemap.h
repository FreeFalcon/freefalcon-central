//-----------------------------------------------------------------------------
// FFStateMap.h  -- maps the legacy STATE_* render states to D3D11 pipeline
// state + FFEmu.hlsl feature flags.
//
// PHASE 2 of the D3D7 -> D3D11 port.
//
// The legacy engine pre-stores ~38 combined states (STATE_SOLID, STATE_LIT,
// STATE_TEXTURE_GOURAUD, STATE_CHROMA_TEXTURE, STATE_ALPHA_TEXTURE_*,
// STATE_MULTITEXTURE, ...) and selects one with RestoreState(STATE_x). Each is
// a bundle of D3D7 render states + texture stage states. In D3D11 that bundle
// becomes: { blend state, depth-stencil state, rasterizer state, sampler state,
// shader feature flags }. This header is the single translation table.
//
// The flag bits (FF_*) mirror the ones in FFEmu.hlsl exactly.
//-----------------------------------------------------------------------------
#ifndef _FFSTATEMAP_H_
#define _FFSTATEMAP_H_

// Artscout - 2026 (#104): the STATE_* enum only -- NOT context.h, which would drag windows.h and the rest of the
// engine in. That keeps this header (and ffstatemap.cpp) dependency-free, so the Vulkan backend can translate the
// legacy states through the SAME table as D3D12 while still building standalone into the Linux ffvulkan lib.
#include "ffstates.h"

// Must match FFEmu.hlsl.
enum FFFlags
{
    FF_TEXTURE0 = 1u << 0,
    FF_TEXTURE1 = 1u << 1,
    FF_VERTEXCOLOR = 1u << 2,
    FF_LIGHTING = 1u << 3,
    FF_CHROMAKEY = 1u << 4,
    FF_ALPHATEST = 1u << 5,
    FF_FOG = 1u << 6,
    FF_MODULATE2X = 1u << 7,
    FF_TEXCOLORDIFFUSE = 1u << 8, // HUD/DED text (color=vertex, texture=mask)
    FF_RTTSOFT =
        1u << 9, // #7 AA-RTT: soft composite of the atlas (alpha=brightness)
    FF_WATER = 1u << 10, // #12: animated water tile (shimmer + tint)
    // Artscout - 2026 (#104): named, not left as bare 1u<<11 / 1u<<12 literals at the call sites. FFMapState never
    // sets these two -- SetEmissive() / SetAfterburner() do -- but they are the same FF_* word and belong here so
    // both backends spell them the same way.
    FF_EMISSIVE =
        1u
        << 11, // #49: self-illuminated surface (afterburner cone, nav/formation lights)
    FF_AFTERBURNER =
        1u
        << 12, // #49: afterburner cone (COMP_AB/COMP_AB2) -- warm recolor in the PS
    FF_IRGREY =
        1u
        << 14, // #DX12 A5: sensor pass grey-out (TGP/TV, Maverick/FLIR/IR) -> luma in PS
    FF_COCKPIT =
        1u
        << 13, // Artscout - 2026: #72 cockpit-fidelity pass (specular + reduced
    // ambient flood + mild contrast). Sticky, set by SetCockpitPass().
    FF_GLOC =
        1u
        << 15, // Artscout - 2026: G-force / end-flight vignette (blackout/redout). Fullscreen
    // post-process pass: the PS darkens/tints by radial UV distance (gGloc). Replaces
    // the legacy screen-space tunnel-ring so it works on D3D11/D3D12 and per-eye in VR.
    FF_NVG =
        1u
        << 16, // Artscout - 2026: #97 night-vision goggles -- recolor world passes (terrain/
    // objects/cockpit/sky) to green phosphor + tube gain in the PS. OR'd into m_flags
    // by the world Begin*Pass funcs when SetNvgMode(true).
    FF_FULLBRIGHT =
        1u
        << 17, // Artscout - 2026: #97 unlit/full-bright -- force lit=1 so a lit BSP renders at full
    // material colour regardless of time-of-day. For the exit-menu dialog (dark at night).
    FF_CLOUD =
        1u
        << 18, // Artscout - 2026: #13 volumetric cloud layer -- the PS raymarches the slab in
    // gCloud0/1/2/3 along the view ray. Set only by BeginCloudPass.
    FF_BINDLESS =
        1u
        << 19, // Artscout - 2026 (#107): sample the base texture from the resident heap by the
    // per-vertex slot instead of t0. Set only by DrawTerrainMeshBindless; same bit and
    // meaning as FF_BINDLESS in the Vulkan shader.
};

// Coarse buckets the ~38 states collapse into. Each names a small set of D3D11
// state objects created once at startup (see D3D11Renderer).
// Artscout - 2026 (#13): BLEND_PREMUL = ONE / INV_SRC_ALPHA -- src + dst*(1-a), for rgb ALREADY weighted by its
// own coverage. A volumetric march yields exactly that (scat accumulates as trans*a*lit), as do premultiplied
// sprites. Without it there were only two choices and neither is correct for such data: ALPHA multiplies by
// coverage a SECOND time (dark, opaque only at the core), ADDITIVE cannot occlude at all (bright, see-through).
// Appended, so existing values keep their numbers.
enum FFBlendMode
{
    BLEND_OPAQUE,
    BLEND_ALPHA,
    BLEND_ADDITIVE,
    BLEND_PREMUL
};
enum FFFilterMode
{
    FILTER_LINEAR,
    FILTER_POINT
};
enum FFAddrMode
{
    ADDR_WRAP,
    ADDR_CLAMP
};

struct FFStateDesc
{
    unsigned int flags; // FF_* bits for the shader cbuffer
    FFBlendMode blend;
    FFFilterMode filter;
    FFAddrMode addr;
    bool depthWrite; // most opaque states write depth; alpha usually not
    bool depthTest;
};

// Translate one legacy STATE_* into a D3D11 state bundle. Implemented as a
// switch in FFStateMap.cpp. The cases below are grouped by the legacy naming:
//   *_LIT / *_GOURAUD   -> FF_VERTEXCOLOR (engine pre-lights into vertex color)
//   TEXTURE / CHROMA / ALPHA / MULTITEXTURE -> texture + key/alpha/blend bits
//   *_SMOOTH vs *_NOFILTER -> FILTER_LINEAR vs FILTER_POINT
//   *_PERSPECTIVE        -> no-op (D3D11 is always perspective-correct)
//   *_CLAMP              -> ADDR_CLAMP
//
// Returns false for an unknown state (caller falls back to STATE_SOLID).
bool FFMapState(int legacyState, FFStateDesc& out);

#endif // _FFSTATEMAP_H_
