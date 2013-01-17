/***************************************************************************\
    DrawOVC.cpp
    Miro "Jammer" Torrielli
    08Nov03

	- Drawable stratus
\***************************************************************************/
#include "RenderOW.h"
#include "Matrix.h"
#include "TOD.h"
#include "Tex.h"
#include "DrawOVC.h"
#include "RealWeather.h"

extern int g_nGfxFix;

#ifdef USE_SH_POOLS
MEM_POOL Drawable2DCloud::pool;
#endif

BOOL Drawable2DCloud::greenMode	= FALSE;
Tcolor Drawable2DCloud::litCloudColor = { 0.f };

//static const float TEX_UV_LSB = 1.f/1024.f;
//static const float TEX_UV_MIN = TEX_UV_LSB;
//static const float TEX_UV_MAX = 1.f-TEX_UV_LSB;

Drawable2DCloud::Drawable2DCloud() : DrawableObject( 1.0f )
{
	radius = 1.f;
	layerRadius = 0;
	drawClassID = realClouds;
}

Drawable2DCloud::~Drawable2DCloud(void)
{
}

void Drawable2DCloud::Update(Tpoint *worldPos, int txtIndex)
{
	memcpy(&position,worldPos,sizeof(Tpoint));

	if(realWeather->weatherCondition == SUNNY)
		cloudTexture = realWeather->CirrusCumTextures;
	else if(realWeather->weatherCondition == FAIR)
		cloudTexture = realWeather->CirrusCumTextures;
	else
		cloudTexture = realWeather->overcastTexture;
}

void Drawable2DCloud::Draw(class RenderOTW *renderer, int)
{
	// if inside an overcast, no draw
	if(realWeather->InsideOvercast())return;

	Tpoint ws;
	float minFog,cloudColor;
	ThreeDVertex v0,v1,v2,v3;
 
 	if(realWeather->weatherCondition < POOR)
 	{
 		minFog = .2f;
 		cloudColor = 1.f;
 	}
 	else if(realWeather->weatherCondition == POOR)
 	{
 		minFog = .8f;
 		cloudColor = .8f;
 	}
 	else
 	{
 		minFog = .8f;
 		cloudColor = .6f;
 	}

	cloudColor = 1.f;
	layerRadius = 80000;

	ws.x = position.x;
	ws.y = position.y;
	ws.z = position.z;
	renderer->TransformPoint(&ws,&v0);
	
	ws.x = position.x + layerRadius;
	ws.y = position.y;
	renderer->TransformPoint(&ws,&v1);
	
	ws.x = position.x + layerRadius;
	ws.y = position.y - layerRadius;
	renderer->TransformPoint(&ws,&v2);
		
	ws.x = position.x;
	ws.y = position.y - layerRadius;
	renderer->TransformPoint(&ws,&v3);
	
	v0.u = TEX_UV_MIN, v0.v = TEX_UV_MAX, v0.q = v0.csZ * Q_SCALE;
	v1.u = TEX_UV_MIN, v1.v = TEX_UV_MIN, v1.q = v1.csZ * Q_SCALE;
	v2.u = TEX_UV_MAX, v2.v = TEX_UV_MIN, v2.q = v2.csZ * Q_SCALE;
	v3.u = TEX_UV_MAX, v3.v = TEX_UV_MAX, v3.q = v3.csZ * Q_SCALE;
	
	v0.a = 1.f - min(max(renderer->GetRangeOnlyFog(v0.csZ),minFog),1.f);
	v1.a = 1.f - min(max(renderer->GetRangeOnlyFog(v1.csZ),minFog),1.f);
	v2.a = 1.f - min(max(renderer->GetRangeOnlyFog(v2.csZ),minFog),1.f);
	v3.a = 1.f - min(max(renderer->GetRangeOnlyFog(v3.csZ),minFog),1.f);
	
	if(greenMode)
	{
		v0.r = v1.r = v2.r = v3.r = 0.f;
		v0.g = v1.g = v2.g = v3.g = .4f;
		v0.b = v1.b = v2.b = v3.b = 0.f;
	}
	else
	{
		v0.r = v1.r = v2.r = v3.r = litCloudColor.r;
		v0.g = v1.g = v2.g = v3.g = litCloudColor.g;
		v0.b = v1.b = v2.b = v3.b = litCloudColor.b;
	}
	
	renderer->context.RestoreState(STATE_ALPHA_TEXTURE_GOURAUD_PERSPECTIVE);
	renderer->context.SelectTexture1(cloudTexture.TexHandle());
	renderer->DrawSquare(&v0,&v1,&v2,&v3,CULL_ALLOW_ALL);
}

void Drawable2DCloud::SetGreenMode(BOOL state)
{
	greenMode = state;
}

void Drawable2DCloud::SetCloudColor(Tcolor *color)
{
	memcpy(&litCloudColor,color,sizeof(Tcolor));
}
