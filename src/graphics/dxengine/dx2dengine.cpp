#include <math.h>
#include "../include/ObjectInstance.h"
#include "dxdefines.h"
#include "DXVBManager.h"
#include "mmsystem.h"
#include "../include/TexBank.h"
#include "dxengine.h"
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



DWORD CDXEngine::GetTextureHandle(char *TexName)
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
        Dyn2DVertexBuffer[i].Vb->Lock(DDLOCK_DISCARDCONTENTS bitor DDLOCK_NOSYSLOCK bitor DDLOCK_WAIT bitor DDLOCK_WRITEONLY, (void**)&Dyn2DVertexBuffer[i].VbPtr, NULL);;
    }
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
    m_pD3DD->ComputeSphereVisibility((D3DVECTOR*)&XMMPos.d3d, &Radius2D, 1, 0, &ClipResult);

    if (ClipResult bitand D3DSTATUS_DEFAULT) return false;

    return true;
}



DWORD CDXEngine::ComputeSphereVisibility(LPD3DVECTOR lpCenters, LPD3DVALUE  lpRadii, DWORD dwNumSpheres)
{
    DWORD ClipResult;
    XMMAcc = _mm_loadu_ps((float*)lpCenters);
    XMMPos.Xmm = _mm_sub_ps(XMMAcc, XMMCamera.Xmm);
    // Check for object visibility, return NULL is not visible
    m_pD3DD->ComputeSphereVisibility((D3DVECTOR*)&XMMPos.d3d, lpRadii, dwNumSpheres, 0, &ClipResult);

    return ClipResult;

}


float CDXEngine::DX2D_GetDistance(D3DXVECTOR3 *Pos, float Radius, DWORD Flags)
{
    DWORD ClipResult;
    //Store the radius
    Radius2D = Radius;
    // get the position and make it camera relative
    XMMPos.Xmm = _mm_loadu_ps((float*)Pos);

    if ( not (Flags bitand CAMERA_VERTICES))XMMPos.Xmm = _mm_sub_ps(XMMPos.Xmm, XMMCamera.Xmm);

    // Check for object visibility, return NULL is not visible
    m_pD3DD->ComputeSphereVisibility((D3DVECTOR*)&XMMPos.d3d, &Radius2D, 1, 0, &ClipResult);

    if (ClipResult bitand D3DSTATUS_DEFAULT) return -1.0f;

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
    // check for buffer limit
    if (VbIndex + Size >= MAX_2D_VERTICES)
    {
        // Start with a new Buffer from index 0
        VBSelected++;
        VbIndex = 0;
    }

    // if no more buffers, exit here
    if (VBSelected >= MAX_2D_BUFFERS) return false;

    return true;
}


// This function add a Quad to the vertex buffers and sorting list...
// WARNING  Does not check for Visibility, call DX2D_GetVisibility() or DX2D_SetupQuad before...
void CDXEngine::DX2D_AddQuad(DWORD Layer, DWORD Flags, D3DXVECTOR3 *Pos, D3DDYNVERTEX *Quad, float Radius, DWORD TexHandle)
{
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
void CDXEngine::DX2D_AddTri(DWORD Layer, DWORD Flags, D3DXVECTOR3 *Pos, D3DDYNVERTEX *Tri, float Radius, DWORD TexHandle)
{
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
void CDXEngine::DX2D_AddBi(DWORD Layer, DWORD Flags, D3DXVECTOR3 *Pos, D3DDYNVERTEX *Segment, float Radius, DWORD TexHandle)
{
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
void CDXEngine::DX2D_AddSingle(DWORD Layer, DWORD Flags, D3DXVECTOR3 *Pos, D3DDYNVERTEX *Segment, float Radius, DWORD TexHandle)
{
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



void CDXEngine::DX2D_AddPoly(DWORD Layer, DWORD Flags, D3DXVECTOR3 *Pos, D3DDYNVERTEX *Poly, float Radius, DWORD Vertices, DWORD TexHandle)
{
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
    DWORD Tex = Draws2D[Start].TexHandle;

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
    DXFlagsType Flags;
    // Fog at Max range
    m_FogLevel = m_LinearFogLevel;

    D3DXMATRIX unit;
    D3DXMatrixIdentity(&unit);
    m_pD3DD->SetTransform(D3DTRANSFORMSTATE_WORLD, (LPD3DMATRIX)&unit);

    // Reset any previous state flag
    Flags.w = 0xffff;
    SetRenderState(Flags, Flags, DISABLE);
    // adjust the flags for the surface
    Flags.w = 0;
    Flags.b.Texture = Flags.b.Alpha = 1;
    // set the engine status
    SetRenderState(Flags, Flags, ENABLE);
    // No Culling
    m_pD3DD->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    // set the engine status
    m_pD3DD->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, FALSE);

    // Disable any texture stage Alpha and Color
    m_pD3DD->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
    m_pD3DD->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    m_pD3DD->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    m_pD3DD->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    m_pD3DD->SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
    m_pD3DD->SetTextureStageState(2, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    m_pD3DD->SetTextureStageState(3, D3DTSS_COLOROP, D3DTOP_DISABLE);
    m_pD3DD->SetTextureStageState(3, D3DTSS_ALPHAOP, D3DTOP_DISABLE);


    switch (m_RenderState)
    {

        case DX_TV:

            // FRB - B&W
            if ((g_bGreyMFD) and ( not bNVGmode))
                m_pD3DD->SetRenderState(D3DRENDERSTATE_TEXTUREFACTOR, 0x00a0a0a0);
            else
                m_pD3DD->SetRenderState(D3DRENDERSTATE_TEXTUREFACTOR, 0x0000a000 /*NVG_T_FACTOR*/);

            m_pD3DD->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            m_pD3DD->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
            m_pD3DD->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);

            m_pD3DD->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_TFACTOR);
            m_pD3DD->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CURRENT);
            m_pD3DD->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
            break;

        case DX_NVG:
            m_pD3DD->SetRenderState(D3DRENDERSTATE_TEXTUREFACTOR, 0x0000a000 /*NVG_T_FACTOR*/);

            m_pD3DD->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            m_pD3DD->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
            m_pD3DD->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);

            m_pD3DD->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_TFACTOR);
            m_pD3DD->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CURRENT);
            m_pD3DD->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
            break;

        case DX_OTW:
        default :
            m_pD3DD->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            m_pD3DD->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
            m_pD3DD->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
            break;

    }

    // More setting for 2D drawing
    m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, TRUE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHAREF, (DWORD)1);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, D3DCMP_GREATEREQUAL);

    m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_pD3DD->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    m_pD3DD->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    m_pD3DD->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

    // No HW Light for 2D objects
    m_pD3DD->SetRenderState(D3DRENDERSTATE_LIGHTING, FALSE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR2);

    m_FogLevel = m_LinearFogLevel;
    m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGEND,   *(DWORD *)(&m_FogLevel));
}

// Sorting and Flushing all the 2D objects
void CDXEngine::DX2D_Flush2DObjects(void)
{
    DWORD LastTexHandle = -1;

    // Track of the drawing mode
    bool Mode_2D = false, Mode_3D = false;

    // if no 2D objects to Draw, exit here
    if ( not Total2DItems) return;

    // Set the View Mode for the 2D stuff
    DX2D_SetViewMode();

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
        if (Layer == LAYER_TOP) m_pD3DD->SetRenderState(D3DRENDERSTATE_ZENABLE, FALSE);

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
                    if (Draw.TexHandle) m_pD3DD->SetTexture(0, ((TextureHandle *)Draw.TexHandle)->m_pDDS);
                    else m_pD3DD->SetTexture(0, NULL);

                    LastTexHandle = Draw.TexHandle;
                }

                // Chweck if a set of lines
                if (Draw.Flags bitand POLY_LINE)
                {
                    m_pD3DD->DrawIndexedPrimitiveVB(D3DPT_LINELIST, Draw.Vb, 0, MAX_2D_VERTICES,
                                                    (LPWORD)&DrawIndexes, Indexed2D, 0);
                }
                else
                {
                    //make the Draw
#ifdef DRAW_USING_2D_FANS
                    m_pD3DD->DrawPrimitiveVB(D3DPT_TRIANGLEFAN, Draw.Vb, Draw.Index, Draw.NrVertices, 0);
#else
                    m_pD3DD->DrawIndexedPrimitiveVB(D3DPT_TRIANGLELIST, Draw.Vb, 0, MAX_2D_VERTICES,
                                                    (LPWORD)&DrawIndexes, Indexed2D, 0);
#endif
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

        if (Layer == LAYER_TOP) m_pD3DD->SetRenderState(D3DRENDERSTATE_ZENABLE, TRUE);
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

    for (int i = 0; i < MAX_2D_BUFFERS; i++)
    {
        CheckHR(D3D->CreateVertexBuffer(&VBDesc, &Dyn2DVertexBuffer[i].Vb, NULL));
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
        Dyn2DVertexBuffer[i].Vb->Release();
    }
}


// this function returns a detail level as a float
// used to scale some 2D/3D items like lines...
float CDXEngine::GetDetailLevel(D3DVECTOR *WorldPos, float MaxRange)
{
    _MM_ALIGN16 XMMVector CPos;
    // make it in camera space
    _asm
    {
        mov edx, DWORD PTR WorldPos // Get th World position
        movups xmm0, XMMWORD PTR [edx] // into XMM0
        subps xmm0, XMMCamera // subtract it
        mulps xmm0, xmm0; // square of all parameters
        movaps CPos, xmm0; // stores
    }

    return  sqrtf(CPos.d3d.x + CPos.d3d.y + CPos.d3d.z) / MaxRange * m_LODBiasCx;
}





// The 3D Point draw function
void CDXEngine::Draw3DPoint(D3DVECTOR *WorldPos, DWORD Color, bool Emissive, bool CameraSpace)
{

    // check for Buffer locked
    if ( not TheVbManager.SimpleBuffer.VbPtr) TheVbManager.OpenSimpleBuffer();

    // Get the appropriate index list in the buffer
    DWORD Index = POINTS_OFFSET + TheVbManager.SimpleBuffer.Points;
    D3DSIMPLEVERTEX *VPtr = &TheVbManager.SimpleBuffer.VbPtr[Index];

    if ( not CameraSpace)
    {
        // make it in camera space
        _asm
        {
            mov edx, DWORD PTR WorldPos // Get th World position
            movups xmm0, XMMWORD PTR [edx] // into XMM0
            mov eax, DWORD PTR VPtr // Get the Camera position
            subps xmm0, XMMCamera // subtract it
            movups XMMWORD PTR [eax], xmm0 // save in the vertex
        }
    }
    else
    {
        _asm
        {
            mov edx, DWORD PTR WorldPos // Get th World position
            movups xmm0, XMMWORD PTR [edx] // into XMM0
            mov eax, DWORD PTR VPtr // Get the Camera position
            movups XMMWORD PTR [eax], xmm0 // save in the vertex
        }
    }

    VPtr->dwColour = Color;

    if (Emissive) VPtr->dwSpecular = Color;
    else VPtr->dwSpecular = 0;

    // update pointers and Counters
    TheVbManager.SimpleBuffer.Points++;

    if (TheVbManager.SimpleBuffer.MaxPoints < MAX_POINTS) TheVbManager.SimpleBuffer.MaxPoints++;

    if (TheVbManager.SimpleBuffer.Points >= MAX_POINTS) TheVbManager.SimpleBuffer.Points = 0;
}




// The 3D Line draw function
void CDXEngine::Draw3DLine(D3DVECTOR *WorldStart, D3DVECTOR *WorldEnd, DWORD ColorStart, DWORD ColorEnd, bool Emissive, bool CameraSpace)
{

    // check for Buffer locked
    if ( not TheVbManager.SimpleBuffer.VbPtr) TheVbManager.OpenSimpleBuffer();

    // Get the appropriate index list in the buffer
    DWORD Index = LINES_OFFSET + TheVbManager.SimpleBuffer.Lines * 2;
    D3DSIMPLEVERTEX *VPtr = &TheVbManager.SimpleBuffer.VbPtr[Index];

    if ( not CameraSpace)
    {
        // make it in camera space
        _asm
        {
            mov edx, DWORD PTR WorldStart // Get the World Start position
            movups xmm0, XMMWORD PTR [edx] // into XMM0
            mov eax, DWORD PTR VPtr // Get the Camera position
            subps xmm0, XMMCamera // subtract it
            mov ecx, DWORD PTR WorldEnd // Get the World End position
            movups XMMWORD PTR [eax], xmm0 // save in the vertex
            movups xmm1, XMMWORD PTR [ecx] // into XMM0
            subps xmm1, XMMCamera // subtract it
            movups XMMWORD PTR [eax+SIZE D3DSIMPLEVERTEX], xmm1 // save in the 2nd vertex
        }
    }
    else
    {
        _asm
        {
            mov edx, DWORD PTR WorldStart // Get the World Start position
            movups xmm0, XMMWORD PTR [edx] // into XMM0
            mov eax, DWORD PTR VPtr // Get the Camera position
            mov ecx, DWORD PTR WorldEnd // Get the World End position
            movups XMMWORD PTR [eax], xmm0 // save in the vertex
            movups xmm1, XMMWORD PTR [ecx] // into XMM0
            movups XMMWORD PTR [eax+SIZE D3DSIMPLEVERTEX], xmm1 // save in the 2nd vertex
        }
    }

    VPtr->dwColour = ColorStart;
    (VPtr + 1)->dwColour = ColorEnd;

    if (Emissive)
    {
        VPtr->dwSpecular = ColorStart;
        (VPtr + 1)->dwSpecular = ColorEnd;
    }
    else
    {
        VPtr->dwSpecular = 0;
        (VPtr + 1)->dwSpecular = 0;
    }

    // update pointers and Counters
    TheVbManager.SimpleBuffer.Lines++;

    if (TheVbManager.SimpleBuffer.MaxLines < MAX_LINES) TheVbManager.SimpleBuffer.MaxLines++;

    if (TheVbManager.SimpleBuffer.Lines >= MAX_LINES) TheVbManager.SimpleBuffer.Lines = 0;
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
    // First of all save present renderer State
    DWORD StateHandle;

    // Finished Drawing, unlock all Vertex Buffers
    for (int i = 0; i < MAX_2D_BUFFERS; i++) Dyn2DVertexBuffer[i].Vb->Unlock();

    CheckHR(m_pD3DD->CreateStateBlock(D3DSBT_ALL, &StateHandle));


    DXFlagsType Flags;
    m_FogLevel = m_LinearFogLevel;

    FlushInit();

#ifdef DEBUG_2D_ENGINE
    REPORT_VALUE("2D Vertices", Dyn2DVertexBuffer[0].LastIndex);
#endif
    SetStencilMode(STENCIL_CHECK);

    // unlock the Simple Buffer if used
    if (TheVbManager.SimpleBuffer.VbPtr)
    {
        TheVbManager.SimpleBuffer.Vb->Unlock();
        TheVbManager.SimpleBuffer.VbPtr = NULL;
    }

    /////////////////////// PROGRAM FOR 'SOLID' POINTS/LINES \\\\\\\\\\\\\\\\\\\\\

    // Reset any previous state flag
    Flags.w = 0xffff;
    SetRenderState(Flags, Flags, DISABLE);
    // adjust the flags for the surface
    Flags.w = 0;
    Flags.b.Alpha = Flags.b.Point = Flags.b.VColor = 1;
    // set the engine status
    SetRenderState(Flags, Flags, ENABLE);
    // set the engine status
    m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
    // Stup the Fog level fro this object
    m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGEND,   *(DWORD *)(&m_FogLevel));
    m_pD3DD->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    m_pD3DD->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_LIGHTING, FALSE);

    m_pD3DD->SetRenderState(D3DRENDERSTATE_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR2);

    ////////////////////////////// POINTS FLUSH \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

    if (TheVbManager.SimpleBuffer.MaxPoints)
    {
        // Make the draw
        m_pD3DD->DrawPrimitiveVB(D3DPT_POINTLIST, TheVbManager.SimpleBuffer.Vb, POINTS_OFFSET, TheVbManager.SimpleBuffer.Points, 0);
        TheVbManager.SimpleBuffer.Points = TheVbManager.SimpleBuffer.MaxPoints = 0;
    }


    ////////////////////////////// LINES FLUSH \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

    if (TheVbManager.SimpleBuffer.MaxLines)
    {
        // Make the draw
        m_pD3DD->DrawPrimitiveVB(D3DPT_LINELIST, TheVbManager.SimpleBuffer.Vb, LINES_OFFSET, TheVbManager.SimpleBuffer.MaxLines * 2, 0);
        TheVbManager.SimpleBuffer.Lines = TheVbManager.SimpleBuffer.MaxLines = 0;
    }

#ifdef DEBUG_2D_ENGINE
    START_PROFILE("DYN FLUSH:");
#endif
    DX2D_Flush2DObjects();
#ifdef DEBUG_2D_ENGINE
    STOP_PROFILE("DYN FLUSH:");
#endif
    //CheckHR(m_pD3DD->ApplyStateBlock(StateHandle));
    m_pD3DD->ApplyStateBlock(StateHandle);
    CheckHR(m_pD3DD->DeleteStateBlock(StateHandle));

    // Reset any previous state flag
    ResetFeatures();

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

    m_pD3DD->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, FALSE);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);



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
        m_pD3DD->SetTransform(D3DTRANSFORMSTATE_WORLD, (LPD3DMATRIX)&AppliedState);
        // Dof Level at 0
        DofLevel = 0;

        RadarMaterial.diffuse.a = m_BlipIntensity / 255.0f;

        m_pD3DD->SetMaterial(&RadarMaterial);


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

    m_pD3DD->SetRenderState(D3DRENDERSTATE_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
    m_pD3DD->SetRenderState(D3DRENDERSTATE_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR2);
    m_pD3DD->SetMaterial(&TheMaterial);

}


void CDXEngine::DrawBlitNode(void)
{
    ///////////////////////// Draw the Primitive /////////////////////////////////
#ifdef INDEXED_MODE_ENGINE
    if (m_NODE.SURFACE->dwPrimType == D3DPT_POINTLIST)
    {
        m_pD3DD->DrawPrimitiveVB(m_NODE.SURFACE->dwPrimType, m_VB.Vb, (DWORD) * ((Int16*)(m_NODE.BYTE + sizeof(DxSurfaceType))) + m_VB.BaseOffset,
                                 m_NODE.SURFACE->dwVCount, 0);
    }
    else
    {
        m_pD3DD->DrawIndexedPrimitiveVB(m_NODE.SURFACE->dwPrimType, m_VB.Vb, m_VB.BaseOffset, m_VB.NVertices,
                                        (LPWORD)(m_NODE.BYTE + sizeof(DxSurfaceType)), m_NODE.SURFACE->dwVCount, 0);
    }


#else
    m_pD3DD->DrawPrimitiveVB(m_NODE.SURFACE->dwPrimType, m_VB.Vb, (DWORD) * ((Int16*)(m_NODE.BYTE + sizeof(DxSurfaceType))) + m_VB.BaseOffset,
                             m_NODE.SURFACE->dwVCount, 0);
#endif

}
