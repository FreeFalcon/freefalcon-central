#include "stdhdr.h"
#include "simmath.h"
#include "hdigi.h"
#include "simveh.h"
#include "campwp.h"
#include "object.h"
#include "otwdrive.h"

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

   // OutputDebugString( "In Auto Track\n" );

   desSpeed = 1.0f;
   rollDir = 0.0f;
   rollLoad = 0.0f;

	/*---------------------------*/
	/* Range to current waypoint */
	/*---------------------------*/
	rng = (trackX - self->XPos()) * (trackX - self->XPos()) + (trackY - self->YPos()) *	(trackY - self->YPos());

	/*------------------------------------*/
	/* Heading error for current waypoint */
	/*------------------------------------*/
	desHeading = (float)atan2 ( trackY - self->YPos(), trackX - self->XPos()) - self->Yaw();
	if (desHeading > 180.0F * DTR)
		desHeading -= 360.0F * DTR;
	else if (desHeading < -180.0F * DTR)
		desHeading += 360.0F * DTR;

	// rollLoad is normalized (0-1) factor of how far off-heading we are
	// to target
	rollLoad = desHeading / (90.0F * DTR);
	if (rollLoad < 0.0F)
		rollLoad = -rollLoad;
	if ( desHeading > 0.0f )
		rollDir = 1.0f;
	else
		rollDir = -1.0f;

	desSpeed = rng/( 500.0f * 500.0f );
	desSpeed = min( desSpeed, 1.0f );

	rollLoad *= desSpeed;
	rollLoad = min( 1.0f, rollLoad );

	// if we're close, just point to spot then go
	if ( fabs(rollLoad) > 0.1f && rng < 1000.0f * 1000.0f )
		desSpeed = 0.0f;

	LevelTurn (rollLoad, rollDir, TRUE);
    AltitudeHold(trackZ);
	MachHold(desSpeed, 0.0F, FALSE);

	return( 0.0f );
}

int HeliBrain::MachHold (float m1, float, int)
{

	m1 = -m1;
	if ( m1 > 1.0f )
		m1 = 1.0f;
	else if ( m1 < -1.0f )
		m1 = -1.0f;

	pStick = m1;


	return TRUE;

}

void HeliBrain::Loiter (void)
{
	LevelTurn (0.1f, 1.0f, TRUE);
    // AltitudeHold(holdAlt);
    AltitudeHold( OTWDriver.GetGroundLevel(self->XPos(), self->YPos() ) - 150.0f );
	MachHold(0.1f, 0.0F, FALSE);
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

int HeliBrain::AltitudeHold (float desAlt)
{
	float alterr;
	// float altdamp;
	int retval = 0;


	/*
	// get altitude difference and normalize to 5000 ft
	alterr = (desAlt - self->ZPos()) * 0.0001;

	// damp normalized to 50ft/sec
	altdamp = (self->ZDelta() * 0.05) * ( 1.0 - fabs(alterr) );

	throtl += -(5000.0 * alterr) + altdamp;

   	if ( throtl > 1.0 )
   	{
		   throtl = 1.0;
   	}
   	else if ( throtl < 0.0 )
   	{
		   throtl = 0.0;
   	}
	*/

	// normalized to 1000ft
	alterr = (desAlt - self->ZPos()) * 0.001F;

	// 0.5 throtl is neutral, 1.0 if full up, 0.0 is full down
	// if we want to move up, alterr will be negative
	throtl = 0.5F - (alterr * 0.5F);

	// check throttle between 0 and 1
   	if ( throtl > 1.0 )
   	{
		   throtl = 1.0;
   	}
   	else if ( throtl < 0.0 )
   	{
		   throtl = 0.0;
   	}

	return (retval);
}

void HeliBrain::GammaHold (float desGamma)
{
float elevCmd;

   desGamma = max ( min ( desGamma, 30.0F), -30.0F);
	elevCmd = desGamma - self->GetGamma() * RTD;

	elevCmd *= 0.25F * self->Kias() / 350.0F;
   elevCmd /= self->platformAngles->cosphi;
/*
MonoLocate (35, 1);
MonoPrint ("%.2f %.2f %.2f\n", desGamma, af->gmma*RTD, elevCmd);
*/
	if (elevCmd > 0.0F)
		elevCmd *= elevCmd;
	else
		elevCmd *= -elevCmd;

	/*
   gammaHoldIError += 0.0025F*elevCmd;
   if (gammaHoldIError > 1.0F)
      gammaHoldIError = 1.0F;
   else if (gammaHoldIError < -1.0F)
      gammaHoldIError = -1.0F;
	*/

/*
MonoLocate (35, 2);
MonoPrint ("%.2f %.2f %.2f\n", eintg, elevCmd, af->pstick);
MonoLocate (35, 3);
MonoPrint ("%.2f %.2f %.2f\n", af->x, af->y, af->z);
MonoLocate (35, 4);
MonoPrint ("%.2f %.2f %.2f %8ld\n", af->nxcgb, af->nycgb, af->nzcgb, SimLibElapsedTime);
*/
}

void HeliBrain::RollOutOfPlane(void)
{
float eroll;

   /*-----------------------*/
   /* first pass, save roll */
   /*-----------------------*/
   if (lastMode != RoopMode)
   {
      mnverTime = 4.0F;

      /*----------------------------------------------------*/
      /* want to roll toward the vertical but limit to keep */
      /* droll < 45 degrees.                                */
      /*----------------------------------------------------*/
      if (self->Roll() >= 0.0)
      {
         newroll = self->Roll() - 45.0F*DTR;
      }
      else 
      {
         newroll = self->Roll() + 45.0F*DTR;
      }
   }
    
   /*------------*/
   /* roll error */
   /*------------*/
   eroll = newroll - self->Roll();

   /*-----------------------------*/
   /* roll the shortest direction */
   /*-----------------------------*/
   if (eroll < -180.0F*DTR)
      eroll += 360.0F*DTR;
   else if (eroll > 180.0F*DTR)
      eroll -= 360.0F*DTR;

   /*-----------*/
   /* exit mode */
   /*-----------*/
   mnverTime -= SimLibMajorFrameTime;

   if (mnverTime > 0.0) 
   {
      AddMode (RoopMode);
   }
}

void HeliBrain::OverBank (float delta)
{
float eroll=0.0F;

   if (targetData == NULL)
      return;

   /*-------------------------*/
   /* not in a vertical fight */
   /*-------------------------*/
   if (fabs(self->Pitch()) < 70.0*DTR)
   {
      /*-----------------------*/
      /* Find a new roll angle */
      /*-----------------------*/
      if (lastMode != OverBMode)
      {
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
   /*----------------*/
   /* vertical fight */
   /*----------------*/
   else 
   {
   }

   /*------*/
   /* exit */
   /*------*/
   if (fabs(eroll) > 1.0)
   {
      AddMode(OverBMode);
   }
}
