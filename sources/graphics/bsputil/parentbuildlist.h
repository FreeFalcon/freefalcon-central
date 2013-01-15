/***************************************************************************\
    ParentBuildList.h
    Scott Randolph
    February 16, 1998

    Provides build time services for accumulating and processing
	the list of parent objects.
\***************************************************************************/
#ifndef _PARENTBUILDLIST_H_
#define _PARENTBUILDLIST_H_

#include "ObjectParent.h"
#include "LODBuildList.h"

extern class BuildTimeParentList	TheParentBuildList;


typedef struct BuildTimeParentEntry {
	int					id;
	char				filename[100];
	BuildTimeLODEntry	**pBuildLODs;
	int					nBuildLODs;
	int bflags;
	struct BuildTimeParentEntry	*prev;
	struct BuildTimeParentEntry	*next;
} BuildTimeParentEntry;


class BuildTimeParentList {
  public:
	  void AddExisiting (ObjectParent *op, int id);
	  void MergeEntries();
	BuildTimeParentList()	{ head = tail = NULL; startpoint = 0; };
	~BuildTimeParentList()	{};

	void	AddItem( int id, char *filename );
	BOOL	BuildParentTable();
	void	WriteVersion( int file );
	void	WriteParentTable( int file );
	void	ReleasePhantomReferences( void );
	void	SetStartPoint (int n) { startpoint = n; };
  protected:
	BuildTimeParentEntry	*head;
	BuildTimeParentEntry	*tail;
	int startpoint;
};

#endif // _PARENTBUILDLIST_H_
