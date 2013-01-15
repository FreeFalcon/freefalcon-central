#include <math.h> // MD -- 20031107: for mods to AltHold()

#include "stdhdr.h"
#include "digi.h"
#include "simveh.h"
#include "airframe.h"
#include "Aircrft.h"
#include "otwdrive.h"
#include "Graphics\Include\tmap.h"
#include "lantirn.h"
#include "cpmanager.h"
#include "cphsi.h"
#include "pilotinputs.h"
#include "flightdata.h"
#include "fcc.h"

extern bool g_bRealisticAvionics;	//MI
extern bool g_bINS;	//MI
extern bool g_bTFRFixes;
extern bool g_bCalibrateTFR_PitchCtrl;

void DigitalBrain::ThreeAxisAP (void)
{
   float headingErr;

	self->af->SetSimpleMode(FALSE);

   // do nothing if on ground
   if ( self->OnGround() )
   		return;

   headingErr = holdPsi - af->sigma;
   AltitudeHold(holdAlt);
   SetYpedal( headingErr * 0.05F * RTD * self->GetVt()/cornerSpeed);
   SetMaxRoll ((float)fabs(self->Roll()*RTD));

   if (self->Roll() > 5.0F * DTR)
   {
      SetMaxRollDelta (-self->Roll()*RTD);
   }
   else if (self->Roll() >= 0.0F * DTR)
   {
      SetMaxRollDelta (-self->Roll()*RTD - 1.0F * DTR);
   }
   else if (self->Roll() > -5.0F * DTR)
   {
      SetMaxRollDelta (-self->Roll()*RTD + 1.0F * DTR);
   }
   else
   {
      SetMaxRollDelta (-self->Roll()*RTD);
   }
}

void DigitalBrain::WaypointAP (void)
{
	self->af->SetSimpleMode(TRUE);
   GroundCheck();

   if (self->curWaypoint)
   {
	   if(((AircraftClass*) self)->af->GetSimpleMode())
      {
		   SimpleGoToCurrentWaypoint();
	   }
	   else
      {
		   GoToCurrentWaypoint();
	   }
      lastMode = curMode;
      curMode = WaypointMode;
      if (groundAvoidNeeded)
         PullUp();
   }
   else
   {
      if (self->waypoint)
      {
         self->curWaypoint = self->waypoint;
      }
      else
      {
         Loiter();
      }
   }
}
// JPO - Lantirn mode auto pilot
void DigitalBrain::LantirnAP (void)
{
	float max_roll = 50.0F;

	//MI
	if(theLantirn->GetTFRMode() == LantirnClass::TFR_STBY)
	{
		//Here we want manual control
		self->af->SetMaxRoll(190.0F);
		rStick = UserStickInputs.rstick;
		pStick = UserStickInputs.pstick;     
		yPedal = UserStickInputs.rudder;
		throtl = UserStickInputs.throttle;
		return;
	}

	float headingErr;

	self->af->SetSimpleMode(FALSE);
	curMode = NoMode;
	// do nothing if on ground
	if ( self->OnGround() )
		return;

	//   self->af->SetSimpleMode(TRUE);
	//   GroundCheck();
	headingErr = holdPsi - af->sigma;
	if(g_bTFRFixes)
	{
		//disabled for now...
		//SetYpedal( headingErr * 0.05F * RTD * self->GetVt()/cornerSpeed);
		SetYpedal( 0.0F );

		//quick and dirty fix to level the wings! -NEES WORK, and possibly hold holdPsi!
		theLantirn->roll = self->Roll();
		float roll_multiply = 0.0F;
		//if (fabs(self->Roll()) * RTD > af->GetTFR_MaxRoll() / 2.0F)
		if (fabs(self->Roll()) * RTD > af->GetTFR_MaxRoll() / 1.0F)
			roll_multiply = 1.0F;
		else
			roll_multiply = 0.5F;
		if (fabs(self->Roll()) * RTD * roll_multiply > max_roll)
			roll_multiply = max_roll / (float) fabs(self->Roll()) * RTD;
		if (fabs(self->Roll() * RTD) < 0.001)
			roll_multiply = 0.0F;

		SetRstick( -self->Roll() * roll_multiply * RTD);
		SetMaxRoll (0.0F);
		SetMaxRollDelta (-self->Roll() * roll_multiply * RTD);


		float alterr = theLantirn->GetHoldHeight() + self->ZPos();
		float desGamma = alterr * af->GetTFR_Gain() * af->GetTFR_Corner() /self->GetKias();

		//if (theLantirn->gammaCorr < 0.0F)
			desGamma += theLantirn->gammaCorr * af->GetTFR_GammaCorrMult();
		//else if (theLantirn->gammaCorr < 5.0F)
		//	desGamma += 5.0F + (theLantirn->gammaCorr - 5.0F) / 2.0F;
		//else
		//	desGamma += theLantirn->gammaCorr;

		desGamma = max ( min ( desGamma, theLantirn->GetGLimit() * 5.0F), -theLantirn->GetGLimit() * 4.0F);
		if (theLantirn->evasize > 0)
			desGamma += af->GetEVA_Gain() * (theLantirn->evasize)*(theLantirn->evasize);

		if(!g_bCalibrateTFR_PitchCtrl)
			theLantirn->PID_error = desGamma - af->gmma * RTD;
		else
			theLantirn->PID_error = (theLantirn->GetTFRAlt() - 300.0F)/100 - af->gmma * RTD;

		theLantirn->MinG = max(-(theLantirn->GetGLimit()) / 2.0F, -2.0F);
		theLantirn->MaxG = min(theLantirn->GetGLimit() -1.0F, 6.5F -1.0F);

		theLantirn->PID_Output = PIDLoop (theLantirn->PID_error, af->GetPID_K(), af->GetPID_KD(),
			af->GetPID_KI(), SimLibMajorFrameTime, &theLantirn->PID_lastErr, &theLantirn->PID_MX,
			theLantirn->MaxG * af->GetTFR_Corner() / self->GetKias(),
			theLantirn->MinG * af->GetTFR_Corner() / self->GetKias(),
			af->GetTFR_LimitMX());

		float elevCmd = theLantirn->PID_Output;
		elevCmd *= self->GetKias() / af->GetTFR_Corner();
		float gammaCmd = elevCmd + (1.0F/self->platformAngles.cosphi);
		if (fabs(self->Roll()) < af->GetTFR_MaxRoll() * DTR)
		{
			if (theLantirn->evasize == 2)
				SetPstick (theLantirn->GetGLimit(), af->MaxGs(), AirframeClass::GCommand);
			else if (theLantirn->evasize >= 3)
				SetPstick (af->MaxGs(), af->MaxGs(), AirframeClass::GCommand);
			else
				SetPstick(min(max(gammaCmd, max(1.0F - (theLantirn->GetGLimit() - 1) / 2.0F, -2.0F)), 6.5F), maxGs, AirframeClass::GCommand);
		}
	}
	else
	{
		if (theLantirn->evasize == 0)
		{
			// JB 010325 Custom hold alt code to minimize neg g's.
			SetYpedal( 0.0F );
			SetRstick( -self->Roll() * 2.0F * RTD);
			SetMaxRoll (0.0F);

			float alterr = theLantirn->GetHoldHeight() + self->ZPos();
			alterr -= self->ZDelta();
			float desGamma = alterr * 0.015F;

			desGamma = max ( min ( desGamma, 30.0F), -30.0F);
			float elevCmd = desGamma - af->gmma * RTD;

			elevCmd *= 0.25F * self->GetKias() / 350.0F;

			if (fabs (af->gmma) < (45.0F * DTR))
				elevCmd /= self->platformAngles.cosphi;

			if (elevCmd > 0.0F)
				elevCmd *= elevCmd;
			else
				elevCmd *= -elevCmd;

			gammaHoldIError += 0.0025F*elevCmd;
			if (gammaHoldIError > 1.0F)
				gammaHoldIError = 1.0F;
			else if (gammaHoldIError < -1.0F)
				gammaHoldIError = -1.0F;

			float gammaCmd = gammaHoldIError + elevCmd + (1.0F/self->platformAngles.cosphi);
			SetPstick(min(max(gammaCmd, max(1.0f - (theLantirn->GetGLimit() - 1) / 2.0f, -2.0F)), 6.5F), maxGs, AirframeClass::GCommand);
		}
		else 
			SetPstick (theLantirn->GetGLimit(), af->MaxGs(), AirframeClass::GCommand);
		

		SetYpedal( headingErr * 0.05F * RTD * self->GetVt()/cornerSpeed);
		SetMaxRoll ((float)fabs(self->Roll()*RTD));

		if (self->Roll() > 5.0F * DTR)
			SetMaxRollDelta (-self->Roll()*RTD);
		else if (self->Roll() >= 0.0F * DTR)
			SetMaxRollDelta (-self->Roll()*RTD - 1.0F * DTR);
		else if (self->Roll() > -5.0F * DTR)
			SetMaxRollDelta (-self->Roll()*RTD + 1.0F * DTR);
		else
			SetMaxRollDelta (-self->Roll()*RTD);
	}
}
float DigitalBrain::PIDLoop(float error, float K, float KD, float KI, float Ts, float *lastErr, float *MX,
							float Output_Top, float Output_Bottom, bool LimitMX)
{
	float MP = K * error;
	float MD = KD/Ts * (error - *lastErr);
	*MX += KI*Ts * error;
	float Output = MP + MD + *MX;
	if (Output > Output_Top)
		Output = Output_Top;
	if (Output < Output_Bottom)
		Output = Output_Bottom;
	if (LimitMX)
		*MX = Output - MP - MD;
	*lastErr = error;

	return Output;
}
void DigitalBrain::RealisticAP(void)
{
	// do nothing if on ground
	if(self->OnGround())
		return;

#if 0  // MD -- 20031108: see "else"
	//Right switch
	if(self->IsOn(AircraftClass::AltHold))	//up
		AltHold();	
	else if(self->IsOn(AircraftClass::AttHold) && self->IsOn(AircraftClass::RollHold))	//down
		PitchRollHold();
	else
		AcceptManual();

	//Left switch
	if(!self->IsOn(AircraftClass::AttHold))	//does nothing in ATT HOLD position
	{
		if(self->IsOn(AircraftClass::RollHold))
			RollHold();
		else if(self->IsOn(AircraftClass::HDGSel))
			HDGSel();
		else if(self->IsOn(AircraftClass::StrgSel))
			FollowWP();
	}
#else
	// MD -- 20031108: reworking autopilot code so that it matches the real behavior a little
	// better.  The AP will hold pitch independently of the left switch state so that if you
	// want pitch hold and STRG SEL for example, you should be allowed to do that.  Old SP3
	// code would only allow fixed pitch and fixed roll hold mode.

	if(self->IsOn(AircraftClass::AttHold) || self->IsOn(AircraftClass::AltHold)) {

		//Right switch
		if(self->IsOn(AircraftClass::AltHold))	
			AltHold();	//up
		else
			if (self->IsOn(AircraftClass::AttHold))
				PitchHold();  //down
		else {
			AcceptManual();  // not really used unless something really wierd happens...
			return;
		}

		//Left switch
		if(self->IsOn(AircraftClass::RollHold))
			RollHold();
		else if(self->IsOn(AircraftClass::HDGSel))
			HDGSel();
		else if(self->IsOn(AircraftClass::StrgSel))
			FollowWP();
	}
#endif
	//get our pedal
	yPedal = UserStickInputs.rudder;
}
void DigitalBrain::AltHold(void)
{
	if(CheckAPParameters())
	{
	//	AcceptManual();  MD -- 20031108: switch will remain in selected pitch mode according to the dash one!
		self->SetAutopilot(AircraftClass::APOff);
		return;
	}
 	float alterr = currAlt + self->ZPos();
	alterr -= self->ZDelta();

	if (self->AutopilotType() == AircraftClass::LantirnAP)
		GammaHold(alterr * 0.015F);
	else
	{
		// MD -- 20031107: Adding some damping when you are close to the mark
		// this should get rid of most of the porpoise effect if we are lucky.
		// I'm no aerodynamics guru but it seems to me that when you are close
		// to the desired altitude, you want to shoot for a zero degree pitch
		// to stay straight and level.  This seems to do better than the original
		// algorithm that always applied the GammaHold(alterr * 0.015) pitch angle
		// correction but it seems like that would cause enough overshoot even
		// at small deltas from desired altitude so as to lead to the porpoising
		// effect we all love to hate.
		//  All that said, if you can do better than this, with a more scientific
		// approach, please go ahead!
		// The F-16 dash one says the autopilot holds alt +/- 100 feet.  The old
		// algorithm did that too but with a lot more gyration around the desired
		// altitude reference.

		// NB: it might be appropriate to add this for LantirnAP as well but changing
		// the pitch rate multipliers for that seems intuitively more risky so without
		// doing more investigation...well, leave it alone for now.

		float abs_alterr = fabsf(alterr);
		if (abs_alterr < 15.0F) {
			GammaHold(alterr * 0.0F);
		}
		else {
			if (abs_alterr < 50.0F) {
				GammaHold(alterr * 0.0015F);
			}
			else {
				if (abs_alterr < 100.0F)
					GammaHold(alterr * 0.005F);
				else
					GammaHold(alterr * 0.015F);
			}
		}
	}
}
void DigitalBrain::PitchRollHold(void)
{
	if(CheckAPParameters())
	{
	//	AcceptManual();  MD -- 20031108: switch will remain in selected pitch mode according to the dash one!
		self->SetAutopilot(AircraftClass::APOff);
		return;
	}

	float corrPitch = 0;
	float CurrentPitch = self->Pitch() * RTD;
	if(CurrentPitch < 0)
		CurrentPitch *= -1;
	
	//anything to do?
	if(self->Pitch() * RTD > destPitch + 0.5F ||  self->Pitch() * RTD < destPitch - 0.5F)
	{
		if(CurrentPitch > destPitch)
		{
			//down
			//How much to correct?
			corrPitch = CurrentPitch - destPitch;
			if(self->Pitch() * RTD > destPitch)
				pStick = ((0.5F * af->pstick) - (0.5F * max(corrPitch * 5.0F,15.0F) * DTR));
			//turned too far? correct
			else if(self->Pitch() * RTD < destPitch)
				pStick = ((0.5F * af->pstick) + (0.5F * max(corrPitch * 5.0F,15.0F) * DTR));
			else
				pStick = 0.0F;
		}
		else
		{
			//up
			//How much to correct?
			corrPitch = destPitch - CurrentPitch;
			if(self->Pitch() * RTD < destPitch)
				pStick = ((0.5F * af->pstick) + (0.5F * max(corrPitch * 5.0F,15.0F) * DTR));
			//turned too far? correct
			else if(self->Pitch() * RTD > destPitch)
				pStick = ((0.5F * af->pstick) - (0.5F * max(corrPitch * 5.0F,15.0F) * DTR));
			else
				pStick = 0.0F;
		}
	}
	else
		pStick = 0.0F;

	float corrRoll = 0;
	float CurrentRoll = self->Roll() * RTD;
	if(CurrentRoll < 0)
		CurrentRoll *= -1;
	//anything to do?
	if(self->Roll() * RTD > destRoll + 1.0F ||  self->Roll() * RTD < destRoll - 1.0F)
	{
		if(CurrentRoll > destRoll)
		{
			//bank left
			//How much to correct?
			corrRoll = CurrentRoll - destRoll;
			if(self->Roll() * RTD > destRoll)
				rStick = ((0.5F * af->rstick) - (0.5F * max(corrRoll, 6.5F) * DTR));
			//turned too far? correct
			else if(self->Roll() * RTD < destRoll)
				rStick = ((0.5F * af->rstick) + (0.5F * max(corrRoll, 6.5F) * DTR));
			else
				rStick = 0.0F;
		}
		else
		{
			//bank right
			//How much to correct?
			corrRoll = destRoll - CurrentRoll;
			if(self->Roll() * RTD < destRoll)
				rStick = ((0.5F * af->rstick) + (0.5F * max(corrRoll, 6.5F) * DTR));
			//turned too far? correct
			else if(self->Roll() * RTD > destRoll)
				rStick = ((0.5F * af->rstick) - (0.5F * max(corrRoll, 6.5F) * DTR));
			else
				rStick = 0.0F;
		}
	}
	else
		rStick = 0.0F;
}
void DigitalBrain::FollowWP(void)
{
	if(CheckAPParameters())
	{
	//	AcceptManual();  MD -- 20031108: switch will remain in selected pitch mode according to the dash one!
		self->SetAutopilot(AircraftClass::APOff);
		return;
	}

	float wpX, wpY, wpZ;
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

	if(self == playerAC && playerAC->FCC->GetStptMode() != FireControlComputer::FCCWaypoint &&
		playerAC->FCC->GetStptMode() != FireControlComputer::FCCMarkpoint &&
		playerAC->FCC->GetStptMode() != FireControlComputer::FCCDLinkpoint) 
	{
		AcceptManual();
		return;
	}

	if(self && self->curWaypoint)
		self->curWaypoint->GetLocation (&wpX, &wpY, &wpZ);
	else
	{
		AcceptManual();
		return;
	}
	//MI add in INS Drift
	if(g_bINS && g_bRealisticAvionics)
	{
		if (playerAC != NULL)
		{
			wpX += playerAC->GetINSLatDrift();
			wpY += playerAC->GetINSLongDrift();
		}
	}

   	/*------------------------------------*/
	/* Heading error for current waypoint */
	/*------------------------------------*/
	HeadingDifference = (float)atan2(wpY - self->YPos(), wpX - self->XPos()) - self->Yaw();
	if (HeadingDifference >= 180.0F * DTR)
		HeadingDifference -= 360.0F * DTR;
	else if (HeadingDifference <= -180.0F * DTR)
		HeadingDifference += 360.0F * DTR;

	HeadingDifference *= RTD;
	
	CheckForTurn();
}
void DigitalBrain::HDGSel(void)
{
	if(CheckAPParameters())
	{
	//	AcceptManual();  MD -- 20031108: switch will remain in selected pitch mode according to the dash one!
		self->SetAutopilot(AircraftClass::APOff);
		return;
	}

	float FinalHeading = OTWDriver.pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_HEADING);
	if(FinalHeading == 0)
		FinalHeading = 360;
	float curHeading = self->Yaw() * RTD;
	if(curHeading < 0)
		curHeading += 360;
	
	HeadingDifference = FinalHeading - curHeading;

	if(HeadingDifference >= 180)
		HeadingDifference -= 360;
	else if(HeadingDifference <= -180)
		HeadingDifference += 360;

	CheckForTurn();
}
void DigitalBrain::RollHold(void)
{
	if(CheckAPParameters())
	{
	//	AcceptManual();  MD -- 20031108: switch will remain in selected pitch mode according to the dash one!
		self->SetAutopilot(AircraftClass::APOff);
		return;
	}
	
	float corrRoll = 0;
	float CurrentRoll = self->Roll() * RTD;
	if(CurrentRoll < 0)
		CurrentRoll *= -1;
	//anything to do?
	if(self->Roll() * RTD > destRoll + 1.0F ||  self->Roll() * RTD < destRoll - 1.0F)
	{
		if(CurrentRoll > destRoll)
		{
			//bank left
			//How much to correct?
			corrRoll = CurrentRoll - destRoll;
			if(self->Roll() * RTD > destRoll)
				rStick = ((0.5F * af->rstick) - (0.5F * max(corrRoll, 6.5F) * DTR));
			//turned too far? correct
			else if(self->Roll() * RTD < destRoll)
				rStick = ((0.5F * af->rstick) + (0.5F * max(corrRoll, 6.5F) * DTR));
			else
				rStick = 0.0F;
		}
		else
		{
			//bank right
			//How much to correct?
			corrRoll = destRoll - CurrentRoll;
			if(self->Roll() * RTD < destRoll)
				rStick = ((0.5F * af->rstick) + (0.5F * max(corrRoll, 6.5F) * DTR));
			//turned too far? correct
			else if(self->Roll() * RTD > destRoll)
				rStick = ((0.5F * af->rstick) - (0.5F * max(corrRoll, 6.5F) * DTR));
			else
				rStick = 0.0F;
		}
	}
	else
		rStick = 0.0F;

	//here we get our Roll axis anyway, the AP just holds it
	rStick = 0.5F * af->rstick + 0.5F * UserStickInputs.rstick;
}

// MD -- 20031108: adding a pitch hold function so that the AP pitch hold modes
// can operate correctly with STRG SEL and HDG SEL
void DigitalBrain::PitchHold(void)
{
	if(CheckAPParameters())
	{
	//	pitch switch will remain in selected pitch mode according to the dash one!
		self->SetAutopilot(AircraftClass::APOff);
		return;
	}

	// Use the Gamma (pitch angle) hold command here since it seems to work right
	// and the pitch/roll hold code above in earlier versions of this file don't seem to
	// hold pitch at all to speak of.
	if (!self->IsOn(AircraftClass::StickStrng))
		GammaHold(destPitch);

	// Now grab any user input to emulate stick steering during autopilot operation.
	// AP just holds the set reference.  Pressure on the stick will change the reference
	// provided the pitch and other parameters that would disconnect the AP stay within limits.
	// Be careful to take this input only if there is some pressure on the stick though!
	if ((UserStickInputs.pstick > 0.05F) || (UserStickInputs.pstick < -0.05F)) {
		self->SetAPFlag(AircraftClass::StickStrng);
		pStick = 0.0F * af->pstick + 0.5F * UserStickInputs.pstick;
		self->SetNewPitch();
	} else {
		if (self->IsOn(AircraftClass::StickStrng))
			self->ClearAPFlag(AircraftClass::StickStrng);
	}

}

#define AP_TURN 1	//faster and it oscillates
void DigitalBrain::CheckForTurn(void)
{
	//anything to do for us?
	if(HeadingDifference < -1.0F || HeadingDifference > 1.0F)
	{
		//MI DON'T TOUCH THIS CODE!!!!
		//my brain was smoking after I got this down! It seems to work just fine.
		if(HeadingDifference < 0)
		{
			//turn left
			if(self->Roll() * RTD > -29.0F)
				rStick = ((0.5F * af->rstick) - (AP_TURN * min(29 - (self->Roll() * RTD < 0 ? -self->Roll() * RTD : 0), 10) * DTR));
			else if(self->Roll() * RTD < -30.5F)
				rStick = ((0.5F * af->rstick) + (AP_TURN * min(29 - (self->Roll() * RTD < 0 ? (-self->Roll() * RTD > 29 ? (-self->Roll() * RTD - 29) : -self->Roll() * RTD) : 0), 10) * DTR));
			else
				rStick = 0.0F;
		}
		else
		{
			//turn right
			if(self->Roll() * RTD < 29.0F)
				rStick = ((0.5F * af->rstick) + (AP_TURN * min(29 - (self->Roll() * RTD > 0 ? self->Roll() * RTD : 0), 10) * DTR));
			else if(self->Roll() * RTD > 30.5F)
				rStick = ((0.5F * af->rstick) - (AP_TURN * min(29 - (self->Roll() * RTD > 0 ? (self->Roll() * RTD > 29 ? (self->Roll() * RTD - 29) : self->Roll() * RTD) : 0), 10) * DTR));
			else
				rStick = 0.0F;
		}
	}
	else
	{
		if(self->Roll() * RTD > 0.5F || self->Roll() *RTD < -0.5F)
		{
			if(self->Roll() * RTD > 0.5F)
				bank = (self->Roll() * RTD) - 1;
			else
				bank = (-self->Roll() * RTD) + 1;
			if(self->Roll() * RTD < -0.5F)
				rStick = ((0.5F * af->rstick) + (0.5F * max(bank,2.5F) * DTR));
			else if(self->Roll() * RTD > 0.5F)
				rStick = ((0.5F * af->rstick) - (0.5F * max(bank,2.5F) * DTR));
			else
				rStick = 0.0F;
		}
		else
			rStick = 0.0F;
	}
}

// MD -- 20031108: if any of these conditions are true, then according to the dash one
// the autopilot automatically disconnects.  The way it does this is by releasing the
// pitch AP control switch which is normally held in place magnetically when a valid
// autopilot mode is connected and none of these conditions do apply.

bool DigitalBrain::APAutoDisconnect(void)
{
	if(af->gearHandle == 1.0F) // handle is down
		return TRUE;

	if (self->mFaults->GetFault(FaultClass::flcs_fault))
		return TRUE;

	if (af->IsEngineFlag(AirframeClass::FuelDoorOpen))
		return TRUE;

	if (!self->HasPower(AircraftClass::APPower))
		return TRUE;
	
	if (self->TrimAPDisc)
		return TRUE;

	if (self->TEFExtend)
		return TRUE;

	if (self->GetAlpha() > 15.0F)
		return TRUE;

	if (af->IsSet(AirframeClass::MPOverride))
		return TRUE;

	return FALSE;
}

int DigitalBrain::CheckAPParameters(void)
{
	//dont do anything if not within parameters
	if((self->Pitch() * RTD > 60.2F) || (self->Pitch() * RTD < -60.2F))
		return TRUE;
	else if((self->Roll() * RTD > 60.2F) || (self->Roll() * RTD < -60.2F))
		return TRUE;
	else if(self->af->mach > 0.95 || -self->ZPos() > 40000)
		return TRUE;
	else
		return FALSE;

	return FALSE;
}
void DigitalBrain::AcceptManual(void)
{
	//me123 said the switches reset themselves. So here we go....
	self->SetAutopilot(AircraftClass::APOff);
	self->ClearAPFlag(AircraftClass::AttHold);
	self->ClearAPFlag(AircraftClass::AltHold);
	/*rStick = UserStickInputs.rstick;
	pStick = UserStickInputs.pstick;     
	yPedal = UserStickInputs.rudder;
	throtl = UserStickInputs.throttle;*/
}