//-----------------------------------------------------------------------------
// FFStateMap.cpp  -- legacy STATE_* -> D3D11 state bundle translation.
// PHASE 2 of the D3D7 -> D3D11 port. See FFStateMap.h.
//-----------------------------------------------------------------------------
#include "stdafx.h"
#include "FFStateMap.h"

// Small helper to fill a descriptor compactly.
static FFStateDesc Make(unsigned int flags,
                        FFBlendMode  blend  = BLEND_OPAQUE,
                        FFFilterMode filter = FILTER_LINEAR,
                        FFAddrMode   addr   = ADDR_WRAP,
                        bool depthWrite = true,
                        bool depthTest  = true)
{
	FFStateDesc d;
	d.flags      = flags;
	d.blend      = blend;
	d.filter     = filter;
	d.addr       = addr;
	d.depthWrite = depthWrite;
	d.depthTest  = depthTest;
	return d;
}

bool FFMapState(int s, FFStateDesc& out)
{
	switch (s)
	{
	// ---- untextured ----
	case STATE_SOLID:        out = Make(0);                                 return true;
	case STATE_LIT:          out = Make(FF_VERTEXCOLOR | FF_LIGHTING);      return true;
	case STATE_GOURAUD:      out = Make(FF_VERTEXCOLOR | FF_LIGHTING);      return true;

	// ---- textured, opaque ----
	case STATE_TEXTURE:
	case STATE_TEXTURE_PERSPECTIVE:
		out = Make(FF_TEXTURE0);                                           return true;
	case STATE_TEXTURE_LIT:
	case STATE_TEXTURE_LIT_PERSPECTIVE:
	case STATE_TEXTURE_GOURAUD:
	case STATE_TEXTURE_GOURAUD_PERSPECTIVE:
		out = Make(FF_TEXTURE0 | FF_VERTEXCOLOR | FF_LIGHTING);            return true;
	case STATE_TEXTURE_SMOOTH:
	case STATE_TEXTURE_SMOOTH_PERSPECTIVE:
		out = Make(FF_TEXTURE0, BLEND_OPAQUE, FILTER_LINEAR);             return true;
	case STATE_TEXTURE_NOFILTER:
	case STATE_TEXTURE_NOFILTER_PERSPECTIVE:
		out = Make(FF_TEXTURE0, BLEND_OPAQUE, FILTER_POINT);             return true;

	// Text glyphs: textured + alpha-tested, alpha-blended.
	case STATE_TEXTURE_TEXT:
		// Artscout - 2026: #7 FILTER_POINT (was LINEAR): blit the font glyph without bilinearly bleeding the next
		// font-atlas ROW -> letters do not "jump" vertically (D3D7/FF6 effectively did this). The
		// atlas->panel upscale is smoothed separately (RTT_SOFT/composite).
		out = Make(FF_TEXTURE0 | FF_ALPHATEST, BLEND_ALPHA, FILTER_POINT,
		           ADDR_CLAMP, /*depthWrite*/ false);                      return true;

	// Artscout - 2026: Terrain (textured + pre-lit vertex color) + distance haze (fog).
	case STATE_LANDSCAPE_LIT:
	case STATE_LANDSCAPE_GOURAUD:
		out = Make(FF_TEXTURE0 | FF_VERTEXCOLOR | FF_FOG);                 return true;

	// ---- translucent, untextured ----
	// Artscout - 2026: #36/#31: Z-TEST OFF. Screen (XYZRHW) effects arrive with sz=1.0 (DrawPrimitive
	// "NOTE: HACK", far away) -> with depthTest=true all 3D geometry culled them
	// (visible only against the sky = "barely visible/absent": explosions/smoke/tracers).
	// The reference draws these hand-sorted translucent effects with ZFUNC=ALWAYS
	// (as already done for the RTT overlay STATE_CHROMA_TEXTURE_GOURAUD2). depthWrite is already off.
	case STATE_ALPHA_SOLID:
	case STATE_ALPHA_LIT:
	case STATE_ALPHA_GOURAUD:
		out = Make(FF_VERTEXCOLOR, BLEND_ALPHA, FILTER_LINEAR, ADDR_WRAP,
		           /*depthWrite*/ false, /*depthTest*/ false);            return true;

	// ---- chroma-keyed (discard, stays opaque blend) ----
	case STATE_CHROMA_TEXTURE:
	case STATE_CHROMA_TEXTURE_PERSPECTIVE:
		out = Make(FF_TEXTURE0 | FF_CHROMAKEY);                            return true;
	case STATE_CHROMA_TEXTURE_LIT:
	case STATE_CHROMA_TEXTURE_LIT_PERSPECTIVE:
	case STATE_CHROMA_TEXTURE_GOURAUD:
	case STATE_CHROMA_TEXTURE_GOURAUD_PERSPECTIVE:
		out = Make(FF_TEXTURE0 | FF_CHROMAKEY | FF_VERTEXCOLOR);           return true;
	case STATE_CHROMA_TEXTURE_GOURAUD2:   // 3D-pit HUD/MFD RTT (D3D7 MPR_SE_CHROMA2)
		// Artscout - 2026: FF_MODULATE2X (x2) restored for HUD/MFD/DED symbology BRIGHTNESS (without it the displays
		// are dim). The RTT background (0,0,0,0) is chroma-keyed out, so the doubling hits only
		// the symbology. Z OFF: the reference draws the RTT/2D overlay with ZFUNC=ALWAYS.
		// #7 ADDR_CLAMP (was WRAP): WRAP made HUD text "float vertically" -- glyph UVs at the
		// cell boundary wrapped and pulled in the adjacent font-atlas row. CLAMP fixes it.
		// For HUD lines/geometry (untextured) the address mode has no effect.
		// #7 GOURAUD2 = both the HUD text/line blit AND the atlas->panel composite (when MSAA is off).
		// The composite needs LINEAR (smooth upscale, else the font looks rough). FILTER_POINT here gave
		// stair-stepping. Restored LINEAR. MODULATE2X removed (brightness test).
		out = Make(FF_TEXTURE0 | FF_CHROMAKEY | FF_VERTEXCOLOR,
		           BLEND_OPAQUE, FILTER_LINEAR, ADDR_CLAMP, /*depthWrite*/ false, /*depthTest*/ false);
		return true;

	case STATE_RTT_SOFT:   // Artscout - 2026: #7 ADDITIVE EMISSIVE composite of the display RTT atlas (HUD/MFD/DED/RWR)
		// NO chroma, BLEND_ADDITIVE: the atlas symbology is ADDED onto the cockpit (emissive), black
		// background gives 0, the line/text AA gradient adds smoothly -> no hard chroma cut ->
		// no rim/volume. Shader FF_RTTSOFT: c.rgb=atlas*tint, c.a=rttAlpha. depth off.
		// ADDR_CLAMP (composite, no wrap). Used ALWAYS instead of chroma (see DrawRttQuad).
		out = Make(FF_TEXTURE0 | FF_RTTSOFT | FF_VERTEXCOLOR,
		           BLEND_ADDITIVE, FILTER_LINEAR, ADDR_CLAMP, /*depthWrite*/ false, /*depthTest*/ false);
		return true;

	// ---- alpha-blended textures ----
	// Artscout - 2026: #36/#31: depthTest=false (6th arg) -- the same screen effects with sz=1.0 (explosions/smoke/
	// tracers/particles/clouds/sky via RestoreState(STATE_ALPHA_TEXTURE_*)). All calls are the
	// screen path (draw2d/drawparticlesys/drawsgmt/drawcld/otwsky); object geometry
	// does not use these states -> disabling the Z-test is isolated and safe.
	case STATE_ALPHA_TEXTURE:
	case STATE_ALPHA_TEXTURE_PERSPECTIVE:
		out = Make(FF_TEXTURE0, BLEND_ALPHA, FILTER_LINEAR, ADDR_WRAP, false, /*depthTest*/ false); return true;
	case STATE_ALPHA_TEXTURE_LIT:
	case STATE_ALPHA_TEXTURE_LIT_PERSPECTIVE:
	case STATE_ALPHA_TEXTURE_GOURAUD:
	case STATE_ALPHA_TEXTURE_GOURAUD_PERSPECTIVE:
		out = Make(FF_TEXTURE0 | FF_VERTEXCOLOR, BLEND_ALPHA, FILTER_LINEAR, ADDR_WRAP, false, /*depthTest*/ false); return true;
	case STATE_ALPHA_TEXTURE_SMOOTH:
	case STATE_ALPHA_TEXTURE_SMOOTH_PERSPECTIVE:
		out = Make(FF_TEXTURE0, BLEND_ALPHA, FILTER_LINEAR, ADDR_WRAP, false, /*depthTest*/ false); return true;
	case STATE_ALPHA_TEXTURE_NOFILTER:
	case STATE_ALPHA_TEXTURE_NOFILTER_PERSPECTIVE:
		out = Make(FF_TEXTURE0, BLEND_ALPHA, FILTER_POINT, ADDR_WRAP, false, /*depthTest*/ false); return true;
	case STATE_ALPHA_TEXTURE_PERSPECTIVE_CLAMP:
		out = Make(FF_TEXTURE0, BLEND_ALPHA, FILTER_LINEAR, ADDR_CLAMP, false, /*depthTest*/ false); return true;

	// ---- multitexture ----
	case STATE_MULTITEXTURE:
		out = Make(FF_TEXTURE0 | FF_TEXTURE1 | FF_FOG);                    return true;

	// Artscout - 2026: #12: water terrain tiles -- same as multitexture terrain plus the animated water effect.
	case STATE_WATER:
		out = Make(FF_TEXTURE0 | FF_TEXTURE1 | FF_FOG | FF_WATER);         return true;
	case STATE_MULTITEXTURE_ALPHA:
		out = Make(FF_TEXTURE0 | FF_TEXTURE1 | FF_FOG, BLEND_ALPHA, FILTER_LINEAR, ADDR_WRAP, false, /*depthTest*/ false); return true;

	default:
		out = Make(0);   // fall back to SOLID
		return false;
	}
}
