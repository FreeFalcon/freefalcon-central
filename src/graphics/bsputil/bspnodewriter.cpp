/***************************************************************************\
    BSPnodeWriter.cpp
    Scott Randolph
    March 19, 1998

    This handles writing the disk representation of the run time BSP trees.
\***************************************************************************/
#include <io.h>
#include "PolyWriter.h"
#include "BSPnodeWriter.h"


// Global storage used to build the packed node store prior to writing it
// to disk.
static const int	MAX_NODE_STORE_SIZE		= 256 * 1024;	// Bytes (arbitrary)
static const int	MAX_NODE_STORE_COUNT	= 10000;		// Nodes (arbitrary)

static BYTE			NodeStoreBuffer[MAX_NODE_STORE_SIZE];
static void			*NodeStore;
static BNodeType	TagListBuffer[MAX_NODE_STORE_COUNT];
static BNodeType	*TagList;
static int			MaxTagListLength = -1;


void	InitNodeStore( void )
{
	NodeStore	= NodeStoreBuffer;
	TagList		= TagListBuffer;
}


void	WriteNodeStore( int file )
{
	int result;
	int	size;

//	printf( "TODO:  Compress BSP tree!!!\n" );

	// Write the tag list length
	size = (TagList - TagListBuffer);
	MaxTagListLength = max( MaxTagListLength, size );
	result = write( file, &size, sizeof(size) );

	// Write the tag list
	size *= sizeof(*TagList);
	result = write( file, TagListBuffer, size );

	// Write the tree nodes
	size = OffsetFromPtr( NodeStore );
	result = write( file, NodeStoreBuffer, size );

	// Make sure the write was successful (assume if the last one worked, they all did)
	if (result < 0 ) {
		perror( "Writing BSP tree" );
	}
}


// Publicly called exploder function.
int StoreBNode( BNode *node )
{
	int	offset;
	int	siblingOffset;


	// Record the type of this record for later decoding
	StoreTag( node->Type() );


	// Handle our sibling (if any)
	if (node->sibling) {
		siblingOffset = StoreBNode( node->sibling );
	} else {
		siblingOffset = -1;
	}


	// Do our type specific handling
	switch( node->Type() ) {
	  case tagBSubTree:
		offset = StoreBSubTree( (BSubTree*)node );
		break;
	  case tagBRoot:
		offset = StoreBRoot( (BRoot*)node );
		break;
	  case tagBSpecialXform:
		offset = StoreBSpecialXform( (BSpecialXform*)node );
		break;
	  case tagBSlotNode:
		offset = StoreBSlotNode( (BSlotNode*)node );
		break;
	  case tagBDofNode:
		offset = StoreBDofNode( (BDofNode*)node );
		break;
	  case tagBSwitchNode:
		offset = StoreBSwitchNode( (BSwitchNode*)node );
		break;
	  case tagBSplitterNode:
		offset = StoreBSplitterNode( (BSplitterNode*)node );
		break;
	  case tagBPrimitiveNode:
		offset = StoreBPrimitiveNode( (BPrimitiveNode*)node );
		break;
	  case tagBLitPrimitiveNode:
		offset = StoreBLitPrimitiveNode( (BLitPrimitiveNode*)node );
		break;
	  case tagBCulledPrimitiveNode:
		offset = StoreBCulledPrimitiveNode( (BCulledPrimitiveNode*)node );
		break;
	  case tagBLightStringNode:
		offset = StoreBLightStringNode( (BLightStringNode*)node );
		break;
	  default:
		printf("ERROR:  Unrecognized node type.\n");
		ShiError("Unrecognized node type.");
	}

	// Store our sibling's offset
	node = (BNode*)PtrFromOffset( offset );
	node->sibling = (BNode*)siblingOffset;

	// Return a pointer into the storage block where we put the node
	return offset;
}


// Worker sub-functions
int StoreBSubTree( BSubTree *node )
{
	// Copy ourselves into storage, then point to the new copy for pointer updates
	int offset = Store( node, sizeof(*node) );
	node = (BSubTree*)PtrFromOffset( offset );

	// Store our dependents
	node->subTree = (BNode*)StoreBNode( node->subTree );

	// Store our tables
	node->pCoords  = (Ppoint*)Store( node->pCoords,  node->nCoords*sizeof(*node->pCoords) );
	node->pNormals = (Pnormal*)Store( node->pNormals, node->nNormals*sizeof(*node->pCoords) );

	return offset;
}


int StoreBRoot( BRoot *node )
{
	// Copy ourselves into storage, then point to the new copy for pointer updates
	int offset = Store( node, sizeof(*node) );
	node = (BRoot*)PtrFromOffset( offset );

	// Store our dependents
	node->subTree = (BNode*)StoreBNode( node->subTree );

	// Store our tables
	node->pCoords  = (Ppoint*)Store( node->pCoords,  node->nCoords*sizeof(*node->pCoords) );
	node->pNormals = (Pnormal*)Store( node->pNormals, node->nNormals*sizeof(*node->pCoords) );
	node->pTexIDs  = (int*)Store( node->pTexIDs,  node->nTexIDs*sizeof(*node->pCoords) );

	return offset;
}


int StoreBSpecialXform( BSpecialXform *node )
{
	// Copy ourselves into storage, then point to the new copy for pointer updates
	int offset = Store( node, sizeof(*node) );
	node = (BSpecialXform*)PtrFromOffset( offset );

	// Store our dependents
	node->subTree = (BNode*)StoreBNode( node->subTree );

	// Store our tables
	node->pCoords  = (Ppoint*)Store( node->pCoords,  node->nCoords*sizeof(*node->pCoords) );

	return offset;
}


int StoreBSlotNode( BSlotNode *node )
{
	// Copy ourselves into storage, then point to the new copy for pointer updates
	int offset = Store( node, sizeof(*node) );
	node = (BSlotNode*)PtrFromOffset( offset );

	return offset;
}


int StoreBDofNode( BDofNode *node )
{
	// Copy ourselves into storage, then point to the new copy for pointer updates
	int offset = Store( node, sizeof(*node) );
	node = (BDofNode*)PtrFromOffset( offset );

	// Store our dependents
	node->subTree = (BNode*)StoreBNode( node->subTree );

	// Store our tables
	node->pCoords  = (Ppoint*)Store( node->pCoords,  node->nCoords*sizeof(*node->pCoords) );
	node->pNormals = (Pnormal*)Store( node->pNormals, node->nNormals*sizeof(*node->pNormals) );

	return offset;
}


int StoreBSwitchNode( BSwitchNode *node )
{
	int			subTreesOffset;
	BSubTree	**subTrees;

	// Copy ourselves into storage, then point to the new copy for pointer updates
	int offset = Store( node, sizeof(*node) );
	node = (BSwitchNode*)PtrFromOffset( offset );

	// Store our table of children
	subTreesOffset = Store( node->subTrees, node->numChildren*sizeof(*node->subTrees) );
	node->subTrees = (BSubTree**)subTreesOffset;
	subTrees = (BSubTree**)PtrFromOffset( subTreesOffset );

	// Now store each child tree
	for (int i=0; i<node->numChildren; i++) {
		subTrees[i] = (BSubTree*)StoreBNode( subTrees[i] );
	}

	return offset;
}


int StoreBSplitterNode( BSplitterNode *node )
{
	// Copy ourselves into storage, then point to the new copy for pointer updates
	int offset = Store( node, sizeof(*node) );
	node = (BSplitterNode*)PtrFromOffset( offset );

	// Now store each of our children
	node->front	= (BNode*)StoreBNode( node->front );
	node->back	= (BNode*)StoreBNode( node->back );

	return offset;
}


int StoreBPrimitiveNode( BPrimitiveNode *node )
{
	// Copy ourselves into storage, then point to the new copy for pointer updates
	int offset = Store( node, sizeof(*node) );
	node = (BPrimitiveNode*)PtrFromOffset( offset );

	// Now store our polygon
	node->prim		= (Prim*)StorePrimitive( node->prim );

	return offset;
}


int StoreBLitPrimitiveNode( BLitPrimitiveNode *node )
{
	// Copy ourselves into storage, then point to the new copy for pointer updates
	int offset = Store( node, sizeof(*node) );
	node = (BLitPrimitiveNode*)PtrFromOffset( offset );

	// Now store our polygon
	node->poly		= (Poly*)StorePrimitive( node->poly );
	node->backpoly	= (Poly*)StorePrimitive( node->backpoly );

	return offset;
}


int StoreBCulledPrimitiveNode( BCulledPrimitiveNode *node )
{
	// Copy ourselves into storage, then point to the new copy for pointer updates
	int offset = Store( node, sizeof(*node) );
	node = (BCulledPrimitiveNode*)PtrFromOffset( offset );

	// Now store our polygon
	node->poly		= (Poly*)StorePrimitive( node->poly );

	return offset;
}


int StoreBLightStringNode( BLightStringNode *node )
{
	// Copy ourselves into storage, then point to the new copy for pointer updates
	int offset = Store( node, sizeof(*node) );
	node = (BLightStringNode*)PtrFromOffset( offset );

	// Now store our polygon
	node->prim		= (Prim*)StorePrimitive( node->prim );

	return offset;
}


int Store( void *src, int size )
{
	void *p = NodeStore;

	if ( ((BYTE*)NodeStore)+size > NodeStoreBuffer+MAX_NODE_STORE_SIZE ) {
		printf("Exceeded tree construction buffer.  Update code.\n");
		ShiError("Exceeded tree construction buffer.  Update code.");
	}

	memcpy( NodeStore, src, size );
	NodeStore = ((BYTE*)NodeStore) + size;

	// Return the offset into the buffer at which this data was stored
	return OffsetFromPtr( p );
}


void StoreTag( BNodeType tag)
{
	if( TagList - TagListBuffer < MAX_NODE_STORE_COUNT ) {
		*TagList++ = tag;
	} else {
		printf("Exceeded tree construction tag buffer.  Update code.\n");
		ShiError("Exceeded tree construction tag buffer.  Update code.");
	}
}


void* PtrFromOffset( int offset )
{
	return NodeStoreBuffer + offset;
}


int OffsetFromPtr( void *ptr )
{
	return (BYTE*)ptr - NodeStoreBuffer;
}


int GetMaxTagListLength(void)
{
	return MaxTagListLength;
}
