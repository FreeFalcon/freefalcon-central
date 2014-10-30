#include "TimeMgr.h"
#include "TOD.h"
#include "RenderOW.h"
#include "RViewPnt.h"
#include "Tex.h"
#include "falclib/include/fakerand.h"
#include "Drawtrcr.h"
#include "Draw2d.h"

#include "Graphics/DXEngine/DXTools.h"
#include "Graphics/DXEngine/DXDefines.h"
#include "Graphics/DXEngine/DXEngine.h"
#include "Graphics/DXEngine/DXVBManager.h"

#ifdef USE_SH_POOLS
MEM_POOL DrawableTracer::pool;
#endif

// bleh
extern int sGreenMode, gameCompressionRatio;

/***************************************************************************\
    Initialize a tracer
\***************************************************************************/
DXDrawableTracer::DXDrawableTracer(void) : DrawableTracer()
{
}

/***************************************************************************\
    Initialize a tracer
\***************************************************************************/
DXDrawableTracer::DXDrawableTracer(float w) : DrawableTracer(w)
{
}

/***************************************************************************\
    Initialize a tracer
\***************************************************************************/
DXDrawableTracer::DXDrawableTracer(Tpoint *p, float w) : DrawableTracer(p, w)
{
}


/***************************************************************************\
    Remove an instance of a tracer
\***************************************************************************/
DXDrawableTracer::~DXDrawableTracer(void)
{
}


/***************************************************************************\
    Remove an instance of a tracer
\***************************************************************************/
void DXDrawableTracer::Update(Tpoint *head, Tpoint *tail)
{
    position = *head;
    tailEnd = *tail;
}


/***************************************************************************\
    Draw this segmented trail on the given renderer.
\***************************************************************************/
//void DrawableTracer::Draw( class RenderOTW *renderer, int LOD )
void DXDrawableTracer::Draw(class RenderOTW *renderer, int)
{
    D3DVECTOR v0, v1, v2, v3, v4, v5;
    int lineColor, LineEndColor;
    float DetailLevel;
    D3DVECTOR Vector;


    //COUNT_PROFILE("Tracers Nr");
    //START_PROFILE("Tracers Time");


    // COBRA - RED - Tracers are updated on by the Gun Exec... this makes flying tracers to freeze
    // if no more 'driven' by the gun EXEC... they appear stopped at midair
    if (LastPos.x == position.x and LastPos.z == position.z and LastPos.y == position.y and gameCompressionRatio)
    {
        if (parentList) parentList->RemoveMe();

        return;
    }

    // Get the last position for next comparison
    LastPos = position;

    // Get the Detail level of the drawable
    DetailLevel = TheDXEngine.GetDetailLevel((D3DVECTOR*)&position, TRACER_VISIBLE_DISTANCE) / radius;

    // Too much far to draw it...
    if (DetailLevel > 1.0f)
    {
        //STOP_PROFILE("Tracers Time");
        return;
    }

    // Alpha Check
    if (alpha > 1.0f) alpha = 1.0f;

    // now Alpha is proportional to distance
    float LineAlpha =/*alpha * */(1 - DetailLevel * 0.2f);


    // Set the Colour
    lineColor = ((unsigned int)(LineAlpha * 255.0f) << 24)       + // alpha
                ((unsigned int)(r * 255.0f) << 16) + // blue
                ((unsigned int)(g * 255.0f) << 8)  + // green
                ((unsigned int)(b * 255.0f));   // red

    LineEndColor = ((unsigned int)(LineAlpha * 32.0f) << 24)       + // alpha
                    ((unsigned int)(r * 255.0f) << 16) + // blue
                    ((unsigned int)(g * 255.0f) << 8)  + // green
                    ((unsigned int)(b * 255.0f));   // red

    if (DetailLevel > 0.15f)
    {
        TheDXEngine.Draw3DPoint((D3DVECTOR*)&position, lineColor, EMISSIVE);
        //STOP_PROFILE("Tracers Time");
        return;
    }

    if (DetailLevel > 0.03f)
    {
        TheDXEngine.Draw3DLine((D3DVECTOR*)&position, (D3DVECTOR*)&tailEnd, lineColor, LineEndColor, EMISSIVE);
        //STOP_PROFILE("Tracers Time");
        return;
    }

    // Get the direction vector
    Vector.x = tailEnd.x - position.x;
    Vector.y = tailEnd.y - position.y;
    Vector.z = tailEnd.z - position.z;

    // Normalize the Direction vector
    float k = sqrtf(Vector.x * Vector.x + Vector.y * Vector.y + Vector.z * Vector.z);
    Vector.x /= k;
    Vector.y /= k;
    Vector.z /= k;

    v0 = *(D3DVECTOR*)&position;
    v5 = *(D3DVECTOR*)&tailEnd;

    D3DVECTOR mid(Vector);
    mid.x *= 0.2f * k;
    mid.y *= 0.2f * k;
    mid.z *= 0.2f * k;
    v1.z = mid.z + radius / 2 * Vector.y;
    v1.x = mid.x + radius / 2 * Vector.z;
    v1.y = mid.y + radius / 2 * Vector.x;

    v2.z = mid.z - radius / 2 * Vector.y;
    v2.x = mid.x - radius / 2 * Vector.z;
    v2.y = mid.y - radius / 2 * Vector.x;

    v3.z = mid.z + radius / 2 * Vector.x;
    v3.y = mid.y + radius / 2 * Vector.z;
    v3.x = mid.x + radius / 2 * Vector.y;

    v4.z = mid.z - radius / 2 * Vector.x;
    v4.y = mid.y - radius / 2 * Vector.z;
    v4.x = mid.x - radius / 2 * Vector.y;

    v1.x += position.x;
    v1.y += position.y;
    v1.z += position.z;
    v2.x += position.x;
    v2.y += position.y;
    v2.z += position.z;
    v3.x += position.x;
    v3.y += position.y;
    v3.z += position.z;
    v4.x += position.x;
    v4.y += position.y;
    v4.z += position.z;

    TheDXEngine.Draw3DLine((D3DVECTOR*)&v0, (D3DVECTOR*)&v5, lineColor, LineEndColor, EMISSIVE);
    TheDXEngine.Draw3DLine((D3DVECTOR*)&v0, (D3DVECTOR*)&v1, lineColor, LineEndColor, EMISSIVE);
    TheDXEngine.Draw3DLine((D3DVECTOR*)&v0, (D3DVECTOR*)&v2, lineColor, LineEndColor, EMISSIVE);
    TheDXEngine.Draw3DLine((D3DVECTOR*)&v0, (D3DVECTOR*)&v3, lineColor, LineEndColor, EMISSIVE);
    TheDXEngine.Draw3DLine((D3DVECTOR*)&v0, (D3DVECTOR*)&v4, lineColor, LineEndColor, EMISSIVE);
    //STOP_PROFILE("Tracers Time");

}

