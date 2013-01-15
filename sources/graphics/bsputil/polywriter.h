/***************************************************************************\
    PolyWriter.h
    Scott Randolph
    March 23, 1998

    This handles writing the disk representation of the run time primitives.
\***************************************************************************/
#ifndef _POLYWRITER_H_
#define _POLYWRITER_H_
#include "PolyLib.h"


// Publicly called
int StorePrimitive( Prim *prim );

// Worker sub-functions
int StorePrimPointFC( PrimPointFC *prim );
int StorePrimLineFC( PrimLineFC *prim );
int StorePrimLtStr( PrimLtStr *prim );
int StorePolyFC( PolyFC *prim );
int StorePolyVC( PolyVC *prim );
int StorePolyFCN( PolyFCN *prim );
int StorePolyVCN( PolyVCN *prim );
int StorePolyTexFC( PolyTexFC *prim );
int StorePolyTexVC( PolyTexVC *prim );
int StorePolyTexFCN( PolyTexFCN *prim );
int StorePolyTexVCN( PolyTexVCN *prim );

#endif // _POLYWRITER_H_
