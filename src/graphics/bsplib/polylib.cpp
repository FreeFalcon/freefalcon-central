/***************************************************************************\
    PolyLib.cpp
    Scott Randolph
    February 9, 1998

    //JAM 06Jan04 - Begin Major Rewrite
\***************************************************************************/
#include "stdafx.h"
#include "StateStack.h"
#include "ClipFlags.h"
#include "PolyLib.h"

// The pointers to the active jump tables and rendering state tables.
const DrawPrimFp	*DrawPrimJumpTable				= NULL;
const DrawPrimFp	*DrawPrimNoClipJumpTable		= NULL;
const ClipPrimFp	*ClipPrimJumpTable				= NULL;
const DrawPrimFp	*DrawPrimNoFogNoClipJumpTable	= NULL;
const ClipPrimFp	*ClipPrimNoFogJumpTable			= NULL;
const DrawPrimFp	*DrawPrimFogNoClipJumpTable		= NULL;
const ClipPrimFp	*ClipPrimFogJumpTable			= NULL;
const int			*RenderStateTable				= NULL;
const int			*RenderStateTablePC				= NULL;
const int			*RenderStateTableNPC			= NULL;

// Jump tables for polygon drawing
const DrawPrimFp DrawPrimNoClipWithTexJumpTable[PpolyTypeNum] =
{
	(DrawPrimFp)DrawPrimPoint,						// Point
	(DrawPrimFp)DrawPrimLine,						// Line

	(DrawPrimFp)DrawPoly,							// Flat
	(DrawPrimFp)DrawPolyL,							// Flat Lit
	(DrawPrimFp)DrawPolyG,							// Gouraud
	(DrawPrimFp)DrawPolyGL,							// Gouraud Lit
	(DrawPrimFp)DrawPolyT,							// Textured
	(DrawPrimFp)DrawPolyTL,							// Textured Flat Lit
	(DrawPrimFp)DrawPolyTG,							// Textured Gouraud
	(DrawPrimFp)DrawPolyTGL,						// Textured Gouraud Lit
	(DrawPrimFp)DrawPolyT,							// Chroma Textured
	(DrawPrimFp)DrawPolyTL,							// Chroma Textured Flat Lit
	(DrawPrimFp)DrawPolyTG,							// Chroma Textured Gouraud
	(DrawPrimFp)DrawPolyTGL,						// Chroma Textured Gouraud Lit

	// With alpha
	(DrawPrimFp)DrawPoly,							// Flat
	(DrawPrimFp)DrawPolyL,							// Flat Lit
	(DrawPrimFp)DrawPolyG,							// Gouraud
	(DrawPrimFp)DrawPolyGL,							// Gouraud Lit
	(DrawPrimFp)DrawPolyAT,							// Textured
	(DrawPrimFp)DrawPolyATL,						// Textured Flat Lit
	(DrawPrimFp)DrawPolyATG,						// Textured Gouraud
	(DrawPrimFp)DrawPolyATGL,						// Textured Gouraud Lit
	(DrawPrimFp)DrawPolyAT,							// Chroma Textured
	(DrawPrimFp)DrawPolyATL,						// Chroma Textured Flat Lit
	(DrawPrimFp)DrawPolyATG,						// Chroma Textured Gouraud
	(DrawPrimFp)DrawPolyATGL,						// Chroma Textured Gouraud Lit

	(DrawPrimFp)DrawPolyT,							// Bilinear Alpha per Texel Hack
};

const DrawPrimFp DrawPrimFogNoClipWithTexJumpTable[PpolyTypeNum] =
{
	(DrawPrimFp)DrawPrimFPoint,						// Point
	(DrawPrimFp)DrawPrimFLine,						// Line

	(DrawPrimFp)DrawPolyF,							// Flat
	(DrawPrimFp)DrawPolyFL,							// Flat Lit
	(DrawPrimFp)DrawPolyFG,							// Gouraud
	(DrawPrimFp)DrawPolyFGL,						// Gouraud Lit
	(DrawPrimFp)DrawPolyFT,							// Textured
	(DrawPrimFp)DrawPolyFTL,						// Textured Flat Lit
	(DrawPrimFp)DrawPolyFTG,						// Textured Gouraud
	(DrawPrimFp)DrawPolyFTGL,						// Textured Gouraud Lit
	(DrawPrimFp)DrawPolyFT,							// Chroma Textured
	(DrawPrimFp)DrawPolyFTL,						// Chroma Textured Flat Lit
	(DrawPrimFp)DrawPolyFTG,						// Chroma Textured Gouraud
	(DrawPrimFp)DrawPolyFTGL,						// Chroma Textured Gouraud Lit

	// With alpha (disallows fog with textures)
	(DrawPrimFp)DrawPolyF,							// Flat
	(DrawPrimFp)DrawPolyFL,							// Flat Lit
	(DrawPrimFp)DrawPolyFG,							// Gouraud
	(DrawPrimFp)DrawPolyFGL,						// Gouraud Lit
	(DrawPrimFp)DrawPolyFAT,						// Textured
	(DrawPrimFp)DrawPolyFATL,						// Textured Flat Lit
	(DrawPrimFp)DrawPolyFATG,						// Textured Gouraud
	(DrawPrimFp)DrawPolyFATGL,						// Textured Gouraud Lit
	(DrawPrimFp)DrawPolyFAT,						// Chroma Textured
	(DrawPrimFp)DrawPolyFATL,						// Chroma Textured Flat Lit
	(DrawPrimFp)DrawPolyFATG,						// Chroma Textured Gouraud
	(DrawPrimFp)DrawPolyFATGL,						// Chroma Textured Gouraud Lit

	(DrawPrimFp)DrawPolyFT,							// Bilinear Alpha per Texel Hack
};

const DrawPrimFp DrawPrimNoClipNoTexJumpTable[PpolyTypeNum] =
{
	(DrawPrimFp)DrawPrimPoint,						// Point
	(DrawPrimFp)DrawPrimLine,						// Line

	(DrawPrimFp)DrawPoly,							// Flat
	(DrawPrimFp)DrawPolyL,							// Flat Lit
	(DrawPrimFp)DrawPolyG,							// Gouraud
	(DrawPrimFp)DrawPolyGL,							// Gouraud Lit
	(DrawPrimFp)DrawPoly,							// Textured
	(DrawPrimFp)DrawPolyL,							// Textured Flat Lit
	(DrawPrimFp)DrawPolyG,							// Textured Gouraud
	(DrawPrimFp)DrawPolyGL,							// Textured Gouraud Lit
	(DrawPrimFp)DrawPoly,							// Chroma Textured
	(DrawPrimFp)DrawPolyL,							// Chroma Textured Flat Lit
	(DrawPrimFp)DrawPolyG,							// Chroma Textured Gouraud
	(DrawPrimFp)DrawPolyGL,							// Chroma Textured Gouraud Lit

	// With alpha
	(DrawPrimFp)DrawPoly,							// Flat
	(DrawPrimFp)DrawPolyL,							// Flat Lit
	(DrawPrimFp)DrawPolyG,							// Gouraud
	(DrawPrimFp)DrawPolyGL,							// Gouraud Lit
	(DrawPrimFp)DrawPoly,							// Textured
	(DrawPrimFp)DrawPolyL,							// Textured Flat Lit
	(DrawPrimFp)DrawPolyG,							// Textured Gouraud
	(DrawPrimFp)DrawPolyGL,							// Textured Gouraud Lit
	(DrawPrimFp)DrawPoly,							// Chroma Textured
	(DrawPrimFp)DrawPolyL,							// Chroma Textured Flat Lit
	(DrawPrimFp)DrawPolyG,							// Chroma Textured Gouraud
	(DrawPrimFp)DrawPolyGL,							// Chroma Textured Gouraud Lit

	(DrawPrimFp)DrawPoly,							// Bilinear Alpha per Texel Hack
};

const DrawPrimFp DrawPrimFogNoClipNoTexJumpTable[PpolyTypeNum] = 
{
	(DrawPrimFp)DrawPrimFPoint,						// Point
	(DrawPrimFp)DrawPrimFLine,						// Line

	(DrawPrimFp)DrawPolyF,							// Flat
	(DrawPrimFp)DrawPolyFL,							// Flat Lit
	(DrawPrimFp)DrawPolyFG,							// Gouraud
	(DrawPrimFp)DrawPolyFGL,						// Gouraud Lit
	(DrawPrimFp)DrawPolyF,							// Textured
	(DrawPrimFp)DrawPolyFL,							// Textured Flat Lit
	(DrawPrimFp)DrawPolyFG,							// Textured Gouraud
	(DrawPrimFp)DrawPolyFGL,						// Textured Gouraud Lit
	(DrawPrimFp)DrawPolyF,							// Chroma Textured
	(DrawPrimFp)DrawPolyFL,							// Chroma Textured Flat Lit
	(DrawPrimFp)DrawPolyFG,							// Chroma Textured Gouraud
	(DrawPrimFp)DrawPolyFGL,						// Chroma Textured Gouraud Lit

	// With alpha
	(DrawPrimFp)DrawPolyF,							// Flat
	(DrawPrimFp)DrawPolyFL,							// Flat Lit
	(DrawPrimFp)DrawPolyFG,							// Gouraud
	(DrawPrimFp)DrawPolyFGL,						// Gouraud Lit
	(DrawPrimFp)DrawPolyF,							// Textured
	(DrawPrimFp)DrawPolyFL,							// Textured Flat Lit
	(DrawPrimFp)DrawPolyFG,							// Textured Gouraud
	(DrawPrimFp)DrawPolyFGL,						// Textured Gouraud Lit
	(DrawPrimFp)DrawPolyF,							// Chroma Textured
	(DrawPrimFp)DrawPolyFL,							// Chroma Textured Flat Lit
	(DrawPrimFp)DrawPolyFG,							// Chroma Textured Gouraud
	(DrawPrimFp)DrawPolyFGL,						// Chroma Textured Gouraud Lit

	(DrawPrimFp)DrawPolyF,							// Bilinear Alpha per Texel Hack
};

const DrawPrimFp DrawPrimWithClipJumpTable[PpolyTypeNum] = 
{
	(DrawPrimFp)DrawClippedPrim,					// Point
	(DrawPrimFp)DrawClippedPrim,					// Line
	(DrawPrimFp)DrawClippedPrim,					// Flat
	(DrawPrimFp)DrawClippedPrim,					// Flat Lit
	(DrawPrimFp)DrawClippedPrim,					// Gouraud
	(DrawPrimFp)DrawClippedPrim,					// Gouraud Lit
	(DrawPrimFp)DrawClippedPrim,					// Textured
	(DrawPrimFp)DrawClippedPrim,					// Textured Flat Lit
	(DrawPrimFp)DrawClippedPrim,					// Textured Gouraud
	(DrawPrimFp)DrawClippedPrim,					// Textured Gouraud Lit
	(DrawPrimFp)DrawClippedPrim,					// Chroma Textured
	(DrawPrimFp)DrawClippedPrim,					// Chroma Textured Flat Lit
	(DrawPrimFp)DrawClippedPrim,					// Chroma Textured Gouraud
	(DrawPrimFp)DrawClippedPrim,					// Chroma Textured Gouraud Lit

	// With alpha
	(DrawPrimFp)DrawClippedPrim,					// Flat
	(DrawPrimFp)DrawClippedPrim,					// Flat Lit
	(DrawPrimFp)DrawClippedPrim,					// Gouraud
	(DrawPrimFp)DrawClippedPrim,					// Gouraud Lit
	(DrawPrimFp)DrawClippedPrim,					// Textured
	(DrawPrimFp)DrawClippedPrim,					// Textured Flat Lit
	(DrawPrimFp)DrawClippedPrim,					// Textured Gouraud
	(DrawPrimFp)DrawClippedPrim,					// Textured Gouraud Lit
	(DrawPrimFp)DrawClippedPrim,					// Chroma Textured
	(DrawPrimFp)DrawClippedPrim,					// Chroma Textured Flat Lit
	(DrawPrimFp)DrawClippedPrim,					// Chroma Textured Gouraud
	(DrawPrimFp)DrawClippedPrim,					// Chroma Textured Gouraud Lit

	(DrawPrimFp)DrawClippedPrim,					// Bilinear Alpha per Texel Hack
};

const ClipPrimFp ClipPrimWithTexJumpTable[PpolyTypeNum] = 
{
	(ClipPrimFp)ClipPrimPoint,						// Point
	(ClipPrimFp)ClipPrimLine,						// Line
	(ClipPrimFp)ClipPoly,							// Flat
	(ClipPrimFp)ClipPolyL,							// Flat Lit
	(ClipPrimFp)ClipPolyG,							// Gouraud
	(ClipPrimFp)ClipPolyGL,							// Gouraud Lit
	(ClipPrimFp)ClipPolyT,							// Textured
	(ClipPrimFp)ClipPolyTL,							// Textured Flat Lit
	(ClipPrimFp)ClipPolyTG,							// Textured Gouraud
	(ClipPrimFp)ClipPolyTGL,						// Textured Gouraud Lit
	(ClipPrimFp)ClipPolyT,							// Chroma Textured
	(ClipPrimFp)ClipPolyTL,							// Chroma Textured Flat Lit
	(ClipPrimFp)ClipPolyTG,							// Chroma Textured Gouraud
	(ClipPrimFp)ClipPolyTGL,						// Chroma Textured Gouraud Lit

	// With alpha
	(ClipPrimFp)ClipPoly,							// Flat
	(ClipPrimFp)ClipPolyL,							// Flat Lit
	(ClipPrimFp)ClipPolyG,							// Gouraud
	(ClipPrimFp)ClipPolyGL,							// Gouraud Lit
	(ClipPrimFp)ClipPolyAT,							// Textured
	(ClipPrimFp)ClipPolyATL,						// Textured Flat Lit
	(ClipPrimFp)ClipPolyATG,						// Textured Gouraud
	(ClipPrimFp)ClipPolyATGL,						// Textured Gouraud Lit
	(ClipPrimFp)ClipPolyAT,							// Chroma Textured
	(ClipPrimFp)ClipPolyATL,						// Chroma Textured Flat Lit
	(ClipPrimFp)ClipPolyATG,						// Chroma Textured Gouraud
	(ClipPrimFp)ClipPolyATGL,						// Chroma Textured Gouraud Lit

	(ClipPrimFp)ClipPolyT,							// Bilinear Alpha per Texel Hack
};

const ClipPrimFp ClipPrimFogWithTexJumpTable[PpolyTypeNum] = 
{
	(ClipPrimFp)ClipPrimFPoint,						// Point
	(ClipPrimFp)ClipPrimFLine,						// Line
	(ClipPrimFp)ClipPolyF,							// Flat
	(ClipPrimFp)ClipPolyFL,							// Flat Lit
	(ClipPrimFp)ClipPolyFG,							// Gouraud
	(ClipPrimFp)ClipPolyFGL,						// Gouraud Lit
	(ClipPrimFp)ClipPolyFT,							// Textured
	(ClipPrimFp)ClipPolyFTL,						// Textured Flat Lit
	(ClipPrimFp)ClipPolyFTG,						// Textured Gouraud
	(ClipPrimFp)ClipPolyFTGL,						// Textured Gouraud Lit
	(ClipPrimFp)ClipPolyFT,							// Chroma Textured
	(ClipPrimFp)ClipPolyFTL,						// Chroma Textured Flat Lit
	(ClipPrimFp)ClipPolyFTG,						// Chroma Textured Gouraud
	(ClipPrimFp)ClipPolyFTGL,						// Chroma Textured Gouraud Lit

	// With alpha (disallows fog with textures)
	(ClipPrimFp)ClipPolyF,							// Flat
	(ClipPrimFp)ClipPolyFL,							// Flat Lit
	(ClipPrimFp)ClipPolyFG,							// Gouraud
	(ClipPrimFp)ClipPolyFGL,						// Gouraud Lit
	(ClipPrimFp)ClipPolyAT,							// Textured
	(ClipPrimFp)ClipPolyATL,						// Textured Flat Lit
	(ClipPrimFp)ClipPolyATG,						// Textured Gouraud
	(ClipPrimFp)ClipPolyATGL,						// Textured Gouraud Lit
	(ClipPrimFp)ClipPolyAT,							// Chroma Textured
	(ClipPrimFp)ClipPolyATL,						// Chroma Textured Flat Lit
	(ClipPrimFp)ClipPolyATG,						// Chroma Textured Gouraud
	(ClipPrimFp)ClipPolyATGL,						// Chroma Textured Gouraud Lit

	(ClipPrimFp)ClipPolyFT,							// Bilinear Alpha per Texel Hack
};

const ClipPrimFp ClipPrimNoTexJumpTable[PpolyTypeNum] = 
{
	(ClipPrimFp)ClipPrimPoint,						// Point
	(ClipPrimFp)ClipPrimLine,						// Line
	(ClipPrimFp)ClipPoly,							// Flat
	(ClipPrimFp)ClipPolyL,							// Flat Lit
	(ClipPrimFp)ClipPolyG,							// Gouraud
	(ClipPrimFp)ClipPolyGL,							// Gouraud Lit
	(ClipPrimFp)ClipPoly,							// Textured
	(ClipPrimFp)ClipPolyL,							// Textured Flat Lit
	(ClipPrimFp)ClipPolyG,							// Textured Gouraud
	(ClipPrimFp)ClipPolyGL,							// Textured Gouraud Lit
	(ClipPrimFp)ClipPoly,							// Chroma Textured
	(ClipPrimFp)ClipPolyL,							// Chroma Textured Flat Lit
	(ClipPrimFp)ClipPolyG,							// Chroma Textured Gouraud
	(ClipPrimFp)ClipPolyGL,							// Chroma Textured Gouraud Lit

	// With alpha
	(ClipPrimFp)ClipPoly,							// Flat
	(ClipPrimFp)ClipPolyL,							// Flat Lit
	(ClipPrimFp)ClipPolyG,							// Gouraud
	(ClipPrimFp)ClipPolyGL,							// Gouraud Lit
	(ClipPrimFp)ClipPoly,							// Textured
	(ClipPrimFp)ClipPolyL,							// Textured Flat Lit
	(ClipPrimFp)ClipPolyG,							// Textured Gouraud
	(ClipPrimFp)ClipPolyGL,							// Textured Gouraud Lit
	(ClipPrimFp)ClipPoly,							// Chroma Textured
	(ClipPrimFp)ClipPolyL,							// Chroma Textured Flat Lit
	(ClipPrimFp)ClipPolyG,							// Chroma Textured Gouraud
	(ClipPrimFp)ClipPolyGL,							// Chroma Textured Gouraud Lit

	(ClipPrimFp)ClipPoly,							// Bilinear Alpha per Texel Hack
};									

const ClipPrimFp ClipPrimFogNoTexJumpTable[PpolyTypeNum] = 
{
	(ClipPrimFp)ClipPrimFPoint,						// Point
	(ClipPrimFp)ClipPrimFLine,						// Line
	(ClipPrimFp)ClipPolyF,							// Flat
	(ClipPrimFp)ClipPolyFL,							// Flat Lit
	(ClipPrimFp)ClipPolyFG,							// Gouraud
	(ClipPrimFp)ClipPolyFGL,						// Gouraud Lit
	(ClipPrimFp)ClipPolyF,							// Textured
	(ClipPrimFp)ClipPolyFL,							// Textured Flat Lit
	(ClipPrimFp)ClipPolyFG,							// Textured Gouraud
	(ClipPrimFp)ClipPolyFGL,						// Textured Gouraud Lit
	(ClipPrimFp)ClipPolyF,							// Chroma Textured
	(ClipPrimFp)ClipPolyFL,							// Chroma Textured Flat Lit
	(ClipPrimFp)ClipPolyFG,							// Chroma Textured Gouraud
	(ClipPrimFp)ClipPolyFGL,						// Chroma Textured Gouraud Lit

	// With alpha
	(ClipPrimFp)ClipPolyF,							// Flat
	(ClipPrimFp)ClipPolyFL,							// Flat Lit
	(ClipPrimFp)ClipPolyFG,							// Gouraud
	(ClipPrimFp)ClipPolyFGL,						// Gouraud Lit
	(ClipPrimFp)ClipPolyF,							// Textured
	(ClipPrimFp)ClipPolyFL,							// Textured Flat Lit
	(ClipPrimFp)ClipPolyFG,							// Textured Gouraud
	(ClipPrimFp)ClipPolyFGL,						// Textured Gouraud Lit
	(ClipPrimFp)ClipPolyF,							// Chroma Textured
	(ClipPrimFp)ClipPolyFL,							// Chroma Textured Flat Lit
	(ClipPrimFp)ClipPolyFG,							// Chroma Textured Gouraud
	(ClipPrimFp)ClipPolyFGL,						// Chroma Textured Gouraud Lit

	(ClipPrimFp)ClipPolyF,							// Bilinear Alpha per Texel Hack
};

// Draw state table which maps from polygon type to rendering state.
const int RenderStateTableWithNPCTex[PpolyTypeNum] = 
{
	STATE_LIT,										// Point
	STATE_LIT,										// Line

	STATE_LIT,										// Flat
	STATE_LIT,										// Flat Lit
	STATE_GOURAUD,									// Gouraud
	STATE_GOURAUD,									// Gouraud Lit
	STATE_TEXTURE_LIT,								// Textured
	STATE_TEXTURE_LIT,								// Textured Flat Lit
	STATE_TEXTURE_GOURAUD,							// Textured Gouraud
	STATE_TEXTURE_GOURAUD,							// Textured Gouraud Lit
	STATE_CHROMA_TEXTURE_LIT,						// Chroma Textured
	STATE_CHROMA_TEXTURE_LIT,						// Chroma Textured Flat Lit
	STATE_CHROMA_TEXTURE_GOURAUD,					// Chroma Textured Gouraud
	STATE_CHROMA_TEXTURE_GOURAUD,					// Chroma Textured Gouraud Lit

	// With alpha
	STATE_ALPHA_LIT,								// Flat
	STATE_ALPHA_LIT,								// Flat Lit
	STATE_ALPHA_GOURAUD,							// Gouraud
	STATE_ALPHA_GOURAUD,							// Gouraud Lit
	STATE_ALPHA_TEXTURE_LIT,						// Textured
	STATE_ALPHA_TEXTURE_LIT,						// Textured Flat Lit
	STATE_ALPHA_TEXTURE_GOURAUD,					// Textured Gouraud
	STATE_ALPHA_TEXTURE_GOURAUD,					// Textured Gouraud Lit
	STATE_ALPHA_TEXTURE_LIT,						// Chroma Textured
	STATE_ALPHA_TEXTURE_LIT,						// Chroma Textured Flat Lit
	STATE_ALPHA_TEXTURE_GOURAUD,					// Chroma Textured Gouraud
	STATE_ALPHA_TEXTURE_GOURAUD,					// Chroma Textured Gouraud Lit

	STATE_ALPHA_TEXTURE_PERSPECTIVE_CLAMP,			// Bilinear Alpha per Texel Hack
};

const int RenderStateTableWithPCTex[PpolyTypeNum] = 
{
	STATE_LIT,										// Point
	STATE_LIT,										// Line
	
	STATE_LIT,										// Flat
	STATE_LIT,										// Flat Lit
	STATE_GOURAUD,									// Gouraud
	STATE_GOURAUD,									// Gouraud Lit
	STATE_TEXTURE_LIT_PERSPECTIVE,					// Textured
	STATE_TEXTURE_LIT_PERSPECTIVE,					// Textured Flat Lit
	STATE_TEXTURE_GOURAUD_PERSPECTIVE,				// Textured Gouraud
	STATE_TEXTURE_GOURAUD_PERSPECTIVE,				// Textured Gouraud Lit
	STATE_CHROMA_TEXTURE_LIT_PERSPECTIVE,			// Chroma Textured
	STATE_CHROMA_TEXTURE_LIT_PERSPECTIVE,			// Chroma Textured Flat Lit
	STATE_CHROMA_TEXTURE_GOURAUD_PERSPECTIVE,		// Chroma Textured Gouraud
	STATE_CHROMA_TEXTURE_GOURAUD_PERSPECTIVE,		// Chroma Textured Gouraud Lit

	// With alpha
	STATE_ALPHA_LIT,								// Flat
	STATE_ALPHA_LIT,								// Flat Lit
	STATE_ALPHA_GOURAUD,							// Gouraud
	STATE_ALPHA_GOURAUD,							// Gouraud Lit
	STATE_ALPHA_TEXTURE_LIT_PERSPECTIVE,			// Textured
	STATE_ALPHA_TEXTURE_LIT_PERSPECTIVE,			// Textured Flat Lit
	STATE_ALPHA_TEXTURE_GOURAUD_PERSPECTIVE,		// Textured Gouraud
	STATE_ALPHA_TEXTURE_GOURAUD_PERSPECTIVE,		// Textured Gouraud Lit
	STATE_ALPHA_TEXTURE_LIT_PERSPECTIVE,			// Chroma Textured
	STATE_ALPHA_TEXTURE_LIT_PERSPECTIVE,			// Chroma Textured Flat Lit
	STATE_ALPHA_TEXTURE_GOURAUD_PERSPECTIVE,		// Chroma Textured Gouraud
	STATE_ALPHA_TEXTURE_GOURAUD_PERSPECTIVE,		// Chroma Textured Gouraud Lit

	STATE_ALPHA_TEXTURE_PERSPECTIVE_CLAMP,			// Bilinear Alpha per Texel Hack
};

const int RenderStateTableNoTex[PpolyTypeNum] = 
{
	STATE_SOLID,									// Point
	STATE_SOLID,									// Line

	STATE_SOLID,									// Flat
	STATE_SOLID,									// Flat Lit
	STATE_GOURAUD,									// Gouraud
	STATE_GOURAUD,									// Gouraud Lit
	STATE_SOLID,									// Textured
	STATE_SOLID,									// Textured Flat Lit
	STATE_SOLID,									// Textured Gouraud
	STATE_GOURAUD,									// Textured Gouraud Lit
	STATE_SOLID,									// Chroma Textured
	STATE_SOLID,									// Chroma Textured Flat Lit
	STATE_SOLID,									// Chroma Textured Gouraud
	STATE_GOURAUD,									// Chroma Textured Gouraud Lit

	// With alpha
	STATE_ALPHA_SOLID,								// Flat
	STATE_ALPHA_SOLID,								// Flat Lit
	STATE_ALPHA_GOURAUD,							// Gouraud
	STATE_ALPHA_GOURAUD,							// Gouraud Lit
	STATE_ALPHA_SOLID,								// Textured
	STATE_ALPHA_SOLID,								// Textured Flat Lit
	STATE_ALPHA_GOURAUD,							// Textured Gouraud
	STATE_ALPHA_GOURAUD,							// Textured Gouraud Lit
	STATE_ALPHA_SOLID,								// Chroma Textured
	STATE_ALPHA_SOLID,								// Chroma Textured Flat Lit
	STATE_ALPHA_GOURAUD,							// Chroma Textured Gouraud
	STATE_ALPHA_GOURAUD,							// Chroma Textured Gouraud Lit

	STATE_SOLID,									// Bilinear Alpha per Texel Hack
};

// Function to handle trivial accept/reject of potentially clipped primitives.
void DrawClippedPrim(Prim *prim)
{
	int *xyzIdxPtr,*end;
	UInt32 clipFlag,clipFlagAND,clipFlagOR;

	ShiAssert(prim->nVerts > 0);

	xyzIdxPtr = prim->xyz;
	end = xyzIdxPtr + prim->nVerts;

	// Check the clipping flags for all our vertices
	clipFlag = TheStateStack.ClipInfoPool[*xyzIdxPtr++].clipFlag;
	clipFlagAND	= clipFlag;
	clipFlagOR	= clipFlag;

	while(xyzIdxPtr < end)
	{
		clipFlag = TheStateStack.ClipInfoPool[*xyzIdxPtr++].clipFlag;
		clipFlagAND	&= clipFlag;
		clipFlagOR	|= clipFlag;
	};

	// See if we're on, off, or straddling the screen
	if(clipFlagAND)
	{
		// All verts clipped by the same edge, so skip
		return;
	}
	else if(clipFlagOR == ON_SCREEN)
	{
		// All verts on screen, so just draw
		DrawPrimNoClipJumpTable[prim->type](prim);
		return;
	}
	else
	{
		// Do a full clip operation
		ClipPrimJumpTable[prim->type](prim,clipFlagOR);
	}
}
