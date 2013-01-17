/***************************************************************************\
    OTWcull.cpp
    Scott Randolph
    January 2, 1995

    This class provides 3D drawing functions specific to rendering out the
	window views including terrain.

	This file provides the terrain culling computations.  The view volume
	is computed and the posts required to draw the terrain within the volume
	are identified.
\***************************************************************************/
#include <math.h>
#include "TMap.h"
#include "Tpost.h"
#include "RViewPnt.h"
#include "RenderOW.h"


static const int	MAX_POSITIVE_I	=  25000;
static const int	MAX_NEGATIVE_I	= -25000;
static const float	MAX_POSTIVE_F	= MAX_POSITIVE_I * 819.995f;	// FeetPerPost
static const float	MAX_NEGATIVE_F	= MAX_NEGATIVE_I * 819.995f;	// FeetPerPost



/***************************************************************************\
    Draw one ring of terrain data clipped by the viewing frustum edges.
\***************************************************************************/
void RenderOTW::BuildRingList( void )
{
	int				LOD;
	int				startRing;
	int				stopRing;
	int				ring;


	ShiAssert( IsReady() );


	//
	// Start a new span list
	//
	firstEmptySpan = spanList;


	//
	// Setup the data we need for each LOD
	//
	for ( LOD = viewpoint->GetLowLOD(); LOD >= viewpoint->GetHighLOD(); LOD-- ) {

		// Get the available range for each LOD
		LODdata[LOD].availablePostRange = viewpoint->GetAvailablePostRange( LOD );

		// Decide which post is to be the center of the rings at this level
		LODdata[LOD].centerRow = (int)WORLD_TO_LEVEL_POST( viewpoint->X(), LOD );
		LODdata[LOD].centerCol = (int)WORLD_TO_LEVEL_POST( viewpoint->Y(), LOD );

		// Determine the location of the glue row and column at the outside of this LOD region
		// NOTE:  These two must evaluate to zero or one, as they are used arithmetically
		LODdata[LOD].glueOnBottom   = LODdata[LOD].centerRow & 1;
		LODdata[LOD].glueOnLeft		= LODdata[LOD].centerCol & 1;


#ifdef TWO_D_MAP_AVAILABLE
		if (twoDmode) {
			char	message[80];
			SetColor( 0xFFFFFFFF );
			sprintf( message, "LOD%0d Glue: %s %s", LOD, 
				LODdata[LOD].glueOnBottom ? "Bottom" : "Top",
				LODdata[LOD].glueOnLeft   ? "Left"   : "Right" );
			ScreenText( 0.0f, 8.0f * LOD, message );
		}
#endif	


		// Compute and record the camera space versions of the X, Y, and Z world space axes
		// Scale for LOD's horizontal step size
		LODdata[LOD].Xstep[0] = T.M11 * LEVEL_POST_TO_WORLD( 1, LOD );
		LODdata[LOD].Xstep[1] = T.M21 * LEVEL_POST_TO_WORLD( 1, LOD );
		LODdata[LOD].Xstep[2] = T.M31 * LEVEL_POST_TO_WORLD( 1, LOD );

		// Scale for LOD's horizontal step size
		LODdata[LOD].Ystep[0] = T.M12 * LEVEL_POST_TO_WORLD( 1, LOD );
		LODdata[LOD].Ystep[1] = T.M22 * LEVEL_POST_TO_WORLD( 1, LOD );
		LODdata[LOD].Ystep[2] = T.M32 * LEVEL_POST_TO_WORLD( 1, LOD );

		// Leave in world space units of feet
		LODdata[LOD].Zstep[0] = T.M13;
		LODdata[LOD].Zstep[1] = T.M23;
		LODdata[LOD].Zstep[2] = T.M33;
	}


	//
	// Determine and record which rings we need to draw at each LOD
	//

	// Start the rings at the most distant available post
	startRing = LODdata[ viewpoint->GetLowLOD() ].availablePostRange;

	// We will consider rings at each level of detail from lowest detail to highest
	for ( LOD = viewpoint->GetLowLOD(); LOD >= viewpoint->GetHighLOD(); LOD-- ) {

		// Figure where to stop this LOD and move to the next
		if (LOD != viewpoint->GetHighLOD()) {

			// The stop ring is one ring in from the first available ring at the next LOD
			stopRing = (LODdata[LOD-1].availablePostRange >> 1) - 1;

			// Make sure we have enough space for connectors at each LOD
			if (stopRing > startRing-3) {
				stopRing = startRing-3;
			}

			// Stop at zero in any case
			if (stopRing < 0) {
				stopRing = 0;
			}
		} else {
			stopRing = 0;
		} 

		ShiAssert( startRing <= maxSpanOffset );
		ShiAssert( stopRing >= 0 );

		for ( ring = startRing; ring >= stopRing; ring-- ) {

			ShiAssert( firstEmptySpan < spanList + spanListMaxEntries );
			ShiAssert( ring <= LODdata[LOD].availablePostRange );

			firstEmptySpan->ring = ring;
			firstEmptySpan->LOD = LOD;
			
			// Figure out the actual location of each side of this ring
			firstEmptySpan->Tsector.insideEdge = LEVEL_POST_TO_WORLD( LODdata[LOD].centerRow + ring,     LOD );
			firstEmptySpan->Bsector.insideEdge = LEVEL_POST_TO_WORLD( LODdata[LOD].centerRow - ring + 1, LOD );
			firstEmptySpan->Rsector.insideEdge = LEVEL_POST_TO_WORLD( LODdata[LOD].centerCol + ring,     LOD );
			firstEmptySpan->Lsector.insideEdge = LEVEL_POST_TO_WORLD( LODdata[LOD].centerCol - ring + 1, LOD );

			firstEmptySpan++;
		}

		// If we've reached the viewer, stop now
		if (stopRing == 0) {
			break;
		}
		
		// What is the next ring at the next LOD (back out 1 to get an extra ring for the glue)
		startRing = (stopRing << 1) + 1;
	}

	// Pad the end of the list to avoid overrunning the list when performing look ahead
	ShiAssert( firstEmptySpan < spanList + spanListMaxEntries );
	if ( firstEmptySpan != spanList ) {
		firstEmptySpan->ring = -1;
		firstEmptySpan->LOD = (firstEmptySpan-1)->LOD;
		firstEmptySpan->Tsector.insideEdge = 0.0f;
		firstEmptySpan->Bsector.insideEdge = 0.0f;
		firstEmptySpan->Rsector.insideEdge = 0.0f;
		firstEmptySpan->Lsector.insideEdge = 0.0f;
	}
}



/***************************************************************************\
    Compute the regions of posts to be drawn in each sector which has
	a horizontal traversal order.
\***************************************************************************/
void RenderOTW::ClipHorizontalSectors( void )
{
	SpanListEntry	*span=NULL;
	int				LOD=0;
	int				w=0, e=0;
	float			startLocation=0.0F;
	BoundSegment	eastBoundry[4];
	BoundSegment	westBoundry[4];


	// Construct the ordered edge list (horizontal span case - X buckets (since X is North/Up))
	if ( Yaw() > PI ) {
		// Necessarily, rightX2 >= leftX2
		if (rightX1 > rightX2) {
			// Right front corner is north most
			startLocation = rightX1;
			westBoundry[0].edge = right_edge;
			westBoundry[1].edge = back_edge;
			westBoundry[2].edge = left_edge;
			westBoundry[0].end = rightX2;
			westBoundry[1].end = leftX2;
			westBoundry[2].end = leftX1;
			eastBoundry[0].edge = front_edge;
			eastBoundry[1].edge = left_edge;
			eastBoundry[2].edge = back_edge;
			eastBoundry[0].end = leftX1;
			eastBoundry[1].end = leftX2;
			eastBoundry[2].end = rightX2;
		} else {
			// Right back corner is north most
			startLocation = rightX2;
			westBoundry[0].edge = back_edge;
			westBoundry[1].edge = left_edge;
			westBoundry[2].edge = front_edge;
			westBoundry[0].end = leftX2;
			westBoundry[1].end = leftX1;
			westBoundry[2].end = rightX1;
			eastBoundry[0].edge = right_edge;
			eastBoundry[1].edge = front_edge;
			eastBoundry[2].edge = left_edge;
			eastBoundry[0].end = rightX1;
			eastBoundry[1].end = leftX1;
			eastBoundry[2].end = leftX2;
		}
	} else {
		// Necessarily, rightX2 <= leftX2
		if (leftX1 > leftX2) {
			// Left front corner is north most
			startLocation = leftX1;
			westBoundry[0].edge = front_edge;
			westBoundry[1].edge = right_edge;
			westBoundry[2].edge = back_edge;
			westBoundry[0].end = rightX1;
			westBoundry[1].end = rightX2;
			westBoundry[2].end = leftX2;
			eastBoundry[0].edge = left_edge;
			eastBoundry[1].edge = back_edge;
			eastBoundry[2].edge = right_edge;
			eastBoundry[0].end = leftX2;
			eastBoundry[1].end = rightX2;
			eastBoundry[2].end = rightX1;
		} else {
			// Left back corner is north most
			startLocation = leftX2;
			westBoundry[0].edge = left_edge;
			westBoundry[1].edge = front_edge;
			westBoundry[2].edge = right_edge;
			westBoundry[0].end = leftX1;
			westBoundry[1].end = rightX1;
			westBoundry[2].end = rightX2;
			eastBoundry[0].edge = back_edge;
			eastBoundry[1].edge = right_edge;
			eastBoundry[2].edge = front_edge;
			eastBoundry[0].end = rightX2;
			eastBoundry[1].end = rightX1;
			eastBoundry[2].end = leftX1;
		}
	}

	// Terminated the edge chain with a point greater than any possible to ensure we see an "upturn"
	eastBoundry[3].end = MAX_POSTIVE_F;
	westBoundry[3].end = MAX_POSTIVE_F;


	// Force a fresh start by noting an illegal "current" LOD
	LOD = -1;

	// Now clip the top horizontal spans to the bounding region
	for (span = spanList; span != firstEmptySpan; span++) {

		// Spans are  empty until we get to the top corner of the bounding region
		if (span->Tsector.insideEdge > startLocation) {
			span->Tsector.maxEndPoint	= MAX_NEGATIVE_F;
			span->Tsector.minEndPoint	= MAX_POSTIVE_F;
			continue;
		}

		// Restart the edge traversal since the spans can take a step backward at LOD change
		if (span->LOD != LOD) {
			LOD = span->LOD;
			e = w = 0;
		}

		// Switch controlling edges when required
		if (span->Tsector.insideEdge <= westBoundry[w].end) {

			// Try to find an edge that will advance us at least one row
			while ( westBoundry[w+1].end <= westBoundry[w].end ) {
				w++;

				if (span->Tsector.insideEdge > westBoundry[w].end) {
					break;
				}
			}

			// Nothing on these spans if we've exhausted our edges
			if (span->Tsector.insideEdge <= westBoundry[w].end) {
				span->Tsector.maxEndPoint	= MAX_NEGATIVE_F;
				span->Tsector.minEndPoint	= MAX_POSTIVE_F;
				continue;
			}

			ShiAssert( w < 3 );
			ShiAssert( westBoundry[w].end <= westBoundry[w-1].end );
		}
		if (span->Tsector.insideEdge <= eastBoundry[e].end) {

			// Try to find an edge that will advance us at least one row
			while ( eastBoundry[e+1].end <= eastBoundry[e].end ) {
				e++;

				if (span->Tsector.insideEdge > eastBoundry[e].end) {
					break;
				}
			}

			// We can't exhaust this edge without having already exhausted the one above
//			ShiAssert( span->Tsector.insideEdge > eastBoundry[e].end );
//			ShiAssert( e > 0 );		// We _must_ have taken a step in the while above
//			ShiAssert( e < 3 );
//			ShiAssert( eastBoundry[e].end <= eastBoundry[e-1].end );
		}


		// Compute the intersection of this span with the bounding region
		span->Tsector.minEndPoint = westBoundry[w].edge.Y( (float)span->Tsector.insideEdge );
		span->Tsector.maxEndPoint = eastBoundry[e].edge.Y( (float)span->Tsector.insideEdge );
	}

	// Force a fresh start by noting an illegal "current" LOD
	LOD = -1;

	// Now fill in the extents of spans at the bottom of the bounding region
	for (span = firstEmptySpan-1; span >= spanList; span--) {

		// Spans are empty until we get to the top corner of the bounding region
		if (span->Bsector.insideEdge > startLocation) {
			span->Bsector.maxEndPoint	= MAX_NEGATIVE_F;
			span->Bsector.minEndPoint	= MAX_POSTIVE_F;
			continue;
		}
	
		// Restart the edge traversal since the spans can take a step backward at LOD change
		if (span->LOD != LOD) {
			LOD = span->LOD;
			e = w = 0;
		}

		// Switch controlling edges when required
		if (span->Bsector.insideEdge <= westBoundry[w].end) {

			// Try to find an edge that will advance us at least one row
			while (westBoundry[w+1].end <= westBoundry[w].end) {
				w++;

				if (span->Bsector.insideEdge > westBoundry[w].end) {
					break;
				}
			}

			// Nothing on these spans if we've exhausted our edges
			if (span->Bsector.insideEdge <= westBoundry[w].end) {
				span->Bsector.maxEndPoint	= MAX_NEGATIVE_F;
				span->Bsector.minEndPoint	= MAX_POSTIVE_F;
				continue;
			}

			ShiAssert( w < 3 );
			ShiAssert( westBoundry[w].end <= westBoundry[w-1].end );
		}
		if (span->Bsector.insideEdge <= eastBoundry[e].end) {

			// Try to find an edge that will advance us at least one row
			while ( eastBoundry[e+1].end <= eastBoundry[e].end ) {
				e++;

				if (span->Bsector.insideEdge > eastBoundry[e].end) {
					break;
				}
			}

			// We can't exhaust this edge without having already exhausted the one above
//			ShiAssert( e > 0 );		// We _must_ have taken a step in the while above
//			ShiAssert( e < 3 );
//			ShiAssert( eastBoundry[e].end <= eastBoundry[e-1].end );
		}


		// Compute the intersection of this span with the bounding region
		span->Bsector.minEndPoint = westBoundry[w].edge.Y( (float)span->Bsector.insideEdge );
		span->Bsector.maxEndPoint = eastBoundry[e].edge.Y( (float)span->Bsector.insideEdge );
	}
}




/***************************************************************************\
    Compute the regions of posts to be drawn in each sector which has
	a vertical traversal order.
\***************************************************************************/
void RenderOTW::ClipVerticalSectors( void )
{
	SpanListEntry	*span=NULL;
	int				LOD=0;
	int				n=0, s=0;
	float			startLocation=0.0F;
	BoundSegment	northBoundry[4];
	BoundSegment	southBoundry[4];


	// Construct the ordered edge list (horizontal span case - Y buckets (since Y is East/Right))
	if ( (Yaw() < PI_OVER_2) || (Yaw() > 3.0f*PI_OVER_2) ) {
		// Necessarily, rightY2 > leftY2
		if (rightY1 > rightY2) {
			// Right front corner is east most
			startLocation = rightY1;
			northBoundry[0].edge = right_edge;
			northBoundry[1].edge = back_edge;
			northBoundry[2].edge = left_edge;
			northBoundry[0].end = rightY2;
			northBoundry[1].end = leftY2;
			northBoundry[2].end = leftY1;
			southBoundry[0].edge = front_edge;
			southBoundry[1].edge = left_edge;
			southBoundry[2].edge = back_edge;
			southBoundry[0].end = leftY1;
			southBoundry[1].end = leftY2;
			southBoundry[2].end = rightY2;
		} else {
			// Right back corner is east most
			startLocation = rightY2;
			northBoundry[0].edge = back_edge;
			northBoundry[1].edge = left_edge;
			northBoundry[2].edge = front_edge;
			northBoundry[0].end = leftY2;
			northBoundry[1].end = leftY1;
			northBoundry[2].end = rightY1;
			southBoundry[0].edge = right_edge;
			southBoundry[1].edge = front_edge;
			southBoundry[2].edge = left_edge;
			southBoundry[0].end = rightY1;
			southBoundry[1].end = leftY1;
			southBoundry[2].end = leftY2;
		}
	} else {
		// Necessarily, rightY2 <= leftY2
		if (leftY1 > leftY2) {
			// Left front corner is east most
			startLocation = leftY1;
			northBoundry[0].edge = front_edge;
			northBoundry[1].edge = right_edge;
			northBoundry[2].edge = back_edge;
			northBoundry[0].end = rightY1;
			northBoundry[1].end = rightY2;
			northBoundry[2].end = leftY2;
			southBoundry[0].edge = left_edge;
			southBoundry[1].edge = back_edge;
			southBoundry[2].edge = right_edge;
			southBoundry[0].end = leftY2;
			southBoundry[1].end = rightY2;
			southBoundry[2].end = rightY1;
		} else {
			// Left back corner is east most
			startLocation = leftY2;
			northBoundry[0].edge = left_edge;
			northBoundry[1].edge = front_edge;
			northBoundry[2].edge = right_edge;
			northBoundry[0].end = leftY1;
			northBoundry[1].end = rightY1;
			northBoundry[2].end = rightY2;
			southBoundry[0].edge = back_edge;
			southBoundry[1].edge = right_edge;
			southBoundry[2].edge = front_edge;
			southBoundry[0].end = rightY2;
			southBoundry[1].end = rightY1;
			southBoundry[2].end = leftY1;
		}
	}

	// Terminated the edge chain with a point greater than any possible to ensure we see an "upturn"
	northBoundry[3].end = MAX_POSTIVE_F;
	southBoundry[3].end = MAX_POSTIVE_F;


	// Force a fresh start by noting an illegal "current" LOD
	LOD = -1;

	// Now fill in the extents of spans along the right edge of the bounding region
	for (span = spanList; span != firstEmptySpan; span++) {

		// Spans are empty until we get to the top corner of the bounding region
		if (span->Rsector.insideEdge > startLocation) {
			span->Rsector.minEndPoint	= MAX_POSTIVE_F;
			span->Rsector.maxEndPoint	= MAX_NEGATIVE_F;
			continue;
		}

		// Restart the edge traversal since the spans can take a step backward at LOD change
		if (span->LOD != LOD) {
			LOD = span->LOD;
			n = s = 0;
		}

		// Switch controlling edges when required
		if (span->Rsector.insideEdge <= southBoundry[s].end) {

			// Try to find an edge that will advance us at least one row
			while (southBoundry[s+1].end <= southBoundry[s].end) {
				s++;

				if (span->Rsector.insideEdge > southBoundry[s].end) {
					break;
				}
			}

			// Nothing on these spans if we've exhausted our edges
			if (span->Rsector.insideEdge <= southBoundry[s].end) {
				span->Rsector.minEndPoint	= MAX_POSTIVE_F;
				span->Rsector.maxEndPoint	= MAX_NEGATIVE_F;
				continue;
			}

			ShiAssert( s < 3 );
			ShiAssert( southBoundry[s].end <= southBoundry[s-1].end );
		}
		if (span->Rsector.insideEdge <= northBoundry[n].end) {

			// Try to find an edge that will advance us at least one row
			while (northBoundry[n+1].end <= northBoundry[n].end) {
				n++;

				if (span->Rsector.insideEdge > northBoundry[n].end) {
					break;
				}
			}

			// We can't exhaust this edge without having already exhausted the one above
//			ShiAssert( n > 0 );		// We _must_ have taken a step in the while above
//			ShiAssert( n < 3 );
//			ShiAssert( northBoundry[n].end <= northBoundry[n-1].end );
		}


		// Compute the intersection of this span with the bounding region
		span->Rsector.minEndPoint = southBoundry[s].edge.X( (float)span->Rsector.insideEdge );
		span->Rsector.maxEndPoint = northBoundry[n].edge.X( (float)span->Rsector.insideEdge );
	}


	// Force a fresh start by noting an illegal "current" LOD
	LOD = -1;

	// Now fill in the extents of spans along the left edge of the bounding region
	for (span = firstEmptySpan-1; span >= spanList; span--) {

		// Spans are empty until we get to the top corner of the bounding region
		if (span->Lsector.insideEdge > startLocation) {
			span->Lsector.maxEndPoint	= MAX_NEGATIVE_F;
			span->Lsector.minEndPoint	= MAX_POSTIVE_F;
			continue;
		}
	
		// Restart the edge traversal since the spans can take a step backward at LOD change
		if (span->LOD != LOD) {
			LOD = span->LOD;
			n = s = 0;
		}

		// Switch controlling edges when required
		if (span->Lsector.insideEdge <= southBoundry[s].end) {

			// Try to find an edge that will advance us at least one row
			while (southBoundry[s+1].end <= southBoundry[s].end) {
				s++;

				if (span->Lsector.insideEdge > southBoundry[s].end) {
					break;
				}
			}

			// Nothing on these spans if we've exhausted our edges
			if (span->Lsector.insideEdge <= southBoundry[s].end) {
				span->Lsector.minEndPoint	= MAX_POSTIVE_F;
				span->Lsector.maxEndPoint	= MAX_NEGATIVE_F;
				continue;
			}

			ShiAssert( s < 3 );
			ShiAssert( southBoundry[s].end <= southBoundry[s-1].end );
		}
		if (span->Lsector.insideEdge <= northBoundry[n].end) {

			// Try to find an edge that will advance us at least one row
			while (northBoundry[n+1].end <= northBoundry[n].end) {
				n++;

				if (span->Lsector.insideEdge > northBoundry[n].end) {
					break;
				}
			}

			// We can't exhaust this edge without having already exhausted the one above
//			ShiAssert( n > 0 );		// We _must_ have taken a step in the while above
//			ShiAssert( n < 3 );
//			ShiAssert( northBoundry[n].end <= northBoundry[n-1].end );
		}


		// Compute the intersection of this span with the bounding region
		span->Lsector.minEndPoint = southBoundry[s].edge.X( (float)span->Lsector.insideEdge );
		span->Lsector.maxEndPoint = northBoundry[n].edge.X( (float)span->Lsector.insideEdge );
	}
}



/***************************************************************************\
    Given the spanList which contains the world space extents of the
	inner edges of the horizontal and vertical spans of squares, we
	construct the set of all lower left corner posts for the squares we
	need to draw.

	NOTE:  We count on the truncation during the float to int conversion to
	adjust the end points side to side.  The inner/outter adjustment is explicit.
\***************************************************************************/
void RenderOTW::BuildCornerSet( void )
{
	SpanListEntry	*span;


	// Move from inner ring outward
	for ( span = firstEmptySpan-1; span>=spanList; span-- ) {

		// The start/stop points were computed for the inside edges of each ring of squares.
		// Make sure the outter edge doesn't dictate a larger span.
		if (span != spanList) {
			if (span->LOD == (span-1)->LOD) {
				// Normal case (look out one ring)
				ShiAssert( (span-1) >= spanList );
				span->Tsector.minEndPoint = min( span->Tsector.minEndPoint, (span-1)->Tsector.minEndPoint );
				span->Tsector.maxEndPoint = max( span->Tsector.maxEndPoint, (span-1)->Tsector.maxEndPoint );
				span->Rsector.minEndPoint = min( span->Rsector.minEndPoint, (span-1)->Rsector.minEndPoint );
				span->Rsector.maxEndPoint = max( span->Rsector.maxEndPoint, (span-1)->Rsector.maxEndPoint );
				span->Bsector.minEndPoint = min( span->Bsector.minEndPoint, (span-1)->Bsector.minEndPoint );
				span->Bsector.maxEndPoint = max( span->Bsector.maxEndPoint, (span-1)->Bsector.maxEndPoint );
				span->Lsector.minEndPoint = min( span->Lsector.minEndPoint, (span-1)->Lsector.minEndPoint );
				span->Lsector.maxEndPoint = max( span->Lsector.maxEndPoint, (span-1)->Lsector.maxEndPoint );
			} else {
				// Connector case (look out two rings to a lower LOD)
				ShiAssert( (span-2) >= spanList );
				span->Tsector.minEndPoint = min( span->Tsector.minEndPoint, (span-2)->Tsector.minEndPoint );
				span->Tsector.maxEndPoint = max( span->Tsector.maxEndPoint, (span-2)->Tsector.maxEndPoint );
				span->Rsector.minEndPoint = min( span->Rsector.minEndPoint, (span-2)->Rsector.minEndPoint );
				span->Rsector.maxEndPoint = max( span->Rsector.maxEndPoint, (span-2)->Rsector.maxEndPoint );
				span->Bsector.minEndPoint = min( span->Bsector.minEndPoint, (span-2)->Bsector.minEndPoint );
				span->Bsector.maxEndPoint = max( span->Bsector.maxEndPoint, (span-2)->Bsector.maxEndPoint );
				span->Lsector.minEndPoint = min( span->Lsector.minEndPoint, (span-2)->Lsector.minEndPoint );
				span->Lsector.maxEndPoint = max( span->Lsector.maxEndPoint, (span-2)->Lsector.maxEndPoint );
			}
		}

		// Convert all the start/stop points into units of level posts
		span->Tsector.startDraw	= WORLD_TO_LEVEL_POST( span->Tsector.minEndPoint, span->LOD );
		span->Tsector.stopDraw	= WORLD_TO_LEVEL_POST( span->Tsector.maxEndPoint, span->LOD );
		span->Rsector.startDraw	= WORLD_TO_LEVEL_POST( span->Rsector.minEndPoint, span->LOD );
		span->Rsector.stopDraw	= WORLD_TO_LEVEL_POST( span->Rsector.maxEndPoint, span->LOD );
		span->Bsector.startDraw	= WORLD_TO_LEVEL_POST( span->Bsector.minEndPoint, span->LOD );
		span->Bsector.stopDraw	= WORLD_TO_LEVEL_POST( span->Bsector.maxEndPoint, span->LOD );
		span->Lsector.startDraw	= WORLD_TO_LEVEL_POST( span->Lsector.minEndPoint, span->LOD );
		span->Lsector.stopDraw	= WORLD_TO_LEVEL_POST( span->Lsector.maxEndPoint, span->LOD );

#if 0
		// If min and max fall onto the same level post, we can't tell the different between an empty
		// segment and a segment containing a single post.  This check resolves that ambiguity.
		// It may also prevent us from causing neighboring verticies to be transformed
		// when we won't, in fact be drawing anything.
		if ( span->Tsector.minEndPoint > span->Tsector.maxEndPoint ) {
			span->Tsector.startDraw = MAX_POSITIVE_I;
			span->Tsector.stopDraw  = MAX_NEGATIVE_I;
		}
		if ( span->Rsector.minEndPoint > span->Rsector.maxEndPoint ) {
			span->Rsector.startDraw = MAX_POSITIVE_I;
			span->Rsector.stopDraw  = MAX_NEGATIVE_I;
		}
		if ( span->Bsector.minEndPoint > span->Bsector.maxEndPoint ) {
			span->Bsector.startDraw = MAX_POSITIVE_I;
			span->Bsector.stopDraw  = MAX_NEGATIVE_I;
		}
		if ( span->Lsector.minEndPoint > span->Lsector.maxEndPoint ) {
			span->Lsector.startDraw = MAX_POSITIVE_I;
			span->Lsector.stopDraw  = MAX_NEGATIVE_I;
		}
#endif

		// Make all the start/stop points relative to the ring centers at each LOD
		span->Tsector.startDraw	-= LODdata[span->LOD].centerCol;
		span->Tsector.stopDraw	-= LODdata[span->LOD].centerCol;
		span->Rsector.startDraw	-= LODdata[span->LOD].centerRow;
		span->Rsector.stopDraw	-= LODdata[span->LOD].centerRow;
		span->Bsector.startDraw	-= LODdata[span->LOD].centerCol;
		span->Bsector.stopDraw	-= LODdata[span->LOD].centerCol;
		span->Lsector.startDraw	-= LODdata[span->LOD].centerRow;
		span->Lsector.stopDraw	-= LODdata[span->LOD].centerRow;
	}

	
 	// AT THIS POINT:	startDraw and stopDraw in all the rings completly specify all the lower 
	//					left corner points for squares to be drawn, but may OVER specify them.
	//					TrimCornerSet() below will adjust and round the start/stop points as
	//					necessary.
}



/***************************************************************************\
    Given the spanList which contains the world space extents of the
	inner edges of the horizontal and vertical spans of squares, we
	construct the set of all lower left corner posts for the squares we
	need to draw.

	NOTE:  We count on the truncation during the float to int conversion to
	adjust the end points side to side.  The inner/outter adjustment is explicit.
\***************************************************************************/
void RenderOTW::TrimCornerSet( void )
{
	SpanListEntry	*span;


	// Move from inner ring outward
	for ( span = firstEmptySpan-1; span>=spanList; span-- ) {

		if ( (span < spanList+2) || (span->LOD == (span-2)->LOD) ) {

			// Normal Draw.
			// Cut off all spans at their intersections with the y=x and y=-x lines
			if (span->Tsector.startDraw < -span->ring) {
				span->Tsector.startDraw = -span->ring;
			}
			if (span->Tsector.stopDraw > span->ring) {
				span->Tsector.stopDraw = span->ring;
			}
			if (span->Rsector.startDraw < -span->ring+1) {
				span->Rsector.startDraw = -span->ring+1;
			}
			if (span->Rsector.stopDraw > span->ring-1) {
				span->Rsector.stopDraw = span->ring-1;
			}
			if (span->Bsector.startDraw < -span->ring) {
				span->Bsector.startDraw = -span->ring;
			}
			if (span->Bsector.stopDraw > span->ring) {
				span->Bsector.stopDraw = span->ring;
			}
			if (span->Lsector.startDraw < -span->ring+1) {
				span->Lsector.startDraw = -span->ring+1;
			}
			if (span->Lsector.stopDraw > span->ring-1) {
				span->Lsector.stopDraw = span->ring-1;
			}

		} else {
			int LOD = span->LOD;

			if ( LOD == (span-1)->LOD ) {

				// Glue control span (also controls some connector drawing)
				if ( LODdata[LOD].glueOnBottom ) {
					// For Glue
					if (span->Bsector.startDraw < -span->ring +1-LODdata[LOD].glueOnLeft) {
						span->Bsector.startDraw = -span->ring +1-LODdata[LOD].glueOnLeft;
					}
					if (span->Bsector.stopDraw > span->ring - LODdata[LOD].glueOnLeft) {
						span->Bsector.stopDraw = span->ring - LODdata[LOD].glueOnLeft;
					}

					// For connector
					if ( LODdata[LOD].glueOnLeft ) {
						// Bottom Left
						// Clipping
						if (span->Tsector.startDraw < -span->ring) {
							span->Tsector.startDraw = -span->ring;
						}
						if (span->Tsector.stopDraw > span->ring) {
							span->Tsector.stopDraw = span->ring;
						}
						if (span->Rsector.startDraw < -span->ring) {
							span->Rsector.startDraw = -span->ring;
						}
						if (span->Rsector.stopDraw > span->ring) {
							span->Rsector.stopDraw = span->ring;
						}

						// Rounding
						span->Tsector.startDraw = (span->Tsector.startDraw -1) | 1;
						span->Tsector.stopDraw  = (span->Tsector.stopDraw  -1) | 1;
						span->Rsector.startDraw = (span->Rsector.startDraw -1) | 1;
						span->Rsector.stopDraw  = (span->Rsector.stopDraw  -1) | 1;
					} else {
						// Bottom Right
						// Clipping
						if (span->Tsector.startDraw < -span->ring) {
							span->Tsector.startDraw = -span->ring;
						}
						if (span->Tsector.stopDraw > span->ring) {
							span->Tsector.stopDraw = span->ring;
						}
						if (span->Lsector.startDraw < -span->ring) {
							span->Lsector.startDraw = -span->ring;
						}
						if (span->Lsector.stopDraw > span->ring) {
							span->Lsector.stopDraw = span->ring;
						}

						// Rounding
						span->Tsector.startDraw = span->Tsector.startDraw & ~1;
						span->Tsector.stopDraw  = span->Tsector.stopDraw  & ~1;
						span->Lsector.startDraw = (span->Lsector.startDraw -1) | 1;
						span->Lsector.stopDraw  = (span->Lsector.stopDraw  -1) | 1;
					}
				} else {
					// For Glue
					if (span->Tsector.startDraw < -span->ring +1-LODdata[LOD].glueOnLeft) {
						span->Tsector.startDraw = -span->ring +1-LODdata[LOD].glueOnLeft;
					}
					if (span->Tsector.stopDraw > span->ring - LODdata[LOD].glueOnLeft) {
						span->Tsector.stopDraw = span->ring - LODdata[LOD].glueOnLeft;
					}

					// For connector
					if ( LODdata[LOD].glueOnLeft ) {
						// Top Left
						// Clipping
						if (span->Bsector.startDraw < -span->ring) {
							span->Bsector.startDraw = -span->ring;
						}
						if (span->Bsector.stopDraw > span->ring) {
							span->Bsector.stopDraw = span->ring;
						}
						if (span->Rsector.startDraw < -span->ring) {
							span->Rsector.startDraw = -span->ring;
						}
						if (span->Rsector.stopDraw > span->ring) {
							span->Rsector.stopDraw = span->ring;
						}

						// Rounding
						span->Bsector.startDraw = (span->Bsector.startDraw -1) | 1;
						span->Bsector.stopDraw  = (span->Bsector.stopDraw  -1) | 1;
						span->Rsector.startDraw = span->Rsector.startDraw & ~1;
						span->Rsector.stopDraw  = span->Rsector.stopDraw  & ~1;
					} else {
						// Top Right
						// Clipping
						if (span->Bsector.startDraw < -span->ring) {
							span->Bsector.startDraw = -span->ring;
						}
						if (span->Bsector.stopDraw > span->ring) {
							span->Bsector.stopDraw = span->ring;
						}
						if (span->Lsector.startDraw < -span->ring) {
							span->Lsector.startDraw = -span->ring;
						}
						if (span->Lsector.stopDraw > span->ring) {
							span->Lsector.stopDraw = span->ring;
						}

						// Rounding
						span->Bsector.startDraw = span->Bsector.startDraw & ~1;
						span->Bsector.stopDraw  = span->Bsector.stopDraw  & ~1;
						span->Lsector.startDraw = span->Lsector.startDraw & ~1;
						span->Lsector.stopDraw  = span->Lsector.stopDraw  & ~1;
					}
				}

				if ( LODdata[LOD].glueOnLeft ) {
					// For Glue
					if (span->Lsector.startDraw < -span->ring+1) {
						span->Lsector.startDraw = -span->ring+1;
					}
					if (span->Lsector.stopDraw > span->ring-1) {
						span->Lsector.stopDraw = span->ring-1;
					}
				} else {
					// For Glue
					if (span->Rsector.startDraw < -span->ring+1) {
						span->Rsector.startDraw = -span->ring+1;
					}
					if (span->Rsector.stopDraw > span->ring-1) {
						span->Rsector.stopDraw = span->ring-1;
					}
				}

			} else {

				// Outter Xform span (controls some parts of connector drawing)
				if ( LODdata[LOD].glueOnBottom ) {
					if (LODdata[LOD].glueOnLeft) {
						// Bottom Left
						// Clipping
						if (span->Bsector.startDraw < -span->ring) {
							span->Bsector.startDraw = -span->ring;
						}
						if (span->Bsector.stopDraw > span->ring-1) {
							span->Bsector.stopDraw = span->ring-1;
						}
						if (span->Lsector.startDraw < -span->ring) {
							span->Lsector.startDraw = -span->ring;
						}
						if (span->Lsector.stopDraw > span->ring-1) {
							span->Lsector.stopDraw = span->ring-1;
						}
						span->Tsector.startDraw = MAX_POSITIVE_I;
						span->Tsector.stopDraw  = MAX_NEGATIVE_I;
						span->Rsector.startDraw = MAX_POSITIVE_I;
						span->Rsector.stopDraw  = MAX_NEGATIVE_I;

						// Rounding
						span->Bsector.startDraw = (span->Bsector.startDraw -1) | 1;
						span->Bsector.stopDraw  = (span->Bsector.stopDraw  -1) | 1;
						span->Lsector.startDraw = (span->Lsector.startDraw -1) | 1;
						span->Lsector.stopDraw  = (span->Lsector.stopDraw  -1) | 1;
					} else {
						// Bottom Right
						// Clipping
						if (span->Bsector.startDraw < -span->ring+1) {
							span->Bsector.startDraw = -span->ring+1;
						}
						if (span->Bsector.stopDraw > span->ring) {
							span->Bsector.stopDraw = span->ring;
						}
						if (span->Rsector.startDraw < -span->ring) {
							span->Rsector.startDraw = -span->ring;
						}
						if (span->Rsector.stopDraw > span->ring-1) {
							span->Rsector.stopDraw = span->ring-1;
						}
						span->Tsector.startDraw = MAX_POSITIVE_I;
						span->Tsector.stopDraw  = MAX_NEGATIVE_I;
						span->Lsector.startDraw = MAX_POSITIVE_I;
						span->Lsector.stopDraw  = MAX_NEGATIVE_I;

						// Rounding
						span->Bsector.startDraw = span->Bsector.startDraw & ~1;
						span->Bsector.stopDraw  = span->Bsector.stopDraw  & ~1;
						span->Rsector.startDraw = (span->Rsector.startDraw -1) | 1;
						span->Rsector.stopDraw  = (span->Rsector.stopDraw  -1) | 1;
					}
				} else {
					if (LODdata[LOD].glueOnLeft) {
						// Top Left
						// Clipping
						if (span->Tsector.startDraw < -span->ring) {
							span->Tsector.startDraw = -span->ring;
						}
						if (span->Tsector.stopDraw > span->ring-1) {
							span->Tsector.stopDraw = span->ring-1;
						}
						if (span->Lsector.startDraw < -span->ring+1) {
							span->Lsector.startDraw = -span->ring+1;
						}
						if (span->Lsector.stopDraw > span->ring) {
							span->Lsector.stopDraw = span->ring;
						}
						span->Bsector.startDraw = MAX_POSITIVE_I;
						span->Bsector.stopDraw  = MAX_NEGATIVE_I;
						span->Rsector.startDraw = MAX_POSITIVE_I;
						span->Rsector.stopDraw  = MAX_NEGATIVE_I;

						// Rounding
						span->Tsector.startDraw = (span->Tsector.startDraw -1) | 1;
						span->Tsector.stopDraw  = (span->Tsector.stopDraw  -1) | 1;
						span->Lsector.startDraw = span->Lsector.startDraw & ~1;
						span->Lsector.stopDraw  = span->Lsector.stopDraw  & ~1;
					} else {
						// Top Right
						// Clipping
						if (span->Tsector.startDraw < -span->ring+1) {
							span->Tsector.startDraw = -span->ring+1;
						}
						if (span->Tsector.stopDraw > span->ring) {
							span->Tsector.stopDraw = span->ring;
						}
						if (span->Rsector.startDraw < -span->ring+1) {
							span->Rsector.startDraw = -span->ring+1;
						}
						if (span->Rsector.stopDraw > span->ring) {
							span->Rsector.stopDraw = span->ring;
						}
						span->Bsector.startDraw = MAX_POSITIVE_I;
						span->Bsector.stopDraw  = MAX_NEGATIVE_I;
						span->Lsector.startDraw = MAX_POSITIVE_I;
						span->Lsector.stopDraw  = MAX_NEGATIVE_I;

						// Rounding
						span->Tsector.startDraw = span->Tsector.startDraw & ~1;
						span->Tsector.stopDraw  = span->Tsector.stopDraw  & ~1;
						span->Rsector.startDraw = span->Rsector.startDraw & ~1;
						span->Rsector.stopDraw  = span->Rsector.stopDraw  & ~1;
					}
				}
			}
		}
	}

	
 	// AT THIS POINT:	startDraw and stopDraw in all the rings completly and exactly specify
	//					the lower left corner points for squares to be drawn.
}



/***************************************************************************\
    Given the spanList which contains all the lower left corner posts,
	construct the expanded set which also includes every post's N, E, and NE
	neighbor.
\***************************************************************************/
void RenderOTW::BuildVertexSet( void )
{
	SpanListEntry	*span;
	SpanListEntry	*innerSpan;
	SpanListEntry	*outterSpan;
	SpanListEntry	*controlSpan;
	int				LOD;
	int				lowPos;


	// First clear the list of spans to be transformed
	for ( span = spanList; span<firstEmptySpan; span++ ) {
		span->Tsector.startXform = MAX_POSITIVE_I;
		span->Tsector.stopXform  = MAX_NEGATIVE_I;
		span->Rsector.startXform = MAX_POSITIVE_I;
		span->Rsector.stopXform  = MAX_NEGATIVE_I;
		span->Bsector.startXform = MAX_POSITIVE_I;
		span->Bsector.stopXform  = MAX_NEGATIVE_I;
		span->Lsector.startXform = MAX_POSITIVE_I;
		span->Lsector.stopXform  = MAX_NEGATIVE_I;
	}


	// Move from outter ring inward (same traversal as the drawing loop will use)
	for ( span = spanList+1; span<firstEmptySpan; span++ ) {

		if ( span->LOD == (span+1)->LOD ) {

			// We will call DrawTerrainRing on this span
			span->Tsector.startXform = min( span->Tsector.startDraw,  span->Tsector.startXform );
			span->Tsector.stopXform  = max( span->Tsector.stopDraw+1, span->Tsector.stopXform  );
			span->Rsector.startXform = min( span->Rsector.startDraw,  span->Rsector.startXform );
			span->Rsector.stopXform  = max( span->Rsector.stopDraw+1, span->Rsector.stopXform  );
			span->Bsector.startXform = min( span->Bsector.startDraw,  span->Bsector.startXform );
			span->Bsector.stopXform  = max( span->Bsector.stopDraw+1, span->Bsector.stopXform  );
			span->Lsector.startXform = min( span->Lsector.startDraw,  span->Lsector.startXform );
			span->Lsector.stopXform  = max( span->Lsector.stopDraw+1, span->Lsector.stopXform  );

			(span-1)->Tsector.startXform = min( span->Tsector.startDraw,  (span-1)->Tsector.startXform );
			(span-1)->Tsector.stopXform  = max( span->Tsector.stopDraw+1, (span-1)->Tsector.stopXform  );
			(span-1)->Rsector.startXform = min( span->Rsector.startDraw,  (span-1)->Rsector.startXform );
			(span-1)->Rsector.stopXform  = max( span->Rsector.stopDraw+1, (span-1)->Rsector.stopXform  );
			(span+1)->Bsector.startXform = min( span->Bsector.startDraw,  (span+1)->Bsector.startXform );
			(span+1)->Bsector.stopXform  = max( span->Bsector.stopDraw+1, (span+1)->Bsector.stopXform  );
			(span+1)->Lsector.startXform = min( span->Lsector.startDraw,  (span+1)->Lsector.startXform );
			(span+1)->Lsector.stopXform  = max( span->Lsector.stopDraw+1, (span+1)->Lsector.stopXform  );

		} else {

			// Skip the inner transform ring (last ring at lower detail level)
			span++;

			LOD = span->LOD;

			// We'll call draw ConnectorRing on this span (outter xform)
			// TOP
			if ( LODdata[LOD].glueOnBottom ) {
				innerSpan  = span+1;		// "glue control"
				controlSpan= span+1;		// "glue control"
				outterSpan = span-2;		// "last drawn"
			} else {
				innerSpan  = span;			// "outter xform"
				controlSpan= span;			// "outter xform"
				outterSpan = span-2;		// "last drawn"
			}
			innerSpan->Tsector.startXform  = min( controlSpan->Tsector.startDraw,		innerSpan->Tsector.startXform );
			innerSpan->Tsector.stopXform   = max( controlSpan->Tsector.stopDraw+2,		innerSpan->Tsector.stopXform  );
			lowPos = (controlSpan->Tsector.startDraw  + LODdata[LOD].glueOnLeft)	>> 1;
			outterSpan->Tsector.startXform = min( lowPos, outterSpan->Tsector.startXform );
			lowPos = (controlSpan->Tsector.stopDraw+2 + LODdata[LOD].glueOnLeft)	>> 1;
			outterSpan->Tsector.stopXform  = max( lowPos, outterSpan->Tsector.stopXform  );

			// RIGHT
			if ( LODdata[LOD].glueOnLeft ) {
				innerSpan  = span+1;		// "glue control"
				controlSpan= span+1;		// "glue control"
				outterSpan = span-2;		// "last drawn"
			} else {
				innerSpan  = span;			// "outter xform"
				controlSpan= span;			// "outter xform"
				outterSpan = span-2;		// "last drawn"
			}
			innerSpan->Rsector.startXform  = min( innerSpan->Rsector.startDraw,			innerSpan->Rsector.startXform );
			innerSpan->Rsector.stopXform   = max( innerSpan->Rsector.stopDraw+2,		innerSpan->Rsector.stopXform  );
			lowPos = (innerSpan->Rsector.startDraw  + LODdata[LOD].glueOnBottom)	>> 1;
			outterSpan->Rsector.startXform = min( lowPos, outterSpan->Rsector.startXform );
			lowPos = (innerSpan->Rsector.stopDraw+2 + LODdata[LOD].glueOnBottom)	>> 1;
			outterSpan->Rsector.stopXform  = max( lowPos, outterSpan->Rsector.stopXform  );

			// BOTTOM
			if ( LODdata[LOD].glueOnBottom ) {
				innerSpan  = span+1;		// "glue control"
				controlSpan= span;			// "outter xform"
				outterSpan = span-1;		// "inner xform"
			} else {
				innerSpan  = span+2;		// "first normal draw"
				controlSpan= span+1;		// "glue control"
				outterSpan = span-1;		// "inner xform"
			}
			innerSpan->Bsector.startXform  = min( controlSpan->Bsector.startDraw,		innerSpan->Bsector.startXform );
			innerSpan->Bsector.stopXform   = max( controlSpan->Bsector.stopDraw+2,		innerSpan->Bsector.stopXform  );
			lowPos = (controlSpan->Bsector.startDraw  + LODdata[LOD].glueOnLeft)	>> 1;
			outterSpan->Bsector.startXform = min( lowPos, outterSpan->Bsector.startXform );
			lowPos = (controlSpan->Bsector.stopDraw+2 + LODdata[LOD].glueOnLeft)	>> 1;
			outterSpan->Bsector.stopXform  = max( lowPos, outterSpan->Bsector.stopXform  );

			// LEFT
			if ( LODdata[LOD].glueOnLeft ) {
				innerSpan  = span+1;		// "glue control"
				controlSpan= span;			// "outter xform"
				outterSpan = span-1;		// "inner xform"
			} else {
				innerSpan  = span+2;		// "first normal draw"
				controlSpan= span+1;		// "glue control"
				outterSpan = span-1;		// "inner xform"
			}
			innerSpan->Lsector.startXform  = min( controlSpan->Lsector.startDraw,		innerSpan->Lsector.startXform );
			innerSpan->Lsector.stopXform   = max( controlSpan->Lsector.stopDraw+2,		innerSpan->Lsector.stopXform  );
			lowPos = (controlSpan->Lsector.startDraw  + LODdata[LOD].glueOnBottom)	>> 1;
			outterSpan->Lsector.startXform = min( lowPos, outterSpan->Lsector.startXform );
			lowPos = (controlSpan->Lsector.stopDraw+2 + LODdata[LOD].glueOnBottom)	>> 1;
			outterSpan->Lsector.stopXform  = max( lowPos, outterSpan->Lsector.stopXform  );

			span++;

			// We'll call draw gap filler on this span
			if ( LODdata[LOD].glueOnBottom ) {
				span->Bsector.startXform = min( span->Bsector.startDraw,			span->Bsector.startXform );
				span->Bsector.stopXform  = max( span->Bsector.stopDraw+1,			span->Bsector.stopXform  );
				(span+1)->Bsector.startXform = min( span->Bsector.startDraw,	(span+1)->Bsector.startXform );
				(span+1)->Bsector.stopXform  = max( span->Bsector.stopDraw+1,	(span+1)->Bsector.stopXform  );
			} else {
				span->Tsector.startXform = min( span->Tsector.startDraw,			span->Tsector.startXform );
				span->Tsector.stopXform  = max( span->Tsector.stopDraw+1,			span->Tsector.stopXform  );
				(span-1)->Tsector.startXform = min( span->Tsector.startDraw,	(span-1)->Tsector.startXform );
				(span-1)->Tsector.stopXform  = max( span->Tsector.stopDraw+1,	(span-1)->Tsector.stopXform  );
			}
			if ( LODdata[span->LOD].glueOnLeft ) {
				span->Lsector.startXform = min( span->Lsector.startDraw,			span->Lsector.startXform );
				span->Lsector.stopXform  = max( span->Lsector.stopDraw+1,			span->Lsector.stopXform  );
				(span+1)->Lsector.startXform = min( span->Lsector.startDraw,	(span+1)->Lsector.startXform );
				(span+1)->Lsector.stopXform  = max( span->Lsector.stopDraw+1,	(span+1)->Lsector.stopXform  );
			} else {
				span->Rsector.startXform = min( span->Rsector.startDraw,			span->Rsector.startXform );
				span->Rsector.stopXform  = max( span->Rsector.stopDraw+1,			span->Rsector.stopXform  );
				(span-1)->Rsector.startXform = min( span->Rsector.startDraw,	(span-1)->Rsector.startXform );
				(span-1)->Rsector.stopXform  = max( span->Rsector.stopDraw+1,	(span-1)->Rsector.stopXform  );
			}
		}
	}


#if 0
	// Try to eliminate overlapping vertex entries in adjacent rows and columns
	// Move from outter ring inward
	// TODO:  Fix this -- it occasionally removes verticies we actually need for connector rings
	// Because construction in vertical can require verts in the horizontal (and vis-versa),
	// we'd have to find the associated span and add in the verticies we want to avoid doing
	// ourselves -- probably not worth the trouble.
	for ( span = spanList; span<firstEmptySpan; span++ ) {
		span->Tsector.startXform = max( -span->ring-1, span->Tsector.startXform );
		span->Tsector.stopXform  = min(  span->ring+1, span->Tsector.stopXform  );
		span->Rsector.startXform = max( -span->ring-1, span->Rsector.startXform );
		span->Rsector.stopXform  = min(  span->ring+1, span->Rsector.stopXform  );
		span->Bsector.startXform = max( -span->ring-1, span->Bsector.startXform );
		span->Bsector.stopXform  = min(  span->ring+1, span->Bsector.stopXform  );
		span->Lsector.startXform = max( -span->ring-1, span->Lsector.startXform );
		span->Lsector.stopXform  = min(  span->ring+1, span->Lsector.stopXform  );
	}
#endif
}



/***************************************************************************\
    Step through the spans and transform all the identified verticies we'll
	need for drawing.
\***************************************************************************/
void RenderOTW::TransformVertexSet( void )
{
	SpanListEntry	*span;
	int				LOD;
	int				ring;
	SpanMinMax		*sector;

	// Move from inner ring outward
	for ( span = firstEmptySpan-1; span>=spanList; span-- ) {
		LOD		= span->LOD;
		ring	= span->ring;

		// TOP_SPAN
		sector	= &span->Tsector;
		TransformRun( ring,  sector->startXform, sector->stopXform - sector->startXform, LOD, TRUE );

		// RIGHT_SPAN
		sector	= &span->Rsector;
		TransformRun( sector->startXform,  ring, sector->stopXform - sector->startXform, LOD, FALSE );

		// BOTTOM_SPAN
		sector	= &span->Bsector;
		TransformRun( -ring, sector->startXform, sector->stopXform - sector->startXform, LOD, TRUE );

		// LEFT_SPAN
		sector	= &span->Lsector;
		TransformRun( sector->startXform, -ring, sector->stopXform - sector->startXform, LOD, FALSE );
	}
}



/***************************************************************************\
    Compute the edges of the potentially visible area of terrain.
\***************************************************************************/
void RenderOTW::ComputeBounds(void)
{
	// To avoid rounding errors (or possibly some other minor computational bug)
	// I compute culling assuming a viewpoint somewhat behind the real
	// camera postion which will always yeild a conservative estimate.
	// To be really paranoid, the CULL_BACKUP_DISTANCE is
	// a function of field of view to get a constant linear margin.
	float cullMargin		= LEVEL_POST_TO_WORLD( 2, viewpoint->GetHighLOD() );
	float backupDistance	= cullMargin / (float)sin( diagonal_half_angle );
	Tpoint	CullPoint;
	GetAt( &CullPoint );
	CullPoint.x = X() - CullPoint.x * backupDistance;
	CullPoint.y = Y() - CullPoint.y * backupDistance;
	CullPoint.z = Z() - CullPoint.z * backupDistance;
	float distanceLimit = far_clip + backupDistance;


	// Get the vertical bounds of the terrain arround our viewpoint
	float areaFloor;
	float areaCeiling;
	areaFloor	= viewpoint->GetTerrainFloor();		// -Z is up
	areaCeiling	= viewpoint->GetTerrainCeiling();	// -Z is up


	//	First compute the front and back distances of the ground patch
	//  (front may be behind the viewer, in which case it will be negative)
	//  (back will never be behind the viewer and will never be negative)
	float top	= Pitch() + diagonal_half_angle;
	float bottom= Pitch() - diagonal_half_angle;

	float aboveMin = areaFloor - CullPoint.z;		// -Z is up
	float aboveMax = areaCeiling - CullPoint.z;		// -Z is up

	float front = -1e30f;
	float back =  -1e30f;

	if ( bottom < -PI_OVER_2 ) {
		// bottom vector points down and backward
		if (aboveMin >= 0.0f) {
			// We're above ground so intersect the bottom with min height
			front = -aboveMin * (float)tan( -bottom-PI_OVER_2 );
			if ( top > 0 ) {
				// top vector points up
				back = distanceLimit;
			} else {
				// top vector points down
				back = aboveMin * (float)tan( top+PI_OVER_2 );
			}
		} else {
			// We're below ground 
			if ( top > 0 ) {
				// top is pointing upward, so intersect the top with min height
				front = -aboveMin / (float)tan( top );
				back = distanceLimit;
			} else {
				// We're below all terrain
				front = 0.0f;
				back = 0.0f;
			}
		}
	} else if ( bottom < 0 ) {
		// bottom vector points downward, but not backward
		if (aboveMin >= 0.0f) {
			// We're above ground
			if ( top >= 0 ) {
				// the top vector points upward
				back = distanceLimit;
			} else {
				// the top vector points downward as well
				back = aboveMin * (float)tan( top+PI_OVER_2 );
			}	
			if ( aboveMax > 0 ) {
				// we're above all terrain
				front = aboveMax * (float)tan( bottom+PI_OVER_2 );
			} else {
				// we're down in it
				if ( top > PI_OVER_2 ) {
					// top vector points backward
					front = aboveMax / (float)tan( PI - top );
				} else {
					// top vector points upward, but not backward
					front = 0.0f;
				}
			}
		} else {
			// We're below ground
			if ( top >= 0 ) {
				// top is pointing upward, so intersect the top with min height
				front = -aboveMin / (float)tan( top );
				back = distanceLimit;
			} else {
				// the top vector points downward as well -- we're below all terrain
				front = 0.0f;
				back = 0.0f;
			}	
		}
	} else {
		// bottom vector points upward
		if ( aboveMax > 0 ) {
			// we're above all terrain
			front = 0.0f;
			back = 0.0f;
		} else if (aboveMin >= 0) {
			// we're down in it
			back = -aboveMax / (float)tan( bottom );
			if ( top > PI_OVER_2 ) {
				// top vector points backward
				front = aboveMax * (float)tan( top-PI_OVER_2 );
			} else {
				// top vector points up, but not backward
				front = 0.0f;
			}
		} else {
			// We're below ground
			back = -aboveMax / (float)tan( bottom );
			if ( top > PI_OVER_2 ) {
				// top vector points backward
				front = aboveMax * (float)tan( top-PI_OVER_2 );
			} else {
				// top vector points up, but not backward, so intersect the top with min height
				front = -aboveMin / (float)tan( top );
			}
		}
	}

	// Clamp the front and back values at the largest distance we could ever see
	if (back > distanceLimit)	{ back = distanceLimit; }
	if (front < -distanceLimit)	{ front = -distanceLimit; }
	if (front > distanceLimit)	{ front = distanceLimit; }
	
	//
	// Now go compute the left and right edges
	//
	float A;
	float B;
	float C;
	float temp;


	// Intersect the min elevation plane and the side of the view volume to get the right edge.
	// (left side is a mirror of the right side)

	// With the viewer looking down the x axis,
	// the right side of view volume is defined by the origin (eye point) and 
	Tpoint	Corner1 = { 1.0f, (float)tan(diagonal_half_angle),  1.0f };
	Tpoint	Corner2 = { 1.0f, (float)tan(diagonal_half_angle), -1.0f };


	// This plane should then be rotated in pitch about the y axis (look up/down)
	// Rotate the two points
	// NOTE:  We're computing the sides with pitch always pointing downward because
	//		  the plane/plane intersection does the wrong thing when looking up.
	// NOTE2: It would seem to me that I should have a minus sign in front of the pitch
	//		  term in the following two lines.  I may have a sign wrong somewhere else, 
	//        though, since it seems to work correctly as written...
	float	sinPitch	= (float)sin( fabs(Pitch()) );
	float	cosPitch	= (float)cos( fabs(Pitch()) );
	float	sinYaw		= (float)sin( Yaw() );
	float	cosYaw		= (float)cos( Yaw() );

	temp		= Corner1.x * cosPitch	- Corner1.z * sinPitch;
	Corner1.z	= Corner1.x * sinPitch	+ Corner1.z * cosPitch;
	Corner1.x	= temp;

	temp		= Corner2.x * cosPitch	- Corner2.z * sinPitch;
	Corner2.z	= Corner2.x * sinPitch	+ Corner2.z * cosPitch;
	Corner2.x	= temp;


	// Construct the normal to the plane using the cross product of the vectors from the eye point
	// (Note:  The normal components ARE the A, B, and C coefficients of the plane equation)
	A = Corner1.y*Corner2.z - Corner1.z*Corner2.y;
	B = Corner1.z*Corner2.x - Corner1.x*Corner2.z;
	C = Corner1.x*Corner2.y - Corner1.y*Corner2.x;

	// Since the origin is one of the points on the plane, D is necessarily zero
	//
	// The intersection of the plane with the minimum terrain feature height
	// is the worst case.  This is the plane z = (alt - areaFloor)
	// We use height above minimum since we set the eyepoint at the origin and positive z is down.
	//
	// The line equation given by performing the substitution for z in the plane equation is:
	// Ax + By + C*aboveMin + D = 0;
	// Where A B C and D are the coefficients of the plane equation
	//
	// The Line equation coefficients are:
	// a = A;
	// b = B;
	// c = C*aboveMin + D;
	//
	// Convert C in place so A, B and C become the line equation coefficients
	C *= aboveMin;


	// Now get two points on this line and two points on its reflection across the X axis
	// point one on each side is at X = front.  Point two on each side is at X = back.
	rightY1	= -(A*front+C)/B;	rightY2	= -(A*back+C)/B;
	leftY1	= -rightY1;			leftY2	= -rightY2;

	// Rotate the four points about the z axis to account for the viewer's yaw angle
	// and shift them out to the viewers location in world space
	rightX1	= cosYaw*front - sinYaw*rightY1 + CullPoint.x;
	rightY1	= sinYaw*front + cosYaw*rightY1 + CullPoint.y;
	
	rightX2	= cosYaw*back - sinYaw*rightY2 + CullPoint.x;
	rightY2	= sinYaw*back + cosYaw*rightY2 + CullPoint.y;
	
	leftX1	= cosYaw*front - sinYaw*leftY1 + CullPoint.x;
	leftY1	= sinYaw*front + cosYaw*leftY1 + CullPoint.y;

	leftX2	= cosYaw*back - sinYaw*leftY2 + CullPoint.x;
	leftY2	= sinYaw*back + cosYaw*leftY2 + CullPoint.y;


	//
	// Build the four edges bounding the ground patch
	//
	right_edge.SetupWithPoints( rightX1, rightY1, rightX2, rightY2 );
	right_edge.Normalize();
	left_edge.SetupWithPoints( leftX1, leftY1, leftX2, leftY2 );
	left_edge.Normalize();
	front_edge.SetupWithPoints( leftX1, leftY1, rightX1, rightY1 );
	front_edge.Normalize();
	back_edge.SetupWithPoints( leftX2, leftY2, rightX2, rightY2 );
	back_edge.Normalize();

#if 0
	// Sometimes we end up here with a bow tie shaped area -- bad -- so we hack a fix
	if (right_edge.LeftOf( leftX1, leftY1 )) {
		float t;
		t = leftX1;
		leftX1 = rightX1;
		rightX1 = t;
		t = leftY1;
		leftY1 = rightY1;
		rightY1 = t;

		right_edge.SetupWithPoints( rightX1, rightY1, rightX2, rightY2 );
		right_edge.Normalize();
		left_edge.SetupWithPoints( leftX1, leftY1, leftX2, leftY2 );
		left_edge.Normalize();
		front_edge.SetupWithPoints( leftX1, leftY1, rightX1, rightY1 );
		front_edge.Normalize();
		back_edge.SetupWithPoints( leftX2, leftY2, rightX2, rightY2 );
		back_edge.Normalize();
	}

	ShiAssert( (leftX1 == rightX1 && leftY1 == rightY1) || right_edge.RightOf( leftX1, leftY1 ) );
	ShiAssert( (leftX1 == rightX1 && leftY1 == rightY1) || left_edge.LeftOf( rightX1, rightY1 ) );
	ShiAssert( right_edge.RightOf( leftX2, leftY2 ) );
	ShiAssert( left_edge.LeftOf( rightX2, rightY2 ) );
#endif


#ifdef TWO_D_MAP_AVAILABLE
	if (twoDmode) {
		// Draw the view volume representation (assuming its in world space)
		SetColor( 0xFF0000A0 );
		Render2DLine((UInt16)((xRes>>1) + TWODSCALE*( WORLD_TO_FLOAT_GLOBAL_POST(rightY1-viewpoint->Y()) )),	(UInt16)((yRes>>1) - TWODSCALE*( WORLD_TO_GLOBAL_POST(rightX1-viewpoint->X()) )), 
					 (UInt16)((xRes>>1) + TWODSCALE*( WORLD_TO_FLOAT_GLOBAL_POST(rightY2-viewpoint->Y()) )),	(UInt16)((yRes>>1) - TWODSCALE*( WORLD_TO_GLOBAL_POST(rightX2-viewpoint->X()) )) );                           
		Render2DLine((UInt16)((xRes>>1) + TWODSCALE*( WORLD_TO_FLOAT_GLOBAL_POST(leftY1 -viewpoint->Y()) )),	(UInt16)((yRes>>1) - TWODSCALE*( WORLD_TO_GLOBAL_POST(leftX1 -viewpoint->X()) )),  
					 (UInt16)((xRes>>1) + TWODSCALE*( WORLD_TO_FLOAT_GLOBAL_POST(leftY2 -viewpoint->Y()) )),	(UInt16)((yRes>>1) - TWODSCALE*( WORLD_TO_GLOBAL_POST(leftX2 -viewpoint->X()) )) );                           
		Render2DLine((UInt16)((xRes>>1) + TWODSCALE*( WORLD_TO_FLOAT_GLOBAL_POST(leftY1 -viewpoint->Y()) )),	(UInt16)((yRes>>1) - TWODSCALE*( WORLD_TO_GLOBAL_POST(leftX1 -viewpoint->X()) )),  
					 (UInt16)((xRes>>1) + TWODSCALE*( WORLD_TO_FLOAT_GLOBAL_POST(rightY1-viewpoint->Y()) )),	(UInt16)((yRes>>1) - TWODSCALE*( WORLD_TO_GLOBAL_POST(rightX1-viewpoint->X()) )) );                           
		Render2DLine((UInt16)((xRes>>1) + TWODSCALE*( WORLD_TO_FLOAT_GLOBAL_POST(leftY2 -viewpoint->Y()) )),	(UInt16)((yRes>>1) - TWODSCALE*( WORLD_TO_GLOBAL_POST(leftX2 -viewpoint->X()) )),  
					 (UInt16)((xRes>>1) + TWODSCALE*( WORLD_TO_FLOAT_GLOBAL_POST(rightY2-viewpoint->Y()) )),	(UInt16)((yRes>>1) - TWODSCALE*( WORLD_TO_GLOBAL_POST(rightX2-viewpoint->X()) )) );
	}
#endif
}
