#include "stdhdr.h"
#include "mfd.h"
#include "hud.h"
#include "sms.h"
#include "simdrive.h"
#include "camp2sim.h"
#include "Graphics\Include\render2d.h"
#include "otwdrive.h"	//MI
#include "cpmanager.h"	//MI
#include "icp.h"		//MI
#include "aircrft.h"	//MI
#include "fcc.h"		//MI
#include "radardoppler.h" //MI

//MI
void DrawBullseyeCircle(VirtualDisplay* display, float cursorX, float cursorY);


void DteMfdDrawable::Display (VirtualDisplay* newDisplay)
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
    display->TextCenter(0.0f,0.2f, "DTC ID");
    float height = display->TextHeight();
    display->TextCenter(0.0f,0.2f - height, "126534");
    LabelButton(0, "ON");
    LabelButton(2, "LOAD");
    LabelButton(5, "FCR");
    LabelButton(6, "DLNK");
    LabelButton(9, "----", "GPS");
    BottomRow();
    LabelButton(15, "MSMD");
    LabelButton(16, "PROF");
    LabelButton(17, "INV");
    LabelButton(18, "COMM");
    LabelButton(19, "MPD");
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
}

void DteMfdDrawable::PushButton (int whichButton, int whichMFD)
{
    MfdDrawable::PushButton(whichButton, whichMFD);
}
