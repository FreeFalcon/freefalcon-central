#ifndef _FALCON4_VERSION_H
#define _FALCON4_VERSION_H

#include "f4VerNum.h"

#define STRINGERL(A) #A
#define STRINGER(A) STRINGERL(A)
#define Charmakerl(A) #@A
#define Charmaker(A) Charmakerl(A)

#define VERSION_COMMSTRING    "FreeFalcon 6.1 " F4BuildType "(" F4LanguageAbbrev ")\0"
#define VERSION_FILEDESC      "FreeFalcon 6.1 " F4BuildType "(" F4LanguageAbbrev ")\0"
#define VERSION_FILEVERSION    STRINGER(F4MajorVersion) ",0," STRINGER(F4MinorVersion) "," STRINGER (F4Language)
#define VERSION_PRODUCTVERSION STRINGER(F4MajorVersion) ".0" STRINGER(F4MinorVersion) STRINGER (F4Language)
#define VERSION_SPECIAL        F4BuildType F4LanguageAbbrev " Edition"

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

#pragma message ( VERSION_FILEDESC )
#pragma message ( VERSION_FILEVERSION )
#pragma message ( VERSION_PRODUCTVERSION )
#pragma message ( VERSION_SPECIAL )

extern int gLangIDNum;

#endif

