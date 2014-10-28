#pragma once

#include <ddraw.h>
#include <d3d.h>
#include <d3dxcore.h>
#include <d3dxmath.h>
#include "../include/ObjectInstance.h"
#include "DxDefines.h"

// The only VB Manager
extern class CDXVbManager TheVbManager;


// Macros to enter bitand leave VB Critical section
#define LOCK_VB_MANAGER EnterCriticalSection(&cs_VbManager);
#define UNLOCK_VB_MANAGER LeaveCriticalSection(&cs_VbManager);


// This definition enables the use o the Draw Scrambler
// the Draw Scrambler avoid drawing the same item in same VB more times consecutively, degrading performances...
//#define USE_DRAW_SCRAMBLER

//#define CRYPTED_MODELS
#define KEY_CRYPTER 0xa5FF2009


// *** Number of Vertex Buffer defined at start ***
#define BASE_VERTEX_BUFFERS 8
#define MAX_VERTEX_BUFFERS 64



// *** Number of VATs and DRAW ITEMs created at boot in their own Pools
#define BASE_VATS 16384
#define BASE_DRAWS 16384

// Returened if failed to find a chunk of space for data
#define CHUNK_NOT_FOUND -1


// Classes for Vertex Buffers
#define VB_CLASS_FEATURES 0x0000001
#define VB_CLASS_DOMAIN_GROUND 0x0000002
#define VB_CLASS_DOMAIN_AIR 0x0000004



// *New vertex type *
typedef struct
{
    float vx;
    float vy;
    float vz;
    float nx;
    float ny;
    float nz;
    DWORD dwColour;
    DWORD dwSpecular;
    float tu;
    float tv;
} D3DVERTEXEX;


// *** Vertex buffer Allocation table Item ****
class CVbVAT
{


public:
    CVbVAT(CVbVAT *Parent, CVbVAT *Child, DWORD Id, DWORD Start, DWORD Size, DWORD VBFree);
    ~CVbVAT(void);

    CVbVAT* Next, *Prev; // Pointers in the List
    DWORD ID; // Model Identifier
    DWORD VStart, VSize; // Start and end of this chunk
    DWORD Gap; // Gap to next VbVAT chunk
};




// *** Draw Item of an object Belonging to a VB
class CDrawItem
{


public:
    CDrawItem();
    DWORD ID; // Id
    CDrawItem *NextInPool; // Next intem in the Draw Items Pool
    CDrawItem *Next, *Prev; // Next bitand Previous Draw item in list
    void *Object; // Object Instance Pointer
    D3DXMATRIX RotMatrix; // Object Rotation
    Ppoint Pos; // Object Position
    // float scale; // Object Scale
    bool DynamicLited; // under Fx of Dynamic lights
    DWORD LightID; // Light Owner ID
    float FogLevel; // The Fog Level for the Object
};



typedef struct
{
    DWORD Class; // CLass of Vertex Buffer
    DWORD Free; // Vertex Space available
    CDrawItem *pDrawRoot; // Root for the VB Draw List
    CDrawItem *pDrawPtr; // Pointer to Last Draw Item
    DWORD DrawsCount; // Draw Items Present
    CVbVAT *pVAT; // Root for the VB VAT
    DWORD BootGap; // The Eventual 1st Gap to 1st VAT
    LPDIRECT3DVERTEXBUFFER7 Vb; // Assigned VB
} VBufferListType;




#define MAX_MANAGED_MODELS 0x4000
#define D3D m_pD3D
#define D3DFVF_MANAGED (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX1)
#define VERTEX_STRIDE sizeof(D3DVERTEXEX)


typedef struct
{
    LPDIRECT3DVERTEXBUFFER7 Vb; // Assigned VB address
    CVbVAT *pVAT; // The VB VAT assigned
    VBufferListType *pVbList; // Pointer to the VB List Item assigned
    DWORD NVertices; // Number Of Vertices composing the object
    DWORD BaseOffset; // Base Index of vertices in the VB
    DWORD NNodes; // Number of Nodes in the model
    void *Nodes; // Pointer to Object Draw Nodes
    DWORD NTex; // Number of Textures
    DWORD *Texs; // Pointer to textures
    BYTE *Root; // Root of the Model
    bool Valid; // Valid model
} VBItemType;



///////////////////////////////////// DYNAMIC BUFFERS STUFFS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

#define MAX_DYNAMIC_BUFFERS 4 // Number of Dynamic Vertex Buffers
// *Dynamic Buffer Vertex Type *
typedef struct
{
    D3DVECTOR pos;
    DWORD dwColour;
    DWORD dwSpecular;
    float tu;
    float tv;
} D3DDYNVERTEX;
#define D3DFVF_DYNAMIC (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX1)


// * Simple Items Point/Line Vertex type
// *Dynamic Buffer Vertex Type *
typedef struct
{
    D3DVECTOR pos;
    DWORD dwColour;
    DWORD dwSpecular;
    float tu, tv;
} D3DSIMPLEVERTEX;
#define D3DFVF_SIMPLE (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX1)

// The Base Item for a Dynamic Vertex Buffer, it may define a single vertex or a BillBoarded Square
// 3D items are 'unioned' with XMM 128 bits variables, to be available to use with XMM math
class CDrawBaseItem
{
public :
    float Distance;
    CDrawBaseItem *Next, *NextInPool;
    D3DDYNVERTEX Vtx[4];
};

// This is the Primitive Node used in Dynamic Buffering
class CDynamicDraw
{
public:
    CDynamicDraw(void);

    CDynamicDraw *Next; // pointers to adiacent primitives in list
    DWORD TexID; // Texture ID
    D3DXVECTOR3 CoordSize; // The Left Up Angle used to calculate sides proportions
    CDrawBaseItem *StartItem, *LastItem; // The linked Draw Items List
};


// This is the Dynamic Buffer Item
typedef struct
{
    LPDIRECT3DVERTEXBUFFER7 Vb; // Assigned VB address
    void *VbPtr; // The pointer in the VB
    DWORD LastIndex; // last assigned index in the buffer
} DynBufferType;


// This is the Simple Buffer Item
typedef struct
{
    LPDIRECT3DVERTEXBUFFER7 Vb; // Assigned VB address
    D3DSIMPLEVERTEX *VbPtr; // The pointer in the VB
    DWORD Points; // numebr of Lite/Emitting points
    DWORD Lines; // numebr of Lite/Emitting lines
    DWORD MaxPoints; // numebr of Lite/Emitting points
    DWORD MaxLines; // numebr of Lite/Emitting lines
    D3DSIMPLEVERTEX *PointPtr; // The pointer in the VB
    D3DSIMPLEVERTEX *LinePtr; // The pointer in the VB
} SimpleBufferType;

#define MAX_POINTS 16384 // Number of Points managed in a simple buffer
#define MAX_LINES 16384 // Number of Lines managed in a simple buffer
#define MAX_QUADS 4095 // Number of Emitting Quads managed in a simple buffer

#define POINTS_OFFSET 0 // Offset in the Simple Buffer form POINTS
#define LINES_OFFSET (POINTS_OFFSET+MAX_POINTS) // Offset in the Simple Buffer form LINES
#define QUADS_OFFSET (LINES_OFFSET+MAX_LINES*2) // Offset in the Simple Buffer form QUADS

class CDXVbManager
{
public:
    DWORD TotalDraws; // Total number of draws allocated
    CDXVbManager();
    virtual ~CDXVbManager(void);
    bool SetupModel(DWORD ID, BYTE *Root, DWORD Class);
    void ReleaseModel(DWORD ID);
    void AssertValidModel(DWORD ID)
    {
        pVBuffers[ID].Valid = true;
    }
    void Setup(IDirect3D7 *pD3D);
    void Release(void);
    void GetModelData(VBItemType &, DWORD);
    DWORD GetTextureID(DWORD ID, DWORD TexIdx);
    void AddDrawRequest(ObjectInstance *objInst, DWORD ID, D3DXMATRIX *Transformation, bool Lited, DWORD LightID, float FogLevel = 0);
    void AddDrawItem(VBufferListType *pVBDesc, DWORD ID, ObjectInstance *objInst, D3DXMATRIX *Transformation, bool Lited, DWORD LightID, float FogLevel = 0);
    void ResetDrawList(void);
    bool GetDrawItem(ObjectInstance **objInst, DWORD *ID, D3DXMATRIX *Transformation, bool *Lited, DWORD *LightID, float *FogLevel);
    void ClearDrawList(void);
    void Encrypt(DWORD *);
    void Decrypt(DWORD *);
    bool CheckDataID(DWORD ID);
    (BYTE*) GetModelRoot(DWORD ID)
    {
        return ((BYTE*)pVBuffers[ID].Root);
    }
    DWORD *GetModelTextures(DWORD ID)
    {
        return pVBuffers[ID].Texs;
    }
    IDirect3D7* GetVBD3D(void)
    {
        return m_pD3D;
    };

    VBufferListType *GetVbList(void)
    {
        return pVbList;
    };


protected:

    DWORD VBAddObject(VBufferListType *Vbl, DWORD nVertices, DWORD ID);
    bool VBCheckForBuffer(DWORD ID, DWORD Class, DWORD nVertices);
    void CreateVB(DWORD i, DWORD Class);
    void DestroyVAT(VBufferListType *pVb, CVbVAT *Vat);

    VBItemType pVBuffers[MAX_MANAGED_MODELS];
    IDirect3D7 *m_pD3D;
    VBufferListType pVbList[MAX_VERTEX_BUFFERS]; // List of managed vertex Buffers
    VBufferListType PitList; // The Pit List

    CDrawItem *pVDrawItemPool, *pDrawPoolPtr; // Pool of Draw Items
    CDrawItem ThePitItem; // The 3D Pit Draw Item

    DWORD BufferToDraw; // VBuffer to go under Draw
    CDrawItem *RootItemToDraw, *NextItemToDraw; // Root bitand Next Item to be Drawn

    DWORD DrawHits[MAX_MANAGED_MODELS]; // Cache Hits Used in drawing Models
    DWORD DrawPass; // Draw Cycle

    bool VBManagerInitialized; // Flag about the object initialization


    //////////////////////////////// The Dynamic Buffer stuffs \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

public:
    DynBufferType *GetDynamicBuffer(DWORD VertexNr);
    void AddDynamicPrimitive(WORD DynBufferNr, CDynamicDraw *CDD);
    //CDynamicDraw *GetPrimitiveList(void);
    DynBufferType *GetDynamicBufferNr(DWORD Nr);
    SimpleBufferType SimpleBuffer; // The simple items Buffer
    void OpenSimpleBuffer(void);



protected:
    DynBufferType DynamicBuffer[MAX_DYNAMIC_BUFFERS]; // The NON STATIC Vertex Buffers



};


extern CRITICAL_SECTION cs_VbManager;
