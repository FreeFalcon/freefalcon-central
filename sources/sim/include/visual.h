#ifndef _VISUAL_MODEL_H
#define _VISUAL_MODEL_H

#include "sensclas.h"
#include "visualData.h"

class VisualClass : public SensorClass
{
  public :
	VisualClass( int type, SimMoverClass* self );
	virtual ~VisualClass( void );

	virtual int		CanSeeObject (SimObjectType*);		// Is the object within my field of view?
/* ADDED BY S.G. */	inline virtual int		CanSeeObject (float az, float el);		// Is the given value in my field of view
	virtual float	GetSignature (SimObjectType*);		// What is the signal strength?
	virtual int		CanDetectObject (SimObjectType*);	// Is signal strong enough?
	VisualDataType*	GetTypeData( void)						{return typeData; }

	enum VISUALType { EYEBALL, TARGETINGPOD };
	int VisualType( void )							{ return visualType; };

  protected :
	VisualDataType *typeData;
	VISUALType visualType;
};

#endif

