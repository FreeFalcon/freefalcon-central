#ifndef _BEAMRIDER_H
#define _BEAMRIDER_H

#include "sensclas.h"

class FalconEntity;
class SimObjectType;
class SimMoverClass;


class BeamRiderClass : public SensorClass
{
  public :
	BeamRiderClass( int type, SimMoverClass* body );
	virtual ~BeamRiderClass( void );
	
	virtual SimObjectType* Exec( SimObjectType *targetList );
	void SetGuidancePlatform (FalconEntity*);
	FalconEntity* Getplatform (void) {return radarPlatform;};
  protected:
	FalconEntity			*radarPlatform;			// The platform mounting our illumination radar

	VU_ID					lastChaffID;			// ID of last chaff we dealt with

	UInt32					lastTargetLockSend;		// When did we last send a lock message?

	unsigned long chafftime;//me123
	virtual void	ConsiderDecoy( SimObjectType *target );

	// Target notification function (triggers RWR and Digi responses)
/* S.G. I NEED IT PUBLIC  */ public :	void			SendTrackMsg (SimObjectType* tgtptr, unsigned int trackType, unsigned int hardpoint = 0);		// Send a track message
};

#endif // _BEAMRIDER_H

