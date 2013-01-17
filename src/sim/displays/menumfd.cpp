#include "stdhdr.h"
#include "mfd.h"
#include "Aircrft.h"
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


struct MfdMenuButtons {
    char *label1, *label2;
    enum {
	ModeReset = -1,
	    ModeParent = -2,
	    ModeNoop = -3,
    };
    int nextMode;
};
#define PARENT { NULL, NULL, MfdMenuButtons::ModeParent }
#define NOENTRY { NULL, NULL, MfdMenuButtons::ModeNoop }

static const MfdMenuButtons mainpage[20] =
{
    {"BLANK", NULL, MFDClass::MfdOff}, //1
    //{NULL, NULL, MFDClass::HUDMode},
	{NULL, NULL, MFDClass::HADMode}, // RV - I-Hawk
	{NULL, NULL, MFDClass::RWRMode},
    {"RCCE", NULL, MfdMenuButtons::ModeNoop},
	{"RESET", "MENU", MfdMenuButtons::ModeReset},	// 5
    {NULL, NULL, MFDClass::SMSMode},
    {NULL, NULL, MFDClass::FCCMode},
    {NULL, NULL, MFDClass::DTEMode},
    {NULL, NULL, MFDClass::TestMode},
    {NULL, NULL, MFDClass::FLCSMode},	//10
    PARENT,
    PARENT,
    PARENT,
    PARENT,
    PARENT,
    {NULL, NULL, MFDClass::FLIRMode},
    {NULL, NULL, MFDClass::TFRMode},
    {NULL, NULL, MFDClass::WPNMode},
    {NULL, NULL, MFDClass::TGPMode},
    {NULL, NULL, MFDClass::FCRMode},	// 20
};


static const MfdMenuButtons resetpage[20] =
{ // reset page menu
    {"BLANK", NULL, MFDClass::MfdOff},    // 1
	NOENTRY,
	NOENTRY,
	NOENTRY,
    {"RESET", "MENU", MfdMenuButtons::ModeReset},    // 5
    {"SBC DAY", "RESET", MfdMenuButtons::ModeNoop},
    {"SBC NIGHT", "RESET", MfdMenuButtons::ModeNoop},
    {"SBC DFLT", "RESET", MfdMenuButtons::ModeNoop},
    {"SBC DAY", "SET", MfdMenuButtons::ModeNoop},
    {"SBC NIGHT", "SET", MfdMenuButtons::ModeNoop},    // 10
    PARENT,
    PARENT,
    PARENT,
    PARENT,
    PARENT,    // 15
    {NULL, NULL, MfdMenuButtons::ModeNoop},
    {NULL, NULL, MfdMenuButtons::ModeNoop},
    {"NVIS", "OVRD", MfdMenuButtons::ModeNoop},
    {"PROG DCLT", "RESET", MfdMenuButtons::ModeNoop},
    {"MSMD", "RESET", MfdMenuButtons::ModeNoop},    // 20
};

struct MfdPage {
    const MfdMenuButtons *buttons;
};
static const MfdPage mfdpages[] = {
    {mainpage},
    {resetpage},
};
static const int NMFDPAGES = sizeof(mfdpages) / sizeof(mfdpages[0]);

void MfdMenuDrawable::Display (VirtualDisplay* newDisplay)
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
    if (g_bRealisticAvionics) {
	const MfdMenuButtons *mb = mfdpages[mfdpage].buttons;
	MFDClass::MfdMode curmode = MfdDisplay[OnMFD()]->CurMode();
	for (int i = 0; i < 20; i++) {
	    if (mb[i].label1)
		LabelButton(i, mb[i].label1, mb[i].label2, mb[i].nextMode == curmode);
	    else if (mb[i].nextMode >= 0)
		LabelButton(i, MFDClass::ModeName(mb[i].nextMode), NULL, mb[i].nextMode == curmode);
	    else if (mb[i].nextMode == MfdMenuButtons::ModeParent)
		MfdDrawable::DefaultLabel(i);
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
    }
    else {
	char tmpStr[12];//me123 addet becourse of bingo display
	LabelButton (1, "HSD");
	LabelButton (2, "FCR");
	LabelButton (3, "SMS");
	sprintf (tmpStr, "BINGO %.0f", playerAC->bingoFuel);//me123 status ok
	LabelButton (15,  tmpStr);//me123 status ok
	LabelButton (11, "RWR");
	LabelButton (12, "HUD");
	LabelButton (13, "MENU", NULL, 1);
    }
}

void MfdMenuDrawable::PushButton (int whichButton, int whichMFD)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (g_bRealisticAvionics) {
		int mode;
		switch(mode = mfdpages[mfdpage].buttons[whichButton].nextMode)
		{
		case MfdMenuButtons::ModeNoop:
			break;
			
		case MfdMenuButtons::ModeReset:
			mfdpage = 1 - mfdpage;
			break;
			
		case MfdMenuButtons::ModeParent:
			MfdDrawable::PushButton(whichButton, whichMFD);
			break;
			
		default:
			MfdDisplay[whichMFD]->SetNewMode((MFDClass::MfdMode)mode);
			break;
		}
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
	    
	case 15:
	    {
		float bingo = playerAC->bingoFuel;
		
		if ( bingo >= 0 )
		{
		    if (bingo <= 1000.0f)
			playerAC->bingoFuel +=  100.0f;
		    else if (bingo < 3000.0f)
			playerAC->bingoFuel += 200.0f;
		    else if (bingo < 10000.0f)
			playerAC->bingoFuel+= 500.0f;
		    else
			playerAC->bingoFuel = 0.0f;
		}
	    }
	    break;
	}
	// Check other MFD if needed;
	if (nextMode != MFDClass::MfdOff && (otherMfd < 0 || MfdDisplay[otherMfd]->mode != nextMode))
	    MfdDisplay[whichMFD]->SetNewMode(nextMode);
    }
}
