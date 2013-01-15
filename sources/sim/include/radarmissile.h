#ifndef _RADARMISSILE_H_
#define _RADARMISSILE_H_

#include "radar.h"


class RadarMissileClass : public RadarClass
{
  public :
	RadarMissileClass (int type, SimMoverClass* parentPlatform);
	virtual ~RadarMissileClass()	{};

	virtual SimObjectType* Exec( SimObjectType* targetList );

  protected:
	virtual SimObjectType* ConsiderDecoy( SimObjectType *target, BOOL canGuide );

	VU_ID	lastChaffID;	// ID of chaff bundle our target most recently dropped (or FalconNullId)

	BOOL	couldGuide;		// Were we detecting the target last frame?
};

#endif // _RADARMISSILE_H_