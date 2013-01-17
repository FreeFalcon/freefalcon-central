/***************************************************************************\
    ScriptNames.cpp
    Scott Randolph
    April 5, 1998

    Provides custom code for use by specific BSPlib objects.
\***************************************************************************/
#include <stdio.h>
#include <string.h>
#include "Scripts.h"
#include "ScriptNames.h"


typedef struct ScriptNameRecord {
	char	*name;
	int		number;
} ScriptNameRecord;


// The list (add new scripts here)
ScriptNameRecord	ScriptNameList[] = {
	{ "UH1.ANS",		 0 },
	{ "Rotate.ANS",		 0 },
	{ "AH64.ANS",		 1 },
	{ "Hokum.ANS",		 2 },
	{ "OneProp.ANS",	 3 },
	{ "C130.ANS",		 4 },
	{ "E3.ANS",			 5 },
	{ "VASIF",			 6 },
	{ "VASIN",			 7 },
	{ "Chaff.ANS",		 8 },
	{ "Beacon.ANS",		 9 },
	{ "CHUTEDED.ANS",	10 },
	{ "LongBow.ANS",	11 },
	{ "Cycle2.ANS",		12 },
	{ "Cycle4.ANS",		13 },
	{ "Cycle10.ANS",	14 },
	{ "TStrobe.ANS",	15 },	
	{ "TU95.ANS",	16 },
	{ "Meatball.ANS", 17},
	{ "ComplexProp", 18},
};

// The length of the list for validation
int					ScriptNameListLen = sizeof(ScriptNameList)/sizeof(*ScriptNameList);


// This function retrieves the script number for a given index and verifies that the script exists
int GetScriptNumberFromName( char *name )
{
	int i;

	for (i=0; i<ScriptNameListLen; i++) {
		if (stricmp( name, ScriptNameList[i].name ) == 0) {

			// We found a match, so return its number if the script exists
			if ((ScriptNameList[i].number < ScriptArrayLength) && 
				(ScriptArray[ScriptNameList[i].number])) {
				return ScriptNameList[i].number;
			}

		}
	}

	// We never found an acceptable match
	printf("Undefined script %s referenced.  Add to Scripts.CPP and ScriptNames.CPP\n", name);
	return -1;
}
