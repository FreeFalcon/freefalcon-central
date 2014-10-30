/***************************************************************************\
    Clip2D.cpp
    Scott Randolph
    October 8, 1997

    This is a portion of the implemention for Render2D (see Render2D.h)
 These function provides 2D viewport clipping services.
\***************************************************************************/
#include <cISO646>
#include "Render2D.h"


// The use of the global storage defined here introduces a requirement that only
// one thread at a time do clipping.  If this requirment is unacceptable, this
// storage should become a member of the Render2D class.  Not doing so now
// saves us one pointer indirection per use of this storage (ie: this->)
static const int MAX_VERT_LIST  = 32; // (2 x largest number of verts in input fan)
static TwoDVertex extraVerts[MAX_VERT_LIST]; // Used to hold temporaty vertices
static int extraVertCount; // created by clipping.

extern int g_nGfxFix; // MN

/***************************************************************************\
 Set the clip flags for the given vertex.
\***************************************************************************/
void Render2D::SetClipFlags(TwoDVertex* vert)
{
    vert->clipFlag = ON_SCREEN;

    if (vert->x < leftPixel) vert->clipFlag or_eq CLIP_LEFT;
    else if (vert->x > rightPixel) vert->clipFlag or_eq CLIP_RIGHT;

    if (vert->y < topPixel) vert->clipFlag or_eq CLIP_TOP;
    else if (vert->y > bottomPixel) vert->clipFlag or_eq CLIP_BOTTOM;
}

/***************************************************************************
 Given a list of vertices which make up a fan, clip them to the
 top, bottom, left, right, and near planes.  Then draw the resultant
 polygon.
***************************************************************************/
void Render2D::ClipAndDraw2DFan(TwoDVertex** vertPointers, unsigned count, bool gifPicture)
{
    TwoDVertex **v, **p, **lastIn, **nextOut;
    TwoDVertex **inList, **outList, **temp;
    TwoDVertex *vertList1[MAX_VERT_LIST]; // Used to hold poly vert pointer lists
    TwoDVertex *vertList2[MAX_VERT_LIST]; // Used to hold poly vert pointer lists
    DWORD clipTest = 0;

    ShiAssert(vertPointers);
    ShiAssert(count >= 3);

    // Initialize the vertex buffers
    outList = vertList1;
    lastIn = vertPointers + count;

    for (nextOut = outList; vertPointers < lastIn; nextOut++)
    {
        clipTest or_eq (*vertPointers)->clipFlag;
        *nextOut = (*vertPointers++);
    }

    inList = vertList2;
    extraVertCount = 0;


    // Note:  we handle only leading and trailing culled triangles.  If
    // a more complicated non-planar polygon with interior triangles culled is
    // presented, too many triangles will get culled.  To handle that case, we'd
    // have to check all triangles instead of stopping after the second reject loop below.
    // If a new set of un-culled triangles was encountered, we'd have to make a new polygon
    // and resubmit it.
    if (gifPicture /*or g_nGfxFix bitand 0x08*/)  // removed again, caused AG radar not to be displayed on Matrox G400
    {
        temp = inList;
        inList = outList;
        outList = temp;
        lastIn = nextOut - 1;
        nextOut = outList;

        // We only support one flavor of clipping right now. The other version would just
        // be this same code repeated with inverted compare signs.
        // ShiAssert( CullFlag == CULL_ALLOW_CW );

        // Always copy the vertex at the root of the fan
        *nextOut++ = *inList;

        for (p = &inList[0], v = &inList[1]; v < lastIn; v++)
        {
            // Only clockwise triangles are accepted
            if ((((*(v + 1))->y - (*v)->y)) * (((*p)->x - (*v)->x)) <
                (((*(v + 1))->x - (*v)->x)) * (((*p)->y - (*v)->y)))
            {
                // Accept
                break;
            }
        }

        *nextOut++ = *v;

        for (p, v; v < lastIn; v++)
        {
            // Only clockwise triangles are accepted
            if ((((*(v + 1))->y - (*v)->y)) * (((*p)->x - (*v)->x)) >=
                (((*(v + 1))->x - (*v)->x)) * (((*p)->y - (*v)->y)))
            {
                // Reject
                break;
            }

            *nextOut++ = *(v + 1);
        }

        ShiAssert(nextOut - outList <= MAX_VERT_LIST);

        if (nextOut - outList <= 2)  return;
    }




    else // do the old code


    {


        // Clip to the bottom plane
        if (clipTest bitand CLIP_BOTTOM)
        {
            temp = inList;
            inList = outList;
            outList = temp;
            lastIn = nextOut - 1;
            nextOut = outList;

            for (p = lastIn, v = &inList[0]; v <= lastIn; v++)
            {

                // If the edge between this vert and the previous one crosses the line, trim it
                if (((*p)->clipFlag xor (*v)->clipFlag) bitand CLIP_BOTTOM)
                {
                    ShiAssert(extraVertCount < MAX_VERT_LIST);
                    *nextOut = &extraVerts[extraVertCount];
                    extraVertCount++;
                    IntersectBottom(*p, *v, *nextOut++);
                }

                // If this vert isn't clipped, use it
                if ( not ((*v)->clipFlag bitand CLIP_BOTTOM))
                {
                    *nextOut++ = *v;
                }

                p = v;
            }

            ShiAssert(nextOut - outList <= MAX_VERT_LIST);

            if (nextOut - outList <= 2)  return;
        }


        // Clip to the top plane
        if (clipTest bitand CLIP_TOP)
        {
            temp = inList;
            inList = outList;
            outList = temp;
            lastIn = nextOut - 1;
            nextOut = outList;

            for (p = lastIn, v = &inList[0]; v <= lastIn; v++)
            {

                // If the edge between this vert and the previous one crosses the line, trim it
                if (((*p)->clipFlag xor (*v)->clipFlag) bitand CLIP_TOP)
                {
                    ShiAssert(extraVertCount < MAX_VERT_LIST);
                    *nextOut = &extraVerts[extraVertCount];
                    extraVertCount++;
                    IntersectTop(*p, *v, *nextOut++);
                }

                // If this vert isn't clipped, use it
                if ( not ((*v)->clipFlag bitand CLIP_TOP))
                {
                    *nextOut++ = *v;
                }

                p = v;
            }

            ShiAssert(nextOut - outList <= MAX_VERT_LIST);

            if (nextOut - outList <= 2)  return;
        }


        // Clip to the right plane
        if (clipTest bitand CLIP_RIGHT)
        {
            temp = inList;
            inList = outList;
            outList = temp;
            lastIn = nextOut - 1;
            nextOut = outList;

            for (p = lastIn, v = &inList[0]; v <= lastIn; v++)
            {

                // If the edge between this vert and the previous one crosses the line, trim it
                if (((*p)->clipFlag xor (*v)->clipFlag) bitand CLIP_RIGHT)
                {
                    ShiAssert(extraVertCount < MAX_VERT_LIST);
                    *nextOut = &extraVerts[extraVertCount];
                    extraVertCount++;
                    IntersectRight(*p, *v, *nextOut++);
                }

                // If this vert isn't clipped, use it
                if ( not ((*v)->clipFlag bitand CLIP_RIGHT))
                {
                    *nextOut++ = *v;
                }

                p = v;
            }

            ShiAssert(nextOut - outList <= MAX_VERT_LIST);

            if (nextOut - outList <= 2)  return;
        }


        // Clip to the left plane
        if (clipTest bitand CLIP_LEFT)
        {
            temp = inList;
            inList = outList;
            outList = temp;
            lastIn = nextOut - 1;
            nextOut = outList;

            for (p = lastIn, v = &inList[0]; v <= lastIn; v++)
            {

                // If the edge between this vert and the previous one crosses the line, trim it
                if (((*p)->clipFlag xor (*v)->clipFlag) bitand CLIP_LEFT)
                {
                    ShiAssert(extraVertCount < MAX_VERT_LIST);
                    *nextOut = &extraVerts[extraVertCount];
                    extraVertCount++;
                    IntersectLeft(*p, *v, *nextOut++);
                }

                // If this vert isn't clipped, use it
                if ( not ((*v)->clipFlag bitand CLIP_LEFT))
                {
                    *nextOut++ = *v;
                }

                p = v;
            }

            ShiAssert(nextOut - outList <= MAX_VERT_LIST);

            if (nextOut - outList <= 2)  return;
        }


    }

    // Finally draw the resultant polygon
    // OW
#if 0
    context.Primitive(MPR_PRM_TRIFAN,
                      MPR_VI_COLOR bitor MPR_VI_TEXTURE,
                      (unsigned short)(nextOut - outList), sizeof(MPRVtxTexClr_t));

    for (v = outList; v < nextOut; v++)
    {
        context.StorePrimitiveVertexData(*v);
    }

#else
    context.DrawPrimitive(MPR_PRM_TRIFAN, MPR_VI_COLOR bitor MPR_VI_TEXTURE,
                          (unsigned short)(nextOut - outList), (MPRVtxTexClr_t **) outList);
#endif
}


inline void InterpolateColorAndTex(TwoDVertex *v1, TwoDVertex *v2, TwoDVertex *v, float t)
{
    // Compute the interpolated color and texture coordinates
    v->r = v1->r + t * (v2->r - v1->r);
    v->g = v1->g + t * (v2->g - v1->g);
    v->b = v1->b + t * (v2->b - v1->b);
    v->a = v1->a + t * (v2->a - v1->a);

    v->u = v1->u + t * (v2->u - v1->u);
    v->v = v1->v + t * (v2->v - v1->v);
}


// This function is expected to be called first in the clipping chain
void Render2D::IntersectBottom(TwoDVertex *v1, TwoDVertex *v2, TwoDVertex *v)
{
    float t;

    // Compute the parametric location of the intersection of the edge and the clip plane
    t = (bottomPixel - v1->y) / (v2->y - v1->y);
    ShiAssert((t >= -0.001f) and (t <= 1.001f));

    // Compute the camera space intersection point
    v->y = bottomPixel;
    v->x = v1->x + t * (v2->x - v1->x);

    // Compute the interpolated color and texture coordinates
    InterpolateColorAndTex(v1, v2, v, t);


    // Now determine if the point is out to the sides
    if (v->x < leftPixel)
    {
        v->clipFlag = CLIP_LEFT;
    }
    else if (v->x > rightPixel)
    {
        v->clipFlag = CLIP_RIGHT;
    }
    else
    {
        v->clipFlag = ON_SCREEN;
    }
}


// This function is expected to be called second in the clipping chain
// (ie: after bottom clipping, but before horizontal)
void Render2D::IntersectTop(TwoDVertex *v1, TwoDVertex *v2, TwoDVertex *v)
{
    float t;

    // Compute the parametric location of the intersection of the edge and the clip plane
    t = (topPixel - v1->y) / (v2->y - v1->y);
    ShiAssert((t >= -0.001f) and (t <= 1.001f));

    // Compute the camera space intersection point
    v->y = topPixel;
    v->x = v1->x + t * (v2->x - v1->x);

    // Compute the interpolated color and texture coordinates
    InterpolateColorAndTex(v1, v2, v, t);


    // Now determine if the point is out to the sides
    if (v->x < leftPixel)
    {
        v->clipFlag = CLIP_LEFT;
    }
    else if (v->x > rightPixel)
    {
        v->clipFlag = CLIP_RIGHT;
    }
    else
    {
        v->clipFlag = ON_SCREEN;
    }
}


// This function is expected to be called third in the clipping chain
// (ie: after vertical clipping is complete, but before the other side is done)
void Render2D::IntersectRight(TwoDVertex *v1, TwoDVertex *v2, TwoDVertex *v)
{
    float t;

    // Compute the parametric location of the intersection of the edge and the clip plane
    t = (rightPixel - v1->x) / (v2->x - v1->x);
    ShiAssert((t >= -0.001f) and (t <= 1.001f));

    // Compute the camera space intersection point
    v->x = rightPixel;
    v->y = v1->y + t * (v2->y - v1->y);
    v->clipFlag = ON_SCREEN;

    // Compute the interpolated color and texture coordinates
    InterpolateColorAndTex(v1, v2, v, t);
}


// This function is expected to be called fourth in the clipping chain
// (ie: last)
void Render2D::IntersectLeft(TwoDVertex *v1, TwoDVertex *v2, TwoDVertex *v)
{
    float t;

    // Compute the parametric location of the intersection of the edge and the clip plane
    t = (leftPixel - v1->x) / (v2->x - v1->x);
    ShiAssert((t >= -0.001f) and (t <= 1.001f));

    // Compute the camera space intersection point
    v->x = leftPixel;
    v->y = v1->y + t * (v2->y - v1->y);
    v->clipFlag = ON_SCREEN;

    // Compute the interpolated color and texture coordinates
    InterpolateColorAndTex(v1, v2, v, t);
}
