/***************************************************************************\
    Render3D.h
    Scott Randolph
    January 2, 1996

    This class provides 3D drawing functions specific to rendering out the
	window views including terrain.
\***************************************************************************/
#ifndef _RENDER3D_H_
#define _RENDER3D_H_

#include "StateStack.h"
#include "Render2D.h"


// Possible values for CullFlag in DrawSquare call
#define CULL_ALLOW_ALL	0
#define CULL_ALLOW_CW	1
#define CULL_ALLOW_CCW	2

static const float	NEAR_CLIP = 1.0f;
//static const float Q_SCALE = 0.001f;	// Use to keep Q in 16.16 range for MPR
// COBRA - DX - fits more this value
static const float Q_SCALE = 0.0008f;	// Use to keep Q in 16.16 range for MPR


typedef struct ThreeDVertex: public TwoDVertex {
/* TwoDVertex provides:
	float	x, y;
	float	r, g, b, a;	
	float	u, v, q; 
	DWORD	clipFlag;

   Then I add:
*/
	float	csX, csY, csZ;		// Camera space coordinates
} ThreeDVertex;


class Render3D : public Render2D {
  public:
	Render3D()	{};
	virtual ~Render3D()	{};

	Trotation	Tbb;				// Transformation matrix for billboards

	// Setup and Cleanup need to have additions here, but still call the parent versions
	virtual void Setup( ImageBuffer *imageBuffer );
	virtual void Cleanup( void )	{ Render2D::Cleanup(); };

	// Overload this function to get extra work done at start frame
	virtual void StartDraw( void );
	
	// Set the camera parameters we're to use for rendering
	void SetObjectDetail( float scaler );
	void SetFOV( float horizontal_fov, float NearZ=0.2f );
	void SetFar( float distance )	{ far_clip = distance; };
	void SetCamera( const Tpoint* pos, const Trotation* rot );

	float GetObjectDetail( void )	{ return detailScaler; };
	float GetFOV( void )			{ return horizontal_half_angle * 2.0f; };
	float GetVFOV( void )			{ return vertical_half_angle * 2.0f; };
	float GetDFOV( void )			{ return diagonal_half_angle * 2.0f; };
	float GetFar( void )			{ return far_clip; }; //JAM 09Dec03


    virtual void SetViewport( float leftSide, float topSide, float rightSide, float bottomSide );

	// Setup the 3D object lighting
	void SetLightDirection( const Tpoint* dir );
 	void GetLightDirection( Tpoint* dir );

	// Turn 3D object texturing on/off
	void SetObjectTextureState( BOOL state );
	BOOL GetObjectTextureState( void )			{ return objTextureState; };

	// Get the location and orientation of the camera for this renderer
	float	X( void )	{ return cameraPos.x; };
	float	Y( void )	{ return cameraPos.y; };
	float	Z( void )	{ return cameraPos.z; };

	float	Yaw( void )		{ return yaw;	};
	float	Pitch( void )	{ return pitch;	};
	float   Roll( void )	{ return roll;	};

	void	GetAt( Tpoint *v )		{ v->x = cameraRot.M11, v->y = cameraRot.M12, v->z = cameraRot.M13; };
	void	GetLeft( Tpoint *v )	{ v->x = cameraRot.M21, v->y = cameraRot.M22, v->z = cameraRot.M23; };
	void	GetUp( Tpoint *v )		{ v->x = cameraRot.M31, v->y = cameraRot.M32, v->z = cameraRot.M33; };


	// Transform the given worldspace point into pixel coordinates using the current camera
	void	TransformPoint( Tpoint* world, ThreeDVertex* pixel );
	void	TransformPointToView( Tpoint* world, Tpoint *result );
	void	TransformPointToViewSwapped(Tpoint *world,Tpoint *result); //JAM 03Dec03
	void	TransformBillboardPoint( Tpoint* world, Tpoint *viewOffset, ThreeDVertex* pixel );
	void	TransformTreePoint( Tpoint* world, Tpoint *viewOffset, ThreeDVertex* pixel );
	void	UnTransformPoint( Tpoint* pixel, Tpoint* vector );
	void	TransformCameraCentricPoint( Tpoint* world, ThreeDVertex* pixel );
	float	ZDistanceFromCamera( Tpoint* p );


	// Draw flat shaded geometric primitives in world space using the current camera parameters
	void Render3DPoint( Tpoint* p1 );
	void Render3DLine( Tpoint* p1, Tpoint* p2 );
	void Render3DFlatTri( Tpoint* p1, Tpoint* p2, Tpoint* p3 );
	void Render3DObject( GLint id, Tpoint* pos, const Trotation* orientation );

	// Draw a full featured square or tri given the already transformed (but not clipped) verts
	//JAM 14Sep03
	void DrawSquare( ThreeDVertex* v0, ThreeDVertex* v1, ThreeDVertex* v2, ThreeDVertex* v3, int CullFlag, bool gifPicture=false, bool terrain=false);
	void DrawTriangle( ThreeDVertex* v0, ThreeDVertex* v1, ThreeDVertex* v2, int CullFlag, bool gifPicture=false, bool terrain=false);
/*	void DrawSquare( ThreeDVertex* v0, ThreeDVertex* v1, ThreeDVertex* v2, ThreeDVertex* v3, int CullFlag, bool gifPicture = false );
	void DrawTriangle( ThreeDVertex* v0, ThreeDVertex* v1, ThreeDVertex* v2, int CullFlag, bool gifPicture = false);*/
	//JAM

  protected:
	// Draw a fan which is known to require clipping
	//JAM 14Sep03
	void ClipAndDraw3DFan( ThreeDVertex** vertPointers, unsigned count, int CullFlag, bool gifPicture = false, bool terrain = false, bool sort=false);
//	void ClipAndDraw3DFan( ThreeDVertex** vertPointers, unsigned count, int CullFlag, bool gifPicture = false );
	//JAM

  private:
	void IntersectNear(   ThreeDVertex *v1, ThreeDVertex *v2, ThreeDVertex *v );
	void IntersectTop(    ThreeDVertex *v1, ThreeDVertex *v2, ThreeDVertex *v );
	void IntersectBottom( ThreeDVertex *v1, ThreeDVertex *v2, ThreeDVertex *v );
	void IntersectLeft(   ThreeDVertex *v1, ThreeDVertex *v2, ThreeDVertex *v );
	void IntersectRight(  ThreeDVertex *v1, ThreeDVertex *v2, ThreeDVertex *v );

  protected:
	float	far_clip;
	float	detailScaler;
	float	resRelativeScaler;
	BOOL	objTextureState;

	float	horizontal_half_angle;
	float	vertical_half_angle;
	float	diagonal_half_angle;

	float	oneOVERtanHFOV;
	float	oneOVERtanVFOV;

	float	yaw;
	float	pitch;
	float	roll;

	Tpoint		cameraPos;			// Camera position in world space
	Trotation	cameraRot;			// Camera orientation matrix

	float		lightAmbient;
	float		lightDiffuse;
	float		lightSpecular;
	Tpoint		lightVector;

	Tpoint		move;				// Camera space translation required to position visible objects
	Trotation	T;					// Transformation matrix including aspect ratio and FOV effects
	//Trotation	Tbb;				// Transformation matrix for billboards
	Trotation	Tt;					// Transformation matrix for trees
};


/* Helper functions to compute the horizontal, vertical, and near clip flags */
inline DWORD GetRangeClipFlags( float z, float )
{
	if ( z < NEAR_CLIP ) {
		return CLIP_NEAR;
	}
//	if ( z > far_clip ) {
//		return CLIP_FAR;
//	}
	return ON_SCREEN;
}

inline DWORD GetHorizontalClipFlags( float x, float z )
{
	if ( fabs(x) > z ) {
		if ( x > z ) {
			return CLIP_RIGHT;
		} else {
			return CLIP_LEFT;
		}
	}
	return ON_SCREEN;
}

inline DWORD GetVerticalClipFlags( float y, float z )
{
	if ( fabs(y) > z ) {
		if ( y > z ) {
			return CLIP_BOTTOM;
		} else {
			return CLIP_TOP;
		}
	}
	return ON_SCREEN;
}


#endif // _RENDER3D_H_
