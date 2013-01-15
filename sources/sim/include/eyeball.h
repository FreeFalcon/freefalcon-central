#ifndef _EYEBALL_MODEL_H
#define _EYEBALL_MODEL_H

#include "visual.h"

class EyeballClass : public VisualClass
{
  public :
	EyeballClass (int type, SimMoverClass* self);
	virtual ~EyeballClass (void);

	virtual SimObjectType* Exec (SimObjectType* targetList);
	
  protected :
	virtual float	GetSignature (SimObjectType*);
};

#endif

