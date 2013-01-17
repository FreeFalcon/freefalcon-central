/***************************************************************************\
    LODBuildList.h
    Scott Randolph
    February 16, 1998

    Provides build time services for sharing LODs among parent
	objects.
\***************************************************************************/
#ifndef _LODBUILDLIST_H_
#define _LODBUILDLIST_H_

#include "ObjectLOD.h"

extern class BuildTimeLODList	TheLODBuildList;


typedef struct BuildTimeLODEntry {
	float	radius;						// \		This stuff
	float	minX, maxX;					//  \		exactly mirrors
	float	minY, maxY;					//   \		the information
	float	minZ, maxZ;					//    \		the parent objects
										//     \	will want to
	int		nSwitches;					//      \	accumulate.
	int		nDOFs;						//       >	
	int		nSlots;						//      /	
	int		nDynamicCoords;				//     /		
	int		nTextureSets;				//    /		
										//   /		
	Ppoint	*pSlotAndDynamicPositions;	//  /

	int		flags;						// This stuff will be moved into the LOD record

	char	filename[100];	// Source file name with no extension.

	int							index;
	int bflags;
	struct BuildTimeLODEntry	*prev;
	struct BuildTimeLODEntry	*next;
} BuildTimeLODEntry;


class BuildTimeLODList {
  public:
	  BuildTimeLODEntry * AddExisiting (ObjectLOD *op, ObjectParent *parent);
	BuildTimeLODList()	{ head = tail = NULL; };
	~BuildTimeLODList()	{};

	BuildTimeLODEntry*	AddReference( char *filename );
	BOOL				BuildLODTable();
	void				WriteLODData( int file );		// Must call before WriteLODHeaders
	void				WriteLODHeaders( int file );	// Must call after WriteLODData

  protected:
	BuildTimeLODEntry	*head;
	BuildTimeLODEntry	*tail;
};

#endif //_LODBUILDLIST_H_