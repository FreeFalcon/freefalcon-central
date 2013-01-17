#ifndef _EASYHARMPOD_H
#define _EASYHARMPOD_H

#include "HarmPod.h"


class EasyHarmTargetingPod : public HarmTargetingPod
{
  public:
	EasyHarmTargetingPod(int idx, SimMoverClass* newPlatform);
	virtual ~EasyHarmTargetingPod(void);
	
	virtual void			Display (VirtualDisplay *newDisplay);
};

#endif // _EASYHARMPOD_H
