/***************************************************************************\
    StateStack.cpp
    Scott Randolph
    February 9, 1998

    JAM 29Sep03 - Begin Major Rewrite.
\***************************************************************************/
#include "stdafx.h"
#include <math.h>
#include "ColorBank.h"
#include "ObjectInstance.h"
#include "ClipFlags.h"
#include "StateStack.h"
#include "context.h"
#include "vmath.h"
#include "fmath.h"
#include "FalcLib\include\playerop.h"
#include "FalcLib\include\dispopts.h"
#include "RedProfiler.h"

#include "Graphics\DXEngine\DXTools.h"
#include "Graphics\DXEngine\DXDefines.h"
#include "Graphics\DXEngine\DXEngine.h"
#include "Graphics\DXEngine\DXVBManager.h"
extern	bool g_Use_DX_Engine;


extern "C" {
	void MonoPrint(char *string, ...);
}

#define PERSP_CORR_RADIUS_MULTIPLIER 3.f

extern bool g_bSlowButSafe;
extern int g_nFogRenderState;

StateStackClass	TheStateStack;

int verts = 0;

// Create the global storage for our static members.
TransformFp	StateStackClass::Transform;
float		StateStackClass::LODRange;
float		StateStackClass::LODBiasInv;
float		StateStackClass::fogValue;
Pmatrix		StateStackClass::Rotation;
Ppoint		StateStackClass::Xlation;

D3DFrame::Matrix StateStackClass::mV;
D3DFrame::Matrix StateStackClass::mW;
D3DFrame::Matrix StateStackClass::mP;

Ppoint		StateStackClass::ObjSpaceEye;
Ppoint		StateStackClass::ObjSpaceLight;
ContextMPR	*StateStackClass::context;
const int	*StateStackClass::CurrentTextureTable;
Spoint		*StateStackClass::XformedPosPool;
Spoint		*StateStackClass::XformedPosPoolNext;
Pintensity	*StateStackClass::IntensityPool;
Pintensity	*StateStackClass::IntensityPoolNext;

PclipInfo	*StateStackClass::ClipInfoPool;
PclipInfo	*StateStackClass::ClipInfoPoolNext;
class ObjectInstance *StateStackClass::CurrentInstance;
const class ObjectLOD *StateStackClass::CurrentLOD;
StateStackFrame StateStackClass::stack[MAX_STATE_STACK_DEPTH];
int			StateStackClass::stackDepth;
float		StateStackClass::hAspectWidthCorrection;
float		StateStackClass::hAspectDepthCorrection;
float		StateStackClass::vAspectWidthCorrection;
float		StateStackClass::vAspectDepthCorrection;
float		StateStackClass::scaleX;
float		StateStackClass::scaleY;
float		StateStackClass::shiftX;
float		StateStackClass::shiftY;
int			StateStackClass::LODused;
Pmatrix		*StateStackClass::Tt;
Pmatrix		*StateStackClass::Tb;
float		StateStackClass::lightAmbient = .3f;
float		StateStackClass::lightDiffuse = .6f;
float		StateStackClass::lightSpecular = .6f;
Ppoint		StateStackClass::lightVector;

// Reserved storage space for computed values.
static Spoint		XformedPosPoolBuffer[MAX_VERT_POOL_SIZE];
static Pintensity	IntensityPoolBuffer[MAX_VERT_POOL_SIZE];
static PclipInfo	ClipInfoPoolBuffer[MAX_VERT_POOL_SIZE];

inline void normalizeVector(Ppoint *v)
{
	float mag = SqrtF(v->x*v->x+v->y*v->y+v->z*v->z);
	v->x /= mag;
	v->y /= mag;
	v->z /= mag;
}

// Functions used to maintain the states above.
StateStackClass::StateStackClass()
{
	stackDepth			= 0;
	XformedPosPoolNext	= XformedPosPoolBuffer;
	IntensityPoolNext		= IntensityPoolBuffer;
	ClipInfoPoolNext	= ClipInfoPoolBuffer;
	LODBiasInv			= 1.0f;
	SetTextureState(TRUE);
	SetFog(1.f,NULL);
}

void StateStackClass::SetContext(ContextMPR *cntxt)
{
	context = cntxt;
}

void StateStackClass::SetLight(float a, float d, float s, Ppoint *atLight)
{
	lightAmbient = d;
	lightDiffuse = a;
	lightSpecular = s;

	memcpy(&lightVector,atLight,sizeof(Ppoint));
	ObjSpaceLight = lightVector;

	// COBRA - DX - Switching btw Old and New Engine - the DX SUNLight
	if(g_Use_DX_Engine){
		// Setup light properties
		TheDXEngine.SetSunLight(a, d ,s);
		
		// Setup Light Vector - Light direction has to be reversed
		/*D3DVECTOR	Dir;
		Dir.x=ObjSpaceLight.y;
		Dir.y=ObjSpaceLight.z;
		Dir.z=ObjSpaceLight.x;
		TheDXEngine.SetSunVector(Dir);*/
	}

}

void StateStackClass::SetCameraProperties(float ooTanHHAngle, float ooTanVHAngle, float sclx, float scly, float shftx, float shfty)
{
	float	rx2;

	rx2 = (ooTanHHAngle*ooTanHHAngle);
	hAspectDepthCorrection = 1.f/(float)sqrt(rx2 + 1.0f);
	hAspectWidthCorrection = rx2*hAspectDepthCorrection;

	rx2 = (ooTanVHAngle*ooTanVHAngle);
	vAspectDepthCorrection = 1.f/(float)sqrt(rx2 + 1.0f);
	vAspectWidthCorrection = rx2*vAspectDepthCorrection;

	scaleX = sclx;
	scaleY = scly;
	shiftX = shftx;
	shiftY = shfty;

}

void StateStackClass::SetTextureState(BOOL state)
{
	if(state)
	{
		RenderStateTablePC				= RenderStateTableWithPCTex;
		RenderStateTableNPC				= RenderStateTableWithNPCTex;

		DrawPrimNoFogNoClipJumpTable	= DrawPrimNoClipWithTexJumpTable;
		ClipPrimNoFogJumpTable			= ClipPrimWithTexJumpTable;

		DrawPrimFogNoClipJumpTable		= DrawPrimFogNoClipWithTexJumpTable;
		ClipPrimFogJumpTable			= ClipPrimFogWithTexJumpTable;
	}
	else
	{
		RenderStateTablePC				= RenderStateTableNoTex;
		RenderStateTableNPC				= RenderStateTableNoTex;

		DrawPrimNoFogNoClipJumpTable	= DrawPrimNoClipNoTexJumpTable;
		ClipPrimNoFogJumpTable			= ClipPrimNoTexJumpTable;

		DrawPrimFogNoClipJumpTable		= DrawPrimFogNoClipNoTexJumpTable;
		ClipPrimFogJumpTable			= ClipPrimFogNoTexJumpTable;
	}

	// FIXME
	DrawPrimNoClipJumpTable				= DrawPrimFogNoClipJumpTable;
	ClipPrimJumpTable					= ClipPrimFogJumpTable;
}

void StateStackClass::SetFog(float alpha, Pcolor *color)
{
	if(color)
	{
		UInt32 c;

		c  = FloatToInt32(color->r * 255.9f);
		c |= FloatToInt32(color->g * 255.9f) << 8;
		c |= FloatToInt32(color->b * 255.9f) << 16;
		context->SetState(MPR_STA_FOG_COLOR,c);
	}

	fogValue = alpha;

	// FIXME
	DrawPrimNoClipJumpTable = DrawPrimFogNoClipJumpTable;
	ClipPrimJumpTable		= ClipPrimFogJumpTable;
}

void StateStackClass::SetView(const Ppoint *pos, Pmatrix *cameraRot)
{
	D3DFrame::Vector vP;

	float pitch = (float)-asin(cameraRot->M13);
	float roll = (float)atan2(cameraRot->M23,cameraRot->M33);
	float yaw = (float)atan2(cameraRot->M12,cameraRot->M11);

	vP.x=pos->y; vP.y=-pos->z; vP.z=pos->x; 

	mV.SetViewMatrix(pitch,roll,yaw,vP);

	if(g_Use_DX_Engine){
		D3DXMATRIX Camera,p;
		D3DXMatrixIdentity(&Camera);
	}
}

void StateStackClass::SetWorld(const Pmatrix *rot, const Ppoint *pos)
{
	mW.InitIdentity();
	mW.m[0][0]=rot->M11; mW.m[0][1]=rot->M12; mW.m[0][2]=rot->M13;
	mW.m[1][0]=rot->M21; mW.m[1][1]=rot->M22; mW.m[1][2]=rot->M23;
	mW.m[2][0]=rot->M31; mW.m[2][1]=rot->M32; mW.m[2][2]=rot->M33; 
	mW.m[3][0]=pos->x; mW.m[3][1]=pos->y; mW.m[3][2]=pos->z; mW.m[3][3]=1.f;
}

void StateStackClass::SetProjection(float fov, float aspect)
{
//	if(context)
//		mW.SetProjectionMatrix(fov,aspect,context->ZNEAR,context->ZFAR);
}

void StateStackClass::SetCamera(const Ppoint *pos, const Pmatrix *rotWaspect, Pmatrix *Bill, Pmatrix *Tree)
{
	ShiAssert(stackDepth == 0);

	// COBRA - DX - Switching btw Old and New Engine - the Camera projections
	if(g_Use_DX_Engine){
		//***************************************************************************************
		// DX - RED - THIS IS AN HACK... This will be done completely back in the Engine module
		// as it should be used by the Old Engine as it is for Ground & other NOT BSP features...
		Pmatrix Temp;
		ZeroMemory(&Temp,sizeof(Temp));
		Temp.M31=-1.0;
		Temp.M23=-1.0;
		Temp.M12=1.0;
		MatrixMult(&Temp,rotWaspect,&Rotation);
	} else {
		// Store our rotation from world to camera space (including aspect scale effects)
		Rotation = *rotWaspect;	
	}



	// Compute the vector from the camera to the origin rotated into camera space
	Xlation.x = -pos->x * Rotation.M11 - pos->y * Rotation.M12 - pos->z * Rotation.M13;
	Xlation.y = -pos->x * Rotation.M21 - pos->y * Rotation.M22 - pos->z * Rotation.M23;
	Xlation.z = -pos->x * Rotation.M31 - pos->y * Rotation.M32 - pos->z * Rotation.M33;

	// Intialize the eye postion in world space
	ObjSpaceEye = *pos;

	Tb = Bill;
	Tt = Tree;
}

void StateStackClass::CompoundTransform(const Pmatrix *rot, const Ppoint *pos)
{
	Ppoint tempP;

	// Compute the rotated translation vector for this object
	Xlation.x += pos->x * Rotation.M11 + pos->y * Rotation.M12 + pos->z * Rotation.M13;
	Xlation.y += pos->x * Rotation.M21 + pos->y * Rotation.M22 + pos->z * Rotation.M23;
	Xlation.z += pos->x * Rotation.M31 + pos->y * Rotation.M32 + pos->z * Rotation.M33;

	Pmatrix tempM = Rotation;
	tempP.x = ObjSpaceEye.x - pos->x;
	tempP.y = ObjSpaceEye.y - pos->y;
	tempP.z = ObjSpaceEye.z - pos->z;
	Ppoint tempP2 = ObjSpaceLight;

	// Composit the camera matrix with the object rotation
	MatrixMult(&tempM,rot,&Rotation);

	// Compute the eye point in object space
	MatrixMultTranspose(rot,&tempP,&ObjSpaceEye);

	// Compute the light direction in object space.
	MatrixMultTranspose(rot,&tempP2,&ObjSpaceLight);



	// COBRA - DX - Switching btw Old and New Engine - the Camera projections
	if(g_Use_DX_Engine){
		D3DXMATRIX Camera;
		D3DXMatrixIdentity(&Camera);
		//ZeroMemory(&Camera, sizeof(Camera));
		//D3DXMatrixRotationX(&Camera, PI/2);
		TheDXEngine.SetCamera(&Camera);
		// Setup Light Vector - Light direction has to be reversed
		D3DVECTOR	Dir;
		Dir.x=ObjSpaceLight.y;
		Dir.y=-ObjSpaceLight.z;
		Dir.z=ObjSpaceLight.x;
		TheDXEngine.SetSunVector(Dir);
	}

}

// The asymetric scale factors MUST be <= 1.0f.
// The global scale factor can be any positive value.
// The effects of the scales are multiplicative.
static const UInt32	OP_NONE	= 0;
static const UInt32	OP_FOG	= 1;
static const UInt32	OP_WARP	= 2;


/*inline*/ void StateStackClass::pvtDrawObject(UInt32 operation, ObjectInstance *objInst, const Pmatrix *rot, const Ppoint *pos, const float sx, const float sy, const float sz, const float scale)
{	

	UInt32 clipFlag;
	float MaxLODRange;
	static int in = 0;
	
	ShiAssert(objInst);

	PushAll();

	// Set up our transformations
	CompoundTransform(rot,pos);


	SetWorld(rot,pos);

	if((operation & OP_WARP)||(scale != 1.f))
	{
		Pmatrix	tempM;
		float	cx,cy,cz;
		cx=cz=cy=scale;

		if(operation & OP_WARP){
			cx*=sx;
			cy*=sy;
			cz*=sz;
		}

		ShiAssert((sx > 0.0f) && (sx <= 1.0f));
		ShiAssert((sy > 0.0f) && (sy <= 1.0f));
		ShiAssert((sz > 0.0f) && (sz <= 1.0f));

		Pmatrix	stretchM = {	cx,		0.f,	0.f,
								0.f,	cy,		0.f,
								0.f,	0.f,	cz	};

		tempM = Rotation;
		MatrixMult(&tempM,&stretchM,&Rotation);

		D3DFrame::Matrix mS,mT;
		mT = mW;
		mS.InitIdentity();
		mS.m[0][0]=cx; mS.m[1][1]=cy; mS.m[2][2]=cz;
		mW = mS*mT;
	}

/*	if(scale != 1.f)
	{
		Pmatrix	tempM;

		Pmatrix scaleM = {	scale,	0.f,	0.f,
							0.f,	scale,	0.f,
							0.f,	0.f,	scale };

		tempM = Rotation;
		MatrixMult(&tempM,&scaleM,&Rotation);

		D3DFrame::Matrix mS,mT;
		mT = mW;
		mS.InitIdentity();
		mS.m[0][0]=scale; mS.m[1][1]=scale; mS.m[2][2]=scale;
		mW = mS*mT;
	}*/

	// Store the adjusted range for LOD determinations
	LODRange = Xlation.x * LODBiasInv;

	// Choose the appropriate LOD of the object to be drawn
	CurrentInstance = objInst;

	if (objInst->ParentObject)
	{
		if (g_bSlowButSafe && F4IsBadCodePtr((FARPROC) objInst->ParentObject)) // JB 010220 CTD (too much CPU)
			CurrentLOD = 0; // JB 010220 CTD
		else // JB 010220 CTD
		if (objInst->id < 0 || objInst->id >= TheObjectListLength || objInst->TextureSet < 0) // JB 010705 CTD second try
		{
			ShiAssert(FALSE);
			CurrentLOD = 0;
		}
		else 
			CurrentLOD = objInst->ParentObject->ChooseLOD(LODRange,&LODused,&MaxLODRange);

		if(CurrentLOD)
		{
			// Decide if we need clipping, or if the object is totally off screen
			clipFlag = CheckBoundingSphereClipping();

			// Continue only if some part of the bounding volume is on screen
			if (clipFlag != OFF_SCREEN)
			{
				COUNT_PROFILE("Draw Objects Nr");
			// Set the jump pointers to turn on/off clipping
				if (clipFlag == ON_SCREEN)
				{
					Transform = TransformNoClip;
					DrawPrimJumpTable = DrawPrimNoClipJumpTable;
				}
				else
				{
					Transform = TransformWithClip;
					DrawPrimJumpTable = DrawPrimWithClipJumpTable;
				}

				// Choose perspective correction or not
	//			if ((Xlation.x > CurrentInstance->Radius() * PERSP_CORR_RADIUS_MULTIPLIER) && 
	//				!(CurrentLOD->flags & ObjectLOD::PERSP_CORR))
	//			{
	//				RenderStateTable = RenderStateTableNPC;
	//			}
	//			else
	//			{
					RenderStateTable = RenderStateTablePC;
	//			}

				in ++;

				if (in == 1)
				{
					verts = 0;
				}

				// Draw the object
				CurrentLOD->Draw();

//				if (in == 1)
//				{
//					if (verts)
//					{
//						MonoPrint ("Obj %d:%d %d : %d\n", objInst->id, LODused, (int) MaxLODRange, verts);
//					}
//				}

				in --;
			}
		}
	}

	PopAll();
}

// This call is for the application to call to draw an instance of an object.
void StateStackClass::DrawObject(ObjectInstance *objInst, const Pmatrix *rot, const Ppoint *pos, const float scale)
{
	
	// COBRA - DX - Switching btw Old and New Engine - the Camera projections
	if(g_Use_DX_Engine)	{
		D3DXMATRIX	Rot,k;
		Ppoint	XSave=Xlation,EyeSave=ObjSpaceEye,LightSave=ObjSpaceLight;
		Pmatrix	RotSave=Rotation;
		CompoundTransform(rot, pos);
		AssignPmatrixToD3DXMATRIX(&Rot, &Rotation);
		/*float p=Xlation.x;
		Xlation.x=Xlation.z;
		Xlation.y=Xlation.y;
		Xlation.z=-p;*/
		/*D3DXMatrixTranslation(&k, Xlation.x, Xlation.y, Xlation.z);
		D3DXMatrixMultiply(&Rot, &Rot, &k);
		TheDXEngine.SetCamera(&Rot);
		ZeroMemory(&Rot,sizeof(Rot));
			Rot.m01=1.0f;
			Rot.m12=1.0f;
			Rot.m20=1.0f;
			Rot.m33=1.0f;
		Xlation.x=Xlation.y=Xlation.z=0;*/
		TheDXEngine.DrawObject(objInst,&Rot,&Xlation,1.f,1.f,1.f,scale);
		Xlation=XSave;
		ObjSpaceEye=EyeSave;
		ObjSpaceLight=LightSave;
		Rotation=RotSave;
	}
	else pvtDrawObject(OP_NONE,objInst,rot,pos,1.f,1.f,1.f,scale);
}

// This call is for the BSPlib to call to draw a child instance attached to a slot.
void StateStackClass::DrawSubObject(ObjectInstance *objInst, const Pmatrix *rot, const Ppoint *pos)
{
	
	// COBRA - DX - Switching btw Old and New Engine - the Camera projections
	if(g_Use_DX_Engine)	{
		D3DXMATRIX	Rot;
		AssignPmatrixToD3DXMATRIX(&Rot, (Pmatrix*)rot);
		TheDXEngine.DrawObject(objInst,&Rot,pos,1.f,1.f,1.f,1.f);
	}
	else pvtDrawObject(OP_NONE,objInst,rot,pos,1.f,1.f,1.f,1.f);
}

// This call is rather specialized.  It is intended for use in drawing shadows which  
// are simple objects (no slots, dofs, etc) but require asymetric scaling in x and y 
// to simulate orientation changes of the object casting the shadow.
void StateStackClass::DrawWarpedObject(ObjectInstance *objInst, const Pmatrix *rot, const Ppoint *pos, const float sx, const float sy, const float sz, const float scale)
{	
	// COBRA - DX - Switching btw Old and New Engine - the Camera projections
	if(g_Use_DX_Engine)	{
		D3DXMATRIX	Rot;
		AssignPmatrixToD3DXMATRIX(&Rot, (Pmatrix*)rot);
		TheDXEngine.DrawObject(objInst,&Rot,pos,sx,sy,sz,scale);
	}
	else pvtDrawObject(OP_WARP,objInst,rot,pos,sx,sy,sz,scale);
	
}

/*inline*/ UInt32 StateStackClass::CheckBoundingSphereClipping(void)
{
	// Decide if we need clipping, or if the object is totally off screen
	// REMEMBER:  Xlation is camera oriented, but still X front, Y right, Z down
	//			  so range from viewer is in the X term.
	// NOTE:  We compute "d", the distance from the viewer at which the bounding
	//		  sphere should intersect the view frustum.  We use .707 since the
	//		  rotation matrix already normalized us to a 45 degree half angle.
	//		  We do have to adjust the radius shift by the FOV correction factors,
	//		  though, since it didn't go through the matix.
	// NOTE2: We could develop the complete set of clip flags here by continuing to 
	//        check other edges instead of returning in the clipped cases.  For now,
	//        we only need to know if it IS clipped or not, so we terminate early.
	// TODO:  We should roll the radius of any attached slot children into the check radius
	//		  to ensure that we don't reject a parent object whose _children_ may be on screen.
	//        (though this should be fairly rare in practice)
	float	rd;
	float	rh;
	float	rx;
//	UInt32	clipFlag = ON_SCREEN;

	rx=CurrentInstance->Radius();
	rd = rx * vAspectDepthCorrection;
	rh = rx * vAspectWidthCorrection;
	if (-(Xlation.z - rh) >= Xlation.x - rd) {
		if (-(Xlation.z + rh) > Xlation.x + rd) {
			return OFF_SCREEN;			// Trivial reject top
		}
//		clipFlag = CLIP_TOP;
		return CLIP_TOP;
	}
	if (Xlation.z + rh >= Xlation.x - rd) {
		if (Xlation.z - rh > Xlation.x + rd) {
			return OFF_SCREEN;			// Trivial reject bottom
		}
//		clipFlag |= CLIP_BOTTOM;
		return CLIP_BOTTOM;
	}

	rd = rx * hAspectDepthCorrection;
	rh = rx * hAspectWidthCorrection;
	if (-(Xlation.y - rh) >= Xlation.x - rd) {
		if (-(Xlation.x + rh) > Xlation.x + rd) {
			return OFF_SCREEN;			// Trivial reject left
		}
//		clipFlag |= CLIP_LEFT;
		return CLIP_LEFT;
	}
	if (Xlation.y + rh >= Xlation.x - rd) {
		if (Xlation.y - rh > Xlation.x + rd) {
			return OFF_SCREEN;			// Trivial reject right
		}
//		clipFlag |= CLIP_RIGHT;
		return CLIP_RIGHT;
	}

	rh = rx;
	if (Xlation.x - rh < NEAR_CLIP_DISTANCE) {
		if (Xlation.x + rh < NEAR_CLIP_DISTANCE) {
			return OFF_SCREEN;			// Trivial reject near
		}
//		clipFlag |= CLIP_NEAR;
		return CLIP_NEAR;
	}

//	return clipFlag;
	return ON_SCREEN;
}

void StateStackClass::PushAll(void)
{
	ShiAssert(stackDepth < MAX_STATE_STACK_DEPTH);

	stack[stackDepth].XformedPosPool		= XformedPosPool;
	stack[stackDepth].IntensityPool			= IntensityPool;
	stack[stackDepth].ClipInfoPool			= ClipInfoPool;

	stack[stackDepth].Rotation				= Rotation;
	stack[stackDepth].Xlation				= Xlation;
	
	stack[stackDepth].ObjSpaceEye			= ObjSpaceEye;
	stack[stackDepth].ObjSpaceLight			= ObjSpaceLight;
	
	stack[stackDepth].CurrentInstance		= CurrentInstance;
	stack[stackDepth].CurrentLOD			= CurrentLOD;
	stack[stackDepth].CurrentTextureTable	= CurrentTextureTable;

	stack[stackDepth].DrawPrimJumpTable		= DrawPrimJumpTable;
	stack[stackDepth].Transform				= Transform;

	XformedPosPool							= XformedPosPoolNext;
	IntensityPool								= IntensityPoolNext;
	ClipInfoPool							= ClipInfoPoolNext;

	stackDepth++;
}

void StateStackClass::PopAll(void)
{
	stackDepth--;

	XformedPosPoolNext	= XformedPosPool;
	IntensityPoolNext		= IntensityPool;
	ClipInfoPoolNext	= ClipInfoPool;

	XformedPosPool		= stack[stackDepth].XformedPosPool;
	IntensityPool			= stack[stackDepth].IntensityPool;
	ClipInfoPool		= stack[stackDepth].ClipInfoPool;

	Rotation			= stack[stackDepth].Rotation;
	Xlation				= stack[stackDepth].Xlation;

	ObjSpaceEye			= stack[stackDepth].ObjSpaceEye;
	ObjSpaceLight		= stack[stackDepth].ObjSpaceLight;

	CurrentInstance		= stack[stackDepth].CurrentInstance;
	CurrentLOD			= stack[stackDepth].CurrentLOD;
	CurrentTextureTable	= stack[stackDepth].CurrentTextureTable;

	DrawPrimJumpTable	= stack[stackDepth].DrawPrimJumpTable;
	Transform			= stack[stackDepth].Transform;
}

void StateStackClass::PushVerts(void)
{
	ShiAssert(stackDepth < MAX_STATE_STACK_DEPTH);

	stack[stackDepth].XformedPosPool	= XformedPosPool;
	stack[stackDepth].IntensityPool		= IntensityPool;
	stack[stackDepth].ClipInfoPool		= ClipInfoPool;

	XformedPosPool						= XformedPosPoolNext;
	IntensityPool							= IntensityPoolNext;
	ClipInfoPool						= ClipInfoPoolNext;

	stackDepth++;
}


void StateStackClass::PopVerts(void)
{
	stackDepth--;

	XformedPosPoolNext	= XformedPosPool;
	IntensityPoolNext		= IntensityPool;
	ClipInfoPoolNext	= ClipInfoPool;

	XformedPosPool		= stack[stackDepth].XformedPosPool;
	IntensityPool			= stack[stackDepth].IntensityPool;
	ClipInfoPool		= stack[stackDepth].ClipInfoPool;
}

// Cobra - RED - This function is now about 2 times faster that it was before with following changes...!!!
// can be 3 times if FIXME is solved
void StateStackClass::Light(const Pnormal *n, int i, const Ppoint *p)
{
	float iSpec=0.f,iDiff=0.f;
	Ppoint viewVector,halfVector;

	ShiAssert(IsValidIntensityIndex(i-1));

	while(i--)
	{	
		// Cobra - RED - If Poly Normal facing other side dnt calculate light, 
		// just assign last calculated light to avoid dark spots on near polys
		//if((double)(ObjSpaceEye.x*n->i+ObjSpaceEye.y*n->j+ObjSpaceEye.z*n->k)<(double)0.0){		// Operations are following the Normal check to keep the pocessor
		//	*IntensityPoolNext = LastLight;											// cache still online and execute a backaward cache call
		//	n++; p++; IntensityPoolNext++;											// which is faster than a forward call for a P Class processor
		//	continue;																 					
		//}
		// Cobra - RED - End
			

		iDiff = max(n->i*ObjSpaceLight.x+n->j*ObjSpaceLight.y+n->k*ObjSpaceLight.z,0.f)*lightDiffuse;

		// Cobra - RED - Zero is Zero both in Float and Long...but Long is faster
		//	...........(lightSpecular).........................................
		if(!LODused && ((*(long*)&lightSpecular)&0x7fffffff) && DisplayOptions.bSpecularLighting)
		{
			viewVector.x = ObjSpaceEye.x - p->x;
			viewVector.y = ObjSpaceEye.y - p->y;
			viewVector.z = ObjSpaceEye.z - p->z;
			normalizeVector(&viewVector);

			// FIXME - RED - This check should avoid any light calculation if camera NOT 
			// LOOKING at the poly face, even iDiff could be not calculated
			// but till when BSP seems to draw even hidden polys I have to assign just
			// iDiff to have not dark spots on near polys, however avoiding iSpec calculations
			if((viewVector.x*n->i+viewVector.y*n->j+viewVector.z*n->k)<0.0){	
				*IntensityPoolNext = min(lightAmbient+iDiff,1.f);			// Operations are following the Normal check to keep the pocessor
				n++; p++; IntensityPoolNext++;								// cache still online and execute a backaward cache call		
				continue;													// which is faster than a forward call for a P Class processor
			}

			halfVector.x = ObjSpaceLight.x + viewVector.x;
			halfVector.y = ObjSpaceLight.y + viewVector.y;
			halfVector.z = ObjSpaceLight.z + viewVector.z;
			normalizeVector(&halfVector);

			// Cobra - RED - Easy using FPU... isn't it...? But now Conditional Integers improve of about 100%
			//iSpec = powf(max(n->i*halfVector.x+n->j*halfVector.y+n->k*halfVector.z,0.f),32.f)*lightSpecular;
			iSpec=n->i*halfVector.x+n->j*halfVector.y+n->k*halfVector.z;
			if(iSpec<=0) iSpec=0;
			else {
				iSpec=iSpec*iSpec;					//	iSpec^2;
				iSpec=iSpec*iSpec;					//	iSpec^4;
				iSpec=iSpec*iSpec;					//	iSpec^8;
				iSpec=iSpec*iSpec;					//	iSpec^16;
				iSpec=iSpec*iSpec;					//	iSpec^32;
				iSpec*=lightSpecular;
			}
			// Cobra - RED - End

		}

		*IntensityPoolNext = min(lightAmbient+iDiff+iSpec,1.f);
		n++; p++; IntensityPoolNext++;
	}
}


/*inline*/ void StateStackClass::TransformInline(Ppoint *p, int n, const BOOL clip)
{
	float scratch_x,scratch_y,scratch_z;

	// Make sure we've got enough room in the transformed position pool
	ShiAssert(IsValidPosIndex(n-1));

	while(n--)
	{
		scratch_z = Rotation.M11 * p->x + Rotation.M12 * p->y + Rotation.M13 * p->z + Xlation.x;
		scratch_x = Rotation.M21 * p->x + Rotation.M22 * p->y + Rotation.M23 * p->z + Xlation.y;
		scratch_y = Rotation.M31 * p->x + Rotation.M32 * p->y + Rotation.M33 * p->z + Xlation.z;

		if(clip)
		{
			ClipInfoPoolNext->clipFlag = ON_SCREEN;

			if(scratch_z < NEAR_CLIP_DISTANCE)
				ClipInfoPoolNext->clipFlag |= CLIP_NEAR;

			if(fabs(scratch_y) > scratch_z)
			{
				if(scratch_y > scratch_z)
					ClipInfoPoolNext->clipFlag |= CLIP_BOTTOM;
				else
					ClipInfoPoolNext->clipFlag |= CLIP_TOP;
			}

			if(fabs(scratch_x) > scratch_z)
			{
				if(scratch_x > scratch_z)
					ClipInfoPoolNext->clipFlag |= CLIP_RIGHT;
				else
					ClipInfoPoolNext->clipFlag |= CLIP_LEFT;
			}

			ClipInfoPoolNext->csX = scratch_x;
			ClipInfoPoolNext->csY = scratch_y;
			ClipInfoPoolNext->csZ = scratch_z;

			ClipInfoPoolNext++;
		}

		register float OneOverZ = 1.f/scratch_z;
		p++;

		XformedPosPoolNext->z = scratch_z;
		XformedPosPoolNext->x = XtoPixel(scratch_x * OneOverZ);
		XformedPosPoolNext->y = YtoPixel(scratch_y * OneOverZ);
		XformedPosPoolNext++;
	}
}

void StateStackClass::TransformNoClip(Ppoint *p, int n)
{
	TransformInline(p, n, FALSE);
}

void StateStackClass::TransformWithClip(Ppoint *p, int n)
{
	// TODO:  Need to make sure we're not going to walk off the end...
	TransformInline(p, n, TRUE);
}

void StateStackClass::TransformBillboardWithClip(Ppoint *p, int n, BTransformType type)
{
	float	scratch_x, scratch_y, scratch_z;
	Pmatrix	*T;

	// Make sure we've got enough room in the transformed position pool
	ShiAssert(IsValidPosIndex(n-1));

	if(type == Tree)
	{
		T = Tt;
	}
	else
	{
		T = Tb;
	}

	while(n--)
	{
		scratch_z = T->M11 * p->x + T->M12 * p->y + T->M13 * p->z + Xlation.x;
		scratch_x = T->M21 * p->x + T->M22 * p->y + T->M23 * p->z + Xlation.y;
		scratch_y = T->M31 * p->x + T->M32 * p->y + T->M33 * p->z + Xlation.z;

		ClipInfoPoolNext->clipFlag = ON_SCREEN;

		if(scratch_z < NEAR_CLIP_DISTANCE)
			ClipInfoPoolNext->clipFlag |= CLIP_NEAR;

		if(fabs(scratch_y) > scratch_z)
		{
			if(scratch_y > scratch_z)
				ClipInfoPoolNext->clipFlag |= CLIP_BOTTOM;
			else
				ClipInfoPoolNext->clipFlag |= CLIP_TOP;
		}

		if(fabs(scratch_x) > scratch_z)
		{
			if(scratch_x > scratch_z)
				ClipInfoPoolNext->clipFlag |= CLIP_RIGHT;
			else
				ClipInfoPoolNext->clipFlag |= CLIP_LEFT;
		}

		ClipInfoPoolNext->csX = scratch_x;
		ClipInfoPoolNext->csY = scratch_y;
		ClipInfoPoolNext->csZ = scratch_z;

		ClipInfoPoolNext++;

		register float OneOverZ = 1.f/scratch_z;
		p++;

		XformedPosPoolNext->z = scratch_z;
		XformedPosPoolNext->x = XtoPixel(scratch_x * OneOverZ);
		XformedPosPoolNext->y = YtoPixel(scratch_y * OneOverZ);
		XformedPosPoolNext++;
	}
}

BOOL StateStackClass::IsValidPosIndex(int i)
{
	return (i+XformedPosPool < XformedPosPoolBuffer+MAX_VERT_POOL_SIZE);
}

BOOL StateStackClass::IsValidIntensityIndex(int i)
{
	return (i+IntensityPool < IntensityPoolBuffer+MAX_VERT_POOL_SIZE);
}
