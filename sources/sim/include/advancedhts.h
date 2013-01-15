#ifndef _ADVHARMPOD_H
#define _ADVHARMPOD_H

#include "HarmPod.h"


class AdvancedHarmTargetingPod : public HarmTargetingPod
{
  public:
	AdvancedHarmTargetingPod(int idx, SimMoverClass* newPlatform);
	virtual ~AdvancedHarmTargetingPod(void);
	
	virtual void			HADDisplay (VirtualDisplay *newDisplay);
	virtual void			HADExpDisplay (VirtualDisplay *newDisplay);
	virtual void			HASDisplay (VirtualDisplay *newDisplay);
	virtual void			POSDisplay (VirtualDisplay *newDisplay);
	virtual void			HandoffDisplay (VirtualDisplay *newDisplay);

private:
	void					DrawEmitter ( GroundListElement* tmpElement, float &displayX, float &displayY, float &origDisplayY );
	void					GetOsbPos ( int &OsbIndex, float &OsbPosX, float &OsbPosY );

	MissileClass			*curMissile;
	WayPointClass			*curTargetWP, *prevTargetWP;
	float					curTOF, prevTOF;
	SIM_ULONG				timer;
	int						prevTargteSymbol, prevWPNum;
	bool					missileLaunched;
};

#endif // _ADVHARMPOD_H
