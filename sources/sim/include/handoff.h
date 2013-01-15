/***************************************************************************\
    Handoff.h
    Scott Randolph
    October 9, 1998

    This module handles the conversion of camp to sim and sim to camp
	object when they cross the player bubble boundry.
\***************************************************************************/
#ifndef _HANDOFF_H_
#define _HANDOFF_H_

class SimObjectType;

typedef enum { HANDOFF_RADAR, HANDOFF_RANDOM, HANDOFF_LEADER } HandOffType;

FalconEntity*	SimCampHandoff( FalconEntity  *current, HandOffType style );
SimObjectType*	SimCampHandoff( SimObjectType *current, SimObjectType *tgtList, HandOffType style );

#endif // _HANDOFF_H_