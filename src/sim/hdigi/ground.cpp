#include "stdhdr.h"
#include "hdigi.h"
#include "otwdrive.h"
#include "simveh.h"

#define MIN_ALT 200.0F

void HeliBrain::GroundCheck(void)
{
	float groundAlt;

	// if we're landing do nothing....
	if ( onStation == Landing ||
		 onStation == Landed ||
		 onStation == DropOff ||
		 onStation == PickUp)
	{
      	groundAvoidNeeded = FALSE;
      	return;
	}

   // get altitude in 1 secs...
   groundAlt = OTWDriver.GetGroundLevel(self->XPos() + (self->XDelta() ),
   									    self->YPos() + (self->YDelta() ));

   // In order for helicopters to not look stupid, we want to keep them
   // at alts between 500 and 1500 ft
   if ( self->ZPos() - groundAlt > -500.0f )
   {
		groundAlt = groundAlt - 500.0f - ( self->ZPos() - groundAlt + 500.0f ) * 2.0f;
		AltitudeHold( groundAlt );
   }
   else if ( self->ZPos() - groundAlt < -1500.0f )
   {
		AltitudeHold( groundAlt -1500.0f );
   }

}

void HeliBrain::PullUp(void)
{
	// level out and peg the collective
	LevelTurn (0.0f, 0.0f, TRUE);
	MachHold(0.0f, 0.0F, FALSE);
	throtl = 1.0;
}
