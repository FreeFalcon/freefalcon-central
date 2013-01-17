#include "TimeMgr.h"
#include "TOD.h"
#include "RenderOW.h"
#include "RViewPnt.h"
#include "Tex.h"
#include "falclib\include\fakerand.h"
#include "Drawtrcr.h"
#include "Draw2d.h"

#include "Graphics\DXEngine\DXTools.h"
#include "Graphics\DXEngine\DXDefines.h"
#include "Graphics\DXEngine\DXEngine.h"
#include "Graphics\DXEngine\DXVBManager.h"


#ifdef USE_SH_POOLS
MEM_POOL	DrawableTracer::pool;
#endif

// bleh
extern int sGreenMode, gameCompressionRatio;

/***************************************************************************\
    Initialize a tracer
\***************************************************************************/
DrawableTracer::DrawableTracer( void )
: DrawableObject( 1.0 )
{
   // Set to position 0.0, 0.0, 0.0;
   position.x = 0.0F;
   position.y = 0.0F;
   position.z = 0.0F;
   tailEnd.x = 0.0F;
   tailEnd.y = 0.0F;
   tailEnd.z = 0.0F;
   radius = width = 0.5f;
   alpha = 0.2f;
   r = 1.00f;
   g = 1.00f;
   b = 0.50f;
   type = TRACER_TYPE_TRACER;
}

/***************************************************************************\
    Initialize a tracer
\***************************************************************************/
DrawableTracer::DrawableTracer( float w )
: DrawableObject( 1.0 )
{
   // Set to position 0.0, 0.0, 0.0;
   position.x = 0.0F;
   position.y = 0.0F;
   position.z = 0.0F;
   tailEnd.x = 0.0F;
   tailEnd.y = 0.0F;
   tailEnd.z = 0.0F;
   radius = width = w;
   alpha = 0.2f;
   r = 1.00f;
   g = 1.00f;
   b = 0.50f;
   type = TRACER_TYPE_TRACER;
}

/***************************************************************************\
    Initialize a tracer
\***************************************************************************/
DrawableTracer::DrawableTracer( Tpoint *p, float w )
: DrawableObject( 1.0 )
{
   position = *p;
   tailEnd = *p;
   radius = width = w;
   alpha = 0.2f;
   r = 1.00f;
   g = 1.00f;
   b = 0.50f;
   type = TRACER_TYPE_TRACER;
}


/***************************************************************************\
    Remove an instance of a tracer
\***************************************************************************/
DrawableTracer::~DrawableTracer( void )
{
}


/***************************************************************************\
    Remove an instance of a tracer
\***************************************************************************/
void DrawableTracer::Update( Tpoint *head, Tpoint *tail )
{
	position = *head;
	tailEnd = *tail;
}


/***************************************************************************\
    Draw this segmented trail on the given renderer.
\***************************************************************************/
//void DrawableTracer::Draw( class RenderOTW *renderer, int LOD )
void DrawableTracer::Draw( class RenderOTW *renderer, int)
{
	ThreeDVertex		v0, v1, v2, v3, v4, v5;
	Tpoint				cpos, cend;
	int					lineColor;

	// COBRA - RED - Tracers are updated on by the Gun Exec... this makes flying tracers to freeze
	// if no more 'driven' by the gun EXEC... they appear stopped at midair
	if(LastPos.x==position.x && LastPos.z==position.z && LastPos.y==position.y && gameCompressionRatio) { if(parentList) parentList->RemoveMe(); return; }

	// Get the last position for next comparison
	LastPos=position;

#if 1
// 2000-10-11 REMOVED BY S.G. SO TRACER BALL DO LONGER HAVE 'Silver bullet'
#else
	if ( type == TRACER_TYPE_BALL )
	{
		cend = tailEnd;
		cpos.x = (position.x - tailEnd.x) * 2.8f;
		cpos.y = (position.y - tailEnd.y) * 2.8f;
		cpos.z = (position.z - tailEnd.z) * 2.8f;
		Drawable2D::DrawGlowSphere( renderer, &cend, 2.5f, 0.8f );
		cend.x += cpos.x;
		cend.y += cpos.y;
		cend.z += cpos.z;
		Drawable2D::DrawGlowSphere( renderer, &cend, 2.5f, 0.8f );
		cend.x += cpos.x;
		cend.y += cpos.y;
		cend.z += cpos.z;
		Drawable2D::DrawGlowSphere( renderer, &cend, 2.5f, 0.8f );
		cend.x += cpos.x;
		cend.y += cpos.y;
		cend.z += cpos.z;
		Drawable2D::DrawGlowSphere( renderer, &cend, 2.5f, 0.8f );
		return;
	}
#endif

	// alpha += 0.1f;
	if ( alpha > 1.0f )
		alpha = 1.0f;

	// get camera-centric coords of end points
	// NOTE: we work in camera-centric coords here to avoid floating point
	// round-off due to large world space XYZ vals vs the small precision
	// needed to do tracer points.
	cpos.x = position.x - renderer->X();
	cpos.y = position.y - renderer->Y();
	cpos.z = position.z - renderer->Z();
	cend.x = tailEnd.x - renderer->X();
	cend.y = tailEnd.y - renderer->Y();
	cend.z = tailEnd.z - renderer->Z();

	// 1st get 2 points for width at 1 end
	if ( !ConstructWidth( renderer, &cpos, &cend, &v2, &v3, &v5, &v4 ) )
	{
		if ( sGreenMode )
			renderer->SetColor( 0xFF00FF00 );
		else
		{
			lineColor =	((unsigned int)255 << 24) 	     +	// alpha
						((unsigned int)(b*255.0f) << 16) +	// blue
						((unsigned int)(g*255.0f) << 8)  +	// green
						((unsigned int)(r*255.0f));		 	// red

			renderer->SetColor( lineColor );
		}

		// should we do a point?
		renderer->TransformCameraCentricPoint( &cpos,  &v0  );
		renderer->TransformCameraCentricPoint( &cend, &v1 );
		if ( fabs( v0.x - v1.x ) * fabs( v0.y - v1.y ) < 0.9f  )
		{
//			if ( v0.clipFlag == ON_SCREEN )
//				renderer->Render2DPoint( (UInt16)v0.x, (UInt16)v0.y );
			int lineColor =	((unsigned int)(alpha*255.0f) << 24) 	     +	// alpha
							((unsigned int)(r*255.0f) << 16) +	// blue
							((unsigned int)(g*255.0f) << 8)  +	// green
							((unsigned int)(b*255.0f));		 	// red
			TheDXEngine.Draw3DPoint((D3DVECTOR*)&position, lineColor, EMISSIVE);
		}
		else
		{
			int lineColor =	((unsigned int)(alpha*255.0f) << 24) 	     +	// alpha
							((unsigned int)(r*255.0f) << 16) +	// blue
							((unsigned int)(g*255.0f) << 8)  +	// green
							((unsigned int)(b*255.0f));		 	// red
			int LineEndColor =	((unsigned int)(alpha*64.0f) << 24) 	     +	// alpha
							((unsigned int)(r*255.0f) << 16) +	// blue
							((unsigned int)(g*255.0f) << 8)  +	// green
							((unsigned int)(b*255.0f));		 	// red
			TheDXEngine.Draw3DLine((D3DVECTOR*)&position, (D3DVECTOR*)&tailEnd, lineColor, LineEndColor, EMISSIVE);
			//renderer->Render3DLine( &position, &tailEnd );
		}

		return;
	}

	// Set up our drawing mode
//	if ( renderer->GetAlphaMode() )
//	{
		renderer->context.RestoreState( STATE_ALPHA_GOURAUD );
		// renderer->context.SelectTexture( TracerTrailTexture.TexHandle() );
		// renderer->context.RestoreState( STATE_ALPHA_TEXTURE_GOURAUD_TRANSPARENCY_PERSPECTIVE );
		v0.a = v1.a = alpha;
		v3.a = v2.a = 0.0f;
		v4.a = v5.a = 0.0f;
		v1.a = alpha * 0.2f;
/*	}
	else
	{
		renderer->context.RestoreState( STATE_GOURAUD );
		v0.a = v1.a = 1.0f;
		v3.a = v2.a = 1.0f;
		v4.a = v5.a = 1.0f;
	}
*/
	// get 2 points for width at other end
	// ConstructWidth( renderer, &tailEnd, &position, &v4, &v5 );

	// Transform the end points
	renderer->TransformCameraCentricPoint( &cpos,  &v0  );
	renderer->TransformCameraCentricPoint( &cend, &v1 );

	// set rgb of width points and end points

	if ( sGreenMode )
	{
		v3.r = v2.r = 0.20f;
		v3.g = v2.g = 0.80f;
		v3.b = v2.b = 0.20f;
		v4.r = v5.r = 0.20f;
		v4.g = v5.g = 0.80f;
		v4.b = v5.b = 0.20f;
	
		v0.r = 0.30f;
		v0.g = 1.00f;
		v0.b = 0.30f;
	
		v1.r = 0.30f;
		v1.g = 1.00f;
		v1.b = 0.30f;
	}
	else
	{
		v3.r = v2.r = 1.0f;
		v3.g = v2.g = 1.0f;
		v3.b = v2.b = 0.6f;
		v4.r = v5.r = 1.0f;
		v4.g = v5.g = 1.0f;
		v4.b = v5.b = 0.6f;
	
		v0.r = r;
		v0.g = g;
		v0.b = b;
	
		v1.r = r;
		v1.g = g;
		v1.b = b;
	}


	// uv
	/*
	v1.v = 0.5f;
	v0.v = 0.5f;
	v0.u = 0.8f + PRANDFloat() * 0.2f;
	v1.u = 0.2f + PRANDFloat() * 0.2f;

	v2.u = 0.8f + PRANDFloat() * 0.2f;
	v2.v = 0.2f + PRANDFloat() * 0.2f;
	v3.u = 0.8f + PRANDFloat() * 0.2f;
	v3.v = 0.8f + PRANDFloat() * 0.2f;
	v4.u = 0.2f + PRANDFloat() * 0.2f;
	v4.v = 0.8f + PRANDFloat() * 0.2f;
	v5.u = 0.2f + PRANDFloat() * 0.2f;
	v5.v = 0.2f + PRANDFloat() * 0.2f;
	*/

	// q
	/*
	v0.q = v0.z * 0.001f;
	v1.q = v1.z * 0.001f;
	v2.q = v2.z * 0.001f;
	v3.q = v3.z * 0.001f;
	v4.q = v4.z * 0.001f;
	v5.q = v5.z * 0.001f;
	*/

	v0.q = 1.0f;
	v1.q = 1.0f;
	v2.q = 1.0f;
	v3.q = 1.0f;
	v4.q = 1.0f;
	v5.q = 1.0f;

	// Draw the polygon
	renderer->DrawSquare( &v1, &v5, &v2, &v0, CULL_ALLOW_ALL );
	renderer->DrawSquare( &v1, &v4, &v3, &v0, CULL_ALLOW_ALL );
}



/***************************************************************************\
    Help function to compute the "width points" of the tracer
	Returns TRUE when we should do polygon.
	FALSE just do line.
\***************************************************************************/
BOOL DrawableTracer::ConstructWidth( RenderOTW *renderer,
									 Tpoint *start,
									 Tpoint *end,
									 ThreeDVertex *xformLeft,
									 ThreeDVertex *xformRight,  
									 ThreeDVertex *xformLefte,
									 ThreeDVertex *xformRighte )
{
	Tpoint	left, right, wloc;
	Tpoint	UP;
	float	dx, dy, dz;
	float	widthX, widthY, widthZ;
	float	mag, normalizer;

	
	// Compute the direction of this segment in world space
	dx = end->x - start->x;
	dy = end->y - start->y;
	dz = end->z - start->z;
	
	// Compute the cross product of the two vectors: along vector of
	// segment with either of the 2 endppoints (this is essentially the
	// DOV vector since we're really working in camera centric coords)
	widthX = end->y * dz - end->z * dy;
	widthY = end->z * dx - end->x * dz;
	widthZ = end->x * dy - end->y * dx;

	// Compute the magnitude of the cross product result
	mag = (float)sqrt( widthX*widthX + widthY*widthY + widthZ*widthZ );

	// If the cross product was degenerate (parallel vectors), use the "up" vector
	if (mag < 0.001f) {
	 	renderer->GetUp( &UP );
		widthX = UP.x;
		widthY = UP.y;
		widthZ = UP.z;
		mag = (float)sqrt( widthX*widthX + widthY*widthY + widthZ*widthZ );
	}

	// Normalize the width vector, then scale it to 1/2 of the total width of the segment
	normalizer = scale * width / mag;
	widthX *= normalizer;
	widthY *= normalizer;
	widthZ *= normalizer;

	// get location on line where we apply width
	wloc.x = start->x + dx * 0.05f;
	wloc.y = start->y + dy * 0.05f;
	wloc.z = start->z + dz * 0.05f;


	// Compute the world space location of the two corners at the end of this segment
	left.x  = wloc.x - widthX * 0.5f;
	left.y  = wloc.y - widthY * 0.5f;
	left.z  = wloc.z - widthZ * 0.5f;
	right.x = wloc.x + widthX * 0.5f;
	right.y = wloc.y + widthY * 0.5f;
	right.z = wloc.z + widthZ * 0.5f;

	// Transform the two new corners
	renderer->TransformCameraCentricPoint( &left,  xformLeft  );
	renderer->TransformCameraCentricPoint( &right, xformRight );

	if ( fabs( xformLeft->x - xformRight->x ) * fabs( xformLeft->y - xformRight->y ) < 0.7f && alpha == 1.0f )
		return FALSE;

	// get location on line where we apply width
	wloc.x = start->x + dx * 0.95f;
	wloc.y = start->y + dy * 0.95f;
	wloc.z = start->z + dz * 0.95f;


	// Compute the world space location of the two corners at the end of this segment
	left.x  = wloc.x - widthX;
	left.y  = wloc.y - widthY;
	left.z  = wloc.z - widthZ;
	right.x = wloc.x + widthX;
	right.y = wloc.y + widthY;
	right.z = wloc.z + widthZ;

	// Transform the two new corners
	renderer->TransformCameraCentricPoint( &left,  xformLefte  );
	renderer->TransformCameraCentricPoint( &right, xformRighte );

	return TRUE;
}
