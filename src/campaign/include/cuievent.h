
#ifndef CUIEVENT_H
#define CUIEVENT_H

#include <tchar.h>
#include "FalcMesg.h"
#include "Camplib.h"

// ===============================
// Defines and the message include
// ===============================

#define		CUI_MD					2			// Maximum data slots
#define		CUI_ME					2			// Maximum entity data
#define		CUI_MS					8			// Maximum string ids

#include "MsgInc\CampEventMsg.h"

// ============================
// CampUI Event Structure
// ============================

// This is the structure we make our campaign ui event list out of
typedef struct uieventnode
	{
	short			x,y;					// Location of the event (if any)
	CampaignTime	time;
	uchar			flags;
	Team			team;					// The team which benifited the most from this
	_TCHAR			*eventText;				// The text output
	uieventnode		*next;
	} CampUIEventElement;

// ============================
// Functions
// ============================

extern void DisposeEventList (CampUIEventElement* root);

extern void SendCampUIMessage (FalconCampEventMessage *message);

extern void TrimEventList (CampUIEventElement *root, int length);

#endif