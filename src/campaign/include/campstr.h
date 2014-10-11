//
// Campaign strings file.
//
// Most of these are for my tool, so don't need to be double wide, but others of
// these should be double wide and come from files.
//
// KCK, Dec 9, 1996
//

#ifndef CAMPSTR_H
#define CAMPSTR_H

#include <tchar.h>
#include "GndUnit.h"
#include "Team.h"

// These are used in my camp tool, so don't need to be UNICODE
extern char TOTStr[7][6];
extern char TargetTypeStr[7][15];
extern char OrderStr[GORD_LAST][15];
extern char FormStr[3][15];
extern char SpecialStr[3][15];
extern char Side[NUM_COUNS][3];

// These are used by FreeFalcon text string builders
extern _TCHAR ObjectiveStr[33][20];
extern _TCHAR MissStr[AMIS_OTHER][20];
extern _TCHAR WPActStr[WP_LAST][20];
extern _TCHAR CountryNameStr[NUM_COUNS][20];
extern _TCHAR gUnitNameFormat[40];

// Functions
extern _TCHAR* GetSTypeName(int domain, int type, int stype, _TCHAR buffer[]);
extern _TCHAR* GetNumberName(int nameid, _TCHAR *buffer);
extern _TCHAR* GetTimeString(CampaignTime time, _TCHAR buffer[], int seconds = TRUE);
extern void ReadIndex(char* filename);
extern void FreeIndex(void);
extern void ReadIndexedString(int sid, _TCHAR *wstr, int len);
extern void ConvertChToWCh(_TCHAR *wstr, char *str, int len);

#endif
