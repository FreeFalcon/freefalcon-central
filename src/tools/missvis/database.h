/***************************************************************************\
	DataBase.h
	Scott Randolph
	November 18, 1998

	Read in and store missile test data.
\***************************************************************************/
#ifndef _DATABASE_H_
#define _DATABASE_H_

#include "Utils\Types.h"


extern class DataBaseClass	TheDataBase;


typedef struct DataPoint {
	unsigned	time;
	float		x, y, z;
	float		dx, dy, dz;
	float		yawCmd, pitchCmd;
	int			targetState;
	float		range;
	float		timeToImpact;
	float		groundZ;
	float		targetX, targetY, targetZ;
} DataPoint;


class DataBaseClass {
  public:
	DataBaseClass()		{ TheData = NULL; TheDataLength = 0; };
	~DataBaseClass()	{ FreeData(); };

	void	ReadData( char *filename );
	void	FreeData( void );

	void	Process( void(*fn)( DataPoint *arg ), unsigned startAt, unsigned stopBefore );

	DataPoint	*TheData;
	int			TheDataLength;
};

#endif // _DATABASE_H_
