#ifndef DXDEFINES_H
#define DXDEFINES_H

//*******************************************************************************************************
// DX Format Encoding functions
// 2005 [R]ed
//*******************************************************************************************************

//#pragma once
#include "../../FastMath.h"
#include <d3dxmath.h>


// ***************************** MODELS RELEASE VERSION *********************
#define	MODEL_VERSION	0x0002



//******************************* MACROS ************************************
//#define	F_I32				(DWORD)
#define	F_TO_R(r)			((F_I32( r * 255.9f))<<16)
#define	F_TO_A(a)			((F_I32( a * 255.9f))<<24)
#define	F_TO_G(g)			((F_I32( g * 255.9f))<<8)
#define	F_TO_B(b)			((F_I32( b * 255.9f)))
#define	F_TO_RGB(r,g,b)		( F_TO_R(r) | F_TO_G(g) | F_TO_B(b))
#define	F_TO_ARGB(a,r,g,b)	( F_TO_A(a) | F_TO_RGB(r,g,b))

#define	F_TO_UR(r)			((F_I32( r))<<16)
#define	F_TO_UA(a)			((F_I32( a))<<24)
#define	F_TO_UG(g)			((F_I32( g))<<8)
#define	F_TO_UB(b)			((F_I32( b)))
#define	F_TO_URGB(r,g,b)	( F_TO_UR(r) | F_TO_UG(g) | F_TO_UB(b))
#define	F_TO_UARGB(a,r,g,b)	( F_TO_UA(a) | F_TO_URGB(r,g,b))








//***************************************************************************
// The XMM Support Header
#include "xmmintrin.h"

#define	OBJECTS_HEADER_NAME		"KoreaObj.Dxh"
#define	OBJECTS_DATABASE_NAME	"KoreaObj.Dxl"

#define XDOF_NEGATE (1<<0)
#define XDOF_MINMAX (1<<1)
#define XDOF_SUBRANGE (1<<2)
#define XDOF_ISDOF   (1<<31)

#define	RENDER_STATE_FLAGS	8						// RENDER STATE FLAGS Allocated in the following structure
#define	RENDER_USED_FLAGS	5						// RENDER STATE FLAGS effectively used
#define	MAX_RENDER_STATES	(1<<RENDER_USED_FLAGS)	// Max States

#define	MAX_SCRIPTS_X_MODEL	2						// number of scripts available for a model

typedef	enum { ROOT=0, VERTEX, DOT, LINE, TRIANGLE, POLY, DOF, CLOSEDOF, SLOT, MODELEND } NodeType;
typedef	enum { NO_DOF=0, ROTATE, XROTATE, TRANSLATE, SCALE, SWITCH, XSWITCH } DofType;
typedef	enum { STENCIL_OFF=0, STENCIL_ON, STENCIL_WRITE, STENCIL_CHECK } StencilModeType;

typedef	union{
			struct	{	
	
				// ****** This are the RENDER FLAGS, must be in a BYTE, update StateFlags if not ********
				DWORD	Alpha		:1;
				DWORD	Lite		:1;
				DWORD	ChromaKey	:1;
				DWORD	VColor		:1;
				DWORD	Texture		:1;
				DWORD	SwEmissive	:1;		
				DWORD	f1			:1;		// Spare RENDER FLAG, for future use
				DWORD	f2			:1;		// Spare RENDER FLAG, for future use
				//****************************************************************************************
				DWORD	zBias		:1;
				DWORD	BillBoard	:1;
				DWORD	Point		:1;
				DWORD	Line		:1;
				DWORD	Poly		:1;	
				DWORD	Gouraud		:1;		// Dbug Flags
				DWORD	Disable		:1;		// Debug Flag
				DWORD	Frame		:1;		// Debug Flag
				//****************************************************************************************
				DWORD	Spare		:8;		// Spare Bytes
				//****************************************************************************************
				DWORD	SWLightOwner:8;		// The Static Light Owner for this surface... ignored in runtime/used in editing
			} b;
			// *** WARNING *** THIS MUST BE THE BIT SIZE OF THE NUMEBR OF
			// FLAGS DEFINING THE RENDERING STATE !!!!!
			char	StateFlags;
			//***********************************************************
			DWORD	w;
		} DXFlagsType;


// File Header Structure
typedef	struct { 
					DWORD	dwModelSize;			// Model Size in Bytes
					DWORD	dwNodesNr;				// Total number of nodes in the Model
				} DxHeaderType;

// Structure defining a Vector item
typedef	struct	{ D3DVALUE	px, py, pz; } VectorsType;	

// Structure defining a Normal item
typedef	struct	{ D3DVALUE	nx, ny, nz; } NormalType;	

// Structure defining a Texture Coord Item
typedef	struct	{ D3DVALUE	tu, tv; } TextureType;	

// Structure defining a Diffuse Type
typedef	struct	{ union { DWORD dwDiffuse; char da, dr, dg, db; }; } ColourType;	

// Structure defining a Specular Type
typedef	struct	{ union { DWORD dwSpecular; char sa, sr, sg, sb; }; } SpecularType;	

// Type of Items available for drawing
typedef enum {	
				DX_ROOT=0,					// ROOT OF another model, not used usually
				DX_SURFACE,					// SURFACE Type
				DX_MATERIAL,				// MATERIAL Type
				DX_TEXTURE,					// TEXTURE Type
				DX_DOF,						// DOF Type
				DX_ENDDOF,					// DOF END
				DX_SLOT,					// SLOT Type
				DX_SWITCH,					// SWITCH Type
				DX_LIGHT,					// LIGHT Type
				DX_MODELEND
				} ItemType;

// The header of each node
typedef struct {	
					DWORD			dwNodeSize;
					DWORD			dwNodeID;
					ItemType		Type;
				} DXNodeHeadType;


//****************************** DX NODES STRUCTURES *********************************

// * SURFACE ITEM *
typedef struct {	
					DXNodeHeadType		h;						// NODE HEAD
					DXFlagsType			dwFlags;				// Node Flags from DXNodes
					DWORD				dwVCount;				// Vertex counting
					DWORD				dwStride;				// Vertex stride, size of each vertex in bytes
					D3DPRIMITIVETYPE	dwPrimType;				// DX Primitive Type
					DWORD				dwzBias;				// Surface zBias
					float				SpecularIndex;			// the Power for spcularity of the surface
					DWORD				TexID[2];				// Texture used by the Surface
					DWORD				SwitchNumber,SwitchMask;// the Switch Number for switcvhable emissive surfaces and its Mask
					DWORD				DefaultSpecularity;		// Switchable emissive surfaces, defines the specularity
				} DxSurfaceType;


//************************************************************************************

// * MATERIAL ITEM *
typedef	struct	{
					DXNodeHeadType		h;						// NODE HEAD
					D3DMATERIAL			m;						// DX MATERIAL STRUCTURE
				} DxMaterialType;


//************************************************************************************

// * TEXTURE ITEM *
typedef	struct	{
					DXNodeHeadType		h;						// NODE HEAD
					DWORD				Tex[4];					// TEXTURES
				}	DxTextureType;



//************************************************************************************

// * DOF ITEM *
typedef	struct	{
					DXNodeHeadType		h;						// NODE HEAD
					DWORD				dwDOFTotalSize;
					DofType				Type;
					union {				int	dofNumber;
										int SwitchNumber;
					};
					float				min,max,multiplier,future;
					union {				int  flags;
										int SwitchBranch;
					};
					Ppoint				scale;
					D3DXMATRIX			rotation;
					Ppoint				translation; 
				} DxDofType;
					
//************************************************************************************

// * DOF END ITEM *
typedef	struct	{
					DXNodeHeadType		h;						// NODE HEAD
				}DxDofEndType;

//************************************************************************************

// * SLOT ITEM *
typedef	struct	{	
					DXNodeHeadType		h;						// NODE HEAD
					DWORD				SlotNr;
					D3DXMATRIX			rotation;
				}DxSlotType;

//************************************************************************************

// * MODEL END ITEM *
typedef	struct	{
	DXNodeHeadType		h;						// NODE HEAD
}DxEndModelType;

// The scripts constants
// WARNING : Change of this must be done carefully, synced with scripts variables...
typedef	enum	{	SCRIPT_NONE=0, 
					SCRIPT_ANIMATE,
					SCRIPT_ROTATE,
					SCRIPT_HELY,
					SCRIPT_BEACON,
					SCRIPT_VASIF,
					SCRIPT_VASIN,
					SCRIPT_MEATBALL,
					SCRIPT_CHAFF,
					SCRIPT_CHUTEDIE,
} ScriptType;

// * MODEL SCRIPTS MANAGEMENT *
typedef	struct	{
	ScriptType		Script;
	DWORD			Arguments[3];
}	DXScriptVariableType;



// * DX Database Textures section *
typedef struct DxDbHeader{
		DWORD					Version;
		DWORD					Id;
		DWORD					VBClass;
		DWORD					ModelSize;
		DWORD					dwNVertices;
		DWORD					dwPoolSize;
		DWORD					pVPool;
		DWORD					dwNodesNr;
		DXScriptVariableType	Scripts[MAX_SCRIPTS_X_MODEL];
		DWORD					dwLightsNr;
		DWORD					pLightsPool;
		DWORD					dwTexNr;
}DxDbHeader;


typedef	union 	{
	struct	nameles	{
					DWORD	BillBoard	:	1;
				};
	DWORD		Word;
}	DrawBaseFlags;



// A data structure compatible with vectors and XMM math
typedef	union {	
				__m128	Xmm;
				struct {float x; float y; float z; DrawBaseFlags Flags; } d3d;
} XMMVector;

// A data structure compatible with vectors and XMM math
typedef	union {	
				__m128	Xmm;
				struct {float a; float r; float g; float b; } d3d;
} XMMColor;


inline DWORD XMM_ARGB(XMMColor *Source)
{
	DWORD	r;
	_asm{	
			push	eax
			push	edx
			mov		edx, DWORD PTR Source
			fld		[edx]
			fistp	r
			mov		eax,r
			sal		eax,8
			fld		[edx+4]
			fistp	r
			or		eax,r
			sal		eax,8
			fld		[edx+8]
			fistp	r
			or		eax,r
			sal		eax,8
			fld		[edx+16]
			fistp	r
			or		eax,r
			mov		r,eax
			pop		edx
			pop		eax
	}
	return	r;
}



//************************************************************************************
///////////////////////// LIGHT FEATURES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

typedef	union{
	struct 

	{
		DWORD		OwnLight		:1;
		DWORD		NotSelfLight	:1;
		DWORD		Static			:1;
		DWORD		DofDim			:1;
		DWORD		Random			:1;
	};
	WORD	w;
} DXLightFlagsType;

typedef	struct	{
	DWORD				Switch;				// The Light activation switch, -1 if always on
	DWORD				SwitchMask;			// The Light activation switch Mask, -1 if always valid
	DWORD				Argument;
	DXLightFlagsType	Flags;
	D3DLIGHT7			Light;				// The light
} DXLightType;






















//////////////////////////////// OLD STRUCTURES FORE RELEASE AND CHANGED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\


///////////////////////////////////////// RELEASE 001 \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

///////////////////////// LIGHT FEATURES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

typedef	struct	{
	DWORD		Switch;							// The Light activation switch, -1 if always on
	bool		OwnLight;
	D3DLIGHT7	Light;							// The light
} DXLightType_01;


/********************************************************************************************************/
/********************************************************************************************************/
/********************************************************************************************************/
/********************************************************************************************************/


///////////////////////////////////////// RELEASE 0000 BETA \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

// * DX Database Textures section *
typedef struct DxDbHeader_00{
/*		*** DWORD				Version; *** Added this field in Version 1.00 */
		DWORD					Id;
		DWORD					VBClass;
		DWORD					ModelSize;
		DWORD					dwNVertices;
		DWORD					dwPoolSize;
		DWORD					pVPool;
		DWORD					dwNodesNr;
		DXScriptVariableType	Scripts[MAX_SCRIPTS_X_MODEL];
		DWORD					dwLightsNr;
		DWORD					pLightsPool;
		DWORD					dwTexNr;
}DxDbHeader_00;


typedef struct {	
					DXNodeHeadType		h;						// NODE HEAD
					DXFlagsType			dwFlags;				// Node Flags from DXNodes
					DWORD				dwVCount;				// Vertex counting
					DWORD				dwStride;				// Vertex stride, size of each vertex in bytes
					D3DPRIMITIVETYPE	dwPrimType;				// DX Primitive Type
					DWORD				dwzBias;				// Surface zBias
					float				SpecularIndex;			// the Power for spcularity of the surface
					DWORD				TexID[2];				// Texture used by the Surface
					DWORD				SwitchNumber;			// the Switch Number for switcvhable emissive surfaces
					D3DCOLORVALUE		DefaultSpecularity;		// Switchable emissive surfaces, defines the specularity
				} DxSurfaceType_00;


#endif