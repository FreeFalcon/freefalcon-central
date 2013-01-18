#pragma once

// IF DEFINED, THIS MACRO, MAKES THE ENCODER AND ENGINE TO USE INDEXED VERTICES
// IN MODEL AND INDEXED DRAWS IN ENGINE
#define INDEXED_MODE_ENGINE



#include <ddraw.h>
#include <d3d.h>
#include <d3dxcore.h>
#include <d3dxmath.h>
#include "../include/TexBank.h"
#include "DxDefines.h"
#include "DXVbManager.h"
#include "DX2DEngine.h"
#include "DXLightEngine.h"


#define DEFAULT_ZBIAS 0 // Base zBias level
#ifdef EDIT_ENGINE
#define MAX_ALPHA_SURFACES 32*1024 // Max Stackable Alpha Suraces
#define MAX_SOLID_SURFACES 32*1024 // Max Stackable Alpha Suraces
#else
#define MAX_ALPHA_SURFACES 8*1024 // Max Stackable Alpha Suraces
#define MAX_SOLID_SURFACES 32*1024 // Max Stackable Alpha Suraces
#endif

#define NVG_T_FACTOR 0x0030A030
#define DX_MAX_NESTED_STATES 128


// The DX Engine states
typedef enum { DX_OTW = 0, DX_TV, DX_NVG, DX_DBS } DX_StateType;

// This is the structure used to save DX Engine status
typedef struct
{
    DX_StateType RenderState;
} DX_StatesStackType;


#define ENABLE TRUE
#define DISABLE FALSE

#define MAX_FOG_RANGE 200000.0f


// Tish Structure is used to store various nodes to be drawn secondary
// e.g. ALPHA BLENDED NODES
typedef struct
{
    VBItemType Vb; // The VB the Node belongs to
    BYTE *Surface; // The Surface Node address
    D3DXMATRIX State; // The Transformation for the surface
    DWORD TexID;
    ObjectInstance *ObjInst;
    float FogLevel;
    bool LightMap[MAX_DYNAMIC_LIGHTS];
} SurfaceItemType;

typedef union
{
    BYTE *BYTE;
    DXNodeHeadType *HEAD;
    DxSurfaceType *SURFACE;
    DxMaterialType *MATERIAL;
    DxTextureType *TEXTURE;
    DxDofType *DOF;
    DxDofEndType *DOFEND;
    DxSlotType *SLOT;
} NodeScannerType;


typedef struct
{
    DWORD StackLevel, StackMax;
    SurfaceItemType *Stack;
} SurfaceStackType ;

#define SURFACE_STACK(name, max) static SurfaceStackType name;
#define INIT_S_STACK(name, max) name.StackLevel=0; name.StackMax=max; name.Stack=(SurfaceItemType*)malloc(sizeof(SurfaceItemType)*max);



class CDXEngine
{
public:
    CDXEngine(void);
    ~CDXEngine(void);

    IDirect3DDevice7* GetD3DD(void)
    {
        return m_pD3DD;
    };


    // Various functions for Debug
    void EnableCull(bool Status);
    void MoveDof(bool Status);
    DWORD DXDrawCalls, DXDrawVertices, DXTexSwitches, LastTexID, DXStateChanges;
    static bool DrawPoints, DrawLines;
#ifdef DEBUG_LOD_ID
    char LodLabel[32];
    char *GetLodUsedLabel(void)
    {
        return LodLabel;
    }
#endif

#ifdef DEBUG_ENGINE
    void DrawFrameSurfaces(NodeScannerType *NODE, float Alpha = 1.0f);
    bool UseZBias;
    static IDirect3DDevice7 *m_pD3DD;
    static IDirect3D7 *m_pD3D;
    static IDirectDraw7 *m_pDD;
#endif


    // Main Object drawing function
    void FlushBuffers(void);
    void DrawObject(ObjectInstance *objInst, D3DXMATRIX *RotMatrix, const Ppoint *Pos, const float sx, const float sy, const float sz, const float scale, bool CameraSpace = false, DWORD LightID = NULL);
    void DrawBlip(ObjectInstance *objInst, D3DXMATRIX *RotMatrix, const Ppoint *Pos, const float sx, const float sy, const float sz, const float scale, bool CameraSpace);
    void Setup(IDirect3DDevice7 *pD3DD, IDirect3D7 *pD3D, IDirectDraw7 *pDD);
    void Release(void);
    void SetCamera(D3DXMATRIX *Settings, D3DVECTOR Pos, D3DXMATRIX *BB);
    void SetProjection(D3DXMATRIX *Settings)
    {
        Projection = *Settings;
        m_pD3DD->SetTransform(D3DTRANSFORMSTATE_PROJECTION, (LPD3DMATRIX)&Projection);
    }
    void SetWorld(D3DXMATRIX *Settings)
    {
        World = *Settings;
    }
    void SetViewport(DWORD l, DWORD t, DWORD r, DWORD b);
    void SetFogLevel(float FogLevel);
    void SetBlipIntensity(float Intensity)
    {
        m_BlipIntensity = Intensity;
    };
    void LinearFog(bool Status)
    {
        m_LinearFog = Status;
    }
    bool LinearFog(void)
    {
        return m_LinearFog;
    }
    void SetFogColor(D3DCOLORVALUE *Color)
    {
        m_FogColor = *Color;
    }
    void CreateZeroTexture(void);
    void SelectDDSTexture(IDirectDrawSurface7 * TexID);
    void SelectTexture(GLint texID);
    void ClearLights(void)
    {
        TheLightEngine.ResetLightsList();
    }
    DWORD SetStencilMode(DWORD Stencil);
    void ClearStencil(void)
    {
        m_pD3DD->Clear(NULL, NULL, D3DCLEAR_STENCIL, 0, 1.0f, 0);
        m_StencilRef = 0;
        SetStencilMode(STENCIL_OFF);
    }

    void SaveState(void);
    void RestoreState(void);

    // Cache/Unload Textures fuinctions
    void LoadTextures(DWORD ID);
    void UnLoadTextures(DWORD ID);

    // The Light used in Rendering
    static D3DLIGHT7 TheSun, TheNVG, TheTV;
    static D3DCOLORVALUE TheSunColour;
    void SetSunColour(float r, float g, float b)
    {
        TheSunColour.r = r;
        TheSunColour.g = g;
        TheSunColour.b = b;
    }
    void SetSunLight(float Ambient, float Diffuse, float Specular);
    void SetSunVector(D3DVECTOR Direction)
    {
        TheSun.dvDirection = Direction;
    }
    void SetLODBias(float Bias)
    {
        m_LODBiasCx = Bias;
    }
    void SetPitMode(bool Mode)
    {
        m_PitMode = Mode;
    }
    bool GetPitMode(void)
    {
        return m_PitMode;
    }
    void SetState(DX_StateType Mode)
    {
        m_RenderState = Mode;
    }
    void ResetState(void)
    {
        m_RenderState = DX_OTW;
        m_StatesStackLevel = 0;
    }
    DX_StateType GetState(void)
    {
        return m_RenderState;
    }

    static D3DMATERIAL7 TheMaterial;

#ifndef EDIT_ENGINE
private:
#endif

    // This Auto Variable is settle with Textues IDs during Drawing
    static DWORD m_TexUsed[256];
    static DWORD m_TexID, m_LastTexID;
    static DXFlagsType m_LastFlags;
    static DWORD m_LastZBias, m_LastSpecular;
    static float m_LODBiasCx;
    static ObjectInstance *m_TheObjectInstance, *m_LastObjectInstance;
    static float m_FogLevel;
    static float m_BlipIntensity;
    static float m_LinearFogLevel;
    static bool m_LinearFog;
    static D3DCOLORVALUE m_FogColor;
    // The VB of the object under Drawing
    static VBItemType m_VB;
    static NodeScannerType m_NODE;
    static TextureHandle *ZeroTex;
    static DWORD m_AlphaTextureStage;
    static StencilModeType m_StencilMode;
    static DWORD m_StencilRef;
    static bool m_PitMode;

    // Debug Flags
    bool m_bCullEnable, m_bDofMove;

    // The Drawn Model Transform state, The stack, The Stack Pointer
    static D3DXMATRIX State, DofTransformation, AppliedState;
    static D3DXMATRIX StateStack[128];
    static DWORD StateStackLevel;
    inline void PushMatrix(D3DXMATRIX *p);
    inline void PopMatrix(D3DXMATRIX *p);
    void FlushInit(void);
    void FlushObjects(void);
    void FlushBlips(void);
    inline void DrawNode(ObjectInstance *objInst, DWORD LightOwner, DWORD LodID);
    inline void DrawBlitNode(void);
#ifdef EDIT_ENGINE
    void DrawNodeEx(NodeScannerType *NODE, ObjectInstance *objInst, DWORD LightOwner, DWORD LodID);
    void ModelInit(ObjectInstance *objInst, DxDbHeader* Header, DWORD *Textures, D3DXMATRIX *State, DWORD LightOwner, DWORD nTexsPerBank);
    bool m_FrameDrawMode, m_SkipSwitch, m_ScriptsOn;
    void SetScripts(bool state)
    {
        m_ScriptsOn = state;
    }
    void DofManageEx(NodeScannerType *NODE, ObjectInstance *objInst, D3DXMATRIX *NewState);
    DWORD m_DofLevel;
    void PushMatrixEx(D3DXMATRIX *p);
    void PopMatrixEx(D3DXMATRIX *p);
    bool SwitchManageEx(NodeScannerType *NODE, ObjectInstance *objInst, D3DXMATRIX *NewState);
    bool m_EmitDefaultSpecularity;
#endif

    friend bool DXScript_Beacon(D3DVECTOR *pos, ObjectInstance *obj, DWORD *Argument);



    // The main D3DD used by the Engine
#ifndef DEBUG_ENGINE
    static IDirect3DDevice7 *m_pD3DD;
    static IDirect3D7 *m_pD3D;
    static IDirectDraw7 *m_pDD;
#endif
    //The Stacks for Surfaces
    SURFACE_STACK(m_AlphaStack, MAX_ALPHA_SURFACES);
    SURFACE_STACK(m_SolidStack, MAX_SOLID_SURFACES);
#ifdef DEBUG_ENGINE
    SURFACE_STACK(m_FrameStack, MAX_SOLID_SURFACES);
    float SelectColor, StepColor;
#endif

    DWORD PushSurface(SurfaceStackType *Stack, D3DXMATRIX *State);
    bool PopSurface(SurfaceStackType *Stack, D3DXMATRIX *State);
    bool GetSurface(DWORD Level, SurfaceStackType *Stack, D3DXMATRIX *State);
    bool PushSurfaceIntoSort(SurfaceStackType *Stack, D3DXMATRIX *State);
    void DrawSortedAlpha(DWORD Level, bool SetupMode);

    /*SurfaceStackType m_AlphaStack[MAX_ALPHA_SURFACES];
    DWORD m_AlphaStackLevel;*/

    // Secondary surface draw function
    void DrawSurface();
    void ResetFeatures(void);
    void DOFManage(void);
    void SetWORLD(D3DXMATRIX *);
    void DrawAlphaSurfaces(void);
    void DrawSolidSurfaces(void);
    void SWITCHManage(void);
    void SetNormalViewMode(void);
    void SetViewMode(void);

    float Process_DOFRot(float dofrot, int dofNumber, int flags, float min, float max, float multiplier, float unused);
    void DOF(void);
    void AssignDOFRotation(D3DXMATRIX *R);
    void AssignDOFTranslation(D3DXMATRIX *T);
    void SetRenderState(DXFlagsType Flags, DXFlagsType NewFlags, bool Enable);
    void StoreSetupState(void);

    static D3DXMATRIX CameraView, BBMatrix;
    static D3DVECTOR CameraPos, LightDir;
    static D3DXMATRIX Projection;
    static D3DXMATRIX World;
    static D3DVIEWPORT7 ViewPort;
    // The DX Engine Default State Block
    DWORD DxEngineStateHandle;
    static DX_StateType m_RenderState;
    static DWORD m_StatesStackLevel;
    static DX_StatesStackType m_StatesStack[DX_MAX_NESTED_STATES];

    ///////////////////////////////// 2D ENGINE ITEMS ////////////////////////////////////////////////////////////////

public:
    void Draw3DPoint(D3DVECTOR *WorldPos, DWORD Color, bool Emissive = false, bool CameraSpace = false);
    void Draw3DLine(D3DVECTOR *WorldStart, D3DVECTOR *WorldEnd, DWORD ColorStart, DWORD ColorEnd, bool Emissive = false,  bool CameraSpace = false);
    float GetDetailLevel(D3DVECTOR *WorldPos, float MaxRange);
    void SetupTexturesOnDevice(void);
    void LoadTexture(char *FileName);
    DWORD GetTextureHandle(char *TexName);
    CTextureItem *DX2D_GetTextureItem(char *TexName);
    void DX2D_GetTextureCoords(CTextureItem *Ti, CDrawBaseItem *Item);
    void CleanUpTexturesOnDevice(void);
    void ReleaseTextures(void);
    void DX2D_GetTextureUV(CTextureItem *Ti, DWORD Index, float &u, float &v);
    float FogLevel(void)
    {
        return m_FogLevel;
    }
    float LODBiasCx(void)
    {
        return m_LODBiasCx;
    }


    CDynamicDraw *DX2D_FindDrawItem(DWORD TexID);

    void FlushDynamicObjects(void);
    void DynamicItemFlush(CDynamicDraw *DrawItem);


    INT16 *IndexBuffer; // Buffer for Verticves Indexes
    DWORD LastIndex; // position of last stored index in the index buffer
    DWORD DynamicItems; // Number of Dynamic Items present
    static _MM_ALIGN16 XMMVector vbb0, vbb1, vbb2, vbb3; // The Vertices
    static _MM_ALIGN16 XMMVector BBvbb0, BBvbb1, BBvbb2, BBvbb3; // The Vertices used for BillBoarding
    static _MM_ALIGN16 XMMVector XMMCamera; // the Camera position compatible with XMM Math
    static _MM_ALIGN16 XMMVector BBCx[3]; // This are the XMM ordered Matrix CXes used to BB Stuff

    CDynamicDraw *DynamicDrawRoot;
    CDynamicPrimitive *DynPrimitiveList, *DynPrimitiveLast;
    CDynamicDraw *LastPassed;
    CDynamicDraw *DynDrawPool; // The Dynamic Draws Pool
    CDrawBaseItem *pDrawBasePool, *pDrawBasePoolPtr; // Pool of Draw Items

    ///////////////////////////////////////////// 2D STUFF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

public:
    DWORD ComputeSphereVisibility(LPD3DVECTOR lpCenters, LPD3DVALUE  lpRadii, DWORD dwNumSpheres = 1);
    bool DX2D_GetVisibility(D3DXVECTOR3 *Pos, float Radius, DWORD Flags = 0);
    float DX2D_GetDistance(D3DXVECTOR3 *Pos, float Radius, DWORD Flags = 0);
    float DX2D_GetDistance(D3DXVECTOR3 *Pos, DWORD Flags = 0);
    void DX2D_GetRelativePosition(D3DXVECTOR3 *Pos);
    void DX2D_NextLayer(DWORD Flags = 0)
    {
        LayerSelected++;
        Layers[LayerSelected].Flags = Flags;
    };
    void DX2D_SetLayer(DWORD Layer = LAYER_NODRAW, DWORD Flags = 0)
    {
        if (Layer == LAYER_NODRAW) return;

        LayerSelected = Layer;
        Layers[LayerSelected].Flags = Flags;
    };
    void DX2D_Reset(void);
    void DX2D_InitLists(void);
    void DX2D_AddQuad(DWORD Layer, DWORD Flags, D3DXVECTOR3 *Pos, D3DDYNVERTEX *Quad, float Radius, DWORD TexHandle);
    void DX2D_AddTri(DWORD Layer, DWORD Flags, D3DXVECTOR3 *Pos, D3DDYNVERTEX *Tri, float Radius, DWORD TexHandle);
    void DX2D_AddBi(DWORD Layer, DWORD Flags, D3DXVECTOR3 *Pos, D3DDYNVERTEX *Segment, float Radius, DWORD TexHandle);
    void DX2D_AddSingle(DWORD Layer, DWORD Flags, D3DXVECTOR3 *Pos, D3DDYNVERTEX *Segment, float Radius, DWORD TexHandle);
    void DX2D_AddPoly(DWORD Layer, DWORD Flags, D3DXVECTOR3 *Pos, D3DDYNVERTEX *Poly, float Radius, DWORD Vertices, DWORD TexHandle);
    void DX2D_SetDrawOrder(DWORD *Order);
    void DX2D_SetupSquareCx(float y, float z);
    // void DX2D_TransformBB(D3DXVECTOR3 *Pos, D3DDYNVERTEX *Coord, D3DDYNVERTEX *Dest, DWORD Nr=1);
    void DX2D_TransformBB(XMMVector *Pos, XMMVector *Coord, D3DDYNVERTEX *Dest, DWORD Nr);
    void DX2D_TransformBB(XMMVector *Pos, D3DDYNVERTEX *Vertex, DWORD Nr);
    void DX2D_MakeCameraSpace(D3DXVECTOR3 *Result, D3DXVECTOR3 *Pos);
    void DX2D_ForceDistance(float Distance);
    void DX2D_AddObject(DWORD ID, DWORD Layer, SurfaceStackType *Stack, D3DXVECTOR3 *Pos);
    void DX2D_SetViewMode(void);

private:
    void DX2D_Flush2DObjects(void);
    void DX2D_Init(void);
    void DX2D_Release(void);
    DWORD DX2D_GenerateIndexes(DWORD Start);
    DWORD DX2D_SortIndexes(DWORD Start);
    void DX2D_AssignLayers(void);
    inline bool CheckBufferSpace(DWORD VbIndex, DWORD Size);

    static DWORD LayerSelected, Total2DVertices, Total2DItems, VBSelected;
    static float Radius2D, TestDistance;
    static LayerItemType Layers[MAX_2D_LAYERS];
    static DrawItemType Draws2D[MAX_2D_ITEMS];
    static Dyn2DBufferType CDXEngine::Dyn2DVertexBuffer[MAX_2D_BUFFERS];
    static unsigned short DrawIndexes[MAX_VERTICES_PER_DRAW + 32];
    static SortItemType SortBuffer[MAX_2D_ITEMS];
    static DWORD SortBuckets[4][256];
    static DWORD SortTail[4][256];
    static DWORD DrawOrder[MAX_2D_LAYERS], Indexed2D;
    static DWORD IndexStart;


    ////////////////////////////////////// LIGHT ENGINE ITEMS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

public:
    void DrawOwnSpot(Trotation *); // Draw the observer SpotLight...

private:
    D3DLIGHT7 DXLightsList[MAX_DYNAMIC_LIGHTS]; // The Dynamic Lights List
    DWORD LightsNumber; // The number of Valid Lights in the List
    void AddDynamicLight(D3DLIGHT7 *Light, D3DXMATRIX *RotMatrix, D3DVECTOR *Pos);
    void RemoveDynamicLights(void);
    DWORD LightID;
    CTextureSurface *TexturesList;

};

extern CDXEngine TheDXEngine;
extern bool g_Use_DX_Engine;

