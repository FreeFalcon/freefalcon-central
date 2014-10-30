#define bigasscow
#ifdef bigasscow
#include "stdhdr.h"
#include "simfile.h"
#include "Bomb.h"
#include "bombdata.h"
#include "initdata.h"
#include "datafile.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gReadInMemPool;
#endif

BombDataSetClass* BombDataset = NULL;
int numBombDatasets = 0;

#define Bomb_DIR     "sim\\bombdata"
#define Bomb_DATASET "bombtypes.lst"

BombAuxData DefaultBombAuxData;

BombInputData* BombInputRead(SimlibFileClass* inputFile);


// JPO structure to read in auxilary variables
#define OFFSET(x) offsetof(BombAuxData, x)
static const InputDataDesc AuxBombDataDesc[] =
{

    { "cbuStrengthModel", InputDataDesc::ID_INT, OFFSET(cbuStrengthModel), "0"},
    { "cbuLethalHeight", InputDataDesc::ID_FLOAT, OFFSET(cbuLethalHeight), "300"},
    { "cbuIneffectiveHeight", InputDataDesc::ID_FLOAT, OFFSET(cbuIneffectiveHeight), "6000"},
    { "cbuDamageDiameterBurstAltMultiplier", InputDataDesc::ID_FLOAT, OFFSET(cbuDamageDiameterBAMult), ".2083"},
    { "cbuMaxDamageDiameter",   InputDataDesc::ID_FLOAT, OFFSET(cbuMaxDamageDiameter), "300"},
    { "cbuBlastMultiplier", InputDataDesc::ID_FLOAT, OFFSET(cbuBlastMultiplier), "1.0"},
    { "sndFlightSFX", InputDataDesc::ID_FLOAT, OFFSET(sndFlightSFX), "0"},
    { "lauSalvoSize", InputDataDesc::ID_INT, OFFSET(lauSalvoSize), "-1"},
    { "lauWeaponId", InputDataDesc::ID_INT, OFFSET(lauWeaponId), "0"},
    { "lauRounds", InputDataDesc::ID_INT,      OFFSET(lauRounds), "0"},
    { "lauAzimuth", InputDataDesc::ID_FLOAT, OFFSET(lauAzimuth), "0"},
    { "lauElevation", InputDataDesc::ID_FLOAT, OFFSET(lauElevation), "0"},
    { "lauRippleTimeMS",        InputDataDesc::ID_INT,     OFFSET(lauRippleTime), "10"},
    { "psFeatureImpact", InputDataDesc::ID_STRING, OFFSET(psFeatureImpact), ""},
    { "psBombImpact", InputDataDesc::ID_STRING, OFFSET(psBombImpact), ""},
    { "JDAMLift", InputDataDesc::ID_FLOAT, OFFSET(JDAMLift), "5"},
    { "JSOWmaxRange", InputDataDesc::ID_FLOAT, OFFSET(JSOWmaxRange), "40"},

    { NULL},
};



void BombClass::ReadInput(int idx)
{
    if (BombDataset and idx < numBombDatasets)
    {
        auxData =
            BombDataset[min(idx, numBombDatasets - 1)].auxData;
    }
    else
    {
        auxData = &DefaultBombAuxData;
    }

    /*
    weight = inputData->wm0 + inputData->wp0;
    wprop  = inputData->wp0;
    mass   = weight/GRAVITY;
    m0     = inputData->wm0/GRAVITY;
    mp0    = inputData->wp0/GRAVITY;
    mprop  = wprop/GRAVITY;
    */
}

void ReadAllBombData(void)
{
    int i;
    SimlibFileClass* mslList;
    SimlibFileClass* inputFile;
    char buffer[80];
    char fileName[_MAX_PATH];
    char fName[_MAX_PATH];

    memset(&DefaultBombAuxData, 0, sizeof(DefaultBombAuxData));
    DefaultBombAuxData.lauRippleTime = 10;
    /*
    DefaultBombAuxData.cbuStrengthModel=0;
    DefaultBombAuxData.cbuLethalHeight=0;
    DefaultBombAuxData.cbuIneffectiveHeight=0;
    DefaultBombAuxData.sndFlightSFX=0;
    */

    /*-----------------*/
    /* open input file */
    /*-----------------*/
    sprintf(fileName, "%s\\%s\0", Bomb_DIR, Bomb_DATASET);
    mslList = SimlibFileClass::Open(fileName, SIMLIB_READ);

    //   F4Assert(mslList);
    if ( not mslList) // MLR 2003-11-11 Prevent CTD if files are missing.
        return;

    numBombDatasets = atoi(mslList->GetNext());
#ifdef USE_SH_POOLS
    BombDataset = (BombDataSetClass *)MemAllocPtr(gReadInMemPool, sizeof(BombDataSetClass) * numBombDatasets, 0);
#else
    BombDataset = new BombDataSetClass[numBombDatasets];
#endif
    memset(BombDataset, 0, sizeof(BombDataset));

    for (i = 0; i < numBombDatasets; i++)
    {
        mslList->ReadLine(buffer, 80);
        /*-------------------------------------------*/
        /* Open the basic input file for the Bomb */
        /*-------------------------------------------*/
        sprintf(fName, "%s\\%s.dat", Bomb_DIR, buffer);
        inputFile = SimlibFileClass::Open(fName, SIMLIB_READ);

        //F4Assert(inputFile);
        if (inputFile)
        {
            strncpy(BombDataset[i].name, buffer, sizeof(BombDataset[i].name));
            BombDataset[i].name[sizeof(BombDataset[i].name) - 1] = '\0';
            BombDataset[i].inputData = BombInputRead(inputFile);
            BombDataset[i].auxData = BombAuxAeroRead(inputFile); // JPO
            inputFile->Close();
            delete inputFile;
        }
    }

    mslList->Close();
    delete mslList;
}

void FreeAllBombData(void)
{
    if (BombDataset)
    {
        int i;

        for (i = 0; i < numBombDatasets; i++)
        {
            delete BombDataset[i].auxData; // JPO
        }

        delete [] BombDataset;
    }

    BombDataset = 0;
}

BombInputData* BombInputRead(SimlibFileClass* inputFile)
{
    BombInputData* inputData;

#ifdef USE_SH_POOLS
    inputData = (BombInputData *)MemAllocPtr(gReadInMemPool, sizeof(BombInputData), 0);
#else
    inputData = new BombInputData;
#endif

    inputData->Version = atoi(inputFile->GetNext());

    if (inputData->Version > BOMB_ID_VER)
    {
        // we are screwed
#ifdef USE_SH_POOLS
        // what to do???
        //inputData = (BombInputData *)MemAllocPtr(gReadInMemPool, sizeof(BombInputData),0);
#else
        delete inputData;
#endif
    }

    // no data to get for version 0;
    return (inputData);
}


BombAuxData *BombAuxAeroRead(SimlibFileClass* inputFile)
{
    BombAuxData *auxBombData;

    auxBombData = new BombAuxData;

    if (ParseSimlibFile(auxBombData, AuxBombDataDesc, inputFile) == false)
    {
        //     F4Assert( not "Bad parsing of aux aero data");
    }

    return (auxBombData);
}
#endif


#if 0
BombAeroData* BombAeroRead(SimlibFileClass* inputFile)
{
    BombAeroData* aeroData;
    float multiplier;
    int i, j;

    /*-----------------------------------*/
    /* Allocate memory for the aero data */
    /*-----------------------------------*/
#ifdef USE_SH_POOLS
    aeroData = (BombAeroData *)MemAllocPtr(gReadInMemPool, sizeof(BombAeroData), 0);
#else
    aeroData = new BombAeroData;
#endif

    /*--------------------------------*/
    /* read in mach breakpoints first */
    /* first read in the number of    */
    /* breakpoints, then the actual   */
    /* points                         */
    /*--------------------------------*/
    aeroData->numMach = atoi(inputFile->GetNext());

    /*------------------------------------*/
    /* Allocate memory for the mach array */
    /*------------------------------------*/
#ifdef USE_SH_POOLS
    aeroData->mach = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * aeroData->numMach, 0);
#else
    aeroData->mach = new float[aeroData->numMach];
#endif

    for (i = 0; i < aeroData->numMach; i++)
    {
        aeroData->mach[i] = (float)atof(inputFile->GetNext());
    }

    /*------------------------------------*/
    /* then read in the alpha breakpoints */
    /*------------------------------------*/
    aeroData->numAlpha = atoi(inputFile->GetNext());

    /*-------------------------------------*/
    /* Allocate memory for the alpha array */
    /*-------------------------------------*/
#ifdef USE_SH_POOLS
    aeroData->alpha = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * aeroData->numAlpha, 0);
#else
    aeroData->alpha = new float[aeroData->numAlpha];
#endif

    for (i = 0; i < aeroData->numAlpha; i++)
    {
        aeroData->alpha[i] = (float)atof(inputFile->GetNext());
    }

    /*--------------------------------------------*/
    /* Allocate memory for the normal force array */
    /*--------------------------------------------*/
#ifdef USE_SH_POOLS
    aeroData->cz = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * aeroData->numMach * aeroData->numAlpha, 0);
#else
    aeroData->cz = new float[aeroData->numAlpha * aeroData->numMach];
#endif

    // Get normal multiplier
    multiplier = (float)atof(inputFile->GetNext());

    for (i = 0; i < aeroData->numMach; i++)
        for (j = 0; j < aeroData->numAlpha; j++)
        {
            aeroData->cz[i * aeroData->numAlpha + j] = (float)atof(inputFile->GetNext()) * multiplier;
        }

    /*--------------------------------------------*/
    /* Allocate memory for the normal force array */
    /*--------------------------------------------*/
#ifdef USE_SH_POOLS
    aeroData->cx = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * aeroData->numMach * aeroData->numAlpha, 0);
#else
    aeroData->cx = new float[aeroData->numAlpha * aeroData->numMach];
#endif

    // Get normal multiplier
    multiplier = (float)atof(inputFile->GetNext());

    for (i = 0; i < aeroData->numMach; i++)
        for (j = 0; j < aeroData->numAlpha; j++)
        {
            aeroData->cx[i * aeroData->numAlpha + j] = (float)atof(inputFile->GetNext()) * multiplier;
        }

    return (aeroData);
}

BombEngineData* BombEngineRead(SimlibFileClass* inputFile)
{
    BombEngineData* engineData;
    int i;

    /*----------------------------------*/
    /* Allocate the engine data pointer */
    /*----------------------------------*/
#ifdef USE_SH_POOLS
    engineData = (BombEngineData *)MemAllocPtr(gReadInMemPool, sizeof(BombEngineData), 0);
#else
    engineData = new BombEngineData;
#endif

    /*------------------------------*/
    /* Read in burntime breakpoints */
    /*------------------------------*/
    engineData->numBreaks = atoi(inputFile->GetNext());

    /*---------------------------------------*/
    /* Allocate breakpoint and thrust arrays */
    /*---------------------------------------*/
#ifdef USE_SH_POOLS
    engineData->times = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * engineData->numBreaks, 0);
    engineData->thrust = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * engineData->numBreaks, 0);
#else
    engineData->times = new float[engineData->numBreaks];
    engineData->thrust = new float[engineData->numBreaks];
#endif

    /*---------------------*/
    /* Read in breakpoints */
    /*---------------------*/
    for (i = 0; i < engineData->numBreaks; i++)
    {
        engineData->times[i] = (float)atof(inputFile->GetNext());
    }

    /*---------------------*/
    /* Read in thrust data */
    /*---------------------*/
    for (i = 0; i < engineData->numBreaks; i++)
    {
        engineData->thrust[i] = (float)atof(inputFile->GetNext());
    }

    return (engineData);
}

BombRangeData* BombRangeRead(SimlibFileClass* inputFile)
{
    BombRangeData* rangeData;
    int numAlt, numVel, numAspect;
    int i, j, k;
    float scaleFactor;

    /*------------------------------------*/
    /* Allocate memory for the range data */
    /*------------------------------------*/
#ifdef USE_SH_POOLS
    rangeData = (BombRangeData *)MemAllocPtr(gReadInMemPool, sizeof(BombRangeData), 0);
#else
    rangeData = new BombRangeData;
#endif

    // Get scale factor
    scaleFactor = (float)atof(inputFile->GetNext());

    /*----------------------*/
    /* Altitude breakpoints */
    /*----------------------*/
    rangeData->numAltBreakpoints = atoi(inputFile->GetNext());
    ShiAssert(rangeData->numAltBreakpoints > 0 and rangeData->numAltBreakpoints < 100); // JPO some checks.
    numAlt = rangeData->numAltBreakpoints;
#ifdef USE_SH_POOLS
    rangeData->altBreakpoints = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * numAlt, 0);
#else
    rangeData->altBreakpoints = new float[numAlt];
#endif

    for (i = 0; i < numAlt; i++)
        rangeData->altBreakpoints[i] = (float)atof(inputFile->GetNext());

    /*----------------------*/
    /* Velocity breakpoints */
    /*----------------------*/
    rangeData->numVelBreakpoints = atoi(inputFile->GetNext());
    ShiAssert(rangeData->numVelBreakpoints > 0 and rangeData->numVelBreakpoints < 100); // JPO some checks.
    numVel = rangeData->numVelBreakpoints;
#ifdef USE_SH_POOLS
    rangeData->velBreakpoints = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * numVel, 0);
#else
    rangeData->velBreakpoints = new float[numVel];
#endif

    for (i = 0; i < numVel; i++)
        rangeData->velBreakpoints[i] = (float)atof(inputFile->GetNext());

    /*--------------------*/
    /* Aspect breakpoints */
    /*--------------------*/
    rangeData->numAspectBreakpoints = atoi(inputFile->GetNext());
    numAspect = rangeData->numAspectBreakpoints;
    ShiAssert(rangeData->numAspectBreakpoints > 0 and rangeData->numAspectBreakpoints < 100); // JPO some checks.
#ifdef USE_SH_POOLS
    rangeData->aspectBreakpoints = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * numAspect, 0);
#else
    rangeData->aspectBreakpoints = new float[numAspect];
#endif

    for (i = 0; i < numAspect; i++)
        rangeData->aspectBreakpoints[i] = (float)atof(inputFile->GetNext());

    /*----------------*/
    /* The range data */
    /*----------------*/
#ifdef USE_SH_POOLS
    rangeData->data = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * numAspect * numAlt * numVel, 0);
#else
    rangeData->data = new float[numAlt * numVel * numAspect];
#endif

    for (i = 0; i < numAlt; i++)
    {
        for (j = 0; j < numVel; j++)
        {
            for (k = 0; k < numAspect; k++)
            {
                rangeData->data[i * numVel * numAspect + j * numAspect + k] =
                    (float)atof(inputFile->GetNext()) * scaleFactor;
            }
        }
    }

    return (rangeData);
}
#endif

