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


extern const float RANGE_POSITION;

void FlcsMfdDrawable::Display (VirtualDisplay* newDisplay)
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
    static const float arrowH = 0.0375f;
    static const float arrowW = 0.0433f;
    display = newDisplay;

    LabelButton(6, "LOC", "D234");
    LabelButton(8, "HEX");

    float x5, y5;
    float x7, y7;
    GetButtonPos(5, &x5, &y5);
    GetButtonPos(7, &x7, &y7);

    display->AdjustOriginInViewport(x5 + arrowW, y5 + arrowH/2);
    display->Tri (0.0F, arrowH, arrowW, -arrowH, -arrowW, -arrowH);
	/*------------*/
	/* down arrow */
	/*------------*/
    display->CenterOriginInViewport();    
    display->AdjustOriginInViewport( x7 + arrowW, y7 - arrowH/2);
    display->Tri (0.0F, -arrowH, arrowW, arrowH, -arrowW, arrowH);
    display->CenterOriginInViewport();    

    //LabelButton (19, "MORE");
    display->TextCenterVertical (0.0f, 0.0f, "CONTENT");
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

void FlcsMfdDrawable::PushButton (int whichButton, int whichMFD)
{
    MfdDrawable::PushButton(whichButton, whichMFD);
}
