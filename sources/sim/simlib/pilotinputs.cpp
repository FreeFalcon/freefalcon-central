#include "stdhdr.h"
#include "PilotInputs.h"
#include "simio.h"
#include "simmath.h"
#include "Sim/Include/Fcc.h"

PilotInputs UserStickInputs;
extern int	UseKeyboardThrottle;
extern float throttleOffset;
extern float rudderOffset;
extern float rudderOffsetRate;
extern float pitchStickOffset;
extern float rollStickOffset;
extern float pitchStickOffsetRate;
extern float rollStickOffsetRate;
extern float throttleOffsetRate;
extern int keyboardPickleOverride;
extern int keyboardTriggerOverride;
extern float pitchElevatorTrimRate;
extern float pitchAileronTrimRate;
extern float pitchRudderTrimRate;
extern float pitchManualTrim;	//MI
extern float yawManualTrim;		//MI
extern float rollManualTrim;	//MI
int PickleOverride;
int TriggerOverride;

// Keyboard stuff
extern float g_frollStickOffset;
extern float g_fpitchStickOffset;
extern float g_frudderOffset;

PilotInputs::PilotInputs (void)
{
   pickleButton = Off;
   pitchOverride = Off;
   missileStep = Off;
   speedBrakeCmd = Center;
   missileOverride = Center;
   trigger = Center;
   tmsPos = Center;
   trimPos = Center;
   dmsPos = Center;
   cmmsPos = Center;
   cursorControl = Center;
   micPos = Center;
   manRange = 0.0F;
   antennaEl = 0.0F;
   pstick = 0.0F;
   rstick = 0.0F;
   throttle = 0.0F;
   rudder = 0.0F;
   ptrim = 0.0f;
   rtrim = 0.0f;
   ytrim = 0.0f;
   PickleTime=0;

   // Retro 12Jan2004 -- aargh I hate falcon
	PickleOverride = 0;
	keyboardPickleOverride = 0;
	trigger = Center;
	pickleButton = Off;

   for (int i = 0; i < 2; i++)	// Retro 12Jan2004
   {
	 engineThrottle[i] = 0.0f;
   }

   currentlyActiveEngine = Both_Engines;	// Retro 12Jan2004
}

PilotInputs::~PilotInputs (void)
{
}

#include "SimDrive.h"	 // Retro 31Dec2003 - needed for TrimAPDisc stuff
#include "aircrft.h"	 // Retro 31Dec2003 - ditto
#include "airframe.h"	 // Retro 7Feb2004



void PilotInputs::Update(){
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	// Retro 31Dec2003
	if (IO.AnalogIsUsed(AXIS_PITCH)){
		pstick = Math.DeadBand(IO.ReadAnalog(AXIS_PITCH), -0.05F, 0.05F) * 1.05F; 	// Retro 31Dec2003
	}
	else {
		pitchStickOffset += pitchStickOffsetRate * SimLibMajorFrameTime;
		pitchStickOffset = max ( min (pitchStickOffset, 1.0F), -1.0F);
		pstick = pitchStickOffset;
		if (pitchStickOffsetRate == 0.0F){
			//pitchStickOffset *= 0.9F;
			pitchStickOffset *= g_fpitchStickOffset;
		}
	}
	pstick = max ( min (pstick, 1.0F), -1.0F);

	// Retro 31Dec2003
	if (IO.AnalogIsUsed(AXIS_ROLL)){	
		rstick = Math.DeadBand(IO.ReadAnalog(AXIS_ROLL), -0.05F, 0.05F) * 1.05F;
	}
	else {
		rollStickOffset += rollStickOffsetRate * SimLibMajorFrameTime;
		rollStickOffset = max ( min (rollStickOffset, 1.0F), -1.0F);
		rstick = rollStickOffset;
		if (rollStickOffsetRate == 0.0F){
			//rollStickOffset *= 0.9F;
			rollStickOffset *= g_frollStickOffset;
		}
	}
	rstick = max ( min (rstick, 1.0F), -1.0F);

	/*******************************************************************************/
   	//	Retro 12Jan2004 - featuring left/right throttle axis =)
	//	=======================================================
	//
	//	New dual throttle pilotinput code:
	//	Operates like this: the throttle values are stored in an arry
	//	engineThrottle[2], index into it with enum Engine_t, (Left_Engine/Right_Engine)
	//	
	//	Now if a user has 2 throttles everything is peachy.. one throttle per axis
	//	If the user has 1 throttle a three-state variable (currentlyActiveEngine)
	//	controls which throttle receives the update: left/right or both
	//
	//	If the user does NOT have a throttle, this 3-state variable controls which
	//	axis gets updated BY THE KEYBOARD
	//
	//	Stuff like having one axis controls one throttle and the keyboard controlling
	//	the other is not considered.
	//
	//	This 3-state variable should also control various other avionics-related
	//	operations so that we don´t have to introduce 6.23*10^23 new keypresses
	//
	//	Access functions to get the current controlled axis and to set it are provided
	//	insider the pilotinput class. The enum is within the class scope !
	/*******************************************************************************/
	if (playerAC) {
		AirframeClass *af = playerAC->af;
		if ((af) && (af->GetNumberEngines() == 2))
		{
			/*******************************************************************************/
			//	keyboard only, right engine axis is not even evalutated !	
			/*******************************************************************************/
			if ((!IO.AnalogIsUsed(AXIS_THROTTLE))||(UseKeyboardThrottle))
			{
				throttleOffset += throttleOffsetRate;	  
				throttleOffset = max ( min (throttleOffset, 1.5F), 0.0F);

				if (currentlyActiveEngine == Both_Engines){
					engineThrottle[Left_Engine] = engineThrottle[Right_Engine] = throttleOffset;
				}
				else{
					engineThrottle[currentlyActiveEngine] = throttleOffset;
				}
			}
			/*******************************************************************************/
			// both axis mapped. state of 3-way variable not important
			// keyboard not considered
			/*******************************************************************************/
			else if ((IO.AnalogIsUsed(AXIS_THROTTLE2))&&(IO.AnalogIsUsed(AXIS_THROTTLE))){
				engineThrottle[Left_Engine] = IO.ReadAnalog(AXIS_THROTTLE);
				engineThrottle[Right_Engine] = IO.ReadAnalog(AXIS_THROTTLE2);
			}
			/*******************************************************************************/
			// only one throttle axis, 3-way variable decides which engine is controlled
			// keyboard not considered
			/*******************************************************************************/
			else if (IO.AnalogIsUsed(AXIS_THROTTLE)){
				if (currentlyActiveEngine == Both_Engines){
					engineThrottle[Left_Engine] = engineThrottle[Right_Engine] = IO.ReadAnalog(AXIS_THROTTLE);
				}
				else{
					engineThrottle[currentlyActiveEngine] = IO.ReadAnalog(AXIS_THROTTLE);
				}
			}

			// keep them values sane..
			engineThrottle[Left_Engine] = max ( min (engineThrottle[0], 1.5F), 0.0F);
			engineThrottle[Right_Engine] = max ( min (engineThrottle[1], 1.5F), 0.0F);

			//TJL 01/17/04 Adding this to get the old engine/throttle code to work
			// Retro 7Feb2004 - a bit cleaner but still not really happy about that..
			throttle = engineThrottle[currentlyActiveEngine];
		}
		else { // end Retro 12Jan2004 (this is the old, single-engine code)
			if (IO.AnalogIsUsed (AXIS_THROTTLE) && !UseKeyboardThrottle){	// Retro 31Dec2003
				//throttle = 1.5F - (IO.ReadAnalog(2) * 1.05F + 1.0F) * 0.75F;
				throttle = IO.ReadAnalog(AXIS_THROTTLE);	// Retro 31Dec2003
			}
			else {
				throttleOffset += throttleOffsetRate;	  
				throttleOffset = max ( min (throttleOffset, 1.5F), 0.0F);
				throttle = throttleOffset;
			}
			engineThrottle[Left_Engine] = engineThrottle[Right_Engine] = 0;	// Retro 7Feb2004
			throttle = max ( min (throttle, 1.5F), 0.0F);
		}
	}	// Retro 12Jan2004

	if (IO.AnalogIsUsed (AXIS_YAW)){
		// Retro 31Dec2003
		rudder = Math.DeadBand(-IO.ReadAnalog(AXIS_YAW), -0.03F, 0.03F) * 1.05F;
	}
	else {
		rudderOffset += rudderOffsetRate * SimLibMajorFrameTime;
		rudderOffset = max ( min (rudderOffset, 1.0F), -1.0F);
		rudder = rudderOffset;
		if (rudderOffsetRate == 0.0F){
			//rudderOffset *= 0.9F;
			rudderOffset *= g_frudderOffset;
		}
	}
	rudder = max ( min (rudder, 1.0F), -1.0F);

	/*******************************************************************************/
	// Retro 31Dec2003 - trimming with analogue axis
	//
	// OK.. trying to make sense of that trim issue
	//	there are two ways.. the trim hat on the stick and the 3 dials on the left console
	//
	//	) with the TrimAPDisc(onnected) switch set to DISC ONLY the dial-input is considered
	//	) else, the sum of the two inputs is considered
	/*******************************************************************************/
	extern bool g_bRealisticAvionics;

	if (IO.AnalogIsUsed(AXIS_TRIM_PITCH) == false){	// trimming with keyboard (as before)
		ptrim += pitchElevatorTrimRate * SimLibMajorFrameTime;
		ptrim += pitchManualTrim * SimLibMajorFrameTime;	//MI
		pitchManualTrim = 0.0F;	//MI
		ptrim = max( min(ptrim, 0.5f), -0.5f);
	}
	else {
		if (
			(!g_bRealisticAvionics)||
			((playerAC) && (!playerAC->TrimAPDisc))
		){
			ptrim += pitchElevatorTrimRate * SimLibMajorFrameTime;
		}
		ptrim = IO.GetAxisValue(AXIS_TRIM_PITCH)/20000.f;
		ptrim = max( min(ptrim, 0.5f), -0.5f);
	}
	   
	if (IO.AnalogIsUsed(AXIS_TRIM_ROLL) == false){	// trimming with keyboard (as before)
		rtrim += pitchAileronTrimRate * SimLibMajorFrameTime;
		rtrim += rollManualTrim * SimLibMajorFrameTime;	//MI
		rollManualTrim = 0.0F;	//MI
		rtrim = max( min(rtrim, 0.5f), -0.5f);
	}
	else {
		if (
			(!g_bRealisticAvionics)||	// TrimAPDisc only works in realistic avionics..
			((playerAC) && (!playerAC->TrimAPDisc))
		){
			rtrim += pitchAileronTrimRate * SimLibMajorFrameTime;
		}

		rtrim = IO.GetAxisValue(AXIS_TRIM_ROLL)/20000.f;
		rtrim = max( min(rtrim, 0.5f), -0.5f);
	}

	if (IO.AnalogIsUsed(AXIS_TRIM_YAW) == false){	// trimming with keyboard (as before)
		ytrim += pitchRudderTrimRate * SimLibMajorFrameTime;
		ytrim += yawManualTrim * SimLibMajorFrameTime;	//MI
		yawManualTrim = 0.0F;	//MI
		ytrim = max( min(ytrim, 0.5f), -0.5f);
	}
	else {
		if (
			(!g_bRealisticAvionics)||	// TrimAPDisc only works in realistic avionics..
			((playerAC) && (!playerAC->TrimAPDisc))
		){
			ytrim += pitchRudderTrimRate * SimLibMajorFrameTime;
		}

		ytrim += IO.GetAxisValue(AXIS_TRIM_YAW)/20000.f;
		ytrim = max( min(ytrim, 0.5f), -0.5f);
	}
	// Retro 31Dec2003 ends..

	//all joystick button functionality is now in sijoy
	//if (IO.ReadDigital(0) || keyboardTriggerOverride)
	if (keyboardTriggerOverride || TriggerOverride){
		trigger = Down;
	}
	else {
		trigger = Center;
		// COBRA - RED - The Pickle Stuff
		// RV - I-Hawk - Added a check to allow ARH "Maddog" launch only in boresight mode
        if ((SimDriver.GetPlayerEntity()) && (SimDriver.GetPlayerEntity()->IsAirplane())){
			if (keyboardPickleOverride || PickleOverride){
				if (!PickleTime) PickleTime=SimLibElapsedTime;
				else if((SimLibElapsedTime-PickleTime) > playerAC->FCC->GetPickleTime() && pickleButton==Off &&
						 playerAC->FCC->AllowMaddog() ) 
					pickleButton = On;
			}
			else{
				pickleButton = Off;
				PickleTime=0;
			}
		}
	}
}


void PilotInputs::Reset(void)
{
	throttleOffset = 0.0F;
	rudderOffset = 0.0F;
	rudderOffsetRate = 0.0F;
	pitchStickOffset = 0.0F;
	rollStickOffset = 0.0F;
	pitchStickOffsetRate = 0.0F;
	rollStickOffsetRate = 0.0F;
	throttleOffsetRate = 0.0F;
	pitchElevatorTrimRate = 0.0f;
	pitchAileronTrimRate = 0.0f;
	pitchRudderTrimRate = 0.0f;
	keyboardPickleOverride = 0;
	keyboardTriggerOverride = 0;
	ptrim = ytrim = rtrim = 0.0f;
	PickleTime=0;

	// Retro 12Jan2004 -- aargh I hate falcon
	PickleOverride = 0;
	keyboardPickleOverride = 0;
	trigger = Center;
	pickleButton = Off;

	for (int i = 0; i < 2; i++)		// Retro 12Jan2004
	{
		engineThrottle[i] = 0.0f;
	}

	currentlyActiveEngine = Both_Engines;	// Retro 12Jan2004
}

/*******************************************************************************/
//	Retro 12Jan2004 (all)
//	Access method to cycle the 3-state variable that controls the engine that should
//	receive updates from axis/keyboard
/*******************************************************************************/
void PilotInputs::cycleCurrentEngine()
{
	if ((SimDriver.GetPlayerEntity())/*&&(SimDriver.GetPlayerEntity()->acFlags & hasTwoEngines)*/)
	{
		switch (currentlyActiveEngine)
		{
		case Left_Engine:	currentlyActiveEngine = Right_Engine;	break;
		case Right_Engine:	currentlyActiveEngine = Both_Engines;	break;
		case Both_Engines:	currentlyActiveEngine = Left_Engine;	break;
		default:			ShiAssert(false);
		}
	}
}