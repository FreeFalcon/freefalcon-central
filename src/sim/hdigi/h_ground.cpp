#include "stdhdr.h"
#include "hdigi.h"
#include "otwdrive.h"
#include "simveh.h"
//TJL 11/29/03
#include "campwp.h"

#define MIN_ALT 200.0F

// RV - Biker - No more need for GroundCheck
//void HeliBrain::GroundCheck(void)
//{
//	float groundAlt;
//	groundAlt = OTWDriver.GetGroundLevel(self->XPos() + (self->XDelta() ),
//   								    self->YPos() + (self->YDelta() ));
//
//
//	// if we're landing do nothing....
//	if ( onStation == Landing ||
//		 onStation == Landed ||
//		 onStation == DropOff ||
//		 onStation == PickUp)
//	{
//      	groundAvoidNeeded = FALSE;
//		AltitudeHold( groundAlt );
//
//      	return;
//	}
//
//   // get altitude in 1 secs...
//   //groundAlt = OTWDriver.GetGroundLevel(self->XPos() + (self->XDelta() ),
//   	//								    self->YPos() + (self->YDelta() ));
//
//   // In order for helicopters to not look stupid, we want to keep them
//   // at alts between 500 and 1500 ft
//	//TJL 11/28/03 Redoing Altitudes.  Keep helo's low
//
///*   if ( self->ZPos() - groundAlt > -500.0f )
//   {
//		groundAlt = groundAlt - 500.0f - ( self->ZPos() - groundAlt + 500.0f ) * 2.0f;
//		AltitudeHold( groundAlt );
//   }
//   
//   else if ( self->ZPos() - groundAlt < -1500.0f)
//	   //TJL 11/15/03 Keep choppers in the weeds
//   {
//		AltitudeHold( groundAlt -500.0f);
//   }
//*/
//
//	//TJL 11/28/03 New Altitude code
//	if (!(self->curWaypoint->GetWPFlags() & WPF_TAKEOFF))
//	{
//		// RV - Red - The Delta height fix
//		float	Delta=self->ZPos() - (groundAlt);
//		if (Delta < -500) AltitudeHold(groundAlt -500);//Max 500 feet above terrain
//		else if (Delta > -150) AltitudeHold(groundAlt -150);//Min 150 feet above terrain.
//	}
//}

// RV - Biker - No more need for PullUp
//void HeliBrain::PullUp(void)
//{
//	// level out and peg the collective
//	LevelTurn (0.0f, 0.0f, TRUE);
//	MachHold(0.0f, 0.0F, FALSE);
//	throtl = 1.0;
//}
