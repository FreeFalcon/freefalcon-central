#include "stdhdr.h"
#include "mfd.h"
#include "hud.h"
#include "sms.h"
#include "simdrive.h"
#include "camp2sim.h"
#include "Graphics\Include\render2d.h"
#include "lantirn.h"
#include "otwdrive.h"	//MI
#include "cpmanager.h"	//MI
#include "icp.h"		//MI
#include "aircrft.h"	//MI
#include "fcc.h"		//MI
#include "radardoppler.h" //MI

//MI
void DrawBullseyeCircle(VirtualDisplay* display, float cursorX, float cursorY);


void FlirMfdDrawable::Display (VirtualDisplay* newDisplay)
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
	else
	{
		theRadar->GetCursorPosition (&cX, &cY);
	}

    display = newDisplay;
    
    if (!theLantirn->IsFLIR()) {
	int ofont = display->CurFont();
	display->SetFont(2);
	display->TextCenterVertical (0.0f, 0.25f, "FLIR");
	display->TextCenterVertical (0.0f, 0.0f, "Off");
	display->SetFont(ofont);
    }
    else {
	LabelButton(5, "Exp", "Fov");
	char fovstr[100];
	sprintf(fovstr, "%.1f", theLantirn->GetFov()*RTD);
	display->TextCenterVertical (0.0f, 0.25f, fovstr);
	LabelButton(19, "Dec", "Fov");

	sprintf(fovstr, "%.1f", theLantirn->GetDPitch()*RTD);
	display->TextCenterVertical (0.0f, 0.0f, fovstr);
	LabelButton(6, "Inc", "Pitch");
	LabelButton(18, "Dec", "Pitch");
    }
	//MI changed
	if(g_bRealisticAvionics)
	{
		if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
			OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
		{
			DrawBullseyeCircle(display, cX, cY);
		}
		else
			DrawReference(MfdDisplay[OnMFD()]->GetOwnShip());
	}
	else
		DrawReference (MfdDisplay[OnMFD()]->GetOwnShip());
    BottomRow();
}

void FlirMfdDrawable::PushButton (int whichButton, int whichMFD)
{
    float var;
    switch (whichButton) {
    case 5: // widen FOV
	var = theLantirn->GetFovScale();
	var *= 1.1f;
	theLantirn->SetFovScale(var);
	break;
    case 6: // pitch up
	var = theLantirn->GetDPitch();
	var += 1.0 * DTR;
	theLantirn->SetDPitch(var);
	break;
    case 18: // pitch down
	var = theLantirn->GetDPitch();
	var -= 1.0 * DTR;
	theLantirn->SetDPitch(var);
	break;
    case 19: // shrink var
	var = theLantirn->GetFovScale();
	var /= 1.1f;
	theLantirn->SetFovScale(var);
	break;
    default:
	MfdDrawable::PushButton(whichButton, whichMFD);
	break;
    }
}
