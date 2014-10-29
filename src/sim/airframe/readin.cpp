/******************************************************************************/
/*                                                                            */
/*  Unit Name : readin.cpp                                                    */
/*                                                                            */
/*  Abstract  : Routines to read in all aero data and assign the correct set  */
/*              to each A/C.                                                  */
/*                                                                            */
/*  Naming Conventions :                                                      */
/*                                                                            */
/*      Public Functions    : Mixed case with no underscores                  */
/*      Private Functions   : Mixed case with no underscores                  */
/*      Public Functions    : Mixed case with no underscores                  */
/*      Global Variables    : Mixed case with no underscores                  */
/*      Classless Functions : Mixed case with no underscores                  */
/*      Classes             : All upper case seperated by an underscore       */
/*      Defined Constants   : All upper case seperated by an underscore       */
/*      Macros              : All upper case seperated by an underscore       */
/*      Structs/Types       : All upper case seperated by an underscore       */
/*      Private Variables   : All lower case seperated by an underscore       */
/*      Public Variables    : All lower case seperated by an underscore       */
/*      Local Variables     : All lower case seperated by an underscore       */
/*                                                                            */
/*  Development History :                                                     */
/*  Date      Programer           Description                                 */
/*----------------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                               */
/*                                                                            */
/******************************************************************************/
#include "stdhdr.h"
#include "airframe.h"
#include "arfrmdat.h"
#include "simfile.h"
#include "limiters.h"
#include "datafile.h"
#include "aircrft.h"

#define AIRFRAME_DIR     "sim\\acdata"
#define AIRFRAME_DATASET "actypes.lst"

#ifdef USE_SH_POOLS
MEM_POOL AirframeDataPool;
extern MEM_POOL gReadInMemPool;
#endif

/*-------------------------------*/
/* Global Vars  */
/*-------------------------------*/
AeroDataSet *aeroDataset = NULL;
int num_sets = 0;

AeroDataSet::~AeroDataSet(void)
{
    delete fcsData;
    delete aeroData;
    delete auxaeroData; // JB 010714
    delete engineData;
}

void AirframeClass::ReSetMaxRoll(void)
{
    maxRoll = aeroDataset[vehicleIndex].inputData[AeroDataSet::MaxRoll];
}

void AirframeDataInitializeStorage(void)
{
#ifdef USE_SH_POOLS
    AirframeDataPool = MemPoolInit(0);
#endif
}

void AirframeDataReleaseStorage(void)
{
#ifdef USE_SH_POOLS
    MemPoolFree(AirframeDataPool);
#endif
}

/********************************************************************/
/*                                                                  */
/* Routine: void AirframeClass::ReadData (char *, char *)          */
/*                                                                  */
/* Description:                                                     */
/*    Reads in the initial condition of for the A/C.                */
/*                                                                  */
/* Inputs:                                                          */
/*    char *acname  - Aircraft type name (File name)                */
/*    char *data_in - Data directory                                */
/*                                                                  */
/* Outputs:                                                         */
/*    None                                                          */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
int AirframeClass::ReadData(int idx)
{
    ShiAssert(idx < num_sets);

    if (idx >= num_sets)
        return 0;

    vehicleIndex = (short)idx;
    /*----------------------------*/
    /* Initialize Inertial Coords */
    /*----------------------------*/
    mach    = initialMach;

    if (mach > 3.0F)
        mach *= FTPSEC_TO_KNOTS;

    SetPosition(initialX, initialY, initialZ);
    sigma   = initialPsi;

    emptyWeight = weight = aeroDataset[idx].inputData[AeroDataSet::EmptyWeight];
    area = aeroDataset[idx].inputData[AeroDataSet::Area];

    if (initialFuel < 0.0F)
    {
        fuel = aeroDataset[idx].inputData[AeroDataSet::InternalFuel];
    }
    else if (initialFuel > aeroDataset[idx].inputData[AeroDataSet::InternalFuel])
    {
        fuel = aeroDataset[idx].inputData[AeroDataSet::InternalFuel];
        externalFuel = initialFuel - aeroDataset[idx].inputData[AeroDataSet::InternalFuel];
    }
    else
    {
        fuel = initialFuel;
    }

    weight += (fuel + externalFuel);

    aoamax = aeroDataset[idx].inputData[AeroDataSet::AOAMax];
    aoamin = aeroDataset[idx].inputData[AeroDataSet::AOAMin];
    betmax = aeroDataset[idx].inputData[AeroDataSet::BetaMax];
    betmin = aeroDataset[idx].inputData[AeroDataSet::BetaMin];
    curMaxGs = maxGs = aeroDataset[idx].inputData[AeroDataSet::MaxGs];
    maxRoll = aeroDataset[idx].inputData[AeroDataSet::MaxRoll];
    minVcas = aeroDataset[idx].inputData[AeroDataSet::MinVcas];
    curMaxStoreSpeed = maxVcas = aeroDataset[idx].inputData[AeroDataSet::MaxVcas];//me123 addet curMaxVcas
    cornerVcas = aeroDataset[idx].inputData[AeroDataSet::CornerVcas];

    gear = new GearData[FloatToInt32(aeroDataset[idx].inputData[AeroDataSet::NumGear])];

    for (int i = 0; i < aeroDataset[idx].inputData[AeroDataSet::NumGear]; i++)
    {
        gear[i].strength = 100.0F; //how many hitpoints it has left
        gear[i].vel = 0.0F; //at what rate is it currently compressing/extending in ft/s
        gear[i].obstacle = 0.0F; //rock height/rut depth
        gear[i].flags = 0;
    }

    if (aeroDataset[idx].inputData[AeroDataSet::NumGear] > 1)
        SetFlag(HasComplexGear);

    return (0);
}

void ReadData(float* inputData, SimlibFileClass* inputFile)
{
    /*-----------------*/
    /* Read in Weight */
    /*-----------------*/
    inputData[AeroDataSet::EmptyWeight] = (float)atof(inputFile->GetNext());

    /*------------------*/
    /* Read in Ref Area */
    /*------------------*/
    inputData[AeroDataSet::Area] = (float)atof(inputFile->GetNext());

    /*--------------*/
    /* Read in Fuel */
    /*--------------*/
    inputData[AeroDataSet::InternalFuel] = (float)atof(inputFile->GetNext());

    /*--------------------------------*/
    /* read in angle of attack limits */
    /*--------------------------------*/

    /*----------------*/
    /* Read in AOAmax */
    /*----------------*/
    inputData[AeroDataSet::AOAMax] = (float)atof(inputFile->GetNext());

    /*----------------*/
    /* Read in AOAmin */
    /*----------------*/
    inputData[AeroDataSet::AOAMin] = (float)atof(inputFile->GetNext());

    /*-----------------*/
    /* Read in BETAmax */
    /*-----------------*/
    inputData[AeroDataSet::BetaMax] = (float)atof(inputFile->GetNext());

    /*-----------------*/
    /* Read in BETAmin */
    /*-----------------*/
    inputData[AeroDataSet::BetaMin] = (float)atof(inputFile->GetNext());

    /*-----------------*/
    /* Read in Max G's */
    /*-----------------*/
    inputData[AeroDataSet::MaxGs] = (float)atof(inputFile->GetNext());

    /*------------------*/
    /* Read in Max Roll */
    /*------------------*/
    inputData[AeroDataSet::MaxRoll] = (float)atof(inputFile->GetNext()) * DTR;

    /*-------------------*/
    /* Read in Min Speed */
    /*-------------------*/
    inputData[AeroDataSet::MinVcas] = (float)atof(inputFile->GetNext());

    /*-------------------*/
    /* Read in Max Speed */
    /*-------------------*/
    inputData[AeroDataSet::MaxVcas] = (float)atof(inputFile->GetNext());

    /*----------------------*/
    /* Read in Corner Speed */
    /*----------------------*/
    inputData[AeroDataSet::CornerVcas] = (float)atof(inputFile->GetNext());

    /*-------------------*/
    /* Read in Theta Max */
    /*-------------------*/
    inputData[AeroDataSet::ThetaMax] = (float)atof(inputFile->GetNext()) * DTR;

    /*-------------------*/
    /* Read in NumGear */
    /*-------------------*/
    inputData[AeroDataSet::NumGear] = (float)atof(inputFile->GetNext());

    /*-------------------*/
    /* Read in GearPos */
    /*-------------------*/
    for (int i = 0; i < inputData[AeroDataSet::NumGear]; i++)
    {
        inputData[AeroDataSet::NosGearX + i * 4] = (float)atof(inputFile->GetNext());
        inputData[AeroDataSet::NosGearY + i * 4] = (float)atof(inputFile->GetNext());
        inputData[AeroDataSet::NosGearZ + i * 4] = (float)atof(inputFile->GetNext());
        inputData[AeroDataSet::NosGearRng + i * 4] = (float)atof(inputFile->GetNext());
    }

    inputData[AeroDataSet::CGLoc] = (float)atof(inputFile->GetNext());
    inputData[AeroDataSet::Length] = (float)atof(inputFile->GetNext());
    inputData[AeroDataSet::Span] = (float)atof(inputFile->GetNext());
    inputData[AeroDataSet::FusRadius] = (float)atof(inputFile->GetNext());
    inputData[AeroDataSet::TailHt] = (float)atof(inputFile->GetNext());
}

/********************************************************************/
/*                                                                  */
/* Routine: void AirframeClass::AeroRead(char *)                   */
/*                                                                  */
/* Description:                                                     */
/*    Find the right data set for this A/C.                         */
/*                                                                  */
/* Inputs:                                                          */
/*    char *acname  - Aircraft type name (File name)                */
/*                                                                  */
/* Outputs:                                                         */
/*    None                                                          */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
void AirframeClass::AeroRead(int idx)
{
    ShiAssert(idx < num_sets);

    if (idx >= num_sets)
        return;

    aeroData = aeroDataset[idx].aeroData;
}

void AirframeClass::AuxAeroRead(int idx)
{
    ShiAssert(idx < num_sets);

    if (idx >= num_sets)
        return;

    auxaeroData = aeroDataset[idx].auxaeroData;
}

/********************************************************************/
/*                                                                  */
/* Routine: void AirframeClass::EngineRead(char *)                 */
/*                                                                  */
/* Description:                                                     */
/*    Find the right data set for this A/C.                         */
/*                                                                  */
/* Inputs:                                                          */
/*    char *acname  - Aircraft type name (File name)                */
/*                                                                  */
/* Outputs:                                                         */
/*    None                                                          */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
void AirframeClass::EngineRead(int idx)
{
    ShiAssert(idx < num_sets);

    if (idx >= num_sets)
        return;

    engineData = aeroDataset[idx].engineData;
}

/********************************************************************/
/*                                                                  */
/* Routine: void AirframeClass::FcsRead(char *)                    */
/*                                                                  */
/* Description:                                                     */
/*    Find the right data set for this A/C.                         */
/*                                                                  */
/* Inputs:                                                          */
/*    char *acname  - Aircraft type name (File name)                */
/*                                                                  */
/* Outputs:                                                         */
/*    None                                                          */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
void AirframeClass::FcsRead(int idx)
{
    ShiAssert(idx < num_sets);

    if (idx >= num_sets)
        return;

    rollCmd = aeroDataset[idx].fcsData;
}

/********************************************************************/
/*                                                                  */
/* Routine: void ReadAllAirframeData (char *)                       */
/*                                                                  */
/* Description:                                                     */
/*    Read all Airframe data sets available.                        */
/*                                                                  */
/* Inputs:                                                          */
/*    char *data_in - Data directory                                */
/*                                                                  */
/* Outputs:                                                         */
/*    None                                                          */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
void ReadAllAirframeData(void)
{
    int i;
    SimlibFileClass* aclist;
    SimlibFileClass* inputFile;
    SimlibFileName buffer;
    SimlibFileName fileName;
    SimlibFileName fName;

    // Allocate pools
    AirframeDataInitializeStorage();

    /*-----------------*/
    /* open input file */
    /*-----------------*/
    sprintf(fileName, "%s\\%s\0", AIRFRAME_DIR, AIRFRAME_DATASET);
    aclist = SimlibFileClass::Open(fileName, SIMLIB_READ);
    num_sets = atoi(aclist->GetNext());

    /*
    #ifdef USE_SH_POOLS
    aeroDataset = (AeroDataSet *)MemAllocPtr(AirFrameDataPool, sizeof(AeroDataSet)*num_sets,0);
    #else
    aeroDataset = new AeroDataSet[num_sets];
    #endif
    */

    aeroDataset = new AeroDataSet[num_sets];

    if (gLimiterMgr)
        delete gLimiterMgr;

    gLimiterMgr = new LimiterMgrClass(num_sets);

    /*--------------------------*/
    /* Do each file in the list */
    /*--------------------------*/
    for (i = 0; i < num_sets; i++)
    {
        aclist->ReadLine(buffer, 80);

        /*-----------------*/
        /* open input file */
        /*-----------------*/
        sprintf(fName, "%s\\%s.dat", AIRFRAME_DIR, buffer);
        inputFile = SimlibFileClass::Open(fName, SIMLIB_READ);

        F4Assert(inputFile);
        ReadData(aeroDataset[i].inputData, inputFile);
        aeroDataset[i].aeroData = AirframeAeroRead(inputFile);
        aeroDataset[i].engineData = AirframeEngineRead(inputFile);
        aeroDataset[i].fcsData = AirframeFcsRead(inputFile);
        gLimiterMgr->ReadLimiters(inputFile, i);
        aeroDataset[i].auxaeroData = AirframeAuxAeroRead(inputFile); // JB 010714
        inputFile->Close();
        delete inputFile;
        inputFile = NULL;
    }

    aclist->Close();
    delete aclist;
    aclist = NULL;
}

void FreeAllAirframeData(void)
{
    delete [] aeroDataset;

    // Deallocate pools
    AirframeDataReleaseStorage();
}

/********************************************************************/
/*                                                                  */
/* Routine: AeroData *AirframeAeroRead (SimlibFileClass* inputFile)  */
/*                                                                  */
/* Description:                                                     */
/*    Read one set of aero data.                                    */
/*                                                                  */
/* Inputs:                                                          */
/*    inputFile - File handle to read                               */
/*                                                                  */
/* Outputs:                                                         */
/*    aeroData  - aerodynamic data set                              */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
AeroData *AirframeAeroRead(SimlibFileClass* inputFile)
{
    int i, numMach, numAlpha;
    int mach, alpha;
    AeroData *aeroData;

    /*-----------------------------------------------*/
    /* Allocate a block of memeory for the aero data */
    /*-----------------------------------------------*/
    aeroData = new AeroData;

    /*-------------------------*/
    /* Read in Num Mach Breaks */
    /*-------------------------*/
    aeroData->numMach = atoi(inputFile->GetNext());
    numMach = aeroData->numMach;

    /*-------------------------------------------*/
    /* Allocate storage for the mach breakpoints */
    /*-------------------------------------------*/
#ifdef USE_SH_POOLS
    aeroData->mach = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * aeroData->numMach, 0);
#else
    aeroData->mach = new float[aeroData->numMach];
#endif

    /*------------------------------*/
    /* Read in the Mach breakpoints */
    /*------------------------------*/
    for (i = 0; i < numMach; i++)
    {
        aeroData->mach[i] = (float)atof(inputFile->GetNext());
    }

    /*--------------------------*/
    /* Read in Num Alpha Breaks */
    /*--------------------------*/
    aeroData->numAlpha = atoi(inputFile->GetNext());
    numAlpha = aeroData->numAlpha;

    /*--------------------------------------------*/
    /* Allocate storage for the Alpha breakpoints */
    /*--------------------------------------------*/
#ifdef USE_SH_POOLS
    aeroData->alpha = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * aeroData->numAlpha, 0);
#else
    aeroData->alpha = new float[aeroData->numAlpha];
#endif

    /*-------------------------------*/
    /* Read in the Alpha breakpoints */
    /*-------------------------------*/
    for (i = 0; i < numAlpha; i++)
    {
        aeroData->alpha[i] = (float)atof(inputFile->GetNext());
    }

    /*---------------------------------*/
    /* Allocate memeory for CLIFT data */
    /*---------------------------------*/
#ifdef USE_SH_POOLS
    aeroData->clift = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * numMach * numAlpha, 0);
#else
    aeroData->clift =  new float[numAlpha * numMach];
#endif

    /*-----------------------*/
    /* Read in lift modifier */
    /*-----------------------*/
    aeroData->clFactor = (float)atof(inputFile->GetNext());

    /*--------------------------*/
    /* read in lift coefficient */
    /*--------------------------*/
    for (mach = 0; mach < numMach; mach++)
    {
        for (alpha = 0; alpha < numAlpha; alpha++)
        {
            aeroData->clift[mach * numAlpha + alpha] = (float)atof(inputFile->GetNext());
        }
    }

    /*---------------------------------*/
    /* Allocate memeory for CDRAG data */
    /*---------------------------------*/
#ifdef USE_SH_POOLS
    aeroData->cdrag = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * numMach * numAlpha, 0);
#else
    aeroData->cdrag = new float[numAlpha * numMach];
#endif

    /*-----------------------*/
    /* Read in drag modifier */
    /*-----------------------*/
    aeroData->cdFactor = (float)atof(inputFile->GetNext());

    /*--------------------------*/
    /* read in DRAG coefficient */
    /*--------------------------*/
    for (mach = 0; mach < numMach; mach++)
    {
        for (alpha = 0; alpha < numAlpha; alpha++)
        {
            aeroData->cdrag[mach * numAlpha + alpha] = (float)atof(inputFile->GetNext()) * 1.5F;
        }
    }

    /*------------------------------*/
    /* Allocate memeory for CY data */
    /*------------------------------*/
#ifdef USE_SH_POOLS
    aeroData->cy = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * numMach * numAlpha, 0);
#else
    aeroData->cy = new float[numAlpha * numMach];
#endif

    /*----------------------*/
    /* Read in yaw modifier */
    /*----------------------*/
    aeroData->cyFactor = (float)atof(inputFile->GetNext());

    /*------------------------*/
    /* read in cy coefficient */
    /*------------------------*/
    for (mach = 0; mach < numMach; mach++)
    {
        for (alpha = 0; alpha < numAlpha; alpha++)
        {
            aeroData->cy[mach * numAlpha + alpha] = (float)atof(inputFile->GetNext());
        }
    }

    return (aeroData);
}

// JPO structure to read in auxilliary variables
#define OFFSET(x) offsetof(AuxAeroData, x)
static const InputDataDesc AuxAeroDataDesc[] =
{
    { "normSpoolRate", InputDataDesc::ID_FLOAT, OFFSET(normSpoolRate), "0.7"},
    { "abSpoolRate", InputDataDesc::ID_FLOAT, OFFSET(abSpoolRate), "0.4"},
    { "jfsSpoolUpRate", InputDataDesc::ID_FLOAT, OFFSET(jfsSpoolUpRate), "15"},
    { "jfsSpoolUpLimit", InputDataDesc::ID_FLOAT, OFFSET(jfsSpoolUpLimit), "0.25"},
    { "lightupSpoolRate", InputDataDesc::ID_FLOAT, OFFSET(lightupSpoolRate), "10"},
    { "flameoutSpoolRate", InputDataDesc::ID_FLOAT, OFFSET(flameoutSpoolRate), "5"},
    { "jfsRechargeTime", InputDataDesc::ID_FLOAT, OFFSET(jfsRechargeTime), "60"},
    { "jfsMinRechargeRpm", InputDataDesc::ID_FLOAT, OFFSET(jfsMinRechargeRpm), "0.12"},
    { "jfsSpinTime", InputDataDesc::ID_FLOAT, OFFSET(jfsSpinTime), "240"}, //MI
    { "mainGenRpm", InputDataDesc::ID_FLOAT, OFFSET(mainGenRpm), "0.63"},
    { "stbyGenRpm", InputDataDesc::ID_FLOAT, OFFSET(stbyGenRpm), "0.6"},
    { "epuBurnTime", InputDataDesc::ID_FLOAT, OFFSET(epuBurnTime), "600"},
    { "DeepStallEngineStall", InputDataDesc::ID_INT, OFFSET(DeepStallEngineStall), "0"}, //Need bool
    { "engineDamageStopThreshold", InputDataDesc::ID_INT, OFFSET(engineDamageStopThreshold), "13"},  // 2002-04-11 ADDED BY S.G. Externalized the 13% chance of engine stop
    { "engineDamageNoRestartThreshold", InputDataDesc::ID_INT, OFFSET(engineDamageNoRestartThreshold), "50"}, // 2002-04-11 ADDED BY S.G. Externalized the 50% chance of engine not able to restart
    { "engineDamageHitThreshold", InputDataDesc::ID_INT, OFFSET(engineDamageHitThreshold), "0"}, // 2002-04-11 ADDED BY S.G. Externalized the hitChance > 0 chance of engine shutting down

    // airframe related
    { "rollMomentum", InputDataDesc::ID_FLOAT, OFFSET(rollMomentum), "1"},
    { "pitchMomentum", InputDataDesc::ID_FLOAT, OFFSET(pitchMomentum), "1"},
    { "yawMomentum", InputDataDesc::ID_FLOAT, OFFSET(yawMomentum), "1"},
    { "pitchElasticity", InputDataDesc::ID_FLOAT, OFFSET(pitchElasticity), "1"},
    { "gearPitchFactor", InputDataDesc::ID_FLOAT, OFFSET(gearPitchFactor), "0.1"},
    { "rollGearGain", InputDataDesc::ID_FLOAT, OFFSET(rollGearGain), "0.6"},
    { "yawGearGain", InputDataDesc::ID_FLOAT, OFFSET(yawGearGain), "0.6"},
    { "pitchGearGain", InputDataDesc::ID_FLOAT, OFFSET(pitchGearGain), "0.8"},
    { "sinkRate", InputDataDesc::ID_FLOAT, OFFSET(sinkRate), "15"},
    { "tefMaxAngle", InputDataDesc::ID_FLOAT, OFFSET(tefMaxAngle), "20"}, // divide angle of TEF by this to get amount of influence on CL and CD
    { "hasFlapperons", InputDataDesc::ID_INT, OFFSET(hasFlapperons), "1"}, // has flapperons are opposed to ailerons bitand flaps
    { "hasTef", InputDataDesc::ID_INT, OFFSET(hasTef), "2"}, // has TEF 0 - no, 1 manual, 2 aoa
    { "CLtefFactor", InputDataDesc::ID_FLOAT, OFFSET(CLtefFactor), "0.05"}, // how much the TEF affect the CL
    { "CDtefFactor", InputDataDesc::ID_FLOAT, OFFSET(CDtefFactor), "0.05"}, // how much the TEF affect the CD
    { "tefNStages", InputDataDesc::ID_INT, OFFSET(tefNStages), "2"}, // number of stages of TEF
    { "tefTakeoff", InputDataDesc::ID_FLOAT, OFFSET(tefTakeOff), "0"}, // Tef angle for takeoff
    { "tefRate", InputDataDesc::ID_FLOAT, OFFSET(tefRate), "5"}, // Tef rate of change
    { "lefRate", InputDataDesc::ID_FLOAT, OFFSET(lefRate), "5"}, // Lef rate of change
    { "hasLef", InputDataDesc::ID_INT, OFFSET(hasLef), "2"}, // has LEF
    { "lefGround", InputDataDesc::ID_FLOAT, OFFSET(lefGround), "-2"}, // lef position on ground
    { "lefMaxAngle", InputDataDesc::ID_FLOAT, OFFSET(lefMaxAngle), "20"}, // max angle of LEF by this to get amount of influence on CL and CD
    { "lefMaxMach", InputDataDesc::ID_FLOAT, OFFSET(lefMaxMach), "0.4"}, // LEF stops at this
    { "lefNStages", InputDataDesc::ID_INT, OFFSET(lefNStages), "1"}, // number of stages of LEF
    { "CDlefFactor", InputDataDesc::ID_FLOAT, OFFSET(CDlefFactor), "0.0"}, // how much the LEF affect the CD
    { "CDSPDBFactor", InputDataDesc::ID_FLOAT, OFFSET(CDSPDBFactor), "0.08"}, // how much the speed brake affect drag
    { "CDLDGFactor", InputDataDesc::ID_FLOAT, OFFSET(CDLDGFactor), "0.06"}, // how much the landing gear affects drag
    { "flapGearRelative", InputDataDesc::ID_INT, OFFSET(flapGearRelative), "1"}, //flaps only work with gear (or tef extend)
    { "maxFlapVcas", InputDataDesc::ID_FLOAT, OFFSET(maxFlapVcas), "370"}, // falps fully retract at
    { "flapVcasRange", InputDataDesc::ID_FLOAT, OFFSET(flapVcasRange), "130"}, // what range of Vcas flaps work over
    { "area2Span", InputDataDesc::ID_FLOAT, OFFSET(area2Span), "0.1066"}, // used to convert life area into span
    { "rudderMaxAngle", InputDataDesc::ID_FLOAT, OFFSET(rudderMaxAngle), "20"}, // max angle of rudder
    { "aileronMaxAngle", InputDataDesc::ID_FLOAT, OFFSET(aileronMaxAngle), "20"}, // max angle of aileron
    { "elevonMaxAngle", InputDataDesc::ID_FLOAT, OFFSET(elevonMaxAngle), "20"}, // max angle of elevon
    { "airbrakeMaxAngle", InputDataDesc::ID_FLOAT, OFFSET(airbrakeMaxAngle), "60"}, // max angle of airbrake
    { "hasSwingWing", InputDataDesc::ID_INT, OFFSET(hasSwingWing), "0"}, // has swing wing capability
    { "isComplex", InputDataDesc::ID_INT, OFFSET(isComplex), "0"}, // has complex model
    { "landingAOA", InputDataDesc::ID_FLOAT, OFFSET(landingAOA), "12.5"},
    { "dragChuteCd", InputDataDesc::ID_FLOAT, OFFSET(dragChuteCd), "0"},
    { "dragChuteMaxSpeed", InputDataDesc::ID_FLOAT, OFFSET(dragChuteMaxSpeed), "170"},
    { "rollCouple", InputDataDesc::ID_FLOAT, OFFSET(rollCouple), "0"},
    { "elevatorRoll", InputDataDesc::ID_INT, OFFSET(elevatorRolls), "1"},
    { "canopyMaxAngle", InputDataDesc::ID_FLOAT, OFFSET(canopyMaxAngle), "20"},
    { "canopyRate", InputDataDesc::ID_FLOAT, OFFSET(canopyRate), "5"},

    // fuel related
    { "fuelFlowFactorNormal", InputDataDesc::ID_FLOAT, OFFSET(fuelFlowFactorNormal), "0.25"},
    { "fuelFlowFactorAb", InputDataDesc::ID_FLOAT, OFFSET(fuelFlowFactorAb), "0.65"},
    { "minFuelFlow", InputDataDesc::ID_FLOAT, OFFSET(minFuelFlow), "1200"},
    { "fuelFwdRes", InputDataDesc::ID_FLOAT, OFFSET(fuelFwdRes), "0"},
    { "fuelAftRes", InputDataDesc::ID_FLOAT, OFFSET(fuelAftRes), "0"},
    { "fuelFwd1", InputDataDesc::ID_FLOAT, OFFSET(fuelFwd1), "0"},
    { "fuelAft1", InputDataDesc::ID_FLOAT, OFFSET(fuelAft1), "0"},
    { "fuelWingAl", InputDataDesc::ID_FLOAT, OFFSET(fuelWingAl), "0"},
    { "fuelWingFr", InputDataDesc::ID_FLOAT, OFFSET(fuelWingFr), "0"},
    { "fuelFwdResRate", InputDataDesc::ID_FLOAT, OFFSET(fuelFwdResRate), "0"},
    { "fuelAftResRate", InputDataDesc::ID_FLOAT, OFFSET(fuelAftResRate), "0"},
    { "fuelFwd1Rate", InputDataDesc::ID_FLOAT, OFFSET(fuelFwd1Rate), "0"},
    { "fuelAft1Rate", InputDataDesc::ID_FLOAT, OFFSET(fuelAft1Rate), "0"},
    { "fuelWingAlRate", InputDataDesc::ID_FLOAT, OFFSET(fuelWingAlRate), "0"},
    { "fuelWingFrRate", InputDataDesc::ID_FLOAT, OFFSET(fuelWingFrRate), "0"},
    { "fuelClineRate", InputDataDesc::ID_FLOAT, OFFSET(fuelClineRate), "0"},
    { "fuelWingExtRate", InputDataDesc::ID_FLOAT, OFFSET(fuelWingExtRate), "0"},
    { "fuelMinFwd", InputDataDesc::ID_FLOAT, OFFSET(fuelMinFwd), "400"},
    { "fuelMinAft", InputDataDesc::ID_FLOAT, OFFSET(fuelMinAft), "250"},
    { "nChaff", InputDataDesc::ID_INT, OFFSET(nChaff), "60"},
    { "nFlare", InputDataDesc::ID_INT, OFFSET(nFlare), "30"},
    { "gunLocation", InputDataDesc::ID_VECTOR, OFFSET(gunLocation), "2 -2 0"},
    { "gunTrailID", InputDataDesc::ID_INT, OFFSET(gunTrailID),   "11"},
    { "nEngines", InputDataDesc::ID_INT, OFFSET(nEngines), "1"},
    { "engine1Location", InputDataDesc::ID_VECTOR, OFFSET(engineLocation[0]), "0 0 0"},
    { "engine2Location", InputDataDesc::ID_VECTOR, OFFSET(engineLocation[1]), "0 0 0"},
    { "engine3Location", InputDataDesc::ID_VECTOR, OFFSET(engineLocation[2]), "0 0 0"},
    { "engine4Location", InputDataDesc::ID_VECTOR, OFFSET(engineLocation[3]), "0 0 0"},
    { "engine5Location", InputDataDesc::ID_VECTOR, OFFSET(engineLocation[4]), "0 0 0"},
    { "engine6Location", InputDataDesc::ID_VECTOR, OFFSET(engineLocation[5]), "0 0 0"},
    { "engine7Location", InputDataDesc::ID_VECTOR, OFFSET(engineLocation[6]), "0 0 0"},
    { "engine8Location", InputDataDesc::ID_VECTOR, OFFSET(engineLocation[7]), "0 0 0"},
    { "engineSmokes", InputDataDesc::ID_INT, OFFSET(engineSmokes), "0"},
    { "wingTipLocation", InputDataDesc::ID_VECTOR, OFFSET(wingTipLocation), "0 0 0"},
    { "vortex1Location", InputDataDesc::ID_VECTOR, OFFSET(vortex1Location), "0 0 0"},
    { "vortex2Location", InputDataDesc::ID_VECTOR, OFFSET(vortex2Location), "0 0 0"},
    { "vortexPS1Location", InputDataDesc::ID_VECTOR, OFFSET(vortexPS1Location), "0 0 0"},
    { "vortexPS2Location", InputDataDesc::ID_VECTOR, OFFSET(vortexPS2Location), "0 0 0"},
    { "vortexPS3Location", InputDataDesc::ID_VECTOR, OFFSET(vortexPS3Location), "0 0 0"},
    { "largeVortex", InputDataDesc::ID_INT, OFFSET(largeVortex), "0"},
    { "vortexAOALimit", InputDataDesc::ID_VECTOR, OFFSET(vortexAOALimit), "29.5"},
    { "refuelLocation", InputDataDesc::ID_VECTOR, OFFSET(refuelLocation), "0 0 0"},
    { "hardpoint1Grp", InputDataDesc::ID_INT, OFFSET(hardpointrg[0]), "-1"},
    { "hardpoint2Grp", InputDataDesc::ID_INT, OFFSET(hardpointrg[1]), "-1"},
    { "hardpoint3Grp", InputDataDesc::ID_INT, OFFSET(hardpointrg[2]), "-1"},
    { "hardpoint4Grp", InputDataDesc::ID_INT, OFFSET(hardpointrg[3]), "-1"},
    { "hardpoint5Grp", InputDataDesc::ID_INT, OFFSET(hardpointrg[4]), "-1"},
    { "hardpoint6Grp", InputDataDesc::ID_INT, OFFSET(hardpointrg[5]), "-1"},
    { "hardpoint7Grp", InputDataDesc::ID_INT, OFFSET(hardpointrg[6]), "-1"},
    { "hardpoint8Grp", InputDataDesc::ID_INT, OFFSET(hardpointrg[7]), "-1"},
    { "hardpoint9Grp", InputDataDesc::ID_INT, OFFSET(hardpointrg[8]), "-1"},
    { "hardpoint10Grp", InputDataDesc::ID_INT, OFFSET(hardpointrg[9]), "-1"},
    { "hardpoint11Grp", InputDataDesc::ID_INT, OFFSET(hardpointrg[10]), "-1"},
    { "hardpoint12Grp", InputDataDesc::ID_INT, OFFSET(hardpointrg[11]), "-1"},
    { "hardpoint13Grp", InputDataDesc::ID_INT, OFFSET(hardpointrg[12]), "-1"},
    { "hardpoint14Grp", InputDataDesc::ID_INT, OFFSET(hardpointrg[13]), "-1"},
    { "hardpoint15Grp", InputDataDesc::ID_INT, OFFSET(hardpointrg[14]), "-1"},
    { "hardpoint16Grp", InputDataDesc::ID_INT, OFFSET(hardpointrg[15]), "-1"},

    // RV - Biker - Add HP-DOF stuff here
    { "hp1SwitchDofRate", InputDataDesc::ID_VECTOR, OFFSET(hpSwitchDofRate[0]), "-1 -1 0"},
    { "hp2SwitchDofRate", InputDataDesc::ID_VECTOR, OFFSET(hpSwitchDofRate[1]), "-1 -1 0"},
    { "hp3SwitchDofRate", InputDataDesc::ID_VECTOR, OFFSET(hpSwitchDofRate[2]), "-1 -1 0"},
    { "hp4SwitchDofRate", InputDataDesc::ID_VECTOR, OFFSET(hpSwitchDofRate[3]), "-1 -1 0"},
    { "hp5SwitchDofRate", InputDataDesc::ID_VECTOR, OFFSET(hpSwitchDofRate[4]), "-1 -1 0"},
    { "hp6SwitchDofRate", InputDataDesc::ID_VECTOR, OFFSET(hpSwitchDofRate[5]), "-1 -1 0"},
    { "hp7SwitchDofRate", InputDataDesc::ID_VECTOR, OFFSET(hpSwitchDofRate[6]), "-1 -1 0"},
    { "hp8SwitchDofRate", InputDataDesc::ID_VECTOR, OFFSET(hpSwitchDofRate[7]), "-1 -1 0"},
    { "hp9SwitchDofRate", InputDataDesc::ID_VECTOR, OFFSET(hpSwitchDofRate[8]), "-1 -1 0"},
    { "hp10SwitchDofRate", InputDataDesc::ID_VECTOR, OFFSET(hpSwitchDofRate[9]), "-1 -1 0"},
    { "hp11SwitchDofRate", InputDataDesc::ID_VECTOR, OFFSET(hpSwitchDofRate[10]), "-1 -1 0"},
    { "hp12SwitchDofRate", InputDataDesc::ID_VECTOR, OFFSET(hpSwitchDofRate[11]), "-1 -1 0"},
    { "hp13SwitchDofRate", InputDataDesc::ID_VECTOR, OFFSET(hpSwitchDofRate[12]), "-1 -1 0"},
    { "hp14SwitchDofRate", InputDataDesc::ID_VECTOR, OFFSET(hpSwitchDofRate[13]), "-1 -1 0"},
    { "hp15SwitchDofRate", InputDataDesc::ID_VECTOR, OFFSET(hpSwitchDofRate[14]), "-1 -1 0"},
    { "hp16SwitchDofRate", InputDataDesc::ID_VECTOR, OFFSET(hpSwitchDofRate[15]), "-1 -1 0"},

    { "gunSwitchDofRate", InputDataDesc::ID_LOOKUPTABLE, OFFSET(gunSwitchDofRate), "-1 -1 0"},

    // sound stuff
    { "sndExt", InputDataDesc::ID_INT, OFFSET(sndExt), "35"},
    { "sndWind", InputDataDesc::ID_INT, OFFSET(sndWind), "33"},
    { "sndAbInt", InputDataDesc::ID_INT, OFFSET(sndAbInt), "40"},
    { "sndAbExt", InputDataDesc::ID_INT, OFFSET(sndAbExt), "39"},
    { "sndEject", InputDataDesc::ID_INT, OFFSET(sndEject), "5"},
    { "sndSpdBrakeStart", InputDataDesc::ID_INT, OFFSET(sndSpdBrakeStart), "141"},
    { "sndSpdBrakeLoop", InputDataDesc::ID_INT, OFFSET(sndSpdBrakeLoop), "140"},
    { "sndSpdBrakeEnd", InputDataDesc::ID_INT, OFFSET(sndSpdBrakeEnd), "139"},
    { "sndSpdBrakeWind", InputDataDesc::ID_INT, OFFSET(sndSpdBrakeWind), "142"},
    { "sndOverSpeed1", InputDataDesc::ID_INT, OFFSET(sndOverSpeed1), "191"},
    { "sndOverSpeed2", InputDataDesc::ID_INT, OFFSET(sndOverSpeed2), "192"},
    { "sndGunStart", InputDataDesc::ID_INT, OFFSET(sndGunStart), "25"},
    { "sndGunLoop", InputDataDesc::ID_INT, OFFSET(sndGunLoop), "26"},
    { "sndGunEnd", InputDataDesc::ID_INT, OFFSET(sndGunEnd), "27"},
    { "sndBBPullup", InputDataDesc::ID_INT, OFFSET(sndBBPullup), "68"},
    { "sndBBBingo", InputDataDesc::ID_INT, OFFSET(sndBBBingo), "66"},
    { "sndBBWarning", InputDataDesc::ID_INT, OFFSET(sndBBWarning), "63"},
    { "sndBBCaution", InputDataDesc::ID_INT, OFFSET(sndBBCaution), "64"},
    { "sndBBChaffFlareLow", InputDataDesc::ID_INT, OFFSET(sndBBChaffFlareLow), "184"},
    { "sndBBFlare", InputDataDesc::ID_INT, OFFSET(sndBBFlare), "138"},
    { "sndBBChaffFlare", InputDataDesc::ID_INT, OFFSET(sndBBChaffFlare), "183"},
    { "sndBBChaffFlareOut", InputDataDesc::ID_INT, OFFSET(sndBBChaffFlareOut), "185"},
    { "sndBBAltitude", InputDataDesc::ID_INT, OFFSET(sndBBAltitude), "65"},
    { "sndBBLock", InputDataDesc::ID_INT, OFFSET(sndBBLock), "67"},
    { "sndTouchDown", InputDataDesc::ID_INT, OFFSET(sndTouchDown), "42"},
    { "sndWheelBrakes", InputDataDesc::ID_INT, OFFSET(sndWheelBrakes), "132"},
    { "sndDragChute", InputDataDesc::ID_INT, OFFSET(sndDragChute), "217"},
    { "sndLowSpeed", InputDataDesc::ID_INT, OFFSET(sndLowSpeed), "167"},
    { "sndFlapStart", InputDataDesc::ID_INT, OFFSET(sndFlapStart), "145"},
    { "sndFlapLoop", InputDataDesc::ID_INT, OFFSET(sndFlapLoop), "144"},
    { "sndFlapEnd", InputDataDesc::ID_INT, OFFSET(sndFlapEnd), "143"},
    { "sndHookStart", InputDataDesc::ID_INT, OFFSET(sndHookStart), "195"},
    { "sndHookLoop", InputDataDesc::ID_INT, OFFSET(sndHookLoop), "194"},
    { "sndHookEnd", InputDataDesc::ID_INT, OFFSET(sndHookEnd), "193"},
    { "sndGearCloseStart", InputDataDesc::ID_INT, OFFSET(sndGearCloseStart), "147"},
    { "sndGearCloseEnd", InputDataDesc::ID_INT, OFFSET(sndGearCloseEnd), "146"},
    { "sndGearOpenStart", InputDataDesc::ID_INT, OFFSET(sndGearOpenStart), "149"},
    { "sndGearOpenEnd", InputDataDesc::ID_INT, OFFSET(sndGearOpenEnd), "150"},
    { "sndGearLoop", InputDataDesc::ID_INT, OFFSET(sndGearLoop), "148"},
    // MLR 2/15/2004 -
    { "sndCanopyOpenStart", InputDataDesc::ID_INT, OFFSET(sndCanopyOpenStart), "271"},
    { "sndCanopyOpenEnd", InputDataDesc::ID_INT, OFFSET(sndCanopyOpenEnd), "272"},
    { "sndCanopyCloseStart", InputDataDesc::ID_INT, OFFSET(sndCanopyCloseStart), "273"},
    { "sndCanopyCloseEnd", InputDataDesc::ID_INT, OFFSET(sndCanopyCloseEnd), "274"},
    { "sndCanopyLoop", InputDataDesc::ID_INT, OFFSET(sndCanopyLoop), "275"},

    { "rollLimitForAiInWP", InputDataDesc::ID_FLOAT, OFFSET(rollLimitForAiInWP), "180.0"}, // 2002-01-31 ADDED BY S.G. AI limition on roll when in waypoint (or similar) mode
    { "flap2Nozzle", InputDataDesc::ID_INT, OFFSET(flap2Nozzle), "0"},
    { "startGroundAvoidCheck", InputDataDesc::ID_FLOAT, OFFSET(startGroundAvoidCheck), "3000.0"}, // 2002-04-17 MN only do ground avoid check when we are closer than this distance to the ground
    { "limitPstick", InputDataDesc::ID_INT, OFFSET(limitPstick), "1"}, //Cobra - changed default to 1 // 2002-04-17 MN method of pullup; 1 = use old code (G force limited), 0 = use full pull (pStick = 1.0f)
    // 2002-02-08 MN refuelling speed, altitude, deceleration distance, follow rate, desiredClosureFactor and IL78Factor
    { "refuelSpeed", InputDataDesc::ID_FLOAT, OFFSET(refuelSpeed), "310.0"},
    { "refuelAltitude", InputDataDesc::ID_FLOAT, OFFSET(refuelAltitude), "22500.0"},
    { "decelDistance", InputDataDesc::ID_FLOAT, OFFSET(decelDistance), "1000.0"},
    { "followRate", InputDataDesc::ID_FLOAT, OFFSET(followRate), "20.0"}, // original was 10, 20 brings some better results
    { "desiredClosureFactor", InputDataDesc::ID_FLOAT, OFFSET(desiredClosureFactor), "0.25"}, // original was hardcoded to 0.25
    // { "IL78Factor", InputDataDesc::ID_FLOAT, OFFSET(IL78Factor), "0.0"}, // IL78 range to basket modifier (to fix "GivingGas" flagging)
    { "longLeg", InputDataDesc::ID_FLOAT, OFFSET(longLeg), "100.0"}, // 60 nm long leg of tanker track pattern
    { "shortLeg", InputDataDesc::ID_FLOAT, OFFSET(shortLeg), "25.0"}, // 25nm short leg of tanker track pattern
    { "refuelRate", InputDataDesc::ID_FLOAT, OFFSET(refuelRate), "50.0"}, // 50 lbs per second
    { "AIBoomDistance", InputDataDesc::ID_FLOAT, OFFSET(AIBoomDistance), "50.0"}, // distance to boom at which we will put the AI on the boom (hack...)

    // 18NOV03 - FRB - Tanker specific info - What type of service available (boom or drogue)
    { "nBooms", InputDataDesc::ID_INT, OFFSET(numBooms), "1"}, // 18NOV03 - FRB - Number of booms (never more than one) 0 = no boom sevice  >=1 = boom.
    { "BoomRFPos", InputDataDesc::ID_VECTOR, OFFSET(BoomRFPos), "0 0 0"}, // 12DEC03 - FRB - a/c 0,0,0 refueling position adjustments for different booms boom
    { "BoomStoredAngle", InputDataDesc::ID_FLOAT, OFFSET(BoomStoredAngle), "14"}, // 12DEC03 - FRB - Angle of boom in stored osition (deg. + = up)
    { "nDrogues", InputDataDesc::ID_INT, OFFSET(numDrogues), "0"}, // 18NOV03 - FRB - Number of drogues  0 = no drogue service  >0 = drogue service and number of stations.
    { "activeDrogue", InputDataDesc::ID_INT, OFFSET(activeDrogue), "1"}, // 18NOV03 - FRB - Number of drogue Slot used for refueling.
    { "DrogueExt", InputDataDesc::ID_FLOAT, OFFSET(DrogueExt), "40"}, // 29NOV03 - FRB - Drogue extension length (feet).
    { "DrogueRFPos", InputDataDesc::ID_VECTOR, OFFSET(DrogueRFPos), "-60 0 0"}, // 12DEC03 - FRB - a/c 0,0,0 refueling position relative to drogue pack (Slot)

    // RV - Biker - Drogue and boom parents now also in FMs (defaults are from KC-135)
    { "boomParent", InputDataDesc::ID_INT, OFFSET(boomParent), "2211"},
    { "drogueParent", InputDataDesc::ID_INT, OFFSET(drogueParent), "2221"},
    { "refuelBoomSlot0", InputDataDesc::ID_INT, OFFSET(refuelBoomSlot), "0"},
    { "refuelDrogueSlot0", InputDataDesc::ID_INT, OFFSET(refuelDrogueSlot[0]), "0"},
    { "refuelDrogueSlot1", InputDataDesc::ID_INT, OFFSET(refuelDrogueSlot[1]), "1"},
    { "refuelDrogueSlot2", InputDataDesc::ID_INT, OFFSET(refuelDrogueSlot[2]), "2"},
    { "tankerType", InputDataDesc::ID_INT, OFFSET(tankerType), "6"},


    // 2002-03-12 MN fuel handling stuff
    { "BingoReturnDistance", InputDataDesc::ID_FLOAT, OFFSET(BingoReturnDistance), "0.0"},
    { "jokerFactor", InputDataDesc::ID_FLOAT, OFFSET(jokerFactor), "2.0"},
    { "bingoFactor", InputDataDesc::ID_FLOAT, OFFSET(bingoFactor), "5.0"},
    { "fumesFactor", InputDataDesc::ID_FLOAT, OFFSET(fumesFactor), "15.0"},

    // 2002-02-23 MN max ripple count
    { "maxRippleCount", InputDataDesc::ID_INT, OFFSET(maxRippleCount), "19"}, // F-16's max ripple count
    { "largePark", InputDataDesc::ID_INT, OFFSET(largePark), "0"}, // requires large parking space

    // RV - I-Hawk - Does this plane has advanced HTS?
    { "HtsAble", InputDataDesc::ID_INT, OFFSET(HtsAble), "0"},

    //MI TFR
    { "Has_TFR", InputDataDesc::ID_INT, OFFSET(Has_TFR), "0"}, //does this plane have TFR?
    { "PID_K", InputDataDesc::ID_FLOAT, OFFSET(PID_K), "0.5"}, //Proportianal gain in TFR PID pitch controler.
    { "PID_KI", InputDataDesc::ID_FLOAT, OFFSET(PID_KI), "0.0"}, //Intergral gain in TFR PID pitch controler
    { "PID_KD", InputDataDesc::ID_FLOAT, OFFSET(PID_KD), "0.39"}, //Differential gain in TFR PID pitch controler
    { "TFR_LimitMX", InputDataDesc::ID_INT, OFFSET(TFR_LimitMX), "0"}, //Limit PID Integrator internal value so it doesn't get stuck in exteme.
    { "TFR_Corner", InputDataDesc::ID_FLOAT, OFFSET(TFR_Corner), "350.0"}, //Corner speed used in TFR calculations
    { "TFR_Gain", InputDataDesc::ID_FLOAT, OFFSET(TFR_Gain), "0.010"}, //Gain for calculating pitch error based on alt. difference
    { "EVA_Gain", InputDataDesc::ID_FLOAT, OFFSET(EVA_Gain), "5.0"}, //Pitch setpoint gain in EVA (evade) code
    { "TFR_MaxRoll", InputDataDesc::ID_FLOAT, OFFSET(TFR_MaxRoll), "20.0"}, //Do not pull the stick in TFR if roll exceeds this value
    { "TFR_SoftG", InputDataDesc::ID_FLOAT, OFFSET(TFR_SoftG), "2.0"}, //Max TFR G pull in soft mode
    { "TFR_MedG", InputDataDesc::ID_FLOAT, OFFSET(TFR_MedG), "4.0"}, //Max TFR G pull in medium mode
    { "TFR_HardG", InputDataDesc::ID_FLOAT, OFFSET(TFR_HardG), "6.0"}, //Max TFR G pull in hard mode
    { "TFR_Clearance", InputDataDesc::ID_FLOAT, OFFSET(TFR_Clearance), "20.0"}, //Minimum clearance above the top of any obstacle [ft]
    { "SlowPercent", InputDataDesc::ID_FLOAT, OFFSET(SlowPercent), "0.0"}, //Flash SLOW when airspeed is lower then this percentage of corner speed
    { "TFR_lookAhead", InputDataDesc::ID_FLOAT, OFFSET(TFR_lookAhead), "4000.0"}, //Distance from ground directly under a/c used to measure ground inclination [ft]
    { "EVA1_SoftFactor", InputDataDesc::ID_FLOAT, OFFSET(EVA1_SoftFactor), "0.6"}, //Turnradius multiplier to get safe distance from ground for FLY_UP in SLOW
    { "EVA2_SoftFactor", InputDataDesc::ID_FLOAT, OFFSET(EVA2_SoftFactor), "0.4"}, //Turnradius multiplier to get safe distance from ground for OBSTACLE in SLOW
    { "EVA1_MedFactor", InputDataDesc::ID_FLOAT, OFFSET(EVA1_MedFactor), "0.8"}, //Turnradius multiplier to get safe distance from ground for FLY_UP in MED
    { "EVA2_MedFactor", InputDataDesc::ID_FLOAT, OFFSET(EVA2_MedFactor), "0.6"}, //Turnradius multiplier to get safe distance from ground for OBSTACLE in MED
    { "EVA1_HardFactor", InputDataDesc::ID_FLOAT, OFFSET(EVA1_HardFactor), "1.2"}, //Turnradius multiplier to get safe distance from ground for FLY_UP in HARD
    { "EVA2_HardFactor", InputDataDesc::ID_FLOAT, OFFSET(EVA2_HardFactor), "0.9"}, //Turnradius multiplier to get safe distance from ground for FLY_UP in MED
    { "TFR_GammaCorrMult", InputDataDesc::ID_FLOAT, OFFSET(TFR_GammaCorrMult), "1.0"}, //Turnradius multiplier to get safe distance from ground for OBSTACLE in HARD
    { "LantirnCameraX", InputDataDesc::ID_FLOAT, OFFSET(LantirnCameraX), "10.0"}, //Position of the camera
    { "LantirnCameraY", InputDataDesc::ID_FLOAT, OFFSET(LantirnCameraY), "2.0"},
    { "LantirnCameraZ", InputDataDesc::ID_FLOAT, OFFSET(LantirnCameraZ), "5.0"},

    // Digi's related MAR calculation
    { "MinTGTMAR", InputDataDesc::ID_FLOAT, OFFSET(minTGTMAR), "0.0"}, // 2002-03-22 ADDED BY S.G. Min TGTMAR for this type of aicraft
    { "MaxMARIdedStart", InputDataDesc::ID_FLOAT, OFFSET(maxMARIdedStart), "0.0"}, // 2002-03-22 ADDED BY S.G. MinMAR for this type of aicraft when target is ID'ed and below 5K
    { "AddMARIded5k", InputDataDesc::ID_FLOAT, OFFSET(addMARIded5k), "0.0"}, // 2002-03-22 ADDED BY S.G. MinMAR for this type of aicraft when target is ID'ed and below 5K
    { "AddMARIded18k", InputDataDesc::ID_FLOAT, OFFSET(addMARIded18k), "0.0"}, // 2002-03-22 ADDED BY S.G. MinMAR for this type of aicraft when target is ID'ed and below 18K
    { "AddMARIded28k", InputDataDesc::ID_FLOAT, OFFSET(addMARIded28k), "0.0"}, // 2002-03-22 ADDED BY S.G. MinMAR for this type of aicraft when target is ID'ed and below 28K
    { "AddMARIdedSpike", InputDataDesc::ID_FLOAT, OFFSET(addMARIdedSpike), "0.0"}, // 2002-03-22 ADDED BY S.G. MinMAR for this type of aicraft when target is ID'ed and spiked

    // 2003-09-30 added by MLR to support Aircraft animations
    { "animEngineRPMMult",   InputDataDesc::ID_FLOAT, OFFSET(animEngineRPMMult), "1000.0"},

    { "animSpoiler1Max",     InputDataDesc::ID_FLOAT, OFFSET(animSpoiler1Max), "0.0"},
    { "animSpoiler1Rate",    InputDataDesc::ID_FLOAT, OFFSET(animSpoiler1Rate), "45.0"},
    { "animSpoiler1OffAtWingSweep", InputDataDesc::ID_FLOAT, OFFSET(animSpoiler1OffAtWingSweep), "70.0"},
    { "animSpoiler1AirBrake", InputDataDesc::ID_INT, OFFSET(animSpoiler1AirBrake), "1"},
    { "animSpoiler2Max",     InputDataDesc::ID_FLOAT, OFFSET(animSpoiler2Max), "0.0"},
    { "animSpoiler2Rate",    InputDataDesc::ID_FLOAT, OFFSET(animSpoiler2Rate), "20.0"},
    { "animSpoiler2OffAtWingSweep", InputDataDesc::ID_FLOAT, OFFSET(animSpoiler2OffAtWingSweep), "45.0"},
    { "animSpoiler2AirBrake", InputDataDesc::ID_INT, OFFSET(animSpoiler2AirBrake), "1"},

    { "animExhNozIdle",      InputDataDesc::ID_FLOAT, OFFSET(animExhNozIdle), "0.0"},
    { "animExhNozMil",       InputDataDesc::ID_FLOAT, OFFSET(animExhNozMil),  "10.0"},
    { "animExhNozAB",        InputDataDesc::ID_FLOAT, OFFSET(animExhNozAB),   "0.0"},
    { "animExhNozRate",      InputDataDesc::ID_FLOAT, OFFSET(animExhNozRate),  "5.0"},

    { "animStrobeOnTime",    InputDataDesc::ID_FLOAT, OFFSET(animStrobeOnTime),     "0.08"},
    { "animStrobeOffTime",   InputDataDesc::ID_FLOAT, OFFSET(animStrobeOffTime),     "2.0"},

    { "animWingFlashOnTime",    InputDataDesc::ID_FLOAT, OFFSET(animWingFlashOnTime),     "0.4"}, // New variable for dat files - MartinV
    { "animWingFlashOffTime",   InputDataDesc::ID_FLOAT, OFFSET(animWingFlashOffTime),     "0.5"}, // New variable for dat files - MartinV

    { "animHookAngle",         InputDataDesc::ID_FLOAT, OFFSET(animHookAngle),       "60.0"},
    { "animHookRate",          InputDataDesc::ID_FLOAT, OFFSET(animHookRate),        "10.0"},

    { "animThrRevAngle",         InputDataDesc::ID_FLOAT, OFFSET(animThrRevAngle),       "60.0"}, // Thrust Reversers - FRB
    { "animThrRevRate",          InputDataDesc::ID_FLOAT, OFFSET(animThrRevRate),        "10.0"}, // Thrust Reversers - FRB

    // RV - Biker - Catapult thrust multiplier
    { "catapultThrustMultiplier",          InputDataDesc::ID_FLOAT, OFFSET(catapultThrustMultiplier),        "4.0"},

    // RV - Biker - Max lasing alt
    { "maxLasingAlt",          InputDataDesc::ID_FLOAT, OFFSET(maxLasingAlt),        "25000.0"},

    // RV - Biker - Check if AC can use programmable EWS
    { "hasProgrammableEWS",         InputDataDesc::ID_INT, OFFSET(hasProgrammableEWS),        "1"},

    // RV - Biker - Data link capability level
    { "dataLinkCapLevel", InputDataDesc::ID_INT, OFFSET(dataLinkCapLevel), "1"},

    //{ "animWingFoldAngle",     InputDataDesc::ID_FLOAT, OFFSET(animWingFoldAngle),   "110.0"}, not implemented
    //{ "animWingFoldRate",      InputDataDesc::ID_FLOAT, OFFSET(animWingFoldRate),    "5.0"},

    { "animAileronRate",       InputDataDesc::ID_FLOAT, OFFSET(animAileronRate),     "45.0"},

    { "animSwingWingStages",          InputDataDesc::ID_INT, OFFSET(animSwingWingStages),     "2"},
    { "animSwingWingMach1",          InputDataDesc::ID_FLOAT, OFFSET(animSwingWingMach[0]),     "0.6"},
    { "animSwingWingMach2",          InputDataDesc::ID_FLOAT, OFFSET(animSwingWingMach[1]),     "0.8"},
    { "animSwingWingMach3",          InputDataDesc::ID_FLOAT, OFFSET(animSwingWingMach[2]),     "0.0"},
    { "animSwingWingMach4",          InputDataDesc::ID_FLOAT, OFFSET(animSwingWingMach[3]),     "0.0"},
    { "animSwingWingMach5",          InputDataDesc::ID_FLOAT, OFFSET(animSwingWingMach[4]),     "0.0"},
    { "animSwingWingMach6",          InputDataDesc::ID_FLOAT, OFFSET(animSwingWingMach[5]),     "0.0"},
    { "animSwingWingMach7",          InputDataDesc::ID_FLOAT, OFFSET(animSwingWingMach[6]),     "0.0"},
    { "animSwingWingMach8",          InputDataDesc::ID_FLOAT, OFFSET(animSwingWingMach[7]),     "0.0"},
    { "animSwingWingMach9",          InputDataDesc::ID_FLOAT, OFFSET(animSwingWingMach[8]),     "0.0"},
    { "animSwingWingMach10",          InputDataDesc::ID_FLOAT, OFFSET(animSwingWingMach[9]),     "0.0"},
    { "animSwingWingAngle1",          InputDataDesc::ID_FLOAT, OFFSET(animSwingWingAngle[0]),     "25.0"},
    { "animSwingWingAngle2",          InputDataDesc::ID_FLOAT, OFFSET(animSwingWingAngle[1]),     "50.0"},
    { "animSwingWingAngle3",          InputDataDesc::ID_FLOAT, OFFSET(animSwingWingAngle[2]),     "0.0"},
    { "animSwingWingAngle4",          InputDataDesc::ID_FLOAT, OFFSET(animSwingWingAngle[3]),     "0.0"},
    { "animSwingWingAngle5",          InputDataDesc::ID_FLOAT, OFFSET(animSwingWingAngle[4]),     "0.0"},
    { "animSwingWingAngle6",          InputDataDesc::ID_FLOAT, OFFSET(animSwingWingAngle[5]),     "0.0"},
    { "animSwingWingAngle7",          InputDataDesc::ID_FLOAT, OFFSET(animSwingWingAngle[6]),     "0.0"},
    { "animSwingWingAngle8",          InputDataDesc::ID_FLOAT, OFFSET(animSwingWingAngle[7]),     "0.0"},
    { "animSwingWingAngle9",          InputDataDesc::ID_FLOAT, OFFSET(animSwingWingAngle[8]),     "0.0"},
    { "animSwingWingAngle10",          InputDataDesc::ID_FLOAT, OFFSET(animSwingWingAngle[9]),     "0.0"},
    { "animSwingWingRate",            InputDataDesc::ID_FLOAT, OFFSET(animSwingWingRate),        "5.0"},

    { "animWheelRadius1",          InputDataDesc::ID_FLOAT, OFFSET(animWheelRadius[0]),     "0.0"},
    { "animWheelRadius2",          InputDataDesc::ID_FLOAT, OFFSET(animWheelRadius[1]),     "0.0"},
    { "animWheelRadius3",          InputDataDesc::ID_FLOAT, OFFSET(animWheelRadius[2]),     "0.0"},
    { "animWheelRadius4",          InputDataDesc::ID_FLOAT, OFFSET(animWheelRadius[3]),     "0.0"},
    { "animWheelRadius5",          InputDataDesc::ID_FLOAT, OFFSET(animWheelRadius[4]),     "0.0"},
    { "animWheelRadius6",          InputDataDesc::ID_FLOAT, OFFSET(animWheelRadius[5]),     "0.0"},
    { "animWheelRadius7",          InputDataDesc::ID_FLOAT, OFFSET(animWheelRadius[6]),     "0.0"},
    { "animWheelRadius8",          InputDataDesc::ID_FLOAT, OFFSET(animWheelRadius[7]),     "0.0"},

    { "animRefuelAngle",           InputDataDesc::ID_FLOAT, OFFSET(animRefuelAngle),     "1.0"},
    { "animRefuelRate",            InputDataDesc::ID_FLOAT, OFFSET(animRefuelRate),       ".5"},

    { "fuelGaugeMultiplier",            InputDataDesc::ID_FLOAT, OFFSET(fuelGaugeMultiplier),       "10"},

    { "animGearMaxComp1",          InputDataDesc::ID_FLOAT, OFFSET(animGearMaxComp[0]),     "0.0"},
    { "animGearMaxComp2",          InputDataDesc::ID_FLOAT, OFFSET(animGearMaxComp[1]),     "0.0"},
    { "animGearMaxComp3",          InputDataDesc::ID_FLOAT, OFFSET(animGearMaxComp[2]),     "0.0"},
    { "animGearMaxComp4",          InputDataDesc::ID_FLOAT, OFFSET(animGearMaxComp[3]),     "0.0"},
    { "animGearMaxComp5",          InputDataDesc::ID_FLOAT, OFFSET(animGearMaxComp[4]),     "0.0"},
    { "animGearMaxComp6",          InputDataDesc::ID_FLOAT, OFFSET(animGearMaxComp[5]),     "0.0"},
    { "animGearMaxComp7",          InputDataDesc::ID_FLOAT, OFFSET(animGearMaxComp[6]),     "0.0"},
    { "animGearMaxComp8",          InputDataDesc::ID_FLOAT, OFFSET(animGearMaxComp[7]),     "0.0"},

    { "animGearMaxExt1",          InputDataDesc::ID_FLOAT, OFFSET(animGearMaxExt[0]),     "0.0"},
    { "animGearMaxExt2",          InputDataDesc::ID_FLOAT, OFFSET(animGearMaxExt[1]),     "0.0"},
    { "animGearMaxExt3",          InputDataDesc::ID_FLOAT, OFFSET(animGearMaxExt[2]),     "0.0"},
    { "animGearMaxExt4",          InputDataDesc::ID_FLOAT, OFFSET(animGearMaxExt[3]),     "0.0"},
    { "animGearMaxExt5",          InputDataDesc::ID_FLOAT, OFFSET(animGearMaxExt[4]),     "0.0"},
    { "animGearMaxExt6",          InputDataDesc::ID_FLOAT, OFFSET(animGearMaxExt[5]),     "0.0"},
    { "animGearMaxExt7",          InputDataDesc::ID_FLOAT, OFFSET(animGearMaxExt[6]),     "0.0"},
    { "animGearMaxExt8",          InputDataDesc::ID_FLOAT, OFFSET(animGearMaxExt[7]),     "0.0"},

    { "sndExternalVol",           InputDataDesc::ID_FLOAT, OFFSET(sndExternalVol),     "-2000.0"},
    { "sndInt",                   InputDataDesc::ID_INT, OFFSET(sndInt),             "-1"}, // MLR 2003-11-10 internal engine sound
    // MLR 1/8/2004 -
    { "sndIntChart", InputDataDesc::ID_LOOKUPTABLE, OFFSET(sndIntChart),        "1  0 1"},
    { "sndAbIntChart", InputDataDesc::ID_LOOKUPTABLE, OFFSET(sndAbIntChart),      "2  0 0  1 1"},
    { "sndInt2Chart", InputDataDesc::ID_LOOKUPTABLE, OFFSET(sndInt2Chart),       "3  0 0  1 1  1.035 1"},
    { "sndExtChart", InputDataDesc::ID_LOOKUPTABLE, OFFSET(sndExtChart),        "1  0 1"},
    { "sndAbExtChart", InputDataDesc::ID_LOOKUPTABLE, OFFSET(sndAbExtChart),      "2  0 0  1 1"},
    { "sndExt2Chart", InputDataDesc::ID_LOOKUPTABLE, OFFSET(sndExt2Chart),       "3  0 0  1 1  1.035 1"},

    { "sndIntPitchChart", InputDataDesc::ID_LOOKUPTABLE, OFFSET(sndIntPitchChart),        "2 0 0 1.03 1.47"},
    { "sndAbIntPitchChart", InputDataDesc::ID_LOOKUPTABLE, OFFSET(sndAbIntPitchChart),      "2 0 0 1.03 1.47"},
    { "sndInt2PitchChart", InputDataDesc::ID_LOOKUPTABLE, OFFSET(sndInt2PitchChart),       "2 0 0 1.03 1.47"},
    { "sndExtPitchChart", InputDataDesc::ID_LOOKUPTABLE, OFFSET(sndExtPitchChart),        "2 0 0 1.03 1.47"},
    { "sndAbExtPitchChart", InputDataDesc::ID_LOOKUPTABLE, OFFSET(sndAbExtPitchChart),      "2 0 0 1.03 1.47"},
    { "sndExt2PitchChart", InputDataDesc::ID_LOOKUPTABLE, OFFSET(sndExt2PitchChart),       "2 0 0 1.03 1.47"},



    { "sndInt2", InputDataDesc::ID_INT, OFFSET(sndInt2),                "0"},
    { "sndExt2", InputDataDesc::ID_INT, OFFSET(sndExt2),                "0"},
    // MLR 1/8/2004 - aerodynamic sounds
    { "sndAero1",               InputDataDesc::ID_INT, OFFSET(sndAero[0]),         "0"},
    { "sndAero2",               InputDataDesc::ID_INT, OFFSET(sndAero[1]),         "0"},
    { "sndAero3",               InputDataDesc::ID_INT, OFFSET(sndAero[2]),         "0"},
    { "sndAero4",               InputDataDesc::ID_INT, OFFSET(sndAero[3]),         "0"},

    { "sndAero1AOAChart",     InputDataDesc::ID_LOOKUPTABLE, OFFSET(sndAeroAOAChart[0]),     "1 0 0"},
    { "sndAero2AOAChart",     InputDataDesc::ID_LOOKUPTABLE, OFFSET(sndAeroAOAChart[1]),     "1 0 0"},
    { "sndAero3AOAChart",     InputDataDesc::ID_LOOKUPTABLE, OFFSET(sndAeroAOAChart[2]),     "1 0 0"},
    { "sndAero4AOAChart",     InputDataDesc::ID_LOOKUPTABLE, OFFSET(sndAeroAOAChart[3]),     "1 0 0"},

    { "sndAero1SpeedChart",     InputDataDesc::ID_LOOKUPTABLE, OFFSET(sndAeroSpeedChart[0]),   "1 0 0"},
    { "sndAero2SpeedChart",     InputDataDesc::ID_LOOKUPTABLE, OFFSET(sndAeroSpeedChart[1]),   "1 0 0"},
    { "sndAero3SpeedChart",     InputDataDesc::ID_LOOKUPTABLE, OFFSET(sndAeroSpeedChart[2]),   "1 0 0"},
    { "sndAero4SpeedChart",     InputDataDesc::ID_LOOKUPTABLE, OFFSET(sndAeroSpeedChart[3]),   "1 0 0"},

    // MLR 2003-11/16 Flare dispenser locations, directions, and qtys
    // note qtys only reflect what could be carried and do not reflect what the aircraft has
    { "FlarePos1",                InputDataDesc::ID_VECTOR, OFFSET(Flare.Pos[0]),             "0 0 0"},
    { "FlareVec1",                InputDataDesc::ID_VECTOR, OFFSET(Flare.Vec[0]),             "0 0 200"},
    { "FlareCount1",              InputDataDesc::ID_INT,    OFFSET(Flare.Decoys[0]),             "30"},
    { "FlarePos2",                InputDataDesc::ID_VECTOR, OFFSET(Flare.Pos[1]),             "0 0 0"},
    { "FlareVec2",                InputDataDesc::ID_VECTOR, OFFSET(Flare.Vec[1]),             "0 0 200"},
    { "FlareCount2",              InputDataDesc::ID_INT,    OFFSET(Flare.Decoys[1]),             "30"},
    { "FlarePos3",                InputDataDesc::ID_VECTOR, OFFSET(Flare.Pos[2]),             "0 0 0"},
    { "FlareVec3",                InputDataDesc::ID_VECTOR, OFFSET(Flare.Vec[2]),             "0 0 200"},
    { "FlareCount3",              InputDataDesc::ID_INT,    OFFSET(Flare.Decoys[2]),             "30"},
    { "FlarePos4",                InputDataDesc::ID_VECTOR, OFFSET(Flare.Pos[3]),             "0 0 0"},
    { "FlareVec4",                InputDataDesc::ID_VECTOR, OFFSET(Flare.Vec[3]),             "0 0,200"},
    { "FlareCount4",              InputDataDesc::ID_INT,    OFFSET(Flare.Decoys[3]),             "30"},
    { "FlarePos5",                InputDataDesc::ID_VECTOR, OFFSET(Flare.Pos[4]),             "0 0 0"},
    { "FlareVec5",                InputDataDesc::ID_VECTOR, OFFSET(Flare.Vec[4]),             "0 0 200"},
    { "FlareCount5",              InputDataDesc::ID_INT,    OFFSET(Flare.Decoys[4]),             "30"},
    { "FlarePos6",                InputDataDesc::ID_VECTOR, OFFSET(Flare.Pos[5]),             "0 0 0"},
    { "FlareVec6",                InputDataDesc::ID_VECTOR, OFFSET(Flare.Vec[5]),             "0 0 200"},
    { "FlareCount6",              InputDataDesc::ID_INT,    OFFSET(Flare.Decoys[5]),             "30"},
    { "FlarePos7",                InputDataDesc::ID_VECTOR, OFFSET(Flare.Pos[6]),             "0 0 0"},
    { "FlareVec7",                InputDataDesc::ID_VECTOR, OFFSET(Flare.Vec[6]),             "0 0 200"},
    { "FlareCount7",              InputDataDesc::ID_INT,    OFFSET(Flare.Decoys[6]),             "30"},
    { "FlarePos8",                InputDataDesc::ID_VECTOR, OFFSET(Flare.Pos[7]),             "0 0 0"},
    { "FlareVec8",                InputDataDesc::ID_VECTOR, OFFSET(Flare.Vec[7]),             "0 0 200"},
    { "FlareCount8",              InputDataDesc::ID_INT,    OFFSET(Flare.Decoys[7]),             "30"},
    { "FlarePos9",                InputDataDesc::ID_VECTOR, OFFSET(Flare.Pos[8]),             "0 0 0"},
    { "FlareVec9",                InputDataDesc::ID_VECTOR, OFFSET(Flare.Vec[8]),             "0 0 200"},
    { "FlareCount9",              InputDataDesc::ID_INT,    OFFSET(Flare.Decoys[8]),             "30"},
    { "FlarePos10",                InputDataDesc::ID_VECTOR, OFFSET(Flare.Pos[9]),             "0 0 0"},
    { "FlareVec10",                InputDataDesc::ID_VECTOR, OFFSET(Flare.Vec[9]),             "0 0 200"},
    { "FlareCount10",              InputDataDesc::ID_INT,    OFFSET(Flare.Decoys[9]),             "30"},
    { "FlareSeq",                 InputDataDesc::ID_INT, OFFSET(Flare.Sequence),             "0"},
    { "FlareDispensers",          InputDataDesc::ID_INT, OFFSET(Flare.Count),                "1"},

    // MLR 2003-11/16 Chaff dispenser locations, directions, and qtys
    // note qtys only reflect what could be carried and do not reflect what the aircraft has
    { "ChaffPos1",                InputDataDesc::ID_VECTOR, OFFSET(Chaff.Pos[0]),             "0 0 0"},
    { "ChaffVec1",                InputDataDesc::ID_VECTOR, OFFSET(Chaff.Vec[0]),             "0 0 200"},
    { "ChaffCount1",              InputDataDesc::ID_INT,    OFFSET(Chaff.Decoys[0]),             "30"},
    { "ChaffPos2",                InputDataDesc::ID_VECTOR, OFFSET(Chaff.Pos[1]),             "0 0 0"},
    { "ChaffVec2",                InputDataDesc::ID_VECTOR, OFFSET(Chaff.Vec[1]),             "0 0 200"},
    { "ChaffCount2",              InputDataDesc::ID_INT,    OFFSET(Chaff.Decoys[1]),             "30"},
    { "ChaffPos3",                InputDataDesc::ID_VECTOR, OFFSET(Chaff.Pos[2]),             "0 0 0"},
    { "ChaffVec3",                InputDataDesc::ID_VECTOR, OFFSET(Chaff.Vec[2]),             "0 0 200"},
    { "ChaffCount3",              InputDataDesc::ID_INT,    OFFSET(Chaff.Decoys[2]),             "30"},
    { "ChaffPos4",                InputDataDesc::ID_VECTOR, OFFSET(Chaff.Pos[3]),             "0 0 0"},
    { "ChaffVec4",                InputDataDesc::ID_VECTOR, OFFSET(Chaff.Vec[3]),             "0 0 200"},
    { "ChaffCount4",              InputDataDesc::ID_INT,    OFFSET(Chaff.Decoys[3]),             "30"},
    { "ChaffPos5",                InputDataDesc::ID_VECTOR, OFFSET(Chaff.Pos[4]),             "0 0 0"},
    { "ChaffVec5",                InputDataDesc::ID_VECTOR, OFFSET(Chaff.Vec[4]),             "0 0 200"},
    { "ChaffCount5",              InputDataDesc::ID_INT,    OFFSET(Chaff.Decoys[4]),             "30"},
    { "ChaffPos6",                InputDataDesc::ID_VECTOR, OFFSET(Chaff.Pos[5]),             "0 0 0"},
    { "ChaffVec6",                InputDataDesc::ID_VECTOR, OFFSET(Chaff.Vec[5]),             "0 0 200"},
    { "ChaffCount6",              InputDataDesc::ID_INT,    OFFSET(Chaff.Decoys[5]),             "30"},
    { "ChaffPos7",                InputDataDesc::ID_VECTOR, OFFSET(Chaff.Pos[6]),             "0 0 0"},
    { "ChaffVec7",                InputDataDesc::ID_VECTOR, OFFSET(Chaff.Vec[6]),             "0 0 200"},
    { "ChaffCount7",              InputDataDesc::ID_INT,    OFFSET(Chaff.Decoys[6]),             "30"},
    { "ChaffPos8",                InputDataDesc::ID_VECTOR, OFFSET(Chaff.Pos[7]),             "0 0 0"},
    { "ChaffVec8",                InputDataDesc::ID_VECTOR, OFFSET(Chaff.Vec[7]),             "0 0 200"},
    { "ChaffCount8",              InputDataDesc::ID_INT,    OFFSET(Chaff.Decoys[7]),             "30"},
    { "ChaffPos9",                InputDataDesc::ID_VECTOR, OFFSET(Chaff.Pos[8]),             "0 0 0"},
    { "ChaffVec9",                InputDataDesc::ID_VECTOR, OFFSET(Chaff.Vec[8]),             "0 0 200"},
    { "ChaffCount9",              InputDataDesc::ID_INT,    OFFSET(Chaff.Decoys[8]),             "30"},
    { "ChaffPos10",                InputDataDesc::ID_VECTOR, OFFSET(Chaff.Pos[9]),             "0 0 0"},
    { "ChaffVec10",                InputDataDesc::ID_VECTOR, OFFSET(Chaff.Vec[9]),             "0 0 200"},
    { "ChaffCount10",              InputDataDesc::ID_INT,    OFFSET(Chaff.Decoys[9]),             "30"},
    { "ChaffSeq",                 InputDataDesc::ID_INT, OFFSET(Chaff.Sequence),             "0"},
    { "ChaffDispensers",          InputDataDesc::ID_INT, OFFSET(Chaff.Count),                "1"},
    { "PilotEyePos",                InputDataDesc::ID_VECTOR, OFFSET(pilotEyePos),             "15 0 -3"},
    { "typeAC",   InputDataDesc::ID_INT, OFFSET(typeAC), "0"}, //TJL 02/08/04 Used to set aircraft specific features
    { "typeEngine",   InputDataDesc::ID_INT, OFFSET(typeEngine), "0"},//TJL 02/08/04 Used to set engine specific features

    { "elevRate",   InputDataDesc::ID_FLOAT, OFFSET(elevRate), "90"},  // MLR 1/14/2004 - Stab rates
    { "gunPitch", InputDataDesc::ID_FLOAT, OFFSET(gunElevation), "0"},   // MLR 1/28/2004 -
    { "gunYaw", InputDataDesc::ID_FLOAT, OFFSET(gunAzimuth), "0"},   // MLR 1/28/2004 -

    { "MeanTimeBetweenFailures", InputDataDesc::ID_FLOAT, OFFSET(MeanTimeBetweenFailures), "0"},   //Wombat778 2-24-04

    //{ "anim",          InputDataDesc::ID_FLOAT, OFFSET(anim),     "0.0"},

    // RV - Biker - Use default values
    //{ "A2GJDAMAlt", InputDataDesc::ID_FLOAT, OFFSET(A2GJDAMAlt), "-1"},
    //{ "A2GJSOWAlt", InputDataDesc::ID_FLOAT, OFFSET(A2GJSOWAlt), "-1"},
    //{ "A2GHarmAlt", InputDataDesc::ID_FLOAT, OFFSET(A2GHarmAlt), "-1"},
    //{ "A2GAGMAlt", InputDataDesc::ID_FLOAT, OFFSET(A2GAGMAlt), "-1"},
    //{ "A2GGBUAlt", InputDataDesc::ID_FLOAT, OFFSET(A2GGBUAlt), "-1"},
    //{ "A2GDumbHDAlt", InputDataDesc::ID_FLOAT, OFFSET(A2GDumbHDAlt), "-1"},
    //{ "A2GClusterAlt", InputDataDesc::ID_FLOAT, OFFSET(A2GClusterAlt), "-1"},
    //{ "A2GDumbLDAlt", InputDataDesc::ID_FLOAT, OFFSET(A2GDumbLDAlt), "-1"},
    //{ "A2GGenericBombAlt", InputDataDesc::ID_FLOAT, OFFSET(A2GGenericBombAlt), "-1"},
    //{ "A2GGunRocketAlt", InputDataDesc::ID_FLOAT, OFFSET(A2GGunRocketAlt), "-1"},
    //{ "A2GCameraAlt", InputDataDesc::ID_FLOAT, OFFSET(A2GCameraAlt), "-1"},
    //{ "A2GBombMissileAlt",          InputDataDesc::ID_FLOAT, OFFSET(A2GBombMissileAlt), "-1"},

    { "A2GJDAMAlt", InputDataDesc::ID_FLOAT, OFFSET(A2GJDAMAlt), "20000.0"},
    { "A2GJSOWAlt", InputDataDesc::ID_FLOAT, OFFSET(A2GJSOWAlt), "25000.0"},
    { "A2GHarmAlt", InputDataDesc::ID_FLOAT, OFFSET(A2GHarmAlt), "20000.0"},
    { "A2GAGMAlt", InputDataDesc::ID_FLOAT, OFFSET(A2GAGMAlt), "18000.0"},
    { "A2GGBUAlt", InputDataDesc::ID_FLOAT, OFFSET(A2GGBUAlt), "18000.0"},
    { "A2GDumbHDAlt", InputDataDesc::ID_FLOAT, OFFSET(A2GDumbHDAlt),  "1500.0"},
    { "A2GClusterAlt", InputDataDesc::ID_FLOAT, OFFSET(A2GClusterAlt), "15000.0"},
    { "A2GDumbLDAlt", InputDataDesc::ID_FLOAT, OFFSET(A2GDumbLDAlt), "15000.0"},
    { "A2GGenericBombAlt", InputDataDesc::ID_FLOAT, OFFSET(A2GGenericBombAlt), "1000.0"},
    { "A2GGunRocketAlt", InputDataDesc::ID_FLOAT, OFFSET(A2GGunRocketAlt),  "8000.0"},
    { "A2GCameraAlt", InputDataDesc::ID_FLOAT, OFFSET(A2GCameraAlt), "12000.0"},
    { "A2GBombMissileAlt",          InputDataDesc::ID_FLOAT, OFFSET(A2GBombMissileAlt), "13000.0"},

    { "swingwingHinge",             InputDataDesc::ID_VECTOR, OFFSET(swingWingHinge),   "2 3 0"},
    //Cobra 10/30/04 TJL
    { "animIntakeRamp1",           InputDataDesc::ID_2DTABLE, OFFSET(animIntakeRamp[0]), "0 0"},
    { "animIntakeRamp2",           InputDataDesc::ID_2DTABLE, OFFSET(animIntakeRamp[1]), "0 0"},
    { "animIntakeRamp3",           InputDataDesc::ID_2DTABLE, OFFSET(animIntakeRamp[2]), "0 0"},

    { "FTITStart", InputDataDesc::ID_FLOAT, OFFSET(FTITStart), "5.0"},//TJL 09/11/04
    { "FTITIdle", InputDataDesc::ID_FLOAT, OFFSET(FTITIdle), "7.0"},//TJL 09/11/04
    { "FTITMax", InputDataDesc::ID_FLOAT, OFFSET(FTITMax), "8.9"},//TJL 09/11/04
    { "criticalAOA", InputDataDesc::ID_FLOAT, OFFSET(criticalAOA), "0.0"},//TJL 09/11/04
    { "hasIFF", InputDataDesc::ID_INT, OFFSET(hasIFF), "0"},//Cobra 11/20/04
    { "hasThrRev", InputDataDesc::ID_INT, OFFSET(hasThrRev), "0"},//Cobra 02/06/05
    { NULL}
};
#undef OFFSET

// MLR 2/5/2004 - added
//extern float g_fA2GJDAMAlt; //TJL 10/27/03 Sets AI JSOW attack altitude
//extern float g_fA2GJSOWAlt; //TJL 10/27/03 Sets AI JSOW attack altitude
//extern float g_fA2GHarmAlt; //TJL 10/27/03 Sets AI HARM attack altitude (all set to SP3 defaults)
//extern float g_fA2GAGMAlt; //TJL 10/27/03 Sets AI AGM attack altitude
//extern float g_fA2GGBUAlt; //TJL 10/27/03 Sets AI GBU attack altitude
//extern float g_fA2GDumbHDAlt; //TJL 10/27/03 Sets AI Durandal attack altitude
//extern float g_fA2GClusterAlt; //TJL 10/27/03 Sets AI Cluster Bomb attack altitude
//extern float g_fA2GDumbLDAlt; //TJL 10/27/03 Sets AI Generic Bomb attack altitude
//extern float g_fA2GGenericBombAlt; //TJL 10/27/03 Sets AI Generic Bomb attack altitude
//extern float g_fA2GGunRocketAlt; //TJL 10/27/03 Sets AI Gun and Rocket altitude
//extern float g_fA2GCameraAlt; //TJL 10/27/03 Sets AI BDA/Recon altitude
//extern float g_fBombMissileAltitude;

AuxAeroData *AirframeAuxAeroRead(SimlibFileClass* inputFile)
{
    AuxAeroData *auxaeroData;

    auxaeroData = new AuxAeroData;

    if (ParseSimlibFile(auxaeroData, AuxAeroDataDesc, inputFile) == false)
    {
        //     F4Assert( not "Bad parsing of aux aero data");
    }

    // RV - Biker - That does not work so remove it
    // MLR 2/5/2004 - Load defaults as needed
    //if(auxaeroData->A2GJDAMAlt<0)
    // auxaeroData->A2GJDAMAlt=g_fA2GJDAMAlt;
    //
    //if(auxaeroData->A2GJSOWAlt<0)
    // auxaeroData->A2GJSOWAlt=g_fA2GJSOWAlt;
    //
    //if(auxaeroData->A2GHarmAlt<0)
    // auxaeroData->A2GHarmAlt=g_fA2GHarmAlt;
    //
    //if(auxaeroData->A2GAGMAlt<0)
    // auxaeroData->A2GAGMAlt=g_fA2GAGMAlt;
    //
    //if(auxaeroData->A2GGBUAlt<0)
    // auxaeroData->A2GGBUAlt=g_fA2GGBUAlt;
    //
    //if(auxaeroData->A2GDumbHDAlt<0)
    // auxaeroData->A2GDumbHDAlt=g_fA2GDumbHDAlt;
    //
    //if(auxaeroData->A2GClusterAlt<0)
    // auxaeroData->A2GClusterAlt=g_fA2GClusterAlt;
    //
    //if(auxaeroData->A2GDumbLDAlt<0)
    // auxaeroData->A2GDumbLDAlt=g_fA2GDumbLDAlt;
    //
    //if(auxaeroData->A2GGenericBombAlt<0)
    // auxaeroData->A2GGenericBombAlt=g_fA2GGenericBombAlt;
    //
    //if(auxaeroData->A2GGunRocketAlt<0)
    // auxaeroData->A2GGunRocketAlt=g_fA2GGunRocketAlt;
    //
    //if(auxaeroData->A2GCameraAlt<0)
    // auxaeroData->A2GCameraAlt=g_fA2GCameraAlt;
    //
    //if(auxaeroData->A2GBombMissileAlt<0)
    // auxaeroData->A2GBombMissileAlt=g_fBombMissileAltitude;*/

    return (auxaeroData);
}

EngineData::~EngineData()
{
    delete mach;
    delete alt;
    delete[] thrust[0];
    delete[] thrust[1];
    delete[] thrust[2];

    if (hasFuelFlow)
    {
        delete[] fuelflow[0];
        delete[] fuelflow[1];
        delete[] fuelflow[2];
    }
};


/***********************************************************************/
/*                                                                     */
/* Routine: EngineData *AirframeEngineRead (SimlibFileClass* inputFile) */
/*                                                                     */
/* Description:                                                        */
/*    Read one set of engine data.                                     */
/*                                                                     */
/* Inputs:                                                             */
/*    inputFile - file handle to read                                  */
/*                                                                     */
/* Outputs:                                                            */
/*    engineData - Engine data set                                     */
/*                                                                     */
/*  Development History :                                              */
/*  Date      Programer           Description                          */
/*---------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                        */
/*                                                                     */
/***********************************************************************/
EngineData *AirframeEngineRead(SimlibFileClass* inputFile)
{
    int numMach, numAlt;
    int i, mach, alt;
    EngineData *engineData;
    char *tok;

    /*-------------------------------------------------*/
    /* Allocate a block of memeory for the engine data */
    /*-------------------------------------------------*/
    engineData = new EngineData;

    engineData->fuelflow[0] = 0; // MLR 5/17/2004 -
    engineData->fuelflow[1] = 0;
    engineData->fuelflow[2] = 0;

    engineData->hasFuelFlow = 0;

    tok = inputFile->GetNext(); // MLR 5/17/2004 -

    while (stricmp(tok, "engopt") == 0)
    {
        tok = inputFile->GetNext();

        if (stricmp(tok, "fuelflow") == 0)
        {
            engineData->hasFuelFlow = 1;
            tok = inputFile->GetNext();
        }
    }

    /*-------------------------*/
    /* Read in thrust modifier */
    /*-------------------------*/
    engineData->thrustFactor = (float)atof(tok);

    /*----------------------------*/
    /* Read in fuel flow modifier */
    /*----------------------------*/
    engineData->fuelFlowFactor = (float)atof(inputFile->GetNext());

    /*-------------------------*/
    /* Read in Num Mach Breaks */
    /*-------------------------*/
    engineData->numMach = atoi(inputFile->GetNext());
    numMach = engineData->numMach;

    /*-------------------------------------------*/
    /* Allocate storage for the mach breakpoints */
    /*-------------------------------------------*/
#ifdef USE_SH_POOLS
    engineData->mach = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * numMach, 0);
#else
    engineData->mach = new float[numMach];
#endif

    /*------------------------------*/
    /* Read in the Mach breakpoints */
    /*------------------------------*/
    for (mach = 0; mach < numMach; mach++)
    {
        engineData->mach[mach] = (float)atof(inputFile->GetNext());
    }

    /*------------------------*/
    /* Read in Num Alt Breaks */
    /*------------------------*/
    engineData->numAlt = atoi(inputFile->GetNext());
    numAlt = engineData->numAlt;

    /*-------------------------------------------*/
    /* Allocate storage for the alt breakpoints */
    /*-------------------------------------------*/
#ifdef USE_SH_POOLS
    engineData->alt = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * numAlt, 0);
#else
    engineData->alt = new float[numAlt];
#endif

    /*------------------------------*/
    /* Read in the Mach breakpoints */
    /*------------------------------*/
    for (alt = 0; alt < numAlt; alt++)
    {
        engineData->alt[alt] = (float)atof(inputFile->GetNext());
    }

    /*----------------------------------------------*/
    /* Allocate Storage for three thrust data sets  */
    /* Set 0 - Idle                                 */
    /* Set 1 - Mil                                  */
    /* Set 2 - AB                                   */
    /*----------------------------------------------*/
#ifdef USE_SH_POOLS
    engineData->thrust[0] = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * numMach * numAlt, 0);
    engineData->thrust[1] = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * numMach * numAlt, 0);
    engineData->thrust[2] = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * numMach * numAlt, 0);
#else
    engineData->thrust[0] = new float[numMach * numAlt];
    engineData->thrust[1] = new float[numMach * numAlt];
    engineData->thrust[2] = new float[numMach * numAlt];
#endif

    /*-------------------------*/
    /* Read in the thrust data */
    /*-------------------------*/
    for (i = 0; i < 3; i++)
    {
        for (alt = 0; alt < numAlt; alt++)
        {
            for (mach = 0; mach < numMach; mach++)
            {
                engineData->thrust[i][alt * numMach + mach] =
                    (float)atof(inputFile->GetNext());
            }
        }
    }

    if (engineData->hasFuelFlow) // MLR 5/16/2004 -
    {

#ifdef USE_SH_POOLS
        engineData->fuelflow[0] = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * numMach * numAlt, 0);
        engineData->fuelflow[1] = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * numMach * numAlt, 0);
        engineData->fuelflow[2] = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * numMach * numAlt, 0);
#else
        engineData->fuelflow[0] = new float[numMach * numAlt];
        engineData->fuelflow[1] = new float[numMach * numAlt];
        engineData->fuelflow[2] = new float[numMach * numAlt];
#endif

        /*---------------------------*/
        /* Read in the fuelflow data */
        /*---------------------------*/
        for (i = 0; i < 3; i++)
        {
            for (alt = 0; alt < numAlt; alt++)
            {
                for (mach = 0; mach < numMach; mach++)
                {
                    engineData->fuelflow[i][alt * numMach + mach] =
                        (float)atof(inputFile->GetNext());
                }
            }
        }
    }

    // JB 010706
    engineData->hasAB = false;

    for (alt = 0; alt < numAlt and not engineData->hasAB; alt++)
        for (mach = 0; mach < numMach and not engineData->hasAB; mach++)
            if (engineData->thrust[1][alt * numMach + mach] not_eq engineData->thrust[2][alt * numMach + mach])
                engineData->hasAB = true;

    return (engineData);
}

/********************************************************************/
/*                                                                  */
/* Routine: RollData *AirframeFcsRead (SimlibFileClass* inputFile)   */
/*                                                                  */
/* Description:                                                     */
/*    Read one set of FCS data.                                     */
/*                                                                  */
/* Inputs:                                                          */
/*    inputFile - file handle to read                               */
/*                                                                  */
/* Outputs:                                                         */
/*    rollCmd - roll performance data set                           */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
RollData *AirframeFcsRead(SimlibFileClass* inputFile)
{
    int numAlpha, numQbar;
    int alpha, qbar;
    RollData *rollCmd;
    float scale;

    /*-------------------------------------*/
    /* Allocate storage for Roll rate Data */
    /*-------------------------------------*/
    rollCmd = new RollData;

    /*--------------------------------*/
    /* Read in number of alpha Breaks */
    /*--------------------------------*/
    rollCmd->numAlpha = atoi(inputFile->GetNext());
    numAlpha = rollCmd->numAlpha;

    /*---------------------------------------*/
    /* Reserve storage for alpha breakpoints */
    /*---------------------------------------*/
#ifdef USE_SH_POOLS
    rollCmd->alpha = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * numAlpha, 0);
#else
    rollCmd->alpha = new float[numAlpha];
#endif

    /*-------------------------------*/
    /* Read in the alpha breakpoints */
    /*-------------------------------*/
    for (alpha = 0; alpha < numAlpha; alpha++)
    {
        rollCmd->alpha[alpha] = (float)atof(inputFile->GetNext());
    }

    /*-------------------------------*/
    /* Read in number of qbar Breaks */
    /*-------------------------------*/
    rollCmd->numQbar = atoi(inputFile->GetNext());
    numQbar = rollCmd->numQbar;

    /*---------------------------------------*/
    /* Reserve storage for qbar breakpoints */
    /*---------------------------------------*/
#ifdef USE_SH_POOLS
    rollCmd->qbar = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * numQbar, 0);
#else
    rollCmd->qbar = new float[numQbar];
#endif

    /*-------------------------------*/
    /* Read in the qbar breakpoints */
    /*-------------------------------*/
    for (qbar = 0; qbar < numQbar; qbar++)
    {
        rollCmd->qbar[qbar] = (float)atof(inputFile->GetNext());
    }

    /*-------------------------------------*/
    /* Allocate memeory for roll rate data */
    /*-------------------------------------*/
#ifdef USE_SH_POOLS
    rollCmd->roll = (float *)MemAllocPtr(gReadInMemPool, sizeof(float) * numQbar * numAlpha, 0);
#else
    rollCmd->roll = new float[numAlpha * numQbar];
#endif

    /*-------------------*/
    /* read in roll rate */
    /*-------------------*/
    scale = (float)atof(inputFile->GetNext());

    for (alpha = 0; alpha < numAlpha; alpha++)
    {
        for (qbar = 0; qbar < numQbar; qbar++)
        {
            rollCmd->roll[alpha * numQbar + qbar] = (float)atof(inputFile->GetNext()) * scale;
        }
    }

    return (rollCmd);
}



