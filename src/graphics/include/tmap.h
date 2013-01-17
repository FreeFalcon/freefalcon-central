/***************************************************************************\
    Tmap.h
    Scott Randolph
    August 21, 1995

    Top level class which manages the various levels of detail representing
    our map of terrain information.
\***************************************************************************/
#ifndef _TMAP_H_
#define _TMAP_H_

#include "Ttypes.h"
#include "Tlevel.h"



// The global terrain database used by everyone
extern class TMap	TheMap;


class TMap {
  public:
	TMap()	{ initialized = FALSE; };
	~TMap()	{ initialized = FALSE; };

	int		Setup( const char *mapPath );
	void	Cleanup( void );

	// Flag to indicate that this map has or has not been initialized
	BOOL	IsReady( void )		{ return initialized; };

	// Provide access to the levels which make up this map
	TLevel*	Level( int level )		{ ShiAssert( level < nLevels );  return (&Levels[level]); };
	int		NumLevels( void )		{ return nLevels; }
	int		LastNearTexLOD( void )	{ return lastNearTexturedLOD; };
	int		LastFarTexLOD( void )	{ return lastFarTexturedLOD; };

	// Return the world space dimensions of this map
	float	NorthEdge()	{ ShiAssert(IsReady());  return Levels->BlocksHigh()*Levels->FTperBLOCK(); };
	float	EastEdge()	{ ShiAssert(IsReady());  return Levels->BlocksWide()*Levels->FTperBLOCK(); };
	float	WestEdge()	{ ShiAssert(IsReady());  return 0.0f; };
	float	SouthEdge()	{ ShiAssert(IsReady());  return 0.0f; };
	UINT	BlocksHigh() { ShiAssert(IsReady());  return Levels->BlocksHigh(); };
	UINT	BlocksWide() { ShiAssert(IsReady());  return Levels->BlocksWide(); };
  	float	FeetPerBlock()	{ ShiAssert(IsReady()); return Levels->FTperBLOCK(); };
	// Return a course appoximation of terrain height (should be max, positive up)
	float	GetMEA( float FTnorth, float FTeast );

	// Probably should be protected, but things are easier (faster?) this way...
	Tcolor	ColorTable[ 256 ];
	Tcolor	DarkColorTable[ 256 ];
	Tcolor	GreenTable[ 256 ];
	Tcolor	DarkGreenTable[ 256 ];

  protected:
	BOOL	initialized;

	int		lastNearTexturedLOD;
	int		lastFarTexturedLOD;

	int		nLevels;
	TLevel	*Levels;

	Int16	*MEAarray;		// Array of height values
	int		MEAwidth;		// Width of MEA array
	int		MEAheight;		// Height of MEA array
	float	FTtoMEAcell;	// Conversion factor for indexing into the array
	enum {
	    TMAP_LARGETERRAIN = 0x1, // using larger terrain maps
		TMAP_LARGEUIMAP = 0x2, // double sized squad selection map
	};
	int	flags; // misc flags
  protected:
	void	LoadMEAtable( const char *mapPath );
	void	LoadColorTable( HANDLE inputFile );

	static void TimeUpdateCallback( void *self );
	void	UpdateLighting(void);
};

#endif // _TMAP_H_
