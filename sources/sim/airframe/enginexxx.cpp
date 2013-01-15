/******************************************************************************/
/*                                                                            */
/*  Unit Name : engine.cpp                                                    */
/*                                                                            */
/*  Abstract  : Models engine thrust.                                         */
/*                                                                            */
/*  Naming Conventions :                                                      */
/*                                                                            */
/*      Public Functions    : Mixed case with no underscores                  */
/*      Private Functions   : Mixed case with no underscores                  */
/*      Public Functions    : Mixed case with no underscores                  */
/*      Global Variables    : Mixed case with no underscores                  */
/*      Classless Functions : Mixed case with no underscores                  */
/*      Classes             : All upper case seperated by an underscore       */
/*      Defined Constants   : All upper case seperated by an underscore       */
/*      Macros              : All upper case seperated by an underscore       */
/*      Structs/Types       : All upper case seperated by an underscore       */
/*      Private Variables   : All lower case seperated by an underscore       */
/*      Public Variables    : All lower case seperated by an underscore       */
/*      Local Variables     : All lower case seperated by an underscore       */
/*                                                                            */
/*  Development History :                                                     */
/*  Date      Programer           Description                                 */
/*----------------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                               */
/*                                                                            */
/******************************************************************************/
#include "stdhdr.h"
#include "airframe.h"
#include "aircrft.h"
#include "fack.h"
#include "MsgInc/DamageMsg.h"
#include "falcsess.h"
#include "campbase.h"
#include "fsound.h"
#include "soundfx.h"
#include "simIO.h"
#include "PilotInputs.h"
#include "sms.h"  // MD
#include "Otwdrive.h"//Cobra
#include "Graphics/Include/RenderOW.h"

extern OTWDriverClass OTWDriver;//Cobra

extern PilotInputs UserStickInputs;//TJL 01/22/04 Multi-engine

static float fireTimer = 0.0F;
static const float eputime = 600.0f; // 10 minutes epu fuel
static const float jfsrechargetime = 60.0f; // xx minutes of JFS cranking
static const float ftitrate = 0.7f;
int supercruise = FALSE;
extern bool g_bRealisticAvionics;

// MD -- 20040210: adding analog cuttoff value support
extern bool g_bUseAnalogIdleCutoff;

#include "otwdrive.h"
#include "cpmanager.h"

/********************************************************************/
/*                                                                  */
/* Routine: void AirframeClass::EngineModel (time )                 */
/*                                                                  */
/* Description:                                                     */
/*    Models engine thrust as a function of mach and altitude.      */
/*    Calcualtes body axis forces from thrust.                      */
/*                                                                  */
/* Inputs:                                                          */
/*    time                                                          */
/*                                                                  */
/* Outputs:                                                         */
/*    None                                                          */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
void AirframeClass::EngineModel(float dt)
{
	float th1, th2;
	float thrtb1;
	float tgross;
	float fuelFlowSS;
	//float ta01, rpmCmd; TJL 08/21/04 ta01 no longer used
	float rpmCmd;
	int aburnLit;
	float spoolrate = auxaeroData->normSpoolRate;
	//TJL 02/22/04 
	float spoolAltRate = (-platform->ZPos()/25000.0f) + (-mach/2.0F);

	
			
	
    // JPO should we switch on the EPU ?
    if (epuFuel > 0.0f) //only relevant if there is fuel
	{ 
		// MD -- 20040531: adding test to make sure that the EPU keeps running when you land if it was
		// already running (previous logic shut EPU off on touchdown)
		if ((GetEpuSwitch() == ON) || (GeneratorRunning(GenEpu) && (GetEpuSwitch() != OFF)))
		{// pilot command
			GeneratorOn(GenEpu);
		} 
		// auto mode
		else if (!GeneratorRunning(GenMain) && !GeneratorRunning(GenStdby) && IsSet(InAir)) {
			GeneratorOn(GenEpu);
		}
		else if (GetEpuSwitch() == OFF) 
		{
			GeneratorOff(GenEpu);
			EpuClear();
		}
		else 
		{
			GeneratorOff(GenEpu);
			EpuClear();
		}
	}
	else 
	{
		GeneratorOff(GenEpu);
		EpuClear();
	}
    // check hydraulics and generators 
    if (GeneratorRunning(GenMain) || GeneratorRunning(GenStdby)) {
		HydrRestore(HYDR_ALL); // restore those systems we can
    }
    else if (GeneratorRunning(GenEpu)) {
		HydrDown (HYDR_B_SYSTEM); // B system now dead
		HydrRestore(HYDR_A_SYSTEM); // A still OK
    }
    else {
		HydrDown(HYDR_ALL); // all off
    }
	
    // JPO - if the epu is running - well its running!
    // this may be independant of the engine if the pilot wants to check
    if (GeneratorRunning(GenEpu)) {
		EpuClear();
		// below 80%rpm, epu burns fuel.
		if (rpm < 0.80f) {
			EpuSetHydrazine();
			epuFuel -= 100.0f * dt / auxaeroData->epuBurnTime; // burn some hydrazine
			if (epuFuel <= 0.0f) {
				epuFuel = 0.0;
				GeneratorBreak(GenEpu); // well not broken, but done for.
				GeneratorOff(GenEpu);
			}
		}
		EpuSetAir(); // always true, means running really.
    }
	
    // JPO: charge the JFS accumulators, up to 100% 
    if (rpm > auxaeroData->jfsMinRechargeRpm && jfsaccumulator < 100.0f /* 2002-04-11 ADDED BY S.G. If less than 0, don't recharge */ && jfsaccumulator >= 0.0f ) {
		jfsaccumulator += 100.0f * dt / auxaeroData->jfsRechargeTime;
		jfsaccumulator = min(jfsaccumulator, 100.0f);
    }
    // transfer fuel
    FuelTransfer(dt);
	
#if 0 // not ready yet.JPO, but basically it should work as normal, but ignore the throttle.
    if (IsSet(ThrottleCheck)) {
		rpm      = oldRpm[0]; // just remember where we were... JPO
		if(throtl >= 1.5F)
			throtl = pwrlev;
		if (fabs (throtl - pwrlev) > 0.1F)
			ClearFlag(ThrottleCheck);
		pwrlev = throtl;
    }
    else
    {
		if (fabs (throtl - pwrlev) > 0.1F)
            ClearFlag (EngineOff);
    }
#endif
	
    if (IsSet(EngineStopped)) {
		ftit = Math.FLTust(0.0f, 20.0f, dt, oldFtit); // cool down the engine
		float modRpm = (float)(rpm*rpm*rpm*rpm*sqrt(rpm));
		
		// Am I increasing the rpm (but not yet in afterburner)?
		if (rpm > oldp01[0])
			Math.FLTust (modRpm, 4.0F, dt, oldp01);
		// Must be decreasing then...
		else {
			// Now check if the 'heat' is still above 1.00 (100%)
			if (oldp01[0] > 1.0F)
				Math.FLTust (modRpm, 7.0F, dt, oldp01);
			else
				Math.FLTust (modRpm, 2.0F, dt, oldp01);
		}
		
		spoolrate = auxaeroData->flameoutSpoolRate; // spool down rate
		fireTimer = 0.0f; // reset - engine is switched off
		// switch all to 0.
		tgross = 0.0F;
		fuelFlow = 0.0F;
		thrtab = 0.0F;
		thrust = 0.0f;
		
		// broken engine - anything but a flame out?
		if((platform->mFaults->GetFault(FaultClass::eng_fault) & ~FaultClass::fl_out) != 0) {
			rpmCmd = 0.0f; // engine must be seized, not going to start or windmill
		}
		else if (IsSet (JfsStart)) {
			rpmCmd = 0.25f; // JFS should take us up to 25%
			spoolrate = 15.0f;
			//decrease spin time
			JFSSpinTime -= SimLibMajorFrameTime;
			if(JFSSpinTime <= 0)
				ClearFlag(JfsStart);
		}
		//TJL 01/18/04 Added parens to correct operator precedence error
		else { // engine windmill (~12% at 450 knts) (me123 - this works on mine)
			rpmCmd = (platform->GetKias() / 450.0f) * 0.12f;
		}
		
		// attempt to spool up/down
		rpm = Math.FLTust (rpmCmd, spoolrate, dt, oldRpm);

		// MD -- 20040210: add check for throttle up to idle to trigger engine light
		// use with caution...this was done at speed and not extensively tested.
		if (g_bUseAnalogIdleCutoff && (rpm >= 0.20F) && !IO.IsAxisCutOff(AXIS_THROTTLE))
		{
			ClearFlag(AirframeClass::EngineStopped);
			platform->mFaults->ClearFault(FaultClass::eng_fault, FaultClass::fl_out);
		}
    }
    else if (!IsSet(EngineOff))
    {
		/*------------------*/
		/* get gross thrust */
		/*------------------*/
		if((platform->mFaults->GetFault(FaultClass::eng_fault) & FaultClass::fl_out) ||
			(g_bUseAnalogIdleCutoff && IO.IsAxisCutOff(AXIS_THROTTLE)))
		{
			SetFlag(EngineStopped); //JPO - engine is now stopped!
			throtl = 0.0F;
		}
		pwrlev = throtl;
		// AB Failure
		
		
		
		//MI
		if(auxaeroData->DeepStallEngineStall)
		{
			//me123 if in deep stall and in ab lets stall the engine
			// JPO - 10% chance, each time we check...
			if (stallMode >= DeepStall && pwrlev >= 1.0F && rand() % 10 == 1)
			{ 
				SetFlag(EngineStopped);
				// mark it as a flame out
				platform->mFaults->SetFault(FaultClass::eng_fault, FaultClass::fl_out, FaultClass::fail, FALSE);
			}
		}
		
		if (platform->IsSetFlag(MOTION_OWNSHIP))
		{
			if (platform->mFaults->GetFault(FaultClass::eng_fault) & FaultClass::a_b)
			{
				pwrlev = min (pwrlev, 0.99F);
			}
			
			if (platform->mFaults->GetFault(FaultClass::eng_fault) & FaultClass::efire)
			{
				pwrlev *= 0.5F;
				
				if (fireTimer >= 0.0F)
					fireTimer += max (throtl, 0.3F) * dt;
				
				// On fire long enough, blow up
				if (fireTimer > 60.0F) // 60 seconds at mil power
				{
					fireTimer = -1.0F;
					FalconDamageMessage* message;
					message = new FalconDamageMessage (platform->Id(), FalconLocalGame );
					message->dataBlock.fEntityID  = platform->Id();
					
					message->dataBlock.fCampID = platform->GetCampaignObject()->GetCampID();
					message->dataBlock.fSide   = platform->GetCampaignObject()->GetOwner();
					message->dataBlock.fPilotID   = ((AircraftClass*)platform)->pilotSlot;
					message->dataBlock.fIndex     = platform->Type();
					message->dataBlock.fWeaponID  = platform->Type();
					message->dataBlock.fWeaponUID = platform->Id();
					message->dataBlock.dEntityID  = message->dataBlock.fEntityID;
					message->dataBlock.dCampID = message->dataBlock.fCampID;
					message->dataBlock.dSide   = message->dataBlock.fSide;
					message->dataBlock.dPilotID   = message->dataBlock.fPilotID;
					message->dataBlock.dIndex     = message->dataBlock.fIndex;
					message->dataBlock.damageType = FalconDamageType::CollisionDamage;
					message->dataBlock.damageStrength = 2.0F * platform->MaxStrength();
					message->dataBlock.damageRandomFact = 1.5F;
					
					message->RequestOutOfBandTransmit ();
					FalconSendMessage (message,TRUE);
				}
			}
		}
		if (engineData->hasAB) // JB 010706
			pwrlev = max (min (pwrlev, 1.5F), 0.0F);
		else
			pwrlev = max (min (pwrlev, 1.0F), 0.0F);
		
		if (rpm < 0.68f) { // below Idle
			rpmCmd = 0.7f;
			spoolrate = auxaeroData->lightupSpoolRate;
			thrtb1 = 0.0f;
			if (rpm > 0.5f)
				ClearFlag(JfsStart);
			ftit = Math.FLTust(5.1F * (rpm/0.7f), ftitrate, dt, oldFtit);
			// sfr: added rampstart fix
			EngineRpmMods(rpmCmd);
			rpmCmd = EngineRpmMods(rpmCmd);
			spoolrate = spoolAltRate + spoolrate;
			rpm = Math.FLTust (rpmCmd, spoolrate, dt, oldRpm);
			if (fuel > 0.0F){
				rpm = max (rpm, 0.01F);
			}
		}
		//else if (pwrlev <= 1.0)
		if ((pwrlev <= 1.0f && rpm <= 1.0f) || (pwrlev > 1.0f && rpm <= 1.0f))
		{
			/*-------------------*/
			/* Mil power or less */
			/*-------------------*/
			th1 = Math.TwodInterp (-z, mach, engineData->alt,
				engineData->mach, engineData->thrust[0], engineData->numAlt,
				engineData->numMach, &curEngAltBreak, &curEngMachBreak);
			th2 = Math.TwodInterp (-z, mach, engineData->alt,
				engineData->mach, engineData->thrust[1], engineData->numAlt,
				engineData->numMach, &curEngAltBreak, &curEngMachBreak);
			
			aburnLit = FALSE;
			// sfr: reverting back old logic
			thrtb1 = ((th2 - th1)*pwrlev + th1) / mass; 
			//thrtb1 = (3.33f*(th2 - th1)*(rpm - 0.7f) + th1)/ mass; // saints code
			rpmCmd = 0.7F + 0.3F * pwrlev;

			// sfr: added per instructions
			// sfr: commenting one of lines
			//EngineRpmMods(rpmCmd);
			rpmCmd = EngineRpmMods(rpmCmd);
			//TJL 02/22/04 Add in the alt
			spoolrate = spoolAltRate + spoolrate;
			rpm = Math.FLTust (rpmCmd, spoolrate, dt, oldRpm);
			if (fuel > 0.0F){
				rpm = max (rpm, 0.01F);
			}
			// sfr: end added

			// ftit calculated
			if (rpm < 0.9F)
			{
				ftit = Math.FLTust(5.1F + (rpm - 0.7F) / 0.2F * 1.0F, ftitrate, dt, oldFtit);
			}
			else if (rpm < 1.0F)
			{
				ftit = Math.FLTust(6.1F + (rpm - 0.9F) / 0.1F * 1.5F, ftitrate, dt, oldFtit);
			}
		}
		else
			/*--------------------------*/
			/* Some stage of afterburner */
			/*--------------------------*/
		{
			th1 = Math.TwodInterp (-z, mach, engineData->alt,
				engineData->mach, engineData->thrust[1], engineData->numAlt,
				engineData->numMach, &curEngAltBreak, &curEngMachBreak);
			th2 = Math.TwodInterp (-z, mach, engineData->alt,
				engineData->mach, engineData->thrust[2], engineData->numAlt,
				engineData->numMach, &curEngAltBreak, &curEngMachBreak);
			
			aburnLit = TRUE;
			// sfr: reverting back code 
			thrtb1 = (2.0F*(th2 - th1)*(pwrlev-1.0F) + th1) / mass;
			//thrtb1 = (3.33F*(th2 - th1)*(rpm - 0.7F) + th1) / mass; // saints code
			rpmCmd = 1.0F + 0.06F * (pwrlev - 1.0F);
			// sfr: added per instructions
			// sfr: commenting one of lines
			//EngineRpmMods(rpmCmd);
			rpmCmd = EngineRpmMods(rpmCmd);
			//TJL 02/22/04 Add in the alt
			spoolrate = spoolAltRate + spoolrate;
			rpm = Math.FLTust (rpmCmd, spoolrate, dt, oldRpm);
			if (fuel > 0.0F){
				rpm = max (rpm, 0.01F);
			}
			// sfr: end added

			// ftit calculated
			ftit = Math.FLTust(7.6F + (rpm - 1.0F) / 0.03F * 0.1F, ftitrate, dt, oldFtit);
		}
		
		/*--------------------------------*/
		/* scale thrust to reference area */
		/*--------------------------------*/
		thrtab = thrtb1 * engineData->thrustFactor;
		
		
		/*-----------------*/
		/* engine dynamics */
		/*-----------------*/
		if(IsSet(Trimming))// || simpleMode == SIMPLE_MODE_AF)
		{
			ethrst = 1.0F;
			tgross = thrtab;
			olda01[0] = tgross;
			olda01[1] = tgross;
			olda01[2] = tgross;
			olda01[3] = tgross;
			
			rpm = rpmCmd;
			oldRpm[0] = rpm;
			oldRpm[1] = rpm;
			oldRpm[2] = rpm;
			oldRpm[3] = rpm;
		}
		else
		{
			//TJL 08/21/04 This lag filter no longer needed.
			tgross = thrtab;
			/*
			if(aburnLit)
			{
				
				//AB
				ta01 = auxaeroData->abSpoolRate + spoolAltRate;
				tgross = Math.FLTust(thrtab,ta01,dt,olda01);
			}
			else
			{
				//MIL
				if(pwrlev <= 1.0F)
					ta01 = auxaeroData->normSpoolRate + spoolAltRate;
				else
					ta01 = auxaeroData->abSpoolRate + spoolAltRate;
				
				tgross = Math.FLTust(thrtab,ta01,dt,olda01);
			}*/

		}
		
		/*-----------*/
		/*   burn fuel */
		/*-----------*/
		if (AvailableFuel() <= 0.0f || IsEngineFlag(MasterFuelOff)) { // no fuel - dead engine.
			SetFlag(EngineStopped);
			// mark it as a flame out
			platform->mFaults->SetFault(FaultClass::eng_fault, FaultClass::fl_out, FaultClass::fail, FALSE);
		}
		else
		{
			// JPO - back to basics... this stuff doesn't have to be complicated surely.
			// fuel flow is proportional to thrust.
			// thrust factor is already in, its just that tgross is thrust/mass, 
			// so we get rid of the mass component again.
			
			if(engineData->hasFuelFlow) // MLR 5/17/2004 - 
			{
				// sfr: new fuel code per instructions
				if(aburnLit == TRUE)
				{
					float fflow1, fflow2;

					fflow1 = Math.TwodInterp (-z, mach, engineData->alt,
						engineData->mach, engineData->fuelflow[1],engineData->numAlt,
						engineData->numMach, &curEngAltBreak, &curEngMachBreak);
					fflow2 = Math.TwodInterp (-z, mach, engineData->alt,
						engineData->mach, engineData->fuelflow[2],engineData->numAlt,
						engineData->numMach, &curEngAltBreak, &curEngMachBreak);
					
					
					//fuelFlowSS = (33.3333333F*(fflow2 - fflow1)*(rpm-1.0F) + fflow1);
					 fuelFlowSS = (2.0F * ( fflow2 - fflow1)* (pwrlev - 1.0F) + fflow1);
				}
				else
				{ 
					float fflow1, fflow2;

					fflow1 = Math.TwodInterp (-z, mach, engineData->alt,
						engineData->mach, engineData->fuelflow[0],engineData->numAlt,
						engineData->numMach, &curEngAltBreak, &curEngMachBreak);
					fflow2 = Math.TwodInterp (-z, mach, engineData->alt,
						engineData->mach, engineData->fuelflow[1],engineData->numAlt,
						engineData->numMach, &curEngAltBreak, &curEngMachBreak);

					//fuelFlowSS = ((fflow2 - fflow1)*(rpm-.7f)*3.33333f + fflow1);
					fuelFlowSS = (fflow2 - fflow1)* pwrlev + fflow1;
				}
				// end fuel

				// sfr: old fuel
#if 0
				if(rpm>1.0)
				{
					float fflow1, fflow2;

					fflow1 = Math.TwodInterp (-z, mach, engineData->alt,
						engineData->mach, engineData->fuelflow[1],engineData->numAlt,
						engineData->numMach, &curEngAltBreak, &curEngMachBreak);
					fflow2 = Math.TwodInterp (-z, mach, engineData->alt,
						engineData->mach, engineData->fuelflow[2],engineData->numAlt,
						engineData->numMach, &curEngAltBreak, &curEngMachBreak);
					
					
					fuelFlowSS = (33.3333333F*(fflow2 - fflow1)*(rpm-1.0F) + fflow1);
				}
				else
				{ 
					float fflow1, fflow2;

					fflow1 = Math.TwodInterp (-z, mach, engineData->alt,
						engineData->mach, engineData->fuelflow[0],engineData->numAlt,
						engineData->numMach, &curEngAltBreak, &curEngMachBreak);
					fflow2 = Math.TwodInterp (-z, mach, engineData->alt,
						engineData->mach, engineData->fuelflow[1],engineData->numAlt,
						engineData->numMach, &curEngAltBreak, &curEngMachBreak);

					fuelFlowSS = ((fflow2 - fflow1)*(rpm-.7f)*3.33333f + fflow1);
				}
				// don't want to use thrust multiplier
				fuelFlowSS *= auxaeroData->nEngines;
#endif
			}
			else
			{
				if (aburnLit)
				{
					fuelFlowSS =  auxaeroData->fuelFlowFactorAb * tgross  * mass;
				}
				else
				{ 
					fuelFlowSS = auxaeroData->fuelFlowFactorNormal * tgross * mass;
				}
			}
			
			// For simplified model, burn less fuel
			if (IsSet(Simplified))
			{
				fuelFlowSS *= 0.75F;
			}
			
			if (!platform->IsSetFlag(MOTION_OWNSHIP))
			{
				fuelFlowSS *= 0.75F;
			}
			
			fuelFlow += (fuelFlowSS - fuelFlow) / 10;
			
			/*----------------------------------------------------------*/
			/* If fuel flow less < 100 lbs/min fuel flow == 100lbs/min) */
			/*----------------------------------------------------------*/
			if (fuelFlow < auxaeroData->minFuelFlow)
				fuelFlow = auxaeroData->minFuelFlow;//me123 from 1000
			if (fuelFlowSS < auxaeroData->minFuelFlow)
				fuelFlowSS = auxaeroData->minFuelFlow;//me123 from 1000
			
			if (!IsSet(NoFuelBurn))
			{
				// JPO - fuel is now burnt and transferred.
				BurnFuel(fuelFlowSS * dt / 3600.0F);
#if 0 // old code
				if (externalFuel > 0.0)
					externalFuel -= fuelFlowSS * dt / 3600.0F;
				else
					fuel = fuel - fuelFlowSS * dt / 3600.0F ;//me123 deleted + externalFuel;
#endif	     
				weight -= fuelFlowSS * dt / 3600.0F;
				mass    = weight / GRAVITY;
			}
			
			/*
			if (IsSet(Refueling))
			{
			fuel += 3000.0F * dt;
			if (fuel > 12000.0F)
			fuel = 12000.0F;
			}
			*/
		}
		//TJL 02/21/04 
		// sfr: commenting this, done inside ifelse
		/*
		EngineRpmMods(rpmCmd);
		rpmCmd = EngineRpmMods(rpmCmd);
		//TJL 02/22/04 Add in the alt
		spoolrate = spoolAltRate + spoolrate;

		rpm = Math.FLTust (rpmCmd, spoolrate, dt, oldRpm);
		if (fuel > 0.0F)
			rpm = max (rpm, 0.01F);
			*/
		
		
		// ADDED BY S.G. TO SIMULATE THE HEAT PRODUCED BY THE ENGINE
		// I'M USING A PREVIOUSLY UNUSED ARRAY CALLED oldp01 FOR ENGINE HEAT TEMPERATURE
		// Afterburner lit?
		if (rpm > 100.0f)
			Math.FLTust (rpm + 0.5f, 0.5F, dt, oldp01);
		else {
			// the 'modified rpm will be rpm^4.5
			float modRpm = (float)(rpm*rpm*rpm*rpm*sqrt(rpm));
			
			// Am I increasing the rpm (but not yet in afterburner)?
			if (rpm > oldp01[0])
				Math.FLTust (modRpm, 4.0F, dt, oldp01);
			// Must be decreasing then...
			else {
				// Now check if the 'heat' is still above 1.00 (100%)
				if (oldp01[0] > 1.0F)
					Math.FLTust (modRpm, 7.0F, dt, oldp01);
				else
					Math.FLTust (modRpm, 2.0F, dt, oldp01);
			}
		}
		
		// END OF ADDED SECTION
		/*------------*/
		/* net thrust */
		/*------------*/
		if (platform->IsPlayer() && supercruise){
			thrust = tgross * ethrst*1.5F;
		}
		else {
			thrust = tgross * ethrst;
		}
		//Cobra Thrust Reverse
		if (auxaeroData->hasThrRev){
			if (platform->OnGround() && thrustReverse == 2){
				thrust = (-thrust * 0.40f);
			}
			//Cobra Thrust Reverse
			static int doOnce = 0;
			if (platform->IsPlayer() && thrustReverse == 2 && doOnce == 0){
				OTWDriver.ToggleThrustReverseDisplay();
				doOnce = 1;
			}
			else if (platform->IsPlayer() && thrustReverse == 0 && doOnce == 1){
				OTWDriver.ToggleThrustReverseDisplay();
				doOnce = 0;
			}
		}
		
		if(stallMode >= EnteringDeepStall){
			thrust *= 0.1f;
		}
		
   }
   else
   {
	   if (IsSet(ThrottleCheck))
	   {
		   if(throtl >= 1.5F)
			   throtl = pwrlev;
		   if (g_bUseAnalogIdleCutoff)
		   {
			   if (!IO.IsAxisCutOff(AXIS_THROTTLE) && (rpm > 0.20F))
				   ClearFlag(ThrottleCheck);
		   }
		   else
		   {
			   if (fabs (throtl - pwrlev) > 0.1F)
					ClearFlag(ThrottleCheck);
		   }
		   pwrlev = throtl;
	   }
	   else
	   {
		   if (g_bUseAnalogIdleCutoff)
		   {
			   if (!IO.IsAxisCutOff(AXIS_THROTTLE))
				   ClearFlag (EngineOff);
		   }
		   else
		   {
			   if (fabs (throtl - pwrlev) > 0.1F)
					ClearFlag (EngineOff);
		   }
	   }
	   
	   thrust   = 0.0F;
	   tgross   = 0.0F;
	   fuelFlow = 0.0F;
	   rpm      = oldRpm[0]; // just remember where we were... JPO
	   // Changed so the Oil light doesn't come on :-) - RH
	   ftit     = 5.85f;
   }
   
   // turn on stdby generator
   if (rpm > auxaeroData->stbyGenRpm && platform->MainPowerOn()) {
       GeneratorOn(GenStdby);
   }
   else
	   GeneratorOff(GenStdby);
   
   // tunr on main generator
   if (rpm > auxaeroData->mainGenRpm && platform->MainPowerOn()) {
       GeneratorOn(GenMain);
   }
   else
	   GeneratorOff(GenMain);
   
   if (IsSet(OnObject) && vcas < 175) // JB carrier
	   // RV - Biker - Use catapult thrust multiplier from FMs
	   //thrust *= 4;
	   thrust *= GetCatapultThrustMultiplier();


   /*------------------*/
   /* body axis accels */
   /*------------------*/
   if (nozzlePos == 0) { // normal case JPO
	   xprop =  thrust;
	   yprop =  0.0F;
	   zprop =  0.0F;
	   /*-----------------------*/
	   /* stability axis accels */
	   /*-----------------------*/
	   xsprop =  xprop *platform->platformAngles->cosalp;
	   ysprop =  yprop;
	   //   zsprop = 0.0F;		//assume flcs cancels this out? (makes life easier)
	   //   zsprop = -thrust*platform->platformAngles->sinalp * 0.001F; //why the 0.001F ?
	   //	zsprop = -thrust*platform->platformAngles->sinalp; // JPO previous
	   zsprop = - xprop * platform->platformAngles->sinalp;
   }
   else { // harrier fake stuff - doesn't really work.
	   mlTrig noz;
	   mlSinCos(&noz, nozzlePos);
	   xprop = thrust * noz.cos;
	   yprop =  0.0F;
	   zprop = -thrust * noz.sin;
	   /*-----------------------*/
	   /* stability axis accels */
	   /*-----------------------*/
	   xsprop =  xprop *platform->platformAngles->cosalp;
	   ysprop =  yprop;
	   //   zsprop = 0.0F;		//assume flcs cancels this out? (makes life easier)
	   //   zsprop = -thrust*platform->platformAngles->sinalp * 0.001F; //why the 0.001F ?
	   //	zsprop = -thrust*platform->platformAngles->sinalp; // JPO previous
	   zsprop = - xprop * platform->platformAngles->sinalp +
		   zprop * platform->platformAngles->cosalp;
	   
   }
   
   ShiAssert(!_isnan(platform->platformAngles->cosalp));
   ShiAssert(!_isnan(platform->platformAngles->sinalp));
   
   /*------------------*/
   /* wind axis accels */
   /*------------------*/
   xwprop =  xsprop*platform->platformAngles->cosbet +
	   ysprop*platform->platformAngles->sinbet;
   ywprop = -xsprop*platform->platformAngles->sinbet +
	   ysprop*platform->platformAngles->cosbet;
   zwprop =  zsprop;
}

//**************************************
// Multi-Engine Code Start
// Code is expandable. 
// Initial write has engine1 (left), engine2 (right)
// Copy/Paste of original Engine model code with mods
//**************************************

//TJL 01/11/04 Multi-Engine
void AirframeClass::MultiEngineModel(float dt)
{
	float th1, th2, th12, th22;
	float thrtb1, thrtb2;
	float tgross, tgross2;
	//float fuelFlowSS;
	//float fuelFlowSS2;//Engine 2
	float fuelFlowTotal;
	//float ta01, rpmCmd; TJL 08/21/04 ta01 and ta02 no longer used
	float rpmCmd;
	float rpmCmd2;
	//float ta02, rpmCmd2;//Engine 2
	int aburnLit = FALSE;
	int aburnLit2 = FALSE;
	float spoolrate = auxaeroData->normSpoolRate;
	float spoolrate2 = auxaeroData->normSpoolRate;
	//TJL 02/22/04 
	float spoolAltRate = (-platform->ZPos()/25000.0f) + (-mach/2.0F);

//***********************************************
	// #1
    // JPO should we switch on the EPU ?
    if (epuFuel > 0.0f) //only relevant if there is fuel
	{ 
		if (GetEpuSwitch() == ON) 
		{// pilot command
			GeneratorOn(GenEpu);
		} 
		// auto mode
		else if (!GeneratorRunning(GenMain) && !GeneratorRunning(GenStdby) && IsSet(InAir)) {
			GeneratorOn(GenEpu);
		}
		else if (GetEpuSwitch() == OFF) 
		{
			GeneratorOff(GenEpu);
			EpuClear();
		}
		else 
		{
			GeneratorOff(GenEpu);
			EpuClear();
		}
	}
	else 
	{
		GeneratorOff(GenEpu);
		EpuClear();
	}

//*****************************************************
	// #2
    // check hydraulics and generators 
    if (GeneratorRunning(GenMain) || GeneratorRunning(GenStdby)) {
		HydrRestore(HYDR_ALL); // restore those systems we can
    }
    else if (GeneratorRunning(GenEpu)) {
		HydrDown (HYDR_B_SYSTEM); // B system now dead
		HydrRestore(HYDR_A_SYSTEM); // A still OK
    }
    else {
		HydrDown(HYDR_ALL); // all off
    }

//********************************************************
	// #3
    // JPO - if the epu is running - well its running!
    // this may be independant of the engine if the pilot wants to check
    if (GeneratorRunning(GenEpu)) {
		EpuClear();
		// below 80%rpm, epu burns fuel.
		if (rpm < 0.80f) {
			EpuSetHydrazine();
			epuFuel -= 100.0f * dt / auxaeroData->epuBurnTime; // burn some hydrazine
			if (epuFuel <= 0.0f) {
				epuFuel = 0.0;
				GeneratorBreak(GenEpu); // well not broken, but done for.
				GeneratorOff(GenEpu);
			}
		}
		EpuSetAir(); // always true, means running really.
    }
	
//*****************************************************
	// #4
    // JPO: charge the JFS accumulators, up to 100% 
    if (rpm > auxaeroData->jfsMinRechargeRpm && jfsaccumulator < 100.0f /* 2002-04-11 ADDED BY S.G. If less than 0, don't recharge */ && jfsaccumulator >= 0.0f ) {
		jfsaccumulator += 100.0f * dt / auxaeroData->jfsRechargeTime;
		jfsaccumulator = min(jfsaccumulator, 100.0f);
    }

//*****************************************************
	// #5
    // transfer fuel
    FuelTransfer(dt);
	
#if 0 // not ready yet.JPO, but basically it should work as normal, but ignore the throttle.
    if (IsSet(ThrottleCheck)) {
		rpm      = oldRpm[0]; // just remember where we were... JPO
		if(throtl >= 1.5F)
			throtl = pwrlev;
		if (fabs (throtl - pwrlev) > 0.1F)
			ClearFlag(ThrottleCheck);
		pwrlev = throtl;
    }
    else
    {
		if (fabs (throtl - pwrlev) > 0.1F)
            ClearFlag (EngineOff);
    }
#endif
	
//*****************************************************
	//#6 Engine 1 left
    if (IsSet(EngineStopped))
	{
		ftit = Math.FLTust(0.0f, 20.0f, dt, oldFtit); // cool down the engine
		float modRpm = (float)(rpm*rpm*rpm*rpm*sqrt(rpm));
		
		// Am I increasing the rpm (but not yet in afterburner)?
		if (rpm > oldp01[0])
			Math.FLTust (modRpm, 4.0F, dt, oldp01);
		// Must be decreasing then...
		else
		{
			// Now check if the 'heat' is still above 1.00 (100%)
			if (oldp01[0] > 1.0F)
				Math.FLTust (modRpm, 7.0F, dt, oldp01);
			else
				Math.FLTust (modRpm, 2.0F, dt, oldp01);
		}
		
		
		spoolrate = auxaeroData->flameoutSpoolRate; // spool down rate
		fireTimer = 0.0f; // reset - engine is switched off
		// switch all to 0.
		tgross = 0.0F;
		fuelFlow = fuelFlowSS = 0.0F;
		thrtab = 0.0F;
		thrust = 0.0f; //Engine 1 Left

		
		// broken engine - anything but a flame out?
		if((platform->mFaults->GetFault(FaultClass::eng_fault) & ~FaultClass::fl_out) != 0) {
			rpmCmd = 0.0f; // engine must be seized, not going to start or windmill
		}
		//else if (IsSet (JfsStart))
		else if (IsSet (JfsStart) && UserStickInputs.getCurrentEngine() == UserStickInputs.Left_Engine)
		{
			rpmCmd = 0.25f; // JFS should take us up to 25%
			spoolrate = 15.0f;
			//decrease spin time
			JFSSpinTime -= SimLibMajorFrameTime;
			if(JFSSpinTime <= 0)
				ClearFlag(JfsStart);
		}
		else { // engine windmill (~12% at 450 knts) (me123 - this works on mine)
			rpmCmd = (platform->GetKias() / 450.0f) * 0.12f;
		}
		
		// attempt to spool up/down
		rpm = Math.FLTust (rpmCmd, spoolrate, dt, oldRpm);
    }

//*****************************************************
	//#7 More shut down conditions
    else if (!IsSet(EngineOff))
	
    {
		//7.1 Engine flame out, shut down
		if(platform->mFaults->GetFault(FaultClass::eng_fault) & FaultClass::fl_out) {
			SetFlag(EngineStopped);
			engine1Throttle = 0.0F;
		}

		//need to recombine these later
		pwrlevEngine1 = engine1Throttle;
		
		//7.2 Deep Stall Flameout
		if(auxaeroData->DeepStallEngineStall)
		{
			//me123 if in deep stall and in ab lets stall the engine
			// JPO - 10% chance, each time we check...
			if (stallMode >= DeepStall && pwrlevEngine1 >= 1.0F && rand() % 10 == 1)
			{ 
				SetFlag(EngineStopped);
				// mark it as a flame out
				platform->mFaults->SetFault(FaultClass::eng_fault, FaultClass::fl_out, FaultClass::fail, FALSE);
			}
		}
		
		//7.3 Flying
		if (platform->IsSetFlag(MOTION_OWNSHIP))
		{
			//7.3.1 Lost AB
			//TODO Make Engine 2 Faults
			if (platform->mFaults->GetFault(FaultClass::eng_fault) & FaultClass::a_b)
			{
				pwrlevEngine1 = min (engine1Throttle, 0.99F);
			}

			//7.3.2 Engine Fire
			if ((platform->mFaults->GetFault(FaultClass::eng_fault) & FaultClass::efire))
			{
				//Engine 1
				if (platform->mFaults->GetFault(FaultClass::eng_fault) & FaultClass::efire)
				{
					pwrlevEngine1 *= 0.5F;
					if (fireTimer >= 0.0F)
						fireTimer += max (pwrlevEngine1, 0.3F) * dt;
				}
				
				
				// On fire long enough, blow up
				if (fireTimer > 60.0F) // 60 seconds at mil power
				{
					fireTimer = -1.0F;
					FalconDamageMessage* message;
					message = new FalconDamageMessage (platform->Id(), FalconLocalGame );
					message->dataBlock.fEntityID  = platform->Id();
					
					message->dataBlock.fCampID = platform->GetCampaignObject()->GetCampID();
					message->dataBlock.fSide   = platform->GetCampaignObject()->GetOwner();
					message->dataBlock.fPilotID   = ((AircraftClass*)platform)->pilotSlot;
					message->dataBlock.fIndex     = platform->Type();
					message->dataBlock.fWeaponID  = platform->Type();
					message->dataBlock.fWeaponUID = platform->Id();
					message->dataBlock.dEntityID  = message->dataBlock.fEntityID;
					message->dataBlock.dCampID = message->dataBlock.fCampID;
					message->dataBlock.dSide   = message->dataBlock.fSide;
					message->dataBlock.dPilotID   = message->dataBlock.fPilotID;
					message->dataBlock.dIndex     = message->dataBlock.fIndex;
					message->dataBlock.damageType = FalconDamageType::CollisionDamage;
					message->dataBlock.damageStrength = 2.0F * platform->MaxStrength();
					message->dataBlock.damageRandomFact = 1.5F;
					
					message->RequestOutOfBandTransmit ();
					FalconSendMessage (message,TRUE);
				}
			}
		}

		//7.4 
		if (engineData->hasAB)
		{// JB 010706
			pwrlevEngine1 = max (min (pwrlevEngine1, 1.5F), 0.0F);
		}
		else
		{
			pwrlevEngine1 = max (min (pwrlevEngine1, 1.0F), 0.0F);
		}

		//7.5 Engine 1
		// Make sure engine is on
		if (rpm < 0.68f) { // below Idle
			rpmCmd = 0.7f;
			spoolrate = auxaeroData->lightupSpoolRate;
			thrtb1 = 0.0f;
			if (rpm > 0.5f)
				ClearFlag(JfsStart);
			ftit = Math.FLTust(5.1F * (rpm/0.7f), ftitrate, dt, oldFtit);

			// sfr: added, rampstart fix
			Engine1RpmMods(rpmCmd);
			rpmCmd = Engine1RpmMods(rpmCmd);
			spoolrate = spoolrate + spoolAltRate;
			rpm = Math.FLTust (rpmCmd, spoolrate, dt, oldRpm);
			if (fuel > 0.0F){
				rpm = max (rpm, 0.01F);
			}
		}
		//7.6 MIL Power Engine 1
		//if (pwrlevEngine1 <= 1.0f)
		else if ((pwrlevEngine1 <= 1.0f && rpm <= 1.0f) || (pwrlevEngine1 > 1.0f && rpm <= 1.0f))
		{
			/*-------------------*/
			/* Mil power or less */
			/*-------------------*/
			th1 = Math.TwodInterp (-z, mach, engineData->alt,
				engineData->mach, engineData->thrust[0], engineData->numAlt,
				engineData->numMach, &curEngAltBreak, &curEngMachBreak);
			th2 = Math.TwodInterp (-z, mach, engineData->alt,
				engineData->mach, engineData->thrust[1], engineData->numAlt,
				engineData->numMach, &curEngAltBreak, &curEngMachBreak);
			
			aburnLit = FALSE;
			// sfr: reverting back code
			thrtb1 = ((th2 - th1)*pwrlevEngine1 + th1) / mass; 
			//thrtb1 = (3.33f*(th2 - th1)*(rpm - 0.7f) + th1)/ mass; // saints code
			rpmCmd = 0.7F + 0.3F * pwrlevEngine1;
			// sfr: added per instructions
			// sfr: commenting out dup line
			//Engine1RpmMods(rpmCmd);
			rpmCmd = Engine1RpmMods(rpmCmd);
			//TJL 02/22/04 Add in the alt
			spoolrate = spoolAltRate + spoolrate;
			rpm = Math.FLTust (rpmCmd, spoolrate, dt, oldRpm);
			if (fuel > 0.0F){
				rpm = max (rpm, 0.01F);
			}
			// sfr: end added

			// ftit calculated
			if (rpm < 0.9F)
			{
				ftit = Math.FLTust(5.1F + (rpm - 0.7F) / 0.2F * 1.0F, ftitrate, dt, oldFtit);
			}
			else if (rpm < 1.0F)
			{
				ftit = Math.FLTust(6.1F + (rpm - 0.9F) / 0.1F * 1.5F, ftitrate, dt, oldFtit);
			}			
		}


		//7.7 AB Power Engine 1
		//if (pwrlevEngine1 > 1.0f)
		else
			/*--------------------------*/
			/* Some stage of afterburner */
			/*--------------------------*/
		{
			th1 = Math.TwodInterp (-z, mach, engineData->alt,
				engineData->mach, engineData->thrust[1], engineData->numAlt,
				engineData->numMach, &curEngAltBreak, &curEngMachBreak);
			th2 = Math.TwodInterp (-z, mach, engineData->alt,
				engineData->mach, engineData->thrust[2], engineData->numAlt,
				engineData->numMach, &curEngAltBreak, &curEngMachBreak);
			
			aburnLit = TRUE;
			// sfr: reverting back code
			thrtb1 = (2.0F*(th2 - th1)*(pwrlevEngine1-1.0F) + th1) / mass;
			//thrtb1 = (3.33F*(th2 - th1)*(rpm - 0.7F) + th1) / mass; // old saints code
			rpmCmd = 1.0F + 0.06F * (pwrlevEngine1 - 1.0F);
			// sfr: added per instructions
			// sfr: commenting out dup line
			//Engine1RpmMods(rpmCmd);
			rpmCmd = Engine1RpmMods(rpmCmd);
			//TJL 02/22/04 Add in the alt
			spoolrate = spoolAltRate + spoolrate;
			rpm = Math.FLTust (rpmCmd, spoolrate, dt, oldRpm);
			if (fuel > 0.0F){
				rpm = max (rpm, 0.01F);
			}
			// sfr: end added

			// ftit calculated
			ftit = Math.FLTust(7.6F + (rpm - 1.0F) / 0.03F * 0.1F, ftitrate, dt, oldFtit);
		}

		/*--------------------------------*/
		/* scale thrust to reference area */
		/*--------------------------------*/
		thrtab = thrtb1; //Engine 1

		//7.8
		/*-----------------*/
		/* engine dynamics */
		/*-----------------*/
		if(IsSet(Trimming))// || simpleMode == SIMPLE_MODE_AF)
		{
			//Engine 1
			ethrst = 1.0F;
			tgross = thrtab;
			olda01[0] = tgross;
			olda01[1] = tgross;
			olda01[2] = tgross;
			olda01[3] = tgross;
			
			rpm = rpmCmd;
			oldRpm[0] = rpm;
			oldRpm[1] = rpm;
			oldRpm[2] = rpm;
			oldRpm[3] = rpm;
		}

		//7.9
		else
		{
			tgross = thrtab;
			//TJL 08/21/04 this lag filter no longer needed.
			/*
			//Engine 1
			if(aburnLit)
			{
				//AB
				ta01 = auxaeroData->abSpoolRate + spoolAltRate;
				tgross = Math.FLTust(thrtab,ta01,dt,olda01);
			}
			else
			{
				//MIL
				if(pwrlevEngine1 <= 1.0F)
					ta01 = auxaeroData->normSpoolRate + spoolAltRate;
				else
					ta01 = auxaeroData->abSpoolRate + spoolAltRate;
				
				tgross = Math.FLTust(thrtab,ta01,dt,olda01);
			}*/
		}
		
		//7.10
		/*-----------*/
		/* burn fuel */
		/*-----------*/
		if (AvailableFuel() <= 0.0f || IsEngineFlag(MasterFuelOff)) { // no fuel - dead engine.
			SetFlag(EngineStopped);
			// mark it as a flame out
			platform->mFaults->SetFault(FaultClass::eng_fault, FaultClass::fl_out, FaultClass::fail, FALSE);
		}
		else
		{
			// JPO - back to basics... this stuff doesn't have to be complicated surely.
			// fuel flow is proportional to thrust.
			// thrust factor is already in, its just that tgross is thrust/mass, 
			// so we get rid of the mass component again.
			if(engineData->hasFuelFlow) // MLR 5/17/2004 - 
			{
				// sfr: new fuel code per instructions
				if(aburnLit == TRUE)
				{
					float fflow1, fflow2;

					fflow1 = Math.TwodInterp (-z, mach, engineData->alt,
						engineData->mach, engineData->fuelflow[1],engineData->numAlt,
						engineData->numMach, &curEngAltBreak, &curEngMachBreak);
					fflow2 = Math.TwodInterp (-z, mach, engineData->alt,
						engineData->mach, engineData->fuelflow[2],engineData->numAlt,
						engineData->numMach, &curEngAltBreak, &curEngMachBreak);
					
					
					//fuelFlowSS = (33.3333333F*(fflow2 - fflow1)*(rpm-1.0F) + fflow1);
					 fuelFlowSS = (2.0F * ( fflow2 - fflow1)* (pwrlev - 1.0F) + fflow1);
				}
				else
				{ 
					float fflow1, fflow2;

					fflow1 = Math.TwodInterp (-z, mach, engineData->alt,
						engineData->mach, engineData->fuelflow[0],engineData->numAlt,
						engineData->numMach, &curEngAltBreak, &curEngMachBreak);
					fflow2 = Math.TwodInterp (-z, mach, engineData->alt,
						engineData->mach, engineData->fuelflow[1],engineData->numAlt,
						engineData->numMach, &curEngAltBreak, &curEngMachBreak);

					//fuelFlowSS = ((fflow2 - fflow1)*(rpm-.7f)*3.33333f + fflow1);
					fuelFlowSS = (fflow2 - fflow1)* pwrlev + fflow1;
				}
				// end fuel

#if 0
				if(rpm>1.0)
				{
					float fflow1, fflow2;

					fflow1 = Math.TwodInterp (-z, mach, engineData->alt,
						engineData->mach, engineData->fuelflow[1],engineData->numAlt,
						engineData->numMach, &curEngAltBreak, &curEngMachBreak);
					fflow2 = Math.TwodInterp (-z, mach, engineData->alt,
						engineData->mach, engineData->fuelflow[2],engineData->numAlt,
						engineData->numMach, &curEngAltBreak, &curEngMachBreak);
					
					
					fuelFlowSS = (33.3333333F*(fflow2 - fflow1)*(rpm-1.0F) + fflow1);
				}
				else
				{ 
					float fflow1, fflow2;

					fflow1 = Math.TwodInterp (-z, mach, engineData->alt,
						engineData->mach, engineData->fuelflow[0],engineData->numAlt,
						engineData->numMach, &curEngAltBreak, &curEngMachBreak);
					fflow2 = Math.TwodInterp (-z, mach, engineData->alt,
						engineData->mach, engineData->fuelflow[1],engineData->numAlt,
						engineData->numMach, &curEngAltBreak, &curEngMachBreak);

					fuelFlowSS = ((fflow2 - fflow1)*(rpm-.7f)*3.33333f + fflow1);
				}
#endif
			}
			else
			{
				if (aburnLit)
				{
					fuelFlowSS =  auxaeroData->fuelFlowFactorAb * tgross  * mass;
				}
				else
				{ 
					fuelFlowSS = auxaeroData->fuelFlowFactorNormal * tgross * mass;
				}
			}

			fuelFlow += (fuelFlowSS - fuelFlow) / 10;

			//Determine MinFuelFlow Engine 1
			if (fuelFlow < auxaeroData->minFuelFlow)
				fuelFlow = auxaeroData->minFuelFlow;
			if (fuelFlowSS < auxaeroData->minFuelFlow)
				fuelFlowSS = auxaeroData->minFuelFlow;
			
		}
		/*
			//Total Fuel Burn
			fuelFlowTotal = fuelFlowSS + fuelFlowSS2;
			// Throw CBEFuelFlow the Total so the indicator works
			fuelFlow = fuelFlowTotal;
			*/

		if (!IsSet(NoFuelBurn))
		{
			// JPO - fuel is now burnt and transferred.
			//Using combined total between engines
			BurnFuel(fuelFlowSS * dt / 3600.0F);     
			weight -= fuelFlowSS * dt / 3600.0F;
			mass    = weight / GRAVITY;
		}
		
		//TJL 02/21/04 
		// sfr: commenting out these lines, dont inside ifelse
		/*
		Engine1RpmMods(rpmCmd);
		rpmCmd = Engine1RpmMods(rpmCmd);
		spoolrate = spoolrate + spoolAltRate;

		rpm = Math.FLTust (rpmCmd, spoolrate, dt, oldRpm);
		if (fuel > 0.0F)
			rpm = max (rpm, 0.01F);
			*/
		
		// ADDED BY S.G. TO SIMULATE THE HEAT PRODUCED BY THE ENGINE
		// I'M USING A PREVIOUSLY UNUSED ARRAY CALLED oldp01 FOR ENGINE HEAT TEMPERATURE
		// Afterburner lit?
		//Engine 1 (does this need to be here???)
		if (rpm > 100.0f){
			Math.FLTust (rpm + 0.5f, 0.5F, dt, oldp01);
		}
		else 
		{
			// the 'modified rpm will be rpm^4.5
			float modRpm = (float)(rpm*rpm*rpm*rpm*sqrt(rpm));
			
			// Am I increasing the rpm (but not yet in afterburner)?
			if (rpm > oldp01[0])
				Math.FLTust (modRpm, 4.0F, dt, oldp01);
			// Must be decreasing then...
			else {
				// Now check if the 'heat' is still above 1.00 (100%)
				if (oldp01[0] > 1.0F)
					Math.FLTust (modRpm, 7.0F, dt, oldp01);
				else
					Math.FLTust (modRpm, 2.0F, dt, oldp01);
				}
		}
		
		//There is code everywhere 
		if(stallMode >= EnteringDeepStall)
			thrust *= 0.1f;
		
   }
   else
   {
	   if (IsSet(ThrottleCheck))
	   {
		   //Engine 1
		   if(engine1Throttle >= 1.5F)
			   engine1Throttle = pwrlevEngine1;
		   if (fabs (engine1Throttle - pwrlevEngine1) > 0.1F)
			   ClearFlag(ThrottleCheck);
		   pwrlevEngine1 = engine1Throttle;

	   }
	   else
	   {
		   if ((fabs (engine1Throttle - pwrlevEngine1) > 0.1F))
		   {
			   ClearFlag (EngineOff);
		   }
	   }
	   
	   thrust   = 0.0F;
	   tgross   = 0.0F;
	   fuelFlow = 0.0F;
	   rpm      = oldRpm[0]; // just remember where we were... JPO
	   // Changed so the Oil light doesn't come on :-) - RH
	   ftit     = 5.85f;
   }
   
   
	//#6 Engine 2
	if (IsEngineFlag(EngineStopped2)) //new engine flag
	{
		ftit2 = Math.FLTust(0.0f, 20.0f, dt, oldFtit2); // cool down the engine
		float modRpm2 = (float)(rpm2*rpm2*rpm2*rpm2*sqrt(rpm2));
		
		// Am I increasing the rpm (but not yet in afterburner)?
		if (rpm2 > oldp01Eng2[0])
			Math.FLTust (modRpm2, 4.0F, dt, oldp01Eng2);
		// Must be decreasing then...
		else
		{
			// Now check if the 'heat' is still above 1.00 (100%)
			if (oldp01[0] > 1.0F)
				Math.FLTust (modRpm2, 7.0F, dt, oldp01Eng2);
			else
				Math.FLTust (modRpm2, 2.0F, dt, oldp01Eng2);
		}
		
		//Shut down
		spoolrate2 = auxaeroData->flameoutSpoolRate;
		fireTimer = 0.0f;
		tgross2 = 0.0F;
		fuelFlow2 = fuelFlowSS2 = 0.0F;
		thrtab2 = 0.0F;
		thrust2 = 0.0F;

		
		// broken engine - anything but a flame out?
		//TODO Chase down faults to fault Engine 2
		if((platform->mFaults->GetFault(FaultClass::eng_fault) & ~FaultClass::fl_out) != 0) {
			rpmCmd2 = 0.0f; // engine must be seized, not going to start or windmill
		}
		//else if (IsSet (JfsStart)) 
		else if (IsSet (JfsStart) && UserStickInputs.getCurrentEngine() == UserStickInputs.Right_Engine)
		{
			rpmCmd2 = 0.25f; // JFS should take us up to 25%
			spoolrate2 = 15.0f;
			//decrease spin time
			JFSSpinTime -= SimLibMajorFrameTime;
			if(JFSSpinTime <= 0)
				ClearFlag(JfsStart);
		}
		else { // engine windmill (~12% at 450 knts) (me123 - this works on mine)
			rpmCmd2 = (platform->GetKias() / 450.0f) * 0.12f;
		}
		
		// attempt to spool up/down
		rpm2 = Math.FLTust (rpmCmd2, spoolrate2, dt, oldRpm2);
    }

	//end
/******************************************
// Engine 2
//
*/
//*****************************************************
	//#7 More shut down conditions
    else if (!IsSet(EngineOff2))
	
    {
		//7.1 Engine flame out, shut down
		if (platform->mFaults->GetFault(FaultClass::eng2_fault) & FaultClass::fl_out)
		{
			SetEngineFlag(EngineStopped2);
			engine2Throttle = 0.0F;
		}

		//need to recombine these later
		pwrlevEngine2 = engine2Throttle;
		
		//7.2 Deep Stall Flameout
		if(auxaeroData->DeepStallEngineStall)
		{
			//me123 if in deep stall and in ab lets stall the engine
			// JPO - 10% chance, each time we check...
			if (stallMode >= DeepStall && pwrlevEngine2 >= 1.0F && rand() % 10 == 1)
			{ 
				SetEngineFlag(EngineStopped2);
				// mark it as a flame out
				platform->mFaults->SetFault(FaultClass::eng2_fault, FaultClass::fl_out, FaultClass::fail, FALSE);
			}
		}
		
		//7.3 Flying
		if (platform->IsSetFlag(MOTION_OWNSHIP))
		{
			//7.3.1 Lost AB
			//TODO Make Engine 2 Faults
			if (platform->mFaults->GetFault(FaultClass::eng2_fault) & FaultClass::a_b)
			{
				pwrlevEngine2 = min (engine2Throttle, 0.99F);
			}

			//7.3.2 Engine Fire
			if (platform->mFaults->GetFault(FaultClass::eng2_fault) & FaultClass::efire)
			{
				//Engine 2
				if (platform->mFaults->GetFault(FaultClass::eng2_fault) & FaultClass::efire)
				{
					pwrlevEngine2 *= 0.5F;
					if (fireTimer >= 0.0F)
						fireTimer += max (pwrlevEngine2, 0.3F) * dt;
				}
				
				// On fire long enough, blow up
				if (fireTimer > 60.0F) // 60 seconds at mil power
				{
					fireTimer = -1.0F;
					FalconDamageMessage* message;
					message = new FalconDamageMessage (platform->Id(), FalconLocalGame );
					message->dataBlock.fEntityID  = platform->Id();
					
					message->dataBlock.fCampID = platform->GetCampaignObject()->GetCampID();
					message->dataBlock.fSide   = platform->GetCampaignObject()->GetOwner();
					message->dataBlock.fPilotID   = ((AircraftClass*)platform)->pilotSlot;
					message->dataBlock.fIndex     = platform->Type();
					message->dataBlock.fWeaponID  = platform->Type();
					message->dataBlock.fWeaponUID = platform->Id();
					message->dataBlock.dEntityID  = message->dataBlock.fEntityID;
					message->dataBlock.dCampID = message->dataBlock.fCampID;
					message->dataBlock.dSide   = message->dataBlock.fSide;
					message->dataBlock.dPilotID   = message->dataBlock.fPilotID;
					message->dataBlock.dIndex     = message->dataBlock.fIndex;
					message->dataBlock.damageType = FalconDamageType::CollisionDamage;
					message->dataBlock.damageStrength = 2.0F * platform->MaxStrength();
					message->dataBlock.damageRandomFact = 1.5F;
					
					message->RequestOutOfBandTransmit ();
					FalconSendMessage (message,TRUE);
				}
			}
		}

		//7.4 
		if (engineData->hasAB)
		{// JB 010706
			pwrlevEngine2 = max (min (pwrlevEngine2, 1.5F), 0.0F);
		}
		else
		{
			pwrlevEngine2 = max (min (pwrlevEngine2, 1.0F), 0.0F);
		}

		//7.5 Engine 2
		if (rpm2 < 0.68f) { // below Idle
			rpmCmd2 = 0.7f;
			spoolrate2 = auxaeroData->lightupSpoolRate;
			thrtb2 = 0.0f;
			if (rpm2 > 0.5f)
				ClearFlag(JfsStart);
			ftit2 = Math.FLTust(5.1F * (rpm2/0.7f), ftitrate, dt, oldFtit2);
			// sfr: added rampstart
			rpmCmd2 = Engine2RpmMods(rpmCmd2);
			spoolrate2 = spoolrate2 + spoolAltRate;
			rpm2 = Math.FLTust (rpmCmd2, spoolrate2, dt, oldRpm2);
			if (fuel > 0.0F){
				rpm2 = max (rpm2, 0.01F);
			}
		}

		//7.6 MIL Power Engine 2
		//if (pwrlevEngine2 <= 1.0f)
		else if ((pwrlevEngine2 <= 1.0f && rpm2 <= 1.0f) || (pwrlevEngine2 > 1.0f && rpm2 <= 1.0f))
		{
			/*-------------------*/
			/* Mil power or less */
			/*-------------------*/
			th12 = Math.TwodInterp (-z, mach, engineData->alt,
				engineData->mach, engineData->thrust[0], engineData->numAlt,
				engineData->numMach, &curEngAltBreak, &curEngMachBreak);
			th22 = Math.TwodInterp (-z, mach, engineData->alt,
				engineData->mach, engineData->thrust[1], engineData->numAlt,
				engineData->numMach, &curEngAltBreak, &curEngMachBreak);
			
			aburnLit2 = FALSE;
			// sfr: reverting back code
			thrtb2 = ((th22 - th12)*pwrlevEngine2 + th12) / mass;
			//thrtb2 = (3.33f*(th22 - th12)*(rpm2 - 0.7f) + th12)/ mass; // old saints code
			rpmCmd2 = 0.7F + 0.3F * pwrlevEngine2;
			// sfr: added per instructions
			// sfr: commenting dup line
			//Engine2RpmMods(rpmCmd2);
			rpmCmd2 = Engine2RpmMods(rpmCmd2);
			//TJL 02/22/04 Add in the alt
			spoolrate2 = spoolAltRate + spoolrate2;
			rpm2 = Math.FLTust (rpmCmd2, spoolrate2, dt, oldRpm2);
			if (fuel > 0.0F){
				rpm2 = max (rpm2, 0.01F);
			}
			// sfr: end added

			// ftit calculated
			if (rpm2 < 0.9F)
			{
				ftit2 = Math.FLTust(5.1F + (rpm2 - 0.7F) / 0.2F * 1.0F, ftitrate, dt, oldFtit2);
			}
			else if (rpm2 < 1.0F)
			{
				ftit2 = Math.FLTust(6.1F + (rpm2 - 0.9F) / 0.1F * 1.5F, ftitrate, dt, oldFtit2);
			}
		}

		//7.7 AB Power Engine 2
		//if (pwrlevEngine2 > 1.0f)
		else
			/*--------------------------*/
			/* Some stage of afterburner */
			/*--------------------------*/
		{
			th12 = Math.TwodInterp (-z, mach, engineData->alt,
				engineData->mach, engineData->thrust[1], engineData->numAlt,
				engineData->numMach, &curEngAltBreak, &curEngMachBreak);
			th22 = Math.TwodInterp (-z, mach, engineData->alt,
				engineData->mach, engineData->thrust[2], engineData->numAlt,
				engineData->numMach, &curEngAltBreak, &curEngMachBreak);
			
			aburnLit2 = TRUE;
			// sfr: reverting back code
			thrtb2 = (2.0F*(th22 - th12)*(pwrlevEngine2-1.0F) + th12) / mass;
			//thrtb2 = (3.33F*(th22 - th12)*(rpm2 - 0.7F) + th12) / mass; // old saints code
			rpmCmd2 = 1.0F + 0.06F * (pwrlevEngine2 - 1.0F);
			// sfr: added per instructions
			// sfr: removing dup line
			//Engine2RpmMods(rpmCmd2);
			rpmCmd2 = Engine2RpmMods(rpmCmd);
			//TJL 02/22/04 Add in the alt
			spoolrate2 = spoolAltRate + spoolrate2;
			rpm2 = Math.FLTust (rpmCmd2, spoolrate2, dt, oldRpm2);
			if (fuel > 0.0F){
				rpm2 = max (rpm2, 0.01F);
			}
			// sfr: end added

			// ftit calculated
			ftit2 = Math.FLTust(7.6F + (rpm2 - 1.0F) / 0.03F * 0.1F, ftitrate, dt, oldFtit2);
		}

		/*--------------------------------*/
		/* scale thrust to reference area */
		/*--------------------------------*/
		thrtab2 = thrtb2;//Engine 2

		//7.8
		/*-----------------*/
		/* engine dynamics */
		/*-----------------*/
		if(IsSet(Trimming))// || simpleMode == SIMPLE_MODE_AF)
		{
			//Engine 2
			ethrst2 = 1.0F;
			tgross2 = thrtab2;
			olda012[0] = tgross2;
			olda012[1] = tgross2;
			olda012[2] = tgross2;
			olda012[3] = tgross2;
			
			rpm2 = rpmCmd2;
			oldRpm2[0] = rpm2;
			oldRpm2[1] = rpm2;
			oldRpm2[2] = rpm2;
			oldRpm2[3] = rpm2;
		}

		//7.9
		else
		{
			tgross2 = thrtab2;
			//TJL 08/21/04 This lag filter no longer needed
			/*
			//Engine 2
			if(aburnLit2)
			{
				//AB
				ta02 = auxaeroData->abSpoolRate + spoolAltRate;
				tgross2 = Math.FLTust(thrtab2,ta02,dt,olda012);
			}
			else
			{
				//MIL
				if(pwrlevEngine2 <= 1.0F)
					ta02 = auxaeroData->normSpoolRate + spoolAltRate;
				else
					ta02 = auxaeroData->abSpoolRate + spoolAltRate;
				
				tgross2 = Math.FLTust(thrtab2,ta02,dt,olda012);
			}*/

		}
		
		//7.10
		/*-----------*/
		/* burn fuel */
		/*-----------*/
		if (AvailableFuel() <= 0.0f || IsEngineFlag(MasterFuelOff)) { // no fuel - dead engine.
			SetEngineFlag(EngineStopped2);
			// mark it as a flame out
			platform->mFaults->SetFault(FaultClass::eng2_fault, FaultClass::fl_out, FaultClass::fail, FALSE);
		}
		else
		{
			// JPO - back to basics... this stuff doesn't have to be complicated surely.
			// fuel flow is proportional to thrust.
			// thrust factor is already in, its just that tgross is thrust/mass, 
			// so we get rid of the mass component again.
			if(engineData->hasFuelFlow) // MLR 5/17/2004 - 
			{
				// sfr: new fuel code per instructions
				if(aburnLit2 == TRUE)
				{
					float fflow1, fflow2;

					fflow1 = Math.TwodInterp (-z, mach, engineData->alt,
						engineData->mach, engineData->fuelflow[1],engineData->numAlt,
						engineData->numMach, &curEngAltBreak, &curEngMachBreak);
					fflow2 = Math.TwodInterp (-z, mach, engineData->alt,
						engineData->mach, engineData->fuelflow[2],engineData->numAlt,
						engineData->numMach, &curEngAltBreak, &curEngMachBreak);
					
					
					//fuelFlowSS = (33.3333333F*(fflow2 - fflow1)*(rpm-1.0F) + fflow1);
					 fuelFlowSS = (2.0F * ( fflow2 - fflow1)* (pwrlev - 1.0F) + fflow1);
				}
				else
				{ 
					float fflow1, fflow2;

					fflow1 = Math.TwodInterp (-z, mach, engineData->alt,
						engineData->mach, engineData->fuelflow[0],engineData->numAlt,
						engineData->numMach, &curEngAltBreak, &curEngMachBreak);
					fflow2 = Math.TwodInterp (-z, mach, engineData->alt,
						engineData->mach, engineData->fuelflow[1],engineData->numAlt,
						engineData->numMach, &curEngAltBreak, &curEngMachBreak);

					//fuelFlowSS = ((fflow2 - fflow1)*(rpm-.7f)*3.33333f + fflow1);
					fuelFlowSS = (fflow2 - fflow1)* pwrlev + fflow1;
				}
				// end fuel

#if 0
				if(rpm2>1.0)
				{
					float fflow1, fflow2;

					fflow1 = Math.TwodInterp (-z, mach, engineData->alt,
						engineData->mach, engineData->fuelflow[1],engineData->numAlt,
						engineData->numMach, &curEngAltBreak, &curEngMachBreak);
					fflow2 = Math.TwodInterp (-z, mach, engineData->alt,
						engineData->mach, engineData->fuelflow[2],engineData->numAlt,
						engineData->numMach, &curEngAltBreak, &curEngMachBreak);
					
					
					fuelFlowSS2 = (33.3333333F*(fflow2 - fflow1)*(rpm2-1.0F) + fflow1);
				}
				else
				{ 
					float fflow1, fflow2;

					fflow1 = Math.TwodInterp (-z, mach, engineData->alt,
						engineData->mach, engineData->fuelflow[0],engineData->numAlt,
						engineData->numMach, &curEngAltBreak, &curEngMachBreak);
					fflow2 = Math.TwodInterp (-z, mach, engineData->alt,
						engineData->mach, engineData->fuelflow[1],engineData->numAlt,
						engineData->numMach, &curEngAltBreak, &curEngMachBreak);

					fuelFlowSS2 = ((fflow2 - fflow1)*(rpm2-.7f)*3.33333f + fflow1);
				}
#endif
			}
			else
			{
				if (aburnLit2)
				{
					fuelFlowSS2 =  auxaeroData->fuelFlowFactorAb * tgross2  * mass;
				}
				else
				{ 
					fuelFlowSS2 = auxaeroData->fuelFlowFactorNormal * tgross2 * mass;
				}
			}

			fuelFlow2 += (fuelFlowSS2 -fuelFlow2)/10;

			//Engine 2
			if (fuelFlow2 < auxaeroData->minFuelFlow)
				fuelFlow2 = auxaeroData->minFuelFlow;
			if (fuelFlowSS2 < auxaeroData->minFuelFlow)
				fuelFlowSS2 = auxaeroData->minFuelFlow;
/*
			//Total Fuel Burn
			fuelFlowTotal = fuelFlowSS + fuelFlowSS2;
			// Throw CBEFuelFlow the Total so the indicator works
			fuelFlow = fuelFlowTotal;
			*/

			if (!IsSet(NoFuelBurn))
			{
				// JPO - fuel is now burnt and transferred.
				//Using combined total between engines
				BurnFuel(fuelFlowSS2 * dt / 3600.0F);     
				weight -= fuelFlowSS2 * dt / 3600.0F;
				mass    = weight / GRAVITY;
			}

		}


		//TJL 02/21/04 
		// sfr: commenting out, done inside if else
		/*Engine2RpmMods(rpmCmd2);
		rpmCmd2 = Engine2RpmMods(rpmCmd2);
		spoolrate2 = spoolrate2 + spoolAltRate;

		rpm2 = Math.FLTust (rpmCmd2, spoolrate2, dt, oldRpm2);
		if (fuel > 0.0F)
			rpm2 = max (rpm2, 0.01F);
			*/
		
		// ADDED BY S.G. TO SIMULATE THE HEAT PRODUCED BY THE ENGINE
		// I'M USING A PREVIOUSLY UNUSED ARRAY CALLED oldp01 FOR ENGINE HEAT TEMPERATURE
		// Afterburner lit?

		//Engine 2
		if (rpm2 > 100.0f)
			Math.FLTust (rpm2 + 0.5f, 0.5F, dt, oldp01Eng2);
		else {
			// the 'modified rpm will be rpm^4.5
			float modRpm2 = (float)(rpm2*rpm2*rpm2*rpm2*sqrt(rpm2));
			
			// Am I increasing the rpm (but not yet in afterburner)?
			if (rpm2 > oldp01Eng2[0])
				Math.FLTust (modRpm2, 4.0F, dt, oldp01Eng2);
			// Must be decreasing then...
			else {
				// Now check if the 'heat' is still above 1.00 (100%)
				if (oldp01Eng2[0] > 1.0F)
					Math.FLTust (modRpm2, 7.0F, dt, oldp01Eng2);
				else
					Math.FLTust (modRpm2, 2.0F, dt, oldp01Eng2);
			}

		}

		//Total Thrust
		//thrust = ((tgross * ethrst) + (tgross2 * ethrst2));
		
		//There is code everywhere 
		if(stallMode >= EnteringDeepStall)
			thrust *= 0.1f;
		
   }
   else
   {
	   if (IsSet(ThrottleCheck))
	   {
			//Engine 2
		   if(engine2Throttle >= 1.5F)
			   engine2Throttle = pwrlevEngine2;
		   if (fabs (engine2Throttle - pwrlevEngine2) > 0.1F)
			   ClearFlag(ThrottleCheck);
		   pwrlevEngine2 = engine2Throttle;

	   }
	   else
	   {
		   if (fabs (engine2Throttle - pwrlevEngine2) > 0.1F)
		   {
			   ClearFlag (EngineOff2);
		   }
	   }
	   
	   thrust   = 0.0F;
	   tgross2	= 0.0F;
	   fuelFlow2= 0.0F;
	   rpm2		= oldRpm2[0];
	   ftit2	= 5.85f;
   }


// END 
//****************************************

//Merge Engine 1 and Engine 2 Data
   //Total Thrust
   //Clamp Thrust when below idle
   if (rpm <= 0.7f)
		thrust1 = 0.01f;
   else 
		thrust1 = (tgross * ethrst);

   if (rpm2 <= 0.7f)
		thrust2 = 0.01f;
   else
		thrust2 = (tgross2 * ethrst2);
   
   //combined
	thrust = ( thrust1 + thrust2 );
	//Cobra Thrust Reverse
	if (auxaeroData->hasThrRev){
		if (platform->OnGround() && thrustReverse == 2){
			thrust = (-thrust * 0.40f);
		}
		//Cobra Thrust Reverse
		static int doOnce = 0;
		if (platform->IsPlayer() && thrustReverse == 2 && doOnce == 0){
			OTWDriver.ToggleThrustReverseDisplay();
			doOnce = 1;
		}
		else if (platform->IsPlayer() && thrustReverse == 0 && doOnce == 1){
			OTWDriver.ToggleThrustReverseDisplay();
			doOnce = 0;
		}
	}

	//Total Fuel Burn
	fuelFlowTotal = fuelFlowSS + fuelFlowSS2;
	// Throw CBEFuelFlow the Total so the cockpit indicator works
	fuelFlow = fuelFlowTotal;

	//TJL 09/11/04 
	//TODO QC this
	CalcFtit(ftitLeft, ftitRight);


   // turn on stdby generator
   if (rpm > auxaeroData->stbyGenRpm && platform->MainPowerOn()) {
       GeneratorOn(GenStdby);
   }
   else
	   GeneratorOff(GenStdby);
   
   // tunr on main generator
   if (rpm > auxaeroData->mainGenRpm && platform->MainPowerOn()) {
       GeneratorOn(GenMain);
   }
   else
	   GeneratorOff(GenMain);
   
   if (IsSet(OnObject) && vcas < 175) // JB carrier
	   // RV - Biker - Use catapult thrust multiplier from FMs
	   //thrust *= 4;
	   thrust *= GetCatapultThrustMultiplier();
  
   /*------------------*/
   /* body axis accels */
   /*------------------*/
   if (nozzlePos == 0) { // normal case JPO
	   xprop =  thrust;
	   yprop =  0.0F;
	   zprop =  0.0F;
	   /*-----------------------*/
	   /* stability axis accels */
	   /*-----------------------*/
	   xsprop =  xprop *platform->platformAngles->cosalp;
	   ysprop =  yprop;
	   //   zsprop = 0.0F;		//assume flcs cancels this out? (makes life easier)
	   //   zsprop = -thrust*platform->platformAngles->sinalp * 0.001F; //why the 0.001F ?
	   //	zsprop = -thrust*platform->platformAngles->sinalp; // JPO previous
	   zsprop = - xprop * platform->platformAngles->sinalp;
   }
   else { // harrier fake stuff - doesn't really work.
	   mlTrig noz;
	   mlSinCos(&noz, nozzlePos);
	   xprop = thrust * noz.cos;
	   yprop =  0.0F;
	   zprop = -thrust * noz.sin;
	   /*-----------------------*/
	   /* stability axis accels */
	   /*-----------------------*/
	   xsprop =  xprop *platform->platformAngles->cosalp;
	   ysprop =  yprop;
	   //   zsprop = 0.0F;		//assume flcs cancels this out? (makes life easier)
	   //   zsprop = -thrust*platform->platformAngles->sinalp * 0.001F; //why the 0.001F ?
	   //	zsprop = -thrust*platform->platformAngles->sinalp; // JPO previous
	   zsprop = - xprop * platform->platformAngles->sinalp +
		   zprop * platform->platformAngles->cosalp;
	   
   }
   
   ShiAssert(!_isnan(platform->platformAngles->cosalp));
   ShiAssert(!_isnan(platform->platformAngles->sinalp));
   
   /*------------------*/
   /* wind axis accels */
   /*------------------*/
   xwprop =  xsprop*platform->platformAngles->cosbet +
	   ysprop*platform->platformAngles->sinbet;
   ywprop = -xsprop*platform->platformAngles->sinbet +
	   ysprop*platform->platformAngles->cosbet;
   zwprop =  zsprop;
}

//End Multi-Engine Code


// JPO new support routines for hydraulics
void AirframeClass::HydrBreak(int sys)
{
    if (sys & HYDR_A_SYSTEM) { // mark A system as down and broke
		hydrAB &= ~ HYDR_A_SYSTEM;
		hydrAB |= HYDR_A_BROKE;
    }
    if (sys & HYDR_B_SYSTEM) { // mark A system as down and broke
		hydrAB &= ~ HYDR_B_SYSTEM;
		hydrAB |= HYDR_B_BROKE;
    }
}

void AirframeClass::HydrRestore(int sys) 
{
    // restore A if not broke
		if ((sys & HYDR_A_SYSTEM) && (hydrAB & HYDR_A_BROKE) == 0) {
	hydrAB |= HYDR_A_SYSTEM;
    }
    // restore B if not broke
    if ((sys & HYDR_B_SYSTEM) && (hydrAB & HYDR_B_BROKE) == 0) {
		hydrAB |= HYDR_B_SYSTEM;
    }
}

void AirframeClass::StepEpuSwitch()
{
    switch (epuState) {
    case OFF:
	epuState = AUTO;
	break;
    case AUTO:
	epuState = ON;
	break;
    case ON:
	epuState = OFF;
	break;
    }
}

void AirframeClass::JfsEngineStart ()
{
    if (jfsaccumulator < 90.0f) { // not charged
	return;
    }
    //F4SoundFXSetPos( SFX_VULEND, 0, x, y, z, 1.0f );
	platform->SoundPos.Sfx(SFX_VULEND); // MLR 5/16/2004 - 
    jfsaccumulator = 0.0f; // all used up
    if (fuel <= 0.0f) return; // nothing to run JFS off.
    
    // attempting JFS start - only works below 400kias and 20,000ft
    if (platform->GetKias() > 400.0f) {
	if (platform->GetKias() - 400.0f > rand()%100) { // one shot failed
	    return;
	}
    }
    if (-z > 20000.0f) { 
	if ( (-z) - 20000.0f > rand() % 5000) {
	    return;
	}
    }
    SetFlag(AirframeClass::JfsStart);
	//MI add in JFS spin time
	JFSSpinTime = 240;	//4 minutes available


}

// JPO start the engine quickly - for deaggregation purposes.
void AirframeClass::QuickEngineStart ()
{
    ClearFlag (EngineStopped);
	
    //ClearFlag(JfsStart); TJL 08/15/04 
    //GeneratorOn(GenMain);
    //GeneratorOn(GenStdby);
	//GeneratorOff(GenEpu);  // MD -- 20040531: don't forget to ensure the EPU isn't running
    rpm = 0.75f;
    oldRpm[0] = rpm;
    oldRpm[1] = rpm;
    oldRpm[2] = rpm;
    oldRpm[3] = rpm;
    HydrRestore(HYDR_ALL);

	//TJL 01/11/04 Multi-Engine Quick Start
	if (auxaeroData->nEngines == 2)
	{		
		ClearEngineFlag(AirframeClass::EngineStopped2);
		rpm2 = 0.75f;
		oldRpm2[0] = rpm2;
		oldRpm2[1] = rpm2;
		oldRpm2[2] = rpm2;
		oldRpm2[3] = rpm2;
	}
	ClearFlag(JfsStart); //TJL 08/15/04 Moved here
    GeneratorOn(GenMain);
    GeneratorOn(GenStdby);
	GeneratorOff(GenEpu);
}

// fuel management stuff
// all fairly F16 specific currently.
// Tank F2 is ignored - assumed to be part of F1.

// clear down fuel tanks.
void AirframeClass::ClearFuel ()
{
    for (int i = 0; i < MAX_FUEL; i++)
	m_tanks[i] = 0.0f;
}

// split a fuel load across the tanks.
// allocate in priority order.
void AirframeClass::AllocateFuel (float totalfuel)
{
		totalfuel = max(totalfuel, 0.0f);

    ClearFuel();
    m_tanks[TANK_AFTRES] = min(totalfuel/2.0f, m_tankcap[TANK_AFTRES]);
    m_tanks[TANK_FWDRES] = min(totalfuel/2.0f, m_tankcap[TANK_FWDRES]);
    totalfuel -= m_tanks[TANK_AFTRES] + m_tanks[TANK_FWDRES];

    m_tanks[TANK_A1] = min(totalfuel/2.0f, m_tankcap[TANK_A1]);
    m_tanks[TANK_F1] = min(totalfuel/2.0f, m_tankcap[TANK_F1]);
    totalfuel -= m_tanks[TANK_F1] + m_tanks[TANK_A1];

    m_tanks[TANK_WINGFR] = min(totalfuel/2.0f, m_tankcap[TANK_WINGFR]);
    m_tanks[TANK_WINGAL] = min(totalfuel/2.0f, m_tankcap[TANK_WINGAL]);
    totalfuel -= m_tanks[TANK_WINGFR] + m_tanks[TANK_WINGAL];

    // fill drop tanks now.
    m_tanks[TANK_LEXT] = min(m_tankcap[TANK_LEXT], totalfuel/2.0f);
    m_tanks[TANK_REXT] = min(m_tankcap[TANK_REXT], totalfuel/2.0f);
    totalfuel -= m_tanks[TANK_REXT];
    totalfuel -= m_tanks[TANK_LEXT];
    m_tanks[TANK_CLINE] = min(m_tankcap[TANK_CLINE], totalfuel);
    totalfuel -= m_tanks[TANK_CLINE];
    if (totalfuel > 1.0)
		{
			totalfuel = totalfuel; // JB 010506 Do something for release build
			//ShiWarning("Too much fuel for the plane");
		}
    // recompute internal/external ammounts
    RecalculateFuel ();
}

// what fuel do we have available (resevoirs only)
float AirframeClass::AvailableFuel()
{
    if (!g_bRealisticAvionics) 
	return externalFuel + fuel;
    else if (IsEngineFlag(MasterFuelOff))
	return 0.0f;
    else if (fuelPump == FP_OFF && nzcgb < -0.5f) // -ve G
	return 0.0f;
    switch (fuelPump) {
    case FP_FWD:
	return m_tanks[TANK_FWDRES];
    case FP_AFT:
	return m_tanks[TANK_AFTRES];
    default:
	return m_tanks[TANK_AFTRES] + m_tanks[TANK_FWDRES];
    }
}

// burn fuel from the two possible resevoirs
int AirframeClass::BurnFuel (float bfuel)
{
    FuelPump tfp = fuelPump;
    if (tfp == FP_NORM)  { // deal with empty tanks
	if (m_tanks[TANK_AFTRES] <= 0.0f)
	    tfp = FP_FWD;
	else if (m_tanks[TANK_FWDRES] <= 0.0f)
	    tfp = FP_AFT;
    }

    // TODO: broken FFP if hydrB offline ... erratic transfer rates.
    // TODO C of G calculations
    switch (tfp) {
    case FP_OFF: // XXX or should no flow occur?
    case FP_NORM:
	m_tanks[TANK_AFTRES] -= bfuel/2.0f;
	m_tanks[TANK_FWDRES] -= bfuel/2.0f;
	break;
    case FP_FWD:
	if (m_tanks[TANK_FWDRES] <= 0.0f)
	    return 0;
	m_tanks[TANK_FWDRES] -= bfuel;
	break;
    case FP_AFT:
	if (m_tanks[TANK_AFTRES] <= 0.0f)
	    return 0;
	m_tanks[TANK_AFTRES] -= bfuel;
	break;
    }
    return 1;
}

// cross feed tank 1 from tank 2
void AirframeClass::FeedTank(int t1, int t2, float dt)
{
    // room for some more?
    float delta = m_tankcap[t1] - m_tanks[t1];
    delta = min(delta, m_tanks[t2]); // limit to amount in tank2

    float maxtrans = m_trate[t2] * dt; // limit to max trans rate
    delta = min(delta, maxtrans);

    if (delta > 0) { // transfer
	m_tanks[t1] += delta;
	m_tanks[t2] -= delta;
    }
    if (m_tanks[t2] < 0.0f) // sanity check
	m_tanks[t2] = 0.0f;
}

// do the overall tank transfers.
void AirframeClass::FuelTransfer (float dt)
{
    FeedTank(TANK_FWDRES, TANK_F1, dt);
    FeedTank(TANK_F1, TANK_WINGFR, dt);
	
    FeedTank(TANK_AFTRES, TANK_A1, dt);
    FeedTank(TANK_A1, TANK_WINGAL, dt);
	
    // transfer wing externals if
    // switch set, or cline empty and
    // we have some fuel to transfer
    // only happens if externals are pressurized.
    if (airSource == AS_NORM || airSource == AS_DUMP) {
		if (((engineFlags & WingFirst) ||
			m_tanks[TANK_CLINE] <= 0.0f) &&
			(m_tanks[TANK_REXT] > 0.0f ||
			m_tanks[TANK_LEXT] > 0.0f)) {
			FeedTank(TANK_WINGFR, TANK_REXT, dt);
			FeedTank(TANK_WINGAL, TANK_LEXT, dt);
		}
		else if (m_tanks[TANK_CLINE] > 0.0f) {
			FeedTank(TANK_WINGFR, TANK_CLINE, dt/2.0f);
			FeedTank(TANK_WINGAL, TANK_CLINE, dt/2.0f);
		}
    }
    if (!platform->isDigital) {
		if (m_tanks[TANK_FWDRES] < auxaeroData->fuelMinFwd)
		{
			if(!g_bRealisticAvionics)
				platform->mFaults->SetFault(fwd_fuel_low_fault);
			else
				platform->mFaults->SetCaution(fwd_fuel_low_fault);
		}
		else if (fuelSwitch != FS_TEST && platform->mFaults->GetFault(fwd_fuel_low_fault))
			platform->mFaults->ClearFault(fwd_fuel_low_fault);
		if (m_tanks[TANK_AFTRES] < auxaeroData->fuelMinAft)
		{
			if(!g_bRealisticAvionics)
				//platform->mFaults->SetFault(fwd_fuel_low_fault);	//MI should probably be AFT tank
				platform->mFaults->SetFault(aft_fuel_low_fault);
			else
				platform->mFaults->SetCaution(aft_fuel_low_fault);
		}
		else if (fuelSwitch != FS_TEST && platform->mFaults->GetFault(aft_fuel_low_fault))
			platform->mFaults->ClearFault(aft_fuel_low_fault);
    }
    // recompute internal/external ammounts
    RecalculateFuel ();

	// MD -- 20040531: adding a check to see if an external tank ran dry.  When they do run dry
	// there's a chance that the CAT limiter needs adjusting.  For now this only operates on the
	// centerline tank but it may need to take dollies into account as well at some point.

	if ((m_tankcap[TANK_CLINE] > 0.0F) && (m_tanks[TANK_CLINE] <= 0.0F))
		platform->Sms->ChooseLimiterMode(1);
}

// loose a tank.
void AirframeClass::DropTank (int n)
{
    ShiAssert(n>=0 && n < MAX_FUEL);
    m_tanks[n] = 0.0f;
    m_tankcap[n] = 0.0f;
    float fuelBefore = externalFuel;
    RecalculateFuel();
    float fuelDropped = fuelBefore - externalFuel;
    weight -= fuelDropped;
    mass    = weight / GRAVITY;
}

// recalculate the quick access.
void AirframeClass::RecalculateFuel ()
{
	//MI fix for refueling not filling up tanks
	if(IsSet(AirframeClass::Refueling))
		return;
    // recompute internal/external ammounts
    fuel = 0.0f;
    for (int i = 0; i <= TANK_MAXINTERNAL; i++)
	fuel += m_tanks[i];
    externalFuel = 0.0f;
    for (i = TANK_MAXINTERNAL+1; i < MAX_FUEL; i++)
	externalFuel += m_tanks[i];
}

// fuel dial stuff
void AirframeClass::GetFuel(float *fwdp, float *aftp, float *total)
{
    if (!g_bRealisticAvionics) {
	*fwdp = fuel;
	*aftp = externalFuel;
	*total = fuel + externalFuel;
	return;
    }
    else {
	float mply = 1;

	// MLR 2003-10-12 Fuel needles are pinned if it's not an F-16
	// it defaults to 10.
	mply=auxaeroData->fuelGaugeMultiplier;
		// original MPS code
		//if(platform->IsF16()) mply = 10;

	*total = fuel + externalFuel;
	//MI fuel's in 100's of lbs
	*total = static_cast<float>((((int)*total + 50) / 100) * 100);
	switch (fuelSwitch) {
	case FS_TEST:
	    *fwdp = *aftp = 2000*mply;
	    *total = 6000;
	    break;
	default:
	case FS_NORM:
	    *fwdp = m_tanks[TANK_FWDRES] + m_tanks[TANK_F1]; // + m_tanks[TANK_WINGFR]; //JPG 7 Jan 04 - We only want FR/AL qty's per -1
	    *aftp = m_tanks[TANK_AFTRES] + m_tanks[TANK_A1]; // + m_tanks[TANK_WINGAL]; // Wing amounts are NOT included when knob is in NORM
	    *fwdp *= mply;
	    *aftp *= mply;
	    break;
	case FS_RESV:
	    *fwdp = m_tanks[TANK_FWDRES];
	    *aftp = m_tanks[TANK_AFTRES];
	    *fwdp *= mply;
	    *aftp *= mply;
	    break;
	case FS_WINGINT:
	    *fwdp = m_tanks[TANK_WINGFR];
	    *aftp = m_tanks[TANK_WINGAL];
	    *fwdp *= mply;
	    *aftp *= mply;
	    break;
	case FS_WINGEXT:
	    *fwdp = m_tanks[TANK_REXT];
	    *aftp = m_tanks[TANK_LEXT];
	    *fwdp *= mply;
	    *aftp *= mply;
	    break;
	case FS_CENTEREXT:
	    *fwdp = m_tanks[TANK_CLINE];
	    *aftp = 0.0f;
	    *fwdp *= mply;
	    *aftp *= mply;
	    break;
	}
    }
}

// fuel display switch
void AirframeClass::IncFuelSwitch()
{
    if (fuelSwitch == FS_LAST)
	fuelSwitch = FS_FIRST;
    else fuelSwitch = (FuelSwitch)(((int)fuelSwitch)+1);
    if (fuelSwitch == FS_TEST) {
		if(!g_bRealisticAvionics)
		{
			platform->mFaults->SetFault(fwd_fuel_low_fault);
			platform->mFaults->SetFault(aft_fuel_low_fault);
		}
		else
		{
			platform->mFaults->SetCaution(fwd_fuel_low_fault);
			platform->mFaults->SetCaution(aft_fuel_low_fault);
		}
    }
}

void AirframeClass::DecFuelSwitch()
{
    if (fuelSwitch == FS_FIRST)
	fuelSwitch = FS_LAST;
    else 
	fuelSwitch = (FuelSwitch)(((int)fuelSwitch)-1);
    if (fuelSwitch == FS_TEST) {
		if(!g_bRealisticAvionics)
		{
			platform->mFaults->SetFault(fwd_fuel_low_fault);
			platform->mFaults->SetFault(aft_fuel_low_fault);
		}
		else
		{
			platform->mFaults->SetCaution(fwd_fuel_low_fault);
			platform->mFaults->SetCaution(aft_fuel_low_fault);
		}
    }
}
// fuel pump switch
void AirframeClass::IncFuelPump()
{
    if (fuelPump == FP_LAST)
	fuelPump = FP_FIRST;
    else 
	fuelPump = (FuelPump)(((int)fuelPump)+1);
}

void AirframeClass::DecFuelPump()
{
    if (fuelPump == FP_FIRST)
	fuelPump = FP_LAST;
    else fuelPump = (FuelPump)(((int)fuelPump)-1);
}

// air source switch
void AirframeClass::IncAirSource()
{
    if (airSource == AS_LAST)
	airSource = AS_FIRST;
    else 
	airSource = (AirSource)(((int)airSource)+1);
}

void AirframeClass::DecAirSource()
{
    if (airSource == AS_FIRST)
	airSource = AS_LAST;
    else airSource = (AirSource)(((int)airSource)-1);
}

// JPO check for trapped fuel
// 5 conditions to be met on the real jet, lets see how close we can get.
// 1. Fuel display must be in normal
// 2. Air refueling not happened in the last 90 seconds
// 3. Fuselage fuel 500lbs < capacity for 30 seconds
// 4. Total fuel 500lbs > fuselage fuel for 30 seconds
// 5. Fuel Flow < 18000pph for 30 seconds.
int AirframeClass::CheckTrapped() 
{
    if (fuelSwitch != FS_NORM ) return 0; // cond 1
    if (externalFuel < 500) return 0; // cond 4
    if (fuelFlow > 18000) return 0; // cond 5

    float fuscap = m_tankcap[TANK_FWDRES] + m_tankcap[TANK_F1] + m_tankcap[TANK_WINGFR]
	+ m_tankcap[TANK_AFTRES] + m_tankcap[TANK_A1] + m_tankcap[TANK_WINGAL];
    if (fuel > fuscap - 500) return 0; // cond 3
    // TODO 30 second timer 
    return 1;
}
//MI Home fuel
int AirframeClass::CheckHome(void)
{
	//Calc how much fuel we have at our selected homepoint
	if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp)
	{
		WayPointClass *wp = platform->GetWayPointNo(
			OTWDriver.pCockpitManager->mpIcp->HomeWP);
		float wpX, wpY, wpZ;
		if(wp)
		{
			wp->GetLocation(&wpX, &wpY, &wpZ);
			//Calculate the distance to it
			float deltaX = wpX - x;
			float deltaY = wpY - y;
			float distanceToSta	= (float)sqrt(deltaX * deltaX + deltaY * deltaY);
			float fuelConsumed;
			if (!IsSet(InAir)){
				// JPO - when we're on the runway or something.
				fuelConsumed = 0;
			}
			else {
				fuelConsumed = distanceToSta / platform->GetVt() * FuelFlow() / 3600.0F;
			}
			HomeFuel =  (int)(platform->GetTotalFuel() - fuelConsumed);

			if(platform->IsF16())
			{
	 			int	fuelOnStation;
	 			float fuelConsumed	= distanceToSta / 6000.0f * 10.0f * 0.67f;
				fuelConsumed += min(1,distanceToSta / 6000.0f / 80.0f) *(500.0f - (-platform->ZPos()) /40.0f*0.5f);
	 			fuelOnStation = (int)(platform->GetTotalFuel() - fuelConsumed);
				HomeFuel = fuelOnStation;
			}

			if(HomeFuel < 800){
				return 1;	//here we get a warning
			}
			else{
				return 0;	//here not
			}
		 }
	}
	return 0;	//dummy
}

float AirframeClass::GetJoker()
{
	float jokerfactor = auxaeroData->jokerFactor;
    return GetAeroData(AeroDataSet::InternalFuel) / jokerfactor; // default 2.0 = about 3500 for F-16
}

float AirframeClass::GetBingo()
{
	float bingofactor = auxaeroData->bingoFactor;
    return GetAeroData(AeroDataSet::InternalFuel) / bingofactor;// default 5.0 = about 1500 for F-16
}

float AirframeClass::GetFumes()
{
	float fumesfactor = auxaeroData->fumesFactor;
    return GetAeroData(AeroDataSet::InternalFuel) / fumesfactor; // default 15.0 = about 500 for F-16
}

//TJL 02/21/04 Home of engine specific modifications
float AirframeClass::EngineRpmMods(float rpmCmd)
{
	//
	float rpmCmdBase = 0.7F;
	float rpmCmdZ = 0.0F;
	float rpmRSE = 0.0F;

	//PW-100/PW-220
	if (auxaeroData->typeEngine == 1 || auxaeroData->typeEngine == 2)
	{
		//TJL -1 says idle RPM increases from 0.84 till it is MIL power at 1.4 Mach
		if (mach >= 0.84f && mach <= 1.4f)
			rpmCmd = max (mach/1.4f, rpmCmd);
		else if (mach > 1.4f)
			rpmCmd = max (0.99f, rpmCmd);
		//TJL -1 says that RPM increases > 10K to give sufficient stall margin
		if (platform->ZPos() <= -100.0F)
		{
			rpmCmdZ = ((-platform->ZPos()/10000.0F)/30) + rpmCmdBase;
			rpmCmd = max (rpmCmdZ, rpmCmd);
		}

		//AB Schedule per -1
		//Area 3 Seg 5 no light
		if ((platform->ZPos() <= -35000.0f && platform->ZPos() >= -45000.0f) &&
			(mach <= 0.8f && mach >0.4f))
			rpmCmd = min (1.025f, rpmCmd); 

		//Area 2 Only Seg 1 will light
		if ((platform->ZPos() <= -45000.0f && platform->ZPos() >= -55000.0f) && 
			(mach <=0.95f && mach > 0.4f))
			rpmCmd = min (1.01f, rpmCmd);

		//Area 1 No AB available
		if ((platform->ZPos() <= -30000.0f && platform->ZPos() > -55000.0f) && mach <= 0.4f)
			rpmCmd = min (0.99f, rpmCmd);
		else if ((platform->ZPos() <= -55000.0f) && mach <= 0.95f)
			rpmCmd = min (0.99f, rpmCmd);

	}//end


	// PW-229/GE-110/GE-129
	if (auxaeroData->typeEngine == 3 || auxaeroData->typeEngine == 4 || auxaeroData->typeEngine == 5)
	{
		//Reduced Speed Excursion Logic 0.5 - 0.6 is the switch range, we'll call it 5.5 for coding
		if (mach > 0.55f && mach < 1.1f)
		{
			rpmRSE = 0.79f;
			rpmCmd = max (rpmCmd, rpmRSE);
		}
		//Idle schedule to MIL from 1.1 to 1.4 MACH
		else if (mach >= 1.1f && mach <= 1.4f)
			rpmCmd = max (mach/1.4f, rpmCmd);
		else if (mach > 1.4f)
			rpmCmd = max (0.99f, rpmCmd);

		//Idle schedule 
		//TJL -1 says that RPM increases > 10K to give sufficient stall margin
		if (platform->ZPos() <= -100.0F)
		{
			//rpmZ = (((SqrtF(-platform->ZPos()))/2000.0F) + rpmBase);
			rpmCmdZ = ((-platform->ZPos()/10000.0F)/30) + rpmCmdBase;
			rpmCmd = max (rpmCmdZ, rpmCmd);
		}

		//Reduce AB schedule
		if ((platform->ZPos() < -50000.0F) && vcas < 250.0F)
		{
			rpmCmd = min (1.01f, rpmCmd);
		}

			
		//Zone 2 AB no lights or delayed lights possible
		if ((platform->ZPos() < -30000.0F) && vcas < 225.0F)
		{
		//	if ((SimLibElapsedTime - engEventTimer) >= 5000)
			if (rpm <= 1.0F && engFlag1 == 0)
			{
				int randnum = rand()%100;
				if (randnum == 1)
					engFlag1 = 1;

				else if (randnum == 2)
					engFlag1 = 2;
				else
					engFlag1 = 3;//this stops the loop

			}

		}
		else
			//Reset flag when out of condition
			engFlag1 = 0;

		//AB no light
		if (engFlag1 == 1)
			rpmCmd = min (0.99f, rpmCmd);
		//AB partial light
		else if (engFlag1 == 2)
			rpmCmd = min (1.01f, rpmCmd);


	}




	return rpmCmd;

}

float AirframeClass::Engine1RpmMods (float rpmCmd)
{
	//
	float rpmCmdBase = 0.7F;
	float rpmCmdZ = 0.0F;
	float rpmRSE = 0.0F;

	//RPM effect for any of the modern engines
	//Note, even the F-4E GE J79 schedules idle speed so keep this effect for engines
	if (auxaeroData->typeEngine == 9 || auxaeroData->typeEngine == 10
		|| auxaeroData->typeEngine == 11)
	{
		//TJL -1 says idle RPM increases from 0.84 till it is MIL power at 1.4 Mach
		if (mach >= 0.84f && mach <= 1.4f)
			rpmCmd = max (mach/1.4f, rpmCmd);
		else if (mach > 1.4f)
			rpmCmd = max (0.99f, rpmCmd);
		//TJL -1 says that RPM increases > 10K to give sufficient stall margin
		if (platform->ZPos() <= -100.0F)
		{
			rpmCmdZ = ((-platform->ZPos()/10000.0F)/30) + rpmCmdBase;
			rpmCmd = max (rpmCmdZ, rpmCmd);
		}

	}

//PW-100/PW-220
	if (auxaeroData->typeEngine == 1 || auxaeroData->typeEngine == 2)
	{
		//TJL -1 says idle RPM increases from 0.84 till it is MIL power at 1.4 Mach
		if (mach >= 0.84f && mach <= 1.4f)
			rpmCmd = max (mach/1.4f, rpmCmd);
		else if (mach > 1.4f)
			rpmCmd = max (0.99f, rpmCmd);
		//TJL -1 says that RPM increases > 10K to give sufficient stall margin
		if (platform->ZPos() <= -100.0F)
		{
			rpmCmdZ = ((-platform->ZPos()/10000.0F)/30) + rpmCmdBase;
			rpmCmd = max (rpmCmdZ, rpmCmd);
		}

		//AB Schedule per -1
		//Area 3 Seg 5 no light
		if ((platform->ZPos() <= -35000.0f && platform->ZPos() >= -45000.0f) &&
			(mach <= 0.8f && mach >0.4f))
			rpmCmd = min (1.025f, rpmCmd); 

		//Area 2 Only Seg 1 will light
		if ((platform->ZPos() <= -45000.0f && platform->ZPos() >= -55000.0f) && 
			(mach <=0.95f && mach > 0.4f))
			rpmCmd = min (1.01f, rpmCmd);

		//Area 1 No AB available
		if ((platform->ZPos() <= -30000.0f && platform->ZPos() > -55000.0f) && mach <= 0.4f)
			rpmCmd = min (0.99f, rpmCmd);
		else if ((platform->ZPos() <= -55000.0f) && mach <= 0.95f)
			rpmCmd = min (0.99f, rpmCmd);

	}//end


	// PW-229/GE-110/GE-129
	if (auxaeroData->typeEngine == 3 || auxaeroData->typeEngine == 4 || auxaeroData->typeEngine == 5)
	{
		//Reduced Speed Excursion Logic 0.5 - 0.6 is the switch range, we'll call it 5.5 for coding
		if (mach > 0.55f && mach < 1.1f)
		{
			rpmRSE = 0.79f;
			rpmCmd = max (rpmCmd, rpmRSE);
		}
		//Idle schedule to MIL from 1.1 to 1.4 MACH
		else if (mach >= 1.1f && mach <= 1.4f)
			rpmCmd = max (mach/1.4f, rpmCmd);
		else if (mach > 1.4f)
			rpmCmd = max (0.99f, rpmCmd);

		//Idle schedule 
		//TJL -1 says that RPM increases > 10K to give sufficient stall margin
		if (platform->ZPos() <= -100.0F)
		{
			//rpmZ = (((SqrtF(-platform->ZPos()))/2000.0F) + rpmBase);
			rpmCmdZ = ((-platform->ZPos()/10000.0F)/30) + rpmCmdBase;
			rpmCmd = max (rpmCmdZ, rpmCmd);
		}

		//Reduce AB schedule
		if ((platform->ZPos() < -50000.0F) && vcas < 250.0F)
		{
			rpmCmd = min (1.01f, rpmCmd);
		}

			
		//Zone 2 AB no lights or delayed lights possible
		if ((platform->ZPos() < -30000.0F) && vcas < 225.0F)
		{
		//	if ((SimLibElapsedTime - engEventTimer) >= 5000)
			if (rpm <= 1.0F && engFlag1 == 0)
			{
				int randnum = rand()%100;
				if (randnum == 1)
					engFlag1 = 1;

				else if (randnum == 2)
					engFlag1 = 2;
				else
					engFlag1 = 3;//this stops the loop

			}

		}
		else
			//Reset flag when out of condition
			engFlag1 = 0;

		//AB no light
		if (engFlag1 == 1)
			rpmCmd = min (0.99f, rpmCmd);
		//AB partial light
		else if (engFlag1 == 2)
			rpmCmd = min (1.01f, rpmCmd);
	}



	//F18A-D MIL at 1.23 mach
	if (auxaeroData->typeEngine == 6 || auxaeroData->typeEngine == 7
		|| auxaeroData->typeAC == 8 || auxaeroData->typeAC == 9)
	{
		if (mach >= 0.9f && mach <= 1.23f)
			rpmCmd = max (mach/1.23f, rpmCmd);
		else if (mach > 1.23f)
			rpmCmd = max (0.99f, rpmCmd);

		//RPM increases > 10K to give sufficient stall margin
		if (platform->ZPos() <= -100.0F)
		{
			rpmCmdZ = ((-platform->ZPos()/10000.0F)/30) + rpmCmdBase;
			rpmCmd = max (rpmCmdZ, rpmCmd);
		}

	}

	//F18E/F RPM to MIL at 1.23 mach
	if (auxaeroData->typeEngine == 8 || auxaeroData->typeAC == 10)
	{
		if (mach >= 1.18f && mach <= 1.23f)
			rpmCmd = max (mach/1.23f, rpmCmd);
		else if (mach > 1.23f)
			rpmCmd = max (0.99f, rpmCmd);

		//RPM increases > 10K to give sufficient stall margin
		if (platform->ZPos() <= -100.0F)
		{
			rpmCmdZ = ((-platform->ZPos()/10000.0F)/30) + rpmCmdBase;
			rpmCmd = max (rpmCmdZ, rpmCmd);
		}

	}


	// F-14 spools engine up when Mach < 0.9 and AOA over 18
	if ((auxaeroData->typeAC == 6 || auxaeroData->typeAC == 7) || 
		(auxaeroData->typeEngine == 9 || auxaeroData->typeEngine == 10))
	{
		if (alpha >= 18.0F && mach <= 0.9F)
			rpmCmd = max (0.85f, rpmCmd);

		//Rich Stability cutback reduced operation
		if ((platform->ZPos() <= -40000.0f) && mach <= 0.45f)
			rpmCmd = min (1.015f, rpmCmd);
		else if ((platform->ZPos() <= -45000.0f) && mach <= 0.6f)
			rpmCmd = min (1.015f, rpmCmd);
		else if ((platform->ZPos() <= -50000.0f) && mach <= 1.0f)
			rpmCmd = min (1.01f, rpmCmd);
		else if ((platform->ZPos() <= -55000.0f) && mach <= 1.1f)
			rpmCmd = min (1.01f, rpmCmd);
		else if ((platform->ZPos() <= -60000.0f) && mach <= 1.2f)
			rpmCmd = min (1.01f, rpmCmd);

	}
	
	// F-4E Engine Stall Zone 
	if (auxaeroData->typeEngine == 11 || auxaeroData->typeAC == 11)
	{
		if ((SimLibElapsedTime - engEventTimer) >= 1000)
		{
			if(platform->ZPos() < -10000.0F && vcas < 150.0F)
			{
				if (1 == (rand() % 30))
				{
					SetFlag(EngineStopped);
					engEventTimer = SimLibElapsedTime;
				}
				else
					engEventTimer = SimLibElapsedTime;
			}
			
			if (platform->ZPos() < -10000.0F && alpha > 28.0F && rpm >= 1.0f)
			{
				if (1 == (rand() % 30))
				{
					SetFlag(EngineStopped);
					engEventTimer = SimLibElapsedTime;
				}
				else
					engEventTimer = SimLibElapsedTime;
			}
			else
				engEventTimer = SimLibElapsedTime;
		}
		
		if (platform->ZPos() < -50000.0F && vcas < 300.0F)
		{
			rpmCmd = min (0.99F, rpmCmd);

		}
	}


	return rpmCmd;
}

float AirframeClass::Engine2RpmMods (float rpmCmd2)
{
	//
	//
	float rpmCmdBase = 0.7F;
	float rpmCmdZ = 0.0F;
	float rpmRSE = 0.0F;

	//RPM effect for any of the modern engines
	if (auxaeroData->typeEngine == 9 || auxaeroData->typeEngine == 10
		|| auxaeroData->typeEngine == 11)
	{
		//TJL -1 says idle RPM increases from 0.84 till it is MIL power at 1.4 Mach
		if (mach >= 0.84f && mach <= 1.4f)
			rpmCmd2 = max (mach/1.4f, rpmCmd2);
		else if (mach > 1.4f)
			rpmCmd2 = max (0.99f, rpmCmd2);
		//TJL -1 says that RPM increases > 10K to give sufficient stall margin
		if (platform->ZPos() <= -100.0F)
		{
			rpmCmdZ = ((-platform->ZPos()/10000.0F)/30) + rpmCmdBase;
			rpmCmd2 = max (rpmCmdZ, rpmCmd2);
		}

	}
//PW-100/PW-220
	if (auxaeroData->typeEngine == 1 || auxaeroData->typeEngine == 2)
	{
		//TJL -1 says idle RPM increases from 0.84 till it is MIL power at 1.4 Mach
		if (mach >= 0.84f && mach <= 1.4f)
			rpmCmd2 = max (mach/1.4f, rpmCmd2);
		else if (mach > 1.4f)
			rpmCmd2 = max (0.99f, rpmCmd2);
		//TJL -1 says that RPM increases > 10K to give sufficient stall margin
		if (platform->ZPos() <= -100.0F)
		{
			rpmCmdZ = ((-platform->ZPos()/10000.0F)/30) + rpmCmdBase;
			rpmCmd2 = max (rpmCmdZ, rpmCmd2);
		}

		//AB Schedule per -1
		//Area 3 Seg 5 no light
		if ((platform->ZPos() <= -35000.0f && platform->ZPos() >= -45000.0f) &&
			(mach <= 0.8f && mach >0.4f))
			rpmCmd2 = min (1.025f, rpmCmd2); 

		//Area 2 Only Seg 1 will light
		if ((platform->ZPos() <= -45000.0f && platform->ZPos() >= -55000.0f) && 
			(mach <=0.95f && mach > 0.4f))
			rpmCmd2 = min (1.01f, rpmCmd2);

		//Area 1 No AB available
		if ((platform->ZPos() <= -30000.0f && platform->ZPos() > -55000.0f) && mach <= 0.4f)
			rpmCmd2 = min (0.99f, rpmCmd2);
		else if ((platform->ZPos() <= -55000.0f) && mach <= 0.95f)
			rpmCmd2 = min (0.99f, rpmCmd2);

	}//end


	// PW-229/GE-110/GE-129
	if (auxaeroData->typeEngine == 3 || auxaeroData->typeEngine == 4 || auxaeroData->typeEngine == 5)
	{
		//Reduced Speed Excursion Logic 0.5 - 0.6 is the switch range, we'll call it 5.5 for coding
		if (mach > 0.55f && mach < 1.1f)
		{
			rpmRSE = 0.79f;
			rpmCmd2 = max (rpmCmd2, rpmRSE);
		}
		//Idle schedule to MIL from 1.1 to 1.4 MACH
		else if (mach >= 1.1f && mach <= 1.4f)
			rpmCmd2 = max (mach/1.4f, rpmCmd2);
		else if (mach > 1.4f)
			rpmCmd2 = max (0.99f, rpmCmd2);

		//Idle schedule 
		//TJL -1 says that RPM increases > 10K to give sufficient stall margin
		if (platform->ZPos() <= -100.0F)
		{
			//rpmZ = (((SqrtF(-platform->ZPos()))/2000.0F) + rpmBase);
			rpmCmdZ = ((-platform->ZPos()/10000.0F)/30) + rpmCmdBase;
			rpmCmd2 = max (rpmCmdZ, rpmCmd2);
		}

		//Reduce AB schedule
		if ((platform->ZPos() < -50000.0F) && vcas < 250.0F)
		{
			rpmCmd2 = min (1.01f, rpmCmd2);
		}

			
		//Zone 2 AB no lights or delayed lights possible
		if ((platform->ZPos() < -30000.0F) && vcas < 225.0F)
		{
		//	if ((SimLibElapsedTime - engEventTimer) >= 5000)
			if (rpm2 <= 1.0F && engFlag2 == 0)
			{
				int randnum = rand()%100;
				if (randnum == 1)
					engFlag2 = 1;

				else if (randnum == 2)
					engFlag2 = 2;
				else
					engFlag2 = 3;//this stops the loop

			}

		}
		else
			//Reset flag when out of condition
			engFlag2 = 0;

		//AB no light
		if (engFlag2 == 1)
			rpmCmd2 = min (0.99f, rpmCmd2);
		//AB partial light
		else if (engFlag2 == 2)
			rpmCmd2 = min (1.01f, rpmCmd2);
	}


	//F18A-D MIL at 1.23 mach
	if (auxaeroData->typeEngine == 6 || auxaeroData->typeEngine == 7
		|| auxaeroData->typeAC == 8 || auxaeroData->typeAC == 9)
	{
		if (mach >= 0.9f && mach <= 1.23f)
			rpmCmd2 = max (mach/1.23f, rpmCmd2);
		else if (mach > 1.23f)
			rpmCmd2 = max (0.99f, rpmCmd2);

		//RPM increases > 10K to give sufficient stall margin
		if (platform->ZPos() <= -100.0F)
		{
			rpmCmdZ = ((-platform->ZPos()/10000.0F)/30) + rpmCmdBase;
			rpmCmd2 = max (rpmCmdZ, rpmCmd2);
		}

	}


	//F18E/F RPM to MIL at 1.23 mach
	if (auxaeroData->typeEngine == 8 || auxaeroData->typeAC == 10)
	{
		if (mach >= 1.18f && mach <= 1.23f)
			rpmCmd2 = max (mach/1.23f, rpmCmd2);
		else if (mach > 1.23f)
			rpmCmd2 = max (0.99f, rpmCmd2);

		//RPM increases > 10K to give sufficient stall margin
		if (platform->ZPos() <= -100.0F)
		{
			rpmCmdZ = ((-platform->ZPos()/10000.0F)/30) + rpmCmdBase;
			rpmCmd2 = max (rpmCmdZ, rpmCmd2);
		}

	}

	// F-14 spools engine up when Mach < 0.9 and AOA over 18
	if ((auxaeroData->typeAC == 6 || auxaeroData->typeAC == 7) || 
		(auxaeroData->typeEngine == 9 || auxaeroData->typeEngine == 10))
	{
		if (alpha >= 18.0F && mach <= 0.9F)
			rpmCmd2 = max (0.85f, rpmCmd2);

			//Rich Stability cutback reduced operation
		if ((platform->ZPos() <= -40000.0f) && mach <= 0.45f)
			rpmCmd2 = min (1.015f, rpmCmd2);
		else if ((platform->ZPos() <= -45000.0f) && mach <= 0.6f)
			rpmCmd2 = min (1.015f, rpmCmd2);
		else if ((platform->ZPos() <= -50000.0f) && mach <= 1.0f)
			rpmCmd2 = min (1.01f, rpmCmd2);
		else if ((platform->ZPos() <= -55000.0f) && mach <= 1.1f)
			rpmCmd2 = min (1.01f, rpmCmd2);
		else if ((platform->ZPos() <= -60000.0f) && mach <= 1.2f)
			rpmCmd2 = min (1.01f, rpmCmd2);

	}

		// F-4E Engine Stall Zone 
	if (auxaeroData->typeEngine == 11 || auxaeroData->typeAC == 11)
	{
		if ((SimLibElapsedTime - engEventTimer2) >= 1000)
		{
			if(platform->ZPos() < -10000.0F && vcas < 150.0F)
			{
				if (1 == (rand() % 30))
				{
					SetEngineFlag(EngineStopped2);
					engEventTimer2 = SimLibElapsedTime;
				}
				else
					engEventTimer2 = SimLibElapsedTime;
			}

			if (platform->ZPos() < -10000.0F && alpha > 28.0F && rpm2 >= 1.0f)
			{
				if (1 == (rand() % 30))
				{
					SetEngineFlag(EngineStopped2);
					engEventTimer2 = SimLibElapsedTime;
				}
				else
					engEventTimer2 = SimLibElapsedTime;
			}
			else
				engEventTimer2 = SimLibElapsedTime;
		}
		
		if (platform->ZPos() < -50000.0F && vcas < 300.0F)
		{
			rpmCmd2 = min (0.99F, rpmCmd2);

		}
	}



	return rpmCmd2;
}

//TJL 09/11/04 Added so different aircraft can calculate their FTIT.
//temps in Celcius and toss values 0 - 12 (so gauge uses these instead of 0 - 1200)
//Reads from Auxaero .dat files.  Start(0-0.68), Idle (0.69 - 0.85), Max (0.86 - 1.03)
float AirframeClass::CalcFtit(float tmpLeft, float tmpRight)
	{
			//left
			//start to idle
			if (rpm < 0.69f)
				ftitLeft = rpm * auxaeroData->FTITStart;
			else if (rpm >=0.69f && rpm < 0.85f)
				ftitLeft = rpm * auxaeroData->FTITIdle;
			else 
				ftitLeft = rpm * auxaeroData->FTITMax;

			//Right
			//start to idle
			if (rpm2 < 0.69f)
				ftitRight = rpm2 * auxaeroData->FTITStart;
			else if (rpm2 >=0.69f && rpm2 < 0.85f)
				ftitRight = rpm2 * auxaeroData->FTITIdle;
			else 
				ftitRight = rpm2 * auxaeroData->FTITMax;

		return (tmpLeft, tmpRight);
	}

