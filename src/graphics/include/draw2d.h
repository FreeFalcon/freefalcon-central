/*
** Name: DRAW2D.H
** Description:
**		Class description for drawable 2d objects (billboards).  These
**		may (or not) have a sequence of textures to use for animations.
** History:
**		14-Jul-97 (edg)  We go dancing in .....
*/
#ifndef _DRAW2D_H_
#define _DRAW2D_H_

#include "DrawObj.h"
#include "DrawBSP.h"	// Included only until we can derive from DrawableObject

// types of 2d objects
#define DRAW2D_AIR_EXPLOSION1		0
#define DRAW2D_SMALL_HIT_EXPLOSION	1
#define DRAW2D_SMOKECLOUD1			2
#define DRAW2D_FLARE				3
#define DRAW2D_AIR_EXPLOSION2		4
#define DRAW2D_SMOKERING			5
#define DRAW2D_SMALL_CHEM_EXPLOSION	6
#define DRAW2D_CHEM_EXPLOSION		7
#define DRAW2D_SMALL_DEBRIS_EXPLOSION 8
#define DRAW2D_DEBRIS_EXPLOSION		9
#define DRAW2D_CLOUD5				10
#define DRAW2D_AIR_DUSTCLOUD		11
#define DRAW2D_GUNSMOKE				12
#define DRAW2D_SMOKECLOUD2			13
#define DRAW2D_EXPLSTAR_GLOW		14
#define DRAW2D_MISSILE_GLOW			15
#define DRAW2D_EXPLCIRC_GLOW		16
#define DRAW2D_EXPLCROSS_GLOW		17
#define DRAW2D_CRATER1				18
#define DRAW2D_CRATER2				19
#define DRAW2D_CRATER3				20
#define DRAW2D_FIRE					21
#define DRAW2D_GROUND_STRIKE		22
#define DRAW2D_WATER_STRIKE			23
#define DRAW2D_HIT_EXPLOSION		24
#define DRAW2D_SPARKS				25
#define DRAW2D_ARTILLERY_EXPLOSION	26
#define DRAW2D_SHOCK_RING			27
#define DRAW2D_LONG_HANGING_SMOKE	28
#define DRAW2D_SHAPED_FIRE_DEBRIS	29
#define DRAW2D_WATER_CLOUD			30
#define DRAW2D_DARK_DEBRIS			31
#define DRAW2D_FIRE_DEBRIS			32
#define DRAW2D_LIGHT_DEBRIS			33
#define DRAW2D_EXPLCIRC_GLOW_FADE	34
#define DRAW2D_SHOCK_RING_SMALL		35
#define DRAW2D_FAST_FADING_SMOKE	36
#define DRAW2D_LONG_HANGING_SMOKE2	37
#define DRAW2D_FLAME				38
#define DRAW2D_FIRE_EXPAND			39
#define DRAW2D_GROUND_DUSTCLOUD		40
#define DRAW2D_FIRE_HOT				41
#define DRAW2D_FIRE_MED				42
#define DRAW2D_FIRE_COOL			43
#define DRAW2D_FIRE1				44
#define DRAW2D_FIRE2				45
#define DRAW2D_FIRE3				46
#define DRAW2D_FIRE4				47
#define DRAW2D_FIRE5				48
#define DRAW2D_FIRE6				49
#define DRAW2D_FIRESMOKE			50
#define DRAW2D_TRAILSMOKE			51
#define DRAW2D_TRAILDUST			52
#define DRAW2D_FIRE7				53
#define DRAW2D_BLUE_CLOUD			54
#define DRAW2D_STEAM_CLOUD			55
#define DRAW2D_GROUND_FLASH			56
#define DRAW2D_GROUND_GLOW			57
#define DRAW2D_MISSILE_GROUND_GLOW	58
#define DRAW2D_BIG_SMOKE1			59
#define DRAW2D_BIG_SMOKE2			60
#define DRAW2D_BIG_DUST				61
#define DRAW2D_INCENDIARY_EXPLOSION	62
#define DRAW2D_WATERWAKE			63
extern const int DRAW2D_MAXTYPES;

// table entry to control the animation seq for different types of objects
typedef struct _TYPES2D
{
	int flags;			// controls sequencing
		#define	ANIM_STOP			0x00000001		// stop when end reached
		#define	ANIM_HOLD_LAST		0x00000002		// hold last frame
		#define	ANIM_LOOP			0x00000004		// loop over again
		#define	ANIM_LOOP_PING		0x00000008		// fwd-back loop
		#define	FADE_LAST			0x00000010		// fade only on last frame
		#define	FADE_START			0x00000020		// start fade immediately
		#define	HAS_ORIENTATION		0x00000040		// uses rot matrix
		#define	ALPHA_BRIGHTEN		0x00000080		// use brighten blending
		#define	USES_BB_MATRIX		0x00000100		// use billboard (roll) matrix
		#define	USES_TREE_MATRIX	0x00000200		// use tree (roll+pitch) matrix
		#define	GLOW_SPHERE			0x00000400		// special type
		#define	GLOW_RAND_POINTS	0x00000800		// randomize points of star
		#define	ANIM_HALF_RATE		0x00001000		// animate 8 frames/sec
		#define	FIRE_SCATTER_PLOT	0x00002000		// kind of a particle effect
		#define	SMOKE_SCATTER_PLOT	0x00004000		// kind of a particle effect
		#define	TEXTURED_CONE		0x00008000		// "shaped" effect
		#define	EXPLODE_SCATTER_PLOT 0x00010000		// kind of a particle effect
		#define	SEQ_SCATTER_ANIM	0x00020000		// sequence animation frames in scatter plots
		#define	ANIM_NO_CLAMP		0x00040000		// don't clamp maximum texid -- used in scatter
		#define	GOURAUD_TRI			0x00080000		// just a solid color tri
		#define	ALPHA_DAYLIGHT		0x00100000		// do alpha in daylight
		#define	DO_FIVE_POINTS		0x00200000		// do dark alpha in center
		#define	NO_RANDOM_BLEND		0x00400000		// don't randomly blend rgb's
		#define	ALPHA_PER_TEXEL		0x00800000		// use APL texture set
		#define	NO_FIVE_POINTS		0x01000000		// disable 5 point
		#define	RAND_START_FRAME	0x02000000		// randomize scatter start
		#define	GROUND_GLOW			0x04000000		// align with ground

	float initAlpha;	// starting alpha value
	float fadeRate;		// rate of alpha fade/ms (i.e. 1 sec total fade = .001)
	int texId;			// array of texure Ids in sequence
	int startTexture;	// staring texture in sheet
	int numTextures;	// number of textures in sequence
	float expandRate;	// rate radius explands in ft/sec
	Tpoint *glowVerts;	// glowing "sphere" type objs
	int numGlowVerts;	// number of verts
	float maxExpand;	// maximum expand size
} TYPES2D;


class Texture;

class Drawable2D : public DrawableObject {

#ifdef USE_SH_POOLS
  public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(Drawable2D) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(Drawable2D), 400, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif

  public:
	Drawable2D( int type, float scale, Tpoint *p );
	Drawable2D( int type, float scale, Tpoint *p, Trotation *rot );
	Drawable2D( int type, float scale, Tpoint *p, int nVerts, Tpoint *verts, Tpoint *uvs );
	virtual ~Drawable2D();

	virtual void Draw( class RenderOTW *renderer, int LOD );
	virtual void Update( const Tpoint *pos, const Trotation *rot );
	void SetPosition( Tpoint *p );
	void SetStartTime ( DWORD start, DWORD now );
	void SetScale2D( float s ) { scale2d = s; };
	void SetRadius( float r ) { initRadius = radius = realRadius = r; };
	void SetAlpha( float a ) { initAlpha = alpha = a; };
	float GetScale2D( void ) { return scale2d; };
	float GetAlphaTimeToLive( void );
	static void SetLOD( float LOD );
	static void SetGreenMode( BOOL mode );
	static void DrawGlowSphere( class RenderOTW *renderer, Tpoint *pos, float radius, float alpha  );

  protected:
    int	type;				// type
  	float alpha;			// global alpha value for the object
  	float initAlpha;		// global alpha value for the object
  	float initRadius;		// radius value for the object
	int curFrame;			// current frame we're showing
	int curSFrame;			// current scatter frame we're showing
	int firstFrame;			// current frame we're showing
	int curBFrame;			// current base frame we're showing
	unsigned int startTime; // in ms
	unsigned int alphaStartTime; // in ms
	unsigned int expandStartTime; // in ms
	unsigned int startSFrame; // in ms
	TYPES2D typeData;		// data for anim type
	BOOL startFade;			// start fading?
	BOOL explicitStartTime;	// caller set start time
	Texture *curTex;		// last valid texture we got
	Tpoint oVerts[4];		// object space verts
	Tpoint uvCoords[4];		// x = u, y = v
	int numObjVerts;		// number of obj space verts ( = 3 or 4 )
	float scale2d;			// scale of the object
	float realRadius;		// real radius of poly, we may want radius
							// to be larger for better sorting

	// TODO:  Move this to a subclass???
	BOOL hasOrientation;	// uses orientation matrix
	Trotation orientation;	// orientation

	void DrawGlowSphere( class RenderOTW *renderer, int LOD );
	void DrawGouraudTri( class RenderOTW *renderer, int LOD );
	void DrawTexturedCone( class RenderOTW *renderer, int LOD );

	int GetAnimFrame( int dT, DWORD start );

	void ScatterPlot( class RenderOTW *renderer );
	void APLScatterPlot( class RenderOTW *renderer );

	// Handle time of day notifications
	static void TimeUpdateCallback( void *unused );

  public:
	static void SetupTexturesOnDevice( DXContext *rc );
	static void ReleaseTexturesOnDevice( DXContext *rc );
};


// external proto for lens flare function
void Draw2DLensFlare( class RenderOTW *renderer );
void Draw2DSunGlowEffect( class RenderOTW *renderer, Tpoint *cntr, float dist, float alpha );

#endif // _DRAW2D_H_
