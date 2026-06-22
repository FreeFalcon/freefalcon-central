#include <math.h>
#include "../include/ObjectInstance.h"
#include "dxdefines.h"
#include "DXVBManager.h"
extern bool g_bUseD3D11;	// PHASE 4
#include "mmsystem.h"
#include "../include/TexBank.h"
#include "dxengine.h"
#include "d3d11/D3D11Renderer.h"	// #27: DrawDynamic2D/BeginDynamic2D + g_pD3D11Renderer
#include "../include/ObjectLOD.h"
#include "../../falclib/include/token.h"
#include "../../falclib/include/falclib.h"
#include "../../falclib/include/f4find.h"
#include "../../falclib/include/fakerand.h"
#ifndef DEBUG_ENGINE
#include "../include/Realweather.h"
#endif
#include "../../include/ComSup.h"

//#define DRAW_USING_2D_FANS
extern bool g_bGreyMFD;
extern bool bNVGmode;


#undef DEBUG_2D_ENGINE
#ifdef DEBUG_2D_ENGINE
DWORD Debug_Vertices2D;
#endif

DWORD CDXEngine::IndexStart;

_MM_ALIGN16 XMMVector CDXEngine::vbb0, CDXEngine::vbb1, CDXEngine::vbb2, CDXEngine::vbb3; // The Vertices
_MM_ALIGN16 XMMVector CDXEngine::BBvbb0, CDXEngine::BBvbb1, CDXEngine::BBvbb2, CDXEngine::BBvbb3; // The Vertices used for BillBoarding

_MM_ALIGN16 __m128 XMMAcc, XMMAcc1;
_MM_ALIGN16 XMMVector XMMPos, XMMRadius, XMMStore;

// This are the XMM ordered Matrix CXes used to BB Stuff
_MM_ALIGN16 XMMVector CDXEngine::BBCx[3];
// This is the Distance coming from visibility chek for future uses
float CDXEngine::TestDistance;






///////////////////////////// TEXTURES MANAGEMEND FOR 2D FEATURES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\


// This function load and create a Texture, look for its items definition file and add them
void CDXEngine::LoadTexture(char *FileName)
{
#ifndef DEBUG_ENGINE
    char Path[256];
    char Buffer[1024];
    FILE *fp;
    CTextureSurface *Ts = TexturesList;
    CTextureItem *Ti;


    // ok...The surface manager
    if ( not Ts) Ts = TexturesList = new CTextureSurface();
    else
    {
        // look for last texture manager
        while (Ts->Next) Ts = Ts->Next;

        // append the new one
        Ts->Next = new CTextureSurface();
        Ts = Ts->Next;
    }

    // ok, setup the file name for the texture
    strncpy(Ts->FileName, FileName, sizeof(Ts->FileName) - 5);
    strcat(Ts->FileName, ".DDS");

    // Default to a 1st Item with texture Name and full surface covering
    Ti = Ts->ItemList = new CTextureItem;
    strncpy(Ti->Name, FileName, sizeof(Ti->Name));
    Ti->TuTv[0][0] = 0.0f;
    Ti->TuTv[0][1] = 0.0f;
    Ti->TuTv[1][0] = 1.0f;
    Ti->TuTv[1][1] = 0.0f;
    Ti->TuTv[2][0] = 1.0f;
    Ti->TuTv[2][1] = 1.0f;
    Ti->TuTv[3][0] = 0.0f;
    Ti->TuTv[3][1] = 1.0f;

    // look for the Texture Items name
    sprintf(Path, "%s\\terrdata\\MiscTex\\%s.ITM", FalconDataDirectory, FileName);
    fp = fopen(Path, "r");

    // if not found return
    if ( not fp) return;

    char *Name;
    int  b;
    float Unit = 1.0f;

    // ok, now the parsing
    while (1)
    {
        // if EOF exit here
        if ( not fgets(Buffer, sizeof Buffer, fp))
        {
            fclose(fp);
            return;
        }

        // Skip if a comment, or a New Line
        if (Buffer[0] == '#' or Buffer[0] == ';' or Buffer[0] == '\n')
            continue;

        // Skip initial Spaces or TABs
        for (b = 0; b < sizeof(Buffer) and (Buffer[b] == ' ' or Buffer[b] == '\t'); b++);

        // Ok, get the Item Name
        Name = strtok(&Buffer[b], "=\n");

        // Check if Unit Command
        if ( not strcmp(Name, "Unit"))
        {
            Unit = TokenF(0);
            continue;
        }

        if (Name)
        {
            Ti = Ts->ItemList;

            // look for last texture manager
            while (Ti->Next) Ti = Ti->Next;

            // append the new one
            Ti->Next = new CTextureItem();
            Ti = Ti->Next;
        }

        strncpy(Ti->Name, Name, sizeof(Ti->Name));
        // Get the U/V Coords
        float PosX = TokenF(0) * Unit, PosY = TokenF(0) * Unit, SizeX = TokenF(0) * Unit, SizeY = TokenF(0) * Unit;
        Ti->TuTv[0][0] = PosX;
        Ti->TuTv[0][1] = PosY;
        Ti->TuTv[1][0] = PosX + SizeX;
        Ti->TuTv[1][1] = PosY;
        Ti->TuTv[2][0] = PosX + SizeX;
        Ti->TuTv[2][1] = PosY + SizeY;
        Ti->TuTv[3][0] = PosX;
        Ti->TuTv[3][1] = PosY + SizeY;
    }

#endif
}



DWORD_PTR CDXEngine::GetTextureHandle(char *TexName) // Artscout - 2026 (x64): pointer-sized
{

    // look for an item owning such a name
    CTextureSurface *Ts = TexturesList;
    CTextureItem *Ti;

    // Look thru the list
    while (Ts)
    {
        // look thru loaded texture surfaces
        Ti = Ts->ItemList;

        // for an item owning such name
        while (Ti)
        {
            // if found, return the surface handle
            if ( not strcmp(Ti->Name, TexName)) return Ts->Tex.TexHandle();

            // else next item
            Ti = Ti->Next;
        }

        Ts = Ts->Next;
    }

    // if here, no texture item owning such name found
    return NULL;

}


void CDXEngine::SetupTexturesOnDevice(void)
{
    CTextureSurface *Ts = TexturesList;

    while (Ts)
    {
        Ts->Tex.LoadAndCreate(Ts->FileName, MPR_TI_DDS);
        Ts = Ts->Next;
    }

    CreateZeroTexture();
}


void CDXEngine::CleanUpTexturesOnDevice(void)
{
    if ( not TexturesList)
        return;

    CTextureSurface *Ts = TexturesList;

    // Release all Textures
    while (Ts)
    {
        Ts->Tex.FreeAll();
        Ts = Ts->Next;
    }

    if (ZeroTex)
    {
        //ZeroTex->m_pDDS->Release();
        delete ZeroTex;
        ZeroTex = NULL;
    }

}



void CDXEngine::DX2D_GetTextureCoords(CTextureItem *Ti, CDrawBaseItem *Item)
{
    if ( not Ti) return;

    // Assign vertices of the Passed texture
    for (int a = 0; a < 4; a++)
    {
        Item->Vtx[a].tu = Ti->TuTv[a][0];
        Item->Vtx[a].tv = Ti->TuTv[a][1];
    }
}



void CDXEngine::DX2D_GetTextureUV(CTextureItem *Ti, DWORD Index, float &u, float &v)
{
    if ( not Ti) return;

    u = Ti->TuTv[Index][0];
    v = Ti->TuTv[Index][1];
}




CTextureItem *CDXEngine::DX2D_GetTextureItem(char *TexName)
{
    // look for an item owning such a name
    CTextureSurface *Ts = TexturesList;
    CTextureItem *Ti;

    // Look thru the list
    while (Ts)
    {
        // look thru loaded texture surfaces
        Ti = Ts->ItemList;

        // for an item owning such name
        while (Ti)
        {
            // if found, assign coords and exit
            if ( not strcmp(Ti->Name, TexName)) return(Ti);

            // else next item
            Ti = Ti->Next;
        }

        Ts = Ts->Next;
    }

    return NULL;
}


void CDXEngine::ReleaseTextures(void)
{
    CTextureSurface *Ts = TexturesList, *Tsl;
    CTextureItem *Ti, *Tl;

    // Release all Textures
    while (Ts)
    {
        Ti = Ts->ItemList;

        // Release all items of the texture surface
        while (Tl = Ti)
        {
            Ti = Ti->Next;
            delete Tl;
        }

        Ts->Tex.FreeAll();
        Tsl = Ts;
        Ts = Ts->Next;
        delete Tsl;
    }

    TexturesList = NULL;
}



///////////////////////////////////////// 2D STUFF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\


DWORD CDXEngine::LayerSelected;
DWORD CDXEngine::Total2DVertices;
DWORD CDXEngine::Total2DItems;
DWORD CDXEngine::VBSelected;
float CDXEngine::Radius2D;
LayerItemType CDXEngine::Layers[MAX_2D_LAYERS];
Dyn2DBufferType CDXEngine::Dyn2DVertexBuffer[MAX_2D_BUFFERS];
DrawItemType CDXEngine::Draws2D[MAX_2D_ITEMS];
// index buffer, let a little overhead to avoid limi checkings
unsigned short CDXEngine::DrawIndexes[MAX_VERTICES_PER_DRAW + 32];
DWORD CDXEngine::Indexed2D;
SortItemType CDXEngine::SortBuffer[MAX_2D_ITEMS];
// The Radix Sort Tables
DWORD CDXEngine::SortBuckets[4][256];
DWORD CDXEngine::SortTail[4][256];
DWORD CDXEngine::DrawOrder[MAX_2D_LAYERS];
static const DWORD DefOrder[] = {LAYER_ROOF, LAYER_STRATUS2, LAYER_MIDDLE, LAYER_STRATUS1, LAYER_GROUND, LAYER_TOP, LAYER_NODRAW};


// reset function before any use
void CDXEngine::DX2D_InitLists(void)
{
    // reset variables
    LayerSelected = Total2DVertices = Total2DItems = VBSelected = 0;
    // Reset layers to UNINITIALIZED
    memset(Layers, 0xff, sizeof(Layers));

    // Layers Flags
    for (int l = 0; l < MAX_2D_LAYERS; l++) Layers[l].Flags = 0;

    // Default Draw Order
    memcpy(DrawOrder, DefOrder, sizeof(DrawOrder));

}

// reset function before any use
void CDXEngine::DX2D_Reset(void)
{
    // Reinitialize Lists
    DX2D_InitLists();
    // reset the Draw Order
    memset(DrawOrder, 0xff, sizeof(DrawOrder));

    // and reset all Vertex Buffers to be filled in
    for (int i = 0; i < MAX_2D_BUFFERS; i++)
    {
        Dyn2DVertexBuffer[i].LastIndex = Dyn2DVertexBuffer[i].LastTapeIndex = 0;
        if ( not g_bUseD3D11)	// PHASE 4b: the DX 2D engine is disabled under D3D11 (Vb=NULL)
            Dyn2DVertexBuffer[i].Vb->Lock(DDLOCK_DISCARDCONTENTS bitor DDLOCK_NOSYSLOCK bitor DDLOCK_WAIT bitor DDLOCK_WRITEONLY, (void**)&Dyn2DVertexBuffer[i].VbPtr, NULL);;
    }
}


// #27 D3D11: frustum sphere culling -- replaces D3D7 ComputeSphereVisibility.
// M = CameraView*Projection (row-major, the shader applies clip = worldPos*M, world=identity).
// Gribb-Hartmann planes for the v*M convention: colK=(m0K,m1K,m2K,m3K).
// The sphere is VISIBLE if its center is no farther than -radius from any plane (conservative:
// cull only fully-outside ones -> visible particles never disappear).
static bool DX2D_SphereVisibleD3D11(const D3DXMATRIX &View, const D3DXMATRIX &Proj,
                                    float px, float py, float pz, float r)
{
    D3DXMATRIX M;
    D3DXMatrixMultiply(&M, &View, &Proj);
    const float pl[6][4] =
    {
        { M.m00 + M.m03, M.m10 + M.m13, M.m20 + M.m23, M.m30 + M.m33 }, // left  (x>=-w)
        { M.m03 - M.m00, M.m13 - M.m10, M.m23 - M.m20, M.m33 - M.m30 }, // right (x<= w)
        { M.m01 + M.m03, M.m11 + M.m13, M.m21 + M.m23, M.m31 + M.m33 }, // bottom(y>=-w)
        { M.m03 - M.m01, M.m13 - M.m11, M.m23 - M.m21, M.m33 - M.m31 }, // top   (y<= w)
        { M.m02,         M.m12,         M.m22,         M.m32         }, // near  (z>= 0)
        { M.m03 - M.m02, M.m13 - M.m12, M.m23 - M.m22, M.m33 - M.m32 }, // far   (z<= w)
    };
    for (int i = 0; i < 6; ++i)
    {
        const float a = pl[i][0], b = pl[i][1], c = pl[i][2], d = pl[i][3];
        const float len = sqrtf(a * a + b * b + c * c);
        if (len < 1e-6f) continue;
        if ((px * a + py * b + pz * c + d) / len < -r) return false;
    }
    return true;
}

// This function returns the visibility for an objects of a certain radius in a certain Pos
// WARNING  This function stores the calculated position for following uses in XMMPos variable
// as we suppose calculating the visibility is just before rendering same item
// returns the Distance from Camera, -1 if out of FOV
bool CDXEngine::DX2D_GetVisibility(D3DXVECTOR3 *Pos, float Radius, DWORD Flags)
{
    DWORD ClipResult;
    //Store the radius
    Radius2D = Radius;
    // get the position and make it camera relative
    XMMPos.Xmm = _mm_loadu_ps((float*)Pos);

    if ( not (Flags bitand CAMERA_VERTICES)) XMMPos.Xmm = _mm_sub_ps(XMMPos.Xmm, XMMCamera.Xmm);

    // Check for object visibility, return NULL is not visible
    if (g_bUseD3D11)
    {
        if (Flags bitand CAMERA_VERTICES) return true;   // verts already in camera space
        // #13/clouds: CameraView is rotation ONLY (no translation), so the frustum is centered
        // at the origin -> we must test the CAMERA-RELATIVE point (XMMPos = Pos - Camera,
        // see above), not the world Pos. Previously world was passed -> visibility depended on HEADING
        // (clouds came and went). The D3D7 path below also uses XMMPos.d3d. (per report 2026-06-17)
        return DX2D_SphereVisibleD3D11(CameraView, Projection, XMMPos.d3d.x, XMMPos.d3d.y, XMMPos.d3d.z, Radius);
    }
    return true;   // #34 dead D3D7 ComputeSphereVisibility path removed (D3D11 returns above)
}



DWORD CDXEngine::ComputeSphereVisibility(LPD3DVECTOR lpCenters, LPD3DVALUE  lpRadii, DWORD dwNumSpheres)
{
    DWORD ClipResult;
    XMMAcc = _mm_loadu_ps((float*)lpCenters);
    XMMPos.Xmm = _mm_sub_ps(XMMAcc, XMMCamera.Xmm);
    // Check for object visibility, return NULL is not visible
    if (g_bUseD3D11)
        // #13: camera-relative point (XMMPos = lpCenters - Camera, see above), not world -- see comment in DX2D_GetVisibility
        return DX2D_SphereVisibleD3D11(CameraView, Projection, XMMPos.d3d.x, XMMPos.d3d.y, XMMPos.d3d.z,
                                       lpRadii ? lpRadii[0] : 0.0f) ? 0 : D3DSTATUS_DEFAULT;
    return 0;   // #34 dead D3D7 ComputeSphereVisibility path removed (D3D11 returns above)
}


float CDXEngine::DX2D_GetDistance(D3DXVECTOR3 *Pos, float Radius, DWORD Flags)
{
    DWORD ClipResult;
    //Store the radius
    Radius2D = Radius;
    // get the position and make it camera relative
    XMMPos.Xmm = _mm_loadu_ps((float*)Pos);

    if ( not (Flags bitand CAMERA_VERTICES))XMMPos.Xmm = _mm_sub_ps(XMMPos.Xmm, XMMCamera.Xmm);

    // Check for object visibility, return -1 if not visible (out of frustum)
    if (g_bUseD3D11)
    {
        if ( not (Flags bitand CAMERA_VERTICES)
             and not DX2D_SphereVisibleD3D11(CameraView, Projection, XMMPos.d3d.x, XMMPos.d3d.y, XMMPos.d3d.z, Radius))	// #13: camera-relative, not world (CameraView has no translation)
            return -1.0f;
    }
    // #34 dead D3D7 ComputeSphereVisibility else-branch removed (D3D11 path above)

    // setup the DISTANCE FROM CAMERA
    XMMStore.Xmm = _mm_mul_ps(XMMPos.Xmm, XMMPos.Xmm);
    return TestDistance = sqrtf(XMMStore.d3d.x + XMMStore.d3d.y + XMMStore.d3d.z);
}


float CDXEngine::DX2D_GetDistance(D3DXVECTOR3 *Pos, DWORD Flags)
{
    // get the position and make it camera relative
    XMMPos.Xmm = _mm_loadu_ps((float*)Pos);

    if ( not (Flags bitand CAMERA_VERTICES))XMMPos.Xmm = _mm_sub_ps(XMMPos.Xmm, XMMCamera.Xmm);

    // setup the DISTANCE FROM CAMERA
    XMMStore.Xmm = _mm_mul_ps(XMMPos.Xmm, XMMPos.Xmm);
    return TestDistance = sqrtf(XMMStore.d3d.x + XMMStore.d3d.y + XMMStore.d3d.z);
}


void CDXEngine::DX2D_ForceDistance(float Distance)
{
    TestDistance = Distance;
}



void CDXEngine::DX2D_GetRelativePosition(D3DXVECTOR3 *Pos)
{
    Pos->x = XMMPos.d3d.x;
    Pos->y = XMMPos.d3d.y;
    Pos->z = XMMPos.d3d.z;
}




void CDXEngine::DX2D_MakeCameraSpace(D3DXVECTOR3 *Result, D3DXVECTOR3 *Pos)
{
    Result->x = (float)((double) Pos->x - (double) XMMCamera.d3d.x);
    Result->y = (float)((double) Pos->y - (double) XMMCamera.d3d.y);
    Result->z = (float)((double) Pos->z - (double) XMMCamera.d3d.z);
}



inline bool CDXEngine::CheckBufferSpace(DWORD VbIndex, DWORD Size)
{
    // Buffer full -> drop the remaining primitives of the frame.
    // PREVIOUSLY: VBSelected++ on overflow, but MAX_2D_BUFFERS=1 -> VBSelected=1, and
    // the next Add* accessed Dyn2DVertexBuffer[1] (out of the static array) -> OOB.
    // The multi-buffer mechanism is vestigial (1 buffer) -- just drop, leave VBSelected alone.
    if (VbIndex + Size >= MAX_2D_VERTICES) return false;
    if (VBSelected >= MAX_2D_BUFFERS) return false;

    return true;
}


// This function add a Quad to the vertex buffers and sorting list...
// WARNING  Does not check for Visibility, call DX2D_GetVisibility() or DX2D_SetupQuad before...
void CDXEngine::DX2D_AddQuad(DWORD Layer, DWORD Flags, D3DXVECTOR3 *Pos, D3DDYNVERTEX *Quad, float Radius, DWORD_PTR TexHandle)
{
    // #27 D3D11: accumulate in the CPU VbPtr (DX2D_Init), draw in DX2D_Flush2DObjects via DrawDynamic2D.
    _MM_ALIGN16 XMMVector V[4];

#ifdef DATE_PROTECTION
    extern bool DateOff;

    if (DateOff and PRANDFloat() < 0.3f) return;

#endif

    // not going to overflow stuff
    if (Total2DItems >= MAX_2D_ITEMS) return;

    // Get the Index for the selected VB
    DWORD &VbIndex = Dyn2DVertexBuffer[VBSelected].LastIndex;

    // if no more space, exit
    if ( not CheckBufferSpace(VbIndex, 4)) return;

    // Get Distance from a previous test if POLY DECLARED VISIBLE, or calcualte if from scratch
    float Distance;

    if (Flags bitand POLY_VISIBLE) Distance = TestDistance;
    else
    {
        Distance = DX2D_GetDistance(Pos, Radius, Flags);

        if (Distance < 0.0f) return;
    }

    // if Camera vertices, the passed position is the real one
    if (Flags bitand CAMERA_VERTICES) *(D3DXVECTOR3*)&XMMPos.d3d = *Pos;

    if (Flags bitand CALC_DISTANCE) Distance = DX2D_GetDistance(Pos, Flags);

    // check if layer initialized, if not, initialize it
    if (Layers[Layer].Start == -1) Layers[Layer].Start = Total2DItems;
    // If layer already initialized
    else
    {
        // get a pointer to last draw for the layer
        DWORD Index = Layers[Layer].End;
        // Link new Draw Item
        Draws2D[Index].Next = Total2DItems;
    }

    // This is the last Draw for the Layer
    Layers[Layer].End = Total2DItems;

    // * VERTEX CREATION REQUIRED ONLY FOR QUAD - IT CAMES FROM RADIUS *
    if (Flags bitand POLY_CREATE)
    {
        // Prepare radius CX
        XMMRadius.d3d.x = XMMRadius.d3d.y = XMMRadius.d3d.z = Radius;
        V[0].Xmm = _mm_mul_ps(XMMRadius.Xmm, vbb0.Xmm);
        V[1].Xmm = _mm_mul_ps(XMMRadius.Xmm, vbb1.Xmm);
        V[2].Xmm = _mm_mul_ps(XMMRadius.Xmm, vbb2.Xmm);
        V[3].Xmm = _mm_mul_ps(XMMRadius.Xmm, vbb3.Xmm);
    }
    else
    {
        V[0].Xmm = _mm_loadu_ps((float*)&Quad[0].pos);
        V[1].Xmm = _mm_loadu_ps((float*)&Quad[1].pos);
        V[2].Xmm = _mm_loadu_ps((float*)&Quad[2].pos);
        V[3].Xmm = _mm_loadu_ps((float*)&Quad[3].pos);
    }

    // * BILLBOARD VERTICES * - go directly into Vertex Buffer
    if (Flags bitand POLY_BB) DX2D_TransformBB(&XMMPos, V, &Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex], 4);
    // if not BillBoarded, add Distance here and put into Vertex Buffer
    else
    {
        // if passed coords in already camera vertex, just copy
        if (Flags bitand CAMERA_VERTICES)
        {
            _mm_storeu_ps((float*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex + 0], V[0].Xmm);
            _mm_storeu_ps((float*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex + 1], V[1].Xmm);
            _mm_storeu_ps((float*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex + 2], V[2].Xmm);
            _mm_storeu_ps((float*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex + 3], V[3].Xmm);

        }
        else
        {
            _mm_storeu_ps((float*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex + 0], _mm_add_ps(XMMPos.Xmm, V[0].Xmm));
            _mm_storeu_ps((float*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex + 1], _mm_add_ps(XMMPos.Xmm, V[1].Xmm));
            _mm_storeu_ps((float*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex + 2], _mm_add_ps(XMMPos.Xmm, V[2].Xmm));
            _mm_storeu_ps((float*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex + 3], _mm_add_ps(XMMPos.Xmm, V[3].Xmm));
        }
    }

    D3DDYNVERTEX *ptr = (D3DDYNVERTEX*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex].dwColour;
    D3DDYNVERTEX *src = (D3DDYNVERTEX*)&Quad[0].dwColour;

    // copy the Rest of Vertex Data
    _mm_storeu_ps((float*)ptr++, _mm_loadu_ps((float*)src++));
    _mm_storeu_ps((float*)ptr++, _mm_loadu_ps((float*)src++));
    _mm_storeu_ps((float*)ptr++, _mm_loadu_ps((float*)src++));
    _mm_storeu_ps((float*)ptr++, _mm_loadu_ps((float*)src++));


    // get the Draw under setting
    DrawItemType &Draw = Draws2D[Total2DItems];
    // * setup the items to track bitand sort the Quad *
    // The Scaled Distance for sorting
    Draw.Dist256 = F_I32(Distance * 256.0f);
    // The Texture Handle
    Draw.TexHandle = TexHandle;
    // The vertex buffer assigned
    Draw.Vb = Dyn2DVertexBuffer[VBSelected].Vb;
    // Index of vertices in the Vertex Buffer
    Draw.Index = VbIndex;
    // vertices of the item, vertices for a quad are 6 ( 2 triangles )
#ifdef DRAW_USING_2D_FANS
    Draw.NrVertices = 4;
#else
    Draw.NrVertices = 6;
#endif
    // Final Item in the list
    Draw.Next = 0xffffffff;
    // Assign Flags from the draw
    Draw.Flags = Flags;
    // Assign height
    Draw.Height = Pos->z;

    // if local coords, update with camera Z
    if (Flags bitand CAMERA_VERTICES) Draw.Height += CameraPos.z;

    // Update the Sort Buffer
    SortBuffer[Total2DItems].Index = Total2DItems;
    // new Quad
    Total2DItems++;
    // new index in VB, if full get next VBuffer
    VbIndex += 4;

#ifdef DEBUG_2D_ENGINE
    Debug_Vertices2D += 4;
#endif
}



// This function add a Quad to the vertex buffers and sorting list...
// WARNING  Does not check for Visibility, call DX2D_GetVisibility() or DX2D_SetupQuad before...
void CDXEngine::DX2D_AddTri(DWORD Layer, DWORD Flags, D3DXVECTOR3 *Pos, D3DDYNVERTEX *Tri, float Radius, DWORD_PTR TexHandle)
{
    // #27 D3D11: accumulate in the CPU VbPtr, draw via DrawDynamic2D in DX2D_Flush2DObjects.
    _MM_ALIGN16 XMMVector V[4];

    // not going to overflow stuff
    if (Total2DItems >= MAX_2D_ITEMS) return;

    // Get the Index for the selected VB
    DWORD &VbIndex = Dyn2DVertexBuffer[VBSelected].LastIndex;

    // if no more space, exit
    if ( not CheckBufferSpace(VbIndex, 3)) return;

    // Get Distance from a previous test if POLY DECLARED VISIBLE, or calcualte if from scratch
    float Distance = (Flags bitand POLY_VISIBLE) ? TestDistance : DX2D_GetDistance(Pos, Radius);

    if (Distance < 0.0f) return;

    // check if layer initialized, if not, initialize it
    if (Layers[Layer].Start == -1) Layers[Layer].Start = Total2DItems;
    // If layer already initialized
    else
    {
        // get a pointer to last draw for the layer
        DWORD Index = Layers[Layer].End;
        // Link new Draw Item
        Draws2D[Index].Next = Total2DItems;
    }

    // This is the last Draw for the Layer
    Layers[Layer].End = Total2DItems;

    V[0].Xmm = _mm_loadu_ps((float*)&Tri[0].pos);
    V[1].Xmm = _mm_loadu_ps((float*)&Tri[1].pos);
    V[2].Xmm = _mm_loadu_ps((float*)&Tri[2].pos);

    // * BILLBOARD VERTICES * - go directly into Vertex Buffer
    if (Flags bitand POLY_BB) DX2D_TransformBB(&XMMPos, V, &Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex], 3);
    // if not BillBoarded, add Distance here and put into Vertex Buffer
    else
    {
        _mm_storeu_ps((float*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex + 0], _mm_add_ps(XMMPos.Xmm, V[0].Xmm));
        _mm_storeu_ps((float*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex + 1], _mm_add_ps(XMMPos.Xmm, V[1].Xmm));
        _mm_storeu_ps((float*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex + 2], _mm_add_ps(XMMPos.Xmm, V[2].Xmm));
    }

    // copy the Rest of Vertex Data
    _mm_storeu_ps((float*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex + 0].dwColour, _mm_loadu_ps((float*)&Tri[0].dwColour));
    _mm_storeu_ps((float*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex + 1].dwColour, _mm_loadu_ps((float*)&Tri[1].dwColour));
    _mm_storeu_ps((float*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex + 2].dwColour, _mm_loadu_ps((float*)&Tri[2].dwColour));


    // get the Draw under setting
    DrawItemType &Draw = Draws2D[Total2DItems];
    // * setup the items to track bitand sort the Quad *
    // The Scaled Distance for sorting
    Draw.Dist256 = F_I32(Distance * 256.0f);
    // The Texture Handle
    Draw.TexHandle = TexHandle;
    // The vertex buffer assigned
    Draw.Vb = Dyn2DVertexBuffer[VBSelected].Vb;
    // Index of vertices in the Vertex Buffer
    Draw.Index = VbIndex;
    // vertices of the item, vertices for a Tri
    Draw.NrVertices = 3;
    // Final Item in the list
    Draw.Next = 0xffffffff;
    // Assign Flags from the draw
    Draw.Flags = Flags;
    // Assign height
    Draw.Height = Pos->z;

    // if local coords, update with camera Z
    if (Flags bitand CAMERA_VERTICES) Draw.Height += CameraPos.z;

    // Update the Sort Buffer
    SortBuffer[Total2DItems].Index = Total2DItems;
    // new Quad
    Total2DItems++;
    // new index in VB, if full get next VBuffer
    VbIndex += 3;

#ifdef DEBUG_2D_ENGINE
    Debug_Vertices2D += 3;
#endif
}


// This function add a 2 vertex element to the vertex buffers and sorting list...
// WARNING  Does not check for Visibility, call DX2D_GetVisibility() or DX2D_SetupQuad before...
void CDXEngine::DX2D_AddBi(DWORD Layer, DWORD Flags, D3DXVECTOR3 *Pos, D3DDYNVERTEX *Segment, float Radius, DWORD_PTR TexHandle)
{
    // #27 D3D11: accumulate in the CPU VbPtr, draw via DrawDynamic2D in DX2D_Flush2DObjects.
    _MM_ALIGN16 XMMVector V[2];

    // not going to overflow stuff
    if (Total2DItems >= MAX_2D_ITEMS) return;

    // Get the Index for the selected VB
    DWORD &VbIndex = Dyn2DVertexBuffer[VBSelected].LastIndex;

    // if no more space, exit
    if ( not CheckBufferSpace(VbIndex, 2)) return;

    // Get Distance from a previous test if POLY DECLARED VISIBLE, or calcualte if from scratch
    float Distance;

    if (Flags bitand POLY_VISIBLE) Distance = TestDistance;
    else
    {
        Distance = DX2D_GetDistance(Pos, Radius, Flags);

        if (Distance < 0.0f) return;
    }

    // if Camera vertices, the passed position is the real one
    if (Flags bitand CAMERA_VERTICES) *(D3DXVECTOR3*)&XMMPos.d3d = *Pos;

    if (Flags bitand CALC_DISTANCE) Distance = DX2D_GetDistance(Pos, Flags);

    // check if layer initialized, if not, initialize it
    if (Layers[Layer].Start == -1) Layers[Layer].Start = Total2DItems;
    // If layer already initialized
    else
    {
        // get a pointer to last draw for the layer
        DWORD Index = Layers[Layer].End;
        // Link new Draw Item
        Draws2D[Index].Next = Total2DItems;
    }

    // This is the last Draw for the Layer
    Layers[Layer].End = Total2DItems;

    V[0].Xmm = _mm_loadu_ps((float*)&Segment[0].pos);
    V[1].Xmm = _mm_loadu_ps((float*)&Segment[1].pos);

    // if passed coords in already camera vertex, just copy
    if (Flags bitand CAMERA_VERTICES)
    {
        _mm_storeu_ps((float*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex + 0], V[0].Xmm);
        _mm_storeu_ps((float*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex + 1], V[1].Xmm);

    }
    else
    {
        _mm_storeu_ps((float*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex + 0], _mm_add_ps(XMMPos.Xmm, V[0].Xmm));
        _mm_storeu_ps((float*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex + 1], _mm_add_ps(XMMPos.Xmm, V[1].Xmm));
    }

    D3DDYNVERTEX *ptr = (D3DDYNVERTEX*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex].dwColour;
    D3DDYNVERTEX *src = (D3DDYNVERTEX*)&Segment[0].dwColour;

    // copy the Rest of Vertex Data
    _mm_storeu_ps((float*)ptr++, _mm_loadu_ps((float*)src++));
    _mm_storeu_ps((float*)ptr++, _mm_loadu_ps((float*)src++));

    // if this is a tape entry
    if (Flags bitand TAPE_ENTRY)
    {
        // Just store vertices and update tape pointers
        Dyn2DVertexBuffer[VBSelected].LastTapeIndex = VbIndex;
        VbIndex += 2;
    }
    else
    {
        // get the Draw under setting
        DrawItemType &Draw = Draws2D[Total2DItems];
        // * setup the items to track bitand sort the Quad *
        // The Scaled Distance for sorting
        Draw.Dist256 = F_I32(Distance * 256.0f);
        // The Texture Handle
        Draw.TexHandle = TexHandle;
        // The vertex buffer assigned
        Draw.Vb = Dyn2DVertexBuffer[VBSelected].Vb;
        // Index of vertices in the Vertex Buffer
        Draw.Index = VbIndex, Draw.Index2 = Dyn2DVertexBuffer[VBSelected].LastTapeIndex;

        // vertices of the item, vertices for a quad are 6 ( 2 triangles )
        if (Flags bitand POLY_LINE) Draw.NrVertices = 2;

        if (Flags bitand POLY_TAPE) Draw.NrVertices = 6;

        // Final Item in the list
        Draw.Next = 0xffffffff;
        // Assign Flags from the draw
        Draw.Flags = Flags;
        // Assign height
        Draw.Height = Pos->z;

        // if local coords, update with camera Z
        if (Flags bitand CAMERA_VERTICES) Draw.Height += CameraPos.z;

        // Update the Sort Buffer
        SortBuffer[Total2DItems].Index = Total2DItems;
        // new Quad
        Total2DItems++;
        Dyn2DVertexBuffer[VBSelected].LastTapeIndex = VbIndex;
        // new index in VB, if full get next VBuffer
        VbIndex += 2;
    }

#ifdef DEBUG_2D_ENGINE
    Debug_Vertices2D += 2;
#endif
}





// This function add a SINGLE VERTEX element to the vertex buffers and sorting list...
// WARNING  Does not check for Visibility, call DX2D_GetVisibility() or DX2D_SetupQuad before...
void CDXEngine::DX2D_AddSingle(DWORD Layer, DWORD Flags, D3DXVECTOR3 *Pos, D3DDYNVERTEX *Segment, float Radius, DWORD_PTR TexHandle)
{
    // #27 D3D11: accumulate in the CPU VbPtr, draw via DrawDynamic2D in DX2D_Flush2DObjects.
    _MM_ALIGN16 XMMVector V;

    // not going to overflow stuff
    if (Total2DItems >= MAX_2D_ITEMS) return;

    // Get the Index for the selected VB
    DWORD &VbIndex = Dyn2DVertexBuffer[VBSelected].LastIndex;

    // if no more space, exit
    if ( not CheckBufferSpace(VbIndex, 1)) return;

    // Get Distance from a previous test if POLY DECLARED VISIBLE, or calcualte if from scratch
    float Distance;

    if (Flags bitand POLY_VISIBLE) Distance = TestDistance;
    else
    {
        Distance = DX2D_GetDistance(Pos, Radius, Flags);

        if (Distance < 0.0f) return;
    }

    // if Camera vertices, the passed position is the real one
    if (Flags bitand CAMERA_VERTICES) *(D3DXVECTOR3*)&XMMPos.d3d = *Pos;

    if (Flags bitand CALC_DISTANCE) Distance = DX2D_GetDistance(Pos, Flags);

    // check if layer initialized, if not, initialize it
    if (Layers[Layer].Start == -1) Layers[Layer].Start = Total2DItems;
    // If layer already initialized
    else
    {
        // get a pointer to last draw for the layer
        DWORD Index = Layers[Layer].End;
        // Link new Draw Item
        Draws2D[Index].Next = Total2DItems;
    }

    // This is the last Draw for the Layer
    Layers[Layer].End = Total2DItems;

    V.Xmm = _mm_loadu_ps((float*)&Segment->pos);

    // if passed coords in already camera vertex, just copy
    if (Flags bitand CAMERA_VERTICES)
    {
        _mm_storeu_ps((float*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex + 0], V.Xmm);

    }
    else
    {
        _mm_storeu_ps((float*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex + 0], _mm_add_ps(XMMPos.Xmm, V.Xmm));
    }

    D3DDYNVERTEX *ptr = (D3DDYNVERTEX*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex].dwColour;
    D3DDYNVERTEX *src = (D3DDYNVERTEX*)&Segment->dwColour;

    // copy the Rest of Vertex Data
    _mm_storeu_ps((float*)ptr++, _mm_loadu_ps((float*)src++));

    // if this is a tape entry
    if (Flags bitand TAPE_ENTRY)
    {
        // Just store vertices and update tape pointers
        Dyn2DVertexBuffer[VBSelected].LastTapeIndex = VbIndex;
        VbIndex += 1;
    }
    else
    {
        // get the Draw under setting
        DrawItemType &Draw = Draws2D[Total2DItems];
        // * setup the items to track bitand sort the Quad *
        // The Scaled Distance for sorting
        Draw.Dist256 = F_I32(Distance * 256.0f);
        // The Texture Handle
        Draw.TexHandle = TexHandle;
        // The vertex buffer assigned
        Draw.Vb = Dyn2DVertexBuffer[VBSelected].Vb;
        // Index of vertices in the Vertex Buffer
        Draw.Index = VbIndex, Draw.Index2 = Dyn2DVertexBuffer[VBSelected].LastTapeIndex;

        // vertices of the item, vertices for a quad are 6 ( 2 triangles )
        if (Flags bitand POLY_LINE) Draw.NrVertices = 2;
        else Draw.NrVertices = 1;

        // Final Item in the list
        Draw.Next = 0xffffffff;
        // Assign Flags from the draw
        Draw.Flags = Flags;
        // Assign height
        Draw.Height = Pos->z;

        // if local coords, update with camera Z
        if (Flags bitand CAMERA_VERTICES) Draw.Height += CameraPos.z;

        // Update the Sort Buffer
        SortBuffer[Total2DItems].Index = Total2DItems;
        // new Quad
        Total2DItems++;
        Dyn2DVertexBuffer[VBSelected].LastTapeIndex = VbIndex;
        // new index in VB, if full get next VBuffer
        VbIndex += 1;
    }

#ifdef DEBUG_2D_ENGINE
    Debug_Vertices2D += 1;
#endif
}



void CDXEngine::DX2D_AddPoly(DWORD Layer, DWORD Flags, D3DXVECTOR3 *Pos, D3DDYNVERTEX *Poly, float Radius, DWORD Vertices, DWORD_PTR TexHandle)
{
    // #27 D3D11: accumulate in the CPU VbPtr, draw via DrawDynamic2D in DX2D_Flush2DObjects.
    _MM_ALIGN16 XMMVector V;


    // not going to overflow stuff
    if (Total2DItems >= MAX_2D_ITEMS) return;

    // Get the Index for the selected VB
    DWORD &VbIndex = Dyn2DVertexBuffer[VBSelected].LastIndex;

    // if no more space, exit
    if ( not CheckBufferSpace(VbIndex, Vertices)) return;

    // Get Distance from a previous test if POLY DECLARED VISIBLE, or calcualte if from scratch
    float Distance = (Flags bitand POLY_VISIBLE) ? TestDistance : DX2D_GetDistance(Pos, Radius);

    if (Distance < 0.0f) return;

    // check if layer initialized, if not, initialize it
    if (Layers[Layer].Start == -1) Layers[Layer].Start = Total2DItems;
    // If layer already initialized
    else
    {
        // get a pointer to last draw for the layer
        DWORD Index = Layers[Layer].End;
        // Link new Draw Item
        Draws2D[Index].Next = Total2DItems;
    }

    // This is the last Draw for the Layer
    Layers[Layer].End = Total2DItems;

    for (DWORD a = 0; a < Vertices; a++)
    {
        V.Xmm = _mm_loadu_ps((float*)&Poly[a].pos);

        // * BILLBOARD VERTICES * - go directly into Vertex Buffer
        if (Flags bitand POLY_BB) DX2D_TransformBB(&XMMPos, &V, &Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex + a], 1);
        // if not BillBoarded, add Distance here and put into Vertex Buffer
        else _mm_storeu_ps((float*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex + a], _mm_add_ps(XMMPos.Xmm, V.Xmm));

        // copy the Rest of Vertex Data
        _mm_storeu_ps((float*)&Dyn2DVertexBuffer[VBSelected].VbPtr[VbIndex + a].dwColour, _mm_loadu_ps((float*)&Poly[a].dwColour));

    }

    // get the Draw under setting
    DrawItemType &Draw = Draws2D[Total2DItems];
    // * setup the items to track bitand sort the Quad *
    // The Scaled Distance for sorting
    Draw.Dist256 = F_I32(Distance * 256.0f);
    // The Texture Handle
    Draw.TexHandle = TexHandle;
    // The vertex buffer assigned
    Draw.Vb = Dyn2DVertexBuffer[VBSelected].Vb;
    // Index of vertices in the Vertex Buffer
    Draw.Index = VbIndex;
    // vertices of the item, vertices for a Tri
    Draw.NrVertices = Vertices;
    // Final Item in the list
    Draw.Next = 0xffffffff;
    // Assign Flags from the draw
    Draw.Flags = Flags;
    // Assign height
    Draw.Height = Pos->z;

    // if local coords, update with camera Z
    if (Flags bitand CAMERA_VERTICES) Draw.Height += CameraPos.z;

    // Update the Sort Buffer
    SortBuffer[Total2DItems].Index = Total2DItems;
    // new Quad
    Total2DItems++;
    // new index in VB, if full get next VBuffer
    VbIndex += Vertices;
#ifdef DEBUG_2D_ENGINE
    Debug_Vertices2D += Vertices;
#endif
}



// function inserting a 3D object into the Alpha sorting list
void CDXEngine::DX2D_AddObject(DWORD ID, DWORD Layer, SurfaceStackType *Stack, D3DXVECTOR3 *Pos)
{

    // Get Distance from a previous test if POLY DECLARED VISIBLE, or calcualte if from scratch
    float Distance = DX2D_GetDistance(Pos, (DWORD)CAMERA_VERTICES);

    // check if layer initialized, if not, initialize it
    if (Layers[Layer].Start == -1) Layers[Layer].Start = Total2DItems;
    // If layer already initialized
    else
    {
        // get a pointer to last draw for the layer
        DWORD Index = Layers[Layer].End;
        // Link new Draw Item
        Draws2D[Index].Next = Total2DItems;
    }

    // This is the last Draw for the Layer
    Layers[Layer].End = Total2DItems;

    // get the Draw under setting
    DrawItemType &Draw = Draws2D[Total2DItems];
    // * setup the items to track bitand sort the Quad *
    // The Scaled Distance for sorting
    Draw.Dist256 = F_I32(Distance * 256.0f);
    // The Texture Handle
    Draw.TexHandle = NULL;
    // The vertex buffer assigned
    Draw.Vb = (LPDIRECT3DVERTEXBUFFER7)Stack;
    // Index of vertices in the Vertex Buffer
    Draw.Index = 0;
    // vertices of the item, vertices for a Tri
    Draw.NrVertices = 0;
    // Final Item in the list
    Draw.Next = 0xffffffff;
    // Assign Flags from the draw
    Draw.Flags = POLY_3DOBJECT bitor ID;
    // Assign height, Add camera Offset as height is used to evaluate
    // Vertical object position, and assign right layer
    Draw.Height = Pos->z + CameraPos.z;

    // Update the Sort Buffer
    SortBuffer[Total2DItems].Index = Total2DItems;
    // new item to be sorted
    Total2DItems++;
}







// This function generates Indexes built from the sorting list
// returns the ending index
DWORD CDXEngine::DX2D_GenerateIndexes(DWORD Start)
{
    DWORD Index = 0;
    // Offset for vertex indexes for both triangles and Quads
    DWORD VOffsets[] = {0, 1, 2, 0, 2, 3};
    // reset the indexed vertices counter
    Indexed2D = 0;
    // The mode flags
    bool LineMode = false, DotMode = false;

#if MAX_2D_BUFFERS > 1
    // Setup the starting VB
    DWORD Vb = (DWORD)Draws2D[Start].Vb;
#endif
    DWORD_PTR Tex = Draws2D[Start].TexHandle; // Artscout - 2026 (x64): pointer-sized

    // Setup for lines
    if (Draws2D[Start].Flags bitand POLY_LINE) LineMode = true;

    // thru all the list
    while (Start not_eq 0xffffffff and Index < MAX_VERTICES_PER_DRAW)
    {
        DrawItemType &Draw = Draws2D[Start];
#if MAX_2D_BUFFERS > 1

        // check if changed VB, exit if changed
        if (Draw.Vb not_eq (LPDIRECT3DVERTEXBUFFER7)Vb) return Start;

#endif

        // if texture changed exit here
        if (Draw.TexHandle not_eq Tex) return Start;

        // * SORTED 3D OBJECT *
        if (Draw.Flags bitand POLY_3DOBJECT)
        {
            //if 1st item, return it
            if ( not Index) return Draw.Next;
            else return Start;
        }

        // * LINE INDEXED *
        if (Draw.Flags bitand POLY_LINE)
        {
            // if it was not a Line mode, close here
            if ( not LineMode) return Start;

            //  if a line Tape
            if (Draw.Flags bitand POLY_TAPE) DrawIndexes[Index++] = (unsigned short)Draw.Index, DrawIndexes[Index++] = (unsigned short)Draw.Index2;
            else DrawIndexes[Index++] = (unsigned short)Draw.Index, DrawIndexes[Index++] = (unsigned short)Draw.Index + 1;

            // number of indexed vertices
            Indexed2D += 2;
            // next item
            Start = Draw.Next;
            // repeat
            continue;
        }
        else

            // if we are in Line Mode, exit here
            if (LineMode) return Start;

        // * FAN INDEXED *
        if (Draw.Flags bitand POLY_FAN)
        {
            // Check if indexing overflows the draw limit
            if ((Index + (Draw.NrVertices - 2) * 3) > MAX_VERTICES_PER_DRAW) return Start;

            unsigned short  Count = (unsigned short)Draw.Index;
            unsigned short v, Center = DrawIndexes[Index++] = Count++;
            DrawIndexes[Index++] = Count++;
            v = DrawIndexes[Index++] = Count++;

            for (DWORD a = 0; a < Draw.NrVertices - 3; a++)
            {
                DrawIndexes[Index++] = Center;
                DrawIndexes[Index++] = v;
                v = DrawIndexes[Index++] = Count++;
            }

            // number of indexed vertices
            Indexed2D += (Draw.NrVertices - 2) * 3;
            // next item
            Start = Draw.Next;
            // repeat
            continue;
        }

        // * STRIP INDEXED *
        if (Draw.Flags bitand POLY_STRIP)
        {
            unsigned short  Count = (unsigned short)Draw.Index;
            DrawIndexes[Index++] = Count++;
            unsigned short v = DrawIndexes[Index++] = Count++;
            unsigned short l = DrawIndexes[Index++] = Count++;

            for (DWORD a = 0; a < Draw.NrVertices - 3; a++)
            {
                DrawIndexes[Index++] = l;
                DrawIndexes[Index++] = v;
                v = l;
                l = DrawIndexes[Index++] = Count++;
            }

            // number of indexed vertices
            Indexed2D += (Draw.NrVertices - 2) * 3;
            // next item
            Start = Draw.Next;
            // repeat
            continue;
        }

        // * TAPE INDEXED *
        if (Draw.Flags bitand POLY_TAPE)
        {
            unsigned short  Count1 = (unsigned short)Draw.Index;
            unsigned short  Count2 = (unsigned short)Draw.Index2;

            DrawIndexes[Index++] = Count2;
            DrawIndexes[Index++] = Count2 + 1;
            DrawIndexes[Index++] = Count1;

            DrawIndexes[Index++] = Count2 + 1;
            DrawIndexes[Index++] = Count1;
            DrawIndexes[Index++] = Count1 + 1;

            // number of indexed vertices
            Indexed2D += 6;
            // next item
            Start = Draw.Next;
            // repeat
            continue;
        }


        // * LIST INDEXED *
        for (DWORD a = 0; a < Draw.NrVertices; a++) DrawIndexes[Index++] = (unsigned short)(Draw.Index + VOffsets[a]);

        // number of indexed vertices
        Indexed2D += Draw.NrVertices;
        // next item
        Start = Draw.Next;

    }

    return Start;
}


void CDXEngine::DX2D_AssignLayers(void)
{
    // get the entry of AUTO Layer
    DWORD Start = Layers[LAYER_AUTO].Start, Next;
    // get the Startus Layers
#ifndef DEBUG_ENGINE
    float Stratus1Z = realWeather->stratusZ, Stratus2Z = realWeather->stratus2Z;
#else
    float Stratus1Z = 0.0f, Stratus2Z = 0.0f;
#endif
    DWORD Layer;

    // For each item in the AUTO list
    while (Start not_eq 0xFFFFFFFF)
    {
        // Default to GROUND LAYER
        Layer = LAYER_GROUND;
        // get the Item and it's Z
        DrawItemType &Draw = Draws2D[Start];

        if (Draw.Height <= Stratus2Z) Layer = LAYER_ROOF;
        else if (Draw.Height <= Stratus1Z) Layer = LAYER_MIDDLE;

        // ok, assign the item at the end of the layer
        if (Layers[Layer].Start == -1) Layers[Layer].Start = Start;
        else Draws2D[Layers[Layer].End].Next = Start;

        // assign links
        Layers[Layer].End = Start;
        Next = Draws2D[Start].Next;
        Draws2D[Start].Next = 0xffffffff;
        // ok, next item
        Start = Next;
    }
}



DWORD CDXEngine::DX2D_SortIndexes(DWORD Start)
{
    DWORD Idx, Row, Next;

    // UnInitialize sort Buckets
    memset(SortBuckets, 0xff, sizeof(SortBuckets));
    memset(SortTail, 0xff, sizeof(SortTail));

    // * SORT LOWER DIGIT DIRECTLY FROM THE DRAW LIST INTO BUCKETS *
    while (Start not_eq 0xffffffff)
    {
        // Take next item
        Next = Draws2D[Start].Next;
        // No link
        Draws2D[Start].Next = 0xffffffff;
        // Get the lower Digit
        Idx = *(unsigned char*)&Draws2D[Start].Dist256;

        // If bucket already assigned, link to old one
        if (SortBuckets[0][Idx] not_eq 0xffffffff) Draws2D[Start].Next = SortBuckets[0][Idx];

        // Assign this to the bucked
        SortBuckets[0][Idx] = Start;
        // Next Item
        Start = Next;
    }

    // * SORT 2ND DIGIT INTO 2ND BUCKETS ROW *
    // Start from top
    Row = 0xff;

    // Till a valid Bucket pointed
    while (Row not_eq 0xffffffff)
    {
        // if a valid bucket
        if (SortBuckets[0][Row] not_eq 0xffffffff)
        {
            // get the Bucket
            Start = SortBuckets[0][Row];

            do
            {
                // Take next item
                Next = Draws2D[Start].Next;
                // Break any link
                Draws2D[Start].Next = 0xffffffff;
                // Get the Digit
                Idx = *(((unsigned char*)&Draws2D[Start].Dist256) + 1);

                // If bucket not already assigned assign
                if (SortBuckets[1][Idx] == 0xffffffff) SortBuckets[1][Idx] = Start;
                // else tail it
                else Draws2D[SortTail[1][Idx]].Next = Start;

                // This is always however the last item
                SortTail[1][Idx] = Start;
                // Next Item
                Start = Next;
            }
            while (Start not_eq 0xffffffff); // Repeat till end of list
        }

        // Next Row
        Row--;
    }

    // * SORT 3RD DIGIT INTO 3RD BUCKETS ROW *
    // Start from top
    Row = 0xff;

    // Till a valid Bucket pointed
    while (Row not_eq 0xffffffff)
    {
        // if a valid bucket
        if (SortBuckets[1][Row] not_eq 0xffffffff)
        {
            // get the Bucket
            Start = SortBuckets[1][Row];

            do
            {
                // Take next item
                Next = Draws2D[Start].Next;
                // Break any link
                Draws2D[Start].Next = 0xffffffff;
                // Get the Digit
                Idx = *(((unsigned char*)&Draws2D[Start].Dist256) + 2);

                // If bucket not already assigned assign
                if (SortBuckets[2][Idx] == 0xffffffff) SortBuckets[2][Idx] = Start;
                // else tail it
                else Draws2D[SortTail[2][Idx]].Next = Start;

                // This is always however the last item
                SortTail[2][Idx] = Start;
                // Next Item
                Start = Next;
            }
            while (Start not_eq 0xffffffff); // Repeat till end of list
        }

        // Next Row
        Row--;
    }

    // * SORT 4TH DIGIT INTO 4TH BUCKETS ROW *
    // Start from top
    Row = 0xff;

    // Till a valid Bucket pointed
    while (Row not_eq 0xffffffff)
    {
        // if a valid bucket
        if (SortBuckets[2][Row] not_eq 0xffffffff)
        {
            // get the Bucket
            Start = SortBuckets[2][Row];

            do
            {
                // Take next item
                Next = Draws2D[Start].Next;
                // Break any link
                Draws2D[Start].Next = 0xffffffff;
                // Get the Digit
                Idx = *(((unsigned char*)&Draws2D[Start].Dist256) + 3);

                // If bucket not already assigned assign
                if (SortBuckets[3][Idx] == 0xffffffff) SortBuckets[3][Idx] = Start;
                // else tail it
                else Draws2D[SortTail[3][Idx]].Next = Start;

                // This is always however the last item
                SortTail[3][Idx] = Start;
                // Next Item
                Start = Next;
            }
            while (Start not_eq 0xffffffff); // Repeat till end of list
        }

        // Next Row
        Row--;
    }


    // ok, now link all buckets
    // Start from bottom
    Row = 0x00;
    // Unassign any link
    Next = 0xffffffff;

    while (Row < 0x100)
    {
        // get an item to link
        if (SortBuckets[3][Row] not_eq 0xffffffff)
        {
            // Link to previous one
            Draws2D[SortTail[3][Row]].Next = Next;
            // and get this as next to be linked
            Start = Next = SortBuckets[3][Row];
        }

        Row++;
    }

    // Return the Entry point of the List
    return Start;

}



// Sorting and Flushing all the 2D objects
void CDXEngine::DX2D_SetViewMode(void)
{
    if (g_bUseD3D11)
    {
        // #27 D3D11: a single 2D-in-3D state (alpha-blend, no Z-write, no lighting, alpha-test
        // chroma) is set by BeginDynamic2D. Texture/indices in DX2D_Flush2DObjects.
        if (g_pD3D11Renderer) g_pD3D11Renderer->BeginDynamic2D();
        return;
    }
    // Artscout - 2026: #34 dead D3D7 tail removed (D3D11 branch above returns).
}

// Sorting and Flushing all the 2D objects
void CDXEngine::DX2D_Flush2DObjects(void)
{
    DWORD_PTR LastTexHandle = -1; // Artscout - 2026 (x64): pointer-sized

    // Track of the drawing mode
    bool Mode_2D = false, Mode_3D = false;

    // if no 2D objects to Draw, exit here
    if ( not Total2DItems) return;

    // Set the View Mode for the 2D stuff
    DX2D_SetViewMode();

    // #27 D3D11: vertices accumulated by Add* in the CPU VbPtr -- convert+upload ONCE
    // (MAX_2D_BUFFERS=1 -> VBSelected=0). Then DrawDynamic2DIndexed per Draws2D item.
    if (g_bUseD3D11 and g_pD3D11Renderer)
        g_pD3D11Renderer->UploadDynamic2D(Dyn2DVertexBuffer[VBSelected].VbPtr,
                                          (int)Dyn2DVertexBuffer[VBSelected].LastIndex);

#ifdef DEBUG_2D_ENGINE
    DWORD Vertices = 0;
#endif
    // The Layer Counter
    DWORD Layer;
    DWORD l = 0, DrawStart, NextDraw;

#ifdef DEBUG_2D_ENGINE
    START_PROFILE("DYN SORT:");
#endif
    // Assign Layers to AUTO Items
    DX2D_AssignLayers();
#ifdef DEBUG_2D_ENGINE
    STOP_PROFILE("DYN SORT:");
#endif

    // Ok, draw Layers in Order
    do
    {
        Layer = DrawOrder[l++];

        // Artscout - 2026 (x64): skip invalid layers BEFORE indexing Layers[]. DX2D_Reset leaves
        // DrawOrder filled with LAYER_NODRAW (0xFFFFFFFF); if a flush runs with items before any
        // DX2D_SetDrawOrder (e.g. the menu Munitions 3D viewer, no world render), Layer==0xFFFFFFFF.
        // The old code indexed Layers[0xFFFFFFFF] at line below and only checked NODRAW afterwards:
        // on x86 the index wrapped mod 2^32 into mapped static memory (harmless); on x64 it faults.
        if (Layer >= MAX_2D_LAYERS) continue; // covers LAYER_NODRAW and any garbage index

        // get the Layer Entry point in the list
        DrawStart = Layers[Layer].Start;
#ifdef DEBUG_2D_ENGINE
        START_PROFILE("DYN SORT:");
#endif

        if (Layer == LAYER_NODRAW) continue;

        // check if Layer need to be sorted and eventually sort it
        if (1 or Layers[Layer].Flags bitand LAYER_SORT) DrawStart = DX2D_SortIndexes(DrawStart);

#ifdef DEBUG_2D_ENGINE
        STOP_PROFILE("DYN SORT:");
#endif

        // TOP LAYER makes no Z Checks

        // ok, flush all the Draws till end of Layer
        while (DrawStart not_eq 0xffffffff)
        {
#ifdef DRAW_USING_2D_FANS
            NextDraw = Draws2D[DrawStart].Next;
#else
#ifdef DEBUG_2D_ENGINE
            START_PROFILE("DYN SORT:");
#endif
            // generate Draw Indexes
            NextDraw = DX2D_GenerateIndexes(DrawStart);
#ifdef DEBUG_2D_ENGINE
            STOP_PROFILE("DYN SORT:");
#endif
#endif
            // Assign the Draw Item
            DrawItemType &Draw = Draws2D[DrawStart];

            /////////////////// DRAWING A 3D ALPHA OBJECT HERE ////////////////////////////////
            // Check if a solid 3D object
            if (Draw.Flags bitand POLY_3DOBJECT)
            {
                // Draw the sorted object setting u the right mode if not already in 3D mode
                DrawSortedAlpha(Draw.Flags bitand (0xffffff), not Mode_3D);
                // Mark that we are in 3D mode
                Mode_2D = false;
                Mode_3D = true;
            }
            else
            {
                ////////////////// DRAWING A 2D OBJECT HERE ////////////////////////////////

                // if not already in 2D mode, set the 2D drawing parameters
                if ( not Mode_2D)
                {
                    DX2D_SetViewMode();
                    LastTexHandle = -1;
                }

                // eventually assign texture
                if (LastTexHandle not_eq Draw.TexHandle)
                {
                    // #34 dead D3D7 SetTexture removed (D3D11 sets the texture in DrawDynamic2DIndexed)
                    LastTexHandle = Draw.TexHandle;
                }

                // Chweck if a set of lines
                // #34 dead D3D7 else-branches removed (D3D11 indexed draw only)
                {
                    // #27: indexed draw over the uploaded UploadDynamic2D buffer.
                    void *srv = Draw.TexHandle ? (void*)((TextureHandle *)Draw.TexHandle)->m_pDDS : NULL;
                    int prim = (Draw.Flags bitand POLY_LINE) ? D3DPT_LINELIST : D3DPT_TRIANGLELIST;
                    if (g_pD3D11Renderer)
                        g_pD3D11Renderer->DrawDynamic2DIndexed((unsigned short*)&DrawIndexes, (int)Indexed2D,
                                                               (struct ID3D11ShaderResourceView*)srv, prim);
                }

                // Mark that we are in 2D mode
                Mode_3D = false;
                Mode_2D = true;

            }

            ///////////////////////////////////////////////////////////////////////////////////////////////

            // ok, go to next draw
            DrawStart = NextDraw;
#ifdef DEBUG_2D_ENGINE
            COUNT_PROFILE("2D DRAWS");
#endif
        }

    }
    while (Layer not_eq LAYER_TOP and l <= LAYER_TOP); // END with TOP LAYER in any case


    // buffer is flushed
    Total2DItems = 0;
#ifdef DEBUG_2D_ENGINE
    REPORT_VALUE("2D VERTICES :", Dyn2DVertexBuffer[0].LastIndex);
    // Debug_Vertices2D=0;
#endif
    DX2D_InitLists();

}


void CDXEngine::DX2D_SetDrawOrder(DWORD *Order)
{
    memcpy(DrawOrder, Order, sizeof(DrawOrder));
}

void CDXEngine::DX2D_TransformBB(XMMVector *Pos, XMMVector *Coord, D3DDYNVERTEX *Dest, DWORD Nr)
{
    _MM_ALIGN16 XMMVector XMMStore;
    _MM_ALIGN16 __m128 C0 = BBCx[0].Xmm, C1 = BBCx[1].Xmm, C2 = BBCx[2].Xmm;

    while (Nr--)
    {
        // Execute the BB by 1st Cx
        XMMStore.Xmm = _mm_mul_ps(C0, Coord->Xmm);
        Dest->pos.x = XMMStore.d3d.x + XMMStore.d3d.y + XMMStore.d3d.z + Pos->d3d.x;
        // Execute the BB by 2nd Cx
        XMMStore.Xmm = _mm_mul_ps(C1, Coord->Xmm);
        Dest->pos.y = XMMStore.d3d.x + XMMStore.d3d.y + XMMStore.d3d.z + Pos->d3d.y;
        // Execute the BB by 3rd Cx
        XMMStore.Xmm = _mm_mul_ps(C2, Coord->Xmm);
        Dest->pos.z = XMMStore.d3d.x + XMMStore.d3d.y + XMMStore.d3d.z + Pos->d3d.z;
        // Next vertex
        Coord++;
        Dest++;
    }
}



void CDXEngine::DX2D_TransformBB(XMMVector *Pos, D3DDYNVERTEX *Vertex, DWORD Nr)
{
    _MM_ALIGN16 XMMVector XMMStore, XMMStore1, XMMStore2, Coord;
    _MM_ALIGN16 __m128 C0 = BBCx[0].Xmm, C1 = BBCx[1].Xmm, C2 = BBCx[2].Xmm;

    while (Nr--)
    {
        // Get the quad coords
        Coord.Xmm = _mm_loadu_ps((float*)&Vertex->pos);
        // Execute the BB by 1st Cx
        XMMStore.Xmm = _mm_mul_ps(C0, Coord.Xmm);
        Vertex->pos.x = XMMStore.d3d.x + XMMStore.d3d.y + XMMStore.d3d.z + Pos->d3d.x;
        // Execute the BB by 2nd Cx
        XMMStore1.Xmm = _mm_mul_ps(C1, Coord.Xmm);
        Vertex->pos.y = XMMStore1.d3d.x + XMMStore1.d3d.y + XMMStore1.d3d.z + Pos->d3d.y;
        // Execute the BB by 3rd Cx
        XMMStore2.Xmm = _mm_mul_ps(C2, Coord.Xmm);
        Vertex->pos.z = XMMStore2.d3d.x + XMMStore2.d3d.y + XMMStore2.d3d.z + Pos->d3d.z;
        // next quad
        Vertex++;
    }
}


// 2D Engine initializations
// This function creates all buffers needed by the rendering engine for 2D/Billboarded stuffs
void CDXEngine::DX2D_Init(void)
{
    LastPassed = NULL;

    // Creates the Vertex Buffer Descriptor
    D3DVERTEXBUFFERDESC VBDesc;
    VBDesc.dwSize = sizeof(D3DVERTEXBUFFERDESC);
    VBDesc.dwCaps = D3DVBCAPS_WRITEONLY bitor D3DVBCAPS_DONOTCLIP;
    VBDesc.dwFVF = D3DFVF_DYNAMIC;
    VBDesc.dwNumVertices = MAX_2D_VERTICES - 1;

    // #34 D3D11: instead of a locked D3D7 VB -- a persistent CPU buffer. Add* accumulate into it
    // (SIMD stores, as before), the flush converts to the object vertex and draws via
    // DrawDynamic2D. VbPtr is kept alive the whole session (DX2D_Reset only zeroes LastIndex).
    // Vb=NULL (unused in D3D11). Dead D3D7 D3D->CreateVertexBuffer branch removed.
    for (int i = 0; i < MAX_2D_BUFFERS; i++)
    {
        Dyn2DVertexBuffer[i].Vb = NULL;
        // +16 vertices of padding: the D3D7 VB rounded the size, malloc does not; guards a small
        // write overrun of billboard quads at the very end of the buffer (else heap corruption).
        Dyn2DVertexBuffer[i].VbPtr = (D3DDYNVERTEX*)malloc(((size_t)MAX_2D_VERTICES + 16) * sizeof(D3DDYNVERTEX));
    }

    DX2D_Reset();
}



// 2DEngine release
void CDXEngine::DX2D_Release(void)
{
    // DO NOT RELEASE DOR NOW, need to find right descturctor for this
    //ReleaseTextures();
    for (int i = 0; i < MAX_2D_BUFFERS; i++)
    {
        if (Dyn2DVertexBuffer[i].Vb)	// PHASE 1 (D3D7->D3D11): 2D VBs were not created under D3D11 -> NULL
            Dyn2DVertexBuffer[i].Vb->Release();
    }
}


// this function returns a detail level as a float
// used to scale some 2D/3D items like lines...
float CDXEngine::GetDetailLevel(D3DVECTOR *WorldPos, float MaxRange)
{
    _MM_ALIGN16 XMMVector CPos;
    // make it in camera space
    // Artscout - 2026 (x64): SSE asm rewritten with intrinsics (x86+x64; rest of file uses .Xmm).
    // Load x,y,z safely (no 16-byte OOB read past the 12-byte D3DVECTOR).
    __m128 wp = _mm_set_ps(0.0f, WorldPos->z, WorldPos->y, WorldPos->x);
    CPos.Xmm = _mm_sub_ps(wp, XMMCamera.Xmm);   // subtract camera
    CPos.Xmm = _mm_mul_ps(CPos.Xmm, CPos.Xmm);  // square all components

    return  sqrtf(CPos.d3d.x + CPos.d3d.y + CPos.d3d.z) / MaxRange * m_LODBiasCx;
}





// The 3D Point draw function
void CDXEngine::Draw3DPoint(D3DVECTOR *WorldPos, DWORD Color, bool Emissive, bool CameraSpace)
{
    if (g_bUseD3D11)
    {
        // #27 D3D11: store FULL world-space (view*proj handles it), without camera subtraction.
        SimpleBufferType &sb = TheVbManager.SimpleBuffer;
        if (sb.VbPtr and sb.Points < MAX_POINTS)
        {
            D3DSIMPLEVERTEX *VPtr = &sb.VbPtr[POINTS_OFFSET + sb.Points];
            VPtr->pos = *WorldPos;
            VPtr->dwColour = Color;
            VPtr->dwSpecular = Emissive ? Color : 0;
            VPtr->tu = VPtr->tv = 0.0f;
            sb.Points++;
            if (sb.MaxPoints < MAX_POINTS) sb.MaxPoints++;
        }
        return;
    }
    // Artscout - 2026: #34 dead D3D7 tail removed (D3D11 branch above returns).
}




// The 3D Line draw function
void CDXEngine::Draw3DLine(D3DVECTOR *WorldStart, D3DVECTOR *WorldEnd, DWORD ColorStart, DWORD ColorEnd, bool Emissive, bool CameraSpace)
{
    if (g_bUseD3D11)
    {
        // #27 D3D11: tracers. FULL world-space for both ends, without camera subtraction.
        SimpleBufferType &sb = TheVbManager.SimpleBuffer;
        if (sb.VbPtr and sb.Lines < MAX_LINES)
        {
            D3DSIMPLEVERTEX *VPtr = &sb.VbPtr[LINES_OFFSET + sb.Lines * 2];
            VPtr[0].pos = *WorldStart; VPtr[0].dwColour = ColorStart;
            VPtr[0].dwSpecular = Emissive ? ColorStart : 0; VPtr[0].tu = VPtr[0].tv = 0.0f;
            VPtr[1].pos = *WorldEnd;   VPtr[1].dwColour = ColorEnd;
            VPtr[1].dwSpecular = Emissive ? ColorEnd : 0;   VPtr[1].tu = VPtr[1].tv = 0.0f;
            sb.Lines++;
            if (sb.MaxLines < MAX_LINES) sb.MaxLines++;
        }
        return;
    }
    // Artscout - 2026: #34 dead D3D7 tail removed (D3D11 branch above returns).
}













// This function setups the Square sides CX, even not normalized, for following Billboarded DrawBaseItems to be returned
void CDXEngine::DX2D_SetupSquareCx(float y, float z)
{
    // The X is always 0 ( 2D square )
    vbb0.d3d.x = vbb1.d3d.x = vbb2.d3d.x = vbb3.d3d.x = 0;
    vbb0.d3d.y = vbb3.d3d.y = -y;
    vbb1.d3d.y = vbb2.d3d.y = y;
    vbb0.d3d.z = vbb1.d3d.z = -z;
    vbb2.d3d.z = vbb3.d3d.z = z;

    // the 4 vertices of the square with BillBoard CXs
    D3DXVec3TransformCoord((D3DXVECTOR3*)&BBvbb0.d3d, (D3DXVECTOR3*)&vbb0.d3d, &BBMatrix);
    D3DXVec3TransformCoord((D3DXVECTOR3*)&BBvbb1.d3d, (D3DXVECTOR3*)&vbb1.d3d, &BBMatrix);
    D3DXVec3TransformCoord((D3DXVECTOR3*)&BBvbb2.d3d, (D3DXVECTOR3*)&vbb2.d3d, &BBMatrix);
    D3DXVec3TransformCoord((D3DXVECTOR3*)&BBvbb3.d3d, (D3DXVECTOR3*)&vbb3.d3d, &BBMatrix);
}






void CDXEngine::FlushDynamicObjects(void)
{
    if (g_bUseD3D11)
    {
        // #27 D3D11: first the simple items (points/line tracers from Draw3DPoint/Line),
        // then 2D particles. All via the object pipeline (BeginDynamic2D), no texture.
        SimpleBufferType &sb = TheVbManager.SimpleBuffer;
        if (g_pD3D11Renderer and sb.VbPtr and (sb.MaxPoints or sb.MaxLines))
        {
            g_pD3D11Renderer->BeginDynamic2D(true);   // #31 tracers/sparks -- additive glow
            if (sb.MaxPoints)
                g_pD3D11Renderer->DrawDynamic2D(&sb.VbPtr[POINTS_OFFSET], (int)sb.MaxPoints, NULL, D3DPT_POINTLIST);
            if (sb.MaxLines)
                g_pD3D11Renderer->DrawDynamic2D(&sb.VbPtr[LINES_OFFSET], (int)sb.MaxLines * 2, NULL, D3DPT_LINELIST);
        }
        sb.Points = sb.Lines = sb.MaxPoints = sb.MaxLines = 0;   // reset for the next frame

        DX2D_Flush2DObjects();
        return;
    }
    // Artscout - 2026: #34 dead D3D7 tail removed (D3D11 branch above returns).
}





/////////////////////////////////////////// RADAR STUFF ////////////////////////////////////////////////////

// This function is used for just drawin Radar stuff
// in this function m_FogLevel is the blit intensity

void CDXEngine::DrawBlip(ObjectInstance *objInst, D3DXMATRIX *RotMatrix, const Ppoint *Pos, const float sx, const float sy, const float sz, const float scale, bool CameraSpace)
{
    D3DXMATRIX Scale, State;
    D3DVECTOR p;
    DxDbHeader *Model;

    // The object position is always calculated relative to the camera position
    // if coming from out world, if IN CAMERA SPACE, position is already relative to camera,
    // and even visibility is skipped
    if (CameraSpace)
    {
        p.x = Pos->x;
        p.y = Pos->y;
        p.z = Pos->z;
        State = *RotMatrix;
    }
    else
    {
        p.x = -CameraPos.x + Pos->x;
        p.y = -CameraPos.y + Pos->y;
        p.z = -CameraPos.z + Pos->z;
    }


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

    // ************ ADD Other Features ***********
    D3DXMatrixIdentity(&Scale);
    Scale.m00 = scale * sx;
    Scale.m11 = scale * sy;
    Scale.m22 = scale * sz;
    D3DXMatrixMultiply(&State, RotMatrix, &Scale);
    // *******************************************

    // *********** Base transformations **********
    D3DXMatrixTranslation(&Scale, p.x, p.y, p.z);
    D3DXMatrixMultiply(&State, &State, &Scale);
    // *******************************************

    TheVbManager.AddDrawRequest(objInst, Model->Id, &State, false, 0, m_BlipIntensity);
}



void CDXEngine::FlushBlips(void)
{
    // #27 D3D11: radar blips (DX_DBS) via the object pipeline, green material.
    // #34: the dead D3D7 (m_pD3DD) paths have been removed from this file.
    ObjectInstance *objInst = NULL;
    DWORD LodID;
    bool Lited;
    DWORD LightOwner;
    DWORD DofLevel = 0;

    D3DMATERIAL7 RadarMaterial;


    // not a previous object instalce
    m_LastObjectInstance = NULL;

    RadarMaterial.specular.r = RadarMaterial.specular.g = RadarMaterial.specular.b = 0.0f;
    RadarMaterial.emissive.r = RadarMaterial.emissive.b = 0.0f;
    RadarMaterial.emissive.g = 0.5f;
    RadarMaterial.diffuse.r = RadarMaterial.diffuse.b = 0.0f;
    RadarMaterial.diffuse.g = 0.0f;
    RadarMaterial.ambient.r = RadarMaterial.ambient.g = RadarMaterial.ambient.b = 0.0f;
    RadarMaterial.dvPower = 0.0f;

    // Radar: alpha-blend ON, no texture; color comes from the material (green, set per blip below).
    // Artscout - 2026: #34 dead D3D7 else-branch removed.
    if (g_pD3D11Renderer)
    {
        g_pD3D11Renderer->SetObjectAlphaBlend(true);
        g_pD3D11Renderer->SetTexture(0, NULL);
    }



    ///////////////////////////// HERE STARTS THE DRAWING ENGINE LOOP //////////////////////////////
    // The Loop flushes all objects from the VBuffers

    // Till objects to Draw
    while (TheVbManager.GetDrawItem(&objInst, &LodID, &AppliedState, &Lited, &LightOwner, &m_BlipIntensity))
    {

        // Consistency Check
        if ( not objInst) continue;

        // assign for engine use
        m_TheObjectInstance = objInst;

        // gets the pointer to the Model Vertex Buffer
        TheVbManager.GetModelData(m_VB, LodID);

        // Consistency Check
        if ( not m_VB.Valid) continue;

        // Ok... transform the object
        DofLevel = 0;
        RadarMaterial.diffuse.a = m_BlipIntensity / 255.0f;

        if (g_pD3D11Renderer)   // #34 dead D3D7 else-branch removed
        {
            g_pD3D11Renderer->SetWorld((const float*)&AppliedState);
            // green blip, alpha=intensity (equiv. D3D7 emissive.g + diffuse.a).
            g_pD3D11Renderer->SetMaterialColor(0.15f, 1.0f, 0.15f, m_BlipIntensity / 255.0f);
        }


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



            // Selects actions for each node
            switch (m_NODE.HEAD->Type)
            {


                case DX_SWITCH:
                    case DX_LIGHT:
                        case DX_TEXTURE:
                            case DX_MATERIAL:
                                case DX_SLOT:
                                    case DX_ROOT:
                                            break;

                case DX_SURFACE: // Setup the Texture setup the Texture to be used
                        if ( not DofLevel) DrawBlitNode();

                    break;

                case DX_DOF:
                        DofLevel++;
                    break;

                case DX_ENDDOF:
                        DofLevel--;
                    break;


                default :
                        char s[128];
                    printf(s, "Corrupted Model ID : %d ", LodID);
                    MessageBox(NULL, s, "DX Engine", NULL);
            }


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

    if (g_pD3D11Renderer)   // #34 dead D3D7 else-branch removed
    {
        g_pD3D11Renderer->SetMaterialColor(1.0f, 1.0f, 1.0f, 1.0f);   // reset material
        g_pD3D11Renderer->SetObjectAlphaBlend(false);
    }

}


void CDXEngine::DrawBlitNode(void)
{
    if (g_bUseD3D11)
    {
        // #27 D3D11: draw the radar-blip surface from the D3D11 mirror VB (like DrawSurface).
        if (g_pD3D11Renderer and m_VB.VbD3D11)
        {
            void *idxPtr = m_NODE.BYTE + sizeof(DxSurfaceType);
            if (m_NODE.SURFACE->dwPrimType == D3DPT_POINTLIST)
                g_pD3D11Renderer->DrawObjectStrip(m_NODE.SURFACE->dwPrimType, m_VB.VbD3D11, VERTEX_STRIDE,
                                                  (int)((DWORD) * ((Int16*)idxPtr)),
                                                  (int)m_NODE.SURFACE->dwVCount);
            else
                g_pD3D11Renderer->DrawObjectIndexed(m_NODE.SURFACE->dwPrimType, m_VB.VbD3D11, VERTEX_STRIDE,
                                                    0, (unsigned short*)idxPtr,
                                                    (int)m_NODE.SURFACE->dwVCount);
        }
        return;
    }
    // Artscout - 2026: #34 dead D3D7 tail removed (D3D11 branch above returns).
}
