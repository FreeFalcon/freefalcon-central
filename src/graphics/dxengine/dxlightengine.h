#pragma once

#include <ddraw.h>
#include <d3d.h>
#include <d3dxcore.h>
#include <d3dxmath.h>
#include "DxDefines.h"
#include "DXVbManager.h"

#define	MAX_SAMETIME_LIGHTS				7					// Max Dynamic Lights on at same time
#define	MAX_DYNAMIC_LIGHTS				64					// Number of maximum Dynamic lights
#define	DYNAMIC_LIGHT_INSIDE_RANGE		50000.0f				// Range in Feets under which Dynamic Lights are considered


class	CDXLightElement
{
public:
	_MM_ALIGN16 XMMVector		Pos;
	bool						On;
	DXLightFlagsType			Flags;
	float						CameraDistance;	
	DWORD						LightID;
	D3DLIGHT7					Light;
	float						alphaX, alphaY, phi;
};



typedef	struct	{
	DWORD	Index;
	float	Distance;
} LightIndexType;

class	CDXLight
{
public:
	void Setup(IDirect3DDevice7	*pD3DD, IDirect3D7	*pD3D);
	DWORD	AddDynamicLight(DWORD ID, DXLightType *Light, D3DXMATRIX *RotMatrix, D3DVECTOR *Pos, float Range);
	void	ResetLightsList(void);
	void	UpdateDynamicLights(DWORD ID, D3DVECTOR *Pos, float Radius);
	static	bool LightsToOn[MAX_DYNAMIC_LIGHTS];
	void	EnableMappedLights(void);

private:

	static	CDXLightElement		LightList[MAX_DYNAMIC_LIGHTS];
	static	IDirect3DDevice7	*m_pD3DD;
	static	IDirect3D7			*m_pD3D;
	static	LightIndexType		SwitchedList[7];
	static	DWORD				LightID, DynamicLights;
	static	float				MaxRange;
	static	bool				LightsLoaded;

};



extern	CDXLight	TheLightEngine;
