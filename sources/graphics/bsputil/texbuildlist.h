/***************************************************************************\
	TexBuildList.cpp
    Scott Randolph
    March 4, 1998

    Provides build time services for sharing textures among all objects.
\***************************************************************************/
#ifndef _TEXBUILDLIST_H_
#define _TEXBUILDLIST_H_

#include "TexBank.h"

extern class BuildTimeTextureList	TheTextureBuildList;


typedef struct BuildTimeTextureEntry {
	char	filename[20];	// Source filename of the bitmap (extension, but no path)
	int		index;
	int		refCount;

	struct BuildTimeTextureEntry	*prev;
	struct BuildTimeTextureEntry	*next;
} BuildTimeTextureEntry;


class BuildTimeTextureList {
  public:
	BuildTimeTextureList()	{ head = tail = NULL; listLen = 0; };
	~BuildTimeTextureList()	{};

	int		AddReference( char *filename );
	void	ConvertToAPL( int index );

	void	BuildPool( void );
	void	WritePool( int file );
	void	WriteTextureData( int file );
	void	Report(void);

  protected:
	BuildTimeTextureEntry	*head;
	BuildTimeTextureEntry	*tail;
	int						listLen;
	int						maxCompressedTextureSize;
};

#endif //_TEXBUILDLIST_H_