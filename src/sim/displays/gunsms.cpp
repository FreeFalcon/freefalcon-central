#include "stdhdr.h"
#include "SmsDraw.h"
#include "fcc.h"
#include "guns.h"
#include "Graphics\Include\display.h"
#include "hardpnt.h"
#include "simveh.h"
#include "Sms.h"
#include "aircrft.h"
#include "simdrive.h"

void SmsDrawable::GunDisplay (void)
{
    char tmpStr[12];
    int station, numRounds;
    FireControlComputer *FCC = Sms->ownship->GetFCC();

    // JPO - moved a lot of it into these two routines.
    TopRow(0);
    BottomRow();
    
	//MI
	if(g_bRealisticAvionics && FCC && FCC->GetSubMode() ==
		FireControlComputer::EEGS)
	{
		char tempstr[4] = "";
		sprintf(tempstr,"%s", Sms->FEDS == TRUE ? "ON" : "OFF");
		LabelButton (19, "SCOR", tempstr);
	}
	else if(g_bRealisticAvionics)
		LabelButton(19,"");
	else
		LabelButton (19, "SCOR", "OFF");
    
    if (Sms->ownship)
    {
	if (FCC->GetMasterMode() == FireControlComputer::Dogfight)
	    station = 0;
	else
	    station = Sms->curHardpoint;
	
	GunClass *gun = Sms->GetGun(station);
	if (gun)
	    numRounds = gun->numRoundsRemaining / 10;
	else
	    numRounds = 0;
    }
    else
	numRounds = 0;
    
	//MI I think this should only show RDY when we are in ARM, like all the other weapons
    if (numRounds)
    {
		if(!g_bRealisticAvionics)
		{
			
			sprintf (tmpStr, "%02dGUN", numRounds);
			display->TextCenter (0.3F, 0.6F, "RDY");
		}
		else
		{
			float x,y = 0;
			GetButtonPos(5, &x, &y);	//button for Gun
			sprintf (tmpStr, "%02dGUN", numRounds);
			if(Sms->MasterArm() == SMSBaseClass::Arm)
				display->TextCenter (0.3F, y, "RDY");
			else if(Sms->MasterArm() == SMSBaseClass::Sim)
				display->TextCenter (0.3F, y, "SIM");
		}
    }
    else
	sprintf (tmpStr, "00GUN");
    
    LabelButton (5, tmpStr);
}
