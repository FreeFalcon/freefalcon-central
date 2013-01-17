/***************************************************************************\
    PolyLib.h
    Scott Randolph
    February 9,1998

    //JAM 06Jan04 - Begin Major Rewrite
\***************************************************************************/
#ifndef _POLYLIB_H_
#define _POLYLIB_H_

#include "grtypes.h"
#include "context.h"

typedef enum PpolyType
{
	PointF = 0,
	LineF,
//	LtStr,

	F,
	FL,
	G,
	GL,
	Tex,
	TexL,
	TexG,
	TexGL,
	CTex,
	CTexL,
	CTexG,
	CTexGL,

	AF,
	AFL,
	AG,
	AGL,
	ATex,
	ATexL,
	ATexG,
	ATexGL,
	CATex,
	CATexL,
	CATexG,
	CATexGL,

	BAptTex,

	PpolyTypeNum
}
PpolyType;

typedef Trotation	Pmatrix;

typedef Tpoint		Ppoint;

// FIXME
typedef struct Spoint : public Ppoint
{
	float s;
}
Spoint;

typedef struct	Pcolor
{
	float r,g,b,a;
}
Pcolor;

typedef struct	Pnormal
{
	float i,j,k;
}
Pnormal;

typedef struct Ptexcoord
{
	float u,v;
}
Ptexcoord;

typedef float Pintensity;

typedef struct PclipInfo
{
	UInt32 clipFlag;		// Which edges this point is outside
	float csX,csY,csZ;		// Camera space coordinates of this point
}
PclipInfo;

// Polygon structures
typedef struct Prim
{
	PpolyType	type;
	int			nVerts;
	int			*xyz;		// Indexes XformedPosPool
}
Prim;

typedef struct PrimPointFC: public Prim
{
	int			rgba;		// Indexes ColorPool
}
PrimPointFC;

typedef struct PrimLineFC: public Prim
{
	int			rgba;		// Indexes ColorPool
}
PrimLineFC;

typedef struct PrimLtStr: public Prim
{
	int			rgba;		// Indexes ColorPool
	int			rgbaBack;	// Indexes ColorPool (-1 means omnidirectional -- could subclass instead)
	float		i,j,k;		// Direction vector (negative dot with eyepos means use back color)
}
PrimLtStr;

typedef struct Poly: public Prim
{
	float		A,B,C,D;	// Polygon plane equation for back face culling
}
Poly;

typedef struct PolyFC: public Poly
{
	int			rgba;		// Indexes ColorPool
}
PolyFC;

typedef struct PolyVC: public Poly
{
	int			*rgba;		// Indexes ColorPool
}
PolyVC;

typedef struct PolyFCN: public PolyFC
{
	int			I;			// Indexes IntensityPool
}
PolyFCN;

typedef struct PolyVCN: public PolyVC
{
	int			*I;			// Indexes IntensityPool
}
PolyVCN;

typedef struct PolyTexFC: public PolyFC
{
	int			texIndex;	// Indexes the local texture id table
	Ptexcoord	*uv;
}
PolyTexFC;

typedef struct PolyTexVC: public PolyVC
{
	int			texIndex;	// Indexes the local texture id table
	Ptexcoord	*uv;
}
PolyTexVC;

typedef struct PolyTexFCN: public PolyFCN
{
	int			texIndex;	// Indexes the local texture id table
	Ptexcoord	*uv;
}
PolyTexFCN;

typedef struct PolyTexVCN: public PolyVCN
{
	int			texIndex;	// Indexes the local texture id table
	Ptexcoord	*uv;
}
PolyTexVCN;

// Polygon render state tables
extern const int *RenderStateTable;
extern const int *RenderStateTablePC;
extern const int *RenderStateTableNPC;

extern const int RenderStateTableNoTex[];
extern const int RenderStateTableWithPCTex[];
extern const int RenderStateTableWithNPCTex[];

// Jump table for polygon draw functions
typedef void (*DrawPrimFp)(Prim *prim);
typedef void (*ClipPrimFp)(Prim *prim,UInt32 clipFlag);

extern const DrawPrimFp *DrawPrimJumpTable;

extern const DrawPrimFp *DrawPrimNoClipJumpTable;
extern const ClipPrimFp *ClipPrimJumpTable;

extern const DrawPrimFp *DrawPrimNoFogNoClipJumpTable;
extern const ClipPrimFp *ClipPrimNoFogJumpTable;
extern const DrawPrimFp *DrawPrimFogNoClipJumpTable;
extern const ClipPrimFp *ClipPrimFogJumpTable;

extern const DrawPrimFp DrawPrimWithClipJumpTable[];
extern const DrawPrimFp DrawPrimNoClipWithTexJumpTable[];
extern const DrawPrimFp DrawPrimNoClipNoTexJumpTable[];
extern const DrawPrimFp DrawPrimFogNoClipWithTexJumpTable[];
extern const DrawPrimFp DrawPrimFogNoClipNoTexJumpTable[];
extern const ClipPrimFp ClipPrimWithTexJumpTable[];
extern const ClipPrimFp ClipPrimNoTexJumpTable[];
extern const ClipPrimFp ClipPrimFogWithTexJumpTable[];
extern const ClipPrimFp ClipPrimFogNoTexJumpTable[];

// Polygon draw functions (no clipping)
void DrawPrimPoint(	PrimPointFC		*point);
void DrawPrimLine(	PrimLineFC		*line);
void DrawPrimLtStr(	PrimLtStr		*ltstr);
void DrawPoly(		PolyFC			*poly);
void DrawPolyL(		PolyFCN			*poly);
void DrawPolyG(		PolyVC			*poly);
void DrawPolyGL(	PolyVCN			*poly);
void DrawPolyT(		PolyTexFC		*poly);
void DrawPolyTL(	PolyTexFCN		*poly);
void DrawPolyTG(	PolyTexVC		*poly);
void DrawPolyTGL(	PolyTexVCN		*poly);

void DrawPrimFPoint(PrimPointFC		*point);
void DrawPrimFLine(	PrimLineFC		*line);
void DrawPolyF(		PolyFC			*poly);
void DrawPolyFL(	PolyFCN			*poly);
void DrawPolyFG(	PolyVC			*poly);
void DrawPolyFGL(	PolyVCN			*poly);
void DrawPolyFT(	PolyTexFC		*poly);
void DrawPolyFTL(	PolyTexFCN		*poly);
void DrawPolyFTG(	PolyTexVC		*poly);
void DrawPolyFTGL(	PolyTexVCN		*poly);

void DrawPolyAT(	PolyTexFC		*poly);
void DrawPolyATL(	PolyTexFCN		*poly);
void DrawPolyATG(	PolyTexVC		*poly);
void DrawPolyATGL(	PolyTexVCN		*poly);

void DrawPolyFAT(	PolyTexFC		*poly);
void DrawPolyFATL(	PolyTexFCN		*poly);
void DrawPolyFATG(	PolyTexVC		*poly);
void DrawPolyFATGL(	PolyTexVCN		*poly);

// Polygon clip functions
void DrawClippedPrim(Prim *prim);

void ClipPrimPoint(	PrimPointFC		*point,	UInt32 clipFlag);
void ClipPrimLine(	PrimLineFC		*line,	UInt32 clipFlag);
void ClipPrimLtStr(	PrimLtStr		*ltstr,	UInt32 clipFlag);
void ClipPoly(		PolyFC			*poly,	UInt32 clipFlag);
void ClipPolyL(		PolyFCN			*poly,	UInt32 clipFlag);
void ClipPolyG(		PolyVC			*poly,	UInt32 clipFlag);
void ClipPolyGL(	PolyVCN			*poly,	UInt32 clipFlag);
void ClipPolyT(		PolyTexFC		*poly,	UInt32 clipFlag);
void ClipPolyTL(	PolyTexFCN		*poly,	UInt32 clipFlag);
void ClipPolyTG(	PolyTexVC		*poly,	UInt32 clipFlag);
void ClipPolyTGL(	PolyTexVCN		*poly,	UInt32 clipFlag);

void ClipPrimFPoint(PrimPointFC		*point,	UInt32 clipFlag);
void ClipPrimFLine(	PrimLineFC		*line,	UInt32 clipFlag);
void ClipPolyF(		PolyFC			*poly,	UInt32 clipFlag);
void ClipPolyFL(	PolyFCN			*poly,	UInt32 clipFlag);
void ClipPolyFG(	PolyVC			*poly,	UInt32 clipFlag);
void ClipPolyFGL(	PolyVCN			*poly,	UInt32 clipFlag);
void ClipPolyFT(	PolyTexFC		*poly,	UInt32 clipFlag);
void ClipPolyFTL(	PolyTexFCN		*poly,	UInt32 clipFlag);
void ClipPolyFTG(	PolyTexVC		*poly,	UInt32 clipFlag);
void ClipPolyFTGL(	PolyTexVCN		*poly,	UInt32 clipFlag);

void ClipPolyAT(	PolyTexFC		*poly,	UInt32 clipFlag);
void ClipPolyATL(	PolyTexFCN		*poly,	UInt32 clipFlag);
void ClipPolyATG(	PolyTexVC		*poly,	UInt32 clipFlag);
void ClipPolyATGL(	PolyTexVCN		*poly,	UInt32 clipFlag);

void ClipPolyFAT(	PolyTexFC		*poly,	UInt32 clipFlag);
void ClipPolyFATL(	PolyTexFCN		*poly,	UInt32 clipFlag);
void ClipPolyFATG(	PolyTexVC		*poly,	UInt32 clipFlag);
void ClipPolyFATGL(	PolyTexVCN		*poly,	UInt32 clipFlag);

// Polygon decode functions
Prim	*RestorePrimPointers(BYTE *baseAddress,int offset);

void	RestorePrimPointFC(PrimPointFC *prim,BYTE *baseAddress);
void	RestorePrimLineFC(PrimLineFC *prim,BYTE *baseAddress);
void	RestorePrimLtStr(PrimLtStr *prim,BYTE *baseAddress);
void	RestorePolyFC(PolyFC *prim,BYTE *baseAddress);
void	RestorePolyVC(PolyVC *prim,BYTE *baseAddress);
void	RestorePolyFCN(PolyFCN *prim,BYTE *baseAddress);
void	RestorePolyVCN(PolyVCN *prim,BYTE *baseAddress);
void	RestorePolyTexFC(PolyTexFC *prim,BYTE *baseAddress);
void	RestorePolyTexVC(PolyTexVC *prim,BYTE *baseAddress);
void	RestorePolyTexFCN(PolyTexFCN *prim,BYTE *baseAddress);
void	RestorePolyTexVCN(PolyTexVCN *prim,BYTE *baseAddress);

#endif // _POLYLIB_H_