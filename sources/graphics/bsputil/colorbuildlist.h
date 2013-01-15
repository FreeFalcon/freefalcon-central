/***************************************************************************\
	ColorBuildList.h
    Scott Randolph
    February 17, 1998

    Provides build time services for sharing colors among all objects.
\***************************************************************************/
#ifndef _COLORBUILDLIST_H_
#define _COLORBUILDLIST_H_

#include "ColorBank.h"

extern class BuildTimeColorList	TheColorBuildList;


typedef struct BuildTimeColorReference {
	int								*target;
	struct BuildTimeColorReference	*next;
} BuildTimeColorReference;


typedef struct BuildTimeColorEntry {
	Pcolor						color;
	BOOL						preLit;
 	int							index;
	BuildTimeColorReference		*refs;

	struct BuildTimeColorEntry	*prev;
	struct BuildTimeColorEntry	*next;
} BuildTimeColorEntry;


class BuildTimeColorList {
  public:
	  void MergeColors ();
	BuildTimeColorList()	{ head = tail = NULL; numColors = numPrelitColors = 0; };
	~BuildTimeColorList()	{};

	void	AddReference(int *target, Pcolor color, BOOL preLit);
	void	AddReference(int *target, int *source);

	void	BuildPool();
	void	WritePool( int file );
	void	Report(void);

  protected:
	BuildTimeColorEntry	*head;
	BuildTimeColorEntry	*tail;
	int					numColors;
	int					numPrelitColors;
};

#endif //_COLORBUILDLIST_H_