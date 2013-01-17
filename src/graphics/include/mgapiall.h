#ifndef MGIAPIALL_H
#define MGIAPIALL_H
/*
 * Attempt to emulate Multigen API
 */

#include <stdio.h>

enum OpCode {
	OPCODE_HEADER = 1,
	OPCODE_GROUP = 2,

	OPCODE_OBJECT = 4,
	OPCODE_POLYGON = 5,

	OPCODE_PUSH_LEVEL = 10,
	OPCODE_POP_LEVEL = 11,

	OPCODE_DEGREE_OF_FREEDOM = 14,

	OPCODE_PUSH_SUBFACE = 19,
	OPCODE_POP_SUBFACE = 20,

	OPCODE_TEXT_COMMENT = 31,
	OPCODE_COLOR_TABLE = 32,
	OPCODE_LONG_IDENTIFIER = 33,

	OPCODE_TRANSFORMATION_MATRIX = 49,
	OPCODE_VECTOR = 50,

	OPCODE_BINARY_SEPARATING_PLANE = 55,

	OPCODE_REPLICATE_CODE = 60,
	OPCODE_LOCAL_INSTANCE = 61,
	OPCODE_LOCAL_INSTANCE_LIBRARY = 62,
	OPCODE_EXTERNAL_REFERENCE = 63,
	OPCODE_TEXTURE_REFERENCE_RECORD = 64,

// unused for version > = 1500,
	OPCODE_MATERIAL_TABLE = 66,

	OPCODE_SHARED_VERTEX_TABLE = 67,
// new name for version > 1500
	OPCODE_VERTEX_PALETTE = 67,

	OPCODE_VERTEX_COORDINATE = 68,
	OPCODE_VERTEX_WITH_NORMAL = 69,
	OPCODE_VERTEX_WITH_NORMAL_AND_UV = 70,
	OPCODE_VERTEX_WITH_UV = 71,
	OPCODE_VERTEX_LIST = 72,
	OPCODE_LEVEL_OF_DETAIL = 73,
	OPCODE_BOUNDING_BOX = 74,

	OPCODE_ROTATE_ABOUT_EDGE_TRANSFORM = 76,

// unused for version > 1500
	OPCODE_SCALE_TRANSFORM = 77,

	OPCODE_TRANSLATE_TRANSFORM = 78,
	OPCODE_SCALE_WITH_INDEPENDENT_XYZ_SCALE = 79,
	OPCODE_ROTATE_ABOUT_POINT_TRANSFORM = 80,
	OPCODE_ROTATE_AND_OR_SCALE_TRANSFORM = 81,
	OPCODE_PUT_TRANSFORM = 82,
	OPCODE_EYEPOINT_AND_TRACKPLANE_POSITION = 83,
	OPCODE_ROAD_SEGMENT = 87,
	OPCODE_ROAD_ZONE = 88,
	OPCODE_MORPHING_VERTEX_LIST = 89,
	OPCODE_LINKAGE_RECORD = 90,
	OPCODE_SOUND_BEAD = 91,
	OPCODE_ROAD_PATH_BEAD = 92,
	OPCODE_SOUND_PALETTE = 93,
	OPCODE_GENERAL_MATRIX_TRANSFORM = 94,
	OPCODE_TEXT_BEAD = 95,
	OPCODE_SWITCH_BEAD = 96,
	OPCODE_LINE_STYLE_RECORD = 97,
	OPCODE_CLIPPING_QUADRILATERAL_BEAD = 98,
	OPCODE_EXTENSION = 100,
	OPCODE_LIGHT_SOURCE_RECORD = 101,
	OPCODE_LIGHT_SOURCE_PALETTE = 102,

// unused for version > = 1500,
	OPCODE_DELAUNAY_HEADER = 103,
	OPCODE_DELAUNAY_POINTS = 104,

	OPCODE_BOUNDING_SPHERE = 105,
	OPCODE_BOUNDING_CYLINDER = 106,
	OPCODE_BOUNDING_VOLUME_CENTER = 108,
	OPCODE_BOUNDING_VOLUME_ORIENTATION = 109,

	OPCODE_TEXTURE_MAPPING_PALETTE = 112,
	OPCODE_MATERIAL_PALETTE = 113,
	OPCODE_COLOR_NAME_PALETTE = 114,
	OPCODE_CONTINUOUSLY_ADAPTIVE_TERRAIN = 115,
	OPCODE_CAT_DATA = 116,

	OPCODE_PUSH_ATTRIBUTE = 122,
	OPCODE_POP_ATTRIBUTE = 123,
};


#define MMSG_ERROR 1
#define MMSG_WARNING 2
#define MMSG_STATUS 3

#define MG_TRUE 1
#define MG_FALSE 0

#define MG_NULL 0

typedef struct mgrec {
    int len;
    OpCode type;
    int vrsn;
    int xdata;
    char *data;
    struct mgrec *next;
    struct mgrec *child;
    struct mgrec *parent;
} mgrec;
typedef float mgmatrix[12];

enum FltTypes { 
    fltHeader = 1,
	fltGroup,
    fltIcoord, 
	fltVU, fltVV, 
	fltVertex, fltPolyMaterial,
	fltDiffuse, fltMatAlpha,
	fltPolygon, fltBsp, fltSwitch, fltDof, fltLightPoint,
	fltPolyTransparency, fltGcLightMode, fltPolyTexture, 
	fltMatrix, fltVColor, fltLpDirectionalityType,
	fltLpBackColor,  fltPolyMgTemplate, fltPolyLineStyle,
	fltPolyDrawType,  fltDPlaneA, fltDPlaneB, fltDPlaneC, fltDPlaneD,
	fltDofPutAnchorX, fltDofPutAnchorY, fltDofPutAnchorZ,
	fltDofPutAlignX, fltDofPutAlignY, fltDofPutAlignZ,
	fltDofPutTrackX, fltDofPutTrackY, fltDofPutTrackZ,
	fltDofMaxX, fltDofMinX, fltXref, fltLodSwitchIn, fltXrefFilename, fltLod};

extern char *mgGetName (mgrec *rec);
extern char *mgRec2Filename(mgrec *rec);
extern void mgGetLastError (char *buf, int size);
extern void mgFree (char *);
extern void mgInit (int, void *);
extern mgrec *mgOpenDb (char *filename);
extern void mgCloseDb (mgrec *db);
extern void mgSetMessagesEnabled (int, int);
extern void mgExit (void);
extern char *mgGetComment (mgrec *db);
extern int mgGetFirstTexture (mgrec *db, int *texind, char texname[]);
extern int mgGetNextTexture (mgrec *db, int *texind, char texname[]);
extern int mgGetIcoord (mgrec *rec, FltTypes type, double *x, double *y, double *z);
extern int mgGetVtxColorRGB (mgrec *rec, short *r, short *g, short *b);
extern int mgGetVtxNormal (mgrec *rec, float *i, float *j, float *k);
extern int mgGetAttList (mgrec *rec, ...);
extern mgrec *mgGetChild (mgrec *parent);
extern mgrec *mgGetNext (mgrec *base);
extern mgrec *mgGetParent (mgrec *child);
extern int mgIsCode (mgrec *rec, FltTypes types);
extern mgrec *mgGetMaterial(mgrec *db, int matind);
extern int mgGetNormColor (mgrec *matRec, FltTypes types, float *r, float *g, float *b );
extern int mgGetPolyColorRGB(mgrec *rec, short *r, short *g, short *b);
extern int mgGetPolyNormal (mgrec *rec, double *i, double *j, double *k);
extern int mgGetMatrix(mgrec *rec, FltTypes type, mgmatrix *matrix );
extern int mgIndex2RGB(mgrec *rec, int colind, float inten, short *r, short *g, short *b);


#pragma pack (push, 1)

// version 1500

struct flt_HeaderRecord {
	short 	Opcode;
	short	recordLen;
	char	IDField[8];
	int		formatRev;
	int		DBRev;
	char	dateRev[32];
	short	nextGroupID;
	short	nextLODID;
	short	nextObjectID;
	short 	nextPolyID;
	short	unitFactor;
	char	vertexUnit;
	char	texflag;
	int		flags;
	int		notused0[6];
	int		projectionType;
	int		notused1[7];
	short	nextDOFID;
	short	vertexStorageType;
	int		databaseOrigin;
	double	swDBcoordx;
	double	swDBcoordy;
	double	deltax;
	double	deltay;
	short	nextSoundBeadID;
	short	nextPathBeadID;
	int		reserved0[2];
	short	nextClippingRegionBeadID;
	short	nextTextBeadID;
	short	nextBSPID;
	short	nextSwitchBeadID;
	int		reserved1;
	double	swCornerLatitude;	
	double	swCornerLongitude;	
	double	neCornerLatitude;	
	double	neCornerLongitude;	
	double	originalLatitude;	
	double	originalLongitude;	
	double	lambertUpperLatitude;	
	double	lambertLowerLatitude;
	short	nextLightSourceID;
	short	reserved2;
	int		reserved3;
};

struct flt_GroupRecord {
	short 	Opcode;
	short	recordLen;
	char	IDField[8];
	short 	relativePriority;
	short	spareAlignment;
	int		Flags;
	short	sfx1, sfx2;
	char	layerNumber;
	char	reserved0;	
	int		reserved1;
};

struct flt_ObjectRecord {
	short 	Opcode;
	short	recordLen;
	char	IDField[8];
	int		Flags;
	short	relativePriority;
	short	transparencyFactor;
	short	sfx1, sfx2;
	short	significance;
	short	spare;
};

struct flt_BinarySeparatingPlane {
	short 	Opcode;
	short 	recordLen;
	char	IDField[8];
	int		reserved;
	double	a, b, c, d;	
};

struct flt_SharedVertex {
	short 	Opcode;
	short	recordLen;
	int		totalLen;
};

struct flt_VertexList {
	short 	Opcode;
	short	recordLen;
	int		offset[1];
};

struct flt_VertexCoordinate {
	short 	Opcode;
	short	recordLen;
	unsigned short 	vertexColor;
	unsigned short 	Flags;
	double	x, y, z;
	unsigned int		packedColor;
	int		reserved;
};

struct flt_VertexCoordinateTexture {
	short 	Opcode;
	short	recordLen;
	unsigned short 	vertexColor;
	unsigned short 	Flags;
	double	x, y, z;
	float	u, v;
	unsigned int		packedColor;
	int		reserved;
};

struct flt_VertexCoordinateNormal {
	short 	Opcode;
	short	recordLen;
	unsigned short 	vertexColor;
	unsigned short 	Flags;
	double	x, y, z;
	float	nx, ny, nz;
	unsigned int		packedColor;
};

struct flt_VertexCoordinateTextureNormal {
	short 	Opcode;
	short	recordLen;
	unsigned short 	vertexColor;
	unsigned short 	Flags;
	double	x, y, z;
	float	nx, ny, nz;
	float	u, v;
	unsigned int		packedColor;
};

struct flt_PolygonRecord {
	short 	Opcode;
	short	recordLen;
	char	IDField[8];
	int		colorCode;
	short	relativePriority;
	char	howToDraw;
	char	texWhite;
	unsigned short	primaryColor;
	unsigned short	secondaryColor;
	char	notused;
	char	templateTransparency;
	short	detailTextureNo;
	short	textureNo;
	short	materialCode;
	short	surfaceCode;
	short	featureID;
	int		IRMaterialCode;
	short	transparency;
	char	influences;
	char	linestyle;
	unsigned int		Flags;
	char	lightMode;
	char	reserved0;
	char	reserved1[6];
	unsigned int		packedPrimaryColor;
	unsigned int		packedSecondaryColor;
};

struct flt_ColorRecord {
	short 	Opcode;
	short	recordLen;
	unsigned char	reserved[128];
	int		rgb[512];	
};

struct flt_MaterialTable {
	float	ambientRed, ambientGreen, ambientBlue;
	float	diffuseRed, diffuseGreen, diffuseBlue;
	float	specularRed, specularGreen, specularBlue;
	float	emissiveRed, emissiveGreen, emissiveBlue;
	float	shininess, alpha;
	int		Flags;
	char	name[12];
	int		spares[28];
};

struct flt_MaterialRecord {
	short 	Opcode;
	short	recordLen;
	flt_MaterialTable	mat[64];
};

struct flt_LongIDRecord {
	short 	Opcode;
	short	recordLen;
	char	id[1];
};

struct flt_TexturePatternRecord {
	short 	Opcode;
	short	recordLen;
	char	filename[200];
	int		patternIndex;
	int		xloc, yloc;
};

struct flt_DegreeOfFreedomRecord {
	short 	Opcode;
	short	recordLen;
	char	IDField[8];
	int		reserved;
	double	originx, originy, originz;
	double	pointxaxis_x, pointxaxis_y, pointxaxis_z;
	double	pointxyplane_x, pointxyplane_y, pointxyplane_z;
	double	minz, maxz, currentz, incrementz;
	double	miny, maxy, currenty, incrementy;
	double	minx, maxx, currentx, incrementx;
	double	minpitch, maxpitch, currentpitch, incrementpitch;
	double	minroll, maxroll, currentroll, incrementroll;
	double	minyaw, maxyaw, currentyaw, incrementyaw;
	double	minzscale, maxzscale, currentzscale, incrementzscale;
	double	minyscale, maxyscale, currentyscale, incrementyscale;
	double	minxscale, maxxscale, currentxscale, incrementxscale;
};

struct flt_SwitchRecord {
	short 	Opcode;
	short	recordLen;
	char	IDField[8];
	int		CurrentMaskIndex;
	int		NumberWordPerMask;
	int		NumberMask;
	int		MaskList[1];
};

struct flt_BoundingBoxRecord {
	short 	Opcode;
	short	recordLen;
	int		reserved;
	double	minx, miny, minz;
	double	maxx, maxy, maxz;
};

struct flt_BoundingSphereRecord {
	short 	Opcode;
	short	recordLen;
	int		reserved;
	double	radius;
};

struct flt_LODRecord {
	short 	Opcode;
	short	recordLen;
	char	IDField[8];
	int		Spare;
	double	switchin, switchout;
	short	fxid1, fxid2;
	int		Flags;
	double	centerx, centery, centerz;
	double	morphrange;
};

struct flt_ExternalReferenceRecord {
	short 	Opcode;
	short	recordLen;
	char	filename[200];
	char	reserved[2];
	short	padding;
	int		Flags;
};

struct flt_PutRecord {
	short 	Opcode;
	short	recordLen;
	int		temp;
	double	fromoriginx, fromoriginy, fromoriginz;
	double	fromalignx, fromaligny, fromalignz;
	double	fromtrackx, fromtracky, fromtrackz;
	double	toalignx, toaligny, toalignz;
	double	totrackx, totracky, totrackz;
	double	temp1, temp2, temp3;
};

struct flt_TranslateRecord {
	short 	Opcode;
	short	recordLen;
	int		temp;
	double	refx, refy, refz;
	double	deltax, deltay, deltaz;
};

struct flt_RotatePointRecord {
	short 	Opcode;
	short	recordLen;
	int		temp;
	double	centerx, centery, centerz;
	float	axisi, axisj, axisk;
	float	angle;
};

struct flt_CommentRecord {
	short 	Opcode;
	short	recordLen;
	char	id[1];
};

struct flt_TransformationMatrixRecord {
	short 	Opcode;
	short	recordLen;
	float	matrix[16];
};

#pragma pack (pop)

#pragma pack (push, 1)

//---------------------------------------
// version > 1500

struct aflt_HeaderRecord {
	short 	Opcode;
	short	recordLen;
	char	IDField[8];
	int		formatRev;
	int		DBRev;
	char	dateRev[32];
	short	nextGroupID;
	short	nextLODID;
	short	nextObjectID;
	short 	nextPolyID;
	short	unitFactor;
	char	vertexUnit;
	char	texflag;
	int		flags;
	int		notused0[6];
	int		projectionType;
	int		notused1[7];
	short	nextDOFID;
	short	vertexStorageType;
	int		databaseOrigin;
	double	swDBcoordx;
	double	swDBcoordy;
	double	deltax;
	double	deltay;
	short	nextSoundBeadID;
	short	nextPathBeadID;
	int		reserved0[2];
	short	nextClippingRegionBeadID;
	short	nextTextBeadID;
	short	nextBSPID;
	short	nextSwitchBeadID;
	int		reserved1;
	double	swCornerLatitude;	
	double	swCornerLongitude;	
	double	neCornerLatitude;	
	double	neCornerLongitude;	
	double	originalLatitude;	
	double	originalLongitude;	
	double	lambertUpperLatitude;	
	double	lambertLowerLatitude;
	short	nextLightSourceID;
	short	reserved2;
	short	nextRoadBeadIDNumber;
	short	nextCATBeadIDNumber;
	short	reserved3[4];
	int		EarthEllipsoidModel;
};

struct aflt_GroupRecord {
	short 	Opcode;
	short	recordLen;
	char	IDField[8];
	short 	relativePriority;
	short	spareAlignment;
	int		Flags;
	short	sfx1, sfx2;
	short	significance;
	char	layerCode;
	char	reserved0;
	int		reserved1;
};

struct aflt_ObjectRecord {
	short 	Opcode;
	short	recordLen;
	char	IDField[8];
	int		Flags;
	short	relativePriority;
	short	transparencyFactor;
	short	sfx1, sfx2;
	short	significance;
	short	spare;
};

struct aflt_PolygonRecord {
	short 	Opcode;
	short	recordLen;
	char	IDField[8];
	int		colorCode;
	short	relativePriority;
	char	howToDraw;
	char	texWhite;
	unsigned short	primaryColorName;
	unsigned short	secondaryColorName;
	char	notused;
	char	templateTransparency;
	short	detailTextureNo;
	short	textureNo;
	short	materialCode;
	short	surfaceCode;
	short	featureID;
	int		IRMaterialCode;
	short	transparency;
	char	influences;
	char	linestyle;
	unsigned int		Flags;
	char	lightMode;
	char	reserved0;
	char	reserved1[4];
	unsigned int		packedPrimaryColor;
	unsigned int		packedSecondaryColor;
	short	texturemappingindex;
	short	reserved2[2];
	unsigned int		primaryColor, secondaryColor;
};

struct aflt_DegreeOfFreedomRecord {
	short 	Opcode;
	short	recordLen;
	char	IDField[8];
	int		reserved;
	double	originx, originy, originz;
	double	pointxaxis_x, pointxaxis_y, pointxaxis_z;
	double	pointxyplane_x, pointxyplane_y, pointxyplane_z;
	double	minz, maxz, currentz, incrementz;
	double	miny, maxy, currenty, incrementy;
	double	minx, maxx, currentx, incrementx;
	double	minpitch, maxpitch, currentpitch, incrementpitch;
	double	minroll, maxroll, currentroll, incrementroll;
	double	minyaw, maxyaw, currentyaw, incrementyaw;
	double	minzscale, maxzscale, currentzscale, incrementzscale;
	double	minyscale, maxyscale, currentyscale, incrementyscale;
	double	minxscale, maxxscale, currentxscale, incrementxscale;
	int		Flags;
};

struct aflt_VertexList {
	short 	Opcode;
	short	recordLen;
	int		offset[1];
};

struct aflt_BinarySeparatingPlane {
	short 	Opcode;
	short 	recordLen;
	char	IDField[8];
	int		reserved;
	double	a, b, c, d;	
};

struct aflt_ExternalReferenceRecord {
	short 	Opcode;
	short	recordLen;
	char	filename[200];
	char	reserved[2];
	int		Flags;
	short	reserved1;
};

struct aflt_LODRecord {
	short 	Opcode;
	short	recordLen;
	char	IDField[8];
	int		Spare;
	double	switchin, switchout;
	short	fxid1, fxid2;
	int		Flags;
	double	centerx, centery, centerz;
	double	morphrange;
};

struct aflt_SwitchRecord {
	short 	Opcode;
	short	recordLen;
	char	IDField[8];
	int		CurrentMaskIndex;
	int		NumberWordPerMask;
	int		NumberMask;
	int		MaskList[1];
};

struct aflt_LongIDRecord {
	short 	Opcode;
	short	recordLen;
	char	id[1];
};

struct aflt_BoundingBoxRecord {
	short 	Opcode;
	short	recordLen;
	int		reserved;
	double	minx, miny, minz;
	double	maxx, maxy, maxz;
};

struct aflt_BoundingSphereRecord {
	short 	Opcode;
	short	recordLen;
	int		reserved;
	double	radius;
};

struct aflt_SharedVertex {	// Gamegen's new name : Vertex Palette
	short 	Opcode;
	short	recordLen;
	int		totalLen;
};

struct aflt_VertexCoordinate {
	short 	Opcode;
	short	recordLen;
	unsigned short 	vertexColorName;
	unsigned short 	Flags;
	double	x, y, z;
	unsigned int	packedColor;
	unsigned int 	vertexColor;
};

struct aflt_VertexCoordinateTexture {
	short 	Opcode;
	short	recordLen;
	unsigned short 	vertexColorName;
	unsigned short 	Flags;
	double	x, y, z;
	float	u, v;
	unsigned int	packedColor;
	unsigned int 	vertexColor;
};

struct aflt_VertexCoordinateNormal {
	short 	Opcode;
	short	recordLen;
	unsigned short 	vertexColorName;
	unsigned short 	Flags;
	double	x, y, z;
	float	nx, ny, nz;
	unsigned int	packedColor;
	unsigned int 	vertexColor;
};

struct aflt_VertexCoordinateTextureNormal {
	short 	Opcode;
	short	recordLen;
	unsigned short 	vertexColorName;
	unsigned short 	Flags;
	double	x, y, z;
	float	nx, ny, nz;
	float	u, v;
	unsigned int	packedColor;
	unsigned int 	vertexColor;
};

struct aflt_ColorRecord {
	short 	Opcode;
	short	recordLen;
	unsigned char	reserved[128];
	int		rgb[1024];
	int		totalColorName;
};

struct aflt_ColorNameRecord {
	short	length;
	short	reserved0;
	short	colorIndex;
	char	colorName[1];
};

struct aflt_MaterialRecord {
	short 	Opcode;
	short	recordLen;
	int		materialIndex;
	char	name[12];
	int		Flags;
	float	ambientRed, ambientGreen, ambientBlue;
	float	diffuseRed, diffuseGreen, diffuseBlue;
	float	specularRed, specularGreen, specularBlue;
	float	emissiveRed, emissiveGreen, emissiveBlue;
	float	shininess, alpha;
	int		spares;
};

struct aflt_TexturePatternRecord {
	short 	Opcode;
	short	recordLen;
	char	filename[200];
	int		patternIndex;
	int		xloc, yloc;
};

struct aflt_PutRecord {
	short 	Opcode;
	short	recordLen;
	int		temp;
	double	fromoriginx, fromoriginy, fromoriginz;
	double	fromalignx, fromaligny, fromalignz;
	double	fromtrackx, fromtracky, fromtrackz;
	double	toalignx, toaligny, toalignz;
	double	totrackx, totracky, totrackz;
	double	temp1, temp2, temp3;
};

struct aflt_TranslateRecord {
	short 	Opcode;
	short	recordLen;
	int		temp;
	double	refx, refy, refz;
	double	deltax, deltay, deltaz;
};

struct aflt_RotatePointRecord {
	short 	Opcode;
	short	recordLen;
	int		temp;
	double	centerx, centery, centerz;
	float	axisi, axisj, axisk;
	float	angle;
};

struct aflt_CommentRecord {
	short 	Opcode;
	short	recordLen;
	char	id[1];
};

struct aflt_TransformationMatrixRecord {
	short 	Opcode;
	short	recordLen;
	float	matrix[16];
};

#pragma pack (pop)

#endif