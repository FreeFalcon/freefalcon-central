/***************************************************************************\
    PolyLibDraw.cpp
    Miro "Jammer" Torrielli
    06Oct03

	- Begin Major Rewrite (Fat-free version)
\***************************************************************************/
#include "stdafx.h"
#include "StateStack.h"
#include "ColorBank.h"
#include "TexBank.h"
#include "PolyLib.h"
#include "context.h"
#include "TerrTex.h"
#include "FalcLib\include\dispopts.h"

extern int verts;


static inline void SetSpecularFog()
{
	DWORD specular = (min(255,FloatToInt32(TheStateStack.fogValue*255.f)) << 24)+0xFFFFFF;
	TheStateStack.context->UpdateSpecularFog(specular);
}

static inline void SelectState(GLint state)
{
/*	if(DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS && TheTerrTextures.lightLevel < 0.5f)
	{
		TheStateStack.context->RestoreState(STATE_MULTITEXTURE_ALPHA);
	}
	else*/
		TheStateStack.context->RestoreState(state);
}

// This helper function is used for all non-interpolated primitives. Gouraud shaded primitives do their own thing.
static inline void SetForegroundColor(DWORD opFlag, int rgbaIdx, int IIdx)
{
	Pcolor		*rgba;
	Pintensity	I;
	DWORD		color = 0;

#ifdef DEBUG
	int	colorSet = TRUE;
#endif

	
	if(opFlag & PRIM_COLOP_COLOR)
	{
		ShiAssert(rgbaIdx >= 0);
		rgba = &TheColorBank.ColorPool[rgbaIdx];
	
		if(opFlag & PRIM_COLOP_INTENSITY)
		{
			ShiAssert(IIdx >= 0);
			I = TheStateStack.IntensityPool[IIdx];

			color	 = FloatToInt32(rgba->r * I * 255.9f);
			color	|= FloatToInt32(rgba->g * I * 255.9f) << 8;
			color	|= FloatToInt32(rgba->b * I * 255.9f) << 16;
		}
		else
		{
			color	 = FloatToInt32(rgba->r * 255.9f);
			color	|= FloatToInt32(rgba->g * 255.9f) << 8;
			color	|= FloatToInt32(rgba->b * 255.9f) << 16;
		}

		if(ShadowBSPRendering) 							// COBRA - RED - if rendering a shadow, alpha applied here
			color |= FloatToInt32(rgba->a * 255.9f * ShadowAlphaLevel) << 24;
		else 
			color |= FloatToInt32(rgba->a * 255.9f ) << 24;
	}
	else if(opFlag & PRIM_COLOP_INTENSITY)
	{
		ShiAssert(IIdx >= 0);

		color	 = FloatToInt32(TheStateStack.IntensityPool[IIdx] * 255.9f);
		color	|= color << 8;
		color	|= color << 8;
	}

	ShiAssert(colorSet);
	TheStateStack.context->SelectForegroundColor(color);
}

static inline void pvtDraw2DPrim(PpolyType type, int nVerts, int *xyzIdxPtr)
{
	TheStateStack.context->RestoreState(STATE_SOLID);
	TheStateStack.context->DrawPrimitive2D(type, nVerts, xyzIdxPtr);
}

static inline void pvtDraw2DLine(int *xyzIdxPtr)
{
	TheStateStack.context->RestoreState(STATE_SOLID);
	TheStateStack.context->Draw2DLine(&TheStateStack.XformedPosPool[xyzIdxPtr[0]], &TheStateStack.XformedPosPool[xyzIdxPtr[1]]);
}

static inline void pvtDraw2DPoint(int *xyzIdxPtr)
{
	TheStateStack.context->RestoreState(STATE_SOLID);
	TheStateStack.context->Draw2DPoint(&TheStateStack.XformedPosPool[xyzIdxPtr[0]]);
}

void DrawPrimPoint(PrimPointFC *point)
{
	//JAM 15Dec03
	BOOL bToggle = FALSE;

	if(TheStateStack.context->bZBuffering && DisplayOptions.bZBuffering)
	{
		bToggle = TRUE;
		TheStateStack.context->SetZBuffering(FALSE);
	}

	ShiAssert(point->type == PointF);
	ShiAssert(point->nVerts >= 1);

	verts += point->nVerts;

	SetForegroundColor(PRIM_COLOP_COLOR, point->rgba, -1);
	pvtDraw2DPrim(PointF, point->nVerts, point->xyz);

   //JAM 15Dec03
	if(bToggle && DisplayOptions.bZBuffering)
		TheStateStack.context->SetZBuffering(TRUE);
}

void DrawPrimFPoint(PrimPointFC *point)
{
	//JAM 15Dec03
	BOOL bToggle = FALSE;

	if(TheStateStack.context->bZBuffering && DisplayOptions.bZBuffering)
	{
		bToggle = TRUE;
		TheStateStack.context->SetZBuffering(FALSE);
	}

	ShiAssert(point->type == PointF);
	ShiAssert(point->nVerts >= 1);

	verts += point->nVerts;

	SetSpecularFog();
	SetForegroundColor(PRIM_COLOP_COLOR, point->rgba, -1);
	pvtDraw2DPrim(PointF, point->nVerts, point->xyz);

   //JAM 15Dec03
	if(bToggle && DisplayOptions.bZBuffering)
		TheStateStack.context->SetZBuffering(TRUE);
}

void DrawPrimLine(PrimLineFC *line)
{
	//JAM 15Dec03
	BOOL bToggle = FALSE;

	if(TheStateStack.context->bZBuffering && DisplayOptions.bZBuffering)
	{
		bToggle = TRUE;
		TheStateStack.context->SetZBuffering(FALSE);
	}

	ShiAssert(line->type == LineF);
	ShiAssert(line->nVerts >= 2);

	verts += line->nVerts;

	SetForegroundColor(PRIM_COLOP_COLOR, line->rgba, -1);
	ShiAssert(line->nVerts == 2);
	pvtDraw2DLine(line->xyz);

   //JAM 15Dec03
	if(bToggle && DisplayOptions.bZBuffering)
		TheStateStack.context->SetZBuffering(TRUE);
}

void DrawPrimFLine(PrimLineFC *line)
{
	//JAM 15Dec03
	BOOL bToggle = FALSE;

	if(TheStateStack.context->bZBuffering && DisplayOptions.bZBuffering)
	{
		bToggle = TRUE;
		TheStateStack.context->SetZBuffering(FALSE);
	}

	ShiAssert(line->type == LineF);
	ShiAssert(line->nVerts >= 2);

	verts += line->nVerts;

	SetSpecularFog();
	SetForegroundColor(PRIM_COLOP_COLOR, line->rgba, -1);
	ShiAssert(line->nVerts == 2);

	pvtDraw2DLine(line->xyz);

   //JAM 15Dec03
	if(bToggle && DisplayOptions.bZBuffering)
		TheStateStack.context->SetZBuffering(TRUE);
}

void DrawPoly(PolyFC *poly)
{
	verts += poly->nVerts;
	SetForegroundColor(PRIM_COLOP_COLOR,poly->rgba,-1);
	SelectState(RenderStateTable[poly->type]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_NONE,poly,poly->xyz,NULL,NULL,NULL,true);
	TheStateStack.context->SetPalID(0);
}

void DrawPolyF(PolyFC *poly)
	{
	verts += poly->nVerts;
	SetSpecularFog();
	SetForegroundColor(PRIM_COLOP_COLOR,poly->rgba,-1);
	SelectState(RenderStateTable[poly->type]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_NONE,poly,poly->xyz,NULL,NULL,NULL,true);
	TheStateStack.context->SetPalID(0);
}

void DrawPolyL(PolyFCN *poly)
{
	verts += poly->nVerts;
	SetForegroundColor(PRIM_COLOP_COLOR | PRIM_COLOP_INTENSITY,poly->rgba,poly->I);
	SelectState(RenderStateTable[poly->type]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_NONE,poly,poly->xyz,NULL,NULL,NULL,true);
	TheStateStack.context->SetPalID(0);
}

void DrawPolyFL(PolyFCN *poly)
{
	verts += poly->nVerts;
	SetSpecularFog();
	SetForegroundColor(PRIM_COLOP_COLOR | PRIM_COLOP_INTENSITY,poly->rgba,poly->I);
	SelectState(RenderStateTable[poly->type]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_NONE,poly,poly->xyz,NULL,NULL,NULL,true);
	TheStateStack.context->SetPalID(0);
}

void DrawPolyG(PolyVC *poly)
{
	verts += poly->nVerts;
	SelectState(RenderStateTable[poly->type]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_COLOR,poly,poly->xyz,poly->rgba,NULL,NULL,false);
	TheStateStack.context->SetPalID(0);
}

void DrawPolyFG(PolyVC *poly)
{
	verts += poly->nVerts;
	SetSpecularFog();
	SelectState(RenderStateTable[poly->type]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_COLOR,poly,poly->xyz,poly->rgba,NULL,NULL,false);
	TheStateStack.context->SetPalID(0);
}

void DrawPolyGL(PolyVCN *poly)
{
	verts += poly->nVerts;
	SelectState(RenderStateTable[poly->type]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_COLOR | PRIM_COLOP_INTENSITY,poly,poly->xyz,poly->rgba,poly->I,NULL,false);
	TheStateStack.context->SetPalID(0);
}

void DrawPolyFGL(PolyVCN *poly)
{
	verts += poly->nVerts;
	SetSpecularFog();
	SelectState(RenderStateTable[poly->type]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_COLOR | PRIM_COLOP_INTENSITY,poly,poly->xyz,poly->rgba,poly->I,NULL,false);
	TheStateStack.context->SetPalID(0);
}

void DrawPolyT(PolyTexFC *poly)
{
	verts += poly->nVerts;
	SelectState(RenderStateTable[poly->type]);
	TheTextureBank.Select(TheStateStack.CurrentTextureTable[poly->texIndex]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_TEXTURE,poly,poly->xyz,NULL,NULL,poly->uv,false);
	TheStateStack.context->SetPalID(0);
}

void DrawPolyFT(PolyTexFC *poly)
{
	verts += poly->nVerts;
	SetSpecularFog();
	SelectState(RenderStateTable[poly->type]);
	TheTextureBank.Select(TheStateStack.CurrentTextureTable[poly->texIndex]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_TEXTURE,poly,poly->xyz,NULL,NULL,poly->uv,false);
	TheStateStack.context->SetPalID(0);
}

void DrawPolyGT(PolyTexFC *poly)
{
	verts += poly->nVerts;
	SetSpecularFog();
	SelectState(RenderStateTable[poly->type]);
	TheTextureBank.Select(TheStateStack.CurrentTextureTable[poly->texIndex]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_TEXTURE,poly,poly->xyz,NULL,NULL,poly->uv,false);
	TheStateStack.context->SetPalID(0);
}

void DrawPolyAT(PolyTexFC *poly)
{
	verts += poly->nVerts;
	SetForegroundColor(PRIM_COLOP_COLOR,poly->rgba,-1);
	SelectState(RenderStateTable[poly->type]);
	TheTextureBank.Select(TheStateStack.CurrentTextureTable[poly->texIndex]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_TEXTURE,poly,poly->xyz,NULL,NULL,poly->uv,true);
	TheStateStack.context->SetPalID(0);
}

void DrawPolyFAT(PolyTexFC *poly)
{
	verts += poly->nVerts;
	SetSpecularFog();
	SetForegroundColor(PRIM_COLOP_COLOR,poly->rgba,-1);
	SelectState(RenderStateTable[poly->type]);
	TheTextureBank.Select(TheStateStack.CurrentTextureTable[poly->texIndex]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_TEXTURE,poly,poly->xyz,NULL,NULL,poly->uv,true);
	TheStateStack.context->SetPalID(0);
}

void DrawPolyTL(PolyTexFCN *poly)
{
	verts += poly->nVerts;
	SetForegroundColor(PRIM_COLOP_INTENSITY,-1,poly->I);
	SelectState(RenderStateTable[poly->type]);
	TheTextureBank.Select(TheStateStack.CurrentTextureTable[poly->texIndex]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_TEXTURE,poly,poly->xyz,NULL,NULL,poly->uv,true);
	TheStateStack.context->SetPalID(0);
}

void DrawPolyFTL(PolyTexFCN *poly)
{
	verts += poly->nVerts;
	SetSpecularFog();
	SetForegroundColor(PRIM_COLOP_INTENSITY,-1,poly->I);
	SelectState(RenderStateTable[poly->type]);
	TheTextureBank.Select(TheStateStack.CurrentTextureTable[poly->texIndex]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_TEXTURE,poly,poly->xyz,NULL,NULL,poly->uv,true);
	TheStateStack.context->SetPalID(0);
}

void DrawPolyATL(PolyTexFCN *poly)
{
	verts += poly->nVerts;
	SetForegroundColor(PRIM_COLOP_COLOR | PRIM_COLOP_INTENSITY,poly->rgba,poly->I);
	SelectState(RenderStateTable[poly->type]);
	TheTextureBank.Select(TheStateStack.CurrentTextureTable[poly->texIndex]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_TEXTURE,poly,poly->xyz,NULL,NULL,poly->uv,true);
	TheStateStack.context->SetPalID(0);
}

void DrawPolyFATL(PolyTexFCN *poly)
{
	verts += poly->nVerts;
	SetSpecularFog();
	SetForegroundColor(PRIM_COLOP_COLOR | PRIM_COLOP_INTENSITY,poly->rgba,poly->I);
	SelectState(RenderStateTable[poly->type]);
	TheTextureBank.Select(TheStateStack.CurrentTextureTable[poly->texIndex]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_TEXTURE,poly,poly->xyz,NULL,NULL,poly->uv,true);
	TheStateStack.context->SetPalID(0);
}

void DrawPolyTG(PolyTexVC *poly)
{
	verts += poly->nVerts;
	SelectState(RenderStateTable[poly->type]);
	ShiAssert((poly->type == TexG) || (poly->type == CTexG));
	TheTextureBank.Select(TheStateStack.CurrentTextureTable[poly->texIndex]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_TEXTURE,poly,poly->xyz,NULL,NULL,poly->uv,false);
	TheStateStack.context->SetPalID(0);
}

void DrawPolyFTG(PolyTexVC *poly)
{
	verts += poly->nVerts;
	SetSpecularFog();
	SelectState(RenderStateTable[poly->type]);
	ShiAssert((poly->type == TexG) || (poly->type == CTexG));
	TheTextureBank.Select(TheStateStack.CurrentTextureTable[poly->texIndex]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_TEXTURE,poly,poly->xyz,NULL,NULL,poly->uv,false);
	TheStateStack.context->SetPalID(0);
}

void DrawPolyATG(PolyTexVC *poly)
{
	verts += poly->nVerts;
	SelectState(RenderStateTable[poly->type]);
	ShiAssert((poly->type == ATexG) || (poly->type == CATexG));
	TheTextureBank.Select(TheStateStack.CurrentTextureTable[poly->texIndex]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_COLOR | PRIM_COLOP_TEXTURE,poly,poly->xyz,poly->rgba,NULL,poly->uv,false);
	TheStateStack.context->SetPalID(0);
}

void DrawPolyFATG(PolyTexVC *poly)
{
	verts += poly->nVerts;
	SetSpecularFog();
	SelectState(RenderStateTable[poly->type]);
	ShiAssert((poly->type == ATexG) || (poly->type == CATexG));
	TheTextureBank.Select(TheStateStack.CurrentTextureTable[poly->texIndex]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_COLOR | PRIM_COLOP_TEXTURE,poly,poly->xyz,poly->rgba,NULL,poly->uv,false);
	TheStateStack.context->SetPalID(0);
}

void DrawPolyTGL(PolyTexVCN *poly)
{
	verts += poly->nVerts;
	SelectState(RenderStateTable[poly->type]);
	TheTextureBank.Select(TheStateStack.CurrentTextureTable[poly->texIndex]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_INTENSITY | PRIM_COLOP_TEXTURE,poly,poly->xyz,NULL,poly->I,poly->uv,false);
	TheStateStack.context->SetPalID(0);
}

void DrawPolyFTGL(PolyTexVCN *poly)
{
	verts += poly->nVerts;
	SetSpecularFog();
	SelectState(RenderStateTable[poly->type]);
	TheTextureBank.Select(TheStateStack.CurrentTextureTable[poly->texIndex]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_INTENSITY | PRIM_COLOP_TEXTURE,poly,poly->xyz,NULL,poly->I,poly->uv,false);
	TheStateStack.context->SetPalID(0);
}

void DrawPolyATGL(PolyTexVCN *poly)
{
	verts += poly->nVerts;

	SelectState(RenderStateTable[poly->type]);
	TheTextureBank.Select(TheStateStack.CurrentTextureTable[poly->texIndex]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_COLOR | PRIM_COLOP_INTENSITY | PRIM_COLOP_TEXTURE,poly,poly->xyz,poly->rgba,poly->I,poly->uv,false);
	TheStateStack.context->SetPalID(0);
}

void DrawPolyFATGL(PolyTexVCN *poly)
{
	verts += poly->nVerts;
	SetSpecularFog();
	SelectState(RenderStateTable[poly->type]);
	TheTextureBank.Select(TheStateStack.CurrentTextureTable[poly->texIndex]);
	TheStateStack.context->DrawPoly(PRIM_COLOP_COLOR | PRIM_COLOP_INTENSITY | PRIM_COLOP_TEXTURE,poly,poly->xyz,poly->rgba,poly->I,poly->uv,false);
	TheStateStack.context->SetPalID(0);
}
