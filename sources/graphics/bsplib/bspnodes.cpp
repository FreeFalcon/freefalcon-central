/***************************************************************************\
    BSPnodes.cpp
    Scott Randolph
    January 30, 1998

    This provides the structure for the runtime BSP trees.
\***************************************************************************/
#include "stdafx.h"
#include "StateStack.h"
#include "ClipFlags.h"
#include "TexBank.h"
#include "ObjectInstance.h"
#include "Scripts.h"
#include "BSPnodes.h"
#include "falclib\include\mltrig.h"
#include "falclib\include\IsBad.h"

#pragma warning(disable : 4291)

#define XDOF_NEGATE (1<<0)
#define XDOF_MINMAX (1<<1)
#define XDOF_SUBRANGE (1<<2)
// compute the angle give sides of a triangle

#define XDOF_ISDOF   (1<<31)

bool	ShadowBSPRendering;										// COBRA - RED - this is to inform we r rendering a shadow,
float	ShadowAlphaLevel;										// that may be affected by TOD Light level

/***************************************************************\
	To improve performance, these classes use several global
	variables to store command data instead of passing it
	through the call stack.
	The global variables are maintained by the StackState
	module.
\***************************************************************/

// Determine the type of an encoded node and intialize and contruct
// it appropriatly.
BNode* BNode::RestorePointers( BYTE *baseAddress, int offset, BNodeType **tagListPtr )
{
	BNode		*node;
	BNodeType	tag;

	// Get this record's tag then advance the pointer to the next tag we'll need
	tag = **tagListPtr;
	*tagListPtr = (*tagListPtr)+1;
	

	// Apply the proper virtual table setup and constructor
	switch( tag ) {
	  case tagBSubTree:
		node = new (baseAddress+offset) BSubTree( baseAddress, tagListPtr );
		break;
	  case tagBRoot:
		node = new (baseAddress+offset) BRoot( baseAddress, tagListPtr );
		break;
	  case tagBSpecialXform:
		node = new (baseAddress+offset) BSpecialXform( baseAddress, tagListPtr );
		break;
	  case tagBSlotNode:
		node = new (baseAddress+offset) BSlotNode( baseAddress, tagListPtr );
		break;
	  case tagBDofNode:
		node = new (baseAddress+offset) BDofNode( baseAddress, tagListPtr );
		break;
	  case tagBSwitchNode:
		node = new (baseAddress+offset) BSwitchNode( baseAddress, tagListPtr );
		break;
	  case tagBSplitterNode:
		node = new (baseAddress+offset) BSplitterNode( baseAddress, tagListPtr );
		break;
	  case tagBPrimitiveNode:
		node = new (baseAddress+offset) BPrimitiveNode( baseAddress, tagListPtr );
		break;
	  case tagBLitPrimitiveNode:
		node = new (baseAddress+offset) BLitPrimitiveNode( baseAddress, tagListPtr );
		break;
	  case tagBCulledPrimitiveNode:
		node = new (baseAddress+offset) BCulledPrimitiveNode( baseAddress, tagListPtr );
		break;
	  case tagBLightStringNode:
		node = new (baseAddress+offset) BLightStringNode( baseAddress, tagListPtr );
		break;
	  case tagBTransNode:
		node = new (baseAddress+offset) BTransNode( baseAddress, tagListPtr );
		break;
	  case tagBScaleNode:
		node = new (baseAddress+offset) BScaleNode( baseAddress, tagListPtr );
		break;
	  case tagBXDofNode:
		node = new (baseAddress+offset) BXDofNode( baseAddress, tagListPtr );
		break;
	  case tagBXSwitchNode:
		node = new (baseAddress+offset) BXSwitchNode( baseAddress, tagListPtr );
		break;
	  case tagBRenderControlNode:
		node = new (baseAddress+offset) BRenderControlNode( baseAddress, tagListPtr );
		break;

	  default:
		ShiError("Decoding unrecognized BSP node type.");
	}

	return node;
}


// Convert from file offsets back to pointers
BNode::BNode( BYTE *baseAddress, BNodeType **tagListPtr )
{
	// Fixup our sibling, if any
	if ((int)sibling >= 0) {
		sibling = RestorePointers( baseAddress, (int)sibling, tagListPtr );
	} else {
		sibling = NULL;
	}
}


// Convert from file offsets back to pointers
BSubTree::BSubTree( BYTE *baseAddress, BNodeType **tagListPtr )
:BNode( baseAddress, tagListPtr )
{
	// Fixup our dependents
	subTree		= RestorePointers( baseAddress, (int)subTree, tagListPtr );

	// Fixup our data pointers
	pCoords		= (Ppoint *)(baseAddress + (int)pCoords);
	pNormals	= (Pnormal *)(baseAddress + (int)pNormals);
}


// Convert from file offsets back to pointers
BRoot::BRoot( BYTE *baseAddress, BNodeType **tagListPtr )
:BSubTree( baseAddress, tagListPtr )
{
	// Fixup our extra data pointers
	pTexIDs		= (int*)(baseAddress + (int)pTexIDs);
}


// Convert from file offsets back to pointers
BSpecialXform::BSpecialXform( BYTE *baseAddress, BNodeType **tagListPtr )
:BNode( baseAddress, tagListPtr )
{
	// Fixup our dependents
	subTree		= RestorePointers( baseAddress, (int)subTree, tagListPtr );

	// Fixup our data pointers
	pCoords		= (Tpoint *)(baseAddress + (int)pCoords);
}


// Convert from file offsets back to pointers
BSlotNode::BSlotNode( BYTE *baseAddress, BNodeType **tagListPtr )
:BNode( baseAddress, tagListPtr )
{
}


// Convert from file offsets back to pointers
BDofNode::BDofNode( BYTE *baseAddress, BNodeType **tagListPtr )
:BSubTree( baseAddress, tagListPtr )
{
}

// Convert from file offsets back to pointers
BXDofNode::BXDofNode( BYTE *baseAddress, BNodeType **tagListPtr )
:BSubTree( baseAddress, tagListPtr )
{
	flags|=XDOF_ISDOF;
}


// Convert from file offsets back to pointers
BTransNode::BTransNode( BYTE *baseAddress, BNodeType **tagListPtr )
:BSubTree( baseAddress, tagListPtr )
{
}

// Convert from file offsets back to pointers
BScaleNode::BScaleNode( BYTE *baseAddress, BNodeType **tagListPtr )
:BSubTree( baseAddress, tagListPtr )
{
}


// Convert from file offsets back to pointers
BSwitchNode::BSwitchNode( BYTE *baseAddress, BNodeType **tagListPtr )
:BNode( baseAddress, tagListPtr )
{
	// Fixup our table of children
	subTrees = (BSubTree**)(baseAddress + (int)subTrees);

	// Now fixup each child tree
	for (int i=0; i<numChildren; i++) {
		subTrees[i] = (BSubTree*)RestorePointers( baseAddress, (int)subTrees[i], tagListPtr );
	}
}

// Convert from file offsets back to pointers
BXSwitchNode::BXSwitchNode( BYTE *baseAddress, BNodeType **tagListPtr )
:BNode( baseAddress, tagListPtr )
{
	// Fixup our table of children
	subTrees = (BSubTree**)(baseAddress + (int)subTrees);

	// Now fixup each child tree
	for (int i=0; i<numChildren; i++) {
		subTrees[i] = (BSubTree*)RestorePointers( baseAddress, (int)subTrees[i], tagListPtr );
	}
}


// Convert from file offsets back to pointers
BSplitterNode::BSplitterNode( BYTE *baseAddress, BNodeType **tagListPtr )
:BNode( baseAddress, tagListPtr )
{
	// Fixup our dependents
	front	= RestorePointers( baseAddress, (int)front, tagListPtr );
	back	= RestorePointers( baseAddress, (int)back,  tagListPtr );
}


// Convert from file offsets back to pointers
BPrimitiveNode::BPrimitiveNode( BYTE *baseAddress, BNodeType **tagListPtr )
:BNode( baseAddress, tagListPtr )
{
	// Now fixup our polygon
	prim		= RestorePrimPointers( baseAddress, (int)prim );
}


// Convert from file offsets back to pointers
BLitPrimitiveNode::BLitPrimitiveNode( BYTE *baseAddress, BNodeType **tagListPtr )
:BNode( baseAddress, tagListPtr )
{
	// Now fixup our polygons
	poly		= (Poly*)RestorePrimPointers( baseAddress, (int)poly );
	backpoly	= (Poly*)RestorePrimPointers( baseAddress, (int)backpoly );
}


// Convert from file offsets back to pointers
BCulledPrimitiveNode::BCulledPrimitiveNode( BYTE *baseAddress, BNodeType **tagListPtr )
:BNode( baseAddress, tagListPtr )
{
	// Now fixup our polygon
	poly		= (Poly*)RestorePrimPointers( baseAddress, (int)poly );
}


// Convert from file offsets back to pointers
BLightStringNode::BLightStringNode( BYTE *baseAddress, BNodeType **tagListPtr )
:BPrimitiveNode( baseAddress, tagListPtr )
{
}


BRenderControlNode::BRenderControlNode( BYTE *baseAddress, BNodeType **tagListPtr )
:BNode( baseAddress, tagListPtr )
{
}

//JAM 06Jan04
void BSubTree::Draw(void)
{
	BNode *child;

	if(nNormals) TheStateStack.Light(pNormals,nNormals,pCoords); //- COBRA - RED - Call only if Normals available

	if(nDynamicCoords == 0)
		TheStateStack.Transform(pCoords,nCoords);
	else
	{
		TheStateStack.Transform(pCoords,nCoords-nDynamicCoords);
		TheStateStack.Transform(TheStateStack.CurrentInstance->DynamicCoords+DynamicCoordOffset,nDynamicCoords);
	}

	child = subTree;
	ShiAssert(FALSE == F4IsBadReadPtr( child, sizeof*child) );

	do
	{
		child->Draw();
		child = child->sibling;
	}
	while(child); // JB 010306 CTD
}
//JAM

void BRoot::Draw(void)
{
	// Compute the offset to the first texture in the texture set
	int texOffset = TheStateStack.CurrentInstance->TextureSet * 
		(nTexIDs/max(1,TheStateStack.CurrentInstance->ParentObject->nTextureSets)); // Cobra - TexSets = 0 CTD
	TheStateStack.SetTextureTable( pTexIDs + texOffset );
								   
	if (ScriptNumber > 0) {
		ShiAssert( ScriptNumber < ScriptArrayLength );
		if (ScriptNumber < ScriptArrayLength) {
			ScriptArray[ScriptNumber]();
		}
	}

	BSubTree::Draw();
	// LOOK HERE JAM!!!!
	//TheStateStack.context.setGlobalZBias(0);

}

void BSpecialXform::Draw(void)
{
	ShiAssert( subTree );

	TheStateStack.PushVerts();
	TheStateStack.TransformBillboardWithClip( pCoords, nCoords, type );
	subTree->Draw();
	TheStateStack.PopVerts();
}

void BSlotNode::Draw(void)
{
	ShiAssert( slotNumber < TheStateStack.CurrentInstance->ParentObject->nSlots );
	if (slotNumber >= TheStateStack.CurrentInstance->ParentObject->nSlots )
		return; // JPO fix
	ObjectInstance *subObject = TheStateStack.CurrentInstance->SlotChildren[slotNumber];

	if (subObject) {
		TheStateStack.DrawSubObject( subObject, &rotation, &origin );
	}
}

void BDofNode::Draw(void)
{
	Pmatrix	dofRot;
	Pmatrix	R;
	Ppoint	T;
	mlTrig trig;

	ShiAssert( dofNumber < TheStateStack.CurrentInstance->ParentObject->nDOFs );
	if (dofNumber >= TheStateStack.CurrentInstance->ParentObject->nDOFs )
		return;
	// Set up our free rotation
	mlSinCos (&trig, TheStateStack.CurrentInstance->DOFValues[dofNumber].rotation);
	dofRot.M11 = 1.0f;	dofRot.M12 = 0.0f;	dofRot.M13 = 0.0f;
	dofRot.M21 = 0.0f;	dofRot.M22 = trig.cos;	dofRot.M23 = -trig.sin;
	dofRot.M31 = 0.0f;	dofRot.M32 = trig.sin;	dofRot.M33 = trig.cos;


	// Now compose this with the rotation into our parents coordinate system
	MatrixMult( &rotation, &dofRot, &R );

	// Now do our free translation
	// SCR 10/28/98:  THIS IS WRONG FOR TRANSLATION DOFs.  "DOFValues" is supposed to 
	// translate along the local x axis, but this will translate along the parent's x axis.
	// To fix this would require a bit more math (and/or thought).  Since it
	// only happens once in Falcon, I'll leave it broken and put a workaround into
	// the KC10 object so that the parent's and child's x axis are forced into alignment
	// by inserting an extra dummy DOF bead.
	T.x = translation.x + TheStateStack.CurrentInstance->DOFValues[dofNumber].translation;
	T.y = translation.y;
	T.z = translation.z;

	// Draw our subtree
	TheStateStack.PushAll();
	TheStateStack.CompoundTransform( &R, &T );
	BSubTree::Draw();
	TheStateStack.PopAll();
}




float Process_DOFRot(int dofNumber, int flags, float min, float max, float multiplier, float unused);

float Process_DOFRot(int dofNumber, int flags, float min, float max, float multiplier, float unused)
{
	float dofrot=TheStateStack.CurrentInstance->DOFValues[dofNumber].rotation;

	if(flags & XDOF_NEGATE)
	{
	  dofrot=-dofrot;
	}

	if(flags & XDOF_MINMAX)
	{
	  if(dofrot<min)
		dofrot=min;
	  if(dofrot>max)
		dofrot=max;
	}

	if(flags & XDOF_SUBRANGE && min!=max)
	{
	  // rescales dofrot so it is 0.0 at Min and 1.0 at Max
	  // it could exceed those bounds, unless MINMAX is set.
	  dofrot-=min;
	  dofrot/=max-min;
	  // for dofs, convert it to 1 radian
	  if(flags & XDOF_ISDOF)
		  dofrot*=(float)(3.14159/180.0);
	  // then it get's rescaled below.
	}

	return(dofrot*=multiplier);
}


void BXDofNode::Draw(void)
{
	Pmatrix	dofRot;
	Pmatrix	R;
	Ppoint	T;
	mlTrig trig;

	ShiAssert( dofNumber < TheStateStack.CurrentInstance->ParentObject->nDOFs );
	if (dofNumber >= TheStateStack.CurrentInstance->ParentObject->nDOFs )
		return;
	// Set up our free rotation

	float dofrot=Process_DOFRot(dofNumber,flags,min,max,multiplier,future);

	/*
	float dofrot=TheStateStack.CurrentInstance->DOFValues[dofNumber].rotation;

	if(flags & XDOF_NEGATE)
	{
	  dofrot=-dofrot;
	}

	if(flags & XDOF_MINMAX)
	{
	  if(dofrot<min)
		dofrot=min;
	  if(dofrot>max)
		dofrot=max;
	}

	if(flags & XDOF_SUBRANGE && min!=max)
	{
	  // rescales dofrot so it is 0.0 at Min and 1.0 at Max
	  // it could exceed those bounds, unless MINMAX is set.
	  dofrot-=min;
	  dofrot/=max-min;
	  // then it get's rescaled below.
	}

	dofrot*=multiplier;
*/


	mlSinCos (&trig, dofrot);
	dofRot.M11 = 1.0f;	dofRot.M12 = 0.0f;	dofRot.M13 = 0.0f;
	dofRot.M21 = 0.0f;	dofRot.M22 = trig.cos;	dofRot.M23 = -trig.sin;
	dofRot.M31 = 0.0f;	dofRot.M32 = trig.sin;	dofRot.M33 = trig.cos;


	// Now compose this with the rotation into our parents coordinate system
	MatrixMult( &rotation, &dofRot, &R );

	// Now do our free translation
	// SCR 10/28/98:  THIS IS WRONG FOR TRANSLATION DOFs.  "DOFValues" is supposed to 
	// translate along the local x axis, but this will translate along the parent's x axis.
	// To fix this would require a bit more math (and/or thought).  Since it
	// only happens once in Falcon, I'll leave it broken and put a workaround into
	// the KC10 object so that the parent's and child's x axis are forced into alignment
	// by inserting an extra dummy DOF bead.
	T.x = translation.x + TheStateStack.CurrentInstance->DOFValues[dofNumber].translation;
	T.y = translation.y;
	T.z = translation.z;

	// Draw our subtree
	TheStateStack.PushAll();
	TheStateStack.CompoundTransform( &R, &T );
	BSubTree::Draw();
	TheStateStack.PopAll();
}
/*
#define BNF_REVERSED_EFFECT     (1<<0) // the effect is reverse (node specific)
#define BNF_NEGATE				(1<<1)
#define BNF_POSITIVE_ONLY		(1<<2)
#define BNF_NEGATIVE_ONLY		(1<<3)
#define BNF_CLAMP				(1<<4)

// MLR - my little helper function
float Process_a(float dofRotScalar, int, int Flags);

float Process_a(float dofRotScalar, int DofID, int flags)
{
	float a=0;

	if(dofRotScalar)
	{
		a = TheStateStack.CurrentInstance->DOFValues[DofID].rotation / dofRotScalar;

		if(flags & BNF_NEGATE)
		{
			a=-a;
		}

		if(flags  & BNF_POSITIVE_ONLY)
		{
		  if(a<0) a=0;
		}

		if(flags  & BNF_NEGATIVE_ONLY)
		{
		  if(a>0) a=0;
		}

		if(flags  & BNF_CLAMP)
		{
		  if(a>1) a=1;
		  if(a<-1) a=-1;
		}
	}
	return (a);
}
*/


// MLR 2003-10-10 New Translator Node
void BTransNode::Draw(void)
{
	Ppoint	T;

    Pmatrix	dofRot; // just a dummy to pass to CompoundTransform()
	dofRot.M11 = 1.0f;	dofRot.M12 = 0.0f;	dofRot.M13 = 0.0f;
	dofRot.M21 = 0.0f;	dofRot.M22 = 1.0f;	dofRot.M23 = 0.0;
	dofRot.M31 = 0.0f;	dofRot.M32 = 0.0;	dofRot.M33 = 1.0;



	float a=0;
	//ShiAssert( dofNumber < TheStateStack.CurrentInstance->ParentObject->nDOFs );
	if (dofNumber >= TheStateStack.CurrentInstance->ParentObject->nDOFs )
		return;

	// the multiplier gets converted from radians to degrees
	a=Process_DOFRot(dofNumber,flags,min,max,multiplier,future);


	T.x = translation.x * a;
	T.y = translation.y * a;
	T.z = translation.z * a;

	// Draw our subtree
	TheStateStack.PushAll();
	TheStateStack.CompoundTransform( &dofRot, &T );
	BSubTree::Draw();
	TheStateStack.PopAll();
}



// MLR 2003-10-10
//   only scales along axii
void BScaleNode::Draw(void)
{
	Ppoint	T;

	float a=0,dx=1,dy=1,dz=1;
	//ShiAssert( dofNumber < TheStateStack.CurrentInstance->ParentObject->nDOFs );
	if (dofNumber >= TheStateStack.CurrentInstance->ParentObject->nDOFs )
		return;

	//a=Process_a(dofRotScalar,dofNumber,flags);
	a=Process_DOFRot(dofNumber,flags,min,max,multiplier,future);

	dx=1.0F - (1.0f-scale.x) * a;
	dy=1.0F - (1.0f-scale.y) * a;
	dz=1.0F - (1.0f-scale.z) * a;

    Pmatrix	dofRot; // just a dummy to pass to CompoundTransform()
	dofRot.M11 = dx;	dofRot.M12 = 0.0f;	dofRot.M13 = 0.0f;
	dofRot.M21 = 0.0f;	dofRot.M22 = dy;	dofRot.M23 = 0.0;
	dofRot.M31 = 0.0f;	dofRot.M32 = 0.0;	dofRot.M33 = dz;

	T.x = translation.x;
	T.y = translation.y;
	T.z = translation.z;

	// Draw our subtree
	TheStateStack.PushAll();
	TheStateStack.CompoundTransform( &dofRot, &T );
	BSubTree::Draw();
	TheStateStack.PopAll();
}



void BSwitchNode::Draw(void)
{
	UInt32		mask;
	int			i = 0;

	ShiAssert( switchNumber < TheStateStack.CurrentInstance->ParentObject->nSwitches );
	if (switchNumber >= TheStateStack.CurrentInstance->ParentObject->nSwitches)
		return;
	mask = TheStateStack.CurrentInstance->SwitchValues[switchNumber];

#if 0	// This will generally be faster due to early termination
	// Go until all ON switch children have been drawn.
	while (mask) {
#else	// This will work even if the mask is set for non-existent children
	// Go until all children have been considered for drawing.
	while (i < numChildren) {
#endif
		if( subTrees[i] ) // MLR 2003-10-12 changed from ShiAssert.
		{

			// Only draw this subtree if the corresponding switch bit is set
			if (mask & 1) {
				TheStateStack.PushVerts();
				subTrees[i]->Draw();
				TheStateStack.PopVerts();
			}
			mask >>= 1;
			i++;
		}
	}
}


#define XSWT_REVERSED_EFFECT (1<<0)

// MLR 2003-10-12 New Node, this switch is on opposite of the normal switch.
void BXSwitchNode::Draw(void)
{
	UInt32		mask;
	int			i = 0;

	ShiAssert( switchNumber < TheStateStack.CurrentInstance->ParentObject->nSwitches );
	if (switchNumber >= TheStateStack.CurrentInstance->ParentObject->nSwitches)
		return;
	mask = TheStateStack.CurrentInstance->SwitchValues[switchNumber];
	
	if(flags & XSWT_REVERSED_EFFECT)
		mask=~mask;
	
	
	while (i < numChildren) 
	{
		if(subTrees[i] )
		{
			// Only draw this subtree if the corresponding switch bit is set
			if (mask & 1) {
				TheStateStack.PushVerts();
				subTrees[i]->Draw();
				TheStateStack.PopVerts();
			}
			mask >>= 1;
			i++;
		}
	}
}


void BSplitterNode::Draw(void)
{
	BNode	*child;

	ShiAssert( front );
	ShiAssert( back );

	if (A*TheStateStack.ObjSpaceEye.x + 
		B*TheStateStack.ObjSpaceEye.y + 
		C*TheStateStack.ObjSpaceEye.z + D > 0.0f) {

		child = front;
		do
		{
			child->Draw();
			child = child->sibling;
		} while (child);

		child = back;
		do
		{
			child->Draw();
			child = child->sibling;
		} while (child);
	}
	else
	{
		child = back;
		do
		{
			child->Draw();
			child = child->sibling;
		} while (child);

		child = front;
		do
		{
			child->Draw();
			child = child->sibling;
		} while (child);
	}
}

void BPrimitiveNode::Draw(void)
{
	// Call the appropriate draw function for this primitive
	DrawPrimJumpTable[prim->type]( prim );
}

void BLitPrimitiveNode::Draw(void)
{
	// Choose the front facing polygon so that lighting is correct
	if ((poly->A*TheStateStack.ObjSpaceEye.x + poly->B*TheStateStack.ObjSpaceEye.y + poly->C*TheStateStack.ObjSpaceEye.z + poly->D) >= 0) {
		DrawPrimJumpTable[poly->type]( poly );
	} else {
		DrawPrimJumpTable[poly->type]( backpoly );
	}
}


void BCulledPrimitiveNode::Draw(void)
{
	// Only draw front facing polygons
	if ((poly->A*TheStateStack.ObjSpaceEye.x + poly->B*TheStateStack.ObjSpaceEye.y + poly->C*TheStateStack.ObjSpaceEye.z + poly->D) >= 0){
		// Call the appropriate draw function for this polygon
		DrawPrimJumpTable[poly->type]( poly );

	}
}

void BLightStringNode::Draw(void)
{
	// Clobber the primitive color with the appropriate front or back color
	if ((A*TheStateStack.ObjSpaceEye.x + B*TheStateStack.ObjSpaceEye.y + C*TheStateStack.ObjSpaceEye.z + D) >= 0) {
		((PrimPointFC*)prim)->rgba = rgbaFront;
	} else {
		((PrimPointFC*)prim)->rgba = rgbaBack;
	}

	// Call the appropriate draw function for this polygon
	DrawPrimJumpTable[prim->type]( prim );
}


void BRoot::LoadTextures(void)
{
	for (int i=0; i<nTexIDs; i++) {
		// Skip unsed texture indices
		if (pTexIDs[i] >= 0) {
			TheTextureBank.Reference( pTexIDs[i] );
		}
	}
}


void BRoot::UnloadTextures(void)
{
	for (int i=0; i<nTexIDs; i++) {
		if (pTexIDs[i] >= 0) {
			TheTextureBank.Release( pTexIDs[i] );
		}
	}
}

void BRenderControlNode::Draw(void)
{
	switch(Control)
	{
	case rcZBias:
		TheStateStack.context->setGlobalZBias(FArg[0]);
		break;
	}
}
