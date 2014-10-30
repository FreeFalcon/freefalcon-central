/***************************************************************************\
    Handoff.cpp
    Scott Randolph
    October 9, 1998

    This module handles the conversion of camp to sim and sim to camp
 object when they cross the player bubble boundry.
\***************************************************************************/
#include "CampBase.h"
#include "SimBase.h"
#include "Object.h"
#include "handoff.h"



// Pass in the currently held sim object.  (IT MUST ALREADY BE REFERENCED)
// If it is still valid it will be returned as is.
// If it needs to be changed (camp to sim or sim to camp) its analog will
// be searched for in the provided target list.  If found, the new object will
// returned.  If not found, a SimObjectType will be created for the new object
// and returned WITHOUT a reference count.  If the target is no longer valid
// and no handoff is possible, NULL will be returned.  IT IS THE RESPONSIBLITY
// OF THE CALLER TO HANDLE REFERENCE AND RELEASES.  Normally, this means the caller
// should Release his old target AFTER calling this function, but BEFORE clobbering
// his pointer to it.  He must also Reference any new target returned by this
// function.
FalconEntity* SimCampHandoff(FalconEntity *current, HandOffType style)
{
    // Quit now if we don't have a current object
    if ( not current)
        return NULL;

    if (current->IsSim())
    {
        // Quit now if we still have our sim emitter
        if ( not current->IsDead())
        {
            // Its still just fine
            return current;
        }

        // See if it reaggregated
        if (((SimBaseClass*)current)->GetCampaignObject() and 
            ((SimBaseClass*)current)->GetCampaignObject()->IsAggregate() and 
 not ((SimBaseClass*)current)->GetCampaignObject()->IsDead())
        {
            // Switch to the campaign unit
            return ((SimBaseClass*)current)->GetCampaignObject();
        }
        else
        {
            // I guess it just plain died
            return NULL;
        }
    }
    else
    {
        if (current->IsDead())
        {
            // I guess it just plain died
            return NULL;
        }
        else if (((CampBaseClass*)current)->IsAggregate())
        {
            // Its still just fine
            return current;
        }
        else
        {
            // Lets look for our sim children...
            SimBaseClass *simobj;

            switch (style)
            {
                case HANDOFF_RADAR:
                {
                    // If we're a HARM, we've got to find a radar vehicle
                    // Get the list of sim entities in our battalion
                    VuListIterator componentIterator(((CampBaseClass*)current)->GetComponents());
                    simobj = (SimBaseClass*)componentIterator.GetFirst();
                    int campRadarType = current->GetRadarType();

                    // Make sure we didn't get asked to find a radar in a non-radar unit
                    ShiAssert(campRadarType);

                    // Search the list for a battalion radar vehicle
                    while (simobj)
                    {
                        ShiAssert(simobj->IsSim());

                        if (simobj->GetRadarType() == campRadarType and not simobj->IsDead())
                        {
                            break;
                        }

                        simobj = (SimBaseClass*)componentIterator.GetNext();
                    }
                }
                break;

                case HANDOFF_RANDOM:
                {
                    // Pick a random component of the campaign unit to try to hold lock upon
                    int vehs = ((CampBaseClass*)current)->NumberOfComponents();

                    if (vehs)
                    {
                        int i = rand() % vehs;
                        simobj = ((CampBaseClass*)current)->GetComponentEntity(i);

                        // Just in case a component vehicle died but the vehicle count hadn't been updated yet
                        if ( not simobj)
                        {
                            simobj = ((CampBaseClass*)current)->GetComponentLead();
                        }

                        if (simobj and simobj->IsDead())
                            simobj = NULL;
                    }
                    else
                    {
                        // Nothing to find
                        simobj = NULL;
                    }
                }
                break;

                case HANDOFF_LEADER:
                default:
                    simobj = ((CampBaseClass*)current)->GetComponentLead();
            }

            // Return the component we found
            return simobj;
        }
    }
}



// Pass in the currently held sim object.  (IT MUST ALREADY BE REFERENCED)
// If it is still valid it will be returned as is.
// If it needs to be changed (camp to sim or sim to camp) its analog will
// be searched for in the provided target list.  If found, the new object will
// returned.  If not found, a SimObjectType will be created for the new object
// and returned WITHOUT a reference count.  If the target is no longer valid
// and no handoff is possible, NULL will be returned.  IT IS THE RESPONSIBLITY
// OF THE CALLER TO HANDLE REFERENCE AND RELEASES.  Normally, this means the caller
// should Release his old target AFTER calling this function, but BEFORE clobbering
// his pointer to it.  He must also Reference any new target returned by this
// function.
SimObjectType* SimCampHandoff(SimObjectType *current, SimObjectType *targetList, HandOffType style)
{
    CampBaseClass *campobj;
    SimBaseClass *simobj;
    SimObjectType *t;

    // if no target, nothing to validate
    if (current == NULL)
    {
        return NULL;
    }

    // check if our target is a sim vs campaign
    if (current->BaseData()->IsSim())
    {
        // is the target still valid?
        if ( not current->BaseData()->IsDead())
        {
            return current;
        }
        else
        {
            // get its parent
            campobj = ((SimBaseClass *)current->BaseData())->GetCampaignObject();

            // is the parent in the sim lists?
            // if so we want to try and find a matching target in the target list
            if ( not campobj or not campobj->IsAggregate() or campobj->IsDead())
            {
                return NULL;
            }
            else if (campobj->InSimLists())
            {
                t = targetList;

                while (t)
                {
                    if (t->BaseData() == (FalconEntity *)campobj)
                    {
                        // we found it
                        return t;
                    }

                    t = t->next;
                }
            }

            // We'll synthesize a SimObjectType around the target (for buildings and thing not in the target list)
            return new SimObjectType(campobj);
        } // if sim target no longer active
    }
    // campaign unit
    else
    {
        // get campaign object
        campobj = (CampBaseClass *)current->BaseData();

        if ( not campobj or F4IsBadCodePtr((FARPROC) campobj)) // JB 010220 CTD
            return NULL; // JB 010220 CTD

        if (campobj->IsDead())
        {
            // I guess it just plain died
            return NULL;
        }
        else if (campobj->IsAggregate())
        {
            // Its still just fine
            return current;
        }
        else
        {
            // Lets look for our sim children...
            switch (style)
            {
                case HANDOFF_RADAR:
                {
                    // If we're a HARM, we've got to find a radar vehicle
                    // Get the list of sim entities in our battalion
                    VuListIterator componentIterator(campobj->GetComponents());
                    simobj = (SimBaseClass*)componentIterator.GetFirst();
                    int campRadarType = campobj->GetRadarType();

                    // Make sure we didn't get asked to find a radar in a non-radar unit
                    ShiAssert(campRadarType);

                    // Search the list for a battalion radar vehicle
                    while (simobj)
                    {
                        if ( not simobj->IsDead() and simobj->IsAwake() and simobj->GetRadarType() == campRadarType)
                        {
                            break;
                        }

                        simobj = (SimBaseClass*)componentIterator.GetNext();
                    }
                }
                break;

                case HANDOFF_RANDOM:
                {
                    // Pick a random component of the campaign unit to try to hold lock upon
                    int vehs = campobj->NumberOfComponents();

                    if (vehs)
                    {
                        int i = rand() % vehs;
                        simobj = campobj->GetComponentEntity(i);

                        // Just in case a component vehicle died but the vehicle count hadn't been updated yet
                        if ( not simobj)
                        {
                            simobj = campobj->GetComponentLead();
                        }

                        if (simobj and (simobj->IsDead() or not simobj->IsAwake()))
                        {
                            return NULL;
                        }
                    }
                    else
                    {
                        return NULL;
                    }
                }
                break;

                case HANDOFF_LEADER:
                default:
                    simobj = campobj->GetComponentLead();
            }
        }

        // now try to find the sim object in the target list
        t = targetList;

        while (t)
        {
            if (t->BaseData() == (FalconEntity *)simobj)
            {
                // we found it, return it
                return t;
            }

            t = t->next;
        }

        // If we reached here we didn't find it in our target list.
        // We'll synthesize a SimObjectType around the target (for buildings)
        // NOTE:  Since we don't reference here, we had BETTER reference and release
        // at least once in the caller, or else this won't ever be deleted...  Very bad
        // form, I know...
        if (simobj)
        {
            return new SimObjectType(simobj);
        }
        else
        {
            return NULL;
        }
    }
}

