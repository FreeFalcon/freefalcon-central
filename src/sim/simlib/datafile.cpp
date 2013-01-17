// DatFile parser support routines
#include "stdhdr.h"
#include "datafile.h"
#include "simfile.h" // to read in the extra data
#include "graphics/include/grtypes.h"
#include <falclib\include\lookuptable.h>
#include <falclib\include\token.h>

// The hard work of assigning stuff to a field.
// converts the value to the right format (int, char, float string etc)
// and assigns it to the right element of the structure
// we may have to get cleverer here one day - such as
// - quoted strings
// - escaped characters
// - new formats (like arrays).
// but thats on an as needed basis.
bool AssignField (const InputDataDesc *field, void *dataPtr, const char *value)
{
    char *cp = (char *)dataPtr + field->offset;

    switch (field->type) {
    case InputDataDesc::ID_INT:
	{
	    int *ip = (int *)cp;
	    int n;
	    if (*value == '0' && (value[1] == 'x' || value[1] == 'X')) {
		if (sscanf(value+2, "%x", &n) == 1)
		    *ip = n;
		else return false;
	    }
	    else if (isdigit(*value) || *value=='-')
		*ip = atoi(value);
	    else return false;
	}
	break;
    case InputDataDesc::ID_FLOAT:
	{
	    float *fp = (float *)cp;
	    if (isdigit(*value) || *value == '.' || *value == '-' || *value =='+')
		*fp = (float)atof(value);
	    else 
		return false;
	}
	break;
    case InputDataDesc::ID_STRING:
	{
	    char **vp = (char **)cp;
	    if (*vp) free (*vp);
	    *vp = (char *)malloc(strlen(value) + 1);
	    strcpy (*vp, value);
	}
	break;
    case InputDataDesc::ID_CHAR:
	{
	    *cp = *value;
	}
	break;
    case InputDataDesc::ID_VECTOR: // X, Y, Z
	{
	    Tpoint *tp = (Tpoint *)cp;
	    if (sscanf(value, "%g %g %g", &tp->x, &tp->y, &tp->z) != 3)
			if (sscanf(value, "%g, %g, %g", &tp->x, &tp->y, &tp->z) != 3) // MLR 12/4/2003 - Make the vector reading a little more flexible
			{
				return false;

			}
	}
	break;
	case InputDataDesc::ID_FLOAT_ARRAY:
	{
		int count,l;
	    float *fp = (float *)cp;
		char buffer[1024];
		strcpy(buffer,value);

		count=TokenI(buffer,0);
		for(l=0;l<count;l++)
		{
          fp[l]=TokenF(0,0);
		}
	}
	break;
	case InputDataDesc::ID_LOOKUPTABLE:
	{
		int l;
	    LookupTable *lut = (LookupTable *)cp;
		char buffer[1024];
		strcpy(buffer,value);

		lut->pairs=TokenI(buffer,0);
		for(l=0;l<lut->pairs;l++)
		{
			lut->table[l].input =TokenF(0,0);
			lut->table[l].output=TokenF(0,0);
		}
	}
	break;
	case InputDataDesc::ID_2DTABLE://Cobra 10/31/04 TJL
	{
	    TwoDimensionTable *lut = (TwoDimensionTable *)cp;
		char buffer[2000];
		strcpy(buffer,value);
		lut->Parse(buffer);
	}
	break;

    
	default:
	F4Assert(!"Bad format type");
	return false;
    }
    return true;
}

// helper routine to find the right description
const InputDataDesc* FindField(const InputDataDesc *desc, const char *key)
{
    while (desc->name) {
	if (strcmpi(desc->name, key) == 0)
	    return desc;
	desc ++;
    }
    return NULL;
}

//splits the line up
bool ParseField(void *dataPtr, const char *line, const InputDataDesc *desc) 
{
    char keybuf[1024];

    while (isspace(*line)) // skip leanding white space
	line ++;
    if (*line == '\0' || *line == '\n') return true; // just ignore blank lines
    const char *cp = line;
    while (*cp && !isspace(*cp))
	cp ++;
    if (*cp == '\0') // bad data
	return false;
    strncpy (keybuf, line, cp - line);
    keybuf[cp-line] = '\0';

    while (isspace(*cp))
	cp ++;
    // cp now at the start of the next bit
    const InputDataDesc *field = FindField(desc, keybuf);
    if (field)
	return AssignField(field, dataPtr, cp);
    else return false;
}

// fill in all values from defaults
void Initialise (void *dataPtr, const InputDataDesc *desc)
{
    for(; desc->name; desc ++) {
	AssignField(desc, dataPtr, desc->defvalue);
    }
}

void
FileReader::Initialise(void *dataPtr)
{
    const InputDataDesc *dp;

    for (dp = m_desc;dp->name; dp++) 
	AssignField(dp, dataPtr, dp->defvalue);
}

bool
FileReader::AssignField(const InputDataDesc *desc, void *dataPtr, const char *value)
{
    return ::AssignField(desc, dataPtr, value);
}

bool FileReader::ParseField(void *dataPtr, const char *line){
    return ::ParseField(dataPtr, line, m_desc);
}

// JPO
// Read in key value stuff from a simlibfile
// initialises the structure first to default values
// stops on end of file
bool ParseSimlibFile(void *dataPtr, const InputDataDesc *desc, SimlibFileClass* inputFile)
{
    FileReader fr(desc);
    SimlibFileName buffer;
	
    fr.Initialise(dataPtr);
    while (inputFile->ReadLine(buffer, sizeof buffer) == SIMLIB_OK) {
		if (buffer[0] == '#') continue;
		if (fr.ParseField(dataPtr, buffer) == false){
			// MLR 12/16/2003 - 
			// Who cares if one line failed!  This breaks files that have obsolete/unsupported data in them.
			// return false; 
		}
    }
    return true;
}
