#include "stdhdr.h"
#include "simmath.h"
#include "hdigi.h"
#include "simveh.h"
#include "campwp.h"
#include "object.h"
#include "otwdrive.h"

#include "SimDrive.h"

float HeliBrain::VectorTrack(float, int)
{
	return( 0.0f );
}

float HeliBrain::AutoTrack(float)
{
	float rng, desHeading;
	float rollLoad;
	float rollDir;
	float desSpeed;

	desSpeed = 1.0f;
	rollDir = 0.0f;
	rollLoad = 0.0f;

	// Range to current waypoint
	rng = (trackX - self->XPos()) * (trackX - self->XPos()) + (trackY - self->YPos()) *	(trackY - self->YPos());

	// Heading error for current waypoint
	desHeading = (float)atan2 ( trackY - self->YPos(), trackX - self->XPos()) - self->Yaw();
	if (desHeading > 180.0F * DTR)
		desHeading -= 360.0F * DTR;
	else if (desHeading < -180.0F * DTR)
		desHeading += 360.0F * DTR;

	// rollLoad is normalized (0-1) factor of how far off-heading we are to target
	rollLoad = desHeading / (90.0F * DTR);
	if (rollLoad < 0.0F)
		rollLoad = -rollLoad;
	if ( desHeading > 0.0f )
		rollDir = 1.0f;
	else
		rollDir = -1.0f;

	desSpeed = rng/(500.0f * 500.0f);
	desSpeed = min(desSpeed, 1.0f);

	rollLoad *= desSpeed;
	rollLoad = min( 1.0f, rollLoad );

	// if we're close, just point to spot then go
	if (fabs(rollLoad) > 0.1f && rng < 1000.0f * 1000.0f)
		desSpeed = 0.0f;

	LevelTurn (rollLoad, rollDir, TRUE);
	MachHold(desSpeed, self->GetWPalt(), TRUE);
	//MachHold(desSpeed, 300.0f, TRUE);

	return(0.0f);
}

// RV - Biker - New MachHold which now controlls speed and alltitude
// altSet is alltitude above ground -> positive value
void HeliBrain::MachHold (float speedSet, float altSet, int groundAvoid)
{
	float altGround;
	float altOffset;
	float altAct;
	
	float powerT;
	float powerO;
	
	float COEFF_P = 0.05f;
	float COEFF_I = 0.01f;
	float COEFF_FB = 0.01f;

	// coefficients if we do need ground avoid
	if (groundAvoid) {
		COEFF_P = 0.01f;
		//COEFF_I = 0.00005f;
		COEFF_I = 0.005f;
		COEFF_FB = 0.01f;
	}

	// other coefficients if we don't need ground avoid
	else {
		COEFF_P = 0.01f;
		//COEFF_I = 0.000002f;
		COEFF_I = 0.0002f;
		COEFF_FB = 0.005f;
	}
		
	// do height calculation in pos values
    altGround = -OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
										  self->YPos() + self->YDelta());
	
	// actual height above ground
	altAct = -self->ZPos() -self->offsetZ - altGround;

	// control error
	altOffset = (altSet - altAct);

	// output before limitation
	powerT = powerI + altOffset * COEFF_P;

	// limit output powerO
	if (powerT > 0.0f)
		powerO = min(powerT, 0.5f);
	else
		powerO = max(powerT, -0.5f);
	
	// integral
	powerI = powerI + COEFF_I * (altOffset * COEFF_P - (powerT - powerO) * COEFF_FB)*SimLibMajorFrameTime;

	// limit integral on control error
	if (abs(altOffset) < 100) {
		if (powerI > 0.0f)
			powerI = min(powerI, 0.5f);
		else
			powerI = max(powerI, -0.5f);
	}
	else {
		if (powerI > 0.0f)
			powerI = min(powerI, 1.0f);
		else
			powerI = max(powerI, -1.0f);
	}

	// 0.5 throttle is neutral, 1.0 if full up, 0.0 is full down
	throtl = 0.5f + powerO;

	// reduce speed if we have to climb
	if (powerO >= 0.0f && groundAvoid)
		speedSet = speedSet * (1.0f - powerO*2.0f);

	// limit speed values min -1 max +1
	if ( speedSet > 1.0f )
		speedSet = 1.0f;
	else if ( speedSet < -1.0f )
		speedSet = -1.0f;

	// -1.0 pStick is full speed forward, 1.0 pStick full speed back	
	pStick = -speedSet;

	return;
}

//int HeliBrain::MachHold (float m1, float, int)
//{
//
//	m1 = -m1;
//	if ( m1 > 1.0f )
//		m1 = 1.0f;
//	else if ( m1 < -1.0f )
//		m1 = -1.0f;
//
//	pStick = m1;
//
//
//	return TRUE;
//
//}

void HeliBrain::Loiter (void)
{
	LevelTurn (0.0f, 0.0f, TRUE);
	MachHold(0.0f, 150.0F, TRUE);
}

void HeliBrain::LevelTurn(float load_factor, float turnDir, int)
{
	load_factor *= turnDir;

	if ( load_factor > 1.0f )
		load_factor = 1.0f;
	else if ( load_factor < -1.0f )
		load_factor = -1.0f;

	rStick = load_factor;
}

//int HeliBrain::AltitudeHold (float desAlt)
//{
//	float alterr;
//	// float altdamp;
//	int retval = 0;
//
//
//	/*
//	// get altitude difference and normalize to 5000 ft
//	alterr = (desAlt - self->ZPos()) * 0.0001;
//
//	// damp normalized to 50ft/sec
//	altdamp = (self->ZDelta() * 0.05) * ( 1.0 - fabs(alterr) );
//
//	throtl += -(5000.0 * alterr) + altdamp;
//
//   	if ( throtl > 1.0 )
//   	{
//		   throtl = 1.0;
//   	}
//   	else if ( throtl < 0.0 )
//   	{
//		   throtl = 0.0;
//   	}
//	*/
//
//	// normalized to 1000ft
//	alterr = (desAlt - self->ZPos()) * 0.001F;
//
//	// 0.5 throtl is neutral, 1.0 if full up, 0.0 is full down
//	// if we want to move up, alterr will be negative
//	throtl = 0.5F - (alterr * 0.5F);
//
//	// check throttle between 0 and 1
//   	if ( throtl > 1.0 )
//   	{
//		   throtl = 1.0;
//   	}
//   	else if ( throtl < 0.0 )
//   	{
//		   throtl = 0.0;
//   	}
//
//	return (retval);
//}

void HeliBrain::GammaHold (float desGamma)
{
	float elevCmd;

	desGamma = max ( min ( desGamma, 30.0F), -30.0F);
	elevCmd = desGamma - self->GetGamma() * RTD;

	elevCmd *= 0.25F * self->GetKias() / 350.0F;
	elevCmd /= self->platformAngles.cosphi;

	// MonoLocate (35, 1);
	// MonoPrint ("%.2f %.2f %.2f\n", desGamma, af->gmma*RTD, elevCmd);

	if (elevCmd > 0.0F)
		elevCmd *= elevCmd;
	else
		elevCmd *= -elevCmd;
}

void HeliBrain::RollOutOfPlane(void)
{
	float eroll;

	// first pass, save roll
	if (lastMode != RoopMode) {
		mnverTime = 4.0F;

		// want to roll toward the vertical but limit to keep
		// droll < 45 degrees.
		if (self->Roll() >= 0.0) {
			newroll = self->Roll() - 45.0F*DTR;
		}
		else {
			newroll = self->Roll() + 45.0F*DTR;
		}
	}
    
	// roll error
	eroll = newroll - self->Roll();

	// roll the shortest direction
	if (eroll < -180.0F*DTR)
		eroll += 360.0F*DTR;
	else if (eroll > 180.0F*DTR)
		eroll -= 360.0F*DTR;

	// exit mode
	mnverTime -= SimLibMajorFrameTime;

	if (mnverTime > 0.0) {
		AddMode (RoopMode);
	}
}

void HeliBrain::OverBank (float delta)
{
	float eroll=0.0F;

	if (targetData == NULL)
		return;

	// not in a vertical fight
	if (fabs(self->Pitch()) < 70.0*DTR) {
		// Find a new roll angle
		if (lastMode != OverBMode) {
			if (self->Roll() > 0.0F)
				newroll = targetData->droll + delta;
			else
				newroll = targetData->droll - delta;

			if (newroll > 180.0F * DTR)
				newroll -= 360.0F * DTR;
			else if (newroll < -180.0F * DTR)
				newroll += 360.0F * DTR;
		}

		eroll = newroll - self->Roll();
	}
	// vertical fight

	// exit
	if (fabs(eroll) > 1.0) {
		AddMode(OverBMode);
	}
}
