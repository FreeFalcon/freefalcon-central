#include "stdhdr.h"
#include "mfd.h"
#include "hud.h"
#include "sms.h"
#include "simdrive.h"
#include "camp2sim.h"
#include "Graphics\Include\render2d.h"
#include "Graphics\Include\canvas3d.h"
#include "Graphics\Include\tviewpnt.h"
#include "Graphics\Include\renderir.h"
#include "otwdrive.h"	
#include "cpmanager.h"
#include "icp.h"	
#include "aircrft.h"
#include "fcc.h"	
#include "radardoppler.h"
#include "laserpod.h"
#include "airframe.h"

void DrawBullseyeCircle(VirtualDisplay* display, float cursorX, float cursorY);
SensorClass* FindLaserPod (SimMoverClass* theObject);

extern bool g_bRealisticAvionics;
extern bool g_bGreyMFD;
extern bool g_bGreyScaleMFD;
extern bool bNVGmode;

int	TgpMfdDrawable::flash = FALSE;

void TgpMfdDrawable::DisplayInit (ImageBuffer* image)
{
    DisplayExit();
    
		if (!g_bGreyScaleMFD)
			g_bGreyMFD = false;

    privateDisplay = new RenderIR;
    ((RenderIR*)privateDisplay)->Setup(image, OTWDriver.GetViewpoint());
    
		if ((g_bGreyMFD) && (!bNVGmode))
			privateDisplay->SetColor(GetMfdColor(MFD_WHITE));
		else
			privateDisplay->SetColor (0xff00ff00);
   	((Render3D*)privateDisplay)->SetFOV(6.0f*DTR);
}
TgpMfdDrawable::TgpMfdDrawable()
{
	StbyMode = TRUE;
	MenuMode = FALSE;
	SP = FALSE;
}

void TgpMfdDrawable::Display (VirtualDisplay* newDisplay)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	float cX, cY = 0;
	sprintf(Str, "");
	if(!g_bRealisticAvionics) 
	{
		display = privateDisplay;
		OffMode(display);
		return;
    }
	self = ((AircraftClass*)playerAC);
	theRadar = (RadarDopplerClass*)FindSensor(playerAC, SensorClass::Radar);
	laserPod = (LaserPodClass*)FindLaserPod (playerAC->Sms->Ownship());
	pFCC = playerAC->Sms->Ownship()->GetFCC();
	Sms = playerAC->Sms;
	display = newDisplay;
	if(!theRadar || !pFCC || !self || !Sms)
	{
		ShiWarning("Oh Oh shouldn't be here without a radar or FCC or player or SMS!");
		return;
	}
	else if(!laserPod)
	{
		//display = privateDisplay;			//Wombat778 3-24-04  CTD Fix in virtual cockpit.
		OffMode(display);
		return;
	}
	else if(!self->HasPower(AircraftClass::RightHptPower))
	{
		OffMode(display);
		BottomRow();
		StbyMode = TRUE;
		return;
	}
		
	flash = (vuxRealTime & 0x200);
	
	//Check if we're in standby
	if(self->PodCooling > 0.0F)	//always stby
		pFCC->InhibitFire = TRUE;

	if(pFCC->InhibitFire)
		StbyMode = TRUE;

	if(SP)
		pFCC->preDesignate = FALSE;

	laserPod->MenuMode = MenuMode;

	//pod image
	if(laserPod && self->PodCooling <= 0.0F)
	{
		laserPod->SetIntensity(GetIntensity());
		laserPod->Display(display);
	}
	else if(self->PodCooling > 0.0F)
	{
		CoolingDisplay(display);
		OSBLabels(display);
		BottomRow();
		return;
	}

	// FRB - B&W display
	if ((g_bGreyMFD) && (!bNVGmode))
		display->SetColor(GetMfdColor(MFD_WHITE));
	else
		display->SetColor (0xff00ff00);

	// OSB Button Labels	
	OSBLabels(display);
	
	//Bottom Row
	BottomRow();

	//Master Arm
	DrawMasterArm(display);

	//Ralt/Laser/Impact time/weapons indication
	if(!MenuMode)
	{
		DrawRALT(display);
		LaserIndicator(display);
		ImpactTime(display);
		DrawHDPT(display, Sms);
	}

	theRadar->GetCursorPosition (&cX, &cY);
	if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
		OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
		DrawBullseyeCircle(display, cX, cY);
	else
		DrawReference(MfdDisplay[OnMFD()]->GetOwnShip());		

	//Laser Ranging
	DrawRange(display);

	//Booster 18/09/2004 - Draw Pull Up cross on MFD-SMS if ground Collision
   	if (playerAC->mFaults->GetFault(alt_low))
		DrawRedBreak(display);

	// RV - I-Hawk - if in TGP mode (should be only in AG mode) and pitch < -20 degrees or
	// Roll > 75 degrees and pitch < 0 degrees, show attiutde warning
	if ( playerAC && ((fabs(playerAC->Roll()) > 75.0f*DTR && playerAC->Pitch() < 0.0f) ||
							  ( playerAC->Pitch() < -20.0f*DTR)) )
	{
		for ( int i = 0; i < 4; i++ )
		{
			(MfdDisplay[i])->SetTGPWarning(true); // Assign to all
		}
	}

	else
	{
		for ( int i = 0; i < 4; i++ )
		{
			(MfdDisplay[i])->SetTGPWarning(false); // Remove from all
		}
	}
	
	// Check, if 1 of the MFDs is showing TGP and the attitude warning is set, show it... 
	// (works on all MFDs and not only here)
	for ( int i = 0; i < 4; i++ )
	{
		if ( (MfdDisplay[i])->GetTGPWarning() && (MfdDisplay[i])->CurMode() == MFDClass::TGPMode )
			{ 
				TGPAttitudeWarning(display);
				break;
			}
	}
}

void TgpMfdDrawable::PushButton (int whichButton, int whichMFD)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	FireControlComputer* pFCC = playerAC->Sms->Ownship()->GetFCC();
	LaserPodClass* laserPod = (LaserPodClass* )FindLaserPod (playerAC->Sms->Ownship());
    switch (whichButton)
    {
	case 0:
		MenuMode = !MenuMode;
	break;
    case 2:
		if(laserPod)
			laserPod->ToggleFOV();
	break;
	case 3:
		pFCC->LaserFire = FALSE;
		pFCC->InhibitFire = !pFCC->InhibitFire;
		StbyMode = !StbyMode;
	break;
    case 5:
		if(MenuMode)
		{
			StbyMode = FALSE;
			MenuMode = FALSE;
			pFCC->InhibitFire = FALSE;
		}
		else if(laserPod)
			laserPod->BHOT = !laserPod->BHOT;
	break;
	case 7:
		pFCC->preDesignate = !pFCC->preDesignate;
		SP = !SP;
	break;
	case 8:
		pFCC->RecalcPos();
	break;
	case 9:
		StbyMode = !StbyMode;
		//pFCC->InhibitFire = !pFCC->InhibitFire;
		MenuMode = !MenuMode;
	break;
	case 19:
		if ((g_bGreyMFD) || (!g_bGreyScaleMFD))
			g_bGreyMFD = false;
		else
			g_bGreyMFD = true;
	break;
	case 10:
    case 11:	
    case 12:	
    case 13:
	case 14:
		MfdDrawable::PushButton(whichButton, whichMFD);
	break;
	default:
	break;
	}
}
void TgpMfdDrawable::OffMode(VirtualDisplay* display)
{
	float cX, cY = 0;
	display->TextCenterVertical (0.0f, 0.2f, "TGP");  // JPG 14 Dec 03 - Just changed to CAPs
	int ofont = display->CurFont();
	display->SetFont(2);
	display->TextCenterVertical (0.0f, 0.0f, "OFF");  // this too
	display->SetFont(ofont);
	theRadar->GetCursorPosition (&cX, &cY);
	if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&  // JPG 14 Dec 03 - Added BE/ownship info
		OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
		DrawBullseyeCircle(display, cX, cY);
	else
		DrawReference(MfdDisplay[OnMFD()]->GetOwnShip());
	BottomRow();
}
VirtualDisplay* TgpMfdDrawable::GetDisplay(void)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if(!playerAC || !playerAC->Sms)
		return privateDisplay;

	SensorClass* laserPod = FindLaserPod (playerAC->Sms->Ownship());
			
	if(laserPod)
	{
		if (!laserPod->GetDisplay())
		{
			if (privateDisplay)
			{
				laserPod->DisplayInit(((Render2D*)(privateDisplay))->GetImageBuffer());

				if ((g_bGreyMFD) && (!bNVGmode))
					privateDisplay->SetColor (0xff000000);
			}
			laserPod->viewPoint = viewPoint;
		}
		return laserPod->GetDisplay();
	}

	return privateDisplay;
}
void TgpMfdDrawable::DrawRALT(VirtualDisplay* display)
{
	if(TheHud && !(self->mFaults && self->mFaults->GetFault(FaultClass::ralt_fault))
		&& self->af->platform->RaltReady() &&
		TheHud->FindRollAngle(-TheHud->hat) && TheHud->FindPitchAngle(-TheHud->hat))
	{
		float x,y = 0;
		GetButtonPos(5, &x, &y);
		y += display->TextHeight();
		x -= 0.05F;
		int RALT = (int)-TheHud->hat;
		char tempstr[10] = "";
		if(RALT > 9990)
		RALT = 9990;
		sprintf(tempstr, "%d", RALT);
		display->TextRight(x, y, tempstr);
	}
}
void TgpMfdDrawable::DrawHDPT(VirtualDisplay* display, SMSClass* Sms)
{
	float leftEdge;
    float width = display->TextWidth("M ");
	float x = 0, y = 0;
	GetButtonPos(8, &x, &y); // use button 8 as a reference (lower rhs)
	y -= display->TextHeight()*2;
	for(int i=0; i<Sms->NumHardpoints(); i++)
    {
		char c = HdptStationSym(i, Sms);
		if (c == ' ') continue; // Don't bother drawing blanks.
		Str[0] = c;
		Str[1] = '\0';
		if(i < 6)
		{
			leftEdge = -x + width * (i-1);
			display->TextLeft(leftEdge, y, Str, (Sms->CurHardpoint() == i ? 2 : 0));
		}
		else
		{
			leftEdge = x - width * (Sms->NumHardpoints() - i - 1);
			// Box the current station
			display->TextRight(leftEdge, y, Str, (Sms->CurHardpoint() == i ? 2 : 0));
		}
    }
}
char TgpMfdDrawable::HdptStationSym(int n, SMSClass* Sms) // JPO new routine
{
    if (Sms->hardPoint[n] == NULL) return ' '; // empty hp
	if(Sms->hardPoint[n]->weaponCount <= 0) return ' ';	//MI don't bother drawing empty hardpoints
    if (Sms->StationOK(n) == FALSE) return 'F'; // malfunction on  HP
    if (Sms->hardPoint[n]->GetWeaponType() == Sms->curWeaponType)
	return '0' + n; // exact match for weapon
    if (Sms->hardPoint[n]->GetWeaponClass() == Sms->curWeaponClass)
	return 'M'; // weapon of similar class
    return ' ';
}
void TgpMfdDrawable::LaserIndicator(VirtualDisplay* display)
{
	float x,y = 0;
	GetButtonPos(9, &x, &y);
	y += display->TextHeight();

#if 0
	display->TextLeft(.25f, .80f, "LaserArm         ", (pFCC->LaserArm          ? 2 : 0));
	display->TextLeft(.25f, .70f, "CheckForLaserFire", (pFCC->CheckForLaserFire ? 2 : 0));
	display->TextLeft(.25f, .60f, "InhibitFire      ", (pFCC->InhibitFire       ? 2 : 0));
	display->TextLeft(.25f, .50f, "LaserWasFired    ", (pFCC->LaserWasFired     ? 2 : 0));
	display->TextLeft(.25f, .40f, "LaserFire        ", (pFCC->LaserFire         ? 2 : 0));
	display->TextLeft(.25f, .30f, "ManualFire       ", (pFCC->ManualFire        ? 2 : 0));
#endif

	if(laserPod && pFCC->LaserArm)
	{
		//armed and fired
		if(pFCC->LaserFire)
		{
			if(flash)
				display->TextLeft(x -0.75F, y, "L");
		}
		//armed, put it there steady
		else if(pFCC->LaserArm)
			display->TextLeft(x -0.75F, y, "L");
	}

/*	display->TextLeft(.25f, .80f, "LaserArm         ", (pFCC->LaserArm          ? 2 : 0));
	display->TextLeft(.25f, .70f, "CheckForLaserFire", (pFCC->CheckForLaserFire ? 2 : 0));
	display->TextLeft(.25f, .60f, "InhibitFire      ", (pFCC->InhibitFire       ? 2 : 0));
	display->TextLeft(.25f, .50f, "LaserWasFired    ", (pFCC->LaserWasFired     ? 2 : 0));
	display->TextLeft(.25f, .40f, "LaserFire        ", (pFCC->LaserFire         ? 2 : 0));
	display->TextLeft(.25f, .30f, "ManualFire       ", (pFCC->ManualFire        ? 2 : 0)); */
}
void TgpMfdDrawable::ImpactTime(VirtualDisplay* display)
{
	float x,y = 0;
	GetButtonPos(9, &x, &y);
	if(pFCC->ImpactTime >= 10.0F)
		sprintf(Str, "00:%2.0f", pFCC->ImpactTime);
	else if(pFCC->ImpactTime > 0 && pFCC->ImpactTime < 10.0F)
		sprintf(Str, "00:0%1.0f", pFCC->ImpactTime);
	else
		sprintf(Str, "00:00");

	y -= display->TextHeight();
	display->TextLeft(x - 0.55F, y, Str);
}
void TgpMfdDrawable::DrawMasterArm(VirtualDisplay* display)
{
	float x,y = 0;
	char *mode = "";
	if (Sms->CurStationOK())
    {
		switch (Sms->MasterArm())
		{
		case SMSBaseClass::Safe:
			//MI not here in real
			if(!g_bRealisticAvionics)
				mode = "SAF";
			else
				mode = "";
			break;
			
		case SMSBaseClass::Sim:
			mode = "SIM";
			break;
			
		case SMSBaseClass::Arm:
			mode = "RDY";
			break;
		}
	}
    else
		mode = "MAL";
    
	GetButtonPos (12, &x, &y);
	// JPO - do this ourselves, so we can pass the rest off to the superclass.
	display->TextCenter(x, y + display->TextHeight(), mode);
}
void TgpMfdDrawable::CoolingDisplay(VirtualDisplay* display)
{
	int ofont = display->CurFont();
	display->SetFont(2);
	display->TextCenterVertical (0.0f, 0.2f, "NOT TIMED OUT");
	display->SetFont(ofont);
}
void TgpMfdDrawable::OSBLabels(VirtualDisplay* display)
{
	if(MenuMode)
	{
		if(StbyMode)
			LabelButton(0, "STBY");
		else
			LabelButton(0, "A-G");
		if(laserPod->CurFOV() < 1.7F * DTR)
			sprintf(Str, "EXP");
		else
			sprintf(Str, "%s", (laserPod->CurFOV() < (3.5F * DTR)) ? "NARO" : "WIDE");
		LabelButton(2, Str);
		if(pFCC->InhibitFire)
			LabelButton(3, "OVRD", NULL, 2);
		else
			LabelButton(3, "OVRD");

		if(pFCC->IsAGMasterMode())
		{
			if(StbyMode)
				LabelButton(5, "A-G");
			else
				LabelButton(5, "A-G", NULL, 2);
			if(StbyMode)
				LabelButton(9, "STBY", NULL, 2);
			else
				LabelButton(9, "STBY");
		}
		else if(pFCC->IsAAMasterMode())
		{
			LabelButton(19, "A-A");
			if(StbyMode)
				LabelButton(9, "STBY", NULL, 2);
			else
				LabelButton(9, "STBY");
		}
		else	//NAV
		{
			if(StbyMode)
				LabelButton(5, "A-G");
			else
				LabelButton(5, "A-G", NULL, 2);
			if(StbyMode)
				LabelButton(9, "STBY", NULL, 2);
			else
				LabelButton(9, "STBY");

			LabelButton(19, "A-A");
		}
	}
	else if(StbyMode)
	{
		LabelButton(0, "STBY");
		if(laserPod->CurFOV() < 1.7F * DTR)
			sprintf(Str, "EXP");
		else
			sprintf(Str, "%s", (laserPod->CurFOV() < (3.5F * DTR)) ? "NARO" : "WIDE");
		LabelButton(2, Str);
		if(pFCC->InhibitFire)
			LabelButton(3, "OVRD", NULL, 2);
		else
			LabelButton(3, "OVRD");
		LabelButton(4, "CNTL");
		sprintf(Str,"%s",  laserPod->BHOT ? "BHOT" : "WHOT");
		LabelButton(5, Str);
		if(SP)
			LabelButton(7, "S", "P", 2);
		else
			LabelButton(7, "S", "P");
		LabelButton(8, "C", "Z");
		LabelButton(9, "T", "G");
		if (g_bGreyMFD)
			LabelButton(19, "GRAY", "OFF");
		else
			LabelButton(19, "GRAY", "ON");
	}
	else
	{
		LabelButton(0, "A-G");	//would be either A-A or A-G.. but AA isn't modelled
		if(laserPod->CurFOV() < 1.7F * DTR)
			sprintf(Str, "EXP");
		else
			sprintf(Str, "%s", (laserPod->CurFOV() < (3.5F * DTR)) ? "NARO" : "WIDE");
		LabelButton(2, Str);
		if(pFCC->InhibitFire)
			LabelButton(3, "OVRD", NULL, 2);
		else
			LabelButton(3, "OVRD");
		LabelButton(4, "CNTL");
		if(SP)
			LabelButton(7, "S", "P", 2);
		else
			LabelButton(7, "S", "P");
		LabelButton(8, "C", "Z");
		LabelButton(9, "T", "G");
		if (g_bGreyMFD)
			LabelButton(19, "GRAY", "OFF");
		else
			LabelButton(19, "GRAY", "ON");

		//BHOT/WHOT
		sprintf(Str, "%s", laserPod->BHOT ? "BHOT" : "WHOT");
		LabelButton(5, Str);
	}
}
void TgpMfdDrawable::DrawRange(VirtualDisplay* display)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	FireControlComputer* pFCC = playerAC->Sms->Ownship()->GetFCC();
	float x,y = 0;

	if(!pFCC)
		return;

	//don't draw range in slave mode
	if(pFCC->preDesignate)
		return;

	float range = pFCC->LaserRange * FT_TO_NM;
	
	if(range <= 0.0F)
	{
		if(pFCC->LaserFire)
			sprintf(Str, "L 00.0");
		else
			sprintf(Str, "T 00.0");
	}
	else if(range < 10.0F) 
	{
		if(pFCC->LaserFire)
			sprintf(Str, "L 0%1.1f", range);
		else
			sprintf(Str, "T 0%1.1f", range);
	}
	else if(range >= 10.0F) 
	{
		if(pFCC->LaserFire)
			sprintf(Str, "L %2.1f", range);
		else
			sprintf(Str, "T %2.1f", range);
	}

	GetButtonPos(15, &x, &y);
	y -= display->TextHeight();
	display->TextCenter(x + 0.60F, y, Str);
}
