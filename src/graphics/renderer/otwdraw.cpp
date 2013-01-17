/***************************************************************************\
    OTWdraw.cpp
    Scott Randolph
    January 2, 1995

    This class provides 3D drawing functions specific to rendering out the
	window views including terrain.

	This file contains the implementations of the terrain polygon
	drawing functions.
\***************************************************************************/
#include <math.h>
#include "Tmap.h"
#include "Tpost.h"
#include "TerrTex.h"
#include "FarTex.h"
#include "RViewPnt.h"
#include "RenderOW.h"
// sfr: @TODO remove this hack
#include "Falclib/Include/IsBad.h"

//#define SET_FG_COLOR_ON_FLAT		// Call SetColor for each flat shaded terrain chunck


/***************************************************************************
    Helper function for DrawTerrainRing.
***************************************************************************/
void RenderOTW::DrawTerrainSquare( int r, int c, int LOD )
{
	TerrainVertex		*v0, *v1, *v2, *v3;
	Tpost				*post;


	// Get the vertecies required to draw this square
	v0 = vertexBuffer[LOD] + maxSpanExtent*r + c;			// South-West
	v1 = vertexBuffer[LOD] + maxSpanExtent*(r+1) + c;		// North-West
	v2 = v0+1;												// South-East
	v3 = v1+1;												// North-East


#ifdef TWO_D_MAP_AVAILABLE
	if (twoDmode) {

		ShiAssert( v0->clipFlag == ON_SCREEN );
		ShiAssert( v1->clipFlag == ON_SCREEN );
		ShiAssert( v2->clipFlag == ON_SCREEN );
		ShiAssert( v3->clipFlag == ON_SCREEN );

		// Draw the square represented by this lower left corner
		SetColor( 0x80400000 );
		Render2DTri((UInt16)(v0->x),					(UInt16)(v0->y),
					(UInt16)(v0->x+(TWODSCALE<<LOD)),	(UInt16)(v0->y),
					(UInt16)(v0->x+(TWODSCALE<<LOD)),	(UInt16)(v0->y-(TWODSCALE<<LOD)));
		Render2DTri((UInt16)(v0->x),					(UInt16)(v0->y),
					(UInt16)(v0->x+(TWODSCALE<<LOD)),	(UInt16)(v0->y-(TWODSCALE<<LOD)),
					(UInt16)(v0->x),					(UInt16)(v0->y-(TWODSCALE<<LOD)));

		return;
	}
#endif


	// If all verticies are clipped by the same edge, skip this square
	if (v0->clipFlag & v1->clipFlag & v2->clipFlag & v3->clipFlag) {				
		return;														
	}

	context.RestoreState(v0->RenderingStateHandle);
	
	// If required, get the post which will provide the texture for this segment
	// and setup the texture coordinates at the corners of this square
	if ( v0->RenderingStateHandle > STATE_GOURAUD 
		//&& !F4IsBadReadPtr(v0->post, sizeof(Tpost)) // JB 010318 CTD (too much CPU)
	) {

		post = v0->post;

		if (LOD <= TheMap.LastNearTexLOD()) {
			TheTerrTextures.Select( &context, post->texID );
		} 
		else {
			TheFarTextures.Select( &context, post->texID );
		}

		v0->u = post->u;
		v0->v = post->v;
		v1->u = post->u;
		v1->v = post->v - post->d;
		v2->u = post->u + post->d;
		v2->v = post->v;
		v3->u = post->u + post->d;
		v3->v = post->v - post->d;

// ShiAssert(v0->u <= 1.0f && v1->u <= 1.0f && v2->u <= 1.0f);
// ShiAssert(v0->v <= 1.0f && v1->v <= 1.0f && v2->v <= 1.0f);
// ShiAssert(v0->u >= 0.0 && v1->u >= 0.0 && v2->u >= 0.0);
// ShiAssert(v0->v >= 0.0 && v1->v >= 0.0 && v2->v >= 0.0);

#if defined( SET_FG_COLOR_ON_FLAT )
	} else if ( v0->RenderingStateHandle == STATE_SOLID ) {
		SetColor( (FloatToInt32(v0->r * 255.9f))		|
				  (FloatToInt32(v0->g * 255.9f) << 8)	|
				  (FloatToInt32(v0->b * 255.9f) << 16) |
				  (FloatToInt32(v0->a * 255.9f) << 24)   );
#endif
	}

	// Draw the square
	DrawSquare( v0,v1,v3,v2,CULL_ALLOW_CW,false,true); //JAM 14Sep03
}


/***************************************************************************
    Draw an element of a connector ring.  Expect the high detail LOD number
	and the r/c address of the lower left corner in highres units.  We'll 
	handle alignment with the low detail data here.
***************************************************************************/
void RenderOTW::DrawUpConnector( int r, int c, int LOD ) {

	TerrainVertex	*v0, *v1, *v2, *v3, *v4;
	int				lowRow;
	int				lowCol;
	int				lowKeyOffset;
	int				highKeyOffset;
	Tpost			*post;


	// Compute the corresponding post locations in the lower detail level
	// (Include adjustment for misalignment between levels)
	lowRow = (r+1 + LODdata[LOD].glueOnBottom) >> 1;
	lowCol = (c   + LODdata[LOD].glueOnLeft)   >> 1;

	// Compute the offsets of the key vetecies
	highKeyOffset = maxSpanExtent*r      + c;
	lowKeyOffset  = maxSpanExtent*lowRow + lowCol;

	// Fetch the required vertecies
	v2 = vertexBuffer[LOD+1] + lowKeyOffset;
	v3 = vertexBuffer[LOD+1] + lowKeyOffset+1;

	v1 = vertexBuffer[LOD] + highKeyOffset;
	v0 = vertexBuffer[LOD] + highKeyOffset+1;
	v4 = vertexBuffer[LOD] + highKeyOffset+2;


	// Skip this segment if it is entirely clipped
	if (v0->clipFlag & v1->clipFlag & v2->clipFlag & v3->clipFlag & v4->clipFlag) {
		return;
	}

	context.RestoreState(v0->RenderingStateHandle);

	// If required, get the post which will provide the texture for this segment
	if ( v0->RenderingStateHandle > STATE_GOURAUD 
		&& !F4IsBadReadPtr(viewpoint, sizeof(RViewPoint))) { // JB 010408 CTD
		post = viewpoint->GetPost( lowRow-1 + LODdata[LOD+1].centerRow, 
								   lowCol   + LODdata[LOD+1].centerCol, LOD+1 );
		ShiAssert( post );

		// Select the texture
		if (LOD < TheMap.LastNearTexLOD()) {
			TheTerrTextures.Select( &context, post->texID );
		} else {
			TheFarTextures.Select( &context, post->texID );
		}

		// Set texture coordinates
		v0->v = post->v - (post->d*0.5f);
		v0->u = post->u + (post->d*0.5f);
		v1->v = v0->v;
		v1->u = post->u;				
		v2->v = post->v - post->d;		
		v2->u = post->u;				
		v3->v = v2->v;		
		v3->u = post->u + post->d;		
		v4->v = v0->v;				
		v4->u = v3->u;		
#if defined( SET_FG_COLOR_ON_FLAT )
	} else if ( v0->RenderingStateHandle == STATE_SOLID ) {
		SetColor( (FloatToInt32(v0->r * 255.9f))		|
				  (FloatToInt32(v0->g * 255.9f) << 8)	|
				  (FloatToInt32(v0->b * 255.9f) << 16) |
				  (FloatToInt32(v0->a * 255.9f) << 24)   );
#endif
	}

																				
#ifdef TWO_D_MAP_AVAILABLE
	if (twoDmode) {
		SetColor( 0x80004000 );

		if ( lowCol <= -lowRow+1 ) {
			// Mark as end piece
			SetColor( 0x80000040 );
		} else {
			// Not left end -- so we WILL use the leftmost triangle
			ShiAssert( v1->clipFlag == ON_SCREEN );
		}

		ShiAssert( v0->clipFlag == ON_SCREEN );
		ShiAssert( v2->clipFlag == ON_SCREEN );
		ShiAssert( v3->clipFlag == ON_SCREEN );

		if ( lowRow <= lowCol+1 ) {
			// Mark as end piece
			SetColor( 0x80000040 );
		} else {
			// Not right end -- so we WILL use the rightmost triangle
			ShiAssert( v4->clipFlag == ON_SCREEN );
		}

#if 1
		Render2DTri((UInt16)(v2->x),	(UInt16)(v2->y),
					(UInt16)(v1->x),	(UInt16)(v1->y),
					(UInt16)(v4->x),	(UInt16)(v4->y));
		Render2DTri((UInt16)(v2->x),	(UInt16)(v2->y),
					(UInt16)(v3->x),	(UInt16)(v3->y),
					(UInt16)(v4->x),	(UInt16)(v4->y));
#else
		Render2DTri((UInt16)(v2->x),						(UInt16)(v2->y),
					(UInt16)(v2->x+(TWODSCALE<<1<<LOD)),	(UInt16)(v2->y),
					(UInt16)(v2->x+(TWODSCALE<<1<<LOD)),	(UInt16)(v2->y+(TWODSCALE<<LOD)));
		Render2DTri((UInt16)(v2->x),						(UInt16)(v2->y),
					(UInt16)(v2->x+(TWODSCALE<<1<<LOD)),	(UInt16)(v2->y+(TWODSCALE<<LOD)),
					(UInt16)(v2->x),						(UInt16)(v2->y+(TWODSCALE<<LOD)));
#endif
		return;
	}
#endif

	if ( lowCol <= -lowRow+1 ) {
		// Left end -- skip first triangle
		DrawSquare(v0,v2,v3,v4,CULL_ALLOW_CW,false,true); //JAM 14Sep03
	} else if ( lowRow <= lowCol+1 ) {
		// Right end -- skip last triangle
		DrawSquare(v0,v1,v2,v3,CULL_ALLOW_CW,false,true); //JAM 14Sep03
	} else {
		// Interior segment -- draw it all
		DrawSquare(v0,v1,v2,v3,CULL_ALLOW_CW,false,true); //JAM 14Sep03
		DrawTriangle( v0, v3, v4,CULL_ALLOW_CW,false,true); //JAM 14Sep03
	}
}



/***************************************************************************\
    Draw an element of a connector ring.  Expect the high detail LOD number
	and the r/c address of the upper left corner in highres units.  We'll 
	handle alignment with the low detail data here.
\***************************************************************************/
void RenderOTW::DrawDownConnector( int r, int c, int LOD ) {															\

	TerrainVertex	*v0, *v1, *v2, *v3, *v4;
	int				lowRow;
	int				lowCol;
	int				lowKeyOffset;
	int				highKeyOffset;
	Tpost			*post;


	// Compute the corresponding post locations in the lower detail level
	// (Include adjust for misalignment between levels)
	lowRow = (r-1 + LODdata[LOD].glueOnBottom) >> 1;
	lowCol = (c   + LODdata[LOD].glueOnLeft)   >> 1;

	// Compute the offsets of the key vetecies
	highKeyOffset = maxSpanExtent*r      + c;
	lowKeyOffset  = maxSpanExtent*lowRow + lowCol;


	// Fetch the required vertecies
	v3 = vertexBuffer[LOD+1] + lowKeyOffset;
	v2 = vertexBuffer[LOD+1] + lowKeyOffset+1;

	v4 = vertexBuffer[LOD] + highKeyOffset;
	v0 = vertexBuffer[LOD] + highKeyOffset+1;
	v1 = vertexBuffer[LOD] + highKeyOffset+2;


	// Skip this segment if it is entirely clipped
	if (v0->clipFlag & v1->clipFlag & v2->clipFlag & v3->clipFlag & v4->clipFlag) {
		return;
	}

	context.RestoreState(v3->RenderingStateHandle);
	
	// If required, set up the texture for this segment
	if ( v3->RenderingStateHandle > STATE_GOURAUD ) {
		post = v3->post;
		ShiAssert( post );

		// Select the texture
		if (LOD < TheMap.LastNearTexLOD()) {
			TheTerrTextures.Select( &context, post->texID );
		} else {
			TheFarTextures.Select( &context, post->texID );
		}

		// Set texture coordinates
		v0->v = post->v - (post->d*0.5f);	
		v0->u = post->u + (post->d*0.5f);	
		v1->v = v0->v;
		v1->u = post->u + post->d;			
		v2->v = post->v;					
		v2->u = v1->u;			
		v3->v = post->v;	
		v3->u = post->u;					
		v4->v = v1->v;	
		v4->u = post->u;					
#if defined( SET_FG_COLOR_ON_FLAT )
	} else if ( v3->RenderingStateHandle == STATE_SOLID ) {
		SetColor( ((FloatToInt32(v0->r * 255.9f) & 0xFF))		|
				  ((FloatToInt32(v0->g * 255.9f) & 0xFF) << 8)	|
				  ((FloatToInt32(v0->b * 255.9f) & 0xFF) << 16) |
				  ((FloatToInt32(v0->a * 255.9f) & 0xFF) << 24)   );
#endif
	}

																				
#ifdef TWO_D_MAP_AVAILABLE
	if (twoDmode) {
		SetColor( 0x80004000 );

		if ( lowCol <= lowRow ) {
			// Mark as end piece
			SetColor( 0x80000040 );
		} else {
			// Not left end -- so we WILL use the leftmost triangle
			ShiAssert( v4->clipFlag == ON_SCREEN );
		}

		ShiAssert( v0->clipFlag == ON_SCREEN );
		ShiAssert( v2->clipFlag == ON_SCREEN );
		ShiAssert( v3->clipFlag == ON_SCREEN );

		if ( lowCol >= -lowRow ) {
			// Mark as end piece
			SetColor( 0x80000040 );
		} else {
			// Not right end -- so we WILL use the rightmost triangle
			ShiAssert( v1->clipFlag == ON_SCREEN );
		}

#if 1
		Render2DTri((UInt16)(v2->x),	(UInt16)(v2->y),
					(UInt16)(v1->x),	(UInt16)(v1->y),
					(UInt16)(v4->x),	(UInt16)(v4->y));
		Render2DTri((UInt16)(v2->x),	(UInt16)(v2->y),
					(UInt16)(v3->x),	(UInt16)(v3->y),
					(UInt16)(v4->x),	(UInt16)(v4->y));
#else
		Render2DTri((UInt16)(v2->x),						(UInt16)(v2->y),
					(UInt16)(v2->x-(TWODSCALE<<1<<LOD)),	(UInt16)(v2->y),
					(UInt16)(v2->x-(TWODSCALE<<1<<LOD)),	(UInt16)(v2->y-(TWODSCALE<<LOD)));
		Render2DTri((UInt16)(v2->x),						(UInt16)(v2->y),
					(UInt16)(v2->x-(TWODSCALE<<1<<LOD)),	(UInt16)(v2->y-(TWODSCALE<<LOD)),
					(UInt16)(v2->x),						(UInt16)(v2->y-(TWODSCALE<<LOD)));
#endif
		return;
	}
#endif

	if ( lowCol <= lowRow ) {
		// Left end -- skip last triangle
		DrawSquare(v0,v1,v2,v3,CULL_ALLOW_CW,false,true); //JAM 14Sep03
	} else if ( lowCol >= -lowRow ) {
		// Right end -- skip first triangle
		DrawSquare(v0,v2,v3,v4,CULL_ALLOW_CW,false,true); //JAM 14Sep03
	} else {
		// Interior segment -- draw it all
		DrawSquare(v0,v1,v2,v3,CULL_ALLOW_CW,false,true); //JAM 14Sep03
		DrawTriangle(v0,v3,v4,CULL_ALLOW_CW,false,true); //JAM 14Sep03
	}
}



/***************************************************************************\
    Draw an element of a connector ring.  Expect the high detail LOD number
	and the r/c address of the lower left corner in highres units.  We'll 
	handle alignment with the low detail data here.
\***************************************************************************/
void RenderOTW::DrawRightConnector( int r, int c, int LOD ) {															\

	TerrainVertex	*v0, *v1, *v2, *v3, *v4;
	int				lowRow;
	int				lowCol;
	int				lowKeyOffset;
	int				highKeyOffset;
	Tpost			*post;


	// Compute the corresponding post locations in the lower detail level
	// (Include adjust for misalignment between levels)
	lowRow = (r   + LODdata[LOD].glueOnBottom) >> 1;
	lowCol = (c+1 + LODdata[LOD].glueOnLeft)   >> 1;

	// Compute the offsets of the key vetecies
	highKeyOffset = maxSpanExtent*r      + c;
	lowKeyOffset  = maxSpanExtent*lowRow + lowCol;


	// Fetch the required vertecies
	v3 = vertexBuffer[LOD+1] + lowKeyOffset;
	v2 = vertexBuffer[LOD+1] + lowKeyOffset+maxSpanExtent;

	v4 = vertexBuffer[LOD] + highKeyOffset;
	v0 = vertexBuffer[LOD] + highKeyOffset+maxSpanExtent;
	v1 = vertexBuffer[LOD] + highKeyOffset+maxSpanExtent+maxSpanExtent;


	// Skip this segment if it is entirely clipped
	if (v0->clipFlag & v1->clipFlag & v2->clipFlag & v3->clipFlag & v4->clipFlag) {
		return;
	}

	// If required, get the post which will provide the texture for this segment
	if ( v0->RenderingStateHandle > STATE_GOURAUD ) {
		post = viewpoint->GetPost( lowRow   + LODdata[LOD+1].centerRow, 
								   lowCol-1 + LODdata[LOD+1].centerCol, LOD+1 );
		ShiAssert( post );

		context.RestoreState(v0->RenderingStateHandle);

		// Select the texture
		if (LOD < TheMap.LastNearTexLOD()) {
			TheTerrTextures.Select( &context, post->texID );
		} else {
			TheFarTextures.Select( &context, post->texID );
		}

		// Set texture coordinates
		v0->v = post->v - (post->d*0.5f);
		v0->u = post->u + (post->d*0.5f);
		v1->v = post->v - post->d;				
		v1->u = v0->u;
		v2->v = v1->v;				
		v2->u = post->u + post->d;		
		v3->v = post->v;		
		v3->u = v2->u;		
		v4->v = post->v;		
		v4->u = v0->u;
#if defined( SET_FG_COLOR_ON_FLAT )
	} else if ( v0->RenderingStateHandle == STATE_SOLID ) {
		SetColor( ((FloatToInt32(v0->r * 255.9f) & 0xFF))		|
				  ((FloatToInt32(v0->g * 255.9f) & 0xFF) << 8)	|
				  ((FloatToInt32(v0->b * 255.9f) & 0xFF) << 16) |
				  ((FloatToInt32(v0->a * 255.9f) & 0xFF) << 24)   );
#endif
	}

																				
#ifdef TWO_D_MAP_AVAILABLE
	if (twoDmode) {
		SetColor( 0x80004000 );

		if ( lowRow >= lowCol-1 ) {
			// Mark as end piece
			SetColor( 0x80000040 );
		} else {
			// Not top corner -- will use upper triangle
			ShiAssert( v1->clipFlag == ON_SCREEN );
		}
		ShiAssert( v0->clipFlag == ON_SCREEN );
		ShiAssert( v2->clipFlag == ON_SCREEN );
		ShiAssert( v3->clipFlag == ON_SCREEN );

		if ( lowRow <= 1-lowCol ) {
			// Mark as end piece
			SetColor( 0x80000040 );
		} else {
			// Not bottom corner -- will use bottom triangle
			ShiAssert( v4->clipFlag == ON_SCREEN );
		}

#if 1
		Render2DTri((UInt16)(v2->x),	(UInt16)(v2->y),
					(UInt16)(v1->x),	(UInt16)(v1->y),
					(UInt16)(v4->x),	(UInt16)(v4->y));
		Render2DTri((UInt16)(v2->x),	(UInt16)(v2->y),
					(UInt16)(v3->x),	(UInt16)(v3->y),
					(UInt16)(v4->x),	(UInt16)(v4->y));
#else
		Render2DTri((UInt16)(v2->x),					(UInt16)(v2->y),
					(UInt16)(v2->x-(TWODSCALE<<LOD)),	(UInt16)(v2->y),
					(UInt16)(v2->x-(TWODSCALE<<LOD)),	(UInt16)(v2->y+(TWODSCALE<<1<<LOD)));
		Render2DTri((UInt16)(v2->x),					(UInt16)(v2->y),
					(UInt16)(v2->x-(TWODSCALE<<LOD)),	(UInt16)(v2->y+(TWODSCALE<<1<<LOD)),
					(UInt16)(v2->x),					(UInt16)(v2->y+(TWODSCALE<<1<<LOD)));
#endif
		return;
	}
#endif

	if ( lowRow >= lowCol-1 ) {
		// Top corner -- skip first (top) triangle
		DrawSquare(v0,v2,v3,v4,CULL_ALLOW_CW,false,true); //JAM 14Sep03
	} else if ( lowRow <= 1-lowCol ) {
		// Bottom corner -- skip last (bottom) triangle
		DrawSquare(v0,v1,v2,v3,CULL_ALLOW_CW,false,true); //JAM 14Sep03
	} else {
		// Interior segment -- draw it all
		DrawSquare(v0,v1,v2,v3,CULL_ALLOW_CW,false,true); //JAM 14Sep03
		DrawTriangle(v0,v3,v4,CULL_ALLOW_CW,false,true); //JAM 14Sep03
	}
}



/***************************************************************************\
    Draw an element of a connector ring.  Expect the high detail LOD number
	and the r/c address of the lower right corner in highres units.  We'll 
	handle alignment with the low detail data here.
\***************************************************************************/
void RenderOTW::DrawLeftConnector( int r, int c, int LOD ) {

	TerrainVertex	*v0, *v1, *v2, *v3, *v4;
	int				lowRow;
	int				lowCol;
	int				lowKeyOffset;
	int				highKeyOffset;
	Tpost			*post;


	// Compute the corresponding post locations in the lower detail level
	// (Include adjust for misalignment between levels)
	lowRow = (r   + LODdata[LOD].glueOnBottom) >> 1;
	lowCol = (c-1 + LODdata[LOD].glueOnLeft)   >> 1;

	// Compute the offsets of the key vetecies
	highKeyOffset = maxSpanExtent*r      + c;
	lowKeyOffset  = maxSpanExtent*lowRow + lowCol;


	// Fetch the required vertecies
	v2 = vertexBuffer[LOD+1] + lowKeyOffset;
	v3 = vertexBuffer[LOD+1] + lowKeyOffset+maxSpanExtent;

	v1 = vertexBuffer[LOD] + highKeyOffset;
	v0 = vertexBuffer[LOD] + highKeyOffset+maxSpanExtent;
	v4 = vertexBuffer[LOD] + highKeyOffset+maxSpanExtent+maxSpanExtent;


	// Skip this segment if it is entirely clipped
	if (v0->clipFlag & v1->clipFlag & v2->clipFlag & v3->clipFlag & v4->clipFlag) {
		return;
	}

	context.RestoreState(v2->RenderingStateHandle);

	// If required, set up the texture for this segment
	if ( v2->RenderingStateHandle > STATE_GOURAUD ) {
		post = v2->post;
		ShiAssert( post );

		// Select the texture
		if (LOD < TheMap.LastNearTexLOD()) {
			TheTerrTextures.Select( &context, post->texID );
		} else {
			TheFarTextures.Select( &context, post->texID );
		}

		// Set texture coordinates
		v0->v = post->v - (post->d*0.5f);
		v0->u = post->u + (post->d*0.5f);
		v1->v = post->v;		
		v1->u = v0->u;
		v2->v = post->v;		
		v2->u = post->u;		
		v3->v = post->v - post->d;				
		v3->u = post->u;		
		v4->v = v3->v;				
		v4->u = v0->u;
#if defined( SET_FG_COLOR_ON_FLAT )
	} else if ( v2->RenderingStateHandle == STATE_SOLID ) {
		SetColor( ((FloatToInt32(v0->r * 255.9f) & 0xFF))		|
				  ((FloatToInt32(v0->g * 255.9f) & 0xFF) << 8)	|
				  ((FloatToInt32(v0->b * 255.9f) & 0xFF) << 16) |
				  ((FloatToInt32(v0->a * 255.9f) & 0xFF) << 24)   );
#endif
	}

																				
#ifdef TWO_D_MAP_AVAILABLE
	if (twoDmode) {
		SetColor( 0x80004000 );

		if ( lowRow >= -lowCol ) {
			// Mark as end piece
			SetColor( 0x80000040 );
		} else {
			// Not top corner -- so we WILL use the upper triangle
			ShiAssert( v4->clipFlag == ON_SCREEN );
		}

		ShiAssert( v0->clipFlag == ON_SCREEN );
		ShiAssert( v2->clipFlag == ON_SCREEN );
		ShiAssert( v3->clipFlag == ON_SCREEN );

		if ( lowRow <= lowCol ) {
			// Mark as end piece
			SetColor( 0x80000040 );
		} else {
			// Not bottom corner -- so we WILL use the bottom triangle
			ShiAssert( v1->clipFlag == ON_SCREEN );
		}

#if 1
		Render2DTri((UInt16)(v2->x),	(UInt16)(v2->y),
					(UInt16)(v1->x),	(UInt16)(v1->y),
					(UInt16)(v4->x),	(UInt16)(v4->y));
		Render2DTri((UInt16)(v2->x),	(UInt16)(v2->y),
					(UInt16)(v3->x),	(UInt16)(v3->y),
					(UInt16)(v4->x),	(UInt16)(v4->y));
#else
		Render2DTri((UInt16)(v3->x),					(UInt16)(v3->y),
					(UInt16)(v3->x+(TWODSCALE<<LOD)),	(UInt16)(v3->y),
					(UInt16)(v3->x+(TWODSCALE<<LOD)),	(UInt16)(v3->y+(TWODSCALE<<1<<LOD)));
		Render2DTri((UInt16)(v3->x),					(UInt16)(v3->y),
					(UInt16)(v3->x+(TWODSCALE<<LOD)),	(UInt16)(v3->y+(TWODSCALE<<1<<LOD)),
					(UInt16)(v3->x),					(UInt16)(v3->y+(TWODSCALE<<1<<LOD)));
#endif
		return;
	}
#endif

	if ( lowRow >= -lowCol ) {
		// Top corner -- skip last (top) triangle
		DrawSquare(v0,v1,v2,v3,CULL_ALLOW_CW,false,true); //JAM 14Sep03
	} else if ( lowRow <= lowCol ) {
		// Bottom corner -- skip first (bottom) triangle
		DrawSquare(v0,v2,v3,v4,CULL_ALLOW_CW,false,true); //JAM 14Sep03
	} else {
		// Interior segment -- draw it all
		DrawSquare(v0,v1,v2,v3,CULL_ALLOW_CW,false,true); //JAM 14Sep03
		DrawTriangle(v0,v3,v4,CULL_ALLOW_CW,false,true); //JAM 14Sep03
	}
}
