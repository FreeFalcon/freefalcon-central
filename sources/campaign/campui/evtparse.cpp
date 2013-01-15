// EventReader.cpp : Defines the class behaviors for the application.
//

#include <stdio.h>
#include <tchar.h>
#include "mesg.h"
#include "CmpGlobl.h"
#include "EvtParse.h"

// =================================
// Class functions
// =================================

// none

// =================================
// Our reading and parsing functions
// =================================

/*
extern int CreateCampFile(char *filename, char* path);
extern FILE* OpenCampFile (char *filename, char *ext, char *mode);
extern void CloseCampFile (FILE *fp);

// Builds an event list read from filename. Only adds events who's type is set in types param.
// ie: if you want to add event types 1 and 3, types should be: 0001010 = 0x12
EventElement* ReadEventFile (char* filename, uchar types[EVT_MESSAGE_BITS])
	{
	FILE* inFile;
	EventElement* tmpEvent;
	EventElement* curEvent;
	EventElement* rootEvent = NULL;
	int	gotmem=0;

	inFile = OpenCampFile(filename, "acm", "rb");
	if (inFile)
		{
		curEvent = NULL;
		tmpEvent = new EventElement;
		while (fread(&(tmpEvent->idData), sizeof (EventIdData), 1, inFile) > 0)
			{
			tmpEvent->next = NULL;
			if (!gotmem)
				tmpEvent->eventData = new unsigned char[tmpEvent->idData.size];
			fread(tmpEvent->eventData, tmpEvent->idData.size, 1, inFile);
			if (types[tmpEvent->idData.type >> 3] & (0x01 << (tmpEvent->idData.type & 0x0007)))
				{
				// This is an event we're interested in, let's add it to our list
				if (rootEvent == NULL)
					rootEvent = tmpEvent;
				else
					curEvent->next = tmpEvent;
				curEvent = tmpEvent;
				tmpEvent = new EventElement;
				gotmem = 0;
				}
         else
         {
            delete [] tmpEvent->eventData;
         }

			}
		if (gotmem)
			delete tmpEvent->eventData;
		delete tmpEvent;
		CloseCampFile(inFile);
		}
	return rootEvent;
	}
*/

void InsertEventToList (EventElement* theEvent, EventElement* baseEvent)
{
	ShiAssert (baseEvent && theEvent);

	if (baseEvent)
	{
		theEvent->next = baseEvent->next;
		baseEvent->next = theEvent;
	}
}

void DisposeEventList (EventElement* rootEvent)
	{
	EventElement* tmpEvent;

	while (rootEvent)
		{
		tmpEvent = rootEvent;
		rootEvent = rootEvent->next;
//		if (tmpEvent->eventData)
//			delete [] tmpEvent->eventData;
//		tmpEvent->eventData;
		delete tmpEvent;
		}
	}

