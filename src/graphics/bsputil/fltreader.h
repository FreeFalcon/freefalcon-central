/***************************************************************************\
    FLTreader.h
    Scott Randolph
    January 29, 1998

    This is the tool which reads Multigen FLT files into our internal
	tree structure.
\***************************************************************************/
#ifndef _FLTREADER_H_
#define _FLTREADER_H_

#include "ParentBuildList.h"
#include "LODBuildList.h"


BOOL	ReadControlFlt(BuildTimeParentEntry *buildParent);
BRoot*	ReadGeometryFlt(BuildTimeLODEntry *buildLODentry);


#endif //_FLTREADER_H_