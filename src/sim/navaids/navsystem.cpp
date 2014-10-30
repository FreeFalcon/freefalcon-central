/*-----------------------------------------------------------

NAVSYSTEM.CPP

  Written by: Vincent Finley
  For: Microprose Inc.

 Revision History:
 Created: 1/9/98

 Function List:
 NavigationSystem::SetControlMechanism()
 NavigationSystem::GetControlMechanism()
 NavigationSystem::ToggleControlMechanism
 NavigationSystem::SetInstrumentMode

 NavigationSystem::GetInstrumentMode
 NavigationSystem::StepInstrumentMode
 NavigationSystem::GetMarkPoint
 NavigationSystem::SetMarkPoint
 NavigationSystem::GetDataLink
 NavigationSystem::SetDataLink
 NavigationSystem::FillNextDataLink

 NavigationSystem::SetTacanChannel
 NavigationSystem::GetTacanChannel

 NavigationSystem::StepTacanChannelDigit
 NavigationSystem::StepTacanBand

 Description:
 This file consolidates all of the cockpit related navigation
 instruments and lists.
-----------------------------------------------------------------*/

#include "navsystem.h"
#include "vu2.h"
#include "classtbl.h"
#include "ptdata.h"
#include "cphsi.h"
#include "fcc.h"
#include "atcbrain.h"
#include "simdrive.h"
#include "find.h"
#include "flight.h"
#include "aircrft.h"

//MI for ICP stuff
extern bool g_bRealisticAvionics;
#include "icp.h"
extern bool g_bINS;

// sfr: this is the only variable of this type, I couldnt see any other
// and the stupid guy who changed this added dereferences all around, including in the constructor
// serious, this guys should be jailed
NavigationSystem* gNavigationSys;

extern char* DLink_Type_Str[5];

static const float DEF_LAT = 33.775918333F;    // 33 degrees, 45 minutes, 33.06 secs
static const float DEF_LONG = 119.1148778F;    //119 degrees,  6 minutes, 53.56
float FALCON_ORIGIN_LAT = DEF_LAT;
float FALCON_ORIGIN_LONG = DEF_LONG;


//------------------------------------------------------
// ConvertRadtoNav()
//------------------------------------------------------

float ConvertRadtoNav(float radAngle)
{
    if (radAngle <= HALFPI and radAngle >= -PI)
    {
        return((HALFPI - radAngle) * RTD);
    }
    else if (radAngle <= PI and radAngle >= HALFPI)
    {
        return(((5 * HALFPI) - radAngle) * RTD);
    }
    else
    {
        return((2 * PI - radAngle) * RTD);
    }
}
////////////////////////////////////////////////////////



//------------------------------------------------------
// ConvertNavtoRad()
//------------------------------------------------------
float ConvertNavtoRad(float navAngle)
{
    if ((navAngle > 0.0F) and (navAngle < 90.0F))
    {
        return(HALFPI - (navAngle * DTR));
    }
    else
    {
        return(5 * HALFPI - (navAngle * DTR));
    }
}
////////////////////////////////////////////////////////


void SetLatLong(float latitude, float longitude)
{
    FALCON_ORIGIN_LAT = latitude;
    FALCON_ORIGIN_LONG = longitude;
}

void ResetLatLong()
{
    FALCON_ORIGIN_LAT = DEF_LAT;
    FALCON_ORIGIN_LONG = DEF_LONG;
}

void GetLatLong(float *latitude, float *longitude)
{
    *latitude = FALCON_ORIGIN_LAT;
    *longitude = FALCON_ORIGIN_LONG;
}
//---------------------------------------------------------------
// ApproxLatLong
//---------------------------------------------------------------

void ApproxLatLong(float x, float y, float* latitude, float* longitude)
{

    float cosLatitude;

    *latitude = (FALCON_ORIGIN_LAT * FT_PER_DEGREE + x) / EARTH_RADIUS_FT;
    cosLatitude = (float)cos(*latitude);

    *longitude = ((FALCON_ORIGIN_LONG * DTR * EARTH_RADIUS_FT * cosLatitude) + y) / (EARTH_RADIUS_FT * cosLatitude);
}



//---------------------------------------------------------------
// BuildLatLongStr
//---------------------------------------------------------------

void BuildLatLongStr(float latitude, float longitude, char* latStr, char* longStr)
{

    int latDeg, longDeg;
    float latMin, longMin;
    char *LatStr = "N";
    char *LongStr = "E";

    latitude *= RTD;
    longitude *= RTD;

    longDeg = FloatToInt32(longitude);
    longMin = (float)fabs(longitude - longDeg) * DEG_TO_MIN;

    latDeg = FloatToInt32(latitude);
    latMin = (float)fabs(latitude - latDeg) * DEG_TO_MIN;

    // format lat/long here
    if (latDeg < 0)
        LatStr = "S";

    if (longDeg < 0)
        LongStr = "W";

    if (latMin < 10.0F)
    {
        sprintf(latStr, "LAT  %s %3d *  0%2.2f \'", LatStr, latDeg, latMin);
    }
    else
    {
        sprintf(latStr, "LAT  %s %3d *  %2.2f \'", LatStr, latDeg, latMin);
    }

    if (longMin < 10.0F)
    {
        sprintf(longStr, "LNG  %s %3d *  %02.2f \'", LongStr, longDeg, longMin);
    }
    else
    {
        sprintf(longStr, "LNG  %s %3d *  %2.2f \'", LongStr, longDeg, longMin);
    }
}



//---------------------------------------------------------------
// GetTypeString
//---------------------------------------------------------------
void GetTypeString(int index, char* string)
{

    uchar dataType = Falcon4ClassTable[index].dataType;


    if (dataType == DTYPE_FEATURE)
    {
        struct FeatureEntry* dataPtr;
        dataPtr = (struct FeatureEntry*) Falcon4ClassTable[index].dataPtr;
        strcpy(string, "feature");
    }
    else if (dataType == DTYPE_OBJECTIVE)
    {
        struct ObjClassDataType* dataPtr;
        dataPtr = (struct ObjClassDataType*) Falcon4ClassTable[index].dataPtr;
        strcpy(string, dataPtr->Name);
    }
    else if (dataType == DTYPE_UNIT)
    {
        struct UnitClassDataType* dataPtr;
        dataPtr = (struct UnitClassDataType*) Falcon4ClassTable[index].dataPtr;
        strcpy(string, dataPtr->Name);
    }
    else if (dataType == DTYPE_VEHICLE)
    {
        struct VehicleClassDataType* dataPtr;
        dataPtr = (struct VehicleClassDataType*) Falcon4ClassTable[index].dataPtr;
        strcpy(string, dataPtr->Name);
    }
    else if (dataType == DTYPE_WEAPON)
    {
        struct WeaponClassDataType* dataPtr;
        dataPtr = (struct WeaponClassDataType*) Falcon4ClassTable[index].dataPtr;
        strcpy(string, dataPtr->Name);
    }
    else
    {
        string = NULL;
    }
}



//---------------------------------------------------------------
// NavigationSystem::NavigationSystem
//---------------------------------------------------------------

NavigationSystem::NavigationSystem()
{

    int i;

    mInstrumentMode = NAV;
    mCurrentTCNSrc = ICP;

    mpCurrentDomain[AUXCOMM] = TacanList::AA;
    mpCurrentDomain[ICP] = TacanList::AA;

    memset(&mpCurrentTCN[ICP], 0, sizeof mpCurrentTCN[ICP]);
    memset(&mpCurrentTCN[AUXCOMM], 0, sizeof mpCurrentTCN[ICP]);

    SetTacanChannel(AUXCOMM, 106, TacanList::X);

    //MI initialize the ICP's settings
    SetTacanChannel(ICP, 106, TacanList::X);

    mpMissionTacans = NULL;
    mpCurrentMissionTacan = NULL;

    mCurrentMark = 0;
    mCurrentDLink = 0;

    mUHFMode = UHF_NORM;

    mpCurrentIls.frontx = 0.0F;
    mpCurrentIls.fronty = 0.0F;
    mpCurrentIls.backx = 0.0F;
    mpCurrentIls.backy = 0.0F;
    mpCurrentIls.z = 0.0F;
    mpCurrentIls.rwyidx = 0;
    *mpCurrentIls.rwynum = NULL;
    mpCurrentIls.heading = 0;
    mpCurrentIls.sinHeading = 0.0F;
    mpCurrentIls.cosHeading = 0.0F;
    mpCurrentIls.vuID = FalconNullId;
    mpCurrentIls.pobjective = NULL;


    mCurrentDESTOA = 0;
    mCurrentVIPOA = 0;
    mCurrentVRPOA = 0;

    for (i = 0; i < MAX_MARKPOINTS; i++)
    {
        mpMarkPoints[i].pointType = NODATA;
        mpMarkPoints[i].pWaypoint = NULL;
        *(mpMarkPoints[i].pLatStr) = NULL;
        *(mpMarkPoints[i].pLongStr) = NULL;
    }

    for (i = 0; i < MAX_DLINKPOINTS; i++)
    {
        mpDLinkPoints[i].pointType = FalconDLinkMessage::NODLINK;
        mpDLinkPoints[i].pWaypoint = NULL;
    }

    //MI OA Stuff
    if (g_bRealisticAvionics)
    {
        for (i = 0; i < MAX_DESTOA; i++)
        {
            mpDESTOA[i].pointType = NODATA;
            mpDESTOA[i].pWaypoint = NULL;
            *(mpDESTOA[i].pLatStr) = NULL;
            *(mpDESTOA[i].pLongStr) = NULL;
        }

        for (i = 0; i < MAX_VIPOA; i++)
        {
            mpVIPOA[i].pointType = NODATA;
            mpVIPOA[i].pWaypoint = NULL;
            *(mpVIPOA[i].pLatStr) = NULL;
            *(mpVIPOA[i].pLongStr) = NULL;
        }

        for (i = 0; i < MAX_VRPOA; i++)
        {
            mpVRPOA[i].pointType = NODATA;
            mpVRPOA[i].pWaypoint = NULL;
            *(mpVRPOA[i].pLatStr) = NULL;
            *(mpVRPOA[i].pLongStr) = NULL;
        }
    }
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::~NavigationSystem
//---------------------------------------------------------------

NavigationSystem::~NavigationSystem()
{

    int i;

    DeleteMissionTacans();

    for (i = 0; i < MAX_MARKPOINTS; i++)
    {
        if (mpMarkPoints[i].pWaypoint)
        {
            delete mpMarkPoints[i].pWaypoint;
        }
    }

    for (i = 0; i < MAX_DLINKPOINTS; i++)
    {
        if (mpDLinkPoints[i].pWaypoint)
        {
            delete mpDLinkPoints[i].pWaypoint;
        }
    }

    //MI OA Stuff
    if (g_bRealisticAvionics)
    {
        for (i = 0; i < MAX_DESTOA; i++)
        {
            if (mpDESTOA[i].pWaypoint)
                delete mpDESTOA[i].pWaypoint;
        }

        for (i = 0; i < MAX_VIPOA; i++)
        {
            if (mpVIPOA[i].pWaypoint)
                delete mpVIPOA[i].pWaypoint;
        }

        for (i = 0; i < MAX_VRPOA; i++)
        {
            if (mpVRPOA[i].pWaypoint)
                delete mpVRPOA[i].pWaypoint;
        }
    }
}

/////////////////////////////////////////////////////////////////


//---------------------------------------------------------------
// NavigationSystem::SetIlsData
//---------------------------------------------------------------
void NavigationSystem::SetIlsData(VU_ID airbaseid, int rwyidx)
{

    int backIdx;

    mpCurrentIls.vuID = airbaseid;
    mpCurrentIls.pobjective = (ObjectiveClass *)vuDatabase->Find(mpCurrentIls.vuID);
    ShiAssert(FALSE == F4IsBadReadPtr(mpCurrentIls.pobjective, sizeof * mpCurrentIls.pobjective));
    // if (mpCurrentIls.pobjective->ZPos() == 0.0f) // JPO - fix up old data
    //      mpCurrentIls.pobjective->SetPosition (mpCurrentIls.pobjective->XPos(),
    // mpCurrentIls.pobjective->YPos(),
    // OTWDriver.GetGroundLevel(mpCurrentIls.pobjective->XPos(), mpCurrentIls.pobjective->YPos()));

    mpCurrentIls.rwyidx = rwyidx;
    backIdx = mpCurrentIls.pobjective->brain->GetOppositeRunway(mpCurrentIls.rwyidx);

    TranslatePointData(mpCurrentIls.pobjective, GetFirstPt(backIdx), &mpCurrentIls.backx, &mpCurrentIls.backy);
    TranslatePointData(mpCurrentIls.pobjective, GetFirstPt(mpCurrentIls.rwyidx), &mpCurrentIls.frontx, &mpCurrentIls.fronty);

    // 2001-05-15 MODIFIED BY S.G. PER JULIAN'S INSTRUCTION BUT USING MY APPROACH. JULIAN'S APPROACH CAN POSSIBLY CREATE PROBLEMS IN OTHER SECTION OF THE CODE THAT ASSUMES THE OBJECTIVE IS AT ZERO ALTITUDE
    // mpCurrentIls.z = mpCurrentIls.pobjective->ZPos();
    if (mpCurrentIls.pobjective->ZPos() == 0.0f) // JPO - fix up old data
        mpCurrentIls.z = OTWDriver.GetGroundLevel(mpCurrentIls.pobjective->XPos(), mpCurrentIls.pobjective->YPos());
    else
        mpCurrentIls.z = mpCurrentIls.pobjective->ZPos();

    mpCurrentIls.heading = PtHeaderDataTable[backIdx].data;
    mpCurrentIls.sinHeading = PtHeaderDataTable[backIdx].sinHeading;
    mpCurrentIls.cosHeading = PtHeaderDataTable[backIdx].cosHeading;

    if (PtHeaderDataTable[backIdx].ltrt == 0)
    {
        // 2001-05-24 MODIFIED BY S.G. NEED TO ROUND, NOT TRUNCATE
        // sprintf(mpCurrentIls.rwynum, "%02d", (int)(PtHeaderDataTable[backIdx].data/10.0F + 0.499F));
        sprintf(mpCurrentIls.rwynum, "%02d", (int)(PtHeaderDataTable[backIdx].data / 10.0F + 0.53F));
    }
    else if (PtHeaderDataTable[backIdx].ltrt == -1)   // left, just a guess vwf
    {
        // 2001-05-24 MODIFIED BY S.G. NEED TO ROUND, NOT TRUNCATE
        // sprintf(mpCurrentIls.rwynum, "%02dL", (int)(PtHeaderDataTable[backIdx].data/10.0F + 0.499F));
        sprintf(mpCurrentIls.rwynum, "%02dL", (int)(PtHeaderDataTable[backIdx].data / 10.0F + 0.53F));
    }
    else  if (PtHeaderDataTable[backIdx].ltrt == 1)
    {
        // 2001-05-24 MODIFIED BY S.G. NEED TO ROUND, NOT TRUNCATE
        // sprintf(mpCurrentIls.rwynum, "%02dR", (int)(PtHeaderDataTable[backIdx].data/10.0F + 0.499F));
        sprintf(mpCurrentIls.rwynum, "%02dR", (int)(PtHeaderDataTable[backIdx].data / 10.0F + 0.53F));
    }
    else
    {
        ShiWarning("Bad ILS Data");
    }
}
/////////////////////////////////////////////////////////////////


//---------------------------------------------------------------
// NavigationSystem::ExecIls
//---------------------------------------------------------------
void NavigationSystem::ExecIls(void)
{
    //MI moved to the class
    /*float xfrontdiff;
    float yfrontdiff;
    float xbackdiff;
    float ybackdiff;
    float zdiff;
    float bearingToLocalizer;
    float distToToLocalizer;
    float el;
    float approach;*/

    mpCurrentIls.gpDeviation = -180.0F * DTR;
    mpCurrentIls.gsDeviation = 180.0F * DTR;

    if (SimDriver.GetPlayerAircraft() and mpCurrentIls.rwyidx not_eq 0 and mpCurrentIls.vuID not_eq FalconNullId and (GetInstrumentMode() == NavigationSystem::ILS_TACAN or GetInstrumentMode() == NavigationSystem::ILS_NAV))
    {

        xfrontdiff = mpCurrentIls.frontx - SimDriver.GetPlayerAircraft()->XPos();
        yfrontdiff = mpCurrentIls.fronty - SimDriver.GetPlayerAircraft()->YPos();
        xbackdiff = mpCurrentIls.backx - SimDriver.GetPlayerAircraft()->XPos();
        ybackdiff = mpCurrentIls.backy - SimDriver.GetPlayerAircraft()->YPos();

        zdiff = (float)fabs(mpCurrentIls.z - SimDriver.GetPlayerAircraft()->ZPos());

        bearingToLocalizer = (float) atan2(xbackdiff, ybackdiff); // radians +-pi, xaxis = 0deg

        if (bearingToLocalizer >= -90.0F * DTR and bearingToLocalizer <= 180.0F * DTR)
        {
            bearingToLocalizer = 90.0F * DTR - bearingToLocalizer;
        }
        else
        {
            bearingToLocalizer = -(270.0F * DTR + bearingToLocalizer);
        }

        distToToLocalizer = (float) sqrt(xfrontdiff * xfrontdiff + yfrontdiff * yfrontdiff);

        el = (float) atan2(zdiff, distToToLocalizer);
        approach = (float)mpCurrentIls.heading;

        if (approach > 180)
        {
            approach -= 360;
        }

        approach *= DTR;

        mpCurrentIls.gpDeviation = bearingToLocalizer - approach; // Calc approach error

        if (mpCurrentIls.gpDeviation < -180.0F * DTR)
        {
            mpCurrentIls.gpDeviation += 360.0F * DTR;
        }
        else if (mpCurrentIls.gpDeviation > 180.0F * DTR)
        {
            mpCurrentIls.gpDeviation -= 360.0F * DTR;
        }

        mpCurrentIls.gsDeviation = 3.0F * DTR - el; // Calc glideSlope error 3 degress from level
    }
}

/////////////////////////////////////////////////////////////////

//---------------------------------------------------------------
// NavigationSystem::GetILSAttribute
//---------------------------------------------------------------

BOOL NavigationSystem::GetILSAttribute(Attribute attribute, float* value)
{

    BOOL returnVal = FALSE;

    *value = 0.0F;

    // MD -- 20040605: adding a reception check here -- you should not be able to get a signal and therefore
    // needles should remain inactive if range to station is greater than ~25nm (150k feet)
    if (mpCurrentIls.vuID not_eq FalconNullId and mpCurrentIls.rwyidx not_eq 0 and (distToToLocalizer < 150000.0F))
    {
        //MI check to make sure we are allowed to get the ILS
        if (g_bRealisticAvionics)
        {
            VU_ID ID;
            int Digit1, Digit2, Digit3, TacanChannel;
            int range, type;
            float ilsf;

            if (GetControlSrc() == NavigationSystem::AUXCOMM)
            {
                //Get our current AUXComm channel
                Digit1 = /*gNavigationSys->*/GetTacanChannel(NavigationSystem::AUXCOMM, 2);
                Digit2 = /*gNavigationSys->*/GetTacanChannel(NavigationSystem::AUXCOMM, 1);
                Digit3 = /*gNavigationSys->*/GetTacanChannel(NavigationSystem::AUXCOMM, 0);
                TacanChannel = (Digit1 * 100 + Digit2 * 10 + Digit3);

                gTacanList->GetVUIDFromChannel(TacanChannel,
                                               GetTacanBand(NavigationSystem::AUXCOMM), GetDomain(NavigationSystem::AUXCOMM),
                                               &ID, &range, &type, &ilsf);

                if (GetTacanBand(NavigationSystem::AUXCOMM) == TacanList::Y or //Tacanband isn't X
                    GetDomain(NavigationSystem::AUXCOMM) == TacanList::AA or //Not in AG Mode
                    mpCurrentIls.vuID not_eq ID) //Tacanchannel not same
                {
                    return FALSE;
                }
            }
            else if (GetControlSrc() == NavigationSystem::ICP)
            {
                Digit1 = /*gNavigationSys->*/GetTacanChannel(NavigationSystem::ICP, 2);
                Digit2 = /*gNavigationSys->*/GetTacanChannel(NavigationSystem::ICP, 1);
                Digit3 = /*gNavigationSys->*/GetTacanChannel(NavigationSystem::ICP, 0);
                TacanChannel = (Digit1 * 100 + Digit2 * 10 + Digit3);

                gTacanList->GetVUIDFromChannel(TacanChannel, //Tacanchannel not same
                                               GetTacanBand(NavigationSystem::ICP), GetDomain(NavigationSystem::ICP),
                                               &ID, &range, &type, &ilsf);

                if (GetTacanBand(NavigationSystem::ICP) == TacanList::Y or //Tacanband isn't X
                    GetDomain(NavigationSystem::ICP) == TacanList::AA or // Not in AG mode
                    mpCurrentIls.vuID not_eq ID)
                {
                    return FALSE;
                }
            }
        }

        if (attribute == GP_DEV)
        {
            *value = mpCurrentIls.gpDeviation;
            returnVal = TRUE;

            if (g_bRealisticAvionics and g_bINS and SimDriver.GetPlayerAircraft())
                SimDriver.GetPlayerAircraft()->LOCValid = TRUE; //Flag not visible
        }
        else if (attribute == GS_DEV)
        {
            *value = mpCurrentIls.gsDeviation;
            returnVal = TRUE;

            if (g_bRealisticAvionics and g_bINS and SimDriver.GetPlayerAircraft())
                SimDriver.GetPlayerAircraft()->GSValid = TRUE; //Flag not visible
        }
    }
    else
    {
        //MI additions
        if (g_bRealisticAvionics and g_bINS and SimDriver.GetPlayerAircraft())
        {
            SimDriver.GetPlayerAircraft()->LOCValid = FALSE; //Flag visible
            SimDriver.GetPlayerAircraft()->GSValid = FALSE; //Flag visible
        }
    }

    return returnVal;
}


/////////////////////////////////////////////////////////////////

//---------------------------------------------------------------
// NavigationSystem::GetILSAttribute
//---------------------------------------------------------------

BOOL NavigationSystem::GetILSAttribute(Attribute attribute, char* pRwyNum)
{
    BOOL returnVal = FALSE;

    *pRwyNum = NULL;

    if (mpCurrentIls.vuID not_eq FalconNullId and mpCurrentIls.rwyidx not_eq 0)
    {

        if (attribute == RWY_NUM)
        {
            strcpy(pRwyNum, mpCurrentIls.rwynum);
            returnVal = TRUE;
        }
    }

    return returnVal;
}
/////////////////////////////////////////////////////////////////

//---------------------------------------------------------------
// NavigationSystem::GetILSAttribute
//---------------------------------------------------------------

BOOL NavigationSystem::GetILSAttribute(Attribute attribute, VU_ID* pId)
{
    BOOL returnVal = FALSE;

    *pId = mpCurrentIls.vuID;

    if (*pId not_eq FalconNullId)
    {
        returnVal = TRUE;
    }

    return returnVal;
    attribute;
}
/////////////////////////////////////////////////////////////////



void NavigationSystem::DeleteMissionTacans(void)
{

    Tacan_Data_LL_Str* pLink;
    Tacan_Data_LL_Str* pNext;

    if (mpMissionTacans)
    {
        pLink = mpMissionTacans;

        while (pLink)
        {
            pNext = pLink->pNext;
            delete pLink->pData;
            delete pLink;
            pLink = pNext;
        }

        mpMissionTacans = NULL;
        mpCurrentMissionTacan = NULL;
    }
}
/////////////////////////////////////////////////////////////////


//---------------------------------------------------------------
// NavigationSystem::SetMissionTacans
//---------------------------------------------------------------

void NavigationSystem::SetMissionTacans(AircraftClass* ownship)
{

    WayPointClass* pwaypoint = ownship->waypoint;
    VU_ID id;
    int channel;
    TacanList::Domain domain;
    TacanList::StationSet set;
    Tacan_Data_LL_Str* pLink;
    Tacan_Data_LL_Str* prevLink;
    FlightClass* pFlight = NULL;
    FlightClass* pTankerFlight = NULL;

    if (mpMissionTacans)
    {
        DeleteMissionTacans();
    }

    // 2001-10-09 M.N. Modified -> Sorting direction changed so that our home base will become
    // the first Tacanlist element and thus will be set up as our ICP tacan channel
    // old sorting: If alternate base present, its tacan channel gets ICP tacan channel

    // Moved tanker tacan channel behind waypoint "channels"

    // prevLink = new Tacan_Data_LL_Str;
    // prevLink->pData = new Tacan_Data_Str;
    prevLink = NULL;

    while (pwaypoint)
    {
        if (pwaypoint->GetWPAction() == WP_LAND)
        {

            id = pwaypoint->GetWPTargetID();
            int range, ttype;
            float ilsfreq;

            if (gTacanList->GetChannelFromVUID(id, &channel, &set, &domain,
                                               &range, &ttype, &ilsfreq))   // If we find the tacan put it in the list
            {


                pLink = new Tacan_Data_LL_Str;
                pLink->pData = new Tacan_Data_Str;

                pLink->pNext = NULL;
                pLink->pPrevious = NULL;
                pLink->pData->vuID = id;
                pLink->pData->channel = channel;
                pLink->pData->set = set;
                pLink->pData->digits[2] = channel / 100; // Break up into digits
                channel = channel - pLink->pData->digits[2] * 100;
                pLink->pData->digits[1] = channel / 10;
                channel = channel - pLink->pData->digits[1] * 10;
                pLink->pData->digits[0] = channel;
                pLink->pData->range = range;
                pLink->pData->ilsfreq = ilsfreq;
                pLink->pData->ttype = ttype;

                pLink->type = AIRBASE;

                //add link
                if (mpMissionTacans == NULL)
                {
                    mpMissionTacans = pLink;
                    pLink->pPrevious = NULL;
                }
                else
                {
                    // mpMissionTacans->pPrevious = pLink;
                    // pLink->pNext = mpMissionTacans;

                    // 2001-10-08 M.N. sort the other way around
                    //                 -> first list entry is always our home base
                    ShiAssert(prevLink);
                    pLink->pPrevious = prevLink;
                    mpMissionTacans->pNext = pLink;

                }

                prevLink = pLink;
            }
        }

        pwaypoint = pwaypoint->GetNextWP();
    }

    pFlight = (FlightClass*) ownship->GetCampaignObject();

    if (pFlight)
    {
        pTankerFlight = pFlight->GetTankerFlight();
    }

    if (pTankerFlight)
    {
        pLink = new Tacan_Data_LL_Str;
        pLink->pData = new Tacan_Data_Str;

        pLink->pNext = NULL;
        pLink->pPrevious = NULL;
        pLink->pData->vuID = pTankerFlight->Id();
        pLink->pData->channel = (int)pTankerFlight->tacan_channel;
        channel = pLink->pData->channel;
        pLink->pData->set = TacanList::Y;
        pLink->pData->digits[2] = channel / 100; // Break up into digits
        channel = channel - pLink->pData->digits[2] * 100;
        pLink->pData->digits[1] = channel / 10;
        channel = channel - pLink->pData->digits[1] * 10;
        pLink->pData->digits[0] = channel;
        pLink->pData->range = 150;
        pLink->pData->ilsfreq = 0;
        pLink->pData->ttype = 1;

        pLink->type = TANKER;

        // mpMissionTacans = pLink;


        //add link (we already have at least one tacan, so just add another one
        // ShiAssert(prevLink); not needed anymore

        // M.N. we have no airbase - can be in IA; CTD Fix

        if ( not mpMissionTacans)
            mpMissionTacans = pLink;
        else
        {
            pLink->pPrevious = prevLink;
            mpMissionTacans->pNext = pLink;
        }
    }


    // Now we have a list of Homeplate->Alt->Tanker or Homeplate->Tanker or only Homeplate
    // mpCurrentTCN[ICP] = homeplate tacan channel in every case
    // old algorithm result: if Alternate base present, mpCurrentTCN[ICP] = altbase tacan

    if (mpMissionTacans)
    {
        mpCurrentMissionTacan = mpMissionTacans;
        CopyMissionTCNData(&mpCurrentTCN[ICP], mpCurrentMissionTacan);
        SetDomain(mpMissionTacans->type);
    }
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::CopyMissionTCNData
//---------------------------------------------------------------

void NavigationSystem::CopyMissionTCNData(Tacan_Data_Str* pData, Tacan_Data_LL_Str* pLink)
{

    pData->vuID = pLink->pData->vuID;
    pData->channel = pLink->pData->channel;
    pData->set = pLink->pData->set;
    pData->digits[0] = pLink->pData->digits[0];
    pData->digits[1] = pLink->pData->digits[1];
    pData->digits[2] = pLink->pData->digits[2];
    pData->range = pLink->pData->range;
    pData->ilsfreq = pLink->pData->ilsfreq;
    pData->ttype = pLink->pData->ttype;
}

/////////////////////////////////////////////////////////////////

//---------------------------------------------------------------
// NavigationSystem::GetType
//---------------------------------------------------------------

NavigationSystem::Type NavigationSystem::GetType(void)
{

    if (mpCurrentMissionTacan)
    {
        return mpCurrentMissionTacan->type;
    }
    else
    {
        return TOTAL_TYPES;
    }

}



//---------------------------------------------------------------
// NavigationSystem::StepPreviousTacan
//---------------------------------------------------------------

void NavigationSystem::StepPreviousTacan(void)
{

    Tacan_Data_LL_Str* p_current;

    if (mpCurrentMissionTacan and mpCurrentMissionTacan->pData->vuID not_eq FalconNullId)
    {
        if (mpCurrentMissionTacan->pPrevious == NULL)
        {

            p_current = mpMissionTacans;

            while (p_current->pNext not_eq NULL)
            {
                p_current = p_current->pNext;
            }

            mpCurrentMissionTacan = p_current;
        }
        else
        {
            mpCurrentMissionTacan = mpCurrentMissionTacan->pPrevious;
        }

        CopyMissionTCNData(&mpCurrentTCN[ICP], mpCurrentMissionTacan);
        SetDomain(mpCurrentMissionTacan->type);
    }
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::StepNextTacan
//---------------------------------------------------------------

void NavigationSystem::StepNextTacan(void)   // For ICP Only
{

    if (mpCurrentMissionTacan and mpCurrentMissionTacan->pData->vuID not_eq FalconNullId)
    {

        if (mpCurrentMissionTacan->pNext == NULL)
        {
            mpCurrentMissionTacan = mpMissionTacans;
        }
        else
        {
            mpCurrentMissionTacan = mpCurrentMissionTacan->pNext;
        }

        CopyMissionTCNData(&mpCurrentTCN[ICP], mpCurrentMissionTacan);
        SetDomain(mpCurrentMissionTacan->type);
    }
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::GetHomeID
//---------------------------------------------------------------

void NavigationSystem::GetHomeID(VU_ID* id)
{

    if (mpMissionTacans)
    {
        *id = mpMissionTacans->pData->vuID;
    }
    else
    {
        *id = FalconNullId;
    }
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::GetCurrentID
//---------------------------------------------------------------

void NavigationSystem::GetCurrentID(VU_ID* id)
{

    if (mpCurrentMissionTacan)
    {
        *id = mpCurrentMissionTacan->pData->vuID;
    }
    else
    {
        *id = FalconNullId;
    }
}

/////////////////////////////////////////////////////////////////

//---------------------------------------------------------------
// NavigationSystem::GetTCNPosition
//---------------------------------------------------------------

BOOL NavigationSystem::GetTCNPosition(float *xp, float *yp, float *zp)
{

    VuEntity* entity;

    entity = vuDatabase->Find(mpCurrentTCN[mCurrentTCNSrc].vuID);

    if (entity == NULL)
    {
        *xp = *yp = *zp = 0;
        return FALSE;
    }

    if (entity->EntityType()->classInfo_[VU_CLASS] == CLASS_UNIT and 
        entity->EntityType()->classInfo_[VU_TYPE] == TYPE_FLIGHT)
    {
        if (((FlightClass*) entity)->GetComponentLead())
        {
            entity = ((FlightClass*) entity)->GetComponentLead();
        }
    }

    // else if(entity->GetType() == STYPE_UNIT_CARRIER) {
    // }

    *xp = entity->XPos();
    *yp = entity->YPos();
    *zp = entity->ZPos();

    return TRUE;
}

//---------------------------------------------------------------
// NavigationSystem::GetTCNAttribute
//---------------------------------------------------------------

BOOL NavigationSystem::GetTCNAttribute(Attribute attribute, float* value)
{

    switch (attribute)
    {
        case RANGE:
            *value = static_cast<float>(mpCurrentTCN[mCurrentTCNSrc].range);
            return TRUE;

        case ILSFREQ:
            *value = mpCurrentTCN[mCurrentTCNSrc].ilsfreq;
            return TRUE;
    }

    float x, y, z;

    if (GetTCNPosition(&x, &y, &z) not_eq TRUE)
        return FALSE;

    switch (attribute)
    {
        case X_POS:
            *value = x;
            break;

        case Y_POS:
            *value = y;
            break;

        case Z_POS:
            *value = z;
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::ToggleControlSrc
//---------------------------------------------------------------

void NavigationSystem::ToggleControlSrc(void)
{

    if ( not g_bRealisticAvionics)
    {
        //MI Original code
        if (mCurrentTCNSrc == ICP)
        {
            mCurrentTCNSrc = AUXCOMM;
        }
        else
        {
            mCurrentTCNSrc = ICP;
        }
    }
    else
    {
        //MI modified for ICP stuff
        //These pages change when in Backup mode
        if (OTWDriver.pCockpitManager->mpIcp->CheckBackupPages())
            OTWDriver.pCockpitManager->mpIcp->ClearStrings();

        if (mCurrentTCNSrc == ICP)
            mCurrentTCNSrc = AUXCOMM;
        else
            mCurrentTCNSrc = ICP;

        // MD --20040605: and update ILS info since we may be changing TACAN completely
        /*gNavigationSys->*/
        SetIlsFromTacan();
    }
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::GetControlSrc
//---------------------------------------------------------------

NavigationSystem::Tacan_Channel_Src NavigationSystem::GetControlSrc(void)
{

    return mCurrentTCNSrc;

}

//------------------------------------------------------------------------
// NavigationSystem::SetControlSrc  // MD -- 20031120: for cockpit support
//------------------------------------------------------------------------

void NavigationSystem::SetControlSrc(Tacan_Channel_Src src)
{
    mCurrentTCNSrc = src;
    // MD --20040605: and update ILS info since we may be changing TACAN completely
    /*gNavigationSys->*/
    SetIlsFromTacan();
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::SetDomain
//---------------------------------------------------------------

void NavigationSystem::SetDomain(Type type)
{

    switch (type)
    {
        case AIRBASE:
            mpCurrentDomain[ICP] = TacanList::AG;
            break;

        case CARRIER:
            mpCurrentDomain[ICP] = TacanList::AG;
            break;

        case TANKER:
            mpCurrentDomain[ICP] = TacanList::AA;
            break;
    }

    // MD --20040605: and update ILS info since we may be changing TACAN completely
    /*gNavigationSys->*/SetIlsFromTacan();
}


// Control Switches and buttons
//---------------------------------------------------------------
// NavigationSystem::SetDomain
//---------------------------------------------------------------

void NavigationSystem::SetDomain(Tacan_Channel_Src src, TacanList::Domain domain)
{

    if (domain < 0 or domain >= TacanList::NumDomains)
    {
        ShiWarning("Bad Nav Domain");
        return;
    }

    VU_ID vuID;

    mpCurrentDomain[src] = domain;
    FindTacanStation(src, mpCurrentTCN[src].channel, mpCurrentTCN[src].set,
                     &vuID, &mpCurrentTCN[src].range, &mpCurrentTCN[src].ttype, &mpCurrentTCN[src].ilsfreq);
    mpCurrentTCN[src].vuID = vuID;
    // MD --20040605: and update ILS info since we may be changing TACAN completely
    /*gNavigationSys->*/
    SetIlsFromTacan();
}

////////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::GetDomain
//---------------------------------------------------------------

TacanList::Domain NavigationSystem::GetDomain(Tacan_Channel_Src src)
{

    return mpCurrentDomain[src];
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::ToggleDomain
//---------------------------------------------------------------
TacanList::Domain NavigationSystem::ToggleDomain(Tacan_Channel_Src src)
{

    VU_ID vuID;

    if (mpCurrentDomain[src] == TacanList::AA)
    {
        mpCurrentDomain[src] = TacanList::AG;
    }
    else
    {
        mpCurrentDomain[src] = TacanList::AA;
    }

    FindTacanStation(src, mpCurrentTCN[src].channel, mpCurrentTCN[src].set,
                     &vuID, &mpCurrentTCN[src].range, &mpCurrentTCN[src].ttype, &mpCurrentTCN[src].ilsfreq);
    mpCurrentTCN[src].vuID = vuID;

    return mpCurrentDomain[src];
}

/////////////////////////////////////////////////////////////////



// Overall Instrument Mode
//---------------------------------------------------------------
// NavigationSystem::SetInstrumentMode
//---------------------------------------------------------------

void NavigationSystem::SetInstrumentMode(Instrument_Mode mode)
{

    if (mode < 0 or mode >= TOTAL_MODES)
    {
        ShiWarning("Bad NAV Mode");
        return;
    }

    mInstrumentMode = mode;
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::GetInstrumentMode
//---------------------------------------------------------------

NavigationSystem::Instrument_Mode NavigationSystem::GetInstrumentMode(void)
{

    return mInstrumentMode;

}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::StepInstrumentMode
//---------------------------------------------------------------

void NavigationSystem::StepInstrumentMode(void)
{


    //MI additions
    if (g_bRealisticAvionics and g_bINS and SimDriver.GetPlayerAircraft())
    {
        SimDriver.GetPlayerAircraft()->LOCValid = TRUE; //Flag not visible
        SimDriver.GetPlayerAircraft()->GSValid = TRUE; //Flag not visible
    }

    if (mInstrumentMode == TACAN)
    {
        mInstrumentMode = NAV;
    }
    else
    {
        if ( not g_bRealisticAvionics)
        {
            //MI original code
            switch (mInstrumentMode)
            {
                case NAV:
                    mInstrumentMode = ILS_NAV;
                    break;

                case ILS_NAV:
                    mInstrumentMode = ILS_TACAN;
                    break;

                case ILS_TACAN:
                    mInstrumentMode = TACAN;
                    break;

                case TACAN:
                    mInstrumentMode = NAV;
                    break;
            }
        }
        else
        {
            //MI modified for ICP stuff
            switch (mInstrumentMode)
            {
                case NAV:
                    mInstrumentMode = ILS_NAV;
                    SimDriver.GetPlayerAircraft()->FCC->SetMasterMode(FireControlComputer::ILS);
                    break;

                case ILS_NAV:
                    mInstrumentMode = ILS_TACAN;
                    SimDriver.GetPlayerAircraft()->FCC->SetMasterMode(FireControlComputer::ILS);
                    break;

                case ILS_TACAN:
                    mInstrumentMode = TACAN;
                    SimDriver.GetPlayerAircraft()->FCC->SetMasterMode(FireControlComputer::Nav);
                    break;

                case TACAN:
                    mInstrumentMode = NAV;
                    SimDriver.GetPlayerAircraft()->FCC->SetMasterMode(FireControlComputer::Nav);
                    break;
            }

        }
    }
}

/////////////////////////////////////////////////////////////////


// Mark and DLink Functions

//---------------------------------------------------------------
// NavigationSystem::GetMarkIndex
//---------------------------------------------------------------

int NavigationSystem::GetMarkIndex(void)
{

    return mCurrentMark;
}

//MI OA Stuff
//---------------------------------------------------------------
// NavigationSystem::GetDESTOA
//---------------------------------------------------------------

int NavigationSystem::GetDESTOAIndex(void)
{
    return mCurrentDESTOA;
}

//---------------------------------------------------------------
// NavigationSystem::GetVIPOA
//---------------------------------------------------------------
int NavigationSystem::GetVIPOAIndex(void)
{
    return mCurrentVIPOA;
}

//---------------------------------------------------------------
// NavigationSystem::GetVRPOA
//---------------------------------------------------------------
int NavigationSystem::GetVRPOAIndex(void)
{
    return mCurrentVRPOA;
}

//---------------------------------------------------------------
// NavigationSystem::GetMarkPoint
//---------------------------------------------------------------

void NavigationSystem::GetMarkPoint(Point_Type* ppointType, char* platStr, char* plongStr)
{

    *platStr = NULL;
    *plongStr = NULL;

    *ppointType = mpMarkPoints[mCurrentMark].pointType;

    strcpy(platStr, mpMarkPoints[mCurrentMark].pLatStr);
    strcpy(plongStr, mpMarkPoints[mCurrentMark].pLongStr);
}

//MI OA Stuff
//---------------------------------------------------------------
// NavigationSystem::GetDESTOA
//---------------------------------------------------------------
void NavigationSystem::GetDESTOA(Point_Type* ppointType, char* platStr, char* plongStr)
{
    *platStr = NULL;
    *plongStr = NULL;

    *ppointType = mpDESTOA[mCurrentDESTOA].pointType;

    strcpy(platStr, mpDESTOA[mCurrentDESTOA].pLatStr);
    strcpy(plongStr, mpDESTOA[mCurrentDESTOA].pLongStr);
}

//---------------------------------------------------------------
// NavigationSystem::GetVIPOA
//---------------------------------------------------------------
void NavigationSystem::GetVIPOA(Point_Type* ppointType, char* platStr, char* plongStr)
{
    *platStr = NULL;
    *plongStr = NULL;

    *ppointType = mpVIPOA[mCurrentVIPOA].pointType;

    strcpy(platStr, mpVIPOA[mCurrentVIPOA].pLatStr);
    strcpy(plongStr, mpVIPOA[mCurrentVIPOA].pLongStr);
}

//---------------------------------------------------------------
// NavigationSystem::GetVRPOA
//---------------------------------------------------------------
void NavigationSystem::GetVRPOA(Point_Type* ppointType, char* platStr, char* plongStr)
{
    *platStr = NULL;
    *plongStr = NULL;

    *ppointType = mpVRPOA[mCurrentVRPOA].pointType;

    strcpy(platStr, mpVIPOA[mCurrentVRPOA].pLatStr);
    strcpy(plongStr, mpVIPOA[mCurrentVRPOA].pLongStr);
}


/////////////////////////////////////////////////////////////////

//---------------------------------------------------------------
// NavigationSystem::GetMarkWayPoint
//---------------------------------------------------------------

void NavigationSystem::GetMarkWayPoint(int index, WayPointClass** ppwayPoint)
{
    if (mpMarkPoints[index].pWaypoint)
        *ppwayPoint = mpMarkPoints[index].pWaypoint;
    else
        *ppwayPoint = NULL;
}

//MI OA Stuff
//---------------------------------------------------------------
// NavigationSystem::GetDESTOAPoint
//---------------------------------------------------------------
void NavigationSystem::GetDESTOAPoint(int index, WayPointClass** ppwayPoint)
{
    if (mpDESTOA[index].pWaypoint)
        *ppwayPoint = mpDESTOA[index].pWaypoint;
    else
        *ppwayPoint = NULL;
}

//---------------------------------------------------------------
// NavigationSystem::GetVRPOAPoint
//---------------------------------------------------------------
void NavigationSystem::GetVIPOAPoint(int index, WayPointClass** ppwayPoint)
{
    if (mpVIPOA[index].pWaypoint)
        *ppwayPoint = mpVIPOA[index].pWaypoint;
    else
        *ppwayPoint = NULL;
}

//---------------------------------------------------------------
// NavigationSystem::GetVRPOAPoint
//---------------------------------------------------------------
void NavigationSystem::GetVRPOAPoint(int index, WayPointClass** ppwayPoint)
{
    if (mpVRPOA[index].pWaypoint)
        *ppwayPoint = mpVRPOA[index].pWaypoint;
    else
        *ppwayPoint = NULL;
}


//---------------------------------------------------------------
// NavigationSystem::GetMarkWayPoint
//---------------------------------------------------------------

void NavigationSystem::GetMarkWayPoint(WayPointClass** ppwayPoint)
{

    if (mpMarkPoints[mCurrentMark].pWaypoint)
        *ppwayPoint = mpMarkPoints[mCurrentMark].pWaypoint;
    else
        *ppwayPoint = NULL;
}

//MI OA stuff
//---------------------------------------------------------------
// NavigationSystem::GetDESTOAPoint
//---------------------------------------------------------------
void NavigationSystem::GetDESTOAPoint(WayPointClass** ppwayPoint)
{
    if (mpDESTOA[mCurrentMark].pWaypoint)
        *ppwayPoint = mpDESTOA[mCurrentMark].pWaypoint;
    else
        *ppwayPoint = NULL;
}

//---------------------------------------------------------------
// NavigationSystem::GetVIPOAPoint
//---------------------------------------------------------------
void NavigationSystem::GetVIPOAPoint(WayPointClass** ppwayPoint)
{
    if (mpVIPOA[mCurrentMark].pWaypoint)
        *ppwayPoint = mpVIPOA[mCurrentMark].pWaypoint;
    else
        *ppwayPoint = NULL;
}

//---------------------------------------------------------------
// NavigationSystem::GetVRPOAPoint
//---------------------------------------------------------------
void NavigationSystem::GetVRPOAPoint(WayPointClass** ppwayPoint)
{
    if (mpVRPOA[mCurrentMark].pWaypoint)
        *ppwayPoint = mpVRPOA[mCurrentMark].pWaypoint;
    else
        *ppwayPoint = NULL;
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::SetMarkPoint
//---------------------------------------------------------------

void NavigationSystem::SetMarkPoint(Point_Type type, float x, float y, float z, long arriveTime)
{

    float latitude;
    float longitude;

    if (mpMarkPoints[mCurrentMark].pWaypoint)
    {

        delete mpMarkPoints[mCurrentMark].pWaypoint;
    }

    mpMarkPoints[mCurrentMark].pWaypoint = new WayPointClass(0, 0, 0, 0, 0, 0, 0, 0);
    mpMarkPoints[mCurrentMark].pWaypoint->SetLocation(x, y, z);
    mpMarkPoints[mCurrentMark].pWaypoint->SetWPArrive(arriveTime);


    mpMarkPoints[mCurrentMark].pointType = type;

    ApproxLatLong(x, y, &latitude, &longitude);
    BuildLatLongStr(latitude, longitude, mpMarkPoints[mCurrentMark].pLatStr, mpMarkPoints[mCurrentMark].pLongStr);
    SimDriver.GetPlayerAircraft()->FCC->waypointStepCmd = 127;
}

//MI OA Stuff
//---------------------------------------------------------------
// NavigationSystem::SetDESTOAPoint
//---------------------------------------------------------------
void NavigationSystem::SetDESTOAPoint(Point_Type type, float x, float y, float z, int number)
{
    float latitude;
    float longitude;

    if (mpDESTOA[number].pWaypoint)
        delete mpDESTOA[number].pWaypoint;

    mpDESTOA[number].pWaypoint = new WayPointClass(0, 0, 0, 0, 0, 0, 0, 0);
    mpDESTOA[number].pWaypoint->SetLocation(x, y, z);

    mpDESTOA[number].pointType = type;

    ApproxLatLong(x, y, &latitude, &longitude);
    BuildLatLongStr(latitude, longitude, mpDESTOA[number].pLatStr, mpDESTOA[number].pLongStr);
    SimDriver.GetPlayerAircraft()->FCC->waypointStepCmd = 127;
}

//---------------------------------------------------------------
// NavigationSystem::SetVIPOAPoint
//---------------------------------------------------------------
void NavigationSystem::SetVIPOAPoint(Point_Type type, float x, float y, float z, long arriveTime)
{
    float latitude;
    float longitude;

    if (mpVIPOA[mCurrentVIPOA].pWaypoint)
        delete mpVIPOA[mCurrentVIPOA].pWaypoint;

    mpVIPOA[mCurrentVIPOA].pWaypoint = new WayPointClass(0, 0, 0, 0, 0, 0, 0, 0);
    mpVIPOA[mCurrentVIPOA].pWaypoint->SetLocation(x, y, z);
    //Needed?
    //mpVIPOA[mCurrentVIPOA].pWaypoint->SetWPArrive(arriveTime);


    mpVIPOA[mCurrentVIPOA].pointType = type;

    ApproxLatLong(x, y, &latitude, &longitude);
    BuildLatLongStr(latitude, longitude, mpVIPOA[mCurrentVIPOA].pLatStr, mpVIPOA[mCurrentVIPOA].pLongStr);
    SimDriver.GetPlayerAircraft()->FCC->waypointStepCmd = 127;
}

//---------------------------------------------------------------
// NavigationSystem::SetVRPOAPoint
//---------------------------------------------------------------
void NavigationSystem::SetVRPOAPoint(Point_Type type, float x, float y, float z, long arriveTime)
{
    float latitude;
    float longitude;

    if (mpVRPOA[mCurrentVRPOA].pWaypoint)
        delete mpVRPOA[mCurrentVRPOA].pWaypoint;

    mpVRPOA[mCurrentVRPOA].pWaypoint = new WayPointClass(0, 0, 0, 0, 0, 0, 0, 0);
    mpVRPOA[mCurrentVRPOA].pWaypoint->SetLocation(x, y, z);
    //Needed?
    //mpVRPOA[mCurrentVRPOA].pWaypoint->SetWPArrive(arriveTime);


    mpVRPOA[mCurrentVRPOA].pointType = type;

    ApproxLatLong(x, y, &latitude, &longitude);
    BuildLatLongStr(latitude, longitude, mpVRPOA[mCurrentVRPOA].pLatStr, mpVRPOA[mCurrentVRPOA].pLongStr);
    SimDriver.GetPlayerAircraft()->FCC->waypointStepCmd = 127;
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::GotoPrevMark
//---------------------------------------------------------------

void NavigationSystem::GotoPrevMark(void)
{

    if (mCurrentMark == 0)
        mCurrentMark = MAX_MARKPOINTS - 1;
    else
        mCurrentMark--;
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::GotoNextMark
//---------------------------------------------------------------

void NavigationSystem::GotoNextMark(void)
{

    if (mCurrentMark == MAX_MARKPOINTS - 1)
        mCurrentMark = 0;
    else
        mCurrentMark++;
}

/////////////////////////////////////////////////////////////////


//---------------------------------------------------------------
// NavigationSystem::GetDLinkWayPoint
//---------------------------------------------------------------

void NavigationSystem::GetDLinkWayPoint(int index, WayPointClass** ppwayPoint)
{

    if (mpDLinkPoints[index].pWaypoint)
        *ppwayPoint = mpDLinkPoints[index].pWaypoint;
    else
        *ppwayPoint = NULL;
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::GetDLinkWayPoint
//---------------------------------------------------------------

void NavigationSystem::GetDLinkWayPoint(WayPointClass** ppwayPoint)
{

    if (mpDLinkPoints[mCurrentDLink].pWaypoint)
        *ppwayPoint = mpDLinkPoints[mCurrentDLink].pWaypoint;
    else
        *ppwayPoint = NULL;
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::GetDataLink
//---------------------------------------------------------------

void NavigationSystem::GetDataLink(FalconDLinkMessage::DLinkPointType* ptype,
                                   int* ppointNumber,
                                   char* ptypeStr,
                                   char* ptarget,
                                   char* pthreat,
                                   char* pheading,
                                   char* pdistance)
{
    *ptype = mpDLinkPoints[mCurrentDLink].pointType;
    *ppointNumber = mCurrentDLink;

    if (*ptype == FalconDLinkMessage::NODLINK)
    {
        *ptarget = NULL;
        *pthreat = NULL;
        *ptypeStr = NULL;
        *pheading = NULL;
        *pdistance = NULL;
    }
    else
    {
        strcpy(ptarget, mpDLinkTarget);
        strcpy(pthreat, mpDLinkThreat);
        strcpy(ptypeStr, mpDLinkPoints[mCurrentDLink].pTypeStr);
        strcpy(pheading, mpDLinkPoints[mCurrentDLink].attackHeading);
        strcpy(pdistance, mpDLinkPoints[mCurrentDLink].distance);

    }
}

/////////////////////////////////////////////////////////////////


//---------------------------------------------------------------
// NavigationSystem::SetDataLinks
//---------------------------------------------------------------

void NavigationSystem::SetDataLinks(char totalPoints,
                                    USHORT target,
                                    USHORT threat,
                                    FalconDLinkMessage::DLinkPointType* ptype,
                                    short* px,
                                    short* py,
                                    short* pz,
                                    long* parriveTime)
{
    int i;
    float tgtx;
    float tgty;
    float tgtz;
    float ipx;
    float ipy;
    float ipz;
    float deltax;
    float deltay;
    int heading;
    float distance;

    GetTypeString(target, mpDLinkTarget);
    GetTypeString(threat, mpDLinkThreat);

    for (i = 0; i < totalPoints; i++)
    {

        mpDLinkPoints[i].pointType = ptype[i];
        strcpy(mpDLinkPoints[i].pTypeStr, DLink_Type_Str[ptype[i]]);

        if (mpDLinkPoints[i].pWaypoint)
        {
            delete mpDLinkPoints[i].pWaypoint;
        }

        mpDLinkPoints[i].pWaypoint = new WayPointClass;
        mpDLinkPoints[i].pWaypoint->SetLocation(GridToSim(px[i]),
                                                GridToSim(py[i]),
                                                -(float)pz[i] * GRIDZ_SCALE_FACTOR);

        mpDLinkPoints[i].pWaypoint->SetWPArrive(parriveTime[i]);


        if (i and mpDLinkPoints[i].pointType == FalconDLinkMessage::TGT and mpDLinkPoints[i - 1].pointType == FalconDLinkMessage::IP)
        {

            mpDLinkPoints[i].pWaypoint->GetLocation(&tgtx, &tgty, &tgtz);
            mpDLinkPoints[i - 1].pWaypoint->GetLocation(&ipx, &ipy, &ipz);

            deltax = tgtx - ipx;
            deltay = tgty - ipy;

            heading = FloatToInt32(ConvertRadtoNav((float)atan2(deltax, deltay)));

            if (heading == 0)
            {
                heading = 360;
            }

            distance = (float)sqrt(deltax * deltax + deltay + deltay) * FT_TO_NM;

            sprintf(mpDLinkPoints[i].attackHeading, "%3d", heading);

            if (mpDLinkPoints[i].attackHeading[0] == ' ')
            {
                mpDLinkPoints[i].attackHeading[0] = '0';
            }

            strcpy(mpDLinkPoints[i - 1].attackHeading, mpDLinkPoints[i].attackHeading);

            sprintf(mpDLinkPoints[i].distance, "%2.1f", distance);
            strcpy(mpDLinkPoints[i - 1].distance, mpDLinkPoints[i].distance);
        }
        else
        {

            *(mpDLinkPoints[i].attackHeading) = NULL;
            *(mpDLinkPoints[i].distance) = NULL;
        }
    }

    for (i = totalPoints; i < MAX_DLINKPOINTS; i++)
    {
        mpDLinkPoints[i].pointType = FalconDLinkMessage::NODLINK;
    }

    SimDriver.GetPlayerAircraft()->FCC->waypointStepCmd = 127;
}

/////////////////////////////////////////////////////////////////


int NavigationSystem::GetDLinkIndex(void)
{
    return mCurrentDLink;
}



//---------------------------------------------------------------
// NavigationSystem::GotoPrevDLink
//---------------------------------------------------------------

void NavigationSystem::GotoPrevDLink(void)
{

    if (mCurrentDLink == 0)
        mCurrentDLink = MAX_DLINKPOINTS - 1;
    else
        mCurrentDLink--;
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::GotoNextDLink
//---------------------------------------------------------------

void NavigationSystem::GotoNextDLink(void)
{

    if (mCurrentDLink == MAX_DLINKPOINTS - 1)
        mCurrentDLink = 0;
    else
        mCurrentDLink++;
}

/////////////////////////////////////////////////////////////////




// Tacan Station Functions
//---------------------------------------------------------------
// NavigationSystem::SetTacanChannel
//---------------------------------------------------------------

void NavigationSystem::SetTacanChannel(Tacan_Channel_Src src, int digit, int value)
{

    if (digit < 0 or digit > 2)
    {
        // Tacan Channels have between one and three digits.
        ShiWarning("Too many digits"); // Element #2 = MSDigit, Element #0 = LSDigit
        return;
    }

    if (digit == 2 and (value < 0 or value > 1))
    {
        // Element #2 can only take values of 0 or 1
        ShiWarning("Bad TACAN Number");
        return;
    }

    VU_ID vuID;

    mpCurrentTCN[src].digits[digit] = value; // Set the element to the appropiate value
    mpCurrentTCN[src].channel = mpCurrentTCN[src].digits[2] * 100 + // Set the update the channel in the struct
                                            mpCurrentTCN[src].digits[1] * 10 +
                                            mpCurrentTCN[src].digits[0];

    FindTacanStation(src, mpCurrentTCN[src].channel, mpCurrentTCN[src].set,
                     &vuID, &mpCurrentTCN[src].range, &mpCurrentTCN[src].ttype, &mpCurrentTCN[src].ilsfreq);
    mpCurrentTCN[src].vuID = vuID;
    // MD --20040605: and update ILS info since we may be changing TACAN completely
    /*gNavigationSys->*/
    SetIlsFromTacan();
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::FindTacanStation
//---------------------------------------------------------------

void NavigationSystem::FindTacanStation(Tacan_Channel_Src src,
                                        int channel,
                                        TacanList::StationSet set,
                                        VU_ID *id,
                                        int *rangep,
                                        int *type,
                                        float *ilsfreq
                                       )
{
    *rangep = 0;
    *type = 0;
    *ilsfreq = 0;

    if (set == TacanList::Y and mpCurrentDomain[src] == TacanList::AA)   // this only works for tankers now
    {

        FlightClass* p_flight;
        BOOL result = FALSE;

        VuListIterator findWalker(SimDriver.tankerList);
        *id = FalconNullId;
        p_flight = (FlightClass*)findWalker.GetFirst();

        while (p_flight and result == FALSE)
        {

            if (((int)p_flight->tacan_channel) == channel)
            {
                *id = p_flight->Id();
                result = TRUE;
                *rangep = 150;
                *type = 1;
                *ilsfreq = 0;
            }
            else
            {
                p_flight = (FlightClass*)findWalker.GetNext();
            }
        }

    }
    else
    {
        if ( not gTacanList->GetVUIDFromChannel(channel, set, mpCurrentDomain[src],
                                            id, rangep, type, ilsfreq))
        {
            *id = FalconNullId;
        }
    }
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::GetTacanChannel
//---------------------------------------------------------------

int NavigationSystem::GetTacanChannel(Tacan_Channel_Src src, int digit)
{

    return mpCurrentTCN[src].digits[digit];
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::SetTacanChannel
//---------------------------------------------------------------

void NavigationSystem::SetTacanChannel(Tacan_Channel_Src src, int channel)
{


    if (channel < 1 or channel > 126)   // Tacan Channels are numbered 1 to 126
    {
        ShiWarning("Bad TACAN Channel");
        return;
    }

    VU_ID vuID;

    mpCurrentTCN[src].channel = channel; // Set the new channel

    // Break up into digits
    mpCurrentTCN[src].digits[2] = channel / 100;
    channel = channel - mpCurrentTCN[src].digits[2] * 100;
    mpCurrentTCN[src].digits[1] = channel / 10;
    channel = channel - mpCurrentTCN[src].digits[1] * 10;
    mpCurrentTCN[src].digits[0] = channel;

    FindTacanStation(src, mpCurrentTCN[src].channel, mpCurrentTCN[src].set,
                     &vuID, &mpCurrentTCN[src].range, &mpCurrentTCN[src].ttype, &mpCurrentTCN[src].ilsfreq);
    mpCurrentTCN[src].vuID = vuID;
    // MD --20040605: and update ILS info since we may be changing TACAN completely
    /*gNavigationSys->*/
    SetIlsFromTacan();
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::GetTacanChannel
//---------------------------------------------------------------

int NavigationSystem::GetTacanChannel(Tacan_Channel_Src src)
{

    return mpCurrentTCN[src].channel;

}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::SetTacanBand
//---------------------------------------------------------------

void NavigationSystem::SetTacanBand(Tacan_Channel_Src src, TacanList::StationSet set)
{

    if (set not_eq TacanList::X and set not_eq TacanList::Y)
    {
        ShiWarning("Bad TACAN Band");
        return;
    }

    VU_ID vuID;

    mpCurrentTCN[src].set = set;
    FindTacanStation(src, mpCurrentTCN[src].channel, mpCurrentTCN[src].set,
                     &vuID, &mpCurrentTCN[src].range, &mpCurrentTCN[src].ttype, &mpCurrentTCN[src].ilsfreq);
    mpCurrentTCN[src].vuID = vuID;
    // MD --20040605: and update ILS info since we may be changing TACAN completely
    /*gNavigationSys->*/
    SetIlsFromTacan();
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::GetTacanBand
//---------------------------------------------------------------

TacanList::StationSet NavigationSystem::GetTacanBand(Tacan_Channel_Src src)
{

    return mpCurrentTCN[src].set;

}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::SetTacanChannel
//---------------------------------------------------------------

void NavigationSystem::SetTacanChannel(Tacan_Channel_Src src, int channel, TacanList::StationSet set)
{

    if (channel < 1 or channel > 126)   // Tacan Channels are numbered 1 to 126
    {
        ShiWarning("Bad TACAN Number");
        return;
    }

    if (set not_eq TacanList::X and set not_eq TacanList::Y)
    {
        ShiWarning("Bad TACAN Band");
        return;
    }

    VU_ID vuID;

    mpCurrentTCN[src].channel = channel; // Set the new channel

    // Break up into digits
    mpCurrentTCN[src].digits[2] = channel / 100;
    channel = channel - mpCurrentTCN[src].digits[2] * 100;
    mpCurrentTCN[src].digits[1] = channel / 10;
    channel = channel - mpCurrentTCN[src].digits[1] * 10;
    mpCurrentTCN[src].digits[0] = channel;

    mpCurrentTCN[src].set = set;

    FindTacanStation(src, mpCurrentTCN[src].channel, mpCurrentTCN[src].set,
                     &vuID, &mpCurrentTCN[src].range, &mpCurrentTCN[src].ttype, &mpCurrentTCN[src].ilsfreq);

    mpCurrentTCN[src].vuID = vuID;
    // MD --20040605: and update ILS info since we may be changing TACAN completely
    // sfr: wooooa, we are creating gNavigationSys here, how can we use it???
    /*gNavigationSys->*/
    SetIlsFromTacan();
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::GetTacanChannel
//---------------------------------------------------------------

void NavigationSystem::GetTacanChannel(Tacan_Channel_Src src, int* channel, TacanList::StationSet* set)
{

    *channel = mpCurrentTCN[src].channel;
    *set = mpCurrentTCN[src].set;
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::StepTacanChannelDigit
//---------------------------------------------------------------

void NavigationSystem::StepTacanChannelDigit(Tacan_Channel_Src src, int digit, int direction)
{

    if (digit < 0 or digit > 2)
    {
        // Tacan Channels have between one and three digits.
        // Element #2 = MSDigit, Element #0 = LSDigit
        ShiWarning("Bad TACAN Step");
        return;
    }

    VU_ID vuID;

    //JPO rewrite to step in either direction.
    // increment/decrement first, ask questions later.
    mpCurrentTCN[src].digits[digit] += direction; // Set the element to the appropiate value

    if (digit == 2)  // can only be 0 or 1.
    {
        if (mpCurrentTCN[src].digits[digit] < 0) mpCurrentTCN[src].digits[digit] = 1;

        if (mpCurrentTCN[src].digits[digit] > 1) mpCurrentTCN[src].digits[digit] = 0;
    }
    else
    {
        if (mpCurrentTCN[src].digits[digit] < 0) mpCurrentTCN[src].digits[digit] = 9;

        if (mpCurrentTCN[src].digits[digit] > 9) mpCurrentTCN[src].digits[digit] = 0;

    }

    mpCurrentTCN[src].channel = mpCurrentTCN[src].digits[2] * 100 + // Set the update the channel in the struct
                                            mpCurrentTCN[src].digits[1] * 10 +
                                            mpCurrentTCN[src].digits[0];

    FindTacanStation(src, mpCurrentTCN[src].channel, mpCurrentTCN[src].set,
                     &vuID, &mpCurrentTCN[src].range, &mpCurrentTCN[src].ttype, &mpCurrentTCN[src].ilsfreq);
    mpCurrentTCN[src].vuID = vuID;
    // MD --20040605: and update ILS info since we may be changing TACAN completely
    /*gNavigationSys->*/
    SetIlsFromTacan();
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::StepTacanBand
//---------------------------------------------------------------

void NavigationSystem::StepTacanBand(Tacan_Channel_Src src)
{

    VU_ID vuID;

    if (mpCurrentTCN[src].set == TacanList::X)
    {
        mpCurrentTCN[src].set = TacanList::Y;
    }
    else
    {
        mpCurrentTCN[src].set = TacanList::X;
    }

    FindTacanStation(src, mpCurrentTCN[src].channel, mpCurrentTCN[src].set,
                     &vuID, &mpCurrentTCN[src].range, &mpCurrentTCN[src].ttype, &mpCurrentTCN[src].ilsfreq);
    mpCurrentTCN[src].vuID = vuID;
    // MD --20040605: and update ILS info since we may be changing TACAN completely
    /*gNavigationSys->*/
    SetIlsFromTacan();
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::GetTacanCampID
//---------------------------------------------------------------

// For getting the Id of the tacan object
void NavigationSystem::GetTacanVUID(Tacan_Channel_Src src, VU_ID* p_vuID)
{

    if (src == ICP)
    {
        *p_vuID = mpCurrentTCN[0].vuID;
    }
    else
    {
        *p_vuID = mpCurrentTCN[1].vuID;
    }
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::ToggleUHFSrc
//---------------------------------------------------------------
void NavigationSystem::ToggleUHFSrc(void)
{

    if (mUHFMode == UHF_NORM)
    {
        mUHFMode = UHF_BACKUP;
    }
    else
    {
        mUHFMode = UHF_NORM;
    }
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// NavigationSystem::GetUHFSrc
//---------------------------------------------------------------
NavigationSystem::UHF_Mode_Type NavigationSystem::GetUHFSrc(void)
{
    return mUHFMode;
}

/////////////////////////////////////////////////////////////////

//------------------------------------------------------------------
// NavigationSystem::SetUHFSrc  // MD -- 20031121: for cockpit stuff
//------------------------------------------------------------------
void NavigationSystem::SetUHFSrc(NavigationSystem::UHF_Mode_Type mode)
{
    mUHFMode = mode;
}

/////////////////////////////////////////////////////////////////

//---------------------------------------------------------------
// NavigationSystem::GetAirbase
//---------------------------------------------------------------
void NavigationSystem::GetAirbase(VU_ID* pATCId)
{
    WayPointClass *pcurrentWaypoint;
    Objective airbase;
    GridIndex x, y;
    vector pos;

    // 2002-04-08 MN CTD fix
    if ( not SimDriver.GetPlayerAircraft())
    {
        *pATCId = FalconNullId;
        return;
    }

    if (GetInstrumentMode() == TACAN or GetInstrumentMode() == ILS_TACAN)
    {
        GetTacanVUID(GetControlSrc(), pATCId);
    }
    else if (GetInstrumentMode() == NAV or GetInstrumentMode() == ILS_NAV)
    {
        pcurrentWaypoint = SimDriver.GetPlayerAircraft()->curWaypoint;

        if (pcurrentWaypoint and pcurrentWaypoint->GetWPAction() == WP_LAND)
        {
            *pATCId = pcurrentWaypoint->GetWPTargetID();
        }
        else if (SimDriver.GetPlayerAircraft())
        {
            pos.x = SimDriver.GetPlayerAircraft()->XPos();
            pos.y = SimDriver.GetPlayerAircraft()->YPos();

            ConvertSimToGrid(&pos, &x, &y);
            airbase = FindNearestFriendlyAirbase(SimDriver.GetPlayerAircraft()->GetTeam(), x, y);

            if (airbase)
                *pATCId = airbase->Id();
            else
                *pATCId = FalconNullId;
        }
    }
    else
    {
        *pATCId = FalconNullId;
    }
}


//---------------------------------------------------------------
// NavigationSystem::IsTCNTanker
//---------------------------------------------------------------
BOOL NavigationSystem::IsTCNTanker(void)
{
    VU_ID VuId;
    CampBaseClass* pCampBase;
    FlightClass* pFlight;

    GetTacanVUID(GetControlSrc(), &VuId);
    pCampBase = (CampBaseClass*) vuDatabase->Find(VuId);

    if (pCampBase and pCampBase->EntityType()->classInfo_[VU_CLASS] == CLASS_UNIT and pCampBase->EntityType()->classInfo_[VU_TYPE] == TYPE_FLIGHT)
    {
        pFlight = (FlightClass*) pCampBase;

        if (pFlight->GetUnitMission() == AMIS_TANKER)
        {
            return TRUE;
        }

        return FALSE;
    }

    return FALSE;
}

//---------------------------------------------------------------
// NavigationSystem::IsTCNCarrier
//---------------------------------------------------------------

BOOL NavigationSystem::IsTCNCarrier(void)
{
    VU_ID VuId;
    CampBaseClass* pCampBase;

    GetTacanVUID(GetControlSrc(), &VuId);
    pCampBase = (CampBaseClass*) vuDatabase->Find(VuId);

    if (pCampBase and pCampBase->EntityType()->classInfo_[VU_CLASS] == CLASS_UNIT and pCampBase->EntityType()->classInfo_[VU_TYPE] == TYPE_TASKFORCE and pCampBase->EntityType()->classInfo_[VU_STYPE] == STYPE_UNIT_CARRIER)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//---------------------------------------------------------------
// NavigationSystem::IsTCNAirbase
//---------------------------------------------------------------

BOOL NavigationSystem::IsTCNAirbase(void)
{
    VU_ID VuId;
    CampBaseClass* pCampBase;

    GetTacanVUID(GetControlSrc(), &VuId);
    pCampBase = (CampBaseClass*) vuDatabase->Find(VuId);

    if (pCampBase and pCampBase->IsObjective() and pCampBase->EntityType()->classInfo_[VU_TYPE] == TYPE_AIRBASE)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//MI
void NavigationSystem::GetILSData(float *LocDev, float *finalHeading, float *finalGS, float *DistToSta)
{
    //current Localizer deviation
    *LocDev = mpCurrentIls.gpDeviation;

    //Get the current Glidepath deviation
    *finalGS = mpCurrentIls.gsDeviation;

    //runway heading
    *finalHeading = (float)mpCurrentIls.heading;

    //Get the distance
    *DistToSta = distToToLocalizer;
}

// MD -- 20040604: Adding a new function to set the ILS data based
// on the TACAN channel selection.  Previously, the ILS info for
// a player's aircraft was set as a side effect of air traffic related
// radio calls.  This function will be used instead to set the current
// ILS selection based on setting or sampling the TACAN channel.  This
// should mean that all player aircraft get proper ILS steering and that
// ILS steering cues kick in even if you don't call ATC to report
// inbound for landing.
void NavigationSystem::SetIlsFromTacan()
{
    VU_ID ATCId = FalconNullId;
    ObjectiveClass *atc;
    int rwindex = 0;
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (playerAC and playerAC->IsSetFlag(MOTION_OWNSHIP))
    {
        // Find the VU_ID for the entity that the TACAN channel represents
        GetTacanVUID(GetControlSrc(), &ATCId);

        if (ATCId not_eq FalconNullId)
        {
            // use the VU_ID to get the pointer to the airbase
            atc = (ObjectiveClass*)vuDatabase->Find(ATCId);

            // is the objective selected actually an airbase??
            if (atc and atc->IsObjective() and atc->EntityType()->classInfo_[VU_TYPE] == TYPE_AIRBASE)
            {
                // if so, figure out which runway player should use at selected airbase
                rwindex = atc->brain->FindBestLandingRunway(playerAC, FALSE);

                // sfr: only do if rwindex is valid
                if (rwindex not_eq 0)
                {
                    // and set the ILS data
                    /*gNavigationSys->*/SetIlsData(ATCId, rwindex);
                }
            }
        }
    }
}
