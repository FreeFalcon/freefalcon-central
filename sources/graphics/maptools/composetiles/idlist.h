/*******************************************************************************\
	Unique ID list manager class -- Used by the ComposeTiles tool.

	Scott Randolph
	Spectrum HoloByte
	May 12, 1997
\*******************************************************************************/
#ifndef _IDLIST_H_
#define _IDLIST_H_

#include "TileList.h"


const int		maxCodes = 0xFFFF;


class IDListManager {
  public:
	IDListManager()							{ numCodes = 0; tileFile = -1; };
	~IDListManager()						{};

	void	Setup( char *path, TileListManager *tileListmgr );
	void	Cleanup( void );

	WORD	GetIDforCode( __int64 code );
	DWORD	GetNumNewCodes( void )				{ return numCodes; };
	DWORD	GetNumTotalCodes( void )			{ return numCodes+startID; };

  protected:
	void	WriteCompositeTile( WORD NWcode, WORD NEcode, WORD SWcode, WORD SEcode );

  protected:
	DWORD			startID;
	__int64			Codes[maxCodes];
	DWORD			numCodes;

	TileListManager	*pTileListMgr;
	int				tileFile;
};

#endif // _IDLIST_H_