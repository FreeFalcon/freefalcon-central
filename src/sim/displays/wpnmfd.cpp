#include "stdhdr.h"
#include "mfd.h"
#include "hud.h"
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
#include "mavdisp.h"
#include "misldisp.h"
#include "airframe.h"
#include "simweapn.h"
#include "missile.h"
#include "harmpod.h"
#include "commands.h"

extern bool g_bGreyMFD;
extern bool g_bGreyScaleMFD;
extern bool bNVGmode;

//MI
void DrawBullseyeCircle(VirtualDisplay* display, float cursorX, float cursorY);

WpnMfdDrawable::WpnMfdDrawable()
{
	self = NULL;
	pFCC = NULL;
	mavDisplay = NULL;
	Sms = NULL;
	theRadar = NULL;
}
void WpnMfdDrawable::DisplayInit (ImageBuffer* image)
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

VirtualDisplay* WpnMfdDrawable::GetDisplay(void)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if(!playerAC || !playerAC->Sms || 
		!playerAC->Sms->curWeapon)
		return privateDisplay;

	Sms = playerAC->Sms;

	VirtualDisplay* retval= privateDisplay;
	MissileClass* theMissile = (MissileClass*)(Sms->GetCurrentWeapon());
	float rx, ry, rz;
	Tpoint pos;
	
	if(Sms->CurHardpoint() < 0)
	{
		return retval;
	}
	
	if(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponClass() == wcAgmWpn &&
		Sms->curWeaponType == wtAgm65)
	{
		if (theMissile && theMissile->IsMissile())
		{
			mavDisplay = (MaverickDisplayClass*)theMissile->display;
			if (mavDisplay)
			{
				if (!mavDisplay->GetDisplay())
				{
					if (privateDisplay)
					{
						mavDisplay->DisplayInit(((Render2D*)(privateDisplay))->GetImageBuffer());
					}
					mavDisplay->viewPoint = viewPoint;
					
					// Set missile initial position
					Sms->hardPoint[Sms->CurHardpoint()]->GetSubPosition(Sms->curWpnNum, &rx, &ry, &rz);
					rx += 5.0F;
					pos.x = Sms->Ownship()->XPos() + Sms->Ownship()->dmx[0][0]*rx + Sms->Ownship()->dmx[1][0]*ry +
						Sms->Ownship()->dmx[2][0]*rz;
					pos.y = Sms->Ownship()->YPos() + Sms->Ownship()->dmx[0][1]*rx + Sms->Ownship()->dmx[1][1]*ry +
						Sms->Ownship()->dmx[2][1]*rz;
					pos.z = Sms->Ownship()->ZPos() + Sms->Ownship()->dmx[0][2]*rx + Sms->Ownship()->dmx[1][2]*ry +
						Sms->Ownship()->dmx[2][2]*rz;
					mavDisplay->SetXYZ (pos.x, pos.y, pos.z);
				}
				retval = mavDisplay->GetDisplay();
			}
		}
	}
	return (retval);
}
void WpnMfdDrawable::Display (VirtualDisplay* newDisplay)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	float cX, cY;

	display = newDisplay;
	self = ((AircraftClass*)playerAC);
	theRadar = (RadarDopplerClass*)FindSensor(playerAC, SensorClass::Radar);
	pFCC = playerAC->Sms->Ownship()->GetFCC();
	Sms = playerAC->Sms;
	mavDisplay = NULL;
	display = newDisplay;
	HarmTargetingPod* harmPod = (HarmTargetingPod*)FindSensor(Sms->Ownship(), SensorClass::HTS);
	if(!theRadar || !pFCC || !self || !Sms)
	{
		ShiWarning("Oh Oh shouldn't be here without a radar or FCC or player or SMS!");
		return;
	}

	if( !g_bRealisticAvionics || !Sms->curWeapon || (Sms->curWeaponType != wtAgm65 && Sms->curWeaponType != wtAgm88) )
	{
		OffMode(display);
		return;
	}

	if ( harmPod && Sms->curWeaponType == wtAgm88 )
	{
		// This makes sure we are in the correct FCC submode
		if ( pFCC->GetSubMode() != FireControlComputer::HARM )
		{
			pFCC->SetSubMode( FireControlComputer::HARM );
			harmPod->SetSubMode( HarmTargetingPod::HarmModeChooser );
		}

		HARMWpnMode();
	}

	if(Sms->curWeapon && Sms->curWeaponType == wtAgm65) // RV-I-Hawk - No do for HARM WPN
	{
	    ShiAssert(Sms->curWeapon->IsMissile());
		mavDisplay = (MaverickDisplayClass*)((MissileClass*)Sms->GetCurrentWeapon())->display;

		// FRB - B&W display
		if ((g_bGreyMFD) && (!bNVGmode))
			display->SetColor(GetMfdColor(MFD_WHITE));
		else
			display->SetColor (0xff00ff00);

		if(mavDisplay && (!((MissileClass*)Sms->GetCurrentWeapon())->Covered || playerAC->AutopilotType() == AircraftClass::CombatAP) && Sms->MavCoolTimer <= 0.0F)
		{
			mavDisplay->SetIntensity(GetIntensity());
			mavDisplay->viewPoint = viewPoint;
			mavDisplay->Display(display);
			//DLZ
			DrawDLZ(display);
		}
		else if(Sms->MavCoolTimer > 0.0F && Sms->Powered)
		{
			display->TextCenter(0.0F, 0.7F, "NOT TIMED OUT");
		}
		DrawRALT(display);
	}
	
	// RV-I-Hawk - No do for HARM WPN in HTSSubmode
	if ( Sms->curWeaponType == wtAgm65 ||( Sms->curWeaponType == wtAgm88 && harmPod->GetSubMode() != HarmTargetingPod::HarmModeChooser) )
	{
		// FRB - B&W display
		if ((g_bGreyMFD) && (!bNVGmode))
			display->SetColor(GetMfdColor(MFD_WHITE));
		else
			display->SetColor (0xff00ff00);

    //SMSMissiles
		DrawHDPT(display, Sms);
	}

	// RV-I-Hawk - No do for HARM WPN
	if ( Sms->curWeaponType == wtAgm65 )
	{
		// FRB - B&W display
		if ((g_bGreyMFD) && (!bNVGmode))
			display->SetColor(GetMfdColor(MFD_WHITE));
		else
			display->SetColor (0xff00ff00);

    //OSB's
		OSBLabels(display);
	}

	//Reference symbol
	theRadar->GetCursorPosition (&cX, &cY);
	if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
	   OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
	{
		// FRB - B&W display
		if ((g_bGreyMFD) && (!bNVGmode))
			display->SetColor(GetMfdColor(MFD_WHITE));
		else
			display->SetColor (0xff00ff00);

		DrawBullseyeCircle(display, cX, cY);
	}

	else
	{
		DrawReference(MfdDisplay[OnMFD()]->GetOwnShip());
	}

	//Booster 18/09/2004 - Draw Pull Up cross on MFD-SMS if ground Collision
   	if (playerAC->mFaults->GetFault(alt_low))
	{
		DrawRedBreak(display);
	}

	// RV - I-Hawk
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

void WpnMfdDrawable::DrawDLZ(VirtualDisplay* display)
{
	if(!pFCC){ return; }

	float yOffset;
	float percentRange;
	float rMax, rMin;
	float range;
	float dx, dy, dz;

	if (pFCC->missileTarget)
	{
		// Range Carat / Closure
		rMax   = pFCC->missileRMax;
		rMin   = pFCC->missileRMin;
		
		// get range to ground designaate point
		dx = Sms->Ownship()->XPos() - pFCC->groundDesignateX;
		dy = Sms->Ownship()->YPos() - pFCC->groundDesignateY;
		dz = Sms->Ownship()->ZPos() - pFCC->groundDesignateZ;
		range = (float)sqrt( dx*dx + dy*dy + dz*dz );
		
		// Normailze the ranges for DLZ display
		percentRange = range / pFCC->missileWEZDisplayRange;
		rMin /= pFCC->missileWEZDisplayRange;
		rMax /= pFCC->missileWEZDisplayRange;
		
		// Clamp in place
		rMin = min (rMin, 1.0F);
		rMax = min (rMax, 1.0F);
		
		// Draw the symbol
		
		// Rmin/Rmax
		display->Line (0.9F, -0.8F + rMin * 1.6F, 0.95F, -0.8F + rMin * 1.6F);
		display->Line (0.9F, -0.8F + rMin * 1.6F, 0.9F,  -0.8F + rMax * 1.6F);
		display->Line (0.9F, -0.8F + rMax * 1.6F, 0.95F, -0.8F + rMax * 1.6F);
		
		// Range Caret
		yOffset = min (-0.8F + percentRange * 1.6F, 1.0F);
		
		display->Line (0.9F, yOffset, 0.9F - 0.03F, yOffset + 0.03F);
		display->Line (0.9F, yOffset, 0.9F - 0.03F, yOffset - 0.03F);
	}
}

void WpnMfdDrawable::PushButton(int whichButton, int whichMFD)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	FireControlComputer* pFCC = playerAC->Sms->Ownship()->GetFCC();
	HarmTargetingPod* harmPod = (HarmTargetingPod*)FindSensor(Sms->Ownship(), SensorClass::HTS);

	// RV - I-Hawk - First check if FCC isn't in HARM mode
	if ( pFCC->GetMasterMode() != FireControlComputer::AirGroundHARM )
	{
		switch (whichButton)
		{
		case 0:
			if(!Sms->Powered)
			{
				Sms->Powered = TRUE;
			}
			break;

		case 1:
			Sms->StepMavSubMode();
			break;

		case 2:
			if(mavDisplay)
			{
				mavDisplay->ToggleFOV();
			}
			break;

		case 4:
			((MissileClass*)Sms->GetCurrentWeapon())->HOC = !((MissileClass*)Sms->GetCurrentWeapon())->HOC;
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

	else	// Now handle the HARM mode 
	{
		switch ( harmPod->GetSubMode() )
		{
		case HarmTargetingPod::HarmModeChooser:
		default:
			switch ( whichButton )
			{
			case 10:
			case 11:	
			case 12:	
			case 13:
			case 14:
				MfdDrawable::PushButton(whichButton, whichMFD);
				break;

			case 19:
				harmPod->SetSubMode( HarmTargetingPod::POS );
				harmPod->SetRange( 60 );
				break;
                
			case 18:
				harmPod->SetSubMode( HarmTargetingPod::HAS );
				harmPod->SetRange( 60 );
				harmPod->ResetHASTimer();
				harmPod->SetFilterMode( HarmTargetingPod::ALL );
				break;

			}
			break;

		case HarmTargetingPod::HAS:
			switch ( whichButton )
			{
			case 0:
				harmPod->SetSubMode( HarmTargetingPod::HarmModeChooser );
				pFCC->dropTrackCmd = TRUE; // drop target if switching HTS mode
				break;

			case 2:
				harmPod->ToggleZoomMode();
				break;
			
			case 3:
				// Go to the threats filtering screen
				harmPod->SetSubMode( HarmTargetingPod::FilterMode ); 
				harmPod->SetLastSubMode( HarmTargetingPod::HAS ); 
				break;

			case 6:
				// Reset HAS timer
				harmPod->ResetHASTimer();
				break;

			case 10:
			case 11:	
			case 12:	
			case 13:
			case 14:
				MfdDrawable::PushButton(whichButton, whichMFD);
			}

			break;

		case HarmTargetingPod::POS:
			switch ( whichButton )
			{
			case 0:
				harmPod->SetSubMode( HarmTargetingPod::HarmModeChooser );
				pFCC->dropTrackCmd = TRUE; // drop target if switching HTS mode
				break;

			case 10:
			case 11:	
			case 12:	
			case 13:
			case 14:
				MfdDrawable::PushButton(whichButton, whichMFD);
				break;

			case 16:
				harmPod->SetPOSTargetIndex ( 0 );
				harmPod->LockPOSTarget();
				break;

			case 17:
				harmPod->SetPOSTargetIndex ( 1 );
				harmPod->LockPOSTarget();
				break;

			case 18:
				harmPod->SetPOSTargetIndex ( 2 );
				harmPod->LockPOSTarget();
				break;

			case 19:
				harmPod->SetPOSTargetIndex ( 3 );
				harmPod->LockPOSTarget();
				break;
			}

			break;

		case HarmTargetingPod::Handoff:
			switch ( whichButton )
			{
			case 0:
				harmPod->SetSubMode( HarmTargetingPod::HarmModeChooser );
				pFCC->dropTrackCmd = TRUE; // drop target if switching HARM mode
				break;

			case 10:
			case 11:	
			case 12:	
			case 13:
			case 14:
				MfdDrawable::PushButton(whichButton, whichMFD);
				break;
			}

			break;

		case HarmTargetingPod::FilterMode:
			switch ( whichButton )
			{
			case 0:
				harmPod->SetSubMode( HarmTargetingPod::HAS ); // Return right back to HAS
				playerAC->SOIManager ( SimVehicleClass::SOI_WEAPON );
				break;

			case 10:
			case 11:	
			case 12:	
			case 13:
			case 14:
				MfdDrawable::PushButton(whichButton, whichMFD);
				break;

				// Eache of this buttons sets a different filtering mode and getting back to HAS
			case 19:
				harmPod->SetFilterMode ( HarmTargetingPod::ALL );
				harmPod->ResetHASTimer();
				harmPod->SetSubMode( HarmTargetingPod::HAS );
				playerAC->SOIManager ( SimVehicleClass::SOI_WEAPON );
				break;
                
			case 18:
				harmPod->SetFilterMode ( HarmTargetingPod::HP );
				harmPod->ResetHASTimer();
				harmPod->SetSubMode( HarmTargetingPod::HAS );
				playerAC->SOIManager ( SimVehicleClass::SOI_WEAPON );
				break;

			case 17:
				harmPod->SetFilterMode ( HarmTargetingPod::HA );
				harmPod->ResetHASTimer();
				harmPod->SetSubMode( HarmTargetingPod::HAS );
				playerAC->SOIManager ( SimVehicleClass::SOI_WEAPON );
				break;

			case 16:
				harmPod->SetFilterMode ( HarmTargetingPod::LA );
				harmPod->ResetHASTimer();
				harmPod->SetSubMode( HarmTargetingPod::HAS );
				playerAC->SOIManager ( SimVehicleClass::SOI_WEAPON );
				break;			
			}

			break;
		}
	}
}

void WpnMfdDrawable::OSBLabels(VirtualDisplay* display)
{
	char tempstr[10] = "";
	if(!Sms->Powered)
		LabelButton (0, "STBY");

	else
		LabelButton (0, "OPER");

	if(Sms->MavSubMode == SMSBaseClass::PRE)
		sprintf(tempstr,"PRE");

	else if(Sms->MavSubMode == SMSBaseClass::VIS)
		sprintf(tempstr, "VIS");

	else
		sprintf(tempstr,"BORE");

	LabelButton(1, tempstr);
	
	if (g_bGreyMFD)
		LabelButton(19, "GRAY", "OFF");
	else
		LabelButton(19, "GRAY", "ON");
	// RV - Biker - Make FOV switching this dynamic
	//if (mavDisplay && mavDisplay->CurFOV() > (3.5F * DTR))
	float ZoomMin;
	float ZoomMax;

	if((MissileClass*)Sms->GetCurrentWeapon() && ((MissileClass*)Sms->GetCurrentWeapon())->GetEXPLevel() > 0 && ((MissileClass*)Sms->GetCurrentWeapon())->GetFOVLevel() > 0) {
		ZoomMin = ((MissileClass*)Sms->GetCurrentWeapon())->GetFOVLevel();
		ZoomMax = ((MissileClass*)Sms->GetCurrentWeapon())->GetEXPLevel();
	}
	else {
		// This should work for old values
		ZoomMin = 3.0f;
		ZoomMax = 6.0f;
	}

	if (mavDisplay && mavDisplay->CurFOV() > 12.0f/(ZoomMax-(ZoomMax-ZoomMin)/2.0f) * DTR)
		LabelButton (2, "FOV");
	else
		LabelButton (2, "EXP", NULL, 1);
	if(Sms->curWeapon)
	{
		if(((MissileClass*)Sms->GetCurrentWeapon())->HOC)
			LabelButton (4, "HOC");
		else
			LabelButton (4, "COH");
	}
	//Doc says this isn't there anymore in the real deal
	//LabelButton (19, pFCC->subModeString);
	char tmpStr[12];
    float width = display->TextWidth("M ");
    if (Sms->CurHardpoint() < 0)
		return;
    sprintf (tmpStr, "%d%s", Sms->NumCurrentWpn(), Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponData()->mnemonic);
    ShiAssert(strlen(tmpStr) < sizeof (tmpStr));
    LabelButton(5, tmpStr);

	char *mode = "";
	if(Sms->Powered && Sms->MavCoolTimer <= 0.0F)
	{
		switch (Sms->MasterArm())
		{
		case SMSBaseClass::Safe:
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

    float x, y;
	GetButtonPos (12, &x, &y);
	display->TextCenter(x, y + display->TextHeight(), mode);
	BottomRow();
}
void WpnMfdDrawable::DrawRALT(VirtualDisplay* display)
{
	if( TheHud && !(self->mFaults && self->mFaults->GetFault(FaultClass::ralt_fault))
	    && self->af->platform->RaltReady() &&
	    TheHud->FindRollAngle(-TheHud->hat) && TheHud->FindPitchAngle(-TheHud->hat) )
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
void WpnMfdDrawable::DrawHDPT(VirtualDisplay* display, SMSClass* Sms)
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
char WpnMfdDrawable::HdptStationSym(int n, SMSClass* Sms) // JPO new routine
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

// RV - I-Hawk - Manage HARM modes display
void WpnMfdDrawable::HARMWpnMode()
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	
	pFCC = playerAC->Sms->Ownship()->GetFCC();
	Sms = playerAC->Sms;
	HarmTargetingPod* harmPod = (HarmTargetingPod*)FindSensor(Sms->Ownship(), SensorClass::HTS);

	if ( pFCC->GetMasterMode() == FireControlComputer::AirGroundHARM ) // Varify we are in HARM FCC Master mode
	{
		switch ( harmPod->GetSubMode() ) // Check which Submode 
		{
		case HarmTargetingPod::HarmModeChooser: 
		default:
			LabelButton (17, "DL");
			LabelButton (18, "HAS");
			LabelButton (19, "POS");
			BottomRow();

			if ( !(playerAC->GetSOI() == SimVehicleClass::SOI_WEAPON) )
			{
				DWORD tempColor = display->Color();
				display->SetColor(GetMfdColor(MFD_WHITY_GRAY));
				display->TextCenter(0.0F, 0.6F, "NOT SOI");
				display->SetColor(tempColor);
			}
			break;
		
		case HarmTargetingPod::POS:
			LabelButton (0, "POS");
			LabelButton (1, "TBL1");
			LabelButton (2, "PB");
			LabelButton (4, "UFC");
			LabelButton (8, "GS", "OF");
			LabelButton (9, "T", "I");
			harmPod->SetIntensity(GetIntensity());
			harmPod->POSDisplay(display);
			BottomRow();

			if ( !(playerAC->GetSOI() == SimVehicleClass::SOI_WEAPON) )
			{
				DWORD tempColor = display->Color();
				display->SetColor(GetMfdColor(MFD_WHITY_GRAY));
				display->TextCenter(0.0F, 0.6F, "NOT SOI");
				display->SetColor(tempColor);
			}
			break;

		case HarmTargetingPod::HAS: 
		
			LabelButton (0, "HAS");
			LabelButton (1, "TBL1");

			switch ( harmPod->GetZoomMode() )
			{
			case HarmTargetingPod::Center: 
			    LabelButton (2, "CTR");
				break;

			case HarmTargetingPod::Right: 
			    LabelButton (2, "RT");
				break;

			case HarmTargetingPod::Left: 
			    LabelButton (2, "LT");
				break;

			case HarmTargetingPod::Wide:
			default:
			    LabelButton (2, "WIDE");
				break;
			}

			LabelButton (3, "SRCH");
			LabelButton (4, "UFC");
			LabelButton (6, "R", "S");
			LabelButton (9, "T", "I");
			harmPod->SetIntensity(GetIntensity());
			harmPod->HASDisplay(display);
			BottomRow();

			if ( !(playerAC->GetSOI() == SimVehicleClass::SOI_WEAPON) )
			{
				DWORD tempColor = display->Color();
				display->SetColor(GetMfdColor(MFD_WHITY_GRAY));
				display->TextCenter(0.0F, 0.52F, "NOT SOI");
				display->SetColor(tempColor);
			}
			break;

			case HarmTargetingPod::Handoff: 
		
			LabelButton (0, "HAS");
			LabelButton (1, "TBL1");

			switch ( harmPod->GetZoomMode() )
			{
			case HarmTargetingPod::Center: 
			    LabelButton (2, "CTR");
				break;

			case HarmTargetingPod::Right: 
			    LabelButton (2, "RT");
				break;

			case HarmTargetingPod::Left: 
			    LabelButton (2, "LT");
				break;

			case HarmTargetingPod::Wide: 
				if ( harmPod->GetPreHandoffMode() == HarmTargetingPod::HAS )
				{
                    LabelButton (2, "WIDE");
				}
				break;
			}

			LabelButton (4, "UFC");
			LabelButton (9, "T", "I");
			harmPod->SetIntensity(GetIntensity());
			harmPod->HandoffDisplay(display);
			BottomRow();

			if ( !(playerAC->GetSOI() == SimVehicleClass::SOI_WEAPON) )
			{
				DWORD tempColor = display->Color();
				display->SetColor(GetMfdColor(MFD_WHITY_GRAY));
				display->TextCenter(0.0F, 0.6F, "NOT SOI");
				display->SetColor(tempColor);
			}
			break;

		case HarmTargetingPod::FilterMode: 
			LabelButton (0, "HAS");

			float x,y;
			int boxed = harmPod->GetFilterMode();

			GetButtonPos(19, &x, &y);
			display->TextLeft ( x, y, "ALL", boxed == HarmTargetingPod::ALL ? 2 : 0 );

			GetButtonPos(18, &x, &y);
			display->TextLeft ( x, y, "HP", boxed == HarmTargetingPod::HP ? 2 : 0 );

			GetButtonPos(17, &x, &y);
			display->TextLeft ( x, y, "HA", boxed == HarmTargetingPod::HA ? 2 : 0 );

			GetButtonPos(16, &x, &y);
			display->TextLeft ( x, y, "LA", boxed == HarmTargetingPod::LA ? 2 : 0 );

			BottomRow();

			if ( !(playerAC->GetSOI() == SimVehicleClass::SOI_WEAPON) )
			{
				DWORD tempColor = display->Color();
				display->SetColor(GetMfdColor(MFD_WHITY_GRAY));
				display->TextCenter(0.0F, 0.6F, "NOT SOI");
				display->SetColor(tempColor);
			}
			break;
		}
	}
}

void WpnMfdDrawable::OffMode(VirtualDisplay* display)
{
	float cX, cY = 0;
	display->TextCenterVertical (0.0f, 0.2f, "WPN");
	int ofont = display->CurFont();
	display->SetFont(2);
	display->TextCenterVertical (0.0f, 0.0f, "OFF");
	display->SetFont(ofont);
	theRadar->GetCursorPosition (&cX, &cY);
	if( OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&  // JPG 14 Dec 03 - Added BE/ownship info
		OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo )
	{
		DrawBullseyeCircle(display, cX, cY);
	}

	else
	{
		DrawReference(MfdDisplay[OnMFD()]->GetOwnShip());
	}

	BottomRow();
}