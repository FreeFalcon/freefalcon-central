#include "stdhdr.h"
#include "F4Vu.h"
#include "missile.h"
#include "Graphics\Include\display.h"
#include "simveh.h"
#include "airunit.h"
#include "simdrive.h"
#include "fcc.h"
#include "object.h"
#include "team.h"
#include "entity.h"
#include "simmover.h"
#include "soundfx.h"
#include "classtbl.h"
#include "rwr.h"
#include "EasyHTS.h"

void HarmTargetingPod::Display (VirtualDisplay* activeDisplay)
{
	/*float			 displayY;*/
	//float			x2, y2;
	//float			cosAng, sinAng;
	//mlTrig			trig;
	//int				boxed;
	//UInt32			color;
	FireControlComputer* FCC = ((SimVehicleClass*)platform)->GetFCC();
}


EasyHarmTargetingPod::EasyHarmTargetingPod(int idx, SimMoverClass* self) : HarmTargetingPod (idx, self)
{
}


EasyHarmTargetingPod::~EasyHarmTargetingPod (void)
{
}


void EasyHarmTargetingPod::Display (VirtualDisplay* activeDisplay)
{
	float			displayX, displayY;
	float			x2, y2;
	float			cosAng, sinAng;
	GroundListElement*	tmpElement;
	mlTrig			trig;
	int				boxed;
	UInt32			color;
	FireControlComputer* FCC = ((SimVehicleClass*)platform)->GetFCC();

	
	// Let the base class do the basics
	HarmTargetingPod::Display( activeDisplay ); // FRB

	// Set up the trig functions of our current heading
	mlSinCos (&trig, platform->Yaw());
	cosAng = trig.cos;
	sinAng = trig.sin;
	
	// Draw all known emmitters
	for (tmpElement = FCC->GetFirstGroundElement(); tmpElement; tmpElement = tmpElement->GetNext())
	{
	    if (tmpElement->BaseObject() == NULL) continue;
		// Compute the world space oriented, display space scaled, ownship relative postion of the emitter
		y2 = (tmpElement->BaseObject()->XPos() - platform->XPos()) * FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
		x2 = (tmpElement->BaseObject()->YPos() - platform->YPos()) * FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;

		// Rotate it into heading up space and translate it down to deal with our vertical offset
		displayX = cosAng * x2 - sinAng * y2;
		displayY = sinAng * x2 + cosAng * y2 + HTS_Y_OFFSET;

		// Skip this one if its off screen
		if ((fabs(displayX) > 1.0f) || (fabs(displayY) > 1.0f)) {
			continue;
		}

		// Set the symbols draw intensity based on its state
		if (tmpElement->IsSet(GroundListElement::Launch)) {
			color = 0x00FFFFFF;
			boxed = flash ? 2 : 0;
		} else if (tmpElement->IsSet(GroundListElement::Track)) {
			color = 0x00FFFFFF;
			boxed = 2;
		} else if (tmpElement->IsSet(GroundListElement::Radiate)) {
			color = 0x00FFFFFF;
			boxed = 0;
		} else {
			color = 0x00808080;
			boxed = 0;
		}

		// Set the symbols draw color based on its team
		if ( TeamInfo[platform->GetTeam()]->TStance(tmpElement->BaseObject()->GetTeam()) == War )
		{
			color &= 0x000000FF;		// Red means at war
		}
		else if ( TeamInfo[platform->GetTeam()]->TStance(tmpElement->BaseObject()->GetTeam()) == Allied )
		{
			color &= 0x00FF0000;		// Blue means our team
		}
		else
		{
			color &= 0x0000FF00;		// Green means everyone else
		}

		
		// Adjust our display location and draw the emitter symbol
		display->AdjustOriginInViewport (displayX, displayY);
		display->SetColor( color );
		DrawEmitterSymbol( tmpElement->symbol, boxed );

		// Mark the locked target
		if (lockedTarget && tmpElement->BaseObject() == lockedTarget->BaseData())
		{
			display->SetColor( 0xFF00FF00 );
			display->Circle (0.0F, 0.0F, 0.08F);
		}

		display->AdjustOriginInViewport (-displayX, -displayY);
	}
	
	// Reset our display parameters
	display->SetColor( 0xFF00FF00 );
}
