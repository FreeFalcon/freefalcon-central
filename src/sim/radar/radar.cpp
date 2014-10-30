#include "stdhdr.h"
#include "falcmesg.h"
#include "MsgInc/TrackMsg.h"
#include "simmover.h"
#include "Graphics/Include/Render2D.h"
#include "Graphics/Include/Mono2d.h"
#include "mfd.h"
#include "Object.h"
#include "falcsess.h"
#include "simdrive.h"
#include "otwdrive.h"
#include "campbase.h"
#include "handoff.h"
#include "radar.h"
#include "fack.h"
#include "aircrft.h"
#include "simfile.h" // to read in the extra data
#include "datafile.h" // datafile support routines

/* 2001-04-05 S.G. 'SOJ' */ #include "flight.h"
/* 2001-04-05 S.G. 'SOJ' */ #include "Classtbl.h"
/* 2001-09-07 S.G. RP5 */ extern bool g_bRP5Comp;

extern float g_fMoverVrValue;

// 2000-09-22 MODIFIED BY S.G. PER TEO CHER MIN, LOOK DOWN ANGLE SHOULD BE 2.5 INSTEAD OF 5.0
// static const float LOOK_DOWN_DECISION_ANGLE = 5 * DTR;
static const float LOOK_DOWN_DECISION_ANGLE = 2.5 * DTR;

// main beam radar return
// the ground/water gives where the main beam is

const UInt32 RadarClass::TrackUpdateTime = 2000;

const float RadarClass::CursorRate = 0.15f;//me123 status ok. changed from 0.25

RadarDataType* RadarDataTable = NULL;
short NumRadarEntries = 0;

RadarDataSet *radarDatFileTable = NULL;
short NumRadarDatFileTable = 0;

RadarClass::RadarClass(int type, SimMoverClass* parentPlatform) : SensorClass(parentPlatform)
{
    sensorType = Radar;
    dataProvided = ExactPosition;
    lastTargetLockSend = 0;
    isEmitting = TRUE;
    targetUnderCursor = FalconNullId;
    lasttargetUnderCursor = NULL;//me123
#if not NO_REMOTE_BUGGED_TARGET
    RemoteBuggedTarget = NULL;
#endif
    oldseekerElCenter = 0.0f;
    radarData = &RadarDataTable[ type ];
    digiRadarMode = DigiRWS; // 2002-02-09 ADDED BY S.G. Need to init the radar mode the digi will be set to by default...
    flag = FirstSweep; // 2002-03-10 ADDED BY S.G. Tells the RadarDigi::Exec function that the radar is doing is first sweep since creation, don't apply TimeToLock

    if ( not radarData->RDRDataInd) radarData->RDRDataInd = type;

    if (radarData->RDRDataInd < NumRadarDatFileTable)
        radarDatFile = &radarDatFileTable[radarData->RDRDataInd];
    else
        radarDatFile = 0;

    ShiAssert(radarDatFile);

    platform->SetRdrRng(radarData->NominalRange);
    platform->SetRdrAz(radarData->ScanHalfAngle);
    platform->SetRdrEl(radarData->ScanHalfAngle);
    platform->SetRdrCycleTime(3.0F);
    platform->SetRdrAzCenter(0.0f);
    platform->SetRdrElCenter(0.0f);
}

void RadarClass::SetPower(BOOL state)
{
    // Player's radar can break
    if (platform == SimDriver.GetPlayerAircraft())
    {
        if (((AircraftClass*)platform)->mFaults and (
                ((AircraftClass*)platform)->mFaults->GetFault(FaultClass::fcc_fault) == FaultClass::xmtr or
                ((AircraftClass*)platform)->mFaults->GetFault(FaultClass::fcc_fault) == FaultClass::bus))
        {
            state = FALSE;
        }
    }

    isOn = state;
    isEmitting = isEmitting and state;

    if ( not isEmitting)
    {
        if (lockedTarget) SendTrackMsg(lockedTarget, Track_Unlock);

        ClearSensorTarget();
        platform->SetRdrRng(0.0f);
    }
    else
    {
        platform->SetRdrRng(radarData->NominalRange);
    }
}


void RadarClass::SetEmitting(BOOL state)
{
    // JB 010706 If the range of the radar is zero, don't allow it to be turned on
    if (radarData->NominalRange == 0.0)
        state = FALSE;

    // Player's radar can break
    if (platform == SimDriver.GetPlayerAircraft())
    {
        if (((AircraftClass*)platform)->mFaults and (
                ((AircraftClass*)platform)->mFaults->GetFault(FaultClass::fcc_fault) == FaultClass::xmtr or
                ((AircraftClass*)platform)->mFaults->GetFault(FaultClass::fcc_fault) == FaultClass::bus))
        {
            state = FALSE;
        }
    }

    isEmitting = state and isOn;

    if ( not isEmitting)
    {
        if (lockedTarget)
        {
            SendTrackMsg(lockedTarget, Track_Unlock);
        }

        ClearSensorTarget();
        platform->SetRdrRng(0.0f);
        digiRadarMode = DigiOFF; // 2002-02-10 ADDED BY S.G. Resets digi radar mode to OFF when it stops emitting
    }
    else
    {
        platform->SetRdrRng(radarData->NominalRange);
        digiRadarMode = DigiRWS; // 2002-02-10 ADDED BY S.G. Resets digi radar mode to RWS when it starts emitting
    }
}


void RadarClass::SetDesiredTarget(SimObjectType* newTarget)
{
    if ( not newTarget or newTarget == lockedTarget)
    {
        return;
    }

    if (platform->IsAirplane() and platform->OnGround()) return;

    // If the baseData for the newTarget is the same as the lockedTarget then they are the same

    if ((newTarget) and (lockedTarget) and (newTarget->BaseData() == lockedTarget->BaseData()))
    {
        // S.G. NEED TO AT LEAST SET THE NEW LOCK TARGET, EVEN IF IT IS THE SAME BASE
        SensorClass::SetSensorTarget(newTarget);
        // END OF ADDED SECTION
        return;
    }

    // If we're not interested in our locked target anymore, tell him he's off the hook
    //if (lockedTarget) // JB 010223 CTD
    // if (lockedTarget and not F4IsBadCodePtr((FARPROC) lockedTarget) and not F4IsBadCodePtr((FARPROC) lockedTarget->BaseData())) { // JB 010223 CTD
    //me123 this is done in SetSensorTarget below SendTrackMsg (lockedTarget->BaseData()->Id(), Track_Unlock);
    // }

    // Note our new interest
    if (newTarget)
        SetSensorTarget(newTarget);

    // Force an immediate lock message next Exec cycle
    lastTargetLockSend = 0;
}


void RadarClass::SetSensorTarget(SimObjectType* newTarget)
{
    // 2002-02-10 ADDED BY S.G. Reset the digi radar mode to RWS upon losing your target
    if ( not newTarget)
    {
        digiRadarMode = DigiRWS;
    }

    if (lockedTarget and lockedTarget not_eq newTarget)
    {
        SendTrackMsg(lockedTarget, Track_Unlock);
    }

    SensorClass::SetSensorTarget(newTarget);
}

// 2002-02-10 ADDED BY S.G. Need to deal with our digiRadarMode before passing it to the SensorClass::ClearSensorTarget
void RadarClass::ClearSensorTarget(void)
{
    digiRadarMode = DigiRWS;
    SensorClass::ClearSensorTarget();
}

void RadarClass::DisplayInit(ImageBuffer* newImage)
{
    DisplayExit();

    privateDisplay = new Render2D;
    ((Render2D*)privateDisplay)->Setup(newImage);

    privateDisplay->SetColor(0xff00ff00);
}


float RadarClass::ReturnStrength(SimObjectType* target)
{
    static const float TAN_DECISION_ANGLE = (float)tan(LOOK_DOWN_DECISION_ANGLE);
    float S;
    float dz;
    float Vr;
    // float bottom;
    // float top;

    // FRB - CTD's here (bad target pointer)
    if (F4IsBadReadPtr(target, sizeof(SimObjectType)))
        return -1.0f;

    // Compare target range to nominal range against F16 sized target
    S = radarData->NominalRange / target->localData->range;

    // Factor in relative RCS of target
    S *= target->BaseData()->GetRCSFactor();

    // 2001-09-08 ADDED BY S.G. CHECK IF SHOULD USE RP5 DATA OR NOT
    if (0)// me123 agreed with jjb to test this g_bRP5Comp)
    {
        // END OF ADDED SECTION
        // See if we're looking downward
        dz = target->BaseData()->ZPos() - platform->ZPos();

        if (dz > target->localData->range * TAN_DECISION_ANGLE)
        {
            S *= radarData->LookDownPenalty;
        }
    }

    // See if the target is jamming
    if (target->BaseData()->IsSPJamming())
    {
        // MODIFIED BY S.G. SO ECM DEVICE ARE ONLY EFFECTIVE FROM ENEMY LOCATED AT AN az OF ±60° IN FRONT/BACK OF THE PLANE
        // AND AN el OF -30° TO +15°
        // S *= radarData->JammingPenalty;
        float ecmAngleFactor = 1;
        int iAz, iEl;

        iAz = (int)fabs(target->localData->azFrom * RTD);
        iEl = (int)(target->localData->elFrom * RTD);

        // If we don't go inside this if body, we do not have ECM protection at all so 'S' isn't reduced
        if ((iAz < 60 or iAz > 120) and iEl > -30 and iEl < 15)
        {
            if (iAz < 60 and iAz >= 30)
                ecmAngleFactor = (float)sqrt((60.0f * DTR - (float)fabs(target->localData->azFrom)) / (30.0f * DTR));
            else if (iAz > 120 and iAz <= 150)
                ecmAngleFactor = (float)sqrt(((float)fabs(target->localData->azFrom) - 120.0f * DTR) / (30.0f * DTR));

            if (iEl > 5)
                ecmAngleFactor *= (float)sqrt((15.0f * DTR - target->localData->elFrom) / (10.0f * DTR));
            else if (iEl < -20)
                ecmAngleFactor *= (float)sqrt((30.0f * DTR - target->localData->elFrom) / (10.0f * DTR));

            float temp;
            temp = S * radarData->JammingPenalty;
            temp = S - temp;
            temp = temp * ecmAngleFactor;
            S = S - temp;

            // 2001-08-01 ADDED BY S.G. VEHICLES HAVE DIFFERENT STRENGHT JAMMERS
            Falcon4EntityClassType* classPtr = (Falcon4EntityClassType*)target->BaseData()->EntityType();

            if (classPtr->dataType == DTYPE_VEHICLE)
            {
                int iJammerStrenght = (((VehicleClassDataType*)(classPtr->dataPtr))->Name[14] bitand 0x7f);

                if (iJammerStrenght)
                    S /= (float)iJammerStrenght / 10.0f;
            }
        }

        // END OF ADDED SECTION
    }

    // 2001-04-05 ADDED BY S.G. SO DEAGGREGATED GROUND RADAR ARE ALSO AFFECTED BY SOJ
    // Only if we're a battalion or an AWAC... Need to check the platform's canpaign object because Missiles (may be others?) don't have any
    if (platform->GetCampaignObject() and (platform->GetCampaignObject()->IsBattalion() or (platform->GetCampaignObject()->IsFlight() and platform->GetSType() == STYPE_UNIT_AWACS)))
    {
        CampBaseClass *campBaseObj;

        // Get the campaign object of the target we are querying
        if (target->BaseData()->IsSim())
            campBaseObj = ((SimBaseClass *)target->BaseData())->GetCampaignObject();
        else
            campBaseObj = (CampBaseClass *)target->BaseData();

        // Must be a flight because only them can have/be SOJed
        if (campBaseObj and campBaseObj->IsFlight())
        {
            // If its the ECM flight or an ECM protected flight...
            Flight ecmFlight = ((FlightClass *)campBaseObj)->GetECMFlight();

            if (ecmFlight)
            {
                if ( not ecmFlight->IsAreaJamming())
                    ecmFlight = NULL;
            }
            else if (((FlightClass *)campBaseObj)->HasAreaJamming())
                ecmFlight = (FlightClass *)campBaseObj;

            if (ecmFlight)
            {
                // Now, here's what we need to do:
                // 1. For now jamming power has 360 degrees coverage
                // 2. The radar range will be reduced by the ratio of its normal range and the jammer's range to the radar to the forth power
                // 3. The jammer is dropping the radar gain, effectively dropping its detection distance
                // 4. If the flight is outside this new range, it's not detected.

                // Get the range of the SOJ to the radar
                float jammerRange = DistSqu(ecmFlight->XPos(), ecmFlight->YPos(), platform->XPos(), platform->YPos());
                float mrs = radarData->NominalRange * NM_TO_KM * radarData->NominalRange * NM_TO_KM;

                // If the SOJ is within the radar normal range, 'adjust' it. If this is now less that ds (our range to the radar), return 0.
                // SOJ can jamm even if outside the detection range of the radar
                if (jammerRange < mrs * 2.25f)
                {
                    jammerRange = jammerRange / (mrs * 2.25f); // No need to check for zero because jammerRange has to be LESS than mrs to go in
                    S *= (jammerRange * jammerRange);
                }
            }
        }
    }

    // END OF ADDED SECTION
    // See if the target is in the Doppler notch
    if (target->BaseData()->OnGround() and not target->BaseData()->IsMover())
    {
        S = 0.0f;   //me123 don't show ground target's that don't move // 2001-09-07 S.G. ARE THEY SHOWING UP ON THE GM RADAR? IF NOT, WE HAVE A PROBLEM SINCE THEY WON'T SHOW ANYWHERE...
    }

    Vr = (float)cos(target->localData->ataFrom) * target->BaseData()->GetVt();

    // 2000-11-24 QUESTION BY S.G. me123, DON'T YOU THINK A MAX OF 450 knots IS A BIT FAST FOR GROUND VEHICLE?
    if (target->BaseData()->OnGround() and target->BaseData()->IsMover())
    {
        Vr = g_fMoverVrValue * (float)rand() / BIGGEST_RANDOM_NUMBER;   //me123 let's make some bogus Vt for the moving ground target
    }

    //me123 instead of simply checkign if target is 2.5 degrees below the horizon, lets atempt to
    //figure out the main beam cluuter effect
    // 2001-09-07 ADDED BY S.G. RP5 DATA ALREADY TAKES THIS INTO ACCOUNT. HAVING IT DONE IN THE EXE DOUBLES THE DESIRED EFFECT
    if (0)// me123 agreed with jjb to test this g_bRP5Comp)
    {
        // See if the target is in the Doppler notch
        Vr = (float)cos(target->localData->ataFrom) * target->BaseData()->GetVt();

        if (fabs(Vr) < radarData->NotchSpeed)
        {
            S *= radarData->NotchPenalty;
        }
    }
    else
    {
        dz = target->BaseData()->ZPos() - platform->ZPos();
        float mainclutter = FALSE;
        float lookdown = asin(dz / target->localData->range) - radarData->BeamHalfAngle;
        float mainbeamclutterrange = -platform->ZPos() * FT_TO_NM / sin(lookdown);
        mainclutter = min(1.0f,  radarData->NominalRange * FT_TO_NM / (mainbeamclutterrange - target->localData->range * FT_TO_NM))  ;

        if (mainclutter < 0.0f) mainclutter = 0.0f;

        //look down
        if (mainclutter)
            S /= max(1.0f, mainclutter / (radarData->LookDownPenalty * 2));

        // notch
        if (fabs(Vr) < radarData->NotchSpeed)
        {

            //notch look up
            if ( not mainclutter)
                S *= min(1.0f, radarData->NotchPenalty * 1.5f);

            //notch look down
            else
            {
                S /= max(3.0f, mainclutter / radarData->NotchPenalty);
                S /= max(1.0f, mainclutter / (radarData->LookDownPenalty));
            }
        }

        if (platform->IsAirplane())
        {
            // sidelobe effect 1  ("negative" doppler)
            if (Vr < 0.0f)
            {
                S *= 0.75f ;
            }

            // sidelobe effect 2  (co speed target)
            if (target->localData->rangedot > -20.0f and target->localData->rangedot < 20.0f * KNOTS_TO_FTPSEC)
            {
                S *= 0.90f;//me123 side lope clutter
            }
        }
    }

    // Hammer the signal to zero if the target is "in" the terrain and
    // we don't have line of sight (if the target is above terrain, we
    // assume line of sight EVEN THOUGH we might be wrong if we are low)
    if ( not platform->CheckLOS(target))
    {
        // 2001-05-14 MODIFIED BY S.G. SINCE THE RETURN VALUE IS ALWAYS LESS THAN SOMETHING
        // AND -1 IS LESS THAN 0, I'LL USE IT TO FLAG 'NoLOS' TO THE RadarDigi FUNCTION
        //S = 0.0f;
        S = -1.0f;
    }

    /* OTWDriver.GetAreaFloorAndCeiling (&bottom, &top);
    if (target->BaseData()->ZPos() > top or platform->ZPos() > top) {
     if ( not OTWDriver.CheckLOS( platform, target->BaseData() ) ) {
     S = 0.0f;
     }
    }*/

    return S;
}

void RadarClass::SendTrackMsg(SimObjectType* tgtptr, unsigned int trackType, unsigned int hardpoint)
{
    static int count = 0;
    static int countb = 0;
    ++count;

    if (
        (tgtptr == NULL) or (tgtptr->BaseData() == NULL) or
        (tgtptr->localData->lockmsgsend == Track_None and trackType == Track_Unlock) or
        (tgtptr->localData->lockmsgsend == Track_Launch and trackType == Track_Lock) or
        ( not ((SimBaseClass*)tgtptr->BaseData())->IsAirplane()) or
        (
            tgtptr->localData->lockmsgsend == trackType and (
                trackType not_eq Track_Lock or tgtptr->localData->lastRadarMode == hardpoint
            )
        )
    )
    {
        return; // 2002-02-10 MODIFIED BY S.G. Need to send a 'Track_Lock if the radar mode has changed
    }

    VU_ID id = tgtptr->BaseData()->Id();
    tgtptr->localData->lockmsgsend = trackType;

    if (trackType == Track_Lock)
    {
        // 2002-02-10 ADDED BY S.G. Keep track of the last radar mode if it's a Track_Lock message
        tgtptr->localData->lastRadarMode = hardpoint;
    }

    // Create and fill in the message structure
    VuGameEntity *game = vuLocalSessionEntity->Game();

    if ( not game) return;

    VuSessionsIterator Sessioniter(game);
    VuSessionEntity*   sess;
    sess = Sessioniter.GetFirst();
    int reliable = 1;

    while (sess)
    {
        if (
            (sess->CameraCount() > 0) and 
            (
                (sess->GetCameraEntity(0)->Id() == platform->Id()) or
                (sess->GetCameraEntity(0)->Id() == id)
            )
        )
        {
            reliable = 2;
            break;
        }

        sess = Sessioniter.GetNext();
    }

    countb ++;
    FalconTrackMessage* trackMsg = new FalconTrackMessage(reliable, platform->Id(), FalconLocalGame);

    ShiAssert(trackMsg);
    trackMsg->dataBlock.trackType = trackType;
    trackMsg->dataBlock.hardpoint = hardpoint;
    trackMsg->dataBlock.id = id;

    // Send our track list
    FalconSendMessage(trackMsg, TRUE);
}

// read in the .dat file

static const char RADAR_DIR[] = "sim\\radar";
static const char RADAR_DATASET[] = "radtypes.lst";


#define OFFSET(x) offsetof(RadarDataSet, x)
static const InputDataDesc radarDataDesc[] =
{
    {"Indx", InputDataDesc::ID_INT, OFFSET(Indx), "0" },
    {"prf", InputDataDesc::ID_INT, OFFSET(prf), "0" },
    {"TimeToLock", InputDataDesc::ID_INT, OFFSET(TimeToLock), "0" },
    {"MaxTwstargets", InputDataDesc::ID_INT, OFFSET(MaxTwstargets), "0" },
    {"Timetosearch1", InputDataDesc::ID_INT, OFFSET(Timetosearch1), "0" },
    {"Timetosearch2", InputDataDesc::ID_INT, OFFSET(Timetosearch2), "0" },
    {"Timetosearch3", InputDataDesc::ID_INT, OFFSET(Timetosearch3), "0" },
    {"Timetoacuire", InputDataDesc::ID_INT, OFFSET(Timetoacuire), "0" },
    {"Timetoguide", InputDataDesc::ID_INT, OFFSET(Timetoguide), "0" },
    {"Timetocoast", InputDataDesc::ID_INT, OFFSET(Timetocoast), "0" },
    {"Rangetosearch1", InputDataDesc::ID_INT, OFFSET(Rangetosearch1), "0" },
    {"Rangetosearch2", InputDataDesc::ID_INT, OFFSET(Rangetosearch2), "0" },
    {"Rangetosearch3", InputDataDesc::ID_INT, OFFSET(Rangetosearch3), "0" },
    {"Rangetoacuire", InputDataDesc::ID_INT, OFFSET(Rangetoacuire), "0" },
    {"Rangetoguide", InputDataDesc::ID_INT, OFFSET(Rangetoguide), "0" },
    {"Sweeptimesearch1", InputDataDesc::ID_INT, OFFSET(Sweeptimesearch1), "0" },
    {"Sweeptimesearch2", InputDataDesc::ID_INT, OFFSET(Sweeptimesearch2), "0" },
    {"Sweeptimesearch3", InputDataDesc::ID_INT, OFFSET(Sweeptimesearch3), "0" },
    {"Sweeptimeacuire", InputDataDesc::ID_INT, OFFSET(Sweeptimeacuire), "0" },
    {"Sweeptimeguide", InputDataDesc::ID_INT, OFFSET(Sweeptimeguide), "0" },
    {"Sweeptimecoast", InputDataDesc::ID_INT, OFFSET(Sweeptimecoast), "0" },
    {"Timeskillfactor", InputDataDesc::ID_INT, OFFSET(Timeskillfactor), "0" },
    {"Rwrsoundsearch1", InputDataDesc::ID_INT, OFFSET(Rwrsoundsearch1), "0" },
    {"Rwrsoundsearch2", InputDataDesc::ID_INT, OFFSET(Rwrsoundsearch2), "0" },
    {"Rwrsoundsearch3", InputDataDesc::ID_INT, OFFSET(Rwrsoundsearch3), "0" },
    {"Rwrsoundacuire", InputDataDesc::ID_INT, OFFSET(Rwrsoundacuire), "0" },
    {"Rwrsoundguide", InputDataDesc::ID_INT, OFFSET(Rwrsoundguide), "0" },
    {"Rwrsymbolsearch1", InputDataDesc::ID_INT, OFFSET(Rwrsymbolsearch1), "0" },
    {"Rwrsymbolsearch2", InputDataDesc::ID_INT, OFFSET(Rwrsymbolsearch2), "0" },
    {"Rwrsymbolsearch3", InputDataDesc::ID_INT, OFFSET(Rwrsymbolsearch3), "0" },
    {"Rwrsymbolacuire", InputDataDesc::ID_INT, OFFSET(Rwrsymbolacuire), "0" },
    {"Rwrsymbolguide", InputDataDesc::ID_INT, OFFSET(Rwrsymbolguide), "0" },
    {"AirFireRate", InputDataDesc::ID_INT, OFFSET(AirFireRate), "0" },
    {"Maxmissilesintheair", InputDataDesc::ID_INT, OFFSET(Maxmissilesintheair), "0" },
    {"Elevationbumpamounta", InputDataDesc::ID_INT, OFFSET(Elevationbumpamounta), "0" },
    {"Elevationbumpamountb", InputDataDesc::ID_INT, OFFSET(Elevationbumpamountb), "0" },
    {"AverageSpeed", InputDataDesc::ID_INT, OFFSET(AverageSpeed), "0" },
    {"MaxAngleDiffTws", InputDataDesc::ID_FLOAT, OFFSET(MaxAngleDiffTws), "0" },
    {"MaxRangeDiffTws", InputDataDesc::ID_FLOAT, OFFSET(MaxRangeDiffTws), "0" },
    {"MaxAngleDiffSam", InputDataDesc::ID_FLOAT, OFFSET(MaxAngleDiffSam), "0" },
    {"MaxRangeDiffSam", InputDataDesc::ID_FLOAT, OFFSET(MaxRangeDiffSam), "0" },
    {"MaxNctrRange", InputDataDesc::ID_FLOAT, OFFSET(MaxNctrRange), "364572.66" },
    {"NctrDelta", InputDataDesc::ID_FLOAT, OFFSET(NctrDelta), "0.1" },
    {"MinEngagementAlt", InputDataDesc::ID_FLOAT, OFFSET(MinEngagementAlt), "300.0" },
    {"MinEngagementRange", InputDataDesc::ID_FLOAT, OFFSET(MinEngagementRange), "0"},
    {NULL} // must be final node
};
#undef OFFSET

static void ReadDataArray(void *dataPtr, SimlibFileClass* inputFile, const InputDataDesc *desc)
{
    SimlibFileName buffer;

    while (inputFile->ReadLine(buffer, sizeof buffer) == SIMLIB_OK and buffer[0] not_eq 0)
    {
        ParseField(dataPtr, buffer, desc);
        buffer[0] = 0;
    }
}

void ReadAllRadarData(void)
{
    int i;
    SimlibFileClass* rclist;
    SimlibFileClass* inputFile;
    SimlibFileName buffer;
    SimlibFileName fileName;
    SimlibFileName fName;

    sprintf(fileName, "%s\\%s", RADAR_DIR, RADAR_DATASET);
    rclist = SimlibFileClass::Open(fileName, SIMLIB_READ);

    if (rclist == NULL) return;

    NumRadarDatFileTable = atoi(rclist->GetNext());

    radarDatFileTable = new RadarDataSet[NumRadarDatFileTable];

    for (i = 0; i < NumRadarDatFileTable; i++)
    {
        rclist->ReadLine(buffer, 80);

        /*-----------------*/
        /* open input file */
        /*-----------------*/
        sprintf(fName, "%s\\%s.dat", RADAR_DIR, buffer);
        inputFile = SimlibFileClass::Open(fName, SIMLIB_READ);

        F4Assert(inputFile);
        ReadDataArray(&radarDatFileTable[i], inputFile, radarDataDesc);
        inputFile->Close();
        delete inputFile;
        inputFile = NULL;
    }

    rclist->Close();
    delete rclist;
}


