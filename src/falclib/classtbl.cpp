// Use objects in directory ObjectSet0708 with this file.
/*
 * Machine Generated Class Table Constants loader file.
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 11-August-1999 at 16:28:04
 * Generated from file Access Class Table
 */

#include <windows.h>
#include <stdio.h>
#include "F4vu.h"
#include "ClassTbl.h"
#include "F4Find.h"
#include "entity.h"

extern bool g_bFFDBC;

// Which langauge should we use?
int gLangIDNum = 1;

/*
* Class Table Constant Init Function
*/

void InitClassTableAndData(char *name, char *objset)
{
    FILE* filePtr;
    char  fileName[MAX_PATH];

    if (stricmp(objset, "objects") not_eq 0)
    {
        //Check for correct object data version
        ShiAssert(stricmp("ObjectSet0708", objset) == 0);
    }

    sprintf(fileName, "%s\\%s.ini", FalconObjectDataDir, name);

    gLangIDNum = GetPrivateProfileInt("Lang", "Id", 0, fileName);

    filePtr = OpenCampFile(name, "ct", "rb");

    if (filePtr)
    {
        //fread (&NumEntities, sizeof (short), 1, filePtr);
        // FF - DB Control
        //fseek(filePtr, 0, SEEK_SET);
        fread(&NumEntities, sizeof(short), 1, filePtr);
        fseek(filePtr, 0, SEEK_SET);

        if (NumEntities == 0)
            g_bFFDBC = true;

        // FF - DB Control
        if (g_bFFDBC)
        {
            // FF - get real count of entries
            short iknt = 0;
            fseek(filePtr, 0, SEEK_END);
            fseek(filePtr, -2, SEEK_CUR);
            fread(&iknt, sizeof(short), 1, filePtr);
            fseek(filePtr, 0, SEEK_SET);
            // Move pointer past the 0 entries
            fread(&NumEntities, sizeof(short), 1, filePtr);

            if (NumEntities == 0)
                NumEntities = iknt;
        }
        else
        {
            fseek(filePtr, 0, SEEK_SET);
            fread(&NumEntities, sizeof(short), 1, filePtr);
        }

        Falcon4ClassTable = new Falcon4EntityClassType[NumEntities];
#if defined(_M_IX86)
        fread(Falcon4ClassTable, sizeof(Falcon4EntityClassType), NumEntities, filePtr);
#else
        // Artscout - 2026: x64 serialization fix. The .ct file stores
        // Falcon4EntityClassType in the 32-bit (x86) layout where the trailing
        // dataPtr is a 4-byte field (it actually holds a small index that
        // LoadClassTable fixes up into a real pointer later). On x64 the struct
        // is 4 bytes larger (dataPtr 8 bytes), so a bulk fread of sizeof()*N
        // over-reads and desyncs every entry -> garbage dataType/dataPtr ->
        // out-of-bounds write in the fixup loop. The prefix (vuClassData..
        // dataType) is byte-identical on disk and in x64 memory (pack(1), no
        // pointers/8-byte members in VuEntityType). Read the on-disk 32-bit
        // layout, copy the common prefix, and widen dataPtr.
        {
#pragma pack(1)
            struct DiskEntityClass
            {
                VuEntityType vuClassData;
                short        visType[7];
                short        vehicleDataIndex;
                uchar        dataType;
                unsigned int dataPtr; // 32-bit (x86) pointer/index slot on disk
            };
#pragma pack()
            DiskEntityClass *disk = new DiskEntityClass[NumEntities];
            fread(disk, sizeof(DiskEntityClass), NumEntities, filePtr);
            const size_t prefix = offsetof(DiskEntityClass, dataPtr);

            for (int n = 0; n < NumEntities; n++)
            {
                memcpy(&Falcon4ClassTable[n], &disk[n], prefix);
                Falcon4ClassTable[n].dataPtr = (void *)(size_t)disk[n].dataPtr;
            }

            delete[] disk;
        }
#endif
        fclose(filePtr);
    }
    else
    {
        Falcon4ClassTable = NULL;
    }
}
