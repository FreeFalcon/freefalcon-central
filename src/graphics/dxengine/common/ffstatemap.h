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

#include "context.h"   // for the STATE_* enum

// Must match FFEmu.hlsl.
enum FFFlags
{
	FF_TEXTURE0    = 1u << 0,
	FF_TEXTURE1    = 1u << 1,
	FF_VERTEXCOLOR = 1u << 2,
	FF_LIGHTING    = 1u << 3,
	FF_CHROMAKEY   = 1u << 4,
	FF_ALPHATEST   = 1u << 5,
	FF_FOG         = 1u << 6,
	FF_MODULATE2X  = 1u << 7,
	FF_TEXCOLORDIFFUSE = 1u << 8,	// HUD/DED text (color=vertex, texture=mask)
	FF_RTTSOFT     = 1u << 9,		// #7 AA-RTT: soft composite of the atlas (alpha=brightness)
	FF_WATER       = 1u << 10,		// #12: animated water tile (shimmer + tint)
	// bits 11 (FF_EMISSIVE) and 12 (FF_AFTERBURNER) are set via renderer methods, not this map.
	FF_IRGREY      = 1u << 14,		// #DX12 A5: sensor pass grey-out (TGP/TV, Maverick/FLIR/IR) -> luma in PS
	FF_COCKPIT     = 1u << 13,		// Artscout - 2026: #72 cockpit-fidelity pass (specular + reduced
									// ambient flood + mild contrast). Sticky, set by SetCockpitPass().
	FF_GLOC        = 1u << 15,		// Artscout - 2026: G-force / end-flight vignette (blackout/redout). Fullscreen
									// post-process pass: the PS darkens/tints by radial UV distance (gGloc). Replaces
									// the legacy screen-space tunnel-ring so it works on D3D11/D3D12 and per-eye in VR.
};

// Coarse buckets the ~38 states collapse into. Each names a small set of D3D11
// state objects created once at startup (see D3D11Renderer).
enum FFBlendMode  { BLEND_OPAQUE, BLEND_ALPHA, BLEND_ADDITIVE };
enum FFFilterMode { FILTER_LINEAR, FILTER_POINT };
enum FFAddrMode   { ADDR_WRAP, ADDR_CLAMP };

struct FFStateDesc
{
	unsigned int flags;       // FF_* bits for the shader cbuffer
	FFBlendMode  blend;
	FFFilterMode filter;
	FFAddrMode   addr;
	bool         depthWrite;  // most opaque states write depth; alpha usually not
	bool         depthTest;
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
