#include "stdhdr.h"
#include "mfd.h"
#include "hud.h"
#include "sms.h"
#include "simdrive.h"
#include "camp2sim.h"
#include "Graphics/Include/render2d.h"
#include "otwdrive.h"	//MI
#include "cpmanager.h"	//MI
#include "icp.h"		//MI
#include "aircrft.h"	//MI
#include "fcc.h"		//MI
#include "radardoppler.h" //MI

//MI
void DrawBullseyeCircle(VirtualDisplay* display, float cursorX, float cursorY);

void BlankMfdDrawable::Display (VirtualDisplay* newDisplay)
{
	//MI
	float cX, cY = 0;
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(playerAC, SensorClass::Radar);
	if(!theRadar)
	{
		ShiWarning("Oh Oh shouldn't be here without a radar!");
		return;
	}

    display = newDisplay;
	//MI
	if(g_bRealisticAvionics && theRadar)
	{
		theRadar->GetCursorPosition (&cX, &cY);
	}
    if (g_bRealisticAvionics) {
	int ofont = display->CurFont();
	display->SetFont(2);
	display->TextCenterVertical (0.0f, 0.0f, "BLANK");
	display->SetFont(ofont);
	//MI changed
	if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
		OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
	{
		DrawBullseyeCircle(display, cX, cY);
	}
	else
		DrawReference(MfdDisplay[OnMFD()]->GetOwnShip());
	
	BottomRow();
    }
    else 
	display->TextCenterVertical (0.0f, 0.0f, "Off");
}

void BlankMfdDrawable::PushButton (int whichButton, int whichMFD)
{
    if (g_bRealisticAvionics) {
        MfdDrawable::PushButton(whichButton, whichMFD);
    }
    else {
	MFDClass::MfdMode nextMode = MFDClass::MfdOff;
	int otherMfd;
	
	// For in cockpit, you can't have two of the same thing
	if (whichMFD == 0)
	    otherMfd = 1;
	else if (whichMFD == 1)
	    otherMfd = 0;
	else
	    otherMfd = -1;
	

	switch (whichButton)
	{
	case 1:
	    nextMode = MFDClass::FCCMode;
	    break;
	    
	case 2:
	    nextMode = MFDClass::FCRMode;
	    break;
	    
	case 3:
	    nextMode = MFDClass::SMSMode;
	    break;
	    
	case 11:
	    nextMode = MFDClass::RWRMode;
	    break;
	    
	case 12:
	    //nextMode = MFDClass::HUDMode; 
		nextMode = MFDClass::HADMode; // RV - I-Hawk 
	    break;
	    
	}
	// Check other MFD if needed;
	if (nextMode != MFDClass::MfdOff && (otherMfd < 0 || MfdDisplay[otherMfd]->mode != nextMode))
	    MfdDisplay[whichMFD]->SetNewMode(nextMode);
    }
}
