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

IDirect3DDevice7 *CDXEngine::m_pD3DD;
IDirect3D7 *CDXEngine::m_pD3D;
IDirectDraw7 *CDXEngine::m_pDD;

D3DXMATRIX CDXEngine::State, CDXEngine::DofTransformation, CDXEngine::AppliedState;
DWORD CDXEngine::StateStackLevel;
D3DXMATRIX CDXEngine::CameraView;
D3DXMATRIX CDXEngine::BBMatrix;
D3DVECTOR CDXEngine::CameraPos;
D3DVECTOR CDXEngine::LightDir;
D3DXMATRIX CDXEngine::Projection;
D3DXMATRIX CDXEngine::World;
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
	if (DxEngineStateHandle) CheckHR(m_pD3DD->DeleteStateBlock(DxEngineStateHandle));
}

// The Default engine states for the renderer
// This state must be sampled at D3DD CREATION PHASE, to keep it independent by following
// BSP engine state changes
void CDXEngine::StoreSetupState(void)
{
    // First of all save present renderer State
    DWORD StateHandle;
    CheckHR(m_pD3DD->CreateStateBlock(D3DSBT_ALL, &StateHandle));

    // Setup all Default engine States
    m_pD3DD->SetRenderState(D3DRENDERSTATE_STIPPLEDALPHA, FALSE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, FALSE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_STENCILENABLE, FALSE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_DITHERENABLE, FALSE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE, TRUE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_LASTPIXEL, TRUE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, TRUE);

    // Disable all stages
    for (int i = 0; i < 8; i++)
    {
        m_pD3DD->SetTextureStageState(i, D3DTSS_COLOROP, D3DTOP_DISABLE);
        m_pD3DD->SetTextureStageState(i, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    }

    m_pD3DD->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
    m_pD3DD->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
    m_pD3DD->SetTextureStageState(1, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
    m_pD3DD->SetTextureStageState(1, D3DTSS_MINFILTER, D3DTFN_LINEAR);
    m_pD3DD->SetTextureStageState(2, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
    m_pD3DD->SetTextureStageState(2, D3DTSS_MINFILTER, D3DTFN_LINEAR);
    m_pD3DD->SetTextureStageState(3, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
    m_pD3DD->SetTextureStageState(3, D3DTSS_MINFILTER, D3DTFN_LINEAR);

    // Enable Alpha Rendering
    m_pD3DD->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);

    CheckHR(m_pD3DD->CreateStateBlock(D3DSBT_PIXELSTATE, &DxEngineStateHandle));

    // Restore Previous State Block and delete it from memory
    CheckHR(m_pD3DD->ApplyStateBlock(StateHandle));
    CheckHR(m_pD3DD->DeleteStateBlock(StateHandle));


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
    m_pD3DD->SetTransform(D3DTRANSFORMSTATE_VIEW, (LPD3DMATRIX)&CameraView);

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

    // get the Handle of the Texture from the Texture Bank
    texID = (texID not_eq -1) ? TheTextureBank.GetHandle(texID) : (GLint)ZeroTex;

    if (texID) texID = (GLint)((TextureHandle *)texID)->m_pDDS;

    // only Texture on Stage 0 is needed for normal View
    if (m_RenderState == DX_OTW)
    {
        CheckHR(m_pD3DD->SetTexture(0, (IDirectDrawSurface7 *)texID));
        return;
    }

    // if here, TV and NVG need othe texture stages setted
    CheckHR(m_pD3DD->SetTexture(1, (IDirectDrawSurface7 *)texID));
    CheckHR(m_pD3DD->SetTexture(3, (IDirectDrawSurface7 *)texID));
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
void CDXEngine::Setup(IDirect3DDevice7 *pD3DD, IDirect3D7 *pD3D, IDirectDraw7 *pDD)
{
    m_pD3DD = pD3DD;
    m_pD3D = pD3D;
    m_pDD = pDD;
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
    TheLightEngine.Setup(pD3DD, pD3D);

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

    DDPIXELFORMAT ddpf;
    DDBLTFX ddbltfx;
    ZeroTex->m_pDDS->GetPixelFormat(&ddpf);
    ddbltfx.dwSize = sizeof(ddbltfx);
    ddbltfx.dwFillColor = 0xffffffff; // Pure White

    ZeroTex->m_pDDS->Blt(
        NULL,        // Destination is entire surface
        NULL,        // No source surface
        NULL,        // No source rectangle
        DDBLT_COLORFILL, &ddbltfx);
}




void CDXEngine::Release(void)
{
    m_pD3DD->DeleteStateBlock(DxEngineStateHandle);
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

    switch (Stencil)
    {

        case STENCIL_OFF:
            m_pD3DD->SetRenderState(D3DRENDERSTATE_STENCILFUNC, D3DCMP_ALWAYS);
            m_pD3DD->SetRenderState(D3DRENDERSTATE_STENCILPASS, D3DSTENCILOP_KEEP);
            m_pD3DD->SetRenderState(D3DRENDERSTATE_STENCILENABLE, FALSE);
            break;

        case STENCIL_ON:
            break;

        case STENCIL_WRITE:
            m_StencilRef++;
            m_pD3DD->SetRenderState(D3DRENDERSTATE_STENCILFUNC, D3DCMP_ALWAYS);
            m_pD3DD->SetRenderState(D3DRENDERSTATE_STENCILMASK, 0xffffffff);
            m_pD3DD->SetRenderState(D3DRENDERSTATE_STENCILWRITEMASK, 0xffffffff);
            m_pD3DD->SetRenderState(D3DRENDERSTATE_STENCILREF, m_StencilRef);
            m_pD3DD->SetRenderState(D3DRENDERSTATE_STENCILPASS, D3DSTENCILOP_REPLACE);
            m_pD3DD->SetRenderState(D3DRENDERSTATE_STENCILENABLE, TRUE);
            break;

        case STENCIL_CHECK:
            m_pD3DD->SetRenderState(D3DRENDERSTATE_STENCILFUNC, m_StencilRef ? D3DCMP_GREATER : D3DCMP_ALWAYS);
            m_pD3DD->SetRenderState(D3DRENDERSTATE_STENCILMASK, 0xffffffff);
            m_pD3DD->SetRenderState(D3DRENDERSTATE_STENCILWRITEMASK, 0xffffffff);
            m_pD3DD->SetRenderState(D3DRENDERSTATE_STENCILREF, m_StencilRef);
            m_pD3DD->SetRenderState(D3DRENDERSTATE_STENCILPASS, D3DSTENCILOP_KEEP);
            m_pD3DD->SetRenderState(D3DRENDERSTATE_STENCILENABLE, TRUE);
            break;
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

    switch (m_RenderState)
    {

        case DX_DBS:
            m_pD3DD->SetRenderState(D3DRENDERSTATE_TEXTUREFACTOR, NVG_T_FACTOR);
            /*m_pD3DD->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TFACTOR);
            m_pD3DD->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);
            m_pD3DD->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);*/
            m_pD3DD->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
            m_pD3DD->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
            m_pD3DD->SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
            m_pD3DD->SetTextureStageState(3, D3DTSS_COLOROP, D3DTOP_DISABLE);;
            m_pD3DD->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_FLAT);
            m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);

            // The Texture Stage used for Alpha Calculations
            m_AlphaTextureStage = 0;
            // Select the appropriate Light
            TheNVG.dvDirection = TheTV.dvDirection = TheSun.dvDirection;
            m_pD3DD->SetLight(0, &TheNVG);
            m_pD3DD->LightEnable(0, true);
            break;



        case DX_NVG:
        case DX_TV:

            // FRB - B&W
            if ((m_RenderState == DX_TV) and ( not bNVGmode) and (g_bGreyMFD))
                m_pD3DD->SetRenderState(D3DRENDERSTATE_TEXTUREFACTOR, 0x00a0a0a0);
            else
                m_pD3DD->SetRenderState(D3DRENDERSTATE_TEXTUREFACTOR, 0x0000a000 /*NVG_T_FACTOR*/);

            //m_pD3DD->SetRenderState( D3DRENDERSTATE_TEXTUREFACTOR, NVG_T_FACTOR);

            m_pD3DD->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
            m_pD3DD->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
            m_pD3DD->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_ADDSMOOTH);

            m_pD3DD->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            m_pD3DD->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
            m_pD3DD->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_ADDSIGNED);

            m_pD3DD->SetTextureStageState(2, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
            m_pD3DD->SetTextureStageState(2, D3DTSS_COLORARG1, D3DTA_CURRENT);
            m_pD3DD->SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DOTPRODUCT3);

            m_pD3DD->SetTextureStageState(3, D3DTSS_COLORARG2, D3DTA_TFACTOR);
            m_pD3DD->SetTextureStageState(3, D3DTSS_COLORARG1, D3DTA_CURRENT);
            m_pD3DD->SetTextureStageState(3, D3DTSS_COLOROP, D3DTOP_ADDSIGNED);

            m_pD3DD->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
            m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGENABLE, TRUE);

            // The Texture Stage used for Alpha Calculations
            m_AlphaTextureStage = 3;
            // Select the appropriate Light
            m_pD3DD->LightEnable(0, false);
            TheNVG.dvDirection = TheTV.dvDirection = TheSun.dvDirection;
            m_pD3DD->SetLight(0, (m_RenderState == DX_NVG) ? &TheNVG : &TheTV);
            m_pD3DD->LightEnable(0, true);
            break;

        case DX_OTW:
        default :
            m_pD3DD->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            m_pD3DD->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
            m_pD3DD->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
            m_pD3DD->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
            m_pD3DD->SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
            m_pD3DD->SetTextureStageState(3, D3DTSS_COLOROP, D3DTOP_DISABLE);;
            m_pD3DD->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
#ifdef EDIT_ENGINE
#else
            m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGENABLE, TRUE);
#endif

            // The Texture Stage used for Alpha Calculations
            m_AlphaTextureStage = 0;
            // Select the appropriate Light
            m_pD3DD->LightEnable(0, false);
            m_pD3DD->SetLight(0, &TheSun);
            m_pD3DD->LightEnable(0, true);
            break;
    }
}





// Function switching the renderer State
void CDXEngine::SetRenderState(DXFlagsType Flags, DXFlagsType NewFlags, bool Enable)
{

    // ********************** ENABLE BLOCK *********************
    if (Enable)
    {

#ifdef EDIT_ENGINE

        if (m_FrameDrawMode)
        {
            Flags.w = 0;
            Flags.b.Line = 1;
            Flags.b.VColor = 1;
            NewFlags.w = 0;
            m_pD3DD->SetRenderState(D3DRENDERSTATE_FILLMODE, D3DFILL_WIREFRAME);
            m_pD3DD->SetRenderState(D3DRENDERSTATE_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
            TheMaterial.specular.r = TheMaterial.specular.g = TheMaterial.specular.b = 1.0f;
            TheMaterial.emissive.r = TheMaterial.emissive.g = TheMaterial.emissive.b = 1.0f;
            m_pD3DD->SetRenderState(D3DRENDERSTATE_CULLMODE, (m_bCullEnable) ? D3DCULL_CW : D3DCULL_NONE);
        }

#endif

        // **** CHROMA KEYING INITIALIZATION ****
        if (Flags.b.ChromaKey)
        {
            m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, TRUE);
            m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHAREF, (DWORD)1);
            m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, D3DCMP_GREATEREQUAL);
            m_pD3DD->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
            m_pD3DD->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
            m_pD3DD->SetTextureStageState(m_AlphaTextureStage, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
            m_pD3DD->SetTextureStageState(m_AlphaTextureStage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
            m_pD3DD->SetTextureStageState(m_AlphaTextureStage, D3DTSS_MAGFILTER, D3DTFG_POINT);
            m_pD3DD->SetTextureStageState(m_AlphaTextureStage, D3DTSS_MINFILTER, D3DTFN_POINT);
        }

        // **************************************



        // *** ALPHA BLENDING SELECTION - FROM VERTEX COLOR SELECTION *
        if (Flags.b.VColor)
        {
            m_pD3DD->SetTextureStageState(m_AlphaTextureStage, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
            m_pD3DD->SetTextureStageState(m_AlphaTextureStage, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
            m_pD3DD->SetTextureStageState(m_AlphaTextureStage, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

        }


#ifdef EDIT_ENGINE

        if (Flags.b.Gouraud) m_pD3DD->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);

#endif
    }


    // ********************** DISABLE BLOCK *********************
    if ( not Enable)
    {

#ifdef EDIT_ENGINE

        if (m_FrameDrawMode)
        {
            // Done to reset material
            TheMaterial.specular.r = TheMaterial.specular.g = TheMaterial.specular.b = 0.0f;
            TheMaterial.emissive.r = TheMaterial.emissive.g = TheMaterial.emissive.b = 0.0f;
            TheMaterial.dvPower = -1.0f;
            m_pD3DD->SetRenderState(D3DRENDERSTATE_CULLMODE, (m_bCullEnable) ? D3DCULL_CW : D3DCULL_NONE);
        }

#endif

        // **** CHROMA KEYING DISABLES ****
        if (Flags.b.ChromaKey)
        {
            m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, FALSE);
            m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, D3DCMP_ALWAYS);
            m_pD3DD->SetTextureStageState(m_AlphaTextureStage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
            m_pD3DD->SetTextureStageState(m_AlphaTextureStage, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
            m_pD3DD->SetTextureStageState(m_AlphaTextureStage, D3DTSS_MINFILTER, D3DTFN_LINEAR);
        }

        // **************************************

        // *** ALPHA BLENDING SELECTION - FROM TEXTURE COLOR SELECTION *
        if (Flags.b.VColor)
        {
            m_pD3DD->SetTextureStageState(m_AlphaTextureStage, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
            m_pD3DD->SetTextureStageState(m_AlphaTextureStage, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
            m_pD3DD->SetTextureStageState(m_AlphaTextureStage, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

        }

#ifdef EDIT_ENGINE

        if (Flags.b.Gouraud) m_pD3DD->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_FLAT);

#endif
    }
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

    // Switching Emissive surfaces feature, setup the flags for the surface
    m_pD3DD->SetRenderState(D3DRENDERSTATE_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR2);

    if (m_TheObjectInstance->SwitchValues and (NewFlags.b.SwEmissive))
    {
        if ( not (m_TheObjectInstance->SwitchValues[m_NODE.SURFACE->SwitchNumber]&m_NODE.SURFACE->SwitchMask))
            m_pD3DD->SetRenderState(D3DRENDERSTATE_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
    }


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
        m_pD3DD->SetRenderState(D3DRENDERSTATE_ZBIAS, m_LastZBias);
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
        m_pD3DD->SetTransform(D3DTRANSFORMSTATE_WORLD, (LPD3DMATRIX)&R);
    }


    ////////////////////// Surface SPECULARITY  management ///////////////////////////
    if (TheMaterial.power not_eq m_NODE.SURFACE->SpecularIndex or m_LastSpecular not_eq m_NODE.SURFACE->DefaultSpecularity)
    {
        TheMaterial.power = m_NODE.SURFACE->SpecularIndex;
        m_LastSpecular = m_NODE.SURFACE->DefaultSpecularity;
        TheMaterial.dcvSpecular.r = (float)((m_LastSpecular >> 16) bitand 0xff) / 255.0f;
        TheMaterial.dcvSpecular.g = (float)((m_LastSpecular >> 8) bitand 0xff) / 255.0f;
        TheMaterial.dcvSpecular.b = (float)(m_LastSpecular bitand 0xff) / 255.0f;
        m_pD3DD->SetMaterial(&TheMaterial);
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

    if (m_NODE.SURFACE->dwPrimType == D3DPT_POINTLIST)
    {
        // SelectTexture(NULL);
        // m_LastTexID=-1;
        hr = m_pD3DD->DrawPrimitiveVB(m_NODE.SURFACE->dwPrimType, m_VB.Vb, (DWORD) * ((Int16*)(m_NODE.BYTE + sizeof(DxSurfaceType))) + m_VB.BaseOffset,
                                      m_NODE.SURFACE->dwVCount, 0);
    }
    else
    {
        hr = m_pD3DD->DrawIndexedPrimitiveVB(m_NODE.SURFACE->dwPrimType, m_VB.Vb, m_VB.BaseOffset, m_VB.NVertices,
                                             (LPWORD)(m_NODE.BYTE + sizeof(DxSurfaceType)), m_NODE.SURFACE->dwVCount, 0);
    }


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
        m_pD3DD->SetTransform(D3DTRANSFORMSTATE_WORLD, (LPD3DMATRIX)&AppliedState);
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
    m_pD3DD->SetTransform(D3DTRANSFORMSTATE_WORLD, (LPD3DMATRIX)&AppliedState);
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
        DWORD ClipResult;
        m_pD3DD->ComputeSphereVisibility(&p, &r, 1, 0, &ClipResult);

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
            if (Light->Switch == -1 or (objInst->SwitchValues[Light->Switch] bitand Light->SwitchMask)) TheLightEngine.AddDynamicLight(Liter, Light, RotMatrix, &p, LODRange);

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

    // Initialize the Default Material
    m_pD3DD->SetMaterial(&TheMaterial);

    D3DXMATRIX unit;
    D3DXMatrixIdentity(&unit);
    m_pD3DD->SetTransform(D3DTRANSFORMSTATE_WORLD, (LPD3DMATRIX)&unit);

    // Initial zBias
    m_pD3DD->SetRenderState(D3DRENDERSTATE_ZBIAS, DEFAULT_ZBIAS);
    m_LastZBias = DEFAULT_ZBIAS;
    // Draw Mode
    m_pD3DD->SetRenderState(D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID);
    // Light On
    m_pD3DD->SetRenderState(D3DRENDERSTATE_LIGHTING, TRUE);
    // Culling
#ifndef DEBUG_ENGINE
    m_pD3DD->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_CW);
#else
    m_pD3DD->SetRenderState(D3DRENDERSTATE_CULLMODE, (m_bCullEnable) ? D3DCULL_CW : D3DCULL_NONE);
#endif
    // ZBuffering
    m_pD3DD->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);
    // **************************************

    m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, TRUE);

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
            m_pD3DD->SetTransform(D3DTRANSFORMSTATE_WORLD, (LPD3DMATRIX)&AppliedState);
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
            // No Fog into the pit
            float Start = 5.0f;
            m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGSTART, *(DWORD*)&Start);
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
            // Restore Fog
            float Start = 0.0f;
            m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGSTART, *(DWORD*)&Start);

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
        m_pD3DD->SetTransform(D3DTRANSFORMSTATE_WORLD, (LPD3DMATRIX)&AppliedState);
        // Stup the Fog level fro this object
        m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGEND,   *(DWORD *)(&m_FogLevel));


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
        while (m_NODE.HEAD->Type not_eq DX_MODELEND)
        {
            // Draw the Node
            DrawNode(objInst, LightOwner, LodID);
            // Traverse the model
            m_NODE.BYTE += m_NODE.HEAD->dwNodeSize;
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

    m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);

    m_pD3DD->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, FALSE);

    while (PopSurface(&m_AlphaStack, &State))
    {
        if (AppliedState not_eq State) m_pD3DD->SetTransform(D3DTRANSFORMSTATE_WORLD, (LPD3DMATRIX)&State);

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

        // Stup the Fog level for this object
        if (m_FogLevel not_eq LastFog) m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGEND,   *(DWORD *)(&m_FogLevel));

        LastFog = m_FogLevel;
        DrawSurface();
    }

    m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
}





void CDXEngine::DrawSortedAlpha(DWORD Level, bool SetupMode)
{

    D3DXMATRIX State;

    // Initialize data parameters
    if (SetupMode) FlushInit();

    // Setup Alpha features
    m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, FALSE);

    // Get the surface data and update transformations / features
    GetSurface(Level, &m_AlphaStack, &State);
    m_pD3DD->SetTransform(D3DTRANSFORMSTATE_WORLD, (LPD3DMATRIX)&State);
    AppliedState = State;

    if (m_LastObjectInstance not_eq m_TheObjectInstance) TheLightEngine.EnableMappedLights(), m_LastObjectInstance = m_TheObjectInstance;

    m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGEND,   *(DWORD *)(&m_FogLevel));

    // Draw the surface
    DrawSurface();

}



void CDXEngine::DrawSolidSurfaces(void)
{
    D3DXMATRIX State;
    ObjectInstance *LastObj = NULL;
    float LastFog = 0;

#ifndef DEBUG_ENGINE
    m_pD3DD->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_CW);
#else
    m_pD3DD->SetRenderState(D3DRENDERSTATE_CULLMODE, (m_bCullEnable) ? D3DCULL_CW : D3DCULL_NONE);
#endif
    m_pD3DD->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);


    while (PopSurface(&m_SolidStack, &State))
    {
        if (State not_eq AppliedState) m_pD3DD->SetTransform(D3DTRANSFORMSTATE_WORLD, (LPD3DMATRIX)&State);

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

        // Stup the Fog level for this object
        if (m_FogLevel not_eq LastFog) m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGEND,   *(DWORD *)(&m_FogLevel));

        LastFog = m_FogLevel;
        DrawSurface();
    }
}


#ifdef DEBUG_ENGINE
void CDXEngine::DrawFrameSurfaces(NodeScannerType *NODE, float Alpha)
{
    // First of all save present renderer State
    D3DXMATRIX State;
    D3DMATERIAL7 OldMat = TheMaterial, Mat2;

    if (SelectColor >= 1.0f) StepColor = -0.2f;

    if (SelectColor <= 0.0f) StepColor = 0.2f;

    //SelectColor+=StepColor;
    SelectColor = 0.9f;

    m_NODE = *NODE;

    // First of all save present renderer State
    DWORD StateHandle;
    CheckHR(m_pD3DD->CreateStateBlock(D3DSBT_ALL, &StateHandle));

    TheMaterial.emissive.a = Alpha;
    TheMaterial.emissive.g = TheMaterial.emissive.b = 1.0f - SelectColor;
    TheMaterial.emissive.r = 1.0f;
    TheMaterial.diffuse.a = Alpha;
    TheMaterial.diffuse.g = TheMaterial.diffuse.b = 1.0f - SelectColor;
    TheMaterial.diffuse.r = 1.0f;
    TheMaterial.ambient.a = Alpha;
    TheMaterial.ambient.g = TheMaterial.ambient.b = 1.0f - SelectColor;
    TheMaterial.ambient.r = 1.0f;
    TheMaterial.specular.a = Alpha;
    TheMaterial.specular.r = TheMaterial.specular.g = TheMaterial.specular.b = 1.0f;;


    Mat2.emissive.a = Alpha * 0.2f;
    Mat2.emissive.g = Mat2.emissive.b = 1.0f - SelectColor;
    Mat2.emissive.r = SelectColor / 2 + 0.5f;
    Mat2.diffuse.a = Alpha * 0.2f;
    Mat2.diffuse.g = Mat2.diffuse.b = 1.0f - SelectColor;
    Mat2.diffuse.r = SelectColor;
    Mat2.ambient.a = Alpha * 0.2f;
    Mat2.ambient.g = Mat2.ambient.b = 1.0f - SelectColor;
    Mat2.ambient.r = SelectColor;
    Mat2.specular.a = Alpha * 0.2f;
    Mat2.specular.r = Mat2.specular.g = Mat2.specular.b = SelectColor;

    m_pD3DD->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, FALSE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_COLORVERTEX, FALSE);

    m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, FALSE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_CULLMODE, (m_bCullEnable) ? D3DCULL_CW : D3DCULL_NONE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
    //m_pD3DD->SetRenderState(D3DRENDERSTATE_ZFUNC,D3DCMP_ALWAYS);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, FALSE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);

    m_pD3DD->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
    m_pD3DD->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_LIGHTING, TRUE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_FLAT);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_ZBIAS, 15);

    m_TexID = -1;
    /* while(PopSurface(&m_FrameStack, &State)){
     m_pD3DD->SetTransform( D3DTRANSFORMSTATE_WORLD, (LPD3DMATRIX)&State );
     AppliedState=State;*/
    m_pD3DD->SetMaterial(&Mat2);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID);
    CheckHR(m_pD3DD->DrawPrimitive(m_NODE.SURFACE->dwPrimType, D3DFVF_MANAGED, m_NODE.BYTE + sizeof(DxSurfaceType), m_NODE.SURFACE->dwVCount, 0));
    m_pD3DD->SetMaterial(&TheMaterial);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_FILLMODE, D3DFILL_WIREFRAME);
    CheckHR(m_pD3DD->DrawPrimitive(m_NODE.SURFACE->dwPrimType, D3DFVF_MANAGED, m_NODE.BYTE + sizeof(DxSurfaceType), m_NODE.SURFACE->dwVCount, 0));

    // }


    // Restore Previous State Block and delete it from memory
    CheckHR(m_pD3DD->ApplyStateBlock(StateHandle));
    CheckHR(m_pD3DD->DeleteStateBlock(StateHandle));

    TheMaterial = OldMat;
    m_pD3DD->SetMaterial(&TheMaterial);
}
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
    DWORD StateHandle;
    CheckHR(m_pD3DD->CreateStateBlock(D3DSBT_ALL, &StateHandle));

    // Setup the state for the DX engine
    CheckHR(m_pD3DD->ApplyStateBlock(DxEngineStateHandle));

    CheckHR(m_pD3DD->SetRenderState(D3DRENDERSTATE_ZENABLE, D3DZB_TRUE));

    // *** Default engine initializations ***
    m_pD3DD->SetRenderState(D3DRENDERSTATE_COLORVERTEX, TRUE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_AMBIENT, 0xff000000);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR2);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_CLIPPING, FALSE);

    // Initialize data parameters
    FlushInit();

#ifndef DEBUG_ENGINE
    m_pD3DD->SetRenderState(D3DRENDERSTATE_RANGEFOGENABLE, TRUE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_NONE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGVERTEXMODE, D3DFOG_LINEAR);

    // New Fog stuff
    if (m_LinearFog)
    {
        m_LinearFogLevel = realWeather->LinearFogEnd();
        m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGEND, *(DWORD *)(&m_FogLevel));
    }
    else
    {
        m_LinearFogLevel = MAX_FOG_RANGE;
    }

    FogStart = 0.0f;
    m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGSTART, *(DWORD *)(&FogStart));
#endif

    // Initalize last specularity
    m_LastSpecular = 0;

    // Set Up the View port
    m_pD3DD->SetViewport(&ViewPort);

    // Set Up the Field of View Projection
    m_pD3DD->SetTransform(D3DTRANSFORMSTATE_PROJECTION, (LPD3DMATRIX)&Projection);

    // Set Up the camera View for the drawing
    m_pD3DD->SetTransform(D3DTRANSFORMSTATE_VIEW, (LPD3DMATRIX)&CameraView);


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

    // Restore Previous State Block and delete it from memory
    CheckHR(m_pD3DD->ApplyStateBlock(StateHandle));
    CheckHR(m_pD3DD->DeleteStateBlock(StateHandle));

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
    m_pD3DD->SetTransform(D3DTRANSFORMSTATE_WORLD, (LPD3DMATRIX)&AppliedState);

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
