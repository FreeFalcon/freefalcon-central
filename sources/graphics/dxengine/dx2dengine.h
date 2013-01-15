#define	MAX_VERTEX_INDEXED	(65536*(MAX_DYNAMIC_BUFFERS+1))
#define	MAX_DYNAMIC_ITEMS	(MAX_VERTEX_INDEXED/4)
#define	MAX_DYNAMIC_VERTICES		65500				// Number of Vetrtices in a dynamic Buffer



#define	EMISSIVE	true
#define	NO_TEXTURE	NULL

// This is the primitive used to draw a dynamic Oobject
class	CDynamicPrimitive
{
	public:
		CDynamicPrimitive		*Next;
		LPDIRECT3DVERTEXBUFFER7 Vb;					// Assigned VB address
		DWORD					Offset;					// The Offset in the Vertex Buffer when loaded
		DWORD					Number;					// The Number of vertices
		DWORD					TexID;				// Texture ID
};



// The Texture item, this class got a texture definition name and it's U/V tex Coords in the parent surface
class	CTextureItem
{
public:
	CTextureItem(void)	{ Next=NULL; Name[0]=0; memset(TuTv, 0, sizeof(TuTv)); }
	CTextureItem	*Next;
	char			Name[32];
	float			TuTv[4][2];
};


// This is a texture Surface Descrition
class	CTextureSurface
{
public:
	CTextureSurface(void)	{ Next=NULL; FileName[0]=0; ItemList=NULL; }
	CTextureSurface	*Next;
	Texture			Tex;
	char			FileName[32];
	CTextureItem	*ItemList;
};



// ************************************ 2D STUFF *******************************************

#define	MAX_2D_BUFFERS					1				// Number of 2D Buffers
#define	MAX_2D_VERTICES					0x10000			// Numebr of vertices for each 2D Vertex Buffer
#define	MAX_2D_LAYERS					7				// Number of 2D layers in a scene
#define	MAX_2D_ITEMS					0x10000			// Number of possible managed Quads

#define	MAX_2D_VERTICES_MANAGED			(MAX_2D_BUFFERS * MAX_2D_VERTICES);
#define	MAX_VERTICES_PER_DRAW			2048			// Max vertices passed to a draw function

// Draw Flags
#define	POLY_BB							0x00000001		// Billboard quad flag
#define	POLY_CREATE						0x00000002		// create vertices flag
//***************************************************************************************************************************
#define	POLY_VISIBLE					0x00000004		// Force rendering ( used when Visibility check is already performed )
														// WARNING - GetDistance must be performed before to have the engine
														// to store the item Distance to be used, as it is not calculated
														// in the draw function
//***************************************************************************************************************************
#define	CAMERA_VERTICES					0x00000008		// The verices passed to the Drawing function are CAMERA SPACE Coordinates
														// so, not relative to object center
//***************************************************************************************************************************
#define	CALC_DISTANCE					0x00000010		// Recalc the distance of the passed object
//***************************************************************************************************************************
#define	TAPE_ENTRY						0x00000020		// This is a Tape Entry




#define	POLY_FAN						0x0100000		// FAN ITEM
#define	POLY_STRIP						0x0200000		// STRIP ITEM
#define	POLY_TAPE						0x0400000		// TAPE ITEM
#define	POLY_LINE						0x0800000		// TAPE ITEM
#define	POLY_3DOBJECT					0x1000000		// this is a 3D object coming from DX Engine


// Layers Flags	
#define	LAYER_SORT						0x00000001		// sorting Layer Flag

// The layers used in drawing 2D stuff
#define	LAYER_GROUND					0x00000000		// Draw Layer
#define	LAYER_STRATUS1					0x00000001		// Draw Layer
#define	LAYER_MIDDLE					0x00000002		// Draw Layer
#define	LAYER_STRATUS2					0x00000003		// Draw Layer
#define	LAYER_ROOF						0x00000004		// Draw Layer
#define	LAYER_TOP						0x00000005		// Draw Layer
#define	LAYER_AUTO						0x00000006		// Draw Layer
#define	LAYER_NODRAW					0xffffffff		// No layer to Draw


// The structure used to map & sort 2D Quads
typedef	struct	{
	DWORD			Next;								// Next Item in sorting
	DWORD			Dist256;							// Distance integer scaled up to 256
	LPDIRECT3DVERTEXBUFFER7	Vb;							// Assigned Vb;
	DWORD			Index, Index2;						// The indexes in the VBuffer
	DWORD			NrVertices;							// vertices
	DWORD			TexHandle;							// The Texture Handle
	DWORD			Flags;
	float			Height;
} DrawItemType;




// The structure of a layer manager
// In falcon 2D Alpha Objects are divided into layers btw the 2 Cloud Layers...
// This to give the right draw order
typedef	struct	{
	DWORD	Start, End;
	DWORD	Flags;
} LayerItemType;



// The structure of items  pointing a vertex buffer
typedef	struct	{
	LPDIRECT3DVERTEXBUFFER7	Vb;
	D3DDYNVERTEX			*VbPtr;				// The pointer in the VB
	DWORD					LastIndex;			// last assigned index in the buffer
	DWORD					LastTapeIndex;		// the Last Tape Index
}	Dyn2DBufferType;	



// Structure of a sorting item
typedef	struct	{
	DWORD	Index, Next;
} SortItemType;


