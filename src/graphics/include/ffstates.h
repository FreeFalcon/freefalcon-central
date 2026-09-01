//-----------------------------------------------------------------------------
// ffstates.h -- the legacy STATE_* prestored-render-state enum, on its own.
//
// Artscout - 2026 (#104, Linux port). Split out of context.h, which it still lives in front of: context.h includes
// this header, so every existing user keeps seeing the same names at the same (global) scope and nothing else moves.
// The reason for the split is ffstatemap.h: it needs ONLY these values, but pulling context.h for them dragged in
// windows.h, d3d7compat.h and the rest of the engine -- which the Vulkan backend cannot include, because it also
// builds standalone into the Linux ffvulkan library. With the enum here, ffstatemap.{h,cpp} are dependency-free and
// BOTH backends translate STATE_* through the one table instead of each inventing its own.
//
// The legacy engine pre-stores these ~41 state bundles and selects one with RestoreState(STATE_x).
//-----------------------------------------------------------------------------
#ifndef _FFSTATES_H_
#define _FFSTATES_H_

enum
{
    //SOLID (plainPolys)
    STATE_SOLID = 0,
    STATE_LIT,
    STATE_GOURAUD,

    //TEXTURED (texturedPolys)
    STATE_TEXTURE,
    STATE_TEXTURE_PERSPECTIVE,
    STATE_TEXTURE_LIT,
    STATE_TEXTURE_LIT_PERSPECTIVE,
    STATE_TEXTURE_SMOOTH,
    STATE_TEXTURE_SMOOTH_PERSPECTIVE,
    STATE_TEXTURE_GOURAUD,
    STATE_TEXTURE_GOURAUD_PERSPECTIVE,

    STATE_TEXTURE_NOFILTER,
    STATE_TEXTURE_NOFILTER_PERSPECTIVE,

    STATE_TEXTURE_TEXT,

    STATE_LANDSCAPE_LIT,
    STATE_LANDSCAPE_GOURAUD,

    //TRANSLUCENT (translucentPolys)
    STATE_ALPHA_SOLID,
    STATE_ALPHA_LIT,
    STATE_ALPHA_GOURAUD,

    STATE_CHROMA_TEXTURE,
    STATE_CHROMA_TEXTURE_PERSPECTIVE,
    STATE_CHROMA_TEXTURE_LIT,
    STATE_CHROMA_TEXTURE_LIT_PERSPECTIVE,
    STATE_CHROMA_TEXTURE_GOURAUD,
    STATE_CHROMA_TEXTURE_GOURAUD_PERSPECTIVE,
    STATE_CHROMA_TEXTURE_GOURAUD2, // ASSO: new color blending state for 3D pit HUD
    STATE_ALPHA_TEXTURE,
    STATE_ALPHA_TEXTURE_PERSPECTIVE,
    STATE_ALPHA_TEXTURE_LIT,
    STATE_ALPHA_TEXTURE_LIT_PERSPECTIVE,
    STATE_ALPHA_TEXTURE_SMOOTH,
    STATE_ALPHA_TEXTURE_SMOOTH_PERSPECTIVE,
    STATE_ALPHA_TEXTURE_GOURAUD,
    STATE_ALPHA_TEXTURE_GOURAUD_PERSPECTIVE,

    STATE_ALPHA_TEXTURE_NOFILTER,
    STATE_ALPHA_TEXTURE_NOFILTER_PERSPECTIVE,

    STATE_ALPHA_TEXTURE_PERSPECTIVE_CLAMP,

    //DOUBLE-TEXTURED (texturedPolys)
    STATE_MULTITEXTURE,
    STATE_MULTITEXTURE_ALPHA,

    STATE_RTT_SOFT, // #7 AA-RTT: soft composite of the displays atlas (D3D11/MSAA only)

    STATE_WATER, // #12: animated water terrain tile (D3D11 only)

    //
    MAXIMUM_MPR_STATE = 41
};

#endif // _FFSTATES_H_
