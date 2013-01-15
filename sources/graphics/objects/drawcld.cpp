/***************************************************************************
    DrawCLD.cpp
    Miro "Jammer" Torrielli
    08Nov03

	- Drawable cumulus
***************************************************************************/
#include "RenderOW.h"
#include "Matrix.h"
#include "TOD.h"
#include "Tex.h"
#include "DrawCLD.h"
#include "RealWeather.h"

extern int g_nGfxFix;

#ifdef USE_SH_POOLS
MEM_POOL Drawable3DCloud::pool;
#endif

BOOL Drawable3DCloud::greenMode	= FALSE;
Tcolor Drawable3DCloud::litCloudColor = { 0.f };

//static const float TEX_UV_LSB = 1.f/1024.f;
//static const float TEX_UV_MIN = TEX_UV_LSB;
//static const float TEX_UV_MAX = 1.f-TEX_UV_LSB;

Drawable3DCloud::Drawable3DCloud() : DrawableObject(1.0f){
	drawClassID = realClouds;
	radius = realWeather->puffRadius;
}

Drawable3DCloud::~Drawable3DCloud(){
}

void Drawable3DCloud::Update(Tpoint *worldPos, int txtIndex){
	memcpy(&position,worldPos,sizeof(Tpoint));
	cloudTexture = realWeather->CumulusTextures;//[txtIndex];
}

void Drawable3DCloud::Draw(class RenderOTW *renderer, int){
	// RED - LINEAR FOG - Remove the Clouds under Overcast layer...
	if(!(realWeather->weatherCondition == FAIR))/* || (realWeather->weatherCondition > FAIR &&
	(-realWeather->viewerZ) > (-realWeather->stratusZ) && (-realWeather->viewerZ) < (-realWeather->stratusZ)+(realWeather->stratusDepth))))*/
	{
		return;
	}

	float minFog;
	Tpoint os,pv;
	ThreeDVertex v0,v1,v2,v3;

	renderer->TransformPointToView(&position,&pv);

	os.x =  0.f;
	os.y = -radius * 2.f;
	os.z = -radius * 2.f;
	renderer->TransformBillboardPoint(&os,&pv,&v0);

	os.x =  0.f;
	os.y =  radius * 2.f;
	os.z = -radius * 2.f;
	renderer->TransformBillboardPoint(&os,&pv,&v1);

	os.x =  0.f;
	os.y =  radius * 2.f;
	os.z =  radius * 2.f;
	renderer->TransformBillboardPoint(&os,&pv,&v2);

	os.x =  0.f;
	os.y = -radius * 2.f;
	os.z =  radius * 2.f;
	renderer->TransformBillboardPoint(&os,&pv,&v3);

	v0.u = TEX_UV_MIN, v0.v = TEX_UV_MIN, v0.q = v0.csZ * Q_SCALE;
	v1.u = TEX_UV_MAX, v1.v = TEX_UV_MIN, v1.q = v1.csZ * Q_SCALE;
	v2.u = TEX_UV_MAX, v2.v = TEX_UV_MAX, v2.q = v2.csZ * Q_SCALE;
	v3.u = TEX_UV_MIN, v3.v = TEX_UV_MAX, v3.q = v3.csZ * Q_SCALE;
	
	if(greenMode){
		minFog = .2f;

		v0.r = v1.r = v2.r = v3.r = 0.f;
		v0.g = v1.g = v2.g = v3.g = .4f;
		v0.b = v1.b = v2.b = v3.b = 0.f;
	}
	else {
		minFog = .4f;

		v0.r = v1.r = litCloudColor.r;
		v2.r = v3.r = litCloudColor.r * .88f;

		v0.g = v1.g = litCloudColor.g;
		v2.g = v3.g = litCloudColor.g * .88f;

		v0.b = v1.b = litCloudColor.b;
		v2.b = v3.b = litCloudColor.b * .88f;
	}

	v0.a = 1.f - min(max(renderer->GetRangeOnlyFog(v0.csZ),minFog),1.f);
	v1.a = 1.f - min(max(renderer->GetRangeOnlyFog(v1.csZ),minFog),1.f);
	v2.a = 1.f - min(max(renderer->GetRangeOnlyFog(v2.csZ),minFog),1.f);
	v3.a = 1.f - min(max(renderer->GetRangeOnlyFog(v3.csZ),minFog),1.f);

	if(v0.csZ > 50.f){
		renderer->context.RestoreState(STATE_ALPHA_TEXTURE_GOURAUD);
	}
	else {
		renderer->context.RestoreState(STATE_ALPHA_TEXTURE_GOURAUD_PERSPECTIVE);
	}

	renderer->context.SelectTexture1(cloudTexture.TexHandle());
	renderer->DrawSquare(&v0,&v1,&v2,&v3,CULL_ALLOW_ALL,(g_nGfxFix > 0));
}

void Drawable3DCloud::SetGreenMode(BOOL state){
	greenMode = state;
}

void Drawable3DCloud::SetCloudColor(Tcolor *color){
	memcpy(&litCloudColor,color,sizeof(Tcolor));
}
