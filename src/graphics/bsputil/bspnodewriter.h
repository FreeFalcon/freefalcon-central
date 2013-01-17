/***************************************************************************\
    BSPnodeWriter.h
    Scott Randolph
    March 19, 1998

    This handles writing the disk representation of the run time BSP trees.
\***************************************************************************/
#ifndef _BSPNODEWRITER_H_
#define _BSPNODEWRITER_H_
#include "BSPnodes.h"


// Publicly called
void	InitNodeStore( void );
int		StoreBNode( BNode *node );
void	WriteNodeStore( int file );

// Worker sub-functions
int  StoreBSubTree( BSubTree *node );
int  StoreBRoot( BRoot *node );
int  StoreBSpecialXform( BSpecialXform *node );
int  StoreBSlotNode( BSlotNode *node );
int  StoreBDofNode( BDofNode *node );
int  StoreBSwitchNode( BSwitchNode *node );
int  StoreBSplitterNode( BSplitterNode *node );
int  StoreBPrimitiveNode( BPrimitiveNode *node );
int  StoreBLitPrimitiveNode( BLitPrimitiveNode *node );
int  StoreBCulledPrimitiveNode( BCulledPrimitiveNode *node );
int  StoreBLightStringNode( BLightStringNode *node );

int   Store( void *src, int size );
void  StoreTag( BNodeType );
void* PtrFromOffset( int offset );
int   OffsetFromPtr( void *ptr );
int   GetMaxTagListLength(void);

#endif // _BSPNODEWRITER_H_