/*===============================================================

TACAN.CPP

  Written by: Vincent Finley
  For: Microprose Inc.

 Revision History:
 Created: 1/1/98
 Rev A: 1/7/98 List can now be searched and sorted by campId or channel
 Rev B: 1/16/98 Tankers and Carriers added.  Tacan list is now sorted by VU_ID.
 Rev C: 10/23/98 Added code to dynamically assign a tacan channel to Tankers and Carriers.
 Functions added: InitDynamicChans, CleanupDynamicChans, AssignChannel,
   RetireChannel.
 Functions moded: AddTacan, RemoveTacan, GetVUIDFromChannel

 Function List:
 TacanList::TacanList()
 TacanList::~TacanList()
 TacanList::AddTacan()
 TacanList::RemoveTacan()
 TacanList::GetChannelFromVUID()
 TacanList::GetVUIDFromChannel()
 TacanList::GetPointerFromVUID()
 TacanList::InsertIntoTacanList()
 TacanList::ResolveStationList()
 TacanList::StoreStation()
 TacanList::GetChannelFromCampID()
 TacanList::InitDynamicChans()
 TacanList::CleanupDynamicChans()
 TacanList::AssignChannel()
 TacanList::RetireChannel()
 SearchForChannel()

 Description:
 This file implements the TacanList Class.  It provides a
 mechanism for storing an searching tacan channel information.

=================================================================*/



#include <windows.h>
#include <stdio.h>

#include "vu2.h"
#include "F4Thread.h"
#include "campbase.h"
#include "f4error.h"
#include "tacan.h"
#include "cpres.h"
#include "classtbl.h"
#include "unit.h"
#include "find.h"
#include "flight.h"
#include "simdrive.h"
#include "aircrft.h"
#include "navsystem.h"
#include "Navunit.h"

//---------------------------------------------------------------
// External Constant Initialization
//---------------------------------------------------------------

TacanList* gTacanList;
const char* gpTacanFileName = "sim\\sigdata\\tacan\\stations.dat";

int CompareCampIDs(void**, void**);

#ifdef USE_SH_POOLS
MEM_POOL gTacanMemPool;
#endif


//---------------------------------------------------------------
// TacanList::TacanList();
//
// Okay, so say for instance you have a group of airbases.  It
// would be nice to put them in a list and sort them according to
// tacan station.  Then you can use this information to navigate
// to the nearest airbase.
//
// This constructor reads in a file that associates the CampaignId
// number of an airbase with a tacan channel and band.  It then
// creates a structure to hold this information and then inserts
// it into a sorted linked list.  Then the linked list is copied
// into a sorted array so that we can perform binary searches
// upon it.
//---------------------------------------------------------------

TacanList::TacanList()
{

    LinkedCampStationStr *p_stations = NULL;
    FILE *p_File = NULL;
    BOOL done = FALSE;
    StationSet band = X;
    char bandChar = 'x';
    int channel = 106;
    int stationId = 0;
    int result = 0;
    int callsign = 0;
    int range, tactype;
    float ilsfreq;
    char buffer[1024];

    mpTList = NULL;
    mCampListTally = 0;
    p_File = OpenCampFile("stations", "dat", "r");
    // p_File = CP_OPEN(gpTacanFileName, "r");
    F4Assert(p_File);

    // Error: Couldn't open file
    if (p_File == NULL) done = TRUE;

    while (done == FALSE)
    {
        if (fgets(buffer, sizeof buffer, p_File) == NULL)
        {
            done = TRUE;
            break;
        }

        // check for comments.
        if (buffer[0] == ';' or buffer[0] == '#' or buffer[0] == '\n')
            continue;

        result = sscanf(buffer, "%d %d %c %d %d %d %f",
                        &stationId, &channel, &bandChar, &callsign,
                        &range, &tactype, &ilsfreq);

        F4Assert(result >= NUM_TACAN_FIELDS); // Four Fields should be read in or EOF (-1)

        if (result < NUM_TACAN_FIELDS)
            continue;

        switch (result)  // fill in missing bits
        {
            case NUM_TACAN_FIELDS:
                range = 150;

                // fall
            case NUM_TACAN_FIELDS+1:
                tactype = 1;

                // fall
            case NUM_TACAN_FIELDS+2:
                ilsfreq = 111.1f;
                break;
        }

        if (bandChar == 'x' or bandChar == 'X')
        {
            band = X;
        }
        else if (bandChar == 'y' or bandChar == 'Y')
        {
            band = Y;
        }
        else
        {
            ShiWarning("Invalid Tacan band\n"); // Band can only be type X or Y
        }

        F4Assert(channel > 0 and channel <= NUM_CHANNELS); // Channel must be between 1 - 126 inclusive

        if (StoreStation(&p_stations, (short)stationId, channel, band, callsign,
                         range, tactype, ilsfreq))   // Insert into ordered linked list
        {
            mCampListTally++; // If there are no duplicates, increment Tally
        }
    }

    ResolveStationList(&p_stations, &mpCampList, mCampListTally); // Copy linked list into array
    fclose(p_File);

    InitDynamicChans();
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// TacanList::~TacanList();
//
// Cleanup all memory.  This means freeing up all structures in the
// list and then blowing away the pointers to the structures.
//---------------------------------------------------------------

TacanList::~TacanList()
{

    int i;

    LinkedTacanVUStr* p_link;
    LinkedTacanVUStr* p_current;

    for (i = 0; i < mCampListTally; i++)
    {
        delete mpCampList[i];
    }

    delete [] mpCampList; // Blow away the CampID list

    p_current = mpTList;

    while (p_current)   // Walk the tacan list and delete all links
    {
        p_link = p_current->p_next;
        // delete p_current->p_station;
        delete p_current;
        p_current = p_link;
    }

    CleanupDynamicChans();
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// TacanList::AddTacan
//---------------------------------------------------------------

void TacanList::AddTacan(CampBaseClass *p_campEntity)
{

    int channel = 0;
    StationSet set;
    Domain domain;
    LinkedTacanVUStr* p_tacanVUStr = NULL;
    LinkedTacanVUStr** p_next = NULL;
    LinkedTacanVUStr* p_previous = NULL;

    if (p_campEntity->IsObjective() and 
        p_campEntity->GetType() == TYPE_AIRBASE) // If inserting an airbase
    {
        domain = AG;

        if (GetChannelFromCampID(&channel, &set, p_campEntity->GetCampId())) // Get channel and band from mpCampList
        {
            if (mpTList == NULL)   // If the mpTList (tacan) list is empty ...
            {
                p_next = &mpTList; // The next links are also empty
                p_previous = NULL;
                InsertIntoTacanList(&p_previous, p_next, p_campEntity->Id(), p_campEntity->GetCampID(), channel, set, domain);
            }
            else if ( not GetPointerFromVUID(mpTList, p_campEntity->Id(), &p_previous, &p_tacanVUStr))
            {
                // Check the sorted mpTList (tacan) list for duplicate entries stop ...
                // searching when we find a VU_ID greater than the one we are searching for.
                // If the entity isn't already in the list then do the following.
                p_next = &p_tacanVUStr; // The next link will be where the search ended.
                InsertIntoTacanList(&p_previous, p_next, p_campEntity->Id(), p_campEntity->GetCampID(), channel, set, domain);
            }
        }
    }
    else if (p_campEntity->EntityType()->classInfo_[VU_CLASS] == CLASS_UNIT and 
             p_campEntity->EntityType()->classInfo_[VU_TYPE] == TYPE_FLIGHT and 
             ((Unit) p_campEntity)->GetUnitMission() == AMIS_TANKER)// If inserting a tanker
    {
        ((FlightClass*)p_campEntity)->tacan_channel = (uchar) AssignChannel(p_campEntity->Id(), AA, p_campEntity->GetCampID()); // assign a unique channel
        ((FlightClass*)p_campEntity)->tacan_band = 'Y';
    }
    else if (p_campEntity->EntityType()->classInfo_[VU_CLASS] == CLASS_UNIT and 
             p_campEntity->EntityType()->classInfo_[VU_TYPE] == TYPE_TASKFORCE and 
             p_campEntity->GetSType() == STYPE_UNIT_CARRIER) // If inserting a carrier
    {
        ((TaskForceClass*)p_campEntity)->tacan_channel = (uchar) AssignChannel(p_campEntity->Id(), AG, p_campEntity->GetCampID()); // assign a unique channel
        ((TaskForceClass*)p_campEntity)->tacan_band = 'Y';
    }


}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// TacanList::RemoveTacan
//---------------------------------------------------------------

void TacanList::RemoveTacan(VU_ID id, int type)
{

    LinkedTacanVUStr* p_tacanVUStr;
    LinkedTacanVUStr* p_previous;

    if (type == NavigationSystem::AIRBASE) // If removing an airbase
    {
        if (GetPointerFromVUID(mpTList, id, &p_previous, &p_tacanVUStr))   // Find location in list
        {

            if (p_tacanVUStr->p_previous)   // Break the chain and relink
            {
                p_tacanVUStr->p_previous->p_next = p_tacanVUStr->p_next;

                if (p_tacanVUStr->p_next)
                {
                    p_tacanVUStr->p_next->p_previous = p_tacanVUStr->p_previous;
                }
            }
            else
            {
                mpTList = p_tacanVUStr->p_next;

                if (p_tacanVUStr->p_next)
                {
                    p_tacanVUStr->p_next->p_previous = NULL;
                }
            }

            delete p_tacanVUStr; // Destroy link
        }
    }
    else if (type == NavigationSystem::TANKER) // If removing a tanker
    {
        RetireChannel(id);
    }
    else if (type == NavigationSystem::CARRIER) // If removing a carrier
    {
        RetireChannel(id);
    }
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// TacanList::GetChannelFromVUID
//---------------------------------------------------------------

BOOL TacanList::GetChannelFromVUID(VU_ID id,
                                   int* p_channel, StationSet* p_set, Domain* p_domain,
                                   int *rangep, int *ttype, float *ilsfreq)
{

    BOOL result;
    LinkedTacanVUStr* tacanVUStr;
    LinkedTacanVUStr* p_previous;

    result = GetPointerFromVUID(mpTList, id, &p_previous, &tacanVUStr);

    if (result)
    {
        *p_channel = tacanVUStr->channel;
        *p_set = tacanVUStr->set;
        *p_domain = tacanVUStr->domain;
        TacanCampStr *tinfo; /// XXXXX Bleah

        if (GetCampTacanFromVUID(&tinfo, tacanVUStr->camp_id))
        {
            *rangep = tinfo->range;
            *ttype = tinfo->tactype;
            *ilsfreq = tinfo->ilsfreq;
        }
        else
        {
            *rangep = 150;
            *ttype = 1;
            *ilsfreq = 0;
        }

    }

    return result;
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// TacanList::GetVUIDFromChannel
//---------------------------------------------------------------

BOOL TacanList::GetVUIDFromChannel(int channel, StationSet set, Domain domain,
                                   VU_ID*vuid, int *rangep, int *ttype, float *ilsfreq)
{

    BOOL result = FALSE;
    LinkedTacanVUStr* p_current;
    *rangep = 0;
    *ttype = 0;
    *ilsfreq = 0;

    if (set == X and domain == AG)
    {
        p_current = mpTList;

        while (p_current and (p_current->channel not_eq channel or p_current->set not_eq set or p_current->domain not_eq domain))
        {
            p_current = p_current->p_next;
        }

        if (p_current)
        {
            *vuid = p_current->vuID;
            TacanCampStr *tinfo; /// XXXXX Bleah

            if (GetCampTacanFromVUID(&tinfo, p_current->camp_id))
            {
                *rangep = tinfo->range;
                *ttype = tinfo->tactype;
                *ilsfreq = tinfo->ilsfreq;
            }

            result = TRUE;
        }
    }
    else if (set == Y and domain == AA)   // Note this only works for the host machine
    {

        p_current = mpAssigned;

        while (p_current and (p_current->channel not_eq channel or p_current->set not_eq set or p_current->domain not_eq domain))
        {
            p_current = p_current->p_next;
        }

        if (p_current)
        {
            *vuid = p_current->vuID;
            *rangep = 150;
            *ttype = 1;
            result = TRUE;
        }
    }
    else if (set == Y and domain == AG)   // M.N. for Carrier Tacans
    {

        p_current = mpAssigned;

        while (p_current and (p_current->channel not_eq channel or p_current->set not_eq set or p_current->domain not_eq domain))
        {
            p_current = p_current->p_next;
        }

        if (p_current)
        {
            *vuid = p_current->vuID;
            *rangep = 50;
            *ttype = 1;
            result = TRUE;
        }
    }


    return result;
}

/////////////////////////////////////////////////////////////////


//---------------------------------------------------------------
// TacanList::GetVUIDFromLocation
//---------------------------------------------------------------

BOOL TacanList::GetVUIDFromLocation(float x, float y, Domain domain,
                                    VU_ID* id, int *range, int *type, float *ilsfreq)
{

    VuEntity* p_entity;
    LinkedTacanVUStr* p_current = mpTList;
    BOOL result = FALSE;
    float tacanX;
    float tacanY;
    float distance;
    float bestDistance = -1.0F;
    VU_ID bestTacan;

    bestTacan = FalconNullId;

    while (p_current)
    {

        if (p_current->domain == domain)
        {

            p_entity = vuDatabase->Find(p_current->vuID);
            tacanX = p_entity->XPos();
            tacanY = p_entity->YPos();

            distance = DistSqu(tacanX, tacanY, x, y);

            if (distance < bestDistance or bestDistance < 0.0F)
            {
                bestDistance = distance;
                bestTacan = p_current->vuID;
                result = TRUE;
            }
        }

        p_current = p_current->p_next;
    }

    *id = bestTacan;
    return result;
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// TacanList::GetPointerFromVUID
//
// Returns True if record is found.  If found, the pointer of the
// record will be stored in p_after.  If the record is not found
// The pointers of the records immediately before and after where
// the record of interest would fall in order are stored in
// p_before and p_after.
//---------------------------------------------------------------

BOOL TacanList::GetPointerFromVUID(LinkedTacanVUStr* p_list, VU_ID id, LinkedTacanVUStr** p_before, LinkedTacanVUStr** p_after)
{

    BOOL result = FALSE;
    LinkedTacanVUStr* p_current = p_list;
    LinkedTacanVUStr* p_previous = NULL;


    while (p_current and p_current->vuID < id)
    {
        p_previous = p_current;
        p_current = p_current->p_next;
    }

    *p_after = p_current;
    *p_before = p_previous;

    if (*p_after and (*p_after)->vuID == id)
    {
        result = TRUE;
    }

    return result;
}
/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// TacanList::GetCampTacanFromVUID
//
// Returns True if record is found.  If found, the pointer of the
// record will be stored in tacaninfo.
//---------------------------------------------------------------

/////////////////////////////////////////////////////////////////

BOOL TacanList::GetCampTacanFromVUID(TacanCampStr **tacaninfo, short campid)
{
    TacanCampStr key;
    TacanCampStr** p_occurrence;
    BOOL returnStatus = FALSE;

    key.campaignID = campid;

    p_occurrence = (TacanCampStr**) bsearch(&key, mpCampList, mCampListTally,
                          sizeof(TacanCampStr*),
                          (int (*)(const void*, const void*))SearchForChannel); // Do the binary search

    if (p_occurrence)
    {
        *tacaninfo = *p_occurrence;
        returnStatus = TRUE;
    }

    return returnStatus;
}


//---------------------------------------------------------------
// TacanList::InsertIntoTacanList
//---------------------------------------------------------------

void TacanList::InsertIntoTacanList(LinkedTacanVUStr** p_previous,
                                    LinkedTacanVUStr** p_next,
                                    VU_ID vuId,
                                    short camp_id,
                                    int channel,
                                    StationSet set,
                                    Domain domain)
{

    LinkedTacanVUStr* p_tacanVUStr;

#ifdef USE_SH_POOLS
    p_tacanVUStr = (LinkedTacanVUStr *)MemAllocPtr(gTacanMemPool, sizeof(LinkedTacanVUStr), 0); // Create a new link
#else
    p_tacanVUStr = new LinkedTacanVUStr; // Create a new link
#endif
    p_tacanVUStr->channel = channel; // Stuff data into link
    p_tacanVUStr->set = set;
    p_tacanVUStr->domain = domain;
    p_tacanVUStr->vuID = vuId;
    p_tacanVUStr->camp_id = camp_id;
    p_tacanVUStr->p_previous = NULL;
    p_tacanVUStr->p_next = NULL;

    if (*p_previous and *p_next)   // If there are links that follow this link
    {
        p_tacanVUStr->p_previous = *p_previous;
        p_tacanVUStr->p_next = *p_next; // The link that follows is the next link
        (*p_previous)->p_next = p_tacanVUStr;
        (*p_next)->p_previous = p_tacanVUStr; // New link is now the previous
    }
    else if (*p_next)
    {
        p_tacanVUStr->p_previous = NULL;
        p_tacanVUStr->p_next = *p_next;
        (*p_next)->p_previous = p_tacanVUStr;
        mpTList = p_tacanVUStr; // Overwrite the head pointer
    }
    else if (*p_previous)
    {
        p_tacanVUStr->p_previous = *p_previous;
        p_tacanVUStr->p_next = NULL;
        (*p_previous)->p_next = p_tacanVUStr;
    }
    else
    {
        p_tacanVUStr->p_previous = NULL;
        p_tacanVUStr->p_next = NULL;
        mpTList = p_tacanVUStr; // Overwrite the head pointer
    }
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// TacanList::ResolveStationList
//
// Creates an array to hold all the station elements.  Then walks
// the linked list created by StoreStation and stores all station
// elements into the array.  The links from the linked list are
// then cleaned up.
//
// The purpose of placing the elements into an array is so that
// we can perform binary searches.
//---------------------------------------------------------------

void TacanList::ResolveStationList(LinkedCampStationStr** p_list, TacanCampStr*** p_CampIdArray, int size)
{

    if (size <= 0)   // Make sure we have work to do
    {
        //F4Assert(FALSE);
        ShiWarning("No work to do\n");
        return;
    }

    if (p_list == NULL)   // Check for a good pointer
    {
        //F4Assert(FALSE);
        ShiWarning("no pointer\n");
        return;
    }

    int i = 0;
    LinkedCampStationStr *p_current;
    LinkedCampStationStr *p_previous;

#ifdef USE_SH_POOLS
    *p_CampIdArray = (TacanCampStr **)MemAllocPtr(gTacanMemPool, sizeof(TacanCampStr *) * size, 0); // Create a new link
#else
    *p_CampIdArray = new TacanCampStr*[size]; // Create the array
#endif

    p_current = *p_list;

    while (p_current)   // While we are not at the end of the linked list
    {
        (*p_CampIdArray)[i] = p_current->p_station; // Copy contents to the station struct into the array

        p_previous = p_current; // Save location of the link
        p_current = p_current->p_next; // Goto the next link
        delete p_previous; // Remove the previous link
        i++; // Increment element of the array
    }

#if 0

    p_occurrence = (TacanCampStr**) bsearch(&key, mpCampList, mCampListTally,
                          sizeof(TacanCampStr*),
                          (int (*)(const void*, const void*))SearchForChannel); // Do the binary search
#endif


    qsort(*p_CampIdArray, size, sizeof(TacanCampStr*), (int (*)(const void*, const void*))CompareCampIDs);


    F4Assert(i == size); // Otherwise memory has been stomped
    // or will be stomped when we clean up
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// TacanList::StoreStation
//
// This function is responsible for inserting tacan channel
// elements into a sorted linked list.  Essentially this function
// performs an insertion sort upon the list.  The list is created
// in ascending order, sorted by channel first then by band.
// Legal values for channel range from 1-126.  Legal values for
// band are TacanList::X and TacanList::Y.
//---------------------------------------------------------------

BOOL TacanList::StoreStation(LinkedCampStationStr** p_list,
                             short airbaseId, int channel,
                             StationSet band, int callsign,
                             int range, int tactype, float ilsfreq)
{

    if (channel < 1 or channel > 126)   // Note: the value of channel should be
    {
        //F4Assert(FALSE); // constrained to 1 - 126
        ShiWarning("invalid channel\n");
        return FALSE;
    }

    if (band not_eq X and band not_eq Y)   // Channels are constrained to
    {
        //F4Assert(FALSE); // X or Y band.
        ShiWarning("invalid band\n");
        return FALSE;
    }

    if (p_list == NULL)
    {
        //F4Assert(FALSE); // Bad pointer
        ShiWarning("bad pointer\n");
        return FALSE;
    }


    TacanCampStr* p_staStr;
    LinkedCampStationStr* p_linkStaStr;
    // LinkedCampStationStr* p_previous = NULL;
    // LinkedCampStationStr* p_current = NULL;
    // BOOL done = FALSE;
    // BOOL returnStatus = FALSE;


#ifdef USE_SH_POOLS
    p_staStr = (TacanCampStr *)MemAllocPtr(gTacanMemPool, sizeof(TacanCampStr), 0); // Create a new link
    p_linkStaStr = (LinkedCampStationStr *)MemAllocPtr(gTacanMemPool, sizeof(LinkedCampStationStr), 0); // Create a new link
#else
    p_staStr = new TacanCampStr; // Create a new element
    p_linkStaStr = new LinkedCampStationStr; // Create a new link
#endif
    p_staStr->channel = channel; // Fill the element
    p_staStr->set = band;
    p_staStr->campaignID = airbaseId;
    p_staStr->callsign = callsign;
    p_staStr->range = range;
    p_staStr->tactype = tactype;
    p_staStr->ilsfreq = ilsfreq;
    p_linkStaStr->p_station = p_staStr; // Fill the link
    // p_current = *p_list;

    p_linkStaStr->p_next = *p_list;
    *p_list = p_linkStaStr;

    return TRUE;


#if 0

    while (done == FALSE)
    {

        if (*p_list == NULL)
        {
            *p_list = p_linkStaStr; // No elements in list
            returnStatus = TRUE; // Automatically Insert
            done = TRUE;
        }
        else if (p_current->p_next == NULL)
        {
            if (p_current->p_station->campaignID > p_linkStaStr->p_station->campaignID)
            {
                p_linkStaStr->p_next = p_current;
                *p_list = p_linkStaStr;
                done = TRUE;
                returnStatus = TRUE;
            }
            else if (p_current->p_station->campaignID < p_linkStaStr->p_station->campaignID)
            {
                p_current->p_next = p_linkStaStr;
                done = TRUE;
                returnStatus = TRUE;
            }
            else
            {
                delete p_linkStaStr->p_station; // Clean up
                delete p_linkStaStr;
                done = TRUE; // Return false so that we don't increment the Tally
                returnStatus = FALSE;
                F4Assert(FALSE); // Duplicate Entries, sloppy file
            }
        }
        else if (p_current->p_station->campaignID > p_linkStaStr->p_station->campaignID)
        {
            p_linkStaStr->p_next = p_current; // Break links and insert

            if (p_previous)   // If inserting before the first element
            {
                p_previous->p_next = p_linkStaStr;
            }
            else
            {
                *p_list = p_linkStaStr;
            }

            done = TRUE;
            returnStatus = TRUE;
        }
        else if (p_current->p_station->campaignID == p_linkStaStr->p_station->campaignID)   // Duplicate Airbases
        {
            delete p_linkStaStr->p_station; // Clean up
            delete p_linkStaStr;
            done = TRUE; // Return false so that we don't increment the Tally
            returnStatus = FALSE;
            F4Assert(FALSE); // Duplicate Entries, sloppy file
        }
        else if (p_current->p_station->campaignID < p_linkStaStr->p_station->campaignID)
        {
            p_previous = p_current;
            p_current = p_current->p_next;
        }
    }

    return returnStatus;
#endif
}

/////////////////////////////////////////////////////////////////



//---------------------------------------------------------------
// TacanList::GetChannelFromCampID
//
// Given a particular campaign ID, find the
// corresponding channel.  This function performs a binary
// search upon a sorted array of tacan stations.  If an airbase
// if found, TRUE is returned
//---------------------------------------------------------------

BOOL TacanList::GetChannelFromCampID(int* channel, StationSet* band, short airbaseId)
{

    if (airbaseId < 0)
    {
        //F4Assert(FALSE); // Bad Id
        ShiWarning("bad airbase id\n");
        return FALSE;
    }

    TacanCampStr key;
    TacanCampStr** p_occurrence;
    BOOL returnStatus = FALSE;

    key.campaignID = airbaseId;

    p_occurrence = (TacanCampStr**) bsearch(&key, mpCampList, mCampListTally,
                          sizeof(TacanCampStr*),
                          (int (*)(const void*, const void*))SearchForChannel); // Do the binary search

    if (p_occurrence)
    {
        *channel = (*p_occurrence)->channel;
        *band = (*p_occurrence)->set;
        returnStatus = TRUE;
    }

    return returnStatus;
}

/////////////////////////////////////////////////////////////////

BOOL TacanList::GetCallsignFromCampID(short campId, int *callsign)
{
    if (campId < 0)
    {
        //F4Assert(FALSE); // Bad Id
        ShiWarning("bad campid\n");
        return FALSE;
    }

    TacanCampStr key;
    TacanCampStr** p_occurrence;
    BOOL returnStatus = FALSE;

    key.campaignID = campId;

    p_occurrence = (TacanCampStr**) bsearch(&key, mpCampList, mCampListTally,
                          sizeof(TacanCampStr*),
                          (int (*)(const void*, const void*))SearchForChannel); // Do the binary search

    if (p_occurrence)
    {
        *callsign = (*p_occurrence)->callsign;
        returnStatus = TRUE;
    }

    return returnStatus;
}


//---------------------------------------------------------------
// TacanList::InitDynamicChans
//
// Set up the dynamic lists and get ready to stuff data into the
// list.  Note: This should only be called by either the
// constructor or after CleanupDynamicChans() has been called
//---------------------------------------------------------------

void TacanList::InitDynamicChans(void)
{
    mpAssigned = NULL;
    mpRetired = NULL;
    mLastUnused = NUM_CHANNELS;
}
/////////////////////////////////////////////////////////////////

//---------------------------------------------------------------
// TacanList::CleanupDynamicChans
//
// Okay kids, put your toys away.  Clean up the dynamic lists.
// Free up memory for a rainy day.  This should only be called
// by the destructor or when the campaign is over.
//---------------------------------------------------------------

void TacanList::CleanupDynamicChans(void)
{
    LinkedTacanVUStr* pCurrent = NULL;
    LinkedTacanVUStr* pNext = NULL;

    pCurrent = mpAssigned; // Walk the assigned list and free everything

    while (pCurrent)
    {
        pNext = pCurrent->p_next;
        delete pCurrent;
        pCurrent = pNext;
    }

    pCurrent = mpRetired; // Walk the retired list and free everything

    while (pCurrent)
    {
        pNext = pCurrent->p_next;
        delete pCurrent;
        pCurrent = pNext;
    }
}
/////////////////////////////////////////////////////////////////

//---------------------------------------------------------------
// TacanList::AssignChannel
//
// When the campaign creates a Tanker flight or carrier, it will
// need a tacan channel.  First check if we have any retired
// channels that are laying around (retired).  If so, move the
// retired LinkedTacanVUStr pointer from the retired list to
// the assigned list.  If there are no retired channels (this is
// the initial state) create a LinkedTacanVUStr instance and
// assign the current mLastUnused channel to it.
//---------------------------------------------------------------

int TacanList::AssignChannel(VU_ID vuID, Domain domain, short camp_id)
{
    LinkedTacanVUStr* pTacanStr;
    int channel = -1;

    if (mpRetired == NULL)
    {

        ShiAssert(mLastUnused >= g_nMinTacanChannel);

        if (mLastUnused >= g_nMinTacanChannel)   // Note: we have a total of 56 channels to work with.
        {
            // If we need more than 56 at a time, then the campaign has gone wild.
            // Ignore anything over 56 requests.
            // MN this doesn't seem to be an issue anymore...(why 56 at all ??)

#ifdef USE_SH_POOLS
            pTacanStr = (LinkedTacanVUStr *)MemAllocPtr(gTacanMemPool, sizeof(LinkedTacanVUStr), 0); // Create a new link
#else
            pTacanStr = new LinkedTacanVUStr;
#endif

            if (mpAssigned)
            {
                mpAssigned->p_previous = pTacanStr; // Were going to add to the head of the list, so copy the new link into the current head
            }

            pTacanStr->p_next = mpAssigned; // The next link is the current head ... since we are adding to the head
            pTacanStr->p_previous = NULL;
            pTacanStr->channel = mLastUnused--; // Be sure to decrement the last unused
            pTacanStr->set = Y;
            pTacanStr->domain = domain;
            pTacanStr->vuID = vuID;
            pTacanStr->camp_id = camp_id;

            mpAssigned = pTacanStr; // Place it into the assigned list
            channel = mpAssigned->channel;
        }
    }
    else
    {
        pTacanStr = mpRetired; // Pull off the head of the retired list
        mpRetired = pTacanStr->p_next;

        if (mpAssigned)
        {
            mpAssigned->p_previous = pTacanStr; // Were going to add to the head of the list, so copy the new link into the current head
        }

        pTacanStr->p_next = mpAssigned; // The next link is the current head ... since we are adding to the head
        pTacanStr->p_previous = NULL;
        pTacanStr->set = Y;
        pTacanStr->domain = domain;
        pTacanStr->vuID = vuID;
        pTacanStr->camp_id = camp_id;

        mpAssigned = pTacanStr; // Place it into the assigned list
        channel = mpAssigned->channel;
    }

    return channel;
}
/////////////////////////////////////////////////////////////////

//---------------------------------------------------------------
// TacanList::RetireChannel
//
// The campaign has just destroyed the tanker flight or carrier.
// The channel is no longer in use so retire it by placing the
// pointer into the retired list.  The retired list is bascally
// a free pool.
//---------------------------------------------------------------
void TacanList::RetireChannel(VU_ID vuID)
{
    BOOL found = FALSE;
    LinkedTacanVUStr* pTacanStr = NULL;

    pTacanStr = mpAssigned;

    while ( not found and pTacanStr)
    {

        if (pTacanStr->vuID == vuID)
        {
            found = TRUE;

            if (pTacanStr->p_previous)   // If I'm not the first in the list
            {
                pTacanStr->p_previous->p_next = pTacanStr->p_next; // Break the link
            }
            else
            {
                mpAssigned = pTacanStr->p_next; // Otherwise make my next link the first
            }

            if (pTacanStr->p_next)
            {
                pTacanStr->p_next->p_previous = pTacanStr->p_previous;
            }

            if (mpRetired)   // If there is already a head on the retired list
            {
                mpRetired->p_previous = pTacanStr; // give the head my pointer
            }

            pTacanStr->p_next = mpRetired; // Put the head pointer into my next
            mpRetired = pTacanStr; // Make myself the head
        }
        else
        {
            pTacanStr = pTacanStr->p_next; // Otherwise goto the next link
        }
    }
}

/////////////////////////////////////////////////////////////////
//---------------------------------------------------------------
// TacanList::ChannelToFrequency
//
// Find the frequency corresponding to a given channel
//---------------------------------------------------------------

int TacanList::ChannelToFrequency(StationSet set, int channel)
{
    switch (set)
    {
        case X:
            if (channel < 64)
                return 961 + channel;
            else
                return (channel = 126) + 1213;

            break;

        case Y:
            if (channel < 64)
                return 1087 + channel;
            else return (channel - 126) + 1087;

            break;

        default:
            return 0;
    }
}

/////////////////////////////////////////////////////////////////

int CompareCampIDs(void** element1, void** element2)
{
    int returnStatus;

    TacanList::TacanCampStr** el1 = (TacanList::TacanCampStr**) element1;
    TacanList::TacanCampStr** el2 = (TacanList::TacanCampStr**) element2;

    if ((*el1)->campaignID < (*el2)->campaignID)
    {
        returnStatus = -1;
    }
    else if ((*el1)->campaignID == (*el2)->campaignID)
    {
        returnStatus = 0;
    }
    else
    {
        returnStatus = 1;
    }

    return returnStatus;
}

//---------------------------------------------------------------
// SearchForChannel
//
// Called by GetChannel in the bsearch routine.  This is a
// callback function used to compare elements within the
// bsearch routine.
//---------------------------------------------------------------

int SearchForChannel(void* element1, void** element2)
{

    int returnStatus;
    TacanList::TacanCampStr* el1 = (TacanList::TacanCampStr*) element1;
    TacanList::TacanCampStr** el2 = (TacanList::TacanCampStr**) element2;

    if (el1->campaignID < (*el2)->campaignID)
    {
        returnStatus = -1;
    }
    else if (el1->campaignID == (*el2)->campaignID)
    {
        returnStatus = 0;
    }
    else
    {
        returnStatus = 1;
    }

    return returnStatus;
}

/////////////////////////////////////////////////////////////////
