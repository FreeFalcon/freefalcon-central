// 
// Camp UI events are slimmed down event data designed to display for the player
//
// They consist of a text string, a time, a location and some team data - mostly to just pop
// up on our event display map.
//
// This file manages the list of these events built by the campaign

#include <stdio.h>
#include <tchar.h>
#include "mesg.h"
#include "Team.h"
#include "CUIEvent.h"
#include "Entity.h"
#include "Campaign.h"

// =================================
// A few globals
// =================================

// ==============================================
// A couple of global functions. Note: most of
// The real functionality is in CmpClass
// ==============================================

void SendCampUIMessage (FalconCampEventMessage *message)
	{
	FalconSendMessage(message,FALSE);			// KCK NOTE: We can miss a few of these and live
	}

// This frees the memory for the list passed
void DisposeEventList (CampUIEventElement* root)
	{
	CampUIEventElement* deadEvent;

	while (root)
		{
		deadEvent = root;
		root = root->next;
		delete deadEvent->eventText;
		deadEvent->eventText = NULL;
		delete deadEvent;
		}
	}

void TrimEventList (CampUIEventElement *root, int length)
	{
	int			i;

	for (i=1; i<length; i++)
		{
		if (root)
			root = root->next;
		}
	if (root)
		{
		DisposeEventList(root->next);
		root->next = NULL;
		}
	}
