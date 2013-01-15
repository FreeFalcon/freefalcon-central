#ifndef _HARMSEEKER_H_
#define _HARMSEEKER_H_

#include "rwr.h"
#include "harmpod.h"

class HarmSeekerClass : public RwrClass
{
  public :
	HarmSeekerClass (int type, SimMoverClass* parentPlatform);
	virtual ~HarmSeekerClass()	{};

	virtual SimObjectType* Exec( SimObjectType* targetList );

	virtual int ObjectDetected( FalconEntity*, int, int dummy = 0 )	{return FALSE;}; // 2002-02-09 MODIFIED BY S.G. Added the unused dummy var

  protected:
	float	driftRateX;		// What is the drift rate if signal is lost
	float	driftRateY;		// (in units of feet/second)

	bool	launched;
	bool	launchedInPOS;
	bool	handedoff;


	BOOL	couldGuide;		// Were we detecting the target last frame?
};

#endif // _HARMSEEKER_H_