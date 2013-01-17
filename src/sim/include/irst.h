#ifndef _IRST_MODEL_H
#define _IRST_MODEL_H

#include "sensclas.h"
#include "irstData.h"

class IrstClass : public SensorClass
{
  public:
	IrstClass (int idx, SimMoverClass* self);
	virtual ~IrstClass (void)	{};

	virtual void SetDesiredTarget( SimObjectType* newTarget );

	virtual SimObjectType* Exec (SimObjectType* targetList);

	int		CanSeeObject( SimObjectType *target );
	int		CanDetectObject( SimObjectType *target );
	/* S.G. TO BRING typeData VISIBLE TO EVERYONE */ IRSTDataType *GetTypeData(void) { return typeData; };

  protected:
	float			GetSignature( SimObjectType *target );
	SimObjectType*	ConsiderDecoy( SimObjectType *target );
	float			GetSunFactor( SimObjectType *target );

	VU_ID			lastFlareID;

	IRSTDataType *typeData;
// 2000-11-24 MOVED HERE BY S.G. SO IT'S NOT CONSIDERED GLOBAL
	int tracking;//me123
};

#endif

