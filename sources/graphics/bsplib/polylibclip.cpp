/***************************************************************************\
    PolyLibClip.cpp
    Scott Randolph
    February 24, 1998

    Provides clipping functions for 3D polygons of various types.
\***************************************************************************/
#include "stdafx.h"
#include "StateStack.h"
#include "ColorBank.h"
#include "PolyLib.h"
#include "ClipFlags.h"


typedef struct ClipVert {
	int			xyz;
	int			rgba;
	int			I;
	Ptexcoord	uv;
} ClipVert;


// Intersect edge with z=near plane
// This function is expected to be called first in the clipping chain
static void IntersectNear( ClipVert *v1, ClipVert *v2, ClipVert *v, BOOL color, BOOL light, BOOL tex )
{
	float	x, y, z, t;

	// Compute the parametric location of the intersection of the edge and the clip plane
	t = (NEAR_CLIP_DISTANCE                      - TheStateStack.ClipInfoPool[v1->xyz].csZ) / 
		(TheStateStack.ClipInfoPool[v2->xyz].csZ - TheStateStack.ClipInfoPool[v1->xyz].csZ);
	ShiAssert( (t >= -0.001f) && (t <= 1.001f) );
	
	// Compute the camera space intersection point
	TheStateStack.ClipInfoPool[v->xyz].csZ = z = NEAR_CLIP_DISTANCE;

	TheStateStack.ClipInfoPool[v->xyz].csX = x = TheStateStack.ClipInfoPool[v1->xyz].csX +
		t * (TheStateStack.ClipInfoPool[v2->xyz].csX - TheStateStack.ClipInfoPool[v1->xyz].csX);
	TheStateStack.ClipInfoPool[v->xyz].csY = y = TheStateStack.ClipInfoPool[v1->xyz].csY +
		t * (TheStateStack.ClipInfoPool[v2->xyz].csY - TheStateStack.ClipInfoPool[v1->xyz].csY);

	// Now interpolate any other vertex parameters required
	if (color) {
		TheColorBank.ColorPool[v->rgba].r = TheColorBank.ColorPool[v1->rgba].r +
			t * (TheColorBank.ColorPool[v2->rgba].r - TheColorBank.ColorPool[v1->rgba].r);
		TheColorBank.ColorPool[v->rgba].g = TheColorBank.ColorPool[v1->rgba].g +
			t * (TheColorBank.ColorPool[v2->rgba].g - TheColorBank.ColorPool[v1->rgba].g);
		TheColorBank.ColorPool[v->rgba].b = TheColorBank.ColorPool[v1->rgba].b +
			t * (TheColorBank.ColorPool[v2->rgba].b - TheColorBank.ColorPool[v1->rgba].b);
		TheColorBank.ColorPool[v->rgba].a = TheColorBank.ColorPool[v1->rgba].a +
			t * (TheColorBank.ColorPool[v2->rgba].a - TheColorBank.ColorPool[v1->rgba].a);
	}
	if (light) {
		TheStateStack.IntensityPool[v->I] = TheStateStack.IntensityPool[v1->I] +
			t * (TheStateStack.IntensityPool[v2->I] - TheStateStack.IntensityPool[v1->I]);
	}
	if (tex) {
		v->uv.u = v1->uv.u + t * (v2->uv.u - v1->uv.u);
		v->uv.v = v1->uv.v + t * (v2->uv.v - v1->uv.v);
	}

	// Now determine if the point is out to the sides
	TheStateStack.ClipInfoPool[v->xyz].clipFlag  = GetHorizontalClipFlags( x, z );
	TheStateStack.ClipInfoPool[v->xyz].clipFlag |= GetVerticalClipFlags(   y, z );

	// Compute the screen space coordinates of the new point
	register float OneOverZ = 1.0f / z;
	TheStateStack.XformedPosPool[v->xyz].z = z;
	TheStateStack.XformedPosPool[v->xyz].x = TheStateStack.XtoPixel( x * OneOverZ );
	TheStateStack.XformedPosPool[v->xyz].y = TheStateStack.YtoPixel( y * OneOverZ );
}


// Compute the parametric location of the intersection of the ray with the edge indicated
// by the flag parameter
static inline float	ComputeT( float x, float y, float z, float dx, float dy, float dz, UInt32 flag )
{
	switch (flag) {
	  case CLIP_BOTTOM:
		return (y - z) / (dz - dy);
	  case CLIP_TOP:
		return (y + z) / (-dz - dy);
	  case CLIP_RIGHT:
		return (x - z) / (dz - dx);
	  case CLIP_LEFT:
		return (x + z) / (-dz - dx);
	  default:
		ShiWarning( "Bad clip type!" );
		return 1.0f;
	}
}


// Helper function which clips the segment against the edge indicated by the flag argument
static inline void IntersectSide( ClipVert *v1, ClipVert *v2, ClipVert *v, BOOL color, BOOL light, BOOL tex, UInt32 flag )
{
	float	x, y, z, t;
	float	dx, dy, dz;	

	// Compute the parametric location of the intersection of the edge and the clip plane
	dx = TheStateStack.ClipInfoPool[v2->xyz].csX - TheStateStack.ClipInfoPool[v1->xyz].csX;
	dy = TheStateStack.ClipInfoPool[v2->xyz].csY - TheStateStack.ClipInfoPool[v1->xyz].csY;
	dz = TheStateStack.ClipInfoPool[v2->xyz].csZ - TheStateStack.ClipInfoPool[v1->xyz].csZ;
	t = ComputeT(TheStateStack.ClipInfoPool[v1->xyz].csX, 
				 TheStateStack.ClipInfoPool[v1->xyz].csY, 
				 TheStateStack.ClipInfoPool[v1->xyz].csZ, 
				 dx,
				 dy,
				 dz,
				 flag );
	ShiAssert( (t >= -0.002f) && (t <= 1.002f) );
	
	// Compute the camera space intersection point
	TheStateStack.ClipInfoPool[v->xyz].csZ = z = TheStateStack.ClipInfoPool[v1->xyz].csZ + t * (dz);
	TheStateStack.ClipInfoPool[v->xyz].csX = x = TheStateStack.ClipInfoPool[v1->xyz].csX + t * (dx);	// Note: either dx or dy is used only once, so could
	TheStateStack.ClipInfoPool[v->xyz].csY = y = TheStateStack.ClipInfoPool[v1->xyz].csY + t * (dy);	// be avoided, but this way, the code is more standardized...

	// Now interpolate any other vertex parameters required
	if (color) {
		TheColorBank.ColorPool[v->rgba].r = TheColorBank.ColorPool[v1->rgba].r +
			t * (TheColorBank.ColorPool[v2->rgba].r - TheColorBank.ColorPool[v1->rgba].r);
		TheColorBank.ColorPool[v->rgba].g = TheColorBank.ColorPool[v1->rgba].g +
			t * (TheColorBank.ColorPool[v2->rgba].g - TheColorBank.ColorPool[v1->rgba].g);
		TheColorBank.ColorPool[v->rgba].b = TheColorBank.ColorPool[v1->rgba].b +
			t * (TheColorBank.ColorPool[v2->rgba].b - TheColorBank.ColorPool[v1->rgba].b);
		TheColorBank.ColorPool[v->rgba].a = TheColorBank.ColorPool[v1->rgba].a +
			t * (TheColorBank.ColorPool[v2->rgba].a - TheColorBank.ColorPool[v1->rgba].a);
	}
	if (light) {
		TheStateStack.IntensityPool[v->I] = TheStateStack.IntensityPool[v1->I] +
			t * (TheStateStack.IntensityPool[v2->I] - TheStateStack.IntensityPool[v1->I]);
	}
	if (tex) {
		v->uv.u = v1->uv.u + t * (v2->uv.u - v1->uv.u);
		v->uv.v = v1->uv.v + t * (v2->uv.v - v1->uv.v);
	}

	// Now determine if the point is out to the sides
	if (flag & (CLIP_TOP | CLIP_BOTTOM)) {
		TheStateStack.ClipInfoPool[v->xyz].clipFlag = GetHorizontalClipFlags( x, z );
	} else {
		TheStateStack.ClipInfoPool[v->xyz].clipFlag = ON_SCREEN;
	}

	// Compute the screen space coordinates of the new point
	register float OneOverZ = 1.0f / z;
	TheStateStack.XformedPosPool[v->xyz].z = z;
	TheStateStack.XformedPosPool[v->xyz].x = TheStateStack.XtoPixel( x * OneOverZ );
	TheStateStack.XformedPosPool[v->xyz].y = TheStateStack.YtoPixel( y * OneOverZ );
}


// Intersect edge with y=z plane
// This function is expected to be called second in the clipping chain
// (ie: after near clip, but before all the others)
static void IntersectBottom( ClipVert *v1, ClipVert *v2, ClipVert *v, BOOL color, BOOL light, BOOL tex )
{
	IntersectSide( v1, v2, v, color, light, tex, CLIP_BOTTOM );
}


// Intersect edge with y=z plane
// This function is expected to be called second in the clipping chain
// (ie: after near clip, but before all the others)
static void IntersectTop( ClipVert *v1, ClipVert *v2, ClipVert *v, BOOL color, BOOL light, BOOL tex )
{
	IntersectSide( v1, v2, v, color, light, tex, CLIP_TOP );
}


// Intersect edge with y=z plane
// This function is expected to be called second in the clipping chain
// (ie: after near clip, but before all the others)
static void IntersectRight( ClipVert *v1, ClipVert *v2, ClipVert *v, BOOL color, BOOL light, BOOL tex )
{
	IntersectSide( v1, v2, v, color, light, tex, CLIP_RIGHT );
}


// Intersect edge with y=z plane
// This function is expected to be called second in the clipping chain
// (ie: after near clip, but before all the others)
static void IntersectLeft( ClipVert *v1, ClipVert *v2, ClipVert *v, BOOL color, BOOL light, BOOL tex )
{
	IntersectSide( v1, v2, v, color, light, tex, CLIP_LEFT );
}


/***************************************************************************\
	Here begin the functions which are actually used to populate the
	clipping jump table.
\***************************************************************************/
static inline void pvtClipPrimPoint( PrimPointFC *point, DrawPrimFp drawFn )
{
	PrimPointFC	newPoint;
	int			xyz[MAX_VERTS_PER_POLYGON];
	int			*xyzIdxPtr, *end;

	ShiAssert( point->nVerts > 0 );



	xyzIdxPtr = point->xyz;
	end = xyzIdxPtr + point->nVerts;
	newPoint.nVerts = 0;

	do {
		if (TheStateStack.ClipInfoPool[*xyzIdxPtr].clipFlag == 0) {
			ShiAssert( newPoint.nVerts < MAX_VERTS_PER_POLYGON );
			xyz[newPoint.nVerts] = *xyzIdxPtr;
			newPoint.nVerts++;
		}
		xyzIdxPtr++;
	} while (xyzIdxPtr < end);

	if (newPoint.nVerts) {
		newPoint.type	= PointF;
		newPoint.rgba	= point->rgba;
		newPoint.xyz	= xyz;
		drawFn( &newPoint );
	}
}


void ClipPrimPoint( PrimPointFC *point, UInt32 )
{
	pvtClipPrimPoint( point, (DrawPrimFp)DrawPrimPoint );
}


void ClipPrimFPoint( PrimPointFC *point, UInt32 )
{
	pvtClipPrimPoint( point, (DrawPrimFp)DrawPrimFPoint );
}


static inline void pvtClipPrimLine( PrimLineFC *line, DrawPrimFp drawFn )
{
	PrimLineFC	newLine;
	ClipVert	v0, v1;
	int			xyz[2];
	int			*xyzIdxPtr, *end;

	ShiAssert( line->nVerts > 1 );

	// Set up our temporary primitive
	newLine.type	= LineF;
	newLine.rgba	= line->rgba;
	newLine.xyz		= xyz;
	newLine.nVerts	= 2;

	xyzIdxPtr = line->xyz;
	end = xyzIdxPtr + line->nVerts - 1;

	xyz[0] = TheStateStack.XformedPosPoolNext - TheStateStack.XformedPosPool;
	xyz[1] = xyz[0] + 1;
	v0.xyz	= xyz[0];
	v1.xyz	= xyz[1];
	
	do {
		// Copy the relevant data to avoid clobbering it for any other lines which share it
		TheStateStack.XformedPosPool[v0.xyz]	= TheStateStack.XformedPosPool[xyzIdxPtr[0]];
		TheStateStack.ClipInfoPool[v0.xyz]		= TheStateStack.ClipInfoPool[xyzIdxPtr[0]];
		TheStateStack.XformedPosPool[v1.xyz]	= TheStateStack.XformedPosPool[xyzIdxPtr[1]];
		TheStateStack.ClipInfoPool[v1.xyz]		= TheStateStack.ClipInfoPool[xyzIdxPtr[1]];

		// Clip near
		if        (TheStateStack.ClipInfoPool[v0.xyz].clipFlag & CLIP_NEAR) {
			IntersectNear( &v0, &v1, &v0, FALSE, FALSE, FALSE );
		} else if (TheStateStack.ClipInfoPool[v1.xyz].clipFlag & CLIP_NEAR) {
			IntersectNear( &v0, &v1, &v1, FALSE, FALSE, FALSE );
		}
		if (TheStateStack.ClipInfoPool[v0.xyz].clipFlag & TheStateStack.ClipInfoPool[v1.xyz].clipFlag) {
			continue;
		}

		// Clip bottom
		if        (TheStateStack.ClipInfoPool[v0.xyz].clipFlag & CLIP_BOTTOM) {
			IntersectBottom( &v0, &v1, &v0, FALSE, FALSE, FALSE );
		} else if (TheStateStack.ClipInfoPool[v1.xyz].clipFlag & CLIP_BOTTOM) {
			IntersectBottom( &v0, &v1, &v1, FALSE, FALSE, FALSE );
		}
		if (TheStateStack.ClipInfoPool[v0.xyz].clipFlag & TheStateStack.ClipInfoPool[v1.xyz].clipFlag) {
			continue;
		}

		// Clip top
		if        (TheStateStack.ClipInfoPool[v0.xyz].clipFlag & CLIP_TOP) {
			IntersectTop( &v0, &v1, &v0, FALSE, FALSE, FALSE );
		} else if (TheStateStack.ClipInfoPool[v1.xyz].clipFlag & CLIP_TOP) {
			IntersectTop( &v0, &v1, &v1, FALSE, FALSE, FALSE );
		}
		if (TheStateStack.ClipInfoPool[v0.xyz].clipFlag & TheStateStack.ClipInfoPool[v1.xyz].clipFlag) {
			continue;
		}

		// Clip right
		if        (TheStateStack.ClipInfoPool[v0.xyz].clipFlag & CLIP_RIGHT) {
			IntersectRight( &v0, &v1, &v0, FALSE, FALSE, FALSE );
		} else if (TheStateStack.ClipInfoPool[v1.xyz].clipFlag & CLIP_RIGHT) {
			IntersectRight( &v0, &v1, &v1, FALSE, FALSE, FALSE );
		}
		if (TheStateStack.ClipInfoPool[v0.xyz].clipFlag & TheStateStack.ClipInfoPool[v1.xyz].clipFlag) {
			continue;
		}

		// Clip left
		if        (TheStateStack.ClipInfoPool[v0.xyz].clipFlag & CLIP_LEFT) {
			IntersectLeft( &v0, &v1, &v0, FALSE, FALSE, FALSE );
		} else if (TheStateStack.ClipInfoPool[v1.xyz].clipFlag & CLIP_LEFT) {
			IntersectLeft( &v0, &v1, &v1, FALSE, FALSE, FALSE );
		}
		if (TheStateStack.ClipInfoPool[v0.xyz].clipFlag & TheStateStack.ClipInfoPool[v1.xyz].clipFlag) {
			continue;
		}

		// Draw the line
		xyz[0] = v0.xyz;
		xyz[1] = v1.xyz;
		drawFn( &newLine );

	} while (++xyzIdxPtr < end);
}


void ClipPrimLine( PrimLineFC *line, UInt32 )
{
	pvtClipPrimLine( line, (DrawPrimFp)DrawPrimLine );
}


void ClipPrimFLine( PrimLineFC *line, UInt32 )
{
	pvtClipPrimLine( line, (DrawPrimFp)DrawPrimFLine );
}


// TODO:  If we really want this inlined and optimized fully, we need to pass in flags to say
// which pointers are in use so that the tests can be evaulated at compile time in all cases.
inline BOOL pvtClipPoly( UInt32 clipTest, int *nVerts, int *xyz, int *rgba, int *I, Ptexcoord *uv )
{
	ClipVert		*v, *p, *lastIn,  *nextOut;
	ClipVert		*inList, *outList, *temp;
	ClipVert		vertList1[MAX_VERTS_PER_CLIPPED_POLYGON];
	ClipVert		vertList2[MAX_VERTS_PER_CLIPPED_POLYGON];
	ClipVert		extraVertIdx;
	int				i;

	ShiAssert( xyz );
	ShiAssert( *nVerts >= 3 );
	ShiAssert( *nVerts <= MAX_VERTS_PER_CLIPPED_POLYGON );

	// Intialize the vertex buffers
	outList	= vertList1;
	nextOut	= outList;
	i = 0;
	do {
		nextOut->xyz				= xyz[i];
		if (rgba)	nextOut->rgba	= rgba[i];
		if (I)		nextOut->I		= I[i];
		if (uv)		nextOut->uv		= uv[i];
		i++;
		nextOut++;
	} while( i < *nVerts );
	inList				= vertList2;
	extraVertIdx.xyz	= TheStateStack.XformedPosPoolNext - TheStateStack.XformedPosPool;
	extraVertIdx.rgba	= TheColorBank.nColors;
	extraVertIdx.I		= TheStateStack.IntensityPoolNext - TheStateStack.IntensityPool;


	// Clip to the near plane
	if (clipTest & CLIP_NEAR) {
		temp = inList;
		inList = outList;
		outList = temp;
		lastIn = nextOut-1;
		nextOut = outList;

		for (p=lastIn, v=&inList[0]; v <= lastIn; v++) {

			// If the edge between this vert and the previous one crosses the line, trim it
			if (CLIP_NEAR & (TheStateStack.ClipInfoPool[p->xyz].clipFlag ^ TheStateStack.ClipInfoPool[v->xyz].clipFlag)) {
				ShiAssert( TheStateStack.IsValidPosIndex(extraVertIdx.xyz) );
				*nextOut = extraVertIdx;
				extraVertIdx.xyz++;
				if (rgba)	extraVertIdx.rgba++;
				if (I)		extraVertIdx.I++;
				IntersectNear( p, v, nextOut, rgba!=NULL, I!=NULL, uv!=NULL );
				clipTest |= TheStateStack.ClipInfoPool[nextOut->xyz].clipFlag;
				nextOut++;
			}
			
			// If this vert isn't clipped, use it
			if (!(TheStateStack.ClipInfoPool[v->xyz].clipFlag & CLIP_NEAR)) {
				*nextOut++ = *v;
			}

			p = v;
		}
		ShiAssert( nextOut - outList <= MAX_VERTS_PER_CLIPPED_POLYGON );
		if (nextOut - outList <= 2)  return FALSE;

		// NOTE:  We might get to this point and find a polygon is now marked totally clipped
		// since doing the near clip can change the flags and make a vertex appear to have
		// changed sides of the viewer.  We'll ignore this issue since it is quietly handled
		// and would probably cost more to detect than it would save it early termination.
	}


	// Clip to the bottom plane
	if (clipTest & CLIP_BOTTOM) {
		temp = inList;
		inList = outList;
		outList = temp;
		lastIn = nextOut-1;
		nextOut = outList;

		for (p=lastIn, v=&inList[0]; v <= lastIn; v++) {

			// If the edge between this vert and the previous one crosses the line, trim it
			if (CLIP_BOTTOM & (TheStateStack.ClipInfoPool[p->xyz].clipFlag ^ TheStateStack.ClipInfoPool[v->xyz].clipFlag)) {
				ShiAssert( TheStateStack.IsValidPosIndex(extraVertIdx.xyz) );
				*nextOut = extraVertIdx;
				extraVertIdx.xyz++;
				if (rgba)	extraVertIdx.rgba++;
				if (I)		extraVertIdx.I++;
				IntersectBottom( p, v, nextOut++, rgba!=NULL, I!=NULL, uv!=NULL );
			}
			
			// If this vert isn't clipped, use it
			if (!(TheStateStack.ClipInfoPool[v->xyz].clipFlag & CLIP_BOTTOM)) {
				*nextOut++ = *v;
			}

			p = v;
		}
		ShiAssert( nextOut - outList <= MAX_VERTS_PER_CLIPPED_POLYGON );
		if (nextOut - outList <= 2)  return FALSE;
	}


	// Clip to the top plane
	if (clipTest & CLIP_TOP) {
		temp = inList;
		inList = outList;
		outList = temp;
		lastIn = nextOut-1;
		nextOut = outList;

		for (p=lastIn, v=&inList[0]; v <= lastIn; v++) {

			// If the edge between this vert and the previous one crosses the line, trim it
			if (CLIP_TOP & (TheStateStack.ClipInfoPool[p->xyz].clipFlag ^ TheStateStack.ClipInfoPool[v->xyz].clipFlag)) {
				ShiAssert( TheStateStack.IsValidPosIndex(extraVertIdx.xyz) );
				*nextOut = extraVertIdx;
				extraVertIdx.xyz++;
				if (rgba)	extraVertIdx.rgba++;
				if (I)		extraVertIdx.I++;
				IntersectTop( p, v, nextOut++, rgba!=NULL, I!=NULL, uv!=NULL );
			}
			
			// If this vert isn't clipped, use it
			if (!(TheStateStack.ClipInfoPool[v->xyz].clipFlag & CLIP_TOP)) {
				*nextOut++ = *v;
			}

			p = v;
		}
		ShiAssert( nextOut - outList <= MAX_VERTS_PER_CLIPPED_POLYGON );
		if (nextOut - outList <= 2)  return FALSE;
	}


	// Clip to the right plane
	if (clipTest & CLIP_RIGHT) {
		temp = inList;
		inList = outList;
		outList = temp;
		lastIn = nextOut-1;
		nextOut = outList;

		for (p=lastIn, v=&inList[0]; v <= lastIn; v++) {

			// If the edge between this vert and the previous one crosses the line, trim it
			if (CLIP_RIGHT & (TheStateStack.ClipInfoPool[p->xyz].clipFlag ^ TheStateStack.ClipInfoPool[v->xyz].clipFlag)) {
				ShiAssert( TheStateStack.IsValidPosIndex(extraVertIdx.xyz) );
				*nextOut = extraVertIdx;
				extraVertIdx.xyz++;
				if (rgba)	extraVertIdx.rgba++;
				if (I)		extraVertIdx.I++;
				IntersectRight( p, v, nextOut++, rgba!=NULL, I!=NULL, uv!=NULL );
			}
			
			// If this vert isn't clipped, use it
			if (!(TheStateStack.ClipInfoPool[v->xyz].clipFlag & CLIP_RIGHT)) {
				*nextOut++ = *v;
			}

			p = v;
		}
		ShiAssert( nextOut - outList <= MAX_VERTS_PER_CLIPPED_POLYGON );
		if (nextOut - outList <= 2)  return FALSE;
	}

	
	// Clip to the left plane
	if (clipTest & CLIP_LEFT) {
		temp = inList;
		inList = outList;
		outList = temp;
		lastIn = nextOut-1;
		nextOut = outList;

		for (p=lastIn, v=&inList[0]; v <= lastIn; v++) {

			// If the edge between this vert and the previous one crosses the line, trim it
			if (CLIP_LEFT & (TheStateStack.ClipInfoPool[p->xyz].clipFlag ^ TheStateStack.ClipInfoPool[v->xyz].clipFlag)) {
				ShiAssert( TheStateStack.IsValidPosIndex(extraVertIdx.xyz) );
				*nextOut = extraVertIdx;
				extraVertIdx.xyz++;
				if (rgba)	extraVertIdx.rgba++;
				if (I)		extraVertIdx.I++;
				IntersectLeft( p, v, nextOut++, rgba!=NULL, I!=NULL, uv!=NULL );
			}
			
			// If this vert isn't clipped, use it
			if (!(TheStateStack.ClipInfoPool[v->xyz].clipFlag & CLIP_LEFT)) {
				*nextOut++ = *v;
			}

			p = v;
		}
		ShiAssert( nextOut - outList <= MAX_VERTS_PER_CLIPPED_POLYGON );
		if (nextOut - outList <= 2)  return FALSE;
	}


	// Now replace the input data with our generated output data
	// THERE HAD BETTER BE ENOUGH ROOM!
	*nVerts = i = nextOut - outList;
	while (i) {
		i--;
		xyz[i]				= outList->xyz;
		if (rgba)	rgba[i]	= outList->rgba;
		if (I)		I[i]	= outList->I;
		if (uv)		uv[i]	= outList->uv;
		outList++;
	}
	return TRUE;
}


void ClipPoly( PolyFC *poly, UInt32 clipTest )
{
	PolyFC	newPoly;
	int		xyz[MAX_VERTS_PER_CLIPPED_POLYGON];
	int		i;

	// Initialize our temporary polygon
	newPoly.type	= poly->type;
	newPoly.rgba	= poly->rgba;
	newPoly.nVerts	= poly->nVerts;
	newPoly.xyz		= xyz;
	for (i=0; i<newPoly.nVerts; i++) {
		xyz[i] = poly->xyz[i];
	}

	// Clip the temporary polygon (destructive)
	if (pvtClipPoly( clipTest, &newPoly.nVerts, newPoly.xyz, NULL, NULL, NULL )) {
		DrawPoly( &newPoly );
	}
}


void ClipPolyF( PolyFC *poly, UInt32 clipTest )
{
	PolyFC	newPoly;
	int		xyz[MAX_VERTS_PER_CLIPPED_POLYGON];
	int		i;

	// Initialize our temporary polygon
	newPoly.type	= poly->type;
	newPoly.rgba	= poly->rgba;
	newPoly.nVerts	= poly->nVerts;
	newPoly.xyz		= xyz;
	for (i=0; i<newPoly.nVerts; i++) {
		xyz[i] = poly->xyz[i];
	}

	// Clip the temporary polygon (destructive)
	if (pvtClipPoly( clipTest, &newPoly.nVerts, newPoly.xyz, NULL, NULL, NULL )) {
		DrawPolyF( &newPoly );
	}
}


void ClipPolyL( PolyFCN *poly, UInt32 clipTest )
{
	PolyFCN	newPoly;
	int		xyz[MAX_VERTS_PER_CLIPPED_POLYGON];
	int		i;

	// Initialize our temporary polygon
	newPoly.type	= poly->type;
	newPoly.nVerts	= poly->nVerts;
	newPoly.rgba	= poly->rgba;
	newPoly.I		= poly->I;
	newPoly.xyz		= xyz;
	for (i=0; i<newPoly.nVerts; i++) {
		xyz[i] = poly->xyz[i];
	}

	// Clip the temporary polygon (destructive)
	if (pvtClipPoly( clipTest, &newPoly.nVerts, newPoly.xyz, NULL, NULL, NULL )) {
		DrawPolyL( &newPoly );
	}
}


void ClipPolyFL( PolyFCN *poly, UInt32 clipTest )
{
	PolyFCN	newPoly;
	int		xyz[MAX_VERTS_PER_CLIPPED_POLYGON];
	int		i;

	// Initialize our temporary polygon
	newPoly.type	= poly->type;
	newPoly.nVerts	= poly->nVerts;
	newPoly.rgba	= poly->rgba;
	newPoly.I		= poly->I;
	newPoly.xyz		= xyz;
	for (i=0; i<newPoly.nVerts; i++) {
		xyz[i] = poly->xyz[i];
	}

	// Clip the temporary polygon (destructive)
	if (pvtClipPoly( clipTest, &newPoly.nVerts, newPoly.xyz, NULL, NULL, NULL )) {
		DrawPolyFL( &newPoly );
	}
}


void ClipPolyG( PolyVC *poly, UInt32 clipTest )
{
	PolyVC	newPoly;
	int		xyz[MAX_VERTS_PER_CLIPPED_POLYGON];
	int		rgba[MAX_VERTS_PER_CLIPPED_POLYGON];
	int		i;

	// Initialize our temporary polygon
	newPoly.type	= poly->type;
	newPoly.nVerts	= poly->nVerts;
	newPoly.xyz		= xyz;
	newPoly.rgba	= rgba;
	for (i=0; i<newPoly.nVerts; i++) {
		xyz[i]	= poly->xyz[i];
		rgba[i]	= poly->rgba[i];
	}

	// Clip the temporary polygon (destructive)
	if (pvtClipPoly( clipTest, &newPoly.nVerts, newPoly.xyz, newPoly.rgba, NULL, NULL )) {
		DrawPolyG( &newPoly );
	}
}


void ClipPolyFG( PolyVC *poly, UInt32 clipTest )
{
	PolyVC	newPoly;
	int		xyz[MAX_VERTS_PER_CLIPPED_POLYGON];
	int		rgba[MAX_VERTS_PER_CLIPPED_POLYGON];
	int		i;

	// Initialize our temporary polygon
	newPoly.type	= poly->type;
	newPoly.nVerts	= poly->nVerts;
	newPoly.xyz		= xyz;
	newPoly.rgba	= rgba;
	for (i=0; i<newPoly.nVerts; i++) {
		xyz[i]	= poly->xyz[i];
		rgba[i]	= poly->rgba[i];
	}

	// Clip the temporary polygon (destructive)
	if (pvtClipPoly( clipTest, &newPoly.nVerts, newPoly.xyz, newPoly.rgba, NULL, NULL )) {
		DrawPolyFG( &newPoly );
	}
}


void ClipPolyGL( PolyVCN *poly, UInt32 clipTest )
{
	PolyVCN		newPoly;
	int			xyz[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			rgba[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			I[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			i;

	// Initialize our temporary polygon
	newPoly.type	= poly->type;
	newPoly.nVerts	= poly->nVerts;
	newPoly.xyz		= xyz;
	newPoly.rgba	= rgba;
	newPoly.I		= I;
	for (i=0; i<newPoly.nVerts; i++) {
		xyz[i]	= poly->xyz[i];
		rgba[i]	= poly->rgba[i];
		I[i]	= poly->I[i];
	}

	// Clip the temporary polygon (destructive)
	if (pvtClipPoly( clipTest, &newPoly.nVerts, newPoly.xyz, newPoly.rgba, newPoly.I, NULL )) {
		DrawPolyGL( &newPoly );
	}
}


void ClipPolyFGL( PolyVCN *poly, UInt32 clipTest )
{
	PolyVCN		newPoly;
	int			xyz[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			rgba[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			I[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			i;

	// Initialize our temporary polygon
	newPoly.type	= poly->type;
	newPoly.nVerts	= poly->nVerts;
	newPoly.xyz		= xyz;
	newPoly.rgba	= rgba;
	newPoly.I		= I;
	for (i=0; i<newPoly.nVerts; i++) {
		xyz[i]	= poly->xyz[i];
		rgba[i]	= poly->rgba[i];
		I[i]	= poly->I[i];
	}

	// Clip the temporary polygon (destructive)
	if (pvtClipPoly( clipTest, &newPoly.nVerts, newPoly.xyz, newPoly.rgba, newPoly.I, NULL )) {
		DrawPolyFGL( &newPoly );
	}
}


void ClipPolyT( PolyTexFC *poly, UInt32 clipTest )
{
	PolyTexFC	newPoly;
	int			xyz[MAX_VERTS_PER_CLIPPED_POLYGON];
	Ptexcoord	uv[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			i;

	// Initialize our temporary polygon
	newPoly.type		= poly->type;
	newPoly.nVerts		= poly->nVerts;
	newPoly.texIndex	= poly->texIndex;
	newPoly.xyz			= xyz;
	newPoly.uv			= uv;
	for (i=0; i<newPoly.nVerts; i++) {
		xyz[i]	= poly->xyz[i];
		uv[i]	= poly->uv[i];
	}

	// Clip the temporary polygon (destructive)
	if (pvtClipPoly( clipTest, &newPoly.nVerts, newPoly.xyz, NULL, NULL, newPoly.uv )) {
		DrawPolyT( &newPoly );
	}
}


void ClipPolyFT( PolyTexFC *poly, UInt32 clipTest )
{
	PolyTexFC	newPoly;
	int			xyz[MAX_VERTS_PER_CLIPPED_POLYGON];
	Ptexcoord	uv[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			i;

	// Initialize our temporary polygon
	newPoly.type		= poly->type;
	newPoly.nVerts		= poly->nVerts;
	newPoly.texIndex	= poly->texIndex;
	newPoly.xyz			= xyz;
	newPoly.uv			= uv;
	for (i=0; i<newPoly.nVerts; i++) {
		xyz[i]	= poly->xyz[i];
		uv[i]	= poly->uv[i];
	}

	// Clip the temporary polygon (destructive)
	if (pvtClipPoly( clipTest, &newPoly.nVerts, newPoly.xyz, NULL, NULL, newPoly.uv )) {
		DrawPolyFT( &newPoly );
	}
}


void ClipPolyAT( PolyTexFC *poly, UInt32 clipTest )
{
	PolyTexFC	newPoly;
	int			xyz[MAX_VERTS_PER_CLIPPED_POLYGON];
	Ptexcoord	uv[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			i;

	// Initialize our temporary polygon
	newPoly.type		= poly->type;
	newPoly.nVerts		= poly->nVerts;
	newPoly.rgba		= poly->rgba;
	newPoly.texIndex	= poly->texIndex;
	newPoly.xyz			= xyz;
	newPoly.uv			= uv;
	for (i=0; i<newPoly.nVerts; i++) {
		xyz[i]	= poly->xyz[i];
		uv[i]	= poly->uv[i];
	}

	// Clip the temporary polygon (destructive)
	if (pvtClipPoly( clipTest, &newPoly.nVerts, newPoly.xyz, NULL, NULL, newPoly.uv )) {
		DrawPolyAT( &newPoly );
	}
}


//JAM 09Sep03
void ClipPolyFAT( PolyTexFC *poly, UInt32 clipTest )
{
	PolyTexFC	newPoly;
	int			xyz[MAX_VERTS_PER_CLIPPED_POLYGON];
	Ptexcoord	uv[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			i;

	// Initialize our temporary polygon
	newPoly.type		= poly->type;
	newPoly.nVerts		= poly->nVerts;
	newPoly.rgba		= poly->rgba;
	newPoly.texIndex	= poly->texIndex;
	newPoly.xyz			= xyz;
	newPoly.uv			= uv;
	for (i=0; i<newPoly.nVerts; i++) {
		xyz[i]	= poly->xyz[i];
		uv[i]	= poly->uv[i];
	}

	// Clip the temporary polygon (destructive)
	if (pvtClipPoly( clipTest, &newPoly.nVerts, newPoly.xyz, NULL, NULL, newPoly.uv )) {
		DrawPolyFAT( &newPoly );
	}
}
//JAM


void ClipPolyTL( PolyTexFCN *poly, UInt32 clipTest )
{
	PolyTexFCN	newPoly;
	int			xyz[MAX_VERTS_PER_CLIPPED_POLYGON];
	Ptexcoord	uv[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			i;

	// Initialize our temporary polygon
	newPoly.type		= poly->type;
	newPoly.nVerts		= poly->nVerts;
	newPoly.texIndex	= poly->texIndex;
	newPoly.I			= poly->I;
	newPoly.xyz			= xyz;
	newPoly.uv			= uv;
	for (i=0; i<newPoly.nVerts; i++) {
		xyz[i]	= poly->xyz[i];
		uv[i]	= poly->uv[i];
	}

	// Clip the temporary polygon (destructive)
	if (pvtClipPoly( clipTest, &newPoly.nVerts, newPoly.xyz, NULL, NULL, newPoly.uv )) {
		DrawPolyTL( &newPoly );
	}
}


void ClipPolyFTL( PolyTexFCN *poly, UInt32 clipTest )
{
	PolyTexFCN	newPoly;
	int			xyz[MAX_VERTS_PER_CLIPPED_POLYGON];
	Ptexcoord	uv[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			i;

	// Initialize our temporary polygon
	newPoly.type		= poly->type;
	newPoly.nVerts		= poly->nVerts;
	newPoly.texIndex	= poly->texIndex;
	newPoly.I			= poly->I;
	newPoly.xyz			= xyz;
	newPoly.uv			= uv;
	for (i=0; i<newPoly.nVerts; i++) {
		xyz[i]	= poly->xyz[i];
		uv[i]	= poly->uv[i];
	}

	// Clip the temporary polygon (destructive)
	if (pvtClipPoly( clipTest, &newPoly.nVerts, newPoly.xyz, NULL, NULL, newPoly.uv )) {
		DrawPolyFTL( &newPoly );
	}
}


void ClipPolyATL( PolyTexFCN *poly, UInt32 clipTest )
{
	PolyTexFCN	newPoly;
	int			xyz[MAX_VERTS_PER_CLIPPED_POLYGON];
	Ptexcoord	uv[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			i;

	// Initialize our temporary polygon
	newPoly.type		= poly->type;
	newPoly.nVerts		= poly->nVerts;
	newPoly.rgba		= poly->rgba;
	newPoly.texIndex	= poly->texIndex;
	newPoly.I			= poly->I;
	newPoly.xyz			= xyz;
	newPoly.uv			= uv;
	for (i=0; i<newPoly.nVerts; i++) {
		xyz[i]	= poly->xyz[i];
		uv[i]	= poly->uv[i];
	}

	// Clip the temporary polygon (destructive)
	if (pvtClipPoly( clipTest, &newPoly.nVerts, newPoly.xyz, NULL, NULL, newPoly.uv )) {
		DrawPolyATL( &newPoly );
	}
}


//JAM 09Sep03
void ClipPolyFATL( PolyTexFCN *poly, UInt32 clipTest )
{
	PolyTexFCN	newPoly;
	int			xyz[MAX_VERTS_PER_CLIPPED_POLYGON];
	Ptexcoord	uv[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			i;

	// Initialize our temporary polygon
	newPoly.type		= poly->type;
	newPoly.nVerts		= poly->nVerts;
	newPoly.rgba		= poly->rgba;
	newPoly.texIndex	= poly->texIndex;
	newPoly.I			= poly->I;
	newPoly.xyz			= xyz;
	newPoly.uv			= uv;
	for (i=0; i<newPoly.nVerts; i++) {
		xyz[i]	= poly->xyz[i];
		uv[i]	= poly->uv[i];
	}

	// Clip the temporary polygon (destructive)
	if (pvtClipPoly( clipTest, &newPoly.nVerts, newPoly.xyz, NULL, NULL, newPoly.uv )) {
		DrawPolyFATL( &newPoly );
	}
}
//JAM


void ClipPolyTG( PolyTexVC *poly, UInt32 clipTest )
{
	PolyTexVC	newPoly;
	int			xyz[MAX_VERTS_PER_CLIPPED_POLYGON];
	Ptexcoord	uv[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			i;

	// Initialize our temporary polygon
	newPoly.type		= poly->type;
	newPoly.nVerts		= poly->nVerts;
	newPoly.texIndex	= poly->texIndex;
	newPoly.xyz			= xyz;
	newPoly.uv			= uv;
	for (i=0; i<newPoly.nVerts; i++) {
		xyz[i]	= poly->xyz[i];
		uv[i]	= poly->uv[i];
	}

	// Clip the temporary polygon (destructive)
	if (pvtClipPoly( clipTest, &newPoly.nVerts, newPoly.xyz, NULL, NULL, newPoly.uv )) {
		DrawPolyTG( &newPoly );
	}
}


void ClipPolyFTG( PolyTexVC *poly, UInt32 clipTest )
{
	PolyTexVC	newPoly;
	int			xyz[MAX_VERTS_PER_CLIPPED_POLYGON];
	Ptexcoord	uv[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			i;

	// Initialize our temporary polygon
	newPoly.type		= poly->type;
	newPoly.nVerts		= poly->nVerts;
	newPoly.texIndex	= poly->texIndex;
	newPoly.xyz			= xyz;
	newPoly.uv			= uv;
	for (i=0; i<newPoly.nVerts; i++) {
		xyz[i]	= poly->xyz[i];
		uv[i]	= poly->uv[i];
	}

	// Clip the temporary polygon (destructive)
	if (pvtClipPoly( clipTest, &newPoly.nVerts, newPoly.xyz, NULL, NULL, newPoly.uv )) {
		DrawPolyFTG( &newPoly );
	}
}


void ClipPolyATG( PolyTexVC *poly, UInt32 clipTest )
{
	PolyTexVC	newPoly;
	int			xyz[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			rgba[MAX_VERTS_PER_CLIPPED_POLYGON];
	Ptexcoord	uv[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			i;

	// Initialize our temporary polygon
	newPoly.type		= poly->type;
	newPoly.nVerts		= poly->nVerts;
	newPoly.texIndex	= poly->texIndex;
	newPoly.xyz			= xyz;
	newPoly.rgba		= rgba;
	newPoly.uv			= uv;
	for (i=0; i<newPoly.nVerts; i++) {
		xyz[i]	= poly->xyz[i];
		rgba[i]	= poly->rgba[i];
		uv[i]	= poly->uv[i];
	}

	// Clip the temporary polygon (destructive)
	if (pvtClipPoly( clipTest, &newPoly.nVerts, newPoly.xyz, newPoly.rgba, NULL, newPoly.uv )) {
		DrawPolyATG( &newPoly );
	}
}


//JAM 09Sep03
void ClipPolyFATG( PolyTexVC *poly, UInt32 clipTest )
{
	PolyTexVC	newPoly;
	int			xyz[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			rgba[MAX_VERTS_PER_CLIPPED_POLYGON];
	Ptexcoord	uv[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			i;

	// Initialize our temporary polygon
	newPoly.type		= poly->type;
	newPoly.nVerts		= poly->nVerts;
	newPoly.texIndex	= poly->texIndex;
	newPoly.xyz			= xyz;
	newPoly.rgba		= rgba;
	newPoly.uv			= uv;
	for (i=0; i<newPoly.nVerts; i++) {
		xyz[i]	= poly->xyz[i];
		rgba[i]	= poly->rgba[i];
		uv[i]	= poly->uv[i];
	}

	// Clip the temporary polygon (destructive)
	if (pvtClipPoly( clipTest, &newPoly.nVerts, newPoly.xyz, newPoly.rgba, NULL, newPoly.uv )) {
		DrawPolyFATG( &newPoly );
	}
}
//JAM


void ClipPolyTGL( PolyTexVCN *poly, UInt32 clipTest )
{
	PolyTexVCN	newPoly;
	int			xyz[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			I[MAX_VERTS_PER_CLIPPED_POLYGON];
	Ptexcoord	uv[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			i;

	// Initialize our temporary polygon
	newPoly.type		= poly->type;
	newPoly.nVerts		= poly->nVerts;
	newPoly.texIndex	= poly->texIndex;
	newPoly.xyz			= xyz;
	newPoly.I			= I;
	newPoly.uv			= uv;
	for (i=0; i<newPoly.nVerts; i++) {
		xyz[i]	= poly->xyz[i];
		I[i]	= poly->I[i];
		uv[i]	= poly->uv[i];
	}

	// Clip the temporary polygon (destructive)
	if (pvtClipPoly( clipTest, &newPoly.nVerts, newPoly.xyz, NULL, newPoly.I, newPoly.uv )) {
		DrawPolyTGL( &newPoly );
	}
}


void ClipPolyFTGL( PolyTexVCN *poly, UInt32 clipTest )
{
	PolyTexVCN	newPoly;
	int			xyz[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			I[MAX_VERTS_PER_CLIPPED_POLYGON];
	Ptexcoord	uv[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			i;

	// Initialize our temporary polygon
	newPoly.type		= poly->type;
	newPoly.nVerts		= poly->nVerts;
	newPoly.texIndex	= poly->texIndex;
	newPoly.xyz			= xyz;
	newPoly.I			= I;
	newPoly.uv			= uv;
	for (i=0; i<newPoly.nVerts; i++) {
		xyz[i]	= poly->xyz[i];
		I[i]	= poly->I[i];
		uv[i]	= poly->uv[i];
	}

	// Clip the temporary polygon (destructive)
	if (pvtClipPoly( clipTest, &newPoly.nVerts, newPoly.xyz, NULL, newPoly.I, newPoly.uv )) {
		DrawPolyFTGL( &newPoly );
	}
}


void ClipPolyATGL( PolyTexVCN *poly, UInt32 clipTest )
{
	PolyTexVCN	newPoly;
	int			xyz[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			rgba[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			I[MAX_VERTS_PER_CLIPPED_POLYGON];
	Ptexcoord	uv[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			i;

	// Initialize our temporary polygon
	newPoly.type		= poly->type;
	newPoly.nVerts		= poly->nVerts;
	newPoly.texIndex	= poly->texIndex;
	newPoly.xyz			= xyz;
	newPoly.rgba		= rgba;
	newPoly.I			= I;
	newPoly.uv			= uv;
	for (i=0; i<newPoly.nVerts; i++) {
		xyz[i]	= poly->xyz[i];
		rgba[i]	= poly->rgba[i];
		I[i]	= poly->I[i];
		uv[i]	= poly->uv[i];
	}

	// Clip the temporary polygon (destructive)
	if (pvtClipPoly( clipTest, &newPoly.nVerts, newPoly.xyz, newPoly.rgba, newPoly.I, newPoly.uv )) {
		DrawPolyATGL( &newPoly );
	}
}


//JAM 09Sep03
void ClipPolyFATGL( PolyTexVCN *poly, UInt32 clipTest )
{
	PolyTexVCN	newPoly;
	int			xyz[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			rgba[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			I[MAX_VERTS_PER_CLIPPED_POLYGON];
	Ptexcoord	uv[MAX_VERTS_PER_CLIPPED_POLYGON];
	int			i;

	// Initialize our temporary polygon
	newPoly.type		= poly->type;
	newPoly.nVerts		= poly->nVerts;
	newPoly.texIndex	= poly->texIndex;
	newPoly.xyz			= xyz;
	newPoly.rgba		= rgba;
	newPoly.I			= I;
	newPoly.uv			= uv;
	for (i=0; i<newPoly.nVerts; i++) {
		xyz[i]	= poly->xyz[i];
		rgba[i]	= poly->rgba[i];
		I[i]	= poly->I[i];
		uv[i]	= poly->uv[i];
	}

	// Clip the temporary polygon (destructive)
	if (pvtClipPoly( clipTest, &newPoly.nVerts, newPoly.xyz, newPoly.rgba, newPoly.I, newPoly.uv )) {
		DrawPolyFATGL( &newPoly );
	}
}
//JAM
