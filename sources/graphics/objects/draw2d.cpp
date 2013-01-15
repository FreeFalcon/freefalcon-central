/*
** Name: DRAW2D.CPP
** Description:
**		Class description for drawable 2d objects (billboards).  These
**		may (or not) have a sequence of textures to use for animations.
** History:
**		14-Jul-97 (edg)  We go dancing in .....
*/
#include "TimeMgr.h"
#include "TOD.h"
#include "RenderOW.h"
#include "RViewPnt.h"
#include "Tex.h"
#include "falclib\include\fakerand.h"
#include "Draw2d.h"
#include "FalcLib\include\dispopts.h" //JAM 04Oct03
#include "Graphics\DXEngine\DXEngine.h"
#include "Graphics\DXEngine\DXVBManager.h"

//JAM 18Nov03
#include "RealWeather.h"

// HACK: turn off optimizations for this function.  The optimizer
// may have a bug in it.....
#pragma optimize("",off)


#ifdef USE_SH_POOLS
MEM_POOL	Drawable2D::pool;
#endif

// define for preloading all animation textures
// comment out to use on-the-fly textures
// #define PRELOAD_TEX

static float sLOD = 1.0f;
int sGreenMode = 0;
static float sMaxScreenRes = 1.0f;

static	Tcolor	gLight;

#define NUM_TEX_SHEETS	32
#define NUM_TEXTURES_USED	7
#define TEX_UV_DIM		0.246f
Texture gGlobTextures[NUM_TEX_SHEETS];

// for alpha per texel textures
static const int	NUM_APL_TEXTURES = 11;
Texture		gAplTextures[NUM_APL_TEXTURES];
Texture		gAplTexturesGreen[NUM_APL_TEXTURES];

// scatter plotting vertices
/*
#define NUM_SMOKE_SCATTER_FRAMES		5
#define NUM_SMOKE_SCATTER_POINTS		6
#define NUM_FIRE_SCATTER_FRAMES		8
#define NUM_FIRE_SCATTER_POINTS		12
#define NUM_EXPLODE_SCATTER_FRAMES		1
#define NUM_EXPLODE_SCATTER_POINTS		10
*/
#define NUM_SMOKE_SCATTER_FRAMES		1
#define NUM_SMOKE_SCATTER_POINTS		1
#define NUM_FIRE_SCATTER_FRAMES		8
#define NUM_FIRE_SCATTER_POINTS		12
#define NUM_EXPLODE_SCATTER_FRAMES		1
#define NUM_EXPLODE_SCATTER_POINTS		10

#define SCATTER_ZMAX			(5000.0f + 15000.0f * sLOD)
Tpoint gFireScatterPoints[ NUM_FIRE_SCATTER_FRAMES ][ NUM_FIRE_SCATTER_POINTS ];
Tpoint gSmokeScatterPoints[ NUM_SMOKE_SCATTER_FRAMES ][ NUM_SMOKE_SCATTER_POINTS ];
Tpoint gExplodeScatterPoints[ NUM_EXPLODE_SCATTER_FRAMES ][ NUM_EXPLODE_SCATTER_POINTS ];

extern int g_nGfxFix;// MN

typedef struct
{
	float u;
	float v;
} UV;

// each sheet is a 256x256 texture containing 16 64x64 textures
// for animation.  This is a lookup table so we can get the upper left
// corner uv based on anim frame
UV gTexUV[16] =
{
	{ 0.002f, 0.002f },
	{ 0.252f, 0.002f },
	{ 0.502f, 0.002f },
	{ 0.752f, 0.002f },
	{ 0.002f, 0.252f },
	{ 0.252f, 0.252f },
	{ 0.502f, 0.252f },
	{ 0.752f, 0.252f },
	{ 0.002f, 0.502f },
	{ 0.252f, 0.502f },
	{ 0.502f, 0.502f },
	{ 0.752f, 0.502f },
	{ 0.002f, 0.752f },
	{ 0.252f, 0.752f },
	{ 0.502f, 0.752f },
	{ 0.752f, 0.752f },
};


// current light level
static float lightLevel = 1.0f;

Tpoint glowCrossVerts[] =
{
	//		X				Y				Z
	{		0.0f,			0.0f,			-14.0f },
	{		0.0f,			1.0f,			-2.0f },
	{		0.0f,			2.0f,			-1.0f },
	{		0.0f,			14.0f,			 0.0f },
	{		0.0f,			2.0f,			 1.0f },
	{		0.0f,			1.0f,			 2.0f },
	{		0.0f,			0.0f,			 14.0f },
	{		0.0f,		   -1.0f,			 2.0f },
	{		0.0f,		   -2.0f,			 1.0f },
	{		0.0f,		   -14.0f,			 0.0f },
	{		0.0f,		   -2.0f,			-1.0f },
	{		0.0f,		   -1.0f,			-2.0f },
};
int numGlowCrossVerts = sizeof( glowCrossVerts )/ sizeof( Tpoint );

Tpoint glowCircleVerts[20];
int numGlowCircleVerts = sizeof( glowCircleVerts )/ sizeof( Tpoint );

Tpoint glowSquareVerts[4];
int numGlowSquareVerts = sizeof( glowSquareVerts )/ sizeof( Tpoint );

Tpoint glowStarVerts[20];
int numGlowStarVerts = sizeof( glowStarVerts )/ sizeof( Tpoint );

Tpoint coneDim = { 4.5f, 2.0f, 2.0f };

// lens flare stuff
#define NUM_FLARE_CIRCLES		10
Tpoint lensFlareVerts[16];
const int numLensFlareVerts = sizeof( lensFlareVerts )/ sizeof( Tpoint );
float lensAlphas[] =
{
	0.28f,
	0.37f,
	0.20f,
	0.24f,
	0.28f,
	0.30f,
	0.26f,
	0.35f,
	0.25f,
	0.22f,
};
float lensRadius[] =
{
	2.02f,
	3.06f,
	1.05f,
	0.45f,
	0.75f,
	1.38f,
	0.87f,
	6.03f,
	7.09f,
	10.78f,
};

float lensDist[] =
{
	-12.00f,
	-32.00f,
	-70.00f,
	-121.00f,
	-181.00f,
	70.00f,
	125.00f,
	10.00f,
	165.00f,
	105.00f,
};

Tcolor lensRGB[] =
{
	{	1.00f,		1.00f, 		0.60f 	},
	{	1.00f,		0.80f, 		1.00f 	},
	{	0.90f,		1.00f, 		0.80f 	},
	{	1.00f,		0.80f, 		0.80f 	},
	{	1.00f,		0.90f, 		1.00f 	},

	{	0.80f,		0.95f, 		0.60f 	},
	{	1.00f,		0.40f, 		0.80f 	},
	{	1.00f,		1.00f, 		0.50f 	},
	{	1.00f,		0.20f, 		1.00f 	},
	{	0.70f,		0.70f, 		1.00f 	},
};

Tcolor lensCenterRGB[] =
{
	{	0.30f,		0.30f, 		1.00f 	},
	{	0.60f,		0.20f, 		0.30f 	},
	{	0.20f,		0.20f, 		1.00f 	},
	{	0.70f,		0.30f, 		1.00f 	},
	{	0.20f,		0.20f, 		0.80f 	},

	{	1.00f,		0.40f, 		0.90f 	},
	{	0.70f,		0.70f, 		1.00f 	},
	{	0.30f,		1.00f, 		0.90f 	},
	{	0.40f,		0.90f, 		1.00f 	},
	{	0.10f,		1.00f, 		0.60f 	},
};

TYPES2D gTypeTable[] =
{
	// Air explosion
	{
		ANIM_STOP | FADE_START | DO_FIVE_POINTS,			// flags
		1.0f,								// initAlpha
		0.0f,								// fadeRate
		1,									// texture id sequence
		0,									// startTexture
		16,									// numTextures
		20.0f,								// expandRate
		NULL,								// glowVerts
		0,									// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Small Hit Explosion
	{
		ANIM_HALF_RATE | ANIM_STOP | DO_FIVE_POINTS,							// flags
		1.0f,								// initAlpha
		0.0f,								// fadeRate
		0,									// texture id sequence
		0,									// startTexture
		12,									// numTextures
		0.0f,								// expandRate
		NULL,								// glowVerts
		0,									// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Smoke
	{
		ANIM_LOOP | FADE_START | ALPHA_PER_TEXEL,		// flags
		0.9f,  // COBRA - RED - a Little More visible: was 1.0f,								// initAlpha
		.00005f,								// fadeRate
		10,
		12,									// startTexture
		1,									// numTextures
		30.0f,								// expandRate
		NULL,								// glowVerts
		0,			// numGlowVerts
		300.0f,							// maxExpand
	},
	// Flare
	{
		ANIM_LOOP ,	// flags
		1.0f,								// initAlpha
		0.0f,								// fadeRate
		4,
		14,									// startTexture
		1,									// numTextures
		0.0f,								// expandRate
		NULL,								// glowVerts
		0,									// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Air Explosion 2
	{
		ANIM_HALF_RATE | ANIM_STOP | EXPLODE_SCATTER_PLOT | ANIM_NO_CLAMP,							// flags
		0.8f,								// initAlpha
		0.0f,								// fadeRate
		1,
		0,									// startTexture
		16,									// numTextures
		0.0f,								// expandRate
		NULL,								// glowVerts
		NUM_EXPLODE_SCATTER_FRAMES,									// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Smoke Ring
	{
		FADE_START | ANIM_HOLD_LAST ,	// flags
		1.0f,								// initAlpha
		0.0008f,							// fadeRate
		3,
		15,									// startTexture
		1,									// numTextures
		480.0f,								// expandRate
		NULL,								// glowVerts
		0,									// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Small Chem Explosion
	{
		ANIM_HALF_RATE | ANIM_STOP | DO_FIVE_POINTS,							// flags
		1.0f,								// initAlpha
		0.0f,								// fadeRate
		6,									// texture id sequence
		0,									// startTexture
		16,									// numTextures
		0.0f,								// expandRate
		NULL,								// glowVerts
		0,									// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Chem Explosion
	{
		ANIM_HALF_RATE | ANIM_STOP | EXPLODE_SCATTER_PLOT | ANIM_NO_CLAMP,							// flags
		0.8f,								// initAlpha
		0.0f,								// fadeRate
		6,
		0,									// startTexture
		16,									// numTextures
		0.0f,								// expandRate
		NULL,								// glowVerts
		NUM_EXPLODE_SCATTER_FRAMES,			// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Small Debris Explosion
	{
		ANIM_HALF_RATE | ANIM_STOP | DO_FIVE_POINTS,							// flags
		1.0f,								// initAlpha
		0.0f,								// fadeRate
		4,									// texture id sequence
		0,									// startTexture
		14,									// numTextures
		0.0f,								// expandRate
		NULL,								// glowVerts
		0,									// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Debris Explosion
	{
		ANIM_HALF_RATE | ANIM_STOP | EXPLODE_SCATTER_PLOT | ANIM_NO_CLAMP,							// flags
		0.8f,								// initAlpha
		0.0f,								// fadeRate
		4,
		0,									// startTexture
		14,									// numTextures
		0.0f,								// expandRate
		NULL,								// glowVerts
		NUM_EXPLODE_SCATTER_FRAMES,			// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Cloud 5
	{
		ANIM_HOLD_LAST | USES_BB_MATRIX,	// flags
		1.0f,								// initAlpha
		0.0f,								// fadeRate
		3,
		13,									// startTexture
		1,									// numTextures
		0.0f,								// expandRate
		NULL,								// glowVerts
		0,									// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Dustcloud
	{
		ANIM_LOOP | FADE_START | ALPHA_PER_TEXEL,		// flags
		0.8f,								// initAlpha
		.00005f,							// fadeRate
		8,
		13,									// startTexture
		1,									// numTextures
		20.0f,								// expandRate
		NULL,								// glowVerts
		NUM_SMOKE_SCATTER_FRAMES,			// numGlowVerts
		300.0f,							// maxExpand
	},
	// Gunsmoke
	{
		ANIM_LOOP | FADE_START,		// flags
		0.8f,								// initAlpha
		.0001f,								// fadeRate
		6,
		8,									// startTexture
		8,									// numTextures
		20.0f,								// expandRate
		NULL,								// glowVerts
		0,									// numGlowVerts
		300.0f,							// maxExpand
	},
	// Air Smoke 2
	{
		ANIM_LOOP | FADE_START | ALPHA_PER_TEXEL,		// flags
		1.0f,								// initAlpha
		.0002f,								// fadeRate
		10,
		12,									// startTexture
		1,									// numTextures
		20.0f,								// expandRate
		NULL,								// glowVerts
		NUM_SMOKE_SCATTER_FRAMES,			// numGlowVerts
		400.0f,							// maxExpand
	},
	// Glowing explosion Object Square
	{
		FADE_START | ALPHA_DAYLIGHT | ALPHA_BRIGHTEN | GLOW_SPHERE,		// flags
		1.0f,								// initAlpha
		.0006f,								// fadeRate
		NULL,								// texture id sequence
		0, 									// numTextures
		0, 									// numTextures
		0.0f,								// expandRate
		glowSquareVerts,						// glowVerts
		numGlowSquareVerts,					// numGlowVerts
		10000.0f,							// maxExpand
	},
	// missile Glowing Object
	{
		GLOW_SPHERE | GLOW_RAND_POINTS,		// flags
		0.9f,								// initAlpha
		0.0f,								// fadeRate
		NULL,								// texture id sequence
		0, 									// numTextures
		0, 									// numTextures
		0.0f,								// expandRate
		glowStarVerts,						// glowVerts
		numGlowStarVerts,					// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Glowing explosion Object Sphere
	{
		GLOW_SPHERE | ALPHA_BRIGHTEN | ALPHA_DAYLIGHT,						// flags
		0.6f,								// initAlpha
		0.0f,								// fadeRate
		NULL,								// texture id sequence
		0, 									// numTextures
		0, 									// numTextures
		0.0f,								// expandRate
		glowCircleVerts,						// glowVerts
		numGlowCircleVerts,					// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Glowing explosion Object Cross
	{
		GLOW_SPHERE ,						// flags
		1.0f,								// initAlpha
		.0009f,								// fadeRate
		0,									// texture id sequence
		0, 									// numTextures
		0, 									// numTextures
		50.0f,								// expandRate
		glowCrossVerts,						// glowVerts
		numGlowCrossVerts,					// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Crater 1
	{
		ANIM_HOLD_LAST,						// flags
		1.0f,								// initAlpha
		0.0f,								// fadeRate
		3,
		14,									// start Textures
		1,									// numTextures
		0.0f,								// expandRate
		NULL,								// glowVerts
		0,									// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Crater 2
	{
		ANIM_HOLD_LAST,						// flags
		1.0f,								// initAlpha
		0.0f,								// fadeRate
		3,
		14,									// start Textures
		1,									// numTextures
		0.0f,								// expandRate
		NULL,								// glowVerts
		0,									// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Crater 3
	{
		ANIM_HOLD_LAST,						// flags
		1.0f,								// initAlpha
		0.0f,								// fadeRate
		3,
		14,									// start Textures
		1,									// numTextures
		0.0f,								// expandRate
		NULL,								// glowVerts
		0,									// numGlowVerts
		10000.0f,							// maxExpand
	},
	// FIRE
	{
		RAND_START_FRAME | ANIM_LOOP | SEQ_SCATTER_ANIM | FIRE_SCATTER_PLOT | ALPHA_PER_TEXEL,			// flags
		0.9f,								// initAlpha
		0.0f,								// fadeRate
		0,
		0,									// startTexture
		6,									// numTextures
		0.0f,								// expandRate
		NULL,								// glowVerts
		NUM_FIRE_SCATTER_FRAMES,			// numGlowVerts
		10000.0f,							// maxExpand
	},
	// GROUND STRIKE
	{
		USES_TREE_MATRIX,					// flags
		0.8f,								// initAlpha
		0.0f,								// fadeRate
		5,
		0,									// startTexture
		16,									// numTextures
		0.0f,								// expandRate
		NULL,								// glowVerts
		0,									// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Water STRIKE
	{
		USES_TREE_MATRIX,					// flags
		0.8f,								// initAlpha
		0.0f,								// fadeRate
		2,
		0,									// startTexture
		16,									// numTextures
		0.0f,								// expandRate
		NULL,								// glowVerts
		0,									// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Hit Explosion
	{
		ANIM_HALF_RATE | ANIM_STOP | EXPLODE_SCATTER_PLOT | ANIM_NO_CLAMP,							// flags
		0.8f,								// initAlpha
		0.0f,								// fadeRate
		0,
		0,									// startTexture
		12,									// numTextures
		0.0f,								// expandRate
		NULL,								// glowVerts
		NUM_EXPLODE_SCATTER_FRAMES,			// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Sparks
	/*
	{
		ANIM_STOP | USES_BB_MATRIX | ANIM_HALF_RATE,							// flags
		1.0f,								// initAlpha
		0.0f,								// fadeRate
		0,
		12,									// startTexture
		4,									// numTextures
		0.0f,								// expandRate
		NULL,								// glowVerts
		0,									// numGlowVerts
		10000.0f,							// maxExpand
	},
	*/
	// SPARKS explosion Object Star
	{
		FADE_START | ALPHA_DAYLIGHT | ALPHA_BRIGHTEN | GLOW_SPHERE | GLOW_RAND_POINTS,		// flags
		1.0f,								// initAlpha
		.0015f,								// fadeRate
		NULL,								// texture id sequence
		0, 									// numTextures
		0, 									// numTextures
		0.0f,								// expandRate
		glowCircleVerts,						// glowVerts
		numGlowCircleVerts,					// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Artillery Explosion
	{
		USES_TREE_MATRIX | ANIM_STOP,					// flags
		0.8f,								// initAlpha
		0.0f,								// fadeRate
		3,
		0,									// startTexture
		10,									// numTextures
		0.0f,								// expandRate
		NULL,								// glowVerts
		0,									// numGlowVerts
		10000.0f,							// maxExpand
	},
	// shock ring
	{
		USES_TREE_MATRIX | FADE_START | ANIM_HOLD_LAST,	// flags
		0.5f,								// initAlpha
		0.0007f,							// fadeRate
		4,
		15,									// startTexture
		1,									// numTextures
		480.0f,								// expandRate
		NULL,								// glowVerts
		0,									// numGlowVerts
		10000.0f,							// maxExpand
	},
	// long hanging smoke
	{
		ANIM_HALF_RATE | ANIM_LOOP | FADE_START | DO_FIVE_POINTS,		// flags
		1.0f,								// initAlpha
		.00001f,								// fadeRate
		6,
		0,									// startTexture
		8,									// numTextures
		10.0f,								// expandRate
		NULL,								// glowVerts
		0,									// numGlowVerts
		500.0f,							// maxExpand
	},
	// Shaped FIRE DEBRIS
	{
		ANIM_LOOP | TEXTURED_CONE | FADE_START,					// flags
		1.0f,								// initAlpha
		0.0001f,								// fadeRate
		6,
		8,									// startTexture
		8,									// numTextures
		0.0f,								// expandRate
		&coneDim,								// glowVerts
		1,									// numGlowVerts
		10000.0f,							// maxExpand
	},

	// Water CLOUD
	{
		ANIM_LOOP | FADE_START | ALPHA_PER_TEXEL,		// flags
		0.6f,								// initAlpha
		.0001f,								// fadeRate
		7,
		14,									// startTexture
		1,									// numTextures
		20.0f,								// expandRate
		NULL,								// glowVerts
		NUM_SMOKE_SCATTER_FRAMES,			// numGlowVerts
		100.0f,							// maxExpand
	},
	// DARK DEBRIS
	{
		GOURAUD_TRI | GLOW_RAND_POINTS,		 // flags
		1.0f,								// initAlpha
		0.0f,								// fadeRate
		NULL,								// texture id sequence
		0,									// startTexture
		0,									// numTextures
		0.0f,								// expandRate
		NULL,								// glowVerts
		0,									// numGlowVerts
		10000.0f,							// maxExpand
	},
	// FIRE DEBRIS
	{
		GOURAUD_TRI | GLOW_RAND_POINTS,		 // flags
		1.0f,								// initAlpha
		0.0f,								// fadeRate
		NULL,								// texture id sequence
		0,									// startTexture
		0,									// numTextures
		0.0f,								// expandRate
		NULL,								// glowVerts
		0,									// numGlowVerts
		10000.0f,							// maxExpand
	},
	// LIGHT DEBRIS
	{
		GOURAUD_TRI | GLOW_RAND_POINTS,		 // flags
		1.0f,								// initAlpha
		0.0f,								// fadeRate
		NULL,								// texture id sequence
		0,									// startTexture
		0,									// numTextures
		0.0f,								// expandRate
		NULL,								// glowVerts
		0,									// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Glowing explosion Object Sphere
	{
		GLOW_SPHERE | ALPHA_BRIGHTEN | ALPHA_DAYLIGHT,						// flags
		0.8f,								// initAlpha
		0.0009f,								// fadeRate
		NULL,								// texture id sequence
		0, 									// numTextures
		0, 									// numTextures
		10.0f,								// expandRate
		glowCircleVerts,						// glowVerts
		numGlowCircleVerts,					// numGlowVerts
		100.0f,							// maxExpand
	},
	// shock ring -- small
	{
		USES_TREE_MATRIX | FADE_START | ANIM_HOLD_LAST,	// flags
		0.6f,								// initAlpha
		0.0007f,							// fadeRate
		4,
		15,									// startTexture
		1,									// numTextures
		100.0f,								// expandRate
		NULL,								// glowVerts
		0,									// numGlowVerts
		10000.0f,							// maxExpand
	},
	// fast fading cloud
	{
		ANIM_HALF_RATE | ANIM_LOOP | FADE_START | DO_FIVE_POINTS,		// flags
		1.0f,								// initAlpha
		.0005f,								// fadeRate
		6,
		0,									// startTexture
		8,									// numTextures
		60.0f,								// expandRate
		NULL,								// glowVerts
		0,									// numGlowVerts
		10000.0f,							// maxExpand
	},
	// long hanging smoke 2
	{
		ANIM_LOOP | FADE_START | ALPHA_PER_TEXEL,		// flags
		0.5f,								// initAlpha
		.000015f,								// fadeRate
		6,
		12,									// startTexture
		1,									// numTextures
		15.0f,								// expandRate
		NULL,								// glowVerts
		NUM_EXPLODE_SCATTER_FRAMES,			// numGlowVerts
		500.0f,							// maxExpand
	},
	// FLAME
	{
		ANIM_LOOP | DO_FIVE_POINTS,			// flags
		0.7f,								// initAlpha
		0.0f,								// fadeRate
		4,
		0,									// startTexture
		8,									// numTextures
		0.0f,								// expandRate
		NULL,								// glowVerts
		0,									// numGlowVerts
		10000.0f,							// maxExpand
	},
	// FIRE EXPANDING
	{
		FADE_START | RAND_START_FRAME | ANIM_LOOP | SEQ_SCATTER_ANIM | FIRE_SCATTER_PLOT | ALPHA_PER_TEXEL,			// flags
		0.6f,								// initAlpha
		.00045f,								// fadeRate
		0,
		0,									// startTexture
		6,									// numTextures
		30.0f,								// expandRate
		NULL,								// glowVerts
		NUM_FIRE_SCATTER_FRAMES,			// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Dustcloud
	{
		ANIM_LOOP | FADE_START | ALPHA_PER_TEXEL,		// flags
		0.4f,								// initAlpha
		.00004f,							// fadeRate
		8,
		13,									// startTexture
		1,									// numTextures
		30.0f,								// expandRate
		NULL,								// glowVerts
		1,									// numGlowVerts
		50.0f,								// maxExpand
	},
	// fire hot
	{
		ANIM_LOOP | SEQ_SCATTER_ANIM | FIRE_SCATTER_PLOT | ALPHA_PER_TEXEL,			// flags
		0.7f,								// initAlpha
		.0009f,								// fadeRate
		0,
		0,									// startTexture
		2,									// numTextures
		10.0f,								// expandRate
		NULL,								// glowVerts
		NUM_FIRE_SCATTER_FRAMES,			// numGlowVerts
		10000.0f,							// maxExpand
	},
	// fire med
	{
		ANIM_LOOP | SEQ_SCATTER_ANIM | FIRE_SCATTER_PLOT | ALPHA_PER_TEXEL,			// flags
		0.6f,								// initAlpha
		.0009f,								// fadeRate
		2,
		0,									// startTexture
		2,									// numTextures
		20.0f,								// expandRate
		NULL,								// glowVerts
		NUM_FIRE_SCATTER_FRAMES,			// numGlowVerts
		10000.0f,							// maxExpand
	},
	// fire cool
	{
		ANIM_LOOP | SEQ_SCATTER_ANIM | FIRE_SCATTER_PLOT | ALPHA_PER_TEXEL,			// flags
		0.4f,								// initAlpha
		.0009f,								// fadeRate
		4,
		0,									// startTexture
		2,									// numTextures
		30.0f,								// expandRate
		NULL,								// glowVerts
		NUM_FIRE_SCATTER_FRAMES,			// numGlowVerts
		10000.0f,							// maxExpand
	},
	// fire 1
	{
		FADE_LAST | ANIM_LOOP | SEQ_SCATTER_ANIM | SMOKE_SCATTER_PLOT | ALPHA_PER_TEXEL,			// flags
		1.0f,								// initAlpha
		0.0001f,								// fadeRate
		0,
		0,									// startTexture
		2,									// numTextures
		50.0f,								// expandRate
		NULL,								// glowVerts
		1,			// numGlowVerts
		120.0f,							// maxExpand
	},
	// fire 2
	{
		FADE_START | ANIM_LOOP | SEQ_SCATTER_ANIM | FIRE_SCATTER_PLOT | ALPHA_PER_TEXEL,			// flags
		0.6f,								// initAlpha
		0.0001f,								// fadeRate
		0,
		0,									// startTexture
		2,									// numTextures
		20.0f,								// expandRate
		NULL,								// glowVerts
		NUM_FIRE_SCATTER_FRAMES,			// numGlowVerts
		10000.0f,							// maxExpand
	},
	// fire 3
	{
		FADE_START | ANIM_LOOP | SEQ_SCATTER_ANIM | FIRE_SCATTER_PLOT | ALPHA_PER_TEXEL,			// flags
		0.4f,								// initAlpha
		0.0001f,								// fadeRate
		2,
		0,									// startTexture
		2,									// numTextures
		10.0f,								// expandRate
		NULL,								// glowVerts
		NUM_FIRE_SCATTER_FRAMES,			// numGlowVerts
		10000.0f,							// maxExpand
	},
	// fire 4
	{
		FADE_START | ANIM_LOOP | SEQ_SCATTER_ANIM | SMOKE_SCATTER_PLOT | ALPHA_PER_TEXEL,			// flags
		0.9f,								// initAlpha
		0.00007f,								// fadeRate
		0,
		0,									// startTexture
		2,									// numTextures
		30.0f,								// expandRate
		NULL,								// glowVerts
		1,									// numGlowVerts
		10000.0f,							// maxExpand
	},
	// fire 5
	{
		FADE_START | ANIM_LOOP | SEQ_SCATTER_ANIM | FIRE_SCATTER_PLOT | ALPHA_PER_TEXEL,			// flags
		0.9f,								// initAlpha
		0.00007f,								// fadeRate
		0,
		0,									// startTexture
		2,									// numTextures
		30.0f,								// expandRate
		NULL,								// glowVerts
		1,			// numGlowVerts
		10000.0f,							// maxExpand
	},
	// fire 6
	{
		FADE_START | ANIM_LOOP | SEQ_SCATTER_ANIM | FIRE_SCATTER_PLOT | ALPHA_PER_TEXEL,			// flags
		0.8f,								// initAlpha
		0.0001f,								// fadeRate
		0,
		0,									// startTexture
		2,									// numTextures
		3.0f,								// expandRate
		NULL,								// glowVerts
		NUM_FIRE_SCATTER_FRAMES,			// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Fire Smoke
	{
		ANIM_LOOP | FADE_START | ALPHA_PER_TEXEL,				// flags
		0.9f, // COBRA - RED - a Little More visible: was 0.8f,	// initAlpha
		.00005f,												// fadeRate
		6,
		12,									// startTexture
		1,									// numTextures
		30.0f,								// expandRate
		NULL,								// glowVerts
		NUM_SMOKE_SCATTER_FRAMES,			// numGlowVerts
		300.0f,							// maxExpand
	},
	// Trail Smoke
	{
		ANIM_LOOP | FADE_START | ALPHA_PER_TEXEL,		// flags
		1.0f,								// initAlpha
		.0002f,								// fadeRate
		10,
		12,									// startTexture
		1,									// numTextures
		30.0f,								// expandRate
		NULL,								// glowVerts
		NUM_SMOKE_SCATTER_FRAMES,			// numGlowVerts
		40.0f,							// maxExpand
	},
	// Trail Dust
	{
		ANIM_LOOP | FADE_START | ALPHA_PER_TEXEL,					// flags
		0.5f, // COBRA - RED - a Little Less visible: was 0.8f,		// initAlpha
		.0002f,														// fadeRate
		8,
		13,									// startTexture
		1,									// numTextures
		30.0f,								// expandRate
		NULL,								// glowVerts
		NUM_SMOKE_SCATTER_FRAMES,			// numGlowVerts
		50.0f,							// maxExpand
	},
	// fire 7
	{
		ANIM_LOOP | SEQ_SCATTER_ANIM | FIRE_SCATTER_PLOT | ALPHA_PER_TEXEL,			// flags
		0.15f,								// initAlpha
		0.0001f,								// fadeRate
		6,
		0,									// startTexture
		1,									// numTextures
		40.0f,								// expandRate
		NULL,								// glowVerts
		NUM_FIRE_SCATTER_FRAMES,			// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Blue Cloud
	{
		ANIM_LOOP | FADE_START | ALPHA_PER_TEXEL,		// flags
		0.6f,								// initAlpha
		.0003f,								// fadeRate
		9,
		0,									// startTexture
		1,									// numTextures
		10.0f,								// expandRate
		NULL,								// glowVerts
		NUM_SMOKE_SCATTER_FRAMES,			// numGlowVerts
		100.0f,							// maxExpand
	},
	// STEAM CLOUD
	{
		ANIM_LOOP | FADE_START | ALPHA_PER_TEXEL,		// flags
		0.8f,								// initAlpha
		.00005f,								// fadeRate
		7,
		14,									// startTexture
		1,									// numTextures
		40.0f,								// expandRate
		NULL,								// glowVerts
		NUM_SMOKE_SCATTER_FRAMES,			// numGlowVerts
		400.0f,							// maxExpand
	},
	// Ground Flash
	{
		GROUND_GLOW | GLOW_RAND_POINTS | GLOW_SPHERE | ALPHA_BRIGHTEN | ALPHA_DAYLIGHT,						// flags
		0.4f,								// initAlpha
		0.0005f,								// fadeRate
		NULL,								// texture id sequence
		0, 									// numTextures
		0, 									// numTextures
		-170.0f,								// expandRate
		glowCircleVerts,						// glowVerts
		numGlowCircleVerts,					// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Ground Glow
	{
		GROUND_GLOW | GLOW_RAND_POINTS | GLOW_SPHERE | ALPHA_BRIGHTEN | ALPHA_DAYLIGHT,						// flags
		0.2f,								// initAlpha
		0.0f,								// fadeRate
		NULL,								// texture id sequence
		0, 									// numTextures
		0, 									// numTextures
		0.0f,								// expandRate
		glowCircleVerts,						// glowVerts
		numGlowCircleVerts,					// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Missile Ground Glow
	{
		GROUND_GLOW | GLOW_RAND_POINTS | GLOW_SPHERE | ALPHA_BRIGHTEN | ALPHA_DAYLIGHT,						// flags
		0.2f,								// initAlpha
		0.0f,								// fadeRate
		NULL,								// texture id sequence
		0, 									// numTextures
		0, 									// numTextures
		0.0f,								// expandRate
		glowCircleVerts,						// glowVerts
		numGlowCircleVerts,					// numGlowVerts
		10000.0f,							// maxExpand
	},
	// Big Smoke 1
	{
		ANIM_LOOP | FADE_START | ALPHA_PER_TEXEL,		// flags
		1.0f,								// initAlpha
		.00005f,								// fadeRate
		10,
		12,									// startTexture
		1,									// numTextures
		20.0f,								// expandRate
		NULL,								// glowVerts
		0,			// numGlowVerts
		30000.0f,							// maxExpand
	},
	// Big Smoke 2
	{
		ANIM_LOOP | FADE_START | ALPHA_PER_TEXEL,		// flags
		1.0f,								// initAlpha
		.00005f,								// fadeRate
		6,
		12,									// startTexture
		1,									// numTextures
		20.0f,								// expandRate
		NULL,								// glowVerts
		NUM_SMOKE_SCATTER_FRAMES,			// numGlowVerts
		30000.0f,							// maxExpand
	},
	// Big Dust 1
	{
		ANIM_LOOP | FADE_START | ALPHA_PER_TEXEL,		// flags
		1.0f,								// initAlpha
		.00006f,							// fadeRate
		8,
		13,									// startTexture
		1,									// numTextures
		20.0f,								// expandRate
		NULL,								// glowVerts
		NUM_SMOKE_SCATTER_FRAMES,			// numGlowVerts
		30000.0f,							// maxExpand
	},
	// Incendiary Explosion 
	{
		FADE_START | ANIM_LOOP | ANIM_HALF_RATE | EXPLODE_SCATTER_PLOT | ANIM_NO_CLAMP,							// flags
		1.0f,								// initAlpha
		0.0001f,								// fadeRate
		1,
		0,									// startTexture
		16,									// numTextures
		80.0f,								// expandRate
		NULL,								// glowVerts
		NUM_EXPLODE_SCATTER_FRAMES,									// numGlowVerts
		10000.0f,							// maxExpand
	},
		// Water Wake
	{
		ANIM_LOOP | FADE_START | ALPHA_PER_TEXEL,		// flags
		0.6f,								// initAlpha
		0.00001f,								// fadeRate
		7,									// texid
		14,									// startTexture
		1,									// numTextures
		10.0f,								// expandRate
		NULL,								// glowVerts
		NUM_SMOKE_SCATTER_FRAMES,			// numGlowVerts
		100.0f,							// maxExpand
	},

};

const int DRAW2D_MAXTYPES = sizeof(gTypeTable)/sizeof(gTypeTable[0]);

/*
** Name Drawable 2d constructor
** Description:
**		init some vars.  pre-fetch the textures we want to use
*/
Drawable2D::Drawable2D( int type2d, float scale, Tpoint *p )
: DrawableObject( scale )
{
	// sanity check
	ShiAssert( type2d < DRAW2D_MAXTYPES && type2d >= 0 );

	// save type
	type = type2d;

	// get typeData ptr
	typeData = gTypeTable[ type2d ];

	// init curframe
	curFrame = -1;
	curSFrame = 0;
	curBFrame = 0;
	if ( typeData.flags & RAND_START_FRAME )
		startSFrame = rand() % typeData.numGlowVerts;
	else
		startSFrame = 0;

	// set position -- world coords
	position = *p;

	// no transparency to start
	initAlpha = alpha = typeData.initAlpha;

	// scale is equivalent to radius
	realRadius = initRadius = radius = scale;

	// init misc...
	startFade = FALSE;
	hasOrientation = FALSE;
	numObjVerts = 0;
	startTime = 0;
	explicitStartTime = FALSE;

	scale2d = 1.0f;

	// now update the ref count to the textures and load them
	// if not already done
	#ifndef PRELOAD_TEX
	// if ( typeData.glTexId )
	// 	glInsertTexture( *typeData.glTexId, 1 );
	#endif
}

/*
** Name Drawable 2d constructor
** Description:
**		init some vars.  pre-fetch the textures we want to use
*/
Drawable2D::Drawable2D( int type2d, float scale, Tpoint *p, Trotation *rot )
: DrawableObject( scale )
{
	// save type
	type = type2d;

	// sanity check
	ShiAssert( type2d < DRAW2D_MAXTYPES && type2d >= 0 );

	// get typeData ptr
	typeData = gTypeTable[ type2d ];


	// init curframe
	curFrame = -1;
	curSFrame = 0;
	curBFrame = 0;
	if ( typeData.flags & RAND_START_FRAME )
		startSFrame = rand() % typeData.numGlowVerts;
	else
		startSFrame = 0;

	// set position -- world coords
	position = *p;

	// no transparency to start
	initAlpha = alpha = typeData.initAlpha;

	// scale is equivalent to radius
	realRadius = initRadius = radius = scale;

	// init misc...
	startFade = FALSE;
	numObjVerts = 0;
	startTime = 0;
	explicitStartTime = FALSE;
	scale2d = 1.0f;

	// now update the ref count to the textures and load them
	// if not already done
	#ifndef PRELOAD_TEX
	// if ( typeData.glTexId )
	// 	glInsertTexture( *typeData.glTexId, 1 );
	#endif

	orientation = *rot;
	hasOrientation = TRUE;
}


/*
** Name Drawable 2d constructor
** Description:
**		init some vars.  pre-fetch the textures we want to use
**		This constructor takes an array of verts in objects space.
**		Valid number is 3 (triangle) or 4 (trapezoid).
**		Specification starts with bottom left vert and goes clockwise.
*/
Drawable2D::Drawable2D( int type2d, float scale, Tpoint *p, int nVerts, Tpoint *verts, Tpoint *uvs)
: DrawableObject( scale )
{
	int i;

	// save type
	type = type2d;

	// sanity check
	ShiAssert( type2d < DRAW2D_MAXTYPES && type2d >= 0 );

	// at the moment we only allow 4 verts
	ShiAssert( nVerts == 4 );

	// get typeData ptr
	typeData = gTypeTable[ type2d ];

	// init curframe
	curFrame = -1;
	curSFrame = 0;
	curBFrame = 0;
	if ( typeData.flags & RAND_START_FRAME )
		startSFrame = rand() % typeData.numGlowVerts;
	else
		startSFrame = 0;

	// set position -- world coords
	position = *p;

	// no transparency to start
	initAlpha = alpha = typeData.initAlpha;

	// scale is equivalent to radius
	realRadius = initRadius = radius = scale;

	// init misc...
	startFade = FALSE;
	numObjVerts = 0;
	hasOrientation = FALSE;
	numObjVerts = nVerts;
	startTime = 0;
	explicitStartTime = FALSE;
	scale2d = 1.0f;

	// setup the verts
	for ( i = 0; i < numObjVerts; i++ )
	{
		oVerts[i] = verts[i];
		uvCoords[i] = uvs[i];
	}

	// now update the ref count to the textures and load them
	// if not already done
	#ifndef PRELOAD_TEX
	// if ( typeData.glTexId )
	// 	glInsertTexture( *typeData.glTexId, 1 );
	#endif

}



/*
** Name Drawable 2d destructor
** Description:
**		release the textures we used
*/
Drawable2D::~Drawable2D( void )
{
	#ifndef PRELOAD_TEX
	// if ( typeData.glTexId )
	// 	glDeleteTexture( *typeData.glTexId, 0 );
	#endif
}



/*
** Name: SetPosition
** Description:
**		Change the position of the object
*/
void Drawable2D::SetPosition( Tpoint *p )
{
	// Update the location of this object
	position = *p;

}

/*
** Name: GetAlphaTimeToLive
** Description:
**		Based on the fade rate and init alpha, will return approximately
**		how long (in secs) the effect will list
*/
float Drawable2D::GetAlphaTimeToLive( void )
{
	float rate;

	// no fading
	if ( typeData.fadeRate == 0.0f )
		return 0.0f;

	typeData.fadeRate*=(1.0f+(float)((rand()&0x07)-4)/10.0f);

	rate = initAlpha/( 1000.0f * typeData.fadeRate );

	if ( type == DRAW2D_FIRE1 )
		rate += 3.0f;

	return rate;
}

/*
** Name: Update
** Description:
**		Change the position, orientation of the object
*/

void Drawable2D::Update( const Tpoint *pos, const Trotation *rot )
{

	// Update the location of this object
	position = *pos;
	orientation = *rot;

}




/*
** Name: Draw
** Description:
**		Sequence thru the texture animation and draw a square.
*/
void Drawable2D::Draw( class RenderOTW *renderer, int LOD )
{
	Tpoint				ws={0.0F};
	ThreeDVertex		v0, v1, v2, v3, spos;
	Tpoint				upv={0.0F}, leftv={0.0F};
	Tpoint				dl={0.0F}, du={0.0F};
	DWORD				now=0;
	int					dT=0;
	BOOL				doFivePoints = FALSE;


	// force green mode (debugging)
	// sGreenMode = 15;


	// special case of object
	if ( typeData.flags & GLOW_SPHERE )
	{
		DrawGlowSphere( renderer, LOD );
		return;
	}

	// special case of object
	if ( typeData.flags & GOURAUD_TRI )
	{
		DrawGouraudTri( renderer, LOD );
		return;
	}

	spos.x = 0.0F;
	spos.y = 0.0F;
	spos.r = 0.0F;
	spos.g = 0.0F;
	spos.b = 0.0F;
	spos.a = 0.0F;	
	spos.u = 0.0F;
	spos.v = 0.0F;
	spos.q = 0.0F; 
	spos.clipFlag = 0;
	spos.csX = 0.0F;
	spos.csY = 0.0F;
	spos.csZ = 0.0F;	
	v0.csZ = 0.0F;
	v1.csZ = 0.0F;
	v2.csZ = 0.0F;
	v3.csZ = 0.0F;

	// see if we should do 5 points for billboard
	if ( typeData.flags & DO_FIVE_POINTS )
	{
		float scaleZ;

		renderer->TransformPoint( &position,  &spos );
		if ( spos.csZ < 1.0f )
			return;
		scaleZ = (SCATTER_ZMAX - spos.csZ)/SCATTER_ZMAX;
		if ( scaleZ < 0.0f ) scaleZ = 0.0f;
		scaleZ *= scaleZ;
		doFivePoints = ( scaleZ * sLOD > 0.5f );
	}


	// Get the curent time
	now = TheTimeManager.GetClockTime();
	if ( explicitStartTime == FALSE )
	{
		startTime = now;
		firstFrame = 0;
	}


	/*
	** what's our new frame going to be?
	*/
	dT = now - startTime;
	// time going backwards, and prior to start time return.....
	if ( dT < 0 )
		return;

	//  get the texture....
	if ( typeData.flags & ALPHA_PER_TEXEL )
	{
		if ( curFrame < 0 )
		{
			// ok to start
			if ( explicitStartTime == TRUE )
				expandStartTime = startTime;
			else
				expandStartTime = now;

			if ( typeData.flags & FADE_START )
			{
				if ( explicitStartTime == TRUE )
					alphaStartTime = startTime;
				else
					alphaStartTime = now;
				startFade = TRUE;
			}

			// now we can say start time is true if not already
			explicitStartTime = TRUE;
		}
		curFrame = GetAnimFrame( dT, startTime );

		if ( sGreenMode )
			curTex = &gAplTexturesGreen[ typeData.texId + curFrame];
		else
			curTex = &gAplTextures[ typeData.texId + curFrame];
	}
	else
	{
		curTex = &gGlobTextures[ typeData.texId + sGreenMode ];
	
		// have we started yet?
		if ( curFrame < 0 )
		{
			if ( firstFrame == typeData.numTextures )
			{
				firstFrame = GetAnimFrame( dT, startTime );
				if ( firstFrame == typeData.numTextures )
					return;
			}
	
			// ok to start
			if ( explicitStartTime == TRUE )
				expandStartTime = startTime;
			else
				expandStartTime = now;

			curFrame = firstFrame;

			if ( typeData.flags & FADE_START )
			{
				if ( explicitStartTime == TRUE )
					alphaStartTime = startTime;
				else
					alphaStartTime = now;
				startFade = TRUE;
			}

			// now we can say start time is true if not already
			explicitStartTime = TRUE;
		}
		else
		{
			curFrame = GetAnimFrame( dT, startTime );
		}


		// reached the end? and should we hold it?
		if (  curFrame == typeData.numTextures )
		{
			if ( typeData.flags & ANIM_HOLD_LAST )
			{
				curFrame--;
			}
			else if ( typeData.flags & ANIM_NO_CLAMP )
			{
				// do nothing
			}
			else
			{
				return;
			}
		}
	}


	if ( startFade == TRUE )
	{
		// do fade... 10 seconds until nothing...
		dT = now - alphaStartTime;
		alpha = initAlpha - ((float)( dT )) * typeData.fadeRate;
		if ( alpha <= 0.05f )
			return;
		if ( alpha > initAlpha )
			alpha = initAlpha;
	}

	// do expansion
	dT = now - expandStartTime;
	realRadius = initRadius + ((float)( dT )) * typeData.expandRate * 0.001f;
	if ( realRadius > typeData.maxExpand )
		realRadius = typeData.maxExpand;
	radius = realRadius;
	if ( realRadius <= 0.0f )
		return;


	// sanity check since time can now go backwards!
	if ( !(typeData.flags & ANIM_NO_CLAMP ) )
	{
		if ( curFrame >= typeData.numTextures )
		{
			curFrame = typeData.numTextures - 1;
		}
		else if ( curFrame < 0 )
		{
			curFrame = 0;
		}
	}

	if ( typeData.flags & (FIRE_SCATTER_PLOT | SMOKE_SCATTER_PLOT | EXPLODE_SCATTER_PLOT) )
	{
		if ( typeData.flags & ALPHA_PER_TEXEL )
			APLScatterPlot( renderer );
		else
			ScatterPlot( renderer );
		return;
	}

	if ( typeData.flags & (TEXTURED_CONE) )
	{
		DrawTexturedCone( renderer, LOD );
		return;
	}



	// if we don't have object space verts then the word verts
	// are determined by using position and camera/orientation matrix
	// vectors
	if ( numObjVerts == 0 )
	{
		// square billboard always facing camera and aligneed with camera
		if ( hasOrientation == FALSE )
		{
			// get the left and up vectors for the camera
			renderer->GetLeft( &leftv );
			renderer->GetUp( &upv );
		}
		else // polygon square oriented in world
		{
			// use orientation matrix to get vectors
			// use x and y row vectors
			leftv.x = -orientation.M11;
			leftv.y = -orientation.M12;
			leftv.z = -orientation.M13;
			upv.x = orientation.M21;
			upv.y = orientation.M22;
			upv.z = orientation.M23;
		}
	
		// based on position and the camera vectors, get the 4 corners of
		// the square.
		// NOTE:  Could just use billboard matrix to do this...
		dl.x = leftv.x * scale2d * realRadius;
		dl.y = leftv.y * scale2d * realRadius;
		dl.z = leftv.z * scale2d * realRadius;
		du.x = upv.x * scale2d * realRadius;
		du.y = upv.y * scale2d * realRadius;
		du.z = upv.z * scale2d * realRadius;
	
		ws.x = position.x + dl.x - du.x;
		ws.y = position.y + dl.y - du.y;
		ws.z = position.z + dl.z - du.z;
		renderer->TransformPoint( &ws,  &v0 );

		// immediately cull any non-oriented billboards
		// that are behind near clip
		if ( hasOrientation == FALSE && v0.csZ < 1.0f )
			return;
	
		ws.x = position.x + dl.x + du.x;
		ws.y = position.y + dl.y + du.y;
		ws.z = position.z + dl.z + du.z;
		renderer->TransformPoint( &ws,  &v1 );

		ws.x = position.x - dl.x + du.x;
		ws.y = position.y - dl.y + du.y;
		ws.z = position.z - dl.z + du.z;
		renderer->TransformPoint( &ws,  &v2 );
	
		ws.x = position.x - dl.x - du.x;
		ws.y = position.y - dl.y - du.y;
		ws.z = position.z - dl.z - du.z;
		renderer->TransformPoint( &ws,  &v3 );


		if ( typeData.flags & ALPHA_PER_TEXEL )
		{
			v1.u = 0.0f;
			v1.v = 0.0f;
	
			v2.u = 1.0f;
			v2.v = 0.0f;
	
			v3.u = 1.0f;
			v3.v = 1.0f;
	
			v0.u = 0.0f;
			v0.v = 1.0f;
		}
		else
		{
			v1.u = gTexUV[ typeData.startTexture + curFrame ].u;
			v1.v = gTexUV[ typeData.startTexture + curFrame ].v;
	
			v2.u = v1.u + TEX_UV_DIM;
			v2.v = v1.v;
	
			v3.u = v1.u + TEX_UV_DIM;
			v3.v = v1.v + TEX_UV_DIM;
	
			v0.u = v1.u;
			v0.v = v1.v + TEX_UV_DIM;
		}
		// u,v texture coords
		v0.q = v0.csZ * 0.001f;
		v1.q = v1.csZ * 0.001f;
		v2.q = v2.csZ * 0.001f;
		v3.q = v3.csZ * 0.001f;
	}
	else
	{
		Tpoint viewLoc;
		Tpoint os;
		// object space verts were specified.  

		// at the moment 4 verts are required -- although at some point
		// we may wish to generalize this to 3 or more....


		// otay.  now we xform depending on the matrix flag specified
		if ( typeData.flags & USES_BB_MATRIX )
		{
			// get the position of this object in view space
			renderer->TransformPointToView( &position,  &viewLoc );
			os.x = oVerts[0].x * scale2d * realRadius;
			os.y = oVerts[0].y * scale2d * realRadius;
			os.z = oVerts[0].z * scale2d * realRadius;
			renderer->TransformBillboardPoint( &os,  &viewLoc, &v0 );
			os.x = oVerts[1].x * scale2d * realRadius;
			os.y = oVerts[1].y * scale2d * realRadius;
			os.z = oVerts[1].z * scale2d * realRadius;
			renderer->TransformBillboardPoint( &os,  &viewLoc, &v1 );
			os.x = oVerts[2].x * scale2d * realRadius;
			os.y = oVerts[2].y * scale2d * realRadius;
			os.z = oVerts[2].z * scale2d * realRadius;
			renderer->TransformBillboardPoint( &os,  &viewLoc, &v2 );
			os.x = oVerts[3].x * scale2d * realRadius;
			os.y = oVerts[3].y * scale2d * realRadius;
			os.z = oVerts[3].z * scale2d * realRadius;
			renderer->TransformBillboardPoint( &os,  &viewLoc, &v3 );
		}
		else if ( typeData.flags & USES_TREE_MATRIX )
		{
			// get the position of this object in view space
			renderer->TransformPointToView( &position,  &viewLoc );

			os.x = oVerts[0].x * scale2d * realRadius;
			os.y = oVerts[0].y * scale2d * realRadius;
			os.z = oVerts[0].z * scale2d * realRadius;
			renderer->TransformTreePoint( &os,  &viewLoc, &v0 );
			os.x = oVerts[1].x * scale2d * realRadius;
			os.y = oVerts[1].y * scale2d * realRadius;
			os.z = oVerts[1].z * scale2d * realRadius;
			renderer->TransformTreePoint( &os,  &viewLoc, &v1 );
			os.x = oVerts[2].x * scale2d * realRadius;
			os.y = oVerts[2].y * scale2d * realRadius;
			os.z = oVerts[2].z * scale2d * realRadius;
			renderer->TransformTreePoint( &os,  &viewLoc, &v2 );
			os.x = oVerts[3].x * scale2d * realRadius;
			os.y = oVerts[3].y * scale2d * realRadius;
			os.z = oVerts[3].z * scale2d * realRadius;
			renderer->TransformTreePoint( &os,  &viewLoc, &v3 );

		}
		else  
		{
			// at the moment must be either BB or TREE Matrix
			ShiWarning( "Bad Type" );
		}

		// u,v texture coords
		v0.u = uvCoords[0].x * TEX_UV_DIM;	v0.v = uvCoords[0].y * TEX_UV_DIM;
		v1.u = uvCoords[1].x * TEX_UV_DIM;	v1.v = uvCoords[1].y * TEX_UV_DIM;
		v2.u = uvCoords[2].x * TEX_UV_DIM;	v2.v = uvCoords[2].y * TEX_UV_DIM;
		v3.u = uvCoords[3].x * TEX_UV_DIM;	v3.v = uvCoords[3].y * TEX_UV_DIM;

		v0.u += gTexUV[ typeData.startTexture + curFrame ].u;
		v0.v += gTexUV[ typeData.startTexture + curFrame ].v;
		v1.u += gTexUV[ typeData.startTexture + curFrame ].u;
		v1.v += gTexUV[ typeData.startTexture + curFrame ].v;
		v2.u += gTexUV[ typeData.startTexture + curFrame ].u;
		v2.v += gTexUV[ typeData.startTexture + curFrame ].v;
		v3.u += gTexUV[ typeData.startTexture + curFrame ].u;
		v3.v += gTexUV[ typeData.startTexture + curFrame ].v;

		// u,v texture coords
		v0.q = v0.csZ * 0.001f;
		v1.q = v1.csZ * 0.001f;
		v2.q = v2.csZ * 0.001f;
		v3.q = v3.csZ * 0.001f;
	}

//	if( renderer->GetAlphaMode() )
//	{
		if( !sGreenMode ) //JAM - FIXME
		{
			if(typeData.flags & USES_TREE_MATRIX)
				renderer->context.RestoreState(STATE_ALPHA_TEXTURE_GOURAUD_PERSPECTIVE);
			else
				renderer->context.RestoreState(STATE_ALPHA_TEXTURE_GOURAUD);
		}
		else
		{
			if(typeData.flags & USES_TREE_MATRIX)
				renderer->context.RestoreState(STATE_ALPHA_TEXTURE_GOURAUD_PERSPECTIVE);
			else
				renderer->context.RestoreState(STATE_ALPHA_TEXTURE_GOURAUD);
		}
/*	}
	else
	{
		doFivePoints = FALSE;

		if(typeData.flags & USES_TREE_MATRIX)
			renderer->context.RestoreState(STATE_ALPHA_TEXTURE_PERSPECTIVE);
		else
			renderer->context.RestoreState(STATE_ALPHA_TEXTURE);
	}
*/
	if( !sGreenMode ) //JAM - FIXME
	{
		if( renderer->GetFilteringMode() )
		{
			renderer->context.SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR);
//			renderer->context.InvalidateState();
		}
	}

	renderer->context.SetState(MPR_STA_ALPHA_OP_FUNCTION,MPR_TO_MODULATE);
	renderer->context.SelectTexture1(curTex->TexHandle());


	// color and alpha
	if ( doFivePoints )
	{
		spos.a = alpha;
		spos.r = 1.0f;
		spos.g = 1.0f;
		spos.b = 1.0f;
		spos.u = v1.u + TEX_UV_DIM * 0.5f;
		spos.v = v1.v + TEX_UV_DIM * 0.5f;
		spos.q = spos.csZ * 0.001f;
		v0.a = v1.a = 0.0f;
		v2.a = v3.a = 0.0f;
		// randomly tweak the edge RGB's to give a glowy kind of effect
		v0.r = v2.r = v3.r = v1.r = 0.1f + NRANDPOS * 0.8f;
		v0.g = v2.g = v3.g = v1.g = 0.0f;
		v0.b = v2.b = v3.b = v1.b = 0.1f + NRANDPOS * 0.8f;

		// Draw the 4 tris
		renderer->DrawTriangle( &spos, &v0, &v1,  CULL_ALLOW_ALL );
		renderer->DrawTriangle( &spos, &v1, &v2,  CULL_ALLOW_ALL );
		renderer->DrawTriangle( &spos, &v2, &v3,  CULL_ALLOW_ALL );
		renderer->DrawTriangle( &spos, &v3, &v0,  CULL_ALLOW_ALL );
	}
	else
	{
		// color and alpha

		// if this type uses 5 points and its scaled to not at the moment,
		// adjust alpha so that it appears more alpha'd
		if ( typeData.flags & DO_FIVE_POINTS )
		{
			v0.a = v1.a = alpha * 0.65f;
			v2.a = v3.a = alpha * 0.65f;
		}
		else
		{
			v0.a = v1.a = alpha;
			v2.a = v3.a = alpha;
		}
		v0.r = v1.r = 1.0f;
		v0.g = v1.g = 1.0f;
		v0.b = v1.b = 1.0f;
		v2.r = v3.r = 1.0f;
		v2.g = v3.g = 1.0f;
		v2.b = v3.b = 1.0f;

		// Draw the polygon
		renderer->DrawSquare( &v0, &v1, &v2, &v3, CULL_ALLOW_ALL );
	}

}



/*
** Name: SetupTexturesOnDevice
** Description:
**		This is texture setup for 2d animations
*/
void Drawable2D::SetupTexturesOnDevice(DXContext *rc)
{
	int i, j;
	char texfile[64];
	float alp;
	int intalp;
	int r,g,b;

	sLOD = 1.0f;
	sGreenMode = 0;

	for ( i = 0; i < NUM_TEX_SHEETS; i++ )
	{
		if ( i >= 0 && i < NUM_TEXTURES_USED || i >= 15 && i < 15 + NUM_TEXTURES_USED )
		{
			sprintf( texfile, "bom00%02d.gif", i+1 );

			// it looks like the way we share a palette is to set the texture
			// palette prior to LoadAndCreate
			if ( i > 0 && i < NUM_TEXTURES_USED )
			{
				gGlobTextures[i].SetPalette(gGlobTextures[0].GetPalette());
			}
			else if ( i > 15 && i < 15 + NUM_TEXTURES_USED )
			{
				gGlobTextures[i].SetPalette(gGlobTextures[15].GetPalette());
			}

			if ( i == 0 )
			{
				gGlobTextures[0].LoadAndCreate( texfile, MPR_TI_CHROMAKEY | MPR_TI_PALETTE | MPR_TI_ALPHA );
				Palette *globPal = gGlobTextures[0].GetPalette();
				for ( j = 0; j < 256; j++ )
				{
					intalp = (globPal->paletteData[j] >> 24);
					globPal->paletteData[j] &= 0x00ffffff;
					if ( j == 0 )
						continue;
					alp = 255.0f * 0.5f +  255.0f * 0.5f * NRANDPOS;
					intalp = FloatToInt32( alp );

					r = (globPal->paletteData[j] & 0x000000ff);
					g = (globPal->paletteData[j] & 0x0000ff00) >> 8;
					b = (globPal->paletteData[j] & 0x00ff0000) >> 16;

					intalp = max( ( r + b + g )/3, 120 );
					globPal->paletteData[j] |= ( intalp << 24 );
				}
				globPal->UpdateMPR( globPal->paletteData );
			}
			else if ( i < NUM_TEXTURES_USED )
			{
				gGlobTextures[i].LoadAndCreate( texfile, MPR_TI_CHROMAKEY | MPR_TI_PALETTE | MPR_TI_ALPHA );
			}
			else
			{
				gGlobTextures[i].LoadAndCreate( texfile, MPR_TI_CHROMAKEY | MPR_TI_PALETTE );
			}
		}
	}

	// setup the scatter plotting
	for ( i = 0; i < NUM_FIRE_SCATTER_FRAMES; i++ )
	{
		for ( j = 0; j < NUM_FIRE_SCATTER_POINTS; j++ )
		{
			if ( i == 0 )
			{
				gFireScatterPoints[i][j].x = (float)((float)rand()/(float)RAND_MAX);
				if ( rand() & 1 )
					gFireScatterPoints[i][j].x = -gFireScatterPoints[i][j].x;
				gFireScatterPoints[i][j].y = (float)((float)rand()/(float)RAND_MAX);
				if ( rand() & 1 )
					gFireScatterPoints[i][j].y = -gFireScatterPoints[i][j].y;
				gFireScatterPoints[i][j].z = 0.3f + (float)((float)rand()/(float)RAND_MAX) * 0.7f;
										
				continue;
			}
			gFireScatterPoints[i][j].x = gFireScatterPoints[i-1][j].x;
			gFireScatterPoints[i][j].y = gFireScatterPoints[i-1][j].y - 0.2f;
			gFireScatterPoints[i][j].z = gFireScatterPoints[i-1][j].z + 0.1f;
			if ( gFireScatterPoints[i][j].y < -1.0f )
			{
				gFireScatterPoints[i][j].y += 2.0f;
				gFireScatterPoints[i][j].z = 0.3f + (float)((float)rand()/(float)RAND_MAX) * 0.7f;
			}
		}
	}

	/*
	// setup the scatter plotting
	for ( i = 0; i < NUM_SMOKE_SCATTER_FRAMES; i++ )
	{
		for ( j = 0; j < NUM_SMOKE_SCATTER_POINTS; j++ )
		{
			if ( i == 0 )
			{
				gSmokeScatterPoints[i][j].x = (float)((float)rand()/(float)RAND_MAX);
				if ( rand() & 1 )
					gSmokeScatterPoints[i][j].x = -gSmokeScatterPoints[i][j].x;
				gSmokeScatterPoints[i][j].y = (float)((float)rand()/(float)RAND_MAX);
				if ( rand() & 1 )
					gSmokeScatterPoints[i][j].y = -gSmokeScatterPoints[i][j].y;
				gSmokeScatterPoints[i][j].z = (float)((float)rand()/(float)RAND_MAX) * 0.3f + 0.3f;
				continue;
			}

			gSmokeScatterPoints[i][j].x = gSmokeScatterPoints[i-1][j].x;
			gSmokeScatterPoints[i][j].y = gSmokeScatterPoints[i-1][j].y;
			gSmokeScatterPoints[i][j].z = gSmokeScatterPoints[i-1][j].z + 0.10f;
		}
	}
	for ( i = 0; i < NUM_FIRE_SCATTER_FRAMES; i++ )
	{
		for ( j = 0; j < NUM_FIRE_SCATTER_POINTS; j++ )
		{
			if ( i == 0 )
			{
				if ( j & 1 )
				{
					gFireScatterPoints[i][j].x = (float)((float)rand()/(float)RAND_MAX);
					if ( rand() & 1 )
						gFireScatterPoints[i][j].x = -gFireScatterPoints[i][j].x;
					gFireScatterPoints[i][j].y = (float)((float)rand()/(float)RAND_MAX);
					if ( rand() & 1 )
						gFireScatterPoints[i][j].y = -gFireScatterPoints[i][j].y;
					gFireScatterPoints[i][j].z = 0.9f + (float)((float)rand()/(float)RAND_MAX) * 0.3f;
				}
				else
				{
					gFireScatterPoints[i][j].x = (float)((float)rand()/(float)RAND_MAX);
					if ( rand() & 1 )
						gFireScatterPoints[i][j].x = -gFireScatterPoints[i][j].x;
					gFireScatterPoints[i][j].y = (float)((float)rand()/(float)RAND_MAX);
					if ( rand() & 1 )
						gFireScatterPoints[i][j].y = -gFireScatterPoints[i][j].y;
					gFireScatterPoints[i][j].z = 0.3f + (float)((float)rand()/(float)RAND_MAX) * 0.3f;
				}
										
				continue;
			}

			if ( j & 1 )
			{
				gFireScatterPoints[i][j].x = gFireScatterPoints[i-1][j].x;
				gFireScatterPoints[i][j].y = gFireScatterPoints[i-1][j].y + 0.2f;
				gFireScatterPoints[i][j].z = gFireScatterPoints[i-1][j].z * 0.8f;
				if ( gFireScatterPoints[i][j].y > 1.0f )
				{
					gFireScatterPoints[i][j].y -= 2.0f;
					gFireScatterPoints[i][j].z = 0.9f + (float)((float)rand()/(float)RAND_MAX) * 0.3f;
				}
			}
			else
			{
				gFireScatterPoints[i][j].x = gFireScatterPoints[i-1][j].x;
				gFireScatterPoints[i][j].y = gFireScatterPoints[i-1][j].y - 0.2f;
				gFireScatterPoints[i][j].z = gFireScatterPoints[i-1][j].z * 1.2f;
				if ( gFireScatterPoints[i][j].y < -1.0f )
				{
					gFireScatterPoints[i][j].y += 2.0f;
					gFireScatterPoints[i][j].z = 0.3f + (float)((float)rand()/(float)RAND_MAX) * 0.3f;
				}
			}
		}
	}
	*/

	// setup the scatter plotting
	for ( i = 0; i < NUM_SMOKE_SCATTER_FRAMES; i++ )
	{
		for ( j = 0; j < NUM_SMOKE_SCATTER_POINTS; j++ )
		{
			if ( i == 0 )
			{
				if ( j == 0 )
				{
					gSmokeScatterPoints[i][j].x = 0.0f;
					gSmokeScatterPoints[i][j].y = 0.0f;
					gSmokeScatterPoints[i][j].z = 1.0f;
				}
				else
				{
					gSmokeScatterPoints[i][j].x = (float)((float)rand()/(float)RAND_MAX) * 0.3f + 0.7f;
					if ( rand() & 1 )
						gSmokeScatterPoints[i][j].x = -gSmokeScatterPoints[i][j].x;
					gSmokeScatterPoints[i][j].y = (float)((float)rand()/(float)RAND_MAX) * 0.3f + 0.7f;
					if ( rand() & 1 )
						gSmokeScatterPoints[i][j].y = -gSmokeScatterPoints[i][j].y;
					gSmokeScatterPoints[i][j].z = (float)((float)rand()/(float)RAND_MAX) * 0.3f + 0.3f;
				}
				continue;
			}

			if ( j == 0 )
			{
				gSmokeScatterPoints[i][j].x = gSmokeScatterPoints[i-1][j].x;
				gSmokeScatterPoints[i][j].y = gSmokeScatterPoints[i-1][j].y;
				gSmokeScatterPoints[i][j].z = gSmokeScatterPoints[i-1][j].z;
			}
			else
			{
				gSmokeScatterPoints[i][j].x = gSmokeScatterPoints[i-1][j].x;
				gSmokeScatterPoints[i][j].y = gSmokeScatterPoints[i-1][j].y;
				gSmokeScatterPoints[i][j].z = gSmokeScatterPoints[i-1][j].z + 0.10f;
			}
			if ( gSmokeScatterPoints[i][j].z > 1.5f )
				gSmokeScatterPoints[i][j].z = (float)((float)rand()/(float)RAND_MAX) * 0.3f + 0.3f;
		}
	}

	// setup the scatter plotting
	for ( i = 0; i < NUM_EXPLODE_SCATTER_FRAMES; i++ )
	{
		for ( j = 0; j < NUM_EXPLODE_SCATTER_POINTS; j++ )
		{
			if ( j == 0 )
			{
				gExplodeScatterPoints[i][j].x = 0.0f;
				gExplodeScatterPoints[i][j].y = 0.0f;
				gExplodeScatterPoints[i][j].z = 0.80f;
				continue;
			}
			gExplodeScatterPoints[i][j].x = (float)((float)rand()/(float)RAND_MAX);
			if ( rand() & 1 )
				gExplodeScatterPoints[i][j].x = -gExplodeScatterPoints[i][j].x;
			gExplodeScatterPoints[i][j].y = (float)((float)rand()/(float)RAND_MAX);
			if ( rand() & 1 )
				gExplodeScatterPoints[i][j].y = -gExplodeScatterPoints[i][j].y;
			gExplodeScatterPoints[i][j].z = (float)((float)rand()/(float)RAND_MAX) * 0.3f + 0.3f;

		}
	}



	// set up the "glowing ball" verts
	float rad, radstep, radius;
	rad = 0.0f;
	radstep = 2.0f * PI / (float)numGlowCircleVerts;
	for ( i = 0; i < numGlowCircleVerts; i++ )
	{
		glowCircleVerts[i].x = 0.0f;
		glowCircleVerts[i].y = (float)sin(rad);
		glowCircleVerts[i].z = -(float)cos(rad);
		rad += radstep;
	}

	// set up the "glowing square" verts
	rad = 0.0f;
	radstep = 2.0f * PI / (float)numGlowSquareVerts;
	for ( i = 0; i < numGlowSquareVerts; i++ )
	{
		glowSquareVerts[i].x = 0.0f;
		glowSquareVerts[i].y = (float)sin(rad);
		glowSquareVerts[i].z = -(float)cos(rad);
		rad += radstep;
	}

	rad = 0.0f;
	radstep = 2.0f * PI / (float)numLensFlareVerts;
	for ( i = 0; i < numLensFlareVerts; i++ )
	{
		lensFlareVerts[i].x = 0.0f;
		lensFlareVerts[i].y = (float)sin(rad);
		lensFlareVerts[i].z = -(float)cos(rad);
		rad += radstep;
	}

	// create star by scaling every other circle vert
	for ( i = 0; i < numGlowCircleVerts; i++ )
	{
		if ( i & 1 )
			radius = 0.3f;
		else
			radius = 3.0f;
		glowStarVerts[i].x = 0.0f;
		glowStarVerts[i].y = glowCircleVerts[i].y * radius;
		glowStarVerts[i].z = glowCircleVerts[i].z * radius;
	}


	// Load our normal textures
	gAplTextures[0].LoadAndCreate( "sfx01.APL", MPR_TI_CHROMAKEY | MPR_TI_PALETTE );
	Palette *aplPal0 = gAplTextures[0].GetPalette();
	for ( j = 0; j < 256; j++ )
	{
		intalp = (aplPal0->paletteData[j] >> 24);
		aplPal0->paletteData[j] &= 0x00ffffff;
		if ( j == 0 ){
			continue;
		}
		alp = (float)intalp;
		alp = alp * 0.5f + alp * 0.5f * PRANDFloat();
		intalp = FloatToInt32( alp );
		aplPal0->paletteData[j] |= ( intalp << 24 );
	}
	aplPal0->UpdateMPR( aplPal0->paletteData );

	for (i=1; i<NUM_APL_TEXTURES; i++)
	{
		sprintf( texfile, "sfx%02d.APL", i+1 );

		// temp comment
		// gAplTextures[i].palette = gAplTextures[0].palette;
		gAplTextures[i].LoadAndCreate( texfile, MPR_TI_CHROMAKEY | MPR_TI_PALETTE );
		Palette *aplPalI = gAplTextures[i].GetPalette();
		for ( j = 0; j < 256; j++ )
		{
			intalp = (aplPalI->paletteData[j] >> 24);
			aplPalI->paletteData[j] &= 0x00ffffff;
			if ( j == 0 )
				continue;
			alp = (float)intalp;
			if ( i < 6 )
			{
				alp = alp * 0.6f + alp * 0.4f * PRANDFloat();
			}
			else if ( alp > 50.0f )
				alp = alp * 0.8f + alp * 0.2f * PRANDFloat();
			intalp = FloatToInt32( alp );
			aplPalI->paletteData[j] |= ( intalp << 24 );
		}
		aplPalI->UpdateMPR( aplPalI->paletteData );

	}

	// Load our green textures
	gAplTexturesGreen[0].LoadAndCreate( "sfxg01.GIF", MPR_TI_CHROMAKEY | MPR_TI_PALETTE );

	for (i=1; i<NUM_APL_TEXTURES; i++)
	{
		sprintf( texfile, "sfxg%02d.GIF", i+1 );
		gAplTexturesGreen[i].SetPalette(gAplTexturesGreen[0].GetPalette());
		gAplTexturesGreen[i].LoadAndCreate( texfile, MPR_TI_CHROMAKEY | MPR_TI_PALETTE );
	}

	// Initialize the lighting conditions and register for future time of day updates
	TimeUpdateCallback( NULL );
	TheTimeManager.RegisterTimeUpdateCB( TimeUpdateCallback, NULL );
}



void Drawable2D::ReleaseTexturesOnDevice(DXContext *rc)
{
	int i;

	// Stop receiving time updates
	TheTimeManager.ReleaseTimeUpdateCB( TimeUpdateCallback, NULL );


	// release sheets
	for ( i = 0; i < NUM_TEX_SHEETS; i++ )
	{
		if ( i >= 0 && i < NUM_TEXTURES_USED || i >= 15 && i < 15 + NUM_TEXTURES_USED )
		{
			gGlobTextures[i].FreeAll();
		}
	}

	// release apl textures
	for ( i = 0; i < NUM_APL_TEXTURES; i++ )
	{
		gAplTextures[i].FreeAll();
		gAplTexturesGreen[i].FreeAll();
	}
}


//JAM
void Drawable2D::TimeUpdateCallback( void *)
{
	Tcolor light;

	// Get the light level from the time of day manager
	lightLevel = TheTimeOfDay.GetLightLevel();

	TheTimeOfDay.GetTextureLightingColor( &gLight );

	// Get the light level from the time of day manager
	TheTimeOfDay.GetTextureLightingColor( &light );

	// right now we're assuming all APL have the same palette
	gAplTextures[6].GetPalette()->LightTexturePalette( &light );
	gAplTextures[7].GetPalette()->LightTexturePalette( &light );
	gAplTextures[8].GetPalette()->LightTexturePalette( &light );
	gAplTextures[9].GetPalette()->LightTexturePalette( &light );
	gAplTextures[10].GetPalette()->LightTexturePalette( &light );
	gAplTexturesGreen[0].GetPalette()->LightTexturePalette( &light );

	// now light the palette used for all our textures
	// TODO: the range is incorrect at the moment -- until the artists
	// redo the palette
	gGlobTextures[0].GetPalette()->LightTexturePaletteRange( &light, 151, 255 );


}

/*
** Name: DrawGlowSphere
** Description:
**		Draws an alpha faded spherical colored object
*/
void Drawable2D::DrawGlowSphere( class RenderOTW *renderer, int)
{
	Tpoint				center, os;
	ThreeDVertex		v0, v1, v2, vLast;
	int					i;
	DWORD				now;

	if ( alpha <= 0.05f )
		return;

	// Get the curent time
	now = TheTimeManager.GetClockTime();

	if ( startFade == FALSE )
	{
		startFade = TRUE;
		expandStartTime = now;
		alphaStartTime = now;
	}

	// do expansion
	realRadius = initRadius + ((float)( now - expandStartTime )) * typeData.expandRate * 0.001f;
	if ( realRadius > typeData.maxExpand )
		realRadius = typeData.maxExpand;
	radius = realRadius * 1.0f;
	if ( realRadius <= 0.0f )
		return;

	if ( typeData.flags & ALPHA_DAYLIGHT )
	{
	
		alpha = initAlpha - ((float)( now - alphaStartTime )) * typeData.fadeRate;
		if ( alpha <= 0.05f )
			return;
	
	}
	else
	{
//		if ( !renderer->GetAlphaMode() )
//			return;

		alpha = initAlpha * ( 1.0f - lightLevel * 0.5f ) - ((float)( now - alphaStartTime )) * typeData.fadeRate;
		if ( alpha <= 0.05f )
			return;
	}

	// set up the verts rgb and alpha's

	if ( sGreenMode )
	{
		v0.r = 0.0f;
		v0.g = 1.0f;
		v0.b = 0.0f;
		v0.a = alpha;
	
		v1.r = 0.0f;
		v1.g = 1.0f;
		v1.b = 0.0f;
		v1.a = 0.0f;
	
		v2.r = 0.0f;
		v2.g = 1.0f;
		v2.b = 0.0f;
		v2.a = 0.0f;
	}
	else
	{
		v0.a = alpha;
		if ( type == DRAW2D_SPARKS || type == DRAW2D_EXPLSTAR_GLOW )
		{
			v0.r = 1.0f;
			v0.g = 1.0f;
			v0.b = 1.0f;
			v0.a -= alpha * NRANDPOS * 0.9f;

		}
		else if ( (typeData.flags & GROUND_GLOW ) && type != DRAW2D_MISSILE_GROUND_GLOW )
		{
			v0.r = 1.0f;
			v0.g = 0.3f + NRANDPOS * 0.5f;
			v0.b = 0.1f + NRANDPOS * 0.5f;
		}
		else
		{
			v0.r = 1.0f;
			v0.g = 1.0f;
			v0.b = 1.0f;
		}
	

		if ( type == DRAW2D_SPARKS || type == DRAW2D_EXPLSTAR_GLOW )
		{
			v1.r = 1.0f;
			v1.g = 1.0f;
			v1.b = 0.2f + NRANDPOS * 0.2f;
			v1.a = 0.0f;
	
			v2.r = 1.0f;
			v2.g = 1.0f;
			v2.b = 0.2f + NRANDPOS * 0.2f;
			v2.a = 0.0f;
		}
		else if ( typeData.flags & GROUND_GLOW )
		{
			v1.r = 0.1f + NRANDPOS * 0.8f;
			v1.b = 0.0f;
			v1.g = 0.1f + NRANDPOS * 0.8f;
			v1.a = 0.0f;
	
			v2.r = 0.1f + NRANDPOS * 0.8f;
			v2.b = 0.0f;
			v2.g = 0.1f + NRANDPOS * 0.8f;
			v2.a = 0.0f;
		}
		else if ( typeData.flags & ALPHA_BRIGHTEN )
		{
			v1.r = 0.1f + NRANDPOS * 0.8f;
			v1.g = 0.0f;
			v1.b = 0.1f + NRANDPOS * 0.8f;
			v1.a = 0.0f;
	
			v2.r = 0.1f + NRANDPOS * 0.8f;
			v2.g = 0.0f;
			v2.b = 0.1f + NRANDPOS * 0.8f;
			v2.a = 0.0f;
		}
		else
		{
			v1.r = 0.9f;
			v1.g = 0.9f;
			v1.b = 0.4f;
			v1.a = 0.0f;
	
			v2.r = 0.9f;
			v2.g = 0.9f;
			v2.b = 0.4f;
			v2.a = 0.0f;
		}

	}

	// Set up our drawing mode
	renderer->context.RestoreState(STATE_ALPHA_GOURAUD);
	renderer->context.SetState(MPR_STA_ALPHA_OP_FUNCTION,MPR_TO_MODULATE); //JAM 18Oct03

	if ( !(typeData.flags & GROUND_GLOW) )
	{
		// transform the center point
		renderer->TransformPointToView( &position,  &center );
		renderer->TransformPoint( &position,  &v0 );
	
		// do 1st point
		if ( type == DRAW2D_SPARKS )
		{
			float randradius;
	
			randradius = realRadius + realRadius * PRANDFloat() * 0.8f;
	
			os.x = typeData.glowVerts[0].x * scale2d * randradius;
			os.y = typeData.glowVerts[0].y * scale2d * randradius;
			os.z = typeData.glowVerts[0].z * scale2d * randradius;
		}
		else if ( typeData.flags & GLOW_RAND_POINTS )
		{
			float randradius;
	
			randradius = realRadius + realRadius * PRANDFloat();
	
			os.x = typeData.glowVerts[0].x * scale2d * randradius;
			os.y = typeData.glowVerts[0].y * scale2d * randradius;
			os.z = typeData.glowVerts[0].z * scale2d * randradius;
		}
		else
		{
			os.x = typeData.glowVerts[0].x * scale2d * realRadius;
			os.y = typeData.glowVerts[0].y * scale2d * realRadius;
			os.z = typeData.glowVerts[0].z * scale2d * realRadius;
		}
		renderer->TransformBillboardPoint( &os,  &center, &v1 );
	
		// save it for last tri in strip
		vLast = v1;
	
		for ( i = 1; i < typeData.numGlowVerts; i++ )
		{
			// get 3rd point of triangle
			if ( !(i & 1) && (type == DRAW2D_SPARKS) )
			{
				float randradius;
	
				randradius = realRadius + realRadius * PRANDFloat() * 0.8f;
		
				os.x = typeData.glowVerts[i].x * scale2d * randradius;
				os.y = typeData.glowVerts[i].y * scale2d * randradius;
				os.z = typeData.glowVerts[i].z * scale2d * randradius;
			}
			else if ( !(i & 1) && (typeData.flags & GLOW_RAND_POINTS) )
			{
				float randradius;
	
				randradius = realRadius + realRadius * PRANDFloat();
		
				os.x = typeData.glowVerts[i].x * scale2d * randradius;
				os.y = typeData.glowVerts[i].y * scale2d * randradius;
				os.z = typeData.glowVerts[i].z * scale2d * randradius;
			}
			else
			{
				os.x = typeData.glowVerts[i].x * scale2d * realRadius;
				os.y = typeData.glowVerts[i].y * scale2d * realRadius;
				os.z = typeData.glowVerts[i].z * scale2d * realRadius;
			}
			renderer->TransformBillboardPoint( &os,  &center, &v2 );
	
			renderer->DrawTriangle( &v0, &v1, &v2, CULL_ALLOW_ALL );
	
			// move v2 to v1 for next time thru loop
			v1 = v2;
		}
	}
	// ground flash
	// oriented flat to the ground plane
	else
	{
		// transform the center point
		renderer->TransformPoint( &position,  &v0 );
	
		// do 1st point
		if ( typeData.flags & GLOW_RAND_POINTS )
		{
			float randradius;
	
			randradius = realRadius + realRadius * PRANDFloat() * 0.3f;
	
			os.x = position.x + typeData.glowVerts[0].z * scale2d * randradius;
			os.y = position.y + typeData.glowVerts[0].y * scale2d * randradius;
			os.z = position.z;
		}
		else
		{
			os.x = position.x + typeData.glowVerts[0].z * scale2d * realRadius;
			os.y = position.y + typeData.glowVerts[0].y * scale2d * realRadius;
			os.z = position.z;
		}
		renderer->TransformPoint( &os, &v1 );
	
		// save it for last tri in strip
		vLast = v1;
	
		for ( i = 1; i < typeData.numGlowVerts; i++ )
		{
			// get 3rd point of triangle
			if ( !(i & 1) && (typeData.flags & GLOW_RAND_POINTS) )
			{
				float randradius;
	
				randradius = realRadius + realRadius * PRANDFloat() * 0.3f;
		
				os.x = position.x + typeData.glowVerts[i].z * scale2d * randradius;
				os.y = position.y + typeData.glowVerts[i].y * scale2d * randradius;
				os.z = position.z;
			}
			else
			{
				os.x = position.x + typeData.glowVerts[i].z * scale2d * realRadius;
				os.y = position.y + typeData.glowVerts[i].y * scale2d * realRadius;
				os.z = position.z;
			}
			renderer->TransformPoint( &os, &v2 );
	
			renderer->DrawTriangle( &v0, &v1, &v2, CULL_ALLOW_ALL );
	
			// move v2 to v1 for next time thru loop
			v1 = v2;
		}
	}

	// kick out last triangle
	renderer->DrawTriangle( &v0, &v1, &vLast, CULL_ALLOW_ALL );
}


/*
** Name: DrawGouraudTri
** Description:
**		Draws a gouraud shaded triangle
*/
void Drawable2D::DrawGouraudTri( class RenderOTW *renderer, int)
{
	Tpoint				center, os;
	ThreeDVertex		v0, v1, v2, v3;
	DWORD				now;
	float randradius;

	if ( alpha <= 0.05f )
		return;

	// Get the curent time
	now = TheTimeManager.GetClockTime();

	if ( startFade == FALSE )
	{
		startFade = TRUE;
		expandStartTime = now;
		alphaStartTime = now;
	}

	// do fade... 10 seconds until nothing...
	alpha = initAlpha  - ((float)( now - alphaStartTime )) * typeData.fadeRate;
	if ( alpha <= 0.05f )
		return;

	// do expansion
	realRadius = initRadius + ((float)( now - expandStartTime )) * typeData.expandRate * 0.001f;
	if ( realRadius > typeData.maxExpand )
		realRadius = typeData.maxExpand;
	radius = realRadius * 8.0f;
	if ( realRadius <= 0.0f )
		return;

	// set up the verts rgb and alpha's

	if ( type == DRAW2D_DARK_DEBRIS )
	{
		randradius =  0.3f + PRANDFloat() * 0.3f;
		v0.r = randradius * lightLevel;
		v0.g = randradius * lightLevel;
		v0.b = randradius * lightLevel;
		v0.a = alpha;
	
		randradius =  0.2f + PRANDFloat() * 0.2f;
		v1.r = randradius * lightLevel;
		v1.g = randradius * lightLevel;
		v1.b = randradius * lightLevel;
		v1.a = alpha;
	
		randradius =  0.3f + PRANDFloat() * 0.2f;
		v2.r = randradius * lightLevel;
		v2.g = randradius * lightLevel;
		v2.b = randradius * lightLevel;
		v2.a = alpha;

		randradius =  0.2f + PRANDFloat() * 0.2f;
		v3.r = randradius * lightLevel;
		v3.g = randradius * lightLevel;
		v3.b = randradius * lightLevel;
		v3.a = alpha;
	}
	else if ( type == DRAW2D_FIRE_DEBRIS )
	{
		/*
		v0.r = 0.9f;
		v0.g = 0.2f;
		v0.b = 0.2f;
		v0.a = alpha;
	
		v1.r = 0.9f;
		v1.g = 0.9f;
		v1.b = 0.2f;
		v1.a = alpha;
	
		v2.r = 0.9f;
		v2.g = 0.2f;
		v2.b = 0.2f;
		v2.a = alpha;
		*/
		v0.r = 0.0f;
		v0.g = 0.0f;
		v0.b = 0.0f;
		v0.a = 0.2f + PRANDFloat() * 0.2f;
	
		v1.r = 0.1f * lightLevel;
		v1.g = 0.1f * lightLevel;
		v1.b = 0.1f * lightLevel;
		v1.a = 0.2f + PRANDFloat() * 0.2f;
	
		v2.r = 0.2f * lightLevel;
		v2.g = 0.2f * lightLevel;
		v2.b = 0.2f * lightLevel;
		v2.a = 0.2f + PRANDFloat() * 0.2f;

		v3.r = 0.0f;
		v3.g = 0.1f * lightLevel;
		v3.b = 0.2f * lightLevel;
		v3.a = 0.2f + PRANDFloat() * 0.2f;
	}
	else if ( type == DRAW2D_LIGHT_DEBRIS )
	{
		v0.r = 1.0f * lightLevel;
		v0.g = 1.0f * lightLevel;
		v0.b = 1.0f * lightLevel;
		v0.a = 0.2f + PRANDFloat() * 0.2f;
	
		v1.r = 0.9f * lightLevel;
		v1.g = 0.9f * lightLevel;
		v1.b = 0.9f * lightLevel;
		v1.a = 0.2f + PRANDFloat() * 0.2f;
	
		v2.r = 0.8f * lightLevel;
		v2.g = 0.8f * lightLevel;
		v2.b = 0.8f * lightLevel;
		v2.a = 0.2f + PRANDFloat() * 0.2f;

		v3.r = 0.9f * lightLevel;
		v3.g = 0.9f * lightLevel;
		v3.b = 0.9f * lightLevel;
		v3.a = 0.2f + PRANDFloat() * 0.2f;
	}
	else
	{
		return;
	}

	if ( sGreenMode )
	{
		v0.r = v1.r = v2.r = v3.r = 0.0f;
		v0.b = v1.b = v2.b = v3.b = 0.0f;
	}

	// Set up our drawing mode
	renderer->context.RestoreState(STATE_ALPHA_GOURAUD);
	renderer->context.SetState(MPR_STA_ALPHA_OP_FUNCTION,MPR_TO_MODULATE); //JAM 18Oct03

	// transform the center point
	renderer->TransformPointToView( &position,  &center );

	os.x = 0.0f;

	// do 1st point
	randradius = (realRadius + realRadius * PRANDFloat()) * 0.5f;

	os.z = scale2d * randradius;
	os.y = scale2d * randradius;
	renderer->TransformBillboardPoint( &os,  &center, &v0 );

	// do 2nd point

	randradius = (realRadius + realRadius * PRANDFloat()) * 0.5f;

	os.z = -scale2d * randradius;
	os.y = scale2d * randradius;
	renderer->TransformBillboardPoint( &os,  &center, &v1 );

	// do 3nd point
	randradius = (realRadius + realRadius * PRANDFloat()) * 0.5f;

	os.z = -scale2d * randradius;
	os.y = -scale2d * randradius;
	renderer->TransformBillboardPoint( &os,  &center, &v2 );

	// do 4th point
	randradius = (realRadius + realRadius * PRANDFloat()) * 0.5f;

	os.z = scale2d * randradius;
	os.y = -scale2d * randradius;
	renderer->TransformBillboardPoint( &os,  &center, &v3 );


	// kick out triangle
	renderer->DrawSquare( &v0, &v1, &v2, &v3, CULL_ALLOW_ALL );
}


#define DTR                     0.01745329F

/*
** Name: Draw2DLensFlare
** Description:
**		Gets the look at vector and lighting direction.  If the angle
**		between the 2 is within a predefined limit, it will draw a series
**		of alpha blended circles along the light vector.
*/
void Draw2DLensFlare( class RenderOTW *renderer )
{
	Tpoint				position, center, lv, av;
	int					i,j;
	float				dotp;
	float				radius;
	float				alphaPct;
	Tpoint				atPosition;
	bool gif = false;


	// If we're below the overcast layer, do not draw the lens flare.
	if( realWeather->InsideOvercast() || realWeather->UnderOvercast()) return;

	// is there a sun, if not return
	if( !TheTimeOfDay.ThereIsASun() ) return;

	// get the Look At vector and lighting vector
	renderer->GetAt( &av );
	TheTimeOfDay.GetLightDirection( &lv );

	// dot product between light and look at vector.  Both should be
	// unit vectors and as the dotp approaches -1.0f, we should be
	// approaching looking directly into the sun
	dotp = av.x * -lv.x + av.y * -lv.y + av.z * -lv.z;

	if ( dotp > -0.70f )
		return;

	alphaPct = (-dotp-0.70f)/0.30f;

	// Get the center point of the body on a unit sphere in world space
	TheTimeOfDay.CalculateSunMoonPos( &center, FALSE );
	radius = 40.0f + (alphaPct) * (20.0f - 40.0f);

	//JAM 04Oct03
	if(0)
		Draw2DSunGlowEffect(renderer,&center,radius,alphaPct*0.7f);

	if (g_nGfxFix & 0x04)
		gif = true;

	// now calculate where we want to start the first circle (world space)
	// we'll head out from the viewpoint a ways along the lookat vector,
	// and then from that point head along the light vector a ways
	atPosition.x = renderer->X() + av.x * 90.0f - lv.x * 40.0f;
	atPosition.y = renderer->Y() + av.y * 90.0f - lv.y * 40.0f;
	atPosition.z = renderer->Z() + av.z * 90.0f - lv.z * 40.0f;

	
	D3DDYNVERTEX	v[numLensFlareVerts+2], vLast, vLasti;

	v[0].dwSpecular=v[1].dwSpecular=v[2].dwSpecular=v[3].dwSpecular=0x00000000;
	for ( j = 0; j < NUM_FLARE_CIRCLES; j++ )
	{
		// now move the center point of the next circle
		position.x = atPosition.x + lv.x * lensDist[j] * ( 1.2f - alphaPct );
		position.y = atPosition.y + lv.y * lensDist[j] * ( 1.2f - alphaPct );
		position.z = atPosition.z + lv.z * lensDist[j] * ( 1.2f - alphaPct );

		float	Alpha=lensAlphas[j]*alphaPct*lightLevel * 0.80f;
		DWORD	ColorOut, ColorIn;
		ColorOut=v[2].dwColour=v[1].dwColour=F_TO_ARGB(Alpha, lensRGB[j].r, lensRGB[j].g, lensRGB[j].b);
		ColorIn=v[0].dwColour=v[3].dwColour=F_TO_ARGB(Alpha, lensCenterRGB[j].r, lensCenterRGB[j].g, lensCenterRGB[j].b);

		if(Alpha<0.001f) continue;

		v[0].dwColour&=0x00ffffff;

		radius = lensRadius[j];
		if ( renderer->GetFOV() < 60.0f * DTR )
			radius *= 0.3f;

		v[1].pos.x = lensFlareVerts[0].x * radius;
		v[1].pos.y = lensFlareVerts[0].y * radius;
		v[1].pos.z = lensFlareVerts[0].z * radius;



		if ( j == 2 || j == 8 || j == 9 )
		{
			v[0].pos.x = lensFlareVerts[0].x * radius * 0.8f;
			v[0].pos.y = lensFlareVerts[0].y * radius * 0.8f;
			v[0].pos.z = lensFlareVerts[0].z * radius * 0.8f;

			vLast = v[1];
			vLasti = v[0];

			for ( i = 1; i < numLensFlareVerts; i++ )
			{
				v[2].pos.x = lensFlareVerts[i].x * radius;
				v[2].pos.y = lensFlareVerts[i].y * radius;
				v[2].pos.z = lensFlareVerts[i].z * radius;

				v[3].pos.x = lensFlareVerts[i].x * radius * 0.8f;
				v[3].pos.y = lensFlareVerts[i].y * radius * 0.8f;
				v[3].pos.z = lensFlareVerts[i].z * radius * 0.8f;
		
				TheDXEngine.DX2D_AddQuad(LAYER_TOP, POLY_BB, (D3DXVECTOR3*)&position, v, radius, NULL);
		
				// move v2 to v1 for next time thru loop
				v[1] = v[2];
				v[0] = v[3];
			}
			v[2]=vLast; v[3]=vLasti;
			TheDXEngine.DX2D_AddQuad(LAYER_TOP, POLY_BB, (D3DXVECTOR3*)&position, v, radius, NULL);

		}
		else
		{
			// Setup the Center
			v[0].pos.x = 0;
			v[0].pos.y = 0;
			v[0].pos.z = 0;
			v[0].dwSpecular=0x00000000;
			v[0].dwColour=ColorOut & 0x00ffffff;

			// Now the circle vertices
			for ( i = 0; i < numLensFlareVerts; i++ )
			{
				v[i+1].pos.x = lensFlareVerts[i].x * radius;
				v[i+1].pos.y = lensFlareVerts[i].y * radius;
				v[i+1].pos.z = lensFlareVerts[i].z * radius;
				v[i+1].dwSpecular=0x00000000;
				v[i+1].dwColour=ColorOut;

			}

			// Close the Circle
			v[i+1].pos.x = lensFlareVerts[0].x * radius;
			v[i+1].pos.y = lensFlareVerts[0].y * radius;
			v[i+1].pos.z = lensFlareVerts[0].z * radius;
			v[i+1].dwSpecular=0x00000000;
			v[i+1].dwColour=ColorOut;

			// And draw the FAN
			TheDXEngine.DX2D_AddPoly(LAYER_TOP, POLY_BB | POLY_FAN, (D3DXVECTOR3*)&position, v, radius, numLensFlareVerts + 2, NULL);
		}


	} // end for number of circles
}

/*
** Name: Draw2DSunGlowEffect
** Description:
**		Gets the look at vector and lighting direction.  If the angle
**		between the 2 is within a predefined limit, it will draw a series
**		of alpha blended circles along the light vector.
*/
void Draw2DSunGlowEffect( class RenderOTW *renderer, Tpoint *cntr, float dist, float alpha  )
{
	Tpoint				center, os;
	ThreeDVertex		v0, v1, v2, v3, vLast, vLasti;
	int					i;
	float				radius, iradius;
	bool gif = false;
	
//	if ( !renderer->GetAlphaMode() )
//		return;

	if ( alpha < 0.0001 )
		return;

	center = *cntr;


	center.x *= dist * 0.30f;
	center.y *= dist * 0.30f;
	center.z *= dist * 0.30f;

	v0.r = 1.0f;
	v0.g = 1.0f;
	v0.b = 1.0f;
	v1.r = 1.0f;
	v1.g = 1.0f;
	v1.b = 1.0f;
	v2.r = 1.0f;
	v2.g = 1.0f;
	v2.b = 1.0f;

	/*
	v1.r = 0.4f + NRANDPOS * 0.6f;
	v1.g = 1.0f;
	v1.b = 0.4f + NRANDPOS * 0.6f;

	v2.r = 0.4f + NRANDPOS * 0.6f;
	v2.g = 1.0f;
	v2.b = 0.4f + NRANDPOS * 0.6f;
	*/

	v0.q = v1.q = v2.q = vLast.q = 0.0f;

	// Set up our drawing mode
	renderer->context.RestoreState(STATE_ALPHA_GOURAUD);
	renderer->context.SetState(MPR_STA_ALPHA_OP_FUNCTION,MPR_TO_MODULATE); //JAM 18Oct03

	// set alphas for this circle
	v0.a = v1.a = v2.a = vLast.a = 0.0f;

	// clear center?
	v0.a = alpha;

	// set radius for this circle
	radius = 1.8f;
	if ( renderer->GetFOV() < 60.0f * DTR )
		radius *= 0.3f;

	// do center point
	renderer->TransformCameraCentricPoint( &center, &v0 );
	center.y = v0.csX;
	center.z = v0.csY;
	center.x = v0.csZ;

	// 1 SHIMMERY CIRCLE

	// v1.r = 0.4f + NRANDPOS * 0.6f;
	// v1.g = 1.0f;
	// v1.b = 0.4f + NRANDPOS * 0.6f;

	// do 1st point
	os.x = 0.0f;

	os.y = lensFlareVerts[0].y * radius;
	os.z = lensFlareVerts[0].z * radius;

	renderer->TransformBillboardPoint( &os, &center, &v1 );

	// save it for last tri in strip
	vLast = v1;

	if (g_nGfxFix & 0x04) // sun, sun glow and moon fix
		gif = true;

	for ( i = 1; i < numLensFlareVerts; i++ )
	{
		os.y = lensFlareVerts[i].y * radius;
		os.z = lensFlareVerts[i].z * radius;
	
		// v2.r = 0.4f + NRANDPOS * 0.6f;
		// v2.g = 1.0f;
		// v2.b = 0.4f + NRANDPOS * 0.6f;

		renderer->TransformBillboardPoint( &os, &center, &v2 );

		renderer->DrawTriangle( &v0, &v1, &v2, CULL_ALLOW_ALL, gif );

		// move v2 to v1 for next time thru loop
		v1 = v2;
	}




	// kick out last triangle
	renderer->DrawTriangle( &v0, &v1, &vLast, CULL_ALLOW_ALL, gif );

#if 0

	// 2 SHIMMERY STAR

	// do 1st point
	float randRadius;
	randRadius = radius + PRANDFloat() * radius * 1.7f;
	os.y = lensFlareVerts[0].y * randRadius;
	os.z = lensFlareVerts[0].z * randRadius;

	renderer->TransformBillboardPoint( &os, &center, &v1 );

	v1.r = 1.0f;
	v1.g = 1.0f;
	v1.b = 1.0f;

	// save it for last tri in strip
	vLast = v1;
	v0.a *= 0.8f;

	for ( i = 1; i < numLensFlareVerts; i++ )
	{
		if ( i & 1 )
		{
			os.y = lensFlareVerts[i].y * radius * 0.1f;
			os.z = lensFlareVerts[i].z * radius * 0.1f;
		}
		else
		{
			randRadius = radius + PRANDFloat() * radius * 1.7f;
			os.y = lensFlareVerts[i].y * randRadius;
			os.z = lensFlareVerts[i].z * randRadius;
		}
	
		renderer->TransformBillboardPoint( &os, &center, &v2 );

		renderer->DrawTriangle( &v0, &v1, &v2, CULL_ALLOW_ALL );

		// move v2 to v1 for next time thru loop
		v1 = v2;
	}
	// kick out last triangle
	renderer->DrawTriangle( &v0, &v1, &vLast, CULL_ALLOW_ALL );

#endif


	// 3 RING AROUND SUN

	v1.r = 0.9f;
	v1.g = 0.6f;
	v1.b = 0.6f;

	v2.r = 0.9f;
	v2.g = 0.6f;
	v2.b = 0.6f;

	v0.r = 1.0f;
	v0.g = 1.0f;
	v0.b = 1.0f;

	v3.r = 1.0f;
	v3.g = 1.0f;
	v3.b = 1.0f;


	v0.q = v1.q = v2.q = v3.q = 0.0f;

	// Set up our drawing mode
	renderer->context.RestoreState(STATE_ALPHA_GOURAUD);
	renderer->context.SetState(MPR_STA_ALPHA_OP_FUNCTION,MPR_TO_MODULATE); //JAM 18Oct03

	// set alphas for this circle
	v0.a = v3.a = 0.0f;

	// clear center?
	v1.a = v2.a = alpha * 0.55f;

	// set radius for this circle
	radius = 0.5f;
	iradius = 0.3f;
	if ( renderer->GetFOV() < 60.0f * DTR )
	{
		radius *= 0.3f;
		iradius *= 0.3f;
	}

	// do 1st 2 points
	os.y = lensFlareVerts[0].y * radius;
	os.z = lensFlareVerts[0].z * radius;

	renderer->TransformBillboardPoint( &os, &center, &v1 );

	os.y = lensFlareVerts[0].y * iradius;
	os.z = lensFlareVerts[0].z * iradius;

	renderer->TransformBillboardPoint( &os, &center, &v0 );


	// save it for last tri in strip
	vLast = v1;
	vLasti = v0;

	for ( i = 1; i < numLensFlareVerts; i++ )
	{
		os.y = lensFlareVerts[i].y * radius;
		os.z = lensFlareVerts[i].z * radius;
	
		renderer->TransformBillboardPoint( &os, &center, &v2 );

		os.y = lensFlareVerts[i].y * iradius;
		os.z = lensFlareVerts[i].z * iradius;
	
		renderer->TransformBillboardPoint( &os, &center, &v3 );

		renderer->DrawTriangle( &v0, &v1, &v2, CULL_ALLOW_ALL, gif );
		renderer->DrawTriangle( &v0, &v2, &v3, CULL_ALLOW_ALL, gif );

		// save current points to past points
		v1 = v2;
		v0 = v3;
	}

	// kick out last 2 triangle
	renderer->DrawTriangle( &v0, &v1, &vLast, CULL_ALLOW_ALL, gif );
	renderer->DrawTriangle( &v0, &vLast, &vLasti, CULL_ALLOW_ALL, gif );
}



/*
** Name: GetAnimFrame
** Description:
**		Gets the current animation frame based on time delta from
**		start of object existence
*/
int Drawable2D::GetAnimFrame( int dT, DWORD start )
{
	int ms;
	int num;
	int rem;
	int newFrame;

	if ( dT <= 0 )
		return 0;

	if ( typeData.flags & ANIM_HALF_RATE )
	{
		ms = 104;
	}
	else
	{
		ms = 62;
	}
	if ( typeData.flags & (FIRE_SCATTER_PLOT | SMOKE_SCATTER_PLOT | EXPLODE_SCATTER_PLOT) )
	{
		// ms = 124;
		curSFrame = dT/164 + startSFrame;
		if ( curSFrame >= typeData.numGlowVerts )
		{
			startSFrame = 0;
			curSFrame = curSFrame % typeData.numGlowVerts;
		}

		// hack for fireball
		if ( type >= DRAW2D_FIRE1 && type <= DRAW2D_FIRE6 )
		{
			if ( curBFrame >= 6 )
			{
				return 0;
			}
			else
			{
				switch (type )
				{
					case DRAW2D_FIRE4:
						curBFrame = dT/100;
						break;
					case DRAW2D_FIRE5:
						curBFrame = dT/50;
						break;
					default:
						curBFrame = dT/400;
						break;
				}
				if ( curBFrame >= 6 )
				{
					if ( typeData.flags & FADE_LAST )
					{
						alphaStartTime = start + dT;
						startFade = TRUE;
					}

					if ( type == DRAW2D_FIRE3 )
						curBFrame = 7;
					else if ( type == DRAW2D_FIRE5 )
					{
						if ( rand() & 1 )
							curBFrame = 8;
						else
							curBFrame = 6;
					}
					else if ( type == DRAW2D_FIRE4 || type == DRAW2D_FIRE1 )
					{
						if ( rand() & 1 )
							curBFrame = 10;
						else
							curBFrame = 6;
					}
					else
						curBFrame = 6;

					return 0;
				}
			}
		}

	}

	newFrame = dT/ms;
	if ( newFrame >= typeData.numTextures )
	{
		if ( typeData.flags & ANIM_LOOP_PING )
		{
			num = newFrame / typeData.numTextures;
			rem = newFrame % typeData.numTextures;
			// if num is odd, we're going backwards
			if ( num & 1 )
			{
				newFrame = typeData.numTextures - 1 - rem;
			}
			else
			{
				newFrame = rem;
			}
		}
		else if ( typeData.flags & ANIM_LOOP )
		{
			newFrame = newFrame % typeData.numTextures;
		}
		else if ( typeData.flags & ANIM_NO_CLAMP )
		{
			// do nothing
		}
		else // non looping
		{
			newFrame = typeData.numTextures;
			if ( typeData.flags & FADE_LAST )
			{
				alphaStartTime = start + typeData.numTextures * ms;
				startFade = TRUE;
			}
		}
	}

	return newFrame;
}

/*
** Specifically set a start time and other associated variables
*/
void Drawable2D::SetStartTime ( DWORD start, DWORD now )
{
	explicitStartTime = TRUE;
	startTime = start;
	firstFrame = GetAnimFrame( (int)(now - start), startTime );
};

/*
*/
void Drawable2D::APLScatterPlot ( RenderOTW *renderer )
{
	Tpoint				ws;
	ThreeDVertex		v0, v1, v2, v3, spos;
	ThreeDVertex		vm;
	TwoDVertex			*vertArray[6];
	Tpoint				leftv;
	Tpoint				dl;
	float				screenR, elementR, elementRbase;
	float 				xRes;
	float 				yRes;
	float				top, bottom, left, right;
	int					i;
	float				scaleZ;
	int					numToPlot;
	BOOL				doFivePoints;
	int					texseq;

	// make our radius larger -- this way building fires will sort
	// better on buildings
	radius = realRadius * 3.0f;

	// figure out our max pixel locations
	top = renderer->GetTopPixel();
	bottom = renderer->GetBottomPixel();
	left = renderer->GetLeftPixel();
	right = renderer->GetRightPixel();
	xRes = right - left;
	yRes = bottom - top;

	// get the left vector for the camera
	renderer->GetLeft( &leftv );

	// find screen radius we'll be scattering into
	dl.x = -leftv.x * scale2d * realRadius;
	dl.y = -leftv.y * scale2d * realRadius;
	dl.z = -leftv.z * scale2d * realRadius;
	ws.x = position.x + dl.x ;
	ws.y = position.y + dl.y ;
	ws.z = position.z + dl.z ;
	renderer->TransformPoint( &position,  &spos );
	if ( spos.csZ < 1.0f )
		return;

	// hack to make long hanging smoke look better
	if ( type == DRAW2D_LONG_HANGING_SMOKE2 || type == DRAW2D_FIRE1 )
	{
		scaleZ = 1.0f;
	}
	else
	{
		scaleZ = (SCATTER_ZMAX - spos.csZ)/SCATTER_ZMAX;
		if ( scaleZ < 0.0f ) scaleZ = 0.0f;
		scaleZ *= scaleZ;
	}

	renderer->TransformPoint( &ws,  &v0 );
	screenR = (float)fabs( v0.x - spos.x );
	/*
	if (screenR > sMaxScreenRes * xRes * 0.5f )
	{
		screenR = xRes * sMaxScreenRes * 0.5f;
	}
	*/
	elementRbase = screenR * 0.4f + ( 1.0f - scaleZ ) * 0.6f;
	screenR *= 0.4f - ( 1.0f - scaleZ ) * 0.3f;

	// setup the 4 verts, the only thing we'll be modifiying in the
	// scatter loop is the x,y screen coords
	v0.csZ = spos.csZ;
	v0.clipFlag = 0;
	v0.q = v0.csZ * 0.001f;
	v1.csZ = spos.csZ;
	v1.clipFlag = 0;
	v1.q = v0.q;
	v2.csZ = spos.csZ;
	v2.clipFlag = 0;
	v2.q = v0.q;
	v3.csZ = spos.csZ;
	v3.clipFlag = 0;
	v3.q = v0.q;

	// a nicer effect: put a point in the center of a square, make it
	// a dark alpha and have the edges fade to 0.  LOD this effect.
	// doFivePoints = !(typeData.flags & NO_FIVE_POINTS ) && ( scaleZ * sLOD > 0.4f || type == DRAW2D_LONG_HANGING_SMOKE2 );
	doFivePoints = !(typeData.flags & NO_FIVE_POINTS );

	// setup rendering context
	if(sGreenMode) //JAM - FIXME
			renderer->context.RestoreState(STATE_ALPHA_TEXTURE);
	else
	{
//		if(renderer->GetAlphaMode()) //JAM - FIXME
			renderer->context.RestoreState(STATE_ALPHA_TEXTURE_GOURAUD);
/*		else
		{
			renderer->context.RestoreState(STATE_ALPHA_TEXTURE);
			doFivePoints = FALSE;
		}
*/
		if (renderer->GetFilteringMode())
		{
			renderer->context.SetState(MPR_STA_TEX_FILTER, MPR_TX_BILINEAR);
//			renderer->context.InvalidateState();
		}
	}

	renderer->context.SetState(MPR_STA_ALPHA_OP_FUNCTION,MPR_TO_MODULATE); //JAM 18Oct03

	if ( type == DRAW2D_LONG_HANGING_SMOKE2 || type == DRAW2D_FIRE1 )
		// numToPlot = (NUM_EXPLODE_SCATTER_POINTS);
		numToPlot = 1;
	else if ( typeData.flags & FIRE_SCATTER_PLOT )
		numToPlot = max( 1, (int)(NUM_FIRE_SCATTER_POINTS * scaleZ * sLOD));
	else if ( typeData.flags & SMOKE_SCATTER_PLOT )
		numToPlot = max( 1, (int)(NUM_SMOKE_SCATTER_POINTS * scaleZ * sLOD));
	else
		numToPlot = max( 1, (int)(NUM_EXPLODE_SCATTER_POINTS * scaleZ * sLOD));

	// color and alpha
	if ( doFivePoints )
	{
		vm.a = alpha;
		vm.r = 1.0f;
		vm.g = 1.0f;
		vm.b = 1.0f;
		vm.csZ = spos.csZ;
		vm.clipFlag = 0;
		vm.q = v0.q;
		v0.a = v1.a = 0.0f;
		v2.a = v3.a = 0.0f;
		v0.g = v2.g = v3.g = v1.g = 0.0f;
		// randomly tweak the edge RGB's to give a glowy kind of effect
		if ( typeData.flags & NO_RANDOM_BLEND )
		{
			v0.r = v2.r = v3.r = v1.r = 0.0f;
			v0.b = v2.b = v3.b = v1.b = 0.0f;
		}
		else
		{
			v0.r = v2.r = v3.r = v1.r = 0.1f + NRANDPOS * 0.8f;
			v0.b = v2.b = v3.b = v1.b = 0.1f + NRANDPOS * 0.8f;
		}
	}
	/*
	else if ( typeData.flags & EXPLODE_SCATTER_PLOT )
	{
		v0.r = v1.r = 1.0f;
		v0.g = v1.g = 1.0f;
		v0.b = v1.b = 1.0f;
		v0.a = v1.a = alpha * (1.0f - scaleZ);
		v2.r = v3.r = 1.0f;
		v2.g = v3.g = 1.0f;
		v2.b = v3.b = 1.0f;
		v2.a = v3.a = alpha;
	}
	*/
	else
	{
		v0.r = v1.r = 1.0f;
		v0.g = v1.g = 1.0f;
		v0.b = v1.b = 1.0f;
		// v0.a = v1.a = alpha * (1.0f - scaleZ);
		v0.a = v1.a = alpha;
		v2.r = v3.r = 1.0f;
		v2.g = v3.g = 1.0f;
		v2.b = v3.b = 1.0f;
		v2.a = v3.a = alpha;
	}


	// now run the scatter loop
	for ( i = 0; i < numToPlot; i++ )
	{

		// center of this element
		if ( typeData.flags & FIRE_SCATTER_PLOT )
		{
			dl.x = gFireScatterPoints[ curSFrame ][ i ].x * screenR + spos.x;
			if (gFireScatterPoints[ curSFrame ][ i ].y > 0.0f )
				dl.y = gFireScatterPoints[ curSFrame ][ i ].y * screenR + spos.y;
			else
				dl.y = gFireScatterPoints[ curSFrame ][ i ].y * screenR * 1.5f + spos.y;
			elementR = gFireScatterPoints[ curSFrame ][ i ].z * elementRbase +
						elementRbase * 0.25f;
		}
		else if ( typeData.flags & EXPLODE_SCATTER_PLOT )
		{
			dl.x = gExplodeScatterPoints[ curSFrame ][ i ].x * screenR + spos.x;
			if (gExplodeScatterPoints[ curSFrame ][ i ].y > 0.0f )
				dl.y = gExplodeScatterPoints[ curSFrame ][ i ].y * screenR + spos.y;
			else
				dl.y = gExplodeScatterPoints[ curSFrame ][ i ].y * screenR * 1.5f + spos.y;
			elementR = gExplodeScatterPoints[ curSFrame ][ i ].z * elementRbase +
						elementRbase * 0.25f;
		}
		else
		{
			dl.x = gSmokeScatterPoints[ curSFrame ][ i ].x * screenR + spos.x;
			if (gSmokeScatterPoints[ curSFrame ][ i ].y > 0.0f )
				dl.y = gSmokeScatterPoints[ curSFrame ][ i ].y * screenR + spos.y;
			else
				dl.y = gSmokeScatterPoints[ curSFrame ][ i ].y * screenR * 1.5f + spos.y;
			elementR = gSmokeScatterPoints[ curSFrame ][ i ].z * elementRbase +
						elementRbase * 0.25f;
		}

		// we do our own clipping checks -- for now, any point not within
		// screen bounds, we toss the entire element
		
		//if ( dl.x < left || dl.y < top || dl.x > right || dl.y > bottom )
		//	continue;

		v0.x = dl.x - elementR;
		v0.y = dl.y - elementR;
		//if ( v0.x < left || v0.y < top || v0.x > right || v0.y > bottom )
		//	continue;

		v1.x = dl.x + elementR;
		v1.y = dl.y - elementR;
		//if ( v1.x < left || v1.y < top || v1.x > right || v1.y > bottom )
		//	continue;

		v2.x = dl.x + elementR;
		v2.y = dl.y + elementR;
		//if ( v2.x < left || v2.y < top || v2.x > right || v2.y > bottom )
		//	continue;

		v3.x = dl.x - elementR;
		v3.y = dl.y + elementR;
		//if ( v3.x < left || v3.y < top || v3.x > right || v3.y > bottom )
		//	continue;

		if ( v2.x < left || v2.y < top || v0.x > right || v0.y > bottom )
			continue;
		
		renderer->SetClipFlags( &v0 );
		renderer->SetClipFlags( &v1 );
		renderer->SetClipFlags( &v2 );
		renderer->SetClipFlags( &v3 );



		// get and set texture
		if ( typeData.flags & SEQ_SCATTER_ANIM )
		{
			// hack for fireball
			if ( type >= DRAW2D_FIRE1 && type <= DRAW2D_FIRE6 )
			{
				if ( curBFrame >= 6 )
				{
					texseq = curBFrame;
					numToPlot = 1;
				}
				else
					texseq = curBFrame + typeData.texId + (curFrame+i) % typeData.numTextures;
			}
			else
				texseq = typeData.texId + (curFrame+i) % typeData.numTextures;
		}
		else
		{
			texseq = typeData.texId + curFrame;
		}

		// hack, only randomize alpha if fire texture
		if ( doFivePoints && texseq >= 0 && texseq <= 5 )
		{
			vm.a = alpha * 0.1f + alpha * 0.9f * NRANDPOS;
			if ( vm.a < 0.02f )
				continue;
		}


		v1.u =0.0f;
		v1.v =0.0f;
		v0.u = v1.u + 1.0f;
		v0.v = v1.v;
		v3.u = v1.u + 1.0f;
		v3.v = v1.v + 1.0f;
		v2.u = v1.u;
		v2.v = v1.v + 1.0f;
		vm.u = v1.u + 0.5f;
		vm.v = v1.v + 0.5f;

		if ( sGreenMode == 0)
			curTex = &gAplTextures[ texseq ];
		else
			curTex = &gAplTexturesGreen[ texseq ];
		renderer->context.SelectTexture1( curTex->TexHandle() );

		// do a middle point....
		if ( doFivePoints )
		{
			vm.x = dl.x;
			vm.y = dl.y;
			renderer->SetClipFlags( &vm );
			vertArray[0] = &vm;
			vertArray[1] = &v0;
			vertArray[2] = &v1;
			vertArray[3] = &v2;
			vertArray[4] = &v3;
			vertArray[5] = &v0;
			if (g_nGfxFix & 0x01)
				renderer->ClipAndDraw2DFan( vertArray, 6, true); // true = gifPicture -> fix for clipping explosion graphics
			else
				renderer->ClipAndDraw2DFan( vertArray, 6); 

			/*
			renderer->DrawTriangle( &v0, &v1, &vm, CULL_ALLOW_ALL );
			renderer->DrawTriangle( &v1, &v2, &vm, CULL_ALLOW_ALL );
			renderer->DrawTriangle( &v2, &v3, &vm, CULL_ALLOW_ALL );
			renderer->DrawTriangle( &v3, &v0, &vm, CULL_ALLOW_ALL );
			*/
		}
		else
		{
			vertArray[0] = &v0;
			vertArray[1] = &v1;
			vertArray[2] = &v2;
			vertArray[3] = &v3;
			if (g_nGfxFix & 0x01)
				renderer->ClipAndDraw2DFan( vertArray, 4, true );
			else
				renderer->ClipAndDraw2DFan( vertArray, 4);
			// renderer->DrawSquare( &v0, &v1, &v2, &v3, CULL_ALLOW_ALL );
		}

	}

};

/*
*/
void Drawable2D::ScatterPlot ( RenderOTW *renderer )
{
	Tpoint				ws;
	ThreeDVertex		v0, v1, v2, v3, spos;
	ThreeDVertex		vm;
	Tpoint				leftv;
	Tpoint				dl;
	float				screenR, elementR, elementRbase;
	// float 				xRes = renderer->GetImageBuffer()->targetXres();
	// float 				yRes = renderer->GetImageBuffer()->targetYres();
	float 				xRes;
	float 				yRes;
	float				top, bottom, left, right;
	int					i;
	float				scaleZ;
	int					numToPlot;
	BOOL				doFivePoints;

	// better on buildings
	radius = realRadius * 3.0f;

	// figure out our max pixel locations
	top = renderer->GetTopPixel();
	bottom = renderer->GetBottomPixel();
	left = renderer->GetLeftPixel();
	right = renderer->GetRightPixel();
	xRes = right - left;
	yRes = bottom - top;

	// get the left vector for the camera
	renderer->GetLeft( &leftv );

	// find screen radius we'll be scattering into
	dl.x = -leftv.x * scale2d * realRadius;
	dl.y = -leftv.y * scale2d * realRadius;
	dl.z = -leftv.z * scale2d * realRadius;
	ws.x = position.x + dl.x ;
	ws.y = position.y + dl.y ;
	ws.z = position.z + dl.z ;
	renderer->TransformPoint( &position,  &spos );
	if ( spos.csZ < 1.0f )
		return;

	// hack to make long hanging smoke look better
	if ( type == DRAW2D_LONG_HANGING_SMOKE2 )
	{
		scaleZ = 1.0f;
	}
	else
	{
		scaleZ = (SCATTER_ZMAX - spos.csZ)/SCATTER_ZMAX;
		if ( scaleZ < 0.0f ) scaleZ = 0.0f;
		scaleZ *= scaleZ;
	}

	renderer->TransformPoint( &ws,  &v0 );
	screenR = (float)fabs( v0.x - spos.x );
	/*
	if (screenR > sMaxScreenRes * xRes * 0.5f )
	{
		screenR = xRes * sMaxScreenRes * 0.5f;
	}
	*/
	elementRbase = screenR * 0.4f + ( 1.0f - scaleZ ) * 0.6f;
	screenR *= 0.4f - ( 1.0f - scaleZ ) * 0.3f;

	// setup the 4 verts, the only thing we'll be modifiying in the
	// scatter loop is the x,y screen coords
	v0.csZ = spos.csZ;
	v0.clipFlag = 0;
	v0.q = v0.csZ * 0.001f;
	v1.csZ = spos.csZ;
	v1.clipFlag = 0;
	v1.q = v0.q;
	v2.csZ = spos.csZ;
	v2.clipFlag = 0;
	v2.q = v0.q;
	v3.csZ = spos.csZ;
	v3.clipFlag = 0;
	v3.q = v0.q;

	// a nicer effect: put a point in the center of a square, make it
	// a dark alpha and have the edges fade to 0.  LOD this effect.
	doFivePoints = ( scaleZ * sLOD > 0.4f || type == DRAW2D_LONG_HANGING_SMOKE2 );

	// setup rendering context
//	if( renderer->GetAlphaMode() )
//	{
		if(!sGreenMode) //JAM - FIXME
			renderer->context.RestoreState(STATE_ALPHA_TEXTURE_GOURAUD);
		else
			renderer->context.RestoreState(STATE_ALPHA_TEXTURE_GOURAUD);
/*	}
	else
	{
		renderer->context.RestoreState(STATE_ALPHA_TEXTURE);
		doFivePoints = FALSE;
	}
*/
	if( !sGreenMode )
	{
		if( renderer->GetFilteringMode() )
		{
			renderer->context.SetState( MPR_STA_TEX_FILTER, MPR_TX_BILINEAR );
//			renderer->context.InvalidateState();
		}
	}

	renderer->context.SetState(MPR_STA_ALPHA_OP_FUNCTION,MPR_TO_MODULATE); //JAM 18Oct03
	renderer->context.SelectTexture1( curTex->TexHandle() );

	if ( type == DRAW2D_LONG_HANGING_SMOKE2 && doFivePoints )
		numToPlot = (NUM_EXPLODE_SCATTER_POINTS);
	else if ( typeData.flags & FIRE_SCATTER_PLOT )
		numToPlot = max( 1, (int)(NUM_FIRE_SCATTER_POINTS * scaleZ * sLOD));
	else if ( typeData.flags & SMOKE_SCATTER_PLOT )
		numToPlot = max( 1, (int)(NUM_SMOKE_SCATTER_POINTS * scaleZ * sLOD));
	else
		numToPlot = max( 1, (int)(NUM_EXPLODE_SCATTER_POINTS * scaleZ * sLOD));

	// color and alpha
	if ( doFivePoints )
	{
		vm.a = alpha;
		vm.r = 1.0f;
		vm.g = 1.0f;
		vm.b = 1.0f;
		vm.csZ = spos.csZ;
		vm.clipFlag = 0;
		vm.q = v0.q;
		v0.a = v1.a = 0.0f;
		v2.a = v3.a = 0.0f;
		v0.g = v2.g = v3.g = v1.g = 0.0f;
		// randomly tweak the edge RGB's to give a glowy kind of effect
		if ( typeData.flags & NO_RANDOM_BLEND )
		{
			v0.r = v2.r = v3.r = v1.r = 0.0f;
			v0.b = v2.b = v3.b = v1.b = 0.0f;
		}
		else
		{
			v0.r = v2.r = v3.r = v1.r = 0.1f + NRANDPOS * 0.8f;
			v0.b = v2.b = v3.b = v1.b = 0.1f + NRANDPOS * 0.8f;
		}
	}
	else if ( typeData.flags & EXPLODE_SCATTER_PLOT )
	{
		v0.r = v1.r = 1.0f;
		v0.g = v1.g = 1.0f;
		v0.b = v1.b = 1.0f;
		v0.a = v1.a = alpha * (1.0f - scaleZ);
		v2.r = v3.r = 1.0f;
		v2.g = v3.g = 1.0f;
		v2.b = v3.b = 1.0f;
		v2.a = v3.a = alpha;
	}
	else
	{
		v0.r = v1.r = 0.0f;
		v0.g = v1.g = 0.0f;
		v0.b = v1.b = 0.0f;
		v0.a = v1.a = alpha * (1.0f - scaleZ);
		v2.r = v3.r = 1.0f;
		v2.g = v3.g = 1.0f;
		v2.b = v3.b = 1.0f;
		v2.a = v3.a = alpha;
	}


	// now run the scatter loop
	for ( i = 0; i < numToPlot; i++ )
	{
		// center of this element
		if ( typeData.flags & FIRE_SCATTER_PLOT )
		{
			dl.x = gFireScatterPoints[ curSFrame ][ i ].x * screenR + spos.x;
			if (gFireScatterPoints[ curSFrame ][ i ].y > 0.0f )
				dl.y = gFireScatterPoints[ curSFrame ][ i ].y * screenR + spos.y;
			else
				dl.y = gFireScatterPoints[ curSFrame ][ i ].y * screenR * 1.5f + spos.y;
			elementR = gFireScatterPoints[ curSFrame ][ i ].z * elementRbase +
						elementRbase * 0.25f;
		}
		else if ( typeData.flags & EXPLODE_SCATTER_PLOT )
		{
			dl.x = gExplodeScatterPoints[ curSFrame ][ i ].x * screenR + spos.x;
			if (gExplodeScatterPoints[ curSFrame ][ i ].y > 0.0f )
				dl.y = gExplodeScatterPoints[ curSFrame ][ i ].y * screenR + spos.y;
			else
				dl.y = gExplodeScatterPoints[ curSFrame ][ i ].y * screenR * 1.5f + spos.y;
			elementR = gExplodeScatterPoints[ curSFrame ][ i ].z * elementRbase +
						elementRbase * 0.25f;
			if ( !doFivePoints )
			{
				if ( i == 0 )
				{
					v0.a = v1.a = alpha;
					v2.a = v3.a = alpha;
				}
				else
				{
					v0.a = v1.a = 0.45f;
					v2.a = v3.a = 0.45f;
				}
			}
		}
		else
		{
			dl.x = gSmokeScatterPoints[ curSFrame ][ i ].x * screenR + spos.x;
			if (gSmokeScatterPoints[ curSFrame ][ i ].y > 0.0f )
				dl.y = gSmokeScatterPoints[ curSFrame ][ i ].y * screenR + spos.y;
			else
				dl.y = gSmokeScatterPoints[ curSFrame ][ i ].y * screenR * 1.5f + spos.y;
			elementR = gSmokeScatterPoints[ curSFrame ][ i ].z * elementRbase +
						elementRbase * 0.25f;
		}

		// we do our own clipping checks -- for now, any point not within
		// screen bounds, we toss the entire element
		if ( dl.x < left || dl.y < top || dl.x > right || dl.y > bottom )
			continue;

		v0.x = dl.x - elementR;
		v0.y = dl.y - elementR;
		if ( v0.x < left || v0.y < top || v0.x > right || v0.y > bottom )
			continue;

		v1.x = dl.x + elementR;
		v1.y = dl.y - elementR;
		if ( v1.x < left || v1.y < top || v1.x > right || v1.y > bottom )
			continue;

		v2.x = dl.x + elementR;
		v2.y = dl.y + elementR;
		if ( v2.x < left || v2.y < top || v2.x > right || v2.y > bottom )
			continue;

		v3.x = dl.x - elementR;
		v3.y = dl.y + elementR;
		if ( v3.x < left || v3.y < top || v3.x > right || v3.y > bottom )
			continue;

		if ( typeData.flags & ANIM_NO_CLAMP )
		{
			// ANIM_NO_CLAMP uses a type of sequence animation: the frames
			// are sequenced and staggered such that the 1st point begins
			// the animation followed by point 2, etc....  Usefull for
			// rippling type explosions
			int texseq = curFrame - i;

			if ( texseq < 0 || texseq >= typeData.numTextures )
				continue;

			texseq += typeData.startTexture;
			v1.u = gTexUV[ texseq ].u;
			v1.v = gTexUV[ texseq ].v;
		}
		else if ( typeData.flags & SEQ_SCATTER_ANIM )
		{
			int texseq = typeData.startTexture + (curFrame+i) % typeData.numTextures;

			v1.u = gTexUV[ texseq ].u;
			v1.v = gTexUV[ texseq ].v;
		}
		else
		{
			v1.u = gTexUV[ typeData.startTexture + curFrame].u;
			v1.v = gTexUV[ typeData.startTexture + curFrame].v;
		}

		v0.u = v1.u + TEX_UV_DIM;
		v0.v = v1.v;
		v3.u = v1.u + TEX_UV_DIM;
		v3.v = v1.v + TEX_UV_DIM;
		v2.u = v1.u;
		v2.v = v1.v + TEX_UV_DIM;

		// do a middle point....
		if ( doFivePoints )
		{
			vm.x = dl.x;
			vm.y = dl.y;
			vm.u = v1.u + TEX_UV_DIM * 0.5f;
			vm.v = v1.v + TEX_UV_DIM * 0.5f;
			bool gif = false;
			if (g_nGfxFix & 0x02)
				gif = true;
				
			renderer->DrawTriangle( &v0, &v1, &vm, CULL_ALLOW_ALL, gif );
			renderer->DrawTriangle( &v1, &v2, &vm, CULL_ALLOW_ALL, gif );
			renderer->DrawTriangle( &v2, &v3, &vm, CULL_ALLOW_ALL, gif );
			renderer->DrawTriangle( &v3, &v0, &vm, CULL_ALLOW_ALL, gif );
		}
		else
		{
			if (g_nGfxFix & 0x02)
				renderer->DrawSquare( &v0, &v1, &v2, &v3, CULL_ALLOW_ALL, true);
			else 
				renderer->DrawSquare( &v0, &v1, &v2, &v3, CULL_ALLOW_ALL );
		}

	}

};


/*
** Name: DrawTexturedCone
** Description:
**		Basically this is 1 segment of a trail.  It can be animated.
**		The dimensions of the segment are specified by the orientation
**		matrix ( direction ) and 1 Tpoint where x is the length dim, y
**		is the radius at the start position and z iis the radius at the
**		end point.
*/
void Drawable2D::DrawTexturedCone( class RenderOTW *renderer, int)
{
	Tpoint	left, right;
	Tpoint	DOV, UP;
	float	dx, dy, dz;
	float	widthX, widthY, widthZ;
	float	mag, normalizer;
	Tpoint  end;
	ThreeDVertex		v0, v1, v2, v3;
	UV					uvStart;

	// setup rendering context
//	if(renderer->GetAlphaMode()) //JAM - FIXME
		renderer->context.RestoreState(STATE_ALPHA_TEXTURE_GOURAUD_PERSPECTIVE);
//	else
//		renderer->context.RestoreState(STATE_ALPHA_TEXTURE_PERSPECTIVE);

	renderer->context.SetState(MPR_STA_ALPHA_OP_FUNCTION,MPR_TO_MODULATE); //JAM 18Oct03
	renderer->context.SelectTexture1( curTex->TexHandle() );

	// get the end point
	end.x = orientation.M11 * typeData.glowVerts->x * realRadius * scale2d + position.x;
	end.y = orientation.M12 * typeData.glowVerts->x * realRadius * scale2d + position.y;
	end.z = orientation.M13 * typeData.glowVerts->x * realRadius * scale2d + position.z;
	
	// Get the vector from the eye to the trail segment in world space
	DOV.x = end.x - renderer->X();
	DOV.y = end.y - renderer->Y();
	DOV.z = end.z - renderer->Z();

	// Compute the direction of this segment in world space
	dx = end.x - position.x;
	dy = end.y - position.y;
	dz = end.z - position.z;
	
	// Compute the cross product of the two vectors
	widthX = DOV.y * dz - DOV.z * dy;
	widthY = DOV.z * dx - DOV.x * dz;
	widthZ = DOV.x * dy - DOV.y * dx;

	// Compute the magnitude of the cross product result
	mag = (float)sqrt( widthX*widthX + widthY*widthY + widthZ*widthZ );

	// If the cross product was degenerate (parallel vectors), use the "up" vector
	if (mag < 0.001f) {
	 	renderer->GetUp( &UP );
		widthX = UP.x;
		widthY = UP.y;
		widthZ = UP.z;
		mag = (float)sqrt( widthX*widthX + widthY*widthY + widthZ*widthZ );
	}

	// Normalize the width vector, then scale it to 1/2 of the total width of the segment
	normalizer = scale2d * realRadius / mag;
	widthX *= normalizer ;
	widthY *= normalizer ;
	widthZ *= normalizer ;


	// Compute the world space location of the two corners at the end of this segment
	left.x  = end.x - widthX * typeData.glowVerts->z;
	left.y  = end.y - widthY * typeData.glowVerts->z;
	left.z  = end.z - widthZ * typeData.glowVerts->z;
	right.x = end.x + widthX * typeData.glowVerts->z;
	right.y = end.y + widthY * typeData.glowVerts->z;
	right.z = end.z + widthZ * typeData.glowVerts->z;

	// Transform the two new corners
	renderer->TransformPoint( &left,  &v0  );
	renderer->TransformPoint( &right, &v1 );

	left.x  = position.x - widthX * typeData.glowVerts->y;
	left.y  = position.y - widthY * typeData.glowVerts->y;
	left.z  = position.z - widthZ * typeData.glowVerts->y;
	right.x = position.x + widthX * typeData.glowVerts->y;
	right.y = position.y + widthY * typeData.glowVerts->y;
	right.z = position.z + widthZ * typeData.glowVerts->y;

	// Transform the two new corners
	renderer->TransformPoint( &left,  &v3  );
	renderer->TransformPoint( &right, &v2 );

	// set up
	v0.r = 1.0f;
	v0.g = 1.0f;
	v0.b = 1.0f;
	v0.a = alpha;
	v1.r = 1.0f;
	v1.g = 1.0f;
	v1.b = 1.0f;
	v1.a = alpha;
	v2.r = 1.0f;
	v2.g = 1.0f;
	v2.b = 1.0f;
	v2.a = alpha;
	v3.r = 1.0f;
	v3.g = 1.0f;
	v3.b = 1.0f;
	v3.a = alpha;

	uvStart.u = gTexUV[ typeData.startTexture + curFrame ].u;
	uvStart.v = gTexUV[ typeData.startTexture + curFrame ].v;

	v3.u = uvStart.u;
	v3.v = uvStart.v;
	v0.u = uvStart.u + TEX_UV_DIM;
	v0.v = uvStart.v;
	v1.u = uvStart.u + TEX_UV_DIM;
	v1.v = uvStart.v + TEX_UV_DIM;
	v2.u = uvStart.u;
	v2.v = uvStart.v + TEX_UV_DIM;

	v0.q = v0.csZ * 0.001f;
	v1.q = v1.csZ * 0.001f;
	v2.q = v2.csZ * 0.001f;
	v3.q = v3.csZ * 0.001f;

	renderer->DrawSquare( &v0, &v1, &v2, &v3, CULL_ALLOW_ALL );
}

/*
** Name: SetLOD
** Description:
*/
void Drawable2D::SetLOD( float LOD )
{
	sLOD = LOD;
	sMaxScreenRes = 0.1f + sLOD * 0.9f;
}

/*
** Name: SetGreenMode
** Description:
*/
void Drawable2D::SetGreenMode( BOOL mode )
{
	// if mode is non zero, set greenmode to 1st green texture offset
	if ( mode )
		sGreenMode = 15;
	else
		sGreenMode = 0;
}


/*
** Name: DrawGlowSphere
** Description:
**		Draws an alpha faded spherical colored object
**		This version is a static function that can be called by
**		other classes.
*/
void Drawable2D::DrawGlowSphere( class RenderOTW *renderer, Tpoint *pos, float radius, float alpha )
{
	Tpoint				center, os;
	ThreeDVertex		v0, v1, v2, vLast;
	int					i;
	TYPES2D 			*typeData;		// data for anim type

	// get the type data info
	typeData = &gTypeTable[ DRAW2D_EXPLCIRC_GLOW ];


	// set up the verts rgb and alpha's

	if ( sGreenMode )
	{
		v0.r = 0.0f;
		v0.g = 1.0f;
		v0.b = 0.0f;
		v0.a = alpha;
	
		v1.r = 0.0f;
		v1.g = 1.0f;
		v1.b = 0.0f;
		v1.a = 0.0f;
	
		v2.r = 0.0f;
		v2.g = 1.0f;
		v2.b = 0.0f;
		v2.a = 0.0f;
	}
	else
	{
		v0.r = 1.0f;
		v0.g = 1.0f;
		v0.b = 1.0f;
		v0.a = alpha;
	

		v1.r = 0.1f + NRANDPOS * 0.8f;
		v1.g = 0.0f;
		v1.b = 0.1f + NRANDPOS * 0.8f;
		v1.a = 0.0f;

		v2.r = 0.1f + NRANDPOS * 0.8f;
		v2.g = 0.0f;
		v2.b = 0.1f + NRANDPOS * 0.8f;
		v2.a = 0.0f;

	}


	// Set up our drawing mode
	renderer->context.RestoreState(STATE_ALPHA_GOURAUD);
	renderer->context.SetState(MPR_STA_ALPHA_OP_FUNCTION,MPR_TO_MODULATE); //JAM 18Oct03

	// transform the center point
	renderer->TransformPointToView( pos,  &center );
	renderer->TransformPoint( pos,  &v0 );

	// do 1st point
	os.x = typeData->glowVerts[0].x * radius;
	os.y = typeData->glowVerts[0].y * radius;
	os.z = typeData->glowVerts[0].z * radius;
	renderer->TransformBillboardPoint( &os,  &center, &v1 );

	// save it for last tri in strip
	vLast = v1;

	for ( i = 1; i < typeData->numGlowVerts; i++ )
	{
		// get 3rd point of triangle
		os.x = typeData->glowVerts[i].x * radius;
		os.y = typeData->glowVerts[i].y * radius;
		os.z = typeData->glowVerts[i].z * radius;
		renderer->TransformBillboardPoint( &os,  &center, &v2 );

		renderer->DrawTriangle( &v0, &v1, &v2, CULL_ALLOW_ALL );

		// move v2 to v1 for next time thru loop
		v1 = v2;
	}

	// kick out last triangle
	renderer->DrawTriangle( &v0, &v1, &vLast, CULL_ALLOW_ALL );
}

// optimize back on
#pragma optimize("",on) //JAM - ??
