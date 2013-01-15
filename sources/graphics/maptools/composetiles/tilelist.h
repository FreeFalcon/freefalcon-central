/*******************************************************************************\
	Original texture tile manager class -- Used by the ComposeTiles tool.

	Scott Randolph
	Spectrum HoloByte
	May 14, 1997
\*******************************************************************************/
#ifndef _TILELIST_H_
#define _TILELIST_H_


typedef struct TileListEntry {
	WORD			texCode;
	char			name[20];
	BYTE			data[16*16];
	TileListEntry	*next;
} TileListEntry;

class TileListManager {
  public:
	TileListManager()						{ totalTiles = 0; };
	~TileListManager()						{};

	void			Setup( char *path );
	void			Cleanup( void );

	const char*		GetFileName( WORD texCode );
	const BYTE*		GetImageData( WORD texCode );
	const DWORD*	GetSharedPalette( void )		{ return palette; };
	const int		GetTileCount( void )			{ return totalTiles; };

	void			WriteSharedPaletteData( int TargetFile );

  protected:
	void			ReadImageData( char *filename, BYTE *target, DWORD size );

  protected:
	TileListEntry	*tileListHead;
	int				totalTiles;

	DWORD		*palette;
};

#endif // _TILELIST_H_