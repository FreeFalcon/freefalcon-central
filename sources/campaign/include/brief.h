//
// Brief and Debrief reading functions.
// 
// These read scripted brief and debrief files, and evaluate their instructions
//

#ifndef BRIEF_H
#define BRIEF_H

#include <tchar.h>

// =======================================
// #defines and forward declarations
// =======================================

#define MAX_STACK					10			// levels of #IF statements we can handle
#define MAX_STRLEN_PER_TOKEN		256			// maximum string length of each token
#define MAX_STRLEN_PER_PARAGRAPH	1024		// maximum size of default string buffer

#define GBD_PLAYER_ELEMENT			1
#define GBD_PLAYER_TASK				2
#define GBD_PACKAGE_LABEL			3
#define GBD_PACKAGE_MISSION			4
#define GBD_PACKAGE_ELEMENT_NAME	5
#define GBD_PACKAGE_ELEMENT_TASK	6
#define GBD_PACKAGE_STPTHDR	7
#define GBD_PACKAGE_STPT	8


class C_Window;

// =======================================
// Global externs
// =======================================

// =======================================
// Access functions
// =======================================

extern void BuildCampBrief (C_Window *win);

extern void BuildCampDebrief (C_Window *win);

extern void BuildCampBrief (_TCHAR *brief_string);

extern void BuildCampDebrief (_TCHAR *brief_string);

extern int GetBriefingData (int query, int data, _TCHAR *buffer, int len = 128);

// These add strings to a buffer
extern void AddStringToBuffer (_TCHAR *string, _TCHAR *buffer);				// _tcscat();

extern void AddIndexedStringToBuffer (int sid, _TCHAR *buffer);

extern void AddNumberToBuffer (int num, _TCHAR *buffer);					// _tsprinf("%d",x);

extern void AddNumberToBuffer (float num, int decimals, _TCHAR *buffer);	// _tsprinf("%.df",x);

extern void AddTimeToBuffer (CampaignTime time, _TCHAR *buffer, int seconds = TRUE);

extern void AddLocationToBuffer (char type, GridIndex x, GridIndex y, _TCHAR *buffer);

extern void ConstructOrderedSentence (short maxlen,_TCHAR *buffer, _TCHAR *format, ... );

#endif
