#ifndef _ALR56_MODEL_H
#define _ALR56_MODEL_H

#include "PlayerRwr.h"

class SimBaseClass;

class ALR56Class : public PlayerRwrClass
{
public :
	ALR56Class (int idx, SimMoverClass* self);
	virtual ~ALR56Class (void);

	virtual void PushButton (int whichButton, int whichMFD);
	
	// State Control Functions	
	virtual void ToggleAutoDrop(void)					{dropPattern	= !dropPattern;};

  protected:
	// Helper functions
	virtual float	GetLethality (FalconEntity* theObject);
	virtual void	AutoSelectAltitudePriority(void)	{ /* leave this to the player to do. */ };
};

#endif

