/***************************************************************************\
    DrawShdw.h    Scott Randolph
    July 2, 1997

    Derived class to draw BSP'ed objects with shadows (specificly for aircraft)
\***************************************************************************/
#include "Matrix.h"
#include "RViewPnt.h"
#include "RenderOW.h"
#include "DrawShdw.h"
#include "tod.h"
#include "weather.h"
#include "RealWeather.h"


#ifdef USE_SH_POOLS
MEM_POOL DrawableShadowed::pool;
#endif



/***************************************************************************\
    Initialize a container for a BSP object to be drawn
\***************************************************************************/
DrawableShadowed::DrawableShadowed(int ID, const Tpoint *pos, const Trotation *rot, float s, int ShadowID)
    : DrawableBSP(ID, pos, rot, s), shadowInstance(ShadowID)
{
}



/***************************************************************************\
    Make sure the object is placed on the ground then draw it.
\***************************************************************************/
void DrawableShadowed::Draw(class RenderOTW *renderer, int LOD)
{
    //JAM 14May04
    // Draw our shadow if we're close to the ground
    // if (position.z > renderer->viewpoint->GetTerrainCeiling()) { // -Z is up
    if (position.z > renderer->viewpoint->GetGroundLevel(position.x, position.y) - 2000.f)
    {
        float yaw;
        float pitch;
        float roll;
        float sinYaw;
        float cosYaw;
        float sx;
        float sy;
        float       dZ; // COBRA - RED - Delta Z for Shadow
        float ds; // COBRA - RED - Delta CX for Shadows
        Tpoint pos, LightDir;
        Trotation rot;

        yaw = (float)atan2(orientation.M21, orientation.M11);
        pitch = (float) - asin(orientation.M31);
        roll = (float)atan2(orientation.M32, orientation.M33);
        sinYaw = (float)sin(yaw);
        cosYaw = (float)cos(yaw);

        TheTimeOfDay.GetLightDirection(&LightDir); // COBRA - RED - For Light direction

        pos.z = renderer->viewpoint->GetGroundLevel(position.x, position.y);

        // RED - Linear Fog - checvk if under visibility limit
        if (pos.z < realWeather->VisibleLimit())
        {
            dZ = fabs(position.z - pos.z); // Absolute distance ( not oriented, but who cares...???)
            pos.x = position.x - LightDir.x * dZ * (2 + LightDir.z); // COBRA - RED - Light Direction Casting
            pos.y = position.y - LightDir.y * dZ * (2 + LightDir.z); // COBRA - RED - Light Direction Casting

            ds = max(0.005f, 1.0f - dZ / 1000.0f); // COBRA - RED - Approxximate distance

            sx = max(0.3f, (float)fabs(cos(pitch)));
            sy = max(0.3f, (float)fabs(cos(roll)));

            rot.M11 = cosYaw;
            rot.M12 = -sinYaw;
            rot.M13 = 0.0f;
            rot.M21 = sinYaw;
            rot.M22 = cosYaw;
            rot.M23 = 0.0f;
            rot.M31 = 0.0f;
            rot.M32 = 0.0f;
            rot.M33 = 1.0f;

            /** sfr: We dont want to be affected by fog leftovers by others: blue shadow fix */
            TheStateStack.SetFog(1.0f, NULL);

            ((Render3D *)renderer)->context.setGlobalZBias(.02f);//(.009f);

            ShadowBSPRendering = true; // COBRA - RED - We are rendering a Shadow affected by TOD Light
            ds = ds * ds; // The cube of Distance is used for Shadow Alpha
            ShadowAlphaLevel = TheTimeOfDay.GetAmbientValue() * 3.0f * ds; // calculate the Shadow Alpha based on TOD Light and Distance

            if (ShadowAlphaLevel > 1.0f) ShadowAlphaLevel = 1.0f; // Limit Check

            // FRB - Almost no shadows when there is no sun (overcast or heavy overcast)
            if (realWeather->weatherCondition == INCLEMENT)
                ShadowAlphaLevel = 0.1f;
            else if (realWeather->weatherCondition == POOR)
                ShadowAlphaLevel = 0.3f;

            TheStateStack.DrawWarpedObject(&shadowInstance, &rot, &pos, sx, sy, 1.0f, instance.Radius());
            ShadowBSPRendering = false; // End of Shadow rendering

            ((Render3D *)renderer)->context.setGlobalZBias(0.f);
        }
    }

    // Tell our parent class to draw us now
    DrawableBSP::Draw(renderer, LOD);
}
