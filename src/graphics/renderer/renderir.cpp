/***************************************************************************\
    RenderIR.cpp
    Miro "Jammer" Torrielli
    30Dec03

    This sub class draws an out the window view in simulated IR (green on black)
\***************************************************************************/
#include "Tmap.h"
#include "Tpost.h"
#include "Draw2D.h"
#include "DrawOVC.h"
#include "RenderIR.h"
#include "TOD.h"
#include "RealWeather.h"
#include "FalcLib\include\playerop.h"
#include "Graphics\DXEngine\DXEngine.h"

extern	bool g_bUse_DX_Engine;
extern bool g_bGreyMFD;
extern bool bNVGmode;

void RenderIR::Setup(ImageBuffer *imageBuffer, RViewPoint *vp)
{
	RenderTV::Setup(imageBuffer,vp);
	
	sky_color.r			= sky_color.g			= sky_color.b			= 0.f;
	haze_sky_color.r	= haze_sky_color.g		= haze_sky_color.b		= 0.f;
	haze_ground_color.r	= haze_ground_color.g	= haze_ground_color.b	= 0.f;
	earth_end_color.r	= earth_end_color.g		= earth_end_color.b		= 0.f;

	//JAM 08Jan04
	lightAmbient = 1.f;
	lightDiffuse = 0.f;
	lightSpecular = 0.f;
}

void RenderIR::StartDraw(void)
{
//	DX - YELLOW BUG FIX - RED
	RenderOTW::StartDraw();
	//Drawable2D::SetGreenMode(TRUE); // RV - I-Hawk - Do not force green 
	TheColorBank.SetColorMode(ColorBankClass::NormalMode);

	context.SetIRmode(TRUE);
	//realWeather->SetGreenMode(TRUE); // RV - I-Hawk - Do not force green
	// Enable DX engine TV Mode
	TheDXEngine.SaveState();
	TheDXEngine.SetState(DX_TV);
}

void RenderIR::EndDraw(void)
{
//	DX - YELLOW BUG FIX - RED
	RenderOTW::EndDraw();
	Drawable2D::SetGreenMode(FALSE);

	context.SetIRmode(FALSE);
	realWeather->SetGreenMode(FALSE);
	// Disable DX engine TV Mode
	TheDXEngine.RestoreState();

}

void RenderIR::ComputeVertexColor(TerrainVertex *vert, Tpost *post, float distance, float x, float y)
{
	vert->r = 0.0f; 
	vert->b = 0.0f; 
	vert->g = TheMap.ColorTable[post->colorIndex].g*NVG_LIGHT_LEVEL; //TheMap.GreenTable[post->colorIndex].g*NVG_LIGHT_LEVEL;
	vert->a = 1.f;

	// FRB - B&W
	if ((g_bGreyMFD) && (!bNVGmode))
		vert->r = vert->b = vert->g; 

	vert->RenderingStateHandle = state_far;
}
