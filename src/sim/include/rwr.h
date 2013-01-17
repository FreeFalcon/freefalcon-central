/***************************************************************************\
    Rwr.h

    This provides the basis for all radar dector sensors.  This is the
	class that receives track messages.  It also has some basic helper
	functions to be shared by two or more cousin classes.
\***************************************************************************/
#ifndef _RWR_MODEL_H
#define _RWR_MODEL_H

#include "sensclas.h"
#include "rwrData.h"

class FalconEntity;

#define RWR_GET_STT  1
#define RWR_GET_SAM  2
#define RWR_GET_TWS  4
#define RWR_GET_RWS  8
#define RWR_GET_MASK 0xff

#define RWR_PRIORITIZE_STT  256
#define RWR_PRIORITIZE_SAM  512
#define RWR_PRIORITIZE_TWS  1024
#define RWR_PRIORITIZE_RWS  2048
#define RWR_PRIORITIZE_MASK 0xff00

class RwrClass : public SensorClass
{
  public :
	RwrClass (int type, SimMoverClass*);	// Pass in the type index and the platform this RWR is mounted upon  
	virtual ~RwrClass (void);

	virtual int ObjectDetected (FalconEntity*, int trackType, int dummy = 0) = 0; // 2002-02-09 MODIFIED BY S.G. Added the unused dummy var
	virtual void DisplayInit (ImageBuffer*);
	virtual void  GetAGCenter (float* x, float* y);	// Center of rwr ground search
	static void DrawSymbol (VirtualDisplay *display, int symbolID, int boxed=0);	// Draw the specified symbol
	//MI
	static void DrawHat(VirtualDisplay *display);

  public:
	static const int	RADIATE_CYCLE;		// How long (in ms) will we "coast" a radiating emitter
	static const int	TRACK_CYCLE;		// How long (in ms) will we "coast" a locked on emitter
	static const int	GUIDANCE_CYCLE;		// How long (in ms) will we "coast" a guiding emitter

	int CanDetectObject (SimObjectType*);				// Are emmisions strong enough to warrant annunciation?
	RwrDataType*	GetTypeData(void)	{ return typeData; } // 2002-02-11 ADDED BY S.G. Like the other SensorClasses

  protected:
	int CanSeeObject (SimObjectType*);					// Is object within my field of view?
	int BeingPainted (SimObjectType*);					// Is object illuminating us?
	int CanDetectObject (FalconEntity*);				// Are emmisions strong enough to warrant annunciation?
	void DrawEmitterSymbol (int symbolID, int boxed=0);	// Draw the specified symbol
	
  protected:
	struct RwrDataType	*typeData;
};

#endif

