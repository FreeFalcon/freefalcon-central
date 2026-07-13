#include <cISO646>
#include "time.h"
#include <math.h>
#include "../include/ObjectInstance.h"
#include "dxdefines.h"
#include "DXVBManager.h"
#include "mmsystem.h"
#include "../include/TexBank.h"
#ifndef DEBUG_ENGINE
#include "../include/realweather.h"
#endif
#include "dxengine.h"
#include "../include/ObjectLOD.h"
#include "DXTools.h"
#include "../include/Tod.h"
#include "../../falclib/include/Fakerand.h"
#include "../../include/ComSup.h"
#include "common/IRenderer.h"	// PHASE 4: D3D11 object path
#include "OpenXRBackend.h"   // temp VR stereo diag
#include <stdio.h>
extern bool g_bUseGpu;   // #DX12 п.4: GPU mode (D3D11 || D3D12) -- the object pass runs on the active renderer

// #34: world matrix -> the shader cbObject (D3D11). The dead D3D7 m_pD3DD->SetTransform else-branch
// was removed.
#define DX_SET_WORLD(M) do { \
    if (g_pRenderer) g_pRenderer->SetWorld((const float *)&(M)); \
} while (0)

// This variable is the Model ID presently under draw
DWORD gDebugLodID;

extern bool g_bGreyMFD;
extern bool bNVGmode;
extern int TheObjectLODsCount;

#ifdef DEBUG_LOD_ID
extern char TheLODNames[10000][32];
#endif

// ********************************** STATIC GLOBAL VARIABLES ***************************************

CDXEngine TheDXEngine;

// #34 C1: CDXEngine D3D7 device statics (m_pD3DD/m_pD3D/m_pDD) removed.

D3DXMATRIX CDXEngine::State, CDXEngine::DofTransformation, CDXEngine::AppliedState;
DWORD CDXEngine::StateStackLevel;
D3DXMATRIX CDXEngine::CameraView;
D3DXMATRIX CDXEngine::BBMatrix;
D3DVECTOR CDXEngine::CameraPos;
D3DVECTOR CDXEngine::LightDir;
D3DXMATRIX CDXEngine::Projection;
D3DXMATRIX CDXEngine::World;

// #28: current-frame sun+ambient -- for per-object dynamic lighting (UpdateDynamicLights
// in dxlightengine.cpp builds the 'sun + nearby dynamic lamps' set and calls SetLights).
GpuLightCPU g_d3d11Sun = {};
float g_d3d11Amb[4] = { 0.45f, 0.45f, 0.45f, 1.0f };
D3DVIEWPORT7 CDXEngine::ViewPort;
_MM_ALIGN16 XMMVector CDXEngine::XMMCamera; // the Camera position compatible with XMM Math
DWORD CDXEngine::m_TexID, CDXEngine::m_LastTexID;
DXFlagsType CDXEngine::m_LastFlags;
DWORD CDXEngine::m_LastZBias;
float CDXEngine::m_LODBiasCx;
D3DMATERIAL7 CDXEngine::TheMaterial;
D3DXMATRIX CDXEngine::StateStack[128];
DWORD CDXEngine::m_TexUsed[256];
DWORD CDXEngine::m_LastSpecular;
float CDXEngine::m_FogLevel;
float CDXEngine::m_BlipIntensity;
float CDXEngine::m_LinearFogLevel;
D3DCOLORVALUE CDXEngine::m_FogColor;
DWORD CDXEngine::m_AlphaTextureStage;

D3DLIGHT7 CDXEngine::TheSun, CDXEngine::TheNVG, CDXEngine::TheTV;
D3DCOLORVALUE CDXEngine::TheSunColour;

SurfaceStackType CDXEngine::m_AlphaStack;
SurfaceStackType CDXEngine::m_SolidStack;
#ifdef DEBUG_ENGINE
SurfaceStackType CDXEngine::m_FrameStack;
#endif
bool CDXEngine::DrawPoints, CDXEngine::DrawLines;
bool CDXEngine::m_LinearFog;
VBItemType CDXEngine::m_VB;
NodeScannerType CDXEngine::m_NODE;
ObjectInstance *CDXEngine::m_TheObjectInstance;
ObjectInstance *CDXEngine::m_LastObjectInstance;
TextureHandle *CDXEngine::ZeroTex;
StencilModeType CDXEngine::m_StencilMode;
DWORD CDXEngine::m_StencilRef;
bool CDXEngine::m_PitMode;

DX_StateType CDXEngine::m_RenderState;
DWORD CDXEngine::m_StatesStackLevel;
DX_StatesStackType CDXEngine::m_StatesStack[DX_MAX_NESTED_STATES];

#ifdef DATE_PROTECTION
bool DateOff = true;
#define PROTECTION_MONTH 1
#define PROTECTION_YEAR 2008
#endif

// ********************************* THIS SECTION IS THE REAL ENGINE ********************************
CDXEngine::CDXEngine(void)
{
    DxEngineStateHandle = NULL;
    ZeroTex = NULL;
    TexturesList = NULL;
    m_LinearFog = false;
    m_StatesStackLevel = 0;
    m_RenderState = DX_OTW;

#ifdef DATE_PROTECTION

    time_t t;
    struct tm *today;

    t = time(NULL);
    today = localtime(&t);

    if (today->tm_mon > PROTECTION_MONTH or today->tm_year > PROTECTION_YEAR)
        DateOff = true;
    else
        DateOff = false;


#endif
}

CDXEngine::~CDXEngine(void)
{
    CleanUpTexturesOnDevice();
    ReleaseTextures();
	// #34: DxEngineStateHandle is never set under D3D11 (StoreSetupState is a no-op); dead D3D7
	// DeleteStateBlock removed.
}

// The Default engine states for the renderer
// This state must be sampled at D3DD CREATION PHASE, to keep it independent by following
// BSP engine state changes
void CDXEngine::StoreSetupState(void)
{
    // #34 D3D11: D3D7 state-block save/restore replaced by D3D11 state objects (FFStateMap); no-op.
}

void CDXEngine::SetFogLevel(float FogLevel)
{
    m_FogLevel = m_LinearFog ? 1.0f : FogLevel;
}


void CDXEngine::SetCamera(D3DXMATRIX *Settings, D3DVECTOR Pos, D3DXMATRIX *BB)
{
    CameraView = *Settings;
    CameraPos = Pos;
#ifdef EDIT_ENGINE
    CameraView.m30 = CameraPos.x;
    CameraView.m31 = CameraPos.y;
    CameraView.m32 = CameraPos.z;
#endif
    // #34 D3D11: view matrix into the shader cbuffer (dead D3D7 SetTransform else removed)
    if (g_pRenderer) g_pRenderer->SetView((const float *)&CameraView);

    // The BB Stuff
    BBMatrix = *BB;
    BBCx[0].d3d.x = BB->m00, BBCx[0].d3d.y = BB->m10, BBCx[0].d3d.z = BB->m20, BBCx[0].d3d.Flags.Word = 0;
    BBCx[1].d3d.x = BB->m01, BBCx[1].d3d.y = BB->m11, BBCx[1].d3d.z = BB->m21, BBCx[1].d3d.Flags.Word = 0;
    BBCx[2].d3d.x = BB->m02, BBCx[2].d3d.y = BB->m12, BBCx[2].d3d.z = BB->m22, BBCx[2].d3d.Flags.Word = 0;

    // set the XMM Camera
    *((D3DVECTOR*)&XMMCamera.d3d) = Pos;
}



VOID CDXEngine::SelectTexture(GLint texID)
{
    // eventually select other textures for NVG/TV

    // Artscout - 2026 (x64): texID is a small bank index, but the handle/SRV it resolves to are
    // pointer-sized. Use a DWORD_PTR local so the pointer isn't truncated (GLint dropped the high 32 bits).
    DWORD_PTR h = (texID not_eq -1) ? TheTextureBank.GetHandle(texID) : (DWORD_PTR)ZeroTex;

    if (h) h = (DWORD_PTR)((TextureHandle *)h)->m_pDDS;

    if (g_bUseGpu)	// PHASE 4/#DX12: m_pDDS holds the GPU texture handle (D3D11 SRV or D3D12Texture*)
    {
        if (g_pRenderer)
            g_pRenderer->SetTexture(0, (struct ID3D11ShaderResourceView *)h);
        return;
    }
    // #34 dead D3D7 SetTexture stages removed (D3D11 returns above)
}





// The View Port setting function
// The passed parameters are in Screen Pixels
void CDXEngine::SetViewport(DWORD l, DWORD t, DWORD r, DWORD b)
{
    ViewPort.dwX = l;
    ViewPort.dwY = t;
    ViewPort.dwWidth = r - l;
    ViewPort.dwHeight = b - t;
    ViewPort.dvMinZ = 0.0f;
    ViewPort.dvMaxZ = 1.0f;
}


// The Engine initialization Function
void CDXEngine::Setup()
{
    // #34 C1: no D3D7 device to store.
    m_LastFlags.w = 0;
    m_TexID = m_LastTexID = -1;
    INIT_S_STACK(m_AlphaStack, MAX_ALPHA_SURFACES);
    INIT_S_STACK(m_SolidStack, MAX_SOLID_SURFACES);
#ifdef DEBUG_ENGINE
    INIT_S_STACK(m_FrameStack, 5000);
#endif
    m_bCullEnable = true;
    m_bDofMove = false;
    m_LastFlags.w = 0;
#ifdef DEBUG_ENGINE
    UseZBias = true;
#endif

    // Initializes the 2D Engine
    DX2D_Init();

    // Initialize the Light engine
    TheLightEngine.Setup();   // #34 C1: D3D7 device args removed

    ZeroMemory(&TheMaterial, sizeof(TheMaterial));
    TheMaterial.ambient.r = TheMaterial.ambient.g = TheMaterial.ambient.b = 1.0f;
    TheMaterial.diffuse.r = TheMaterial.diffuse.g = TheMaterial.diffuse.b = 1.0f;
    TheMaterial.specular.r = TheMaterial.specular.g = TheMaterial.specular.b = 1.0f;
    TheMaterial.dvPower = 6.8f;

    /////////// Initializes the Environmental Light Object to DEFAULT VALUES ///////////////////////
    ZeroMemory(&TheSun, sizeof(TheSun));
    TheSun.dltType = D3DLIGHT_DIRECTIONAL;
    TheSun.dcvAmbient.r = TheSun.dcvAmbient.g = TheSun.dcvAmbient.b = 1.0f;
    TheSun.dcvDiffuse.r = TheSun.dcvDiffuse.g = TheSun.dcvDiffuse.b = 1.0f;
    TheSun.dcvSpecular.r = TheSun.dcvSpecular.g = TheSun.dcvSpecular.b = 1.0f;
    TheSunColour.r = TheSunColour.g = TheSunColour.b = 1.0f;
    ////////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////// The NVG Mode used Light ///////////////////////////////////////
    ZeroMemory(&TheNVG, sizeof(TheNVG));
    TheNVG.dltType = D3DLIGHT_DIRECTIONAL;
    TheNVG.dcvAmbient.r = 0.52f;
    TheNVG.dcvAmbient.g = 0.52f;
    TheNVG.dcvAmbient.b = 0.52f;

    TheNVG.dcvDiffuse.r = 0.04f;
    TheNVG.dcvDiffuse.g = 0.04f;
    TheNVG.dcvDiffuse.b = 0.04f;

    TheNVG.dcvSpecular.r = 0.05f;
    TheNVG.dcvSpecular.g = 0.05f;
    TheNVG.dcvSpecular.b = 0.05f;

    ////////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////// The TV/IR Mode used Light ///////////////////////////////////////
    ZeroMemory(&TheTV, sizeof(TheTV));
    TheTV.dltType = D3DLIGHT_DIRECTIONAL;
    TheTV.dcvAmbient.r = 0.52f;
    TheTV.dcvAmbient.g = 0.52f;
    TheTV.dcvAmbient.b = 0.52f;

    TheTV.dcvDiffuse.r = 0.04f;
    TheTV.dcvDiffuse.g = 0.04f;
    TheTV.dcvDiffuse.b = 0.04f;

    TheTV.dcvSpecular.r = 0.05f;
    TheTV.dcvSpecular.g = 0.05f;
    TheTV.dcvSpecular.b = 0.05f;

    ////////////////////////////////////////////////////////////////////////////////////////////////


    // No Light added for now...
    LightsNumber = 0;

#ifdef EDIT_ENGINE
    m_FrameDrawMode = false;
    m_ScriptsOn = false;
#endif

    // Store the SETUP STATE for the renderer
    //Thsi state has to be stored at D3DD creation phase
    StoreSetupState();
    m_LinearFog = false;


}


// **** CREATION OF THE ZERO TEXTURE - SUCH TEXTURE IS USED BY TEXTURE STAGES IN PRESENCE OF UNTEXTURE4S SURFACES,
// TO RENDER THE APPROPRIATE WAY THE NVG VIEW
void CDXEngine::CreateZeroTexture(void)
{
    // Create the Zero Texture
    ZeroTex = new TextureHandle();
    ZeroTex->Create("", 0, 32, 64, 64, TextureHandle::FLAG_MATCHPRIMARY);

    // PHASE 5: in D3D11 fill ZeroTex with a real WHITE texture (previously skipped ->
    // m_pDDS=NULL -> polygons with texID=-1 sampled nothing -> white/broken). ZeroTex is needed
    // as a neutral white texture for untextured polygons (result = white * vertexcolor).
    extern bool g_bUseD3D12;
    if (g_bUseD3D12)   // #DX12: bake via Load (no-op stub under D3D12); skip the dead DDraw Blt path
    {
        static DWORD s_white[64 * 64];
        for (int i = 0; i < 64 * 64; ++i) s_white[i] = 0xFFFFFFFF;
        ZeroTex->Load(0, 0, (BYTE*)s_white);   // bakes a white 64x64 -> valid SRV
        return;
    }

    // Artscout - 2026: [DX7-PURGE] DDraw surface GetPixelFormat/Blt colour-fill removed
    // (GPU path above bakes ZeroTex white via Load and returns).
}




void CDXEngine::Release(void)
{
    // #34 C1: no D3D7 device / state block to release.
    // Release the 2D Engine items
    DX2D_Release();

    // Release the Zero Texture
    if (ZeroTex)
    {
        delete ZeroTex;
        ZeroTex = NULL;
    }

}


// Setup the Environmental light properties
void CDXEngine::SetSunLight(float Ambient, float Diffuse, float Specular)
{
    TheSun.dcvAmbient.r = TheSunColour.r * Ambient;
    TheSun.dcvAmbient.g = TheSunColour.g * Ambient;
    TheSun.dcvAmbient.b = TheSunColour.b * Ambient;

    TheSun.dcvDiffuse.r = TheSunColour.r * Diffuse;
    TheSun.dcvDiffuse.g = TheSunColour.g * Diffuse;
    TheSun.dcvDiffuse.b = TheSunColour.b * Diffuse;

    TheSun.dcvSpecular.r = TheSunColour.r * Specular;
    TheSun.dcvSpecular.g = TheSunColour.g * Specular;
    TheSun.dcvSpecular.b = TheSunColour.b * Specular;

#ifndef DEBUG_ENGINE
    TheTimeOfDay.GetLightDirection((Tpoint*)&LightDir);
    LightDir.x = -LightDir.x ;
    LightDir.y = -LightDir.y ;
    LightDir.z = -LightDir.z ;
#endif

    // PHASE 6: port the directional sun light to D3D11 (object path, VS_Object
    // computes col = dwColour * saturate(ambient + sum N.L)). One directional source
    // (sun) + TOD ambient. Previously SetLights was not called -> FF_LIGHTING was off.
    if (g_bUseGpu and g_pRenderer)
    {
        float amb[4] = { TheSun.dcvAmbient.r, TheSun.dcvAmbient.g, TheSun.dcvAmbient.b, 1.0f };
        GpuLightCPU sun;
        memset(&sun, 0, sizeof(sun));
        // -LightDir: the shader takes Ldir = -L.Direction; LightDir is already 'toward the sun' (reference negates
        // GetLightDirection). Consistent with FlushBuffers (the effective path). Previously this was
        // +LightDir (backwards), but the call was overwritten by FlushBuffers -- unify them.
        sun.Direction[0] = -LightDir.x;  sun.Direction[1] = -LightDir.y;  sun.Direction[2] = -LightDir.z;
        sun.Color[0] = TheSun.dcvDiffuse.r; sun.Color[1] = TheSun.dcvDiffuse.g; sun.Color[2] = TheSun.dcvDiffuse.b;
        sun.Params[1] = 0.0f;   // directional
        g_pRenderer->SetLights(amb, 1, &sun, sizeof(sun));
    }
}




void CDXEngine::EnableCull(bool Status)
{
    m_bCullEnable = Status;
}

void CDXEngine::MoveDof(bool Status)
{
    m_bDofMove = Status;
}


extern DWORD gDebugTextureID;

// * Function referencing and loading textures given an Object Instance *
void CDXEngine::LoadTextures(DWORD ID)
{

    // Fetch the VB Data of this Model
    VBItemType VB;
    TheVbManager.GetModelData(VB, ID);

    gDebugLodID = ID;

    // Get the Textures Offsets
    DWORD *texOffset = VB.Texs;

    // Register each texture for the Model ( and load it if not available ) and setup local Textures List
    for (DWORD a = 0; a < VB.NTex; a++)
    {
#ifndef DEBUG_ENGINE
        gDebugTextureID = *texOffset;
#endif
        TheTextureBank.Reference(*texOffset++);
    }

    gDebugLodID = -1;
#ifndef DEBUG_ENGINE
    gDebugTextureID = -1;
#endif
}


// * Function Dereferencing textures given an Object Instance *
void CDXEngine::UnLoadTextures(DWORD ID)
{

    // Fetch the VB Data of this Model
    VBItemType VB;

    TheVbManager.GetModelData(VB, ID);

    // Consistency check
    if ( not VB.Valid) return;

    // Get the Textures Offsets
    DWORD *texOffset = VB.Texs;


    // DeRegister each texture for the Model
    for (DWORD a = 0; a < VB.NTex; a++) TheTextureBank.Release(*texOffset++);

}


// Stenciling Functions
DWORD CDXEngine::SetStencilMode(DWORD Stencil)
{
    DWORD LastMode = (DWORD)m_StencilMode;

    // #34 D3D11: 3D-cockpit stencil mask via D3D11 state objects (dead D3D7 switch removed).
    m_StencilMode = (StencilModeType)Stencil;
    if (g_pRenderer)
    {
        switch (Stencil)
        {
        case STENCIL_WRITE:
            m_StencilRef++;
            g_pRenderer->SetStencil(2, m_StencilRef);   // cockpit writes ref
            break;
        case STENCIL_CHECK:
            if (m_StencilRef) g_pRenderer->SetStencil(3, m_StencilRef); // world: ref>stencil
            else              g_pRenderer->SetStencil(0, 0);            // ref==0 -> ALWAYS
            break;
        case STENCIL_OFF:
        default:
            g_pRenderer->SetStencil(0, 0);
            break;
        }
    }
    return LastMode;
}


// * This Function just resets any Feature/lag fro a drawing
void CDXEngine::ResetFeatures(void)
{
    m_LastFlags.w = 0xffffffff;
    DXFlagsType Spare;
    Spare.w = 0x00;
    SetRenderState(m_LastFlags, Spare, DISABLE);
    m_LastFlags.w = 0;

    SelectTexture(-1);
    m_TexID = -1;
    LastTexID = 0xcccccccc;

}





// ********************* SURFACES STACK MANAGEMENT ***************************
// function Pushing in a surface in Surface stack
DWORD CDXEngine::PushSurface(SurfaceStackType *Stack, D3DXMATRIX *State)
{
    DWORD Level = Stack->StackLevel;

    // if enough space Stores the surface Data
    if (Stack->StackLevel < Stack->StackMax)
    {
        DWORD l = Stack->StackLevel++;
        Stack->Stack[l].Vb = m_VB;
        Stack->Stack[l].State = *State;
        Stack->Stack[l].Surface = m_NODE.BYTE;
        Stack->Stack[l].TexID = m_TexID;
        Stack->Stack[l].ObjInst = m_TheObjectInstance;
        Stack->Stack[l].FogLevel = m_FogLevel;
        memcpy(Stack->Stack[l].LightMap, TheLightEngine.LightsToOn, sizeof(Stack->Stack[l].LightMap));
    }

    return Level;
}


// Function pushing a surface into stack, and putting it into sorting lop
bool CDXEngine::PushSurfaceIntoSort(SurfaceStackType *Stack, D3DXMATRIX *State)
{
    DWORD Level;

    Level = PushSurface(Stack, State);
    D3DXVECTOR3 Pos;
    Pos.x = State->m30, Pos.y = State->m31, Pos.z = State->m32;
    DX2D_AddObject(Level, LAYER_AUTO, Stack, &Pos);
    return true;
}



// function Popping out a surface from Surface stack
bool CDXEngine::PopSurface(SurfaceStackType *Stack, D3DXMATRIX *State)
{
    // if stack not empty the assign variables with stacked data
    if (Stack->StackLevel)
    {
        DWORD l = --Stack->StackLevel;
        m_VB = Stack->Stack[l].Vb;
        *State = Stack->Stack[l].State;
        m_NODE.BYTE = Stack->Stack[l].Surface;
        m_TexID = Stack->Stack[l].TexID;
        m_TheObjectInstance = Stack->Stack[l].ObjInst;
        m_FogLevel = Stack->Stack[l].FogLevel;
        memcpy(TheLightEngine.LightsToOn, Stack->Stack[l].LightMap, sizeof(TheLightEngine.LightsToOn));
        return true;
    }

    return false;
}



// function Getting out a surface from Surface stack
bool CDXEngine::GetSurface(DWORD Level, SurfaceStackType *Stack, D3DXMATRIX *State)
{
    // if stack not empty the assign variables with stacked data
    if (Level < Stack->StackLevel)
    {
        m_VB = Stack->Stack[Level].Vb;
        *State = Stack->Stack[Level].State;
        m_NODE.BYTE = Stack->Stack[Level].Surface;
        m_TexID = Stack->Stack[Level].TexID;
        m_TheObjectInstance = Stack->Stack[Level].ObjInst;
        m_FogLevel = Stack->Stack[Level].FogLevel;
        memcpy(TheLightEngine.LightsToOn, Stack->Stack[Level].LightMap, sizeof(TheLightEngine.LightsToOn));
        return true;
    }

    return false;
}


// Function pushing the DX render state into the states stack
void CDXEngine::SaveState(void)
{
    m_StatesStack[m_StatesStackLevel].RenderState = m_RenderState;

    if (m_StatesStackLevel < DX_MAX_NESTED_STATES) m_StatesStackLevel++;
}


void CDXEngine::RestoreState(void)
{
    if (m_StatesStackLevel)
    {
        m_RenderState = m_StatesStack[--m_StatesStackLevel].RenderState;
    }
    else m_RenderState = DX_OTW;
}


// funcion pushing in the Matrix Stack a Matrix
inline void CDXEngine::PushMatrix(D3DXMATRIX *p)
{
    StateStack[StateStackLevel] = *p;
    StateStackLevel++;
}

// funcion popping out the Matrix Stack a Matrix
inline void CDXEngine::PopMatrix(D3DXMATRIX *p)
{
    if (StateStackLevel) *p = StateStack[--StateStackLevel];
}



// Function Selecting Normal View Mode, no NVG, no TV
void CDXEngine::SetViewMode(void)
{
    // #34 D3D11: texture stages / NVG/TV modes are emulated by the FFEmu shader.
    m_AlphaTextureStage = 0;
}





// Function switching the renderer State
void CDXEngine::SetRenderState(DXFlagsType Flags, DXFlagsType NewFlags, bool Enable)
{
    // #34 D3D11: per-surface render flags are D3D11 state objects (FFStateMap); no-op.
}



// *************************************** DRAW SECTION **********************************************

extern DWORD VCounter;
DWORD D3DErroCount;


// ********************************
// * the SURFACE DRAWING Function *
// ********************************
void CDXEngine::DrawSurface()
{
#ifdef DEBUG_ENGINE
    DXDrawCalls++;
    DXDrawVertices += m_NODE.SURFACE->dwVCount;
#endif


    DXFlagsType NewFlags;
    NewFlags.w = m_NODE.SURFACE->dwFlags.w;

    // #34: dead D3D7 emissive-source SetRenderState removed (D3D11 emissive via FF_EMISSIVE shader, #49).


    ////////////////////// Test if any change in rendering mode /////////////
    //if(NewFlags.StateFlags not_eq m_LastFlags.StateFlags){

#ifdef DEBUG_ENGINE
    DXStateChanges++;
#endif

    // Selects changed Flags
    DXFlagsType ChangedFlags, DisabledFlags, EnabledFlags;
    ChangedFlags.w = m_LastFlags.w xor NewFlags.w;
    DisabledFlags.w = ChangedFlags.w bitand (compl NewFlags.w);
    EnabledFlags.w = ChangedFlags.w bitand NewFlags.w;



    // Check for changes in lags affecting RENDERER STATE
    if (DisabledFlags.StateFlags) SetRenderState(DisabledFlags, NewFlags, DISABLE);

    /*if(EnabledFlags.StateFlags)*/
    SetRenderState(NewFlags, NewFlags, ENABLE);

    m_LastFlags.w = NewFlags.w;
    //}


    /////////////////////// TEXTURE CHANGE Feature //////////////////////////
    if (m_TexID not_eq LastTexID)
    {
        SelectTexture(m_TexID);
#ifdef DEBUG_ENGINE
        DXTexSwitches++;
#endif
        LastTexID = m_TexID;
    }




    ////////////////////// ZBIAS Checking done every time ////////////////////
#ifdef DEBUG_ENGINE

    if (UseZBias and m_LastZBias not_eq m_NODE.SURFACE->dwzBias)
    {
        m_LastZBias = m_NODE.SURFACE->dwzBias;
        m_pD3DD->SetRenderState(D3DRENDERSTATE_ZBIAS, m_LastZBias);
    }

#else

    if (m_LastZBias not_eq m_NODE.SURFACE->dwzBias)
    {
        m_LastZBias = m_NODE.SURFACE->dwzBias;
        // #34: dead D3D7 ZBIAS removed (no device under D3D11)
    }

#endif


    ///////////////// Bill Boarded Surfaces Management - START //////////////
    if (NewFlags.b.BillBoard)
    {
        // Apply the BillBoard Transformation
        D3DXMATRIX R = BBMatrix;
        R.m30 = AppliedState.m30;
        R.m31 = AppliedState.m31;
        R.m32 = AppliedState.m32;
        R.m33 = 1.0f;
        DX_SET_WORLD(R);
    }


    ////////////////////// Surface SPECULARITY  management ///////////////////////////
    if (TheMaterial.power not_eq m_NODE.SURFACE->SpecularIndex or m_LastSpecular not_eq m_NODE.SURFACE->DefaultSpecularity)
    {
        TheMaterial.power = m_NODE.SURFACE->SpecularIndex;
        m_LastSpecular = m_NODE.SURFACE->DefaultSpecularity;
        TheMaterial.dcvSpecular.r = (float)((m_LastSpecular >> 16) bitand 0xff) / 255.0f;
        TheMaterial.dcvSpecular.g = (float)((m_LastSpecular >> 8) bitand 0xff) / 255.0f;
        TheMaterial.dcvSpecular.b = (float)(m_LastSpecular bitand 0xff) / 255.0f;
        // #34: dead D3D7 SetMaterial removed (D3D11 material via shader, #29)
        // #29 D3D11: surface specular -> shader (Blinn-Phong from light 0). power=SpecularIndex,
        // color=dcvSpecular (from DefaultSpecularity). power=0 or color=0 -> no highlight.
        if (g_pRenderer)
            g_pRenderer->SetMaterialSpecular(TheMaterial.dcvSpecular.r, TheMaterial.dcvSpecular.g,
                                                  TheMaterial.dcvSpecular.b, (float)m_NODE.SURFACE->SpecularIndex);
    }



#ifdef EDIT_ENGINE

    /////////////////////////////////THIS IS THE EDIT ENGINE CALL \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

    CheckHR(m_pD3DD->DrawPrimitive(m_NODE.SURFACE->dwPrimType, D3DFVF_MANAGED, m_NODE.BYTE + sizeof(DxSurfaceType), m_NODE.SURFACE->dwVCount, 0));

    //////////////////////////////////// END EDIT ENGINE CALL \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

#else

    //////////////////////////////////THIS IS THE GAME ENGINE CALL \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

    HRESULT hr;

    ///////////////////////// Draw the Primitive /////////////////////////////////
#ifdef INDEXED_MODE_ENGINE

    if (g_bUseGpu)
    {
        // PHASE 4/#DX12 п.4: draw from the per-model GPU mirror VB (D3D11 buffer or D3D12 resource).
        hr = 0;
        extern bool g_bUseD3D12;
        void* vbh = g_bUseD3D12 ? m_VB.VbD3D12 : (void*)m_VB.VbD3D11;
        if (g_pRenderer and vbh)
        {
            // per-model buffer: vertices from 0, baseVertex=0, indices 0-based as is.
            void *idxPtr = m_NODE.BYTE + sizeof(DxSurfaceType);

            // Alpha-test (chroma cutout) -- strictly like D3D7: enable ONLY for
            // surfaces with the ChromaKey flag (see context.cpp/SetRenderState,
            // ALPHATESTENABLE is set only in the MPR_SE_CHROMA branch). Opaque
            // surfaces draw without cutout (dark texture RGB, even at alpha=0).
            g_pRenderer->SetAlphaTestEnabled(m_NODE.SURFACE->dwFlags.b.ChromaKey != 0);

            // #49 self-illuminated surfaces (D3D7 SwEmissive: afterburner cone, nav/formation
            // lights). D3D7 keeps the emissive (COLOR2) source on these UNLESS their switch is
            // off (then EMISSIVEMATERIALSOURCE -> MATERIAL = no glow). The D3D11 object shader
            // had dropped emissive entirely, so the afterburner plume went dark at dusk/night.
            // Mirror the D3D7 rule and flag only SwEmissive surfaces (panels stay light-shaded).
            bool afterburner = false;   // #49 hoisted: also used to wrap the draw in additive blend
            {
                bool emissive = false;

                if (NewFlags.b.SwEmissive)
                {
                    if (m_TheObjectInstance->SwitchValues)
                        emissive = (m_TheObjectInstance->SwitchValues[m_NODE.SURFACE->SwitchNumber]
                                    & m_NODE.SURFACE->SwitchMask) != 0;
                    else
                        emissive = true;   // no switch table -> D3D7 default keeps COLOR2 (glow)
                }

                g_pRenderer->SetEmissive(emissive);

                // #49 afterburner cone: among emissive surfaces, the AB cone is the one whose
                // switch is COMP_AB (0) / COMP_AB2 (30) -- exterior lights use other switch numbers
                // (tail strobe 7, nav 8, land 9). Flag it so the shader applies the warm flame
                // gradient (reference real_af.png) instead of the model's stylized blue emissive.
                // Require the Alpha flag: the AB cone is an Alpha surface (drawn in the alpha pass).
                // Restricting to Alpha keeps the additive blend flip INSIDE the alpha pass, where
                // restoring BLEND_ALPHA is correct. Without this, an opaque emissive surface with
                // switch 0 in the SOLID pass would leave alpha-blend + no-depth-write set for the
                // rest of the pass -> the whole aircraft turned translucent (interior showed through).
                afterburner = emissive
                              && NewFlags.b.Alpha
                              && (m_NODE.SURFACE->SwitchNumber == 0       // COMP_AB
                                  || m_NODE.SURFACE->SwitchNumber == 30);  // COMP_AB2
                g_pRenderer->SetAfterburner(afterburner);
            }

            // #49 the afterburner cone is an Alpha surface (drawn in the alpha pass, BLEND_ALPHA ->
            // translucent/dull). Flip it to pure additive so it glows bright (then restore alpha
            // for the surrounding translucent surfaces e.g. canopy glass).
            if (afterburner)
                g_pRenderer->SetObjectAdditiveBlend(true);

            if (m_NODE.SURFACE->dwPrimType == D3DPT_POINTLIST)
                g_pRenderer->DrawObjectStrip(m_NODE.SURFACE->dwPrimType, vbh, VERTEX_STRIDE,
                                                  (int)((DWORD) * ((Int16*)idxPtr)),
                                                  (int)m_NODE.SURFACE->dwVCount);
            else
                g_pRenderer->DrawObjectIndexed(m_NODE.SURFACE->dwPrimType, vbh, VERTEX_STRIDE,
                                                    0, (unsigned short*)idxPtr,
                                                    (int)m_NODE.SURFACE->dwVCount);

            if (afterburner)
                g_pRenderer->SetObjectAdditiveBlend(false);   // restore alpha-pass blend
        }
    }
    // #34: dead D3D7 DrawPrimitiveVB/DrawIndexedPrimitiveVB else-branches removed (D3D11 draws above)


#else
    CheckHR(m_pD3DD->DrawPrimitiveVB(m_NODE.SURFACE->dwPrimType, m_VB.Vb, (DWORD) * ((Int16*)(m_NODE.BYTE + sizeof(DxSurfaceType))) + m_VB.BaseOffset,
                                     m_NODE.SURFACE->dwVCount, 0));
#endif

    //////////////////////////////////// END GAME ENGINE CALL \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

#endif


#ifdef STAT_DX_ENGINE
    VCounter += m_NODE.SURFACE->dwVCount;
    COUNT_PROFILE("*** DX Draws ");

    if (hr) COUNT_PROFILE("*** DX ERRORS ");

#endif


    ///////////////// Bill Boarded Surfaces Management - END ////////////////////
    if (NewFlags.b.BillBoard)
    {
        // Get back to original transformation
        DX_SET_WORLD(AppliedState);
    }

}




// ********************************
// * DOF Process as in FreeFalcon code*
// ********************************

float CDXEngine::Process_DOFRot(float dofrot, int dofNumber, int flags, float min, float max, float multiplier, float unused)
{
    // Negated DOF
    if (flags bitand XDOF_NEGATE) dofrot = -dofrot;

    // DOF Limits
    if (flags bitand XDOF_MINMAX)
    {
        if (dofrot < min) dofrot = min;

        if (dofrot > max) dofrot = max;
    }

    // Scaled 0-1 DOF
    if (flags bitand XDOF_SUBRANGE and min not_eq max)
    {
        dofrot -= min;
        dofrot /= max - min;

        // Angular DOF
        if (flags bitand XDOF_ISDOF) dofrot *= (float)(3.14159 / 180.0);
    }

    // Final Scaling
    return(dofrot *= multiplier);
}




void CDXEngine::AssignDOFRotation(D3DXMATRIX *R)
{
    float DofRot;

    // ************ NORMAL ROTATION DOF **************
    if (m_NODE.DOF->Type == ROTATE)
    {
        DofRot = m_TheObjectInstance->DOFValues[m_NODE.DOF->dofNumber].rotation;
        // Apply DOF Rotation on X axis
        D3DXMatrixRotationX(R, DofRot);
        // Apply DOF transformation
        D3DXMatrixMultiply(R, R, &m_NODE.DOF->rotation);
    }




    // ************ EXTENDED ROTATION DOF **************
    if (m_NODE.DOF->Type == XROTATE)
    {
        DofRot = Process_DOFRot(m_TheObjectInstance->DOFValues[m_NODE.DOF->dofNumber].rotation, m_NODE.DOF->dofNumber, m_NODE.DOF->flags,
                                m_NODE.DOF->min, m_NODE.DOF->max, m_NODE.DOF->multiplier, m_NODE.DOF->future);
        // Apply DOF Rotation on X axis
        D3DXMatrixRotationX(R, DofRot);
        // Apply DOF transformation
        D3DXMatrixMultiply(R, R, &m_NODE.DOF->rotation);
    }



    // ************ TRANSLATION DOF - NO ROTATION ******
    if (m_NODE.DOF->Type == TRANSLATE) D3DXMatrixIdentity(R);



    // *** SCALING DOF - ROTATION MATRIX USED TO SCALE ***
    if (m_NODE.DOF->Type == SCALE)
    {
        DofRot = Process_DOFRot(m_TheObjectInstance->DOFValues[m_NODE.DOF->dofNumber].rotation, m_NODE.DOF->dofNumber, m_NODE.DOF->flags,
                                m_NODE.DOF->min, m_NODE.DOF->max, m_NODE.DOF->multiplier, m_NODE.DOF->future);

        // Apply Scaling at the destination Matrix
        ZeroMemory(R, sizeof(D3DXMATRIX));
        R->m00 = 1.0f - (1.0f - m_NODE.DOF->scale.x) * DofRot;
        R->m11 = 1.0f - (1.0f - m_NODE.DOF->scale.y) * DofRot;
        R->m22 = 1.0f - (1.0f - m_NODE.DOF->scale.z) * DofRot;
        R->m33 = 1.0f;
    }

}


void CDXEngine::AssignDOFTranslation(D3DXMATRIX *T)
{
    float DofRot;

    // *** NORMAL ROTATION DOF ***
    if (m_NODE.DOF->Type == ROTATE)  D3DXMatrixTranslation(T, m_NODE.DOF->translation.x + m_TheObjectInstance->DOFValues[m_NODE.DOF->dofNumber].translation,
                m_NODE.DOF->translation.y, m_NODE.DOF->translation.z);

    // *** EXTENDED ROTATION DOF ***
    if (m_NODE.DOF->Type == XROTATE) D3DXMatrixTranslation(T, m_NODE.DOF->translation.x + m_TheObjectInstance->DOFValues[m_NODE.DOF->dofNumber].translation,
                m_NODE.DOF->translation.y, m_NODE.DOF->translation.z);

    // *** TRANSLATION DOF - NO ROTATION ***
    if (m_NODE.DOF->Type == TRANSLATE)
    {
        DofRot = Process_DOFRot(m_TheObjectInstance->DOFValues[m_NODE.DOF->dofNumber].rotation, m_NODE.DOF->dofNumber, m_NODE.DOF->flags,
                                m_NODE.DOF->min, m_NODE.DOF->max, m_NODE.DOF->multiplier, m_NODE.DOF->future);
        // Get DOF base translation
        Ppoint P = m_NODE.DOF->translation;
        // Apply DOF Scaling
        P.x *= DofRot;
        P.y *= DofRot;
        P.z *= DofRot;
        // Andcreate translation Matrix
        D3DXMatrixTranslation(T, P.x, P.y, P.z);
    }

    // *** SCALING DOF ***
    if (m_NODE.DOF->Type == SCALE) D3DXMatrixTranslation(T, m_NODE.DOF->translation.x, m_NODE.DOF->translation.y, m_NODE.DOF->translation.z);



}



// ********************************
// * the NORMAL DOF Function      *
// ********************************
void CDXEngine::DOF(void)
{
    D3DXMATRIX R, T;

#ifdef DEBUG_ENGINE

    if (m_bDofMove)
    {
        float rot = sinf((float)timeGetTime() / 1500.0f);
        m_TheObjectInstance->DOFValues[m_NODE.DOF->dofNumber].rotation = ((float)PI / 6.0f) * rot;
    }

#endif

#ifndef DEBUG_ENGINE

    // * CONSISTENCY CHECK  *
    if (m_NODE.DOF->dofNumber >= m_TheObjectInstance->ParentObject->nDOFs) return;

#endif
    // **** CALCULATE THE DOF IMPOSED ROTATION ****
    AssignDOFRotation(&R);

    // **** CALCULATE THE DOF IMPOSED TRANSLATION ****
    AssignDOFTranslation(&T);

    // Mix All and set to Actual Applied State
    D3DXMatrixMultiply(&R, &R, &T);
    D3DXMatrixMultiply(&AppliedState, &R, &AppliedState);
    DX_SET_WORLD(AppliedState);
}










// ********************************
// * the DOF MANAGEMENT Function  *
// ********************************
void CDXEngine::DOFManage()
{

#ifdef EDIT_ENGINE
    m_DofLevel++;

    if (m_SkipSwitch) return;

#endif

    // Select the DOF Type
    switch (m_NODE.DOF->Type)
    {

        case NO_DOF:
            break;

            // * POSITIONAL DOF MANAGEMENT *
        case ROTATE:
        case XROTATE:
        case TRANSLATE:
        case SCALE:
            PushMatrix(&AppliedState);
#ifdef DEBUG_ENGINE
            //if(NODE.SURFACE->dwFlags.b.Disable) break;
#endif

            DOF();
            break;

        case SWITCH:
        case XSWITCH:
            SWITCHManage();
            break;

    }


}







// ***********************************
// * the switch MANAGEMENT Function  *
// ***********************************
void CDXEngine::SWITCHManage()
{

    //Consistency check
    if ( not m_TheObjectInstance->SwitchValues)
    {
        // If no switches then skip the switch
        m_NODE.BYTE += m_NODE.DOF->dwDOFTotalSize;
        //and return
        return;
    }

    // Gets the Switch Number and value
    DWORD SWNumber = m_NODE.DOF->SwitchNumber;
    DWORD Value = m_TheObjectInstance->SwitchValues[SWNumber];
    BYTE *LastAddr = m_NODE.BYTE;

    if (m_NODE.DOF->Type == XSWITCH) Value = compl Value;

    // Traverse the Switch Items
    while (m_NODE.DOF->SwitchNumber == SWNumber and (m_NODE.DOF->Type == SWITCH or m_NODE.DOF->Type == XSWITCH))
    {
        // If value found then Exit here pointing the SWITCH, next to it is the SURFACE
        if (Value bitand (1 << m_NODE.DOF->SwitchBranch))
        {
            PushMatrix(&AppliedState);
            return;
        }

#ifdef EDIT_ENGINE
        m_SkipSwitch = true;
        m_DofLevel = 1;
        return;
#else
        m_NODE.BYTE += m_NODE.DOF->dwDOFTotalSize;
#endif
    }
}






// * This Function just Transformates the Object and pass it to the VB Manager for later Drawing *
// The 'CameraSpace' flag is used for child items from an undergoing draw, as the position is already relative to the camera
// and so need no camera relative calculations
// #16/#26 forward decl (falclib/include/isbad.h) for guarding dangling reads in the lights loop.
extern bool F4IsBadReadPtr(const void *lp, unsigned int ucb);

void CDXEngine::DrawObject(ObjectInstance *objInst, D3DXMATRIX *RotMatrix, const Ppoint *Pos, const float sx, const float sy, const float sz, const float scale, bool CameraSpace, DWORD LightOwner)
{
    D3DXMATRIX Scale, State;
    D3DVECTOR p;
    bool Visible = false;
    DWORD Liter;
    DxDbHeader *Model;

#ifdef DEBUG_LOD_ID
    // Debug pahse of LODs, clear any label
    LodLabel[0] = 0;
#endif;

    // Consistency Check
    if ( not objInst->ParentObject)
        return;

#ifndef DEBUG_ENGINE

    // Consistency Check
    if (objInst->id < 0 or objInst->id >= TheObjectListLength or objInst->TextureSet < 0)
        return;

#endif

    // if BLIT RADAR MODE, got to draw and return
    if (m_RenderState == DX_DBS)
    {
        DrawBlip(objInst, RotMatrix, Pos, sx, sy, sz, scale, CameraSpace);
        return;
    }


    // The object position is always calculated relative to the camera position
    // if coming from out world, if IN CAMERA SPACE, position is already relative to camera,
    // and even visibility is skipped
    if (CameraSpace)
    {
        p.x = Pos->x;
        p.y = Pos->y;
        p.z = Pos->z;
        State = *RotMatrix;
        Visible = true;
    }
    else
    {
        p.x = -CameraPos.x + Pos->x;
        p.y = -CameraPos.y + Pos->y;
        p.z = -CameraPos.z + Pos->z;
    }


    // NEW TEXTURE MANAGEMENT
    // if Textures not referenced, refernce them
#ifndef DEBUG_ENGINE

    if ( not objInst->TexSetReferenced)
    {
        objInst->ParentObject->ReferenceTexSet(objInst->TextureSet);
        objInst->TexSetReferenced = true;
    }

#endif
    ///////////////////////////////// CHECK FOR AVAILABLE LOD ///////////////////////////////////////
    // get the object distance
    float LODRange = sqrtf(p.x * p.x + p.y * p.y + p.z * p.z) * m_LODBiasCx;
    // The model pointer
    ObjectLOD *CurrentLOD = NULL;
    // Calculate the LOD based on FOV
    float MaxLODRange;
    int LODused;
    CurrentLOD = objInst->ParentObject->ChooseLOD(LODRange , &LODused, &MaxLODRange);

    // if not a lod persent, end here
    if ( not CurrentLOD) return;

    // ok assign The Model
    Model = (DxDbHeader*)CurrentLOD->root;

    // FRB - Filter out bad/nonexistant models
    if ((Model->Id <= 0) or (Model->Id >= (unsigned int)  TheObjectLODsCount))
        return;

    ///////////////////////////////// HERE CHECK FOR VISIBILITY /////////////////////////////////////
    // Camera Spacce objects are always visible
    if ( not CameraSpace)
    {
#ifndef DEBUG_ENGINE
        // Compute the object visibility -  Return if Clipped out
        D3DVALUE r = (D3DVALUE)(objInst->Radius() * scale);
        DWORD ClipResult = 0;	// PHASE 4: D3D11 -- without the D3D7 clip test treat as visible (frustum cull later)

        // if Visible assert it, if not visible got to check for Lights
        if (ClipResult bitand D3DSTATUS_DEFAULT) goto LightCheck;

        Visible = true;

        // ************ ADD Other Features ***********
        D3DXMatrixIdentity(&Scale);
        Scale.m00 = scale * sx;
        Scale.m11 = scale * sy;
        Scale.m22 = scale * sz;
        D3DXMatrixMultiply(&State, RotMatrix, &Scale);
        // *******************************************
#else
        State = *RotMatrix;
#endif

        // *********** Base transformations **********
        D3DXMatrixTranslation(&Scale, p.x, p.y, p.z);
        D3DXMatrixMultiply(&State, &State, &Scale);
        // *******************************************

        // #16 DIAG REMOVED (#10): per-object/per-frame fopen("objxform_diag.txt") stalled rendering on
        // the ground (tons of file I/O) -- the cockpit could not appear in time. The block was purely
        // diagnostic (projection to a file), did not affect rendering.
    }

    // check if child enlighted
    if (LightOwner not_eq NULL) Liter = LightOwner;
    else
    {
        if ( not ++LightID) LightID++;

        Liter = LightID;
    }

    // Pass the object to the vertex Buffer
#ifdef DEBUG_ENGINE
    VBItemType VB;
    TheVbManager.AddDrawRequest(objInst, objInst->id, &State, true, Liter);
    TheVbManager.GetModelData(VB, objInst->id);

    if (((DxDbHeader*)VB.Root)->dwLightsNr)
    {
        DXLightType *Light = (DXLightType*)(VB.Root + ((DxDbHeader*)VB.Root)->pLightsPool);
        DWORD LightsNr = ((DxDbHeader*)VB.Root)->dwLightsNr;

        while (LightsNr--)
        {
            if (objInst->SwitchValues[Light->Switch] bitand Light->SwitchMask) TheLightEngine.AddDynamicLight(Liter, Light, RotMatrix, &p, 100);

            Light++;
        }
    }

#else

    ////////////////////////// HERE ONLY IF VISIBLE OR TO TEST FOR LIGHTS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\

    //COUNT_PROFILE("DRAWN OBJECTS");
    // if object is visible and requested to draw ( may be also only add lights ) draw it
    if (Visible)
    {

#ifdef DEBUG_LOD_ID
        strcpy(LodLabel, TheLODNames[Model->Id]);
#endif
        // FOG CALCULATION
        // We r calculating the max range That should be valid for the LINEAR FOR MODE
        // to have m_FogLevel level at LODRange distance...
        float FogLevel = (m_FogLevel < 1.0f) ? LODRange / ((1 - m_FogLevel) * m_LODBiasCx) : m_LinearFogLevel;

        // if just a DOT the draw it as dynamic item
        if (Model->dwNVertices == 1)
        {
            //Calculate Specularness based on sunlight direction
            float Si;
            D3DXVECTOR3 Op;
            D3DXVec3Normalize(&Op, (D3DXVECTOR3*)&p);
            //
            Op = Op - *(D3DXVECTOR3*)&LightDir;
            /* Op = Op * Op;*/
            Si = 2.0f - (Op.x + Op.y + Op.z);

            // Ok, this is a HACK I do not like... seems nothing at render level lets u understand what kind of object u r going to render
            // only clue is fron DXM model header, that has a Class Air/Ground/Feature index...
            // hoping it is updated...

            // * Any class not air/ground get a normal draw
            if (Model->VBClass not_eq VB_CLASS_DOMAIN_GROUND and Model->VBClass not_eq VB_CLASS_DOMAIN_AIR) Si = 0.2f;

            // Ground vehicles, hi Q reflection index
            if (Model->VBClass == VB_CLASS_DOMAIN_GROUND)
            {
                Si *= Si * (1 - PRANDFloatPos() * 0.1f);
                Si *= Si;
                Si *= Si;
                Si /= 256.0f;

                if (Si < 0.2f) Si = 0.2f;
            }

            // Air vehicles, lower Q...
            if (Model->VBClass == VB_CLASS_DOMAIN_AIR)
            {
                Si *= Si * (1 - PRANDFloatPos() * 0.6f);
                Si /= 4.0f;

                if (Si < 0.3f) Si = 0.3f;
            }

            // Calculate the color based on Fog level
            // DWORD Color=(min(255,FloatToInt32(m_FogLevel*255.f)) << 24)+0x102010;
            DWORD Color = F_TO_UARGB(min(255.0f, F_I32(m_FogLevel * 255.f)), Si * 240.0f, Si * 255.0f, Si * 240.0f);
            Draw3DPoint((D3DVECTOR*)Pos, Color);
#ifdef DEBUG_LOD_ID
            strcpy(LodLabel, ".");
#endif
        }
        else TheVbManager.AddDrawRequest(objInst, Model->Id, &State, (LODRange <= (DYNAMIC_LIGHT_INSIDE_RANGE * 2)) ? true : false, Liter, FogLevel);
    }

LightCheck:
#ifdef LIGHT_ENGINE_DEBUG
    START_PROFILE("LIGHTS ON TIME");
#endif

    // if inside Lights visibility range, check for lights --- FRB - Bad dwLightsNr check and SwitchValues
    //if(LODRange<=DYNAMIC_LIGHT_INSIDE_RANGE and Model->dwLightsNr and (Model->dwLightsNr<11) and (objInst->SwitchValues))
    if (LODRange <= DYNAMIC_LIGHT_INSIDE_RANGE and Model->dwLightsNr)
    {
        // Get the Lights area in the model
        DXLightType *Light = (DXLightType*)((char*)Model + Model->pLightsPool);
        // The number of lights
        DWORD LightsNr = Model->dwLightsNr;

        // and add all of them to the dynamic lights list
        while (LightsNr--)
        {
            // #16/#26 guard: Switch==-1 means "always on". Otherwise index objInst->SwitchValues
            // ONLY if the array is present and readable up to that index -- a dangling/garbage
            // objInst->SwitchValues or a bogus Light->Switch was crashing here on 3D entry
            // (PreLoadScene -> DrawableBuilding). If unreadable, treat the light as off.
            bool lightOn = (Light->Switch == -1);

            if ( not lightOn and Light->Switch >= 0 and objInst->SwitchValues
                 and not F4IsBadReadPtr(objInst->SwitchValues, (unsigned)(Light->Switch + 1) * sizeof(objInst->SwitchValues[0])))
            {
                lightOn = (objInst->SwitchValues[Light->Switch] bitand Light->SwitchMask) != 0;
            }

            if (lightOn) TheLightEngine.AddDynamicLight(Liter, Light, RotMatrix, &p, LODRange);

            Light++;
        }
    }

#ifdef LIGHT_ENGINE_DEBUG
    STOP_PROFILE("LIGHTS ON TIME");
#endif
#endif
}


void CDXEngine::FlushInit(void)
{
    // if not yet created create the Zero Texture
    if ( not ZeroTex) CreateZeroTexture();

    D3DXMATRIX unit;
    D3DXMatrixIdentity(&unit);

    // #34 D3D11: identity world into cbObject (dead D3D7 material/render-state setup removed).
    DX_SET_WORLD(unit);
    m_LastZBias = DEFAULT_ZBIAS;

    // Select the appropriate View Mode
    SetViewMode();
    //Reset Features
    ResetFeatures();
}



inline void CDXEngine::DrawNode(ObjectInstance *objInst, DWORD LightOwner, DWORD LodID)
{
    // Selects actions for each node
    switch (m_NODE.HEAD->Type)
    {


        case DX_SWITCH:
        case DX_LIGHT:
        case DX_TEXTURE:
        case DX_MATERIAL:
        case DX_ROOT:
            break;


            // * SURFACE MANAGEMENT *
        case DX_SURFACE: // Setup the Texture setup the Texture to be used
#ifdef EDIT_ENGINE
            if (m_SkipSwitch) break;

#endif

            if (m_NODE.SURFACE->dwFlags.b.Texture and m_NODE.SURFACE->TexID[0] not_eq -1) m_TexID = m_TexUsed[m_NODE.SURFACE->TexID[0]];
            else m_TexID = -1;


            // Alpha Surfaces are deferred to another Draw
            if (m_NODE.SURFACE->dwFlags.b.Alpha)
            {
#ifdef STAT_DX_ENGINE
                COUNT_PROFILE("Alpha Surfaces Nr");
#endif
                // PushSurface(&m_AlphaStack, &AppliedState);
                PushSurfaceIntoSort(&m_AlphaStack, &AppliedState);
                break;
            }

            // Solid Surfaces are deferred to another Draw
            if (m_NODE.SURFACE->dwFlags.b.VColor)
            {
#ifdef STAT_DX_ENGINE
                COUNT_PROFILE("Solid Surfaces Nr");
#endif
                PushSurface(&m_SolidStack, &AppliedState);
                break;
            }

            DrawSurface();
            break;

        case DX_DOF:
            DOFManage();
            break;

        case DX_ENDDOF:
#ifdef EDIT_ENGINE
            if (m_SkipSwitch)
            {
                m_DofLevel--;

                if ( not m_DofLevel) m_SkipSwitch = false;

                break;
            }

#endif
            PopMatrix(&AppliedState);
            DX_SET_WORLD(AppliedState);
            break;

            // if bad slot exit else get the Slot Children
        case DX_SLOT:
#ifdef EDIT_ENGINE
            if (m_SkipSwitch) break;

#endif

            if (m_NODE.SLOT->SlotNr >= objInst->ParentObject->nSlots) break;

            {
                ObjectInstance *subObject = objInst->SlotChildren[m_NODE.SLOT->SlotNr];

                if ( not subObject) break;

                D3DXMATRIX p;
                D3DXMatrixMultiply(&p, &m_NODE.SLOT->rotation, &AppliedState);
                Ppoint k;
                k.x = 0;
                k.y = 0;
                k.z = 0;
                // Draw the object IN CAMERA SPACE - Child always depend on parent Lights...
                DrawObject(subObject, &p, &k, 1, 1, 1, 1, true, LightOwner);
            }
            break;

        default :
            char s[128];
            printf(s, "Corrupted Model ID : %d ", LodID);
            MessageBox(NULL, s, "DX Engine", NULL);

    }

}



void CDXEngine::FlushObjects(void)
{

    ObjectInstance *objInst = NULL;
    DWORD LodID;
    bool Lited, WasInPitMode;
    DWORD LightOwner;

    //TheTextureBank.SetDeferredLoad(true);

    // not a previous object instalce
    m_LastObjectInstance = NULL;

    // COBRA - RED - The it stuff... Pits need to be stenciled, so, all its objects are popped as 1st
    // from the VB Manager, and then drawn, its solid suraces too are to be drawn just after
    // finished Pit mode
    WasInPitMode = false;

    ///////////////////////////// HERE STARTS THE DRAWING ENGINE LOOP //////////////////////////////
    // The Loop flushes all objects from the VBuffers

    // Till objects to Draw
    while (TheVbManager.GetDrawItem(&objInst, &LodID, &AppliedState, &Lited, &LightOwner, &m_FogLevel))
    {
        // ok, just entered Pit Mode
        if (m_PitMode and not WasInPitMode)
        {
            //START_PROFILE("3D PIT");
            // enable stenciling in Write Mode
            SetStencilMode(STENCIL_WRITE);
            // #34 D3D11: pit fog is handled by the shader; dead D3D7 FOGSTART removed
        }

        // ok, just Exited Pit Mode
        if ( not m_PitMode and WasInPitMode)
        {
            // Save transformation State
            D3DXMATRIX OldState = AppliedState;
            // Immediatly draw Solid surfaces ( coming from Pit )
            DrawSolidSurfaces();
            AppliedState = OldState;
            // enable stenciling in Check Mode
            SetStencilMode(STENCIL_CHECK);
            // #34 D3D11: pit fog is handled by the shader; dead D3D7 FOGSTART removed

        }

        WasInPitMode = m_PitMode;

        // The Stack For the State Transformations resetted
        StateStackLevel = 0;

        // Consistency Check
        if ( not objInst) continue;

        // assign for engine use
        m_TheObjectInstance = objInst;

        // gets the pointer to the Model Vertex Buffer
        TheVbManager.GetModelData(m_VB, LodID);

        // Consistency Check
        if ( not m_VB.Valid) continue;

#ifdef STAT_DX_ENGINE
        COUNT_PROFILE("*** DX Objects");
#endif
        // Execute the Scripts 0 bitand 1 if existant
        DXScriptVariableType *Script = ((DxDbHeader*)m_VB.Root)->Scripts;
        D3DVECTOR pos;
        pos.x = AppliedState.m30;
        pos.y = AppliedState.m31;
        pos.z = AppliedState.m32;

        if (Script[0].Script) if ( not DXScriptArray[Script[0].Script](&pos, objInst, Script[0].Arguments)) goto DrawSection;

        if (Script[1].Script)( not DXScriptArray[Script[1].Script](&pos, objInst, Script[1].Arguments));

    DrawSection:

#ifndef DEBUG_ENGINE
#ifdef LIGHT_ENGINE_DEBUG
        START_PROFILE("LIGHTS UPDATE TIME");
#endif
#endif

        gDebugLodID = LodID;

        // Update the lights for the object
        if (Lited) TheLightEngine.UpdateDynamicLights(LightOwner, &pos, objInst->Radius());

#ifndef DEBUG_ENGINE
#ifdef LIGHT_ENGINE_DEBUG
        STOP_PROFILE("LIGHTS UPDATE TIME");
#endif
#endif


        // Ok... transform the object
        DX_SET_WORLD(AppliedState);
        // Stup the Fog level fro this object
        // #34: dead D3D7 FOGEND removed


        // Calculates the Texture Base Index in the Texture Bank
        int nTexsPerBank = m_VB.NTex / max(1, objInst->ParentObject->nTextureSets);
        DWORD *texOffset = m_VB.Texs + objInst->TextureSet * nTexsPerBank;

        // Register each texture for the Model ( and load it if not available ) and setup local Textures List
        for (int a = 0; a < nTexsPerBank; a++) m_TexUsed[a] = *texOffset++;

        //////////////////////// ********* HERE STARTS THE REAL NODES PARSING ***** ///////////////////////////////////
        //                                                                                                           //
        //                                                                                                           //
        //                                                                                                           //
        //                                                                                                           //
        // // Starting address
        m_NODE.BYTE = (BYTE*)m_VB.Nodes;

        // Till end of Model
        // #54 LOAD-HANG GUARD: this traversal holds cs_VbManager (taken in FlushBuffers:2344);
        // a broken/half-loaded model with dwNodeSize==0 (or a lost DX_MODELEND) -> INFINITE loop
        // -> the lock is never released -> the loader thread hangs forever in SetupModel (LOCK_VB_MANAGER).
        // Bail out on a zero step and on a cap = the model's declared node count (+slack).
        long _ndGuard = 0;
        long _ndMax   = (long)m_VB.NNodes + 16;   // a model cannot have more nodes than its header declares

        while (m_NODE.HEAD->Type not_eq DX_MODELEND)
        {
            // Draw the Node
            DrawNode(objInst, LightOwner, LodID);
            // Traverse the model
            DWORD _ndStep = m_NODE.HEAD->dwNodeSize;

            // #54: a zero-sized node (or runaway count) would loop forever holding cs_VbManager;
            // bail out of the traversal instead of hanging.
            if (_ndStep == 0 or ++_ndGuard > _ndMax)
                break;

            m_NODE.BYTE += _ndStep;
        }

        //                                                                                                           //
        //                                                                                                           //
        //                                                                                                           //
        //                                                                                                           //
        //                                                                                                           //
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    //TheTextureBank.SetDeferredLoad(false);
}



void CDXEngine::DrawAlphaSurfaces(void)
{
    D3DXMATRIX State;
    ObjectInstance *LastObj = NULL;
    float LastFog = 0;

    if (g_pRenderer)	// PHASE 5: translucent surfaces (canopy glass) -- alpha-blend (D3D7 removed #34)
        g_pRenderer->SetObjectAlphaBlend(true);

    while (PopSurface(&m_AlphaStack, &State))
    {
        if (AppliedState not_eq State) DX_SET_WORLD(State);

        AppliedState = State;
#ifndef DEBUG_ENGINE
#ifdef LIGHT_ENGINE_DEBUG
        START_PROFILE("LIGHTS ON TIME");
#endif
#endif

        // if Changed object, remap all lights
        if (LastObj not_eq m_TheObjectInstance) TheLightEngine.EnableMappedLights();

        LastObj = m_TheObjectInstance;

#ifndef DEBUG_ENGINE
#ifdef LIGHT_ENGINE_DEBUG
        STOP_PROFILE("LIGHTS ON TIME");
#endif
#endif

        DrawSurface();
    }

    if (g_pRenderer)	// PHASE 5: restore the opaque state (D3D7 removed #34)
        g_pRenderer->SetObjectAlphaBlend(false);
}





void CDXEngine::DrawSortedAlpha(DWORD Level, bool SetupMode)
{

    D3DXMATRIX State;

    // Initialize data parameters
    if (SetupMode) FlushInit();

    // Setup Alpha features
    if (g_pRenderer)	// PHASE 5: sorted transparency (D3D7 removed #34)
        g_pRenderer->SetObjectAlphaBlend(true);

    // Get the surface data and update transformations / features
    GetSurface(Level, &m_AlphaStack, &State);
    DX_SET_WORLD(State);
    AppliedState = State;

    if (m_LastObjectInstance not_eq m_TheObjectInstance) TheLightEngine.EnableMappedLights(), m_LastObjectInstance = m_TheObjectInstance;

    // Draw the surface
    DrawSurface();

}



void CDXEngine::DrawSolidSurfaces(void)
{
    D3DXMATRIX State;
    ObjectInstance *LastObj = NULL;
    float LastFog = 0;

    // #34 D3D11: cull/zwrite come from BeginObjectPass (dead D3D7 state setup removed).

    while (PopSurface(&m_SolidStack, &State))
    {
        if (State not_eq AppliedState) DX_SET_WORLD(State);

        AppliedState = State;
#ifndef DEBUG_ENGINE
#ifdef LIGHT_ENGINE_DEBUG
        START_PROFILE("LIGHTS ON TIME");
#endif
#endif

        // if Changed object, remap all lights
        if (LastObj not_eq m_TheObjectInstance) TheLightEngine.EnableMappedLights();

        LastObj = m_TheObjectInstance;
#ifndef DEBUG_ENGINE
#ifdef LIGHT_ENGINE_DEBUG
        STOP_PROFILE("LIGHTS ON TIME");
#endif
#endif

        DrawSurface();
    }
}


#ifdef DEBUG_ENGINE
// Artscout - 2026: #34 removed dead DrawFrameSurfaces (EDIT_ENGINE wireframe draw; no callers, all D3D7 m_pD3DD).

#endif


extern DWORD LODsLoaded;
// *************** This function is the REAL SCENE DRAW FUNCTION *********************
// it flushes all requested Drawsand draws all poly types
void CDXEngine::FlushBuffers(void)
{
    float FogStart = 0.0;
    D3DErroCount = 3;



#ifndef DEBUG_ENGINE
    //REPORT_VALUE("LODs : ", LODsLoaded);
#endif

    // First of all save present renderer State
    DWORD StateHandle = 0;

    if (g_bUseGpu)
    {
        // PHASE 4/#DX12 п.4: GPU object pass (D3D11 or D3D12) -- shaders/state/transforms/lighting.
        if (g_pRenderer and g_pRenderer->IsValid())
        {
            g_pRenderer->BeginObjectPass();
            g_pRenderer->SetProj((const float *)&Projection);
            g_pRenderer->SetView((const float *)&CameraView);
            g_pRenderer->SetCameraPos(CameraPos.x, CameraPos.y, CameraPos.z);	// #29 specular

            // Sun (directional) + ambient. CROSS-CHECK WITH FF7: object light model =
            // vertexColor * (TheSun.dcvAmbient + TheSun.dcvDiffuse.N.L), where dcvAmbient/dcvDiffuse
            // are time-of-day modulated in SetSunLight() (statestack.cpp). Previously we took
            // TheSunColour (unmodulated) + a fixed ambient 0.45 -> objects stayed bright at night.
            // Now we give the shader TOD values -> on par with the reference (dark night).
            // Ambient light source by mode (like the reference SetLight(0,&The*)):
            // NVG -> green boost TheNVG, TV -> TheTV, else the sun.
            D3DLIGHT7 &envL = (m_RenderState == DX_NVG) ? TheNVG
                            : (m_RenderState == DX_TV)  ? TheTV : TheSun;
            GpuLightCPU sun;
            ZeroMemory(&sun, sizeof(sun));
            sun.Direction[0] = -LightDir.x;
            sun.Direction[1] = -LightDir.y;
            sun.Direction[2] = -LightDir.z;
            sun.Color[0] = envL.dcvDiffuse.r;
            sun.Color[1] = envL.dcvDiffuse.g;
            sun.Color[2] = envL.dcvDiffuse.b;
            sun.Params[1] = 0.0f;	// directional
            const float amb[4] = { envL.dcvAmbient.r, envL.dcvAmbient.g, envL.dcvAmbient.b, 1.0f };
            g_pRenderer->SetLights(amb, 1, &sun, sizeof(sun));
            // #28: save for per-object dynamic lighting (UpdateDynamicLights).
            g_d3d11Sun = sun;
            g_d3d11Amb[0] = amb[0]; g_d3d11Amb[1] = amb[1]; g_d3d11Amb[2] = amb[2]; g_d3d11Amb[3] = amb[3];

            // PHASE 5: the stencil buffer is cleared to 0 each frame -> reset the CPU counter too
            // for ref, else on an 8-bit stencil ref&0xFF==0 once every 256 frames (black frame).
            m_StencilRef = 0;
        }

        FlushInit();
        m_LinearFogLevel = MAX_FOG_RANGE;
        m_LastSpecular = 0;
    }
    // #34 D3D11: dead D3D7 device/state setup (else-branch) removed.


    LOCK_VB_MANAGER;

    // Start resetting Draw Pointers
    TheVbManager.ResetDrawList();

    // Flush all cached VB objects
    if (m_RenderState == DX_DBS) FlushBlips();
    else FlushObjects();

    // Draw the Solid Surfaces
    DrawSolidSurfaces();

    // Flush Dynamic Buffers bitand sorted objects
    FlushDynamicObjects();

    //Reset Features
    ResetFeatures();

    // Setup VB draw list for a new round
    TheVbManager.ClearDrawList();
    // Clear any light
    TheLightEngine.ResetLightsList();

    UNLOCK_VB_MANAGER;

    // #34 D3D11: no D3D7 state block to restore.

    gDebugLodID = -1;
    m_AlphaStack.StackLevel = 0;

}


#ifdef EDIT_ENGINE


void CDXEngine::ModelInit(ObjectInstance *objInst, DxDbHeader* Header, DWORD *Textures, D3DXMATRIX *State, DWORD LightOwner, DWORD nTexsPerBank)
{
    D3DXMATRIX Position;

    ResetState();

    m_TheObjectInstance = objInst;
    AppliedState = *State;

    // Setup the state for the DX engine
    CheckHR(m_pD3DD->ApplyStateBlock(DxEngineStateHandle));

    // *** Default engine initializations ***
    m_pD3DD->SetRenderState(D3DRENDERSTATE_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR2);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_CLIPPING, FALSE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);

    // Set Up the View port
    m_pD3DD->SetViewport(&ViewPort);

    // Set Up the Field of View Projection
    m_pD3DD->SetTransform(D3DTRANSFORMSTATE_PROJECTION, (LPD3DMATRIX)&Projection);

    // Set Up the camera View for the drawing
    m_pD3DD->SetTransform(D3DTRANSFORMSTATE_VIEW, (LPD3DMATRIX)&CameraView);

    // Initialize data parameters
    FlushInit();

    //Reset Features
    ResetFeatures();

    // The Stack For the State Transformations resetted
    StateStackLevel = 0;
    // Transform the model
    AppliedState = *State;

    // Execute the Scripts 0 bitand 1 if existant
    D3DVECTOR pos;
    pos.x = AppliedState.m30;
    pos.y = AppliedState.m31;
    pos.z = AppliedState.m32;

    if ( not m_ScriptsOn) goto DrawSection;

    DXScriptVariableType *Script = (Header)->Scripts;

    if (Script[0].Script) if ( not DXScriptArray[Script[0].Script](&pos, objInst, Script[0].Arguments)) goto DrawSection;

    if (Script[1].Script)( not DXScriptArray[Script[1].Script](&pos, objInst, Script[1].Arguments));

DrawSection:

    TheLightEngine.UpdateDynamicLights(LightOwner, &pos, 2000.0f/*objInst->Radius()*/);

    // Ok... transform the object
    DX_SET_WORLD(AppliedState);

    // Calculates the Texture Base Index in the Texture Bank
    DWORD *texOffset = (DWORD*)(Textures + objInst->TextureSet * nTexsPerBank);

    // Register each texture for the Model ( and load it if not available ) and setup local Textures List
    for (DWORD a = 0; a < nTexsPerBank; a++) m_TexUsed[a] = *texOffset++;

}



void CDXEngine::DrawNodeEx(NodeScannerType *NODE, ObjectInstance *objInst, DWORD LightOwner, DWORD LodID)
{
    m_TheObjectInstance = objInst;
    m_NODE = *NODE;
    DrawNode(objInst, LightOwner, LodID);
}


void CDXEngine::DofManageEx(NodeScannerType *NODE, ObjectInstance *objInst, D3DXMATRIX *NewState)
{

    m_TheObjectInstance = objInst;
    m_NODE = *NODE;
    // Reset the Applied State Matrix
    AppliedState = *NewState;
    // Manage the DOF
    DOF();
    // copy result to destination matrix
    *NewState = AppliedState;

}


bool CDXEngine::SwitchManageEx(NodeScannerType *NODE, ObjectInstance *objInst, D3DXMATRIX *NewState)
{
    bool value;

    m_TheObjectInstance = objInst;
    m_NODE = *NODE;

    // Reset the Applied State Matrix
    AppliedState = *NewState;

    // Manage the switch
    SWITCHManage();

    value = m_SkipSwitch;
    m_SkipSwitch = false;
    return value;
}



void CDXEngine::PushMatrixEx(D3DXMATRIX *NewState)
{
    PushMatrix(NewState);
}

void CDXEngine::PopMatrixEx(D3DXMATRIX *NewState)
{
    PopMatrix(NewState);
}

#endif
