#include "stdhdr.h"
#include "simfile.h"
#include "missile.h"
#include "missdata.h"
#include "initdata.h"
#include "datafile.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gReadInMemPool;
#endif

MissileDataSetClass* missileDataset = NULL;
int numMissileDatasets = 0;

#define MISSILE_DIR     "sim\\misdata"
#define MISSILE_DATASET "mistypes.lst"

void MissileClass::ReadInput(int idx)
{
    inputData = missileDataset[min(idx, numMissileDatasets - 1)].inputData;
    weight = inputData->wm0 + inputData->wp0;
    wprop = inputData->wp0;
    mass = weight / GRAVITY;
    m0 = inputData->wm0 / GRAVITY;
    mp0 = inputData->wp0 / GRAVITY;
    mprop = wprop / GRAVITY;
}

void ReadAllMissileData(void)
{
    int i;
    SimlibFileClass* mslList;
    SimlibFileClass* inputFile;
    char buffer[80];
    char fileName[_MAX_PATH];
    char fName[_MAX_PATH];

    // open input file
    sprintf(fileName, "%s\\%s\0", MISSILE_DIR, MISSILE_DATASET);
    mslList = SimlibFileClass::Open(fileName, SIMLIB_READ);
    F4Assert(mslList);

    numMissileDatasets = atoi(mslList->GetNext());
#ifdef USE_SH_POOLS
    missileDataset = (MissileDataSetClass *)MemAllocPtr(gReadInMemPool, sizeof(MissileDataSetClass) * numMissileDatasets, 0);
#else
    missileDataset = new MissileDataSetClass[numMissileDatasets];
#endif

    for (i = 0; i < numMissileDatasets; i++)
    {
        mslList->ReadLine(buffer, 80);
        // Open the basic input file for the missile
        sprintf(fName, "%s\\%s.dat", MISSILE_DIR, buffer);
        inputFile = SimlibFileClass::Open(fName, SIMLIB_READ);
        F4Assert(inputFile);
        // 2002-03-08 ADDED BY S.G. Why not read the name while we're at it...
        strncpy(missileDataset[i].name, buffer, sizeof(missileDataset[i].name));
        missileDataset[i].name[sizeof(missileDataset[i].name) - 1] = '\0';
        // END OF ADDED SECTION 2002-03-08
        missileDataset[i].inputData = MissileInputRead(inputFile);
        missileDataset[i].aeroData = MissileAeroRead(inputFile);
        missileDataset[i].engineData = MissileEngineRead(inputFile);
        missileDataset[i].rangeData = MissileRangeRead(inputFile);
        missileDataset[i].auxData = MissileAuxAeroRead(inputFile); // JPO
        inputFile->Close();
        delete inputFile;
    }

    mslList->Close();
    delete mslList;
}

void FreeAllMissileData(void)
{
    int i;

    for (i = 0; i < numMissileDatasets; i++)
    {
        delete missileDataset[i].aeroData;
        delete missileDataset[i].inputData;
        delete missileDataset[i].rangeData;
        delete missileDataset[i].engineData;
        delete missileDataset[i].auxData; // JPO
    }

    delete [] missileDataset;
}

MissileInputData* MissileInputRead(SimlibFileClass* inputFile)
{
    MissileInputData* inputData;

#ifdef USE_SH_POOLS
    inputData = (MissileInputData *)MemAllocPtr(gReadInMemPool, sizeof(MissileInputData), 0);
#else
    inputData = new MissileInputData;
#endif

    inputData->maxTof = (float)atof(inputFile->GetNext());
    inputData->mslpk = (float)atof(inputFile->GetNext());
    inputData->wm0 = (float)atof(inputFile->GetNext());
    inputData->wp0 = (float)atof(inputFile->GetNext());
    inputData->totalImpulse = (float)atof(inputFile->GetNext());
    inputData->area = (float)atof(inputFile->GetNext());
    inputData->nozzleArea = (float)atof(inputFile->GetNext());
    inputData->length = (float)atof(inputFile->GetNext());
    inputData->aoamax = (float)atof(inputFile->GetNext());
    inputData->aoamin = (float)atof(inputFile->GetNext());
    inputData->betmax = (float)atof(inputFile->GetNext());
    inputData->betmin = (float)atof(inputFile->GetNext());
    inputData->mslVmin = (float)atof(inputFile->GetNext());
    inputData->gimlim = (float)atof(inputFile->GetNext()) * DTR;
    inputData->gmdmax = (float)atof(inputFile->GetNext()) * DTR;
    inputData->atamax = (float)atof(inputFile->GetNext()) * DTR;
    inputData->guidanceDelay = (float)atof(inputFile->GetNext());
    inputData->mslBiasn = (float)atof(inputFile->GetNext());
    inputData->mslGnav = (float)atof(inputFile->GetNext());
    inputData->mslBwap = (float)atof(inputFile->GetNext());
    inputData->mslActiveTtg = (float)atof(inputFile->GetNext());
    inputData->seekerType = atoi(inputFile->GetNext());
    inputData->seekerVersion = atoi(inputFile->GetNext());
    inputData->displayType = atoi(inputFile->GetNext());
    // me123 until the dat files are up to date
    //inputData->SensorPrecision= (float)atoi(inputFile->GetNext());

    // LRKLUDGE
    //inputData->mslLoftTime = 10.0F;//40.0F;//me123 from 10
    // 2002-01-28 MN have missile loft time defined by data file
    inputData->mslLoftTime = (float)atof(inputFile->GetNext());

    //me123
    inputData->boostguidesec = (float)atof(inputFile->GetNext()); //me123 how many sec we are in boostguide mode
    inputData->terminalguiderange = (float)atof(inputFile->GetNext()); //me123 what range we transfere to terminal guidence
    inputData->boostguideSensorPrecision = (float)atof(inputFile->GetNext());
    inputData->sustainguideSensorPrecision = (float)atof(inputFile->GetNext());
    inputData->terminalguideSensorPrecision = (float)atof(inputFile->GetNext());
    inputData->boostguideLead = (float)atof(inputFile->GetNext());
    inputData->sustainguideLead = (float)atof(inputFile->GetNext());
    inputData->terminalguideLead = (float)atof(inputFile->GetNext());
    inputData->boostguideGnav = (float)atof(inputFile->GetNext());
    inputData->sustainguideGnav = (float)atof(inputFile->GetNext());
    inputData->terminalguideGnav = (float)atof(inputFile->GetNext());
    inputData->boostguideBwap = (float)atof(inputFile->GetNext());
    inputData->sustainguideBwap = (float)atof(inputFile->GetNext());
    inputData->terminalguideBwap = (float)atof(inputFile->GetNext());

    return (inputData);
}

MissileAeroData* MissileAeroRead(SimlibFileClass* inputFile)
{
    MissileAeroData* aeroData;
    float multiplier;
    int i, j;

    // Allocate memory for the aero data
#ifdef USE_SH_POOLS
    aeroData = (MissileAeroData *)MemAllocPtr(gReadInMemPool, sizeof(MissileAeroData), 0);
#else
    aeroData = new MissileAeroData;
#endif

    // read in mach breakpoints first
    // first read in the number of
    // breakpoints, then the actual points
    aeroData->numMach = atoi(inputFile->GetNext());

    // Allocate memory for the mach array
#ifdef USE_SH_POOLS
    aeroData->mach = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * aeroData->numMach, 0);
#else
    aeroData->mach = new float[aeroData->numMach];
#endif

    for (i = 0; i < aeroData->numMach; i++)
    {
        aeroData->mach[i] = (float)atof(inputFile->GetNext());
    }

    // then read in the alpha breakpoints
    aeroData->numAlpha = atoi(inputFile->GetNext());

    // Allocate memory for the alpha array
#ifdef USE_SH_POOLS
    aeroData->alpha = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * aeroData->numAlpha, 0);
#else
    aeroData->alpha = new float[aeroData->numAlpha];
#endif

    for (i = 0; i < aeroData->numAlpha; i++)
    {
        aeroData->alpha[i] = (float)atof(inputFile->GetNext());
    }

    // Allocate memory for the normal force array
#ifdef USE_SH_POOLS
    aeroData->cz = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * aeroData->numMach * aeroData->numAlpha, 0);
#else
    aeroData->cz = new float[aeroData->numAlpha * aeroData->numMach];
#endif

    // Get normal multiplier
    multiplier = (float)atof(inputFile->GetNext());

    for (i = 0; i < aeroData->numMach; i++)
    {
        for (j = 0; j < aeroData->numAlpha; j++)
        {
            aeroData->cz[i * aeroData->numAlpha + j] = (float)atof(inputFile->GetNext()) * multiplier;
        }
    }

    // Allocate memory for the normal force array
#ifdef USE_SH_POOLS
    aeroData->cx = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * aeroData->numMach * aeroData->numAlpha, 0);
#else
    aeroData->cx = new float[aeroData->numAlpha * aeroData->numMach];
#endif

    // Get normal multiplier
    multiplier = (float)atof(inputFile->GetNext());

    for (i = 0; i < aeroData->numMach; i++)
    {
        for (j = 0; j < aeroData->numAlpha; j++)
        {
            aeroData->cx[i * aeroData->numAlpha + j] = (float)atof(inputFile->GetNext()) * multiplier;
        }
    }

    return (aeroData);
}

MissileEngineData* MissileEngineRead(SimlibFileClass* inputFile)
{
    MissileEngineData* engineData;
    int i;

    // Allocate the engine data pointer
#ifdef USE_SH_POOLS
    engineData = (MissileEngineData *)MemAllocPtr(gReadInMemPool, sizeof(MissileEngineData), 0);
#else
    engineData = new MissileEngineData;
#endif

    // Read in burntime breakpoints
    engineData->numBreaks = atoi(inputFile->GetNext());

    // Allocate breakpoint and thrust arrays
#ifdef USE_SH_POOLS
    engineData->times = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * engineData->numBreaks, 0);
    engineData->thrust = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * engineData->numBreaks, 0);
#else
    engineData->times = new float[engineData->numBreaks];
    engineData->thrust = new float[engineData->numBreaks];
#endif

    // Read in breakpoints
    for (i = 0; i < engineData->numBreaks; i++)
    {
        engineData->times[i] = (float)atof(inputFile->GetNext());
    }

    // Read in thrust data
    for (i = 0; i < engineData->numBreaks; i++)
    {
        engineData->thrust[i] = (float)atof(inputFile->GetNext());
    }

    return (engineData);
}

MissileRangeData* MissileRangeRead(SimlibFileClass* inputFile)
{
    MissileRangeData* rangeData;
    int numAlt, numVel, numAspect;
    int i, j, k;
    float scaleFactor;

    // Allocate memory for the range data
#ifdef USE_SH_POOLS
    rangeData = (MissileRangeData *)MemAllocPtr(gReadInMemPool, sizeof(MissileRangeData), 0);
#else
    rangeData = new MissileRangeData;
#endif

    // Get scale factor
    scaleFactor = (float)atof(inputFile->GetNext());

    // Altitude breakpoints
    rangeData->numAltBreakpoints = atoi(inputFile->GetNext());
    ShiAssert(rangeData->numAltBreakpoints > 0 and rangeData->numAltBreakpoints < 100); // JPO some checks.
    numAlt = rangeData->numAltBreakpoints;
#ifdef USE_SH_POOLS
    rangeData->altBreakpoints = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * numAlt, 0);
#else
    rangeData->altBreakpoints = new float[numAlt];
#endif

    for (i = 0; i < numAlt; i++)
    {
        rangeData->altBreakpoints[i] = (float)atof(inputFile->GetNext());
    }

    // Velocity breakpoints
    rangeData->numVelBreakpoints = atoi(inputFile->GetNext());
    ShiAssert(rangeData->numVelBreakpoints > 0 and rangeData->numVelBreakpoints < 100); // JPO some checks.
    numVel = rangeData->numVelBreakpoints;
#ifdef USE_SH_POOLS
    rangeData->velBreakpoints = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * numVel, 0);
#else
    rangeData->velBreakpoints = new float[numVel];
#endif

    for (i = 0; i < numVel; i++)
    {
        rangeData->velBreakpoints[i] = (float)atof(inputFile->GetNext());
    }

    // Aspect breakpoints
    rangeData->numAspectBreakpoints = atoi(inputFile->GetNext());
    numAspect = rangeData->numAspectBreakpoints;
    ShiAssert(rangeData->numAspectBreakpoints > 0 and rangeData->numAspectBreakpoints < 100); // JPO some checks.
#ifdef USE_SH_POOLS
    rangeData->aspectBreakpoints = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * numAspect, 0);
#else
    rangeData->aspectBreakpoints = new float[numAspect];
#endif

    for (i = 0; i < numAspect; i++)
    {
        rangeData->aspectBreakpoints[i] = (float)atof(inputFile->GetNext());
    }

    // The range data
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
                rangeData->data[i * numVel * numAspect + j * numAspect + k] = (float)atof(inputFile->GetNext()) * scaleFactor;
            }
        }
    }

    return (rangeData);
}

// JPO structure to read in auxilary variables
#define OFFSET(x) offsetof(MissileAuxData, x)
static const InputDataDesc AuxMissileDataDesc[] =
{

    // RV - Biker - Read FOV data from missile FMs
    { "FOVLevel", InputDataDesc::ID_FLOAT, OFFSET(FOVLevel), "2"},
    { "EXPLevel", InputDataDesc::ID_FLOAT, OFFSET(EXPLevel), "4"},

    // RV - Biker - Read WEZ max/min from missile FMs in nm
    { "WEZmax", InputDataDesc::ID_FLOAT, OFFSET(WEZmax), "20"},
    { "WEZmin", InputDataDesc::ID_FLOAT, OFFSET(WEZmin), "20"},

    { "maxGTerminal", InputDataDesc::ID_FLOAT, OFFSET(maxGTerminal), "100"},
    { "maxGNormal", InputDataDesc::ID_FLOAT, OFFSET(maxGNormal), "40"},
    { "MinEngagementRange", InputDataDesc::ID_FLOAT, OFFSET(MinEngagementRange), "0"}, //  moved to radar data 2002-03-08 S.G. Reinstated here so it's more granular
    { "MinEngagementAlt", InputDataDesc::ID_FLOAT, OFFSET(MinEngagementAlt), "-1.0"}, //  2002-03-08 ADDED BY S.G. Instead of in radarData so it's more granular. Default to -1 so if it's not set in the dat file, it will use the Falcon4.WCD value instead.
    { "ProximityfuseChange", InputDataDesc::ID_FLOAT, OFFSET(ProximityfuseChange), "75"},
    { "SecondStageTimer", InputDataDesc::ID_FLOAT, OFFSET(SecondStageTimer), "0" },
    { "SecondStageWeight", InputDataDesc::ID_FLOAT, OFFSET(SecondStageWeight), "0"},
    { "deployableWingsTime", InputDataDesc::ID_FLOAT, OFFSET(deployableWingsTime), "0"}, // A.S.
    { "mistrail", InputDataDesc::ID_INT, OFFSET(mistrail), "0"},
    { "misengGlow", InputDataDesc::ID_INT, OFFSET(misengGlow), "0"},
    { "misengGlowBSP", InputDataDesc::ID_INT, OFFSET(misengGlowBSP), "0"},
    { "misgroundGlow", InputDataDesc::ID_INT, OFFSET(misgroundGlow), "0"},
    { "proximityfuserange", InputDataDesc::ID_INT, OFFSET(proximityfuserange), "1000"},
    { "errorfromparrent", InputDataDesc::ID_INT, OFFSET(errorfromparrent), "0"},
    { "engLocation", InputDataDesc::ID_VECTOR, OFFSET(misengLocation), "0,0,0"}, // MLR 2003-10-11
    { "EngineSound", InputDataDesc::ID_INT, OFFSET(EngineSound), "267"}, // MLR 2003-11-06 missles now have a sound when lit.
    { "rocketDispersionConeAngle", InputDataDesc::ID_FLOAT, OFFSET(rocketDispersionConeAngle), "0"}, // MLR 1/17/2004 -
    { "rocketSalvoSize", InputDataDesc::ID_INT,   OFFSET(rocketSalvoSize), "-1"}, // MLR 1/17/2004 -
    { "sndAim9Growl", InputDataDesc::ID_INT,   OFFSET(sndAim9Growl), "6"}, // SFX_GROWL // MLR 2/29/2004 -
    { "sndAim9GrowlLock",           InputDataDesc::ID_INT,   OFFSET(sndAim9GrowlLock), "7"}, // SFX_GROWLLOCK
    { "sndAim9Uncaged", InputDataDesc::ID_INT,   OFFSET(sndAim9Uncaged), "182"}, // SFX_NO_CAGE
    { "sndAim9EnviroSky",           InputDataDesc::ID_INT,   OFFSET(sndAim9EnviroSky), "186"}, // SFX_AIM9_ENVIRO_SKY
    { "sndAim9EnviroGround",        InputDataDesc::ID_INT,   OFFSET(sndAim9EnviroGround), "187"}, // SFX_AIM9_ENVIRO_GND

    { "pickleTimeDelay", InputDataDesc::ID_INT,   OFFSET(pickleTimeDelay), "0"}, // msec time pickle must be held for missile to launch
    { "psGroundImpact", InputDataDesc::ID_STRING,   OFFSET(psGroundImpact), ""},
    { "psMissileKill", InputDataDesc::ID_STRING,   OFFSET(psMissileKill), ""},
    { "psFeatureImpact", InputDataDesc::ID_STRING,   OFFSET(psFeatureImpact), ""},
    { "psBombImpact", InputDataDesc::ID_STRING,   OFFSET(psBombImpact), ""},
    { "psArmingDelay", InputDataDesc::ID_STRING,   OFFSET(psArmingDelay), ""},
    { "psExceedFOV", InputDataDesc::ID_STRING,   OFFSET(psExceedFOV), ""},

    //RV - I-Hawk - Get heat seeker gimbal tracking factor and launch sound ID from missile FMs
    { "gimbalTrackFactor", InputDataDesc::ID_FLOAT, OFFSET(gimbalTrackFactor), "1.0f"},
    { "launchSound", InputDataDesc::ID_INT, OFFSET(launchSound), "11"},

    { NULL},
};

MissileAuxData *MissileAuxAeroRead(SimlibFileClass* inputFile)
{
    MissileAuxData *auxmissileData;

    auxmissileData = new MissileAuxData();

    // RV - Biker - Why check when do nothing
    //if (ParseSimlibFile(auxmissileData, AuxMissileDataDesc, inputFile) == false) {
    //// F4Assert( not "Bad parsing of aux aero data");
    //}

    ParseSimlibFile(auxmissileData, AuxMissileDataDesc, inputFile);
    return (auxmissileData);
}
