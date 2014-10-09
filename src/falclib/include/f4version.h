#ifndef _FALCON4_VERSION_H
#define _FALCON4_VERSION_H

// dannycoh -removed f4VerNum.h and changed version to reflect FFOSP7.
// dannycoh -removed because it is shown only during build.
//const int FfMajorVersion = 7;
//const int FfMinorVersion = 0;
//const int FfLanguage = 1;
//const int FfBuildNumber = 0;
//#define FfLanguageAbbreviated "US"
//#define FfBuildType "DEBUG "

//#define STRINGERL(A) #A
//#define STRINGER(A) STRINGERL(A)
//#define Charmakerl(A) #@A
//#define Charmaker(A) Charmakerl(A)

//#define VERSION_COMMSTRING    "FreeFalcon 7.0 " FfBuildType "(" FfLanguageAbbreviated ")\0"
//#define VERSION_FILEDESC      "FreeFalcon 7.0 " FfBuildType "(" FfLanguageAbbreviated ")\0"
//#define VERSION_FILEVERSION    STRINGER(FfMajorVersion) ",0," STRINGER(FfMinorVersion) "," STRINGER (FfLanguage)
//#define VERSION_PRODUCTVERSION STRINGER(FfMajorVersion) ".0" STRINGER(FfMinorVersion) STRINGER (FfLanguage)
//#define VERSION_SPECIAL        FfBuildType FfLanguageAbbreviated " Edition"

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

// dannycoh -removed because it is shown only during build.
//#pragma message ( VERSION_FILEDESC )
//#pragma message ( VERSION_FILEVERSION )
//#pragma message ( VERSION_PRODUCTVERSION )
//#pragma message ( VERSION_SPECIAL )

extern int gLangIDNum;

#endif

