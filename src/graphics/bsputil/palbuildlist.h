/***************************************************************************\
	PalBuildList.cpp
    Scott Randolph
    March 9, 1998

    Provides build time services for sharing palettes among all objects.
\***************************************************************************/
#ifndef _PALBUILDLIST_H_
#define _PALBUILDLIST_H_

#include "Palette.h"

extern class BuildTimePaletteList	ThePaletteBuildList;


typedef struct BuildTimePaletteEntry {
	UInt32	palData[256];
	int		index;
	int		refCount;

	struct BuildTimePaletteEntry	*prev;
	struct BuildTimePaletteEntry	*next;
} BuildTimePaletteEntry;


class BuildTimePaletteList {
  public:
	BuildTimePaletteList()	{ head = tail = NULL; listLen = 0; };
	~BuildTimePaletteList()	{};

	int		AddReference( UInt32 *palData );
	void	BuildPool(void);
	void	WritePool( int file );
	void	Report(void);
	void	MergePalette ();
  protected:
	BuildTimePaletteEntry	*head;
	BuildTimePaletteEntry	*tail;
	int						listLen;
};

#endif //_PALBUILDLIST_H_