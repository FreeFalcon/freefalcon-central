/***************************************************************************\
    ObjectInstance.h
    Scott Randolph
    February 9, 1998

    Provides structures and definitions for 3D objects.
\***************************************************************************/
#ifndef _OBJECTINSTANCE_H_
#define _OBJECTINSTANCE_H_

#include "ObjectParent.h"


typedef	struct DOFvalue {
	float	rotation;
	float	translation;
} DOFvalue;


class ObjectInstance {
  public:
	ObjectInstance( int id );
	~ObjectInstance();

	float	Radius(void)	const	{ return ParentObject->radius; };
	float	BoxLeft(void)	const	{ return ParentObject->minY; };
	float	BoxRight(void)	const	{ return ParentObject->maxY; };
	float	BoxTop(void)	const	{ return ParentObject->minZ; };
	float	BoxBottom(void)	const	{ return ParentObject->maxZ; };
	float	BoxFront(void)	const	{ return ParentObject->maxX; };
	float	BoxBack(void)	const	{ return ParentObject->minX; };
	float	RadarSign(void)			{ return RadarSignal; };

	void	SetSwitch( int id, UInt32 value )	{ ShiAssert(id<ParentObject->nSwitches); if(id<ParentObject->nSwitches) SwitchValues[id] = value; };
	void	SetDOFrotation( int id, float r )	{ ShiAssert(id<ParentObject->nDOFs); if(id<ParentObject->nDOFs) DOFValues[id].rotation = r; };
	void	SetDOFxlation( int id, float x )	{ ShiAssert(id<ParentObject->nDOFs); if(id<ParentObject->nDOFs) DOFValues[id].translation = x; };
	void	SetSlotChild( int id, ObjectInstance *o )	{ ShiAssert(id<ParentObject->nSlots); if(id<ParentObject->nSlots)SlotChildren[id] = o; };
	void	SetTextureSet( int id );
	int	GetNTextureSet() { return ParentObject->nTextureSets; };
	void	SetDynamicVertex( int id, float dx, float dy, float dz );
	void	GetDynamicVertex( int id, float *dx, float *dy, float *dz );
	// RV - Biker
	void	GetDynamicCoords( int id, float *dx, float *dy, float *dz );

  public:
	UInt32			*SwitchValues;
	DOFvalue		*DOFValues;
	ObjectInstance	**SlotChildren;
	Ppoint			*DynamicCoords;
	int				TextureSet;
	int				id;
	bool			TexSetReferenced;
	float			RadarSignal;

	ObjectParent	*ParentObject;
};

#endif // _OBJECTINSTANCE_H_
