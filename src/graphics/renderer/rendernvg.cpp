/***************************************************************************\
    RenderNVG.cpp
    Miro "Jammer" Torrielli
    30Dec03

    This sub class draws an out the window view in simulated IR (green on black)
\***************************************************************************/
#include "Tmap.h"
#include "Tpost.h"
#include "TOD.h"
#include "Tex.h"
#include "Draw2D.h"
#include "DrawOVC.h"
#include "ColorBank.h"
#include "RenderNVG.h"
#include "RealWeather.h"
#include "FalcLib\include\playerop.h"

extern bool g_bGreyMFD;
extern bool bNVGmode;

void RenderNVG::Setup(ImageBuffer *imageBuffer, RViewPoint *vp)
{
	RenderOTW::Setup(imageBuffer,vp);
}

void RenderNVG::StartDraw(void)
{
//	DX - YELLOW BUG FIX - RED
	RenderOTW::StartDraw();
	Drawable2D::SetGreenMode(TRUE);
	realWeather->SetGreenMode(TRUE);
	TheColorBank.SetColorMode(ColorBankClass::UnlitGreenMode);
}

void RenderNVG::SetColor(DWORD packedRGBA)
{
	RenderOTW::SetColor(packedRGBA);
}

void RenderNVG::DrawSun(void)
{
	RenderOTW::DrawSun();
}

void RenderNVG::ComputeVertexColor(TerrainVertex *vert, Tpost *post, float distance, float x, float y)
{
	float alpha,fog;

	vert->r = 0.f;
	vert->b = 0.f;
	vert->g = NVG_LIGHT_LEVEL;

	// FRB - B&W
	if ((g_bGreyMFD) && (!bNVGmode))
		vert->r = vert->b = vert->g; 

	if((distance > haze_start+haze_depth+3000)
	||(realWeather->weatherCondition > FAIR && (-realWeather->viewerZ) > (-realWeather->stratusZ)))
	{
		vert->RenderingStateHandle = state_far;
	}
	else if(distance < PERSPECTIVE_RANGE)
	{
		vert->RenderingStateHandle = state_fore;
	}
	else if(!hazed && distance < haze_start)
	{
		vert->RenderingStateHandle = state_near;
	}
	else if(distance > haze_start+haze_depth)
	{
		vert->RenderingStateHandle = state_far;
	}
	else
	{
		vert->RenderingStateHandle = state_mid;
	}

	if(distance > haze_start + haze_depth)
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

	vert->a = alpha;
}

void RenderNVG::SetTimeOfDayColor(void)
{
	lightAmbient = NVG_LIGHT_LEVEL;
	lightDiffuse = 0.f;
	lightSpecular = 0.f;
	TheTimeOfDay.GetLightDirection(&lightVector);

	sky_color.r			= 0.f;
	sky_color.g			= NVG_SKY_LEVEL;
	sky_color.b			= 0.f;
	haze_sky_color.r	= 0.f;
	haze_sky_color.g	= NVG_SKY_LEVEL;
	haze_sky_color.b	= 0.f;
	earth_end_color.r	= 0.f;
	earth_end_color.g	= NVG_SKY_LEVEL;
	earth_end_color.b	= 0.f;
	haze_ground_color.r	= 0.f;
	haze_ground_color.g	= NVG_SKY_LEVEL;
	haze_ground_color.b	= 0.f;

	DWORD ground_haze = (FloatToInt32(haze_ground_color.g*255.9f) << 8)+0xff000000;
	context.SetState(MPR_STA_FOG_COLOR,ground_haze);
}

void RenderNVG::ProcessColor(Tcolor *color)
{
	color->r  = 0.f;
	color->g *= NVG_LIGHT_LEVEL;
	color->b  = 0.f;

	// FRB - B&W
	if (g_bGreyMFD)
		color->r = color->b = color->g; 

}
