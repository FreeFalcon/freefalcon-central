/***************************************************************************\
    PolyWriter.cpp
    Scott Randolph
    March 23, 1998

    This handles writing the disk representation of the run time primitives.
\***************************************************************************/
#include <io.h>
#include "BSPnodeWriter.h"
#include "PolyWriter.h"


int StorePrimitive( Prim *prim )
{
	int offset;

	// Do our type specific handling
	switch( prim->type ) {
	  case PointF:
		offset = StorePrimPointFC( (PrimPointFC*)prim );
		break;
	  case LineF:
		offset = StorePrimLineFC( (PrimLineFC*)prim );
		break;
//	  case LtStr:
//		offset = StorePrimLtStr( (PrimLineFC*)prim );
	  case F:
	  case AF:
		offset = StorePolyFC( (PolyFC*)prim );
		break;
	  case FL:
	  case AFL:
		offset = StorePolyFCN( (PolyFCN*)prim );
		break;
	  case G:
	  case AG:
		offset = StorePolyVC( (PolyVC*)prim );
		break;
	  case GL:
	  case AGL:
		offset = StorePolyVCN( (PolyVCN*)prim );
		break;
	  case Tex:
	  case ATex:
	  case CTex:
	  case CATex:
	  case BAptTex:
		offset = StorePolyTexFC( (PolyTexFC*)prim );
		break;
	  case TexL:
	  case ATexL:
	  case CTexL:
	  case CATexL:
		offset = StorePolyTexFCN( (PolyTexFCN*)prim );
		break;
	  case TexG:
	  case ATexG:
	  case CTexG:
	  case CATexG:
		offset = StorePolyTexVC( (PolyTexVC*)prim );
		break;
	  case TexGL:
	  case ATexGL:
	  case CTexGL:
	  case CATexGL:
		offset = StorePolyTexVCN( (PolyTexVCN*)prim );
		break;
	  default:
		printf("ERROR:  Unrecognized primtive type.\n");
		ShiError("Unrecognized primtive type.");
	}


	// Store our vertex position index array (common to all primitives)
	prim = (Prim*)PtrFromOffset( offset );
	prim->xyz = (int*)Store( prim->xyz, prim->nVerts*sizeof(*prim->xyz) );


	return offset;
}


int StorePrimPointFC( PrimPointFC *prim )
{
	// Copy ourselves into storage, then point to the new copy for pointer updates
	int offset = Store( prim, sizeof(*prim) );
	prim = (PrimPointFC*)PtrFromOffset( offset );
	return offset;
}


int StorePrimLineFC( PrimLineFC *prim )
{
	// Copy ourselves into storage, then point to the new copy for pointer updates
	int offset = Store( prim, sizeof(*prim) );
	prim = (PrimLineFC*)PtrFromOffset( offset );
	return offset;
}


int StorePrimLtStr( PrimLtStr *prim )
{
	// Copy ourselves into storage, then point to the new copy for pointer updates
	int offset = Store( prim, sizeof(*prim) );
	prim = (PrimLtStr*)PtrFromOffset( offset );
	return offset;
}


int StorePolyFC( PolyFC *prim )
{
	// Copy ourselves into storage, then point to the new copy for pointer updates
	int offset = Store( prim, sizeof(*prim) );
	prim = (PolyFC*)PtrFromOffset( offset );
	return offset;
}


int StorePolyVC( PolyVC *prim )
{
	// Copy ourselves into storage, then point to the new copy for pointer updates
	int offset = Store( prim, sizeof(*prim) );
	prim = (PolyVC*)PtrFromOffset( offset );

	// Store our vertex color index array
	prim->rgba = (int*)Store( prim->rgba, prim->nVerts*sizeof(*prim->rgba) );
	return offset;
}


int StorePolyFCN( PolyFCN *prim )
{
	// Copy ourselves into storage, then point to the new copy for pointer updates
	int offset = Store( prim, sizeof(*prim) );
	prim = (PolyFCN*)PtrFromOffset( offset );
	return offset;
}


int StorePolyVCN( PolyVCN *prim )
{
	// Copy ourselves into storage, then point to the new copy for pointer updates
	int offset = Store( prim, sizeof(*prim) );
	prim = (PolyVCN*)PtrFromOffset( offset );

	// Store our vertex color and normal index arrays
	prim->rgba = (int*)Store( prim->rgba, prim->nVerts*sizeof(*prim->rgba) );
	prim->I    = (int*)Store( prim->I   , prim->nVerts*sizeof(*prim->I) );
	return offset;
}


int StorePolyTexFC( PolyTexFC *prim )
{
	// Copy ourselves into storage, then point to the new copy for pointer updates
	int offset = Store( prim, sizeof(*prim) );
	prim = (PolyTexFC*)PtrFromOffset( offset );

	// Store our texture coordinates
	prim->uv   = (Ptexcoord*)Store( prim->uv, prim->nVerts*sizeof(*prim->uv) );
	return offset;
}


int StorePolyTexVC( PolyTexVC *prim )
{
	// Copy ourselves into storage, then point to the new copy for pointer updates
	int offset = Store( prim, sizeof(*prim) );
	prim = (PolyTexVC*)PtrFromOffset( offset );

	// Store our vertex color and our texture coordinates
	prim->rgba = (int*)Store( prim->rgba, prim->nVerts*sizeof(*prim->rgba) );
	prim->uv   = (Ptexcoord*)Store( prim->uv, prim->nVerts*sizeof(*prim->uv) );
	return offset;
}


int StorePolyTexFCN( PolyTexFCN *prim )
{
	// Copy ourselves into storage, then point to the new copy for pointer updates
	int offset = Store( prim, sizeof(*prim) );
	prim = (PolyTexFCN*)PtrFromOffset( offset );

	// Store our texture coordinates
	prim->uv   = (Ptexcoord*)Store( prim->uv, prim->nVerts*sizeof(*prim->uv) );
	return offset;
}

int StorePolyTexVCN( PolyTexVCN *prim )
{
	// Copy ourselves into storage, then point to the new copy for pointer updates
	int offset = Store( prim, sizeof(*prim) );
	prim = (PolyTexVCN*)PtrFromOffset( offset );

	// Store our vertex color and normal index arrays and our texture coordinates
	prim->rgba = (int*)Store( prim->rgba, prim->nVerts*sizeof(*prim->rgba) );
	prim->I    = (int*)Store( prim->I   , prim->nVerts*sizeof(*prim->I) );
	prim->uv   = (Ptexcoord*)Store( prim->uv, prim->nVerts*sizeof(*prim->uv) );
	return offset;
}
