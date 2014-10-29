#include <math.h>
#include "../include/ObjectInstance.h"
#include "dxdefines.h"
#include "DXVBManager.h"
#include "mmsystem.h"
#include "dxengine.h"
#include "../include/ObjectLOD.h"
#include "DXTools.h"
#include "DXLightEngine.h"

#ifndef DEBUG_ENGINE

#endif


/////////////////////////////////////////// LIGHT ENGINE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

CDXLight TheLightEngine;

_MM_ALIGN16 CDXLightElement CDXLight::LightList[MAX_DYNAMIC_LIGHTS];
IDirect3DDevice7 *CDXLight::m_pD3DD;
IDirect3D7 *CDXLight::m_pD3D;
LightIndexType CDXLight::SwitchedList[MAX_SAMETIME_LIGHTS];
DWORD CDXLight::LightID;
DWORD CDXLight::DynamicLights;
float CDXLight::MaxRange;
bool CDXLight::LightsToOn[MAX_DYNAMIC_LIGHTS];
bool CDXLight::LightsLoaded;



void CDXLight::Setup(IDirect3DDevice7 *pD3DD, IDirect3D7 *pD3D)
{
    // assign the D3D pointers
    m_pD3DD = pD3DD;
    m_pD3D = pD3D;

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
        /*if(LightList[idx].On) */
        m_pD3DD->LightEnable(idx + 1, false);
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
    _MM_ALIGN16 XMMVector Pos, Acc, Square;
    DWORD idx, ldx, LightsInList = 0;
    float Dist, MaxDist = 0.0f;

    // setup the position for XMM use
    *(D3DVECTOR*)&Pos.d3d = *pos;

    // if 1st call in this rendering
    if ( not LightsLoaded)
    {
        // Enable the lights + 1 ( light #0 is the Sun )
        idx = 0;

        // while(idx<MAX_DYNAMIC_LIGHTS and LightList[idx].CameraDistance<=DYNAMIC_LIGHT_INSIDE_RANGE){
        while (idx < DynamicLights)
        {
            m_pD3DD->SetLight(idx + 1, &LightList[idx++].Light);
        }

        LightsLoaded = true;
    }

    //reset object own light list
    for (idx = 0; idx < MAX_SAMETIME_LIGHTS; idx++)
    {
        SwitchedList[idx].Distance = DYNAMIC_LIGHT_INSIDE_RANGE + 1.0f;
        SwitchedList[idx].Index = 0;
    }

    //for each light in list setup a distance list
    idx = 0;

    //while(idx<MAX_DYNAMIC_LIGHTS and LightList[idx].CameraDistance<=DYNAMIC_LIGHT_INSIDE_RANGE){
    while (idx < DynamicLights)
    {

        // Light defaults to OFF
        LightsToOn[idx] = false;

        // if this is a light illuminating ONLY THE OWNER, and we r not the owners, skip
        if (LightList[idx].Flags.OwnLight and LightList[idx].LightID not_eq ID)
        {
            idx++;
            continue;
        }

        // if this is a light illuminating NOT THE OWNER, and we r the owners, skip
        if (LightList[idx].Flags.NotSelfLight and LightList[idx].LightID == ID)
        {
            idx++;
            continue;
        }

        // get Vectors distance on X/Y/Z Axis and Square it for incoming use
        Acc.Xmm = _mm_sub_ps(Pos.Xmm, LightList[idx].Pos.Xmm);
        Square.Xmm = _mm_mul_ps(Acc.Xmm, Acc.Xmm);
        // Get the Distance
        Dist = sqrtf(Square.d3d.x + Square.d3d.y + Square.d3d.z);

        // If Object out of the Light range
        if ((Dist - Radius) > LightList[idx].Light.dvRange)
        {
            idx++;
            continue;
        }

        // Calculation for SPOT LIGHTs cone
        if (LightList[idx].Light.dltType == D3DLIGHT_SPOT)
        {

            // Get the Distance btw Light bitand Object
            //float dx=Pos.d3d.x-LightList[idx].Light.dvPosition.x;
            //float dy=Pos.d3d.y-LightList[idx].Light.dvPosition.y;
            //float dz=Pos.d3d.z-LightList[idx].Light.dvPosition.z;
            // Calculate Horizontal and Vertical Angle btw Light bitand Object
            float ax = atan2(Acc.d3d.x, Acc.d3d.y);
            float ay = atan2(Acc.d3d.z, sqrtf(Square.d3d.x + Square.d3d.y));

            // transform the angles in same Sign Domain of the light angles X/Y
            if (fabs(ax - LightList[idx].alphaX) > PI) ax += ax > LightList[idx].alphaX ? -2 * PI : 2 * PI;

            if (fabs(ay - LightList[idx].alphaY) > PI) ay += ax > LightList[idx].alphaY ? -2 * PI : 2 * PI;

            float lPhy = LightList[idx].phi;
            float laX = LightList[idx].alphaX;
            float laY = LightList[idx].alphaY;
            // Calculate the Angular Delta given by Object Radius
            float dPhi = atanf(Radius / Dist);

#ifdef LIGHT_ENGINE_DEBUG
            REPORT_VALUE("Max :", (int)(LightList[idx].alphaX * 180 / PI));
            REPORT_VALUE("Min :", (int)(LightList[idx].alphaY * 180 / PI));
#ifdef DEBUG_LOD_ID
            sprintf(TheLODNames[gDebugLodID], "%3.1f %3.1f", ax * 180 / PI, dPhi * 180 / PI);
#endif
#endif

            if (ax - dPhi > (laX + lPhy) or ax + dPhi < (laX - lPhy) or ay - dPhi > (laY + lPhy) or ay + dPhi < (laY - lPhy))
            {
                idx++;
                continue;
            }


        }

        Dist -= Radius;

        // if List still has a slot, add immediatly the light
        if (LightsInList < MAX_SAMETIME_LIGHTS)
        {
            // setup new distance and index in the list
            SwitchedList[LightsInList].Distance = Dist;
            SwitchedList[LightsInList++].Index = idx;
            // flag the light as going to be switched on
            LightsToOn[idx] = true;

            // and update the longest in list
            if (Dist > MaxDist) MaxDist = Dist;
        }
        else
        {
            // else only if more near than any light in list
            if (Dist < MaxDist)
            {
                // setup a temporary for the new Max Distance
                float TempDist = 0.0f;

                // check if lower distance than any in the object lites list
                for (ldx = 0; ldx < MAX_SAMETIME_LIGHTS; ldx++)
                {
                    // if distance is less
                    if (SwitchedList[ldx].Distance == MaxDist)
                    {
                        // disable the previous light
                        LightsToOn[SwitchedList[ldx].Index] = false;
                        // setup new distance and index in the list
                        SwitchedList[ldx].Distance = Dist;
                        SwitchedList[ldx].Index = idx;
                        // enable the new light
                        LightsToOn[idx] = true;
                    }

                    // check if new longest distance
                    if (SwitchedList[ldx].Distance > TempDist) TempDist = SwitchedList[ldx].Distance;
                }

                // update new Max Distance
                MaxDist = TempDist;
            }
        }

        idx++;
    }

    EnableMappedLights();
}


void CDXLight::EnableMappedLights(void)
{
    // OK...Go disable not usefull lights
    for (DWORD idx = 0; idx < DynamicLights; idx++)
    {
        // if light to be disabled
        if (( not LightsToOn[idx]) and LightList[idx].On)
        {
            m_pD3DD->LightEnable(idx + 1, false);
            LightList[idx].On = false;
        }
    }

    for (DWORD idx = 0; idx < DynamicLights; idx++)
    {
        if (LightsToOn[idx] and ( not LightList[idx].On))
        {
            m_pD3DD->LightEnable(idx + 1, true);
            LightList[idx].On = true;
        }
    }
}






// This function adds a light in the Dynamic Lights List
void CDXEngine::AddDynamicLight(D3DLIGHT7 *Light, D3DXMATRIX *RotMatrix, D3DVECTOR *Pos)
{
    // Only if space available in the list
    if (LightsNumber < MAX_DYNAMIC_LIGHTS)
    {
        // copy the light features
        DXLightsList[LightsNumber] = *Light;
        // transform the Direction
        D3DXVec3TransformCoord((D3DXVECTOR3*)&DXLightsList[LightsNumber].dvDirection, (D3DXVECTOR3*)&DXLightsList[LightsNumber].dvDirection, RotMatrix);
        // transform the Position
        D3DXVec3TransformCoord((D3DXVECTOR3*)&DXLightsList[LightsNumber].dvPosition, (D3DXVECTOR3*)&DXLightsList[LightsNumber].dvPosition, RotMatrix);
        // and translate it
        DXLightsList[LightsNumber].dvPosition.x += Pos->x;
        DXLightsList[LightsNumber].dvPosition.y += Pos->y;
        DXLightsList[LightsNumber].dvPosition.z += Pos->z;
        // Enable the light + 1 ( light #0 is the Sun )
        m_pD3DD->SetLight(LightsNumber + 1, &DXLightsList[LightsNumber]);
        m_pD3DD->LightEnable(LightsNumber + 1, true);
        // a New light in list
        LightsNumber++;
    }
}


// This function clears the Dynamic Lights List
void CDXEngine::RemoveDynamicLights(void)
{
    while (LightsNumber)
    {
        m_pD3DD->LightEnable(LightsNumber, false);
        LightsNumber--;
    }
}



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
