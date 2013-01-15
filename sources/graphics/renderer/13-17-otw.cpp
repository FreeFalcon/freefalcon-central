/***************************************************************************\
    OTW.cpp
    Scott Randolph
    January 2, 1995

    This class provides 3D drawing functions specific to rendering out the
	window views including terrain.

	This file provides the Setup and Cleanup code for the RenderOTW class,
	as well as miscellanious interface functions.
\***************************************************************************/
//JAM 297Sep03 - Begin Major Rewrite
#include <math.h>
#include "falclib\include\debuggr.h"
#include "TimeMgr.h"
#include "TOD.h"
#include "TMap.h"
#include "Tpost.h"
#include "Draw2D.h"
#include "DrawOVC.h"
#include "ColorBank.h"
#include "Device.h"
#include "RViewPnt.h"
#include "RenderOW.h"
#include "falclib\include\fakerand.h"
#include "falclib\include\mltrig.h"
#include "FalcLib\include\playerop.h"
#include "FalcLib\include\dispopts.h"

//JAM 18Nov03
#include "RealWeather.h"

extern float SimLibMajorFrameTime;

#pragma warning(disable : 4127)

const float RenderOTW::PERSPECTIVE_RANGE	= 6000.0f;
const float RenderOTW::NVG_TUNNEL_PERCENT	= 0.2f;
const float RenderOTW::DETAIL_RANGE			= 8000.f;

extern bool g_bEnableWeatherExtensions;
extern float g_fCloudThicknessFactor;
extern bool g_bFullScreenNVG;

extern unsigned long    vuxRealTime;

#ifdef TWO_D_MAP_AVAILABLE
BOOL	twoDmode = FALSE;		// Use to control map display mode while debugging
int		TWODSCALE = 5;
#endif

#ifdef DAVE_DBG
#include "sim\include\simdrive.h"
#endif

#ifdef CHECK_PROC_TIMES
ulong objTime = 0;
ulong terrTime = 0;

#endif


/***************************************************************************\
	Setup the rendering context for thiw view
\***************************************************************************/
void RenderOTW::Setup( ImageBuffer *imageBuffer, RViewPoint *vp )
{
	int				usedLODcount;
	int				LOD;
	int				LODbufferSize;


	// Call our parents Setup code (win is available only after this call)
	Render3D::Setup( imageBuffer );
	
	// Retain a pointer to the TViewPoint we are to use
	viewpoint = vp;

	// Start with the default light source position (over head)
	lightTheta		= 0.0f;
	lightPhi		= PI_OVER_2;
	SunGlareValue	= 0.0f;
	SunGlareWashout	= 0.0f;
//	smoothed = 0; // intialise to something
	textureLevel = 4; //JAM 01Dec03 - Hardcoding this for now, to be removed.
	filtering = hazed = 0;

	// Start with no tunnel vision or cloud effects
	SetTunnelPercent( 0.0f, 0x80808080 );
	PreSceneCloudOcclusion( 0.0f, 0x80808080 );
		
	// Adjust our back clipping plane based on the range defined for this viewpoint
	SetFar( viewpoint->GetMaxRange() * 0.707f );	// far = maxRange * cos(half_angle)

	// Set the default sky and haze properties
	SetDitheringMode( TRUE );
	SetTerrainTextureLevel( TheMap.LastNearTexLOD() );
	SetObjectTextureState( TRUE );	// Default to using textured objects
	SetFilteringMode( FALSE );
	SetHazeMode( TRUE );			// (TRUE = ON, FALSE = OFF)
	SetRoofMode( FALSE );
//	SetSmoothShadingMode( TRUE ); // JPO - this uses the value from previous state

    // Default to full alpha blending for special effects
//    alphaMode = TRUE;

	// Setup a default sky (will be replaced by Time of Day processing)
	sky_color.r			= sky_color.g			= sky_color.b			= 0.5f;
	haze_sky_color.r	= haze_sky_color.g		= haze_sky_color.b		= 0.4f;
	haze_ground_color.r	= haze_ground_color.g	= haze_ground_color.b	= 0.3f;
	earth_end_color.r	= earth_end_color.g		= earth_end_color.b		= 0.2f;


	// Figure out how many posts outward we might ever see at any detail level
	maxSpanOffset = 0;
	for (LOD = viewpoint->GetMinLOD(); LOD <= viewpoint->GetMaxLOD(); LOD++) {
		maxSpanOffset = max( maxSpanOffset, viewpoint->GetMaxPostRange( LOD ) );
	}
	maxSpanExtent = maxSpanOffset*2 + 1;

	// Figure the total number of rings to deal with assuming max number of posts
	// across each of the detail levels.  Include room for a few extra padding rings
	usedLODcount		= viewpoint->GetMaxLOD() - viewpoint->GetMinLOD() + 1;
	spanListMaxEntries	= (maxSpanOffset+1) * usedLODcount + 2;


	// Allocate memory for our list of vertex spans
	spanList = new SpanListEntry[ spanListMaxEntries ];
	firstEmptySpan = spanList;
	if (!spanList) {
		ShiError( "Failed to allocate span buffer" );
	}
	memset (spanList, 0, sizeof(*spanList) * spanListMaxEntries); // JPO zero out

	// Allocate memory for the the transformed vertex buffers we need
	LODbufferSize = (maxSpanExtent) * (maxSpanExtent);
	vertexMemory = new TerrainVertex[ usedLODcount * LODbufferSize ];
	if (!vertexMemory) {
		ShiError("Failed to allocate transformed vertex buffer");
	}
	memset(vertexMemory, 0, sizeof(*vertexMemory) * usedLODcount * LODbufferSize); // JPO start with 0

	// Allocate memory for the array of transformed vertex buffer pointers
	vertexBuffer = new TerrainVertex*[ (viewpoint->GetMaxLOD()+1) ];
	if (!vertexBuffer) {
		ShiError("Failed to allocate transformed vertex buffer list");
	}


	// Setup the + and - indexable vertex pointers (they point into the vertexMemory)
	for ( LOD = 0; LOD <= viewpoint->GetMaxLOD(); LOD++ ) {
		if ( LOD < viewpoint->GetMinLOD() ) {
			vertexBuffer[LOD] = NULL;
		} else {
			vertexBuffer[LOD] = vertexMemory + 
								(LOD-viewpoint->GetMinLOD()) * LODbufferSize +
								maxSpanExtent*maxSpanOffset + maxSpanOffset;
		}
	}


	// Allocate memory for the array of information stored for each LOD (viewer location & vectors)
	LODdata = new LODdataBlock[ (viewpoint->GetMaxLOD()+1) ];	
	if ( !LODdata ) {
		ShiError( "Failed to allocate memory for LOD step vector array" );
	}

	brightness = 1.0; // JB 010618 vary the brightness on cloud thickness
	visibility = 1.0;
	rainFactor = 0;
	snowFactor = 0;
	thunderAndLightning = false;
	thundertimer = 0;
	thunder = false;
	Lightning = false;
	lightningtimer = 0.0F;

	// Initialize the lighting conditions and register for future time of day updates
	TimeUpdateCallback( this );
	TheTimeManager.RegisterTimeUpdateCB( TimeUpdateCallback, this );

	//JAM 26Dec03
	if(DisplayOptions.bZBuffering)
		realWeather->Setup();
	else
		realWeather->Setup(viewpoint->ObjectsBelowClouds(),viewpoint->Clouds());

	realWeather->SetRenderer(this);
}

 

/***************************************************************************\
    Shutdown the renderer.
\***************************************************************************/
void RenderOTW::Cleanup( void )
{
	// Stop receiving time updates
	TheTimeManager.ReleaseTimeUpdateCB( TimeUpdateCallback, this );

	// Release the memory used to hold the LOD data blocks
	ShiAssert( LODdata );
	delete[] LODdata;
	LODdata = NULL;
	
	// Release the memory for the array of transformed vertex buffer pointers
	ShiAssert( vertexBuffer );
	delete[] vertexBuffer;
	vertexBuffer = NULL;

	// Release the memory for the list of vertex spans
	ShiAssert( spanList );
	delete[] spanList;
	spanList = NULL;

	// Release the memory for the the transformed vertex buffers
	ShiAssert( vertexMemory );
	delete[] vertexMemory;
	vertexMemory = NULL;
	
	// Set our allowable ranges to 0
	maxSpanOffset		= 0;
	maxSpanExtent		= 0;
	spanListMaxEntries	= 0;

	// Discard the pointer to our associated TViewPoint object
	viewpoint = NULL;

	// Call our parent's cleanup
	Render3D::Cleanup();
}



/***************************************************************************\
	Do start of frame housekeeping
\***************************************************************************/
void RenderOTW::StartFrame(void)
{
	Render3D::StartFrame();

	if( TheTimeOfDay.GetNVGmode() )
	{
		Drawable2D::SetGreenMode(TRUE);
//		DrawableOvercast::SetGreenMode(TRUE);
		realWeather->SetGreenMode(TRUE); //JAM 10Nov03
		TheColorBank.SetColorMode(ColorBankClass::GreenMode);
	}
}


/***************************************************************************\
	Select the amount of textureing employed for terrain
\***************************************************************************/
void RenderOTW::SetTerrainTextureLevel( int level )
{
	// Limit the texture level to legal values
	textureLevel = max( level, viewpoint->GetMinLOD()-1 );
	textureLevel = min( textureLevel, viewpoint->GetMaxLOD() );
	textureLevel = min( textureLevel, TheMap.LastFarTexLOD() );
	
	// Rearrange the fog settings
	//JAM 13Nov03
	if(realWeather->weatherCondition == INCLEMENT)
	{
		haze_start = far_clip*.05f;
		haze_depth = far_clip*.3f;
	}
	else if(realWeather->weatherCondition == POOR)
	{
		haze_start = far_clip*.1f;
		haze_depth = far_clip*.6f;
	}
	else
	{
		haze_start = far_clip*.1f;
		haze_depth = far_clip*.6f;
	}

	// Rearrange the texture blend settings
//	blend_start	= viewpoint->GetMaxRange( textureLevel - 1 );
//	blend_depth	= viewpoint->GetMaxRange( textureLevel ) * 0.8f;

	// Convert from range from viewer to range from start
	haze_depth	-= haze_start;

	// Reevaluate which rendering states to use for each LOD
	SetupStates();
}


void RenderOTW::SetupStates(void)
{
	state_far = STATE_ALPHA_GOURAUD;

	if(textureLevel == 0)
		state_mid  = state_far;
	else
		state_mid = STATE_TEXTURE_TERRAIN;
}

/***************************************************************************\
	These are used to construct the tunnel vision effect
\***************************************************************************/
static const float	big = 1.2f;
static const float	ltl = 0.5f;
static const struct pnt { float x, y; } OutsidePoints[] = {
	{	 ltl,	 big	},
	{	 1.0f,	 1.0f	},
	{	 big,	 ltl	},
	{	 big,	-ltl	},
	{	 1.0f,	-1.0f	},
	{	 ltl,	-big	},
	{	-ltl,	-big	},
	{	-1.0f,	-1.0f	},
	{	-big,	-ltl	},
	{	-big,	 ltl	},
	{	-1.0f,	 1.0f	},
	{	-ltl,	 big	},
	{	 ltl,	 big	}
};
static const int	NumPoints = sizeof(OutsidePoints)/sizeof(struct pnt);
static const float	PercentBlend = 0.1f;
static const float	PercentScale = 1.0f + PercentBlend;


/***************************************************************************\
	Setup the tunnel vision effect.  This should only be called
	on the primary out the window view.  The percent supplied is
	is relative to the total size of the underlying image object.
	THIS CALL IS TO BE CALLED _BEFORE_ DrawScene()
\***************************************************************************/
void RenderOTW::SetTunnelPercent( float percent, DWORD color )
{
	float startpct = percent;
	// Clamp the percent value to the allowable range
	if (percent > 1.0f)			percent = 1.0f;
	else if (percent < 0.0f)	percent = 0.0f;

	// Apply adjustments if we're in NVG mode
	if (TheTimeOfDay.GetNVGmode() && (percent < NVG_TUNNEL_PERCENT))
	{
		percent = NVG_TUNNEL_PERCENT;

		if (percent <= 0.0f) {
			color = 0;
		} else {
			float t = percent/NVG_TUNNEL_PERCENT;
			color = (FloatToInt32((color & 0x00FF0000) * t) & 0x000000FF) |
					(FloatToInt32((color & 0x00FF0000) * t) & 0x0000FF00) |
					(FloatToInt32((color & 0x00FF0000) * t) & 0x00FF0000);
		}
		if (g_bFullScreenNVG)
			percent = startpct;
	}

	// Store the new tunnel vision parameters
	tunnelColor			= color;
	tunnelPercent		= percent;
	tunnelAlphaWidth	= percent*PercentScale;
	tunnelSolidWidth	= tunnelAlphaWidth - PercentBlend;
}


/***************************************************************************\
	Setup the cloud "fuzz" effect as we enter/leave the clouds
	CALL _BEFORE_ DrawScene() gets underway.
\***************************************************************************/
void RenderOTW::PreSceneCloudOcclusion( float percent, DWORD color )
{
	// If we're in software, we turn on the special color munging function
	if ( image->GetDisplayDevice()->IsHardware() ) {

		// Save the cloud color with alpha for post processing use.
		cloudColor = (color & 0x00FFFFFF) | (FloatToInt32(percent * 255.9f) << 24);

	} else {

		// Set the MPR color correction terms to get "fade out"
		// TODO:  Update this once Marc's changes are in...
#if 1
//		context.SetColorCorrection( color, percent );
#else
		// This is a total hack to get "green" results on MFDs:
		float correction = 1.0f + percent * percent * 100.0f;

		// Red
		if (((color & 0x000000FF) == 0x0 ) &&
			((color & 0x0000FF00) >= 0x10) &&
			((color & 0x00FF0000) == 0x0 )) {

			// Guess that we're on a green display
//			context.SetState( MPR_STA_GAMMA_RED,   (DWORD)(1.0f) );
//			context.SetState( MPR_STA_GAMMA_GREEN, (DWORD)(correction) );
//			context.SetState( MPR_STA_GAMMA_BLUE,  (DWORD)(1.0f) );
		} else {
			// Assume a color display
//			context.SetState( MPR_STA_GAMMA_RED,   (DWORD)(correction) );
//			context.SetState( MPR_STA_GAMMA_GREEN, (DWORD)(correction) );
//			context.SetState( MPR_STA_GAMMA_BLUE,  (DWORD)(correction) );
		}
#endif
	}
}


/***************************************************************************\
	Finish the cloud "fuzz" effect as we enter/leave the clouds
	CALL between DrawScene() and FinishFrame()
\***************************************************************************/
void RenderOTW::PostSceneCloudOcclusion( void )
{
	// If we're in software, we turn off the special color munging function
	if ( image->GetDisplayDevice()->IsHardware() ) {

		// Drop out if alpha is 0
		if ((cloudColor & 0xFF000000) == 0) {
			return;
		}

// OW
#if 1
		// Draw the viewport sized alpha blended polygon
		MPRVtx_t	pVtx[4];

		// Set the foreground color and drawing state
		context.SelectForegroundColor( cloudColor );
		context.RestoreState( STATE_ALPHA_SOLID );

		// Now intialize the four corners of the rectangle to fill
		pVtx[0].x = leftPixel;	pVtx[0].y = bottomPixel;
		pVtx[1].x = leftPixel;	pVtx[1].y = topPixel;
		pVtx[2].x = rightPixel;	pVtx[2].y = topPixel;
		pVtx[3].x = rightPixel;	pVtx[3].y = bottomPixel;

		context.DrawPrimitive( MPR_PRM_TRIFAN, 0, 4, pVtx, sizeof(pVtx[0]));
#else
		// Draw the viewport sized alpha blended polygon
		MPRVtx_t	*p;

		// Set the foreground color and drawing state
		context.SelectForegroundColor( cloudColor );
		context.RestoreState( STATE_ALPHA_SOLID );

		// Start the primitive and get a pointer to the target vertex data
		context.Primitive( MPR_PRM_TRIFAN, 0, 4, sizeof(*p) );
		p = (MPRVtx_t*)context.GetContextBufferPtr();

		// Now intialize the four corners of the rectangle to fill
		p->x = leftPixel;	p->y = bottomPixel;		p++;
		p->x = leftPixel;	p->y = topPixel;		p++;
		p->x = rightPixel;	p->y = topPixel;		p++;
		p->x = rightPixel;	p->y = bottomPixel;		p++;

		// Finish off the primitive and send it (since it'll be slow)
		context.SetContextBufferPtr( (BYTE*)p );
		context.SendCurrentPacket();
#endif

	} else {

		// Set the color correction terms back to normal
		// TODO:  Update this once Marc's changes are in...
//		context.SetState( MPR_STA_GAMMA_RED,   (DWORD)(1.0f) );
//		context.SetState( MPR_STA_GAMMA_GREEN, (DWORD)(1.0f) );
//		context.SetState( MPR_STA_GAMMA_BLUE,  (DWORD)(1.0f) );

	}
}


/***************************************************************************\
	When the tunnel vision effect is in use, this call draws the colored
	screen border which represents the portion of the players vision which
	has been lost.
	THIS CALL IS TO BE CALLED _AFTER_ FinishFrame()
	It is illegal to call this function without first calling SetTunnelPercent()
\***************************************************************************/
void RenderOTW::DrawTunnelBorder( void )
{
	TwoDVertex	vert[NumPoints*2+1];
	TwoDVertex* vertPointers[4];
	int			i;
	int			j1, j2;
	float		x, y;
	float		alpha;

	// Quit now if the tunnel isn't being drawn
	if (tunnelAlphaWidth <= 0.0f)
		return;

	// OW 
	ZeroMemory(vert, sizeof(vert));

	// Restart the rasterizer to draw the tunnel border
	context.StartFrame();

	// Put the clip rectangle at full size to allow drawing of the border
	SetViewport( -1.0f, 1.0f, 1.0f, -1.0f );

	// Initialize all the verticies
	float r = (float)((tunnelColor)     & 0xFF) / 255.9f;
	float g = (float)((tunnelColor>>8)  & 0xFF) / 255.9f;
	float b = (float)((tunnelColor>>16) & 0xFF) / 255.9f;

	for (i = 0; i<NumPoints; i++)
	{
		j1 = i << 1;
		j2 = j1 + 1;

		// Color
		vert[j1].r = vert[j2].r = r;
		vert[j1].g = vert[j2].g = g;
		vert[j1].b = vert[j2].b = b;
		vert[j1].a = vert[j2].a = 1.0f;

		// Root location
		vert[j1].x = viewportXtoPixel( OutsidePoints[i].x );
		vert[j1].y = viewportYtoPixel( OutsidePoints[i].y );
		SetClipFlags( &vert[j1] );

		// Inside edge
		x = (1.0f - tunnelSolidWidth) * OutsidePoints[i].x;
		y = (1.0f - tunnelSolidWidth) * OutsidePoints[i].y;
		vert[j2].x = viewportXtoPixel( x );
		vert[j2].y = viewportYtoPixel( y );
		SetClipFlags( &vert[j2] );
	}
	
	// Special pickup for the one vertex which wasn't colored this time, but will be used next
	vert[i*2].r = r;
	vert[i*2].g = g;
	vert[i*2].b = b;

	// Draw the flat colored mesh if it is visible
	if(tunnelSolidWidth > 0.0f)
	{
		context.RestoreState( STATE_SOLID );

		for(i=NumPoints-2; i>=0; i--)
		{
			j1 = i<<1;
			vertPointers[0] = &vert[j1];
			vertPointers[1] = &vert[j1+2];
			vertPointers[2] = &vert[j1+3];
			vertPointers[3] = &vert[j1+1];
			ClipAndDraw2DFan(vertPointers, 4);
		}
	}

	// Update the ending alpha value and alpha percent for the closing out the view case
	if (tunnelAlphaWidth > 1.0f)
	{
		alpha = (tunnelAlphaWidth - 1.0f) / PercentBlend;
		tunnelAlphaWidth = 1.0f;
	}

	else
		alpha = 0.0f;

	// Fill the blended portion of the border
	// NOTE:  The inside of the solid mesh is the outside of the blending mesh
	// therefore, the odd numbered vertices from above can be reused

	for (i = 0; i<NumPoints; i++)
	{
		j1 = (i << 1)+1;		// Index of vertex to be reused (last times inside edge)
		j2 = j1 + 1;			// Index of vertex to replace (last times outside edge)

		// Alpha
		vert[j2].a = alpha;

		// Inside edge
		x = (1.0f - tunnelAlphaWidth) * OutsidePoints[i].x;
		y = (1.0f - tunnelAlphaWidth) * OutsidePoints[i].y;
		vert[j2].x = viewportXtoPixel(x);
		vert[j2].y = viewportYtoPixel(y);
		SetClipFlags(&vert[j2]);
	}

	// Draw the blended mesh
	context.RestoreState(STATE_ALPHA_GOURAUD);

	for(i=NumPoints-2; i>=0; i--)
	{
		j1 = (i<<1) + 1;
		vertPointers[0] = &vert[j1];
		vertPointers[1] = &vert[j1+2];
		vertPointers[2] = &vert[j1+3];
		vertPointers[3] = &vert[j1+1];
		ClipAndDraw2DFan( vertPointers, 4 );
	}

	// Close down the renderer and flush the queue
	context.FinishFrame(NULL);
}


/***************************************************************************\
    Draw the out the window view, including terrain and all registered objects.
\***************************************************************************/
void RenderOTW::DrawScene( const Tpoint *offset, const Trotation *orientation )
{
	Tpoint	position={0.0F};
	int		containingList=0;
	float	prevFOV = 0.0F, prevLeft = 0.0F, prevRight = 0.0F, prevTop = 0.0F, prevBottom = 0.0F;

	prevFOV = GetFOV();
	GetViewport( &prevLeft, &prevTop, &prevRight, &prevBottom );

	// Reduce the viewport size to save on overdraw costs if there's a tunnel in effect
	if (tunnelSolidWidth > 0.0f)
	{		
		float	visible = (1.0f - tunnelSolidWidth) * big;		

		if (visible <= 1.0f)
		{
			float	left, top, right, bottom;
			float	fov;

			right	= min(  visible, prevRight  );
			left	= max( -visible, prevLeft   );
			top		= min(  visible, prevTop    );
			bottom	= max( -visible, prevBottom );
			fov = 2.0f * (float)atan( right / oneOVERtanHFOV );

			SetFOV( fov );
			SetViewport( left, top, right, bottom );
		}
	}


	// Get our world space position from our viewpoint
	viewpoint->GetPos( &position );

	// Apply the offset (if provided) -- in world space for now (TODO: camera space?)
	if (offset) {
		position.x += offset->x;
		position.y += offset->y;
		position.z += offset->z;
	}

	// Call Render3D's cammera update function with the new position
	SetCamera( &position, orientation );


	// Update the sky color based on our current attitude and position
	AdjustSkyColor();

//	float opacity = viewpoint->CloudOpacity();
/*	if (g_bEnableWeatherExtensions) 
	{
	  if (position.z > viewpoint->GetLocalCloudTops()) // are we below clouds? -ve z's
		{
			float tvis = viewpoint->GetVisibility();
			visibility = 0.95f * visibility + 0.05f * tvis; // phase in new vis
			float train = viewpoint->GetRainFactor();
			rainFactor = 0.95 * rainFactor + 0.05f * train; // phase in rain
			float temp = RealWeather->TemperatureAt(&position);
			static const float TEMP_RANGE = 10;
			static const float TEMP_MIN = TEMP_RANGE / 2.0f;
			
			if (temp < - TEMP_MIN) // all rain turns to snow
			{
				snowFactor = rainFactor;
				rainFactor = 0;
			}
			else if (temp < TEMP_MIN) // maybe do snow
			{
				temp += TEMP_MIN;
				snowFactor = rainFactor * (TEMP_RANGE - temp)/TEMP_RANGE;
				rainFactor = rainFactor * (temp / TEMP_RANGE);
			}
			else 
				snowFactor = 0;
			
			if (viewpoint->GetLightning())
			{
				thunderAndLightning = true;
				opacity = max(opacity, 0.97); // a flash of lightning.
				thundertimer = vuxRealTime + 10000.0f*PRANDFloatPos(); // in 10 seconds time
			}
			else 
				thunderAndLightning = false;
		}
	  else
		{
			visibility = 1;
			rainFactor = 0;
			snowFactor = 0;
    }

    if (thundertimer > 0 && thundertimer < vuxRealTime)
		{
			thunder = true;
			thundertimer = 0;
    }
    else
			thunder = false;
	}
*/
/*	if (visibility < 1) // less than perfect.
	    opacity = max(opacity, 1.0 - visibility);

	// Handle the entering/inside/leaving cloud effects
	if (opacity <= 0.0f && !Lightning) 
	{
*/		// We're not being affected by a cloud, the only effect is sun glare (if any)
		PreSceneCloudOcclusion( SunGlareWashout, 0xFFFFFFFF );
/*	} 
	else
	{
		// We're being affected by a cloud.
		Tcolor		color;
		DWORD		c;
		float		scaler;
		float		blend;

		if (thunderAndLightning) // Lightning for nuke
		{
		    color = TheTimeOfDay.GetLightningColor();
		}
		else if (Lightning)
		{
			color.r = 1.0F;
			color.b = 1.0F;
			color.g = 1.0F;
			lightningtimer += SimLibMajorFrameTime;
			if (lightningtimer > 5.0F)	// 5 seconds full white
				opacity = (0.97F - ((lightningtimer - 6) * 0.068F));
			else
				opacity = 0.97F;
			if (lightningtimer >= 20.0F)
			{
				lightningtimer = 0.0F;
				Lightning = false;
			}
		}
		else
		{
	    // Get the cloud properties
	    color	= viewpoint->CloudColor();
		}

		// Factor in sun glare (if any)
		scaler	= 1.0f / (opacity + SunGlareWashout);
		blend	= max( opacity, SunGlareWashout );

		// Decide on the composite blending color
		color.r = (opacity*color.r + SunGlareWashout) * scaler;
		color.g = (opacity*color.g + SunGlareWashout) * scaler;
		color.b = (opacity*color.b + SunGlareWashout) * scaler;

		// JB 010618 vary the brightness on cloud thickness
		if (g_bEnableWeatherExtensions && !thunderAndLightning) 
		{
			float thickness = fabs(viewpoint->GetLocalCloudTops() - position.z);
			if (thickness > 0)
			{
				float tbrt = max(.2, min(1, g_fCloudThicknessFactor / thickness));
				brightness = 0.95f * brightness + 0.05f * tbrt; // phase in new brt
			}
			else
				brightness = 1.0;

			color.r *= brightness;
			color.g *= brightness;
			color.b *= brightness;
		}

		// Construct a 32 bit RGB value
		ProcessColor( &color );
		c  = (FloatToInt32(color.r * 255.9f));
		c |= (FloatToInt32(color.g * 255.9f)) << 8;
		c |= (FloatToInt32(color.b * 255.9f)) << 16;

		// Are we IN it our NEAR it?
		if (blend >= 1.0f)
		{
			// Clear the screen to cloud color
			context.SetState( MPR_STA_BG_COLOR, c );
			ClearFrame();

			// Draw the tunnel vision effect if any
			if (tunnelSolidWidth > 0.0f)
			{
				SetFOV( prevFOV );
				SetViewport( prevLeft, prevTop, prevRight, prevBottom );
			}

			// And we're done!
			return;

		}
		else
		{
			// We're entering or leaving the cloud, so "fuzz" things
			PreSceneCloudOcclusion( blend, c );
		}
	}
*/
	//JAM 21Nov03
	realWeather->RefreshWeather(this);

	//JAM 15Dec03
	BOOL bToggle = FALSE; 

	if(context.bZBuffering && DisplayOptions.bZBuffering)
	{
		bToggle = TRUE;
		context.SetZBuffering(FALSE);
		context.SetState(MPR_STA_DISABLES,MPR_SE_Z_WRITE);
		context.SetState(MPR_STA_DISABLES,MPR_SE_Z_COMPARE);
	}

	// Draw the sky
	DrawSky();	

   //JAM 15Dec03
	if(bToggle && DisplayOptions.bZBuffering)
	{
		context.SetZBuffering(TRUE);
		context.SetState(MPR_STA_ENABLES,MPR_SE_Z_WRITE);
		context.SetState(MPR_STA_ENABLES,MPR_SE_Z_COMPARE);
	}
	//JAM

	// Sort the object list based on our location
	viewpoint->ResetObjectTraversal();

	// Figure out which list would contain our eye point
	containingList = viewpoint->GetContainingList( position.z );
	
	// Special case if we're above the roof and the roof is diplayed
	if ((containingList == 4) && (skyRoof))
	{
		viewpoint->ObjectsAboveRoof()->DrawBeyond( 0.0f, 0, this );

		// Restore the FOV if it was changed by the tunnel code
		if (tunnelSolidWidth > 0.0f)
		{
			SetFOV( prevFOV );
			SetViewport( prevLeft, prevTop, prevRight, prevBottom );
		}

		return;
	}

	// Draw scene components in height sorted groups dependent of our altitude
	// Upward order (don't draw the one we're in)
	if( containingList > 0 )
	{
		DrawGroundAndObjects(viewpoint->ObjectsInTerrain());

		if( containingList > 1 )
		{
			viewpoint->ObjectsBelowClouds()->DrawBeyond(0.f,0,this);

			if( containingList > 2 )
			{
				DrawCloudsAndObjects(viewpoint->Clouds(),viewpoint->ObjectsInClouds());
			
				if( containingList > 3 )
				{
					viewpoint->ObjectsAboveClouds()->DrawBeyond(0.f,0,this);
				}
			}
		}
	}

	// Downward order (finishing with the one we're in)
	viewpoint->ObjectsAboveRoof()->DrawBeyond(0.f,0,this);
	if( containingList < 4 ) 
	{
		viewpoint->ObjectsAboveClouds()->DrawBeyond(0.f,0,this);

		if( containingList < 3 ) 
		{
			DrawCloudsAndObjects(viewpoint->Clouds(),viewpoint->ObjectsInClouds());

			if( containingList < 2 )
			{
				viewpoint->ObjectsBelowClouds()->DrawBeyond(0.f,0,this);

				if( containingList < 1 ) 
				{
					DrawGroundAndObjects(viewpoint->ObjectsInTerrain());
				}
			}		
		}
	}

	realWeather->Draw();

	// Restore the FOV if it was changed by the tunnel code
	if(tunnelSolidWidth > 0.0f) 
	{
		SetFOV(prevFOV);
		SetViewport(prevLeft,prevTop,prevRight,prevBottom);
		SetCamera(&position,orientation);
	}
}

/***************************************************************************\
    Draw the out the window view, including terrain and all registered objects.
\***************************************************************************/
void RenderOTW::DrawGroundAndObjects( ObjectDisplayList *objectList )
{
	SpanListEntry*	span;
	

#ifdef TWO_D_MAP_AVAILABLE
	if (twoDmode) {
		// Set the clip flag for each vertex to indicate it hasn't been xformed
		int usedLODcount	= viewpoint->GetMaxLOD() - viewpoint->GetMinLOD() + 1;
		int LODbufferSize	= (maxSpanExtent) * (maxSpanExtent);
		for (TerrainVertex* v = vertexMemory; v <  vertexMemory + usedLODcount * LODbufferSize; v++) {
			v->clipFlag = 0xFFFF;
		}

		context.SetState(MPR_STA_DISABLES,MPR_SE_SHADING);
		context.SetState(MPR_STA_ENABLES,MPR_SE_ALPHA); //JAM 02Oct03 - MPR_SE_BLENDING );
	}
#endif

#ifdef CHECK_PROC_TIMES
	ulong procTime = GetTickCount();
	objTime = 0;
#endif
	// Compute the potentially visible region of terrain and divide it into rings
	ComputeBounds();
	BuildRingList();

	// Clip the inside edges of the rings to the computed bounding volume in world space
	ClipHorizontalSectors();
	ClipVerticalSectors();
	BuildCornerSet();
	TrimCornerSet();

	// Transform all the verteces required to draw the terrain squares described in the span list
	BuildVertexSet();
	TransformVertexSet();

#ifdef TWO_D_MAP_AVAILABLE
	if (twoDmode) {
		ThreeDVertex	v;
		ThreeDVertex	*vert = &v;
		Tpoint			v1, v2;
		int	levelCol;
		int	levelRow;
		int	LOD;

		for ( span = spanList; span<firstEmptySpan; span++ ) {
			LOD = span->LOD;

			levelRow = span->ring;
			if (span->Tsector.maxEndPoint > span->Tsector.minEndPoint) {
				v1.y = v2.y = (yRes>>1) - TWODSCALE*WORLD_TO_FLOAT_GLOBAL_POST( span->Tsector.insideEdge - viewpoint->X() );
				v1.x = (xRes>>1) + TWODSCALE*( WORLD_TO_FLOAT_GLOBAL_POST(span->Tsector.maxEndPoint - viewpoint->Y()) );
				v2.x = (xRes>>1) + TWODSCALE*( WORLD_TO_FLOAT_GLOBAL_POST(span->Tsector.minEndPoint - viewpoint->Y()) );

				SetColor( (0x4040 << ((span->LOD-2)*8)) | 0x80000000 );
				Render2DLine( (UInt16)v1.x, (UInt16)v1.y, (UInt16)v2.x, (UInt16)v2.y );                           
			}
			for (levelCol = span->Tsector.startDraw; levelCol <= span->Tsector.stopDraw; levelCol++) {
				vert->x = (xRes>>1) + TWODSCALE*((float)((levelCol+ LODdata[LOD].centerCol) << LOD) - WORLD_TO_FLOAT_GLOBAL_POST(viewpoint->Y()));
				vert->y = (yRes>>1) - TWODSCALE*((float)((levelRow+ LODdata[LOD].centerRow) << LOD) - WORLD_TO_FLOAT_GLOBAL_POST(viewpoint->X()));

				// Draw a marker at this vertex location
				SetColor( 0xF0008080 );
				Render2DTri((UInt16)(vert->x-1), (UInt16)(vert->y-1),
							(UInt16)(vert->x+1), (UInt16)(vert->y-1),
							(UInt16)(vert->x+1), (UInt16)(vert->y+1));
				Render2DTri((UInt16)(vert->x-1), (UInt16)(vert->y-1),
							(UInt16)(vert->x+1), (UInt16)(vert->y+1),
							(UInt16)(vert->x-1), (UInt16)(vert->y+1));
			}

			levelCol = span->ring;
			if (span->Rsector.maxEndPoint > span->Rsector.minEndPoint) {
				v1.x = v2.x = (xRes>>1) + TWODSCALE*WORLD_TO_FLOAT_GLOBAL_POST( span->Rsector.insideEdge - viewpoint->Y() );
				v1.y = (yRes>>1) - TWODSCALE*( WORLD_TO_FLOAT_GLOBAL_POST(span->Rsector.maxEndPoint - viewpoint->X()) );
				v2.y = (yRes>>1) - TWODSCALE*( WORLD_TO_FLOAT_GLOBAL_POST(span->Rsector.minEndPoint - viewpoint->X()) );

				SetColor( (0x4040 << ((span->LOD-2)*8)) | 0x80000000 );
				Render2DLine( (UInt16)v1.x, (UInt16)v1.y, (UInt16)v2.x, (UInt16)v2.y );                           
			}
			for (levelRow = span->Rsector.startDraw; levelRow <= span->Rsector.stopDraw; levelRow++) {
				vert->x = (xRes>>1) + TWODSCALE*((float)((levelCol+ LODdata[LOD].centerCol) << LOD) - WORLD_TO_FLOAT_GLOBAL_POST(viewpoint->Y()));
				vert->y = (yRes>>1) - TWODSCALE*((float)((levelRow+ LODdata[LOD].centerRow) << LOD) - WORLD_TO_FLOAT_GLOBAL_POST(viewpoint->X()));

				// Draw a marker at this vertex location
				SetColor( 0xF0008080 );
				Render2DTri((UInt16)(vert->x-1), (UInt16)(vert->y-1),
							(UInt16)(vert->x+1), (UInt16)(vert->y-1),
							(UInt16)(vert->x+1), (UInt16)(vert->y+1));
				Render2DTri((UInt16)(vert->x-1), (UInt16)(vert->y-1),
							(UInt16)(vert->x+1), (UInt16)(vert->y+1),
							(UInt16)(vert->x-1), (UInt16)(vert->y+1));
			}

			levelRow = -span->ring;
			if (span->Bsector.maxEndPoint > span->Bsector.minEndPoint) {
				v1.y = v2.y = (yRes>>1) - TWODSCALE*WORLD_TO_FLOAT_GLOBAL_POST( span->Bsector.insideEdge - viewpoint->X() );
				v1.x = (xRes>>1) + TWODSCALE*( WORLD_TO_FLOAT_GLOBAL_POST(span->Bsector.maxEndPoint - viewpoint->Y()) );
				v2.x = (xRes>>1) + TWODSCALE*( WORLD_TO_FLOAT_GLOBAL_POST(span->Bsector.minEndPoint - viewpoint->Y()) );

				SetColor( (0x4040 << ((span->LOD-2)*8)) | 0x80000000 );
				Render2DLine( (UInt16)v1.x, (UInt16)v1.y, (UInt16)v2.x, (UInt16)v2.y );                           
			}
			for (levelCol = span->Bsector.startDraw; levelCol <= span->Bsector.stopDraw; levelCol++) {
				vert->x = (xRes>>1) + TWODSCALE*((float)((levelCol+ LODdata[LOD].centerCol) << LOD) - WORLD_TO_FLOAT_GLOBAL_POST(viewpoint->Y()));
				vert->y = (yRes>>1) - TWODSCALE*((float)((levelRow+ LODdata[LOD].centerRow) << LOD) - WORLD_TO_FLOAT_GLOBAL_POST(viewpoint->X()));

				// Draw a marker at this vertex location
				SetColor( 0xF0008080 );
				Render2DTri((UInt16)(vert->x-1), (UInt16)(vert->y-1),
							(UInt16)(vert->x+1), (UInt16)(vert->y-1),
							(UInt16)(vert->x+1), (UInt16)(vert->y+1));
				Render2DTri((UInt16)(vert->x-1), (UInt16)(vert->y-1),
							(UInt16)(vert->x+1), (UInt16)(vert->y+1),
							(UInt16)(vert->x-1), (UInt16)(vert->y+1));
			}

			levelCol = -span->ring;
			if (span->Lsector.maxEndPoint > span->Lsector.minEndPoint) {
				v1.x = v2.x = (xRes>>1) + TWODSCALE*WORLD_TO_FLOAT_GLOBAL_POST( span->Lsector.insideEdge - viewpoint->Y() );
				v1.y = (yRes>>1) - TWODSCALE*( WORLD_TO_FLOAT_GLOBAL_POST(span->Lsector.maxEndPoint - viewpoint->X()) );
				v2.y = (yRes>>1) - TWODSCALE*( WORLD_TO_FLOAT_GLOBAL_POST(span->Lsector.minEndPoint - viewpoint->X()) );

				SetColor( (0x4040 << ((span->LOD-2)*8)) | 0x80000000 );
				Render2DLine( (UInt16)v1.x, (UInt16)v1.y, (UInt16)v2.x, (UInt16)v2.y );                           
			}
			for (levelRow = span->Lsector.startDraw; levelRow <= span->Lsector.stopDraw; levelRow++) {
				vert->x = (xRes>>1) + TWODSCALE*((float)((levelCol+ LODdata[LOD].centerCol) << LOD) - WORLD_TO_FLOAT_GLOBAL_POST(viewpoint->Y()));
				vert->y = (yRes>>1) - TWODSCALE*((float)((levelRow+ LODdata[LOD].centerRow) << LOD) - WORLD_TO_FLOAT_GLOBAL_POST(viewpoint->X()));

				// Draw a marker at this vertex location
				SetColor( 0xF0008080 );
				Render2DTri((UInt16)(vert->x-1), (UInt16)(vert->y-1),
							(UInt16)(vert->x+1), (UInt16)(vert->y-1),
							(UInt16)(vert->x+1), (UInt16)(vert->y+1));
				Render2DTri((UInt16)(vert->x-1), (UInt16)(vert->y-1),
							(UInt16)(vert->x+1), (UInt16)(vert->y+1),
							(UInt16)(vert->x-1), (UInt16)(vert->y+1));
			}
		}

//		return;
	}
#endif



	// Render all the require polygons from farthest to nearest
	for ( span = spanList+1; span<firstEmptySpan; span++ ) {

		// Call the appropriate routine to draw the ring
		if ( span->LOD == (span+1)->LOD ) {
			DrawTerrainRing( span );
		} else {
			// Skip the last ring at the lower LOD (its used only for culling and transformation)
			span++;

			// Use the first span at the new LOD to draw the connector ring
			DrawConnectorRing( span );

			span++;

			// Draw the gap filler
			DrawGapFiller( span );
		}

#ifdef CHECK_PROC_TIMES
	ulong procTime2 = GetTickCount();
#endif
		// Draw any object over this ring

		//JAM 13Nov03
		if(!(realWeather->weatherCondition > FAIR && (-viewpoint->Z()) > (-realWeather->stratusZ)))
		{
			// If we're above the overcast layer, ground objects are not visible.
			objectList->DrawBeyond(LEVEL_POST_TO_WORLD(span->ring,span->LOD),span->LOD,this);
		}
#ifdef CHECK_PROC_TIMES
	procTime2 = GetTickCount() - procTime2;
	objTime += procTime2;
#endif
	}
#ifdef CHECK_PROC_TIMES
	procTime = GetTickCount() - procTime;
	terrTime += procTime;
#endif
	// Draw any remaining objects

	//JAM 13Nov03
	if(!(realWeather->weatherCondition > FAIR && (-viewpoint->Z()) > (-realWeather->stratusZ)))
	{
		// If we're above the overcast layer, ground objects are not visible.
			objectList->DrawBeyond(0.f,-1,this);
	}

	// Turn off all non-default rendering parameters
	context.RestoreState( STATE_SOLID );
}


/***************************************************************************\
    Draw the clouds and the objects within the cloud layer.
\***************************************************************************/
void RenderOTW::DrawCloudsAndObjects(ObjectDisplayList *clouds, ObjectDisplayList *objects)
{
	float distance;
	
	do
	{
		distance = objects->GetNextDrawDistance();
		clouds->DrawBeyond(distance,0,this);
		objects->DrawBeyond(distance,0,this);
	}
	while (distance > -1.0f);
}


/***************************************************************************\
    Transform a row or column of terrain data at the given LOD.  The row and
	column parameters are specified in units of level posts relative to
	the viewpoint.  The local variables "levelRow" and "levelCol" are in
	units of level posts relative to the origin of the map.
\***************************************************************************/
void RenderOTW::TransformRun( int row, int col, int run, int LOD, BOOL do_row )
{
	register TerrainVertex	*vert;
	register Tpost			*post;
	int						levelRow, levelCol, levelStop;
	float					scratch_x, scratch_y, scratch_z;
	float					x, y, z, dz;
	int						*pChange;
	float					*hVector;
	float					*zVector;
	int						vertStep;


	// Dump out if we have nothing to do
	if (run <= 0)
		return;

	// Select which variable to increment based on whether we're doing rows or columns
	if ( do_row ) {
		pChange = &levelCol;
		hVector = LODdata[LOD].Ystep;
		vertStep = 1;
	} else {
		pChange = &levelRow;
		hVector = LODdata[LOD].Xstep;
		vertStep = maxSpanExtent;
	}
	zVector = LODdata[LOD].Zstep;


	// Find the coordinates of the first post to transform FROM
	levelRow	 = row + LODdata[LOD].centerRow;
	levelCol	 = col + LODdata[LOD].centerCol;
	levelStop	= *pChange + run;


	// Find the storage location for the first vertex to transform INTO
	vert =	vertexBuffer[LOD] + maxSpanExtent*row + col;

	// Get the this post from the terrain database
	post = viewpoint->GetPost( levelRow, levelCol, LOD );
	ShiAssert( post );

	// Compute our world space starting location
	x = LEVEL_POST_TO_WORLD( levelRow, LOD );
	y = LEVEL_POST_TO_WORLD( levelCol, LOD );

	// This part does rotation, translation, and scaling on the initial point
	// Note, we're swapping the x and z axes here to get from z up/down to z far/near
	// then we're swapping the x and y axes to get into conventional screen pixel coordinates
	z = post->z;
	scratch_z = T.M11 * x + T.M12 * y + T.M13 * z + move.x;
	scratch_x = T.M21 * x + T.M22 * y + T.M23 * z + move.y;
	scratch_y = T.M31 * x + T.M32 * y + T.M33 * z + move.z;


	// Transform all the rest of the verteces (will break out below)
	while (TRUE) {

		ShiAssert( vert >= vertexBuffer[LOD] - maxSpanExtent*maxSpanOffset - maxSpanOffset );
		ShiAssert( vert <= vertexBuffer[LOD] + maxSpanExtent*maxSpanOffset + maxSpanOffset );

		// Store a pointer to the source post in the transformed vertex structure
		vert->post = post;


#ifdef TWO_D_MAP_AVAILABLE
	if (twoDmode) {
		vert->x = (xRes>>1) + TWODSCALE*((float)(levelCol << LOD) - WORLD_TO_FLOAT_GLOBAL_POST(viewpoint->Y()));
		vert->y = (yRes>>1) - TWODSCALE*((float)(levelRow << LOD) - WORLD_TO_FLOAT_GLOBAL_POST(viewpoint->X()));

		vert->clipFlag = ON_SCREEN;

		vert->r = 0.3f;
		vert->g = 0.1f;
		vert->b = 0.1f;
		vert->a = 0.5f;

		// Draw a marker at this vertex location
		SetColor( 0x80808080 );
		Render2DTri((UInt16)(vert->x-1), (UInt16)(vert->y-1),
					(UInt16)(vert->x+1), (UInt16)(vert->y-1),
					(UInt16)(vert->x+1), (UInt16)(vert->y+1));
		Render2DTri((UInt16)(vert->x-1), (UInt16)(vert->y-1),
					(UInt16)(vert->x+1), (UInt16)(vert->y+1),
					(UInt16)(vert->x-1), (UInt16)(vert->y+1));
	} else {
#endif
		// Now determine if the point is out behind us or to the sides
		// See GetRangeClipFlags(), GetHorizontalClipFlags(), and GetVerticalClipFlags()
		vert->clipFlag = ON_SCREEN;

		if ( scratch_z < NEAR_CLIP ) {
			vert->clipFlag |= CLIP_NEAR;
		}

		if ( fabs(scratch_y) > scratch_z ) {
			if ( scratch_y > scratch_z ) {
				vert->clipFlag |= CLIP_BOTTOM;
			} else {
				vert->clipFlag |= CLIP_TOP;
			}
		}

		if ( fabs(scratch_x) > scratch_z ) {
			if ( scratch_x > scratch_z ) {
				vert->clipFlag |= CLIP_RIGHT;
			} else {
				vert->clipFlag |= CLIP_LEFT;
			}
		}


		vert->csX = scratch_x;
		vert->csY = scratch_y;
		vert->csZ = scratch_z;


		// Finally, do the perspective divide and scale and shift into screen space
		if ( !(vert->clipFlag & CLIP_NEAR) ) {
			ShiAssert( scratch_z > 0.0f );
			register float OneOverZ = 1.0f / scratch_z;
			vert->x = viewportXtoPixel( scratch_x * OneOverZ );
			vert->y = viewportYtoPixel( scratch_y * OneOverZ );
			vert->q = scratch_z * Q_SCALE;
		}


		// Do any color computations required for this post
		ComputeVertexColor(vert,post,scratch_z,scratch_x,scratch_y); //JAM 03Dec03


#ifdef TWO_D_MAP_AVAILABLE
	}
#endif

		//
		// Break out of our loop if we need to
		//
		if (*pChange >= levelStop) {
			break;
		}


		// Advance to the next post to transform
		vert += vertStep;
		(*pChange)++;

		// Get the this post from the terrain database
		post = viewpoint->GetPost( levelRow, levelCol, LOD );
		ShiAssert( post );

		// Compute the new transformed location based on the known horizontal
		// step, and the vertical difference between this post and the previous one
		dz = post->z - z;
		z = post->z;
		scratch_z += hVector[0] + dz * zVector[0];
		scratch_x += hVector[1] + dz * zVector[1];
		scratch_y += hVector[2] + dz * zVector[2];
 	}
}


/***************************************************************************\
    Compute the fog percentage for the given location.
	NOTE:  worldZ is normally negative (-Z points up)
\***************************************************************************/
//JAM 26Dec03
float RenderOTW::GetValleyFog(float distance, float worldZ)
{
	float	fog;
	float	valleyFog;

	// Valley fog
	static const float	VALLEY_HAZE_TOP			= 1000.0f;
	static const float	VALLEY_HAZE_MAX			= 0.75f;
	static const float	VALLEY_HAZE_START_RANGE	= PERSPECTIVE_RANGE;
	static const float	VALLEY_HAZE_FULL_RANGE	= PERSPECTIVE_RANGE + 36000.0f;

	if(worldZ > -VALLEY_HAZE_TOP)
	{
		// We're below the top of the valley fog layer
		valleyFog = (VALLEY_HAZE_TOP + worldZ)/VALLEY_HAZE_TOP;
		valleyFog = min(valleyFog,VALLEY_HAZE_MAX);

		if(distance < VALLEY_HAZE_FULL_RANGE)
		{
			valleyFog *= (distance-VALLEY_HAZE_START_RANGE)/(VALLEY_HAZE_FULL_RANGE-VALLEY_HAZE_START_RANGE);
		}

		// Distance fog
		fog = GetRangeOnlyFog(distance);

		// Mixing
		return max(fog,valleyFog);
	}
	else
	{
		// We're above the valley fog layer
		return GetRangeOnlyFog(distance);
	}
}

	
/***************************************************************************\
    Compute the color and texture blend value for a single terrain vertex.
\***************************************************************************/
//JAM 11Jan04
void RenderOTW::ComputeVertexColor(TerrainVertex *vert, Tpost *post, float distance, float x, float y)
{
	Ppoint n;
	int row,col;
	float fog,alpha;

	float scale = min(distance/far_clip,1.f);
	float inv = 1.f-scale;

	float r = TheMap.ColorTable[post->colorIndex].r*scale+ground_color.r*inv;
	float g = TheMap.ColorTable[post->colorIndex].g*scale+ground_color.g*inv;
	float b = TheMap.ColorTable[post->colorIndex].b*scale+ground_color.b*inv;

	if(realWeather->weatherCondition > FAIR
	&& (-viewpoint->Z()) > (-realWeather->stratusZ))
	{
		vert->RenderingStateHandle = state_far;
	}
	else if(distance > haze_start+haze_depth)
	{
		vert->RenderingStateHandle = state_far;
	}
	else
	{
		if(distance < PERSPECTIVE_RANGE)
		{
			vert->RenderingStateHandle = state_fore;
		}
		else if(distance < haze_start)
		{
			vert->RenderingStateHandle = state_near;
		}
		else
		{
			vert->RenderingStateHandle = state_mid;
		}

		if(DisplayOptions.bSpecularLighting)
		{
			float iDiff = 0.f;

			if(lightDiffuse)
			{
				n.x = sinf(post->phi)*cosf(post->theta);
				n.y = sinf(post->phi)*sinf(post->theta);
				n.z = -cosf(post->phi);
	
				iDiff = max(n.x*lightVector.x+n.y*lightVector.y+n.z*lightVector.z,0.f);
			}

			float iTot = min(lightAmbient+iDiff,1.f);

			r *= iTot;
			g *= iTot;
			b *= iTot;
		}

		if(PlayerOptions.ShadowsOn() && realWeather->weatherCondition == FAIR)
		{
			for(row = realWeather->shadowCell; row < realWeather->numCells-realWeather->shadowCell; row++)
			{
				for(col = realWeather->shadowCell; col < realWeather->numCells-realWeather->shadowCell; col++)
				{
					if(realWeather->weatherCellArray[row][col].onScreen)
					{
						float dx = x-realWeather->weatherCellArray[row][col].shadowPos.x;
						float dy = y-realWeather->weatherCellArray[row][col].shadowPos.y;
						float dz = distance-realWeather->weatherCellArray[row][col].shadowPos.z; 
						float range = FabsF(SqrtF(dx*dx+dy*dy+dz*dz));

						if(range < realWeather->cloudRadius)
						{
							float i = max(1.f-(realWeather->cloudRadius-range)/realWeather->cloudRadius,.5f);
	
							r *= i;
							g *= i;
							b *= i;
						}
					}
				}
			}
		}
		else if(realWeather->weatherCondition > FAIR)
		{
			if((-viewpoint->Z()) > (-realWeather->stratusZ))
			{
				r = haze_ground_color.r;
				g = haze_ground_color.g;
				b = haze_ground_color.b;
			}
			else if(realWeather->isLightning)
			{
				Tcolor lightningColor = TheTimeOfDay.GetLightningColor();
	
				float dx = x-realWeather->lightningGroundPos.x;
				float dz = distance-realWeather->lightningGroundPos.z; 

				float range = FabsF(SqrtF(dx*dx+dz*dz));

				if(range < LIGHTNING_RADIUS)
				{
					float i = max(1.f-(LIGHTNING_RADIUS-range)/LIGHTNING_RADIUS,.5f);
	
					r = ((1.f-i)*lightningColor.r)+(i*r);
					g = ((1.f-i)*lightningColor.g)+(i*g);
					b = ((1.f-i)*lightningColor.b)+(i*b);
				}
			}
		}
	}

	if(distance > haze_start+haze_depth)
	{
		alpha = 0.f;
	}
	else if(distance < PERSPECTIVE_RANGE)
	{
		alpha = 1.f;
	}
	else
	{
		if(hazed)
		{
			fog = min(GetValleyFog(distance,post->z),.6f);

			if(distance < haze_start)
			{
				alpha = 1.f-fog;
			}
			else
			{
				alpha = GetRangeOnlyFog(distance);

				if(alpha < fog)	alpha = fog;

				alpha = 1.f-alpha;
			}
		}
		else
		{
			if(distance < haze_start)
			{
				alpha = 1.f;
			}
			else
			{
				alpha = GetRangeOnlyFog(distance);

				alpha = 1.f-alpha;
			}
		}
	}


	vert->r = r;
	vert->g = g;
	vert->b = b;
	vert->a = alpha;

	context.SetTVmode(FALSE);
	context.SetIRmode(FALSE);
	TheStateStack.SetFog(alpha,(Pcolor*)GetFogColor());
}
//JAM

/***************************************************************************\
    Draw one ring of terrain data as described in the span list.
\***************************************************************************/
void RenderOTW::DrawTerrainRing( SpanListEntry *span )
{
	int				LOD		 = span->LOD;
	register int	r, c;
	int				crossOver;


	// TOP_SPAN -- Horizontal (Sector 0 and 7)
	r = span->ring;

	crossOver = max( span->Tsector.startDraw, 0 ); 
	for ( c = span->Tsector.stopDraw; c >= crossOver; c-- )		DrawTerrainSquare( r, c, LOD );

	crossOver = min( span->Tsector.stopDraw, -1 ); 
	for ( c = span->Tsector.startDraw; c <= crossOver; c++ )	DrawTerrainSquare( r, c, LOD );

		
	// RIGHT_SPAN -- Vertical (Sector 1 and 2)
	c = span->ring;

	crossOver = max( span->Rsector.startDraw, 0 ); 
	for ( r = span->Rsector.stopDraw; r >= crossOver; r-- )		DrawTerrainSquare( r, c, LOD );

	crossOver = min( span->Rsector.stopDraw, -1 ); 
	for ( r = span->Rsector.startDraw; r <= crossOver; r++ )	DrawTerrainSquare( r, c, LOD );


	// BOTTOM_SPAN -- Horizontal (Sector 3 and 4)
	r = -span->ring;

	crossOver = max( span->Bsector.startDraw, 0 ); 
	for ( c = span->Bsector.stopDraw; c >= crossOver; c-- )		DrawTerrainSquare( r, c, LOD );

	crossOver = min( span->Bsector.stopDraw, -1 ); 
	for ( c = span->Bsector.startDraw; c <= crossOver; c++ )	DrawTerrainSquare( r, c, LOD );


	// LEFT_SPAN -- Vertical (Sector 5 and 6)
	c = -span->ring;

	crossOver = max( span->Lsector.startDraw, 0 ); 
	for ( r = span->Lsector.stopDraw; r >= crossOver; r-- )		DrawTerrainSquare( r, c, LOD );

	crossOver = min( span->Lsector.stopDraw, -1 ); 
	for ( r = span->Lsector.startDraw; r <= crossOver; r++ )	DrawTerrainSquare( r, c, LOD );
}


/***************************************************************************\
    Draw one ring of terrain data as described in the span list.
	We're expecting the "outter transform" span.  That is, the first
	span at the new higher LOD.
\***************************************************************************/
void RenderOTW::DrawConnectorRing( SpanListEntry *outterSpan )
{
	int				LOD;
	int				crossOver;
	register int	r, c;
	SpanListEntry	*span;

	LOD = outterSpan->LOD;


	// TOP_SPAN -- Horizontal (Sector 0 and 7)
	span = LODdata[LOD].glueOnBottom ? outterSpan+1 : outterSpan;

	crossOver = max( span->Tsector.startDraw, LODdata[LOD].glueOnLeft );
	for ( c = span->Tsector.stopDraw; c >= crossOver; c-=2 )	DrawUpConnector( span->ring, c, LOD );

	crossOver = min( span->Tsector.stopDraw, -2+LODdata[LOD].glueOnLeft ); 
	for ( c = span->Tsector.startDraw; c <= crossOver; c+=2 )	DrawUpConnector( span->ring, c, LOD );


	// RIGHT_SPAN -- Vertical (Sector 1 and 2)
	span = LODdata[LOD].glueOnLeft ? outterSpan+1 : outterSpan;

	crossOver = max( span->Rsector.startDraw, LODdata[LOD].glueOnBottom ); 
	for ( r = span->Rsector.stopDraw; r >= crossOver; r-=2 )	DrawRightConnector( r, span->ring, LOD );

	crossOver = min( span->Rsector.stopDraw, -2+LODdata[LOD].glueOnBottom ); 
	for ( r = span->Rsector.startDraw; r <= crossOver; r+=2 )	DrawRightConnector( r, span->ring, LOD );


	// BOTTOM_SPAN -- Horizontal (Sector 3 and 4)
	span = LODdata[LOD].glueOnBottom ? outterSpan : outterSpan+1;

	crossOver = max( span->Bsector.startDraw, LODdata[LOD].glueOnLeft ); 
	for ( c = span->Bsector.stopDraw; c >= crossOver; c-=2 )	DrawDownConnector( -span->ring+1, c, LOD );

	crossOver = min( span->Bsector.stopDraw, -2+LODdata[LOD].glueOnLeft ); 
	for ( c = span->Bsector.startDraw; c <= crossOver; c+=2 )	DrawDownConnector( -span->ring+1, c, LOD );

	
	// LEFT_SPAN -- Vertical (Sector 5 and 6)
	span = LODdata[LOD].glueOnLeft ? outterSpan : outterSpan+1;

	crossOver = max( span->Lsector.startDraw, LODdata[LOD].glueOnBottom ); 
	for ( r = span->Lsector.stopDraw; r >= crossOver; r-=2 )	DrawLeftConnector( r, -span->ring+1, LOD );

	crossOver = min( span->Lsector.stopDraw, -2+LODdata[LOD].glueOnBottom ); 
	for ( r = span->Lsector.startDraw; r <= crossOver; r+=2 )	DrawLeftConnector( r, -span->ring+1, LOD );
}


/***************************************************************************\
    Draw a partial ring to fill the gaps between the rectangular connector
	ring and the square inner rings.
\***************************************************************************/
void RenderOTW::DrawGapFiller( SpanListEntry *span )
{
	int				LOD		= span->LOD;
	register int	r, c;
	int				crossOver;


	if ( LODdata[span->LOD].glueOnBottom ) {
		// BOTTOM_SPAN -- Horizontal (Sector 3 and 4)
		r = -span->ring;

		crossOver = max( span->Bsector.startDraw, 0 ); 
		for ( c = span->Bsector.stopDraw; c >= crossOver; c-- )		DrawTerrainSquare( r, c, LOD );

		crossOver = min( span->Bsector.stopDraw, -1 ); 
		for ( c = span->Bsector.startDraw; c <= crossOver; c++ )	DrawTerrainSquare( r, c, LOD );
	} else {
		// TOP_SPAN -- Horizontal (Sector 0 and 7)
		r = span->ring;

		crossOver = max( span->Tsector.startDraw, 0 ); 
		for ( c = span->Tsector.stopDraw; c >= crossOver; c-- )		DrawTerrainSquare( r, c, LOD );

		crossOver = min( span->Tsector.stopDraw, -1 ); 
		for ( c = span->Tsector.startDraw; c <= crossOver; c++ )	DrawTerrainSquare( r, c, LOD );
	}

		
	if ( LODdata[span->LOD].glueOnLeft ) {
		// LEFT_SPAN -- Vertical (Sector 5 and 6)
		c = -span->ring;

		crossOver = max( span->Lsector.startDraw, 0 ); 
		for ( r = span->Lsector.stopDraw; r >= crossOver; r-- )		DrawTerrainSquare( r, c, LOD );

		crossOver = min( span->Lsector.stopDraw, -1 ); 
		for ( r = span->Lsector.startDraw; r <= crossOver; r++ )	DrawTerrainSquare( r, c, LOD );
	} else {
		// RIGHT_SPAN -- Vertical (Sector 1 and 2)
		c = span->ring;

		crossOver = max( span->Rsector.startDraw, 0 ); 
		for ( r = span->Rsector.stopDraw; r >= crossOver; r-- )		DrawTerrainSquare( r, c, LOD );

		crossOver = min( span->Rsector.stopDraw, -1 ); 
		for ( r = span->Rsector.startDraw; r <= crossOver; r++ )	DrawTerrainSquare( r, c, LOD );
	}	
}

void RenderOTW::DrawWeather(const Trotation *orientation )
{
    // rain effects
    if (rainFactor > 0) {
	int max = 100 * rainFactor;
	DWORD rcol = TheTimeOfDay.GetRainColor();
	if (TheTimeOfDay.GetNVGmode())
	    rcol &= 0xff00ff00; // just green component
	DWORD ocol = Color();
	SetColor(rcol);
	mlTrig mlt;
	mlSinCos(&mlt, roll);

	// JB 010608 Adjust for speed
	float speedfactor = (viewpoint->Speed + 10) / 100;
	float dx = 0.033f * mlt.sin / speedfactor;
	float dy = 0.033f * mlt.cos / speedfactor;
	max = float(max) * speedfactor;

	for (int i = 0; i < max; i ++) {
	    float sx, sy;
	    sx = PRANDFloat();
	    sy = PRANDFloat();
	    
	    // just vertical lines currently. Need to slant them based on speed....
	    // but thats tricky, cos we have no knowledge of that here
			// JB 010608 We do now!
	    Render2DLine(viewportXtoPixel(sx), viewportYtoPixel(sy), 
		viewportXtoPixel(sx+dx), viewportYtoPixel(sy + dy));
	}
	SetColor(ocol);
    }

    if (thunderAndLightning) {
	// draw some shapes...

	// do something clever - damm out of time!!
	// algorithm written, (in perl) and ready to go,
	// this is the wrong place anyway, should be tied to a cloud and done in 3-d space.
    }

    if (snowFactor > 0) {
	// snow effects
	int max = 100 * snowFactor;
	DWORD scol = TheTimeOfDay.GetSnowColor();
	if (TheTimeOfDay.GetNVGmode())
	    scol &= 0xff00ff00; // just green component
	DWORD ocol = Color();
	SetColor(scol);
	static const float TRIX = 0.009f, TRI2 = TRIX/2.0f;
	
	for (int i = 0; i < max; i++) {
	    float sx, sy;
	    sx = PRANDFloat();
	    sy = PRANDFloat();

	    // we just draw small 6-pt stars. Two crossed triangles.
	    Render2DTri(viewportXtoPixel(sx), viewportXtoPixel(sy), 
		viewportXtoPixel(sx +TRIX), viewportXtoPixel(sy), 
		viewportXtoPixel(sx+TRI2), viewportXtoPixel(sy + TRIX));
	    Render2DTri(viewportXtoPixel(sx), viewportXtoPixel(sy+TRI2), 
		viewportXtoPixel(sx+TRIX), viewportXtoPixel(sy+TRI2), 
		viewportXtoPixel(sx+TRI2), viewportXtoPixel(sy-TRI2));
	}
	SetColor(ocol);
    }
}

