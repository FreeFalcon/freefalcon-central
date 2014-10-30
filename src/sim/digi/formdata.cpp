#include "stdhdr.h"
#include "simfile.h"
#include "digi.h"

#define FORMATION_DATA_FILE   "formdat.fil"

#ifdef USE_SH_POOLS
extern MEM_POOL gReadInMemPool;
#endif

ACFormationData::ACFormationData(void)
{
    SimlibFileClass* formFile;
    int i, j;
    int num4Slots;
    int num2Slots;
    int formNum;

    /*---------------------*/
    /* open formation file */
    /*---------------------*/
    formFile = SimlibFileClass::Open(FORMATION_DATA_FILE, SIMLIB_READ);
    F4Assert(formFile);
    numFormations = atoi(formFile->GetNext());

#ifdef USE_SH_POOLS
    positionData = (PositionData **)MemAllocPtr(gReadInMemPool, sizeof(PositionData*)*numFormations, 0);
    twoposData = (PositionData *)MemAllocPtr(gReadInMemPool, sizeof(PositionData) * numFormations, 0);
#else
    positionData = new PositionData*[numFormations];
    twoposData = new PositionData[numFormations];
#endif


    for (i = 0; i < numFormations; i++)
    {
        num4Slots = atoi(formFile->GetNext());
        num2Slots = atoi(formFile->GetNext());
        formNum = atoi(formFile->GetNext());

        formFile->GetNext(); // Skip the formation name

#ifdef USE_SH_POOLS
        positionData[i] = (PositionData *)MemAllocPtr(gReadInMemPool, sizeof(PositionData) * num4Slots, 0);
#else
        positionData[i] = new PositionData[num4Slots];
#endif

        for (j = 0; j < num4Slots; j++)
        {
            positionData[i][j].relAz = (float)atof(formFile->GetNext()) * DTR;
            positionData[i][j].relEl = (float)atof(formFile->GetNext()) * DTR;
            positionData[i][j].range = (float)atof(formFile->GetNext()) * NM_TO_FT;
            positionData[i][j].formNum = formNum;
        }

        if (num2Slots)
        {
            twoposData[i].relAz = (float)atof(formFile->GetNext()) * DTR;
            twoposData[i].relEl = (float)atof(formFile->GetNext()) * DTR;
            twoposData[i].range = (float)atof(formFile->GetNext()) * NM_TO_FT;
            twoposData[i].formNum = formNum;
        }
        else
        {
            twoposData[i].relAz = positionData[i][0].relAz;
            twoposData[i].relEl = positionData[i][0].relEl;
            twoposData[i].range = positionData[i][0].range;
            twoposData[i].formNum = formNum;
        }
    }

    formFile->Close();
    delete formFile;
}

ACFormationData::~ACFormationData(void)
{
    int i;

    for (i = 0; i < numFormations; i++)
    {
        delete [] positionData[i];
        positionData[i] = NULL;
    }

    delete [] positionData;
    delete [] twoposData; // JPO memory leak fix
}


int ACFormationData::FindFormation(int msgNum)
{
    int i = 0;
    BOOL done = FALSE;

    while ( not done and i < numFormations)
    {
        if (positionData[i][0].formNum == msgNum)
        {
            done = TRUE;
        }
        else
        {
            i++;
        }
    }

    if (i == numFormations)
        i = 1;

    F4Assert(done == TRUE); // invalid msgNum or formation file needs updating
    return i;
}
