/***************************************************************************\
    RenderWire.h
    Scott Randolph
    May 5, 1998

    This sub class draws an out the window view in wire frame mode.
\***************************************************************************/
#include "TOD.h"
#include "Tpost.h"
#include "RViewPnt.h"
#include "RenderWire.h"


/***************************************************************************\
	Setup the rendering context for this view
\***************************************************************************/
void RenderWire::Setup( ImageBuffer *imageBuffer, RViewPoint *vp )
{
	// Call our base classes setup routine
	RenderOTW::Setup( imageBuffer, vp );
	
	// Set our drawing properties
//	SetSmoothShadingMode( FALSE );
	SetObjectTextureState( TRUE );	// Start with object textures ON
	SetHazeMode( TRUE );			// Start with hazing turned ON

	// Use a black terrain filler
	haze_ground_color.r	= haze_ground_color.g	= haze_ground_color.b	= 0.0f;
	earth_end_color.r	= earth_end_color.g		= earth_end_color.b		= 0.0f;

	// Use a midnight blue sky
	sky_color.r			= 0.0f;
	sky_color.g			= 0.0f;
	sky_color.b			= 0.125f;
	haze_sky_color.r	= 0.0f;
	haze_sky_color.g	= 0.0f;
	haze_sky_color.b	= 0.25f;

	// Use only moderate ambient lighting on the objects
	lightAmbient = 0.8f;
	lightDiffuse = 0.0f;

	// Update our colors to account for our changes to the default settings
	TimeUpdateCallback( this );
}



/***************************************************************************\
	Do end of frame housekeeping
\***************************************************************************/
void RenderWire::StartFrame( void ) {
	RenderOTW::StartDraw();

	TheColorBank.SetColorMode( ColorBankClass::UnlitMode );
}



#define BLEND_MIN	0.25f
#define BLEND_MAX	0.95f
/***************************************************************************\
    Compute the color and texture blend value for a single terrain vertex.
\***************************************************************************/
//void RenderWire::ComputeVertexColor( TerrainVertex *vert, Tpost *post, float distance )
void RenderWire::ComputeVertexColor( TerrainVertex *vert, Tpost *, float distance )
{
//	float	alpha;

	// Set all verts to black
	vert->r = 0.0f;
	vert->g = 0.0f;
	vert->b = 0.0f;

	// Blend out the textures in the distance
/*	if ( hazed && (distance > blend_start)) {
		alpha = (distance - blend_start) / blend_depth;
		if (alpha < BLEND_MIN)	alpha = BLEND_MIN;
		if (alpha > BLEND_MAX)	alpha = BLEND_MAX;

		vert->a = 1.0f - alpha;
		vert->RenderingStateHandle = state_mid;

	} else {
		vert->a = BLEND_MAX;

		if (distance < PERSPECTIVE_RANGE) {
			vert->RenderingStateHandle = state_fore;
		} else {
			vert->RenderingStateHandle = state_near;
		}
	}*/
}
