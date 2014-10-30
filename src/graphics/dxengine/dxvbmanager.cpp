#include <cISO646>
#include "../include/polylib.h"
#include "../../include/ComSup.h"
#include "DXdefines.h"
#include "../include/ObjectInstance.h"
#include "DXEngine.h"
#include "dxvbmanager.h"
#include "DXTools.h"
#ifndef DEBUG_ENGINE
#include "../../sim/INCLUDE/ivibedata.h"
#include "../../FALCLIB/include/fakerand.h"
#endif
CRITICAL_SECTION cs_VbManager;

#ifdef STAT_DX_ENGINE
#include "RedProfiler.h"
#endif
#ifdef DATE_PROTECTION
#include "include/ComSup.h"
extern bool DateOff;
#endif
extern int TheObjectLODsCount;

// The ONLY Vertex Buffers Manager
CDXVbManager TheVbManager;
DWORD VBModels;
DWORD VBase;

// * Constructor *
CDXVbManager::CDXVbManager()
{
    // Critical Section initialization
    InitializeCriticalSection(&cs_VbManager);
    // Initialization not yet done
    VBManagerInitialized = false;
}

CDXVbManager::~CDXVbManager(void)
{
}


CDrawItem::CDrawItem()
{
    Next = NextInPool = NULL;
}


// Encrypting function for the model pointed by 'Buffer'
VOID CDXVbManager::Encrypt(DWORD *Buffer)
{
    // The Crypting Key
    DWORD Key = KEY_CRYPTER;
    DWORD Size;

    // Scramble the Key
    Key += ((DxDbHeader*)Buffer)->Id;
    Size = ((DxDbHeader*)Buffer)->ModelSize / 4;
    Key *= ((DxDbHeader*)Buffer)->VBClass;
    Key += ((DxDbHeader*)Buffer)->ModelSize;
    Size -= 3;
    Buffer += (offsetof(DxDbHeader, ModelSize) + sizeof(DWORD)) / 4;


    // Encrypt data
    while (Size)
    {
        *Buffer xor_eq (Key * Size);
        Buffer++;
        Size--;
    }
}




// Decrypting function for the model pointed by 'Buffer'
VOID CDXVbManager::Decrypt(DWORD *Buffer)
{
    // The Crypting Key
    DWORD Key = KEY_CRYPTER;
    DWORD Size;

    // Scramble the Key
    Key += ((DxDbHeader*)Buffer)->Id;
    Size = ((DxDbHeader*)Buffer)->ModelSize / 4;
    Key *= ((DxDbHeader*)Buffer)->VBClass;
    Key += ((DxDbHeader*)Buffer)->ModelSize;
    Size -= 3;
    Buffer += (offsetof(DxDbHeader, ModelSize) + sizeof(DWORD)) / 4;


    // Decrypt data
    while (Size)
    {
        *Buffer xor_eq (Key * Size);
        Buffer++;
        Size--;
    }
}



CVbVAT::CVbVAT(CVbVAT *Parent, CVbVAT *Child, DWORD Id, DWORD Start, DWORD Size, DWORD VBFree)
{
    // Assign parents
    Next = Child;
    Prev = Parent;

    // and updates its Indexes
    VStart = Start;
    VSize = Size;
    ID = Id;
    // Defaults the Gap to the end of the VB
    Gap = VBFree - Size;

    // updates values for Parent if existant
    if (Prev)
    {
        Prev->Next = this;
        Prev->Gap = Start - Prev->VStart - Prev->VSize;
    }

    // updates values for child if existant
    if (Next)
    {
        Next->Prev = this;
        Gap = Next->VStart - VStart - VSize;
    }
}


// The VAT Destructor
CVbVAT::~CVbVAT(void)
{
}




void CDXVbManager::DestroyVAT(VBufferListType *pVb, CVbVAT *Vat)
{
    // if a Parent, link it to next Item and Update its data
    if (Vat->Prev)
    {
        Vat->Prev->Next = Vat->Next;
        // The parent Gap will increased by the VAT Gap and it's VSize
        Vat->Prev->Gap += Vat->Gap + Vat->VSize;

        // if a Child, Link it to next Item
        if (Vat->Next) Vat->Next->Prev = Vat->Prev;
    }
    else
    {
        // if no parent, the Upadte the Root of the VAT Table
        if (Vat->Next)
        {
            pVb->BootGap = Vat->Next->VStart;
            pVb->pVAT = Vat->Next;
        }
        else
        {
            pVb->BootGap = 0xffff;
            pVb->pVAT = NULL;
        }

        // the next ecames the Root
        if (Vat->Next) Vat->Next->Prev = NULL;
    }

    //  Kill It
    delete Vat;
}




// This is mapped to initialize Vertex Buffers Class
const DWORD BaseClassFeaturesMap[BASE_VERTEX_BUFFERS] =
{
    VB_CLASS_FEATURES,
    VB_CLASS_FEATURES,
    VB_CLASS_DOMAIN_GROUND,
    VB_CLASS_DOMAIN_GROUND,
    VB_CLASS_DOMAIN_AIR,
    VB_CLASS_DOMAIN_AIR,
    VB_CLASS_DOMAIN_AIR,
    VB_CLASS_DOMAIN_AIR,
};


// This function creates a VB and assign it at position 'i' in the VBs list
// and assigns it the Class
void CDXVbManager::CreateVB(DWORD i, DWORD Class)
{
    // Creates the Vertex Buffer Descriptor
    D3DVERTEXBUFFERDESC VBDesc;
    VBDesc.dwSize = sizeof(D3DVERTEXBUFFERDESC);
    VBDesc.dwCaps = D3DVBCAPS_WRITEONLY;
    VBDesc.dwFVF = D3DFVF_MANAGED;
    VBDesc.dwNumVertices = 0xffff;

    CheckHR(D3D->CreateVertexBuffer(&VBDesc, &pVbList[i].Vb, NULL));
    pVbList[i].Free = 0x10000;
    pVbList[i].Class = Class;
    pVbList[i].BootGap = 0xffff;
    pVbList[i].DrawsCount = 0;
}



// COBRA - RED -
// Main Vertex Buffer Manager initialization
// The Base Number of vertex buffers are immediatly allocated
// and the local D3D Device assigned
void CDXVbManager::Setup(IDirect3D7 *pD3D)
{
    // if already initialized exit here
    if (VBManagerInitialized) return;

    // Initialize the Buffer Pointers
    ZeroMemory(pVBuffers, sizeof(pVBuffers));

    // Assign the D3D Direct Draw Device
    m_pD3D = pD3D;

    ///////////// Creates the vertex Buffers and assigns their Addresses /////////////
    ZeroMemory(pVbList, sizeof(pVbList));
    ZeroMemory(&PitList, sizeof(PitList));


    for (int i = 0; i < BASE_VERTEX_BUFFERS; i++)
    {
        // Creates the Vertex Buffer
        CreateVB(i, BaseClassFeaturesMap[i]);
    }


    ///////////// Creates the DRAW ITEMS Pools and assign pointers //////////////
    pVDrawItemPool = NULL;
    CDrawItem* dp = NULL;

    for (int i = 0; i < BASE_DRAWS; i++)
    {
        // if 1st item
        if ( not pVDrawItemPool) pVDrawItemPool = dp = new CDrawItem();
        else dp = dp->NextInPool = new CDrawItem();
    }

    // Rests the Pool Pointer for the Draw Items
    pDrawPoolPtr = pVDrawItemPool;
    VBModels = 0;
    TotalDraws = 0;

    // The Simple Items buffer creation
    D3DVERTEXBUFFERDESC VBDesc;
    VBDesc.dwSize = sizeof(D3DVERTEXBUFFERDESC);
    memset(&SimpleBuffer, 0, sizeof(SimpleBuffer));
    VBDesc.dwCaps = D3DVBCAPS_WRITEONLY;
    VBDesc.dwFVF = D3DFVF_SIMPLE;
    VBDesc.dwNumVertices = 0xffff;
    CheckHR(D3D->CreateVertexBuffer(&VBDesc, &SimpleBuffer.Vb, NULL));

    //////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

    VBase = 0;

    // Initialization done
    VBManagerInitialized = true;
}


// This is the release Function for the Vertex Buffer Manager
void CDXVbManager::Release(void)
{

    // if already NON initialized exit here
    if ( not VBManagerInitialized) return;

    // Release all eventually allocated models
    //for(int i=0; i<MAX_MANAGED_MODELS; i++) ReleaseModel(i);
    // FRB - Reduced number from 16384 to the number of LODs in KO.dxh
    for (int i = 0; i < TheObjectLODsCount; i++) ReleaseModel(i);

    // Release all vertex Buffers
    for (int i = 0; i < MAX_VERTEX_BUFFERS; i++)
    {
        if (pVbList[i].Vb) pVbList[i].Vb->Release();
    }

    for (int i = 0; i < MAX_DYNAMIC_BUFFERS; i++)
    {
        if (DynamicBuffer[i].Vb) DynamicBuffer[i].Vb->Release();

        DynamicBuffer[i].LastIndex = 0;
    }

    ///////////// Removes the DRAW ITEMS Pools //////////////
    CDrawItem *dp = pVDrawItemPool, *dn;

    while (dp)
    {
        dn = dp->NextInPool;
        delete dp;
        dp = dn;
    }

    // Resets the Pool Pointer for the Draw Items
    pDrawPoolPtr = pVDrawItemPool = NULL;
    VBModels = 0;
    TotalDraws = 0;

    ///////////////// REMOVE THE SIMPLE BUFFER ///////////////
    if (SimpleBuffer.Vb) SimpleBuffer.Vb->Release();

    SimpleBuffer.Vb = NULL;

    // Clears the Draw List
    ClearDrawList();

    // Initialization to redo
    VBManagerInitialized = false;
}



// This Function traverses the VAT of the Vertex Buffers List to find a chunk of space
// for the passed number of vertices... If found, the new VAT item is added to the list
// and the Index in the Vertex Buffer for the starting point is returned
// else is returned CHUNK_NOT_FOUND
DWORD CDXVbManager::VBAddObject(VBufferListType *Vbl, DWORD nVertices, DWORD ID)
{
    // If not enough space or wrong Vertex Buffer class skip
    if (Vbl->Free >= nVertices)
    {

        ///////////////// if Here, check for a chunk of space in the VB ///////////////////////////
        // if a free buffer, no VAT still assigned, assign it and exit
        if ( not Vbl->pVAT)
        {
            // Create and assign the VAT
            Vbl->pVAT = new CVbVAT(NULL, NULL, ID, 0, nVertices, Vbl->Free);
            //Update VB Free Space
            Vbl->Free -= nVertices;
            // Gap to 1st Item is Zero..
            Vbl->BootGap = 0x0000;
            // Assign the Object Buffer Descriptor VAT
            pVBuffers[ID].pVAT = Vbl->pVAT;
            // return the Index Start for the VB
            return Vbl->pVAT->VStart;
        }

        CVbVAT *Vat = Vbl->pVAT;

        //////////////////// if Here, Traverse the VAT to look for space //////////////////////////
        // Till End of List
        while (Vat)
        {

            // if already a Boot Gap with enough or Vertices
            if (Vbl->BootGap >= nVertices)
            {
                // Insert new VAT btw Root and the present VAT
                Vbl->pVAT = new CVbVAT(NULL, Vbl->pVAT, ID, 0x0000, nVertices, Vbl->Free);
                //Update VB Free Space
                Vbl->Free -= nVertices;
                // Gap to 1st Item is Zero..
                Vbl->BootGap = 0x0000;
                // Assign the Object Buffer Descriptor VAT
                pVBuffers[ID].pVAT = Vbl->pVAT;
                return Vbl->pVAT->VStart;
            }

            // if found a gap equal or major than requested Vertices add a new VAT
            if (Vat->Gap >= nVertices)
            {
                Vat = new CVbVAT(Vat, Vat->Next, ID, Vat->VStart + Vat->VSize, nVertices, Vbl->Free);
                // Assign the Object Buffer Descriptor VAT
                pVBuffers[ID].pVAT = Vat;
                // Update the Vertices Free Count in the Bufer descriptor
                Vbl->Free -= nVertices;
                return Vat->VStart;
            }

            Vat = Vat->Next;
        }
    }

    // if here, No Space Available
    return CHUNK_NOT_FOUND;
}



// This function looks for an Avilable VB Buffer for the object ID of nVertices Vertices
// if ound, updates the pVuBuffers Data for the ID and returns true
bool CDXVbManager::VBCheckForBuffer(DWORD ID, DWORD Class, DWORD nVertices)
{
    int i = 0;

    // if invalid class, exit here
    if ( not Class) return false;

    // Scan all possible VBs
    while (i < MAX_VERTEX_BUFFERS)
    {

        // if a still inexistant VB ( no class assigned ) create it 
        if ( not pVbList[i].Class) CreateVB(i, Class);

        // if a VB with wrong Class next VB and skip
        if ( not (pVbList[i].Class bitand Class))
        {
            i++;
            continue;
        }

        DWORD Base;

        // if here, the right Class, check for Space and eventually scan
        // if found assign data to the Objects Buffer pointer whose ID is passed
        if ((Base = VBAddObject(&pVbList[i], nVertices, ID)) not_eq CHUNK_NOT_FOUND)
        {
            // Assign the Buffer address
            pVBuffers[ID].Vb = pVbList[i].Vb;
            pVBuffers[ID].BaseOffset = Base;
            pVBuffers[ID].pVbList = &pVbList[i];
            return true;
        }

        i++;
    }

    // if here, all is full 
    return false;

}












// COBRA - RED -
// This function is called whenever there is to prepare a model or drawing
// if the model is not already present it disposes vertex buffer, Nodes List and mark the model as present
bool CDXVbManager::SetupModel(DWORD ID, BYTE *Root, DWORD Class)
{
    // Local Copy of data start
    DWORD *rt = (DWORD*)Root;

    // FRB - Hack to skip huge bad model (dwNVertices > 2 million verts)
    if ((ID == 0) and (((DxDbHeader*)rt)->dwNVertices) > 100)
        return false;

    // If model not already present in list
    if ( not pVBuffers[ID].Valid)
    {

#ifdef CRYPTED_MODELS
        Decrypt(rt);
#endif

        // Get number of vertices in the Model and look for Buffer Space, if no Space Exit
        DWORD dwNVertices = ((DxDbHeader*)rt)->dwNVertices;

        // FRB - Class = 0 is rejected...??
        if (Class == 0)
            Class = 7; // Airborne, probably cockpit

        if ( not VBCheckForBuffer(ID, Class, dwNVertices)) goto Failure;

        DWORD pVPool = ((DxDbHeader*)rt)->pVPool;
        //********************************************************************
        // Allocate local copy of Nodes, no more managed by the old code
        // pVPool is the offset from start of model of the Vertex Pool, so,
        // also the size of Header+Nodes
        void *ptr = NULL;

        if ( not (ptr = malloc(pVPool))) goto Failure;

        // Copy the nodes
        memcpy(ptr, Root, pVPool);
        //*********************************************************************

        // store the pointer to the model
        pVBuffers[ID].Root = (BYTE*)ptr;
        rt = (DWORD*)ptr;

        // Texture pointers update
        pVBuffers[ID].NTex = ((DxDbHeader*)rt)->dwTexNr;
        pVBuffers[ID].Texs = (DWORD*)((BYTE*)rt + sizeof(DxDbHeader));

        // fetch number of nodes, Offset of vertex Pool, size of Vertex Pool, Nr of Vertices in the Vertex Pool
        DWORD dwNodesNr = ((DxDbHeader*)rt)->dwNodesNr;
        DWORD dwPoolSize = ((DxDbHeader*)rt)->dwPoolSize;

        pVBuffers[ID].NVertices = dwNVertices;
        pVBuffers[ID].NNodes = dwNodesNr;

        // The Nodes
        pVBuffers[ID].Nodes = (void*)((BYTE*)rt + sizeof(DxDbHeader) + pVBuffers[ID].NTex * sizeof(DWORD));

        // FRB - Buffer overrun ??? DX7 buffer size limit
        if ((pVBuffers[ID].BaseOffset + dwNVertices) > 0xffff)
            goto Failure;

        // Copy the Vertices in the Assigned Vertex Buffer
        LOCK_VB_MANAGER;
        CheckHR(pVBuffers[ID].Vb->Lock(DDLOCK_NOOVERWRITE bitor DDLOCK_NOSYSLOCK bitor DDLOCK_SURFACEMEMORYPTR bitor DDLOCK_WAIT bitor DDLOCK_WRITEONLY, &ptr, NULL));
        memcpy((void*)((BYTE*)ptr + (pVBuffers[ID].BaseOffset * VERTEX_STRIDE)), Root + pVPool, dwNVertices * VERTEX_STRIDE);
        pVBuffers[ID].Vb->Unlock();
        // Exit the Critical section
        UNLOCK_VB_MANAGER;

#ifdef STAT_DX_ENGINE
        COUNT_PROFILE("VB Added : ");
        VBModels++;
#endif

    }

    // Object Allocated
    return true;

Failure:
    ;

    // Failure to allocate Object
    return false;
}



//  This function just releases a Model and free the memory and VBuffer
void CDXVbManager::ReleaseModel(DWORD ID)
{
    if (ID  >= (WORD) TheObjectLODsCount)
        return;

    // Exit if not a valid model
    if ( not pVBuffers[ID].Valid) return;

    // Enter the Critical section
    LOCK_VB_MANAGER; // FRB

    // *** The model is NO MORE VALID - Used to avoid model calls in cached draws ***
    pVBuffers[ID].Valid = false;

    // Free Model allocated memory
    if (pVBuffers[ID].Root) free(pVBuffers[ID].Root);

    // Then destroy all
    if (pVBuffers[ID].pVAT) DestroyVAT(pVBuffers[ID].pVbList, pVBuffers[ID].pVAT);

    if (pVBuffers[ID].pVbList) pVBuffers[ID].pVbList->Free += pVBuffers[ID].NVertices;

    pVBuffers[ID].Vb = NULL;
    pVBuffers[ID].Nodes = NULL;
    pVBuffers[ID].Texs = NULL;
    pVBuffers[ID].NTex = 0;
    pVBuffers[ID].Root = NULL;
    pVBuffers[ID].pVAT = NULL;
    pVBuffers[ID].pVbList = NULL;


    // Exit the Critical section
    UNLOCK_VB_MANAGER; // FRB

#ifdef STAT_DX_ENGINE
    COUNT_PROFILE("VB Released : ");
    VBModels--;
#endif
}


// Given a model ID and a Texture Index, returns the Texture ID
DWORD CDXVbManager::GetTextureID(DWORD ID, DWORD TexIdx)
{
    if (ID >= (WORD) TheObjectLODsCount)
        return 0;

    // Consistency  checking
    if ( not pVBuffers[ID].Valid) return 0;

    if (TexIdx >= pVBuffers[ID].NTex) return 0;

    return(*(pVBuffers[ID].Texs + TexIdx));

}


// Retuns the pointer to a
void CDXVbManager::GetModelData(VBItemType &vi, DWORD ID)
{
    vi = pVBuffers[ID];
}


// Checks if a Model ID is valid
bool CDXVbManager::CheckDataID(DWORD ID)
{
    if (ID >= (WORD) TheObjectLODsCount)
        return false;

    return(pVBuffers[ID].Valid);

}



// The Draw request enqueuer
void CDXVbManager::AddDrawItem(VBufferListType *pVBDesc, DWORD ID, ObjectInstance *objInst, D3DXMATRIX *Transformation, bool Lited, DWORD LightID, float FogLevel)
{

#ifdef DATE_PROTECTION
    extern IntellivibeData g_intellivibeData;

    if (DateOff and g_intellivibeData.In3D and PRANDFloat() < 0.3f) return;

#endif

    // if Draw list for this VB is Empty
    if ( not pVBDesc->pDrawRoot)
    {
        // Assigns the Draw Item
        pVBDesc->pDrawPtr = pVBDesc->pDrawRoot = pDrawPoolPtr;
        // no Prev, this is the 1st Item of the List
        pDrawPoolPtr->Prev = NULL;
    }
    else
    {
        // if A list already present append the new item
        pVBDesc->pDrawPtr->Next = pDrawPoolPtr;
        // BackWard Link
        pDrawPoolPtr->Prev = pVBDesc->pDrawPtr;
        // and make it the Last of the List
        pVBDesc->pDrawPtr = pDrawPoolPtr;
    }

    // one more draw present for this buffer
    pVBDesc->DrawsCount++;
    // clear its end link
    pVBDesc->pDrawPtr->Next = NULL;


    // Assign data to the Draw Item
    pDrawPoolPtr->Object = objInst;
    pDrawPoolPtr->RotMatrix = *Transformation;
    pDrawPoolPtr->ID = ID;
    pDrawPoolPtr->DynamicLited = Lited;
    pDrawPoolPtr->LightID = LightID;
    pDrawPoolPtr->FogLevel = FogLevel;


    // check if no more Draw Items in the Draw Items Pool add a new one
    if ( not pDrawPoolPtr->NextInPool) pDrawPoolPtr->NextInPool = new CDrawItem();

    // and select this as the next Draw Item to Use
    pDrawPoolPtr = pDrawPoolPtr->NextInPool;
}



// This function appends a Draw request for an Object to the VB the object belongs to
void CDXVbManager::AddDrawRequest(ObjectInstance *objInst, DWORD ID, D3DXMATRIX *Transformation, bool Lited, DWORD LightID, float FogLevel)
{
    if (ID >= (WORD) TheObjectLODsCount)
        return;

    // Enter the Critical section
    LOCK_VB_MANAGER; // FRB

    // if Such Object allocated in the Vertex Buffers
    if (pVBuffers[ID].Valid)
    {
        VBufferListType *pVBDesc;

        // if not a pit object
        if ( not TheDXEngine.GetPitMode())
            // Get the Vertex Buffer Descriptor
            pVBDesc = pVBuffers[ID].pVbList;
        else
            // else get the Pit List
            pVBDesc = &PitList;

        // Add the draw Item to the Vertex Buffer List
        AddDrawItem(pVBDesc, ID, objInst, Transformation, Lited, LightID, FogLevel);
        // one draw more
        TotalDraws++;
    }

    // Exit the Critical section
    UNLOCK_VB_MANAGER; // FRB

#ifdef STAT_DX_ENGINE
    REPORT_VALUE("VB Models :", VBModels);
#endif
}



// This function Resets the draw List making them ready to be flushed
void CDXVbManager::ResetDrawList(void)
{
    // Start from buffer 0 Draw Item Pointers
    BufferToDraw = 0;
    RootItemToDraw = NextItemToDraw = pVbList[BufferToDraw].pDrawRoot;

#ifdef USE_DRAW_SCRAMBLER
    DrawPass = 1;
    ZeroMemory(DrawHits, sizeof(DrawHits));
#endif
}



// This function Clear all Draw list to hold nothing
void CDXVbManager::ClearDrawList(void)
{
    // Reset each Buffer Pointer
    for (int a = 0; a < MAX_VERTEX_BUFFERS; a++)
    {
        // Draw Item Ptr at Start of List
        pVbList[a].DrawsCount = 0;
        // Draw Item Ptr at Start of List
        pVbList[a].pDrawPtr = pVbList[a].pDrawRoot = NULL;
    }

    //////////////////////////////// THE PIT LIST \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
    // Draw Item Ptr at Start of List
    PitList.DrawsCount = 0;
    // Draw Item Ptr at Start of List
    PitList.pDrawPtr = PitList.pDrawRoot = NULL;
    /////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

    // Reset The pointer of the Draw Item Pool
    pDrawPoolPtr = pVDrawItemPool;

    // Reset total draws
    TotalDraws = 0;

    // Reset Draw Passes or the next Buffer
    DrawPass = 1;
}



// This function fill variables of the next Item to draw, returns FALSE if no more Itames to draw
bool CDXVbManager::GetDrawItem(ObjectInstance **objInst, DWORD *ID, D3DXMATRIX *Transformation, bool *Lited, DWORD *LightID, float *FogLevel)
{
    bool Traversed = false;


    // Check for Pit List
    if (PitList.DrawsCount)
    {
        if (PitList.pDrawRoot)
        {
            *objInst = (ObjectInstance*)PitList.pDrawRoot->Object;
            *Transformation = PitList.pDrawRoot->RotMatrix;
            *ID = PitList.pDrawRoot->ID;
            *Lited = PitList.pDrawRoot->DynamicLited;
            *LightID = PitList.pDrawRoot->LightID;
            *FogLevel = PitList.pDrawRoot->FogLevel;
            PitList.pDrawRoot = PitList.pDrawRoot->Next;
            PitList.DrawsCount--;
            TotalDraws--;
#ifndef DEBUG_ENGINE
            //COUNT_PROFILE("PIT OBJECTS");
#endif
            // Signal the DX Engine this is Pit Stuff
            TheDXEngine.SetPitMode(true);
            return true;
        }
        else
        {
            PitList.DrawsCount = 0;
            PitList.pDrawRoot = NULL;
        }
    }

    // No Pit Stuff
    TheDXEngine.SetPitMode(false);

    // if No More draws then exit here
    if ( not TotalDraws) return false;

    // if the Buffer has no VB assigned, we r at end of Used Buffer List... restart
    if ( not pVbList[BufferToDraw].Vb)
    {
        BufferToDraw = 0;
        Traversed = true;
    }

    // if no more Items to draw in the selected VBuffer, seeks in other Buffers
    while ( not pVbList[BufferToDraw].DrawsCount)
    {
        // No more to Draw, then clear the DRAW ROOT
        pVbList[BufferToDraw].pDrawRoot = NULL;
        // look for next buffer
        BufferToDraw++;

        // if the new Buffer has no VB assigned, we r at end of Used Buffer List...
        if ( not pVbList[BufferToDraw].Vb)
        {
            // CONSISTENCY CHECK - AVOID EVERLASTING LOOPS
            // if already traversed all buffer end here
            if (Traversed) return false;

            // Restart from buffer 0
            BufferToDraw = 0;
            // and flag the retraversing of buffers
            Traversed = true;
        }
    }

    RootItemToDraw = pVbList[BufferToDraw].pDrawRoot;

    // Assign data for the caller
    *objInst = (ObjectInstance*)RootItemToDraw->Object;
    *Transformation = RootItemToDraw->RotMatrix;
    *ID = RootItemToDraw->ID;
    *Lited = RootItemToDraw->DynamicLited;
    *LightID = RootItemToDraw->LightID;
    *FogLevel = RootItemToDraw->FogLevel;

    // Unlink this Item and assign next if a next available
    if (RootItemToDraw->Next)
    {
        pVbList[BufferToDraw].pDrawRoot = RootItemToDraw->Next;

        // One Draw Less for this Buffer, if no more draws, clear the Draw pointer
        if ( not (--pVbList[BufferToDraw].DrawsCount)) pVbList[BufferToDraw].pDrawRoot = NULL;
    }
    else
    {
        // if no more an item no more draws from this buffer, reset it's list and count
        pVbList[BufferToDraw].pDrawRoot = NULL;
        pVbList[BufferToDraw].DrawsCount = 0;
    }

    // one draw less
    TotalDraws--;

    return true;
}





// The simple buffer opening function
void CDXVbManager::OpenSimpleBuffer(void)
{
    if ( not SimpleBuffer.VbPtr)
    {
        SimpleBuffer.Vb->Lock(DDLOCK_DISCARDCONTENTS bitor DDLOCK_NOSYSLOCK bitor DDLOCK_WAIT bitor DDLOCK_WRITEONLY, (void**)&TheVbManager.SimpleBuffer.VbPtr, NULL);
        SimpleBuffer.Points = SimpleBuffer.Lines = 0;
        SimpleBuffer.MaxLines = SimpleBuffer.MaxPoints = 0;
    }
}


