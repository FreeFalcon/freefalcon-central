#ifndef _FALCON4_VERSION_H
#define _FALCON4_VERSION_H

#include "f4VerNum.h"

// dannycoh - commented out - only shown during build.
//#define STRINGERL(A) #A
//#define STRINGER(A) STRINGERL(A)
//#define Charmakerl(A) #@A
//#define Charmaker(A) Charmakerl(A)

// dannycoh - commented out - never used.
//#define VERSION_COMMSTRING    "FreeFalcon " FF_BUILD_TYPE "(" FF_LANGUAGE_STRING ")\0" 

// dannycoh - commented out - only shown during build.
//#define VERSION_FILEDESC      "FreeFalcon " FF_BUILD_TYPE "(" FF_LANGUAGE_STRING ")\0"
//#define VERSION_FILEVERSION    STRINGER(FF_MAJOR_VERSION) ",0," STRINGER(FF_MINOR_VERSION) "," STRINGER (FF_LANGUAGE)
//#define VERSION_PRODUCTVERSION STRINGER(FF_MAJOR_VERSION) ".0" STRINGER(FF_MINOR_VERSION) STRINGER (FF_LANGUAGE)
//#define VERSION_SPECIAL        FF_BUILD_TYPE FF_LANGUAGE_STRING " Edition"
// dannycoh - end.

#define F4LANG_ENGLISH 1
#define F4LANG_UK 2
#define F4LANG_GERMAN 3
#define F4LANG_FRENCH 4
#define F4LANG_SPANISH 5
#define F4LANG_ITALIAN 6
#define F4LANG_PORTUGESE 7

#define F4LANG_MASCULINE 0
#define F4LANG_FEMININE 1
#define F4LANG_NEUTER 2

// dannycoh - commented out - only shown during build.
//#pragma message ( VERSION_FILEDESC )
//#pragma message ( VERSION_FILEVERSION )
//#pragma message ( VERSION_PRODUCTVERSION )
//#pragma message ( VERSION_SPECIAL )
// dannycoh - end.

extern int LanguageNumber;

#endif

