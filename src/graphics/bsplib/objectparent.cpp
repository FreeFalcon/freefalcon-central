/***************************************************************************\
    ObjectParent.cpp
    Scott Randolph
    February 9, 1998

    Provides structures and definitions for 3D objects.
\***************************************************************************/
#include <cISO646>
#include "stdafx.h"
#include <io.h>
#include <fcntl.h>
#include "StateStack.h"
#include "TexBank.h"
#include "PalBank.h"
#include "ColorBank.h"
#include "ObjectParent.h"
#include "falclib/include/IsBad.h"

#include "Graphics/DXEngine/DXDefines.h"
extern bool g_bUse_DX_Engine;
unsigned long DXver = 0xFEEF;
extern int nVer;

ObjectParent *TheObjectList = NULL;
int TheObjectListLength = 0;

#ifdef DEBUG_LOD_ID
char TheLODNames[10000][32];
#endif

#ifdef USE_SH_POOLS
MEM_POOL gBSPLibMemPool = NULL;
#endif

ObjectParent::ObjectParent()
{
    refCount = 0;
    pLODs = NULL;
    pSlotAndDynamicPositions = NULL;
}


ObjectParent::~ObjectParent()
{
    ShiAssert(refCount == 0);

#ifdef USE_SH_POOLS
    MemFreePtr(pSlotAndDynamicPositions);
#else
    delete[] pSlotAndDynamicPositions;
#endif
    pSlotAndDynamicPositions = NULL;

#ifdef USE_SH_POOLS
    MemFreePtr(pLODs);
#else
    delete[] pLODs;
#endif
    pLODs = NULL;
    Locked = false;
}


void ObjectParent::SetupTable(char *basename)
{
    int file;
    char filename[_MAX_PATH];

#ifdef USE_SH_POOLS

    if (gBSPLibMemPool == NULL)
    {
        gBSPLibMemPool = MemPoolInit(0);
    }

#endif

    // COBRA - DX - Switching btw Old and New Engine - Select the right Header File
    strcpy(filename, basename);
    // Open the master object file
    strcat(filename, ".DXH");

    file = open(filename, _O_RDONLY bitor _O_BINARY bitor _O_SEQUENTIAL);

    if (file < 0)
    {
        char message[256];
        sprintf(message, "Failed to open object header file %s\n", filename);
        ShiError(message);
    }

    // Read the format version
    VerifyVersion(file);

    // Read the Color Table from the master file
    TheColorBank.ReadPool(file);

    // Read the Palette Table from the master file
    ThePaletteBank.ReadPool(file);

    // Read the Texture Table from the master file
    TheTextureBank.ReadPool(file, basename);

    // Read the object LOD headers from the master file
    ObjectLOD::SetupTable(file, basename);

    // Read the parent object records from the master file
    ReadParentList(file);

    // Close the master file
    close(file);
}


void ObjectParent::CleanupTable(void)
{
    // Make sure all the parent objects are freed
    FlushReferences();

    // Free the LOD table
    ObjectLOD::CleanupTable();

    // Free the texture, palette, and color banks
    TheTextureBank.Cleanup();
    ThePaletteBank.Cleanup();
    TheColorBank.Cleanup();

    // Free our array of parent objects
#ifdef USE_SH_POOLS
    MemFreePtr(TheObjectList);
#else
    delete[] TheObjectList;
#endif
    TheObjectList = NULL;
    TheObjectListLength = 0;

#ifdef USE_SH_POOLS

    if (gBSPLibMemPool not_eq NULL)
    {
        MemPoolFree(gBSPLibMemPool);
        gBSPLibMemPool = NULL;
    }

#endif
}



// Serialization Function, reading the File of parents
static int ReadParentRecord(ObjectParent &Obj, int file)
{
    ParentFileRecord Record;
    // Get the single record
    int result = read(file, &Record, sizeof(ParentFileRecord));

    // if any record, exit here
    if (result < 0) return result;

    // Assign data
    Obj.radius = Record.radius;
    Obj.minX = Record.minX;
    Obj.maxX = Record.maxX;
    Obj.minY = Record.minY;
    Obj.maxY = Record.maxY;
    Obj.minZ = Record.minZ;
    Obj.maxZ = Record.maxZ;

    Obj.RadarSign = Record.RadarSign;
    Obj.IRSign = Record.IRSign;

    Obj.nTextureSets = Record.nTextureSets;
    Obj.nDynamicCoords = Record.nDynamicCoords;
    Obj.nLODs = Record.nLODs;

    // old nSwitches and nDOFs?
    if (nVer)
    {
        Obj.nSwitches = Record.nSwitches; // new
        Obj.nDOFs = Record.nDOFs;
    }
    else
    {
        Obj.nSwitches = (short)Record.nSwitch; // old
        Obj.nDOFs = (short)Record.nDOF;
    }

    Obj.nSlots = Record.nSlots;

    return result;
}



void ObjectParent::ReadParentList(int file)
{
    ObjectParent *objParent;
    ObjectParent *end;
    int i;
    int result;


    // Read the length of the parent object array
    result = read(file, &TheObjectListLength, sizeof(TheObjectListLength));


    // Allocate memory for the parent object array
#ifdef USE_SH_POOLS
    TheObjectList = (ObjectParent *)MemAllocPtr(gBSPLibMemPool, sizeof(ObjectParent) * TheObjectListLength, 0);
#else
    TheObjectList = new ObjectParent[TheObjectListLength];
#endif

    ShiAssert(TheObjectList);


    /*// Now read the elements of the parent array
    result = read( file, TheObjectList, sizeof(*TheObjectList)*TheObjectListLength );
    if (result < 0 ) {
     char message[256];
     sprintf( message, "Reading object list:  %s", strerror(errno) );
     ShiError( message );
    }*/

    // RED - Rewritten with serialization
    int Object = 0;

    // for each parent record
    while (Object < TheObjectListLength)
    {
        // init the parameters
        TheObjectList[Object].Locked = false;
        TheObjectList[Object].pLODs = NULL;
        TheObjectList[Object].refCount = 0;

        // read it, and check result
        int result = ReadParentRecord(TheObjectList[Object], file);

        if (result < 0)
        {
            char message[256];
            sprintf(message, "Reading object list:  %s", strerror(errno));
            ShiError(message);
        }

        // next object
        Object++;
    }



    // Finally, read the reference arrays for each parent in order
    end = TheObjectList + TheObjectListLength;

    for (objParent = TheObjectList; objParent < end; objParent++)
    {

        // Skip any unused objects
        if (objParent->nLODs == 0)
        {
            continue;
        }

        // Allocate and read this parent's slot and dynamic position array
        if (objParent->nSlots or objParent->nDynamicCoords)
        {

#ifdef USE_SH_POOLS
            objParent->pSlotAndDynamicPositions = (Tpoint *)MemAllocPtr(gBSPLibMemPool, sizeof(Ppoint) * (objParent->nSlots + objParent->nDynamicCoords), 0);
#else
            objParent->pSlotAndDynamicPositions = new Ppoint[objParent->nSlots + objParent->nDynamicCoords];
#endif

            result = read(file,
                          objParent->pSlotAndDynamicPositions,
                          (objParent->nSlots + objParent->nDynamicCoords) * sizeof(*objParent->pSlotAndDynamicPositions));
        }
        else
            objParent->pSlotAndDynamicPositions = NULL; // JPO - jsut in case its wrong in the file

        // Allocate memory for this parent's reference list

#ifdef USE_SH_POOLS
        objParent->pLODs = (LODrecord *)MemAllocPtr(gBSPLibMemPool, sizeof(LODrecord) * (objParent->nLODs), 0);
#else
        objParent->pLODs = new LODrecord[objParent->nLODs];
#endif

        // Read the reference list
        char LODName[32];

        for (i = 0; i < objParent->nLODs; i++)
        {
            if (g_bUse_DX_Engine) read(file, LODName, sizeof(LODName));

            result = read(file, &objParent->pLODs[i], sizeof(*objParent->pLODs));
#ifdef DEBUG_LOD_ID

            // LOD ID DEBUG
            if (g_bUse_DX_Engine) memcpy(&TheLODNames[((int)(objParent->pLODs[i].objLOD) >> 1)], LODName, sizeof(LODName));;

#endif

            if (result < 0)
            {
                char message[256];
                sprintf(message, "Reading object reference list:  %s", strerror(errno));
                ShiError(message);
            }
        }

        // Fixup the LOD references
        ShiAssert(TheObjectLODs);

        for (i = 0; i < objParent->nLODs; i++)
        {

            // Replace the offset of the LOD with a pointer into TheObjectLOD array.
            // NOTE:  We're shifting the offset right one bit to clear our special
            // marker.
            objParent->pLODs[i].objLOD = &TheObjectLODs[((int)(objParent->pLODs[i].objLOD) >> 1) ];
        }
    }
}


void ObjectParent::VerifyVersion(int file)
{
    int result;
    UInt32 fileVersion;
    char message[80];

    // Read the magic number at the head of the file
    result = read(file, &fileVersion, sizeof(fileVersion));

    // If the version doesn't match our expectations, report an error
    if (fileVersion not_eq FORMAT_VERSION)
    {
        //Beep( 2000, 500 );
        //Beep( 2000, 500 );
        sprintf(message, "Got object format version 0x%08X, want 0x%08X", fileVersion, FORMAT_VERSION);
        ShiError(message);
    }

    // New version of KO,dxh which uses UINT's for nSwitches and nDOFs to handle the increased number of Switch and DOF ID's.
    if (nVer not_eq (int)DXver)
        nVer = 0; // old KO.dxh version
}


void ObjectParent::FlushReferences(void)
{
    // RED - Rewritten accounting also of Locked
    for (int i = 0; i < TheObjectListLength; i++)
    {
        // if any Ref still present, or locked, release WITH UNLOCK
        if (TheObjectList[i].refCount or TheObjectList[i].Locked)
        {
            TheObjectList[i].Release(true);
        }
    }
}

void ObjectParent::ReferenceTexSet(DWORD TexSet, DWORD MaxTexSet)
{
    LODrecord *record = pLODs + nLODs - 1;

    while (record >= pLODs)
    {
        record->objLOD->ReferenceTexSet(TexSet, (MaxTexSet) ? MaxTexSet : nTextureSets);
        record--;
    }
}

void ObjectParent::ReleaseTexSet(DWORD TexSet, DWORD MaxTexSet)
{
    // RED - if object is locked, do not release
    if (Locked) return;

    LODrecord *record = pLODs + nLODs - 1;

    while (record >= pLODs)
    {
        record->objLOD->ReleaseTexSet(TexSet, (MaxTexSet) ? MaxTexSet : nTextureSets);
        record--;
    }
}


void ObjectParent::Reference(void)
{
    // RED - Possible CTD Fix, if no LODs, exit
    if ( not nLODs) return;

    if (refCount == 0)
    {
        // Reference each of our child LODs (loaded or not)
        LODrecord *record = pLODs + nLODs - 1;

        ShiAssert(FALSE == F4IsBadReadPtr(pLODs, sizeof(*pLODs)));

        while (record >= pLODs)
        {
            ShiAssert(FALSE == F4IsBadReadPtr(record, sizeof(*record)));  // JPO CTD check

            //if (record and not F4IsBadReadPtr(record, sizeof(LODrecord)) and record->objLOD and not F4IsBadCodePtr((FARPROC) record->objLOD)) // JB 010221 CTD
            if (record and not F4IsBadReadPtr(record, sizeof(LODrecord)) and record->objLOD and not F4IsBadReadPtr(record->objLOD, sizeof(ObjectLOD))) // JB 010318 CTD
                record->objLOD->Reference();

            record--;
        }
    }

    refCount++;
}


void ObjectParent::ReferenceWithFetch(void)
{
    // RED - Possible CTD Fix, if no LODs, exit
    if ( not nLODs) return;

    if (refCount == 0)
    {
        // RED - This function is called only by LockAndLoad(), so, lock the object
        Locked = true;

        // Reference each of our child LODs (loaded or not)
        LODrecord *record = pLODs + nLODs - 1;

        while (record >= pLODs)
        {
            ShiAssert(FALSE == F4IsBadReadPtr(record, sizeof * record)); // JPO CTD check
            record->objLOD->Reference();
            record->objLOD->Fetch();
            record->objLOD->ReferenceTexSet(0, nTextureSets);
            record--;
        }
    }

    refCount++;
}


void ObjectParent::Release(bool Unlock)
{
    LODrecord *record = pLODs + nLODs - 1;

    // Now reduce our reference count
    if (refCount) 
        refCount--;

    if (Unlock) 
        Locked = false;

    // RED - Release if count eraches 0, and OBJECT IS NOT LOCKED
    if (refCount == 0 and not Locked)
    {
        // Dereference each of our child LODs
        while (record >= pLODs)
        {
            record->objLOD->Release();
            record--;
        }
    }
}


// COBRA - RED - searching from Near to Far, inverse of before, so that a nearest object LOD
// is found before in part compensating more complex graphic drawing time
ObjectLOD* ObjectParent::ChooseLOD(float range, int *used, float *max_range)
{
    LODrecord *record = NULL; // pLODs+nLODs-1;
    ObjectLOD *objLODptr = NULL;
    int LOD = 0;
    int MaxLOD;

    // 2002-03-29 MN possible CTD fix
    ShiAssert(FALSE == F4IsBadReadPtr(pLODs, sizeof * pLODs));

    if ( not pLODs)
        return NULL;

    MaxLOD = nLODs;

    record = pLODs;

    // COBRA - RED - Check each LOD from HIGHEST detail to LOWEST appropriate
    while (record and MaxLOD--)
    {
        ShiAssert(FALSE == F4IsBadReadPtr(record, sizeof * record)); // JPO CTD check

        if (range < record->maxRange)
        {
            if (record->objLOD->Fetch())
            {
                objLODptr = record->objLOD;
                *max_range = record->maxRange;
                *used = LOD;
            }

            break;
        }

        LOD ++;
        record++;
        ShiAssert(record == NULL or FALSE == F4IsBadReadPtr(record, sizeof * record));
    }

    // Return our final drawing candidate (if any)
    return objLODptr;
}
