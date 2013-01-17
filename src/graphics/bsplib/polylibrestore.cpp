/***************************************************************************\
    PolyLibRestore.cpp
    Scott Randolph
    March 24, 1998

    Provides pointer restoration services for 3D polygons of various types
	after they've been read back from disk.
\***************************************************************************/
#include "stdafx.h"
#include "PolyLib.h"


// Called by the parent BNode to decode each polygon
Prim* RestorePrimPointers( BYTE *baseAddress, int offset )
{
	Prim *prim = (Prim*)(baseAddress + offset );

	// Do our type specific handling
	switch( prim->type ) {
	  case PointF:
		RestorePrimPointFC( (PrimPointFC*)prim, baseAddress );
		break;
	  case LineF:
		RestorePrimLineFC( (PrimLineFC*)prim, baseAddress );
		break;
	  case F:
	  case AF:
		RestorePolyFC( (PolyFC*)prim, baseAddress );
		break;
	  case FL:
	  case AFL:
		RestorePolyFCN( (PolyFCN*)prim, baseAddress );
		break;
	  case G:
	  case AG:
		RestorePolyVC( (PolyVC*)prim, baseAddress );
		break;
	  case GL:
	  case AGL:
		RestorePolyVCN( (PolyVCN*)prim, baseAddress );
		break;
	  case Tex:
	  case ATex:
	  case CTex:
	  case CATex:
	  case BAptTex:
		RestorePolyTexFC( (PolyTexFC*)prim, baseAddress );
		break;
	  case TexL:
	  case ATexL:
	  case CTexL:
	  case CATexL:
		RestorePolyTexFCN( (PolyTexFCN*)prim, baseAddress );
		break;
	  case TexG:
	  case ATexG:
	  case CTexG:
	  case CATexG:
		RestorePolyTexVC( (PolyTexVC*)prim, baseAddress );
		break;
	  case TexGL:
	  case ATexGL:
	  case CTexGL:
	  case CATexGL:
		RestorePolyTexVCN( (PolyTexVCN*)prim, baseAddress );
		break;
	  default:
		ShiError("Unrecognized primtive type in decode.");
	}


	// Restore our vertex position index array (common to all primitives)
	prim->xyz = (int*)( baseAddress + (int)prim->xyz );


	return prim;
}


// Worker functions appropriate to specific polygon data structures
void RestorePrimPointFC( PrimPointFC *, BYTE * )
{
}


void RestorePrimLineFC( PrimLineFC *, BYTE * )
{
}


void RestorePolyFC( PolyFC *, BYTE * )
{
}


void RestorePolyVC( PolyVC *prim, BYTE *baseAddress  )
{
	// Restore our vertex color index array
	prim->rgba = (int*)( baseAddress + (int)prim->rgba );
}


void RestorePolyFCN( PolyFCN *, BYTE *  )
{
}


void RestorePolyVCN( PolyVCN *prim, BYTE *baseAddress  )
{
	// Restore our vertex color and normal index array
	prim->rgba = (int*)( baseAddress + (int)prim->rgba );
	prim->I    = (int*)( baseAddress + (int)prim->I );
}


void RestorePolyTexFC( PolyTexFC *prim, BYTE *baseAddress  )
{
	// Restore our texture coordinates
	prim->uv   = (Ptexcoord*)( baseAddress + (int)prim->uv );
}


void RestorePolyTexVC( PolyTexVC *prim, BYTE *baseAddress  )
{
	// Restore our vertex color and our texture coordinates
	prim->rgba =       (int*)( baseAddress + (int)prim->rgba );
	prim->uv   = (Ptexcoord*)( baseAddress + (int)prim->uv );
}


void RestorePolyTexFCN( PolyTexFCN *prim, BYTE *baseAddress  )
{
	// Restore our texture coordinates
	prim->uv   = (Ptexcoord*)( baseAddress + (int)prim->uv );
}


void RestorePolyTexVCN( PolyTexVCN *prim, BYTE *baseAddress  )
{
	// Restore our vertex color and normal index arrays and our texture coordinates
	prim->rgba =       (int*)( baseAddress + (int)prim->rgba );
	prim->I    =       (int*)( baseAddress + (int)prim->I );
	prim->uv   = (Ptexcoord*)( baseAddress + (int)prim->uv );
}

