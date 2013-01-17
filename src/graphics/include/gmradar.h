/***************************************************************************\
    GMRadar.h
    Scott Randolph
    November 15, 1996

    This class provides the ground mapping radar terrain display.
\***************************************************************************/
#ifndef _GMRADAR_H_
#define _GMRADAR_H_

#include "Render3D.h"


static const float GM_NO_GIMBAL_LIMIT = -1.0f;


typedef struct GroundMapVertex: public MPRVtxClr_t {
/* MPRVtxClr_t provides:
	float	x, y;
	float	r, g, b, a;	

   Then I add:
*/
	DWORD	clipFlag;
} GroundMapVertex;


class RenderGMRadar : public Render3D {
  public:
	RenderGMRadar()				{ LOD = -1;  viewpoint = NULL; };
	virtual ~RenderGMRadar()	{};

	// Setup and Cleanup need to have additions here, but still call the parent versions
	void Setup( ImageBuffer *imageBuffer );
	void Cleanup( void );

	virtual void StartDraw( void );
	virtual void SetViewport( float l, float t, float r, float b );

	BOOL	SetRange( float newRange, int newLOD );
	void	SetGain( float newGain )				{ if(newGain <= 1000.0F && newGain >= 0.0F) gain = newGain; };

	float	GetRange( void )			{ return range; };
	int		GetLOD( void )				{ return LOD; };
	float	GetGain( void )				{ return gain; };

	void StartScene( Tpoint *from, Tpoint *at, float upHdg );
	void TransformScene( void );
	void DrawScene( void );
	void DrawFeatures( void );
	void PrepareToDrawTargets( void );
	void FlushDrawnTargets( void );
	void DrawBlip( float worldX, float worldY );
	void DrawBlip( class DrawableObject* drawable, float GainScale, bool Shaped);
	void FinishScene( void );

	// Recover the information passed into start frame
	Tpoint	*GetFrom( void )		{ return &cameraPos; };
	Tpoint	*GetAt( void )			{ return &centerPos; };
	float	GetHdg( void )			{ return -rotationAngle; };


  protected:
	void SetSubViewport( float l, float t, float r, float b );
	void ClearSubViewport( void );

	void ComputeLightAngles( Tpoint *from, Tpoint *at );
	float ComputeReflectedIntensity( Tpost *post );
	void DrawGMsquare( GroundMapVertex*, GroundMapVertex*, GroundMapVertex*, GroundMapVertex* );

  protected:
	BOOL		SkipDraw;			// Disables drawing the rest of a scene (reset in ComputeScene)

	Tpoint		cameraPos;			// Camera position in world space
	Tpoint		centerPos;			// Center of attention in world space (COA)

	class TViewPoint	*viewpoint;	// Our private viewpoint looking at our COA

	float		lightTheta;			// Asmuth of radar illumination
	float		lightPhi;			// Elevation of radar illumination

	int			LOD;				// Current terrain LOD being used
	float		range;				// Current ground patch radius requested
	float		gain;				// Multiplier for luminance in display

	int			boxCenterRow;		// Level post location of the COA
	int			boxCenterCol;		// Level post location of the COA
	int			boxSize;			// Level post size of drawing candidate box (COA to edge)

	float		prevLeft;			// Previous values for the viewport before we
	float		prevRight;			// changed it.
	float		prevTop;			//
	float		prevBottom;			//

	float		rotationAngle;		// Angle to rotate image (positive rotates clockwise)
	float		vOffset;			// Vertical offset of COA in unit screen space
	float		hOffset;			// Horizontal offset of COA in unit screen space

	float		dCtrX;				// These terms pre-scale and shift the unit screen
	float		dCtrY;				// space coordinates we submit for drawing to
	float		dScaleX;			// correct for any shrinkage of the viewport caused
	float		dScaleY;			// by intersection with the edge of the drawing area.

	float		worldToUnitScale;	// Scale from world to -1.0 to 1.0 space

	float		ScaledCOS;			// cos of rotationAngle times worldToUnitScale
	float		ScaledSIN;			// sin of rotationAngle times worldToUnitScale

	int					drawRadius;	// How many posts out from center to traverse
	GroundMapVertex		*xformBuff;	// Transformed vertex data buffer
};


#endif // _GMRADAR_H_
