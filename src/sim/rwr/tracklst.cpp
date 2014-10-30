#include "stdhdr.h"
#include "vehrwr.h"
#include "simbase.h"
#include "entity.h"
#include "soundfx.h"
#include "team.h"
#include "simmover.h"
#include "simdrive.h"
#include "object.h"
#include "radarData.h"

VehRwrClass::DetectListElement* VehRwrClass::IsTracked(FalconEntity* object)
{
    int i;
    DetectListElement* retval = NULL;

    // Check for existing tracked object
    for (i = 0; i < numContacts + 1; i++)
    {
        if (detectionList[i].entity == object)
        {
            retval = &detectionList[i];
        }
    }

    return retval;
}

VehRwrClass::DetectListElement* VehRwrClass::AddTrack(FalconEntity* object, float lethality)
{
    int i, j;
    DetectListElement *retval = NULL;
    SimObjectType* tmpPtr;

    for (i = 0; i < numContacts; i++) // JB 010727
        detectionList[i].newDetectionDisplay = FALSE; // JB 010727

    // Find the first contact with lower lethality
    for (i = 0; i < numContacts; i++)
    {
        // JB 010718
        //We bump the lethality of things by 10% already in the list
        //to keep from putting things in and out all the time
        if (detectionList[i].lethality * 1.1f < lethality) // 1.1f and < instead of <= (RP5)
        {
            break;
        }
    }

    // Does this track qualify for addition to the list?
    if (i <= numContacts and i < MaxRWRTracks)
    {
        // Drop the last entry if the list is full
        if (detectionList[MaxRWRTracks - 1].entity)
        {
            DropTrack(MaxRWRTracks - 1);
        }

        // Shift everything down to make room for the new entry
        for (j = min(numContacts, MaxRWRTracks - 1); j > i; j--)
        {
            detectionList[j] = detectionList[j - 1]; //ME123 this loop was fucked up. fixed.
        }

        ShiAssert(j <= i);

        // Initialize the new entry -- the rest of the elements will be set by the caller
        detectionList[i].entity = object;
        detectionList[i].radarData = &RadarDataTable[object->GetRadarType()];
        detectionList[i].lastPlayed = 0;
        detectionList[i].isLocked = 0;
        detectionList[i].isAGLocked = 0;//Cobra TJL
        detectionList[i].missileActivity = 0;
        detectionList[i].missileLaunch = 0;
        // JB 010727 RP5 RWR
        // 2001-02-15 ADDED BY S.G. SO THE NEW cantPlay field IS ZEROED AS WELL AND playIt AS WELL
        detectionList[i].cantPlay = 0;
        detectionList[i].playIt = 0;
        // END OF ADDED SECTION
        detectionList[i].previouslyLocked = FALSE;
        detectionList[i].newDetection = TRUE;
        detectionList[i].newDetectionDisplay = TRUE; // JB 010727
        detectionList[i].lethality = lethality;

        VuReferenceEntity(object);
        retval = &detectionList[i];

        // Maintain the "selected" flag
        if (i not_eq 0)
        {
            // If we didn't go into the top slot, we won't change who's selected
            detectionList[i].selected = 0;
        }
        else
        {
            // If we went into the top slot, we'll inherit the flag from whomever we pushed out
            // Just make sure the guy we pushed out doesn't think he still has it.
            detectionList[1].selected = 0;
        }

        // Find this object in the target list, and mark the RWR as tracking
        // TODO:  Make the entity pointer in the detectionList be a SimObjectType to avoid this...
        tmpPtr = platform->targetList;

        while (tmpPtr)
        {
            if (tmpPtr->BaseData() == object)
            {
                tmpPtr->localData->sensorState[Type()] = SensorTrack;
                break;
            }

            tmpPtr = tmpPtr->next;
        }
    }

    numContacts = min(numContacts + 1, MaxRWRTracks);
    return retval;
}

// JB 010718
void VehRwrClass::SortDetectionList(void)
{
    int i, j;
    DetectListElement tmpElement;

    for (i = 1; i < numContacts; i++)
    {
        //if we don't need to move, don't do any unnecessary copies
        if (detectionList[i - 1].lethality < detectionList[i].lethality)
        {
            //copy data for current entry
            tmpElement = detectionList[i];
            j = i;

            while (detectionList[j - 1].lethality < tmpElement.lethality)
            {
                //if our lethality is greater than lethality of entry before us move it down
                detectionList[j] = detectionList[j - 1];
                j--;

                if ( not j)
                {
                    //check to prevent going off top of array
                    if (detectionList[1].selected)
                    {
                        // Bring the selected flag back if we pushed it out of the default slot
                        detectionList[1].selected = 0;
                        tmpElement.selected = 1;
                    }

                    break;
                }
            }

            //insert us back into new place in list
            detectionList[j] = tmpElement;
        }
    }
}

void VehRwrClass::ResortList(VehRwrClass::DetectListElement* theElement)
{
    float thisLethality = theElement->lethality;
    int i;
    int thisId = -1;
    int newId = 0;
    DetectListElement tmpElement;

    // Find out where this element should go
    for (i = 0; i < numContacts; i++)
    {
        if (&detectionList[i] == theElement)
        {
            tmpElement = detectionList[i];
            thisId = i;
        }

        if (detectionList[i].lethality >= thisLethality)//me123 addet =
            newId = i;
    }

    if (newId < thisId)
    {
        // Moving up in the world (moving into a lower number/higher priority slot)
        // Push others down to fill in the hole we're leaving behind
        for (i = thisId; i > newId; i--)
        {
            detectionList[i] = detectionList[i - 1];
        }

        // Drop into our new slot
        ShiAssert(i == newId);
        detectionList[newId] = tmpElement;

        // Bring the selected flag back if we pushed it out of the default slot
        if ((newId == 0) and (detectionList[1].selected))
        {
            detectionList[1].selected = 0;
            detectionList[0].selected = 1;
        }
    }
    else if (newId > thisId)
    {
        // Moving down in the world (moving into a higher number/lower priority slot)
        // Pull others up to fill in the hole we're leaving behind
        for (i = thisId; i < newId; i++)
        {
            detectionList[i] = detectionList[i + 1];
        }

        // Drop into our new slot
        ShiAssert(i == newId);
        detectionList[newId] = tmpElement;

        // Put the selected flag back if we took it with us out of the default slot
        if ((thisId == 0) and (detectionList[newId].selected))
        {
            detectionList[newId].selected = 0;
            detectionList[0].selected = 1;
        }
    }
}

void VehRwrClass::DropTrack(int trackNum)
{
    SimObjectType* tmpPtr = platform->targetList;

    ShiAssert(numContacts > 0);

    // Find this object in the target list, and mark the RWR as not tracking
    // TODO:  Make the entity pointer in the detectionList be a SimObjectType to avoid this...
    while (tmpPtr)
    {
        if (tmpPtr->BaseData() == detectionList[trackNum].entity)
        {
            tmpPtr->localData->sensorState[Type()] = NoTrack;
            break;
        }

        tmpPtr = tmpPtr->next;
    }

    // Let go of the entity
    VuDeReferenceEntity(detectionList[trackNum].entity);
    detectionList[trackNum].entity = NULL;

    // Fix up the "selected" flag if this record had it
    if (detectionList[trackNum].selected)
    {
        if (trackNum == 0)
            // We're going to delete the default one and shift number 1 up to the default slot
            detectionList[1].selected = 1;
        else
            // We deleted the selected emitter, so send it back to the default
            detectionList[0].selected = 1;
    }

    // Shift other contacts up
    for (int i = trackNum; i < numContacts; i++)
    {
        if (i + 1 < MaxRWRTracks)
            detectionList[i] = detectionList[i + 1];
        else
        {
            detectionList[i].entity = NULL;
            detectionList[i].lastHit = 0;
            detectionList[i].lethality = 0.0F;
            detectionList[i].isLocked = 0;
            detectionList[i].isAGLocked = 0;//Cobra TJL
            detectionList[i].missileActivity = 0;
            detectionList[i].missileLaunch = 0;
            detectionList[i].selected = 0;
            // JB 010727 RP5 RWR
            // 2001-02-15 ADDED BY S.G. SO THE NEW cantPlay field IS ZEROED AS WELL AND playIt AS WELL
            detectionList[i].cantPlay = 0;
            detectionList[i].playIt = 0;
            // END OF ADDED SECTION
        }
    }

    // Note the decrease in our total number of tracks
    numContacts--;
}
