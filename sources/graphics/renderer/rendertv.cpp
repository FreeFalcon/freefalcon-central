/***************************************************************************\
    RenderTV.cpp
    Miro "Jammer" Torrielli
    30Dec03

    This sub class draws an out the window view in simulated IR (green on black)
\***************************************************************************/
#include "TOD.h"
#include "Tmap.h"
#include "Tpost.h"
#include "Tex.h"
#include "Draw2D.h"
#include "ColorBank.h"
#include "RViewPnt.h"
#include "RenderTV.h"
#include "RealWeather.h"
#include "FalcLib\include\playerop.h"
#include "Graphics\DXEngine\DXEngine.h"

extern	bool g_bUse_DX_Engine;
extern bool g_bGreyMFD;
extern bool bNVGmode;

void RenderTV::Setup(ImageBuffer *imageBuffer, RViewPoint *vp)
{
	RenderOTW::Setup(imageBuffer,vp);
	
	SetObjectTextureState(FALSE);
	SetHazeMode(TRUE);

	TimeUpdateCallback(this);
}

void RenderTV::StartDraw(void)
{
//	DX - YELLOW BUG FIX - RED
	RenderOTW::StartDraw();
	//Drawable2D::SetGreenMode(TRUE); // RV - I-Hawk - Do not force green
	TheColorBank.SetColorMode(ColorBankClass::NormalMode);

	context.SetTVmode(TRUE);
	//realWeather->SetGreenMode(TRUE); // RV - I-Hawk - Do not force green
	// Enable DX engine TV Mode
	TheDXEngine.SaveState();
	TheDXEngine.SetState(DX_TV);
}

void RenderTV::EndDraw(void)
{
//	DX - YELLOW BUG FIX - RED
	RenderOTW::EndDraw();
	Drawable2D::SetGreenMode(FALSE);

	context.SetTVmode(FALSE);
	realWeather->SetGreenMode(FALSE);
	// Disable DX engine TV Mode
	TheDXEngine.RestoreState();
}

void RenderTV::SetColor(DWORD packedRGBA)
{
	//packedRGBA |= (packedRGBA<<8) & 0xFF; // RV - I-Hawk - Allow colors
	//packedRGBA |= (packedRGBA>>8) & 0xFF; 
	RenderOTW::SetColor(packedRGBA & 0xFFFFFFFF); // Was (0xFF00FF00)
	// Enable DX engine TV Mode, THIS HAS TO BE CHANGED IN ANOTHER WAY
	TheDXEngine.SetState(DX_TV);
}

void RenderTV::ComputeVertexColor(TerrainVertex *vert, Tpost *post, float distance, float x, float y)
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

void RenderTV::ProcessColor(Tcolor *color)
{
	color->r = 1.0f;
	color->b = 1.0f;
}

void RenderTV::DrawSun(void)
{
	Tpoint center;

	ShiAssert(TheTimeOfDay.ThereIsASun());

	TheTimeOfDay.CalculateSunMoonPos(&center,FALSE);

	context.RestoreState(STATE_ALPHA_TEXTURE_GOURAUD);
	context.SelectTexture1(viewpoint->SunTexture.TexHandle());
	DrawCelestialBody(&center,SUN_DIST/4.f,1.f,0.f,1.f,0.f);
}

void RenderTV::DrawMoon(void)
{
	Tpoint center;

	ShiAssert(TheTimeOfDay.ThereIsAMoon());

	TheTimeOfDay.CalculateSunMoonPos(&center,TRUE);

	context.RestoreState(STATE_ALPHA_TEXTURE);
	context.SelectTexture1(viewpoint->GreenMoonTexture.TexHandle());
	DrawCelestialBody(&center,MOON_DIST);
}
