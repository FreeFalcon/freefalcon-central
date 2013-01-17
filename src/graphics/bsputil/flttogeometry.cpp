/***************************************************************************\
    FLTtoGeometry.cpp
    Scott Randolph
    January 29, 1998

    This is the tool which reads Multigen FLT files into our internal
	tree structure.
\***************************************************************************/
#include <stdlib.h>
#include <math.h>
#include <MgAPIall.h>
#include "shi\ShiError.h"
#include "StateStack.h"
#include "ObjectParent.h"
#include "PosBuildList.h"
#include "ColorBuildList.h"
#include "TexBuildList.h"
#include "AlphaPatch.h"
#include "DynamicPatch.h"
#include "ScriptNames.h"
#include "FLTreader.h"
#include "FLTerror.h"


// Build time stamp for versioning info
char FLTtoGeometryTimeStamp[] = __TIMESTAMP__;


// This is needed during tree build, so we make it available to all routines in this file
static mgrec	*db = NULL;		// top record of database file we're processing


// These are used to accumulate the data for the object being constructed.
static BuildTimePosList	*PosPool;

static Pnormal	NormalBuffer[MAX_VERTS_PER_OBJECT_TREE];
static Pnormal	*NormalArray = NormalBuffer;
static int		NormalCnt;

static int		TexIDArray[MAX_TEXTURES_PER_OBJECT];
static int		TexIDCnt;

static Ppoint	SlotPositionArray[MAX_SLOT_AND_DYNAMIC_PER_OBJECT];
static Ppoint	DynamicPositionArray[MAX_SLOT_AND_DYNAMIC_PER_OBJECT];

static Pmatrix	Rotation;
static Ppoint	Translation;
static BOOL		LocalCoords;
static BOOL		PrelightColors;

static BuildTimePosList	*savePosPool[MAX_STATE_STACK_DEPTH];
static Pnormal	*saveNormalArray[MAX_STATE_STACK_DEPTH];
static int		saveNormalCnt[MAX_STATE_STACK_DEPTH];
static Pmatrix	saveRotation[MAX_STATE_STACK_DEPTH];
static Ppoint	saveTranslation[MAX_STATE_STACK_DEPTH];
static BOOL		saveLocalCoords[MAX_STATE_STACK_DEPTH];
static SzInfo_t	saveSizeInfo[MAX_STATE_STACK_DEPTH];

static int		stackDepth = 0;

static int	nSwitches;
static int	nDOFs;	
static int	nSlots;
static int	nDynamicCoords;
static int	nTextureSets;
static int	ObjectFlags;
static int	ScriptNumber;


// Default record processing function (switches out to appropriate type specific calls as required)
BNode*	ProcessRecord(mgrec *rec);
BNode*	ProcessAllChildren(mgrec *rec);


void StartSubObject(Pmatrix *R, Ppoint *T)
{
	ShiAssert( stackDepth < MAX_STATE_STACK_DEPTH );

	savePosPool[stackDepth]			= PosPool;

	saveNormalArray[stackDepth]		= NormalArray;
	saveNormalCnt[stackDepth]		= NormalCnt;

	saveRotation[stackDepth]		= Rotation;
	saveTranslation[stackDepth]		= Translation;
	saveLocalCoords[stackDepth]		= LocalCoords;

	stackDepth++;

	PosPool				= new BuildTimePosList;

	NormalArray		   += NormalCnt;
	NormalCnt			= 0;

	// Now put the new coordinate system into effect (if one was provided)
	if (R) {
		Rotation	= *R;
		Translation	= *T;
		LocalCoords	= TRUE;
	}
}


void EndSubObject(void)
{
	ShiAssert( stackDepth > 0 );
	stackDepth--;

	delete PosPool;

	PosPool			= savePosPool[stackDepth];

	NormalArray		= saveNormalArray[stackDepth];
	NormalCnt		= saveNormalCnt[stackDepth];

	Rotation		= saveRotation[stackDepth];
	Translation		= saveTranslation[stackDepth];
	LocalCoords		= saveLocalCoords[stackDepth];
}


int AddNormalToTable( float i, float j, float k )
{
	int		index;
	Pnormal	*normal;

	ShiAssert( NormalCnt < MAX_VERTS_PER_OBJECT_TREE );

	// See if we've already got a matching normal to share
	for (index=0; index<NormalCnt; index++) {
		if ((i == NormalArray[index].i) &&
			(j == NormalArray[index].j) &&
			(k == NormalArray[index].k)){
			return index;
		}
	}

	normal = &NormalArray[index];

	normal->i = i;
	normal->j = j;
	normal->k = k;

	NormalCnt++;
	return index;
}


void ProcessHeaderCommentFlags(void)
{
	char	*comment;

	ShiAssert( db );

	// Get the header comment (if any)
	comment = mgGetComment( db );
	if (!comment) {
		return;
	}

	char	*current = comment;
	char	*end;
	char	*next;
	char	tag[256];
	char	arg[256];
	DWORD	tagVal;
	int		argVal;


	while (*current) {
		// Break out the next line and null terminate it
		next = current;
		while (*next != '\0' && *next != 0xD && *next != 0xA) {
			next++;
		}
		end = next;
		while (*next == 0xD || *next == 0xA) {
			next++;
		}
		*end = '\0';

		// Get the first word on this line (if there is one)
		if (sscanf( current, "%s %s", tag, arg ) < 1) {
			current = next;
			continue;
		}

		// Convert the tag to an uppercase DWORD for decision making
		tagVal = (toupper(tag[0]) << 24) | (toupper(tag[1]) << 16) | (toupper(tag[2]) << 8) | (toupper(tag[3]));

		// Now handle the various special tags
		switch (tagVal) {
		  case 'DARK':
//			printf( "  Flag: Prelit colors\n" );
			PrelightColors = TRUE;
			break;
		  case 'PERS':
//			printf( "  Flag: Force perspective correction\n" );
			ObjectFlags = ObjectLOD::PERSP_CORR;
			break;
		  case 'VERT':
//			printf( "  Flag: Vertex alpha patch %s\n", arg );
			TheAlphaPatchList.Load(arg);
			break;
		  case 'DYNA':
//			printf( "  Flag: Dynamic vertex list %s\n", arg );
			TheDynamicPatchList.Load(arg);
			break;
		  case 'TEXT':
			argVal = atoi( arg );
//			printf( "  Flag: Texture set count %d\n", argVal );
			nTextureSets = argVal;
			break;
		  case 'ANIM':
			if (ScriptNumber < 0) {
				argVal = GetScriptNumberFromName( arg );
//				printf( "  Flag: Animation script %s = index %0d\n", arg, argVal );
				ScriptNumber = argVal;
			} else {
				printf( "TOO MANY SCRIPTS!  Only one script allowed per object. %s skipped.\n", arg );
			}
			break;
		  default:
			printf( "I don't understand header flag %s -- IGNORED\n", tag );
		}

		// Advance to the next line
		current = next;
	}

	// Free the comment string
	mgFree( comment );
}


void ProcessTexturePalette(void)
{
	int		textureIndex;
	char	texturePath[_MAX_PATH];
	char	textureName[_MAX_PATH];
	char	ext[_MAX_EXT];

	// Clear out the tex ID array construction buffer
	for (textureIndex=0; textureIndex<MAX_TEXTURES_PER_OBJECT; textureIndex++) {
		TexIDArray[textureIndex] = -1;
	}

	// Walk this FLT file's texture palette and put each texture into our global set
	if (mgGetFirstTexture(db, &textureIndex, texturePath)) {

		// Note that we have textures in this object
		if (nTextureSets == 0) {
			nTextureSets = 1;
		}

		// Handle all the textures in this object
		do {
			ShiAssert( textureIndex >= 0 );
			_splitpath( texturePath, NULL, NULL, textureName, ext );
			strcat( textureName, ext );
			ShiAssert( strlen(textureName) < sizeof(textureName) );
			
			// Put this texture into our global pool and point our mapping table to it
			if (textureIndex < MAX_TEXTURES_PER_OBJECT) {
				TexIDArray[textureIndex] = TheTextureBuildList.AddReference(textureName);
				TexIDCnt = max( TexIDCnt, textureIndex+1 );
			} else {
				char string[80];
				sprintf(string, "Texture index %d exceeds (arbitrary) max %d", textureIndex, MAX_TEXTURES_PER_OBJECT-1);
				ShiAssert(strlen(string) < sizeof(string));
				FLTwarning( db, string );
			}

		} while (mgGetNextTexture(db, &textureIndex, texturePath));
	}

	// Make a pass through the texture id array to ensure the texture sets will work out
	if (nTextureSets) {
		int texturesPerSet = TexIDCnt / nTextureSets;
		if (texturesPerSet * nTextureSets != TexIDCnt) {

			// Can't even start since the math doesn't work out
			char string[80];
			sprintf( string, "Bad texture sets.  Requested %0d sets out of %0d ids.", nTextureSets, TexIDCnt );
			FLTwarning( db, string );
			nTextureSets = 1;

		} else {

			// Check each set for completeness
			for (int set=1; set<nTextureSets; set++) {
				for (int i=0; i<texturesPerSet; i++) {
					if (TexIDArray[i] >= 0) {
						if (TexIDArray[set*texturesPerSet + i] < 0) {
							char string[80];
							sprintf( string, "Bad texture set.  Set %0d, texture %0d is missing (MG id %0d).", set, i, set*texturesPerSet+i );
							FLTwarning( db, string );
							TexIDArray[set*texturesPerSet + i] = TexIDArray[i];
						}
					}
				}
			}
		}
	}
}


void ProcessVertex(mgrec *rec, int *xyz, int *rgba, int *I, Ptexcoord *uv, Pcolor polyColor, BOOL *Aflag)
{
	int		retval;

	// GameGen values
	double				x, y, z;
	short				r, g, b;
	float				i, j, k;
	char				*name;
	AlphaPatchRecord	*aPatch;

	// Our values
	struct {
		float	x, y, z;
		Pcolor	color;
		float	i, j, k;
		float	u, v;
	} local;

	// Get the name of the vertex
	name = mgGetName( rec );

	// Get the position of the vertex from the FLT file
	// We convert right here from GameGen space to our internal
	// space.
	// GameGen:	X right, Y front, Z up
	// Us:		X front, Y right, Z down
	retval = mgGetIcoord(rec, fltIcoord, &x, &y, &z);
	if (retval == MG_TRUE) {
		local.x = (float)y;
		local.y = (float)x;
		local.z = -(float)z;
	} else {
		FLTwarning( rec, "Failed to get vertex position." );
		local.x = 0.0f;
		local.y = 0.0f;
		local.z = 0.0f;
	}
	// If we'ge got local coordinates in effect, transform the point into them
	if (LocalCoords) {
		Ppoint p = { local.x+Translation.x, local.y+Translation.y, local.z+Translation.z };
		Ppoint l;

		MatrixMult( &Rotation, &p, &l );
		local.x = l.x;
		local.y = l.y;
		local.z = l.z;
	}
	if ( TheDynamicPatchList.IsDynamic(name) ) {
		PosPool->AddReference( xyz, local.x, local.y, local.z, Dynamic );
	} else {
		PosPool->AddReference( xyz, local.x, local.y, local.z, Static );
	}


	// Get color properties
	if (rgba) {

		// Get the vertex color from the FLT file
		retval = mgGetVtxColorRGB (rec, &r, &g, &b );
		if (retval == MG_TRUE) {
			local.color.r = r / 255.0f;
			local.color.g = g / 255.0f;
			local.color.b = b / 255.0f;
		} else {
			local.color = polyColor;
		}

		// Get the name of this vertex and see if its alpha value is overridden
		aPatch = TheAlphaPatchList.GetPatch( name );
		if (aPatch) {
			if (aPatch->OLD) {
				local.color.r = aPatch->r;
				local.color.g = aPatch->g;
				local.color.b = aPatch->b;
			}
			local.color.a = aPatch->alpha;
		} else {
			local.color.a = polyColor.a;
		}
		if (local.color.a < 1.0f) {
			*Aflag = TRUE;
		}

		TheColorBuildList.AddReference( rgba, local.color, PrelightColors );
	}


	// Get the vertex normal from the FLT file
	if (I) {
		retval = mgGetVtxNormal (rec, &i, &j, &k );
		if (retval == MG_TRUE) {
			// Make ajustments to get from (Y front, X right, Z up) into (X front, Y right, Z down)
			local.i =  j;
			local.j =  i;
			local.k = -k;
		} else {
			FLTwarning(rec, "Failed to get vertex normal");
			local.i =  0.0f;
			local.j =  0.0f;
			local.k = -1.0f;
		}
		// If we'ge got local coordinates in effect, transform the vector into them
		if (LocalCoords) {
			Ppoint p = { local.i, local.j, local.k };
			Ppoint l;

			MatrixMult( &Rotation, &p, &l );
			local.i = l.x;
			local.j = l.y;
			local.k = l.z;
		}
		*I = AddNormalToTable( local.i, local.j, local.k );
	}


	// Get vertex texture coordinates
	// NOTE: GameGen uses 0,0 at lower left while MPR uses 0,0 at upper left, 
	//       so we swap the sign on V.
	if (uv) {
		retval = mgGetAttList ( rec, fltVU, &local.u, fltVV, &local.v, MG_NULL );
		if (retval == 2) {
			uv->u =	local.u;
			uv->v = 1.0f - local.v;
		} else {
			FLTwarning(rec, "Failed to get texture coordinates");
			uv->u = 0.0f;
			uv->v = 0.0f;
		}
	}

	// Free the name string we got from GameGen
	mgFree(name);
}


int ProcessAllVerts(mgrec *rec, int *xyz, int *rgba, int *I, Ptexcoord *uv, Pcolor polyColor, BOOL *Aflag)
{
	mgrec	*child;
	int		vertCount = 0;

	// Walk through all the children of the provided polygon record
	child = mgGetChild(rec);
	while (child) {
		if( mgIsCode(child, fltVertex) ) {
			ProcessVertex( child, xyz, rgba, I, uv, polyColor, Aflag );
			xyz++;
			if (rgba)	rgba++;
			if (I)		I++;
			if (uv)		uv++;
			vertCount++;
		}
		child = mgGetNext(child);
	}

	return vertCount;
}


void GetPrimitiveColor( mgrec *rec, Pcolor *color, BOOL *Aflag )
{
	int		retval;
	short	materialIndex;

	// See if we've got a material assigned
	retval = mgGetAttList ( rec, fltPolyMaterial, &materialIndex, MG_NULL );
	if (retval != MG_TRUE) {
		FLTwarning( rec, "Failed to get poly material" );
	}


	if (materialIndex >= 0 ) {
		// We got a material so use it to the exclusion of the poly color
		mgrec			*matRec;
		matRec = mgGetMaterial( db, materialIndex );
		if (!matRec) {
//			FLTWarning( rec, "Illegal material index on primitive" );
			materialIndex = -1;
		} else {
			retval = mgGetNormColor ( matRec, fltDiffuse, &color->r, &color->g, &color->b );
			if (retval != MG_TRUE) {
				FLTwarning( rec, "Failed to get material diffuse rgb" );
				color->r = 1.0f;
				color->g = 1.0f;
				color->b = 1.0f;
			}
			retval = mgGetAttList ( matRec, fltMatAlpha, &color->a, MG_NULL );
			if (retval != 1) {
				FLTwarning( rec, "Failed to get material alpha" );
				color->a = 1.0f;
			}
		}
	}

	if (materialIndex < 0 ) {
		// No material provided, so fall back to the poly color
		short			red, green, blue;
		unsigned short	alpha;
		retval = mgGetPolyColorRGB( rec, &red, &green, &blue );
		if (retval != MG_TRUE) {
			FLTwarning( rec, "Failed to get poly rgb" );
			red		= 255;
			green	= 255;
			blue	= 255;
		}
		retval = mgGetAttList ( rec, fltPolyTransparency, &alpha, MG_NULL );
		if (retval != 1) {
			FLTwarning( rec, "Failed to get poly alpha" );
			alpha = 0;
		}
		color->r = red		/ 255.0f;
		color->g = green	/ 255.0f;
		color->b = blue		/ 255.0f;
		color->a = 1.0f - alpha / 65535.0f;
	}

	if (color->a == 1.0f) {
		*Aflag = FALSE;
	} else if (color->a > 0.95f) {
		FLTwarning( rec, "Polygon with alpha is > 95% opaque.  Going to solid." );
		*Aflag = FALSE;
		color->a = 1.0f;
	} else {
		*Aflag = TRUE;
	}
}


void GetPrimitiveLightingAndTexture( mgrec *rec, unsigned char *lightMode, short *texIndex )
{
	DWORD	retval;

	ShiAssert( lightMode );
	ShiAssert( texIndex );

	// lightMode (0 flat, 1 Gouraud, 2 lit flat, 3 lit Gouraud)
	retval = mgGetAttList ( rec, fltGcLightMode, lightMode, MG_NULL );
	if (retval != MG_TRUE) {
		FLTwarning( rec, "Failed to get poly lighting mode" );
		*lightMode = 0;
	}

	// Query the polygon's texture
	retval = mgGetAttList ( rec, fltPolyTexture, texIndex, MG_NULL );
	if (retval != MG_TRUE) {
		FLTwarning( rec, "Failed to get poly texture attribute" );
		*texIndex = -1;
	}
	if (*texIndex >= 0 ) {
		if ((*texIndex >= TexIDCnt) || (TexIDArray[*texIndex] < 0)) {
//			FLTwarning( rec, "poly has illegal texture index" );
			*texIndex = -1;
		}
	}
}


void GetPrimitiveNormal( mgrec *rec, float *A, float *B, float *C )
{
	DWORD	retval;
	double	i, j, k;
	Ppoint	n;

	// Get the polygon's normal
	retval = mgGetPolyNormal( rec, &i, &j, &k );
	if (retval != MG_TRUE) {
		FLTwarning( rec, "Failed to get poly normal (degenerate polygon?)" );
		i = 0.0f;
		j = 0.0f;
		k = 1.0f;
	}

	// Make ajustments to get the normal from (Y front, X right, Z up) into
	// (X front, Y right, Z down).
	n.x =  (float)j;
	n.y =  (float)i;
	n.z = -(float)k;

	// If we'ge got local coordinates in effect, transform the normal into them
	if (LocalCoords) {
		Ppoint l;

		MatrixMult( &Rotation, &n, &l );
		n.x = l.x;
		n.y = l.y;
		n.z = l.z;
	}

	*A = n.x;
	*B = n.y;
	*C = n.z;
}


// WARNING.  Keep in mind that the polygon returned is DEPENDENT on the polygon passed in.
// The output polygon points at the same vertex lists as the input polygon _except_ for the
// normal list (if any).  The two polys become inextricably linked.  One cannot exist without
// the other.
Poly *ReversePoly(Poly *poly)
{
	Poly			*p		= NULL;
	int				*I		= NULL;
	Pnormal			*N;
	int				*ISrc;
	int				nVerts = poly->nVerts;

	ShiAssert( poly );
	ShiAssert( nVerts );

	// Construct the proper type of copy
	switch (poly->type) {
	  case FL:
	  case AFL:
		p = new PolyFCN;
		TheColorBuildList.AddReference( &((PolyFCN*)p)->rgba, &((PolyFCN*)poly)->rgba );
		N = &NormalArray[((PolyFCN*)poly)->I];
		((PolyFCN*)p)->I = AddNormalToTable( -N->i, -N->j, -N->k );
		break;

	  case GL:
	  case AGL:
		p = new PolyVCN;
		((PolyVCN*)p)->rgba				= ((PolyVCN*)poly)->rgba;
		((PolyVCN*)p)->I				= I		= new int[nVerts];
		ISrc				= ((PolyVCN*)poly)->I;
		break;

	  case TexL:
	  case ATexL:
	  case CTexL:
	  case CATexL:
		p = new PolyTexFCN;
		TheColorBuildList.AddReference( &((PolyTexFCN*)p)->rgba, &((PolyTexFCN*)poly)->rgba );
		((PolyTexFCN*)p)->texIndex		= ((PolyTexFCN*)poly)->texIndex;
		((PolyTexFCN*)p)->uv			= ((PolyTexVCN*)poly)->uv;
		N = &NormalArray[((PolyTexFCN*)poly)->I];
		((PolyTexFCN*)p)->I = AddNormalToTable( -N->i, -N->j, -N->k );
		break;

	  case TexGL:
	  case ATexGL:
	  case CTexGL:
	  case CATexGL:
		p = new PolyTexVCN;
		((PolyTexVCN*)p)->texIndex		= ((PolyTexVCN*)poly)->texIndex;
		((PolyTexVCN*)p)->rgba			= ((PolyTexVCN*)poly)->rgba;
		((PolyTexVCN*)p)->uv			= ((PolyTexVCN*)poly)->uv;
		((PolyTexVCN*)p)->I				= I		= new int[nVerts];
		ISrc				= ((PolyTexVCN*)poly)->I;
		break;

	  default:
		FLTwarning(db, "Code error.  Tried to reverse an unhandled polygon");
		return NULL;
	}

	// Copy the standard polygon info
	p->type		= poly->type;
	p->xyz		= poly->xyz;
	p->nVerts	= nVerts;

	// Invert the poly normal (not used for back facing polys, but might as well...)
	p->A = -poly->A;
	p->B = -poly->B;
	p->C = -poly->C;
	p->D = -poly->D;

	// Invert the vertex normals, if required
	if (I) {
		while (nVerts--) {
			N		= &NormalArray[*ISrc++];
			*I++	= AddNormalToTable( -N->i, -N->j, -N->k );
		}
	}

	return p;
}


BNode* ProcessSlot( mgrec *rec )
{
	BSlotNode	*node;
	char		*name;
	char		s[4];
	int			slotNum = -1;
	mgrec		*child;
	double		x, y, z;
	int			retval;
	mgmatrix	matrix;


	// Determine if this is a slot
	name = mgGetName( rec );
	sscanf(name, "%c%c%c%c%d", &s[0], &s[1], &s[2], &s[3], &slotNum);
	mgFree(name);
	if (strnicmp(s, "SLOT", 4) != 0) {

		// Well we've got to support Ericks old system of naming the parent object, instead 
		// of the polygon, so lets check that now...
		name = mgGetName( mgGetParent(rec) );
		sscanf(name, "%c%c%c%c%d", &s[0], &s[1], &s[2], &s[3], &slotNum);
		mgFree(name);
		if (strnicmp(s, "SLOT", 4) != 0) {
			return NULL;
		}

	}
	if (slotNum < 1) {
		FLTwarning( rec, "No slot number.  Should be named SLOT## where ## is the slot number" );
		return NULL;
	}


	// Get the child vertex record
	child = mgGetChild( rec );
	if( (!child) || (!mgIsCode(child, fltVertex)) || (mgGetNext(child))) {
		FLTwarning( rec, "Slot should have exactly one vertex as child!" );
		return NULL;
	}


	// Okay, now build the proper node and return it
	node = new BSlotNode;
	ShiAssert( node );
	nSlots = max( nSlots, slotNum );
	node->slotNumber = slotNum-1;	// Convert from 1 based to 0 based counting

	// Query for the local transformation on our parent object
	retval = mgGetMatrix( mgGetParent(rec), fltMatrix, &matrix );
	if (retval == MG_FALSE) {
		node->rotation = IMatrix;
	} else {
		// Here we reorder the matrix elements to get from GG to our coordinate system.
		node->rotation.M11 =  (float)matrix[5];
		node->rotation.M21 =  (float)matrix[4];
		node->rotation.M31 = -(float)matrix[6];

		node->rotation.M12 =  (float)matrix[1];
		node->rotation.M22 =  (float)matrix[0];
		node->rotation.M32 = -(float)matrix[2];

		node->rotation.M13 = -(float)matrix[9];
		node->rotation.M23 = -(float)matrix[8];
		node->rotation.M33 =  (float)matrix[10];
	}

	// Get the location of this slot
	// We convert right here from GameGen space to our internal
	// space.
	// GameGen:	X right, Y front, Z up
	// Us:		X front, Y right, Z down
	retval = mgGetIcoord(child, fltIcoord, &x, &y, &z);
	if (retval == MG_TRUE) {
		node->origin.x = (float)y;
		node->origin.y = (float)x;
		node->origin.z = -(float)z;
	} else {
		FLTwarning( rec, "Failed to get slot position." );
		node->origin.x = 0.0f;
		node->origin.y = 0.0f;
		node->origin.z = 0.0f;
	}


	// Adjust the origin and orientation of the slot if local coordinates are in effect
	if (LocalCoords) {
		Ppoint  p = { node->origin.x+Translation.x, node->origin.y+Translation.y, node->origin.z+Translation.z };
		Pmatrix m = node->rotation;

		MatrixMult( &Rotation, &p, &node->origin );
		MatrixMult( &Rotation, &m, &node->rotation );
	}



	// Store the slot position in our global accumulation array
	if (node->slotNumber >= MAX_SLOT_AND_DYNAMIC_PER_OBJECT) {
		FLTwarning( rec, "Slot number exceeds arbitrary maximum.  Change object or code." );
		delete node;
		return NULL;
	}
	SlotPositionArray[node->slotNumber] = node->origin;

	return node;
}


BNode* ProcessPointLine(mgrec *rec, int nVerts)
{
	Pcolor			color;
	BPrimitiveNode	*node;
	Prim			*prim;
	BOOL			Aflag;

	// Don't bother with degenerates
	if (nVerts < 1) {
		FLTwarning( rec, "Skipping polygon with no verts!" );
		return NULL;
	}

	// Determine if this is a slot
	if (nVerts == 1) {
		BNode *n = ProcessSlot( rec );
		if (n) {
			return n;
		}
	}

	// Query the polygon's color
	GetPrimitiveColor( rec, &color, &Aflag );

	// Set up the primitive for the point or line we're constructing
	prim = new Prim;

	// For now we assume 1 vert means point, more means line
	if (nVerts == 1) {
		prim->type = PointF;
		TheColorBuildList.AddReference( &((PrimPointFC*)prim)->rgba, color, PrelightColors );
	} else {
		prim->type = LineF;
//		TheColorBuildList.AddReference( &((PrimLineFC*)prim)->rgba, color, PrelightColors );
		TheColorBuildList.AddReference( &((PrimLineFC*)prim)->rgba, color, TRUE );
	}

	// Set up the vertex position index array
	prim->xyz = new int[nVerts];
	prim->nVerts = nVerts;
	nVerts = ProcessAllVerts( rec, prim->xyz, NULL, NULL, NULL, color, &Aflag );
	ShiAssert( nVerts == prim->nVerts );

	// We don't handle alpha blending points or lines right now...
	if (Aflag) {
		FLTwarning( rec, "Alpha blended points and lines are not supported.");
	}

	// Construct the primitive node
	node = new BPrimitiveNode;
	node->prim = prim;

	return node;
}


BNode* ProcessLightString(mgrec *rec)
{
	mgrec			*child;
	int				vertCount = 0;
	int				retval;
	short			red, green, blue;
	Pcolor			color;
	BOOL			Aflag;
	BPrimitiveNode	*node;
	PrimPointFC		*prim;
	unsigned		directional;
	unsigned		colorIndex;
	float			i, j, k;
	struct {
		float	i;
		float	j;
		float	k;
	}				local;


	// Get our vertex count
	child = mgGetChild(rec);
	while (child) {
		if( mgIsCode(child, fltVertex) ) {
			vertCount++;
		}
		child = mgGetNext(child);
	}
	if (NormalCnt+vertCount > MAX_VERTS_PER_OBJECT_TREE) {
		FLTwarning( rec, "Too many verts in object!  Simplify or update the code." );
		return NULL;
	}

	// Don't bother with degenerates
	if (vertCount < 1) {
		FLTwarning( rec, "Skipping light string with no verts!" );
		return NULL;
	}

	// Make sure we don't have too many points
	if (vertCount > MAX_VERTS_PER_POLYGON) {
		char	message[80];
		sprintf( message, "Skipping light string with %0d verts (max is %0d)", vertCount, MAX_VERTS_PER_POLYGON );
		FLTwarning( rec, message );
		return NULL;
	}


	// Query the light color
	child = mgGetChild(rec);
	ShiAssert( mgIsCode(child, fltVertex) );
	retval = mgGetAttList ( child, fltVColor, &colorIndex, MG_NULL );
	if (retval != MG_TRUE) {
		FLTwarning( rec, "Failed to get front color index from first vertex" );
		colorIndex = 0;
	}
	mgIndex2RGB (db, colorIndex, 1.0f, &red, &green, &blue);
	color.r = red	/ 255.0f;
	color.g = green	/ 255.0f;
	color.b = blue	/ 255.0f;
	color.a = 1.0f;


	// See if we need to be bi-directional
	retval = mgGetAttList ( rec, fltLpDirectionalityType, &directional, MG_NULL );
	if (retval != MG_TRUE) {
		FLTwarning( rec, "Failed to get directional attribute" );
		directional = 0;
	}
	if (directional == 1) {
		FLTwarning( rec, "Unidirectional lights not supported.  Converted to omni." );
		directional = 0;
	}


	// Set up the primitive for the point we're constructing
	prim = new PrimPointFC;
	prim->type = PointF;


	// Set up the vertex position index array
	prim->xyz = new int[vertCount];
	prim->nVerts = vertCount;
	vertCount = ProcessAllVerts( rec, prim->xyz, NULL, NULL, NULL, color, &Aflag );
	ShiAssert( vertCount == prim->nVerts );

	// We don't handle alpha blending on light strings
	if (Aflag) {
//		FLTwarning( rec, "Alpha blended light strings are not supported.");
	}


	// Do extra work if we're bi-directional
	if (directional == 2) {
		// Construct the appropriate node type
		node = new BLightStringNode;
		node->prim = prim;
		prim->rgba = 0;

		// Get the normal on the first vertex in the string
		child = mgGetChild(rec);
		ShiAssert( mgIsCode(child, fltVertex) );
		retval = mgGetVtxNormal (child, &i, &j, &k );
		if (retval == MG_TRUE) {
			// Make ajustments to get from (Y front, X right, Z up) into (X front, Y right, Z down)
			local.i =  j;
			local.j =  i;
			local.k = -k;
		} else {
			FLTwarning(child, "Failed to get vertex normal from first light point");
			local.i = 1.0f;
			local.j = 0.0f;
			local.k = 0.0f;
		}
		// If we'ge got local coordinates in effect, transform the vector into them
		if (LocalCoords) {
			Ppoint p = { local.i, local.j, local.k };
			Ppoint l;

			MatrixMult( &Rotation, &p, &l );
			local.i = l.x;
			local.j = l.y;
			local.k = l.z;
		}
		
		// Set up the plane that divides the front and back hemispheres
		((BLightStringNode*)node)->A = local.i;
		((BLightStringNode*)node)->B = local.j;
		((BLightStringNode*)node)->C = local.k;
		((BLightStringNode*)node)->D = 
			-PosPool->GetPosFromTarget(prim->xyz)->x * local.i
			-PosPool->GetPosFromTarget(prim->xyz)->y * local.j
			-PosPool->GetPosFromTarget(prim->xyz)->z * local.k;

		// Add the light's front color to the color bank
		TheColorBuildList.AddReference( &((BLightStringNode*)node)->rgbaFront, color, FALSE );

		// Get the back color
		retval = mgGetAttList ( rec, fltLpBackColor, &colorIndex, MG_NULL );
		if (retval != MG_TRUE) {
			FLTwarning( rec, "Failed to get back color index" );
			colorIndex = 0;
		}
		mgIndex2RGB (db, colorIndex, 1.0f, &red, &green, &blue);
		color.r = red	/ 255.0f;
		color.g = green	/ 255.0f;
		color.b = blue	/ 255.0f;
		color.a = 1.0f;

		// Add the light's back color to the color bank
		TheColorBuildList.AddReference( &((BLightStringNode*)node)->rgbaBack,  color, FALSE );
	} else {
		// Construct the appropriate node type
		node = new BPrimitiveNode;
		node->prim = prim;

		// Add the light's color to the color bank
		TheColorBuildList.AddReference( &prim->rgba, color, FALSE );
	}


	return node;
}


BNode* ProcessPolygon(mgrec *rec, int nVerts)
{
	int							retval;
	Pcolor						color;
	float						A, B, C;
	unsigned char				lightMode;
	short						textureIndex;
	BOOL						Aflag;
	BOOL						ChromaFlag = FALSE;
	unsigned char				BilinearAlphaPerTexelFlag = 0;
	BTransformType				RotationType = Normal;
	BuildTimePosList			*savePosPool;

	char			drawType;
	char			billboardType;
	BNode			*node;
	Poly			*poly;
	int				*xyz	= NULL;
	int				*rgba	= NULL;
	int				*I		= NULL;
	Ptexcoord		*uv		= NULL;


	ShiAssert( nVerts >= 3 );


	// Get the primitives lighting mode and texture to decide how it will draw
	GetPrimitiveColor( rec, &color, &Aflag );
	GetPrimitiveNormal( rec, &A, &B, &C );
	GetPrimitiveLightingAndTexture( rec, &lightMode, &textureIndex );

	// Figure out if this is a Tree or Billboard type polygon
	retval = mgGetAttList ( rec, fltPolyMgTemplate, &billboardType, MG_NULL );
	if (retval != 1) {
		FLTwarning( rec, "Failed to get poly billboard type.  Defaulting to NONE." );
		billboardType = 0;
	}
	switch (billboardType) {
	  case 0:		// None
		break;
	  case 1:		// Fixed (Chromakey only)
		if (textureIndex >= 0)  ChromaFlag = TRUE;
		break;
	  case 2:		// Axis (Tree)
		RotationType	= Tree;
		savePosPool		= PosPool;
		PosPool			= new BuildTimePosList;
		if (textureIndex >= 0)  ChromaFlag = TRUE;
		break;
	  case 4:		// Point (Billboard)
		RotationType	= Billboard;
		savePosPool		= PosPool;
		PosPool			= new BuildTimePosList;
		if (textureIndex >= 0)  ChromaFlag = TRUE;
		break;
	  default:
		FLTwarning( rec, "Unrecognized billboard type.  Defaulting to NONE." );
	}


	// Construct a polygon of the appropriate type
	switch (lightMode) {
	  case 0:	// flat
		if (textureIndex >= 0) {
			poly							= new PolyTexFC;
			poly->type						= Tex;
			((PolyTexFC*)poly)->texIndex	= textureIndex;
			((PolyTexFC*)poly)->uv			= uv	= new Ptexcoord[nVerts];
		} else {
			poly							= new PolyFC;
			poly->type						= F;
		}
		TheColorBuildList.AddReference( &((PolyFC*)poly)->rgba, color, PrelightColors );
		break;
	  case 1:	// Gouraud
		if (textureIndex >= 0) {
			poly							= new PolyTexVC;
			poly->type						= TexG;
			((PolyTexFC*)poly)->texIndex	= textureIndex;
			((PolyTexFC*)poly)->uv			= uv	= new Ptexcoord[nVerts];
		} else {
			poly							= new PolyVC;
			poly->type						= G;
		}
		((PolyVC*)poly)->rgba				= rgba	= new int[nVerts];
		break;
	  case 2:	// lit flat
		if (textureIndex >= 0) {
			poly							= new PolyTexFCN;
			poly->type						= TexL;
			((PolyTexFCN*)poly)->texIndex	= textureIndex;
			((PolyTexFCN*)poly)->uv			= uv	= new Ptexcoord[nVerts];
		} else {
			poly							= new PolyFCN;
			poly->type						= FL;
		}
		TheColorBuildList.AddReference( &((PolyFCN*)poly)->rgba, color, PrelightColors );
		((PolyFCN*)poly)->I	= AddNormalToTable( A, B, C );
		break;
	  case 3:	// lit Gouraud
		if (textureIndex >= 0) {
			poly							= new PolyTexVCN;
			poly->type						= TexGL;
			((PolyTexVCN*)poly)->texIndex	= textureIndex;
			((PolyTexVCN*)poly)->uv			= uv	= new Ptexcoord[nVerts];
		} else {
			poly							= new PolyVCN;
			poly->type						= GL;
		}
		((PolyVCN*)poly)->rgba				= rgba	= new int[nVerts];
		((PolyVCN*)poly)->I					= I		= new int[nVerts];
		break;
	}


	// Fill in the common fields for the polygon and get its vertex data
	poly->nVerts			= nVerts;
	poly->xyz				= xyz = new int[nVerts];
	nVerts = ProcessAllVerts( rec, xyz, rgba, I, uv, color, &Aflag );
	ShiAssert( nVerts == poly->nVerts );

	// Fill in the polygon's plane equation (for back face culling if required)
	poly->A = A;
	poly->B = B;
	poly->C = C;
	Ppoint *pp = PosPool->GetPosFromTarget(poly->xyz);
	poly->D = -pp->x * A -pp->y * B -pp->z * C;
#ifdef _DEBUG
	char *name = mgGetName (rec);
	char buf[1024];
	sprintf (buf, "Poly %s x %9.3f y %9.3f z %9.3f, plane A %9.3f B %9.3f C %9.3f D %9.3f\n",
	    name, 
	    pp->x, pp->y, pp->z,
	    poly->A, poly->B, poly->C, poly->D);
	OutputDebugString(buf);
	mgFree(name);
#endif

	// Ajust the poly type if it is chromakeyed
	if (ChromaFlag) {
		poly->type = (PpolyType)(poly->type + CTex-Tex);
	}

	// Adjust the poly type if it requires alpha blending
	if (Aflag) {
		ShiAssert( (poly->type + AF-F) < PpolyTypeNum );
		poly->type = (PpolyType)(poly->type + AF-F);
	}


	// Special case for the virtual cockpit (want bilinear and alpha per texel...)
	// NOTE:  We're missusing the "line style" property of the face since it is
	// available in the UI and otherwise ignored.
	retval = mgGetAttList ( rec, fltPolyLineStyle, &BilinearAlphaPerTexelFlag, MG_NULL );
	if (retval != 1) {
		FLTwarning( rec, "Failed to get poly line style (used for BilinearAPT).  Defaulting to 0." );
		BilinearAlphaPerTexelFlag = 0;
	}
	if (BilinearAlphaPerTexelFlag) {
		if( poly->type != ATex ) {
			FLTwarning( rec, "Line Style controls Bilinear/APT and only works with Flat Transparent faces." );
		} else {
			// Now hack to the special poly type.
			poly->type = BAptTex;

			// And flag the texture as an Alpha Per Texel (.APL)
			TheTextureBuildList.ConvertToAPL( TexIDArray[textureIndex] );
		}
	}


	// Decide if this polygon should be subject to back face culling and create it's node
	retval = mgGetAttList ( rec, fltPolyDrawType, &drawType, MG_NULL );
	if (retval != 1) {
		FLTwarning( rec, "Failed to get poly draw type.  Default to solid back face culled." );
		drawType = 0;
	}
	if ((drawType == 1) || (RotationType != Normal)){
		if (lightMode > 1) {
			node = new BLitPrimitiveNode;
			((BLitPrimitiveNode*)node)->poly = poly;
			((BLitPrimitiveNode*)node)->backpoly = ReversePoly(poly);
		} else {
			node = new BPrimitiveNode;
			((BPrimitiveNode*)node)->prim = poly;
		}
	} else {
		if (drawType != 0) {
			FLTwarning(rec, "Unrecognized polygon type.  Converting to solid back face culled.");
		}
		node = new BCulledPrimitiveNode;
		((BCulledPrimitiveNode*)node)->poly = poly;
	}


	// If we're dealing with a billboard or tree, construct a parental node to handle the transform
	if (RotationType != Normal) {
		BNode *child = node;

		node = new BSpecialXform;
		((BSpecialXform*)node)->pCoords = PosPool->GetPool();
		((BSpecialXform*)node)->nCoords = poly->nVerts;
		((BSpecialXform*)node)->subTree = child;

		if (RotationType == Billboard) {
			((BSpecialXform*)node)->type = Billboard;
		} else {
			((BSpecialXform*)node)->type = Tree;
		}

		// Add in the bounding radius of our private pool to the original pool
		float x = max( fabs(PosPool->SizeInfo.maxX), fabs(PosPool->SizeInfo.minX) );
		float y = max( fabs(PosPool->SizeInfo.maxY), fabs(PosPool->SizeInfo.minY) );
		float z = max( fabs(PosPool->SizeInfo.maxZ), fabs(PosPool->SizeInfo.minZ) );
		float r = x*x + y*y + z*z;

		savePosPool->SizeInfo.radiusSquared	= max( savePosPool->SizeInfo.radiusSquared, r ); 
		savePosPool->SizeInfo.maxX			= max( savePosPool->SizeInfo.maxX,  x );
		savePosPool->SizeInfo.maxY			= max( savePosPool->SizeInfo.maxY,  y );
		savePosPool->SizeInfo.maxZ			= max( savePosPool->SizeInfo.maxZ,  z );
		savePosPool->SizeInfo.minX			= min( savePosPool->SizeInfo.minX, -x );
		savePosPool->SizeInfo.minY			= min( savePosPool->SizeInfo.minY, -y );
		savePosPool->SizeInfo.minZ			= min( savePosPool->SizeInfo.minZ, -z );

		// Put the original pool back
		delete PosPool;
		PosPool = savePosPool;
	}

	return node;
}


BNode* ProcessPrimitive(mgrec *rec)
{
	mgrec			*child;
	int				vertCount = 0;
	

	// Get our vertex count
	child = mgGetChild(rec);
	while (child) {
		if( mgIsCode(child, fltVertex) ) {
			vertCount++;
		}
		child = mgGetNext(child);
	}
	if (vertCount > MAX_VERTS_PER_POLYGON) {
		FLTwarning( rec, "Too many verts in this poly!  Subdivid or update the code." );
		return NULL;
	}
	if (NormalCnt+vertCount > MAX_VERTS_PER_OBJECT_TREE) {
		FLTwarning( rec, "Too many verts in object!  Simplify or update the code." );
		return NULL;
	}


	// Decide if this is a polygon or a point or line primitive or a "special" point
	if (vertCount > 2) {
		return ProcessPolygon(rec, vertCount);
	} else {
		return ProcessPointLine(rec, vertCount);
	}
}


BOOL ProcessSubTree(BSubTree *node, mgrec *rec, SzInfo_t *szInfo, BOOL IncludeParent, Pmatrix *R = NULL, Ppoint *T = NULL )
{
	ShiAssert( node );
	if (!rec)  return FALSE;

	StartSubObject( R, T );

	if (IncludeParent) {
		node->subTree = ProcessRecord( rec );
	} else { 
		node->subTree = ProcessAllChildren( rec );
	}


	if (!node->subTree) {
		ShiAssert( PosPool->numTotal == 0 );
		ShiAssert( NormalCnt == 0 );
		FLTwarning( rec, "Empty subtree." );
		EndSubObject();
		return FALSE;
	}

	// Resolve the position references and move the accumulated data into the node
	if (PosPool->numTotal) {
		node->pCoords			= PosPool->GetPool();
		node->nCoords			= PosPool->numTotal;
		node->nDynamicCoords	= PosPool->numDynamic;

		if (PosPool->numDynamic) {
			if (nDynamicCoords+PosPool->numDynamic >= MAX_SLOT_AND_DYNAMIC_PER_OBJECT) {
				FLTwarning( rec, "Dynamic vertex count exceeds arbitrary maximum.  Change object or code." );
				node->DynamicCoordOffset= -1;
			} else {
				node->DynamicCoordOffset = nDynamicCoords;

				// Copy the dynamic verts original locations
				for (int i=0; i<PosPool->numDynamic; i++) {
					DynamicPositionArray[node->DynamicCoordOffset+i] = 
						*(node->pCoords+PosPool->numStatic+i);
				}

				nDynamicCoords += PosPool->numDynamic;
			}
		} else {
			node->DynamicCoordOffset= -1;
		}
	} else {
		node->pCoords			= NULL;
		node->nCoords			= 0;
		node->nDynamicCoords	= 0;
	}

	// Copy the accumulated normal data into the node
	node->nNormals	= NormalCnt;
	if (NormalCnt) {
		node->pNormals	= new Pnormal[NormalCnt];
		memcpy( node->pNormals, NormalArray,   sizeof(*NormalArray)*NormalCnt );
	} else {
		node->pNormals	= NULL;
	}
	szInfo->radiusSquared	= max( szInfo->radiusSquared, PosPool->SizeInfo.radiusSquared ); 
	szInfo->maxX			= max( szInfo->maxX, PosPool->SizeInfo.maxX );
	szInfo->maxY			= max( szInfo->maxY, PosPool->SizeInfo.maxY );
	szInfo->maxZ			= max( szInfo->maxZ, PosPool->SizeInfo.maxZ );
	szInfo->minX			= min( szInfo->minX, PosPool->SizeInfo.minX );
	szInfo->minY			= min( szInfo->minY, PosPool->SizeInfo.minY );
	szInfo->minZ			= min( szInfo->minZ, PosPool->SizeInfo.minZ );

	EndSubObject();

	return TRUE;
}


BNode* ProcessBsp(mgrec *rec)
{
	BSplitterNode	*node;
	double			A, B, C, D;
	mgrec			*front, *back, *next;
	int				attCnt;
	int				stepCount = 0;

	// Do plane stuff here
	node = new BSplitterNode;

	attCnt = mgGetAttList(rec, 
						  fltDPlaneA, &A, 
						  fltDPlaneB, &B, 
						  fltDPlaneC, &C, 
						  fltDPlaneD, &D, 
						  MG_NULL );
	if (attCnt != 4) {
		FLTwarning( rec, "Failed to get BSP plane equation" );
	}

	// Here we're adjusting the plane equation coefficients to get from
	// GameGen:  Y front, X right, Z up
	//    to
	// Us:		 X front, Y right, Z down
	node->A =  (float)B;
	node->B =  (float)A;
	node->C = -(float)C;
	node->D =  (float)D;

	// If we'ge got local coordinates in effect, transform the plane equation into them
	if (LocalCoords) {
		Ppoint n = { node->A, node->B, node->C };
		Ppoint p = { node->A*node->D+Translation.x, node->B*node->D+Translation.y, node->C*node->D+Translation.z };
		Ppoint l;
		
		// Rotate the normal
		MatrixMult( &Rotation, &n, &l );
		node->A = l.x;
		node->B = l.y;
		node->C = l.z;

		// Recompute the distance to origin
		MatrixMult( &Rotation, &p, &l );
		node->D = -(node->A*p.x + node->B*p.y + node->B*p.z);
	}

	// Now get the front and back sub trees, skipping the splitting plane
	back = NULL;
	front = mgGetChild(rec);
	next = mgGetNext(front);
	while (next) {
		back = next;
		next = mgGetNext(next);
		stepCount++;
	};
	if (stepCount != 2) {
		FLTwarning( rec, "BSP node has too many/too few children" );
	}

	// Process the subtrees
	node->front	= ProcessRecord( front );
	if (back)
	    node->back	= ProcessRecord( back );
	else node -> back = NULL;

	// Handle case where one or both sub-trees are empty
	if (node->back && node->front) {
		return node;
	} else {
		BNode *subtree;

		FLTwarning( rec, "BSP tree is degenerate" );

		if (node->back) {
			subtree = node->back;
		} else {
			subtree = node->front;
		}
		node->front = NULL;
		node->back = NULL;
		delete node;
		return subtree;
	}
}


BNode* ProcessSwitch(mgrec *rec)
{
	static const int	MAX_SWITCH_CHILDREN = 32;

	mgrec			*child;
	char			*name;
	int				i;
	BSubTree		*subTrees[MAX_SWITCH_CHILDREN];
	int				swNum = -1;
	BSwitchNode		*node = new BSwitchNode;


	// Get the record name from GameGen
	name = mgGetName( rec );

	// Extract the switch number from the switch name
	sscanf(name, "%*[a-z,A-Z]%d", &swNum);

	// swNum >= 10000 is a special case to allow single switch numbers to control multiple
	// switch beads in the tree.  In this case, the switch is named SW##xx where ## is the
	// switch number (1 based) and the xx field is used only for uniqueness within GameGen.
	if (swNum >= 10000) {
		swNum -= 10000;
		swNum /= 100;
	}

	if ((swNum < 1) || (swNum > 99)) {
		FLTwarning( rec, "Bad SW number.  Should be named SW## or SW1##xx where ## is the switch number" );
		return NULL;
	}

	nSwitches = max( nSwitches, swNum );
	node->switchNumber = swNum-1;	// Convert from 1 based to 0 based counting
	mgFree(name);
	

	// Walk all our children and attach them to our switch
	node->numChildren = 0;
	i = 0;
	child = mgGetChild( rec );
	while(child) {
		ShiAssert( i < MAX_SWITCH_CHILDREN );

		subTrees[i] = new BSubTree;
		if (ProcessSubTree( subTrees[i], child, &PosPool->SizeInfo, TRUE )) {
			node->numChildren = i+1;
		} else {
			FLTwarning( child, "empty tree under switch" );
			delete subTrees[i];
			subTrees[i] = NULL;
		}

		child = mgGetNext( child );
		i++;
	}

	if (node->numChildren == 0) {
		delete node;
		FLTwarning( rec, "No legal childen on this switch" );
		return NULL;
	}


	// Now move the child pointers into private storage for the node
	node->subTrees = new BSubTree*[node->numChildren];
	ShiAssert( node->subTrees );
	for(i=0; i<node->numChildren; i++) {
		node->subTrees[i] = subTrees[i];
	}


	return node;
}


BNode* ProcessDOF(mgrec *rec)
{
	BDofNode	*node;
	char		*name;
	int			dofNum	= -1;
	int			retval;
	double		originX, originY, originZ;
	double		alignX,  alignY,  alignZ;
	double		trackX,  trackY,  trackZ;
	double		maxXtrans;
	double		minXtrans;

	Ppoint		front;
	Ppoint		right;
	Ppoint		up;
	Ppoint		origin;
	float		mag;


	// Extract the DOF number from the record name
	name = mgGetName( rec );
	sscanf(name, "%*[a-z,A-Z]%d", &dofNum);
	mgFree( name );
	if (dofNum < 1) {
		FLTwarning( rec, "Invalid DOF name.  Requires a number in its record name." );
		return ProcessAllChildren (rec);
	}
	dofNum -= 1;	// Convert from 1 base to 0 based indexing


	// Get our origin and axes and translation limits
	retval = mgGetAttList ( rec,	fltDofPutAnchorX,	&originX,	// Origin in parent space
									fltDofPutAnchorY,	&originY,
									fltDofPutAnchorZ,	&originZ,
									fltDofPutAlignX,	&alignX,	// Point on new GG X axis in parent space
									fltDofPutAlignY,	&alignY,
									fltDofPutAlignZ,	&alignZ,
									fltDofPutTrackX,	&trackX,	// Point on new GG XY plane in parent space
									fltDofPutTrackY,	&trackY,
									fltDofPutTrackZ,	&trackZ,
									fltDofMaxX,			&maxXtrans,	// Xlation limits in parent space
									fltDofMinX,			&minXtrans,
									MG_NULL );
	if (retval != 11) {
		return NULL;
	}
	if (originX < 60000.0f || originX > 60000.0f) {
		FLTwarning( rec, "Invalid DOF size - ignoring" );
		return ProcessAllChildren (rec);
	}

	// Create the DOF node
	node = new BDofNode;
	ShiAssert( node );
	node->dofNumber = dofNum;


	// Store our translation vector from global space to local space
	// NOTE: We're converting from GameGen space to our internal space.
	// GameGen:	X right, Y front, Z up
	// Us:		X front, Y right, Z down
	origin.x =  (float)originY;
	origin.y =  (float)originX;
	origin.z = -(float)originZ;

	
	// Compute the axes of our new local coordinate system in global space
	// NOTE: We're converting from GameGen space to our internal space.
	// GameGen:	X right, Y front, Z up
	// Us:		X front, Y right, Z down
	front.x =  (float)(alignY - originY);
	front.y =  (float)(alignX - originX);
	front.z = -(float)(alignZ - originZ);

	// Temporary right (not necessary perpendicular to front)
	right.x =  (float)(trackY - originY);
	right.y =  (float)(trackX - originX);
	right.z = -(float)(trackZ - originZ);

	// front cross right
	up.x = front.y * right.z - front.z * right.y;
	up.y = front.z * right.x - front.x * right.z;
	up.z = front.x * right.y - front.y * right.x;

	// up cross front
	right.x = up.y * front.z - up.z * front.y;
	right.y = up.z * front.x - up.x * front.z;
	right.z = up.x * front.y - up.y * front.x;

	// Normalize
	mag = sqrt(front.x*front.x + front.y*front.y + front.z*front.z);
	front.x /= mag;
	front.y /= mag;
	front.z /= mag;
	mag = sqrt(right.x*right.x + right.y*right.y + right.z*right.z);
	right.x /= mag;
	right.y /= mag;
	right.z /= mag;
	mag = sqrt(up.x*up.x + up.y*up.y + up.z*up.z);
	up.x /= mag;
	up.y /= mag;
	up.z /= mag;

	// Now construct the transform from world to local coordinates
	Pmatrix WfromL = {	front.x,	right.x,	up.x,
						front.y,	right.y,	up.y,
						front.z,	right.z,	up.z };


	// Compute the rotation from our local coordinates to our parent's coordinates
	MatrixMult( &Rotation, &WfromL, &node->rotation );

	// Compute the translation from our parent's origin to our new local one
	Ppoint T = { origin.x + Translation.x, 
				 origin.y + Translation.y,
				 origin.z + Translation.z };
	MatrixMult( &Rotation, &T, &node->translation );


	// Store our translation vector from global space to local space
	T.x = -origin.x;
	T.y = -origin.y;
	T.z = -origin.z;

	// Store our rotation from global space to local space
	Pmatrix R;
	MatrixTranspose( &WfromL, &R );


	// Now process our children
	SzInfo_t			sz;
	sz.radiusSquared	= -1.0f;
	sz.maxX				= -1e12f;
	sz.maxY				= -1e12f;
	sz.maxZ				= -1e12f;
	sz.minX				= 1e12f;
	sz.minY				= 1e12f;
	sz.minZ				= 1e12f;
	if (!ProcessSubTree( node, rec, &sz, FALSE, &R, &T )) {
		delete node;
		return NULL;
	}

	// Update our bounding volume for the worst case settings
	sz.maxX = node->translation.x + sqrt( sz.radiusSquared ) + (float)maxXtrans;
	sz.maxY = node->translation.y + sqrt( sz.radiusSquared );
	sz.maxZ = node->translation.z + sqrt( sz.radiusSquared );
	sz.minX = node->translation.x - sqrt( sz.radiusSquared ) + (float)minXtrans;
	sz.minY = node->translation.y - sqrt( sz.radiusSquared );
	sz.minZ = node->translation.z - sqrt( sz.radiusSquared );

	float x = max( fabs(sz.maxX), fabs(sz.minX) );
	float y = max( fabs(sz.maxY), fabs(sz.minY) );
	float z = max( fabs(sz.maxZ), fabs(sz.minZ) );

	sz.radiusSquared = x*x + y*y + z*z;

	PosPool->SizeInfo.radiusSquared	= max( PosPool->SizeInfo.radiusSquared, sz.radiusSquared ); 
	PosPool->SizeInfo.maxX			= max( PosPool->SizeInfo.maxX, sz.maxX );
	PosPool->SizeInfo.maxY			= max( PosPool->SizeInfo.maxY, sz.maxY );
	PosPool->SizeInfo.maxZ			= max( PosPool->SizeInfo.maxZ, sz.maxZ );
	PosPool->SizeInfo.minX			= min( PosPool->SizeInfo.minX, sz.minX );
	PosPool->SizeInfo.minY			= min( PosPool->SizeInfo.minY, sz.minY );
	PosPool->SizeInfo.minZ			= min( PosPool->SizeInfo.minZ, sz.minZ );

	// Update the DOF count and return
	nDOFs = max( nDOFs, dofNum+1 );

	return node;
}


BNode* ProcessAllChildren(mgrec *rec)
{
	mgrec	*child;
	BNode	*masterNode = NULL;
	BNode	*node = NULL;

	ShiAssert( rec );

	child = mgGetChild( rec );
	while(child) {
		if (node) {
			// Get to the end of the existing sibling list.
			while (node->sibling) {
				node = node->sibling;
			}

			// Now handle any siblings at this level in the tree.
			node->sibling = ProcessRecord( child );
			if (node->sibling) {
				node = node->sibling;
			}
		} else {
			// This is the first node at this level in the tree.
			masterNode = node = ProcessRecord( child );
		}
		child = mgGetNext( child );
	}

	return masterNode;
}


BNode* ProcessRecord(mgrec *rec)
{
	// Handle our special nodes
	if (mgIsCode(rec, fltPolygon))		return ProcessPrimitive( rec );
	if (mgIsCode(rec, fltBsp))			return ProcessBsp( rec );
	if (mgIsCode(rec, fltSwitch))		return ProcessSwitch( rec );
	if (mgIsCode(rec, fltDof))			return ProcessDOF( rec );
	if (mgIsCode(rec, fltLightPoint))	return ProcessLightString( rec );

	// Not "special", so just look at its children
	return ProcessAllChildren( rec );
}


BRoot* ReadGeometryFlt(BuildTimeLODEntry *buildLODentry)
{
	int		i;
	BOOL	result;
	BRoot	*root;		// Top node in the BSP tree we build


	// open the named database file
	if (!(db = mgOpenDb(buildLODentry->filename))) {
		char msgbuf [1024];
		mgGetLastError(msgbuf, 1024);
		printf("%s\n", msgbuf);
		mgExit();
		return NULL;
	}

	// Initialize internal data structures
	TheAlphaPatchList.Setup();
	TheDynamicPatchList.Setup();
	PosPool					= new BuildTimePosList;
	NormalCnt				= 0;
	TexIDCnt				= 0;
	nSwitches				= 0;
	nDOFs					= 0;
	nSlots					= 0;
	nDynamicCoords			= 0;
	nTextureSets			= 0;
	ObjectFlags				= 0;
	ScriptNumber			= -1;
	Rotation				= IMatrix;
	Translation				= Origin;
	LocalCoords				= FALSE;
	PrelightColors			= FALSE;

	// Process this object's header comments looking for meaningful flags
	ProcessHeaderCommentFlags();

	// Process this object's texture palette
	ProcessTexturePalette();

	// Make sure we're starting in a good state
	ShiAssert( stackDepth == 0 );
	stackDepth = 0;

	// traverse simple hierarchy building our working tree
	root = new BRoot;
	result = ProcessSubTree(root, db, &PosPool->SizeInfo, FALSE);

	// close the database file
	mgCloseDb(db);

	// If we didn't get a proper build, return nothing...
	if (!result) {
		delete root;
		root = NULL;
		FLTwarning( db, "Failed to build a tree" );
		return NULL;
	}

	// Move the texture ID array into the root node
	root->nTexIDs	= TexIDCnt;
	if (TexIDCnt) {
		root->pTexIDs	= new int[TexIDCnt];
		memcpy( root->pTexIDs,  TexIDArray,    sizeof(*TexIDArray)*TexIDCnt );
	} else {
		root->pTexIDs	= NULL;
	}

	// Move the script number into the root node
	root->ScriptNumber = ScriptNumber;

	// Now put the accumulated data into the object's LOD record
	buildLODentry->nSwitches		= nSwitches;
	buildLODentry->nDOFs			= nDOFs;
	buildLODentry->nSlots			= nSlots;
	buildLODentry->nDynamicCoords	= nDynamicCoords;
	buildLODentry->nTextureSets		= nTextureSets;
	buildLODentry->flags			= ObjectFlags;
	buildLODentry->radius			= sqrt( PosPool->SizeInfo.radiusSquared );
	buildLODentry->maxX				= PosPool->SizeInfo.maxX;
	buildLODentry->maxY				= PosPool->SizeInfo.maxY;
	buildLODentry->maxZ				= PosPool->SizeInfo.maxZ;
	buildLODentry->minX				= PosPool->SizeInfo.minX;
	buildLODentry->minY				= PosPool->SizeInfo.minY;
	buildLODentry->minZ				= PosPool->SizeInfo.minZ;

	// Here we store the position of each slot and dynamic vertex for later storage in the parent data
	buildLODentry->pSlotAndDynamicPositions = new Ppoint[nSlots + nDynamicCoords];
	for (i=0; i<nSlots; i++) {
		buildLODentry->pSlotAndDynamicPositions[i] = SlotPositionArray[i];
	}
	for (i=0; i<nDynamicCoords; i++) {
		buildLODentry->pSlotAndDynamicPositions[nSlots+i] = DynamicPositionArray[i];
	}

	// Arbitrary limit for sanity checking...
	ShiAssert( buildLODentry->radius < 60000.0f );

#if 0
	char buffer[1024];
	sprintf( buffer, "Flat %0d  Gouraud %0d  Lit %0d  Lit Gouraud %0d", PolyTypeCount[0], PolyTypeCount[1], PolyTypeCount[2], PolyTypeCount[3]);
	MessageBox(NULL, buffer, buildLODentry->filename, MB_OK);
#endif

#if 0
	char buffer[1024];
	sprintf( buffer, "Pos %0d  Norm %0d", PositionCnt, NormalCnt);
	MessageBox(NULL, buffer, buildLODentry->filename, MB_OK);
#endif

	delete PosPool;
	TheAlphaPatchList.Cleanup();
	TheDynamicPatchList.Cleanup();

	return root;
}
