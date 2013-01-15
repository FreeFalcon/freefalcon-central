/***************************************************************************\
	PosBuildList.h
    Scott Randolph
    April 8, 1998

    Provides build time services for sharing positions among all polygons
	in an object.
\***************************************************************************/
#ifndef _POSBUILDLIST_H_
#define _POSBUILDLIST_H_

#include "PolyLib.h"


typedef enum BuildTimePosType { Static, Dynamic };

typedef struct BuildTimePosReference {
	int								*target;
	struct BuildTimePosReference	*next;
} BuildTimePosReference;


typedef struct BuildTimePosEntry {
	Ppoint						pos;
	BuildTimePosType			type;
	int							index;
	BuildTimePosReference		*refs;

	struct BuildTimePosEntry	*next;
} BuildTimePosEntry;


typedef struct SzInfo_t {
	float	radiusSquared;
	float	minX, maxX;
	float	minY, maxY;
	float	minZ, maxZ;
} SzInfo_t;


class BuildTimePosList {
  public:
	BuildTimePosList();
	~BuildTimePosList()	{};

	void	AddReference(int *target, float x, float y, float z, BuildTimePosType type);
	void	AddReference(int *target, int *source);

	Ppoint*	GetPosFromTarget(int *target);

	Ppoint*	GetPool();

	int					numTotal;
	int					numStatic;
	int					numDynamic;

	SzInfo_t			SizeInfo;

  protected:
	BuildTimePosEntry	*head;
};

#endif //_POSBUILDLIST_H_