/***************************************************************************\
    StateStack.h
    Scott Randolph
    February 9, 1998

    JAM 06Jan04 - Begin Major Rewrite.
\***************************************************************************/
#ifndef _STATESTACK_H_
#define _STATESTACK_H_

#include "Matrix.h"
#include "PolyLib.h"
#include "ColorBank.h"
#include "BSPNodes.h"
#include "d3d.h"
#include "vmath.h"

class ObjectInstance;
class BSubTree;

// The one and only state stack.  This would need to be replaced
// by pointers to instances of StateStackClass passed to each call
// if more than one stack were to be simultaniously maintained.
extern class StateStackClass TheStateStack;

static const int MAX_STATE_STACK_DEPTH				= 20;	// Arbitrary MLR 2003-10-11 Upped depth
static const int MAX_SLOT_AND_DYNAMIC_PER_OBJECT	= 64;	// Arbitrary
static const int MAX_TEXTURES_PER_OBJECT			= 128;	// Arbitrary
static const int MAX_CLIP_PLANES					= 6;	// 5 view volume, plus 1 extra
static const int MAX_VERTS_PER_POLYGON				= 32;	// Arbitrary
static const int MAX_VERT_POOL_SIZE					= 8192;	// Arbitrary
static const int MAX_VERTS_PER_CLIPPED_POLYGON		= MAX_VERTS_PER_POLYGON + MAX_CLIP_PLANES;
static const int MAX_CLIP_VERTS						= 2 * MAX_CLIP_PLANES;
static const int MAX_VERTS_PER_OBJECT_TREE			= MAX_VERT_POOL_SIZE - MAX_VERTS_PER_POLYGON - MAX_CLIP_VERTS;

typedef void (*TransformFp)(Ppoint *p,int n);

typedef struct StateStackFrame
{
	Spoint					*XformedPosPool;
	Pintensity				*IntensityPool;
	PclipInfo				*ClipInfoPool;

	Pmatrix					Rotation;
	Ppoint					Xlation;

	D3DFrame::Matrix mV;
	D3DFrame::Matrix mW;
	D3DFrame::Matrix mP;

	Ppoint	ObjSpaceEye;
	Ppoint	ObjSpaceLight;

	const int 				*CurrentTextureTable;
	class ObjectInstance	*CurrentInstance;
	const class ObjectLOD	*CurrentLOD;

	const DrawPrimFp		*DrawPrimJumpTable;
	TransformFp				Transform;
}
StateStackFrame;

class StateStackClass
{
  public:
	StateStackClass();
	~StateStackClass() { ShiAssert(stackDepth == 0); };

  public:
	static void SetContext(ContextMPR *cntxt);

	static void	SetLight(float a,float d,float s,Ppoint *v);
	static void SetCameraProperties(float ooTanHHAngle,float ooTanVHAngle,float sclx,float scly,float shftx,float shfty);
	static void SetLODBias(float bias);
	static void SetTextureState(BOOL state);
	static void SetFog(float percent,Pcolor *color);

	static void SetCamera(const Ppoint *pos,const Pmatrix *rotWaspect,Pmatrix *Bill,Pmatrix *Tree);

	static void SetView(const Ppoint *pos,Pmatrix *cameraRot); 
	static void SetWorld(const Pmatrix *rot,const Ppoint *pos);
	static void SetProjection(float fov,float aspect);

	static void DrawObject(ObjectInstance *objInst,const Pmatrix *rot,const Ppoint *pos,const float scale=1.f);
	static void DrawWarpedObject(ObjectInstance *objInst,const Pmatrix *rot,const Ppoint *pos,const float sx,const float sy,const float sz,const float scale=1.f);

	// Called by BRoot nodes at draw time
	static void DrawSubObject(ObjectInstance *objInst,const Pmatrix *rot,const Ppoint *pos);
	static void	CompoundTransform(const Pmatrix *rot,const Ppoint *pos);
	static void	Light(const Pnormal *pNormals,int nNormals,const Ppoint *pCoords);
	static void SetTextureTable(const int *pTexIDs) { CurrentTextureTable = pTexIDs; };
	static void	PushAll(void);
	static void	PopAll(void);
	static void	PushVerts(void);
	static void	PopVerts(void);

	// This should be cleaned up and probably have special clip/noclip versions
	static void TransformBillboardWithClip(Ppoint *p,int n,BTransformType type);

	// Called by our own transformations and the clipper
    inline static float XtoPixel(float x) { return (x*scaleX)+shiftX; };
    inline static float YtoPixel(float y) { return (y*scaleY)+shiftY; };

	// For parameter validation during debug
	static BOOL IsValidPosIndex(int i);
	static BOOL IsValidIntensityIndex(int i);

  protected:
	static void	TransformNoClip(Ppoint *pCoords,int nCoords);
	static void	TransformWithClip(Ppoint *pCoords,int nCoords);

	static /*inline*/ DWORD	CheckBoundingSphereClipping(void);
	static /*inline*/ void TransformInline(Ppoint *pCoords,int nCoords,const BOOL clip);

	static /*inline*/ void pvtDrawObject(DWORD operation,ObjectInstance *objInst,const Pmatrix *rot,const Ppoint *pos,const float sx,const float sy,const float sz,const float scale=1.f);

  public:
	// Active transformation function (selects between with or without clipping)
	static TransformFp Transform;

	// Computed data pools
	static Spoint		*XformedPosPool;	// These point into global storage. They will point
	static Pintensity	*IntensityPool;		// to the computed tables for each sub-object.
	static PclipInfo	*ClipInfoPool;
	static Spoint		*XformedPosPoolNext;// These point into global storage. They will point
	static Pintensity	*IntensityPoolNext;	// to at least MAX_CLIP_VERTS empty slots beyond 
	static PclipInfo	*ClipInfoPoolNext;	// the computed tables in use by the current sub-object.

	// Instance of the object we're drawing and its range normalized for resolution and FOV
	static class ObjectInstance		*CurrentInstance;
	static const class ObjectLOD	*CurrentLOD;
	static const int				*CurrentTextureTable;
	static float					LODRange;

	// Fog properties
	static float		fogValue;			// fog precent (set by SetFog())

	// Final transform
	static Pmatrix		Rotation;			// These are the final camera transform
	static Ppoint		Xlation;			// including contributions from parent objects

	static D3DFrame::Matrix mV;
	static D3DFrame::Matrix	mW;
	static D3DFrame::Matrix	mP;

	// Fudge factors for drawing
	static float		LODBiasInv;			// This times real range is LOD evaluation range

	// Object space points of interest
	static Ppoint		ObjSpaceEye;		// Eye point in object space (for BSP evaluation)
	static Ppoint		ObjSpaceLight;		// Light location in object space(for BSP evaluation)

	// Pointers to our clients billboard and tree matrices in case we need them
	static Pmatrix		*Tb;				// Billboard (always faces viewer)
	static Pmatrix		*Tt;				// Tree (always stands up straight and faces viewer)

	// Lighting properties for the BSP objects
	static float		lightAmbient;
	static float		lightDiffuse;
	static float		lightSpecular;
	static Ppoint		lightVector;

	// The context on which we'll draw
	static ContextMPR	*context;

  protected:
	static StateStackFrame	stack[MAX_STATE_STACK_DEPTH];
	static int				stackDepth;

	// Required for object culling
	static float		hAspectWidthCorrection;
	static float		hAspectDepthCorrection;
	static float		vAspectWidthCorrection;
	static float		vAspectDepthCorrection;

    // The parameters required to get from normalized screen space to pixel space
	static float	scaleX;
    static float	scaleY;
    static float	shiftX;
    static float	shiftY;

	static int		LODused;
};

#endif // _STATESTACK_H_