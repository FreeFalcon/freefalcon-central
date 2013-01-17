#include <mgapiall.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <stddef.h>
#include <math.h>

static int lasterr = 0;

static void Normalize (double *xp, double *yp, double *zp)
{
        // normalize
    double x = *xp, y = *yp, z = *zp;
    double mag = sqrt(x*x + y*y + z*z);
    x /= mag;
    y /= mag;
    z /= mag;
    *xp = x;
    *yp = y;
    *zp = z;
}

static void freenode (mgrec *db)
{
    if (db->next) freenode (db->next);
    if (db->child) freenode (db->child);
    free (db);
}

inline void exch_char (char *a, char *b)
{
    *a ^= *b;
    *b ^= *a;
    *a ^= *b;
}

inline short swap_short (short num)
{
    short   data = num;
    char *cnum = (char *) &data;
    exch_char (&cnum[1], &cnum[0]);
    return data;
}

inline int swap_int (int num)
{
    int     data = num;
    char *cnum = (char *) &data;
    exch_char (&cnum[3], &cnum[0]);
    exch_char (&cnum[2], &cnum[1]);
    return data;
}
inline float swap_float (float num)
{
    float   data = num;
    char *cnum = (char *) &data;
    exch_char (&cnum[3], &cnum[0]);
    exch_char (&cnum[2], &cnum[1]);
    return data;
}

inline double swap_double (double num)
{
    double  data = num;
    char *cnum = (char *) &data;
    exch_char (&cnum[7], &cnum[0]);
    exch_char (&cnum[6], &cnum[1]);
    exch_char (&cnum[5], &cnum[2]);
    exch_char (&cnum[4], &cnum[3]);
    return data;
}

struct {
    FltTypes type;
    const char *name;
} FltNames[] = {
#define MKENT(x) { x, #x }
    MKENT(fltHeader),
	MKENT(fltGroup),
	MKENT(fltIcoord), 
	MKENT(fltVU),
	MKENT(fltVV), 
	MKENT(fltVertex), 
	MKENT(fltPolyMaterial),
	MKENT(fltDiffuse), 
	MKENT(fltMatAlpha),
	MKENT(fltPolygon), 
	MKENT(fltBsp), 
	MKENT(fltSwitch), 
	MKENT(fltDof), 
	MKENT(fltLightPoint),
	MKENT(fltPolyTransparency), 
	MKENT(fltGcLightMode), 
	MKENT(fltPolyTexture), 
	MKENT(fltMatrix), 
	MKENT(fltVColor), 
	MKENT(fltLpDirectionalityType),
	MKENT(fltLpBackColor),  
	MKENT(fltPolyMgTemplate), 
	MKENT(fltPolyLineStyle),
	MKENT(fltPolyDrawType),  
	MKENT(fltDPlaneA),
	MKENT(fltDPlaneB), 
	MKENT(fltDPlaneC), 
	MKENT(fltDPlaneD),
	MKENT(fltDofPutAnchorX), 
	MKENT(fltDofPutAnchorY), 
	MKENT(fltDofPutAnchorZ),
	MKENT(fltDofPutAlignX), 
	MKENT(fltDofPutAlignY), 
	MKENT(fltDofPutAlignZ),
	MKENT(fltDofPutTrackX), 
	MKENT(fltDofPutTrackY), 
	MKENT(fltDofPutTrackZ),
	MKENT(fltDofMaxX), 
	MKENT(fltDofMinX), 
	MKENT(fltXref), 
	MKENT(fltLodSwitchIn), 
	MKENT(fltXrefFilename), 
	MKENT(fltLod)
};
static const int FltSize = sizeof(FltNames) / sizeof(FltNames[0]);

static int mgIgnoreRec (OpCode type)
{
#if 1
    return 0;
#else
    switch (type) {
    case OPCODE_POLYGON:
    case OPCODE_BINARY_SEPARATING_PLANE:
    case OPCODE_SWITCH_BEAD:
    case OPCODE_DEGREE_OF_FREEDOM:
    case OPCODE_LIGHT_SOURCE_RECORD:
    case OPCODE_LEVEL_OF_DETAIL:
    case OPCODE_VERTEX_LIST:
    case OPCODE_COLOR_TABLE:
    case OPCODE_HEADER:
    case OPCODE_TEXT_COMMENT:
    case OPCODE_TEXTURE_REFERENCE_RECORD:
    case OPCODE_VERTEX_PALETTE:
    case OPCODE_VERTEX_WITH_NORMAL:
    case OPCODE_VERTEX_WITH_NORMAL_AND_UV:
    case OPCODE_VERTEX_WITH_UV:
    case OPCODE_MATERIAL_TABLE:
    case OPCODE_PUSH_LEVEL:
    case OPCODE_POP_LEVEL:
    case OPCODE_MATERIAL_PALETTE:
    case OPCODE_VERTEX_COORDINATE:
	return 0;
    default:
	return 1;
    }
#endif
}

static int mgIsAncillary (OpCode type)
{
    switch (type) {
    case OPCODE_TEXT_COMMENT:
    case OPCODE_LONG_IDENTIFIER:
    case OPCODE_REPLICATE_CODE:
    case OPCODE_ROAD_ZONE:
    case OPCODE_TRANSFORMATION_MATRIX:
    case OPCODE_VECTOR:
    case OPCODE_BOUNDING_BOX:
    case OPCODE_CAT_DATA:
    case OPCODE_EXTENSION:
    case OPCODE_VERTEX_COORDINATE:
    case OPCODE_VERTEX_WITH_NORMAL:
    case OPCODE_VERTEX_WITH_NORMAL_AND_UV:
    case OPCODE_VERTEX_WITH_UV:
	return 1;
    default:
	return 0;
    }
}

static int mgIsPalette (OpCode type)
{
    switch (type) {
    case OPCODE_VERTEX_PALETTE:
    case OPCODE_COLOR_TABLE:
    case OPCODE_COLOR_NAME_PALETTE:
    case OPCODE_MATERIAL_PALETTE:
    case OPCODE_TEXTURE_REFERENCE_RECORD:
    case OPCODE_EYEPOINT_AND_TRACKPLANE_POSITION:
    case OPCODE_LINKAGE_RECORD:
    case OPCODE_SOUND_PALETTE:
    case OPCODE_LIGHT_SOURCE_PALETTE:
    case OPCODE_LINE_STYLE_RECORD:
    case OPCODE_TEXTURE_MAPPING_PALETTE:
    case OPCODE_MATERIAL_TABLE:
	return 1;
    default:
	return 0;
    }
}

const char *FindFltName (FltTypes f)
{
    for (int i = 0; i < FltSize; i++) {
	if (FltNames[i].type == f) return FltNames[i].name;
    }
    return 0;
}

static int mgGetColorInd (mgrec *rec, int ind, int *rgb)
{
    mgrec *pbase = rec;
    while (pbase -> parent) {
	pbase = pbase -> parent;
    }
    while (pbase && pbase -> type != OPCODE_COLOR_TABLE) {
	pbase = mgGetNext (pbase);
    }
    if (pbase == NULL) return MG_FALSE;
    if (rec->vrsn > 1500) {
	if ((unsigned)ind > (pbase -> len - offsetof(struct aflt_ColorRecord, rgb))/4) 
	    return MG_FALSE;
	struct aflt_ColorRecord *cr;
	cr = (struct aflt_ColorRecord *)pbase->data;
	*rgb = swap_int(cr -> rgb[ind]);
	return MG_TRUE;
    }
    else {
	if ((unsigned)ind > (pbase -> len - offsetof(struct flt_ColorRecord, rgb))/4) 
	    return MG_FALSE;
	struct flt_ColorRecord *cr;
	cr = (struct flt_ColorRecord *)pbase->data;
	*rgb = swap_int(cr -> rgb[ind]);
	return MG_TRUE;
    }
}

void
mgInit (int type, void *param)
{
}

char *mgGetName (mgrec *rec)
{
    mgrec *p;
    for (p = rec->next; p && mgIsAncillary(p->type); p = p -> next) {
	if (p->type == OPCODE_LONG_IDENTIFIER) {
	    char *comm = (char *)malloc (p->len - 4 + 1);
	    memcpy (comm, p->data+4, p->len - 4);
	    comm[p->len - 4] = '\0';
	    return comm;
	}
    }
    char *comm = (char *)malloc (8);
    memcpy (comm, rec->data+4, 8);
    return comm;
}

char *
mgRec2Filename (mgrec *rec)
{
    char *comm = (char *)malloc (9);
    strcpy (comm, "filename");
    return comm;
}

mgrec *mgGetChild (mgrec *rec)
{
    if (rec -> child) return rec->child;
    mgrec *p;
    for (p = rec->next; p && (mgIsAncillary(p->type) || mgIsPalette(p->type)); p = p->next)
	if (p->child) 
	    return p->child;

    return 0;
}

mgrec *mgGetParent (mgrec *rec)
{
    return rec -> parent;
}

void mgExit ()
{
}
mgrec *mgGetNext (mgrec *rec)
{
    mgrec *p;
    for (p = rec->next; 
    p && (mgIsAncillary(p->type) || mgIsPalette(p->type)); 
    p = p->next)
	continue;
    return p;
}

int mgIsCode (mgrec *rec, FltTypes type)
{
    char buf[1024];

    switch (type) {
    case fltPolygon:
	return rec->type == OPCODE_POLYGON;
    case fltBsp:
	return rec->type == OPCODE_BINARY_SEPARATING_PLANE;
    case fltSwitch:
	return rec->type == OPCODE_SWITCH_BEAD;
    case fltDof:
	return rec->type == OPCODE_DEGREE_OF_FREEDOM;
    case fltLightPoint:
	return rec->type == OPCODE_LIGHT_SOURCE_RECORD;
    case fltLod:
	return rec->type == OPCODE_LEVEL_OF_DETAIL;
    case fltVertex:
	return rec->type == OPCODE_VERTEX_LIST;

    default:
	sprintf (buf, "Unknown FltType %d %s\n", type, FindFltName(type));
	OutputDebugString(buf);
	break;
    }
    return MG_FALSE;
}

void mgGetLastError (char *err, int size)
{
    sprintf (err, "Error number %d", lasterr);
}

void mgFree (char *data)
{
    free (data);
}

char *mgGetComment (mgrec *record)
{
    while (record && record -> type != OPCODE_TEXT_COMMENT) {
	record = record -> next;
    }
    if (record) {
	char *comm = (char *)malloc (record -> len - 4);
	memcpy (comm, record -> data, record -> len - 4);
	return comm;
    }
    return 0;
}


int mgGetFirstTexture (mgrec *record, int *texind, char *texname)
{
    return MG_FALSE;
    while (record && record -> type != OPCODE_TEXTURE_REFERENCE_RECORD) {
	record = record -> next;
    }
    if (record) {
#if 0
	if (record->vrsn > 1500) {
	    OutputDebugString ("Version 1500 textures not supported yet\n");
	    return MG_FALSE;
	}
#endif
	struct flt_TexturePatternRecord *tr = (struct flt_TexturePatternRecord *)record -> data;

	*texind = swap_int(tr -> patternIndex);
	strcpy (texname, tr -> filename);
	return MG_TRUE;

    }
    return MG_FALSE;
}

int
mgGetNextTexture (mgrec *record, int *texind, char *texname)
{
    if (record->vrsn > 1500) {
	OutputDebugString ("Version 1500 textures not supported yet\n");
	return MG_FALSE;
    }
    while (record && record -> type != OPCODE_TEXTURE_REFERENCE_RECORD) {
	record = record -> next;
    }
    if (record == NULL) return MG_FALSE;
    while (record && record -> type == OPCODE_TEXTURE_REFERENCE_RECORD) {
	struct flt_TexturePatternRecord *tr = (struct flt_TexturePatternRecord *)record -> data;

	int ind = swap_int(tr -> patternIndex);
	if (ind > *texind) {
	    *texind = ind;
	    strcpy (texname, tr -> filename);
	    return MG_TRUE;
	}
	record = record->next;
    }
    return MG_FALSE;
}


static mgrec *GetVertex (mgrec *rec, int n) 
{
    while (rec -> parent) rec = rec->parent;
    while (rec && rec -> type != OPCODE_VERTEX_PALETTE)
	rec = rec->next;
    if (rec == 0) return 0;
    int voff = rec->len;
    rec = rec->next;
    for (int i = 0; rec && voff < n; i++) {
	if (voff == n) return rec;
	voff += rec -> len;
	rec = rec->next;
    }
    return rec;
}

static int mgGetVertexNormal (mgrec *rec, double *i, double *j, double *k) 
{
    if (rec -> type != OPCODE_VERTEX_LIST) {
	OutputDebugString("GetVertexNormal not a vertex list\n");
	return MG_FALSE;
    }
    int *vtx = (int *)rec->data;
    mgrec *vr = GetVertex (rec, *vtx);
    if (vr == NULL) {
	OutputDebugString("GetVertexNormal no vertex list\n");
	return MG_FALSE;
    }
    switch (vr -> type) {
    case OPCODE_VERTEX_WITH_NORMAL:
	{
	    struct flt_VertexCoordinateNormal *vn = (struct flt_VertexCoordinateNormal *)vr->data;
	    *i = swap_float (vn->nx);
	    *j = swap_float (vn->ny);
	    *k = swap_float (vn->nz);
	    return MG_TRUE;
	}

    case OPCODE_VERTEX_WITH_NORMAL_AND_UV:
	{
	    struct flt_VertexCoordinateTextureNormal *vn = (struct flt_VertexCoordinateTextureNormal *)vr->data;
	    *i = swap_float (vn->nx);
	    *j = swap_float (vn->ny);
	    *k = swap_float (vn->nz);
	    return MG_TRUE;
	}
    default:
	return MG_FALSE;
    }
}

int mgGetVtxNormal (mgrec *rec, float *i, float *j, float *k)
{
    double i1, j1, k1;
    if (mgGetVertexNormal (rec, &i1, &j1, &k1) == MG_TRUE) {
	*i = (float)i1;
	*j = (float)j1;
	*k = (float)k1;
	return MG_TRUE;
    }
    return MG_FALSE;
}

int mgGetVtxColorRGB (mgrec *rec, short *r, short *g, short *b)
{
    if (rec -> type != OPCODE_VERTEX_LIST) {
	OutputDebugString("mgGetVtxColorRGB not a vertex list\n");
    }
    int *vtx = (int *)rec->data;
    mgrec *vr = GetVertex (rec, *vtx);
    if (vr == NULL) {
	char buf[1024];
	sprintf (buf, "No vertex reference found for %d\n", *vtx);
	OutputDebugString (buf);
	return MG_FALSE;
    }
    struct flt_VertexCoordinate *vc = (struct flt_VertexCoordinate *)vr->data;
    int pc= swap_short(vc -> vertexColor);
    int colind = pc >> 7;
    int colint = pc & 0x7f;
    int rgb;
    if (mgGetColorInd (rec, colind, &rgb) != MG_TRUE)
	return MG_FALSE;
    *r = (rgb & 0xff) * colint / 127; 
    *g = ((rgb>>8) & 0xff) * colint / 127; 
    *b = ((rgb>>16) & 0xff) * colint / 127; 

    return MG_TRUE;
}

int mgGetIcoord (mgrec *rec, FltTypes type, double *x, double *y, double *z)
{
    if (type != fltIcoord) {
	OutputDebugString("Not fltIcoord\n");
	return MG_FALSE;
    }
    if (rec -> type == OPCODE_VERTEX_LIST) {
	int *vtx = (int *)rec->data;
	mgrec *vr = GetVertex (rec, *vtx);
	if (vr == NULL) {
	    char buf[1024];
	    sprintf (buf, "No vertex reference found for %d\n", *vtx);
	    OutputDebugString (buf);
	    return MG_FALSE;
	}
	struct flt_VertexCoordinate *vc = (struct flt_VertexCoordinate *)vr->data;
	*x = swap_double (vc -> x);
	*y = swap_double (vc -> y);
	*z = swap_double (vc -> z);
	return MG_TRUE;
    }
    return MG_FALSE;
}

static int mgVtxFetch (mgrec *rec, FltTypes type, void *ptr)
{
    if (rec -> type != OPCODE_VERTEX_LIST) return MG_FALSE;
    int *vtx = (int *)rec->data;
    mgrec *vr = GetVertex (rec, *vtx);
    if (vr == NULL) {
	char buf[1024];
	sprintf (buf, "No vertex reference found for %d\n", *vtx);
	OutputDebugString (buf);
	return MG_FALSE;
    }
    switch (type) {
    case fltVU:
	{
	    if (vr -> type == OPCODE_VERTEX_WITH_NORMAL_AND_UV) {
		struct flt_VertexCoordinateTextureNormal *vt = (struct flt_VertexCoordinateTextureNormal *) vr -> data;
		*(float *)ptr = swap_float(vt -> u);
		break;
	    }
	    else if (vr -> type = OPCODE_VERTEX_WITH_UV) {
		struct flt_VertexCoordinateTexture *vt = (struct flt_VertexCoordinateTexture *) vr -> data;
		*(float *)ptr = swap_float(vt -> u);
		break;
	    }
	    else return MG_FALSE;
	}
	break;
    case fltVV:
	{
	    if (vr -> type == OPCODE_VERTEX_WITH_NORMAL_AND_UV) {
		struct flt_VertexCoordinateTextureNormal *vt = (struct flt_VertexCoordinateTextureNormal *) vr -> data;
		*(float *)ptr = swap_float(vt -> v);
		break;
	    }
	    else if (vr -> type = OPCODE_VERTEX_WITH_UV) {
		struct flt_VertexCoordinateTexture *vt = (struct flt_VertexCoordinateTexture *) vr -> data;
		*(float *)ptr = swap_float(vt -> v);
		break;
	    }
	    else return MG_FALSE;
	}
	break;
    default:
	return MG_FALSE;
    }
    return MG_TRUE;
	
}

static int mgBspFetch (mgrec *rec, FltTypes type, void *ptr)
{
    if (rec -> type != OPCODE_BINARY_SEPARATING_PLANE)
	return 0;
    struct aflt_BinarySeparatingPlane *bsp = 
	(struct aflt_BinarySeparatingPlane *)rec -> data;
    switch (type) {
    case fltDPlaneA:
	*(double *)ptr = swap_double(bsp->a);
	break;
    case fltDPlaneB:
	*(double *)ptr = swap_double(bsp->b);
	break;
    case fltDPlaneC:
	*(double *)ptr = swap_double(bsp->c);
	break;
    case fltDPlaneD:
	*(double *)ptr = swap_double(bsp->d);
	break;
    default:
	OutputDebugString ("Unknown BSP\n");
	break;
    }
    return 1;
}

static int 
mgPolyFetch (mgrec *rec, FltTypes type, void *ptr)
{
    if (rec -> type != OPCODE_POLYGON)
	return 0;
    if (rec -> vrsn > 1500) {
	struct aflt_PolygonRecord *pr = 
	    (struct aflt_PolygonRecord *)rec->data;
	switch (type) {
	case fltPolyMaterial:
	    {
		*(short *)ptr = swap_short(pr->materialCode);
		break;
	    }
	case fltPolyTransparency:
	    {
		*(short*)ptr = swap_short (pr->transparency);
		break;
	    }
    	case fltPolyMgTemplate:
	    {
		*(char *)ptr = pr->templateTransparency;
		break;
	    }
	case fltGcLightMode:
	    {
		*(char *)ptr = pr->lightMode;
		break;
	    }
	case fltPolyTexture:
	    {
		*(short*)ptr = swap_short (pr->textureNo);
		break;
	    }
	case fltPolyLineStyle:
	    {
		*(char *)ptr = pr->linestyle;
		break;
	    }
	case fltPolyDrawType:
	    {
		*(char *)ptr = pr->howToDraw;
		break;
	    }

	default:
	    OutputDebugString ("Unknown poly attr\n");
	    break;
	}
    }
    else {
	struct flt_PolygonRecord *pr = 
	    (struct flt_PolygonRecord *)rec->data;
	switch (type) {
	case fltPolyMaterial:
	    {
		*(short *)ptr = swap_short(pr->materialCode);
		break;
	    }
	case fltPolyTransparency:
	    {
		*(short*)ptr = swap_short (pr->transparency);
		break;
	    }
    	case fltPolyMgTemplate:
	    {
		*(char *)ptr = pr->templateTransparency;
		break;
	    }
	case fltGcLightMode:
	    {
		*(char *)ptr = pr->lightMode;
		break;
	    }
	case fltPolyTexture:
	    {
		*(short*)ptr = swap_short (pr->textureNo);
		break;
	    }
	case fltPolyLineStyle:
	    {
		*(char *)ptr = pr->linestyle;
		break;
	    }
	case fltPolyDrawType:
	    {
		*(char *)ptr = pr->howToDraw;
		break;
	    }

	default:
	    OutputDebugString ("Unknown poly attr\n");
	    break;
	}
    }
    return 1;
}

static int 
mgDofFetch (mgrec *rec, FltTypes type, void *ptr)
{
    if (rec -> type != OPCODE_DEGREE_OF_FREEDOM)
	return 0;
    if (rec -> vrsn > 1500) {
	struct aflt_DegreeOfFreedomRecord *dof = 
	    (struct aflt_DegreeOfFreedomRecord *)rec->data;
	switch (type) {
	case fltDofPutAnchorX:
	    {
		*(double *)ptr = swap_double(dof->originx);
		break;
	    }
	case fltDofPutAnchorY:
	    {
		*(double *)ptr = swap_double(dof->originy);
		break;
	    }
	case fltDofPutAnchorZ:
	    {
		*(double *)ptr = swap_double(dof->originz);
		break;
	    }
	case fltDofPutAlignX:
	    {
		*(double *)ptr = swap_double(dof->pointxaxis_x);
		break;
	    }
	case fltDofPutAlignY:
	    {
		*(double *)ptr = swap_double(dof->pointxaxis_y);
		break;
	    }
	case fltDofPutAlignZ:
	    {
		*(double *)ptr = swap_double(dof->pointxaxis_z);
		break;
	    }
	case fltDofPutTrackX:
	    {
		*(double *)ptr = swap_double(dof->pointxyplane_x);
		break;
	    }
	case fltDofPutTrackY:
	    {
		*(double *)ptr = swap_double(dof->pointxyplane_y);
		break;
	    }
	case fltDofPutTrackZ:
	    {
		*(double *)ptr = swap_double(dof->pointxyplane_z);
		break;
	    }
	case fltDofMaxX:
	    {
		*(double *)ptr = swap_double(dof->maxx);
		break;
	    }
	case fltDofMinX:
	    {
		*(double *)ptr = swap_double(dof->minx);
		break;
	    }

	default:
	    OutputDebugString ("Unknown DOF attr\n");
	    break;
	}
    }
    else {
	struct flt_DegreeOfFreedomRecord *dof = 
	    (struct flt_DegreeOfFreedomRecord *)rec->data;
	switch (type) {
	case fltDofPutAnchorX:
	    {
		*(double *)ptr = swap_double(dof->originx);
		break;
	    }
	case fltDofPutAnchorY:
	    {
		*(double *)ptr = swap_double(dof->originy);
		break;
	    }
	case fltDofPutAnchorZ:
	    {
		*(double *)ptr = swap_double(dof->originz);
		break;
	    }
	case fltDofPutAlignX:
	    {
		*(double *)ptr = swap_double(dof->pointxaxis_x);
		break;
	    }
	case fltDofPutAlignY:
	    {
		*(double *)ptr = swap_double(dof->pointxaxis_y);
		break;
	    }
	case fltDofPutAlignZ:
	    {
		*(double *)ptr = swap_double(dof->pointxaxis_z);
		break;
	    }
	case fltDofPutTrackX:
	    {
		*(double *)ptr = swap_double(dof->pointxyplane_x);
		break;
	    }
	case fltDofPutTrackY:
	    {
		*(double *)ptr = swap_double(dof->pointxyplane_y);
		break;
	    }
	case fltDofPutTrackZ:
	    {
		*(double *)ptr = swap_double(dof->pointxyplane_z);
		break;
	    }
	case fltDofMaxX:
	    {
		*(double *)ptr = swap_double(dof->maxx);
		break;
	    }
	case fltDofMinX:
	    {
		*(double *)ptr = swap_double(dof->minx);
		break;
	    }

	default:
	    OutputDebugString ("Unknown DOF attr\n");
	    break;
	}
    }
    return 1;
}

static int mgMatFetch (mgrec *rec, FltTypes type, void *ptr)
{
    if (rec->vrsn > 1500) {
	OutputDebugString ("mgMatFetch doesn't support 1500\n");
	return MG_FALSE;
    }
    if (rec -> type != OPCODE_MATERIAL_TABLE)
	return MG_FALSE;
    struct flt_MaterialTable *mt = (struct flt_MaterialTable *)rec -> data;
    switch (type) {
    case fltMatAlpha:
	*(float *)ptr = swap_float (mt -> alpha);
	break;
    default:
	return MG_FALSE;
    }
    return MG_TRUE;
}

int mgGetAttList (mgrec *rec, ...)
{
    va_list ap;
    FltTypes type;
    int count = 0;
    va_start(ap, rec);
    while ((type = va_arg(ap, FltTypes)) != 0) {
	switch (type) {
	case fltVU:
	case fltVV:
	    {
		float *fp = va_arg (ap, float *);
		if (mgVtxFetch (rec, type, fp))
		    count ++;
	    }
	    break;
	case fltDPlaneA:
	case fltDPlaneB:
	case fltDPlaneC:
	case fltDPlaneD:
	    {
		double *dp = va_arg(ap, double *);
		if (mgBspFetch (rec, type, dp))
		    count ++;
	    }
	    break;
	case fltPolyMaterial:
	    {
		short *mind = va_arg(ap, short *);
		if (mgPolyFetch(rec, type, mind)) 
		    count ++;
	    }
	    break;
	case fltPolyTransparency:
	    {
		short *tp = va_arg(ap,  short *);
		if (mgPolyFetch(rec, type, tp)) 
		    count ++;
	    }
	    break;
	case fltPolyMgTemplate:
	    {
		char *cp = va_arg(ap, char *);
		if (mgPolyFetch (rec, type, cp))
		    count ++;
	    }
	    break;
	case fltGcLightMode:
	    {
		char *cp = va_arg(ap, char *);
		if (mgPolyFetch (rec, type, cp))
		    count ++;
	    }
	    break;
	case fltPolyTexture:
	    {
		short *cp = va_arg(ap, short *);
		if (mgPolyFetch (rec, type, cp))
		    count ++;
	    }
	    break;
	case fltPolyLineStyle:
	    {
		char *cp = va_arg(ap, char *);
		if (mgPolyFetch (rec, type, cp))
		    count ++;
	    }
	    break;
	case fltPolyDrawType:
	    {
		char *cp = va_arg(ap, char *);
		if (mgPolyFetch (rec, type, cp))
		    count ++;
	    }
	    break;
	case fltMatAlpha:
	    {
		float *fp = va_arg (ap, float *);
		if (mgMatFetch (rec, type, fp))
		    count ++;

	    }
	    break;
	case fltDofPutAnchorX:
	case fltDofPutAnchorY:
	case fltDofPutAnchorZ:
	case fltDofPutAlignX:
	case fltDofPutAlignY:
	case fltDofPutAlignZ:
	case fltDofPutTrackX:
	case fltDofPutTrackY:
	case fltDofPutTrackZ:
	case fltDofMaxX:
	case fltDofMinX:
	    {
		double *dp = va_arg(ap, double *);
		if (mgDofFetch (rec, type, dp))
		    count ++;
	    }
	    break;
	default:
	    {
		char buf[1024];
		sprintf (buf, "Unsupported attr type %d %s\n", type, FindFltName(type));
		OutputDebugString(buf);
		va_arg (ap, char *); // best guess
	    }
	break;

	}
    }
    va_end (ap);
    return count;
} 



static short getshort (FILE *fp)
{
    int c1 = getc(fp);
    int c2 = getc(fp);
    return (c1 << 8) | c2;
}
static mgrec *mgReadSequence (FILE *fp, mgrec *prnt, int vrsn);

static mgrec *mgReadRecord (FILE *fp, mgrec *prnt, int vrsn)
{
    mgrec *rec = NULL;
    
    while (rec == NULL) {
	short type = getshort(fp);
	if (feof(fp)) return 0;
	
	rec = (mgrec *)calloc (1, sizeof *rec);
	rec->type = (OpCode)type;
	rec->len = getshort(fp);
	rec->data = (char *) calloc (1, rec -> len);
	rec->vrsn = vrsn;
	rec->parent = prnt;
	memcpy (rec->data, &rec->type, 2);
	memcpy (rec->data+2, &rec->len, 2);
	fread (rec -> data + 4, rec -> len - 4, 1, fp);
	if (mgIgnoreRec (rec->type)) {
	    free (rec->data);
	    free (rec);
	    rec = NULL;
	}
	else if (rec->type == OPCODE_PUSH_LEVEL) {
	    rec -> child = mgReadSequence(fp, prnt, vrsn);
	}
	else if (rec -> type == OPCODE_VERTEX_LIST) { // recode this as a sequence
	    mgrec *last = rec;
	    char *data = rec -> data;
	    struct flt_VertexList *vl = (struct flt_VertexList *)data;
	    rec -> data = (char *)malloc(sizeof (int));
	    *(int*)rec -> data = swap_int(vl->offset[0]);
	    for (int i = 1; i < (rec->len - 4) / 4; i++) {
		last->next = (mgrec *)calloc (1, sizeof *rec);
		last = last->next;
		last -> type = rec -> type;
		last -> len = rec -> len;
		last -> parent = prnt;
		last -> vrsn = vrsn;
		last -> data = (char *)malloc(sizeof(int));
		*(int*)last -> data = swap_int(vl->offset[i]);
	    }
	    free (data);
	}
    }
    return rec;
}

static mgrec *mgReadSequence (FILE *fp, mgrec *prnt, int vrsn)
{
    mgrec *base = 0;
    mgrec *cur = base;
    mgrec *np;
    int pcount = 0;
    while (np = mgReadRecord (fp, base, vrsn)) {
	if (np -> type == OPCODE_PUSH_LEVEL) {
	    pcount ++;
	    if (cur == NULL) {
		cur = np -> child;
		if (base == NULL) base = cur;
	    }
	    else {
		cur-> child = np -> child;
	    }
	    np -> child = NULL;
	    freenode (np);
	    if (cur) {
		for (np = cur -> child; np; np = np -> next) {
		    np -> parent = base;
		}
	    }
	}
	else if (np -> type == OPCODE_POP_LEVEL) {
	    freenode (np);
	    pcount --;
	    if (pcount <= 0)
		break;
	}
	else {
	    if (base == 0) {
		cur = base = np;
	    }
	    else cur->next = np;
	    np ->parent = prnt;
	    while (cur -> next) {
		cur->parent = prnt;
		cur = cur->next;
	    }
	}
    }
    return base;
}
mgrec *mgOpenDb (char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) return NULL;
    mgrec *base = mgReadRecord (fp, 0, 0);
    if (base->type != OPCODE_HEADER) {
	return NULL;
    }
    struct flt_HeaderRecord *fh = (struct flt_HeaderRecord *)base->data;
    printf ("Version %d, db version %d\n", swap_int(fh->formatRev), swap_int(fh->DBRev));
    base -> next = mgReadSequence (fp, 0, swap_int(fh->formatRev));
    fclose (fp);
    if (base->type != OPCODE_HEADER) {
	return NULL;
    }
    return base;
}


void mgCloseDb (mgrec *db)
{
//    freenode (db);
}

mgrec *mgGetMaterial (mgrec *db, int matind)
{
    if (db -> vrsn > 1500) {
	OutputDebugString ("mgGetMaterial doesn't support 1500\n");
	return MG_FALSE;
    }
    mgrec *pbase = db;
    while (pbase -> parent) {
	pbase = pbase -> parent;
    }
    while (pbase && pbase -> type != OPCODE_MATERIAL_TABLE) {
	pbase = pbase -> next;
    }
    if (pbase == NULL) return 0;
    if (pbase -> child) {
	for (mgrec *cp = pbase -> child; cp; cp = cp -> next) {
	    if (cp -> xdata == matind)
		return cp;
	}
    }
    flt_MaterialRecord *fmr = (flt_MaterialRecord *)pbase->data;
    if (matind < 0 || matind > 64)
	return 0;
    mgrec *mr = (mgrec *) calloc (1, sizeof *mr);
    mr -> type = OPCODE_MATERIAL_TABLE;
    mr -> parent = pbase;
    mr -> xdata = matind;
    mr -> next = pbase -> child;
    pbase->child = mr;
    mr -> data = (char *)calloc (1, sizeof (flt_MaterialTable));
    memcpy (mr -> data, &fmr -> mat[matind], sizeof (flt_MaterialTable));
    return mr;
}

static int mgFind15Material (mgrec *db, int matind, short *r, short *g, short *b)
{
    mgrec *pbase = db;
    while (pbase -> parent) {
	pbase = pbase -> parent;
    }
    while (pbase && pbase -> type != OPCODE_MATERIAL_PALETTE) {
	pbase = pbase -> next;
    }
    if (pbase == NULL) return 0;
    while (pbase->type == OPCODE_MATERIAL_PALETTE) {
	aflt_MaterialRecord *matrec = (aflt_MaterialRecord *)pbase->data;
	if (swap_int(matrec->materialIndex) == matind) {
	    *r = (short)(255.0f * swap_float(matrec->diffuseRed));
	    *g = (short)(255.0f * swap_float(matrec->diffuseGreen));
	    *b = (short)(255.0f * swap_float(matrec->diffuseBlue));
	    return MG_TRUE;
	}
	pbase = pbase->next;
    }
    return MG_FALSE;

}

static int mgFind14Material (mgrec *db, int matind, short *r, short *g, short *b)
{
    mgrec *pbase = mgGetMaterial(db, matind);
    if (pbase == NULL) return MG_FALSE;
    flt_MaterialTable *matrec = (flt_MaterialTable *)pbase->data;
    *r = (short)(255.0f * swap_float(matrec->diffuseRed));
    *g = (short)(255.0f * swap_float(matrec->diffuseGreen));
    *b = (short)(255.0f * swap_float(matrec->diffuseBlue));
    return MG_TRUE;
}

int mgGetPolyColorRGB (mgrec *rec, short *r, short *g, short *b)
{
    if (rec -> type != OPCODE_POLYGON)
	return MG_FALSE;
    int colindex;
    int colintens;
    if (strncmp (rec->data + 4, "f45", 3) == 0 ||
	strncmp (rec->data + 4, "f42", 3) == 0) {
	*r =0; *g = 255; *b = 0;
	return MG_TRUE;
    }

    if (rec->vrsn > 1500) {
        struct aflt_PolygonRecord *pr;
	pr = (struct aflt_PolygonRecord *)rec->data;
	int mc = swap_short(pr->materialCode);
	if (mc != -1) {
	    return mgFind15Material (rec, mc, r, g, b);
	}
	else {
	    int pc = swap_short (pr->primaryColor);
	    colindex = pc >> 7;
	    colintens = pc & 0x7f;
	}
    }
    else {
	struct flt_PolygonRecord *pr;
	pr = (struct flt_PolygonRecord *)rec->data;
	int mc = swap_short(pr->materialCode);
	if (mc != -1) {
	    return mgFind14Material (rec, mc, r, g, b);
	}
	unsigned short pc = swap_short (pr->primaryColor);
	colindex = pc >> 7;
	colintens = pc & 0x7f;
    }
    int rgb;
    if (mgGetColorInd (rec, colindex, &rgb) != MG_TRUE)
	return MG_FALSE;
    *r = (rgb & 0xff) * colintens / 127; 
    *g = ((rgb>>8) & 0xff) * colintens / 127; 
    *b = ((rgb>>16) & 0xff) * colintens / 127; 
    return MG_TRUE;
}

static int mgGetFaceFromVertexNormal (mgrec *vert, double *i, double *j, double *k)
{
    double i1, j1, k1;
    double nx = 0.0, ny = 0.0, nz = 0.0;
    int count = 0;
    while (vert) {
	if (mgGetVertexNormal(vert, &i1, &j1, &k1) != MG_TRUE)
	    return MG_FALSE;
	nx += i1;
	ny += j1;
	nz += k1;
	count ++;
	vert = mgGetNext (vert);
    }
    nx /= count;
    ny /= count;
    nz /= count;
    Normalize (&nx, &ny, &nz);
    *i = nx;
    *j = ny;
    *k = nz;
    return MG_TRUE;
}

int mgGetPolyNormal(mgrec *rec, double *i, double *j, double *k)
{
    mgrec *child = mgGetChild (rec);
    if (child == 0 || child -> type != OPCODE_VERTEX_LIST) {
	OutputDebugString ("No child with vertex\n");
	return MG_FALSE;
    }
    if (mgGetFaceFromVertexNormal (child, i, j, k) == MG_TRUE)
	return MG_TRUE;
    // otherwise compute it ourself.
    OutputDebugString ("Got to calculate our own normal\n");
    double x1, y1, z1;
    if (mgGetIcoord (child, fltIcoord, &x1, &y1, &z1) == MG_FALSE) {
	OutputDebugString ("No vertex 1\n");
	return MG_FALSE;
    }
    double x2, y2, z2;
    child = child -> next;
    if (mgGetIcoord (child, fltIcoord, &x2, &y2, &z2) == MG_FALSE) {
	OutputDebugString ("No vertex 2\n");
	return MG_FALSE;
    }
    double x3, y3, z3;
    child = child -> next;
    if (mgGetIcoord (child, fltIcoord, &x3, &y3, &z3) == MG_FALSE) {
	OutputDebugString ("No vertex 3\n");
	return MG_FALSE;
    }
    // 2 - 1 = u
    x1 = x2 - x1;
    y1 = y2 - y1;
    z1 = z2 - z1;
    // 3 - 2 = v
    x2 = x3 - x2;
    y2 = y3 - y2;
    z2 = z3 - z2;
    // u cross v
    x3 = y1 * z2 + z1 * y2;
    y3 = z1 * x2 + x1 * z2;
    z3 = x1 * y2 + y1 * x2;
    Normalize (&x3, &y3, &z3);
    *i = x3;
    *j = y3;
    *k = z3;

    return MG_TRUE;
}

int mgGetMatrix (mgrec *rec, FltTypes type, mgmatrix *mat)
{
    OutputDebugString("mgGetmatrix not implemented\n");
    return MG_FALSE;
}

int mgGetNormColor (mgrec *rec, FltTypes type, float *r, float *g, float *b)
{
    if (rec->vrsn > 1500) {
	OutputDebugString ("mgGetNormColor not supported in 1500\n");
	return MG_FALSE;
    }
    if (rec -> type != OPCODE_MATERIAL_TABLE)
	return MG_FALSE;
    struct flt_MaterialTable *mt = (struct flt_MaterialTable *)rec -> data;
    switch (type) {
    case fltDiffuse:
	*r = swap_float(mt->diffuseRed);
	*g = swap_float(mt->diffuseGreen);
	*b = swap_float(mt->diffuseBlue);
	break;
    default:
	OutputDebugString ("Unknown type for GetNormColor\n");
	return MG_FALSE;
    }

    return MG_TRUE;
}

int mgIndex2RGB (mgrec *rec, int colind, float intensity, short *r, short *g, short *b)
{
    OutputDebugString("mgIndex2RGB not implemented\n");
    return MG_FALSE;
}
void mgSetMessagesEnabled (int, int)
{
}
