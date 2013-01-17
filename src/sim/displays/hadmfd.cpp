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


//MI
void DrawBullseyeCircle(VirtualDisplay* display, float cursorX, float cursorY);

HadMfdDrawable::HadMfdDrawable()
{
	self = NULL;
	pFCC = NULL;
	Sms = NULL;
	theRadar = NULL;
}

void HadMfdDrawable::Display (VirtualDisplay* newDisplay)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

	display = newDisplay;
	self = ((AircraftClass*)playerAC);
	theRadar = (RadarDopplerClass*)FindSensor(playerAC, SensorClass::Radar);
	pFCC = playerAC->Sms->Ownship()->GetFCC();
	Sms = playerAC->Sms;
	HarmTargetingPod* harmPod = (HarmTargetingPod*)FindSensor(Sms->Ownship(), SensorClass::HTS);
	if( !theRadar || !pFCC || !self || !Sms)
	{
		return;
	}

	if( !harmPod || !self->af->GetIsHtsAble() ) // offMode if no HTS pod or equivalent system on board
	{
		OffMode(display);
		return;
	}

	// This makes sure we are in the correct FCC and Harmpod submodes
	if ( pFCC->GetSubMode() != FireControlComputer::HTS || (harmPod->GetSubMode() != HarmTargetingPod::FilterMode &&
		 harmPod->GetSubMode() != HarmTargetingPod::HAD) )
	{
		if ( playerAC->GetSOI() == SimVehicleClass::SOI_WEAPON )
		{
            pFCC->SetSubMode ( FireControlComputer::HTS );
		}

		harmPod->SetSubMode( HarmTargetingPod::HAD );
		if ( playerAC == Sms->Ownship() )
		{
            playerAC->SOIManager (SimVehicleClass::SOI_RADAR);
		}
	}

    HARMWpnMode();

	//Booster 18/09/2004 - Draw Pull Up cross on MFD-SMS if ground Collision
   	if ( playerAC->mFaults->GetFault(alt_low) )
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

void HadMfdDrawable::DrawDLZ(VirtualDisplay* display)
{
	if(!pFCC) { return; }

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

void HadMfdDrawable::PushButton(int whichButton, int whichMFD)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	FireControlComputer* pFCC = playerAC->Sms->Ownship()->GetFCC();
	HarmTargetingPod* harmPod = (HarmTargetingPod*)FindSensor(Sms->Ownship(), SensorClass::HTS);

	if ( harmPod && harmPod->GetSubMode() == HarmTargetingPod::FilterMode )
	{
		switch ( whichButton )
		{
		case 0:
			harmPod->SetSubMode( HarmTargetingPod::HAD ); // Return right back to HAD
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
			harmPod->SetSubMode( HarmTargetingPod::HAD );
			playerAC->SOIManager ( SimVehicleClass::SOI_WEAPON );
			break;
            
		case 18:
			harmPod->SetFilterMode ( HarmTargetingPod::HP );
			harmPod->SetSubMode( HarmTargetingPod::HAD );
			playerAC->SOIManager ( SimVehicleClass::SOI_WEAPON );
			break;

		case 17:
			harmPod->SetFilterMode ( HarmTargetingPod::HA );
			harmPod->SetSubMode( HarmTargetingPod::HAD );
			playerAC->SOIManager ( SimVehicleClass::SOI_WEAPON );
			break;

		case 16:
			harmPod->SetFilterMode ( HarmTargetingPod::LA );
			harmPod->SetSubMode( HarmTargetingPod::HAD );
			playerAC->SOIManager ( SimVehicleClass::SOI_WEAPON );
			break;			
		}
	}

	else if ( harmPod && harmPod->GetSubMode() == HarmTargetingPod::HAD ) // HAD screen
	{
		switch ( whichButton )
		{
		case 2:
			harmPod->ToggleHADZoomMode();
			harmPod->ResetCursor();
			break;

		case 3:
			// Go to the threats filtering screen
			harmPod->SetSubMode( HarmTargetingPod::FilterMode ); 
			//harmPod->SetLastSubMode( HarmTargetingPod::HAD ); 
			break;

		case 10:
		case 11:	
		case 12:	
		case 13:
		case 14:
			MfdDrawable::PushButton(whichButton, whichMFD);
			break;	

		case 18:
			if ( harmPod->GetHADZoomMode() == HarmTargetingPod::NORM )
			{
				harmPod->DecreaseRange();
			}
			break;

		case 19:
			if ( harmPod->GetHADZoomMode() == HarmTargetingPod::NORM )
			{
                harmPod->IncreaseRange();
			}
			break;
		}
	}

	else // We are in off mode
	{
		switch ( whichButton )
		{
		case 10:
		case 11:	
		case 12:	
		case 13:
		case 14:
			MfdDrawable::PushButton(whichButton, whichMFD);
			break;
		}
	}
}

void HadMfdDrawable::DrawRALT(VirtualDisplay* display)
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

void HadMfdDrawable::DrawHDPT(VirtualDisplay* display, SMSClass* Sms)
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
char HadMfdDrawable::HdptStationSym(int n, SMSClass* Sms) // JPO new routine
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

void HadMfdDrawable::HARMWpnMode()
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	
	pFCC = playerAC->Sms->Ownship()->GetFCC();
	Sms = playerAC->Sms;
	HarmTargetingPod* harmPod = (HarmTargetingPod*)FindSensor(Sms->Ownship(), SensorClass::HTS);

	if ( harmPod->GetSubMode() == HarmTargetingPod::FilterMode )
	{
		LabelButton (0, "HAD");

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
	}

	else if ( harmPod->GetSubMode() == HarmTargetingPod::HAD )
	{
		flash = vuxRealTime & 0x80;

		harmPod->SetIntensity(GetIntensity());

		switch ( harmPod->GetHADZoomMode() )
		{
		case HarmTargetingPod::NORM:
		default:
            harmPod->HADDisplay( display );
			break;

		case HarmTargetingPod::EXP1:
		case HarmTargetingPod::EXP2:
			harmPod->HADExpDisplay( display );
			break;
		}

		LabelButton (0,  "TDDA");

		switch ( harmPod->GetHADZoomMode() )
		{
		case HarmTargetingPod::NORM:
		default:
			LabelButton (2, "NORM");
			break;

		case HarmTargetingPod::EXP1:
			float x,y;
			int boxed;
			boxed = flash ? 2 : 0;
            GetButtonPos(2, &x, &y);
            display->TextCenter ( x, y, "EXP1", boxed );
			break;

		case HarmTargetingPod::EXP2:
			boxed = flash ? 2 : 0;
			GetButtonPos(2, &x, &y);

			if ( flash )
			{
                display->TextCenter ( x, y, "EXP2", boxed );
			}
			break;
		}

		LabelButton (3,  "THRT");
		LabelButton (4,  "CNTL");
		LabelButton (7,  "FR", "ON");
		LabelButton (8,  "GS", "OF");
		LabelButton (9,  "T", "I");
		LabelButton (15,  "MEM");
		LabelButton (16,  "TD", "TM");
		BottomRow();

		if ( !(playerAC->GetSOI() == SimVehicleClass::SOI_WEAPON) )
		{
			DWORD tempColor = display->Color();
			display->SetColor(GetMfdColor(MFD_WHITY_GRAY));
			display->TextCenter(0.0F, 0.6F, "NOT SOI");
			display->SetColor(tempColor);
		}
	}
}

void HadMfdDrawable::OffMode(VirtualDisplay* display)
{
	float cX, cY = 0;
	display->TextCenterVertical (0.0f, 0.2f, "HAD");
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