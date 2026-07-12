#include <math.h>
#include "../include/ObjectInstance.h"
#include "dxdefines.h"
#include "DXVBManager.h"
#include "mmsystem.h"
#include "dxengine.h"
#include "../include/ObjectLOD.h"
#include "DXTools.h"
#include "DXLightEngine.h"
#include "d3d11/D3D11Renderer.h"	// #28: GpuLightCPU + SetLights (per-object dynamic light)
extern bool g_bUseD3D11;	// PHASE 4: D3D7 lighting replaced by a shader cbuffer (SetLights)
extern bool g_bUseGpu;		// #DX12 п.4: dynamic object lighting on the active renderer (D3D11 || D3D12)
// #28: current-frame sun+ambient (filled in CDXEngine::FlushBuffers).
extern D3D11Renderer::GpuLightCPU g_d3d11Sun;
extern float g_d3d11Amb[4];

#ifndef DEBUG_ENGINE

#endif


/////////////////////////////////////////// LIGHT ENGINE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

CDXLight TheLightEngine;

_MM_ALIGN16 CDXLightElement CDXLight::LightList[MAX_DYNAMIC_LIGHTS];
// #34 C1: CDXLight::m_pD3DD/m_pD3D (D3D7 device) removed.
LightIndexType CDXLight::SwitchedList[MAX_SAMETIME_LIGHTS];
DWORD CDXLight::LightID;
DWORD CDXLight::DynamicLights;
float CDXLight::MaxRange;
bool CDXLight::LightsToOn[MAX_DYNAMIC_LIGHTS];
bool CDXLight::LightsLoaded;



void CDXLight::Setup()
{
    // #34 C1: no D3D7 device to store.
    // clear the light list
    memset(LightList, 0x00, sizeof(LightList));

    // Reset the Light List
    ResetLightsList();
}


// Thi function resets and inits the lights list parameters
void CDXLight::ResetLightsList(void)
{

#ifdef LIGHT_ENGINE_DEBUG
    REPORT_VALUE("LIGHTS", DynamicLights);
#endif

    for (int idx = 0; idx < MAX_DYNAMIC_LIGHTS; idx++)
    {
        // setup max distance
        LightList[idx].CameraDistance = DYNAMIC_LIGHT_INSIDE_RANGE + 1;
        // and if light is on switch it off
        // Artscout - 2026: #34 dropped the dead D3D7 m_pD3DD->LightEnable (D3D11 uploads lights
        // per-object via SetLights; there are no fixed-function light slots).
        LightList[idx].On = false;
        LightsToOn[idx] = false;
    }

    // no lights in the list
    DynamicLights = 0;
    // reset max light distance
    MaxRange = DYNAMIC_LIGHT_INSIDE_RANGE + 1;
    // no lights loaded in DX
    LightsLoaded = false;

}



// This function adds a light in the Dynamic Lights List
DWORD CDXLight::AddDynamicLight(DWORD ID, DXLightType *Light, D3DXMATRIX *RotMatrix, D3DVECTOR *Pos, float Range)
{
    DWORD Index = 0;
    float ActualRange = 0.0f;
    bool Assigned = false;

    // Do not add static lights... they r just use to pre compute emissive colours
    if (Light->Flags.Static) return NULL;


    //if already over the higer available range of lights in list, return
    if (Range >= MaxRange) return NULL;

    // else look where to place it
    while (Index < MAX_DYNAMIC_LIGHTS)
    {
        // if found the Light at te Max Range
        if (LightList[Index].CameraDistance == MaxRange and ( not Assigned))
        {
            // substitute with new light
            LightList[Index].Light = Light->Light;
            // apply the owning of the light
            LightList[Index].Flags = Light->Flags;
            // apply the ID of the light
            LightList[Index].LightID = ID;
            // transform the Direction
            D3DXVec3TransformCoord((D3DXVECTOR3*)&LightList[Index].Light.dvDirection, (D3DXVECTOR3*)&LightList[Index].Light.dvDirection, RotMatrix);
            // transform the Position
            D3DXVec3TransformCoord((D3DXVECTOR3*)&LightList[Index].Light.dvPosition, (D3DXVECTOR3*)&LightList[Index].Light.dvPosition, RotMatrix);
            // and translate it
            LightList[Index].Light.dvPosition.x += Pos->x;
            LightList[Index].Light.dvPosition.y += Pos->y;
            LightList[Index].Light.dvPosition.z += Pos->z;

            // Calcualtions for the Cone if SPOT Light
            if (Light->Light.dltType == D3DLIGHT_SPOT)
            {
                // The Light Cone Angle
                LightList[Index].phi = Light->Light.dvPhi / 2.0f;
                LightList[Index].alphaX = atan2(LightList[Index].Light.dvDirection.x, LightList[Index].Light.dvDirection.y);
                LightList[Index].alphaY = atan2(Light->Light.dvDirection.z, sqrtf(LightList[Index].Light.dvDirection.y * LightList[Index].Light.dvDirection.y + LightList[Index].Light.dvDirection.x * LightList[Index].Light.dvDirection.x));
            }

            //update record XMM position
            *(D3DVECTOR*)&LightList[Index].Pos.d3d = LightList[Index].Light.dvPosition;
            // assign new distance
            LightList[Index].CameraDistance = Range;
            // Light has been assigned
            Assigned = true;
        }

        // if new Long range, assign it
        if (LightList[Index].CameraDistance > ActualRange) ActualRange = LightList[Index].CameraDistance;

        // if already out of range exit here
        if (ActualRange > DYNAMIC_LIGHT_INSIDE_RANGE) break;

        // next light
        Index++;
    }

    // number of lights
    DynamicLights = Index;

    // REPORT_VALUE("Dynamic Lights", DynamicLights);
    // set up the longest range left in list
    MaxRange = ActualRange;
    // return the light ID for this object
    return LightID;

}


#ifdef DEBUG_LOD_ID
extern DWORD gDebugLodID;
extern char TheLODNames[10000][32];
#endif

// This function switch on the nearest lights to an object of a certain radius
void CDXLight::UpdateDynamicLights(DWORD ID, D3DVECTOR *pos, float Radius)
{
    if (g_bUseGpu)
    {
        // #28 D3D11: the per-object light set = the sun (light 0) + the nearest active
        // point lamps (flashes/explosions) within their range of the object. Attenuation
        // is computed by the shader (Params.x=range). Then SetLights -> cbLights for this object.
        if ( not g_pRenderer) return;
        const int MAXL = 8;	// = MAX_LIGHTS in FFEmu.hlsl
        D3D11Renderer::GpuLightCPU lights[MAXL];
        int n = 0;
        lights[n++] = g_d3d11Sun;	// sun

        for (DWORD i = 0; i < DynamicLights and n < MAXL; ++i)
        {
            D3DLIGHT7 &L = LightList[i].Light;
            const float dx = L.dvPosition.x - pos->x;
            const float dy = L.dvPosition.y - pos->y;
            const float dz = L.dvPosition.z - pos->z;
            const float range = L.dvRange + Radius;
            if (dx * dx + dy * dy + dz * dz > range * range) continue;	// too far

            D3D11Renderer::GpuLightCPU &g = lights[n++];
            ZeroMemory(&g, sizeof(g));
            g.Position[0] = L.dvPosition.x; g.Position[1] = L.dvPosition.y; g.Position[2] = L.dvPosition.z;
            g.Color[0] = L.dcvDiffuse.r;    g.Color[1] = L.dcvDiffuse.g;    g.Color[2] = L.dcvDiffuse.b;
            g.Params[0] = (L.dvRange > 1.0f) ? L.dvRange : 1.0f;	// range (attenuation)
            g.Params[1] = 1.0f;	// point
        }
        g_pRenderer->SetLights(g_d3d11Amb, n, lights, sizeof(lights[0]));
        return;
    }
}


void CDXLight::EnableMappedLights(void)
{
    // Artscout - 2026: #34 D3D11 uploads per-object lights via SetLights; there are no
    // fixed-function light slots to enable/disable. Kept as a no-op for external callers.
}



// Artscout - 2026: #34 removed the dead D3D7 CDXEngine::AddDynamicLight(D3DLIGHT7*) (3-arg)
// and RemoveDynamicLights -- no callers (the live path is CDXLight::AddDynamicLight, 5-arg,
// D3D11 #28). They used the D3D7 device (m_pD3DD->SetLight/LightEnable).



// *** NO MORE USED  ***
// This function is the HardCoding for the PIT of the Taxi Spotlight
void CDXEngine::DrawOwnSpot(Trotation *Rotation)
{
    /* DXLightType OwnSpot;

     D3DXMATRIX RotMatrix;
    #ifndef DEBUG_ENGINE
     AssignPmatrixToD3DXMATRIX(&RotMatrix, Rotation);
    #endif


     // initialize it
     memset(&OwnSpot, 0, sizeof(OwnSpot));

     OwnSpot.Light.dcvDiffuse.r=OwnSpot.Light.dcvDiffuse.g=OwnSpot.Light.dcvDiffuse.b=1.0f;
     OwnSpot.Light.dcvSpecular.r=OwnSpot.Light.dcvSpecular.g=OwnSpot.Light.dcvSpecular.b=1.0f;
     OwnSpot.Light.dvRange=1500.0f;
     OwnSpot.Light.dvAttenuation0=0.1f;
     OwnSpot.Light.dvAttenuation1=0.01f;
     OwnSpot.Light.dltType=D3DLIGHT_SPOT;
     OwnSpot.Light.dvTheta=0.1f;
     OwnSpot.Light.dvPhi=1.3f;
     OwnSpot.Light.dvFalloff=1.0f;
     OwnSpot.Light.dvDirection.z=0.23f;
     OwnSpot.Light.dvDirection.y=0.0f;
     OwnSpot.Light.dvDirection.x=1.0f;

     OwnSpot.Flags.OwnLight=false;


     D3DVECTOR Pos(4, 0, 5);
     D3DXVec3TransformCoord((D3DXVECTOR3*)&Pos, (D3DXVECTOR3*)&Pos, &RotMatrix);
     TheLightEngine.AddDynamicLight(0, &OwnSpot, &RotMatrix, &Pos, 0);
    */
}
